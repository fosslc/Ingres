/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ex.h> 
#include	<er.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<rtsdata.h>
#include	<erug.h>
#include	<erar.h>
#include	<ug.h>

/**
** Name:    abfexc.sc -	ABF Run-Time System Exception Handler Module.
**
** Description:
**	Contains routines that handle exceptions for the ABF run-time system.
**	Defines:
**
**	abexcexit()	ABF general exit mechanism.
**	IIARexchand() 	ABF run-time system exception handler.
**
** History:
**	Revision  6.4/02
**	12/15/91 (emerson)
**		Fix for bug 41269 in abexcexit.
**
**	Revision  6.0  87/06  wong
**	Modified to use 'FEexits()' and 'FEhandler()', and also extracted
**	ABF specific routines into "abexchand.qc".
**
**	Revision  5.0
**	11-jul-86 (marian)	bug 8868
**	Moved over abexcapp() to abfexc.qc file so interrupt
**	handling would work correctly.
**
**	Revision  4.0  arthur
**	Moved routine abexchand (interrupt handler) to a separate
**	file (abexchand.qc) so that linking an image for an
**	application will not pull in unneeded modules.  2/6/85 (agh)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
**	abexcexit
**		All exits of abf go through this routine (except when
**		no exception handler has been declared yet).
**		If abf is called from equel, then this routine
**		does an i4  jump back to the call point.
**
**	Parameters:
**		status
**			The status of the exit.
**
**	Returns:
**		NOTHING
**
**	Called by:
**		NONE
**
**	Side Effects:
**		NONE
**
**	Trace Flags:
**		NONE
**
**	Error Messages:
**		NONE
**
**	History:
**		Written 7/19/82 (jrc)
**	18-oct-88 (kenl)
**		Changed commit work to IIUIendXaction() routine.
**	12/15/91 (emerson)
**		Changed IIUIendXaction() back to COMMIT WORK, because
**		IIUIendXaction sets autocommit on; we want to leave
**		the autocommit state alone (bug 41269).
*/
VOID
abexcexit (status)
i4	status;
{
    extern STATUS	IIarStatus;

    ADF_CB	*FEadfcb();

    /*
    ** Commit on OK exit for SQL.
    */
    if (status == OK && FEadfcb()->adf_qlang == DB_SQL && FEinMST())
    {
	EXEC SQL COMMIT WORK;
    }

    IIarStatus = status;
    /*
    ** BUG 4521
    **
    ** Since EXsignal now checks the number of arguments passed against
    ** the number declared, eliminate any args in this call.
    */
    EXsignal(EXABFJMP, 0);
}

/*{
** Name:    IIARexchand() -	ABF Run-Time System Exception Handler.
**
** Description:
**	Called when an exception is fired in an imaged application.
**	Restores the environment to a consistent state before leaving.
**
** Returns:
**	DOES NOT RETURN
**
** Side Effects:
**	Restores the environment to a consistent state.
**
** History:
**	Written  7/19/82 (jrc)
**
**	2/6/85 (agh) Keep exception handler basic for run-time system, so that
**	unneeded modules don't get pulled in.  The ABF main exception handler,
**	'abexchand()', has been moved to a separate file (abexchand.qc).
**		
**	06/87 (jhw) -- Modified to use 'FEhandler()' and 'FEexits()'.
**
**	7/30/93 (DonC) Bug 45678
**		Eliminated the reliance on adx_handler.  The return status of
**		adx_handler was DBMS-specific and not applicable to FE routines.
**		As a result, errors like Integer Divide-By-Zero were causing
**		ABF application images to exit to the OS (not really a user
**		friendly behavior). Instead, I replaced the call to adx_handler
**		with the same sort of user-friendly code that `Go' option users
**		have come to know and love. 
** 
**	8/23/93 (DonC)
**		Put the 6.5 header info for gl, sl, and iicommon back in.
**	8/23/93 (DonC)
**		Put the #include <ug.h> back in.
**	1/3/94 (donc) Bug 52903
**		Added Decimal divide-by-zero and overflow support.
**      9-dec-1996 (angusm)
**              Add handling for SIGHUP (EXHANGUP) to cause cleanup of
**              subprocesses: conditional on EXCLEANUP being defined.
**              Initial implementation of this for su4_us5 and axp_osf only,
**              because time not available to test on other UNIX. (bug 62894).
**      18-Feb-1999 (hweho01)
**              Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**              redefinition on AIX 4.3, it is used in <sys/context.h>.
*/
EX
IIARexchand (arg)
EX_ARGS	*arg;
{
    EX		FEhandler();
    VOID	IIUGerr();

    i4      exarg_n;        /* the exception number */

    exarg_n = arg->exarg_num;

    switch (exarg_n)
    {
        case EXFLTDIV:
        case EXINTDIV:
        case EXDECDIV:
		IIUGerr( E_AR0088_divzero, UG_ERR_ERROR, 0 );
                return (EXCONTINUES);

        case EXFLTOVF:
        case EXFLTUND:
        case EXINTOVF:
        case EXDECOVF:
		IIUGerr( E_AR0089_numovf, UG_ERR_ERROR, 0 );
                return (EXCONTINUES);

#ifdef EXFLOAT
        case EXFLOAT:
		IIUGerr( E_AR0090_fltbad, UG_ERR_ERROR, 0 );
                return (EXCONTINUES);
#endif /* EXFLOAT */

        case EX_UNWIND:
	    return (EXRESIGNAL);

        case EXHANGUP:
#ifdef EXCLEANUP
	    PCcleanup(EXHANGUP);
#endif
	    FEexits(ERx(""));
      	    return (EXDECLARE);

        case EXINTR:
	    FEexits(ERx(""));
	    return (EXDECLARE);

        case EXABFJMP:
	    FEexits(ERx(""));
	    return (EXDECLARE);

        default:
	    return FEhandler(arg);

    }
}
