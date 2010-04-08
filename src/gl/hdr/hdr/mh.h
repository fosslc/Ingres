/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef MH_HDR_INCLUDED
# define MH_HDR_INCLUDED 1
#include    <mhcl.h>

/**CL_SPEC
** Name:	MH.h	- Define MH function externs
**
** Specification:
**
** Description:
**	Contains MH function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	26-feb-1996 (thoda04)
**	    added MHpk function prototypes
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of mh.h.
**	15-Jan-1998 (shero03)
**	    add MHrand2 function
**      21-jan-1999 (hanch04)
**          replace i4  and i4 with i4
**      22-jun-1999 (musro02)
**          Add defines of MHf8eqi4 and MHi4eqf8 for nc4_us5
**          There are prototypes for these when HIGHC but they don't exist
**	25-jun-1999 (somsa01)
**	    Added MHsrand2() function prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-sep-2002 (abbjo03)
**	    Add MHlog10().
**       04-Nov-2009 (coomi01) b122767
**          Add MHround()
**/
FUNC_EXTERN f8      MHround(
#ifdef CL_PROTOTYPED
         f8	 x,
         i4      y
#endif
);

FUNC_EXTERN f8      MHacos(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);
				
FUNC_EXTERN f8      MHasin(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN f8      MHatan(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN f8      MHatan2(
#ifdef CL_PROTOTYPED
	f8	    x,
	f8	    y
#endif
);

FUNC_EXTERN f8      MHceil(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN f8      MHcos(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);
				
#ifndef	MHdfinite
FUNC_EXTERN i4      MHdfinite(
#ifdef CL_PROTOTYPED
	double	    d
#endif
);
#endif

FUNC_EXTERN f8      MHexp(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN f8      MHexp10(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN f8      MHfdint(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

#ifndef MHffinite
FUNC_EXTERN i4      MHffinite(
#ifdef CL_PROTOTYPED
	float	    f
#endif
);
#endif

FUNC_EXTERN f8      MHipow(
#ifdef CL_PROTOTYPED
	f8	    x,
	i4	    i
#endif
);

FUNC_EXTERN f8      MHln(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

#ifndef MHlog10
FUNC_EXTERN f8	    MHlog10(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);
#endif

FUNC_EXTERN f8      MHpow(
#ifdef CL_PROTOTYPED
	f8	    x,
	f8	    y
#endif
);

FUNC_EXTERN f8      MHrand(
#ifdef CL_PROTOTYPED
	void
#endif
);

FUNC_EXTERN u_i4    MHrand2(
#ifdef CL_PROTOTYPED
	void
#endif
);

FUNC_EXTERN f8      MHsin(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN f8      MHsqrt(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);

FUNC_EXTERN STATUS  MHsrand(
#ifdef CL_PROTOTYPED
	i4	    seed
#endif
);

FUNC_EXTERN STATUS  MHsrand2(
#ifdef CL_PROTOTYPED
	i4	    seed
#endif
);

FUNC_EXTERN f8      MHtan(
#ifdef CL_PROTOTYPED
	f8	    x
#endif
);


/* FIXME system specific ? */

FUNC_EXTERN i4 MHiipow(
#ifdef CL_PROTOTYPED
	i4	    x, 
	i4	    y
#endif
);
#ifdef nc4_us5
#define MHf8eqi4(dptr, sptr) (*((f8 *) dptr) = *((i4 *) sptr))
#define MHi4eqf8(dptr, sptr) (*((i4 *) dptr) = *((f8 *) sptr))
#else
FUNC_EXTERN void    MHf8eqi4(
#ifdef CL_PROTOTYPED
	f8	    *x,
	i4	    *y
#endif
);

FUNC_EXTERN void    MHi4eqf8(
#ifdef CL_PROTOTYPED
	i4	    *x,
	f8	    *y
#endif
);
#endif /* nc4_us5) */

#ifndef MHi4add
FUNC_EXTERN i4      MHi4add(
#ifdef CL_PROTOTYPED
	i4 dv1, i4 dv2
#endif
);
#endif

#ifndef MHi4sub
FUNC_EXTERN i4      MHi4sub(
#ifdef CL_PROTOTYPED
	i4 dv1, i4 dv2
#endif
);
#endif

#ifndef MHi4mul
FUNC_EXTERN i4      MHi4mul(
#ifdef CL_PROTOTYPED
	i4 dv1, i4 dv2
#endif
);
#endif

#ifndef MHi4div
FUNC_EXTERN i4      MHi4div(
#ifdef CL_PROTOTYPED
	i4 dv1, i4 dv2
#endif
);
#endif

#ifndef MHpkint
FUNC_EXTERN VOID      MHpkint(  /* Take the integer part of a packed decimal*/
#ifdef CL_PROTOTYPED
	PTR pkin, i4 prec, i4 scale, PTR pkout
#endif
);
#endif

#ifndef MHpkceil
FUNC_EXTERN VOID      MHpkceil( /* Take the ceiling of a packed decimal */ 
#ifdef CL_PROTOTYPED
	PTR pkin, i4 prec, i4 scale, PTR pkout
#endif
);
#endif

#ifndef MHpkabs
FUNC_EXTERN VOID      MHpkabs( /* Take the absolute value of a packed decimal */
#ifdef CL_PROTOTYPED
	PTR pkin, i4 prec, i4 scale, PTR pkout
#endif
);
#endif

#ifndef MHpkneg
FUNC_EXTERN VOID      MHpkneg( /* Negate a packed decimal */
#ifdef CL_PROTOTYPED
	PTR pkin, i4 prec, i4 scale, PTR pkout
#endif
);
#endif

#ifndef MHpkadd
FUNC_EXTERN VOID      MHpkadd( /* Add two packed decimals */
#ifdef CL_PROTOTYPED
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
#endif
);
#endif

#ifndef MHpksub
FUNC_EXTERN VOID      MHpksub( /* Subtract two packed decimals */
#ifdef CL_PROTOTYPED
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
#endif
);
#endif

#ifndef MHpkmul
FUNC_EXTERN VOID      MHpkmul( /* Multiply two packed decimals */
#ifdef CL_PROTOTYPED
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
#endif
);
#endif

#ifndef MHpkdiv
FUNC_EXTERN VOID      MHpkdiv( /* Divide two packed decimals */
#ifdef CL_PROTOTYPED
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
#endif
);
#endif

#ifndef MHpkcmp
FUNC_EXTERN i4      MHpkcmp( /* Compare two packed decimals */
#ifdef CL_PROTOTYPED
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2
#endif
);
#endif

# endif /* ! MH_HDR_INCLUDED */
