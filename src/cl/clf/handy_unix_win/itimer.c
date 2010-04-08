/*
**Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <clconfig.h>
# include <systypes.h>
# include <rusage.h>
# include <clitimer.h>
# include <clsigs.h>

/*
**
** History:
**
**	21-may-90 (blaise)
**	    Add <rusage.h> include to pickup correct ifdefs in <clitimer.h>.
**	22-may-90 (rog)
**	    Include <systypes.h> before <rusage.h> so that time_t is defined.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifdef	xCL_030_SETITIMER_EXISTS

i4	IIitimer_dummy;

#else

/*
** We don't handle intervals, and we only handle ITIMER_REAL.
*/

int
IIgetitimer( what, val )
int what;
register struct itimerval *val;
{
    register int ret;

# ifdef xCL_011_USE_SIGVEC
    register int omask = sigblock( sigmask( SIGALRM ) );
# else
    register int *sig = (int*)signal( SIGALRM, SIG_IGN );
# endif

    if( what != ITIMER_REAL || !val || 
    	val->it_interval.tv_usec || val->it_interval.tv_sec )
    {
	    TRdisplay("IIgetitimer: abort! due to invalid paramaters\n");
	    TRflush();
	    abort();
    }
    
    val->it_value.tv_usec = 0;
    
    /* get and reset seconds */
    val->it_value.tv_sec = ret = alarm( 0 );
    alarm( ret );

# ifdef xCL_011_USE_SIGVEC
    (void) sigsetmask( omask );
# else
    (void) signal( SIGALRM, sig );
# endif

    return( ret );
}

int
IIsetitimer( what, val, oval )
int what;
register struct itimerval *val, *oval;
{
    register int new;
    register int ret;
    
# ifdef xCL_011_USE_SIGVEC
    register int omask = sigblock( sigmask( SIGALRM ) );
# else
    register int *sig = (int *)signal( SIGALRM, SIG_IGN );
# endif
    
    if( what != ITIMER_REAL )
    {
	TRdisplay("IIsetitimer: abort! due to invalid paramaters\n");
	TRflush();
	abort();    
    }

    if( val )
    {
	/* we don't support intervals, only delays */
	if( val->it_interval.tv_sec || val->it_interval.tv_usec )
	{
	    TRdisplay("IIsetitimer: abort! intervals are not supported\n");
	    TRflush();
	    abort();
	}
	
	new = val->it_value.tv_sec + (val->it_value.tv_usec ? 1 : 0) ;
    }

    if( oval )
    {
	oval->it_interval.tv_usec = 0;
	oval->it_interval.tv_sec = 0;
	oval->it_value.tv_usec = 0;
	oval->it_value.tv_sec = alarm( 0 );
    }

    ret = val ? alarm( new ) : 0;

# ifdef xCL_011_USE_SIGVEC
    (void) sigsetmask( omask );
# else
    (void) signal( SIGALRM, sig );
# endif

    return( ret >= 0 ? 0 : ret );
}
#endif	/* xCL_030_SETITIMER_EXISTS */
