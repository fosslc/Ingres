
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

/**
** Name:	fmsetfmt.c	This file contains the routine to set up a
**				format structure.
**
** Description:
**	This file defines:
**
**	fmt_setfmt		Set up a format structure.
**
** History:
**	20-dec-86 (grant)	implemented.
**	9/8/89 (elein) garnet
**		Add single quote recognition
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
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
** Name:	fmt_setfmt	-	Set up a format structure.
**
** Description:
**	Given a format string, checks it for legality, and builds
**	a format structure if it is legal.  Any dynamic memory associated
**	with the FMT structure must be allocated prior to the call and passed
**	in through the area parameter.
**
** Inputs:
**	cb		Points to the current ADF_CB
**			control block.
**
**	fmtstr		The format string for which to build a FMT structure.
**
**	area		Allocated area used to build the FMT structure.
**
**	fmt		Must point to a pointer to an FMT structure.
**
**	fmtlen		An optional pointer to a i4  to contain
**			the length of the scanned format string.
**
** Outputs:
**	fmt		Will point to an allocated FMT structure that
**			corresponds to the format string.
**
**	fmtlen		If this is not NULL, the i4  to which it points will
**			contain the length of the format string actually
**			scanned.
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
**
**		The following are specific user errors given in response to
**		their format strings:
**
**		E_FM2000_EXPECTED_CHAR_INSTEAD	Different character was expected
**						than that given.
**		E_FM2001_EXPECTED_NUMBER	Number was expected next.
**		E_FM2006_BAD_CHAR		Illegal character was found.
**		E_FM2007_BAD_FORMAT_CHAR	Illegal character was specified
**						for the format type.
**		E_FM2008_NO_ESCAPED_CHAR	No character follows the escape
**						(i.e. backslash) character.
**		E_FM2009_CHAR_SHOULD_FOLLOW	Specific character must follow
**						character.
**		E_FM200A_FORMAT_TOO_LONG	Width of template format exceeds
**						maximum allowed.
**		E_FM200B_COMP_ALREADY_SPEC	Date component has been
**						specified more than once.
**		E_FM200C_ABS_COMP_SPEC_FOR_INT	Absolute date component cannot
**						be specified in date interval
**						format.
**		E_FM200D_WRONG_COMP_VALUE_SPEC	Wrong value was specified for
**						date component.
**		E_FM200E_WRONG_ORD_VALUE	Wrong ordinal suffix follows a
**						given digit.
**		E_FM200F_ORD_MUST_FOLLOW_NUM	Ordinal suffix doesn't follow
**						a number.
**		E_FM2010_PM_AND_24_HR_SPEC	Before/after noon designation
**						(using 'p') and 24-hour time
**						(using '16' for the hour instead
**						of '4') cannot be specified
**						together.
**		E_FM2011_I_FMT_CANT_HAVE_PREC	Integer format (I) cannot have
**						numeric precision.
**		E_FM2012_PREC_GTR_THAN_WIDTH	For numeric format, the
**						precision cannot be greater than
**						the width.
**		E_FM2014_JUSTIF_ALREADY_SPEC	The justification character ('-'
**						or '+') has been specified more
**						than once.
**		E_FM2015_NO_DATE_COMP_SPEC	No date component was specified.
**
** History:
**	20-dec-86 (grant)	implemented.
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
*/

STATUS
fmt_setfmt(cb, fmtstr, area, fmt, fmtlen)
ADF_CB		*cb;
char		*fmtstr;
PTR		area;
FMT		**fmt;
i4		*fmtlen;
{
    if (fmtstr == NULL || area == NULL || fmt == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    /* set up format pointer to previously allocated memory area */

    *fmt = (FMT *) area;
    if( (STindex(fmtstr, ERx("\""), 0) != NULL) ||
        (STindex(fmtstr, ERx("\'"), 0) != NULL) )
    {
	/* fmt has a template -- set up its pointer */

	(*fmt)->fmt_var.fmt_template = (char *) area + sizeof(FMT);
    }
    else
    {
	(*fmt)->fmt_var.fmt_template = NULL;
    }

    /* now actually set the format structure */

    return f_setfmt(cb, *fmt, fmtstr, fmtlen);
}
