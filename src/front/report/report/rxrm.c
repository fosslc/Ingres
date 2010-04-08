/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxrm.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_X_RM - change the right margin for the report.
**		If this is specified in the page header or footer, set
**		the right margin for the page breaks only.
**
**	Parameters:
**		column	new right margin.
**		type	type of change--absolute,relative,default.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update St_right_margin.
**
**	Called by:
**		r_x_tcmd.
**
**	Error messages:
**		206, 207, 211.
**
**	Trace Flags:
**		5.0, 5.5.
**
**	History:
**		4/15/81 (ps) - written.
**		6/6/84	(gac)	fixed bug 1607 -- checks that .LM # < .RM #.
**		2/8/90 (elein) B9965: Boy it took us a long time to find
**			this one.  Check .LM != RM, too.  Change adjustment
**			to +5 becuase rxlm adds 5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_rm(column,type)
register i4	column;
register i2	type;
{
	/* start of routine */



	switch(type)
	{
		case(B_ABSOLUTE):
			St_right_margin = column;
			break;

		case(B_RELATIVE):
			St_right_margin += column;
			break;

		case(B_DEFAULT):
			St_right_margin = RM_DFLT;

	}

	if (St_right_margin <= 0)
	{
		r_error(206, NONFATAL, Cact_tname, Cact_attribute, NULL);
		St_right_margin = RM_DFLT;
	}
	else
	{
		if (St_right_margin > En_lmax)
		{
			r_error(207, NONFATAL, Cact_tname, Cact_attribute, NULL);
			St_right_margin = En_lmax;
		}
	}

	if (St_right_margin <= St_left_margin)
	{	/* this won't do at all */
		if (St_lm_set)
		{
			r_error(211, NONFATAL, Cact_tname, Cact_attribute, NULL);
		}

		if (St_left_margin + 5 > En_lmax)
			St_right_margin = En_lmax;
		else
			St_right_margin = St_left_margin + 5 ;
	}

	/* now set the permanent margin flags to the current ones */

	if (St_in_pbreak)
	{	/* change page break margin */
		St_p_rm = St_right_margin;
	}
	else
	{
		St_r_rm = St_right_margin;
	}



	St_rm_set = TRUE;

	return;
}
