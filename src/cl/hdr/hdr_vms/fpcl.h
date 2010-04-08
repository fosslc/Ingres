/*
** Copyright (c) 1993, 2002  Ingres Corporation
*/

#include <math.h>
#include <mthdef.h>

/*
** Name: FP.H - Global definitions for the FP compatibility library.
**
** Description:
**      The file contains the type used by FP and the definition of the
**      FP functions.  These are used for math functions.
**
** History:
**      14-jul-1993 (rog)
**          Created.
**	18-dec-2002 (abbjo03)
**	    Add define for FPfdint.
**	27-dec-2002 (abbjo03)
**	    Add include of math.h for prototypes of floor() and ceil(). Modify
**	    FPfdint macro to ensure proper access to its arguments.
*/


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
** These are constants defined in the VMS manuals as the max and min values
** that the exponential system service routine can handle.
*/
#define			FP_EXP_MAX	     88.029
#define			FP_EXP_MIN	    -88.722

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

STATUS	FPdmath( int opp, double *x, double *y, double *result );

#define FPfdint(x)	((*(x) >= 0.0 ) ? (floor(*(x))) : (ceil(*(x))))
