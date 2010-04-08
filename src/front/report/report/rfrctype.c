/*
** Copyright (c) 2004 Ingres Corporation
*/


/* static char	Sccsid[] = "@(#)rfrctype.c	30.1	11/14/84"; */

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
**   R_FRC_ETYPE - set an item to the given datatype if it is a parameter.
**
**	Parameters:
**		item		pointer to item.
**		exp_type	datatype of expression.
**
**	Returns:
**		none.
**
**	Called by:
**		r_g_expr.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		1/31/84 (gac)	written.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_frc_etype(item, exp_type)
ITEM		*item;
DB_DATA_VALUE	*exp_type;
{
	if (item->item_type == I_PAR)
	{
		r_par_type(item->item_val.i_v_par->par_name, exp_type);
	}
}
