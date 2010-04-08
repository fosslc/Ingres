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
# include	<cm.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_P_SNUM - process an RTEXT command which has a parameter which
**	can be a signed number.  Commands such as .lm, .rm, and .newpage
**	fit this pattern.  The format is:
**		commmand [+|-] [n]
**	where the sign indicates that the number is relative to the
**	current value, and unsigned numbers indicate that an absolute
**	change is wanted.  For example .lm +4 means left margin four spaces
**	from the current position, whereas .rm 4 means right margin at column 4
**
**	Parameters:
**		code - code for this command.
**
**	Returns:
**		none.
**
**	Error Messages:
**		104.
**
**	Called by:
**		r_act_set.
**
**	Trace Flags:
**		3.0, 3.8.
**
**	History:
**		4/15/81 (ps) - written.
**		7/30/89 (elein) garnet
**			Set up to recognize expressions.  Set type
**			based on sign.  Set value to 1 or -1 
**		9/6/89 (elein) garnet
**			Add bad_expr check with better msg.
**			Don't set type = "default"  so indiscriminately
**		9/27/89 (elein) garnet
**			Set the tcmd value here if the item is a constant
**			of the correct type and set item type to I_NONE
**			so that rxtcmd can skip evaluation.
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
r_p_snum(code)
ACTION	code;
{
	/* internal declarations */

	i4	error = 0;		/* error flag */
	i4	number = 1;		/* return value from r_g_long */
	i2	type = B_DEFAULT;	/* type of specification */
	ITEM		*item;
	STATUS		status;
	DB_DATA_VALUE	dbv;


	Cact_tcmd->tcmd_val.t_v_vp =
		(VP *) MEreqmem(0,sizeof(VP),TRUE,(STATUS *) NULL);
	item = (ITEM *)MEreqmem(0,sizeof(ITEM), TRUE, (STATUS *)NULL);

	switch (r_g_skip())
	{

		case(TK_SIGN):
			type = B_RELATIVE;
			if( *Tokchar == '-') 
			{
				number = -1;
			}
			/* Skip Sign */
			CMnext(Tokchar);
			break;
		default:
			type = B_ABSOLUTE;
			break;
	}


	status = r_g_expr(item, &dbv);
	if( status == BAD_EXP )
	{
		r_error( 1016,NONFATAL, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
		return;
	}

	if (item->item_type == I_NONE)
	{
		type = B_DEFAULT;
	}
	else if (item->item_type == I_CON )
	{
		if( item->item_val.i_v_con->db_datatype == DB_INT_TYPE)
		{
			item->item_type = I_NONE;
			number *= *(i4 *)(item->item_val.i_v_con->db_data);
		}
	}
	if (r_g_skip() != TK_ENDSTRING)
	{	/* excess characters found */
		r_error(104, NONFATAL, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
		return;
	}
	Cact_tcmd->tcmd_code = code;
	Cact_tcmd->tcmd_item = item;
	(Cact_tcmd->tcmd_val.t_v_vp)->vp_type = type;
	(Cact_tcmd->tcmd_val.t_v_vp)->vp_value = number;
	return;


}
