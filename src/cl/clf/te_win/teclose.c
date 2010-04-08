
# include	<compat.h>
# include	<te.h>
# include	"telocal.h"

/*
** Copyright (c) 1985 Ingres Corporation
**
**	Name:
**		TEclose.c
**
**	Function:
**		Close the terminal
**		
**
**	Arguments:
**
**	Result:
**		Flush the contesnts of the write buffers and close the
**		the terminal.  TEclose does an implicit TErestore(TE_NORMAL).
**
**	Side Effects:
**		None
**
**	History:
**		10/83 - (mmm)
**			written
**
**
*/

STATUS
TEclose(void)
{
	TEflush();
	return(TErestore(TE_NORMAL));
}
