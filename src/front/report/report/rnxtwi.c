/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rnxtwi.c	30.1	11/14/84"; */

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
**   R_NXT_WITHIN - set up the margins, etc for the next column
**	in a .WITHIN list of columns.
**
**	Parameters:
**		ordinal		ordinal of column.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_within, r_x_ewithin.
**
**	Side Effects:
**		pushes the environment stack.  Changes the margins.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		150, 154.
**
**	History:
**		1/11/82 (ps)	written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_nxt_within(ordinal)
ATTRIB	ordinal;
{
	/* external declarations */
	ATT		*r_gt_att();

	/* internal declarations */

	register ATT	*att;			/* ATT of ordinal */

	/* start of routine */



	att = r_gt_att(ordinal);

	r_push_env();		/* save the current environment */

	St_left_margin = att->att_position;
	St_right_margin = St_left_margin + r_ret_wid(ordinal);

	if (St_left_margin < 0)
	{
		St_left_margin = 0;
	}

	if (St_right_margin <= St_left_margin)
	{
		St_right_margin = St_left_margin + 5;
	}

	r_x_tab(St_left_margin,B_ABSOLUTE);


	return;
}
