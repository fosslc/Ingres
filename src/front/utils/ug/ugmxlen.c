/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>


/**
** Name:	ugmxlen.c	- Find maximum line length of multiline string
**
** Description:
**	This file defines:
**
**	IIUGmllMaxLineLen	- Find maximum line length of multline string
**
** History:
**	2/10/90 (mikes)	Initial version
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name: IIUGmllMaxLineLen	- Find maximum line length of multline string
**
** Description:
**	Find the longest line in a (possibly) multiline string.  The lines
**	are separated by a newline character, which is not considered part
**	of any line.
**
** Inputs:
**	string		(char *)	multiline string
**
** Outputs:
**	none
**
**	Returns:
**		length of longest line (i4)
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	2/10/90	(Mike S)	Initial version
**	23-dec-1997 (fucch01)
**	    Casted _nl in call of STindex due to sgi_us5's picky compiler.
**          Was getting incompatible type error that prevented compilation.
*/
i4
IIUGmllMaxLineLen(string)
char	*string;
{
	i4	maxlen = 0;		/* Maximum length so far */
	char	*linestart = string;	/* Start of current line */
	char	*nlptr;			/* Newline which ends current line */

	if ((char *)NULL == string)
		return 0;

	for ( ; ; )
		{
		/* Check for newlines */
		nlptr = STchr(linestart, '\n');
		if ((char *)NULL == nlptr)
		{
			/* No newlines left */
			maxlen = max(maxlen, STlength(linestart));
			return (maxlen);
		}
		else
		{
			/* Found a newline. Check the current line's length */
			maxlen = max(maxlen, nlptr - linestart);
			linestart = nlptr + 1;
		}
	}
}
