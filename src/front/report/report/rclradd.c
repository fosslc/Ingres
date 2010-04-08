/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rclradd.c	30.1	11/14/84"; */

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
**   R_CLR_ADD - add a TCMD P_ACLEAR structure to the footer text for
**	the break which is aggregating this aggregate.
**	Also add to the header for the report, to start this off.
**	This should be added to the end of the footer text for
**	the break, in case any cumulatives are used in this text.
**
**	Parameters:
**		acc - address of ACC structure to clear.  This also
**			contains all needed information.
**
**	Returns:
**		none.
**
**	Called by:
**		r_lnk_agg.
**
**	Side Effects:
**		Will update the TCMD structures in a BRK structure.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		130, 134.
**
**	History:
**		5/18/81 (ps) - written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_clr_add(acc)
register ACC	*acc;
{
	/* internal declarations */

	register TCMD	*tcmd;			/* TCMD structure to add */
	register BRK	*brk;			/* break to add this TCMD to */
	register TCMD	*tcmd2;			/* used in adding to end of list */

	/* start of routine */




	/* first add to header text for report */

	brk = Ptr_brk_top;		/* first add to start of report text */

	tcmd = r_new_tcmd((TCMD *)NULL);
	tcmd->tcmd_code = P_ACLEAR;
	tcmd->tcmd_val.t_v_acc = acc;

	/* add to start of header text for this break */

	if (brk->brk_header != NULL)
	{
		tcmd->tcmd_below = brk->brk_header;
	}

	brk->brk_header = tcmd;

	/* now add to end of footer text for this break */

	if (acc->acc_break == A_REPORT)
	{
		return;
	}

	brk = r_brk_find(acc);

	tcmd = r_new_tcmd((TCMD *)NULL);
	tcmd->tcmd_code = P_ACLEAR;
	tcmd->tcmd_val.t_v_acc = acc;

	if (brk->brk_footer == NULL)
	{
		brk->brk_footer = tcmd;
		return;
	}

	for(tcmd2=brk->brk_footer;tcmd2->tcmd_below != NULL;tcmd2=tcmd2->tcmd_below);

	/* add to end of list */

	tcmd2->tcmd_below = tcmd;

	return;
}
