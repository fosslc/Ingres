/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxneed.c	30.1	11/14/84"; */

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
**   R_X_NEED - check for a specified number of lines on the current
**	page, or else do a page eject.
**
**	Parameters:
**		nlines - number of lines to check for.  If this number of
**			lines is not available on the current page, write
**			out a page eject.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none, directly.
**
**	Called by:
**		r_x_tcmd.
**
**	Trace Flags:
**		5.0, 5.7.
**
**	History:
**		4/20/81 (ps) - written.
**		11/19/83 (gac)	changed St_l_number to St_l_number+1.
**		6/12/84	(gac)	fixed bug 2328 -- ff instead of newlines.
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
r_x_need(nlines)
register i4	nlines;
{
	/* start of routine */

	if ((St_p_length<=0) || (St_in_pbreak))
	{	/* never do a page break when length = 0 */
		return;
	}

	if ((St_l_number-1+En_lc_bottom+nlines) > St_p_length)
	{
		r_x_npage(1, B_RELATIVE);		/* assure page eject */
	}

	return;

}
