
/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# include	<errno.h>		/* header file for error checking */
# include	<ex.h>			/* header file for exceptions */
# include	<cs.h>
# include	<me.h>
# include	<meprivate.h>
# include	<fp.h>
# include	<exinternal.h>

# include	"fplocal.h"

/**
** Name: FPEXP.C - Find the exponential of x (i.e., e**x).
**
** Description:
**	Find the exponential of x (i.e., e**x).
**
**      This file contains the following external routines:
**    
**	FPexp()		   -  Find the exponential of x (i.e., e**x).
**
** History:
**      16-apr-92 (rog)
**          Created from mhexp.c.
**      09-jun-1995 (canor01)
**          semaphore protect exception handler
**	03-jun-1996 (canor01)
**	    Put jmp_buf in thread-local storage when using operating-system
**	    threads.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions
**          from "MEtls..." to "ME_tls..." to avoid conflict with new
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototype for expsig.
**/

TYPESIG expsig(
        int signum,
        EX_SIGCODE      SIGCODE(sigcode),
        EX_SIGCONTEXT   SIGCONTEXT(sigcontext));

/* Externs */

/* statics */

/* return from floating point exception */

static jmp_buf expjmp;

# ifdef OS_THREADS_USED
static ME_TLS_KEY	FPexpkey = 0;
# endif /* OS_THREADS_USED */


/*{
** Name: FPexp() - Exponential.
**
** Description:
**	Find the exponential of x (i.e., e**x).
**
** Inputs:
**      x                               Number to find exponential for
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				if operation suceeded
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
**      16-apr-92 (rog)
**	    Created from MHexp().
*/
STATUS
FPexp(double *x, double *result)
{
    double	internal_result = 1.0;
    STATUS	status;
    TYPESIG	expsig();
    jmp_buf	*expjmp_ptr = NULL;

    if (!FPdfinite(x))
    {
	status = FP_NOT_FINITE_ARGUMENT;
    }
    else
    {
	status = FAIL;

# ifndef DESKTOP

# ifdef OS_THREADS_USED
	if ( FPexpkey == 0 )
	{
	    ME_tls_createkey( &FPexpkey, &status );
	    ME_tls_set( FPexpkey, NULL, &status );
	}
	if ( FPexpkey == 0 )
	{
	    /* not linked with threading libraries */
	    FPexpkey = -1;
	}
	if ( FPexpkey == -1 )
	{
	    expjmp_ptr = &expjmp;
	}
	else
	{
	    ME_tls_get( FPexpkey, (PTR *)&expjmp_ptr, &status );
	    if ( expjmp_ptr == NULL )
	    {
	        expjmp_ptr = (jmp_buf *) MEreqmem(0, sizeof(jmp_buf), 
						  TRUE, NULL );
	        ME_tls_set( FPexpkey, (PTR)expjmp_ptr, &status );
	    }
	}
# else /* OS_THREADS_USED */
	expjmp_ptr = &expjmp;
# endif /* OS_THREADS_USED */
        if (!setjmp(*expjmp_ptr))
	{
	    i_EXsetothersig(SIGFPE, expsig);

	    errno = NOERR;

	    internal_result = exp(*x);

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
	    internal_result = exp(*x);
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
expsig(signum, sigcode, sigcontext)
int		signum;
EX_SIGCODE	SIGCODE(sigcode);
EX_SIGCONTEXT	SIGCONTEXT(sigcontext);
{
	STATUS status;
	jmp_buf *expjmp_ptr = NULL;
# ifdef OS_THREADS_USED
	ME_tls_get( FPexpkey, (PTR *)&expjmp_ptr, &status );
# else /* OS_THREADS_USED */
	expjmp_ptr = &expjmp;
# endif /* OS_THREADS_USED */
	longjmp(*expjmp_ptr, 1);
}
