/*
** Copyright (c) 2004 Ingres Corporation
*/

# if defined(any_aix) || defined(axp_osf)
# undef BITSPERBYTE
# endif

# include <systypes.h>
# ifndef DESKTOP
# if defined(OSX)
# include <float.h>
# ifndef MINDOUBLE
# define MINDOUBLE DBL_MIN
# endif
# ifndef MAXDOUBLE
# define MAXDOUBLE DBL_MAX
# endif
# ifndef MINFLOAT
# define MINFLOAT FLT_MIN
# endif
# ifndef MAXFLOAT
# define MAXFLOAT FLT_MAX
# endif
# else
# include <values.h>    /* for MAXDOUBLE */
# endif /* Mac OSX */
# endif /* DESKTOP */
# include <math.h>              /* for isnan()   */

/**
** Name: MH.H - Global definitions for the MH compatibility library.
**
** Description:
**      The file contains the type used by MH and the definition of the
**      MH functions.  These are used for math functions.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**	17-apr-89 (jrb)
**	    Added FUNC_EXTERNs for new MH routines for DECIMAL.
**	    Also added MH_PK* sign defines.
**	08-jul-89 (stec)
**	    Added defines MH_FLOATPREC and MH_DOUBLEPREC defining max precision
**	    for a float and a double respectively.
**	04-nov-92 (peterw)
**	    Integrated sweeney and smc's changes from ingres63p to define 
**	    MH[df]finite for su4_u42, apl_us5 and dra_us5.
**      09-mar-1993 (stevet)
**          Added MHi4add, MHi4sub, MHi4mul and MHi4div.
**	26-apr-1993 (fredv)
**	    Include systypes.h to avoid redefine error on RS6000.
**	    Undefine BITSPERBYTE for RS6000 to avoid redefine warning.
**	19-mar-93 (sweeney)
**	    Add case for usl_us5 (USL Destiny).
**	24-mar-93 (swm)
**	    Include <systypes.h> before <math.h> on axp_osf because
**	    otherwise widespread inclusion of <sys/types.h> without/before
**	    <systypes.h> occurs causing compilation problems with type
**	    definitions - already done by fredv.
**	    Also, undefine BITSPERBYTE for axp_osf as per RS6000 to avoid
**	    several re-define warnings.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	9-aug-93 (robf)
**          Add su4_cmw
**	04-jan-1994 (rog)
**	    Added MH[df]finite() definitions for Solaris, HP, and RS/6000.
**	20-apr-1994 (mikem)
**	    Fixed comment within comment problem shown up by lint.
**      10-feb-1995 (chech02)
**          Added rs4_us5 for AIX 4.1.
**	19-jun-1995 (hanch04)
**	    fix comment to be ansi-c compat, not ansi c++ 
**	15-Jan-1999 (shero03)
**	    Add maximum random integer
**      21-jan-1999 (hanch04)
**          replace i4  and i4 with i4
**	23-Mar-1999 (kosma01)
**	    On sgi_us5, Chose finite(x) for MHdfinite(x) and MHffinite(x).
**	10-may-1999 (walro03)
**	    Remove obsolete version string apl_us5, dra_us5.
**	17-May-1999 (hweho01)
**          Added changes for ris_u64: 
**          1) use finite(x) for MHdfinite(x) and MHffinite(x).
**          2) undefine BITSPERBYTE to avoid warning. 
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	18-sep-2002 (abbjo03)
**	    Add MHlog10().
**	18-mar-04 (inkdo01)
**	    Add MHi8add, MHi8sub, MHi8mul, MHi8div for bigint support.
**	22-jun-2004 (srisu02)
**	    Add MHisnan().
**	27-aug-2004 (hayke02)
**	    Add int_lnx to list of platforms that use finite(x) for
**	    MH[df]finite(x). This change fixes problem INGSRV 2952, bug
**	    112906.
**	16-Mar-2005 (bonro01)
**          For a64_sol use finite(x) for MHdfinite(x) and MHffinite(x).
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	    Replace multi-platform #ifdef wih xCL_FINITE_EXISTS.
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	14-Feb-2008 (hanje04)
**	    SIR S119978
**	    Only define MAXFLOAT etc on OSX if it's not already defined.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	11-Mar-2010 (kschendel) SIR 123275
**	    Expose size of power-of-10 table here.
**/
/*
** MH[df]finite() should map to some OS function(s), e.g., on Suns
** the finite() call will work; whereas on HP 9000/800, you might
** have to do something like !(isinf(x) || isnan(x)).  If your
** machine doesn't support finite(), then MH[df]finite() should
** always return non-zero.  (cf. the CL spec)
** If your machine doesn't fit one of the existing cases *and*
** there's a way for you to implement MH[df]finite(), then add
** an 'if defined' for your box and defined MH_got_finite so that
** you don't get the default.
*/
#ifdef xCL_FINITE_EXISTS
# define        MHdfinite(x)    (finite(x))
# define        MHffinite(x)    (finite((double) x))
# define        MH_got_finite
#endif

#if defined(any_hpux) || defined(hp3_us5)
# define        MHdfinite(x)    (!(isinf((double) x) || isnan((double) x)))
# define        MHffinite(x)    (!(isinf((double) x) || isnan((double) x)))
# define        MH_got_finite
#endif

#if !defined(MH_got_finite)
# define        MHdfinite(x)    (TRUE)
# define        MHffinite(x)    (TRUE)
#endif /* MH[df]finite() */

#if defined(NT_GENERIC)
#define MHisnan(x) (_isnan ((double)x) )
#else
#define MHisnan(x) (isnan((double)x) )
#endif


#define MHlog10(x)	((f8)log10((double)(x)))

/* Size of the powers-of-10 table defined in mhdata.roc */

#define MHMAX_P10TAB	308


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    MHpkint();
FUNC_EXTERN VOID    MHpkceil();
FUNC_EXTERN VOID    MHpkabs();
FUNC_EXTERN VOID    MHpkneg();
FUNC_EXTERN VOID    MHpkadd();
FUNC_EXTERN VOID    MHpksub();
FUNC_EXTERN VOID    MHpkmul();
FUNC_EXTERN VOID    MHpkdiv();
FUNC_EXTERN i4     MHpkcmp();
FUNC_EXTERN i4      MHi4add(i4 dv1, i4 dv2);
FUNC_EXTERN i4      MHi4sub(i4 dv1, i4 dv2);
FUNC_EXTERN i4      MHi4mul(i4 dv1, i4 dv2);
FUNC_EXTERN i4      MHi4div(i4 dv1, i4 dv2);
FUNC_EXTERN i8      MHi8add(i8 dv1, i8 dv2);
FUNC_EXTERN i8      MHi8sub(i8 dv1, i8 dv2);
FUNC_EXTERN i8      MHi8mul(i8 dv1, i8 dv2);
FUNC_EXTERN i8      MHi8div(i8 dv1, i8 dv2);

/* 
** Defined Constants.
**
*/

/* MH return status codes. */

# define                MH_OK           0
# define		MH_BADARG	(E_CL_MASK + E_MH_MASK + 0x01)
	/* DANGER the above error is hard coded in EX.mar */
# define		MH_PRECISION	(E_CL_MASK + E_MH_MASK + 0x02)
# define		MH_INTERNERR	(E_CL_MASK + E_MH_MASK + 0x03)
# define		MH_MATHERR	(E_CL_MASK + E_MH_MASK + 0x04)
# define		MH_LNZERO	(E_CL_MASK + E_MH_MASK + 0x05)
# define		MH_SQRNEG	(E_CL_MASK + E_MH_MASK + 0x06)

# ifndef FMIN
# define FMIN	1E-37
# endif
# ifndef FMAX
# define FMAX	1E37
# endif

/* MH constants for packed decimal routines */

/*       -- Packed Decimal Values --					    */
/* For decimal, 0x0 through 0x9 are valid digits; 0xa, 0xc, 0xe, and 0xf    */
/* are valid plus signs (with 0xc being preferred); and 0xb and 0xd are	    */
/* valid minus signs (with 0xd being preferred).  Defined below are the	    */
/* preferred plus, preferred minus, and the alternate minus values.  No	    */
/* more than these are needed.						    */
# define		MH_PK_PLUS	    ((u_char)0xc)
# define		MH_PK_MINUS	    ((u_char)0xd)
# define		MH_PK_AMINUS	    ((u_char)0xb)

/*
**  MH_FLOATPREC, MH_DOUBLEPREC - determines the maximum number of
**  significant decimal digits, which a float (or double) can hold.
**
**  08-Jul-89 (stec)
**	Added.
**	15-Jan-1999 (shero03)
**	    Add maximum random2 integer
*/
#define			MH_FLOATPREC	7
#define			MH_DOUBLEPREC	13
#define			MH_MAXRAND2INT  16777215
