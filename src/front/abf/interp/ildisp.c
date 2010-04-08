/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<frscnsts.h>
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

/**
** Name:	ildisplay.c -	Execute IL statements for display loop.
**
** Description:
**	Executes IL statements for a display loop.  Includes a different
**	'IIITstmtExec()' routine for each type of statement.  In particular,
**	this file defines a routine 'IIOgeGenericExec()' for those IL statements
**	that are in effect no-ops.
**
** History:
**	Revision 5.1  86/10  arthur
**	Initial revision.
**
**	Revision 6.2  88/09  joe
**	Upgraded for Rel 6.
**	05/89  wong.  Added support for unattached submenus BEGSUBMENU, SUBMENU.
**	Also, changed names for 'IIIT' prefix.
**	04/89  billc.  Added static frame support.
**
**	Revision 6.3/03/00  89/11  wong
**	Moved data initialization ('IIARhfiHidFldInit()') to
**	'IIITeilExecuteIL()' in "ilcall.c".
**	89/07/17  jfried  Added support for field/column entry activations.
**
**	Revision 6.3/03/01
**	01/11/91 (emerson)
**		Remove 32K limit on statement line number.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Changes in IIITdisplayEx and IIITinitializeEx.
**	04/22/91 (emerson)
**		Modifications for alerters (new function IIITaAlertActEx).
**	03/09/93 (donc)
**		Modified all the routines to call sameInstr with two arguments,
**		the current IL statement address and the offset into the IL
**		statement.  sameInstr was then modified to evaluate whether
**		the operation associated with the IL statement is the same as
**		the last IL statement by fully resolving the operation through
**		the LONGSID interface. Formerly LONGSIDs were treated the same
**		as non-LONGSIDs. These modifications fix bug 45952. 
**	03/25/93 (donc)
**		Delete changes that attempt to fix bug 42317 which were
**	        inadvertantly included with the fix for bug 45952.
**    10/7/93 (donc) Bug 55191
**            Delete the rest of the 42317 changes
**
**	Revision 6.5
**	26-jun-92 (davel)
**		added argument to the IIOgvGetVal() calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes: prevop seems to be used as i4 everywhere,
**	    so declare it as such.
*/

static VOID	clearInstr();
static bool	sameInstr();

static IL	*dsplpstrt ZERO_FILL;
static i4	iiretval = 0;	/* counter for menu items in a frame */

/*{
** Name:	IIOgeGenericExec	-	Generic exec routine
**
** Description:
**	Generic exec routine.  Does nothing.
**	Called from the 'IIILtab' table for an IL statement which
**	is, in effect, a no-op.
**
** IL statements:
**	IL statements such as START that are currently no-ops.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIOgeGenericExec(stmt)
IL	*stmt;
{
	return;
}

/*{
** Name:	IIITstmtHdrEx() -	OSL Statement Header Statement.
**
** Description:
**	This routine is called whenever a STHD IL statement is recognized.
**	At present, this routine exists solely to set breakpoints prior to
**	executing IL statements for a particular OSL statement (by setting
**	the line number.)
**
** IL statements:
**	IL_STHD		intLineNumberOfStatement
**	IL_LSTHD	intLineNumberOfStatement	intKindOfLabel
**
** Inputs:
**	stmt	{IL *}  The IL statement.
**
** History:
**	10/86 (jhw) -- Written.
**	26-sep-1988 (Joe)
**		Changed comment to show that it also handles IL_LSTHD
**		statements.  No code has to change for this, since
**		those are only used by the code generator.
**	01/11/91 (emerson)
**		Remove 32K limit on statement line number.
*/
IIITstmtHdrEx ( stmt )
register IL	*stmt;
{
	static	i4	line_no = 0;

	char	*text;
	i4	stmt_line_no = (i4) ILgetOperand(stmt, 1);

	if (*stmt&ILM_LONGSID)
	{
		stmt_line_no = IIITvtnValToNat((IL) stmt_line_no, 0,
						ERx("IIITstmtHdrEx"));
	}
	if (stmt_line_no == line_no)
		text = ERx("Set breakpoint here");
#ifdef lint
	/* until we do something with text, this will shut lint up. */
	text = text;
#endif
}

/*{
** Name:	IIITmBeginMenuEx() -	Beginning of a list of menuitems
**
** Description:
**	Beginning of a list of menuitems.
**
** IL statements:
**	BEGMENU intNumberOfItems
**	{events}  - There are intNumberOfItems of these.  They
**		    are described below.
**	ENDMENU sidInitializeCode
**
**    All events are of the form:
**
**       operator operands sidCodeForEvent
**
**   The sidCodeForEvent is the label of the code to handle this event.
**   Several events may (or usually do) point to the same code.
**   If they do, they will be sequential.  This fact is important, since
**   when they point to the same code, we want the FRS to know that, so
**   if say a menu item and a function key execute the same code, the FRS
**   will label the menu item with the the function key's name.
**   The remainder of this section shows the operators and their specific
**   operands.
**
**        IL_MENUITEM     stringItemName stringExplanation
**					 intValidate intActivate
**
**        IL_KEYACT       intKeyNumber stringExplanation intValidate intActivate
**
**        IL_FLDENT       stringFieldName
**        IL_FLDACT       stringFieldName
**
**        IL_COLENT       stringTableFieldName stringColumnName
**        IL_COLACT       stringTableFieldName stringColumnName
**
**
**
**	The BEGMENU statement indicates the beginning of a list of
**	operations in a menu (menuitems or field, column or key activations).
**	Its operand is the number of operations in the menu.  It is followed
**	by a list of intermingled MENUITEM, FLDACT, COLACT and KEYACT
**	statements.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITmBeginMenuEx ( stmt )
register IL	*stmt;
{
	if (IIinitmu() == 0)
	{
		IIOerError(ILE_EQUEL, 0, ERx("IIinitmu"), (char *) NULL);
		IIOslSkipIl(stmt, IL_ENDMENU);
		return;
	}

	IIOFujPushJump(ILgetOperand(stmt, 1));
	/*
	** Initialize counter for number of menu items in this frame.
	*/
	iiretval = 0;
	clearInstr();
}

/*{
** Name:	IIITmdBegDispMenuEx() -	Begin a list of Display submenu items
**
** Description:
**	Begin a list of display submenu items.
**	The routine 'IIITmBeginMenuEx()' handles the beginning of a list
**	of menuitems for the main menu of a frame.
**
** IL statements:
**	BEGDISPMU intNumberOfEvents
**	{events}  - There are intNumberOfEvents here.
**	ENDMENU  sidTopOfSubMenu
**
**	The BEGDISPMU statement indicates the beginning of a list of operations
**	(menuitems or key activations) within a submenu.  Its argument is the
**	number of operations in the submenu.  It is followed by a list of events
**	as with BEGMENU above.
**
**	The list of operations is closed by an ENDMENU statement.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
IIITmdBegDispMenuEx ( stmt )
register IL	*stmt;
{
    if (IInestmu() == 0 || IIinitmu() == 0)
    {
	/*
	** If couldn't nest the menu, then skip forward to the POPSUBMU
	** for this BEGDISPMU.
	*/
	IIOerError(ILE_EQUEL, 0, ERx("IInestmu or IIinitmu"), (char *) NULL);
	IIOslSkipIl(stmt, IL_POPSUBMU);
	return;
    }
    IIOFujPushJump(ILgetOperand(stmt, 1));
    /*
    ** Initialize counter for number of menu items in this menu.
    */
    iiretval = 0;
    clearInstr();
}

/*{
** Name:	IIITmsBegSubMenuEx() -	Begin a List of Submenu Items.
**
** Description:
**	Begin a list of submenu items.
**	The routine 'IIITmBeginMenuEx()' handles the beginning of a list
**	of menuitems for the main menu of a frame.
**
** IL statements:
**	BEGSUBMU intNumberOfEvents
**	{events}  - There are intNumberOfEvents here.
**	ENDMENU  sidTopOfSubMenu
**
**	The BEGSUBMU statement indicates the beginning of a list of operations
**	(menuitems or key activations) within a submenu.  Its argument is the
**	number of operations in the submenu.  It is followed by a list of events
**	as with BEGMENU above.
**
**	The list of operations is closed by an ENDMENU statement.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
IIITmsBegSubMenuEx ( stmt )
register IL	*stmt;
{
	IImuonly();
	if ( IIinitmu() == 0 )
	{
		/*
		** If couldn't nest the menu, then skip forward to the ENDMENU
		** for this BEGSUBMU.
		*/
		IIOerError(ILE_EQUEL, 0, ERx("IIinitmu"), (char *) NULL);
		IIOslSkipIl(stmt, IL_ENDMENU);
		return;
	}
	IIOFujPushJump(ILgetOperand(stmt, 1));
	/*
	** Initialize counter for number of menu items in this menu.
	*/
	iiretval = 0;
	clearInstr();
}

/*{
** Name:	IIITdisplayEx() -	Start a display loop
**
** Description:
**	The DISPLAY statement, equivalent to an EQUEL 'display'
**	statement, starts a display loop for the current form.
**
** IL statements:
**
**	IL_DISPLAY  sidBeginMenu.
**
**	The sid points to the IL_BEGMENU for this display loop.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Change the way we check to see if we're in a frame.
**		Also note that we now call IIOFiesIsExecutedStatic
**		instead IIOFisIsStatic, but that's only because the old
**		IIOFisIsStatic was renamed to IIOFiesIsExecutedStatic.
*/
VOID
IIITdisplayEx ( stmt )
register IL	*stmt;
{
    char	*formname;

    if ((formname = IIOFfnFormname()) == NULL || IIOFilpInLocalProc())
    {
	IIOerError(ILE_FRAME, ILE_FORMNAME, IIOFfrFramename(),
			(char *) NULL);
    }
    else if (IIdispfrm(formname,
			IIOFiesIsExecutedStatic() ? ERx("u") : ERx("f")) == 0)
    {
	IIOerError(ILE_FRAME, ILE_FRMDISP, formname, (char *) NULL);
    }
    else
    {
	IIOsiSetIl(ILgetOperand(stmt, 1));
    }
}

/*{
** Name:	IIITdlDispLpEx() -	Top of a display loop
**
** Description:
**	Top of a display loop.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** IL statements:
**	STHD line-number
**	DISPLOOP
**
**	Top of the display loop for a frame.
**	The next IL statement to be executed will be determined
**	by the operation chosen by the user.
**
** Side effects:
**	Resets the global statement pointer based on the selected operation.
**
** History:
**	09/86 (agh) -- Written.
**	04/89 (jhw) -- Rewrote to loop using 'IIchkfrm()'.  JupBug #5309.
*/
VOID
IIITdlDispLpEx ( stmt )
register IL	*stmt;
{
	dsplpstrt = stmt;      
	while ( IIrunform() == 0 )
	{
		if ( IIchkfrm() != 0 )
		{ /* error */
			IIOerError( ILE_EQUEL|ILE_FRAME, 0, ERx("IIrunform"),
				    (char *) NULL
			);
			return;
		}
	}
	/* OK only */
	IIOjiJumpIl(IIretval());
}

/*{
** Name:	IIITmEndMenuEx() -	End of a list of menuitems
**
** Description:
**	End of a list of menuitems.
**
** IL statements:
**
**	ENDMENU sidCodeForDisplayOrMenuLoop
**
**	See the explanation in the header for 'IIITmBeginMenuEx()'.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITmEndMenuEx ( stmt )
register IL	*stmt;
{
    if (IIendmu() == 0)
    {
	/*
	** The menu we just finished didn't seem to take.  Simply
	** go  on.  The next statement is either POPSUBMU for
	** a submenu, or EOF.  In either case, this menu context
	** can't be run.
	*/
	IIOerError(ILE_EQUEL, 0, ERx("IIendmu"), (char *) NULL);
	return;
    }
    IIOsiSetIl(ILgetOperand(stmt, 1));
}

/*{
** Name:	IIITeofEx() -	Generate error if EOF is reached
**
** Description:
**	Generates error message if the IL statement indicating
**	end-of-file is reached.
**	The main loop of the interpreter should never try to execute
**	such a statement.
**
** IL statements:
**	EOF
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
IIITeofEx ( stmt )
IL	*stmt;
{
	IIOerError(ILE_FRAME, ILE_EOFIL, (char *) NULL);
}

/*{
** Name:	FldActEx() -	Set up a field activation
**
** Description:
**	Set up a field activation.  Called by IIITaFldActEx and IIITeFldEntEx.
**
** IL statements:
**
**	FLDACT stringFieldName sidCodeForActivate
**	  ...
**
**	For further information, see the explanation in the header
**	for 'IIITmBeginMenuEx()'.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
static
VOID
FldActEx ( fcn_name, entry_act, stmt )
char  *fcn_name ;
i4     entry_act ;
register IL	*stmt;
{
    if (!sameInstr(stmt, 2))
	iiretval++;
    if (IIFRafActFld((char *) IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL),
            entry_act,
	    iiretval) == 0)
    {
	/*
	** See comment in IIOctColactExec
	*/
	IIOerError(ILE_EQUEL, 0, fcn_name, (char *) NULL);
	IIOslSkipIl(stmt, IL_ENDMENU);
	return;
    }
    IIOFsjSetJump(iiretval, stmt, ILgetOperand(stmt, 2));
}

/*{
** Name:	IIITaFldActEx() -	Set up a field activation
**
** Description:
**	Set up a field activation.
**
** IL statements:
**
**	FLDACT stringFieldName sidCodeForActivate
**	  ...
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITaFldActEx ( stmt )
register IL	*stmt;
{
    FldActEx ( "IIITaFldActEx", 0, stmt ) ;
}

/*{
** Name:	IIITeFldEntEx() -	Set up a field entry activation
**
** Description:
**	Set up a field entry activation.
**
** IL statements:
**
**	FLDENT stringFieldName sidCodeForActivate
**	  ...
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITeFldEntEx ( stmt )
register IL	*stmt;
{
    FldActEx ( "IIITeFldEntEx", 1, stmt ) ;
}

/*{
** Name:	ColActEx() -	Set up a column activation
**
** Description:
**	Sets up a column activation.  Called by IIITaColActEx and IIITeColEntEx.
**
** IL statements:
**      IL_COLENT       stringTableFieldName stringColumnName sidCodeForActivate
**      IL_COLACT       stringTableFieldName stringColumnName sidCodeForActivate
**
**	For further information, see the explanation in the header
**	for 'IIITmBeginMenuEx()'.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
static
VOID
ColActEx ( fcn_name, entry_act, stmt )
char  *fcn_name ;
i4     entry_act ;
register IL	*stmt;
{
    if (!sameInstr(stmt, 3))
	++iiretval;
    if (IITBcaClmAct((char *) IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL),
	    (char *) IIOgvGetVal(ILgetOperand(stmt, 2), 
						DB_CHR_TYPE, (i4 *)NULL),
            entry_act,
	    iiretval) == 0)
    {
	/*
	** If there was an error setting up one of the activates, abandon
	** the whole menu.  This goes to the ENDMENU.  On a submenu, the
	** next statement is a POPSUBMU which is fine, since it will get
	** rid of the jump table, and restore the other menu.  If this is
	** the whole frame menu, then the next statement will be EOF, which
	** will get another error and end the frame.  That should be fine
	** though (the two errors) since this is a pretty unusual
	** case.
	*/
	IIOerError(ILE_EQUEL, 0, fcn_name, (char *) NULL);
	IIOslSkipIl(stmt, IL_ENDMENU);
	return;
    }
    IIOFsjSetJump(iiretval, stmt, ILgetOperand(stmt, 3));
}

/*{
** Name:	IIITaColActEx() -	Set up a column activation
**
** Description:
**	Sets up a column activation.
**
** IL statements:
**      IL_COLACT       stringTableFieldName stringColumnName sidCodeForActivate
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITaColActEx ( stmt )
register IL	*stmt;
{
    ColActEx ( "IIITaColActEx", 0, stmt ) ;
}

/*{
** Name:	IIITeColEntEx() -	Set up a column entry activation
**
** Description:
**	Sets up a column entry activation.
**
** IL statements:
**      IL_COLENT       stringTableFieldName stringColumnName sidCodeForActivate
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITeColEntEx ( stmt )
register IL	*stmt;
{
    ColActEx ( "IIITeColEntEx", 1, stmt ) ;
}

/*{
** Name:	IIITaKeyActEx() -	Set up a key activation
**
** Description:
**	Sets up a key activation.
**
** IL statements:
**
** IL_KEYACT       intKeyNumber stringExplanation intValidate
**			intActivate sidCodeForKeyActivate
**
**	For further information, see the explanation in the header
**	for 'IIITmBeginMenuEx()'.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITaKeyActEx ( stmt )
register IL	*stmt;
{
	if (!sameInstr(stmt, 5))
	    iiretval++;
	if (IInfrskact((i4) (*((i4 *)IIOgvGetVal(ILgetOperand(stmt, 1), 
					DB_INT_TYPE, (i4 *)NULL))),
			ILcharVal(ILgetOperand(stmt, 2), NULL),
			(i4) ILgetOperand(stmt, 3),
			(i4) ILgetOperand(stmt, 4),
			iiretval) == 0)
	{
		/*
		** See comment in IIOctColactExec
		*/
		IIOerError(ILE_EQUEL, 0, ERx("IIactfld"), (char *) NULL);
		IIOslSkipIl(stmt, IL_ENDMENU);
		return;
	}
	IIOFsjSetJump(iiretval, stmt, ILgetOperand(stmt, 5));
}

/*{
** Name:	IIITaTimeoutActEx() - Execute an timeout activation.
**
** Description:
**	Implements an activate on a timeout.
**
** IL Statements:
**	
**	IL_ACTIMOUT	int#SecondsForTimeout	sidCodeForTimeout
**
** Inputs:
**	stmt	{IL *}  IL_ACTIMOUT statement.
**
** History:
**	7-dec-1988 (Joe)
**		First Written
*/
VOID
IIITaTimeoutActEx ( stmt )
register IL	*stmt;
{
    if (!sameInstr(stmt, 2))
	++iiretval;
    if (IIFRtoact((i4) ILgetOperand(stmt, 1), iiretval) == 0)
    {
	/*
	** See comment in IIOctColactExec
	*/
	IIOerError(ILE_EQUEL, 0, ERx("IIFRtoact"), (char *) NULL);
	IIOslSkipIl(stmt, IL_ENDMENU);
	return;
    }
    IIOFsjSetJump(iiretval, stmt, ILgetOperand(stmt, 2));
}

/*{
** Name:	IIITaAlertActEx() - Execute an alerter activation.
**
** Description:
**	Implements an activate on an alerter event.
**
** IL Statements:
**	
**	IL_ACTALERT	sidCodeForAlerter
**
** Inputs:
**	stmt	{IL *}  IL_ACTALERT statement.
**
** History:
**	04/22/91 (emerson)
**		Created.
*/
VOID
IIITaAlertActEx ( stmt )
register IL	*stmt;
{
    if (!sameInstr(stmt, 1))
	++iiretval;
    if (IIFRaeAlerterEvent(iiretval) == 0)
    {
	/*
	** See comment in IIOctColactExec
	*/
	IIOerError(ILE_EQUEL, 0, ERx("IIFRtoact"), (char *) NULL);
	IIOslSkipIl(stmt, IL_ENDMENU);
	return;
    }
    IIOFsjSetJump(iiretval, stmt, ILgetOperand(stmt, 1));
}

/*{
** Name:	IIITaMenuItemEx() -	Set up a menu item
**
** Description:
**	Set up a menu item.
**
** IL statements:
**
**        IL_MENUITEM     stringItemName stringExplanation
**					 intValidate intActivate
**	  ...
**
**	In the MENUITEM statement, the first VALUE contains the name
**	of the menu item, the next is possibly a string containing an
**	explanation, and the last is possibly an integer containing the
**	validation setting.
**	The SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IIITmBeginMenuEx()'.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	4-oct-1988 (Joe)
**		Update for 6.0.  Now calls IInmuact and takes
**		operands 3 and 4 to be integer values rather than
**		a variable.
*/
VOID
IIITaMenuItemEx ( stmt )
register IL	*stmt;
{
	if (!sameInstr(stmt, 5))
		++iiretval;

	if (IInmuact((char *) IIOgvGetVal(ILgetOperand(stmt, 1), 
						DB_CHR_TYPE, (i4 *)NULL),
			ILcharVal(ILgetOperand(stmt, 2), NULL),
			(i4) ILgetOperand(stmt, 3),
			(i4) ILgetOperand(stmt, 4),
			iiretval) == 0)
	{
		/*
		** See comment in IIOctColactExec
		*/
		IIOerError(ILE_EQUEL, 0, ERx("IIactmenu"), (char *) NULL);
		IIOslSkipIl(stmt, IL_ENDMENU);
		return;
	}
	IIOFsjSetJump(iiretval, stmt, ILgetOperand(stmt, 5));
}

/*{
** Name:	IIITmlMenuLpEx() -	Top of a submenu loop
**
** Description:
**	Top of a submenu loop.
**	This routine serves the same purpose, for a submenu, as
**	IIOdlDisploopExec() does for a frame's main menu.
**
** IL statements:
**	MENULOOP
**
**	Top of a submenu loop.
**	The next IL statement to be executed will be determined
**	by the operation chosen by the user.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** Side effects:
**	Resets the global statement pointer based on the selected operation.
**
*/
VOID
IIITmlMenuLpEx ( stmt )
register IL	*stmt;
{

	while ( IIrunnest() == 0 )
	{
		if ( IIchkfrm() != 0 )
		{
		/*
		** I'd like to go to the ENDMENU for this menu.  However,
		** there is no way to find if based on the information I
		** have.  The next ENDMENU in the IL could be the right one,
		** but then again, it could an ENDMENU for a nested
		** menu that is started by code in this menu.  Note, that
		** both the BEGIN and the END MENU for this submenu are
		** below us in the IL.  I guess the only thing to do
		** it exit the frame.
		*/
			IIOerError( ILE_EQUEL|ILE_FRAME, 0, ERx("IIrunnest"),
					(char *) NULL
			);
		}
	}
	IIOjiJumpIl(IIretval());

}

/*{
** Name:	IIITmSubMenuEx() -	Top of a submenu loop
**
** Description:
**	Top of a submenu loop.
**	This routine serves the same purpose, for a submenu, as
**	IIOdlDisploopExec() does for a frame's main menu.
**
** IL statements:
**	SUBMENU
**
**	Top of a submenu loop.
**	The next IL statement to be executed will be determined
**	by the operation chosen by the user.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** Side effects:
**	Resets the global statement pointer based on the selected operation.
**
** History:
**	05/89 (jhw) -- Written.
*/
VOID
IIITmSubMenuEx ( stmt )
register IL	*stmt;
{
	IIrunmu(0);
	IIOjiJumpIl(IIretval());
	IIOFpjPopJump();
}

/*{
** Name:	IIITinitializeEx() -	Initialization for a frame
**
** Description:
**	Performs the required initialization for an OSL frame or procedure.
**
** IL statements:
**	INITIALIZE
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	10/86 (agh) -- Written.
**	04/19/89 (billc) -- Added support for static frames.
**	11/89 (jhw) -- Moved data initialization to 'IIITeilExecuteIL()'.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Change the way we check to see if we're in a frame.
**	08/13/93 (donc)
**		Added global IIarRtsprm. This is the only way a 3GL
**		procedure can pass the correct calling arguments to
**		the frame to be called.
**	10/6/93 (donc)
**		Changed access to global IIarRtsprm. Added function
**	        IIARgarGetArRtsprm to access the global, thereby hiding
**		the global from the linker and allowing resolution at    
**		runtime. This was done because interplib in shared 
**		environments preceeds abfrt in the linking process. To
**		have a globalref defined in this module will cause
**	        unresolved symbol warnings.	 
**	10/7/93 (donc)
**		remove #include <rtsdata.h>, it causes unresolved references
**		to occur at link time against shareable images.
**	11/16/93 (donc)
**		Moved IIARgarGetArRtsprm out of ildisp.c and into ilcall.sc
*/

VOID
IIITinitializeEx ( stmt )
IL	*stmt;
{
	char	*formname;

	/*
	** If current stack frame has a formname and we're not in a local proc,
	** then we are within an OSL frame rather than an OSL procedure.
	*/

	if ( (formname = IIOFfnFormname()) != NULL && !IIOFilpInLocalProc() )
	{

	    IIARfinFrmInit( formname, IIOFgpGetPrm(),
				IIOFgcGetFdesc(), IIOFdgDbdvGet(1)
	    );
	}
	else
	{
	    IIARpinProcInit(IIOFrnRoutinename(), IIOFgpGetPrm(),
				IIOFgcGetFdesc(), IIOFdgDbdvGet(1)
	    );
	}
}

/*{
** Name:	IIITmPopSubmuEx() -	Close a submenu
**
** Description:
**	Closes a submenu by popping off its jump table.
**
** IL statements:
**	POPSUBMU
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITmPopSubmuEx ( stmt )
IL	*stmt;
{
	IIendnest();
	IIOFpjPopJump();
}

/*
** The following is used by clearInstr and sameInstr.  These two
** static routines are used to determine if two sequential activate
** statements in the IL will execute the same code.  See the comments
** on menus for an explanation of why this is needed.  
*/
static i4 prevop = 0;

/*{
** Name:	clearInstr	- Clear the instruction that is stored.
**
** Description:
**	clearInstr is part of a two routine protocol for deciding if
**	two IL activate statements will execute the same code.
**
**	clearInstr is used with sameInstr.  clearInstr should be
**	called anytime that a new set of instructions are being looked
**	at.  The first call to sameInstr after clearInstr is guaranteed
**	to be FALSE.
**
** History:
**	22-nov-1988 (Joe)
**		First Written
*/
static VOID
clearInstr()
{
    prevop = 0;
}

/*{
** Name:	sameInstr	- Decide if this statement is the same as last.
**
** Description:
**	This return determines if statement passed in, is the same statement
**	(in terms of address) as the last one that sameInstr was called
**	with.  If it is, then sameInstr returns TRUE.  Otherwise, FALSE
**	is returned.
**
**	sameInstr will always return FALSE after a call to clearInstr, as
**	long as a true statement address is passed.
**
** Inputs:
**	stmt		The statement to see if it is the same as the last.
**	offset		The offset into the statement that specifies where
**			the operation is to be found for the statement.
**
** Outputs:
**	Returns:
**		TRUE 	if this statement is the same as the last.
**		FALSE	otherwise.
**
** History:
**	09-mar-1993 (donc)
**		Modifications to properly handle operations that involve
**		LONGSIDS
**	22-nov-1988 (Joe)
**		First Written
*/
static bool
sameInstr(stmt, offset)
IL	*stmt;
IL 	offset;
{

    i4 stmt_offset;
    IL *temp;
    i4  operand; 

    /*
    ** The following code resolves operations that involve LONGSID 
    ** translations.  Formerly, LONGSIDs were handled on the same manner
    ** as normal operations, in that an offset could be applied directly
    ** to the statement address to find the start of the operation in
    ** question.  Now LONGSIDs will be resolved through the same mechanism 
    **  as a IIOFsjSetJump. This fixes bug 45952 (DonC)
    */

    operand = ILgetOperand( stmt, offset );
    stmt_offset = (i4) operand;
    if ( *stmt & ILM_LONGSID ) 
	stmt_offset = IIITvtnValToNat( operand, 0, ERx("sameInstr"));
    temp = stmt + stmt_offset;
    stmt_offset = ( temp - IIOframeptr->frminfo.il);
 
    if ( prevop == stmt_offset )
	return TRUE;
    else
	prevop = stmt_offset;
    return FALSE;
}
