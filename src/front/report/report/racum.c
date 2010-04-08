/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<fmt.h>
# include	<ug.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
**   R_A_CUM - evaluate a CUM structure to calculate a cumulative
**	of an aggregate.  The CUM structure points at one of the ACC
**	structures in a linked list.  By accumulating up in the list from the
**	pointer, the accumulation is found since the specified break
**	(who's ACC struct is pointed at by CUM).
**
**	Parameters:
**		cum - ptr to a CUM structure.
**		result - pointer to value of cumulative aggregate.
**
**	Returns:
**		none.
**
**	Called by:
**		r_e_item.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		Syserr: Bad cum address.
**		Syserr: Bad ACC address.
**
**	Trace Flags:
**		170, 173.
**
**	History:
**		5/10/81 (ps) - written.
**		8/15/83 (gac)  bug 1466 fix.
**		10/24/83(nml)- Put in gac's changes from VMS...cumulative
**			       count should be integer format.
**		12/23/83 (gac)	return value for expressions.
**		2/9/84	(gac)	added aggregates for dates.
**		17-dec-1987 (rdesmond)
**			added check for "any" agg to use "count" logic.
**		3/17/89 (martym)
**			Fixed so that the result data structure is filled
**			correctly; either from acc or cum. Bug #4684.
**		3/24/89 (martym)
**			Made modifications so that the result and cum 
**			data strucutures always get their data type &
**			length from the corresponding acc data structure.
**		22-oct-1992 (rdrane)
**			Propagate precision to result DB_DATA_VALUE.
**			Eliminate references to r_syserr() and use IIUGerr()
**			directly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_a_cum(cum,result)
register CUM		*cum;
register DB_DATA_VALUE	*result;
{
	/* internal declarations */

	register ACC	*acc;		/* follows the ACC linked list */
	STATUS		status;
	ADI_FI_DESC	*fdesc;
	ADI_OP_ID	cnt_opid;
	ADI_OP_ID	any_opid;
	DB_DATA_VALUE	agg_res;

	/* start of routine */


	if (cum == NULL)
	{
		IIUGerr(E_RW0005_r_a_cum_Null_CUM,UG_ERR_FATAL,0);
	}


	acc = r_ret_acc(cum->cum_attribute, cum->cum_break, cum->cum_prs,
			cum->cum_ags.adf_agfi, cum->cum_unique);

	if (acc == NULL)
	{
		IIUGerr(E_RW0006_r_a_cum_Null_ACC,UG_ERR_FATAL,0);
	}

	afe_opid(Adf_scb, ERx("count"), &cnt_opid);
	afe_opid(Adf_scb, ERx("any"), &any_opid);
	adi_fidesc(Adf_scb, cum->cum_ags.adf_agfi, &fdesc);

	result->db_datatype = acc->acc_datatype;
	result->db_length = acc->acc_length;
	result->db_prec = acc->acc_prec;
	result->db_data = (PTR) r_ges_next(result->db_length);

	if (acc->acc_above == NULL)
	{
	    /* this is done for efficiency and is the only way avg works
	    ** (and cnt would too if wasn't being handled special)
	    ** since avg(int) and cnt(non-int) have differing datatypes
	    ** of column and result */

	    status = adf_agend(Adf_scb, &(acc->acc_ags), result);
	    if (status != OK)
	    {
			FEafeerr(Adf_scb);
	    }
	}
	else
	{

		cum->cum_ags.adf_agfi = acc->acc_ags.adf_agfi;
		cum->cum_ags.adf_agwork.db_length = acc->acc_ags.adf_agwork.db_length;
		if (cum->cum_ags.adf_agwork.db_data != NULL)
			MEfree(cum->cum_ags.adf_agwork.db_data);
		cum->cum_ags.adf_agwork.db_data = 
				(PTR) MEreqmem(0,(u_i4) cum->cum_ags.adf_agwork.db_length, 
							TRUE,(STATUS *) NULL);

	    status = adf_agbegin(Adf_scb, &(cum->cum_ags));
	    if (status != OK)
	    {
			FEafeerr(Adf_scb);
	    }

		agg_res.db_datatype = acc->acc_datatype;
		agg_res.db_length = acc->acc_length;
		agg_res.db_prec = acc->acc_prec;
		agg_res.db_data = (PTR) r_ges_next(result->db_length);

	    /* now go up the linked list, accumulating */

	    for (; acc != NULL; acc = acc->acc_above)
	    {
			if (acc->acc_ags.adf_agcnt == 0)
			{
		    	/* no data values aggregated */
		    	continue;
			}

			status = adf_agend(Adf_scb, &(acc->acc_ags), &agg_res);
			if (status != OK)
			{
		    	FEafeerr(Adf_scb);
			}

			if (fdesc->adi_fiopid == cnt_opid || fdesc->adi_fiopid == any_opid)
			{
		    	/* count aggregate */

		    	cum->cum_ags.adf_agcnt += *((i4 *)(agg_res.db_data));
			}
			else
			{
		    	status = adf_agnext(Adf_scb, &agg_res, &(cum->cum_ags));
		    	if (status != OK)
		    	{
					FEafeerr(Adf_scb);
		    	}
			}
	    }

	    status = adf_agend(Adf_scb, &(cum->cum_ags), result);
	    if (status != OK)
	    {
			FEafeerr(Adf_scb);
	    }
	}
}
