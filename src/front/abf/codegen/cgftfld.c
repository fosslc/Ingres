/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<frscnsts.h>
#include	<eqrun.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>

#include	"cggvars.h"
#include	"cgil.h"
#include	"cglabel.h"
#include	"cgout.h"
#include	"cgerror.h"


/**
** Name:    cgftfld.c -	Code Generator Table Field Statements Module.
**
** Description:
**	Generates C code from the IL statements relating to table fields.
**	Includes a different IICGstmtGen() routine for each type of
**	statement.
**	Other generation routines for forms statements are in
**	cgfdisp.c and cgfetc.c.
**	This file defines:
**
**	IICGcrwClearrowGen()	-	OSL 'clearrow' statement
**	IICGdrwDeleterowGen()	-	OSL 'deleterow' statement
**	IICGeudEndUnldGen()	-	'endloop' from an 'unloadtable' loop
**	IICGgrwGetrowGen()	-	Generate code for a getrow
**	IICGitbInittableGen()	-	OSL 'inittable' statement
**	IICGltfLoadTFGen()	-	OSL 'loadtable' statement
**	IICGirwInsertrowGen()	-	OSL 'insertrow' statement
**	IICGprwPutrowGen()	-	Generate code for a putrow
**	IICGul1Unldtbl1Gen()	-	Begin executing an 'unloadtable' loop
**	IICGul2Unldtbl2Gen()	-	Iterate through an 'unloadtable' loop
**	IICGvrwValidrowGen()	-	OSL 'validrow' statement
**	IICGptPurgeTbl()	-	OSL 'purgetable' statement
**
** History:
**	Revision 6.0  87/04  arthur
**	Initial revision.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**
**	Revision 6.4/02
**	12/14/91 (emerson)
**		Part of fix for bug 38940 in IICGitbInittableGen.
**
**	Revision 6.5
**	05-feb-93 (davel)
**		Added support for cell attributes "with clause"
**		to insertrow and loadtable functions (IICGirwInsertrowGen() and
**		IICGltfLoadTFGen() ).
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN char *IICGginGetIndex();

/*{
** Name:    IICGcrwClearrowGen() -	Generate code for an OSL 'clearrow' stmt
**
** Description:
**	Generates code for an OSL 'clearrow' statement.
**
** IL statements:
**	CLRROW VALUE VALUE
**	TL1ELM VALUE
**	  ...
**	ENDLIST
**
**		In the CLRROW statement, the first VALUE contains the
**		name of the table field.  The second VALUE indicates the row
**		number.  This VALUE may be NULL, implying the current row.
**		Each TL1ELM element (if any) contains the name of a column.
**		The list is closed by an ENDLIST statement.
**
** Code generated:
**	1) No columns named in OSL; no TL1ELM statements:
**
**	if (IItbsetio(6, form-name, tblfld-name, -2) != 0)
**	{
**		IItclrrow();
**	}
**
**	2) Columns specified in OSL:
**
**	if (IItbsetio(6, form-name, tblfld-name, rownum) != 0)
**	{
**		IItclrcol(column-name-1);
**		IItclrcol(column-name-2);
**		...
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGcrwClearrowGen (stmt)
register IL	*stmt;
{
    register IL	*next;
    char	*rownum;
    char	buf[CGSHORTBUF];

    STprintf(buf, ERx("(i4) %d"), rowCURR);
    rownum = IICGginGetIndex(ILgetOperand(stmt, 2), buf, ERx("CLEARROW"));

    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbsetio"));
    CVna(cmCLEARR, buf);
    IICGocaCallArg(buf);
    IICGocaCallArg( _FormVar );
    IICGstaStrArg(ILgetOperand(stmt, 1));
    IICGocaCallArg(STprintf(buf, ERx("(i4) %s"), rownum));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL1ELM)
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItclrrow"));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();
	}
	else do
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItclrcol"));
	    IICGstaStrArg(ILgetOperand(next, 1));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL1ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

        IICGocbCallBeg(ERx("IITBceColEnd"));
        IICGoceCallEnd();
        IICGoseStmtEnd();

    IICGobeBlockEnd();
    IICGrcsResetCstr();
    return;
}

/*{
** Name:    IICGdrwDeleterowGen() -	Generate code for OSL 'deleterow' stmt
**
** Description:
**	Generates code for an OSL 'deleterow' statement.
**
** IL statements:
**	DELROW VALUE VALUE
**
**		The first VALUE names the table field.
**		The second VALUE, which may be NULL, is an integer row number.
**
** Code generated:
**	1) No row number specified:
**
** 	if (IItbsetio(4, form-name, tblfld-name, -2) != 0)
** 	{
** 		IItdelrow(0);
** 	}
**
**	2) Row number specified in OSL:
**
** 	if (IItbsetio(4, form-name, tblfld-name, rownum) != 0)
** 	{
** 		IItdelrow(0);
** 	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGdrwDeleterowGen (stmt)
register IL	*stmt;
{
	char	*rownum;
	char	buf[CGSHORTBUF];

	STprintf(buf, ERx("(i4) %d"), rowCURR);
	rownum = IICGginGetIndex(ILgetOperand(stmt, 2), buf, ERx("DELETEROW"));

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IItbsetio"));
	CVna(cmDELETER, buf);
	IICGocaCallArg(buf);
	IICGocaCallArg( _FormVar );
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGocaCallArg(STprintf(buf, ERx("(i4) %s"), rownum));
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IItdelrow"));
	IICGocaCallArg(_Zero);
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:    IICGeudEndUnldGen() -	'endloop' from an 'unloadtable' loop
**
** Description:
**	Generates code for an 'endloop' from an 'unloadtable' loop.
**	Endloop's from an unloadtable or from an attached retrieve
**	use different IL statements.  No separate ENDLOOP-type statement is
**	required for an endloop from a while loop.
**
**	In each case, the series of ENDLOOP-type statements is followed
**	by a GOTO to the IL statement immediately following the outer
**	loop being broken.
**
** IL statements:
**	UNLD1 VALUE SIDa
**	STHD SIDb
**	UNLD2 SIDa
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**	stmts within unloadtable block	(before)
**	ENDUNLD
**	GOTO SIDa
**	more stmts in unloadtable block	(after)
**	GOTO SIDb
**	STHD SIDa
**
**		This example shows the basic case of an endloop applied to
**		an unloadtable block.  The unloadtable loop is not
**		itself nested within any other loops.
**
** Code generated:
**	IItunend();
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGeudEndUnldGen (stmt)
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IItunend"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:    IICGgrwGetrowGen() -	Generate code for a getrow
**
** Description:
**	Generates code for a getrow.
**
** IL statements:
**	GETROW VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the GETROW statement, the first VALUE names the table
**		field, and the second the row number (if any).
**		Each GETROW statement is followed by a series of
**		TL2ELM statements.  The first VALUE in each TL2ELM statement
**		is the variable in which the data is put, and the second
**		is the column name.  The list is closed with an ENDLIST
**		statement.
**
** Code generated:
**
**	if (IItbsetio(2, form-name, tblfld-name, rownum) != 0)
**	{
**		IIARgtcTcoGetIO(column-name, &IIdbdvs[offset]);
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGgrwGetrowGen (stmt)
register IL	*stmt;
{
    register IL	*next;
    char	buf[CGSHORTBUF];
    char	*rownum;

    STprintf(buf, ERx("(i4) %d"), rowCURR);
    rownum = IICGginGetIndex(ILgetOperand(stmt, 2), buf, ERx("GETROW"));

    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbsetio"));
    CVna(cmGETR, buf);
    IICGocaCallArg(buf);
    IICGocaCallArg( _FormVar );
    IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
    IICGocaCallArg(STprintf(buf, ERx("(i4) %s"), rownum));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
	    IICGerrError(CG_ILMISSING, ERx("getrow element"), (char *) NULL);
	}
	else do
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IIARgtcTcoGetIO"));
	    IICGocaCallArg(IICGgvlGetVal(ILgetOperand(next, 2), DB_CHR_TYPE));
	    IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

        IICGocbCallBeg(ERx("IITBceColEnd"));
        IICGoceCallEnd();
        IICGoseStmtEnd();

    IICGobeBlockEnd();
    return;
}

/*{
** Name:    IICGitbInittableGen() -	Generate code for OSL 'inittable' stmt
**
** Description:
**	Generates code for an OSL 'inittable' statement.
**
** IL statements:
**	INITTAB VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the INITTAB statement, the first VALUE contains the
**		name of the table field.  The second VALUE names the mode.
**		This VALUE may be NULL, implying 'fill' mode and also that
**		the INITTAB is being done for frame initialization.
**		Each TL2ELM element (if any) contains the name and format
**		of a hidden column.
**		The list is closed by an ENDLIST statement.
**
** Code generated:
**	1) No mode specified.  No hidden columns:
**
**	if (IItbinit(form-name, tblfld-name, ERx("f")) != 0)
**	{
**		IItfill();
**	}
**
**	2) Mode specified in OSL.  Hidden columns:
**
**	if (IItbinit(form-name, tblfld-name, mode) != 0)
**	{
**		IIthidecol(hidden-col-name-1, format);
**		IIthidecol(hidden-col-name-2, format);
**		...
**		IItfill();
**	}
**
**	Note: If this is static frame and the INITTAB is being done for frame
**	initialization, we generate code to skip the tablefield initialization
**	on second and subsequent calls to the frame.
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	12/14/91 (emerson)
**		Part of fix for bug 38940: If the INITTAB is *not* being done
**		for frame initialization, then *don't* skip the tablefield
**		initialization on second and subsequent calls to a static frame.
*/
VOID
IICGitbInittableGen (stmt)
register IL	*stmt;
{
	register IL	*next;
	char		buf[CGSHORTBUF];
	IL		op2 = ILgetOperand(stmt, 2);

	IICGoibIfBeg();
	if (CG_static && ILNULLVAL(op2))
	{
		IICGoicIfCond(ERx("first"), CGRO_EQ, _One);
		IICGoprPrint(ERx(" && "));
	}
	IICGocbCallBeg( ERx("IItbinit") );
	IICGocaCallArg( _FormVar );
	IICGstaStrArg( ILgetOperand(stmt, 1) );
	ILSTRARG( op2, ERx("\"f\"") );	/* mode */
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();

		next = IICGgilGetIl(stmt);
		while ((*next&ILM_MASK) == IL_TL2ELM)
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIthidecol"));
			IICGocaCallArg(IICGgvlGetVal(ILgetOperand(next, 1), DB_CHR_TYPE));
			IICGocaCallArg(IICGgvlGetVal(ILgetOperand(next, 2), DB_CHR_TYPE));
			IICGoceCallEnd();
			IICGoseStmtEnd();

			next = IICGgilGetIl(next);
		}
		if ((*next&ILM_MASK) != IL_ENDLIST)
			IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IItfill"));
		IICGoceCallEnd();
		IICGoseStmtEnd();

	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGltfLoadTFGen() -	Generate code for OSL 'loadtable' stmt
**
** Description:
**	Generates code for an OSL 'loadtable' statement.
**
** IL statements:
**	LOADTABLE VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**      INQELM VALUE VALUE VALUE INT-VALUE
**	  ...
**	ENDLIST
**
**		In the LOADTABLE statement, the first VALUE contains the
**		name of the table field.  The second VALUE is ignored.
**		Each TL2ELM element (if any) contains the name of a column
**		and the value to be placed into it.
**		The list is closed by an ENDLIST statement.
**
**		If a with clause is specified, the additional INQELM list
**		will exist and will contain a list of columns and display
**		attributes to be set in the newly loaded row.
**
** Code generated:
**
**	if (IItbact(form-name, tblfld-name, 1) != 0)
**	{
**		IItcoputio(column-name-1, (i2 *) NULL, 1, 44, 0,
**			&IIdbdvs[offset-1]);
**		IItcoputio(column-name-2, (i2 *) NULL, 1, 44, 0,
**			&IIdbdvs[offset-2]);
**			...
**		IIFRsaSetAttrio(display-attr-1, column-name-1, 
**			(i2 *) NULL, 1, 44, 0, &IIdbdvs[offset-1]);
**		IIFRsaSetAttrio(display-attr-2, column-name-2, 
**			(i2 *) NULL, 1, 44, 0, &IIdbdvs[offset-2]);
**		...
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGltfLoadTFGen (stmt)
register IL	*stmt;
{
    register IL	*next;
    i4  has_cellattr = *stmt & ILM_HAS_CELL_ATTR;
    char	buf[CGSHORTBUF];

    STprintf(buf, ERx("(i4) %d"), rowCURR);

    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbact"));
    IICGocaCallArg( _FormVar );
    IICGstaStrArg(ILgetOperand(stmt, 1));
    IICGocaCallArg(STprintf(buf, ERx("(i4) %d"), 1));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	next = IICGgilGetIl(stmt);
	while ((*next&ILM_MASK) == IL_TL2ELM)
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItcoputio"));
	    IICGstaStrArg(ILgetOperand(next, 1));
	    IICGocaCallArg(_ShrtNull);
	    IICGocaCallArg(_One);
	    CVna((i4) DB_DBV_TYPE, buf);
	    IICGocaCallArg(buf);
	    IICGocaCallArg(_Zero);
	    IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 2)));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    next = IICGgilGetIl(next);
	}

	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	if (has_cellattr)
	{
		next = IICGgilGetIl(next);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IICGerrError(CG_ILMISSING, ERx("inqelm"), (char *)NULL);
		}
		else do
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIFRsaSetAttrio"));
			CVna((i4) ILgetOperand(next, 4), buf);
			IICGocaCallArg(buf);
			ILSTRARG( ILgetOperand(next, 2), _NullPtr );
			IICGocaCallArg(_ShrtNull);  /* null indicator */
			IICGocaCallArg(_One);
			CVna( DB_DBV_TYPE, buf );
			IICGocaCallArg(buf);
			IICGocaCallArg(_Zero);
			IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
			IICGoceCallEnd();
			IICGoseStmtEnd();

			next = IICGgilGetIl(next);
		} while ((*next&ILM_MASK) == IL_INQELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
		    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	}

	IICGocbCallBeg(ERx("IITBceColEnd"));
	IICGoceCallEnd();
	IICGoseStmtEnd();

    IICGobeBlockEnd();
    IICGrcsResetCstr();
    return;
}

/*{
** Name:	IICGirwInsertrowGen() -	Generate code for OSL 'insertrow' stmt
**
** Description:
**	Generates code for an OSL 'insertrow' statement.
**
** IL statements:
**	INSROW VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**      INQELM VALUE VALUE VALUE INT-VALUE
**	  ...
**	ENDLIST
**
**		In the INSROW statement, the first VALUE contains the
**		name of the table field.  The second VALUE indicates the row
**		number.  This VALUE may be NULL, implying the current row.
**		Each TL2ELM element (if any) contains the name of a column
**		and the value to be placed into it.
**		The list is closed by an ENDLIST statement.
**
**		If a with clause is specified, the additional INQELM list
**		will exist and will contain a list of columns and display
**		attributes to be set in the newly inserted row.
**
** Code generated:
**	1) No row number or columns specified:
**
**	if (IItbsetio(5, form-name, tblfld-name, -2) != 0)
**	{
**		if (IItinsert() != 0)
**		{
**		}
**	}
**
**	2) Row number and column names specified in OSL:
**
**	if (IItbsetio(5, form-name, tblfld-name, rownum) != 0)
**	{
**		if (IItinsert() != 0)
**		{
**			IItcoputio(column-name-1, (i2 *) NULL, 1, 44, 0,
**				&IIdbdvs[offset-1]);
**			IItcoputio(column-name-2, (i2 *) NULL, 1, 44, 0,
**				&IIdbdvs[offset-2]);
**			...
**			IIFRsaSetAttrio(display-attr-1, column-name-1, 
**				(i2 *) NULL, 1, 44, 0, &IIdbdvs[offset-1]);
**			IIFRsaSetAttrio(display-attr-2, column-name-2, 
**				(i2 *) NULL, 1, 44, 0, &IIdbdvs[offset-2]);
**			...
**		}
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGirwInsertrowGen (stmt)
register IL	*stmt;
{
    register IL	*next;
    i4  has_cellattr = *stmt & ILM_HAS_CELL_ATTR;
    char	buf[CGSHORTBUF];
    char	*rownum;

    STprintf(buf, ERx("(i4) %d"), rowCURR);
    rownum = IICGginGetIndex(ILgetOperand(stmt, 2), buf, ERx("INSERTROW"));

    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbsetio"));
    CVna(cmINSERTR, buf);
    IICGocaCallArg(buf);
    IICGocaCallArg( _FormVar );
    IICGstaStrArg(ILgetOperand(stmt, 1));
    IICGocaCallArg(STprintf(buf, ERx("(i4) %s"), rownum));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IItinsert"));
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();

	    next = IICGgilGetIl(stmt);
	    while ((*next&ILM_MASK) == IL_TL2ELM)
	    {
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IItcoputio"));
		IICGstaStrArg(ILgetOperand(next, 1));
		IICGocaCallArg(_ShrtNull);
		IICGocaCallArg(_One);
		CVna((i4) DB_DBV_TYPE, buf);
		IICGocaCallArg(buf);
		IICGocaCallArg(_Zero);
		IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 2)));
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	    }

	    if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	    if (has_cellattr)
	    {
		next = IICGgilGetIl(next);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IICGerrError(CG_ILMISSING, ERx("inqelm"), (char *)NULL);
		}
		else do
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIFRsaSetAttrio"));
			CVna((i4) ILgetOperand(next, 4), buf);
			IICGocaCallArg(buf);
			ILSTRARG( ILgetOperand(next, 2), _NullPtr );
			IICGocaCallArg(_ShrtNull);  /* null indicator */
			IICGocaCallArg(_One);
			CVna( DB_DBV_TYPE, buf );
			IICGocaCallArg(buf);
			IICGocaCallArg(_Zero);
			IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
			IICGoceCallEnd();
			IICGoseStmtEnd();

			next = IICGgilGetIl(next);
		} while ((*next&ILM_MASK) == IL_INQELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
		    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	    }

            IICGocbCallBeg(ERx("IITBceColEnd"));
            IICGoceCallEnd();
            IICGoseStmtEnd();

	IICGobeBlockEnd();
    IICGobeBlockEnd();
    IICGrcsResetCstr();
    return;
}

/*{
** Name:    IICGprwPutrowGen() -	Generate code for a putrow
**
** Description:
**	Generates code for a putrow.
**
** IL statements:
**	PUTROW VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the PUTROW statement, the first VALUE names the table
**		field, and the second the row number (if any).
**		Each PUTROW statement is followed by a series of
**		TL2ELM statements.  The first VALUE in each TL2ELM statement
**		is the column name, and the second the variable in which the
**		data is put.  The list is closed with an ENDLIST statement.
**
** Code generated:
**	if (IItbsetio(3, form-name, tblfld-name, rownum) != 0)
**	{
**		IItcoputio(column-name, (i2 *)NULL, 1, 44, 0, &IIdbdvs[offset]);
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGprwPutrowGen (stmt)
register IL	*stmt;
{
    register IL	*next;
    char	buf[CGSHORTBUF];
    char	*rownum;

    STprintf(buf, ERx("(i4) %d"), rowCURR);
    rownum = IICGginGetIndex(ILgetOperand(stmt, 2), buf, ERx("PUTROW"));

    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbsetio"));
    CVna(cmPUTR, buf);
    IICGocaCallArg(buf);
    IICGocaCallArg( _FormVar );
    IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
    IICGocaCallArg(STprintf(buf, ERx("(i4) %s"), rownum));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
	    IICGerrError(CG_ILMISSING, ERx("putrow element"), (char *) NULL);
	}
	else do
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItcoputio"));
	    IICGocaCallArg(IICGgvlGetVal(ILgetOperand(next, 1), DB_CHR_TYPE));
	    IICGocaCallArg(_ShrtNull);
	    IICGocaCallArg(_One);
	    CVna((i4) DB_DBV_TYPE, buf);
	    IICGocaCallArg(buf);
	    IICGocaCallArg(_Zero);
	    IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 2)));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

        IICGocbCallBeg(ERx("IITBceColEnd"));
        IICGoceCallEnd();
        IICGoseStmtEnd();

    IICGobeBlockEnd();
    return;
}

/*{
** Name:    IICGul1Unldtbl1Gen() -	Begin executing an 'unloadtable' loop
**
** Description:
**	Begin executing an 'unloadtable' loop.
**	This routine sets up the table field with its data set, and is
**	called only once for each execution of an unloadtable loop.
**	The routine IICGul2Unldtbl2Gen() is called at the beginning of
**	each iteration through the unloadtable loop.
**
** IL statements:
**	UNLD1 VALUE SID
**	UNLD2 SID	(after)
**
**		In the UNLD1 statement, the VALUE contains the name
**		of the table field.  The SID indicates the end of the
**		unloadtable loop.
**
** Code generated:
**
** label-1: ;
**	if (IItunload() != 0)
**	{
**		IItcogetio((i2 *) NULL, 1, 44, 0, &IIdbdvs[offset], col-name-1);
**		IItcogetio((i2 *) NULL, 1, 44, 0, &IIdbdvs[offset], col-name-2);
**		...
**	}
**	else
**	{
**		IItunend();
**		goto label-2;
**	}
**
**	[ body of unloadtable loop ]
**
**	goto label-1;
** label-2:
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGul1Unldtbl1Gen (stmt)
register IL	*stmt;
{
    char	buf[CGSHORTBUF];
    IL		*goto_stmt;

    goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 2));
    IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbact"));
    IICGocaCallArg( _FormVar );
    IICGstaStrArg(ILgetOperand(stmt, 1));
    IICGocaCallArg(_Zero);
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
    IICGoikIfBlock();
	IICGgtsGotoStmt(goto_stmt);
    IICGobeBlockEnd();
    return;
}

/*{
** Name:    IICGul2Unldtbl2Gen() -	Iterate through an 'unloadtable' loop
**
** Description:
**	Iterates through an 'unloadtable' loop.  This routine is called
**	at the beginning of each iteration through the unloadtable loop.
**	The routine IICGul1Unldtbl1Gen() has already set up the table field
**	with its data set.
**
** IL statements:
**	UNLD1 VALUE SIDa		(before)
**	STHD SIDb
**	UNLD2 SIDa
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**	stmts within unloadtable block	(after)
**	GOTO SIDb
**	STHD SIDa
**
**		In the UNLD2 statement, SIDa indicates the end of
**		the unloadtable loop.
**		Each TL2ELM element (if any) contains a variable, and the
**		name of the table field column or FRS constant to place
**		in that variable.
**		The list is closed by an ENDLIST statement.
**
**		After all the statements within the user's unloadtable
**		block have been executed, the statement GOTO SIDb returns
**		control to the UNLD2 statement at the head of the block.
**
** Code generated:
**	See comments for IICGul1Unldtbl1Gen().
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGul2Unldtbl2Gen (stmt)
register IL	*stmt;
{
    register IL	*next;
    char	buf[CGSHORTBUF];
    IL		*goto_stmt;

    goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 1));
    IICGlblLabelGen(stmt);
    IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);
    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItunload"));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	next = IICGgilGetIl(stmt);
	while ((*next&ILM_MASK) == IL_TL2ELM)
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItcogetio"));
	    IICGocaCallArg(_ShrtNull);
	    IICGocaCallArg(_One);
	    CVna( DB_DBV_TYPE, buf );
	    IICGocaCallArg(buf);
	    IICGocaCallArg(_Zero);
	    IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
	    IICGstaStrArg(ILgetOperand(next, 2));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    next = IICGgilGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

        IICGocbCallBeg(ERx("IITBceColEnd"));
        IICGoceCallEnd();
        IICGoseStmtEnd();

    IICGobeBlockEnd();
    IICGoebElseBlock();

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IItunend"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGgtsGotoStmt(goto_stmt);

    IICGobeBlockEnd();
    IICGrcsResetCstr();
    return;
}

/*{
** Name:    IICGvrwValidrowGen() -	Generate code for an OSL 'validrow' stmt
**
** Description:
**	Generates code for an OSL 'validrow' statement.
**
** IL statements:
**	VALROW VALUE VALUE SID
**	TL1ELM VALUE
**	  ...
**	ENDLIST
**
**		In the VALROW statement, the first VALUE contains the
**		name of the table field.  The second VALUE indicates the row
**		number.  This VALUE may be NULL, implying the current row.
**		The SID indicates the statement to goto if the
**		validation check succeeds.  If the check fails, code
**		to close any currently running queries and return to
**		the top of the display loop will immediately
**		follow the validation check.
**
**		Each TL1ELM element (if any) contains the name of a column.
**		The list is closed by an ENDLIST statement.
**
** Code generated:
**	1) No row number or column names specified:
**
**	if (IItbsetio(7, form-name, tblfld-name, -2) != 0)
**	{
**		IItvalrow();
**		if (IItvalval(0) != 0)
**		{
**			goto next-OSL-statement;
**		}
**	}
**
**	2) Row number and column names specified in OSL:
**
**	if (IItbsetio(7, form-name, tblfld-name, rownum) != 0)
**	{
**		IItcolval(column-name-1);
**		IItcolval(column-name-2);
**		...
**		if (IItvalval(0) != 0)
**		{
**			goto next-OSL-statement;
**		}
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGvrwValidrowGen (stmt)
register IL	*stmt;
{
    register IL	*next;
    char	buf[CGSHORTBUF];
    char	*rownum;
    IL		*goto_stmt;

    goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 3));
    IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);

    STprintf(buf, ERx("(i4) %d"), rowCURR);
    rownum = IICGginGetIndex(ILgetOperand(stmt, 2), buf, ERx("VALIDROW"));

    IICGoibIfBeg();
    IICGocbCallBeg(ERx("IItbsetio"));
    CVna(cmVALIDR, buf);
    IICGocaCallArg(buf);
    IICGocaCallArg( _FormVar );
    IICGstaStrArg(ILgetOperand(stmt, 1));
    IICGocaCallArg(STprintf(buf, ERx("(i4) %s"), rownum));
    IICGoceCallEnd();
    IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
    IICGoikIfBlock();

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL1ELM)
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItvalrow"));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();
	}
	else do
	{
	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IItcolval"));
	    IICGstaStrArg(ILgetOperand(next, 1));
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL1ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	/*
	** If the validation succeeds, continue execution of next OSL statement.
	** If the validation fails, IL code will follow to return to the top of
	** the display loop.
	*/
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IItvalval"));
	IICGocaCallArg(_Zero);
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	    IICGgtsGotoStmt(goto_stmt);
	IICGobeBlockEnd();
    IICGobeBlockEnd();
    IICGrcsResetCstr();
    return;
}

/*{
** Name:	IICGptPurgeTbl - Generate code for OSL 'purgetable' statement
**
** Description:
**	Generates code for an OSL 'purgetable' statement.
**
**	IL statements:
**	    PURGETABLE VALUE VALUE
**
**		The first is the name of the form which may be NULL.
**		The second value is the name of the table field.
**
**	Code genereated:
**		if (IItbsetio((int)8,form_,table_field,(int)0) != (int)0) {
**		}
** Inputs:
**	stmt	The IL statement for which to generate code.
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
IICGptPurgeTbl(stmt)
IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IItbsetio"));
	STprintf(buf, ERx("(int) %d"), cmPURGE);
	IICGocaCallArg(buf);
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGstaStrArg(ILgetOperand(stmt, 2));
	IICGocaCallArg(ERx("(int) 0"));
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	IICGobeBlockEnd();
	IICGrcsResetCstr();
}
