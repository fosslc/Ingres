/*                                      
** Copyright (c) 1986, 2009 Ingres Corporation
*/

#ifndef CSNORMAL_H
#define CSNORMAL_H
 

# include <sp.h>
# include <qu.h>
# ifdef OS_THREADS_USED
# include <systypes.h>
# ifdef SYS_V_THREADS
# include <synch.h>
# include <thread.h>
# endif /* SYS_V_THREADS */
# if defined(sparc_sol) || defined(a64_sol) || defined(hpb_us5)
# include    <sys/lwp.h>
# endif /* need lwp.h */
# ifdef POSIX_THREADS
# include <mecl.h>
# if defined(rux_us5)
# include <dce/pthread.h>
# else
# include <pthread.h>
# endif /* rux_us5 */
# if defined(sgi_us5) || defined(LNX)
# include <sched.h>
# endif /* sgi_us5 Linux */
# endif /* POSIX_THREADS */
# endif /* OS_THREADS_USED */
# ifdef OSX
# include <libkern/OSTypes.h>
# include <libkern/OSAtomic.h>
# endif

# ifdef OS_THREADS_USED
#if defined(dg8_us5) || defined(dgi_us5)
#include <unistd.h>
#define PTHREAD_PROCESS_SHARED        _POSIX_THREADS_PROCESS_SHARED
#define PTHREAD_SCOPE_PROCESS PTHREAD_SCOPE_LOCAL
#define PTHREAD_SCOPE_SYSTEM  PTHREAD_SCOPE_GLOBAL
static int pthread_create_detached = 1;
#define PTHREAD_CREATE_DETACHED &pthread_create_detached
#endif /* dg8_us5 || dgi_us5 */
# endif /* OS_THREADS_USED */
#if defined(xCL_SUSPEND_USING_SEM_OPS)
#include <semaphore.h>
#endif
#if defined(VMS)
#include <builtins.h>
#include <iosbdef.h>
#if defined(KPS_THREADS)
#include <kpbdef.h>
#endif
#endif

/**CL_SPEC
** Name: CSNORMAL.H - Control System CL Module Global Header
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
** History:
**      24-Oct-1986 (fred)
**          Initial definition.
**      10-Nov-1986 (fred)
**          Added error definitions.
**      3-Dec-1986 (fred)
**          Added CL Committee style description (above).
**      15-jan-1987 (fred)
**          Added server initiation block.
**      15-jan-1987 (fred)
**          Added server initiation block.
**      25-sep-1987 (fred)
**          Redocumented for extensions needed for GCF work.
**	26-sep-1988 (rogerk)
**	    Added fields to CS_INFO_CB specifying CS thead priorities.
**	21-oct-1988 (rogerk)
**	    Added CS_CPU_MASK to cs_scb->cs_mask to specify that cpu statistics
**	    should be kept for the session.
**	    Added defines for CSaltr_session call.
**	31-oct-1988 (rogerk)
**	    Added cso_facility for CS_OPTIONS struct.
**	 7-oct-1988 (rogerk)
**	    Added fields in the CS_SCB in which asynchronous routines can write
**	    values without colliding with other threads.
**	    Added return value for CSaltr_session (E_CS001F_CPUSTATS_WASSET).
**	20-feb-1989 (markd)
**	    Added Sequent specific stuff.
**      28-feb-1988 (rogerk)
**          Added cs_pid field to CS_SCB.  Added cs_type, cs_pid to CS_SEMAPHORE
**          structure for cross process semaphores.
**	15-apr-89 (arana)
**	    Since compat.h doesn't include sys/types.h anymore, include
**	    systypes.h in here for the pyramid specific stack context data
**	    structure from sys/context.h for the CS_SCB.
**	11-may-89 (daveb)
**		Remove RCS log
**	19-jul-89 (rogerk)
**	    Added cs_test_set field to CS_SEMAPHORE structure for cross-process
**	    semaphores. Added cs_ef_mask to scb for event flag wait/wake calls.
**	24-jul-89 (rogerk)
**	    Added CS_ASET definition here (moved from csev.h) in order to use
**	    it in the CS_SEMAPHORE structure.
**      15-mar-90 (jkb) Declare CS_ASET as volatile for sequent so the
**          compiler optimizer will do the right things with it
**	23-mar-1990 (mikem)
**	    Added some performance stat fields to the CS_SEMAPHORE structure.
**	13-apr-90 (kimman)
**	    Adding ds3_ulx box specific information.
**	22-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Added machine-specific information for dr3_us5, dr4_us5, 
**		vax_ulx, ps2_us5, sco_us5, hp8_us5, dr6_us5, bul_us5, hp3_us5,
**		aco_u43, dr5_us5, ib1_us5, arx_us5, cvx_u42, x3bx_us5, dg8_us5;
**		CSaccept_connect() now returns a STATUS not a bool;
**		Remove superfluous typedef before structures;
**		Sized cs_name according to GCC_L_PORT and included <gcccl.h>
**		to pick up the GCC_L_PORT length value.
**	1-june-90 (blaise)
**	    Integrated changes from termcl code line:
**		Sun4 needs a long as the atomic type for multiprocessors;
**		Add new CS error E_CS0029_SEM_OWNER_DEAD to return from 
**		CSp_semaphore() when a request is made for a multi-process
**		semaphore which is held by a process which has exited.
**	7-june-90 (blaise)
**	    Added missing "#define cs_aset_type" to su4_u42 #ifdef.
**	    Moved atomic test & set type and atomic test & set op definitions
**	    from <csev.h> to this file. The atomic test & set type
**	    definitions for different boxes were split between the two files.
**	7-june-90 (blaise)
**	    Backed out part of 22-may change, where I changed the return type
**	    of CSaccept_connect(). It does in fact retun a bool.
**      4-sep-90 (jkb)
**	    Add sqs_ptx to the appropiate list of #defines
**	1-oct-90 (chsieh)
**	    Added initial definitions for Bull (bu2_us5) and (bu3_us5)
**	3-Oct-90 (anton)
**	    Reentrant CL changes
**	    Support for CS_CONDITIONS
**      13-nov-90 (jkb)
**          Rename cs.h to csnormal.h so the correct defines 
**          can be included for both mct and
**          non-mct installations  Cs.h now just includes the appropriate
**	    version of cs defines, csnormal.h or csmt.h.
**	09-jan-91 (ralph)
**	    added cso_float to pass in floating point numbers
**	10-Jan-91 (anton)
**	    Added definition of cs_mem_sem
**	21-Jan-91 (jkb)
**	    Added definition of CS_NOTIFY_EVENT
**	28-jan-91 (seputis)
**	    added FUNC_EXTERN for CS_CONDITIONs
**	26-feb-91 (jab)
**	    cleaned up an "#ifndef CS_ACLR(x)" for the ANSI cpp.
**	25-mar-91 (kirke)
**	    Removed obsolete cs_mach for hp8_us5, added current version.
**      22-apr-91 (szeng)
**          Added vax_ulx specific stuff.
**	21-mar-91 (seng)
**	    Add RS/6000 specific entries.
**	22-apr-91 (rudyw)
**	    Add odt_us5 to appropriate section.
**	25-sep-1991 (mikem) integrated following change: 10-jul-91 (kirke)
**	    Changed cs_mach for hp8_us5 to handle HP-UX 8.0 jmp_buf change.
**	25-sep-1991 (mikem) integrated following change: 09-aug-91 (ralph)
**	    Added E_CS0040_ACCT_LOCATE, E_CS0041_ACCT_OPEN, E_CS0042_ACCT_WRITE,
**	    and E_CS0043_ACCT_RESUMED for UNIX session accounting.
**          Added CS_REMOVE_EVENT for IIMONITOR REMOVE
**    12-Feb-92 (daveb)
**        Add cs_client_type field to allow clients to type-tag their
**        scb's that may have a CS_SCB as a common initial member.
**        Part of fix for bug 38056.  *
**	06-jul-92 (swm)
**	    Made definition of CS_TAS for DRS6000 do a 'snoop' check to
**	    avoid unnecessary bus locks, particularly in the CS_getspin
**	    macro which invokes CS_TAS.
**	    Added definition of CS_ACLR and registers for DRS3000.
**	4-jul-92 (daveb)
**	  Add SPBLK to CS_SCB, CS_SEMAPHORE and CS_CONDITION
**	  structures, and add name field to semaphore for
**	  CSn_semaphore.  This will allow simple MO objects for
**	  getting at sessions, sems, and conditions. The downside is
**	  that it requires inclusion of <sp.h> every place that <cs.h>
**	  is now included.  We'll punt that for now by including it
**	  here, and making sure that <sp.h> is gated.
**	9-jul-92 (daveb)
**	  Take out spblk from the semaphore; that can't work as the
**	  sem may be in shared memory at different locations.  Instead,
**	  add some administrative fields that let us do some sanity
**	  checking, and identify the semaphore.
**	9-nov-92 (ed)
**	    Extend object names to 32 DB_MAXNAME
**	23-Oct-92-1992 (daveb)
**	    Prototype external interfaces.  Add CS_SEM_NAME_LEN.
**	4-Dec-1992 (daveb)
**	    prototyped CScnd_name.
**	06-nov-92 (mikem)
**	  su4_us5 port.  Defined machine specific stack stuff for su4_us5 port.
**	  Followed ifdef's for su4_u42.
**	19-jan-92 (sweeney)
**	  add usl_us5 to the Intel [34]86 cases.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	9-aug-93 (robf)
**	    Add su4_cmw
**	12-jan-93 (swm)
**	    Added machine specific stack code for axp_osf port.
**	1-Sep-93 (seiwald)
**	    CS option revamp: added startup error definitions so that the
**	    CL needn't include erclf.h (which is built by ercompile, which
**	    itself is built by the CL).  Added new errors that were 
**	    previously SIprintf'ed.  Removed CS_OPTIONS.
**	20-sep-93 (mikem)
**	    Added cs_smstatistics structure to the CS_SEMAPHORE structure to
**	    keep semaphore contention statistics on a per semaphore basis.
**	18-oct-93 (swm)
**	    Added cs_sync_obj to unix-specific part of CS_SCB type to hold
**	    synchronisation object pointers. The cs_memory element is
**	    being overloaded but a pointer does not fit into a i4
**	    on all machines.
**	    This field is currently unused, it has been added as a hook for
**	    my next integration.
**	22-oct-93 (robf)
**          Added  CS_AS_PRIORITY option
**	28-oct-93 (seiwald)
**	    Define E_CS005x messages for the benefit of cscp.c.
**	24-oct-93 (swm)
**	    Bug #56445
**	    Now using cs_sync_obj.
**	31-jan-94 (jnash)
**	    Add new CSinitiate() errors: E_CS0020_GETPWNAM_FAIL,
**	    E_CS0021_SETEUID_FAIL, E_CS0022_MAP_SSEG_FAIL, 
**	    E_CS0023_CS_EVINIT_FAIL.
**      28-Nov-94 (nive/walro03)
**          Added CS_ASET, cs_aset_op, CS_ISSET, CS_ACLR, CS_TAS,
**          CS_SPININIT, CS_relspin for dg8_us5.
**	20-mar-95 (smiba01)
**	   Added support for dr6_ues based on dr6_us5
**	16-Jun-95 (walro03)
**	   Removed CS_relspin for dg8_us5.  Correct definition is in
**	   cl/clf/hdr/csev.h.  Corrected CS_SPININIT definition.
**	25-May-95 (jenjo02)
**	    Defined CSp|v_semaphore macros to acquire and free semaphores
**	    using in-lined code when possible; when not possible,
**	    the corresponding function call is made.
**	 6-Jun-95 (jenjo02)
**	    Changed obsolete cs_cnt_sem, cs_long_cnt, cs_ms_long_et to
**	    cs_pad, added cs_excl_waiters in CS_SEMAPHORE.
**	15-jul-95 (popri01)
**	   Add nc4_us5 (intel platform)
**	17-jul-95 (morayf)
**	    Add sos_us5 to appropriate section.
**	25-jul-95 (allmi01)
**	   Added dgi_us5 (DG-UX on Intel based platforms) following
**	   usl_us5 (UNIXWARE).
**      29-jul-1995 (pursch)
**          Add changes by gmcquary to 6405 release in 19-oct-92.
**          Add pym_us5 specific defines.
**          Add changes by erickson to 6405 release in 3-dec-94.
**          pym_us5: use spinlock(3X) fns for CS_TAS, etc.
**          Reluctantly use spinunlock() for CS_ACLR (would prefer direct
**          set of variable, but that isn't being reliable).
**          We don't use code from the prior Pyramid port, as those
**          ABI-ish fns no longer exist.
**      04-oct-1995 (wonst02)
**          Add cs_facility to CS_CB for the address of scs_facility().
**	13-nov-1995 (murf)
**		Added support for sui_us5 based on usl_us5.
**	07-Dec-1995 (jenjo02)
**	    Added new cs_mode, CS_MUTEX_MASK, which is set when a
**	    session is computable but spinning trying to acquire
**	    a cross-process semaphore.
**
**	    Added new structure, CS_SEM_STATS, into which a semaphore's
**	    statistics will be returned in response to a (new)
**	    CSs_semaphore() function call.
**	
**	    Defined a new macro, CS_SEMAPHORE_IS_LOCKED, which offers
**	    a dirty look at a semaphore's state, returning TRUE
**	    if the semaphore is apparently latched.
**
**	    Removed define of CS_GWFIO_MASK 0x1000, which wasn't being
**	    used and changed CS_LOG_MASK 0x0000 to 0x1000. CSsuspend()
**	    calls which don't specify a "wait type" mask have always
**	    defaulted to "CS_LOG_MASK", skewing monitor and statistics
**          by incorrectly showing threads waiting on the LOG when
**  	    they're really just waiting on a timer. Real waits on the
**	    log will now be specified by the explicit use of
**	    CS_LOG_MASK.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT and
**          using data from a DLL in code built for static libraries.
**	14-Mch-1996 (prida01)
**	    Add cs_diag call.
**	13-dec-95 (morayf)
**	   Added SNI RMx00 (rmx_us5) register and spinlock code like
**	   the old Pyramid code (pym_us5) in previous code lines.
**	   Yet again, these Pyramid changes don't seem to have made it here
**	   despite previous pursch comment.
**	5-dec-96 (stephenb)
**	    Add defines for CS facility ID's (currently the same as DM facility
**	    ID's in iicommon.h).
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          mv csmt.h into csnormal.h
**      07-mar-1997 (canor01)
**          Add appropriate definitions of CS_ACLR and CS_SPININIT for
**          su4_us5 when OS_THREADS_USED is not defined.
**	20-mar-1997 (canor01)
**	    Add definition for CS_NANO_TO_MASK to provide for nanosecond
**	    timeout requests in CSsuspend().
**	02-may-1997 (canor01)
**	    Add cs_cs_mask to CS_SCB for session flags specific to CS.
**	08-may-1997 (muhpa01)
**	    Added POSIX_DRAFT_4 definitions for pthread functions.
**      09-May-1997 (merja01)
**          Change definitions of pthread_self and pthread_yield  
**          for axp_osf to match axp_osf Digital Unix 1003.1c
**          implementation. Set attr to NULL on pthread_mutex_init
**          to pick up defaults.
**	27-may-1997 (kosma01)
**	    AIX 4.1 (rs4_us5) does not support cross-process pthread 
**	    syncronization, (no PTHREAD_PROCESS_SHARED), for mutexes and
**	    condition varaibles. Cross-process syncro is needed for
**	    CS_SEM_MULTI type semaphores. Simulate (weakly), cross-process
**	    syncronization with CS_TAS of shared storage. Also define
**	    macro SIMULATE_PROCESS_SHARED to support this. However,
**	    the simulation has a limitation. It does not "wake up"
**	    threads waiting for a resource that has just become 
**	    available, as real PTHREAD_PROCESS_SHARED does. It does
**	    not matter if the waiting thread is process releasing the 
**	    resource or in another process, it is not signaled. Instead
**	    a flag is set and as the other threads are dispatched by the
**	    OS, then they will check the flag and act according to its
**	    value.
**
**          For POSIX_THREADS, CS_thread_id_assign will not compile when
**          defined as MECOPY_CONST_MACRO, because in all instances a
**          function is passed,(CS_get_thread_id()), as an argument to
**	    CS_thread_id_assigni and MECOPY_CONST_MACRO wants an address. 
**	    This problem has been addressed by changing the CS_thread_
**	    id_assign macro to be (a = b), and ensuring that the target
**	    type for POSIX_THREADS is pthread_t. 
**  16-Jun-1997 (kinpa04)
**          Made the following changes to correct mutex deadlock errors
**          on axposf using POSIX threads:
**          1. Change the SIMULATE_PROCESS_SHARED routine for 
**	       CS_cp_synch_trylock to call CS_tas directly and
**	       remove superflous call to CS_ISSET.
**          2. Remove CS_ISSET from CS_TAS definition for axposf.  
**          3. Remove else from end of CS_getspin defintion for axposf.
**             If we spin waiting for the lock, the else causes us to 
**             erroneously jump over the next instruction when we break
**             out of the spin loop.
**	20-jun-1997 (muhpa01)
**	    Fixed CS_getspin & CS_SPININIT defs for hp8_us5 by removing
**	    the tailing 'else' condition
**	03-Jul-97 (radve01)
**	    Several info added to the end CS_INFO_CB structure to facilitate
**	    debugging.
**  22-Jul-1997 (kinpa04)
**      Added pthread priorities and scheduling for axp_osf.
**	10-Sep-1997/02-may-1997 (popri01)
**	    Modify Unixware (usl_us5) for OS_THREADS, per Solaris (su4_us5)
**	31-jul-1997 (walro03)
**	    Updates for Tandem NonStop (ts2_us5) port.
	23-sep-1997 (hanch04)
**	    Changed ifdef to if
**      08-Apr-1997 (bonro01)
**          Redefined some posix defines to their dg8_us5 names.
**          Changed the CS_thread_id_assign macro to an assignment
**          because the MECOPY macro could not handle one of the
**          arguments being a function.
**          Defined CS_thread_sigmask to be sigprocmask on dg8_us5.
**          Added pthread_create_detached global variable of 1 because  
**          DG compiler could not take the address of a literal 1.
**          Added pthread_mutexattr_destroy and pthread_condattr_destroy
**          calls for corresponding create calls.
**      15-May-1997 (hweho01)
**          Extended the fix dated 08-Apr-1997 by bonro01 to dgi_us5.
**	16-sep-1997 (canor01)
**	    Add CND_WAITING define for use by OS-threaded CS_CONDITION.
**	15-oct-1997 (canor01)
**	    Remove comment delimiters from inside comment blocks.
**	03-nov-1997 (canor01)
**	    Add stack address parameter to CS_create_thread.
**	16-feb-1998 (toumi01)
**	    Add Linux (lnx_us5).
**	09-Mar-1998 (jenjo02)
**	    Moved CS_MAXPRIORITY, CS_MINPRIORITY, CS_DEFPRIORITY here
**	    from csinternal.h
**	20-Mar-1998 (jenjo02)
**	    Removed cs_access_sem for OS_THREADS. cs_event_sem alone does the
**	    job nicely, protecting changes to cs_mask and cs_state.
**      27-Mar-1998 (muhpa01)
**          For hpb_us5, use version of pthread_mutex_init with NULL param.
**          Also, use sched_yield instead of pthread_yield.
**	31-mar-1998 (canor01)
**	    Add CS_get_stackinfo() macro.
**    20-Apr-1998 (merja01)
**            Remove prior axp_osf definition for __pthread_self.  This 
**            is not needed on 4.0D.
**	10-jun-1998 (walro03 for fanra01 and canor01)
**		Define CS_THREAD_ID, CS_get_thread, CS_thread_equal, and
**		CS_thread_id_assign for Ingres-threaded platforms.
**		Update copyright info.
**	30-jun-1998 (hanch04) 
	    For mckba01 added cs_diag_qry.
**	    As part of the VMS port of ICL diags, Added new field in 
**	    CS_SCB on VMS, have to replicate here as genric code
**	    updates this field.
**	02-jul-1998 (ocoro02)
**	    Added cs_diag_qry to CS_SCB structure.
**	04-Jul-1998 (allmi01)
**	    Added entries forSilicon Graphics (sgi_us5).
**	06-Jul-1998 (fucch01)
**	    Added sgi_us5 to platforms using NULL as second parameter
**	    for pthread_mutex_init instead of pthread_mutexattr_default.
**    10-Jul-1998 (schte01)
**            Changed stkaddr parm on CS_get_stackinfo to correct 
**            compiler error. Added tests for stkaddr before doing
**            pthread_attr_setstackaddr.
**	28-aug-98 (stephenb)
**	    Add defines for thread specific statistics calls. Right now we 
**	    use getrusage() which gives the wrong result for OS threaded
**	    versions of the DBMS, since it returns information for the process
**	    as a whole rather than the thread.
**	17-sep-1998 (canor01)
**	    Add CS_AINCR datatype, an integer datatype that can be atomically
**	    incremented and decremented.
**	22-sep-1998 (walro03)
**	    Added rs4_us5 to platforms using NULL as second parameter
**	    for pthread_mutex_init instead of pthread_mutexattr_default.
**	05-Oct-1998 (jenjo02)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of waits.
**	16-Nov-1998 (jenjo02)
**	    Completed defines of RWLOCK functions.
**	    Added CS_SM_WAKEUP_CB, CS_CPID structures for implementation 
**	    of cross-process resumes using CS_COND instead of IdleThread.
**	    Added new defines for thread suspends to distinguish Log I/O
**	    from other Disk I/O, I/O reads from writes.
**	    Added new stats to tally I/O reads and writes separately.
**	02-Dec-1998 (muhpa01)
**		Removed POSIX_DRAFT_4 code for HP - obsolete.
**      18-jan-1999 (matbe01)
**          Define CS_ASET as i2 rather than char for NCR (nc4_us5).
**    21-Jan-1999 (schte01)
**       For SIMULATE_PROCESS_SHARED don't define CS_CP_COND as pthread_cont_t.
**       It has already been defined as nat.
**    27-Jan-1999 (schte01)
**       Remove #undef USE_IDLE_THREAD.
**	10-Mar-1999 (shero03)
**	    Added cs_random_seed.
**	04-feb-1999 (muhpa01)
**	    Added CS_cp_* definitions for POSIX threads.  Also, for hpb_us5,
**	    added cs_sp definition, and removed CS_get_stackinfo() macro thru
**	    use of xCL_NO_STACK_CACHING - can't get what we want for pthreads.
**	07-Feb-1999 (kosma01)
**	    IRIX 6.5 requires UNBOUND Threads. BOUND Threads are real_time
**	    threads, would need to run as root.
**	14-apr-1999 (devjo01)
**	    - Prefixed autovars generated by CS_get_stackinfo() to
**	    prevent name collision. ('status' was being declared locally,
**	    AND as a parameter when macro was invoked.)
**	    - Remove unneeded '&' operators in POSIX CS_get_stackinfo() macro.
**	    - DEC UNIX 4.0b, does not provide pthread_attr_getstackaddr(),
**          so CS_get_stackinfo() should not be used with axp_osf && 2.0.
**	    Fortunately macro is not needed for that release/OS.
**	19-apr-1999 (hanch04)
**	    CS_SID should always be a SCALARP
**	10-may-1999 (walro03)
**	    Remove obsolete version string aco_u43, arx_us5, bu2_us5, bu3_us5,
**	    bul_us5, cvx_u42, dr3_us5, dr4_us5, dr5_us5, dr6_ues, dr6_uv1,
**	    dra_us5, ib1_us5, odt_us5, ps2_us5, pyr_u42, rtp_us5, sqs_u42,
**	    sqs_us5, vax_ulx, x3bx_us5.
**	20-May-1999 (jenjo02)
**	    Added cs_ditgiop for GatherWrite.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**    03-jul-1999 (devjo01)
**          Added CS_DIZZY_MASK bit to mark sessions which have spun on
**          a cross-process mutex for long enough that a cross process
**          session scheduling/mutex deadlock is suspected.  Bit 0x00020000
**          selected since bit 0x00000020 have been glommed by Ingres 3.0.
**	29-Sep-1999 (ahaal01)
**	    Changed definition of CS_yield_thread from pthread_yield to
**	    sleep(0) on (rs4_us5) for compatability between AIX 4.2 and 4.3.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      08-Dec-1999 (podni01)  
**          Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**          with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**          this code for some reason removed all over the place.
**      13-Dec-1999 (hweho01)
**          Added support for AIX 64-bit platform (ris_u64).
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**          Correct rux_us5 changes that were applied using the rmx_us5
**          label.
**      03-feb-2000 (mosjo01) SIR 97564
**          using sleep(0) instead of sched_yield or pthread_yield
**          because it works on any version of AIX 4.1, 4.2 or 4.3.
**      25-Sep-2000 (hanal04) Bug 102725 INGSRV 1275.
**          Introduced the MEbrk_mutex. This mutex should be acquired
**          whenever we could change the sbrk() value. 
**          CS_create_thread has been modified to acquire this mutex 
**          in order to prevent concurrent conflicts with ME_alloc_brk().
**	29-feb-2000 (toumi01)
**	    Added support for int_lnx (OS threaded build).
**	15-aug-2000 (somsa01)
**	    Added support for ibm_lnx (OS threaded build).
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Sep-2000 (hanje04)
**	    Added support from Alpha Linux (axp_lnx). Modeled on axp_osf
**	    not using OS Threading
**	29-Sep-2000 (jenjo02)
**	    In CS_SCB, deleted cs_dio, cs_bio, which were simply
**	    the sum of cs_dior+cs_diow, cs_bior+biow. Added
**	    MO methods to compute these objects instead.
**	04-Oct-2000 (jenjo02)
**	    Deleted CSp|v_semaphore macros, which were dubious to
**	    begin with and ifdef'd out a long time ago.
**      31-Oct-2000 (hweho01)
**          Redefined the machine specific section of CS_SCB 
**          structure for AIX 64-bit (ris_u64) platform.
**      02-Feb-2001 (linke01)
**          defined CS_get_stackinfo() for usl_us5
**	19-jul-2001 (stephenb)
**	    Add support for i64_aix
**	16-aug-2001 (toumi01)
**	    Add CS_tas support for i64_aix
**	04-Dec-2001 (hanje04)
**	    Added support for IA64 Linux
**	24-jan-2002 (devjo01)
**	    Add CS_cond_signal_from_AST macro.
**      21-Mar-2002 (xu$we01)
**          Provided proper definition for S_yield_thread() and
**          OS_THREADS_USED on dgi_us5.
**	19-apr-2002 (xu$we01)
**          Modified conditions for including cs_sem_list in CS_SEMAPHORE.
**	    Deleted the definition for S_yield_thread() and re-defined
**	    OS_THREADS_USED on dgi_us5 for solving the compilation errors. 
**	06-sep-2002 (somsa01)
**	    Changed type of cs_stktop and cs_stkbottom to SIZE_TYPE on HP.
**	16-sep-2002 (somsa01)
**	    Changed type of cs_jmp_buf to SIZE_TYPE on HP.
**	19-sep-2002 (somsa01)
**	    HP-UX also has _lwp_info(), so let's use it.
**      23-Sep-2002 (hweho01)
**          Added changes for hybrid build on AIX platform.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate
**	01-nov-2002 (somsa01)
**	    On HP-UX, having a thread use the default stack size when
**	    created triggers stack caching. The thread and its stack
**	    are cached at thread termination; it is then used by another
**	    thread which is to be created also with a default stack size.
**	    Thus, added CS_set_default_stack_size() which will reset the
**	    default thread stack size for the current process to the
**	    maximum of the value passed in and CS_THREAD_MIN_STACK on
**	    HP-UX. Also, modified CS_create_thread() to not set the stack
**	    size per thread if stack caching is enabled. For SYS_V_THREADS
**	    and for other platforms, CS_set_default_stack_size() just resets
**	    the value passed in to the max of what is passed in and
**	    CS_THREAD_MIN_STACK.
**	03-mar-2003 (devjo01) b109753
**	    Remove unneeded CS_CONDITION field 'cnd_mutex_sem'.
**	21-May-2003 (bonro01)
**	    Added support for HP Itanium (i64_hpu)
**	17-Nov-2003 (devjo01)
**	    Define the new Compare & Swap Routines.
**	26-Sep-2003 (hanje04)
**	    Redefined CS_create_thread and CS_get_stackinfo for Linux to 
**	    use pthread_attr_setstack and pthread_attr_getstack instead of
**	    pthread_attr_setstackaddr and pthread_attr_getstackaddr because 
**	    the latter function are deprecated on Linux.
**	25-jan-2004 (devjo01)
**	    Make C&S definitions conditional, so we don't need to immediately
**	    implement them for platforms which do not require non-blocking
**	    atomic updates. (e.g. non cluster builds.)
**	06-feb-2004 (hayke02)
**	    Remove use of _lwp_info()/CS_thread_info/CS_THREAD_CPU_MS() for
**	    the HP 64 bit 'add on' (hp2_us5) executables. _lwp_info fails to
**	    update the second values (lwp_utime.tv_sec and lwp_stime.tv_sec)
**	    leading to a hang in OPF due to the elapsed time never exceeding
**	    the cost of the best plan so far, and no timeout. This change
**	    fixes bug 111674, problem INGSRV 2679.
**	05-mar-2004 (somsa01)
**	    Cleaned up i64_lnx port.
**	12-apr-2004 (somsa01)
**	    Updated cs_length to be a SIZE_TYPE in CS_SCB, to correspond
**	    to the changes made to SC0M_OBJECT.
**	10-jun-2004 (devjo01/mutma03)
**	    Add event_wait_sem to CS_SM_WAKEUP_CB.
**	7-Sep-2004 (mutma03)
**	    Added CS_AST_WAIT to indicate the thread is 
**	    waiting for AST.
**	15-Mar-2005 (bonro01)
**	    Added support for Solaris AMD64 a64_sol.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	31-May-2005 (schka24)
**	    a64_sol is solaris, can use Solaris lwp-info stuff.
**	13-Sep-2005 (schka24)
**	    Linux can set thread stack size!  What a concept!
**	    Simplify "bound thread" ifdef poop while I'm in here.
**	04-nov-2005 (abbjo03)
**	    Add VMS-specific section of the CS_SCB and make other changes
**	    needed for VMS to use csnormal.h.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	23-Mar-2007 (hajne04)
**	    SIR 117985
**	    Add ppc_lnx for PowerPC Linux support.
**	31-Jan-2007 (kiria01) b117581
**	    Change the way we define the masks and states for cs_mask and
**	    cs_state. These are now defines in a table macro that gets used
**	    to define enum constants for the values and can be used for
**	    defining the display strings that represent them.
**	13-Feb-2007 (kschendel)
**	    Added machinery needed for new CScancelCheck.
**	09-Apr-2007 (drivi01) b117581
**	    Fix change #485729, replace "GLOBALREF const" with GLOBALCONSTREF and
**	    "GLOBALDEF const" with GLOBALCONSTDEF for cs_state_names and 
**	    cs_mask_names to prevent compile problems of c++ files. 
**	02-May-2007 (jonj/joea)
**	    For axm_vms use __ATOMIC_EXCH_LONG for CS_TAS, CS_ACLR.
**	    Deleted CS_test_set, CS_clear_sem, use TAS,ACLR instead.
**	    Moved #include builtins to top of module.
**	30-May-2007 (kschendel)
**	    a64_lnx: remove long casts from bit number args to bitops macros.
**	30-may-2007 (joea)
**	    Replace __ATOMIC_EXCH_LONG by __TESTBITCCI and __TESTBITSSI
**	    in CS_ACLR and CS_TAS.
**      22-aug-07
**	    Add CS_membar() (memory barrier/fence) as a #define for __sync()
**	    on rs4_us5, and a NULL function elsewhere. This is then used in
**	    the parallel sort DMF code to prevent out-of-order execution of
**	    instructions, leading to incorrect results. This change fixes bug
**	    118591.
**	23-aug-07
**	    Amend CS_member to CS_MEMBAR_MACRO and add in i64_hpu
**      30-Aug-2007 (bonro01)
**          csnormal.h is not used by Windows so move CS_MEMBAR_MACRO to cs.h
**	05-Nov-2006 (smeke01) Bug 119411
**	    On int.lnx thread-ids are unsigned longs, so I removed i4 coercion  
**	    and changed display format from %d to %ul for this platform.
**	    Introduced a conditionally-defined format string TIDFMT to make 
**	    this easier in future. 
**	20-Apr-2008 (hanje04)
**	    Add in-line assembler functions for test_and_set()/clear()
**	    functions. These replace those defined in the now missing
**	    bitops.h
**	07-Oct-2008 (smeke01) b120907
**	    Added new define CS_get_os_tid() to handle platform-specific  
**	    calls to get OS tid.  Currently defined to the function for LNX,  
**	    su4.us5, su9.us5, rs4.us5 and r64.us5. On platforms without such  
**	    functions, dummy value is 0. 
**	27-Nov-2008 (smeke01) b121287
**	    Moved typenames for platform-specific OS thread id types to 
**	    their proper home in csnormal.h, amending them to capitals and 
**	    typedefs at the same time.
**	9-Dec-2008 (kschendel)
**	    Fix the a64-lnx CS_aclr function to have the proper type.
**	    Although C wasn't complaining, c++ was (used in the IngresStream
**	    IB interface), and there's no need for the discrepancy.
**	26-nov-2008 (joea)
**	    Add VMS to list of platforms without pthread_mutexattr_default.
**	    Add #define for cs_sp for axm_vms.
**	02-dec-2008 (joea)
**	    Use sched_yield for CS_yield_thread on VMS.
**	08-dec-2008 (joea)
**	    For VMS, define _CS_lock/unlock_mebrk as blank.
**      08-Dec-2008 (stegr01)
**          extend  ifdefs that use axm_vms to include i64_vms
**          define _CSNORMAL_H so we don't repeatedly reload this H file
**          extend _CS_SM_WAKEUP_CB structure to include ast support
**          include cs_cpu in scb for posix threads on VMS
**          use __ATOMIC_EXCH_LONG in CS_ACLR and CS_TAS instead of deprecated
**           __TESTBITCCI  __TESTBITSSI
**          new macros for CScas4 and CScasptr for IA64
**	02-jul-2009 (joea)
**	    Relocate cs_cpu in the CS_SCB to after the machine-specific section.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbols changed; use sparc / aix / hpux symbols.
**	14-July-2009 (toumi01)
**	    Le quatorze juillet change to x86 Linux threading to end the
**	    historical use of csll.lnx.asm and use minimal low level code
**	    added to csnormal.h to handle atomic functions. This avoids
**	    RELTEXT issues that are problematic for SELinux.
**	26-Aug-2009 (kschendel) b121804
**	    Some void prototypes for gcc 4.3.
**      21-jan-2010 (joea)
**          Add KP Services block pointer to CS_SCB for VMS.
*/


/*
** Ingres thread priorities 
**
** Constants that define MIN and MAX priorities of sessions, and the
** default priority used if priority 0 is given when starting a thread.
**	MIN_PRIORITY is 1 - priority 0 cannot be specified since passing
**	    a zero value to CSadd_thread indicates to use the default priority.
**	    The Idle Job is the only thread that will run at priority zero.
**	MAX_PRIORITY is two less than CS_LIM_PRIORITY to ensure that no
**	    sessions run at higher priority than the admin job (which runs
**	    at CS_LIM_PRIORITY - 1).
**	CUR_PRIORITY means the priority of the current thread
*/
#define 		CS_LIM_PRIORITY 16
#define			CS_MINPRIORITY	1
#define			CS_MAXPRIORITY	(CS_LIM_PRIORITY - 2)
#define			CS_DEFPRIORITY	(CS_LIM_PRIORITY / 2)
#define			CS_CURPRIORITY	-1

# ifdef OS_THREADS_USED
/*
** Operating-system thread primitives
*/

#if defined(LNX)
typedef	pid_t	CS_OS_TID;
#elif defined(sparc_sol)
typedef	lwpid_t	CS_OS_TID;
#elif defined(any_aix)
typedef	tid_t	CS_OS_TID;
#else
typedef	int	CS_OS_TID;	/* safe dummy type for all the rest */
#endif

#if defined(LNX) || defined(sparc_sol) || defined(a64_sol) || defined(any_aix)
typedef	pid_t	CS_OS_PID;
#else
typedef	int	CS_OS_PID;	/* safe dummy type for all the rest */
#endif

/*
** Minimum OS thread stack size
*/
#define			CS_THREAD_MIN_STACK	262144
 
/*}
** Name: CS_RWLOCK - Multiple Readers/Single Writer lock
**
** Description:
**      This structure may be used as the underlying object that will
**      be manipulated by CSp_semaphore() and CSv_semaphore() when
**      operating system semaphores are employed.
**
**      It may be a structure defined by the operating system, or
**      a combination of OS mutexes and flags that can be accessed
**      in a readers/writer fashion by CS code.
**
**      The Ingres CSx_semaphore() routines may also be implemented
**      using OS mutexes (CS_SYNCH) only.
**
*/
# ifdef SYS_V_THREADS
#    define     CS_RWLOCK               rwlock_t        /* rwlock type */
/* all functions are passed a (CS_RWLOCK *) and return OK on success */
#    define     CS_rwlock_init(rwptr,st) *st=rwlock_init(r
#    define     CS_cp_rwlock_init(rwptr,st) *st=rwlock_init(rwptr,USYNC_PROCESS,NULL)
#    define     CS_rwlock_destroy(rwptr) rwlock_destroy(rwptr)
#    define     CS_rw_wrlock(rwptr)     rw_wrlock(rwptr)
#    define     CS_rw_rdlock(rwptr)     rw_rdlock(rwptr)
#    define     CS_rw_unlock(rwptr)     rw_unlock(rwptr)
# endif /* SYS_V_THREADS */

/*}
** Name: CS_SYNCH - Simplest synchronization object
**
** Description:
**      When operating system semaphores are used, this is the simplest
**      form of synchronization object.  It may be defined by the
**      operating system, or it may be a CS_ASET that is manipulated
**      as a synchronization object across concurrent threads by CS code.
*/
# ifdef SYS_V_THREADS
#    define     CS_SYNCH                mutex_t
#    define     CS_synch_init(synchptr) mutex_init(synchptr,NULL,NULL)
#    define     CS_synch_destroy(sptr)  mutex_destroy(sptr)
#    define     CS_synch_lock(sptr)     mutex_lock(sptr)
#    define     CS_synch_trylock(sptr)  mutex_trylock(sptr)
#    define     CS_synch_unlock(sptr)   mutex_unlock(sptr)
 
/* Cross-process synch objects */
#    define     CS_CP_SYNCH             CS_SYNCH
#    define     CS_cp_synch_init(sp,st) *st=mutex_init(sp,USYNC_PROCESS,NULL)
#    define     CS_cp_synch_destroy     CS_synch_destroy
#    define     CS_cp_synch_lock        CS_synch_lock
#    define     CS_cp_synch_unlock      CS_synch_unlock
# endif /* SYS_V_THREADS */
 
# ifdef POSIX_THREADS
#    define     CS_SYNCH                pthread_mutex_t
#if defined(sparc_sol) || defined(axp_osf) || defined(dg8_us5) \
 || defined(dgi_us5) || defined(sgi_us5) || defined(any_hpux) \
 || defined(any_aix) || defined(LNX) \
 || defined(a64_sol) || defined(OSX) || defined(VMS)
#    define     CS_synch_init(synchptr) pthread_mutex_init(synchptr,NULL)
#else
#if defined(DCE_THREADS)
#    define     CS_synch_init(synchptr) pthread_mutex_init(synchptr,\
                                                pthread_mutexattr_default)
#else
#    define     CS_synch_init(synchptr) pthread_mutex_init(synchptr,&pthread_mutexattr_default)
#endif /* DCE_THREADS */
#endif /* sol axp_osf dg8_us5 dgi_us5 sgi_us5 hpux aix Linux */
#    define     CS_synch_destroy(sptr)  pthread_mutex_destroy(sptr)
#    define     CS_synch_lock(sptr)     pthread_mutex_lock(sptr)
#    define     CS_synch_trylock(sptr)  pthread_mutex_trylock(sptr)
#    define     CS_synch_unlock(sptr)   pthread_mutex_unlock(sptr)

/* Cross-process synch objects */
#    define     CS_CP_SYNCH             CS_SYNCH
# ifndef SIMULATE_PROCESS_SHARED
#    define     CS_cp_synch_init(sp,stat)       { pthread_mutexattr_t mattr;\
                                          pthread_mutexattr_init(&mattr);\
                                          pthread_mutexattr_setpshared(&mattr,\
                                                PTHREAD_PROCESS_SHARED);\
                                          *stat=pthread_mutex_init(sp,&mattr);\
                                          pthread_mutexattr_destroy(&mattr);}

#    define     CS_cp_synch_destroy     CS_synch_destroy
#    define     CS_cp_synch_lock        CS_synch_lock
#    define     CS_cp_synch_unlock      CS_synch_unlock
# else /* SIMULATE_PROCESS_SHARED */
#    define     CS_cp_synch_init(sp,stat)  {CS_ACLR(sp) ; *stat = OK; }
#    define     CS_cp_synch_destroy( sp )  CS_ACLR( (sp) )
#    define     CS_cp_synch_unlock( sp )  CS_ISSET( (sp) ) \
                 ? (CS_ACLR( (sp) ), OK) : EPERM
#    define     CS_cp_synch_trylock( sp ) CS_TAS( (sp) ) \
                 ? OK : EBUSY  
# endif /* SIMULATE_PROCESS_SHARED */
# endif /* POSIX_THREADS */


/*}
** Name: CS_COND - Simplest condition object
**
** Description:
**      When operating system semaphores are used, this is the simplest
**      condition object that threads can wait on.
*/
# ifdef SYS_V_THREADS
 
/* initialize a same-process condition */
#    define     CS_COND                 cond_t
#    define     CS_cond_init(condptr)   cond_init(condptr,NULL,NULL)
#    define     CS_cond_destroy(cp)     cond_destroy(cp)
 
/* CS_cond_wait( CS_COND *cp, CS_SYNCH *sp ) */
#    define     CS_cond_wait(cp,sp)     cond_wait(cp,sp)
 
/* CS_cond_timedwait( CS_COND *cp, CS_SYNCH *sp, CS_COND_TIMESTRUCT *tsp ) */
#    define     CS_cond_timedwait(cp,sp,tsp)    cond_timedwait(cp,sp,tsp)
 
/* CS_COND_TIMESTRUCT is used for CS_cond_timedwait */
#    define     CS_COND_TIMESTRUCT      timestruc_t
#    define     CS_cond_signal(cp)      cond_signal(cp)
#    define     CS_cond_signal_from_AST(cp) cond_signal(cp)
#    define     CS_cond_broadcast(cp)   cond_broadcast(cp)
 
/* initialize a cross-process condition */
#    define     CS_CP_COND              CS_COND
#    define     CS_cp_cond_init(cptr,stat)      *stat=cond_init(cptr,USYNC_PROCESS,NULL)
#    define     CS_cp_cond_destroy      CS_cond_destroy
#    define     CS_cp_cond_signal       CS_cond_signal
#    define     CS_cp_cond_broadcast    CS_cond_broadcast
#    define     CS_cp_cond_wait         CS_cond_wait
#    define     CS_cp_cond_timedwait    CS_cond_timedwait
# endif /* SYS_V_THREADS */

# ifdef POSIX_THREADS
#    define     CS_COND                 pthread_cond_t
# ifdef SIMULATE_PROCESS_SHARED
     typedef i4  CS_CP_COND;
#else
#    define     CS_CP_COND              CS_COND
# endif /* SIMULATE_PROCESS_SHARED */
/* initialize a same-process condition */
#if defined(DCE_THREADS)
# define CS_cond_init(condptr) pthread_cond_init(condptr,pthread_condattr_default)
#else
#    define     CS_cond_init(condptr)   pthread_cond_init(condptr,NULL)
#endif /* DCE_THREADS */
# ifndef SIMULATE_PROCESS_SHARED
/* initialize a cross-process condition */
#    define     CS_cp_cond_init(cptr,stat)      { pthread_condattr_t cattr;\
                                          pthread_condattr_init(&cattr);\
                                          pthread_condattr_setpshared(&cattr,\
                                                PTHREAD_PROCESS_SHARED);\
                                          *stat=pthread_cond_init(cptr,&cattr);\
                                          pthread_condattr_destroy(&cattr);\
}
#    define     CS_cp_cond_signal       pthread_cond_signal
#    define     CS_cp_cond_destroy      pthread_cond_destroy
#    define     CS_cp_cond_broadcast    pthread_cond_broadcast
#    define     CS_cp_cond_wait         pthread_cond_wait
#    define     CS_cp_cond_timedwait    pthread_cond_timedwait
# else  /* SIMULATE_PROCESS_SHARED */
#    define     CS_cp_cond_init(cptr,stat) {*cptr = 0, *stat = OK;}
#    define     CS_cp_cond_destroy(cptr)   *cptr = 0
# endif
#    define     CS_cond_destroy         pthread_cond_destroy
 
/* CS_cond_wait( CS_COND *cp, CS_SYNCH *sp ) */
# if defined(DCE_THREADS)
#    define     CS_cond_wait(cp,sp)  pthread_cond_wait(cp, sp) == 0 ? OK : errno
# else
#    define     CS_cond_wait            pthread_cond_wait
# endif /* DCE_THREADS */
#    define     CS_cond_wait            pthread_cond_wait
/* CS_cond_timedwait( CS_COND *cp, CS_SYNCH *sp, CS_COND_TIMESTRUCT *tsp ) */
#    define     CS_cond_timedwait       pthread_cond_timedwait
 
/* CS_COND_TIMESTRUCT is used for CS_cond_timedwait */
#    define     CS_COND_TIMESTRUCT      struct timespec
#    define     CS_cond_signal          pthread_cond_signal
# ifdef axp_osf
#    define     CS_cond_signal_from_AST pthread_cond_signal_int_np
# else
#    define     CS_cond_signal_from_AST pthread_cond_signal
# endif /*axp_osf*/
#    define     CS_cond_broadcast       pthread_cond_broadcast
# endif /* POSIX_THREADS */

/*
** Functions to create, destroy, signal and join
** operating-system threads
*/
/* thread creation flags */
# define CS_DETACHED    0    /* releases resources on exit */
# define CS_JOINABLE    1    /* must be waited for with CS_join_thread() */
 
/* bound or unbound threads? */
#if defined(sgi_us5)
#  if defined(SYS_V_THREADS)
	Error! SGI unbound threads are not SYSV threads.
#  endif
#  define SGI_UNBOUND_THREADS	/* Dunno what this does any more...
				** it's only used in this include file
				*/
#endif

/* Decide whether to lock the "sbrk" lock when creating threads.
** Platforms that track pages by hand in ME need to take the lock,
** since creating threads can change the break.  Platforms that use
** a malloc-equivalent as a high level allocator can assume that
** the malloc-equiv is thread safe, and don't need to bother.
*/
#if defined(xCL_SYS_PAGE_MAP) || !defined(OS_THREADS_USED) || defined(VMS)
#  define _CS_lock_mebrk 
#  define _CS_unlock_mebrk
#else
GLOBALREF CS_SYNCH        MEbrk_mutex;      /* Stop the brk from changing */
#  define _CS_lock_mebrk CS_synch_lock(&MEbrk_mutex);
#  define _CS_unlock_mebrk CS_synch_unlock(&MEbrk_mutex);
#endif
 
# ifdef SYS_V_THREADS
 
# include <sys/priocntl.h>
# include <sys/tspriocntl.h>
 
/* unique thread identifier */
#   define CS_THREAD_ID         thread_t

/* 
** The default STprintf format for printing thread-ids is currently %d. 
** See note under POSIX_THREADS for an example of setting up TIDFMT for 
** printing threads that are not signed integers.
*/
#define TIDFMT "%d"

#   define CS_get_thread_id()   thr_self()
 
#   define CS_thread_id_equal(a,b)      (a == b)
#   define CS_thread_id_assign(a,b)     a = b
#   define CS_set_default_stack_size(stksize, stkcache) \
	{ \
	    stksize = \
	      (stksize > CS_THREAD_MIN_STACK) ? stksize : CS_THREAD_MIN_STACK;\
	}
 
/*
** Create a thread:
** CS_create_thread( size_t stksize,            // size of stack for thread
**		     void *stkaddr,		// address of stack
**                   void *(*startfn)(void *),  // start function
**                   void *args,                // args (in form of pointer)
**                   CS_THREAD_ID *tidptr,      // return thread id here
**                   long flag,                 // CS_JOINABLE: can use
**                                                      CS_join_thread() to
**                                                      get return value.
**                                                 CS_DETACHED: will release
**                                                      all resources and
**                                                      return value on
**                                                      termination
**                   STATUS *status )           // return value
**  returns: OK on success */
#   define CS_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
		_CS_lock_mebrk \
                *statp = thr_create(stkaddr,stksize,(void*(*)())startfn,\
                           (void*)args,\
                           THR_BOUND|((flag)==CS_JOINABLE?0:THR_DETACHED),\
                           tidptr);\
		_CS_unlock_mebrk
 
/* CS_yield_thread(void) - give up time slice */
#   define CS_yield_thread()    thr_yield()
 
/* Exit a thread:
** CS_exit_thread( void *status )
**      if CS_JOINABLE, status is returned, and thread resources are
**      retained until status received by a CS_join_thread()
*/
#   define CS_exit_thread(status)       thr_exit(status)
 
/* Wait for a thread to exit:
** CS_join_thread( CS_THREAD_ID tid,            // id of thread to wait for
**                 void **status )              // receives return status
*/
#   define CS_join_thread(tid,status)   thr_join(tid,NULL,status)
 
 
/* Set priority for a thread:
** CS_thread_setprio( CS_THREAD_ID tid,         // id of thread
**                    int priority,             // priority
**                    STATUS *status )          // return value
*/
/* Ingres threads have priorities up to CS_LIM_PRIORITY */
/* SYS_V threads have priorities 0-127 */
#   define MAX_SYS_V_THR_PRIO 127

/* Solaris priocntl() sets user thread priority from -20 to 0 */
#   define CS_thread_setprio(tid,priority,statp) \
                        { pcparms_t pcparms;\
                          tsparms_t *tsparms = (tsparms_t*)&pcparms.pc_clparms;\
                          pcparms.pc_cid = PC_CLNULL;\
                          priocntl(P_LWPID,P_MYID,PC_GETPARMS,\
                                   (caddr_t)&pcparms);\
                          tsparms->ts_upri = \
                            (((priority*20)/CS_LIM_PRIORITY)-20);\
                          *statp = priocntl(P_LWPID,P_MYID,PC_SETPARMS,\
                                   (caddr_t)&pcparms);\
                        }
 
/* Send a signal to a specific thread in the same process:
** CS_thread_kill( CS_THREAD_ID tid,            // id of thread
**                 int sig )                    // signal number to send
*/
#   define CS_thread_kill(tid,sig)      thr_kill(tid,sig)
 
/* Set signal handling mask for the current thread only:
** CS_thread_sigmask( int act,                  // SIG_BLOCK, SIG_UNBLOCK,
**                                                 or SIG_SETMASK
**                    sigset_t *set,            // new signal mask
**                    sigset_t *oset )          // receive old signal mask
**                                                 if not NULL
*/
#   define CS_thread_sigmask(act,set,oset)      thr_sigsetmask(act,set,oset)

/* Get information about the thread's stack
** void
** CS_get_stackinfo( char    **stackaddr,	// stack address
**                   i4 *stacksize,        // stack size
**                   STATUS  *status )          // status
*/
#ifdef usl_us5
#   define CS_get_stackinfo(sa,ss,st) { *st=FAIL; }
# else                     
#   define CS_get_stackinfo(sa,ss,st) \
	    {   stack_t csgsi_stt;\
		if(thr_stksegment(&csgsi_stt)) *st=FAIL;\
		else\
		{\
		    *sa = (char *)csgsi_stt.ss_sp;\
		    *ss = (i4)csgsi_stt.ss_size;\
		    *st = OK;\
		}\
	    }
# endif /* Unixware */  
# endif /* SYS_V_THREADS */
 
# ifdef POSIX_THREADS
#   define MAX_POSIX_THR_PRIO 127
#   define CS_THREAD_ID         pthread_t

/* 
** The default STprintf format for printing thread-ids is currently %d. 
** This is wrong on int.lnx platforms where thread-ids are unsigned long. As 
** and when other POSIX_THREADS platforms are found to be unsigned long, add 
** them here.
*/
#if defined(int_lnx)
#define TIDFMT "%lu"
#else
#define TIDFMT "%d"
#endif /* int_lnx */

#   define CS_get_thread_id     pthread_self
#   define CS_thread_id_equal(a,b)      pthread_equal(a,b)
#   define CS_thread_id_assign(a,b)     a = b
# if defined(any_hpux)
#   define CS_set_default_stack_size(stksize, stkcache) \
	{ \
	    stksize = \
	      (stksize > CS_THREAD_MIN_STACK) ? stksize : CS_THREAD_MIN_STACK;\
	    if (stkcache == TRUE)\
		pthread_default_stacksize_np(stksize, NULL);\
	}
# else
#   define CS_set_default_stack_size(stksize, stkcache) \
	{ \
	    stksize = \
	      (stksize > CS_THREAD_MIN_STACK) ? stksize : CS_THREAD_MIN_STACK;\
	}
# endif  /* hp-ux */
#if defined(DCE_THREADS)
#   define CS_create_thread(stksize,startfn,args,tidptr,flag,statp) \
                       { pthread_attr_t attr;\
			 _CS_lock_mebrk \
			 pthread_attr_create(&attr);\
			 pthread_attr_setstacksize(&attr, stksize);\
                         if (pthread_create(tidptr,attr,\
                             (pthread_startroutine_t)startfn,args))\
                            *statp=errno;\
                         else\
                            *statp=0;\
                         pthread_attr_delete(&attr);\
			 _CS_unlock_mebrk \
                        }
#   define CS_thread_setprio(tid,priority,statp)\
                                { int prio;\
                                  if (priority < 2)\
                                      prio = PRI_OTHER_MIN;\
                                  else\
                                      prio = (priority - 1)/2 + PRI_OTHER_MIN;\
                                  if (pthread_setprio(tid,prio) == -1)\
                                      *statp=errno;\
                                  else\
                                      *statp = 0;}
#else
# if defined(SGI_UNBOUND_THREADS)
	/* SGI unbound threads -- so, what's different about 'em? */
#   define CS_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                                { pthread_attr_t attr;\
				  _CS_lock_mebrk \
                                  pthread_attr_init(&attr);\
                                  pthread_attr_setscope(&attr,\
                                        PTHREAD_SCOPE_PROCESS);\
                                  if((flag)==CS_DETACHED)\
                                    pthread_attr_setdetachstate(&attr,\
                                        PTHREAD_CREATE_DETACHED);\
				  pthread_attr_setstacksize(&attr, stksize);\
 			          if (stkaddr !=0)\
                                  pthread_attr_setstackaddr(&attr, stkaddr);\
                             *statp=pthread_create(tidptr,&attr,startfn,args);\
                                  pthread_attr_destroy(&attr);\
				  _CS_unlock_mebrk \
}
#   define CS_thread_setprio(tid,priority,statp) {*statp = 0;}
#   define CS_thread_getprio(tid,priority,statp) \
			{ struct sched_param sp; \
                          int policy; \
			 *statp ==pthread_getschedparam(tid,&policy,&sp);\
			priority = sp.sched_priority; \
}
#else
	/* Normal (bound posix) thread creation */
# if defined(any_hpux)
#   define CS_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,\
			    stkcache,statp) \
		{ \
		    pthread_attr_t attr;\
		    _CS_lock_mebrk \
		    pthread_attr_init(&attr);\
		    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);\
		    if ((flag) == CS_DETACHED)\
			pthread_attr_setdetachstate(&attr,\
						    PTHREAD_CREATE_DETACHED);\
		    if (stkcache == FALSE)\
			pthread_attr_setstacksize(&attr, stksize);\
 		    if (stkaddr != 0)\
			pthread_attr_setstackaddr(&attr, stkaddr);\
		    *statp = pthread_create(tidptr, &attr, startfn, args);\
		    pthread_attr_destroy(&attr);\
		    _CS_unlock_mebrk \
		}
# elif defined(LNX) || defined(OSX)
#   define CS_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                                { pthread_attr_t attr;\
				  _CS_lock_mebrk \
                                  pthread_attr_init(&attr);\
                                  pthread_attr_setscope(&attr,\
                                        PTHREAD_SCOPE_SYSTEM);\
                                  if((flag)==CS_DETACHED)\
                                    pthread_attr_setdetachstate(&attr,\
                                        PTHREAD_CREATE_DETACHED);\
				  pthread_attr_setstacksize(&attr, stksize);\
				  if (stkaddr !=0)\
				    pthread_attr_setstack(&attr, stkaddr, stksize);\
				*statp=pthread_create(tidptr,&attr,startfn,args);\
                                  pthread_attr_destroy(&attr);\
				  _CS_unlock_mebrk \
}
# else
#   define CS_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                                { pthread_attr_t attr;\
				  _CS_lock_mebrk \
                                  pthread_attr_init(&attr);\
                                  pthread_attr_setscope(&attr,\
                                        PTHREAD_SCOPE_SYSTEM);\
                                  if((flag)==CS_DETACHED)\
                                    pthread_attr_setdetachstate(&attr,\
                                        PTHREAD_CREATE_DETACHED);\
				  pthread_attr_setstacksize(&attr, stksize);\
 			          if (stkaddr !=0)\
                                  pthread_attr_setstackaddr(&attr, stkaddr);\
			    *statp=pthread_create(tidptr,&attr,startfn,args);\
                                  pthread_attr_destroy(&attr);\
				  _CS_unlock_mebrk \
}
# endif
#   define CS_thread_setprio(tid,priority,statp)        {*statp = 0;}
#   define CS_thread_getprio(tid,priority,statp) \
                        { struct sched_param sp; \
                          int policy; \
                         *statp ==pthread_getschedparam(tid,&policy,&sp);\
                        priority = sp.sched_priority; \
}
# endif /* SGI_UNBOUND_THREADS */
# endif /* DCE_THREADS */

# if defined(sparc_sol) || defined(any_aix)
#   define CS_yield_thread()    sleep(0)
# elif defined(axp_osf) || defined(any_hpux) || \
     defined(a64_sol) || defined(LNX) || \
     defined(OSX) || defined(VMS)
#   define CS_yield_thread()    sched_yield()
# elif defined(sgi_us5)
#   define CS_yield_thread()    sginap(0)
# else  /* axp_osf */
#   define CS_yield_thread()    pthread_yield()
# endif
#   define CS_exit_thread               pthread_exit
#   define CS_join_thread(tid,retp)     pthread_join(tid,retp)

# if defined(DCE_THREADS)
#   define CS_thread_kill(tid,sig)      raise(sig)
#   define CS_thread_sigmask    sigprocmask
#   define CS_thread_detach     pthread_detach
# else
#   define CS_thread_kill(tid,sig)      pthread_kill((pthread_t)tid,sig)
# if defined(any_aix)
#   define CS_thread_sigmask    sigthreadmask
# elif defined(dg8_us5) || defined(dgi_us5)
#   define CS_thread_sigmask    sigprocmask
# elif defined(VMS)
#   define CS_thread_sigmask(act, set, oset)
# else
#   define CS_thread_sigmask    pthread_sigmask
# endif
# endif /* DCE_THREADS */

/* Get information about the thread's stack
** void
** CS_get_stackinfo( char    **stackaddr,       // stack address
**                   i4 *stacksize,        // stack size
**                   STATUS  *status )          // status
*/

# if !defined(xCL_NO_STACK_CACHING)
# if defined(rux_us5)
#   define CS_get_stackinfo(sa,ss,st) { \
        STATUS csgsi_status=OK;\
        pthread_attr_t csgsi_attr;\
	csgsi_status=pthread_attr_create(&csgsi_attr);\
	*sa = (char *)csgsi_attr.field1;\
	if (csgsi_status==OK)\
 	*ss = pthread_attr_getstacksize(csgsi_attr);\
	if (*ss) csgsi_status=OK;\
	else     csgsi_status=errno;\
	*st = csgsi_status;\
				      }
# elif defined(LNX)
#   define CS_get_stackinfo(sa,ss,st) { \
       STATUS csgsi_status=OK;\
        pthread_attr_t csgsi_attr;\
	csgsi_status=pthread_attr_init(&csgsi_attr);\
	if(csgsi_status==OK)\
		csgsi_status=pthread_attr_getstack(\
		&csgsi_attr,(void**)sa,(size_t*)ss);\
	*st = csgsi_status;\
				       }
#else
#   define CS_get_stackinfo(sa,ss,st) { \
        STATUS csgsi_status=OK;\
        pthread_attr_t csgsi_attr;\
	csgsi_status=pthread_attr_init(&csgsi_attr);\
	if(csgsi_status==OK)\
		csgsi_status=pthread_attr_getstackaddr(\
		 &csgsi_attr,(void**)sa);\
	if(csgsi_status==OK)\
		csgsi_status=pthread_attr_getstacksize(\
		 &csgsi_attr,(size_t*)ss);\
	*st = csgsi_status;\
				       }
# endif
# endif /* xCL_NO_STACK_CACHING */
# endif /* POSIX_THREADS */

# if defined(LNX)
#define CS_get_os_tid() syscall( __NR_gettid )
#elif defined(sparc_sol)
#define CS_get_os_tid() _lwp_self()
#elif defined (any_aix)
#define CS_get_os_tid() thread_self()
#elif defined(VMS)
#define CS_get_os_tid() pthread_getselfseq_np()
#else
#define CS_get_os_tid() 0 	/* safe dummy value for all the rest */
#endif

# if defined(LNX)
#define CS_get_os_pid() getpgrp()
#elif defined(sparc_sol) || defined (any_aix)
#define CS_get_os_pid() getpid()
#else
#define CS_get_os_pid() 0 	/* safe dummy value for all the rest */
#endif

# else /* OS_THREADS_USED */
#   define CS_THREAD_ID			i4

#define TIDFMT "%d" /* used to print signed integer thread-ids */

/* unique thread identifier */
#   define CS_get_thread_id()		(1)
#   define CS_thread_id_equal(a,b)	(a == b)
#   define CS_thread_id_assign(a,b)	a = b
# endif /* OS_THREADS_USED */

#ifdef OS_THREADS_USED
/*
** Definitions for thread statistics.
**
** We need an OS function that returns statistics for a specific thread, rather
** than the process as a whole. This particular function call will only be
** used in CSMT (i.e. if the server is compiled to use OS Threads).
** On most system V platforms, there appears to be a function called _lwp_info()
** or a close equivalent [dg_lwp_info() on DG], although this function is not
** posix compliant, it will be used initialy as the base usable function for
** Solaris. The posix version, gethrvtime, is also available on Solaris but
** appears to have no equivalent on other systems.
**
** It is assumed that the _lwp_info() function, or it's equivalent returns a 
** control block defined in CS_THREAD_INFO_CB. This control block should
** contain enough information to calculate the amount of CPU time used by the
** thread in miliseconds. This is done using the CS_THREAD_CPU_MS macro.
*/
#if defined(sparc_sol) || defined(a64_sol) || defined(hpb_us5)
#    define CS_THREAD_INFO_CB		struct lwpinfo
#    define CS_thread_info		_lwp_info
#    define CS_THREAD_CPU_MS(info_cb) \
	   (((info_cb.lwp_utime.tv_sec + info_cb.lwp_stime.tv_sec) * 1000) + \
	    ((info_cb.lwp_utime.tv_nsec + info_cb.lwp_stime.tv_nsec) / 1000000))
#    define CS_THREAD_CPU_MS_USER(info_cb) \
	   (((info_cb.lwp_utime.tv_sec) * 1000) + ((info_cb.lwp_utime.tv_nsec) / 1000000))
#    define CS_THREAD_CPU_MS_SYSTEM(info_cb) \
	   (((info_cb.lwp_stime.tv_sec) * 1000) + ((info_cb.lwp_stime.tv_nsec) / 1000000))
#endif
#endif /* OS_THREADS_USED */

/*
** Global type to hold session ids, 
** Without OS_THREADS_DEFINED, or with POSIX_THREADS:
**          size == sizeof(PTR) == SCALARP (see compat.h) .
** However, with SYS_V_THREADS, it is a long
** With the following definition, there will only be a problem
** if SYS_V_THREADS is defined and sizeof(PTR) > sizeof(CS_THREAD_ID)
** (at the moment, only Solaris uses SYS_V_THREADS)
*/
#if defined(VMS)
#define CS_SID	    long
#else
#define CS_SID      SCALARP
#endif

/*
** Name: CS_SM_WAKEUP_CB	- A "wakeup block" for cross-process resume
**
** Description:
**	This block holds information about a thread which needs to be woken
**	up.
**
** History:
**	3-jul-1992 (bryanp)
**	    created.
**	08-sep-93 (swm)
**	    Changed sid type from i4 to CS_SID to match recent CL
**	    interface revision.
**	15-Oct-1998 (jenjo02)
**	    Moved here from cssminfo.h, added event_sem, event_cond
**	    event_state for OS_THREADS.
**	13-Feb-2007 (kiria01) b117581
**	    Give event_state its own set of mask values as these were
**	    being confusingly borrowed from cs_mask and cs_state.
**	    NOTE: Despite its name, event_state is a plurality of
**	    boolean states rather than one multi-valued state like
**	    for instance cs_state.
*/

struct _CS_SM_WAKEUP_CB
{
    i4		inuse;			/* is this block in use? */
    i4	pid;			/* process which owns this cb */
    CS_SID	sid;			/* session which owns this cb */
#ifdef OS_THREADS_USED
    CS_SYNCH	event_sem;		/* for event signalling and changes to  */
					/* cs_state, cs_mask.			*/
    CS_COND	event_cond;		/* for event signalling			*/
    i4		event_state;		/* state of the event			*/
#define EVENT_STATE_MASKS /* enum event_state_masks */\
_DEFINE(		EV_WAIT_MASK,       0x0001 )\
_DEFINE(		EV_AST_WAIT_MASK,   0x0002 )\
_DEFINE(		EV_EDONE_MASK,      0x0004 )\
_DEFINESEND

#ifdef VMS
    i4          seqno;                  /* establishing thread's seqno */
    i4          astqueue[2];            /* self rel qhdr must be quadword aligned !!!*/
    volatile i4 ast_pending;            /* set if AST pending */
    volatile i4 resume_pending;         /* set if resume pending */
    volatile i4 ast_in_progress;        /* set if AST in progress */
#endif /* VMS */
#endif /* OS_THREADS_USED */
 
#if defined(xCL_SUSPEND_USING_SEM_OPS)
     sem_t        event_wait_sem;
#endif
}; 
typedef struct _CS_SM_WAKEUP_CB  CS_SM_WAKEUP_CB;

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
**	04-Apr-2008 (jonj)
**	    Add IOSB, data for VMS CScp_resume.
*/
struct _CS_CPID
{
    i4		pid;		/* suspended thread's process */
    CS_SID	sid;		/* suspended thread's sid */
#if defined(VMS)
    IOSB	iosb;		/* Thread-specific IOSB */
    PTR		data;		/* Used by VMS write-completion AST */
#endif
#ifdef OS_THREADS_USED
    i4		wakeup;		/* offset to suspended thread's 
					** CS_SM_WAKEUP_CB */
#endif /* OS_THREADS_USED */
}; 
typedef struct _CS_CPID  CS_CPID;
/*{
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
** History:
**      25-Jul-90 (anton)
**          more CL committee changes
**      20-Jun-90 (anton)
**          CL committee changes
**      11-Apr-90 (anton)
**          Created.
**	XX-XXX-92 (daveb)
**	    Add cnd_spblk for MO registry.
**	03-Dec-2003 (jenjo02)
**	    Added cnd_mtx pointer to external semaphore
**	    for OS_THREADS. This is needed by CSMTattn,
**	    CSMTremove when aborting a session in
**	    CS_CNDWAIT state.
**	    Removed all of the other OS_THREADS_USED junk
**	    to simplify and eliminate persistent race
**	    conditions.
**	    Also remove conditions from MO.
*/
struct _CS_CONDITION
{
    CS_CONDITION	*cnd_next, *cnd_prev;	/* queue up waiter here */
    CS_SCB		*cnd_waiter;	/* point back to waiting session */
    char		*cnd_name;	/* point to name string */
# ifdef OS_THREADS_USED
    CS_SEMAPHORE 	*cnd_mtx;	/* External semaphore supplied
					** during CScnd_wait() */
# endif /* OS_THREADS_USED */
};

/*}
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
** History:
**      24-Oct-1986 (fred)
**          Initial VMS Definition.
**	10-Jul-1988 (rogerk)
**	    Added cs_thread_type field.
**	21-oct-1988 (rogerk)
**	    Added CS_CPU_MASK to specify that cpu statistics should be
**	    kept for the session.
**	 7-nov-1988 (rogerk)
**	    Added fields - cs_inkernel, cs_async, cs_asmask, cs_asunmask and
**	    cs_as_q - to enable CS to do most work on global CS data structures
**	    without requiring that such routines be atomic.
**      28-feb-1989 (rogerk)
**          Added cs_pid field for use by cross-process semaphores.
**	15-apr-89 (arana)
**	    Since compat.h doesn't include sys/types.h anymore, include
**	    systypes.h in here for the pyramid specific stack context data
**	    structure from sys/context.h.
**	24-jul-89 (rogerk)
**	    Added cs_ef_mask - if this is set then session will be resumed
**	    when an event matching the mask is signaled.
**    12-Feb-92 (daveb)
**        Add cs_clieint_type field to allow clients to type-tag their
**        scb's that may have a CS_SCB as a common initial member.
**        Part of fix for bug 38056.  
**	06-nov-92 (mikem)
**	  su4_us5 port.  Defined machine specific stack stuff for su4_us5 port.
**	  Followed ifdef's for su4_u42.
**      31-aug-93 (smc)
**        Created CS_SID type for holding session ids, int by default but
**	  long on axp_osf, and changed cs_self to be of type CS_SID; all in
**	  accordance with revised CL interface specification.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**      11-feb-94 (ajc)
**              Added hp8_bls specific entries based on hp8*
**	07-Dec-1995 (jenjo02)
**	    Added new cs_mode, CS_MUTEX_MASK, which is set when a
**	    session is computable but spinning trying to acquire
**	    a cross-process semaphore.
**	02-may-1997 (canor01)
**	    Add cs_cs_mask to CS_SCB for session flags specific to CS.
**	30-sep-1997 (walro03)
**		Earlier change removed definition of CS_SID for ports that
**		don't use OS threads.  Put it back to SCALARP.
**	20-Mar-1998 (jenjo02)
**	    Removed cs_access_sem for OS_THREADS. cs_event_sem alone does the
**	    job nicely, protecting changes to cs_mask and cs_state.
**   28-Jul-1998 (linke01)
**        removed the declaration of CS_ACLR for sui_us5. avoid redeclaration.
**	05-Oct-1998 (jenjo02)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of waits.
**	29-Sep-2000 (jenjo02)
**	    In CS_SCB, deleted cs_dio, cs_bio, which were simply
**	    the sum of cs_dior+cs_diow, cs_bior+biow. Added
**	    MO methods to compute these objects instead.
**    18-Jul-2005 (hanje04)
**        BUG 114860:ISS 1475660
**        In CS_update_cpu, if we're running MT on Linux (and therefore
**        1:1 threading model) Cs_srv_block.cs_cpu has to be a sum of the
**        CPU usage for each thread. Added cs_cpu to CS_SCB to track
**        thread CPU usage so that Cs_srv_block.cs_cpu can be correctly
**        maintained.
**	04-nov-2005 (abbjo03)
**	  Add VMS-specific section.
**	13-Feb-2007 (kschendel)
**	    Added last-cancel-check value for platforms that use it.
**	07-Jun-2007
**	    Bug 118465
**	    CS_ASET is incorrectly defaulting to "char" on int_lnx which
**	    seems to be causing some problems with CS_relspin() where it
**	    can return without releasing the lock. Define CS_ASET to be
**	    "volatile long" along with the other linuxes as, headers
**	    and other documentation state that to be the correct type.
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	    Remove all OSX assembler and implement OSSpinlock calls instead
**	07-Oct-2008 (smeke01) b120907
**	    Added new field to handle platform-specific storage of
**	    OS thread id (as opposed to thread library thread id). 
**
*/

# if defined(any_hpux)
# include <setjmp.h>
# endif


struct _CS_SCB
{
    CS_SCB          *cs_next;           /* forward queue link */
    CS_SCB          *cs_prev;           /* Backward  "     "  */
    SIZE_TYPE       cs_length;          /* Length, including portion not seen */
    i2              cs_type;            /* control block type */
#define                 CS_SCB_TYPE ((i2)0xABCD) /* make it obvious in dumps */
    i2              cs_s_reserved;      /* used by mem mgmt perhaps */
    PTR		    cs_l_reserved;
    PTR             cs_owner;           /* owner of block */
#define                 CS_IDENT	0xAB0000BA
    i4              cs_tag;             /* to look for in a dump (ick) */
#define                 CS_TAG          CV_C_CONST_MACRO('c','s','c','b')
    CS_SID	    cs_self;		/* session id for this session */
#ifdef SCALARP_IS_LONG
#define			CS_DONT_CARE	    (CS_SID)0x0L
#define			CS_ADDER_ID	    (CS_SID)0x1L
#else
#define			CS_DONT_CARE	    (CS_SID)0x0
#define			CS_ADDER_ID	    (CS_SID)0x1
#endif /* SCALARP_IS_LONG */
/*   -	-   -	-   -	-   -	-   -	-   -	-   -	-   -	-   */
    /*
    ** This section is very heavily machine specific.  It is placed near the
    ** top to reduce offsets and thus to reduce code space.
    */

# if defined(rs4_us5) || \
     ( defined(OSX) && defined(__ppc__)) || defined(ppc_lnx)
    /* ibm rs/6000, according to the manual, uses regs this way:
       gpr 0 - scratch - used in prologs
       gpr 1 - saved  - stack pointer
       gpr 2 - saved  - TOC (table of contents) register
       gpr 3 - scratch - param 1, return value
       gpr 4 - scratch - param 2, return value
       ...
       gpr 10 - scratch - param 8, return value
       gpr 11 - scratch, pointer to FCN; DSA ptr to int proc (Env)
       gpr 12 - scratch, PL.8 exception return
       gpr 13-31 - saved.  Non volatile.

 */
    PTR         cs_exsp;                /* exception stack pointer */
    long        cs_pc;                  /* aka LR (link register) */
    long        cs_registers[21];       /* gpr 1..2, 13..31 */
/* byte offsets for specific registers within cs_registers */
#       define CS_SP_OFFSET     0       /* aka gpr 1 */
#       define CS_TOC_OFFSET    4       /* aka gpr 2 */
#       define CS_GPR13_OFFSET  8       /* gpr 13 */
/* index into cs_registers */
#       define CS_SP     0       /* aka gpr 1 */
#       define CS_TOC    1       /* aka gpr 2 */
#       define CS_GPR13  2       /* gpr 13 */

#       define cs_sp cs_registers[0]
#       define cs_toc cs_registers[1]
#       define cs_gpr13 cs_registers[2]

# endif /* ppc */

#if defined(r64_us5)
    PTR         cs_exsp;                /* exception stack pointer */
    long        cs_pc;                  /* aka LR (link register)  */
    long        cs_registers[21];       /* hold sp, toc registers  */
/* byte offsets for specific registers within cs_registers */
#       define CS_SP_OFFSET     0       /* aka gpr 1 */
#       define CS_TOC_OFFSET    8       /* aka gpr 2 */
/* index into cs_registers */
#       define CS_SP     0       /* aka gpr 1 */
#       define CS_TOC    1       /* aka gpr 2 */
#       define CS_GPR13  2       /* gpr 13 */
#       define cs_sp cs_registers[0]
#       define cs_toc cs_registers[1]
#       define cs_gpr13 cs_registers[2]
    long        cs_regs_stored;     /* flag for registers restore in   */
                                    /* CS_swuser, see more detail in   */
                                    /* csll.ris64.asm                  */
# endif /* r64_us5 */

# if defined(su4_u42) || defined(dr6_us5) || \
     defined(sparc_sol) || defined(su4_cmw)
    long	cs_registers[1];	/* stack frame pointer */
    long	cs_pc;			/* PC */
    long	cs_exsp;		/* exception stack pointer */
# define CS_SP 0
# define cs_sp cs_registers[CS_SP]
# endif /* su4_u42 dr6_us5 sparc */

# if defined(sco_us5) || defined(sqs_ptx) || defined(usl_us5) || \
     defined(nc4_us5) || defined(dgi_us5) || defined(sos_us5) || \
     defined(sui_us5) || defined(int_lnx) || defined(i64_lnx) || \
     defined(a64_lnx) || defined(int_rpl) || defined(int_osx)
    /*  We only need to save five registers on Symmetry:  %esi, %edi, 
        %ebx, esp, efp and all 387 floating registers are volatile
        The pc is saved on the stack. */
    long	cs_sp;			/* stack pointer */
    long	cs_fp;			/* frame pointer */
    long	cs_ebx;			/* non-scratch register */
    long	cs_esi;			/* source index */
    long	cs_edi;			/* destination index */
    long	cs_exsp;		/* exception stack pointer */
# endif /* sco_us5 usl_us5 dgi_us5 sui_us5 */

# if defined(ibm_lnx)
  PTR		cs_exsp;		/* Exception stack pointer */
  long		cs_registers[16];	/* Pointer to general purpose registers,
					** also happens to be a pointer to the
					** start of the frame, space is
					** allocated to store all 16 registers,
					** but we only store registers 2 - 14
					*/
  long		cs_pc;			/* PC */
  int		cs_id;			/* This is temp id for debugging */
  long		cs_ca;			/* The link address of the function
					** called
					*/
# define CS_BP 13			/* The reg that stores the calling base
					** ptr
					*/
# define CS_RETADDR 14			/* The reg that stores (rts_address - 2)
					*/
# define CS_SP 15			/* The reg that stores the frame
					** pointer
					*/
# define cs_sp	cs_registers[CS_SP];
# endif /* ibm_lnx */

# if defined(ds3_ulx) || defined(rmx_us5) || defined(pym_us5) || \
     defined(ts2_us5) || defined(sgi_us5) || defined(rux_us5)
    long	    cs_sp;		/* stack pointer : $29 = sp */
    long	    cs_pc;		/* return address: $31 = ra */
    PTR	    	    cs_exsp;		/* exception stack pointer  */
    long            cs_registers[21];   /* saved registers */
#define                 CS_SO	0	/* 	$16 = s0 */
#define			CS_S1	1	/* 	$17 = s1 */
#define			CS_S2	2	/* 	$18 = s2 */
#define			CS_S3	3	/* 	$19 = s3 */
#define			CS_S4	4	/* 	$20 = s4 */
#define			CS_S5	5	/* 	$21 = s5 */
#define			CS_S6	6	/* 	$22 = s6 */
#define			CS_S7	7	/* 	$23 = s7 */
#define			CS_S8	8	/*	$30 = s8 */
#define			CS_F20	9	/*	$f20 */
#define			CS_F21	10	/*	$f21 */
#define			CS_F22	11	/*	$f22 */
#define			CS_F23	12	/*	$f23 */
#define			CS_F24	13	/*	$f24 */
#define			CS_F25	14	/*	$f25 */
#define			CS_F26	15	/*	$f26 */
#define			CS_F27	16	/*	$f27 */
#define			CS_F28	17	/*	$f28 */
#define			CS_F29	18	/*	$f29 */
#define			CS_F30	19	/*	$f30 */
#define			CS_F31	20	/*	$f31 */
# endif /*ds3_ulx, rmx_us5, pym_us5, ts2_us5 */

# if defined(any_hpux)
    long        cs_exsp;          /* Exception stack pointer */
    long        cs_sp;            /* stack pointer */
    struct
    {
	/* keep track of stack bounds */
        SIZE_TYPE     cs_stktop;
        SIZE_TYPE     cs_stkbottom;

	/*
	** For reasons known only to HP, the type of jmp_buf was changed
	** from int to double in HP-UX 8.0.  However, the information
	** contained therein did not change.  We use the union below to
	** remind us that we are still accessing the jmp_buf information.
	**
	** use setjmp/longjmp to save/restore thread registers
	**	cs_jmp_buf[0] == RP
	**	cs_jmp_buf[1] == SP
	*/
	union
	{
	    jmp_buf hp_jmp_buf;
	    SIZE_TYPE cs_jmp_buf[2];
	} hp;
    } cs_mach;
# define CS_check_stack(mach) ((mach)->hp.cs_jmp_buf[1] > (mach)->cs_stktop)
# endif	/* hp-ux */

/*
** 680x0 with FPCP registers saved.
** Note that the 6888[1/2] uses 96 bit IEEE extended precision.
*/

# ifdef dg8_us5

	PTR	cs_exsptr;
					/* The 88K has registers r0..r31.
					 * r0 - always 0
					 * r1 - return pc (use jmp r1 to return)
					 * r2 .. r9 - parameter passing regs.
						      don't have to be saved
					 * r10.. r13- scratch
					 * r14.. r25- used by calling proc.
						      these must be saved.
					 * r26.. r29- reserved. Advised that
						      these should be saved.
					 * r30 - frame pointer
					 * r31 - stack pointer */ 
	long	cs_registers[19]; 
# define CS_SP  18
# define CS_FP  17
# define CS_PC  0
# define CS_R14 1
# define CS_R29 16

# endif /* dg8_us5 */

# if defined(axp_osf) || defined(axp_lnx)
    PTR	    	    cs_exsp;		/* exception stack pointer  */
    long	    cs_pc;		/* AXP return address */
    long	    cs_sp;		/* AXP stack pointer */
    long	    cs_pv;		/* AXP procedure value */
# endif /* axp_osf */

# if defined(VMS)
    i8		    cs_registers[31];   /* ALPHA integer register set */
#define			CS_ALF_R0    0
#define			CS_ALF_R1	 1
#define			CS_ALF_R2	 2
#define			CS_ALF_R3	 3
#define			CS_ALF_R4	 4
#define			CS_ALF_R5	 5
#define			CS_ALF_R6	 6
#define			CS_ALF_R7	 7
#define			CS_ALF_R8	 8
#define			CS_ALF_R9	 9
#define			CS_ALF_R10	10
#define			CS_ALF_R11	11
#define			CS_ALF_R12	12
#define			CS_ALF_R13	13
#define			CS_ALF_R14	14
#define			CS_ALF_R15	15
#define			CS_ALF_R16	16
#define			CS_ALF_R17	17
#define			CS_ALF_R18	18
#define			CS_ALF_R19	19
#define			CS_ALF_R20	20
#define			CS_ALF_R21	21
#define			CS_ALF_R22	22
#define			CS_ALF_R23	23
#define			CS_ALF_R24	24
#define			CS_ALF_AI	25
#define			CS_ALF_RA	26
#define			CS_ALF_PV	27
#define			CS_ALF_R28	28
#define			CS_ALF_FP	29
#define			CS_ALF_SP	30
#define cs_sp	cs_registers[CS_ALF_SP]

    double	    cs_flt_registers[31];	/* ALPHA flt pt register set */
#define			CS_ALF_F0	0
#define			CS_ALF_F1	1
#define			CS_ALF_F2	2
#define			CS_ALF_F3	3
#define			CS_ALF_F4	4
#define			CS_ALF_F5	5
#define			CS_ALF_F6	6
#define			CS_ALF_F7	7
#define			CS_ALF_F8	8
#define			CS_ALF_F9	9
#define			CS_ALF_F10	10
#define			CS_ALF_F11	11
#define			CS_ALF_F12	12
#define			CS_ALF_F13	13
#define			CS_ALF_F14	14
#define			CS_ALF_F15	15
#define			CS_ALF_F16	16
#define			CS_ALF_F17	17
#define			CS_ALF_F18	18
#define			CS_ALF_F19	19
#define			CS_ALF_F20	20
#define			CS_ALF_F21	21
#define			CS_ALF_F22	22
#define			CS_ALF_F23	23
#define			CS_ALF_F24	24
#define			CS_ALF_F25	25
#define			CS_ALF_F26	26
#define			CS_ALF_F27	27
#define			CS_ALF_F28	28
#define			CS_ALF_F29	29
#define			CS_ALF_F30	30
    PTR		    cs_exsp;		/* exception stack pointer */
#endif /* VMS */

/*  -	-   -	-   -	-   -	-   -	-   -	-   -	-   -	-   */
    i4	    cs_client_type;	/* must be 0 in CL SCBs */
    SPBLK	    cs_spblk;		/* used within CS for known thread tree.  */
    char	    *cs_stk_area;       /* Ptr to allocated stack base */
    i4              cs_stk_size;        /* Amt of space allocated there */
    i4              cs_state;           /* Thread state */
#define CS_STATE_ENUMS /* enum cs_state_enums */\
_DEFINE(		CS_FREE,            0x0000 )\
_DEFINE(		CS_COMPUTABLE,      0x0001 )\
_DEFINE(		CS_EVENT_WAIT,      0x0002 )\
_DEFINE(		CS_MUTEX,           0x0003 )\
_DEFINE(		CS_STACK_WAIT,      0x0004 )\
_DEFINE(		CS_UWAIT,           0x0005 )\
_DEFINE(		CS_CNDWAIT,         0x0006 )\
_DEFINESEND

    i4	    cs_mask;
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

     i4              cs_priority;        /* Priority of thread */
     i4	     cs_base_priority;	/* base priority */
    i4		    cs_thread_type;	/* type of thread */
#define                 CS_ASYNC_THREAD        -2   /* async completion */
#define			CS_INTRNL_THREAD       -1   /* Thread used by CS */
#define			CS_USER_THREAD		0   /* User session */
#define			CS_CLEANUP_MASK    0x1000   /* Must cleanup user
						       sessions before exit */
    i4		    cs_timeout;		/* timeout while waiting */
    i4		    cs_sem_count;	/* number of semaphores owned here */
    CS_SCB_Q        cs_rw_q;		/* next on the ready or wait que */
    CS_SEMAPHORE    *cs_sm_root;	/* root for smphr list for this scb */
    CS_SCB	    *cs_sm_next;	/* next scb in smphr list */
    i4		    cs_mode;		/* what's up now */
    i4		    cs_nmode;		/* what's up next */
    i4		    cs_uic;		/* UIC of session owner */
    i4	    	    cs_cputime;		/* cputime of this thread (10ms) - is periodically reset to zero */	
					/* as a side-effect of OPF activity, unless session accounting is ON */
    i4	    	    cs_dior;		/* disk io read count */
    i4	    	    cs_diow;		/* disk io write count */
    i4	    	    cs_bior;		/* fe/dbms comm read count */
    i4	    	    cs_biow;		/* fe/dbms comm write count */
    i4	    	    cs_locks;		/* number of lock waits */
#if defined(VMS)
    i4		    cs_gwfio;		/* number of CS_GWFIO_MASK waits */
#endif
    i4	    	    cs_lkevent;		/* number of LKevent waits */
    i4	    	    cs_logs;		/* number of log waits */
    i4	    	    cs_lgevent;		/* number of LGevent waits */
    i4	    	    cs_lior;		/* number of log read waits */
    i4	    	    cs_liow;		/* number of log write waits */
    PTR   	    cs_sync_obj;	/* Unix CS synchronisation object */
    i4   	    cs_memory;		/* Reason thread is suspended: */
#define			CS_IOW_MASK	    0x200000 /* I/O Write */
#define			CS_IOR_MASK	    0x100000 /* I/O Read */
#define			CS_LIO_MASK	    0x080000 /* Log I/O */
#define			CS_LGEVENT_MASK	    0x040000 /* Log event */
#define			CS_LKEVENT_MASK	    0x020000 /* Lock event */
#define			CS_DIO_MASK	    0x008000 /* Disk I/O */
#define			CS_BIO_MASK	    0x004000 /* BIO */
#define			CS_LOCK_MASK	    0x002000 /* Lock */
#define                 CS_LOG_MASK         0x001000 /* Log */

#define                 CS_BIOR_MASK    (CS_BIO_MASK | CS_IOR_MASK)
#define                 CS_BIOW_MASK    (CS_BIO_MASK | CS_IOW_MASK)
#define                 CS_DIOR_MASK    (CS_DIO_MASK | CS_IOR_MASK)
#define                 CS_DIOW_MASK    (CS_DIO_MASK | CS_IOW_MASK)
#define                 CS_LIOR_MASK    (CS_LIO_MASK | CS_IOR_MASK)
#define                 CS_LIOW_MASK    (CS_LIO_MASK | CS_IOW_MASK)

    i4	    cs_connect;		/* time in seconds of connection */
    PTR		    cs_svcb;		/* server control block addr */
    i4	    cs_ppid;		/* Misnamed! seems to be used for idle timing */
    char	    cs_username[CS_MAXNAME];/* user owning this session */
    i4		    cs_inkernel;	/* scb is busy and should not be altered
					** by interrupt driven routines. */
    i4		    cs_async;		/* set if an asynchronous event occurred
					** for this session while cs_inkernel
					** was set.  there is information to
					** be set in the scb when inkernel is
					** turned off. */
    i4		    cs_asmask;		/* bits to be set in cs_mask when
					** cs_inkernel is turned off. */
    i4		    cs_asunmask;	/* bits to be reset in cs_mask when
					** cs_inkernel is turned off. */
    CS_SCB_Q	    cs_as_q;		/* list of scb's to be moved to ready
					** queue when cs_inkernel is reset. */
    i4         cs_pid;             /* process id of scb's server process */
    i4	    cs_ef_mask;		/* event flag wait mask. */
    i4	    cs_cs_mask;		/* CS-specific session flags */
/* #define		CS_CLEANUP_MASK    0x1000 */ /* defined above */
    CS_CONDITION    *cs_cnd;		/* point to queued waiting condition */
    CS_THREAD_ID    cs_thread_id;       /* OS_THREADs id, zero if not OS_THREADS */
# ifdef OS_THREADS_USED

#if defined(LNX) || defined(VMS)
    i4		    cs_cpu;	  	/* CPU usage of thread */
#endif
    CS_OS_PID	    cs_os_pid;		/* OS process id */
    CS_OS_TID	    cs_os_tid;		/* OS thread id (as opposed to the id returned from the
					** threads library call, as stored in cs_thread_id). 
					** Set to 0 if not available. */

    CS_SM_WAKEUP_CB *cs_evcb;           /* for event signaling and changes */
					/* to cs_state, cs_mask		   */
/* The IdleThread and direct CP suspend/resume cannot coexist! */
# ifdef USE_IDLE_THREAD
    CS_SM_WAKEUP_CB  cs_local_wakeup;	/* pointed to by cs_evcb */
# endif /* USE_IDLE_THREAD */
    CS_CONDITION    cs_cndq;		/* For queueing SCBs awaiting */
					/* the same condition */
# endif /* OS_THREADS_USED */
    PTR		    cs_diag_qry;	/* pointer to the current query for the thread  */
    u_i4	    cs_random_seed;	/* The random function seed value  */
    u_i4	    cs_last_ccheck;	/* (wallclock) time of last cancel
					** check, for CScancelCheck.  May not
					** be used in all cases (eg not used
					** if internal threads).
					*/
    PTR		    cs_ditgiop;		/* A place for DI gatherWrite to anchor a work area */
#if defined(VMS) && defined(KPS_THREADS)
    KPB_PQ          kpb;                /* KP Services block address (64 bit) */
#endif
};

/*
** Name: CS_AINCR - Atomic increment item.
**
** Description:
**	This defines an object type that can be incremented or decremented
**	atomically.
**
** History:
**	17-sep-1998 (canor01)
**	    Added.
**
*/
# ifdef OS_THREADS_USED

struct _CS_AINCR
{
    CS_SYNCH cs_mutex;
    i4  cs_value;
};
typedef struct _CS_AINCR  CS_AINCR;

# else /* OS_THREADS_USED */

typedef i4 CS_AINCR;

# endif /* OS_THREADS_USED */
FUNC_EXTERN i4 CS_atomic_increment(CS_AINCR *aincr);
FUNC_EXTERN i4 CS_atomic_decrement(CS_AINCR *aincr);
FUNC_EXTERN STATUS CS_atomic_init(CS_AINCR *aincr);


/*}
** Name: CS_ASET - Atomic test/set item.
**
** Description:
**	This defines an object type that is atomically (test/set)able by the
**	host operating system.
**
** History:
**	24-jul-1989 (rogerk)
**	    Moved definition from csev.h
**      02-jul-1991 (johnr/rog)
**	    Added hp3_us5 to the appropriate list of definitions as was used
**	    in cs.h in the 63/02p hp3_us5 port.
**	28-Jun-2005 (schka24)
**	    Don't redefine test/set macros for a64_lnx.  Just do the
**	    proper definition.
**	30-May-2007 (kschendel)
**	    ASET type on a64_lnx should just be int, not that it matters
**	    all that much.  I suspect the same is true for other LP64's,
**	    but we'll let the porting engineers figure that one out.
[@history_template@]...
*/
# if defined(rmx_us5) || defined(pym_us5) || defined(rux_us5)
#include <systypes.h>   /* otherwise, get problems with compat.h users */
#include <ulocks.h>
# define cs_aset_type
typedef spinlock_t              CS_ASET;
/* See pym_us5 section below for explanation of type */
# endif /* rmx_us5, pym_us5 */

#if defined(ts2_us5)
#include <synch.h>
#define cs_aset_type
typedef spin_t		CS_ASET;
#endif /* ts2_us5 */

#if defined(sgi_us5)
#include <abi_mutex.h>
#define cs_aset_type
typedef abilock_t		CS_ASET;
#endif /* sgi_us5 */

# ifdef hp3_us5
# define cs_aset_op
# define        CS_ISSET(a)             (*(a) != 0)
# define        CS_ACLR(a)              (*(a) = 0)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)               (CS_tas(a))
# define        CS_SPININIT(s)          (MEfill(sizeof(*s),'\0',(char *)s))
# endif /* hp3_us5 */

#if defined(ds3_ulx) || defined(dr6_us5)
# define cs_aset_type
typedef i4                      CS_ASET;        /* atomicly setable item */
# endif /* ds3_ulx dr6_us5 */

# if defined(nc4_us5)
# define cs_aset_type
typedef i2                      CS_ASET;        /* atomicly setable item */
# endif /* nc4_us5 */

# if defined(sqs_ptx)
# define cs_aset_type
typedef volatile char           CS_ASET;        /* atomicly setable item */
# endif

# if defined(su4_u42) || defined(sparc_sol) || defined(su4_cmw) || \
     defined(a64_sol)
# define cs_aset_type
typedef long                    CS_ASET;        /* atomicly setable item */
# endif

# if defined(i64_hpu)
# define cs_aset_type
typedef long                    CS_ASET;        /* atomicly setable item */

# elif defined(any_hpux)
# define cs_aset_type
/* Note: HP S800 needs 16 byte aligned word.  Allocate a larger region
         to ensure the alignment needs can be met. */
typedef volatile struct {int cs_space[4];} CS_ASET;
# endif

# if defined(ris_us5) || defined(ibm_lnx)
# define cs_aset_type
typedef volatile i4                     CS_ASET;
# endif /* ris_us5 */

# if defined(any_aix)
# define cs_aset_type
typedef int                    		CS_ASET;
# endif /* aix */

# ifdef dg8_us5 /* DG/UX */
# define cs_aset_type
typedef int                     CS_ASET;        /* atomicly setable item */
# endif         /* dg8_us5 */

# if defined(axp_osf) || defined(axp_lnx) || \
     defined(i64_lnx) || defined(ppc_lnx)
# define cs_aset_type
typedef volatile long		CS_ASET;	/* atomicly setable item */
# endif

# if defined(VMS)
# define cs_aset_type
typedef volatile i4 CS_ASET;
#endif

# if defined(a64_lnx) || defined(int_lnx) || defined(int_rpl)
# define cs_aset_type
typedef volatile int	CS_ASET;	/* atomicly setable item */
# endif

# if defined(mg5_osx) || defined(int_osx)
# define cs_aset_type
typedef volatile OSSpinLock	CS_ASET;	/* atomicly setable item */
# endif

# ifndef cs_aset_type
# define cs_aset_type
typedef char                    CS_ASET;        /* atomicly setable item */
# endif

/*
** Atomic test & set operations
*/

# if defined(rmx_us5) || defined(pym_us5) || defined(rux_us5)
/* MIPS processor doesn't offer a test-and-set, so we must use library fn.
** According to spinlock(3x) docn, spinlock fns are better performers than
** the ABI _test_and_set fn. So we use the OS spinlock_t type for our TAS
** target type (CS_ASET), with the cspinlock fn that just tries to set,
** doesn't spin.
** Note that our spinlock target type (CS_SPIN) is CS_ASET plus our
** collision counter, so a superset of spinlock_t.
** spinlock_t is struct {volatile int sl_lock; long sl_count;}
**
** CS_GETSPIN (and CS_SPIN) are only used for quick critical-sections,
** so I specify a spincnt (to initspin) so spinlock() will yield after loops.
** (If we loop there, some process must have unluckily been time-sliced.)
** (If we loop there, some process must have unluckily been time-sliced.)
** Mostly, our spinning is done manually with CS_TAS.
**
** ATOMIC_CLEAR and ATOMIC_SET aren't used outside here and CS_tas (if any),
** so we don't export them.
*/

# define cs_aset_op
# define CS_ISSET(a)    ((a)->sl_lock != 0)
# define CS_ACLR(a)     (spinunlock(a))
/* I'd like to make this "(a)->sl_lock=0", but this isn't reliable. Unknown why.
 */

# define CS_TAS(a)      (cspinlock(a) ? 0 : 1)
/* CS_TAS: cspinlock tries to set lock once, nonzero if fail (in use) */

# define CS_SPININIT(s) (initspin(&((s)->cssp_bit), 50), (s)->cssp_collide=0)
/* CS_SPININIT: clear lock (CS_ASET) & collision count. Most port do memzero,
** but we want to supply an sl_count. Hopefully this isn't frequent, anyway.
** The sl_count to initspin (2nd arg) is arbitrarily chosen.
*/

# define CS_relspin(s)   CS_ACLR(&(s)->cssp_bit)
# define CS_getspin(s)  spinlock(&(s)->cssp_bit)
# endif /* rmx_us5, pym_us5 */

# if defined(ts2_us5)

# define cs_aset_op
# define CS_ISSET(a)		((a)->m_lmutex.lock != 0)
# define CS_ACLR(a)		(_spin_unlock(a))

# define CS_TAS(a)		(_spin_trylock(a) ? 0 : 1)
/* CS_TAS: _spin_trylock tries to set lock once, nonzero if fail (in use) */

# define CS_SPININIT(s)		(_spin_init(&((s)->cssp_bit), NULL), (s)->cssp_collide=0)
/* CS_SPININIT: clear lock (CS_ASET) & collision count. */

# define CS_relspin(s)		CS_ACLR(&(s)->cssp_bit)
# define CS_getspin(s)		_spin_lock(&(s)->cssp_bit)
# endif /* ts2_us5 */

# if defined(sgi_us5)
/* MIPS processor doesn't offer a test-and-set, so we must use library fn.
** According to abilock(3x) docn, spinlock fns are better performers than
** the ABI _test_and_set fn. So we use the OS abilock_t type for our TAS
** target type (CS_ASET), with the acquire_lock fn that just tries to set,
** doesn't spin.
** Note that our spinlock target type (CS_SPIN) is CS_ASET plus our
** collision counter, so a superset of abilock_t.
** abilock_t is struct {unsigned int abi_lock;}
**
** CS_GETSPIN (and CS_SPIN) are only used for quick critical-sections,
** so I specify a spincnt (to initspin) so spin_lock() will yield after loops.
** (If we loop there, some process must have unluckily been time-sliced.)
** Mostly, our spinning is done manually with CS_TAS.
**
** ATOMIC_CLEAR and ATOMIC_SET aren't used outside here and CS_tas (if any),
** so we don't export them.
*/

# define cs_aset_op
# define CS_ISSET(a)    ((a)->abi_lock != 0)
# define CS_ACLR(a)     (release_lock(a))

# define CS_TAS(a)      (acquire_lock(a) ? 0 : 1)
/* CS_TAS: cspinlock tries to set lock once, nonzero if fail (in use) */

# define CS_SPININIT(s) (init_lock(&((s)->cssp_bit)), (s)->cssp_collide=0)
/* CS_SPININIT: clear lock (CS_ASET) & collision count. Most port do memzero,
** but in case, we are using the system function to clear the abilock_t 
** structure. */
# define CS_relspin(s)   CS_ACLR(&(s)->cssp_bit)
# define CS_getspin(s)  spin_lock(&(s)->cssp_bit)
# endif /* sgi_us5 */


# if defined(ds3_ulx)
# define cs_aset_op
# define ATOMIC_SET             0
# define ATOMIC_CLEAR           1
# define CS_ISSET(a)            (*(a) != 0)
# define CS_ACLR(a)             (atomic_op(ATOMIC_CLEAR, (a)))
# define CS_TAS(a)              (CS_tas(ATOMIC_SET, (a)) == 0)
# define CS_tas                 atomic_op
# define CS_SPININIT(s)         (MEfill(sizeof(*s),'\0',(char *)s))
# define CS_relspin(s)          (CS_ACLR(&(s)->cssp_bit))

# define CS_getspin(s)                  \
    if (!CS_TAS(&(s)->cssp_bit))        \
    {                                   \
        (s)->cssp_collide++;            \
        while (!CS_TAS(&(s)->cssp_bit)) \
                ;                       \
    }                                   \
    else
# endif	/* ds3_ulx */

# if defined(sparc_sol) || defined(sui_us5) || defined(usl_us5)
/* OS_THREADS_USED assumed: use CS_tas, but use system spinlock */
# define cs_aset_op
# define        CS_ISSET(a)             (*(a) != 0)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)               (CS_tas(a))
# ifdef OS_THREADS_USED
FUNC_EXTERN i4 CS_aclr(CS_ASET *);
# define        CS_ACLR(a)              (CS_aclr(a))
# define        CS_SPININIT(s)          { STATUS st; CS_cp_synch_init(&(s)->cssp_mutex,&st); }
# define        CS_getspin(s)           CS_synch_lock(&(s)->cssp_mutex)
# define        CS_relspin(s)           CS_synch_unlock(&(s)->cssp_mutex)
# else /* OS_THREADS_USED */
# define        CS_ACLR(a)              (CS_relspin(a))
# define        CS_SPININIT(s)          (MEfill(sizeof(*s),'\0',(char *)s))
# endif /* OS_THREADS_USED */
# endif /* sparc usl */

# if defined(su4_u42) || defined(sqs_ptx) || \
     ((defined(int_lnx) || defined(int_rpl)) && \
     (!defined(LP64) && !defined(NO_INTERNAL_THREADS))) || \
     defined(su4_cmw) || defined(sos_us5) || defined(ibm_lnx)
# define cs_aset_op
# define        CS_ISSET(a)             (*(a) != 0)
# define        CS_ACLR(a)              (CS_relspin(a))
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)               (CS_tas(a))
# define        CS_SPININIT(s)          (MEfill(sizeof(*s),'\0',(char *)s))
# endif

# if (defined(int_lnx) && defined(NO_INTERNAL_THREADS)) || \
     (defined(int_rpl) && defined(NO_INTERNAL_THREADS))
# define cs_aset_op
# define	CS_tas(a)		test_and_set_bit((int) 0, a)
# define	CS_ISSET(a)		(*(a) != (CS_ASET)0)
# define	CS_ACLR(a)		CS_aclr(a)
# define	CS_TAS(a)		(!CS_tas(a))
# define	CS_SPININIT(s)		(MEfill(sizeof(*s),'\0',(char *)s))
# define	CS_relspin(s)		(CS_ACLR(&(s)->cssp_bit))

/*
 * In-line assembler functions for atomic sychronization operations,
 * based on those found in asm-i386/bitops.h.
 * Better soln would probably be to use GCC compiler macros but this 
 * can cause problems with older GCC versions.
*/
# define ADDR (*(CS_ASET *) addr)
static inline void clear_bit(int nr, CS_ASET * addr)
{
	__asm__ __volatile__( "lock ; "
		"btrl %1,%0"
		:"=m" (ADDR)
		:"Ir" (nr));
}
static inline int test_and_set_bit(int nr, CS_ASET * addr)
{
	int oldbit;

	__asm__ __volatile__( "lock ; "
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"Ir" (nr) : "memory");
	return oldbit;
}

# define CS_getspin(s)			 \
    {					 \
    if (CS_tas(&(s)->cssp_bit))		 \
	{				 \
	while (CS_tas(&(s)->cssp_bit))	 \
	    ;				 \
	(s)->cssp_collide++;		 \
	}				 \
    }

static inline void CS_aclr( volatile void * ptr )
{
	__asm__ __volatile__("": : :"memory");
	clear_bit(0, ptr);
	__asm__ __volatile__("": : :"memory");
}

# endif /* int_lnx || int_rpl && NO_INTERNAL_THREADS */

# if defined(a64_sol)
# define cs_aset_op
# define        CS_ISSET(a)             (*(a) = 0)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)               (CS_tas(a))
FUNC_EXTERN i4 CS_aclr(CS_ASET *);
# define	CS_ACLR(a)		(*(a) = 0)
# define        CS_SPININIT(s)          { STATUS st; CS_cp_synch_init(&(s)->cssp_mutex,&st); }
# define        CS_getspin(s)           CS_synch_lock(&(s)->cssp_mutex)
# define        CS_relspin(s)           CS_synch_unlock(&(s)->cssp_mutex)
# endif

# if defined(i64_hpu)
# define cs_aset_op
# define CS_ACLR(a)    (*(a)=0)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define CS_TAS(a)     (!CS_tas(a))
# define CS_ISSET(a)   (*(a) != 0)

# define CS_getspin(s)                   \
    if (!CS_TAS(&(s)->cssp_bit))         \
        {                                \
        (s)->cssp_collide++;             \
        while (!CS_TAS(&(s)->cssp_bit))  \
            ;                            \
        }

# define CS_relspin(s)  CS_ACLR(&(s)->cssp_bit)

# define CS_SPININIT(s)   {              \
    (s)->cssp_collide = 0;               \
    CS_ACLR(&(s)->cssp_bit);             \
    }

# elif defined(any_hpux)
# define cs_aset_op
FUNC_EXTERN i4 CS_aclr(CS_ASET *);
# define CS_ACLR(a)    CS_aclr(a)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define CS_TAS(a)     CS_tas(a)
# define CS_ISSET(a)   CS_isset(a)

# define CS_getspin(s)                   \
    if (!CS_TAS(&(s)->cssp_bit))         \
        {                                \
        (s)->cssp_collide++;             \
        while (!CS_TAS(&(s)->cssp_bit))  \
            ;                            \
        }

# define CS_relspin(s)  CS_ACLR(&(s)->cssp_bit)

# define CS_SPININIT(s)   {              \
    (s)->cssp_collide = 0;               \
    CS_ACLR(&(s)->cssp_bit);             \
    }
# endif	/* hp-ux */

# if defined(dr6_us5) || defined(nc4_us5) 
# define cs_aset_op
# define        CS_ACLR(a)              (*(a) = 0)
# define	CS_ISSET(a)		(*(a) != 0)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)		(CS_ISSET(a) ? 0 : (CS_tas(a)))
# define	CS_SPININIT(s)		(MEfill(sizeof(*s),'\0',(char *)s))
# define        CS_relspin(s)   	(CS_ACLR(&(s)->cssp_bit))
# define CS_getspin(s)                   \
    {                                    \
    if (!CS_TAS(&(s)->cssp_bit))         \
        {                                \
        while (!CS_TAS(&(s)->cssp_bit))  \
            ;                            \
        (s)->cssp_collide++;             \
        }                                \
    }
# endif /* dr6_us5 */

# if defined(dgi_us5) 
# define	CS_ACLR(a)	(*(a) = 0)
#endif /* dgi_us5 */

# ifdef ris_us5
# define cs_aset_op
# define 	CS_ACLR(a)	(*(a) = (CS_ASET)0)
# define	CS_ISSET(a)	(*(a) != 0)
# define        CS_TAS(csspbit) (CS_ISSET(csspbit) ? \
                                      0 : (cs(csspbit, (CS_ASET)0, 1) == 0))
# define	CS_SPININIT(s)	(MEfill(sizeof(*s),'\0',(char *)s))
# define        CS_relspin(s)   (CS_ACLR(&(s)->cssp_bit))
# define CS_getspin(s)                   \
    {					 \
    if (!CS_TAS(&(s)->cssp_bit))         \
        {                                \
        while (!CS_TAS(&(s)->cssp_bit))  \
            ;                            \
        (s)->cssp_collide++;             \
        }				 \
    }
# endif

# if defined(any_aix)         /* AIX 4.1 SMP */
# define cs_aset_op

int _check_lock(atomic_p, int, int);
void _clear_lock(atomic_p, int);
int _safe_fetch(atomic_p);

# define 	CS_ACLR(a)	(_safe_fetch( a ), _clear_lock( (a), 0 ))
# define	CS_ISSET(a)	_safe_fetch( a )
# define        CS_TAS(csspbit) (!(_check_lock(csspbit, (CS_ASET)0, 1)))
# define	CS_SPININIT(s)	(MEfill(sizeof(*s),'\0',(char *)s))
# define        CS_relspin(s)   (CS_ACLR(&(s)->cssp_bit))
# define CS_getspin(s)                   \
    {					 \
    if (!CS_TAS(&(s)->cssp_bit))         \
        {                                \
        while (!CS_TAS(&(s)->cssp_bit))  \
            ;                            \
        (s)->cssp_collide++;             \
        }				 \
    }
# endif /* aix */    /* AIX 4.1 SMP */

/* Note that the assembler CS_tas return code value is flipped !! */
# if defined(i64_lnx)
# define cs_aset_op
# define	CS_ISSET(a)		(*(a) != (CS_ASET)0)
# define	CS_ACLR(a)		(CS_aclr(a))
# define	CS_TAS(a)		(!CS_tas(a))
# define	CS_SPININIT(s)		(MEfill(sizeof(*s),'\0',(char *)s))
# define CS_getspin(s)			 \
    {					 \
    if (CS_tas(&(s)->cssp_bit))		 \
	{				 \
	while (CS_tas(&(s)->cssp_bit))	 \
	    ;				 \
	(s)->cssp_collide++;		 \
	}				 \
    }
# endif	/* i64_lnx */

# if defined(a64_lnx)
# define cs_aset_op
# define 	CS_tas(a)		test_and_set_bit((int) 0, a)
# define	CS_ISSET(a)		(*(a) != (CS_ASET)0)
# define	CS_ACLR(a)		CS_aclr(a)
# define	CS_TAS(a)		(!CS_tas(a))
# define	CS_SPININIT(s)		(MEfill(sizeof(*s),'\0',(char *)s))

/*
 * In-line assembler functions for atomic sychronization operations,
 * based on those previously found in asm-x86_64/bitops.h. File
 * has been removed from later Linux distributions but functions
 * are still needed.
 * Better soln would probably be to use GCC compiler macros but this 
 * requires further investigation.
*/
# define ADDR (*(CS_ASET *) addr)
static __inline__ void clear_bit(int nr, CS_ASET * addr)
{
        __asm__ __volatile__( "lock ; "
                "btrl %1,%0"
                :"=m" (ADDR)
                :"dIr" (nr));
}
static __inline__ int test_and_set_bit(int nr, CS_ASET * addr)
{
        int oldbit;

        __asm__ __volatile__( "lock ; "
                "btsl %2,%1\n\tsbbl %0,%0"
                :"=r" (oldbit),"=m" (ADDR)
                :"dIr" (nr) : "memory");
        return oldbit;
}

# define CS_getspin(s)			 \
    {					 \
    if (CS_tas(&(s)->cssp_bit))		 \
	{				 \
	while (CS_tas(&(s)->cssp_bit))	 \
	    ;				 \
	(s)->cssp_collide++;		 \
	}				 \
    }

static __inline__ void CS_aclr( volatile CS_ASET * ptr )
{
	__asm__ __volatile__("": : :"memory");
	clear_bit(0, ptr);
	__asm__ __volatile__("": : :"memory");
}

# endif	/* a64_lnx */

# ifdef dg8_us5
# define cs_aset_op
# define        CS_ISSET(a)             (CS_isset(a))
FUNC_EXTERN i4 CS_aclr(CS_ASET *);
# define        CS_ACLR(a)              (CS_aclr(a))
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)               (CS_tas(a))
# define        CS_SPININIT(s)          (MEfill(sizeof(*s),'\0',(char *)s))
#endif /* dg8_us5 */

# if defined(axp_osf) || defined(axp_lnx)
# define cs_aset_op
# define        CS_ISSET(a)             (*(a) != (CS_ASET)0)
FUNC_EXTERN i4 CS_aclr(CS_ASET *);
# define        CS_ACLR(a)              (CS_aclr(a))
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define        CS_TAS(a)		(CS_tas(a))
# define        CS_SPININIT(s)          (MEfill(sizeof(*s),'\0',(char *)s))
# define CS_getspin(s)                  \
    if (!CS_TAS(&(s)->cssp_bit))        \
    {                                   \
        while (!CS_TAS(&(s)->cssp_bit)) \
                ;                       \
        (s)->cssp_collide++;            \
    }                                   
# endif	/* axp_osf */

# if defined(VMS)
# define cs_aset_op
# define CS_ISSET(aset) (0!=*(CS_ASET*)(aset))                    
# if defined(axm_vms)
# define CS_ACLR(aset) __TESTBITCCI(aset,0)                       
# define CS_TAS(aset) ((__TESTBITSSI(aset,0))?0:1)                
# endif  /* axm_vms */
# if defined(i64_vms)
# define CS_ACLR(aset) __ATOMIC_EXCH_LONG(aset, 0)
# define CS_TAS(aset) (__ATOMIC_EXCH_LONG(aset, 1) ? 0 : 1)
# endif  /* i64_vms */
# define CS_SPININIT(s) { STATUS st; CS_cp_synch_init(&(s)->cssp_mutex,&st); }
# define CS_getspin(s) CS_synch_lock(&(s)->cssp_mutex)
# define CS_relspin(s) CS_synch_unlock(&(s)->cssp_mutex)

# endif /* VMS */

# ifdef OSX
# define cs_aset_op

# define        CS_ACLR(a)              (OSSpinLockUnlock((CS_ASET *)a))
# define        CS_TAS(a)               OSSpinLockTry((CS_ASET *)a)
# define        CS_ISSET(a)             (*(CS_ASET *)(a) != (CS_ASET)0)
# define        CS_SPININIT(s)          { STATUS st; CS_cp_synch_init(&(s)->cssp_mutex,&st); }
# define CS_getspin(s)                  OSSpinLockLock((CS_ASET *)(&(s)->cssp_bit))
# define CS_relspin(s)                  OSSpinLockUnlock((CS_ASET *)(&(s)->cssp_bit))
# endif

# ifdef ppc_lnx
# define cs_aset_op
/*
** Name: CS_tas
**
** Description:
**	in-line atomic "test and set" routine for 64-bit PowerPC Linux.
**	Operates on the zeroth bit at the address passed
**
** Returns:
**	0 on success (bit was set)
**	1 on failure (bit is already set)
**
** History:
**	25-Mar-2007 (hanje04)
**	    Created.
*/
static __inline__ int
CS_tas(CS_ASET *addr)
{
        CS_ASET _t;
        int _res;

        __asm__ __volatile__(
"1:     ldarx   %0,0,%3         \n" /* load "spin bit" and reserve register */
"       cmpdi   %0,0            \n" /* is it zero? */
"       bne     2f              \n" /* if not, bail with error */
"       addi    %0,0,1          \n" /* add 1 to 0'd "spin bit" */
"       stdcx.  %0,0,%3         \n" /* write value back to memory */
"       bne     1b              \n" /* try again if it fails */
"       isync                   \n" /* synch across multiple processors */
"       li      %1,0            \n" /* load OK return code (0) */
"       b       3f              \n"
"2:     li      %1,1            \n" /* load Fail return code (1) */
"3:                             \n"

:       "=&r"(_t), "=r"(_res), "+m"(*addr)
:       "r"(addr)
:       "memory", "cc");
        return _res;
}

/*
** Name: CS_aclr
**
** Description:
**	in-line atomic clear routine for 64-bit PowerPC Linux.
**	Operates on the zeroth bit at the address passed
**
** History:
**	25-Mar-2007 (hanje04)
**	    Created.
*/
static __inline__ void
CS_aclr(CS_ASET *addr)
{
        CS_ASET _t;
        int _res;

        __asm__ __volatile__(
"1:     ldarx   %0,0,%3       \n" /* load "spin bit" and reserve register */
"       li    	%0,0          \n" /* Zero reserved register */
"       stdcx.  %0,0,%3       \n" /* write value back to memory */
"       bne-	1b	      \n" /* try again if it fails */
"	isync	              \n" /* synch across multiple processors */
:       "=&r"(_t), "=r"(_res), "+m"(*addr)
:       "r"(addr)
:       "memory", "cc");
	return;
}

# define        CS_ACLR(a)              (CS_aclr((CS_ASET *)a))
# define        CS_TAS(a)               (!CS_tas((CS_ASET *)a))
# define        CS_ISSET(a)             (*(a) != (CS_ASET)0)
# define        CS_SPININIT(s)          { STATUS st; CS_cp_synch_init(&(s)->cssp_mutex,&st); }
# define CS_getspin(s)                  \
    if (CS_tas(&(s)->cssp_bit))        \
    {                                   \
        while (CS_tas(&(s)->cssp_bit)) \
                ;                       \
        (s)->cssp_collide++;            \
    }
# endif /* ppc_lnx */

#ifndef cs_aset_op
# define	CS_ISSET(a)		(*(a) != 0)
FUNC_EXTERN i4 CS_tas(CS_ASET *);
# define	CS_TAS(a)		(CS_tas(a))
# define	CS_SPININIT(s)		(MEfill(sizeof(*s),'\0',(char *)s))
# define        CS_relspin(s)   (CS_ACLR(&(s)->cssp_bit))
#endif /* cs_aset_op */

# ifndef CS_ACLR
	# error: must define an atomic clear
# endif

#if defined(conf_CAS_ENABLED)

/*}
** Name: Atomic Compare and swap routines.
**
** Description:
** These routines/macros implement atomic compare and swap for four 
** byte regions, as well as a "pointer" sized region, which may be
** either 4 or 8 bytes depending on the platform.   These routines are
** used where we need to handle contended updates, but cannot block.
** (E.g. updating a counter or a singlely linked lists from a signal 
** handler/AST which might also be updated from other threads.
** 
** An example usage might be:
** 
** {
** 	i4 oldval;
** 
** 	do
** 	{
** 	    oldval = volatile_shared_counter;
** 	} while ( CScas4( &volatile_shared_counter, oldval, oldval + 1 ) );
** }
**
** History:
**	15-oct-2003 (devjo01)
**	    Initial inclusion.
**	18-nov-2003 (devjo01)
**	    Rename routines to highlight parameter change in which
**	    "oldval" is now passed by value, not reference.  Dropped
**	    unused eight byte C&S routine.
**	16-Jun-2006 (kschendel)
**	    a64_lnx has extern cas now (same as a64_sol).
*/

# if defined(ibm_lnx)

    /* IBM S390 Linux */
    static __inline__ int CScas( volatile int *target, int oldval, int newval )
    {
	int		    retval;

	__asm__ __volatile__ ( " l 0,0(0,%1)\n"
			       " cs 0,%3,0(%2)\n"
			       " ipm %0\n"
			       " srl %0,28\n"
			     : "=&d" (retcode)
			     : "a"(oldval), "a"(target), "d"(newval)
			     : "0", "cc" );
	return retval;
    }


#   define CScas4(t,o,n) ((i4)CScas((volatile int *)(t), (int)(o), (int)(n)))
#   define CScasptr(t,o,n) ((i4)CScas((volatile int *)(t), (int)(o), (int)(n)))

# elif (defined(int_lnx) || defined(int_rpl)) && !defined(a64_lnx)

    /* Intel based LINUX */
    static __inline__ int CScas( volatile int *target, int oldval, int newval )
    {
	char                retval;

	__asm__ __volatile__ ( "lock ; cmpxchgl %1, %2\n\t"
                           "setne %0\n\t"
		: "=q"(retval)
		: "q"(newval), "m"(*target), "a"(oldval)
		: "memory" );
	return (int)retval;
    }

#   define CScas4(t,o,n) ((i4)CScas((volatile int *)(t), (int)(o), (int)(n)))
#   define CScasptr(t,o,n) ((i4)CScas((volatile int *)(t), (int)(o), (int)(n)))

# elif defined(axp_osf) || defined(sparc_sol) || defined(a64_sol) || defined(a64_lnx)

    /* Implementation of these routines are in platform's csll.XXX.asm file */
    extern i4 CScas4( i4 *target, i4 oldval, i4 newval );
    extern i4 CScasptr( PTR *target, PTR oldval, PTR newval );

# elif defined(VMS)
/*
** Perform and atomic compare and swap on an aligned four byte
** region.  Macro returns OK (0) if update was performed,
** otherwise, reread oldval, recalc newval, and try again.
*/
# define CScas4(target,oldval,newval) \
   (0 == __CMP_STORE_LONG( (volatile void *)(target), \
    (int)(oldval), (int)(newval), (volatile void *)(target) ))

/*
** Perform atomic C&S op on pointer size arg. (Change builtin func.
*/
#if defined(__INITIAL_POINTER_SIZE) && (__INITIAL_POINTER_SIZE == 64)
# define CScasptr(target,oldval,newval) \
 (0 == __CMP_STORE_QUAD( (volatile void *)(target), \
 (int)(oldval), (int)(newval), (volatile void *)(target) ))
#else
# define CScasptr(target,oldval,newval) \
   (0 == __CMP_STORE_LONG( (volatile void *)(target), \
    (int)(oldval), (int)(newval), (volatile void *)(target) ))
#endif

# else

# error "BUILD ERROR: Need to provide Compare & Swap Routines"

# endif

#endif /* conf_CAS_ENABLED */


/*}
** Name: CS_SEMAPHORE - Semaphore for CS module manipulation
**
** Description:
**      This structure is manipulated by the CS module semaphore manipulation
**      routines to control the internal concurrencly on software objects.
**      The structure itself maintains a list of those sessions who are waiting
**      for the semaphore.  Thus trashing a semaphore will probably bring down
**      the entire server.
**
**      A semaphore may be stored in shared memory and thus be accessible by
**      multiple processes.
**
** History:
**      10-Nov-1986 (fred)
**          Created.
**       6-Feb-1989 (rogerk)
**          Added cs_type and cs_pid fields for shared memory semaphores.
**	24-jul-1989 (rogerk)
**	    Added cs_test_set field for cross process semaphores.
**	23-mar-1990 (mikem)
**	    Added some performance stat fields to the CS_SEMAPHORE structure:
**	    cs_cnt_sem, cs_long_cnt, cs_ms_long_et, cs_pad.
**	4-jul-92 (daveb)
**	  Add SPBLK to CS_SCB, CS_SEMAPHORE and CS_CONDITION
**	  structures, and add name field to semaphore for
**	  CSn_semaphore.  This will allow simple MO objects for
**	  getting at sessions, sems, and conditions. The downside is
**	  that it requires inclusion of <sp.h> every place that <cs.h>
**	  is now included.  We'll punt that for now by including it
**	  here, and making sure that <sp.h> is gated.
**	9-jul-92 (daveb)
**	  Take out spblk from the semaphore; that can't work as the
**	  sem may be in shared memory at different locations.  Instead,
**	  add some administrative fields that let us do some sanity
**	  checking, and identify the semaphore.
**	20-sep-93 (mikem)
**	    Added cs_smstatistics structure to the CS_SEMAPHORE structure to
**	    keep semaphore contention statistics on a per semaphore basis.
**	 6-Jun-95 (jenjo02)
**	    Changed obsolete cs_cnt_sem, cs_long_cnt, cs_ms_long_et to
**	    cs_pad, added cs_excl_waiters.
**	08-jul-1997 (canor01)
**	    Add known sem list.
**	12-Nov-1999 (jenjo02)
**	    Wrap cs_sem_list with xCL_NEED_SEM_CLEANUP, the only condition
**	    under which the list is used.
**	    Remove obsolete cs_list. 
**	    Remove dubious cs_sem_init_pid, cs_sem_init_addr.
**	    Replace CS_SID cs_sid with CS_THREAD_ID cs_thread_id (OS_THREADS).
**	    Shrunk cs_value, cs_type from i4 to i2.
**      19-apr-2002 (xu$we01)
**          Previous "fix" breaks DGI build, because xCL_NEED_SEM_CLEANUP
**          symbol is not in scope for all facilities that reference
**          CS_SEMAPHORE.   This leads to different storage allocation
**          sizes and field offsets for instances of CS_SEMAPHORE declared
**          in for instance SCF, as compared to CS.   Simply including
**          "clconfig.h" within "clnormal.h" is not possible, since as
**          a CL private header files, it will not always be in the
**          include file search path for all facilities.   Since this code is
**          only required for DG UNIX flavors, we are putting in the semi-
**          dirty hack of making the inclusion of cs_sem_list dependent on
**          dgi_us5, and dg8_us5 instead of xCL_NEED_SEM_CLEANUP.
*/
struct _CS_SEMAPHORE
{
#if defined(VMS) || defined(dgi_us5) || defined(dg8_us5)
    QUEUE	    cs_sem_list;	/* known sem list - must be first */
#endif /* VMS || dgi_us5 || dg8_us5 */

    i2              cs_type;            /* semaphore type */
#define                 CS_SEM_SINGLE   0   /* Normal Semaphore */
#define                 CS_SEM_MULTI    1   /* Shared Memory Semaphore */
    i2              cs_value;           /* The current value of the semaphore */
    i4	            cs_count;		/* number of folks
					** sharing this semaphore.
					** By convention, when this number is
					** zero, the semaphore is held
					** exclusively
					*/ 
    CS_SCB	    *cs_owner;		/* the owner of the semaphore */
    CS_SCB	    *cs_next;		/* those folks awaiting this one */
#if defined(VMS)
    CS_SEMAPHORE    *cs_list;		/* list of sem's owned */
#endif
    i4         	    cs_pid;             /* process id of holder if semaphore
                                        ** is CS_SEM_MULTI type */
/* BOAMA01, 30-jul-1996  Added 2 flds to resolve BBSSI/BBCCI interlock failure*/
    i4              cs_pad;		/* Protects next long's bit 0 */
    CS_ASET	    cs_test_set;	/* Field to use for cross-process
					** spin-lock semaphores - this field
					** is used instead of cs_value. */
    char	    cs_sem_name[CS_SEM_NAME_LEN];	/* for CSn_semaphore */

    i4	    	    cs_sem_scribble_check; /* Sanity check */
# define CS_SEM_LOOKS_GOOD	CV_C_CONST_MACRO('I','S','E','M')
# define CS_SEM_WAS_REMOVED	CV_C_CONST_MACRO('D','S','E','M')

#if defined(VMS)
    CS_SEMAPHORE    *cs_sem_init_addr;	/* address in CSi_sem process */
    i4		    cs_sem_init_pid;	/* pid of CSi_sem process */
#endif

# ifdef OS_THREADS_USED
    CS_SYNCH        cs_mutex;           /* synchronization primitive */
    CS_COND         cs_cond;            /* wake-up event */
    CS_THREAD_ID    cs_thread_id;       /* actual thread_id of sem owner */
    i4              cs_cond_waiters;    /* count of those waiting on cs_cond */
# ifdef SIMULATE_PROCESS_SHARED
    CS_CP_COND      cs_cp_cond;         /* cond variable for SPS */
# endif
# endif /* OS_THREADS_USED */
    i4	    	    cs_excl_waiters;	/* if OS threads:
					**    true if any EXCL waiters on sem
					** else
					**    count of EXCL waiters on sh sem */
    struct  _CS_EXT_SMSTAT
    {
        u_i4       cs_smsx_count;  /* collisions on shared vs. exclusive */
        u_i4       cs_smxx_count;  /* exclusive vs. exclusive */
        u_i4       cs_smcx_count;  /* collisions on cross-process */
        u_i4       cs_smx_count;   /* # requests for exclusive */
        u_i4       cs_sms_count;   /* # requests for shared */
        u_i4       cs_smc_count;   /* # requests for cross-process */
        u_i4       cs_smcl_count;  /* # spins waiting for cross-process */
        u_i4       cs_smsp_count;  /* # "single-processor" swusers */
        u_i4       cs_smmp_count;  /* # "multi-processor" swusers */
        u_i4       cs_smnonserver_count;
# ifdef OS_THREADS_USED
        u_i4       cs_sms_hwm;     /* hwm of sharers */
# endif /* OS_THREADS_USED */
                                        /* # "non-server" naps */
    }               cs_smstatistics;    /* statistics for semaphores */

};

/*}
** Name: CS_SEM_STATS - Structure into which semaphore statistics
**			may be returned.
**
** Description:
**	This structure is filled with selected statistics in response 
**	to a CSs_semaphore() function call.
**
** History:
**      07-Dec-1995 (jenjo02)
**          Created.
*/
struct _CS_SEM_STATS
{
    char	    name[CS_SEM_NAME_LEN]; /* semaphore name */
    i4		    type;		/* semaphore type,
					   CS_SEM_SINGLE or
					   CS_SEM_MULTI */
    u_i4	    excl_requests;	/* count of exclusive requests */
    u_i4	    share_requests;	/* count of share requests */
    u_i4	    excl_collisions;	/* exclusive collisions */
    u_i4	    share_collisions;	/* share collisions */
# ifdef OS_THREADS_USED
    u_i4       share_hwm;          /* share high-water-mark */
# endif /* OS_THREADS_USED */
/* The following fields apply to CS_SEM_MULTI types only: */
    u_i4	    spins;		/* number of cross-process spins */
    u_i4	    redispatches;	/* number of redispatches */
};


/*}
** Name: CS_INFO_CB - Server information block
**
** Description:
**      This block is passed to the server startup function to pass
**      to inform the server of various facts in which it may be 
**      interested.
**
** History:
**      15-jan-1987 (fred)
**          Created.
**	26-sep-1988 (rogerk)
**	    Added fields specifying CS thead priorities.
**	31-oct-1988 (rogerk)
**	    Added cso_facility to CS_OPTIONS struct.
**      25-nov-88 (eric)
**          added cso_strvalue, initially for registation of database names.
**	27-Sep-90 (anton)
**	    Can not include gcccl.h to determine port name length.
**	    GCC_L_PORT is the wrong symbol anyways.  We eagerly
**	    await corrections for this problem.  (GC spec)
**	09-jan-91 (ralph)
**	    added cso_float to pass in floating point numbers
**  03-Jul-97 (radve01)
**      Several info added to the end CS_INFO_CB structure to facilitate
**      debugging.
*/

typedef struct _CS_INFO_CB
{
    i4              cs_scnt;            /* Session count */
    i4              cs_ascnt;           /* active session count */
    i4		    cs_min_priority;	/* Min priority for server thread */
    i4		    cs_max_priority;	/* Max priiority for server thread */
    i4		    cs_norm_priority;	/* Priority used for user sessions */
    char	    cs_name[64]; /* port name of the server */
    PTR		    cs_svcb;		/* filled in by initiation */
    CS_SEMAPHORE    *cs_mem_sem; /* semaphore used to protect MEget_pages */
	/*
    ** Additional diagnostic info will be transferred to the server block
	*/
	i4    sc_main_sz;   /* Size of SCF Control Block(SC_MAIN_CB,Sc_main_cb) */
	i4    scd_scb_sz;   /* Size of Session Control Block(SCD_SCB) */
	i4    scb_typ_off;  /* The session type offset inside session cb. */
	i4    scb_fac_off;  /* The current facility offset inside session cb. */
	i4    scb_usr_off;  /* The user-info off inside session control block */
	i4    scb_qry_off;  /* The query-text offset in session control block */
	i4    scb_act_off;  /* The current activity offset in session cb. */
	i4    scb_ast_off;  /* The alert(event) state offset in session cb. */
}   CS_INFO_CB;

# ifdef OS_THREADS_USED
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
**      10-Feb-2009 (smeke01) b119586 
**          Added OS_THREADS_USED to fix a build failure on the 
**          non-multithreaded build version of VMS.
*/
typedef struct _CS_THREAD_STATS_CB
{
    /* input parameters: process/thread identifiers */
    CS_OS_PID		cs_os_pid;	/* OS process id. */
    CS_OS_TID		cs_os_tid;	/* OS thread id. */
    CS_THREAD_ID	cs_thread_id;	/* thread library thread-id */
    /* output parameters: stats fields */
    i4		cs_usercpu;		/* OS cpu usage for thread (user portion) */
    i4		cs_systemcpu;	/* OS cpu usage for thread (system portion) */
    /* input parameter: select stats required for retrieval */ 
    i4		cs_stats_flag;	/* OR'd binary flag  */
#define CS_THREAD_STATS_CPU 0x00000001	/* Retrieve user & system CPU stats for the thread */
}   CS_THREAD_STATS_CB;
# endif

/*
**  Function Prototypes 
*/

/* CS_accept_connect() returns TRUE if the server can handle another
** connection request. If the server has the maximum number of threads
** or is in the middle of shutting down, then it will return FALSE
*/
FUNC_EXTERN bool	CSaccept_connect(
	void
);

FUNC_EXTERN void CS_detach_scb(CS_SCB *);
FUNC_EXTERN void CS_swuser(void);
FUNC_EXTERN void CS_update_cpu(i4 *, i4 *);
FUNC_EXTERN void CSms_thread_nap(i4);


/*
** Enum expansion for _DEFINE definitions above.
*/
#define _DEFINESEND

#define _DEFINE(s,v) s = v,
enum cs_state_enums { CS_STATE_ENUMS cs_state_dummy };
GLOBALCONSTREF char cs_state_names[];
enum cs_mask_masks { CS_MASK_MASKS cs_mask_dummy };
GLOBALCONSTREF char cs_mask_names[];
#ifdef OS_THREADS_USED
enum event_state_masks { EVENT_STATE_MASKS event_state_dummy };
GLOBALREF const char event_state_mask_names[];
#endif /*OS_THREADS_USED*/
#undef _DEFINE


#undef _DEFINESEND

#endif /* CSNORMAL_H */
