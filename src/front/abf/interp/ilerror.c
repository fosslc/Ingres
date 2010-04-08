/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<pc.h>		 
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"

/**
** Name:	ilerror.c -	4GL Interpreter Error Handler and Exit Module.
**
** Description:
**	Contains routines used to handle errors (usually display messages)
**	and exit from the 4GL interpreter.
**
** History:
**	08-jun-92 (leighb) DeskTop Porting Change:
**		Changed 'errno' to 'errnum' cuz 'errno' is a macro in WIN/NT
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (in IIOfeFrmExit).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.2  88/11  joe
**	Updated for main-line product from PC.
**	89/03  wong  Changed to call 'FEexits()' on exit.
**	
**	Revision 5.1  86/05  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIOexExitInterp() -	Exit from the interpreter
**
** Description:
**	Exits from the interpreter.
**	Called from IIOerError() if a fatal error has been detected,
**	or from IIOeiExitExec() in order to execute the OSL 'exit' statement.
**
** Inputs:
**	status		Exit status.
**
** History:
**	03/89 (jhw) -- Changed to call 'FEexits()' rather than explict
**			module cleanups ("##endforms" and 'FEing_exit()'.)
*/
VOID
IIOexExitInterp(status)
i4	status;
{
	FEexits("");

	PCexit(status);
}

/*{
** Name:	IIOskSkipStmt() -	Skip to next user statement
**
** Description:
**	Skips to the next user statement.  This may be the IL corresponding
**	to the next OSL statement, or the top of the display loop if the
**	current OSL statement was the last for an operation.
**
** Inputs:
**	None.
**
** Side effects:
**	Resets the "current" IL statement to be the next STHD (statement
**	header encountered in reading through the IL.
**
*/
VOID
IIOskSkipStmt()
{
	IL	*stmt;
	IL	il_op;

    if (IIOstmt != NULL)
    {
	do
	{
	    stmt = IIOgiGetIl(IIOstmt);
	    il_op = *stmt & ILM_MASK;
	}
	while (il_op != IL_STHD && il_op != IL_LSTHD && il_op != IL_EOF);
    }
    return;
}

/*{
** Name:	IIOfeFrmExit() -	Close current frame because of error
**
** Description:
**	Closes the current frame prematurely because of a serious error.
**
**	Treats certain errors specially so that not too much is done.
**	If the error is
**		ILE_FORMNAME	then assume we are in a frame, but the
**				form name couldn't be found, so close
**				the frame, but done do an IIendfrm()
**				Since IIdispfrm wasn't called yet.
**
** Inputs:
**	None.
**
** Side effects:
**	Sets the global flag IIOfrmErr.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Change the way we check to see if we're in a frame.
*/
VOID
IIOfeFrmExit(errnum)
ER_MSGID	errnum;				 
{
	/*
	** If current stack frame has a formname and we're not in a local proc,
	** then we are within an OSL frame rather than an OSL procedure.
	*/
	if (errnum == ILE_FORMNAME)
	{
		IIARfclFrmClose(IIOFgpGetPrm(), (DB_DATA_VALUE *) NULL);
	}
	else if (IIOFfnFormname() != NULL && !IIOFilpInLocalProc())
	{
		IIendfrm();
		IIARfclFrmClose(IIOFgpGetPrm(), (DB_DATA_VALUE *) NULL);
	}
	else
	{
		IIARpclProcClose(IIOFgpGetPrm(), (DB_DATA_VALUE *) NULL);
	}

	IIOfrmErr = TRUE;
	return;
}

/*{
** Name:	IIOerError() -	Main error handling routine
**
** Description:
**	Main error handling routine.
**	Does nothing for EQUEL-level errors, on the assumption that
**	the appropriate EQUEL level of action has already been taken
**	by the caller.
**
** Inputs:
**	level		The level of error:  ILE_EQUEL, ILE_STMT, ILE_FRAME
**			or ILE_FATAL.
**	num		The internal error number.
**	a1-8		The arguments.
**
*/
/* VARARGS2 */
VOID
IIOerError(level, num, a1, a2, a3, a4, a5, a6, a7, a8)
i4		level;
ER_MSGID	num;
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

	/*
	** If the error is ILE_ILMISSING and current statment is IL_EOF,
	** don't print error.  The EOF error will get caught later.
	*/
	if (!(level & ILE_EQUEL) &&
	    !(num == ILE_ILMISSING && IIOstmt != NULL && (*IIOstmt&ILM_MASK) == IL_EOF))
	{
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
	    IIUGerr(num, 0, count, (PTR) a1, (PTR) a2, (PTR) a3, (PTR) a4,
		    (PTR) a5, (PTR) a6, (PTR) a7, (PTR) a8);
	}


	if (level & ILE_STMT)
	{
		IIOskSkipStmt();
	}
	else if (level & ILE_FRAME)
	{
		IIOfeFrmExit(num);
	}
	else if (level & ILE_FATAL)
	{
		if (level & ILE_SYSTEM)
		{
			/*
			** OSL/STUB : Currently no special action taken.
			** Should we note that a syserr occurred?
			*/
			;
		}
		IIOexExitInterp((i4) num);
	}
}

/*{
** Name:	IIOpeProErr() -	Error in an internal routine
**
** Description:
**	Error in an internal routine.
**	Calls IIOerError() with the routine name as the first
**	character string argument.
**
** Inputs:
**	routine		The name of the routine where the error was discovered.
**	level		The level of error:  ILE_EQUEL, ILE_STMT, ILE_FRAME
**			or ILE_FATAL.
**	num		The internal error number.
**	a1-7		The arguments.
*/
VOID
/* VARARGS3 */
IIOpeProErr(routine, level, num, a1, a2, a3, a4, a5, a6, a7)
char	*routine;
i4	level;
ER_MSGID	num;
char	*a1,
	*a2,
	*a3,
	*a4,
	*a5,
	*a6,
	*a7;
{
	IIOerError(level, num, routine, a1, a2, a3, a4, a5, a6, a7);
}

/*{
** Name:	IIOdeDbErr() -	Suppress any backend error messages
**
** Description:
**	Clears out the pipe blocks being written to the backend and
**	suppresses any error messages from the backend.
**	Called when the interpreter has already found and reported an
**	error in the IL for a database-oriented OSL statement.
**
** Inputs:
**	None.
**
*/
VOID
IIOdeDbErr()
{
	i4		(*save_func)();
	FUNC_EXTERN i4	IIno_err();
	FUNC_EXTERN i4	(*IIseterr())();
 
	save_func = IIseterr(IIno_err);
	IIwritedb(ERx("where $$$"));	/* insure that there's a syntax error */
	IIsyncup((char *) NULL, 0);
	_VOID_ IIseterr(save_func);
	return;
}

/*{
** Name:	IIOslSkipIl() -	Skip til specified IL statement
**
** Description:
**	Skips to the next occurrence of the specified IL statement.
**	For example, skip to the IL_ENDLIST that closes the IL for
**	a 'getrow' statement, where Equel has found an error
**	(such as row number out-of-bounds) at run-time.
**
** Inputs:
**	stmt		The current IL statement.
**	op		The op code to look for.
**
*/
VOID
IIOslSkipIl ( stmt, op )
IL	*stmt;
i4	op;
{
	register IL	*next;

	if ((*stmt&ILM_MASK) != op)
	{
		next = IIOgiGetIl(stmt);
		while (next != NULL && (*next&ILM_MASK) != op && (*next&ILM_MASK) != IL_EOF)
			next = IIOgiGetIl(next);
		if (next == NULL || (*next&ILM_MASK) == IL_EOF)
			IIOerError(ILE_FRAME, ILE_EOFIL, (char *) NULL);
	}
}
