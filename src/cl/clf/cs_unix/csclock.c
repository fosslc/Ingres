/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include   <gl.h>
#include   <clconfig.h>
#include   <systypes.h>
#include   <rusage.h>

#include    <pc.h>
#include    <cs.h>
#include    <tm.h>

#include    <fdset.h>
#include    <csev.h>
#include    <csinternal.h>
#include    <cssminfo.h>

#include    "cslocal.h"

/**
**
**  Name: CSCLOCK.C - routine's to manipulate CS's shared memory "clock".
**
**  Description:
**	The UNIX CS implementation requires cheap access to a system "clock"
**	with at least millisecond granularity.  Currently CS uses this access
**	to service frontend client timeout requests (lock timeout with
**	second granularity), thread internal timeout requests (group commit 
**	thread timeout of millisecond granularity - CSms_thread_nap()), and
**	other timeouts.
**	The CS scheduler maintains a "timeout" queue, and places any threads
**	which have requested timeout on this queue.  If the timeout queue is
**	non-empty the scheduler checks everytime there is a thread switch
**	whether or not a thread should be moved from the timeout queue to the
**	run queue.  This check requires determining the "current time" and 
**	comparing it with the "time" the thread was suspended - these "times"
**	can be relative rather than absolute as only the difference is used.
**	
**	A straight forward implementation could query TMget_stamp() for every
**	suspend and thread switch and determine timeouts that way.  The cost
**	of such an implementation would be a system call for every suspend for
**	timeout and thread switch while a suspend was active.  For comparison
**	a TPC/B transaction thread switches approximately 3 times for every
**	transaction (one read from client, one read from account, and one wait
**	for LG force).  This would mean 3 extra system calls per transaction.
**
**	In 6.4 a single process (the RCP) updated the shared memory "clock"
**	while all other processes read from the shared memory address to 
**	determine the clock's value.  It turned out that the main user of the
**	clock was the RCP which was in charge of group commit.  It did this
**	by updating the clock after waking up from a subsecond "timeout" based
**	on select or poll.
**
**	In 6.5 the RCP no longer is responsible for clock update or group
**	commit.  Mainline group commit threads request timeout service from
**	the CL, and manage group commit.  Before implementation of 
**	CS_update_smclock() the
**	only updater of the shared memory clock was code executed upon return
**	from a server level idle (ie. when the server has nothing to do it
**	calls iiCLpoll() with a timeout equal to minimum timeout on the 
**	timeout queue if the queue is non-empty, or 90 seconds.  This presented
**	a problem if server never went idle (the clock never advanced or
**	advanced very slowly), and thus group commit didn't work properly.
**
**	In order to fix this problem this module implements a shared memory
**	clock, which is only updated as frequently as necessary.  The design
**	attempts to achieve:
**	    o Minimum number of system calls to query the current time, on
**	      some machines the system call to get time can be expensive.
**	      This will adjust as necessary to system usage.  An idle ingres
**	      installation should not use many resources.
**	    o Allow tuning on different architectures.  TM is not used as
**	      it is possible to implement this module with an absolute 
**	      timestamp (only relative times are necessary).  On some machines
**	      a hardward register may be available which can be used as a
**	      relative timestamp counter.
**	    o An efficient implementation as this code is used by the lowest
**	      level of the context switching system.  If profiling warrants
**	      MACRO inlining may be used.
**
**  History:
**      25-apr-94 (mikem)
**	    Created.
**	26-apr-94 (vijay)
**	    Include fdset.h before csev.h. needs defn of fd_set structure.
**	24-oct-1994 (canor01)
**		B64168: CS_clockinit() was referencing undefined clock_current
**		if xCL_005_GETTIMEOFDAY_EXISTS was not defined.
**      29-Nov-94 (nive)
**              For dg8_us5, chnaged the prototype definition for
**              gettimeofday to gettimeofday ( struct timeval *, struct
**              timezone * )
**	23-May-95 (walro03)
**		Reversed previous change (29-Nov-94).  A better way is to use
**		the correct prototype based on xCL_GETTIMEOFDAY_TIMEONLY vs.
**		xCL_GETTIMEOFDAY_TIME_AND_TZ.
**      15-jun-95 (popri01)
**          Add support for additional, variable parm list, GETTIME prototype
**	25-jul-95 (allmi01)
**		Added support for dgi_us5 (DG-UX on Intel based platforms) following dg8_us5.
**	20-feb-97 (muhpa01)
**		Removed local declaration for gettimeofday() for hp8_us5 to fix
**		"Inconsistent parameter list declaration" error.
**      26-mar-1997 (kosma01)
**          Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5
**          to agree with arguments of gettimeofday() in <sys/time.h>.
**      07-Jul-1998 (kinpa04)
**		Quantum count was getting out of sync and timeouts occurred
**		too soon since the out_of_date value was being ignored 
**		when it was negitive. Bug#90930.
**	16-Nov-1998 (jenjo02)
**	    Changed to return the number of ms instead of VOID, added
**	    code for OS threads to return real thread time instead of
**	    quantum time for the server, which is neither accurate nor
**	    thread safe.
**	15-Mar-1999 (popri01)
**	    Unixware (usl_us5) does not have a high-resolution timing
**	    function, so use TMhrnow which should at least be thread-safe.
**	26-mar-1999 (muhpa01)
**	    Add hpb_us5 for calling TMhrnow().
**      05-Apr-1999 (kosma01)
**              Used existing xCL_GETTIMEOFDAY_TIMEONLY_V for sgi_us5.
**              This allows local prototype of gettimeofday to agree with
**              gettimeofday() prototype in <sys/time.h>.
**	09-Apr-1999 (schte01)
**       Added axp_osf to list of TMhrnow users.
**	27-Apr-1999 (ahaal01)
**	    Added rs4_us5 to ifdef for TMhrnow.
**      01-Aug-1999 (linke01)
**         calling TMhrnow(), adopt popri01's change.
**      14-Feb-2000 (linke01)
**          undefined gettimeofday for usl_us5
**	24-May-2000 (toumi01)
**		Added int_lnx to list of TMhrnow users.
**	15-aug-2000 (somsa01)
**	    Added ibm_lnx to list of TMhrnow users.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-feb-2001 (devjo01)
**	    Add usage of 'cscl_calls_since_inc' and 'cscl_numcalls_before_inc'
**	    to prevent premature lock time-outs and session wakes on
**	    when using INTERNAL threads, and 'CS_checktime' is called more 
**	    than 1000 times a second. (b104072/INGSRV1389).
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/

GLOBALDEF i4	Cs_enable_clock = 0;
GLOBALREF i4	Cs_lastquant;


/*{
** Name: CS_checktime() - check that pseudo-quantum timer is being updated.
**
** Description:
**	This routine updates the pseudo-quantum timer by XXX ms every time
**	it is called.  It also checks that the quantum timer keeps close
**	to reality by making a system call to get the real time every XXX
**	times CS_checktime() is called.
**
**	Calls to this routine have been made from the following routines:
**	    CS_xchng_thread().
**	    TODO - CSswitch().
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    VOID
**
** History:
**      25-apr-94 (mikem)
**         Created.
**	16-Nov-1998 (jenjo02)
**	    Changed to return the number of ms instead of VOID, added
**	    code for OS threads to return real thread time instead of
**	    quantum time for the server, which is neither accurate nor
**	    thread safe.
**	15-Mar-1999 (popri01)
**	    Unixware (usl_us5) does not have a high-resolution timing
**	    function, so use TMhrnow which should at least be thread-safe.
**	07-Sep-1999 (bonro01)
**	    Use TMhrnow() for dgi_us5.  tmcl.h header will define this
**	    as clock_gettime().
**	10-Dec-1999 (podni01)
**	    Force rux_us5 to use TMhrnow().
**      13-Dec-1999 (hweho01)
**          AIX 64-bit (ris_u64) does not have gethrtime(), so need to 
**          call TMhrnow().
**	3-nov-00 (inkdo01)
**	    Removed "mod 10" test before performing ADD_QUANTUM, since 
**	    the test costs as much CPU as the addition and it essentially
**	    slows the time slicing process in Ingres threads installations.
**	8-feb-02 (hayke02)
**	    Exclude rs4_us5 from the new OS threading method of updating the
**	    quantum timer by calls to gettimeofday() via TMhrnow() and TMet().
**	    Instead, use the 'old'/internal threading method of calling
**	    CS_realtime_update_smclock() (which will call gettimeofday())
**	    every 500th call, and use CS_ADD_QUANTUM() for the other 499
**	    calls. This change fixes bug 106893.
**	29-may-02 (hayke02)
**	    Extend the above fix for 106893 to dgi_us5.
**      28-July2003 (hanal04) Bug 110468 INGSRV 2367
**          During idle periods (including a fully stalled installation
**          which is awaiting the resumtption of the Deadlock Detector Thread,
**          the idle thread is the only thread driving us towards an update
**          of the real system time (if we are running internal threads).
**          Incrementing a count 5000 times, at a rate of once per second
**          so that we can avoid the cost of a system call causes sessions
**          suspended on a timeout to wait much longer than anticipated.
**          This change will update the system time if the idle thread
**          finds that no other threads have called this function since the
**          idle thread last called this function.
**      30-Jan-2004 (hanal04) Bug 110468 INGSRV 2367 
**          Client testing of initial change showed continued deadlocks
**          tough less frequent. For Sequent only always refresh the 
**          clock to prevent 5 second timeouts becoming 2 hours timeouts.
*/
i4
CS_checktime(void)
{
    bool  idle_update = FALSE;

#if defined(OS_THREADS_USED)
# if !defined(any_aix) && !defined(dgi_us5)
    if (Cs_srv_block.cs_mt)
	{
	HRSYSTIME hrtime;

	TMhrnow(&hrtime);
	return (hrtime.tv_sec * 1000) + (hrtime.tv_nsec / NANO_PER_MILLI);
	}
# endif /* !aix  !dgi_us5 */
#endif /* OS_THREADS_USED */

# if defined(sqs_us5)
    CS_realtime_update_smclock();
#else
    if (&Cs_idle_scb == Cs_srv_block.cs_current)
    {
        if(Cs_sm_cb->css_clock.cscl_calls_since_idle == 0)
        {
            idle_update = TRUE;
        }
        Cs_sm_cb->css_clock.cscl_calls_since_idle = 0;
    }
    else
    {
        Cs_sm_cb->css_clock.cscl_calls_since_idle++;
    }

    if ((++Cs_sm_cb->css_clock.cscl_calls_since_upd >= 
	Cs_sm_cb->css_clock.cscl_numcalls_before_update) || idle_update)
    {
    	Cs_sm_cb->css_clock.cscl_calls_since_upd = 0;
	CS_realtime_update_smclock();
    }
    else if (++Cs_sm_cb->css_clock.cscl_calls_since_inc >=
	     Cs_sm_cb->css_clock.cscl_numcalls_before_inc)
    {
	Cs_sm_cb->css_clock.cscl_calls_since_inc = 0;
	CS_ADD_QUANTUM(Cs_sm_cb->css_clock.cscl_fixed_update);
    }
#endif /* sqs_ptx */

    return(Cs_sm_cb->css_qcount);
}

/*{
** Name: CS_realtime_update_smclock() - update shared mem clock based on syscll.
**
** Description:
**	Update shared memory pseudo-quantum timer based on system call to 
**	determine time.  This routines executes a system call every time it
**	is called, it expected that callers of this routine will optimize to
**	limit number of times it is called.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    number of ms. psuedo-quantum clock was updated.
**
** History:
**      25-apr-94 (mikem)
**         Created.
**	3-nov-00 (inkdo01)
**	    Changed to update Cs_lastquant in tandem with css_qcount. 
**	    Otherwise, change to css_quant can immediately trigger switch
**	    or worse, delay it by a significant amount (by effectively
**	    backing up the clock).
*/
i4
CS_realtime_update_smclock(void)
{
    HRSYSTIME 	t;
    CS_SM_CLOCK	clock_old;
    CS_SM_CLOCK	clock_current;
    i4	time_passed_ms;
    i4	out_of_date;
    i4	sec_diff;

    /* get system time */
    TMhrnow( &t );

    clock_current.cscl_secs = t.tv_sec;
    clock_current.cscl_msecs = t.tv_nsec / 1000000;

    /* move shared mem info into local storage */
    clock_old = Cs_sm_cb->css_clock;

    /* now update CS's pseudo quantum clock, if it looks out of date */
    time_passed_ms = clock_current.cscl_msecs - clock_old.cscl_msecs;
    if ((sec_diff = clock_current.cscl_secs - clock_old.cscl_secs) >= 0)
    {
	time_passed_ms += 
	    (clock_current.cscl_secs - clock_old.cscl_secs) * 1000;
    }
    else if (time_passed_ms < 0)
    {
	/* current shared memory time is greater than time we read */
	time_passed_ms = 0;
    }

    /* how much "out of date" is the pseudo-clock with the "real" time */
    out_of_date = time_passed_ms - 
		(Cs_sm_cb->css_qcount - Cs_sm_cb->css_clock.cscl_qcount);

    if (out_of_date < 0)
    {
	/* pseudo-clock going too fast - lower the per-call quantum update
	** will not adjust the clock backwards - "just slow it down", make
	** sure integer update value is always at least 1.
	*/
        if ( 1 == Cs_sm_cb->css_clock.cscl_fixed_update )
	{
	    /*
	    ** quantum update has been forced to minimum, and clock
	    ** is still running too fast.  Increase 'cscl_numcalls_before_inc'
	    ** to effectively allow clock to on average advance by a
	    ** fraction of a millsecond per call.  Alternative of increasing
	    ** quantum resolution to micro-seconds would have more pervasive
	    ** impact, and would require 64 bit counters.
	    */
	    Cs_sm_cb->css_clock.cscl_numcalls_before_inc += 
	       Cs_sm_cb->css_clock.cscl_numcalls_before_inc;

 
#ifdef bolke01_diag_high
		TRdisplay ("FAST:%d:upd=%d,OoD=%d,cscl_numcalls_before_inc=%d\n",
			__LINE__,Cs_sm_cb->css_clock.cscl_fixed_update,out_of_date,Cs_sm_cb->css_clock.cscl_numcalls_before_inc) ;
#endif /*  bolke01_diag_high */
	}
	else
	{
	    Cs_sm_cb->css_clock.cscl_fixed_update = 
		Cs_sm_cb->css_clock.cscl_fixed_update / 2;
	}
	CS_ADD_QUANTUM(out_of_date);
    }
    else if (out_of_date > 0)
    {
	if ( 1 == Cs_sm_cb->css_clock.cscl_numcalls_before_inc )
	    Cs_sm_cb->css_clock.cscl_fixed_update +=  
		    Cs_sm_cb->css_clock.cscl_fixed_update;
	else
	    Cs_sm_cb->css_clock.cscl_numcalls_before_inc = 
		    Cs_sm_cb->css_clock.cscl_numcalls_before_inc / 2;
	CS_ADD_QUANTUM(out_of_date);
    }

    Cs_lastquant += out_of_date;		/* keep threads in sync */

    Cs_sm_cb->css_clock.cscl_secs = clock_current.cscl_secs;
    Cs_sm_cb->css_clock.cscl_msecs = clock_current.cscl_msecs;
    Cs_sm_cb->css_clock.cscl_qcount = Cs_sm_cb->css_qcount;

    return(out_of_date);
}
/*{
** Name: CS_clockinit()	- initialize the shared memory clock
**
** Description:
**	Initialize the shared memory clock.  Store the current
**	time and initialize the control members to defaults.
**
** Inputs:
**	sm				point to shared memory control
**
** Outputs:
**	none
**
**	Returns:
**	    void
**
** History:
**      24-apr-94 (mikem)
**          Created.
**	24-oct-1994 (canor01)
**		B64168: CS_clockinit() was referencing undefined clock_current
**		if xCL_005_GETTIMEOFDAY_EXISTS was not defined.
**      28-July2003 (hanal04) Bug 110468 INGSRV 2367
**          Initialise the new cs_clock field cscl_calls_since_idle.
*/
void
CS_clockinit(
CS_SMCNTRL	*sm)
{
    HRSYSTIME	t;

    sm->css_qcount = 0;
    sm->css_clock.cscl_qcount = 0;
    sm->css_clock.cscl_calls_since_upd = 0;
    sm->css_clock.cscl_fixed_update = 1;
    sm->css_clock.cscl_numcalls_before_update = 1000;
    sm->css_clock.cscl_numcalls_before_inc = 1;
    sm->css_clock.cscl_calls_since_inc = 0;
    sm->css_clock.cscl_calls_since_idle = 0;

    TMhrnow( &t );
    sm->css_clock.cscl_secs = t.tv_sec;
    sm->css_clock.cscl_msecs = t.tv_nsec / 1000000;
}
