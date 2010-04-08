/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#ifndef FEtalloc
#define FEtalloc(tag, size, status) FEreqlng((tag), (size), FALSE, (status))
#endif
#include	<lt.h>
#include	<adf.h>
#include	<ade.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	"ilexpr.h"

/**
** Name:	igexpr.c -	OSL Interpreted Frame Object Generator
**					Expression Generator Module.
** Description:
**	Contains the routines that generate an OSL intermediate language
**	expression.  Defines:
**
**	IGgenInstr()	generate IL expression instruction.
**	IGbgnExpr()	begin IL expression compilation.
**
**	(ILG internal)
**	IGendExpr()	end IL expression compilation.
**
** History:
**	Revision 6.0  87/06  wong
**	Changes to use ADE module.
**
**	Revision 5.1  86/09/02  15:34:00  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static ADF_CB	*cb = NULL;

/*}
** Name:	CX_DESC -	ADE Compiled Expression Descriptor.
**
** Description:
**	Describes an ADE compiled expression that is being compiled.
*/

static TAGID    tag = 0 ;

typedef struct
{
    PTR		cx ;
    i4     cxsize ;
} CX_DESC ;

/* Current ADE Compiled Expression */

static CX_DESC	top = { NULL, 0 };

/* Free list of Descriptors */
static FREELIST	*listp = NULL;

/* Stack of Active CX Descriptors */
static LIST     *lstack = NULL;

/*{
** Name:	IGgenInstr() -	Generate IL Expression Instruction.
**
** Description:
**	Generate an ADE compiled expression instruction that is part of an
**	IL expression statement.  Note that the operand bases are IL references,
**	which are offset by one and which must be accounted for before being
**	passed to ADE to compile the instruction.
**
** Input:
**	oper	    {ADI_FI_ID}  The function instance ID for the instruction.
**	nops	    {nat}  The number of operands for the instruction.
**	ops	    {ADE_OPERAND []}  Array of operands.
**
** History:
**	09/86 (jhw) -- Written.
**	02/87 (jhw) -- No longer looks up function instance ID from ADF.
**	07/87 (jhw) -- Account for IL reference offset (by one.)
*/

VOID
IGgenInstr (oper, nops, ops)
ADI_FI_ID   oper;
i4	    nops;
ADE_OPERAND *ops;
{
    register ADE_OPERAND    *oprnd;
    register i4	    cnt;
    i4			    junk;

    if ( top.cx == NULL )
	    EXsignal(EXFEBUG, 1, ERx("IGgenInstr()"));

    /* Account for operand offset for ADE */
    for (oprnd = ops, cnt = nops ; --cnt >= 0 ; ++oprnd)
	oprnd->opr_base -= 1;	/* IL references are offset by one */

    /* Compile instruction */
    while ( afc_instr_gen(cb, top.cx, oper, ADE_SMAIN, nops, ops,
			(i4 *)NULL, &junk) != OK )
    {
	if (cb->adf_errcb.ad_errcode != E_AD5506_NO_SPACE)
	{
	    FEafeerr(cb);
	    EXsignal(EXFEBUG, 1, ERx("IGgenInstr(ADE)"));
	}
	else
	{ /* allocate more space */
	    PTR		new_cx;
	    STATUS	status;

	    while ((new_cx = FEtalloc(tag, top.cxsize + top.cxsize/10,
			&status)) == NULL)
		EXsignal(EXFEMEM, 1, ERx("IGgenInstr"));
	    MEcopy(top.cx, (u_i2)top.cxsize, new_cx);
	    top.cxsize += top.cxsize/10;
	    top.cx = new_cx;
	    if (afc_inform_space(cb, top.cx, top.cxsize) != OK)
	    {
		FEafeerr(cb);
		EXsignal(EXFEBUG, 1, ERx("IGgenInstr(realloc)"));
	    }
	}
    } /* end while */
}

/*{
** Name:	IGbgnExpr() -	Begin IL Expression Compilation.
**
** Description:
**	Begin IL expression compilation by determining required space
**	for ADE compiled expression and allocating.
**
** Input:
**	ninstr	{nat}  Number of instructions.
**	nops	{nat}  Number of operands.
**
** History:
**	09/86 (jhw) -- Written.
**	03/87 (jhw) -- Modified to use ADE module.
*/
VOID
IGbgnExpr (ninstr, nops)
i4	ninstr;
i4	nops;
{
    STATUS	status;

    ADF_CB	*FEadfcb();

    /* Space reclaimation:
     *
     *    An effort was made to reclaim expression space pointed to by
     * "cx" using a call to IIUGtagFree but it was found to be impossible 
     * because code generation does not necessarily occur immediately 
     * after a call to IGendExpr.  The space used to keep track of bgn/end
     * nesting IS reclaimed.
     */

    if ( tag == 0 )
       tag = FEgettag();

    if ( cb == NULL )
	cb = FEadfcb();

    if ( top.cx != NULL )
    { /* push */
	CX_DESC	*new;

	/* Create a "stack frame" for this CX descriptor and push it onto the
	 * stack (lstack) after saving the current descriptor for later recall.
	 * We "nest" only if IGbgnExpr is called a second time before calling
	 * IGendExpr.
	 */

	if ( ( listp == NULL && (listp = FElpcreate(sizeof(*new))) == NULL ) ||
			(new = (CX_DESC *)FElpget(listp)) == NULL )
		EXsignal(EXFEMEM, 1, ERx("IGbgnExpr()"));

	STRUCT_ASSIGN_MACRO(top, *new);
	LTpush( &lstack, (PTR)new, 0 ) ;
    }

    top.cxsize = 0 ;

    if (afc_cx_space(cb, ninstr, nops, 0, 0, &top.cxsize) != OK)
    {
	FEafeerr(cb);
	EXsignal(EXFEBUG, 1, ERx("IGbgnExpr(cx space)"));
    }

    while ((top.cx = FEtalloc(tag, top.cxsize, &status)) == NULL)
	EXsignal(EXFEMEM, 1, ERx("IGbgnExpr"));

    if (afc_bgn_comp(cb, top.cx, top.cxsize) != OK)
    {
	FEafeerr(cb);
	EXsignal(EXFEBUG, 1, ERx("IGbgnExpr()"));
    }
}

/*{
** Name:	IGendExpr() -	End IL Expression Compilation.
**
** Description:
**	End IL expression compilation and return address of IL expression
**	code (ADF compiled expression) and size by reference.
**
** Returns:
**	{ILEXPR *}  Reference to structure describing IL expression.
**
** History:
**	09/86 (jhw) -- Written.
**	03/87 (jhw) -- Modified to use ADE module.
*/

static ILEXPR	expr ZERO_FILL;

ILEXPR *
IGendExpr ()
{
    PTR		start[3];
    i4		end[3];
    i4	firstoff[3];
    i4	lastoff[3];
    i4	junk;

    if ( top.cx == NULL )
    {
   	/* An error has occurred: a call to "bgn"
	* was not made before a call to "end".
	*/
	FEafeerr(cb);
	EXsignal(EXFEBUG, 1, ERx("IGendExpr(no bgn)"));
    }

    if (afc_cxinfo(cb, top.cx, ADE_11REQ_1ST_INSTR_ADDRS, (PTR) start) != OK
      || afc_cxinfo(cb, top.cx, ADE_12REQ_FE_INSTR_LIST_I2S, (PTR) end) != OK
      || afc_cxinfo(cb, top.cx, ADE_10REQ_1ST_INSTR_OFFSETS, (PTR) firstoff) 
        != OK
      || afc_cxinfo(cb, top.cx, ADE_14REQ_LAST_INSTR_OFFSETS, (PTR) lastoff)
        != OK
      || afc_end_comp(cb, top.cx, &junk) != OK)
    {
	FEafeerr(cb);
	EXsignal(EXFEBUG, 1, ERx("IGendExpr()"));
    }
    expr.il_code = (IL *)start[ADE_SMAIN-1];
    expr.il_size = end[ADE_SMAIN-1] + end[ADE_SINIT-1] + end[ADE_SFINIT-1];
    expr.il_last = (i4)(lastoff[ADE_SMAIN-1] - firstoff[ADE_SMAIN-1]);

    if ( lstack == NULL )
    {
	top.cx = (PTR) NULL ;
	top.cxsize = 0 ;
    }
    else
    {
	register CX_DESC  *LastFrame ;

	if ( (LastFrame = (CX_DESC *) LTpop( &lstack )) != NULL )
	{
		STRUCT_ASSIGN_MACRO(*LastFrame, top);
		FElpret(listp, (PTR)LastFrame);
	}
    }

    return &expr;
}
