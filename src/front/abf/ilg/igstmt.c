/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include 	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<ex.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#ifndef FEalloc
#define FEalloc(size, status) FEreqlng(0, (size), FALSE, (status))
#endif
#include	<iltypes.h>
#include	<ilops.h>
#include	"ilexpr.h"
#include	"igrdesc.h"

/**
** Name:	igstmt.c -	OSL Interpreted Frame Object Generator
**				Intermediate Language Statement Generator Module
** Description:
**	Contains the routines used to generate IL statements for IL operators
**	used to interpret OSL statements.  The IL statements are generated into
**	an internal structure for later output.
**
**	Also contains routines that allocate and set statement identifiers
**	(SIDs) for use by modules that wish to generate intermediate language
**	statements for the OSL interpreter.  SIDs are used as labels for IL
**	statements during code generation.
**
**	This file defines:
**
**	iiIGstmtInit()		initiate igstmt processing.
**	iiIGofOpenFragment()	open a new fragment of IL.
**	iiIGcfCloseFragment()	close a fragment of IL.
**	iiIGrfReopenFragment()	reopen a closed fragment of IL.
**	iiIGifIncludeFragment()	include a fragment of IL.
**	IGstartStmt()		generate IL OSL statement header.
**	IGgenStmt()		generate IL statement.
**	iiIGnsaNextStmtAddr()	return address of next IL statement.
**	IGgenExpr()		generate IL expression statement.
**	IGinitSID()		return a unique SID.
**	IGsetSID()		associate an SID with an IL offset.
**
**	(ILG internal)
**	IGoutStmts()		output IL statments for interpreted frame object
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Major changes, primarily to address bug 34846:
**		Added new functions iiIGofOpenFragment, iiIGcfCloseFragment,
**		iiIGrfReopenFragment, and iiIGifIncludeFragment.
**		Replaced iiIGlsaLastStmtAddr with iiIGnsaNextStmtAddr.
**		Merged the old igsid.c into this module, since it was so small
**		and now shares private data with this module.
**		Every function was modified.
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Deleted functions iiIGgetOffset and iiIGupdStmt;
**		created function iiIGlsaLastStmtAddr; revised IL_STMT structure
**		(for bugs 39581, 41013, and 41014: array performance).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		modified IGgenStmt and IGoutStmts.
**	04/22/91 (emerson)
**		Modifications for alerters (in IGoutStmts).
**	05/03/91 (emerson)
**		Modifications for alerters (in IGoutStmts):
**		Handle GET EVENT properly.
**		(It needs a special call to LIBQ and thus a special IL op code).
**	05/07/91 (emerson)
**		Added logic in IGoutStmts to generate NEXTPROC statement
**		(for codegen).
**	07/05/91 (rudyw)
**		SCO compiler got confused by call spread out over many lines.
**		Compiler accepted call when presented on a single line.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	01/11/91 (emerson)
**		Remove 32K limit on line numbers in STHD and LSTHD statements.
**
**	Revision 6.0  87/06  wong
**	Changes for expression generation.
**	Added SID processing for IL_DISPLAY and IL_ENDMENU, and
**	for IL_VALROW (which was a 5.1/PC bug also.)
**	Added `activate' operand for IL_MENUITEM.
**	Added 'iiIGupdStmt()' and 'iiIGgetOffset()'.  88/02
**
**	Revision 5.1  86/10/10  14:31:29  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset.
**      25-Mar-2008 (hanal04) Bug 118967
**          Adjust fix for bug 118967. Use I4ASSIGN_MACRO to avoid SIGBUS 
**          on su9.
**	24-Feb-2010 (frima01) Bug 122490
**	    Renaming getsid to iigetsid as neccessary
**	    to avoid confusion with system function.
*/

/* IL Operator Look-up Table (translator.) */

/* Re-define IIILtab on DG so that IIINTERP and OSLSQL can co-exits */

# ifdef DGC_AOS
# define IIILtab IIdgILtab
# endif

GLOBALREF IL_OPTAB	IIILtab[];

/*
** Name:	IL_STMT	-	Internal IL Statement Structure.
**
** Description:
**	An element of the internal intermediate language structure.
**	Each element represents one IL instruction.  The elements are linked
**	into circular lists ("IL lists"), with one list per "fragment".
**	Each circular list is anchored by a node with il_op == IL_OPEN_ANCHOR
**	(while the list is built) or IL_CLOSED_ANCHOR (when the list is closed).
**	The anchor's il_next points to the element for the first operation
**	in the fragment, and the element for the last operation (so far)
**	in the fragment has its il_next pointing to the anchor.
**	Every time an element is added to the list, an new anchor is created,
**	and the previous anchor is converted to the new last element.
**
** History:
**	01/11/91 (emerson)
**		Remove 32K limit on line numbers in STHD and LSTHD statements
**		by using "longsid" techniques:  put statement number into new
**		i4 il_line member of IL_STMT structure instead of i2 il_op1
**	11/07/91 (emerson)
**		Converted il_op and il_op1 thru il_op5 to an array whose address
**		can be returned by the new function iiIGlsaLastStmtAddr
**		(for bugs 39581, 41013, and 41014: array performance).
**	09/20/92 (emerson)
**		Revisions for bug 34846: Added il_child and il_parent to union.
**		Updated description to describe new fragment support.
*/

/* Internal, artificial pseudo-opcodes */

#define	IL_OPEN_ANCHOR		(IL_MAX_OP + 1)	/* "anchor" for an open list */
#define	IL_CLOSED_ANCHOR	(IL_MAX_OP + 2)	/* "anchor" for a closed list */
#define	IL_INCLUDE		(IL_MAX_OP + 3) /* an "include" IL element */
#define	IL_MAX_PSEUDO_OP	(IL_MAX_OP + 3)

typedef struct _ilstmt
{
	IL	il_ops[6];		/* IL operator and up to 5 operands */
#define		il_op	il_ops[0]
#define		il_op1	il_ops[1]
#define		il_op2	il_ops[2]
#define		il_op3	il_ops[3]
#define		il_op4	il_ops[4]
#define		il_op5	il_ops[5]

	i4	il_offset;		/* current IL offset in fragment */

	union {
		IGSID		*il_osid;	/* SID or RDESC (if any) */
		IL		*il_oexpr;	/* IL expression code (if any)*/
		i4		il_oline;	/* line number (if any) */
		struct _ilstmt	*il_ochild;	/* For an IL_INCLUDE node:
						** anchor of included IL list */
		struct _ilstmt	*il_oparent;	/* For an IL_OPEN_ANCHOR node:
						** links elements in the stack
						** of open lists (except for
						** the active open list).
						** For an IL_CLOSED_ANCHOR node:
						** during IGoutStmts, while IL
						** is being generated for an
						** included list anchored by
						** this node, points to the
						** IL_INCLUDE node representing
						** the include. */
	}	il_obj;
#define		il_sid		il_obj.il_osid
#define		il_expr 	il_obj.il_oexpr
#define		il_line 	il_obj.il_oline
#define		il_child 	il_obj.il_ochild
#define		il_parent 	il_obj.il_oparent

	struct _ilstmt	*il_next;		/* next element in IL list */
} IL_STMT;

/* Internal IL Code Data Structures */

static FREELIST	*sidlp = NULL;		/* SID list pool */

static FREELIST	*listp = NULL;		/* IL list pool */

static IL_STMT	*il_list = NULL;	/* current anchor of currently active
					** fragment's circular IL list */

static IL_STMT	*il_list_parent = NULL;	/* anchor of current fragment's parent*/

static i4	il_offset = 0;		/* While IL is being built: offset
					** of next IL instruction in the
					** current fragment relative to the
					** beginning of the fragment.
					** After IL built: total size of IL */

/*{
** Name:	allocIL() -	Allocate an IL_STMT structure.
**
** Description:
**	Internal routine to allocate an IL_STMT structure.
**
** Returns:
**	Non-null pointer to allocated IL_STMT structure
**	with structure members set as follows:
**
**	il_op		IL_OPEN_ANCHOR.
**	il_offset	current contents of the static variable il_offset.
**
**	Remaining members set to binary zeros (courtesy of FElpget).
**
** Exceptions:
**	EXFEMEM		Memory unavailable.
**
** History:
**	09/20/92 (emerson)
**		Created (extracted from IGstartStmt) for bug 34846.
*/
static IL_STMT *
allocIL()
{
	IL_STMT	*il;

	while ((il = (IL_STMT *)FElpget(listp)) == NULL)
	{
		EXsignal(EXFEMEM, 1, ERx("IGgenExpr"));
		/*NOTREACHED*/
	}
	il->il_op = IL_OPEN_ANCHOR;
	il->il_offset = il_offset;

	return il;
}

/*{
** Name:	iiIGstmtInit() -	Initiate igstmt processing.
**
** Description:
**	This must be called before any other routines in this file.
**	Failure to do so can cause e.g. access violations.
**
** History:
**	09/20/92 (emerson)
**		Created (extracted from IGstartStmt & IGinitSID) for bug 34846.
*/
VOID
iiIGstmtInit()
{
	while ((sidlp = FElpcreate(sizeof(IGSID))) == NULL)
	{
		EXsignal(EXFEMEM, 1, ERx("IGinitSID"));
		/*NOTREACHED*/
	}
	while ((listp = FElpcreate(sizeof(IL_STMT))) == NULL)
	{
		EXsignal(EXFEMEM, 1, ERx("iiIGofOpenFragment"));
		/*NOTREACHED*/
	}
	il_list = allocIL();	/* allocate an anchor for the default fragment*/
	il_list->il_next = il_list; /* set the fragment's circular list empty*/
}

/*{
** Name:	iiIGofOpenFragment() -	Open a new fragment of IL.
**
** Description:
**	Opens a new fragment of IL and redirects output to it.
**
**	General discussion of fragments:
**
**	Initially, IGstartStmt, IGgenStmt, and IGgenExpr write their IL
**	to a "default" fragment.  However, this routine can be used to
**	temporarily redirect output to an out-of-line "fragment".
**	This routine creates a new fragment; iiIGcfCloseFragment
**	closes it (and returns an opaque pointer to it).
**
**	Multiple fragments may be open concurrently; they are "stacked".
**	Closing a fragment redirects output to the fragment underneath
**	(referred to as the "parent" fragment).  A closed fragment may be
**	reopened (so that output is redirected to the end of it).
**
**	Closed fragments may be spliced into open fragments by calling
**	iiIGifIncludeFragment.  However, once a fragment has been thus
**	included, it may not subsequently be reopened.
**
**	Care should be taken when using SIDs in fragments.  (See IGinitSID
**	and IGsetSID).  As a general rule, you should generate branches
**	only to SIDs which are set in the same fragment as the branch, or in
**	a fragment that's an "ancestor" of the fragment containing the branch.
**	Violating this rule may cause branches to unexpected locations.
**	At worst, if you generate a branch to a SID which is set in a
**	non-ancestor fragment, and the fragment hasn't yet been included,
**	you may wind up with branches into the middle of instructions.
**
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
VOID
iiIGofOpenFragment()
{
	il_list->il_parent = il_list_parent;
	il_list_parent = il_list;	/* save pointer to "parent" fragment */
	il_offset = 0;			/* set fragment's current size to 0 */
	il_list = allocIL();		/* allocate an anchor for new fragment*/
	il_list->il_next = il_list;	/* set fragment's circular list empty*/
}

/*{
** Name:	iiIGcfCloseFragment() -	Close a fragment of IL.
**
** Description:
**	Closes the currently active fragment of IL (the one to which IL is
**	currently being written) and redirects output to the "parent" fragment:
**	the fragment that was receiving output when the currently active
**	fragment was opened.
**
**	Attempting to close the "default" fragment can cause unpleasant results,
**	such as access violations.
**
**	See iiIGofOpenFragment for a general discussion of fragment processing.
**
** Returns:
**	An opaque pointer to the closed fragment.  This must be specified
**	as the input parameter to iiIGifIncludeFragment when the fragment
**	is subsequently included.
**
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
PTR
iiIGcfCloseFragment()
{
	IL_STMT	*il = il_list;

	il_list = il_list_parent;	/* pop the top fragment off of
					** the stack of active fragments */
	il_list_parent = il_list->il_parent;
	il_list->il_parent = NULL;
	il_offset = il_list->il_offset;	/* restore the current size of
					** the fragment we uncovered */
	il->il_op = IL_CLOSED_ANCHOR;	/* indicate popped fragment is closed */
	return (PTR)il; 		/* return the popped fragment */
}

/*{
** Name:	iiIGrfReopenFragment() -	Reopen a fragment of IL.
**
** Description:
**	Reopens a closed fragment of IL and redirects output to it.
**	The specified fragment must not yet have been included anywhere.
**
** Input:
**	The opaque pointer that was returned by iiIGcfCloseFragment
**	when the fragment was closed.
**
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
VOID
iiIGrfReopenFragment(fragment_anchor)
PTR	fragment_anchor;
{
	IL_STMT	*il = (IL_STMT *)fragment_anchor;

	/*
	** Abort if we're not being passed an anchor of a closed fragment,
	** or if it's a closed fragment that's already been included somewhere.
	*/
	if (il->il_op != IL_CLOSED_ANCHOR || il->il_op1 != 0)
	{
		EXsignal(EXFEBUG, 1, ERx("iiIGrfReopenFragment: bad call"));
	}

	il_list->il_parent = il_list_parent;
	il_list_parent = il_list;	/* save pointer to "parent" fragment */
	il_list = il;			/* make passed-in list current */
	il_offset = il->il_offset;	/* get its current size */
	il->il_op = IL_OPEN_ANCHOR;	/* indicate it's open */
}

/*{
** Name:	iiIGifIncludeFragment() -	Include a fragment of IL.
**
** Description:
**	Includes a closed fragment of IL in the currently open fragment.
**
**	See iiIGofOpenFragment for a general discussion of fragment processing.
**
** Input:
**	The opaque pointer that was returned by iiIGcfCloseFragment
**	when the fragment was closed.
**
** History:
**	09/20/92 (emerson)
**		Created (for bug 34846).
*/
VOID
iiIGifIncludeFragment(fragment_anchor)
PTR	fragment_anchor;
{
	IL_STMT	*il = (IL_STMT *)fragment_anchor;

	/*
	** Abort if we're not being passed an anchor of a closed fragment.
	*/
	if (il->il_op != IL_CLOSED_ANCHOR)
	{
		EXsignal(EXFEBUG, 1, ERx("iiIGifIncludeFragment: bad call"));
	}

	/*
	** Indicate that the closed fragment has been included somewhere.
	*/
	il->il_op1 = 1;

	/*
	** Build an IL_INCLUDE element on top of the current anchor.
	** Bump il_offset by the size of the entire included fragment.
	*/
	il_list->il_child = il;
	il_list->il_op = IL_INCLUDE;
	il_offset += il_list->il_child->il_offset;

	/*
	** Now allocate a new anchor for current fragment,
	** and splice it into the circular list.
	*/
	il = allocIL();
	il->il_next = il_list->il_next;
	il_list->il_next = il;
	il_list = il;
}

/*{
** Name:	IGstartStmt() -	Generate IL OSL Statement Header.
**
** Description:
**	Generates an internal IL statement header (STHD) or labeled statement
**	header (LSTHD) for an OSL statement.  (This is an operator that
**	associates the OSL source statement line number with the IL offset
**	of the code it generates for debugging.)
**
**	Also, associated with some statement headers is a label type used so
**	the code generator can detect certain types of labels that it must
**	generate.
**
** Inputs:
**	line_no	{nat}  The source line number of the OSL statement.
**	label	{nat}  The label type, if any:
**				IL_LB_NONE	none
**				IL_LB_WHILE	etc.
**
** History:
**	06/86 (jhw) -- Written.
**	03/87 (jhw) -- Added support of labels.
**	01/11/91 (emerson)
**		Remove 32K limit on line numbers in STHD and LSTHD statements
**		by using "longsid" techniques:  put statement number into new
**		i4 il_line member of IL_STMT structure instead of i2 il_op1
**		Note: ideally the first parm to this function should be
**		a i4 instead of a nat; as it is, we'll still be limited
**		to 32K source statements on platforms with 16-bit nat's.
**	09/20/92 (emerson)
**		Revisions for bug 34846: The logic for maintaining the circular
**		list of IL instructions has been modified to support "fragments"
*/
VOID
IGstartStmt (line_no, label)
i4	line_no;
i4	label;
{
	IL_STMT	*il = il_list;

	/*
	** Build an IL element on top of the current anchor.
	*/
	il->il_line = (i4)line_no;
	if (label == IL_LB_NONE)
	{
		il->il_op = IL_STHD;
		il_offset += 2;
	}
	else
	{
		il->il_op = IL_LSTHD;
		il->il_op2 = (IL)label;
		il_offset += 3;
	}

	/*
	** Now allocate a new anchor for current fragment,
	** and splice it into the circular list.
	*/
	il = allocIL();
	il->il_next = il_list->il_next;
	il_list->il_next = il;
	il_list = il;
}

/*{
** Name:	IGgenStmt() -	Generate IL Statement.
**
** Description:
**	Generates an internal IL statement with the appropriate operands.
**
** Input:
**	op_with_mods	{ILOP}  IL statement operator with modifier flags.
**	sid		{IGSID *}  Possible SID reference associated with stmt.
**	nops		{nat}  Number of operands.
**	op1 ... op5	{ILREF}  Operator operands.
**
** History:
**	08/86 (jhw) -- Written.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Allow IL_LOCALPROC and IL_MAINPROC to specify a different
**		number of operands than indicated by IIILtab.
**		(We supply 0 operands on the call to IGgenStmt;
**		we'll be computing the operands from the routine's
**		RDESC during IGoutStmts).
**	09/20/92 (emerson)
**		Revisions for bug 34846: The logic for maintaining the circular
**		list of IL instructions has been modified to support "fragments"
*/

/*VARARGS3*/
VOID
IGgenStmt (op_with_mods, sid, nops, op1, op2, op3, op4, op5)
ILOP	op_with_mods;
IGSID	*sid;
i4	nops;
ILREF	op1, op2, op3, op4, op5;
{
	IL_STMT	*il = il_list;
	ILOP	operator = (op_with_mods&ILM_MASK);

	if (sid != NULL || operator == IL_QRYNEXT || operator == IL_QRYSINGLE)
		++nops;

	if (operator == IL_LOCALPROC || operator == IL_MAINPROC)
		nops = IIILtab[operator].il_stmtsize - 1;
	else if (operator < 0 || operator > IL_MAX_OP ||
		nops != IIILtab[operator].il_stmtsize - 1)
	{
		char	buf[64];

		(VOID)STprintf( buf,
			ERx("(in routine `IGgenStmt(%d(%d)%d(%d))')"),
			operator, nops, IL_MAX_OP,
			IIILtab[operator].il_stmtsize - 1 );
		EXsignal(EXFEBUG, 1, buf);
		/*NOTREACHED*/
	}

	/*
	** Build an IL element on top of the current anchor.
	*/
	il->il_op = op_with_mods;
	il->il_sid = sid;
	il->il_op1 = op1;
	il->il_op2 = op2;
	il->il_op3 = op3;
	il->il_op4 = op4;
	il->il_op5 = op5;
	il_offset += nops + 1;

	/*
	** Now allocate a new anchor for current fragment,
	** and splice it into the circular list.
	*/
	il = allocIL();
	il->il_next = il_list->il_next;
	il_list->il_next = il;
	il_list = il;
}

/*{
*** Name:	iiIGnsaNextStmtAddr() -	return address of next IL statement
**
** Description:
**	Returns the address (in the private IL_STMT structure) of where
**	the next IL statement will be generated.  The caller may subsequently
**	update the statement.  The operator may be updated directly
**	(it's at the beginning of the statement).  Operands may be updated
**	using the ILsetOperand macro defined in abf!hdr!iltypes.h
**	(syntax: ILsetOperand(stmt, num, val)).
**
** Returns:
**	{IL *}  The address of the next IL statement.
**
** History:
**	11/07/91 (emerson)
**		Created (for bugs 39581, 41013, and 41014: array performance).
**	09/20/92 (emerson)
**		iIGlsaLastStmtAddr replaced by iiIGnsaNextStmtAddr.
**		(For bug 34846).
*/
IL *
iiIGnsaNextStmtAddr()
{
	return il_list->il_ops;
}

/*{
** Name:	IGgenExpr() -	Generate IL Expression Statement.
**
** Description:
**	Generates an internal IL statement for an IL expression.
**
** Input:
**	operator	{ILOP}  IL operator.
**	result		{ILREF}  Reference to boolean result for IL_LEXPR.
**
** History:
**	09/86 (jhw) -- Written.
**	09/20/92 (emerson)
**		Revisions for bug 34846: The logic for maintaining the circular
**		list of IL instructions has been modified to support "fragments"
*/
VOID
IGgenExpr (operator, result)
ILOP	operator;
IL	result;
{
	IL_STMT	*il = il_list;
	ILEXPR	*expr;

	expr = IGendExpr();

	/*
	** Build an IL element on top of the current anchor.
	*/
	il->il_op = operator;
	il->il_expr = expr->il_code;
        il->il_op1 = (IL)0;
	I4ASSIGN_MACRO(expr->il_size, il->il_op1);
	I4ASSIGN_MACRO(expr->il_last, il->il_op3);
	il_offset += expr->il_size + 5;
	if ((operator&ILM_MASK) == IL_LEXPR)
	{ /* logical expression has boolean result */
		il->il_op5 = result;
		il_offset += 1;
	}

	/*
	** Now allocate a new anchor for current fragment,
	** and splice it into the circular list.
	*/
	il = allocIL();
	il->il_next = il_list->il_next;
	il_list->il_next = il;
	il_list = il;
}

/*
** Name:	IGoutStmts() -	Output IL Statements for
**						Interpreted Frame Object.
** Description:
**	This routine outputs the generated IL code for an interpreted frame
**	object using the IAOM module.  To do so, it translates its internal IL
**	code representation to the external IL code form, and then outputs the
**	IL code by a call to 'IIAMil()'.
**
**	The output generated is the IL that was written into the "default"
**	fragment (the one that was initially open by default), including
**	(recursively) all fragments that were included in it via calls to
**	iiIGifIncludeFragment.
**
** Input:
**	dfile_p		{FILE *}  Debug file pointer.
**
** History:
**	09/86 (jhw) -- Written.
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL), by adding
**	        support for i4 sids.  This is done by seeing if the sid
**	        value is too large to store in a IL operand.  If it is, the
**	        sid value is turned into an i4 constant and the constants ILREF
**	        is stored in the sid's location.  The operand is modified with
**	        ILM_LONGSID.
**	01/11/91 (emerson)
**		Remove 32K limit on line numbers in STHD and LSTHD statements
**		by using "longsid" techniques:  extract i4_to_sid function
**		out of getsid; add new cases for IL_STHD and IL_LSTHD.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		add support for IL_CALLPL, IL_LOCALPROC, and IL_MAINPROC.
**	04/22/91 (emerson)
**		Modifications for alerters (add IL_ACTALERT).
**	05/03/91 (emerson)
**		Modifications for alerters: handle GET EVENT properly.
**		(It needs a special call to LIBQ and thus a special IL op code:
**		IL_GETEVENT).
**	05/07/91 (emerson)
**		Added logic to generate NEXTPROC statement (for codegen).
**	09/20/92 (emerson)
**		Revisions for bug 34846: The logic for maintaining the circular
**		list of IL instructions has been modified to support "fragments"
**		In particular, new logic is needed to handle the IL_INCLUDE,
**		IL_OPEN_ANCHOR, and IL_CLOSED_ANCHOR pseudo-instructions.
**		Also, SID processing changed:
**		IGSID is now (IL_STMT *) internally, instead of i4.
**		Also added IL_QRYROWGET and IL_QRYROWCHK to the appropriate
**		case (for bug 39582).
*/

static IL
i4_to_sid(sidvalue, opr_il)
i4	sidvalue;
IL	*opr_il;
{
	FUNC_EXTERN ILREF	IGsetConst();

	if (sidvalue > MAXI2 || sidvalue < MINI2)
	{
		char	buf[30];

		CVla(sidvalue, buf);
		sidvalue = (i4) IGsetConst(DB_INT_TYPE, buf);
		*opr_il = (*opr_il|ILM_LONGSID);
	}
	return (IL) sidvalue;
}

static IL
iigetsid(il, opr_il)
IL_STMT	*il;
IL	*opr_il;
{
	/*
	** Note: For opaqueness, IGSID is defined as PTR in iltypes.h,
	** but in fact it's (IL_STMT *).  (See IGsetSID).
	** Note that il_sid is of type (IGSID *).
	*/
	IL_STMT *target_il = (IL_STMT *)*il->il_sid;

	/*
	** Note that target_il can be NULL if there were compilation errors.
	** (If an IL listing was requested via the oslsql -t flag,
	** we try to generate IL even if there were compilation errors).
	** Rather than getting a SEGVIO, we arbitrarily return 0.
	*/
	if (target_il == NULL)
	{
		return (IL)0;
	}

	return i4_to_sid(target_il->il_offset - il->il_offset, opr_il);
}

# include <si.h>

VOID
IGoutStmts (dfile_p)
register FILE	*dfile_p;
{
	register IL_STMT	*il;
	register IL		*code;
	IL			*cp;
	STATUS			status;
	register IL             *opr_il; /* Used if opcode must be modified */
        i4			il_size, il_last;

	il_list->il_op = IL_CLOSED_ANCHOR;

	/*
	** Allocate storage for the IL code block.  Get 1 extra IL op
	** so that the IL_CLOSED_ANCHOR pseudo-op that we temporarily stick
	** at the end won't corrupt memory.
	*/
	while ((cp = (IL *)FEreqlng(0, (u_i4) (il_offset+1) * sizeof(*cp),
		FALSE, &status)) == NULL)
	{
		EXsignal(EXFEMEM, 1, ERx("IGoutStmts"));
		/*NOTREACHED*/
	}
	code = cp;

	il = il_list;
	for (;;)
	{
		ILOP    operator;
		i4	stmt_off;
		IGRDESC	*rdesc;
		IL_STMT	*anch_il, *target_il;

		il = il->il_next;
		operator = (il->il_op&ILM_MASK);
		if (il->il_offset != code - cp ||
				(operator < 0 || operator > IL_MAX_PSEUDO_OP))
		{
			char	buf[24];

			(VOID)STprintf( buf,
				ERx("(in routine `IGoutStmts(%d)')"),
				operator );
			EXsignal(EXFEBUG, 1, buf);
			/*NOTREACHED*/
		}
		opr_il = code;
		*code++ = il->il_op;
		switch (operator)
		{
		  case IL_INCLUDE:
			code--;			/* we didn't allocate any space
						** for the INCLUDE */
			stmt_off = il->il_offset;
			anch_il = il->il_child;	 /* -> anchor of included frag*/
			anch_il->il_parent = il; /* link anchor back to parent*/
			il = anch_il->il_next;	/* -> first real IL op in
						** included frag */
			stmt_off -= il->il_offset;
			while (il != anch_il)	/* adjust offsets in included
						** fragment to match parent */
			{
				il->il_offset += stmt_off;
				il = il->il_next;
			}
			il->il_offset += stmt_off;
			continue;		/*skip printing this pseudo-op*/

		  case IL_CLOSED_ANCHOR:
			code--;			/* we didn't allocate any space
						** for the ANCHOR (except for
						** the final one) */
			il = il->il_parent;	/* IL_INCLUDE in parent frag */
			if (il != NULL)		/* if there *was* a parent: */
			{			/* we want to go get the next
						** op after the INCLUDE. */
				continue;	/*skip printing this pseudo-op*/
			}
			goto exitloop;		/* No parent fragment: done! */

		  case IL_LEXPR:
			*code++ = il->il_op5;
			/* fall through ... */
		  case IL_EXPR:	
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = il->il_op3;
			*code++ = il->il_op4;
			I4ASSIGN_MACRO(il->il_op1, il_size);
			I4ASSIGN_MACRO(il->il_op3, il_last);
			MEcopy((PTR) il->il_expr, 
				(il_size * sizeof(IL)), (PTR) code);
			code += il_size;
			break;

		  case IL_IF:
			*code++ = il->il_op1;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_ARR1UNLD:
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_UNLD1:
		  case IL_ARR2UNLD:
			*code++ = il->il_op1;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_UNLD2:
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_DISPLAY:
		  case IL_ENDMENU:
		  case IL_GOTO:
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_MENUITEM:
		  case IL_KEYACT:
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = il->il_op3;
			*code++ = il->il_op4;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_COLENT:
		  case IL_COLACT:
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_FLDENT:
		  case IL_FLDACT:
			*code++ = il->il_op1;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_ACTIMOUT:
			*code++ = il->il_op1;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_VALFLD:
			*code++ = il->il_op1;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_ACTALERT:
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_GETEVENT:
			*code++ = il->il_op1;
			break;

		  case IL_VALIDATE:
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_VALROW:
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_QRYGEN:
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_QRYSINGLE:
		  case IL_QRYROWGET:
		  case IL_QRYROWCHK:
			*code++ = il->il_op1;
			*code++ = (il->il_sid == NULL) ? 0 : iigetsid(il, opr_il);
			break;

		  case IL_QRYNEXT:
			*code++ = il->il_op1;
			*code++ = il->il_op2;
			*code++ = (il->il_sid == NULL) ? 0 : iigetsid(il, opr_il);
			break;

		  case IL_STHD:
			*code++ = i4_to_sid(il->il_line, opr_il);
			break;

		  case IL_LSTHD:
			*code++ = i4_to_sid(il->il_line, opr_il);
			*code++ = il->il_op2;
			break;

		  case IL_CALLPL:
			*code++ = il->il_op1;
			*code++ = iigetsid(il, opr_il);
			break;

		  case IL_LOCALPROC:
			rdesc = (IGRDESC *)il->il_sid;
			*code++ = rdesc->igrd_stacksize_ref;
			*code++ = rdesc->igrd_fdesc_end - rdesc->igrd_fdesc_off;
			*code++ = rdesc->igrd_vdesc_end - rdesc->igrd_vdesc_off;
			*code++ = rdesc->igrd_fdesc_off;
			*code++ = rdesc->igrd_vdesc_off;
			*code++ = rdesc->igrd_dbdv_off;
			*code++ = rdesc->igrd_name_ref;
			target_il = (IL_STMT *)*rdesc->igrd_parent_sid;
			stmt_off = target_il->il_offset - il->il_offset;
			*code++ = i4_to_sid(stmt_off, opr_il);
			break;

		  case IL_MAINPROC:
			rdesc = (IGRDESC *)il->il_sid;
			*code++ = rdesc->igrd_stacksize_ref;
			*code++ = rdesc->igrd_fdesc_end;
			*code++ = rdesc->igrd_vdesc_end;
			*code++ = rdesc->igrd_dbdv_size;
			break;

		  case IL_NEXTPROC:
			*code++ = iigetsid(il, opr_il);
			break;

		  default:
		  {
			register i4    numops = IIILtab[operator].il_stmtsize;

			if (il->il_sid != NULL)
			{
				char	buf[48];

				(VOID)STprintf( buf,
					ERx("(in routine `IGoutStmts(%d)', unhandled SID)"),
					operator );
				EXsignal(EXFEBUG, 1, buf);
				/*NOTREACHED*/
			}
			if (numops > 1)
			{
				*code++ = il->il_op1;
				if (numops > 2)
				{
					*code++ = il->il_op2;
					if (numops > 3)
					{
						*code++ = il->il_op3;
						if (numops > 4)
						{
							*code++ = il->il_op4;
							if (numops > 5)
								*code++ = il->il_op5;
						}
					}
				}
			}
			break;
		  }
		} /* end switch */
		if (dfile_p != NULL)
		{
			IIAMpasPrintAddressedStmt( dfile_p, IIILtab, 
				cp + il->il_offset, cp );
		}
	}
exitloop:
	IIAMil(cp, il_offset);
	FElpdestroy(listp);
	listp = NULL;
}

/*{
** Name:	IGinitSID() -	Return a Unique SID.
**
** Description:
**	Return a unique SID (statement identifier) for use by modules that
**	wish to generate IL statements.  SIDs are used as labels for IL
**	statements in the code generation.
**
** Returns:
**	{IGSID *}  The address of a unique SID.
**
** History:
**	06/86 (jhw) -- Written.
**	09/20/92 (emerson)
**		Copied into this source file from the obsolete igsid.c
**		(and reformatted).  (For bug 34846).
*/
IGSID *
IGinitSID ()
{
	IGSID	*sid;

	while ((sid = (IGSID *)FElpget(sidlp)) == NULL)
	{
		EXsignal(EXFEMEM, 1, ERx("IGinitSID"));
		/*NOTREACHED*/
	}
	*sid = NULL;	/* bug check - must be set before IGoutStmts */
	return sid;
}

/*{
** Name:	IGsetSID() -	Associate an SID with an IL Offset.
**
** Description:
**	Associates the passed in SID with the current IL statement.
**	(On output, references to the SID are replaced by the offset in the IL).
**
** Inputs:
**	sid	{IGSID *}  The address of the SID to be set.
**
** History:
**	06/86 (jhw) -- Written.
**	09/20/92 (emerson)
**		Copied into this source file from the obsolete igsid.c;
**		changed to set the SID to point to the IL_STMT structure
**		for the next statement to be generated in the current fragment.
**		(For bug 34846).
*/
VOID
IGsetSID (sid)
IGSID	*sid;
{
	*sid = (IGSID)il_list;
}
