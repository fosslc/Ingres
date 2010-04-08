/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<stype.h>
# include	<sglob.h>
# include	<er.h>

/*
**   S_BREAK - called in response to the .BREAK command to set up the
**	break columns for a report.  Columns used in .HEADER and .FOOTER
**	commands must be declared in a BREAK (or SORT) command	first.
**
**	Format of the .BREAK command is:
**		.BREAK	{ colname [,] }
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		s_readin.
**
**	Side Effects:
**		Adds to the Cact_ren structure.
**		Sets the Ctype, Csequence, Csection, Cattid, and Ctext
**		fields as well.
**
**	History:
**		4-may-87 (grant)	taken from s_srt_set.
**		25-nov-1987 (peter)
**			Add global St_b_given.
**		8/10/89 (elein) garnet
**			Allow $parameters to be passed through to report
**            1/30/90 (elein) 9942
**                    Add missing parameter to s_error message
**                    where column name listed twice in .break
**                    The error message expected the name of the
**                    duplicated column and we weren't passing it.
**              2/20/90 (martym)
**                      Casting all call(s) to s_w_row() to VOID. Since it
**                      returns a STATUS now and we're not interested here.
**		5/1/90 (elein)
**			Get out of for loop without advancing (no sgskip)
**			if error.  Was causing cascading errors.
**		11-oct-90 (sylviap)
**			Added new paramter to s_g_skip.
**		14-sep-1992 (rdrane)
**			Add support for delimited identifiers.  Converted 
**			s_error() err_num value to hex to facilitate lookup
**			in errw.msg file.  Call s_g_name() for suspected
**			parameters, not s_g_token() - every one seems to...
**		8-dec-1993 (rdrane)
**			Rework delimited identifier support such that
**			the attribute name is stored normalized (b54950).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



VOID
s_break()
{
	/* internal declarations */

	i4		tok_type;		/* next token type */
	i4		rtn_char;		/*
						** dummy variable for s_g_skip()
						** and real variable for
						** s_g_ident().
						*/
	char		*name;			/* break name */
	bool		err;
	char		nrml_buf[(FE_MAXNAME + 1)];

	/* start of routine */

	if (St_b_given)
	{
		s_error(0x3A6, NONFATAL, NULL);
		s_cmd_skip();
		return;
	}
	St_b_given = TRUE;

	save_em();		/* store aside the values of Command fields */

	/* Initialize the fields in Command */

	STcopy(NAM_BREAK, Ctype);
	STcopy(ERx(" "), Csection);
	STcopy(ERx(" "), Ccommand);
	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);
	}

	Csequence = Cact_ren->ren_bcount;

	err = FALSE;
	while (!err)
	{
		tok_type = s_g_skip(TRUE, &rtn_char);

		switch (tok_type)
		{
			case TK_ENDFILE:
			case TK_PERIOD:
				goto allexit;

			case TK_COMMA:
				break;

			case TK_ALPHA:
			case TK_QUOTE:
			case TK_DOLLAR:
				if (tok_type == TK_ALPHA)
				{
					name = s_g_ident(FALSE,&rtn_char,
							 FALSE);
					if  (rtn_char != UI_REG_ID)
					{
						s_error(0x39D,NONFATAL,
							name,NULL);
						s_cmd_skip();
						err = TRUE;
						break;
					}
					IIUGdbo_dlmBEobject(name,FALSE);
				}
				else if (tok_type == TK_QUOTE)	/* Delim ID */
				{
					if  (!St_xns_given)
					{
						s_error(0x39D,NONFATAL,
							name,NULL);
						s_cmd_skip();
						err = TRUE;
						break;
					}
					name = s_g_ident(FALSE,&rtn_char,
							 FALSE);
					if  (rtn_char != UI_DELIM_ID)
					{
						s_error(0x39D,NONFATAL,
							name,NULL);
						s_cmd_skip();
						err = TRUE;
						break;
					}
					_VOID_ IIUGdlm_ChkdlmBEobject(name,
								&nrml_buf[0],
								FALSE);
					name = &nrml_buf[0];
				}
				else
				{
					name = s_g_name(FALSE);
					IIUGdbo_dlmBEobject(name,FALSE);
				}
				if (s_sbr_find(name) != NULL)
				{	/* already specified */
					s_error(0x39E, NONFATAL, name, NULL);
					s_cmd_skip();
					err = TRUE;
					break;
				}

				/* name not specified.	Add it */

				s_sbr_add(name);
				STcopy(name,Cattid);
				_VOID_ s_w_row();
				break;

			default:
				s_error(0x39D, NONFATAL, NULL);
				s_cmd_skip();
				err = TRUE;
				break;

		}
	}

    allexit:
	get_em();
}
