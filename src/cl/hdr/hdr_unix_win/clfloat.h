/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: CLFLOAT.H - floating-point configuration values for machines
**		     supporting IEEE format.
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
**	architecture supporting IEEE format floating-point numbers:
**
**	Single precision numbers:
**				 24
**				 __
**	    x = s * power(2,e) * >  f_k * power(2, -k), -125 <= e <= +128
**				/__
**				 k=1
**
**	Double precision numbers:
**				 53
**				 __
**	    x = s * power(2,e) * >  f_k * power(2, -k), -1021 <= e <= +1024
**				/__
**				 k=1
**
** History:
**      10-jan-91 (stec)
**          Created.
**	    Define values are based on the set of values for IEEE floating
**	    point format numbers described in the CL spec (release 6.3 dated
**	    August, 1990, chapter 4, pages 4-4, 4-5).
**      04-mar-93 (smc)
**          Included <float.h> and excluded cl defines for axp_osf. 
**	26-apr-93 (vijay)
**	    Use <float.h> if exists. xCL_FLOAT_H_EXISTS is defined in bzarch.h.
**      05-Mar-2008 (coomi01) : b119968
**          We need a number, which is a bit less FLT_MAX, called FLT_MAX_ROUND
**          and will help us circumvent rounding up errors when converting to ascii.
**      03-Dec-2009 (coomi01) b122767
**          Add DBL_UNDERFLOW and DBL_UNDERFLOW 
**/



/*
** Platforms that provide an ANSI C <float.h> file do not require these
** defines
*/

#if defined (xCL_FLOAT_H_EXISTS)
#include    <float.h>
#else

/*
**  Defines of other constants.
*/

/*
**      FLT_RADIX   - Radix (base) of floating-point.
*/
#define                 FLT_RADIX	2

/*
**	FLT_ROUNDS  - rounding behaviour for addition.
**
**	    -1	undeterminable rounding; no strict rules apply.
**	     0	towards zero, i.e., truncation.
**	     1	to nearest, i.e., rounds to nearest representable value.
**	     2	towards positive infinity, i.e., always rounds up.
**	     3	towards negative infinity, i.e., always rounds down.
*/
#define			FLT_ROUNDS	1

/*
**	Number of base FLT_RADIX digits in the floating-point mantissa p.
**
**	FLT_MANT_DIG - number of FLT_RADIX digits in the mantissa of a float.
**	DBL_MANT_DIG - number of FLT_RADIX digits in the mantissa of a double.
*/
#define			FLT_MANT_DIG	24
#define			DBL_MANT_DIG	53

/*
**	The minimum positive floating-point number that affects an addition,
**	x such that 1.0 + x != 1.0, power(b, 1-p).
**
**	FLT_EPSILON - smallest addition operand to 1.0 for a float.
**	DBL_EPSILON - smallest addition operand to 1.0 for a double.
*/
#define			FLT_EPSILON	((float)1.19209290e-07)
#define			DBL_EPSILON	2.2204460492503131e-16

/*
**	The number of decimal digits of precision, p if b = 10, otherwise
**	| ((p-1) * log10 b) |
**	|_		   _|
**
**	FLT_DIG - decimal precision for a float.
**	DBL_DIG - decimal precision for a double.
*/
#define			FLT_DIG		6
#define			DBL_DIG		15

/*
**	Minimum negative integer such that FLT_RADIX raised to that power
**	minus 1 is a normalized floating-point number, e_min.
**
**	FLT_MIN_EXP - minimum exponent for a float.
**	DBL_MIN_EXP - minimum exponent for a double.
*/
#define			FLT_MIN_EXP	(-125)
#define			DBL_MIN_EXP	(-1021)

/*
**	Minimum normalized positive floating-point number, power(b, e_min-1)
**
**	FLT_MIN - minimum positive value for a float.
**	DBL_MIN - minimum positive value for a double.
*/
#define			FLT_MIN		((float)1.17549435e-38)
#define			DBL_MIN		2.2250738585072014e-308

/*
**	The minimum negative integer such that 10 raised to that power is in
**	the range of normalized floating-point numbers,
**	    __			    __
**	    | log10 power(b,e_min-1) |
**	    |			     |
**	
**	FLT_MIN_10_EXP - minimum decimal exponent for a float.
**	DBL_MIN_10_EXP - minimum decimal exponent for a double.
*/
#define			FLT_MIN_10_EXP	(-37)
#define			DBL_MIN_10_EXP	(-307)

/*
**	The maximum integer such that FLT_RADIX raised to that power minus 1
**	is a representable finite floating-point number, e_max.
**
**	FLT_MAX_EXP - maximum exponent for a float.
**	DBL_MAX_EXP - maximum exponent for a double.
*/
#define			FLT_MAX_EXP	128
#define			DBL_MAX_EXP	1024

/*
**	The maximum representable floating-point number, 
**	(1 - power(b,p)) * power(b,e_max)
**
**	FLT_MAX	- maximum positive value for a float.
**	DBL_MAX	- maximum positive value for a double.
*/
#define			FLT_MAX		((float)3.40282347e+38)
#define			DBL_MAX		1.7976931348623157e+308

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
*/
#define			FLT_MAX_10_EXP	38
#define			DBL_MAX_10_EXP	308

#endif /* axp_osf */

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
