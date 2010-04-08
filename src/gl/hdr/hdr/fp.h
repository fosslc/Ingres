/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef FP_HDR_INCLUDED
# define FP_HDR_INCLUDED 1

#include <fpcl.h>

/*
** Name: FP.H - Global definitions for the FP compatibility library.
**
** Description:
**      The file contains the definitions of the FP functions.
**	These are used for math functions.
**
** History:
**	14-jul-1993 (rog)
**	    Created.
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of fp.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**  Forward and/or External function references.
*/

#define	FPatan		IIFPatan
FUNC_EXTERN STATUS	FPatan( double *x, double *result );

#define	FPceil		IIFPceil
FUNC_EXTERN STATUS	FPceil( double *x, double *result );

#define	FPcos		IIFPcos
FUNC_EXTERN STATUS	FPcos( double *x, double *result );

#ifndef FPdfinite
#define	FPdfinite	IIFPdfinite
FUNC_EXTERN i4		FPdfinite( double *x );
#endif

#define	FPexp		IIFPexp
FUNC_EXTERN STATUS	FPexp( double *x, double *result );

#ifndef FPdadd
#define	FPdadd		IIFPdadd
FUNC_EXTERN STATUS	FPdadd( double *x, double *y, double *result );
#endif

#ifndef FPfdint
#define	FPfdint		IIFPfdint
FUNC_EXTERN double	FPfdint( double *x );
#endif

#ifndef FPddiv
#define	FPddiv		IIFPddiv
FUNC_EXTERN STATUS	FPddiv( double *x, double *y, double *result );
#endif

#ifndef FPffinite
#define	FPffinite	IIFPffinite
FUNC_EXTERN i4		FPffinite( float *x );
#endif

#ifndef FPdmul
#define	FPdmul		IIFPdmul
FUNC_EXTERN STATUS	FPdmul( double *x, double *y, double *result );
#endif

#ifndef FPdsub
#define	FPdsub		IIFPdsub
FUNC_EXTERN STATUS	FPdsub( double *x, double *y, double *result );
#endif

#define	FPipow		IIFPipow
FUNC_EXTERN STATUS	FPipow( double *x, i4 i, double *result );

#define	FPln		IIFPln
FUNC_EXTERN STATUS	FPln( double *x, double *result );

#define	FPpow		IIFPpow
FUNC_EXTERN STATUS	FPpow( double *x, double *y, double *result );

#define	FPrand		IIFPrand
FUNC_EXTERN double	FPrand( void );

#define	FPsin		IIFPsin
FUNC_EXTERN STATUS	FPsin( double *x, double *result );

#define	FPsqrt		IIFPsqrt
FUNC_EXTERN STATUS	FPsqrt( double *x, double *result );

#define	FPsrand		IIFPsrand
FUNC_EXTERN void	FPsrand( i4 seed );


/* Packed decimal function definitions */
#define	FPpkabs		IIFPpkabs
FUNC_EXTERN void	FPpkabs( PTR pkin, i4  prec, i4  scale, PTR pkout );

#define	FPpkadd		IIFPpkadd
FUNC_EXTERN STATUS	FPpkadd( PTR pk1, i4  p1, i4  s1, PTR pk2, i4  p2,
				 i4  s2, PTR pkout, i4  *pout, i4  *sout );

#define	FPpkceil	IIFPpkceil
FUNC_EXTERN STATUS	FPpkceil( PTR pkin, i4  prec, i4  scale, PTR pkout );

#define	FPpkcmp		IIFPpkcmp
FUNC_EXTERN i4		FPpkcmp( PTR pk1, i4  p1, i4  s1, PTR pk2, i4  p2,
				 i4  s2 );

#define	FPpkdiv		IIFPpkdiv
FUNC_EXTERN STATUS	FPpkdiv( PTR pk1, i4  p1, i4  s1, PTR pk2, i4  p2,
				 i4  s2, PTR pkout, i4  *pout, i4  *sout );

#define	FPpkint		IIFPpkint
FUNC_EXTERN void	FPpkint( PTR pkin, i4  prec, i4  scale, PTR pkout );

#define	FPpkmul		IIFPpkmul
FUNC_EXTERN STATUS	FPpkmul( PTR pk1, i4  p1, i4  s1, PTR pk2, i4  p2,
				 i4  s2, PTR pkout, i4  *pout, i4  *sout );

#define	FPpkneg		IIFPpkneg
FUNC_EXTERN void	FPpkneg( PTR pkin, i4  prec, i4  scale, PTR pkout );

#define	FPpksub		IIFPpksub
FUNC_EXTERN STATUS	FPpksub( PTR pk1, i4  p1, i4  s1, PTR pk2, i4  p2,
				 i4  s2, PTR pkout, i4  *pout, i4  *sout );


/* 
** Defined Constants.
**
*/

/* FP return status codes. */

# define                FP_OK           	OK
# define		FP_NOT_FINITE_ARGUMENT	(E_CL_MASK + E_FP_MASK + 0x01)
# define		FP_OVERFLOW		(E_CL_MASK + E_FP_MASK + 0x02)
# define		FP_UNDERFLOW		(E_CL_MASK + E_FP_MASK + 0x03)
# define		FP_IPOW_DOMAIN_ERROR	(E_CL_MASK + E_FP_MASK + 0x04)
# define		FP_LN_DOMAIN_ERROR	(E_CL_MASK + E_FP_MASK + 0x05)
# define		FP_POW_DOMAIN_ERROR	(E_CL_MASK + E_FP_MASK + 0x06)
# define		FP_SQRT_DOMAIN_ERROR	(E_CL_MASK + E_FP_MASK + 0x07)
# define		FP_DECOVF		(E_CL_MASK + E_FP_MASK + 0x08)
# define		FP_DECDIV		(E_CL_MASK + E_FP_MASK + 0x09)
# define		FP_DIVIDE_BY_ZERO	(E_CL_MASK + E_FP_MASK + 0x0A)

# endif /* ! FP_HDR_INCLUDED */
