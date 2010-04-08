/*
**    Copyright (c) 1993, 2002 Ingres Corporation
*/

#include    <compat.h>
#include    <fp.h>
/*
**
**  Name: FP.C - Floating Point functions for the CL.
**
**  Description:
**      This file contains most of the floating point functions defined by the 
**	CL spec as being part of the FP module.  (The random number functions
**	are in a separate file.)
**
**          FPatan() - Arctangent.
**          FPceil() - Ceiling.
**          FPcos() - Cosine.
**          FPdfinite() - Check for finite double.
**          FPexp() - Exponential.
**          FPffinite() - Check for finite float.
**          FPipow() - Integer power.
**          FPln() - Natural log.
**          FPpow() - Power.
**          FPsin() - Sine.
**          FPsqrt() - Square root.
**          FPdmath() - Basic floating point math functions.
**
**  History: 
**      21-apr-92 (rog)
**          Created from mh.c.
**      21-jul-1993 (rog)
**          Added FPdmath().
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	18-dec-2002 (abbjo03)
**	    Remove definition of FPfdint which will now be a #define in fpcl.h.
*/

FUNC_EXTERN double	MTH$DATAN(double *x);
FUNC_EXTERN double	MTH$DCOS(double *x);
FUNC_EXTERN double	MTH$DEXP(double *x);
FUNC_EXTERN double	MTH$DLOG(double *x);
FUNC_EXTERN double	MTH$DSIN(double *x);
FUNC_EXTERN double	MTH$DSQRT(double *x);
FUNC_EXTERN double	MTH$RANDOM(i4 gseed);
FUNC_EXTERN double	OTS$POWDJ(double x, i4 i);
FUNC_EXTERN double	OTS$POWDD(double x, double i);
FUNC_EXTERN VOID	lib$establish(long func());
FUNC_EXTERN long	lib$sig_to_ret();

/*
** These two internal functions really look strange.  lib$establish sets a
** signal handler that never returns from the function that established it.
** Therefore, we set up these dummy functions with a signal handler that
** converts signals into return statuses.  Then, we try to access the floating
** point number stored at the location pointed to by the passed-in pointer.
** If it contains a valid value, then either the if is true or false, and the
** number is valid, so we return a zero.  If the number is not finite, the
** lib$sig_to_ret handler takes over and returns a non-zero status to the
** caller of the dummy function.  So, if the dummy functions return zero, the
** the number is finite; if not, then the number is either infinite or NaN
** (indistinguishable on a VAX).  So, the two return(0)'s actually make sense.
** Note: we have to force the C compiler not to over optimize because we must
** access through the pointer.
*/
static STATUS
ffinite_internal(float *f)
{
    lib$establish(lib$sig_to_ret);

    if (*f)
	return(0);
    else
	return(0);
}

static STATUS
dfinite_internal(double *d)
{
    lib$establish(lib$sig_to_ret);

    if (*d)
	return(0);
    else
	return(0);
}

i4
FPffinite(float *f)
{
    return((ffinite_internal(f)) ? 0 : 1);
}

i4
FPdfinite(double *d)
{
    return((dfinite_internal(d)) ? 0 : 1);
}

/*{
** Name: FPatan() - Arctangent.
**
** Description:
**	Find the arctangent of x.
**	Returned result is in radians.
**
** Inputs:
**      x                               Pointer to the number to find the
**					arctangent of
**
** Outputs:
**	result				Pointer to the result
**
** Returns:
**	OK				If operation succeeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      21-apr-92 (rog)
**          Created from MHatan().
*/

STATUS
FPatan(double *x, double *result)
{
    STATUS	status;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else
    {
	*result = MTH$DATAN(x);
	status = OK;
    }
    return(status);
}


/*{
** Name: FPceil() - Ceiling.
**
** Description:
**	Returns the smallest integer not less than the
**	argument passed to it.  The return value, although
**	an integer, is returned as an double.
**
** Inputs:
**      x                               Pointer to the number to find ceiling of
**
** Outputs:
**	result				Pointer to the result
**
** Returns:
**	OK				If the operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      21-apr-92 (rog)
**          Created from MHceil().
*/

STATUS
FPceil(double *x, double *result)
{
    STATUS	status;
    
    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else
    {
	*result = FPfdint(x);

	if (*x > *result)
	{
	    *result += 1.0;
	}

	status = OK;
    }

    return(status);
}


/*{
** Name: FPcos() - Cosine.
**
** Description:
**	Find the cosine of x, where x is in radians.
**
** Inputs:
**      x                               Pointer to the number of radians to
**					take cosine of
**
** Outputs:
**	result				Pointer to the result
**
** Returns:
**	OK				If the operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      21-apr-92 (rog)
**          Created from MHcos().
*/

STATUS
FPcos(double *x, double *result)
{
    STATUS	status;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else
    {
	*result = MTH$DCOS(x);
	status = OK;
    }
    return(status);
}


/*{
** Name: FPexp() - Exponential.
**
** Description:
**	Find the exponential of x (i.e., e**x).
**
** Inputs:
**      x                               Pointer to number to find exponential of
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				If operation succeeded
**
**	FP_NOT_FINITE_ARGUMENT		when x is not a finite number
**	
**	FP_OVERFLOW			when the result would overflow
**
**	FP_UNDERFLOW			when the result would underflow
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	21-apr-1992 (rog)
**	    Created from MHexp().
*/

STATUS
FPexp(double *x, double *result)
{
    STATUS	status;
    i4	underflow = 1;
    
    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if (*x > FP_EXP_MAX)
    {
	status = FP_OVERFLOW;
    }
    else if (*x < FP_EXP_MIN)
    {
	status = FP_UNDERFLOW;
    }
    else
    {
	*result = MTH$DEXP(x);
	status = OK;
    }
    return(status);
}


/*{
** Name: FPipow() - Integer power.
**
** Description:
**	Find x**i.
**
** Inputs:
**      x                               Pointer to the number to raise to
**					a power.
**      i                               Integer power to raise it to.
**
** Outputs:
**	result				Pointer to the result.
**
** Returns:
**	OK				If operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
**	FP_IPOW_DOMAIN_ERROR		When x is 0.0 and i <= 0.
**
**	FP_OVERFLOW			when the result would overflow
**
**	FP_UNDERFLOW			when the result would underflow
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	22-apr-1992 (rog)
**          Created from MHipow().
*/

static STATUS
i_FPipow(double x, i4 i, double *result)
{
    i4	underflow = 1;

    lib$establish(lib$sig_to_ret);

    *result = OTS$POWDJ(x, i);
    return(OK);
}

STATUS
FPipow(double *x, i4 i, double *result)
{
    double	internal_result;
    STATUS	status;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if (*x == 0.0 && i <= 0)
    {
        status = FP_IPOW_DOMAIN_ERROR;
    }
    else
    {
	switch (i_FPipow(*x, i, &internal_result))
	{
	case OK:
		*result = internal_result;
		status = OK;
		break;
	case MTH$_FLOOVEMAT:
		status = FP_OVERFLOW;
		break;
	case MTH$_FLOUNDMAT:
		status = FP_UNDERFLOW;
		break;
	default:
		status = FP_IPOW_DOMAIN_ERROR;
		break;
	}
    }

    return(status);
}


/*{
** Name: FPln() - Natural log.
**
** Description:
**	Find the natural logarithm (ln) of x.
**	x must be greater than zero.
**
** Inputs:
**      x                               Pointer to the number to find log of.
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				If operation succeeded
**
**	FP_NOT_FINITE_ARGUMENT		when x is not a finite number
**	
**	FP_LN_DOMAIN_ERROR		when x is <= 0.0.
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	21-apr-1992 (rog)
**	    Created from MHln().
*/

STATUS
FPln(double *x, double *result)
{
    STATUS status;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if (*x <= 0.0)
    {
	status = FP_LN_DOMAIN_ERROR;
    }
    else
    {
	*result = MTH$DLOG(x);
	status = OK;
    }

    return(status);
}


/*{
** Name: FPpow() -- Raise to power of
**
** Description:
**	Find x**y.
**
** Inputs:
**      x                               Pointer to number to raise to a power.
**      y                               Pointer to the power to raise it to.
**
** Outputs:
**	result				Pointer to the result.
**
** Returns:
**	OK				If operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
**	FP_POW_DOMAIN_ERROR		When x is 0.0 and y <= 0 or
**					or x < 0.0 and y is not an integer.
**
**	FP_OVERFLOW			when the result would overflow
**
**	FP_UNDERFLOW			when the result would underflow
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	22-apr-1992 (rog)
**          Created from MHpow().
*/

static STATUS
i_FPpow(double x, double y, double *result)
{
    i4	underflow = 1;

    lib$establish(lib$sig_to_ret);

    *result = OTS$POWDD(x, y);
    return(OK);
}

STATUS
FPpow(double *x, double *y, double *result)
{
    double	internal_result;
    STATUS	status;

    if (!FPdfinite(x) || !FPdfinite(y))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if ((*x == 0.0 && *y <= 0.0) || (*x < 0.0 && (*y != FPfdint(y))))
    {
        status = FP_POW_DOMAIN_ERROR;
    }
    else
    {
	switch (i_FPpow(*x, *y, &internal_result))
	{
	case OK:
		*result = internal_result;
		status = OK;
		break;
	case MTH$_FLOOVEMAT:
		status = FP_OVERFLOW;
		break;
	case MTH$_FLOUNDMAT:
		status = FP_UNDERFLOW;
		break;
	default:
		status = FP_POW_DOMAIN_ERROR;
		break;
	}
    }

    return(status);
}

/*{
** Name: FPsin() - Sine.
**
** Description:
**	Find the sine of x, where x is in radians.
**
** Inputs:
**      x                               Pointer to the number of radians to
**					take sine of.
**
** Outputs:
**	result				Pointer to the result
**
** Returns:
**	OK				If the operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      22-apr-1992 (rog)
**          Created from MHsin().
*/

STATUS
FPsin(double *x, double *result)
{
    STATUS	status;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else
    {
	*result = MTH$DSIN(x);
	status = OK;
    }
    return(status);
}


/*{
** Name: FPsqrt() - Square root.
**
** Description:
**	Find the square root of x.
**	Only works if x is non-negative.
**
** Inputs:
**      x                               Pointer to the number to find square
**					root of.
**
** Outputs:
**      result				Pointer to the result.
**
** Returns:
**	OK				if the operation succeeded.
**	
**	FP_NOT_FINITE_ARGUMENT		when x is not a finite number
**	
**	FP_SQRT_DOMAIN_ERROR		when x is <= 0.0.
**
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      22-apr-1992 (rog)
**          Created from MHsqrt().
*/

STATUS
FPsqrt(double *x, double *result)
{
    STATUS status;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if (*x < 0.0)
    {
	status = FP_SQRT_DOMAIN_ERROR;
    }
    else
    {
	*result = MTH$DSQRT(x);
	status = OK;
    }

    return(status);
}


/*{
** Name: FPdmath() - Basic floating point math functions
**
** Description:
**	Find x+y, x-y, x*y & x/y.
**
** Inputs:
**	opp				Operation to perform
**      x & y				Numbers to add, subtract, multiply
**					or divide.
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				If operation succeeded
**
**	FP_NOT_FINITE_ARGUMENT		when x is not a finite number
**	
**	FP_DIVIDE_BY_ZERO		when y is equal to 0.0.
**	
**	FP_OVERFLOW			when the result would overflow
**
**	FP_UNDERFLOW			when the result would underflow
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	21-jul-1993 (smc/rog)
**	    Created.
*/

static STATUS
i_FPdmath( int opp, double x, double y, double *result )
{
    i4	underflow = 1;

    lib$establish(lib$sig_to_ret);

    switch(opp)
    {
	case FADD: *result = x + y; break;
	case FSUB: *result = x - y; break;
	case FMUL: *result = x * y; break;
	case FDIV: *result = x / y; break;
    }

    return(OK);
}

STATUS
FPdmath( int opp, double *x, double *y, double *result )
{
    STATUS	status;
    double	internal_result;

    if (!FPdfinite(x) || !FPdfinite(y))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if (opp == FDIV && *y == 0.0)
    {
        status = FP_DIVIDE_BY_ZERO;
    }
    else
    {
	switch (i_FPdmath(opp, *x, *y, &internal_result))
	{
	case OK:
		*result = internal_result;
		status = OK;
		break;
	case MTH$_FLOOVEMAT:
		status = FP_OVERFLOW;
		break;
	case MTH$_FLOUNDMAT:
		status = FP_UNDERFLOW;
		break;
	default:
		status = FP_POW_DOMAIN_ERROR;
		break;
	}
    }

    return(status);
}
