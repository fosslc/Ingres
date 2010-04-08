# include	<compat.h>
# include	<gl.h>
# include	<te.h>
# include	<si.h>
# include	"telocal.h"

GLOBALREF bool	TEcmdEnabled;


/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		TEwrite.c
**
**	Function:
**		Write buffer to terminal
**		
**
**	Arguments:
**		char	*buffer;
**		i4	length;
**
**	Result:
**		When the application knows that it is writing more than
**		one character it should call this routine.  TEwrite 
**		efficiently moves many characters to the bufferand then
**		to the terminal.
**
**	Side Effects:
**		None
**
**	History:
**		10/83 - (mmm)
**			written
**
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	29-may-1997 (canor01)
**	    Use new-style function declarations.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Update includes for renamed header
*/

VOID
TEwrite(char *buffer, i4 length)
{
	i4	dummy_count;

	if (! TEcmdEnabled)
		SIwrite(length, buffer, &dummy_count, stdout);
}

VOID
TEcmdwrite(char *buffer, i4 length)
{
	i4	dummy_count;

	if (TEcmdEnabled)
	{
		SIwrite(length, buffer, &dummy_count, stdout);
		_VOID_ TEflush();
	}
}

VOID
TEput(char c)
{
    if (! TEcmdEnabled)
	putc(c, stdout);
}


