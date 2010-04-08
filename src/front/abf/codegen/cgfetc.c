/*
** Copyright (c) 1987, 2008 Ingres Corporation
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
/* ILRF Block definition */
#include	<abfrts.h>
#include	<lo.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgil.h"
#include	"cgout.h"
#include	"cgerror.h"

#include	<fsicnsts.h>

/**
** Name:	cgfetc.c - Code Generator Miscelleneous Form Statements Module.
**
** Description:
**	Generates C code from the IL statements for miscellaneous form
**	statements.  Includes a different 'IICG*Gen()' routine for
**	each type of statement.  Other execution routines for OSL forms
**	statements can be found in "cgfdisp.c" and "cgftfld.c".
**	Defines:
**
**	IICGfopFrsOptGen()	FRS display option statement.
**	IICGscrollGen()		FRS table field scroll statement.
**	IICGhflHelpfileGen()	OSL HELPFILE statement.
**	IICGhfsHelpformsGen()	OSL HELP_FORMS statement.
**	IICGiqfInqFormsGen()	new-style OSL INQUIRE_FORMS statement.
**	IICGif3InqFrs30Gen()	old-style OSL INQUIRE_FORMS statement.
**	IICGmodeGen()		OSL MODE statement.
**	IICGiqInqINGgen()	OSL INQUIRE_INGRES statement.
**	IICGstINGsetGen()	OSL INQUIRE_INGRES statement.
**	IICGmsgMessageGen()	OSL MESSAGE statement.
**	IICGprsPrintscrGen()	OSL PRINTSCREEN statement.
**	IICGprmPromptGen()	OSL PROMPT statement.
**	IICGstfSetFormsGen()	OSL SET_FORMS statement.
**	IICGslpSleepGen()	OSL SLEEP statement.
**	IICGi4gInquire4GL()	OSL INQUIRE_4GL statement
**	IICGs4gSet4GL()		OSL SET_4GL statement
**	IICGslpSetRndGen()	OSL SET RANDOM_SEED statement.
**
** History:
**	Revision 6.0  87/04  arthur
**	Initial revision.
**
**	Revision 6.1  88/05  wong
**	Added 'IICGfopFrsOptGen()' and 'IICGscrollGen()'.
**
**	Revision 6.3  89/10  wong
**	Added SET_INGRES support.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	02/26/91 (emerson)
**		Remove 2nd argument to IICGgadGetAddr (cleanup:
**		part of fix for bug 36097).
**		Also call IICGgdaGetDataAddr instead of IICGgadGetAddr in
**		IICGfopFrsOptGen (this is the crux of the fix for bug 36097).
**
**	Revision 6.5
**	29-jun-92 (davel)
**		Minor change - added new argument to call to IIORcqConstQuery().
**	10-feb-93 (davel)
**		Added support for INQUIRE/SET_4GL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Add IICGslpSetRndGen to support code generation for setting
**	    random seed via IIset_random.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/*{
** Name:	IICGfopFrsOptGen() -	Generate Code for FRS Display Option.
**
** Description:
**	Generates C code for an FRS display option statement.
**
** IL Statements:
**	FRSOPT
**	TL3ELM INT1 INT2 VALUE
**	  ...
**	ENDLIST
**
**		In the TL3ELM statement, the first integer is the option ID and the
**		second is the DB_DT_ID type for the VALUE.  The VALUE is the FRS
**		option value, either a constant or a reference to a variable.
**	
** Code Generated:
**	IIFRgpcontrol(FSPS_OPEN, 0);
**	IIFRgpsetio(INT1, (i2 *) NULL, 1, INT2, sizeof(VALUE), VALUE);
**		...
**	IIFRgpcontrol(FSPS_CLOSE, 0);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/88 (jhw) -- Written.
**	08/90 (jhw) -- Generate correct length for 'IIFRgpsetio()' integers
**		and pass by address not value.  Bug #32771.
**	02/26/91 (emerson)
**		Call IICGgdaGetDataAddr instead of IICGgadGetAddr
**		(this is the crux of the fix for bug 36097).
*/
VOID
IICGfopFrsOptGen ( stmt )
IL	*stmt;
{
	register IL	*next;
	char		buf[CGSHORTBUF];

	/* IIFRgpcontrol( FSPS_OPEN, 0 ) */
	IICGosbStmtBeg();
		IICGocbCallBeg( ERx("IIFRgpcontrol") );
		CVna( FSPS_OPEN, buf );
		IICGocaCallArg(buf);
		IICGocaCallArg(_Zero);
		IICGoceCallEnd();
	IICGoseStmtEnd();

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL3ELM)
	{
		IICGerrError( CG_ILMISSING, ERx("FRSOPT(TL3ELM)"), (char *) NULL );
	}
	else do
	{
		IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIFRgpsetio"));
			CVna( ILgetOperand(next, 1), buf );
			IICGocaCallArg(buf);
			IICGocaCallArg(_ShrtNull);	/* null indicator */
			IICGocaCallArg(_One);		/* (TRUE) is variable */
			CVna( ILgetOperand(next, 2), buf );	/* datatype */
			IICGocaCallArg(buf);
			if ( ILgetOperand(next, 2) == DB_INT_TYPE )
			{
				ILREF	value = ILgetOperand(next, 3);

				/* Length */
				if ( ILISCONST( value ) )
					IICGocaCallArg( ERx("sizeof(i4)") );
				else
					IICGocaCallArg(
						STprintf( buf, "%d",
							IICGvllValLength(value)
						)
					);

				/* Value by address */
				IICGocaCallArg( ILNULLVAL(value)
					? _Zero
					: IICGgdaGetDataAddr(value, DB_INT_TYPE)
				);
			}
			else /* ILgetOperand(next, 2) == DB_CHR_TYPE, etc. */
			{
				IICGocaCallArg( _Zero );
				IICGstaStrArg( ILgetOperand(next, 3) );
			}
			IICGoceCallEnd();
		IICGoseStmtEnd();
	} while ( *(next = IICGgilGetIl(next)) == IL_TL3ELM );

	if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);

	/* IIFRgpcontrol( FSPS_CLOSE, 0 ) */
	IICGosbStmtBeg();
		IICGocbCallBeg( ERx("IIFRgpcontrol") );
		CVna( FSPS_CLOSE, buf );
		IICGocaCallArg(buf);
		IICGocaCallArg(_Zero);
		IICGoceCallEnd();
	IICGoseStmtEnd();

	IICGrcsResetCstr();
}

/*{
** Name:	IICGscrollGen() -	Generate Code for Table Field Scroll Statement.
**
** Description:
**	Generates C code for an FRS table field scroll statement.
**
** IL Statements:
**	SCROLL VAL1 VAL2 VAL3
**
**		The first VALUE is a string reference (constant or variable) to the
**		name of the form (this can be NULL.)  The second VALUE is a string
**		reference to the table field name.  The last VALUE is an integer
**		reference to the row.  The constants rowUP and rowDOWN refer
**		respectively to a SCROLL UP and a SCROLL DOWN and must be handled
**		specially.
**
** Code Generated:
**	if (IItbsetio(1,VAL1,VAL2,rowNONE) != 0)
**	{
**		IItbsmode("up" | "down" | "to");
**		if (IItscroll(0,rowNONE | VAL3) != 0)
**		{
**		}
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/88 (jhw) -- Written.
**	29-jun-92 (davel)
**		added new argument to call to IIORcqConstQuery().
*/
VOID
IICGscrollGen ( stmt )
IL	*stmt;
{
	char	buf[CGSHORTBUF];

	/* if (IItbsetio(1,VAL1,VAL2,rowNONE) != 0) { */
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IItbsetio"));
	IICGocaCallArg( _One );
	ILSTRARG( ILgetOperand(stmt, 1), _FormVar );
	IICGstaStrArg( ILgetOperand(stmt, 2) );
	CVna( rowNONE, buf );
	IICGocaCallArg(buf);
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	{
		static const char	_to[] = ERx("\"to\"");

		ILREF	ival = ILgetOperand(stmt, 3);
		char	*mode = _to;

		if ( !ILNULLVAL(ival) && ILISCONST(ival) )
		{
			PTR	cvp;

			_VOID_ IIORcqConstQuery( &IICGirbIrblk, -ival, 
					DB_INT_TYPE, &cvp, (i4 *)NULL );
			if ( *(i4 *)cvp == rowUP )
				mode = ERx("\"up\"");
			else if ( *(i4 *)cvp == rowDOWN )
				mode = ERx("\"down\"");
		}

		/* IItbsmode("up" | "down" | "to"); */
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IItbsmode"));
		IICGocaCallArg( mode );
		IICGoceCallEnd();
		IICGoseStmtEnd();
		/* if (IItscroll(0,rowNONE | VAL3) != 0) { */
		IICGoibIfBeg();
		IICGocbCallBeg(ERx("IItscroll"));
		IICGocaCallArg( _Zero );
		if ( mode == _to )
			IICGocaCallArg( ILINTVAL(ival, _Zero) );
		else
		{
			CVna( rowNONE, buf );
			IICGocaCallArg(buf);
		}
		IICGoceCallEnd();
		IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
		IICGoikIfBlock();
		IICGobeBlockEnd();
	}
	IICGobeBlockEnd();
	IICGrcsResetCstr();
}

/*{
** Name:	IICGhflHelpfileGen() -	Generate code for OSL 'helpfile' stmt
**
** Description:
**	Generates code for an OSL 'helpfile' statement.
**
** IL statements:
**	HLPFILE VALUE VALUE
**
**		The first VALUE is the subject name, the second the name
**		of the file.
**
** Code generated:
**	IIhelpfile(subject-string, filename-string);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	2-apr-87 (agh)
**		First written.
*/
VOID
IICGhflHelpfileGen (stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIhelpfile"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGstaStrArg(ILgetOperand(stmt, 2));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGhfsHelpformsGen() -	Generate code for OSL 'help_forms' stmt
**
** Description:
**	Generates code for an OSL 'help_forms' statement.
**
** IL statements:
**	HLPFORMS VALUE VALUE
**
**		The first VALUE is the subject name, the second the name
**		of the file.
**
** Code generated:
**	IIfrshelp(0, subject-string, filename-string);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGhfsHelpformsGen (stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIfrshelp"));
	IICGocaCallArg(_Zero);
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGstaStrArg(ILgetOperand(stmt, 2));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGiqfInqFormsGen() -	Generate Code for New-style
**						INQUIRE_FORMS Statement.
** Description:
**	Generates code for a new-style (4.0 and above) OSL
**	'inquire_forms' statement.
**
** IL statements:
**	INQFRS VALUE VALUE VALUE INT-VALUE
**	INQELM VALUE VALUE VALUE INT-VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...	]
**
**		In the INQFRS statement, the first and second VALUEs
**		are the names of any parents of the current object.
**		These 2 VALUEs may be NULL.  The third VALUE
**		is the row number (if any) of the parent.
**		The fourth and last operand is the integer code
**		used by EQUEL for the type of object being inquired on.
**		This appears as an actual integer.
**		In each INQELM statement, the first VALUE indicates the
**		variable to hold the return value; the second is the
**		name of the object being inquired on; and the third
**		is the row number.  The fourth and last operand is
**		the integer equivalent used by EQUEL for the
**		inquire_forms_constant specified by the user.
**		Following the ENDLIST statement, there must be a PUTFORM for
**		each named field which is displayed (as opposed to hidden).
**
** Code generated:
**	For the new-style OSL statement 'inquire_forms field ...':
**
**	if (IIiqset(type-of-object-as-integer, 0, form-name, (char *) NULL,
**		(char *) NULL) != 0)
**	{
**		IIiqfsio((i2 *) NULL, 1, 44, 0, &IIdbdvs[offset],
**			frs-constant-as-integer, field-name, object-code);
**		...
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGiqfInqFormsGen (stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIiqset"));
	CVna((i4) ILgetOperand(stmt, 4), buf);
	IICGocaCallArg(buf);
	IICGocaCallArg(ILINTVAL(ILgetOperand(stmt, 3), _Zero));
	ILSTRARG( ILgetOperand(stmt, 1), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 2), _NullPtr );
	IICGocaCallArg(_NullPtr);
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	{
		register IL	*next;

		next = IICGgilGetIl(stmt);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IICGerrError(CG_ILMISSING, ERx("inqelm"), (char *) NULL);
		}
		else do
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIiqfsio"));
			IICGocaCallArg(_ShrtNull);  /* null indicator */
			IICGocaCallArg(_One);
			CVna( DB_DBV_TYPE, buf );
			IICGocaCallArg(buf);
			IICGocaCallArg(_Zero);
			IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
			CVna((i4) ILgetOperand(next, 4), buf);
			IICGocaCallArg(buf);
			ILSTRARG( ILgetOperand(next, 2), ERx("(char *)NULL") );
			IICGocaCallArg( ILINTVAL(ILgetOperand(next, 3), _Zero) );
			IICGoceCallEnd();
			IICGoseStmtEnd();

			next = IICGgilGetIl(next);
		} while ((*next&ILM_MASK) == IL_INQELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
			IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	}
	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGif3InqFrs30Gen() -	Generate Code for Old-style
**						INQUIRE_FORMS Statement.
** Description:
**	Generates code for an old-style (3.0) OSL 'inquire_forms' statement.
**
** IL statements:
**	OINQFRS VALUE VALUE VALUE VALUE
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...	]
**
**		In the OINQFRS statement, the first, second and third
**		VALUEs are the names of any parents of the current
**		object.  These 3 VALUEs may be NULL.
**		The fourth VALUE is a string constant with the type of
**		object being inquired on.
**		In each TL2ELM statement, the first VALUE indicates the
**		variable to hold the return value; the second is the
**		inquire_forms_constant specified by the user.
**		Following the ENDLIST statement, there must be a PUTFORM for
**		each named field which is displayed (as opposed to hidden).
**
** Code generated:
**	For the old-style OSL statement 'inquire_forms field ...',
**	which takes 2 parent names as arguments (the names of the form
**	and field):
**
**	if (IIinqset("field", form-name, field-name, (char *) NULL,
**		(char *) NULL) != 0)
**	{
**		IIfsinqio((i2 *)NULL, 1, 44, 0, &IIdbdvs[offset],
**			frs-constant-as-string);
**		...
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGif3InqFrs30Gen(stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIinqset"));
	ILSTRARG( ILgetOperand(stmt, 4), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 1), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 2), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 3), _NullPtr );
	IICGocaCallArg(_NullPtr);
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	{
		register IL	*next;

		next = IICGgilGetIl(stmt);
		if ((*next&ILM_MASK) != IL_TL2ELM)
		{
			IICGerrError(CG_ILMISSING, ERx("tl2elm"), (char *) NULL);
		}
		else do
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIfsinqio"));
			IICGocaCallArg(_ShrtNull);  /* null indicator */
			IICGocaCallArg(_One);
			CVna( DB_DBV_TYPE, buf );
			IICGocaCallArg(buf);
			IICGocaCallArg(_Zero);
			IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
			IICGstaStrArg(ILgetOperand(next, 2));
			IICGoceCallEnd();
			IICGoseStmtEnd();

			next = IICGgilGetIl(next);
		} while ((*next&ILM_MASK) == IL_TL2ELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
			IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	}
	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGmodeGen() -	Generate Code for MODE Statement.
**
** Description:
**	Generates code for an OSL MODE statement.
**
** IL statements:
**	MODE VALUE
**
** Code generated:
**	IIset30mode(string-value);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	12/87 (jhw) -- Written.
*/
VOID
IICGmodeGen (stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIset30mode"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGoceCallEnd();
	IICGoseStmtEnd();

	IICGrcsResetCstr();
}

/*{
** Name:	IICGiqInqINGgen() -	Generate code for INQUIRE_INGRES statemt
**
** Description:
**	Generates code for an OSL INQUIRE_INGRES statement.
**
** IL statements:
**	INQING
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...	]
**
**		In each TL2ELM statement, the first VALUE contains the
**		variable to hold the return value, and the second an
**		inquire_ingres_constant.
**		Following the ENDLIST statement, there must be a PUTFORM for
**		each named field which is displayed (as opposed to hidden).
**
** Code generated:
**	IIeqiqio((i2 *) NULL, 1, 44, 0, &IIdbdvs[offset],
**		inq-constant-as-string);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGiqInqINGgen ( stmt )
register IL	*stmt;
{
	register IL	*next;
	char		buf[CGSHORTBUF];

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
		IICGerrError(CG_ILMISSING, ERx("listelm"), (char *) NULL);
	}
	else do
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIeqiqio"));
		IICGocaCallArg(_ShrtNull);  /* null indicator */
		IICGocaCallArg(_One);
		CVna( DB_DBV_TYPE, buf );
		IICGocaCallArg(buf);
		IICGocaCallArg(_Zero);
		IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
		IICGstaStrArg( ILgetOperand(next, 2) );
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	IICGrcsResetCstr();
}

/*{
** Name:	IICGstINGsetGen() -	Generate code for SET_INGRES Statement.
**
** Description:
**	Generates code for an OSL SET_INGRES statement.
**
** IL statements:
**	SETING
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**
**		In each TL2ELM statement, the first VALUE contains the
**		SET_INGRES constant name (a constant string) and the second
**		contains a reference to the DBV set value.
**
** Code generated:
**	IIeqstio( set-constant-as-string, (i2 *) NULL, 1, 44, 0,
**			&IIdbdvs[offset]);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	10/89 (jhw) -- Written.
*/
VOID
IICGstINGsetGen ( stmt )
IL	*stmt;
{
	register IL	*next;
	char		buf[CGSHORTBUF];

	next = IICGgilGetIl(stmt);
	if ( (*next&ILM_MASK) != IL_TL2ELM )
	{
		IICGerrError(CG_ILMISSING, ERx("listelm"), (char *) NULL);
	}
	else do
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIeqstio"));
		IICGstaStrArg( ILgetOperand(next, 1) );
		IICGocaCallArg(_ShrtNull);	/* Null indicator */
		IICGocaCallArg(_One);		/* TRUE (is var) */
		CVna( DB_DBV_TYPE, buf );	/* type */
		IICGocaCallArg(buf);
		IICGocaCallArg(_Zero);		/* length */
		IICGocaCallArg(
			IICGgadGetAddr(ILgetOperand(next, 2))
		);
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	} while ( (*next&ILM_MASK) == IL_TL2ELM );

	if ( (*next&ILM_MASK) != IL_ENDLIST )
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	IICGrcsResetCstr();
}

/*{
** Name:	IICGmsgMessageGen() -	OSL MESSAGE Statement.
**
** Description:
**	Generates code for a MESSAGE statement.
**
** IL statements:
**	MESSAGE VALUE
**
**		The VALUE contains the message.
**
** Code generated:
**	IImessage(string);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	04/87 (agh)  Written.
**	12/88 (jhw)  Changed to call 'IImessage()' instead of 'IISmessage()'.
**			The latter does `printf' style formatting where % is
**			a special character, and is not supported.
*/
VOID
IICGmsgMessageGen ( stmt )
IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IImessage"));
	IICGstaStrArg(ILgetOperand(stmt, 1));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGprsPrintscrGen() -	Generate code for an OSL 'printscreen' stmt
**
** Description:
**	Generates code for an OSL 'printscreen' statement.
**
** IL statements:
**	PRNTSCR VALUE
**
**		The VALUE is the name of the file, or the constant
**		"printer," or NULL.
**
** Code generated:
**
**	IIprnscr(filename-as-string);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGprsPrintscrGen(stmt)
register IL	*stmt;
{
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIprnscr"));
	ILSTRARG( ILgetOperand(stmt, 1), _NullPtr );
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGprmPromptExec() -	Generate code for an OSL 'prompt' stmt
**
** Description:
**	Generates code for an OSL 'prompt' or 'prompt noecho' statement.
**
** IL statements:
**	PROMPT VALUEa VALUE
**	[ PUTFORM VALUE VALUEa ]
**
**  or	NEPROMPT VALUEa VALUE
**
**		The first VALUE is the place to put the result, the
**		second the prompt string.
**		If the field in which the result is being placed is
**		a displayed field, the PROMPT or NEPROMPT statement is
**		followed by a PUTFORM statement.
**
** Code generated:
**	For a prompt statement without the 'noecho' option:
**
**	IIprmptio(0, prompt-string, (i2 *) NULL, 1, 44, 0, &IIdbdvs[offset]);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGprmPromptGen (stmt)
register IL	*stmt;
{
	char		buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIprmptio"));
	IICGocaCallArg(((*stmt&ILM_MASK) == IL_NEPROMPT) ? _One : _Zero);
	IICGstaStrArg(ILgetOperand(stmt, 2));
	IICGocaCallArg(_ShrtNull);  /* null indicator */
	IICGocaCallArg(_One);
	CVna( DB_DBV_TYPE, buf );
	IICGocaCallArg(buf);
	IICGocaCallArg(_Zero);
	IICGocaCallArg(IICGgadGetAddr(ILgetOperand(stmt, 1)));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGstfSetFormsGen() -	New-style 'set_forms' statement
**
** Description:
**	Generates code for a new-style (4.0 and beyond) OSL 'set_forms'
**	statement.
**
** IL statements:
**	SETFRS VALUE VALUE VALUE INT-VALUE
**	INQELM VALUEa VALUE VALUE INT-VALUE
**	  ...
**	ENDLIST
**
**		In the SETFRS statement, the first and second VALUEs
**		are the names of any parents of the current object.
**		These 2 VALUEs may be NULL.  The third VALUE
**		is the row number (if any) of the parent.
**		The fourth and last operand is the integer code
**		used by EQUEL for the type of object being set.
**		This appears as an actual integer.
**		In each INQELM statement, the first VALUE indicates the
**		variable holding the value to set to; the second is the
**		name of the object being set; and the third is the
**		row number (if any).  The fourth and last operand is
**		the integer equivalent used by EQUEL for the
**		set_forms_constant specified by the user.
**
** Code generated:
**	For the new-style OSL statement 'set_forms field ...',
**
**	if (IIiqset(type-of-object-as-integer, 0, form-name, (char *) NULL,
**		(char *) NULL) != 0)
**	{
**		IIstfsio(frs-constant-as-integer, field-name, row-value, (i2 *) NULL,
**			1, 44, 0, &IIdbdvs[offset]);
**		...
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGstfSetFormsGen (stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIiqset"));
	CVna((i4) ILgetOperand(stmt, 4), buf);
	IICGocaCallArg(buf);
	IICGocaCallArg(ILINTVAL(ILgetOperand(stmt, 3), _Zero));
	ILSTRARG( ILgetOperand(stmt, 1), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 2), _NullPtr );
	IICGocaCallArg(_NullPtr);
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	{
		register IL	*next;

		next = IICGgilGetIl(stmt);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IICGerrError(CG_ILMISSING, ERx("inqelm"), (char *) NULL);
		}
		else do
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIstfsio"));
			CVna((i4) ILgetOperand(next, 4), buf);
			IICGocaCallArg(buf);
			ILSTRARG( ILgetOperand(next, 2), _NullPtr );
			IICGocaCallArg( ILINTVAL(ILgetOperand(next, 3), _Zero) );
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
	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name:	IICGsf3SetFrs30Gen() -	Old-style 'set_forms' statement
**
** Description:
**	Generates code for an old-style (3.0) OSL 'set_forms' statement.
**
** IL statements:
**	OSETFRS VALUE VALUE VALUE VALUE
**	TL2ELM VALUE VALUE
**	  ...
**	ENDLIST
**
**		In the OSETFRS statement, the first, second and third
**		VALUEs are the names of any parents of the current
**		object.  These 3 VALUEs may be NULL.
**		The fourth VALUE is a string constant with the type of
**		object being set.
**		In each TL2ELM statement, the first VALUE indicates the
**		inquire_forms_constant specified by the user; the second
**		is the variable to be set.
**		An ENDLIST statement terminates the list.
**
** Code generated:
**	For the old-style OSL statement 'set_forms field ...',
**
**	if (IIinqset("field", form-name, field-name, (char *) NULL,
**		(char *) NULL) != 0)
**	{
**		IIfssetio(frs-constant-as-string, (i2 *) NULL, 1, 44, 0,
**			&IIdbdvs[offset]);
**		...
**	}
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
*/
VOID
IICGsf3SetFrs30Gen (stmt)
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIinqset"));
	ILSTRARG( ILgetOperand(stmt, 4), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 1), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 2), _NullPtr );
	ILSTRARG( ILgetOperand(stmt, 3), _NullPtr );
	IICGocaCallArg(_NullPtr);
	IICGoceCallEnd();
	IICGoicIfCond((char *) NULL, CGRO_NE, _Zero);
	IICGoikIfBlock();
	{
		register IL	*next;

		next = IICGgilGetIl(stmt);
		if ((*next&ILM_MASK) != IL_TL2ELM)
		{
			IICGerrError(CG_ILMISSING, ERx("tl2elm"), (char *) NULL);
		}
		else do
		{
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIfssetio"));
			IICGstaStrArg(ILgetOperand(next, 1));
			IICGocaCallArg(_ShrtNull);  /* null indicator */
			IICGocaCallArg(_One);
			CVna( DB_DBV_TYPE, buf );
			IICGocaCallArg(buf);
			IICGocaCallArg(_Zero);
			IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 2)));
			IICGoceCallEnd();
			IICGoseStmtEnd();

			next = IICGgilGetIl(next);
		} while ((*next&ILM_MASK) == IL_TL2ELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
			IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	}
	IICGobeBlockEnd();
	IICGrcsResetCstr();
	return;
}

/*{
** Name: IICGslpSleepGen	-	Generate code for OSL 'sleep' statement
**
** Description:
**	Generate code for an OSL 'sleep' statement.
**
** IL statements:
**	SLEEP VALUE
**
**		The VALUE contains the number of seconds to 'sleep'.
**
** Code generated:
**	IIsleep(integer-value);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
*/
VOID
IICGslpSleepGen (stmt)
IL	*stmt;
{
	char	buf[CGBUFSIZE];

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIsleep"));
	IICGocaCallArg( STprintf( buf, ERx("(i4)%s"),
				IICGgvlGetVal(ILgetOperand(stmt, 1), DB_INT_TYPE)
			)
	);
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*{
** Name:	IICGi4gInquire4GL() -	Generate code for INQUIRE_4GL statement
**
** Description:
**	Generates code for an OSL inquire_4gl statement.
**
** IL statements:
**	INQ4GL
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...	]
**
**		In each TL2ELM statement, the first VALUE contains the
**		variable to hold the return value, and the second an
**		inquire_4gl constant name.
**		Following the ENDLIST statement, there must be a PUTFORM for
**		each named field which is displayed (as opposed to hidden).
**
** Code generated:
**	IIARi4gInquire4GL(inq-constant-as-string, &IIdbdvs[offset]);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	10-feb-93 (davel)
**		First written.
*/
VOID
IICGi4gInquire4GL ( stmt )
register IL	*stmt;
{
	register IL	*next;

	next = IICGgilGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	{
		IICGerrError(CG_ILMISSING, ERx("listelm"), (char *) NULL);
	}
	else do
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARi4gInquire4GL"));
		IICGstaStrArg( ILgetOperand(next, 2) );
		IICGocaCallArg(IICGgadGetAddr(ILgetOperand(next, 1)));
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	IICGrcsResetCstr();
}

/*{
** Name:	IICGs4gSet4GL() -	Generate code for SET_4GL Statement.
**
** Description:
**	Generates code for an OSL set_4gl statement.
**
** IL statements:
**	SET4GL
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**
**		In each TL2ELM statement, the first VALUE contains the
**		SET_4GL constant name (a constant string) and the second
**		contains a reference to the DBV set value.
**
** Code generated:
**	IIARs4gSet4GL( set-constant-as-string, &IIdbdvs[offset]);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	10-feb-93 (davel)
**		First written.
*/
VOID
IICGs4gSet4GL ( stmt )
IL	*stmt;
{
	register IL	*next;

	next = IICGgilGetIl(stmt);
	if ( (*next&ILM_MASK) != IL_TL2ELM )
	{
		IICGerrError(CG_ILMISSING, ERx("listelm"), (char *) NULL);
	}
	else do
	{
		IICGosbStmtBeg();
		IICGocbCallBeg(ERx("IIARs4gSet4GL"));
		IICGstaStrArg( ILgetOperand(next, 1) );
		IICGocaCallArg( IICGgadGetAddr(ILgetOperand(next, 2)) );
		IICGoceCallEnd();
		IICGoseStmtEnd();

		next = IICGgilGetIl(next);
	} while ( (*next&ILM_MASK) == IL_TL2ELM );

	if ( (*next&ILM_MASK) != IL_ENDLIST )
		IICGerrError(CG_ILMISSING, ERx("endlist"), (char *) NULL);
	IICGrcsResetCstr();
}

/*{
** Name: IICGslpSetRndGen	-	Generate code for OSL 'set random_seed' statement
**
** Description:
**	Generate code for an OSL 'set random_seed' statement.
**
** IL statements:
**	[ SEED VALUE ]
**
**		The VALUE contains the number of seed to 'sow'.
**
** Code generated:
**	IIset_random(integer-value);
**
** Inputs:
**	stmt	The IL statement for which to generate code.
**
** History:
**	29-Jan-2007 (kiria01)
**		First written.
*/
VOID
IICGslpSetRndGen (stmt)
IL	*stmt;
{
	char	buf[CGBUFSIZE];
	
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIset_random"));
	IICGocaCallArg( STprintf( buf, ERx("(i4)%s"),
	                          IICGgvlGetVal(ILgetOperand(stmt, 1), DB_INT_TYPE)
	                        )
	              );
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}
