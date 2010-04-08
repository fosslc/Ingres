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
# include	<cm.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include	<er.h>
# include	<ug.h>

/*
**   R_BRK_SET - set up the BRK structures.
**	One BRK structure is set up for the overall report, one is
**	set up for each sort attribute, one for the page breaks, and
**	one for the detail break.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_ld.
**
**	Side Effects:
**		Sets the Ptr_brk_top pointer.
**		Sets the Ptr_last_brk pointer.
**		Sets the Ptr_det_brk pointer.
**		Sets the Ptr_pag_brk pointer.
**		Set the En_n_breaks field.
**		Will abort on some syserr conditions.
**
**	Error messages:
**		Syserr:bad sort attribute ordinal.
**
**	Trace Flags:
**		1.0, 1.8.
**
**	History:
**		3/15/81 (ps) - written.
**		1/8/85 (rlm) - tagged memory allocation.
**	19-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
**	08/11/89 (elein) garnet
**		added evaluation of $variable for break names
**	12/21/89 (elein)
**		Corrected call to r_syserr
**	1/12/90 (elein)
**		IIUGlbo's for IBM gateways
**		bug 9619: runtime casing required because
**		of copyapp
**	2/7/90 (martym)
**		Coding standards cleanup.
**	4/30/90 (elein)
**		Create TRUE/FALSE return value for nicer messages.
**	8/13/91 (elein) b39069
**		Whoops, need to return TRUE at end.
**	7-oct-1992 (rdrane)
**		Converted r_error() err_num value to hex to facilitate
**		lookup in errw.msg file.  Allow for delimited
**		identifiers and normalize them in the break
**		variable arrays.  Disambiguate failed memory allocation
**		messages.
**	9-dec-1993 (rdrane)
**		Oops!  The sort attribute name is already stored as
**		normalized (b54950).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


bool
r_brk_set()

{
	/* external declarations */
	ATT		*r_gt_att();	/* (mmm) added declaration */
	/* internal declarations */

	ATTRIB		sortatt;	/* ordinal or sort attribute */
	BRK		*o_bstruct;	/* pointer to old break structure */
	BRK		*n_bstruct;	/* pointer to new break structure */
	ATTRIB		ordinal;	/* ordinal of attribute */
	ATT		*att;		/* ATT for the attribute */
	char		**brk_attid;	/* points to array of names of
					** attributes to break on */
	char		*outname;

	/* start of routine */



	/* first set up the BRK structure for overall report */

        if ((o_bstruct = (BRK *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(BRK)), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_brk_set" - 1));
	}

	o_bstruct->brk_attribute = A_REPORT;
	Ptr_brk_top = o_bstruct;
	En_n_breaks = 1;

	if (En_bcount > 0)
	{
		/* .BREAK command specified break attributes */
		/* set up BRK structures for each break attribute */

		for (brk_attid = Ptr_break_top; En_n_breaks <= En_bcount;
		     En_n_breaks++, brk_attid++)
		{
			r_g_set(*brk_attid);
			if( r_g_skip() == TK_DOLLAR )
			{
				CMnext(Tokchar);
				outname = r_g_name();
				if( (*(brk_attid) = r_par_get(outname)) == NULL)
				{
					 r_error(0x3F0,NONFATAL,outname,NULL);
					 return(FALSE);
				}
				/*
				** Normalize any parameter name ...
				*/
				if  (IIUGdlm_ChkdlmBEobject(*brk_attid,
					*brk_attid,FALSE) == UI_BOGUS_ID)
				{
					r_error(0x40A, NONFATAL, *(brk_attid),
						NULL);
				}
			}
			ordinal = r_mtch_att(*brk_attid);
			if ((ordinal > En_n_attribs) || (ordinal < 1))
			{	/* bad attribute ordinal */
				r_error(0x40A, NONFATAL, *(brk_attid), NULL);
				return(FALSE);
			}	/* bad attribute ordinal */

        		if ((n_bstruct = (BRK *)FEreqmem ((u_i4)Rst4_tag,
				(u_i4)(sizeof(BRK)), TRUE,
				(STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("r_brk_set - 2"));
			}
			n_bstruct->brk_attribute = ordinal;
			n_bstruct->brk_above = o_bstruct;
			o_bstruct->brk_below = n_bstruct;
			att = r_gt_att(ordinal);
			att->att_brk_ordinal = En_n_breaks;
			o_bstruct = n_bstruct;
		}
	}
	else
	{
		/* set up BRK structures for each sort attribute */

		for (sortatt = 0; sortatt < En_n_sorted; sortatt++)
		{	/* for each sort attribute */
			ordinal = (Ptr_sort_top[sortatt]).sort_ordinal;
			if ((ordinal > En_n_attribs) || (ordinal < 1))
			{	/* bad attribute ordinal */
				r_error(0x40A, NONFATAL, 
				   (Ptr_sort_top[sortatt]).sort_attid, NULL);
				return(FALSE);
			}	/* bad attribute ordinal */


        		if ((n_bstruct = (BRK *)FEreqmem ((u_i4)Rst4_tag,
				(u_i4)(sizeof(BRK)), TRUE,
				(STATUS *)NULL)) == NULL)
			{
				IIUGbmaBadMemoryAllocation(ERx("r_brk_set - 3"));
			}
			n_bstruct->brk_attribute = ordinal;
			n_bstruct->brk_above = o_bstruct;
			o_bstruct->brk_below = n_bstruct;
			att = r_gt_att(ordinal);
			att->att_brk_ordinal = En_n_breaks;
			En_n_breaks++;
			o_bstruct = n_bstruct;
		}	/* for each sort attribute */
	}

	/* now set up the BRK structure for detail */
	/* use a BRK structure, even though all of the fields aren't needed */

	Ptr_last_brk = o_bstruct;	/* end of BRK linked list */

        if ((Ptr_det_brk = (BRK *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(BRK)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_brk_set - 4"));
	}
	Ptr_det_brk->brk_attribute = A_DETAIL;

	/* now set up the BRK structure for page breaks */

        if ((Ptr_pag_brk = (BRK *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(sizeof(BRK)), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_brk_set - 5"));
	}
	Ptr_pag_brk->brk_attribute = A_PAGE;

	return(TRUE);

}
