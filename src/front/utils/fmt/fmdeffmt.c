
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = dg8_us5
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"
# include	<st.h>

/**
** Name:	fmdeffmt.c	This file contains the routine which creates
**				a default format for a given type.
**
** Description:
**	This file defines:
**
**	fmt_deffmt		Create a default format for a given type.
**
** History:
**	17-dec-86 (grant)	implemented.
**	09/19/87 (dkh) - Added FRS_F4 and FRS_F8 so that different
**			 floating point formats are used for form fields.
**			 This is for 5.0 compatibility and DG.
**	23-may-88 (bruceb)
**		Default f4/f8 formats changed from f20.3 to n20.3.
**	12/31/88 (dkh) - Perf changes.
**	21-apr-92 (Kevinm)
**		Added dg8_us5 to NO_OPTIM.
**	06/14/92 (dkh) - Added support for decimal dataype for 6.5.
**	18-feb-1998 (kitch01) 
**	    The change for bugs 91491 & 84158 caused E_VF0008. 
**	    Remove the addition of 1 to decimal precision for leading minus sign
**	    and 1 for decimal point, these are not taken into consideration when
**	    calculating decimal precision. For a field of decimal(31), such a 
**	    calculation gives a field with precision 32, greater than the 
**	    allowed maximum.
**  01-Apr-1999 (Kitch01)
**		The above change has been backed out and also should be dated 1999.
**		Decimal fields do not take account of the decimal point of leading minus
**      sign, however here we set the default format for a decimal field to fx.n
**		where x is the precision and DOES take account of the decimal point and
**		minus sign. To resolve the above problem ensure that this precision cannot
**		be greater than the maximum decimal precision. removed 2 obsolete variables.
**		decimal(31) will become f31
**		decimal(20,15) will become f22.15
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Jul-2004 (schka24)
**	    Add default for i8.
**	11-Sep-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**/


# define    FRM_I1	ERx("f6")	     /* default format for i1 fields */
# define    FRM_I2	ERx("f6")	     /* default format for i2 fields */
# define    FRM_I4	ERx("f13")	     /* default format for i4 fields */
# define    FRM_I8	ERx("f20")	     /* default format for i8 fields */
# define    FRM_F4	ERx("n10.3")	     /* default format for f4 fields */
# define    FRM_F8	ERx("n10.3")	     /* default format for f8 fields */
# define    FRS_F4	ERx("n20.3")	     /* default fmt for frs f4 fields */
# define    FRS_F8	ERx("n20.3")	     /* default fmt for frs f8 fields */
# define    FRM_DATE	ERx("c25")	     /* default format for date fields */
# define    FRM_ADATE	ERx("c17")	     /* default format for ANSI date fields */
# define    FRM_ATMWO	ERx("c21")	     /* default format for 
				             **	ANSI time without time stamp fields */
# define    FRM_ATMW	ERx("c31")	     /* default format for
				             **	ANSI time with time stamp fields */
# define    FRM_ATME	ERx("c31")	     /* default format for ANSI 
				             **	time with local time stamp fields */
# define    FRM_ATSWO	ERx("c39")	     /* default format for ANSI timestamp 
					     ** without time stamp fields */
# define    FRM_ATSW	ERx("c49")	     /* default format for ANSI 
				             **	timestamp with time stamp fields */
# define    FRM_ATSTMP	ERx("c49")	     /* default format for ANSI timestamp 
				             **	with local time stamp fields */
# define    FRM_AINYM	ERx("c15")	     /* default format for 
				             **	ANSI interval year to month */
# define    FRM_AINDS	ERx("c45")	     /* default format for 
				             **	ANSI interval day to second */
# define    FRM_MONEY	ERx("$---------------.nn")
					/* default format for money fields */

# define    FRM_WI1	6		/* default width for an i1 field */
# define    FRM_WI2	6		/* default width for an i2 field */
# define    FRM_WI4	13		/* default width for an i4 field */
# define    FRM_WI8	20		/* default width for an i8 field */
# define    FRM_WF4	10		/* default width for an f4 field */
# define    FRM_WF8	10		/* default width for an f8 field */
# define    FRS_WF4	20		/* default width for a frs f4 field */
# define    FRS_WF8	20		/* default width for a frs f8 field */
# define    FRM_WDATE	25		/* default width for a date field */
# define    FRM_WADATE	17	        /* default width for ANSI date fields */
# define    FRM_WATMWO	21	        /* default format for ANSI time without 
					** time stamp fields */
# define    FRM_WATMW	31	        /* default format for ANSI time with 
					** time stamp fields */
# define    FRM_WATME	31	        /* default format for ANSI time with local 
					** time stamp fields */
# define    FRM_WATSWO	39	        /* default format for ANSI timestamp 
					** without time stamp fields */
# define    FRM_WATSW	49	        /* default format for ANSI 
				        ** timestamp with time stamp fields */
# define    FRM_WATSTMP	49	        /* default format for ANSI timestamp 
				        ** with local time stamp fields */
# define    FRM_WAINYM	15	        /* default format for 
				        ** ANSI interval year to month */
# define    FRM_WAINDS	45	        /* default format for 
				        ** ANSI interval day to second */
# define    FRM_WMONEY	19		/* default width for a money field */

# define    SEC_DISPLAY_LEN  128	/* Display first 128 bytes by default*/
/* extern's */
/* static's */

GLOBALREF	i4	IIprecision;
GLOBALREF	i4	IIscale;

/*{
** Name:	fmt_ideffmt	- Create a default format for a given type.
**
** Description:
**	This routine creates a default format string given an ADF type.
**	The format string that is created can be used for display to the user
**	or an input to other FMT routines.
**
** Inputs:
**	cb		Points to the current ADF_CB
**			control block.
**
**	type		This is a DB_DATA_VALUE being used to hold the
**			type and length.
**
**		.db_datatype	The ADF datatype id for the type.
**
**		.db_length	The length of the type.
**
**	maxcharwidth	This is the maximum width that a string type can be
**			before the format is wrapped.
**
**	forreport	This applies for string and float types.
**			For string:
**			If TRUE, the format is of the form 'cf0.m' where 'm' is
**			the maxcharwidth. (This is used for reporting.)
**			If FALSE, the format is 'cn.x' where n is the length
**			of the string and 'x' is a calculated width such that
**			the string is most evenly distributed over the multiple
**			lines. (This is used for form input/ouput.)
**			For float:
**			If TRUE, default format is "n10.3".
**			If FALSE, default format is "f20.3", for compatibility
**			and DG.
**
**	fmtstr		Must point to an allocated format string.
**
** Outputs:
**	fmtstr		Will contain the default format string for the type.
**	cols		Number of columns occupied by the default format.
**	lines		Number of lines occupied by the default format.
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
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
**		E_FM6005_DATATYPE_LEN_INVALID	The length of the datatype is
**						illegal.
**
**		Also may return values from afe_ficoerce.
**
** History:
**	17-dec-86 (grant)	implemented.
**	18-feb-1998 (kitch01) 
**	    The change for bugs 91491 & 84158 caused E_VF0008. 
**	    Remove the addition of 1 to decimal precision for leading minus sign
**	    and 1 for decimal point, these are not taken into consideration when
**	    calculating decimal precision. For a field of decimal(31), such a 
**	    calculation gives a field with precision 32, greater than the 
**	    allowed maximum.
**  01-Apr-1999 (Kitch01)
**		The above change has been backed out and also should be dated 1999.
**		Decimal fields do not take account of the decimal point of leading minus
**      sign, however here we set the default format for a decimal field to fx.n
**		where x is the precision and DOES take account of the decimal point and
**		minus sign. To resolve the above problem ensure that this precision cannot
**		be greater than the maximum decimal precision. removed 2 obsolete variables.
**		decimal(31) will become f31
**		decimal(20,15) will become f22.15
**	11-sep-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/

STATUS
fmt_ideffmt(cb, type, maxcharwidth, forreport, fmtstr, cols, lines)
ADF_CB		*cb;
DB_DATA_VALUE	*type;
i4		maxcharwidth;
bool		forreport;
char		*fmtstr;
i4		*cols;
i4		*lines;
{
    i4	    length;
    i4	    width;
    i4	    depth;
    STATUS  status;
    char    *mny_fmt;
    i4	    mny_prec;
    i4	    mny_len;

    if (type == NULL || fmtstr == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    if (cols != NULL)
    {
	*cols = 0;
    }

    if (lines != NULL)
    {
	*lines = 1;
    }

    length = type->db_length;
    if (AFE_NULLABLE_MACRO(type->db_datatype))
    {
	length--;
    }

    /* CHECK FOR ENVIRONMENT SETTINGS OF DEFAULT FORMATS FOR EACH TYPE */

    switch (abs(type->db_datatype))
    {
    default:
	status = afe_dplen(cb, type, &length);
	if (status != OK)
	{
	    return status;
	}

	/* fall through */

    case DB_TXT_TYPE:
    case DB_VCH_TYPE:
	length -= DB_CNTSIZE;

	/* fall through */

    case DB_CHA_TYPE:
    case DB_CHR_TYPE:
	if (length > maxcharwidth)
	{   /* use folding default format */
	    if (forreport)
	    {
		STprintf(fmtstr, ERx("cf0.%d"), maxcharwidth);
		if (cols != NULL)
		{
		    /*
		    **  Report writer should never call fmt_sdeffmt().
		    **  But just in case, we fake up some return values.
		    */
		    *cols = maxcharwidth;
		}
	    }
	    else
	    {	/* for form I/O */
		depth = (length + maxcharwidth - 1) / maxcharwidth;
		width = (length + depth - 1) / depth;
		STprintf(fmtstr, ERx("c%d.%d"), length, width);
		if (cols != NULL)
		{
		    *cols = width;
		    *lines = depth;
		}
	    }

	}
	else
	{
	    STprintf(fmtstr, ERx("c%d"), length);
	    if (cols != NULL)
	    {
		*cols = length;
	    }
	}
	break;

    case DB_INT_TYPE:
	switch (length)
	{
	case 1:
	    STcopy(FRM_I1, fmtstr);
	    if (cols != NULL)
	    {
	        *cols = FRM_WI1;
	    }
	    break;
	case 2:
	    STcopy(FRM_I2, fmtstr);
	    if (cols != NULL)
	    {
	        *cols = FRM_WI2;
	    }
	    break;
	case 4:
	    STcopy(FRM_I4, fmtstr);
	    if (cols != NULL)
	    {
		*cols = FRM_WI4;
	    }
	    break;
	case 8:
	    STcopy(FRM_I8, fmtstr);
	    if (cols != NULL)
	    {
		*cols = FRM_WI8;
	    }
	    break;
	default:
	    return afe_error(cb, E_FM6005_DATATYPE_LEN_INVALID, 4,
			     sizeof(length), (PTR)&length,
			     sizeof(type->db_datatype),
			     (PTR)&(type->db_datatype));
	}
	break;

    case DB_FLT_TYPE:
	switch (length)
	{
	case 4:
		STcopy(forreport ? FRM_F4 : FRS_F4, fmtstr);
		if (cols != NULL)
		{
		    *cols = forreport ? FRM_WF4 : FRS_WF4;
		}
	    break;
	case 8:
		STcopy(forreport ? FRM_F8 : FRS_F8, fmtstr);
		if (cols != NULL)
		{
		    *cols = forreport ? FRM_WF8 : FRS_WF8;
		}
	    break;
	default:
	    return afe_error(cb, E_FM6005_DATATYPE_LEN_INVALID, 4,
			     sizeof(length), (PTR)&length,
			     sizeof(type->db_datatype),
			     (PTR)&(type->db_datatype));
	}
	break;

    case DB_DTE_TYPE:
	STcopy(FRM_DATE, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WDATE;
	}
	break;

    case DB_ADTE_TYPE:
	STcopy(FRM_ADATE, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WADATE;
	}
	break;

    case DB_TMWO_TYPE:
	STcopy(FRM_ATMWO, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WATMWO;
	}
	break;

    case DB_TMW_TYPE:
	STcopy(FRM_ATMW, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WATMW;
	}
	break;

    case DB_TME_TYPE:
	STcopy(FRM_ATME, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WATME;
	}
	break;

    case DB_TSWO_TYPE:
	STcopy(FRM_ATSWO, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WATSWO;
	}
	break;

    case DB_TSW_TYPE:
	STcopy(FRM_ATSW, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WATSW;
	}
	break;

    case DB_TSTMP_TYPE:
	STcopy(FRM_ATSTMP, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WATSTMP;
	}
	break;

    case DB_INYM_TYPE:
	STcopy(FRM_AINYM, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WAINYM;
	}
	break;

    case DB_INDS_TYPE:
	STcopy(FRM_AINDS, fmtstr);
	if (cols != NULL)
	{
	    *cols = FRM_WAINDS;
	}
	break;

    case DB_MNY_TYPE:
	mny_fmt = FRM_MONEY;
	mny_len = STlength(mny_fmt) - 3;
	mny_prec = cb->adf_mfmt.db_mny_prec;
	if (mny_prec > 0)
	{
	    /* add one for decimal point */
	    mny_prec++;
	}

	STcopy(ERx("\""), fmtstr);
	STlcopy(mny_fmt, fmtstr+1, mny_len+mny_prec);
	STcat(fmtstr, ERx("\""));
	if (cols != NULL)
	{
	    *cols = mny_len + mny_prec;
	}
	break;

    case DB_DEC_TYPE:
	IIprecision = DB_P_DECODE_MACRO(type->db_prec);
	IIscale = DB_S_DECODE_MACRO(type->db_prec);

	/*
	**  Add one for a possible leading negative sign.
	*/
	IIprecision++;

	if (IIscale)
	{
		/*
		**  Add one for the decimal point.
		*/
		IIprecision++;
		/* ensure IIprecision is not greater than max allowed */
		IIprecision = IIprecision <= DB_MAX_DECPREC ? IIprecision : DB_MAX_DECPREC; 

		STprintf(fmtstr, ERx("f%d.%d"), IIprecision, IIscale);
	}
	else
	{
		/* ensure IIprecision is not greater than max allowed */
		IIprecision = IIprecision <= DB_MAX_DECPREC ? IIprecision : DB_MAX_DECPREC; 
		STprintf(fmtstr, ERx("f%d"), IIprecision);
	}
	if (cols != NULL)
	{
	    *cols = IIprecision;
	}
	break;
    }

    return OK;
}



/*{
** Name:	fmt_sdeffmt - Get default format and size information.
**
** Description:
**	See header for fmt_ideffmt() for details.
**
** Inputs:
**	See header for fmt_ideffmt() for details.
**
** Outputs:
**	Returns:
**		status	STATUS returned by fmt_ideffmt().
**	Exceptions:
**
** Side Effects:
**
** History:
**	12/31/88 (dkh) - Initial version.
*/
STATUS
fmt_sdeffmt(cb, type, maxcharwidth, forreport, fmtstr, cols, lines)
ADF_CB		*cb;
DB_DATA_VALUE	*type;
i4		maxcharwidth;
bool		forreport;
char		*fmtstr;
i4		*cols;
i4		*lines;
{
	return(fmt_ideffmt(cb, type, maxcharwidth, forreport, fmtstr,
		cols, lines));
}



/*{
** Name:	fmt_deffmt - Cover for fmt_ideffmt.
**
** Description:
**	A simple cover to fmt_ideffmt.
**
** Inputs:
**	See header for fmt_ideffmt for details.
**
** Outputs:
**	See header for fmt_ideffmt() for details.
**
**	Returns:
**		status	STATUS returned by fmt_ideffmt().
**	Exceptions:
**
** Side Effects:
**
** History:
**	12/31/88 (dkh) - Initial version.
*/
STATUS
fmt_deffmt(cb, type, maxcharwidth, forreport, fmtstr)
ADF_CB		*cb;
DB_DATA_VALUE	*type;
i4		maxcharwidth;
bool		forreport;
char		*fmtstr;
{
	return(fmt_ideffmt(cb, type, maxcharwidth, forreport, fmtstr,
		NULL, NULL));
}
