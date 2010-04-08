
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
# include	"format.h"
# include	<st.h>
# include	<dateadt.h>

/**
** Name:	fmftot.c	This file contains the routine that creates
**				a default type for a given format.
**
** Description:
**	This file defines:
**
** 	fmt_ftot		Create a default type given a format.
**
** History:
**	17-dec-86 (grant)	implemented.
**	05/03/89 (dkh) - Zero'ed out type->db_prec.
**	08/16/91 (tom) support for input masking of dates, and string templates
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	fmt_ftot	- Create a default type given a format.
**
** Description:
**	This routine creates a default ADF type given an FMT format.
**	The DB_DATA_VALUE that is created contains this default type,
**	and can be used in other ADF routines.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		Points to a legal FMT holding a format.
**
**	type		Must point to an allocated DB_DATA_VALUE.
**
** Outputs:
**	type		Will contain the default type and length
**			information for the format given.
**
**		.db_datatype	The ADF datatype id for the type.
**
**		.db_length	The length of the type.
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
**		E_FM6001_NO_TEMPLATE		For a numeric or date template
**						format, the fmt parameter does
**						not have the required template.
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**		E_FM600C_B_FMT_IMPLIES_NO_TYPE	The B format may only be used
**						in a Report Writer
**						specification and implies no
**						datatype.
**
** History:
**	17-dec-86 (grant)	implemented.
**	12-may-88 (sylviap)
**		Added money format.  If DG then data type is float (F8).
**	29-sep-88 (sylviap)
**		Took out if DG and money format, then datatype is float.
**		DG will support money datatype, so do not need to call it a 
**		float.
**	20-apr-89 (cmr)
**		Added support for new format (cu) which is used internally 
**		by unloaddb().  It is similar to cf (break at words when 
**	   	printing strings that span lines) but it breaks on quoted 
**		strings as well as words.
**      28-jul-89 (cmr) Added support for 2 new formats 'cfe' and 'cje.
**      20-sep-89 (sylviap) 
**		Added support for a new format 'q'.
**	11-sep-06 (gupsh01)
**		Added support for ANSI date/time types.
*/

STATUS
fmt_ftot(cb, fmt, type)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*type;
{
    if (fmt == NULL || type == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    switch (fmt->fmt_type)
    {
    case F_B:
	return afe_error(cb, E_FM600C_B_FMT_IMPLIES_NO_TYPE, 0);
	break;

    case F_I:
	type->db_datatype = DB_INT_TYPE;
	type->db_length = sizeof(i4);
	break;

    case F_F:
    case F_E:
    case F_G:
    case F_N:
	type->db_datatype = DB_FLT_TYPE;
	type->db_length = sizeof(f8);
	break;

    case F_NT:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	if (STindex(fmt->fmt_var.fmt_template, FCP_CURRENCY, 0) != NULL)
	{   
	    type->db_datatype = DB_MNY_TYPE;
	    type->db_length = sizeof(f8);
	}
	else if (fmt->fmt_prec > 0 || 
		 STindex(fmt->fmt_var.fmt_template, FCP_DECIMAL, 0) != NULL)
	{    /* precision or decimal point implies float */
	    type->db_datatype = DB_FLT_TYPE;
	    type->db_length = sizeof(f8);
	}
	else /* integer */
	{
	    type->db_datatype = DB_INT_TYPE;
	    type->db_length = sizeof(i4);
	}
	break;

    case F_ST:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }
	type->db_datatype = DB_VCH_TYPE;
	type->db_length = fmt->fmt_width + DB_CNTSIZE;
	break;

    case F_DT:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_DTE_TYPE;
	type->db_length = DATE_LENGTH;
	break;

    case F_ADT:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_ADTE_TYPE;
	type->db_length = ADATE_LENGTH;
	break;

    case F_TMWO:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_TMWO_TYPE;
	type->db_length = ATIME_LENGTH;
	break;

    case F_TMW:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_TMW_TYPE;
	type->db_length = ATIME_LENGTH;
	break;

    case F_TME:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_TME_TYPE;
	type->db_length = ATIME_LENGTH;
	break;

    case F_TSWO:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_TSWO_TYPE;
	type->db_length = ATSTMP_LENGTH;
	break;

    case F_TSW:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_TSW_TYPE;
	type->db_length = ATSTMP_LENGTH;
	break;

    case F_TSTMP:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_TSTMP_TYPE;
	type->db_length = ATSTMP_LENGTH;
	break;

    case F_INYM:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_INYM_TYPE;
	type->db_length = AINTYM_LENGTH;
	break;

    case F_INDS:
        if (fmt->fmt_var.fmt_template == NULL)
        {
	    return afe_error(cb, E_FM6001_NO_TEMPLATE, 0);
        }

	type->db_datatype = DB_INDS_TYPE;
	type->db_length = AINTDS_LENGTH;
	break;

    case F_T:
    case F_C:
    case F_CF:
    case F_CFE:
    case F_CJ:
    case F_CJE:
    /* internal format used by unloaddb() */
    case F_CU:
    case F_Q:
	type->db_datatype = DB_VCH_TYPE;

	if (fmt->fmt_width == 0)
	{
	    type->db_length = DB_MAXSTRING;
	}
	else
	{
	    type->db_length = fmt->fmt_width;
	}

	type->db_length += DB_CNTSIZE;	/* for text type byte count */
	break;

    default:
	return afe_error(cb, E_FM6007_BAD_FORMAT, 2,
			 sizeof(fmt->fmt_type), (PTR)&(fmt->fmt_type));
	break;
    }

    /*
    **  For now, set the precision value to zero
    **  so the DB_DATA_VALUE is completely correct.
    **  This needs to change if there is a format
    **  that has a default datatype of PACKED DECIMAL.
    */
    type->db_prec = 0;

    if (cb->adf_qlang == DB_SQL)
    {
	AFE_MKNULL_MACRO(type);
    }

    return OK;
}
