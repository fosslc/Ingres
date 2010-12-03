/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
*/

#include    <compat.h>
#ifdef OS_THREADS_USED
#include    <gl.h>
#include    <st.h>
#include    <systypes.h>
#include    <clconfig.h>
#include    <fdset.h>
#include    <clsigs.h>
#include    <rusage.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <cv.h>
#include    <pc.h>
#include    <lk.h>
#include    <tr.h>
#include    <csev.h>
#include    <cssminfo.h>
#include    <clpoll.h>
#include    <csinternal.h>
#include    <gcccl.h>
#include    <meprivate.h>
#ifdef VMS
#include    <exhdef.h>
#include    <fcntl>
#endif      /* VMS */
#ifdef xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */
#ifdef xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif /* xCL_007_FILE_H_EXISTS */
#ifdef  xCL_074_WRITEV_READV_EXISTS
#include    <sys/uio.h>
#endif
#include    <errno.h>
#include    <pwd.h>
#include    <diracc.h>
#include    <handy.h>
#include    "csmtlocal.h"
#include    "csmtsampler.h"

#if defined(VMS)
#include    <cscl.h>
#include    <csnormal.h>
#endif

# if defined(sparc_sol) || defined(sui_us5)
# include <sys/asm_linkage.h>
# endif

# ifdef xCL_077_BSD_MMAP
# include <sys/stat.h>
# include <sys/mman.h>          /* may change on different systems */
# ifdef    xCL_007_FILE_H_EXISTS
# include    <sys/file.h>
# endif /* xCL_007_FILE_H_EXISTS */
# endif /* xCL_077_BSD_MMAP */

#if defined(sparc_sol)
#include <sys/procfs.h>
#endif

#if defined(LNX)
#include <linux/unistd.h>
#include <asm/param.h>
#endif

#if defined(VMS)
#include <efndef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <sjcdef.h>
#endif

/**
**
**  Name: CSMTHL.C - Control System "High Level" Functions
**
**  Description:
**      This module contains those routines which provide system dispatching 
**      services to the DBMS (only, at the moment).  "High level" refers to
**      those routines which can be implemented in C as opposed to macro.
**
**          CSMT_alloc_stack() - Allocate a thread's stack.
**          CSMT_deal_stack() - Deallocate a thread's stack.
**          CSMT_setup() - Run the thread thru its appropriate
**			functions based on the modes set
**			by the mainline code.
**          CSMT_swuser() - Do a rescheduling context switch. (This function
**			  may be written in assembly language and found
**			  in csll.s)
**          CSMT_find_scb() - Find the SCB belonging to a particular
**			    thread id.
**          CSMT_dump_scb() - Dump the high level scb contents.
**          CSMT_rcv_request() - Prime system to receive a request
**				for new thread addition.
**          CSMT_del_thread() - Final processing for deletion of a thread.
**          CSMT_admin_task() - Thread addition and deletion thread.
**          CSMT_ticker() - Wall-clock watcher for time checks.
**          CSMT_exit_handler() - Final CS server processing.
**          CSMT_fmt_scb() - Format an SCB for display.
**          CSMT_get_thread_stats() - Get thread stats.
[@func_list@]...
**
**
**  History:
**      23-Oct-1986 (fred)
**          Created.
**      21-jan-1987 (fred)
**          Added cputime/thread support
**      19-Nov-1987 (fred)
**          Added last chance exception handling support.
**      13-Oct-88 (arana)
**          Created machine-specific C version of CS_swuser.
**	31-oct-1988 (rogerk)
**	    Only collect CPU stats when exchange thread if old or new
**	    session has CS_CPU_MASK set.
**	    Only write records to accounting file if CS_ACCNTING_MASK
**	    set and it is a user session.
**	 7-nov-1988 (rogerk)
**	    Added non-ast task switching and non-ast methods of protecting
**	    against asynchronous routines. Call CS_ssprsm directly instead 
**	    of through AST's.  Added new inkernel protection.  When turn
**	    of inkernel flag, check if need to call CS_move_sync.
**	 20-feb-1989 (markd)
**	    Added Sequent specifc stuff, include sqs_pushf
**	 27-Feb-1989 (fredv)
**	    Removed inclusion of <sys/signal.h>, <sys/wait.h>.
**	    Use new ifdefs, reorder includes, use rusage.h, handle
**	    wait types, incorrectly handle lack of wait3.  Will need
**	    to return to the last point.  Add stuff for non-getrusage
**	    machines.
**	29-Mar-89 (arana)
**          Modified CS_swuser to not use chgstack system call to switch
**	    contexts if scheduler decides current thread should continue
**	    execution.
**      21-Apr-89 (arana)
**	    Use ME_ALIGN_MACRO to clean up pyramid-specific stack
**	    initialization.
**	12-jun-89 (rogerk)
**	    Added allocated_pages argument to MEget_pages calls.
**	19-jun-89 (rogerk)
**	    Added CS_change_priority routine for cross-process semaphores.
**	27-jul-89 (fred)
**	   Make CSsuspend() call on writes from CS_setup() interruptable
**         to imporve handling.
**    26-aug-89 (rexl)
**        Added calls to protect ME stream allocator and page allocator.
**    16-sep-1989 (fls-sequent)
**        Extensive modifications for MCT.
**	6-Dec-89 (anton)
**	   Check for exception recursion during completion code
**	   and set flag when completion code is running.
**	   SIprintf removed because stderr is not open in server.
**	26-Jan-90 (anton)
**	   Fix cpu time measurement with times function.
**	30-Jan-90 (anton)
**	   Fix timeout queue scan - don't return negative timeout
**	   Fix idle job CPU spin in semaphore cases
**	   Increased idle loop iiCLpoll timeout
**	6-Feb-90 (jkb)
**	   Change the error handling for the times function.  The SYS V
**	   times function may succeed and return a negative number.
**	23-mar-90 (mikem)
**	   Added in code for debugging performance throughput problems.  
**	   The new routine "CS_display_sess_short()" and calls to it have
**	   added to the file, but have been ifdef'd out.
**	1-june-90 (blaise)
**	    Removed duplicate declaration of CS_admin_task();
**	    Integrated changes from termcl code line:
**		Fix server termination via UNIX signals;
**		Add include <clsigs.h> to build properly everywhere.
**	4-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add vast quantities of machine-specific code;
**		Add error message for a failure of iiCLpoll call;
**		Move the guts of the EXCHLDDIED case in cs_handler() to
**		CSsigchld;
**	7-june-90 (blaise)
**	    Fixed syntax errors in ifdefs.
**	4-sep-90 (jkb)
**	    Add sqs_ptx to the appropriate list of ifdefs
**	01-oct-90 (chsieh)
**	    Added bu2_us5 and bu3_us5 to the appropriate #ifdefs. 
**	07-Dec-90 (anton)
**	    Fix cpu use measurement
**	    Reinstate lost cpu calculation change of mine (26-jan-90).
**	    Bugs 20254, 32677, 35024
**	12-nov-90 (mikem) 
**	    integrated from ingres63 library: 30-aug-90 (mikem)
**	    bug #32958
**	    Make Idle loop bump CS_ADD_QUANTUM count if timer expires without
**	    quantum count being updated by the RCP.  See comments above the
**	    call to CS_ADD_QUANTUM for more detail.
**	28-Jan-90 (anton)
**	    Add condition wait state to debug support
**	12-feb-91 (mikem)
**	    Added stack overflow protection to CS_alloc_stack for sun4 based
**	    systems.  See CS_alloc_stack function header for description.
**	12-feb-91 (mikem)
**	    Added two new internal unix CL routines CS_dump_stack() and
**	    CS_su4_dump_stack() to help in debugging AV's by printing out
**	    stack traces.
**	14-feb-91 (ralph)
**	    Implement session accounting
**	25-mar-91 (kirke)
**	    Moved #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.  Also updated CS_fmt_scb() for hp8_us5 to reflect
**	    altered CS_SCB structure.
**	09-apr-91 (seng)
**	    Add ris_us5 specific routines and structures.
**	06-jun-91 (ralph)
**	    Revised session accounting as per review.
**	28-jun-91 (kirke)
**	    Changed hp8_us5 case in CS_fmt_scb() to account for HP-UX 8.0
**	    jmp_buf change.
**	25-sep-1991 (mikem) integrated following change: 06-aug-91 (ralph)
**	    Revised UNIX Session Accounting per Sweeney:
**		- Issue error messages from CS_del_thread()
**		- Add function declarations and return types
**		- Other minor aesthetic changes
**      25-sep-1991 (mikem) integrated following change: 13-Aug-91 (dchan)
**         The decstation compiler is unable to parse the 
**         statement "scb = (CS_SCB *)((CSEV_CB *)scb)->sid;"
**         in CS_find_scb(), complaining
**         about "illegal size in a load/store operation."
**         The statement was broken into 2 separate statements using a
**         temporary, intermediate variable.
**	25-sep-1991 (mikem) integrated following change: 12-sep-91 (kirke)
**	    Changed session accounting struct to use only the basic types.
**	06-apr-1992 (jnash)
**	    Change xCL_077_MMAP_EXISTS to xCL_077_BSD_MMAP to get
**	    MMAP to work properly.
**	02-jul-92 (swm)
**	    Correct include location for dr6_us5 asm_linkage.h file.
**	    Copy all references for dr6_us5 for dr6_uv1 (same hardware but
**	    dr6_uv1 has secure unix extensions to SVR4).
**	    Add dra_us5 to list of machines requiring stack pages from the
**	    server stack.
**	    Add dr6_us5, dr6_uv1 and dra_us5 (all SVR4) to list of systems
**	    that use /dev/zero.
**	4-jul-92 (daveb)
**	    Remove threads from cs_scb_tree on CS_eradicate.
**	01-dec-92 (mikem)
**	    su4_us5 6.5 port.  Added ifdef's for solaris 2.1 port following
**	    those of the su4_u42.
**	13-Dec-1992 (daveb)
**	    In CS_admin_task, separately check for CS_ADD_THREAD at
**	    the end instead of doing it in an else if.  This makes
**	    GCM completion work.
**	26-apr-93 (vijay)
**	    do proper typecasting for ris_us5 specific code. Move else to
**	    the right place in ifdef.
**	27-apr-93 (sweeney)
**	    Added per-box cases for USL destiny generic SVR4 (usl_us5).
**	    Fixed up an STprintf in CS_fmt_scb to do what the author intended.
**	20-jun-93 (vijay)
**	    ris_us5 specific: jump into the middle of CS_ris_setup from swuser.
**	    Else dbx gets confused about the stack and becomes non
**	    cooperative. Use mmap protection scheme for the stack. save float
**	    registers.
**	    Remove inclusion of erclf.h
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	9-aug-93 (robf)
**          add su4_cmw
**	16-aug-93 (ed)
**	    add <st.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      25-aug-93 (smc)
**          Fixed cast of MEget_pages parameter into the correct type.
**	27-aug-93 (swm)
**          Added OSF support for protecting the last page in a stack. This
**          uses mmap() with the MAP_ANONYMOUS attribute rather than
**          mapping /dev/zero.
**          Added axp_osf machine-specific code.
**      01-sep-93 (smc)
**          Changed cast of scb to CS_SID. Fixed up cast in MEget_pages call.
**	20-sep-1993 (mikem)
**	    Enable stack dumping routines on su4_us5.  The su4_u42 routines
**	    work on su4_us5.
**	30-dec-1993 (andys)
**	    Fix up CS_dump_stack to have extra (verbose) argument.
**          Make termination of stack dump loop more clear.
**	14-oct-93 (swm)
**	    Bug #56449
**	    Changed %x to %p for display of pointer addresses.
**	2-oct-93 (swm)
**	    Bug #56447
**	    Cast assignment to cs_used to CS_SID, now that cs_used type has
**	    changed so that CS_SID values can be stored without truncation.
**	03-dec-93 (nrb/swm)
**	    Bug #58873
**	    Enabled code for mmap stack protection. Amended the code to
**	    cater for dynamic page size across the axp_osf range of machines;
**	    we now use the maximum of machine page size and ME_PAGESIZE.
**	    (Note that the Unix CL ME_ALIGN_MACRO assumes its align parameter
**	    is a power of 2, so we assume that the page size passed to the
**	    macro is a power of 2 (which seems reasonable)).
**	10-feb-1994 (ajc)
**	    Added hp8_bls entires based on hp8*
**	20-apr-94 (mikem)
**	    Bug #57043
**	    Added a call to CS_checktime() to CS_xchng_thread().  This call
**	    is responsible for keeping the pseudo-quantum clock up to date, 
**	    this clock is used to maintain both quantum and timeout queues in
**	    CS.  See csclock.c for more information.
**	21-Apr-1994 (daveb)
**	    Made HP CS_swuser void to agree with prototype in cslocal.h
**	    added by mikem for 57043.  Wouldn't compile otherwise.
**	18-Oct-94 (nanpr01)
**	    Handler should be set if pid != -1. Otherwise in BSD it will
**          go into a loop. Bug # 64154 
**      29-Nov-94 (liibi01)
**          Bug 64041, cross integation from 6.4. Add server name and time
**          stamp to error text produced in cs_handler(). Add header file
**          gcccl.h for constant GCC_L_PORT.
**      29-Nov-94 (liibi01)
**          Bug 64042, cross integration from 6.4. Modify the stack dump
**          function and now the error log records the server id as part of
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**          the error message if encountered.
**	3-may-95 (hanch04)
**	    Added changed made by nick.
**          Change CS_admin_task() to separately check for CS_ADD_THREAD
**          at the end instead of doing it in an else if. (daveb)
**          Return address faked for CS_su4_setup() to return to
**          CS_su4_eradicate() is incorrect when using su4_us5 V3 compiler.
**          Changed to new value which is always correct as CS_su4_eradicate()
**          now coded in assembler.
**      13-may-95 (smiba01)
**          Added support for dr6_ues.  These changes taken from ingres63p
**          library.  They were made there on 29-apr-93.
**	 9-Jun-1995 (jenjo02)
**	    In CS_change_priority()
**	    turn off CS_PRIORITY_BIT if old priority queue empty,
**	    turn on CS_PRIORITY_BIT in new priority queue if
**	    thread is computable.
**	    In CS_xchng_thread(), don't scan ready queues if the only
**	    runnable thread is the idle thread. Don't remove/insert
**	    thread on rdy queue if it's already last on the queue.
**	13-Jun-1995 (jenjo02)
**	    Keep cs_num_active in step with number of non-internal threads on 
**	    the ready queues. Previously, this number was not maintained.
**	24-jul-95 (allmi01)
**	    Added support for dgi_us5 (DG-UX on Intel based platforms) following
**	    usl_us5 (UNIXWARE).
**	27-jul-95 (morayf)
**	    Added sos_us5 support based on SCO ODT (odt_us5).
**	22-Aug-1995 (jenjo02)
**	    Count only non-CS_INTRNL_THREADs in cs_num_active.
**	19-sep-95 (nick)
**	    Various changes to get stack guard pages working ( they never
**	    did before ! ) and tidy some code.
**	10-nov-1995 (murf)
**	    Added sui_us5 to all areas specifically defined with usl_us5.
**	    Reason, to port Solaris for Intel.
**	    Also added sui_us5 to include the header defined for su4_us5.
**	30-nov-95 (nick)
**	    CS_su4_dump_stack() defined for dr6_us5 but wasn't prototyped.
**	    Also, include changes for 22-Aug-1995 (jenjo02) for which the
**	    comment is already here but mysteriously, not the actual code
**	    changes themselves.
**	25-jan-96 (smiba01)
**	    sun_cmw was missing from fudge_stack define list
**	12-Jan-1996 (jenjo02)
**	    Fixed cs_wtstatistics, which previously did nothing useful.
**	16-Jan-1996 (jenjo02)
**	    Wait reason for CSsuspend() no longer defaults to CS_LOG_MASK,
**	    which must now be explicitly stated.
**	15-Mch-1996 (prida01)
**	    Print out symbols in stack traces
**      06-dec-95 (morayf)
**          Added support for SNI RMx00 (rmx_us5) - pyramid clone.
**          Also put Pyramid entries in (pym_us5). It seems a bit odd
**          that there were no pym_us5 references here already.
**	18-Mch-1996 (prida01)
**	    Correct guard page check.
**	03-jun-1996 (canor01)
**	    Preliminary changes to support operating system threads. 
**	    Initiate threads with system thread creation functions. All
**	    threads are runnable when they are not sleeping on an
**	    operating system event or mutex.  The "current" scb must
**	    be referenced on a per-thread basis, since multiple threads
**	    may be in the same code at the same time. Block SIGUSR2 for
**	    all threads except those that will handle async i/o completions.
**	    All references to a CS_SID are to a thread id, which is not
**	    the same as a (CS_SCB *).
**	16-sep-1996 (canor01)
**	    If the server state is CS_TERMINATING, do not attempt to
**	    receive another connection.
**	18-sep-1996 (canor01)
**	   To give more meaning to sampler statistics, set flags in idle
**	   thread's scb while sleeping.
**	04-oct-1996 (canor01)
**	   Remove CS_toq_scan() and CS_xchng_thread().  Let the idle thread
**	   sleep with CSsuspend().  If server is CS_TERMINATING, do not change
**	   the state to CS_CLOSING.
**	18-oct-96 (mcgem01)
**	    IIDBMS changed to DBMS for accounting log purposes.
**	31-oct-1996 (canor01)
**	    Add CS_INTERRUPT_MASK to the idle thread's CSsuspend() sleep.
**	11-nov-1996 (canor01)
**	    If server is in process of shutdown, let idle thread loop
**	    until shutdown is complete or canceled.  Also, protect 
**	    cs_user_sessions from simultaneous access.
**	12-nov-1996 (canor01)
**	    The monitor thread starts life as a user thread, but transforms
**	    into a monitor thread.  cs_user_threads flag needs to reflect this.
**	22-nov-1996 (canor01)
**	    Changed names of CL-specific thread-local storage functions 
**	    from "MEtls..." to "ME_tls..." to avoid conflict with new 
**	    functions in GL.  Included <meprivate.h> for function prototypes.
**      18-dec-1996 (canor01)
**          With POSIX threads, CS_SID is (CS_SCB*) since a thread id can
**          be an arbitrary structure.  This actually simplifies CS_find_scb(),
**          since it is passed what it needs to return.
**	19-dec-1996 (wonst02)
**	    Add include <lk.h> for new lock info in cssampler.h.
**	17-jan-1997 (wonst02)
**	    Use CS_tls_ routines instead of ME_tls_ for the SCB.
**      22-jan-1997 (canor01)
**          Change timeout on idle thread's CSsuspend() to allow it to handle
**          slave completion similar to the Ingres-threaded version.
**	07-mar-1997 (canor01)
**	    Make all functions dummies unless OS_THREADS_USED is defined.
**	23-apr-1997 (canor01)
**	    Check connect limit with CSaccept_connect(), which will
**	    log failures with TRdisplay().
**	02-may-1997 (canor01)
**	    Sessions created with the CS_CLEANUP_MASK will be shut
**	    down before any other system threads.  When they exit, they
**	    must call CSterminate() to allow the normal system threads
**	    to exit.
**	07-jul-1997 (muhpa01)
**	    Added calls to CS_thread_detach() for hp8_us5 to release thread
**	    object resources before ending thread (for POSIX draft 4, can't
**	    create threads in detached state)
**	19-sep-1997 (canor01)
**	    Create an alternate stack for each thread that can be used by
**	    signal handlers (allowing recovery from stack overflow of the
**	    main stack).
**	03-nov-1997 (canor01)
**	    If stack_caching is enabled, allocate the thread stack using
**	    CS_alloc_stack, otherwise let the OS allocate the stack.
**	    Deallocate the stack when the thread exits.  Initialize the
**	    stack cache queue at startup.
**	13-Feb-98 (fanra01)
**	    Made changes to move attch of scb to MO just before we start
**	    running.
**	    This is so the SID is initialised to be used as the instance.
**	11-mar-1998 (canor01)
**	    If stack caching is enabled, save the thread's stack header
**	    address in a local variable that can be passed to CS_deal_stack
**	    after the scb has been deallocated.
**	20-Mar-1998 (jenjo02)
**	    Removed cs_access_sem, utilizing cs_event_sem instead to 
**	    protect cs_state and cs_mask.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSMTadd_thread() prototype.
**	02-apr-1998 (canor01)
**	    Store information about the system-allocated stack in an
**	    allocated CS_STK_CB structure.  This can be accessed when doing
**	    a stack dump to prevent walking off the end of the stack.
**	13-may-1998 (muhpa01)
**	    For hpb_us5, increase the size of the alternate stack from 2 to 10
**	    pages.
**      24-jun-1998 (canor01)
**          Correctly pass pointer to CL_ERR_DESC to cs_elog function.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id. This eliminates the need to 
**	    mutex and walk the known SCB list to find an SCB by sid
**	    (CSMTresume(), e.g.).
**	    Removed cs_cnt_mutex, utilizing Cs_known_list_sem instead.
**	16-Nov-1998 (jenjo02)
**	  o Removed IdleThread, replacing cross-process resume mechanism
**	    with one using CP mutex/conditions in shared memory. SCB's
**	    cs_event_sem/cs_event_cond replaced with a pointer to SM 
**	    structure belonging to the session (cs_evcb), and which contains 
**	    the CP cs_event_sem/cs_event_cond.
**	  o IdleThread source code has been #ifdef'd out; in the event
**	    that the CP mechanism causes problems on certain UNIX platforms,
**	    reinstating it will be less painful. (look for "USE_IDLE_THREAD")
**	    On Unix mutations which don't support cross-process mutexes and 
**	    conditions, setting the define for "USE_IDLE_THREAD" in csnormal.h 
**	    may be required.
**	  o Added new distiguishing event wait masks, one for Log I/O, and
**	    one each for I/O read or write, corresponding new statistics
**	    in CS_SYSTEM and CS_SCB.
**	  o New structure (CS_CPID) and external function (CSget_cpid())
**	    which define a thread's cross-process identity.
**	02-Dec-1998 (muhpa01)
**	    Removed code for POSIX_DRAFT_4 for HP - obsolete.
**	29-jan-1999 (schte01)
**       Corrected ifdef USE_USE_IDLE_THREAD to USE_IDLE_THREAD
**	15-mar-1999 (muhpa01)
**	    For hpb_us5, reduced the size of the alternate stack from 10
**	    to 4 pages - 10 was excessive.  Also, remove code for stack
**	    caching for hpb_us5.
**	10-may-1999 (walro03)
**	    Remove obsolete version string sqs_u42, sqs_us5.
**	03-nov-1998 (devjo01)
**	    Cross-integrate canor01 changes for b89582.
**	14-apr-1999 (devjo01)
**	    - Removed creation and initialization of CS_STK_CB structure
**	    when thread is using an OS provided stack.
**	    - Removed unneeded FUNC_EXTERN refs.
**	03-sep-1999 (somsa01)
**	    In CSMT_del_thread(), moved CS_detach_scb() call to RIGHT AFTER
**	    removing the scb from the known list, protecting it with the
**	    Cs_known_list_sem semaphore. In this way, we eliminate a hole
**	    whereby, while a session is exiting, another session accessing
**	    MO objects will not obtain information about the exiting session
**	    (and its scb) before detaching its scb from MO but AFTER its
**	    physical scb was taken off the scb list. This caused SEGVs in MO
**	    routines due to the fact that they obtained null SCB pointers
**	    and tried to dereference them.
**	04-Apr-2000 (jenjo02)
**	    Signals aren't used if the idle thread is not used, hence 
**	    there's no need for an alternate stack for signal handling
**	    in CSMT_setup().
**	30-may-2000 (toumi01)
**	    When we have an idle thread, assign its pid to csi_signal_pid.
**	    This only matters for servers where the main thread and idle
**	    thread pids can be different because of the 1:1 thread model
**	    (e.g. Linux).
**	21-aug-2000 (somsa01)
**	    S390 Linux does not have a stack.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**      26-Mar-2001 (hanal04) Bug 103080 INGSRV 1296
**          Prevent cs_num_active from running away by preventing
**          CSMTresume() calls on sessions that have not been suspended.
**	23-jan-2002 (devjo01)
**	    Backout change 452924 to CSMT_find_scb.
**	22-May-2002 (jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	11-nov-2002 (somsa01)
**	    On HP-UX, there exists the concept of thread/stack caching.
**	    Thus, we do not have to go through the stack caching specific
**	    code that we wrote.
**	16-dec-2002 (somsa01)
**	    In CSMT_del_thread(), changed type of ac_btime to match the type
**	    of cs_connect.
**	21-May-2003 (bonro01)
**	    Add support for HP Itanium (i64_hpu)
**      07-July-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads.
**	23-Sep-2003 (hanje04)
**	    Correct bad #ifdef introduced buy fix for bug 110542
**      28-Dec-2003 (horda03) Bug 111532/INGSRV2649
**          For AXP.OSF, and probably any 64 bit OS
**          the size of the ALTSTACK is inadequate. Thus when an exception 
**	    occurs a second exception occurs while reporting the original 
**	    exception which causes the server to crash. Increased number of 
**	    pages used by the ALTSTACK.
**	17-Nov-2004 (hanje04)
**	    BUG 113487/ISS 13680685
**	    Don't try and free the ALTSTACK on Linux as we allocate it on the
**	    the stack. xCL_SYS_PAGE_MAP was defined as part of the fix for 
**	    bug 113487 and this caused a SEGV which wasn't happening before.
**      15-Mar-2005 (bonro01)
**          Added test for NO_INTERNAL_THREADS to
**          support Solaris AMD64 a64_sol.
**	1-Feb-2006 (kschendel)
**	    Put the static init of StackSem with the definition, ie here.
**	13-Feb-2007 (kiria01) b117581
**	    Give event_state its own set of mask values.
**	07-Oct-2008 (smeke01) b120907
**	    Get OS tid for those platforms that support it (see csnormal.h).
**	01-dec-2008 (joea)
**	    Add VMS-specific accounting in CSMT_del_thread.
**	05-dec-2008 (joea)
**	    Exclude Unix signal and event system code from VMS builds.
**	09-Feb-2009 (smeke01) b119586
**	    Added function CSMT_get_thread_stats().
**      20-mar-2008 (stegr01)
**          Add support for posix threaded VMS itanium build with various hdr includes
**          exclude signal function calls for VMS
**          set password field to NULL
**          define a constant for the maximum length of a pthread object name
**          (pthread.h does not define this although it does document it)
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**     04-nov-2010 (joea)
**         Change CSMT_rcv_request to static.  Add return types to the latter
**         and to CSMT_del_thread.
**/

/*
** Typedef needed for CS_compress() and CS_del_thread()
** must come before forward ref for CS_compress().
** (sweeney)
*/
typedef u_short comp_t;

#define PTHREAD_MAX_OBJNAM_LEN 31


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN comp_t  CS_compress();      /* needed for CS_del_thread(); */

#ifdef xCS_WAIT_INFO_DUMP
static VOID	    CS_display_sess_short();
#endif /* xCS_WAIT_INFO_DUMP */

/*
** Definition of all global variables used by this file.
*/

GLOBALREF CS_SYSTEM           Cs_srv_block;
GLOBALREF CS_SCB	      Cs_idle_scb;

GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;
GLOBALREF	CS_SERV_INFO	*Cs_svinfo;

GLOBALDEF       CS_SEMAPHORE    CS_acct_sem;

GLOBALREF i4		Cs_lastquant;
GLOBALREF i4			Cs_nextpoll ;
GLOBALREF i4			Cs_polldelay;

GLOBALREF CS_SYNCH		CsMultiTaskSem;
GLOBALDEF CS_SYNCH              CsStackSem ZERO_FILL;
GLOBALREF CS_SEMAPHORE		Cs_known_list_sem;



/*{
** Name: CSMT_setup()	- Place bottom level calls to indicated procedures
**
** Description:
**      This routine is called after the initial stack layout is set up. 
**      It simply sits in a loop and calls the indicated routine(s) back 
**      to perform their actions.
**
**	If the thread being called is the admin or idle threads, then special
**	action is taken.
**
** Inputs:
**      none
**
** Outputs:
**	none
**
**	Returns:
**	    Never (on purpose).
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-Nov-1986 (fred)
**          Created.
**	 7-nov-1988 (rogerk)
**	    Make sure to set inkernel while changing the scb's cs_mask field
**	    as this is also set by AST driven routines.  
**	6-Dec-1989 (anton)
**	    Add check for exception recursion from completion code
**	    and added Cs_incomp flag to idle loop
**	1-Feb-1990 (anton)
**	    Increated idle loop iiCLpoll timeout
**	12-nov-90 (mikem) 
**	   integrated from ingres63 library: 30-aug-90 (mikem)
**	   bug #32958
**	   Make Idle loop bump CS_ADD_QUANTUM count if timer expires without
**	   quantum count being updated by the RCP.  See comments above the
**	   call to CS_ADD_QUANTUM for more detail.
**	17-may-1991 (bonobo)
**	   Added the extra ER_ERROR_MSG in each occurence of ERsend.  This
**	   conforms to the ingres65 messaging scheme in er.c.
**	18-sep-1996 (canor01)
**	   To give more meaning to sampler statistics, set flags in idle
**	   thread's scb while sleeping.
**	31-oct-1996 (canor01)
**	    Add CS_INTERRUPT_MASK to the idle thread's CSsuspend() sleep.
**	11-nov-1996 (canor01)
**	    If server is in process of shutdown, let idle thread loop
**	    until shutdown is complete or canceled.  Also, protect 
**	    cs_user_sessions from simultaneous access.
**	22-jan-1997 (canor01)
**	    Change timeout on idle thread's CSsuspend() to allow it to handle
**	    slave completion similar to the Ingres-threaded version.
**	19-sep-1997 (canor01)
**	    Create an alternate stack for each thread that can be used by
**	    signal handlers (allowing recovery from stack overflow of the
**	    main stack).
**	11-mar-1998 (canor01)
**	    If stack caching is enabled, save the thread's stack header
**	    address in a local variable that can be passed to CS_deal_stack
**	    after the scb has been deallocated.
**	29-jan-1999 (schte01)
**       Corrected ifdef USE_USE_IDLE_THREAD to USE_IDLE_THREAD
**      19-sep-1997 (canor01)
**          Create an alternate stack for each thread that can be used by
**          signal handlers (allowing recovery from stack overflow of the
**          main stack).
**	14-apr-1999 (devjo01)
**	    Removed creation and initialization of CS_STK_CB structure
**	    when thread is using an OS provided stack.   This was only
**	    being done in order to support stack dumps.  On many
**	    platforms it is neither necessary nor possible to implement
**	    the CS_get_stackinfo macro used to obtain the OS provided
**	    stack base and size.   So instead of universally allocating
**	    and initializing this control block, just in case this 
**	    information is required for the stack dump, we will get this
**	    on the fly within the OS specific dump code if it is required. 
**	04-Apr-2000 (jenjo02)
**	    Signals aren't used if the idle thread is not used, hence 
**	    there's no need for an alternate stack for signal handling
**	    (ex_stack) in CSMT_setup().
**      26-Mar-2001 (hanal04) Bug 103080 INGSRV 1296
**          Do not set CS_EVENT_WAIT prior to calling CSMTsuspend().
**          An asynchronous interrupt could lead to a CSMTresume() when
**          we have not suspended. CSMTsuspend() will safely set
**          CS_EVENT_WAIT.
**	26-apr-2002 (bespi01) Bug 105576 Problem INGSRV#1698
**	    Added an exit status variable in CSMT_setup to be used when 
**	    calling exit_thread().
**	22-May-2002 (jenjo02)
**	    Include CS_MONITOR sessions in cs_user_sessions count to
**	    be consistent with CS_del_thread().
**      25-Jun-2003 (hanal04) Bug 110345 INGSRV 2357
**          Store the CS_SCB's cs_thread_id in the CS_STK_CB before 
**          deallocating the CS_SCB. The real thread id is needed when
**          we reap stacks and we must not reference deallocated
**          memory.
**      07-Jul-2003 (hanje04)
**          BUG 110542 - Linux Only
**          Enable use of alternate stack for SIGNAL handler in the event
**          of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**          the alternate stack to be malloc'd so declare it on IICSdispatch
**          stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**          for OS threads.
**	21-Sep-2004 (bonro01)
**	    Correct change 471661 for bug 110345.  Add ifdefs for HP to
**	    prevent SIGBUS errors.
**	07-oct-2004 (thaju02)
**	    Change npages to SIZE_TYPE.
**	19-Jan-2005 (schka24)
**	    Fix double-free of alt stack if EXaltstack fails for some reason.
**	11-Mar-2005 (hanje04)
**	    Never use ME_getpages to allocate altstack on Linux, doesn't
**	    play well with pthread and causes memory leak when xCL_077_BSD_MMAP
**	    is defined and USE_IDLE_THREAD isn't (NPTL build). 
**    18-Jul-2005 (hanje04)
**        BUG 114860:ISS 1475660
**        In CS_update_cpu, if we're running MT on Linux (and therefore
**        1:1 threading model) Cs_srv_block.cs_cpu has to be a sum of the
**        CPU usage for each thread. Added cs_cpu to CS_SCB to track
**        thread CPU usage so that Cs_srv_block.cs_cpu can be correctly
**        maintained. Initialize cs_cpu here.
**	01-Aug-2005 (jenjo02)
**	    CS_find_events() now clears csi_wakeme while holding 
**	    csi_spinlock when nevents is zero.
**      13-jan-2004 (wanfr01)
**          Bug 64899, INGSRV87
**          Enabled Alternate stack for AIX so large in clauses won't
**          kill the server
**       1-Jun-2006 (hanal04) Bug 116179
**          Add ifdef and NULL test for stkhdr assignment to prevent
**          SIGSEGV/SIGBUS.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Don't clear EDONE from cs_mask, CSMT never sets it there.
*/
VOID
CSMT_setup(void *parm)
{
    STATUS     (*rtn) ();
    STATUS     cs_handler();
    STATUS     status;
    STATUS     thread_status = OK;	/* exit status of this thread */
    EX_CONTEXT excontext;
    CS_SCB     **ppSCB = NULL;
    CS_SCB     *pSCB;
    i4			nevents, msec;
    i4		tim, reset_wakeme;
    i4		poll_ret;
    i4		qcnt;
    sigset_t		sigs;
    PTR                 ex_stack = NULL;
    SIZE_TYPE		npages;
    CL_ERR_DESC         err_code;
    PTR			thr_stack;
    CS_STK_CB           *stkhdr = NULL;
#ifdef LNX
    char                ex_altstack[CS_ALTSTK_SZ] __attribute__ \
						(( aligned (sizeof(PTR) * 8) ));
#endif


    /*
     * It may not yet be safe to begin. In particular, if this thread
     * was added by the server startup routine running out of
     * CSdispatch, then CSdispatch may not  have completed its work, so
     * we will wait for CSdispatch to authorize us to proceed by
     * waiting on the semaphore which it will post when it is  ready.
     * The infinite wait time is debatable, but it's not clear that we
     * can decide on a non-infinite timeout...
     */

    CS_synch_lock( &CsMultiTaskSem );
    CS_synch_unlock( &CsMultiTaskSem );

    /* make sure thread-local storage index is initialized */
    CSMT_tls_get( (PTR *)&ppSCB, &status );
    if (ppSCB == NULL)
    {
        ppSCB = (CS_SCB **) MEreqmem(0,sizeof(CS_SCB **),TRUE,NULL);
        CSMT_tls_set( (PTR)ppSCB, &status );
    }

    *ppSCB = (CS_SCB *) parm;

    pSCB = *ppSCB;

#ifdef VMS
    /* 
    ** set the thread name for debugging purposes
    ** cs_username is treated for display purposes as an asciz string
    ** however when it is initialised it seems to be padded to the full
    ** length of the field with spaces, so we'll have to move a max
    ** of 31 characters to the stack to be sure we don't overflow.
    ** 31 is the max size of any pthread object name
    */ 
    {
        char username[PTHREAD_MAX_OBJNAM_LEN+1];
        i4 l = min(sizeof(username)-1, sizeof(pSCB->cs_username));
        STlcopy (pSCB->cs_username, username, l);
        username[l] = 0;
        pthread_setname_np(pthread_self(), username, 0);
    }
#endif

    /* now make sure CSadd_thread is ready for us to run */
    CS_synch_lock( &pSCB->cs_evcb->event_sem );
    CS_synch_unlock( &pSCB->cs_evcb->event_sem );
# if !defined(xCL_NO_STACK_CACHING) && !defined(any_hpux)
    stkhdr = (CS_STK_CB *) pSCB->cs_stk_area;
    if ( stkhdr != NULL )
    {
        CS_synch_lock( &CsStackSem );
        stkhdr->cs_used = pSCB->cs_self;
        CS_synch_unlock( &CsStackSem );
    }
    else
    {
	/*
	** Store information about the system-supplied stack.
	** This will be used in case of a stack dump to keep from
	** walking the stack off the end of the stack.
	** The CS_STK_CB that is allocated here is freed at thread end.
	*/
	char *stkptr;
	i4 stksize;

        stkhdr = (CS_STK_CB *) MEreqmem(0, sizeof(CS_STK_CB), FALSE, &status);
        if ( stkhdr != NULL )
	{
            CS_get_stackinfo( &stkptr, &stksize, &status);
	    if ( status == OK )
	    {
                stkhdr->cs_size = stksize;
                stkhdr->cs_begin = stkptr - stkhdr->cs_size;
                stkhdr->cs_end = stkptr;
 
                pSCB->cs_stk_area = (char *) stkhdr;
#ifdef ibm_lnx
                pSCB->cs_registers[CS_SP] = 0;
#else
# if !defined(NO_INTERNAL_THREADS)
                pSCB->cs_sp = 0;
# endif
#endif /* ibm_lnx */
	    }
	}
    }
# endif /* !xCL_NO_STACK_CACHING && !HP */ 

# if (defined(xCL_077_BSD_MMAP) || defined(any_aix)) && !defined(USE_IDLE_THREAD) && !defined(LNX)
    /* set up an alternate stack for signal handler for this thread */
# if defined(i64_hpu)
    if (MEget_pages(ME_MZERO_MASK, 64, 0, &ex_stack, &npages, &err_code) == OK)
# elif defined(any_hpux)
    if (MEget_pages(ME_MZERO_MASK, 4, 0, &ex_stack, &npages, &err_code) == OK)
# else
    if (MEget_pages(ME_MZERO_MASK, 6, 0, &ex_stack, &npages, &err_code) == OK)
# endif  /* hpb_us5 */
    {
        if ( EXaltstack(ex_stack, (npages * ME_MPAGESIZE)) != OK )
        {
            (void)MEfree_pages(ex_stack, npages, &err_code);
	    /* Don't free alt stack again below.  Continue, hoping for
	    ** the best...
	    */
	    ex_stack = NULL;
        }
    }
# elif defined(LNX)
    /* initialize thread cpu usage */
    pSCB->cs_cpu=0;
    ex_stack = ex_altstack;
    EXaltstack(ex_stack, CS_ALTSTK_SZ);

# endif /* if defined(xCL_077_BSD_MMAP) && !defined(USE_IDLE_THREAD) */

    pSCB->cs_os_pid = CS_get_os_pid();
    pSCB->cs_os_tid = CS_get_os_tid();

    if (EXdeclare(cs_handler, &excontext)) 
    {
        pSCB->cs_mode = CS_TERMINATE;
        if (pSCB->cs_mask & CS_FATAL_MASK) 
	{
            (*Cs_srv_block.cs_elog) (E_CS00FF_FATAL_ERROR, 0, 0);

	    CSMTp_semaphore( 1, &Cs_known_list_sem );     /* exclusive */
            Cs_srv_block.cs_num_sessions--;

            if (pSCB->cs_thread_type == CS_USER_THREAD ||
		pSCB->cs_thread_type == CS_MONITOR)
	    {
                Cs_srv_block.cs_user_sessions--;
	    }
	    CSMTv_semaphore( &Cs_known_list_sem );

            CS_exit_thread(&thread_status);
        } 
	else 
	{
            (*Cs_srv_block.cs_elog) (E_CS0014_ESCAPED_EXCPTN, 0, 0);
        }
        pSCB->cs_mask |= CS_FATAL_MASK;
    }

    /* set thread priority here */

    CS_thread_setprio( pSCB->cs_thread_id, pSCB->cs_priority, &status );

    /*
     * Set session control block pointer to the server control block.
     * This field was not set at thread initialization because the
     * value may not have been known.
     */

    pSCB->cs_svcb = Cs_srv_block.cs_svcb;

#ifdef USE_IDLE_THREAD
    if  (pSCB != &Cs_idle_scb)
#endif /* USE_IDLE_THREAD */
    {
#if !defined(VMS)
	/* let the async handler threads handle the SIGUSR2 interrupts */
	sigemptyset( &sigs );
	sigaddset( &sigs, SIGUSR2 );
	CS_thread_sigmask( SIG_BLOCK, &sigs, NULL );
#endif

        if ( Cs_srv_block.cs_scbattach )
            (*Cs_srv_block.cs_scbattach)( pSCB );

        for (;;) 
        {
	    switch (pSCB->cs_mode) 
	    {
	        case CS_INITIATE:
	        case CS_TERMINATE:
	        case CS_INPUT:
	        case CS_OUTPUT:
	        case CS_READ:
		    rtn = Cs_srv_block.cs_process;
		    break;

	        default:
                    (*Cs_srv_block.cs_elog) (E_CS000C_INVALID_MODE, 0, 0);
                    pSCB->cs_mode = CS_TERMINATE;
                    continue;
            }
    
            if (pSCB->cs_mode == CS_INPUT) 
	    {
		CS_synch_lock( &pSCB->cs_evcb->event_sem );
		/* Do not set CS_EVENT_WAIT here. It will be set in CSMTsuspend.
		** If set here scc_gcomplete() call to CSresume() will
		** incorrectly believe we have already been suspended.
		*/
                pSCB->cs_mask   |= CS_INTERRUPT_MASK;
                pSCB->cs_memory |= CS_BIOR_MASK;
		CS_synch_unlock( &pSCB->cs_evcb->event_sem );

                status = (*Cs_srv_block.cs_read) (pSCB, 0);

                if (status == OK)
                    status = CSMTsuspend(CS_INTERRUPT_MASK | CS_BIOR_MASK,
                                       0,
                                       0);
		else
		{
		    CS_synch_lock( &pSCB->cs_evcb->event_sem );
		    pSCB->cs_evcb->event_state &= ~EV_WAIT_MASK;
		    pSCB->cs_state   = CS_COMPUTABLE;
		    pSCB->cs_mask   &= ~CS_INTERRUPT_MASK;
		    pSCB->cs_memory &= ~CS_BIOR_MASK;
		    CS_synch_unlock( &pSCB->cs_evcb->event_sem );
		}

                if ((status) &&
                    (status != E_CS0008_INTERRUPTED) &&
                    (status != E_CS001E_SYNC_COMPLETION)) 
	        {
                        pSCB->cs_mode = CS_TERMINATE;
                }
            } 
	    else if (pSCB->cs_mode == CS_READ) 
	    {
		CS_synch_lock( &pSCB->cs_evcb->event_sem );
		pSCB->cs_evcb->event_state &= ~(EV_WAIT_MASK | EV_EDONE_MASK);
                pSCB->cs_mode = CS_INPUT;
                pSCB->cs_mask &= ~(CS_IRCV_MASK |
                                  CS_TO_MASK |
                                  CS_ABORTED_MASK |
                                  CS_INTERRUPT_MASK |
                                  CS_TIMEOUT_MASK);
                pSCB->cs_memory = 0;
		CS_synch_unlock( &pSCB->cs_evcb->event_sem );
            }

            status = (*rtn) (pSCB->cs_mode, pSCB, &pSCB->cs_nmode);

            if (status)
                pSCB->cs_nmode = CS_TERMINATE;
            if (pSCB->cs_mode == CS_TERMINATE)
                break;

            if ((pSCB->cs_nmode == CS_OUTPUT) ||
                (pSCB->cs_nmode == CS_EXCHANGE)) 
	    {
                do 
	        {
		    CS_synch_lock( &pSCB->cs_evcb->event_sem );
		    pSCB->cs_evcb->event_state = EV_WAIT_MASK;
		    pSCB->cs_state   = CS_EVENT_WAIT;
		    pSCB->cs_mask   |= CS_INTERRUPT_MASK;
		    pSCB->cs_memory |= CS_BIOW_MASK;
		    CS_synch_unlock( &pSCB->cs_evcb->event_sem );

                    status = (*Cs_srv_block.cs_write) (pSCB, 0);
                    if (status == OK)
                        status = CSMTsuspend(CS_INTERRUPT_MASK | CS_BIOW_MASK,
                                           0,
                                           0);
		    else
		    {
			CS_synch_lock( &pSCB->cs_evcb->event_sem );
			pSCB->cs_evcb->event_state &= ~EV_WAIT_MASK;
			pSCB->cs_state   = CS_COMPUTABLE;
			pSCB->cs_mask   &= ~CS_INTERRUPT_MASK;
			pSCB->cs_memory &= ~CS_BIOW_MASK;
			CS_synch_unlock( &pSCB->cs_evcb->event_sem );
		    }
                } while ( ((status == OK) ||
                          (status == E_CS0008_INTERRUPTED)) &&
                          (pSCB->cs_mask & CS_DEAD_MASK) == 0);

                if ((status != OK) &&
                    (status != E_CS0008_INTERRUPTED) &&
                    (status != E_CS001D_NO_MORE_WRITES)) 
	        {
                    pSCB->cs_nmode = CS_TERMINATE;
                }
            }

            if ((pSCB->cs_mask & CS_DEAD_MASK) == 0) 
	    {
                switch (pSCB->cs_nmode) 
	        {
                    case CS_INITIATE:
                    case CS_INPUT:
                    case CS_OUTPUT:
                    case CS_TERMINATE:
                        pSCB->cs_mode = pSCB->cs_nmode;
                        break;

                    case CS_EXCHANGE:
                        pSCB->cs_mode = CS_INPUT;
                        break;

                    default:
                        (*Cs_srv_block.cs_elog) (E_CS000D_INVALID_RTN_MODE, 0, 0);
                        pSCB->cs_mode = CS_TERMINATE;
                        break;
                }
            } 
	    else 
	    {
                pSCB->cs_mode = CS_TERMINATE;
            }

        }
    }
#ifdef USE_IDLE_THREAD
    else    /* job is the idle job */
    {
	Cs_svinfo->csi_signal_pid = getpid();
	for (;;)
	{
	    /* Shutting down? */
	    if (Cs_srv_block.cs_mask & (CS_SHUTDOWN_MASK | CS_FINALSHUT_MASK))
	    {
		i4 user_sessions;

		CSMTp_semaphore( 1, &Cs_known_list_sem );     /* exclusive */
		user_sessions = Cs_srv_block.cs_user_sessions;
		CSMTv_semaphore( &Cs_known_list_sem );

		if (user_sessions <= 0)
		{
		    /*
		    ** If there are no user sessions running, then check for
		    ** the server shutdown mask.  If we have just entered
		    ** shutdown state (or the last user just exited while in
		    ** shutdown state) then call CSterminate to begin server
		    ** shutdown.  This will cause all DBMS tasks to be killed.
		    ** When the last thread has exited then call CSterminate
		    ** again to stop the server.
		    */
		    status = OK;
		    if ((Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK) &&
			((Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) == 0))
		    {
			CSMTterminate(CS_START_CLOSE, 0);
			CSMT_swuser();
		    }

		    /*
		    ** If we (the IdleThread) are the only remaining thread,
		    ** terminate the server.
		    */
		    if ((Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) &&
			(Cs_srv_block.cs_num_sessions == 1))
		    {
			break;
		    }
		}
	    }

	    /* If no events, clear csi_wakeme */
	    reset_wakeme = TRUE;
	    CS_TAS(&Cs_svinfo->csi_wakeme);

	    while (CS_find_events(&reset_wakeme, &nevents) == OK && nevents == 0 &&
		   Cs_srv_block.cs_ready_mask == (CS_PRIORITY_BIT >> CS_PIDLE))
	    {
		/* CS_find_events has cleared csi_wakeme (nevents == 0)*/
		if ( Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK )
		    tim = -100;
		else
		    tim = -300;
		Cs_nextpoll = Cs_polldelay;
		CS_checktime();
		msec = Cs_sm_cb->css_qcount - Cs_lastquant;
		if (msec)
		{
		    Cs_lastquant = Cs_sm_cb->css_qcount;
		}

#ifdef xCS_WAIT_INFO_DUMP
		CS_display_sess_short();
#endif /* xCS_WAIT_INFO_DUMP */

		/* save quantum timer before the poll */
		qcnt = Cs_sm_cb->css_qcount;

		CSMTsuspend( CS_INTERRUPT_MASK | CS_TIMEOUT_MASK, tim, NULL );

		CS_TAS(&Cs_svinfo->csi_wakeme);
	        if (Cs_srv_block.cs_mask & (CS_SHUTDOWN_MASK | CS_FINALSHUT_MASK)
	            || Cs_srv_block.cs_num_sessions == 1)
		    break;
            }

	    if (Cs_srv_block.cs_state == CS_IDLING)
	    {
		Cs_srv_block.cs_state = CS_PROCESSING;
		CSMT_swuser();
	    }
	    if ((Cs_srv_block.cs_state != CS_IDLING)
		&& (Cs_srv_block.cs_state != CS_PROCESSING))
	    {
		break;
	    }
	}
	if ( Cs_srv_block.cs_state != CS_TERMINATING )
	    Cs_srv_block.cs_state = CS_CLOSING;

	CSMTterminate(CS_KILL, 0);

	/* Remove the IdleThread's SCB from the know list */
	CSMTp_semaphore( 1, &Cs_known_list_sem );     /* exclusive */
	pSCB->cs_next->cs_prev = pSCB->cs_prev;
	pSCB->cs_prev->cs_next = pSCB->cs_next;

	/* Decrement session count. */
	Cs_srv_block.cs_num_sessions--;
	CSMTv_semaphore( &Cs_known_list_sem );

        CS_exit_thread(&thread_status);
    }
#endif /* USE_IDLE_THREAD */

    if ( Cs_srv_block.cs_scbdetach )
	(*Cs_srv_block.cs_scbdetach)( pSCB );

# if !defined(xCL_NO_STACK_CACHING) && !defined(any_hpux)
    if ( stkhdr != NULL )
    {
        stkhdr->cs_thread_id = pSCB->cs_thread_id;
    }
# endif /* !xCL_NO_STACK_CACHING && !HP */

    CSMT_del_thread(pSCB);

# ifndef LNX
    if ( ex_stack != NULL )
        (void)MEfree_pages(ex_stack, npages, &err_code);
# endif

# if !defined(xCL_NO_STACK_CACHING) && !defined(any_hpux)
    if ( stkhdr != NULL )
    {
	if ( Cs_srv_block.cs_stkcache == TRUE )
        {
            CS_synch_lock( &CsStackSem );
            CS_deal_stack( stkhdr );
            CS_synch_unlock( &CsStackSem );
        }
	else
	{
	    MEfree( (char *)stkhdr );
	}
    }
# endif /* !xCL_NO_STACK_CACHING && !HP */
    CS_exit_thread(&thread_status);
}

/*{
** Name: CSMT_swuser	- Do a rescheduling context switch
**
** Description:
**      If the server is not in kernel state, save the context
**      of the current session (if there is one), call
**      CS_xchng_thread to get the scb of the next session,
**      and begin executing a new session in a new context.
**
** Inputs:
**      none
**
** Outputs:
**	Returns:
**	    never
**	Exceptions:
**
**
** Side Effects:
**	    The current thread is preempted by another thread.
**
** History:
**      13-Oct-1988 (arana)
**          Created.
**      29-Mar-89 (arana)
**          Don't use chgstack system call to switch contexts if
**	    scheduler decides current thread should continue execution.
*/
VOID
CSMT_swuser()
{
    CS_yield_thread();
}


/*{
** Name: CSMT_find_scb	- Find the scb given the session id
**
** Description:
**      This session hashes the session id to obtain the session 
**      control block.  At the moment, Vax/VMS systems use
**	``Hardware Hashing'' (the most efficient).
**
** Inputs:
**      sid                             The session id to find
**
** Outputs:
**	Returns:
**	    the SCB in question, or 0 if non available
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      11-Nov-1986 (fred)
**          Created.
**
**      25-sep-1991 (mikem) integrated following change: 13-Aug-91 (dchan)
**         The decstation compiler is unable to parse the 
**         statement "scb = (CS_SCB *)((CSEV_CB *)scb)->sid"
**         Complaining about illegal size in a load/store operation.
**         The statement was broken into 2 separate statements
**         using a temporary, intermediate variable.
**	08-sep-93 (swm)
**	    Changed sid, temp_sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	18-dec-1996 (canor01)
**	    With POSIX threads, CS_SID is (CS_SCB*) since a thread id can
**	    be an arbitrary structure.  This actually simplifies CS_find_scb(),
**	    since it is passed what it needs to return.
**   04-Jun-1997 (merja01/kinpa04)
**      If using POSIX threads and scb is CS_ADDR_ID, walk scb
**      chain to find scb.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id. This eliminates the need to walk
**	    the known SCB list to find an SCB by sid.
**      08-Feb-2001 (hanal04) Bug 103872 INGSRV 1368
**          Do not assume that sid will be a valid pointer value. It
**          may actually hold a value that resides outside of the 
**          process address space and cause a SIGSEGV if cast as
**          an SCB pointer.
**	23-jan-2002 (devjo01)
**	    Backout previous change to this routine.  b103872 only
**	    occurs form iimonitor with bad user input.  Put validation,
**	    and exception handler there, and not burden this routine
**	    which needs to be efficient, and except for iimonitor, is
**	    always called with a "guaranteed" valid parameter.
**
[@history_template@]...
*/
CS_SCB *
CSMT_find_scb(CS_SID sid)
{
    CS_SCB *scb;

    /* If CS_ADDER_ID, return the static Admin thread SCB */
    if (sid == CS_ADDER_ID)
	return((CS_SCB *)&Cs_admin_scb);
    else if ((scb = (CS_SCB *)sid) && scb->cs_type == CS_SCB_TYPE)
	return(scb);
    else
	return(CS_NULL_ID);
}

/*{
** Name: CSMT_rcv_request	- Prime system to receive a request for new session
**
** Description:
**	This routine is called to queue a request to the mainline code for
**	a new thread.  It is primarily a common interface point.
**
** Inputs:
**	None.
** Outputs:
**	None.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    A new session is created.
**
** History:
**      24-Nov-1986 (fred)
**          Created.
**      7-Jan-2002 (wanfr01)
**          Bug 108641, INGSRV 1875
**          Rewrite of fix for bug 100847.
**	    On startup of admin thread, check for association_timeout
**	    config parameter, and use that is default timeout instead.
**       5-Dec-2007 (hanal04 Bug 119503 and Bug 118233
**          Bump sleep that gives other threads a chance to shut down
**          cleanly. The dmfrcp now uses a CS_SHUT and will not exit
**          if the admin thread manages to post another listen (precautionary).
**
[@history_template@]...
*/
static STATUS
CSMT_rcv_request()
{
    STATUS	    status;
    CS_SCB 	    *scb;
    static int      assoc_timeout=-1;
    char            *param;

    if (assoc_timeout == -1)
    {
        if (PMget( "$.$.$.$.association_timeout", &param ) != OK ||
            CVan( param, &assoc_timeout ) != OK )
        {
            assoc_timeout = 0;
        }
        if (assoc_timeout < 0)
           assoc_timeout = 0;
        TRdisplay("Association timeout set to %d seconds\n",assoc_timeout);
        assoc_timeout *= 1000;
    }
    CSMTget_scb(&scb);

    CS_synch_lock( &scb->cs_evcb->event_sem );
    scb->cs_evcb->event_state = EV_WAIT_MASK;
    scb->cs_state   = CS_EVENT_WAIT;
    scb->cs_mask   |= CS_INTERRUPT_MASK;
    scb->cs_memory |= CS_BIOR_MASK;
    CS_synch_unlock( &scb->cs_evcb->event_sem );

    status = (*Cs_srv_block.cs_saddr)(Cs_srv_block.cs_crb_buf, 0);

    CS_synch_lock( &scb->cs_evcb->event_sem );
    scb->cs_evcb->event_state &= ~EV_WAIT_MASK;
    scb->cs_state   = CS_COMPUTABLE;
    scb->cs_mask   &= ~CS_INTERRUPT_MASK;
    scb->cs_memory &= ~CS_BIOR_MASK;
    CS_synch_unlock( &scb->cs_evcb->event_sem );

    if (status)
    {
	CL_ERR_DESC err;

	SETCLERR(&err, status, 0);
	(*Cs_srv_block.cs_elog)(E_CS0011_BAD_CNX_STATUS, &err, 0);
	MEfill( sizeof(Cs_srv_block.cs_crb_buf), 0, Cs_srv_block.cs_crb_buf );
	return(FAIL);
    }
    else
    {
	status = CSMTsuspend( CS_INTERRUPT_MASK | CS_BIOR_MASK, assoc_timeout, 0 );
	if (Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK) 
	{
	    /* give other threads a chance to shut down cleanly */
	    if ( Cs_srv_block.cs_num_sessions > 1 )
		sleep(3);
	    status = OK;
	}
	return (status);
    }
}

/*{
** Name: CSMT_del_thread	- Delete the thread.
**
** Description:
**      This routine removes and traces of the thread from existence in the 
**      system.  This amounts, primarily, to deallocating memory.  
**
**	If the server is keeping accounting records for each session then
**	it logs the accounting message indicating the CPU utilization for this
**	thread.
**
** Inputs:
**      scb                             scb to delete
**
** Outputs:
**	Returns:
**	    STATUS
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      09-Jan-1987 (fred)
**          Created.
**	31-oct-1988 (rogerk)
**	    Don't write accounting message unless CS_ACCNTING_MASK is on.
**	    Don't write accounting records for non-user sessions.
**	14-feb-1991 (ralph)
**	    Implement session accounting
**	06-jun-91 (ralph)
**	    Revised session accounting as per review.
**	25-sep-1991 (mikem) integrated following change: 06-aug-91 (ralph)
**	    Revised UNIX Session Accounting per Sweeney:
**		- Issue error messages from CS_del_thread()
**		- Add function declarations and return types
**		- Other minor aesthetic changes
**	25-sep-1991 (mikem) integrated following change: 12-sep-91 (kirke)
**	    Changed session accounting struct to use only the basic types.
**	    The structure is the same as it was before - I merely substituted
**	    the SUN definitions for uid_t, gid_t, dev_t and time_t.  This
**	    results in the iiacct file being exactly the same format on all
**	    the common platforms.  Without this change, different platforms
**	    ended up with different formats.  For example, SUN defines uid_t
**	    as an unsigned short while HP defines it as a long.  We ignore
**	    possible individual structure element alignment, but due to the
**	    particular arrangement of this structure, it should be the same
**	    on just about all modern machines.
**	07-jan-1991 (bonobo)
**	    Added the following change from ingres63p cl: 22-oct-91 (ralph).
**	22-oct-91 (ralph)
**	    Corrected CPU time rounding formula for session accounting
**	    as per kirke to use available precision.
**	4-jul-92 (daveb)
**	    Remove threads from cs_scb_tree on CS_eradicate.
**	12-nov-1996 (canor01)
**	    The monitor thread starts life as a user thread, but transforms
**	    into a monitor thread.  cs_user_threads flag needs to reflect this.
**	02-may-1997 (canor01)
**	    Sessions created with the CS_CLEANUP_MASK will be shut
**	    down before any other system threads.  When they exit, they
**	    must call CSterminate() to allow the normal system threads
**	    to exit.
**	24-Sep-1998 (jenjo02)
**	    Eliminate cs_cnt_mutex, adjust counts while holding known_list_sem
**	    instead. Don't take CS_acct_sem unless session accounting enabled.
**	03-sep-1999 (somsa01)
**	    Moved CS_detach_scb() call to RIGHT AFTER removing the scb
**	    from the known list, protecting it with the Cs_known_list_sem
**	    semaphore. In this way, we eliminate a hole whereby, while a
**	    session is exiting, another session accessing MO objects will
**	    not obtain information about the exiting session (and its scb)
**	    before detaching its scb from MO but AFTER its physical scb was
**	    taken off the scb list. This caused SEGVs in MO routines due to
**	    the fact that they obtained null SCB pointers and tried to
**	    dereference them.
**	22-May-2002 (jenjo02)
**	    Include CS_MONITOR sessions in cs_user_sessions count to
**	    be consistent with CS_del_thread().
**	16-dec-2002 (somsa01)
**	    Changed type of ac_btime to match the type of cs_connect.
**       3-Dec-2004 (hanal04) Bug 113291 INGSRV3011
**          We must not call the TLS cleanup routines if the current
**          thread is deleting a thread associated with another thread.
**          The TLS cleanup routines use the current thread ID not
**          the thread ID from the SCB.
**          The only current case where this may happen is the admin thread
**          trying to clean up after CSMTadd_thread() failed to create a
**          new thread. We were calling the TLS cleanup routines and
**          destroying the admin thread's TLS. This lead to a server
**          crash.
*/
i4
CSMT_del_thread(CS_SCB *scb)
{
    static int		ac_ifi = -2;	/* Accounting file descriptor/status:
					**	>=0	File is open
					**	 -1	Write error; retry
					**	 -2	Initial state
					**	 -3	Sess acctg inactive
					**	 -4	Locate failed
					**	 -5	Open failed
					*/
    int			old_ifi = ac_ifi;
    i4			status;
    STATUS		local_status;
    i4			acctstat = 0;
    auto LOCATION	loc;
    auto char		*fname;
    char		username[25];
    struct passwd	*p_pswd, pswd;
    struct passwd	*getpwnam();
    char                pwnam_buf[512];
    struct passwd	*getpwuid();
    bool		same_thread = TRUE;

#if defined(VMS)
    ILE3	itmlst[] = {
		    { 0, SJC$_ACCOUNTING_MESSAGE, 0, 0},
		    { 0, 0, 0, 0}
		};
    IOSB	iosb;
    struct  {
		char	record_type[16];
		char	user_name[12];
		int	uic;
		int	connect_time;
		int	cpu_ticks;
		int	bio_cnt;
		int	dio_cnt;
	    } acc_rec;
#else
    struct {
        	char    ac_flag;	/* Unused */
        	char    ac_stat;        /* Unused */
        	unsigned short ac_uid;  /* Session's OS real uid */
        	unsigned short ac_gid;  /* Unused */
        	short   ac_tty;         /* Unused */
        	i4      ac_btime;       /* Session start time */
        	comp_t  ac_utime;       /* Session CPU time (secs*60) */
        	comp_t  ac_stime;       /* Unused */
        	comp_t  ac_etime;       /* Session elapsed time (secs*60) */
        	comp_t  ac_mem;         /* average memory usage (unused) */
        	comp_t  ac_io;          /* I/O between FE and BE */
        	comp_t  ac_rw;          /* I/O to disk by BE */
        	char    ac_comm[8];     /* "IIDBMS" */
	    }			acc_rec;
#endif

    CSMTp_semaphore( 1, &Cs_known_list_sem );     /* exclusive */
    scb->cs_next->cs_prev = scb->cs_prev;
    scb->cs_prev->cs_next = scb->cs_next;

    /* remove from known thread tree */
    CS_detach_scb( scb );

    /* Decrement session count.  If user thread decrement user session count */
    Cs_srv_block.cs_num_sessions--;
    if (scb->cs_thread_type == CS_USER_THREAD ||
        scb->cs_thread_type == CS_MONITOR)
    {
	Cs_srv_block.cs_user_sessions--;
    }
    CSMTv_semaphore( &Cs_known_list_sem );

#if !defined(VMS)
    /* Free the event wakeup block */
    CSMT_free_wakeup_block(scb);
#endif

    /* Skip all this if session accounting is not enabled */
    if (Cs_srv_block.cs_mask & CS_ACCNTING_MASK)
    {
#if defined(VMS)
        if (scb->cs_thread_type == CS_USER_THREAD)
	{
	    MEcopy("IIDBMS_SESSION: ", 16, acc_rec.record_type);
	    MEcopy((char*)scb->cs_username, 12, acc_rec.user_name);
	    acc_rec.connect_time = TMsecs() - scb->cs_connect;
	    acc_rec.cpu_ticks = scb->cs_cputime;
	    acc_rec.bio_cnt = scb->cs_bior + scb->cs_biow;
	    acc_rec.dio_cnt = scb->cs_dior + scb->cs_diow;
	    acc_rec.uic = scb->cs_uic;
	    itmlst[0].ile3$w_length = sizeof(acc_rec);
	    itmlst[0].ile3$ps_bufaddr = &acc_rec;
	    status = sys$sndjbcw(EFN$C_ENF, SJC$_WRITE_ACCOUNTING, 0, itmlst,
				 &iosb, 0, 0);
	    if (status & 1)
		status = iosb.iosb$w_status;
	}
#else
	CSMTp_semaphore(TRUE, &CS_acct_sem);
	if ((ac_ifi < 0) && (ac_ifi > -3))
	{
	    /* Attempt to open iiacct, if necessary */
	    if (NMloc(ADMIN, FILENAME, "iiacct", &loc) != OK)
	    {
		ac_ifi = -4;
		acctstat = E_CS0040_ACCT_LOCATE;
	    }
	    else
	    {
		/* iiacct file located, so open it */
		LOtos(&loc, &fname);
		ac_ifi = open(fname, O_WRONLY|O_CREAT|O_APPEND, 0666);
		if (ac_ifi < 0)
		{
		    ac_ifi = -5;
		    acctstat = E_CS0041_ACCT_OPEN;
		}
	    }
	}

	if ((ac_ifi >= 0) &&
	    (scb->cs_thread_type == CS_USER_THREAD))
	{
	    /* Format accounting record */
	    acc_rec.ac_flag		= 0;
	    acc_rec.ac_stat		= 0;
	    acc_rec.ac_gid		= 0;
	    acc_rec.ac_tty		= 0;
	    acc_rec.ac_btime	= scb->cs_connect;
	    acc_rec.ac_utime        = CS_compress(((scb->cs_cputime*60)+50)/100);
	    acc_rec.ac_stime	= 0;
	    acc_rec.ac_etime	= CS_compress((TMsecs() - scb->cs_connect)*60);
	    acc_rec.ac_mem		= 0;
	    acc_rec.ac_io		= CS_compress(scb->cs_bior + scb->cs_biow);
	    acc_rec.ac_rw		= CS_compress(scb->cs_dior + scb->cs_diow);
	    MEmove(	(u_i2)sizeof(scb->cs_username), scb->cs_username, EOS,
		    (u_i2)sizeof(username), username);
	    username[sizeof(username)-1] = EOS;
	    _VOID_ STtrmwhite(username);
	    p_pswd			= iiCLgetpwnam( username, &pswd,
						    pwnam_buf, sizeof(pwnam_buf));
	    if (p_pswd)
		acc_rec.ac_uid	= p_pswd->pw_uid;
	    else
	       acc_rec.ac_uid	= getuid();
	    MEmove(	(u_i2)sizeof("DBMS"), (PTR)"DBMS", EOS,
		    (u_i2)sizeof(acc_rec.ac_comm), (PTR)acc_rec.ac_comm);

	    /* Write accounting record */
	    if (write(ac_ifi, &acc_rec, sizeof(acc_rec)) != sizeof(acc_rec))
	    {
		_VOID_ close(ac_ifi);
		ac_ifi = -1;
		if (old_ifi != -1)
		    acctstat = E_CS0042_ACCT_WRITE;
	    }
	    else if (old_ifi == -1)
		acctstat = E_CS0043_ACCT_RESUMED;
	}
	/*
	** if we set acctstat, we may as well log it here, since we otherwise
	** have to trap it in our caller and special-case ACCT errors. better
	** keep it all in one place. IMHO CS_admin_task() should be catching
	** these, but it appears it ignores all error statuses from
	** CS_del_thread(). (sweeney)
	*/
	if (acctstat != OK)
	    (*Cs_srv_block.cs_elog)(acctstat, 0, 0);
	CSMTv_semaphore(&CS_acct_sem);
#endif
    }

    /* if we're a cleanup thread, let the other system threads die too */
    if ( scb->cs_cs_mask & CS_CLEANUP_MASK )
    {
	i4 active_count = 0;
	CSMTterminate(CS_CLOSE, &active_count);
    }

    /* The TLS cleanup routines called below destroy TLS for the
    ** current thread not the thread identifiedd in the SCB we are
    ** removing. Do NOT call the TLS routines if the SCB thread ID
    ** is not the current thread or the caller will inadvertently
    ** destroy it's own TLS. A known case is the admin thread
    ** cleaning up after CSMTadd_threa() failed. If this is the case
    ** cs_thread_id will be 0 or Cs_thread_id_init depending on the ifdef
    ** in which case we do not need to log a warning message.
    ** If we are not dealing with our known case and the SCB thread ID is
    ** not the current thread we may be leaking TLS so send a warning
    ** to the DBMS log. This should not happen.
    */
# if defined(DCE_THREADS)
    if ( !( CS_thread_id_equal( scb->cs_thread_id, CS_get_thread_id() ))
# else
    if (scb->cs_thread_id != CS_get_thread_id() )
# endif /* DCE_THREADS */
    {
# if defined(DCE_THREADS)
        if ( ! (CS_thread_id_equal( scb->cs_thread_id, Cs_thread_id_init ) ))
# else
        if (scb->cs_thread_id != 0)
# endif /* DCE_THREADS */
        {
            TRdisplay("WARNING: CSMT_del_thread() called by thread:%d for an SCB owned by thread:%d. TLS storage not destroyed.\n", CS_get_thread_id(), scb->cs_thread_id);
        }
	same_thread = FALSE;
    }

    status = (*Cs_srv_block.cs_scbdealloc)(scb);

    /* If CSMTadd_thread failed the admin thread will have called this
    ** routine to clean up after the failed session initiate. 
    ** Do not call the TLS cleanup routines, they will trash the admin thread's
    ** TLS and cause a server crash.
    */
    if(same_thread)
    {
        ME_tls_destroyall( &local_status );
        if ( local_status != OK && status == OK )
	    status = local_status;
        CSMT_tls_destroy( &local_status );
        if ( local_status != OK && status == OK )
	    status = local_status;
    }

    /*
    ** we used to return acctstat here if set. Since we now log from here
    ** (and the returned status is ignored anyway) just return status.
    ** (sweeney)
    */
    return(status);
}

/*{
** Name: cs_admin_task	- Perform administrative functions
**
** Description:
**      This routine performs the administrative functions in the server which 
**	must be performed in thread context.  These include the functions of 
**      adding threads, deleting threads, and performing accounting functions.
**
** Inputs:
**      mode                            How it is called.   Ignored.
**      scb                             the scb for the admin thread
**      next_mode                       ignored.
**      io_area                         ignored.
**	    These ignored parms are included so that this thread looks
**	    like all others.
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-jan-1987 (fred)
**          Created.
**	 7-nov-1988 (rogerk)
**	    Call CS_ssprsm directly instead of through an AST.
**	13-Dec-1992 (daveb)
**	    In CS_admin_task, separately check for CS_ADD_THREAD at
**	    the end instead of doing it in an else if.  This makes
**	    GCM completion work.
**	19-sep-1995 (nick)
**	    Remove magic number 12 ; set next_mode using the correct
**	    mask values instead.
**	16-sep-1996 (canor01)
**	    If the server state is CS_TERMINATING, do not attempt to
**	    receive another connection.
**	23-apr-1997 (canor01)
**	    Check connect limit with CSaccept_connect(), which will
**	    log failures with TRdisplay().
**	14-Apr-1999 (jenjo02)
**	    Instead of calling CSaccept_connect(), create the thread and
**	    let SCD figure out whether to accept or reject the connection.
**	02-Apr-2009 (wanfr01)
**	    Bug 121884
**	    Once detecting a shutdown via  CS_SHUTDOWN_MASK and 
**          CS_FINALSHUT_MASK, the admin thread should wait for other threads
**	     to exit and then exit.
*/
STATUS
CSMT_admin_task()
{
    CS_SCB *scb;
    sigset_t sigs;
    CL_ERR_DESC err;

#if !defined(VMS)
    /* let the async handler threads handle the SIGUSR2 interrupts */
    sigemptyset( &sigs );
    sigaddset( &sigs, SIGUSR2 );
    CS_thread_sigmask( SIG_BLOCK, &sigs, NULL );
#endif

    CSMTget_scb(&scb);

    for (;;)
    {

	if ( Cs_srv_block.cs_state == CS_TERMINATING )
	    return OK;

        if ( CSMT_rcv_request() )
            continue;

	if ((Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK) &&
	    (Cs_srv_block.cs_mask & CS_FINALSHUT_MASK))
	{
	    /* Server has started shutting down - wait for all threads to end */
	    while (Cs_srv_block.cs_num_sessions > 0)
	    {
		sleep(1);
	    }
	    return OK;
	}

	CSMTadd_thread(0, Cs_srv_block.cs_crb_buf, CS_USER_THREAD, 
			    (CS_SID*)NULL, &err);
    }
    /*NOTREACHED*/
    return FAIL;
}

/*{
** Name:	CSMT_get_thread_stats
**
** Description:
**	Obtain the stats for the thread identified in the 
**	supplied parameter block. 
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	pTSCB		Pointer to CS_TSCB parameter block
**	    pTSCB->cs_thread_id - thread-library thread-id
**	    pTSCB->cs_os_pid - OS PID of thread
**	    pTSCB->cs_os_tid - OS TID of thread
**
** Outputs:
**	    ** If CS_THREAD_STATS_CPU is set in pTSCB->cs_stats_flag:
**	    pTSCB->cs_usercpu - user cpu used by thread
**	    pTSCB->cs_systemcpu - system cpu used by thread
**
** Returns:
** 	FAIL or OK as appropriate.
**
** History:
**	09-Feb-2009 (smeke01) b119586
**	    Created.  
*/
#if defined(LNX) || defined(sparc_sol)
STATUS
CSMT_get_thread_stats(CS_THREAD_STATS_CB *pTSCB)
{
#if defined(LNX)
#define CS_MAXPROCPATH 57 /* Max /proc path is: "/proc/" + MAX_UI8_DIGITS + "/task/" + MAX_UI8_DIGITS + "/stat" = 57 */
#elif defined(sparc_sol)
#define CS_MAXPROCPATH 60 /* Max /proc path is: "/proc/" + MAX_UI8_DIGITS + "/lwp/" + MAX_UI8_DIGITS + "/lwpusage" = 60 */
#endif
    FILE*	fp;
    char	szPid[MAX_UI8_DIGITS + 1], szTid[MAX_UI8_DIGITS + 1];
    char	szPath[CS_MAXPROCPATH + 1]; 
    LOCATION	loc;
    long cutime = 0, cstime = 0;
    STATUS status;
    int numread;
#if defined(sparc_sol)
    struct prusage prusage;
#endif

    /*
    ** Check that we're being asked to do something
    ** we know how to do. At the time of writing we
    ** only know how to do thread cpu stats.
    */
    if (pTSCB->cs_stats_flag != CS_THREAD_STATS_CPU)
    {
	TRdisplay("%@ CSMT_get_thread_stats(): unknown stats request via flag value:%x\n", pTSCB->cs_stats_flag);
	return FAIL;
    }

#if defined(sparc_sol) 
    if (pTSCB->cs_thread_id != 0 && pTSCB->cs_thread_id == CS_get_thread_id() && Cs_srv_block.cs_mt)
    {
        /*
        ** Session CPU time wanted for THIS thread, and real thread time
        ** is available.
        */
        CS_THREAD_INFO_CB       thread_info;

        (VOID)CS_thread_info(&thread_info);

	pTSCB->cs_usercpu = CS_THREAD_CPU_MS_USER(thread_info);
	pTSCB->cs_systemcpu = CS_THREAD_CPU_MS_SYSTEM(thread_info);

	return OK;
    }
    /* implied else ... */
#endif

    switch (sizeof(pTSCB->cs_os_pid))
    {
	case 4:
	    CVula(pTSCB->cs_os_pid, szPid);
	    break;
	case 8:
	    CVula8(pTSCB->cs_os_pid, szPid);
	    break;
	default:
	    TRdisplay("%@ CSMT_get_thread_stats(): Unexpected OS pid size of %d\n", sizeof(pTSCB->cs_os_pid));
	    return FAIL;
    }

    switch (sizeof(pTSCB->cs_os_tid))
    {
	case 4:
	    CVula(pTSCB->cs_os_tid, szTid);
	    break;
	case 8:
	    CVula8(pTSCB->cs_os_tid, szTid);
	    break;
	default:
	    TRdisplay("%@ CSMT_get_thread_stats(): Unexpected OS tid size of %d\n", sizeof(pTSCB->cs_os_tid));
	    return FAIL;
    }

#if defined(LNX)
    STpolycat(5, "/proc/", szPid, "/task/", szTid, "/stat", szPath); 
#elif defined(sparc_sol)
    STpolycat(5, "/proc/", szPid, "/lwp/", szTid, "/lwpusage", szPath); 
#endif

    if ((status = LOfroms(PATH & FILENAME, szPath, &loc)) != OK)
    {
	szPath[CS_MAXPROCPATH + 1] = EOS;
	TRdisplay("%@ CSMT_get_thread_stats(): LOfroms() returned status:0x%0x for /proc path:%s\n", status, szPath);
	return FAIL;
    }

    if ((status = SIopen(&loc, "r", &fp)) != OK)
    {
	TRdisplay("%@ CSMT_get_thread_stats(): SIopen() returned status:0x%0x for location string: %s\n", status, loc.string);
	return FAIL;
    }

#if defined(LNX)
    if ( (numread = SIfscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*lu %*lu %*lu %*lu %*lu %lu %lu", &cutime, &cstime)) < 2 )
    {
	TRdisplay("%@ CSMT_get_thread_stats(): SIfscanf() should read 2 items from location %s, read only %d\n", loc.string, numread);
	SIclose(fp);
	return FAIL;
    }
#elif defined(sparc_sol)
    if ( (status = SIread(fp, sizeof(prusage), &numread, (char *)&prusage ) ) != OK )
    {
	TRdisplay("%@ CSMT_get_thread_stats(): SIread() returned status:0x%0x for location string: %s\n", status, loc.string);
	SIclose(fp);
	return FAIL;
    }
#endif

    /* If we got this far, we're happy with the data. If the file close is not OK, we'll treat it as a warning and continue. */
    if ( (status = SIclose(fp) ) != OK )
	TRdisplay("%@ CSMT_get_thread_stats(): SIclose() returned status:0x%0x for location string: %s\n", status, loc.string);

#if defined(LNX)
    /* Linux stores the values in 'jiffy' units, defined in asm/param.h as HZ. Convert to milliseconds. */
    pTSCB->cs_usercpu = cutime * (1000/HZ); 
    pTSCB->cs_systemcpu = cstime * (1000/HZ); 
#elif defined(sparc_sol)
    /* Solaris stores the values as seconds and nanoseconds. Convert to milliseconds. */
    pTSCB->cs_usercpu = (prusage.pr_utime.tv_sec * 1000) + (prusage.pr_utime.tv_nsec / 1000000);
    pTSCB->cs_systemcpu = (prusage.pr_stime.tv_sec * 1000) + (prusage.pr_stime.tv_nsec / 1000000);
#endif

    return OK;
}
#else
/* Not implemented, so return a FAIL */
STATUS
CSMT_get_thread_stats(CS_THREAD_STATS_CB *pTSCB)
{
    return FAIL;
}
#endif


/*
** Name: CSMT_ticker - Wall-clock watcher thread
**
** Description:
**	This routine is the entire body of the "clock watcher" thread.
**	All it does is watch time go by, and update a time-of-day
**	variable in the server block asynchronously.  The time is
**	maintained to within a second, although users must not
**	depend on that for anything critical.  (Excessive load on
**	the machine could make the ticker fall behind, and if that
**	happens the time-of-day counter may jump in erratic increments.)
**
**	This time variable can be used for pretty much anything.
**	The original motivation was to enable periodic async-message
**	checks, without each executing session having to issue timing
**	syscalls wastefully.
**
**	The time variable is "owned" by this thread and is not
**	updated in a thread-safe manner.  It's declared volatile and
**	should be accessed by users with VOLATILE_ASSIGN_MACRO
**	rather than direct assignment.
**
** Inputs:
**	scb			The CS_SCB of the ticker task
**				Note: the scb is mostly not filled in.
**
** Outputs:
**	none
**
** Side Effects:
**	Keeps Cs_srv_block.cs_ticker_time running
**
** History:
**	13-Feb-2007 (kschendel)
**	    Written.
*/

void
CSMT_ticker(void *parm)
{
#if !defined(VMS)
    CS_SCB *scb = (CS_SCB *) parm;
    SYSTIME t;

    while (Cs_srv_block.cs_state != CS_TERMINATING)
    {
	TMet(&t);
	Cs_srv_block.cs_ticker_time = t.TM_secs;

	/* Half-second sleeps should keep the clock running
	** pretty smoothly -- it doesn't have to be exactly on target.
	*/
	II_nap(500000);
    }
    /* If we happen to catch the server terminating, just exit. */
#endif /* ! VMS */

} /* CSMT_ticker */
# endif /* OS_THREADS_USED */
