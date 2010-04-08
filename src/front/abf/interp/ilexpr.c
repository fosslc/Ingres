/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ex.h>
#include	<er.h>
#include	<me.h>
#include	<si.h>
#include	<lo.h>
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
#include	"itcstr.h"

/**
** Name:	ilexpr.c -	Execute IL Statements for Expression Evaluation.
**
** Description:
**	Executes IL statements for expression evaluation.
**	Includes a different IIOstmtExec() routine for each type of
**	statement.
**
** History:
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Created IIITroeReleaseObjExec, and modified IIITdotDotExec
**		and IIITarrArrayExec (for bugs 39581, 41013, and 41014:
**		array performance).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.2  88/12/14  08:49:37  joe
**	Updated the interpreter to 6.2.  Major changes in most files.
**
**	Revision 6.0  87/08/12  11:04:29  arthur
**	Deleted explicit include of erit.h (now included by ilerror.h).
**	87/08/12  10:46:42  arthur  Changed to use ER module.
**
**	Revision 5.1  86/10/17  16:00:56  arthur
**	Code used in 'beta' release to DATEV.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
*/

/*{
** Name:	IIOasAssignExec	-	Execute OSL assignment statement
**
** Description:
**	Executes OSL assignment statement.
**
** IL statements:
**
**	IL_ASSIGN valLHS valRHS intCoersionFID.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	24-oct-1988 (Joe)
**		Upgraded to 6.0.  Now calls IIARasnDbdvAssign.
*/
IIOasAssignExec(stmt)
IL	*stmt;
{
	EX_CONTEXT	ilmcontext;
	EX		IIOmhMhandler();

	/*
	** Declare math exception handler.
	*/
	if (EXdeclare(IIOmhMhandler, &ilmcontext) != OK)
	{
		EXdelete();
		return;
	}

	IIARasnDbdvAssign(IIOFdgDbdvGet((i4) ILgetOperand(stmt, 1)),
	                  IIOFdgDbdvGet((i4) ILgetOperand(stmt, 2)),
			  (i4) ILgetOperand(stmt, 3));
	EXdelete();
	return;
}

/*{
** Name:	IIITdotDotExec	-	Execute OSL 'dot' statement
**
** Description:
**	Executes OSL 'dot' statement -- for access to record members.
**
** IL statements:
**
**	IL_DOT recordObject attributeName tempDbdv.
**
**	IL_DOT may have either of the ILM_RLS_SRC or ILM_HOLD_TARG
**	modifier bits set.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	10/89 (billc) Written.
**	11/07/91 (emerson)
**		Modified to handle new modifiers
**		(for bugs 39581, 41013, and 41014: array performance).
*/
IIITdotDotExec(stmt)
IL	*stmt;
{
	DB_DATA_VALUE	*obj;
	DB_DATA_VALUE	*targ;
	char		*mname;
	i4		flags;
	STATUS		IIARdotDotRef();

	obj = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	mname = IIITtcsToCStr( ILgetOperand(stmt, 2) );
	targ = IIOFdgDbdvGet(ILgetOperand(stmt, 3));

	/*
	** The low-order bit of flags will be off, which means
	** don't coerce - get exact datatype match.
	*/
	flags = *stmt & (ILM_RLS_SRC | ILM_HOLD_TARG);
	if (IIARdotDotRef(obj, mname, targ, flags) != OK)
	{
		/* Runtime system has already reported the error */
		IIOfeFrmExit(ILE_FAIL);
	}
}

/*{
** Name:	IIITarrArrayExec -	Execute OSL 'arrayref' statement
**
** Description:
**	Executes OSL 'arrayref' statement -- for access to array elements.
**
** IL statements:
**
**	IL_ARRAY    arrayObject integerIndex tempDbdv is_lhs, or
**	IL_ARRAYREF arrayObject integerIndex tempDbdv.
**
**	IL_ARRAYREF may have any of the ILM_LHS, ILM_RLS_SRC, or ILM_HOLD_TARG
**	modifier bits set.  IL_ARRAYREF is generated in place of the old
**	IL_ARRAY starting with the 6.4/02 OSL compiler.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	10/89 (billc) Written.
**	01/09/91 (emerson)
**		Change index from i4  to i4.
**	11/07/91 (emerson)
**		Modified to handle new IL_ARRAYREF opcode
**		(for bugs 39581, 41013, and 41014: array performance).
*/
IIITarrArrayExec(stmt)
IL	*stmt;
{
	DB_DATA_VALUE	*obj;
	DB_DATA_VALUE	*targ;
	i4		index;
	IL		rowval;
	i4		flags;
	STATUS		IIARarrArrayRef();

	obj = IIOFdgDbdvGet(ILgetOperand(stmt, 1));

	/* test for null index */
	rowval = ILgetOperand(stmt, 2);
	index = IIITvtnValToNat( rowval, 0, ERx("ARRAY INDEX") );

	targ = IIOFdgDbdvGet(ILgetOperand(stmt, 3));

	if ((*stmt & ILM_MASK) == IL_ARRAY)
	{
		flags = ILgetOperand(stmt, 4); 
	}
	else
	{
		flags = *stmt & (ILM_LHS | ILM_RLS_SRC | ILM_HOLD_TARG);
	}
	_VOID_ IIARarrArrayRef(obj, (i4)index, targ, flags);
	return;
}

/*{
** Name:	IIITroeReleaseObjExec	-	Execute OSL 'release obj' stmt
**
** Description:
**	Executes OSL 'release obj' statement.
**
** IL statements:
**
**	IL_RELEASEOBJ Object.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	11/07/91 (emerson)
**		Created (for bugs 39581, 41013, and 41014: array performance).
*/
IIITroeReleaseObjExec(stmt)
IL	*stmt;
{
	STATUS		IIARroReleaseObj();

	_VOID_ IIARroReleaseObj(IIOFdgDbdvGet(ILgetOperand(stmt, 1)));
	return;
}

/*{
** Name:	IIOmcMovConstExec	-	Execute IL 'movconst' statement
**
** Description:
**	Executes the IL 'movconst' statement.
**	This statement is used to move a constant into a temp.
**	In all cases, the address of the constant is copied into
**	the db_data portion of the temp.  For the character types,
**	the length of the temp is set to be the length of the
**	constant.
**
** IL statements:
**
**	IL_MOVCONST valTempForConstant  valConstant
**
**	The first VALUE (equivalent to the left-hand side of an OSL
**	assignment statement) is the temp; the second VALUE (the right-hand
**	side) is the constant.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	25-oct-1988 (Joe)
**		Upgraded to 6.0 by adding code for the new types
**		of constants that can appear, namely the NULL
**		constant (given by -DB_LTXT_TYPE), HEX constants
**		given by DB_VCH_TYPE and character constants are
**		now DB_CHA_TYPE.
**	27-oct-1988 (Joe)
**		Moved most of the functionality to IIITctdConstToDbv
**		since it was needed in other places.
*/
VOID
IIOmcMovConstExec(stmt)
IL	*stmt;
{
	DB_DATA_VALUE	*dbdv;

	/*
	** Get the type of the temporary and the address of
	** its DBV.
	*/
	dbdv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	_VOID_ IIITctdConstToDbv(ILgetOperand(stmt, 2),
		      (DB_DT_ID) dbdv->db_datatype, dbdv
	);
}

/*{
** Name:	IIOepExprExec	-	Evaluate an expression
**
** Description:
**	Evaluates an expression by calling ade_fe_execute_cx(), the
**	front-end re-write of the ADE execution routine.
**	If this is a logical expression, the logical result is placed
**	into the designated VALUE by ade_fe_execute_cx().
**	EXPR and LEXPR are the only variable-length IL statements.
**
** IL statements:
**	EXPR intSizeOfExprInI2s intOffsetToLastInstr i2CX[intSizeOfExprInI2s]
**	LEXPR valResult  intSizeOfExprInI2s
**			intOffsetToLastInstr i2CX[intSizeOfExprInI2s]
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	17-oct-1986 (agh) written.
**	26-jun-1990 (jhw) Added correction for negative last ADE instruction
**		offsets.  Bug #31023.
*/
VOID
IIOepExprExec(stmt)
IL	*stmt;
{
	DB_STATUS	rval;
	DB_DATA_VALUE	*result_dbv = NULL;
	ILREF		*ilexpr;
	i4		lastinstr_offset;	/* Offset to the last ADE
						** instruction for this
						** expression.
						*/
	EX_CONTEXT	ilmcontext;

	EX		IIOmhMhandler();

	/*
	** Declare math exception handler.
	*/
	if (EXdeclare(IIOmhMhandler, &ilmcontext) != OK)
	{
		EXdelete();
		return;
	}
	/*
	** The only difference between the two kinds of expressions is whether
	** there is a separate result, and consequently, what the offsets are
	** to the size and last ADE instruction operands, and to the first ADE
	** instruction.
	**
	** The offset to the first instruction is just the number of operands
	** for the operator.  The last ADE instruction offset is in bytes and
	** will be negative if it is larger than MAX_2IL (this assumes two's-
	** complement integers.)
	*/
	ilexpr = (ILREF *)( stmt + IIILtab[*stmt&ILM_MASK].il_stmtsize );
	if ((*stmt&ILM_MASK) == IL_LEXPR)
	{
		result_dbv = IIOFdgDbdvGet( ILgetOperand(stmt, 1) );
		lastinstr_offset = (i4) ILgetI4Operand(stmt, 4);
	}
	else
	{
		lastinstr_offset = ILgetI4Operand(stmt, 3);
	}

	if ( lastinstr_offset < 0 )
		lastinstr_offset += (MAX_2IL + 1) * 2;	/* 2's complement */

	rval = IIARevlExprEval( ilexpr, lastinstr_offset,
				IIOFgfaGetFrmAlign(), IIOFdgDbdvGet(1),
				result_dbv
	);

	if (rval != E_DB_OK)
		IIOerError(ILE_STMT, ILE_EXPBAD, (char *) NULL);
	EXdelete();
	return;
}

/*{
** Name:	IIOdaDycAllocExec	-	Allocate a dynamic string
**
** Description:
**	Execute the DYCALLOC statement.  Simply resets the db_length
**	field of the dbv for this dynamic string.
**
** IL statements:
**
**	DYCALLOC valDynamicString
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOdaDycAllocExec(stmt)
IL	*stmt;
{
	DB_DATA_VALUE	*dbdv;
	STATUS		abstrinit();

	dbdv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	dbdv->db_length = 0;
	return;
}

/*{
** Name:	IIOdfDycFreeExec	-	Free a dynamic string
**
** Description:
**	Frees the string that a dynamic string points to, and the
**	dynamic string structure itself.
**
** IL statements:
**	DYCFREE VALUE
**
**	The VALUE is the temp which points to the dynamic string.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOdfDycFreeExec(stmt)
IL	*stmt;
{
	DB_DATA_VALUE	*dbdv;
	STATUS		abstrfree();

	dbdv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	if (dbdv->db_data != NULL)
	    MEfree(dbdv->db_data);
	dbdv->db_length = 0;
	dbdv->db_data = NULL;
	return;
}
