/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_P_VPNUM - process an RTEXT command which has a parameter which
**	can be a signed number or a column name or blank.
**	Commands such as .tab, .center, .right and .left 
**	fit this pattern.  The format is:
**		commmand [+|-][n] | colname | W_COLUMN
**	where the sign indicates that the number is relative to the
**	current value, and unsigned numbers indicate that an absolute
**	change is wanted.  If no value is given, a default is done
**	at run time, and if a column name is given, then the value
**	from the ATT structure for that column is used.
**	For example .right +4 means right justify the next text four spaces
**	from the current position, whereas .tab 4 means tab to column 4.
**	.Center (no parameters) means to center about the center of the
**	page, and .left abc means to left justify the next text in
**	the position set up for column abc.
**
**	If the W_COLUMN is specified, check to make sure we are in
**	a .WI loop.  If so, ignore it and use default.  If in a 
**	.WI loop and a column name is specified, it must be W_COLUMN.
**	If a column name is specified in a .WI loop, it must be the
**	only column in the within loop.
**	
**	Parameters:
**		code - code for this command.
**
**	Returns:
**		none.
**
**	Error Messages:
**		103, 104, 130, 183, 184.
**
**	Called by:
**		r_act_set.
**
**	Trace Flags:
**		3.0, 3.8.
**
**	History:
**		7/15/81 (ps) - written.
**		8/3/89 (elein) garnet
**			change to build expressions into ITEM
**		9/6/89 (elein)
**			Add badexpr check, give better mssg,
**			allow no items for all commands
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
r_p_vpnum(code)
ACTION	code;
{
	/* internal declarations */

	i4	number = 1;		/* return value from r_g_long */
	i2	type = B_DEFAULT;	/* type of specification */
	char	*aname;			/* column name if column spec */
	ATTRIB	ord;			/* return from r_grp_nxt */
	ITEM		*item;
	STATUS		status;
	DB_DATA_VALUE	dbv;

	/* start of routine */


	Cact_tcmd->tcmd_val.t_v_vp =
		(VP *) MEreqmem(0,sizeof(VP),TRUE,(STATUS *) NULL);
	item = (ITEM *)MEreqmem(0,sizeof(ITEM), TRUE, (STATUS *)NULL);
	item->item_type = I_NONE;

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
		case(TK_ENDSTRING):
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
	/* Tab requires argument, ce,ri,le dont */
	if (item->item_type == I_NONE )
	{
		type = B_DEFAULT;
	}

	/*
	 * IF ITEM is an attribute, get ordinal already
	 */
	else if( item->item_type == I_ATT)
	{

		/* We already have the ordinal in item->i_v_att */
		number = item->item_val.i_v_att;
		if(!r_grp_set((i2)number))
		{	/* FALSE if W_COLUMN outside of .WI loop */
			r_error(184, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext,NULL);
			return;
		}

		if (St_cwithin == NULL)
		{	/* no .WITHIN block */
			type = B_COLUMN;
		}
		else
		{	/* .WITHIN block */
			r_grp_set(A_GROUP);
			while(((ord=r_grp_nxt())>0)&&(number!=A_GROUP))
			{	/* only ordinal same as on TAB allowed */
				if (number != ord)
				{
					r_error(183, NONFATAL, Cact_tname, 
						Cact_attribute, Cact_command,
						Cact_rtext, NULL);
					return;
				}
			}
			type = B_DEFAULT;
			number = 1;
		}
	}
	else if( item->item_type == I_CON &&
	          item->item_val.i_v_con->db_datatype == DB_INT_TYPE)
	{
		item->item_type = I_NONE;
		number *= *(i4 *)(item->item_val.i_v_con->db_data);
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
