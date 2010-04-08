/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rretacc.c	30.1	11/14/84"; */

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
**   R_RET_ACC - return address of an ACC structure, given the
**	identifying information.  An attribute ordinal of A_GROUP
**	means to use the current .WITHIN attribute.
**
**	Parameters:
**		aordinal	attribute ordinal of column being aggregated.
**		bordinal	ordinal of break doing aggregating.
**		preset		PRS structure for the aggregate.
**		fid		function id for aggregate.
**		unique		TRUE if aggregate is unique.
**
**	Returns:
**		address of ACC, or NULL.
**
**	Called by:
**		r_x_pracc, r_a_avg, r_a_cum.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		160, 163.
**
**	History:
**		1/11/82 (ps)	written.
**      3/17/89 (martym)
**			Fixed to use opid instead of fid to find acc struct.
**			Bug #4684.
*/



ACC	*
r_ret_acc(aordinal, bordinal, preset, fid, unique)
ATTRIB	aordinal;
ATTRIB	bordinal;
PRS	*preset;
ADI_FI_ID fid;
bool	unique;
{
	/* internal declarations */

	register ATT	*att;			/* ATT of aordinal */
	register ACC	*acc;			/* ptr to list of ACC structs */
	register LAC	*lac;
	ADI_FI_DESC     *fdesc;
	ADI_FI_DESC     *fdesc2;

	/* start of routine */


	att = r_gt_att(aordinal);

	adi_fidesc(Adf_scb, fid, &fdesc);
	for (lac = att->att_lac; lac != NULL; lac = lac->lac_next)
	{
		adi_fidesc(Adf_scb, lac->lac_acc->acc_ags.adf_agfi, &fdesc2);
		if (fdesc2->adi_fiopid == fdesc->adi_fiopid)
		{
			break;
		}
	}

	for (acc = lac->lac_acc; acc != NULL; acc = acc->acc_below)
	{
		adi_fidesc(Adf_scb, acc->acc_ags.adf_agfi, &fdesc2);
		if (bordinal == acc->acc_break && preset == acc->acc_preset &&
		    fdesc->adi_fiopid == fdesc2->adi_fiopid && 
			unique == acc->acc_unique)
		{	/* this is it */
			break;
		}
	}


	return(acc);
}
