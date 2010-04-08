
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<fmt.h>
# include	"format.h"
# include	<cm.h>

/**
** Name:	fmdspinf.c	Contains the routine fmt_dispinfo for displaying
**				the format for the VIFRED and RBF layout.
**
** Description:
**	This file defines:
**
** 	fmt_dispinfo	Returns display information about the format for
**			the VIFRED and RBF layout.
**
** History:
**	18-mar-87 (grant)	created.
**	6-jun-87 (danielt) changed 0 to (DB_DATA_VALUE *)NULL as
**		parameter to fmt_size()
**	06/05/92 (dkh) - Added support for edit masks for character based
**			 toolset.
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
** Name:	fmt_dispinfo	Returns display information about the
**				format for the VIFRED and RBF layout.
**
** Description:
**	This routine returns display information about the format
**	for the VIFRED and RBF layout. For example, VIFRED displays
**	the E10.2 format as "E__.__E___", the "$$$,$$$.nn" format is
**	displayed as "$__,___.__", and the d"02/03/01" format is displayed
**	as "d_/__/__".
**	If the format has no length (e.g. c0), an empty string is returned.
**
** Inputs:
**	cb		ADF control block.
**	fmt		FMT structure.
**	fillchar	a character to place between the important punctuation
**			and the lead character (e.g. '_').
**	dispstr		points to a string in which to place the output.
**
** Outputs:
**	dispstr		points to the display information string.
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
**
**
** History:
**	18-mar-87 (grant)	implemented.
**	20-apr-89 (cmr)
**			Added support for new format (cu) which is
**			used internally by unloaddb().  It is similar to
**			cf (break at words when printing strings that span 
**			lines) but it breaks on quoted strings as well as
**			words.
**      28-jul-89 (cmr) Added support for 2 new formats 'cfe' and 'cje'.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		make sure decimal point location is within 'dispstr' buffer
**		before writting '.' character, else don't do it.
*/

STATUS
fmt_dispinfo(cb, fmt, fillchar, dispstr)
ADF_CB	*cb;
FMT	*fmt;
char	fillchar;
char	*dispstr;
{
    STATUS	status;
    i4		rows;
    i4		cols;
    char	*t;
    i4		len;

    if (fmt == NULL || dispstr == NULL)
    {
	return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
    }

    status = fmt_size(cb, fmt, (DB_DATA_VALUE *)NULL, &rows, &cols);
    if (status != OK)
    {
	return status;
    }

    *(dispstr+cols) = EOS;

    if (cols > 0)
    {
        MEfill((u_i2)cols, fillchar, (PTR)dispstr);
    }
    else
    {
	return OK;
    }

    switch(fmt->fmt_type)
    {
    case F_I:
	*dispstr = fmt->fmt_var.fmt_char;
	break;

    case F_F:
    case F_E:
    case F_G:
    case F_N:
	if (fmt->fmt_type == F_E)
	{
	    cols -= 4;
	    *(dispstr + cols) = fmt->fmt_var.fmt_char;
	}
	if (cols > fmt->fmt_prec)				 
		*(dispstr + cols - fmt->fmt_prec - 1) = '.';	 
	*dispstr = fmt->fmt_var.fmt_char;
	break;

    case F_NT:
	t = fmt->fmt_var.fmt_template;
	if (*t == FC_ESCAPE)
	{
	    t++;
	}

	/* use first character of template as leading character
	** unless it's a blank */

	if (CMspace(t))
	{
	    CMnext(t);
	    CMnext(dispstr);
	}
	else
	{
	    CMcpyinc(t, dispstr);
	}

	while (*t)
	{
	    switch(*t)
	    {
	    case FC_DECIMAL:
	    case FC_SEPARATOR:
		*dispstr++ = *t++;
		break;

	    case FC_ESCAPE:
		t++;
		if (CMspace(t))
		{
		    CMnext(t);
		    CMnext(dispstr);
		}
		else
		{
		    CMcpyinc(t, dispstr);
		}
		break;

	    default:
		/* skip it */

		dispstr++;
		t++;
		break;
	    }
	}
	break;

    case F_DT:
	/* determine length of template without escapes and '|' */

	for (t = fmt->fmt_var.fmt_template, len = 0; *t != EOS;)
	{
	    if (*t == '|')
	    {
		t++;
		continue;
	    }

	    if (*t == '\\')
	    {
		t++;
	    }

	    CMbyteinc(len, t);
	    CMnext(t);
	}

	*dispstr++ = 'd';

	/* skip first character in template */

	t = fmt->fmt_var.fmt_template;
	if (*t == '\\')
	{
	    t++;
	}

	if (CMbytecnt(t) > 1)
	{
	    dispstr++;
	}

	CMnext(t);

	/* only put out punctuation if template is not variable length */

	if (fmt->fmt_width == len)
	{
	    while (*t != EOS)
	    {
		if (*t == '|')
		{
		    t++;
		    continue;
		}

		if (*t == '\\')
		{
		    t++;
		}

		if (CMalpha(t) || CMdigit(t) || CMspace(t))
		{
		    /* skip alphanumerics and spaces */

		    CMnext(t);
		    CMnext(dispstr);
		}
		else	/* use punctuation as I/O mask */
		{
		    CMcpyinc(t, dispstr);
		}
	    }
	}
	break;

    case F_C:
    case F_CF:
    case F_CFE:
    case F_CJ:
    case F_CJE:
    /* internal format used by unloaddb() */
    case F_CU:
	*dispstr = 'c';
	break;

    case F_T:
	*dispstr = 't';
	break;

    case F_ST:
	*dispstr = 's';
	break;

    default:
	*dispstr = '?';
	break;
    }

    return OK;
}
