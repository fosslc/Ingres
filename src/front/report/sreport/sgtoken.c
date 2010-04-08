/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char	Sccsid[] = "@(#)sgtoken.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cm.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 

/*
**   S_G_TOKEN - get a token from the input line, to transfer to the
**	forming command.  A token is ended by a blank, newline, tab,
**	comma, open or close parenthesis, colon, semicolon or a comment.
**	(jupbug #9936)
**
**	If the token starts with one of the delimiters,
**	it will be put into the token to start it.
**
**
**	Parameters:
**		inc	TRUE if to point to character after last
**			character in token.
**
**	Returns:
**		pointer to name, which is an internal buffer, which must
**		be moved after this call to save the string.
**
**	Error Messages:
**		none.
**
**	Trace Flags:
**		12.0, 12.3.
**
**	History:
**		5/29/81 (ps) - written.
**		11/22/83 (gac)	added inc.
**		7/6/89 (elein) garnet
**			Added single quote as an end to a token. Somewhere
**			along the line someone added double quotes.
**		24-jul-89 (sylviap)
**			B6866 - added equal sign as a token.
**		8/31/89 (elein) Back out fix for 6866.
**		9/14/89 (elein)
**			Ok, finally: = is back in and now ! has been added.
**			sqadd was changed to be able to handle these tokens
**		08-feb-90 (sylviap)
**			B9936 - added comments '\*' as a delimiter.
**		3/19/90 (elein) b20466
**			Adding case for < and >.  These are RELOPS
**			which WILL be handled correctly in sqadd (or else!)
**		2/4/91 (elein) b35943
**			break on end comment, too.
**		7/15/91 (elein)
**			porting change ingres63p 262575 by johnr made general.
**			Do not backslash exclamation character.
**		6/3/92	(dkhor)
**			change Tokchar-- to CMprev(Tokchar).  Also include
**			<cm.h> becos' CMprev is defined there.  Fix bug 45622.
**		8/4/92	(dkhor)
**			Back out (rejected) changes made on 6/3/92 
**			(change # 266838).
**		27-aug-92 (dkhor)
**			Re-submit bug fix 45622 in accordance to suggestions
**			by rdrane.
**		17-may-1993 (rdrane)
**			Add support for hex literals.  We look for the initial
**			'x' or 'X' followed immediately by a single quote.
**			If present, copy the 'x' into the buffer and treat as
**			any other single quoted string.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char	*
s_g_token(inc)
bool	inc;
{
	/* internal declarations */

	char		*start_tokchar;
	register char	*start;		/* starting position of name */
	static char	tok_buffer[MAXLINE+1];	/* buffer to contain the token. */
	i4	count = MAXLINE;	/* count of number of chars in token */
	i4	bytecnt;

	/* start of routine */


	start = tok_buffer;
	if ((*Tokchar == '\0') || (*Tokchar == '\n') || (*Tokchar== '\t')
		|| (*Tokchar == ' '))
	{	/* token starts with this kind of delimiter.  Get out */
		*start = '\0';
		return(tok_buffer);
	}

	start_tokchar = Tokchar;
	CMcpyinc(Tokchar, start);
	if  (((*start_tokchar == 'x') || (*start_tokchar == 'X')) &&
	     (*Tokchar == '\''))
	{
		CMcpyinc(Tokchar,start);
	}


	/* B6866 - added equal sign */

	while ((*Tokchar != ' ') && (*Tokchar != '\t') 
		&& (*Tokchar != ',') && (*Tokchar != '(')
		&& (*Tokchar != ')') && (*Tokchar != '\"')
		&& (*Tokchar != ':') && (*Tokchar != ';')
		&& (*Tokchar != '\0') && (*Tokchar != '\n')
		&& (*Tokchar != '\'') && (*Tokchar != '=')
		&& (*Tokchar != '>') && (*Tokchar != '<')
		&& (*Tokchar != '!') ) 
	{
		if ((*Tokchar == '/') && (*(Tokchar+1) == '*'))
			/* break if encounters a comment */
			break;
		if ((*Tokchar == '*') && (*(Tokchar+1) == '/'))
			/* break if encounters an end comment */
			break;

		/* see if we have enough buffer space */
		bytecnt = CMbytecnt(Tokchar);
		if ((count - bytecnt) > 0)
		{
			count -= bytecnt;
			CMcpyinc(Tokchar, start);
		}
		else
			CMnext(Tokchar); 	/* buffer full; move on */
	}

	if (!inc)
	{
		CMprev(Tokchar, start_tokchar);
	}

	*start = '\0';


	return(tok_buffer);

}
