
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
** Name:	fmisrvsd.c	This file contains the routine that checks
**				if a format string specifies right-to-left
**				display.
**
** Description:
**	This file defines:
**
** 	fmt_isreversed		Check the reversed status of a format string.
**
** History:
**	27-mar-87 (bab)	implemented.
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	fmt_isreversed	- Check the reversed status of a format string.
**
** Description:
**	Given a format, this routine checks if the format indicates
**	right-to-left input and display.
**
** Inputs:
**	cb		Points to the current ADF_CB 
**			control block.
**
**	fmt		The format of which to check the reversed status.
**
**	reverse		Points to a bool.
**
** Outputs:
**	reverse		Contains TRUE if the format is reversed.
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
**
** History:
**	27-mar-87 (bab)	implemented.
**	20-apr-89 (cmr)
**			Added support for new format (cu) which is
**			used internally by unloaddb().  It is similar to
**			cf (break at words when printing strings that span 
**			lines) but it breaks on quoted strings as well as
**			words.
**      28-jul-89 (cmr) Added support for 2 new formats 'cfe' and 'cje'.
*/

STATUS
fmt_isreversed(cb, fmt, reverse)
ADF_CB		*cb;
FMT		*fmt;
bool		*reverse;
{
    if (fmt == NULL || reverse == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    *reverse = FALSE;

    switch (fmt->fmt_type)
    {
    case F_C:
    case F_CF:
    case F_CFE:
    case F_CJ:
    case F_CJE:
    /* internal format used by unloaddb() */
    case F_CU:
	if (fmt->fmt_var.fmt_reverse)
		*reverse = TRUE;
	break;
    }

    return OK;
}
