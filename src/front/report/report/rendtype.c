/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rendtype.c	30.1	11/14/84"; */

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
**   R_END_TYPE - routine called when the end of a .header/.footer/.detail
**	block has been found.  This makes sure that all the .WI blocks
**	are ended for now and turns off the St_adjusting flag.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_tcmd_set.
**
**	Side Effects:
**		May pop the stack and set St_cwithin,St_ccoms.
**		Will set St_adjusting.
**
**	Trace Flags:
**		104, 100.
**
**	History:
**		1/8/82 (ps)	written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_end_type()
{
	/* internal declarations */

	static	char tstring[1] = {0};		/* used to fool r_p_ewithin */

	/* start of routine */


	if (St_cwithin!=NULL)
	{	/* have some in the stack.  Get rid of them */

		Cact_tcmd = r_new_tcmd(Cact_tcmd);
		r_g_set(tstring);		/* must be set to null string */

		r_p_ewithin();
	}

	St_adjusting = FALSE;

	return;
}
