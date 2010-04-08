
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
** Name:	fmkind.c	This file contains the FMT routine fmt_type.
**
** Description:
**	This file defines:
**
** 	fmt_kind	Returns whether a format is a fixed-length,
**			a variable-length, or a B format.
**
** History:
**	10-feb-87 (grant)	created.
**	08/16/91 (tom) support for input masking of dates, and string templates
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
** Name:	fmt_kind	Returns whether a format is a fixed-length,
**				a variable-length, or a B format.
**
** Description:
**	This procedure returns whether a given format is fixed length (e.g.
**	C50 or F10), variable length (e.g. C0 or CJ0.100), or a B format.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		Points to a legal FMT holding a format.
**
**	fmtkind		Must point to a nat.
**
** Outputs:
**	fmtkind		Will contain one of the following values:
**				FM_FIXED_LENGTH_FORMAT
**				FM_VARIABLE_LENGTH_FORMAT
**				FM_B_FORMAT
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
**		E_FM6007_BAD_FORMAT		The format type does not exist.
**
** History:
**	10-feb-87 (grant)	implemented.
**	20-apr-89 (cmr)
**			Added support for new format (cu) which is
**			used internally by unloaddb().  It is similar to
**			cf (break at words when printing strings that span 
**			lines) but it breaks on quoted strings as well as
**			words.
**      28-jul-89 (cmr) Added support for 2 new formats 'cfe' and 'cje' .
**      20-sep-89 (sylviap) 
**		Added support for a new format 'q'.
*/

STATUS
fmt_kind(cb, fmt, fmtkind)
ADF_CB	*cb;
FMT	*fmt;
i4	*fmtkind;
{
    if (fmt == NULL || fmtkind == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    switch(fmt->fmt_type)
    {
    case F_B:
	*fmtkind = FM_B_FORMAT;
	break;

    case F_I:
    case F_F:
    case F_G:
    case F_E:
    case F_N:
    case F_NT:
    case F_DT:
    case F_ST:
	*fmtkind = FM_FIXED_LENGTH_FORMAT;
	break;

    case F_C:
    case F_T:
    case F_CF:
    case F_CFE:
    case F_CJ:
    case F_CJE:
    /* internal format used by unloaddb() */
    case F_CU:
	if (fmt->fmt_width == 0)

	{
	    *fmtkind = FM_VARIABLE_LENGTH_FORMAT;
	}
	else
	{
	    *fmtkind = FM_FIXED_LENGTH_FORMAT;
	}
	break;

    case F_Q:
	*fmtkind = FM_Q_FORMAT;
	break;

    default:
	return afe_error(cb, E_FM6007_BAD_FORMAT, 2,
			 sizeof(fmt->fmt_type), (PTR) &(fmt->fmt_type));
    }

    return OK;
}
