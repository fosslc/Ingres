
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<er.h>


/**
** Name:	vqerr.c -	process vq errors
**
** Description:
**	Call IIUGerr.
**
**	This file defines:
**
**	IIVQerr		error routine
**
** History:
**	06/29/89 (tom) - created
**
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changed 'errno' to 'errnum' cuz 'errno' is a macro in WIN/NT
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/



/*{
** Name:	IIVQerr - error routine
**
** Description:
**	call IIUGerr..
**
** Inputs:
**	i4 errnum		- ingres error number
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
*/
VOID 
IIVQerr(errnum, argcount, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
ER_MSGID errnum;					 
i4  argcount;
PTR	a1;
PTR	a2;
PTR	a3;
PTR	a4;
PTR	a5;
PTR	a6;
PTR	a7;
PTR	a8;
PTR	a9;
PTR	a10;
{


	IIUGerr(errnum, UG_ERR_ERROR, argcount, a1, a2, a3, a4, a5, 
		a6, a7, a8, a9, a10);
}



/*{
** Name:	IIVQer0 - error routine with no arguments
**
** Description:
**	call IIUGerr..
**
** Inputs:
**	ER_MSGID errnum		- ingres error number
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
*/
VOID 
IIVQer0(errnum) 
ER_MSGID errnum;				 
{
	IIUGerr(errnum, UG_ERR_ERROR, 0);
}

/*{
** Name:	IIVQer1 - error routine with one argument
**
** Description:
**	call IIUGerr..
**
** Inputs:
**	ER_MSGID errnum		- ingres error number
**	PTR a1;			- argument 1
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
*/
VOID 
IIVQer1(errnum, a1) 
ER_MSGID errnum;				 
PTR	a1;
{
	IIUGerr(errnum, UG_ERR_ERROR, 1, a1);
}

/*{
** Name:	IIVQer2 - error routine with 2 arguments
**
** Description:
**	call IIUGerr..
**
** Inputs:
**	ER_MSGID errnum		- ingres error number
**	PTR a1;			- argument 1
**	PTR a2;			- argument 2
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
*/
VOID 
IIVQer2(errnum, a1, a2) 
ER_MSGID errnum;				 
PTR	a1;
PTR	a2;
{
	IIUGerr(errnum, UG_ERR_ERROR, 2, a1, a2);
}

/*{
** Name:	IIVQer3 - error routine with three argument
**
** Description:
**	call IIUGerr..
**
** Inputs:
**	ER_MSGID errnum		- ingres error number
**	PTR a1;			- argument 1
**	PTR a2;			- argument 2
**	PTR a3;			- argument 3
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
*/
VOID 
IIVQer3(errnum, a1, a2, a3) 
ER_MSGID errnum;				 
PTR	a1;
PTR	a2;
PTR	a3;
{
	IIUGerr(errnum, UG_ERR_ERROR, 3, a1, a2, a3);
}

/*{
** Name:	IIVQer4 - error routine with 4 argument
**
** Description:
**	call IIUGerr..
**
** Inputs:
**	ER_MSGID errnum		- ingres error number
**	PTR a1;			- argument 1
**	PTR a2;			- argument 2
**	PTR a3;			- argument 3
**	PTR a4;			- argument 4
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
*/
VOID 
IIVQer4(errnum, a1, a2, a3, a4) 
ER_MSGID errnum;				 
PTR	a1;
PTR	a2;
PTR	a3;
PTR	a4;
{
	IIUGerr(errnum, UG_ERR_ERROR, 4, a1, a2, a3, a4);
}
