/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<st.h>
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
** Name:    cgarray.c -	Code Generator Array and Record Statements Module.
**
** Description:
**	Generates C code from the IL statements relating to arrays and records.
**	Includes a different IICGstmtGen() routine for each type of statement.
**
**	This file defines:
**
**	IICGcrClearRecGen()	-	Clear a record
**	IICGcarClearArrRowGen()	-	OSL 'clearrow' statement
**	IICGdarDeleteArrRowGen()-	OSL 'deleterow' statement
**	IICGiarInsertArrRowGen()-	OSL 'insertrow' statement
**	IICGua1UnldArr1Gen()	-	Begin executing an 'unloadtable' loop
**	IICGua2UnldArr2Gen()	-	Iterate through an 'unloadtable' loop
**	IICGuaeUnldArrEndGen()	-	Terminate an 'unloadtable' loop
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Created IIOcrClearRecExec (for bug 39582).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Created IICGuaeUnldArrEndGen
**		(for bugs 39581, 41013, and 41014: array performance).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails supporting long SIDs, and
**		introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**
**	Revision 6.0  87/04  arthur
**	Initial revision.
*/

char	*IICGginGetIndex();
char	*IndDef = ERx("(i4) 0");

/*{
** Name:    IICGcrClearRecGen() -	Generate code to clear a record.
**
** Description:
**	Generates code to clear a record.  Currently generated only
**	for a SELECT into a record.
**
** IL statements:
**	CLRREC VALUE
**
**		The VALUE contains the record object.
**
** Code generated:
**
**	IIARdocDotClear(&IIdbdvs[op1]);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	09/20/92 (emerson)
**		 Created. (for bug 39582).
*/
VOID
IICGcrClearRecGen (stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARdocDotClear"));
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*{
** Name:    IICGcarClearArrRowGen() -	Generate code for an OSL 'clearrow' stmt
**
** Description:
**	Generates code for an OSL 'clearrow' statement on an array.
**
** IL statements:
**	ARRCLRROW VALUE VALUE
**
**		In the CLRROW statement, the first VALUE contains the
**		dbdv of the array object.  The second VALUE indicates the row
**		number.  
**
** Code generated:
**
**	IIARarcArrayClear(&IIdbdvs[op1], *((i4 *) IIdbdvs[op2].db_data));
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGcarClearArrRowGen (stmt)
register IL	*stmt;
{
	IL		obj;
	char		*ind;
	IL		*next;
	char		buf[CGSHORTBUF];

	obj = ILgetOperand(stmt, 1);
        ind = IICGginGetIndex(ILgetOperand(stmt, 2), IndDef, ERx("CLEARROW"));

	IICGoibIfBeg();

        IICGocbCallBeg(ERx("IIARarrArrayRef"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGocaCallArg(ind);
	IICGocaCallArg(ERx("&IIelt"));
	IICGocaCallArg(_Zero);
	IICGoceCallEnd();

	IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
	IICGoikIfBlock();

	    next = IICGgilGetIl(stmt);
	    if ((*next&ILM_MASK) == IL_ENDLIST)
	    {
		IICGosbStmtBeg();
	    	IICGocbCallBeg(ERx("IIARdocDotClear"));
	    	IICGocaCallArg(ERx("&IIelt"));
	    	IICGoceCallEnd();
		IICGoseStmtEnd();
	    }
	    else
	    {
		while ((*next&ILM_MASK) == IL_TL1ELM)
		{
		    IICGosbStmtBeg();
		    IICGocbCallBeg(ERx("IIARdoaDotAssign"));
		    IICGocaCallArg(ERx("&IIelt"));
		    IICGstaStrArg(ILgetOperand(next, 1));
		    IICGocaCallArg(ERx("(DB_DATA_VALUE*) NULL"));
		    IICGoceCallEnd();
		    IICGoseStmtEnd();
		    next = IICGgilGetIl(next);
		}
		if ((*next&ILM_MASK) != IL_ENDLIST)
			IICGerrError(CG_ILMISSING, ERx("endlist"), (char*)NULL);
	    }

	IICGobeBlockEnd();

	return;
}

/*{
** Name:    IICGdarDeleteArrRowGen() -	Generate code for OSL 'deleterow' stmt
**
** Description:
**	Generates code for an OSL 'deleterow' statement on an array.
**
** IL statements:
**	DELROW VALUE VALUE
**
**		The first VALUE is the dbdv of the array object
**		The second VALUE is an integer row number.
**
** Code generated:
**
**	IIARardArrayDelete(&IIdbdvs[op1], *((i4 *) IIdbdvs[op2].db_data));
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGdarDeleteArrRowGen (stmt)
register IL	*stmt;
{
	IL		obj;
	char		*ind;

	obj = ILgetOperand(stmt, 1);
        ind = IICGginGetIndex(ILgetOperand(stmt, 2), IndDef, ERx("DELETEROW"));

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARardArrayDelete"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGocaCallArg(ind);
	IICGoceCallEnd();
	IICGoseStmtEnd();

	return;
}

/*{
** Name:	IICGiarInsertArrRowGen() -	Generate code for OSL 'insertrow' stmt
**
** Description:
**	Generates code for an OSL 'insertrow' statement.
**
** IL statements:
**	INSROW VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the INSROW statement, the first VALUE contains the
**		dbdv of an array object.
**		The second VALUE indicates the row number. 
**		Each TL2ELM element (if any) contains the name of a column
**		and the value to be placed into it.
**		The list is closed by an ENDLIST statement.
**
** Code generated:
**	1) No columns specified:
**
**		IIrownum = *((i4 *) IIdbdvs[op2].db_data);
**		IIARariArrayInsert(&IIdbdvs[op1], IIrownum);
**
**	2) Column names specified in OSL:
**
**		IIrownum = *((i4 *) IIdbdvs[op2].db_data);
**		if (IIARariArrayInsert(&IIdbdvs[op1], IIrownum)) == 0)
**		{
**			IIARarrArrayRef(&IIdbdvs[op1], IIrownum+1, &IIelt, 0);
**			IIARdoaDotAssign(&IIelt, op1, &IIdbdvs[op2]);
**				...
**		}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGiarInsertArrRowGen (stmt)
register IL	*stmt;
{
	register IL	*next;
	IL		obj;
	char		buf[CGSHORTBUF];
	char		*rn_name = ERx("IIrownum");
	char		*ind;

	obj = ILgetOperand(stmt, 1);

        ind = IICGginGetIndex(ILgetOperand(stmt, 2), IndDef, ERx("INSERTROW"));

	if ( !STequal(ind, rn_name) )
		IICGovaVarAssign(rn_name, ind);

	/* do we need an IF block? */
	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGoibIfBeg();
	else
		IICGosbStmtBeg();

	IICGocbCallBeg(ERx("IIARariArrayInsert"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGocaCallArg(rn_name);
	IICGoceCallEnd();

	if ((*next&ILM_MASK) == IL_ENDLIST)
	{
		IICGoseStmtEnd();
		return;
	}

	IICGoicIfCond((char *) NULL, CGRO_EQ, _Zero);
	IICGoikIfBlock();

	    IICGosbStmtBeg();
	    IICGocbCallBeg(ERx("IIARarrArrayRef"));
	    IICGocaCallArg(IICGgadGetAddr(obj));
	    IICGocaCallArg(STprintf(buf, ERx("%s+%s"), rn_name, _One));
	    IICGocaCallArg(ERx("&IIelt"));
	    IICGocaCallArg(_Zero);
	    IICGoceCallEnd();
	    IICGoseStmtEnd();

	    while ((*next&ILM_MASK) == IL_TL2ELM)
	    {
		IL from;

		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARdoaDotAssign"));
		IICGocaCallArg(ERx("&IIelt"));
		IICGstaStrArg(ILgetOperand(next, 1));
		from = ILgetOperand(next, 2);
		IICGocaCallArg(IICGgadGetAddr(from));
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	    }

	    if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char*)NULL);

	IICGrcsResetCstr();
	IICGobeBlockEnd();
}

/*{
** Name:    IICGua1UnldArr1Gen() -	Begin executing an 'unloadtable' loop
**
** Description:
**	Begin executing an 'unloadtable' loop on an array object.
**	This routine is called only once for each execution of an unloadtable
**	loop.
**	The routine IICGua2UnldArr2Gen() is called at the beginning of
**	each iteration through the unloadtable loop.
**
** IL statements:
**	ARR1UNLD VALUE VALUE SID
**	ARR2UNLD VALUE SID	(after)
**
**		In the UNLD1 statement, the 1st VALUE contains dbdv of the 
**		array.  The 2nd VALUE is the temp we'll unload from.
**		The SID indicates the end of the unloadtable loop.
**
** Code generated:
**
**	if (IIARaruArrayUnload(&IIdbdvs[unld_op1], &IIdbdvs[unld-op2]) != 0)
**	{
**		goto label2;
**	}
**   label1:
**	if (IIARarnArrayNext(&IIdbdvs[unld1-op2]) != 0)
**	{
**		goto label2;
**	}
**	else
**	{
**		IIARdotDotRef(&IIdbdvs[unld1-op1], unld2-op2, 
**						&IIdbdvs[unld2-op1], 1);
**			...
**	}
**
**	[ unload loop ]
**	goto label1;
**    label2:
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
**		Also fix an apparent bug: Compute the statement passed to
**		IICGlitLabelInsert from IL operand 3 instead operand 2.
**		(The statement passed to IICGgtsGotoStmt was being
**		computed from IL operand 3, as it should).
*/
VOID
IICGua1UnldArr1Gen (stmt)
register IL	*stmt;
{
	IL		obj;
	IL		tmpobj;
	IL		*goto_stmt;

	obj = ILgetOperand(stmt, 1);
	tmpobj = ILgetOperand(stmt, 2);
	goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 3));

	IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIARaruArrayUnload"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGocaCallArg(IICGgadGetAddr(tmpobj));
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	    IICGgtsGotoStmt(goto_stmt);
	IICGobeBlockEnd();
}

/*{
** Name:    IICGua2UnldArr2Gen() -	Iterate through an 'unloadtable' loop
**
** Description:
**	Iterates through an 'unloadtable' loop.  This routine is called
**	at the beginning of each iteration through the unloadtable loop.
**	The routine IICGua1UnldArr1Gen() has already checked the array and
**	done any necessary setup.
**
** IL statements:
**	UNLD1 VALUE SIDa		(before)
**	STHD SIDb
**	UNLD2 VALUE SIDa
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**	stmts within unloadtable block	(after)
**	GOTO SIDb
**	STHD SIDa
**
**		In the UNLD2 statement, VALUE is the array object being
**		unloaded, SIDa indicates the end of the unloadtable loop.
**		Each TL2ELM element (if any) contains a variable, and the
**		name of the array column or FRS constant to place
**		in that variable.
**		The list is closed by an ENDLIST statement.
**
**		After all the statements within the user's unloadtable
**		block have been executed, the statement GOTO SIDb returns
**		control to the UNLD2 statement at the head of the block.
**
** Code generated:
**	See comments for IICGua1UnldArr1Gen().
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGua2UnldArr2Gen (stmt)
register IL	*stmt;
{
	register IL	*next;
	IL		obj;
	IL		*goto_stmt;
	char	buf[CGSHORTBUF];

	goto_stmt = IICGesEvalSid(stmt, ILgetOperand(stmt, 2));
	IICGlblLabelGen(stmt);
	IICGlitLabelInsert(goto_stmt, CGLM_NOBLK);

	obj = ILgetOperand(stmt, 1);

	IICGoibIfBeg();

	IICGocbCallBeg(ERx("IIARarnArrayNext"));
	IICGocaCallArg(IICGgadGetAddr(obj));
	IICGoceCallEnd();

	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	    IICGgtsGotoStmt(goto_stmt);
	IICGobeBlockEnd();

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) == IL_ENDLIST)
		return;

	IICGoebElseBlock();

	    while ((*next&ILM_MASK) == IL_TL2ELM)
	    {
		IL	targ;

		targ = ILgetOperand(next, 1);
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARdotDotRef"));
		IICGocaCallArg(IICGgadGetAddr(obj));
		IICGstaStrArg(ILgetOperand(next, 2));
		IICGocaCallArg(IICGgadGetAddr(targ));
		IICGocaCallArg(_One);
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	    }
	    if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	IICGobeBlockEnd();
	IICGrcsResetCstr();

    	return;
}

/*{
** Name:    IICGuaeUnldArrEndGen() -	Terminate an 'unloadtable' loop
**
** Description:
**	Terminates an 'unloadtable' loop.  Used on an 'endloop' out of
**	an 'unloadtable'.
**
** IL statements:
**	ARRENDUNLD VALUE
**	GOTO SIDa
**
**	In the ARRENDUNLD statement, VALUE is the temp object into which
**	we're unloading.  SIDa indicates the end of the unloadtable loop.
**
** Code generated:
**    label1:
**	[ preceding portion of unload loop ]
**	IIARareArrayEndUnload(&IIdbdvs[arrendunld_op1]);
**	goto label2;
**	[ following portion of unload loop ]
**	goto label1;
**    label2:
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	11/07/91 (emerson)
**		Created (for bugs 39581, 41013, and 41014: array performance).
*/
VOID
IICGuaeUnldArrEndGen (stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIARareArrayEndUnload"));
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
    	return;
}
