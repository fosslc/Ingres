/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<ug.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<cm.h>
# include	<er.h>
# include	<errw.h>
# include	<afe.h>

char	nullargument[] = ERx(" ");

/*
**	R_G_EXPR  --  Reads in an expression from RTEXT,
**		and generates a parse tree for the expression.
**
**
**	Parameters:
**		ptree		points to parse tree generated.
**		exp_type	datatype of expression
**
**	Returns:
**		status:
**			GOOD_EXP	a legal expression found
**			NO_EXP		no expression found
**			BAD_EXP		error
**			NULL_EXP	"null" found
**
**	Side Effects:
**		none.
**
**	Error messages:
**		500, 501, 502, 503.
**
**	Trace Flags:
**		none.
**
**	History:
**		12/19/83 (gac)	written.
**		2/13/86 (mgw) Fix bug 7966 - Strip backslashes in DATE_EXP
**				and STR_EXP cases to compensate for not having
**				stripped it in r_g_string in order to make
**				r_wild_card recognize literal wild card chars.
**		15-may-86 (mgw) bug #'s 9375, 9312, 8838
**				Corrected previous fix. - Remember to end with
**				a '\0' when stripping backslashes (also spell
**				stripped correctly in variable names).
**		27-oct-86 (mgw) bug #10676
**				added new routine, r_strpslsh, to strip slashes
**				from a string, a function no longer performed
**				by r_g_string. Called from r_g_expr and from
**				r_crk_par.
**		23-mar-1987 (yamamoto)
**			Modified for double byte characters.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		23-oct-1992 (rdrane)
**			Eliminate references to r_syserr() and use IIUGerr()
**			directly.
**		17-may-1993 (rdrane)
**			Add support for hex literals.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need afe.h to satisfy gcc 4.3.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

i4
r_g_expr(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	ADI_OP_ID	op;
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_logop();
	i4		status;

	status = r_g_blprimary(ptree, exp_type);
	if (status == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while ((op = r_g_logop()) == OP_AND || op == OP_OR)
	{
		if (exp_type->db_datatype != DB_BOO_TYPE)
		{
			IIUGerr(E_RW0029_Expected_boolean_expr,UG_ERR_FATAL,0);
		}

		exp = r_mak_tree(op, ptree);
		status = r_g_blprimary(&(exp->exp_operand[1]), exp_type);
		if ((status == GOOD_EXP &&
		     exp_type->db_datatype != DB_BOO_TYPE) ||
		    status == NO_EXP)
		{
			IIUGerr(E_RW0029_Expected_boolean_expr,UG_ERR_FATAL,0);
		}
		else if (status == BAD_EXP)
		{
			return(BAD_EXP);
		}
	}

	if (ptree->item_type == I_CON &&
	    ptree->item_val.i_v_con->db_datatype == DB_CHR_TYPE)
	{
		/* strip out backslashes */
		r_strpslsh(ptree->item_val.i_v_con->db_data);
		ptree->item_val.i_v_con->db_length =
				STlength(ptree->item_val.i_v_con->db_data);
	}

	return(status);
}

/*
** r_g_blprimary
** History:
**
**	23-oct-1992 (rdrane)
**		Eliminate references to r_syserr() and use IIUGerr()
**		directly.
*/
i4
r_g_blprimary(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	bool		saw_not = FALSE;
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_logop();
	i4		status;

	if (r_g_logop() == OP_NOT)
	{
		saw_not = TRUE;
		ptree->item_type = I_NONE;
		ptree->item_val.i_v_exp = NULL;
		exp = r_mak_tree(OP_NOT, ptree);
		status = r_g_blelement(&(exp->exp_operand[1]), exp_type);
	}
	else
	{
		status = r_g_blelement(ptree, exp_type);
	}

	if (saw_not && ((status == GOOD_EXP &&
			 exp_type->db_datatype != DB_BOO_TYPE) ||
			status == NO_EXP))
	{
		IIUGerr(E_RW0029_Expected_boolean_expr,UG_ERR_FATAL,0);
	}

	return(status);
}


/*
**	R_G_BLELEMENT -
**
**
**	Parameters:
**		ptree		points to parse tree generated.
**		exp_type	datatype of expression.
**
**	Returns:
**		status:
**			GOOD_EXP	a legal expression found
**			NO_EXP		no expression found
**			BAD_EXP		error
**			NULL_EXP	"null" found
**
**	Side Effects:
**		none.
**
**	Error messages:
**
**	Trace Flags:
**		none.
**
**	History:
**		12/19/83 (gac)	written.
**		10/29/85 (mgw)	bug # 6554 - allow wild card expansion of string
**				operands separated by TK_RELOP operators { !=,
**				>=, >, <, <= } too, not just by TK_EQUALS { = }.
**		1/20/86	 (mgw)	Bug # 6039 - If a comparison is being done
**				between a date and a string, make a date
**				expression out of the string.
**		23-oct-1992 (rdrane)
**			Set/propagate precision to exp_type DB_DATA_VALUE.
**			Converted r_error() err_num values to hex to
**			facilitate lookup in errw.msg file.
**			Eliminate references to r_syserr() and use IIUGerr()
**			directly.
*/

i4
r_g_blelement(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	i4		type;
	DB_DATA_VALUE	prev_type;
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_op();
	i4		status;
	char		isnullstr[20];
	char		*token;
	char		*r_g_name();
	char		*save_Tokchar;
	ADI_OP_ID	opid;

	status = r_g_aexp(ptree, exp_type);
	if (status == BAD_EXP)
	{
		return(BAD_EXP);
	}

	type = r_g_eskip();
	if (type == TK_EQUALS || type == TK_RELOP)
	{
		if (status == NULL_EXP)
		{
			r_error(0x43, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		prev_type.db_datatype = exp_type->db_datatype;
		prev_type.db_length = exp_type->db_length;
		prev_type.db_prec = exp_type->db_prec;
		exp = r_mak_tree(r_g_op(), ptree);
		status = r_g_aexp(&(exp->exp_operand[1]), exp_type);
		switch (status)
		{
		case NO_EXP:
			IIUGerr(E_RW002A_expected_operand,UG_ERR_FATAL,0);

		case BAD_EXP:
			return(BAD_EXP);

		case NULL_EXP:
			r_error(0x43, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);

		case GOOD_EXP:
			r_frc_etype(&(exp->exp_operand[0]), exp_type);
			r_frc_etype(&(exp->exp_operand[1]), &prev_type);

			r_wild_card(&(exp->exp_operand[0]));
			r_wild_card(&(exp->exp_operand[1]));

			exp_type->db_datatype = DB_BOO_TYPE;
			exp_type->db_length = 1;
			exp_type->db_prec = 0;
			break;
		}
	}
	else if (type == TK_ALPHA)
	{
	    /* check if "IS NULL" or "IS NOT NULL" follows */
	    save_Tokchar = Tokchar;
	    token = r_g_name();
	    CVlower(token);
	    if (STequal(token, ERx("is")) )
	    {
		STcopy(ERx(""), isnullstr);
		STcat(isnullstr, token);
		STcat(isnullstr, ERx(" "));
		type = r_g_skip();
		if (type == TK_ALPHA)
		{
		    token = r_g_name();
		    CVlower(token);
		    if (STequal(token, ERx("not")) )
		    {
			STcat(isnullstr, token);
			STcat(isnullstr, ERx(" "));
			type = r_g_skip();
			if (type == TK_ALPHA)
			{
			    token = r_g_name();
			    CVlower(token);
			}
		    }

		    if (STequal(token, ERx("null")) )
		    {
			STcat(isnullstr, token);
			afe_opid(Adf_scb, isnullstr, &opid);
			r_mak_tree(opid, ptree);
			exp_type->db_datatype = DB_BOO_TYPE;
			exp_type->db_length = 1;
			exp_type->db_prec = 0;
			return GOOD_EXP;
		    }
		    else
		    {
			IIUGerr(E_RW002B_expected_IS_NULL,UG_ERR_FATAL,0);
		    }
		}
		else
		{
		    IIUGerr(E_RW002B_expected_IS_NULL,UG_ERR_FATAL,0);
		}
	    }

	    /* did not match, back up */
	    Tokchar = save_Tokchar;
	}


	return(status);
}

/*
** r_g_aexp
**
** History:
**	5-mar-1992 (mgw) Bugs 38991, 31450, 41778, 42710
**		Moved checking for need to strip backslashes in concatinated
**		strings from r_g_aelement() where it mistakenly stripped
**		backslashes from all strings to here where it just does it
**		where it's needed - for concatinated strings.
**	23-oct-1992 (rdrane)
**		Set/propagate precision to exp_type DB_DATA_VALUE.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
**		Eliminate references to r_syserr() and use IIUGerr()
**		directly.
*/

i4
r_g_aexp(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	DB_DATA_VALUE	prev_type;
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_op();
	i4		status;
	ITEM		*tmp0_item;
	ITEM		*tmp1_item;

	status = r_g_arterm(ptree, exp_type);
	if (status == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while (r_g_eskip() == TK_SIGN)
	{
		if (status == NULL_EXP)
		{
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		prev_type.db_datatype = exp_type->db_datatype;
		prev_type.db_length = exp_type->db_length;
		prev_type.db_prec = exp_type->db_prec;
		exp = r_mak_tree(r_g_op(), ptree);
		status = r_g_arterm(&(exp->exp_operand[1]), exp_type);
		switch (status)
		{
		case NO_EXP:
			IIUGerr(E_RW002A_expected_operand,UG_ERR_FATAL,0);

		case BAD_EXP:
			return(BAD_EXP);

		case NULL_EXP:
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);

		case GOOD_EXP:
			tmp0_item = &(exp->exp_operand[0]);
			tmp1_item = &(exp->exp_operand[1]);
			r_frc_etype(tmp0_item, exp_type);
			r_frc_etype(tmp1_item, &prev_type);
			status = r_preferable(&prev_type, exp_type, exp_type);
			break;
		}

		/* strip backslashes if necessary - bugs 38991, 41778 etc. */
		if (tmp0_item->item_type == I_CON &&
			tmp0_item->item_val.i_v_con->db_datatype == DB_CHR_TYPE)
		{
			r_strpslsh(tmp0_item->item_val.i_v_con->db_data);
			tmp0_item->item_val.i_v_con->db_length =
				STlength(tmp0_item->item_val.i_v_con->db_data);
		}
		if (tmp1_item->item_type == I_CON &&
			tmp1_item->item_val.i_v_con->db_datatype == DB_CHR_TYPE)
		{
			r_strpslsh(tmp1_item->item_val.i_v_con->db_data);
			tmp1_item->item_val.i_v_con->db_length =
				STlength(tmp1_item->item_val.i_v_con->db_data);
		}
	}

	return(status);
}

/*
** r_g_arterm
**
** History:
**	23-oct-1992 (rdrane)
**		Set/propagate precision to exp_type DB_DATA_VALUE.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
*/
i4
r_g_arterm(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	DB_DATA_VALUE	prev_type;
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_op();
	i4		status;

	status = r_g_arfactor(ptree, exp_type);
	if (status == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while (r_g_eskip() == TK_MULTOP)
	{
		if (status == NULL_EXP)
		{
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		prev_type.db_datatype = exp_type->db_datatype;
		prev_type.db_length = exp_type->db_length;
		prev_type.db_prec = exp_type->db_prec;
		exp = r_mak_tree(r_g_op(), ptree);
		status = r_g_arfactor(&(exp->exp_operand[1]), exp_type);
		switch (status)
		{
		case NO_EXP:
			IIUGerr(E_RW002A_expected_operand,UG_ERR_FATAL,0);

		case BAD_EXP:
			return(BAD_EXP);

		case NULL_EXP:
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);

		case GOOD_EXP:
			r_frc_etype(&(exp->exp_operand[0]), exp_type);
			r_frc_etype(&(exp->exp_operand[1]), &prev_type);
			status = r_preferable(&prev_type, exp_type, exp_type);
			break;
		}
	}

	return(status);
}

/*
** r_g_arfactor
**
** History:
**	23-oct-1992 (rdrane)
**		Set/propagate precision to exp_type DB_DATA_VALUE.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
**		Eliminate references to r_syserr() and use IIUGerr()
**		directly.
*/
i4
r_g_arfactor(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	DB_DATA_VALUE	prev_type;
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_op();
	i4		status;

	status = r_g_arprimary(ptree, exp_type);
	if (status == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while (r_g_eskip() == TK_POWER)
	{
		if (status == NULL_EXP)
		{
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		prev_type.db_datatype = exp_type->db_datatype;
		prev_type.db_length = exp_type->db_length;
		prev_type.db_prec = exp_type->db_prec;
		exp = r_mak_tree(r_g_op(), ptree);
		status = r_g_arprimary(&(exp->exp_operand[1]), exp_type);
		switch (status)
		{
		case NO_EXP:
			IIUGerr(E_RW002A_expected_operand,UG_ERR_FATAL,0);

		case BAD_EXP:
			return(BAD_EXP);

		case NULL_EXP:
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);

		case GOOD_EXP:
			r_frc_etype(&(exp->exp_operand[0]), exp_type);
			r_frc_etype(&(exp->exp_operand[1]), &prev_type);
			exp_type->db_datatype = prev_type.db_datatype;
			exp_type->db_length = prev_type.db_length;
			exp_type->db_prec = prev_type.db_prec;
			break;
		}
	}

	return(status);
}

/*
** r_g_arprimary
**
** History:
**	23-oct-1992 (rdrane)
**		Set/propagate precision to exp_type DB_DATA_VALUE.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
*/
i4
r_g_arprimary(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	EXP		*exp;
	EXP		*r_mak_tree();
	ADI_OP_ID	r_g_op();
	DB_DATA_VALUE	*dbv;
	i4		status;

	if (r_g_eskip() == TK_SIGN)
	{
		ptree->item_type = I_CON;
		dbv = (DB_DATA_VALUE *) MEreqmem(0,
						(u_i4) sizeof(DB_DATA_VALUE),
						TRUE, (STATUS *) NULL);
		ptree->item_val.i_v_con = dbv;
		dbv->db_datatype = DB_INT_TYPE;
		dbv->db_length = sizeof(i4);
		dbv->db_prec = 0;
		dbv->db_data = (PTR) MEreqmem(0, (u_i4) sizeof(i4),
						TRUE, (STATUS *) NULL);  
		*((i4 *)(dbv->db_data)) = 0;
		exp = r_mak_tree(r_g_op(), ptree);
		status = r_g_aelement(&(exp->exp_operand[1]), exp_type);
		switch (status)
		{
		case NO_EXP:
			IIUGerr(E_RW002A_expected_operand,UG_ERR_FATAL,0);

		case BAD_EXP:
			return(BAD_EXP);

		case NULL_EXP:
			r_error(0x44, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}
	}
	else
	{
		status = r_g_aelement(ptree, exp_type);
		if (status == BAD_EXP)
		{
			return(BAD_EXP);
		}
	}

	return(status);
}


/*
**	r_g_aelement
**
**	History:
**		2/13/86 - (mgw) Fix bug number 7966 - pass new argument to
**				r_g_string to tell it not to strip backslashes
**				so that r_wild_card can distinguish between
**				wild card literals and genuine wild cards.
**
**		6-aug-1986 (Joe)
**			Added support for single quotes.  Passed kind
**			of quote to r_g_string.	 Also took out strip slash
**			parameter.
**		8/5/91 (elein) 38991
**			Another chapter to the strip slashing saga:
**			Don't forget to do it when the constant string
**			is a part of an expression.
**		5-mar-1992 (mgw) Bug #s 38991, 31450, 41778, 42710
**			Move Elein's 8/5/91 fix for 38991 up into r_g_aexp()
**			to only get it when concatinating constant strings.
**		23-oct-1992 (rdrane)
**			Set/propagate precision to exp_type DB_DATA_VALUE.
**			Eliminate references to r_syserr() and use IIUGerr()
**			directly.
**		9-dec-1992 (rdrane)
**			Fix-up parameterization of r_g_double() to support
**			interpretation of DECIMAL numeric constants, as well as
**			setting of the DB_DATA_VALUE.
**		4-jan-1993 (rdrane)
**			Correct TK_QUOTE processing to recognize delimited
**			identifiers.
**		17-may-1993 (rdrane)
**			Add support for hex literals.  Remove unused num_digs,
**			numprec, number, isinteger, and isdecimal.
**		06-apr-1999 (toumi01)
**			Modify the fix for bug 3845 to allocate the default
**			string " " (in the case of a null argument) from
**			variable storage rather than from the literal pool,
**			since we may overwrite it while checking for wild
**			cards.  This avoids the SIGSEGV report for Linux and
**			Data General, bug 94085.
*/

i4
r_g_aelement(ptree, exp_type)
ITEM		*ptree;
DB_DATA_VALUE	*exp_type;
{
	char		*token;
	char		*r_g_name();
	i4		tokvalue;
	i4		hex_chars;
	char		*str;
	DB_DATA_VALUE	*dbv;
	i4		status = GOOD_EXP;

	switch(tokvalue = r_g_eskip())
	{
	case(TK_QUOTE):
	case(TK_SQUOTE):
		if  ((tokvalue == TK_QUOTE) && (St_xns_given))
		{
			token = r_g_ident(TRUE);
			status = r_p_param(token, ptree, exp_type);
			break;
		}
		str = r_g_string(tokvalue);
		ptree->item_type = I_CON;
		dbv = (DB_DATA_VALUE *) MEreqmem(0,
						(u_i4) sizeof(DB_DATA_VALUE),
						TRUE, (STATUS *) NULL);  
		ptree->item_val.i_v_con = dbv;
		exp_type->db_datatype = dbv->db_datatype = DB_CHR_TYPE;
		exp_type->db_length = dbv->db_length = STlength(str);
		exp_type->db_prec = 0;
		dbv->db_data = (PTR) str;
		if (dbv->db_length == 0)
		/* fix for bug 3845 (0 length chars not allowed by ADF) */
		{
			exp_type->db_length = dbv->db_length = 1;
			dbv->db_data = nullargument;
		}
		break;

	case (TK_HEXLIT):
		CMnext(Tokchar);
		if  ((hex_chars = r_g_hexlit((DB_TEXT_STRING **)&str)) == 0)
		{
			return(BAD_EXP);
		}
		ptree->item_type = I_CON;
		dbv = (DB_DATA_VALUE *) MEreqmem(0,
						(u_i4) sizeof(DB_DATA_VALUE),
						TRUE, (STATUS *) NULL);  
		ptree->item_val.i_v_con = dbv;
		exp_type->db_datatype = dbv->db_datatype = DB_VCH_TYPE;
		exp_type->db_length = dbv->db_length =
		     hex_chars + sizeof(((DB_TEXT_STRING *)str)->db_t_count);
		exp_type->db_prec = 0;
		dbv->db_data = (PTR)str;
		break;

	case(TK_OPAREN):
		CMnext(Tokchar);
		status = r_g_expr(ptree, exp_type);
		if (status == BAD_EXP)
		{
			return(BAD_EXP);
		}

		if (r_g_eskip() != TK_CPAREN)
		{
			IIUGerr(E_RW002C_expected_parenthesis,UG_ERR_FATAL,0);
		}

		CMnext(Tokchar);
		break;

	case(TK_NUMBER):
		dbv = (DB_DATA_VALUE *)MEreqmem((u_i4)0,
						(u_i4)sizeof(DB_DATA_VALUE),
						TRUE,(STATUS *)NULL);  
		if  (dbv == (DB_DATA_VALUE *)NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_g_aelement - dbv"));
		}
		if (r_g_double(dbv) != 0)
		{
			IIUGerr(E_RW002D_expected_legal_number,UG_ERR_FATAL,0);
		}
		ptree->item_type = I_CON;
		ptree->item_val.i_v_con = dbv;
		exp_type->db_datatype = dbv->db_datatype;
		exp_type->db_length = dbv->db_length;
		exp_type->db_prec = dbv->db_prec;
		break;

	case(TK_ALPHA):
		token = r_g_name();
		status = r_p_param(token, ptree, exp_type);
		break;

	case(TK_DOLLAR):
		CMnext(Tokchar);
		if (r_g_eskip() != TK_ALPHA)
		{
			IIUGerr(E_RW002E_expected_alphanumeric,UG_ERR_FATAL,0);
		}

		token = r_g_name();
		status = r_p_tparam(token, TRUE, ptree, exp_type);
		break;

	default:
		ptree = NULL;
		status = NO_EXP;
		break;
	}

	return(status);
}


/*
**	R_WILD_CARD - if an item is a string constant, the wild-card
**		characters within the string are converted to special
**		characters used by the lexcomp routine in evaluating
**		whether two strings are equal.
**
**	History:
**		2/13/86 - (mgw) - Fix bug 7966 - strip backslashes here instead
**				of in r_g_string so we can distinguish between
**				wild card literals and genuine wild cards here.
*/

VOID
r_wild_card(item)
ITEM	*item;
{
	register char	*s;		/* before string */
	register char	*t;		/* after string */
	register char	*send;		/* end of string */

	if (item->item_type == I_CON &&
	    item->item_val.i_v_con->db_datatype == DB_CHR_TYPE)
	{
		t = s = item->item_val.i_v_con->db_data;
		send = s + item->item_val.i_v_con->db_length;
		while (s < send)
		{
			switch (*s)
			{
			case '*':
				*s = DB_PAT_ANY;
				break;

			case '?':
				*s = DB_PAT_ONE;
				break;

			case '[':
				*s = DB_PAT_LBRAC;
				break;

			case ']':
				*s = DB_PAT_RBRAC;
				break;

			case '\\':
				s++;
				break;
			}

			CMcpyinc(s, t);
		}

		item->item_val.i_v_con->db_length -= (send - t);
	}
}


/*
**	R_STRPSLSH - Strip the backslashes out of a string. Thus ab\"cd\"ef
**		becomes ab"cd"ef and ab\\cd becomes ab\cd etc.
**
**	Parameters:
**		string - null-terminated string to strip backslashes from
**
**	Returns:
**		none.
**
**	Side Effects:
**		strips the backslashes from the string
**
**	History:
**		27-oct-86 (mgw) first created.
*/

VOID
r_strpslsh(string)
char	*string;
{
	register char	*unstripped;
	register char	*stripped;

	unstripped = stripped = string;
	while (*unstripped != EOS)
	{
		if (*unstripped == '\\')
			unstripped++;
		CMcpyinc(unstripped, stripped);
	}

	*stripped = EOS;
}

/*{
** Name:	r_preferable	Compares two datatypes and returns the
**				"preferable" one.
**
** Description:
**	This routine compares one datatype with another datatype and
**	returns the "preferable" one.
**	If the datatypes are not compatible an error is given.	Two datatypes
**	are compatible if one is coercible into the other. The "preferable"
**	datatype is the one to which the other gets coerced.
**
** Inputs:
**	type1		A datatype
**	type2		Another datatype
**
** Outputs:
**	pref_type	The preferable datatype of the two.
**	Returns:	If they are not compatible, BAD_EXP is returned.
**			Otherwise, GOOD_EXP is returned.
**
** History:
**	23-mar-87 (grant)	implemented.
**	23-oct-1992 (rdrane)
**		Ensure set/propagate precision in DB_DATA_VALUE structures.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
*/

i4
r_preferable(type1, type2, pref_type)
DB_DATA_VALUE	*type1;
DB_DATA_VALUE	*type2;
DB_DATA_VALUE	*pref_type;
{
    bool		cancoerce;
    DB_DATA_VALUE	*pdbv;
    DB_DATA_VALUE	*odbv;

    if (type1->db_datatype == DB_NODT)
    {
	pdbv = type2;
    }
    else if (type2->db_datatype == DB_NODT)
    {
	pdbv = type1;
    }
    else
    {
	if (afe_preftype(type1->db_datatype, type2->db_datatype))
	{   /* type1 is preferable to type2 */
	    pdbv = type1;
	    odbv = type2;
	}
	else
	{   /* type2 is preferable to type1 */
	    pdbv = type2;
	    odbv = type1;
	}

	afe_cancoerce(Adf_scb, odbv, pdbv, &cancoerce);
	if (!cancoerce)
	{
	    r_error(0x1F7, NONFATAL, Cact_tname, Cact_attribute, Cact_command,
		    Cact_rtext, NULL);
	    return BAD_EXP;
	}
    }

    pref_type->db_datatype = pdbv->db_datatype;
    pref_type->db_length = pdbv->db_length;
    pref_type->db_prec = pdbv->db_prec;

    return GOOD_EXP;
}
