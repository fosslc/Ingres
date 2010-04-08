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
# include	<errw.h>

/*
**   R_MAK_CUM - set up a CUM structure for calculating one cumulative.
**
**	Parameters:
**		ordinal - ordinal of attribute being aggregated.
**		bordinal - ordinal of break doing the aggregating.
**		preset - PRS structure to pass through to r_mk_agg.
**		fid - function id of the aggregate.
**		wrkspclen - length of the workspace needed for aggregation.
**		isunique - TRUE if aggregate is prime or unique.
**		result - datatype of result for aggregate.
**
**	Returns:
**		address of CUM which is set up.
**
**	Called by:
**		r_p_agg.
**
**	Side Effects:
**		Updates the Cact_tcmd ptr.
**
**	Error Messages:
**		Syserr:Bad attribute ordinal.
**
**	Trace Flags:
**		3.0, 3.12.
**
**	History:
**		5/12/81 (ps) - written.
**		1/13/84 (gac)	changed for expressions.
**		2/9/84 (gac)	added aggregates for dates.
**		3/17/89 (martym)
**			Fixed so that acc structs are created 
**			in the calling routine. Bug #4684.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in CUM structures.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



CUM	*
r_mk_cum(ordinal, bordinal, preset, fid, wrkspclen, isunique, result)
ATTRIB	ordinal;
ATTRIB	bordinal;
PRS	*preset;
ADI_FI_ID fid;
i4	wrkspclen;
bool	isunique;
DB_DATA_VALUE *result;
{
	/* external declarations */
	ATT		*r_gt_att();		/* (mmm) added declaration */
	ACC		*r_mk_agg();		/* (mmm) added declaration */

	/* internal declarations */

	CUM		*cum;			/* tmp ptr to CUM structure */
	ATTRIB		ord;			/* return from r_grp_nxt */
	ATT		*att;

	/* start of routine */

	/* create a CUM structure and put it in the Cact_tcmd struct */

	cum = (CUM *) MEreqmem(0,(u_i4) sizeof(CUM),TRUE,(STATUS *) NULL);
	cum->cum_break = bordinal;
	cum->cum_attribute = ordinal;
	cum->cum_prs = preset;
	cum->cum_unique = isunique;
	cum->cum_datatype = result->db_datatype;
	cum->cum_length = result->db_length;
	cum->cum_prec = result->db_prec;
	cum->cum_ags.adf_agfi = fid;
	cum->cum_ags.adf_agwork.db_length = wrkspclen;
	cum->cum_ags.adf_agwork.db_data = (PTR) MEreqmem(0,(u_i4) wrkspclen,
							TRUE,(STATUS *) NULL);


	return(cum);
}
