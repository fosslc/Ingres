/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpusnum.c	30.1	11/14/84"; */

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

/*
**   R_P_USNUM - set up a TCMD structure for one of the RTEXT commands
**	which expect an absolute number as argument.
**	Commands in this category include .newline, .newpage, and .pl.
**	The general format is:
**		command [+|-] [n]
**	where both sign and n are optional.
**
**	Parameters:
**		code - TCMD code to use for this command.
**		valdef - value to give to default.
**		minval - minimumn allowable value.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		104.
**
**	Called by:
**		r_act_set.
**
**	Trace Flags:
**		3.0, 3.9.
**
**	History:
**		4/17/81 (ps) - written.
**		7/30/89 (elein) garnet
**			Change to build expressions into an item instead
**			of evaluating here and now.
**		9/6/89 (elein) garnet
**			Add badexpr check and give better msg
**              9/27/89 (elein) garnet
**                      Set the tcmd value here if the item is a constant
**                      of the correct type and set item type to I_NONE
**                      so that rxtcmd can skip evaluation.
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
r_p_usnum(code,valdef,minval)
ACTION	code;
i2	valdef;
i2	minval;
{
	/* internal declarations */

	i4	error = 0;		/* error return from r_g_long */
	i4	number;			/* temp storage of parameter value */
	ITEM		*item;
	STATUS		status;
	DB_DATA_VALUE	dbv;

	/* start of routine */

	item = (ITEM *)MEreqmem(0,sizeof(ITEM), TRUE, (STATUS *)NULL);
	status = r_g_expr(item, &dbv);

	if( status == BAD_EXP )
	{
		r_error( 1016,NONFATAL, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
		return;
	}
	Cact_tcmd->tcmd_code = code;
	Cact_tcmd->tcmd_item = item;
	/* No expression is ok here, use default
	 * If we have a constant and it is an integer
	 * 	use it and pretend there is no ITEM
	 */
	if (item->item_type == I_NONE)
	{
		Cact_tcmd->tcmd_val.t_v_long = valdef;
	}
	else if (item->item_type == I_CON)
	{
		if( item->item_val.i_v_con->db_datatype == DB_INT_TYPE)
		{
			item->item_type = I_NONE;
			Cact_tcmd->tcmd_val.t_v_long =
			*(i4 *)(item->item_val.i_v_con->db_data);
		}
	}
	if (r_g_skip() != TK_ENDSTRING)
	{	/* excess characters found */
		r_error(104, NONFATAL, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
		return;
	}
	return;
}
