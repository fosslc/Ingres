/*
**    Copyright (c) 1986, 2000 Ingres Corporation
*/

#include    <compat.h>
#include    <mh.h>
# ifdef ALPHA
# ifdef fabs
#	undef fabs
# endif
# include    <math.h>
# endif
# include    <ex.h>
# include    <me.h>
# include    <cs.h>
# include    <errno.h>
# include    <stdlib.h>

# define	NOERR		0	/* resets 'errno' to no error state */


/**
**
**  Name: MH.C - Math functions for the CL.
**
**  Description:
**      This file contains all of the math functions defined by the CL spec
**	as being part of the MH module.
**
**          MHatan() - Arctangent.
**          MHatan2() - Arctangent.
**          MHceil() - Ceiling.
**          MHacos() - Arccosine.
**          MHcos() - Cosine.
**          MHexp() - Exponential.
**          MHipow() - Integer power.
**          MHln() - Natural log.
**          MHpow() - Power.
**          MHsrand() - Set random number generator.
**          MHrand() - Random number.
**          MHasin() - Arcsine.
**          MHsin() - Sine.
**          MHsqrt() - Square root.
**          MHexp10() - Ten to a power.
**          MHfdint() - Take integer part of a double.
**
**
**  History:    $Log-for RCS$
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	15-jun-93 (kevinm)
**	   Changed all of the MTH$D functions to MTH$T functions on alpha.
**	24-jun-93 (kevinm)
**		Declared OTS$ and MTH$ routines with prototypes so that the
**		compiler does the proper conversion.
**	07-apr-94 (walt)
**	    BUG 62135.  On ALPHA/VMS the atan function MHatan wasn't passing
**	    it's argument to MTH$TATAN, causing an AV in the server.
**	26-may-94 (wolf) 
**	    A FABS macro was added to compat.h, but it conflicts with the
**	    definition in math.h.  For now, undefine it.
**	18-aug-95 (duursma)
**	    Replaced the bodies of all routines with modified UNIX CL versions.
**	    Rather than calling low-level VMS math routines, we go through the 
**	    C runtimes instead.  This has the advantage that no exceptions can 
**	    be raised.  By simply checking errno, we determine whether an error 
**	    condition (basically either domain error or range error) exists, and
**	    we then call EXsignal() with the appropriate argument.
**	    Also updated some comments, eg. corrected the list of exceptions
**	    that each routine can raise. 
**	    This fixes bugs 69501 and 70559.
**      15-Jan-1999 (shero03)
**          Add MHrand2
**      01-Mar-1999 (shero03)
**          Use TLS seed when available
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
*/

/* # defines */
/* typedefs */

typedef void FP_Callback(CS_SCB **scb);
/* forward references */
/* externs */
FUNC_EXTERN void FP_get_scb(CS_SCB **scb);
/* statics */

/* jseed must be loaded with an initial value of some sort. */
static u_i4 jseed = 734221605;
static ME_TLS_KEY  MHrandomkey = 0;



/*{
** Name: MHtan() - Tangent.
**
** Description:
**	Find the tangent of angle x in radians.
**
** Inputs:
**      x                               Number of radians to find the tangent of.
**
** Outputs:
**      none
**
**	Returns:
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**	2-may-2007 (dougi)
**	    Cloned from MHatan().
*/

f8
MHtan(x)
f8  x;
{
    f8	result;

    errno = NOERR;
    result = tan(x);

    switch (errno)
    {
      case NOERR:
	return(result);
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0);
}


/*{
** Name: MHatan() - Arctangent.
**
** Description:
**	Find the arctangent of x.
**	Returned result is in radians.
**
** Inputs:
**      x                               Number to find the arctangent of.
**
** Outputs:
**      none
**
**	Returns:
**	    Returned result is in radians.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	07-apr-94 (walt)
**	    BUG 62135.  On ALPHA/VMS the atan function MHatan wasn't passing
**	    it's argument to MTH$TATAN, causing an AV in the server.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHatan(x)
f8  x;
{
    f8	result;

    errno = NOERR;
    result = atan(x);

    switch (errno)
    {
      case NOERR:
	return(result);
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0);
}


/*{
** Name: MHatan2() - Arctangent.
**
** Description:
**	Find the arctangent of x and y.
**	Returned result is in radians.
**
** Inputs:
**      x                  X-coordinate to find the arctangent of.
**      y                  Y-coordinate to find the arctangent of.
**
** Outputs:
**      none
**
**	Returns:
**	    Returned result is in radians.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**	3-may-2007 (dougi)
**	    Cloned from MHatan().
*/

f8
MHatan2(x, y)
f8  x;
f8  y;
{
    f8	result;

    errno = NOERR;
    result = atan2(x, y);

    switch (errno)
    {
      case NOERR:
	return(result);
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0);
}


/*{
** Name: MHceil() - Ceiling.
**
** Description:
**	Returns the smallest integer not less than the
**	argument passed to it.  The return value, although
**	an integer, is returned as an f8.
**
** Inputs:
**      x                               Number to find ceiling for.
**
** Outputs:
**	none
**
**	Returns:
**	    The smallest integer (as an f8) not less than x.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHceil(x)
f8  x;
{
    f8	result;

    errno = NOERR;
    result = ceil(x);
    switch (errno)
    {
      case NOERR:
	return result;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }

    return (0.0);
}


/*{
** Name: MHcos() - Cosine.
**
** Description:
**	Find the cosine of x, where x is in radians.
**
** Inputs:
**      x                               Number of radians to take cosine of.
**
** Outputs:
**	none
**
**	Returns:
**	    Cosine of x radians.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**	    MH_MATHERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHcos(x)
f8  x;
{
    f8	result;

    errno = NOERR;
    result = cos(x);
    switch (errno)
    {
      case NOERR:
# ifdef	notdef
	if (result < (-1.0 + -1.0e-10) || result > (1.0 + 1.0e-10))
	{
	    EXsignal(MH_MATHERR, 0);
	}
# endif
	return(result);
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}


/*{
** Name: MHacos() - Arccosine.
**
** Description:
**	Find the arccosine of x in radians.
**
** Inputs:
**      x                               Value to take arccosine of.
**
** Outputs:
**	none
**
**	Returns:
**	    Number of radians in arccosine of x.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**	    MH_MATHERR
**
** Side Effects:
**	    none
**
** History:
**	2-may-2007 (dougi)
**	    Cloned from MHcos().
*/

f8
MHacos(x)
f8  x;
{
    f8	result;

    errno = NOERR;
    result = acos(x);
    switch (errno)
    {
      case NOERR:
# ifdef	notdef
	if (result < (-1.0 + -1.0e-10) || result > (1.0 + 1.0e-10))
	{
	    EXsignal(MH_MATHERR, 0);
	}
# endif
	return(result);
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}


/*{
** Name: MHexp() - Exponential.
**
** Description:
**	Find the exponential of x (i.e., e**x).
**
** Inputs:
**      x                               Number to find exponential for.
**
** Outputs:
**      none
**
**	Returns:
**	    exp(x)
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHexp(x)
f8  x;
{
    f8	result;
    f8	ret_val	= 0.0;

    errno  = NOERR;
    result = exp(x);

    switch (errno)
    {
      case NOERR:
	ret_val = result;
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }

    return(ret_val);
}


/*{
** Name: MHipow() - Integer power.
**
** Description:
**	Find x**i.
**
** Inputs:
**      x                               Number to raise to a power.
**      i                               Integer power to raise it to.
**
** Outputs:
**      none
**
**	Returns:
**	    x**i
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/
f8
MHipow(x,i)
f8	x;
i4	i;
{
    f8	result;

    errno = NOERR;
    if (i >= 0)
    {
	result = pow(x, (f8) i);
    }
    else
    {
	result = 1.0 / pow(x, (f8) -i);
    }
    switch(errno)
    {
      case NOERR:
	return(result);
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}

/*{
 ** Name: MHln() - Natural log.
 **
 ** Description:
 **	Find the natural logarithm (ln) of x.
 **	x must be greater than zero.
 **
 ** Inputs:
 **      x                               Number to find log of.
 **
 ** Outputs:
 **      none
 **
 **	Returns:
 **	    ln(x)
 **
 **	Exceptions:
 **	    MH_BADARG
 **	    EXFLTOVF
 **	    MH_INTERNERR
 **
 ** Side Effects:
 **	    none
 **
 ** History:
 **      10-sep-86 (thurston)
 **          Upgraded to Jupiter coding standards.
 **	18-aug-95 (duursma)
 **	    Replaced body of routine with modified UNIX CL version
 */

f8
MHln(x)
f8  x;
{
    f8	result;
    f8	ret_val = 0.0;

    errno = NOERR;

    if (x > 0.0)
	result = log(x);
    else
	errno = EDOM;
    switch(errno)
    {
      case NOERR:
	ret_val = result;
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(ret_val);
}


/*{
** Name: MHpow() -- Raise to power of
**
** Description:
**	Find x**y.
**
** Inputs:
**      x                               Number to raise to a power.
**      y                               Power to raise it to.
**
** Outputs:
**      none
**
**	Returns:
**	    x**y
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	23-jun-93 (kevinm)
**	    On alpha we are using IEEE and there isn't a raise a
**	    T-floating base to a T-floating exponent routine.  Therefore
**	    we're using a G floating routine.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
**
*/

f8
MHpow(x, y)
f8 x, y;
{
    f8	result;

    errno = NOERR;
    if (y >= 0.0)
    {
	result = pow(x, y);
    }
    else
    {
	result = 1.0 / pow(x, -y);
    }

    switch(errno)
    {
      case NOERR:
	return(result);
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}

/*{
** Name: MHsrand() - Set random number generator.
**
** Description:
**	Initialize random number generator to a particular
**	starting point.  This should just plain work with no problems.
**
** Inputs:
**      seed                            Just any ol' number.
**
** Outputs:
**      none
**
**	Returns:
**	    OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version;
**	    Eliminated obsolete comments regarding static variables
*/

static i4 gseed;          /* Seed for RNG */
STATUS
MHsrand(seed)
i4	seed;
{
    srand(seed);
    return(OK);
}


/*{
** Name: MHrand() - Random number.
**
** Description:
**	Return the next random number.
**	This also should just plain work with no problems.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    A random number as an f8.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHrand()
{
    return( ( rand() % MAXI2 ) / (f8) MAXI2 );

}



/*{
** Name: MHrand2() - Random number.
**
** Description:
**      Return the next random integer.
**      This routine improves the standard ANSI algorithm by using more
**      high order bits.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**      Returns:
**          A random number as an u_i4.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      15-Jan-1999 (shero03)
**          Created.
**      01-Mar-1999 (shero03)
**          Use TLS seed when available
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
*/
u_i4
MHrand2()
{
    u_i4        *seedptr;
    CS_SCB      *scb;

    FP_get_scb(&scb);                   /* see if we have sessions */
    if (scb)
    {
        seedptr = (u_i4 *)&scb->cs_random_seed; /* yes, and its non 0 use it */
        if (*seedptr == 0)
           seedptr = &jseed;
    }
    else
        seedptr = &jseed;
    *seedptr = *seedptr * 1103515245 + 12345;
    return( (*seedptr >> 7) & MH_MAXRAND2INT );
}


/*{
** Name: MHsrand2() - Set random number generator for algorithm 2.
**
** Description:
**      Initialize random number generator 2 to a particular
**      starting point.
**
**      Note:   This routine sets a static variable, which is normally
**              discouraged in a server environment.  However, due to the
**              nature of the random number generator, it will probably not
**              effect the functioning of the MHrand() routine if many sessions
**              are actively sharing this variable.  In fact, it may indeed
**              give an added randomness to the routine!
**
** Inputs:
**      seed                            Just any ol' number.
**
** Outputs:
**      none
**
**      Returns:
**          OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          Sets a static variable.  (See note in description above.)
**
** History:
**      19-Jan-1999 (shero03)
**          Created.
**      01-Mar-1999 (shero03)
**          Use TLS seed when available
*/
STATUS
MHsrand2(seed)
i4      seed;
{
    CS_SCB      *scb;

    FP_get_scb(&scb);                   /* see if we have sessions */
    if (scb)
        scb->cs_random_seed = seed;
    else
        jseed = seed;
    return(OK);
}


/*{
** Name: MHsin() - Sine.
**
** Description:
**	Find the sine of x, where x is in radians.
**
** Inputs:
**      x                               Number of radians to take sine of.
**
** Outputs:
**	none
**
**	Returns:
**	    sine of x radians.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHsin(x)
f8  x;
{
    f8	result;
    errno = NOERR;
    result = sin(x);
    switch(errno)
    {
      case NOERR:
# ifdef	notdef
	if (result < (-1.0 + -1.0e-10) || result > (1.0 + 1.0e-10))
	{
	    EXsignal(MH_MATHERR, 0);
	}
# endif
	return(result);
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}


/*{
** Name: MHasin() - Arcsine.
**
** Description:
**	Find the arcsine of x in radians.
**
** Inputs:
**      x                               Value to take arcsine of.
**
** Outputs:
**	none
**
**	Returns:
**	    arcsine of x in radians.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-may-2007 (dougi)
**	    Cloned from MHsin().
*/

f8
MHasin(x)
f8  x;
{
    f8	result;
    errno = NOERR;
    result = asin(x);
    switch(errno)
    {
      case NOERR:
# ifdef	notdef
	if (result < (-1.0 + -1.0e-10) || result > (1.0 + 1.0e-10))
	{
	    EXsignal(MH_MATHERR, 0);
	}
# endif
	return(result);
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}


/*{
** Name: MHsqrt() - Square root.
**
** Description:
**	Find the square root of x.
**	Only works if x is non-negative.
**
** Inputs:
**      x                               Number to find square root of.
**
** Outputs:
**      none
**
**	Returns:
**	    Square root of x.
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHsqrt(x)
f8  x;
{
    f8	result;
    f8	ret_val = 0.0;
    
    if (x >= 0.0)
    {
	errno = NOERR;
	result = sqrt(x);
    }
    else
    {
	errno = EDOM;
    }
    switch (errno)
    {
      case NOERR:
	ret_val = result;
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(ret_val);
}


/*{
** Name: MHexp10() - Ten to a power.
**
** Description:
**	Find 10**x.
**
** Inputs:
**      x                               Number to raise 10 to.
**
** Outputs:
**      none
**
**	Returns:
**	    10**x
**
**	Exceptions:
**	    MH_BADARG
**	    EXFLTOVF
**	    MH_INTERNERR
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHexp10(x)
f8  x;
{
    f8	result;

    errno = NOERR;
    result = pow(10.0, x);

    switch(errno)
    {
      case NOERR:
	return(result);
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}


/*{
** Name: MHfdint() - Take integer part of a double.
**
** Description:
**      Take the integer part of the passed in
**	double and return it as a double.
**
** Inputs:
**      x                               The double to take the integer part of.
**
** Outputs:
**      none
**
**	Returns:
**	    Integer part of x (as an f8).
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-sep-86 (thurston)
**          Upgraded to Jupiter coding standards.
**	18-aug-95 (duursma)
**	    Replaced body of routine with modified UNIX CL version
*/

f8
MHfdint(val)
f8  val;
{
    f8	result;

    errno = NOERR;
    result = (val >= 0.0 ? floor(val) : ceil(val));

    switch(errno)
    {
      case NOERR:
	return(result);
	break;
      case EDOM:
	EXsignal(MH_BADARG, 0);
	break;
      case ERANGE:
	EXsignal(EXFLTOVF, 0);
	break;
      default:
	EXsignal(MH_INTERNERR, 0);
	break;
    }
    return(0.0);
}
