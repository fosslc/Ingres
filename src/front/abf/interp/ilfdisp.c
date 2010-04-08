/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<si.h>
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
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	"itcstr.h"

/**
** Name:	ilfdisp.c -	Execute IL statements for form display stmts
**
** Description:
**	Executes IL statements for OSL form display statements.
**	Includes a different IIOstmtExec() routine for each type of
**	statement.
**	Other execution routines for forms statements are in
**	ilftfld.qc and ilfetc.qc.
**
** History:
**	Revision 5.1  87/04  arthur
**	Initial revision.
**
**	Revision 6.2  88/10/25  joe
**	Updated to 6.2.
**
**	Revision 6.3/03  89/07/17  jfried
**	Added support for RESUME ENTRY.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		added argument to the IIOgvGetVal() calls.
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**	08/22/93 (dkh) - Fixed typos in previous submission.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	IIOcaClrallExec	-	Execute OSL 'clear field all' statement
**
** Description:
**	Execute OSL 'clear field all' statement.
**
** IL statements:
**	CLRALL
**
**	The CLRALL statement takes no arguments.  It is exactly equivalent
**	to an OSL 'clear field all'.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
VOID
IIOcaClrallExec(stmt)
IL	*stmt;
{
	IIclrflds();
}

/*{
** Name:	IIOclClrfldExec	-	Execute OSL 'clear field' statement
**
** Description:
**	Execute OSL 'clear field' statement.
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
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	25-oct-1988 (Joe)
**		Upgraded to 6.0.  Convert value to C string.
*/
IIOclClrfldExec(stmt)
IL	*stmt;
{
	IIfldclear(IIITtcsToCStr(ILgetOperand(stmt, 1)));
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIOcsClrscrExec	-	Execute OSL 'clear screen' statement
**
** Description:
**	Execute OSL 'clear screen' statement.
**
** IL statements:
**	CLRSCR
**
**	The CLRSCR statement takes no arguments.  It is exactly equivalent
**	to an OSL 'clear screen'.
**
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOcsClrscrExec(stmt)
IL	*stmt;
{
	IIclrscr();
	return;
}

/*{
** Name:	IIOgfGetformExec	-	Execute a getform
**
** Description:
**	Executes a getform.
**
** IL statements:
**	GETFORM strFieldName valPlaceToPutValue
**
**	The first VALUE names the field on which to do a 'getform';
**	the second is the data area for that field within the interpreter.
**
** Inputs:
**	stmt	The IL statement to execute.
**
**	History:
**		9/90 (Mike S) Use IIARgfGetFldIO (Bug 33374)
*/
IIOgfGetformExec(stmt)
IL	*stmt;
{
	if (IIfsetio(IIOFfnFormname()) == 0)
		return (FAIL);		/* ERROR */

	IIARgfGetFldIO((char *)IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL), 
		       IIOFdgDbdvGet((i4) ILgetOperand(stmt, 2)));
	return (OK);
}

/*{
** Name:	IIOpfPutformExec	-	Execute a putform
**
** Description:
**	Execute a putform.
**
** IL statements:
**	PUTFORM strFieldName valPlaceToGetValue
**
**		The first VALUE names the field on which to do a 'putform'.
**		The second VALUE is the data area for that field
**		within the interpreter.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOpfPutformExec(stmt)
IL	*stmt;
{
	if (IIfsetio(IIOFfnFormname()) == 0)
		return (FAIL);		/* ERROR */
	IIputfldio(IIOgvGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE, (i4 *)NULL),
		   (i2 *) NULL, 1, DB_DBV_TYPE, 0,
		   IIOFdgDbdvGet((i4) ILgetOperand(stmt, 2)));
	return (OK);
}

/*{
** Name:	IIOryRedisplayExec	-	Execute OSL 'redisplay' stmt
**
** Description:
**	Executes OSL 'redisplay' statement.
**
** IL statements:
**	REDISPLAY
**
**		The REDISPLAY statement takes no operands.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOryRedisplayExec(stmt)
IL	*stmt;
{
	IIredisp();
	return;
}

/*{
** Name:	IIOruResumeExec	-	Execute simple OSL 'resume' statement
**
** Description:
**	Execute simple OSL 'resume' statement.
**
** IL statements:
**	RESUME
**	GOTO SID
**
**		The RESUME statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOruResumeExec(stmt)
IL	*stmt;
{
	/*
	** No action needed here, since this IL statement is followed
	** by a 'GOTO'.
	*/
	return;
}

/*{
** Name:	IIOrcResColumnExec	-	Execute 'resume column' stmt
**
** Description:
**	Execute the OSL equivalent of a 'resume column' statement.
**	The routine IIOrdResFieldExec() handles 'resume' statements that
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOrcResColumnExec(stmt)
IL	*stmt;
{
	IIrescol(IIITtcsToCStr(ILgetOperand(stmt, 1)),
		 IIITtcsToCStr(ILgetOperand(stmt, 2)));
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIOrdResFieldExec	-	Execute OSL 'resume field' stmt
**
** Description:
**	Execute the OSL 'resume field' statement when it refers to a
**	simple field on the form.
**	The routine IIOrcResColumnExec() handles 'resume' statements that
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOrdResFieldExec(stmt)
IL	*stmt;
{
	IIresfld(IIITtcsToCStr(ILgetOperand(stmt, 1)));
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIOrmResMenuExec	-	Execute OSL 'resume menu' stmt
**
** Description:
**	Execute OSL 'resume menu' statement.
**
** IL statements:
**	RESMENU
**	GOTO SID
**
**		The RESMENU statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOrmResMenuExec(stmt)
IL	*stmt;
{
	IIresmu();
	return;
}

/*{
** Name:	IIOrnResNextExec	-	Execute OSL 'resume next' stmt
**
** Description:
**	Execute OSL 'resume next' statement.
**
** IL statements:
**	RESNEXT
**	GOTO SID
**
**		The RESNEXT statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOrnResNextExec(stmt)
IL	*stmt;
{
	IIresnext();
	return;
}

/*{
** Name:	IIITreResEntryExec	-	Execute OSL 'resume entry' stmt
**
** Description:
**	Execute OSL 'resume entry' statement.
**
** IL statements:
**	RESENTRY
**	GOTO SID
**
**		The RESENTRY statement is always followed by a GOTO statement,
**		which returns control to the top of the frame's display loop.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIITreResEntryExec(stmt)
IL	*stmt;
{
	IIFRreResEntry();
	return;
}

/*{
** Name:	IIOvaValidateExec	-	Execute an OSL 'validate' stmt
**
** Description:
**	Executes a simple OSL 'validate' statement.
**	A 'validate field' statement is handled by IIOvfValFieldExec().
**
** IL statements:
**
**	VALIDATE sidForSuccess
**
**		The SID indicates the top of the display loop:  control
**		returns there if a validation check fails.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOvaValidateExec(stmt)
IL	*stmt;
{
	/*
	** The value from IIchkfrm is a i4, but is really a
	** boolean TRUE or FALSE.
	*/
	if (IIchkfrm())
		IIOsiSetIl(ILgetOperand(stmt, 1));
	return;
}

/*{
** Name:	IIOvfValFieldExec	-	Execute a 'validate field' stmt
**
** Description:
**	Executes an OSL 'validate field' statement.
**	A simple 'validate' statement is handled by IIOvaValidateExec().
**
** IL statements:
**
**	VALFLD VALUE SID
**
**		The VALUE indicates the name of the field.
**		The SID indicates the top of the display loop:  control
**		returns there if the validation check fails.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOvfValFieldExec(stmt)
IL	*stmt;
{
	if (IIvalfld(IIITtcsToCStr(ILgetOperand(stmt, 1))))
		IIOsiSetIl(ILgetOperand(stmt, 2));
	IIITrcsResetCStr();
	return;
}


/*{
** Name:	IIITrnfResNxtFld - Execute a 'resume nextfield' stmt
**
** Description:
**	Execute the 'resume nextfield' statment.
**
**	IL statement:
**		RESNFLD
**		GOTO SID
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
IIITrnfResNxtFld(stmt)
IL	*stmt;
{
	IIFRgotofld(0);
}



/*{
** Name:	IIITrpfResPrvFld - Execute a 'resume previousfield' stmt
**
** Description:
**	Execute the 'resume previousfield' statment.
**
**	IL statement:
**		RESPFLD
**		GOTO SID
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
IIITrpfResPrvFld(stmt)
IL	*stmt;
{
	IIFRgotofld(1);
}
