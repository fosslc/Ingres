/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sgset.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 

/*
**   S_G_SET - set of routines for reading in tokens from the input line
**	These routines can return strings (quote delimited), tokens
**	(delimited by blanks, etc) or parenthesized lists(nested to any
**	depth).  A global character pointer (Tokchar)
**	is used to handle the locations on the line.
**	The line must be ended with an endofstring ('\0' character). 
**
**	The following routines are included in this set:
**		s_g_set - set up the internal pointers, flags, etc.
**			This must be called to preset the line.
**		s_g_skip - skip white spaces.  This 
**			can be used also to detect the type of token.
**			Also skips all comments, and reads in new lines.
**		s_g_name - return an INGRES style name.
**		s_g_string - return the string, up to the quote delimiter.
**		s_g_token - return a token (up to comma, blank, nl, tab or endstring)
**		s_g_paren - return stuff between parentheses.
**
**	Parameters:
**		string - start of the string to start the token routines.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets up Tokchar, the global character pointer.
**
**	Error messages:
**		none.
**
**	Trace Flags:
**		12.0, 12.1.
**
**	History:
**		5/29/81 (ps) - written.
*/



s_g_set(string)
char	*string;		/* start of line */
{
	/* start of routine */


	Tokchar = string;
	return;
}
