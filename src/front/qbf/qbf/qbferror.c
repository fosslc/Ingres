/*
**	qbferror.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>

/**
** Name:	qbferror.c - Error routine for QBF to use.
**
** Description:
**	Special version of FDerror() for QBF to use until
**	QBF is changed to use IIUGerr().
**
** History:
**	09/08/87 (dkh) - Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	FDerror - Output error message for QBF.
**
** Description:
**	Output error message for QBF.  This is a special version
**	that just calls IIUGerr().
**
** Inputs:
**	errnum	Error number of error message.
**	numargs	Number arguments passed in to print with error message.
**	a1	First argument.
**	a2	Second argument.
**	a3	Third argument.
**	a4	Fourth argument.
**	a5	Fifth argument.
**	a6	Sixth argument.
**	a7	Seventh argument.
**	a8	Eigth argument.
**	a9	Ninth argument.
**	a10	Tenth argument.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/08/87 (dkh) - Initial version.
*/
/* VARARGS2 */
VOID
FDerror(errnum, numargs, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )
ER_MSGID	errnum;
i4		numargs;
PTR		a1;
PTR		a2;
PTR		a3;
PTR		a4;
PTR		a5;
PTR		a6;
PTR		a7;
PTR		a8;
PTR		a9;
PTR		a10;
{
	IIUGerr(errnum, 0, numargs, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}
