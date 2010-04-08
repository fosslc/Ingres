/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rretwid.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_RET_WID - return the width of a column, based on an evaluation
**	of the att_width and att_fmt fields in the ATT structure.
**
**	Parameters:
**		ordinal		attribute ordinal of column
**
**	Returns:
**		width of attribute, or 0.
**
**	Called by:
**		r_eval_pos, r_nxt_wi, r_scn_tcmd.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		160, 164.
**
**	History:
**		1/11/82 (ps)	written.
**		8/10/83 (gac)	added date templates.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i2
r_ret_wid(ordinal)
ATTRIB	ordinal;
{
	/* internal declarations */

	register ATT	*att=NULL;		/* ATT of aordinal */
	register i2	width=0;		/* temp holder of return val */
	register FMT	*fmt=NULL;		/* fast ptr to format */
	STATUS		status;
	i4		rows;
	i4		cols;

	/* start of routine */



	att = r_gt_att(ordinal);

	if (att->att_width > 0)
	{	/* explicitly set.  Use this */

		width = att->att_width;
	}
	else
	{	/* calculate width from the format */
		fmt = att->att_format;
		if (fmt != NULL)
		{
			status = fmt_size(Adf_scb, fmt, &(att->att_value),
					  &rows, &cols);
			if (status != OK)
			{
				FEafeerr(Adf_scb);
			}
			else
			{
				width = cols;
			}
		}
	}


	return(width);
}
