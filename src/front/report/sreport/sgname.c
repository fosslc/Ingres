/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sgname.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	 <cm.h> 

/*
**   S_G_NAME - get an alphanumeric string from the internal line.  An
**	alphanumeric can contain letters, digits or underscore characters
**	(_).
**
**	Parameters:
**		inc	TRUE if Tokchar is to point to character after last
**			chararacter of name.
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
**		6/1/81 (ps) - written.
**		11/22/83 (gac)	added inc.
**		24-mar-1987 (yamamoto)
**			Changed CH macros to CM.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char	*
s_g_name(inc)
bool	inc;
{
	/* internal declarations */

	char	*start;			/* starting position of name */
	char	*hptr;
	static char	tok_buffer[MAXLINE+1];	/* place to put alpha name */
	i4	count = MAXLINE;	/* count of name size */

	/* start of routine */


	start = tok_buffer;
	hptr = Tokchar;

	while (CMnmchar(Tokchar))
	{
		CMbytedec(count, Tokchar);
		if (count > 0)
		{
			CMcpychar(Tokchar, start);
			CMnext(start);
		}
		CMnext(Tokchar);
	}

	if (!inc)
	{
		CMprev(Tokchar, hptr);
	}

	*start = '\0';


	return(tok_buffer);

}
