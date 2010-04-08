/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>
# include	<errno.h>		/* header file for error checking */
# include	<clsigs.h>		/* header file for signal definitions */
# include	<ex.h>			/* header file for exceptions */
# include       <cs.h>
# include       <me.h>
# include	<meprivate.h>
# include	<fp.h>

# include	"fplocal.h"

/**
** Name: FPMATH.C - basic floating point maths functions.
**
** Description:
**
** 	Apply basic floating point mathmatical functions.
**
**      This file contains the following external routines:
**    
**	FPdmath()	   -  Find x+y, x-y, x*y & x/y.
**
** History:
**	19-jul-1992 (smc)
**	    Created.
**	09-jun-1995 (canor01)
**	    semaphore protect exception handler
**      03-jun-1996 (canor01)
**          Put jmp_buf in thread-local storage when using operating-system
**          threads.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**/

/* Externs */

/* statics */

/* return from floating point exception */

static jmp_buf fpmathjmp;

# ifdef OS_THREADS_USED
static ME_TLS_KEY       FPfpmathkey = 0;
# endif /* OS_THREADS_USED */


/*{
** Name: FPdmath() - apply basic math functions to a pair of floating
**                   point numbers.
**
** Description:
**	Find x+y, x-y, x*y & x/y.
**
** Inputs:
**      op				Operation to perform.
**      x & y                           Numbers to add, subtract, multiply
**                                      or divide.
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				If operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
**	FP_OVERFLOW			When the result would overflow
**
**	FP_UNDERFLOW			When the result would underflow
**
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
*/

STATUS
FPdmath(int op, double *x, double *y, double *result)
{
    double	internal_result;
    STATUS	status;
    TYPESIG	fpmathsig();
    jmp_buf	*fpmathjmp_ptr = NULL;

    if (!FPdfinite(x) || !FPdfinite(y))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else if (op == FDIV && *y == 0.0)
    {
	status = FP_DIVIDE_BY_ZERO;
    }
    else
    {
	status = FAIL;

# ifndef DESKTOP

# ifdef OS_THREADS_USED
        if ( FPfpmathkey == 0 )
        {
            ME_tls_createkey( &FPfpmathkey, &status );
            ME_tls_set( FPfpmathkey, NULL, &status );
        }
        if ( FPfpmathkey == 0 )
        {
            /* not linked with threading libraries */
            FPfpmathkey = -1;
        }
        if ( FPfpmathkey == -1 )
        {
            fpmathjmp_ptr = &fpmathjmp;
        }
        else
        {
            ME_tls_get( FPfpmathkey, (PTR *)&fpmathjmp_ptr, &status );
            if ( fpmathjmp_ptr == NULL )
            {
                fpmathjmp_ptr = (jmp_buf *) MEreqmem( 0, sizeof(jmp_buf), 
						      TRUE, NULL );
                ME_tls_set( FPfpmathkey, (PTR)fpmathjmp_ptr, &status );
            }
	}
# else /* OS_THREADS_USED */
        fpmathjmp_ptr = &fpmathjmp;
# endif /* OS_THREADS_USED */

        if (!setjmp(*fpmathjmp_ptr))
	{
	    i_EXsetothersig(SIGFPE, fpmathsig);

	    errno = NOERR;

	    switch(op)
	    {
	        case FADD: internal_result = *x + *y; break;
		case FSUB: internal_result = *x - *y; break;
		case FMUL: internal_result = *x * *y; break;
		case FDIV: internal_result = *x / *y; break;
	    }

	    /* Check to make sure the result is a finite number */
	    if (errno == NOERR && FPdfinite(&internal_result))
	    {
		*result = internal_result;
		status = OK;
	    }
	}

	if (status != OK)
	    status = (internal_result == 0.0) ? FP_UNDERFLOW : FP_OVERFLOW;

	i_EXrestoreorigsig(SIGFPE);
# else /* DESKTOP */
__try
{
	    switch(op)
	    {
	        case FADD: internal_result = *x + *y; break;
		case FSUB: internal_result = *x - *y; break;
		case FMUL: internal_result = *x * *y; break;
		case FDIV: internal_result = *x / *y; break;
	    }

	    /* Check to make sure the result is a finite number */
	    if (FPdfinite(&internal_result))
	    {
		*result = internal_result;
		status = OK;
	    }
}
__except (EXCEPTION_EXECUTE_HANDLER)
{
        switch (GetExceptionCode())
        {
                case EXCEPTION_FLT_OVERFLOW:
                        status = FP_OVERFLOW;
			break;
                case EXCEPTION_FLT_UNDERFLOW:
                        status = FP_UNDERFLOW;
			break;
                default:
                        status = FP_OVERFLOW;
			break;
        }
}
# endif /* DESKTOP */
    }

    return(status);
}

/* catch the floating point exception and raise flag */

TYPESIG
fpmathsig(signum, sigcode, sigcontext)
int		signum;
EX_SIGCODE	SIGCODE(sigcode);
EX_SIGCONTEXT	SIGCONTEXT(sigcontext);
{
	STATUS status;
	jmp_buf *fpmathjmp_ptr;
# ifdef OS_THREADS_USED
        ME_tls_get( FPfpmathkey, (PTR *)&fpmathjmp_ptr, &status );
# else /* OS_THREADS_USED */
        fpmathjmp_ptr = &fpmathjmp;
# endif /* OS_THREADS_USED */
	longjmp(*fpmathjmp_ptr, 1);
}

