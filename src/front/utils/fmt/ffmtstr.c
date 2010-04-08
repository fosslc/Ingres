/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<bt.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	"format.h"
# include	<cm.h>
# include	<st.h>

/**
** Name:	ffmtstr.c	This file contains the FMT routine fmt_fmtstr.
**
** Description:
**	This file defines:
**
**	fmt_fmtstr	Returns a format string given a FMT structure.
**
** History:
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	fmt_fmtstr	Returns a format string given a FMT structure.
**
** Description:
**	This procedure returns a format string corresponding to a given
**	FMT structure.
**
** Inputs:
**	cb		Pointer to the control block.
**
**	fmt		Pointer to a FMT structure previously initialized
**			by a call to fmt_setfmt.
**
**	str		Points to a buffer large enough to hold the format
**			string.
** Outputs:
**	str		Will contain the format string.
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
** History:
**		2/16/82 (ps)	written.
**		11/6/82 (ps)	add ref to fmt_sign.
**		7/26/83 (gac)	add date template.
**		5/10/84 (gac)	fixed bug 2076 -- backslash in numeric
**				template now works.
**		1/20/85 (rlm)	used STprintf's instead of successive STcats
**				and CV calls.
**		2/11/87 (gac)	changed to fmt_fmtstr. got rid of string
**				allocation.
**		3/27/87 (bab)	return cr, cfr, cjr when character format
**				is reversed (right-to-left).
**		20-apr-89 (cmr)	Added support for new format (cu) which is
**				used internally by unloaddb().  It is 
**				similar to cf (break at words when printing 
**			    	strings that span lines) but it breaks on 
**				quoted strings as well as words.
**		28-jul-89 (cmr)	Added support for 2 new formats 'cfe' and 'cje'.
**		20-sep-89 (sylviap)	
**			Added support for a new format 'q'.
**		6-may-93 (rdrane)	
**			Added support for input mask format 's'.
*/

STATUS
fmt_fmtstr(cb, fmt, str)
ADF_CB	*cb;
FMT	*fmt;
char	*str;
{
	/* internal declarations */

	char		name[5];	/* format name string */
	char		*sign;		/* sign of format */

	/* start of routine */

	if (fmt == NULL || str == NULL)
	{
		return afe_error(cb, E_FM6000_NULL_ARGUMENT, 0);
	}

	switch(fmt->fmt_sign)
	{
		case(FS_PLUS):
			sign = ERx("+");
			break;

		case(FS_MINUS):
			sign = ERx("-");
			break;

		case(FS_STAR):
			sign = ERx("*");
			break;

		default:
			sign = ERx("");
			break;
	}

	name[0] = EOS;

	switch(fmt->fmt_type)
	{
		case(F_T):
			STcopy(ERx("t"), name);
			break;

		case(F_C):
			if (!fmt->fmt_var.fmt_reverse)
				STcopy(ERx("c"), name);
			else
				STcopy(ERx("cr"), name);
			break;

		case(F_CF):
			if (!fmt->fmt_var.fmt_reverse)
				STcopy(ERx("cf"), name);
			else
				STcopy(ERx("cfr"), name);
			break;

		case(F_CFE):
			if (!fmt->fmt_var.fmt_reverse)
				STcopy(ERx("cfe"), name);
			else
				STcopy(ERx("cfer"), name);
			break;

		case(F_CJ):
			if (!fmt->fmt_var.fmt_reverse)
				STcopy(ERx("cj"), name);
			else
				STcopy(ERx("cjr"), name);
			break;

		case(F_CJE):
			if (!fmt->fmt_var.fmt_reverse)
				STcopy(ERx("cje"), name);
			else
				STcopy(ERx("cjer"), name);
			break;

		/* internal format used by unloaddb() */
		case(F_CU):
			if (!fmt->fmt_var.fmt_reverse)
				STcopy(ERx("cu"), name);
			else
				STcopy(ERx("cur"), name);
			break;
		
		case(F_B):
			STcopy(ERx("b"), name);
			break;

		case(F_Q):
			STcopy(ERx("q"), name);
			break;

		case(F_I):
		case(F_G):
		case(F_F):
		case(F_N):
		case(F_E):
			STprintf(name, ERx("%c"), fmt->fmt_var.fmt_char);
			break;

		case(F_NT):
			STprintf(str, ERx("%s\"%s\""), sign,
				 fmt->fmt_var.fmt_template);
			for ( ; *str != EOS; CMnext(str))
			{
				if (*str == FC_ESCAPE)
				{
					*str = '\\';
				}
			}
			break;

		case(F_ST):
			STcopy(ERx("s"), name);
			STprintf(str, ERx("%s%s\"%s\""), sign, name,
				 fmt->fmt_var.fmt_template);
			name[0] = EOS;
			break;
#ifdef DATEFMT
		case(F_DT):
			STcopy(ERx("d"), name);
			if (BTtest(FD_INT, (char *)&(fmt->fmt_prec)))
			{
				STcat(name, ERx("i"));
			}

			STprintf(str, ERx("%s%s\"%s\""), sign, name,
				 fmt->fmt_var.fmt_template);
			name[0] = EOS;
			break;
#endif

		default:
			return afe_error(cb, E_FM6007_BAD_FORMAT, 2,
					 sizeof(fmt->fmt_type),
					 (PTR) &(fmt->fmt_type));
	}

	/*
	**	name was used to defer a bunch of similar STprintf's
	*/

	if (STlength(name) > 0)
	{
		if (fmt->fmt_prec > 0)
			STprintf(str, ERx("%s%s%d.%d"), sign, name,
				 fmt->fmt_width, fmt->fmt_prec);
		else
			STprintf(str, ERx("%s%s%d"), sign, name, fmt->fmt_width);
	}

	return OK;
}
