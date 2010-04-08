/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxswidth.c	30.1	11/14/84"; */

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
**   R_X_SWIDTH - set a new default width for a column.  
**
**	Parameters:
**		ordinal		ordinal of column for this width.
**		width		new width default for this column.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd.
**
**	Side Effects:
**		Updates the att_width field in one of the ATT structures.
**
**	Error Messages:
**		231.
**
**	Trace Flags:
**		5.0, 5.10.
**
**	History:
**		2/10/82 (ps) - written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_swidth(ordinal,width)
ATTRIB	ordinal;
i2	width;
{
	/* internal declarations */

	register ATT	*attribute;		/* fast ptr to ATT structure */

	/* start of routine */



	if ((width < 1) || (width > En_lmax))
	{
		r_error(231, NONFATAL, Cact_tname, Cact_attribute, NULL);
		return;
	}

	attribute = r_gt_att(ordinal);
	attribute->att_width = width;


	return;
}
