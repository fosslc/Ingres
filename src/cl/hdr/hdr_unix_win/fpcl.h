/*
** Copyright (c) 2004 Ingres Corporation
*/

# if defined(any_aix) || defined(axp_osf)
# undef BITSPERBYTE
# endif

# ifdef DESKTOP
#define MAXDOUBLE       1.79769313486231570e+308
#define MAXFLOAT        ((float)3.40282346638528860e+38)
#define MINDOUBLE       4.94065645841246544e-324
#define MINFLOAT        ((float)1.40129846432481707e-45)
# define _IEEE          1
# elif defined(OSX) /* Mac OSX doesn't have values.h */
# include <float.h>
# define MINDOUBLE DBL_MIN
# define MAXDOUBLE DBL_MAX
# define MINFLOAT FLT_MIN
# define MAXFLOAT FLT_MAX
# else
# include <values.h>    /* for MAXDOUBLE */
# endif /* DESKTOP */

#include <math.h>
#ifdef NT_GENERIC
#include <float.h>
#endif

/**
** Name: FP.H - Global definitions for the FP compatibility library.
**
** Description:
**      The file contains the type used by FP and the definition of the
**      FP functions.  These are used for math functions.
**
** History:
**	14-jul-1993 (rog)
**	    Created.
**	06-Aug-1993 (fredv)
**	    Defined FPdfinite and FPffinite for ris_us5.
**	9-aug-93 (robf)
**          Add su4_cmw
**	02-nov-1993 (nrb/smc)
**	    Bug #59814
**	    Defined FPdfinite and FPffinite for dr6_us5 & usl_us5.
**      11-feb-94 (ajc)
**          Added hp8_bls specific entries based on hp8*
**      10-feb-95 (chech02)
**          Added us4_rs5 for AIX 4.1.
**      30-Nov-94 (nive/walro03)
**          Defined FPdfinite and FPffinite for dg8_us5.
**	23-mar-95 (smiba01)
**	    Added dr6_ues entries based on dr6_us5
**      01-may-95 (morayf)
**  	    Added odt_us5 floating point finiteness checks using isnan()
**	    as finite() is not available on SCO. isinf() check is not
**	    provided either because the nan and infinite structures are
**	    unioned such that the relevant overflow check is made on the
**	    same address whichever is used. Thus isnan() does both
**	    checks (ugh !).
**	22-may-95 (emmag)
**	    Desktop porting changes.
**	15-jun-95 (popri01)
**	    Added nc4_us5 to FPdfinit and FPffinite defines
**      17-Jul-95 (morayf)
**	    Added new SCO sos_us5 to be like odt_us5 wrt finite checks.
**	25-jul-95 (allmi01)
**	    Added support for dgi_us5 (DG-UX on Intel based platforms) following
**	    dg8_us5).
**	11-oct-95 (tutto01)
**	    Added NT_ALPHA in addition to NT_INTEL.
**	08-nov-1995 (sweeney)
**	    restore changes inadvertently deleted by Ramesh.
**		17-Jul-95 (morayf)
**	  	Added new SCO sos_us5 to be like odt_us5 wrt finite checks.
**		29-jul-95 (pursch)
**		Defined FPdfinite and FPffinite for pym_us5.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with su4_us5.
**              Reason, to port Solaris for Intel.
**	15-dec-95 (morayf)
**	    Added SNI RMx00 (rmx_us5) to those using finite().
**      21-may-97 (musro02)
**          Added sqs_ptx defines of FPdfinite and FPffinite. Use isnan()
**          because neither finite() nor isinf() exist.
**	28-jul-1997 (walro03)
**		Added Tandem NonStop (ts2_us5) to those using finite().
**	26-Nov-1997 (allmi01)
**		Added definitions for Silicon Graphics (sgi_us5) initial port.
**	16-feb-1998 (toumi01)
**		Added Linux (lnx_us5) to those using finite().
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, odt_us5.
**      03-jul-99 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_u
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**	13-Oct-1999 (hweho01)
**		Added ris_u64 to those using finite().
**	14-Jun-2000 (hanje04)
**		Added ibm_lnx to those using finite()
**	07-Sep-2000 (hanje04)
**		Added Alpha Linux (axp_lnx) to those using finite().
**	07-feb-2001 (somsa01)
**		Added porting changes for i64_win.
**	23-jul-2001 (stephenb)
**	 	Add support for i64_aix
**	04-Dec-2001 (hanje04)
**	    Add support for IA64 Linux (i64_lnx)
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	20-Apr-04 (gordy)
**	    Added include of values.h needed for fptest.
**	14-may-2004 (penga03)
**	    Added MAXDOUBLE, MAXFLOAT, MINDOUBLE, MINFLOAT and _IEEE.
**	16-Mar-2005 (bonro01)
**	    Added a64_sol to those using finite()
**	21-Apr-2005 (hanje04)
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	    Use xCL_FINITE_EXISTS instead of multi-platform #ifdef
**      10-Oct-2005 (hweho01)
**          Tru64 (axp_osf) also uses the finite functions that are  
**          ifdefed with xCL_FINITE_EXISTS.
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	20-Jun-2009 (kschendel) SIR 122138
**	    use any-aix symbol.
**/


/* FP constants for packed decimal routines */

/*       -- Packed Decimal Values --					    */
/* For decimal, 0x0 through 0x9 are valid digits; 0xa, 0xc, 0xe, and 0xf    */
/* are valid plus signs (with 0xc being preferred); and 0xb and 0xd are	    */
/* valid minus signs (with 0xd being preferred).  Defined below are the	    */
/* preferred plus, preferred minus, and the alternate minus values.  No	    */
/* more than these are needed.						    */
# define		FP_PK_PLUS	    ((u_char)0xc)
# define		FP_PK_MINUS	    ((u_char)0xd)
# define		FP_PK_AMINUS	    ((u_char)0xb)

/*
**  Macro and defines for basic math functions
*/

#define		FADD 	0
#define 	FSUB 	1
#define 	FMUL 	2
#define 	FDIV 	3

#define 	FPdadd(x, y, r)		FPdmath(FADD, x, y, r)
#define 	FPdsub(x, y, r)		FPdmath(FSUB, x, y, r)
#define 	FPdmul(x, y, r)		FPdmath(FMUL, x, y, r)
#define 	FPddiv(x, y, r)		FPdmath(FDIV, x, y, r)

STATUS	FPdmath( int op, double *x, double *y, double *result );

/*
** FDfdint used to be a function because there was some ifdef'ed
** VAX assembly code that is no longer appropriate.
*/

#define	FPfdint(x)	((*x >= 0.0 ) ? floor(*x) : ceil(*x))

/*
** FP[df]finite() should map to some OS function(s), e.g., on Suns
** the finite() call will work; whereas on HP 9000/800, you might
** have to do something like !(isinf(x) || isnan(x)).  If your
** machine doesn't support finite(), then FP[df]finite() should
** always return non-zero.  (cf. the CL spec)
** If your machine doesn't fit one of the existing cases *and*
** there's a way for you to implement FP[df]finite(), then add
** an 'if defined' for your box and defined FP_got_finite so that
** you don't get the default.
*/
#if defined(xCL_FINITE_EXISTS) || defined(axp_osf)
# define	FPdfinite(x)	(finite(*x))
# define	FPffinite(x)	(finite((double) *x))
# define	FP_got_finite

#elif defined(any_hpux)
# define	FPdfinite(x)	(!(isinf(*x) || isnan(*x)))
# define	FPffinite(x)	(!(isinf((double) *x) || isnan((double) *x)))
# define	FP_got_finite
#endif

#if defined(sos_us5) || defined(sqs_ptx)
# define	FPdfinite(x)	(!isnan(*x))
# define	FPffinite(x)	(!isnan((double) *x))
# define	FP_got_finite
#endif

#if defined(NT_GENERIC)
# define        FPdfinite(x)    (_finite(*x))
# define        FPffinite(x)    (_finite((double) *x))
# define        FP_got_finite
#endif

#if !defined(FP_got_finite)
#    error	You must define FPdfinite and FPffinite; see rog@m for help.
#endif /* FP[df]finite() */
