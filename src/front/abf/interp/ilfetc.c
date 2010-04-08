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
#include	<afe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<eqrun.h>
#include	<fdesc.h>
#include	<fsicnsts.h>
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
# include	"itcstr.h"
#include	<erit.h>

/**
** Name:	ilfetc.c -	IL Interpreter Miscellaneous Forms Statements
**					Execution Routines.
** Description:
**	Contains routines that execute IL statements for miscellaneous OSL
**	forms statements, each represented by a 'IIOstmtExec()' routine.  (Other
**	execution routines OSL for forms statements are in "ilfdisp.c" and
**	and "ilftfld.c".)  This file defines:
**
**	IIOhfHelpfileExec()	execute OSL 'helpfile' statement.
**	IIOhsHelpformsExec()	execute OSL 'help_forms' stmt.
**	IIOifInqFormsExec()	execute OSL 'inquire_forms' stmt.
**	IIITiqInqINGex()	execute OSL INQUIRE_INGRES statement.
**	IIITstINGsetEx()	execute OSL SET_INGRES statement.
**	IIOmeMessageExec()	execute OSL MESSAGE statement.
**	IIOpnPrintScrExec()	execute OSL 'printscreen' stmt.
**	IIOpmPromptExec	()	execute OSL 'prompt' stmt.
**	IIOsfSetFormsExec()	execute OSL 'set_forms' statement.
**	IIOspSleepExec()	execute OSL 'sleep' statement.
**	IIITi4gInquire4GL()	execute OSL INQUIRE_4GL statement.
**	IIITs4gSet4GL()		execute OSL SET_4GL statement.
**	IIOspSetRndExec()	execute OSL 'set random' statement.
**
** History:
**	Revision 5.1  87/04/09  15:37:54  arthur
**		Added includes of ade.h and frmalign.h.
**		Changed (INGTYPE *) to (i4 *).
**	86/12/17  10:32:35  wong
**		Corrected to pass NULL string instead of empty string
**		to 'IIiqset()' for `SetForms' and 'InquireForms'.
**		86  arthur  Initial revision.
**
**	Revision 6.2  88/12/14  09:01:11  joe
**		Updated the interpreter to 6.2. Major changes in most files.
** 
**	Revision 6.3/03/00  89/10  wong
**		Added SET_INGRES support.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.5
**
**	10-feb-93 (davel)
**		Added SET_4GL and INQUIRE_4GL support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2007 (kiria01) b117277
**	    Added execution function IIOspSetRndExec to can IIset_random
**	    to support SET RANDOM_SEED
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN bool IIfrshelp();
FUNC_EXTERN bool IIhelpfile();

/*{
** Name:	IIITfoeFrsOptExec	- Execute the FRS options for Withs.
**
** Description:
**	Implement the FRSOPT IL statement.  This statement is generated
**	when a forms statement uses a WITH clause.
**	<comments>
**
** IL statements:
**	IL_FRSOPT
**	IL_TL3ELM	intFRSParamId	intTypeOfValue	valToSetParam
**	...
**	IL_ENDLIST
** Inputs:
**	stmt		A pointer to the FRSOPT statement.
**
** Side Effects:
**	Calls the FRS IIFRgpcontrol and IIFRgpsetio routines.
**
** History:
**	27-oct-1988 (Joe)
**		Written based on code generator.
**	08/90 (jhw) -- Corrected to catch Null values and pass correct (non-
**		Nullable) data length to 'IIFRgpsetio()'.  Bug #32771.
*/
VOID
IIITfoeFrsOptExec(stmt)
IL	*stmt;
{
    IIFRgpcontrol(FSPS_OPEN, 0);
    stmt = IIOgiGetIl(stmt);
    while ((*stmt&ILM_MASK) == IL_TL3ELM)
    {
	DB_DATA_VALUE	*dbv;
	DB_DATA_VALUE	forconst;
	i4		type;
	IL		data_operand;

	type = (i4) ILgetOperand(stmt, 2);
	if (ILisConst((data_operand = ILgetOperand(stmt, 3))))
	{
		dbv = &forconst;
		_VOID_ IIITctdConstToDbv(data_operand, (DB_DT_ID) type, dbv);
	}
	else
	{
		dbv = IIOFdgDbdvGet(data_operand);
		if ( AFE_NULLABLE(dbv->db_datatype) )
		{ /* get (non-Nullable) data length */
			if ( type == DB_INT_TYPE && AFE_ISNULL(dbv) )
				IIUGerr(E_IT001F_NULLINT, 0, 1, ERx("POPUP"));

			STRUCT_ASSIGN_MACRO(*dbv, forconst);
			dbv = &forconst;
			AFE_UNNULL(dbv);
		}
	}
	IIFRgpsetio(/* Param ID */  (i4) ILgetOperand(stmt, 1),
		    /* NULL IND */     NULL,
		    /* data is ptr */  1,
		    /* type of data */ type,
		    /* size of data */ dbv->db_length,
		    /* data ptr */     dbv->db_data
	);
	stmt = IIOgiGetIl(stmt);
    }
    if ((*stmt&ILM_MASK) != IL_ENDLIST)
    {
	IIOerError(ILE_STMT, ILE_ILMISSING, ERx("IIITfoeFrsOptExec"),
			(char *) NULL);
    }
    IIFRgpcontrol(FSPS_CLOSE, 0);
    return;
}

/*{
** Name:	IIITsteScrollTableExec  - Execute a scroll statement
**
** Description:
**	Executes a scroll statement.
**
** IL Statements:
**
**	IL_SCROLL	valFormName* valTableFieldToScroll valRowToScrollTo
**
** Inputs:
**	stmt		The IL_SCROLL statement.
**
** History:
**	21-nov-1988 (Joe)
**		First Written
*/
IIITsteScrollTableExec(stmt)
IL	*stmt;
{
    char	*form;

    form = IIITtcsToCStr(ILgetOperand(stmt, 1));
    if (form == NULL)
	form = IIOFfnFormname();
    if (IItbsetio(1, form, IIITtcsToCStr(ILgetOperand(stmt, 2)), rowNONE) != 0)
    {
	char	*mode = ERx("to");
	IL	valRowToScrollTo;
	i4	rownum;

	valRowToScrollTo = ILgetOperand(stmt, 3);
	rownum = (i4) IIITvtnValToNat(valRowToScrollTo, 0, ERx("SCROLL"));
	if (ILisConst(valRowToScrollTo))
	{
	    if (rownum == rowUP)
	    {
		mode = ERx("up");
		rownum = rowNONE;
	    }
	    else if (rownum == rowDOWN)
	    {
		mode = ERx("down");
		rownum = rowNONE;
	    }
	}
	IItbsmode(mode);
	_VOID_ IItscroll(0, rownum);
    }
    IIITrcsResetCStr();
    return;
}

/*{
** Name:	IIOhfHelpfileExec() -	Execute OSL 'helpfile' statement
**
** Description:
**	Execute an OSL 'helpfile' statement.
**
** IL statements:
**	HLPFILE VALUE VALUE
**
**		The first VALUE is the subject name, the second the name
**		of the file.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOhfHelpfileExec(stmt)
IL	*stmt;
{
	_VOID_ IIhelpfile(IIITtcsToCStr(ILgetOperand(stmt, 1)),
		          IIITtcsToCStr(ILgetOperand(stmt, 2)));
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIOhsHelpformsExec() -	Execute OSL 'help_forms' stmt
**
** Description:
**	Execute an OSL 'help_forms' statement.
**
** IL statements:
**	HLPFORMS VALUE VALUE
**
**		The first VALUE is the subject name, the second the name
**		of the file.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOhsHelpformsExec(stmt)
IL	*stmt;
{
	_VOID_ IIfrshelp(0, IIITtcsToCStr(ILgetOperand(stmt, 1)),
			    IIITtcsToCStr(ILgetOperand(stmt, 2)));
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIOifInqFormsExec() -	Execute an 'inquire_forms' stmt
**
** Description:
**	Executes an OSL 'inquire_forms' statement.
**
** IL statements:
**	INQFRS VALUE VALUE VALUE INT-VALUE
**	INQELM VALUEa VALUE VALUE INT-VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...  ]
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOifInqFormsExec(stmt)
register IL	*stmt;
{
	i4	rownum;
	char	*parent1;
	char	*parent2;

	parent1 = IIITtcsToCStr(ILgetOperand(stmt, 1));
	parent2 = IIITtcsToCStr(ILgetOperand(stmt, 2));
	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 3), 0,
			ERx("INQUIRE_FORMS"));

	if (IIiqset((i4) ILgetOperand(stmt, 4), rownum, parent1, parent2,
		NULL) != 0)
	{
		register IL	*next;

		next = IIOgiGetIl(stmt);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("inqelm"),
				(char *) NULL);
		}
		else do
		{
		    IIITrcsResetCStr();
		    IIiqfsio((i2 *) NULL, 1, DB_DBV_TYPE, 0,
			     IIOFdgDbdvGet(ILgetOperand(next, 1)),
			     (i4) ILgetOperand(next, 4),
			     IIITtcsToCStr(ILgetOperand(next, 2)),
			     (i4) IIITvtnValToNat(ILgetOperand(next, 3), 0,
						  ERx("INQUIRE_FORMS")));
		    next = IIOgiGetIl(next);
		} while ((*next&ILM_MASK) == IL_INQELM);

		if ((*next&ILM_MASK) != IL_ENDLIST)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
		}
	}
	IIITrcsResetCStr();

	return;
}

/*{
** Name:	IIITiqInqINGex() -	Execute an 'inquire_ingres' stmt
**
** Description:
**	Executes an OSL 'inquire_ingres' statement.
**
** IL statements:
**	INQING
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...  ]
**
**		In each TL2ELM statement, the first VALUE contains the
**		variable to hold the return value, and the second an
**		inquire_ingres_constant.
**		Following the ENDLIST statement, there must be a PUTFORM for
**		each named field which is displayed (as opposed to hidden).
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
VOID
IIITiqInqINGex ( stmt )
register IL	*stmt;
{
	register IL	*next;

	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("listelm"), (char *) NULL);
	else do
	{
	    IIeqiqio(NULL, 1, DB_DBV_TYPE, 0,
		     IIOFdgDbdvGet(ILgetOperand(next, 1)),
		     IIITtcsToCStr(ILgetOperand(next, 2)));
	    next = IIOgiGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"), (char *) NULL);

	return;
}

/*{
** Name:	IIITstINGsetEx() -	Execute an SET_INGRES Statement.
**
** Description:
**	Executes an OSL SET_INGRES statement.
**
** IL statements:
**	SETING
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**
**		In each TL2ELM statement, the first VALUE contains the
**		SET_INGRES constant name (a constant string) and the second
**		contains a reference to the DBV with the set value.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	10/89 (jhw) -- Written.
*/
VOID
IIITstINGsetEx ( stmt )
IL	*stmt;
{
	register IL	*next;

	next = IIOgiGetIl(stmt);
	if ( (*next&ILM_MASK) != IL_TL2ELM )
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("listelm"),(char*)NULL);
	else do
	{
		IIeqstio( IIITtcsToCStr(ILgetOperand(next, 1)),
			(i2 *)NULL, TRUE, DB_DBV_TYPE, 0,
			IIOFdgDbdvGet(ILgetOperand(next, 2))
		);
		next = IIOgiGetIl(next);
	} while ( (*next&ILM_MASK) == IL_TL2ELM );

	if ( (*next&ILM_MASK) != IL_ENDLIST )
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),(char*)NULL);

}

/*{
** Name:	IIITis3InqSet30 - Execute an old style (3.0) inquire/set
**
** Description:
**	This executes an old style inquire_forms or set_forms.  These two
**	statements are combined in one routine since their execution is
**	so much alike, and the IL for them is so much alike.
**
**      In release 3.0 and earlier, inquire_forms and set_forms did not
**	allow multiple dependent objects to be inquired on in one statement.
**	For example, only one field could be inquired on at a time.  These
**	old style statements are supported for backward compatibility.
**
** IL Statements:
**
**    IL_OINQFRS valParentName1* valParentName2 valParentName3 valTypeOfObject
**    {
**	  IL_TL2ELM valPlaceToPutValue valInquireConstant
**    }
**
**    IL_OSETFRS valParentName1* valParentName2 valParentName3 valTypeOfObject
**    {
**	  IL_TL2ELM valInquireConstant valValueToSetConstant
**    }
**
** Inputs:
**	stmt		The IL_OINQFRS or IL_OSETFRS statement.
**
** History:
**	22-nov-1988 (Joe)
**		First Written
*/
IIITis3InqSet30(stmt)
IL	*stmt;
{
    bool	inquire;

    inquire = (*stmt&ILM_MASK) == IL_OINQFRS;
    if (IIinqset(IIITtcsToCStr(ILgetOperand(stmt, 4)),
		 IIITtcsToCStr(ILgetOperand(stmt, 1)),
		 IIITtcsToCStr(ILgetOperand(stmt, 2)),
		 IIITtcsToCStr(ILgetOperand(stmt, 3)),
		 NULL) != 0)
    {
	stmt = IIOgiGetIl(stmt);
	if ((*stmt&ILM_MASK) != IL_TL2ELM)
	{
	    IIOerError(ILE_STMT, ILE_ILMISSING,
		       ERx("Missing list element in IIITis3InqSet30"),
		       (char *) NULL);
	}
	else
	{
	    do
	    {
		IIITrcsResetCStr();
		if (inquire)
		{
		    IIfsinqio(NULL, 1, DB_DBV_TYPE, 0,
			  IIOFdgDbdvGet(ILgetOperand(stmt, 1)),
			  IIITtcsToCStr(ILgetOperand(stmt, 2)));
		}
		else
		{
		    IIfssetio(IIITtcsToCStr(ILgetOperand(stmt, 1)),
			      NULL, 1, DB_DBV_TYPE, 0,
			      IIOFdgDbdvGet(ILgetOperand(stmt, 2)));
		}
		stmt = IIOgiGetIl(stmt);
	    } while ((*stmt&ILM_MASK) == IL_TL2ELM);
	    if ((*stmt&ILM_MASK) != IL_ENDLIST)
	    {
		IIOerError(ILE_STMT, ILE_ILMISSING,
			   ERx("Missing end list in IIITis3InqSet30"),
			   (char *) NULL);
	    }
	}
    }
    IIITrcsResetCStr();
    return;
}

/*{
** Name:	IIITmseModeStmtExec	-	Execute the mode statement.
**
** Description:
**	This executes the 4GL mode statement by calling the IIset30mode.
**
**
** IL statements:
**	
**	IL_MODE	valModeToSetForm
**
** Inputs:
**	stmt	The IL_MODE statement.
**
** History:
**	22-nov-1988 (Joe)
*/
IIITmseModeStmtExec(stmt)
IL	*stmt;
{
    IIset30mode(IIITtcsToCStr(ILgetOperand(stmt, 1)));
    IIITrcsResetCStr();
    return;
}

/*{
** Name:	IIOmeMessageExec() -	Execute OSL MESSAGE Statement.
**
** Description:
**	Execute OSL MESSAGE statement.
**
** IL statements:
**	MESSAGE VALUE
**
**		The VALUE contains the message.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	06/89 (jhw) -- Should call 'IImessage()' not 'IISmessage()'.
*/
IIOmeMessageExec(stmt)
IL	*stmt;
{
	IImessage(IIITtcsToCStr(ILgetOperand(stmt, 1)));
	IIITrcsResetCStr();
	return;
}

/*{
** Name:	IIOpnPrintScrExec() -	Execute OSL 'printscreen' stmt
**
** Description:
**	Execute an OSL 'printscreen' statement.
**
** IL statements:
**	PRNTSCR VALUE
**
**		The VALUE is the name of the file, or the constant
**		"printer," or NULL.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOpnPrintScrExec(stmt)
IL	*stmt;
{
    IIprnscr(IIITtcsToCStr(ILgetOperand(stmt, 1)));
    IIITrcsResetCStr();
    return;
}

/*{
** Name:	IIOpmPromptExec	() -	Execute OSL 'prompt' stmt
**
** Description:
**	Execute an OSL 'prompt' or 'prompt noecho' statement.
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
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOpmPromptExec(stmt)
IL	*stmt;
{
	IIprmptio(((*stmt&ILM_MASK) == IL_NEPROMPT ? 1 : 0),
		  IIITtcsToCStr(ILgetOperand(stmt, 2)),
		  NULL,
		  1,
		  DB_DBV_TYPE,
		  0,
		  IIOFdgDbdvGet(ILgetOperand(stmt, 1)));
	IIITrcsResetCStr();

	return;
}

/*{
** Name:	IIOsfSetFormsExec() -	Execute an 'set_forms' statement
**
** Description:
**	Executes an OSL 'set_forms' statement.
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
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOsfSetFormsExec(stmt)
register IL	*stmt;
{
	i4	rownum;
	char	*parent1;
	char	*parent2;

	parent1 = IIITtcsToCStr(ILgetOperand(stmt, 1));
	parent2 = IIITtcsToCStr(ILgetOperand(stmt, 2));
	rownum = (i4) IIITvtnValToNat(ILgetOperand(stmt, 3), 0,
			ERx("SET_FORMS"));

	if (IIiqset(ILgetOperand(stmt, 4), rownum, parent1, parent2, 0) != 0)
	{
		register IL	*next;

		next = IIOgiGetIl(stmt);
		if ((*next&ILM_MASK) != IL_INQELM)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("inqelm"),
				(char *) NULL);
		}
		else do
		{
			IIITrcsResetCStr();
			IIstfsio((i4) ILgetOperand(next, 4),
				 IIITtcsToCStr(ILgetOperand(next, 2)),
				 (i4) IIITvtnValToNat(ILgetOperand(next, 3), 0,
						     ERx("SET_FORMS")),
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
	IIITrcsResetCStr();

	return;
}

/*{
** Name:	IIOspSleepExec() -	Execute OSL 'sleep' statement
**
** Description:
**	Execute OSL 'sleep' statement.
**
** IL statements:
**	SLEEP VALUE
**
**		The VALUE contains the number of seconds to 'sleep'.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOspSleepExec(stmt)
IL	*stmt;
{
	IIsleep((i4) IIITvtnValToNat(ILgetOperand(stmt, 1), 0, ERx("SLEEP")));
	return;
}

/*{
** Name:	IIITi4gInquire4GL() -	Execute an 'inquire_4gl' stmt
**
** Description:
**	Executes an OSL 'inquire_4gl' statement.
**
** IL statements:
**	INQ4GL
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**	[ PUTFORM VALUE VALUEa ]
**	[  ...  ]
**
**		In each TL2ELM statement, the first VALUE contains the
**		variable to hold the return value, and the second an
**		INQUIRE_4GL constant name.
**		Following the ENDLIST statement, there must be a PUTFORM for
**		each named field which is displayed (as opposed to hidden).
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	09-feb-93 (davel)
**		written.
*/
VOID
IIITi4gInquire4GL ( stmt )
register IL	*stmt;
{
	register IL	*next;

	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL2ELM)
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("listelm"), (char *) NULL);
	else do
	{
	    IIARi4gInquire4GL( IIITtcsToCStr(ILgetOperand(next, 2)),
			       IIOFdgDbdvGet(ILgetOperand(next, 1)));
	    next = IIOgiGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL2ELM);

	if ((*next&ILM_MASK) != IL_ENDLIST)
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"), (char *) NULL);

	return;
}

/*{
** Name:	IIITs4gSet4GL() -	Execute a set_4gl Statement.
**
** Description:
**	Executes an OSL SET_4GL statement.
**
** IL statements:
**	SET4GL
**	TL2ELM VALUEa VALUE
**	  ...
**	ENDLIST
**
**		In each TL2ELM statement, the first VALUE contains the
**		SET_4GL constant name (a constant string) and the second
**		contains a reference to the DBV with the set value.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	09-feb-93 (davel)
**		written.
*/
VOID
IIITs4gSet4GL ( stmt )
IL	*stmt;
{
	register IL	*next;

	next = IIOgiGetIl(stmt);
	if ( (*next&ILM_MASK) != IL_TL2ELM )
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("listelm"),(char*)NULL);
	else do
	{
	    IIARs4gSet4GL( IIITtcsToCStr(ILgetOperand(next, 1)),
			   IIOFdgDbdvGet(ILgetOperand(next, 2)) );
	    next = IIOgiGetIl(next);
	} while ( (*next&ILM_MASK) == IL_TL2ELM );

	if ( (*next&ILM_MASK) != IL_ENDLIST )
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),(char*)NULL);
}

/*{
** Name:	IIOspSetRndExec() -	Execute OSL 'set randome_seed' statement
**
** Description:
**	Execute OSL 'set random_seed' statement.
**
** IL statements:
**	[ SEED VALUE ]
**
**		The VALUE contains the seed to 'sow'.
**
** Inputs:
**	stmt	The IL statement to execute.
**
** History:
**	29-Jan-2007 (kiria01)
**	    Created
*/
IIOspSetRndExec(stmt)
IL	*stmt;
{
	IIset_random((i4) IIITvtnValToNat(ILgetOperand(stmt, 1), 0, ERx("SET RANDOM_SEED")));
	return;
}
