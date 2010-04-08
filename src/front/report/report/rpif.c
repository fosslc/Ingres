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
# include	<errw.h>
# include       <er.h>

/*
**   R_P_IF - set up the TCMD structures from an RTEXT if statement.
**	A parse tree of the boolean expression is generated
**	up to the THEN command. The IFS structure points to this parse tree.
**	TCMDS are generated for the "then" actions and then the "else" actions
**	and the IFS structure points to both of these lists.
**	If "elseif" is encountered, another IFS structure is generated
**	and is pointed to by the "else" part of the previous IFS (as if
**	an .ELSE .IF was encountered).
**	The next command (tcmd_below) of the last command in each list
**	will be a no-op command generated ahead of time.
**
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		none.
**
**	History:
**		12/12/83 (gac)	written.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
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
r_p_if()
{
	/* internal declarations */

	DB_DATA_VALUE	type;
	TCMD	*end_cmd;
	IFS	*ifstruct;
	ACTION	code;
	ITEM	expr_item;
	ACTION	r_mak_act();
	i4	status;

	/* start of routine */

	/* Setup no-op command for ENDIF now so we know what to put in
	** next_command of the last command of each block of commands in
	** the if statement.
	*/

	end_cmd = r_new_tcmd((TCMD *)NULL);
	end_cmd->tcmd_code = P_NOOP;

	for (;;)
	{
		Cact_tcmd->tcmd_code = P_IF;
		ifstruct = (IFS *) MEreqmem(0,sizeof(IFS),TRUE,(STATUS *) NULL);
		Cact_tcmd->tcmd_val.t_v_ifs = ifstruct;
		Cact_tcmd->tcmd_below = end_cmd;

		status = r_g_expr(&expr_item, &type);
		if (status == BAD_EXP)
		{
			r_cmd_skp();
		}
		else if (status == GOOD_EXP &&
			 type.db_datatype == DB_BOO_TYPE &&
			 expr_item.item_type == I_EXP)
		{
			ifstruct->ifs_exp = expr_item.item_val.i_v_exp;
		}
		else
		{
			r_syserr(E_RW003A_expected_boolean_expr);
		}

		r_advance();	/* Get next command */
		if (r_gt_code(Cact_command) != C_THEN)
		{
			r_syserr(E_RW003C_expected_THEN);
		}

		code = r_mak_act(&ifstruct->ifs_then, end_cmd);
		if (code != C_ELSEIF)
		{
			break;
		}

		r_g_set(Cact_rtext);	/* Point to boolean expression */

		Cact_tcmd = r_new_tcmd((TCMD *)NULL);
		ifstruct->ifs_else = Cact_tcmd;
	}

	if (code == C_ELSE)
	{
		code = r_mak_act(&ifstruct->ifs_else, end_cmd);
	}
	else
	{
		ifstruct->ifs_else = end_cmd;
	}

	if (code != C_ENDIF)
	{
		r_syserr(E_RW003B_expected_ENDIF);
	}

	Cact_tcmd = end_cmd;

	return;
}

/*
**   R_MAK_ACT - make a linear list of TCMD actions ending with .ELSE,
**	.ELSEIF, or .ENDIF.
**
**	Parameters:
**		start	pointer to start of list of TCMD actions
**			returned by this routine.
**		end	pointer to NO_OP TCMD action to end list of actions.
**
**	Returns:
**		code of ending command.
**
**	Side Effects:
**		First advances to next command.
**		Cact_tcmd points to last action in list.
**
**	History:
**		12/14/83 (gac)	written.
*/

ACTION
r_mak_act(start, end)
TCMD	**start;
TCMD	*end;
{
	ACTION	code;
	bool	first = TRUE;

	Cact_tcmd = NULL;

	r_advance();
	while ((code = r_gt_code(Cact_command)) != C_ELSE &&
	       code != C_ELSEIF &&
	       code != C_ENDIF)
	{
		Cact_tcmd = r_new_tcmd(Cact_tcmd);

		if (first)
		{
			*start = Cact_tcmd;
			first = FALSE;
		}

		r_act_set();
		r_advance();
	}

	if (Cact_tcmd == NULL)
	{	/* no actions for then */
		*start = end;
	}
	else
	{
		Cact_tcmd->tcmd_below = end;
	}

	return(code);
}
