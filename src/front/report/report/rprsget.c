/*
** Copyright (c) 2004 Ingres Corporation
*/
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
# include	<ug.h>
# include	<er.h>

/*
**   R_PRS_GET - either find an existing PRS structure, or
**	create a new one, filling in the fields.
**	This is used to find a matching PRS structure if one exists.
**	This allows the multiple use of the same structures, which
**	is used when comparing ACC structures to see if they are
**	the same.  Actually, we could simply create new PRS
**	structures whenever an aggregate with a preset is found,
**	but that would mean that a separate ACC would be set up
**	for each of the aggregates, which would require duplicate
**	calculations.  This routine simply conserves these PRS structs.
**
**	Parameters:
**		type	type of preset (PT_NCONSTANT, PT_ATTRIBUTE,
**					or PT_DCONSTANT).
**		value	value of preset.
**
**	Returns:
**		address of PRS structure, either existing or new.
**		Do not return NULL ptr.
**
**	Called by:
**		r_p_agg.
**
**	Side Effects:
**		May update Ptr_prs_top on first one.
**
**	Error messages:
**		Syserr: Bad type.
**
**	Trace Flags:
**		3.0, 3.11.
**
**	History:
**		1/21/82 (ps)	written.
**		2/8/84	(gac)	added aggregates of dates.
**		1/8/85  (rlm)	tagged memory allocation.
**		01-nov-88 (sylviap)
**			If PT_CONSTANT, checks if datatypes are the same.  If
**			not, then fall through case statement and allocate 
**			memory. (#3804)
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/



PRS	*
r_prs_get(type,value)
i2	type;
PTR	value;
{
	/* internal declarations */

	register PRS	*prs = NULL;		/* fast ptr to PRS structure */
	PRS		*fetch;			/* address needed for alloc */
	PRS		*lprs = NULL;		/* last in the list */ 
	STATUS		status;
	i4		cmp_result;
	DB_DATA_VALUE	*tvalue;		/* temporary value ptr */
	DB_DATA_VALUE	*tprs;			/* temporary prs ptr   */

	/* start of routine */

	for(prs=Ptr_prs_top; prs!=NULL; prs=prs->prs_below)
	{	/* see if it matches */
		lprs = prs;
		if (prs->prs_type == type)
		{
			switch(type)
			{
			case(PT_CONSTANT):
				/*
				**  Check if datatypes are the same.  If not, 
				**  then fall through and allocate space.
				**  adc_compare MUST have the same datatypes.
				**  # 3804
				*/
				tprs = &(prs->prs_pval.pval_constant);
				tvalue = (DB_DATA_VALUE *) value;
				if (tvalue->db_datatype != tprs->db_datatype)
				{
					break;
				}
				status = adc_compare(Adf_scb,
				     &(prs->prs_pval.pval_constant),
				     (DB_DATA_VALUE *)value,
				     &cmp_result);
				if (status != OK)
				{
					FEafeerr(Adf_scb);
				}

				if (cmp_result == 0)
				{
					goto gotit;
				}
				break;

			case(PT_ATTRIBUTE):
				if (prs->prs_pval.pval_ordinal ==
				    *(i4 *) value)
				{
					goto gotit;
				}
				break;
			}
		}
	}

	/* if we made it here, we didn't find.  Create one */

        if ((fetch = (PRS *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(PRS)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_prs_get"));
	}
							 

	prs = fetch;
	prs->prs_type = type;

	switch(type)
	{
	case(PT_CONSTANT):
		MEcopy((PTR)value, (u_i2)sizeof(DB_DATA_VALUE),
		       (PTR)&(prs->prs_pval.pval_constant));
		break;

	case(PT_ATTRIBUTE):
		prs->prs_pval.pval_ordinal = *((i4 *)value);
		break;
	}

	if (Ptr_prs_top == NULL)
	{
		Ptr_prs_top = prs;
	}
	else
	{
		lprs->prs_below = prs;
	}

    gotit:
	/* All come here to exit */

	return(prs);
}
