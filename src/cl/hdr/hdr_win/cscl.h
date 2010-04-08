/******************************************************************************
** Copyright (c) 1987, 2004 Ingres Corporation
**
** Name: CS.H - Control System CL Module Global Header
**
** Description:
**      This file contains define's and datatype definitions
**      for the Ingres Compatibilty Library/Control System
**      Module.  This module contains those routines which
**      are used to control the activity in a multithreaded
**      server environment.  The structures defined here are
**      originally being designed for use in a multithreaded
**      DBMS Server;  however, an attempt is being made to define
**      the system such that this code could be used to control
**      other types of multithreaded servers, specifically thinking
**      in terms of uses for terminal servers in Online Transaction
**      Processing (OLTP) systems.
**
** Specification:
**
**  Description:
**
**	This CLF module provides a Control System (CS) for controlling
**	multiple threads of activity within a session.  The CS module allows
**	multiple threads of execution to share the machine cycles in a
**	priority-based manner under control of the INGRES system.
**
**	The CS module can be easily implemented to support the degenerate
**	case of a single thread of execution in systems which encourage
**	such a model.
**
**  Intended Uses:
**
**	The initial purpose of the CS module is to provide support for
**	multithreading of the DBMS.  However, the CS module has been
**	designed so the interface to the module will work equally well
**	with other Ingres applications.  Thus, it would be possible to use
**	the CS module to build a multiterminal server for the Ingres frontends.
**
**    Assumptions:
**
**	It is assumed that the notion of a multithreaded server is
**	understood.  The CS module provides the capability to support
**	such an object.
**
**	It is assumed that the notion of a semaphore is understood.
**	(Reference: "Operating Systems," Madnick, Stuart E., & Donovan,
**	John J., McGraw-Hill Book Co., 1974.)
**
**	When CS is providing full services, the CS module assumes that
**	the system provides the following capabilities.  Implementation
**	platforms may choose to implement less than a full service
**	CS module.
**
**(	    Timer Based Interrupts:
**		The CS module makes use of the system's timers to
**		provide time-sliced execution of threads within a
**		priority.  CS does not require the use of time-sliced
**		execution.  Implementation platforms may choose not to
**		provide this capability and have all threads "voluntarily"
**		give up the CPU periodically.
**
**	    Asynchronous Event Completion:
**		The CS module provides interface points to allow
**		a thread to suspend itself while awaiting the completion
**		of a system event.  Once again, implementation platforms
**		may choose to use polling to determine the completion
**		events in environments where asynchrony is less convenient.
**
**	    Uninterrupted Processing Mode:
**		The CS module must occasionally run in a mode during
**		which it is not informed of asynchronous event completion.
**		These periods of time are short, but required nonetheless.
**)
**	When the CS module is configured to run in the degenerate case described
**	above, these assumptions can be relaxed.
**
**	It is assumed that the remainder of the Compatibility Library and,
**	indeed, of INGRES, is modified to use the features provided by the
**	CS module.  Alone, the CS module cannot provide full multi-threaded
**	server capability.  It expects that the other CL modules which interact
**	with the system cooperate with it.  This means, primarily, that the
**	other CL modules make the appropriate suspend & resume calls on
**	behalf of the current thread as necessary.  For example, the DI module
**	of the CL is expected to be built such that I/O requests inform the
**	system that the resume (CSresume()) routine is to be called at the
**	completion of the I/O, and then the DI module, having requested an
**	asynchronous I/O, should call the suspend routine (CSsuspend()).
**	Thus, DIread() may be coded (details removed for the sake of clarity)
**	as
**(	    DIread(...)
**	    {
**		preprocessing...
**		status_of_queuing =
**			system_io_request(parms..., CSresume, session_id);
**		if (status_of_queuing != successful)
**		{
**		    process error and return;
**		}
**		status = CSsuspend(...);
**		postprocessing...
**	    }
**)
**    Definitions/Concepts:
**
**	(Note:  In this context, it is important to remind the reader that,
**	independent of terms used elsewhere in RTI documentation, the terms
**	defined in the "CL Specification Guidelines" are in use here.
**	Thus, "session" refers to the System's (as defined in the aforementioned
**	document) concept of the invoking process, and that "thread"
**	is intended to refer to a line of activity within the session.
**	Thus, in some sense, "session" is to "system" as "thread" is
**	to "session.")
**
**	The CS module supports the multithreading of a number of similar
**	threads.  The threads all must run the same application code.
**	Applications are specified to the CS module as a set of functions
**	which are called by the CS module to accomplish the work of the
**	application.  These functions are specified as pointers to critical
**	entry points in the main application.  These entry points are
**(	    server initiation,
**	    communication routines (await thread request, reject thread request,
**				    disconnect threads, read thread, write
**				    thread, and out-of-band completion routine)
**	    thread processing,
**	    server termination,
**	    error logging,	and
**	    memory allocation.
**	
**	Each of these is called in a prescribed manner to perform their
**	various functions.
**
**	At the time the server is invoked, the specified server initiation
**	routine is called as
**	    status = (*server_initiation)(&CS_INFO_CB);
**	This routine is expected to perform whatever initiation is necessary
**	for the server being run.
**
**	As new threads are added (via the await_thread_request routine),
**	the thread processing routine is called
**	    status = (*thread_processing)(mode_of_operation,	-- see below
**					    &thread_control_block,
**					    &next_mode_of_operation);
**	where mode_of_operation & next_mode_of_operation are as specified
**	below.  Any status != OK results in thread termination.
**
**	When reading or writing are necessary,
**	    status = (*read_routine)(&thread_control_block, sync_boolean)
**			or
**	    status = (*write_routine)(&thread_control_block, sync_boolean)
**		(All communication routines are called using this
**		    paradigm.)
**	are called.  Status's here of OK result in normal processing;
**	if read returns E_CS001E_SYNC_COMPLETION, then it is assumed
**	that the read request already completed, and that no thread
**	suspension need take place.  Similarly, if write returns
**	E_CS001D_NO_MORE_WRITES, then no suspension for the write needs
**	to take place.  Note that the system is optimized on the assumption
**	that the server will do more write's than reads, so the normal
**	control flow is
**
**	    if (need to read)
**	    {
**		call read routine
**		if status != sync completion
**		    suspend
**	    }
**	    call processing routine
**	    if (need to write)
**	    {
**		do
**		{
**		    call write routine
**		    if status != no more writes
**			suspend
**		} while status == OK
**	    }
**
**	When time comes for the server to terminate,
**	    (*termination_routine)()
**)	is called to allow the system to run itself down as necessary.
**
**	Error logging is used to place errors encountered by the CS module
**	into the server error log.  Memory allocation is used to allocate and
**	deallocate the thread control blocks which are used by the CS module
**	and by the underlying application.  By changing these critical entry
**	points, the underlying application controlled by CS can be changed.
**
**	Threads are called with a mode of operation, which specifies the
**	activity which the thread is expected to perform, and returns with
**	a mode of operation which specifies what activity the thread expects
**	to perform next.  The possible modes of operations are:
**
**(	    CS_INITIATE -- Given to the processing routine to indicate that
**		a new thread is to be initiated.  Never returned to the
**		CS module.
**	    CS_INPUT -- Given to processing routine to indicate that input has
**		been supplied from the user.  Returned to the CS module to
**		indicate that more input is needed.
**	    CS_OUTPUT -- Given to the processing routine to indicate that
**		it is expected to be providing more output (i.e. it last
**		returned CS_OUTPUT).  Returned to the CS module to indicate
**		that the CS module should output the supplied buffer and
**		call the processing routine back.
**	    CS_EXCHANGE -- Never given to a processing routine.
**		Returned to the CS module to indicate that the module should
**		output the supplied buffer, and await an input message from
**		the user.
**	    CS_TERMINATE -- Given to the processing routine to indicate that
**		it should terminate the current thread.  Returned to the
**		CS module to indicate that the thread is to be terminated,
**		and that the CS module should call back the processing routine
**		with the CS_TERMINATE operation code.
**)
**	At thread initiation time, the thread is called to allow it to perform
**	whatever application specific actions are necessary.  After that,
**	the thread sets	its mode of operation.  Thus, normally, a DBMS thread
**	will return an operation mode of CS_INPUT, to indicate that the user
**	needs to supply a query.  Once so supplied, the DBMS will return
**	CS_OUTPUT until the query has been completed, at which point it
**	will return CS_EXCHANGE, to indicate that the output should be sent
**	and that the CS module should then await new input.  Once the user
**	sends a termination request, the DBMS would return an operation
**	mode of CS_TERMINATE, with which it will be called back.  Returned
**	modes from a called CS_TERMINATE are ignored.
**
**	The CS module provides the capability for the threads to interact
**	in the sharing of resources.  This is provided by having the
**	threads declare and use CS_SEMAPHORE's, which, when combined with the
**	'p' and 'v' operations described below, allow many threads to
**	cooperate in the sharing and utilization of software resources.
**
**	The CS module provides for the prioritized execution of threads
**	within the session.  As the session becomes ready to execute a
**	new thread, CS will pick the highest priority thread to execute.
**	Within a priority, the threads are picked in a "round robin"
**	fashion.  The scheduler is not currently intended to be preemptive;
**	that is, should a higher priority thread become ready, the lower
**	priority thread will be allowed to either complete its current
**	time quantum (see below) or be suspended when it so volunteers.
**
**	The CS module provides for timesliced execution of the threads within
**	a session.  The time quantum can be selected by the system administrator
**	at a particular user site.  A provision can be made to disable this
**	feature, so that threads will be suspended only while awaiting a system
**	event completion.  (On VMS, this is done via the server startup
**	program.)
**
**	Threads volunteer to be suspended while awaiting the completion of some
**	system event.  They may also be suspended while awaiting a semaphore.
**	Each thread is allowed to be awaiting no more than one regular
**	event or semaphore at a time.  A provision is made to allow the thread
**	to be informed of an out-of-band I/O (for interrupts).
**
**	The CS module provides the following data structures.
**	The CS_SEMAPHORE is used to control access to shared data structures
**	within the session under software control.
**	The CS_SCB is the thread control block.  This structure is returned
**	to the CS module by the thread control block allocation routine passed
**	into the module.  It is expected that the actual thread control block
**	will be larger than that passed here;  however, the CS module will use
**	only the CS_SCB portion, and expects that it be at the returned address
**	of the allocated space.
**
**	Threads are added to the session as requested by the users.  The method
**	of attaching to the server will vary from system to system.  On VAX/VMS
**	systems, users will send a request to public mailbox to request such
**	an action.
**
**	As a thread is added to the session, the CS module will add the new
**	thread to its list of known threads.  The underlying application will
**	be called to initiate this thread, and operation will proceed as
**	described above.  The difference between the multithreaded nature
**	of, say, a VAX/VMS system and the single threaded nature of
**	another system should be entirely hidden within the CS module.
**	There is no reason for the underlying application to behave any
**	differently in these differing systems.
**
**	The CS module declares no global variables which are known outside
**	itself.  Internally, it may require varying support on each system.
**
**  Possible Futures:
**
**	Since the underlying application is fully specified at server startup,
**	the CS module needs to know nothing about how the underlying application
**	works.  However, at the moment, the method of communication from the
**	user to/from the server is known by the CS module (it currently uses
**	IN/NT to transmit information between the user and the thread).
**	In the future, should it be the case that the company decides to build
**	servers from something other than DBMS's, the CS module should be easily
**	converted to the fully generalized model. (This has been done with GCF).
**
**	It may, at some point, be desirable for the CS module to allow each
**	thread to run different code.  This, in environments with which I am
**	familiar should not be too difficult.
**
**	History:
**	25-jul-95 (emmag)	
**	    Add define for CS_READ_AHEAD
**	19-aug-95 (emmag)
**	    Add defines for all thread types for the performance improvement
**	    project.
**	31-aug-1995 (shero03)
**	    Add pid and cpu in SCB
**	12-sep-1995 (shero03)
**	    Added sync_obj in SCB
**	20-sep-1995 (shero03)
**	    Changed the username field to be MAXNAME size
**	04-oct-1995 (wonst02)
**	    Add cs_facility to CS_CB for the address of scs_facility().
**	 6-Jun-95 (jenjo02)
**	    Changed obsolete cs_cnt_sem, cs_long_cnt, cs_ms_long_et to
**	    cs_pad, added cs_excl_waiters in CS_SEMAPHORE.
**      07-Dec-1995 (jenjo02)
**          Added new cs_mode, CS_MUTEX_MASK, which is set when a
**          session is computable but spinning trying to acquire
**          a cross-process semaphore.
**
**          Added new structure, CS_SEM_STATS, into which a semaphore's
**          statistics will be returned in response to a (new)
**          CSs_semaphore() function call.
**
**          Defined a new macro, CS_SEMAPHORE_IS_LOCKED, which offers
**          a dirty look at a semaphore's state, returning TRUE
**          if the semaphore is apparently latched.
**
**          Removed define of CS_GWFIO_MASK 0x1000, which wasn't being
**          used and changed CS_LOG_MASK 0x0000 to 0x1000. CSsuspend()
**          calls which don't specify a "wait type" mask have always
**          defaulted to "CS_LOG_MASK", skewing monitor and statistics
**          by incorrectly showing threads waiting on the LOG when
**          they're really just waiting on a timer. Real waits on the
**          log will now be specified by the explicit use of
**          CS_LOG_MASK.
**	08-feb-1996 (canor01)
**	    Added cs_multi_lst structure to CS_SEMAPHORE for Windows
**	    NT.  This structure holds pid/handle pairs to maintain
**	    unique handles for each process accessing a cross-process
**	    semaphore.
**	24-oct-1996 (canor01)
**	    Modified for compatibility with new Unix native-threaded CS.
**	12-dec-1996 (canor01)
**	    Added macro for CS_synch_destroy
**	24-feb-1997 (cohmi01)
**	    Add error #define. Bug 80050, 80242
**	24-feb-1997 (canor01)
**	    Added CS_thread_id_equal and CS_thread_id_assign macros for
**	    compatibility with Unix native-threaded CS.
**	24-mar-1997 (canor01)
**	    Add CS_NANO_TO_MASK for CS_SCB.cs_mask to allow for
**	    nanosecond timeouts.
**      02-may-1997 (canor01)
**          Add cs_cs_mask to CS_SCB for session flags specific to CS.
**	06-jun-1997 (canor01)
**	    Change definition of CS_SID to match Microsoft's.
**	22-jul-1997 (somsa01)
**	    Copied over changes made by radve01 (change #431055) to csnormal.h .
**	04-aug-97 (mcgem01)
**	    Back out a previous submission, due to a problem with the
**	    maintenance of the known semaphore list.
**	10-sep-97 (mcgem01)
**	    Add a define for CS_THREAD_ID.
**	03-nov-1997 (canor01)
**	    Remove event member of semaphore structure to cut down on total
**	    number of system resouces consumed.
**	07-nov-1997 (canor01)
**	    Add E_CS0047_SHUT_STATE.
**	03-Dec-1997 (jenjo02)
**	    Added cs_get_rcp_pid to CS_CB.
**	30-dec-1997 (canor01)
**	    Change CS_synch_xxx macros to return proper values on NT.
**	08-jan-1998 (somsa01)
**	    Added the defines necessary to mimick "setuid" from UNIX.
**	    (Bug #87751)
**	12-feb-1998 (canor01)
**	    Add cs_di_event to CS_SCB.  All dio overlapped structures can
**	    re-use the same event for the thread instead of creating and
**	    closing new events on each I/O.
**      16-Feb-98 (fanra01)
**          Add attach and detach fuctions to structure so that this can be
**          done when the thread is active.
**	19-feb-1998 (somsa01)
**	    We also need to keep track of the working directory in the
**	    SETUID structure.  (Bug #89006)
**	25-feb-1998 (somsa01)
**	    Removed the ClientInPipe handle from the SETUID structure.
**      09-Mar-1998 (jenjo02)
**          Moved CS_MAXPRIORITY, CS_MINPRIORITY, CS_DEFPRIORITY here
**          from csinternal.h
**	30-jun-1998 (hanch04)
**	    For mckba01 added cs_diag_qry.
**	    As part of the VMS port of ICL diags, Added new field in
**	    CS_SCB on VMS, have to replicate here as genric code
**	    updates this field.
**	28-jul-1998 (canor01)
**	    Remove extraneous "typedef".  Add CS_synch_trylock() macro.
**	31-jul-1998 (canor01)
**	    Add definition for CS_SUSPENDED.
**	11-aug-98 (mcgem01)
**	    Semaphore names can be greater than 32 characters - add a
**	    16 byte buffer to allow for facility name prefixes.
**	30-aug-98 (mcgem01)
**	    Add cs_stkcache to CS_CB.
**      17-sep-1998 (canor01)
**          Add CS_AINCR datatype, an integer datatype that can be atomically
**          incremented and decremented.
**	07-nov-1998 (canor01)
**	    Add ASYNC_TIMEOUT structure for trapping asynchronous BIO events
**	    that time out.
**	16-Nov-1998 (jenjo02)
**	  o Added CS_CPID structure, used by LG/LK, CScp_resume().
**	  o Added defines for new CSsuspend masks.
**	22-feb-1999 (somsa01)
**	    Added cs_random_seed to CS_SCB.
**	14-jun-1999 (somsa01)
**	    Added define for CS_CTHREAD_EVENT.
**	20-jan-1999 (canor01)
**	    Force use of ANSI versions of CreateMutex and CreateEvent.
**	10-Mar-1999 (shero03)
**	    Added cs_random_seed.
**	14-jun-1999 (somsa01)
**	    Added define for CS_CTHREAD_EVENT.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	05-nov-1999 (somsa01)
**	    Added cs_ditgiop for GatherWrite.
**	08-nov-1999 (somsa01)
**	    In CS_INFO_CB, allow up to GL_MAX_LISTEN server listen addresses
**	    for multi-capable servers.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of waits. Also, for NT, changed CS_CPID to
**	    also hold the thread's id.
**	28-dec-1999 (somsa01)
**	    Removed code for multi-capable servers which was accidentally
**	    added as part of the GatherWrite code. It wil be re-added
**	    later when the multi-capable server code is complete.
**	10-jan-2000 (somsa01)
**	    Added cs_dior, cs_diow, cs_bior, cs_biow, cs_lior, and cs_liow.
**	22-jan-2000 (somsa01)
**	    Added SERVICE_CONTROL_STOP_NOINGSTOP define.
**	24-jul-2000 (somsa01 for jenjo02)
**	    In CS_SEMAPHORE:
**	    Remove obsolete cs_list.
**	    Remove dubious cs_sem_init_pid, cs_sem_init_addr.
**	    Replace CS_SID cs_sid with CS_THREAD_ID cs_thread_id (OS_THREADS).
**	    Shrunk cs_value, cs_type from i4 to i2.
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	12-feb-2001 (somsa01)
**	    Thread ID's are now SCALARP instead of DWORD. This is for
**	    compatibility with NT-IA64 port. Also, sync'ed up members
**	    of CS_SCB with that of UNIX version.
**	20-mar-2002 (somsa01)
**	    Added ExitCode variable to the SETUID structure.
**	17-mar-2003 (somsa01)
**	    Removed unneeded CS_CONDITION field 'cnd_mutex_sem'.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	22-jan-2004 (somsa01)
**	    Updated definition of CS_CONDITION. Added cnd_mtx pointer to
**	    external semaphore. This is needed by CSattn, CSremove when
**	    aborting a session in CS_CNDWAIT state. Removed all of the
**	    other OS_THREADS_USED junk to simplify and eliminate persistence
**	    race conditions. Also remove conditions from MO.
**	27-jan-2004 (penga03)
**	    Added definiation of CS_ISSET, CS_ACLR, CS_TAS, CScas4 and 
**	    CScasptr. And added cs_format_lkkey field in CS_CB.
**	14-may-2004 (penga03)
**	    Added CS_KILL_EVENT define.
**	11-Jan-2005 (gorvi01)
**		Updated inclusion of CScas4 for Windows. (i64_win and a64_win port)
**	2-Mar-2005 (drivi01) on behalf of (lakvi01)
**	    Changed type from i4 to SIZE_TYPE for cs_length.
**      11-mar-2005 (horda03) Bug 105321 INGSRV 1502
**          On NT CSnoresnow() not implemented.
**	08-Feb-2007 (kiria01) b117581
**          Change the way we define the masks and states for cs_mask and
**          cs_state. These are now defines in a table macro that gets used
**          to define enum constants for the values and can be used for
**          defining the display strings that represent them.
**	09-Apr-2007 (drivi01) b117581
**	    Fix change #485729, replace "GLOBALREF const" with GLOBALCONSTREF and
**	    "GLOBALDEF const" with GLOBALCONSTDEF for cs_state_names and 
**	    cs_mask_names to prevent compile problems of c++ files. 
**	16-Feb-2009 (fanra01)
**      SIR 122443
**	    Switch cs_mutex type from Win32 mutex to CRITICAL_SECTION
**      28-Jan-2010 (horda03) Bug 121811
**          As cs_diag() for Evidence Set port.
**
******************************************************************************/

#ifndef		CS_INCLUDED
#define		CS_INCLUDED	1
/******************************************************************************
**
**  Nested includes
**
******************************************************************************/

#include <sp.h>

/******************************************************************************
**
**  Forward and/or External typedef/struct references.
**
******************************************************************************/

typedef struct _CS_SCB CS_SCB;
typedef struct _CS_SEMAPHORE	CS_SEMAPHORE;
typedef struct _CS_CONDITION	CS_CONDITION;
typedef struct _CS_SEM_STATS    CS_SEM_STATS;
typedef struct _CS_CPID  CS_CPID;

/******************************************************************************
**
**  Defines of other constants.
**
******************************************************************************/

#define CS_MAXNAME      32

#define CS_SID	SCALARP    /* thread id */

#define CS_SEM_NAME_LEN	CS_MAXNAME + 16

/*
** Ingres thread priorities
**
** Constants that define MIN and MAX priorities of sessions, and the
** default priority used if priority 0 is given when starting a thread.
**      MIN_PRIORITY is 1 - priority 0 cannot be specified since passing
**          a zero value to CSadd_thread indicates to use the default priority.
**          The Idle Job is the only thread that will run at priority zero.
**      MAX_PRIORITY is two less than CS_LIM_PRIORITY to ensure that no
**          sessions run at higher priority than the admin job (which runs
**          at CS_LIM_PRIORITY - 1).
**      CUR_PRIORITY means the priority of the current thread
*/
#define                 CS_LIM_PRIORITY 16
#define                 CS_MINPRIORITY  1
#define                 CS_MAXPRIORITY  (CS_LIM_PRIORITY - 2)
#define                 CS_DEFPRIORITY  (CS_LIM_PRIORITY / 2)
#define                 CS_CURPRIORITY  -1

/* Compatibility with Unix */
# define CS_ASET        DWORD
# define CS_AINCR       LONG
# define CS_ISSET(a)    (*(a) != 0)
# define CS_ACLR(a)     (*(a) = 0)
# define CS_TAS(a)      (InterlockedExchange(a, 1) ? 0 : 1)

# define CS_atomic_increment InterlockedIncrement
# define CS_atomic_decrement InterlockedDecrement
# define CS_atomic_init(aincr) (*aincr = 0)

#if !defined(NT_IA64) && !defined(NT_AMD64)
# define CScasptr( p, o, n )  ((o) != InterlockedCompareExchange( (LONG volatile *)p, (LONG)n, (LONG)o ))
# else
# define CScasptr( p, o, n )  ((o) != InterlockedCompareExchange64( (LONGLONG volatile *)p, (LONGLONG)n, (LONGLONG)o ))
#endif  /* !NT_IA64 && !NT_AMD64 */
# define CScas4( p, o, n )  ((o) != InterlockedCompareExchange( (LONG volatile *)p, (LONG)n, (LONG)o ))

# define CS_SYNCH	HANDLE

# define CS_synch_init(synchptr)	{\
					SECURITY_ATTRIBUTES sa;\
					iimksec(&sa);\
					*synchptr = \
					    CreateMutexA(&sa,FALSE,NULL);\
					}
# define CS_synch_lock(synchptr)     WaitForSingleObject(*synchptr,INFINITE)
# define CS_synch_trylock(synchptr)  WaitForSingleObject(*synchptr,0)
# define CS_synch_unlock(synchptr)   (ReleaseMutex(*synchptr)?OK:GetLastError())
# define CS_synch_destroy(synchptr)  (CloseHandle(*synchptr)?OK:GetLastError())

/* thread creation flags */
# define CS_DETACHED    0    /* releases resources on exit */
# define CS_JOINABLE    1    /* must be waited for with CS_join_thread() */
# define CS_SUSPENDED   2    /* thread immediately suspends itself */

# define CS_get_thread_id	     GetCurrentThreadId

# define CS_thread_id_equal(a,b)        (a == b)
# define CS_thread_id_assign(a,b)       a = b
# define CS_THREAD_ID			DWORD

# define CS_COND		   HANDLE
# define CS_cond_init(condptr)     {\
				   SECURITY_ATTRIBUTES sa;\
				   iimksec(&sa);\
				   *condptr = \
				      CreateEventA(&sa,TRUE,FALSE,NULL);\
				   }
# define CS_cond_destroy(condptr)  (CloseHandle(*condptr)?OK:GetLastError())



/******************************************************************************
**
**      The following constants are used to allow the called
**      thread to determine how it is being called, and to instruct
**      the dispatcher how it needs to be called next.
**
******************************************************************************/

#define CS_INITIATE     0x1
#define CS_INPUT        0x2
#define CS_OUTPUT       0x3
#define CS_TERMINATE    0x4	
#define CS_EXCHANGE		0x5
#define	CS_READ			0x10

/******************************************************************************
**
**  The following are the definitions of errors which can be returned
**  from the Control System call.  Some are accompanied by an associated
**  host operating system error.
**
******************************************************************************/

#define	E_CS0001_INVALID_STATE		(E_CL_MASK + E_CS_MASK + 0x0001)
#define	E_CS0002_SCB_ALLOC_FAIL		(E_CL_MASK + E_CS_MASK + 0x0002)
#define	E_CS0003_NOT_QUIESCENT		(E_CL_MASK + E_CS_MASK + 0x0003)
#define	E_CS0004_BAD_PARAMETER		(E_CL_MASK + E_CS_MASK + 0x0004)
#define	E_CS0006_TIMER_ERROR		(E_CL_MASK + E_CS_MASK + 0x0006)
#define	E_CS0007_RDY_QUE_CORRUPT	(E_CL_MASK + E_CS_MASK + 0x0007)
#define	E_CS0008_INTERRUPTED		(E_CL_MASK + E_CS_MASK + 0x0008)
#define	E_CS0009_TIMEOUT		(E_CL_MASK + E_CS_MASK + 0x0009)
#define	E_CS000A_NO_SEMAPHORE		(E_CL_MASK + E_CS_MASK + 0x000A)
#define	E_CS000B_NO_FREE_STACKS		(E_CL_MASK + E_CS_MASK + 0x000B)
#define	E_CS000C_INVALID_MODE		(E_CL_MASK + E_CS_MASK + 0x000C)
#define	E_CS000D_INVALID_RTN_MODE	(E_CL_MASK + E_CS_MASK + 0x000D)
#define	E_CS000E_SESSION_LIMIT		(E_CL_MASK + E_CS_MASK + 0x000E)
#define	E_CS000F_REQUEST_ABORTED	(E_CL_MASK + E_CS_MASK + 0x000F)
#define	E_CS0010_QUANTUM_INVALID	(E_CL_MASK + E_CS_MASK + 0x0010)
#define	E_CS0011_BAD_CNX_STATUS		(E_CL_MASK + E_CS_MASK + 0x0011)
#define	E_CS0012_BAD_ADMIN_SCB		(E_CL_MASK + E_CS_MASK + 0x0012)
#define	E_CS0013_CORRUPT_ADMIN_SCB	(E_CL_MASK + E_CS_MASK + 0x0013)
#define	E_CS0014_ESCAPED_EXCPTN		(E_CL_MASK + E_CS_MASK + 0x0014)
#define	E_CS0015_FORCED_EXIT		(E_CL_MASK + E_CS_MASK + 0x0015)
#define	E_CS0016_SYSTEM_ERROR		(E_CL_MASK + E_CS_MASK + 0x0016)
#define	E_CS0017_SMPR_DEADLOCK		(E_CL_MASK + E_CS_MASK + 0x0017)
#define	E_CS0018_NORMAL_SHUTDOWN	(E_CL_MASK + E_CS_MASK + 0x0018)
#define	E_CS0019_INVALID_READY		(E_CL_MASK + E_CS_MASK + 0x0019)
#define	E_CS001A_STACK_DEAL_FAIL	(E_CL_MASK + E_CS_MASK + 0x001A)
#define	E_CS001B_IDLE_RESUME		(E_CL_MASK + E_CS_MASK + 0x001B)
#define	E_CS001C_TERM_W_SEM		(E_CL_MASK + E_CS_MASK + 0x001C)
#define	E_CS001D_NO_MORE_WRITES		(E_CL_MASK + E_CS_MASK + 0x001D)
#define	E_CS001E_SYNC_COMPLETION	(E_CL_MASK + E_CS_MASK + 0x001E)
#define	E_CS001F_CPUSTATS_WASSET	(E_CL_MASK + E_CS_MASK + 0x001F)

#define	E_CS0024_NON_NUMERIC_ARG	(E_CL_MASK + E_CS_MASK + 0x0024)
#define	E_CS0025_NUMERIC_ARG		(E_CL_MASK + E_CS_MASK + 0x0025)
#define	E_CS0026_POSITIVE_VALUE		(E_CL_MASK + E_CS_MASK + 0x0026)
#define	E_CS0029_SEM_OWNER_DEAD		(E_CL_MASK + E_CS_MASK + 0x0029)
#define	E_CS002A_FLOAT_ARG		(E_CL_MASK + E_CS_MASK + 0x002A)
#define	E_CS002B_NONNEGATIVE_VALUE	(E_CL_MASK + E_CS_MASK + 0x002B)
#define	E_CS002C_FRACTION_VALUE		(E_CL_MASK + E_CS_MASK + 0x002C)
#define	E_CS002D_CS_DEFINE		(E_CL_MASK + E_CS_MASK + 0x002D)
#define	E_CS002E_CS_SHORT_FDS		(E_CL_MASK + E_CS_MASK + 0x002E)

#define	E_CS0030_CS_PARAM		(E_CL_MASK + E_CS_MASK + 0x0030)

#define	E_CS0040_ACCT_LOCATE		(E_CL_MASK + E_CS_MASK + 0x0040)
#define	E_CS0041_ACCT_OPEN		(E_CL_MASK + E_CS_MASK + 0x0041)
#define	E_CS0042_ACCT_WRITE		(E_CL_MASK + E_CS_MASK + 0x0042)
#define	E_CS0043_ACCT_RESUMED		(E_CL_MASK + E_CS_MASK + 0x0043)

#define E_CS0047_SHUT_STATE		(E_CL_MASK + E_CS_MASK + 0x0047)

#define E_CS0050_CS_SEMOP_GET           (E_CL_MASK + E_CS_MASK + 0x0050)

#define	E_CS00F9_2STK_OVFL		(E_CL_MASK + E_CS_MASK + 0x00F9)
#define	E_CS00FA_UNWIND_FAILURE		(E_CL_MASK + E_CS_MASK + 0x00FA)
#define	E_CS00FB_UNDELIV_AST		(E_CL_MASK + E_CS_MASK + 0x00FB)
#define	E_CS00FC_CRTCL_RESRC_HELD	(E_CL_MASK + E_CS_MASK + 0x00FC)
#define	E_CS00FD_THREAD_STK_OVFL	(E_CL_MASK + E_CS_MASK + 0x00FD)
#define	E_CS00FE_RET_FROM_IDLE		(E_CL_MASK + E_CS_MASK + 0x00FE)
#define	E_CS00FF_FATAL_ERROR		(E_CL_MASK + E_CS_MASK + 0x00FF)

/******************************************************************************
**
**      Types of commands and modifiers from the monitor session
**
******************************************************************************/

#define		CS_CLOSE        0x0001
#define		CS_COND_CLOSE   0x0002
#define		CS_KILL         0x0003
#define		CS_CRASH		0x0004
#define		CS_START_CLOSE	0x0005

/******************************************************************************
**
**	External Event Definitions
**
******************************************************************************/

#define		CS_THREAD_EVENT		0x0000
#define		CS_SERVER_EVENT		0x0100

#define		CS_ATTN_EVENT	    0x0001
#define		CS_RCVR_EVENT	    0x0002
#define		CS_SHUTDOWN_EVENT   0x0004
#define		CS_NOTIFY_EVENT     0x0008
#define		CS_REMOVE_EVENT     0x0010
#define		CS_CTHREAD_EVENT    0x0020
#define		CS_KILL_EVENT       0x0040

#define		CS_TIMEDOUT		E_CS0009_TIMEOUT
#define		CS_REQUEST_ABORTED 	E_CS000F_REQUEST_ABORTED

/******************************************************************************
**
**	Options for CSalter_session calls.
**
******************************************************************************/

#define		CS_AS_CPUSTATS	    1
#define         CS_AS_PRIORITY      2   /* Change session priority*/


/*
** CS_SEMAPHORE_IS_LOCKED macro definition.
**
** Returns TRUE if the semaphore is apparently locked either
** exclusively or shared. Note that this is a dirty, non-atomic
** peek at the semaphore, hence may not always return a correct
** answer - but that's intentional.
*/
#define CS_SEMAPHORE_IS_LOCKED( sem ) \
        ((sem)->cs_owner || (sem)->cs_count)


/******************************************************************************
**
** Name: CS_CB - Control System Request Control Block
**
** Description:
**      This routine is used to initiate the services of the CS module
**      of the system.  It contains the information necessary for the
**      dispatcher to correctly operate the server.  This information
**      includes the addresses of routines to call to allocate session
**      or thread control blocks, to deallocate SCB's, and the address
**      of a routine to perform main processing.  This routine is always
**      called with a standard parameter interface (see the CSdispatch()
**      routine header for details).
**
**      Also included is the adminstrative information needed to operate
**      the server.  This is primarily information about the maximum number
**      of sessions to allow, the maximum number of active sessions, and the
**      size of the stack needed for each active session for this type of
**      server.
**
******************************************************************************/
typedef struct _CS_CB
{
    i4      cs_scnt;            /* Number of sessions */
    i4      cs_ascnt;           /* nbr of active sessions */
    i4      cs_stksize;         /* size of stack in bytes */
    bool    cs_stkcache;        /* enable stack caching w/threads */ 
    STATUS  (*cs_scballoc)();   /* Routine to allocate SCB's */
    STATUS  (*cs_scbdealloc)(); /* Routine to dealloc  SCB's */
    STATUS	(*cs_saddr)();		/* Routine to await session requests */
    STATUS	(*cs_reject)();		/* how to reject connections */
    STATUS	(*cs_disconnect)();	/* how to dis- connections */
    STATUS	(*cs_read)();		/* Routine to do reads */
    STATUS	(*cs_write)();		/* Routine to do writes */
    STATUS  (*cs_process)();    /* Routine to do major processing */
    STATUS	(*cs_attn)();		/* Routine to process attn calls */
    VOID	(*cs_elog)();       /* Routine to log errors */
    STATUS	(*cs_startup)();	/* startup the server */
    STATUS	(*cs_shutdown)();	/* shutdown the server */
    STATUS	(*cs_format)();		/* format scb's */
    STATUS	(*cs_facility)();	/* return current facility */
    STATUS	(*cs_diag)();	/* DIAG STUFF EXDUMP */
    VOID            (*cs_scbattach)();  /* Attach thread control block to MO */
    VOID            (*cs_scbdetach)();  /* Detach thread control block */
    i4          (*cs_get_rcp_pid)();    /* return RCP's pid */
#define          CS_NOCHANGE     -1	/* no change to this parm */
    char	*(*cs_format_lkkey)();  /*Format an LK_LOCK_KEY for display */
} CS_CB;

/******************************************************************************
**
** Name: CS_SCB_Q - Que entry for the session control block
**
** Description:
**      This is simply a structure containing the standard queue
**      header element for session control blocks.  Building a
**      standard structure for this reduces name space problems
**      as well as guaranteeing correct usage.
**
******************************************************************************/
typedef struct _CS_SCB_Q
{
    CS_SCB *cs_q_next;		/* moving forward . . . */
    CS_SCB *cs_q_prev;		/* as well as backwards */
} CS_SCB_Q;

typedef struct _CS_MULTI_LST CS_MULTI_LST;
struct _CS_MULTI_LST
{
    HANDLE              hSem;
    HANDLE              hCnd;
    i4                  pid;
};

/******************************************************************************
**
** Name: CS_SEMAPHORE - Semaphore for CS module manipulation
**
** Description:
**      This structure is manipulated by the CS module semaphore manipulation
**      routines to control the internal concurrencly on software objects.
** 
** History:
**	03-nov-1997 (canor01)
**	    Remove event member of semaphore structure to cut down on total
**	    number of system resouces consumed.
**	27-jul-1998 (canor01)
**	    Remove extraneous "typedef".
**	24-jul-2000 (somsa01 for jenjo02)
**	    Remove obsolete cs_list.
**	    Remove dubious cs_sem_init_pid, cs_sem_init_addr.
**	    Replace CS_SID cs_sid with CS_THREAD_ID cs_thread_id (OS_THREADS).
**	    Shrunk cs_value, cs_type from i4 to i2.
**	16-Feb-2009 (fanra01)
**	    Switch cs_mutex type.
**
******************************************************************************/
struct _CS_SEMAPHORE
{
    i2          cs_value;
    i2          cs_count;
    CRITICAL_SECTION    cs_section;         /* Within process sync object */
    void*       cs_mutex;                   /* The exclusive semaphore  */
    CS_SCB	*cs_owner;		    /* the owner of the semaphore */
    i2          cs_type;                    /* Type of semaphore */
#define                 CS_SEM_SINGLE   0   /* Normal Semaphore */
#define                 CS_SEM_MULTI    1   /* Shared Memory Semaphore */
    CS_THREAD_ID cs_thread_id;		    /* actual thread id of sem holder */
    i4          cs_pid;                     /* process id of holder if semaphore
                                            ** is CS_SEM_MULTI type */
    i4 		cs_excl_waiters;	    /* true if any EXCL waiters on sem */
    DWORD	cs_test_set;		    /* test-and-set word */
    DWORD       cs_event_lock;              /* spinlock for event creation */
    i4     	cs_pad[1];		    /* pad for now */

    char	cs_sem_name[CS_SEM_NAME_LEN];	/* for CSn_semaphore */
    
    i4     	cs_sem_scribble_check;

# define CS_SEM_LOOKS_GOOD	CV_C_CONST_MACRO('I','S','E','M')
# define CS_SEM_WAS_REMOVED	CV_C_CONST_MACRO('D','S','E','M')

    struct  _CS_EXT_SMSTAT
    {
        u_i4            cs_smsx_count;  /* collisions on shared vs. exclusive */
        u_i4            cs_smxx_count;  /* exclusive vs. exclusive */
        u_i4            cs_smx_count;   /* # requests for exclusive */
        u_i4            cs_sms_count;   /* # requests for shared */
        u_i4            cs_sms_hwm;     /* hwm of sharers */
    }               cs_smstatistics;    /* statistics for semaphores */

# define CS_MULTI_LST_COUNT		4
    CS_MULTI_LST    cs_multi_lst[CS_MULTI_LST_COUNT];

};


/* No such function on WNT */
#define CSnoresnow(a,b)


#define CSp_SEMAPHORE(exclusive, sem, status)\
    status = CSp_semaphore (exclusive, sem)

#define CSv_SEMAPHORE(sem, status) \
    status = CSv_semaphore (sem)


/*
** Name: CS_CPID	- CrossProcess thread IDentifier
**
** Description:
**	This block holds information about a thread which participates
**	in a cross-process suspend/resume.
**
** History:
**	15-Oct-1998 (jenjo02)
**	    Created.
**	20-dec-1999 (somsa01)
**	    Added holding of the thread's id.
*/
struct _CS_CPID
{
    i4     		pid;		/* suspended thread's process */
    CS_SID		sid;		/* suspended thread's sid */
    DWORD		tid;		/* suspended thread's id */
}; 

/******************************************************************************
**
** Name: CS_CONDITION - Condition waiting object
**
** DESCRIPTION:
**              A CS_CONDITION is a construct useful for waiting on
**      software objects.  A condition differs from a semaphore in that it has
**      different semantics which avoid deadlock problems when protection is
**      needed for a member of a pool of objects which is protected by another
**      semaphore.  Conditions together with a mutex (exclusive semaphore)
**      form a 'monitor' as first described by C.A.R. Hoare.  See
**      ``An Introduction to Programming with Threads'' by Andrew D.
**      Birrell (DEC SRC) and ``Threads Extension for Portable Operating
**      Systems'' IEEE P1003.4a
**
**      Note that a CS_CONDITION may only be accessed by sessions which are
**      members of the same server (that is, they must all have been
**      created by the same caller of CSadd_thread() ). Sessions in a
**      different server may NOT share this CS_CONDITION. Therefore,
**      CS_CONDITION structures probably should NOT be placed in
**      system-wide shared memory; rather, they should be located in
**      memory which is visible only to the sessions in this server.
**
** DEFINITIONS/CONCEPTS:
**              A condition structure is used as a descriptor to wait upon for
**      an event which will be later signaled by another session.  Typically,
**      the condition structure will contain a queue of sessions waiting upon
**      the condition to be signaled.  When a condition is signaled, a single
**      thread waiting on that condition will be made runnable again.
**              Typical use of a condition is that a session which
**      holds a mutex finds it must wait for some other session to do
**      work.  This can happen with an object pool manager which allows
**      objects to exist in the pool while they are being constructed.
**      For example, if one session is building a plan which other
**      sessions want to use, the sessions which want to use the plan
**      can wait on a condition structure until the plan is built and
**      they are signaled to go ahead.
**              When waiting on a condition, the same mutex should be
**      specified on each wait.  (i.e. a mutex can be associated with
**      several conditions, but a condition can be associated with
**      only one mutex.) There is no advantage to using a
**      cross-process semaphore (that is, a semaphore of type
**      CS_SEM_MULTI) with a CS_CONDITION. While some current
**      implementations of CS may allow this, it is not recommended
**      and CS clients should NOT use cross-process semaphores with
**      CS_CONDITION's.
**
******************************************************************************/
struct _CS_CONDITION
{
    CS_CONDITION	*cnd_next, *cnd_prev;	/* queue up waiter here */
    CS_SCB		*cnd_waiter;	/* point back to waiting session */
    char		*cnd_name;	/* point to name string */
    CS_SEMAPHORE	*cnd_mtx;	/* External semaphore supplied
					** during CScnd_wait()
					*/
};

/******************************************************************************
**
** Name: CS_SCB - CS dispatcher's session cb portion
**
** Description:
**      This block is expected to be allocated at the beginning
**      of the system session control block.  That is, when the
**      CS_cb.cs_scballoc() routine is called, it will return a pointer
**      to the CS_SCB portion of the underlying systems SCB.  CS will
**      never use more that this portion.  The top portion of this
**      standard header is expected to be filled in by the aforementioned
**      routine.
**
**      This block is used by the dispatcher to maintain the state of a
**      particular thread.  This section is very machine dependent, and is
**      expected to be altered extensively by other environments.
**
**	NOTE : This structure is referenced in CSLL.MAR with hard-wired offsets
**	to some of the fields.  Any changes in this structure will require
**	changes to the offset definitions in CSLL.MAR as well.
**
******************************************************************************/

struct _CS_SCB
{
    CS_SCB       *cs_next;          /* forward queue link */
    CS_SCB       *cs_prev;          /* Backward  "     "  */
    SIZE_TYPE           cs_length;         /* Length, including portion not seen */
    i2           cs_type;           /* control block type */
#define				CS_SCB_TYPE ((i2)0xABCD)
    i2           cs_s_reserved;     /* used by mem mgmt perhaps */
    PTR		 cs_l_reserved;
    PTR          cs_owner;          /* owner of block */
#if defined(NT_IA64) || defined(NT_AMD64)
#define				CS_IDENT	0xAB000000000000BA
#else
#define				CS_IDENT	0xAB0000BA
#endif  /* NT_IA64 || NT_AMD64 */
    i4           cs_tag;            /* to look for in a dump (ick) */
#define				CS_TAG          CV_C_CONST_MACRO('c','s','c','b')
    CS_SID	     cs_self;			/* session id for this session */
#define				CS_DONT_CARE    ((CS_SID)  0)
#define				CS_ADDR_ID		((CS_SID) -1)
#define				CS_ADDER_ID		CS_ADDR_ID
    i4     	     cs_client_type;	/* must be 0 in CL SCBs */
    ULONG        cs_stk_size;       /* Amt of space allocated there */
    i4 		     cs_state;          /* Thread state */
#define CS_STATE_ENUMS /* enum cs_state_enums */\
_DEFINE(		CS_FREE,            0x0000 )\
_DEFINE(		CS_COMPUTABLE,      0x0001 )\
_DEFINE(		CS_EVENT_WAIT,      0x0002 )\
_DEFINE(		CS_MUTEX,           0x0003 )\
_DEFINE(		CS_STACK_WAIT,      0x0004 )\
_DEFINE(		CS_UWAIT,           0x0005 )\
_DEFINE(		CS_CNDWAIT,         0x0006 )\
_DEFINESEND

   i4 		     cs_mask;
#define CS_MASK_MASKS /* enum cs_mask_masks */\
_DEFINE(		CS_TIMEOUT_MASK,    0x00001 /* timeout can happen */)\
_DEFINE(		CS_INTERRUPT_MASK,  0x00002 /* interrupts are allowed */)\
_DEFINE(		CS_BAD_STACK_MASK,  0x00004 /* thread trashed stack once */)\
_DEFINE(		CS_MUTEX_MASK,	    0x00008 /* spinning on MP sem */)\
_DEFINE(		CS_NANO_TO_MASK,    0x00010 /* Timeout in nanoseconds */)\
_DEFINE(		CS_unused5,         0x00020 /* bit 5 used in 3.0 */)\
_DEFINE(		CS_DEAD_MASK,	    0x00040 /* Thread is doomed */)\
_DEFINE(		CS_IRPENDING_MASK,  0x00080 /* interrupt is pending */)\
_DEFINE(		CS_NOXACT_MASK,	    0x00100 /* thread not in xact */)\
_DEFINE(		CS_MNTR_MASK,	    0x00200 /* job is a monitor */)\
_DEFINE(		CS_FATAL_MASK,	    0x00400 /* thread has fatal error */)\
_DEFINE(		CS_ABORTED_MASK,    0x00800 /* request aborted */)\
_DEFINE(		CS_MID_QUANTUM_MASK,0x01000 /* job start mid quantum */)\
_DEFINE(		CS_EDONE_MASK,	    0x02000 /* Event has completed */)\
_DEFINE(		CS_IRCV_MASK,	    0x04000 /* Job recieved an intrpt */)\
_DEFINE(		CS_TO_MASK,	    0x08000 /* Job has timed out */)\
_DEFINE(		CS_CPU_MASK,	    0x10000 /* Collect CPU stats */)\
_DEFINE(		CS_DIZZY_MASK,      0x20000 /* Spinning way too long */)\
_DEFINE(		CS_SMUTEX_MASK,     0x40000 /* blocked on shared mtx */)\
_DEFINE(		CS_NOSWAP,          0x80000 /* Don't swap out if timer goes off */)\
_DEFINESEND

    i4               cs_priority;	/* priority of thread */
    i4               cs_base_priority;  /* base priority */
    i4 		     cs_thread_type;	/* type of thread */
#define                 CS_NORMAL         0       /* Normal user session */
#define                 CS_MONITOR        1       /* Monitor session */
#define                 CS_FAST_COMMIT    2       /* Fast Commit Thread */
#define                 CS_WRITE_BEHIND   3       /* Write Behind Thread */
#define                 CS_SERVER_INIT    4       /* Server initiator     */
#define                 CS_EVENT          5       /* EVENT service thread */
#define                 CS_2PC            6       /* 2PC recovery thread */
#define                 CS_RECOVERY       7       /* Recovery Thread */
#define                 CS_LOGWRITER      8       /* logfile writer thread */
#define                 CS_CHECK_DEAD     9       /* Check-dead thread */
#define                 CS_GROUP_COMMIT   10      /* group commit thread */
#define                 CS_FORCE_ABORT    11      /* force abort thread */
#define                 CS_AUDIT          12      /* audit thread */
#define                 CS_CP_TIMER       13      /* consistency pt timer */
#define                 CS_CHECK_TERM     14      /* terminator thread */
#define                 CS_SECAUD_WRITER  15      /* security audit writer */
#define                 CS_WRITE_ALONG    16      /* Write Along thread */
#define                 CS_READ_AHEAD     17      /* Read Ahead thread */
#define			CS_INTRNL_THREAD -1	/* Thread used by CS */
#define			CS_USER_THREAD    0	/* User session */
    i4		     cs_mode;			/* what's up now */
    i4		     cs_nmode;			/* what's up next */

    i4		     cs_cputime;		/* cputime of this thread (10ms) - is periodically reset to zero */
    						/* as a side-effect of OPF activity, unless session accounting is ON */  
    i4		     cs_inkernel;		/* boolean flag: are we in kernel mode or not? */
    i4              cs_sem_count;		/* number of semaphores
						** owned here
						*/

    SPBLK 	     cs_spblk;			/* used within CS for known
						** thread tree
						*/
    i4	             cs_dior;			/* disk io read count */
    i4    	     cs_diow;			/* disk io write count */
    i4    	     cs_bior;			/* fe/dbms comm count */
    i4    	     cs_biow;			/* fe/dbms comm count */
    i4		     cs_locks;			/* number of locks waits */
    i4               cs_lkevent;		/* number of LKevent waits */
    i4               cs_logs;			/* number of log waits */
    i4               cs_lgevent;		/* number of LGevent waits */
    i4               cs_lior;			/* number of log read waits */
    i4               cs_liow;			/* number of log write waits */
    PTR		     cs_sync_obj;		/* Ptr to object waiting on */
    i4		     cs_memory;			/* amount of memory used */
    i4		     cs_connect;		/* time in seconds of
						** connection
						*/
    i4		     cs_pid;			/* process id */
    PTR		     cs_svcb;			/* server control block addr */
    HANDLE	     cs_access_sem;		/* semaphore for access to
						** scb
						*/
    HANDLE	     cs_event_sem;		/* semaphore for event
						** signaling
						*/
    HANDLE	     cs_di_event;		/* overlapped event for
						** thread dio
						*/
    CS_CONDITION     *cs_cnd;			/* if we are waiting on a
						** condition
						*/
    char	     cs_username[CS_MAXNAME];	/* user owning this session */
    HANDLE	     cs_thread_handle;  /* thread handle (from CreateThread) */
    DWORD            cs_thread_id;      /* thread id (from CreateThread) */
    i4     	     cs_cs_mask;                /* CS-specific session flags */
#define                 CS_CLEANUP_MASK    0x1000   /* Must cleanup user
                                                       sessions before exit */
    CS_CONDITION    cs_cndq;			/* For queueing SCBs awaiting
						** the same condition
						*/
    PTR             cs_diag_qry;        /* pointer to the current query for theread */
    u_i4	    cs_random_seed;	/* The random function seed value  */
    PTR             cs_ditgiop;		/* A place for DI gatherWrite to
					** anchor a work area.
					*/
};

/*}
** Name: CS_SEM_STATS - Structure into which semaphore statistics
**                      may be returned.
**
** Description:
**      This structure is filled with selected statistics in response
**      to a CSs_semaphore() function call.
**
** History:
**      07-Dec-1995 (jenjo02)
**          Created.
*/
struct _CS_SEM_STATS
{
    char            name[CS_SEM_NAME_LEN]; /* semaphore name */
    i4              type;               /* semaphore type,
                                           CS_SEM_SINGLE or
                                           CS_SEM_MULTI */
    u_i4            excl_requests;      /* count of exclusive requests */
    u_i4            share_requests;     /* count of share requests */
    u_i4            excl_collisions;    /* exclusive collisions */
    u_i4            share_collisions;   /* share collisions */
/* The following fields apply to CS_SEM_MULTI types only: */
    u_i4            spins;              /* number of cross-process spins */
    u_i4            redispatches;       /* number of redispatches */
};

/******************************************************************************
**
** Name: CS_INFO_CB - Server information block
**
** Description:
**      This block is passed to the server startup function to pass
**      to inform the server of various facts in which it may be
**      interested.
**
******************************************************************************/

typedef struct _CS_INFO_CB
{
    i4           cs_scnt;           	/* Session count */
    i4           cs_ascnt;          	/* active session count */
    i4 		 cs_min_priority;	/* Min priority for server thread */
    i4 		 cs_max_priority;	/* Max priiority for server thread */
    i4 		 cs_norm_priority;	/* Priority used for user sessions */
    char	 cs_name[16];		/* name of the server */
    PTR		 cs_svcb;		/* filled in by initiation */
    CS_SEMAPHORE *cs_mem_sem;		/* MEgetpages protection */
    /*
    ** Additional diagnostic info will be transferred to the server block
    */
    i4 		 sc_main_sz;	/* Size of SCF Control */
				/* Block(SC_MAIN_CB,Sc_main_cb) */
    i4 		 scd_scb_sz;	/* Size of Session Control Block(SCD_SCB) */
    i4 		 scb_typ_off;	/* The session type offset inside session cb. */
    i4 		 scb_fac_off;	/* The current facility offset inside session */
				/* cb. */
    i4 		 scb_usr_off;	/* The user-info off inside session control */
				/* block */
    i4 		 scb_qry_off;	/* The query-text offset in session control */
				/* block */
    i4 		 scb_act_off;	/* The current activity offset in session cb. */
    i4 		 scb_ast_off;	/* The alert(event) state offset in session */
				/* cb. */
} CS_INFO_CB;

/*}
** Name: CS_THREAD_STATS_CB - Thread stats parameter block
**
** Description:
**      This block supplies parameters to CSMT_get_thread_stats().
**
** The first section of input parameters enables identification of 
** the thread in question.
** The next section of output parameters holds the stats returned
** for a successful 'get'. 
** The final input flag indicates which stats are required.
**
** History:
**      09-Feb-2009 (smeke01) b119586 
**          Created.
*/
typedef struct _CS_THREAD_STATS_CB
{
    /* input parameters: process/thread identifiers */
    HANDLE	     cs_thread_handle;
    /* output parameters: stats fields */
    i4	    	    cs_usercpu;		/* OS cpu usage for thread (user portion) */
    i4	    	    cs_systemcpu;	/* OS cpu usage for thread (system portion) */
    /* input parameter: select stats required */ 
    i4		    cs_stats_flag;	/* OR'd binary flag  */
#define CS_THREAD_STATS_CPU 0x00000001	/* Retrieve user & system CPU stats for the thread */
}   CS_THREAD_STATS_CB;


/*
** Max number of CS threads we'll allow.  Make this reasonably high.
*/

#define	CS_MAX_THREADS	64

/*
** CSaccept_connect is an artifact currently referenced but not used
** (according to seiwald) in scf.
*/

#define	CSaccept_connect	NULL


/******************************************************************************
**
**  Function Prototypes
**
******************************************************************************/

FUNC_EXTERN STATUS iimksec(SECURITY_ATTRIBUTES *sa);
FUNC_EXTERN i4 CS_checktime(VOID);


/******************************************************************************
**
** defines for CSsuspend()
**
******************************************************************************/

#define CS_IOW_MASK	0x200000	/* I/O Write */
#define CS_IOR_MASK	0x100000	/* I/O Read */
#define CS_LIO_MASK	0x080000	/* Log I/O */
#define CS_LGEVENT_MASK 0x040000	/* Log event */
#define CS_LKEVENT_MASK 0x020000	/* Lock event */
#define	CS_DIO_MASK	0x008000	/* Disk I/O */
#define	CS_BIO_MASK	0x004000	/* BIO */
#define	CS_LOCK_MASK	0x002000	/* Lock */
#define	CS_LOG_MASK	0x001000	/* Log */

#define                 CS_BIOR_MASK    (CS_BIO_MASK | CS_IOR_MASK)
#define                 CS_BIOW_MASK    (CS_BIO_MASK | CS_IOW_MASK)
#define                 CS_DIOR_MASK    (CS_DIO_MASK | CS_IOR_MASK)
#define                 CS_DIOW_MASK    (CS_DIO_MASK | CS_IOW_MASK)
#define                 CS_LIOR_MASK    (CS_LIO_MASK | CS_IOR_MASK)
#define                 CS_LIOW_MASK    (CS_LIO_MASK | CS_IOW_MASK)

#define		CSset_id(scb)


/******************************************************************************
**
** defines for NT's version of setiud
**
******************************************************************************/

#define RUN_COMMAND_AS_INGRES		128
#define SERVICE_CONTROL_STOP_NOINGSTOP	129

struct SETUID
{
    DWORD	CreatedProcID;
    char	ClientProcID[32];
    char	WorkingDirectory[256];
    char	cmdline[2048];
    DWORD	ExitCode;
};

struct SETUID_SHM
{
    bool	pending;
};

struct REALUID_SHM
{
    char	RealUserId[25];
    bool	InfoTaken;
};

typedef struct _ASYNC_TIMEOUT
{
        HANDLE event;
        HANDLE fd;
} ASYNC_TIMEOUT;

/*
** Enum expansion for _DEFINE definitions above.
*/
#define _DEFINESEND

#define _DEFINE(s,v) s = v,
enum cs_state_enums { CS_STATE_ENUMS cs_state_dummy };
GLOBALCONSTREF char cs_state_names[];
enum cs_mask_masks { CS_MASK_MASKS cs_mask_dummy };
GLOBALCONSTREF char cs_mask_names[];
#undef _DEFINE

#undef _DEFINESEND


#endif /* CS_INCLUDED */
