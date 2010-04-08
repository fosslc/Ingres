/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<cm.h>
# include	<er.h>

# define	W_NAME		1	/* state: want a param name */
# define	W_EQUALS	2	/* state: want an equal sign */
# define	W_VALUE		3	/* state: want a param value */

/*
**   R_CRK_PAR - crack out the parameters specified on the command line,
**	and stick them into the PAR structures for later use.
**
**	The parameters are specified in the command as in:
**		{ parametername = parameter value  }
**
**	(Note that parameter names may be specified with or without
**	 a leading dollar sign).  
**
**	Parameters:
**		string - address of parenthesized list of parameters.
**
**	Returns:
**		NULL - normal exit.
**		*c - location where error found in parameters.
**
**	Called by:
**		r_crack_cmd.
**
**	Side Effects:
**		Sets the Ptr_par_top ptr.
**		Sets up and changes Tokchar.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		1.0, 1.2.
**
**	History:
**		6/6/81 (ps) - written.
**		4/5/84	(gac)	^ in parameter strings causes next letter
**				to be uppercase.
**		2/13/86 (mgw)	Fixed bug 7966 - Added parameter to r_g_string
**
**		6-aug-1986 (Joe)
**			Added support for single quoted strings.
**			Check for TK_SQUOTE, pass to r_g_string.
**			Also took out the strip slash parameter to r_g_string.
**		27-oct-86 (mgw)	bug #10676
**			stripped the slashes from value after call to
**			r_g_string.
**		25-nov-1986 (yamamoto)
**			Modified for double byte characters.
**		11-jul-89 (sylviap)
**			Added DG change to accept curly brackets for parameters.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		4/12/90 (elein) b21085
**			Remove ^ as a special character,
**			reverses gac's 1984 change
**		9/4/90 (elein) b33240
**			allow '(x=)' parameter.
**      19-mar-97 (rodjo04)
**          Now when an number is encountered immediately following
**          an equal sign, It will assume it is a value. EX '(x=12c2)'
**      19-Aug-1997 (rodjo04) bug 84614.
**          When '(x=12c2)' is encountered, call  r_g_nname() instead of 
**          r_g_name().  
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jun-2007 (kibro01) b118486
**	    Note that we set this parameter through the CLI
**      20-Aug-2007 (kibro01) b118976
**          Ensure that quoted parameters are accepted like normal parameters
*/

char	*		/* GAC -- added cast */
r_crk_par(string)
register char	*string;
{
	/* internal declarations */

	char		*name = NULL;		/* ptr to param name */
	char		*value = NULL;		/* ptr to parm value */
	char		*temp = NULL;		/* (mmm) added to use CVlower instead of lowercase */
	i2		state = W_NAME;		/* state (what we want) */
	char		*src;
	char		*dst;
	i4		tokvalue;

	/* start of routine */


	if (string == NULL)
	{
		return(NULL);
	}

	/* If this is quoted, remove the quotes from both ends (b118976) */
	if (string[0] == '"')
	{
		i4 slen = STlength(string);
		if (string[slen-1] == '"')
		{
			string[slen-1] = '\0';
			string++;
		}
		
	}

	r_g_set(string);		/* set up string for token routines */

	for (;;)
	{
		switch (state)
		{
		case(W_NAME):
			/* want a parameter name */
			switch(r_g_skip())
			{
			case(TK_ENDSTRING):
			case(TK_CPAREN):
			case(TK_RCURLY):		/* added for DG */
				/* end of param list.  Get out */


				return(NULL);

# ifdef DGC_AOS
			case(TK_COLON):
# endif
			case(TK_COMMA):
			case(TK_OPAREN):
			case(TK_LCURLY):		/* added for DG */
			case(TK_DOLLAR):
				CMnext(Tokchar);
				continue;	/* skip it */

			case(TK_ALPHA):
			case(TK_NUMBER):
				/* valid (enough) param name */
				name = r_g_name();
				CVlower(name);
				break;

			default:
				/* huh??? */
				return (Tokchar);
			}
			state = W_EQUALS;
			break;

		case(W_EQUALS):
			/* need an equal sign (or keyword IS) */
			switch(r_g_skip())
			{
			case(TK_EQUALS):
				CMnext(Tokchar);
				break;		/* o.k. */

			case(TK_ALPHA):
				temp = r_g_name();
				CVlower(temp);
				if (STequal(temp, ERx("is")) )
				{
					break;		/* o.k. */
				}
				/* drop through to error */

			default:
				return(Tokchar);		/* not o.k. */

			}
			state = W_VALUE;
			break;

		case(W_VALUE):
			/* want a value for the parameter */
			switch(tokvalue = r_g_skip())
			{
			case(TK_SIGN):
            case(TK_NUMBER):
				value = r_g_nname();
				break;
          
            case(TK_ALPHA):
				/* looks good, go on */
				value = r_g_name();
				break;

			case(TK_QUOTE):
			case(TK_SQUOTE):
				value = r_g_string(tokvalue);
				r_strpslsh(value);
				break;
			case(TK_CPAREN):
				value = r_g_name();
				break;

			default:
				/* huh ??? */
				return(Tokchar);

			}

			/* should have a name/value pair. Set up PAR */

			r_par_clip(name);
			/* Note that we set through CLI (kibro01) b118486 */
			r_par_put(name, value, NULL, NULL, TRUE);

			state = W_NAME;
			break;
		}
	}
}

