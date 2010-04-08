/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	<stype.h> 
# include	<sglob.h> 
# include	<rglob.h> 
# include	<cm.h> 
# include	<er.h>
# include 	"ersr.h"

/*
**   S_PROCESS - read in the text file and process the commands.  This
**	calls underlying routines to write to the RACTION file.
**
**	This routine is essentially the main loop of the program.
**
**	Parameters:
**		within_if	TRUE if processing commands within an
**				.IF statement.
**
**	Returns:
**		none.
**
**	Called by:
**		s_readin.
**
**	Side Effects:
**		none.
**
**	History:
**		6/1/81 (ps) - written.
**		11/10/83 (gac)	split off from s_readin for if statement.
**		12/6/83	(gac)	bug 938 fixed -- do not allow .NEWPAGE in 
**			.HEADER page.
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		6/21/89 (elein) garnet
**			Added recognition and call for setup/cleanup commands.
**              8/10/89 (elein) garnet
**                    Allow $parameters to be passed through to report
**		8/21/89 (elein) garnet
**			Add case for .INCLUDE, and allow final write (s_w_row)
**			for IN_LIST case which may, in the case of include
**			end with EOF
**              9/12/89 (elein)
**                      When calling s_cmd_skip change state so we
**                      don't get repetitive error messages for bad .LET 
**			statements
**              2/20/90 (martym)
**                      Casting all call(s) to s_w_row() to VOID. Since it
**                      returns a STATUS now and we're not interested here.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		05-apr-90 (sylviap)
**			Calls IISRwds_WidthSet for .PAGEWIDTH support 
**			(#129, #588)
**              23-apr-90 (sylviap)
**                      If running report w/-i flag with a file w/multiple
**                      reports, flags an error. (jupbug #21354, US#347)
**              11-oct-90 (sylviap)
**                      Added new parameter to s_g_skip.
**		15-oct-90 (steveh)
**			Check for empty input file. (bug #32954)
**		11/6/90 (elein) 31499, 34229
**			When we see a comma in the IN_OTHER case,
**			tack it on.  Don't scan them out.
**		11/9/90 (elein) 33082
**			Don't fall into IN_IF if ELSEIF incorrectly
**			placed.
**		26-aug-1992 (rdrane)
**			Use new constants in s_reset() invocations.
**			All FUNC_EXTERN's now in HDR files.  Explicitly indicate
**			that functions returns VOID.  Converted r_error()
**			err_num value to hex to facilitate lookup in errw.msg
**			file.  Got rid of a lot register declarations.
**			Add processing for expanded namespace comand.
**			Re-wrote r_gt_code() for efficiencies and to allow
**			commands to contain any legal INGRES FE name character.
**		25-nov-1992 (rdrane)
**			Rename references to S_ANSI92 to S_DELIMID as per
**			the LRC.
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
s_process(within_if)
bool	within_if;
{
	/* internal declarations */

	register i4	state = IN_NOTHING;	/* state switch */
	register i4	type;				/* type of next token */
	register i4	code;				/* code for command type */
			char	name[MAXCMD+1];		/* holds name of command */
	register bool	wantchar = TRUE;	/* TRUE if loop needs new char*/
			 bool	reported = FALSE;	/*
										** TRUE if error reported and
										** scanning for command
										*/
			 bool	else_seen = FALSE;	/* TRUE if .ELSE processed */
	register char	*save_Tokchar;
			 bool	previous_token = FALSE;	/*
											** TRUE if token already seen
											** in .PRINT command
											*/
		DB_DT_ID exp_type;
		i4		rtn_char;              	 /* dummy variable for sgskip */
		bool	empty_check_flag = TRUE; /* TRUE if input file empty */


	/* loop through tokens, adding commands etc. */

	for(;;)
	{

		/* get next token start */
		type = s_g_skip(wantchar, &rtn_char);
		wantchar = TRUE;			/* default */

		if (type == TK_ENDFILE)
		{
			/* check for an empty file */
			if (empty_check_flag == TRUE && within_if == FALSE)
			{
				/*
				** issue a message and exit if the input .rw
				** file is empty
				*/
				r_syserr(S_SR0018_empty_rw_file, En_sfile);
			}
			break;
		}

		/* make note of the fact that the file is not empty */
		empty_check_flag = FALSE;

		switch(state)
		{
			case(IN_NOTHING):
				/* looking for the start of a command */
				switch(type)
				{
					case(TK_COMMA):
						break;

					case(TK_PERIOD):	/* found one */
						state = WANT_COMMAND;
						reported = FALSE;
						break;

					default:	/* garbage found */
						if (!reported)
						{
							s_error(0x387, NONFATAL, Tokchar, NULL);
							reported = TRUE;
						}
						break;

				}
				break;

			case(WANT_COMMAND):
				/* looking for command name */
				STcopy(ERx(""), name);
				previous_token = FALSE;
				switch(type)
				{
					case(TK_ALPHA):		/* only valid one */
						/* name must be all alphabetic */
						r_gt_word(name);
						CVlower(name);		/* make lower case */
						if ((code=s_get_scode(name)) != S_ERROR)
						{	/* found REPSPEC command */
							if (within_if && code != S_INCLUDE)
							{	/* Command cannot be within .IF statement */
								CVupper(name);
								s_error(0x3C0, NONFATAL, name, NULL);
								CVlower(name);
								reported = TRUE;
							}
							switch(code)
							{
								case(S_HEADER):
									s_brk_set(B_HEADER);
									break;

								case(S_FOOTER):
									s_brk_set(B_FOOTER);
									break;

								case(S_DETAIL):
									s_rso_set(NAM_DETAIL,B_HEADER);
									break;

								case(S_SORT):
									if (St_s_given)
									{
										s_error(0x39F, NONFATAL, NULL);
										s_cmd_skip();
									}
									else
									{
										St_s_given = TRUE;
										s_srt_set();
									}
									wantchar = FALSE;
									break;

								case(S_BREAK):
									s_break();
									wantchar = FALSE;
									break;

								/* SETUP */
								case(S_SQUERY):
									if (St_setup_given)
									{
										s_error(0x3EB, NONFATAL, NULL);
									}
									s_setclean(S_SQUERY);
									St_setup_given = TRUE;
									wantchar = FALSE;
									break;

								/* CLEANUP */
								case(S_CQUERY):
									if (St_clean_given)
									{
										s_error(0x3EB, NONFATAL, NULL);
									}
									s_setclean(S_CQUERY);
									St_clean_given = TRUE;
									wantchar = FALSE;
									break;

								case(S_QUERY):
									s_quer_set();
									wantchar = FALSE;
									break;

								case(S_DATA):
									s_dat_set();
									break;

								case(S_OUTPUT):
									s_out_set();
									break;

								case(S_NAME):
									/* jb #21354, US #347 */
									if ((St_ispec) && (St_one_name))
									{
									   s_error(0x407, FATAL, NULL);
									   break;
									}
								        s_nam_set();
									wantchar = FALSE;
									St_one_name = TRUE;
									break;

								case(S_SREMARK):
									if (s_srem_set() == FAIL)
									{
									    wantchar = FALSE;
									}
									break;

								case(S_LREMARK):
									if (s_lrem_set() == FAIL)
									{
									    wantchar = FALSE;
									}
									break;

								case(S_DECLARE):
									s_dcl_set();
									wantchar = FALSE;
									break;

								case(S_INCLUDE):
									s_include();
									wantchar = FALSE;
									break;
								/* .PAGEWIDTH */
								case(S_PGWIDTH):
									IISRwds_WidthSet();
									wantchar = FALSE;
									break;

								case(S_DELIMID):
									if  (!IISRxs_XNSSet())
									{
										CVupper(name);
										s_error(0x388, NONFATAL, name, NULL);
										CVlower(name);
										reported = TRUE;
									}
									break;

								default:
									break;

							}
							state = IN_NOTHING;
							break;
						}

						/* check for RTEXT command */

						if ((code=r_gt_code(name))!=C_ERROR)
						{
							switch (code)
							{
								case (C_TEXT):
								case (C_PRINTLN):
									state = IN_PRINT;
									break;

								case (C_TFORMAT):
								case (C_FORMAT):
								case (C_POSITION):
								case (C_WITHIN):
								case (C_WIDTH):
									state = IN_LIST;
									break;

								case (C_ELSEIF):
									if (!within_if)
									{
										CVupper(name);
										s_error(0x3C1, NONFATAL, name, NULL);
										CVlower(name);
										reported = TRUE;
										state = IN_OTHER;
										break;
									}
									else if (else_seen)
									{
										CVupper(name);
										s_error(0x3C2, NONFATAL, name, NULL);
										CVlower(name);
										reported = TRUE;
										state = IN_OTHER;
										break;
									}
									/* Drop through */
								case (C_IF):
									state = IN_IF;
									break;

								case (C_ELSE):
									if (!within_if)
									{
										CVupper(name);
										s_error(0x3C1, NONFATAL, name, NULL);
										CVlower(name);
										reported = TRUE;
									}
									else if (else_seen)
									{
										CVupper(name);
										s_error(0x3C2, NONFATAL, name, NULL);
										CVlower(name);
										reported = TRUE;
									}
									else
									{
										else_seen = TRUE;
									}
									state = IN_OTHER;
									break;

								case (C_ENDIF):
									if (!within_if)
									{
										CVupper(name);
										s_error(0x3C1, NONFATAL, name, NULL);
										CVlower(name);
										reported = TRUE;
									}
									state = IN_OTHER;
									break;

								case (C_THEN):
									CVupper(name);
									s_error(0x3C1, NONFATAL, name, NULL);
									CVlower(name);
									reported = TRUE;
									state = WANT_COMMAND;
									break;

								case (C_LET):
									save_Tokchar = Tokchar;
									state = IN_LET;
									break;

								case (C_NPAGE):
									if (STequal(Csection, B_HEADER)  &&
									    STequal(Cattid, NAM_PAGE) )
									{
										s_error(0x396, NONFATAL, NULL);
									}
									state = IN_OTHER;
									break;
									
								default:
									state = IN_OTHER;
									break;
							}
							s_set_cmd(name);
							break;
						}

						/* can't recognize command */

					default:
						if (!reported)
						{
							s_error(0x388, NONFATAL, name, NULL);
							reported = TRUE;
						}
						state = IN_NOTHING;
						break;

				}
				break;

			case(IN_PRINT):
				/* in print command.  Try to concatenate */
				switch(type)
				{
					case(TK_COMMA):
						break;

					case(TK_PERIOD):
						_VOID_ s_w_row();
						/* add the end of print command */
						s_set_cmd(NAM_EPRINT);
						_VOID_ s_w_row();
						state = WANT_COMMAND;
						reported = FALSE;
						break;

					default:	/* valid command found */
						s_tok_add(previous_token, state);
						previous_token = TRUE;
						wantchar = FALSE;
						break;

				}
				break;

			case(IN_LIST):
				/*
				** In format, tformat or position  command.
				** Try to concatenate
				*/
				switch(type)
				{
					case(TK_COMMA):
						break;

					case(TK_PERIOD):
						_VOID_ s_w_row();
						state = WANT_COMMAND;
						reported = FALSE;
						break;

					default:	/* valid command found */
						s_tok_add(previous_token, state);
						previous_token = TRUE;
						wantchar = FALSE;
						break;
				}
				break;

			case(IN_IF):
			/* In .IF or .ELSEIF command, look for <boolean expression> */
				s_tok_add(FALSE, state);
				_VOID_ s_w_row();
				if (s_g_skip(FALSE, &rtn_char) != TK_PERIOD)
				{
					s_error(0x3C3, NONFATAL, NULL);
					reported = TRUE;
				}
				save_Tokchar = Tokchar-1;
				state = IN_IFTHEN;
				wantchar = TRUE;
				break;

			case(IN_IFTHEN):
			/* In .IF or .ELSEIF command, look for .THEN */
				r_gt_word(name);
				CVlower(name);
				if (r_gt_code(name) == C_THEN)
				{
					s_set_cmd(name);
					_VOID_ s_w_row();
				}
				else
				{
					if (!reported)
					{
						s_error(0x3C3, NONFATAL, NULL);
						reported = TRUE;
					}
					Tokchar = save_Tokchar;		/* backup to start of command; could be next one */
				}
				if (code == C_IF)
				{
					s_process(TRUE);	/* Push down a level */
					state = WANT_COMMAND;
					reported = FALSE;
				}
				else
				{
					state = IN_NOTHING;
				}
				break;

			case(IN_LET):
				if (type == TK_ALPHA)
				{
					s_cat(s_g_token(TRUE));
					wantchar = FALSE;
					state = IN_LET_EQUALS;
				}
				else
				{
					Tokchar = save_Tokchar;
					s_error(0x3D7, NONFATAL, NULL);
					reported = TRUE;
					s_cmd_skip();
					state = IN_NOTHING;
				}
				break;

			case(IN_LET_EQUALS):
				if (type == TK_COLON)
				{
					s_cat(ERx(":"));
					type = s_g_skip(TRUE, &rtn_char);
				}

				if (type == TK_EQUALS)
				{
					s_cat(ERx("="));
					state = IN_LET_RIGHT;
				}
				else
				{
					s_error(0x3D7, NONFATAL, NULL);
					reported = TRUE;
					s_cmd_skip();
					state = IN_NOTHING;
				}
				break;

			case(IN_LET_RIGHT):
				exp_type = s_g_expr();
				if (exp_type == DB_BOO_TYPE ||
				    exp_type == BAD_EXP)
				{
					if (exp_type != BAD_EXP)
					{
						s_error(0x3D8, NONFATAL, NULL);
					}
					reported = TRUE;
					s_cmd_skip();
					state = IN_NOTHING;
				}
				else
				{
					_VOID_ s_w_row();
					s_set_cmd(NAM_ELET);
					_VOID_ s_w_row();
				}

				wantchar = FALSE;
				state = IN_NOTHING;
				break;

			case(IN_OTHER):
				switch(type)
				{
					case(TK_COMMA):
						s_cat(ERx(","));
						wantchar = TRUE;
						break;

					case(TK_PERIOD):
						_VOID_ s_w_row();
						if (within_if && code == C_ENDIF)
						{
							return;		/* Pop up a level */
						}
						state = WANT_COMMAND;
						reported = FALSE;
						break;

					default:
						/* change to not add commas */
						s_tok_add(FALSE, state);
						wantchar = FALSE;
						break;
				}
				break;
		}
	}

	/* end of file found. */

	switch(state)
	{
		case(IN_PRINT):
			/* flush out print command */
			_VOID_ s_w_row();
			s_set_cmd(NAM_EPRINT);		/* end it right */
			/* fall through */

		case(IN_LIST):
			/* NO BREAK */
		case(IN_OTHER):
			/* flush final command */
			_VOID_ s_w_row();
			if (within_if && code == C_ENDIF)
			{
				return;		/* Pop up a level */
			}
			break;

		case(IN_IF):
		case(IN_IFTHEN):
			r_error(0x3A2, FATAL, NULL);
			
		default:
			break;
	}

	if (within_if)
	{
		r_error(0x3A2, FATAL, NULL);
	}

	return;
}


VOID
r_gt_word(word)
char	*word;
{
	i4	nc;
/*
** KJ 03/25/87U (TY)
*/
	i4	nb;
	char	*hptr;

	nc = 0;
	hptr = Tokchar;

	while (TRUE)
	{	/* get next char */
	    if (CMnmchar(Tokchar))
	    {
			nb = CMbytecnt(Tokchar);
			if (nc + nb <= MAXCMD)
			{
				CMcpyinc(Tokchar,word);
			}
			continue;
	    }
	    break;	/* get out */
	}
	*word = EOS;
	CMprev(Tokchar,hptr);

	return;
}
