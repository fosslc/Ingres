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
**   R_ACC_MAKE - create an ACC structure , filling in the fields.
**
**	Parameters:
**		brk_ord - ordinal of break aggregating the ordinal.
**		ordinal - ordinal of attribute being aggregated.
**		preset - PRS structure, if specified.
**		fid - function id of aggregate.
**		wrkspclen - length of workspace for aggregation.
**		isunique - TRUE if aggregate is unique.
**		result - datatype of result for aggregate.
**
**	Returns:
**		address of ACC structure.
**
**	Called by:
**		r_acc_get, r_p_agg.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		2/8/84 (gac)	written.
**		22-oct-1992 (rdrane)
**			Propagate precision to result DB_DATA_VALUE.
*/




ACC	*
r_acc_make(brk_ord, ordinal, preset, fid, wrkspclen, isunique, result)
ATTRIB	brk_ord;
ATTRIB	ordinal;
PRS	*preset;
ADI_FI_ID fid;
i4	wrkspclen;
bool	isunique;
DB_DATA_VALUE *result;
{
	ACC	*nacc;

	nacc = (ACC *) MEreqmem(0,sizeof(ACC),TRUE,(STATUS *) NULL);
	nacc->acc_break = brk_ord;
	nacc->acc_attribute = ordinal;
	nacc->acc_preset = preset;
	nacc->acc_unique = isunique;
	nacc->acc_datatype = result->db_datatype;
	nacc->acc_length = result->db_length;
	nacc->acc_prec = result->db_prec;
	nacc->acc_ags.adf_agfi = fid;
	nacc->acc_ags.adf_agwork.db_length = wrkspclen;
	nacc->acc_ags.adf_agwork.db_data = (PTR) MEreqmem(0,wrkspclen,TRUE,(STATUS *) NULL);
	nacc->acc_below = NULL;
	nacc->acc_above = NULL;

	return(nacc);
}
