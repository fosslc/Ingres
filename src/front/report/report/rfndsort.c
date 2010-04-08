/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfndsort.c	30.1	11/14/84"; */

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
**   R_FND_SORT - find the sort order associated with an attribute ordinal.
**	This returns the ordinal of the break associated with an ordinal
**	or an error code if errors occur.
**
**	The sort orders are:
**		page break - A_BREAK.
**		detail	   - A_DETAIL.
**		report	   - A_REPORT.
**	These will return exactly as sent.
**
**	For others, which will be attributes, return the ordinal of
**	the break, or A_ERROR, for errors. If the attribute number
**	A_GROUP is sent, the current .WITHIN column, if any, is used.
**
**	Parameters:
**		ordinal - attribute ordinal, or A_DETAIL, etc.
**
**	Returns:
**		ordinal of break, A_DETAIL, etc. or A_ERROR.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		2.0, 2.11.
**
**	History:
**		5/10/81 (ps) - written.
**		4/5/90 (elein) performance
**                              Since fetching attribute call needs no range
**                              check use &Ptr_att_top[] directly instead of
**                              calling r_gt_att. We don't expect brk_attribute
**                              to be A_REPORT, A_GROUP, or A_PAGE
*/



ATTRIB
r_fnd_sort(ordinal)
ATTRIB	ordinal;
{
	/* internal declarations */

	ATTRIB		tord;			/* temp ordinal of break */
	ATT		*att;			/* fast ATT ptr */

	/* start of routine */



	tord = A_ERROR;			/* preset to error */

	switch (ordinal)
	{
		case(A_PAGE):
			return(A_PAGE);

		case(A_DETAIL):
			return(A_DETAIL);

		case(A_REPORT):
			return(A_REPORT);

		default:
			if ((ordinal < 0) || (ordinal>En_n_attribs))
			{
				return(A_ERROR);
			}
			att = &(Ptr_att_top[ordinal-1]);
			if (att == NULL)
			{
				return(A_ERROR);
			}
			tord = att->att_brk_ordinal;


			if ((tord > 0) && (tord <= En_n_breaks))
			{
				return(tord);
			}
			return(A_ERROR);
	}

}
