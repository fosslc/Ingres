/*
** Copyright (c) 2008, 2010 Ingres Corporation
**
*/

/**
**
**  Name: adupatcomp.c - Routines related to pattern compiling
**
**  Description:
**	This file contains routines to handle the parsing of input pattern and
**	the compiling of this into a pattern stream.
**
** This file defines the following externally visible routines:
**
**  s	find_op()		Look for an operator
**  o	trace_op()		Dissassemble 1 opt to trace
**  s	pat_bld_init()		Use to initialise pattern list
**  s	chk_space()		Ensure enough space in the pattern buffer
**  s	pat_op_simple()		Append trivial OP
**  s   pat_op_1()		Append single param OP
**  s   pat_op_2()		Append dual param OP
**  s	pat_bld_newpat()	Start a new pattern
**  s	pat_bld_finpat()	Complete pattern frame in list
**  s	pat_update_num()	Adjust a numeric pattern operand
**  s	pat_insert_num()	Insert a numeric pattern operand
**  s	pat_op_lit()		Flush any pending literal text (including utf8)
**  s	pat_op_ulit()		Flush any Unicode literal
**  s	pat_op_CElit()		Flush any Unicode literal as CE elements
**  s	pat_op_any_one()	Generate PAT_ANY_1 
**  s	pat_op_wild()		Generate PAT_ANY_0_W
**  s	pat_op_bitset()		Generate PAT_[N]BITSET_1
**  s	pat_op_set()		Generate PAT_[N]SET_1
**  s	pat_op_uset()		Generate a non-CE Unicode PAT_[N]SET_1
**  s	pat_op_CEset()		Generate a CE list form PAT_[N]SET_1
**  s	pat_set_add()		Add set element
**  s   pat_rep_push()		Push repeat context
**  s   pat_rep_pop()		Pop repeat context
**  s   pat_set_case()		Set case entry in repeat context
**  s   pat_apply_repeat()	Set regexp range params into operator
**  o   adu_pat_decode()	Decode operator
**  o	adu_pat_disass()	Disassemble pattern operator
**  o	pat_dump()		Dump compiled pattern to an ASCII file
**  o	pat_dbg()		Dump compiled pattern to trace for dbg call
**  o	pat_load()		Load pattern from file
**	adu_patcomp_set_pats()	Is provided as an entry point as it allows for a
**				pattern buffer data type to be defined that could
**				be used to move the pattern compilation into parse time
**				with the pattern opcodes being in the CX data.
**	adu_patcomp_free()	Release compiled pattern resources
**  s	prs_num*()		Parse base 10 unsigned integer
**  s	cmp_*()			Compare routines for use with pat_set_add()
**	adu_patcomp_like()	Build non-unicode LIKE pattern
**	adu_patcomp_like_uni()	Build unicode LIKE pattern
**	adu_patcomp_kbld()	Build non-unicode pattern key
**	adu_patcomp_kbld_uni()	Build unicode pattern key
**	adu_patcomp_summary()	Summarise 
**
**
** (s)tatic, (o)ptionally compiled
**
** Function prototypes for the external routines are defined in
** ADUINT.H file.
**
**  History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**      04-Sep-2008 (kiria01) SIR120473
**          Reduced MEreqmem overhead by running in stack memory. Particularly
**	    the pattern buffer which for future work will need to have its length
**	    bounded and memory pre-allocated.
**	07-Oct-2008 (kiria01) SIR120473
**	    Switched the MEmemset calls to MEfill as the formed not defined for
**	    VMS.
**	27-Oct-2008 (kiria01) SIR120473
**	    Introduce AD_PATDATA having dropped from u_i4 vector
**	    to an u_i2 form that fits in with VLUP arrangements.
**	    Added adu_patcomp_kbld and adu_patcomp_kbld_uni.
**	12-Dec-2008 (kiria01) SIR120473
**	    Address alignment issue on Solaris with the i4 flags in the
**	    patdata structure.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	30-Dec-2008 (kiria01) SIR120473
**	    Added support for PAT_FNDLIT to speed up literal scans.
**	06-Jan-2009 (kiria01) SIR120473
**	    Handle the escape char as a proper DBV so that Unicode endian
**	    issues are avoided.
**	    Also adjust sizes of the PAT_SET fields to avoid alignment problems
**	    on Solaris.
**	    Bar patterns of repeated empty such as ()* or (|xx)* as these
**	    will yield infinite loops.
**	09-Feb-2009 (kiria01) SIR120473
**	    Code to set AD_PAT2_PURE_LITERAL and AD_PAT2_LITERAL pattern summary
**	    hints to detect at PATCOMP time whether a pattern operator could
**	    be replaced by a non-pattern EQUAL operation. This exports
**	    information that can be used at parsetime to optimise lookups that
**	    might resolve to a hashable lookup.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) b122128
**	    Ensure that key is properly built if an implied % is present
**	    such with FNDLIT. Added logic to sence applicability of PAT_ENDLIT
**      14-Jul-2009 (hanal04) Bug 122311
**          Warning cleanup for Janitor's project.
**	23-Apr-2010 (wanfr01) Bug 123139
**	    Add cv.h for CVal definition
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. Essentially the UCS_BASIC
**	    additions have been isolated in service routines that handle the
**	    generation of literals or set data.
**	    to use UCS2 CEs for comparison related actions.
**      02-Dec-2010 (gupsh01) SIR 124685
**          Protype cleanup.
**/

#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <st.h>
#include <id.h>
#include <me.h>
#include <iicommon.h>
#include <ulf.h>
#include <adf.h>
#include <adfint.h>
#include <aduint.h>
#include <adulcol.h>
#include <aduucol.h>
#include <nm.h>
#include <cv.h>

#include "adupatexec.h"

/*
** Define data for instruction decoding, dump and load code for debug.
**
** The operator definitions are defined in a table macro that is used
** here to create the lookup tables for the dump and load code.
*/
enum {
    Pn_0 =	0,
    Pn_1 =	1,
    Pn__ =	2,

    P1__ =	Pn__,
    P1_N,

    P2__ =	Pn__,
    P2_M,

    opClass__ =	Pn__,
    opClass_C,
    opClass_J,
    opClass_L,
    opClass_S,
    opClass_NS,
    opClass_B,
    opClass_NB,

    V1_0 =	Pn_0,
    V1_1 =	Pn_1,
    V1__ =	Pn__,

    V2_W,
    V2__ =	Pn__
};
#define _ENDDEFINE
#define _DEFINE(ty,v,idx) opTy_##ty=v,
enum optypes_enum { PAT_OPTYPES opTy_max };
#undef _DEFINE
#define _DEFINE(ty,v,idx) #ty,
static const char optypes[][10]={ PAT_OPTYPES "*BAD*" };
#undef _DEFINE
#define _DEFINE(ty,v,idx) idx,
static const char optypesidx[]={ PAT_OPTYPES };
#undef _DEFINE
#define _DEFINEOP(op, v, ty, p1, p2, cl, v1, v2) {#op, opTy_##ty,P1_##p1,P2_##p2,opClass_##cl, V1_##v1,V2_##v2},
#define _DEFINEEX(op, v)
static const struct {
    char opname[32];
    u_i1 ty, p1, p2, cl, v1, v2;
} opcodes[] = {
    PAT_OPCODES {"",opTy_max,P1__,P2__,opClass__,V1__,V2__}
};
#undef _DEFINEOP
#undef _DEFINEEX
#undef _ENDDEFINE


/*
** PAT_BLD - local structure
**
** This structure holds the state needed to build the compiled pattern.
** The pattern is built using the pat_op_* routines and the memory into
** which it is compiled will be managed by the routine chk_space().
**
** This structure is common to the compilers and any datatype differences
** are isolated in the datatype specific routines - the handling however
** should be generic.
*/
/* At times it is necessary to store pointers into the operator list whilst
** building the operators. As the memory is managed, the pointers must be
** relative to a base address. The base address chosen will be patdata.
*/

typedef struct _PAT_BLD {
    ADF_CB		*adf_scb;
    AD_PATDATA		*patdata;	/* (vm) See AD_PATDATA description */
    u_i1		*wrtptr;	/* Write pointer */
    u_i1		*patbufend;	/* End of pattern buffer */
    u_i1		*last_op;	/* Location of prev-op */
    u_i1		*prelit_op;	/* Location of op before lit */

    /* PAT_REP structure to handle repeat counts and nesting (and case labels) */
    struct _PAT_REP {	/* Pointers offset to (u_i1*)patdata */
	i4	inst;		/* PAT_FOR address */
	i4	caseopr;	/* PAT_CASE oprnd address */
	i4	prlbl;		/* prior label address */
    }			rep_ctxs[DEF_STK+1];
    i4			rep_depth;
    u_i4		len;		/* Managed size of data */
    bool		force_fail;
    u_i4		no_of_ops;	/* Count of pattern operators used */
#if PAT_DBG_TRACE>0
    const u_i1		*start_text;	/* Pattern text for comment */
#endif
} PAT_BLD;

/*
** PAT_SET structure to aid building sorted set.
*/
#define PAT_SET_STK_DIM 64
typedef struct _PAT_SET {
    u_i4    n;
    u_i4    lim;
    u_i4    part;
    u_i4    last;
    struct strslot {
#define ISRANGE -1
	i2 len;
	u_i1 ch[8];
    } set[PAT_SET_STK_DIM];
} PAT_SET;

/*
** Local data
*/
static u_i1 dummy_op = PAT_NOP;

/*
** MO variable: exp.adf.adu.pat_debug
** Having defined the ima_mib_objects flat object table, set variable
** using:
**
**   UPDATE ima_mib_objects
**   SET value=8 WHERE classid='exp.adf.adu.pat_debug'
**
** This would enable the ^ set operator to be honoured
*/
i4 ADU_pat_debug = 0;
#define PAT_DBG_TRACE_ENABLE	0x01	/* General comp trace */
#define PAT_DBG_TRACE_COALESCE	0x02	/* Trace set coalesce */
#define PAT_DBG_TRACE_INJECT	0x04	/* Insert trace enable opcode */
#define PAT_DBG_NEG_ENABLE	0x08	/* Allow ^ in set for NSET tests */


/*
** Character classes:
*/
static const u_char chclasses[] = {
    'a','l','n','u','m',0,			'0','-','9','A','-','Z','a','-','z',0,
    'a','l','p','h','a',0,			'A','-','Z','a','-','z',0,
    'd','i','g','i','t',0,			'0','-','9',0,
    'l','o','w','e','r',0,			'a','-','z',0,
    's','p','a','c','e',0,			' ',0,
    'u','p','p','e','r',0,			'A','-','Z',0,
    'w','h','i','t','e','s','p','a','c','e',0,	0x09, /* Horizontal Tabulation */
						0x0A, /* Line Feed */
						0x0B, /* Vertical Tabulation */
						0x0C, /* Form Feed */
						0x0D, /* Carriage Return */
						0,
    0,0
};

static const UCS2 chclasses_uni[] = {
    'a','l','n','u','m',0,			'0','-','9','A','-','Z','a','-','z',0,
    'a','l','p','h','a',0,			'A','-','Z','a','-','Z',0,
    'd','i','g','i','t',0,			'0','-','9',0,
    'l','o','w','e','r',0,			'a','b','c','d','e','f','g','h','i','j',
						'k','l','m','n','o','p','q','r','s','t',
						'u','v','w','x','y','z',0,
    's','p','a','c','e',0,			' ',0,
    'u','p','p','e','r',0,			'A','B','C','D','E','F','G','H','I','J',
						'K','L','M','N','O','P','Q','R','S','T',
						'U','V','W','X','Y','Z',0,
    'w','h','i','t','e','s','p','a','c','e',0,	0x0009, /* Horizontal Tabulation */
						0x000A, /* Line Feed */
						0x000B, /* Vertical Tabulation */
						0x000C, /* Form Feed */
						0x000D, /* Carriage Return */
						0x0085, /* Next Line */
	/* class "Zl" */			0x2028, /* Line Separator */
	/* class "Zp" */			0x2029, /* Paragraph Separator */
	/* class "Zs" */			0x0020, /* Space */
						0x00A0, /* No-Break Space */
						0x1680, /* Ogham Space Mark */
						0x2000, /* En Quad */
						0x2001, /* Em Quad */
						0x2002, /* En Space */
						0x2003, /* Em Space */
						0x2004, /* Three-Per-Em Space */ 
						0x2005, /* Four-Per-Em Space */ 
						0x2006, /* Six-Per-Em Space */
						0x2007, /* Figure Space */
						0x2008, /* Punctuation Space */
						0x2009, /* Thin Space */
						0x200A, /* Hair Space */
						0x202F, /* Narrow No-Break Space */
						0x3000, /* Ideographic Space */
						0,
    0,0
};

static i4 cmp_simple   (PTR, const u_i1*, u_i4, const u_i1*, u_i4);
static i4 cmp_collating(PTR, const u_i1*, u_i4, const u_i1*, u_i4);
static i4 cmp_unicode  (PTR, const u_i1*, u_i4, const u_i1*, u_i4);
static i4 cmp_unicodeCE(PTR, const u_i1*, u_i4, const u_i1*, u_i4);
static bool pat_set_add(PAT_SET **listp, const u_char *ch, u_i4 len, bool range, 
			i4 (*cmp)(PTR, const u_i1*, u_i4, const u_i1*, u_i4), 
			PTR arg);
static u_i1 * trace_op(u_i1 *pc, bool uni);


/*
** Name: find_op() - Look for an operator
**
**      This routine scans the operator table for an appropriate operator
**
** Inputs:
**	optype		Type of operator
**	N		Value for any first operand
**	M		Value for any second operand
**	wild		Whether wild
**
** Returns:
**	u_i1		Operator for match and PAT_BUGCHK if not
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static u_i1 
find_op(enum optypes_enum optype, u_i4 N, u_i4 M, bool wild)
{
    enum PAT_OP_enums op, best = PAT_BUGCHK;
    u_i1 v1 = (N < V1__?N:V1__);
    for (op = optypesidx[optype]; opcodes[op].ty == optype &&
				opcodes[op].opname[0]; op++)
    {
	if (wild)
	{
	    if (opcodes[op].v2 == V2_W)
	    {
		if (opcodes[op].v1 == v1)
		    return op;
		if (opcodes[op].p1 == P1_N && N)
		    best = op;
	    }
	}
	else if (M > 0)
	{
	    if (opcodes[op].p2 == P2_M)
		return op;
	}
	else if (opcodes[op].v2 != V2_W)
	{
	    if (opcodes[op].v1 == v1)
		return op;
	    if (opcodes[op].p1 == P1_N)
		best = op;
	}
    }
    return best;
}


#if PAT_DBG_TRACE>0
/*
** Name: trace_op() - Dissassemble 1 opt to trace
**
**      This routine disassembles 1 instriction to the trace log
**
** Inputs:
**	pc		Address of start of instruction
**	uni		bool set if unicode
**
** Returns:
**	u_i1*		Address of next instruction
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static u_i1 *
trace_op(u_i1 *pc, bool uni)
{
    char buf[80];
    i8 v = 0;
    char *p;
    p = buf;
    while (adu_pat_disass(buf, sizeof(buf), &pc, &p, &v, uni)){
	*p++ = '\n';
	TRwrite(0, p - buf, buf);
	p = buf;
	*p++ = '\t';
	*p++ = '\t';
    }
    *p = '\n';
    TRwrite(0, p - buf, buf);
    return pc;
}
#endif


/*
** Name: pat_bld_init() - Initialise PAT_BLD structure
**
**      This routine initialises the managed buffer of the compiled
**	pattern. This routine needs to be called to begin the proocess of
**	comiling the patterns in place.
**
** Inputs:
**	adf_scb		ADF control block for error handling
**	ctx		Pointer to structure instance to initialise
**	pat_flags	Flags to pin in pattern.
**
** Outputs:
**	ctx		Initialised contect
**
** Returns:
**	None
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	15-Dec-2008 (kiria01) SIR120473
**	    Dropped redundant flags copy from sea_ctx.
*/
static VOID
pat_bld_init (ADF_CB *adf_scb, AD_PAT_SEA_CTX *sea_ctx, PAT_BLD *ctx, u_i4 pat_flags)
{
    i4 i;

    /* ->patdata is passed in preset with a valid .patdata.length -
    ** save it now as we will clear the lot */
    ctx->patdata = sea_ctx->patdata;
    MEfill(sizeof(*sea_ctx), 0, (PTR)sea_ctx);
    sea_ctx->patdata = ctx->patdata;
    /*sea_ctx->buffer = NULL;*/
    /*sea_ctx->bufend = NULL;*/
    /*sea_ctx->buftrueend = NULL;*/
    /*sea_ctx->buflen = 0;*/
    /*sea_ctx->at_bof = FALSE;*/
    /*sea_ctx->at_eof = FALSE;*/
    /*sea_ctx->trace = FALSE;*/
    sea_ctx->force_fail = TRUE; /* Default to fail */
    /*sea_ctx->cmplx_lim_exc = FALSE;*/
    /*sea_ctx->stalled = NULL;*/
    /*sea_ctx->pending = NULL;*/
    /*sea_ctx->free = NULL;*/
    /*sea_ctx->setbuf = NULL;*/
#if PAT_DBG_TRACE>0
    /*sea_ctx->nid = 0;*/
    /*sea_ctx->infile = NULL;*/
    /*sea_ctx->outfile = NULL;*/
#endif
    /*sea_ctx->nctxs_extra = 0;*/
    sea_ctx->nctxs = DEF_THD;
    for (i = DEF_THD-1; i >= 0; i--)
    {
	sea_ctx->ctxs[i].next = sea_ctx->free;
	sea_ctx->free = &sea_ctx->ctxs[i];
    }

    ctx->adf_scb = adf_scb;
    /* wrtptr & patbufend setup in pat_bld_newpat */
    ctx->prelit_op = NULL;
    ctx->last_op = &dummy_op;
    ctx->rep_depth = 0;
    ctx->len = ctx->patdata->patdata.length*sizeof(u_i2);
    ctx->force_fail = FALSE;
    ctx->no_of_ops = 0;
    ctx->patdata->patdata.npats = 0;
    ctx->patdata->patdata.flags = pat_flags&0xFFFF;
    ctx->patdata->patdata.flags2 = pat_flags>>16;
    ctx->patdata->patdata.length = PATDATA_1STFREE;
}


/*
** Name: chk_space() - Ensure enough space in the pattern buffer
**
**      This routine ensures that there is enough space for the requested
**	action to succeed. As every pattern will need completing, an extra
**	sizeof(u_i4)*PATDATA_1STFREE bytes (12) is always present.
**
** Inputs:
**	ctx		PAT_BLD context
**	n		Requested number of bytes extra needed.
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			E_DB_ERROR if no space.
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
chk_space(
	PAT_BLD	*ctx,
	i4	n)
{
    DB_STATUS db_stat;
    /*
    ** Always allow for adding a new pattern extra.
    */
    if (ctx->wrtptr + n + sizeof(u_i2)*PATDATA_1STFREE > ctx->patbufend)
    {
	ctx->force_fail = TRUE;
	return adu_error(ctx->adf_scb, E_AD1027_PAT_LEN_LIMIT, 0);
    }
    return E_DB_OK;
}


/*
** Name: pat_op_simple() - Append trivial OP
**
**      This routine puts a simple operand in place - no operands.
**
** Inputs:
**	ctx		PAT_BLD context to be updated.
**	op		The opcode to write.
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			E_DB_ERROR if no space.
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	24-Aug-2009 (kiria01) b122525
**	    Dropped PAT_ANY_1 from per_lit_op processing as it
**	    should not be there.
*/
static DB_STATUS
pat_op_simple(
	PAT_BLD *ctx,
	enum PAT_OP_enums op)
{
    DB_STATUS db_stat;
    ctx->no_of_ops++;
    if (op == PAT_FINAL)
    {
	/* Drop trailing % */
	switch (*ctx->last_op)
	{
	case PAT_ANY_0_W:
	    *ctx->last_op = PAT_MATCH;
	    return E_DB_OK;
	case PAT_ANY_1_W:
	    *ctx->last_op = PAT_ANY_1;
	    op = PAT_MATCH;
	    break;
	case PAT_ANY_N_W:
	    *ctx->last_op = PAT_ANY_N;
	    op = PAT_MATCH;
	    break;
	case PAT_LIT:
	    /* If Collation or UnicodeCE present then must not use PAT_ENDLIT */
	    if (ctx->patdata->patdata.flags2 & (AD_PAT2_COLLATE|AD_PAT2_UNICODE_CE))
		break;
	    if (ctx->prelit_op)
	    {
		u_i1 *p = ctx->prelit_op, op = *p;
		while(!adu_pat_decode(&p, 0, 0, 0, 0, 0) && *p == PAT_NOP)
		    /*SKIP*/;
		switch (*ctx->prelit_op)
		{
		case PAT_ANY_0_W:
		    op = PAT_NOP;
		    /* If making a no-op we may be able to remove it. */
		    if (*p == PAT_LIT)
		    {
			ctx->no_of_ops--;
			ctx->wrtptr--;
			p--;
			ctx->last_op--;
			MEcopy(ctx->prelit_op+1, ctx->wrtptr-ctx->prelit_op, ctx->prelit_op);
		    }
		    break;
		case PAT_ANY_1_W:
		    op = PAT_ANY_1;
		    break;
		case PAT_ANY_N_W:
		    op = PAT_ANY_N;
		    break;
		}
		if (*p == PAT_LIT)
		{
		    *ctx->prelit_op = op;
		    *p = PAT_ENDLIT;
		}
		ctx->prelit_op = NULL;
	    }
	    break;
	}
    }

    if (db_stat = chk_space(ctx, 1))
	return db_stat;
    if (ctx->prelit_op &&
	    op != PAT_ANY_0_W &&
	    op != PAT_ANY_1_W)
	ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    *ctx->wrtptr++ = op;
    return E_DB_OK;
}


/*
** Name: pat_op_1() - Append single param OP
**
**      This routine puts a 1 operand instruction in place.
**
** Inputs:
**	ctx		PAT_BLD context to be updated.
**	op		The opcode to write.
**	p1		The operand
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			E_DB_ERROR if no space.
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	24-Aug-2009 (kiria01) b122525
**	    Dropped PAT_ANY_N from per_lit_op processing as it
**	    should not be there.
*/
static DB_STATUS
pat_op_1(
	PAT_BLD *ctx,
	enum PAT_OP_enums op,
	i4 p1)
{
    DB_STATUS db_stat;

    ctx->no_of_ops++;
    if (db_stat = chk_space(ctx, 1+NUMSZ(p1)))
	return db_stat;
    if (ctx->prelit_op &&
	    op != PAT_ANY_N_W)
	ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    *ctx->wrtptr++ = op;
    PUTNUM(ctx->wrtptr, p1);
    return E_DB_OK;
}


/*
** Name: pat_op_2() - Append dual param OP
**
**      This routine puts a 2 operand instruction in place.
**
** Inputs:
**	ctx		PAT_BLD context to be updated.
**	op		The opcode to write.
**	p1		The 1st operand
**	p2		The 2nd operand
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			E_DB_ERROR if no space.
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_op_2(
	PAT_BLD *ctx,
	enum PAT_OP_enums op,
	i4 p1,
	i4 p2)
{
    DB_STATUS db_stat;

    ctx->no_of_ops++;
    if (db_stat = chk_space(ctx, 1+NUMSZ(p1)+NUMSZ(p2)))
	return db_stat;
    if (ctx->prelit_op)
	ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    *ctx->wrtptr++ = op;
    PUTNUM(ctx->wrtptr, p1);
    PUTNUM(ctx->wrtptr, p2);
    return E_DB_OK;
}


/*
** Name: pat_bld_newpat() - Start a new pattern
**
**      This routine Starts a new pattern frame in the list
**
** Inputs:
**	ctx		PAT_BLD control block
**	ch		Start of pattern text
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise error from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_bld_newpat (PAT_BLD *ctx, const u_i1 *ch)
{
    DB_STATUS db_stat = E_DB_OK;
    ctx->patdata->patdata.npats++;
    ctx->wrtptr = (u_i1*)&ctx->patdata->vec[ctx->patdata->patdata.length+1];
    ctx->patbufend = (u_i1*)ctx->patdata->vec + ctx->len;
    ctx->prelit_op = NULL;
    ctx->last_op = &dummy_op;
#if PAT_DBG_TRACE>0
    ctx->start_text = ch;
    if (ADU_pat_debug & PAT_DBG_TRACE_INJECT)
	db_stat = pat_op_simple(ctx, PAT_TRACE);
#endif
    return db_stat;
}


/*
** Name: pat_bld_finpat() - Complete pattern frame in list
**
**      This routine finishes of the housekeeping required to complete
**	a new pattern frame. The memory needed for a following lis will
**	be present as pre-allocated by chk_space.
**
** Inputs:
**	ctx		PAT_BLD context
**	ch		End of pattern text
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	26-Jan-2009 (kiria01) SIR120473
**	    Cure the non-reporting of the out of space error.
*/
static VOID
pat_bld_finpat (PAT_BLD *ctx, const u_char *ch)
{
    if (ctx->patdata->patdata.npats)
    {
	u_i4 nl_idx;
	u_i4 end_idx;
	u_i4 extra;

#if PAT_DBG_TRACE>0
	if (ctx->start_text &&
#ifdef _ADU_GEN_HELEM
	    ADU_pat_debug &&
#endif
	    /*
	    ** Perform benign space check to see if buffer has
	    ** space for the requested comment. Otherwise failure is hard
	    ** and worse, not reported as the status is not propagared.
	    ** !chk_space(ctx, ch - ctx->start_text+5)
	    */
	    ctx->wrtptr + (ch - ctx->start_text + 5) +
		sizeof(u_i2)*PATDATA_1STFREE < ctx->patbufend)
	{
	    if ((SCALARP)ctx->wrtptr & (sizeof(i2)-1))
		*ctx->wrtptr++ = PAT_NOP;
	    *ctx->wrtptr++ = PAT_COMMENT;
	    {   i8 tmp = ch - ctx->start_text;
		PUTNUM(ctx->wrtptr, tmp);
	    }
	    while (ctx->start_text < ch)
		*ctx->wrtptr++ = *ctx->start_text++;
	}
#endif
	if ((SCALARP)ctx->wrtptr & (sizeof(i2)-1))
	    *ctx->wrtptr++ = PAT_NOP;

	end_idx = (u_i2*)ctx->wrtptr - ctx->patdata->vec;
	nl_idx = ctx->patdata->patdata.length;
	extra = end_idx - nl_idx;
	ctx->patdata->vec[nl_idx] = extra;
	ctx->patdata->patdata.length = end_idx;
    }
}


/*
** Name: pat_update_num() - Adjust a numeric pattern operand
**
**      This routine adds the specified number to a numeric operand
**	taking care to handle the overflow from 1 to 2,2 to 4 or 4 to 8
**	bytes if it should happen.
**
**	NOTE: The caller is expected to chk_space()
**
**	This routine and pat_insert_num*() will be use when we coalesce
**	consecutive operands such as:
**
**	PAT_LIT 3,'123'	    \
**	followed by	     >	PAT_LIT 6,'123XYZ'
**	PAT_LIT 3,'XYZ'	    /
**
**	PAT_ANY_1	    \
**	followed by	     >	PAT_ANY_N_W, 2
**	PAT_ANY_1_W	    /
**
** Inputs:
**	p		Address of pointer to numeric operand
**			Underlying pointer will be left pointing to next
**			operand.
**	adj		The value to add to the numeric operand
**	wrtptr		The address of the write pointer that will need
**			to be updated if the operand size does change.
**
** Outputs:
**	p		Underlying pointer will be left pointing to next
**			operand even if it did not grow.
**	wrtptr		The address of the write pointer that will need
**			to be updated if the operand size does change.
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static VOID
pat_update_num(
	u_i1 **p,
	i8 adj,
	u_i1 **wrtptr)
{
    u_i1 *p1 = *p;
    i8 ov = GETNUM(p1);
    i8 nv = ov+adj;
    i4 nl = NUMSZ(nv);
    i4 ol = NUMSZ(ov);
    if (nl != ol)
    {
	i4 l = *wrtptr - p1;
	MEmemmove(p1+nl-ol, p1, l);
	*wrtptr += nl-ol;
    }
    PUTNUM((*p), nv);
}


/*
** Name: pat_insert_num() - Insert a numeric pattern operand
**
**      This routine inserts the specified value as a numeric operand
**	to an operator that is being updated from implied operand to
**	an explicit one. This is similar to pat_update_num() except this
**	starts with 0 and always updats the wrtptr.
**
**	NOTE: The caller is expected to chk_space()
**
** Inputs:
**	p		Address of pointer to numeric operand
**			Underlying pointer will be left pointing to next
**			operand.
**	nv		The value to use as new operand
**	wrtptr		The address of the write pointer that will need
**			to be updated.
**
** Outputs:
**	p		Underlying pointer will be left pointing to next
**			operand.
**	wrtptr		The address of the write pointer that will need
**			to be updated.
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static VOID
pat_insert_num(
	u_i1 **p,
	i8 nv,
	u_i1 **wrtptr)
{
    u_i1 *p1 = *p;
    i4 nl = NUMSZ(nv);
    i4 l = *wrtptr - *p;
    MEmemmove(p1+nl, p1, l);
    *wrtptr += nl;
    PUTNUM((*p), nv);
}


/*
** Name: pat_op_lit() - Flush any pending literal text (non-unicode)
**
**      This routine uses the marker indicating the first item of a text
**	run to output as a literal the outstanding text.
**
** Inputs:
**	ctx		PAT_BLD context
**	lit		Address of pointer to current literal start if any
**	end		Address of end of literal
**
** Outputs:
**	ctx		If space limited, the .patdata buffer will be
**			realloc'd and the managed pointers adjusted.
**	lit		Pointer will be cleared.
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	04-Mar-2009 (kiria01) bBUGNO
**	    We have a complication with zero-width collating characters
**	    in that they need stripping out of the patterns
*/
static DB_STATUS
pat_op_lit(
	PAT_BLD	*ctx,
	const u_char	**lit,
	const u_char	*end)
{
    DB_STATUS db_stat;
    const u_char *l;
    if (l = *lit)
    {
	u_i1 *p;

	*lit = NULL;
	if (db_stat = chk_space(ctx, (end - l + 1) * sizeof(UCS2) + 8))
	    return db_stat;
	/* Check coalesce */
	switch (*ctx->last_op)
	{
	case PAT_LIT:
	    /* Update consequtive length */
	    p = ctx->last_op+1;
	    pat_update_num(&p, end - l, &ctx->wrtptr);
	    /*
	    ** Leaves last_op unchange ready for another coalesce
	    ** and the wrtptr ready for the data to be appended
	    */
	    break;
	case PAT_ANY_0_W:
	case PAT_ANY_1_W:
	case PAT_ANY_N_W:
	    ctx->prelit_op = ctx->last_op;
	    /*FALLTHROUGH*/
	default:
	    ctx->no_of_ops++;
	    ctx->last_op = ctx->wrtptr;
	    *ctx->wrtptr++ = PAT_LIT;
	    {   i8 tmp = (end - l) * sizeof(u_char);
		PUTNUM(ctx->wrtptr, tmp);
	    }
	}
	if (ctx->patdata->patdata.flags & AD_PAT_WO_CASE)
	{
	    while (l < end)
	    {
		CMtolower(l, ctx->wrtptr);
		ctx->wrtptr++;
		l++;
	    }
	}
	else
	{
	    while (l < end)
		*ctx->wrtptr++ = *l++;
	}
    }
    return E_DB_OK;
}


/*
** Name: pat_op_ulit() - Flush any Unicode literal
**
**      This routine uses the marker indicating the first item of a text
**	run to output as a literal the outstanding text - as for the
**	routine pat_op_lit() but for unicode.
**
**	We can use the raw data, as the UCS_BASIC collation is in effect.
**
** Inputs:
**	ctx		PAT_BLD context
**	lit		Address of pointer to current literal start if any
**	end		Address of endof literal
**
** Outputs:
**	lit		Pointer will be cleared.
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**	19-Nov-2010 (kiria01) SIR 124690
**          Initial creation.
*/
static DB_STATUS
pat_op_ulit(
	PAT_BLD	*ctx,
	const UCS2	**lit,
	const UCS2	*end)
{
    DB_STATUS db_stat;
    const UCS2 *l;
    if (l = *lit)
    {
	u_i1 *p;

	*lit = NULL;
	if (db_stat = chk_space(ctx, (end - l + 1) * sizeof(UCS2) + 8))
	    return db_stat;
	/* Check coalesce */
	switch (*ctx->last_op)
	{
	case PAT_LIT:
	    /* Update consequtive length */
	    p = ctx->last_op+1;
	    pat_update_num(&p, (end - l)*sizeof(UCS2), &ctx->wrtptr);
	    /*
	    ** Leaves last_op unchanged ready for another coalesce
	    ** and the wrtptr ready for the data to be appended
	    */
	    break;
	case PAT_ANY_0_W:
	case PAT_ANY_1_W:
	case PAT_ANY_N_W:
	    ctx->prelit_op = ctx->last_op;
	    /*FALLTHROUGH*/
	default:
	    ctx->no_of_ops++;
	    if ((SCALARP)ctx->wrtptr & (sizeof(UCS2)-1))
	    {
		/* Stick in NOP to align the LITERAL */
		*ctx->wrtptr++ = PAT_NOP;
	    }
	    ctx->last_op = ctx->wrtptr;
	    *ctx->wrtptr++ = PAT_LIT;
	    {   i8 tmp = (end - l) * sizeof(UCS2);
		PUTNUM(ctx->wrtptr, tmp);
	    }
	}
	if (ctx->patdata->patdata.flags & AD_PAT_WO_CASE)
	{
	    while (l < end)
	    {
		adu_patda_ucs2_lower_1((ADUUCETAB *)ctx->adf_scb->adf_ucollation,
				&l, end, (UCS2**)&ctx->wrtptr);
	    }
	}
	else
	{
	    while (l < end)
		*ctx->wrtptr++ = *l++;
	}
    }
    return E_DB_OK;
}

/*
** Name: pat_op_CElit() - Flush any Unicode literal as CE elements
**
**      This routine uses the marker indicating the first item of a text
**	run to output as a literal the outstanding text - as for the
**	routine pat_op_lit() but for unicode CE.
**
**	Instead of the raw data, Unicode UTF-16 chars are replaced by the
**	first 3 CE levels of data for the compare to function. The CE data
**	is generated in the adu_pat_MakeCEchar/adu_pat_MakeCElen routines.
**
** Inputs:
**	ctx		PAT_BLD context
**	lit		Address of pointer to current literal start if any
**	end		Address of endof literal
**
** Outputs:
**	lit		Pointer will be cleared.
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_op_CElit(
	PAT_BLD	*ctx,
	const UCS2	**lit,
	const UCS2	*end)
{
    DB_STATUS db_stat;
    const UCS2 *l;
    if (l = *lit)
    {
	u_i4 len;
	u_i1 *p;

	*lit = NULL;
	/* Calc length of CE data */
	if (db_stat = adu_pat_MakeCElen(ctx->adf_scb, l, end, &len))
	    return db_stat;
	if (db_stat = chk_space(ctx, len + 2 + 8))
	    return db_stat;
	/* Check coalesce */
	switch (*ctx->last_op)
	{
	case PAT_LIT:
	    /* Update consequtive length */
	    p = ctx->last_op+1;
	    pat_update_num(&p, len, &ctx->wrtptr);
	    /*
	    ** Leaves last_op unchanged ready for another coalesce
	    ** and the wrtptr ready for the data to be appended
	    */
	    break;
	case PAT_ANY_0_W:
	case PAT_ANY_1_W:
	case PAT_ANY_N_W:
	    ctx->prelit_op = ctx->last_op;
	    /*FALLTHROUGH*/
	default:
	    ctx->no_of_ops++;
	    if ((SCALARP)ctx->wrtptr & (sizeof(UCS2)-1))
	    {
		/* Stick in NOP to align the LITERAL */
		*ctx->wrtptr++ = PAT_NOP;
	    }
	    ctx->last_op = ctx->wrtptr;
	    *ctx->wrtptr++ = PAT_LIT;
	    PUTNUM(ctx->wrtptr, len);
	}
	adu_pat_MakeCEchar(ctx->adf_scb, &l, end,
		(UCS2**)&ctx->wrtptr, ctx->patdata->patdata.flags);
    }
    return E_DB_OK;
}


/*
** Name: pat_op_any_one() - Generate PAT_ANY_1 
**
**      This routine generates PAT_ANY_1 operations and checks for a
**	potential coalesce
**
** Inputs:
**	ctx		PAT_CTX context
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_op_any_one(
	PAT_BLD *ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    switch(*ctx->last_op)
    {
    case PAT_ANY_0_W:
	*ctx->last_op = PAT_ANY_1_W;
	break;
    case PAT_ANY_1:
    case PAT_ANY_1_W:
	{
	    u_i1 *p = ctx->last_op;
	    *p++ = (*ctx->last_op == PAT_ANY_1)?PAT_ANY_N:PAT_ANY_N_W;
	    pat_insert_num(&p, (i8)2, &ctx->wrtptr);
	}
	break;
    case PAT_ANY_N:
    case PAT_ANY_N_W:
	{
	    u_i1 *p = ctx->last_op+1;
	    pat_update_num(&p, (i8)1, &ctx->wrtptr);
	}
	break;
    default:
	db_stat = pat_op_simple(ctx, PAT_ANY_1);
    }
    return db_stat;
}


/*
** Name: pat_op_wild() - Generate PAT_ANY_0_W
**
**      This routine generates the PAT_ANY_0_W and checks for coalesce.
**
** Inputs:
**	ctx		PAT_BLD context
**
** Outputs:
**	ctx		If space limited, the .patdata buffer will be
**			realloc'd and the managed pointers adjusted.
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**      30-Dec-2008 (kiria01) SIR120473
**          Added logic to convert PAT_LIT to faster PAT_FNDLIT
**          when preceeded and followed by PAT_ANY_?_W.
*/
static DB_STATUS
pat_op_wild(
	PAT_BLD *ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    /* Only if preceeding operators look ok for the conversion
    ** of PAT_LIT to PAT_FNDLIT will .prelit_op be set. We sanity
    ** check the instruction following which must be a PAT_LIT
    ** possibly preceded by benign PAT_NOPs */
    if (ctx->prelit_op)
    {
	u_i1 *p = ctx->prelit_op, op = *p;
	while(!adu_pat_decode(&p, 0, 0, 0, 0, 0) && *p == PAT_NOP)
	    /*SKIP*/;
	switch (*ctx->prelit_op)
	{
	case PAT_ANY_0_W:
	    op = PAT_NOP;
	    /* If making a no-op we may be able to remove it.
	    ** Unicode may need it to retain alignment so chk followed by a no-op
	    ** in this case */
	    if (*p == PAT_LIT)
	    {
		if (ctx->patdata->patdata.flags2 & (AD_PAT2_UCS_BASIC|AD_PAT2_UNICODE_CE))
		{
		    if (ctx->prelit_op[1] == PAT_NOP)
		    {
			ctx->wrtptr -= 2;
			p -= 2;
			ctx->last_op -= 2;
			MEcopy(ctx->prelit_op+2, ctx->wrtptr-ctx->prelit_op, ctx->prelit_op);
		    }
		}
		else
		{
		    ctx->wrtptr--;
		    p--;
		    ctx->last_op--;
		    MEcopy(ctx->prelit_op+1, ctx->wrtptr-ctx->prelit_op, ctx->prelit_op);
		}
	    }
	    break;
	case PAT_ANY_1_W:
	    op = PAT_ANY_1;
	    break;
	case PAT_ANY_N_W:
	    op = PAT_ANY_N;
	    break;
	}
	if (*p == PAT_LIT)
	{
	    ctx->no_of_ops++; /* double count FND or looks like a literal */
	    *ctx->prelit_op = op;
	    *p = PAT_FNDLIT;
	}
	ctx->prelit_op = NULL;
    }
    switch (*ctx->last_op)
    {
    case PAT_ANY_1:
	*ctx->last_op = PAT_ANY_1_W;
	break;
    case PAT_ANY_N:
	*ctx->last_op = PAT_ANY_N_W;
	break;
    case PAT_ANY_0_W:
    case PAT_ANY_1_W:
    case PAT_ANY_N_W:
	/* Nothing changes - already W */
	break;
    default:
	db_stat = pat_op_simple(ctx, PAT_ANY_0_W);
    }
    return db_stat;
}


/*
** Name: pat_op_bitset() - Generate PAT_[N]BITSET_1
**
**      This routine builds a bitset operator for quick compares.
**
** Inputs:
**	ctx		PAT_BLD context
**	neg		Bool indicating whether to negate operator
**	lo		Lowest bit set
**	hi		Highest bit set
**	count		Count of bits set
**	set		Address of 256 bit buffer
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**      29-Jan-2009 (kiria01) SIR120473
**          Adjust the hi count to avoid it being excluded.
*/
static DB_STATUS
pat_op_bitset(
	PAT_BLD	*ctx,
	i4		neg,
	i4		lo,
	i4		hi,
	i4		count,
	u_char		*set)
{
    DB_STATUS db_stat;

    if (hi < lo)
    {
	if (neg)
	    /* This is [^] which means 1 character not in the empty
	    ** set - in other words the same as _ */
	    return pat_op_any_one(ctx);
	lo = 0;
    }
    ctx->no_of_ops++;
    lo &= ~7;
    hi++;   /* Make hi extrinsic to the range */
    hi = (hi + 7 - lo)/8;
    set += lo/8;
    while (hi && !set[hi-1])
	hi--;
    while(hi && !set[0])
    {
	hi--;
	set++;
	lo += 8;
    }
    /* We could check coalesce but at present it seems unlikely to
    ** benefit. Would need [a-c1-9][a-c1-9][a-c1-9] to meld to
    ** [a-c1-9]{3} etc.
    ** If a coalesce were to be done there would HAVE to be an exact set
    ** match!
    if (*ctx->last_op == PAT_BSET_1)
    {
	->PAT_BSET_N
    }
    else if (*ctx->last_op == PAT_BSET_N)
    {
	->PAT_BSET_N++
    }
    else
    */

    if (db_stat = chk_space(ctx, hi + 19))
	return db_stat;
    ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    *ctx->wrtptr++ = neg?PAT_NBSET_1:PAT_BSET_1;
    PUTNUM(ctx->wrtptr, lo);
    PUTNUM(ctx->wrtptr, hi);
    while (hi--)
	*ctx->wrtptr++ = *set++;
    return E_DB_OK;
}


/*
** Name: pat_op_set() - Generate PAT_[N]SET_1
**
**      This routine generated the simpler set operator
**
**	The routine pat_set_add() will have arranged the data in
**	a simple list of singleton characters and ranges. It will
**	have done so using a passed function for the compares so
**	that the logic is the same regardless of the datatype.
**
**	NOTE: The set passed may in fact be a dual set with both
**	selectors and de-selectors. The [.part] member, if !=0 will
**	indicate the boundary between the parts. If either [.part]
**	is != 0 or neg is set, the instruction generated will be
**	the fuller PAT_NSET form.
**	 A set of:		 will become:
**	  '[a-z]'		  PAT_SET_1  3   '-az'
**	  '[^a-z]'		  PAT_NSET_1 4 0 '-az'
**	  '[a-z^f-m]'		  PAT_NSET_1 7 3 '-az-fm'
**	  '[a-z^]'		  PAT_SET_1  3   '-az'
**
** Inputs:
**	ctx		PAT_BLD context
**	neg		Bool indicating sense of the set
**	setbuf		PAT_SET context
**	sptr		End address of set buffer
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	20-Sep-2009 (kiria01) b122611
**	    Fix uninitialised access when setbuf->n == 1
*/
static DB_STATUS
pat_op_set(
	PAT_BLD *ctx,
	i4		neg,
	PAT_SET		*setbuf,
	i4 (*cmp)(PTR, const u_i1*, u_i4, const u_i1*, u_i4),
	PTR arg)
{
    u_i4 Nn=1, Mn=0;
    bool wild = FALSE;
    DB_STATUS db_stat;
    u_i1 *hold_n, *real_last_op = ctx->last_op, op;
    u_i4 i, new_len = 0, l;

    if (neg && !setbuf->n)
    {
	/* This is [^] which means 1 character not in the empty
	** set - in other words the same as _ */
	return pat_op_any_one(ctx);
    }
    else if (!neg && setbuf->n == 1)
    {
	/* This is [x] which means 1 character - in other words the same
	** as LIT 'x' */
	const u_char *p = setbuf->set[0].ch;
	l = setbuf->set[0].len;
	return pat_op_lit(ctx, &p, p+l);
    }
    ctx->no_of_ops++;
    if (db_stat = chk_space(ctx, setbuf->n + 10))
	return db_stat;
    if (setbuf->part && setbuf->part != setbuf->n)
	neg = TRUE;
    ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    op = find_op(neg?opTy_NSET:opTy_SET, Nn, Mn, wild);

    *ctx->wrtptr++ = op;
    if (opcodes[op].p1 == P1_N)
	PUTNUM(ctx->wrtptr, Nn);
    if (opcodes[op].p2 == P2_M)
	PUTNUM(ctx->wrtptr, Mn);
    /* At this point we need an operand - we insert a 0 now
    ** and fix up after */
    hold_n = ctx->wrtptr;
    PUTNUM0(ctx->wrtptr);
    if (neg)
	PUTNUM0(ctx->wrtptr);
    for (i = 0; i < setbuf->n; i++)
    {
	if (neg && i == setbuf->part)
	{
	    u_i1 *hold_n2 = hold_n+NUMSZ(0);
	    new_len = ctx->wrtptr - hold_n2 - NUMSZ(0);
	    pat_update_num(&hold_n2, new_len, &ctx->wrtptr);
	}
	if (i+1 < setbuf->n && setbuf->set[i+1].len == ISRANGE)
	{
	    if (db_stat = chk_space(ctx, setbuf->set[i].len+1))
		return db_stat;
	    /* Write range introducer - but only if it is not
	    ** an artificial one that spans nothing */
	    if (cmp == 0 ||
		    cmp(arg, setbuf->set[i+2].ch, setbuf->set[i+2].len,
			       setbuf->set[i].ch, setbuf->set[i].len) != 1)
		*ctx->wrtptr++ = AD_5LIKE_RANGE;
	    /* Write LWB */
	    if (l = setbuf->set[i].len)
		MEmemcpy(ctx->wrtptr, setbuf->set[i].ch, l);
	    ctx->wrtptr += l;
	    /* Skip two slots to drop out and write UPB as next slot */
	    i += 2;
	}
	else if (*setbuf->set[i].ch == AD_5LIKE_RANGE)
	{
	    /* A lone '-'! As the '-' is used as a range
	    ** introducer we make this into a fake range '---' */
	    *ctx->wrtptr++ = AD_5LIKE_RANGE;
	    *ctx->wrtptr++ = AD_5LIKE_RANGE;
	    /* ... and let the final one be written below */
	}
	if (l = setbuf->set[i].len)
	{
	    if (db_stat = chk_space(ctx, l))
		return db_stat;
	    MEmemcpy(ctx->wrtptr, setbuf->set[i].ch, l);
	    ctx->wrtptr += l;
	}
    }
    /* Don't use the insert form as it wiil mean that
    ** the buffer build above will not be aligned */
    new_len = ctx->wrtptr - hold_n - 1;
    pat_update_num(&hold_n, new_len, &ctx->wrtptr);
    /* We check for coalesce but at present it seems unlikely to
    ** benefit. Would need [a-c1-9][a-c1-9][a-c1-9] to meld to
    ** [a-c1-9]{3} etc.
    ** If a coalesce is to be done there would HAVE to be an exact set
    ** match!
    */
    if (opcodes[*real_last_op].cl == opcodes[*ctx->last_op].cl)
    {
	u_i4 N = opcodes[*real_last_op].v1, M = 0, len;
	u_i1 *p = real_last_op;
	p++;
	if (N == V1__)
	    N = GETNUM(p);
	if (opcodes[*real_last_op].p2 == P2_M)
	    M = GETNUM(p);
	N += Nn;
	M += Mn;
	len = GETNUM(p);
	if (len == new_len && !MEcmp(p, ctx->wrtptr-len, len))
	{
	    i4 opN = N < V1__ ? N : V1__;
	    u_i1 *setdata = ctx->wrtptr-len;
	    if (opcodes[*real_last_op].v2 == V2_W)
		wild = TRUE;
	    op = find_op(opcodes[op].ty, N, M, wild);
	    if (op == PAT_BUGCHK)
	    {
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_ENABLE)
		    TRdisplay("Failed to find combining OP");
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("attempting OPs\n");
		    trace_op(real_last_op, FALSE);
		    trace_op(ctx->last_op, FALSE);
		}
#endif
	    }
	    else
	    {
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("Combining OPs\n");
		    trace_op(real_last_op, FALSE);
		    trace_op(ctx->last_op, FALSE);
		}
#endif
		ctx->wrtptr = real_last_op;
		ctx->last_op = real_last_op;
		*ctx->wrtptr++ = op;
		if (opcodes[op].p1 == P1_N)
		    PUTNUM(ctx->wrtptr, N);
		if (opcodes[op].p2 == P2_M)
		    PUTNUM(ctx->wrtptr, M);
		{
		    u_i8 tmp = len;
		    PUTNUM(ctx->wrtptr, tmp);
		}
		while (len--)
		    *ctx->wrtptr++ = *setdata++;
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("... into OP\n");
		    trace_op(ctx->last_op, FALSE);
		}
#endif
	    }
	}
    }
    return E_DB_OK;
}


/*
** Name: pat_op_uset() - Generate non-CE unicode PAT_[N]SET_1
**
**      This routine generates the simpler set operator
**
**	The routine pat_set_add() will have arranged the data in
**	a simple list of singleton characters and ranges. It will
**	have done so using a passed function for the compares so
**	that the logic is the same regardless of the datatype.
**
**	NOTE: The set passed may in fact be a dual set with both
**	selectors and de-selectors. The [.part] member, if !=0 will
**	indicate the boundary between the parts. If either [.part]
**	is != 0 or neg is set, the instruction generated will be
**	the fuller PAT_NSET form.
**	 A set of:		 will become:
**	  '[a-z]'		  PAT_SET_1  3   '-az'
**	  '[^a-z]'		  PAT_NSET_1 4 0 '-az'
**	  '[a-z^f-m]'		  PAT_NSET_1 7 3 '-az-fm'
**	  '[a-z^]'		  PAT_SET_1  3   '-az'
**
** Inputs:
**	ctx		PAT_BLD context
**	neg		Bool indicating sense of the set
**	setbuf		PAT_SET context
**	sptr		End address of set buffer
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**	19-Nov-2010 (kiria01) SIR 124690
**          Initial creation.
*/
static DB_STATUS
pat_op_uset(
	PAT_BLD *ctx,
	i4		neg,
	PAT_SET		*setbuf)
{
    u_i4 Nn=1, Mn=0;
    bool wild = FALSE;
    DB_STATUS db_stat;
    u_i1 *hold_n, *real_last_op = ctx->last_op, op;
    u_i4 i, new_len = 0, l;

    if (neg && !setbuf->n)
    {
	/* This is [^] which means 1 character not in the empty
	** set - in other words the same as _ */
	return pat_op_any_one(ctx);
    }
    else if (!neg && setbuf->n == 1)
    {
	/* This is [x] which means 1 character - in other words the same
	** as LIT 'x' */
	const UCS2 *p = (UCS2 *)setbuf->set[0].ch;
	l = setbuf->set[0].len;
	return pat_op_ulit(ctx, &p, p+l/sizeof(UCS2));
    }
    ctx->no_of_ops++;
    if (db_stat = chk_space(ctx, setbuf->n + 10))
	return db_stat;
    if (setbuf->part && setbuf->part != setbuf->n)
	neg = TRUE;
    ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    op = find_op(neg?opTy_NSET:opTy_SET, Nn, Mn, wild);

    *ctx->wrtptr++ = op;
    if (opcodes[op].p1 == P1_N)
	PUTNUM(ctx->wrtptr, Nn);
    if (opcodes[op].p2 == P2_M)
	PUTNUM(ctx->wrtptr, Mn);
    /* At this point we need an operand - we insert a 0 now
    ** and fix up after */
    hold_n = ctx->wrtptr;
    PUTNUM0(ctx->wrtptr);
    if (neg)
	PUTNUM0(ctx->wrtptr);
    for (i = 0; i < setbuf->n; i++)
    {
	if (neg && i == setbuf->part)
	{
	    u_i1 *hold_n2 = hold_n+NUMSZ(0);
	    new_len = ctx->wrtptr - hold_n2 - NUMSZ(0);
	    pat_update_num(&hold_n2, new_len, &ctx->wrtptr);
	}
	if (i+1 < setbuf->n && setbuf->set[i+1].len == ISRANGE)
	{
	    if (db_stat = chk_space(ctx, setbuf->set[i].len+1))
		return db_stat;
	    /* Write range introducer - but only if it is not
	    ** an artificial one that spans nothing */
	    if (cmp_unicode(NULL, setbuf->set[i+2].ch, setbuf->set[i+2].len,
			       setbuf->set[i].ch, setbuf->set[i].len) != 1)
	    {
		((UCS2*)ctx->wrtptr)[0] = U_RNG;
		ctx->wrtptr += sizeof(UCS2);
	    }
	    /* Write LWB */
	    if (l = setbuf->set[i].len)
		MEmemcpy(ctx->wrtptr, setbuf->set[i].ch, l);
	    ctx->wrtptr += l;
	    /* Skip two slots to drop out and write UPB as next slot */
	    i += 2;
	}
	else if (setbuf->set[i].len == sizeof(UCS2) &&
		*((UCS2*)setbuf->set[i].ch) == U_RNG)
	{
	    /* A lone '-'! As the '-' is used as a range
	    ** introducer we make this into a fake range '---' */
	    ((UCS2*)ctx->wrtptr)[0] = U_RNG;
	    ((UCS2*)ctx->wrtptr)[0] = U_RNG;
	    ctx->wrtptr += sizeof(UCS2)*2;
	    /* ... and let the final one be written below */
	}
	if (l = setbuf->set[i].len)
	{
	    if (db_stat = chk_space(ctx, l))
		return db_stat;
	    MEmemcpy(ctx->wrtptr, setbuf->set[i].ch, l);
	    ctx->wrtptr += l;
	}
    }
    /* Don't use the insert form as it wiil mean that
    ** the buffer build above will not be aligned */
    new_len = ctx->wrtptr - hold_n - 1;
    pat_update_num(&hold_n, new_len, &ctx->wrtptr);
    /* We check for coalesce but at present it seems unlikely to
    ** benefit. Would need [a-c1-9][a-c1-9][a-c1-9] to meld to
    ** [a-c1-9]{3} etc.
    ** If a coalesce is to be done there would HAVE to be an exact set
    ** match!
    */
    if (opcodes[*real_last_op].cl == opcodes[*ctx->last_op].cl)
    {
	u_i4 N = opcodes[*real_last_op].v1, M = 0, len;
	u_i1 *p = real_last_op;
	p++;
	if (N == V1__)
	    N = GETNUM(p);
	if (opcodes[*real_last_op].p2 == P2_M)
	    M = GETNUM(p);
	N += Nn;
	M += Mn;
	len = GETNUM(p);
	if (len == new_len && !MEcmp(p, ctx->wrtptr-len, len))
	{
	    i4 opN = N < V1__ ? N : V1__;
	    u_i1 *setdata = ctx->wrtptr-len;
	    if (opcodes[*real_last_op].v2 == V2_W)
		wild = TRUE;
	    op = find_op(opcodes[op].ty, N, M, wild);
	    if (op == PAT_BUGCHK)
	    {
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_ENABLE)
		    TRdisplay("Failed to find combining OP");
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("attempting OPs\n");
		    trace_op(real_last_op, FALSE);
		    trace_op(ctx->last_op, FALSE);
		}
#endif
	    }
	    else
	    {
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("Combining OPs\n");
		    trace_op(real_last_op, FALSE);
		    trace_op(ctx->last_op, FALSE);
		}
#endif
		ctx->wrtptr = real_last_op;
		ctx->last_op = real_last_op;
		*ctx->wrtptr++ = op;
		if (opcodes[op].p1 == P1_N)
		    PUTNUM(ctx->wrtptr, N);
		if (opcodes[op].p2 == P2_M)
		    PUTNUM(ctx->wrtptr, M);
		{
		    u_i8 tmp = len;
		    PUTNUM(ctx->wrtptr, tmp);
		}
		while (len--)
		    *ctx->wrtptr++ = *setdata++;
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("... into OP\n");
		    trace_op(ctx->last_op, FALSE);
		}
#endif
	    }
	}
    }
    return E_DB_OK;
}


/*
** Name: pat_op_CEset() - Generate a CE list form PAT_[N]SET_1
**
**      This routine generates a unicode PAT_[N]SET_1.
**
**	The routine pat_set_add() will have arranged the data in
**	a simple list of singleton characters and ranges. It will
**	have done so using a passed function for the compares so
**	that the logic is the same regardless of the datatype.
**
**	NOTE: The set passed may in fact be a dual set with both
**	selectors and de-selectors. The [.part] member, if !=0 will
**	indicate the boundary between the parts. If either [.part]
**	is != 0 or neg is set, the instruction generated will be
**	the fuller PAT_NSET form.
**
** Inputs:
**	ctx		PAT_BLD context
**	neg		Bool indicating sense of the set
**	setbuf		PAT_SET context
**	sptr		End address of set buffer
**
** Returns:
**	DB_STATUS	E_DB_OK if ok
**			Otherwise result from chk_space
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_op_CEset(
	PAT_BLD *ctx,
	i4	neg,
	PAT_SET	*setbuf)
{
    u_i4 Nn=1, Mn=0;
    bool wild = FALSE;
    DB_STATUS db_stat;
    u_i4 len;
    UCS2 *to;
    u_i1 *hold_n, *real_last_op = ctx->last_op, op;
    u_i4 i, new_len = 0, l;

    if (neg && !setbuf->n)
    {
	/* This is [^] which means 1 character not in the empty
	** set - in other words the same as _ */
	return pat_op_any_one(ctx);
    }
    else if (!neg && setbuf->n == 1)
    {
	/* This is [x] which means 1 character - in other words the same
	** as LIT 'x' */
	const u_char *p = setbuf->set[0].ch;
	l = setbuf->set[0].len; /* Ought to be 6 */
	/* NOTE we don't pass this to pat_op_CElit as the SET data is already in
	** CE form in the buffer */
	return pat_op_lit(ctx, &p, p+l);
    }
    ctx->no_of_ops++;
    /* Calc length of CE data */
    if (db_stat = chk_space(ctx, 2 + 8))
	return db_stat;

    if (setbuf->part && setbuf->part != setbuf->n)
	neg = TRUE;
    ctx->prelit_op = NULL;
    ctx->last_op = ctx->wrtptr;
    op = find_op(neg?opTy_NSET:opTy_SET, Nn, Mn, wild);

    *ctx->wrtptr++ = op;
    if (opcodes[op].p1 == P1_N)
	PUTNUM(ctx->wrtptr, Nn);
    if (opcodes[op].p2 == P2_M)
	PUTNUM(ctx->wrtptr, Mn);
    /* At this point we need an operand - we insert a 0 now
    ** and fix up after */
    hold_n = ctx->wrtptr;
    PUTNUM0(ctx->wrtptr);
    if (neg)
	PUTNUM0(ctx->wrtptr);
    if ((SCALARP)ctx->wrtptr & (sizeof(UCS2)-1))
    {
	/* Stick in NOP to align the LITERAL */
	pat_insert_num(&ctx->last_op, PAT_NOP, &ctx->wrtptr);
	hold_n++;
    }
    for (i = 0; i < setbuf->n; i++)
    {
	if (neg && i == setbuf->part)
	{
	    u_i1 *hold_n2 = hold_n+NUMSZ(0);
	    new_len = ctx->wrtptr - hold_n2 - NUMSZ(0);
	    pat_update_num(&hold_n2, new_len, &ctx->wrtptr);
	}
	if (i+1 < setbuf->n && setbuf->set[i+1].len == ISRANGE)
	{
	    if (db_stat = chk_space(ctx, (setbuf->set[i].len/sizeof(UCS2)+1)*
					    (MAX_CE_LEVELS-1)*sizeof(UCS2)))
		return db_stat;
	    /* Write range introducer as CE */
	    ((UCS2*)ctx->wrtptr)[0] = U_RNG;
	    ((UCS2*)ctx->wrtptr)[1] = U_RNG;
	    ((UCS2*)ctx->wrtptr)[2] = U_RNG;
	    ctx->wrtptr += sizeof(UCS2)*3;
	    /* Write LWB */
	    if (l = setbuf->set[i].len)
		MEmemcpy(ctx->wrtptr, setbuf->set[i].ch, l);
	    ctx->wrtptr += l;
	    /* Skip two slots to drop out and write UPB as next slot */
	    i += 2;
	}
	if (l = setbuf->set[i].len)
	{
	    if (db_stat = chk_space(ctx, (l/sizeof(UCS2))* (MAX_CE_LEVELS-1)*sizeof(UCS2)))
		return db_stat;
	    MEmemcpy(ctx->wrtptr, setbuf->set[i].ch, l);
	    ctx->wrtptr += l;
	}
    }
    /* Don't use the insert form as it will mean that
    ** the buffer build above will not be aligned */
    new_len = ctx->wrtptr - hold_n - 1;
    pat_update_num(&hold_n, new_len, &ctx->wrtptr);

    /* We check for coalesce but at present it seems unlikely to
    ** benefit. Would need [a-c1-9][a-c1-9][a-c1-9] to meld to
    ** [a-c1-9]{3} etc.
    ** If a coalesce is to be done there would HAVE to be an exact set
    ** match!
    */
    if (opcodes[*real_last_op].cl == opcodes[*ctx->last_op].cl)
    {
	u_i4 N = opcodes[*real_last_op].v1, M = 0, len;
	u_i1 *p = real_last_op;
	p++;
	if (N == V1__)
	    N = GETNUM(p);
	if (opcodes[*real_last_op].p2 == P2_M)
	    M = GETNUM(p);
	N += Nn;
	M += Mn;
	len = GETNUM(p);
	if (len == new_len && !MEcmp(p, ctx->wrtptr-len, len))
	{
	    i4 opN = N < V1__ ? N : V1__;
	    u_i1 *setdata = ctx->wrtptr-len;
	    if (opcodes[*real_last_op].v2 == V2_W)
		wild = TRUE;
	    op = find_op(opcodes[op].ty, N, M, wild);
	    if (op == PAT_BUGCHK)
	    {
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_ENABLE)
		    TRdisplay("Failed to find combining OP");
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("attempting OPs\n");
		    trace_op(real_last_op, TRUE);
		    trace_op(ctx->last_op, TRUE);
		}
#endif
	    }
	    else
	    {
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("Combining OPs\n");
		    trace_op(real_last_op, TRUE);
		    trace_op(ctx->last_op, TRUE);
		}
#endif
		ctx->wrtptr = real_last_op;
		ctx->last_op = real_last_op;
		*ctx->wrtptr++ = op;
		if (opcodes[op].p1 == P1_N)
		    PUTNUM(ctx->wrtptr, N);
		if (opcodes[op].p2 == P2_M)
		    PUTNUM(ctx->wrtptr, M);
		{
		    u_i8 tmp = len;
		    PUTNUM(ctx->wrtptr, tmp);
		}
		while (len--)
		    *ctx->wrtptr++ = *setdata++;
#if PAT_DBG_TRACE>0
		if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
		    TRdisplay("... into OP\n");
		    trace_op(ctx->last_op, TRUE);
		}
#endif
	    }
	}
    }
    return E_DB_OK;
}


/*
** Name: pat_set_add() - Add set element
**
**  This routine adds to the set list. What we are about here is that 
**  we want to add the set elements into the list in a sorted manner
**  so that they are in collation order. When inserting an element
**  we insert singleton characters and LWB characters by insertion
**  sort. If the character has been seen before or is found within
**  a range then we ignore it. When inserting a range UPB we will
**  have retained the index of the LWB and will be able to have the
**  UPB scan span contained entries. In this way we optimise the
**  the set data so that duplicates or overlaps are avoided and the
**  set operators can be optimised to exit early.
**
** Inputs:
**	listp		Set work descriptor
**	ch		Pointer to start of char
**	end		Pointer to end of char
**	range		Boolean set true if this is a range upper bound
**			to use [.last] as lower.
**	cmp		Function pointer to handle the compare
**
** Outputs:
**	Mone
**
** Returns:
**	bool		True if range error found
**
** Exceptions:
**	none
**
** Side Effects:
**	Manages memeory
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	04-Jan-2010 (kiria01) b123096
**	    Catch illegal reversed range error.
*/
static bool
pat_set_add(
    PAT_SET **listp,
    const u_char *ch,
    u_i4 len,
    bool range,
    i4 (*cmp)(PTR, const u_i1*, u_i4, const u_i1*, u_i4),
    PTR arg)
{
    PAT_SET *list = *listp;
    u_i4 lim = 0;
    u_i4 i;
    i4 res;
    struct strslot ins;
    if (list)
	lim = list->lim;
    
    /* Manage space */
    if (!list || list->n+1 >= lim)
    {
	lim += 100;
	list = (PAT_SET*)MEreqmem(0, sizeof(*list)-sizeof(list->set)+
					    sizeof(*list->set)*lim,
		    FALSE, NULL);
	if (!list)
	    return TRUE;
	if (*listp)
	{
    	    MEmemcpy(list, *listp, sizeof(*list)-sizeof(list->set)+
					    sizeof(list->set)*(lim-100));
	    if ((*listp)->lim != PAT_SET_STK_DIM)
		MEfree((PTR)*listp);
	    *listp = list;
	}
	else
	{
	    list->last = lim;
	    list->n = 0;
	    list->part = 0;
	}
	list->lim = lim;
	*listp = list;
    }

    /* Take copy that we will insert */
    MEmemcpy(ins.ch, ch, ins.len = len);

re_try:
    if (range)
    {
	/*
	** Adding the upper bound - the lower will be at [.last]
	*/
	for (i = list->last; i <= list->n; i++)
	{
	    if (i == list->n)
	    {
		/*
		** Found no slot strictly greater than new UPB so
		** anything from last+1 to i-1 will become 0,UPB.
		** To keep things simple we will remove these now/
		*/
		if (i > list->last+1)
		    list->n -= i - list->last - 1;
		i = list->last + 1;
		break;
	    }
	    res = cmp(arg, ins.ch, ins.len, list->set[i].ch, list->set[i].len);
	    if (!res && i == list->last)
		/* LWB=UPB => 1 char so we found it - nothing else to do */
		return FALSE;
	    if (res == -1)
	    {
		/* res=-1 implies inserting ?-B into ?CZ or ?C-Z - leave to merge */
		ins=list->set[i];
		res = 0;
	    }
	    else if (res < 0 && i == list->last)
	    {
		return TRUE;
	    }
	    else if (res < 0)
	    {
		/*
		** Found the first slot strictly greater than new UPB so
		** anything from last+1 to i-1 will become 0,UPB.
		** To keep things simple we will remove these now.
		*/
		if (i > list->last+1)
		{
		    u_i4 j;
		    /* Remove gap */
		    for (j = 0; j < list->n - i; j++)
			list->set[list->last+1+j] = list->set[i+j];
		    list->n -= i - list->last - 1;
		}
		/* Create space */
		for (i = list->n-1; i > list->last; i--)
		{
		    list->set[i+2] = list->set[i];
		}
		i = list->last + 1;
		break;
	    }
	    /* The current one will be gobbled up but we need to see
	    ** if this is a part of a range. If it is then two ranges
	    ** coalesce */
	    if (i+1 < list->n && list->set[i+1].len == ISRANGE)
	    {
		i += 2;
		res = cmp(arg, ch, len, list->set[i].ch, list->set[i].len);
		/* If the upper bound is straddled we'll just continue. */
		if (res <= 0)
		{
		    u_i4 j;
		    /* Ranges overlap - merge. */
		    /* Remove gap but leave old UPB in place */
		    for (j = i - 1; j < list->n; j++)
			list->set[list->last+1 + j - i + 1] = list->set[j];
		    list->n -= i - list->last - 2;
		    return FALSE;
		}
	    }
	}
	if (i == list->last)
	    return TRUE;
	/* Leave i pointing to last+1 */
	list->set[i++].len = ISRANGE;
	list->set[i] = ins;
	list->n += 2;
	list->last = list->n;
    }
    else
    {
	for (i = list->part; i < list->n; i++)
	{
	    if (list->set[i].len == ISRANGE)
	    {
		/* The comparand is after LWB to get here - cmp with UPB */
		i++;
		res = cmp(arg, ins.ch, ins.len, list->set[i].ch, list->set[i].len);
		if (res <= 1)
		{
		    /* Already included in range oradj - point last to LWB */
		    list->last = i-2;
		    if (res == 1)
		    {
			/* Adjacent coalesce D added to A-C => A-D */
			if (i+1<list->n &&
			    cmp(arg, ins.ch, ins.len, list->set[i+1].ch, list->set[i+1].len) == -1)
			{
			    /* ... even better D added to A-CE => A-E or to A-CE-G => A-G */
			    ins = list->set[i+1];
			    range = TRUE;
			    goto re_try;
			}
			list->set[i] = ins;
		    }
		    return FALSE;
		}
	    }
	    else
	    {
		res = cmp(arg, ins.ch, ins.len, list->set[i].ch, list->set[i].len);
		if (res == 1 && (i >= list->n - 1 || list->set[i+1].len != ISRANGE))
		{
		    /* Adjacent coalesce - adding B to A => A-B */
		    list->last = i;
		    if (i < list->n - 1 &&
			    cmp(arg, ins.ch, ins.len, 
				    list->set[i+1].ch, list->set[i+1].len) == 1)
			/* Adjacent coalesce even better - adding B to AC => A-C */
			ins = list->set[i+1];
		    range = TRUE;
		    goto re_try;
		}
		if (res > 0)
		    continue;
		list->last = i;
		if (!res)
		    /* Already present */
		    return FALSE;
		/* Found insert point */
		if (res == -1)
		{
		    /* Adjacent coalesce = adding A to B => A-B */
		    if (i < list->n - 2 && list->set[i+1].len == ISRANGE)
		    {
			/* ... with a range - A to B-X => A-X */
			list->set[i] = ins;
			/* LWB pulled down to include - LWB already correct */
			return FALSE;
		    }
		    /* ... with a singleton - make range */
		    for (i = list->n; i > list->last; i--)
			list->set[i+1] = list->set[i-1];
		    list->set[list->last+1].len = ISRANGE;
		    list->n++;
		    /* Drop to common insert code */
		    break;
		}
		/* Straight insert */
		for (i = list->n; i > list->last; i--)
		    list->set[i] = list->set[i-1];
		break;
	    }
	}
	/* Insert at space or at end */
	list->last = i;
	list->set[i] = ins;
	list->n++;
    }
    return FALSE;
}


/*
** Name: pat_rep_push() - Push repeat context
**
**  This routine pushes a repeat context onto the stack for processing
**  and it is called when a '(' has been seen.
**  When parsing the '(', it will not be known whether there will be alternations
**  or if this is just a simple repeat group. We start by assuming the latter and
**  inject the PAT_FOR instruction with a suitable default set of parameters - the
**  block end offset will need to be fixed up at the end so the repeat context
**  will retain the address of the PAT_FOR instruction. The pattern operators can
**  be parsed until either the matching right parenthesis is seen or an alternation
**  character parsed.
**
**  Aside from establising the block scope, this routine needs to retain the
**  address of the instruction that we are about to add - the PAT_FOR_N_M for later
**  fix-up.
**
** Inputs:
**	ctx		Pattern build context
**
** Outputs:
**	Mone
**
** Returns:
**	statue		True if range error found
**
** Exceptions:
**	none
**
** Side Effects:
**	Updates parse context stack
**
** History:
**      29-Jul-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_rep_push(PAT_BLD *ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    u_i4 sp;
    if ((sp = ctx->rep_depth) > DEF_STK)
	return adu_error(ctx->adf_scb, E_AD101F_REG_EXPR_SYN, 0);

    ctx->rep_depth++;
    ctx->rep_ctxs[sp].inst = ctx->wrtptr - (u_i1*)ctx->patdata->vec;
    ctx->rep_ctxs[sp].caseopr = 0;
    ctx->rep_ctxs[sp].prlbl = 0;
    pat_op_2(ctx, PAT_FOR_N_M, 1, 0); /* N = 1,M = 0 */
    PUTNUM0(ctx->wrtptr);		/* L - fixed up later */
    return db_stat;
}


/*
** Name: pat_rep_pop() - Pop repeat context
**
**  This routine pops a repeat context from the stack and processes it.
**
**  This is called when finally, the ')' is seen and the the offsets
**  need to be calculated and applied and any range modifier parsed.
**  The range modifier will determine whether PAT_NEXT or PAT_NEXT_W
**  is used and its offset will be awkward to count; it will usually
**  be based on the PAT_FOR offset being -len-4 but a really big
**  pattern could increase the offset size. Anyway, the offset will be
**  based on materialised lengths of spanned instructions and it is
**  important that we calculate these offsets in a particular sequence
**  otherwise setting a length or offset could shift data invalidating
**  any higher machine addresses that might be store. As long as the
**  PAT_LABEL and PAT_JUMP instructions for a given case level are
**  fixed up in reverse order, the updates will be safe. As any nested
**  levels will already have been materialised, we need only be
**  concerned with those with zero offset.
**
** Inputs:
**	ctx		Pattern build context
**
** Outputs:
**	Mone
**
** Returns:
**	statue		True if range error found
**
** Exceptions:
**	none
**
** Side Effects:
**	Updates parse context stack
**
** History:
**      29-Jul-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_rep_pop(PAT_BLD *ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    u_i4 sp;
    u_i1 *inst;
    i4 l = 1 + NUMSZ(0) + 1 + NUMSZ(0);
    i4 offset;
    u_i4 patlen;
    if (!ctx->rep_depth)
	/* ')' without '(' */
	return adu_error(ctx->adf_scb, E_AD101F_REG_EXPR_SYN, 0);

    sp = --ctx->rep_depth;
    /* Any case context to fix up? */
    if (ctx->rep_ctxs[sp].caseopr)
    {
	u_i4 ncase, i, j, lbloff;
	i4 n;
	/* Fix prior label - was set to 0, wrtptr is at end of final case
	** alternation so length (needed for label) is wrtptr - instr after
	** label. */
	inst = ctx->rep_ctxs[sp].prlbl + (u_i1*)ctx->patdata->vec + 1;
	n = ctx->wrtptr - inst - NUMSZ(0);
	pat_update_num(&inst, n, &ctx->wrtptr);

	inst = ctx->rep_ctxs[sp].caseopr + (u_i1*)ctx->patdata->vec;
	ncase = GETNUM(inst);
	lbloff = inst - (u_i1*)ctx->patdata->vec;
	if (db_stat = chk_space (ctx, 4*ncase))
	    return db_stat;
	for (j = ncase; j > 0; j--)
	{
	    u_i1 *jinst, *jopr;
	    i4 skplen;
	    offset = lbloff;    /* Base label */
	    /* Chain down the case blocks to process in reverse
	    ** order so the stored pointer offsets remain valid */
	    for (i = 1; i < j; i++)
	    {
		u_i1 *lblopr = offset + (u_i1*)ctx->patdata->vec + 1;
		offset = GETNUM(lblopr);
		offset += lblopr - (u_i1*)ctx->patdata->vec+1+NUMSZ(0);
	    }
	    /* Point to PAT_LABEL & get offset to its PAT_JUMP, The
	    ** case lenghts  were calculated in case processing as
	    ** they became known. */
	    inst = offset + (u_i1*)ctx->patdata->vec + 1;
	    patlen = GETNUM(inst);
	    /* Point to PAT_JUMP. Its param will be 0 so we can compute
	    ** the offset to the endcase (wrtptr).
	    */
	    jinst = inst + patlen;
	    skplen = ctx->wrtptr - (jinst+1+NUMSZ(0));
	    jopr = jinst + 1;
	    /* Fix jump offset now known */
	    pat_update_num(&jopr, skplen, &ctx->wrtptr);
	    /* Now we know the size of the jump instr - add to label
	    ** size. */
	    skplen = jopr - jinst;
	    inst = offset + (u_i1*)ctx->patdata->vec + 1;
	    pat_update_num(&inst, skplen, &ctx->wrtptr);
	}
    }
    inst = ctx->rep_ctxs[sp].inst + (u_i1*)ctx->patdata->vec;
    inst++;
    offset = GETNUM(inst);
    offset = GETNUM(inst);
    offset = ctx->wrtptr - inst - 1;
    pat_update_num(&inst, offset, &ctx->wrtptr);
    if (NUMSZ(-offset-4)== 3)
	pat_op_1(ctx, PAT_NEXT, -offset-4);
    else if (NUMSZ(-offset-6)== 5)
	pat_op_1(ctx, PAT_NEXT, -offset-6);
    else
	pat_op_1(ctx, PAT_NEXT, -offset-10);
    /* Make last op be the PAT_FOR for range to adjust */
    ctx->last_op = ctx->rep_ctxs[sp].inst + (u_i1*)ctx->patdata->vec;
    ctx->prelit_op = NULL;
    return db_stat;
}


/*
** Name: pat_set_case() - Set case entry in repeat context
**
**  This routine handles alternation in the repeat context. It will NOT
**  be called for the outermost level which cannot have a repeat context.
**  Eg: 'pat1|pat2' will not get this handling but '(pat1|pat2)' would.
**  In the latter case the outer level has only one alternation, level 1
**  has 2.
**
**  When the alternation character is found, a case context is initialised
**  and the PAT_CASE and PAT_LABEL instructions injected, straight after
**  the PAT_FOR instruction. The PAT_CASE operand will be fixed up at each
**  subsequent alternation. Subsequent alternation characters just add a
**  PAT_JUMP and PAT_LABEL to the stream.
**
** Inputs:
**	ctx		Pattern build context
**
** Outputs:
**	Mone
**
** Returns:
**	statue		True if range error found
**
** Exceptions:
**	none
**
** Side Effects:
**	Updates parse context stack
**
** History:
**      29-Jul-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_set_case(PAT_BLD *ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    u_i4 sp = ctx->rep_depth;
    u_i1 *inst;
    i4 l = 1 + NUMSZ(0) + 1 + NUMSZ(0);
    u_i4 patlen;
    if (!sp--)
    {
	/* FIXME error */
    }
    if (!ctx->rep_ctxs[sp].caseopr)
    {
	i8 v;
	/* Up to now this was a simple repeat block - now it must
	** become a case block too. This involves injecting the
	** PAT_CASE 1/PAT_LABEL n sequence */
	/* Do any reallocation now before we set&use pointers */
	if (db_stat = chk_space(ctx, 2*l))
	    return db_stat;
	inst = ctx->rep_ctxs[sp].inst + (u_i1*)ctx->patdata->vec;
	/* Skip PAT_FOR */
	if (adu_pat_decode(&inst, 0, 0, 0, 0, 0))
	{
	    /* FIXME error */
	}
	/* Make space for injected instructions */
	MEmemmove(inst+l, inst, ctx->wrtptr-inst);
	ctx->wrtptr += l;
	/* PAT_CASE 1 AND keep location of count operand (relative) */
	*inst++ = PAT_CASE;
	ctx->rep_ctxs[sp].caseopr = inst - (u_i1*)ctx->patdata->vec;
	PUTNUM0(inst);
	/* PAT_LABEL patlen */
	ctx->rep_ctxs[sp].prlbl = inst - (u_i1*)ctx->patdata->vec;
	ctx->no_of_ops++;
	*inst++ = PAT_LABEL;
	PUTNUM0(inst);
    }
    else if (db_stat = chk_space(ctx, l))
	return db_stat;

    /* Fix prior label - was set to 0, wrtptr is at end of last case
    ** alternation so length (needed for label) is wrtptr - instr after
    ** label. At this point we don't know the size of the jumps so
    ** exclude for now - fix later */
    inst = ctx->rep_ctxs[sp].prlbl + (u_i1*)ctx->patdata->vec + 1;
    patlen = ctx->wrtptr - inst - NUMSZ(0);
    pat_update_num(&inst, patlen, &ctx->wrtptr);
    /* Add an extra case block - incr case count */
    inst = ctx->rep_ctxs[sp].caseopr + (u_i1*)ctx->patdata->vec;
    pat_update_num(&inst, 1, &ctx->wrtptr);

    if (db_stat = pat_op_1(ctx, PAT_JUMP, 0))
	return db_stat;

    ctx->rep_ctxs[sp].prlbl = ctx->wrtptr - (u_i1*)ctx->patdata->vec;
    return pat_op_1(ctx, PAT_LABEL, 0);
}


/*
** Name: pat_apply_repeat() - Set regexp range params into operator
**
**  This routine updates the operator stream to complete the setting
**  of the repeat counts.
**
** Inputs:
**	ctx		Pattern build context
**	lo		lower bound
**	hi		upper bound
**	wild		Whether hi is infinite or not
**
** Outputs:
**	Mone
**
** Returns:
**	statue		True if range error found
**
** Exceptions:
**	none
**
** Side Effects:
**	Updates parse context stack
**
** History:
**      29-Jul-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_apply_repeat(PAT_BLD *ctx, i4 Ndelta, i4 Mdelta, bool wild)
{
    DB_STATUS db_stat = E_DB_OK;
    u_i1 *p = ctx->last_op;
    enum PAT_OP_enums op = *p, new_op;
    u_i4 N = opcodes[op].v1, M = 0;
    i4 opN, olen;

    if (opcodes[op].ty == opTy_FOR && wild)
    {
	u_i1 *Br, *p2 = p, opNext;
	/* Decode branch to find PAT_NEXT to convert to _W */
	adu_pat_decode(&p2, 0, 0, 0, 0, &Br);
	if (*Br != PAT_NEXT)
	{
	    /* FIXME error */
	}
	else
	{
	    *Br = find_op(opcodes[*Br].ty, V1__, V2_W, wild);
	    wild = FALSE; /* Dealt with in the NEXT */
	}
    }
    p++;
    if (opcodes[op].p1 == P1_N)
	N = GETNUM(p);
    if (opcodes[op].p2 == P2_M)
	M = GETNUM(p);
    if (!wild && opcodes[op].v2 == V2_W)
	wild = TRUE;
    olen = p - ctx->last_op - 1;
    if (Ndelta != -1 || N)
	N += Ndelta;
    M += Mdelta;

    new_op = find_op(opcodes[op].ty, N < V1__ ? N : V1__, M, wild);

    if (new_op == PAT_BUGCHK ||
	new_op == op && (N != 0 && opcodes[op].p1 != P1_N ||
			 M != 0 && opcodes[op].p2 != P2_M))
    {
	u_i1 *inst, *hold;
	i4 l;
#if PAT_DBG_TRACE>0
	if (ADU_pat_debug & PAT_DBG_TRACE_ENABLE)
	    TRdisplay("Failed to find OP\n");
	if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
	    trace_op(ctx->last_op, FALSE);
	}
#endif
	/*
	** An attempt to repeat an instruction with no matching
	** repeat varient - setup as a repeat block.
	*/
	if (db_stat = chk_space(ctx, 8))
	    return db_stat;
	inst = ctx->last_op;
	/* We need an extra 8 bytes to open up a FOR & NEXT block
	** about the last instruction as it has no integral repeat
	** capability */
	l = ctx->wrtptr - inst;
	MEmemmove(inst+4, inst, l);
	ctx->no_of_ops++;
	*inst++ = PAT_FOR_N_M;
	PUTNUM0(inst);
	PUTNUM0(inst);
	PUTNUM0(inst);
	inst -= 3;
	ctx->wrtptr += 4; 
	pat_update_num(&inst, Ndelta+1, &ctx->wrtptr);
	pat_update_num(&inst, Mdelta, &ctx->wrtptr);
	pat_update_num(&inst, l, &ctx->wrtptr);
	op = wild ? PAT_NEXT_W : PAT_NEXT;
	pat_op_1(ctx, op, -l - (
		NUMSZ(-l-4) == 3
		    ? 4
		    : NUMSZ(-l-6)== 5
			? 6
			: 10));
    }
    else
    {
	i4 nlen = 0;
#if PAT_DBG_TRACE>0
	if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
	    TRdisplay("Applying range OP\n");
	    trace_op(ctx->last_op, FALSE);
	}
#endif
	if (opcodes[new_op].p1 == P1_N)
	    nlen += NUMSZ(N);
	if (opcodes[new_op].p2 == P2_M)
	    nlen += NUMSZ(M);
	if (olen != nlen)
	{
	    MEmemmove(p - (olen - nlen), p, ctx->wrtptr - p);
	    ctx->wrtptr -= (olen - nlen);
	}
	p = ctx->last_op;
	*p++ = new_op;
	if (opcodes[new_op].p1 == P1_N)
	    PUTNUM(p, N);
	if (opcodes[new_op].p2 == P2_M)
	    PUTNUM(p, M);
#if PAT_DBG_TRACE>0
	if (ADU_pat_debug & PAT_DBG_TRACE_COALESCE){
	    TRdisplay("... into OP\n");
	    trace_op(ctx->last_op, FALSE);
	}
#endif
    }
    return db_stat;
}

/*
** Name: adu_pat_decode() - Decode operator
**
**      This routine decodes one pattern operator.
**
** Inputs:
**	pc1		Instruction address
**
** Outputs:
**	pc1		Updated PC
**	N		Value of N if set
**	M		Value of M if set -1 if wild
**	L		Value of string length if set
**	S		Absolute address of string - or NULL if not set
**	B		Absolute address of branch dest - or NULL if not set
**
** Returns:
**	bool	If TRUE bad instruction encountered
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      30-Jul-2008 (kiria01) SIR120473
**          Initial creation.
**	28-Jul-2009 (kiria01) b122377
**	    Exported routine.
*/
bool
adu_pat_decode(
    u_i1 **pc1,
    i4 *N,
    i4 *M,
    i4 *L,
    u_i1 **S,
    u_i1 **B
)
{
    u_i1 *pc = *pc1;
    u_i1 op = *pc++;

    if (op >= PAT_OP_MAX)
    {
	return TRUE;
    }
    else
    {
	i4 v;
	if (opcodes[op].p1 != P1__)
	{
	    v = GETNUM(pc);
	    if (N)
		*N = v;
	}
	if (opcodes[op].p2 != P2__)
	{
	    v = GETNUM(pc);
	    if (M)
		*M = v;
	}
	else if (opcodes[op].v2 == V2_W)
	{
	    if (M)
		*M = -1;
	}
	switch (opcodes[op].cl)
	{
	case opClass__:
	    break;
	case opClass_C:/* CASE */
	    v = GETNUM(pc);
	    if (L)
		*L = v;
	    break;
	case opClass_J:/* Jump/Label */
	    v = GETNUM(pc);
	    if (B)
		*B = pc+v;
	    break;
	case opClass_L:/* Literal text */
	case opClass_S:/* Subset */
	    v = GETNUM(pc);
	    if (L)
	    {
		*L = v;
		if (S)
		    *S = pc;
	    }
	    pc += v;
	    break;
	case opClass_NS:/* NSubset */
	    v = GETNUM(pc);
	    if (L)
	    {
		*L = v;
		v = GETNUM(pc);
		if (S)
		    *S = pc;
	    }
	    pc += v;
	    break;
	case opClass_B:/* Bitset */
	case opClass_NB:/* Bitset */
	    v = GETNUM(pc);
	    v = GETNUM(pc);
	    pc += v;
	    break;
	}
    }
    *pc1 = pc;
    return FALSE;
}


#if PAT_DBG_TRACE>0
/*
** Name: adu_pat_disass() - Disassemble pattern operator
**
**      This routine disassembles one pattern operator and formats it into
**	the supplied buffer. Note that ths routine may need to format more
**	than one line of data at a time. To call this routine use as follows...
**
**		u_i8 v = 0;
**		char *p = buf;
**		while (adu_pat_disass(buf, sizeof(buf), &pc, &p, &v, FALSE))
**		{
**		    *p++ = '\n';
**		    TRwrite(0, p - buf, buf);
**		    p = buf;
**		    *p++ = '\t';
**		}
**		*p++ = '\n';
**		TRwrite(0, p - buf, buf);
**
** Inputs:
**	buf		Buffer to receive result data
**	buflen		Length of buffer
**	pc		Operator address
**	p		Location in buf to start writing to
**	v		Context - unitially 0
**	uni		Use unicode interpretation
**
** Outputs:
**	buf		Updated with text
**	pc		Updated with next instruction address
**	p		Updated write pointer
**	v		Updated context
**
** Returns:
**	bool	If true, routine should be called again with
**		same parameters to retrieve continuation lines.
**		Otherwise, the record in the buffer is complete.
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
bool
adu_pat_disass(
    char *buf,
    u_i4 buflen,
    u_i1 **pc1,
    char **p1,
    i8 *v1,
    bool unifl)
{
    static u_i1 hex[] = "0123456789ABCDEF";
    u_i1 *pc = *pc1;
    char *p = *p1;
    i8 v = *v1;
    u_i4 uni = unifl ? 1 : 0;

    if (v > 0)
    {
	while (v--)
	{
	    UCS2 c2 = *((UCS2*)pc);
	    u_i1 c = *pc++;
	    if (p+uni >= buf+buflen-5)
	    {
		*p1 = p;
		*pc1 = pc-1;
		*v1 = v+1;
		return 1;
	    }
	    if (uni)
	    {
		pc++;
		v--;
		*p++ = hex[(c2>>12)&15];
		*p++ = hex[(c2>>8)&15];
		*p++ = hex[(c2>>4)&15];
		*p++ = hex[(c2>>0)&15];
		*p++ = ' ';
	    }
	    else if (isprint(c) && c != '\'')
		*p++ = c;
	    else
	    {
		*p++ = '\\';
		*p++ = 'x';
	    }
	}
	*p++ = '\'';
    }
    else if (v < 0)
    {
	while (v++)
	{
	    i4 i = 7;
	    UCS2 c2 = *((UCS2*)pc);
	    u_i1 c = *pc++;
	    if (p+uni >= buf+buflen-10)
	    {
		*p1 = p;
		*pc1 = pc-1;
		*v1 = v-1;
		return 1;
	    }
	    do
	    {
		if (c&1)
		    *p++ = '1';
		else
		    *p++ = '0';
		c /= 2;
	    } while(i--);
	    if (v)
		*p++ = ' ';
	}
	*p++ = '\'';
    }
    else
    {
	u_i1 op;
	i8 offset, count;
	i4 n;
	op = *pc++;
	if (op >= PAT_OP_MAX)
	{
	    STcat(p, "Bad op code:");
	    p += STlength(p);
	    CVla8((u_i8)op, p);
	    p += STlength(p);
	}
	else
	{
	    if (opcodes[op].cl == opClass_J)
	    {
		/* If Jump or label - hint at pc */
		*p++ = '#';
		CVptrax(pc-1, p);
		p += STlength(p);
		*p++ = '\n';
	    }
	    STcopy(opcodes[op].opname, p);
	    p += STlength(p);
	    if (opcodes[op].p1 != P1__)
	    {
		*p++ = ' ';
		v = GETNUM(pc);
		CVla8(v, p);
		p += STlength(p);
	    }
	    if (opcodes[op].p2 != P2__)
	    {
		*p++ = ' ';
		v = GETNUM(pc);
		CVla8(v, p);
		p += STlength(p);
	    }
	    switch (opcodes[op].cl)
	    {
	    case opClass__:
		break;
	    case opClass_C:/* CASE */
		*p++ = ' ';
		v = GETNUM(pc);
		CVla8(v, p);
		p += STlength(p);
		break;
	    case opClass_J:/* Jump/Label */
		*p++ = ' ';
		v = GETNUM(pc);
		CVla8(v, p);
		p += STlength(p);
		/* Output a pc hint */
		*p++ = ' ';
		*p++ = '#';
		CVptrax(pc+v, p);
		p += STlength(p);
		break;
	    case opClass_L:/* Literal text */
	    case opClass_S:/* Subset */
	    case opClass_NS:/* Subset */
		*p++ = ' ';

		v = GETNUM(pc);
		CVla8(v, p);
		p += STlength(p);
		*p++ = ' ';
		if (opcodes[op].cl == opClass_NS)
		{
		    u_i1 *pc2 = pc;
		    i8 v2 = GETNUM(pc);
		    v -= pc-pc2;
		    CVla8(v2, p);
		    p += STlength(p);
		    *p++ = ' ';
		}
		*p++ = '\'';
		while (v--)
		{
		    UCS2 c2 = *((UCS2*)pc);
		    u_i1 c = *pc++;
		    if (p+uni >= buf+buflen-5)
		    {
			*p1 = p;
			*pc1 = pc-1;
			*v1 = v+1;
			return 1;
		    }
		    if (uni)
		    {
			pc++;
			v--;
			*p++ = hex[(c2>>12)&15];
			*p++ = hex[(c2>>8)&15];
			*p++ = hex[(c2>>4)&15];
			*p++ = hex[(c2>>0)&15];
			*p++ = ' ';
		    }
		    else if (isprint(c) && c != '\'')
			*p++ = c;
		    else
		    {
			*p++ = '\\';
			*p++ = 'x';
			*p++ = hex[(c>>4)&15];
			*p++ = hex[(c>>0)&15];
		    }
		}
		*p++ = '\'';
		break;
	    case opClass_B:/* Bitset */
	    case opClass_NB:/* Bitset */
		*p++ = ' ';
		offset = GETNUM(pc);
		CVla8(offset, p);
		p += STlength(p);
		*p++ = ' ';
		v = GETNUM(pc);
		CVla8(v, p);
		p += STlength(p);
		*p++ = ' ';
		*p++ = '\'';
		while(v--)
		{
		    u_i1 x = *pc++;
		    i4 i = 7;
		    if (p - buf >= (i4)buflen - 10)
		    {
			*p1 = p;
			*pc1 = pc-1;
			*v1 = -v-1;
			return 1;
		    }
		    do
		    {
			if (x&1)
			    *p++ = '1';
			else
			    *p++ = '0';
			x /= 2;
		    } while(i--);
		    if (v)
			*p++ = ' ';
		}
		*p++ = '\'';
		break;
	    }
	}
    }
    *pc1 = pc;
    *p1 = p;
    return 0;
}


/*
** Name: pat_dump() - Dump compiled pattern to an ASCII file
**
**      This routine disassembles a pattern list into a file
**
** Inputs:
**	patdata		Pattern data to dump
**	fname		File name to write to.
**
** Outputs:
**	None
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	Creates a file
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static VOID
pat_dump(
	AD_PATDATA	*patdata,
	char		*fname)
{
    LOCATION loc;
    FILE *fd = NULL;
    STATUS rval;
    bool tr = FALSE;
    u_i4 i;
    u_i4 base = PATDATA_1STFREE;
    i4 uni = 0;

    if (!fname || !STcompare(fname, "TRACE"))
    {
	tr = TRUE;
    }
    else if (LOfroms(PATH&FILENAME, fname, &loc) != OK ||
	(rval = SIopen(&loc, "w", &fd)) != OK)
    {
	TRdisplay("Can't open debugging file (%s)\n SIopen returns %p",
		fname, rval);
	return;
    }
    {
	char buf[4096], *p = buf;
	static const char *fl_esc[] = {
	    "AD_PAT_NO_ESCAPE",
	    "AD_PAT_HAS_ESCAPE",
	    "AD_PAT_DOESNT_APPLY",
	    "AD_PAT_HAS_UESCAPE"
	};
#define _DEFINE(n,v,t) t,
#define _DEFINEEND
	static const char *names[] = { AD_PAT_FORMS };
#undef _DEFINEEND
#undef _DEFINE
	SIprintf(p, "*PATTERN-LIST %d %x %d\n",
		       patdata->patdata.npats,
		       patdata->patdata.flags+
			    ((u_i4)patdata->patdata.flags<<16),
		       patdata->patdata.length);
	p += STlength(p);
	*p++ = '#'; /* Issue flags decode as a comment */
	*p++ = ' ';
	if (patdata->patdata.flags & AD_PAT_FORM_NEGATED)
	{
	    STcopy("NOT ", p);
	    p += STlength(p);
	}
	STcopy(names[(patdata->patdata.flags & AD_PAT_FORM_MASK) >> 8], p);
	p += STlength(p);
	*p++ = ',';
	STcopy(fl_esc[patdata->patdata.flags&AD_PAT_ISESCAPE_MASK], p);
	p += STlength(p);
	if (patdata->patdata.flags & AD_PAT_WO_CASE)
	{
	    STcopy(",WO_CASE", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags & AD_PAT_WITH_CASE)
	{
	    STcopy(",WITH_CASE", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags & AD_PAT_DIACRIT_OFF)
	{
	    STcopy(",DIACRIT_OFF", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags & AD_PAT_BIGNORE)
	{
	    STcopy(",BIGNORE", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_COLLATE)
	{
	    STcopy(",COLLATE", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_UNICODE_CE)
	{
	    STcopy(",UNICODE_CE", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_UCS_BASIC)
	{
	    STcopy(",UCS_BASIC", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_UTF8_UCS_BASIC)
	{
	    STcopy(",UTF8_UCS_BASIC", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_ENDLIT)
	{
	    STcopy(",ENDLIT", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_MATCH)
	{
	    STcopy(",MATCH", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_LITERAL)
	{
	    STcopy(",LITERAL", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_PURE_LITERAL)
	{
	    STcopy(",PURE_LITERAL", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_FORCE_FAIL)
	{
	    STcopy(",FORCE_FAIL", p);
	    p += STlength(p);
	}
	if (patdata->patdata.flags2 & AD_PAT2_RSVD_UNUSED)
	{
	    SIprintf(p, ",RSVD_UNUSED=%x", patdata->patdata.flags & AD_PAT2_RSVD_UNUSED);
	    p += STlength(p);
	}
	*p++ = '\n';
	*p++ = EOS;
	if (tr)
	    TRdisplay(buf);
	else
	    SIfprintf(fd, buf);
    }
    if (patdata->patdata.flags2 & (AD_PAT2_UCS_BASIC|AD_PAT2_UNICODE_CE))
	uni = 1;

    for (i = 0; i < patdata->patdata.npats &&
		base < patdata->patdata.length; i++)
    {
	u_i4 patlen = patdata->vec[base];
	u_i1 *pc = (u_i1*)&patdata->vec[base+1];
	u_i1 *patbufend = (u_i1*)&patdata->vec[base+patlen];
	i4 n;
	char buf[256];

	if (tr)
	    TRdisplay("*PATTERN %d\n", patbufend - pc);
	else
	    SIfprintf(fd, "*PATTERN %d\n", patbufend - pc);
	while (pc < patbufend)
	{
	    i8 v = 0;
	    char *p = buf;
	    while (adu_pat_disass(buf, sizeof(buf), &pc, &p, &v, uni))
	    {
		*p++ = '\n';
		if (tr)
		    TRwrite(0, p - buf-1, buf);
		else
		    SIwrite(p - buf, buf, &n, fd);
		p = buf;
		*p++ = '\t';
	    }
	    *p++ = '\n';
	    if (tr)
		TRwrite(0, p - buf-1, buf);
	    else
		SIwrite(p - buf, buf, &n, fd);
	}
	if (tr)
	    TRdisplay("*END-PATTERN\n");
	else
	    SIfprintf(fd, "*END-PATTERN\n");
	base += patlen;
    }
    if (tr)
	TRdisplay("*END-PATTERN-LIST\n");
    else
    {
	SIfprintf(fd, "*END-PATTERN-LIST\n");
	SIclose(fd);
    }
}


/*
** Name: pat_dbg() - Dump compiled pattern to trace
**
**      This routine disassembles a pattern sequence to dbmslog
**
** Inputs:
**	pc		Pattern data to dump
**	pcend		Upto address or 0 for 1 shot
**	uni		Non-zero if unicode literals
**
** Outputs:
**	None
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	Writes to trace file
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static VOID
pat_dbg(
	u_i1		*pc,
	u_i1		*pcend,
	i4		uni)
{
    char buf[100];
    TRdisplay("----------\n");
    do
    {
	i8 v = 0;
	char *p = buf;
	while (adu_pat_disass(buf, sizeof(buf), &pc, &p, &v, uni))
	{
	    TRwrite(0, p - buf, buf);
	    p = buf;
	    *p++ = '\t';
	}
	TRwrite(0, p - buf, buf);
    } while(pc < pcend);
    TRdisplay("----------\n");
}


/*
** Name: pat_dump_ctx() - Dump sea_ctx to file 
**
**      This routine disassembles a pattern list into a file
**
** Inputs:
**	patdata		Pattern data to dump
**	fname		File name to write to.
**
** Outputs:
**	None
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	Creates a file
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
VOID
adu_patcomp_dump (
	PTR	data,
	char	*fname)
{
    union {
	PTR ptr;
	AD_PAT_SEA_CTX *sea_ctx;
	DB_DATA_VALUE *dv;
    } pdata;
    AD_PATDATA *patdata;

    if (!fname)
	fname = "TRACE";
    pdata.ptr = data;
    if (pdata.dv->db_datatype == DB_PAT_TYPE)
	patdata = (AD_PATDATA*)pdata.dv->db_data;
    else
	patdata = pdata.sea_ctx->patdata;
    pat_dump(patdata, fname);
}


/*
** Name: pat_load() - Load pattern from file
**
**      This routine assembles a pattern list from a file
**
** Inputs:
**	pat_ctx		Pattern search context to load into
**	fname		File name to read from.
**
** Outputs:
**	.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static DB_STATUS
pat_load(
PAT_BLD		*pat_ctx,
char		*fname)
{
    DB_STATUS db_stat;
    AD_PAT_CTX *ctx = 0;
    LOCATION loc;
    FILE *fd = NULL;
    STATUS rval;
    char buf[512];
    bool in_pat = FALSE;
    bool in_list = FALSE;

    /* Drop any pre-load pattern fragment. (Part pat_bld_init) */
    pat_ctx->patdata->patdata.npats = 0;
    pat_ctx->patdata->patdata.length = PATDATA_1STFREE;
    pat_ctx->prelit_op = NULL;
    pat_ctx->last_op = &dummy_op;
    pat_ctx->force_fail = FALSE;

    if (LOfroms(PATH&FILENAME, fname, &loc) != OK ||
	(rval = SIopen(&loc, "r", &fd)) != OK)
    {
	TRdisplay("Can't open debugging file (%s)\n SIopen returns %p",
		fname, rval);
	return rval;
    }
    while (SIgetrec(buf, sizeof(buf), fd) == OK)
    {
	char *words[10];
	i4 n = 10;
	i8 v, v2, offset, count;
	char *p;
	u_i1 *p2;

	if (*buf == '#')
	    continue;
	STgetwords(buf, &n, words);
	if (!in_list && n == 4 &&
	    !STcompare("*PATTERN-LIST", words[0]))
	{
	    u_i4 flags = 0;
	    if (CVahxl(words[2], &flags) ||
		CVal8(words[3], &v))
	    {
		TRdisplay("*PATTERN-LIST parm error\n");
		pat_ctx->force_fail = TRUE;
	    }
	    v *= sizeof(i2);
	    chk_space(pat_ctx, (u_i4)v);
	    in_list = TRUE;
	}
	else if (in_list &&
	    !STcompare("*END-PATTERN-LIST", words[0]))
	{
	    in_list = FALSE;
	    if (in_pat)
	    {
		TRdisplay("Missing *END-PATTERN\n");
		pat_ctx->force_fail = TRUE;
	    }
	    in_pat = 0;
	}
	else if (!in_pat && n == 2 &&
	    !STcompare("*PATTERN", words[0]))
	{
	    u_i4 flags = 0;
	    in_pat = TRUE;
	    pat_bld_finpat(pat_ctx, 0);
	    if (db_stat = pat_bld_newpat(pat_ctx, NULL))
		return db_stat;

	    if (CVal8(words[1], &v))
	    {
		TRdisplay("*PATTERN parm error\n");
		pat_ctx->force_fail = TRUE;
	    }
	    chk_space(pat_ctx, (i4)v);
	}
	else if (in_pat &&
	    !STcompare("*END-PATTERN", words[0]))
	{
	    in_pat = FALSE;
	}
	else if (!in_pat)
	{
	    TRdisplay("Not in pattern - line dropped\n");
	    pat_ctx->force_fail = TRUE;
	}
	else
	{
	    i4 op, operand = 1;
	    for (op = 0; op < PAT_OP_MAX; op++)
	    {
		if (!STcompare(opcodes[op].opname, words[0]))
		    break;
	    }
	    if (op == PAT_OP_MAX)
	    {
		TRdisplay("OP not recognised - %s\n", words[0]);
		pat_ctx->force_fail = TRUE;
		continue;
	    }

	    *pat_ctx->wrtptr++ = op;

	    if (opcodes[op].p1 == P1_N)
	    {
		if (!words[operand])
		{
		    TRdisplay("Param N missing - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		if (CVal8(words[operand], &v))
		{
		    TRdisplay("Param N syntax %s %s\n", words[0], words[operand]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		PUTNUM(pat_ctx->wrtptr, v);
		operand++;
	    }
	    if (opcodes[op].p2 == P2_M)
	    {
		if (!words[operand])
		{
		    TRdisplay("Param M missing - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		if (CVal8(words[operand], &v))
		{
		    TRdisplay("Param M syntax %s %s %s\n", words[0], words[1], words[operand]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		PUTNUM(pat_ctx->wrtptr, v);
		operand++;
	    }
	    switch (opcodes[op].cl)
	    {
	    case opClass__:
		if (words[operand])
		{
		    TRdisplay("Extra params - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		break;
	    case opClass_C:/* CASE */
	    case opClass_J:/* Jump/Label */
		if (!words[operand])
		{
		    TRdisplay("Numeric param missing - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		if (CVal8(words[operand], &v))
		{
		    TRdisplay("Param J/F syntax %s ... %s\n", words[0], words[operand]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		operand++;
		PUTNUM(pat_ctx->wrtptr, v);
		break;
	    case opClass_L:/* Literal text */
	    case opClass_S:/* Subset */
	    case opClass_NS:/* Subset */
		if (!words[operand])
		{
		    TRdisplay("String length param missing - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		if (CVal8(words[operand], &v) || v < 0)
		{
		    TRdisplay("Param len syntax %s ... %s\n", words[0], words[operand]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		operand++;
		PUTNUM(pat_ctx->wrtptr, v);
		p2 = pat_ctx->wrtptr + v;
		v2 = -1;
		if (opcodes[op].cl == opClass_NS)
		{
		    if (!words[operand])
		    {
			TRdisplay("Aux String length param missing - %s\n", words[0]);
			pat_ctx->force_fail = TRUE;
			continue;
		    }
		    if (CVal8(words[operand], &v2) || v2 < 0)
		    {
			TRdisplay("Param len2 syntax %s ... %s\n", words[0], words[operand]);
			pat_ctx->force_fail = TRUE;
			continue;
		    }
		    operand++;
		    v -= NUMSZ(v2);
		    PUTNUM(pat_ctx->wrtptr, v2);
		}
		if (!(p = words[operand++]))
		{
		    TRdisplay("String param missing - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}

		while (v--)
		{
		    u_i1 c = *p++;
		    if (c == '\\')
		    {
			u_i1 ch;
			if (*p == '\n')
			{
			    if (SIgetrec(buf, sizeof(buf), fd) != OK)
			    {
				TRdisplay("Continuation record not got\n");
				pat_ctx->force_fail = TRUE;
				break;
			    }
			    p = buf+1;
			    v++;
			    continue;
			}
			if (*p++ != 'x')
			{
			    TRdisplay("Escape char x expected\n");
			    pat_ctx->force_fail = TRUE;
			    break;
			}
			c = *p++;
			c -= '0';
			if (c > 9)
			{
			    c += '0'-'A'+10;
			    if (c>15)
				c += 'A'-'a';
			}
			if (c > 15)
			{
			    TRdisplay("Non-hex digit\n");
			    pat_ctx->force_fail = TRUE;
			    break;
			}
			ch = c * 16;
			c = *p++;
			c -= '0';
			if (c > 9)
			{
			    c += '0'-'A'+10;
			    if (c>15)
				c += 'A'-'a';
			}
			if (c > 15)
			{
			    TRdisplay("Non-hex digit\n");
			    pat_ctx->force_fail = TRUE;
			    break;
			}
			ch |= c;
			*pat_ctx->wrtptr++ = ch;
		    }
		    else
		    	*pat_ctx->wrtptr++ = c;
		}
		if (pat_ctx->wrtptr != p2)
		{
		    TRdisplay("String mis-write\n");
		    pat_ctx->force_fail = TRUE;
		}
		break;
	    case opClass_B:/* Bitset */
	    case opClass_NB:/* Bitset */
		if (!words[operand])
		{
		    TRdisplay("Offset param expected - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		CVal8(words[operand++], &offset);
		PUTNUM(pat_ctx->wrtptr, offset);
		if (!words[operand])
		{
		    TRdisplay("Count param expected - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		CVal8(words[operand++], &count);
		PUTNUM(pat_ctx->wrtptr, count);
		if (!(p = words[operand++]))
		{
		    TRdisplay("Bitset param expected - %s\n", words[0]);
		    pat_ctx->force_fail = TRUE;
		    continue;
		}
		while(count)
		{
		    u_i1 x = 0;
		    i4 i = 7;
		    do
		    {
			x /= 2;
			switch(*p++)
			{
			case '1':
			    x |= 128;
			    break;
			case '0':
			    break;
			default:
			    TRdisplay("Bad bit value - %s\n", words[0]);
			    pat_ctx->force_fail = TRUE;
			    break;
			}
		    } while(i--);
		    if (--count)
		    {
			if (*p == '\\')
			{
			    if (SIgetrec(buf, sizeof(buf), fd) != OK)
			    {
				TRdisplay("Continuation record not got\n");
				pat_ctx->force_fail = TRUE;
				break;
			    }
			    p = buf+1;
			}
			else if (*p++ != ' ')
			{
			    TRdisplay("Bitset structure problem - %s\n", words[0]);
			    pat_ctx->force_fail = TRUE;
			    break;
			}
		    }
		    *pat_ctx->wrtptr++ = x;
		}
		break;
	    }
	}
    }
    pat_bld_finpat(pat_ctx, 0);
    SIclose(fd);
    return db_stat;
}
#endif


/*
** Name: adu_patcomp_set_pats() - Move pattern buffer to sea ctx
**
**      This routine Rolls up the pattern list and moves it into search context
**	creating as many search contexts (AD_PAT_CTX) as needed
**
** Inputs
**	sea_ctx		    SEA_CTX to transfer into
**	patdata		    patdata context to transfer from
**
** Outputs:
**	sea_ctx		    Receives pattern list
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**	Might allocate memory
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
DB_STATUS
adu_patcomp_set_pats (AD_PAT_SEA_CTX *sea_ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    u_i4 i;
    u_i4 base = PATDATA_1STFREE;
    AD_PATDATA *patdata = sea_ctx->patdata;
    for (i = 0; i < patdata->patdata.npats &&
		base < patdata->patdata.length; i++)
    {
	AD_PAT_CTX *ctx, **p = &sea_ctx->stalled;
	u_i4 patlen = patdata->vec[base];
	if (ctx = sea_ctx->free)
	{
	    sea_ctx->free = ctx->next;
	    ctx->next = NULL;
	}
	else
	{
	    ctx = (AD_PAT_CTX *)MEreqmem(0, sizeof(*ctx), FALSE, &db_stat);
	    if (db_stat)
		return db_stat;
	}
	ctx->next = NULL;
	/* Establish parent block */
	ctx->parent = sea_ctx;
	ctx->pc = (u_i1*)&patdata->vec[base+1];
	ctx->patbufend = (u_i1*)&patdata->vec[base+patlen];
	ctx->litset = ctx->litsetend = NULL;
	/* Link to stalled list */
	while (*p)
	    p = &(*p)->next;
	*p = ctx;
	base += patlen;
    }
    return db_stat;
}


/*
** Name: adu_patcomp_free() - Release compiled pattern resources
**
**      This routine frees allocated memoryfrom pattern building.
**
** Inputs:
**	sea_ctx		Pointer to the SEA_CTX to be released
**
** Returns:
**	Void
**
** Exceptions:
**	none
**
** Side Effects:
**	Releases memory
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**	22-Aug-2008 (toumi01)
**	    Free patdata to avoid memory leak.
*/
VOID
adu_patcomp_free(AD_PAT_SEA_CTX *sea_ctx)
{
    AD_PAT_CTX *ctx, *endctx;
    if (!sea_ctx)
	return;

    while(ctx = sea_ctx->pending)
    {
	sea_ctx->pending = ctx->next;
	if ((u_i4)(ctx - sea_ctx->ctxs) >= sea_ctx->nctxs)
	    MEfree((char*)ctx);
    }
    while(ctx = sea_ctx->stalled)
    {
	sea_ctx->stalled = ctx->next;
	if ((u_i4)(ctx - sea_ctx->ctxs) >= sea_ctx->nctxs)
	    MEfree((char*)ctx);
    }
    while(ctx = sea_ctx->free)
    {
	sea_ctx->free = ctx->next;
	if ((u_i4)(ctx - sea_ctx->ctxs) >= sea_ctx->nctxs)
	    MEfree((char*)ctx);
    }
    if (sea_ctx->setbuf)
	MEfree(sea_ctx->setbuf);

#if PAT_DBG_TRACE>0
    if (sea_ctx->infile)
	MEfree(sea_ctx->infile);
    if (sea_ctx->outfile)
	MEfree(sea_ctx->outfile);
#endif
}


/*
** Name: prs_num*() - Parse base 10 unsigned integer
**
**      These routines parse a simple number advancing the read poiner as
**	it progresses. prs_num() deals with normal ASCII while prs_num_uni()
**	handles the Unicode UTF-16.
**
** Inputs
**	p	'read' pointer
**	e	End of buffer
**	n	Address of number to return result.
**
** Outputs:
**	*p	Updated 'read' pointer
**	n
**
** Returns:
**	none - leave unparsed data as is.
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static VOID
prs_num(const u_char **p, const u_char *e, u_i4 *np)
{
    i4 n = 0;
    while (*p < e)
    {
	i4 c = **p - '0';
	if (c < 0 || c > 9 || I4_MAX/10 <= n)
	    break;
	n = 10*n + c;
	(*p)++;
    }
    *np = n;
}

static VOID
prs_num_uni(const UCS2 **p, const UCS2 *e, u_i4 *np)
{
    i4 n = 0;
    while (*p < e)
    {
	i4 c = **p - '0';
	if (c < 0 || c > 9 || I4_MAX/10 <= n)
	    break;
	n = 10*n + c;
	(*p)++;
    }
    *np = n;
}


/*
** Name: cmp_*() - Compare routines for use with pat_set_add()
**
**      These routines localise the comparisons needed to handle
**	the collation/data type form to be dealt with.
**	    cmp_simple
**	    cmp_collation
**	    cmp_unicode
**	    cmp_unicodeCE
**
** Inputs
**	arg	Argument passed from pat_set_add()
**	a0	Start address of LHS
**	a1	End address of LHS
**	b0	Start address of RHS
**	b1	End address of RHS
**
** Outputs:
**	None
**
** Returns:
**	i4	Result of compare
**
** Exceptions:
**	none
**
** Side Effects:
**	None
**
** History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
*/
static i4
cmp_simple(PTR arg_unused, const u_i1*a, u_i4 al, const u_i1*b, u_i4 bl)
{
    i4 res = 0;
    u_i4 i = 0;
    while (i < al && i < bl && !(res = ((u_char*)a)[i] - ((u_char*)b)[i]))
	i++;
    if (res)
	return res;
    /* Disable any adjacency coalesce if different lengths */
    return 2*(al - bl);
}

static i4
cmp_collating(PTR arg, const u_i1*a, u_i4 al, const u_i1*b, u_i4 bl)
{
    ADULTABLE *adf_collation = (ADULTABLE *)arg;
    ADULcstate la, lb;
    adulstrinitn(adf_collation, (u_char*)a, al, &la);
    adulstrinitn(adf_collation, (u_char*)b, bl, &lb);
    return adultrans(&la) - adultrans(&lb);
}

static i4
cmp_unicode(PTR arg_unused, const u_i1*a, u_i4 al, const u_i1*b, u_i4 bl)
{
    i4 res = 0;
    u_i4 i = 0;
    while (i < al && i < bl && !(res = ((UCS2*)a)[i/2] - ((UCS2*)b)[i/2]))
	i+=sizeof(UCS2);
    if (!res)
	return res;
    /* Disable any adjacency coalesce if different lengths */
    return 2*(al - bl);
}

static i4
cmp_unicodeCE(PTR arg_unused, const u_i1*a, u_i4 al, const u_i1*b, u_i4 bl)
{
    i4 res = 0;
    u_i4 i = 0;
    while (i < al && i < bl && !(res = ((UCS2*)a)[i/2] - ((UCS2*)b)[i/2]))
	i+=sizeof(UCS2);
    if (!res)
	res = al - bl;
    /* Disable any adjacency coalesce */
    return 2*res;
}


/*
** Name: adu_patcomp_like() - ADF function instance to return 
**
** Description:
**      This function parses the pattern and builds the equivalent list of
**	operators.
**	Some optimisation will be performed on the generated opcodes to
**	remove redundant wild cards etc.
**	The scan is one-pass with the potential for merging operators left
**	to the pat_op_* routines that inspect the last opcode written.
**
**	This is the non-Unicode compiler - see adu_patcomp_like_uni() for
**	Unicode variant.
**
**	NOTE: on collation handling. This routine is collation aware but for
**	the purposes of LIKE, we will store the raw characters as we would
**	with the non-collated form. On the whole the non-collation pattern
**	will be the same as the collation aware form. The only difference
**	will be with sets. With collation, bitset will be disabled and the
**	range specifier will be checked for lo-hi validity in the set code.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	pat_dv				Pointer to dbv structure defining
**					the pattern.
**	esc_dv				Pointer to dbv structure defining the
**					single character escape. (May be NULL)
**	pat_flags			Search modifier flags
**	sea_ctx				Pointer to the AD_PAT_SEA_CTX
**					address of the structure created for
**					the pattern
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9998_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      04-Apr-2008 (kiria01)
**          Initial creation.
*/

DB_STATUS
adu_patcomp_like(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*pat_dv,
	DB_DATA_VALUE	*esc_dv,
	u_i4		pat_flags,
	AD_PAT_SEA_CTX	*sea_ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_CTX *ctx = NULL;
    const u_char *p, *e, *f;
    const u_char *lit = NULL;
    u_char temp_esc[8];
    i4 len;
    PAT_BLD pat_ctx;
    bool use_collation = FALSE;
    bool allow_pat = TRUE;
    bool regexp = FALSE;
    bool pure = TRUE;
    i4 (*cmp)(PTR, const u_i1*, u_i4, const u_i1*, u_i4) = cmp_simple;

    /*
    ** Parse out the following wild characters.
    ** Essentially controlled by the two variables allow_pat and regexp.
    ** For BEGINING, CONTAINING and ENDING, both will be false.
    ** For LIKE, allow_pat is true.
    ** For SIMILAR TO, both are true.
    **  Without escape:
    **      LS   _		1 char
    **      LS   %		0 or more chars
    **   BCELS   \I=file	\
    **   BCELS   \O=file	 > if compiletime selected
    **	 BCELS   \T=?		/
    **       S	 [...]		1 off char set
    **	     S   (...)		Bracketed unit
    **	     S   ^		In set marks negation
    **	    LS   -		In set marks range
    **	     S   |		Alternation operator
    **	     S   {n,m}		Repeat modifier: n-m off
    **       S   ?		Repeat modifier: 0 or 1 off
    **       S   *		Repeat modifier: 0 or more off
    **       S   +		Repeat modifier: 1 or more off
    **
    **  With escape @ active add:
    **   BCELS   @I=file	\
    **   BCELS   @O=file	 > if compiletime selected
    **	 BCELS   @T=?		/
    **      LS   @_		Literal _
    **      LS   @%		Literal %
    **      L    @[....@]	1 off char set
    **       S	 @^		Literal ^
    **       S	 @-		Literal -
    **       S	 @[		Literal [
    **       S	 @]		Literal ]
    **   BCEL    @|		Alternation operator
    **       S   @|		Literal |
    **   BCELS   @@		Literal esc char
    **	     S   @(		Literal (
    **	     S   @)		Literal )
    **	     S   @{		Literal {
    **       S   @?		Literal ?
    **       S   @*		Literal *
    **       S   @+		Literal +
    */
    if (pat_flags & (AD_PAT2_COLLATE<<16))
    {
	use_collation = TRUE;
	cmp = cmp_collating;
    }

    pat_bld_init(adf_scb, sea_ctx, &pat_ctx, pat_flags);

    if (esc_dv && (pat_flags&AD_PAT_ISESCAPE_MASK)==AD_PAT_HAS_UESCAPE)
    {
	DB_DATA_VALUE temp_dv;
	temp_dv.db_length = sizeof(temp_esc);
	temp_dv.db_data = (PTR)temp_esc;
	temp_dv.db_datatype = DB_CHA_TYPE;
	temp_dv.db_collID = DB_UNSET_COLL;
	temp_dv.db_prec = 0;
	if (db_stat = adu_nvchr_unitochar(adf_scb, esc_dv, &temp_dv))
	    return db_stat;
    }
    else if (esc_dv && (pat_flags&AD_PAT_ISESCAPE_MASK)==AD_PAT_HAS_ESCAPE)
    {
	CMcpychar(esc_dv->db_data, temp_esc);
    }
    else
    {
	if (esc_dv)
	    esc_dv = NULL;
        temp_esc[0] = '\\';
    }
    if (db_stat = adu_lenaddr(adf_scb, pat_dv, &len, (char **)&p))
    {
	/* sea_ctx will be freed in adu_patcomp_free */
	return (db_stat);
    }

    pat_bld_newpat(&pat_ctx, p);
    /*
    ** If SIMILAR TO allow regexp.
    ** If CONTAINING or ENDING, inject initial match any operator.
    ** If CONTAINING or ENDING or BEGINNING - disallow patterns.
    */
    switch (pat_flags & AD_PAT_FORM_MASK)
    {
    case AD_PAT_FORM_SIMILARTO:
	regexp = TRUE;
	break;
    case AD_PAT_FORM_CONTAINING:
    case AD_PAT_FORM_ENDING:
	if (db_stat = pat_op_wild(&pat_ctx))
	    return db_stat;
	/* FALLTHROUGH */
    case AD_PAT_FORM_BEGINNING:
	allow_pat = FALSE;
	pure = FALSE;
    }
    e = p + len;
    while (p < e)
    {
	bool handle_set = FALSE;
	if (!CMcmpcase(p, temp_esc))
	{
	    pure = FALSE;
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    /* Eat escape */
	    CMnext(p);
	    /* Handle the debug options - always enabled even without
	    ** ESCAPE modifier */
	    if (p >= e)
	    {
		if (esc_dv)
		    return adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0);
		p -= CMbytecnt(temp_esc);
	    }
#if PAT_DBG_TRACE>0
	    /*DEBUG extensions always active even if no ESCAPE*/
	    else if (ADU_pat_debug && p[1] == '=')
	    {
		if (*p == 'I')
		{
		    char *fp;
		    p += 2;
		    for (f = p; p < e && CMcmpcase(p, temp_esc); CMnext(p))
			/*SKIP*/;
		    fp = sea_ctx->infile = MEreqmem(0, p - f + 1, FALSE, &db_stat);
		    if (db_stat)
			return db_stat;
		    while (f < p)
			*fp++ = (char)*f++;
		    *fp = 0;
		    continue;
		}
		else if (*p == 'O')
		{
		    char *fp;
		    p += 2;
		    for (f = p; p < e && CMcmpcase(p, temp_esc); CMnext(p))
			/*SKIP*/;
		    fp = sea_ctx->outfile = MEreqmem(0, p - f + 1, FALSE, &db_stat);
		    if (db_stat)
			return db_stat;
		    while (f < p)
			*fp++ = (char)*f++;
		    *fp = 0;
		    continue;
		}
		else if (*p == 'T')
		{
		    p += 2;
		    pat_op_simple(&pat_ctx, (*p++ == '0')
			    ?PAT_NOTRACE:PAT_TRACE);
		    continue;
		}
	    }
#endif

	    /* Now check for ESCAPE modified */
	    if (esc_dv)
	    {
		if (*p == AD_7LIKE_BAR && !regexp)
		{
		    p++;
		    if (((pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_BEGINNING ||
			 (pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_CONTAINING) &&
			 (db_stat = pat_op_wild(&pat_ctx)))
			    return db_stat;
		    if (db_stat = pat_op_simple(&pat_ctx, PAT_FINAL))
			return db_stat;
		    pat_bld_finpat(&pat_ctx, p);

		    pat_bld_newpat(&pat_ctx, p);
		    if (((pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_CONTAINING ||
			 (pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_ENDING) &&
			 (db_stat = pat_op_wild(&pat_ctx)))
			    return db_stat;
		}
		else if (*p == AD_3LIKE_LBRAC && allow_pat && !regexp)
		{
		    handle_set = TRUE;
		}
		else if (allow_pat && (*p == AD_1LIKE_ONE ||
					*p == AD_2LIKE_ANY) ||
			    regexp && (*p == AD_3LIKE_LBRAC ||
					*p == AD_4LIKE_RBRAC ||
					*p == AD_8LIKE_LPAR ||
					*p == AD_9LIKE_RPAR ||
					*p == AD_7LIKE_BAR ||
					*p == AD_6LIKE_CIRCUM ||
					*p == AD_5LIKE_RANGE ||
					*p == AD_13LIKE_PLUS ||
					*p == AD_12LIKE_STAR ||
					*p == AD_14LIKE_QUEST ||
					*p == AD_10LIKE_LCURL
			    ) ||
			!CMcmpcase(p, temp_esc))
		{
		    lit = p;
		    CMnext(p);
		}
		else
		{
		    /* Not a recognised escaped char */
		    return adu_error(adf_scb,
			regexp ? E_AD101D_BAD_ESC_SEQ : allow_pat
			    ? E_AD1018_BAD_ESC_SEQ : E_AD101C_BAD_ESC_SEQ, 0);
		}
	    }
	    else
	    {
		/* leave escape char in stream as a literal */
		lit = temp_esc;
		/* literal the escape & start new one */
		if (db_stat = pat_op_lit(&pat_ctx, &lit, lit+CMbytecnt(lit)))
		    return db_stat;
		lit = p;
	    }
	}
	else if (*p == AD_1LIKE_ONE && allow_pat)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    if (db_stat = pat_op_any_one(&pat_ctx))
		return db_stat;
	    p++;
	}
 	else if (*p == AD_2LIKE_ANY && allow_pat)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    if (db_stat = pat_op_wild(&pat_ctx))
		return db_stat;
	    p++;
	}
	else if (*p == AD_3LIKE_LBRAC && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    handle_set = TRUE;
	}
	else if (*p == AD_8LIKE_LPAR && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    if (p[1] == AD_9LIKE_RPAR || p[1] == AD_7LIKE_BAR)
		return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
	    /* push sub-pattern context */
	    if (db_stat = pat_rep_push(&pat_ctx))
		return db_stat;
	    p++;
	    continue;	/* No repeat ctx */
	}
	else if (*p == AD_9LIKE_RPAR && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    /* pop sub-pattern context */
	    if (db_stat = pat_rep_pop(&pat_ctx))
		return db_stat;
	    p++;
	}
	else if (*p == AD_7LIKE_BAR && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    /* Alternation */
	    if (!pat_ctx.rep_depth)
	    {
		/* No need for case handling - keep simple */
		if (db_stat = pat_op_simple(&pat_ctx, PAT_FINAL))
		    return db_stat;
		pat_bld_finpat(&pat_ctx, p);

		pat_bld_newpat(&pat_ctx, p);
	    }
	    else
	    {
		if (p[1] == AD_9LIKE_RPAR || p[1] == AD_7LIKE_BAR)
		    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
		if (db_stat = pat_set_case(&pat_ctx))
		    return db_stat;
	    }
	    p++;
	    continue;	/* No repeat ctx */
	}
	else if ((pat_flags & AD_PAT_BIGNORE) && (CMspace(p) || !*p) ||
		use_collation && /* Don't store zero width collation chars */
		adul0widcol(adf_scb->adf_collation, p))
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
		return db_stat;
	    CMnext(p);
	}
	else /* Literal */
	{
	    if (!lit)
		lit = p;
	    CMnext(p);
	}
	if (handle_set)
	{
	    PAT_SET **setbuf = (PAT_SET**)&sea_ctx->setbuf;
	    PAT_SET pat_set_stk;
	    i4 count = 0;
	    bool bitset_valid = !use_collation;
	    bool neg = FALSE, part_neg = FALSE;
	    bool in_range = FALSE;
	    bool allow_rng = FALSE;
	    u_char ch;
	    u_char v, set[256/8];
	    u_i4 lo = 256, hi = 0, dash = 0;
	    const u_char *save_p = NULL, *save_e;

	    pure = FALSE;
	    if (bitset_valid)
		MEfill(256/8, 0, set);
	    if (!*setbuf)
	    {
		pat_set_stk.lim = PAT_SET_STK_DIM;
		pat_set_stk.part = 0;
		pat_set_stk.last = 0;
		*setbuf = &pat_set_stk;
	    }
	    (*setbuf)->n = 0;
	    p++;
	    if (*p == AD_6LIKE_CIRCUM &&
		(regexp ||
		 allow_pat && (ADU_pat_debug & PAT_DBG_NEG_ENABLE))
		)
	    {
		p++;
		neg = TRUE;
	    }
	    while (p < e)
	    {
		i4 l = CMbytecnt(p);
		i4 bit;
		u_char *byt, tmp[8];
		const u_char *pt, *pte;

		/* Handle ESCAPE char */
		if (!CMcmpcase(p, temp_esc))
		{
		    p += l;
		    if (*p == AD_4LIKE_RBRAC && !regexp)
		    {
			goto rbrac_common;
		    }
		    if ((!allow_pat || (*p != AD_1LIKE_ONE &&
					*p != AD_2LIKE_ANY)) &&
			    (!regexp || (*p != AD_3LIKE_LBRAC &&
					*p != AD_4LIKE_RBRAC &&
					*p != AD_8LIKE_LPAR &&
					*p != AD_9LIKE_RPAR &&
					*p != AD_7LIKE_BAR &&
					*p != AD_6LIKE_CIRCUM &&
					*p != AD_5LIKE_RANGE &&
					*p != AD_13LIKE_PLUS &&
					*p != AD_12LIKE_STAR &&
					*p != AD_14LIKE_QUEST &&
					*p != AD_10LIKE_LCURL
			    )) &&
			CMcmpcase(p, temp_esc))
		    {
			if ((*setbuf)->lim == PAT_SET_STK_DIM)
			    sea_ctx->setbuf = NULL;
			return adu_error(adf_scb, regexp ? E_AD101D_BAD_ESC_SEQ : allow_pat
			    ? E_AD1018_BAD_ESC_SEQ : E_AD101C_BAD_ESC_SEQ, 0);
		    }
		    l = CMbytecnt(p);
		    goto ch_common;
		}
		else if (*p == AD_6LIKE_CIRCUM && regexp && !neg && !part_neg)
		{
		    p++;
		    part_neg = TRUE;
		    (*setbuf)->part = (*setbuf)->n;
		    continue;
		}
		else if (*p == AD_3LIKE_LBRAC && regexp && p[1] == AD_16LIKE_COLON)
		{
		    const u_char *c = chclasses;
		    if (*temp_esc == AD_16LIKE_COLON)
		    {
			if ((*setbuf)->lim == PAT_SET_STK_DIM)
			    sea_ctx->setbuf = NULL;
			return adu_error(adf_scb, E_AD101E_ESC_SEQ_CONFLICT, 0);
		    }
		    p += 2;
		    for (;;)
		    {
			const u_char *pp = p;
			i4 res;
			while (*c)
			{
			    char t[4];
			    CMtolower(pp, t);
			    pp++;
			    if (res = *c++ - *t)
				break;
			}
			while (*c++)
			    /*SKIP*/;
			if (!res)
			{
			    if (*pp != AD_16LIKE_COLON || pp[1] != AD_4LIKE_RBRAC)
			    {
				/* Expected :] in char class */
				if ((*setbuf)->lim == PAT_SET_STK_DIM)
				    sea_ctx->setbuf = NULL;
				return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
			    }
			    /* Temporarily inject char class eqv */
			    save_p = pp + 2; save_e = e;
			    p = e = c;
			    while (*e++)
				/*SKIP*/;
			    break;
			}
			while (*c++)
			    /*SKIP*/;
			if (!*c)
			{
			    /* Unrecognised char class */
			    if ((*setbuf)->lim == PAT_SET_STK_DIM)
				sea_ctx->setbuf = NULL;
			    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
			}
		    }
		}
		else if (!*p && regexp && save_p)
		{
		    /* Undo temporary injection of char class */
		    p = save_p; e = save_e;
		    save_p = NULL;
		    continue;
		}
		else if (*p == AD_4LIKE_RBRAC && regexp)
		{
rbrac_common:	    if (in_range)
		    {
			/* Trailing '-' */
			if ((*setbuf)->lim == PAT_SET_STK_DIM)
			    sea_ctx->setbuf = NULL;
			return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
		    }
		    p++;
		    /* Clean exit */
		    if (bitset_valid && (
			    count>(i4)(hi - lo)/2 && /* dense enough */
				(neg || lo != hi)|| /* not singleton */
			    part_neg))
			db_stat = pat_op_bitset(&pat_ctx, neg, lo, hi, count, set);
		    else
			db_stat = pat_op_set(&pat_ctx, neg, *setbuf, 
					    cmp, adf_scb->adf_collation);
		    if (!db_stat)
			goto set_ok;
		    if ((*setbuf)->lim == PAT_SET_STK_DIM)
			sea_ctx->setbuf = NULL;
		    return db_stat;
		}
		else if (*p == AD_5LIKE_RANGE &&
			!in_range &&
			allow_rng)
		{
		    /* Handle range in the next section */
		    in_range = TRUE;
		    p++;
		}
		else if (*p == AD_1LIKE_ONE ||
			*p == AD_2LIKE_ANY ||
			*p == AD_3LIKE_LBRAC)
		{
		    if ((*setbuf)->lim == PAT_SET_STK_DIM)
			sea_ctx->setbuf = NULL;
		    return adu_error(adf_scb, E_AD1016_PMCHARS_IN_RANGE, 0);
		}
		else
		{
ch_common:	    /* If ANY set character is MB then we
		    ** will not support BITSET */
		    if ((pat_flags & AD_PAT_WO_CASE) && CMupper(p))
		    {
			CMtolower(p, tmp);
			pt = tmp;
			pte = pt + 1;
		    }
		    else
		    {
			pt = p;
			pte = e;
		    }
		    if (l > 1)
			bitset_valid = FALSE;
		    else if (bitset_valid)
		    {
			/* Bitset handling */
			if (!in_range)
			{
			    /* Setup for a bit range of 1 :-) */
			    v = *pt;
    				
			    /* Is single char or range start
			    ** could be a new low */
			    if (v < lo)
				lo = v;
			    if (v > hi)
				hi = v;
			}
			else if (v > *pt)
			{
			    /* Sanity test - start before end*/
			    if ((*setbuf)->lim == PAT_SET_STK_DIM)
				sea_ctx->setbuf = NULL;
			    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
			}
			for(;v <= *pt; v++)
			{
			    bit = 1<<(v&7);
			    byt = &set[v>>3];
			    if (part_neg)
			    {
				if ((*byt & bit))
				{
				    /* Clear the bits */
				    set[v>>3] &= ~(1<<(v&7));
				    count--;
				}
			    }
		    	    else if (!(*byt & bit))
			    {
				/* Set the bits */
			        set[v>>3] |= 1<<(v&7);
		    		count++;
			    }
			    if (v == *pt)
				break;
			}
			if (v > hi)
			    hi = v;
		    }
		    /*
		    ** Once out of this block - may only allow next
		    ** char to be valid '-' if this char is not a range terminal.
		    */
		    allow_rng = !in_range;

		    /* Setup the none Bitset anyway incase needed.
		    ** For collation support bwe need to see the length of the
		    ** character.
		    */
		    if (use_collation &&
			    adul0widcol(adf_scb->adf_collation, (u_char*)pt))
			/* Drop zero width characters from set */
			l = 1;

		    else if (use_collation)
		    {
			ADULcstate lp;
			adulstrinitp((ADULTABLE *)adf_scb->adf_collation,
				    (u_char*)pt, (u_char*)pte, &lp);
			adultrans(&lp);
			adulnext(&lp);
			l = adulptr(&lp)-pt;
			if (pat_set_add(setbuf, pt, l, in_range, cmp, adf_scb->adf_collation))
			    /* Drop out of loop to return error
			    ** E_AD1015_BAD_RANGE
			    */
			    break;
		    }
		    else if (pat_set_add(setbuf, pt, l, in_range, cmp, adf_scb->adf_collation))
			/* Drop out of loop to return error
			** E_AD1015_BAD_RANGE
			*/
			break;
		    in_range = FALSE;
		    p += l;
		}
	    }
	    /*missing ]*/
	    if ((*setbuf)->lim == PAT_SET_STK_DIM)
		sea_ctx->setbuf = NULL;
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
set_ok:	    if ((*setbuf)->lim == PAT_SET_STK_DIM)
		sea_ctx->setbuf = NULL;
	}
	if (regexp && p < e)
	{
	    u_i4 lo = 0, hi = 0;
	    bool wild = FALSE;
	    const u_char *old_p = p;
	    if (*p == AD_10LIKE_LCURL)
	    {
		p++;
		prs_num(&p, e, &lo);

		if (p < e && *p == AD_15LIKE_COMMA)
		{
		    p++;
		    if (p < e && *p == AD_11LIKE_RCURL)
		    {
			wild = TRUE;
			hi = lo;
		    }
		    else
		    {
			prs_num(&p, e, &hi);
			if (hi < lo)
			    /* Repeat counts out of order */
			    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
		    }
		}
		else
		    hi = lo;
		if (p < e && *p == AD_11LIKE_RCURL)
		    p++;
		else
		    /* '}' expected */
		    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
		if (!wild && lo == 1 && hi == 1)
		    continue;
	    }
	    else if (*p == AD_12LIKE_STAR)
	    {
		p++;
		lo = hi = 0;
		wild = TRUE;
	    }
	    else if (*p == AD_13LIKE_PLUS)
	    {
		p++;
		lo = hi = 1;
		wild = TRUE;
	    }
	    else if (*p == AD_14LIKE_QUEST)
	    {
		p++;
		lo = 0;
		hi = 1;
	    }
	    else 
		continue;
	    /* Force out any part literal - runs will coalesce */
	    if (lit && (db_stat = pat_op_lit(&pat_ctx, &lit, old_p)))
		return db_stat;

	    /* Here we have the repeat parsed out into lo, hi and wild.
	    ** The last_op will point to the instruction that the repeat
	    ** counts are to apply to.
	    ** NOTE:
	    ** One problem at this point relates to PAT_LIT. It will be
	    ** representing (usually) an unbroken sequence of literal
	    ** text but the range must only apply to the last character.
	    ** this will require that the instruction be split and as
	    ** the PAT_LIT has no repeat count variants we model the
	    ** repeating character as a single char set. Thus:
	    ** 'ABCD{4,8}' starts as: PAT_LIT 4 'ABCD'
	    ** but is treated as: 'ABC[D]{4,8}' and the instructions
	    ** are then: PAT_LIT 3 'ABC' and PAT_SET_N_M 4 4 1 'D'
	    */
	    if (*pat_ctx.last_op == PAT_LIT)
	    {
		u_i1 *pc;
		u_i4 l;
		chk_space(&pat_ctx, 8);
		pat_ctx.prelit_op = NULL; /* Can't ignore trailing context */
		pc = pat_ctx.last_op + 1;
		l = GETNUM(pc);
		if (l > 1)
		{
		    /* Split the literal at the last CHAR! */
		    u_i1 *s1 = pc, *s2;
		    pc += l;
		    s2 = pc;
		    CMprev(s2, s1);
		    if (s2 > s1)
		    {
			i4 l2 = pc - s2;
			MEmemmove(s2 + sizeof(u_i1)+sizeof(u_i1), s2, l2);
			pat_ctx.wrtptr += sizeof(u_i1)+sizeof(u_i1);
			s1 = pat_ctx.last_op + 1;
			pat_ctx.last_op = s2;
			*s2++ = PAT_LIT;
			PUTNUM(s2, l2);
			pat_update_num(&s1, (i8)-l2, &pat_ctx.wrtptr);
		    }
		}
		pc = pat_ctx.last_op;
		*pc++ = PAT_SET_1;
		l = GETNUM(pc);
		if (*pc == AD_5LIKE_RANGE)
		{
		    *pat_ctx.wrtptr++ = AD_5LIKE_RANGE;
		    *pat_ctx.wrtptr++ = AD_5LIKE_RANGE;
		    pc = pat_ctx.last_op + 1;
		    pat_update_num(&pc, (i8)2, &pat_ctx.wrtptr);
		}
	    }
	    if (db_stat = pat_apply_repeat(&pat_ctx, lo-1, hi-lo, wild))
		return db_stat;
	}
    }
    if (db_stat = pat_op_lit(&pat_ctx, &lit, p))
	return db_stat;
    /*
    ** If BEGINNING or CONTAINING, inject final match any operator
    */
    if ((pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_BEGINNING ||
	(pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_CONTAINING)
    {
	if (db_stat = pat_op_wild(&pat_ctx))
	    return db_stat;
    }
    if (db_stat = pat_op_simple(&pat_ctx, PAT_FINAL))
	return db_stat;

    pat_bld_finpat(&pat_ctx, (u_i1*)p);

#if PAT_DBG_TRACE>0
    /*
    ** If \I=file was specified - throw away and pull opcode stream from file
    */
    if (sea_ctx->infile)
    {
	if (db_stat = pat_load(&pat_ctx, sea_ctx->infile))
	    pat_ctx.force_fail = TRUE;
	MEfree(sea_ctx->infile);
	sea_ctx->infile = NULL;
    }
#endif

    if (db_stat = adu_patcomp_set_pats(sea_ctx))
	return db_stat;

#if PAT_DBG_TRACE>0
    /*
    ** If \O=file was specified - dump the pattern buffers now
    ** (even if they did just came from the file)
    */
    if (sea_ctx->outfile)
    {
	pat_dump(sea_ctx->patdata, sea_ctx->outfile);
	pat_ctx.force_fail = TRUE;
	MEfree(sea_ctx->outfile);
	sea_ctx->outfile = NULL;
    }
#endif
    if (!pat_ctx.force_fail)
	sea_ctx->force_fail = FALSE;
    if (sea_ctx->patdata->patdata.npats == 1)
    {
	if (pat_ctx.no_of_ops == 1 ||
	    pat_ctx.no_of_ops == 2 &&
		sea_ctx->patdata->patdata.first_op == PAT_LIT)
	{
	    if (pure)
		sea_ctx->patdata->patdata.flags2 |= AD_PAT2_PURE_LITERAL;
	    sea_ctx->patdata->patdata.flags2 |= AD_PAT2_LITERAL;
	}
	else if (sea_ctx->patdata->patdata.first_op == PAT_MATCH &&
		pat_ctx.no_of_ops == 2)
	{
	    sea_ctx->patdata->patdata.flags2 |= AD_PAT2_MATCH;
	}
	else if (sea_ctx->patdata->patdata.first_op == PAT_ENDLIT &&
		pat_ctx.no_of_ops == 2)
	{
	    sea_ctx->patdata->patdata.flags2 |= AD_PAT2_ENDLIT;
	}
    }
    return db_stat;
}


/*
** Name: adu_patcomp_like_uni() - ADF function instance to return 
**
** Description:
**      This function parses the pattern and builds the equivalent list of
**	operators.
**	Some optimisation will be performed on the generated opcodes to
**	remove redundant wild cards etc.
**	The scan is one-pass with the potential for merging operators left
**	to the pat_op_* routines that inspect the last opcode written.
**	This is the Unicode variant and is very similar to the standard
**	version except for the character sixe and the the absence of BITSET
**	processing.
**
**	This is the Unicode compiler - see adu_patcomp_like() for
**	non-Unicode variant.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	pat_dv				Pointer to dbv structure defining
**					the pattern.
**	esc_dv				Pointer to dbv structure defining the
**					single character escape. (May be NULL)
**	pat_flags			Search modifier flags
**	sea_ctx				Pointer to the AD_PAT_SEA_CTX
**					address of the structure created for
**					the pattern
**
**	Returns:
**	    E_AD0000_OK			Completed successfully.
**          E_AD9998_INTERNAL_ERROR	If the data types are unexpected
**
**	Exceptions:
**	    none
**
** History:
**      04-Apr-2008 (kiria01)
**          Initial creation.
**	30-Jan-2009 (kiria01) SIR120473 
**	    When splitting off a trailing character due to repetition
**	    handling remember that it is a unit of CE data we lop off and
**	    not just a single character.
*/

DB_STATUS
adu_patcomp_like_uni(
	ADF_CB		*adf_scb,
	DB_DATA_VALUE	*pat_dv,
	DB_DATA_VALUE	*esc_dv,
	u_i4		pat_flags,
	AD_PAT_SEA_CTX	*sea_ctx)
{
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_CTX *ctx = NULL;
    const UCS2 *p, *e;
    const UCS2 *lit = NULL;
    UCS2 temp_esc[4];
    const UCS2 *f;
    i4 len;
    PAT_BLD pat_ctx;
    bool allow_pat = TRUE;
    bool regexp = FALSE;
    bool pure = TRUE;
    DB_STATUS (*pat_op_set_fn)(PAT_BLD*, i4, PAT_SET*) = pat_op_CEset;
    DB_STATUS (*pat_op_lit_fn)(PAT_BLD*, const UCS2 **, const UCS2 *) = pat_op_CElit;

    /*
    ** Parse out the following wild characters
    ** Essentially controlled by the two variables allow_pat and regexp.
    ** For BEGINING, CONTAINING and ENDING, both will be false.
    ** For LIKE, allow_pat is true.
    ** For SIMILAR TO, both are true.
    **  Without escape:
    **      LS   _		1 char
    **      LS   %		0 or more chars
    **   BCELS   \I=file	\
    **   BCELS   \O=file	 > if compiletime selected
    **	 BCELS   \T=?		/
    **       S	 [...]		1 off char set
    **	     S   (...)		Bracketed unit
    **	     S   ^		In set marks negation
    **	    LS   -		In set marks range
    **	     S   |		Alternation operator
    **	     S   {n,m}		Repeat modifier: n-m off
    **       S   ?		Repeat modifier: 0 or 1 off
    **       S   *		Repeat modifier: 0 or more off
    **       S   +		Repeat modifier: 1 or more off
    **
    **  With escape @ active add:
    **   BCELS   @I=file	\
    **   BCELS   @O=file	 > if compiletime selected
    **	 BCELS   @T=?		/
    **      LS   @_		Literal _
    **      LS   @%		Literal %
    **      L    @[....@]	1 off char set
    **       S	 @^		Literal ^
    **       S	 @-		Literal -
    **       S	 @[		Literal [
    **       S	 @]		Literal ]
    **   BCEL    @|		Alternation operator
    **       S   @|		Literal |
    **   BCELS   @@		Literal esc char
    **	     S   @(		Literal (
    **	     S   @)		Literal )
    **	     S   @{		Literal {
    **       S   @?		Literal ?
    **       S   @*		Literal *
    **       S   @+		Literal +
    */
    if ((pat_flags & (AD_PAT2_UCS_BASIC|AD_PAT2_UNICODE_CE)<<16) == 0)
	pat_flags |= (AD_PAT2_UCS_BASIC<<16);
    
    pat_bld_init(adf_scb, sea_ctx, &pat_ctx, pat_flags);
    if (pat_flags & (AD_PAT2_UCS_BASIC<<16))
    {
	/* Swap out SET and LIT functions to not store CE */
	pat_op_set_fn = pat_op_uset;
	pat_op_lit_fn = pat_op_ulit;
    }
    if (esc_dv && (pat_flags&AD_PAT_ISESCAPE_MASK)==AD_PAT_HAS_ESCAPE)
    {
	DB_DATA_VALUE temp_dv;
	temp_dv.db_length = sizeof(temp_esc[0]);
	temp_dv.db_data = (PTR)&temp_esc[0];
	temp_dv.db_datatype = DB_NCHR_TYPE;
	temp_dv.db_collID = DB_UNSET_COLL;
	temp_dv.db_prec = 0;
	if (db_stat = adu_nvchr_coerce(adf_scb, esc_dv, &temp_dv))
	    return db_stat;
    }
    else if (esc_dv && (pat_flags&AD_PAT_ISESCAPE_MASK)==AD_PAT_HAS_UESCAPE)
    {
	I2ASSIGN_MACRO(*(UCS2*)esc_dv->db_data, temp_esc[0]);
    }
    else
    {
	if (esc_dv)
	    esc_dv = NULL;
        temp_esc[0] = '\\';
    }
    if (db_stat = adu_lenaddr(adf_scb, pat_dv, &len, (char **)&p))
    {
	/* sea_ctx will be freed in adu_patcomp_free */
	return (db_stat);
    }

    pat_bld_newpat(&pat_ctx, (u_i1*)p);
    /*
    ** If SIMILAR TO allow regexp.
    ** If CONTAINING or ENDING, inject initial match any operator.
    ** If CONTAINING or ENDING or BEGINNING - disallow patterns.
    */
    switch (pat_flags & AD_PAT_FORM_MASK)
    {
    case AD_PAT_FORM_SIMILARTO:
	regexp = TRUE;
	break;
    case AD_PAT_FORM_CONTAINING:
    case AD_PAT_FORM_ENDING:
	if (db_stat = pat_op_wild(&pat_ctx))
	    return db_stat;
	/* FALLTHROUGH */
    case AD_PAT_FORM_BEGINNING:
	allow_pat = FALSE;
	pure = FALSE;
    }
    e = p + len/sizeof(UCS2);
    while (p < e)
    {
	bool handle_set = FALSE;
	if (*p == temp_esc[0])
	{
	    pure = FALSE;
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    /* Eat escape */
	    p++;
	    /* Handle the debug options - always enabled even without
	    ** ESCAPE modifier */
	    if (p >= e)
	    {
		if (esc_dv)
		    return adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0);
		p--;
	    }
#if PAT_DBG_TRACE>0
	    /*DEBUG extensions always active even if no ESCAPE*/
	    else if (ADU_pat_debug && p[1] == '=')
	    {
		if (*p == 'I')
		{
		    char *fp;
		    p += 2;
		    for (f = p; p < e && *p != *temp_esc; p++)
			/*SKIP*/;
		    fp = sea_ctx->infile = MEreqmem(0, p - f + 1, FALSE, &db_stat);
		    if (db_stat)
			return db_stat;
		    while (f < p)
			*fp++ = (char)*f++;
		    *fp = 0;
		    continue;
		}
		else if (*p == 'O')
		{
		    char *fp;
		    p += 2;
		    for (f = p; p < e && *p != *temp_esc; p++)
			/*SKIP*/;
		    fp = sea_ctx->outfile = MEreqmem(0, p - f + 1, FALSE, &db_stat);
		    if (db_stat)
			return db_stat;
		    while (f < p)
			*fp++ = (char)*f++;
		    *fp = 0;
		    continue;
		}
		else if (*p == 'T')
		{
		    p += 2;
		    pat_op_simple(&pat_ctx, (*p++ == '0')
			    ?PAT_NOTRACE:PAT_TRACE);
		    continue;
		}
	    }
#endif
	    /* Now check for ESCAPE modified */
	    if (esc_dv)
	    {
		if (*p == AD_7LIKE_BAR && !regexp)
		{
		    p++;
		    if (((pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_BEGINNING ||
			 (pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_CONTAINING) &&
			 (db_stat = pat_op_wild(&pat_ctx)))
			    return db_stat;
		    if (db_stat = pat_op_simple(&pat_ctx, PAT_FINAL))
			return db_stat;
		    pat_bld_finpat(&pat_ctx, (u_i1*)p);

		    pat_bld_newpat(&pat_ctx, (u_i1*)p);
		    if (((pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_CONTAINING ||
			 (pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_ENDING) &&
			 (db_stat = pat_op_wild(&pat_ctx)))
			    return db_stat;
		}
		else if (allow_pat && *p == AD_3LIKE_LBRAC && !regexp)
		{
		    handle_set = TRUE;
		}
		else if (allow_pat && (*p == AD_1LIKE_ONE ||
					*p == AD_2LIKE_ANY) ||
			    regexp && (*p == AD_3LIKE_LBRAC ||
					*p == AD_4LIKE_RBRAC ||
					*p == AD_8LIKE_LPAR ||
					*p == AD_9LIKE_RPAR ||
					*p == AD_7LIKE_BAR ||
					*p == AD_6LIKE_CIRCUM ||
					*p == AD_5LIKE_RANGE ||
					*p == AD_13LIKE_PLUS ||
					*p == AD_12LIKE_STAR ||
					*p == AD_14LIKE_QUEST ||
					*p == AD_10LIKE_LCURL
			    ) ||
			*p == *temp_esc)
		{
		    lit = p;
		    p++;
		}
		else
		{
		    /* Not a recognised escaped char */
		    return adu_error(adf_scb, regexp ? E_AD101D_BAD_ESC_SEQ : allow_pat
			    ? E_AD1018_BAD_ESC_SEQ : E_AD101C_BAD_ESC_SEQ, 0);
		}
	    }
	    else
	    {
		/* leave escape char in stream as a literal */
		lit = temp_esc;
		/* literal the escape & start new one */
		if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, lit+1))
		    return db_stat;
		lit = p;
	    }
	}
	else if (*p == AD_1LIKE_ONE && allow_pat)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    if (db_stat = pat_op_any_one(&pat_ctx))
		return db_stat;
	    p++;
	}
 	else if (*p == AD_2LIKE_ANY && allow_pat)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    if (db_stat = pat_op_wild(&pat_ctx))
		return db_stat;
	    p++;
	}
	else if (*p == AD_3LIKE_LBRAC && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    handle_set = TRUE;
	}
	else if (*p == AD_8LIKE_LPAR && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    if (p[1] == AD_9LIKE_RPAR || p[1] == AD_7LIKE_BAR)
		return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
	    /* push sub-pattern context */
	    if (db_stat = pat_rep_push(&pat_ctx))
               return db_stat;
	    p++;
	    continue;   /* No repeat ctx */
	}
	else if (*p == AD_9LIKE_RPAR && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    /* pop sub-pattern context */
	    if (db_stat = pat_rep_pop(&pat_ctx))
		return db_stat;
	    p++;
	}
	else if (*p == AD_7LIKE_BAR && regexp)
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    /* Alternation */
	    if (!pat_ctx.rep_depth)
	    {
		/* No need for case handling - keep simple */
		if (db_stat = pat_op_simple(&pat_ctx, PAT_FINAL))
		    return db_stat;
		pat_bld_finpat(&pat_ctx, (u_i1*)p);

		pat_bld_newpat(&pat_ctx, (u_i1*)p);
	    }
	    else
	    {
		if (p[1] == AD_9LIKE_RPAR || p[1] == AD_7LIKE_BAR)
		    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
		if (db_stat = pat_set_case(&pat_ctx))
		    return db_stat;
	    }
	    p++;
	    continue;   /* No repeat ctx */
	}
	else if ((pat_flags & AD_PAT_BIGNORE) && (*p == U_BLANK || !*p))
	{
	    /* Force out any part literal - runs will coalesce */
	    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
		return db_stat;
	    p++;
	}
	else /* Literal */
	{
	    if (!lit)
		lit = p;
	    p++;
	}
	if (handle_set)
	{
	    PAT_SET **setbuf = (PAT_SET**)&sea_ctx->setbuf;
	    PAT_SET pat_set_stk;
	    i4 count = 0;
	    bool bitset_valid = FALSE; /* There ought to be a way of allowing
				       ** efficient bitset access but at the moment
				       ** patda provids CE list not code points.
				       ** TODO: consider using both forms & defer
				       ** CE until needed (maybe never if no set
				       ** ranges used) */
	    bool neg = FALSE, part_neg = FALSE;
	    bool in_range = FALSE;
	    bool allow_rng = FALSE;
	    UCS2 ch;
	    u_char v, set[256/8];
	    u_i4 lo = 256, hi = 0, dash = 0;
	    const UCS2 *save_p = NULL, *save_e;
	    UCS2 temp_ce[(MAX_CE_LEVELS-1)*1048];
	    UCS2 *ce = temp_ce;

	    if (pat_flags & (AD_PAT2_UCS_BASIC<<16))
		bitset_valid = TRUE;
	    pure = FALSE;
	    if (bitset_valid)
		MEfill(256/8, 0, set);
	    if (!*setbuf)
	    {
		pat_set_stk.lim = PAT_SET_STK_DIM;
		pat_set_stk.part = 0;
		pat_set_stk.last = 0;
		*setbuf = &pat_set_stk;
	    }
	    (*setbuf)->n = 0;
	    p++;
	    if (*p == AD_6LIKE_CIRCUM &&
		(regexp ||
		 allow_pat && (ADU_pat_debug & PAT_DBG_NEG_ENABLE))
		)
	    {
		p++;
		neg = TRUE;
	    }
	    while (p < e)
	    {
		i4 bit;
		u_char *byt;
		/* Handle ESCAPE char */
		if (*p == *temp_esc)
		{
		    p++;
		    if (*p == AD_4LIKE_RBRAC && !regexp)
		    {
			goto rbrac_common;
		    }
		    if ((!allow_pat || (*p != AD_1LIKE_ONE &&
					*p != AD_2LIKE_ANY)) &&
			    (!regexp || (*p != AD_3LIKE_LBRAC &&
					*p != AD_4LIKE_RBRAC &&
					*p != AD_8LIKE_LPAR &&
					*p != AD_9LIKE_RPAR &&
					*p != AD_7LIKE_BAR &&
					*p != AD_6LIKE_CIRCUM &&
					*p != AD_5LIKE_RANGE &&
					*p != AD_13LIKE_PLUS &&
					*p != AD_12LIKE_STAR &&
					*p != AD_14LIKE_QUEST &&
					*p != AD_10LIKE_LCURL
			    )) &&
			    *p != *temp_esc)
		    {
			if ((*setbuf)->lim == PAT_SET_STK_DIM)
			    sea_ctx->setbuf = NULL;
			return adu_error(adf_scb, regexp ? E_AD101D_BAD_ESC_SEQ : allow_pat
			    ? E_AD1018_BAD_ESC_SEQ : E_AD101C_BAD_ESC_SEQ, 0);
		    }
		    goto ch_common;
		}
		else if (*p == AD_6LIKE_CIRCUM && regexp && !neg && !part_neg)
		{
		    p++;
		    part_neg = TRUE;
		    (*setbuf)->part = (*setbuf)->n;
		    continue;
		}
		else if (*p == AD_3LIKE_LBRAC && regexp && p[1] == AD_16LIKE_COLON)
		{
		    const UCS2 *c = chclasses_uni;
		    if (*temp_esc == AD_16LIKE_COLON)
		    {
			if ((*setbuf)->lim == PAT_SET_STK_DIM)
			    sea_ctx->setbuf = NULL;
			return adu_error(adf_scb, E_AD101E_ESC_SEQ_CONFLICT, 0);
		    }
		    p += 2;
		    for (;;)
		    {
			const UCS2 *pp = p;
			i4 res;
			while (*c)
			{
			    u_char t[4];
			    CMtolower(pp, t);
			    if (res = *c++ - (*pp++ < 128 ? *t : pp[-1]))
				break;
			}
			while (*c++)
			    /*SKIP*/;
			if (!res)
			{
			    if (*pp != AD_16LIKE_COLON || pp[1] != AD_4LIKE_RBRAC)
			    {
				/* Error with char class :] expected */
				if ((*setbuf)->lim == PAT_SET_STK_DIM)
				    sea_ctx->setbuf = NULL;
				return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
			    }
			    /* Temporarily inject char class eqv */
			    save_p = pp + 2; save_e = e;
			    p = e = c;
			    while (*e++)
				/*SKIP*/;
			    break;
			}
			while (*c++)
			    /*SKIP*/;
			if (!*c)
			{
			    /* Unrecognised char class name */
			    if ((*setbuf)->lim == PAT_SET_STK_DIM)
				sea_ctx->setbuf = NULL;
			    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
			}
		    }
		}
		else if (!*p && regexp && save_p)
		{
		    /* Undo temporary injection of char class */
		    p = save_p; e = save_e;
		    save_p = NULL;
		    continue;
		}
		else if (*p == AD_4LIKE_RBRAC && regexp)
		{
rbrac_common:	    if (in_range)
		    {
			/* Trailing '-' */
			if ((*setbuf)->lim == PAT_SET_STK_DIM)
			    sea_ctx->setbuf = NULL;
			return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
		    }
		    p++;
		    /* Clean exit */
		    if (bitset_valid && (neg || part_neg))
			db_stat = pat_op_bitset(&pat_ctx, neg, lo, hi, count, set);
		    else
			db_stat = (*pat_op_set_fn)(&pat_ctx, neg, *setbuf);
		    if (!db_stat)
			goto set_ok;
		    if ((*setbuf)->lim == PAT_SET_STK_DIM)
			sea_ctx->setbuf = NULL;
		    return db_stat;
		}
		else if (*p == AD_5LIKE_RANGE &&
			!in_range &&
			allow_rng)
		{
		    /* Handle range in the next section */
		    in_range = TRUE;
		    p++;
		}
		else if (*p == AD_1LIKE_ONE ||
			*p == AD_2LIKE_ANY ||
			*p == AD_3LIKE_LBRAC)
		{
		    if ((*setbuf)->lim == PAT_SET_STK_DIM)
			sea_ctx->setbuf = NULL;
		    return adu_error(adf_scb, E_AD1016_PMCHARS_IN_RANGE, 0);
		}
		else
		{
ch_common:	    /* If ANY set character is non-ASCII then we
		    ** will not support BITSET */
		    if (*p >= 128)
			bitset_valid = FALSE;
		    else if (bitset_valid)
		    {
			u_char tmp = (pat_flags & AD_PAT_WO_CASE)?cmtolower(p):*p;
			/* Bitset handling */
			if (!in_range)
			{
			    /* Setup for a bit range of 1 :-) */
			    v = tmp;
    				
			    /* Is single char or range start
			    ** could be a new low */
			    if (v < lo)
				lo = v;
			    if (v > hi)
				hi = v;
			}
			else if (v > tmp)
			{
			    /* Sanity test - start before end*/
			    if ((*setbuf)->lim == PAT_SET_STK_DIM)
				sea_ctx->setbuf = NULL;
			    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
			}
			for(;v <= tmp; v++)
			{
			    bit = 1<<(v&7);
			    byt = &set[v>>3];
			    if (part_neg)
			    {
				if ((*byt & bit))
				{
				    /* Clear the bits */
				    set[v>>3] &= ~(1<<(v&7));
				    count--;
				}
			    }
		    	    else if (!(*byt & bit))
			    {
				/* Set the bits */
				set[v>>3] |= 1<<(v&7);
				count++;
			    }
			    if (v == tmp)
				break;
			}
			if (v > hi)
			    hi = v;
		    }
		    /*
		    ** Once out of this block - may only allow next
		    ** char to be valid '-' if this char is not a range terminal.
		    */
		    allow_rng = !in_range;
		    /* Setup the none Bitset anyway incase needed.
		    ** Now, about a range. If we are passed a range,
		    ** we rearrange the input stream so that the '-'
		    ** precedes its two character (poss-MB) operands.
		    ** By convention, a range specifier at the start
		    ** of the buffer will be treated as a literal,
		    */
		    {
			u_i4 ce_len = 0;
			UCS2 *cep = ce;
			if ((db_stat = adu_pat_MakeCElen(adf_scb, p, p+1, &ce_len)))
			{
			    if ((*setbuf)->lim == PAT_SET_STK_DIM)
				sea_ctx->setbuf = NULL;
			    return db_stat;
			}
			adu_pat_MakeCEchar(adf_scb, &p, p+1, &ce, pat_flags);
			if (pat_set_add(setbuf, (u_char*)cep, ce_len, in_range, cmp_unicodeCE, NULL))
			    /* Drop out of loop to return error
			    ** E_AD1015_BAD_RANGE
			    */
			    break;
		    }
		    in_range = FALSE;
		}
	    }
	    /*missing ]*/
	    if ((*setbuf)->lim == PAT_SET_STK_DIM)
		sea_ctx->setbuf = NULL;
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
set_ok:	    if ((*setbuf)->lim == PAT_SET_STK_DIM)
		sea_ctx->setbuf = NULL;
	}
	if (regexp && p < e)
	{
	    u_i4 lo = 0, hi = 0;
	    bool wild = FALSE;
	    const UCS2 *old_p = p;
	    if (*p == AD_10LIKE_LCURL)
	    {
		p++;
		prs_num_uni(&p, e, &lo);

		if (p < e && *p == AD_15LIKE_COMMA)
		{
		    p++;
		    if (p < e && *p == AD_11LIKE_RCURL)
		    {
			wild = TRUE;
			hi = lo;
		    }
		    else
		    {
			prs_num_uni(&p, e, &hi);
			if (hi < lo)
			    /* Repeat bounds error */
			    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
		    }
		}
		else
		    hi = lo;
		if (p < e && *p == AD_11LIKE_RCURL)
		    p++;
		else
		    /* Expected '}' */
		    return adu_error(adf_scb, E_AD101F_REG_EXPR_SYN, 0);
		if (!wild && lo == 1 && hi == 1)
		    continue;
	    }
	    else if (*p == AD_12LIKE_STAR)
	    {
		p++;
		lo = hi = 0;
		wild = TRUE;
	    }
	    else if (*p == AD_13LIKE_PLUS)
	    {
		p++;
		lo = hi = 1;
		wild = TRUE;
	    }
	    else if (*p == AD_14LIKE_QUEST)
	    {
		p++;
		lo = 0;
		hi = 1;
	    }
	    else 
		continue;
	    /* Force out any part literal - runs will coalesce */
	    if (lit && (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, old_p)))
		return db_stat;

	    /* Here we have the repeat parsed out into lo, hi and wild.
	    ** The last_op will point to the instruction that the repeat
	    ** counts are to apply to.
	    ** NOTE:
	    ** One problem at this point relates to PAT_LIT. It will be
	    ** representing (usually) an unbroken sequence of literal
	    ** text but the range must only apply to the last character.
	    ** Unlike with the non-unicode form we split the PAT_LIT
	    ** but do not convert it into a PAT_SET form. Instead we let
	    ** the repeat code handle the insertion of extra instructions.
	    ** Thus:
	    ** 'ABCD{4,8}' starts as:
	    **	PAT_LIT 8 'ABCD'
	    ** but is treated as: 'ABC[D]{4,8}' and the instructions
	    ** are then:
	    **	PAT_LIT 6 'ABC'
	    **	PAT_FOR_N_M 4 4 4
	    **	PAT_LIT 2 'D'
	    **	PAT_NEXT -8
	    */
	    if (*pat_ctx.last_op == PAT_LIT)
	    {
		u_i1 *pc;
		u_i4 l;
		chk_space(&pat_ctx, 8);
		pat_ctx.prelit_op = NULL; /* Can't ignore trailing context */
		pc = pat_ctx.last_op + 1;
		l = GETNUM(pc);
		if (l > 1)
		{
		    /* Split the literal at the last CHAR! */
		    u_i1 *s1 = pc, *s2;
		    pc += l;
		    s2 = pc;
		    s2 -= (MAX_CE_LEVELS-1)*sizeof(UCS2)/sizeof(*s1);
		    if (s2 > s1)
		    {
			i4 l2 = pc - s2;
			MEmemmove(s2 + sizeof(u_i1)+sizeof(u_i1), s2, l2);
			pat_ctx.wrtptr += sizeof(u_i1)+sizeof(u_i1);
			s1 = pat_ctx.last_op + 1;
			pat_ctx.last_op = s2;
			*s2++ = PAT_LIT;
			PUTNUM(s2, l2);
			pat_update_num(&s1, (i8)-l2, &pat_ctx.wrtptr);
		    }
		}
	    }
	    if (db_stat = pat_apply_repeat(&pat_ctx, lo-1, hi-lo, wild))
		return db_stat;
	}
    }
    if (db_stat = (*pat_op_lit_fn)(&pat_ctx, &lit, p))
	return db_stat;

    /*
    ** If BEGINING or CONTAINING, inject final match any operator
    */
    if ((pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_BEGINNING ||
	(pat_flags & (AD_PAT_FORM_MASK)) == AD_PAT_FORM_CONTAINING)
    {
	if (db_stat = pat_op_wild(&pat_ctx))
	    return db_stat;
    }

    if (db_stat = pat_op_simple(&pat_ctx, PAT_FINAL))
	    return db_stat;

    pat_bld_finpat(&pat_ctx, (u_i1*)p);

#if PAT_DBG_TRACE>0
    /*
    ** If \I=file was specified - throw away and pull opcode stream from file
    */
    if (sea_ctx->infile)
    {
	if (db_stat = pat_load(&pat_ctx, sea_ctx->infile))
	    pat_ctx.force_fail = TRUE;
	MEfree(sea_ctx->infile);
	sea_ctx->infile = NULL;
    }
#endif

    if (db_stat = adu_patcomp_set_pats(sea_ctx))
	return db_stat;

#if PAT_DBG_TRACE>0
    /*
    ** If \O=file was specified - dump the pattern buffers now
    ** (even if they did just came from the file)
    */
    if (sea_ctx->outfile)
    {
	pat_dump(sea_ctx->patdata, sea_ctx->outfile);
	pat_ctx.force_fail = TRUE;
	MEfree(sea_ctx->outfile);
	sea_ctx->outfile = NULL;
    }
#endif

    if (!pat_ctx.force_fail)
	sea_ctx->force_fail = FALSE;
    if (sea_ctx->patdata->patdata.npats == 1)
    {
	if (pat_ctx.no_of_ops == 1 ||
	    pat_ctx.no_of_ops == 2 &&
		sea_ctx->patdata->patdata.first_op == PAT_LIT)
	{
	    if (pure)
		sea_ctx->patdata->patdata.flags2 |= AD_PAT2_PURE_LITERAL;
	    sea_ctx->patdata->patdata.flags2 |= AD_PAT2_LITERAL;
	}
	else if (sea_ctx->patdata->patdata.first_op == PAT_MATCH &&
		pat_ctx.no_of_ops == 2)
	{
	    sea_ctx->patdata->patdata.flags2 |= AD_PAT2_MATCH;
	}
	/* No point checking for .first_op == PAT_ENDLIT for Unicode
	** as the collation implied renders the optimisation invalid */
    }
    return db_stat;
}


/*{
** Name: adu_patcomp_kbld - Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.
**
**	This routine handles non-Unicode and UCS_BASIC UTF8 patterns and will pass
**	other patterns to adu_patcomp_kbld_uniCE or adu_patcomp_kbld_uni if UCS_BASIC
**	implied.
**
**	However, the type of operation to be used is also included amongst the
**	parameters.  The possible operators, and their results, are listed
**	below. Of these it is expected that only LIKE will be supported with
**	DB_PAT_TYPE.
**
**	    ADI_EQ_OP ('=')
**	    ADI_NE_OP ('!=')
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**	    ADI_LIKE_OP
**
**	In addition to returning the key pair, the type of key produced is
**	returned.  The choices are
**
**	    ADC_KNOMATCH    --> No values will match this key.
**				    No key is formed.
**	    ADC_KEXACTKEY   --> The key will match exactly one value.
**				    The key that is formed is placed as the low
**				    key.
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.  The caller
**				    should seek to the low-key, then scan until
**				    reaching a point greater than or equal to
**				    the high key.
**	    ADC_KHIGHKEY    --> High key found -- only high key formed.
**	    ADC_KLOWKEY	    --> Low key found -- only low key formed.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				    No key was formed.
**
**	Given these values with LIKE, the combinations possible are shown
**	below.
**
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				Returned when a generated KEY pair will
**				be suspect - typically due to collation.
**
**
** Inputs:
**      adf_scb                         scb
**      adc_kblk                        Pointer to key block data structure...
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**
** Outputs:
**      *adc_kblk                      Key block filled with following
**	    .adc_tykey                       Type key provided
**	    .adc_lokey                       if .adc_tykey is ADC_KRANGEKEY and
**					     key buffer passed.
**	    .adc_hikey			     if .adc_tykey is ADC_KRANGEKEY and
**					     key buffer passed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**
** History:
**	27-Oct-2008 (kiria01) SIR120473
**	    Drop out with KALLMATCH if an alternation character is seen as
**	    the alternatives will overly complicate things at present.
**	05-Dec-2008 (kiria01) SIR120473
**	    Address alignment issue on Solaris
**	07-Jan-2009 (kiria01) SIR120473
**	    Don't let PAT_FNDLIT get treated as PAT_LIT for keys.
**      29-Jan-2009 (kiria01) SIR120473
**          Correct the byte adjust from BITSET
**	09-Feb-2009 (kiria01) SIR120473
**	    Regularise the determining of .adc_tykey. Understanding that
**	    the only reasonable values for LIKE are RANGE and ALL and that
**	    these are inequality types unusable with hashing means that
**	    we don't need to scan the pattern to find the other info if
**	    key buffers are not passed. Although it is possible in the rare
**	    case that we can determine that an exact match can be done (no
**	    pattern ops) or that a match cannot occur (over long pattern)
**	    these can be detected at patcomp time too and better optimised
**	    there.
**      27-Mar-2009 (kiria01,coomi01) b121841
**          Detect null pattern data and return ADC_KALLMATCH
*/
DB_STATUS
adu_patcomp_kbld (
ADF_CB		*adf_scb,
ADC_KEY_BLK	*adc_kblk)
{
    AD_PATDATA *patdata;
    AD_PATDATA _patdata;
    DB_STATUS db_stat = E_DB_OK;
    DB_DATA_VALUE *lo_dv   = &adc_kblk->adc_lokey;
    DB_DATA_VALUE *hi_dv   = &adc_kblk->adc_hikey;
    u_i4 i;
    u_i4 pfx_len_lim;
    u_i4 save_lohilen;
    u_i4 base = PATDATA_1STFREE;
    u_i4 nomatch = 0;
    u_i1 *lo = (u_i1*)lo_dv->db_data;
    u_i1 *hi = (u_i1*)hi_dv->db_data;
    u_i1 lowestchar = 0x00;
    u_i1 highestchar = 0xFF;
    bool patseen = FALSE;

    if (adc_kblk->adc_opkey != ADI_LIKE_OP)
	return adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "pat kbld op");

    if (adc_kblk->adc_kdv.db_datatype != DB_PAT_TYPE)
	return adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "pat kbld dt");

    patdata = (AD_PATDATA *)adc_kblk->adc_kdv.db_data;
    {
	u_i2 flags;

	if (NULL == patdata)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}

	/* Hand off Unicode pattern */
	I2ASSIGN_MACRO(patdata->patdata.flags2, flags);
	if (flags & AD_PAT2_UCS_BASIC)
	    return adu_patcomp_kbld_uni(adf_scb, adc_kblk);
	if (flags & AD_PAT2_UNICODE_CE)
	    return adu_patcomp_kbld_uniCE(adf_scb, adc_kblk);

	/* We cannot optimise to a range if collation is in effect */
	if (flags & AD_PAT2_COLLATE)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}
	/* We cannot optimise to a range if blanks are to be ignored. */
	I2ASSIGN_MACRO(patdata->patdata.flags, flags);
	if (flags & AD_PAT_BIGNORE)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}
    }

    /* We will probably return ADC_KRANGEKEY so set that now */
    adc_kblk->adc_tykey = ADC_KRANGEKEY;

    /* If no key buffers passed this is a query as to whether
    ** an equality lookup (HASH) can be supported. As LIKE cannot
    ** return ADC_KEXACTKEY or ADC_KNOMATCH we go no further than
    ** say range match for now. If asked for the hi/lo keys we can
    ** later switch to ALL if need be. */
    if (!lo || !hi)
	return E_DB_OK;

    if (ME_ALIGN_MACRO(patdata, sizeof(i2)) != (PTR)patdata)
    {
	if ((i4)sizeof(_patdata) >= adc_kblk->adc_kdv.db_length)
	    patdata = &_patdata;
	else
	{
	    patdata = (AD_PATDATA*)MEreqmem(0, adc_kblk->adc_kdv.db_length,
			FALSE, &db_stat);
            if (!patdata || db_stat)
		return db_stat;
	}
	MEcopy(adc_kblk->adc_kdv.db_data,
		adc_kblk->adc_kdv.db_length, patdata);
    }


    /* Now get string length of low and high keys (assumed to be the same) */
    save_lohilen = lo_dv->db_length;
    if (abs(lo_dv->db_datatype) == DB_VCH_TYPE ||
	abs(lo_dv->db_datatype) == DB_TXT_TYPE ||
	abs(lo_dv->db_datatype) == DB_LTXT_TYPE)
    {
	lo += DB_CNTSIZE;
	hi += DB_CNTSIZE;
	save_lohilen -= DB_CNTSIZE;
    }
    pfx_len_lim = save_lohilen;
    switch (abs(lo_dv->db_datatype))
    {
    case DB_CHR_TYPE:
	lowestchar  = MIN_CHAR;
	highestchar = IIMAX_CHAR_VALUE;
	break;
    case DB_TXT_TYPE:
	lowestchar  = 0x01;
	highestchar = AD_MAX_TEXT;
	break;
    }

    /* It is very unlikely that we will have any great length
    ** of prefix-pattern as most pattern operators will break it */
    for (i = 0; i < patdata->patdata.npats &&
		base < patdata->patdata.length; i++)
    {
	u_i4 patlen = patdata->vec[base];
	u_i1 *pc = (u_i1*)&patdata->vec[base+1];
	u_i1 *patbufend = (u_i1*)&patdata->vec[base+patlen];
	u_i4 pfx_len = 0;
	i8 N, M, v;
	i4 n;
	bool wild = FALSE;

	while (pc < patbufend && !wild)
	{
	    u_i1 op = *pc++;
	    u_i4 cl, cl2;

	    if (opcodes[op].v1 != V1__)
		N = opcodes[op].v1;
	    else if (opcodes[op].p1 == P1_N)
		N = GETNUM(pc);
	    if (opcodes[op].p2 == P2_M)
		M = GETNUM(pc);
	    else if (opcodes[op].v2 == V2_W)
		wild = 1;
	    switch (opcodes[op].cl)
	    {
	    case opClass__:
		switch (op)
		{
		default:
		    /* Most operators we use to terminate the key */
		    patseen = TRUE;
		    /*FALLTHROUGH*/
		case PAT_FINAL:
		    pc = patbufend;
		    break;
		case PAT_NOP:
		case PAT_TRACE:
		case PAT_NOTRACE:
		case PAT_BOS:
		case PAT_EOS:
		case PAT_BOM:
		case PAT_EOM:
		    continue;

/*ANY*/		case PAT_ANY_0_W:
		case PAT_ANY_1:
		case PAT_ANY_1_W:
		case PAT_ANY_N:
		case PAT_ANY_N_W:
		case PAT_ANY_N_M:
		    goto anyn;
		}

		break;
	    case opClass_C:/* CASE */
	    case opClass_J:/* Jump/Label */
		v = GETNUM(pc);
		patseen = TRUE;
		/* No point looking further for this slot */
		pc = patbufend;
		break;

	    case opClass_L:/* Literal text */
		/* What this is all about - we have a prefix string:
		** This needs adding as a prefix to both low and high */
		v = GETNUM(pc);
		if (op != PAT_LIT)
		{
		    if (op == PAT_FNDLIT || op == PAT_ENDLIT)
		    {
			/* FNDLIT/ENDLIT are essentially a % so treat
			** as such for keybld */
			wild = TRUE;
			goto anyn;
		    }
		    pc += v;
		    break;
		}
		if (patdata->patdata.flags & AD_PAT_WO_CASE)
		{
		    cl2 = 0;
		    /* Note: LIT pattern is already lowercase */
		    while (v >= (cl = CMbytecnt(pc)) && pfx_len_lim - pfx_len >= cl)
		    {
			u_i1 pc2[4];
			if (i == 0)
			{
			    MEcopy(pc, cl, lo + pfx_len);
			    MEcopy(pc, cl, hi + pfx_len);
			}
			else
			{
			    if (cl != (cl2 = CMbytecnt(lo + pfx_len)))
				break;
			    if (MEcmp(pc, lo + pfx_len, cl) < 0)
				MEcopy(pc, cl, lo + pfx_len);
			    if (cl != (cl2 = CMbytecnt(hi + pfx_len)))
				break;
			    if (MEcmp(pc, hi + pfx_len, cl) > 0)
				MEcopy(pc, cl, hi + pfx_len);
			}
			CMtoupper(pc, pc2);
			cl2 = CMbytecnt(pc2);
			if (cl != cl2)
			    break;
			if (MEcmp(pc2, lo + pfx_len, cl) < 0)
			    MEcopy(pc2, cl, lo + pfx_len);
			if (MEcmp(pc2, hi + pfx_len, cl) > 0)
			    MEcopy(pc2, cl, hi + pfx_len);
			pc += cl;
			v--;
			pfx_len += cl;
		    }
		}
		else
		{
		    cl2 = 0;
		    while (v >= (cl = CMbytecnt(pc)) && pfx_len_lim - pfx_len >= cl)
		    {
			if (i == 0)
			{
			    MEcopy(pc, cl, lo + pfx_len);
			    MEcopy(pc, cl, hi + pfx_len);
			}
			else
			{
			    if (cl != (cl2 = CMbytecnt(lo + pfx_len)))
				break;
			    if (MEcmp(pc, lo + pfx_len, cl) < 0)
				MEcopy(pc, cl, lo + pfx_len);
			    if (cl != (cl2 = CMbytecnt(hi + pfx_len)))
				break;
			    if (MEcmp(pc, hi + pfx_len, cl) > 0)
				MEcopy(pc, cl, hi + pfx_len);
			}
			pc += cl;
			v--;
			pfx_len += cl;
		    }
		}
		if (i > 0 && cl != cl2)
		{
		    pc = patbufend;
		    break;
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (v > save_lohilen - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (v)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

/*SET*/	    case opClass_S:/* Subset */
		/* We have a low and a high character:
		** Low at *pc (or +1 if range introducer) and high at the
		** end of the set literal */
		patseen = TRUE;
		v = GETNUM(pc);
		{
		    u_i1 *lo_ch = pc;
		    u_i1 *hi_ch = pc;
		    u_i1 _lo_ch2[4];
		    u_i1 *lo_ch2 = _lo_ch2;
		    u_i1 _hi_ch2[4];
		    u_i1 *hi_ch2 = _hi_ch2;
		    /* The lower range char MIGHT be a range
		    ** operator - if so get the next char */
		    if (*lo_ch == AD_5LIKE_RANGE)
			lo_ch++;
		    /* Find the start of the upper range char */
		    while (pc - lo_ch < v)
		    {
			hi_ch = pc;
			CMnext(pc);
		    }
		    if (CMbytecnt(lo_ch) > 1 || CMbytecnt(hi_ch) > 1)
		    {
			/* No point looking further as we have hit
			** a multibyte character */
			pc = patbufend;
			break;
		    }
		    if (patdata->patdata.flags & AD_PAT_WO_CASE)
		    {
			CMtoupper(lo_ch, lo_ch2);
			CMtoupper(hi_ch, hi_ch2);
			if (*lo_ch > *hi_ch2)
			    lo_ch = hi_ch2;
			if (*hi_ch > *lo_ch2)
			    hi_ch = lo_ch2;
		    }
		    while (N > 0 && pfx_len < pfx_len_lim)
		    {
			if (i == 0)
			{
			    lo[pfx_len] = *lo_ch;
			    hi[pfx_len] = *hi_ch;
			}
			else
			{
			    if (lo[pfx_len] > *lo_ch)
				lo[pfx_len] = *lo_ch;
			    if (hi[pfx_len] > *hi_ch)
				hi[pfx_len] = *hi_ch;
			}
			N--;
			pfx_len++;
		    }
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > save_lohilen - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

/*BSET*/    case opClass_B:/* Bitset */
		/* We have a low and a high character: (both will be single byte)
		** Low at *pc (or +1 if range introducer) and high at the
		** end of the set literal */
		patseen = TRUE;
		n = N;
		v = GETNUM(pc);
		{
		    u_i1 _lo_ch[2];
		    u_i1 *lo_ch = _lo_ch;
		    u_i1 _hi_ch[2];
		    u_i1 *hi_ch = _hi_ch;
		    u_i1 _lo_ch2[2];
		    u_i1 *lo_ch2 = _lo_ch2;
		    u_i1 _hi_ch2[2];
		    u_i1 *hi_ch2 = _hi_ch2;
		    lo_ch[0] = v;
		    v = GETNUM(pc);
		    hi_ch[0] = v * 8 + lo_ch[0];
		    pc += v;
		    if (patdata->patdata.flags & AD_PAT_WO_CASE)
		    {
			CMtoupper(lo_ch, lo_ch2);
			CMtoupper(hi_ch, hi_ch2);
			if (*lo_ch > *hi_ch2)
			    lo_ch = hi_ch2;
			if (*hi_ch > *lo_ch2)
			    hi_ch = lo_ch2;
		    }
		    while (N > 0 && pfx_len < pfx_len_lim)
		    {
			if (i == 0)
			{
			    lo[pfx_len] = *lo_ch;
			    hi[pfx_len] = *hi_ch;
			}
			else
			{
			    if (lo[pfx_len] > *lo_ch)
				lo[pfx_len] = *lo_ch;
			    if (hi[pfx_len] > *hi_ch)
				hi[pfx_len] = *hi_ch;
			}
			N--;
			pfx_len++;
		    }
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > save_lohilen - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

/*NBSET*/    case opClass_NB:/* Bitset */
		v = GETNUM(pc);	    /*skip offset*/
		/*FALLTHROUGH*/

/*NSET*/    case opClass_NS:/* Subset */
		v = GETNUM(pc);
		pc += v;
anyn:		/* We know min length but no more */
		patseen = TRUE;
		if (!pfx_len && wild)
		{
		    /* Don't return range key if really a scan needed */
	    	    adc_kblk->adc_tykey = ADC_KALLMATCH;
		    goto common_return;
		}
		if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
		{
		    /* Terminate pattern now */
		    pc = patbufend;
		    break;
		}
		while (N > 0 && pfx_len_lim - pfx_len > 0)
		{
		    lo[pfx_len] = lowestchar;
		    hi[pfx_len] = highestchar;
		    N--;
		    pfx_len++;
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > save_lohilen - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N || wild)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

	    }
	}
	/* We have to use the shortest of keys (least specific) */
	if (pfx_len < pfx_len_lim)
	    pfx_len_lim = pfx_len;
	base += patlen;
    }
    if (nomatch == patdata->patdata.npats)
    {
	/* At present there is no useful way that we can
	** make use of this fact so we leave things as a
	** range lookup.
	**
	**  adc_kblk->adc_tykey = ADC_KNOMATCH;
	**  goto common_return;
	*/
    }
    if (!patseen && patdata->patdata.npats == 1)
    {
	/* As this is effrectvly an EQ, we could do the following
	** BUT it breaks REPEATABLE selects in that they loop endlessly!
	**
	**  adc_kblk->adc_tykey = ADC_KEXACTKEY;
	**  save_lohilen = pfx_len_lim;
	*/
    }

    MEfill(save_lohilen - pfx_len_lim, 0, lo+pfx_len_lim);
    if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
    {
	/* Can't just pad with 0xFF for high - they must be legal UTF8
	** so fill with max follower and then fixup leaders */
	MEfill(save_lohilen - pfx_len_lim, 0xBF, hi+pfx_len_lim);
	while (save_lohilen - pfx_len_lim >= 4)
	{
	    hi[pfx_len_lim] = 0xF7;
	    pfx_len_lim += 4;
	}
	switch (save_lohilen - pfx_len_lim)
	{
	case 3:
	    hi[pfx_len_lim] = 0xEF;
	    break;
	case 2:
	    hi[pfx_len_lim] = 0xDF;
	    break;
	case 1:
	    hi[pfx_len_lim] = 0x7F;
	    break;
	}
	/* On final gotcha. If we leave the range keys as they are,
	** adc_1helem_rti will see them and treat as though UTF-8
	** ignoring the collation! This is because by default
	** the collID is not copied to the hi & lo keys. Without
	** this it will break hashing!
	** The following probably should be propagated through OPB. */
		lo_dv->db_collID = hi_dv->db_collID = DB_UCS_BASIC_COLL;
    }
    else
	MEfill(save_lohilen - pfx_len_lim, highestchar, hi+pfx_len_lim);

    if (lo_dv->db_datatype == DB_VCH_TYPE ||
	lo_dv->db_datatype == DB_TXT_TYPE ||
	lo_dv->db_datatype == DB_LTXT_TYPE)
    {
	((DB_TEXT_STRING*)lo_dv->db_data)->db_t_count = save_lohilen;
	((DB_TEXT_STRING*)hi_dv->db_data)->db_t_count = save_lohilen;
    }
    if (MEcmp(hi, lo, save_lohilen) < 0)
	adc_kblk->adc_tykey = ADC_KALLMATCH;

common_return:
    if (patdata != &_patdata && (PTR)patdata != adc_kblk->adc_kdv.db_data)
	MEfree((PTR)patdata);
    return db_stat;
}


/*{
** Name: adu_patcomp_kbld_uniCE - Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.
**
**	This routine handles Unicode/UTF8 patterns and will pass non-Unicode
**	patterns to adu_patcomp_kbld or UCS_BASIC collation to adu_patcomp_kbld_uni
**
**	However, the type of operation to be used is also included amongst the
**	parameters.  The possible operators, and their results, are listed
**	below. Of these it is expected that only LIKE will be supported with
**	DB_PAT_TYPE.
**
**	    ADI_EQ_OP ('=')
**	    ADI_NE_OP ('!=')
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**	    ADI_LIKE_OP
**
**	In addition to returning the key pair, the type of key produced is
**	returned.  The choices are
**
**	    ADC_KNOMATCH    --> No values will match this key.
**				    No key is formed.
**	    ADC_KEXACTKEY   --> The key will match exactly one value.
**				    The key that is formed is placed as the low
**				    key.
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.  The caller
**				    should seek to the low-key, then scan until
**				    reaching a point greater than or equal to
**				    the high key.
**	    ADC_KHIGHKEY    --> High key found -- only high key formed.
**	    ADC_KLOWKEY	    --> Low key found -- only low key formed.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				    No key was formed.
**
**	NOTE: As Unicode keys are still using the raw characters and not CE
**	data we cannot really aid Unicode range selection as the patdata data
**	contains only CE data and there is no efficient way of reversing the
**	mapping. Until CE data is used for keys we will err on the side of
**	caution and return ADC_KALLMATCH.
**	NOTE about NOTE: If the Pattern was provided with a PAT_COMMENT then
**	we can improve the ADC_KALLMATCH to a ADC_KRANGEKEY by aiding the
**	reversing of the CE mapping by limited lookup.
**
**	Given these values with LIKE, the combinations possible are shown
**	below.
**
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				Returned when a generated KEY pair will
**				be suspect - typically due to collation.
**
**
** Inputs:
**      adf_scb                         scb
**      adc_kblk                        Pointer to key block data structure...
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**
** Outputs:
**      *adc_kblk                      Key block filled with following
**	    .adc_tykey                       Type key provided
**	    .adc_lokey                       if .adc_tykey is ADC_KRANGEKEY and
**					     key buffer passed.
**	    .adc_hikey			     if .adc_tykey is ADC_KRANGEKEY and
**					     key buffer passed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**
** History:
**	27-Oct-2008 (kiria01) SIR120473
**	    Drop out with KALLMATCH if an alternation character is seen as
**	    the alternatives will overly complicate things at present.
**	02-Dec-2008 (kiria01) SIR120473
**	    Complete last fix for UTF8 which gets through to here but
**	    needs reconverting.
**	05-Dec-2008 (kiria01) SIR120473
**	    Don't let UTF8 NULLS be seen in the range keys.
**	12-Dec-2008 (kiria01) SIR120473
**	    Address alignment issue on Solaris
**	07-Jan-2009 (kiria01) SIR120473
**	    Don't let PAT_FNDLIT get treated as PAT_LIT for keys.
**      27-Mar-2009 (kiria01,coomi01) b121841
**          Detect null pattern data and return ADC_KALLMATCH
*/
DB_STATUS
adu_patcomp_kbld_uniCE (
ADF_CB		*adf_scb,
ADC_KEY_BLK	*adc_kblk)
{
    AD_PATDATA *patdata;
    AD_PATDATA _patdata;
    DB_STATUS db_stat = E_DB_OK;
    DB_DATA_VALUE *lo_dv   = &adc_kblk->adc_lokey;
    DB_DATA_VALUE *hi_dv   = &adc_kblk->adc_hikey;
    u_i4 i;
    u_i4 pfx_len_lim;
    u_i4 lohicount;
    u_i4 base = PATDATA_1STFREE;
    u_i4 nomatch = 0;
    UCS2 *lo = (UCS2*)lo_dv->db_data;
    UCS2 *hi = (UCS2*)hi_dv->db_data;
    bool patseen = FALSE;
    i4 last_diff;
    UCS2 _l[2048], *l1, *l2, *l3, *h1, *h2, *h3;
    PTR memalloc = NULL;
    const UCS2 *comment = NULL;
    u_i2 commentlen = 0;
    bool UTF8mode = FALSE;

    if (adc_kblk->adc_opkey != ADI_LIKE_OP)
	return adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "patu kbld op");

    if (adc_kblk->adc_kdv.db_datatype != DB_PAT_TYPE)
	return adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "patu kbld dt");

    patdata = (AD_PATDATA *)adc_kblk->adc_kdv.db_data;
    {
	u_i2 flags;

	if (NULL == patdata)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}

	/* Hand off non-Unicode pattern */
	I2ASSIGN_MACRO(patdata->patdata.flags2, flags);
	if (flags & AD_PAT2_UCS_BASIC)
	    return adu_patcomp_kbld_uni(adf_scb, adc_kblk);

	if (~flags & AD_PAT2_UNICODE_CE)
	    return adu_patcomp_kbld(adf_scb, adc_kblk);

	/* We cannot optimise to a range if:
	** - blanks are to be ignored
	** - case is to be ignored*/
	I2ASSIGN_MACRO(patdata->patdata.flags, flags);
	if (flags & (AD_PAT_BIGNORE|AD_PAT_WO_CASE))
	{
	    /* Text modification is implied that renders this
	    ** impractical, return ALLMATCH */
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}
    }

    /* We will probably return ADC_KRANGEKEY so set that now */
    adc_kblk->adc_tykey = ADC_KRANGEKEY;

    /* If no key buffers passed this is a query as to whether
    ** an equality lookup (HASH) can be supported. As LIKE cannot
    ** return ADC_KEXACTKEY or ADC_KNOMATCH we go no further than
    ** say range match for now. If asked for the hi/lo keys we can
    ** later switch to ALL if need be. */
    if (!lo || !hi)
	return E_DB_OK;

    if (ME_ALIGN_MACRO(patdata, sizeof(i2)) != (PTR)patdata)
    {
	if ((i4)sizeof(_patdata) >= adc_kblk->adc_kdv.db_length)
	    patdata = &_patdata;
	else
	{
	    patdata = (AD_PATDATA*)MEreqmem(0, adc_kblk->adc_kdv.db_length,
			FALSE, &db_stat);
            if (!patdata || db_stat)
		return db_stat;
	}
	MEcopy(adc_kblk->adc_kdv.db_data,
		adc_kblk->adc_kdv.db_length, patdata);
    }
    
#ifndef _ADU_GEN_HELEM
    /* Now get string length of low and high keys (assumed to be the same) */
    lohicount = lo_dv->db_length / sizeof(UCS2);
    /* Write out the length of the results if NVARCHAR */
    if (lo_dv->db_datatype == DB_NVCHR_TYPE)
	*lo++ = *hi++ = --lohicount;
    else if (lo_dv->db_datatype != DB_NCHR_TYPE &&
	    (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED))
    {
	/* Awkward bit - for UTF8 we have Unicode CE PAT data but not
	** a unicode output buffer. We will need to build the UCS2 data
	** and then convert to UTF8. We will arrange that buffers are
	** suitably arranged that we don't need too much memory */
	UTF8mode = TRUE;
	if (lo_dv->db_datatype == DB_VCH_TYPE ||
	    lo_dv->db_datatype == DB_TXT_TYPE ||
	    lo_dv->db_datatype == DB_LTXT_TYPE)
	    lohicount--;
	lohicount *= sizeof(UCS2);
    }
    /* The Unicode helem should be a CE list consisting of 4 lists of
    ** CE level data, sepearted by a (UCS2)0. But it isn't - we use the raw
    ** code points but need to reverse engineer these from the PAT data. */
    pfx_len_lim = lohicount;
    if (pfx_len_lim > sizeof(_l)/sizeof(UCS2)/9)
    {
	memalloc = MEreqmem(0, pfx_len_lim * 9 * sizeof(UCS2), FALSE, &db_stat);
	if (!memalloc || db_stat)
	{
	    pfx_len_lim = sizeof(_l)/sizeof(UCS2)/9;
	    l1 = _l;
	}
	else
	    l1 = (UCS2*)memalloc;
    }
    else
	l1 = _l;
    h1 = l1 + pfx_len_lim;
    l2 = h1 + pfx_len_lim;
    h2 = l2 + pfx_len_lim;
    l3 = h2 + pfx_len_lim;
    h3 = l3 + pfx_len_lim;
#else
    /* Now get string length of low and high keys (assumed to be the same) */
    lohicount = lo_dv->db_length / sizeof(UCS2);

    /* The Unicode helem is a CE list which consists of 4 lists of
    ** CE level data, sepearted by a (UCS2)0. We drop the level 4
    ** and deal only with the more significant levels */
    pfx_len_lim = lohicount;
    if (pfx_len_lim > sizeof(_l)/sizeof(UCS2)/4)
    {
	memalloc = (UCS2*)MEreqmem(0, pfx_len_lim * 4 * sizeof(UCS2), FALSE, &db_stat);
	if (!memalloc || db_stat)
	{
	    pfx_len_lim = sizeof(_l)/sizeof(UCS2)/4;
	    l2 = _l;
	}
	else
	    l2 = (UCS2*)memalloc;
    }
    else
	l2 = _l;
    l1 = lo;
    h1 = hi;
    l3 = l2 + pfx_len_lim;
    h2 = l3 + pfx_len_lim;
    h3 = h2 + pfx_len_lim;

    lo_dv->db_datatype = DB_HELEM_TYPE;
    hi_dv->db_datatype = DB_HELEM_TYPE;
#endif

    /* It is very unlikely that we will have any great length
    ** of prefix-pattern as most pattern operators will break it */
    for (i = 0; i < patdata->patdata.npats &&
		base < patdata->patdata.length; i++)
    {
	u_i4 patlen = patdata->vec[base];
	u_i1 *pc = (u_i1*)&patdata->vec[base+1];
	u_i1 *patbufend = (u_i1*)&patdata->vec[base+patlen];
	u_i4 pfx_len = 0;
	i8 N, M, v;
	i4 n;
	bool wild = FALSE;

	while (pc < patbufend && !wild)
	{
	    u_i1 op = *pc++;

	    if (opcodes[op].v1 != V1__)
		N = opcodes[op].v1;
	    else if (opcodes[op].p1 == P1_N)
		N = GETNUM(pc);
	    if (opcodes[op].p2 == P2_M)
		M = GETNUM(pc);
	    else if (opcodes[op].v2 == V2_W)
		wild = 1;
	    switch (opcodes[op].cl)
	    {
	    case opClass__:
		switch (op)
		{
		default:
		    /* Most operators we use to terminate the key */
		    patseen = TRUE;
		    /*FALLTHROUGH*/
		case PAT_FINAL:
		    goto common_exit;
		case PAT_NOP:
		case PAT_TRACE:
		case PAT_NOTRACE:
		case PAT_BOS:
		case PAT_EOS:
		case PAT_BOM:
		case PAT_EOM:
		    continue;

/*ANY*/		case PAT_ANY_0_W:
		case PAT_ANY_1:
		case PAT_ANY_1_W:
		case PAT_ANY_N:
		case PAT_ANY_N_W:
		case PAT_ANY_N_M:
		    goto anyn;
		}

		break;
	    case opClass_C:/* CASE */
	    case opClass_J:/* Jump/Label */
		v = GETNUM(pc);
		patseen = TRUE;
		/* No point looking further for this slot */
common_exit:
#ifndef _ADU_GEN_HELEM
		if (!comment)
		{
		    do
		    {
			if (*pc == PAT_COMMENT)
			{
			    pc++;
			    v = GETNUM(pc);
			    commentlen = v/sizeof(UCS2);
			    comment = (UCS2*)pc;
			    break;
			}
		    } while (pc < patbufend &&
			!adu_pat_decode(&pc, 0, 0, 0, 0, 0));
		}
#endif
		pc = patbufend;
		break;

	    case opClass_L:/* Literal text */
		/* What this is all about - we have a prefix string:
		** This needs adding as a prefix to both low and high */
		v = GETNUM(pc);
		if (op != PAT_LIT)
		{
		    if (op == PAT_FNDLIT || op == PAT_ENDLIT)
		    {
			/* FNDLIT/ENDLIT are essentially a % so treat
			** as such for keybld */
			wild = TRUE;
			goto anyn;
		    }
		    if (op == PAT_COMMENT && !comment)
		    {
			commentlen = v/sizeof(UCS2);
			comment = (UCS2*)pc;
		    }
		    pc += v;
		    break;
		}
		{
		    UCS2 *ptr = (UCS2*)pc; /* PAT literals UCS2 aligned */
		    pc += v;
		    while (ptr < (UCS2*)pc && pfx_len < pfx_len_lim)
		    {
			if (i == 0)
			{
			    if (pfx_len < pfx_len_lim)
			    {
				l1[pfx_len] = h1[pfx_len] = *ptr++;
				l2[pfx_len] = h2[pfx_len] = *ptr++;
				l3[pfx_len] = h3[pfx_len] = *ptr++;
				pfx_len++;
			    }
			}
			else
			{
			    if (pfx_len < pfx_len_lim)
			    {
				if (l1[pfx_len] == ptr[0]
					? l2[pfx_len] == ptr[1]
					  ? l3[pfx_len] > ptr[2]
					  : l2[pfx_len] > ptr[1]
					: l1[pfx_len] > ptr[0])
				{
				    l1[pfx_len] = *ptr++;
				    l2[pfx_len] = *ptr++;
				    l3[pfx_len] = *ptr++;
				}
				else if (h1[pfx_len] == ptr[0]
					? h2[pfx_len] == ptr[1]
					  ? h3[pfx_len] < ptr[2]
					  : h2[pfx_len] < ptr[1]
					: h1[pfx_len] < ptr[0])
				{
				    h1[pfx_len] = *ptr++;
				    h2[pfx_len] = *ptr++;
				    h3[pfx_len] = *ptr++;
				}
				else
				    ptr += 3;
				pfx_len++;
			    }
			}
		    }
		    v = pc - (u_i1*)ptr;
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (v > (lohicount - pfx_len_lim)*(MAX_CE_LEVELS-1) &&
			    *pc != PAT_MATCH)
		    nomatch++;
		if (v)
		    /* No point looking further for this slot */
		    goto common_exit;
		break;

/*SET*/	    case opClass_S:/* Subset */
		/* We have a low and a high character:
		** Low at *pc (or +1 if range introducer) and high at the
		** end of the set literal */
		patseen = TRUE;
		v = GETNUM(pc);
		{
		    UCS2 *lo_ch = (UCS2*)pc; /* PAT literals UCS2 aligned */
		    UCS2 *hi_ch;
		    /* The lower range char MIGHT be a range
		    ** operator - if so get the next char */
		    if (*lo_ch == U_RNG)
			lo_ch += MAX_CE_LEVELS-1;
		    /* Find the start of the upper range char - easy -
		    ** it's the high end is the last on in the set */
		    pc += v;
		    hi_ch = (UCS2*)pc;
		    hi_ch -= (MAX_CE_LEVELS-1);

		    while (N > 0 && pfx_len < pfx_len_lim)
		    {
			if (pfx_len < pfx_len_lim)
			{
			    if (i == 0)
			    {
				l1[pfx_len] = lo_ch[0];
				l2[pfx_len] = lo_ch[1];
				l3[pfx_len] = lo_ch[2];
				h1[pfx_len] = hi_ch[0];
				h2[pfx_len] = hi_ch[1];
				h3[pfx_len] = hi_ch[2];
			    }
			    else
			    {
				if (l1[pfx_len] == lo_ch[0]
					? l2[pfx_len] == lo_ch[1]
					  ? l3[pfx_len] > lo_ch[2]
					  : l2[pfx_len] > lo_ch[1]
					: l1[pfx_len] > lo_ch[0])
				{
				    l1[pfx_len] = lo_ch[0];
				    l2[pfx_len] = lo_ch[1];
				    l3[pfx_len] = lo_ch[2];
				}
				if (h1[pfx_len] == hi_ch[0]
					? h2[pfx_len] == hi_ch[1]
					  ? h3[pfx_len] < hi_ch[2]
					  : h2[pfx_len] < hi_ch[1]
					: h1[pfx_len] < hi_ch[0])
				{
				    h1[pfx_len] = hi_ch[0];
				    h2[pfx_len] = hi_ch[1];
				    h3[pfx_len] = hi_ch[2];
				}
			    }
			    pfx_len++;
			}
			N--;
		    }
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N*(MAX_CE_LEVELS-1) > lohicount - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N)
		    /* No point looking further for this slot */
		    goto common_exit;
		break;

/*BSET*/    case opClass_B:/* Bitset - for unicodeCE??? */
/*NBSET*/   case opClass_NB:/* Bitset */
		v = GETNUM(pc);	    /*skip offset*/
		/*FALLTHROUGH*/

/*NSET*/    case opClass_NS:/* Subset */
		v = GETNUM(pc);
		pc += v;
anyn:		/* We know min length but no more */
		patseen = TRUE;
		if (!pfx_len && wild)
		{
		    /* Don't return range key if really a scan needed */
	    	    adc_kblk->adc_tykey = ADC_KALLMATCH;
		    goto common_return;
		}
		N *= (MAX_CE_LEVELS-1);
		while (N > 0 && pfx_len < pfx_len_lim)
		{
		    l1[pfx_len] = AD_MIN_UNICODE;
		    l2[pfx_len] = AD_MIN_UNICODE;
		    l3[pfx_len] = AD_MIN_UNICODE;
		    h1[pfx_len] = AD_MAX_UNICODE;
		    h2[pfx_len] = AD_MAX_UNICODE;
		    h3[pfx_len] = AD_MAX_UNICODE;
		    N--;
		    pfx_len++;
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > lohicount - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N || wild)
		    /* No point looking further for this slot */
		    goto common_exit;
		break;

	    }
	}
	/* We have to use the shortest of keys (least specific) */
	if (pfx_len < pfx_len_lim)
	    pfx_len_lim = pfx_len;
	base += patlen;
    }
    if (nomatch == patdata->patdata.npats)
    {
	/* At present there is no useful way that we can
	** make use of this fact so we leave things as a
	** range lookup.
	**
	**  adc_kblk->adc_tykey = ADC_KNOMATCH;
	**  goto common_return;
	*/
    }
    if (!patseen && patdata->patdata.npats == 1)
    {
	/* As this is effectivly an EQ, we could do the following
	** BUT it breaks REPEATABLE selects in that they loop endlessly!
	**
	**  adc_kblk->adc_tykey = ADC_KEXACTKEY;
	**  lohicount = pfx_len_lim;
	*/
    }

    {
	register u_i4 j;
	register UCS2 *lo1 = lo, *hi1 = hi;

#ifndef _ADU_GEN_HELEM
	if (!comment)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    j = 0;
	}
	else
	{
	    UCS2 *CE = h3 + pfx_len_lim, *CEend = CE;
	    const UCS2 *cp = comment;
	    u_i4 CElen;

	    /* Although we are able to generate the helem directly for lookup
	    ** the present OPF/QEF have no way to be told that the data is in
	    ** the correct format - it can only handle the code points. Snag
	    ** is that we will now have to reverse the helem data back to the
	    ** code points - this is typically not reversable. We could go
	    ** through every feasible codepoint and map it through to compare
	    ** with the helem data we have - this would be costly. HOWEVER, we
	    ** know the complete subset of characters that could be candidates
	    ** so the reverse will not be so bad.
	    ** 
	    */
	    for (i = 0; i < commentlen; i++)
	    {
		if (db_stat = adu_pat_MakeCElen(adf_scb, cp, cp+1, &CElen))
		{
		    goto common_return;
		}
		if (CElen == 0)
		{
		    /* The current character has no collation - we cannot
		    ** move beyond this codepoint as the bounds will not
		    ** be accurate if we do. Instead we don't allow the reverse
		    ** search to use any more characters. At worst this will
		    ** reduce the viable prefix length.
		    */
		    commentlen = i;
		    break;
		}
		adu_pat_MakeCEchar(adf_scb, &cp, cp+1,
                            &CEend, patdata->patdata.flags);
	    }
	    /* We need to perform the lookups into the CE list for each
	    ** character and lookup the coresponding from comment[].
	    ** If we are in UTF8mode - we don't want to write this UCS2 data
	    ** direct to lo[] & hi[] - instead we want it somewhere else so we
	    ** can convert back to UTF8. We will use *l1 and *h1 as we will have
	    ** finished with these as we go along. */
	    if (UTF8mode)
	    {
		lo1 = l1;
		hi1 = h1;
	    }
	    for (j = 0; j < pfx_len_lim; j++)
	    {
		u_i4 seen = 0;
		for (i = j; seen != (1|2) && i < commentlen; i++)
		{
		    UCS2 *pCE = &CE[(MAX_CE_LEVELS-1)*i];
		    if (!(seen & 1) &&
			pCE[0] == l1[j] &&
			pCE[1] == l2[j] &&
			pCE[2] == l3[j])
		    {
			lo1[j] = comment[i];
			seen |= 1;
		    }
		    if (!(seen & 2) &&
			pCE[0] == h1[j] &&
			pCE[1] == h2[j] &&
			pCE[2] == h3[j])
		    {
			hi1[j] = comment[i];
			seen |= 2;
		    }
		}
		if (seen == 1)
		    hi1[j] = AD_MAX_UNICODE;
		else if (seen == 2)
		    lo1[j] = AD_MIN_UNICODE;
		else if (seen != 3)
		    /* Drop out for rest of the ranges to be default filled */
		    break;
	    }
	}
#else
#	ifdef LITTLE_ENDIAN_INT
	  /* On little-endian we need helem data to be byte swapped. */
	  /* lo/hi[0..pfx_len_lim-1] == l1/h1 level 1 */
	  for (i = 0; i < pfx_len_lim; i++)
	  {
	    register UCS2 tmp = lo1[i];
	    lo1[i] = (tmp<<8)|(tmp>>8);
	    tmp = hi1[i];
	    hi1[i] = (tmp<<8)|(tmp>>8);
	  }
#	else
	    /* lo/hi[0..pfx_len_lim-1] is already setup */
#	endif
	j = pfx_len_lim;
	/* CE terminator */
	if (j < lohicount)
	{
	    lo1[j] = AD_MIN_UNICODE;
	    hi1[j] = AD_MAX_UNICODE;
	    j++;
	}
	/* level2 */
	for (i = 0; j < lohicount && i < pfx_len_lim; i++, j++)
	{
#	  ifdef LITTLE_ENDIAN_INT
	    register UCS2 tmp = l2[i];
	    lo1[j] = (tmp<<8)|(tmp>>8);
	    tmp = h2[i];
	    hi1[j] = (tmp<<8)|(tmp>>8);
#	  else
	    lo1[j] = l2[i];
	    hi1[j] = h2[i];
#	  endif
	}
	/* CE terminator */
	if (j < lohicount)
	{
	    lo1[j] = AD_MIN_UNICODE;
	    hi1[j] = AD_MAX_UNICODE;
	    j++;
	}
	/* level3 */
	for (i = 0; j < lohicount && i < pfx_len_lim; i++, j++)
	{
#	  ifdef LITTLE_ENDIAN_INT
	    register UCS2 tmp = l3[i];
	    lo1[pfx_len_lim] = (tmp<<8)|(tmp>>8);
	    tmp = h3[i];
	    hi1[pfx_len_lim] = (tmp<<8)|(tmp>>8);
#	  else
	    lo1[j] = l3[i];
	    hi1[j] = h3[i];
#	  endif
	}
#endif
	/* If we happen to have a full key match, drop the last CE to keep range */
	if (j >= lohicount && !patseen)
	    j = lohicount-1;
	/* CE terminators to the end of buffer */
	while (j < lohicount)
	{
	    lo1[j] = AD_MIN_UNICODE;
	    hi1[j] = AD_MAX_UNICODE;
	    j++;
	}
	if (UTF8mode)
	{
	    DB_DATA_VALUE dv;
	    dv.db_datatype = DB_NCHR_TYPE;
	    dv.db_length = lohicount*sizeof(UCS2);
	    dv.db_collID = DB_UNSET_COLL;
	    dv.db_prec = 0;
	    dv.db_data = (PTR)hi1;
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    if (!(db_stat = adu_nvchr_toutf8(adf_scb, &dv, hi_dv)))
	    {
		/* Trim the the chars that will become NULLs and upset toutf8 */
		while (pfx_len_lim > 0 &&
			lo1[pfx_len_lim-1] == AD_MIN_UNICODE)
		    pfx_len_lim--;
		if (!pfx_len_lim)
		    lo1[pfx_len_lim++] = 1;
		dv.db_length = pfx_len_lim*sizeof(UCS2);
		dv.db_data = (PTR)lo1;
		if (!(db_stat = adu_nvchr_toutf8(adf_scb, &dv, lo_dv)))
		{
		    /* If type is variable length - repad to full length
		    ** so that the lo & hi lengths match for the MEcmp ... */
		    if (lo_dv->db_datatype == DB_VCH_TYPE ||
			lo_dv->db_datatype == DB_TXT_TYPE ||
			lo_dv->db_datatype == DB_LTXT_TYPE)
		    {
			u_i2 l, h;
			I2ASSIGN_MACRO(*(u_i2*)lo_dv->db_data, l);
			I2ASSIGN_MACRO(*(u_i2*)hi_dv->db_data, h);
			if (h > l)
			    MEfill(h - l, '\0', lo_dv->db_data+l+sizeof(u_i2));
			I2ASSIGN_MACRO(h, *(u_i2*)lo_dv->db_data);
		    }
		    /* While the keys have been created to aid a range retrieval,
		    ** case differences may have screwed up the benefit. In UTF8
		    ** lower case collates before upper case and so a range for the
		    ** case insensitive retrieval of 'T%' (eqv. '\[Tt\]%') should
		    ** be 't\0x00...' to 'T\0xFF...' which is what we have built.
		    ** Sadly, this contravenes the ASCII collation that the range
		    ** retrieval will use and so a 'NOMATCH' scenario is triggered.
		    ** As range retrieval keys will usually be helpful we don't
		    ** just want to do a full table scan if we can avoid it so if
		    ** the generated keys look ok compared to each other we will
		    ** eave them intact - otherwise we will disable them and allow
		    ** full scan (or rather re-enable if ok) */
		    if (MEcmp(hi_dv->db_data, lo_dv->db_data, hi_dv->db_length) > 0)
			adc_kblk->adc_tykey = ADC_KRANGEKEY;
		}
	    }
	}
	if (memalloc)
	    MEfree(memalloc);
    }
common_return:
    if (patdata != &_patdata &&
	    (PTR)patdata != adc_kblk->adc_kdv.db_data)
	MEfree((PTR)patdata);
    return db_stat;
}


/*{
** Name: adu_patcomp_kbld_uni - Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.
**
**	This routine handles Unicode UCS_BASIC patterns and will pass non-Unicode
**	or UTF8 UCS_BASIC patterns to adu_patcomp_kbld and the other Unicodes
**	to adu_patcomp_kbld_uniCE
**
**	However, the type of operation to be used is also included amongst the
**	parameters.  The possible operators, and their results, are listed
**	below. Of these it is expected that only LIKE will be supported with
**	DB_PAT_TYPE.
**
**	    ADI_EQ_OP ('=')
**	    ADI_NE_OP ('!=')
**	    ADI_LT_OP ('<')
**	    ADI_LE_OP ('<=')
**	    ADI_GT_OP ('>')
**	    ADI_GE_OP ('>=')
**	    ADI_LIKE_OP
**
**	In addition to returning the key pair, the type of key produced is
**	returned.  The choices are
**
**	    ADC_KNOMATCH    --> No values will match this key.
**				    No key is formed.
**	    ADC_KEXACTKEY   --> The key will match exactly one value.
**				    The key that is formed is placed as the low
**				    key.
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.  The caller
**				    should seek to the low-key, then scan until
**				    reaching a point greater than or equal to
**				    the high key.
**	    ADC_KHIGHKEY    --> High key found -- only high key formed.
**	    ADC_KLOWKEY	    --> Low key found -- only low key formed.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				    No key was formed.
**
**	NOTE: As Unicode keys are still using the raw characters and not CE
**	data we cannot really aid Unicode range selection as the patdata data
**	contains only CE data and there is no efficient way of reversing the
**	mapping. Until CE data is used for keys we will err on the side of
**	caution and return ADC_KALLMATCH.
**	NOTE about NOTE: If the Pattern was provided with a PAT_COMMENT then
**	we can improve the ADC_KALLMATCH to a ADC_KRANGEKEY by aiding the
**	reversing of the CE mapping by limited lookup.
**
**	Given these values with LIKE, the combinations possible are shown
**	below.
**
**	    ADC_KRANGEKEY   --> Two keys were formed in the pair.
**	    ADC_KALLMATCH   --> The given value will match all values --
**				Returned when a generated KEY pair will
**				be suspect - typically due to collation.
**
**
** Inputs:
**      adf_scb                         scb
**      adc_kblk                        Pointer to key block data structure...
**	    .adc_kdv			The DB_DATA_VALUE to form
**					a key for.
**		.db_datatype		Datatype of data value.
**		.db_length		Length of data value.
**		.db_data		Pointer to the actual data.
**	    .adc_opkey			Key operator used to tell this
**					routine what kind of comparison
**					operation the key is to be
**					built for, as described above.
**					NOTE: ADI_OP_IDs should not be used
**					directly.  Rather the caller should
**					use the adi_opid() routine to obtain
**					the value that should be passed in here.
**          .adc_lokey			DB_DATA_VALUE to recieve the
**					low key, if built.  Note, its datatype
**					and length should be the same as
**					.adc_hikey's datatype and length.
**		.db_datatype		Datatype for low key.
**	    	.db_length		Length for low key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the low key.  If this
**					is NULL, no data will be written.
**	    .adc_hikey			DB_DATA_VALUE to recieve the
**					high key, if built.  Note, its datatype
**					and length should be the same as 
**					.adc_lokey's datatype and length.
**		.db_datatype		Datatype for high key.
**		.db_length		Length for high key (i.e. how
**					much space is reserved for the
**					data portion).
**		.db_data		Pointer to location to put the
**					actual data of the high key.  If this
**					is NULL, no data will be written.
**	    .adc_pat_flags		*** USED ONLY FOR THE `LIKE' family
**					of operators ***
**
** Outputs:
**      *adc_kblk                      Key block filled with following
**	    .adc_tykey                       Type key provided
**	    .adc_lokey                       if .adc_tykey is ADC_KRANGEKEY and
**					     key buffer passed.
**	    .adc_hikey			     if .adc_tykey is ADC_KRANGEKEY and
**					     key buffer passed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**
** History:
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation.
*/
DB_STATUS
adu_patcomp_kbld_uni (
ADF_CB		*adf_scb,
ADC_KEY_BLK	*adc_kblk)
{
    AD_PATDATA *patdata;
    AD_PATDATA _patdata;
    DB_STATUS db_stat = E_DB_OK;
    DB_DATA_VALUE *lo_dv   = &adc_kblk->adc_lokey;
    DB_DATA_VALUE *hi_dv   = &adc_kblk->adc_hikey;
    u_i4 i;
    u_i4 pfx_len_lim;
    u_i4 lohicount;
    u_i4 base = PATDATA_1STFREE;
    u_i4 nomatch = 0;
    UCS2 *lo = (UCS2*)lo_dv->db_data;
    UCS2 *hi = (UCS2*)hi_dv->db_data;
    UCS2 lowestchar = 0x0000;
    UCS2 highestchar = 0xFFFF;
    bool patseen = FALSE;

    if (adc_kblk->adc_opkey != ADI_LIKE_OP)
	return adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "patu kbld op");

    if (adc_kblk->adc_kdv.db_datatype != DB_PAT_TYPE)
	return adu_error (adf_scb, E_AD9998_INTERNAL_ERROR,
		2, 0, "patu kbld dt");

    patdata = (AD_PATDATA *)adc_kblk->adc_kdv.db_data;
    {
	u_i2 flags;

	if (NULL == patdata)
	{
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}

	/* Hand off non-Unicode pattern */
	I2ASSIGN_MACRO(patdata->patdata.flags2, flags);
	if (flags & AD_PAT2_UNICODE_CE)
	    return adu_patcomp_kbld_uniCE(adf_scb, adc_kblk);
	if (~flags & AD_PAT2_UCS_BASIC)
	    return adu_patcomp_kbld(adf_scb, adc_kblk);

	/* We cannot optimise to a range if:
	** - blanks are to be ignored
	** - case is to be ignored*/
	I2ASSIGN_MACRO(patdata->patdata.flags, flags);
	if (flags & (AD_PAT_BIGNORE|AD_PAT_WO_CASE))
	{
	    /* Text modification is implied that renders this
	    ** impractical, return ALLMATCH */
	    adc_kblk->adc_tykey = ADC_KALLMATCH;
	    return E_DB_OK;
	}
    }

    /* We will probably return ADC_KRANGEKEY so set that now */
    adc_kblk->adc_tykey = ADC_KRANGEKEY;

    /* If no key buffers passed this is a query as to whether
    ** an equality lookup (HASH) can be supported. As LIKE cannot
    ** return ADC_KEXACTKEY or ADC_KNOMATCH we go no further than
    ** say range match for now. If asked for the hi/lo keys we can
    ** later switch to ALL if need be. */
    if (!lo || !hi)
	return E_DB_OK;

    if (ME_ALIGN_MACRO(patdata, sizeof(i2)) != (PTR)patdata)
    {
	if ((i4)sizeof(_patdata) >= adc_kblk->adc_kdv.db_length)
	    patdata = &_patdata;
	else
	{
	    patdata = (AD_PATDATA*)MEreqmem(0, adc_kblk->adc_kdv.db_length,
			FALSE, &db_stat);
            if (!patdata || db_stat)
		return db_stat;
	}
	MEcopy(adc_kblk->adc_kdv.db_data,
		adc_kblk->adc_kdv.db_length, patdata);
    }
    
    /* Now get string length of low and high keys (assumed to be the same) */
    lohicount = lo_dv->db_length / sizeof(UCS2);
    /* Write out the length of the results if NVARCHAR */
    if (lo_dv->db_datatype == DB_NVCHR_TYPE)
	*lo++ = *hi++ = --lohicount;
    /* The Unicode UCS_BASIC helem consists of the raw code points */
    pfx_len_lim = lohicount;

    /* It is very unlikely that we will have any great length
    ** of prefix-pattern as most pattern operators will break it */
    for (i = 0; i < patdata->patdata.npats &&
		base < patdata->patdata.length; i++)
    {
	u_i4 patlen = patdata->vec[base];
	u_i1 *pc = (u_i1*)&patdata->vec[base+1];
	u_i1 *patbufend = (u_i1*)&patdata->vec[base+patlen];
	u_i4 pfx_len = 0;
	i8 N, M, v;
	i4 n;
	bool wild = FALSE;

	while (pc < patbufend && !wild)
	{
	    u_i1 op = *pc++;

	    if (opcodes[op].v1 != V1__)
		N = opcodes[op].v1;
	    else if (opcodes[op].p1 == P1_N)
		N = GETNUM(pc);
	    if (opcodes[op].p2 == P2_M)
		M = GETNUM(pc);
	    else if (opcodes[op].v2 == V2_W)
		wild = 1;
	    switch (opcodes[op].cl)
	    {
	    case opClass__:
		switch (op)
		{
		default:
		    /* Most operators we use to terminate the key */
		    patseen = TRUE;
		    /*FALLTHROUGH*/
		case PAT_FINAL:
		    pc = patbufend;
		    break;
		case PAT_NOP:
		case PAT_TRACE:
		case PAT_NOTRACE:
		case PAT_BOS:
		case PAT_EOS:
		case PAT_BOM:
		case PAT_EOM:
		    continue;

/*ANY*/		case PAT_ANY_0_W:
		case PAT_ANY_1:
		case PAT_ANY_1_W:
		case PAT_ANY_N:
		case PAT_ANY_N_W:
		case PAT_ANY_N_M:
		    goto anyn;
		}

		break;
	    case opClass_C:/* CASE */
	    case opClass_J:/* Jump/Label */
		v = GETNUM(pc);
		patseen = TRUE;
		/* No point looking further for this slot */
		pc = patbufend;
		break;

	    case opClass_L:/* Literal text */
		/* What this is all about - we have a prefix string:
		** This needs adding as a prefix to both low and high */
		v = GETNUM(pc);
		if (op != PAT_LIT)
		{
		    if (op == PAT_FNDLIT || op == PAT_ENDLIT)
		    {
			/* FNDLIT/ENDLIT are essentially a % so treat
			** as such for keybld */
			wild = TRUE;
			goto anyn;
		    }
		    pc += v;
		    break;
		}
		if (patdata->patdata.flags & AD_PAT_WO_CASE)
		{
		    const UCS2 *upc = (UCS2 *)pc;
		    UCS2 *plo = &lo[pfx_len];
		    UCS2 *phi = &hi[pfx_len];
		    pc += v;
		    while (upc < (UCS2*)pc && pfx_len < pfx_len_lim)
		    {
			if (i == 0)
			{
			    *plo = 0xFFFF;
			    *phi = 0x0000;
			}
			adu_patda_ucs2_minmax_1((ADUUCETAB *)adf_scb->adf_ucollation,
				&upc, (UCS2*)pc, &plo, &phi);
			pfx_len = max(plo-lo, phi-hi);
		    }
		}
		else
		{
		    UCS2 *upc = (UCS2 *)pc;
		    pc += v;
		    while (upc < (UCS2 *)pc && pfx_len < pfx_len_lim)
		    {
			UCS2 ch = *pc++;
			if (i == 0)
			    lo[pfx_len] = hi[pfx_len] = ch;
			else
			{
			    if (lo[pfx_len] > ch)
				lo[pfx_len] = ch;
			    if (hi[pfx_len] < ch)
				hi[pfx_len] = ch;
			}
			pfx_len++;
		    }
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (v > lohicount - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (v)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

/*SET*/	    case opClass_S:/* Subset */
		/* We have a low and a high character:
		** Low at *pc (or +1 if range introducer) and high at the
		** end of the set literal */
		patseen = TRUE;
		v = GETNUM(pc);
		{
		    UCS2 *lo_ch = (UCS2*)pc; /* PAT literals UCS2 aligned */
		    UCS2 *hi_ch;
		    /* The lower range char MIGHT be a range
		    ** operator - if so get the next char */
		    if (*lo_ch == U_RNG)
			lo_ch++;
		    /* Find the start of the upper range char - easy -
		    ** it's the high end is the last on in the set */
		    pc += v;
		    hi_ch = (UCS2*)pc;
		    hi_ch--;

		    while (N > 0 && pfx_len < pfx_len_lim)
		    {
			if (pfx_len < pfx_len_lim)
			{
			    if (i == 0)
			    {
				lo[pfx_len] = *lo_ch;
				hi[pfx_len] = *hi_ch;
			    }
			    else
			    {
				if (lo[pfx_len] > *lo_ch)
				    lo[pfx_len] = *lo_ch;
				if (hi[pfx_len] < *hi_ch)
				    hi[pfx_len] = *hi_ch;
			    }
			    pfx_len++;
			}
			N--;
		    }
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > lohicount - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

/*BSET*/    case opClass_B:/* Bitset - for unicode */
		/* We have a low and a high character:
		** Low at *pc (or +1 if range introducer) and high at the
		** end of the set literal */
		patseen = TRUE;
		n = N;
		v = GETNUM(pc);
		{
		    UCS2 _lo_ch[4];
		    UCS2 _hi_ch[4];
		    const UCS2 *lo_ch = _lo_ch;
		    const UCS2 *hi_ch = _hi_ch;
		    UCS2 _lo_ch2[4];
		    UCS2 _hi_ch2[4];
		    _lo_ch[0] = v;
		    v = GETNUM(pc);
		    _hi_ch[0] = v * 8 + _lo_ch[0];
		    pc += v;
		    if (patdata->patdata.flags & AD_PAT_WO_CASE)
		    {
			UCS2 *lo_ch2 = _lo_ch2;
			UCS2 *hi_ch2 = _hi_ch2;
			_lo_ch2[0] = 0xFFFF;
			_hi_ch2[0] = 0x0000;
			lo_ch2 = _lo_ch2;
			hi_ch2 = _hi_ch2;
			adu_patda_ucs2_minmax_1((ADUUCETAB *)adf_scb->adf_ucollation,
				&lo_ch, lo_ch+1, &lo_ch2, &hi_ch2);
			lo_ch2 = _lo_ch2;
			hi_ch2 = _hi_ch2;
			adu_patda_ucs2_minmax_1((ADUUCETAB *)adf_scb->adf_ucollation,
				&hi_ch, hi_ch+1, &lo_ch2, &hi_ch2);
			lo_ch = _lo_ch2;
			hi_ch = _hi_ch2;
		    }
		    while (N > 0 && pfx_len < pfx_len_lim)
		    {
			if (i == 0)
			{
			    lo[pfx_len] = *lo_ch;
			    hi[pfx_len] = *hi_ch;
			}
			else
			{
			    if (lo[pfx_len] > *lo_ch)
				lo[pfx_len] = *lo_ch;
			    if (hi[pfx_len] > *hi_ch)
				hi[pfx_len] = *hi_ch;
			}
			N--;
			pfx_len++;
		    }
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > lohicount - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

/*NBSET*/   case opClass_NB:/* Bitset */
		v = GETNUM(pc);	    /*skip offset*/
		/*FALLTHROUGH*/

/*NSET*/    case opClass_NS:/* Subset */
		v = GETNUM(pc);
		pc += v;
anyn:		/* We know min length but no more */
		patseen = TRUE;
		if (!pfx_len && wild)
		{
		    /* Don't return range key if really a scan needed */
	    	    adc_kblk->adc_tykey = ADC_KALLMATCH;
		    goto common_return;
		}
		while (N > 0 && pfx_len < pfx_len_lim)
		{
		    lo[pfx_len] = AD_MIN_UNICODE;
		    hi[pfx_len] = AD_MAX_UNICODE;
		    N--;
		    pfx_len++;
		}
		/* If still pattern left then this slot might not
		** ever match the column */
		if (N > lohicount - pfx_len_lim && *pc != PAT_MATCH)
		    nomatch++;
		if (N || wild)
		    /* No point looking further for this slot */
		    pc = patbufend;
		break;

	    }
	}
	/* We have to use the shortest of keys (least specific) */
	if (pfx_len < pfx_len_lim)
	    pfx_len_lim = pfx_len;
	base += patlen;
    }
    if (nomatch == patdata->patdata.npats)
    {
	/* At present there is no useful way that we can
	** make use of this fact so we leave things as a
	** range lookup.
	**
	**  adc_kblk->adc_tykey = ADC_KNOMATCH;
	**  goto common_return;
	*/
    }
    if (!patseen && patdata->patdata.npats == 1)
    {
	/* As this is effectivly an EQ, we could do the following
	** BUT it breaks REPEATABLE selects in that they loop endlessly!
	**
	**  adc_kblk->adc_tykey = ADC_KEXACTKEY;
	**  lohicount = pfx_len_lim;
	*/
    }

    {
	register u_i4 j = pfx_len_lim;
	register UCS2 *lo1 = lo, *hi1 = hi;

	/* If we happen to have a full key match, drop the last char to keep range */
	if (j >= lohicount && !patseen)
	    j = lohicount-1;
	/* CE terminators to the end of buffer */
	while (j < lohicount)
	{
	    lo1[j] = AD_MIN_UNICODE;
	    hi1[j] = AD_MAX_UNICODE;
	    j++;
	}
    }
common_return:
    if (patdata != &_patdata && (PTR)patdata != adc_kblk->adc_kdv.db_data)
	MEfree((PTR)patdata);
    return db_stat;
}


/*{
** Name: adu_patcomp_summary - Summarise characteristics of pattern
**
** Description:
**      This routine returns a representation of the type of pattern
**	present such that decisions can be made to use change or drop.
**
** Inputs:
**	pat_dv,			DBV Pattern to summarise
**	    .db_datatype	Datatype of pattern value (DB_PAT_TYPE).
**	    .db_length		Length of pattern value.
**	    .db_data		Pointer to the actual pattern.
** 
** Outputs:
**
**	eqv_dv			Equivalent string
**	    .db_datatype	Datatype of data value.
**	    .db_length		Length of data value.
**	    .db_data		Pointer to the actual date.
**				Pass as NULL if not needed.
**
**	Returns:
**	  typedef enum _PAT_IS {
**	    ADU_PAT_IS_NORMAL	Nothing special
**	    ADU_PAT_IS_MATCH	Match all - no restriction
**	    ADU_PAT_IS_PURE	Pattern could be used with =
**	    ADU_PAT_IS_LITERAL	eqv_dv pattern could be =
**	    ADU_PAT_IS_NOMATCH	Can never match anything
**	    ADU_PAT_IS_FAIL	Pattern has failure bit marked
**	  } ADU_PAT_IS;
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**
** History:
**	10-Feb-2009 (kiria01) SIR120473
**	    Created
**	
*/

ADU_PAT_IS
adu_patcomp_summary (
DB_DATA_VALUE	*pat_dv,
DB_DATA_VALUE	*eqv_dv)
{
    AD_PATDATA *patdata;
    u_i2 flags;
    u_i2 flags2;

    if (pat_dv->db_datatype == DB_PAT_TYPE &&
	(patdata = (AD_PATDATA *)pat_dv->db_data))
    {
	I2ASSIGN_MACRO(patdata->patdata.flags, flags);
	I2ASSIGN_MACRO(patdata->patdata.flags2, flags2);
	
	if (flags2 & AD_PAT2_FORCE_FAIL)
	    return ADU_PAT_IS_FAIL;

	if (flags2 & AD_PAT2_PURE_LITERAL)
	    return ADU_PAT_IS_PURE;

	if (flags2 & AD_PAT2_MATCH)
	    return ADU_PAT_IS_MATCH;

	if (eqv_dv &&
	    (flags2 & AD_PAT2_LITERAL))
	{
	    /* Unicode CE literals will be of no use as they will
	    ** be encoded as CE data so next bit only for
	    ** non-unicode */
	    if (~flags2 & AD_PAT2_UNICODE_CE)
	    {
		u_i1 *pc = &patdata->patdata.first_op;
		i4 L = 0;
		u_i1 *S = pc;
		if (*pc == PAT_LIT && !adu_pat_decode(&pc, 0, 0, &L, &S, 0) ||
		    *pc == PAT_FINAL)
		{
		    i4 len = eqv_dv->db_length;
		    char *addr = eqv_dv->db_data;
		    switch (eqv_dv->db_datatype)
		    {
		    case DB_TXT_TYPE:
		    case DB_LTXT_TYPE:
		    case DB_VCH_TYPE:
		    case DB_VBYTE_TYPE:
		    case DB_UTF8_TYPE:
		    case DB_NVCHR_TYPE:
			if (L <= len-(i4)sizeof(i2))
			{
			    if (L)
				MEcopy(S, L, addr+sizeof(i2));
			    if (eqv_dv->db_datatype == DB_NVCHR_TYPE)
				L /= sizeof(UCS2);
			    *(i2*)addr = L;
			    return ADU_PAT_IS_LITERAL;
			}
			break;
		    case DB_CHA_TYPE:
		    case DB_CHR_TYPE:
		    case DB_BYTE_TYPE:
		    case DB_NCHR_TYPE:
			if (L <= len)
			{
			    if (L)
				MEcopy(S, L, addr);
			    eqv_dv->db_length = L;
			    return ADU_PAT_IS_LITERAL;
			}
		    }	
		}
	    }
	}
    }
    return ADU_PAT_IS_NORMAL;
}
