/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>


/**
** Name:	ftswapprn.c -	Exchange certain characters for others as
**				required for particular environments.
**
** Description:
**	This file defines:
**
**	FTswapparens		Exchange opening and closing brackets in
**				a string.
**
** History:
**	06-may-87 (bab)
**		Written for Hebrew project.
**/

/*{
** Name:	FTswapparens	- change all open/close parentheses, braces,
**				  brackets and <>'s to their appropriate
**				  counterparts.
**
** Description:
**	This routine swaps all open X's (X is one of the above objects)
**	for close X's, and visa-versa.  This is so that when a visually
**	correct string for an RL field (i.e.  *[z-a]) is reversed, it will
**	still parse correctly (i.e. as [a-z]* instead of ]a-z[*).
**
** Inputs:
**	string	The string to 'reverse'.
**
** Outputs:
**	string	The reversed string.  Reversal is done in place.
**
** Returns:
**	None
**
** Exceptions:
**	None
**
** Side Effects:
**
** History:
**	06-may-87 (bab)
**		Written for Hebrew project.
*/
VOID
FTswapparens(string)
register char	*string;
{
	for (; *string; string++)
	{
		if (*string == '(')
			*string = ')';
		else if (*string == ')')
			*string = '(';
		else if (*string == '[')
			*string = ']';
		else if (*string == ']')
			*string = '[';
		else if (*string == '{')
			*string = '}';
		else if (*string == '}')
			*string = '{';
		else if (*string == '<')
			*string = '>';
		else if (*string == '>')
			*string = '<';
	}
}
