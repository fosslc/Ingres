/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<er.h>
# include	<ug.h>
# include	<rtype.h> 
# include	<rglob.h> 

/*
**   R_M_DSORT - set up the default sort list (ascending order on the first
**		 column which is participating in the report).
**
**		 This routine sets Ptr_sort_top and En_scount to valid values
**		 if successful, and (SORT *)NULL and zero respectively if
**		 unsuccessful.
**
**	Parameters:
**		None.
**
**	Returns:
**		TRUE -	Success.
**		FALSE -	The table in question does not contain any columns
**			which are eligible to participate in the report.
**			The caller should take appropriate action to abort
**			report setup.
**
**	Called by:
**		r_rep_ld, rFget.
**
**	History:
**		3/9/82 (ps)	written.
**		6/28/84	(gac)	fixed bug 2320 -- "detail","page" or "report"
**				cannot be a break column.
**		12/20/84 (rlm)	SRT / SORT structures merged and made dynamic
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		3/20/90 (elein) performance
**			Change STcompare to STequal
**		17-dec-1992 (rdrane)
**			Redeclare the function as returning bool.
**			Bypass columns which are unsupported datatypes or have
**			been pre-deleted (RBF ChooseColumn support).  Thus, we
**			default to sorting on the first "actively participating"
**			column.  Don't set En_s_count in the degenerate case,
**			nor bother allocating the Ptr_sort_top structure.
**			Don't assume that Ptr_sort_top and En_scount are
**			initialized to anything meaningful.
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
r_m_dsort()
{
	ATTRIB	ordinal;
	ATT	*att_ptr;


	Ptr_sort_top = (SORT *)NULL;
	En_scount = 0;

	att_ptr = (ATT *)NULL;
	ordinal = 1;
	while (ordinal <= En_n_attribs)
	{
		att_ptr = r_gt_att(ordinal);
		if  ((att_ptr != (ATT *)NULL) && (!att_ptr->pre_deleted))
		{
			break;
		}
		ordinal++;
	}

	if  ((att_ptr == (ATT *)NULL) || (att_ptr->pre_deleted))
	{
		/*
		** This is the degenerate case of no attributes or
		** all unsupported and/or pre-deleted attributes.
		** ChooseColumns is supposed to preclude this case by
		** not allowing unsupported datatypes to participate
		** and not allowing the user to delete all participating
		** columns.  So, we're really guarding against a table which
		** has no columns which are supported datatypes.
		*/
		return(FALSE);
	}

	if ((Ptr_sort_top = (SORT *)FEreqmem((u_i4)Rst4_tag,
					(u_i4)(sizeof(SORT)),
					TRUE,(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_dsort"));
	}
	En_scount = 1;


	if ((STequal(att_ptr->att_name,NAM_DETAIL)) ||
	    (STequal(att_ptr->att_name,NAM_PAGE)) ||
	    (STequal(att_ptr->att_name,NAM_REPORT)))
	{
		r_m_sort(1,ordinal,ERx("n"),O_ASCEND);
	}
	else
	{
		r_m_sort(1,ordinal,ERx("y"),O_ASCEND);
	}

	return(TRUE);
}
