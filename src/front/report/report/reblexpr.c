/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<ug.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<errw.h>

/*{
** Name:	r_e_blexpr() -	Evaluate A Boolean Expression.
**
** Description:
**	R_E_BLEXPR  -- Evaluate a boolean expression.
**
** Parameters:
**	exp		points to root of parse tree of expression.
**
** Returns:
**	boolvalue	boolean (and possibly null) value.
**
** Side Effects:
**	none.
**
** Trace Flags:
**	99.
**
** History:
**	12/20/83 (gac)	written.
**	6/13/85	 (ac)	bug # 5599 fixes. Break(columnname) function
**			now work for the first row of data. (.e.q.
**			for the first row of data, break(columnname)
**			always return TRUE.
**	8/2/89 (elein) B7333
**			Add parameter to reitem, TRUE, evaluate expr
**	22-oct-1992 (rdrane)
**			Set precision in ffdbv DB_DATA_VALUE just to be
**			safe.  Eliminate references to r_syserr() and use
**			IIUGerr() directly.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-oct-2009 (joea)
**          Use DB_BOO_LEN instead of sizeof(bool) for DB_BOO_TYPE fields.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_e_blexpr(exp, boolvalue)
EXP		*exp;
DB_DATA_VALUE	*boolvalue;
{
	i4		isnull;
	ATTRIB		ord;
	ATT		*r_gt_att();
	ITEM		item;
	STATUS		status;

	switch(exp->exp_opid)
	{
		case(OP_AND):
			if (exp->exp_operand[0].item_type != I_EXP ||
			    exp->exp_operand[1].item_type != I_EXP)
			{
				IIUGerr(E_RW001A_And_must_have_boolean,
					UG_ERR_FATAL,0);
			}

			r_nullbool(exp, FALSE, boolvalue);
			break;

		case(OP_OR):
			if (exp->exp_operand[0].item_type != I_EXP ||
			    exp->exp_operand[1].item_type != I_EXP)
			{
				IIUGerr(E_RW001B_Or_must_have_boolean,
					UG_ERR_FATAL,0);
			}

			r_nullbool(exp, TRUE, boolvalue);
			break;

		case(OP_NOT):
			if (exp->exp_operand[1].item_type != I_EXP)
			{
				IIUGerr(E_RW001C_Not_must_have_boolean,
					UG_ERR_FATAL,0);
			}

			r_e_blexpr(exp->exp_operand[1].item_val.i_v_exp,
				   boolvalue);
			status = adc_isnull(Adf_scb, boolvalue, &isnull);
			if (status != OK)
			{
				FEafeerr(Adf_scb);
			}

			if (!isnull)
			{   /* if not null, complement it */
				*((bool *)boolvalue->db_data) =
					TRUE - *((bool *)boolvalue->db_data);
			}
			break;

		case(OP_BREAK):
			if (exp->exp_operand[0].item_type != I_ATT)
			{
				IIUGerr(E_RW001D_Must_be_break_column,
					UG_ERR_FATAL,0);
			}

			ord = exp->exp_operand[0].item_val.i_v_att;

			boolvalue->db_datatype = DB_BOO_TYPE;
			boolvalue->db_length = DB_BOO_LEN;
			boolvalue->db_prec = 0;
			boolvalue->db_data =
				(PTR) r_ges_next((i2)boolvalue->db_length);

			*((bool *)(boolvalue->db_data)) =
			    (ord == A_REPORT) ? St_rep_break :
				(St_break == Ptr_brk_top ||
				 (St_brk_ordinal > 0 && St_brk_ordinal <=
					       r_gt_att(ord)->att_brk_ordinal));
			break;

		default:
			/* relational operators executed by ADF */

			item.item_type = I_EXP;
			item.item_val.i_v_exp = exp;
			r_e_item(&item, boolvalue, TRUE );
			break;
	}
}

/*{
** Name:	r_nullbool	Evaluates two boolean (possibly null) operands
**				and ANDs or ORs them to get a possibly null
**				result.
**
** Description:
**	This routine evaluates two boolean (possibly null) operands in an
**	expression tree, then ANDs or ORs them to get a possibly null result.
**	This procedure uses three-valued logic.
**
** Inputs:
**	exp		points to an expression tree node containing the two
**			operands of the AND or OR operation.
**
**	is_or		TRUE if operation is OR; FALSE if operation is AND.
**
** Outputs:
**	boolvalue	points to the resulting boolean value.
**
**	Returns:
**		none
**
** History:
**	1-apr-87 (grant)	implemented.
**	22-oct-1992 (rdrane)
**		Set precision in ffdbv DB_DATA_VALUE just to be safe.
*/

VOID
r_nullbool(exp, is_or, boolvalue)
EXP		*exp;
bool		is_or;
DB_DATA_VALUE	*boolvalue;
{
	DB_DATA_VALUE	dbdv[2];
	i4		multivalue[2];
	i4		isnull;
	i4		i;
	STATUS		status;

# define	NLL	-1

	for (i = 0; i < 2; i++)
	{
		r_e_blexpr(exp->exp_operand[i].item_val.i_v_exp, &dbdv[i]);

		status = adc_isnull(Adf_scb, &dbdv[i], &isnull);
		if (status != OK)
		{
			FEafeerr(Adf_scb);
		}

		if (isnull)
		{
			multivalue[i] = NLL;
		}
		else
		{
			multivalue[i] = *((bool *) dbdv[i].db_data);
		}
	}

	boolvalue->db_datatype = DB_BOO_TYPE;
	boolvalue->db_length = DB_BOO_LEN;
	boolvalue->db_prec = 0;
	AFE_MKNULL_MACRO(boolvalue);
	boolvalue->db_data = (PTR) r_ges_next((i2)boolvalue->db_length);

	if (multivalue[0] == is_or || multivalue[1] == is_or)
	{
		/* return TRUE if OR; return FALSE if AND */

		*((bool *) boolvalue->db_data) = is_or;
	}
	else if (multivalue[0] == NLL || multivalue[1] == NLL)
	{
		/* return null value */

		adc_getempty(Adf_scb, boolvalue);
	}
	else
	{
		/* return complement of is_or */

		*((bool *) boolvalue->db_data) = TRUE - is_or;
	}
}
