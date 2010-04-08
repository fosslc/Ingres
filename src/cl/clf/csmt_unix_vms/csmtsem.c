/*
** Copyright (c) 1994, 2005 Ingres Corporation All Rights Reserved.
*/

# include <compat.h>
# ifdef OS_THREADS_USED
# include <gl.h>
# include <clconfig.h>
# include <systypes.h>
# include <clsigs.h>
# include <cs.h>
# include <me.h>
# include <mo.h>
# include <pc.h>
# include <st.h>
# include <tr.h>
# include <me.h>
# include <ercl.h>

# include <csinternal.h>
# include <iinap.h>
# ifdef xCL_NEED_SEM_CLEANUP
# include <csev.h>
# include <cssminfo.h>
# endif

# include "csmtmgmt.h"
# include "csmtlocal.h"

/**
**
**  Name: CSMTSEM.C - module containing CS semaphore routines.
**
**  Description:
**	Contains all routines which deal with CS semaphore operations.  
**	Broken out of csinterface.c to give compiler optimizers a chance
**	at this file (csinterface.c is nooptim on some platforms because of 
**	some stack manipulations done be some other routines).
**
**	external functions:
**
**      CSMTp_semaphore() - Perform a "P" operation, claiming the semaphore
**	    CSMTv_semaphore() - Perform a "V" operation, releasing the semaphore
**	    CSMTi_semaphore() - Initialize Semaphore.
**	    CSMTa_semaphore() - attach to a CS_SEM_MULTI semaphore
**	    CSMTr_semaphore() - Remove a semaphore
**	    CSMTd_semaphore() - detach semaphore.
**	    CSMTn_semaphore() - give a name to a semaphore
**	    CSMTw_semaphore() - Initialize and Name a semaphore
**			      (CSi_ + CSn_) in one operation.
**	    CSMTs_semaphore() - Return semaphore statistics.
**
**	static functions:
**
**	    CSMT_sem_owner_dead() - determine if process holding semaphore is dead
**
**  History:
**      25-apr-94 (mikem)
**	    Created.  Moved code from csinterface.c.
**  	17-May-94 (daveb) 59127
**  	    Fix stat initialization, add code to debug leaks.
**  	    Semaphores are not attached for IMA stat purposes
**  	    until they are named.  
**	24-May-1994 (fredv)
**	    Removed the extra right ')' that cause compilation error in
**		CSa_semaphore.
**	31-mar-1995 (canor01)
**	    added include of <me.h>
**	11-May-1995 (jenjo02)
**	    Substantially modified spin logic to dispatch another
**	    thread instead of napping. General code cleanup, 
**	    hole plugging (i.e., don't leave inkernel set, ever).
**	    Added CSw_semaphore(), which combines the functionality
**	    of CSi and CSn to initialize AND name a semaphore with
**	    one call.
**	25-May-1995 (jenjo02)
**	    Renamed CSp|v_semaphore function entry point names
**	    to CSp|v_semaphore_function to distinguish them from
**	    macros with the same names. CSp|v_semaphore_function
**	    are defined in cs.h .
**	05-Jun-1995 (jenjo02)
**	    Added support for shared cross-process semaphores.
**	    Altered exclusive->shared collisions to block additional
**	    shares while exclusive request is pending.
**	    Changed MP spin-dispatch to be somewhat "self-tuning".
**	    Instead of always spinning Cs_max_sem_loops-times before
**	    redispatch, compute the average number of historic
**	    spins-before-grant and use that number. That number 
**	    will vary over the life of the server and the specific
**	    semaphore, so it should be more useful than the 
**	    fixed Cs_max_sem_loops number.
**	21-Sep-1995 (jenjo02)
**	    In CSv_semaphore(), spin on shared cross-process
**	    semaphore until atomic set is acquired; other processes
**	    may be updating the semaphore (and holding the latch)
**	    at the same time. Also cleaned up handling of inkernel
**	    in CSv_semaphore().
**	07-Dec-1995(jenjo02)
**	    Defined CSs_semaphore() function which returns selected
**	    statistics for a given semaphore.
**	03-jun-1996 (canor01)
**	    Initial implementation of OS semaphores for use with
**	    OS threads.
**	21-Sep-1996 (jenjo02)
**	    Removed code that gave preferential treatment to EXCL
**	    waiters on a SHARED semaphore. Previously we queued new
**	    SHARE requests if there were EXCL waiters on a SHARE-held
**	    semaphore. Because we have no control over which thread
**	    gets dispatched by CS_cond_signal(), we can't be sure
**	    that the EXCL threads will be awoken when we'd like them to.
**	09-Oct-1996 (jenjo02)
**	    CS_sem_attach(), CS_detach_sem only CS_SEM_MULTI type semaphores.
**	26-Oct-1996 (jenjo02)
**	    If sem appears to already be owned by requesting thread,
**	    recheck it a few times before giving an error.
**	    Clear sem cs_sid, cs_sid in the same order in which they're
**	    assigned in CSp_semaphore().
**	13-Nov-1996 (jenjo02)
**	    Changed CS_cond_timedwait() to CS_cond_wait() while awaiting
**	    EXCL access to a SHARED semaphore. There appears to be no
**	    reason to periodically wake up.
**	18-dec-1996 (canor01)
**	    Added status parameter to CS_cp_synch_init and CS_cp_cond_init
**	    macros.
**	20-Dec-1996 (jenjo02)
**	    In CSv_semaphore(), check for excl_waiters and signal the
**	    condition for exclusively-held sems as well as shared-held sems.
**	29-Jan-1997 (wonst02)
**	    Use CS_get_thread_id() instead of scb->cs_self if scb is NULL in
**	    CSp_semaphore().
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmtsem.c and moved all calls to CSMT...
**	19-Feb-1997 (jenjo02)
**	    Do not register semaphores with MO; delete calls to
**	    CS_sem_attach() and CS_detach_sem().
**      07-mar-1997 (canor01)
**          Make all functions dummies unless OS_THREADS_USED is defined.
**	14-Apr-1997 (bonro01)
**	    Added code to get the error number from 'errno' instead
**	    of from the posix functions return value for dg8_us5.
**	17-apr-1997 (canor01)
**	    Reduce calls to PCpid().
**	16-may-1997 (muhpa01)
**	    Make POSIX_DRAFT_4 change due to differences in CS_synch_trylock()
**	    return codes.  Also, modify declarations for CS_cp_cond_wait() and
**	    CS_cp_cond_signal for hp8_us5 to quiet compiler complaint
**      08-jul-1997 (canor01)
**          Maintain semaphores on known semaphore list.
**      25-jul-1997 (canor01)
**          Jasmine may free memory where semaphore is allocated before
**          removing the semaphore.  This can play havoc with the known
**          semaphore list.
**	05-aug-1997 (canor01)
**	    Restore known semaphore list for Jasmine.
**	06-aug-1997 (canor01)
**	    Add debugging statements to trace known semaphore list
**	    problems.
**	07-aug-1997 (canor01)
**	    Temporarily disable the known semaphore list.
**	19-aug-1997 (canor01)
**	    Make known semaphore list dependent on xCL_NEED_SEM_CLEANUP.
**	19-Aug-1997 (hweho01)
**	    Extended the fix on 14-apr-1997 by bonro01 to dgi_us5.
**	12-Sep-1997 (jenjo02)
**	    Added sanity check of semaphore to CSp_semaphore(), CSr_semaphore().
**	    Added code to preempt additional SHARE requests if there are
**	    EXCL waiters to prevent starving EXCL requests.
**	    Don't allow CSi_semaphore(), CSw_semaphore() to reinitialize
**	    an already initialized semaphore.
**	    If a semaphore already has a name, don't let CSn_semaphore()
**	    overwrite it.
**	03-nov-1997 (canor01)
**	    Do not try to remove a semaphore that was never initialized.
**	10-Dec-1997 (jenjo02)
**	    Installed check for dead semaphore owner. UNIX fails to notify
**	    mutex waiters that the owning process of the mutex has died, and
**	    they will wait forever. This is a feature, but leads to the RCP
**	    or servers hanging when a mutex-holding process dies. Nothing can
**	    be done but forcibly kill the hung process(es). To minimize the
**	    possibility that this will happen, CSMTp_semaphore() will now
**	    utilize a trylock spin loop if the apparent mutex owner is in
**	    a separate process, and will periodically check to see if the 
**	    owning process has died. RCP is an exception; if RCP is the mutex
**	    owner, we'll commit to using a synch_lock (which may wait forever)
**	    instead of the more expensive trylock loop, reasoning that if the
**	    RCP dies, all of the servers are dead anyway. Note that there's a
**	    window between the time that the decision is made to use a trylock
**	    loop or commit to a sync_lock; if we elect to commit to a synch_lock,
**	    the real mutex owner may change to another process and that process
**	    may die - we'll be left in a permanent synch_lock wait, but that 
**	    window is fairly small and judged an acceptable risk given that
**	    there's nothing else we can do.
**  26-Feb-1998 (bonro01)
**      Modified semaphore routines to chain multi
**      semaphores off the Cs_sm_cb shared memory instead
**      of the Cs_srv_block shared memory.
**	17-Apr-1998 (merja01)
**		Move preprocessor "#" to column 1 to fix compile errors
**		on Digital UNIX axp_osf.		
**	10-Jun-1998 (schte01)
**	    Modified CSMTv_semaphore to test for SIMULATE_PROCESS_SHARED before
**          doing a CS_synch_lock on sem->cs_mutex. The unlock of the mutex had
**          did an unlock using CS_cp_synch_unlock for a cross-process mutex
**          and got an CS000A_NO_SEMAPHORE error.
**	16-Jun-1998 (schte01)
**          Removed erroneous code from CSMTv_semaphore (per jenjo02).
**	12-Aug-1998 (schte01)   
**       In the loop that's executed when the mutex is blocked (EBUSY),
**	    add test for PROCESS_SIMULATE_SHARED to call CS_cp_synch_lock 
**       if trying to get a cross-process semaphore in the case where 
**       the process that has the semaphore is the same as the process
**       requesting the semaphore.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
**	    Don't skew stats by counting share vs share temporary collisions;
**	    count only share vs excl.
**	16-Nov-1998 (jenjo02)
**	    Upped spin retry count from 10 to 5000 when checking for 
**	    apparent pSEM deadlock - 10 wasn't enough time.
**	02-Dec-1998 (muhpa01)
**	    Removed code for POSIX_DRAFT_4 for HP - obsolete
**	23-Dec-1998 (schte01)   
**       Corrected coding of else statement in CSMTp_semaphore so that 
**       ...cond_signal is only issued on behalf of the thread requesting
**       the mutex in SHARE. This was causing a hang of the system when the  
**       mutex was gotten in SHARE and there was more than one EXCL waiter.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	18-Jan-1999 (bonro01)
**	    Added code to get proper errno for POSIX DRAFT6 on DGUX.
**      22-Jun-1999 (podni01)
**          Added rmx_us5 to the list of the platforms to use "errno" as
**          a return code instead of return value from the posix functions.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	21-Oct-1999 (jenjo02)
**	    CSMTget_scb() only when we must, like when the underlying
**	    mutex is blocked and we must change scb->cs_state. Drilling
**	    down to find the thread's SCB turns out to be fairly 
**	    expensive when one considers the number of CSp/v calls.
**	    Instead of the semaphore owner (sem->cs_sid) being
**	    scb->cs_self (which requires finding the SCB), the
**	    thread_id is used instead, which can be had by a quick
**	    CS_get_thread_id() call.
**	15-Nov-1999 (jenjo02)
**	    Removed dubious cs_sem_init_pid, cs_sem_init_addr from
**	    CS_SEMAPHORE, replaced cs_sid with cs_thread_id.
**	28-dec-1999 (devjo01)
**	    Fix test for semaphore self deadlock. (b97886).
**      08-Dec-1999 (podni01)  
**          Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**          with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**          this code for some reason removed all over the place.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Replaced CSget_scb() with CSMTget_scb().
**	10-jul-2001 (toumi01)
**	    At least on ibm_lnx (pthreads and SIMULATE_PROCESS_SHARED)
**	    we can have two processes preempted waiting for a mutex:
**		#1 - wants EXCL but sem is held SHARE
**		#2 - wants SHARE but an EXCL is pending
**	    Process #2 calls CS_cp_synch_lock and loops yielding the
**	    processor and trying to reacquire the mutex.
**	    Process #1 (which holds the semaphore's mutex) loops forever:
**		while (we don't own the mutex with no EXCL pending)
**		    call CS_cp_cond_signal(&sem->cs_cp_cond)
**			set sem->cs_cp_cond to 1
**		    ...
**		    call CS_cp_cond_wait(&sem->cs_cp_cond, &sem->cs_test_set); 
**			if (sem->cs_cp_cond) return(OK)
**			release mutex
**			yield process
**			wait to re-acquire mutex
**		    ...
**	    The whole SIMULATE_PROCESS_SHARED mechanism seems highly
**	    questionable in this situation, but threads being a black
**	    art and machine-specific I've restrained myself from doing
**	    a re-write.  For Linux call a (new) routine that disregards
**	    sem->cs_cp_cond and just does the release-yield-reacquire
**	    on the mutex.  The downside is that we could wait forever if
**	    a process dies holding the mutex, but in the (only) context
**	    where this was seen (dsa multiwrite test) the (dead) owner
**	    would be dmfrcp, in which case we'd be out of luck anyway.
**	    The area of code is question can be found by searching for
**	    this comment (already in the code; don't blame me!):
**		"This code is pretty chancy, maybe even bogus"
**	11-jul-2000 (toumi01)
**	    If SIMULATE_PROCESS_SHARED and _POSIX_THREAD_PROCESS_SHARED
**	    are both defined, force a compile error.  If the machine
**	    supports POSIX.4 Process-shared synchronization then we should
**	    be using it.
**	03-Dec-2001 (hanje04)
**	   Added IA64 Linux to list of platforms using CS_cp_synch_yield 
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	10-Dec-2003 (hanal04) Bug 111380 INGSRV2621
**          Prevent excessive, wasted trylocks on a semaphore for which
**          we have a very high level of contention.
**	7-Apr-2004 (schka24)
**	    Unconditionally init single-process sems, but trace any
**	    which look like they might have been inited already.
**	19-jan-2005 (devjo01)
**	    Correct faulty formatting specs when reporting possible
**	    reinitialization of semaphores.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	17-Oct-2007 (hanje04)
**	    BUG 114907
**	    _POSIX_THREAD_PROCESS_SHARED can be defined to -1 and therefore
**	    still not be supported. This is the case with Xtools 2.x on
**	    Mac OSX
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    _POSIX_THREAD_PROCESS_SHARED is now correctly defined in OS X
**	    10.5 (Panther) but it doesn't appear to work properly. Disable
**	    check for that and SIMULATE_PROCESS_SHARED until problems are
**	    resolved.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/

/*
**  Forward and/or External typedef/struct references.
*/

static bool CSMT_sem_owner_dead(
	    CS_SEMAPHORE    *sem,
	    CS_SCB          *scb);

GLOBALREF CS_SYSTEM           Cs_srv_block;
GLOBALREF i4		      Cs_incomp;
GLOBALREF i4             Cs_numprocessors;
GLOBALREF i4             Cs_max_sem_loops;
GLOBALREF i4             Cs_desched_usec_sleep;

# ifdef SIMULATE_PROCESS_SHARED
# ifndef OSX 
/*
** FIXME!!
** don't check for POSIX_THREAD_SUPPORT on OSX, it's there but doesn't work
*/
# if defined(_POSIX_THREAD_PROCESS_SHARED) && _POSIX_THREAD_PROCESS_SHARED > 0
Do not use SIMULATE_PROCESS_SHARED:
POSIX.4 Process-shared synchronization is supported on this machine
# endif
# endif
# if defined(any_hpux)
int CS_cp_cond_wait();
int CS_cp_cond_signal();
int CS_cp_synch_yield();
# else
int CS_cp_cond_wait( CS_CP_COND *, CS_ASET *);
int CS_cp_cond_signal(CS_CP_COND *);
int CS_cp_synch_yield( CS_ASET *);
# endif /* hpux */
# endif
/*
**  Defines of other constants.
*/

/*
** Number of spin loops to execute before degredating to 1 second sleeps.
** This value has been chosen such that no correctly configured system should
** ever reach this value.  This case is most likely to happen if the owner
** of a semaphore goes into an infinite loop.  Reaching this number of loops
** indicates a bug in the system somewhere.
** Currently set to 100,000 loops (about 1 million instructions).
*/

/* Cs_max_sem_loops has deprecated this value */
# define                        CS_MAX_SEM_LOOPS    100000

# define		MAX_KILL_LOOPS	500 /* How often to check for dead owner */
# define		MAX_YIELD_LOOPS	10  /* How often to yield to another thread */


# ifdef SEM_LEAK_DEBUG

/* called when a sem is seen in the badlist */
void
CSMT_sem_badlist( CS_SEMAPHORE *sem )
{
    /* breakpoint here! */
}

/* fill in entries with the debugger, maybe */

CS_SEMAPHORE *badlist[] =
{
	(CS_SEMAPHORE *)0,
	(CS_SEMAPHORE *)0,
	(CS_SEMAPHORE *)0,
};

# define BADSEMS (sizeof(badlist) / sizeof(badlist[0]))

CSMT_sem_checkbad( CS_SEMAPHORE *sem )
{
    i4  i;

    for( i = 0; i < BADSEMS ; i++ )
	if( sem == badlist[ i ] )
	    CSMT_sem_badlist( sem );
}

# endif


/*{
** Name: CSMTp_semaphore()	- perform a "P" operation on a semaphore
**
** Description:
**	This routine is called to allow a thread to request a semaphore
**	and if it is not available, to queue up for the semaphore.  The
**	thread requesting the semaphore will be returned to the ready
**	queue when the semaphore is V'd.
**
**	At the time that the semaphore is granted, the fact that a semaphore
**	is held by the thread is noted in the thread control block, to prevent
**	the stopping of the thread (via the monitor) while a semaphore is held.
**
**	Shared semaphores refer to an extension of the semaphore concept to
**	include a more general software lock.  This mechanism is used to
**	facilitate a more efficient reference count mechanism.
**
**	If the requested semaphore has a type CS_SEM_MULTI then that semaphore
**	may be located in shared memory and the granting/blocking on this
**	semaphore must work between multiple processes.  CS_SEM_MULTI semaphores
**	are expected to model spin locks and should only be held for short
**	periods of time.  Clients should never do operations which may cause
**	their threads to be suspended while holding a CS_SEM_MULTI semaphore.
**
**	All semaphores should be initialized with a CSi_semaphore call before
**	being used by CSp_semaphore().
**
** Inputs:
**	exclusive			Boolean value representing whether this
**					request is for an exclusive semaphore
**	sem				A pointer to a CS_SEMAPHORE for which
**					the thread wishes to wait.
**
** Outputs:
**	none
**	Returns:
**	    OK				- semaphore granted.
**	    E_CS000F_REQUEST_ABORTED	- request interrupted.
**	    E_CS0017_SMPR_DEADLOCK	- semaphore already held by requestor.
**          E_CS0029_SEM_OWNER_DEAD     - owner process of semaphore has exited.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-Nov-1986 (fred)
**	    Created.
**	14-jan-1987 (fred)
**	    Added shared/exclusive boolean.
**	11-Nov-1987 (fred)
**	    Add support for semphore collision collection & reporting.
**	01-oct-88 (eric)
**	    Added checks for ASTs that are disabled or in progress.
**	 7-nov-88 (rogerk)
**	    Call CS_ssprsm directly instead of through AST's.
**	    Added new inkernel protection.  Set cs_inkernel flag in both
**	    the server block and scb to make the routine behave as if atomic.
**	    When done, turn off the inkernel flags and check to see if we
**	    CS_move_async is needed.
**	12-jun-89 (rogerk)
**	    Added cross-process semaphore support.  Loop until lock is granted.
**	31-Oct-89 (anton)
**	    Added quantum support
**	23-mar-1990 (mikem)
**	    Changes to increase xact throughput for multiprocessors.  LG/LK
**	    cross process semaphores are now built on top of the cross process
**	    code for regular CS{p,v}_semaphores.  Fixes to this file were
**	    necessary to make these semaphores work correctly when called from
**	    the dmfrcp, rather than the server.
**
**	    - Changed the code to refer to "Cs_srv_block.cs_pid" rather than
**	      to "scb->cs_pid" when it wanted the current process' pid.  The
**	      old method did not work for some situations in non-dbms processes.
**	    - Some code refered to the "scb" control block to keep track of
**	      performance stats.  This code was changed to not be called in
**	      the cases where the "scb" did not exist (was NULL).
**	    - Changed code to handle special case of Cs_incomp and multiprocess
**	      semaphores.  If this case happens we spin until we get the
**	      resource as switching will result in recursive scheduling.
**	02-apr-1990 (mikem)
**	    If the state is CS_INITIALIZING then this is being called
**	    from a non-dbms server (ie. the rcp, acp, createdb, ...).
**	    We never want to call CSsuspend() from CSp_semaphore in this case,
**	    so the code will spin on the semaphore instead.
**	31-may-90 (blaise)
**	    Integrated changes from termcl:
**		Changes to make CSp_semaphore return an error if a multiprocess
**		semaphore is held by a process which has exited (and so will
**		never be granted). Add new routine CS_sem_owner_dead(), and
**		call it to check whether the process owner still exists
**		(after spinning for some amount of time).
**	14-dec-90 (mikem)
**	    Do not spin on cross process semaphores when running on a single
**	    processor machine.  Call II_nap() instead.
**	30-may-1991 (bryanp)
**	    B37501: Call II_nap, rather than CSsuspend, if CSp_semaphore() spins
**	    too long on a CS_SEM_MULTI-type semaphore. CSp_semaphore is at times
**	    called from places where a CSresume may be pending or about to
**	    occur; we cannot safely CSsuspend in order to block the thread for
**	    a second because the CSsuspend call may "eat" the pending CSresume
**	    and the CSresume would be lost, leading to a "hung" thread.
**	25-sep-1991 (mikem) integrated following change: 19-aug-1991 (rogerk)
**	    Changed spin lock in cross process semaphore code to try to
**	    avoid tying up the memory bus with interlock instructions by
**	    looping on the test/set call.  We now loop just looking for
**	    when the semaphore becomes free.  Only when we see it is free
**	    do we try to grab it with a test/set call.  This "snooping"
**	06-apr-1992 (mikem)
**	    SIR #43445
**	    Changed the looping properties of CSp_semaphore() when operating
**	    on a cross process semaphore.  The code now loops Cs_max_sem_loops
**	    (defaults to 200) loops before going into II_yield_cpu_time_slice().
**	17-jul-1992 (bryanp)
**	    Once we start yielding our time slice, check periodically to see if
**	    the semaphore owner has died.
**	23-Oct-1992 (daveb)
**	    prototyped
**	20-sep-93 (mikem)
**	    Bug #66616
**	    Added support for new, per semaphore, semaphore contention 
**	    statistics gathering to CSp_semaphore().  Also added a few
**	    new statistics fields to the server level, semaphore contention
**	    statistics.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointers.
**      17-May-1994 (daveb) 59127
**  	    Add ifdef-ed out leak-diagnostic call
**	11-May-1995 (jenjo02)
**	    Changed multi-process waits to CS_swuser() after spin
**	    count is exhausted rather than napping the entire 
**	    server, in the hope that there's some useful work to be
**	    done while we "wait" for the semaphore to become free.
**
**	    Also invented CSp|v_semaphore macros to front-end function 
**	    calls; the only time the functions contained in here
**	    will get invoked will be when the macro detects a collision
**	    or anomalous condition, all in an effort to decrease the
**	    cost of acquiring and freeing a semaphore which, in all
**	    but blocked conditions, should be virtually free.
**	25-May-1995 (jenjo02)
**	    Changed function name to CSp_semaphore_function. Macro is
**	    also named CSp_semaphore for invisibility's sake;
**	    it calls CSp_semaphore_function().
**	05-Jun-1995 (jenjo02)
**	    Added support for shared cross-process semaphores.
**	
**	    Altered exclusive->shared collisions to block additional
**	    shares while exclusive request is pending.
**	 
**	    Changed MP spin-dispatch to be somewhat "self-tuning".
**	    Instead of always spinning Cs_max_sem_loops-times before
**	    redispatch, compute the average number of historic
**	    spins-before-grant and use that number. That number 
**	    will vary over the life of the server and the specific
**	    semaphore, so it should be more useful than the 
**	    fixed Cs_max_sem_loops number.
**	03-jun-1996 (canor01)
**	    Added version using operating system semaphores.
**	26-Oct-1996 (jenjo02)
**	    If sem appears to already be owned by requesting thread,
**	    recheck it a few times before giving an error.
**	13-Nov-1996 (jenjo02)
**	    Changed CS_cond_timedwait() to CS_cond_wait() while awaiting
**	    EXCL access to a SHARED semaphore. There appears to be no
**	    reason to periodically wake up.
**	29-Jan-1997 (wonst02)
**	    Use CS_get_thread_id() instead of scb->cs_self if scb is NULL.
**	18-Mar-1997 (kosma01)
**	    Implement semaphores for PTHREAD machines that don't support
**	    PTHREAD_PROCESS_SHARED, like rs4_us5
**	17-Jun-1997 (kosma01 for kinpa04/merja01)
**	    Remove not "!" from CS_cp_synch_trylock for
**	    SIMULATE_PROCESS_SHARED.   The was corrupting the return
**	    value of CS_cp_synch_trylock on axposf. (and rs4_us5,(kosma01))
**	12-Sep-1997 (jenjo02)
**	    Added sanity check of semaphore to CSp_semaphore(), CSr_semaphore().
**	    Added code to preempt additional SHARE requests if there are
**	    EXCL waiters to prevent starving EXCL requests.
**	10-Dec-1997 (jenjo02)
**	    Installed check for dead semaphore owner. See large comment above.
**	12-Aug-1998 (schte01)   
**       In the loop that's executed when the mutex is blocked (EBUSY),
**	    add test for PROCESS_SIMULATE_SHARED to call CS_cp_synch_lock 
**       if trying to get a cross-process semaphore in the case where 
**       the process that has the semaphore is the same as the process
**       requesting the semaphore.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
**	16-Nov-1998 (jenjo02)
**	    Upped spin retry count from 10 to 5000 when checking for 
**	    apparent pSEM deadlock - 10 wasn't enough time.
**	23-Dec-1998 (schte01)   
**       Corrected coding of else statement in CSMTp_semaphore so that 
**       ...cond_signal is only issued on behalf of the thread requesting
**       the mutex in SHARE. This was causing a hang of the system when the  
**       mutex was gotten in SHARE and there was more than one EXCL waiter.
**	21-Oct-1999 (jenjo02)
**	    CSMTget_scb() only when we must, like when the underlying
**	    mutex is blocked and we must change scb->cs_state. Drilling
**	    down to find the thread's SCB turns out to be fairly 
**	    expensive when one considers the number of CSp/v calls.
**	    Instead of the semaphore owner (sem->cs_sid) being
**	    scb->cs_self (which requires finding the SCB), the
**	    thread_id is used instead, which can be had by a quick
**	    CS_get_thread_id() call.
**	28-dec-1999 (devjo01)
**	    Fix test for semaphore self deadlock. (b97886).
**	10-Dec-2003 (hanal04) Bug 111380 INGSRV2621
**          Massive amounts of user and even more system CPU are wasted
**          on trylock calls when we have very high levels of mutex contention.
**          Client diagnostcs showed up to 90 threads all competing for the
**          same mutex.
**          Use changes in semaphore statistics to check for high levels of
**          contention on this mutex. With high numbers of threads
**          competing for the same mutex we must stop looping over trylock
**          calls and make a blocked request for the mutex.
**          This change removes yield_loops so that II_MAX_SEM_LOOPS loops
**          II_MAX_SEM_LOOPS times instead of an effective II_MAX_SEM_LOOPS *
**          MAX_YIELD_LOOPS times.
**      23-Apr-2004 (hanal04) Bug 112192 INGSRV2803
**          If there is only 1 CPU we should move to a blocked request
**          for the mutex immediately. Spinning on trylocks is guaranteed
**          to be a waste of time.
**      28-Sep-2004 (hanal04) Bug 113126 INGSRV2981
**          Only set sem->cs_pid to the tid/pid of the requesting session
**          once we have physically and logically acquired the semaphore.
**          In the case of shared requests do not set the tid/pid if
**          another shared owner has already done so.
**      07-May-2004 (hanal04) Bug 112192 INGSRV2803
**          If there is only 1 CPU we should move to a blocked request
**          for the mutex immediately. Spinning on trylocks is guaranteed
**          to be a waste of time.
**      18-Jan-2010 (hanal04) Bug 123148
**          Every 10 attempts to exclusively grab a mutex that is blocked
**          by ia single shared owner check to see if the owner is a known
**          and if so check to see whether the owner is dead. If it is, we
**          can safely put the mutex back into circulation to avoid a
**          hang.
*/
STATUS
CSMTp_semaphore(i4 exclusive, register CS_SEMAPHORE *sem)
{
     CS_SCB		*scb;
     i4			saved_state;
     STATUS		status;
     CS_THREAD_ID	tid;
     u_i4		req_limit;	/* Used for contention check */
     u_i4		*cur_req_cnt;   /* Value is inc's by other requestors */
     u_i4		yield_limit;    /* Used for contention checking */      
     u_i4		try_logical = 0;

     /* Get this thread's thread_id */
     CS_thread_id_assign( tid, CS_get_thread_id() );
    
    if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
	TRdisplay("%@ CSMTp_semaphore called with uninitialized semaphore, thread: %d\n",
		    tid);
	return( E_CS000A_NO_SEMAPHORE );
    }

    if (exclusive)
    {
        /* If Cs_numprocessors threads asked for this mutex after this 
        ** thread we have heavy mutex contention. 
        */
	sem->cs_smstatistics.cs_smx_count++;
        req_limit = sem->cs_smstatistics.cs_smx_count + Cs_numprocessors + 2;
        cur_req_cnt = &sem->cs_smstatistics.cs_smx_count;
	if (sem->cs_type == CS_SEM_MULTI)
	    Cs_srv_block.cs_smstatistics.cs_smmx_count++;
	else
	    Cs_srv_block.cs_smstatistics.cs_smsx_count++;
    }
    else
    {   
        /* If Cs_numprocessors threads asked for this mutex after this 
        ** thread we have heavy mutex contention. 
        */
	sem->cs_smstatistics.cs_sms_count++;
        req_limit = sem->cs_smstatistics.cs_sms_count + Cs_numprocessors + 2;
        cur_req_cnt = &sem->cs_smstatistics.cs_sms_count;
	if (sem->cs_type == CS_SEM_MULTI)
	    Cs_srv_block.cs_smstatistics.cs_smms_count++;
	else
	    Cs_srv_block.cs_smstatistics.cs_smss_count++;
    }

    /* If there are Cs_numprocessors * 2 yields on this semaphore after
    ** this thread tries to obtain it we have heavy mutex contention.
    */
    yield_limit = sem->cs_smstatistics.cs_smmp_count + Cs_numprocessors;

# ifdef SIMULATE_PROCESS_SHARED
	if ( sem->cs_type == CS_SEM_MULTI )
	    status = CS_cp_synch_trylock( &sem->cs_test_set );
	else
#if defined(DCE_THREADS)
	{
        /*  Under Posix draft 4, pthread_mutex_trylock() returns 1 if
        **  successful, 0 if mutex is locked, and -1 if there's an error
        */

        if ( !( status = CS_synch_trylock( &sem->cs_mutex ) ) )
                status = EBUSY;
        else if ( status == 1 )
                status = OK;
    	}
# else
        status = CS_synch_trylock( &sem->cs_mutex );
# endif /* DCE_THREADS */
	if ( status == OK )
#elif defined( DGUX )
	status = CS_synch_trylock( &sem->cs_mutex );
    if (status)
	status = errno;
#else
    status = CS_synch_trylock( &sem->cs_mutex );
# endif
    if ( status == OK )
    {
	if (exclusive)
	{
	    /* Want EXCL and we hold the mutex */
	    if ( sem->cs_count == 0 )
	    {
		sem->cs_value = 1;
		CS_thread_id_assign( sem->cs_thread_id, tid );
		sem->cs_pid = Cs_srv_block.cs_pid;
		return(OK);
	    }
	}
	/* Want SHARE and we hold the mutex */
	else if ( sem->cs_excl_waiters == 0 )
	{
	    sem->cs_count++;
	    /* If we are not tracking one of the shared holders
	    ** update the tid pid details.
            ** A tracked shared owner sets tid and pid to 0
	    ** when it releases it's shared in CSMTv_semaphore.
	    */
            if(sem->cs_pid == 0)
	    {
                CS_thread_id_assign( sem->cs_thread_id, tid );
                sem->cs_pid = Cs_srv_block.cs_pid;
            }

	    if ( sem->cs_count > sem->cs_smstatistics.cs_sms_hwm )
		sem->cs_smstatistics.cs_sms_hwm = sem->cs_count;
# ifdef SIMULATE_PROCESS_SHARED
	    status = ( sem->cs_type == CS_SEM_MULTI) 
		     ? ( CS_cp_synch_unlock( &sem->cs_test_set ) )
		     : ( CS_synch_unlock ( &sem->cs_mutex ) );
#if defined(rux_us5)
	    if (status) status = errno;
#endif
	    return( status );
#elif defined( DGUX )
	    status = CS_synch_unlock( &sem->cs_mutex );
	    if (status)
		status = errno;
	    return(status);
#else
	    return( CS_synch_unlock( &sem->cs_mutex ) );
# endif
	}
	/*
	** We hold the mutex but are being preemted because
	**
	**    1. Want EXCL but sem is held SHARE
	**    2. Want SHARE, but an EXCL is pending.
	**
	** Fall thru to release the mutex and wait for the
	** condition to be corrected.
	*/
	TRdisplay("%@ CSMTp_semaphore mutex %s preempted\n", sem->cs_sem_name);
	sem->cs_smstatistics.cs_smsx_count++;
	if (sem->cs_type == CS_SEM_MULTI)
	    Cs_srv_block.cs_smstatistics.cs_smmsx_count++;
	else
	    Cs_srv_block.cs_smstatistics.cs_smssx_count++;
    }
    else if (status == EBUSY)
    {
	/* Semaphore is blocked */
	if ( CS_thread_id_equal( sem->cs_thread_id, tid ) &&
	    sem->cs_pid == Cs_srv_block.cs_pid)
	{
	    /*
	    ** Thread apparently already owns the semaphore.
	    ** Since we're looking at the semaphore dirty,
	    ** check it a few more times to make sure
	    ** we're not looking at a semaphore in a transitive 
	    ** state which appeares to match our 
	    ** pid and tid.
	    */
	    i4 spin = 5000;
	    while ( CS_thread_id_equal( sem->cs_thread_id, tid ) &&
		    sem->cs_pid == Cs_srv_block.cs_pid &&
		    --spin );

	    if (spin == 0)
	    {
		/* This thread already owns the semaphore */
		CS_breakpoint();
		TRdisplay("%@ CSMTp_semaphore deadlocked, %s, thread: %d line: %d\n",
			    sem->cs_sem_name, tid, __LINE__);
		return(E_CS0017_SMPR_DEADLOCK);
	    }
	}
	if (exclusive)
	{
	    sem->cs_smstatistics.cs_smxx_count++;
	    if (sem->cs_type == CS_SEM_MULTI)
		Cs_srv_block.cs_smstatistics.cs_smmxx_count++;
	    else
		Cs_srv_block.cs_smstatistics.cs_smsxx_count++;
	}
	else
	{
	    /* Don't count shared vs shared collisions, just share vs excl */
	    if (sem->cs_value || sem->cs_excl_waiters)
	    {
		sem->cs_smstatistics.cs_smsx_count++;
		if (sem->cs_type == CS_SEM_MULTI)
		    Cs_srv_block.cs_smstatistics.cs_smmsx_count++;
		else
		    Cs_srv_block.cs_smstatistics.cs_smssx_count++;
	    }
	}
    }
    else
    {
	/* Semaphore is bogus */
	TRdisplay("%@ CSMTp_semaphore failed for %s, thread: %d line: %d status: %d\n",
		    sem->cs_sem_name, tid, __LINE__, status);
	return(E_CS000A_NO_SEMAPHORE);
    }

    /* 
    ** Mutex is blocked, we must wait.
    */

    /* Only now do we get a pointer to the sessions's SCB */
    CSMTget_scb(&scb);
    
    scb->cs_sync_obj = (PTR)sem;
    saved_state = scb->cs_state;
    scb->cs_state = CS_MUTEX;

    /* Don't retake the mutex if we already hold it (logical block) */
    if (status == EBUSY)
    {
	if (sem->cs_type == CS_SEM_MULTI &&
	    sem->cs_pid && sem->cs_pid != Cs_srv_block.cs_pid)
	{
	    i4		trylock_loops;
	    i4		kill_loops = 0;

	    do
	    {
		trylock_loops = 0;

		do
		{
		    /*
		    ** If the apparent mutex owner is in this process,
		    ** then we can commit to wait for it, otherwise
		    ** we must spin on a trylock loop to avoid waiting
		    ** forever should the other process die.
		    **
		    ** But if the apparent owner is the RCP, we can commit
		    ** to waiting for it because if the RCP dies, we
		    ** all die.
                    **
                    ** If we have yielded more than the limit or requested
                    ** more than the req_limit we should block.
		    */
                    if ((sem->cs_pid && sem->cs_pid != Cs_srv_block.cs_pid &&
                        sem->cs_pid != Cs_srv_block.cs_rcp_pid &&
                        Cs_numprocessors > 1) &&
                        (yield_limit > sem->cs_smstatistics.cs_smmp_count) &&
                        (req_limit > *cur_req_cnt))
# ifdef SIMULATE_PROCESS_SHARED
			status = CS_cp_synch_trylock( &sem->cs_test_set);
		    else
			status = CS_cp_synch_lock( &sem->cs_test_set);
# else
			status = CS_synch_trylock( &sem->cs_mutex );
		    else
			status = CS_synch_lock( &sem->cs_mutex );
# endif
#if defined( DGUX ) || defined(rmx_us5) || defined(rux_us5)
		    if (status)
			status = errno;
#endif /* defined( DGUX ) */
		    if (status == EBUSY)
		    {
			sem->cs_smstatistics.cs_smcl_count++;
			Cs_srv_block.cs_smstatistics.cs_smcl_count++;
		    }
		} while ((status == EBUSY) && 
			 (++trylock_loops < Cs_max_sem_loops) &&
                         (yield_limit > sem->cs_smstatistics.cs_smmp_count) &&
                         (req_limit > *cur_req_cnt));

		if (status == EBUSY)
		{
		    /* Every once in a while, check for a dead owner process */
		    if ( kill_loops++ > MAX_KILL_LOOPS )
		    {
			if ( CSMT_sem_owner_dead(sem, scb) )
			{
			    TRdisplay("%@ CSMTp_semaphore pid:%d thread:%d sem %s owner pid %d dead\n",
				    Cs_srv_block.cs_pid, tid,
				    sem->cs_sem_name, sem->cs_pid);
			    scb->cs_state = saved_state;
			    return(E_CS0029_SEM_OWNER_DEAD);
			}
			kill_loops = 0;
		    }
		    /* We've exhausted max sem loops, yield to another thread */
                    sem->cs_smstatistics.cs_smmp_count++;
                    Cs_srv_block.cs_smstatistics.cs_smmp_count++;
                    CS_yield_thread();
		}

	    } while (status == EBUSY);
	}
	else
	{
	    status =
# ifdef SIMULATE_PROCESS_SHARED
	(sem->cs_type == CS_SEM_MULTI)
	    ? (CS_cp_synch_lock( &sem->cs_test_set))
	    : (CS_synch_lock( &sem->cs_mutex)) ;
# else
	CS_synch_lock( &sem->cs_mutex );
# endif
#if defined( DGUX ) || defined(rmx_us5) || defined(rux_us5)
	    if (status)
		status = errno;
#endif /* defined( DGUX ) */
	}
    }

    /*
    ** Loop until the semaphore becomes logically available.
    **
    **   1. want EXCL, but sem is held SHARE
    **   2. want SHARE, but an EXCL is pending
    **
    ** A word on thread scheduling:
    **
    **   With UNIX threads, we have no way of assuring that a particular
    **   thread will get scheduled when we'd like it to, so we fiddle
    **   with the thread priorities here to try to prevent starvation.
    **
    **   When an EXCL is pending on a SHARE-held semaphore, other SHARE
    **   requests will wait until the EXCL is granted, so we urge the
    **   EXCL to get first crack by raising the priority of the EXCL
    **   waiter(s) and lowering the priority of the waiting SHAREs. In
    **   CSMT_semaphore(), a SHARE holder trying to release its interest
    **   of the semaphore is given the highest priority so that UNIX will
    **   schedule it ahead of all others and allow it to get the mutex,
    **   decrement the share count, and signal the condition.
    */
    while ( status == OK &&
	   (exclusive && sem->cs_count) ||
	   (!exclusive && sem->cs_excl_waiters) )
    {
	sem->cs_cond_waiters++;
        try_logical++;

	if ( exclusive )
	{
	    /*
	    ** EXCL waiting for SHARE count to drain. Up our thread
	    ** priority to increase our chances of getting scheduled
	    ** ahead of share requests that are waiting for this
	    ** EXCL request to be satisfied.
	    */
	    sem->cs_excl_waiters++;
	    if (scb->cs_priority < CS_LIM_PRIORITY)
		CS_thread_setprio( scb->cs_thread_id, CS_LIM_PRIORITY-1, &status);

            /* If a single shared owner exists and that owner is
            ** identifiable and that process has died, put the mutex 
            ** back into circulation. 
            ** We own the mutex so the sem pid/sid can be safely used.
            ** Only do this after every 10 attempts to get logical access
            ** to avoid unnecessary CSMT_sem_owner_dead() calls.
            */
            if( ((try_logical % 10) == 0) &&
                (sem->cs_count == 1) && 
                (sem->cs_thread_id != (CS_THREAD_ID)0) &&
                (sem->cs_pid != 0) &&
                CSMT_sem_owner_dead(sem, scb) )
            {
                sem->cs_count--;
                sem->cs_thread_id = (CS_THREAD_ID)0;
                sem->cs_pid = 0;
            }

	}
	else
	{
	    /*
	    ** SHARE being preempted by EXCL.
	    **
	    ** Lower our thread priority to give the EXCL waiter a better
	    ** chance of getting scheduled.
	    **
	    ** Wake up a higher-priority EXCL waiter, then wait for 
	    ** it to signal us, at which time we'll
	    ** re-test the condition.
	    */
	    if (scb->cs_priority > 0)
		CS_thread_setprio( scb->cs_thread_id, 0, &status);
# ifdef SIMULATE_PROCESS_SHARED
	    status = (sem->cs_type == CS_SEM_MULTI)
		   ? (CS_cp_cond_signal( &sem->cs_cp_cond))
		   : (CS_cond_signal( &sem->cs_cond ) ) ;
#if defined(rux_us5)
	    if (status) status = errno;
#endif
# else
	    status = CS_cond_signal( &sem->cs_cond );
# endif
	}

	if (status == OK)
	{
	    /*
	    ** If multi-process condition, we can't wait forever
	    ** for the condition to be signalled - the signalling process
	    ** might die and we'll never be signalled.
	    **
	    ** This code is pretty chancy, maybe even bogus, but
	    ** doesn't seem to cause any problems...
	    */
	    if (sem->cs_type == CS_SEM_MULTI)
	    {
		do
		{
		    CS_COND_TIMESTRUCT	ts;

		    ts.tv_sec = time(NULL);
		    ts.tv_sec += 5; /* Wait five seconds */
		    ts.tv_nsec = 0;

# ifdef SIMULATE_PROCESS_SHARED
# ifdef LNX 
		    status = CS_cp_synch_yield( &sem->cs_test_set ); 
# else
		    status = CS_cp_cond_wait( &sem->cs_cp_cond, &sem->cs_test_set); 
# endif
# else
		    status = CS_cond_timedwait( &sem->cs_cond,
						&sem->cs_mutex,
						&ts );
#if defined(rux_us5)
			if (status) status = errno;
#endif
# endif

		    /* If timed out, check for dead owner */
# ifdef xCL_USE_ETIMEDOUT
		    if ( status == ETIMEDOUT && 
# else
		    if ( status == ETIME && 
# endif
			 CSMT_sem_owner_dead(sem, scb) )
		    {
# ifdef SIMULATE_PROCESS_SHARED
			CS_cp_synch_unlock( &sem->cs_test_set);
# else
			CS_synch_unlock( &sem->cs_mutex );
# endif
			/* Reset this threads priority */
			if (scb->cs_priority < CS_LIM_PRIORITY)
			    CS_thread_setprio( scb->cs_thread_id, 
				    scb->cs_priority, &status);
			scb->cs_state = saved_state;
			return(E_CS0029_SEM_OWNER_DEAD);
		    }
# ifdef xCL_USE_ETIMEDOUT
		} while (status == ETIMEDOUT);
# else
		} while (status == ETIME);
# endif

		/* We now physically own the mutex, we will still loop
		** round on the outer loop if we can not logically acquire
		** the mutex.
		*/
	    }
	    else
		status = CS_cond_wait( &sem->cs_cond,
				       &sem->cs_mutex);
	}

	if ( exclusive )
	    sem->cs_excl_waiters--;

	sem->cs_cond_waiters--;

	/* Reset this threads priority */
	if (scb->cs_priority < CS_LIM_PRIORITY)
	    CS_thread_setprio( scb->cs_thread_id, scb->cs_priority, &status);
    }

    scb->cs_state = saved_state;

    if ( status == OK )
    {
	if ( exclusive )
	{
	    CS_thread_id_assign( sem->cs_thread_id, tid );
	    sem->cs_pid = Cs_srv_block.cs_pid;
	    sem->cs_value = 1;
	    return(OK);
	}
	else /* shared */
	{
	    if(sem->cs_pid == 0)
	    {
	        /* If we are not tracking one of the shared holders
	        ** update the tid pid details.
                ** A tracked shared owner sets tid and pid to 0
	        ** when it releases it's shared in CSMTv_semaphore.
	        */
	        CS_thread_id_assign( sem->cs_thread_id, tid );
	        sem->cs_pid = Cs_srv_block.cs_pid;
	    }
	    if ( ++sem->cs_count > sem->cs_smstatistics.cs_sms_hwm )
		sem->cs_smstatistics.cs_sms_hwm = sem->cs_count;

# ifdef SIMULATE_PROCESS_SHARED
		if (sem->cs_type == CS_SEM_MULTI)
			status = CS_cp_synch_unlock( &sem->cs_test_set);
		else    status = CS_synch_unlock( &sem->cs_mutex);
#if defined(rux_us5)
		if (status) status = errno;	
#endif
		return(status);
#elif defined( DGUX )
		status = CS_synch_unlock( &sem->cs_mutex );
		if (status)
		    status = errno;
		return(status);
#else
	    return( CS_synch_unlock( &sem->cs_mutex ) );
# endif
	}
    }
    else
    {
	CS_breakpoint();
	TRdisplay("%@ CSMTp_semaphore failed for %s, thread: %d line: %d, status=%d\n",
		    sem->cs_sem_name, tid, __LINE__, status);
	return(E_CS0017_SMPR_DEADLOCK);
    }
}

/*{
** Name: CSMTv_semaphore	- Perform a "V" operation, releasing the semaphore
**
** Description:
**	This routine is used by a running thread to release a semaphore which
**	it currently owns.  The act of doing this decrements the thread's
**	owned semaphore count.	It will allow the threads awaiting this
**	semaphore to try once again.
**
**	If the semaphore type is CS_SEM_MULTI then it is a cross process
**	semaphore.  The semaphore may be kept in shared memory and there may
**	be threads in other servers waiting on its release.
**
** Inputs:
**	sem				A pointer to the semaphore in question.
**
** Outputs:
**	none
**	Returns:
**	    OK
**	    E_CS000A_NO_SEMAPHORE if the task does not own a semaphore.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-Nov-1986 (fred)
**	    Created on Jupiter.
**	31-oct-1988 (rogerk)
**	    Make sure we don't exit without resetting cs_inkernel.
**	 7-nov-1988 (rogerk)
**	    Added new inkernel protection.  Set cs_inkernel flag in both
**	    the server block and scb to make the routine behave as if atomic.
**	    When done, turn off the inkernel flags and check to see if we
**	    CS_move_async is needed.
**	12-jun-89 (rogerk)
**	    Added cross-process semaphore support.
**	30-Jan-90 (anton)
**	    Fix idle job CPU spin in semaphore cases
**	23-mar-1990 (mikem)
**	    Changes to increase xact throughput for multiprocessors.  LG/LK
**	    cross process semaphores are now built on top of the cross process
**	    code for regular CS{p,v}_semaphores.  Fixes to this file were
**	    necessary to make these semaphores work correctly when called from
**	    the dmfrcp, rather than the server.
**
**	    - Changed the code to refer to "Cs_srv_block.cs_pid" rather than
**	      to "scb->cs_pid" when it wanted the current process' pid.  The
**	      old method did not work for some situations in non-dbms processes.
**	    - Some code refered to the "scb" control block to keep track of
**	      performance stats.  This code was changed to not be called in
**	      the cases where the "scb" did not exist (was NULL).
**	9-jul-92 (daveb)
**	    Add deferred attach of CS_MULTI semaphores.
**	23-Oct-1992 (daveb)
**	    prototyped
**      17-May-1994 (daveb) 59127
**  	    Add ifdef-ed out leak-diagnostic call
**	25-May-1995 (jenjo02)
**	    Changed function name to CSv_semaphore_function. Macro is
**	    also named CSv_semaphore for invisibility's sake;
**	    it calls CSv_semaphore_function().
**	05-Jun-1995 (jenjo02)
**	    Added support for shared cross-process semaphores.
**	21-Sep-1995 (jenjo02)
**	    In CSv_semaphore(), spin on shared cross-process
**	    semaphore until atomic set is acquired; other processes
**	    may be updating the semaphore (and holding the latch)
**	    at the same time. Also cleaned up handling of inkernel
**	    in CSv_semaphore().
**	26-Oct-1996 (jenjo02)
**	    Clear sem cs_sid, cs_sid in the same order in which they're
**	    assigned in CSp_semaphore().
**	20-Dec-1996 (jenjo02)
**	    In CSv_semaphore(), check for excl_waiters and signal the
**	    condition for exclusively-held sems as well as shared-held sems.
**	24-Oct-1996 (jenjo02)
**	    Check cs_value, cs_count before taking the synch lock.
**	10-Dec-1997 (jenjo02)
**	    Install check for dead SHARE owner process.
**	10-Jun-1998 (schte01)
**	    Check for SIMULATE_PROCESS_SHARED and do a CS_cp_synch_lock instead
**          of a CS_synch lock if so.
**	16-Jun-1998 (schte01)
**          Removed erroneous code from CSMTv_semaphore (per jenjo02).
**	21-Oct-1999 (jenjo02)
**	    Avoid CSget_scb() calls unless we're blocked and have to
**	    change scb->cs_state. Use thread_id to identify sem owner
**	    instead of scb->cs_self.
**      19-Mar-2004 (hanal04) Bug 112160 INGSRV2792
**          We can not safely check sem->cs_excl_waiters to determine the
**          need to increase our priority to CS_LIM_PRIORITY as an 
**          exclusive waiter may be about to increment the value from
**          zero to one. This could lead to an OS scheduling hang as
**          the exclusive waiter in CSMTp_semaphore continues to be 
**          scheduled ahead of the shared release call which is blocking it.
**      28-Sep-2004 (hanal04) Bug 113126 INGSRV2981
**          Do not set the sem->cs_pid if we have acquired the mutex to release
**          a shared lock. We'll immediately drop the shared count and release
**          the mutex.
*/
STATUS
CSMTv_semaphore(register CS_SEMAPHORE *sem)
{
    STATUS      	status;
    CS_THREAD_ID	tid;

    /* Get this thread's thread_id */
    CS_thread_id_assign( tid, CS_get_thread_id() );
    
    if (sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
	TRdisplay("%@ CSMTv_semaphore called with uninitialized semaphore, thread: %d\n",
		    tid);
	return( E_CS000A_NO_SEMAPHORE );
    }

    if (sem->cs_value)
    {
	/*
	** Held exclusively. Make sure this thread holds the
	** semaphore.
	*/
	if ( CS_thread_id_equal( sem->cs_thread_id, tid ) &&
	    sem->cs_pid == Cs_srv_block.cs_pid)
	{
	    sem->cs_value = 0;
	} 
	else
	    return(E_CS000A_NO_SEMAPHORE);
    }
    else /* shared */
    {
	if ( sem->cs_count == 0 )
	    return(E_CS000A_NO_SEMAPHORE);

	/* 
	** Before resorting to more convoluted measures, trylock
	** acquiring the mutex once. If we're the only sharer,
	** we might well get it.
	*/
# ifdef SIMULATE_PROCESS_SHARED
	if ( sem->cs_type == CS_SEM_MULTI )	
	    status = CS_cp_synch_trylock( &sem->cs_test_set );
	else
	    status = CS_synch_trylock( &sem->cs_mutex );
#elif defined( DGUX )
	status = CS_synch_trylock( &sem->cs_mutex );
	if (status)
	    status = errno;
#else
	status = CS_synch_trylock( &sem->cs_mutex );
# endif
	if ( status )
	{
	    i4		saved_state;
	    i4		reset_priority;
	    CS_SCB      *scb;

	    /* Didn't get it, get the session's SCB */
	    CSMTget_scb(&scb);
	    
	    saved_state = scb->cs_state;
	    reset_priority = 0;

	    scb->cs_sync_obj = (PTR)sem;
	    scb->cs_state = CS_MUTEX;

            /*
            ** There could be exclusive waiters, up our thread priority to
            ** the max so that we have the best possible chance of getting
            ** the mutex before a lower-priority EXCL waiter is scheduled.
            ** We can't safely check sem->cs_excl_waiters as we don't own
            ** the mutex. Assume the worst, even if there aren't exclusive
	    ** waiters quickly releasing critical resourcs is not a bad thing.
            */
            if (scb->cs_priority != CS_LIM_PRIORITY)
            {
                CS_thread_setprio( scb->cs_thread_id, CS_LIM_PRIORITY, &status);
                reset_priority = TRUE;
            }

            if (sem->cs_type == CS_SEM_MULTI &&
                sem->cs_pid && sem->cs_pid != Cs_srv_block.cs_pid)
            {
                i4                      trylock_loops;
                i4                      kill_loops = 0;
                i4                      yield_loops = 0;
 
                do
                {
                    trylock_loops = 0;
 
                    do
                    {
                        /*
                        ** If the apparent mutex owner is in this process,
                        ** then we can commit to wait for it, otherwise
                        ** we must spin on a trylock loop to avoid waiting
                        ** forever should the other process die.
                        **
                        ** But if the apparent owner is the RCP, we can commit
                        ** to waiting for it because if the RCP dies, we
                        ** all die.
                        */
                        if (sem->cs_pid && sem->cs_pid != Cs_srv_block.cs_pid &&
                            sem->cs_pid != Cs_srv_block.cs_rcp_pid)
# ifdef SIMULATE_PROCESS_SHARED
                            status = CS_cp_synch_trylock( &sem->cs_test_set);
                        else
                            status = CS_cp_synch_lock( &sem->cs_test_set);
# else
                            status = CS_synch_trylock( &sem->cs_mutex );
                        else
                            status = CS_synch_lock( &sem->cs_mutex );
# endif
#if defined( DGUX ) || defined(rmx_us5) || defined(rux_us5)
                        if (status)
                            status = errno;
#endif /* defined( DGUX ) */
                    } while (status == EBUSY &&
                             ++trylock_loops < Cs_max_sem_loops);
 
 
                    if (status == EBUSY)
                    {
                        /* Every once in a while, check for a dead owner process */
                        if (kill_loops++ > MAX_KILL_LOOPS)
                        {
                            if ( CSMT_sem_owner_dead(sem, scb) )
                            {
                                TRdisplay("%@ CSMTv_semaphore pid:%d thread:%d sem %s owner pid %d dead\n",
                                        Cs_srv_block.cs_pid, tid,
                                        sem->cs_sem_name, sem->cs_pid);
                                scb->cs_state = saved_state;
                                if (reset_priority)
                                {
                                    CS_thread_setprio( scb->cs_thread_id,
                                            scb->cs_priority, &status);
                                }
                                return(E_CS0029_SEM_OWNER_DEAD);
                            }
                            kill_loops = 0;
                        }
                        /* Every once in another while, yield to another thread */
                        if (yield_loops++ > MAX_YIELD_LOOPS)
                        {
                            /* Yield our time slice */
                            CS_yield_thread();
                            yield_loops = 0;
                        }
                    }
 
                } while (status == EBUSY);
            }
            else
            {
                status =
# ifdef SIMULATE_PROCESS_SHARED
                (sem->cs_type == CS_SEM_MULTI)
                    ? (CS_cp_synch_lock( &sem->cs_test_set))
                    : (CS_synch_lock( &sem->cs_mutex)) ;
# else
                CS_synch_lock( &sem->cs_mutex );
# endif
#if defined( DGUX ) || defined(rmx_us5) || defined(rux_us5)
                if (status)
                    status = errno;
#endif /* defined( DGUX ) */
            }
 
	    if (reset_priority)
		CS_thread_setprio( scb->cs_thread_id, scb->cs_priority, &status);

	    scb->cs_state = saved_state;

	    if (status)
		return(E_CS000A_NO_SEMAPHORE);
	}

	/* We now hold the mutex */
	if ( sem->cs_count-- == 0 )
	{
	    sem->cs_count = 0;
# ifdef SIMULATE_PROCESS_SHARED
	    (sem->cs_type == CS_SEM_MULTI)
		? (CS_cp_synch_unlock( &sem->cs_test_set))
		: (CS_synch_unlock( &sem->cs_mutex)) ;
# else
	    CS_synch_unlock( &sem->cs_mutex );
# endif
	    return(E_CS000A_NO_SEMAPHORE);
	}
    }

    if ( CS_thread_id_equal( sem->cs_thread_id, tid ) &&
	sem->cs_pid == Cs_srv_block.cs_pid)
    {
	sem->cs_thread_id = (CS_THREAD_ID)0;
	sem->cs_pid = 0;
    } 

    /*
    ** If there are any threads waiting on this 
    ** semaphore's condition, wake one of them up.
    */
    if ( sem->cs_cond_waiters )
    {
# ifdef SIMULATE_PROCESS_SHARED
	status = (sem->cs_type == CS_SEM_MULTI)
		   ? (CS_cp_cond_signal( &sem->cs_cp_cond))
		   : (CS_cond_signal( &sem->cs_cond ) ) ;
#if defined(rux_us5)
	if (status) status = errno;
#endif
        if (status)
# else
	if ( status = CS_cond_signal( &sem->cs_cond ) )
# endif
	{
# ifdef SIMULATE_PROCESS_SHARED
	    (sem->cs_type == CS_SEM_MULTI)
		? (CS_cp_synch_unlock( &sem->cs_test_set))
		: (CS_synch_unlock( &sem->cs_mutex ) ) ;
# else
	    CS_synch_unlock( &sem->cs_mutex );
# endif
	    return(E_CS000A_NO_SEMAPHORE);
	}
    }

# ifdef SIMULATE_PROCESS_SHARED
    if ( ((sem->cs_type == CS_SEM_MULTI)
	       ? (CS_cp_synch_unlock( &sem->cs_test_set))
	       : (CS_synch_unlock( &sem->cs_mutex ) ) ) == OK )
	
	
# else
    if ( CS_synch_unlock( &sem->cs_mutex) == OK )
# endif
        return(OK);     
    else
    {
        return(E_CS000A_NO_SEMAPHORE);
    }
}

/*{
** Name: CSMTi_semaphore	- Initialize Semaphore.
**
** Description:
**	This routine is used to initialize a semaphore for use by the
**	CSp_semaphore and CSv_semaphore routines.  This should always
**	be called before the first use of a semaphore and needs only
**	be called once.  It is illegal to call CSi_semaphore on a
**	semaphore that is currently in use.
**
** Inputs:
**      sem				A pointer to the semaphore to initialize
**	type				Semaphore type:
**					    CS_SEM_SINGLE - normal semaphore
**					    CS_SEM_MULTI  - semaphore may be
**						requested by threads in
**						different processes.
**
** Outputs:
**      none
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	    Illegal semaphore type specified.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-Jun-1989 (rogerk)
**          Created for Terminator.
**	4-jul-92 (daveb)
**	    Add to new cs_sem_tree, and clear out new name field.
**	9-Jul-92 (daveb)
**	    Sem_tree won't work, use CS_sem_attach scheme instead.
**	23-Oct-1992 (daveb)
**	    prototyped
**      17-May-1994 (daveb) 59127
**          Move attach to name; only named sems will be attached.
**  	    Initialize all fields to zero, and only explicitly fill
**  	    in those that may be non-zero.  This will clear out the
**  	    stats fields.  Add ifdef-ed out leak-diagnostic call
**	17-apr-1997 (canor01)
**	    Get the pid from the Cs_srv_block.
**      08-jul-1997 (canor01)
**          Add semaphores to known list.
**	12-Sep-1997 (jenjo02)
**	    If sem has already been initialized, don't do it again.
**	18-Jan-1999 (bonro01)
**	    Remove Duplicate CS_synch_unlock()
**	7-Apr-2004 (schka24)
**	    Can't always trust scribble check, so always init single-process
**	    sems;  trace any that look inited so we can find leaks.
**	19-jan-2005 (devjo01)
**	    Correct faulty formatting spec when reporting possible
**	    reinitialization of a semaphore.
*/
STATUS
CSMTi_semaphore(CS_SEMAPHORE *sem, i4  type)
{

    /*
    ** Get RCP's pid if we don't already know it.
    ** Note that the get_rcp_pid function won't be filled in
    ** if this is a pseudoserver or if it's still early in server startup.
    */
    if (Cs_srv_block.cs_rcp_pid == 0 && Cs_srv_block.cs_get_rcp_pid)
	Cs_srv_block.cs_rcp_pid = (*Cs_srv_block.cs_get_rcp_pid)();

    if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
	return (E_CS0004_BAD_PARAMETER);

#ifdef SEM_LEAK_DEBUG
    CSMT_sem_checkbad( sem );
#endif

    if (type == CS_SEM_SINGLE || sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
	/* Trace apparent re-init of single-process sems, if it's for
	** real (and not just unlucky garbage in the scribble check) we
	** have a resource leak.  (And if not, we have sloppy code somewhere.)
	*/
	if (sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD)
	    TRdisplay("%@ Possible reinit of sem '%.#s' (%x)\n",
		STnlength(sizeof(sem->cs_sem_name),sem->cs_sem_name),
		sem->cs_sem_name, sem);

	/* Clear everything out, including stats */
	MEfill( sizeof(*sem), 0, (char *)sem ); 

	/* Then do stuff that may be non-zero */
	sem->cs_type = type;
	if ( type == CS_SEM_SINGLE )
	{
	    if ( CS_synch_init( &sem->cs_mutex ) ||
		 CS_cond_init( &sem->cs_cond ) )
		return (E_CS0004_BAD_PARAMETER);
	}
	else
	{
	    STATUS status;
#     ifdef SIMULATE_PROCESS_SHARED
	    CS_cp_synch_init( &sem->cs_test_set, &status);
	    if ( status == OK )
		CS_cp_cond_init( &sem->cs_cp_cond, &status );
#     else
	    CS_cp_synch_init( &sem->cs_mutex, &status );
	    if ( status == OK )
		CS_cp_cond_init( &sem->cs_cond, &status );
#     endif
	    if ( status != OK )
		return (E_CS0004_BAD_PARAMETER);
	}
	sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

#     ifdef xCL_NEED_SEM_CLEANUP
    /* add to known lists */
    if ( type == CS_SEM_MULTI ) {
        int status;
        int i;
        char *key;
        void *addr;
        CS_SHM_IDS *new_shm = (void *)0;
        CS_SHM_IDS *found_shm = (void *)0;

        status = MEget_seg(sem, &key, &addr);
        if ( status == 0 )
        {
            QUr_init( &sem->cs_sem_list );
            CS_cp_synch_lock( &Cs_sm_cb->css_shm_mutex );
            for (i = 0; i < CS_SHMSEG; ++i)
            {
                if ( new_shm == (void *)0 && Cs_sm_cb->css_shm_ids[i].key[0] == 0 )
                    new_shm = &Cs_sm_cb->css_shm_ids[i];
                else if ( Cs_sm_cb->css_shm_ids[i].key[0] == 0 )
                    continue;
                else if ( STequal(key, Cs_sm_cb->css_shm_ids[i].key) ) {
                    found_shm = &Cs_sm_cb->css_shm_ids[i];
                    break;
                }
            }
            if ( found_shm == (void *)0 && new_shm )
            {
                STcopy(key, new_shm->key);
                found_shm = new_shm;
            }
            if ( found_shm )
            {
                QUr_insert( &sem->cs_sem_list, &found_shm->offset);
                found_shm->count++;
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Added CS_SEM_MULTI %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
            }
            else
            {
                TRdisplay("Exceeded shared memory semaphore structures\n");
            }
            CS_cp_synch_unlock( &Cs_sm_cb->css_shm_mutex );
        }
        else
        {
            QUinit( &sem->cs_sem_list );
            CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
            QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_multi_sem );
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Added CS_SEM_MULTI in LOCAL storage: %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
            CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
        }
    }
    else {
        QUinit( &sem->cs_sem_list );
        CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
        QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_single_sem );
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Added CS_SEM_SINGLE %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
        CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
    }
# endif /* xCL_NEED_SEM_CLEANUP */
    }

    return( OK );
}


/*{
** Name: CSMTw_semaphore	- Initialize and Name Semaphore
**			  in one felled swoop.
**
** Description:
**	This routine is used to initialize a semaphore for use by the
**	CSp_semaphore and CSv_semaphore routines.  This should always
**	be called before the first use of a semaphore and needs only
**	be called once.  It is illegal to call CSi_semaphore on a
**	semaphore that is currently in use.
**
**	Give the semaphore a name, up to CS_SEM_NAME_LEN characters.
**	If the name is too long, it will be silently truncated to fit.
**	The name will be copied, so the input may vanish after the call
**	returns.
**
** Inputs:
**      sem				A pointer to the semaphore to initialize
**	type				Semaphore type:
**					    CS_SEM_SINGLE - normal semaphore
**					    CS_SEM_MULTI  - semaphore may be
**						requested by threads in
**						different processes.
**	name				the name to give it
**
** Outputs:
**      none
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	    Illegal semaphore type specified.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Apr-1995 (jenjo02)
**          Created.
**	09-Oct-1996 (jenjo02)
**	    CS_sem_attach() only to CS_SEM_MULTI type semaphores.
**	17-apr-1997 (canor01)
**	    Get the pid from the Cs_srv_block.
**      08-jul-1997 (canor01)
**          Add semaphores to known list.
**	12-Sep-1997 (jenjo02)
**	    If sem has already been initialized, don't do it again.
**	7-Apr-2004 (schka24)
**	    Above has led to accidental non-init of sems, so always init
**	    single-process sems, and trace any that look like a reinit.
**	19-jan-2005 (devjo01)
**	    Correct faulty formatting spec when reporting possible
**	    reinitialization of a semaphore.
*/
STATUS
CSMTw_semaphore(CS_SEMAPHORE *sem, i4  type, char *string)
{

    /*
    ** Get RCP's pid if we don't already know it.
    ** Note that the get_rcp_pid function won't be filled in
    ** if this is a pseudoserver or if it's still early in server startup.
    */
    if (Cs_srv_block.cs_rcp_pid == 0 && Cs_srv_block.cs_get_rcp_pid)
	Cs_srv_block.cs_rcp_pid = (*Cs_srv_block.cs_get_rcp_pid)();

    if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
	return (E_CS0004_BAD_PARAMETER);

#ifdef SEM_LEAK_DEBUG
    CSMT_sem_checkbad( sem );
#endif

    if (type == CS_SEM_SINGLE || sem->cs_sem_scribble_check != CS_SEM_LOOKS_GOOD)
    {
	/* Trace apparent re-init of single-process sems, if it's for
	** real (and not just unlucky garbage in the scribble check) we
	** have a resource leak.  (And if not, we have sloppy code somewhere.)
	*/
	if (sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD)
	    TRdisplay("%@ Possible w-reinit of sem '%.#s' (%x) to %s\n",
		STnlength(sizeof(sem->cs_sem_name),sem->cs_sem_name),
		sem->cs_sem_name, sem, string);

	/* Clear everything out, including stats */
	MEfill( sizeof(*sem), 0, (char *)sem ); 

	/* Then do stuff that may be non-zero */
	sem->cs_type = type;
	if ( type == CS_SEM_SINGLE )
	{
	    if ( CS_synch_init( &sem->cs_mutex ) ||
		 CS_cond_init( &sem->cs_cond ) )
		return (E_CS0004_BAD_PARAMETER);
	}
	else
	{
	    STATUS status;
#     ifdef SIMULATE_PROCESS_SHARED
	    CS_cp_synch_init( &sem->cs_test_set, &status );
	    if ( status == OK )
		 CS_cp_cond_init( &sem->cs_cp_cond, &status );
#     else
	    CS_cp_synch_init( &sem->cs_mutex, &status );
	    if ( status == OK )
		 CS_cp_cond_init( &sem->cs_cond, &status );
#     endif
	    if ( status != OK )
		return (E_CS0004_BAD_PARAMETER);
	}
	sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

	STmove( string, EOS, sizeof( sem->cs_sem_name ), sem->cs_sem_name );

#     ifdef xCL_NEED_SEM_CLEANUP
    /* add to known lists */
    if ( type == CS_SEM_MULTI ) {
        int status;
        int i;
        char *key;
        void *addr;
        CS_SHM_IDS *new_shm = (void *)0;
        CS_SHM_IDS *found_shm = (void *)0;

        status = MEget_seg(sem, &key, &addr);
        if ( status == 0 )
        {
            QUr_init( &sem->cs_sem_list );
            CS_cp_synch_lock( &Cs_sm_cb->css_shm_mutex );
            for (i = 0; i < CS_SHMSEG; ++i)
            {
                if ( new_shm == (void *)0 && Cs_sm_cb->css_shm_ids[i].key[0] == 0 )
                    new_shm = &Cs_sm_cb->css_shm_ids[i];
                else if ( Cs_sm_cb->css_shm_ids[i].key[0] == 0 )
                    continue;
                else if ( STequal(key, Cs_sm_cb->css_shm_ids[i].key) ) {
                    found_shm = &Cs_sm_cb->css_shm_ids[i];
                    break;
                }
            }
            if ( found_shm == (void *)0 && new_shm )
            {
                STcopy(key, new_shm->key);
                found_shm = new_shm;
            }
            if ( found_shm )
            {
                QUr_insert( &sem->cs_sem_list, &found_shm->offset);
                found_shm->count++;
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Added CS_SEM_MULTI %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
            }
            else
            {
                TRdisplay("Exceeded shared memory semaphore structures\n");
            }
            CS_cp_synch_unlock( &Cs_sm_cb->css_shm_mutex );
        }
        else
        {
            QUinit( &sem->cs_sem_list );
            CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
            QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_multi_sem );
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Added CS_SEM_MULTI in LOCAL storage: %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
            CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
        }
    }
    else {
        QUinit( &sem->cs_sem_list );
        CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
        QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_single_sem );
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Added CS_SEM_SINGLE %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
        CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
    }
# endif /* xCL_NEED_SEM_CLEANUP */
    }

    return( OK );
}


/*{
** Name:	CSMTa_semaphore	\- attach to a CS_SEM_MULTI semaphore
**
** Description:
**	Allows the process to use a CS_SEM_MULTI semaphore that was not created
**	by the current process.  Silent no-op on CS_SEM_SINGLE semaphores.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		the sem to attach.
**
** Outputs:
**	sem		may be written.
**
** Returns:
**	OK		or reportable error.
**
** Side Effects:
**	May make the semaphore known to management facilities.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented, created for VMS.
**      17-May-1994 (daveb) 59127
**          Check validity of semaphore as best you can before
**  	    attaching.  Add ifdef-ed out leak-diagnostic call
*/
STATUS
CSMTa_semaphore( CS_SEMAPHORE *sem )
{
    STATUS clstat = FAIL;

#ifdef SEM_LEAK_DEBUG
    CSMT_sem_checkbad( sem );
#endif
    
    if( sem->cs_type == CS_SEM_MULTI && *sem->cs_sem_name != EOS &&
       sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD )
	clstat =  OK;

    return( clstat );
}

/*
** Name: CSMTr_semaphore - Remove a semaphore
**
** Description:
**	This routine is used by a running thread to return any resources from
**	a semaphore that is no longer needed. It should be called when a
**	CS_SEMPAHORE which has been intialized with CSi_semaphore will no
**	longer be needed, or will go out of context.
**
**	We don't care if detach fails, as long as we scribble it.
**
** Inputs:
**	sem	    A pointer to the semaphore to be invalidated.
**
** Outputs:
**	none
**
** Returns:
**	OK
**	other
**
** Side Effects:
**	Calls to CSp_semaphore and CSv_semaphore may become invalid until
**	CSi_semaphore is called again.
**
** History:
**	23-Oct-1992 (daveb)
**	    prototyped.  Do check on SEM_MULTI, 'cause it seems
**	    like a lot of 'em are tunring up removed, suspiciously.
**	3-Dec-1992 (daveb)
**	    Check and log scribble failues; I'm seeing a memory
**	    smash with the tell-tale string "DSEM" written where
**	    other data belongs!  Also allow us to turn off sem
**	    attach/detach completely, on the assumption it's going
**	    to be a long time tracking down badly removed ones.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**      17-May-1994 (daveb) 59127
**  	    Add ifdef-ed out leak-diagnostic call
**	09-Oct-1996 (jenjo02)
**	    CS_detach_sem() only CS_SEM_MULTI type semaphores.
**      08-jul-1997 (canor01)
**          Remove semaphores from known list.
**	02-Sep-1997 (jenjo02)
**	    Wrap code in sanity check before writing semaphore memory.
**	03-nov-1997 (canor01)
**	    Do not try to remove a semaphore that was never initialized.
**	08-Apr-1999 (bonro01)
**	    Moved QUr_init() UP to proper place because it was initializing
**	    LOCAL semaphore pointers instead of just MULTI semaphores.
*/
STATUS
CSMTr_semaphore( CS_SEMAPHORE *sem )
{
    if (sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD)
    {
	sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;
#ifdef SIMULATE_PROCESS_SHARED
	if (sem->cs_type == CS_SEM_MULTI)
	{
	   CS_cp_cond_destroy( &sem->cs_cp_cond );
	   CS_cp_synch_destroy( &sem->cs_test_set );
	}
	else
	{
#endif
	CS_cond_destroy( &sem->cs_cond );
	CS_synch_destroy( &sem->cs_mutex );
#ifdef SIMULATE_PROCESS_SHARED
	}
#endif

# ifdef xCL_NEED_SEM_CLEANUP
    /* remove from known list */
    if (sem->cs_type == CS_SEM_MULTI )
    {
        int status;
        int i;
        char *key;
        void *addr;
        CS_SHM_IDS *found_shm = (void *)0;

        CS_cp_synch_lock( &Cs_sm_cb->css_shm_mutex );
        status = MEget_seg(sem, &key, &addr);
        if ( status == 0 )
        {
            for (i = 0; i < CS_SHMSEG; ++i)
            {
                if ( Cs_sm_cb->css_shm_ids[i].key[0] == 0 )
                    continue;
                else if ( STequal(key, Cs_sm_cb->css_shm_ids[i].key) ) {
                    found_shm = &Cs_sm_cb->css_shm_ids[i];
                    break;
                }
            }
            if ( found_shm )
            {
                QUr_remove( &sem->cs_sem_list, &found_shm->offset );
                found_shm->count--;
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Deleted CS_SEM_MULTI %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
	    QUr_init( &sem->cs_sem_list );
            }
            else
            {
                CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
                QUremove( &sem->cs_sem_list );
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Deleted CS_SEM_MULTI from LOCAL storage: %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
                CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
                QUinit( &sem->cs_sem_list );
            }
        }
        CS_cp_synch_unlock( &Cs_sm_cb->css_shm_mutex );
    }
    else
    {
        CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
        QUremove( &sem->cs_sem_list );
# ifdef xCS_TRACE_SEMLIST
        TRdisplay("Deleted CS_SEM_SINGLE %s @%x, next %x prev %x\n",
	    sem->cs_sem_name, &sem->cs_sem_list, 
	    sem->cs_sem_list.q_next, sem->cs_sem_list.q_prev );
# endif /* xCS_TRACE_SEMLIST */
        CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
        QUinit( &sem->cs_sem_list );
    }
# endif /* xCL_NEED_SEM_CLEANUP */

	return( OK );
    }
    else
    {
	return(E_CS000A_NO_SEMAPHORE);
    }
}

/*{
** Name:	CSMTd_semaphore - detach semaphore.
**
** Description:
**	Detach from use of the specified CS_SEM_MULTI semphore, leaving
**	it for other processes to use.  Save no-op on CS_SEM_SINGLE sem.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		semaphore to detach.
**
** Outputs:
**	sem		will probably be written.
**
** Returns:
**	OK		or reportable error.
**
** History:
**	23-Oct-1992 (daveb)
**	    prototyped
*/
STATUS
CSMTd_semaphore( CS_SEMAPHORE *sem )
{
    return(OK);
}


/*{
** Name:	CSMTn_semaphore - give a name to a semaphore
**
** Description:
**	Give the semaphore a name, up to CS_SEM_NAME_LEN characters.
**	If the name is too long, it will be silently truncated to fit.
**	The name will be copied, so the input may vanish after the call
**	returns.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	sem		the semaphore to name.
**	name		the name to give it.
**
** Outputs:
**	sem		probably written.
**
** Returns:
**	none.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented, created for VMS.
**	15-Dec-1993 (daveb) 58424
**	    STlcopy for CSn_semaphore is wrong.  It'll write an EOS
**  	    into the cs_scribble_check that follows.
**      17-May-1994 (daveb) 59127
**          Attach as sem is named, if sem looks good and has
**  	    no name already.  Check to see it looks good.
**	09-Oct-1996 (jenjo02)
**	    CS_sem_attach() only to CS_SEM_MULTI type semaphores.
**	12-Sep-1996 (jenjo02)
**	    If semaphore already has a name, don't overwrite it.
*/
VOID
CSMTn_semaphore(
    CS_SEMAPHORE *sem,
    char *string)
{
    if( *sem->cs_sem_name == EOS )
    {
	STmove( string, EOS, sizeof( sem->cs_sem_name ), sem->cs_sem_name );
    }
}


/*{
** Name:	CSMTs_semaphore - return stats for a semaphore.
**
** Description:
**	Return semaphore statistics.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	options	        CS_INIT_SEM_STATS to initialize return area
**			CS_ROLL_SEM_STATS to accumulate in return area
**			CS_CLEAR_SEM_STATS to additionally clear the 
**			accumulators from the semaphore.
**	sem		the semaphore for which stats are wanted.
**	length		the size of the output area.
**
** Outputs:
**	stats		filled in with the appropriate stats
**			unless output area is too small or
**	 		semaphore doesn't look like a semaphore.
**
** Returns:
**	OK 		if stats were returned in output area
**	FAIL		if they were not
**
** History:
**	07-Dec-1995 (jenjo02)
**	    created
*/
STATUS
CSMTs_semaphore(
    i4		 options,
    CS_SEMAPHORE *sem,
    CS_SEM_STATS *stats,
    i4		 length )
{
    if (length >= sizeof(CS_SEM_STATS))
    {
	if (sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD)
	{
	    if (options & CS_ROLL_SEM_STATS && *stats->name != EOS)
	    {
		stats->excl_requests    += sem->cs_smstatistics.cs_smx_count;
		stats->share_requests   += sem->cs_smstatistics.cs_sms_count;
		stats->excl_collisions  += sem->cs_smstatistics.cs_smxx_count;
		stats->share_collisions += sem->cs_smstatistics.cs_smsx_count;
		if (sem->cs_smstatistics.cs_sms_hwm > stats->share_hwm)
		    stats->share_hwm 	 = sem->cs_smstatistics.cs_sms_hwm;
		stats->spins 		+= sem->cs_smstatistics.cs_smcl_count;
		stats->redispatches 	+= sem->cs_smstatistics.cs_smmp_count;
	    }
	    else
	    {
		STcopy( sem->cs_sem_name, stats->name);
		stats->type = sem->cs_type;
		stats->excl_requests    = sem->cs_smstatistics.cs_smx_count;
		stats->share_requests   = sem->cs_smstatistics.cs_sms_count;
		stats->excl_collisions  = sem->cs_smstatistics.cs_smxx_count;
		stats->share_collisions = sem->cs_smstatistics.cs_smsx_count;
		stats->share_hwm 	= sem->cs_smstatistics.cs_sms_hwm;
		stats->spins 		= sem->cs_smstatistics.cs_smcl_count;
		stats->redispatches 	= sem->cs_smstatistics.cs_smmp_count;
	    }
	    if (options & CS_CLEAR_SEM_STATS)
	    {
		MEfill(sizeof(sem->cs_smstatistics), 0, 
			(char *)&sem->cs_smstatistics);
	    }
	    return(OK);
	}
	else if ((options & CS_ROLL_SEM_STATS) == 0)
	{
	    MEfill(sizeof(*stats), 0, (char *)stats);
	}

    }
    return(FAIL);
}


/*{
** Name: CSMT_sem_owner_dead()    - determine if owner of semaphore is dead.
**
** Description:
**      Uses system specific method to determine if process associated with
**      current owner of semaphore still exists.  If process has exited then
**      TRUE is returned, and the routine`s caller (CSp_semaphore()) will
**      return an error indicating that the semaphore cannot be obtained.
**
** Inputs:
**      sem                             semaphore in question.
**
** Outputs:
**      none.
**
**      Returns:
**          TRUE        - the owner process of input semaphore is dead.
**          FALSE       - the owner process of input semaphore is alive.
**
** History:
**      31-may-90 (blaise)
**          Added from termcl (function written by mikem).
**      29-Jan-92 (gautam)
**          Pass the sem->cs_pid in instead and ensure we are'nt
**          trying to kill pid 0.
**	25-Nov-1997 (jenjo02)
**	    Spin for a while instead of putting the server to sleep.
**      19-Mar-2004 (hanal04) Bug 112160 INGSRV2792
**          The assignment of sem_pid is done without mutex protection.
**          Invalid values such as negative pids can therefore be assigned
**          to sem_pid. We must try to ensure we do not see the same partial
**          update or we will return a false positive.
*/
static bool
CSMT_sem_owner_dead(
    CS_SEMAPHORE    *sem,
    CS_SCB          *scb)
{
    i4	sem_pid = sem->cs_pid;
    i4		i;
    bool        ret_val = FALSE;
    STATUS	status;
    i4		cur_priority = scb->cs_priority;

    /* intra-process semaphore ? */
    if (sem_pid && sem_pid != Cs_srv_block.cs_pid &&
	kill(sem_pid, 0) == -1)
    {
        /* process does not exist ??*/

	/* Spin and yield to ensure we do not see the same pid value
	** unless the process really has died.
	*/
	for (i = 0; i < 5; i++)
	{
            CS_yield_thread();
            if(sem_pid != sem->cs_pid)
		break;
	    if(cur_priority > 1)
            {
		/* drop our priority, if the owner is alive we need it to
		** be scheduled again before we are to avoid a false positive
		*/
		cur_priority = cur_priority/2;
		CS_thread_setprio( scb->cs_thread_id, cur_priority, &status);
	    }
	}
	if(cur_priority != scb->cs_priority)
	    CS_thread_setprio( scb->cs_thread_id, scb->cs_priority, &status);

        /* take care of case where we collided with a process that existed,
        ** then gave up the semaphore, then exited.
        */
        if (sem_pid == sem->cs_pid)
            ret_val = TRUE;
    }
    return(ret_val);
}

#ifdef xCL_NEED_SEM_CLEANUP
/*
** Name: CSMT_cp_sem_cleanup()
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**  system.  This is necessary for operating systems that retain
**  a portion of the underlying synchronization object (mutex or
**  semaphore) in the kernel that is not automatically released
**  by releasing shared memory or exiting all processes using the
**  object.
**
** Inputs:
**      key                             shared memory key of area to cleanup
**
** Outputs:
**      none.
**
**      Returns:
**
** History:
**	26-Feb-1998 (bonro01)
**		Created.
**      18-aug-1998 (hweho01)
**         If the shared segment has been attached, just call 
**         ME_offset_to_addr() to get the shared memory address.  
*/
STATUS
CSMT_cp_sem_cleanup(
    char            *key,
    CL_ERR_DESC     *err_code)
{
    int             i, stat=0;
    PTR             shm_addr;
    SIZE_TYPE       pages_got;
    CL_ERR_DESC     sys_err;
    CS_SEMAPHORE    *sem;
    CS_SHM_IDS      *found_shm;
    bool            called_attach = FALSE;

    if( ( ME_offset_to_addr(key, 0, &shm_addr) ) == OK )  
      stat = OK;
    else
     {
      if( ( stat = MEget_pages(ME_SSHARED_MASK, 0, key, &shm_addr,
                              &pages_got, &sys_err )) == OK )
          called_attach = TRUE;
     }

    if (stat == OK)
    {
        found_shm = (void *)0;
        CS_cp_synch_lock(&Cs_sm_cb->css_shm_mutex);
        for (i = 0; i < CS_SHMSEG; ++i)
        {
            if ( Cs_sm_cb->css_shm_ids[i].key[0] == 0 )
                continue;
            else if ( STequal(key, Cs_sm_cb->css_shm_ids[i].key) ) {
                found_shm = &Cs_sm_cb->css_shm_ids[i];
                break;
            }
        }
        CS_cp_synch_unlock(&Cs_sm_cb->css_shm_mutex);

        if ( found_shm )
        {
            CS_SEMAPHORE *sem;
            while ( found_shm->offset.q_next != (void *) 0 )
            {
                sem = (CS_SEMAPHORE *)((char *)shm_addr + (long)(found_shm->offset.q_next));
                if( CSMTr_semaphore(sem) )
		{
# ifdef xCS_TRACE_SEMLIST
		    TRdisplay("ERROR: CSMTr_semaphore failed in CSMT_cp_sem_cleanup\n");
#endif /* xCS_TRACE_SEMLIST */
		    break;
		}
            }
            Cs_sm_cb->css_shm_ids[i].key[0] = 0;
        }
        if( called_attach )
          stat = MEdetach(key, shm_addr, &sys_err);
    }
    return(stat);
}
#endif

#ifdef SIMULATE_PROCESS_SHARED
/*{
** Name: CS_cp_synch_lock()    - For Threads systems that do not support PROCESS_SHARED mutex and 
**				 condition attributes. CS_cp_synch_lock will lock a simple mutex.
**
** Description:
**	Waits impatiently to secure a process shared mutex. Method of
**	waiting depends on number of processors. If hardware has only
**	a single processor, then yield to another thread. If hardware
**	has more than one processor, then spin while waiting for mutex
**	to become available, (the thread holding the mutex may be running
**	on another processor). On single or multi processor systems, 
**	this function waits until it obtains the mutex. If it never 
**	obtains the mutex it waits forever.
**	
**
** Inputs:
**      CS_ASET mutex 
**
** Outputs:
**      none.
**
**      Returns:
**          OK		- Obtained mutex
**
** History:
**      20-mar-97 (kosma01) 
**          created.
*/
STATUS
CS_cp_synch_lock (CS_ASET *mutex)
{
   if (!CS_ISSET(mutex) && CS_TAS(mutex))
      return ( OK );  /* Now we have the semaphore locked */
   else
   {  
        /* this isn't going to be easy */
      if (Cs_numprocessors <= 1)
      {
	/* SP environment */
	 for (;;)
	 {
	     CS_yield_thread();
	     if (CS_TAS(mutex))  return (OK) ;
	 }
      }
      else
      {
        /* MP environment */
        for (;;)
	{
	   i4  spin;
	   for (spin = Cs_max_sem_loops ; spin; spin--)
	   {
	      if (CS_TAS(mutex)) return ( OK );
           }
	   CS_yield_thread();   
	} /* for ever */
      } /* Cs_numprocessors */
    }  /* CS_ISSET && CS_TAS */
} /*  spin_lock */

/*{
** Name: CS_cp_cond_wait()     - For Threads systems that do not support
**				 PROCESS_SHARED mutexes and condition variables.
**				 CS_cp_cond_wait will wait for a sign that
**				 indicates it is time to check if the condition
**				 it is waiting for has been satisfied. 
**
** Description:
**      Waits impatiently for a sign that it is time to check if a 
**	condition has been satisfied. After obtaining a mutex for
**	some resource a thread calls this psuedo pthread function
**	to wait for the time when it might be useful to use the 
**	resource. While waiting, the mutex is released. When this
**	function returns the thread has the mutex and it is time
**	to recheck the conditions for using the resource.
**
**	This function simulates pthread_cond_wait for MULTI
**	(cross-process) semaphores on platforms that do not
**	support pthread PROCESS_SHARED mutex or condition
**	variable. The big difference between this function and
**	a real pthread_cond_wait is how the thread is "awakened"
**	when the condition variable is signalled. On a supported
**	platform, a thread issues a pthread_cond_signal for a 
**	condition variable and the OS wakes each thread waiting
**	for on that condition variable even if it is in a different
**	process. As each thread wakes it is given the mutex associated
**	with the condition variable and it checks to see if the
**	resource associated with the mutex is usable,(condition 
**	satisfied). 
**	
**	On platforms without support for PROCESS_SHARED mutexes and
**	condition variables, this function does the same but the
**	OS does not signal it when the condition variable is signalled.
**	Instead, after a condition variable has been set by a thread,
**	that thread's time slice runs out and the OS dispatches other
**	threads, eventually dispatching a thread that checks on the
**	condition variable and notices that it has be signalled.
**
** Inputs:
**      cond_variable,  mutex
**
** Outputs:
**      none.
**
**      Returns:
**          OK          - Time to check condition
**
** History:
**      20-mar-97 (kosma01)
**          created.
*/
STATUS
CS_cp_cond_wait( CS_CP_COND *cond, CS_ASET *mutex )
{
  for(;;)
  {
    if ( *cond )
    {
      *cond = 0;
      return ( OK );
    }
    CS_cp_synch_unlock( mutex );
    CS_yield_thread();
    CS_cp_synch_lock( mutex );
  }
}

/*{
** Name: CS_cp_cond_signal()   - For Threads systems that do not support
**                               PROCESS_SHARED mutexes and condition variables.
**                               CS_cp_cond_signal will signal, (flag), that 
**                               now is the time to check if the conditions for 
**				 using a mutex gaurded resource have been satisfied.
**
** Description:
**	see CS_cp_cond_wait() above
**
** Inputs:
**      condition variable
**
** Outputs:
**      none.
**
**      Returns:
**          OK          - Condition has been signalled
**
** History:
**      20-mar-97 (kosma01)
**          created.
*/

STATUS
CS_cp_cond_signal( CS_CP_COND *cond )
{
  *cond = 1;
  return ( OK );
}

/*{
** Name: CS_cp_synch_yield()	For Threads systems that do not support
**				PROCESS_SHARED mutexes and condition variables.
**				Yield the mutex used for cross-process
**				synchronization and then wait to re-acquire it.
**
** Description:
**	This routine is called when our held mutex is preempted.  We'd like to
**	do a timed wait (in case the mutex owner has died), but since we don't
**	have OS threading support for that, yield our mutex and ourselves the
**	fates and hope for the best.
**
** Inputs:
**      mutex
**
** Outputs:
**      none.
**
**      Returns:
**          OK          - We again hold the desired mutex
**
** History:
**	10-jul-01 (toumi01)
**          created.
*/
STATUS
CS_cp_synch_yield( CS_ASET *mutex )
{
    CS_cp_synch_unlock( mutex );
    CS_yield_thread();
    CS_cp_synch_lock( mutex );
    return( OK );
}

# endif /* SIMULATE_PROCESS_SHARED */
# endif /* OS_THREADS_USED */
