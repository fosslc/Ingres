/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<cm.h>
# include	<er.h>

/*
**   S_G_PAREN - pull out the text between matching parentheses from
**	the internal line.  This is put into an internal static buffer,
**	with extra blanks, tabs removed.  Nested parentheses are handled
**	correctly.
**
**	Note that the opening and closing parentheses are put into the
**		buffer.
**
**	Parameters:
**		inc	TRUE if Tokchar is to point to character after last
**			character in parentheses.
**
**	Returns:
**		address of static buffer containing string.
**
**	Error Messages:
**		930.
**		Syserr:Improper Return.
**
**	Trace Flags:
**		12.0, 12.4.
**
**	History:
**		5/29/81 (ps) - written.
**		08/06/86 (drh)	Added TK_SQUOTE support
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char	*
s_g_paren(inc)
bool	inc;
{
	/* internal declarations */

	static	char	tok_buffer[MAXLINE+1];	/* contains string */
	register i4	count = MAXLINE;	/* count of number of chars found */
	i4		level = 1;		/* level of nesting of parens */
	char		*delim = ERx("");	/* add as separation between tokens */
	char		*qt;			/* ptr to token */
	i4             leng = 0;               /* length of token */
	i4             rtn_char;               /* dummy variable for sgskip */

	/* start of routine */



	STcopy(ERx("("),tok_buffer);
	count--;

	for(;;)
	{
		switch(s_g_skip(TRUE, &rtn_char))
		{
			case(TK_OPAREN):
				if (--count > 0)
				{	/* add paren */
					STcat(tok_buffer, ERx("("));
				}
				level++;
				delim=ERx("");
				break;

			case(TK_CPAREN):
				if (count-- > 0)
				{
					STcat(tok_buffer, ERx(")"));
				}
				if (--level <= 0)
				{


					if (inc)
					{
						CMnext(Tokchar);
					}

					return(tok_buffer);
				}
				delim=ERx("");
				break;


			case(TK_ENDFILE):

				r_error(930, FATAL, NULL);

			case(TK_QUOTE):
				qt = s_g_string(FALSE, (i4) TK_QUOTE);
				leng = STlength(delim);
				if (count > leng)
				{	/* add delimiter */
					STcat(tok_buffer, delim);
					count -= leng;
				}	/* add delimiter */
				delim = ERx(" ");	/* change delimiter to blank */

				leng = STlength(qt);
				if (count > leng)
				{	/* add string */
					STcat(tok_buffer, qt);
					count -= leng;
				}	/* add string */
				break;

			case(TK_SQUOTE):
				qt = s_g_string(FALSE, (i4) TK_SQUOTE);
				leng = STlength(delim);
				if (count > leng)
				{	/* add delimiter */
					STcat(tok_buffer, delim);
					count -= leng;
				}	/* add delimiter */
				delim = ERx(" ");	/* change delimiter to blank */

				leng = STlength(qt);
				if (count > leng)
				{	/* add string */
					STcat(tok_buffer, qt);
					count -= leng;
				}	/* add string */
				break;

			case(TK_COMMA):
				if (--count > 0)
				{
					STcat(tok_buffer, ERx(","));
				}
				delim = ERx("");
				break;

			default:
				/* must be token add it */
				qt = s_g_token(FALSE);
				leng = STlength(delim);
				if (count > leng)
				{
					STcat(tok_buffer, delim);
					count -= leng;
				}
				delim = ERx(" ");	/* reset delimiter */
				leng = STlength(qt);
				if (count > leng)
				{
					STcat(tok_buffer, qt);
					count -= leng;
				}
				break;

		}
	}

	/* should never get this far */

}
