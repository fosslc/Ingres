/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	 <cm.h> 
# include	<si.h>
# include	<er.h>
# include	<errw.h>

# define	CWIDTH	26
# define	OLDMAXRNAME	12

/*
**   R_M_BLOCK - set up a BLOCK default report.  Note that the
**	right and left margins have been set in r_m_rprt.
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
**		May change format widths for numeric fields.
**
**	Trace Flags:
**		50, 56.
**
**	History:
**		3/30/82 (ps)	written.
**		6/26/84	(gac)	fixed bug 2445 -- added date type.
**		2/9/90  (martym) Fixed bug 8122 -- So that it would 
**			behave consistently for NULLABLE and NOT_NULLABLE 
**			fields.
**              4/5/90 (elein) performance
**                      Since fetching attribute call needs no range
**                      check use &Ptr_att_top[] directly instead of
**                      calling r_gt_att. We don't expect brk_attribute
**                      to be A_REPORT, A_GROUP, or A_PAGE
**		4/22/90 (elein) IBM Porting change
**			(1/16/90-mdw) change call to fmt_setfmt to use a
**			variable rather than an ERx-defined string
**			with the call--reentrancy problems in IBM
**		08-jun-90 (sylviap)
**			Uses TTLE_DFLT to figure out if the column will fit
**			within the specified page width.  Flags error if
**			it cannot.
**		9-nov-1992 (rdrane)
**			Force generation of single quoted string constants
**			for 6.5 delimited identifier support.  Ensure att_name
**			unnormalized before generating RW statements.
**		12/09/92 (dkh)
**			Added Choose Columns support for rbf.
**		2-feb-1992 (rdrane)
**			Bypass columns which are unsupported datatypes as well
**			as pre-deleted.
**		27-oct-1993 (rdrane)
**			In addition to generating the column name as
**			unnormalized, we additionally need to escape any
**			embedded single quotes so the generated .PRINT
**			command doesn't fail.
**		22-dec-1993 (mgw) Bug #58122
**			Pad the proper buffer with blanks when right
**			justifying, tmp_buf2, not ibuf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Mar-2005 (lakvi01)
**	    Type-casted NULL as (i4 *) in call to fmt_setfmt() because
**	    it was segv-ing on AMD64 Windows.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_m_block()
{

	/* internal declarations */

	register ATT	*att;		/* ATT structure for column */
	register ATTRIB	i;		/* counter */
	i4		j;		/* another counter */
	i4		curx=0;		/* current position */
	i4		nextx=0;	/* start of next print position */
	i4		cury=1;		/* current line number */
	i2		width=0;	/* width of a column */
	i4		maxx=0;		/* actual maximum margin */
	char		ibuf[50];	/* holds ascii numbers from CVna  */
	char		newname[FE_MAXNAME+1];	/* name of column */
	char		fmtstr[MAX_FMTSTR];
	i4		name_len, tmp_len;
	char		numcol_width[4];/* width of numeric field column */
	char		tmp_buf1[(FE_UNRML_MAXNAME + 1)];
	char		tmp_buf2[(FE_UNRML_MAXNAME + 1)];

	/* start of routine */


	STcopy(ERx("f10"), numcol_width);

	/* First set up the detail lines */


	Cact_attribute = NAM_DETAIL;
	Cact_type = B_HEADER;

	curx = St_left_margin;
	nextx = St_left_margin;
	cury = 1;

	/* First fill in the default locations etc. for each column */

	for(i=1; i<=En_n_attribs; i++)
	{	/* place the next attribute */
		att = &(Ptr_att_top[i-1]);
		if  ((att->pre_deleted) ||
		     (att->att_value.db_data == (PTR)NULL))
		{
			continue;
		}
		name_len = STlength(att->att_name);
		if (name_len <= OLDMAXRNAME &&
		    (abs(att->att_value.db_datatype) == DB_INT_TYPE ||
		     abs(att->att_value.db_datatype) == DB_FLT_TYPE))
		{
			if (abs(att->att_value.db_datatype) == DB_INT_TYPE)
			{
				/* change width of numeric fields to 10 */
				fmt_setfmt(Adf_scb, numcol_width, att->att_format,
					   &(att->att_format), (i4 *)NULL);
				att->att_dformatted = FALSE;
				r_x_sformat(i, att->att_format);
			}

			width = 24;   /* 10 + title */
		}
		else
		{
			/* 
			** make room for the colon space 
			** after title (TTLE_DFLT)
			*/
			width = name_len + TTLE_DFLT + r_ret_wid(i);
		}

		/* Check if title and format fit within the page width */
		if (width > En_lmax)
		{
			tmp_len = En_lmax;
			r_error(1036, FATAL, &tmp_len, NULL);
		}
		att->att_width = width;
		if ((nextx+width)>=St_right_margin)
		{	/* go to next line */
			cury++;
			maxx=max(maxx,curx);
			curx = St_left_margin;
			nextx = curx;
		}
		else
		{	/* fits on this line */
			curx = nextx;
		}
		att->att_position = nextx;
		att->att_line = cury;
		nextx += (CWIDTH * ((width/CWIDTH)+1));
		curx += width;

	}

	maxx = max(maxx, curx);
	St_right_margin = maxx;


	if (cury > 1)
	{	/* keep on one page */
		CVna(cury,ibuf);
		r_m_action(ERx("need"), ibuf, NULL);
	}

	/* Now put out the commands for a wrapped detail line */

	r_m_action(ERx("block"), NULL);

	cury = 1;
	for (i=1; i<= En_n_attribs; i++)
	{	/* print out the next attribute */
		att = &(Ptr_att_top[i-1]);
		if  ((att->pre_deleted) ||
		     (att->att_value.db_data == (PTR)NULL))
		{
			continue;
		}
		_VOID_ IIUGxri_id(att->att_name,&tmp_buf1[0]);
		if (att->att_line > cury)
		{	/* go to a new line */
			r_m_action(ERx("end"), ERx("block"), NULL);
			r_m_action(ERx("block"), NULL);
			cury = att->att_line;
		}
		r_m_action(ERx("within"), &tmp_buf1[0], NULL);
		r_m_action(ERx("top"), NULL);
		STcopy(att->att_name, newname);
/*
** KJ	04/11/86 (TY)
*/
		CMtoupper(newname, newname);
		rF_bstrcpy(&tmp_buf2[0],newname);
		name_len = STlength(&tmp_buf2[0]);
		if (name_len <= OLDMAXRNAME &&
		    (abs(att->att_value.db_datatype) == DB_INT_TYPE ||
		     abs(att->att_value.db_datatype) == DB_FLT_TYPE))
		{
			/* always right justify */
			STcat(&tmp_buf2[0],ERx(": "));
			for (j=name_len; j < OLDMAXRNAME; j++)
			{	/* pad with blanks */
				STcat(&tmp_buf2[0],ERx(" "));
			}
			r_m_action(ERx("print"), ERx("\'"), &tmp_buf2[0],
				   ERx("\', "), &tmp_buf1[0], NULL);
		}
		else
		{
			/* always left justify */
			r_m_action(ERx("print"),ERx("\'"),&tmp_buf2[0],
				   ERx(": \', "), &tmp_buf1[0], NULL);
		}

		r_m_action(ERx("endprint"), NULL);
		r_m_action(ERx("newline"), NULL);
		r_m_action(ERx("end"), ERx("within"), NULL);
	}

	r_m_action(ERx("end"), ERx("block"), NULL);
	if (cury > 1)
	{	/* put in a separator line */
		r_m_action(ERx("newline"), NULL);
	}

	/* Now set up the report header */

	Cact_attribute = NAM_REPORT;
	Cact_type = B_HEADER;
	r_m_rhead();
	r_m_ptop(0,0,1);
	r_m_rtitle(1,2);

	/* Set up the page header */

	Cact_attribute = NAM_PAGE;
	Cact_type = B_HEADER;
	r_m_ptop(1,0,1);
	r_m_action(ERx("newline"), NULL);

	/* Set up the page footer */

	Cact_attribute = NAM_PAGE;
	Cact_type = B_FOOTER;
	r_m_pbot(1,1);

	return;
}

