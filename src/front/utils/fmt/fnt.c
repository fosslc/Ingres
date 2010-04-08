/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<cv.h>
# include	<me.h>
# include	<clfloat.h>
# include	<mh.h>
# include	<st.h>
# include	<er.h>
# include	<fmt.h>
# include	<multi.h>
# include	<adf.h>
# include	<afe.h>
# include	"format.h"

FUNC_EXTERN	ADF_CB	*FEadfcb();

/*
**   F_NT - write into a string a number under the numeric editing control.
**	    For example,
**		$,$$$,$$$.nnCR
**		+$,$$$.00
**		+ $Z,ZZZ,ZZZ.ZZ
**
**
**	    An edit field is made up of the following special characters:
**
**		n - put out a numeric character (zero if none).
**			FC_DIGIT.
**		z - numeric character, or blank when zero.
**			FC_ZERO.
**		FC_CURRENCY - floating dollar sign, or numeric.
**			FC_CURRENCY.
**		- - floating minus sign on negative, or space, or numeric.
**			FC_MINUS.
**		+ - floating sign, or numeric.
**			FC_PLUS.
**		, - true comma, if between numerics.
**			FC_SEPARATOR.
**		. - true decimal.
**			FC_DECIMAL.
**		* - print asterisk, or numeric.
**			FC_STAR.
**		' ' - true blank.
**			FC_BLANK.
**		(, ), <, >, {, }, [, or ] -
**		    floating on negative, space on positive, or numeric.
**			FC_OPAREN, FC_CPAREN, FC_OANGLE, FC_CANGLE, FC_OCURLY,
**			FC_CCURLY, FC_OSQUARE, FC_CSQUARE, respectively.
**		CR - credit symbol, if negative.
**			No symbol defined.
**		DB - debit symbol, if negative.
**			No symbol defined.
**		Note: for CR and DB the case used on input is used on output.
**		NOTE: Throughout this code, FC_SEPARATOR and FC_DECIMAL
**			are only compared against characters in the format
**			string.
**
**	Any character preceded by a backslash (\) will be passed
**	through directly.  In the fmt template, the character will be preceded
**	by FC_ESCAPE.
**
**
**	Parameters:
**		cb	- pointer to an ADF_CB control block.
**		value	- pointer to a u_char buffer containing the ASCII
**			  representation of the number to fit to the template.
**		fmt	- pointer to an FMT structure containing template.
**		result	- Pointer to a DB_DATA_VALUE of type DB_TEXT_STRING
**			  to contain edited number.  Must be wide enough for
**			  template length + 1.
**		is_zero	- TRUE if the numeric value is exactly zero.
**		is_neg	- TRUE if the numeric value is strictly less than zero.
**			  However, for input masking purposes, is_zero and
**			  is_neg may be set simultaneously when an initial
**			  minus sign is entered.
**
**	Returns:
**		OK
**		E_FM1000_FORMAT_OVERFLOW
**		E_FM600A_CORRUPTED_TEMPLATE
**
**	Called by:
**		f_format.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		none.
**
**	History:
**		4/25/81 (ps) - written.
**		3/1/83 (gac) - allow \\ in numeric template.
**		3/22/83 (mmm) - made compat lib conversion
**				-changed MHfdint calls to MHfdint
**		7/2/85  (jrc) - Changes for international support.
**				Output of FC_CURRENCY, FC_SEPARATOR,
**				FC_DECIMAL is from values in multinational
**				structure
**		8/28/87 (gac)	added parentheses, angle brackets and curly
**				brackets for negative numbers.
**		9/8/88 (elein)	JB2940: added support for integers with trailing
**				brackets, minus signs, and plus signs
**		11/10/89 (elein) B8616: Trailing money, especially two
**				character money symbols had garbage instead
**				of the appropriate symbol.
**		20-nov-90 (bruceb/joe)
**			When obtaining individual characters from a float
**			via MHfdint() usage, if the result is out of the
**			range 0-9, treat it as a 0.  This handles the cases
**			where the number is greater than the precision of
**			floats on the machine.  Added getdigit() to do this.
**		31-jan-1991 (Joe)
**	    	     Integrated above porting change (w4gl03 360019).
**		17-aug-91 (leighb) DeskTop Porting Change:
**			move static func decl outside of calling function.
**		8-27-91 (tom) added function to handle input masking,
**				called from window manager specific code.
**		1-21-92 (tom) changed the way that we convert the number in
**				fmt_ntmodify.. now we parse the input ourselves
**				instead of using fmt_cvt.
**		1-27-92 (tom) need to ignore the first digit if it is 0 so
**				cursor positioning will work out correctly.
**		1-31-92 (tom) implement -0.0, and support displaying
**				a null string if no digits are found.
**		08-jun-92 (leighb) DeskTop Porting Change: added cv.h
**		8-13-92 (tom)
**			fix bad bug introduced when I allowed the
**			decimal point to be positioned at the end of the number.
**		27-apr-93 (rudyw)
**			Fix for bug 49141. Check if the minus sign for a 
**			negative number results in field overflow.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	7-oct-1993 (rdrane)
**		Correct handling of trailing signs - trailing plus sign always
**		prints sign corresponding to value, trailing minus prints only
**		if value is minus.  b55129.
**	12-oct-1993 (rdrane)
**		Extensive modifications to preserve the precision and value
**		of the input numeric value.  We no longer unconditionally
**		convert to float prior to formatting, but rather use the
**		appropriate CV routine for integer, decimal, and float.
**		Note that this results in changed parameterization for
**		f_nt().  Obsolete local variables were removed.  Made
**		decimal_sym a local variable which is set each time from the
**		ADF_CB, rather than a static which is fixed on the first call.
**		Note that it will always be present/specified (part of FEadfcb()
**		processing).  Eliminated static routine getdigit(), since
**		we're now doing what it said we should have done in the first
**		place - use the value's ASCII string representation.  This
**		eliminates rounding of the value which is wrong for integers,
**		wrong for decimal, and now superfluous for float, since
**		f_format() calls CVfa() which calls fcvt() which always rounds.
**		Thus, the number we see is already at the precision of the FMT
**		template.  Similar story for decimal and CVpka().
**	23-feb-1994 (rdrane)
**		Ensure that v_ptr is set to v_dec only when v_dec is not NULL
**		(b59993).  Also, fill FC_ZERO's to the right of the decimal
**		point with zeroes, not FC_ZERO's.
**	1-apr-1994 (rdrane)
**		Add is_fract to indicate a value which has no integer portion,
**		and note that is_zero has none either (b61977).  Check for an
**		initial input or a minus sign when called for input masking
**		(aspect of b56659).
**	26-jan-95 (chan) b65248
**		Fix in fmt_ntmodify() - set is_neg only if FC_MINUS
**		is the first digit seen.  Otherwise for templates
**		such as 'nnn\-nn\-nnnn', only the first eight digits
**		can be entered.
**	26-jan-95 (chan) b64139
**		Fix in fmt_ntmodify() - Special case when ch_input
**		is decimal_sym for the case where the template is
**		'zzzz.zz', the decimal-only values such as 0.25 are
**		not accepted.
**	26-Nov-1996 (strpa05)
**		Check for an integer portion produced by CVround. An integer
**		portion may have been introduced via rounding. Is_fract is
**		generated by testing the floating point value so will be unaware
**		of the integer. Introduce is_roundup and test for char before
**		dp > '0'. If so then allow a single integer position.
**		(Bug 79404)
**	31-mar-1997 (donc)
**		Integrate OpenROAD fixes, conditionally execute fix for
**		bug 66590 which although it may fix a character-based tool
**		bug, breaks OPenROAD
**	05-dec-1997 (mulde06) Bug# 87418
**		Code did not handle floating point mask formats once the number 
**		reached the 10 thousand place.  For instance, with the following mask,
**		(((,(((,((n.nn) the formating would be fine until the 5th number
**		was entered.  If you were entering consequtive numbers, then entering
**		1 would give you	1.00, entering 12 would give you 12.00, 123 -> 123.00, 
**		1234 -> 1,234.00 and 12345 -> 1,234.50 instead of 12,345.00
**  26-jan-1998 (mulde06) Bug# 85259
**		Cross-integrated 276406 from ingres63p library
**		Fixed cursor positioning problem with zzz.zz formant and input 
**		masking turned on.
**	09-feb-1998 (somsa01)
**	    Last change introduced a C++ style comment. This needed to be
**	    changed, since it broke the build on UNIX.
**	20-mar-1998 (i4jo01)
**	    Change 432980 was lost due to change 433989. Reinstated the fix 
**	    for bug 85526.
**	22-dec-1998 (crido01) SIR 94437
**	    Add the '^' character to the numeric templates which indicates
**	    that remaining decimal digits be rounded-up, as opposed to the
**	    default truncation.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      10-jun-1999 (dupbr01) Bug #93240
**	    Replace one part of the change of Bug 87418 which was done 
**	    specifically for OpenRoad in fmt_ntmodify when we test each 
**	    character of the string to know where to put the cursor. 
**	    So We replace the comparaison to ',' by a comparaison to 
**	    'thou_sym' which can be a '.' or a ',' according the value of 
**	    II_DECIMAL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-Feb-2001 (zhahu02) For bug 98731, XIntegration change 274292 bug 66537
**          Display plus or minus sign only once when using templates with 
**          blankspaces (barpe06 for 2.0). 
**    06-Feb-2001 (barpe06) Bug 103582
**         Display the MINUS sign with template: +"zzzzz". This was lost before.
**      10-jul-2001 (bolke01) Bug #104753
**	    check for sign character in rightmost location in template, this 
**	    may be the ione of -rRbB  . If '-' then set flag to indicate that 
**	    sign is righthand. If one of the debit/credit chars are identified 
**	    then check for the correct preceding character ( cCdD ). If 
**	    detected then set the flag as for '-'
**    30-Oct-2001 (horda03) Bug 106214
**       Undo above fix. The template +"zzzzz" should not display the sign as
**       the '+' is outside the "" (double quotes) and means Right Justify the
**       filed. The 'z' character inside the numeric template means that either
**       a digit or a space is displayed (no mention of the sign). If the MINUS
**       sign is required then the above template can be re-written as +"-----".
**       For more information see the "Numeric Templates" documentation.
**	31-oct-2001 (somsa01)
**	    Corrected last x-integration to properly remove code that was
**	    removed in the original submission.
**	29-May-2002 (hanje04)
**	    Corrected fix for 104753 which was using CMprev value with only
**	    1 agrgument. Second argument should be starting position and fails
**	    to compile on Linux without it.
**	    Having looked at Macro defn it would also fail for all DOUBLEBYTE
**	    builds.
**	24-jun-2002 (malsa02) bug 107872
**	    In fmt_ntmodify(), removed warning for space characters in input 
**	    string. This was causing a beep everytime a character was entered
**	    in a OpenROAD entryfield if the associated field template had
**	    space characters in it.
**	18-Dec-2002 (hanch04)
**	    Fix bad cross integration
**	21-apr-2003 (malsa02) Bug 110107
**	    Fix for bug 97274 causes problems with some integer templates.
**	    I have reworked fix for 97274.
**	13-May-2003 (hanje04)
**	   Replaced nat introduced by fix for Bug 110107 i4.
**	22-mar-2004 (malsa02) bug 109794
**	    In fmt_ntmodify(), we were incorrectly dropping the sign indicator when we 
**	    encounted an input value of '0.00..'.
**      31-aug-2004 (sheco02)
**          X-integrate change 467349 to main and line up history.
*/


static bool	GoodPos();


STATUS
f_nt(cb, value, fmt, result, is_zero, is_neg, is_fract)
ADF_CB		*cb;
u_char		*value;
FMT		*fmt;
DB_TEXT_STRING	*result;
bool		is_zero;
bool		is_neg;
bool		is_fract;

{
	i4			i;
	STATUS		status;
	i4		mny_lort;	/* leading or trailing money sign */
	char		*mny_sym;	/* money symbol */
	char		*t_ptr;		/* ptr to template */
	char		*t_dec;		/* ptr to decimal point in template */
	char		*tprev;		/* ptr to previous char in template */
	char		*rightmost;	/* JB2940: rightmost char of template */
	u_char		*r_ptr;		/* ptr to result */
	u_char		*r_dec;		/* ptr to decimal point in result */
	u_char		*re_ptr;	/* end of result string */
	u_char		*v_ptr;		/* ptr to value */
	u_char		*v_dec;		/* ptr to decimal point in value */
	u_char		*ve_ptr;	/* end of value string */
	char		sign;		/* sign character */
	bool		sign_lort = FALSE; 	/* leading or trailing sign (def=left)*/
	char		chkchar;	/* used in filling of characters */
	char		fillchar;	/* used in filling of characters */
	char		thous_sym;	/* thousands separator symbol */
	char		decimal_sym;	/* decimal point symbol */
	bool		blankit;
	bool		is_roundup = FALSE;     /* integer portion via roundup */
        bool            sign_displayed=FALSE;   /* has sign already been displayed? */

	/*
	** Set up numeric and money symbols, default decimal
	** point locations, and default status return value.
	*/
	decimal_sym = cb->adf_decimal.db_decimal;
	thous_sym = FC_SEPARATOR;
	if  (decimal_sym != FC_DECIMAL)
	{
		thous_sym = FC_DECIMAL;
	}
	mny_sym = cb->adf_mfmt.db_mny_sym;
	mny_lort = cb->adf_mfmt.db_mny_lort;

	t_dec = NULL;
	r_dec = (u_char *)NULL;
	v_dec = (u_char *)NULL;

	status = OK;

	/*
	** Preset the result string.
	*/
	r_ptr = result->db_t_text;
	result->db_t_count = fmt->fmt_width;
	MEfill((u_i2)fmt->fmt_width, (u_char)FC_FILL, (PTR)r_ptr);
	re_ptr = r_ptr + result->db_t_count;

	/*
	** Find the rightmost decimal point in the template.  At the
	** same time, look for any numeric codes.  If found, then turn
	** off the zero blanking option.
	*/
	blankit = TRUE;
	t_ptr = fmt->fmt_var.fmt_template;
	i = 0;
	while (i < fmt->fmt_width)
	{
		switch(*t_ptr)
		{
		case FC_ESCAPE:
			CMnext(t_ptr);	/* skip escaped character */
			break;
		case FC_ROUNDUP:
		case FC_DIGIT:
			blankit = FALSE;
			break;
		case FC_DECIMAL:
			t_dec = t_ptr;
			r_dec = result->db_t_text + i;
		default:
			break;
		}
		CMbyteinc(i, t_ptr);
		CMnext(t_ptr);
	}

        /* bolke01 - check for sign character in rightmost location in
        **           template.    
        **           is  sign  > sign_lort = TRUE;
        **           not  sign > sign_lort = FALSE;
        */
        CMprev(t_ptr,fmt->fmt_var.fmt_template);
        if (*t_ptr == FC_MINUS) 
                sign_lort = TRUE ;
        else if ((*t_ptr == 'b') || (*t_ptr == 'B')) 
        {       CMprev(t_ptr,fmt->fmt_var.fmt_template);
                if((*t_ptr == 'd') || (*t_ptr == 'D')) 
                        sign_lort = TRUE ;
        }
        else if((*t_ptr == 'r') || (*t_ptr == 'R')) 
        {       CMprev(t_ptr,fmt->fmt_var.fmt_template);
                if((*t_ptr == 'c') || (*t_ptr == 'C')) 
                        sign_lort = TRUE ;
        }

	/*
	** If zero and the template is consistent with "blank if zero",
	** then simply return, as the result has already initialized to
	** blanks.
	*/
	if ((blankit) && (is_zero) && (!is_neg))
	{
		return(status);
	}

	/*
	** Establish the decimal point (if any) in the numeric value, as
	** well as the value's sign.  Note that if the value is negative,
	** then the ASCII representation already begins with a minus
	** sign, so reset the value pointer to skip over it before we go
	** looking for any decimal point.
	*/
	sign = FC_PLUS;
	if  (is_neg)
	{
		sign = FC_MINUS;
		CMnext(value);
	}
	v_ptr = value;
	ve_ptr = value + STlength((char *)value);
	while (v_ptr < ve_ptr)
	{
		if  (*v_ptr == decimal_sym)
		{
			v_dec = v_ptr;
			break;
		}
		CMnext(v_ptr);
	}



	/*
	** Set result and template pointers to the decimal point.
	** If there isn't one, position to the rightmost characters
	** of value, template, and result strings in preparation of
	** processing the "integer" portion of the value.  If there
	** is one, position to the decimal point character of the
	**  value, template, and result strings and process the
	** "decimal" portion of the value from left to right.
	*/
	if  (v_dec == (u_char *)NULL)
	{
		v_ptr = ve_ptr;
	}
	else
	{
		v_ptr = v_dec;
		CMnext(v_ptr);
	}

	if (t_dec == NULL)
	{
		t_ptr = fmt->fmt_var.fmt_template +
		        STlength(fmt->fmt_var.fmt_template);
		/* JB2940 - set pointer to rightmost character */
		rightmost = t_ptr;
		CMprev(rightmost, fmt->fmt_var.fmt_template);
		r_ptr = result->db_t_text + fmt->fmt_width;
	}
	else
	{
		/*
		** Copy the initial decimal point (if we're here,
		** there has to be one!) and get it out of the way.
		*/
		t_ptr = t_dec;
		CMnext(t_ptr);
		r_ptr = r_dec;
		*r_ptr = decimal_sym;
		CMnext(r_ptr);
		while (r_ptr < re_ptr)
		{
			switch(*t_ptr)
			{
				case(FC_ESCAPE):
					/*
					** Skip the escape character and
					** unconditionally copy the temmplate
					** character.  Not that this makes
					** sense when we're to the right of
					** the decimal point ...
					*/
					CMnext(t_ptr);
					CMcpychar(t_ptr, r_ptr);
					break;

				case(FC_ROUNDUP):
				case(FC_DIGIT):
				case(FC_CURRENCY):	/* GAC - deleted 'T' */
				case(FC_ZERO):
				case(FC_STAR):
					/*
					** If we have a digit, put it in.
					** Otherwise, fill with a zero
					*/
					if  (v_ptr < ve_ptr)
					{
						*r_ptr = *v_ptr;
						CMnext(v_ptr);
						break;
					}
					*r_ptr = '0';
					break;

				case(FC_PLUS):
					*r_ptr = sign;
					break;

				case(FC_MINUS):
				case(FC_OPAREN):
				case(FC_CPAREN):
				case(FC_OANGLE):
				case(FC_CANGLE):
				case(FC_OCURLY):
				case(FC_CCURLY):
				case(FC_OSQUARE):
				case(FC_CSQUARE):
					if (sign == FC_MINUS)
					{
						*r_ptr = *t_ptr;
					}
					else
					{
						*r_ptr = FC_FILL;
					}
					break;

				case('c'):
				case('C'):
				case('d'):
				case('D'):
					/*
					** Must be CR or DB symbol
					** (already checked)
					*/
					if (sign == FC_MINUS)
					{	/* put in CR or DB on neg */
						CMcpyinc(t_ptr,r_ptr);
						*r_ptr = *t_ptr;
					}
					else
					{	/* blank out the field */
						*r_ptr = FC_FILL;
						CMnext(r_ptr);
						*r_ptr = FC_FILL;
					}	/* blank out the field */
					break;

				case(FC_DECIMAL):
					/*
					** We should never see this - we're
					** starting from the rightmost
					** decimal point as it is!
					*/
					*r_ptr = decimal_sym;
					break;

				default:
					CMcpychar(t_ptr, r_ptr);
					break;

			}
			CMnext(r_ptr);
			CMnext(t_ptr);
		}

		/*
		** JB2940 - set pointer to rightmost character.
		** What we're really doing is ensuring that we'll
		** never match with t_ptr, it's only important for
		** templates with no decimal portion.
		**/
		rightmost = NULL;

		t_ptr = t_dec;
		r_ptr = r_dec;
		if  (v_dec != (u_char *)NULL)
		{
			v_ptr = v_dec;
		}
	}


	/*
	** Now do the integer part of the number, processing
	** from right to left.  The value, template, and result
	** pointers should all be positioned to the character just
	** beyond the end character of the integer portion - the EOS
	** if we had no decimal portion, and the decimal point if we
	** did.  So, adjust accordingly.  Skip any processing of the value
	** if there is no integer part.  Note that is_zero will also never
	** have an integer part, so check that too!  Yes, we should put
	** this whole code (after adjusting the template) into an if
	** statement to avoid checking is_fract and is_zero on each iteration,
	** but the code is indented enough and how long are the templates
	** expected to be really?
	*/
	CMprev(t_ptr, fmt->fmt_var.fmt_template);
	r_ptr -= CMbytecnt(t_ptr);
	CMprev(v_ptr,value);

	/* Check for an integer portion produced by CVround eg. 0.999 to 
	** precision 2 gives 1.00 , is_fract will not know about this !
	*/

	if ((*v_ptr > '0') && (is_fract))
		is_roundup = TRUE;

	while (((!is_fract)||is_roundup)  && (!is_zero) && (v_ptr >= value))
	{
		if  (t_ptr < fmt->fmt_var.fmt_template)
		{
			/*
			** We still have digits, but we're out
			** of template characters
			*/
			status = E_FM1000_FORMAT_OVERFLOW;
			break;
		}


		tprev = t_ptr;
		CMprev(tprev, fmt->fmt_var.fmt_template);

		/*
		** Check for escaped characters.  The correct order is:
		** o	Copy the escaped template character
		** o	Decrement the template and result pointers by the
		**	bytecount of the copied escaped template character.
		** o	Decrement the template pointer by the size of the
		**	escape character (always one; combine with previous
		**	decrement operation)
		*/
		if  ((t_ptr > fmt->fmt_var.fmt_template) &&
		     (*tprev == FC_ESCAPE))
		{
			CMcpychar(t_ptr, r_ptr);
			i = CMbytecnt(t_ptr);
			t_ptr -= (i + 1);
			r_ptr -= i;
			continue;
		}

		switch(*t_ptr)
		{
			case(FC_ROUNDUP):
			case(FC_DIGIT):
			case(FC_ZERO):
			case(FC_CURRENCY):	/* GAC - deleted 'T' */
			case(FC_STAR):
			case(FC_OPAREN):
			case(FC_OANGLE):
			case(FC_OCURLY):
			case(FC_OSQUARE):
				/*
				** Output the next digit
				*/
				*r_ptr = *v_ptr;
				CMprev(v_ptr,value);
				/* Just one shot at that roundup digit */
				is_roundup = FALSE;
				break;

			case(FC_CPAREN):
			case(FC_CANGLE):
			case(FC_CCURLY):
			case(FC_CSQUARE):
				/* if no decimal part and this is rightmost
				 * character, put out bracket or filler
				 * otherwise, get next character JB2940
				 */
				if( (t_dec == NULL) && (t_ptr == rightmost) )
				{
					if (sign == FC_MINUS)
					{
						*r_ptr = *t_ptr;
					}
					else
					{
						*r_ptr = FC_FILL;
					}
				}
				else
				{
					/*
					** Output the next digit
					*/
					*r_ptr = *v_ptr;
					CMprev(v_ptr,value);
					/* Just one shot at that roundup digit */
					is_roundup == FALSE;
				}
				break;
			case(FC_MINUS):
			case(FC_PLUS):
				/* if no decimal part and this is rightmost
				 * character and this is not a "floating"
				 * sign, put out sign or filler.
				 * otherwise, get next character JB2940
				 */
				if( (t_dec == NULL) && (t_ptr == rightmost) &&
				    (tprev >= fmt->fmt_var.fmt_template) &&
				   (*tprev != FC_MINUS) && (*tprev != FC_PLUS) )
				{
					/*
					**		VALUE
					**		    | +	| -
					**	S	----+---+---
					**	I	+   | + | -
					**	G	----+---+---
					**	N	-   |   | -
					**
					** Result matrix.  sign ==> VALUE
					** b55129.	   t_ptr==> SIGN
					**		   r_ptr==> RESULT
					*/
					if (sign == FC_MINUS)
					{
						*r_ptr = FC_MINUS;
					}
					else if (*t_ptr == FC_PLUS)
					{
						*r_ptr = FC_PLUS;
					}
					else
					{
						*r_ptr = FC_FILL;
					}
				}
				else
				{
					/*
					** Output the next digit
					*/
					*r_ptr = *v_ptr;
					CMprev(v_ptr,value);
					/* Just one shot at that roundup digit */
					is_roundup == FALSE;
				}
				break;

			case('r'):
			case('R'):
			case('B'):
			case('b'):
				/* got a CR or DB template */
				if (sign == FC_MINUS)
				{
					*r_ptr-- = *t_ptr--;
					*r_ptr = *t_ptr;
				}
				else
				{
					*r_ptr-- = FC_FILL;
					*r_ptr = FC_FILL;
					t_ptr--;
				}
				tprev = t_ptr;
				CMprev(tprev, fmt->fmt_var.fmt_template);
				break;

			case(FC_SEPARATOR):
				*r_ptr = thous_sym;
				break;

			case(FC_DECIMAL):
				*r_ptr = decimal_sym;
				break;

			default:
				if (CMspace(t_ptr))
				{
					CMcpychar(t_ptr, r_ptr);
				}
				else
				{
					status = E_FM600A_CORRUPTED_TEMPLATE;
				}
				break;

		}
		t_ptr = tprev;
		r_ptr -= CMbytecnt(t_ptr);
	}	/* get next digit */

	/* Check if a minus sign causes display field overflow */
	if  ((sign == FC_MINUS) && (t_ptr < fmt->fmt_var.fmt_template) && (!sign_lort))
	{
		status = E_FM1000_FORMAT_OVERFLOW;
	}


	/* done with number. process rest of template */
	while (t_ptr >= fmt->fmt_var.fmt_template)
	{
		tprev = t_ptr;
		CMprev(tprev, fmt->fmt_var.fmt_template);
		if  ((t_ptr > fmt->fmt_var.fmt_template) &&
		     (*tprev == FC_ESCAPE))
		{
			CMcpychar(t_ptr, r_ptr);
			i = CMbytecnt(t_ptr);
			t_ptr -= (i + 1);
			r_ptr -= i;
			continue;
		}

		/* preset the chkchar and fillchar */
		chkchar = fillchar = '\0';

		switch(*t_ptr)
		{
			case(FC_MINUS):
			case(FC_OPAREN):
			case(FC_CPAREN):
			case(FC_OANGLE):
			case(FC_CANGLE):
			case(FC_OCURLY):
			case(FC_CCURLY):
			case(FC_OSQUARE):
			case(FC_CSQUARE):
				if (*(r_ptr+1) == thous_sym)
				{	/* get rid of commas  */
					CMnext(r_ptr);
				}
				if (sign == FC_MINUS)
                                {
                                        if (!sign_displayed)
                                        {
                                                chkchar = *r_ptr = *t_ptr;
                                                sign_displayed = TRUE;
                                        }
        				/*
					** If sign has already been displayed then use a blank.
        				*/
                                        else
        				{
                                                chkchar = *r_ptr = ' ';
        				}
                                }
                                else
                                {
                                        *r_ptr = FC_FILL;
                                }
				chkchar = *t_ptr;
                                break;

			case(FC_CURRENCY):	/* GAC - deleted 'T' */
			{
				i4	l;
				i4	cl;
				u_char	*cp;
				u_char	*cp1;

				if (*(r_ptr+1) == thous_sym)
				{	/* get rid of commas */
					CMnext(r_ptr);
				}
				l = r_ptr - result->db_t_text + 1;
				cl = STlength(mny_sym);
				if (l < cl)
				{
					status = E_FM1000_FORMAT_OVERFLOW;
					break;
				}
				cp1 = r_ptr + 1;
				cp = r_ptr - (cl - 1);
				r_ptr = cp;
				/*
				** If the currency symbol belongs at the end
				** (M_MNY_TRAILING) then move the result inplace
				*/
				if (mny_lort == DB_TRAIL_MONY)
				{
					/* b8616: changed from for (;*cp1;)
					 * which didn't work because
					 * cp1 didn't necessarily have
					 * an EOS!  re_ptr is set earlier
					 */
					for (; cp1 < re_ptr ; )
					{
						CMcpyinc(cp1,cp);
					}
				}
				MEcopy((PTR) mny_sym, (u_i2) cl, (PTR) cp);
				chkchar = FC_CURRENCY;	/* GAC - deleted 'T' */
				break;
			}

			case(FC_PLUS):
                                if (*(r_ptr+1) == thous_sym)
                                {       /* get rid of comma */
                                        CMnext(r_ptr);
                                }
                                *r_ptr = sign;
                                chkchar = FC_PLUS;
				break;

			case(FC_ROUNDUP):
			case(FC_DIGIT):
				*r_ptr = '0';
				break;

			case(FC_SEPARATOR):
				*r_ptr = thous_sym;
				break;

			case(FC_DECIMAL):
				*r_ptr = decimal_sym;
				break;

			case('r'):
			case('R'):
			case('B'):
			case('b'):
				/* end of CR or DB template */
				if (sign == FC_MINUS)
				{
					*r_ptr-- = *t_ptr--;
					*r_ptr = *t_ptr;
				}
				else
				{
					*r_ptr-- = FC_FILL;
					*r_ptr = FC_FILL;
					t_ptr--;
				}
				tprev = t_ptr;
				CMprev(tprev, fmt->fmt_var.fmt_template);
				break;

			case(FC_ZERO):
				if (*(r_ptr+1) == thous_sym)
				{	/* git rid of comma */
					*(r_ptr+1) = FC_FILL;
				}
				*r_ptr = FC_FILL;
				break;

			case(FC_STAR):
				if (*(r_ptr+1) == thous_sym)
				{	/* get rid of commas */
					CMnext(r_ptr);
				}
				*r_ptr = FC_STAR;
				chkchar = FC_STAR;
				fillchar = FC_STAR;
				break;

			default:
				if (CMspace(t_ptr))
				{
					CMcpychar(t_ptr, r_ptr);
				}
				else
				{
					status = E_FM600A_CORRUPTED_TEMPLATE;
				}
				break;

		}


		t_ptr = tprev;
		r_ptr -= CMbytecnt(t_ptr);

		/* if chkchar in effect, skip over other occurrences of it */

		if (chkchar != '\0')
		{
			while (t_ptr >= fmt->fmt_var.fmt_template &&
			       (*t_ptr == chkchar || *t_ptr == FC_SEPARATOR ||
			        *t_ptr == FC_DECIMAL))
			{
				CMprev(t_ptr, fmt->fmt_var.fmt_template);
				if (fillchar != '\0')
				{
					*r_ptr = fillchar;
					r_ptr -= CMbytecnt(t_ptr);
				}
			}
		}

	}


	return(status);
}


/*{
** Name:	fmt_ntposition - check validity of target cursor position
**
** Description:
**		This function is called by the window manager library when
**		attempts are made to change the cursor position or contents
**		of the text widget associated with an entry field.
**
** Inputs:
**	str			- the formatted numeric string
**	len 		- the length of the string
**	target		- the target cursor column
**	dir			- requested direction to move if we can't land at the
**				  requested position (must be 1:=forward, or -1:= backward).
**
** Outputs:
**
**	Returns:
**		i4		- the column to position to.
**
** History:
**	12-9-91	(tom) - created
**	12-oct-1993 (rdrane)
**		Make decimal_sym a local variable which is set each time
**		from the ADF_CB, rather than a static which is fixed on
**		the first call.  Note that it will always be present/specified
**		(part of FEadfcb() processing).  Pass it to GoodPos().
**
**
*/

i4
fmt_ntposition(str, len, target, dir)
	char *str;
	i4 len;
	i4 target;
	i4 dir;
{
	i4 dcpt;
	char *tp;
	i4 trips;
	char decimal_sym;
	char save;
	i4 i;
	ADF_CB *cb = FEadfcb();

	decimal_sym = cb->adf_decimal.db_decimal;

	if (target > len)
		target = len;

	/* first establish decimal point position */
	for (dcpt = 0; dcpt < len; CMbyteinc(dcpt, &str[dcpt]))
	{
		if (decimal_sym == str[dcpt])
			break;
	}

	/* we move first in the desired direction, and if necessary
	   in the other direction looking for a good position */
	for (trips = 0; GoodPos(str, len, dcpt, target, decimal_sym) == FALSE;)
	{
		target += dir;			/* CM no no .. */

		if (target > len || target < 0)
		{
			dir = -dir;
			target += dir;			/* CM no no .. */
			if (trips++ > 2)
				break;
		}
	}

	return target;
}

/*{
** Name:	GoodPos - check validity of target cursor position
**
** Description:
**		This function is called as a subroutine of the fmt_ntposition
**		routine to check on the validity of any potential position.
**
** Inputs:
**	str			- the formatted numeric string, null terminated
**  len			- length of the string
**	dcpt		- the index of the decimal point (or one beyond the
**					end of the string if there is no dec pt)
**	target		- the target cursor column
**	decimal_sym - decimal point symbol in use.
**
** Outputs:
**
**	Returns:
**		bool	- TRUE or FALSE with the obvious meanings (whew...)
**
** History:
**	12-09-91	(tom) - created
**	01-20-92	(tom) - allow the cursor to be positioned immediately to
**						the left of the decimal point (bug reported by royt)
**	12-oct-1993 (rdrane)
**		Pass in decimal_sym, since it's no longer a global static.
**  06-Aug-1997 (rodjo04) bug 82140.
**      Fixed posistioning so that left most digit of a numeric 
**      field can be edited according to the input mask.
*/

static bool
GoodPos(str, len, dcpt, target, decimal_sym)
	char *str;
	i4 len;
	i4 dcpt;
	i4 target;
	char decimal_sym;
{
	i4 i;

	/* if we are to the left of the decimal point */
	if (target <= dcpt)
	{
		/* see if we are immediately to the right of a digit
		   or if it is to the left of the decimal point */
		if (target >= 0)		/* test first if we are greater than or equal to  0 */
		{
			if (  CMdigit(&str[target - 1])	/* CM no no... */
                                                   || CMdigit(&str[target])
			   || str[target] == decimal_sym
			   )
				return TRUE;
		}
		else
		{
			return FALSE; 	
		}

		/* if we are currently immediately to the right of a digit,
		   it must be the first digit in the string */
		if (target < len && CMdigit(&str[target]))
		{
			for (i = 0; i < len; CMbyteinc(i, &str[i]))
			{
				if (CMdigit(&str[i]) && i == target)
					return TRUE;
			}
		}
	}
	else	/* we are to the right of the decimal point */
	{
		/* we restrict the input position so that it is less
		   than the length of the field, and it must have a
		   digit on one side or the other. Note: we now allow
		   it to be beyond the end of the field. */
		if (  target <= len
		   && (  CMdigit(&str[target])
			  || CMdigit(&str[target - 1])
			  )
		   )
		{
			return TRUE;
		}
	}

	return FALSE;
}


/*
** Name: fmt_ntmodify	- Process input masking modification request
**
** Description:
**  Window manager specific code calls this function when input masking
**  is turned on and the modification callback is activated for a numeric
**  template field.
**
** Inputs:
**	fmt		- format structure
**	ch_input 	- character input (EOS if it's a delete, or paste)
**	old_s		- the old display string
**  	*pos		- ptr to cursor position in the old string (-1 for
**			  no position in which case we set to dec. pt. position)
**
** Outputs:
**	*pos		- ptr to new cursor position
**	new_s		- callers buffer to place the new string
**	*waring		- post to caller if there was a bad character in the string
**
** Returns:
**	TRUE if all is well else FALSE
**
**
** Exceptions:
**
** Side Effects:
**
**
** History:
**	27-aug-91 (tom)	Written.
**  04-apr-92 (tom) Fixed bug 42750, concerning marking the field
**				and then loosing the first character typed.
**	04-dec-92 (tom) When we transition from a null string (nothing
**				displayed) to a digit being entered we need to
**				position at the dec. pt.
**	12-oct-1993 (rdrane)
**		Make decimal_sym a local variable which is set each time
**		from the ADF_CB, rather than a static which is fixed on
**		the first call.  Note that it will always be present/specified
**		(part of FEadfcb() processing).
**	7-dec-1993 (rdrane)
**		Modify interface to f_nt().  Rework usage of ts and buffer such that 
**		we no longer need to cast to (DB_TEXT *), and so won't core dump from
**		time to time due to misalignment.  We were able to eliminate ts, and
**		use buf as strictly a char buffer with no casts.  We also lost the
**		stuph regarding preserving "-0".
**	1-apr-1994 (rdrane)
**		Add is_fract to indicate a value which has no integer portion
**		(b61977).  Restore the stuph regarding preserving "-0" and propagate
**		any previous minus indication (aspect of b56659).
**	02-feb/1995 (carly01)
**		66950 - integer (numeric format) simple table editing
**		produces incorrect results: e.g. 123 shows as 231.  The
**		fix determines the rightmost character from the input
**		data length and not the fixed rightmost position of the 
**		input field or the byte immediately to the left of the
**		decimal.
**	18-jun-1997 (rodjo04)
**	    bug 81287. Fixed input masking for decimal fields that 66950     
**	    fix broke. Now both integers and decimals work.
**	24-jun-1997 (bobmart)
**	    Adjust rodjo04's fix; STindex() returns a pointer to char.
**	    The problem was pointed out by the ris_us5 compiler.
**  06-Aug-1997 (rodjo04) 
**      bug 82140. Fixed right justification of input masked numeric fields. 
**  08-sep-1997 (rodjo04)
**      Corrected problem with cursor position being calculated
**      incorrectly because of non-digits and white spaces being counted 
**      as digits. Also fixed problem with negitive sign '-' not being able 
**		to be deleted. 
**  24-Dec-1997 (rodjo04)
**      bug 87872  Fixed positioning problems with input masking  +'---zzz'.
**  08-July-1999 (islsa01)
**      bug 97274  Fixed zero default problem with input masking.
**  27-Jan-2005 (chopr01)
**      bug 113166 fixed insert problem when using template like +'nnn\-nn\-nnnn'
**      and input masking on, it required rolling back the changes done for 
**      bug 108472. For the money field with template like +'$**,***,***.nn'
**      changed the cursor calculation logic, so that '*' is not treated as
**      digit during the calculation 
**  28-Sep-2006 (ianma02)
**	Adding a part of the fix for bug 108472 (from change 462151) that is
**	still needed.
**  06-Nov-2006 (kiria01) b117042
**	Conform CMxxxxx macro calls to relaxed bool form
*/
bool
fmt_ntmodify(fmt, is_nullable, ch_input, old_s, pos, new_s, warning)
	FMT*	fmt;
	bool	is_nullable;
	char	ch_input;
	char*	old_s;
	i4*	pos;
	char*	new_s;
	bool*	warning;
{
	register char*			s;
	register char*			out;
	AFE_DCL_TXT_MACRO(100)	result;
	ADF_CB*					cb = FEadfcb();
	char*					ins_ptr = NULL;
	char*					is_decm = NULL;
	char					decimal_sym;
	char					thous_sym;
	char					buf[100];
	char					tmp[2];
	bool					was_overstrike = FALSE;
	bool					overstrike = FALSE;
	bool					decpt_found = FALSE;
	bool					first_digit = TRUE;
	bool					is_neg = FALSE;
	bool					is_fract = FALSE;
	bool					is_zero = FALSE;
	bool					found_zero = FALSE;
	i4					curdig;
	i4					digpos = 0;
	f8					num;
	STATUS					ret;
	i4					s_len=0;

	decimal_sym = cb->adf_decimal.db_decimal;
	thous_sym = FC_SEPARATOR;
	if  (decimal_sym != FC_DECIMAL)
	{
		thous_sym = FC_DECIMAL;
	}

	*warning = FALSE;
	is_decm = STindex(old_s, &decimal_sym, STlength(old_s));

	/* some initial tests */

	if (CMdigit(&ch_input))
	{
        if (is_decm == NULL)
            ++(*pos); 
 	    ins_ptr = old_s + *pos;
	}
	else if ( 	ch_input == FC_PLUS 	/* force positive */
			|| 	ch_input == FC_MINUS 	/* force negative */
	        ||  ch_input == EOS			/* move to left of decptr */
			||	ch_input == decimal_sym	/* move to right of decpt */
			||	ch_input == '@'			/* just reformat */
			)
	{
		/* do nothing, handled below */
	}
	else
	{
		return FALSE;		/* unrecognized input char */
	}

	/* if the length of the input string is 0, and the user entered
		a digit.. then it is the case where we are transitioning
		a null (no string) to a value.. we need to position the cursor
		at the decimal point so we will say the old string is just
		the digit that was entered, and the input character is our special
		trigger to put the cursor on the decimal point */
	if (STlength(old_s) == 0 && CMdigit(&ch_input))
	{
		tmp[0] = ch_input;
		tmp[1] = EOS;
		old_s = tmp;
		ch_input = EOS;
	}

	/* trim whitespace from beginning and end of string */	 /* CM no no */
	for (s = old_s + STlength(old_s); s > old_s && CMwhite(s-1); s--);
	*s = EOS;

	/* since we maybe eat some of the end of the string, back up where
		we thought the insertion pointer was */
	if (ins_ptr && ins_ptr > s) 
	    ins_ptr = s ;

	for (s = old_s; CMwhite(s); CMnext(s));

	s_len = STlength(s);	/* (bug 110107) */

	for (out = buf; ; CMnext(s)) {
		switch (*s) {
			case ' ':		/* blanks and signs are not digits so don't */
			case '+':		/* count them */
			case '-':
			case '$':
                        case '*':
			case FC_OPAREN:
			case FC_OANGLE:
			case FC_OCURLY:
			case FC_OSQUARE:
				break;
			default:
                               if (*s != thous_sym)
					digpos++;  
				break;
		}
		if (s == ins_ptr)
		{
			*out++ = ch_input;
            curdig = digpos;  
		
			/* if we have seen a decimal point, then we go into overstrike */
			if (decpt_found == TRUE)
			{
				was_overstrike = overstrike = TRUE;

				/* if we are after the decimal point then we must be
				   in front of a digit, else we assume that we are
				   at the end of the number in which case we just beep */
				if (!CMdigit(s))
				{
					*warning = TRUE;
					return FALSE;
				}
			}
		}

		switch (*s)
		{
		case '!':		/* some characters that should never appear */
		case '\"':
		case '#':
		case '%':
		case '&':
		case '\'':
		case '/':
		case ':':
		case ';':
		case '=':
		case '?':
		case '@':
		case '\\':
		case '^':
		case '_':
		case '`':
		case '~':
			*warning = TRUE;
			break;

		case FC_MINUS:		/* these are the negative indicators */
		    if(first_digit == TRUE)
				is_neg = TRUE;
		    break;

		case FC_OPAREN:
		case FC_CPAREN:
		case FC_OANGLE:
		case FC_CANGLE:
		case FC_OCURLY:
		case FC_CCURLY:
		case FC_OSQUARE:
		case FC_CSQUARE:
			is_neg = TRUE;
			break;

		case 'C':			/* for CR */
		case 'c':
			if (s[1] == 'r' || s[1] == 'R')
			{
				is_neg = TRUE;
				s++;
			}
			else
			{
				*warning = TRUE;
			}
			break;

		case 'D':			/* for DB */
		case 'd':
			if (s[1] == 'b' || s[1] == 'B')
			{
				is_neg = TRUE;
				s++;
			}
			else
			{
				*warning = TRUE;
			}
			break;

		default:
			if (CMdigit(s))
			{
				if (overstrike == TRUE)
				{
					overstrike = FALSE;
				}
				else if (first_digit == TRUE)
				{
					/* we need to ignore the first digit if it is
					   a '0' so our calculation of curdig is correct
					   when we attempt to position the cursor below */
					first_digit = FALSE;

                                        /*
                                        ** (Bug#: 113166)
                                        ** Rolled back the following changes made for Bug#: 108472,
                                        ** This was causing regression Bug#: 113166 
                                        */ 
                                        if ((*s == '0'))  
					{  
						found_zero = TRUE;

						/*
						**	(bug 110107)
						**	The above bug is a side effect of fix for bug 97274.
						**	Reworked 97274 fix below.
						**	We have found a single '0' in our input string.
						**	This is a special case. Here we will have to copy it
						**	out to the out buffer so that it is processed further.
						**	If not we assume its an empty buffer and return.
						**	(ref: see check below where we check if buffer is
						**	empty and is nullable...in that case we just return
						**	without further processing.
						*/

						if(s_len == 1)
							*out++ = *s;
					}
					else
        				*out++ = *s;  
				}
				else
				{
					*out++ = *s;
				}
			}
			else if (*s == decimal_sym)
			{
				*out++ = *s;
				first_digit = FALSE;
				decpt_found = TRUE;
			}
			else if (  CMalpha(s))
			{
				*warning = TRUE;
			}
		}
		if (*s == EOS)
			break;
	}
	*out = EOS;

	if (  out == buf				/* if there's nothing in the buffer */
	   && is_nullable				/* and the field is nullable */
	   && ch_input != FC_MINUS		/* and we're not trying to change sign */
	   && ch_input != FC_PLUS
	   && ch_input != decimal_sym
	   )
	{
		*new_s = EOS;				/* then we allow a null buffer */
		*pos = 0;
        /*
        ** right justify if numeric field and 
        ** there is nothing in the buffer
        */ 
        if (fmt->fmt_type == F_NT)
            *pos = fmt->fmt_width - 1;

		return TRUE;
	}

	/* don't let them attempt to jam digits into positions beyond
		the end of the mask if there is a decimal point */
	if (overstrike == TRUE)
	{
		return FALSE;
	}
     
	/*
	** Convert the number, first propagating
	** any negative indicators.
	*/
	
	if ((ret = CVaf(buf, decimal_sym, &num)) != OK)
	{
		num = 0.0;
	}
	
	/* 
	** (bug 109794)
	** Reset flags below only if Cvaf() reported a conversion
	** error. If not we will be incorrectly dropping the sign 
	** indicator. 
	*/

	if (num == 0.0)
	{
		if( ret != OK)
		{
			is_zero = TRUE;
			is_neg = FALSE;
		}
	}
	else if ((num < 1.0) && (num > -1.0))
	{
		is_fract = TRUE;
	}

	if  ((is_neg) && (!is_zero))
	{
		STcopy(&buf[0],(char *)&result.text[0]);
		STpolycat(2,ERx("-"), (char *)&result.text[0],&buf[0]);
	}

	/*
	** Are we explicitly setting a sign on an initial or zero value?
	** This primarily handles an initial minus sign input.
	*/
	if  ((is_zero) &&
		 ((ch_input == FC_MINUS) || ((is_neg) && (ch_input != FC_PLUS))))
	{
	    is_neg = TRUE;
		ch_input = EOS;	/* force position to dec. pt. */
	}

	if (f_nt(cb, &buf[0], fmt, &result, is_zero, is_neg, is_fract) != OK)
	{
		return FALSE;
	}
	result.text[result.count] = EOS;
	STcopy((char *)result.text, new_s);

	/* see if we are requesting a particular position ('@' fixes Bug# 85259) */
	if (ch_input == EOS || ch_input == decimal_sym || ch_input == '@')
	{
		/* find the digits and decimal point */
		s = STindex(new_s, &decimal_sym, 0);
		if (s == NULL)
		{
			s = new_s + STlength(new_s);

			/* must back over possible trailing negative indicators,
			   or spaces reserved for them */
			while (s > new_s && !CMdigit(s - 1))
			{
				s--;
			    if (*s == FC_MINUS) 
				{
					s++;
					break;
				}
			}
			if (s== new_s && ch_input == decimal_sym)
			{
				char *t = fmt->fmt_var.fmt_template;
				char *tp, *sp;
				tp = STindex(t, &decimal_sym, 0);
				if (tp != NULL)
				{
				    s = new_s + (tp - t);
				    *s = decimal_sym;
				    sp = s;
				    while(*++tp)
					if (*tp == FC_ZERO || *tp == FC_DIGIT)
					    *(++sp) = '0';
				}
			}
		}
		*pos = s - new_s;
		if (ch_input == decimal_sym)
			(*pos)++;
	}
	else if (CMdigit(&ch_input))	/* set cursor position */
	{
		/* if we inserted a digit before the decimal point then we want
		   to ignore the first 0 if we found it because */
		if (was_overstrike == FALSE)
			curdig -= found_zero;

		for (s = new_s; *s != EOS; CMnext(s))
		{
			if (CMdigit(s) || *s == decimal_sym)
			{
				if (--curdig <= 0)
					break;
			}
		}
		*pos = s - new_s + 1;
	}
	return TRUE;
}
