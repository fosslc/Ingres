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
# include	<errw.h>
# include       <er.h>

/*
**   R_A_OPS - process a TCMD P_AOP structure, containing the address
**	of an ACC structure to aggregate upon.
**
**	Parameters:
**		acc - ACC structure containing accumulator to aggregate upon.
**
**	Returns:
**		none.
**
**	Called by:
**		r_x_tcmd.
**
**	Error Messages:
**		Syserr:Null ACC pointer.
**		Syserr:Bad aggregate type.
**
**	Trace Flags:
**		170, 174.
**
**	History:
**		5/16/81 (ps) - written.
**		2/8/84	(gac)	added aggregates of dates.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_a_ops(acc)
ACC	*acc;

{
	/* external declarations */
	ATT		 *r_gt_att();	/* (mmm) added declaration */

	/* internal declarations */

	register ATT	*att;		/* fast ptr to ATT */
	STATUS		status;

	/* start of routine */


	if (acc == NULL)
	{
		r_syserr(E_RW0008_r_a_ops_Null_ACC);
	}


	/* process the correct aggregate */

	att = r_gt_att(acc->acc_attribute);

	status = adf_agnext(Adf_scb, &(att->att_value), &(acc->acc_ags));
	if (status != OK)
	{
	    FEafeerr(Adf_scb);
	}

	return;
}
