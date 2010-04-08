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
** Name:	ilcntrl.c -	Execute IL statements for control structures
**
** Description:
**	Executes IL statements for control structures such as 'if' statements.
**	Includes a different IIOstmtExec() routine for each type of
**	statement.
**
** History:
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails supporting long SIDs, and
**		introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.2  88/09  joe
**	Updated for 6.2.
**
**	Revision 5.1  86/09  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
**/

/*
** static indicating that a jump was just done within the IL.
** The IIOgiGetIl() routine, therefore, should get the current statement,
** rather than the next statement.
*/
static bool	getCurrentIl = TRUE;

/*{
** Name:	IIOgtGoToExec	-	Change order of IL statement execution
**
** Description:
**	Changes order of IL statement execution.
**
** IL statements:
**	GOTO offset
**
**	Equivalent to a programming language 'goto' statement.
**	Its operand is the relative offset (in i2's) of the next IL statement
**	to be executed.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** Side effects:
**	Changes the "next" IL statement to be executed by calling
**	IIOsiSetIl().
**
*/
VOID
IIOgtGoToExec(stmt)
IL	*stmt;
{
	IIOsiSetIl(ILgetOperand(stmt, 1));
}

/*{
** Name:	IIOieIfExec		-	Branch if not equal
**
** Description:
**	Branches to the specified IL statement if the VALUE is
**	FALSE.  If the VALUE is TRUE, does not change the order
**	of execution of IL statements.
**
** IL statements:
**	IF VALUE1 SID1
**		IL statements within if-then or elseif-then block
**	ENDBLOCK SID2
**	STHD SID1
**		evaluate expression into VALUE2
**	IF VALUE2 SID3
**		IL statements for this 'if' or 'elseif'
**	ENDBLOCK SID2
**	STHD SID3
**		IL statements for the 'else'
**	STHD SID2
**
**	This IL code corresponds to OSL statements of the form:
**
**		if cond1 then
**			if-block1
**		elseif cond2 then
**			if-block2
**		else
**			else-block
**		endif;
**
**	If VALUE1 is TRUE, the statements within the 'if-then' block
**	(if-block1) will be executed.  After the execution of these
**	statements, the ENDBLOCK indicates the next IL statement to
**	skip to.  (This statement would be the one for the OSL
**	statement following the OSL 'endif'.)
**	If VALUE1 is FALSE, the statements within the 'if-then' block
**	are skipped, and the next test is performed.  If VALUE2 is also
**	FALSE, then the statements within the 'else' block will be executed.
**
**	Note that OSL 'elseif' constructs are converted into IF statements
**	in the IL.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** Side effects:
**	If the condition is FALSE, changes the "next" IL statement
**	to be executed by calling IIOsiSetIl().
**
*/
IIOieIfExec(stmt)
IL	*stmt;
{
	if (!IIITvtnValToNat(ILgetOperand(stmt, 1), 0, ERx("IF, WHILE")))
		IIOsiSetIl(ILgetOperand(stmt, 2));
}

/*{
** Name:	IIOgiGetIl	-	Get next IL statement
**
** Description:
**	Gets next IL statement to be executed.
**	This is the sequentially next IL statement, except just after
**	there has been a jump within the IL array via IIOsiSetIl().
**	In that case, IIOgiGetIL() returns the current statement,
**	which should be executed next.
**
**	This routine is part of a multi-routine protocol to traverse
**	through the IL statements.  Included in this protocol is
**	IIOsiSetIl, IIOjiJumpIl and IIITsniSetNextIl.
**
** Inputs:
**	stmt	The current IL statement
**
** Outputs:
**	Returns:
**		The next IL statement to execute, or EOF statement if
**		end-of-file has been reached.
**		NULL will only be returned from this routine if
**		the next statement is set to be NULL by IIITsniSetNextIl.
**
*/
IL *
IIOgiGetIl(stmt)
IL	*stmt;
{
	IL	*next;
	IL	il_op;
	static IL	il_eof[] = {IL_EOF, IL_EOF};

	if (getCurrentIl)
	{
		getCurrentIl = FALSE;
		return (IIOstmt);
	}

	il_op = *stmt & ILM_MASK;
	if (stmt == NULL || il_op > IL_MAX_OP
		|| il_op == IL_EOF)
	{
	    if (stmt != NULL && il_op != IL_EOF)
		IIOerError(ILE_FRAME, ILE_EOFIL, (char *) NULL);
	    return il_eof;
	}
	else if (il_op == IL_EXPR)
	{
		next = stmt + IIILtab[il_op].il_stmtsize
			+ ILgetI4Operand(stmt, 1);
	}
	else if (il_op == IL_LEXPR)
	{
		next = stmt + IIILtab[il_op].il_stmtsize
			+ ILgetI4Operand(stmt, 2);
	}
	else
	{
		next = stmt + IIILtab[il_op].il_stmtsize;
	}

	if (next == NULL || (*next&ILM_MASK) > IL_MAX_OP)
	{
		IIOerError(ILE_FRAME, ILE_EOFIL, (char *) NULL);
		return il_eof;
	}

	IIOstmt = next;
	return (next);
}

/*{
** Name:	IIOsiSetIl	-	Set next IL statement to be executed
**
** Description:
**	Sets next IL statement to be executed.  This is part of
** 	a multi-routine protocol that includes IIOgiGetIl, IIOjiJumpIl
**      and IIITsniSetNextIl.
**	These routines provide an interface to address the IL block.
**
** Inputs:
**	offset		The relative offset of the statement being
**			set from the current statement.
**
** Outputs:
**	None.
**
** Side effects:
**	Resets the global IL statement pointer IIOstmt.
**
** History:
**	26-sep-1988 (Joe)
**		Changed to call IIITsniSetNextIl.
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails supporting long SIDs.
*/
IIOsiSetIl(offset)
IL	offset;
{
    i4	real_offset = (i4) offset;

    if (*IIOstmt&ILM_LONGSID)
    {
	real_offset = IIITvtnValToNat(offset, 0, ERx("IIOsiSetIl"));
    }
    IIITsniSetNextIl(IIOstmt + real_offset);
}

/*{
** Name:	IIITsniSetNextIL - Set the next IL statement to be executed.
**
** Description:
**	This sets the next IL statement to be executed given an absolute
**	statement addresss.  This routine is part of a multi-routine
**	protocol that includes IIOgiGetIl, IIOsiSetIl, and IIOjiJumpIl.  This
**	routine is different than IIOsiSetIl since it takes an absolute
**	address and not an offset.  
**
**	Note that the address can be NULL if there is logically
**	no next statement. This happens, for example, on returns.
**
** Inputs:
**	
**	next_stmt	The absolute address of the next statement to
**			execute.
**
** Side Effects:
**	Changes the global IL statement pointer IIOstmt.
**
** History:
**	26-sep-1988 (Joe)
**		Added this routine to cleanup the interpreter main
**		loop.
*/
VOID
IIITsniSetNextIl(next_stmt)
IL	*next_stmt;
{
    IIOstmt = next_stmt;
    getCurrentIl = TRUE;
    if (IIOstmt != NULL && IIORcaCheckAddr(&il_irblk, IIOstmt) != OK)
	IIOerError(ILE_FRAME, ILE_JMPBAD, (char *) NULL);
    return;
}

/*{
** Name:	IIOjiJumpIl	-	Set next IL statement using jump table
**
** Description:
**	Sets the next IL statement to be executed, using the jump
**	table for the current menu or submenu.
**
**	Callers must assume that jump table indices start at 1.
**
** Inputs:
**	index		Index into the jump table.
**
** Outputs:
**	None.
**
** Side effects:
**	Resets the global IL statement pointer IIOstmt.
**
** History:
**	26-sep-1988 (Joe)
**		Changed to call IIITsniSetNextIl.
**	05/89 (jhw) -- Exit frame on bad index.
**	05/89 (billc) -- Bomb out on bad jump-table or codeblock (internal err)
*/
VOID
IIOjiJumpIl ( index )
i4	index;
{
	if ( index == 0 )
	{
		/* 
		** "falling off" the end of the display loop.  Could be a
		** timeout without a timeout activation.
		*/
		IIOfrFrmReturn();
	}
	else
	{
		IL	*codeblock;
		ILJUMP	*table;

		if ((table = IIOFgjGetJump()) == NULL
		  || (codeblock = IIOFgbGetCodeblock()) == NULL)
		{
			IIOerError(ILE_FRAME, ILE_JMPBAD, (char *) NULL);
			return;
		}

		IIITsniSetNextIl( codeblock + table->ilj_offsets[index - 1] );
	}
}

/*{
** Name:	IIITgfsGotoFromStmt	- Execute a jump relative to a statement
**
** Description:
**	Given a statement, and a SID contained in that statement, execute
**	a goto using the SID relative to the statement.
**
** Inputs:
**	stmt		The statement containing the sid.
**
**	oper_num	The number of the operand that contains the sid.
**
** History:
**	1-dec-1988 (Joe)
**		First Written
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails supporting long SIDs.
*/
IIITgfsGotoFromStmt(stmt, oper_num)
IL	*stmt;
i4	oper_num;
{
    IL		sid;
    i4	offset;

    sid = ILgetOperand(stmt, oper_num);
    if (*stmt & ILM_LONGSID)
	offset = IIITvtnValToNat(sid, 0, ERx("IIITgfsGotoFromStmt"));
    else
	offset = (i4) sid;
    IIITsniSetNextIl(stmt + offset);
}
