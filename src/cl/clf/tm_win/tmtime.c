/*
**    Copyright (c) 1987, Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<cs.h>
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
**	32-may-95 (emmag)
**	    Desktop porting changes. 
**	18-jul-95 (canor01)
**	    semaphore protect globals for MCT only
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**/

/* # defines */

# define	MIN_PER_HR	60
# define	SEC_PER_MIN	60
# define	MICRO_PER_MILLI	1000

/* External variables */

GLOBALREF CS_SEMAPHORE  CL_misc_sem;


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
*/
# ifdef NT_GENERIC
# define timezone _timezone
# endif

VOID
TMzone(zone)
i4	*zone;
{
# ifdef xCL_GETTIMEOFDAY_TIME_AND_TZ
 
	struct timeval t;
	struct timezone tz;
	struct tm *tm;

	gettimeofday(&t, &tz);
	tm = localtime( &t.tv_sec );
	*zone = (i4) (tz.tz_minuteswest - ( tm->tm_isdst ? MIN_PER_HR : 0 ));
 
# else
 
	extern long timezone;
	extern int daylight;
        struct tm       *tm;
        time_t          secs;
 
	tzset();
        secs = time((time_t*)0);
        tm = localtime( &secs );
        *zone = (i4) (timezone/SEC_PER_MIN - ( tm->tm_isdst ? MIN_PER_HR : 0 ));
 
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
**	Return seconds and microseconds since 00:00:00 GMT, 1/1/1970.
**	On sysV machines, microseconds field is always 0.
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
*/

VOID
TMet(stime)
SYSTIME	*stime;
{

#   ifdef xCL_005_GETTIMEOFDAY_EXISTS
    {
        struct timeval  t;
	struct timezone tz;
# ifdef dg8_us5
        int gettimeofday ( struct timeval *, struct timezone * ) ;
# else
	int		gettimeofday();
# endif

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
	gettimeofday(&t);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
	gettimeofday(&t, &tz);
# endif

	gettimeofday(&t, &tz);
	stime->TM_secs = t.tv_sec;
	stime->TM_msecs = t.tv_usec;
    }
#   else
    {

	static i4      seconds = 0;
	static i4      usecs   = 0;

	stime->TM_secs = time(0);


	/* DMF code expects timestamps to be of finer granularity than
	** a second.  In order to fake this on UNIX's with this granularity
	** we will make the timestamps monatomically increasing within
	** a process.  We increase 1000 at a time because the timestamp that
	** actually gets printed is only in 100th's of a second.
	*/   
# ifdef MCT
	gen_Psem(&CL_misc_sem);
# endif /* MCT */
	if (stime->TM_secs == seconds)
	{
	    usecs += 1000;
	}
	else 
	{
	    seconds = stime->TM_secs;
	    usecs = 0;
	}    
	stime->TM_msecs = usecs;
# ifdef MCT
        gen_Vsem(&CL_misc_sem); 
# endif /* MCT */
    }
#   endif /* xCL_005_GETTIMEOFDAY_EXISTS */
	
    return;
}
