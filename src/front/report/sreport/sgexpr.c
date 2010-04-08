/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<st.h>
# include	<stype.h> 
# include	<sglob.h> 
# include	<er.h>

/*
**	S_G_EXPR  --  Reads in an expression from RTEXT.
**
**
**	Parameters:
**		None.
**
**	Returns:
**		type of expression:
**			NO_EXP		no expression found
**			BAD_EXP		error
**			any DB_DT_ID datatype values
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		11/18/83 (gac)	written.
**		1/22/85	(gac)	fixed bug #4907 -- RBF trim >98 characters
**				no longer gets syserr if Filed and then
**				SREPORTed.
**		08/06/86 (drh)	Added TK_SQUOTE, and new param to s_g_string.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		11-apr-90 (cmr)
**			If the expr token is the string RBFAGGS then s_cat a 
**			blank after it so that the next token (the agg #)
**			does not get concatenated with RBFAGGS in Ctext.
**		4/27/90 (martym)
**			Added code to handle cases where the string is 
**			RBFSETUP to ensure that if there is any data source 
**			information following it, it is concatenated 
**			correctlly.
**		11-oct-90 (sylviap)
**			Added new paramter to s_g_skip.
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Fix-up external declarations -
**			they've been moved to hdr files.  Actually, they're all
**			static to this module!
**		17-may-1993 (rdrane)
**			Add support for hex literals.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static	DB_DT_ID	s_g_blprimary();
static	DB_DT_ID	s_g_blelement();
static	DB_DT_ID	s_g_aexp();
static	DB_DT_ID	s_g_arterm();
static	DB_DT_ID	s_g_arfactor();
static	DB_DT_ID	s_g_arprimary();
static	DB_DT_ID	s_g_aelement();

DB_DT_ID
s_g_expr()
{
	DB_DT_ID	exp_type;
	char		*token;
	ADI_OP_ID	op;

 	exp_type = s_g_blprimary();
	if (exp_type == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while ((op = s_g_logop(&token)) == OP_AND || op == OP_OR)
	{
		if (exp_type != DB_BOO_TYPE)
		{
			s_error(0x3D4, NONFATAL, NULL);
			return(BAD_EXP);
		}
 		s_cat(ERx(" "));
 		s_cat(token);
 		s_cat(ERx(" "));
 		exp_type = s_g_blprimary();
		if (exp_type != DB_BOO_TYPE)
		{
			if (exp_type != BAD_EXP)
			{
				s_error(0x3D4, NONFATAL, NULL);
			}
			return(BAD_EXP);
		}
	}

	return(exp_type);
}

DB_DT_ID
s_g_blprimary()
{
	DB_DT_ID	exp_type;
	bool		saw_not = FALSE;
	char		*token;

	if (s_g_logop(&token) == OP_NOT)
	{
 		s_cat(token);
 		s_cat(ERx(" "));
		saw_not = TRUE;
	}

 	exp_type = s_g_blelement();
	if (exp_type == BAD_EXP)
	{
		return(BAD_EXP);
	}

	if (saw_not && exp_type != DB_BOO_TYPE)
	{
		s_error(0x3D4, NONFATAL, NULL);
		return(BAD_EXP);
	}

	return(exp_type);
}

DB_DT_ID
s_g_blelement()
{
	i4		type;
	DB_DT_ID	exp_type;
	char		*save_Tokchar;
	char		*token;
	i4             rtn_char;               /* dummy variable for sgskip */

 	exp_type = s_g_aexp();
	if (exp_type == BAD_EXP)
	{
		return(BAD_EXP);
	}

	type = s_g_skip(FALSE, &rtn_char);
	if (type == TK_EQUALS || type == TK_RELOP)
	{
 		s_cat(s_g_op());
 		exp_type = s_g_aexp();
		if (exp_type == NO_EXP)
		{
			s_error(0x3D5, NONFATAL, NULL);	/* missing operand */
			return(BAD_EXP);
		}
		else if (exp_type == BAD_EXP)
		{
			return(BAD_EXP);
		}

		exp_type = DB_BOO_TYPE;
	}
	else if (type == TK_ALPHA)
	{
	    /* check if "IS NULL" or "IS NOT NULL" follows */
	    save_Tokchar = Tokchar;
	    token = s_g_name(TRUE);
	    CVlower(token);
	    if (STequal(token, ERx("is")) )
	    {
		s_cat(ERx(" "));
		s_cat(token);
		s_cat(ERx(" "));
		type = s_g_skip(TRUE, &rtn_char);
		if (type == TK_ALPHA)
		{
		    token = s_g_name(TRUE);
		    CVlower(token);
		    if (STequal(token, ERx("not")))
		    {
			s_cat(token);
			s_cat(ERx(" "));
			type = r_g_skip();
			if (type == TK_ALPHA)
			{
			    token = s_g_name(TRUE);
			    CVlower(token);
			}
		    }

		    if (STequal(token, ERx("null")))
		    {
			s_cat(token);
			return DB_BOO_TYPE;
		    }
		    else
		    {
			s_error(0x3DA, NONFATAL, NULL);
			return BAD_EXP;
		    }
		}
		else
		{
		    s_error(0x3DA, NONFATAL, NULL);
		    return BAD_EXP;
		}
	    }
	    /* did not match, back up */
	    Tokchar = save_Tokchar;
	}

	return(exp_type);
}

DB_DT_ID
s_g_aexp()
{
	DB_DT_ID	exp_type;
	i4             rtn_char;               /* dummy variable for sgskip */

 	exp_type = s_g_arterm();
	if (exp_type == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while (s_g_skip(FALSE, &rtn_char) == TK_SIGN)
	{
 		s_cat(s_g_op());
 		exp_type = s_g_arterm();
		if (exp_type == NO_EXP)
		{
			s_error(0x3D5, NONFATAL, NULL);	/* missing operand */
			return(BAD_EXP);
		}
		else if (exp_type == BAD_EXP)
		{
			return(BAD_EXP);
		}
	}

	return(exp_type);
}

DB_DT_ID
s_g_arterm()
{
	DB_DT_ID	exp_type;
	i4             rtn_char;               /* dummy variable for sgskip */

 	exp_type = s_g_arfactor();
	if (exp_type == BAD_EXP)
	{
		return(BAD_EXP);
	}

	while (s_g_skip(FALSE, &rtn_char) == TK_MULTOP)
	{
 		s_cat(s_g_op());
 		exp_type = s_g_arfactor();
		if (exp_type == NO_EXP)
		{
			s_error(0x3D5, NONFATAL, NULL);	/* missing operand */
			return(BAD_EXP);
		}
		else if (exp_type == BAD_EXP)
		{
			return(BAD_EXP);
		}
	}

	return(exp_type);
}

DB_DT_ID
s_g_arfactor()
{
	DB_DT_ID	exp_type;
	i4             rtn_char;               /* dummy variable for sgskip */

 	exp_type = s_g_arprimary();
	if (exp_type == BAD_EXP)
	{
		return(BAD_EXP);
	}

	if (s_g_skip(FALSE, &rtn_char) == TK_POWER)
	{
 		s_cat(s_g_op());
 		exp_type = s_g_arfactor();
		if (exp_type == NO_EXP)
		{
			s_error(0x3D5, NONFATAL, NULL);	/* missing operand */
			return(BAD_EXP);
		}
	}

	return(exp_type);
}

DB_DT_ID
s_g_arprimary()
{
	DB_DT_ID	exp_type;
	bool		saw_sign = FALSE;
	i4             rtn_char;               /* dummy variable for sgskip */

	if (s_g_skip(FALSE, &rtn_char) == TK_SIGN)
	{
 		s_cat(s_g_op());
		saw_sign = TRUE;
	}

 	exp_type = s_g_aelement();
	if (saw_sign && exp_type == NO_EXP)
	{
		s_error(0x3D5, NONFATAL, NULL);	/* missing operand */
		return(BAD_EXP);
	}

	return(exp_type);
}

DB_DT_ID
s_g_aelement()
{
	bool		saw_cum = FALSE;
	i4		agg_code;
	i4		funct_type;
	i4		arg1_type;
	i4		arg2_type;
	DB_DT_ID	exp_type = DB_NODT;
	char		*token;
	char		*s_g_name();
	ADI_OP_ID	op_code;
	i4             rtn_char;               /* dummy variable for sgskip */

	switch(s_g_skip(FALSE, &rtn_char))
	{
	case(TK_QUOTE):
		s_cat(s_g_string(TRUE, (i4) TK_QUOTE ));
		exp_type = DB_CHR_TYPE;
		break;

	case(TK_SQUOTE):
		s_cat(s_g_string(TRUE, (i4) TK_SQUOTE ) );
		exp_type = DB_CHR_TYPE;
		break;

	case (TK_HEXLIT):
		s_cat(ERx("X"));
		CMnext(Tokchar);
		token = s_g_string(TRUE, TK_SQUOTE);
		CVupper(token);
		s_cat(token);
		exp_type = DB_CHR_TYPE;
		break;

	case(TK_OPAREN):
 		s_cat(s_g_op());
		exp_type = s_g_expr();
 		if (exp_type == BAD_EXP)
		{
			return(BAD_EXP);
		}
		if (s_g_skip(FALSE, &rtn_char) == TK_CPAREN)
		{
 			s_cat(s_g_op());
		}
		else
		{
			s_error(0x3CE, NONFATAL, NULL);
			return(BAD_EXP);
		}
		break;

	case(TK_NUMBER):
 		s_cat(s_g_number(TRUE));
		exp_type = DB_INT_TYPE;
		break;

	case(TK_ALPHA):
		token = s_g_name(TRUE);
 		s_cat(token);
		if (r_g_ltype(token) != OP_NONE)
		{
			s_error(0x3CF, NONFATAL, NULL);
			return(BAD_EXP);
		}

		/*
		** For Ctext a blank is needed between RBFAGGS and 
		** the next token (which will be the agg number).
		*/
		if ( STequal(token, ERx("RBFAGGS")))
			s_cat(ERx(" "));

		/*
		** See if this is an RBF report based on a JoinDef: 
		*/
		if (STequal(token, ERx("RBFSETUP")))
		{
			if (s_g_skip(FALSE, &rtn_char) == TK_COMMA)
			{
				s_cat(ERx(","));
				if (s_g_skip(TRUE, &rtn_char) == TK_NUMBER)
 					s_cat(s_g_number(TRUE));
				if (s_g_skip(FALSE, &rtn_char) == TK_COMMA)
					s_cat(ERx(","));	
				if (s_g_skip(TRUE, &rtn_char) == TK_ALPHA)
 					s_cat(s_g_name(TRUE));
			}
		}

		if (r_gt_cagg(token))
		{	/* cum agg found. Keep its parameters here */
			saw_cum = TRUE;
			if (s_g_skip(FALSE, &rtn_char) == TK_OPAREN)
			{	/* add break var to token */
 				s_cat(s_g_paren(TRUE));
			}

			/* next should be agg name */
			if (s_g_skip(FALSE, &rtn_char) != TK_ALPHA)
			{
				s_error(0x395, NONFATAL, NULL);
				return(BAD_EXP);
			}
 			s_cat(ERx(" "));
			token = s_g_name(TRUE);
 			s_cat(token);
		}

		op_code = r_gt_funct(token);
		if (op_code != OP_NONE)
		{	/* should be an aggregate or function */
			if (op_code == OP_BREAK)
			{
				exp_type = DB_BOO_TYPE;
			}

			if (s_g_skip(FALSE, &rtn_char) == TK_OPAREN)
			{	/* if function, get its parameters.
				  (If it isn't, this will get its format.) */
				s_cat(s_g_paren(TRUE));
			}
		}

		if (s_g_skip(FALSE, &rtn_char) == TK_OPAREN)
		{
			/* This must be the format */
			s_cat(s_g_paren(TRUE));
		}
		break;

	case(TK_DOLLAR):
 		s_cat(s_g_op());
		if (s_g_skip(FALSE, &rtn_char) == TK_ALPHA)
		{
 			s_cat(s_g_name(TRUE));
		}
		else
		{
			s_error(0x3D3, NONFATAL, NULL);
			return(BAD_EXP);
		}

		break;

	default:
		exp_type = NO_EXP;
		break;
	}

	return(exp_type);
}
