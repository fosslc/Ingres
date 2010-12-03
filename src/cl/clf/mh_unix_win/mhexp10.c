/*
**  Copyright (c) 1987, 2004 Ingres Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<gl.h>		/* header file for compatibility lib */
# include	<systypes.h>
# include	<clconfig.h>		/* for TYPESIG */
# include	<math.h>		/* header file for math package */
# include	<errno.h>		/* header file for error checking */
# include	<clsigs.h>		/* header file for signal definitions */
# include	<ex.h>			/* header file for exceptions */
# include	<cs.h>
# include	<me.h>
# include	<meprivate.h>
# include	<mh.h>
# include	"mhlocal.h"

# if defined(i64_win)
# pragma optimize("", off)
# endif

/**
** Name: MHEXP10.C - Ten to a power.
**
** Description:
**	Find 10**x.
**
**      This file contains the following external routines:
**    
**	MHexp10()	   -  Find 10**x.
**
**      This file contains the following external routines:
**
**	exp10sig()	   -  Catch the exception.
**
** History:
 * Revision 1.2  90/05/24  15:00:29  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:54  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:43:52  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:55:14  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:15:58  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:56  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.3  89/01/05  19:29:02  mikem
 * new definitions of signal handlers.
 * 
 * Revision 1.2  88/10/31  11:07:41  jeff
 * mikem checking in changes for jeff:
 * 	a bunch of mh module changes.  Most dealing with making signals 
 *      raised for math signals match up across vms and unix (ie. return
 * 	what the adf mainline is expecting.)  
 * 
 * Revision 1.1  88/08/05  13:42:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/13  13:51:47  mikem
**      changed to use "mhlocal.h" rather than "mh.h" do stop possible confusion
**      
**      Revision 1.2  87/11/10  13:55:31  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/21  17:31:06  mikem
**      Updated to Jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
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
**	15-oct-1998 (walro03)
**	    Detect overflow condition by calling finite() on AIX (rs4_us5).
**      20-oct-98 (muhpa01)
**          Determine if result overflowed by calling isfinite() on hpb_us5
**          (HP-UX 11.00).
**      12-may-99 (hweho01)
**          Determine if result overflowed by calling finite() on ris_u64
**          (AIX 64-bit).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Aug-2001 (hanje04)
**          Determine if result overflowed by calling finite() on Linux.
**          (Intel, Alpha and s390).
**	04-Dec-2001 (hanje04)
**	    IA64 Linux to use finite().
**	12-dec-2001 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent "createdb" from causing
**	    the DBMS server to SEGV.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      09-May-2005 (hanal04) Bug 114470
**          Submission of open files on nc4_us5 build. errno must be declared.
**	01-Nov-2007 (hanje04)
**	    SIR 114907
**	    OSX defines xCL_FINITE_EXISTS but should use isfinite()
**	08-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototype for exp10sig.
**/

/* # defines */
/* typedefs */
/* forward references */
TYPESIG exp10sig(i4 signal);

/* externs */

/* statics */

static jmp_buf exp10jmp;

# ifdef OS_THREADS_USED
static ME_TLS_KEY       MHexp10key = 0;
# endif /* OS_THREADS_USED */



/*{
** Name: MHexp10() - Ten to a power.
**
** Description:
**	Find 10**x.
**
** 	UNIX math package will not handle case where
** 	x is negative and non-integral.  It returns
** 	0 and set errno to EDOM.
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
**      21-jul-87 (mmm)
**          Upgraded UNIX cl to Jupiter coding standards.
**	24-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Change include <signal.h> to <clsigs.h>;
**		Silence warnings by using TYPESIG.
**	30-dec-92 (mikem)
**	    su4_us5 port.  Fixed cast of 1st argument to setjmp() - it should
**	    be a jmp_buf not a jmp_buf *.  This generated a compiler warning
**	    on su4_us5.
**	09-jun-1995 (canor01)
**	    semaphore protect exception handling
**      21-jun-2002 (huazh01 for hayke02)
**          For nc4_us5 declare errno locally. 
**          This change fixes bug 103611.
*/

f8
MHexp10(x)
f8	x;
{
	f8	result;
	STATUS  status;
	TYPESIG	exp10sig(i4 signal);
	TYPESIG	(*oldsig)(i4 sig);
	jmp_buf *exp10jmp_ptr;
#ifdef nc4_us5
	i4	errno = NOERR;
#else
	errno = NOERR;
#endif

#ifdef nc4_us5
        i4      errno = NOERR;
#else
        errno = NOERR;
#endif

	oldsig = signal( SIGFPE, exp10sig );

# ifndef DESKTOP

# ifdef OS_THREADS_USED
        if ( MHexp10key == 0 )
        {
            ME_tls_createkey( &MHexp10key, &status );
            ME_tls_set( MHexp10key, NULL, &status );
        }
        if ( MHexp10key == 0 )
        {
	    /* not linked with threading libraries */
	    MHexp10key = -1;
	}
	if ( MHexp10key == -1 )
	{
            exp10jmp_ptr = &exp10jmp;
	}
	else
	{
            ME_tls_get( MHexp10key, (PTR *)&exp10jmp_ptr, &status );
            if ( exp10jmp_ptr == NULL )
            {
                exp10jmp_ptr = (jmp_buf *) MEreqmem(0, sizeof(jmp_buf), 
						    TRUE, NULL );
                ME_tls_set( MHexp10key, (PTR)exp10jmp_ptr, &status );
            }
	}
# else /* OS_THREADS_USED */
        exp10jmp_ptr = &exp10jmp;
# endif /* OS_THREADS_USED */

	if( setjmp( *exp10jmp_ptr ) )
		errno = EDOM;
	else
		result = pow(10.0, x);

	(void) signal( SIGFPE, oldsig );

	switch(errno)
	{
		case NOERR:
# if defined(any_hpux) || defined(OSX)
			if (!isfinite(result))
			    EXsignal(EXFLTOVF, 0);
# endif
# ifdef	SUN_FINITE
			if (!finite(result))
			    EXsignal(EXFLTOVF, 0);
# endif
# if defined(any_aix) || defined(LNX)
			if (!finite(result))
			    EXsignal(EXFLTOVF, 0);
# endif
			return(result);
			break;
		case EDOM:
			EXsignal(MH_BADARG, 0);
			result = 0.0;
			break;
		case ERANGE:
			EXsignal(EXFLTOVF, 0);
			result = 0.0;
			break;
		default:
			EXsignal(MH_INTERNERR, 0);
			result = 0.0;
			break;
	}
# else /* DESKTOP */
__try
{
		result = pow(10.0, x);
}
__except (EXCEPTION_EXECUTE_HANDLER)
{
        switch (GetExceptionCode())
        {
                case EXCEPTION_FLT_OVERFLOW:
                        EXsignal(EXFLTOVF, 0);
			break;
                default:
                        EXsignal(MH_INTERNERR, 0);
			break;
        }
	result = 0.0;
}
# endif /* DESKTOP */
	return(result);
}

/* catch the floating point exception and raise flag */

TYPESIG
exp10sig( signal )
i4  signal;
{
	STATUS  status;
	jmp_buf *exp10jmp_ptr;

# ifdef OS_THREADS_USED
        ME_tls_get( MHexp10key, (PTR *)&exp10jmp_ptr, &status );
# else /* OS_THREADS_USED */
        exp10jmp_ptr = &exp10jmp;
# endif /* OS_THREADS_USED */
	longjmp( *exp10jmp_ptr, 1 );
}
