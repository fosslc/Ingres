/*
** Copyright (c) 2004 Ingres Corporation 
** All Rights Reserved.
*/

# include <compat.h>
# include <clconfig.h>
# include <tm.h>
# include <cs.h>
# ifdef sqs_ptx
# include <sys/timers.h>
# endif /* sqs_ptx */


/*
** Name: TMhrnow
**
** Definition:
**      Get the current system time in high resolution (currently defined
**      to be a value in seconds and nanoseconds). On solaris, this function can
**      simply be defined to be clock_gettime() since the system structure,
**      timespec, actually matches the definition of HRSYSTIME, and can be 
**	defined to look the same. On some platforms, a TMhrnow() function may 
**      really need to be written, and should be placed in cl!clf!tm time.c
**      HRSYSTIME is defined to be:
**      struct HRSYSTIME
**      {
**          longnat     tv_sec;         ** number of secs since 1-jan-1970 **
**          longnat     tv_nsec;        ** nanoseconds **
**      };
**
**      Note: platforms that define this function to be a system function
**      should avoid declaring a prototype for it in tm.h, since this will be
**      macro'd out and cause syntax errors in the compile, and anyway it should
**      already be prototyped in the included system header. If you do need to
**      write a real version of this function, make sure you exclude the 
**      platforms that have it defined to a system call from any prototype you
**      add to tm.h
**
** History:
**      15-Mar-1999 (popri01)
**          Created, based on NT version.
**	29-mar-1999 (popri01)
**	    Support platforms which implement TMhrnow as a macro.
**	    This should also allow the code to be generic for other platforms
**	    (i.e., eliminate usl_us5 ifdef's).
**	02-apr-1999 (walro03)
**	    Support for Sequent (sqs_ptx)
** 
*/

#ifdef TMHRNOW_WRAPPED_CLOCKGETTIME
#undef TMhrnow
#define WRAPPED
#endif

#ifndef WRAPPED
#ifdef OS_THREADS_USED
static CS_SYNCH nanomutex;
#endif /* OS_THREADS_USED */
static HRSYSTIME lasttime ZERO_FILL;
static bool initialized = FALSE;
#endif /* WRAPPED */

STATUS
TMhrnow(HRSYSTIME *stime)
{
#if defined(sqs_ptx)
	struct timespec cur_syst;
#else
	SYSTIME cur_syst;
#endif /* sqs_ptx */

#ifdef TMHRNOW_WRAPPED_CLOCKGETTIME
	return clock_gettime( CLOCK_REALTIME, stime );
#endif

#ifndef WRAPPED

	if ( !initialized )
	{
		initialized = TRUE;
#ifdef OS_THREADS_USED
		CS_synch_init(&nanomutex);
#endif /* OS_THREADS_USED */
	}

#ifdef sqs_ptx
	getclock(TIMEOFDAY, &cur_syst);
	stime->tv_sec = cur_syst.tv_sec;
	stime->tv_nsec = cur_syst.tv_nsec;
#else
    	TMet(&cur_syst);
	stime->tv_sec = cur_syst.TM_secs;
	stime->tv_nsec = cur_syst.TM_msecs * NANO_PER_MILLI;
#endif /* sqs_ptx */

	/*
	** if we have been called twice within the same 
	** interval, increment the time by one nanosecond.
	*/
#ifdef OS_THREADS_USED
	CS_synch_lock(&nanomutex);
#endif /* OS_THREADS_USED */
	if ( stime->tv_sec == lasttime.tv_sec &&
	     stime->tv_nsec <= lasttime.tv_nsec )
	{
		stime->tv_nsec = lasttime.tv_nsec + 1;
	}
	lasttime.tv_sec = stime->tv_sec;
	lasttime.tv_nsec = stime->tv_nsec;
#ifdef OS_THREADS_USED
	CS_synch_unlock(&nanomutex);
#endif /* OS_THREADS_USED */
 
	return OK;
#endif /* WRAPPED */

}

