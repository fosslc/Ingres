/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<feconfig.h>
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
#include	"ilargs.h"

/**
** Name:	itilloop.c -	Main Loop for 4GL Interpreter.
**
** Description:
**	Contains the interpreter loop for the 4GL interpreter.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 5.1  86/05/15  arthur
**	Initial revision.
**/

GLOBALREF ILARGS	IIITARGS;

/*{
** Name:	IIOilInterpLoop		-	Main loop of the interpreter
**
** Description:
**	Main loop of the interpreter.
**
** History:
**	15-May-1986 (agh)
**		First written
**	26-sep-1988 (Joe)
**		Took out special case checks for the return operators,
**		and made the return execution procedure IIOrtReturnExec
**		actually make the next instruction NULL  This should
**		allow other instructions that need to leave the
**		loop be possible to add without having to change
**		this code.  It also fixes a PC bug about using
**		a possibly stale pointer.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Pop control stack at exit.
*/
VOID
IIOilInterpLoop()
{
	register IL	*stmt;
	VOID		IIAMpsPrintStmt();
	IFCONTROL	*save_IIOcontrolptr;

	save_IIOcontrolptr = IIOcontrolptr;

	while ((stmt = IIOgiGetIl(IIOstmt)) != NULL)
	{
		/*
		** If IIOdebug flag is set, print out the statement.
		*/
		if (IIOdebug)
			IIAMpasPrintAddressedStmt(IIITARGS.ildbgfile, IIILtab,
				stmt, IIOFgbGetCodeblock());
		/*
		** After the call to the execution routine, the
		** statement pointer stmt may no longer be valid because
		** of swapping.
		*/
		(*(IIILtab[*stmt&ILM_MASK].il_func))(stmt);

		/*
		** Stop interpreting IL statements in the current frame
		** if there was a frame-level error.
		*/
		if (IIOfrmErr)
		{
			break;
		}
	}

	/*
	** The first instruction (except for a STHD)
	** in the frame or procedure was a
	** START, MAINPROC, or LOCALPROC instruction,
	** which pushed an entry onto the control stack.
	** Now we want to pop that entry off.
	**
	** The only exception is when an entry couldn't be pushed
	** (because we ran out of memory).  In that case,
	** IIOcontrolptr will be unchanged.
	*/
	if ( save_IIOcontrolptr != IIOcontrolptr )
	{
		IIOFpcPopControl();
	}
	return;
}
