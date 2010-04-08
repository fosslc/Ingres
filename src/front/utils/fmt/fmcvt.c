
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"
# include	<cm.h>


/**
** Name:	fmcvt.c		This file contains the routine for converting
**				a given display value into an internal value.
**
** Description:
**	This file defines:
**
** 	fmt_cvt			Convert a display value to an internal value.
**				This is now a cover routine for fcvt_sql (the
**				old fmt_cvt).
**
**	fcvt_sql		A modified fmt_cvt.  This routine is now passed
**				a pointer to a i4  (sqlpat).  sqlpat is set to
**				TRUE if a QUEL pattern matching character was
**				found AND it was changed to an SQL pattern
**				matching character.  This information is
**				required by higher level routines who need to
**				know when to use the LIKE clause.
**
** History:
**	29-dec-86 (grant)	implemented.
**	28-may-87 (bab)
**		Added code to reverse buffers when field is RL (for
**		Hebrew project), and to handle F_CF, F_CJ formats.
**		Need to pad out display buffers to full rows*cols
**		size when reversing to avoid losing 'leading' blanks.
**	6-jun-87 (danielt) deleted qry_lang and qry_mode arguments to 
**		f_Tstr() 
**	19-nov-87 (kenl) 
**		Renamed old fmt_cvt to fcvt_sql and added cover named
**		fmt_cvt.  See comments above.
**	10/29/87 (dkh) - Changed to not do prefix match on the null string.
**	09/23/88 (dkh) - Fixed jup bug 3233.
**	01/04/89 (dkh) - Changed to call "afe_cvinto()" instead of
**			 "adc_cvinto()".
**	20-apr-89 (cmr)
**		Added support for new format (cu) which is
**		used internally by unloaddb().  It is similar to
**		cf (break at words when printing strings that span 
**		lines) but it breaks on quoted strings as well as
**		words.
**	28-jul-89 (cmr)
**		Added support for 2 new formats 'cfe' and 'cje'.
**	08-dec-89 (bruceb)	Fix for bug 2624.
**		Trim trailing blanks on reversed fields.
**	8/14/90 (elein)
**		Save the error messages properly for the second chance
**		intepretation of money, numbers and dates.
**  09/10/91 (tom) added support for string and date template input masking
**	08/24/92 (dkh) - Fixed acc complaints.
**      04/03/92 (smc) - Cast parameter of STbcompare to match prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* extern's */
FUNC_EXTERN	STATUS	fcvt_sql();

/* static's */
/*{
** Name:	fmt_cvt		- Convert a display value to an internal value.
**
** Description:
**	Cover for routine fcvt_sql.  Calls fcvt_sql with a pointer to a i4,
**	sqlpat, which is ignored by this cover routine.
**	
** Inputs:
**	cb		Points to the current ADF_CB control block.
**
**	fmt		The format that was used to produce the display value.
**
**	display		A DB_DATA_VALUE for the display value.
**		.db_datatype	Must be DB_LTXT_TYPE.
**		.db_length	The length of the display value.
**		.db_data	The actual display value.
**
**	value		A DB_DATA_VALUE into which the display value is
**			to be converted.
**		.db_datatype	The ADF typeid for the internal value.
**		.db_length	The length of the value.
**		.db_data	Must point to an area in which to
**				to place the converted value.
**
**	qry_mode	If TRUE, wild card characters will be converted
**			to their appropriate pattern-match control character
**			and backslashes (which escape wild card characters)
**			will be stripped.
**
**	qry_lang	Determines what the wild card characters are.
**			
**
** Outputs:
**	value		Will contain the converted value.
**		.db_data	Will point to the converted value.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_ERROR	If some error occurred.
**
**		Additional errors from ADF routine adc_cvinto can be returned.
**
** History:
**	19-nov-87 (kenl)	implemented.
*/

STATUS
fmt_cvt(cb, fmt, display, value, qry_mode, qry_lang)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*display;
DB_DATA_VALUE	*value;
bool		qry_mode;
i4		qry_lang;
{
    i4		sqlpat;

    sqlpat = PM_NOT_FOUND;
    return(fcvt_sql(cb, fmt, display, value, qry_mode, qry_lang,
	   PM_NO_CHECK, &sqlpat));
}


/*{
** Name:	fcvt_sql	- Convert a display value to an internal value.
**
** Description:
**	This routine converts a display value that was displayed according
**	to a particular format into an internal value of an ADF type. 
**	If the ADF type is nullable and the display value is the null string,
**	the internal value returned will be null.
**
**	This routine is needed since the formats can output values that
**	aren't legal inputs for the normal ADF conversion routines.
**	For example, the date formats can output dates in several possible
**	ways.  The output is not necessarily a legal INGRES date input.
**	
**	This routine will use the information in the FMT to check the input.
**	
** Inputs:
**	cb		Points to the current ADF_CB control block.
**
**	fmt		The format that was used to produce the display value.
**
**	display		A DB_DATA_VALUE for the display value.
**		.db_datatype	Must be DB_LTXT_TYPE.
**		.db_length	The length of the display value.
**		.db_data	The actual display value.
**
**	value		A DB_DATA_VALUE into which the display value is
**			to be converted.
**		.db_datatype	The ADF typeid for the internal value.
**		.db_length	The length of the value.
**		.db_data	Must point to an area in which to
**				to place the converted value.
**
**	qry_mode	If TRUE, wild card characters will be converted
**			to their appropriate pattern-match control character
**			and backslashes (which escape wild card characters)
**			will be stripped.
**
**	qry_lang	Determines what the wild card characters are.
**			[Currently, this is ignored and the wild card
**			characters are '*', '?', '[', and ']'.]
**
**	flags		Indicate whether to perform pattern matching checks.
**			And also whether input masking is taking place.
**
**	sqlpat		A pointer to a nat.  Set to appropriate value by
**			f_wildcard().
**			
**
** Outputs:
**	value		Will contain the converted value.
**		.db_data	Will point to the converted value.
**
**	sqlpat		TRUE if QUEL pattern matching character was changed
**			to its SQL equivalent, otherwise FALSE.
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
**		The following are internal errors:
**
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
**		E_FM6003_DISPLAY_NOT_LONGTEXT	The datatype of the display
**						parameter is not LONGTEXT.
**		E_FM600B_FMT_DATATYPE_INVALID	The format is invalid for the
**						datatype given.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**		E_FM6001_NO_TEMPLATE		For a numeric or date template
**						format, the fmt parameter does
**						not have the required template.
**
**		The following are specific user errors given in response to
**		their display input:
**
**		E_FM2000_EXPECTED_CHAR_INSTEAD	A different character was
**						expected than that given.
**		E_FM2001_EXPECTED_NUMBER	A number was expected.
**		E_FM2002_EXPECTED_WORD		A word was expected.
**		E_FM2003_WRONG_WORD		The wrong word was used.
**		E_FM2004_DATE_COMP_NOT_ENTERED	For a date display, one of the
**						date components specified in
**						the template was not entered.
**		E_FM2005_DAY_OF_YEAR_TOO_LARGE	The given day of the given year
**						exceeds the number of days in
**						the year.
**		E_FM2006_BAD_CHAR		An illegal character was found.
**
**		Additional errors from ADF routine adc_cvinto can be returned.
**
** History:
**	29-dec-86 (grant)	implemented.
**	26-may-87 (bab)
**		Added code to handle reversed fields, and to handle
**		F_CF, F_CJ formats.
**	19-nov-87 (kenl)
**		Renamed the old fmt_cvt to fcvt_sql.  Added parameter to
**		indicate if a QUEL pattern matching character was changed
**		to its SQL equivalent.
**	8-22-91 (tom)
**		Changed old argument pm_cmd to more generic "flags".
**		It now carries a bit telling if input masking is active
**		which affects whether or not we do adf conversion.
**		This should have no effect on any existing code,
**		and seems better than creating yet another entry point.
*/

STATUS
fcvt_sql(cb, fmt, display, value, qry_mode, qry_lang, flags, sqlpat)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*display;
DB_DATA_VALUE	*value;
bool		qry_mode;
i4		qry_lang;
i4		flags;
i4		*sqlpat;
{
    STATUS		status;
    i4			rows;
    i4			cols;
    DB_TEXT_STRING	*disp;
    DB_DATA_VALUE	from;
    AFE_DCL_TXT_MACRO(DB_MAXSTRING) buffer;
    DB_TEXT_STRING	*text;
    ADF_CB		saved_cb;
    STATUS		saved_status;
    i4			saved_emsglen;
    char		saved_errmsgp[DB_MAXSTRING];
    u_char		*c;
    u_char		*cend;
    i4			len;
    i4			cursize;
    i4			i;
    bool		reverse;
    u_char		decimal_sym;
    i4			nullstrlen;
    i4			class;
    bool		is_null;
    i4			pm_cmd = flags & PM_CMD_MASK;
    bool		inp_mask = (flags & INP_MASK_ACTIVE) != 0;

    if (fmt == NULL || display == NULL || value == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    if (display->db_datatype != DB_LTXT_TYPE)
    {
	return afe_error(cb, E_FM6003_DISPLAY_NOT_LONGTEXT, 2,
			 sizeof(display->db_datatype),
			 (PTR)&(display->db_datatype));
    }

    status = fmt_size(cb, fmt, value, &rows, &cols);
    if (status != OK)
    {
	return status;
    }

    disp = (DB_TEXT_STRING *) (display->db_data);
    text = (DB_TEXT_STRING *) &buffer;
    nullstrlen = cb->adf_nullstr.nlst_length;

    if (AFE_NULLABLE_MACRO(value->db_datatype) &&
	((disp->db_t_count == 0 && (nullstrlen == 0 || nullstrlen > cols)) ||
	 (disp->db_t_count > 0 && nullstrlen > 0 && nullstrlen <= cols &&
	  disp->db_t_count == nullstrlen &&
	  STbcompare((char *)disp->db_t_text, disp->db_t_count,
		     cb->adf_nullstr.nlst_string, nullstrlen, TRUE) == 0)))
    {   /* display is null string so return null value */
	status = adc_getempty(cb, value);
	return status;
    }

    switch (fmt->fmt_type)
    {
    case F_NT:
    case F_DT:
    case F_ST:
	if (fmt->fmt_type == F_NT)
	{   /* scan input using numeric template */
	    status = f_innt(cb, fmt, display, value);
	}
	else if (fmt->fmt_type == F_ST)
	{   /* scan input using a string template */
	    status = f_inst(cb, fmt, display, value, &is_null);
	}
	else
	{   /* scan input using a date template */
	    status = f_indate(cb, fmt, display, value);

	    /* if we failed on date conversion, and input masking is
	       active, then we do not want to attempt adf conv. */
	    if (status != OK && inp_mask && fmt->fmt_emask)
	    {
		/* if the target is nullable we have to make
		  one last attempt at conversion just for the sake
		  of testing for null. Our string conversion routine (f_inst)
		  recognizes that it's a date, and tells us if it is null */
		if (AFE_NULLABLE_MACRO(value->db_datatype))
		{
		    _VOID_ f_inst(cb, fmt, display, value, &is_null);

		    /* if the string conversion yeilds a null, then we
		       say the status is ok */
		    if (is_null)
			status = OK;
		}
		/* because we are input masking.. we do not want to
		   perform the afe_cvinto conversion attempt */
		break;
	    }
	}

	if (status != OK)
	{   /* didn't pass template scan. try the generic conversion */

	    /* save error info from template scan */
	    saved_status = status;
	    saved_emsglen = cb->adf_errcb.ad_emsglen;
	    STcopy( cb->adf_errcb.ad_errmsgp, saved_errmsgp);

	    cb->adf_errcb.ad_emsglen    = 0;

	    status = afe_cvinto(cb, display, value);
	    if (status != E_DB_OK)
	    {
		/* restore error info from template scan */
		status = saved_status;
	        STcopy( saved_errmsgp, cb->adf_errcb.ad_errmsgp);
	        cb->adf_errcb.ad_emsglen = saved_emsglen;
	    }
	}
	break;

    case F_T:
	/*
	** convert from T format to text, replacing \c & \ddd with control char 
	*/
	text->db_t_count = f_Tstr(disp->db_t_text, disp->db_t_count,
				  text->db_t_text, DB_MAXSTRING);

	if (qry_mode)
	{
	    IIAFddcDetermineDatatypeClass(value->db_datatype, &class);
            if (class != DATE_DTYPE)
	    {
	        f_wildcard(text, qry_lang, flags, sqlpat);
	    }
	}

	from.db_datatype = DB_LTXT_TYPE;
	from.db_length = DB_MAXSTRING + DB_CNTSIZE;
	from.db_data = (PTR) text;

	status = afe_cvinto(cb, &from, value);
	break;

    case F_C:
    case F_CF:
    case F_CFE:
    case F_CJ:
    case F_CJE:
    /* internal format used by unloaddb() */
    case F_CU:
	_VOID_ fmt_isreversed(cb, fmt, &reverse);
	if (reverse || fmt->fmt_sign == FS_PLUS || fmt->fmt_sign == FS_MINUS
	    || qry_mode)
	{
	    /* using the buffer space provided by text as a temp. */
	    MEcopy((PTR)disp->db_t_text, (u_i2)disp->db_t_count,
		   (PTR)text->db_t_text);
	    text->db_t_count = disp->db_t_count;

	    /*
	    **  Reverse the buffer contents, one row at a time.
	    **  Expects that fmt_size will return non-zero row
	    **  count--because this is used only in the
	    **  forms system, and no '0' (variable) width'd fields
	    **  are allowed.  (Simply does not reverse if 0 row count
	    **  returned.)
	    */
	    if (reverse)
	    {
		c = text->db_t_text;
		len = rows * cols;
		cursize = text->db_t_count;
		/*
		** Need to pad out reversed fields to full width of
		** the columns, else end up swapping a shortened
		** string, losing leading blanks on last row.
		** (An optimization would be to only pad out to
		** length required for the rows actually used; also
		** to do so only if not FS_PLUS/F_C.)
		*/
		if (len > cursize)
		{
			MEfill((u_i2)(len - cursize), (unsigned char)' ',
				(PTR)(c + cursize));
			text->db_t_count = len;
		}
		for (i = 1; i <= rows; i++)
		{
		    f_revrsbuf(cols, TRUE, c);
		    c += cols;
		}
		/* Trim any of what are now trailing blanks. */
		cend = text->db_t_text;
		c = cend + text->db_t_count - 1;
		while ( c >= cend && *c-- == ' ' )
		{
		    --(text->db_t_count);
		}

	    }

	    /* only F_C formats are currently looking at FS_PLUS status */
	    if ((fmt->fmt_sign == FS_PLUS || fmt->fmt_sign == FS_MINUS)
		&& fmt->fmt_type == F_C)
	    {
		/* skip beginning blanks */
		c = text->db_t_text;
		cend = c + text->db_t_count;
		for (; c < cend && CMspace(c); CMnext(c))
		    ;

		len = cend - c;

		/* if there actually were some leading blanks, re-locate text */
		if (len != text->db_t_count)
		{
		    /* copy text onto itself--OK per ME CL specification */
		    MEcopy((PTR)c, (u_i2)len, (PTR)(text->db_t_text));
		    text->db_t_count = len;
		}
	    }

	    if (qry_mode)
	    {
	        IIAFddcDetermineDatatypeClass(value->db_datatype, &class);
                if (class != DATE_DTYPE)
	        {
	            f_wildcard(text, qry_lang, pm_cmd, sqlpat);
	        }
	    }

	    from.db_datatype = DB_LTXT_TYPE;
	    from.db_length = DB_MAXSTRING + DB_CNTSIZE;
	    from.db_data = (PTR) text;

	    status = afe_cvinto(cb, &from, value);
	    break;
	}
	/* else, fall through */

    default:
	if (abs(value->db_datatype) == DB_INT_TYPE)
	{
	    /* check that entered string does not contain a decimal point */

	    decimal_sym = (u_char) cb->adf_decimal.db_decimal;
	    disp = (DB_TEXT_STRING *) display->db_data;
	    c = disp->db_t_text;
	    cend = c + disp->db_t_count;
	    for (; c < cend; c++)
	    {
		if (*c == decimal_sym)
		{
		    return afe_error(cb, E_FM2006_BAD_CHAR, 2, 1, (PTR)c);
		}
	    }
	}

	status = afe_cvinto(cb, display, value);
	break;
    }

    return status;
}
