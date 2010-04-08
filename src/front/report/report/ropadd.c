/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_OP_ADD - add a TCMD P_AOP structure to the header text for
**	either the detail text (if the attribute being aggregated
**	is not in the sort list) or the break text for the attribute
**	being aggregated.  This structure indicates when the basic
**	aggregating is to be done.  When adding the TCMD structure,
**	first go through the header TCMD linked list, searching for
**	a P_PRINT command referring to this attribute.	If one is
**	found, add the TCMD P_AOP structure immediately after it
**	(for the benefit of the page aggregations and the cumulative
**	aggregations).	If none is found, add at the beginning of the
**	list after any P_NEED, P_NEWLINE, P_NPAGE, or P_ACLEAR
**	TCMD structures.
**
**	Parameters:
**		acc - accumulator structure of aggregation.
**		brk - break BRK structure to add the TCMD to.
**
**	Returns:
**		none.
**
**	Called by:
**		r_lnk_agg.
**
**	Side Effects:
**		Will update the TCMD header list in a BRK structure.
**
**	Error Messages:
**		Syserr:Bad brk or acc ptr.
**		Syserr:Did not add TCMD structure.
**
**	Trace Flags:
**		130, 133.
**
**	History:
**		5/9/81 (ps) - written.
**		5/10/83 (nl) - Put in changes since code was ported
**			       to UNIX.
**			       Change P_CATT references to P_ATT to
**			       fix bug 937.
**		1/19/84 (gac)	changed for printing expressions.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_op_add(acc,brk)
ACC	*acc;
BRK	*brk;
{
	/* internal declarations */

	TCMD	*tcmd;			/* TCMD structure to add */
	register TCMD	*tsearch;	/* used in searching for place of TCMD */
	register TCMD	*otsearch;	/* also used in search */
	ATTRIB	ord;			/* return from r_grp_nxt */

	/* start of routine */



	if ((brk == NULL) || (acc == NULL))
	{
		r_syserr(E_RW0037_r_op_add_Bad_brk_or_a);
	}

	tcmd = r_new_tcmd((TCMD *)NULL);
	tcmd->tcmd_code = P_AOP;
	tcmd->tcmd_val.t_v_acc = acc;

	/* see if any reference is in the structure now to the attribute */

	if ((tsearch=brk->brk_header) == NULL)
	{
		brk->brk_header = tcmd;
		return;
	}

	for(; tsearch != NULL; tsearch = tsearch->tcmd_below)
	{
		switch(tsearch->tcmd_code)
		{
			case(P_WITHIN):
				/* set up .WITHIN loop */
				tsearch = r_wi_begin(tsearch);
				break;

			case(P_END):
				if (tsearch->tcmd_val.t_v_long == P_WITHIN)
				{	/* end of .WITHIN */
					tsearch = r_wi_end();
				}
				break;

			case(P_PRINT):
				if (tsearch->tcmd_val.t_v_pel->pel_item.item_type == I_ATT)
				{
					if (St_cwithin == NULL)
					{	/* not in a .WITHIN loop */
						if (tsearch->tcmd_val.t_v_pel->pel_item.item_val.i_v_att == acc->acc_attribute)
						{	/* matched on the attribute */
							tcmd->tcmd_below = tsearch->tcmd_below;
							tsearch->tcmd_below = tcmd;
							return;
						}
					}
					else
					{	/* in a .WITHIN loop */
						r_grp_set(tsearch->tcmd_val.t_v_pel->pel_item.item_val.i_v_att);
						while ((ord=r_grp_nxt()) > 0)
						{
							if (ord == acc->acc_attribute)
							{	/* found match on the ordinal.	Put after loop */
								tsearch = r_wi_end();
								tcmd->tcmd_below = tsearch->tcmd_below;
								tsearch->tcmd_below = tcmd;
								return;
							}
						}
					}
				}
				break;

			default:
				break;
		}
	}

	/* not in list now.  add to beginning of list, after any
	** paging type commands and after Clear routines.
	*/


	otsearch = NULL;
	for (tsearch = brk->brk_header;;)
	{
		switch(tsearch->tcmd_code)
		{
			case(P_NEED):
			case(P_NEWLINE):
			case(P_NPAGE):
			case(P_ACLEAR):
				/* skip these */
				otsearch = tsearch;	/* save old one */
				if((tsearch = tsearch->tcmd_below) != NULL)
				{
					break;
				}
				/* else pass right through */

			default:
				/* add it before this one */
				tcmd->tcmd_below = tsearch;
				if (otsearch == NULL)
				{
					brk->brk_header = tcmd;
					return;
				}

				otsearch->tcmd_below = tcmd;
				return;
		}
	}
	/* NOTREACHED */
}
