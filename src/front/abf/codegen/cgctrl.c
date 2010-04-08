/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>

#include	"cggvars.h"
#include	"cglabel.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cgerror.h"


/**
** Name:    cgctrl.c -	Code Generator IL Control Module.
**
** Description:
**	Routines for moving within array of IL stmts.
**
**	IICGgilGetIl()
**	IICGnilNextIl()
**	IICGgotGotoGen()
**	IICGifgIfGen()
**
** History:
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.0  87/06  arthur
**	Initial revision.
**
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
*/


/*{
** Name:    IICGgilGetIl() -	Get next IL statement
**
** Description:
**	Gets next IL statement to be executed.
**	This is the sequentially next IL statement, except just after
**	there has been a jump within the IL array via IICGsiSletIl().
**	In that case, IICGgilGetIL() returns the current statement,
**	which should be executed next.
**
** Inputs:
**	stmt	The current IL statement
**
** Outputs:
**	Returns:
**		The next IL statement to execute, or NULL if
**		end-of-file has been reached.
**
*/
IL *
IICGgilGetIl(stmt)
register IL	*stmt;
{
	register IL	*next;
	IL      	il_op;

	il_op = *stmt & ILM_MASK;
	if (stmt == NULL || il_op > IL_MAX_OP)
	{
		IICGerrError(CG_EOFIL, (char *) NULL);
		return (NULL);
	}

	if (il_op == IL_EXPR)
		next = IICGnilNextIl(stmt) + ILgetI4Operand(stmt, 1);
	else if (il_op == IL_LEXPR)
		next = IICGnilNextIl(stmt) + ILgetI4Operand(stmt, 2);
	else
		next = IICGnilNextIl(stmt);

	if (next == NULL || (*next&ILM_MASK) > IL_MAX_OP)
	{
		IICGerrError(CG_EOFIL, (char *) NULL);
		return (NULL);
	}

	CG_il_stmt = next;
	return (next);
}

/*{
** Name:    IICGnilNextIl() -	Returns next IL statement
**
** Description:
**	Returns the sequentially next IL statement in the array of
**	IL for the frame.
**	Does NOT re-set the global pointer to the current IL statement.
**
** Inputs:
**	stmt		The current IL statement.
**
** Outputs:
**	Returns:
**		A pointer to the sequentially next IL statement.
**
** Side effects:
**	None.
**
*/
IL *
IICGnilNextIl(stmt)
register IL	*stmt;
{
	return (stmt + IIILtab[*stmt&ILM_MASK].il_stmtsize);
}

/*{
** Name:    IICGgotGotoGen() -	Generate code for a 'goto' statement
**
** Description:
**	Generates code for a 'goto' statement.
**	Also, enters the IL statement which is the object of the 'goto'
**	into the array of IL statements for which labels must be generated.
**
** IL statements:
**	GOTO offset
**
**	Equivalent to a programming language 'goto' statement.
**	Its operand is the relative offset (in i2's) of the next IL statement
**	to be executed.
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGgotGotoGen(stmt)
register IL	*stmt;
{
	IL	*next;
	IL	*goto_stmt;

	goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 1));
	IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
	/*
	** Do not generate a 'goto' if this IL statement ends a submenu.
	*/
	next = IICGnilNextIl(stmt);
	if ((*next&ILM_MASK) != IL_POPSUBMU)
		IICGgtsGotoStmt(goto_stmt);
}

/*{
** Name:    IICGifgIfGen() -	Generate code for a 'if' statement
**
** Description:
**	Generates code for a 'if' statement.
**	An IL 'if' statement branches to the specified statement
**	if the VALUE is FALSE.	If the VALUE is TRUE, the order
**	of execution of IL statements does not change.
**
** IL statements:
**	IF VALUE1 SID1
**		IL statements for this 'if'
**	GOTO SID2
**	STHD SID1
**		evaluate expression into VALUE2
**	IF VALUE2 SID3
**		IL statements for this 'elseif'
**	GOTO SID2
**	STHD SID3
**		IL statements for the 'else'
**	STHD SID2
**
**	This IL code corresponds to OSL statements of the form:
**
**		if cond1 then
**			if-block
**		elseif cond2 then
**			else-if-block
**		else
**			else-block
**		endif;
**
**	If VALUE1 is TRUE, the C code corresponding to the IL statements
**	within the 'if-block' will be executed.	 After the execution of these
**	statements, the GOTO indicates the next IL statement to
**	skip to.  (This statement would be the one for the OSL
**	statement following the OSL 'endif'.)
**	If VALUE1 is FALSE, the statements within the 'if-block'
**	are skipped, and the next test is performed.  If VALUE2 is also
**	FALSE, then the statements within the 'else' block will be executed.
**
**	Note that OSL 'elseif' constructs are converted into IF statements
**	in the IL.
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGifgIfGen(stmt)
register IL	*stmt;
{
	IL		*goto_stmt;

	goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 2));
	IICGoibIfBeg();
	IICGoicIfCond(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_INT_TYPE),
		CGRO_EQ, ERx("0"));
	IICGoikIfBlock();
	IICGgtsGotoStmt(goto_stmt);
	IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
	IICGobeBlockEnd();
	return;
}
