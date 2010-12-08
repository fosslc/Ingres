/*
** Copyright (c) 1993, 2009 Ingres Corporation
**      All Rights Reserved.
*/
#ifndef TM_INCLUDE
#define TM_INCLUDE

#include    <tmcl.h>

/**CL_SPEC
** Name:	TM.h	- Define TM function externs
**
** Specification:
**
** Description:
**	Contains TM function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**      13-jun-1993 (ed)
**          make safe for multiple inclusions
**	26-jul-1993 (mikem)
**	    Added TMperfstat() func_extern and TM_PERFSTAT definition.
**	    Also moved TM_INCLUDE multiple include protection from tmcl.h 
**	    into this file to prevent multiple defines of TM_PERFSTAT by
**	    some modules.
**      21-oct-1996 (canor01)
**          Make TM_lookup_month() visible (for Jasmine).
**      14-dec-1998 (canor01)
**	    Add prototype for TMhrnow()
**      11-apr-1999 (schte01)
**       Remove semicolon from 14-dec-1998 comment - it caused compiler
**       errors on axp.
**	10-Dec-2001 (bolke01)
**	    Added ifdnef for VMS arround TMhrnow, structure not used in VMS cl
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-nov-2003 (abbjo03)
**	    Add prototype for TMsecs_to_stamp().
**	14-jan-2009 (joea)
**	    Do not hide TMhrnow prototype on VMS.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

/*
**  External typedef/struct references.
*/

/*}
** Name: TM_PERFSTAT - single structure defining all perfstats available.
**
** Description:
**      This structure contains entries for all known interesting performance
**      statistics available from the OS about a process.  Not all statistics
**      will be available on all OS's.  Those stats which are unavailable will
**      be set to "-1".  Use the TMperfstat() routine to retrieve these stats
**      from the OS.
**
**      Although the current set of fields are patterned after those available
**      on BSD systems, it is expected that more fields will be added to this
**      structure.  Future proposals will add structure members.  Clients should
**      only reference structure members by name and not assume a particular
**      order of members.  Any changes to the structure will require recompile
**      and relink of client and CL routines.
**
** History:
**      26-july-93 (mikem)
**          Created.
*/
typedef struct
{
    SYSTIME         tmperf_utime;       /* user time used */
    SYSTIME         tmperf_stime;       /* system time used */
    SYSTIME	    tmperf_cpu;		/* total cpu used:
					** tmperf_utime + tmperf_stime
					*/
    i4         tmperf_maxrss;      /* maximum resident set size */
    i4         tmperf_idrss;       /* current resident set size */
    i4         tmperf_minflt;      /* pg flts not requiring physical I/O */
    i4         tmperf_majflt;      /* pg flts requiring physical I/O */
    i4         tmperf_nswap;       /* process swaps */
    i4         tmperf_reads;       /* read I/O's from disk */
    i4         tmperf_writes;      /* write I/O's to disk */
    i4	    tmperf_dio;		/* # of disk I/O operations - 
					** tmperf_reads + tmperf_writes
					*/
    i4         tmperf_msgsnd;      /* messages sent */
    i4         tmperf_msgrcv;      /* messages received */
    i4	    tmperf_msgtotal;	/* total messages */
    i4         tmperf_nsignals;    /* # of signals received */
    i4         tmperf_nvcsw;       /* # of voluntary context switches */
    i4         tmperf_nivcsw;      /* # of involuntary context switches */
} TM_PERFSTAT;


/*
**  Forward and/or External function references.
*/


FUNC_EXTERN STATUS  TMbreak(
	SYSTIME		*tm, 
	struct TMhuman	*human
);

FUNC_EXTERN i4      TMcmp(
	SYSTIME		*time1, 
	SYSTIME		*time2
);

FUNC_EXTERN i4      TMcmp_stamp(
	TM_STAMP	*time1, 
	TM_STAMP	*time2
);

FUNC_EXTERN i4 TMcpu(
	void
);

FUNC_EXTERN STATUS  TMcvtsecs(
	i4		secs, 
	char		*buf
);

FUNC_EXTERN VOID    TMend(
	TIMER		*tm
);

FUNC_EXTERN VOID    TMformat(
	TIMER		*tm, 
	i4		mode, 
	char		*ident, 
	char		decchar, 
	char		*buf
);

FUNC_EXTERN VOID    TMget_stamp(
	TM_STAMP	*stamp
);

# ifndef TMhrnow
FUNC_EXTERN STATUS  TMhrnow(
	HRSYSTIME       *stime
	);
# endif /* TMhrnow */

FUNC_EXTERN STATUS  TMinit(
	void
);

FUNC_EXTERN VOID    TMnow(
	SYSTIME	    *stime
);

FUNC_EXTERN STATUS  TMperfstat(
    TM_PERFSTAT *stat,
    CL_SYS_ERR  *sys_err
);

FUNC_EXTERN i4 TMsecs(
	void
);

FUNC_EXTERN void TMsecs_to_stamp(
	i4		secs,
	TM_STAMP	*stamp);

FUNC_EXTERN VOID    TMstamp_str(
	TM_STAMP    *time, 
	char	    *string
);

FUNC_EXTERN VOID    TMstart(
	TIMER	    *tm
);

FUNC_EXTERN VOID    TMstr(
	SYSTIME	    *timeval, 
	char	    *timestr
);

FUNC_EXTERN STATUS  TMstr_stamp(
	char	    *string,     
	TM_STAMP    *time
);

FUNC_EXTERN STATUS  TMyrsize(
	i4	    year, 
	i4	    *daysinyear
);

FUNC_EXTERN STATUS  TM_lookup_month(
	char	    *month_str, 
	i4	    *month_num
);

FUNC_EXTERN VOID    TMzone(
	i4	    *zone
);

/*
**  Defines of other constants.
*/

/* TM return status codes (common to all CL's). */

# define 	TM_NOSUPPORT	(E_CL_MASK + E_TM_MASK + 0x0F)

# endif /* _TM_INCLUDE */
