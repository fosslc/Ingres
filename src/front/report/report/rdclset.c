
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<afe.h>
# include	<fedml.h>
# include	<cm.h>
# include	<er.h>
# include	<errw.h>


/**
** Name:	rdclset.c	Contains routine to set up declared variables.
**
**
** Description:
**	This file defines:
**
**	r_dcl_set	Sets up the declared variables in a report and prompts
**			for the value of any variable declared with a prompt
**			string.
**
** History:
**	15-jun-87 (grant)	Written.
**	1/11/90  (martym)   Changed to pass in parameter to prompt user or not.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
	char	*r_g_paren();
/* static's */

#define  GOT_ERROR   -1
#define  GOT_NOT      0
#define  WANT_WITH    1
#define  GOT_WITH     2
#define  WANT_PROMPT  3
#define  WANT_VALUE   4

/*{
** Name:  r_dcl_set	Sets up the declared variables in a report and prompts
**			for the value of any variable declared with a prompt
**			string.
**
** Description:
**	This routine sets up the declared variables by placing them in the
**	runtime parameter list.  Any variable which has a prompt string is
**	prompted for its value.
**
** Inputs:
**		PromptUser -- TRUE will prompt user, FALSE will not.
**
** Outputs:
**	none
**
**	Returns:
**		none.
**
** History:
**	15-jun-87 (grant)	written.
**	1/10/90 (martym)	Added the parameter PromptUser.
**      3/20/90 (elein) performance
**                      Change STcompare to STequal
**	7/18/90 (elein)
**		Add recognition of WITH VALUE option to .DECLARE
**	25-may-1993 (rdrane)
**		Allow WITH VALUE to be a hexadecimal string literal.
**		Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.
**	14-dec-2007 (smeke01)
**		Make WITH VALUE & WITH PROMPT work correctly together.
**		Made sure that the default value is saved in our
**		parameter lookup table, even when there's a prompt.
**	13-Jun-2007 (kibro01) b118486
**		Note that we are not here setting a value through CLI
*/

VOID
r_dcl_set(PromptUser)
bool PromptUser;
{
    DCL			*dcl;
    i4			token_type;
    i4			state;
    DB_USER_TYPE	user_type;
    DB_DATA_VALUE	datatype;
    ITEM		item;
    char		*token;
    char		*prompt_string;
    char		*value;
    i4			curr_len;
    i4			token_len;
    bool		nullable;
    STATUS		status;
    bool		found_null;
    bool		found_error;
    char		tmp_buf[MAXRTEXT];

    /* put each declared variable in the runtime parameter list */

    found_error = FALSE;
    for (dcl = Ptr_dcl_top; dcl != NULL; dcl = dcl->dcl_below)
    {
	/* point to beginning of variable datatype which follows = sign */
	r_g_set(dcl->dcl_datatype);

	user_type.dbut_kind = DB_UT_NOTNULL;
	STcopy(ERx(""), user_type.dbut_name);

	state = WANT_WITH;
	prompt_string = NULL;
	value = NULL;
	found_null = FALSE;
	/*
	** Pick up variable type.  Keep appending until
	** a character is found which is not an alpha
	** or a paren OR until the words 'not' or 'with'
	** are found.  If "not" or "with" are found, 
	** set the state and advance to the next word.
	**/
	for (;;)
	{
	    token_type = r_g_skip();
	    if (token_type == TK_ALPHA)
	    {
	        token = r_g_name();
	    }
	    else if (token_type == TK_OPAREN)
	    {
		token = r_g_paren();
	    }
	    else
	    {
		break;
	    }

	    if (STequal(token, ERx("not")))
	    {
		state = GOT_NOT;
	        token_type=r_g_skip();
		break;
	    }
	    else if (STequal(token, ERx("with")))
	    {
		state = GOT_WITH;
	        token_type=r_g_skip();
		break;
	    }
	    else
	    {
		curr_len = STlength(user_type.dbut_name);
		if (curr_len > 0  && curr_len < DB_TYPE_SIZE &&
		    token_type == TK_ALPHA)
		{
		    STcat(user_type.dbut_name, ERx(" "));
		    curr_len++;
		}

		token_len = STlength(token);
		if (curr_len+token_len > DB_TYPE_SIZE)
		{
		    *(token+DB_TYPE_SIZE-curr_len) = EOS;
		}

		STcat(user_type.dbut_name, token);
	    }
	}

	/*
	** usertype created.  Make into adf type
	*/
	status = afe_tychk(Adf_scb, &user_type, &datatype);
	if (status != OK)
	{
	    r_error(0x34, NONFATAL, dcl->dcl_name, Cact_command, Cact_rtext,
		    NULL);
	    continue;
	}

	nullable = (En_qlang == FEDMLSQL);


	/*
	** At the beginning, we should be advanced to the next word
	** and in state: WANT_WITH, GOT_NOT or GOT_WITH from 
	** previous loop.  (WANT_WITH allows either "with" or "not")
	**
	** At the end of each iteration, advance to next word.  The loop
	** ends on error or we run out of words (token_type will be
	** TK_ENDSTRING and there will be no more dcls for this
	** variable)   It is an error to reach ENDSTRING with no
	** more dcls if we are looking for anything other than a WITH
	*/
	for(; state != GOT_ERROR; token_type = r_g_skip())
	{
	   if( token_type == TK_ENDSTRING )
	   {
		/* 
		** Check next line for continuation of this variable
		*/
		if (dcl->dcl_below != NULL &&
		    STequal(dcl->dcl_name, dcl->dcl_below->dcl_name) )
		{
			dcl = dcl->dcl_below;
			r_g_set(dcl->dcl_datatype);
			token_type = r_g_skip();
		}
		else if (state == WANT_WITH)
		{
			break;
		}
		else
		{
		         r_error(0x32, NONFATAL, dcl->dcl_name, Cact_command,
		                 Cact_rtext, NULL);
		         state  = GOT_ERROR;
		  	 break;
		}
	   }
	   switch( state)
	   {
		/*
		** "with" or "not" are valid here 
		*/
		case(WANT_WITH):
		   token = r_g_name();
	    	   if (STequal(token, ERx("with")) )
		   {
			state = GOT_WITH;
		   }
		   else if (STequal(token, ERx("not")) )
		   {
		   	state = GOT_NOT;
		   }
		   else
		   {
			r_error(0x32, NONFATAL, dcl->dcl_name, Cact_command,
			        Cact_rtext, NULL);
			state  = GOT_ERROR;
		   }
		   break;

		/*
		** "null","prompt","value" are valid here 
		*/
		case(GOT_WITH):
		   token = r_g_name();
	    	   if (STequal(token, ERx("null")) )
		   {
			if( found_null)
			{
				r_error(0x32, NONFATAL, dcl->dcl_name,
					Cact_command, Cact_rtext, NULL);
				state  = GOT_ERROR;
				break;
			}
			nullable   = TRUE;
			state      = WANT_WITH;
			found_null = TRUE;
		   }
	    	   else if (STequal(token, ERx("prompt")) )
		   {
			state = WANT_PROMPT;
		   }
	    	   else if (STequal(token, ERx("value")) )
		   {
			state = WANT_VALUE;
		   }
	    	   else
		   {
			r_error(0x32, NONFATAL, dcl->dcl_name, Cact_command,
			        Cact_rtext, NULL);
			state  = GOT_ERROR;
		   }
		   break;

		/*
		** String constant valid here
		*/
		case(WANT_PROMPT):
		/* NO BREAK */
		case(WANT_VALUE):
	           if (token_type == TK_QUOTE || token_type == TK_SQUOTE)
	           {
		       if( state == WANT_PROMPT)
		       {
		          prompt_string = r_g_string(token_type);
		          r_strpslsh(prompt_string);
		       }
		       else
		       {
			  value = r_g_string(token_type);
		          r_strpslsh(value);
		       }
		       state  = WANT_WITH;
	           }
	           else if (token_type == TK_HEXLIT)
		   {
			  CMnext(Tokchar);
			  STcopy(ERx("X'"),&tmp_buf[0]);
			  value = r_g_string(TK_SQUOTE);
			  STcat(&tmp_buf[0],value);
			  MEfree(value);
			  value = STalloc(STcat(&tmp_buf[0],ERx("\'")));
			  state  = WANT_WITH;
		   }
	           else
	           {
		       r_error(0x32, NONFATAL, dcl->dcl_name, Cact_command,
		 	      Cact_rtext, NULL);
		       state  = GOT_ERROR;
                   }
                   break;


		case(GOT_NOT):
		   token = r_g_name();
	    	   if (STequal(token, ERx("null")) )
		   {
			if( found_null)
			{
				r_error(0x32, NONFATAL, dcl->dcl_name,
					Cact_command, Cact_rtext, NULL);
				state  = GOT_ERROR;
				break;
			}
			nullable = FALSE;
			found_null = TRUE;
		   }
		   state = WANT_WITH;
		   break;
	   }
	}

	if( state == GOT_ERROR)
	{
		found_error = TRUE;
		continue;
	}

	if (nullable)
	{
	    AFE_MKNULL_MACRO(&datatype);
	}

	/* now put declared variable in the runtime parameter list */
	r_par_put(dcl->dcl_name, value, &datatype, prompt_string, FALSE);
    }

    /* prompt for declared variables with prompt string
    ** but don't bother if there were errors--we're gonna
    ** bomb anyway
    */
    if (PromptUser && found_error==FALSE)
	r_fix_parl(TRUE);	
}
