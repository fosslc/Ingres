/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
**   R_ACC_GET - either find an existing ACC structure in a LAC list, or
**	create a new one, filling in the fields.
**	This will add a new ACC into a linked list of leveled aggregates
**	for an attribute, if it is not a page break aggregation and it
**	has no preset value.  Otherwise, it is left without an upward
**	link, in order to distinguish it as one that needs full aggregation
**	(i.e. aggregating on every data tuple.).
**
**	Parameters:
**		listadd - address of ptr to linked list of ACC's.
**		ordinal - ordinal of attribute being aggregated.
**		bordinal - ordinal of break aggregating the ordinal.
**		preset - PRS structure, if specified.
**		fid - function id of aggregate.
**		wrkspclen - length of workspace needed for aggregation.
**		isunique - TRUE if aggregate is prime or unique.
**		result - datatype of result for aggregate.
**
**	Returns:
**		address of ACC structure.  Do not return NULL ptr.
**
**	Called by:
**		r_mk_agg.
**
**	Side Effects:
**		May update part of the ACC structure for an attribute.
**
**	Error messages:
**		Syserr:Bad break ordinal for att.
**
**	Trace Flags:
**		3.0, 3.11.
**
**	History:
**		5/9/81 (ps) - written.
**		2/8/84	(gac)	added aggregates for dates.
**		3/17/89 (martym)
**			Fixed so that opid is used as key to find 
**			acc struct instead of fid. Bug #4684.
**		24-jul-89 (sylviap)
**			Bug 6209: Added check for unique and average.
*/



ACC	*
r_acc_get(listadd, bordinal, ordinal, preset, fid, wrkspclen, isunique, result)
ACC	**listadd;
ATTRIB	bordinal;
ATTRIB	ordinal;
PRS	*preset;
ADI_FI_ID fid;
i4	wrkspclen;
bool	isunique;
DB_DATA_VALUE *result;
{

	/* internal declarations */

	ACC	*acc;			/* ptr to one ACC structure */
	ACC	*nacc;			/* ptr to a second one (used in creation) */
	ATTRIB	brkord;			/* ordinal of break of attribute */
	bool	linkok = TRUE;		/* true if ok to try to link in new acc */
	ACC	*oldacc = NULL;		/* previous acc found in linked list */
	ACC	*r_acc_make();
	ADI_FI_DESC	*fdesc;
	ADI_OP_ID	avg_opid;
	ADI_FI_DESC	*fdesc2;

	/* start of routine */



	brkord = r_fnd_sort(bordinal);		/* break ordinal of break */



	if (brkord == A_ERROR)
	{
		r_syserr(E_RW0003_r_acc_get_Bad_break);
	}

	afe_opid(Adf_scb, ERx("avg"), &avg_opid);
	adi_fidesc(Adf_scb, fid, &fdesc);

	/* see if ok to link in new structure */

	if (preset != NULL || bordinal == A_PAGE || isunique ||
	    fdesc->adi_fiopid == avg_opid)
	{
		linkok = FALSE;
	}

	/* add in correct order, or find existing one */

	for(acc = *listadd; acc!=NULL; acc=acc->acc_below)
	{
		adi_fidesc(Adf_scb, acc->acc_ags.adf_agfi, &fdesc2);
		if (acc->acc_break == bordinal && preset == acc->acc_preset &&
		    fdesc2->adi_fiopid == fdesc->adi_fiopid 
			&& acc->acc_unique == isunique)
		{	/* match existing one */
			return(acc);
		}

		if (linkok == FALSE)
		{	/* use this loop to skip to end of the linked list */
			oldacc = acc;
			continue;
		}

		adi_fidesc(Adf_scb, acc->acc_ags.adf_agfi, &fdesc);

		if (r_fnd_sort(acc->acc_break) < brkord ||
		    acc->acc_preset != NULL || acc->acc_break == A_PAGE ||
		    acc->acc_unique || fdesc->adi_fiopid == avg_opid)
		{	/* went too far. insert new one */
			nacc = r_acc_make(bordinal, ordinal, preset, fid,
					  wrkspclen, isunique, result);
			nacc->acc_below = acc;		/* link it in */
			/* 
			** Bug 6209 - added check for uniqueness.  Also added
			** check for average.  All other logic checked for 
			** average, so appears that this should also.
			*/
			if ((acc->acc_preset == NULL) && 
				(acc->acc_break != A_PAGE) && (!acc->acc_unique)
		    		&& (fdesc->adi_fiopid != avg_opid))
			{	/* link it in */
				acc->acc_above = nacc;
			}	/* link it in */
			if (oldacc != NULL)
			{	/* not first in list */
				oldacc->acc_below = nacc;
				nacc->acc_above = oldacc;
				return(nacc);
			}

			/* first in list */

			*listadd =  nacc;
			return(nacc);

		}
		oldacc = acc;

	}

	/* add to end of list */

	nacc = r_acc_make(bordinal, ordinal, preset, fid, wrkspclen, isunique,
			  result);
	if (oldacc == NULL)
	{
		*listadd = nacc;
	}
	else
	{
		oldacc->acc_below = nacc;
	}
	if (linkok == TRUE)
	{
		nacc->acc_above = oldacc;
	}
	return(nacc);

}
