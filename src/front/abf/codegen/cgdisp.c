/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<si.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>
#include        <abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgil.h"
#include	"cglabel.h"
#include	"cgout.h"
#include	"cgerror.h"

/**
** Name:	cgdisplay.c -	Code Generator Display/Menu Loop Module.
**
** Description:
**	Contains routines used to generates C code from IL statements for a
**	display or menu loop.  Includes a different 'IICGstmtGen()' routine
**	for each type of statement.  Also,  includes 'IICGnullOp()' for
**	statements that have no operation.  Defines:
**
**	IICGnullOp()
**	IICGmBeginMenu()
**	IICGmdBegDispMenu()	generate code for display menu setup.
**	IICGmsBegSubMenu()	generate code for submenu setup.
**	IICGdisplay()
**	IICGdlDispLoop()
**	IICGmEndMenu()
**	IICGmPopSubmu()
**	IICGeof()
**	IICGaFldAct()
**	IICGaColAct()
**	IICGaKeyAct()
**	IICGaMenuItem()
**	IICGaTimeoutAct()
**	IICGaAlertAct()
**	IICGmlpMenuLoop()
**	IICGmSubMenu()
**	IICGinitialize()
**	IICGstmtHdr()		output comment for statement header.
**	IICGlstmtHdr()		generate code for labeled statement header.
**
** History:
**	Revision 6.0  87/07  arthur
**	Initial revision.
**
**	Revision 6.1  88/11  wong
**	Added ACTIMOUT support.
**
**	Revision 6.2  89/05  wong
**	Added support for BEGSUBMU and SUBMENU for unattached submenus.
**	89/12  Added support for global line number.
**
**	Revision 6.3/03/00  89/11  wong
**	Moved hidden field initialization to routine initialization in
**	"cgfiltop.c".
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	01/11/91 (emerson)
**		Remove 32K limit on number of source statements.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures
**		(in IICGinitialize and IICGmEndMenu).
**	04/22/91 (emerson)
**		Modifications for alerters (added function IICGaAlertAct).
**
**	Revision 6.4/02
**	04/25/92 (emerson)
**		One more fix to allow > 64K bytes of IL (in _genActivate).
**
**	Revision 6.5
**	29-jun-92 (davel)
**		Minor change - added new argument to call to IIORcqConstQuery().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF i4	iiCGlineNo;	/* current line number */

static i4	iiretval = 0;	/* counter for menu items in a frame */
static IL	*prevop = NULL; /* beginning of last operation */

static VOID	EndActList();

/*{
** Name:	IICGnullOp() -	Generic code generation routine.
**
** Description:
**	Generic code generation routine.  Does nothing.
**	Called from the 'IIILtab' table for an IL statement which
**	is, in effect, a no-op.
**
** IL statements:
**	IL statements such as START that are currently no-ops.
**
** Code generated:
**	None.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
*/
VOID
IICGnullOp ( stmt )
IL	*stmt;
{
	return;	/* null function! */
}

/*{
** Name:	IICGmBeginMenu() -	Beginning of a list of menuitems
**
** Description:
**	Generate code for the beginning of a list of menuitems.
**
** IL statements:
**	BEGMENU INT
**	MENUITEM VALUE STRING INT INT SID
**	  ...
**	FLDACT VALUE SID
**	  ...
**	FLDENT VALUE SID
**	  ...
**	COLACT VALUE VALUE SID
**	  ...
**	COLENT VALUE VALUE SID
**	  ...
**	KEYACT VALUE STRING INT INT SID
**	  ...
**	ACTIMOUT INT SID
**	...
**	ENDMENU SID
**
**	The BEGMENU statement indicates the beginning of a list of operations in
**	a menu (menuitems or field, column or key activations).  Its operand is
**	the number of operations in the menu.  It is followed by a list of
**	intermingled MENUITEM, FLDACT, COLACT, FLDENT, COLENT, ACTIMOUT and
**	KEYACT statements.
**
**	In both MENUITEM and KEYACT statements, the VALUE contains the name of
**	the menu item or the number of the FRS key, respectively; the STRING is
**	a (possibly empty) string specifying an explanation; the first INT is
**	an integer specifying the validation setting; and the second INT is an
**	integer specifying the activation setting.
**	In the FLDACT and FLDENT statements, the VALUE is the name of the field.
**	In the COLACT and COLENT statements, the first VALUE is the name of the
**	table field, and the second the name of the column.
**	In the ACTIMOUT statement, the INT contains the time-out period value.
**
**	In each of these activation specification statements, the SID gives the
**	offset within the IL array for the first statement of the operation.
**
**	The list of operations is closed by an ENDMENU statement.  Its SID
**	operand gives the offset within the IL array for the initialization
**	for this display loop.
**
** Code generated:
**
**	if (IIinitmu() == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
VOID
IICGmBeginMenu ( stmt )
register IL	*stmt;
{
	EndActList();
	IICGlblLabelGen(stmt);
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIinitmu"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();

	/*
	** Initialize counter for number of menu items in this frame,
	** and address within IL array of previous operation.
	*/
	iiretval = 0;
	prevop = NULL;
	return;
}

/*{
** Name:	IICGmdBegDispMenu() -	Begin a List of Display Sub-Menu Items.
**
** Description:
**	Generate code to begin a list of sub-menu items for a display menu.
**	The routine 'IICGmBeginMenu()' handles the beginning of a list
**	of menuitems for the main menu of a frame.
**
** IL statements:
**	BEGDISPMU INT
**	MENUITEM VALUE STRING INT INT SID
**	  ...
**	FLDACT VALUE SID
**	  ...
**	FLDENT VALUE SID
**	  ...
**	COLACT VALUE VALUE SID
**	  ...
**	COLENT VALUE VALUE SID
**	  ...
**	KEYACT VALUE STRING INT INT SID
**	  ...
**	ACTIMOUT INT SID
**	  ...
**	ENDMENU SID
**
**	The BEGDISPMU statement indicates the beginning of a list of operations
**	(menuitems or key activations) within a displayed submenu.  Its operand
**	is the number of operations in the submenu.  It is followed by a list of
**	intermingled MENUITEM, KEYACT, FLDACT, etc. statements.
**
**	For further information on these last statements, see the explanation
**	in the header for 'IICGmBeginMenu()'.
**
**	The list of operations is closed by an ENDMENU statement.  Its SID
**	operand gives the offset within the IL array for the MENULOOP statement
**	for this display menu loop.
**
** Code generated:
**
**	if (IInestmu() == 0 || IIinitmu() == 0)
**	{
**		goto end-of-nested-display;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
VOID
IICGmdBegDispMenu ( stmt )
register IL	*stmt;
{
	EndActList();
	IICGpxlPrefixLabel(CGL_MUINIT, CGLM_FORM);
	IICGlblLabelGen(stmt);

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IInestmu"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoprPrint(ERx(" || "));
	IICGocbCallBeg(ERx("IIinitmu"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();

	/*
	** Initialize counter for number of menu items in this menu,
	** and address within IL array of previous operation.
	*/
	iiretval = 0;
	prevop = NULL;
	return;
}

/*{
** Name:	IICGmsBegSubMenu() -	Begin a List of Sub-menu Items.
**
** Description:
**	Generate code to begin a list of sub-menu items.
**
** IL statements:
**	BEGSUBMU INT
**	MENUITEM VALUE STRING INT INT SID
**		...
**	KEYACT VALUE STRING INT INT SID
**		...
**	ACTIMOUT INT SID
**		...
**	ENDMENU SID
**
**	The BEGSUBMU statement indicates the beginning of a list of operations
**	(menuitems or key activations) specification statements for a submenu.
**	Its operand is the number of operations in the submenu.  It is followed
**	by a list of intermingled MENUITEM, KEYACT and ACTIMOUT statements.
**
**	For further information on these last statements, see the explanation
**	in the header for 'IICGmBeginMenu()'.
**
**	The list of operations is closed by an ENDMENU statement.  Its SID
**	operand gives the offset within the IL array for the SUBMENU statement
**	for this submenu.
**
** Code generated:
**
**	IImuonly();
**	if (IIinitmu() == 0)
**	{
**		goto end-of-nested-display;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/89 (jhw) -- Written.
*/
VOID
IICGmsBegSubMenu ( stmt )
register IL	*stmt;
{
	EndActList();
	IICGpxlPrefixLabel(CGL_MUINIT, CGLM_FORM);
	IICGlblLabelGen(stmt);

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IImuonly"));
	IICGoceCallEnd();
	IICGoseStmtEnd();

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIinitmu"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();

	/*
	** Initialize counter for number of menu items in this menu,
	** and address within IL array of previous operation.
	*/
	iiretval = 0;
	prevop = NULL;
}

/*{
** Name:	IICGdisplay() -	Generate Code for DISPLAY Statement.
**
** Description:
**	Generates code for the IL DISPLAY statement, which starts a
**	display for the current form.
**
** IL statements:
**	DISPLAY SID
**
**	The SID gives the offset within the IL array for the statements
**	to set up the menu.
**
** Code generated:
**
**	if (IIdispfrm(form-name, "f") == 0)
**	{
**		goto end-of-display-loop;
**	}
**	goto menu-initialization;
**
**	label: ;
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	03/88 (jhw) -- Moved CGL_FDBEG label generation here.  JupBug #2239.
**	04/89 (jhw) -- Moved CGL_FDBEG label generation back to
**			'IICGdlDispLoop()'.
**	04/89 (billc) -- Added support for static frames.
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
*/
VOID
IICGdisplay ( stmt )
register IL	*stmt;
{
	register IL	*next;
	char		buf[CGSHORTBUF];

	IICGlebLblEnterBlk();
	IICGlarLblActReset();
	IICGlnxLblNext(CGLM_FORM);

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIdispfrm"));
	IICGocaCallArg( _FormVar );

	if (CG_static)
		IICGocaCallArg(ERx("first == 1 ? \"f\" : \"u\""));
	else
		IICGocaCallArg(ERx("\"f\""));

	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();
	IICGgtsGotoStmt(IICGesEvalSid(stmt, ILgetOperand(stmt, 1)));

	/*
	** Generate a label for the next IL statement, unless it is an
	** INITIALIZE, which will generate its own label.
	*/
	next = IICGnilNextIl(stmt);
	if ((*next&ILM_MASK) != IL_INITIALIZE)
		IICGlblLabelGen(next);
	return;
}

/*{
** Name:	IICGdlDispLoop() -	Top of a Display Loop.
**
** Description:
**	Generates code for the top of a display loop.
**
** IL statements:
**	STHD line-number
**	DISPLOOP
**
**	Top of the display loop for a frame.
**	The next statement to be executed will be determined
**	by the operation chosen by the user.
**
** Code generated:
**
**	label: ;		// possible if not already done
**	begin-run-form-loop: ;
**	if (IIrunform() == 0)
**	{
**		goto end-of-run-form-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	03/88 (jhw) -- Moved CGL_FDBEG label generation to 'IICGdisplay()'.
**	04/89 (jhw) -- Moved CGL_FDBEG label generation here and now goto
**		CGL_FDFINAL on error.  JupBug #5309.  (#2239 fix was incorrect;
**		the correct fix is now in 'IICGfleFileEnd()'.)
*/
VOID
IICGdlDispLoop ( stmt )
register IL	*stmt;
{
	/*
	** If the OSL file contained an initialize block, then that block
	** ended with a GOTO to the DISPLOOP statement, and a label has
	** already been generated for this IL statement.  If there was
	** no initialize block, a label must be generated here.
	*/
	if (!IICGlrqLabelRequired(stmt))
		IICGlblLabelGen(stmt);

	IICGpxlPrefixLabel(CGL_FDBEG, CGLM_FORM);

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIrunform"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDFINAL, CGLM_FORM);
	IICGobeBlockEnd();

	return;
}

/*{
** Name:	IICGmEndMenu() -	End of a List of Menuitems.
**
** Description:
**	Generates code for the end of a list of menuitems.
**
** IL statements:
**	ENDMENU SID
**
**	The SID gives the offset within the IL array for the initialization
**	for this display loop.
**
**	See the explanation in the header for 'IICGmBeginMenu()'.
**
** Code generated:
**
**	if (IIendmu() == 0)
**	{
**		goto end-of-display-loop;
**	}
**	goto label-for-initializations;
**
**	label-for-end-of-run-form-loop:
**	if (IIchkfrm() == 0)
**	{
**		goto begin-run-form-loop;
**	}
**	label-for-end-of-display-loop:
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	04/89 (jhw) -- Added CGL_FDFINAL label generation and 'IIchkfrm()'
**		check generation.  JupBug #5309.
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL): support long SIDs
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Check to see if IL_ENDMENU is at end of a procedure;
**		treat as if at end of file.
*/
VOID
IICGmEndMenu ( stmt )
register IL	*stmt;
{
	register IL	*next, op;

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIendmu"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();

	IICGgtsGotoStmt(IICGesEvalSid(stmt, ILgetOperand(stmt, 1)));

	IICGpxlPrefixLabel(CGL_FDFINAL, CGLM_FORM);
	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIchkfrm"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDBEG, CGLM_FORM);
	IICGobeBlockEnd();

	IICGpxlPrefixLabel(CGL_FDEND, CGLM_FORM);

	/*
	** Close the current label block, unless this ENDMENU
	** is the last IL statement in the file or procedure.
	*/
	next = IICGnilNextIl(stmt);
	op = (*next&ILM_MASK);
	if (op == IL_STHD)
	{
		next = IICGnilNextIl(next);
		op = (*next&ILM_MASK);
	}
	if (op != IL_EOF && op != IL_LOCALPROC)
	{
		IICGlexLblExitBlk();
	}
}

/*{
** Name:	IICGmPopSubmu() -	Close a Sub-Menu.
**
** Description:
**	Generates code to close a submenu.
**
** IL statements:
**	POPSUBMU
**
** Code generated:
**
**	IIendnest();
**#ifdef Attached Query End
**	if (IIiqset(1,0,(char *)0) != 0 && IImode[0] != '\0')
**		IIstfsio(21,(char *)0,0,(short *)0,1,32,0,IImode);
**#endif
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	10/90 (jhw) -- Removed local 'IImode[]' block close, which is no longer
**		needed to support forms mode switch for attached queries; bug
**		#31929.  This change also corrects #32994 as a side-effect.
*/
VOID
IICGmPopSubmu ( stmt )
register IL	*stmt;
{
	register IL	*next;

	/* end the sub-menu */
	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIendnest"));
	IICGoceCallEnd();
	IICGoseStmtEnd();

	/* Note:  If the next statement is a QRYFREE statement, it must be the
	** end of an attached query.  So generate reset of forms mode to save
	** mode in 'IImode[]'.
	*/
	if ( *(next = IICGnilNextIl(stmt)) == IL_QRYFREE )
	{
		/*
		** Output code to set to back to previous mode.
		**
		**	if (IIiqset(1,0,(char *)0) != 0)
		**		IIstfsio(21,(char*)0,0,(short*)0,1,32,0,IImode);
		*/
		IICGoibIfBeg();
			IICGocbCallBeg(ERx("IIiqset"));
			IICGocaCallArg( _One );
			IICGocaCallArg( _Zero );
			IICGocaCallArg(ERx("(char *)0"));
			IICGoceCallEnd();
		IICGoicIfCond( (char *) NULL, CGRO_NE, _Zero );
		IICGoprPrint(ERx(" && "));
		IICGoicIfCond(ERx("IImode[0]"), CGRO_NE, ERx("'\\0'"));
	
		IICGoikIfBlock();
			/* Set to previous mode */
			IICGosbStmtBeg();
			IICGocbCallBeg(ERx("IIstfsio"));
			IICGocaCallArg(ERx("21"));
			IICGocaCallArg(ERx("(char *)0"));
			IICGocaCallArg( _Zero );
			IICGocaCallArg(ERx("(short *)0"));
			IICGocaCallArg( _One );
			IICGocaCallArg(ERx("32"));
			IICGocaCallArg( _Zero );
			IICGocaCallArg(ERx("IImode"));
			IICGoceCallEnd();
			IICGoseStmtEnd();
		IICGobeBlockEnd();
	}

	return;
}

/*{
** Name:	IICGeof() -	Generate Error if EOF is Reached.
**
** Description:
**	Sets a flag indicating that the end of the IL file has been reached.
**
** IL statements:
**	EOF
**
** Code generated:
**	None.
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
VOID
IICGeof ( stmt )
register IL	*stmt;
{
	IICGdonFrmDone = TRUE;
	return;
}

/*
** Name:	_genActivate() -	Generate Code for an Activation.
**
** Description:
**	Generates code to set up an activation.  The function call and its
**	arguments are generated from the 'func' and '(*opgen)()' parameters,
**	resp.  The activation number operand is incremented whenever the
**	operation IL code for the activation changes.  (The operation IL code
**	is specified in the operand specified by the 'actop' parameter.)
**
** Code generated:
**
**	if ("funcname"("opgen()") == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**	actop	{nat}  The operand of the IL statement that specifies the
**			IL offset of the first statement for the operation.
**	func	{char *}  The name of the activation specification function.
**	opgen	{VOID (*)()}  The function that will generate the arguments
**			for the activation specification function.
**
** History:
**	04/89 (jhw) -- Abstracted from the orginal code for each activation
**			generation function.
**	04/25/92 (emerson)
**		One more fix to allow > 32K of IL:  Check for a long SID
**		when computing curop.  (Bug 43870).
*/
static VOID
_genActivate ( stmt, actop, func, opgen )
register IL	*stmt;
i4		actop;
char		*func;
VOID		(*opgen)();
{
	IL	*curop;	 /* beginning in IL array of current operation */
	char	buf[CGSHORTBUF];

	IICGoibIfBeg();
	IICGocbCallBeg(func);
	(*opgen)(stmt);

	/*
	** Check if this activation applies to the same operation
	** as the last specified activation.
	*/
	curop = IICGesEvalSid(stmt, ILgetOperand(stmt, actop));
	if (prevop != curop)
	{
		prevop = curop;
		++iiretval;
	}

	_VOID_ CVna(iiretval, buf);
	IICGocaCallArg(buf);
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
	IICGobeBlockEnd();
}

/*{
** Name:	IICGaFldAct() -	Set up a Field Activation.
**
** Description:
**	Generates code to set up a field activation.
**
** IL statements:
**	BEGMENU
**	FLDACT VALUE SID
**	  ...
**
**	In the FLDACT statement, the VALUE is the name of the field.
**	The SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**
**	if (IIFRafActFld(field-name, 0, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
static VOID
_actFld ( stmt )
register IL	*stmt;
{
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
        IICGocaCallArg("0") ;
}

VOID
IICGaFldAct ( stmt )
IL	*stmt;
{
	_genActivate(stmt, 2, ERx("IIFRafActFld"), _actFld);
}

/*{
** Name:	IICGeFldEnt() -	Set up a Field Activation.
**
** Description:
**	Generates code to set up a field activation.
**
** IL statements:
**	BEGMENU
**	FLDACT VALUE SID
**	  ...
**
**	In the FLDENT statement, the VALUE is the name of the field.
**	The SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**
**	if (IIFRafActFld(field-name, 1, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
static VOID
_entFld ( stmt )
register IL	*stmt;
{
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
        IICGocaCallArg("1") ;
}

VOID
IICGeFldEnt ( stmt )
IL	*stmt;
{
	_genActivate(stmt, 2, ERx("IIFRafActFld"), _entFld);
}

/*{
** Name:	IICGaColAct() -	Set up a Column Activation.
**
** Description:
**	Generates code to set up a column activation.
**
** IL statements:
**	BEGMENU INT
**	  ...
**	COLACT VALUE VALUE SID
**	  ...
**
**	In the COLACT statement, the first VALUE is the name of the table
**	field, and the second the name of the column.
**	The SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**
**	if (IITBcaClmAct(tblfld-name, column-name, 0, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
static VOID
_actCol ( stmt )
register IL	*stmt;
{
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 2), DB_CHR_TYPE));
        IICGocaCallArg("0") ;
}

VOID
IICGaColAct ( stmt )
IL	*stmt;
{
	_genActivate(stmt, 3, ERx("IITBcaClmAct"), _actCol);
}

/*{
** Name:	IICGeColEnt() -	Set up a Column Activation.
**
** Description:
**	Generates code to set up a column activation.
**
** IL statements:
**	BEGMENU INT
**	  ...
**	COLACT VALUE VALUE SID
**	  ...
**
**	In the COLACT statement, the first VALUE is the name of the table
**	field, and the second the name of the column.
**	The SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**
**	if (IITBcaClmAct(tblfld-name, column-name, 1, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
static VOID
_entCol ( stmt )
register IL	*stmt;
{
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 2), DB_CHR_TYPE));
        IICGocaCallArg("1") ;
}

VOID
IICGeColEnt ( stmt )
IL	*stmt;
{
	_genActivate(stmt, 3, ERx("IITBcaClmAct"), _entCol);
}

/*{
** Name:	IICGaKeyAct() -	Set up a Key Activation.
**
** Description:
**	Generates code to set up a key activation.
**
** IL statements:
**	BEGMENU
**	  ...
**	KEYACT VALUE STRING INT INT SID
**	  ...
**
**	In the KEYACT statement, the VALUE contains the number of the FRS key;
**	the STRING is a (possibly empty) string specifying an explanation; the
**	first INT is an integer specifying the validation setting; and the
**	second INT is an integer specifying the activation setting.  The SID
**	gives the offset within the IL array for the first statement of this
**	operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**	For an OSL 'key frskeyn' declaration, without any explanation,
**	validation setting or activation setting:
**
**	if (IInfrskact(key-number, (char *) NULL, 2, 2, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
static VOID
_keyAct ( stmt )
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_INT_TYPE));
	IICGocaCallArg( ILCHARVAL( ILgetOperand(stmt, 2), _NullPtr ) );
	/*
	** The Forms System uses the FRS_UF constant to indicate that the
	** user did not specify a validation or an activation.
	** The OSL translator has put this constant in-line if no
	** action was specified.
	*/
	_VOID_ CVna((i4) ILgetOperand(stmt, 3), buf);
	IICGocaCallArg(buf);
	_VOID_ CVna((i4) ILgetOperand(stmt, 4), buf);
	IICGocaCallArg(buf);

}

VOID
IICGaKeyAct ( stmt )
IL	*stmt;
{
	_genActivate(stmt, 5, ERx("IInfrskact"), _keyAct);
}

/*{
** Name:	IICGaMenuItem() -	Set up a Menu Item.
**
** Description:
**	Generates code to set up a menu item.
**
** IL statements:
**	MENUITEM VALUE STRING INT INT SID
**	  ...
**
**	In the MENUITEM statement, the VALUE contains the name of the menu item;
**	the STRING is a (possibly empty) string specifying an explanation; the
**	first INT is an integer specifying the validation setting; and the
**	second INT is an integer specifying the activation setting.  The SID
**	gives the offset within the IL array for the first statement of this
**	operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**	For an OSL menuitem declaration, without any explanation,
**	validation setting or activation setting:
**
**	if (IInmuact(menuitem-name, (char *) NULL, 2, 2, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
*/
static VOID
_menuAct ( stmt )
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGocaCallArg(IICGgvlGetVal(ILgetOperand(stmt, 1), DB_CHR_TYPE));
	IICGocaCallArg( ILCHARVAL(ILgetOperand(stmt, 2), _NullPtr) );
	/*
	** The Forms System uses the FRS_UF constant to indicate that the
	** user did not specify a validation or an activation.
	** The OSL translator has put this constant in-line if no
	** action was specified.
	*/
	_VOID_ CVna((i4) ILgetOperand(stmt, 3), buf);
	IICGocaCallArg(buf);
	_VOID_ CVna((i4) ILgetOperand(stmt, 4), buf);
	IICGocaCallArg(buf);
}

VOID
IICGaMenuItem ( stmt )
IL	*stmt;
{
	_genActivate(stmt, 5, ERx("IInmuact"), _menuAct);
}

/*{
** Name:	IICGaTimeoutAct() -	Set up an Activation on Timeout.
**
** Description:
**	Generates code to set up an activation on timeout.
**
** IL statements:
**	ACTIMOUT INT SID
**	  ...
**
**	In the ACTIMOUT statement, the INT is the value for the timeout.
**	The SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**	For an 4GL ON TIMEOUT activation:
**
**	if (IIFRtoact(time_out_period, iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	11/88 (jhw) -- Written.
*/
static VOID
_actTimeOut ( stmt )
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	_VOID_ CVna((i4) ILgetOperand(stmt, 1), buf);
	IICGocaCallArg(buf);
}

VOID
IICGaTimeoutAct ( stmt )
IL	*stmt;
{
	_genActivate( stmt, 2, ERx("IIFRtoact"), _actTimeOut );
}

/*{
** Name:	IICGaAlertAct() -	Set up an Activation on Alerter Event.
**
** Description:
**	Generates code to set up an activation on an alerter event.
**
** IL statements:
**	ACTALERT SID
**	  ...
**
**	In the ACTALERT statement,
**	the SID gives the offset within the IL array for the first
**	statement of this operation.
**
**	For further information, see the explanation in the header
**	for 'IICGmBeginMenu()'.
**
** Code generated:
**	For an 4GL ON DBEVENT activation:
**
**	if (IIFRaeAlerterEvent(iiretval-number) == 0)
**	{
**		goto end-of-display-loop;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	04/22/91 (emerson)
**		Created.
*/
static VOID
_actAlerter ( stmt )
register IL	*stmt;
{
	;
}

VOID
IICGaAlertAct ( stmt )
IL	*stmt;
{
	_genActivate( stmt, 1, ERx("IIFRaeAlerterEvent"), _actAlerter );
}

/*{
** Name:	IICGmlpMenuLoop() -	Top of a Display Sub-Menu Loop.
**
** Description:
**	Generates code for the top of a display submenu loop.
**	This routine serves the same purpose, for a display submenu, as
**	'IICGdlpDisploopGen()' does for a frame's main display loop.
**
** IL statements:
**	MENULOOP
**
**	Top of a display submenu loop.
**	The next statement to be executed will be determined
**	by the operation chosen by the user.
**
** Code generated:
**
**	label-for-nested-initializations: ;
**	label-for-nested-block-begin: ;
**	if (IIrunnest() == 0)
**	{
**		goto end-of-nested-display;
**	}
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	04/89 (jhw) -- Corrected to goto CGL_FDFINAL on error.  JupBug #5309.
*/
VOID
IICGmlpMenuLoop ( stmt )
register IL	*stmt;
{
	IICGlebLblEnterBlk();
	IICGlarLblActReset();
	IICGlnxLblNext(CGLM_FORM);

	IICGlblLabelGen(stmt);
	IICGlitLabelInsert(stmt, CGLM_NOBLK);
	IICGpxlPrefixLabel(CGL_FDINIT, CGLM_FORM);
	IICGpxlPrefixLabel(CGL_FDBEG, CGLM_FORM);

	IICGoibIfBeg();
	IICGocbCallBeg(ERx("IIrunnest"));
	IICGoceCallEnd();
	IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
	IICGoikIfBlock();
		IICGpxgPrefixGoto(CGL_FDFINAL, CGLM_FORM);
	IICGobeBlockEnd();
}

/*{
** Name:	IICGmSubMenu() -	Top of a Sub-Menu.
**
** Description:
**	Generates code for the top of a submenu.
**	This routine serves the same purpose, for a submenu, as
**	'IICGdlpDisploopGen()' does for a frame's main display loop.
**
** IL statements:
**	SUBMENU
**
**	Top of a submenu.
**	The next statement to be executed will be determined
**	by the operation chosen by the user.
**
** Code generated:
**
**	label-for-nested-initializations: ;
**	label-for-nested-block-begin: ;
**	IIrunmu(0);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	05/89 (jhw) -- Written.
*/
VOID
IICGmSubMenu ( stmt )
register IL	*stmt;
{
	IICGlebLblEnterBlk();
	IICGlarLblActReset();
	IICGlnxLblNext(CGLM_FORM);

	IICGlblLabelGen(stmt);
	IICGlitLabelInsert(stmt, CGLM_NOBLK);
	IICGpxlPrefixLabel(CGL_FDINIT, CGLM_FORM);
	IICGpxlPrefixLabel(CGL_FDBEG, CGLM_FORM);

	IICGosbStmtBeg();
	IICGocbCallBeg(ERx("IIrunmu"));
	IICGocaCallArg( _Zero );
	IICGoceCallEnd();
	IICGoseStmtEnd();
}

/*{
** Name:	IICGinitialize() -	Initialization for a Frame/Procedure.
**
** Description:
**	Generates code to perform the required initialization for an
**	OSL frame or procedure.
**
** IL statements:
**	INITIALIZE
**
** Code generated:
**	1) For an OSL frame:
**
**	label-for-initializations: ;
**	first = 0;	(only if frame is static)
**	if (IIfsetio((char *) NULL) == 0)
**	{
**		goto end-of-display-loop;
**	}
**	IIARfinFrmInit(form-name, rtsfparam, &fdesc-for-frame, IIdbdvs);
**
**	2) For an OSL procedure:
**
**	label-for-initializations: ;
**	IIARpinProcInit(proc-name, rtsfparam, &fdesc-for-proc, IIdbdvs);
**
** Inputs:
**	stmt	{IL *}  The IL statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	04/89 (billc) -- Added support for static frames.
**	11/89 (jhw) -- Moved hidden field initialization to "cgfiltop.c".
**	07/90 (jhw) -- Moved static "first" clear to 'IICGfleFileEnd()' in
**		"cgfiltop.c".  This guarantees that it will be set before the
**		frame returns for the first time, and not too early! Bug #30608.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Change the name of the FDESC.
**		Also use the local procedure name in call to IIARpinProcInit.
*/
VOID
IICGinitialize ( stmt )
register IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGlblLabelGen(stmt);
	IICGpxlPrefixLabel(CGL_FDINIT, CGLM_FORM);

	/*
	** If current stack frame has a formname, then we are within
	** an OSL frame rather than an OSL procedure.
	*/
	if (CG_form_name != NULL)
	{
		IICGoibIfBeg();
		IICGocbCallBeg(ERx("IIfsetio"));
		IICGocaCallArg( _NullPtr );
		IICGoceCallEnd();
		IICGoicIfCond( (char *) NULL, CGRO_EQ, _Zero );
		IICGoikIfBlock();
			IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
		IICGobeBlockEnd();
	}

	IICGosbStmtBeg();
	if (CG_form_name != NULL)
	{
		IICGocbCallBeg(ERx("IIARfinFrmInit"));
		STcopy( _FormVar, buf );
	}
	else
	{
		IICGocbCallBeg(ERx("IIARpinProcInit"));
		_VOID_ STprintf(buf, ERx("\"%s\""), CG_routine_name);
	}
	IICGocaCallArg(buf);
	IICGocaCallArg(ERx("rtsfparam"));
	IICGocaCallArg(ERx("&IIfdesc"));
	IICGocaCallArg(ERx("IIdbdvs"));
	IICGoceCallEnd();
	IICGoseStmtEnd();
	return;
}

/*
** Name:	PrintLineNo() -	Output Line Number Comment.
**
** Description:
**	Outputs a comment showing the input line number.  This line number
**	should correspond to the line of the input OSL source that generated
**	the code following this comment.
**
** Input:
**	lineno	{i4}  The line number.
**
** History:
**	07/87 (jhw) -- Written.
**	01/11/91 (emerson)
**		Remove 32K limit on number of source statements.
*/
static VOID
PrintLineNo (lineno)
i4 lineno;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGoprPrint( STprintf(buf, ERx("/* Line: %ld */"), lineno) );
	IICGoleLineEnd();
}

static i4	line_no = 0;	/* for debugging only */

/*
** Name:	GetLineNo() -	Get Line Number from a STHD or LSTHD statement.
**
** Input:
**	stmt	{IL *}	The statement from which to retrive the line number.
**
** Returns:
**	{i4}  The line number.
**
** History:
**	01/11/91 (emerson)
**		Written (to remove 32K limit on number of source statements).
**	29-jun-92 (davel)
**		Minor change - added new argument to call to IIORcqConstQuery().
*/
i4
GetLineNo (stmt)
IL	*stmt;
{
	i4	current;

	current = iiCGlineNo = (i4) ILgetOperand(stmt, 1);
	if (*stmt & ILM_LONGSID)
	{
		PTR	rval = NULL;

		_VOID_ IIORcqConstQuery(&IICGirbIrblk, -(current),
					DB_INT_TYPE, &rval, (i4 *)NULL);
		current = (i4) *((i4 *) rval);
	}
	if (current == line_no)
	{
		char	*text;

		text = ERx("Set breakpoint here");
	}
	return current;
}

/*{
** Name:	IICGstmtHdr() -	Output Comment for Statement Header.
**
** Description:
**	Outputs a comment showing the line number of the OSL input
**	source that generated the STHD and any following IL code.
**
** IL statements:
**	STHD INT
**
**		The INT is the line number in the original OSL file.
**
** Input:
**	stmt	{IL *}	The statement for which to generate code.
**
** History:
**	07/87 (jhw) -- Written.
**	12/89 (jhw) -- Set global current line number.
**	01/11/91 (emerson)
**		Remove 32K limit on number of source statements.
*/
VOID
IICGstmtHdr ( stmt )
IL	*stmt;
{
	PrintLineNo(GetLineNo(stmt));
}

/*{
** Name:	IICGlstmtHdr() - Generate Code for Labeled Statement Header.
**
** Description:
**	Generates code for a labeled statement header.
**	This IL statement indicates that a label must be generated,
**	or that a different operation is about to begin.
**
**	This routine can also be used to set breakpoints prior to
**	executing IL statements for a particular OSL statement (by setting
**	the line number.)
**
** IL statements:
**	LSTHD INT INT
**
**		The first INT is the line number in the original OSL file.
**		The second is a constant indicating what type of label
**		is required at this point.
**
** Code generated:
**	For a block containing the operations for a menuitem, field
**	or key activation:
**
**	/* Line:  line-no * /
**	if (IIretval() == iiretval-number)
**	{
**
** Inputs:
**	stmt	{IL *}	The statement for which to generate code.
**
** History:
**	07/87 (agh) -- Written.
**	12/89 (jhw) -- Set global current line number.
**	01/11/91 (emerson)
**		Remove 32K limit on number of source statements.
*/
VOID
IICGlstmtHdr ( stmt )
register IL	*stmt;
{
	i4		current;
	char	buf[CGSHORTBUF];

	current = GetLineNo(stmt);

	switch (ILgetOperand(stmt, 2))
	{
	  case IL_LB_MENU:
	  {
		i4	retval;

		if ((retval = IICGlnxLblNext(CGLM_ACTIV)) == 1)
		{
			if (current != 0)
			PrintLineNo(current);
			IICGoibIfBeg();
		}
		else
		{
			IICGobeBlockEnd();
			if (current != 0)
			PrintLineNo(current);
			IICGoeiElseIf();
		}
		IICGocbCallBeg(ERx("IIretval"));
		IICGoceCallEnd();
		_VOID_ CVna(retval, buf);
		IICGoicIfCond((char *) NULL, CGRO_EQ, buf);
		IICGoikIfBlock();
		break;
	  }

	  case IL_LB_WHILE:
	  case IL_LB_UNLD:
		if (current != 0)
			PrintLineNo(current);
		if (!IICGlrqLabelRequired(stmt))
		{
			IICGlitLabelInsert(stmt, CGLM_NOBLK);
			IICGlblLabelGen(stmt);
		}
		break;

	  case IL_LB_NONE:
	  default:
		break;
	} /* end switch */
	return;
}

/*
** Name:	EndActList() -	End list of activations
**
** Description:
**	Generates code to end a list of activations.
**	Called from 'IICGmsBegSubMenu()' and 'IICGmBeginMenu()'.
**
** Code generated:
**
**	else
**	{
**		if (1 == 1)
**		{
**			goto end-of-display-loop;
**		}
**	}
*/
static VOID
EndActList()
{
	IICGobeBlockEnd();
	IICGoebElseBlock();
		IICGoibIfBeg();
		IICGoicIfCond( _One , CGRO_EQ, _One );
		IICGoikIfBlock();
			IICGpxgPrefixGoto(CGL_FDEND, CGLM_FORM);
		IICGobeBlockEnd();
	IICGobeBlockEnd();
}
