/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<pc.h>		 
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"cgerror.h"

/**
** Name:	cgerror.c -		Code Generator Error Module.
**
** Description:
**	Generates error messages for the code generator which produces
**	C code from the OSL intermediate language.  Defines:
**
**	IICGerrError()
**	IICGpreProErr()
**
** History:
**	Revision 6.0  89/02  arthur
**	Initial revision.
**	07-aug-1987 (arthur)
**		Changed to use ER module.
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changed 'errno' to 'errnum' cuz 'errno' is a macro in WIN/NT
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static VOID	_exitCodeGen();

/*{
** Name:	IICGerrError() -	Main error handling routine
**
** Description:
**	Main error handling routine.
**
** Inputs:
**	errnum		The internal error number.
**	a1-8		The arguments.
**
** History:
**	feb-1987 (arthur) Written.
*/
/* VARARGS1 */
VOID
IICGerrError(errnum, a1, a2, a3, a4, a5, a6, a7, a8)
ER_MSGID	errnum;					 
char		*a1,
		*a2,
		*a3,
		*a4,
		*a5,
		*a6,
		*a7,
		*a8;
{
	i4	count;

	if (a1 == NULL)
		count = 0;
	else if (a2 == NULL)
		count = 1;
	else if (a3 == NULL)
		count = 2;
	else if (a4 == NULL)
		count = 3;
	else if (a5 == NULL)
		count = 4;
	else if (a6 == NULL)
		count = 5;
	else if (a7 == NULL)
		count = 6;
	else if (a8 == NULL)
		count = 7;
	else
		count = 8;

	IIUGerr(errnum, 0, count, a1, a2, a3, a4, a5, a6, a7, a8);
	_exitCodeGen(errnum);
}

/*{
** Name:	IICGpreProErr() -	Error in an internal routine
**
** Description:
**	Error in an internal routine.
**	Calls IICGerrError() with the routine name as the first
**	character string argument.
**
** Inputs:
**	routine		The name of the routine where the error was discovered.
**	errnum		The internal error number.
**	a1-7		The arguments.
**
** History:
**	feb-1987 (arthur) Written.
*/
/* VARARGS2 */
VOID
IICGpreProErr(routine, errnum, a1, a2, a3, a4, a5, a6, a7)
char		*routine;
ER_MSGID	errnum;					 
char		*a1,
		*a2,
		*a3,
		*a4,
		*a5,
		*a6,
		*a7;
{
	IICGerrError(errnum, routine, a1, a2, a3, a4, a5, a6, a7);
}

/*{
** Name:	_exitCodeGen() - Exit from the code generator
**
** Description:
**	Exits from the code generator.
**	Called from IICGerrError() if a fatal error has been detected.
**
** Inputs:
**	status		Exit status.
**
** History:
**	feb-1987 (arthur) Written.
*/
static VOID
_exitCodeGen ( status )
i4	status;
{
	IICGoflFlush();
	PCexit(status);
}
