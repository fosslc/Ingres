/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sgnumber.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	 <cm.h> 

/*
**   S_G_NUMBER - get an string representing a number from the internal line. 
**	A number has the following format:
**		{digit}[.{digit}][e[+|-]{digit}]]
**
**	Parameters:
**		inc	TRUE if to point to character after last
**			character in number.
**
**	Returns:
**		pointer to name, which is an internal buffer.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		12.0, 12.7.
**
**	History:
**		11/22/83 (gac)	written.
**		24-mar-1987 (yamamoto)
**			Changed CH macros to CM.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

char	*
s_g_number(inc)
bool	inc;
{
	/* internal declarations */

	char	*start;			/* starting position of name */
	char	*hptr;
	static char	tok_buffer[MAXLINE+1];	/* place to put alpha name */
	i4             rtn_char;               /* dummy variable for sgskip */

	/* start of routine */


	start = tok_buffer;
	hptr = Tokchar;

	while (CMdigit(Tokchar))
	{
		CMcpyinc(Tokchar, start);
	}

	if (*Tokchar == '.')
	{
		CMcpyinc(Tokchar, start);
		while (CMdigit(Tokchar))
		{
			CMcpyinc(Tokchar, start);
		}
	}

	s_g_skip(FALSE, &rtn_char);

	if (*Tokchar == 'e' || *Tokchar == 'E')
	{
		CMcpyinc(Tokchar, start);

		if (s_g_skip(FALSE, &rtn_char) == TK_SIGN)
		{
			CMcpyinc(Tokchar, start);
		}

		s_g_skip(FALSE, &rtn_char);

		while (CMdigit(Tokchar))
		{
			CMcpyinc(Tokchar, start);
		}
	}

	if (!inc)
	{
		CMprev(Tokchar, hptr);
	}

	*start = '\0';


	return(tok_buffer);

}
