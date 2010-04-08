/*
** Copyright (c) 1985, 2009 Ingres Corporation
*/

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
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      8-jul-93 (ed)
**          move nested include protection to glhdr
**	16-mar-1999 (popri01)
**	    Add NANOPERSEC constant.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	23-oct-2003 (devjo01)
**	    Cross 103715 changes from UNIX CL.
**	14-jan-2009 (joea)
**	    Remove TMhrnow macro and implement as a function.  Add
**	    TM_GETTIME_ERR.
**      13-mar-2009 (stegr01)
**          Add a couple more defines of msec/nsecs for compatibility with
**          Unix variant
**/

/* 
** Defined Constants
*/
#define NANOPERSEC	1000000000

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
#define                 TM_GETTIME_ERR  (E_CL_MASK + E_TM_MASK + 0x11)
/* Error getting system time */

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
# define	MICROS_PER_SEC	1000000
# define        NANO_PER_MILLI  ( NANOPERSEC / 1000 )
# define	NANO_PER_MICRO	( NANOPERSEC / MICROS_PER_SEC )
# define        HUNDREDNANO_PER_MILLI ( NANO_PER_MILLI / 100)
# define        MICRO_PER_MILLI 1000

/*
** Client code can not depend on these values being filled in.
** Some values may not make sense in all supported environments,
** e.g. working set size.
*/

typedef struct
{
	i4	stat_cpu;	/* CPU in milliseconds */
	i4	stat_dio;	/* Direct I/O -- database files */
	i4	stat_bio;	/* Buffered I/O -- communications */
	i4	stat_pgflts;	/* Page faults */
	i4	stat_wc;	/* Wall clock -- elapsed time */
	i4	stat_ws;	/* Working set size */
	i4	stat_phys;	/* Physical pages */
} TIMERSTAT;

/*
**	A TIMER structure contains three TIMERSTAT blocks:
**	- one holding the starting statistics count (set at TMstart)
**	- one holding the statistics count at TMend invocation
**	- one holding the difference
*/
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


/*}
** Name: TM_STAMP - Time stamp.
**
** Description:
**      This structure represents the internal storage used to store a
**	timestamp.  If the host operating system supports zones, then
**      they should be supported by the corresponding CL routines.
**
** History:
**      20-jan-1987 (Derek)
**	    Created.
*/
typedef struct _TM_STAMP
{
    char	    stamp[8];
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
#ifdef CL_PROTOTYPED
        TIMER       *outer,
        TIMER       *inner,
        TIMER       *result
#endif
);


/*
** Name: TMhrnow
**
** Definition:
**      Get the current system time in high resolution (currently defined
**      to be a value in seconds and nanoseconds). On VMS, this is the 
**	same as TMnow which already returns the seconds & nanoseconds.
**
*/

typedef struct _HRSYSTIME
{
    i4     tv_sec;
    i4     tv_nsec;
} HRSYSTIME;
