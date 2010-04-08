/*
** Copyright (c) 1998 Ingres Corporation 
** All Rights Reserved.
*/

# include <compat.h>
# include <clconfig.h>
# include <tm.h>
# include <cs.h>

/*
** Name: TMhrnow
**
** Definition:
**      Get the current system time in high resolution (currently defined
**      to be a value in seconds and nanoseconds). On solaris, this function can
**      simply be defined to be clock_gettime() since the system structure,
**      timespec, actually matches the definition of HRSYSTIME, and can be defin
ed
**      to look the same. On some platforms, a TMhrnow() function may realy
**      need to be written, and should be placed in cl!clf!tm time.c
**      HRSYSTIME is defined to be:
**      struct HRSYSTIME
**      {
**          i4          tv_sec;         ** number of secs since 1-jan-1970 **
**          i4          tv_nsec;        ** nanoseconds **
**      };
**
**      Note: platforms that define this function to be a system function
**      should avoid declaring a prototype for it in tm.h, since this will be
**      macro'd out and cause syntax errors in the compile, and anyway, it shoul
d
**      already be prototyped in the included system header. If you do need to
**      write a real version of this function, make sure you exclude the plaufor
ms
**      that have it defined to a system call from any prototype you add to
**      tm.h
**
** History:
**	14-may-99 (abbjo03)
**		Change interval calculation to not take into account the 11
**		"lost" days in Sept. 1752.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/

# define HNANOPERSEC ((longlong)10000000)
# define HNANOPERUSEC (HNANOPERSEC / 1000000)

/*
** The difference between 01-jan-1601 and 01-jan-1970 in 100-nanosecond
** intervals. (134774 days)
*/
static longlong interval = (longlong)134774 * 
                           (longlong)24 * 
                           (longlong)60 * 
                           (longlong)60 * 
                           HNANOPERSEC;

static CS_SYNCH nanomutex;
static HRSYSTIME lasttime ZERO_FILL;
static bool initialized = FALSE;

STATUS
TMhrnow(HRSYSTIME *stime)
{
    FILETIME filetime;
    longlong timecalc;
    ULARGE_INTEGER uli;

    if ( !initialized )
    {
        initialized = TRUE;
        CS_synch_init(&nanomutex);
    }

    /* get count of 100-nanosecond intervals since January 1, 1601 */
    GetSystemTimeAsFileTime(&filetime);

    /* convert to 64-bit integer */
    uli.LowPart = filetime.dwLowDateTime;
    uli.HighPart = filetime.dwHighDateTime;
    timecalc = uli.QuadPart;

    /* adjust to the Unix epoch */
    timecalc -= interval;

    stime->tv_sec = (i4) (timecalc / HNANOPERSEC);

    stime->tv_nsec = (i4) (timecalc % HNANOPERSEC) * 100;

    /*
    ** if we have been called twice within the same 100-nanosecond
    ** interval, increment the time by one nanosecond.
    */
    CS_synch_lock(&nanomutex);
    if ( stime->tv_sec == lasttime.tv_sec &&
         stime->tv_nsec <= lasttime.tv_nsec )
    {
        stime->tv_nsec = lasttime.tv_nsec + 1;
    }
    lasttime.tv_sec = stime->tv_sec;
    lasttime.tv_nsec = stime->tv_nsec;
    CS_synch_unlock(&nanomutex);

    return (OK);
}
