/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>
# include	<errno.h>		/* header file for error checking */
# include	<clsigs.h>		/* header file for signal definitions */
# include	<ex.h>			/* header file for exceptions */
# include	<cs.h>
# include	<me.h>
# include	<meprivate.h>
# include	<fp.h>
# include	<exinternal.h>

# include	"fplocal.h"

/**
** Name: FPPOW.C - raise number to a power.
**
** Description:
**
** 	raise number to a power.  Find x**y.
**
**      This file contains the following external routines:
**    
**	FPpow()		   -  Find x**y.
**
** History:
**	16-apr-1992 (rog)
**	    Created from mhpow.c.
**	20-sep-1993 (rog)
**	    Re-named powsig to IIfppowsig so that it doesn't conflict 
**	    with similarly named functions in MH.
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
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**      29-Nov-2010 (frima01) SIR 124685
**          Added prototype for IIfppowsig.
**/

/* Externs */

/* statics */

/* prototypes */

TYPESIG IIfppowsig(
	int		signum,
	EX_SIGCODE	SIGCODE(sigcode),
	EX_SIGCONTEXT	SIGCONTEXT(sigcontext)
);

/* return from floating point exception */

static jmp_buf powjmp;

# ifdef OS_THREADS_USED
static ME_TLS_KEY       FPpowkey = 0;
# endif /* OS_THREADS_USED */


/*{
** Name: FPpow() - raise number to a power.
**
** Description:
**	Find x**y.
**
** Inputs:
**      x                               Number to raise to a power.
**      y                               Power to raise it to.
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				If operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
**	FP_POW_DOMAIN_ERROR		When x is 0.0 and y <= 0 or
**					or x < 0.0 and y is not an integer.
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
** History:
**	16-apr-1992 (rog)
**	    Created from MHpow().
*/

STATUS
FPpow(double *x, double *y, double *result)
{
    double	internal_result;
    STATUS	status;
    TYPESIG	IIfppowsig();
    jmp_buf 	*powjmp_ptr;

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
	status = FAIL;

# ifndef DESKTOP

# ifdef OS_THREADS_USED
        if ( FPpowkey == 0 )
        {
            ME_tls_createkey( &FPpowkey, &status );
            ME_tls_set( FPpowkey, NULL, &status );
        }
        if ( FPpowkey == 0 )
        {
            /* not linked with threading libraries */
            FPpowkey = -1;
        }
        if ( FPpowkey == -1 )
        {
            powjmp_ptr = &powjmp;
        }
        else
        {
            ME_tls_get( FPpowkey, (PTR *)&powjmp_ptr, &status );
            if ( powjmp_ptr == NULL )
            {
                powjmp_ptr = (jmp_buf *) MEreqmem( 0, sizeof(jmp_buf), 
						   TRUE, NULL );
                ME_tls_set( FPpowkey, (PTR)powjmp_ptr, &status );
            }
	}
# else /* OS_THREADS_USED */
        powjmp_ptr = &powjmp;
# endif /* OS_THREADS_USED */

        if (!setjmp(*powjmp_ptr))
	{
	    i_EXsetothersig(SIGFPE, IIfppowsig);

	    errno = NOERR;

	    internal_result = pow(*x, *y);

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
	    internal_result = pow(*x, *y);

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
IIfppowsig(signum, sigcode, sigcontext)
int		signum;
EX_SIGCODE	SIGCODE(sigcode);
EX_SIGCONTEXT	SIGCONTEXT(sigcontext);
{
    STATUS	status;
    jmp_buf 	*powjmp_ptr;

# ifdef OS_THREADS_USED
    ME_tls_get( FPpowkey, (PTR *)&powjmp_ptr, &status );
# else /* OS_THREADS_USED */
    powjmp_ptr = &powjmp;
# endif /* OS_THREADS_USED */
    longjmp(*powjmp_ptr, 1);
}
