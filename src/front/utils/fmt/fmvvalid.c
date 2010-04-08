/*
**	fmvvalid.c
**
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"
# include       <er.h>

/**
** Name:	fmmvalid.c - Check format string against datatype for VIFRED.
**
** Description:
**	This file contains the routines(s) that VIFRED will call to
**	check a format string for validity against a datatype.
**	Difference between this and fmt_isvalid() is that this
**	routine must fill in the length of the datatype if the
**	datatype is one of the character datatypes.
**
** Defines:
** 	fmt_isvalid		Check the validity of format string.
**
** History:
**	08/04/87 (dkh) - Initial version.
**	10/07/87 (dkh) - Fixed jup bug 1078.
**	05/23/88 (dkh) - Added new routine fmt_justify().
**	10/13/88 (dkh) - Fixed venus bug 3580.
**	03-aug-1989	(mgw)
**		fixed for adc_lenchk() interface change.
**	06/15/92 (dkh) - Added support for decimal datatype for 6.5.
**	06/22/93 (dkh) - Fixed bugs 51530 and 51533.  Vifred now properly
**			 supports a field with decimal datatype and
**			 numeric templates.
**	07/19/93 (dkh) - Changed call to adc_lenchk() to match
**			 interface change.  The second parameter is
**			 now a i4  instead of a bool.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	13/08/98 (kitch01) Bugs 84158 & 91491.
**		Decimal field precision does not include space for any sign
**		or period.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* extern's */

FUNC_EXTERN	STATUS		fmt_isvalid();
FUNC_EXTERN	DB_STATUS	adc_lenchk();

/* static's */

/*{
** Name:	fmt_vfvalid - Check validity of format string for VIFRED.
**
** Description:
**	Check the validity of passed in format string against the
**	passed in datatype for VIFRED.  If compatible, check to see
**	if the datatype is one of the following (DB_CHR_TYPE,
**	DB_TXT_TYPE, DB_CHA_TYPE and DB_VCH_TYPE).  Since VIFRED
**	does not allow the user to specify a length for any of
**	the character datatypes, the data length must be obtained
**	from the format.
**
** Inputs:
**	cb	Pointer to a ADF_CB control block.
**	fmt	The format to check on.
**	type
**	    .db_datatype	The ADF typeid of the type.
**	valid	Points to a bool.
**
** Outputs:
**	type	DB_DATA_VALUE for checking the format against.
**	    .db_length		The data length is filled in from
**				the format length if datatype is
**				a character datatype.
**	valid	Contains TRUE if the format is valid for the datatype.
**		Otherwise, contains FALSE.
**	is_str	Set to TRUE if the datatype is one of the character
**		ones and that db_length was set.  FALSE otherwise.
**
**	Returns:
**		OK			If it completed successfully.
**		E_FM600D_BAD_LEN	Bad length for datatype.
**
**		Returns from:
**			fmt_isvalid()
**
** History:
**	08/14/87 (dkh) - Initial version.
**	03-aug-1989	(mgw)
**		adc_lenchk() interface changed to take DB_DATA_VALUE last
**		argument instead of i4.
**	13/08/98 (kitch01) Bugs 84158 & 91491.
**		Decimal field precision does not include space for any sign
**		or period.
*/
STATUS
fmt_vfvalid(cb, fmt, type, valid, is_str)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*type;
bool		*valid;
bool		*is_str;
{
	STATUS		retval;
	DB_DT_ID	dtype;
	i4		usrlen;
	DB_DATA_VALUE	intrlen;
	DB_USER_TYPE	user_type;
	i4		fmtkind;
	i4		precision;
	i4		scale;

	/*
	**  Call fmt_isvalid() to do format versus datatype check.
	*/
	if ((retval = fmt_isvalid(cb, fmt, type, valid)) != OK)
	{
		return(retval);
	}

	if (!valid)
	{
		return(FAIL);
	}

	/*
	**  If the datatype is decimal and the format specification
	**  is not one of numeric template, f and i, then return
	**  FAIL to indicate that the datatype and format are NOT
	**  compatible.
	*/
	if (abs(type->db_datatype) == DB_DEC_TYPE &&
		fmt->fmt_type != F_NT &&
		fmt->fmt_type != F_F &&
		fmt->fmt_type != F_I)
	{
		return(FAIL);
	}

	dtype = abs(type->db_datatype);

	*is_str = FALSE;

	/*
	**  Check to see if datatype is one of the known character ones.
	*/
	if (dtype == DB_CHR_TYPE || dtype == DB_TXT_TYPE ||
		dtype == DB_CHA_TYPE || dtype == DB_VCH_TYPE)
	{
		if (retval = fmt_kind(cb, fmt, &fmtkind) != OK)
		{
			return(retval);
		}
		if (fmtkind == FM_VARIABLE_LENGTH_FORMAT)
		{
			if (fmt->fmt_prec == 0)
			{
				/*
				**  This should never happen.
				*/
				return(FAIL);
			}
			type->db_length = fmt->fmt_prec;
		}
		else
		{
			type->db_length = fmt->fmt_width;
		}

		/*
		**  Call adc_lenchk to get internal length.
		*/
		if (adc_lenchk(cb, TRUE, type, &intrlen) != OK)
		{
			/*
			**  Bad data length for the datatype.
			**  Put out error message.  Put in
			**  a data length of 1 so call to
			**  afe_vtyoutput() will work.
			**  Return value from afe_vtyoutput is
			**  ignored since the datatype must be
			**  OK or we won't be at this point.
			*/
			usrlen = type->db_length;
			type->db_length = 1;
			_VOID_ afe_vtyoutput(cb, type, &user_type);
			return(afe_error(cb, E_FM600D_BAD_LEN,
				usrlen, (PTR) user_type.dbut_name));
		}

		/*
		**  Set internal length for datatype.
		*/
		type->db_length = intrlen.db_length;
		*is_str = TRUE;
	}
	else if (dtype == DB_DEC_TYPE)
	{
		/*
		**  Check if there are any digits after the decimal.
		**  This gives us the "scale value for a decimal
		**  datatype.  If no digits, then scale is zero.
		**  We only handle normal formats for now.
		*/
		if (fmt->fmt_type != F_NT)
		{
			/*
			**  The precision for the decimal datatype is based
			**  on the number of digits in the format.
			**  The degenerate case of a format ending in a
			**  period with no trailing digits is treated as
			**  if the period were not present.  This is how
			**  6.4 handles the problem.
			**
			**  Precision is will be the width minus one for the
			**  a (possible) leading minus sign.  If there is
			**  to be digits after the period, then subtract one
			**  more to account for space occupied by the period.
			**
			**  For numeric templates, the precision/scale values
			**  are encoded in FMT struct member fmt_ccsize which
			**  is set by f_setfmt().
			*/
			/* Bug 84158 and 91491. For a decimal field the precision
			** is based on the total number of digits in the format. 
			** Space for any sign and period is not counted.
			*/
			precision = fmt->fmt_width;
			scale = fmt->fmt_prec;
		}
		else
		{
			precision = DB_P_DECODE_MACRO(fmt->fmt_ccsize);
			scale = DB_S_DECODE_MACRO(fmt->fmt_ccsize);
		}
		type->db_length = DB_PREC_TO_LEN_MACRO(precision);
		type->db_prec = DB_PS_ENCODE_MACRO(precision,scale);
		if (adc_lenchk(cb, TRUE, type, &intrlen) != OK)
		{
			usrlen = type->db_length;
			type->db_length = 1;
			type->db_prec = DB_PS_ENCODE_MACRO(1,0);
			_VOID_ afe_tyoutput(cb, type, &user_type);
			return(afe_error(cb, E_FM600D_BAD_LEN,
				usrlen, (PTR) user_type.dbut_name));
		}
		type->db_length = intrlen.db_length;
	}

	return(OK);
}



/*{
** Name:	fmt_justify - Checks for format justification.
**
** Description:
**	This routine is used for checking whether a format has justification
**	specified for it.  It returns TRUE if justification is specified,
**	FALSE otherwise.
**
** Inputs:
**	fmt	Pointer to FMT structure.
**
** Outputs:
**
**	Returns:
**		TRUE	If format contains justification.
**		FALSE	If format does not contain justification.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	05/23/88 (dkh) - Initial version.
*/
bool
fmt_justify(adfcb, fmt)
ADF_CB	*adfcb;
FMT	*fmt;
{
	bool	retval;

	switch(fmt->fmt_sign)
	{
	    case FS_PLUS:
	    case FS_MINUS:
	    case FS_STAR:
		retval = TRUE;
		break;
	    
	    default:
		retval = FALSE;
	}

	return(retval);
}
