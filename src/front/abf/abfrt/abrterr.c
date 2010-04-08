/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<tr.h>		/* 6-x_PC_80x86 */
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<abftrace.h>

/**
** Name:	abrterr.c -	ABF Run-Time System Error Reporting Module.
**
** Description:
**	Contains routines that report errors for ABF and the ABF run-time
**	system.  Defines:
**
**	iiarUsrErr()	report user error.
**	abusrerr()	report user error.
**	abproerr()	report internal program error.
**	aberrlog()	log error.
**
** History:
**	Revision 8.0  89/08  wong
**	Added 'iiarUsrErr()'.  Removed unused 'abingerr()' and 'absyserr()'.
**
**	Revision 6.0  87/08  wong
**	Added ER support.
**
**	Revision 2.0  82/10  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

static VOID	errreport();

/*{
** Name:	iiarUsrErr() -	Report User (or Application Developer) Error.
**
** Description:
**	Used to report errors caused by the end-user in the operation of an
**	application, or by the application developer in the specification of
**	the application operation.
**
** Inputs:
**	err	{ER_MSGID}  The error number.
**	argn	{nat}  The number of arguments.
**	a1-a10	{PTR}  The arguments.
**
** History:
**	08/89 (jhw) -- Written.
*/
VOID
iiarUsrErr ( err, argn, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )
ER_MSGID err;
i4	argn;
PTR	a1,
	a2,
	a3,
	a4,
	a5,
	a6,
	a7,
	a8,
	a9,
	a10;
{
	IIUGerr( err, UG_ERR_ERROR,
			argn, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10
	);
}

/*{
** Name:	abusrerr() -	Report User Error.
**
** Description:
**	Report an error caused by a user action.
**
** Input:
**	errnum	{ER_MSGID}  The internal error number.
**	a1-8	{char *}  The arguments.
**
** History:
**	Written (jrc)
*/
/*VARARGS2*/
VOID
abusrerr (errnum, a1, a2, a3, a4, a5, a6, a7, a8)
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
    errreport(errnum, a1, a2, a3, a4, a5, a6, a7, a8);
}

/*{
** Name:    abproerr() -	Report Internal Program Error.
**
** Description:
**	Report an internal program error.  (Essentially a bug-check.)
**
** Input:
**	routine	{char *}  The name of the routine in which the error occurred.
**	errnum	{ER_MSGID}  The internal error number.
**	a1-8	{char *}  The arguments.
**
** Returns:
**	DOES NOT RETURN
**
** Side Effects:
**	Calls exit to quit.
**
** History:
**	Written (jrc)
*/
/*VARARGS3*/
VOID abproerr (routine, errnum, a1, a2, a3, a4, a5, a6, a7, a8)
char		*routine;
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
    errreport(errnum, routine, a1, a2, a3, a4, a5, a6, a7, a8);
    abexcexit(-1);
}

/*{
** Name:    aberrlog() -	Log an Error.
**
** Description:
**	This should log the error, but for now just report it
**	if the trace flag ABTLOGERR is set.
**
** Inputs:
**	routine	{char *}  The name of the routine in which the error occurred.
**	err	{ER_MSGID}  The internal error number.
**	a1-8	{char *}  The arguments.
**
** History:
**	Written (jrc)
*/
/*VARARGS3*/
VOID
aberrlog (routine, err, a1, a2, a3, a4, a5, a6, a7, a8)
char		*routine;
ER_MSGID	err;
char		*a1,
		*a2,
		*a3,
		*a4,
		*a5,
		*a6,
		*a7,
		*a8;
{
    if (TRgettrace(ABTLOGERR, 0))
    {
	errreport(routine, a1, a2, a3, a4, a5, a6, a7, a8, (char *)NULL);
    }
}

/*
** Name:    errreport() -	Report Error.
**
** Description:
*/
static VOID
errreport (err, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
ER_MSGID	err;
char		*a1,
		*a2,
		*a3,
		*a4,
		*a5,
		*a6,
		*a7,
		*a8,
		*a9,
		*a10;
{
    i4	argn;

    if (a1 == (char *)NULL)
	argn = 0;
    else if (a2 == (char *)NULL)
	argn = 1;
    else if (a3 == (char *)NULL)
	argn = 2;
    else if (a4 == (char *)NULL)
	argn = 3;
    else if (a5 == (char *)NULL)
	argn = 4;
    else if (a6 == (char *)NULL)
	argn = 5;
    else if (a7 == (char *)NULL)
	argn = 6;
    else if (a8 == (char *)NULL)
	argn = 7;
    else if (a9 == (char *)NULL)
	argn = 8;
    else if (a10 == (char *)NULL)
	argn = 9;
    else
	argn = 10;

    IIUGerr(err, 0, argn, (PTR)a1, (PTR)a2, (PTR)a3, (PTR)a4, (PTR)a5, (PTR)a6,
				(PTR)a7, (PTR)a8, (PTR)a9, (PTR)a10
    );
}
