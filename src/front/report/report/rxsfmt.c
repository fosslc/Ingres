/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_X_SFORMAT - set a new default format for an attribute.
**
**	Parameters:
**		ordinal - ordinal of attribute for this format.
**		fmt - FMT structure to use.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd, r_fmt_dflt, r_st_adflt.
**
**	Side Effects:
**		Updates the FMT structure in one of the ATT structures.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		5.0, 5.10.
**
**	History:
**		6/6/81 (ps) - written.
**      	07/20/89 (martym) - Bug #6527. Fixed by taking the absolute 
**		value of each attribute's datatype when comparing it to DB_
**		FLT_TYPE -or- DB_DTE_TYPE, since the data type could be 
**		nullable.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-sep-2006 (gupsh01)
**	    Added support for new ANSI date/time types.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_sformat(ordinal,fmt)
ATTRIB	ordinal;
register FMT	*fmt;
{
	/* internal declarations */

	register ATT	*attribute;		/* fast ptr to ATT structure */
	STATUS		status;
	i4		rows;
	i4		cols;
	i4		kind;
	i4		old_kind;

	/* start of routine */



	attribute = r_gt_att(ordinal);

	if (attribute->att_format != NULL)
	{
	    fmt_kind(Adf_scb, attribute->att_format, &old_kind);
	}
	else
	{
	    old_kind = ~FM_B_FORMAT;
	}

	attribute->att_format = fmt;

	status = fmt_kind(Adf_scb, fmt, &kind);
	if (status != OK)
	{
	    FEafeerr(Adf_scb);
	}

	status = fmt_size(Adf_scb, fmt, &(attribute->att_value),
			  &rows, &cols);
	if (status != OK)
	{
	    FEafeerr(Adf_scb);
	}

	if (rows != 1 || kind == FM_B_FORMAT)
	{
	    attribute->att_dformatted = FALSE;
	}
	/*
	** Bug #6527. Take the absolute value of the attribute data type,
	** since it could be nullable:
	*/
	else if (St_before_report && !(attribute->att_dformatted) &&
		 old_kind != FM_B_FORMAT &&
	         ((!St_tflag_set && !St_compatible) ||
	          ((St_truncate || (!St_tflag_set && St_compatible)) &&
	           (abs(attribute->att_value.db_datatype) == DB_FLT_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_DTE_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_ADTE_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_TMWO_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_TMW_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_TME_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_TSWO_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_TSW_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_TSTMP_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_INYM_TYPE ||
	            abs(attribute->att_value.db_datatype) == DB_INDS_TYPE))))
	{
	    /*
	    ** In 5.0, +t was the default.
	    ** In 6.0, neither +t nor -t is the default so for compatibility
	    ** make the default +t.
	    ** For this routine, +t implies only floats and dates break
	    ** when their formatted values change. Other types break when
	    ** their actual value changes.
	    ** If -t is explicitly set (even with the compatibility flag set),
	    ** all attributes break when their actual value changes,
	    ** NOT when their formatted value changes.
	    ** If neither +t nor -t is set, any attribute has the ability
	    ** to break when its formatted value changes.
	    */

	    /*
	    ** If an attribute has its format set in the report header,
	    ** then it will break when its formatted value changes.
	    ** This code gets space to hold the current and previous
	    ** formatted values.
	    */

	    attribute->att_dformatted = TRUE;
	    if (attribute->att_brk_ordinal > 0)
	    {
		cols += DB_CNTSIZE;
		attribute->att_dispwidth = cols;
		attribute->att_cdisp = (DB_TEXT_STRING *) MEreqmem(0,cols,TRUE,(STATUS *) NULL);
		attribute->att_pdisp = (DB_TEXT_STRING *) MEreqmem(0,cols,TRUE,(STATUS *) NULL);
	    }
	}


	return;
}
