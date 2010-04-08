/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<er.h>

/*
**   R_ST_ADFLT - maybe set the default position and/or format
**	for an attribute.  This is called when scanning the
**	TCMD lists to return the length of an attribute which
**	is being printed, and optionally set the defaults if
**	it is the first reference to the attribute.
**
**	Parameters:
**		ordinal	ordinal of attribute being printed, or
**			the ordinal of the attribute being aggregated
**			in an aggregate.
**		curpos	current position (used to set att_position)
**		fmt	FMT structure, if specified.  Used to get
**			length or set att_format.
**		fid	function id of aggregate. This is 0 if no aggregate.
**
**	Returns:
**		length, in characters, of this printing of the
**		attribute, calculated from the format.
**
**	Called by:
**		r_scn_tcmd.
**
**	Side Effects:
**		May set att_position and att_format fields in the
**		ATT structure specified by ordinal.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		140, 146.
**
**	History:
**		9/30/81 (ps)	written.
**		16-dec-1987 (rdesmond)
**			check for agg "any" as well as "count" when filling
**			dvp.
**
**
 * Revision 61.1  88/03/25  22:12:12  sylviap
 * *** empty log message ***
 * 
 * Revision 60.8  87/12/17  17:07:40  rdesmond
 * bug fixes
 * 
 * Revision 60.7  87/08/21  08:43:23  peterk
 * update for ER changes.
 * 
 * Revision 60.6  87/08/16  10:56:14  peter
 * Remove ifdefs for xRTR1 xRTR2 xRTR3 xRTM
 * 
 * Revision 60.5  87/06/01  18:23:33  grant
 * fixed bug where a count aggregate of a string column with a numeric format
 * was getting a FMT error 21600B.
 * 
 * Revision 60.4  87/04/08  01:28:21  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.3  87/03/26  17:54:30  grant
 * Initial 6.0 changes calling FMT, AFE, and ADF libraries
 * 
 * Revision 60.2  86/11/19  19:48:15  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/19  19:48:04  dave
 * *** empty log message ***
 * 
 * Revision 40.3  85/10/29  15:35:24  grant
 * Porting integration.
 * 
**Revision 30.1  85/08/14  20:05:50  source
**llama code freeze 08/14/85
**
**		Revision 3.22.127.1  85/07/13  05:14:48  roger
**		Propagated fix for bug #4919.  In ".PR <attr> (c0)",
**		use the attribute width as the default.
**
**	23-oct-1992 (rdrane)
**		Ensure set/propagate precision in DB_DATA_VALUE
**		structures.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
r_st_adflt(ordinal, curpos, fmt, fid)
ATTRIB		ordinal;
i4		curpos;
register FMT	*fmt;
ADI_FI_ID	fid;
{
	/* internal declarations */

	register ATT	*att;			/* fast ptr to ATT structure */
	i4		len;			/* length of format */
	STATUS		status;
	bool		valid;
	i4		rows;
	ADI_OP_ID	cnt_opid;
	ADI_OP_ID	any_opid;
	ADI_FI_DESC	*fdesc;
	DB_DATA_VALUE	*dvp;
	DB_DATA_VALUE	temp_dbdv;

	/* start of routine */


	att = r_gt_att(ordinal);


	if (att->att_position < 0)
	{	/* hasn't been set yet */
		att->att_position = curpos;
	}

	if (fid != 0)
	{
		afe_opid(Adf_scb, ERx("count"), &cnt_opid);
		afe_opid(Adf_scb, ERx("any"), &any_opid);
		adi_fidesc(Adf_scb, fid, &fdesc);

		if (fdesc->adi_fiopid == cnt_opid ||
		  fdesc->adi_fiopid == any_opid)
		{   /* aggregate is count or any */
			dvp = &temp_dbdv;
			dvp->db_datatype = DB_INT_TYPE;
			dvp->db_length = sizeof(i4);
			dvp->db_prec = 0;
		}
		else
		{
			dvp = &(att->att_value);
		}
	}
	else
	{
		dvp = &(att->att_value);
	}

	/* if format specified, use it.  If not, use def */

	if (fmt != NULL)
	{
		status = fmt_isvalid(Adf_scb, fmt, dvp, &valid);
		if (status != OK)
		{
		    FEafeerr(Adf_scb);
		}

		if (att->att_format == NULL && valid && dvp != &temp_dbdv)
		{	/* use this as default */
			r_x_sformat(ordinal,fmt);
		}
	}
	else
	{	/* no format specified yet.  Set to system default */
		if (att->att_format == NULL)
		{	/* set up the default */
			r_fmt_dflt(ordinal);
		}

		if (fid != 0)
		{
			fmt = r_n_fmt(ordinal, fid);
		}
		else
		{
			fmt = att->att_format;
		}
	}

	status = fmt_size(Adf_scb, fmt, dvp, &rows, &len);
	if (status != OK)
	{
	    FEafeerr(Adf_scb);
	}


	return(len);
}
