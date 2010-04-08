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
**   R_BRK_FIND - find the BRK structure associated with an ACC
**	structure.  Since the ACC structure contain the aggregating
**	break attribute, find that associated BRK structure for
**	filling in the CLR ptrs.
**
**	Parameters:
**		acc - ACC structure to use to find BRK structure.
**
**	Returns:
**		BRK structure ptr.
**
**	Called by:
**		r_clr_add.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		Syserr:Bad attribute ordinal.
**		Syserr:Bad break ordinal.
**
**	Trace Flags:
**		4.0, 4.8.
**
**	History:
**		5/10/81 (ps) - written.
**	12/21/89 (elein)_
**		Changed parameter to r_syserr to be
**		a pointer
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
*/



BRK	*
r_brk_find(acc)
ACC	*acc;

{

	/* internal declarations */

	register BRK	*brk;			/* fast brk pointer */

	/* start of routine */



	if (acc == NULL)
	{
		r_syserr(E_RW000B_r_brk_find_Null_ACC);
	}


	switch(acc->acc_break)
	{
		case(A_PAGE):
			return(Ptr_pag_brk);

		case(A_REPORT):
			return(Ptr_brk_top);

		default:
			if ((acc->acc_attribute>En_n_attribs) ||
			    (acc->acc_attribute==A_DETAIL))
			{
				r_syserr(E_RW000C_r_brk_find_Bad_attrib,
					&(acc->acc_attribute));
			}


			for(brk=Ptr_brk_top;brk!=NULL;brk=brk->brk_below)
			{	/* go through break linked list */
				if (brk->brk_attribute == acc->acc_break)
				{
					return(brk);
				}
			}
			r_syserr(E_RW000D_r_brk_find_Bad_break);
	}
	/* NOTREACHED */
}
