/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <er.h>

/**
** Name:	wordwrap.c - word wrap a message
**
** Description:
**	Defines
**		word_wrap	- word wrap a message
**
** History:
**	16-dec-96 (joea)
**		Created based on wordwrap.c in replicator library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define INDENT		10
# define WRAP_LENGTH	70	/* must be > INDENT */


/*{
** Name:	word_wrap - word wrap a message
**
** Description:
**	Puts in returns at appropriate points in a string, with the end of
**	line no greater than WRAP_LENGTH.  Subsequent lines are indented
**	INDENT spaces.
**
** Inputs:
**	message_string	- the string to be word wrapped
**
** Outputs:
**	message_string	- the resulting word-wrapped string
**
** Returns:
**	none
*/
void
word_wrap(
char	*message_string)
{
	u_i4	j, k;
	u_i4	indent;
	u_i4	tmp_length;
	i4	wrap_point = WRAP_LENGTH;
	char	tmp_string[1000];
	char	tmp_string2[1000];
	char	*s1, *s2;

	if (STlength(message_string) > sizeof(tmp_string))  /* just in case */
		return;

	STcopy(message_string, tmp_string);
	*message_string = EOS;
	indent = 0;
	tmp_length = STlength(tmp_string);
	s1 = tmp_string;
	for (j = 0; j < tmp_length; j += wrap_point)
	{
		s2 = s1;
		if (STlength(s1) > (u_i4)(WRAP_LENGTH - indent))
		{
			for (k = 0, wrap_point = 0; *s1 != EOS &&
				k < WRAP_LENGTH - indent; k++, CMnext(s1))
			{
				/*
				** find last delimiter before
				** WRAP_LENGTH - indent
				*/
				if (!CMwhite(s1) && k < WRAP_LENGTH - indent)
					wrap_point = k;
			}
			if (wrap_point != 0)
			{
				STlcopy(s2, tmp_string2, wrap_point);
				STcat(message_string, tmp_string2);
				s1 = s2 + wrap_point;
			}
			else
			{
				STlcopy(s2, tmp_string2, WRAP_LENGTH - indent);
				STcat(message_string, tmp_string2);
				wrap_point = WRAP_LENGTH - indent;
			}
			STcat(message_string, ERx("\n          "));
			indent = INDENT;
		}
		else	/* all done */
		{
			STcat(message_string, s1);
			wrap_point = WRAP_LENGTH - indent;
		}
	}
}
