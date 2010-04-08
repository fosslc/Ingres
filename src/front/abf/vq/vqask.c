
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<uf.h>
# include	<ug.h>


/**
** Name:	vqask.c -	ask a yes/no question
**
** Description:
**	Ask a yes no question and return the boolean result.
**
**	This file defines:
**
**	IIVQask		- ask y/n question
**
** History:
**	08/05/89 (tom) - created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	IIVQask - ask a yes/no question
**
** Description:
**	This function returns a boolean result of asking the user
**	a yes/no question.
**
** Inputs:
**	ER_MSGID prompt;	- message number to use for the prompt
**	i4 args;		- number of arguments
**	char *a1, *a2 ...	- variable number of arguments
**
** Outputs:
**	Returns:
**		bool		- TRUE or FALSE depending on user's answer
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	08/05/89 (tom) - created
*/
bool
IIVQask(prompt, args, a1, a2, a3, a4)
ER_MSGID prompt;
i4  args;
char *a1;
char *a2;
char *a3;
char *a4;
{

	char answer[200];

	IIUFask(ERget(prompt), FALSE, answer, args, a1, a2, a3, a4);

	return (IIUGyn(answer, (STATUS*)NULL));
}
