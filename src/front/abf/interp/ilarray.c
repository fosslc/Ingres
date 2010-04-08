/*
**Copyright (c) 1989, 2004 Ingres Corporation
*/


#include	<compat.h>
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
#include	<eqrun.h>
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
** Name:	ilarray.c -	Execute array and record operations.
**
** Description:
**	Executes IL statements for array-row manipulations.
**	Includes a different IIOstmtExec() routine for each type of
**	statement.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Created IIOcrClearRecExec (for bug 39582).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Created IIOuaeUnloadArrEndExec
**		(for bugs 39581, 41013, and 41014: array performance).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.3/03/00  89/11  billc
**	Initial revision.
**	11/15/90 (emerson)
**		Bug fixes in IIOcawClearArrRowExec and IIOiarInsertArrRowExec.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

STATUS	IIARardArrayDelete();
STATUS	IIARariArrayInsert();
STATUS 	IARarrArrayRef();
STATUS	IIARaruArrayUnload();
STATUS	IIARarnArrayNext();
STATUS	IIARareArrayEndUnload();
STATUS	IIARarcArrayClear();
STATUS	IIARdoaDotAssign();

/*{
** Name:	IIOcrClearRecExec - Clear a record.
**
** Description:
**	Clears a record.  Currently generated only by osqrygen, for a SELECT
**	into a record.
**
** IL statements:
**	CLRREC VALUE
**
**		The VALUE contains the record object.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	09/20/92 (emerson)
**		Created. (for bug 39582).
*/
VOID
IIOcrClearRecExec(stmt)
register IL	*stmt;
{
	_VOID_ IIARdocDotClear(IIOFdgDbdvGet(ILgetOperand(stmt, 1)));
}

/*{
** Name:	IIOcawClearArrRowExec - Execute an OSL 'clearrow' stmt on array.
**
** Description:
**	Executes an OSL 'clearrow' statement.
**
** IL statements:
**	ARRCLRROW VALUE VALUE
**	TL1ELM VALUE
**	  ...
**	ENDLIST
**
**		In the CLRROW statement, the first VALUE contains the
**		array object.  The second VALUE indicates the row
**		number.
**		Each TL1ELM element (if any) contains the name of a column.
**		The list is closed by an ENDLIST statement.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History
**	11/89 (billc)	- Written
**	11/15/90 (emerson)
**		Add 4th arg to call to IIARarrArrayRef.
**	11/15/90 (emerson)
**		Initialize element before calling IIARarrArrayRef
**		(bug 34468).
*/
VOID
IIOcawClearArrRowExec(stmt)
register IL	*stmt;
{
	i4		rownum;
	register IL	*next;
    	DB_DATA_VALUE	*dbv;
    	DB_DATA_VALUE	element;

	dbv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	rownum = IIITvtnValToNat(ILgetOperand(stmt, 2), 0, ERx("CLEARROW"));

	/* setup 'element' to be an array element. */
	element.db_datatype = DB_DMY_TYPE;
	element.db_length = element.db_prec = 0;

	if (IIARarrArrayRef(dbv, rownum, &element, TRUE) != OK)
	{ /* scram.  error already reported in abfrt */
		IIOslSkipIl(stmt, IL_ENDLIST);
		return;
	}
	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) == IL_ENDLIST)
	{
		_VOID_ IIARdocDotClear(&element);
	}
	else
	{ /* insert the objects */
		while ((*next&ILM_MASK) == IL_TL1ELM)
  		{
			_VOID_ IIARdoaDotAssign( &element,
					IIITtcsToCStr(ILgetOperand(next, 1)),
					(DB_DATA_VALUE *)NULL
			);

			IIITrcsResetCStr();
	
			next = IIOgiGetIl(next);
		}
		if ((*next&ILM_MASK) != IL_ENDLIST)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING,
		    			ERx("endlist"), (char *) NULL);
		}
	}
	return;
}

/*{
** Name:	IIOdarDeleteArrRowExec	- Execute an OSL 'deleterow' stmt
**
** Description:
**	Executes an OSL 'deleterow' statement on an array.
**
** IL statements:
**	ARRDELROW VALUE VALUE
**
**		The first VALUE is the array object.
**		The second VALUE is an integer row number.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History
**	11/89 (billc)	- Written
**
*/
VOID
IIOdarDeleteArrRowExec(stmt)
register IL	*stmt;
{
	i4		rownum;
    	DB_DATA_VALUE	*dbv;

	dbv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	rownum = IIITvtnValToNat(ILgetOperand(stmt, 2), 0, ERx("DELETEROW"));
	_VOID_ IIARardArrayDelete(dbv, rownum);
	return;
}

/*{
** Name:	IIOiarInsertArrRowExec	-	Execute an OSL 'insertrow' stmt
**
** Description:
**	Executes an OSL 'insertrow' statement on an array row.
**
** IL statements:
**	ARRINSROW VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the INSROW statement, the first VALUE contains the
**		dbdv of an array object.  The second VALUE indicates the row
**		number.
**		Each TL2ELM element (if any) contains the name of a column
**		and the value to be placed into it.
**		The list is closed by an ENDLIST statement.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History
**	11/89 (billc)	- Written
**	11/15/90 (emerson)
**		Add 4th arg to call to IIARarrArrayRef.
*/
VOID
IIOiarInsertArrRowExec(stmt)
register IL	*stmt;
{
	i4		rownum;
	register IL	*next;
	DB_DATA_VALUE	*dbv;
	DB_DATA_VALUE	element;

	dbv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	rownum = IIITvtnValToNat(ILgetOperand(stmt, 2), 0, ERx("INSERTROW"));

	/* We have an array object. */
	if ( IIARariArrayInsert(dbv, rownum) != OK )
	{ /* scram.  error already reported in abfrt */
		IIOslSkipIl(stmt, IL_ENDLIST);
		return;
	}

	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) == IL_ENDLIST)
		return;

	/* setup 'element' to be an array element. */
	element.db_datatype = DB_DMY_TYPE;
	element.db_length = element.db_prec = 0;

	/* get the new array row */
	_VOID_ IIARarrArrayRef(dbv, (i4) (rownum + 1), &element, FALSE);

	/* insert the objects */
	while ((*next&ILM_MASK) == IL_TL2ELM)
  	{
		_VOID_ IIARdoaDotAssign( &element,
				IIITtcsToCStr(ILgetOperand(next, 1)),
   				IIOFdgDbdvGet(ILgetOperand(next, 2))
		);

		next = IIOgiGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST)
	{
		IIOerError(ILE_STMT, ILE_ILMISSING,
		    		ERx("endlist"), (char *) NULL);
	}
	return;
}

/*{
** Name:	IIOua1UnloadArr1Exec	- Begin executing an 'unloadtable' loop
**
** Description:
**	Begin executing an 'unloadtable' loop on an array object.
**	This routine is
**	called only once for each execution of an unloadtable loop.
**	The routine IIOua2UnloadArr2Exec() is called at the beginning of
**	each iteration through the unloadtable loop.
**
** IL statements:
**	ARR1UNLD VALUE VALUE SID
**	ARR2UNLD VALUE SID	(after)
**
**		In the ARR1UNLD statement, the 1st VALUE contains the array
**		object.  The 2nd VALUE is the temp we'll unload through.
**		The SID indicates the end of the unloadtable loop.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History
**	11/89 (billc)	- Written
**
*/
VOID
IIOua1UnloadArr1Exec(stmt)
register IL	*stmt;
{
    	DB_DATA_VALUE *dbv;
    	DB_DATA_VALUE *tdbv;

	dbv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));
	tdbv = IIOFdgDbdvGet(ILgetOperand(stmt, 2));
	if (IIARaruArrayUnload(dbv, tdbv) != OK)
		IIOsiSetIl(ILgetOperand(stmt, 3));
	return;
}

/*{
** Name:	IIOua2UnloadArr2Exec	- Iterate through an 'unloadtable' loop
**
** Description:
**	Iterates through an 'unloadtable' loop on an array object.
**	This routine is called at the beginning of
**	each iteration through the unloadtable loop, and increments the array
**	row.
**
** IL statements:
**	ARR1UNLD VALUE SIDa		(before)
**	STHD SIDb
**	ARR2UNLD VALUE SIDa
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**	stmts within unloadtable block	(after)
**	GOTO SIDb
**	STHD SIDa
**
**		In the ARR2UNLD statement, VALUE is the current array element,
**		SIDa indicates the end of the unloadtable loop.
**		Each TL2ELM element (if any) contains a variable, and the
**		name of the array column or FRS constant to place
**		in that variable.
**		The list is closed by an ENDLIST statement.
**
**		After all the statements within the user's unloadtable
**		block have been executed, the statement GOTO SIDb returns
**		control to the ARR2UNLD statement at the head of the block.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History
**	11/89 (billc)	- Written
**
*/
VOID
IIOua2UnloadArr2Exec(stmt)
register IL	*stmt;
{
    	register IL	*next;
    	DB_DATA_VALUE	*dbv;

	dbv = IIOFdgDbdvGet(ILgetOperand(stmt, 1));

	if (IIARarnArrayNext( dbv ) != OK)
	{ /* no more array elements. */
		IIOsiSetIl(ILgetOperand(stmt, 2));
		return;
	}

	next = IIOgiGetIl(stmt);
	while ((*next&ILM_MASK) == IL_TL2ELM)
	{
    		_VOID_ IIARdotDotRef( dbv,
			IIITtcsToCStr(ILgetOperand(next, 2)),
	       		IIOFdgDbdvGet(ILgetOperand(next, 1)),
			TRUE /* coerce dbv if necessary */
		);

    		next = IIOgiGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST)
	{
    		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
	       		(char *) NULL);
	}
	return;
}

/*{
** Name:	IIOuaeUnloadArrEndExec	- Terminate an 'unloadtable' loop
**
** Description:
**	Terminates an 'unloadtable' loop on an array object.
**	Used when exiting an 'unloadtable' via an 'endloop' statement.
**
** IL statements:
**	ARRENDUNLD VALUE
**	GOTO SIDa
**
**		In the ARRENDUNLD statement, VALUE is the current array element.
**		SIDa indicates the end of the unloadtable loop.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	11/07/91 (emerson)
**		Created (for bugs 39581, 41013, and 41014: array performance).
*/
VOID
IIOuaeUnloadArrEndExec(stmt)
register IL	*stmt;
{
	_VOID_ IIARareArrayEndUnload(IIOFdgDbdvGet(ILgetOperand(stmt, 1)));
	return;
}
