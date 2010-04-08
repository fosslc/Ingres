/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rdoheader.c	30.1	11/14/84"; */

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
**   R_DO_HEADER - follow the TCMD structure linked list for the
**	header text for all breaks from the current break down to
**	the last break, in that order.
**
**	Parameters:
**		brk - the current brk structure.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		4.0, 4.3.
**
**	History:
**		4/5/81 (ps) - written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_do_header(brk)
BRK	*brk;

{
	/* start of routine */



	while (brk != NULL)
	{
		r_x_tcmd(brk->brk_header);
		brk = brk->brk_below;
	}

	return;

}
