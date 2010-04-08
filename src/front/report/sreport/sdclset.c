/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h>
# include	 <sglob.h>
# include	<st.h>
# include	<er.h>

/*
**   S_DECL_SET - called in response to the .DECLARE command to declare the
**	runtime variables (parameters) for a report.
**
**	Format of the .DECLARE command is:
**	    .DECLARE  variable_name = datatype [WITH NULL | NOT NULL]
**						[WITH PROMPT "prompt_string"]
**						[WITH VALUE value]
**		  { , variable_name = datatype . . . }
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
**		Sets the Ctype, Csequence, Csection, Cattid, and Ctext
**		fields as well.
**
**	History:
**		6/11/87 (gac)	stolen from s_srt_set.
**		25-nov-1987 (peter)
**			Add global St_d_given.
**		1/31/90 (elein) b9949
**			Parameters to s_error need to be PTRs
**              2/20/90 (martym)
**                      Casting all call(s) to s_w_row() to VOID. Since it
**                      returns a STATUS now and we're not interested here.
**		3/20/90 (elein) performance
**			Change STcompare to STequal
**		7/13/90 (elein)
**                      Use Declare sequence number to allow unordered
**			multiple declares.
**			Allow new "with value" syntax
**		11-oct-90 (sylviap)
**			Added new paramter to s_g_skip.
**	16-mar-1993 (rdrane)
**		Increment Seq_declare by the actual Csequence
**		value, not a constant "1".  This avoids duplicate
**		keys when multiple .DECLARE statements exist and
**		other than the last one requires multiple rows in
**		ii_rcommands (b44871).
**	25-may-1993 (rdrane)
**		Allow WITH VALUE to be a hexadecimal string literal.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
**	7-oct-1993 (rdrane)
**		Don't bother incrementing Seq_declare - just save the last
**		known value of Csequence.  Note that "allexit" is a misnomer,
**		since we'll return directly on eror conditions.  So, don't
**		forget to save Csequence, since calls to s_cat() can cause
**		it to be incremented, and we can't lose those!  The change for
**		b44871 was mega-overkill, and quickly overflowed the
**		rcosequence's i2.  This fixes b55256, and should have been the
**		fix for b44871 in the first place.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


FUNC_EXTERN STATUS s_w_row();


# define	WANT_VAR	1	/* expecting variable name */
# define	WANT_EQUALS	2	/* expecting = sign */
# define	WANT_TYPE	3	/* expecting datatype */
# define	WANT_PROMPT	4	/* expecting prompt string */
# define	WANT_VALUE	5	/* expecting value */
# define	WANT_WITH	6	/* expecting with */

VOID
s_dcl_set()
{
    /* internal declarations */

    i4		type;			/* next token type */
    DB_DT_ID	exp_type;		/* expression type for value */
    char	*token;
    i4		state = WANT_VAR;	/* state of parse */
    char	max_size[5];		/* 5 is arbitrary */
    bool	inc;			/* always inc to next character
					** execept after fetching a value
					*/
    i4          rtn_char;               /* dummy variable for sgskip */

    /* start of routine */


    if (Cact_ren == NULL)
    {
	s_error(0x38A, FATAL, NULL);
    }

    save_em();		/* store aside the values of Command fields */

    /* Initialize the fields in Command */

    STcopy(NAM_DECLARE, Ctype);
    STcopy(ERx(" "), Csection);
    STcopy(ERx(" "), Ccommand);
    STcopy(ERx(""), Ctext);
    Csequence = Seq_declare;
    inc = TRUE;

    for (;;)
    {
	type = s_g_skip(inc, &rtn_char);

    	inc = TRUE;
	switch(state)
	{
	case(WANT_VAR):
	    switch(type)
	    {
	    case(TK_PERIOD):
	    case(TK_ENDFILE):
		goto allexit;

	    case(TK_COMMA):
		_VOID_ s_w_row();
		STcopy(ERx(""), Ctext);
		break;

	    case(TK_ALPHA):
		token = s_g_name(FALSE);
		CVlower(token);
		if (STlength(token) > FE_MAXNAME)
		{
			CVna(FE_MAXNAME,max_size);
			s_error(0x3A1, NONFATAL, (PTR) max_size, NULL);
			s_cmd_skip();
    			Seq_declare = Csequence;
			return;
		}
		STcopy(token, Cattid);
		state = WANT_EQUALS;
		break;

	    default:
		s_error(0x3A0, NONFATAL, NULL);
		s_cmd_skip();
    		Seq_declare = Csequence;
		return;

	    }
	    break;

	case(WANT_EQUALS):
	    switch(type)
	    {
	    case(TK_ENDFILE):
		r_error(0x3A2, FATAL, NULL);

	    case(TK_EQUALS):
		state = WANT_TYPE;
		break;

	    default:
		s_error(0x3A0, NONFATAL, NULL);
		s_cmd_skip();
    		Seq_declare = Csequence;
		return;
	    }
	    break;

	case(WANT_TYPE):
	case(WANT_WITH):
	    switch(type)
	    {
	    case(TK_PERIOD):
	    case(TK_ENDFILE):
		goto allexit;

	    case(TK_COMMA):
		_VOID_ s_w_row();
		STcopy(ERx(""), Ctext);
		state = WANT_VAR;
		break;

	    case(TK_ALPHA):
		token = s_g_name(FALSE);
		CVlower(token);
		if (STlength(Ctext) > 0)
		{
			s_cat(ERx(" "));
		}
		s_cat(token);
		if (STequal(token, ERx("prompt")))
		{
		    state = WANT_PROMPT;
		}
		else if (STequal(token, ERx("value")))
		{
		    state = WANT_VALUE;
		}
		break;

	    case(TK_OPAREN):
		token = s_g_paren(FALSE);
		s_cat(token);
		break;

	    default:
		s_error(0x3A0, NONFATAL, NULL);
		s_cmd_skip();
    		Seq_declare = Csequence;
		return;
	    }
	    break;

	case(WANT_PROMPT):
	/* NO BREAK */
	case(WANT_VALUE):
	    switch(type)
	    {
	    case(TK_ENDFILE):
		r_error(0x3A2, FATAL, NULL);

	    case(TK_QUOTE):
	    case(TK_SQUOTE):
		token = s_g_string(FALSE, type);
		s_cat(ERx(" "));
		s_cat(token);
		state = WANT_WITH;
		break;

	    case (TK_HEXLIT):
		s_cat(ERx(" X"));	/* Don't lose the leading blank! */
		CMnext(Tokchar);
		token = s_g_string(FALSE, TK_SQUOTE);
		CVupper(token);
		s_cat(token);
		state = WANT_WITH;
		break;

	    default:
		s_error(0x3A0, NONFATAL, NULL);
		s_cmd_skip();
    		Seq_declare = Csequence;
		return;
	    }
	    break;
	}
    }

allexit:
    _VOID_ s_w_row();
    Seq_declare = Csequence;
    get_em();
}
