/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>
# include	<errno.h>		/* header file for error checking */
# include	<clsigs.h>
# include	<ex.h>			/* header file for exceptions */
# include	<cs.h>
# include	<me.h>
# include	<meprivate.h>
# include	<fp.h>
# include	<exinternal.h>

# include	"fplocal.h"

/**
** Name: FPIPOW.C - Integer power.
**
** Description:
**	Find x**i.
**
**      This file contains the following external routines:
**    
**	FPipow()	   -  Integer power.
**
** History:
**	16-apr-1992 (rog)
**	    Created from mhipow.c.
**	20-sep-1993 (rog)
**	    Re-named ipowsig to IIfpipowsig so that it doesn't conflict 
**	    with similarly named functions in MH.
**      09-jun-1995 (canor01)
**          semaphore protect exception handler
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
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototype for IIFPipowsig.
**/

TYPESIG IIFPipowsig(
        int signum,
        EX_SIGCODE      SIGCODE(sigcode),
        EX_SIGCONTEXT   SIGCONTEXT(sigcontext));

/* Externs */

/* statics */

/* return from floating point exception */

static jmp_buf ipowjmp;

# ifdef OS_THREADS_USED
static ME_TLS_KEY       FPipowkey = 0;
# endif /* OS_THREADS_USED */


/*{
** Name: FPipow() - Integer power.
**
** Description:
**	Find x**i.
**
** Inputs:
**      x                               Number to raise to a power.
**      i                               Integer power to raise it to.
**
** Outputs:
**      result				Pointer to the result
**
** Returns:
**	OK				If operation suceeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
**	FP_IPOW_DOMAIN_ERROR		When x is 0.0 and i <= 0.
**
**	FP_OVERFLOW			When the result would overflow
**
**	FP_UNDERFLOW			When the result would underflow
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	16-apr-1992 (rog)
**	    Created from MHipow().
**	16-Aug-1993 (fredv)
**	    Moved <clsigs.h> before <ex.h> to avoid redefining EXCONTINUE,
**	    which is also defined in /usr/include/sys/context.h, on ris_us5.
*/
STATUS
FPipow(double *x, i4 i, double *result)
{
    double	internal_result = 1.0;
    STATUS	status;
    TYPESIG	IIFPipowsig();
    jmp_buf	*ipowjmp_ptr = NULL;

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
	status = FAIL;

# ifndef DESKTOP

# ifdef OS_THREADS_USED
        if ( FPipowkey == 0 )
        {
            ME_tls_createkey( &FPipowkey, &status );
            ME_tls_set( FPipowkey, NULL, &status );
        }
        if ( FPipowkey == 0 )
        {
	    /* not linked with threading libraries */
	    FPipowkey = -1;
	}
        if ( FPipowkey == -1 )
        {
            ipowjmp_ptr = &ipowjmp;
	}
	else
	{
            ME_tls_get( FPipowkey, &ipowjmp_ptr, &status );
            if ( ipowjmp_ptr == NULL )
            {
                ipowjmp_ptr = (jmp_buf *) MEreqmem( 0, sizeof(jmp_buf), 
						    TRUE, NULL );
                ME_tls_set( FPipowkey, (PTR)ipowjmp_ptr, &status );
            }
	}
# else /* OS_THREADS_USED */
        ipowjmp_ptr = &ipowjmp;
# endif /* OS_THREADS_USED */

        if (!setjmp(*ipowjmp_ptr))
	{
	    i_EXsetothersig(SIGFPE, IIFPipowsig);

	    errno = NOERR;

	    internal_result = pow(*x, (double) i);

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
# else  /* DESKTOP */
__try
{
	    internal_result = pow(*x, (double) i);

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
IIFPipowsig(signum, sigcode, sigcontext)
int		signum;
EX_SIGCODE	SIGCODE(sigcode);
EX_SIGCONTEXT	SIGCONTEXT(sigcontext);
{
	STATUS status;
        jmp_buf *ipowjmp_ptr = NULL;
# ifdef OS_THREADS_USED
        ME_tls_get( FPipowkey, &ipowjmp_ptr, &status );
# else /* OS_THREADS_USED */
        ipowjmp_ptr = &ipowjmp;
# endif /* OS_THREADS_USED */
	longjmp(*ipowjmp_ptr, 1);
}
