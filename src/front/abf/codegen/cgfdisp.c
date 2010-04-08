/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<si.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<frscnsts.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>

#include	"cggvars.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cglabel.h"
#include	"cgerror.h"


/**
** Name:	cgfdisp.c - Code Generator Form Display Statements Module.
**
** Description:
**	Generates C code from the IL statements for form display
**	statements.  Includes a different IICGstmtGen() routine for
**	each type of statement.
**	Other execution routines for OSL forms statements are in "cgftfld.c"
**	and "cgfetc.c".
**	This file defines:
**
**	IICGclaClrallGen()	-	OSL 'clear field all' statement
**	IICGclfClrfldGen()	-	OSL 'clear field' statement
**	IICGclsClrscrGen()	-	OSL 'clear screen' statement
**	IICGgtfGetformGen()	-	Generate code for a getform
**	IICGptfPutformGen()	-	Generate code for a putform
**	IICGrdyRedisplayGen()	-	OSL 'redisplay' stmt
**	IICGrsuResumeGen()	-	OSL 'resume' statement
**	IICGrscResColumnGen()	-	'resume column' stmt
**	IICGrsfResFieldGen()	-	OSL 'resume field' stmt
**	IICGrsmResMenuGen()	-	OSL 'resume menu' stmt
**	IICGrsnResNextGen()	-	OSL 'resume next' stmt
**      IICGrseResEntryGen()    -       OSL 'resume entry' stmt
**	IICGvldValidateGen()	-	OSL 'validate' stmt
**	IICGvlfValFieldGen()	-	'validate field' stmt
**	IICGrnfResNxtFld()	-	'resume nextfield' stmt
**	IICGrpfResPrvFld()	-	'resume previousfield' stmt
**
** History:
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**
**	Revision 6.0  87/04  arthur
**	Initial revision.
*/


/*{
** Name:	IICGclaClrallGen() -	Generate code for 'clear field all' stmt
**
** Description:
**	Generates code for an OSL 'clear field all' statement.
**
** IL statements:
**	CLRALL
**
**	The CLRALL statement takes no arguments.  It is exactly equivalent
**	to an OSL 'clear field all'.
**
** Code generated:
**
**	IIclrflds();
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** History:
**	3-apr-87 (agh)
**		First written.
*/
VOID
IICGclaClrallGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIclrflds"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGclfClrfldGen() -	Generate code for OSL 'clear field' stmt
**
** Description:
**	Generates code for OSL 'clear field' statement.
**	If the original OSL code contained a statement of the form
**	'clear field fld1, fld2, ..., fldn', the translator turned
**	this into n IL statements, each doing a CLRFLD on one field.
**
** IL statements:
**	CLRFLD VALUE
**
**	The VALUE is the name of the field to be cleared.  This can
**	be a string constant, or the name of a field on the form.
**
** Code generated:
**
**	IIfldclear(field-name);
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGclfClrfldGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIfldclear"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGclsClrscrGen() -	Generate code for 'clear screen' stmt
**
** Description:
**	Generates code for OSL 'clear screen' statement.
**
** IL statements:
**	CLRSCR
**
**	The CLRSCR statement takes no arguments.  It is exactly equivalent
**	to an OSL 'clear screen'.
**
** Code generated:
**
**	IIclrscr();
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGclsClrscrGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIclrscr"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGgtfGetformGen() -	Generate code for a getform
**
** Description:
**	Generates code for a getform.
**
** IL statements:
**	GETFORM VALUE VALUE
**
**	The first VALUE names the field on which to do a 'getform';
**	the second is the data area for that field.
**
** Code generated:
**
**	if (IIfsetio(form-name) != 0)
**	{
**		IIARgfGetFldIO(field_name, &IIdbdvs[offset]);
**	}
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** History:
**	9/90 (Mike S) Use IIARgfGetFldIO (Bug 33374)
*/
VOID
IICGgtfGetformGen(stmt)
register IL	*stmt;
{
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIfsetio"));
	IICGocaCallArg( _FormVar );
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
	IICGoikIfBlock();
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARgfGetFldIO"));
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 2)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGobeBlockEnd();
	return;
}

/*{
** Name:	IICGptfPutformGen() -	Generate code for a putform
**
** Description:
**	Generates code for a putform.
**
** IL statements:
**	PUTFORM VALUE VALUE
**
**		The first VALUE names the field on which to do a 'putform'.
**		The second VALUE is the data area for that field.
**
** Code generated:
**
**	if (IIfsetio(form-name) != 0)
**	{
**		IIputfldio(field-name, (i2 *) NULL, 1, 44, 0, &IIdbdvs[offset]);
**	}
**
** Inputs:
**	stmt	The IL statement to generate code for.
*/
VOID
IICGptfPutformGen(stmt)
register IL	*stmt;
{
	char		buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIfsetio"));
	IICGocaCallArg( _FormVar );
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
	IICGoikIfBlock();
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIputfldio"));
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
	IICGocaCallArg( _ShrtNull );
	IICGocaCallArg( _One );
	CVna( DB_DBV_TYPE, buf );
	IICGocaCallArg(buf);
	IICGocaCallArg( _Zero );
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 2)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGobeBlockEnd();
	return;
}

/*{
** Name:	IICGrdyRedisplayGen() -	Generate code for OSL 'redisplay' stmt
**
** Description:
**	Generates code fors OSL 'redisplay' statement.
**
** IL statements:
**	REDISPLAY
**
**		The REDISPLAY statement takes no operands.
**
** Code generated:
**
**	IIredisp();
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrdyRedisplayGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIredisp"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGrsuResumeGen() -	Generate code for simple OSL 'resume'
**
** Description:
**	Generates code for a simple OSL 'resume' statement.
**
** IL statements:
**	RESUME
**	GOTO SID
**
**		The RESUME statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Code generated:
**	None.
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrsuResumeGen(stmt)
IL	*stmt;
{
	/*
	** No action needed here, since this IL statement is followed
	** by a 'GOTO'.
	*/
	return;
}

/*{
** Name:	IICGrscResColumnGen() -	Generate code for 'resume column' stmt
**
** Description:
**	Generates code for the OSL equivalent of a 'resume column' statement.
**	The routine IICGrsdResFieldGen() handles 'resume' statements that
**	refer to a simple field.
**
** IL statements:
**	RESCOL VALUE VALUE
**	GOTO SID
**
**		The first VALUE is the name of the table field, the second
**		the name of the column.
**		The RESCOL statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Code generated:
**
**	IIrescol(tblfld-name, column-name);
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrscResColumnGen(stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIrescol"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGstaStrArg(ILgetOperand(stmt, 2));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGrsfResFieldGen() -	Generate code for 'resume field' stmt
**
** Description:
**	Generates code for the OSL 'resume field' statement when it refers to a
**	simple field on the form.
**	The routine IICGrscResColumnGen() handles 'resume' statements that
**	refer to a column in a table field.
**
** IL statements:
**	RESFLD VALUE
**	GOTO SID
**
**		The VALUE is the name of the field to place the cursor on.
**		The RESFLD statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Code generated:
**
**	IIresfld(field-name);
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrsfResFieldGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIresfld"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGrsmResMenuGen() -	Generate code for OSL 'resume menu' stmt
**
** Description:
**	Generates code for OSL 'resume menu' statement.
**
** IL statements:
**	RESMENU
**	GOTO SID
**
**		The RESMENU statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Code generated:
**
**	IIresmu();
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrsmResMenuGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIresmu"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGrsnResNextGen() -	Generate code for OSL 'resume next' stmt
**
** Description:
**	Generates code for OSL 'resume next' statement.
**
** IL statements:
**	RESNEXT
**	GOTO SID
**
**		The RESNEXT statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Code generated:
**
**	IIresnext();
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrsnResNextGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIresnext"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:        IICGrseResEntryGen() - Generate code for OSL 'resume entry' stmt
**
** Description:
**	Generates code for OSL 'resume entry' statement.
**
** IL statements:
**	RESENTRY
**	GOTO SID
**
**		The RESENTRY statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Code generated:
**
**	IIFRreResEntry();
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
*/
VOID
IICGrseResEntryGen(stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIFRreResEntry"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGvldValidateGen() -	Generate code for an OSL 'validate' stmt
**
** Description:
**	Generates code for a simple OSL 'validate' statement.
**	A 'validate field' statement is handled by IICGvfValFieldGen().
**
** IL statements:
**	VALIDATE SID
**
**		The SID indicates the statement to goto if the
**		validation check succeeds.  If the check fails, code
**		to close any currently running queries and return to
**		the top of the display loop will immediately
**		follow the validation check.
**
** Code generated:
**
**	if (IIchkfrm() != 0)
**	{
**		goto next-OSL-statement;
**	}
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGvldValidateGen(stmt)
IL	*stmt;
{
	IL		*goto_stmt;

	goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 1));
	IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIchkfrm"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
	IICGoikIfBlock();
	IICGgtsGotoStmt(goto_stmt);
	IICGobeBlockEnd();
	return;
}

/*{
** Name:	IICGvlfValFieldGen() -	Generate code for 'validate field' stmt
**
** Description:
**	Generates code for an OSL 'validate field' statement.
**	A simple 'validate' statement is handled by IICGvldValidateGen().
**
** IL statements:
**	VALFLD VALUE SID
**
**		The VALUE indicates the name of the field.
**		The SID indicates the statement to goto if the
**		validation check succeeds.  If the check fails, code
**		to close any currently running queries and return to
**		the top of the display loop will immediately
**		follow the validation check.
**
** Code generated:
**
**	if (IIvalfld(field-name) != 0)
**	{
**		goto next-OSL-statement;
**	}
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGvlfValFieldGen(stmt)
IL	*stmt;
{
	IL		*goto_stmt;

	goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 2));
	IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIvalfld"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
	IICGoikIfBlock();
	IICGgtsGotoStmt(goto_stmt);
	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}


/*{
** Name:	IICGrnfResNxtFld - Generate code for 'rsume nextfield' stmt
**
** Description:
**	Generates code for the OSL equivalent of a 'resume nextfield' statement.
**
**	IL statements:
**		RESNFLD
**		GOTO SID
**
**	Code generated:
**		IIFRgotofld((int) 0);
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/29/93 (dkh) - Initial version.
*/
void
IICGrnfResNxtFld(stmt)
IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIFRgotofld"));
	STprintf(buf, ERx("(int) 0"));
	IICGocaCallArg(buf);
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}



/*{
** Name:	IICGrpfResPrvFld - Generate code for 'rsume previousfield' stmt
**
** Description:
**	Generates code for the OSL equivalent of a 'resume previousfield'
**	statement.
**
**	IL statements:
**		RESPFLD
**		GOTO SID
**
**	Code generated:
**		IIFRgotofld((int) 1);
**
** Inputs:
**	stmt	The IL statement to generate code for.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/29/93 (dkh) - Initial version.
*/
void
IICGrpfResPrvFld(stmt)
IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIFRgotofld"));
	STprintf(buf, ERx("(int) 1"));
	IICGocaCallArg(buf);
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}
