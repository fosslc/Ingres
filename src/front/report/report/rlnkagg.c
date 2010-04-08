/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rlnkagg.c	30.1	11/14/84"; */

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
**   R_LNK_AGG - called after all the RTEXT has been processed to run
**	through the ACC structures in the ATT structures
**	to set up the TCMD structures
**	This makes for very efficient processing
**	of aggregates.  TCMD structures indicating that an operation
**	is to be done are added in the text for detail or for a 
**	break, if the aggregated attribute is a break.  TCMD structures
**	indicating that the accumulator is to be cleared out are added
**	to the header text for the break attribute (or page).
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_load.
**
**	Side Effects:
**		Adds TCMD structures to the header text of some of the
**		breaks (which are aggregating other attributes).
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		130, 132, 141.
**
**	History:
**		5/15/81 (ps) - written.
**              4/5/90 (elein) performance
**                              Since fetching attribute call needs no range
**                              check use &Ptr_att_top[] directly instead of
**                              calling r_gt_att. We don't expect brk_attribute
**                              to be A_REPORT, A_GROUP, or A_PAGE
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_lnk_agg()
{
	/* internal declarations */

	ATTRIB		natt = 1;		/* counter of att number */
	register ATT	*att;			/* ptr to attribute struct */
	register BRK	*brk;			/* ptr to break structure */
	register ACC	*acc;			/* ptr to ACC being processed */
	register LAC	*lac;			/* ptr to list of ACC */

	/* start of routine */


	/* process the attribute list, one by one */

	for (natt=1; natt<=En_n_attribs; ++natt )
	{
		/* if in sort list, aggregate only on breaks in value.
		** if not in sort list, aggregate in detail break. */

		att = &(Ptr_att_top[natt-1]);
		for(brk=Ptr_brk_top; brk!=NULL; brk=brk->brk_below)
		{	/* if in list, save the break */
			if (natt == brk->brk_attribute)
			{	/* attribute is this break */
				break;
			}
		}

		if (brk == NULL)
		{
			brk = Ptr_det_brk;
		}

		for (lac = att->att_lac; lac != NULL; lac = lac->lac_next)
		{
		    for (acc = lac->lac_acc; acc != NULL; acc = acc->acc_below)
		    {
			if (acc->acc_above == NULL)
			{   /* no pointer up.  do full aggregations */
			    if (acc->acc_unique)
			    {	/* unique aggregate */
				r_op_add(acc,brk);
			    }
			    else
			    {
				r_op_add(acc,Ptr_det_brk);
			    }
			}
			r_clr_add(acc);
		    }
		}
	}


	return;

}
