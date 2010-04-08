/*
** Copyright (C) 1993, 2004 Ingres Corporation
*/

#include    <compat.h>

#include    <er.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>

/*
**  Name: TMPRFSTAT.C - return per-process performance statistics.
**
**  Description:
**
**	Returns per-process performance statistics.  Implements TMperfstat()
**	function.  Currently contains implementations for getrusage() machines
**	(BSD - mostly) and XXX machines (SVR4 - mostly).
**
**	External routines:
**
**	    TMperfstat()  - Return performance statistics available from the OS.
**
**	Internal routines:
**	
**	    default_perfstat()   - default (null) implementation of TMperfstat()
**	    getrusage_perfstat() - getrusage() implementation of TMperfstat()
**
**  History:
**      26-jul-93 (mikem)
**	    Created.
**	23-aug-93 (mikem)
**	    Fixes to the procfs implementation to work make it work on programs
**	    which alter their effective uid after process startup.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	26-jan-2004 (somsa01)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
*/


/* External variablese */
GLOBALREF CS_SEMAPHORE  CL_misc_sem;

GLOBALREF i4 Di_reads;	/* from DI */
GLOBALREF i4 Di_writes;

GLOBALREF i4 Gc_reads;	/* from GC */
GLOBALREF i4 Gc_writes;

/*
**  Definition of static variables and forward static functions.
*/

static STATUS wnt_perfstat(
    TM_PERFSTAT *stat,
    CL_SYS_ERR  *sys_err);

static STATUS default_perfstat(
    TM_PERFSTAT *stat,
    CL_SYS_ERR  *sys_err);

static TM_PERFSTAT default_tmperf = {
    {-1, -1}, 	/* stat->tmperf_utime    */
    {-1, -1}, 	/* stat->tmperf_stime    */
    {-1, -1}, 	/* stat->tmperf_cpu      */
    -1, 	/* stat->tmperf_maxrss   */
    -1,		/* stat->tmperf_idrss    */
    -1,		/* stat->tmperf_minflt   */
    -1,		/* stat->tmperf_majflt	 */
    -1,		/* stat->tmperf_nswap	 */
    -1,		/* stat->tmperf_reads	 */
    -1,		/* stat->tmperf_writes	 */
    -1,		/* stat->tmperf_dio   	 */
    -1,		/* stat->tmperf_msgsnd	 */
    -1,		/* stat->tmperf_msgrcv	 */
    -1,		/* stat->tmperf_msgtotal */
    -1,		/* stat->tmperf_nsignals */
    -1,		/* stat->tmperf_nvcsw	 */
    -1		/* stat->tmperf_nivcsw	 */
};

/*
**  Defines of other constants.
*/
 
/* number of 100-nano-seconds in 1 second */
#define		HNANO	10000000
 
/* number of 100-nano-seconds in 1 milli-second */
#define		HNANO_PER_MILLI	10000
 
/* number of milli-seconds in 1 second */
#define		MILLI	1000
/* number of seconds in the lo word  */
#define		SECONDS_PER_DWORD 0xFFFFFFFF / HNANO_PER_MILLI

/*{
** Name: TMperfstat()   - Return performance statistics available from the OS.
**
** Description:
**      Returns available performance statistics available from the OS for
**      the current process.  As best as is possible, per platform, TMperfstat()
**      provides a snapshot of all performance statistics available and
**      returns the statistics by filling in the TM_PERFSTAT structure passed
**      in by the caller.  Implimentations of this function will attempt to
**      collect the data required with the minimum possible affect on the
**      state being measured (ie. will try to make a single system call vs.
**      multiple system calls where possible).
**
**      The members of the TM_PERFSTAT structure are public and are defined in
**      <tm.h>;  see <tm.h> for description of current members of TM_PERFSTAT
**      structure.
**
**      Some statistics may not be available on some operating systems, if
**      a statistic is not available then TMperfstat() will initialize the
**      field to the value "-1".  In the case of SYSTIME structure members
**      one must test both members of the structure for -1.
**
**      It is expected that over time new CL proposals will add members to
**      the TM_PERFSTAT structure as new OS performance statistics become
**      available.   Clients which access the structure members by name,
**      and do not make structure order assumptions will be upward compatible
**      with future structure definition changes.
**
** Inputs:
**      stat                            pointer to a TM_PERFSTAT structure.
**
** Outputs:
**      stat                            TM_PERFSTAT structure is filled in by
**                                      this routine with available OS
**                                      statistics.  Unavailable fields will be
**                                      set to -1.
**      sys_err
**
**      Returns:
**          OK - success.
**          TM_NOSUPPORT - no support for gathering system statistics.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-jul-93 (mikem)
**         Created.
**	29-aug-1995 (shero03)
**	    Obtain statistics from NT.
*/
STATUS 
TMperfstat(
    TM_PERFSTAT *stat,
    CL_SYS_ERR  *sys_err)
{
    STATUS	status;

    CL_CLEAR_ERR(sys_err);

    if ((status = wnt_perfstat(stat, sys_err)) != OK)
	(void) default_perfstat(stat, sys_err);

    return(status);
}


/*{
** Name: wnt_perfstat()   - Windows NT implementation of TMperfstat()
**
** Description:
**      Implementation of TMperfstat() on Windows NT.
**
**	The current implementation supports the following structure members:
**		tmperf_cpu
**		tmperf_maxrss
**		tmperf_idrss
**		tmperf_majflt
**		tmperf_dio
**		tmperf_msgtotal
**	all other members are set to -1.
**
** Inputs:
**      stat                            pointer to a TM_PERFSTAT structure.
**
** Outputs:
**      stat                            TM_PERFSTAT structure is filled in by
**                                      this routine with available OS
**                                      statistics.  Unavailable fields will be
**                                      set to -1.
**
**      sys_err				system specific error.
**
**      Returns:
**          OK - success.
**          TM_NOSUPPORT - no support for gathering system statistics.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-aug-1995 (shero03)
**         Created.
*/
/* ARGSUSED */
static STATUS
wnt_perfstat(
    TM_PERFSTAT *stat,
    CL_SYS_ERR  *sys_err)
{

    FILETIME	kerneltime;
    FILETIME	usertime;
    FILETIME	dummy;
    i4     	worktime;
			     	
    /* assign -1 to all fields */

    *stat = default_tmperf;

    if (! GetProcessTimes( GetCurrentProcess(),
    		     &dummy, &dummy,
		     &kerneltime, &usertime) )
    {
	DWORD error = GetLastError();
        return(TM_NOSUPPORT);
    }

    stat->tmperf_utime.TM_secs = usertime.dwHighDateTime * SECONDS_PER_DWORD;
    worktime = usertime.dwLowDateTime / HNANO_PER_MILLI;
    stat->tmperf_utime.TM_secs += worktime / MILLI;
    stat->tmperf_utime.TM_msecs = worktime % MILLI; 
     
    stat->tmperf_stime.TM_secs = kerneltime.dwHighDateTime * SECONDS_PER_DWORD;
    worktime = kerneltime.dwLowDateTime / HNANO_PER_MILLI;
    stat->tmperf_stime.TM_secs += worktime / MILLI;
    stat->tmperf_stime.TM_msecs = worktime % MILLI; 

    worktime = stat->tmperf_utime.TM_msecs + stat->tmperf_stime.TM_msecs;
    if (worktime >= MILLI)
    {
        stat->tmperf_cpu.TM_secs = worktime / MILLI;
        worktime = worktime % MILLI;
    }
    else
    {
    	stat->tmperf_cpu.TM_secs = 0;
    }

    stat->tmperf_cpu.TM_msecs = worktime;
    stat->tmperf_cpu.TM_secs += stat->tmperf_utime.TM_secs +
    				stat->tmperf_stime.TM_secs;
    				 
    stat->tmperf_reads = Di_reads;	/* from DI_read */
    stat->tmperf_writes = Di_writes;	/* from DI_write */
    stat->tmperf_dio = Di_reads + Di_writes;

    stat->tmperf_msgsnd = Gc_writes;	/* from GCACL */
    stat->tmperf_msgrcv = Gc_reads; 	/* from GCACL */
    stat->tmperf_msgtotal = Gc_reads + Gc_writes;

    return(OK);
}

/*{
** Name: default_perfstat()   - default (null) implementation of TMperfstat()
**
** Description:
**	If no system support exists to gather performance statistics on the
**	machine then the following implementation is used.  This implementation
**	simply initializes all fields of the structure to -1.
**
** Inputs:
**      stat                            pointer to a TM_PERFSTAT structure.
**
** Outputs:
**      stat                            TM_PERFSTAT structure is filled in by
**                                      this routine with available OS
**                                      statistics.  Unavailable fields will be
**                                      set to -1.
**
**      sys_err				system specific error.
**
**      Returns:
**          OK - success.
**          TM_NOSUPPORT - no support for gathering system statistics.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-jul-93 (mikem)
**         Created.
*/
/* ARGSUSED */
static STATUS
default_perfstat(
    TM_PERFSTAT *stat,
    CL_SYS_ERR  *sys_err)
{
    /* assign -1 to all fields */

    *stat = default_tmperf;

    return(TM_NOSUPPORT);
}
