/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<ug.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<errw.h>

/*
**   R_A_CACCS - clear out the accumulators in the ACC structures in a
**	TCMD structure with the P_ACLEAR code.	These are in the header
**	texts of the breaks, put there by r_lnk_agg.  Before clearing out
**	the accumulator (using the preset structure), however, the
**	accumulator is "opped" with the accumulator on its right for
**	multiple level aggregations.  This is not performed if the
**	one to the right is not doubly linked (i.e. has a acc_above
**	link set).  This is not set on page breaks, or when presets are
**	used.
**
**	Parameters:
**		acc - ptr to ACC structure to clear out.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd.
**
**	Side Effects:
**		clears out some of the accumulators, "op's" on others.
**
**	Trace Flags:
**		170, 172.
**
**	History:
**		5/18/81 (ps) - written.
**		2/8/84	(gac)	added aggregates of dates.
**		17-dec-1987 (rdesmond)
**			added check for "any" agg to use "count" logic.
**		3/23/89 (martym)
**			Added code to coerce presets to the type of their 
**			associated attribute.
**		06/27/92 (dkh) - Updated to match new afe_ficoerce() interface.
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
r_a_caccs(acc)
ACC	*acc;

{
	/* internal declarations */

	DB_DATA_VALUE	*preset = NULL;
	ATT		*att;			/* ptr to an ATT */
	ATT		*r_gt_att();		/* (mmm) added */
	DB_DATA_VALUE	dbv;
	DB_DATA_VALUE	tdbv;
	STATUS		status;
	ADI_FI_DESC	*fdesc;
	ADI_OP_ID	cnt_opid;
	ADI_OP_ID	any_opid;
	ADI_FI_ID  fid;
	i4  length;
	AFE_OPERS ops;
	

	/* start of routine */



	if (acc == NULL)
	{
		IIUGerr(E_RW0002_r_a_caccs_Null_ACC,UG_ERR_FATAL,0);
	}

	/* get the preset value */

	if (acc->acc_preset != NULL)
	{
		switch(acc->acc_preset->prs_type)
		{
		case(PT_CONSTANT):
			preset = &(acc->acc_preset->prs_pval.pval_constant);
			break;

		case(PT_ATTRIBUTE):
			att = r_gt_att(acc->acc_preset->prs_pval.pval_ordinal);
			preset = &(att->att_value);
			break;
		}
	}

	afe_opid(Adf_scb, ERx("count"), &cnt_opid);
	afe_opid(Adf_scb, ERx("any"), &any_opid);
	adi_fidesc(Adf_scb, acc->acc_ags.adf_agfi, &fdesc);

	/*
	** If the accumulator to the right is not doubly linked
	** (i.e. has a acc_above link set), this accumulator is "opped" with
	** the accumulator on its right for multiple level aggregations,
	** but only if this accumulator has aggregated some data values.
	*/

	if (acc->acc_below != NULL && acc->acc_below->acc_above != NULL &&
	    acc->acc_ags.adf_agcnt > 0)
	{
	    dbv.db_datatype = acc->acc_datatype;
	    dbv.db_length = acc->acc_length;
	    dbv.db_prec = acc->acc_prec;
	    dbv.db_data = MEreqmem(0,dbv.db_length,TRUE,NULL);

	    status = adf_agend(Adf_scb, &(acc->acc_ags), &dbv);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
	    }

	    if (fdesc->adi_fiopid == cnt_opid)
	    {
		/* count aggregate */
		acc->acc_below->acc_ags.adf_agcnt += *((i4 *)(dbv.db_data));
	    }
	    else if (fdesc->adi_fiopid == any_opid)
	    {
		/* any aggregate */
		if (acc->acc_below->acc_ags.adf_agcnt == 0)
		  acc->acc_below->acc_ags.adf_agcnt = *((i4 *)(dbv.db_data));
	    }
	    else
	    {
		status = adf_agnext(Adf_scb, &dbv, &(acc->acc_below->acc_ags));
		if (status != OK)
		{
		    FEafeerr(Adf_scb);
		}
	    }

	    MEfree(dbv.db_data);
	}

	/* now clear out the accumulator */

	status = adf_agbegin(Adf_scb, &(acc->acc_ags));
	if (status != OK)
	{
	    FEafeerr(Adf_scb);
	}

	/* put in preset value if there is one */

	if (preset != NULL)
	{
	    if (fdesc->adi_fiopid == cnt_opid)
	    {
		/* count aggregate */

		acc->acc_ags.adf_agcnt = *((i4 *)(preset->db_data));
	    }
	    else
	    {

		att = r_gt_att(acc->acc_attribute);
		tdbv.db_datatype = att->att_value.db_datatype;
		tdbv.db_length = 0;
		tdbv.db_prec = 0;
		/*
		**  The result length from afe_ficoerce does not
		**  appear to be used.  So I've left it alone
		**  after returning from afe_ficoerce().  (dkh)
		*/
		status = afe_ficoerce(Adf_scb, preset, &tdbv, 
				&fid);
		if (status != OK)
		{
		    FEafeerr(Adf_scb);
		}
		ops.afe_opcnt = 1;
		ops.afe_ops = &(preset);
	    dbv.db_datatype = att->att_value.db_datatype;
	    dbv.db_length = att->att_value.db_length;
	    dbv.db_prec = att->att_value.db_prec;
	    dbv.db_data = MEreqmem(0,dbv.db_length,TRUE,NULL);
		status = afe_clinstd(Adf_scb, fid, &ops, &dbv);
		if (status != OK)
		{
		    FEafeerr(Adf_scb);
		}

		status = adf_agnext(Adf_scb, &dbv, &(acc->acc_ags));
		if (status != OK)
		{
		    FEafeerr(Adf_scb);
		}
	    MEfree(dbv.db_data);
	    }
	}

	return;
}
