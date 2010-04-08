/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rptparam.c	30.1	11/14/84"; */

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
**   R_P_TPARAM - reference a runtime parameter.
**	A runtime parameter is specified as in:
**		$paramname
**
**	Parameters:
**		name			string containing name of parameter.
**		preceded_by_dollar	if TRUE, name was preceded by '$'.
**					if FALSE, name was wasn't preceded by
**					'$' so it might be a declared variable.
**					If it doesn't exist, return NO_EXP.
**		item			pointer to item containing pointer to
**					parameter in parameter table.
**		type			datatype of parameter.
**
**	Returns:
**		status:
**			GOOD_EXP	parameter or declared variable found.
**			NO_EXP		declared variable doesn't exist.
**
**	Called by:
**		r_g_expr.
**
**	Side effects:
**		none.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		7/7/81 (ps)	written.
**		6/24/83 (gac)	added date templates & numeric formats.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		12/15/83 (gac)	reference runtime parameter in an expression.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jun-2007 (kibro01) b118486
**	    Note this isn't CLI when setting parameter
*/

i4
r_p_tparam(name, preceded_by_dollar, item, type)
char		*name;
bool		preceded_by_dollar;
ITEM		*item;
DB_DATA_VALUE	*type;
{
	PAR	*par;
	PAR	*r_par_put();

	/* start of routine */

	r_par_clip(name);
	if (!r_par_find(name, &par))
	{
		if (preceded_by_dollar)
		{
			par = r_par_put(name, NULL, NULL, NULL, FALSE);
		}
		else
		{
			return NO_EXP;
		}
	}
	else if (!preceded_by_dollar && !par->par_declared)
	{
		return NO_EXP;
	}

	item->item_type = I_PAR;
	item->item_val.i_v_par = par;

	type->db_datatype = par->par_value.db_datatype;
	type->db_length = par->par_value.db_length;
	type->db_prec = par->par_value.db_prec;

	return GOOD_EXP;
}
