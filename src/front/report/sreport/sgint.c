/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sginteger.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	 <cm.h> 

/*
**   S_G_INTEGER - get an string representing an integer from the internal line. 
**	An integer has the following format:
**		digit{digit}
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
**		Changed CH macro to CM.
*/



char	*
s_g_integer(inc)
bool	inc;
{
	/* internal declarations */

	char	*start;			/* starting position of name */
	char	*hptr;
	static char	tok_buffer[MAXLINE+1];	/* place to put alpha name */

	/* start of routine */


	start = tok_buffer;
	hptr = Tokchar;

	while (CMdigit(Tokchar))
	{
		CMcpyinc(Tokchar, start);
	}

	if (!inc)
	{
		CMprev(Tokchar, hptr);
	}

	*start = '\0';


	return(tok_buffer);

}
