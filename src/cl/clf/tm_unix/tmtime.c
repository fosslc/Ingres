/*
**    Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<cs.h>
# include	<mu.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include	"tmlocal.h"

/**
** Name: TMTIME.C - Functions used by the TM CL module.
**
** Description:
**      This file contains the following tm routines:
**    
**      TMzone()        -  Return minutes of time wes of Greenwich.
**      TMnow()         -  Return seconds and milliseconds since 
**			   00:00:00 GMT, 1/1/1970.
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	10-nov-87 (mikem)
**          Initial integration of 6.0 changes into 50.04 cl to create 
**          6.0 baseline cl
**	06-jan-88 (anton)
**          tmlocal.h moved
**	05-aug-88 (roger)
** 	    UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
**	10-aug-88 (jeff)
** 	    added daveb's fixs for non-bsd
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	1-sep-89 (daveb)
**	    Quite a bit of DBMS code assumes that TMnow is going to return
**	    a unique id.  That's what TMtimestamp is for, but apparently
**	    it isn't used as widely as it should be.  So, we'll write this
**	    in terms of TMget_stamp, which happens to work because we know
**	    what TMget_stamp is really doing.
**	02-feb-90 (mikem)
**	    Added TMet() for performance debugging.  This is an internal to the
**	    UNIX CL and is not part of the official interface. 
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Change the method of testing whether daylight saving time is
**		currently in operation. Now call localtime() and use the value
**		of tm_isdst which it returns.
**	27-june-90 (jkb)
**	    remove backslash to continue to new line the compiler complains
**	    about it.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) to describe 2
**          versions of gettimeofday() available.
**      30-Nov-94 (nive)
**              For dg8_us5, chnaged the prototype definition for
**              gettimeofday to gettimeofday ( struct timeval *, struct
**              timezone * )
**      23-May-95 (walro03)
**              Reversed previous change (30-Nov-94).  A better way is to use
**              the correct prototype based on xCL_GETTIMEOFDAY_TIMEONLY vs.
**              xCL_GETTIMEOFDAY_TIME_AND_TZ.
**	07-Jun-95 (walro03)
**		Due to compile errors, removed change of 23-May-95.  Program
**		has compile errors on platforms with xCL_GETTIMEOFDAY_TIMEONLY.
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**	25-sept-95 (allmi01)
**		Fixed dgi_us5 support for changes made to DG-UX r4.10 fcs which hides tz_minuteswest and tz_dsttime
**		if not compile flags _KERNEL_SOURCE or  __bsd_tod for include/sys header time.h.
**	04-Dec-1995 (walro03/mosjo01)
**		Compile errors on DG/UX when # directive does not start in 
**		column 1.  Moved #ifdef, #else, #endif to col.1.
**	03-jun-1996 (canor01)
**	    Clean up semaphores used with operating-system threads.
**	29-jul-1997 (walro03)
**		Assorted compile, link, and runtime errors on Tandem NonStop
**		(ts2_us5) caused by useof _REENTRANT.  We must keep _REENTRANT
**		defined, but let's call the non-reentrant module.  Also moved
**		#if, #else, #endif to column 1.
**	14-Aug-1997 (toumi01)
**	    Move #ifdef etc. to column 1, as required by many compilers.
**      26-Aug-1997 (kosma01)
**              Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5,
**              to agree with arguments of gettimeofday() in <sys/time.h>.
**      09-sep-1998 (hweho01)
**              Extended the fix dated 25-sept-95 by allmi01 to dg8_us5. 
**	18-apr-1998 (canor01)
**	    Clean up use of _REENTRANT.  It was originally implemented as
**	    a placeholder for su4_us5, but somehow remained in the code, 
**	    causing problems on other platforms.
**	16-Nov-1998 (jenjo02)
**	    TMet() now called by CSMTsuspend for timed waits. Faults
**	    in code was causing 2 gettimeofday()'s to be executed,
**	    TM_msecs wasn't being set correctly.
**	22-Nov-1998 (kosma01)
**		Used existing xCL_GETTIMEOFDAY_TIMEONLY_V for sgi_us5.
**		This allows local prototype of gettimeofday to agree with
**		gettimeofday() prototype in <sys/time.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Oct-2002 (bonro01)
**	    Corrected invalid comment on endif line, that was causing
**	    compiler warnings.
**      15-Mar-2005 (bonro01)
**          Added support for Solaris AMD64 a64_sol.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/

/* # defines */

# define	MIN_PER_HR	60
# define	SEC_PER_MIN	60
# define	MICRO_PER_MILLI	1000

/* External variables */

# ifdef OS_THREADS_USED
static MU_SEMAPHORE	TMet_sem;
static bool TMet_sem_init;
# endif /* OS_THREADS_USED */

/*{
** Name: TMzone	- Return minutes to Greenwich
**
** Description:
**	Local time zone (in minutes of time westward from Greenwich).
**	This cannot be static, since it might change during an
**	INGRES session.
**
**	On systems without gettimeofday (sysV), uses the externs
**	timezone, and daylight.
**
** Inputs:
**
** Outputs:
**      zone                            local time zone in minutes
**	    				westward from Greenwich.
**	Returns:
**	    
**	Exceptions:
**	    none.
**
** Side Effects:
**	    none.
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**      06-jul-1987 (mmm)
**          Updated for jupiter unix cl.
**	28-Jul-1988 (daveb)
**	    Eliminate FTIME ifdef.  Note that the whole concept of TMzone
**	    is fatally flawed, since it assumes that the timezone correction
**	    we are returning is correct for all time.  In fact, it is only
**	    correct for "now".  It needs two more arguments, the time to be
**	    corrected, and the timezone.  (In a server, we can't assume that
**	    the timezone of the server process is right for the thread that
**	    is running the query.).
**	07-mar-89 (russ)
**	    Changed ifdef BSD42 to the more general xCL_005_GETTIMEOFDAY_EXISTS
**	    define.
**	27-june-90 (jkb)
**	    remove backslash to continue to new line the compiler complains
**	    about it.
**	20-apr-94 (mikem)
**	    Changed TMzone() to use new ifdef "xCL_GETTIMEOFDAY_TIME_AND_TZ".
**	    Only on systems where gettimeofday exists and it correctly supports
**	    time zone information will it be used.
**	23-may-95 (tutto01)
**	    Changed localtime call to the reentrant localtime_r.
**	23-Sep-2005 (hanje04)
**	    Quiet compiler warnings
*/

VOID
TMzone(zone)
i4	*zone;
{
# if defined (xCL_GETTIMEOFDAY_TIME_AND_TZ) || \
     defined (xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
 
	struct timeval	t;
	struct timezone tz;
	struct tm	*tm;
	struct tm	localtime_res;

	gettimeofday(&t, &tz);

# if defined(sparc_sol) || defined(a64_sol)
	localtime_r(&t.tv_sec, &localtime_res);
	*zone = (i4) (tz.tz_minuteswest - (localtime_res.tm_isdst ? MIN_PER_HR : 0));
# else
	tm = localtime( (time_t *)&t.tv_sec );
# endif /* solaris */
#if defined(dgi_us5) || defined(dg8_us5)
	*zone = (i4) (tz.__hide__tz_minuteswest - ( tm->tm_isdst ? MIN_PER_HR : 0 ));
#else
	*zone = (i4) (tz.tz_minuteswest - ( tm->tm_isdst ? MIN_PER_HR : 0 ));
#endif /* dgi_us5 */
 
# else

	extern long	timezone;
	extern int 	daylight;
        struct tm       *tm;
	struct tm 	localtime_res;
        time_t          secs;
 
	tzset();
        secs = time((time_t*)0);

# if defined(sparc_sol) || defined(a64_sol)
	localtime_r(&secs, &localtime_res);
        *zone = (i4) (timezone/SEC_PER_MIN - (localtime_res.tm_isdst ? MIN_PER_HR : 0));
# else
        tm = localtime( &secs );
        *zone = (i4) (timezone/SEC_PER_MIN - (tm->tm_isdst ? MIN_PER_HR : 0));
# endif /* solaris */
 
# endif	/* xCL_005_GETTIMEOFDAY_EXISTS */
}


/*{
** Name: TMnow	- Current time
**
** Description:
**	This routine simply returns the number of seconds since
**	00:00:00 GMT, Jan. 1, 1970.  
**
**	Return seconds and milliseconds since 00:00:00 GMT, 1/1/1970.
**	On sysV machines, milliseconds field is always 0.
**
** Inputs:
**
** Outputs:
**      stime                           current UCT in seconds since 1/1/70
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**      06-jul-87 (mmm)
**          initial unix jupiter cl.
**	1-sep-89 (daveb)
**	    Quite a bit of DBMS code assumes that TMnow is going to return
**	    a unique id.  That's what TMtimestamp is for, but apparently
**	    it isn't used as widely as it should be.  So, we'll write this
**	    in terms of TMget_stamp, which happens to work because we know
**	    what TMget_stamp is really doing.
*/

VOID
TMnow(stime)
SYSTIME	*stime;
{
	TMget_stamp( (TM_STAMP *) stime );
}

/*{
** Name: TMet	- Current time (really)
**
** Description:
**	This routine simply returns the number of seconds since
**	00:00:00 GMT, Jan. 1, 1970.  
**
**	Return seconds and milliseconds since 00:00:00 GMT, 1/1/1970.
**	On sysV machines, milliseconds field is always 0.
**
**	This function is not an approved interface to the CL.  It is only to
**	be used internally by the UNIX CL for debugging purposes (currently
**	used for benchmark debugging - testing exactly elapsed time during
**	sub-second pauses in the dbms, rcp, ...).  It is used instead of 
**	TMnow() because of daveb's change to TMnow which salts the time with
**	the pid to workaround a misuse of TMnow() by the dbms.
**
** Inputs:
**
** Outputs:
**      stime                           current UCT in seconds since 1/1/70
**
**	Returns:
**	    VOID
** History:
**	02-feb-90 (mikem)
**	    created.
**      20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) in TMet() to describe 2
**          versions of gettimeofday() available.
**	16-Nov-1998 (jenjo02)
**	    TMet() now called by CSMTsuspend for timed waits. Faults
**	    in code was causing 2 gettimeofday()'s to be executed,
**	    TM_msecs wasn't being set correctly.
**	04-Apr-2001 (bonro01)
**	    Correct millisecond time increment for code that does not
**	    use gettimeofday() function.  There was some confusion
**	    about whether this routine returned microseconds or milliseconds.
*/

VOID
TMet(stime)
SYSTIME	*stime;
{

#   ifdef xCL_005_GETTIMEOFDAY_EXISTS
    {
        struct timeval  t;
	struct timezone tz;
# if defined(dg8_us5) || defined(dgi_us5)
        int gettimeofday ( struct timeval *, struct timezone * ) ;
# else
# if defined(xCL_GETTIMEOFDAY_TIMEONLY_V)
    int gettimeofday ( struct timeval *, ...) ;
# else
	int		gettimeofday();
# endif /* xCL_GETTIMEOFDAY_TIMEONLY_V */
# endif

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
	gettimeofday(&t);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
	gettimeofday(&t, &tz);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
	gettimeofday(&t, &tz);
# endif
	stime->TM_secs = t.tv_sec;
	stime->TM_msecs = t.tv_usec / 1000;
    }
#   else
    {

	static i4      seconds = 0;
	static i4      msecs   = 0;

	stime->TM_secs = time(0);


	/* DMF code expects timestamps to be of finer granularity than
	** a second.  In order to fake this on UNIX's with this granularity
	** we will make the timestamps monatomically increasing within
	** a process.
	*/   
#ifdef OS_THREADS_USED
	if ( !TMet_sem_init )
	    MUw_semaphore( &TMet_sem, "TM et sem" );
	MUp_semaphore( &TMet_sem );
#endif /* OS_THREADS_USED */
	if (stime->TM_secs == seconds)
	{
	    if(++msecs >= 1000)
	    {
	        stime->TM_secs = ++seconds;
	        msecs=0;
	    }

	}
	else 
	{
	    seconds = stime->TM_secs;
	    msecs = 0;
	}    
	stime->TM_msecs = msecs;
#ifdef OS_THREADS_USED
	MUv_semaphore( &TMet_sem );
#endif /* OS_THREADS_USED */
    }
#   endif /* xCL_005_GETTIMEOFDAY_EXISTS */
	
    return;
}
