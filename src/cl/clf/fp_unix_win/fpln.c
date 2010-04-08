/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<errno.h>		/* header file for error checking */
# include	<fp.h>

# include	"fplocal.h"
# include	<ex.h>			/* header file for exceptions */

/*
** Name: FPLN.C - Natural log.
**
** Description:
**	Find the natural logarithm (ln) of x.
**	x must be greater than zero.
**
**      This file contains the following external routines:
**    
**	FPln()		   -  Natural log.
**
** History:
**	16-apr-1992 (rog)
**	    Created from mhln.c.
**      19-feb-1999 (matbe01)
**          Move include for ex.h to end of list. The check for "ifdef
**          EXCONTINUE" does no good if sys/context.h (via fp.h) is
**          included after ex.h.
**/


/*{
** Name: FPln() - Natural log.
**
** Description:
**	Find the natural logarithm (ln) of x.
**	x must be greater than zero.
**
** Inputs:
**      x                               Number to find log of.
**
** Outputs:
**      result				Pointer to the result.
**
** Returns:
**	OK				
**
**	FP_NOT_FINITE_ARGUMENT		when x is not a finite number.
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
**      16-apr-1992 (rog)
**          Created from MHln().
*/
STATUS
FPln(double *x, double *result)
{
	STATUS	status;

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
	    *result = log(*x);
	    status = OK;
	}

	return(status);
}
