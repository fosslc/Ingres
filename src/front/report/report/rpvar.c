/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpvar.c	30.1	11/14/84"; */

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
**   R_P_VAR - set up an ITEM structure for a program variable.
**
**	Parameters:
**		name	name of program variable.
**		item	item returned by this routine.
**
**	Returns:
**		TRUE	if name is an actual program variable.
**		FALSE	if not.
**
**	Side Effects:
**		none.
**
**	Called By:
**		r_p_param.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		3/31/81 (ps) - written.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		12/20/83 (gac)	added item for expressions.
**		17-may-90 (sylviap)
**			Added page_width as a user variable. (#958)
*/



bool
r_p_var(name, item)
char	*name;
ITEM	*item;
{
	/* start of routine */

	switch(r_gt_vcode(name))
	{
	case(VC_PAGENUM):
		item->item_val.i_v_pv = &St_p_number;
		break;

	case(VC_LINENUM):
		item->item_val.i_v_pv = &St_l_number;
		break;

	case(VC_POSNUM):
		item->item_val.i_v_pv = &St_pos_number;
		break;

	case(VC_LEFTMAR):
		item->item_val.i_v_pv = &St_left_margin;
		break;

	case(VC_RIGHTMAR):
		item->item_val.i_v_pv = &St_right_margin;
		break;

	case(VC_PAGELEN):
		item->item_val.i_v_pv = &St_p_length;
		break;

	case(VC_PAGEWDTH):
		item->item_val.i_v_pv = &En_lmax;
		break;

	default:
		return(FALSE);
	}

	item->item_type = I_PV;

	return(TRUE);
}
