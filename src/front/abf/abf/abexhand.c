/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<tr.h>		/* 6-x_PC_80x86 */
#include	<ex.h>
#include	<si.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abftrace.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abclass.h>
#include	<ug.h>
#include	"erab.h"

/**
** Name:	abexhand.c -	ABF Exception Handler Module.
**
** Description:
**	Contains the main exception handler for ABF (one of many.)  Defines:
**
**	abexchand()	ABF exception handler.
**	abexcapp()	initializes excapp.
**	abexcintr()	ABF interrupt handler.
**
** History:
**	Revision  6.0  87/06  wong
**	Moved all ABF specific exception handlers here.
**	('abexcpr()' and 'abexcintr()'.)
**
**	Revision  5.0
**	11-jul-86 (marian)	bug # 8868
**	Moved abexcapp() from abfexc.qc so abnormal exits
**	will still write to the system catalogs.
**
**	Revision  4.0  arthur
**	Extracted from "abfexc.qc" so that linking an executable image of an
**	application will not pull in unnecessary modules. 2/6/85 (agh)
**	12/19/89 (dkh) - VMS shared lib changes - References to IIarStatus
**			 is now through IIar_Status.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
*/

static bool	abexctry = FALSE;
static APPL	*excapp = NULL;

static VOID abexcpr();

/*{
** Name:    abexchand() -	ABF Run-time Exception Handler.
**
** Description:
**	Called when an exception occurs in ABF.	 Restores the environment
**	to a consistent state before leaving.
**
** Input:
**	args	{EX_ARGS *}  Exception argument structure.
**
** Returns:
**	{EX}  EXCONTINUES on interrupt.
**	      EXDECLARE on jump.
**	      EXRESIGNAL otherwise.
**
** Side Effects:
**	restores the environment to a consistent state.
**
** History:
**	Written	 7/19/82 (jrc)
**	23-sep-1985 (joe)
**		Changed so EXINTR will check to see if the debugging
**		trace flags are set and if so call the routine 'abdmpfrm()'.
**		Also, exceptions EXEXIT and EXCONTINUE cause errors
**		to be printed and either exit or continue.
**	05/89 (jhw)  Removed call to 'abdmpfrm()' and EXCONTINUE handling.
**	04/90 (jhw)  Removed call to 'FEendforms()'; this is handled elsewhere
**		(usually by 'FEhandler()'.)
**      18-Feb-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
*/
EX
abexchand (arg)
EX_ARGS *arg;
{
    EX	rval;

    GLOBALREF STATUS	*IIar_Status;

    if (arg->exarg_num == EX_UNWIND)
	return EXRESIGNAL;		/* we've already been here */

    if ( arg->exarg_num == EXINTR )
	return EXCONTINUES;
    else if ( arg->exarg_num == EXABFJMP )
	rval = EXDECLARE;
    else
    {
	if (arg->exarg_num == EXEXIT)
	    abexcpr(arg);
	rval = EXRESIGNAL;
    }
    IIresync();
    /*
    ** BUG 1766
    */
    if ( *IIar_Status != OK && excapp != NULL )
    {
	if (!abexctry)
	{
	    FEmsg(ERget(E_AB0001_Exceptional_error), FALSE, (char *)NULL);
	    abexctry = TRUE;
	    IIAMappFree(excapp);
	}
	else
	{
	    FEmsg(ERget(E_AB0002_Error_writing), FALSE, (char *)NULL);
	}
    }
    return rval;
}

/*
** bug 8868
**	move abexcapp to abexchand.qc from abfexc.qc
**
** Initialize excapp
*/

VOID
abexcapp (app)
APPL	*app;
{
    excapp = app;
}

/*
** Name:    abexcpr() - Print an exception argument.
**
** Description:
**	Prints the arguments in an exception.
**
** Inputs:
**	arg		- The handlers EXARGS.
**
** History:
**	23-sep-1985 (joe)
**		Written
*/
static VOID
abexcpr(arg)
EX_ARGS *arg;
{
	char	bufr[FE_PROMPTSIZE];
	i4	prargs[10];
	i4	*exarr;
	i4	*prarr;
	i4	i;

	if (!TRgettrace(ABTLOGERR, 0))
		return;
	for (i = 0, exarr = (i4 *)arg->exarg_array, prarr = prargs;
		i < arg->exarg_count && i < 10; i++, prarr++, exarr++)
	{
		*prarr = *exarr;
	}
	IIUGfmt(bufr, FE_PROMPTSIZE,
			ERget(E_AB0003_Exception), 1, arg->exarg_num);
	SIfprintf(stderr, ERx("%s"), bufr);
	IIUGfmt(bufr, FE_PROMPTSIZE, prargs[0], arg->exarg_count-1, prargs[1],
		prargs[2], prargs[3], prargs[4], prargs[5], prargs[6],
		prargs[7], prargs[8], prargs[9]);
	SIfprintf(stderr, ERx("%s"), bufr);
	SIflush(stderr);
}

/*{
** Name:    abexcintr() -	ABF Interrupt Handler.
**
** Description:
**	General purpose interrupt handler.  The handler looks for an
**	interrupt and returns EXDECLARE if it is.  Otherwise, it
**	returns RESIGNAL.
**
** History:
**	Written	 12/27/84 (jrc)
**	19-aug-91 (blaise)
**		Following an interrupt, abort the current transaction before
**		returning. Bug #38993.
*/
EX
abexcintr(arg)
EX_ARGS *arg;
{
	if (arg->exarg_num == EXINTR)
	{
		IIUIabortXaction();
		return EXDECLARE;
	}
	else
	{
		return EXRESIGNAL;
	}
}
