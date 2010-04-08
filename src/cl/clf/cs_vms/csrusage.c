/*
**Copyright (c) 2009 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <rusage.h>
#include <tr.h>
#include <pc.h>

/* 
** History
**
**	16-Feb-93 (smc)
**		Fixed up gettimeofday to use cast of struct timezone.
**      20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) in CS_rrusage() to describe 2
**          versions of gettimeofday() available.
**	13-Apr-95 (georgeb)
**	    Added if defined(usl_us5) for gettimeofday() as Unixware 2.0
**	    expects a second argument which is NULL.
**	29-may-1997 (canor01)
**	    Clean up compiler warnings.
**      30-mar-1997 (kosma01)
**          Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5
**          to agree with arguments of gettimeofday() in <sys/time.h>.
**      29-feb-2000 (wansh01)
**         added if defined to define CS_itv for sos_us5 
**      20-mar-2009 (stegr01)
**         lifted from Unix stream to support Posix threading on VMS
**         for Itanium port
*/

# if defined(xCL_005_GETTIMEOFDAY_EXISTS) || defined(sos_us5) 
struct timeval	CS_itv;
# endif

void
CS_rrusage()
{
#ifdef xCL_003_GETRUSAGE_EXISTS
    struct timeval	tv;
    struct rusage	ruse;

    getrusage(RUSAGE_SELF, &ruse);

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
#if defined(usl_us5)
    gettimeofday(&tv, NULL);
#else
    gettimeofday(&tv);
#endif
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
    gettimeofday(&tv, (struct timezone *)0);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
    gettimeofday(&tv, (struct timezone *)0);
# endif

    tv.tv_sec -= CS_itv.tv_sec;
    if (tv.tv_usec < CS_itv.tv_usec)
    {
	tv.tv_sec--;
	tv.tv_usec += 1000000;
    }
    tv.tv_usec -= CS_itv.tv_usec;
    TRdisplay("Elapsed time:\t%d.%d\n", tv.tv_sec, tv.tv_usec);

    TRdisplay("Rusage struct:\n");
    TRdisplay("utime:\t\t%d.%d\n", ruse.ru_utime.tv_sec, ruse.ru_utime.tv_usec);
    TRdisplay("stime:\t\t%d.%d\n", ruse.ru_stime.tv_sec, ruse.ru_stime.tv_usec);
    TRdisplay("maxrss:\t%d\n", ruse.ru_maxrss);
    TRdisplay("ixrss:\t\t%d\n", ruse.ru_ixrss);
    TRdisplay("idrss:\t\t%d\n", ruse.ru_idrss);
    TRdisplay("isrss:\t\t%d\n", ruse.ru_isrss);
    TRdisplay("minflt:\t%d\n", ruse.ru_minflt);
    TRdisplay("majflt:\t%d\n", ruse.ru_majflt);
    TRdisplay("nswap:\t\t%d\n", ruse.ru_nswap);
    TRdisplay("inblock:\t%d\n", ruse.ru_inblock);
    TRdisplay("oublock:\t%d\n", ruse.ru_oublock);
    TRdisplay("msgsnd:\t%d\n", ruse.ru_msgsnd);
    TRdisplay("msgrcv:\t%d\n", ruse.ru_msgrcv);
    TRdisplay("nsignals:\t%d\n", ruse.ru_nsignals);
    TRdisplay("nvcsw:\t\t%d\n", ruse.ru_nvcsw);
    TRdisplay("nivcsw:\t%d\n", ruse.ru_nivcsw);
#endif /* xCL_003_GETRUSAGE_EXISTS */
}

CS_recusage()
{
# ifdef xCL_005_GETTIMEOFDAY_EXISTS

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
#if defined(usl_us5)
    gettimeofday(&CS_itv, NULL);
#else
    gettimeofday(&CS_itv);
#endif
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
    gettimeofday(&CS_itv, (struct timezone *)0);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
    gettimeofday(&CS_itv, (struct timezone *)0);
# endif

    PCatexit(CS_rrusage);
# endif
}
