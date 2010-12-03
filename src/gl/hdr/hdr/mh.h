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
**	10-May-2010 (kschendel) b123712
**	    Add MHtrunc().
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/
FUNC_EXTERN f8      MHround(
         f8	 x,
         i4      y
);

FUNC_EXTERN f8      MHacos(
	f8	    x
);
				
FUNC_EXTERN f8      MHasin(
	f8	    x
);

FUNC_EXTERN f8      MHatan(
	f8	    x
);

FUNC_EXTERN f8      MHatan2(
	f8	    x,
	f8	    y
);

FUNC_EXTERN f8      MHceil(
	f8	    x
);

FUNC_EXTERN f8      MHcos(
	f8	    x
);
				
#ifndef	MHdfinite
FUNC_EXTERN i4      MHdfinite(
	double	    d
);
#endif

FUNC_EXTERN f8      MHexp(
	f8	    x
);

FUNC_EXTERN f8      MHexp10(
	f8	    x
);

FUNC_EXTERN f8      MHfdint(
	f8	    x
);

#ifndef MHffinite
FUNC_EXTERN i4      MHffinite(
	float	    f
);
#endif

FUNC_EXTERN f8      MHipow(
	f8	    x,
	i4	    i
);

FUNC_EXTERN f8      MHln(
	f8	    x
);

#ifndef MHlog10
FUNC_EXTERN f8	    MHlog10(
	f8	    x
);
#endif

FUNC_EXTERN f8      MHpow(
	f8	    x,
	f8	    y
);

FUNC_EXTERN f8      MHrand(
	void
);

FUNC_EXTERN u_i4    MHrand2(
	void
);

FUNC_EXTERN f8      MHsin(
	f8	    x
);

FUNC_EXTERN f8      MHsqrt(
	f8	    x
);

FUNC_EXTERN STATUS  MHsrand(
	i4	    seed
);

FUNC_EXTERN STATUS  MHsrand2(
	i4	    seed
);

FUNC_EXTERN f8      MHtan(
	f8	    x
);

FUNC_EXTERN f8 MHtrunc(f8 x, i4 pos);


/* FIXME system specific ? */

FUNC_EXTERN i4 MHiipow(
	i4	    x, 
	i4	    y
);
#ifdef nc4_us5
#define MHf8eqi4(dptr, sptr) (*((f8 *) dptr) = *((i4 *) sptr))
#define MHi4eqf8(dptr, sptr) (*((i4 *) dptr) = *((f8 *) sptr))
#else
FUNC_EXTERN void    MHf8eqi4(
	f8	    *x,
	i4	    *y
);

FUNC_EXTERN void    MHi4eqf8(
	i4	    *x,
	f8	    *y
);
#endif /* nc4_us5) */

#ifndef MHi4add
FUNC_EXTERN i4      MHi4add(
	i4 dv1, i4 dv2
);
#endif

#ifndef MHi4sub
FUNC_EXTERN i4      MHi4sub(
	i4 dv1, i4 dv2
);
#endif

#ifndef MHi4mul
FUNC_EXTERN i4      MHi4mul(
	i4 dv1, i4 dv2
);
#endif

#ifndef MHi4div
FUNC_EXTERN i4      MHi4div(
	i4 dv1, i4 dv2
);
#endif

#ifndef MHpkint
FUNC_EXTERN VOID      MHpkint(  /* Take the integer part of a packed decimal*/
	PTR pkin, i4 prec, i4 scale, PTR pkout
);
#endif

#ifndef MHpkceil
FUNC_EXTERN VOID      MHpkceil( /* Take the ceiling of a packed decimal */ 
	PTR pkin, i4 prec, i4 scale, PTR pkout
);
#endif

#ifndef MHpkabs
FUNC_EXTERN VOID      MHpkabs( /* Take the absolute value of a packed decimal */
	PTR pkin, i4 prec, i4 scale, PTR pkout
);
#endif

#ifndef MHpkneg
FUNC_EXTERN VOID      MHpkneg( /* Negate a packed decimal */
	PTR pkin, i4 prec, i4 scale, PTR pkout
);
#endif

#ifndef MHpkadd
FUNC_EXTERN VOID      MHpkadd( /* Add two packed decimals */
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
);
#endif

#ifndef MHpksub
FUNC_EXTERN VOID      MHpksub( /* Subtract two packed decimals */
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
);
#endif

#ifndef MHpkmul
FUNC_EXTERN VOID      MHpkmul( /* Multiply two packed decimals */
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
);
#endif

#ifndef MHpkdiv
FUNC_EXTERN VOID      MHpkdiv( /* Divide two packed decimals */
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2,
	PTR pkout, i4 *pout, i4 *sout
);
#endif

#ifndef MHpkcmp
FUNC_EXTERN i4      MHpkcmp( /* Compare two packed decimals */
	PTR pk1, i4 p1, i4 s1, 
	PTR pk2, i4 p2, i4 s2
);
#endif

# endif /* ! MH_HDR_INCLUDED */
