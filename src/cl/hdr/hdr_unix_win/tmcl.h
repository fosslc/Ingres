/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

#include <systypes.h>
#ifdef sgi_us5
# include <sys/time.h>
#else
# include <time.h>
#endif

/**CL_SPEC
** Name TM.H - Global definitions for the TM compatibility library.
**
** Description:
**      The file contains the type used by TM and the definition of the
**      TM functions.  These are used for time manipulation.
**
** Specification:
**
**  Description:  
**	These new TM routines perform various time related functions.  There
**	are two types of times: timestamps and seconds.  A timestamp
**	is a high resolution clock sample.  Seconds are used to 
**	represent low resolution events and dates.
**      
**  Intended Uses:
**	The TM routines are intended for for use by anyone needing time
**	and/or date information.
**      
**  Assumptions:
**	A timestamp must fit into a 8-byte quantity.
**
**	The resolution of a timestamp must be finer than a second.
**      If a host operating system cannot support this, it should
**      fake a finer granularity that is quaranteed to be 
**      monotonically increasing within a single process.
**
**  Definitions:
**
**  Concepts:
**
**	The timestamp is probably just the raw value of the machines clock
**	register or the rare value of the software clock.  Some processing
**	might be needed to shrink it into 8-bytes, but currently this
**	doesn't seem to be a problem.
**
**	The conversion routines are used to display the timestamps, and
**	for comparing user given timestamps to internally stored timestamps.
**
**(	The format of the output/input string is fixed as:
**	    dd-mmm-yyyy hh:mm:ss (The space between the yyyy and hh can be
**				  ':' on input, so that it can be easy typed
**)				  on an INGRES command line.)
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	08-jan-86 (jeff)
**	    Changed FUNC_EXTERN of TMcvtsecs to return STATUS instead of VOID.
**	    Same for TMyrsize and TMbreak.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-dec-87 (anton)
**	    use getrusage on bsd
**	8-may-89 (daveb)
**		VTIMES is dead for r6.  remove all references.
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      8-jul-93 (ed)
**          move nested include protection to glhdr
**	24-apr-1997 (canor01)
**	    Add definition of HUNDREDNANO_PER_MILLI for use by NT's
**	    GetProcessTimes() and GetThreadTimes() functions.
**	15-aug-1997 (canor01)
**	    Add wrappers for gmtime and asctime to make reentrant
**	    versions transparent.
**	20-oct-97 (mcgem01)
**	    Don't use wrappers for gmtime and asctime on NT.  Also
**	    added includes for systypes.h and time.h
**	27-Oct-1997 (kosma01)
**	    Change the transparent wrapper for asctime on rs4_us5 
**	    platforms. asctime on rs4_us5 does not take a length.
**	    leave len in macro but do not pass it to reentrant 
**	    function asctime_r.
**	03-Apr-1998 (muhpa01)
**	    Include hpb_us5 in change for asctime_r for rs4_us5
**	08-Jul-1998 (kosma01)
**	    sgi_us5 asctime_r takes only two arguments. Add sgi_us5
**	    to rs4_us5 ifdef.
**	1-dec-98 (stephenb)
**	    Add definition for TMhrnow(). On Solaris this is simply a call to
**	    clock_gettime(), but it may realy need to be coded on other
**	    platforms.
**	19-jan-1999 (muhpa01)
**	    Add hp8_us5 to Solaris definitiion for TMhrnow().
**	16-feb-99 (rosga02)
**		Add definition of TMhrnow() for sui_us5
**	16-mar-1999 (popri01)
**	    Add NANOPERSEC constant.
**	    Relocate NANO_PER_MILLI and MICRO_PER_MILLI from tmprfstat.c.
**	29-mar-1999 (popri01)
**	    Because some platforms, such as Unixware (usl_us5), do not 
**	    support high-resolution timing, implement a pseudo function (see 
**	    tmhrnow.c). But allow for a wrapped implementation on platforms
**	    that have native functions.
**	31-Mar-1999 (kosma01)
**	    On sgi_us5, <time.h> does not include (sys/time.h>, so 
**	    do so directly.
**	09-Apr-99 (bonro01)
**	    Add definition of TMhrnow() for DGUX. 
**	    DGUX uses clock_gettime() for TMhrnow() so use 'struct timespec'
**	    for HRSYSTIME to prevent incompatible pointer compiler warnings.
**	12-Apr-99 (schte01)
**       Add axp_osf to TMhrnow list and add use of timespec for axp_osf.
**      14-May-1999 (allmi01)
**          Set proper macro replacements for rux_us5 with pthreads.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      07-jul-99 (hweho01)
**          Defined the appropriate TM_copy_asctime() function for 
**          AIX 64-bit platform (ris_u64).
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**          Correct rux_us5 changes that were applied using the rmx_us5
**          label.
**	29-feb-2000 (toumi01)
**	    Added support for int_lnx (OS threaded build) - asctime_r takes
**	    two arguments.
**	20-apr-1999 (hanch04)
**	    Use long to match OS time stamp types
**	15-aug-2000 (somsa01)
**	    Added support for ibm_lnx.
**      10-Nov-2000 (hweho01)
**          Defined the appropriate TM_copy_asctime() function for 
**          Compaq 64-bit platform (axp_osf).
**	21-jun-2001
**	    Added MICROS_PER_SEC, NANO_PER_MICRO.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	04-Dec-2001 (hanje04)
**	    Add support IA64 Linux (i64_lnx)
**	25-Mar-2002 (xu$we01)
**	    Add definition for TM_copy_asctime() for dgi_us5.
**	09-May-2002 (xu$we01)
**	    Solve the compilation error "undefined symbol TMstr " for
**	    dgi_us5.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	16-Mar-2005 (bonro01)
**	    Solaris a64_sol uses clock_gettime() for TMhrnow()
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	07-Dec-2007 (kiria01) b119773
**	    Use proper timespec on linux
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
**/

/* 
** Defined Constants
*/
# define NANOPERSEC      1000000000
# define MICROS_PER_SEC	 1000000
# define NANO_PER_MILLI ( NANOPERSEC / 1000 )
# define NANO_PER_MICRO ( NANOPERSEC / 1000000 )
# define HUNDREDNANO_PER_MILLI ( NANO_PER_MILLI / 100)
# define MICRO_PER_MILLI 1000

/* TM return status codes. */

#define                 TM_OK           0
# define		TM_BAD_MSECS	(E_CL_MASK + E_TM_MASK + 0x01)
/* :Number of milliseconds for an alram was less than zero */
# define		TM_ZERO_TIME	(E_CL_MASK + E_TM_MASK + 0x02)
/* :Zero seconds resulted when rounding milliseconds for an alarm */
#define			TM_NOTIMEZONE	(E_CL_MASK + E_TM_MASK + 0x03)
/* :The environment variable, II_TIMEZONE, is undefined */
#define                 TM_SYNTAX       (E_CL_MASK + E_TM_MASK + 0x04)
/* syntax error in time format */

typedef struct
	{
	long int	time;
	unsigned short 	millitm;
	short		timezone;
	short		dstflag;
	}   TMSTRUCT;

struct TM_SYSTIME				/* define what system time */
{						/* is for 4.2 UNIX */
	i4	TM_secs;
	i4	TM_msecs;
};

typedef struct TM_SYSTIME	SYSTIME;

struct TMhuman					/* human readable time format*/
{						/* broken up into components */
	char	wday[4];			/* for the report writer */
	char	month[4];
	char	day[3];
	char	year[5];
	char	hour[3];
	char	mins[3];
	char	sec[3];
};

# define	NORMYEAR	365		/* # of days in a norm year */
# define	LEAPYEAR	366		/* # of days in a leap year */




typedef struct
{
	i4	stat_cpu;
	i4	stat_dio;
	i4	stat_bio;
	i4	stat_pgflts;
	i4	stat_wc;
	/*! BEGIN BUG - working set and physical pages not originally implemented */
	i4	stat_ws;
	i4	stat_phys;
	i4	stat_usr_cpu;
	/*! END BUG */
} TIMERSTAT;


typedef struct
{
	TIMERSTAT	timer_start;
	TIMERSTAT	timer_end;
	TIMERSTAT	timer_value;
} TIMER;

/* TMformat display modes
*/

# define	TIMER_START	1
# define	TIMER_END	2
# define	TIMER_VALUE	3

/*
** Wrappers for reentrant gmtime and asctime functions
**
** void TM_copy_gmtime( time_t *clock, struct tm *res );
**
** void TM_copy_asctime( struct tm *tmin, char *buf, int buflen );
**
*/
# if defined (UNIX) && defined (OS_THREADS_USED)
# if defined(rux_us5)
# define TM_copy_gmtime(clock,res)      (void)gmtime( clock );
# else
# define TM_copy_gmtime(clock,res)      (void)gmtime_r( clock, res );
# endif /* rux_us5*/
# if defined(any_aix) || defined(sgi_us5) || defined(thr_hpux) || \
     defined(axp_osf) || defined(LNX) || defined(OSX)
# define TM_copy_asctime(tmin,buf,len)	(void)asctime_r( tmin, buf );
# else
# define TM_copy_asctime(tmin,buf,len)	(void)asctime_r( tmin, buf, len );
# endif
# else /* UNIX and OS_THREADS_USED */
# define TM_copy_gmtime(clock,res)	(void)memcpy(res,gmtime(clock),\
						sizeof(struct tm));
# define TM_copy_asctime(tmin,buf,len)	(void)memcpy(buf,asctime(tmin),len);
# endif /* UNIX and OS_THREADS_USED */


/*}
** Name: TM_STAMP - Time stamp.
**
** Description:
**      This structure represents the internal storage used to store a
**	timestamp.  If the host operating system supports zones, then
**      they should be supported by the corresponding CL routines.
**
**	On unix this is basically a mimic of the SYSTIME structure.  A
**	different structure has been used in the hope that the SYSTIME
**	structure can be phased out by a reorganization of the TM module.
**
** History:
**      20-jan-1987 (Derek)
**	    Created.
[@history_template@]...
*/
typedef struct _TM_STAMP
{
    i4		tms_sec;			/* seconds */
    i4		tms_usec;			/* micro seconds */
#define			TM_SIZE_STAMP		24
						/* TM_SIZE_STAMP is the
                                                ** number of BYTES used 
                                                ** to contain the time
                                                ** stamp. */
}   TM_STAMP;


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    TMsubstr(
        TIMER       *outer,
        TIMER       *inner,
        TIMER       *result
);

FUNC_EXTERN VOID    TMet(
	SYSTIME *stime
);

/*
** Name: TMhrnow
**
** Definition:
**	Get the current system time in high resolution (currently defined
**	to be a value in seconds and nanoseconds). On solaris, this function can
**	simply be defined to be clock_gettime() since the system structure,
**	timespec, actually matches the definition of HRSYSTIME, and can be defined
**	to look the same. On some platforms, a TMhrnow() function may realy
**	need to be written, and should be placed in cl!clf!tm time.c
**	HRSYSTIME is defined to be:
**	struct HRSYSTIME
**	{
**	    i4	tv_sec;		** number of secs since 1-jan-1970 **
**	    i4	tv_nsec;	** nanoseconds **
**	};
**
*/

#if defined (sparc_sol) || defined(any_hpux) || defined (sui_us5) || \
    defined (DGUX) || defined (axp_osf) || \
    defined(a64_sol) || defined (LNX)
#define TMHRNOW_WRAPPED_CLOCKGETTIME
#define HRSYSTIME struct timespec

FUNC_EXTERN STATUS TMhrnow(HRSYSTIME *stime);

#define		TMhrnow(hrtime)		clock_gettime( CLOCK_REALTIME, (hrtime) )
#else
typedef struct _HRSYSTIME
{
    long	tv_sec;
    long	tv_nsec;
} HRSYSTIME;
#endif /* su4_us5 hp8_us5 sui_us5 DGUX axp_osf */
