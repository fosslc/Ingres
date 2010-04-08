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
# include	<ug.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
**   R_A_ACC - evaluate an ACC structure.
**
**	Parameters:
**		acc - the ACC structure to process.
**		result - pointer to the value of the aggregate.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_e_item.
**
**	Error messages:
**		Syserr:NULL acc pointer.
**
**	Trace Flags:
**		none.
**
**	History:
**		2/13/84 (gac)	written.
**		22-oct-1992 (rdrane)
**			Propagate precision to result DB_DATA_VALUE.
**			Eliminate references to r_syserr() and use
**			IIUGerr() directly.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_a_acc(acc,result)
register ACC		*acc;
register DB_DATA_VALUE	*result;
{
	/* internal declarations */

	ACC	*acct;
	STATUS	status;

	/* start of routine */

	if (acc == NULL)
	{
		IIUGerr(E_RW0001_r_a_acc_Null_ACC,UG_ERR_FATAL,0);
	}

	acct = r_ret_acc(acc->acc_attribute, acc->acc_break, acc->acc_preset,
			 acc->acc_ags.adf_agfi, acc->acc_unique);

	result->db_datatype = acct->acc_datatype;
	result->db_length = acct->acc_length;
	result->db_prec = acct->acc_prec;
	result->db_data = (PTR) r_ges_next(result->db_length);

	status = adf_agend(Adf_scb, &(acct->acc_ags), result);
	if (status != OK)
	{
		FEafeerr(Adf_scb);
	}
}
