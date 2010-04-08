/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sgop.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<cm.h>

/*
**   S_G_OP - get an operator from the input line, to transfer to the
**	forming command.
**
**
**	Parameters:
**		none.
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
**		11/22/83 (gac)	written.
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		3/5/90 (elein)
**			<> is a 2 character symbol. Need to
**			recognize it.
*/



char	*
s_g_op()
{
	/* internal declarations */

	register char	*start;		/* starting position of name */
	static char	tok_buffer[MAXLINE+1];	/* buffer to contain the token. */
	char		*pptr;

	/* start of routine */


	start = tok_buffer;
	pptr = Tokchar;
	CMcpyinc(Tokchar, start);

	switch (*pptr)
	{
		case('<'):
			if (*Tokchar == '>')
			{
				*start++ = *Tokchar++;
				break;
			}
			/*NO BREAK*/
		case('>'):
		case('!'):
			if (*Tokchar == '=')
			{
				*start++ = *Tokchar++;
			}
			break;

		case('*'):
			if (*Tokchar == '*')
			{
				*start++ = *Tokchar++;
			}
			break;

		default:
			break;

	}

	*start = '\0';


	return(tok_buffer);

}
