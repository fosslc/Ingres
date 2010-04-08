/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cv.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	<er.h>
# include	<st.h>
# include	<ug.h>
# include	<nm.h>
# include	<rtype.h> 
# include	<rglob.h> 

/*
** R_G_DOUBLE -  Process a numeric string constant pointed to by Tokchar
**		 and determine if it represents an integer, a float, or
**		 a decimal number.  Then, effect the proper alpha to numeric
**		 conversion into a static DB_DATA_VALUE.
**
**		 This routine allows for coercion of integer values to float
**		 if St_compatible is NOT set, interpretation of excessively
**		 large integers as DECIMAL if not pre-6.5, and coercion of
**		 DECIMAL to float if II_NUMERIC_LITERAL is "float".
**
**		 This routine allocates it's db_data with MEreqmem, and it is
**		 the client's responsibility to free and/or reallocate this
**		 memory.
**
** Comments from the original atof() routine of long ago ...
**
**  ASCII TO FLOATING CONVERSION
**
**	Converts the string 'str' to floating point and stores the
**	result into the cell pointed to by 'val'.
**
**	Returns zero for ok, negative one for syntax error, and
**	positive one for overflow.
**	(actually, it doesn't check for overflow)
**
**	The syntax which it accepts is pretty much what you would
**	expect.  Basically, it is:
**		{<sp>} [+|-] {<sp>} {<digit>} [.{digit}] {<sp>} [<exp>]
**	where <exp> is "e" or "E" followed by an integer, <sp> is a
**	space character, <digit> is zero through nine, [] is zero or
**	one, and {0} is zero or more.
**
** Parameters:
**	dbv -		pointer to a DB_DATA_VALUE
**
** Outputs:
**	dbv -		The DB_DATA_VALUE is populated with
**				db_datatype
**				db_length
**				db_prec
**				db_data
**			which represents the determined numeric type and the
**			actual converted numeric value.
**
** Returns:
**	-1 -	Error.  Could be syntax, could be an unexpressable value.
**	 0 -	Success.
**	 1 -	Overflow.
**
** History:
**	2/12/79 (eric)
**		Fixed bug where '0.0e56' (zero mantissa with an exponent)
**		reset the mantissa to one.
**	5/20/81 (ps)
**		Modified for report formatter.
**	01-nov-88 (sylviap)
**		Added break in case statement if sign is '-'.
**	8/6/90 (elein) b32098
**		Since R6 is supposed to handle integer arithmetic,
**		make sure integer values are in proper range.
**		Each caller should use this value in an int dbv
**		(NOT a float dbv) if isinteger is true.
**	8-dec-1992 (rdrane)
**		Totally re-written to accomodate DECIMAL datatype -
**		parameterization, methodology, etc., all re-done.
**		We now use the CV and AFE routines to effect conversion
**		from string, and the r_g_set() routines to traverse Tokchar.
**		All this greatly simplifies our parsing.  Incorporated fix
**		for bug 48337.  Still doesn't return 1 for overflow ...
**	14-dec-1992 (rdrane)
**		Test II_NUMERIC_LITERAL only against the string "float" -
**		davel says that that's the only value which can be trusted!
**		Fix typo in bug number in 8-dec-1992 comment.
**      08-30-93 (huffman)
**              add <me.h>
**	31-aug-1993 (rdrane)
**		Cast u_char db_t_text references to (char *) so prototyping
**		in STzapblank() doesn't complain.
**	11-oct-1993 (rdrane)
**		Correct handling of II_NUMERIC_LITERAL.  If not set at all,
**		we were erroneously treating DECIMAL as FLOAT!
**	4-jan-1994 (rdrane)
**		Use OpenSQL level to determine 6.5 syntax features support.
**		Rename is_pre_65 to dec_support, and reverse sense of boolean
**		compares.
**	16-aug-1999 (carsu07)
**	 	Moving change 436813 for bug 92001 from rreport.c to rgdouble.c.
**		This change needs to be moved to prevent bug 98262.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


i4
r_g_double(result_dbdv)
DB_DATA_VALUE	*result_dbdv;
{
	f8	f8_value;
	bool	is_int;
	bool	is_dec;
	bool	dec_support;
	i4	num_digs;
	i4	num_scale;
	STATUS	cv_stat;
	i4	*expon;
	char	*ptr;
	char	*save_Tokchar;
	DB_TEXT_STRING *textstr;
	char	save_char;
	DB_DATA_VALUE ltxt_dbdv;
	DB_DATA_VALUE dec_dbdv;
	char    tmpdecimal;


	is_int = TRUE;
	is_dec = FALSE;

	num_digs = 0;
	num_scale = 0;

	dec_support = FALSE;
	if  (STcompare(IIUIcsl_CommonSQLLevel(),UI_LEVEL_65) >= 0)
	{
		dec_support = TRUE;
	}

	save_Tokchar = Tokchar;

	/*
	** Skip leading blanks, look for a sign, and if found
	** skip that too.
	*/
	if (r_g_skip() == TK_SIGN)
	{
		CMnext(Tokchar);
	}

	/*
	** Skip blanks after any sign, and start collecting digits up
	** to any decimal point.
	*/
	if  (r_g_skip() == TK_NUMBER)
	{
		while (CMdigit(Tokchar))
		{
			num_digs++;
			CMnext(Tokchar);
		}
	}

	/*
	** Check for any fractional part.
	*/
	if (*Tokchar == '.')
	{
		CMnext(Tokchar);
		is_int = FALSE;
		if  (dec_support)
		{
			is_dec = TRUE;
		}
		while (CMdigit(Tokchar))
		{
			num_digs++;
			num_scale++;
			CMnext(Tokchar);
		}
	}

	/*
	** Skip blanks before any possible exponent.  If found, continue
	** and parse the value of the exponent.
	*/
	if  (r_g_skip() == TK_ALPHA)
	{
		if  ((*Tokchar == 'e') || (*Tokchar == 'E'))
		{
			is_int = FALSE;
			is_dec = FALSE;
			CMnext(Tokchar);
			if (r_g_long(&expon) != 0)
			{
				return (-1);
			}
		}
		/*
		** If it's not a valid exponent, let it go for
		** now (the previous RW did!).  It will probably
		** precipitatre its own error soon enough!
		*/
	}

	/*
	** Temporarily NULL terminate the string, trim all blanks from it,
	** and place it into textstr for later use.
	*/
	save_char = *Tokchar;
	*Tokchar = EOS;
	textstr = (DB_TEXT_STRING *)MEreqmem((u_i4)0,
				(u_i4)(sizeof(u_i2) +
					((Tokchar - save_Tokchar) + 1)),
				TRUE,(STATUS *)NULL);
	if  (textstr == (DB_TEXT_STRING *)NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_g_double - textstr"));
	}
	textstr->db_t_count = STzapblank(save_Tokchar,
					 (char *)&textstr->db_t_text[0]);
	*Tokchar = save_char;

	/*
	** We should always be able to convert it to a float ...
	*/
	if  (CVaf(&textstr->db_t_text[0],'.',&f8_value) != OK)
	{
		_VOID_ MEfree((PTR)textstr);
		return(-1);
	}

	/*
	** If it's an integer but would overflow an i4, then reset it to
	** decimal and continue.  If we're re pre-6.5, though, complain,
	** reset it to float, and continue.  If we're running in 5.0
	** compatible mode (integers were not allowed in 5.0), reset to float
	** and continue.
	*/
	if (is_int)
	{
		if  ((f8_value > MAXI4) || (f8_value < MINI4))
		{
			is_int = FALSE;
			if  (!dec_support)
			{
				r_error(0x40D,NONFATAL,Cact_tname,
					Cact_attribute,Cact_command,
					Cact_rtext,NULL);
			}
			else
			{
				is_dec = TRUE;
			}
		}
		else if  (St_compatible)
		{
			is_int = FALSE;
		}
		else
		{
			result_dbdv->db_datatype = DB_INT_TYPE;
			result_dbdv->db_length = sizeof(i4);
			result_dbdv->db_data = MEreqmem((u_i4)0,
						(u_i4)result_dbdv->db_length,
						TRUE,(STATUS *)NULL);
			if  (result_dbdv->db_data == (PTR)NULL)
			{
				IIUGbmaBadMemoryAllocation(
						ERx("r_g_double - i4"));
			}
			cv_stat = CVal(&textstr->db_t_text[0],
				       (i4 *)result_dbdv->db_data);
			_VOID_ MEfree((PTR)textstr);
			if  (cv_stat != OK)
			{
				return(-1);
			}
			return(0);
		}
	}

	/*
	** If it's a decimal but would overflow on either total digits or
	** scale, then reset to float continue.  Note that is_dec will never
	** be TRUE if we're pre-6.5!
	*/
	if (is_dec)
	{
		if  (num_digs > DB_MAX_DECPREC)
		{
			is_dec = FALSE;
			r_error(0x415,NONFATAL,Cact_tname,Cact_attribute,
				Cact_command,Cact_rtext,NULL);
		}
		else
		{
			/*
			** textstr was setup unconditionally above
			*/
			ltxt_dbdv.db_datatype = DB_LTXT_TYPE;
			ltxt_dbdv.db_length = sizeof(u_i2) +
					      ((Tokchar - save_Tokchar) + 1);
			ltxt_dbdv.db_prec = 0;
			ltxt_dbdv.db_data = (PTR)textstr;
			result_dbdv->db_datatype = DB_DEC_TYPE;
			result_dbdv->db_length = DB_PREC_TO_LEN_MACRO(num_digs);
			result_dbdv->db_prec = DB_PS_ENCODE_MACRO(num_digs,
								  num_scale);
			result_dbdv->db_data = MEreqmem((u_i4)0,
					(u_i4)result_dbdv->db_length,
					TRUE,(STATUS *)NULL);
			if  (result_dbdv->db_data == (PTR)NULL)
			{
				IIUGbmaBadMemoryAllocation(
						ERx("r_g_double - dec value"));
			}
			tmpdecimal=Adf_scb->adf_decimal.db_decimal;
			Adf_scb->adf_decimal.db_decimal = '.';
			cv_stat = afe_cvinto(Adf_scb,&ltxt_dbdv,result_dbdv);
			Adf_scb->adf_decimal.db_decimal = tmpdecimal;
			_VOID_ MEfree((PTR)textstr);
			textstr = (DB_TEXT_STRING *)NULL;
			if  (cv_stat != OK)
			{
				return(-1);
			}
		}
	}

	/*
	** If it's a decimal and II_NUMERIC_LITERAL is anything but "float",
	** then leave it as decimal.  Otherwise keep going and make it a float.
	** Note once again that is_dec will never be TRUE if we're pre-6.5!
	*/
	if  (is_dec)
	{
		NMgtAt(ERx("II_NUMERIC_LITERAL"),&ptr);
		if  ((ptr == NULL) || (*ptr == EOS))
		{
			return(0);
		}
		CVlower(ptr);
		if  (STcompare(ptr,ERx("float")) != 0)
		{
			return(0);
		}
	}

	/*
	** For any of several reasons, it must be a float.
	*/

	result_dbdv->db_datatype = DB_FLT_TYPE;
	result_dbdv->db_length = sizeof(f8);
	result_dbdv->db_prec = 0;
	result_dbdv->db_data = MEreqmem((u_i4)0,(u_i4)sizeof(f8),
					TRUE,(STATUS *)NULL);
	if  (result_dbdv->db_data == (PTR)NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_g_double - float value"));
	}
	*((f8 *)result_dbdv->db_data) = f8_value;

	/*
	** If we're coercing a decimal, then we've already free'd textstr!
	*/
	if  (textstr != (DB_TEXT_STRING *)NULL)
	{
		_VOID_ MEfree((PTR)textstr);
	}
	return(0);
}
