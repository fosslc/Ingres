/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rnxtacc.c	30.1	11/14/84"; */

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
**   R_NXT_ACC - find the next ACC structure in the linked lists
**	of ACC's (see the LAC structure for the start) for one
**	attribute/primitive pair.  ACC's are set up in doubly
**	linked lists (parent and child ptrs) when the aggregates are
**	leveled for efficiency of leveled aggregation.  However,
**	no parent ptr is given for any ACC structure that must be
**	fully aggregated, which means that the actual aggregations
**	are applied at each new tuple from the data array.  This
**	will be true for the first ACC in the linked list of a
**	leveled aggregate, any ACC being aggregated for each page,
**	and any preset aggregate.  This routine also breaks the
**	downward link of any page aggregate and any aggregate with
*:	a preset value.
**
**	Parameters:
**		acc - ptr to current ACC structure.
**
**	Returns:
**		acc - ptr to next ACC structure (child of the current one),
**			or NULL, if at end of list.
**
**	Called by:
**		r_lnk_agg.
**
**	Side Effects:
**		breaks the link in the ACC list to the page or preset ACC, if there.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		4.0, 4.8.
**
**	History:
**		5/10/81 (ps) - written.
*/



ACC	*
r_nxt_acc(acc)
register ACC	*acc;
{
	/* internal declarations */

	register ACC	*acc2;		/* temp holder of acc address */
	/* start of routine */



	if ((acc2 = acc->acc_below) != NULL)
	{
		/* if page or preset ACC, break links */

		if (acc2->acc_above == NULL)
		{
			acc->acc_below = NULL;
		}
	}


	return(acc2);
}
