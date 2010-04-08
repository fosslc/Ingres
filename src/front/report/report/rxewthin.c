/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxewithin.c	30.1	11/14/84"; */

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
**   R_X_EWITHIN - end a .WITHIN block.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_end.
**
**	Side Effects:
**		Sets St_cwithin, may set St_ccoms, Cact_tcmd.
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
r_x_ewithin()
{
	/* start of routine */



	St_cwithin = St_cwithin->tcmd_below;

	r_x_tab(St_right_margin,B_ABSOLUTE);		/* position at end of column */

	r_pop_env();

	if (St_cwithin->tcmd_code == P_WITHIN)
	{	/* another column to process */
		r_nxt_within((ATTRIB)St_cwithin->tcmd_val.t_v_long);
		Cact_tcmd = St_ccoms;
	}
	else
	{	/* last column finished */
		r_pop_be(P_WITHIN);
		St_cwithin = NULL;
		St_ccoms = NULL;
	}


	return;
}
