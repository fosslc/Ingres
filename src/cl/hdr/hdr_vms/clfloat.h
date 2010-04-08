/*
** Copyright (c) 1991, Ingres Corporation
** All Rights Reserved
*/

/**
** Name: CLFLOAT.H - floating-point configuration values for VAX/VMS.
**
** ===========================================================================
**
**  7-apr-1994 (walt)
**	This version is now for ALPHA/VMS.  At the bottom it includes the
**	compiler's regular <float.h> file, rather than the previously used
**	"sys$library:float_3_1.h".
**
**	Using the standard <float.h> file allows the correct IEEE definitions
**	to be used if a compile is done with the /float=ieee option.
**
**	The coments immediately below discuss only the VAX F and D floating
**	types.  IEEE floating point isn't available on VAXs.
**
**	The #define statements in this file are all within comments and have
**	no effect.
**	
** ===========================================================================
**	
**
** Description:
**      This file provides the constants that would be provided by an ANSI C
**	<float.h> file. If a real <float.h> file is available on a system, it
**	should be included by <clfloat.h>.
**
**	The characteristics of floating-point types are defined in terms of a
**	model that describes a representation of floating-point numbers and
**	values that provide information about an implementation's floating-point
**	arithmetic. The following parameters are used to define the model for
**	each floating-point type:
**
**	   s  sign (+1/-1)
**	   b  base or radix of exponent representation (an integer > 1)
**	   e  exponent (an integer between a minimum e_min and a maximum e_max)
**	   p  precision (the number of base 'b' digits in the mantissa)
**	   f_k non-negative integers less than 'b' (the mantissa digits)
**
**	A normalized floating-point number x (f_1 > 0 if x != 0) is defined
**	as follows:
**				 p
**				 __
**	    x = s * power(b,e) * >  f_k * power(b, -k), e_min <= e <= e_max
**				/__
**				 k=1
**
**	    where power(b,e) returns value of b raised to the power of e.
**
**	C language defines two different floating-point types: single
**	precision (float) and double precision (double). On machines with
**	VAX architecture float and double are implemented in hardware as
**	F_floating and D_floating format numbers.
**
**	So, for F_floating format numbers:
**				 24
**				 __
**	    x = s * power(2,e) * >  f_k * power(2, -k), -125 <= e <= +128
**				/__
**				 k=1
**
**	and for D_floating format numbers:
**				 56
**				 __
**	    x = s * power(2,e) * >  f_k * power(2, -k), -1021 <= e <= +1024
**				/__
**				 k=1
**
** History:
**      10-jan-91 (stec)
**          Created. First defines were created based on the set of values
**	    in the CL spec, but later decision was made to use the latest
**	    version of float.h provided by DEC. Since C compiler and VMS on
**	    Condor were found not to be the latest, float.h was copied from
**	    sys$library on Swift, which is running 5.4 VMS. The copied file
**	    was placed in sys$library on Condor under the name float_3_1.h.
**	    This issue of defines having different values in different versions
**	    of float.h on VMS was discussed with Dave Brower; his suggestion
**	    was to use float.h for 3.1 VMS/C compiler, provided there were
**	    no compilation problems. 
**	7-apr-94 (walt)
**	    Include <float.h> rather than the previously used "sys$library:
**	    float_3_1.h" which contained only VAX definitions.  Now IEEE
**	    definitions will be used if a file is compiled with /float=ieee.
**      05-Mar-2008 (coomi01) : b119968
**          We need a number, which is a bit less FLT_MAX, called FLT_MAX_ROUND
**          and will help us circumvent rounding up errors when converting to ascii.
**      03-Dec-2009 (coomi01) b122767
**          Add DBL_UNDERFLOW and DBL_UNDERFLOW, moved std header to top of file.
**/

/*
**  Defines of other constants.
*/

/*
**      FLT_RADIX   - Radix (base) of floating-point.
#define                 FLT_RADIX	2
*/

/*
**	FLT_ROUNDS  - rounding behaviour for addition.
**
**	    -1	undeterminable rounding; no strict rules apply.
**	     0	towards zero, i.e., truncation.
**	     1	to nearest, i.e., rounds to nearest representable value.
**	     2	towards positive infinity, i.e., always rounds up.
**	     3	towards negative infinity, i.e., always rounds down.
#define			FLT_ROUNDS	1
*/

/*
**	Number of base FLT_RADIX digits in the floating-point mantissa p.
**
**	FLT_MANT_DIG - number of FLT_RADIX digits in the mantissa of a float.
**	DBL_MANT_DIG - number of FLT_RADIX digits in the mantissa of a double.
#define			FLT_MANT_DIG	24
#define			DBL_MANT_DIG	56
*/

/*
**	The minimum positive floating-point number that affects an addition,
**	x such that 1.0 + x != 1.0, power(b, 1-p).
**
**	FLT_EPSILON - smallest addition operand to 1.0 for a float.
**	DBL_EPSILON - smallest addition operand to 1.0 for a double.
#define			FLT_EPSILON	((float)5.96046448e-8)
#define			DBL_EPSILON	1.38777878078144570e-17
*/

/*
**	The number of decimal digits of precision, p if b = 10, otherwise
**	| ((p-1) * log10 b) |
**	|_		   _|
**
**	FLT_DIG - decimal precision for a float.
**	DBL_DIG - decimal precision for a double.
#define			FLT_DIG		6
#define			DBL_DIG		16
*/

/*
**	Minimum negative integer such that FLT_RADIX raised to that power
**	minus 1 is a normalized floating-point number, e_min.
**
**	FLT_MIN_EXP - minimum exponent for a float.
**	DBL_MIN_EXP - minimum exponent for a double.
#define			FLT_MIN_EXP	(-127)
#define			DBL_MIN_EXP	(-127)
*/

/*
**	Minimum normalized positive floating-point number, power(b, e_min-1)
**
**	FLT_MIN - minimum positive value for a float.
**	DBL_MIN - minimum positive value for a double.
#define			FLT_MIN		((float)2.93873588e-39)
#define			DBL_MIN		2.93873587705571880e-39
*/

/*
**	The minimum negative integer such that 10 raised to that power is in
**	the range of normalized floating-point numbers,
**	    __			    __
**	    | log10 power(b,e_min-1) |
**	    |			     |
**	
**	FLT_MIN_10_EXP - minimum decimal exponent for a float.
**	DBL_MIN_10_EXP - minimum decimal exponent for a double.
#define			FLT_MIN_10_EXP	(-38)
#define			DBL_MIN_10_EXP	(-38)
*/

/*
**	The maximum integer such that FLT_RADIX raised to that power minus 1
**	is a representable finite floating-point number, e_max.
**
**	FLT_MAX_EXP - maximum exponent for a float.
**	DBL_MAX_EXP - maximum exponent for a double.
#define			FLT_MAX_EXP	127
#define			DBL_MAX_EXP	127
*/

/*
**	The maximum representable floating-point number, 
**	(1 - power(b,p)) * power(b,e_max)
**
**	FLT_MAX	- maximum positive value for a float.
**	DBL_MAX	- maximum positive value for a double.
#define			FLT_MAX		((float)1.70141173e+38)
#define			DBL_MAX		1.70141183460469230e+38
*/

/*
**	The maximum integer such that 10 raised to that power is in
**	the range of representable floating-point numbers,
**	      			    
**	    | log10 ((1 - power(b,-p) * power(b,e_max) |
**	    |					       |
**	    __					      __
**	
**	FLT_MAX_10_EXP	- maximum decimal exponent for a float.
**	DBL_MAX_10_EXP	- maximum decimal exponent for a double.
#define			FLT_MAX_10_EXP	38
#define			DBL_MAX_10_EXP	38
*/

/* Include compiler's standard float.h file */
#include <float.h>

/*
**       FLT_MAX_ROUND is decimal value of the IEEE FLT_MAX with the
**       lowest bit of the Significand reset to 0.
**
*/
#ifdef IEEE_FLOAT
#define FLT_MAX_ROUND   3.4028232635611926e+38
#endif

#ifndef DBL_UNDERFLOW
#define                 DBL_UNDERFLOW   (DBL_MIN - .5)
#define                 DBL_OVERFLOW    (DBL_MAX + .5)
#endif
