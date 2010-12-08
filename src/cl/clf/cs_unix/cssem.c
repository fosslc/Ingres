/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
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
# include <qu.h>

# include <csinternal.h>
# include <iinap.h>

# ifdef xCL_NEED_SEM_CLEANUP
# include <csev.h>
# include <cssminfo.h>
# endif

# include "csmgmt.h"
# include "cslocal.h"

/**
**
**  Name: CSSEM.C - module containing CS semaphore routines.
**
**  Description:
**	Contains all routines which deal with CS semaphore operations.  
**	Broken out of csinterface.c to give compiler optimizers a chance
**	at this file (csinterface.c is nooptim on some platforms because of 
**	some stack manipulations done be some other routines).
**
**	external functions:
**
**	    CSp_semaphore() - Perform a "P" operation, claiming the semaphore
**	    CSv_semaphore() - Perform a "V" operation, releasing the semaphore
**	    CSi_semaphore() - Initialize Semaphore.
**	    CSa_semaphore() - attach to a CS_SEM_MULTI semaphore
**	    CSr_semaphore() - Remove a semaphore
**	    CSd_semaphore() - detach semaphore.
**	    CSn_semaphore() - give a name to a semaphore
**	    CSw_semaphore() - Initialize and Name a semaphore
**			      (CSi_ + CSn_) in one operation.
**	    CSs_semaphore() - Return semaphore statistics.
**
**	static functions:
**
**	    CS_sem_owner_dead() - determine if process holding semaphore is dead
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
**	02-Jul-1996 (jenjo02)
**	    Removed #ifdef VMS code. VMS has its own CL.
**	    Also removed references to Cs_srv_block.cs_inkernel
**	    and scb->cs_inkernel which are artifacts from the
**	    days when this code was shared with VMS.
**      09-jan-1997 (canor01)
**          Do not register semaphores with MO; delete calls to
**          CS_sem_attach().
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**	    check Cs_srv_block.cs_mt
**	08-jul-1997 (canor01)
**	    Maintain semaphores on known semaphore list.
**	07-aug-1997 (canor01)
**	    Temporarily disable the known semaphore list.
**	19-aug-1997 (canor01)
**	    Make known semaphore list dependent on xCL_NEED_SEM_CLEANUP.
**  21-Aug-1997 (devjo01)
**	    Modify CSp_semaphore_function to detect deadlocks caused
**	    by CPU startvation. (*5631925).
**	23-sep-1997 (kosma01)
**	    For SIMULATE_PROCESS_SHARED platforms, different functions
**	    clean up MULTI semaphores and SINGLE semaphores.
**  12-Dec-97 (bonro01)
**      Added cleanup code for cross-process semaphores during
**      MEsmdestroy().  This code is currently only required for DGUX
**      so it is enabled by xCL_NEED_SEM_CLEANUP.
**  23-Mar-1999 (hanal04)
**          In CSp_semaphore_function() take local copies of unprotected
**          data values before using those values in division. b95990.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	13-oct-1999 (somsa01)
**	    In CSp_semaphore_function(), moved first breakout check for
**	    Cs_incomp to the point where we are blocked, as that is the
**	    point where we should die if we're in completion. It was
**	    causing the "P" operation in sc0m_deallocate() to fail,
**	    leading to a memory leak.
**  03-Jul-1999 (devjo01)
**	    Modify CSp_semaphore_function to temporarily reduce session
**	    priority for long term spinners on a cross process mutex.
**	    Ingres cannot detect session scheduling/mutex deadlocks
**          that span multiple servers.  This hueristic assumes such a
**          deadlock exists if we reach the point where the old logic 
**          would sleep the process. (b95607/INGSRV0736).
**	15-Nov-1999 (jenjo02)
**	    Removed cs_sem_init_pid, cs_sem_init_addr, both of which were
**	    of dubious value and just wasting space.
**  08-Dec-1999 (hanal04) Bug 95607 INGSRV 736
**          Slight modification to the above change. During a mutex
**          deadlock the DBMS logs grow continually with 'reducing priority'
**          messages until the disk is full. Also report the address of the
**          semaphore that the session is spinning on.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Renamed all external CS?_semaphore functions
**	    to IICS?_semaphore, deleted all cs_mt tests.
**  11-Jan-2002 (hanal04) Bug 106804 INGSRV 1655.
**          Improve the time in which session priority mutex deadlock 
**          is avoided in CSp_semaphore().
**	21-Jan-2007 (kiria01) b117581
**	    Use global cs_state_names instead of explicit strings
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added include of qu.h with QUr_* prototypes.
**/


/*
**  Forward and/or External typedef/struct references.
*/

static bool CS_sem_owner_dead(
	    int             sem_pid,
	    CS_SEMAPHORE    *sem);

GLOBALREF i4		      Cs_incomp;
GLOBALREF i4             Cs_numprocessors;
GLOBALREF i4             Cs_max_sem_loops;
GLOBALREF i4             Cs_desched_usec_sleep;


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

# define                        CS_MAX_SEM_LOOPS    100000

/*
** Max number of 'reducing priority....' messages to log.
*/
 
# define                        MAX_LOG_DROP    100


# ifdef SEM_LEAK_DEBUG

/* called when a sem is seen in the badlist */
void
CS_sem_badlist( CS_SEMAPHORE *sem )
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

CS_sem_checkbad( CS_SEMAPHORE *sem )
{
    i4  i;

    for( i = 0; i < BADSEMS ; i++ )
	if( sem == badlist[ i ] )
	    CS_sem_badlist( sem );
}

# endif


/*{
** Name: CSp_semaphore()	- perform a "P" operation on a semaphore
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
**  16-oct-1996 (merja01)
**      Moved ifdef VMS to col 1 so it would compile on OSF.
**  21-Aug-1997 (devjo01)
**	    Modify CSp_semaphore_function to better detect deadlocks caused
**	    by CPU startvation. (*5631925).
**  23-Mar-1999 (hanal04)
**      Cross process semaphore statistics are updated when the semaphore is
**      blocked and are not therefore protected from cross process updaters.
**      Take local copies when trying to prevent divide by zero errors. b95990
**	13-oct-1999 (somsa01)
**	    Moved first breakout check for Cs_incomp to the point where we
**	    are blocked, as that is the point where we should die if we're
**	    in completion. It was causing the "P" operation in
**	    sc0m_deallocate() to fail, leading to a memory leak.
**  03-Jul-1999 (devjo01)
**          Modify CSp_semaphore_function to temporarily reduce session
**          priority for long term spinners on a cross process mutex.
**          Ingres cannot detect session scheduling/mutex deadlocks
**          that span multiple servers.  This hueristic assumes such a
**          deadlock exists if we reach the point where the old logic 
**          would sleep the process. (b95607/INGSRV0736).
**  08-Dec-1999 (hanal04) Bug 95607 INGSRV 736
**          Slight modification to the above change. During a mutex
**          deadlock the DBMS logs grow continually with 'reducing priority'
**          messages until the disk is full. Also report the address of the
**          semaphore that the session is spinning on.
**  27-Mar-2001 (devjo01)
**	    Remove VMS conditionals around 'inkernel' sets, since
**	    this may be needed when CSresume is called in a completion
**	    handler.
**  11-Jan-2002 (hanal04) Bug 106804 INGSRV 1655.
**          Further adjustment to the mutex priority deadlock avoidance
**          code. To reduce the contention window the session priority
**          in now halved instead of decremented.
*/
STATUS
IICSp_semaphore(i4 exclusive, register CS_SEMAPHORE *sem)
{
    register	CS_SCB	*scb = Cs_srv_block.cs_current;
    register	CS_SCB	*next_scb;
    CS_SEMAPHORE        *bsem;
    i4			save_priority;
    i4			base_priority;
    i4             drop_count = 0;
    CS_SEM_STATS        sstat;

#ifdef SEM_LEAK_DEBUG
    CS_sem_checkbad( sem );
#endif

    if ((sem->cs_type == CS_SEM_SINGLE) &&
        (Cs_srv_block.cs_state != CS_INITIALIZING) &&
	(sem->cs_owner != scb))
    {
	for (;;)
	{
		 
	    if (sem->cs_value == 0)
	    {
	        if (exclusive && (sem->cs_count == 0)) 
	        {
		    sem->cs_value = 1;
	    	    sem->cs_owner = scb;
		    scb->cs_sem_count++;

		    Cs_srv_block.cs_smstatistics.cs_smx_count++;
		    sem->cs_smstatistics.cs_smx_count++;
		    return(OK);
	        }
	        else if (!exclusive && !sem->cs_excl_waiters)
		/* grant share only if there are no exclusive waiters */
	        {
		    sem->cs_count++;
		    scb->cs_sem_count++;

		    Cs_srv_block.cs_smstatistics.cs_sms_count++;
		    sem->cs_smstatistics.cs_sms_count++;

		    /* Now, go move everyone back to ready */
		    while ((next_scb = sem->cs_next) != NULL)
		    {
			if (next_scb->cs_state == CS_MUTEX)
			{
			    next_scb->cs_state = CS_COMPUTABLE;
		            Cs_srv_block.cs_ready_mask |=
			        (CS_PRIORITY_BIT >> next_scb->cs_priority);
			}
#ifdef	xDEBUG
		        else
			    TRdisplay("CS_MUTEX p BUG (tell CLF/CS owner): scb: %p, cs_state: %w%<(%x)\n",
				    next_scb,
				    cs_state_names,
				    next_scb->cs_state);
#endif
		        sem->cs_next = next_scb->cs_sm_next;
			next_scb->cs_sm_next = 0;
		    }

		    return(OK);

	        }
	        else /* cannot have right now.  Wait */
		{
		    sem->cs_excl_waiters = 1; /* at least one excl waiter */
		    Cs_srv_block.cs_smstatistics.cs_smsx_count++;
		    sem->cs_smstatistics.cs_smsx_count++;
		}
	    }
	    else /* if (sem->cs_value == 0) */
	    {
		Cs_srv_block.cs_smstatistics.cs_smxx_count++;
		sem->cs_smstatistics.cs_smxx_count++;
	    }

	    /*
	    ** If we're at this point, then we better not be in completion.
	    ** Otherwise, this is not good!
	    */
	    if (Cs_incomp)
	    {
		CS_breakpoint();
		return (FAIL);
	    }

	    scb->cs_state = CS_MUTEX;
	    scb->cs_sm_next = sem->cs_next;
	    scb->cs_sync_obj = (PTR) sem;
	    sem->cs_next = (CS_SCB *)scb->cs_self;


	    CS_swuser();

	    if (scb->cs_mask & CS_ABORTED_MASK)
	    {
	 	Cs_srv_block.cs_ready_mask
			|= (CS_PRIORITY_BIT >> scb->cs_priority);
		return(E_CS000F_REQUEST_ABORTED);
	    }
	} /* for (;;) */

    }
    else if (sem->cs_type == CS_SEM_MULTI)
    {
	/*
	** Cross process semaphore.
	** Loop doing TEST/SET instructions until the lock can be granted.
	** If semaphore held by same process, suspend this thread, but leave
	** its state as computable so that when the holder releases the lock,
	** it does not have to set this thread's state.
	**
	** Set the inkernel flag so that we do not switch threads via the
	** quantum timer while in the middle of setting the semaphore pid
	** and owner.
	*/
        /* b95990 */
	register i4 real_loops, loops, yield_loops, avg_spins,
                         smxx_count, smsx_count;
	real_loops = loops = yield_loops = 0;
	avg_spins = 0;

	Cs_srv_block.cs_inkernel = 1;

	if (scb)
	    base_priority = scb->cs_priority;

	for (;;)
	{
	    /*
	    ** Test the spin lock before trying the Test-And-Set operation.
	    ** This prevents us from tying up the bus with interlock
	    ** instructions while waiting for the spin lock to become free.
	    ** Instead, we only try the TAS operation if the spin lock is free.
	    */
	    if ( ! CS_ISSET(&sem->cs_test_set) &&
		     CS_TAS(&sem->cs_test_set))
	    {
		for (;;)
		{
		    if (exclusive && !sem->cs_count)
		    {
			/*
			** Grant exclusive if no sharers of sem.
			**
		        ** Mark the semphore as owned by this thread.
		        ** The cs_pid and cs_owner values uniquely
		        ** identify this thread.
		        */
		        sem->cs_pid = Cs_srv_block.cs_pid;
		        sem->cs_owner = scb;
			sem->cs_smstatistics.cs_smx_count++;
		    } 
		    else if (!exclusive && !sem->cs_excl_waiters)
		    {
			/*
			** Grant shared access iff there are no
			** exclusive requests pending, otherwise
			** spin until exclusive gets a chance.
			**
			** Bump count of sharers, release
			** latch to allow other sharers a
			** chance.
			*/
			sem->cs_count++;
			sem->cs_smstatistics.cs_sms_count++;
			CS_ACLR(&sem->cs_test_set);
		    }
		    else
		    {
			/*
			** Semaphore is blocked, either because
			** exclusive wanted but there are sharers
			** or shared is wanted but there's an
			** exclusive trying to get in.
			**
			** Free the latch, then spin, spin, spin...
			*/
			sem->cs_excl_waiters = 1;
			CS_ACLR(&sem->cs_test_set);
			break;
		    }

		    /*
		    ** Semaphore granted.
		    */
		    Cs_srv_block.cs_smstatistics.cs_smc_count++;

		    if (scb)
		    {
	    	        scb->cs_sem_count++;
			if ( scb->cs_mask & CS_DIZZY_MASK )
			{
			    /* Long term spinner, restore original priority */
		    	    scb->cs_inkernel = 1;
			    scb->cs_mask &= ~(CS_DIZZY_MASK|CS_MUTEX_MASK);
		    	    CS_change_priority(scb, base_priority);
		            scb->cs_inkernel = 0;
			    TRdisplay("%@ Multiple server mutex contention deadlock avoided");
			}
		    }

		    Cs_srv_block.cs_inkernel = 0;
		    if (Cs_srv_block.cs_async && scb)
	    	        CS_move_async(scb);

		    return(OK);
		}
	    }

	    /*
	    ** Semaphore is blocked.
	    **
	    ** Spin/redispatch til it becomes free.
	    */
	    Cs_srv_block.cs_smstatistics.cs_smcl_count++;
	    sem->cs_smstatistics.cs_smcl_count++;

	    if (loops++ == 0)
	    {
		/*
		** Compute spin quantum as average number of historic
		** mp spins required before semaphore is granted,
		** or as Cs_max_sem_loops if there's yet no
		** history for this semaphore.
		*/
                /* b95990 - Take snap shot to prevent divide by zero */
                smxx_count = sem->cs_smstatistics.cs_smxx_count;
                smsx_count = sem->cs_smstatistics.cs_smsx_count;
                if(smxx_count + smsx_count)
                {
                   avg_spins = sem->cs_smstatistics.cs_smcl_count /
                             (smxx_count + smsx_count);
                }
                else
                {
                   avg_spins = Cs_max_sem_loops;
                }

		Cs_srv_block.cs_smstatistics.cs_smcx_count++;

		if (exclusive)
		    sem->cs_smstatistics.cs_smxx_count++;
		else 
		    sem->cs_smstatistics.cs_smsx_count++;
	    }

	    /*
	    ** Redispatch only if we have an SCB to begin with, 
	    ** we're not a "pseudo-server" (no other threads to
	    ** run), or if we're "in completion". In those cases,
	    ** just spin til we get the lock.
	    */
	    if ( (scb) &&   
		!(Cs_srv_block.cs_state == CS_INITIALIZING) &&
		!(Cs_incomp) )
	    {
		if (sem->cs_pid == Cs_srv_block.cs_pid)
		{
		    /*
		    ** Semaphore owned by another thread in the same process.
		    **
		    ** Immediately redispatch another thread.
		    */
		    if (sem->cs_owner == scb ||
			Cs_incomp )
		    {
			/* BUG - it should never be the case that while we are
			** "in completion" that another thread in the same process
			** holds the same semaphore which we are requesting.  This
			** will lead to an infinite wait, so we will return
			** deadlock.
			*/
			Cs_srv_block.cs_inkernel = 0;
			if (Cs_srv_block.cs_async)
			     CS_move_async(scb);

			/*
			** No need to restore base priority here,
			** this case is immediate death, scb 
			** could never have gotten "dizzy".
			*/
			return(E_CS0017_SMPR_DEADLOCK);
		    }
		}
		else 
		{
		    /*
		    ** Semaphore owned by another process.
		    **
		    ** On an SP machine, immediately redispatch another thread
		    ** so that we can continue do get some work done. 
		    ** Hopefully a newly dispatched thread will do something
		    ** to cause the holding process to get a slice of the cpu
		    ** and free the semaphore.
		    ** On an MP machine, loop Cs_max_sem_loops(avg_spins) before 
		    ** dispatching another thread; most requests on an MP
		    ** machine should be satisfied before Cs_max_sem_loops 
		    ** (avg_spins) have been executed.
		    */

		    /* (previous spin algorithm)
		    ** On a SP machine call II_nap() immediately to deschedule
		    ** the current process, and hopefully schedule the process
		    ** which owns the semaphore.  On a MP machine loop
		    ** Cs_max_sem_loops before descheduling the process; most
		    ** requests on MP machine should be satisfied before
		    ** Cs_max_sem_loops have been executed.
		    **
		    ** Some have suggested that rather than nap one should
		    ** schedule another thread in the server.  Attempts to
		    ** call CSsuspend() at this point cause timing bugs.
		    ** We can't CSsuspend; a CSresume may be pending or about to
		    ** occur. (B37501)
		    **
		    ** It is also questionable whether one should keep running
		    ** when another process has been descheduled while holding
		    ** a critical resource, it is probably better for the the
		    ** throughput on the entire system to give up the run-slot
		    ** to the sleeping process which was descheduled holding
		    ** the semaphore.
		    */
		    if (Cs_numprocessors <= 1)
		    {
			/* SP machine */

			Cs_srv_block.cs_smstatistics.cs_smsp_count++;
			sem->cs_smstatistics.cs_smsp_count++;

		    }
		    else if (loops > avg_spins)
		    {
			/* MP machine */

			Cs_srv_block.cs_smstatistics.cs_smmp_count++;
			sem->cs_smstatistics.cs_smmp_count++;

			/* Reset loop count to give this thread another "spin"
			** quantum to compete with other processes for semaphore.
			*/
			loops = 0;
		    }
		    else
			continue;
		}

		/*  
		** Search chain of blocking SCB's until we get to
		** one that is either computable, or not in our
		** server (different PID).   If SCB is in our
		** server, and is set to a lower priority, then
		** lower our priority temporarily.  This ultimately
		** will give the blocking SCB a shot at the CPU, and
		** will prevent a CPU starvation deadlock.  This is
		** a generalization of the former algorithim which
		** just looked at the owner of 'sem', and therefore
		** was unable to detect more complicated cycles.
		**
		** For now, we do not attempt to traverse past SCBs
		** not in a CS_MUTEX state.   This should be okay,
		** because the other normal states are either not
		** entered into while holding a semaphore, or will
		** convert to a supported state.
		*/
		save_priority = scb->cs_priority;

		bsem = sem;
		while ((bsem->cs_pid == Cs_srv_block.cs_pid ||
			bsem->cs_type == CS_SEM_SINGLE) &&
		       bsem->cs_owner != NULL)
		{
		    if (bsem->cs_owner->cs_state == CS_COMPUTABLE)
		    {
		        if (scb->cs_priority > bsem->cs_owner->cs_priority)
			{
		    	    scb->cs_inkernel = 1;
		    	    CS_change_priority(scb, bsem->cs_owner->cs_priority);
		            scb->cs_inkernel = 0;
#ifdef xDEBUG
			    if ( bsem != sem )
			    {
				TRdisplay("%@ Single server mutex contention deadlock avoided");
			    }
#endif
			}
		        break;
		    }
		    if (bsem->cs_owner->cs_state != CS_MUTEX)
			break;
		    bsem = (CS_SEMAPHORE*)(bsem->cs_owner->cs_sync_obj);
		}

		/*
		** Allow next runnable thread to continue - when this thread
		** is selected again to run it will retry the semaphore.
		**
		** Set this thread's cs_mask to CS_MUTEX_MASK, 
		** leave its state as COMPUTABLE, and point the
		** scb to the semaphore we're trying to snatch.
		** This to provide some useful info about why
		** this thread is computable but not apparently
		** doing anything.
		*/
		scb->cs_inkernel = 1;
		scb->cs_mask |= CS_MUTEX_MASK;
		scb->cs_sync_obj = (PTR) sem;
		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
		CS_swuser();

		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;
		scb->cs_mask &= ~CS_MUTEX_MASK;
		scb->cs_inkernel = 0;

		if (scb->cs_mask & CS_ABORTED_MASK)
		{
		    if ( scb->cs_priority != base_priority )
		    {
		        /* Restore original priority */
			Cs_srv_block.cs_inkernel = 1;
		        scb->cs_inkernel = 1;
		        scb->cs_mask &= ~(CS_DIZZY_MASK|CS_MUTEX_MASK);
		        CS_change_priority(scb, base_priority);
		        scb->cs_inkernel = 0;
		    }

		    Cs_srv_block.cs_ready_mask
				|= (CS_PRIORITY_BIT >> scb->cs_priority);
		    Cs_srv_block.cs_inkernel = 0;
		    if (Cs_srv_block.cs_async)
			 CS_move_async(scb);
		    return(E_CS000F_REQUEST_ABORTED);
		}
		 


		/*
		** If we had to move down the threads priority, restore it now.
		*/
		if (scb->cs_priority != save_priority)
		{
		    scb->cs_inkernel = 1;
		    CS_change_priority(scb, save_priority);
		    scb->cs_inkernel = 0;
		}
	    }

	    /*
	    ** Once in a great while (every 500 spins),
	    ** see if the owning process has died. We could
	    ** check more often, but the knowledge of its demise
	    ** will probably lead to our own untimely passing,
	    ** so we'll try to get as much work done as we can
	    ** in the meantime.
	    */
	    if (sem->cs_pid != Cs_srv_block.cs_pid &&
		yield_loops++ > 500)
	    {
		yield_loops = 0;
		if (CS_sem_owner_dead(sem->cs_pid, sem))
		{
		    if ( scb && scb->cs_priority != base_priority )
		    {
		        /* Restore original priority */
		        scb->cs_inkernel = 1;
		        scb->cs_mask &= ~(CS_DIZZY_MASK|CS_MUTEX_MASK);
		        CS_change_priority(scb, base_priority);
		        scb->cs_inkernel = 0;
		    }
		    Cs_srv_block.cs_inkernel = 0;
		    if (Cs_srv_block.cs_async && scb)
			CS_move_async(scb);
		    return(E_CS0029_SEM_OWNER_DEAD);
		 }
	    }

	    /*
	    ** If the semaphore is held for much longer than is normally
	    ** expected, then assume a cross process mutex/scheduling
	    ** deadlock, and take remedial measures.
	    */

	    if (real_loops++ > CS_MAX_SEM_LOOPS)
	    {
		if ((Cs_srv_block.cs_state == CS_INITIALIZING) ||
		    (Cs_incomp || !scb))
		{
		    /* We have been called from a completion routine (most
		    ** likely from LG/LK called by the LGK event handler called
		    ** by CS_find_events().  In this case we cannot reshedule
		    ** as it would lead to recursive rescheduling.  We must
		    ** just spin until the resource is released.  It is
		    ** guaranteed that the resource is not held by another
		    ** thread within the same process (ie. holders of the
		    ** LGK shared memory semaphores never volunarily switch
		    ** while holding the semaphore.
		    **
		    ** If the state is CS_INITIALIZING then this is being called
		    ** from a non-dbms server (ie. the rcp, acp, createdb, ...).
		    ** We never want to call CSsuspend() in this case, so
		    ** we will sleep awhile so as not to chew up all cycles on
                    ** the machine.
		    */
                    if (Cs_srv_block.cs_state == CS_INITIALIZING)
		    {
			sem->cs_smstatistics.cs_smnonserver_count++;
			Cs_srv_block.cs_smstatistics.cs_smnonserver_count++;
                        II_nap(1000000);
		    }

		    continue;
		}

		/*
		** New logic: to fix b95607.
		**
		** Assume if we're spinning this long that there exists
		** a multi-process mutex/scheduling deadlock.  To break
		** this we'll keep on reducing our session priority in
		** the expectation that this will allow a lower priority
		** session in our server to release the resource blocking
		** the session living in another process which is holding
		** the mutex, we're trying to get.  If we reach the bitter
		** end with this still unresolved my inclination would be
		** to fail the semaphore request, but for now, we'll
		** just bounce between the old and new strategies, giving
		** each a chance.
		**
		** Old logic: (sleeping won't help us, since session that
		**  needs to release the critical resource lives in this
		**  server also.)
		**
		** > We can't CSsuspend; a CSresume may be pending or about to
		** > occur. If there are other runnable threads, we'd like to
		** > let them continue to run, but CSswuser() calls have looped
		** > a LONG time at this point (CS_MAX_SEM_LOOPS already), so
		** > we'll instead force the entire server to sleep for 1 second
		** > and then try again. (B37501)
		*/

		if ( scb->cs_priority > (CS_PIDLE+1) )
                {
                    if(drop_count < MAX_LOG_DROP)
                    {
                        if(drop_count == 0)
                        {
                            if((CSs_semaphore(CS_INIT_SEM_STATS, sem,  &sstat,
                                        sizeof(sstat))) != OK)
                                STcopy("mutex", sstat.name);
                        }
                        TRdisplay("%@ Reducing session %p priority to %d, waiting for %s\n",
                                scb, scb->cs_priority/2, sstat.name);
                        drop_count++;
                    }

	    	    scb->cs_inkernel = 1;
		    scb->cs_mask |= (CS_MUTEX_MASK|CS_DIZZY_MASK);

		    /* No risk of setting priority to 0 as scb->cs_priority
		    ** must be greater than CS_PIDLE+1, i.e. 2 or more.
		    */
		    CS_change_priority(scb, scb->cs_priority/2);
	    	    scb->cs_inkernel = 0;
		}
		else
		{
	    	    scb->cs_inkernel = 1;
		    CS_change_priority(scb, base_priority);
		    scb->cs_inkernel = 0;
		    Cs_srv_block.cs_inkernel = 0;
		    if (Cs_srv_block.cs_async)
		        CS_move_async(scb);
		    II_nap(1000000); 
		    Cs_srv_block.cs_inkernel = 1;
		}
		loops = 0;
		real_loops -= avg_spins;
	    }
	    else
	        continue;
	} /* for (;;) */ 
    }
    else
    {
	if (sem->cs_owner != (CS_SCB *)1 && sem->cs_owner != scb)
	{
	    sem->cs_owner = (CS_SCB *)1;
	    return(OK);
	}
	else
	    return(E_CS0017_SMPR_DEADLOCK);
    }
}

/*{
** Name: CSv_semaphore	- Perform a "V" operation, releasing the semaphore
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
*/
STATUS
IICSv_semaphore( register CS_SEMAPHORE *sem )
{
    register	CS_SCB	*scb = Cs_srv_block.cs_current;
    register	CS_SCB	*next_scb;

#ifdef SEM_LEAK_DEBUG
    CS_sem_checkbad( sem );
#endif

    if ((sem->cs_type == CS_SEM_SINGLE) &&
	(Cs_srv_block.cs_state != CS_INITIALIZING)) 
    {
	/*
	** Single process semaphore
	*/
	if (scb->cs_sem_count-- == 0)
	{
	    scb->cs_sem_count = 0;
	    return(E_CS000A_NO_SEMAPHORE);
	}

	if (sem->cs_value)
	{
	    /*
	    ** if held exclusively, then just release 
	    **
	    ** by definition, cs_value == 1
	    **                cs_owner == scb
	    **                cs_count == 0
	    */
	    sem->cs_value = 0;
	    sem->cs_owner = 0;
	}
	else
	    /*
	    ** shared semaphore
	    **	
	    ** by definition, cs_value == 0
	    **                cs_owner == 0
	    **                cs_count >  0
	    */
	{
	    if (sem->cs_count-- == 0)
	    {
		sem->cs_count = 0;
		scb->cs_sem_count++;
		return(E_CS000A_NO_SEMAPHORE);
	    }
	}
	if (sem->cs_count == 0)
	{ /* move waiters back to ready */
	    sem->cs_excl_waiters = 0; /* all get an equal opportunity */
	    while ((next_scb = sem->cs_next) != NULL)
	    {
		if (next_scb->cs_state == CS_MUTEX)
		{
		    next_scb->cs_state = CS_COMPUTABLE;
		    Cs_srv_block.cs_ready_mask |=
			(CS_PRIORITY_BIT >> next_scb->cs_priority);
		}
#ifdef	xDEBUG
		else
		    TRdisplay("CS_MUTEX v BUG (tell CLF/CS owner): scb: %p, cs_state: %w%<(%x)\n",
			    next_scb,
			    cs_state_names,
			    next_scb->cs_state);
#endif
		sem->cs_next = next_scb->cs_sm_next;
		next_scb->cs_sm_next = 0;
	    }
	}

	return(OK);
    }
    else if (sem->cs_type == CS_SEM_MULTI)
    {
	/*
	** Cross process semaphore.
	** 
	** Any waiters for this lock will be spinning trying to acquire it.
	** The first to request it after we reset the value will be granted.
	**
	** Note that waiters cannot be suspended in MUTEX state in order to
	** wait for this semaphore as we are not able to wake them up when we
	** release the semaphore since they may be in a different process.
	*/
	if (CS_ISSET(&sem->cs_test_set) &&
	    sem->cs_count == 0)
	{
	    /*
	    ** Held exclusively. Make sure this thread holds the
	    ** semaphore.
	    */
	    if (sem->cs_owner == scb &&
		sem->cs_pid == Cs_srv_block.cs_pid)
	    {
		/*
		** Reset cs_owner/cs_pid
		*/
		sem->cs_owner = NULL;
		sem->cs_pid = 0;
	    }
	    else 
	    {
		return(E_CS000A_NO_SEMAPHORE);
	    }
	}
	else 
	{
	    if (scb && scb->cs_sem_count == 0)
	    {
		return(E_CS000A_NO_SEMAPHORE);
	    }
	    /*
	    ** Spin until we get the lock. Another process may be
	    ** working on this semaphore.
	    */
	    while (!(CS_TAS(&sem->cs_test_set)));
	    
	    /*
	    ** Posit held shared, decrement count of sharers.
	    ** If no longer shared, allow new share requests.
	    */
	    if (--sem->cs_count == 0)
		sem->cs_excl_waiters = 0;
	    else if (sem->cs_count < 0)
	    {
		sem->cs_count = 0;
		CS_ACLR(&sem->cs_test_set);
		return(E_CS000A_NO_SEMAPHORE);
	    }
	}

	/*
	** Release the semaphore value.
	*/
	CS_ACLR(&sem->cs_test_set);

	if (scb)
	{
	    scb->cs_sem_count--;
	}
	return(OK);
	
    }
    else if (sem->cs_owner == (CS_SCB *)1)
    {
	sem->cs_owner = 0;
	sem->cs_next = 0;
	return(OK);
    }
    else 
    {
	/*
	**  Something's wrong, e.g., thread does not own semaphore.
	*/
	return(E_CS000A_NO_SEMAPHORE);
    }
	
}

/*{
** Name: CSi_semaphore	- Initialize Semaphore.
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
**	08-jul-1997 (canor01)
**	    Add semaphores to known list.
*/
STATUS
IICSi_semaphore(CS_SEMAPHORE *sem, i4  type)
{
    if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
	return (E_CS0004_BAD_PARAMETER);

#ifdef SEM_LEAK_DEBUG
    CS_sem_checkbad( sem );
#endif

    /* Clear everything out, including stats */
    MEfill( sizeof(*sem), 0, (char *)sem ); 

    /* Then do stuff that may be non-zero */
    sem->cs_type = type;
    CS_ACLR(&sem->cs_test_set);
    sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

# ifdef xCL_NEED_SEM_CLEANUP
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
            }
            else
            {
                TRdisplay("Exceeded shared memory semaphore structures\n");
            }
            CS_cp_synch_unlock( &Cs_sm_cb->css_shm_mutex );
        }
        else
        {
            TRdisplay("Initialized MULTI semaphore in local process storage\n");
            QUinit( &sem->cs_sem_list );
            CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
            QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_multi_sem );
            CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
        }
    }
    else {
        QUinit( &sem->cs_sem_list );
        CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
        QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_single_sem );
        CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
    }
# endif /* xCL_NEED_SEM_CLEANUP */

    return( OK );
}


/*{
** Name: CSw_semaphore	- Initialize and Name Semaphore
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
**	08-jul-1997 (canor01)
**	    Add semaphores to known list.
*/
STATUS
IICSw_semaphore(CS_SEMAPHORE *sem, i4  type, char *string)
{
    if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
	return (E_CS0004_BAD_PARAMETER);

#ifdef SEM_LEAK_DEBUG
    CS_sem_checkbad( sem );
#endif

    /* Clear everything out, including stats */
    MEfill( sizeof(*sem), 0, (char *)sem ); 

    /* Then do stuff that may be non-zero */
    sem->cs_type = type;
    CS_ACLR(&sem->cs_test_set);
    sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

# ifdef xCL_NEED_SEM_CLEANUP
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
            }
            else
            {
                TRdisplay("Exceeded shared memory semaphore structures\n");
            }
            CS_cp_synch_unlock( &Cs_sm_cb->css_shm_mutex );
        }
        else
        {
            TRdisplay("Initialized MULTI semaphore in local process storage\n");
            QUinit( &sem->cs_sem_list );
            CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
            QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_multi_sem );
            CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
        }
    }
    else {
        QUinit( &sem->cs_sem_list );
        CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
        QUinsert( &sem->cs_sem_list, &Cs_srv_block.cs_single_sem );
        CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
    }
# endif /* xCL_NEED_SEM_CLEANUP */

    STmove( string, EOS, sizeof( sem->cs_sem_name ), sem->cs_sem_name );
    
    return( OK );
}


/*{
** Name:	CSa_semaphore	\- attach to a CS_SEM_MULTI semaphore
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
IICSa_semaphore( CS_SEMAPHORE *sem )
{
    STATUS clstat = FAIL;

#ifdef SEM_LEAK_DEBUG
    CS_sem_checkbad( sem );
#endif
    
    if( sem->cs_type == CS_SEM_MULTI && *sem->cs_sem_name != EOS &&
       sem->cs_sem_scribble_check == CS_SEM_LOOKS_GOOD )
	clstat = OK;

    return( clstat );
}

/*
** Name: CSr_semaphore - Remove a semaphore
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
**	08-jul-1997 (canor01)
**	    Remove semaphores from known list.
*/
STATUS
IICSr_semaphore( CS_SEMAPHORE *sem )
{

#ifdef SEM_LEAK_DEBUG
    CS_sem_checkbad( sem );
#endif

    sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;

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
            }
            else
            {
                QUremove( &sem->cs_sem_list );
                QUinit( &sem->cs_sem_list );
            }
        }
        CS_cp_synch_unlock( &Cs_sm_cb->css_shm_mutex );
        QUr_init( &sem->cs_sem_list );
    }
    else
    {
        QUremove( &sem->cs_sem_list );
        QUinit( &sem->cs_sem_list );
    }
# endif /* xCL_NEED_SEM_CLEANUP */

    return( OK );

}

/*{
** Name:	CSd_semaphore - detach semaphore.
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
IICSd_semaphore( CS_SEMAPHORE *sem )
{
    return( OK );
}


/*{
** Name:	CSn_semaphore - give a name to a semaphore
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
*/
VOID
IICSn_semaphore(
    CS_SEMAPHORE *sem,
    char *string)
{
    STmove( string, EOS, sizeof( sem->cs_sem_name ), sem->cs_sem_name );
}


/*{
** Name:	CSs_semaphore - return stats for a semaphore.
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
IICSs_semaphore(
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
** Name: CS_sem_owner_dead()    - determine if owner of semaphore is dead.
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
*/
static bool
CS_sem_owner_dead(
    int             sem_pid,
    CS_SEMAPHORE    *sem)
{
    bool        ret_val = FALSE;

    /* intra-process semaphore ? */
    if (sem_pid && kill(sem_pid, 0) == -1)
    {
        /* process does not exist ??*/

        II_nap(5000000);

        /* take care of case where we collided with a process that existed,
        ** then gave up the semaphore, then exited.
        */
        if (sem_pid == sem->cs_pid)
	{
            ret_val = TRUE;
	}
    }
    return(ret_val);
}

/*
** Name: CS_cp_sem_cleanup()
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**	system.  This is necessary for operating systems that retain
**	a portion of the underlying synchronization object (mutex or
**	semaphore) in the kernel that is not automatically released
**	by releasing shared memory or exiting all processes using the
**	object.
**  This routine differs from CSremove_all_sems() in that this is called
**  during shared memory deletion, while CSremove_all_sems() is called
**  during dmfrcp shutdown.
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
**  23-Feb-1998 (bonro01)
**      Created.
**	10-Nov-2010 (kschendel) SIR 124685
**	    Make void, nothing uses the return value.
*/
void
CS_cp_sem_cleanup(
    char            *key,
    CL_ERR_DESC     *err_code)
{

    CL_CLEAR_ERR(err_code);

# ifdef OS_THREADS_USED
#ifdef xCL_NEED_SEM_CLEANUP
    if ( (Cs_srv_block.cs_mt) )
	CSMT_cp_sem_cleanup( key, err_code );
# endif /* xCL_NEED_SEM_CLEANUP */
# endif /* OS_THREADS_USED */

}

/*{
** Name: CSremove_all_sems - delete known semaphores at shutdown
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**	system.  This is necessary for operating systems that retain
**	a portion of the underlying synchronization object (mutex or
**	semaphore) in the kernel that is not automatically released
**	by releasing shared memory or exiting all processes using the
**	object.
**  This routine is now used to cleanup only those semaphores that are
**  not defined within shared memory.
**
** Inputs:
**      type		CS_SEM_MULTI - to release cross-process sems
**			CS_SEM_SINGLE - to release private sems
**
** Outputs:
**      none.
**
** Returns:
**      OK		- operation succeeded
**      other system-specific return
**
** History:
**	08-jul-1997 (canor01)
**	    Created.
**  23-Feb-1998 (bonro01)
**      Modified semaphore cleanup so that semaphores within shared
**      memory are cleaned up during shared memory destory while
**      the rest of dmfrcp semaphores are cleaned up using this
**      routine.
*/
STATUS
CSremove_all_sems( i4  type )
{
# ifdef xCL_NEED_SEM_CLEANUP
    CS_SEMAPHORE *sem;
    QUEUE        *semlist;

    if ( type == CS_SEM_MULTI )
	semlist = &Cs_srv_block.cs_multi_sem;
    else
	semlist = &Cs_srv_block.cs_single_sem;

# ifdef OS_THREADS_USED
    if ( Cs_srv_block.cs_mt )
	CS_synch_lock( &Cs_srv_block.cs_semlist_mutex );
# endif /* OS_THREADS_USED */

    for ( sem = (CS_SEMAPHORE *) semlist->q_next;
	  sem != (CS_SEMAPHORE *) semlist;
	  sem = (CS_SEMAPHORE *) sem->cs_sem_list.q_next )
    {
        sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;

# ifdef OS_THREADS_USED
# ifdef SIMULATE_PROCESS_SHARED

        if ( type == CS_SEM_MULTI )
        {
	  CS_cp_synch_destroy( &sem->cs_test_set );
          CS_cp_cond_destroy( &sem->cs_cp_cond );
        }
        else
        {
	  CS_synch_destroy( &sem->cs_mutex );
          CS_cond_destroy( &sem->cs_cond );
        }
# else
	CS_synch_destroy( &sem->cs_mutex );
        CS_cond_destroy( &sem->cs_cond );
# endif /* SIMULATE_PROCESS_SHARED */
# endif /* OS_THREADS_USED */
    }

# ifdef OS_THREADS_USED
    if ( Cs_srv_block.cs_mt )
	CS_synch_unlock( &Cs_srv_block.cs_semlist_mutex );
# endif /* OS_THREADS_USED */

# endif /* xCL_NEED_SEM_CLEANUP */
    return( OK );

}

