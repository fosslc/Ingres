/*
**Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cv.h>
# include <cvgcc.h>
  
/* 
NO_OPTIM=rs4_us5 ris_u64 i64_aix a64_lnx
*/

/**
** Name: CVGCC.C - Network conversion support routines
**
** Description:
**
**	CV2n_f4	- convert float to NTS (network transfer syntax)
**	CV2n_f8 - convert double to NTS
**	CV2l_f4 - convert float from NTS to local syntax
**	CV2l_f8 - convert double from NTS
**
**	This file contains support routines for the network conversion
**	macros defined in cvgcc.h.  If a conversion operation is too
**	complex to be coded as a macro, it is coded here as a procedure.
**	The procedure name is the same as the macro name, with the 
**	"_MACRO" stripped.  This currently applies to the macros which
**	support f4 & f8 conversion.
**
**	See cvgcc.h for a description of the macro interface.
**
**	The following is a description of the NTS for f4's and f8's:
**
**		-----------------
**		|     exp(7:0)  |	lowest-address byte
**		| S | exp(14:8)	|
**		|   mant(55:48) |
**		|   mant(63:56) |	highest-address byte
**		|   mant(39:32) |
**		|   mant(47:40) |
**	-------------------------------	(f8 rep. only below this line)
**		|   mant(23:16) |
**		|   mant(31:24) |
**		|   mant(7:0)   |
**		|   mant(15:8)  |
**		-----------------
**
**      where S is the sign bit; if S is set, the represented number
**      is negative.  The f4 representation has 32 bits of mantissa,
**      and is a total of 6 bytes long.  The f8 representation has
**      64 bits of mantissa, and is a total of 10 bytes long.  "Exp"
**      is in excess-16382 (!) representation:  i.e., "exp" values
**      of 1 through 32767 represent true binary exponents of -16381
**      through 16385.  An "exp" value of 0, together with a sign bit
**      of 0, represents 0.  "mant" is a fraction normalized to the
**      range 0.5..1.0, with the redundant most significant bit
**      not represented.
**
**      Note the odd byte ordering of the mantissa:  each 2-byte chunk
**      is in lohi (VAX-like) order, but the 2-byte chunks are in hilo
**      order.  A similar eccentricity is found on the VAX; we
**      emulate it.
**      
** 	17-Sep-85	(Mike Berrow) - Written
** 	13-Aug-86	(Jim Shankland) - Documented network representations;
** 		isolated machine-dependent operations.
**	15-Feb-90 (seiwald)
**	    Reworked to be float support for CV macro interface.
**	    Borrowed relevant comments from cvnet.c.
**	    Truncated irrelevant history.
**	27-Apr-90 (jorge)
**	     Added include for cvnet.h, cvnet.h include used to be imbedded in 
**	     cvsysdep.h
**	31-Aug-92 (purusho)
**	     Adding amd_us5 define since it is similar to gld_u42
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**	29-may-1997 (loera01) Bug 82561: problems converting F4 and F8.
**	    Added NO_OPTIM rs4_us5.
**	10-may-1999 (walro03)
**	    Remove obsolete version string amd_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Apr-2001 (hweho01)
**          Added NO_OPTIM to ris_u64. Fix the converting problems with 
**          with F4 and F8.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	06-Jan-2003 (hanje04)
**	    Added NO_OPTIM for a64_lnx becuase of compiler segv with GCC 3.2.1
** 	    Checking in for now, will try removing with next compiler update.
**/

/*
** Name: CV2n_f4
**
** Description:
**	Convert local F4 value to network F4 format.
**	There are no alignment restrictions on input
**	or output.
**
** Input:
**	src	Local F4 value
**
** Output:
**	dst	Network F4 value
**	status	OK or error code.
**
** Returns:
**	VOID
**
** History:
*/

VOID
CV2n_f4( src, dst, status )
register u_i1	*src;
register u_i1	*dst;
STATUS		*status;
{
    u_i1	sign;
    u_i2	biased;
    i4		exponent;
    u_i4	mantissa;
    u_i2	sign_and_exp;
    f4		temp;

    /*
    ** Extract float components.
    */
    MECOPY_CONST_MACRO( (PTR)src, sizeof( temp ), (PTR)&temp );
    sign = CV_F4SIGN( &temp );
    biased = CV_F4EXP( &temp );
    mantissa = CV_F4MANT( &temp );

    /*
    ** Unbias exponent and check range.
    */
    exponent = (i4)biased - CV_F4BIAS;

    if ( exponent < CV_F4EXPMIN )  
    {
	/*
	** Very small magnitudes forced to 0.
	*/
	sign = 0;
	biased = 0;
	mantissa = 0;
    }
    else  if ( exponent > CV_F4EXPMAX )
    {
	/*
	** Very large magnitudes force to max.
	** Sign is not changed.
	*/
	biased = 0x7FFF;
	mantissa = 0xFFFFFFFF;
    }
    else
    {
	/*
	** Re-bias the exponent for NTS.
	** Sign and mantissa unchanged.
	*/
	biased = (u_i2)(exponent + CV_EXPBIAS);
    }

    /*
    ** Build NTS float value.
    */
    sign_and_exp = ((u_i2)sign << 15) | biased;

    dst[0] = ((u_i1 *)&sign_and_exp)[ CV_I2BYTE0 ];
    dst[1] = ((u_i1 *)&sign_and_exp)[ CV_I2BYTE1 ];
    dst[2] = ((u_i1 *)&mantissa)[ CV_I4BYTE2 ];
    dst[3] = ((u_i1 *)&mantissa)[ CV_I4BYTE3 ];
    dst[4] = ((u_i1 *)&mantissa)[ CV_I4BYTE0 ];
    dst[5] = ((u_i1 *)&mantissa)[ CV_I4BYTE1 ];

    *status = OK;
    return;
}
 
/*
** Name: CV2n_f8
**
** Description:
**	Convert local F8 value to network F8 format.
**	There are no alignment restrictions on input
**	or output.
**
** Input:
**	src	Local F8 value
**
** Output:
**	dst	Network F8 value
**	status	OK or error code.
**
** Returns:
**	VOID
**
** History:
*/

VOID 
CV2n_f8( src, dst, status )
register u_i1	*src;
register u_i1	*dst;
STATUS		*status;
{
    u_i1	sign;
    u_i2	biased;
    i4		exponent;
    u_i4	mantlo;
    u_i4	manthi;
    u_i2	sign_and_exp;
    f8		temp;

    /*
    ** Extract local float components.
    */
    MECOPY_CONST_MACRO( (PTR)src, sizeof( temp ), (PTR)&temp );
    sign = CV_F8SIGN( &temp );
    biased = CV_F8EXP( &temp );
    manthi = CV_F8MANTHI( &temp );
    mantlo = CV_F8MANTLO( &temp );

    /*
    ** Unbias exponent and check range.
    */
    exponent = (i4)biased - CV_F8BIAS;

    if ( exponent < CV_F8EXPMIN )
    {
	/*
	** Very small magnitudes forced to 0.
	*/
	sign = 0;
	biased = 0;
	manthi = 0;
	mantlo = 0;
    }
    else  if ( exponent > CV_F8EXPMAX )
    {
	/*
	** Very large magnitudes force to max.
	** Sign is not changed.
	*/
	biased = 0x7FFF;
	manthi = 0xFFFFFFFF;
	mantlo = 0xFFFFFFFF;
    }
    else
    {
	/*
	** Re-bias the exponent for NTS.
	** Sign and mantissa unchanged.
	*/
	biased = (u_i2)(exponent + CV_EXPBIAS);
    }

    /*
    ** Build network float value.
    */
    sign_and_exp = ((u_i2)sign << 15) | biased;

    dst[0] = ((u_i1 *)&sign_and_exp)[ CV_I2BYTE0 ];
    dst[1] = ((u_i1 *)&sign_and_exp)[ CV_I2BYTE1 ];
    dst[2] = ((u_i1 *)&manthi)[ CV_I4BYTE2 ];
    dst[3] = ((u_i1 *)&manthi)[ CV_I4BYTE3 ];
    dst[4] = ((u_i1 *)&manthi)[ CV_I4BYTE0 ];
    dst[5] = ((u_i1 *)&manthi)[ CV_I4BYTE1 ];
    dst[6] = ((u_i1 *)&mantlo)[ CV_I4BYTE2 ];
    dst[7] = ((u_i1 *)&mantlo)[ CV_I4BYTE3 ];
    dst[8] = ((u_i1 *)&mantlo)[ CV_I4BYTE0 ];
    dst[9] = ((u_i1 *)&mantlo)[ CV_I4BYTE1 ];

    *status = OK;
    return;
}

/*
** Name: CV2l_f4
**
** Description:
**	Convert network F4 value to local F4 format.
**	There are no alignment restrictions on input
**	or output.
**
** Input:
**	src	Network F4 value
**
** Output:
**	dst	Local F4 value
**	status	OK or error code.
**
** Returns:
**	VOID
**
** History:
*/

VOID
CV2l_f4( src, dst, status )
register u_i1	*src;
register u_i1	*dst;
STATUS		*status;
{
    u_i1	sign;
    u_i2	biased;
    i4		exponent;
    u_i4	mantissa;
    f4		temp;

    /*
    ** Extract network float components.
    */
    sign = (src[1] & 0x80) >> 7;
    ((u_i1 *)&biased)[ CV_I2BYTE0 ] = src[0];
    ((u_i1 *)&biased)[ CV_I2BYTE1 ] = src[1] & 0x7f;
    ((u_i1 *)&mantissa)[ CV_I4BYTE0 ] = src[4];
    ((u_i1 *)&mantissa)[ CV_I4BYTE1 ] = src[5];
    ((u_i1 *)&mantissa)[ CV_I4BYTE2 ] = src[2];
    ((u_i1 *)&mantissa)[ CV_I4BYTE3 ] = src[3];

    /*
    ** Unbias exponent and check range.
    */
    exponent = (i4)biased - CV_EXPBIAS;

    if ( biased == 0 )
    {
	/*
	** Very small magnitudes forced to 0.
	*/
	sign = 0;
	biased = 0;
	mantissa = 0;
    }
    else  if ( exponent < CV_F4EXPMIN )
    {
	/*
	** Very small magnitudes force to min.
	** Sign is not changed.
	*/
	biased = CV_F4EXPMIN + CV_F4BIAS;
	mantissa = 0;
    }
    else  if ( exponent > CV_F4EXPMAX )
    {
	/*
	** Very large magnitudes force to max.
	** Sign is not changed.
	*/
	biased = CV_F4EXPMAX + CV_F4BIAS;
	mantissa = 0xFFFFFFFF;
    }
    else
    {
	/*
	** Re-bias the exponent for local format.
	** Sign and mantissa unchanged.
	*/
	biased = (u_i2)(exponent + CV_F4BIAS);
    }

    /*
    ** Build local float value.
    */
    CV_MKF4( sign, biased, mantissa, &temp );
    MECOPY_CONST_MACRO( (PTR)&temp, sizeof( temp ), (PTR)dst );

    *status = OK;
    return;
}

/*
** Name: CV2l_f8
**
** Description:
**	Convert network F8 value to local F8 format.
**	There are no alignment restrictions on input
**	or output.
**
** Input:
**	src	Network F8 value
**
** Output:
**	dst	Local F8 value
**	status	OK or error code.
**
** Returns:
**	VOID
**
** History:
*/

VOID
CV2l_f8( src, dst, status )
register u_i1	*src;
register u_i1	*dst;
STATUS		*status;
{
    u_i1 	sign;
    u_i2 	biased;
    i4		exponent;
    u_i4	manthi;
    u_i4	mantlo;
    f8		temp;

    /*
    ** Extract network float components.
    */
    sign = (src[1] & 0x80) >> 7;
    ((u_i1 *)&biased)[ CV_I2BYTE0 ] = src[0];
    ((u_i1 *)&biased)[ CV_I2BYTE1 ] = src[1] & 0x7f;
    ((u_i1 *)&manthi)[ CV_I4BYTE0 ] = src[4];
    ((u_i1 *)&manthi)[ CV_I4BYTE1 ] = src[5];
    ((u_i1 *)&manthi)[ CV_I4BYTE2 ] = src[2];
    ((u_i1 *)&manthi)[ CV_I4BYTE3 ] = src[3];
    ((u_i1 *)&mantlo)[ CV_I4BYTE0 ] = src[8];
    ((u_i1 *)&mantlo)[ CV_I4BYTE1 ] = src[9];
    ((u_i1 *)&mantlo)[ CV_I4BYTE2 ] = src[6];
    ((u_i1 *)&mantlo)[ CV_I4BYTE3 ] = src[7];
    
    /*
    ** Unbias exponent and check range.
    */
    exponent = (i4)biased - CV_EXPBIAS;

    if ( biased == 0 )
    {
	/*
	** Very small magnitudes forced to 0.
	*/
	sign = 0;
	biased = 0;
	manthi = 0;
	mantlo = 0;
    }
    else  if ( exponent < CV_F8EXPMIN )
    {
	/*
	** Very small magnitudes force to min.
	** Sign is not changed.
	*/
	biased = CV_F8EXPMIN + CV_F8BIAS;
	manthi = 0;
	mantlo = 0;
    }
    else  if ( exponent > CV_F8EXPMAX )
    {
	/*
	** Very large magnitudes force to max.
	** Sign is not changed.
	*/
	biased = CV_F8EXPMAX + CV_F8BIAS;
	manthi = 0xFFFFFFFF;
	mantlo = 0xFFFFFFFF;
    }
    else
    {
	/*
	** Re-bias the exponent for local format.
	** Sign and mantissa unchanged.
	*/
	biased = (u_i2)(exponent + CV_F8BIAS);
    }

    /*
    ** Build local float value.
    */
    CV_MKF8( sign, biased, manthi, mantlo, &temp );
    MECOPY_CONST_MACRO( (PTR)&temp, sizeof( temp ), (PTR)dst );

    *status = OK;
    return;
}


# if defined(gld_u42)
/*
** Converting Gould floating point format to ours and back is
** a special kind of hell.  They use base 16 instead of base 2,
** and negate a floating point number by 2's-complementing the
** entire bit pattern.
*/

/*
** Extract the exponent from an f4.  Normalize to base-2, and excess-16382.
*/
i2
f48exp( f4ptr )

f4 *f4ptr;

{
    i2 retval;

    /* Only work on positive values. */
    f4 absf4 = ((CV_FLTREP *)(f4ptr))->repsign ? -*f4ptr : *f4ptr;
    register int rawexp = ((CV_FLTREP *)(&absf4))->repexp;

    if ( absf4 == 0.0 )
	retval = 0;
    else
    {
	/* Find first set bit in the mantissa; adjust the exponent
	** to normalize the mantissa to 0.5 <= x < 1.0
	*/
	register int bitmask = 0x800000;
	rawexp <<= 2;
	while ( !(((FLOATREP *)(&absf4))->repmant & bitmask) )
	{
	    bitmask >>= 1;
	    rawexp--;
	}
	retval = rawexp - BIASDIFF;
    }

    return( retval );
}


/*
** Get the mantissa, normalized to 0.5 <= m < 1.0, for the given floating
** point value.  If 'type' is MANT4, 'ptr' points to an f4; get 32 bits
** of mantissa, not counting the implied 1-bit.  If 'type' is MANT8HI
** or MANT8LO, 'ptr' points to an f8; get either the high or the low
** 32 bits of mantissa, respectively.
*/
f48mant( type, ptr )

i4  type;
f8  *ptr;

{
    f8 absf8;
    register int manthi;
    register int mantlo;
    register int bitmask;
    register int shift = 8 + 1;		/* 8 bits to left-justify, plus 1 for
					** implied 1-bit
					*/

    /* Only work on positive values. */
    absf8 = (*ptr < 0.0) ? -*ptr : *ptr;
    manthi = ((CV_DBLREP *)&absf8)->repmanthi;
    if ( type == CV_MANT4 )
	mantlo = 0;
    else
	mantlo = ((CV_DBLREP *)&absf8)->repmantlo;
    if ( manthi == 0 && mantlo == 0 )
	return( 0 );
    /* Find the first set bit in the native mantissa; adjust the
    ** shift count so as to normalize the INGRES mantissa to 0.5 <= m < 1.0.
    */
    for ( bitmask = 0x800000; !(manthi & bitmask); bitmask >>= 1 )
	shift++;
    if ( type == CV_MANT4 || type == CV_MANT8HI )
	return( (manthi << shift) | CV_HIBITS( mantlo, shift, 32 ) );
    else
	return( mantlo << shift );
}


/*
** Make an f4 out of the given sign, exponent, and mantissa.  Need
** to convert the exponent to base 16 from base 2, and get the implied
** 1-bit into the mantissa.
*/

i4
mkf4( sign, exp, mant, f4ptr )

unsigned sign, exp, mant;
f4 *f4ptr;

{
    if ( exp == 0 )
    {
	*f4ptr = 0.0;
	return(OK);
    }
    exp += CV_F4BDIFF;
    /* Insert the implied 1-bit. */
    mant >>= 1;
    mant |= 0x80000000;
    /* Make the exponent a multiple of 4; adjust mantissa accordingly. */
    while ( exp & 3 )
    {
	mant >>= 1;
	exp++;
    }
    /* Convert the exponent to base 16. */
    exp >>= 2;
    if ( exp < 0 || exp >127 )
	return( CV_EXPTRUNC );
    ((CV_FLTREP *)f4ptr)->repsign = 0;
    ((CV_FLTREP *)f4ptr)->repexp = exp;
    ((CV_FLTREP *)f4ptr)->repmant = mant >> 8;
    if ( sign == 1 )
	*f4ptr = -*f4ptr;
    return( OK );
}



i4
mkf8( sign, exp, manthi, mantlo, f8ptr )

unsigned sign, exp, manthi, mantlo;
f8 *f8ptr;

{
    register int shift = 8;	/* Shift count for mantissa */
    if ( exp == 0 )
    {
	*f8ptr = 0.0;
	return( OK );
    }
    exp += CV_F8BDIFF;
    /* Make the exponent a multiple of 4; adjust mantissa shift count to fit. */
    while ( exp & 3 )
    {
	shift++;
	exp++;
    }
    /* Convert exponent to base 16. */
    exp >>= 2;
    if ( exp < 0 || exp > 127 )
	return( CV_EXPTRUNC );
    /* Add the implied 1-bit to the mantissa; this requires
    ** a 64-bit wide right shift across manthi and mantlo.
    */
    mantlo >>= 1;
    if ( manthi & 1 )
	mantlo |= 0x80000000;
    manthi >>= 1;
    manthi |= 0x80000000;
    /*
    ** Now do a 64-bit wide right shift by 'shift' bits.
    */
    mantlo >>= shift;
    mantlo |= CV_LOBITS(manthi, shift) << (32 - shift);
    manthi >>= shift;
    ((CV_DBLREP *)f8ptr)->repsign = 0;
    ((CV_DBLREP *)f8ptr)->repexp = exp;
    ((CV_DBLREP *)f8ptr)->repmanthi = manthi;
    ((CV_DBLREP *)f8ptr)->repmantlo = mantlo;
    if ( sign == 1 )
	*f8ptr = -*f8ptr;
    return( OK );
}
# endif	/* gld_u42 */
