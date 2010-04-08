/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxpl.c	30.1	11/14/84"; */

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
**   R_X_PL - change the page length for the report.
**	A page length of zero means to suppress any pagination.
**
**	Parameters:
**		nlines - number of lines per page to use for report.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Update St_p_length.
**
**	Called by:
**		r_x_tcmd.
**
**	Error messages:
**		203.
**
**	Trace Flags:
**		5.0, 5.5.
**
**	History:
**		4/15/81 (ps)	written.
**		1/5/81 (ps)	modified to allow PL of 0.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       1-Apr-2009 (hanal04) Bug 121840
**          When St_pl_set is true we should not change the value of
**          St_p_length. However, the user value may not be enough for
**          the hdr + ftr + 1 (or more) rows. As En_lc_top and En_lc_bottom
**          are not established until r_set_dflt() has been called this
**          routine has been modified to check and update St_p_length
**          if called with the current value of St_p_length and it is found to
**          be too small.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_pl(nlines)
register i4	nlines;
{
	/* start of routine */



	if ((!St_pl_set) || (St_p_length == nlines))
	{
		St_p_length = nlines;

		if ((nlines!=0) && (St_p_length<=(En_lc_bottom+En_lc_top)))
		{
			r_error(203, NONFATAL, Cact_tname, Cact_attribute, NULL);
			St_p_length = En_lc_bottom+En_lc_top+1;
		}
	}


	return;
}
