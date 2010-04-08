/*
** Copyright (c) 1985, 2004 Ingres Corporation
*/
#include <math.h>
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
**	17-apr-89 (jrb)
**	    Added FUNC_EXTERNs for new MH routines for DECIMAL.  Also added
**	    MH_PK* sign defines.
**	08-jul-89 (stec)
**	    Added defines MH_FLOATPREC and MH_DOUBLEPREC defining max precision
**	    for a float and a double respectively.
**      09-mar-1993 (stevet)
**          Added defines for MHi4add, MHi4sub, MHi4mul, MHi4div.
**      15-Jan-1999 (shero03)
**          Add maximum random2 integer
**      25-jan-1994 (stevet)
**          Added defines for MHdfine and MHffinte.  
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	18-sep-2002 (abbjo03)
**	    Add MHlog10().
**      10-oct-2002 (zhahu02)
**          Updated for composition of two i4 integers.
**          They are now defined in mhint.c (b108914).
**	18-mar-04 (inkdo01)
**	    Add MHi8add, MHi8sub, MHi8mul, MHi8div for bigint support.
**	22-jun-2004 (srisu02)
**	    Add MHisnan().
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN f8      MHatan();
FUNC_EXTERN f8      MHatan2();
FUNC_EXTERN f8      MHtan();
FUNC_EXTERN f8      MHacos();
FUNC_EXTERN f8      MHcos();
FUNC_EXTERN f8      MHexp();
FUNC_EXTERN f8      MHipow();
FUNC_EXTERN f8      MHexp();
FUNC_EXTERN f8      MHln();
FUNC_EXTERN f8      MHpow();
FUNC_EXTERN STATUS  MHsrand();
FUNC_EXTERN f8      MHrand();
FUNC_EXTERN STATUS  MHsrand2();
FUNC_EXTERN u_i4    MHrand2();
FUNC_EXTERN f8      MHasin();
FUNC_EXTERN f8      MHsin();
FUNC_EXTERN f8      MHsqrt();
FUNC_EXTERN f8      MHexp10();
FUNC_EXTERN f8      MHfdint();
FUNC_EXTERN f8      MHceil();
FUNC_EXTERN VOID    MHpkint();
FUNC_EXTERN VOID    MHpkceil();
FUNC_EXTERN VOID    MHpkabs();
FUNC_EXTERN VOID    MHpkneg();
FUNC_EXTERN VOID    MHpkadd();
FUNC_EXTERN VOID    MHpksub();
FUNC_EXTERN VOID    MHpkmul();
FUNC_EXTERN VOID    MHpkdiv();
FUNC_EXTERN i4      MHpkcmp();
FUNC_EXTERN i8	    MHi8add(i8 dv1, i8 dv2);
FUNC_EXTERN i8	    MHi8sub(i8 dv1, i8 dv2);
FUNC_EXTERN i8	    MHi8mul(i8 dv1, i8 dv2);
FUNC_EXTERN i8	    MHi8div(i8 dv1, i8 dv2);


/* 
** Defined Constants.
*/

/* MH return status codes. */

# define                MH_OK           0
# define		MH_BADARG	(E_CL_MASK + E_MH_MASK + 0x01)
	/* DANGER the above error is hard coded in EX.mar */
# define		MH_PRECISION	(E_CL_MASK + E_MH_MASK + 0x02)
# define		MH_INTERNERR	(E_CL_MASK + E_MH_MASK + 0x03)
# define		MH_MATHERR	(E_CL_MASK + E_MH_MASK + 0x04)

/*
**  MH_FLOATPREC, MH_DOUBLEPREC - determines the maximum number of
**  significant decimal digits, which a float (or double) can hold.
**
**  08-Jul-89 (stec)
**	Added.
**  15-Jan-1999 (shero03)
**      Add maximum random2 integer
*/
#define			MH_FLOATPREC	7
#define			MH_DOUBLEPREC	16
#define                 MH_MAXRAND2INT  16777215

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

#define MHlog10(x)	((f8)log10((double)(x)))

/* 
** VMS already report math exceptions so just set MHdfinite and MHffinte 
** to TRUE is sufficient
*/
# define                MHdfinite(x)	(TRUE)
# define		MHffinite(x)	(TRUE)

#if defined(NT_GENERIC)
#define MHisnan(x) (_isnan ((double)x) )
#else
#define MHisnan(x) (isnan((double)x) )
#endif
