
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

/**
** Name:	fmisvald.c	This file contains the routine that checks
**				the validity of a format string.
**
** Description:
**	This file defines:
**
** 	fmt_isvalid		Check the validity of a format string.
**
** History:
**	18-dec-86 (grant)	implemented.
**	08/16/91 (tom) support for input masking of dates, and string templates
**	06/15/92 (dkh) - Added support for decimal datatype for 6.5.
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	fmt_isvalid	- Check the validity of a format string.
**
** Description:
**	Given a format and a type, this routine checks the validity
**	of the format for the type.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		The format of which to check the validity.
**
**	type		A DB_DATA_VALUE containing the type that the fmt
**			is supposed to be for.
**		.db_datatype	The ADF typeid of the type.
**
**	valid		Points to a bool.
**
** Outputs:
**	valid		Contains TRUE if the format is valid for the datatype.
**			Otherwise, contains FALSE.
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
**      28-jul-89 (cmr) Added support for 2 new formats 'cfe' and 'cje' .
**      20-sep-89 (sylviap) 
**			Added support for a new format 'q'.
**	11-sep-06 (gupsh01)
**			Added support for new ANSI date/time formats.
*/

STATUS
fmt_isvalid(cb, fmt, type, valid)
ADF_CB		*cb;
FMT		*fmt;
DB_DATA_VALUE	*type;
bool		*valid;
{
    if (fmt == NULL || type == NULL || valid == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    switch (fmt->fmt_type)
    {
    case F_B:
	*valid = TRUE;
	break;
    
    case F_I:
    case F_F:
    case F_G:
    case F_E:
    case F_N:
    case F_NT:
	switch (abs(type->db_datatype))
	{
        case DB_INT_TYPE:
        case DB_FLT_TYPE:
        case DB_MNY_TYPE:
	case DB_DEC_TYPE:
	    *valid = TRUE;
	    break;

	default:
	    *valid = FALSE;
	    break;
	}
	break;
    
    case F_ST:
	*valid = (abs(type->db_datatype) == DB_VCH_TYPE);
	break;
    
    case F_DT:
	*valid = (abs(type->db_datatype) == DB_DTE_TYPE);
	break;
    
    case F_ADT:
	*valid = (abs(type->db_datatype) == DB_ADTE_TYPE);
	break;

    case F_TMWO:
	*valid = (abs(type->db_datatype) == DB_TMWO_TYPE);
	break;

    case F_TMW:
	*valid = (abs(type->db_datatype) == DB_TMW_TYPE);
	break;

    case F_TME:
	*valid = (abs(type->db_datatype) == DB_TME_TYPE);
	break;

    case F_TSWO:
	*valid = (abs(type->db_datatype) == DB_TSWO_TYPE);
	break;

    case F_TSW:
	*valid = (abs(type->db_datatype) == DB_TSW_TYPE);
	break;

    case F_TSTMP:
	*valid = (abs(type->db_datatype) == DB_TSTMP_TYPE);
	break;

    case F_INYM:
	*valid = (abs(type->db_datatype) == DB_INYM_TYPE);
	break;

    case F_INDS:
	*valid = (abs(type->db_datatype) == DB_INDS_TYPE);
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
	switch (abs(type->db_datatype))
	{
        case DB_CHR_TYPE:
        case DB_TXT_TYPE:
        case DB_CHA_TYPE:
        case DB_VCH_TYPE:
	    *valid = TRUE;
	    break;

	case DB_DTE_TYPE:
	case DB_ADTE_TYPE:
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	case DB_TSTMP_TYPE:
	case DB_INYM_TYPE:
	case DB_INDS_TYPE:
	    *valid = (fmt->fmt_type == F_C);
	    break;
	
	default:
	    *valid = FALSE;
	    break;
	}
	break;
    
    default:
	return afe_error(cb, E_FM6007_BAD_FORMAT, 2,
			 sizeof(fmt->fmt_type), (PTR) &(fmt->fmt_type));
    }

    return OK;
}
