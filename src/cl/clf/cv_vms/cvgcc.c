/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cv.h>
# include <cvgcc.h>

  
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
**      16-jul-93 (ed)
**	    added gl.h
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	20-Apr-04 (gordy)
**	    Cleaned up conversion macros.  Removed non-VAX platforms.
**/

VOID
CV2n_f4( src, dst, status )
register char	*src;
register char	*dst;
STATUS		*status;	/* set only on error */
{
    i2 sign_and_exp;
    i4 mantissa;
    f4 temp;

    MECOPY_CONST_MACRO( (PTR)src, sizeof( temp ), (PTR)&temp );

    sign_and_exp = ( F4SIGN( &temp ) << 15 );
    sign_and_exp |= F4EXP( &temp );
    mantissa = F4MANT( &temp );

    dst[0] = ((char *)&sign_and_exp)[ I2BYTELO ];
    dst[1] = ((char *)&sign_and_exp)[ I2BYTEHI ];
    dst[2] = ((char *)&mantissa)[ I4BYTE2 ];
    dst[3] = ((char *)&mantissa)[ I4BYTE3 ];
    dst[4] = ((char *)&mantissa)[ I4BYTE0 ];
    dst[5] = ((char *)&mantissa)[ I4BYTE1 ];

    *status = OK;
    return;
}
 
VOID
CV2n_f8( src, dst, status )
register char	*src;
register char	*dst;
STATUS		*status;	/* set only on error */
{
    i2 sign_and_exp;
    i4 mantlo;
    i4 manthi;
    f8 temp;
    register char *cp = (char *) &temp;

    MECOPY_CONST_MACRO( (PTR)src, sizeof( temp ), (PTR)&temp );

    sign_and_exp = ( F8SIGN( &temp ) << 15 );
    sign_and_exp |= F8EXP( &temp );
    mantlo = F8MANTLO( &temp );
    manthi = F8MANTHI( &temp );

    dst[0] = ((char *)&sign_and_exp)[ I2BYTELO ];
    dst[1] = ((char *)&sign_and_exp)[ I2BYTEHI ];
    dst[2] = ((char *)&manthi)[ I4BYTE2 ];
    dst[3] = ((char *)&manthi)[ I4BYTE3 ];
    dst[4] = ((char *)&manthi)[ I4BYTE0 ];
    dst[5] = ((char *)&manthi)[ I4BYTE1 ];
    dst[6] = ((char *)&mantlo)[ I4BYTE2 ];
    dst[7] = ((char *)&mantlo)[ I4BYTE3 ];
    dst[8] = ((char *)&mantlo)[ I4BYTE0 ];
    dst[9] = ((char *)&mantlo)[ I4BYTE1 ];

    *status = OK;
    return;
}

VOID
CV2l_f4( src, dst, status )
register char	*src;
register char	*dst;
STATUS		*status;	/* set only on error */
{
    i2 exp;
    i4 mant;
    f4 temp;
    register int sign = ( ( src[1] & 0x80 ) != 0 );

    ((char *)&exp)[ I2BYTELO ] = src[0];
    ((char *)&exp)[ I2BYTEHI ] = src[1] & 0x7f;
    ((char *)&mant)[ I4BYTE0 ] = src[4];
    ((char *)&mant)[ I4BYTE1 ] = src[5];
    ((char *)&mant)[ I4BYTE2 ] = src[2];
    ((char *)&mant)[ I4BYTE3 ] = src[3];

    MKF4( sign, exp, mant, &temp );
    MECOPY_CONST_MACRO( (PTR)&temp, sizeof( temp ), (PTR)dst );

    *status = OK;
    return;
}

VOID
CV2l_f8( src, dst, status )
register char	*src;
register char	*dst;
STATUS		*status;	/* set only on error */
{
    i2 exp;
    i4 manthi;
    i4 mantlo;
    f8 temp;
    register int sign = ( ( src[1] & 0x80 ) != 0 );

    ((char *)&exp)[ I2BYTELO ] = src[0];
    ((char *)&exp)[ I2BYTEHI ] = src[1] & 0x7f;
    ((char *)&manthi)[ I4BYTE0 ] = src[4];
    ((char *)&manthi)[ I4BYTE1 ] = src[5];
    ((char *)&manthi)[ I4BYTE2 ] = src[2];
    ((char *)&manthi)[ I4BYTE3 ] = src[3];
    ((char *)&mantlo)[ I4BYTE0 ] = src[8];
    ((char *)&mantlo)[ I4BYTE1 ] = src[9];
    ((char *)&mantlo)[ I4BYTE2 ] = src[6];
    ((char *)&mantlo)[ I4BYTE3 ] = src[7];
    
    MKF8( sign, exp, manthi, mantlo, &temp );
    MECOPY_CONST_MACRO( (PTR)&temp, sizeof( temp ), (PTR)dst );

    *status = OK;
    return;
}

