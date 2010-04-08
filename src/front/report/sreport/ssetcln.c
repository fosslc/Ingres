/*
** Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
#include <stype.h>
#include <sglob.h>
#include <er.h>
#include <cm.h>

/*
**   S_SETCLEAN - Create the rcommands for setup and clean up sections
**	
**
**	Parameters:
**		code			either S_SQUERY (Setup query)
**					or S_CQUERY (Cleanup query)
**
**	Returns:
**		none.
**
**	Error Messages:
**		None
**
**	History:
**	 6/21/89 (elein)	written
**	 2/16/90 (martym) 
**		Casting all calls to s_w_row() since it's returning 
**		a STATUS now, and we're not interested in it here.
**	 11-oct-90 (sylviap)
**		Added new paramter to s_g_skip.
**	2/1/91 (elein) 35719
**		Add case for logical operators which must
**		be fetched as a unit since all elements
**		are now delimiters.
**	28-sep-1992 (rdrane)
**		Converted s_error() err_num value to hex to facilitate
**		lookup in errw.msg file.  Fix-up external declarations -
**		they've been moved to hdr files.
**		Support owner.tablename and column qualifications when
**		tablename and column are delimited identifiers, e.g., a."bc".
**		Additionally suport correlation names as delimited identifiers,
**		e.g., "a"."b".  For the a."b" case, s_g_token()	will consider
**		it two tokens, and s_setcln() will nominally separate them with
**		a space.  So, effect a look-ahead and if we see a '."', then
**		save the current token and concatenate it with the next token
**		before proceeding.  The "a".b and "a"."b" cases go through
**		s_g_ident(), not s_g_token(), and so are always returned intact.
**	17-may-1993 (rdrane)
**		Added support for hex literals.
**	1-jun-1993 (rdrane)
**		Give error indication if s_g_ident() finds a delimited
**		identifier when they are not enabled.
**	20-jan-1994 (rdrane)
**		Correct improper parameterization of some error msgs
**		which probably resulted from 1-jun-1993 fix. b58958.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

i4
s_setclean(code)
i4	code;
{

	char	*qt;
	i4	type;		/* for return type of s_g_skip */
	i4	rtn_char;	/*
				** dummy variable for s_g_skip() and
				** s_g_ident()
				*/
	bool	inquote;
        bool	skip_next;
	char	compound_buf[((2 * (FE_MAXNAME + 2)) + 1)];


	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);
	}

	save_em();

	/*
 	 * Set up Ctype field
 	 */
	switch (code){
		case S_CQUERY:
			STcopy(NAM_CLEANUP, Ctype);
			break;
		case S_SQUERY:
			STcopy(NAM_SETUP, Ctype);
			break;
		default:
			/*Uh oh bad code*/
			return (FAIL);
		}

	Csequence = 0;
	STcopy(ERx(" "),Csection);
	STcopy(ERx(" "),Cattid);
	STcopy(ERx(" "),Ccommand);
	STcopy(ERx(""),Ctext);

	inquote = FALSE;
        skip_next = TRUE;
	for(;;)
	{
		type = s_g_skip(skip_next, &rtn_char);
		skip_next = TRUE;
		if ( type == TK_PERIOD)
		{
			_VOID_ s_w_row();
			break;
		}
		else if (type == TK_ENDFILE)
		{
			_VOID_ s_w_row();
			break;
		}
		else if (type == TK_QUOTE)
		{
			/*
			** This handles delimited identifiers as well as
			** constructs of the form '"a".["]bc["]'
			*/
			qt = s_g_ident(FALSE,&rtn_char,TRUE);
			if ((rtn_char != UI_DELIM_ID) || (!St_xns_given))
			{
				s_error(0x0A7,NONFATAL,qt,NULL);
			}
			s_cat(qt);
		}
		else if (type == TK_SQUOTE)
		{
			s_cat(s_g_string(FALSE, type));

		}
		else if (type == TK_RELOP || type == TK_EQUALS)
		{
			s_cat(s_g_op());
			skip_next = FALSE;
		}
		else if (type == TK_HEXLIT)
		{
			s_cat(ERx("X"));
			CMnext(Tokchar);
			s_cat(s_g_string(FALSE, TK_SQUOTE));
		}
		else
		{
			/*
			** If we have a construct 'a.["]bc["]', then ensure that
			** we don't separate the tokens.  Yes, this is a bit of
			** a kludge, and yes, it does assume a lot about how
			** things get parsed, and no, I'm not terribly thrilled
			** with it.  Still, this avoids overrun problems where
			** the compound identifier can wind up being be split
			** across a row!
			*/
			qt = s_g_token(FALSE);
			if  ((*Tokchar == '.') && (*(Tokchar + 1) == '\"'))
			{
				STcopy(qt,&compound_buf[0]);
				/*
				** We've already got the period, but Tokchar is
				** pointing to it, so we gotta skip over it.
				*/
				CMnext(Tokchar);
				qt = s_g_ident(FALSE,&rtn_char,FALSE);
				/*
				** We need to let UI_BOGUS_ID through,
				** or we'll choke on functions, AS
				** target specifications, etc.
				*/
				if ((rtn_char == UI_DELIM_ID) &&
				    (!St_xns_given))
				{
					s_error(0x0A7,NONFATAL,qt,NULL);
				}
				STcat(&compound_buf[0],qt);
				s_cat(&compound_buf[0]);
			}
			else
			{
				s_cat(qt);
			}
		}
		s_cat(ERx(" "));
	}
	get_em();
	return(type);

}
