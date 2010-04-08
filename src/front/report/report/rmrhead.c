/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	 <er.h> 

/*
**   R_M_RHEAD - set up the report header setup commands, which
**	are the nonprinting commands at the start of the report,
**	which are like .FORMAT, .POSITION, and .WIDTH.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_m_column, r_m_wrap.
**
**	Trace Flags:
**		50, 57.
**
**	History:
**		3/30/82 (ps)	written.
**		12/2/89 (martym)   Added support for labels style reports.
**              4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
**	16-aug-90 (sylviap)
**		Took out the 'BEGIN BLOCK' for Label reports.
**	19-nov-1992 (rdrane)
**		Ensure att_name unnormalized before generating RW statements.
**		Ensure unsupported datatypes are screened out/ignored.
**	12/09/92 (dkh) - Added Choose Columns support for rbf.
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
r_m_rhead()
{
	/* internal declarations */

	register ATT	*att;		/* ATT structure for column */
	register i4	i;		/* counter */
	char		ibuf[20];	/* holds number for itoa */
	char		ibuf2[20];	/* holds number for itoa */
	char		fmtstr[256];	/* format string */
	char		tmp_buf[(FE_UNRML_MAXNAME + 1)];

	/* start of routine */


	r_m_action(ERx("begin"), ERx("rbfsetup"), NULL);

	/*
	** FIXME:
	** Include when ready to implement labels styles in 
	** the ReportWriter:
		**
	if (St_style == RS_LABELS)
	{
		CVna((i4)En_lmax, ibuf);
		r_m_action(ERx("rm"), ibuf, NULL);

		CVna(LabelInfo. LineCnt, ibuf);
		s_w_action(ERx("let"),ERx(LABST_LINECNT),ERx("="),ibuf,NULL);
		r_m_action(ERx("endlet"), NULL);

		CVna(LabelInfo. MaxFldSiz, ibuf);
		s_w_action(ERx("let"),ERx(LABST_MAXFLDSIZ),ERx("="),ibuf,NULL);
		r_m_action(ERx("endlet"), NULL);

		CVna(LabelInfo. MaxPerLin, ibuf);
		s_w_action(ERx("let"),ERx(LABST_MAXPERLIN),ERx("="),ibuf,NULL);
		r_m_action(ERx("endlet"), NULL);
	}

	*/

	/* Put in the format and position command for each column */

	for (i=1; i<=En_n_attribs; i++)
	{
		att = &(Ptr_att_top[i-1]);
		/*
		** Screen unsupported datatypes
		*/
		if  ((att->att_value.db_data == (PTR)NULL) ||
			att->pre_deleted)
		{
			continue;
		}
		fmt_fmtstr(Adf_scb, att->att_format, fmtstr);
		_VOID_ IIUGxri_id(att->att_name,&tmp_buf[0]);
		r_m_action(ERx("format"), &tmp_buf[0], ERx("("), 
			fmtstr, ERx(")"), NULL);
		CVna((i4)att->att_position,ibuf);
		CVna((i4)att->att_width,ibuf2);
		r_m_action(ERx("position"), &tmp_buf[0], ERx("("), 
			ibuf, ERx(", "), ibuf2, ERx(")"), NULL);
	}
	r_m_action(ERx("end"), ERx("rbfsetup"), NULL);

	return;

}
