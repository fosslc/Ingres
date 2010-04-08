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
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**    R_MAK_TREE - make an expression parse tree from an operator and an item.
**		The item will be copied to the left subtree of the expression.
**		Then the item will be made to point to the expression.
**
**	Parameters:
**		operator	operator id.
**		item		item of an expression.
**
**	Returns:
**		pointer to expression parse tree made.
**
**	History:
**		12/19/83 (gac)	written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

EXP	*
r_mak_tree(operator, item)
ADI_OP_ID	operator;
ITEM		*item;
{
	EXP	*exp;

	exp = (EXP *) MEreqmem(0,(u_i4) sizeof(EXP),TRUE,(STATUS *) NULL);
	exp->exp_opid = operator;
	exp->exp_fid = ADI_NOFI;
		/* Used MEcopy to copy entire ITEM structure */
	MEcopy((PTR)item, (u_i2)sizeof(ITEM), (PTR)(&exp->exp_operand[0]));
	exp->exp_operand[1].item_type = I_NONE;

	item->item_type = I_EXP;
	item->item_val.i_v_exp = exp;

	return(exp);
}
