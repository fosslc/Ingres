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
# include	<er.h>
# include	<errw.h>

# define	TAB_PRETITLE	ERget(F_RW0002_Report_on_Table_)
# define	JD_PRETITLE 	ERget(F_RW0007_Report_on_JoinDef_)

/*
**   R_M_RTITLE - add commands for the report title.
**	This will be of the form "Table: tabname".
**
**	Parameters:
**		labove	number of newlines above the title.
**		lbelow	number of newlines after the title.
**
**	Returns:
**		none.
**
**	Side Effects:
**		May set St_right_margin.
**
**	Called by:
**		r_m_columns, r_m_wrap.
**
**	History:
**		3/29/82 (ps)	written.
**		1/10/89 (martym)  Changed PRETITLE to TAB_PRETITLE, and added 
**				JD_PRETITLE.
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
**		Make labove and lbelow nat's, not i2's to prevent
**		stack overflow when called from r_m_block(). This
**		routine is only ever called with integer constant
**		args so i4  is appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-Mar-2005 (lakvi01)
**          Properly prototype function.
*/


VOID
r_m_rtitle(labove,lbelow)
i4	labove;
i4	lbelow;
{
	/* internal declarations */

	char	ibuf[50];		/* hold ascii of number */
	i4	width;			/* width of title */
	i4	start;			/* starting position of title */
	char	tmp_buf1[FE_MAXTABNAME];

	/* start of routine */



	r_m_action(ERx("begin"), ERx("rbftitle"), NULL);

	if (En_SrcTyp == JDRepSrc)
		width = STlength(En_ferslv.name) + STlength(JD_PRETITLE);
	else
		width = STlength(En_ferslv.name) + STlength(TAB_PRETITLE);

	start = ((St_right_margin-St_left_margin)/2) - (width/2);
	if (start < St_left_margin)
	{
		start = St_left_margin;
	}

	if (St_right_margin < (start+width))
	{	/* change right margin */
		St_right_margin = St_left_margin + width;
	}

	if (labove > 0)
	{	/* put in newlines above */
		CVna(labove,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}

	r_m_action(ERx("underline"), NULL);
	CVna((i4)start,ibuf);
	r_m_action(ERx("tab"),	ibuf,  NULL);

	/*
	** In addition to having the [owner.]tablename unnormalized,
	** any embedded single quotes must be escaped, since the name
	** appears in the command as a string quoted literal.
	*/
	rF_bstrcpy(&tmp_buf1[0],En_ferslv.name);
	if (En_SrcTyp == JDRepSrc)
	{
	    r_m_action(ERx("print"),ERx("\'"),JD_PRETITLE,&tmp_buf1[0],
		       ERx("\'"),NULL);
	}
	else
	{
		r_m_action(ERx("print"),ERx("\'"),TAB_PRETITLE,&tmp_buf1[0],  
			   ERx("\'"),NULL);
	}

	r_m_action(ERx("endprint"), NULL);
	r_m_action(ERx("nounderline"), NULL);

	if (lbelow > 0)
	{	/* put lines below the title */
		CVna(lbelow,ibuf);
		r_m_action(ERx("newline"),  ibuf,  NULL);
	}
	r_m_action(ERx("end"), ERx("rbftitle"), NULL);


	return;
}
