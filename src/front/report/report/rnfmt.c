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
# include	<me.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>

/*
**   R_N_FMT - return a pointer to a format structure
**	given an ordinal. This is used for aggregates.
**
**	Parameters:
**		ordinal		attribute ordinal of column
**		fid		function id of the aggregate.
**
**	Returns:
**		ptr to either the fmt in the attribute, if it
**		is already the correct type or to an internal
**		FMT structure, if it is not.
**
**	Called by:
**		r_x_prelement.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		160, 165.
**
**	History:
**		1/11/82 (ps)	written.
**		8/15/83 (gac)	bug 1466 -- count agg has integer format by default.
**		4/2/84 (gac)	got rid of type param and r_chk_fmt reference.
**		04-dec-1987 (rdesmond)
**			Removed conditional to check if format already exists
**			for field (for agg type 'count').  Default format
**			should always be created since the format may be
**			incompatible with the datatype of the agg (int).
**			Also, now creates default format for 'any', which
**			is also of type int.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



FMT	*
r_n_fmt(ordinal, fid)
ATTRIB	ordinal;
ADI_FI_ID fid;
{
	/* external declarations */
	ATT		*r_gt_att();

	/* internal declarations */

	ATT		*att;		/* ATT of aordinal */
	FMT		*fmt;		/* fast ptr to format */
	static	FMT	tempfmt = {0};	/* fmt structure */
	i2		width;		/* return from r_ret_wid */
	i4		rows;
	i4		cols;
	STATUS		status;
	char		fmtstr[MAX_FMTSTR];
	i4		size;
	PTR		area = NULL;
	char		err_buf[ER_MAX_LEN];
	ADI_OP_ID	cnt_opid;
	ADI_OP_ID	any_opid;
	ADI_FI_DESC	*fdesc;

	/* start of routine */

	att = r_gt_att(ordinal);
	fmt = att->att_format;
	afe_opid(Adf_scb, ERx("count"), &cnt_opid);
	afe_opid(Adf_scb, ERx("any"), &any_opid);
	adi_fidesc(Adf_scb, fid, &fdesc);

	if (fdesc->adi_fiopid == cnt_opid || fdesc->adi_fiopid == any_opid)
	{   /* aggregate is 'count' or 'any' */
	    /* create a default format for the type */

	    status = fmt_deffmt(Adf_scb, &(att->att_value), MAX_CF, TRUE,
	      fmtstr);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
		return(NULL);
	    }

	    status = fmt_areasize(Adf_scb, fmtstr, &size);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
		return(NULL);
	    }

	    area = MEreqmem(0,size,TRUE,&status);
	    if (area == NULL)
	    {
		ERreport(status, err_buf);
		SIprintf(err_buf);
		return(NULL);
	    }

	    status = fmt_setfmt(Adf_scb, fmtstr, area, &fmt, NULL);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
		return(NULL);
	    }

	    /* get the width of the format */

	    status = fmt_size(Adf_scb, fmt, &(att->att_value), &rows, &cols);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
		return(NULL);
	    }

	    if (area != NULL)
	    {
		MEfree((PTR)area);
	    }

	    /* create a default format for the count */

	    width = min(cols, 12);
	    STcopy(ERx("n"), fmtstr);
	    CVna(width, fmtstr+1);
	    status = fmt_setfmt(Adf_scb, fmtstr, (PTR)&tempfmt, &fmt, NULL);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
		return(NULL);
	    }
	}

	return(fmt);
}
