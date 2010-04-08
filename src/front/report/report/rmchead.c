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
# include	<afe.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	 <cm.h> 
# include	<er.h>

/*
**   R_M_CHEAD - set up the column headings in a default report.
**
**	Parameters:
**		labove	number of newlines above the heading.
**		lbelow	number of newlines below the heading.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_m_columns.
**
**	Trace Flags:
**		50, 58.
**
**	History:
**		3/30/82 (ps)	written.
**		2/1/84 (gac)	added date type.
**              4/5/90 (elein) performance
**                              Since fetching attribute call needs no range
**                              check use &Ptr_att_top[] directly instead of
**                              calling r_gt_att. We don't expect brk_attribute
**                              to be A_REPORT, A_GROUP, or A_PAGE
**		9-nov-1992 (rdrane)
**			Force generation of single quoted string constants
**			for 6.5 delimited identifier support.  Ensure att_name
**			unnormalized before generating RW statements.
**			Ensure unsupported datatypes are screened out/ignored.
**		12/09/92 (dkh) - Added Choose Columns support for rbf.
**		27-oct-1993 (rdrane)
**			In addition to generating the column name as
**			unnormalized, we additionally need to escape any
**			embedded single quotes so the generated .PRINT
**			command doesn't fail.
**		21-jun-1995 (harpa06)
**			Kill the conversion of the first letter of a table name
**			to uppercase.
**		10-nov-1995 (canor01)
**			If not a FIPS installation, then capitalize
**			the first letter of the column name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Properly prototype function.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_m_chead(labove,lbelow)
i2	labove;
i2	lbelow;
{
	/* internal declarations */

	register ATT	*att;		/* ATT struct for a column */
	ATTRIB		i;		/* counter */
	i4		curline;	/* current line, in multiline */
	char		ibuf[20];	/* holds ascii number for itoa */
	char		newname[FE_MAXNAME+1];	/* hold name of column */
	DB_DATA_VALUE	float_type;
	bool		isnumber;
	char		tmp_buf[(FE_UNRML_MAXNAME + 1)];

	/* start of routine */

	float_type.db_datatype = DB_FLT_TYPE;



	r_m_action(ERx("begin"), ERx("rbfhead"), NULL);

	if (labove > 0)
	{	/* put newlines above the heading */
		CVna(labove,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}
	r_m_action(ERx("underline"), NULL);

	curline = 1;
	for(i=1; i<=En_n_attribs; i++)
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
		_VOID_ IIUGxri_id(att->att_name,&tmp_buf[0]);
		if (att->att_line != curline)
		{	/* go to the next line */

			CVna((i4)(att->att_line-curline),ibuf);
			r_m_action(ERx("newline"), ibuf, NULL);
			curline = att->att_line;
		}
		r_m_action(ERx("within"), &tmp_buf[0], NULL);
		r_m_action(ERx("top"), NULL);
		afe_cancoerce(Adf_scb, &(att->att_value), &float_type,
			      &isnumber);
		if (isnumber)
		{
		    /* right justify */
		    if (STlength(att->att_name)<att->att_width)
		    {	/* must move it over some */
			r_m_action(ERx("linestart"), NULL);
			CVna((i4)(att->att_width - STlength(att->att_name)),
			     ibuf);
			r_m_action(ERx("tab"), ERx("+"), ibuf, NULL);
		    }
		}
		STcopy(att->att_name, newname);
		if (IIUIcase() != UI_MIXEDCASE)
		    CMtoupper(newname, newname);
		rF_bstrcpy(&tmp_buf[0],newname);
		r_m_action(ERx("print"), ERx("\'"), &tmp_buf[0], ERx("\'"),
			   NULL);
		r_m_action(ERx("endprint"), NULL);
		r_m_action(ERx("end"), ERx("within"), NULL);
	}
	r_m_action(ERx("nounderline"), NULL);
	if (lbelow > 0)
	{	/* put newlines below */
		CVna(lbelow,ibuf);
		r_m_action(ERx("newline"), ibuf, NULL);
	}
	r_m_action(ERx("end"), ERx("rbfhead"), NULL);

	return;
}
