/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include	<er.h>
# include	<ug.h>

/*
**   R_M_SORT - put a sort column into the SRT array, which
**	must first be allocated to contain En_scount elements.
**	Later, r_srt_set will be called to process these elements.
**
**	Parameters:
**		sortorder	order for this sort column.
**		ordinal		ordinal of sort attribute.
**		isbreak		"y" if break column, "n" if not.
**		order		sort order, either O_ASCEND or O_DESCEND.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Fills in an element of the SRT array.
**
**	Error Messages:
**		Syserr: Bad sort ordinal.
**		Syserr: Bad attribute ordinal.
**
**	Trace Flags:
**		50, 55.
**
**	History:
**		2/20/82 (ps)	written.
**		12/20/84 (rlm)	SORT/SRT structures merged and made dynamic
**		16-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		12/21/89 (elein)
**			Fixed call to rsyserr
**		1/10/90 (elein)
**			Ensure call to rsyserr uses i4 ptr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Mar-2005 (lakvi01)
**          Properly prototype function.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_m_sort(sortorder,ordinal,isbreak,order)
i2	sortorder;
ATTRIB	ordinal;
char	*isbreak;
char	*order;
{
	/* external declarations */
	ATT		*r_gt_att();		/* (mmm) added declaration */

	/* internal declarations */

	register SORT	*srt;			/* SORT structure for this sort
						** order */
	register ATT	*att;			/* ATT struct for ordinal */
	i4		l_so;
	i4		l_ord;

	/* start of routine */



	if ((sortorder<=0) || (sortorder>En_scount))
	{
		l_so = sortorder;
		r_syserr(E_RW0034_Bad_sort_order,&l_so);
	}

	srt = &(Ptr_sort_top[sortorder-1]);

	if ((att=r_gt_att(ordinal)) == NULL)
	{
		l_ord = ordinal; 
		r_syserr(E_RW0035_r_m_sort_Bad_attribut,&l_ord);
	}

        if ((srt->sort_attid = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(STlength(att->att_name)+1), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_sort"));
	}
									 

        if ((srt->sort_break = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(STlength(isbreak)+1), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_sort"));
	}
        if ((srt->sort_direct = (char *)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(STlength(order)+1), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_sort"));
	}

	STcopy(att->att_name, srt->sort_attid);
	STcopy(isbreak, srt->sort_break);
	STcopy(order, srt->sort_direct);

	return;
}
