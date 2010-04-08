/*
**	 Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)rfaggset.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h> 
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFAGGSET - read in the .FOOTER sections of the report specifications
**	to find out what aggregates have been set up. 
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		rFdisplay.
**
**	Side Effects:
**		updates Agg_count.
**
**	History:
**		10/24/82 (ps)	written.
**		24-nov-1987 (rdesmond)
**			renamed to rFagset from rFag_set to resolve name
**			conflict with rFag_sel for MVS.
**              9/22/89 (elein) UG changes
**                      ingresug change #90045
**			changed <rbftype.h> and <rbfglob.h> to
**			"rbftype.h" and "rbfglob.h"
**		22-nov-89 (cmr)	
**			this routine counts the number of aggregates for
**			this report and that's it.  It has been simplified
**			now that we support visible editable aggregates.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
rFagset()
{
	/* internal declarations */

	BRK		*brk;		/* break struct */

	/* start of routine */

	for (brk=Ptr_brk_top; brk!=NULL; brk=brk->brk_below)
	{	/* this includes the report header as well */
		rFag_scn(brk->brk_footer);
	}

	rFag_scn(Ptr_pag_brk->brk_footer);

	return;
}



/*
**   RFAGG_SCN - scan a TCMD list in a .FOOTER section
**	to find out what aggregates have been set up.  Since the user
**	has no options in specifying the format of the aggregates, this
**	simply scans for the aggregate printing TCMD structures and 
**	ignores everything else.
**
**	Parameters:
**		tcmd	start of linked list of TCMD's.
**
**	Returns:
**		none.
**
**	Called by:
**		rFag_scn.
**
**	Side Effects:
**		none.
**
**	History:
**		10/24/82 (ps)	written.
**		2/27/84 (gac)	added printing of expressions.
**		2/4/87 (rld)	eliminate case for I_AVG.
*/

VOID
rFag_scn(tcmd)
register TCMD	*tcmd;
{
	/* internal declarations */

	ACC		*acc;		/* if aggregate struct found */

	/* start of routine */

	for (; tcmd!=NULL; tcmd=tcmd->tcmd_below)
	{
		if ( tcmd->tcmd_code == P_PRINT && 
		    (tcmd->tcmd_val.t_v_pel->pel_item.item_type == I_ACC
		    || tcmd->tcmd_val.t_v_pel->pel_item.item_type == I_CUM) )
		{
			Agg_count++;
		}
	}

	return;
}

