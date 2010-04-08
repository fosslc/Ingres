/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<afe.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>

/*
**   R_M_COLUMN - set up a COLUMN default report.  Note that the
**	right margin may be moved further to the right to make room
**	for all the columns.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_m_rprt.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		50, 56.
**
**	History:
**		2/10/82 (ps)	written.
**		2/1/84 (gac)	added date type.
**		6/28/84	(gac)	fixed bug 2320 -- "detail","page" or "report"
**				cannot be a break column.
**		2/13/90 (martym)
**			This module used to presume that there would always 
**			be one sort column and it would always be the first 
**			column. With the introduction of JoinDef as a source 
**			of data in RBF this assumption is not true any longer. 
**			Changed to take into account all sort columns.
**				
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		26-mar-90 (sylviap)
**			Checks if the column goes beyond St_right_margin (>) 
**			not >=. #724
**			Sets St_wrap if report is wrapping. (#396, #709)
**		02-apr-90 (sylviap)
**			Took out St_wrap.  When r_m_column is called from
**			RBF, will build  a tabular report w/o wrapping.  RBF
**			will prompt user to continue w/wide report or select
**			another style.
**              4/5/90 (elein) performance
**                     Since fetching attribute call needs no range
**                     check use &Ptr_att_top[] directly instead of
**                     calling r_gt_att. We don't expect brk_attribute
**                     to be A_REPORT, A_GROUP, or A_PAGE
**		12/09/92 (dkh)
**			Added Choose Columns support for rbf.
**		15-jan-1993 (rdrane)
**			Generate the attribute name as unnormalized.
**			Remove declaration of r_gt_att() since already declared
**			in included hdr files.
**		2-feb-1992 (rdrane)
**			Bypass columns which are unsupported datatypes as well
**			as pre-deleted.
**		11-mar-1993 (rdrane)
**			Generate the attribute name as unnormalized - we missed
**			format/tformat last time around.  This also means that
**			the attribute name must be loaded into Cact_attribute
**			as unnormalized, or r_nxt_set() will fail it further on
**			down the line.
**		24-jan-1994 (rdrane)
**			Missed this one in the massive b54950 change.
**			r_nxt_set() no longer wants Cact_attribute to
**			be unnormalized.
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
r_m_column()
{
	/* internal declarations */

	register ATT	*att;		/* ATT structure for column */
	register ATTRIB	i;		/* counter */
	i4		curx=0;		/* current position */
	i4		cury=1;		/* current line number */
	i2		width=0;	/* width of a column */
	i4		offset=0;	/* offset from left margin */
	i4		maxx=0;		/* actual maximum margin */
	char		ibuf[50];	/* holds ascii numbers from itoa */
	i4		spacing;	/* spacing to next column */
	bool		gotc0;		/* found at least one c0 column */
	char		fmtstr[256];	/* format string */
	i4		rows;
	i4		cols;
	DB_DATA_VALUE	float_type;
	bool		isnumber;
	SORT		*srt;
	i4		seq = 0;
	char		tmp_buf1[(FE_UNRML_MAXNAME + 1)];

	/* start of routine */

	float_type.db_datatype = DB_FLT_TYPE;

	/* First set up the detail lines */


	Cact_attribute = NAM_DETAIL;
	Cact_type = B_HEADER;

	curx = St_left_margin;
	cury = 1;
	offset = 3;
	spacing = 0;

	/* First fill in the default locations etc. for each column */

	for(i=1; i<=En_n_attribs; i++)
	{	/* place the next attribute */
		att = &(Ptr_att_top[i-1]);
		if  ((att->pre_deleted) ||
		     (att->att_value.db_data == (PTR)NULL))
		{
			continue;
		}
		width = max(r_ret_wid(i),STlength(att->att_name));
		att->att_width = width;

		/* 
		** Only wrap report if a wrap style report or if -l is set from
		** the RW.  Don't wrap RBF tabular reports.
		*/
		if (((St_style == RS_WRAP)||
		    (St_lspec && En_program == PRG_REPORT)) &&
		    (curx+width+spacing) > St_right_margin)
		{
			cury++;
			maxx=max(maxx,curx);
			curx = St_left_margin + offset;
			/* fix for bug 5177
			** if someone is making a default wrap mode
			** report for a table with an ungodly number
			** of attributes, stop indenting the next line at
			** the point when the offset is greater that
			** the right margin.
			*/

			if ((St_left_margin + offset) <= St_right_margin - 3)
			{
				offset += 3;
			}
		}
		else
		{	/* fits on this line */
			if (spacing > 0)
			{
				curx += spacing;
			}
		}
		spacing = SPC_DFLT;

		att->att_position = curx;
		att->att_line = cury;
		curx += width;

	}

	maxx = max(maxx,curx);
	St_right_margin = maxx;
	/* 
	** If in column/tabular mode and in rbf, then always reset En_lmax.
	** Also reset En_lmax when called from RW and -l flag is not set.
	*/
	if ((St_style == RS_COLUMN || St_style == RS_TABULAR) &&
		(!St_lspec || En_program == PRG_RBF))
	{
		En_lmax = maxx;
	}


	if (cury > 1)
	{	/* keep on one page */
		CVna(cury,ibuf);
		r_m_action(ERx("need"), ibuf, NULL);
	}

	/* Now put out the commands for a wrapped detail line */

	r_m_action(ERx("block"), NULL);

	gotc0 = FALSE;
	cury = 1;
	for (i=1; i<= En_n_attribs; i++)
	{	/* print out the next attribute */
		att = &(Ptr_att_top[i-1]);
		if  ((att->pre_deleted) ||
		     (att->att_value.db_data == (PTR)NULL))
		{
			continue;
		}
		width = r_ret_wid(i);
		if (att->att_line > cury)
		{	/* go to a new line */
			r_m_action(ERx("end"), ERx("block"), NULL);
			r_m_action(ERx("block"), NULL);
			cury = att->att_line;
		}
		_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
		r_m_action(ERx("within"), &tmp_buf1[0], NULL);
		r_m_action(ERx("top"), NULL);
		fmt_size(Adf_scb, att->att_format, NULL, &rows, &cols);
		afe_cancoerce(Adf_scb, &(att->att_value), &float_type,
			      &isnumber);
		if (isnumber)
		{
			/* always right justify */
			if (cols < width)
			{	/* must tab over a little */
				CVna((i4)(width-cols), ibuf);
				r_m_action(ERx("tab"), ERx("+"), ibuf, NULL);
			}
		}
		else
		{
			/* always left justify */
			if (rows == 0)
			{	/* got a folding format */
				gotc0 = TRUE;
			}
		}

		_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
		r_m_action(ERx("print"), &tmp_buf1[0], NULL);
		r_m_action(ERx("endprint"), NULL);
		r_m_action(ERx("newline"), NULL);
		r_m_action(ERx("end"), ERx("within"), NULL);
	}

	r_m_action(ERx("end"), ERx("block"), NULL);
	if ((cury > 1) || gotc0)
	{	/* put in a separator line */
		r_m_action(ERx("newline"), NULL);
	}

	/* Now set up the report header */

	Cact_attribute = NAM_REPORT;
	Cact_type = B_HEADER;
	r_m_rhead();
	r_m_ptop(0,0,1);
	r_m_rtitle(1,2);
	r_m_chead(1,2);

	/* Set up the page header */

	Cact_attribute = NAM_PAGE;
	Cact_type = B_HEADER;
	r_m_ptop(1,0,1);
	r_m_chead(1,2);
	if ((cury <= 1) && (En_n_attribs>1))
	{	
		/* 
		** suppress reprint of all sorted column(s):
		*/
		for (srt=Ptr_sort_top, seq=1; seq<=En_scount; srt++, seq++)
		{
			att = &(Ptr_att_top[srt->sort_ordinal-1]);

			if  ((att->pre_deleted) ||
			     (att->att_value.db_data == (PTR)NULL))
			{
				continue;
			}

			if (!STequal(att->att_name,NAM_DETAIL)  &&
		    		!STequal(att->att_name,NAM_PAGE)  &&
		    		!STequal(att->att_name,NAM_REPORT) )
			{
				_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
				fmt_fmtstr(Adf_scb, att->att_format, fmtstr);
				r_m_action(ERx("tformat"), &tmp_buf1[0],
					ERx("("), fmtstr, ERx(")"), NULL);
				fmt_size(Adf_scb, att->att_format, 
					&(att->att_value), &rows, &cols);
				CVna(cols, ibuf);
				r_m_action(ERx("format"), &tmp_buf1[0],
					ERx("(b"), ibuf, ERx(")"), NULL);
			}
		}
	}

	/* Set up the page footer */

	Cact_attribute = NAM_PAGE;
	Cact_type = B_FOOTER;
	r_m_pbot(1,1);

	/* Now set up the header(s) for the sort column(s) */

	if ((cury<=1) && (En_n_attribs>1))
	{	/* suppress if fits on one line */

		for (srt=Ptr_sort_top, seq=1; seq<=En_scount; srt++, seq++)
		{
			att = &(Ptr_att_top[srt->sort_ordinal-1]);
			if  ((att->pre_deleted) ||
			     (att->att_value.db_data == (PTR)NULL))
			{
				continue;
			}
			if (!STequal(att->att_name,NAM_DETAIL)  &&
		    		!STequal(att->att_name,NAM_PAGE)  &&
		    		!STequal(att->att_name,NAM_REPORT) )
			{
				Cact_attribute = att->att_name;
				_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
				Cact_type = B_HEADER;
				fmt_fmtstr(Adf_scb, att->att_format, fmtstr);
				r_m_action(ERx("tformat"), &tmp_buf1[0],
					ERx("("), fmtstr, ERx(")"), NULL);
				fmt_size(Adf_scb, att->att_format, 
					&(att->att_value), &rows, &cols);
				CVna(cols, ibuf);
				r_m_action(ERx("format"), &tmp_buf1[0],
					ERx("(b"), ibuf, ERx(")"), NULL);
			}
		}
	}
	return;
}
