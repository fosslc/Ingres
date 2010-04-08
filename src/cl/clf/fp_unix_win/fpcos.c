/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<errno.h>		/* header file for error checking */
# include	<fp.h>

# include	"fplocal.h"

/*
** Name: FPCOS.C - Cosine
**
** Description:
**	Find the cosine of x, where x is in radians.
**
**      This file contains the following external routines:
**    
**	FPcos()		   -  Find the cos of a number.
**
** History:
**	16-apr-1992 (rog)
**	    Created from mhcos.c.
*/


/*{
** Name: FPcos() - Cosine.
**
** Description:
**	Find the cosine of x.
**	Returned result is in radians.
**
** Inputs:
**      x                               Number to find the cosine of.
**
** Outputs:
**      result				Point to the result
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
**      16-apr-1992 (rog)
**          Created from MHcos();
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
	    *result = cos(*x);
	    status = OK;
	}

	return(status);
}
