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
** Name: FPSIN.C - Sine.
**
** Description:
**	Find the sine of x, where x is in radians.
**
**      This file contains the following external routines:
**    
**	FPsin()	   	-  Sine.
**
** History:
**	16-apr-1992 (rog)
**	    Created from mhsin.c.
*/


/*{
** Name: FPsin() - Sine.
**
** Description:
**	Find the sine of x.
**	Returned result is in radians.
**
** Inputs:
**      x                               Number to find the sine of.
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
**          Created from MHsin();
**      19-feb-1999 (matbe01)
**          Move include for ex.h to end of list. The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via fp.h) is
**          included after ex.h.
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
	    *result = sin(*x);
	    status = OK;
	}

	return(status);
}
