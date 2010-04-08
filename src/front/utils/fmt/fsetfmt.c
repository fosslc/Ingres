/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<bt.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<cm.h>
# include	"format.h"
# include	<st.h>

/*
**   F_SETFMT - set up an FMT structure from a specification of a
**	format, as a string.  This figures out the type of the format
**	and fills in the fields of the FMT structure.
**
**	Valid formats are:
**		"template" - a numeric template surrounded by quotes
**			to specify a numeric field.
**		d[i]"template" or D[I]"template" - a date template surrounded
**			by quotes to specify a date field.
**			'i' denotes date interval for input.
**		fw.d or Fw.d - a numeric specification where "w" is
**			the maximum width and "d" is the number of numbers
**			past the decimal.
**		ew.d or Ew.d - scientific notation of the form
**			n.mmmmE+xx or n.mmmmme-xx where "w" is the
**			field width and "d" is the number of spaces
**			after the decimal.
**		gw.d or Gw.d - use F format if it will fit, otherwise
**			use E format.  Also make sure that the decimal
**			points align (i.e. right justify E format
**			numbers and right justify F format to
**			4 spaces from right edge of field).
**		nw.d or Nw.d - same as G, except decimal points
**			may not align (i.e. right justify all).
**		cn or Cn - character output, where "n" is the maximum
**			field width. If "n" is 0 or unspecified, the
**			actual string length is used.  If followed by an
**			"r" (as in cr30), the field will be handled as
**			a right-to-left field (used for languages such
**			as Hebrew.)
**		cw.n or Cw.n - character column output.	 This will wrap
**			around a character string, which is "w" total
**			characters into columns of "n" characters each.
**			If the "C" is followed by a "f" (as in Cf100.10),
**			the lines will be folded, or broken only at word
**			boundaries.  If followed by a "j" (as in CJ100.10),
**			it will be folded and right justified in the column.
**			If followed by an "r" (as in cFr30.15 or crj100.10),
**			the field will be handled as a right-to-left field
**			(used for languages such as Hebrew.)
**		bn or Bn - blank out the field.	 Used with the .TFORMAT
**			command in the report formatter.
**		in or In - same as Fn.
**		tw[.n] or Tw[.n] - output character string like the terminal
**			monitor does where non-printable characters are
**			displayed in octal (i.e. '\ddd'). Optionally it may
**			wrap like cn.w.
**		qn or Qn - outputs a string characters, but returns a length of
**			zero.  Used in Report Writer ONLY.  Used to send escaped
**			sequences to teh report destination device.
**		s"template" - string datatype template which supports
**			input masking constructs (for Windows4GL)
**
**		If a format specification is preceded by a sign ('-' or '+')
**		then the output is to be left or right justified respectively.
**		If no sign is given, default justification of left for char
**		and right for numeric is used.
**
**	Inputs:
**		cb	pointer to control block.
**		fmt	address of FMT structure to fill in.
**			Make sure that you give a ptr in the
**			fmt->fmt_var.fmt_template location in the
**			structure if you anticipate any
**			template formats.
**			If you don't this routine will return.
**		string	string containing format specification.
**
**		fmtlen	points to a i4  for placing the length of the
**			scanned format string.
**	Outputs:
**		This routine also fills in the other fields in
**		the fmt structure sent.
**
**		If fmtlen is not NULL, the i4  it points to will contain
**		the length of the format string actually scanned.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_ERROR	If some error occurred.
**
**		If an error occurred, the caller can look in the field
**		cb->adf_errcb.ad_errcode to determine the specific error.
**		The following is a list of possible error codes that can be
**		returned by this routine:
**
**		E_FM2000_EXPECTED_CHAR_INSTEAD	Different character was expected
**						than that given.
**		E_FM2001_EXPECTED_NUMBER	Number was expected next.
**		E_FM2006_BAD_CHAR		Illegal character was found.
**		E_FM2007_BAD_FORMAT_CHAR	Illegal character was specified
**						for the format type.
**		E_FM2008_NO_ESCAPED_CHAR	No character follows the escape
**						(i.e. backslash) character.
**		E_FM2009_CHAR_SHOULD_FOLLOW	Specific character must follow
**						character.
**		E_FM200A_FORMAT_TOO_LONG	Width of template format exceeds
**						maximum allowed.
**		E_FM200B_COMP_ALREADY_SPEC	Date component has been
**						specified more than once.
**		E_FM200C_ABS_COMP_SPEC_FOR_INT	Absolute date component cannot
**						be specified in date interval
**						format.
**		E_FM200D_WRONG_COMP_VALUE_SPEC	Wrong value was specified for
**						date component.
**		E_FM200E_WRONG_ORD_VALUE	Wrong ordinal suffix follows a
**						given digit.
**		E_FM200F_ORD_MUST_FOLLOW_NUM	Ordinal suffix doesn't follow
**						a number.
**		E_FM2010_PM_AND_24_HR_SPEC	Before/after noon designation
**						(using 'p') and 24-hour time
**						(using '16' for the hour instead
**						of '4') cannot be specified
**						together.
**		E_FM2011_I_FMT_CANT_HAVE_PREC	Integer format (I) cannot have
**						numeric precision.
**		E_FM2012_PREC_GTR_THAN_WIDTH	For numeric format, the
**						precision cannot be greater than
**						the width.
**		E_FM2014_JUSTIF_ALREADY_SPEC	The justification character ('-'
**						or '+') has been specified more
**						than once.
**		E_FM2015_NO_DATE_COMP_SPEC	No date component was specified.
**		E_FM2016_INVALID_TEMPLATE	An invalid template was entered.
**
**	History:
**		6/20/81 (ps)	written.
**		4/4/82 (ps)	allow "i" as surrogate for "f".
**		11/5/82 (ps)	add reference to sign before format.
**		2/9/83 (gac)	add date templates.
**		3/1/83 (gac)	allow \\ in numeric template.
**		1/20/85 (rlm)	ifdef out date format code for CS
**				environment
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**		12/19/86 (gac)	added F_I, got rid of fmt_template for F_...
**		1/26/87 (gac)	added DI.
**		1/27/87 (gac)	added T format.
**		3/27/87 (bab)	added acceptance of reversed char formats.
**		6/24/87 (danielt) changed \0 to EOS
**		12-may-88 (sylviap)
**			Returns an error for format f8.8 and f8.7.
**			Flags f5.1.1 as an invalid format.
**			Returns as error if DG and date format is passed.
**			  For DG, the date format is not implemented.
**		18-jul-88 (marian)
**			Took out the error check for invalid formats like
**			f5.1.1.  This check was breaking reports.  A fix
**			was made to vifred (tfcreate.qsc, fmt.c) to check
**			the return from fmt_setfmt() to determine if an invalid
**			format was specified.
**		29-sep-88 (sylviap)
**			Took out if DG and date format.  DG will now support 
**			  date datatype.
**	10/14/88 (dkh) - Fixed venus bug 3636.
**		20-apr-89 (cmr)
**			Added support for new format (cu) which is
**			used internally by unloaddb().  It is similar to
**			cf (break at words when printing strings that span 
**			lines) but it breaks on quoted strings as well as
**			words.
**		27-jul-89 (cmr)
**			Support for new formats cfe and cje.
**		9/7/89 (elein) garnet
**			Add single quote recognition
**		20-sep-89 (sylviap)
**			Support for new Q format.
**		1/14/90 (elein) 35306
**			Display widths of zero are be illegal for numeric
**			and date templates. Return error.
**		1/9/91 (elein) 35289
**			Add parens around (string+1) in CMcmpnocase calls.
**			This is a macro and substitution is done incorrectly
**			when expanded down to cmtolower.  This was causing
**			CR and DB to fail as elements of numeric templates.
**	 	7-23-91 (tom) added string template support
**		3-24-93 (johnr)
**			Set the default date format to left justified instead
**		 	of none so that blank padding will happen later.  This
**			fixes sep test vma16 hanging and diffs.
**	04/22/93 (dkh) - Rolled back above change since it applied to all
**			 formats and not just date formats.  This caused
**			 leading blanks in character fields to be removed
**			 and fields to be non-scrollable.
**	06/22/93 (dkh) - Fixed bugs 51530 and 51533.  Vifred now properly
**			 supports a field with decimal datatype and
**			 numeric templates.
**	15-jul-1993 (rdrane)
**			Ensure that numeric templates end with the same
**			delimiter with which they began (b44053).  Extended this
**			to date and string templates, since they had the same
**			deficiency.
**	24-feb-1997 (i4jo01)
**			Floating Point: If precision is 0 then there is no 
**			need to check for space for decimal point and
**			leading 0. (b80413) 
**	13-aug-1998 (kitch01)
**		Bug 91491. When using numeric templates for decimal fields
**		ensure the precision is the total number of places defined.
**		e.g. zzz.nn the precision is 5.
**	22-dec-1998 (crido01) SIR 94437
**		Add the '^' (FC_ROUNDUP) character to numeric templates in 
**		order to indicate theat a decimal values remaing digits are
**		to be rounded-up as opposed to being truncated.
**	 7-jul-2000 (hayke02)
**		Initialize fmt_ccsize to 0 - this prevents a SEGV in
**		f_cvta() when fmt_template is non-NULL (because either
**		fmt_string or fmt_reverse has been set) and fmt_ccsize
**		is non-zero. This change fixes bug 102055.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


STATUS
f_setfmt(cb, fmt, string, fmtlen)
ADF_CB		*cb;
register FMT	*fmt;
char		*string;
i4		*fmtlen;
{
	/* internal declarations */

	i4		type = F_ERROR; /* type of format found */
	i4		number;		/* used in finding field width */
	char		*c;		/* temp ptr to char strings */
	bool		dec_found = FALSE;	/* true if decimal found */
	i4		length = 0;	/* length of template */
				/* for date template */
	bool		hr_24 = FALSE;
	bool		p_found = FALSE;
	bool		dow_found = FALSE;
	char		token[100];
	char		*t;
	char		class;
	i4		value;
	i4		fwidth = 0;
	i4		digit;
	i4		compwidth[6];
	char		*f_gnum();
	char		*strstart;
	bool		reverse = FALSE;	/* specify right-to-left fmt */
	char		*comp;
	STATUS		err;
	i4		prec_digits = 0;
	i4		scale_digits = 0;
	i4		currencysym = FALSE;
	i4		plussym = FALSE;
	i4		minussym = FALSE;
	char	t_delim;	/*
						** Save start delimiter of template
						** strings - date, numeric, and string.
						*/

	/* start of routine */

	fmt->fmt_type = F_ERROR;
	fmt->fmt_sign = FS_NONE;
	fmt->fmt_emask = NULL;

	/*
	**  Get rid of any trailing blanks.
	*/
	STtrmwhite(string);

	strstart = string;

	/* first skip over any leading blanks and check for sign */

	for(;; CMnext(string))
	{
		if (CMspace(string) || *string == '\t')
			continue;
		if (*string == '+' || *string == '-' || *string == '*')
		{	/* justification specified */
			if (fmt->fmt_sign != FS_NONE)
			{	/* already specified.  ERROR */
			    if (fmtlen != NULL)
			    {
				*fmtlen = string - strstart;
			    }
			    return afe_error(cb, E_FM2014_JUSTIF_ALREADY_SPEC, 0);
			}

			switch (*string)
			{
			case '+':
			    fmt->fmt_sign = FS_PLUS;
			    break;

			case '-':
			    fmt->fmt_sign = FS_MINUS;
			    break;

			case '*':
			    fmt->fmt_sign = FS_STAR;
			    break;
			}

			continue;	/* skip other leading blanks */
		}
		break;
	}

	switch(*string)
	{
		case('\"'):	/* numeric template */
		case('\''):	/* numeric template */
			type = F_NT;
			break;

#ifdef DATEFMT
		case('d'):
		case('D'):
			type = F_DT;
			break;
#endif  /* DATEFMT */

		case('s'):
		case('S'):
			type = F_ST;
			break;

		case('i'):
		case('I'):
			type = F_I;
			break;

		case('f'):
		case('F'):
			type = F_F;
			break;

		case('e'):
		case('E'):
			type = F_E;
			break;

		case('g'):
		case('G'):
			type = F_G;
			break;

		case(FC_ROUNDUP):
		case('n'):
		case('N'):
			type = F_N;
			break;

		case('c'):
		case('C'):
			type = F_C;
			/* see if "f" or "j" or "r"  or "u" follows */
			switch(*(string+1))
			{
				case('r'):
				case('R'):
					string++;
					reverse = TRUE;
					if (*(string+1) == 'f'
						|| *(string+1) == 'F')
					{
						string++;
						type = F_CF;
					}
					else if (*(string+1) == 'j'
						|| *(string+1) == 'J')
					{
						string++;
						type = F_CJ;
					}
					break;

				case('f'):
				case('F'):
					string++;
					type = F_CF;
					/* r and e are mutually exclusive */
					switch (*(string+1))
					{
						case 'r':
						case 'R':
							reverse = TRUE;
							string++;
							break;
						case 'e':
						case 'E':
							type = F_CFE;
							string++;
							break;
					}
					break;

				case('j'):
				case('J'):
					string++;
					type = F_CJ;
					/* r and e are mutually exclusive */
					switch (*(string+1))
					{
						case 'r':
						case 'R':
							reverse = TRUE;
							string++;
							break;
						case 'e':
						case 'E':
							type = F_CJE;
							string++;
							break;
					}
					break;

				/* internal format used by unloaddb() */
				case('u'):
				case('U'):
					string++;
					type = F_CU;
					if (*(string+1) == 'r'
						|| *(string+1) == 'R')
					{
						string++;
						reverse = TRUE;
					}
					break;
			}
			break;

		case('b'):
		case('B'):
			type = F_B;
			break;

		case('q'):
		case('Q'):
			if (fmt->fmt_sign != FS_NONE)
			{	/* Do not specify justification with Q format.  
				**		ERROR */
			    if (fmtlen != NULL)
			    {
				*fmtlen = string - strstart;
			    }
			    return afe_error(cb, E_FM2017_NO_JUSTIF, 0);
			}
			type = F_Q;
			break;
		case('t'):
		case('T'):
			type = F_T;
			break;

		default:
			if (fmtlen != NULL)
			{
				*fmtlen = string - strstart;
			}
			return afe_error(cb, E_FM2007_BAD_FORMAT_CHAR, 2,
					 CMbytecnt(string), (PTR)string);
	}

	fmt->fmt_width = 0;
	fmt->fmt_prec = 0;
	fmt->fmt_ccsize = 0;

	switch (type)
	{
		case(F_NT):
			c = fmt->fmt_var.fmt_template;	/* ptr to template */

			/* change upper case to lower for special chars
			** check for legal template characters, and
			** and check for decimal point location */

			t_delim = *string;
			for (CMnext(string);
			     (*string!='\"') && (*string!='\'') && (*string!= EOS);
			     CMnext(string))
			{
				if (length++ >= MAXFORM)
				{	/* too big, skip it */
					break;
				}

				switch(*string)
				{
					case('\\'):
						/* keep next character directly */
						if (*(++string) == EOS)
						{	/* end of string */
							if (fmtlen != NULL)
							{
								*fmtlen = string - strstart;
							}
							return afe_error(cb, E_FM2008_NO_ESCAPED_CHAR, 0);
						}

						*c++ = FC_ESCAPE;
						CMcpychar(string, c);
						CMnext(c);
						break;

					case('Z'):
					case('N'):
						*c++ = *string + ('a'-'A');
						if (dec_found == TRUE)
						{
							fmt->fmt_prec++;
						}
						prec_digits++;
						break;

					case('c'):
					case('C'):
						if (CMcmpnocase((string+1), ERx("r")) == 0)
						{
							*c++ = *string++;
							*c++ = *string;
							break;
						}
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						return afe_error(cb, E_FM2009_CHAR_SHOULD_FOLLOW, 4,
								 1, (PTR)ERx("r"),
								 1, (PTR)string);

					case('d'):
					case('D'):
						if (CMcmpnocase((string+1), ERx("b"))
						    == 0)
						{
							*c++ = *string++;
							*c++ = *string;
							break;
						}
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						return afe_error(cb, E_FM2009_CHAR_SHOULD_FOLLOW, 4,
								 1, (PTR)ERx("b"),
								 1, (PTR)string);

					case(FC_CURRENCY):
						if (dec_found)
						{
							currencysym = TRUE;
						}
					case(FC_ROUNDUP):
					case(FC_DIGIT):
					case(FC_STAR):
					case(FC_ZERO):
						/* significant digit in fraction */
						if (dec_found == TRUE)
						{
							fmt->fmt_prec++;
						}
						prec_digits++;
						*c++ = *string;
						break;

					case(FC_MINUS):
					case(FC_PLUS):
						if (!dec_found)
						{
							if (*string == FC_PLUS)
							{
								plussym = TRUE;
							}
							else
							{
								minussym = TRUE;
							}
							prec_digits++;
						}
					case(FC_SEPARATOR):
					case(FC_OPAREN):
					case(FC_CPAREN):
					case(FC_OANGLE):
					case(FC_CANGLE):
					case(FC_OCURLY):
					case(FC_CCURLY):
					case(FC_OSQUARE):
					case(FC_CSQUARE):
						*c++ = *string;
						break;

					case(FC_DECIMAL):
						dec_found = TRUE;
						fmt->fmt_prec = 0;
						*c++ = *string;
						break;


					default:
						if (CMspace(string))
						{
							CMcpychar(string, c);
							CMnext(c);
						}
						else
						{
							if (fmtlen != NULL)
							{
								*fmtlen = string - strstart;
							}
							return afe_error(cb, E_FM2006_BAD_CHAR, 2,
									 CMbytecnt(string), (PTR)string);
						}

				}
			}

			/*
			**  Now encode precision/scale info into the
			**  fmt_ccsize member of the FMT struct.
			**  This member is not being used for numeric
			**  templates (F_NT) so we won't run into any
			**  collisions.
			*/
			if (currencysym)
			{
				prec_digits--;
			}
			if (minussym)
			{
				prec_digits--;
			}
			if (plussym)
			{
				prec_digits--;
			}
			if (prec_digits <= 0 || prec_digits < fmt->fmt_prec)
			{
				fmt->fmt_ccsize = 0;
			}
			else
			{
				i2	ps;

				fmt->fmt_ccsize = DB_PS_ENCODE_MACRO(prec_digits, fmt->fmt_prec);
			/*	fmt->fmt_ccsize = ps;	*/
			}

			/*
			** Finished with template.  Make sure it
			** ended with a delimiting quote of the
			** proper stripe.  Skip the ending delimiter,
			** and terminate fmt string. (b44053)
			*/
			if  (t_delim != *string)
			{
				return(afe_error(cb,E_FM2016_INVALID_TEMPLATE,0));
			}
			CMnext(string);

			*c = EOS;		/* end the template */
			for(c=fmt->fmt_var.fmt_template; *c!=EOS; CMnext(c))
			{
				if (*c != FC_ESCAPE)
				{
					CMbyteinc(fmt->fmt_width, c);
				}
			}

			if (fmt->fmt_width > MAXFORM)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM200A_FORMAT_TOO_LONG, 
					2, sizeof(fmt->fmt_width), (PTR)&(fmt->fmt_width));
			}
			else if (fmt->fmt_width <= 0)
			{
	 			return afe_error(cb,E_FM2016_INVALID_TEMPLATE, 2, 
					STlength(fmt->fmt_var.fmt_template),
					fmt->fmt_var.fmt_template);
			}
			break;

		case(F_ST):
			string++;
			while (CMwhite(string))
				CMnext(string);

			if ( (*string != '\"') && (*string != '\'') )
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD, 4,
						 1, (PTR)ERx("\""),
						 CMbytecnt(string), (PTR)string);
			}

			c = fmt->fmt_var.fmt_template;	/* ptr to template */

			/* change upper case to lower for special chars
			** check for legal template characters, and
			** and check for decimal point location */

			t_delim = *string;
			for (CMnext(string);
			     (*string!='\"') && (*string!='\'') && (*string!= EOS);
			     CMnext(string))
			{
				if (length++ >= MAXFORM)
				{	/* too big, skip it */
					break;
				}

				switch(*string)
				{
				case('\\'):
					/* keep next character directly */
					if (*(++string) == EOS)
					{	/* end of string */
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						return afe_error(cb, E_FM2008_NO_ESCAPED_CHAR, 0);
					}

					*c++ = '\\';
					CMcpychar(string, c);
					CMnext(c);
					break;

				default:
					/* for now we will just copy characters, later
					we will validate with f_stsetup */
					CMcpychar(string, c);
					CMnext(c);
				}
			}

			/*
			** Finished with template.  Make sure it
			** ended with a delimiting quote of the
			** proper stripe.  Skip the ending delimiter,
			** and terminate fmt string. (b44053)
			*/
			if  (t_delim != *string)
			{
				return(afe_error(cb,E_FM2016_INVALID_TEMPLATE,0));
			}
			CMnext(string);

			*c = EOS;		/* end the template */

			/* now we setup the entry and content control masks */
			if ((err = f_stsetup(fmt, c+1)) != OK)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return err;
			}

			if (fmt->fmt_width > MAXFORM)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM200A_FORMAT_TOO_LONG, 2, 
					sizeof(fmt->fmt_width), (PTR)&(fmt->fmt_width));
			}
			break;

/* long ifdef knocks out about 230 lines */
#ifdef DATEFMT
		case(F_DT):
			f_initlen();	/* initialize global lengths */

			if (CMcmpnocase((string+1), ERx("i")) == 0)
			{
				string++;
				BTset(FD_INT, (char *)&(fmt->fmt_prec));
			}

			c = fmt->fmt_var.fmt_template;	/* ptr to template */

			/* check for legal date template characters */

			string++;
			while (CMwhite(string))
			{
				CMnext(string);
			}
			if ( (*string != '\"') && (*string != '\'') )
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2000_EXPECTED_CHAR_INSTEAD, 4,
						 1, (PTR)ERx("\""),
						 CMbytecnt(string), (PTR)string);
			}

			t_delim = *string;
			for (string++; (*string!='\"')&&(*string!='\'')
				&& (*string!=EOS);)
			{
				if (fmt->fmt_width >= MAXFORM)
				{	/* too big, skip it */
					break;
				}

				switch(*string)
				{
				case('\\'):
					/* keep next character directly */
					if (*(string+1) == EOS)
					{	/* end of string */
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						return afe_error(cb, E_FM2008_NO_ESCAPED_CHAR, 0);
					}
					*c++ = *string++;
					CMbyteinc(fmt->fmt_width, string);
					CMcpyinc(string, c);
					fwidth = 0;
					break;
				case('|'):
					*c++ = *string++;
					fwidth = 0;
					break;
				case('0'):
					*c++ = *string++;
					fmt->fmt_width++;
					fwidth++;
					break;
				case('1'):
					length = 1;
					if (*(string+1) == '6')
					{
						if (BTtest(FD_HRS, (char *)&(fmt->fmt_prec)))
						{
							if (fmtlen != NULL)
							{
								*fmtlen = string - strstart;
							}
							comp = F_SFullunit[3].k_name;
							return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
									 STlength(comp), (PTR)comp);
						}

						if (BTtest(FD_INT, (char *)&(fmt->fmt_prec)))
						{
							if (fmtlen != NULL)
							{
								*fmtlen = string - strstart;
							}
							return afe_error(cb, 
									E_FM200C_ABS_COMP_SPEC_FOR_INT, 2,
									2, (PTR)string);
						}
						BTset(FD_ABS, (char *)&(fmt->fmt_prec));
						BTset(FD_HRS, (char *)&(fmt->fmt_prec));
						hr_24 = TRUE;
						length = 2;
						compwidth[3] = max(fwidth+length, MAXSETWIDTH);
					}
					else
					{
						if (BTtest(FD_YRS, (char *)&(fmt->fmt_prec)))
						{
							if (fmtlen != NULL)
							{
								*fmtlen = string - strstart;
							}
							comp = F_SFullunit[0].k_name;
							return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
									 STlength(comp), (PTR)comp);
						}
						if (	*(string+1) == '9' 
						   &&	*(string+2) == '0'
						   &&	*(string+3) == '1'
						   )
						{
							if (BTtest(FD_INT, (char *)&(fmt->fmt_prec)))
							{
								if (fmtlen != NULL)
								{
									*fmtlen = string - strstart;
								}
								return afe_error(cb, 
									E_FM200C_ABS_COMP_SPEC_FOR_INT, 2,
									4, (PTR)string);
							}
							BTset(FD_ABS, (char *)&(fmt->fmt_prec));
							length = 4;
						}
						BTset(FD_YRS, (char *)&(fmt->fmt_prec));
						compwidth[0] = max(fwidth+length, MAXSETWIDTH);
					}
					fmt->fmt_width += max(MAXSETWIDTH-fwidth, length);
					for (; length > 0; length--)
						*c++ = *string++;
					fwidth = (CMspace(string)) ? -1 : 0;
					break;
				case('2'):
					if (BTtest(FD_MONS, (char *)&(fmt->fmt_prec)))
					{
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						comp = F_SFullunit[1].k_name;
						return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
								 STlength(comp), (PTR)comp);
					}
					BTset(FD_MONS, (char *)&(fmt->fmt_prec));
					compwidth[1] = max(fwidth+1, MAXSETWIDTH);
					fmt->fmt_width += max(MAXSETWIDTH-fwidth, 1);
					*c++ = *string++;
					fwidth = (CMspace(string)) ? -1 : 0;
					break;
				case('3'):
					if (BTtest(FD_DAYS, (char *)&(fmt->fmt_prec)))
					{
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						comp = F_SFullunit[2].k_name;
						return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
								 STlength(comp), (PTR)comp);
					}
					BTset(FD_DAYS, (char *)&(fmt->fmt_prec));
					compwidth[2] = max(fwidth+1, MAXSETWIDTH);
					fmt->fmt_width += max(MAXSETWIDTH-fwidth, 1);
					*c++ = *string++;
					fwidth = (CMspace(string)) ? -1 : 0;
					break;
				case('4'):
					if (BTtest(FD_HRS, (char *)&(fmt->fmt_prec)))
					{
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						comp = F_SFullunit[3].k_name;
						return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
								 STlength(comp), (PTR)comp);
					}
					BTset(FD_HRS, (char *)&(fmt->fmt_prec));
					compwidth[3] = max(fwidth+1, MAXSETWIDTH);
					fmt->fmt_width += max(MAXSETWIDTH-fwidth, 1);
					*c++ = *string++;
					fwidth = (CMspace(string)) ? -1 : 0;
					break;
				case('5'):
					if (BTtest(FD_MINS, (char *)&(fmt->fmt_prec)))
					{
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						comp = F_SFullunit[4].k_name;
						return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
								 STlength(comp), (PTR)comp);
					}
					BTset(FD_MINS, (char *)&(fmt->fmt_prec));
					compwidth[4] = max(fwidth+1, MAXSETWIDTH);
					fmt->fmt_width += max(MAXSETWIDTH-fwidth, 1);
					*c++ = *string++;
					fwidth = (CMspace(string)) ? -1 : 0;
					break;
				case('6'):
					if (BTtest(FD_SECS, (char *)&(fmt->fmt_prec)))
					{
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						comp = F_SFullunit[5].k_name;
						return afe_error(cb, E_FM200B_COMP_ALREADY_SPEC, 2,
								 STlength(comp), (PTR)comp);
					}
					BTset(FD_SECS, (char *)&(fmt->fmt_prec));
					compwidth[5] = max(fwidth+1, MAXSETWIDTH);
					fmt->fmt_width += max(MAXSETWIDTH-fwidth, 1);
					*c++ = *string++;
					fwidth = (CMspace(string)) ? -1 : 0;
					break;
				case('7'):
				case('8'):
				case('9'):
					if (fmtlen != NULL)
					{
						*fmtlen = string - strstart;
					}
					return afe_error(cb, E_FM2006_BAD_CHAR, 2,
							 1, (PTR)string);
					break;
				default:
					if (CMspace(string))
					{
						CMbyteinc(fmt->fmt_width, string);
						CMbyteinc(fwidth, string);
						CMcpyinc(string, c);
					}
					else if (CMalpha(string))
					{
						length = f_getword(&string, 0, token);
						if (f_dkeyword(token, &class, &value))
						{
							if (	class != F_TIUNIT 
							   &&	BTtest(FD_INT, (char *)&(fmt->fmt_prec))
							   )
							{
								if (fmtlen != NULL)
								{
									*fmtlen = string - strstart;
								}
								return afe_error(cb, 
										E_FM200C_ABS_COMP_SPEC_FOR_INT, 2,
										length, (PTR)token);
							}
							switch(class)
							{
							case(F_MON):
								if (value != F_MONVALUE)
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									comp = F_SFullunit[1].k_name;
									return afe_error(cb, 
											E_FM200D_WRONG_COMP_VALUE_SPEC, 4,
											length, (PTR)token,
											 STlength(comp), (PTR)comp);
								}
								if (BTtest(FD_MONS, (char *)&(fmt->fmt_prec)))
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									comp = F_SFullunit[1].k_name;
									return afe_error(cb,
											E_FM200B_COMP_ALREADY_SPEC, 2,
											STlength(comp), (PTR)comp);
								}
								BTset(FD_ABS, (char *)&(fmt->fmt_prec));
								BTset(FD_MONS, (char *)&(fmt->fmt_prec));
								if (length == F_monlen)
								{
									fmt->fmt_width += F_monmax - F_monlen;
								}
								break;
							case(F_DOW):
								if (value != F_DOWVALUE)
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									comp = ERget(S_FM0001_day_of_week);
									return afe_error(cb, 
											E_FM200D_WRONG_COMP_VALUE_SPEC, 4,
											length, (PTR)token,
											STlength(comp), (PTR)comp);
								}
								if (dow_found)
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									comp = ERget(S_FM0001_day_of_week);
									return afe_error(cb,
											E_FM200B_COMP_ALREADY_SPEC, 2,
											STlength(comp), (PTR)comp);
								}
								BTset(FD_ABS, (char *)&(fmt->fmt_prec));
								dow_found = TRUE;
								if (length == F_dowlen)
								{
									fmt->fmt_width += F_dowmax - F_dowlen;
								}
								break;
							case(F_PM):
								if (value != F_PMVALUE)
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									comp = F_Fullpm[1].k_name;
									return afe_error(cb,
											E_FM200D_WRONG_COMP_VALUE_SPEC, 4,
											length, (PTR)token,
											STlength(comp), (PTR)comp);
								}
								if (p_found)
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									comp = F_Fullpm[1].k_name;
									return afe_error(cb,
											E_FM200B_COMP_ALREADY_SPEC, 2,
											STlength(comp), (PTR)comp);
								}
								BTset(FD_ABS, (char *)&(fmt->fmt_prec));
								p_found = TRUE;
								break;
							case(F_ORD):
								t = string - length;
								for (CMprev(t, strstart); 
										CMwhite(t); 
											CMprev(t, strstart))
									;
								if (*t >= '1' && *t <= '6')
								{
									digit = *t - '0';
									if (STcompare(F_Ordinal[digit].k_name,
										token) != 0)
									{
										if (fmtlen != NULL)
										{
											*fmtlen = string - strstart;
										}
										return afe_error(cb, 
												E_FM200E_WRONG_ORD_VALUE, 4,
												length, (PTR)token,
												1, (PTR)t);
									}
								}
								else
								{
									if (fmtlen != NULL)
									{
										*fmtlen = string - strstart;
									}
									return afe_error(cb,
											E_FM200F_ORD_MUST_FOLLOW_NUM, 2,
											length, (PTR)token);
								}
								BTset(FD_ABS, (char *)&(fmt->fmt_prec));
								break;

							case(F_TIUNIT):
								if (BTtest(FD_INT, (char *)&(fmt->fmt_prec)))
								{
									if (value < 6)
									{
										value += 6;
									}
									else
									{
										value -= 6;
									}

									fmt->fmt_width += 
										max(length, STlength(	
											F_Fullunit[value].k_name)) 
												- length;

								}
								break;
							}
						}
						fmt->fmt_width += length;
						for (t = token; length > 0; length--)
							*c++ = *t++;
						fwidth = (CMspace(string)) ? -1 : 0;
					}
					else
					{
						CMbyteinc(fmt->fmt_width, string);
						CMcpyinc(string, c);
						fwidth = 0;
					}
					break;
				}
			}

			/*
			** Finished with template.  Make sure it
			** ended with a delimiting quote of the
			** proper stripe.  Skip the ending delimiter,
			** and terminate fmt string. (b44053)
			*/
			if  (t_delim != *string)
			{
				return(afe_error(cb,E_FM2016_INVALID_TEMPLATE,0));
			}
			CMnext(string);

			*c = EOS;		/* end the template */

			if (p_found && hr_24)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2010_PM_AND_24_HR_SPEC, 0);
			}

			if (BThigh((char *)&(fmt->fmt_prec), FD_LEN) < FD_YRS &&
			    !dow_found && !p_found)
			{
				return afe_error(cb, E_FM2015_NO_DATE_COMP_SPEC, 0);
			}

			if (BTtest(FD_DAYS, (char *)&(fmt->fmt_prec)) &&
			    !BTtest(FD_MONS, (char *)&(fmt->fmt_prec)))
			{
				/* for day of year */
				if (BTtest(FD_YRS, (char *)&(fmt->fmt_prec)))
					fmt->fmt_width += max(3 - compwidth[2], 0);
				else				/* days since 1-jan-1582 */
					fmt->fmt_width += max(6 - compwidth[2], 0);
			}

			if (BTtest(FD_INT, (char *)&(fmt->fmt_prec)) &&
			    BTtest(FD_MONS, (char *)&(fmt->fmt_prec)) &&
			    !BTtest(FD_YRS, (char *)&(fmt->fmt_prec)))
			{	/* total number of months */
				fmt->fmt_width += max(4 - compwidth[1], 0);
			}

			switch(BTnext(FD_HRS-1, (char *)&(fmt->fmt_prec), FD_LEN))
			{
			case(FD_SECS):
				fmt->fmt_width += max(5 - compwidth[5], 0);
				break;
			case(FD_MINS):
				fmt->fmt_width += max(4 - compwidth[4], 0);
				break;
			default:
				break;
			}

			if (fmt->fmt_width > MAXFORM)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM200A_FORMAT_TOO_LONG, 2,
						 sizeof(fmt->fmt_width), (PTR)&(fmt->fmt_width));
			}
			else if (fmt->fmt_width <= 0)
			{
	 			return afe_error(cb,E_FM2016_INVALID_TEMPLATE, 2,
					STlength(fmt->fmt_var.fmt_template),
					(PTR) fmt->fmt_var.fmt_template);
			}
			/* now we attempt to setup the entry and content control masks.
			   this will only be done if the date template does not have
			   certain restricted formatting elements.
			   if this succeeds then the fmt_xmask and fmt_ccary will be set */
			f_dtsetup(fmt, c+1);

			break;
#endif
		case(F_I):
		case(F_F):
		case(F_E):
		case(F_G):
		case(F_N):
			fmt->fmt_var.fmt_char = *string++;

			/* get a number here for field width */

			string = f_gnum(string,&number);
			if (number < 0)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
						 (i4) STlength(string), (PTR) string);
			}
			fmt->fmt_width = number;

			/* should be either end-string or a decimal */

			if (*string != FC_DECIMAL)
			{
				type = F_I;	/* force it to be integer */
				break;
			}
			else if (type == F_I)
			{	/* Iw.n is illegal */
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2011_I_FMT_CANT_HAVE_PREC, 0);
			}
			string++;		/* skip decimal */

			if (type == F_F)
			{
				fmt->fmt_ccsize = 1;
			}
			else
			{
				fmt->fmt_ccsize = 0;
			}

			if (!CMdigit(string))
			{
				break;		/* end of string */
			}

			string = f_gnum(string,&number);
			if (number < 0)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
						 (i4) STlength(string), (PTR) string);
			}
			fmt->fmt_prec = number;

			/*
			**  Allow for the decimal and a leading zero.
			*/
			if (fmt->fmt_prec && fmt->fmt_prec > fmt->fmt_width-2)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2012_PREC_GTR_THAN_WIDTH, 4,
						 sizeof(fmt->fmt_prec), (PTR)&(fmt->fmt_prec),
						 sizeof(fmt->fmt_width), &(fmt->fmt_width));
			}
			break;

		case(F_T):
		case(F_C):
		case(F_CF):
		case(F_CFE):
		case(F_CJ):
		case(F_CJE):
		/* internal format used by unloaddb() */
		case(F_CU):
			string++;
			/* char formats may have column width */
			string = f_gnum(string,&number);
			if (number < 0)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
						 (i4) STlength(string), (PTR) string);
			}
			fmt->fmt_width = number;

			/* should be either end-string or a decimal */

			if (*string == FC_DECIMAL)
			{
				string++;		/* skip decimal */

				if (CMdigit(string))
				{

					string = f_gnum(string,&number);
					if (number < 0)
					{
						if (fmtlen != NULL)
						{
							*fmtlen = string - strstart;
						}
						return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
								 (i4) STlength(string), (PTR) string);
					}

					fmt->fmt_prec = number;

					/*
					**  It's an error to have a precision
					**  greater than the width if the
					**  width is not zero.
					*/
					if (fmt->fmt_width != 0 &&
						number > fmt->fmt_width)
					{
						return afe_error(cb, E_FM2012_PREC_GTR_THAN_WIDTH, 4,
							sizeof(fmt->fmt_prec), (PTR)&(fmt->fmt_prec),
							sizeof(fmt->fmt_width), &(fmt->fmt_width));
					}
				}
			}

			break;

		case(F_B):
			string++;
			if (!CMdigit(string))
			{	/* just 'b' specified. Assume 0 */
				break;
			}

			string = f_gnum(string,&number);
			if (number < 0)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
				return afe_error(cb, E_FM2001_EXPECTED_NUMBER, 2,
						 (i4) STlength(string), (PTR) string);
			}

			fmt->fmt_width = number;
			break;
		case(F_Q):
			string++;
			/* char formats may have column width */
			string = f_gnum(string,&number);
			if (number != 0)
			{
				if (fmtlen != NULL)
				{
					*fmtlen = string - strstart;
				}
			        return afe_error(cb, E_FM2018_EXPECT_ZERO, 0);
			}
			fmt->fmt_width = number;
			break;
	}
	
	fmt->fmt_type = type;		/* NOW fill in type.  No error */
	switch(type)
	{
		case(F_C):
		case(F_CF):
		case(F_CFE):
		case(F_CJ):
		case(F_CJE):
		/* internal format used by unloaddb() */
		case(F_CU):
			/* indicate if format is right-to-left */
			fmt->fmt_var.fmt_reverse = reverse;
			break;
	}

	if (fmtlen != NULL)
	{
		*fmtlen = string - strstart;
	}
	return OK;
}
