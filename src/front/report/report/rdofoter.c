/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rdofooter.c	30.1	11/14/84"; */

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
**   R_DO_FOOTER - follow the TCMD structures for the footer text for
**	each break less major than or equal to the current break.
**	Footers are processed from the least major up to the current
**	break.
**
**	Parameters:
**		brk - the current break structure.
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
r_do_footer(brk)
BRK	*brk;

{

	/* internal declarations */

	BRK	*tbrk;				/* temp break pointer */

	/* start of routine */




	for (tbrk=Ptr_last_brk; brk != NULL; tbrk=tbrk->brk_above)
	{
		r_x_tcmd(tbrk->brk_footer);

		if (tbrk == brk)
		{	/* finished. get out. */

			break;
		}	/* finished. get out. */
	}

	return;
}
