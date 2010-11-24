/*
** Copyright (c) 1987, 2008 Ingres Corporation
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
**	28-feb-1988 (rogerk)
**	    Added cs_pid field to CS_SCB.  Added cs_type, cs_pid to CS_SEMAPHORE
**	    structure for cross process semaphores.
**	 5-sep-1989 (rogerk)
**	    Added CS_NOSWAP status mask value.
**	14-sep-89 (paul)
**	    Added new CS interrupt type for alerters
**	29-Dec-1989 (anton)
**	    Adding CSswitch macro - cl interface change for UNIX quantum
**	    support bug 6143
**	08-aug-90 (ralph)
**	    added cso_float to pass in floating point numbers
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		Added CS_GWFIO_MASK for use by Gateways to indicate to CSsuspend
**		that a session is waiting on Gateway I/O. Also added cs_gwfio
**		stat to the SCB. This statistic is used to count the number of
**		GWFIO suspensions that a thread takes.
**	15-jan-91 (ralph)
**	    Make cso_float a simple structure element rather than a union member
**	04-feb-91 (neil)
**	    Added external def'n of CSfind_scb.
**	06-Dec-90 (anton)
**	    Added cs_mem_sem to CS_INFO_CB to return the MEget_pages
**	    semaphore to the CL
**	    Increased the allowable size of a server name
**	07-Jan-91 (anton)
**	    Add support for CS_CONDITIONs
**	05-nov-91 (ralph)
**	    6.4->6.5 merge
**	    28-jan-91 (seputis)
**		added FUNC_EXTERN for CS_CONDITIONs
**	    18-aug-91 (ralph)
**		added CS_REMOVE_EVENT for IIMONITOR REMOVE
**    12-Feb-92 (daveb)
**        Add cs_clieint_type field to allow clients to type-tag their
**        scb's that may have a CS_SCB as a common initial member.
**        Part of fix for bug 38056.  *
**    29-sep-92 (kwatts)
**	  Add dummy CSn_semaphore until such time as a real
**	  semaphore naming scheme for VAX is defined.
**	20-oct-1992 (bryanp)
**	    Add definition for E_CS0029_SEM_OWNER_DEAD.
**	4-jul-92/28-oct-92 (daveb)
**	  Add SPBLK to CS_SCB, CS_SEMAPHORE and CS_CONDITION
**	  structures, and add name field to semaphore for
**	  CSn_semaphore.  This will allow simple MO objects for
**	  getting at sessions, sems, and conditions. The downside is
**	  that it requires inclusion of <sp.h> every place that <cs.h>
**	  is now included.  We'll punt that for now by including it
**	  here, and making sure that <sp.h> is gated.
**	9-jul-92/28-oct-92 (daveb)
**	  Take out spblk from the semaphore; that can't work as the
**	  sem may be in shared memory at different locations.  Instead,
**	  add some administrative fields that let us do some sanity
**	  checking, and identify the semaphore.
**	28-Oct-1992 (daveb)
**	    Remove dummy CSn_semaphore; it's really a function now.
**	    Add prototypes for CS external functions.
**	28-Oct-1992 (daveb)
**	    Add real n,a,d,r semaphore routines;
**	    Change CS_SEMAPHORE, CS_CONDITION and CS_SCB structures
**	    significantly for IMA monitoring.
**    18-sep-92 (seputis)
**	  Added CS_MAXNAME to define DB_MAXNAME dependencies
**	11-nov-92 (walt)
**		Alpha version.
**	17-nov-92 (walt)
**		Defined structure (CS_ITM) for getting system page size at runtime.
**		The definition is from section 2.4 (Obtaining the Page Size at Run
**		Time) of the Alpha "Compiling and Relinking" porting manual.
**	19-feb-93 (walt)
**		Merged diffs from latest RPLUS version into Alpha version.
**	1-Sep-93 (seiwald)
**	    CS option revamp: Removed CS_OPTIONS.
**	1-mar-94 (walt)
**	    Added some padding between async elements of the SCB because
**	    there's some clobbering going on during the interaction of AST
**	    and non-AST levels (just on Alpha).
**	18-mar-94 (walt)
**	    CSswitch is actually a function in csinterface.c on Alpha.  So
**	    remove its #define from here.  Also define ALLOWED_QUANTUM_COUNT,
**	    the number of quantum ticks that can accumulate before CSswitch
**	    (in csinterface.c) will switch out a thread.
**	01-feb-95 (wolf) 
**	    Added (RobF's) CS_AS_PRIORITY option.
**      01-mar-95 (albany)
**	    Added #pragmas and removed padding from tail-end of structure.
**	    This is to fix a bug which apparently strikes randomly, causing
**	    the server to hang.  A similar situation was solved in this
**	    manner in 6.4 AXP/VMS code.
**	20-jun-1995 (dougb)
**	    Cross integrate my change 274733 from ingres63p:
**	  31-may-95 (dougb)
**	    Add "volatile" to CS_SCB definition in an attempt to avoid
**	    CL_INVALID_READY problems.  We want to avoid any code holding part
**	    of this structure in registers when the asynchronous routine
**	    CSresume() happens to run.  Don't want to resort to compiling the
**	    entire system "/noopt" again.
**	25-sep-95 (dougb)
**	    Remove now-useless member_alignment pragmas.
**      04-oct-95 (duursma)
**	    Added cs_exsp field to CS_SCB.   Allows us to keep a (linked) list
**	    of exception handlers, one per session.
**	21-dec-95 (dougb)
**	    Undo my previous change (of 25-sep-95).
**	15-feb-1996 (duursma)
**	    Added CS_SEM_STATS as partial integration of UNIX change 423116.
**	    Also added CS_LGEVENT_MASK and CS_LKEVENT_MASK.
**	30-jul-1996 (boama01)
**	    Added 2 flds to middle of CS_SEMAPHORE struct to resolve BBSSI/
**	    BBCCI interlock failure for MULTI sems, permitting bit to be
**	    test/set/clear which can be realigned to any nearest long boundary.
**	    This should ONLY be necessary for Alpha (not VAX).
**      07-nov-1996 (lawst01)
**          CS_CB structure was changed to accomodate 2 add'l function ptrs
**          though not currently used in VMS, the source in scdmain.c
**          initializes one of them. They are included in the structure to
**          avoid compile errors.
**      29-jul-1997 (kinte01)
**          CS_CB structure was changed to accomodate 1 add'l function ptrs
**          (cs_sync_obj) that is now used in the new routine cssample.c. This
**          ptr was previously only used in Unix and not defined in VMS CL.
**      06-aug-1997 (teresak)
**          Cross-integrate changes from Unix CL for OS threads
**          5-dec-96 (stephenb)
**            Add defines for CS facility ID's (currently the same as DM facility
**            ID's in iicommon.h).
**          02-may-1997 (canor01)
**            Add cs_cs_mask to CS_SCB for session flags specific to CS.
**          20-may-1997 (shero03)
**            Add xDev_TRACE section
**          03-Jul-97 (radve01)
**            Several info added to the end CS_INFO_CB structure to facilitate
**            debugging.
**	31-Oct-1997 (kinte01)
**		A line was incorrectly wrapped causing compile problems
**              Added ifdef OS_THREADS_USED and CS_RWLOCK that was missing
**              from a previous integration that also caused compile problems
**	25-jun-98 (stephenb)
**	    Add define for CS_CTHREAD_EVENT
**	09-mar-1998 (kinte01)
**        Cross integrate change 433659 from vaxvms_1201 into AlphaVMS CL
**        to fix RSB-RSH deadlocks (bug 87910)
**	19-may-1998 (kinte01)
**	  Cross integrate Unix change 434579 into AlphaVMS CL
**        03-Dec-1997 (jenjo02)
**          Added cs_get_rcp_pid to CS_CB.
**	19-may-1998 (kinte01)
**	  Cross integrate Unix change 434579 into AlphaVMS CL
**        04-mar-1998 (canor01)
**          Add cs_scbattach and cs_scbdetach members.
**	20-may-1998 (kinte01)
**	  Cross integrate Unix change 435120 into AlphaVMS CL
**        09-Mar-1998 (jenjo02)
**          Moved CS_MAXPRIORITY, CS_MINPRIORITY, CS_DEFPRIORITY here
**          from csinternal.h
**        20-Mar-1998 (jenjo02)
**          Removed cs_access_sem for OS_THREADS. cs_event_sem alone does the
**          job nicely, protecting changes to cs_mask and cs_state.
**	20-may-1998 (kinte01)
**	  Cross integrate Unix change 435144 into AlphaVMS CL
**        31-mar-1998 (canor01)
**          Add CS_get_stackinfo() macro.
**	11-jun-1998 (kinte01)
**        Cross integrate change 435890 from oping12
**          19-May-1998 (horda03) X-Integrate change 427896
**             Add field diag_qry to _CS_SCB
**	01-jul-1998 (kinte01)
**	  Changed diag_qry to cs_diag_qry to comply with coding standards.
**	01-jul-1998 (kinte01)
**        Add definitions of CSTHREAD_ID, CS_get_thread_id(),
**        CS_thread_id_equal(a,b), and CS_thread_id_assign(a,b)
**	10-aug-1998 (kinte01)
**	  partially integrate change 423131 from Unix cl
**        07-Dec-1995 (jenjo02)
**          CSsuspend() calls which don't specify a "wait type" mask have 
**          always defaulted to "CS_LOG_MASK", skewing monitor and statistics
**          by incorrectly showing threads waiting on the LOG when
**          they're really just waiting on a timer. Real waits on the
**          log will now be specified by the explicit use of
**          CS_LOG_MASK.
**	16-Nov-1998 (jenjo02)
**	   o Cleaned up defines for ReaderWriter locks, which LK now uses.
**	   o Added CS_CPID structure, used by LG/LK.
**	   o Added new defines for CSsuspend event masks.
**	10-Mar-1999 (shero03)
**	    Added cs_random_seed.
**      22-jun-1999 (horda03)
**          Cross-integration of change 440975.
**          29-Jan-1999 (hanal04, horda03)
**             Added cs_excl_waiters to CS_SEMAPHORE in order to provide
**             support for shared access to cross process semaphores. b95062.
**	14-apr-2000 (devjo01)                    
**	    Cross change 442838 (b95607) from 1.2.
**	23-Jul-2000 (devjo01)
**	       Added CS_SEM_CP_CRITICAL for s101483.
**      19-Sep-2000 (horda03)
**          Do not use the MEMBER_ALIGNMENT prgmas if AXM_VMS defined.
**          Once Ingres is built solely with /MEMBER_ALIGNMENT the pragmas and
**          the AXM_VMS definition can be removed.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	13-mar-2001 (kinte01)
**	   Cross integrate Unix changes into AlphaVMS CL
**	   29-oct-1997 (canor01)
**	      Add cs_stkcache.
**	18-sep-2001 (kinte01)
**	   Add E_CS0037_MAP_SSEG_FAIL
**	12-nov-2001 (kinte01)
**	   add casts as needed per Unix.
**	28-aug-2003 (abbjo03)
**	    Remove CS_ITM (use VMS_ITEM_LIST_3 instead).
**      23-oct-2003 (devjo01)
**          Added cs_format_lkkey to CS_CB.  This is a handcross of applicable
**	    103715 changes from the UNIX CL.
**	23-oct-2003 (kinte01)
**	    Add typedef for CS_ASET
**	24-oct-2003 (kinte01)
**	    Add defines for CS_ACLR, CS_ISSET, CS_TAS, IICSresume_from_AST
**      18-Nov-2003 (devjo01)
**	    Add CScas4 & CScasptr to wrap built-in atomic compare and swap
**	    functions in a macro allowing parameters and return conventions
**	    matching that of all other platforms.
**	10-May-2004 (jenjo02)
**	    Added CS_KILL_EVENT to kill a query (SIR 110141).
**	21-dec-2004 (abbjo03)
**	    Include builtins.h needed by CScas4's __CMP_STORE_xxx instructions.
**	25-apr-2005 (devjo01)
**	    Correct CS_ISSET, CScas4 & CScasptr macros. Add IICSsuspend_for_AST.
**	04-nov-2005 (abbjo03)
**	    Include csnormal.h and remove redundant structure declarations.
**	26-nov-2008 (joea)
**	    Remove redundant OS_THREADS_USED/POSIX_THREADS macros that are in
**	    csnormal.h.  Add #define for E_CS0038.
**      12-Dec-2008 (stegr01)
**          change __CMP_STORE_LONG to __CMP_SWAP_LONG for itanium
**      09-mar-2010 (joea)
**          Add E_CL25F5_KPB_ALLOC_ERROR and E_CL25F6_MEM_STK_ALLOC_ERROR.
**      06-oct-2010 (joea)
**          Remove CScas4 and CScasptr macros since they're already defined in
**          csnormal.h.
*/


# ifndef CS_H_INCLUDED
# define CS_H_INCLUDED

/*
**  Forward and/or External typedef/struct references.
*/

typedef volatile struct _CS_SCB CS_SCB;
typedef struct _CS_SEMAPHORE	CS_SEMAPHORE;
typedef struct _CS_CONDITION	CS_CONDITION;
typedef struct _CS_SEM_STATS	CS_SEM_STATS;
typedef struct _CS_CPID  CS_CPID;
# define IICSresume_from_AST CSresume
# define IICSsuspend_for_AST CSsuspend

/*
**  Defines of other constants.
*/


#define                 CS_MAXNAME	32
						/* should be the same
						** as DB_MAXNAME in dbms.h */

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



/* length of a semaphore name - spec-ed to be at least 24 */

# define		CS_SEM_NAME_LEN	CS_MAXNAME

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
#define		E_CS000B_NO_FREE_STACKS		(E_CL_MASK + E_CS_MASK + 0x000B)
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
#define		E_CS0030_CS_PARAM		(E_CL_MASK + E_CS_MASK + 0x0030)
#define		E_CS0035_RSBRSH_DLOCK           (E_CL_MASK + E_CS_MASK + 0x0035)
#define		E_CS0037_MAP_SSEG_FAIL		(E_CL_MASK + E_CS_MASK + 0x0037)
#define		E_CS0038_CS_EVINIT_FAIL		(E_CL_MASK + E_CS_MASK + 0x0038)
#define         E_CS0040_ACCT_LOCATE            (E_CL_MASK + E_CS_MASK + 0x0040)
#define         E_CS0041_ACCT_OPEN              (E_CL_MASK + E_CS_MASK + 0x0041)
#define         E_CS0042_ACCT_WRITE             (E_CL_MASK + E_CS_MASK + 0x0042)
#define         E_CS0043_ACCT_RESUMED           (E_CL_MASK + E_CS_MASK + 0x0043)
#define         E_CS0046_CS_NO_START_PROF       (E_CL_MASK + E_CS_MASK + 0x0046)
#define         E_CS0050_CS_SEMOP_GET           (E_CL_MASK + E_CS_MASK + 0x0050)
#define         E_CS0051_CS_MSET                (E_CL_MASK + E_CS_MASK + 0x0051)
#define         E_CS0052_CS_SLEEP_BOTCH         (E_CL_MASK + E_CS_MASK + 0x0052)
#define         E_CS0053_CS_SEMOP_RELEASE       (E_CL_MASK + E_CS_MASK + 0x0053)
#define         E_CS0054_CS_MCLEAR              (E_CL_MASK + E_CS_MASK + 0x0054)
#define         E_CS0055_CS_WAKEUP_BOTCH        (E_CL_MASK + E_CS_MASK + 0x0055)
#define         E_CL25F5_KPB_ALLOC_ERROR        (E_CL_MASK + E_CS_MASK + 0x00F5)
#define         E_CL25F6_MEM_STK_ALLOC_ERROR    (E_CL_MASK + E_CS_MASK + 0x00F6)
#define		E_CS00F7_CSMBXCRE_ERROR 	(E_CL_MASK + E_CS_MASK + 0x00F7)
#define		E_CS00F8_CSMBXCRE_NOPRIV 	(E_CL_MASK + E_CS_MASK + 0x00F8)
#define		E_CS00F9_2STK_OVFL		(E_CL_MASK + E_CS_MASK + 0x00F9)
#define		E_CS00FA_UNWIND_FAILURE		(E_CL_MASK + E_CS_MASK + 0x00FA)
#define		E_CS00FB_UNDELIV_AST		(E_CL_MASK + E_CS_MASK + 0x00FB)
#define		E_CS00FC_CRTCL_RESRC_HELD	(E_CL_MASK + E_CS_MASK + 0x00FC)
#define		E_CS00FD_THREAD_STK_OVFL	(E_CL_MASK + E_CS_MASK + 0x00FD)
#define		E_CS00FE_RET_FROM_IDLE		(E_CL_MASK + E_CS_MASK + 0x00FE)
#define		E_CS00FF_FATAL_ERROR		(E_CL_MASK + E_CS_MASK + 0x00FF)

#define         E_CL2557_CS_INTRN_THRDS_NOT_SUP (E_CL_MASK + E_CS_MASK + 0x0057)

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
#define			CS_NOTIFY_EVENT	    0x0008
#define                 CS_REMOVE_EVENT     0x0010
#define			CS_CTHREAD_EVENT    0x0020
#define			CS_KILL_EVENT	    0x0040

/*
**	Options for CSalter_session calls.
*/

#define			CS_AS_CPUSTATS	    1
#define			CS_AS_PRIORITY      2   /* Change session priority */

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
**	19-may-1998 (kinte01)
**	  Cross integrate Unix change 434579 into AlphaVMS CL
**        03-Dec-1997 (jenjo02)
**          Added cs_get_rcp_pid to CS_CB.
**	19-may-1998 (kinte01)
**	  Cross integrate Unix change 434579 into AlphaVMS CL
**        04-mar-1998 (canor01)
**          Add cs_scbattach and cs_scbdetach members.
**	13-mar-2001 (kinte01)
**	   Cross integrate Unix changes into AlphaVMS CL
**	   29-oct-1997 (canor01)
**	      Add cs_stkcache.
**      23-oct-2003 (devjo01)
**          Added cs_format_lkkey to CS_CB.
*/


typedef struct _CS_CB
{
    i4              cs_scnt;            /* Number of sessions */
    i4              cs_ascnt;           /* nbr of active sessions */
    i4              cs_stksize;         /* size of stack in bytes */
    bool            cs_stkcache;        /* enable stack caching w/threads */
    STATUS          (*cs_scballoc)();   /* Routine to allocate SCB's */
    STATUS          (*cs_scbdealloc)(); /* Routine to dealloc  SCB's */
    STATUS	    (*cs_saddr)();	/* Routine to await session requests */
    STATUS	    (*cs_reject)();	/* how to reject connections */
    STATUS	    (*cs_disconnect)();	/* how to dis- connections */
    STATUS	    (*cs_read)();	/* Routine to do reads */
    STATUS	    (*cs_write)();	/* Routine to do writes */
    STATUS          (*cs_process)();    /* Routine to do major processing */
    STATUS	    (*cs_attn)();	/* Routine to process attn calls */
    VOID	    (*cs_elog)();       /* Routine to log errors */
    STATUS	    (*cs_startup)();	/* startup the server */
    STATUS	    (*cs_shutdown)();	/* shutdown the server */
    STATUS	    (*cs_format)();	/* format scb's */
    STATUS          (*cs_diag)();       /* Diagnostics for server */
    STATUS          (*cs_facility)();   /* return current facility */
    i4              (*cs_get_rcp_pid)();/* return RCP's pid */
    VOID            (*cs_scbattach)();  /* Attach thread control block to MO */
    VOID            (*cs_scbdetach)();  /* Detach thread control block */
    char           *(*cs_format_lkkey)();/*Format an LK_LOCK_KEY for display */
#define                 CS_NOCHANGE     -1	/* no change to this parm */
}   CS_CB;

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
*/
typedef struct _CS_SCB_Q
{
    CS_SCB          *cs_q_next;         /* moving forward . . . */
    CS_SCB          *cs_q_prev;         /* as well as backwards */
}   CS_SCB_Q;

#include <csnormal.h>


/*
**  Function Prototypes 
*/

FUNC_EXTERN	VOID	    CSget_sid();	/* fetch the session id */
FUNC_EXTERN	VOID	    CSget_scb();	/* fetch the scb */
FUNC_EXTERN	CS_SCB	    *CSfind_scb();	/* find "other thread's" scb */
FUNC_EXTERN	STATUS	    CSsuspend();	/* suspend current session */
#define                 CS_GWFIO_MASK       0x400000
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

FUNC_EXTERN	VOID	    CSresume();		/* resume session ast */
FUNC_EXTERN	VOID	    CSattn();		/* session interrupt ast */
FUNC_EXTERN	STATUS	    CSp_semaphore();
FUNC_EXTERN	STATUS	    CSv_semaphore();
FUNC_EXTERN	STATUS	    CScnd_init();
FUNC_EXTERN	STATUS	    CScnd_free();
FUNC_EXTERN	STATUS	    CScnd_wait();
FUNC_EXTERN	STATUS	    CScnd_signal();
FUNC_EXTERN	STATUS	    CScnd_broadcast();
FUNC_EXTERN	STATUS	    CScnd_name();
FUNC_EXTERN	char	    *CScnd_get_name();
FUNC_EXTERN     STATUS      CSaltr_session();

    /* CS_accept_connect() returns TRUE if the server can handle another
    ** connection request. If the server has the maximum number of threads
    ** or is in the middle of shutting down, then it will return FALSE
    */
FUNC_EXTERN	i4	    CSaccept_connect();
/*{
** Name: CSswitch - Poll for possible session switch
**
** Description:
**      This call will determine if the inquiring session has
**      used more thanan allotted quantum of CPU and yield
**      the CPU if so.
**
**      How this quantum measurement is managed, updated and
**      checked is system specific.  However, it is essential that
**      this call imposes "negligible" overhead.  This function
**	may be called very frequently and should not do much more than
**	examine a few memory locations before returning for the case that
**	a switch does not occur.
**
**      When a session has consumed its quantum of the CPU resource and
**      has invoked CSswitch(), it will "yield the CPU" to other
**      runnable sessions.
**
**      The quantum allotment will typically be specified at server
**      startup time as an option named "quantum" with a system
**      specific interpretation of the option argument.  This
**      argument should not be passed to mainline code with other
**      options.
**
**      Some CL's may not need this ability; those implementations
**      need only supply a null effect call or macro.
**
**      This call is applicable from INGRES Release 6.1 and on.
**
** Issues/Concepts:
**      This call is supplied to allow polling a value to determine
**      whether to switch between sessions.  This is necessary on machines
**      which can not have a system timer which switchs sessions after
**      a time slice has expired.
**
**      This call should not be implemented to simply yield the
**      CPU as the polling is not a regularly occurring activity
**      of mainline code.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**      Returns:
**          none
**
**      Exceptions:
**          none
**
** Side Effects:
**    Thread (session) switch will occur when the "quantum" value
**    has been exceeded.
**
** History:
**      11-dec-89 (anton)
**          Created.
**	18-mar-94 (walt)
**	    CSswitch is actually a function in csinterface.c on Alpha.  So
**	    remove its #define from here.  Also define ALLOWED_QUANTUM_COUNT,
**	    the number of quantum ticks that can accumulate before CSswitch
**	    (in csinterface.c) will switch out a thread.
*/
#define ALLOWED_QUANTUM_COUNT	1
#if 0
#define	CSswitch()
#endif
/*
**  Function Prototypes 
*/

/* fetch the session id */
FUNC_EXTERN VOID     CSget_sid(i4 *sidptr);

/* fetch the scb */
FUNC_EXTERN VOID     CSget_scb(CS_SCB **scbptr);	

/* find "other thread's" scb */
FUNC_EXTERN     CS_SCB      *CSfind_scb(i4 sid); 

/* suspend current session */
FUNC_EXTERN STATUS     CSsuspend(i4 mask, i4 to_cnt, PTR ecb);

/* resume session ast */
FUNC_EXTERN VOID CSresume(i4 sid);	

/* session interrupt ast */
FUNC_EXTERN VOID CSattn(i4 eid, i4 sid);

/* lock sem */
FUNC_EXTERN STATUS CSp_semaphore(i4 exclusive, CS_SEMAPHORE *sem);

/* unlock sem */
FUNC_EXTERN STATUS CSv_semaphore(CS_SEMAPHORE *sem);

/* init sem */
FUNC_EXTERN STATUS CSi_semaphore(CS_SEMAPHORE *sem, i4 type);

/* remove sem */
FUNC_EXTERN STATUS CSr_semaphore(CS_SEMAPHORE *sem);

/* name sem */
FUNC_EXTERN VOID CSn_semaphore(CS_SEMAPHORE *sem, char *name);

/* attach to MULTI sem */
FUNC_EXTERN STATUS CSa_semaphore( CS_SEMAPHORE *sem );

/* detach from MULTI sem */
FUNC_EXTERN STATUS CSd_semaphore( CS_SEMAPHORE *sem );

/* CS_accept_connect() returns TRUE if the server can handle another
** connection request. If the server has the maximum number of threads
** or is in the middle of shutting down, then it will return FALSE
*/

FUNC_EXTERN bool	CSaccept_connect(void);

FUNC_EXTERN STATUS CSaltr_session(i4 session_id, i4 option, PTR item);

FUNC_EXTERN STATUS CScnd_wait(CS_CONDITION *cnd, CS_SEMAPHORE *sem);

FUNC_EXTERN STATUS CScnd_signal(CS_CONDITION *cnd);

FUNC_EXTERN STATUS CScnd_init( CS_CONDITION *cnd );

FUNC_EXTERN STATUS CScnd_free( CS_CONDITION *cnd );

FUNC_EXTERN STATUS CScnd_broadcast(CS_CONDITION *cnd );

FUNC_EXTERN STATUS CSalter(CS_CB *ccb);

FUNC_EXTERN STATUS CSdispatch(void);

FUNC_EXTERN STATUS CSinitiate(i4 *argc, char ***argv, CS_CB *ccb );

FUNC_EXTERN STATUS CSremove(CS_SID sid);

FUNC_EXTERN STATUS CSterminate(i4 mode, i4 *active_count);

/*
** Constants that define MIN and MAX priorities of sessions, and the
** default priority used if priority 0 is given when starting a thread.
**	MIN_PRIORITY is 1 - priority 0 cannot be specified since passing
**	    a zero value to CSadd_thread indicates to use the default priority.
**	    The Idle Job is the only thread that will run at priority zero.
**	MAX_PRIORITY is two less than CS_LIM_PRIORITY to ensure that no
**	    sessions run at higher priority than the admin job (which runs
**	    at CS_LIM_PRIORITY - 1).
*/
#define			CS_MINPRIORITY	1
#define			CS_MAXPRIORITY	(CS_LIM_PRIORITY - 2)
#define			CS_DEFPRIORITY	(CS_LIM_PRIORITY / 2)
#define 		CS_CURPRIORITY  -1

# endif /* CS_H_INCLUDED */
