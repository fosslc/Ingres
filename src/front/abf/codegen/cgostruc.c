/*
** Copyright (c) 1987, 2008 Ingres Corporation
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
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<iltypes.h>
#include	<ilrffrm.h>

#include	"cggvars.h"
#include	"cgout.h"
#include	"cgerror.h"


/**
** Name:	cgstruct.c -	Code Generator C Control Structures Module.
**
** Description:
**	The generation section of the code generator for outputting
**	C control structures.  Defines:
**
**	IICGoleLineEnd()
**	IICGosbStmtBeg()
**	IICGoseStmtEnd()
**	IICGostStructBeg()
**	IICGosdStructEnd()
**	IICGoeeEndEle()
**	IICGobbBlockBeg()
**	IICGobeBlockEnd()
**	iicgControlBegin()
**	iicgCntrlBlock()
**	IICGofxForExpr()
**	IICGoicIfCond()
**
** History:
**	Revision 6.0  87/02  arthur
**	Initial revision.  Adapted from "newosl/olstruct.c."
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	19-dec-96 (chech02)
**	    Removed READONLY attrib in GLOBALREFs.
**      05-feb-1997 (hanch04)
**              Changed GLOBALREFs to GLOBALCONSTREF.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/*
** Name:	_IICGcontrol -	C Control Statement Names.
*/
GLOBALCONSTREF char	_IICGif[]	;
GLOBALCONSTREF char	_IICGelseif[]	;
GLOBALCONSTREF char	_IICGfor[]	;
GLOBALCONSTREF char	_IICGwhile[]	;


static i4	forexpr = 0;		/* 0 for 1st expr in 'for' block hdr */

/* table of operators for conditions */
static const char	*cgRelOpTable[] = {
			    ERx("  "),	/* NOOP */
			    ERx(" == "),
			    ERx(" != "),
			    ERx(" < "),
			    ERx(" > "),
			    ERx(" <= "),
			    ERx(" >= ")
};

/*{
** Name:    IICGoleLineEnd() -	End a line
**
** Description:
**	Ends a line which is not the end of a C statement.
**
** History:
**	20-feb-1987 (agh)
**		First written.
*/
VOID
IICGoleLineEnd()
{
    IICGoprPrint(ERx("\n"));
    IICGotlTotLen = 0;
    return;
}

/*
** C statements
*/

/*{
** Name:    IICGosbStmtBeg() -	Begin a C statement
**
** Description:
**	This is the start of a C statement.
**	Indent to the proper level.
*/
VOID
IICGosbStmtBeg()
{
    IICGoprPrint(IICGoiaIndArray);
    return;
}

/*{
** Name:    IICGoseStmtEnd() -	End a C statement
**
** Description:
**	End a C statement.
*/
VOID
IICGoseStmtEnd()
{
    IICGoprPrint(ERx(";\n"));
    IICGotlTotLen = 0;
    return;
}

/*{
** Name:    IICGostStructBeg() -	Begin a structure
**
** Description:
**	Begin a structure.  If the 'num_eles' argument is non-zero,
**	the elements of the structure are to be initialized.
**
** Inputs:
**
**	type		The type of structure
**	name		The name of the structure
**	num_eles	The number of elements.  If non-zero, an array
**			of structures of known size is to be initialized.
**	init		CG_INIT if structure is to be initialized and size
**			is known, CG_ZEROINIT if initialized but size is
**			not known, or CG_NOINIT if not to be initialized.
**
*/
VOID
IICGostStructBeg(type, name, num_eles, init)
char	*type;
char	*name;
i4	num_eles;
i4	init;
{
    char	buf[CGBUFSIZE];

    IICGoprPrint(STprintf(buf, ERx("%s%s %s"), IICGoiaIndArray, type, name));
    if (num_eles > 0)
    {
	_VOID_ STprintf(buf, (init == CG_INIT) ? ERx("[%d] = {") : ERx("[%d]"),
			num_eles
	);
    }
    else
    {
	if (init == CG_INIT)
	    STcopy(ERx(" = {"), buf);
	else if (init == CG_ZEROINIT)
	    STcopy(ERx("[] = {"), buf);
	else
	    buf[0] = EOS;
    }
    IICGoprPrint(buf);

    if (init == CG_INIT || init == CG_ZEROINIT)
    {
	IICGoleLineEnd();
	IICGoinIndent(1);
    }
    else
    {
	IICGoseStmtEnd();
    }
    return;
}


/*{
** Name:    IICGosdStructEnd() -	End a structure initialization
**
** Description:
**	Ends a structure initialization.
**
** Inputs:
**	None.
**
*/
VOID
IICGosdStructEnd()
{
    char	buf[CGSHORTBUF];

    IICGoinIndent(-1);
    IICGoprPrint(STprintf(buf, ERx("%s}"), IICGoiaIndArray));
    IICGoseStmtEnd();
    return;
}

/*{
** Name:    IICGoeeEndEle() -	Close an element in an array initialization
**
** Description:
**	Closes an element in an array initialization.
**
** Inputs:
**
**	last		CG_LAST if this is the last element in the array,
**			else CG_NOTLAST.
**
*/
VOID
IICGoeeEndEle(last)
i4	last;
{
    if (last == CG_NOTLAST)
	IICGoprPrint(ERx(","));
    IICGoleLineEnd();
    return;
}

/*{
** Name:    IICGobbBlockBeg() -	Begin a statement block
**
** Description:
**	Begins a C statement block.
*/
VOID
IICGobbBlockBeg()
{
    char	buf[CGSHORTBUF];

    IICGoprPrint(STprintf(buf, ERx("%s{"), IICGoiaIndArray));
    IICGoleLineEnd();
    IICGoinIndent(1);
    return;
}

/*{
** Name:    IICGobeBlockEnd() -	End a statement block
**
** Description:
**	Ends a C statement block.
**
** Inputs:
**	None.
**
*/
VOID
IICGobeBlockEnd()
{
    char	buf[CGSHORTBUF];

    IICGoinIndent(-1);
    IICGoprPrint(STprintf(buf, ERx("%s}"), IICGoiaIndArray));
    IICGoleLineEnd();
    return;
}

/*{
** Name:	iicgControlBegin() -	Begin a C Control Statement.
**
** Description:
**	Begins a C WHILE, IF, or FOR statement.
**
** Inputs:
**	control	{char *}  Control statement name.
**
** History:
**	12/87 (jhw) -- Written.
*/
VOID
iicgControlBegin ( control )
char	*control;
{
    char	buf[CGSHORTBUF];

    IICGoprPrint(STprintf(buf, ERx("%s%s ("), IICGoiaIndArray, control));
    forexpr = 1;
}

/*{
** Name:	iicgCntrlBlock() -	Begin the Statement block for
**						a C Control Structure.
** Description:
**	Begins the statement block for a C control structure such as WHILE,
**	IF, or FOR by closing the control condition and beginning a block.
**
** Inputs:
**	None.
**
** History:
**	12/87 (jhw) -- Abstracted from duplicated control statement code.
*/
VOID
iicgCntrlBlock()
{
    IICGoprPrint(ERx(")"));
    IICGoleLineEnd();
    IICGobbBlockBeg();
}

/*{
** Name:    IICGofxForExpr() -	Generate expr for a 'for' stmt header
**
** Description:
**	Generates an expression for a 'for' statement header.
**	Uses the static 'forexpr'.
**
** Inputs:
**	expr		The expression as a string.
**
*/
VOID
IICGofxForExpr(expr)
char	*expr;
{
    char	buf[CGBUFSIZE];

    if (forexpr != 1)
	_VOID_ STprintf(buf, ERx("; %s"), expr);
    else
	STcopy(expr, buf);
    IICGoprPrint(buf);
    forexpr++;
}

/*{
** Name:    IICGoicIfCond() -	Condition for an 'if' statement
**
** Description:
**	Generate code for the condition within an 'if' statement.
**
** Inputs:
**	lhs	The left-hand side of the condition as a character string.
**		NULL if the LHS has already been generated by the caller.
**	op	The operator.  One of CG_EQ, CG_NOTEQ, etc., which are
**		indexes into the operator table.
**	rhs	The right-hand side of the condition.  As with lhs, may
**		be NULL.
**
*/
VOID
IICGoicIfCond(lhs, op, rhs)
char	*lhs;
i4	op;
char	*rhs;
{
    char	buf[CGSHORTBUF];

    if (lhs != NULL)
	IICGoprPrint(lhs);
    IICGoprPrint(cgRelOpTable[op]);
    if (rhs != NULL)
	IICGoprPrint(rhs);
    return;
}

/*{
** Name:    IICGoebElseBlock() -	Begin ELSE Portion of an IF Statement.
**
** Description:
**	Generate C code to begin the ELSE portion of an IF statement.
**
** Inputs:
**	None.
**
*/
VOID
IICGoebElseBlock()
{
    char	buf[CGSHORTBUF];

    IICGoprPrint(STprintf(buf, ERx("%selse"), IICGoiaIndArray));
    IICGoleLineEnd();
    IICGobbBlockBeg();
    return;
}
