/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include       <ui.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<er.h>

# define	WANT_VAR	1		/* want a break var */


# define	WANT_ORDER	2		/* want a sort order */

/*
**   S_SRT_SET - called in response to the .SORT command to set up the 
**	sort columns for a report.  Columns used in .HEADER and .FOOTER
**	commands must be declared in a SORT command  first.
**
**	Format of the .SORT command is:
**		.SORT  { colname [:ascending|a|descending|d] }
**
**	Parameters:
**		none.
**
**	Returns:
**		type - the type of token which ended the command.
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
**		6/1/81 (ps)	written.
**		12/22/81 (ps)	modified for two table version.
**		07/24/89 (martym) Fixed bug #6682, by changing error 915
**		 	calls, to include a needed parameter.
**            8/10/89 (elein) garnet
**                    Allow $parameters to be passed through to report
**		8/22/89 (elein) garnet
**			Allow ENDFILE as a legitmate end for .includes
**              9/22/89 (elein) UG changes
**                      ingresug change #90025
**                      Dont optimize on su4_u42
**		10/26/89 (elein) garnet
**			Remove validation check for ascending/descending if
**			it is a variable.
**		2/2/90 (elein) us493
**			Catch "$var, $var" case in WANT_ORDER, got ALPHA case.
**			Need to reset state to WANT_VAR
**		2/16/90 (martym) 
**			Casting all calls to s_w_row() since it's returning 
**			a STATUS now, and we're not interested in it here.
**		4/18/90 (elein) 20246
**			Move reference to Cact_ren to after it was tested
**			as valid 
**		5/1/90 (elein)
**			Reset state correctly if error and we are continuing
**			in order to avoid cascading errors.
**		03-may-90 (sylviap)
**			Allows 'asc' and 'desc' as sort directions.
**		7/17/90 (elein)
**			Rework logic to replace outside switch with
**			if/else because of sparc optimizer problems.
**			Also removed su4_u42 NO_OPTIM hint, hoping it will
**			no longer be necessary.
**		11-oct-90 (sylviap)
**			Added new paramter to s_g_skip.
**		14-sep-1992 (rdrane)
**			Add support for delimited identifiers.  Converted 
**			s_error() err_num value to hex to facilitate lookup
**			in errw.msg file.  Call s_g_name() for suspected
**			parameters, not s_g_token() - every one seems to...
**		24-mar-1993 (rdrane)
**			Add TK_QUOTE to TK_ALPHA/TK_DOLLAR case of
**			WANT_ORDER state processing - sreport was failing
**			lines of the form
**				.SORT "delim id1", "delim id2" ...
**			(lines of the form
**				.SORT "delim id1":a, "delim id2":d ...
**			 worked just fine ...)
**		8-dec-1993 (rdrane)
**			Rework delimited identifier support such that
**			the attribute name is stored normalized (b54950).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



i4
s_srt_set()
{
	/* internal declarations */
	i4		type;			/* next token type */
	i4		state = WANT_VAR;	/* state machine */
	RSO		*rso;			/* found/created RSO struct */
	char		*name;			/* break name */
	bool		wantchar = TRUE;	/* true to get another char */
	i4             rtn_char;               /* dummy variable for sgskip */
	char		nrml_buf[(FE_MAXNAME + 1)];

	/* start of routine */


	save_em();		/* store aside the values of Command fields */

	/* Initialize the fields in Command */

	STcopy(NAM_SORT,Ctype);
	STcopy(ERx(" "),Csection);
	/* fix for bug 4646 */
	STcopy(ERx("y"),Ccommand);

	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);
	}
	Csequence = Cact_ren->ren_scount;

	for(;;)
	{
		if (wantchar == TRUE)
		{
			type = s_g_skip(TRUE, &rtn_char);
		}
		wantchar = TRUE;


		if( state == WANT_VAR )
		{
			switch(type)
			{
				case(TK_ENDFILE):
				case(TK_PERIOD):
					goto allexit;

				case(TK_COMMA):
					break;

				case(TK_ALPHA):
				case(TK_QUOTE):
				case(TK_DOLLAR):
					if (type == TK_ALPHA)
					{
						name = s_g_ident(FALSE,
								 &rtn_char,
								 FALSE);
						if  (rtn_char != UI_REG_ID)
						{
							s_error(0x391,NONFATAL,
							name,NULL);
							s_cmd_skip();
							break;
						}
						IIUGdbo_dlmBEobject(name,FALSE);
					}
					else if (type == TK_QUOTE)
					{
						/*
						** Delim ID
						*/
						name = s_g_ident(FALSE,
								 &rtn_char,
								 FALSE);
						if ((rtn_char != UI_DELIM_ID) ||
						    (!St_xns_given))
						{
							s_error(0x391,NONFATAL,
								name,NULL);
							s_cmd_skip();
							break;
						}
						_VOID_ IIUGdlm_ChkdlmBEobject(
								name,
								&nrml_buf[0],
								FALSE);
						name = &nrml_buf[0];
					}
					else
					{
						name = s_g_name(FALSE);
						IIUGdbo_dlmBEobject(name,FALSE);
					}
					if ((rso = s_rso_find(name)) != NULL)
					{	/* already specified */
						if( rso->rso_order  == 0)
						{
							s_error(0x392, NONFATAL,
								name, NULL);
						}
						else
						{
							s_error(0x394, NONFATAL,
								name, NULL);
						}
						state = WANT_ORDER;
						break;
					}

					/* name not specified.  Add it */
					rso = s_rso_add(name,TRUE);
					rso->rso_direct = O_ASCEND;
					STcopy(name,Cattid);
					STcopy(O_ASCEND,Ctext);
					state = WANT_ORDER;
					break;

				default:
					s_error(0x391, NONFATAL, NULL);
					break;

				}
		}
		else /* state == WANT_ORDER */
		{
			switch(type)
			{
				case(TK_ENDFILE):
				case(TK_PERIOD):
					_VOID_ s_w_row();
					goto allexit;

				case(TK_COMMA):
					/*
					** What's really happening here is that
					** we've found the end of the sort
					** specification.  We'll go around
					** again and hope to find the next name
					** or variable, which will go to the
					** TK_ALPHA/DOLLAR/QUOTE case, and which
					** is where we'll actually change state
					** and reprocess the token in the
					** WANT_VAR state.  The logic of this
					** becomes clear when you consider that
					** the comma is optional - whitespace
					** will also go to the TK_ALPHA/...
					** case.  Thus, we avoid duplicating
					** that case's code here.
					*/
					break;

				case(TK_COLON):
					type = s_g_skip(TRUE, &rtn_char);

					if  ((type != TK_ALPHA) &&
					     (type != TK_DOLLAR))
					{
						s_error(0x393, NONFATAL,
							name, NULL);
						break;
					}
					if (type != TK_ALPHA)
					{
						name = s_g_name(FALSE);
						rso->rso_direct = name;
						STcopy(name,Ctext);
						_VOID_ s_w_row();
						state = WANT_VAR;
						break;
					}

					name = s_g_name(FALSE);
					CVlower(name);

					if (STequal(name,ERx("d"))
				   	|| STequal(name,ERx("desc"))
				   	|| STequal(name,ERx("descend"))
				   	|| STequal(name,ERx("descending")))
					{
						rso->rso_direct = O_DESCEND;
						STcopy(O_DESCEND,Ctext);
						_VOID_ s_w_row();
						state = WANT_VAR;
						break;
					}
					if (STequal(name,ERx("a"))
				    	||STequal(name,ERx("asc"))
				    	||STequal(name,ERx("ascend"))
				    	||STequal(name,ERx("ascending")))
					{
						_VOID_ s_w_row();
						state = WANT_VAR;
						break;
					}
					s_error(0x393, NONFATAL, name, NULL);
					break;

				case(TK_ALPHA):
				case(TK_DOLLAR):
				case(TK_QUOTE):
					/*
					** Finish this sort specification, and
					** reprocess this name in the WANT_VAR
					** state above (which will also worry
					** about any disallowed delim IDs).
					*/
					_VOID_ s_w_row();
					wantchar = FALSE;
					state = WANT_VAR;
					break;

				default:
					s_error(0x391, NONFATAL, NULL);
					break;

			}
		}

	}
	allexit:
		get_em();
		return(type);
}
