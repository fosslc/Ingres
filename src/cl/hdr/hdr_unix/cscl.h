/*
** Copyright (c) 2004 Ingres Corporation
*/

/**CL_SPEC
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
**	13-nov-90 (jkb)
**	    Move previous content of this file to csnormal.h so the correct
**	    version of the header file can be included for both mct and
**	    non-mct installations
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	03-jun-1996 (canor01)
**	    Preliminary changes to support operating system threads.
**	    Merge back in duplicated lines from csmt.h and csnormal.h.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**	    mv csmt.h into csnormal.h
**	20-may-1997 (shero03)
**	    Add xDev_TRACE section
**	29-oct-1997 (canor01)
**	    Add cs_stkcache to CS_CB.
**	03-Dec-1997 (jenjo02)
**	    Added cs_get_rcp_pid to CS_CB.
**	04-mar-1998 (canor01)
**	    Add cs_scbattach and cs_scbdetach members.
**	25-jun-98 (stephenb)
**	    Add define for CS_CTHREAD_EVENT
**	11-jan-1999 (canor01)
**	    Move a duplicate message definition.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-sep-2002 (devjo01)
**	    Added cs_format_lkkey to CS_CB.
**	04-Oct-2002 (hanch04)
**	    Added E_CL2557_CS_INTRN_THRDS_NOT_SUP
**	10-May-2004 (jenjo02)
**	    Added CS_KILL_EVENT to kill a query (SIR 110141).
**	10-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/

# ifndef CS_H_INCLUDED
# define CS_H_INCLUDED

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _CS_SCB CS_SCB;
typedef struct _CS_SEMAPHORE	CS_SEMAPHORE;
typedef struct _CS_CONDITION	CS_CONDITION;
typedef struct _CS_SEM_STATS	CS_SEM_STATS;
struct _LK_LOCK_KEY;

/*}
** Name: CS_SCB_Q - Que entry for the session control block
**
** Description:
**      This is simply a structure containing the standard queue
**      header element for session control blocks.  Building a
**      standard structure for this reduces name space problems
**      as well as guaranteeing correct usage.
**
** History:
**      27-Oct-1986 (fred)
**          Created.
[@history_template@]...
*/
typedef struct _CS_SCB_Q
{
    CS_SCB          *cs_q_next;         /* moving forward . . . */
    CS_SCB          *cs_q_prev;         /* as well as backwards */
}   CS_SCB_Q;

/*
**  Defines of other constants.
*/
#define			CS_MAXNAME      32	/* DB_SCHEMA_MAXNAME */

/* length of a semaphore name - spec-ed to be 32 at present */

# define CS_SEM_NAME_LEN	CS_MAXNAME


/* Common Unix and VMS definitions.  (Windows isn't playing at present) */

# include <csnormal.h>


#if defined(PERF_TEST) || defined(xDEV_TRACE)
/*
** defines for performance profiling (facility ID's are the same as in
** iicommon.h just for ease of use)
*/
#define                 CS_CLF_ID           1
#define                 CS_ADF_ID           2
#define                 CS_DMF_ID           3
#define                 CS_OPF_ID           4
#define                 CS_PSF_ID           5
#define                 CS_QEF_ID           6
#define                 CS_QSF_ID           7
#define                 CS_RDF_ID           8
#define                 CS_SCF_ID           9
#define                 CS_ULF_ID           10
#define                 CS_DUF_ID           11
#define                 CS_GCF_ID           12
#define                 CS_RQF_ID           13
#define                 CS_TPF_ID           14
#define                 CS_GWF_ID           15
#define                 CS_SXF_ID           16

#define                 CS_MAX_FACILITY     16
#endif /* PERF_TEST or xDEV_TRACE */

/*
**      The following constants are used to allow the called
**      thread to determine how it is being called, and to instruct
**      the dispatcher how it needs to be called next.
*/
#define                 CS_INITIATE     0x1
						/* Given to the processing 
						** routine to indicate that
						** a new thread is to be
						** initiated.
						** Never returned to the
						** dispatcher.
						*/
#define                 CS_INPUT        0x2
						/*
						** Given to processing routine
						** to indicate that input has 
						** been supplied from the 
						** partner.  Returned to the
						** dispatcher to indicate that
						** more input is needed.
						*/
#define                 CS_OUTPUT       0x3
						/*
						** Given to the processing 
						** routine to indicate that
						** it is expected to be 
						** providing more output (i.e.
						** it last returned CS_OUTPUT.
						** Returned to the dispatcher to
						** indicate that the dispatcher
						** should output the supplied
						** buffer and call the 
						** processing routine back.
						*/
#define			CS_EXCHANGE	0x5
						/*
						** Never given to a processing
						** routine.
						** Returned to the dispatcher
						** to indicate that the 
						** dispatcher should output
						** the supplied buffer, and
						** await an input message from
						** the partner.
						*/
#define                 CS_TERMINATE    0x4	
						/*
						** Given to the processing
						** routine to indicate that
						** it should terminate the
						** current thread.
						** Returned to the dispatcher
						** to indicate that the thread
						** is to be terminated, and that
						** the dispatcher should call 
						** back the processing routine
						** with the CS_TERMINATE
						** operation code.
						*/
#define			CS_READ		0x10	/* read requested, stack removed
						*/
/*
**  The following are the definitions of errors which can be returned
**  from the Control System call.  Some are accompanied by an associated
**  host operating system error.
*/

#define		E_CS0001_INVALID_STATE		(E_CL_MASK + E_CS_MASK + 0x0001)
#define		E_CS0002_SCB_ALLOC_FAIL		(E_CL_MASK + E_CS_MASK + 0x0002)
#define		E_CS0003_NOT_QUIESCENT		(E_CL_MASK + E_CS_MASK + 0x0003)
#define		E_CS0004_BAD_PARAMETER		(E_CL_MASK + E_CS_MASK + 0x0004)
#define		E_CS0006_TIMER_ERROR		(E_CL_MASK + E_CS_MASK + 0x0006)
#define		E_CS0007_RDY_QUE_CORRUPT	(E_CL_MASK + E_CS_MASK + 0x0007)
#define		E_CS0008_INTERRUPTED		(E_CL_MASK + E_CS_MASK + 0x0008)
#define		E_CS0009_TIMEOUT		(E_CL_MASK + E_CS_MASK + 0x0009)
#define		E_CS000A_NO_SEMAPHORE		(E_CL_MASK + E_CS_MASK + 0x000A)
#define		E_CS000C_INVALID_MODE		(E_CL_MASK + E_CS_MASK + 0x000C)
#define		E_CS000D_INVALID_RTN_MODE	(E_CL_MASK + E_CS_MASK + 0x000D)
#define		E_CS000E_SESSION_LIMIT		(E_CL_MASK + E_CS_MASK + 0x000E)
#define		E_CS000F_REQUEST_ABORTED	(E_CL_MASK + E_CS_MASK + 0x000F)
#define		E_CS0010_QUANTUM_INVALID	(E_CL_MASK + E_CS_MASK + 0x0010)
#define		E_CS0011_BAD_CNX_STATUS		(E_CL_MASK + E_CS_MASK + 0x0011)
#define		E_CS0012_BAD_ADMIN_SCB		(E_CL_MASK + E_CS_MASK + 0x0012)
#define		E_CS0013_CORRUPT_ADMIN_SCB	(E_CL_MASK + E_CS_MASK + 0x0013)
#define		E_CS0014_ESCAPED_EXCPTN		(E_CL_MASK + E_CS_MASK + 0x0014)
#define		E_CS0015_FORCED_EXIT		(E_CL_MASK + E_CS_MASK + 0x0015)
#define		E_CS0016_SYSTEM_ERROR		(E_CL_MASK + E_CS_MASK + 0x0016)
#define		E_CS0017_SMPR_DEADLOCK		(E_CL_MASK + E_CS_MASK + 0x0017)
#define		E_CS0018_NORMAL_SHUTDOWN	(E_CL_MASK + E_CS_MASK + 0x0018)
#define		E_CS0019_INVALID_READY		(E_CL_MASK + E_CS_MASK + 0x0019)
#define		E_CS001A_STACK_DEAL_FAIL	(E_CL_MASK + E_CS_MASK + 0x001A)
#define		E_CS001B_IDLE_RESUME		(E_CL_MASK + E_CS_MASK + 0x001B)
#define		E_CS001C_TERM_W_SEM		(E_CL_MASK + E_CS_MASK + 0x001C)
#define		E_CS001D_NO_MORE_WRITES		(E_CL_MASK + E_CS_MASK + 0x001D)
#define		E_CS001E_SYNC_COMPLETION	(E_CL_MASK + E_CS_MASK + 0x001E)
#define		E_CS001F_CPUSTATS_WASSET	(E_CL_MASK + E_CS_MASK + 0x001F)
#define		E_CS0024_NON_NUMERIC_ARG	(E_CL_MASK + E_CS_MASK + 0x0024)
#define		E_CS0025_NUMERIC_ARG		(E_CL_MASK + E_CS_MASK + 0x0025)
#define		E_CS0026_POSITIVE_VALUE		(E_CL_MASK + E_CS_MASK + 0x0026)
#define		E_CS0029_SEM_OWNER_DEAD		(E_CL_MASK + E_CS_MASK + 0x0029)
#define		E_CS002A_FLOAT_ARG		(E_CL_MASK + E_CS_MASK + 0x002A)
#define		E_CS002B_NONNEGATIVE_VALUE	(E_CL_MASK + E_CS_MASK + 0x002B)
#define		E_CS002C_FRACTION_VALUE		(E_CL_MASK + E_CS_MASK + 0x002C)
#define		E_CS002D_CS_DEFINE		(E_CL_MASK + E_CS_MASK + 0x002D)
#define		E_CS002E_CS_SHORT_FDS		(E_CL_MASK + E_CS_MASK + 0x002E)
#define		E_CS002F_NO_FREE_STACKS		(E_CL_MASK + E_CS_MASK + 0x002F)

#define		E_CS0030_CS_PARAM		(E_CL_MASK + E_CS_MASK + 0x0030)

#define		E_CS0035_GETPWNAM_FAIL		(E_CL_MASK + E_CS_MASK + 0x0035)
#define		E_CS0036_SETEUID_FAIL		(E_CL_MASK + E_CS_MASK + 0x0036)
#define		E_CS0037_MAP_SSEG_FAIL		(E_CL_MASK + E_CS_MASK + 0x0037)
#define		E_CS0038_CS_EVINIT_FAIL		(E_CL_MASK + E_CS_MASK + 0x0038)

#define		E_CS0040_ACCT_LOCATE		(E_CL_MASK + E_CS_MASK + 0x0040)
#define		E_CS0041_ACCT_OPEN		(E_CL_MASK + E_CS_MASK + 0x0041)
#define		E_CS0042_ACCT_WRITE		(E_CL_MASK + E_CS_MASK + 0x0042)
#define		E_CS0043_ACCT_RESUMED		(E_CL_MASK + E_CS_MASK + 0x0043)
#define         E_CS0046_CS_NO_START_PROF       (E_CL_MASK + E_CS_MASK + 0x0046)

#define		E_CS0050_CS_SEMOP_GET		(E_CL_MASK + E_CS_MASK + 0x0050)
#define		E_CS0051_CS_MSET		(E_CL_MASK + E_CS_MASK + 0x0051)
#define		E_CS0052_CS_SLEEP_BOTCH		(E_CL_MASK + E_CS_MASK + 0x0052)
#define		E_CS0053_CS_SEMOP_RELEASE	(E_CL_MASK + E_CS_MASK + 0x0053)
#define		E_CS0054_CS_MCLEAR		(E_CL_MASK + E_CS_MASK + 0x0054)
#define		E_CS0055_CS_WAKEUP_BOTCH	(E_CL_MASK + E_CS_MASK + 0x0055)

#define		E_CL2557_CS_INTRN_THRDS_NOT_SUP (E_CL_MASK + E_CS_MASK + 0x0057)

#define		E_CS00F9_2STK_OVFL		(E_CL_MASK + E_CS_MASK + 0x00F9)
#define		E_CS00FA_UNWIND_FAILURE		(E_CL_MASK + E_CS_MASK + 0x00FA)
#define		E_CS00FB_UNDELIV_AST		(E_CL_MASK + E_CS_MASK + 0x00FB)
#define		E_CS00FC_CRTCL_RESRC_HELD	(E_CL_MASK + E_CS_MASK + 0x00FC)
#define		E_CS00FD_THREAD_STK_OVFL	(E_CL_MASK + E_CS_MASK + 0x00FD)
#define		E_CS00FE_RET_FROM_IDLE		(E_CL_MASK + E_CS_MASK + 0x00FE)
#define		E_CS00FF_FATAL_ERROR		(E_CL_MASK + E_CS_MASK + 0x00FF)


/*
**      Types of commands and modifiers from the monitor session
**
*/
#define                 CS_CLOSE        0x0001
#define                 CS_COND_CLOSE   0x0002
#define                 CS_KILL         0x0003
#define			CS_CRASH	0x0004
#define			CS_START_CLOSE	0x0005

/*
**	External Event Definitions
*/

#define			CS_THREAD_EVENT	0x0000
#define			CS_SERVER_EVENT	0x0100

#define			CS_ATTN_EVENT	    0x0001
#define			CS_RCVR_EVENT	    0x0002
#define			CS_SHUTDOWN_EVENT   0x0004
#define                 CS_NOTIFY_EVENT     0x0008
#define                 CS_REMOVE_EVENT     0x0010
#define			CS_CTHREAD_EVENT    0x0020
#define			CS_KILL_EVENT       0x0040

/*
**	Options for CSalter_session calls.
*/

#define			CS_AS_CPUSTATS	    1

#define			CS_AS_PRIORITY	    2	/* Change session priority*/


/*}
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
** History:
**      24-Oct-1986 (fred)
**          Initial Definition.
**      14-Mch-1996 (prida01)
**          Add cs_diag call.
**	29-oct-1997 (canor01)
**	    Add cs_stkcache.
**	03-Dec-1997 (jenjo02)
**	    Added cs_get_rcp_pid.
**	05-sep-2002 (devjo01)
**	    Added cs_format_lkkey to CS_CB.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Refine CS prototypes.
*/
typedef struct _CS_CB
{
    i4		cs_scnt;		/* Number of sessions */
    i4		cs_ascnt;		/* nbr of active sessions */
    i4		cs_stksize;		/* size of stack in bytes */
    bool	cs_stkcache;		/* enable stack caching w/threads */
    STATUS	(*cs_scballoc)(CS_SCB **, void *, i4);	/* Routine to allocate SCB's */
    STATUS	(*cs_scbdealloc)(CS_SCB *); /* Routine to dealloc  SCB's */
    STATUS	(*cs_saddr)(void *, i4); /* Routine to await session requests */
    STATUS	(*cs_reject)(void *, STATUS); /* how to reject connections */
    VOID	(*cs_disconnect)(CS_SCB *); /* how to dis- connections */
    STATUS	(*cs_read)(CS_SCB *, i4);	/* Routine to do reads */
    STATUS	(*cs_write)(CS_SCB *, i4);	/* Routine to do writes */
    STATUS	(*cs_process)(i4, CS_SCB *, i4 *); /* Routine to do major processing */
    STATUS	(*cs_attn)(i4, CS_SCB *); /* Routine to process attn calls */
    VOID	(*cs_elog)(i4, CL_ERR_DESC *, i4, ...);	/* Routine to log errors */
    STATUS	(*cs_startup)(CS_INFO_CB *);	/* startup the server */
    STATUS	(*cs_shutdown)(void);	/* shutdown the server */
    STATUS	(*cs_format)(CS_SCB *,char *,i4,i4);	/* format scb's */
    STATUS	(*cs_diag)(void *);	/* Diagnostics for server */
    STATUS	(*cs_facility)(CS_SCB *); /* return current facility */
    i4		(*cs_get_rcp_pid)(void); /* return RCP's pid */
    VOID	(*cs_scbattach)(CS_SCB *); /* Attach thread control block to MO */
    VOID	(*cs_scbdetach)(CS_SCB *); /* Detach thread control block */
    char	*(*cs_format_lkkey)(struct _LK_LOCK_KEY *,char *); /*Format an LK_LOCK_KEY for display */
#define		CS_NOCHANGE     -1	/* no change to this parm */
}   CS_CB;


/* Export function prototypes that are platform dependent */
FUNC_EXTERN void CS_get_critical_sems(void);
FUNC_EXTERN void CS_rel_critical_sems(void);
FUNC_EXTERN void CS_check_dead(void);

# endif /* CS_H_INCLUDED */
