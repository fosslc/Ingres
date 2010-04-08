/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxtformat.c	30.1	11/14/84"; */

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
**   R_X_TFORMAT - set a temporary format for an attribute.  This format
**	will be used the next time the attribute is printed, and then
**	erased.
**
**	Parameters:
**		ordinal - ordinal of attribute for this format.
**		fmt - FMT structure to use.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd.
**
**	Side Effects:
**		Updates one of the ATT structures.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		5.0, 5.10.
**
**	History:
**		6/6/81 (ps) - written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_tformat(ordinal,fmt)
ATTRIB	ordinal;
FMT	*fmt;
{
	/* internal declarations */

	register ATT	*attribute;		/* fast ptr to ATT structure */

	/* start of routine */



	attribute = r_gt_att(ordinal);
	attribute->att_tformat = fmt;

	return;
}
