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
#include	<adf.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<eqrun.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<frame.h>
#include	<runtime.h>
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
** Name:	ilftfld.c -	Execute table field-related forms stmts
**
** Description:
**	Executes IL statements for OSL forms statements which relate
**	to table fields.
**	Includes a different IIOstmtExec() routine for each type of
**	statement.
**	Other execution routines for forms statements are in
**	ilfdisp.qc and ilfetc.qc.
**
** History:
**	Revision 5.1  87/04  arthur
**	Initial revision.
**
**	Revision 6.0  87/07/17  jfried
**	Added calls to 'IITBceColEnd()' for new FRS support.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.4
**	3/91 (Mike S)
**		Add LOADTABLE
**	04/07/91 (emerson)
**		Modifications for local procedures(in IIOitInittableExec).
**
**	Revision 6.4/02
**	12/14/91 (emerson)
**		Part of fix for bug 38940 in IIOitInittableExec.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		added argument to all the IIOgvGetVal() calls.
**	05-feb-93 (davel)
**		Added support for cell attributes "with clause" 
**		to insertrow and loadtable functions (IIOirInsertrowExec() 
**		and IIITltfLoadTFExec() ).
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	08/22/93 (dkh) - Fixed typos in previous submission.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need runtime.h to satisfy gcc 4.3.
*/

/*{
** Name:	IIOcwClearrowExec	-	Execute an OSL 'clearrow' stmt
**
** Description:
**	Executes an OSL 'clearrow' statement.
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
VOID
IIOcwClearrowExec(stmt)
IL	*stmt;
{
	i4	rownum;
	IL	*next;

	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2), rowCURR,
				ERx("CLEARROW"));

	if (IItbsetio(cmCLEARR, IIOFfnFormname(),
		      IIITtcsToCStr(ILgetOperand(stmt, 1)),
		      rownum) != 0)
	{
	    IIITrcsResetCStr();
	    next = IIOgiGetIl(stmt);
	    if ((*next&ILM_MASK) == IL_ENDLIST)
	    {
		IItclrrow();
	    }
	    else
	    {
		if ((*next&ILM_MASK) != IL_TL1ELM)
		{
		    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("listelm"),
			       (char *) NULL);
		}
		else do
		{
		    IItclrcol(IIITtcsToCStr(ILgetOperand(next, 1)));
		    IIITrcsResetCStr();
		    next = IIOgiGetIl(next);
		} while ((*next&ILM_MASK) == IL_TL1ELM);
		
		if ((*next&ILM_MASK) != IL_ENDLIST)
		{
		    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
			       (char *) NULL);
		}
	    }
	    IITBceColEnd() ;
	}
	else
	{
	    IIITrcsResetCStr();
	    /*
	     ** FRS found an error.  Skip to end of series of IL statements
	     ** for the clearrow.
	     */
	    IIOslSkipIl(stmt, IL_ENDLIST);
	}
	
	return;
}

/*{
** Name:	IIOdrDeleterowExec	-	Execute an OSL 'deleterow' stmt
**
** Description:
**	Executes an OSL 'deleterow' statement.
**
** IL statements:
**	DELROW VALUE VALUE
**
**		The first VALUE names the table field.
**		The second VALUE, which may be NULL, is an integer row number.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOdrDeleterowExec(stmt)
IL	*stmt;
{
	i4	rownum;

	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2), rowCURR, 
				ERx("DELETEROW"));

	if (IItbsetio(cmDELETER, IIOFfnFormname(),
		IIITtcsToCStr(ILgetOperand(stmt, 1)),
		rownum) != 0)
	{
		IItdelrow(0);
	}
	IIITrcsResetCStr();

	return;
}

/*{
** Name:	IIOeuEndUnldExec	- 'endloop' from an 'unloadtable' loop
**
** Description:
**	Executes an 'endloop' from an 'unloadtable' loop.
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOeuEndUnldExec(stmt)
IL	*stmt;
{
	IItunend();
	return;
}

/*{
** Name:	IIOgrGetrowExec		-	Execute a getrow
**
** Description:
**	Executes a getrow.
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
** Inputs:
**	stmt	The IL statement to execute.
*
** History:
**	9/90 (Mike S) Use IIARgtcTcoGetIO (Bug 33374)
**
*/
IIOgrGetrowExec(stmt)
IL	*stmt;
{
	i4	rownum;
	char	*tblfld;
	IL	*next;

	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2), rowCURR,
				ERx("GETROW"));
	tblfld = (char *) IIOgvGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE, 
					(i4 *) NULL);

	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("getrow element"),
			(char *) NULL);
		return;
	}
	if (IItbsetio(cmGETR, IIOFfnFormname(), tblfld, rownum) != 0)
	{
	    while ((*next&ILM_MASK) == IL_TL2ELM)
	    {
		IIARgtcTcoGetIO(
			(char *) IIOgvGetVal(ILgetOperand(next, 2),
					DB_CHR_TYPE, (i4 *)NULL),
			IIOFdgDbdvGet(ILgetOperand(next, 1)));
		next = IIOgiGetIl(next);
	    }
	    if ((*next&ILM_MASK) != IL_ENDLIST)
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"), 
							(char *) NULL);
            IITBceColEnd() ;
	}

	return;
}

/*{
** Name:	IIOitInittableExec	-	Execute an OSL 'inittable' stmt
**
** Description:
**	Executes an OSL 'inittable' statement.
**
** IL statements:
**	INITTAB VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the INITTAB statement, the first VALUE contains the
**		name of the table field.  The second VALUE names the mode.
**		Each TL2ELM element (if any) contains the name and format
**		of a hidden column.
**		The list is closed by an ENDLIST statement.
**
**		Note:  The second VALUE may be NULL.  This implies 'fill' mode.
**		For frames compiled under 6.4/02 or later, this also implies
**		that the INITTAB is being done for frame initialization:
**		for a static frame, the tablefield should be initialized only
**		on the first call to the frame.
**		For frames compiled prior to 6.4/02, the second VALUE
**		may also be NULL for an INITTABLE statement that omits the mode;
**		we will treat this as if it were an INITTAB being done for
**		frame initialization, so such an INITTABLE statement
**		will be erroneously skipped on second and subsequent iterations
**		of a static frame.  I (emerson) deemed this acceptable because:
**
**		(1) It would be difficult to avoid the problem.
**		(2) It's documented in the INGRES/4GL Ref that mode is required
**		    on the INITTABLE statement.
**		(3) Recompiling the frame will correct the problem.
**		(4) Prior to 6.4/02, INITTABLE statements were erroneously
**		    skipped on second and subsequent iterations of a static
**		    frame whether or not mode was specified on the INITTABLE
**		    (bug 38940).
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		We now call IIOFiesIsExecutedStatic instead IIOFisIsStatic
**		(but that's only because the old IIOFisIsStatic was renamed
**		to IIOFiesIsExecutedStatic).
**	12/14/91 (emerson)
**		Part of fix for bug 38940:  When executing a static frame, don't
**		skip the tablefield initialization on second and subsequent
**		calls of the frame unless the second operand is 0.
*/
IIOitInittableExec(stmt)
IL	*stmt;
{
	char	*mode;
	IL	*next;

	mode = IIITtcsToCStr(ILgetOperand(stmt, 2));
	if (mode == NULL)
	{
		if (IIOFiesIsExecutedStatic())
		{
			return;
		}
		mode = ERx("f");
	}

	if ( IItbinit(IIOFfnFormname(), IIITtcsToCStr(ILgetOperand(stmt, 1)),
			mode) != 0 )
	{
		next = IIOgiGetIl(stmt);
		while ((*next&ILM_MASK) == IL_TL2ELM)
		{
			IIthidecol((char *) IIOgvGetVal(ILgetOperand(next, 1),
				DB_CHR_TYPE, (i4 *)NULL),
				(char *) IIOgvGetVal(ILgetOperand(next, 2),
				DB_CHR_TYPE, (i4 *)NULL));
			next = IIOgiGetIl(next);
		}
		if ((*next&ILM_MASK) != IL_ENDLIST)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
		}
		IItfill();
	}
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIITltfLoadTFExec	-	Execute an OSL 'LOADTABLE' stmt
**
** Description:
**	Executes an OSL 'LOADTABLE' statement.
**
** IL statements:
**	LOADTABLE VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**	INQELM VALUE VALUE VALUE INT-VALUE
**	  ...
**	ENDLIST
**
**		In the LOADTABLE statement, the first VALUE contains the
**		name of the table field.  The second VALUE is ignored
**		Each TL2ELM element (if any) contains the name of a column
**		and the value to be placed into it.
**		The list is closed by an ENDLIST statement.
**
**		If a with clause is specified, the additional INQELM list
**		will exist and will contain a list of columns and display
**		attributes to be set in the newly loaded row.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIITltfLoadTFExec(stmt)
register IL	*stmt;
{
    register IL	*next;
    i4  has_cellattr = *stmt & ILM_HAS_CELL_ATTR;

    if (IItbact(IIOFfnFormname(),
		    IIITtcsToCStr(ILgetOperand(stmt, 1)),
		    1) != 0)
    {
	IIITrcsResetCStr();
	next = IIOgiGetIl(stmt);
	while ((*next&ILM_MASK) == IL_TL2ELM)
	{
	    IItcoputio(IIITtcsToCStr(ILgetOperand(next, 1)),
			   (i2 *) NULL, 1, DB_DBV_TYPE, 0,
			   IIOFdgDbdvGet(ILgetOperand(next, 2)));
	    IIITrcsResetCStr();
	    next = IIOgiGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST)
	{
	    IIOerError(ILE_STMT, ILE_ILMISSING,
		ERx("endlist"), (char *) NULL);
	}
	if (has_cellattr)
	{
		next = IIOgiGetIl(next);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("inqelm"),
				(char *) NULL);
		}
		else do
		{
			IIITrcsResetCStr();
			IIFRsaSetAttrio((i4) ILgetOperand(next, 4),
				 IIITtcsToCStr(ILgetOperand(next, 2)),
				 (i2 *) NULL,
				 1,
				 DB_DBV_TYPE,
				 0,
				 IIOFdgDbdvGet(ILgetOperand(next, 1)));
			next = IIOgiGetIl(next);
		} while ((*next&ILM_MASK) == IL_INQELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
		}
	}
	IITBceColEnd() ;
    }
    else
    {
	IIITrcsResetCStr();
	IIOslSkipIl(stmt, IL_ENDLIST);
    }

    return;
}

/*{
** Name:	IIOirInsertrowExec	-	Execute an OSL 'insertrow' stmt
**
** Description:
**	Executes an OSL 'insertrow' statement.
**
** IL statements:
**	INSROW VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**	INQELM VALUE VALUE VALUE INT-VALUE
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
**		attributes to be set in the newly loaded row.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOirInsertrowExec(stmt)
register IL	*stmt;
{
    i4	rownum;
    i4  has_cellattr = *stmt & ILM_HAS_CELL_ATTR;

    rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2), rowCURR,
			ERx("INSERTROW"));

    if (IItbsetio(cmINSERTR, IIOFfnFormname(),
		    IIITtcsToCStr(ILgetOperand(stmt, 1)),
		    rownum) != 0)
    {
	IIITrcsResetCStr();
	if (IItinsert() != 0)
	{
	    register IL	*next;

	    next = IIOgiGetIl(stmt);
	    while ((*next&ILM_MASK) == IL_TL2ELM)
	    {
		IItcoputio(IIITtcsToCStr(ILgetOperand(next, 1)),
			   (i2 *) NULL, 1, DB_DBV_TYPE, 0,
			   IIOFdgDbdvGet(ILgetOperand(next, 2)));
		IIITrcsResetCStr();
		next = IIOgiGetIl(next);
	    }
	    if ((*next&ILM_MASK) != IL_ENDLIST)
	    {
		IIOerError(ILE_STMT, ILE_ILMISSING,
		    ERx("endlist"), (char *) NULL);
	    }

	    if (has_cellattr)
	    {
		next = IIOgiGetIl(next);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("inqelm"),
				(char *) NULL);
		}
		else do
		{
			IIITrcsResetCStr();
			IIFRsaSetAttrio((i4) ILgetOperand(next, 4),
				 IIITtcsToCStr(ILgetOperand(next, 2)),
				 (i2 *) NULL,
				 1,
				 DB_DBV_TYPE,
				 0,
				 IIOFdgDbdvGet(ILgetOperand(next, 1)));
			next = IIOgiGetIl(next);
		} while ((*next&ILM_MASK) == IL_INQELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
		}
	    }
            IITBceColEnd() ;
	}
    }
    else
    {
	IIITrcsResetCStr();
	IIOslSkipIl(stmt, IL_ENDLIST);
    }

    return;
}

/*{
** Name:	IIOprPutrowExec		-	Execute a putrow
**
** Description:
**	Executes a putrow.
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOprPutrowExec(stmt)
IL	*stmt;
{
	i4	rownum;
	IL	*next;

	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2), rowCURR,
				ERx("PUTROW"));

	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("putrow element"),
			(char *) NULL);
		return;
	}
	if (IItbsetio(cmPUTR, IIOFfnFormname(),
	    (char *)IIOgvGetVal(ILgetOperand(stmt, 1), 
					DB_CHR_TYPE, (i4 *)NULL),
	    rownum) != 0)
	{
	    while ((*next&ILM_MASK) == IL_TL2ELM)
	    {
		IItcoputio((char *) IIOgvGetVal(ILgetOperand(next, 1),
						  DB_CHR_TYPE, (i4 *)NULL),
			    (i2 *) NULL,
			    1,
			    DB_DBV_TYPE,
			    0,
			    IIOFdgDbdvGet(ILgetOperand(next, 2)));
		next = IIOgiGetIl(next);
	    }
	    if ((*next&ILM_MASK) != IL_ENDLIST)
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"), 
						(char *) NULL);
            IITBceColEnd() ;
	}

	return;
}

/*{
** Name:	IIOu1Unloadtbl1Exec	- Begin executing an 'unloadtable' loop
**
** Description:
**	Begin executing an 'unloadtable' loop.
**	This routine sets up the table field with its data set, and is
**	called only once for each execution of an unloadtable loop.
**	The routine IIOu2Unloadtbl2Exec() is called at the beginning of
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOu1Unloadtbl1Exec(stmt)
IL	*stmt;
{
	if (IItbact(IIOFfnFormname(),
		    IIITtcsToCStr(ILgetOperand(stmt, 1)), 0) ==0)
	{
		IIOsiSetIl(ILgetOperand(stmt, 2));
	}

	return;
}

/*{
** Name:	IIOu2Unloadtbl2Exec	- Iterate through an 'unloadtable' loop
**
** Description:
**	Iterates through an 'unloadtable' loop.
**	The routine IIOu1Unloadtbl1Exec() sets up the table field
**	with its data set.  This routine is called at the beginning of
**	each iteration through the unloadtable loop.
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOu2Unloadtbl2Exec(stmt)
IL	*stmt;
{
    
    if (IItunload() != 0)
    {
	register IL	*next;
	
	next = IIOgiGetIl(stmt);
	while ((*next&ILM_MASK) == IL_TL2ELM)
	{
	    IItcogetio((i2 *) NULL, 1, DB_DBV_TYPE, 0,
		       IIOFdgDbdvGet(ILgetOperand(next, 1)),
		       IIITtcsToCStr(ILgetOperand(next, 2)));
	    IIITrcsResetCStr();
	    next = IIOgiGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST)
	{
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
		       (char *) NULL);
	}
	IITBceColEnd() ;
    }
    else
    {
	IItunend();
	IIOsiSetIl(ILgetOperand(stmt, 1));
    }
    
    return;
}

/*{
** Name:	IIOvrValidrowExec	-	Execute an OSL 'validrow' stmt
**
** Description:
**	Executes an OSL 'validrow' statement.
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
**		The SID is where to go on success.
**		Each TL1ELM element (if any) contains the name of a column.
**		The list is closed by an ENDLIST statement.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOvrValidrowExec(stmt)
IL	*stmt;
{
	i4	rownum;

	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 2), rowCURR,
				ERx("VALIDROW"));

	if (IItbsetio(cmVALIDR, IIOFfnFormname(),
		IIITtcsToCStr(ILgetOperand(stmt, 1)), rownum) != 0)
	{
	    register IL	*next;

	    IIITrcsResetCStr();
	    next = IIOgiGetIl(stmt);
	    if ((*next&ILM_MASK) != IL_TL1ELM)
		IItvalrow();
	    else
	    {
		while ((*next&ILM_MASK) == IL_TL1ELM)
		{
		    IItcolval(IIITtcsToCStr(ILgetOperand(next, 1)));
		    IIITrcsResetCStr();
		    next = IIOgiGetIl(next);
		}
	    }
	    if ((*next&ILM_MASK) != IL_ENDLIST)
	    {
		    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
			    (char *) NULL);
	    }
	    /*
	    ** If a validation error, go back to top of display loop.
	    */
	    if (IItvalval(0))
		    IIITgfsGotoFromStmt(stmt, 3);
	}
	else
	{
		IIITrcsResetCStr();
		IIOslSkipIl(stmt, IL_ENDLIST);
	}

	return;
}


/*{
** Name:	IIITptPurgeTbl - Execute a 'purgetable' statement
**
** Description:
**	Execute the 'purgetable' statement.
**
**	IL statements:
**	    PURGETABLE VALUE VALUE
**
**		The first VALUE is the name of the form.
**		The second VALUE is the name of the table field.
**
** Inputs:
**	stmt	The IL statement to execute.
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
IIITptPurgeTbl(stmt)
IL	*stmt;
{
	(void) IItbsetio(cmPURGE, IIITtcsToCStr(ILgetOperand(stmt, 1)),
			IIITtcsToCStr(ILgetOperand(stmt, 2)), 0);
}
