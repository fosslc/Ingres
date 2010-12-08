/*
** Copyright 2010 Ingres Corp
*/

#ifndef EXINTERNAL_H_INCLUDED
#define EXINTERNAL_H_INCLUDED

#include <tr.h>		/* Probably already included, but make sure */
#include <clsigs.h>

/* Name: EXINTERNAL.H - CL level definitions for EX
**
** Description:
**
**	It turns out that EX is not well isolated from the rest of the
**	CL, or at least not from CS.  Rather than expose all sorts of
**	horrid internal stuff to the whole world, we'll put the
**	EX local definitions here.  "Local" in this case means
**	either local to EX privately, or local to the CL.
**	There's not enough of this stuff to warrant a separate split
**	into CL things and EX-private (platform dependent) things.
**
**	These definitions used to live in platform dependent exi.h headers.
**
** History:
**	17-Nov-2010 (kschendel) SIR 124685
**	    Invent for prototype fixups throughout CS and EX.
*/

/* Global EX data for CL use */

/* Stack dumper routine, defined in CS, called from EX */
struct _CS_SCB;
GLOBALREF void (*Ex_print_stack)(struct _CS_SCB *, void *, PTR, TR_OUTPUT_FCN *, i4);


/* It turns out that was the only thing needed for Windows EX.
** All the rest is UNIX / VMS.
*/

#ifndef NT_GENERIC
/* Here to end ... */

/* General definitions */

# define	EXMAXARG	30

/* EX global functions */

FUNC_EXTERN TYPESIG (*EXsetsig(int, TYPESIG(*)()))();

FUNC_EXTERN void i_EXsetothersig(int, TYPESIG(*)());
FUNC_EXTERN void i_EXrestoreorigsig(int);

FUNC_EXTERN TYPESIG i_EXcatch(int, EX_SIGCODE SIGCODE(code), EX_SIGCONTEXT SIGCONTEXT(ctx));
FUNC_EXTERN i4 i_EXestablish(void);
FUNC_EXTERN void i_EX_initfunc(void (*)(EX_CONTEXT *), EX_CONTEXT ** (*)(void));
FUNC_EXTERN EX_CONTEXT	*i_EXnext(EX_CONTEXT *);
FUNC_EXTERN void	i_EXpop(EX_CONTEXT **);
FUNC_EXTERN void	i_EXpush(EX_CONTEXT *);
FUNC_EXTERN EX_CONTEXT	**i_EXtop(void);

#ifdef UNIX
FUNC_EXTERN STATUS EXaltstack( PTR ex_stack, i4  s_size);
FUNC_EXTERN void EXdump(u_i4, u_i4 *);

#else
/* VMS */
FUNC_EXTERN void EXdump(u_i4);

#endif

/* Symbol stuff, unix only */
#if defined(i64_hpu)
FUNC_EXTERN uint64_t DIAGSymbolLookupName(char *);
FUNC_EXTERN PTR DIAGSymbolLookupAddr(uint64_t, uint64_t *);
#elif defined(UNIX)
FUNC_EXTERN i4 DIAGSymbolLookupName(char *);
FUNC_EXTERN PTR DIAGSymbolLookupAddr(i4, i4 *);
#endif

#if defined(UNIX)
FUNC_EXTERN i4 DIAGSymbolLookup( void *pc,
	unsigned long *addr, unsigned long *offset,
	PTR buffer, i4 size );

/* I don't want to figure out si.h at the moment, use void * for FILE *.
** Can be fixed later, maybe.
*/
FUNC_EXTERN void DIAGSymbolRead(/*FILE*/ void *, void *, i4);
struct _LOCATION;
FUNC_EXTERN i4 DIAGObjectRead(struct _LOCATION *, i4);
#endif

#endif /* NT_GENERIC from above */

#endif /* EXINTERNAL_H_INCLUDED */
/*
** Copyright 2010 Ingres Corp
*/

#ifndef EXINTERNAL_H_INCLUDED
#define EXINTERNAL_H_INCLUDED

#include <tr.h>		/* Probably already included, but make sure */
#include <clsigs.h>

/* Name: EXINTERNAL.H - CL level definitions for EX
**
** Description:
**
**	It turns out that EX is not well isolated from the rest of the
**	CL, or at least not from CS.  Rather than expose all sorts of
**	horrid internal stuff to the whole world, we'll put the
**	EX local definitions here.  "Local" in this case means
**	either local to EX privately, or local to the CL.
**	There's not enough of this stuff to warrant a separate split
**	into CL things and EX-private (platform dependent) things.
**
**	These definitions used to live in platform dependent exi.h headers.
**
** History:
**	17-Nov-2010 (kschendel) SIR 124685
**	    Invent for prototype fixups throughout CS and EX.
*/

/* Global EX data for CL use */

/* Stack dumper routine, defined in CS, called from EX */
struct _CS_SCB;
GLOBALREF void (*Ex_print_stack)(struct _CS_SCB *, void *, PTR, TR_OUTPUT_FCN *, i4);


/* It turns out that was the only thing needed for Windows EX.
** All the rest is UNIX / VMS.
*/

#ifndef NT_GENERIC
/* Here to end ... */

/* General definitions */

# define	EXMAXARG	30

/* EX global functions */

FUNC_EXTERN TYPESIG (*EXsetsig(int, TYPESIG(*)()))();

FUNC_EXTERN void i_EXsetothersig(int, TYPESIG(*)());
FUNC_EXTERN void i_EXrestoreorigsig(int);

FUNC_EXTERN TYPESIG i_EXcatch(int, EX_SIGCODE SIGCODE(code), EX_SIGCONTEXT SIGCONTEXT(ctx));
FUNC_EXTERN i4 i_EXestablish(void);
FUNC_EXTERN void i_EX_initfunc(void (*)(EX_CONTEXT *), EX_CONTEXT ** (*)(void));
FUNC_EXTERN EX_CONTEXT	*i_EXnext(EX_CONTEXT *);
FUNC_EXTERN void	i_EXpop(EX_CONTEXT **);
FUNC_EXTERN void	i_EXpush(EX_CONTEXT *);
FUNC_EXTERN EX_CONTEXT	**i_EXtop(void);

#ifdef UNIX
FUNC_EXTERN STATUS EXaltstack( PTR ex_stack, i4  s_size);
FUNC_EXTERN void EXdump(u_i4, u_i4 *);

#else
/* VMS */
FUNC_EXTERN void EXdump(u_i4);

#endif

/* Symbol stuff, unix only */
#if defined(i64_hpu)
FUNC_EXTERN uint64_t DIAGSymbolLookupName(char *);
FUNC_EXTERN PTR DIAGSymbolLookupAddr(uint64_t, uint64_t *);
#elif defined(UNIX)
FUNC_EXTERN i4 DIAGSymbolLookupName(char *);
FUNC_EXTERN PTR DIAGSymbolLookupAddr(i4, i4 *);
#endif

#if defined(UNIX)
FUNC_EXTERN i4 DIAGSymbolLookup( void *pc,
	unsigned long *addr, unsigned long *offset,
	PTR buffer, i4 size );

/* I don't want to figure out si.h at the moment, use void * for FILE *.
** Can be fixed later, maybe.
*/
FUNC_EXTERN void DIAGSymbolRead(/*FILE*/ void *, void *, i4);
struct _LOCATION;
FUNC_EXTERN i4 DIAGObjectRead(struct _LOCATION *, i4);
#endif

#endif /* NT_GENERIC from above */

#endif /* EXINTERNAL_H_INCLUDED */
