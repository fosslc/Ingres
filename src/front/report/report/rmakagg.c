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
# include       <er.h>

/*
**   R_MAK_AGG - control routine for setting up the ACC structures
**	of accumulators.  This either creates or returns a ptr to an
**	accumulator.
**
**	Parameters:
**		ordinal - ordinal of attribute being aggregated.
**		bordinal - ordinal of brk doing aggregating.
**		preset - ptr to PRS structure defining the presetting, if any.
**		fid - function id for the aggregate.
**		wrkspclen - length of the workspace for aggregation.
**		isunique - TRUE if the aggregate is prime or unique.
**		result - datatype of result for aggregate.
**
**	Returns:
**		address of ACC structure where
**		this aggregate value is to be stored.
**
**	Called by:
**		r_p_agg, r_mk_cum.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		Syserr: Bad agg type.
**		Syserr: Bad attribute ordinal.
**
**	Trace Flags:
**		3.0, 3.10.
**
**	History:
**		5/9/81 (ps) - written.
**		1/25/84 (gac)	changed for printing expressions.
**		2/8/84	(gac)	added aggregating dates.
**		3/17/89 (martym)
**			Fixed so that opid is used as key to find 
** 			proper acc struct instead of fid.
**			Bug #4684.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



ACC *
r_mk_agg(ordinal, bordinal, preset, fid, wrkspclen, isunique, result)
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

	/* internal declarations */

	register ATT	*att;			/* fast ptr to ATT of aggregatee */
	register ACC	*acc;			/* ptr to ACC struct for this accum */
	STATUS		status;
	LAC		*lac;
	ADI_FI_DESC *fdesc;
	ADI_FI_DESC *fdesc2;

	/* start of routine */



	if (ordinal <= 0)
	{
		r_syserr(E_RW0031_Bad_attribute);
	}

	/* get ATT structure */

	att = r_gt_att(ordinal);

	/* search for beginning of list for this aggregate operation */

	adi_fidesc(Adf_scb, fid, &fdesc2);
	for (lac = att->att_lac; lac != NULL; lac = lac->lac_next)
	{
		adi_fidesc(Adf_scb, lac->lac_acc->acc_ags.adf_agfi, &fdesc);
	    if (fdesc->adi_fiopid == fdesc2->adi_fiopid)
	    {
			break;
	    }
	}

	/* if couldn't find a list pointer for this aggregate operation,
	** create one and insert it into the beginning of the LAC list */

	if (lac == NULL)
	{
	    lac = (LAC *) MEreqmem(0,(u_i4) sizeof(LAC),TRUE,(STATUS *) NULL);
	    lac->lac_acc = NULL;
	    lac->lac_next = att->att_lac;
	    att->att_lac = lac;
	}

	acc = r_acc_get(&(lac->lac_acc), bordinal, ordinal, preset,
			fid, wrkspclen, isunique, result);

	status = adf_agbegin(Adf_scb, &(acc->acc_ags));
	if (status != OK)
	{
		FEafeerr(Adf_scb);
	}


	return(acc);
}
