/*
** Copyright (c) 1987, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
#include        <lo.h>
#include	<si.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
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
#include	"cgout.h"
#include	"cglabel.h"
#include	"cgerror.h"


/**
** Name:	cglabel.c -		Code Generator Labels Module.
**
** Description:
**	Manages the label stack and generates C code for labels.  Defines:
**
**	IICGlbiLblInit()
**	IICGlebLblEnterBlk()
**	IICGlarLblActReset()
**	IICGlexLblExitBlk()
**	IICGlnxLblNext()
**	IICGpxlPrefixLabel()
**	IICGpxgPrefixGoto()
**	IICGlblLabelGen()
**	IICGgtsGotoStmt()
**	IICGlitLabelInsert()
**	IICGlrqLabelRequired()
**	IICGesEvalSid()
**
** History:
**	Revision 6.0  87/03  arthur
**	Initial revision.
**
**	Revision 6.1  88/09  wong
**	Reimplemented 'IICGlcrLabelCurrent()' as macro and modified to
**	support new mode CGLM_FORM0.  Also, moved local structures and
**	definitions here from "cglabel.h".
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**		Also add new function IICGesEvalSid.
**
**	Revision 6.4/02
**	04/25/92 (emerson)
**		One more change in support of > 64K bytes of IL:
**		When doing STprintf's that build a statement label,
**		pass the statement offset as a i4 instead of an int,
**		and change the template to use %ld instead of %d.
**		[The old code would have caused problems generating C code
**		from code blocks that are bigger than 64K bytes on platforms
**		with 16-bit integers, but there may well be further
**		problems in such a scenario, e.g. architectural limitations 
**		in the platform's hardware or C compiler].
**
**	Revision 6.5
**	29-jun-92 (davel)
**		Minor change - added new argument to call to IIORcqConstQuery().
**	13-dec-93 (smc)
**		Bug #58882
**              Dummied out Hash cast when linting to avoid a plethora of
**              lint truncation warnings on machines where i4 < ptr,
**		which can safely be ignored.
**	05-Jun-96 (mckba01)
**		Bug #48589
**		Changed _CurrentLabel macro so  that mode CGLM_FORM0		
**              returns the correct label value for local procedures.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*}
** Name:	CGL_STACK -	Label stack.
*/
typedef struct {
#define	CGL_MODENUM	2
#define	CGL_STKSIZE	10	/* maximum depth of the label stack */
	struct lbl_stack
	{
		i2	sav_cur[CGL_MODENUM]; /* one for each of the 2 modes */
		i2	stk_mode;	/* which mode is "current" */
	} lbl_stack[CGL_STKSIZE];	/* the stacked info */
	struct lbl_cur
	{
		i2	cur_cur;	/* current value */
		i2	cur_high;	/* hi-water mark */
	} lbl_cur[CGL_MODENUM];		/* one for each of the 2 modes */
} CGL_STACK;

/*}
** Name:	CG_LBL_STRUCT -	Marks an IL statement requiring a label
**
** Description:
**	Structure used to mark an IL statement requiring a label.
**	The 'cgl_mode' member indicates whether a C block should be
**	closed before the label is generated.
*/
typedef struct cg_lbl_struct
{
	IL			*cgl_stmt;
	i4			cgl_mode;	/* whether to close a block */
	struct cg_lbl_struct	*cgl_next;
} CG_LBL_STRUCT;

/*
** Size of the hash table for labels.
*/
# define	CGHASHSIZE	31

/*
** Name:	Hash() -	Return a hash value
**
** Description:
**	Returns a hash value for a hash table of size CGHASHSIZE.
**
** Inputs:
**	stmt		The IL statement.
**
** Returns:
**	{u_i4}  Hash value.
*/
#ifndef LINT
#define Hash(stmt) ((u_i4)((u_i4)(SCALARP)(stmt) % CGHASHSIZE))
#else
#define Hash(stmt) 0
#endif /* LINT */

/*
** Data structures private to this module:  (1) the label stack;
** (2) the hash table of IL statements which require labels to be generated;
** (3) a pointer to the beginning of the IL array for this frame.
*/
static CGL_STACK	cgl_labels ZERO_FILL;	/* the label stack */
static i2		cgl_index = 0;		/* index of next free 
							** location in stk */

static FREELIST		*cgl_pool = NULL;	/* allocation pool */
static CG_LBL_STRUCT	*CGLhashTab[CGHASHSIZE]; /* hash table of IL stmts */

static IL		*cgl_iltop;		/* top of IL array for frame */

/* 
** Table of label names - correspond to CGL_xxx constants.
** Must be unique in the first few characters, to allow label uniqueness
** in 8 characters.
*/
static const
	char	*cgLblTable[] = {
			ERx("IIl_"),	/* should never be referenced */
			ERx("IIfdI"),
			ERx("IIfdB"),
			ERx("IIfdF"),
			ERx("IIfdE"),
			ERx("IImuI"),
			ERx("IIosl"),
			(char *) NULL
};

static const char	_Label[]	= ERx("%s%ld: ");
static const char	_Goto[]		= ERx("goto %s%ld");

/*{
** Name:	IICGlbiLblInit() -	Initialize local data structures.
**
** Description:
**	Initializes the local data structures for this module:
**	(1) the label stack; (2) the hash table of IL statements which
**	require labels to be generated; (3) the pool of CG_LBL_STRUCT
**	structures; and (4) the beginning of the IL array for this frame.
**
** History:
**	4-mar-1987 (agh)
**		First written, based on equel/label.c.
*/
VOID
IICGlbiLblInit()
{
	register CG_LBL_STRUCT	**hp;
	register i4		i;

	cgl_index = 0;
	MEfill(sizeof(cgl_labels), '\0', &cgl_labels);

	for ( i = 0, hp = CGLhashTab ; i < CGHASHSIZE ; ++i )
		*hp++ = (CG_LBL_STRUCT *) NULL;

	if ( (cgl_pool = FElpcreate(sizeof(CG_LBL_STRUCT))) == NULL )
		IICGerrError(CG_LABEL, ERx("label pool creation"), (char *)NULL);

	cgl_iltop = CG_il_stmt;
}


/*{
** Name:	IICGlebLblEnterBlk() -	Start a new block for labels.
**
** Description:
**	Starts a new block for labels.  Saves all the global information,
**	and pushes a new block onto the label stack.
*/
VOID
IICGlebLblEnterBlk()
{
	register struct lbl_stack	*l = &cgl_labels.lbl_stack[cgl_index];
	register int			i;

	l->stk_mode = CGL_0;
	for ( i = 0 ; i < CGL_MODENUM ; ++i )
	{
		l->sav_cur[i] = cgl_labels.lbl_cur[i].cur_cur;
	}

	if ( ++cgl_index >= CGL_STKSIZE )
		IICGerrError(CG_LABEL, ERx("label stack overflow"), (char *)NULL);
}

/*{
** Name:	IICGlarLblActReset() -	Reset for activate blocks.
**
** Description:
**	Resets the high-water mark for activate blocks.
**	In the next display or submenu loop, therefore, the
**	activations will start being numbered at 1.
*/
VOID
IICGlarLblActReset()
{
	register struct lbl_cur	*p = &cgl_labels.lbl_cur[CGLM_ACTIV];

	p->cur_cur = 0;
	p->cur_high = 0;
}

/*{
** Name:	IICGlexLblExitBlk() -	Exit a label block.
**
** Description:
**	Exits a label block (pops it) and restores the previous state.
*/
VOID
IICGlexLblExitBlk()
{
	register struct lbl_stack	*l;
	register struct lbl_cur		*p;
	register i4			i;

	if (--cgl_index < 0)
		IICGerrError(CG_LABEL, ERx("label stack underflow"), (char *) NULL);

	l = &cgl_labels.lbl_stack[cgl_index];
	for ( i = 0 ; i < CGL_MODENUM ; ++i )
	{
		cgl_labels.lbl_cur[i].cur_cur = l->sav_cur[i];
	}
	/*
	** Restore highwater for activation values.
	*/
	p = &cgl_labels.lbl_cur[CGLM_ACTIV];
	p->cur_high = p->cur_cur;
}

/*{
** Name:	IICGlnxLblNext() -	Return next label number.
**
** Description:
**	Returns the next label number of the specified mode.
**	Increments the high-water mark, and returns it as the current value.
**
** Inputs:
**	mode	{LBL_MODE}  Mode of label whose next number we want.
**				Note:  CG_FORM0 is not valid in this context.
**
** Returns:
**	{nat}  The next label number.
*/
i4
IICGlnxLblNext(mode)
i4	mode;
{
	register struct lbl_cur		*l = &cgl_labels.lbl_cur[mode];

	if ( mode == CGLM_FORM0 )
	{
		IICGerrError(CG_LABEL, ERx("IICGlnxLblNext(bad mode)"),
				(char *)NULL
		);
		/*NOTREACHED*/
	}
	return (l->cur_cur = ++(l->cur_high));
}

/*
** Name:	_currentLabel() -	Return current label number.
**
** Description:
**	Returns the current label number for the specified mode.
**
** Inputs:
**	mode	{LBL_MODE}  Mode of label whose current value we want.
**
** Returns:
**	{nat}	Current label number.
**
** History:
**	05/87 (agh) -- Written.
**	09/88 (jhw) - Renamed from 'IICGlcrLabelCurrent()' and reimplemented
**			as macro adding support for CGLM_FORM0 to get base
**			form display label for RETFRM and EXIT C code.
**	06/96 (mckba01) - for local procedures mode CGLM_MODE0 now
**			  returns The first "[0]" saved value ".sav_cur" on 
**			  the stack if cgl_index > 0 otherwise the current 
**			  value ".cur_cur" is returned. Used NULL check on
**			  CG_form_name to test for local procedure. 
*/
#define _currentLabel( mode ) \
( \
	( (mode) == CGLM_FORM0 ) \
		? ((CG_form_name == NULL) \
		  	? (( cgl_index > 0) \
                                ? cgl_labels.lbl_stack[0].sav_cur[CGLM_FORM] \
                                : cgl_labels.lbl_cur[CGLM_FORM].cur_cur ) \
			: (( cgl_index > 1 ) \
		 		? cgl_labels.lbl_stack[1].sav_cur[CGLM_FORM] \
				: cgl_labels.lbl_cur[CGLM_FORM].cur_cur )) \
		: cgl_labels.lbl_cur[(mode)].cur_cur \
)

/*{
** Name:	IICGpxlPrefixLabel() -	Generate a C label with a given prefix.
**
** Description:
**	Generates a C label with a given prefix.
**
** Inputs:
**	prefix	{LBL_TYPE}  Mode of the prefix of the label.
**	suffix	{LBL_MODE}  Mode of the suffix.
*/
VOID
IICGpxlPrefixLabel(prefix, suffix)
i4	prefix;
i4	suffix;
{
	char	buf[CGSHORTBUF];

	IICGoleLineEnd();	/* return to left margin */
	IICGoprPrint( STprintf( buf, _Label, cgLblTable[prefix],
				(i4)_currentLabel(suffix)
			)
	);
	/*
	** Generate a semi-colon to ensure that a C statement
	** immediately follows a label.
	*/
	IICGoseStmtEnd();
}

/*{
** Name:	IICGpxgPrefixGoto() -	'goto' statement to a specified prefix.
**
** Description:
**	Generates a C 'goto' statement to a specified prefix.
**
** Inputs:
**	prefix	{LBL_TYPE}  Mode of the prefix of the label.
**	suffix	{LBL_MODE}  Mode of the suffix.
*/
VOID
IICGpxgPrefixGoto(prefix, suffix)
i4	prefix;
i4	suffix;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGoprPrint( STprintf( buf, _Goto, cgLblTable[prefix],
				(i4)_currentLabel(suffix)
			)
	);
	IICGoseStmtEnd();
}

/*
** Name:	BlkEndReq() -	Must C blocks be closed before a label?
**
** Description:
**	Returns the number of C blocks which must be closed before a label
**	is output.
**	This action is required at the end of blocks opened for queries:  once
**	for master queries and twice for master-detail queries.
**
** Inputs:
**	stmt	{IL *}  The current IL statement.
**
** Returns:
**	{nat}  The number of blocks to close before outputting the next label.
*/
static i4
BlkEndReq(stmt)
register IL	*stmt;
{
    register CG_LBL_STRUCT	*elm;
    i4				nblocks = 0;

    for ( elm = CGLhashTab[Hash(stmt)] ; elm != NULL ; elm = elm->cgl_next )
    {
	if (elm->cgl_stmt == stmt)
	{
	    if (elm->cgl_mode == CGLM_BLKEND)
		nblocks = 1;
	    else if (elm->cgl_mode == CGLM_MDBLKEND)
		nblocks = 2;
	    break;
	}
    }
    return nblocks;
}

/*{
** Name:	IICGlblLabelGen() -	Generate a C label.
**
** Description:
**	Generates a C label, with the standard prefix "IIosl".
**	The suffix of the label is the offset of the current IL statement
**	from the beginning of the IL for this frame.
**	If necessary, closes a C block before outputting the label
**	(this is required at the end of blocks opened for queries).
**
** Inputs:
**	stmt	{IL *}  The current IL statement.
*/
VOID
IICGlblLabelGen(stmt)
IL	*stmt;
{
	register i4	blks;	 /* no. of blocks to close
					before outputting label */
	char	buf[CGSHORTBUF];

	/*
	** Close 1 or 2 C blocks opened for a simple for master-detail
	** query, if required.
	*/
	for ( blks = BlkEndReq(stmt) ; blks > 0 ; --blks )
		IICGobeBlockEnd();
	IICGoleLineEnd();	/* return to left margin */
	IICGoprPrint( STprintf(buf, _Label, cgLblTable[CGL_OSL],
				(i4)(stmt - cgl_iltop)
			)
	);
	/*
	** Generate a semi-colon to ensure that a C statement
	** immediately follows a label.
	*/
	IICGoseStmtEnd();
}

/*{
** Name:	IICGgtsGotoStmt() -	Generate a C 'goto' statement.
**
** Description:
**	Generates a C 'goto' statement.  The label has the standard
**	prefix "IIosl".  The suffix is the offset of the IL statement
**	to 'go to' from the beginning of the IL array for this frame.
**
** Inputs:
**	stmt	{IL *}  The IL statement to which to go.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL):
**		Simplify things by changing interface to pass in 
**		target statement instead of current statement and offset to
**		target statement.
*/
VOID
IICGgtsGotoStmt(stmt)
IL	*stmt;
{
	char	buf[CGSHORTBUF];

	IICGosbStmtBeg();
	IICGoprPrint( STprintf(buf, _Goto, cgLblTable[CGL_OSL],
				(i4)(stmt - cgl_iltop)
			)
	);
	IICGoseStmtEnd();
}

/*{
** Name:	IICGlitLabelInsert() -	Insert IL stmt into the label hash table.
**
** Description:
**	Inserts an IL statement into the hash table of those requiring labels.
**	This routine does nothing if the statement has already been entered.
**
** Inputs:
**	stmt	{IL *}  IL statement to be inserted.
**	mode	{LBL_END}  End mode for the label:  either CGLM_NOBLK or
**			CGLM_BLKEND.  The latter indicates that, in the
**			generated code, a C block should be closed before
**			the label is output.
*/
VOID
IICGlitLabelInsert(stmt, mode)
register IL	*stmt;
i4		mode;
{
    register CG_LBL_STRUCT	*elm;
    register i4		hashval;

    hashval = Hash(stmt);
    for ( elm = CGLhashTab[hashval] ; elm != NULL ; elm = elm->cgl_next )
    {
	if (elm->cgl_stmt == stmt)
	    return;		/* already in array of IL statements */
    }

    if ( (elm = (CG_LBL_STRUCT *) FElpget(cgl_pool)) == NULL )
    {
	IICGerrError(CG_LABEL, ERx("label list insertion"), (char *) NULL);
    }
    else
    {
	elm->cgl_stmt = stmt;
	elm->cgl_mode = mode;
	elm->cgl_next = CGLhashTab[hashval];
	CGLhashTab[hashval] = elm;
    }
}

/*{
** Name:	IICGlrqLabelRequired() - Is a label required for this IL stmt?
**
** Description:
**	Returns whether a label is required for the current IL statement.  If a
**	label IS required, then a reference to this statement will already have
**	been entered into the linked list of IL statements.
**
** Inputs:
**	stmt	{IL *}  The current IL statement.
**
** Returns:
**	{bool}	TRUE if label is required.
*/
bool
IICGlrqLabelRequired(stmt)
register IL	*stmt;
{
    register CG_LBL_STRUCT	*elm;
    IL              		il_op;

    /*
    ** A few statements must output some C code BEFORE generating
    ** a label.  These statements will explicitly handle label
    ** generation themselves.
    */
    il_op = *stmt & ILM_MASK;
    if ( il_op == IL_BEGMENU || il_op == IL_BEGDISPMU || il_op == IL_BEGSUBMU )
	return FALSE;

    for ( elm = CGLhashTab[Hash(stmt)] ; elm != NULL ; elm = elm->cgl_next )
    {
	if (elm->cgl_stmt == stmt)
	{
	    return TRUE;
	}
    }
    return FALSE;
}

/*{
** Name:	IICGesEvalSid() - Evaluate a SID.
**
** Description:
**	Returns the address of the IL statement to which the current
**	IL statement refers.
**
** Inputs:
**	stmt	{IL *}  The current IL statement.
**	sid	{IL}	The operand from the current IL statement
**			which contains a short SID or the negation
**			of the index of an integer constant
**			representing a long SID.  This SID is the offset
**			(from the current IL statement) of the statement
**			to which the current IL statement refers.
**
** Returns:
**	{IL *}	The address of the IL statement to which the current
**		IL statement refers.
**
** History:
**	01/09/91 (emerson)
**		Created.
**	29-jun-92 (davel)
**		Added new argument to call to IIORcqConstQuery().
*/
IL *
IICGesEvalSid(stmt, sid)
register IL	*stmt;
IL		sid;
{
	if (*stmt & ILM_LONGSID)
	{
		PTR	rval = NULL;

		_VOID_ IIORcqConstQuery(&IICGirbIrblk, -(sid), 
					DB_INT_TYPE, &rval, (i4 *)NULL);
		stmt += *((i4 *) rval);
	}
	else
	{
		stmt += sid;
	}
	return stmt;
}
