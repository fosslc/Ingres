/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<si.h>
# include	<er.h>

/*{
** Name:	r_prompt() -	Prompt For Parameter Input.
**
** Description:
**   R_PROMPT - send a prompt to the user for entry of a parameter.
**	This is used when the -p option is specified and some of
**	the required parameters are not specified.
**	This prompts the user for a parameter value until it
**	is specified, unless a blank only is given.
**
** Parameters:
**	prompt	string containing prompt.
**	forceit TRUE if user must enter a non-blank value.
**
** Returns:
**	result	address of internal buffer containing
**		returned string from user.
**
** Called by:
**	r_crack, r_par_get.
**
** Side Effects:
**	none.
**
** Error Messages:
**	Syserr: Can't read input.
**
** Trace Flags:
**	2.0, 2.9.
**
** History:	
**		Revision 50.0  86/06/17  11:08:53  wong
**		Clean-up (lint, etc., from 4.0/02b)
**
**		6/30/81 (ps) 	written.
**		7/19/82 (ps)	change to 255 character buffer.
**		11/5/82 (ps)	add forceit parameter.
**		5/9/90 (elein) 21598
**			If Silent do it anyway.
*/

char *
r_prompt (prompt, forceit)
char	*prompt;
bool	forceit;
{
	static char	lbuf[256];		/* holds the prompt */

	/* start of routine */

	FEprompt(prompt, forceit, sizeof(lbuf)-1, lbuf);
	return lbuf;
}
