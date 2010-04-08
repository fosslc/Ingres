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
# include	<er.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
**   R_M_PTOP - set up the page top headings in a default report.
**	This will contain the date, time and report name in different
**	combinations, depending on the report width.
**
**	Parameters:
**		type = 0 if the page top for the report.  This will
**			not contain the report name.
**		     = 1 if the page top for the page break.  This will
**			contain the report name.
**		labove	number of lines above the heading.
**		lbelow	number of lines below the heading.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_m_columns, r_m_wrap.
**
**	Trace Flags:
**		50, 59.
**
**	History:
**		3/30/82 (ps)	written.
**		8/25/82 (ps)	fix bugs.
**		05-mar-91 (sylviap)
**			Increased the buffer size of title to be able to
**			handle long table names. (#34506).
**	30-sep-1992 (rdrane)
**		Replace En_relation with reference to global FE_RSLV_NAME 
**		En_ferslv.name.
**	9-nov-1992 (rdrane)
**		Force generation of single quoted string constants
**		for 6.5 delimited identifier support.
**	27-oct-1993 (rdrane)
**		In addition to generating the [owner.]tablename as
**		unnormalized, we additionally need to escape any
**		embedded single quotes so the generated .PRINT
**		command doesn't fail.
**	22-dec-1993 (mgw) Bug #58122
**		Make type, labove and lbelow nat's, not i2's to prevent
**		stack overflow when called from r_m_block(). This routine
**		is only ever called with integer constant args so i4
**		is appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Mar-2005 (lakvi01)
**          Properly prototype function.
*/


VOID
r_m_ptop(type,labove,lbelow)
i4	type;
i4	labove;
i4	lbelow;
{
	/* internal declarations */

	i4		wdate = 11;	/* width of date */
	i4		wtime = 8;	/* width of time */
	i4		wname;		/* width of title */
	i4		wreport;	/* width of the report */
	i4		midrep;		/* midpoint of report */
	char		title[(2 * FE_MAXTABNAME)];	
					/* hold title of report */
					/* 2x to accommodate "Report: " */
					/* (2x is arbitrary) */
	char		ibuf[20];	/* holds return from itoa */
	char		tmp_buf[FE_MAXTABNAME];

	/* start of routine */



	r_m_action(ERx("begin"), ERx("rbfptop"), NULL);
	if (labove > 0)
	{	/* put some lines above the page top */
		CVna(labove,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}

	STcopy(ERget(F_RW0001_Report_), title);
	rF_bstrcpy(&tmp_buf[0],En_ferslv.name);
	STcat(title,&tmp_buf[0]);
	wname = (type==0) ? 0 : (STlength(title));

	wreport = St_right_margin - St_left_margin;
	midrep = wreport/2;

	if (((midrep-(wname/2)+1)>(St_left_margin+wdate+1))
		&& ((midrep+(wname/2)+1)<(St_right_margin-wtime)))
	{	/* all of them fit.  Print them all */

		r_m_action(ERx("left"), NULL);
		r_m_action(ERx("print"), ERx("current_date"), NULL);
		r_m_action(ERx("endprint"), NULL);

		if (type != 0)
		{
			r_m_action(ERx("center"), NULL);
			r_m_action(ERx("print"), ERx("\'"), title, ERx("\'"), NULL);
			r_m_action(ERx("endprint"), NULL);
		}

		r_m_action(ERx("right"), NULL);
		r_m_action(ERx("print"), ERx("current_time"), NULL);
		r_m_action(ERx("endprint"), NULL);
	}
	else if ((type!=0) && ((wdate+wname+1) <= wreport))
	{	/* at least date and name fit */

		r_m_action(ERx("left"), NULL);
		r_m_action(ERx("print"), ERx("current_date"), NULL);
		r_m_action(ERx("endprint"), NULL);

		r_m_action(ERx("right"), NULL);
		r_m_action(ERx("print"), ERx("\'"), title, ERx("\'"), NULL);
		r_m_action(ERx("endprint"), NULL);
	}
	else if (type!=0)
	{	/* only the name will fit.  Use that */
		r_m_action(ERx("center"), NULL);
		r_m_action(ERx("print"), ERx("\'"), title, ERx("\'"), NULL);
		r_m_action(ERx("endprint"), NULL);
	}
	else
	{	/* only the date will fit.  Print it */
		r_m_action(ERx("center"), NULL);
		r_m_action(ERx("print"), ERx("current_date"), NULL);
		r_m_action(ERx("endprint"), NULL);
	}

	if (lbelow > 0)
	{	/* number of lines below the heading */
		CVna(lbelow,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}
	r_m_action(ERx("end"), ERx("rbfptop"), NULL);

	return;
}
