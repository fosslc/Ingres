/*
**	Copyright (c) 2004 Ingres Corporation
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
# include	<dateadt.h>
# include	"format.h"

/**
** Name:	fmfmtsz.c	This file contains the routine which returns
**				the display size needs for a format.
**
** Description:
**	This file defines:
**
** 	fmt_size		Get the display size needs for a format.
**
** History:
**	18-dec-86 (grant)	implemented.
**
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
** Name:	fmt_size	- Get the display size needs for a format.
**
** Description:
**	This routine computes the display requirements for a particular format.
**	The display requirements are returned as the number
**	of rows and columns a formatted value would take.  For some formats,
**	the number of rows cannot be computed (e.g. c0.100 gives unknown
**	number of lines 100 chars wide). In this case, it returns 0.
**	For c0 the number of columns is the length of the given data type (if
**	it is specified).
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		The format for which to compute the display size.
**
**	type		A DB_DATA_VALUE containing the type that will be
**			displayed with the format.
**
**		.db_datatype	The ADF datatype id for the type.
**
**		.db_length	The length of the type.
**
**			This parameter may be NULL. If it is and the format
**			has no width (e.g. c0 or t0), then the number of
**			columns returned is zero. WARNING: This parameter
**			should not be NULL if you are going to use the
**			columns returned to allocate space for the display
**			value.
**
**	rows		Must point to a nat.
**
**	columns		Must point to a nat.
**
** Outputs:
**	rows		Will contains the number of rows a formatted value
**			could take.
**
**	columns		Will contain the number of columns a formatted value
**			could take.
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
**		E_FM600B_FMT_DATATYPE_INVALID	The format is invalid for the
**						datatype given.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**
** History:
**	18-dec-86 (grant)	implemented.
**	20-apr-89 (cmr)
**			Added support for new format (cu) which is
**			used internally by unloaddb().  It is similar to
**			cf (break at words when printing strings that span 
**			lines) but it breaks on quoted strings as well as
**			words.
**	28-jul-89 (cmr)	Added support for 2 new formats 'cfe' and 'cje'.
**	20-sep-89 (sylviap)	
**		Added support for Q format.
**	08/16/91 (tom) support for input masking of dates, and string templates
**	09/11/06 (gupsh01) Added ANSI date/time support.
*/

STATUS
fmt_size(cb, fmt, type, rows, columns)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*type;
i4		*rows;
i4		*columns;
{
    STATUS	status;
    i4		width;
    i4		prec;
    bool	valid;
    i4		length;

    if (fmt == NULL || rows == NULL || columns == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    if (type != NULL)
    {
        status = fmt_isvalid(cb, fmt, type, &valid);
        if (status != OK)
	{
	    return status;
	}

	if (!valid)
	{
	    return afe_error(cb, E_FM600B_FMT_DATATYPE_INVALID, 4,
			     sizeof(fmt->fmt_type), (PTR)&(fmt->fmt_type),
			     sizeof(type->db_datatype),
			     (PTR)&(type->db_datatype));
	}
    }

    width = fmt->fmt_width;

    switch (fmt->fmt_type)
    {
    case F_B:
    case F_I:
    case F_F:
    case F_G:
    case F_E:
    case F_N:
    case F_NT:
    case F_DT:
    case F_ST:
	*columns = width;
	*rows = 1;
	break;
    
    case F_Q:	
	/* 
	** Length is zero since RW does not leave space for a control sequence
	** to be printed.
	*/
	*columns = 0;
	*rows = 1;
	break;

    case F_T:
    case F_C:
    case F_CF:
    case F_CFE:
    case F_CJ:
    case F_CJE:
    /* internal format used by unloaddb() */
    case F_CU:
        prec = fmt->fmt_prec;
	if (prec == 0)
	{
	    *columns = width;
	    *rows = 1;
	}
	else if (width == 0)
	{
	    *columns = prec;
	    *rows = 0;
	}
	else
	{
	    *columns = prec;
	    *rows = (width + prec - 1) / prec;
	}

	if (*columns == 0 && type != NULL)	/* C0 or T0 */
	{
	    length = type->db_length;
	    if (AFE_NULLABLE_MACRO(type->db_datatype))
	    {
		length--;
	    }

	    switch(abs(type->db_datatype))
	    {
	    case DB_CHR_TYPE:
	    case DB_CHA_TYPE:
		*columns = length;
		break;

	    case DB_DTE_TYPE:
		*columns = DATEOUTLENGTH;
		break;

	    case DB_ADTE_TYPE:
		*columns = ANSIDATE_OUTLENGTH;
		break;

	    case DB_TMWO_TYPE:
		*columns = ANSITMWO_OUTLENGTH;
		break;

	    case DB_TMW_TYPE:
		*columns = ANSITMW_OUTLENGTH;
		break;

	    case DB_TME_TYPE:
		*columns = ANSITME_OUTLENGTH;
		break;

	    case DB_TSWO_TYPE:
		*columns = ANSITSWO_OUTLENGTH;
		break;

	    case DB_TSW_TYPE:
		*columns = ANSITSW_OUTLENGTH;
		break;

	    case DB_TSTMP_TYPE:
		*columns = ANSITSTMP_OUTLENGTH;
		break;

	    case DB_INDS_TYPE:
		*columns = ANSIINDS_OUTLENGTH;
		break;

	    case DB_INYM_TYPE:
		*columns = ANSIINYM_OUTLENGTH;
		break;

	    default:
		status = afe_dplen(cb, type, &length);
		if (status != OK)
		{
		    return status;
		}

		/* fall through */

	    case DB_TXT_TYPE:
	    case DB_VCH_TYPE:
		*columns = length - DB_CNTSIZE;
		break;
	    }

	    if (fmt->fmt_type == F_T)
	    {
		*columns *= 4;	/* \ddd for non-printable characters */
	    }
	}
	break;
    }

    return OK;
}
