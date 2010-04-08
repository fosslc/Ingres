
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"

/**
** Name:	fmformat.c	This file contains the routine to format a
**				value for a single-line output.
**
** Description:
**	This file defines:
**
** 	fmt_format		Format a value for a single-line output.
**
** History:
**	18-dec-86 (grant)	implemented.
**	12/23/87 (dkh) - Fixed to correctly handle II_NULL_STRING that is
**			 longer than the display area.
**	01/04/89 (dkh) - Changed to call "afe_cvinto()" instead of
**			 "adc_cvinto()".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	fmt_format	- Format a value for a single-line output.
**
** Description:
**	This routine takes a FMT and a value and formats it.  The FMT must
**	produce single line output in order for this routine to work.
**	If the value is null, then the null string will be displayed.
**	If the format is a multi-line format, then the routine IIfmt_init and
**	fmt_multi must be used.
**
** Inputs:
**	cb		Points to the current ADF_CB control block.
**
**	fmt		The FMT with which to format the value.
**
**	value		The value to format.
**
**	display		The place to put the formatted value.
**		.db_datatype	Must be DB_LTXT_TYPE.
**		.db_length	The maximum length of this display value.
**		.db_data	Must point to where to put the display value.
**	
**	change_cntrl	if TRUE, convert control characters to '.'
**
** Outputs:
**	display	
**		.db_data	Will point to the formatted value.
**
**	Returns:
**		E_DB_OK		If it completed successfully.
**		E_DB_WARN	If it completed with a warning.
**		E_DB_ERROR	If some error occurred.
**
**		If it completed with a warning, the field
**		cb->adf_errcb.ad_errcode can contain the following specific
**		warnings:
**
**		E_FM1000_FORMAT_OVERFLOW	The format is not wide enough
**						to accommodate the entire
**						number. The formatted value
**						will be filled in with
**						asterisks.
**
**		If an error occurred, the caller can look in the field
**		cb->adf_errcb.ad_errcode to determine the specific error.
**		The following is a list of possible error codes that can be
**		returned by this routine:
**
**		E_FM6000_NULL_ARGUMENT		One of the pointer parameters
**						to this routine was NULL.
**		E_FM6003_DISPLAY_NOT_LONGTEXT	The datatype of the display
**						parameter is not LONGTEXT.
**		E_FM6004_DISPLAY_LEN_TOO_SHORT	The length of the display
**						parameter is too short for the
**						format width.
**		E_FM600B_FMT_DATATYPE_INVALID	The format is invalid for the
**						datatype given.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**		E_FM6008_NOT_SINGLE_LINE_FMT	The given fmt parameter is not
**						a single-line format.
**		E_FM6002_ZERO_WIDTH		Width of the format is zero.
**		E_FM600A_CORRUPTED_TEMPLATE	The template contains an illegal
**						character that should have been
**						caught by fmt_setfmt.
**
**		Additional errors from ADF routine adc_cvinto can be returned.
**
** History:
**	18-dec-86 (grant)	implemented.
**
**	21-oct-1993 (rdrane)
**		Add DECIMAL support for all numeric formats, including
**		templates.
**	11-sep-2006 (gupsh01)
**		Added support for ANSI date/time types.
*/

STATUS
fmt_format(cb, fmt, value, display, change_cntrl)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*value;
DB_DATA_VALUE	*display;
bool		change_cntrl;
{
    i4		rows;
    i4		cols;
    STATUS	status;
    i4		isnull;
    i4		nullstrlen;
    u_char	*nullstring;
    DB_TEXT_STRING *disp;
    u_char	*text;

    if (fmt == NULL || value == NULL || display == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    if (display->db_datatype != DB_LTXT_TYPE)
    {
	return afe_error(cb, E_FM6003_DISPLAY_NOT_LONGTEXT, 2,
			 sizeof(display->db_datatype), &(display->db_datatype));
    }

    status = fmt_size(cb, fmt, value, &rows, &cols);
    if (status != OK)
    {
	return status;
    }

    if (rows != 1)
    {
	return afe_error(cb, E_FM6008_NOT_SINGLE_LINE_FMT, 6,
			 sizeof(fmt->fmt_type), &(fmt->fmt_type),
			 sizeof(fmt->fmt_width), &(fmt->fmt_width),
			 sizeof(fmt->fmt_prec), &(fmt->fmt_prec));
    }

    if (display->db_length-DB_CNTSIZE < cols)
    {
	return afe_error(cb, E_FM6004_DISPLAY_LEN_TOO_SHORT, 4,
			 sizeof(display->db_length), &(display->db_length),
			 sizeof(cols), &cols);
    }

    status = adc_isnull(cb, value, &isnull);
    if (status != OK)
    {
	return status;
    }
    else if (isnull)
    {
	/* put null string in display */

	disp = (DB_TEXT_STRING *) display->db_data;
	disp->db_t_count = cols;
	text = disp->db_t_text;

	nullstrlen = cb->adf_nullstr.nlst_length;
	if (nullstrlen > cols)
	{
	    MEfill(cols, ' ', (PTR) text);
	    return(OK);
	}
	else
	{
	    nullstring = (u_char *) cb->adf_nullstr.nlst_string;
	}

	switch(fmt->fmt_sign)
	{
	case FS_MINUS:
	    f_strlft(nullstring, nullstrlen, text, cols);
	    break;

	case FS_PLUS:
	    f_strrgt(nullstring, nullstrlen, text, cols);
	    break;

	case FS_STAR:
	    f_strctr(nullstring, nullstrlen, text, cols);
	    break;

	case FS_NONE:
	    switch(abs(value->db_datatype))
	    {
	    case DB_INT_TYPE:
	    case DB_FLT_TYPE:
	    case DB_MNY_TYPE:
	        f_strrgt(nullstring, nullstrlen, text, cols);
		break;
	    
	    default:
	        f_strlft(nullstring, nullstrlen, text, cols);
		break;
	    }
	    break;
	}

	return OK;
    }

    switch(abs(value->db_datatype))
    {
    case DB_INT_TYPE:
    case DB_FLT_TYPE:
    case DB_MNY_TYPE:
    case DB_DEC_TYPE:
	status = f_format(cb, value, fmt, display);
	break;

    case DB_DTE_TYPE:
    case DB_ADTE_TYPE:
    case DB_TMWO_TYPE:
    case DB_TMW_TYPE:
    case DB_TME_TYPE:
    case DB_TSWO_TYPE:
    case DB_TSW_TYPE:
    case DB_TSTMP_TYPE:
    case DB_INDS_TYPE:
    case DB_INYM_TYPE:
	status = f_fmtdate(cb, value, fmt, display);
	if (status == OK)
	{
	    if (fmt->fmt_type != F_DT)
	    {
		/*
		** Cn can be used to display a date.
		** Display contains standard string for date.
		*/
		status = f_string(cb, display, fmt, display, change_cntrl);
	    }
	}
	break;

    case DB_CHR_TYPE:
    case DB_TXT_TYPE:
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
	status = f_string(cb, value, fmt, display, change_cntrl);
	break;

    default:
	status = afe_cvinto(cb, value, display);
	break;
    }

    return status;
}
