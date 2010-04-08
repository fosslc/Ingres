/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
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
**   R_E_PAR - get the value for a parameter.
**
**	Parameters:
**		par	pointer to parameter.
**		value	pointer to the value of the parameter.
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
**		Syserr:NULL PAR pointer.
**
**	Trace Flags:
**		none.
**
**	History:
**		2/13/84 (gac)	written.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_e_par(par,value)
register PAR		*par;
register DB_DATA_VALUE	*value;
{
	/* start of routine */

	if (par == NULL)
	{
		r_syserr(E_RW0020_r_e_par_Null_PAR);
	}

	MEcopy((PTR)&(par->par_value), (u_i2)sizeof(DB_DATA_VALUE), (PTR)value);
}
