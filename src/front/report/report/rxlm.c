/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxlm.c	30.1	11/14/84"; */

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
**   R_X_LM - change the left margin for the report.
**		If this is specified in the page break, change the left
**		margin for the page headers and footers.
**		Also set current output position to new LM value.  
**
**	Parameters:
**		column	new left margin.
**		type	whether absolute, default, or relative change.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update St_left_margin.
**		Updates either St_p_lm or St_r_lm.
**
**	Called by:
**		r_x_tcmd.
**
**	Error messages:
**		201, 202, 210.
**
**	Trace Flags:
**		5.0, 5.5.
**
**	History:
**		4/15/81 (ps) - written.
**		6/6/84	(gac)	fixed bug 1607 -- checks that .LM # < .RM #.
**              2/8/90 (elein) B9965: Boy it took us a long time to find
**                      this one.  Check .LM != RM, too.  And ensure adjustment
**			is in range and sticks.
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
r_x_lm(column,type)
register i4	column;
register i2	type;
{
	/* start of routine */



	switch(type)
	{
		case(B_ABSOLUTE):
			St_left_margin = column;
			break;

		case(B_RELATIVE):
			St_left_margin += column;
			break;

		case(B_DEFAULT):
			St_left_margin = LM_DFLT;

	}

	if (St_left_margin < 0)
	{
		r_error(201, NONFATAL, Cact_tname, Cact_attribute, NULL);
		St_left_margin = 0;
	}
	else
	{
		if (St_left_margin > (En_lmax-5))
		{
			r_error(202, NONFATAL, Cact_tname, Cact_attribute, NULL);
			St_left_margin = LM_DFLT;
		}
	}

	if (St_left_margin >= St_right_margin)
	{	/* this would cause problems */
		if (St_rm_set)
		{
			r_error(210, NONFATAL, Cact_tname, Cact_attribute, NULL);
		}	
                if (St_left_margin + 5 > En_lmax)
                        St_right_margin = En_lmax;
                else
                        St_right_margin = St_left_margin + 5;

		if (St_in_pbreak)
		{	/* change the page break margin */
			St_p_rm = St_right_margin;
		}
		else
		{
			St_r_rm = St_right_margin;
		}
	}

	/* now set the permanent margin flags to the current ones */

	if (St_in_pbreak)
	{	/* change the page break margin */
		St_p_lm = St_left_margin;
	}
	else
	{
		St_r_lm = St_left_margin;
	}


	r_x_tab(St_left_margin,B_ABSOLUTE);

	St_lm_set = TRUE;

	return;
}
