/*
** Copyright (c) 2009 Ingres Corporation 
*/
#include <compat.h>
#include <tm.h>
#include <tmtz.h>
#include <gen64def.h>
#include <ssdef.h>
#include <starlet.h>

FUNC_EXTERN long int TMconv(struct _generic_64 *tm);

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
**          i4     tv_sec;         ** number of secs since 1-jan-1970 **
**          i4     tv_nsec;        ** nanoseconds **
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
**      14-jan-2009 (joea)
**          Created.
*/

STATUS
TMhrnow(
HRSYSTIME *stime)
{
    struct _generic_64	timadr;
    unsigned __int64 a100nsec;
    PTR		tz_cb;
    STATUS	status;

    status = sys$gettim(&timadr);
    if (status != SS$_NORMAL)
	return TM_GETTIME_ERR;
    stime->tv_sec = TMconv(&timadr);	/* get seconds since Jan 1, 1970 */
    /* Adjust for timezone */
    status = TMtz_init(&tz_cb);
    if (status != OK)
	return TM_GETTIME_ERR;
    stime->tv_sec -= TMtz_search(tz_cb, TM_TIMETYPE_LOCAL, stime->tv_sec);
    /*
    ** get nanoseconds since last second.  VMS gives 64 bit time as number
    ** of 100 nsec units since Nov 17, 1858.  We mod with 10 million to get
    ** number of 100 nsec units since last second, then convert to nsec.  
    */
    a100nsec = timadr.gen64$q_quadword;
    stime->tv_nsec = (a100nsec % 10000000) * 100;

    return OK;
}
