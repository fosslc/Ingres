/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rpatt.c	30.1	11/14/84"; */

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
**   R_P_ATT - add a reference to an attribute to the TCMD structure.  This
**	is used to refer to the current (or previous) value of the attribute.
**	Also note that ptrs to the current data value are used in HEADER text,
**	and ptrs to the previous data value is used in FOOTER text.
**
**	Parameters:
**		ordinal - the ordinal of the attribute to print.
**		item	pointer to item set up by this procedure.
**		type	datatype of attribute.
**
**	Return:
**		status:
**			GOOD_EXP	attribute was found.
**			NO_EXP		attribute was not found.
**			BAD_EXP		error occurred.
**
**	Side Effects:
**		May update the value of Tokchar.
**
**	Called By:
**		r_p_param.
**
**	Error messages:
**		184.
**
**	Trace Flags:
**		3.0, 3.7.
**
**	History:
**		4/21/81 (ps) - written.
**		5/10/83 (nl) - added changes since code was ported
**			       to UNIX.
**			       Modified to change references to P_CATT and
**			       P_PATT into P_ATT to fix bug 937.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		12/20/83 (gac)	added item for expressions.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures. Converted r_error() err_num values to hex
**			to facilitate lookup in errw.msg file.
**			Removed declaration of r_gt_att(), since already in
**			hdr file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
r_p_att(ordinal, item, type)
ATTRIB		ordinal;
ITEM		*item;
DB_DATA_VALUE	*type;
{
	/* internal declarations */

	register ATT	*attribute;		/* fast ptr to ATT structure */
	ATTRIB		ord;			/* return from r_grp_nxt */
	DB_DT_ID	last_type = DB_NODT;
	i4		status = NO_EXP;

	/* start of routine */

	if (!r_grp_set(ordinal))
	{	/* specified W_COLUMN not in .WI block */
		r_error(0xB8, NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return(BAD_EXP);
	}

	item->item_type = I_ATT;
	item->item_val.i_v_att = ordinal;

	while ((ord = r_grp_nxt()) > 0)
	{
		status = GOOD_EXP;
		attribute = r_gt_att(ord);
		type->db_datatype = attribute->att_value.db_datatype;
		type->db_length = attribute->att_value.db_length;
		type->db_prec = attribute->att_value.db_prec;
		if (last_type != DB_NODT && last_type != type->db_datatype)
		{
			type->db_datatype = DB_NODT;
			break;
		}

		last_type = type->db_datatype;
	}


	return(status);
}
