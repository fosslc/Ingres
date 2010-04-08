/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfindbrk.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_FIND_BRK - check for differences in value between the current
**	data tuple and the previous data tuple on the break variables
**	to see if a break has occurred in the report.
**
**	Parameters:
**		none.
**
**	Returns:
**		Ptr to break structure of most major break attribute.
**		Returns NULL if no break in attributes with associated
**			BRK structures.
**
**	Side Effects:
**		Sets the St_brk_ordinal variable.
**
**	Trace Flags:
**		4.0, 4.2.
**
**	History:
**		4/5/81 (ps) - written.
**		2/1/84 (gac)	added date type.
**		7/26/85 (gac)	break on formatted dates.
**		4/5/90 (elein) performance
**				Since fetching attribute call needs no range
**				check use &Ptr_att_top[] directly instead of
**				calling r_gt_att. We don't expect brk_attribute
**				to be A_REPORT, A_GROUP, or A_PAGE
**		4/8/90 (elein)
**				whoops, off by one on above change
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



BRK	*
r_find_brk()
{


	/* internal declarations */

	register ATT	*attribute;		/* fast ptr to ATT struct associated with break */
	register BRK	*brk;			/* fast ptr to found BRK structure */
	FMT		*fmt;
	i4		cmp_result;
	STATUS		status;
	i4		fmtkind;

	/* start of routine */


	/* start checking at the second BRK structure, because the first
	** is always for the overall report.
	*/

	for (brk=Ptr_brk_top->brk_below; brk!=NULL; brk=brk->brk_below)
	{
	    attribute = &(Ptr_att_top[brk->brk_attribute -1]);


	    if (attribute->att_dformatted)
	    {
		/*
		** compare the current and previously formatted
		** display values.
		*/

		cmp_result = MEcmp((PTR)(attribute->att_cdisp),
				   (PTR)(attribute->att_pdisp),
				   (u_i2)(attribute->att_dispwidth));
	    }
	    else
	    {
		/* compare the current and previous internal value */

		status = adc_compare(Adf_scb, &(attribute->att_value),
				     &(attribute->att_prev_val), &cmp_result);
	        if (status != OK)
	        {
		    FEafeerr(Adf_scb);
	        }
	    }

	    if (cmp_result != 0)
	    {
		St_brk_ordinal = attribute->att_brk_ordinal;


		Cact_attribute = attribute->att_name;
		return(brk);
	    }
	}

	/* no breaks found on attributes with BRK structures */

	St_brk_ordinal = 0;


	return(NULL);

}
