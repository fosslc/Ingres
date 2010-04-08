/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<errno.h>		/* header file for error checking */
# include	<fp.h>

# include	"fplocal.h"

/**
** Name: FPSQRT.C - Square root.
**
** Description:
**	Find the square root of x.
**
**      This file contains the following external routines:
**    
**	FPsqrt()	   -  Find the square root of x.
**
**      28-jan-92 (rog)
**          Created from mhsqrt.c.
**/


/*{
** Name: FPsqrt() - Square root.
**
** Description:
**	Find the square root of x.
**	Only works if x is non-negative.
**
** Inputs:
**      x                               Number to find square root of.
**
** Outputs:
**	result				Pointer to result.
**
** Returns:
**
**	OK      			if operation succeeded.
**
**	FP_NOT_FINITE_ARGUMENT		when x is not a finite number.
**
**	FP_SQRT_DOMAIN_ERROR		when x is < 0.0.
**
** Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      28-jan-92 (rog)
**          Created from MHsqrt().
*/
STATUS
FPsqrt(double *x, double *result)
{
	STATUS	status;

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
	*result = sqrt(*x);
	status = OK;
    }

    return(status);
}
