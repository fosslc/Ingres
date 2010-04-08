/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<er.h>

/*
**   S_TOK_ADD - add a token from the input line into the forming Command line, 
**	If not enough space is left for the token in a print or if command,
**	write out the current one, and start a new one.
**	If a position, format, or tformat command, write it out at
**	the end of the parentheses to try to keep the lines down to size.
**
**	Parameters:
**		prev_tok - TRUE if there was a previous token for the command.
**		state - the state in s_readin.
**
**	Returns:
**		none.
**
**	Called by:
**		s_process.
**
**	Side Effects:
**		May write out the current Command.
**
**	Trace Flags:
**		13.0, 13.4.
**
**	History:
**		6/1/81 (ps) - written.
**		11/10/83 (gac)	added .IF statement.
**		12/1/83 (gac)	allow unlimited-length tokens.
**		1/22/85	(gac)	fixed bug #4907 -- RBF trim >98 characters
**				no longer gets syserr when Filed and
**				SREPORTed.
**		8/4/89 (elein) garnet
**			Pass expressions instead of tokens as default.
**		11-oct-90 (sylviap)
**			Added new paramter to s_g_skip.
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.
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
s_tok_add(prev_tok,state)
bool	prev_tok;
i4	state;
{
	/* internal declarations */

	DB_DT_ID	exp_type;		/* type of expression */
	i4             rtn_char;               /* dummy variable for sgskip */

	/* start of routine */



	/* get the appropriate token */

	if (prev_tok)
	{
 		s_cat(ERx(","));
	}

	switch(state)
	{
	case(IN_IF):
 		exp_type = s_g_expr();
		if (exp_type != DB_BOO_TYPE)
		{
			if (exp_type != BAD_EXP)
			{
				s_error(0x3CA, NONFATAL, NULL);
			}
			s_cmd_skip();
			return;
		}
		break;

	case(IN_PRINT):
 		exp_type = s_g_expr();
		if (exp_type == DB_BOO_TYPE || exp_type == BAD_EXP)
		{
			if (exp_type != BAD_EXP)
			{
				s_error(0x3CB, NONFATAL, NULL);
			}
			s_cmd_skip();
			return;
		}

		if (s_g_skip(FALSE, &rtn_char) == TK_OPAREN)
		{
 			s_cat(s_g_paren(TRUE));
		}
		break;

	default:
		switch(s_g_skip(FALSE, &rtn_char))
		{
			case(TK_OPAREN):
 				s_cat(s_g_paren(TRUE));
				break;

			case(TK_QUOTE):
				s_cat(s_g_string(TRUE, TK_QUOTE));
				break;

			default:
 				exp_type = s_g_expr();
				if (exp_type == DB_BOO_TYPE || exp_type == BAD_EXP)
				{
					if (exp_type != BAD_EXP)
					{
						s_error(0x3CB, NONFATAL, NULL);
					}
					s_cmd_skip();
					return;
				}
				if (s_g_skip(FALSE, &rtn_char) == TK_OPAREN)
				{
 					s_cat(s_g_paren(TRUE));
				}
				break;

		}
		break;
	}

	return;
}
