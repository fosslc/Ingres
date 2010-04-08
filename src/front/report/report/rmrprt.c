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
**   R_M_RPRT - make a default report for the specified data table.
**	The report will be based on the information in the ATT structures,
**	and of the style specified in St_style.	 The values are:
**
**		RS_NULL or
**		RS_DEFAULT - use either RS_COLUMN or RS_BLOCK, depending
**				on which fits.
**		RS_COLUMN - set up a report which spans the page.
**				(First choice default).	 Also sorts
**				on the first column.
**		RS_WRAP - set up an RS_COLUMN report, but wrap at
**			right margin.
**		RS_BLOCK - set up a report where each column has
**			a label next to it, and no column headings.
**			(Second choice default).
**		RS_LABEL - set up a report to print mailing labels.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Trace Flags:
**		50, 51, 52.
**
**	Side Effects:
**		Many.
**
**	Error Messages:
**		SYSERR: Bad style.
**
**	History:
**		2/10/82 (ps)	written.
**		5/27/83 (nml)	Added changes made to VMS code...
**				default width of report is 132 for output
**				file specified with -f	(gac).
**		10/24/83(nml)	Put in gac's changes made on VMS...BUG #1180
**				fixed: -mcolumn wraps at terminal width for RBF.
**		5/18/84 (gac)	bug 1765 fixed -- default width is 132 for file.
**		5/29/84 (gac)	bug 2344 fixed -- -mcolumn now does not wrap.
**		6/29/84 (gac)	bug 2327 fixed -- default line maximum is
**				width for terminal unless -l or -f is specified.
**		1/8/85 (rlm)	Oact's become static arrays, tagged memory
**				allocation
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		12/21/89 (Elein)
**			Fixed rsyserr call
**		1/10/90 (elein)
**			ensure call to rsyserr uses i4 ptr
**		12/2/89  (martym)  Added call to r_m_labels() to implement a 
**				mailing labels sytle report.
**              4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
**		12/09/92 (dkh)
**			Added Choose Columns support for rbf.
**		2-feb-1992 (rdrane)
**			Bypass columns which are unsupported datatypes as well
**			as pre-deleted.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_m_rprt()
{


	/* internal declarations */

	i4		width = 0;		/* full width of report */
	ATTRIB		ordinal;		/* ordinal of attribute */
	register ATT	*att;
	i4		style;			/* style to use */
	i4		temp1, temp2;		/* used to break up max call */
	i4		cols_to_use = 0;

	/* start of routine */



	/* set up for default report */

	style = (St_style==RS_NULL) ? RS_DEFAULT : St_style;
	En_qcount = 0;
	En_acount = 0;

        if ((Cact_rtext = (char *)FEreqmem ((u_i4)Rst5_tag,
		(u_i4)(MAXRTEXT+1), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_rprt"));
	}

        if ((Cact_command = (char *)FEreqmem ((u_i4)Rst5_tag,
		(u_i4)(FE_MAXNAME+1), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_rprt"));
	}
        if ((Cact_type = (char *)FEreqmem ((u_i4)Rst5_tag,
		(u_i4)(FE_MAXNAME+1), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_rprt"));
	}
        if ((Cact_attribute = (char *)FEreqmem ((u_i4)Rst5_tag,
		(u_i4)(FE_MAXNAME+1), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_m_rprt"));
	}

	/* set up default formats, and set the overall width */

	St_left_margin = 0;

	/* if -l or -f specified, use number specified (132 by default)
	** otherwise use width of terminal.
	*/
	if (St_lspec || En_fspec != NULL)
	{
		St_right_margin = En_lmax;
	}
	else
	{
		St_right_margin = En_cols;
	}

	for(width=0,ordinal=En_n_attribs; ordinal>0; ordinal--)
	{
		att = &(Ptr_att_top[ordinal-1]);
		if  ((att->pre_deleted) ||
		     (att->att_value.db_data == (PTR)NULL))
		{
			continue;
		}
		r_fmt_dflt(ordinal);
		temp1 = r_ret_wid(ordinal);
		temp2 = STlength(att->att_name);
		width += max(temp1, temp2);
		cols_to_use++;
	}

	width += ((cols_to_use-1) * SPC_DFLT);


	if (style==RS_DEFAULT)
	{	/* set it to what it really should be */
		style = (width>=St_right_margin) ? RS_BLOCK : RS_COLUMN;
	}


	switch(style)
	{	/* check for legal style, and set the default style */
		case(RS_WRAP):
		case(RS_COLUMN):
			r_m_column();
			break;

		case(RS_BLOCK):
			r_m_block();
			break;

		case(RS_LABELS):
			r_m_labels();
			break;

		default:
			r_syserr(E_RW0033_r_m_rprt_Bad_style,&style);
	}


	return;
}
