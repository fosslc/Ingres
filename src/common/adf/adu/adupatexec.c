/*
** Copyright (c) 2008,2009 Ingres Corporation
*/

/**
**
**  Name: adupatexec.c - Pattern execution
**
**  Description:
**	This file contains the execution engines shells for the pattern match
**	code. Each shell iimplements the state control and data access needed
**	to support the pattern matching itself. The matching logic itself is
**	imported from an generated state machine that is included in each shell
**	to ensure that the logic used will be the same for each datatype.
**
**
** This file defines the following externally visible routines:
**
**	adu_pat_execute()  - Do the pattern scan
**	adu_pat_execute_col()  - Do the pattern scan for collated data
**	adu_pat_execute_uni()  - Do the pattern scan for unicode
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      04-Apr-2008 (kiria01) SIR120473
**          Initial creation.
**      30-Dec-2008 (kiria01) SIR120473
**          Adjust the trace output for unicode.
**	22-May-2009 (kiria01) b122128
**	    Drop unwarranted UTF8 checks from CM macros
**	    Move ctx->parent->bufend into register
**	08-Jan-2010 (kiria01) b123118
**	    Corrected the datatype of the temporary variables
**	    to be signed.
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
#include <aduucol.h>
#include <adulcol.h>
#include <nm.h>

/* Pull in the operator definitions */
#include "adupatexec.h"

/*		*********************
**		*** Big UTF8 NOTE ***
**		*********************
** This module is designed so that it never need process UTF8 chars itself
** as they will have been converted to NVARCHARs and handled with them.
** Therefore to remove the redundant per-character UTF8 overheads - they
** have been short-circuited below.
*/
#ifdef CMGETUTF8
#undef CMGETUTF8
#endif
#define CMGETUTF8 0

#ifndef REGSPATSLOW
#define REG_DEFS register u_i1 *REG_PC, *REG_CH,\
		    *REG_litset, *REG_litsetend, *REG_bufend;\
		 register u_i4 REG_L,REG_M,REG_N,REG_O;
#define REG_LOAD(_ctx) \
{\
    REG_PC = _ctx->pc;\
    REG_CH = _ctx->ch;\
    REG_L = _ctx->L;\
    REG_M = _ctx->frame[0].M;\
    REG_N = _ctx->frame[0].N;\
    REG_O = _ctx->O;\
    REG_litset = _ctx->litset;\
    REG_litsetend = _ctx->litsetend;\
    REG_bufend = _ctx->parent->bufend;\
}
#define REG_STORE(_ctx) \
{\
    _ctx->pc = REG_PC;\
    _ctx->ch = REG_CH;\
    _ctx->L = REG_L;\
    _ctx->frame[0].M = REG_M;\
    _ctx->frame[0].N = REG_N;\
    _ctx->O = REG_O;\
    _ctx->litset = REG_litset;\
    _ctx->litsetend = REG_litsetend;\
}
#define REG_SYNC(_ctx) \
{\
REG_PC = _ctx->pc;\
}
#else
#define REG_PC (ctx->pc)
#define REG_CH (ctx->ch)
#define REG_L (ctx->L)
#define REG_M (ctx->frame[0].M)
#define REG_N (ctx->frame[0].N)
#define REG_O (ctx->O)
#define REG_litset (ctx->litset)
#define REG_litsetend (ctx->litsetend)
#define REG_bufend (ctx->parent->bufend)
#define REG_DEFS
#define REG_LOAD(_ctx)
#define REG_STORE(_ctx)
#define REG_SYNC(_ctx)
#endif
/*
** Pattern complexity limits
*/
u_i4 ADU_pat_cmplx_lim = 10;	/* MO:exp.adf.adu.pat_cmplx_lim */
u_i4 ADU_pat_cmplx_max = 0;	/* MO:exp.adf.adu.pat_cmplx_max */


/*
** Generic operator callout functions common to the date types:
**
** Presently most of these are placeholders for later
** functionality.
*/
static void BUGCHK() {}

static void actPAT_BUGCHK(AD_PAT_CTX *ctx) {BUGCHK();}

static void actPAT_BOM(AD_PAT_CTX *ctx) {}

static void actPAT_EOM(AD_PAT_CTX *ctx) {}

static bool tstPAT_BEGIN(AD_PAT_CTX *ctx){return 1;}

static bool tstPAT_END(AD_PAT_CTX *ctx){return 1;}

static bool tstPAT_BOW(AD_PAT_CTX *ctx){return 1;}
static bool tstPAT_EOW(AD_PAT_CTX *ctx){return 1;}

/*
** PAT_BOS - begining of stream/string primitive
*/
static bool tstPAT_BOS(AD_PAT_CTX *ctx){
    return ctx->parent->at_bof &&
	ctx->ch == ctx->parent->buffer;
}

/*
** PAT_EOS - end of stream/string primitive
*/
static bool tstPAT_EOS(AD_PAT_CTX *ctx){
    return ctx->parent->at_eof &&
	ctx->ch == ctx->parent->bufend;
}


/*{
** Name: alloc_ctx() - Allocate a PAT context
**
** Description:
**	Allocates a PAT ctx
**
** Inputs:
**      ctx - active exection state
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
**	Removes a free execution context or allocates one if needed.
**
** History:
*/
AD_PAT_CTX *alloc_ctx(AD_PAT_CTX *ctx){
    AD_PAT_CTX *ctx2 = ctx->parent->free;
    if (!ctx2)
    {
	DB_STATUS db_stat;
	if (++ctx->parent->nctxs_extra >= ADU_pat_cmplx_max &&
	    (ADU_pat_cmplx_max = ctx->parent->nctxs_extra) > ADU_pat_cmplx_lim)
	{
	    ctx->parent->cmplx_lim_exc = 1;
	    return NULL;
    	}
	ctx2 = (AD_PAT_CTX *)MEreqmem(0, sizeof(*ctx2), FALSE, &db_stat);
	if (db_stat)
	    return NULL;
	++ctx->parent->nctxs_extra;
    }
    else
	ctx->parent->free = ctx2->next;
    return ctx2;
}


/*{
** Name: FORK() - Spawn a pattern context & queue for execution
**
** Description:
**	Creates a copy of the execution state and queues it for 
**	running from a new state.
**
** Inputs:
**      ctx - active exection state
**	state - the state for the child to run at
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
**	Removes a free execution context or allocates one if needed.
**	Queues the context for exection on the pending list.
**
** History:
*/
static void FORK(AD_PAT_CTX *ctx, enum PAT_OP_enums state){
    AD_PAT_CTX *ctx2 = alloc_ctx(ctx);
    if (!ctx2)
	return;

    *ctx2 = *ctx;
    ctx2->state = state;
    ctx2->next = ctx->parent->pending;
    ctx->parent->pending = ctx2;
#if PAT_DBG_TRACE>1
    ctx2->id = ctx->parent->nid++;
    ctx2->idp = ctx->id;
    if (ctx->parent->trace)
    {
	TRdisplay("%d|%d FORK %d %d\n", ctx->idp, ctx->id, ctx2->id, state);
    }
#endif
}


/*{
** Name: FORKB() - Spawn a block pattern context & queue for execution
**
** Description:
**	Creates a copy of the execution state and queues it for 
**	running from a new pc.
**
** Inputs:
**      ctx - active exection state
**	offset - the pattern pc offset for the child to run at
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
**	Removes a free execution context or allocates one if needed.
**	Queues the context for exection on the pending list.
**
** History:
*/
static void FORKB(AD_PAT_CTX *ctx, i4 offset){
    AD_PAT_CTX *ctx2 = alloc_ctx(ctx);
    if (!ctx2)
	return;

    *ctx2 = *ctx;
    ctx2->pc += offset;
    ctx2->state = PAT_CASE;
    ctx2->next = ctx->parent->pending;
    ctx->parent->pending = ctx2;
#if PAT_DBG_TRACE>1
    ctx2->id = ctx->parent->nid++;
    ctx2->idp = ctx->id;
    if (ctx->parent->trace)
    {
	TRdisplay("%d|%d FORKB %d %d\n", ctx->idp, ctx->id, ctx2->id, offset);
    }
#endif
}


/*{
** Name: FORKN() - Spawn a set of pattern contexts & queue for execution
**
** Description:
**	Creates n-1 copy of the execution state and queues them for 
**	running from a new state. Each copy will have its pc adjusted
**	to point to its own block of opcodes to use.
**	This caters for the bracketed alternation syntax of
**	  xxx(pat2|pat1|pat0)yyyy
**	Which becomes:
**		xxx
**		CASE 2
**	    l2:	LABEL len(pat2)	    (=l1-p2)
**	    p2:	pat2
**		JUMP end	    (=end-l1)
**	    l1:	LABEL len(pat1)	    (=p0-p1)
**	    p1:	pat1
**		JUMP end	    (=end-p0)
**	    p0:	pat0
**	    end:yyyy
**
**	In effect this result in separate patterns contexts of
**	  xxxpat0yyyy
**	  xxxpat1yyyy
**	  xxxpat2yyyy
**
**	With pat0 executed in the parent and pat1 and pat2 in the context
**	of the 1st and 2nd children respectivly.
**	(These can of course be nested)
**
** Inputs:
**      ctx - active exection state
**	n - the number of children to spawn
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
**	Removes n free execution context or allocates them if needed.
**	Queues the contexts for exection on the pending list.
**
** History:
*/
static void FORKN(AD_PAT_CTX *ctx, u_i4 n){
    while (n)
    {
	i8 offset;
	AD_PAT_CTX *ctx2 = alloc_ctx(ctx);
	if (!ctx2)
	    return;

	if (*ctx->pc++ != PAT_LABEL)
	{
	    /*FIXME sanity check*/
	}
	offset = GETNUM(ctx->pc); /* pat length */
	/* REG_PC is now pointing at the first instruction
	** of the next pattern - spawn now (copy ctx and queue) */
	*ctx2 = *ctx;
	/* The child starts with a state of CASE from parent */
	ctx2->next = ctx->parent->pending;
	ctx->parent->pending = ctx2;
#if PAT_DBG_TRACE>1
	ctx2->id = ctx->parent->nid++;
	ctx2->idp = ctx->id;
	if (ctx->parent->trace)
	{
	    TRdisplay("%d|%d FORKN %d %d\n", ctx->idp, ctx->id, ctx2->id, n);
	}
#endif
	/* Need to skip the pattern codes that we've forked a context for. */
	ctx->pc += offset;
	/* pc now points to the next pattern block label or the parents next
	** instruction - loop for next or exit */
	n--;
    }
}


/*
** Name: adu_pat_execute() - Basic state machine for patterns
**
**      This routine compares a data stream or string defined by da_ctx
**	with one or more patterns in sea_ctx.
**
**	This varient of the engine handles non-unicode, non-collating
**	character data types.
**	For the collating equivalent, see the routine adu_pat_execute_col()
**	and for the unicode equivalent, see routine adu_pat_execute_uni()
**
** Inputs:
**	sea_ctx		This structure contains the execution contects and
**			state for the pattern(s). This routine will manage
**			these as its scans through the data from da_ctx.
**	da_ctx		This structure defines the input data. It could be
**			a simple string or long object, eiter way the data
**			be passed back in a standard manner.
**
** Outputs:
**	rcmp				Result of comparison, as follows:
**					    <0  if  str_dv1 < pat_dv2
**					    =0  if  str_dv1 = pat_dv2
**					    >0  if  str_dv1 > pat_dv2
**
** Returns:
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
DB_STATUS
adu_pat_execute(
	AD_PAT_SEA_CTX *sea_ctx,
	AD_PAT_DA_CTX *da_ctx,
	i4 *rcmp)
{
    typedef u_char CHAR;
    DB_STATUS db_stat = E_DB_OK;
    AD_PAT_CTX *ctx, **p;
    i8 tmp;
    REG_DEFS
    register CHAR *t;
    register i4 last_diff = 1;
    register u_i4 ch;

    while (sea_ctx->pending = sea_ctx->stalled)
    {
	sea_ctx->stalled = 0;
	/* Get next/only segment for all stalled */
	if (db_stat = adu_patda_get(da_ctx))
	    break;

	while (ctx = sea_ctx->pending){
	    /* Unlink from pending list */
	    sea_ctx->pending = ctx->next;
	    REG_LOAD(ctx)
#if PAT_DBG_TRACE>1
	    if (sea_ctx->trace)
	    {
		if (!REG_CH)
		    /* We're about to do this anyway but need it for trace */
		    REG_CH = sea_ctx->buffer;
		TRdisplay("%d|%d %s %d ch[%.#s]\n",
			ctx->idp, ctx->id,
			ctx->state == PAT_NOP ? "START":"RESUME",
			ctx->state,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
	    /*
	    ** True beginings will have current state set to NOP
	    ** other states will reflect a restart from stall and will
	    ** be the true restart state or a child from CASE.
	    */
	    if (ctx->state == PAT_NOP)
		/* Set current char pointer to start of data buffer */
		REG_CH = sea_ctx->buffer;
	    else if (ctx->state != PAT_CASE)
		goto _restart_from_stall;


/*
** We are about to include the file that has the state machine in
** it generated from the dotty file. It defines 2 entry points
** and jumps out to one of four exits.
** The first entry is the 'GetPatOp' entry at the
** true start. The second is the _restart_from_stall
** entry for doing just that.
** The various exit points relate to the state of the operation.
**
** To allow for the same state table to be used for each shell, reference
** will be made to the macros below. These allow the datatypes specifics
** to be isolated.
*/
#define DIFFPAT(lbl) if (COMPARECH(REG_litset)) goto lbl;
#define NEXTCH ADV(REG_CH);
#define NEXTPAT ADV(REG_litset);

#define COMPARECH(b) (last_diff=CMcmpcase(REG_CH, b))
#define ADV(ptr) CMnext(ptr)
#define HAVERANGE(ptr) (*(ptr)==AD_5LIKE_RANGE?(ptr)++:0)
#if PAT_DBG_TRACE>1
#define DISASS(ctx) \
    if (ctx->parent->trace){\
	char buf[140];\
	u_i1 *tmp = REG_PC;\
	i8 v = 0;\
	char *p = buf;\
	if (REG_litsetend&&REG_litset&&REG_litsetend>REG_litset)\
	    TRdisplay("%d|%d %p N=%d M=%d ch[%.#s]l=%d cf[%.#s]\n",\
		    ctx->idp, ctx->id, REG_PC, REG_N, REG_M,\
		    min(8,REG_bufend-REG_CH), REG_CH,\
		    REG_litsetend-REG_litset,\
		    min(4,REG_litsetend-REG_litset), REG_litset);\
	else\
	    TRdisplay("%d|%d %p N=%d M=%d ch[%.#s]\n",\
		    ctx->idp, ctx->id, REG_PC, REG_N, REG_M,\
		    min(8,REG_bufend-REG_CH), REG_CH);\
	while (adu_pat_disass(buf, sizeof(buf), &tmp, &p, &v, FALSE)){\
	    *p++ = '\n';\
	    TRwrite(0, p - buf, buf);\
	    p = buf;\
	    *p++ = '\t';\
	    *p++ = '\t';\
	}\
	*p = '\n';\
	TRwrite(0, p - buf, buf);\
    }
#else
#define DISASS(ctx)
#endif

#include "adupatexec_inc.i"

#undef DISASS
#undef HAVERANGE
#undef ADV
#undef COMPARECH

#undef NEXTPAT
#undef NEXTCH
#undef DIFFPAT


_SUCCESS:
#if PAT_DBG_TRACE>1
/*SUCCESS*/ if (sea_ctx->trace)
	    {
		TRdisplay("%d|%d SUCCESS ch[%.#s]\n",
			ctx->idp, ctx->id,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
/*SUCCESS*/ *rcmp = 0;
	    return db_stat;

_FAILm1:    last_diff = -1;
	    goto _FAIL;

_FAIL1:	    last_diff = 1;
_FAIL:
#if PAT_DBG_TRACE>1
/*FAIL*/    if (sea_ctx->trace)
	    {
		TRdisplay("%d|%d FAILED %d ch[%.#s]\n",
			ctx->idp, ctx->id, last_diff,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
/*FAIL*/    ctx->next = sea_ctx->free;
	    sea_ctx->free = ctx;
	    continue;

	_STALL:
/*STALL*/   REG_STORE(ctx)
#if PAT_DBG_TRACE>1
/*STALL*/   if (sea_ctx->trace)
	    {
		TRdisplay("%d|%d STALLED ch[%.#s]\n",
			ctx->idp, ctx->id,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
	    /*
	    ** Reset current char pointer to start of data buffer
	    ** which will be loaded with new data shortly
	    */
/*STALL*/   ctx->ch = sea_ctx->buffer;
	    /* Add to end of stalled list */
	    for (p = &sea_ctx->stalled; *p; p = &(*p)->next)
		/*SKIP*/;
	    ctx->next = 0;
	    *p = ctx;
	    continue;
	}
    }
    *rcmp = last_diff;
    return db_stat;
}


/*
** Name: adu_pat_execute_col() - Collation pattern state machine
**
**      This routine compares a data stream or string defined by da_ctx
**	with one or more patterns in sea_ctx.
**
**	This varient of the engine handles non-unicode data typess that use
**	collation. For the base equivalent, see the sister routine
**	adu_pat_execute()
**
** Inputs:
**	sea_ctx		This structure contains the execution contects and
**			state for the pattern(s). This routine will manage
**			these as its scans through the data from da_ctx.
**	da_ctx		This structure defines the input data. It could be
**			a simple unicode string or long object, eiter way
**			the data be passed back in a standard manner.
**			If it needed normalising or case folding it will
**			have been done by da_ctx.
**
** Outputs:
**	rcmp				Result of comparison, as follows:
**					    <0  if  str_dv1 < pat_dv2
**					    =0  if  str_dv1 = pat_dv2
**					    >0  if  str_dv1 > pat_dv2
**
** Returns:
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
**	05-Mar-2009 (kiria01) b121748
**	    Add support for end of string to be marked by pointer/len
**	    instead of just NULL terminated that adul relied on before.
**	    Update REG_CH at end of buffer condition when skipping
**	    zero collation width characters.
**      17-Sep-2009 (hanal04) Bug 122531
**          Correct collation's NEXTCH to avoid infinite loop for
**          non-ADUL_TMULT, non-ADUL_SKVAL character with existing
**          hold state weighting.
*/
DB_STATUS
adu_pat_execute_col(
	AD_PAT_SEA_CTX *sea_ctx,
	AD_PAT_DA_CTX *da_ctx,
	i4 *rcmp)
{
    typedef u_char CHAR;
    DB_STATUS db_stat = E_DB_OK;
    i8 tmp;
    AD_PAT_CTX *ctx, **p;
    REG_DEFS
    register CHAR *t;
    register i4 last_diff = 1;
    u_i4 ch;
    ADULcstate patch;
    /* Unpacked ADULcstate for REG_CH */
    ADULTABLE	*adul_coltbl = (ADULTABLE *)da_ctx->adf_scb->adf_collation;
    i2		adul_holdstate;
    u_char	*adul_nextch;
    i4		adul_val, patl;

    struct {
	i2 holdstate;
	u_char *nextch;
	i4 val;
    } hold, pat;

#define CHPRIME \
{\
    register u_char *s = REG_CH;\
    adul_holdstate = -1;\
    do\
    {\
	adul_val = adul_coltbl->firstchar[*s++];\
	adul_nextch = s;\
	while (adul_val & ADUL_TMULT)\
	{\
	    struct ADUL_tstate *sp;\
	    sp = adul_coltbl->statetab + (adul_val & ADUL_TDATA);\
	    if (sp->flags & ADUL_LMULT)\
	    {\
		adul_val = sp->match;\
		adul_holdstate = sp->nomatch;\
	    }\
	    else if (*s == sp->testchr)\
	    {\
		adul_val = sp->match;\
		s++;\
		if (!(sp->flags & ADUL_FINAL))\
		    adul_nextch = s;\
	    }\
	    else\
		adul_val = sp->nomatch;\
	}\
    } while(adul_val == ADUL_SKVAL);\
    /* adul_val is set ready */\
}

#define CHSAVE(x) x.holdstate = adul_holdstate;\
		    x.nextch = adul_nextch;\
		    x.val = adul_val;

#define CHRESTORE(x) adul_holdstate = x.holdstate;\
		    adul_nextch = x.nextch;\
		    adul_val = x.val;

    while (sea_ctx->pending = sea_ctx->stalled)
    {
	sea_ctx->stalled = 0;
	/* Get next/only segment for all stalled */
	if (db_stat = adu_patda_get(da_ctx))
	    break;

	while (ctx = sea_ctx->pending){
	    /* Unlink from pending list */
	    sea_ctx->pending = ctx->next;
	    REG_LOAD(ctx)
	    /*
	    ** ADUL NOTE: every time we (re)start a context
	    ** the adul stat needs setting as though we are at
	    ** a collation character boundary.
	    */
	    if (ctx->state == PAT_NOP || !REG_CH)
		REG_CH = sea_ctx->buffer;

	    CHPRIME

#if PAT_DBG_TRACE>1
	    if (sea_ctx->trace)
	    {
		if (!REG_CH)
		    /* We're about to do this anyway but need it for trace */
		    REG_CH = sea_ctx->buffer;
		TRdisplay("%d|%d %s %d ch[%.#s]\n",
			ctx->idp, ctx->id,
			ctx->state == PAT_NOP ? "START":"RESUME",
			ctx->state,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
	    /*
	    ** True beginings will have current state set to NOP
	    ** other states will reflect a restart from stall and will
	    ** be the true restart state or a child from CASE.
	    */
	    if (ctx->state == PAT_NOP)
		/* Set current char pointer to start of data buffer */
		REG_CH = sea_ctx->buffer;
	    else if (ctx->state != PAT_CASE)
		goto _restart_from_stall;


/*
** We are about to include the file that has the state machine in
** it generated from the dotty file. It defines 2 entry points
** and jumps out to one of four exits.
** The first entry is the 'GetPatOp' entry at the
** true start. The second is the _restart_from_stall
** entry for doing just that.
** The various exit points relate to the state of the operation.
**
** To allow for the same state table to be used for each shell, reference
** will be made to the macros below. These allow the datatypes specifics
** to be isolated.
**
** The collation character state handling is awkward as it does not sit
** ideally with the processing of NEXTCH and COMPARECH. For this reason,
** the adultrans routine has been flattened out and will be inlined in a
** modified form in NEXTCH. The awkward aspect is that the collation can
** span multiple characters but we need to handle segment overlaps... See
** CUTSTALL for how this is done.
*/
#define DIFFPAT(lbl) if (COMPARECH(adultrans(&patch))) goto lbl;

#define NEXTCH \
{\
    register u_char *s = adul_nextch;\
    register i2     in_holdstate = adul_holdstate;\
    if (s && (adul_val = adul_holdstate) == -1)\
    {\
	REG_CH = s;\
	if (s < (u_char*)REG_bufend)\
	{\
	    adul_val = adul_coltbl->firstchar[*s++];\
	    adul_nextch = s;\
	}\
	else\
	    adul_nextch = NULL;\
    }\
    else\
	adul_holdstate = -1;\
    while(adul_nextch)\
    {\
	while (adul_val & ADUL_TMULT)\
	{\
	    register struct ADUL_tstate	*sp = adul_coltbl->statetab + (adul_val & ADUL_TDATA);\
	    if (sp->flags & ADUL_LMULT)\
	    {\
		adul_val = sp->match;\
		adul_holdstate = sp->nomatch;\
	    }\
	    else if (*s == sp->testchr)\
	    {\
		adul_val = sp->match;\
		if (s < (u_char*)REG_bufend)\
		{\
		    s++;\
		    if (!(sp->flags & ADUL_FINAL))\
			adul_nextch = s;\
		}\
		else\
		    adul_nextch = NULL;\
	    }\
	    else\
		adul_val = sp->nomatch;\
	}\
	if (adul_val != ADUL_SKVAL)\
        {\
            if(in_holdstate != -1)\
                REG_CH = s;\
	    break;\
        }\
	if (s < (u_char*)REG_bufend)\
	{\
	    adul_val = adul_coltbl->firstchar[*s++];\
	    adul_nextch = s;\
	}\
	else\
	{\
	    REG_CH = s;\
	    adul_nextch = NULL;\
	}\
    }/* adul_val is set */\
    if (!adul_nextch && !ctx->parent->at_eof)\
	    goto _CUTSTALL;\
}
#define LITPRIME adulstrinitp(adul_coltbl, REG_litset, REG_litsetend, &patch);
#define NEXTPAT { adulnext(&patch); REG_litset = adulptr(&patch); }

/* At least the COMPARECH is simple after all that */
#define COMPARECH(b) (last_diff=(adul_val-b))

#if PAT_DBG_TRACE>1
#define DISASS(ctx) \
    if (ctx->parent->trace){\
	char buf[140];\
	u_i1 *tmp = REG_PC;\
	i8 v = 0;\
	char *p = buf;\
	if (REG_litsetend&&REG_litset&&REG_litsetend>REG_litset)\
	    TRdisplay("%d|%d %p N=%d M=%d ch[%.#s]l=%d cf[%.#s]\n",\
		    ctx->idp, ctx->id, REG_PC, REG_N, REG_M,\
		    min(8,REG_bufend-REG_CH), REG_CH,\
		    REG_litsetend-REG_litset,\
		    min(4,REG_litsetend-REG_litset), REG_litset);\
	else\
	    TRdisplay("%d|%d %p N=%d M=%d ch[%.#s]\n",\
		    ctx->idp, ctx->id, REG_PC, REG_N, REG_M,\
		    min(8,REG_bufend-REG_CH), REG_CH);\
	while (adu_pat_disass(buf, sizeof(buf), &tmp, &p, &v, FALSE)){\
	    *p++ = '\n';\
	    TRwrite(0, p - buf, buf);\
	    p = buf;\
	    *p++ = '\t';\
	    *p++ = '\t';\
	}\
	*p = '\n';\
	TRwrite(0, p - buf, buf);\
    }
#else
#define DISASS(ctx)
#endif

/*
** The adul collation code complicates things a little more requiring
** the set test loops to be more aware of the adul handling :-(
*/
#define CHINSET(lbl) \
    adulstrinitp(adul_coltbl, REG_litset, REG_litsetend, &patch);\
    if (adulptr(&patch) >= (CHAR*)REG_litsetend) goto lbl;\
    for (; ; adulnext(&patch)){\
	patl = adultrans(&patch);\
	if (patl/COL_MULTI == AD_5LIKE_RANGE){\
	    adulnext(&patch);\
	    if(COMPARECH(adultrans(&patch))<0){\
		adulnext(&patch);\
	    }else{\
		adulnext(&patch);\
		if(COMPARECH(adultrans(&patch))<=0) break;\
	    }\
	}else if (!COMPARECH(patl)) break;\
	if (last_diff < 0 || adulptr(&patch) >= (CHAR*)REG_litsetend) goto lbl;\
    }
#define CHNINSET(lbl) \
    adulstrinitn(adul_coltbl, REG_litset, REG_O, &patch);\
    for (; adulptr(&patch) < (CHAR*)REG_litset+REG_O; adulnext(&patch)){\
	patl = adultrans(&patch);\
	if (patl/COL_MULTI == AD_5LIKE_RANGE){\
	    adulnext(&patch);\
	    if(COMPARECH(adultrans(&patch))<0){\
		adulnext(&patch);\
	    }else{\
		adulnext(&patch);\
		if(COMPARECH(adultrans(&patch))<=0) break;\
	    }\
	}else if (!COMPARECH(patl)) break;\
	if (last_diff < 0 || adulptr(&patch) >= (CHAR*)REG_litset+REG_O) goto lbl;\
    }\
    adulstrinitp(adul_coltbl, REG_litset+REG_O, REG_litsetend, &patch);\
    for (; adulptr(&patch) < (CHAR*)REG_litsetend; adulnext(&patch)){\
	patl = adultrans(&patch);\
	if (patl/COL_MULTI == AD_5LIKE_RANGE){\
	    adulnext(&patch);\
	    if(COMPARECH(adultrans(&patch))<0){\
		adulnext(&patch);\
	    }else{\
		adulnext(&patch);\
		if(COMPARECH(adultrans(&patch))<=0) goto lbl;\
	    }\
	}else if (!COMPARECH(patl)) goto lbl;\
	if (last_diff < 0) break;\
    }

#include "adupatexec_inc.i"

#undef CHNINSET
#undef CHINSET

#undef DISASS
#undef COMPARECH

#undef NEXTPAT
#undef NEXTCH
#undef DIFFPAT
#undef LITPRIME
#undef CHPRIME
#undef CHSAVE
#undef CHRESTORE

_SUCCESS:
#if PAT_DBG_TRACE>1
/*SUCCESS*/ if (sea_ctx->trace)
	    {
		TRdisplay("%d|%d SUCCESS ch[%.#s]\n",
			ctx->idp, ctx->id,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
/*SUCCESS*/ *rcmp = 0;
	    return db_stat;

_FAILm1:    last_diff = -1;
	    goto _FAIL;

_FAIL1:	    last_diff = 1;
_FAIL:
#if PAT_DBG_TRACE>1
/*FAIL*/    if (sea_ctx->trace)
	    {
		TRdisplay("%d|%d FAILED %d ch[%.#s]\n",
			ctx->idp, ctx->id, last_diff,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
/*FAIL*/    ctx->next = sea_ctx->free;
	    sea_ctx->free = ctx;
	    continue;

	_CUTSTALL:
	    /* We got here because adultrans needed more characters
	    ** than we have in one buffer. Pull back bufend to the
	    ** start of the incomplete character - patda will note
	    ** the extra data between bufend and bufrealend and will
	    ** fix up accordingly. */
	    sea_ctx->bufend = REG_CH;
	_STALL:
/*STALL*/   REG_STORE(ctx)
#if PAT_DBG_TRACE>1
/*STALL*/   if (sea_ctx->trace)
	    {
		TRdisplay("%d|%d STALLED ch[%.#s]\n",
			ctx->idp, ctx->id,
			min(8,sea_ctx->bufend-REG_CH), REG_CH);
	    }
#endif
	    /*
	    ** Reset current char pointer to start of data buffer
	    ** which will be loaded with new data shortly
	    */
	    ctx->ch = sea_ctx->buffer;
	    /* Add to end of stalled list */
	    for (p = &sea_ctx->stalled; *p; p = &(*p)->next)
		/*SKIP*/;
	    ctx->next = 0;
	    *p = ctx;
	    continue;
	}
    }
    *rcmp = last_diff;
    return db_stat;
}


/*
** Name: adu_pat_execute_uni() - Unicode pattern state machine
**
**      This routine compares a data stream or string defined by da_ctx
**	with one or more patterns in sea_ctx.
**
**	This varient of the engine handles unicode data typess. For the
**	non-unicode equivalent, see the sister routine adu_pat_execute()
**
** Inputs:
**	sea_ctx		This structure contains the execution contects and
**			state for the pattern(s). This routine will manage
**			these as its scans through the data from da_ctx.
**	da_ctx		This structure defines the input data. It could be
**			a simple unicode string or long object, eiter way
**			the data be passed back in a standard manner.
**			If it needed normalising or case folding it will
**			have been done by da_ctx.
**
** Outputs:
**	rcmp				Result of comparison, as follows:
**					    <0  if  str_dv1 < pat_dv2
**					    =0  if  str_dv1 = pat_dv2
**					    >0  if  str_dv1 > pat_dv2
**
** Returns:
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
DB_STATUS
adu_pat_execute_uni(
	AD_PAT_SEA_CTX *sea_ctx,
	AD_PAT_DA_CTX *da_ctx,
	i4 *rcmp)
{
    typedef UCS2 CHAR;
    DB_STATUS db_stat = E_DB_OK;
    i8 tmp;
    AD_PAT_CTX *ctx, **p;
    REG_DEFS
    register CHAR *t;
    register i4 last_diff = 1;
    u_i4 ch;

    while (sea_ctx->pending = sea_ctx->stalled)
    {
	sea_ctx->stalled = 0;
	/* Get next/only segment for all stalled */
	if (db_stat = adu_patda_get(da_ctx))
	    break;

	while (ctx = sea_ctx->pending){
	    /* Unlink from pending list */
	    sea_ctx->pending = ctx->next;
	    REG_LOAD(ctx)
#if PAT_DBG_TRACE>1
	    if (sea_ctx->trace)
	    {
		UCS2 *x;
		if (!REG_CH)
		    /* We're about to do this anyway but need it for trace */
		    REG_CH = sea_ctx->buffer;
		x = (UCS2*)REG_CH;
		TRdisplay("%d|%d %s %d ch%d%#.2{ %4.2x%}\n",
			ctx->idp, ctx->id,
			ctx->state == PAT_NOP ? "START":"RESUME",
			ctx->state,
			(sea_ctx->bufend-REG_CH)/6,
			min(21,(sea_ctx->bufend-REG_CH)/2), REG_CH,0);
	    }
#endif
	    /*
	    ** True beginings will have current state set to NOP
	    ** other states will reflect a restart from stall and will
	    ** be the true restart state or a child from CASE.
	    */
	    if (ctx->state == PAT_NOP)
		/* Set current char pointer to start of data buffer */
		REG_CH = sea_ctx->buffer;
	    else if (ctx->state != PAT_CASE)
		goto _restart_from_stall;


/*
** We are about to include the file that has the state machine in
** it generated from the dotty file. It defines 2 entry points
** and jumps out to one of four exits.
** The first entry is the 'GetPatOp' entry at the
** true start. The second is the _restart_from_stall
** entry for doing just that.
** The various exit points relate to the state of the operation.
**
** To allow for the same state table to be used for each shell, reference
** will be made to the macros below. These allow the datatypes specifics
** to be isolated.
*/
#define DIFFPAT(lbl) if (COMPARECH(REG_litset)) goto lbl;
#define NEXTCH ADV(REG_CH);
#define NEXTPAT ADV(REG_litset);

#define COMPARECH(b) \
	((last_diff=(i4)(((UCS2*)REG_CH)[0])-(i4)(((UCS2*)b)[0]))?last_diff\
	:(last_diff=(i4)(((UCS2*)REG_CH)[1])-(i4)(((UCS2*)b)[1]))?last_diff\
	:(last_diff=(i4)(((UCS2*)REG_CH)[2])-(i4)(((UCS2*)b)[2])))
#define ADV(ptr) (ptr) += (MAX_CE_LEVELS-1)*sizeof(UCS2)/sizeof(*ptr)
#define HAVERANGE(ptr) (*(ptr)==U_RNG?ADV(ptr):0)

#if PAT_DBG_TRACE>1
#define DISASS(ctx) \
    if (ctx->parent->trace){\
	char buf[140];\
	u_i1 *tmp = REG_PC;\
	i8 v = 0;\
	char *p = buf;\
	UCS2 *x = (UCS2*)REG_CH;\
	if (REG_litsetend&&REG_litset&&REG_litsetend>REG_litset)\
	    TRdisplay("%d|%d %p N=%d M=%d ch%d%#.2{ %4.2x%} cf%d%#.2{ %4.2x%}\n",\
		    ctx->idp, ctx->id, REG_PC, REG_N, REG_M,\
		    (REG_bufend-REG_CH)/6,\
		    min(21,(REG_bufend-REG_CH)/2), REG_CH,0,\
		    (REG_litsetend-REG_litset)/6,\
		    min(21,(REG_litsetend-REG_litset)/2), REG_litset,0);\
	else\
	    TRdisplay("%d|%d %p N=%d M=%d ch%d%#.2{ %4.2x%}\n",\
		    ctx->idp, ctx->id, REG_PC, REG_N, REG_M,\
		    (REG_bufend-REG_CH)/6,\
		    min(21,(REG_bufend-REG_CH)/2), REG_CH,0);\
	while (adu_pat_disass(buf, sizeof(buf), &tmp, &p, &v, TRUE)){\
	    *p++ = '\n';\
	    TRwrite(0, p - buf, buf);\
	    p = buf;\
	    *p++ = '\t';\
	    *p++ = '\t';\
	}\
	*p = '\n';\
	TRwrite(0, p - buf, buf);\
    }
#else
#define DISASS(ctx)
#endif

#include "adupatexec_inc.i"

#undef DISASS
#undef HAVERANGE
#undef ADV
#undef COMPARECH

#undef NEXTPAT
#undef NEXTCH
#undef DIFFPAT

_SUCCESS:
#if PAT_DBG_TRACE>1
/*SUCCESS*/ if (sea_ctx->trace)
	    {
		UCS2 *x = (UCS2*)REG_CH;
		TRdisplay("%d|%d SUCCESS - ch%d%#.2{ %4.2x%}\n",
			ctx->idp, ctx->id,
			(sea_ctx->bufend-REG_CH)/6,
			min(21,(sea_ctx->bufend-REG_CH)/2), REG_CH,0);
	    }
#endif
/*SUCCESS*/ *rcmp = 0;
	    return db_stat;

_FAILm1:    last_diff = -1;
	    goto _FAIL;

_FAIL1:	    last_diff = 1;

_FAIL:
#if PAT_DBG_TRACE>1
/*FAIL*/    if (sea_ctx->trace)
	    {
		UCS2 *x = (UCS2*)REG_CH;
		TRdisplay("%d|%d FAILED %d ch%d%#.2{ %4.2x%}\n",
			ctx->idp, ctx->id, last_diff,
			(sea_ctx->bufend-REG_CH)/6,
			min(21,(sea_ctx->bufend-REG_CH)/2), REG_CH,0);
	    }
#endif
/*FAIL*/    ctx->next = sea_ctx->free;
	    sea_ctx->free = ctx;
	    continue;

	_STALL:
/*STALL*/   REG_STORE(ctx)
#if PAT_DBG_TRACE>1
/*STALL*/   if (sea_ctx->trace)
	    {
		UCS2 *x = (UCS2*)REG_CH;
		TRdisplay("%d|%d STALLED - ch%d%#.2{ %4.2x%}\n",
			ctx->idp, ctx->id,
			(sea_ctx->bufend-REG_CH)/6,
			min(21,(sea_ctx->bufend-REG_CH)/2), REG_CH,0);
	    }
#endif
	    /*
	    ** Reset current char pointer to start of data buffer
	    ** which will be loaded with new data shortly
	    */
	    ctx->ch = sea_ctx->buffer;
	    /* Add to end of stalled list */
	    for (p = &sea_ctx->stalled; *p; p = &(*p)->next)
		/*SKIP*/;
	    ctx->next = 0;
	    *p = ctx;
	    continue;
	}
    }
    *rcmp = last_diff;
    return db_stat;
}
