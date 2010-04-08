/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxspos.c	30.1	11/14/84"; */

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
**   R_X_SPOS - set a new default position for a column.  
**
**	Parameters:
**		ordinal		ordinal of column for this position.
**		position	new position default for this column.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd.
**
**	Side Effects:
**		Updates the att_position field in one of the ATT structures.
**
**	Error Messages:
**		230.
**
**	Trace Flags:
**		5.0, 5.10.
**
**	History:
**		7/16/81 (ps) - written.
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
r_x_spos(ordinal,position)
ATTRIB	ordinal;
i4	position;
{
	/* internal declarations */

	register ATT	*attribute;		/* fast ptr to ATT structure */

	/* start of routine */



	if ((position < 0) || (position > En_lmax))
	{
		r_error(230, NONFATAL, Cact_tname, Cact_attribute, NULL);
		return;
	}

	attribute = r_gt_att(ordinal);
	attribute->att_position = position;


	return;
}
