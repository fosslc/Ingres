/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
**
*/

#include    <compat.h>
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
#include    <tr.h>
#include    <csev.h>
#include    <cssminfo.h>
#include    <clpoll.h>
#include    <csinternal.h>
#include    <exinternal.h>
#include    <gcccl.h>
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
#include    <pm.h>
#include    <exinternal.h>
#include    "cslocal.h"
#include    "cssampler.h"

# ifdef axp_osf
# include <unistd.h>	/* for _SC_PAGESIZE, used in mmap stack protection */
# include <obj.h>      
struct exception_info;
GLOBALDEF struct obj_list *ObjList = NULL;
/* To suppress compilation warnings from sym.h */
#include <demangle.h>
#include <excpt.h>
#include <setjmp.h>
#include <signal.h>
#include <sym.h>
#include <unistd.h>

# endif /* axp_osf */

# if defined(hp8_us5) || defined(int_lnx) || defined(int_rpl) || \
     defined(a64_lnx) || defined(thr_hpux)
#include    <ucontext.h>
# endif

# ifdef i64_hpu
#include    <unwind.h>
# endif

# if defined(sparc_sol) || defined(a64_sol)
# include <link.h>
# include <sys/asm_linkage.h>
# include <sys/frame.h>
# include <sys/stack.h>
# endif

# if defined(ibm_lnx)
void CS_ibmsetup();
# endif

# ifdef xCL_077_BSD_MMAP
# include <sys/stat.h>
# include <sys/mman.h>          /* may change on different systems */
# ifdef    xCL_007_FILE_H_EXISTS
# include    <sys/file.h>
# endif /* xCL_007_FILE_H_EXISTS */
# endif /* xCL_077_BSD_MMAP */

#if defined(any_aix)
FUNC_EXTERN VOID CS_ris_setup();
#endif

/*
** Global string expansions for CS and EV values
**
** The following macros expand the strings for such as CS_STATE_ENUM and
** CS_MASK_MASKS to generate the global *_names strings. Additionally the values
** used are checked for their order and for their being no gaps - either of
** which would cause a mismatch when displayed from such as TRdisplay.
**
** History:
**	31-Jan-2007 (kiria01) b117581
**	    Build the ev_state_mask_names, cs_state_names and
**	    cs_mask_names from the table macroes and check their
**	    consistancy.
**	09-Apr-2007 (drivi01) b117581
**	    Fix change #485729, replace "GLOBALREF const" with GLOBALCONSTREF and
**	    "GLOBALDEF const" with GLOBALCONSTDEF for cs_state_names and 
**	    cs_mask_names to prevent compile problems of c++ files. 
*/
#define _DEFINESEND

/*
** Define the comma separated lists of names as global strings
*/
#define _DEFINE(s,v) #s ","
GLOBALCONSTDEF char cs_state_names[] = CS_STATE_ENUMS ;
GLOBALCONSTDEF char cs_mask_names[] = CS_MASK_MASKS ;
#ifdef OS_THREADS_USED
GLOBALDEF const char event_state_mask_names[] = EVENT_STATE_MASKS ;
#endif /*OS_THREADS_USED*/
#undef _DEFINE

/*
** Check that there are no gaps in the numbers.
** To do this we OR the bit values and check that it
** is a number of form 2**n -1. (2**n has lower n-1 bits clear
** bug 2**n -1 has n-1 lower bits set so ANDing these needs
** to yield 0)
*/

/* Check for gaps in cs_state_enums */
#define _DEFINE(s,v) (1<<v)|
#if ((CS_STATE_ENUMS 0) & ((CS_STATE_ENUMS 0)+1))!=0
# error "cs_state_enums are not contiguous"
#endif
#undef _DEFINE

/* Check for gaps in cs_mask_masks */
#define _DEFINE(s,v) |v
#if ((0 CS_MASK_MASKS) & ((0 CS_MASK_MASKS)+1))!=0
# error "cs_mask_masks are not contiguous"
#endif

/* Check for gaps in event_state_masks */
#ifdef OS_THREADS_USED
# if ((0 EVENT_STATE_MASKS) & ((0 EVENT_STATE_MASKS)+1))!=0
#  error "event_state_masks are not contiguous"
# endif
#endif/*OS_THREADS_USED*/
#undef _DEFINE

/* Check that the values are ordered */
#define _DEFINE(s,v) >= v || v
#if ((-1) CS_STATE_ENUMS == 0)
# error "cs_state_enums are not ordered properly"
#endif
#if ((-1) CS_MASK_MASKS == 0)
# error "cs_mask_masks are not ordered properly"
#endif
#ifdef OS_THREADS_USED
# if ((-1) EVENT_STATE_MASKS == 0)
#  error "event_state_masks are not ordered properly"
# endif
#endif/*OS_THREADS_USED*/
#undef _DEFINE
#undef _DEFINESEND


/**
**
**  Name: CSHL.C - Control System "High Level" Functions
**
**  Description:
**      This module contains those routines which provide system dispatching 
**      services to the DBMS (only, at the moment).  "High level" refers to
**      those routines which can be implemented in C as opposed to macro.
**
**          CS_xchng_thread() - Pick next task and return it.
**          CS_toq_scan() - Scan timeout queue.
**          CS_alloc_stack() - Allocate a thread's stack.
**          CS_deal_stack() - Deallocate a thread's stack.
**          CS_setup() - Run the thread thru its appropriate
**			functions based on the modes set
**			by the mainline code.
**          CS_swuser() - Do a rescheduling context switch. (This function
**			  may be written in assembly language and found
**			  in csll.s)
**          CS_find_scb() - Find the SCB belonging to a particular
**			    thread id.
*          CS_eradicate() - Fully shutdown the thread -- removing
**			      it from any competion in the server.
**          CS_rcv_request() - Prime system to receive a request
**				for new thread addition.
**          CS_del_thread() - Final processing for deletion of a thread.
**          CS_admin_task() - Thread addition and deletion thread.
**          CS_exit_handler() - Final CS server processing.
**          CS_fmt_scb() - Format an SCB for display.
**          CS_last_chance() - Vax/VMS last chance exception handler to catch
**		    smashed stacks.
**	    CSdump_stack() - External way to produce CS_dump_stack()
**	    CS_wake() - Give idling process a kick.
**
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
**	12-dec-1996 (canor01)
**	    To support the sampler thread, give a monitor session a 
**	    thread type of SC_MONITOR so it won't show up as a user
**	    session to the sampler, but count it among user threads
**	    internally.
**    18-feb-1997 (hanch04)
**        As part of merging of Ingres-threaded and OS-threaded servers,
**	  check Cs_srv_block to see if server is running OS threads.
**      02-may-1997 (canor01)
**          Sessions created with the CS_CLEANUP_MASK will be shut
**          down before any other system threads.  When they exit, they
**          must call CSterminate() to allow the normal system threads
**          to exit.
**	15-may-1997 (muhpa01)
**        Fixed minor coding error (missing ';') in hp8_us5 section of code
**	      which was uncovered when OS_THREADS were turned on.
**	29-jul-1997 (walro03)
**	    Updated for Tandem NonStop (ts2_us5).
**	07-Nov-1997 (allmi01)
**	    Added entries for Silicon Graphics (sgi_us5).
**	03-nov-1997 (canor01)
**	    Make changes to CS_alloc_stack and CS_deal_stack to be able
**	    to manage stacks for use by OS threads.
**	13-Feb-98 (fanra01)
**	    Made changes to move attch of scb to MO just before we start
**	    running.
**	    This is so the SID is initialised to be used as the instance.
**	16-feb-1998 (toumi01)
**	    Added Linux (lnx_us5).
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread().
**	02-apr-1998 (canor01)
**	    When doing a stack dump, prevent walking the stack if there 
**	    is no stack information in the scb.
**	18-apr-1998 (canor01)
**	    Return status from CSMT_rcv_request.
**	24-jun-1998 (canor01)
**	    Correctly pass pointer to CL_ERR_DESC to cs_elog function.
**	05-Oct-1998 (jenjo02)
**	    In CS_update_cpu(), use real thread time, if available, instead
**	    of sliced-up process time.
**  14-Oct-1998 (merja01)
**      Added Digital UNIX specific routine CS_axp_osf_dump_stack
**      to write out diagnostic stack trace to errlog.log.
**  11-Nov-1998 (merja01)
**      Changed Digital UNIX stack dump to use CS_dump_stack
**      and added more diagnostic information to routine.
**	11-Nov-1998 (muhpa01)
**	    Added HP-UX specific routine CS_hp_dump_stack to write out
**	    exception stack trace to errlog.log (for hp8_us5 & hpb_us5).
**	16-Nov-1998 (jenjo02)
**	    Added new masks to discriminate I/O CSsuspends for reads or
**	    writes.
**	02-Dec-1998 (muhpa01)
**	    Modified CS_hp_dump_stack for changes in register save
**	    area un ucontext_t on 10.20.
**    04-Dec-1998 (kosma01)
**          sqs_ptx ran afoul of CA_Licensing on a multiprocessor box.
**          The DBMS server would catch a SIGCHLD from a Licensing child.
**          By the time the server's signal handling routine CSsigchld()
**          was executing, Licensing would have wait()ed on the child,
**          (Licensing does not catch SIGCHLD, tho). CSsigchld() would
**          blunder on and do its wait() via PCreap(). Because the server
**          has running children, (IO slaves), the server remains in wait(),
**          waiting for one of the IO slaves to die.
**	11-jan-1999 (canor01)
**	    Replace duplicate error message.
**	01-Feb-1999 (jenjo02)
**	    Count only CS_USER_THREADs in cs_num_active, which is documented to
**	    be the number of "connected (f/e) sessions". CS_INTRNL_THREAD by itself
**	    does not exclude Factotum and other "system" threads.
**	    When adding a thread, test cs_user_sessions count instead
**	    of cs_num_sessions (count of internal AND external sessions).
**	    Dynamic internal threads cause cs_num_sessions to no longer
**	    be deterministic.
**	17-Feb-1999 (matbe01)
**	    CS_join_thread for POSIX threads needs a cast of (void *)
**	    on the first parameter passed.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	10-may-1999 (walro03)
**	    Remove obsolete version string aco_u43, arx_us5, bu2_us5, bu3_us5,
**	    bul_us5, cvx_u42, dr3_us5, dr4_us5, dr5_us5, dr6_ues, dr6_uv1,
**	    dra_us5, ib1_us5, ix3_us5, odt_us5, ps2_us5, pyr_u42, rtp_us5,
**	    sqs_u42, sqs_us5, vax_ulx, x3bx_us5.
**      03-jul-1999 (podni01)
**         Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	27-jul-1999 (muhpa01)
**	    Fix function declaration for CS_hp_dump_stack() and change a couple
**	    nat's to i4.
**	15-Sep-1999 (hanch04)
**	    Print out the hostname and session id when handling exceptions
**	    in cs_handler.
**      24-Sep-1999 (hanch04)
**        Added CSget_svrid.
**      05-Oct-1999 (hanch04)
**	    Fix typo.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	13-sep-2000 (hayke02)
**	    The sqs_ptx only fix for 94452 (CA licensing problems on
**	    multiprocessor boxes - dmfrcp or iidbms waiting for iislave
**	    to die during ingstart) has been extended to all platforms
**	    (even platforms supporting OS threading can run iislaves
**	    if II_THREAD_TYPE is set to internal). This change fixes
**	    bug 98804, problem INGSRV 892.
**	18-oct-1999 (somsa01)
**	    Fixed typo in STprintf() statement in CS_hp_dump_stack() and
**	    CS_axp_osf_dump_stack().
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**      01-Jan-2000 (bonro01)
**          Enable (kosma01) change from 10-Dec-1998 for dgi.
**	27-jun-2000 (toumi01)
**	    Add int_lnx to list of machines requiring stack pages from the
**	    server stack.  (This fixed segv running with II_THREAD_TYPE
**	    INTERNAL.)
**	15-aug-2000 (somsa01)
**	    Added ibm_lnx for Linux on S390. 
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      31-aug-2000 (hweho01)
**       1) Added support for AIX 64-bit platform (ris_u64).
**       2) Removed the cast of (void *) on the first argument in   
**          CS_join_thread() call. It was needed on rel 4.2 of rs4_us5  
**          platform, but it caused error on the current release. 
**      31-aug-2000 (hweho01)
**         Added routine CS_ris64_stack() for ris_u64 platform.
**	07-Sep-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux). Modelled on axp_osf but without using 
**	    CS_axp_osf_dump_stack.
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	04-Oct-2000 (jenjo02)
**	    Implemented CS_FUNCTIONS function vector table for
**	    CS functions which have both Ingres-thread (CS) and OS-thread
**	    (CSMT) components. This eliminates CS->CSMT context
**	    switching for those functions. CS? function components
**	    renamed to IICS? to avoid conflict with function vector
**	    macros. See cs.h for a complete list.
**	30-nov-2000 (toumi01)
**	    Due to problems with OS threads on Linux, turned off OS threads
**	    for ibm_lnx.
**	20-dec-2000 (toumi01)
**	    As for ibm_lnx, make the use of CSget_stk_pages vs. MEget_pages
**	    conditional on OS_THREADS_USED.  That is, use MEget_pages for
**	    servers that use Ingres internal threading (with slaves) only.
**	    A server built with OS_THREADS_USED cannot use MEget_pages,
**	    else we get SIGSEGV on pthread library calls.  But MEget_pages
**	    is more robust in the face of stack size limitations for
**	    allocating the stack areas for internal threads.
**	13-feb-2001 (toumi01)
**	    Linux OS threading (pthread) project:
**	    For Linux use mmap MAP_ANONYMOUS pages for protection.
**	    Add defines for axp_lnx for threading.
**	22-May-2001 (hanje04)
**	    Cleaned up after bad X-integ.
**    05-jan-2001 (devjo01)
**          Fix axf_osf specific error detected by compiler.  STcpy was 
**	    used where STlcopy was intended in CS_axp_osf_dump_stack.
**	16-aug-2001 (toumi01)
**	    dummy defines for i64_aix ingres threading [non-]support
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	03-Dec-2001 (hanje04)
**	    Added support for IA64 Linux using only OS Threading (as per 
**	11-dec-2001 (somsa01)
**	    For 64-bit HP, access is NOT given to 32-bit registers but only
**	    64-bit registers.
**	    i64_aix)
**      05-jan-2001 (devjo01)
**	    Replace STcpy with STlcopy.  STcpy was wrong macro to use, since
**	    a limiting length was specified.
**	23-mar-2001 (devjo01)
**	    Changes to allow asynchronous session resume from a signal
**	    handler (b103715).  Changes include:
**	    - CS_move_async:
**	    Fixed hole where CSresume called by a signal handler would
**	    corrupt cs_as_list.  This was because UNIX does not mask
**	    signal delivery during this routine, so concurrent updates
**	    were possible.   VMS disabled AST delivery in this routine
**	    to avoid this problem.   However, a better solution is to
**	    unhook async list while inkernel is off, then process the
**	    now private list.
**	    - CS_xchg_thread, etc.
**	    Assert 'cs_inkernel' whenever mainline code is manipulating
**	    'cs_rw_q', and check for async resume afterwards.
**	    - Added CS_wake & calls to same.
**	23-jan-2002 (devjo01)
**	    Backout change 452924 to CS_find_scb.
**	26-apr-2002 (bespi01) Bug 105576 Problem INGSRV#1698
**	    CS_alloc_stack() was using the wrong field in join_thread().
**	    CS_deal_stack() was clearing the session id. This value was
**	    used subsequently in CS_alloc_stack().
**	22-May-2002 (jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	22-jul-2002 (devjo01)
**	    Corrected fix to bug 107165, so that we don't suffer memory
**	    leakage when running INTERNAL threads.
**	15-aug-2002 (devjo01)
**	    Misc. improvements to CS_axp_osf_dump_stack.
**	07-Oct-2002 (hanje04)
**	    As part of AMD x86-64 Linux port, replace individual Linux
**	    defines with generic LNX define where apropriate.
**	09-oct-2002 (somsa01)
**	    On HP-UX, do not include what is needed to usexi
**	    U_get_previous_frame() in the making of libcompat.1.sl, as
**	    the HP-UX library which contains this function (libcl.sl)
**	    contains thread-local storage. HP-UX has a drawback in that
**	    shared libraries which are dynamically loaded cannot contain
**	    thread-local storage.
**      16-Oct-2002  (hweho01)
**         1) Enabled prototype declaration CS_ris_setup() for ris_u64.
**         2) Added changes for hybrid build on AIX platform.
**	16-dec-2002 (somsa01)
**	    In CS_del_thread(), changed type of ac_btime to match the type
**	    of cs_connect.
**      22-Oct-2002 (hanal04) Bug 108986
**          Added CS_default_output_fcn() as a default output
**          function to be used by CS_dump_stack().
**	25-Apr-2003 (jenjo02) SIR 111028
**	    Added external function CSdump_stack(), wrapper to
**	    CS_dump_stack().
**	21-May-2003 (bonro01)
**	    Add support for HP Itanium (i64_hpu)    
**	29-Sep-2003 (bonro01)
**	    HP Itanium (i64_hpu) does not support stack dump.    
**	10-Oct-2003 (ahaal01)
**	    Added statement "CS_SCB *CSMT_find_scb();" back to CS_find_scb
**	    to prevent (S) level compiler error in AIX (rs4_us5) platform.
**      27-Oct-2003 (hanje04)
**          BUG 110542 - Linux Only - Ammendment
**          Minor changes to Linux altstack code to get it working on S390
**          Linux as well. Use PROT_NONE when mmap-ing guard pages for 
**	    S390 Linux as PROT_READ doesn't have desired effect.
**      27-Oct-2003 (hanje04) Bug 111164
**          For reasons unknown, the stored return address for threads on
**          S390 Linux was being set to CS_ibmsetup - 2 and not CS_ibmsetup.
**          Whilst this did work for a significant amount of time it now
**          causes the the server to return to the wrong fuction when switching
**          context on startup and everything falls appart.
**          Save return address as CS_ibmsetup for S390 Linux.
**	10-Nov-2003 (ahaal01)
**	    Backed off 10-Oct-2003 change which added statement
**	    "CS_SCB *CSMT_find_scb();" back to CS_find_scb to prevent (S)
**	    level compiler error in AIX (rs4_us5) platform.  This change was
**	    a cross-integration of change 465782 from the ingres25 code line
**	    and is not needed in the main code line.
**	07-oct-2004 (thaju02)
**	    Change pages/size to SIZE_TYPE.
**	20-Oct-2004 (devjo01)
**	    Provide stack trace routine for int_lnx.
**	15-Mar-2005 (bonro01)
**	    Added test for NO_INTERNAL_THREADS to
**	    support Solaris AMD64 a64_sol.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	    No internal thread support. 
**	06-Oct-2005 (hanje04)
**	    Enable CS_lnx_dump_stack for x86_64 Linux (a64_lnx)
**	20-Jan-2006 (bonro01)
**	    Fix bug 112483, "stacks" command in iimonitor abends Solaris
**	    64-bit servers.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**      20-Nov-2006 (hanal04) Bug 117159
**          CS_hp_dump_stack() was not working for HP2 running in 64-bit
**          mode. Used ifdef's to rewrite the code for LP64 (hp2).
**          The 32-bit code is unaffected by these changes.
**	23-Mar-2007
**	    SIR 117985
**	    Add support for 64bit PowerPC Linux (ppc_lnx)
**	13-Apr-2007 (hanje04)
**	    BUG 118063
**	    Re-enable stack tracing on x86_64 Linux and correct formatting
**	    in CS_lnx_dump_stack() for 64bit platforms. Also change format
**	    of first line of output so that it is clearer what the stack
**	    actually looks like. Static function names do not show up
**	    and this can lead to confusion because the first line of the
**	    output is formatted differently.
**	03-Oct-2007 (hanje04 for kschendel)
**	    BUG 118162
**	    Update CS_lnx_dump_stack() for x86_64.
**	04-Oct-2007 (bonro01)
**	    Add CS_sol_dump_stack() for su4_us5, su9_us5, a64_sol
**	    for both 32bit and 64bit stack tracing.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**      27-Aug-2008 (hanal04) Bug 120823
**          Add stack dump support for i64_hpu.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on and config symbols changed, fix here.
**	14-July-2009 (toumi01)
**	    Le quatorze juillet change to x86 Linux threading to end the
**	    historical use of csll.lnx.asm and use minimal low level code
**	    added to csnormal.h to handle atomic functions. This avoids
**	    RELTEXT issues that are problematic for SELinux.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, get from csinternal.
**	    Lots of prototype fixes.
**	    Untangle confusion about what stack dump output-fcn is and does.
**	23-Nov-2010 (kschendel)
**	    Drop a couple more obsolete ports missed in the last edit.
**/


/*
**  Forward and/or External function references.
*/


static STATUS CS_admin_task(i4, CS_SCB *, i4 *, PTR);
static void CS_toq_scan(i4, i4 *);	/* Scan request timeout queue */


#ifdef axp_osf
/* Digital UNIX support for CS_dump_stack */
static VOID CS_axp_osf_dump_stack(CS_SCB *, CONTEXT *, PTR, TR_OUTPUT_FCN *, i4);
#endif /* axp_osf */

# if defined(sparc_sol) || defined(a64_sol)
# if defined(sparc_sol)
static void CS_fudge_stack(CS_SCB *);
#endif
static VOID  CS_sol_dump_stack( CS_SCB *, ucontext_t *, PTR, TR_OUTPUT_FCN *, i4);
# endif

# if defined(any_hpux) && !defined(HPUX_DO_NOT_INCLUDE) && !defined(i64_hpu)

# if defined(LP64)
typedef struct current_frame_def
{
        unsigned long   cur_frsize;     /* Frame size of current routine */
        unsigned long   cursp;          /* The current value of the stack ptr */
        unsigned long   currlo;         /* PC-space of calling routine */
        unsigned long   curgp;
        unsigned long   currp;          /* Initial value of RP */
        unsigned long   curmrp;         /* Initial value of MRP */
        unsigned long   r3;         /* Initial value of sr3 */
        unsigned long   r4;         /* Initial value of sr4 */
        unsigned long   reserved[4];
} CUR_FRAME_DEF;

typedef struct {
	unsigned int no_unwind:1; /* 0..0 */
	unsigned int is_millicode:1; /* 1..1 */
	unsigned int reserved0:1; /* 2..2 */
	unsigned int region_descr:2; /* 3..4 */
	unsigned int reserved1:1; /* 5..5 */
	unsigned int entry_sr:1; /* 6..6 */
	unsigned int entry_fr:4; /* 7..10*/
	unsigned int entry_gr:5; /* 11..15*/
	unsigned int args_stored:1; /* 16..16*/
	unsigned int reserved2:3; /* 17..19*/
	unsigned int stk_overflow_chk:1; /* 20..20*/
	unsigned int two_inst_sp_inc:1; /* 21..21*/
	unsigned int reserved3:1; /* 22 */
	unsigned int c_plus_cleanup:1; /* 23 */
	unsigned int c_plus_try_catch:1; /* 24 */
	unsigned int sched_entry_seq:1; /* 25 */
	unsigned int reserved4:1; /* 26 */
	unsigned int save_sp:1; /* 27..27*/
	unsigned int save_rp:1; /* 28..28*/
	unsigned int save_mrp:1; /* 29..29*/
	unsigned int reserved5:1; /* 30..30*/
	unsigned int has_cleanup:1; /* 31..31*/
	unsigned int reserved6:1; /* 32..32*/
	unsigned int is_HPUX_int_mrkr:1; /* 33..33*/
	unsigned int large_frame_r3:1; /* 34..34*/
	unsigned int alloca_frame:1; /* 35 */
	unsigned int reserved7:1; /* 36..36*/
	unsigned int frame_size:27; /* 37..63*/
} descriptor_bits;

typedef struct { /* unwind entry as the unwind library stores it in the 
                 ** prev frame record 
                 */
descriptor_bits unwind_descriptor_bits;
	unsigned int region_start_address;
	unsigned int region_end_address;
} uw_rec_def;

typedef struct previous_frame_def
{
        unsigned long   prev_frsize;   /* Frame size of current routine */
        unsigned long   prevsp;        /* The current value of the stack ptr */
        unsigned long   prevrlo;       /* PC-space of calling routine */
        unsigned long   prevgp;        /* PC-offset of calling routine */
        uw_rec_def      prevuwr;
        long            prevuwi;
        unsigned long   r3;        /* Initial value of sr3 */
        unsigned long   r4;        /* Initial value of sr4 */
        unsigned long   reserved[4];
} PREV_FRAME_DEF;
#else
typedef struct current_frame_def
{
	unsigned int	cur_frsize;	/* Frame size of current routine */
	unsigned int	cursp;		/* The current value of the stack ptr */
	unsigned int	currls;		/* PC-space of calling routine */
	unsigned int	currlo;		/* PC-offset of calling routine */
	unsigned int	curdp;		/* Data pointer of current routine */
	unsigned int	toprp;		/* Initial value of RP */
	unsigned int	topmrp;		/* Initial value of MRP */
	unsigned int	topsr0;		/* Initial value of sr0 */
	unsigned int	topsr4;		/* Initial value of sr4 */
	unsigned int	r3;		/* Initial value of gr3 */
	unsigned int	cur_r19;	/* GR19 value of calling routine */
} CUR_FRAME_DEF;

typedef struct previous_frame_def
{
	unsigned int	prev_frsize;	/* Frame size of calling routine */
	unsigned int	prevsp;		/* SP of calling routine */
	unsigned int	prevrls;	/* PC-space of calling routine's caller */
	unsigned int	prevrlo;	/* PC-offset of calling routine's caller */
	unsigned int	prevDP;		/* Data pointer of calling routine */
	unsigned int	udescr0;	/* Low word of calling routine's unwind */
					/* descriptor */
	unsigned int	udescr1;	/* High word of calling routine's unwind */
					/* descriptor */
	unsigned int	ustart;		/* Start of the unwind region */
	unsigned int	uw_index;	/* Index into the unwind table */
	unsigned int	uend;		/* End of the unwind region */
	unsigned int	prev_r19;	/* GR19 value of caller's caller */
} PREV_FRAME_DEF;
#endif

static VOID CS_hp_dump_stack( CS_SCB *, ucontext_t *, PTR, TR_OUTPUT_FCN *, i4 );
int U_get_previous_frame( struct current_frame_def *, struct previous_frame_def * );
# endif /* any_hpux */

#ifdef i64_hpu
static VOID CS_i64_hpu_dump_stack(PTR, TR_OUTPUT_FCN *, i4 );
#endif /* i64_hpu */

#if defined(int_lnx) || defined(int_rpl) || defined(a64_lnx)
VOID CS_lnx_dump_stack(CS_SCB *, ucontext_t *, PTR, TR_OUTPUT_FCN *, i4 );
# endif /* int_lnx */

#ifdef xCS_WAIT_INFO_DUMP
static VOID	    CS_display_sess_short();
#endif /* xCS_WAIT_INFO_DUMP */

/*
** Definition of all global variables used by this file.
*/

GLOBALREF CS_SCB	      Cs_idle_scb;
GLOBALREF i4		      CSslavepids[];

GLOBALREF	CS_SMCNTRL	*Cs_sm_cb;
GLOBALREF	CS_SERV_INFO	*Cs_svinfo;
GLOBALREF	u_i4		CSservernum;

#ifdef MCT
GLOBALREF       CS_SEMAPHORE    ME_stream_sem;
GLOBALREF       CS_SEMAPHORE    NM_loc_sem;
GLOBALREF       CS_SEMAPHORE    CL_acct_sem;
GLOBALREF       CS_SEMAPHORE    *ME_page_sem;
#endif /* MCT */
GLOBALREF 	CS_SEMAPHORE	Cs_known_list_sem;

GLOBALDEF i4		Cs_lastquant;
GLOBALDEF i4			Cs_nextpoll ZERO_FILL;
GLOBALDEF i4			Cs_polldelay = 500;
GLOBALDEF i4			Cs_incomp ZERO_FILL;

GLOBALDEF     i4              Cs_polltimeout = 30000;



/*{
** Name: CS_xchng_thread	- Return the next thread to be active
**
** Description:
**      This routine is called at AST level, and will return the next 
**      thread to be activated.
**
**	This routine expects to run non-interruptable and atomic.  No
**	other session should be playing with the server scb lists while
**	this routine is looking at them.
**
**	This routine should never be called while the cs_inkernel flag is
**	set in the server control block.  If that flag is set, then
**	the scb lists may not be in a consistent state.
**
** Inputs:
**      scb                             Previous thread which was running
**
** Outputs:
**      none
**	Returns:
**	    next scb to be run
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-Oct-1986 (fred)
**          Created.
**	31-oct-1988 (rogerk)
**	    Don't save CPU time for thread unless CS_CPU_MASK is set.
**	6-Dec-1989 (anton)
**	    Removed SIprintf because stderr is not open.
**	    Added Cs_incomp flag
**	30-Jan-90 (anton)
**	   Fix idle job CPU spin in semaphore cases
**	13-Apr-90 (kimman)
**	   Adding ds3_ulx specific information.
**	07-Dec-90 (anton)
**	    Assign cpu usage to proper thread. Bug 35024
**	26-oct-93  (robf)
**          Mark picked thread as not inactive
**      20-apr-94 (mikem)
**          Bug #57043
**          Added a call to CS_checktime() to CS_xchng_thread().  This call
**          is responsible for keeping the pseudo-quantum clock up to date, 
**          this clock is used to maintain both quantum and timeout queues in
**          CS.  See csclock.c for more information.
**	15-jun-95 (popri01)
**          Add nc4_us5 to sqs_ptx-like platform ifdefs
**	 9-Jun-1995 (jenjo02)
**	    Don't scan ready queues if the only
**	    runnable thread is the idle thread. Don't remove/insert
**	    thread on rdy queue if it's already last on the queue.
**	30-jan-1997 (canor01)
**	    Don't try to call CLpoll with a NULL scb.
**	27-mar-2001 (devjo01)
**	    Protect ready queue traversal & manipulation with
**	    VMS 'inkernel' style logic.
*/

CS_SCB	*
CS_xchng_thread(CS_SCB *scb)
{
    register i4         i;
    register CS_SCB	*new_scb = NULL;
    CS_SCB		*old_scb = Cs_srv_block.cs_current;
    i4			nevents, tim;
    CL_ERR_DESC		errcode;
#ifdef	CS_STACK_SHARING
    i4			oo_stacks = 0;
#endif

# if defined(sparc_sol)
# define gotit
    if (scb &&
	scb->cs_registers[CS_SP] <=
	(SCALARP)scb->cs_stk_area + sizeof(CS_STK_CB) + 64)
# endif

# if defined(usl_us5) || defined(sgi_us5) || defined(int_lnx) || defined(int_rpl)
# define gotit
    if (scb &&
	scb->cs_sp <=
	(SCALARP)scb->cs_stk_area + sizeof(CS_STK_CB) + 64)
# endif

# if defined(hp8_us5) || defined(hpb_us5) || defined(hp2_us5)
# define gotit
    if (scb && CS_check_stack(&scb->cs_mach))
# endif

# if defined(any_aix) && defined(BUILD_ARCH32)
# define gotit
    if (scb && scb->cs_registers[CS_SP] <=
			(SCALARP)scb->cs_stk_area + sizeof(CS_STK_CB) + 64)
# endif /* aix32 */

# if defined(any_aix) && defined(BUILD_ARCH64)
# define gotit
     if (scb && scb->cs_registers[CS_SP] <=
                       (SCALARP)scb->cs_stk_area + sizeof(CS_STK_CB) + 128)
# endif /* aix64 */

#if defined(axp_osf) || defined(axp_lnx)
# define gotit
    if (scb && scb->cs_sp <=
                        (SCALARP)scb->cs_stk_area + sizeof(CS_STK_CB) + 128)

# endif /* axp_osf */

# if defined(ibm_lnx)
# define gotit
    if (scb && scb->cs_registers[CS_SP] <=
                        (SCALARP)scb->cs_stk_area + sizeof(CS_STK_CB) + 128)

# endif /* ibm_lnx */

# if defined(i64_lnx) || defined(a64_lnx) || \
     defined(NO_INTERNAL_THREADS)
# define gotit
	/* no ingres threading support !! */
# endif

# ifndef gotit
Missing_machine_dependent_scb_condition!!!
# endif

    {
	TRdisplay("Session %p killed due to stack overflow\n", scb);
	CSremove((CS_SID)scb);	/* perhaps a better action can be taken? */
    }
    if (Cs_incomp)
	CS_breakpoint();
    
    /*
    ** If we just came from the idle thread, it just took care of
    ** the following "looking for work" stuff, so we won't
    ** repeat it. (see CS_setup())
    ** If we came from a terminated session, old_scb can be NULL; don't
    ** do the poll now, since we'll probably pick the idle thread anyway.
    */
    if (old_scb && old_scb != (CS_SCB*)&Cs_idle_scb)
    {
        Cs_incomp = 1;
        tim = 0;
        (VOID) CS_find_events(&tim, &nevents);
        CS_checktime();
        tim = Cs_sm_cb->css_qcount - Cs_lastquant;
        if (tim)
        {
            Cs_lastquant = Cs_sm_cb->css_qcount;
            CS_toq_scan(tim, (i4 *)NULL);
            if ((Cs_nextpoll -= tim) < 0)
            {
                Cs_nextpoll = Cs_polldelay;
                tim = 0;
                if(iiCLpoll(&tim) == E_FAILED)
                    TRdisplay("CS_xchng_thread: iiCLpoll failed...serious\n");
            }
        }
        Cs_incomp = 0;
    }

    /*
    ** Turn on 'inkernel', so asynchronous resumes do not touch ready queues.
    */
    Cs_srv_block.cs_inkernel = 1;

    /*
    ** If the idle thread is the only thing on the ready queues,
    ** don't waste time scanning for that which won't be found.
    */
    if (Cs_srv_block.cs_ready_mask == (CS_PRIORITY_BIT >> CS_PIDLE))
    {
	new_scb = (CS_SCB*)&Cs_idle_scb;
	i = CS_PIDLE;
    }
    else for (i = CS_PADMIN; i >= CS_PIDLE; i--)
    {
	/* wander thru the queues looking for work */

	if (!(Cs_srv_block.cs_ready_mask & (CS_PRIORITY_BIT >> i)))
	    continue;

	/* For each priority in the system, check for computable threads */

	for (new_scb = Cs_srv_block.cs_rdy_que[i]->cs_rw_q.cs_q_next;
		new_scb != Cs_srv_block.cs_rdy_que[i];
		new_scb = new_scb->cs_rw_q.cs_q_next)
	{
	    if (new_scb->cs_state == CS_COMPUTABLE)
	    {
		break;
	    }
#ifdef	CS_STACK_SHARING
	    else if ((new_scb->cs_state == CS_STACK_WAIT) && (!oo_stacks))
	    {
		STATUS		status;

		status = CS_alloc_stack(new_scb, &errcode);
		if (status == OK)
		{
		    new_scb->cs_state = CS_COMPUTABLE;
		    break;
		}
		else if (status == E_CS002F_NO_FREE_STACKS)
		{
		    oo_stacks = 1;  /* We are out of stacks */
		    continue;
		}
	    }
#endif
	}

	if (new_scb != Cs_srv_block.cs_rdy_que[i])
	{
	    /* then there is a ready job here */
	    break;
	}
	else
	{
	    Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> i);
	}
    }

    if (new_scb == 0)
    {
	/*
	** Serious problems.  The idle job is always ready.
	*/
	(*Cs_srv_block.cs_elog)(E_CS0007_RDY_QUE_CORRUPT, NULL, 0, 0);
	Cs_srv_block.cs_inkernel = 0;
	if ( Cs_srv_block.cs_async )
	    CS_move_async(new_scb);
	Cs_srv_block.cs_state = CS_ERROR;
	return(scb);
    }
    
    if ((Cs_srv_block.cs_current = new_scb) == (CS_SCB *)&Cs_idle_scb)
    {
	Cs_srv_block.cs_state = CS_IDLING;
    }

    /*
    ** If newly picked thread is already last on its ready queue, don't
    ** unhook/rehook into the same position!. 
    */
    if (new_scb->cs_rw_q.cs_q_next != Cs_srv_block.cs_rdy_que[i])
    {
 	new_scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
	    new_scb->cs_rw_q.cs_q_prev;
        new_scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
	    new_scb->cs_rw_q.cs_q_next;

	/* and put on tail of ready que by priority */
	new_scb->cs_rw_q.cs_q_prev =
	    Cs_srv_block.cs_rdy_que[i]->cs_rw_q.cs_q_prev;
	new_scb->cs_rw_q.cs_q_next =
	    Cs_srv_block.cs_rdy_que[i];
	new_scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = new_scb;
	new_scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = new_scb;
    }

    Cs_srv_block.cs_inkernel = 0;
    if ( Cs_srv_block.cs_async )
	CS_move_async(new_scb);

    /*
    ** If the old or new thread needs to collect CPU statistics, then we need
    ** to figure get the DBMS CPU usage.  If the new session is collecting
    ** statistics, then we need to store the current DBMS CPU time so that
    ** when we later swap out the session, we can calculate its cpu usage.
    ** If the old session is collecting cpu stats, then increment its cpu
    ** usage.
    */
    if (old_scb && old_scb->cs_mask & CS_CPU_MASK)
    {
	CS_update_cpu((i4 *)0, &old_scb->cs_cputime);
    }
    else if (new_scb->cs_mask & CS_CPU_MASK)
    {
	CS_update_cpu((i4 *)0, (i4 *)0);
    }


#ifdef CS_MIDQUANTUM
    /* Not being suspended mid quantum so don't set it. */
    new_scb->cs_mask &= ~CS_MID_QUANTUM_MASK;	/* make sure it is off */
#endif

    return(new_scb);    
}

/*{
** Name: CS_toq_scan	- Scan timeout queue
**
** Description:
**      This routine scans the time out queue, decrementing the timeout counter 
**      for the scb's on the queue, and returning any whose count goes to zero 
**      to the ready queue.  Performing this action in this manner allows the 
**      server to lessen the AST delivery and system resource cost for running 
**      the server.
**
**	This routine should not be called when the server control block is
**	in use as it assumes that all the scb lists are consistent and takes
**	no precaution to guarantee that nobody can change them.
**
** Inputs:
**	On UNIX
**	msec		milliseconds elapsed
**
**	On VMS
**      none
**
** Outputs:
**	On UNIX
**	mintim		compute minimum timeout
**
**	On VMS
**      none
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-Oct-1986 (fred)
**          Created on Jupiter.
**	30-Jan-1990 (anton)
**	    Fixed UNIX timeout - don't return negative time.
**	13-Jun-1995 (jenjo02)
**	    Increment cs_num_active when non-internal thread put on ready queue.
**	12-Jan-1996 (jenjo02)
**	    Fixed cs_wtstatistics, which previously did nothing useful.
**	16-Jan-1996 (jenjo02)
**	    Wait reason for CSsuspend() no longer defaults to CS_LOG_MASK,
**	    which must now be explicitly stated.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
*/
VOID
CS_toq_scan(msec, mintim)
i4	msec;
i4	*mintim;
{
    CS_SCB              *scb;
    CS_SCB              *next_scb;

    for (scb = Cs_srv_block.cs_to_list->cs_rw_q.cs_q_next;
	 scb != Cs_srv_block.cs_to_list;
	 scb = next_scb)
    {
	next_scb = scb->cs_rw_q.cs_q_next;

      /*
      ** Attempt to get the scb lock.  If we can't, then skip this scb for
      ** now.  It's probably in the process of being resumed and we want to
      ** avoid the possibility of deadlocks (the holder of the scb lock may
      ** be trying to get the CS_srv_block lock.)
      */
# ifdef MCT
      if (! MCT_TESTSPIN(&scb->busy))
          continue;
# endif /* MCT */

#ifdef	UNIX
	scb->cs_timeout -= msec;
#else
	if (Cs_srv_block.cs_q_per_sec > 0)
	{
	    scb->cs_timeout -= 1;
	}
	else
	{
	    /* value is negative, ergo subtraction */ 
	    scb->cs_timeout += Cs_srv_block.cs_q_per_sec;
	}
#endif
#ifdef	UNIX
	if (mintim && scb->cs_timeout < *mintim)
	{
	    *mintim = scb->cs_timeout;
	}
#endif
	if (scb->cs_timeout <= 0)
	{
	    /* This event has timed out.  Mark it as such and put on rdy q */

	    /*
	    ** Don't gather statistics on wait times for the DBMS task
	    ** threads. They are expected to be waiting and they only
	    ** throw off the statistics.
	    */
	    if (scb->cs_memory & CS_BIO_MASK)
	    {
		if ( scb->cs_memory & CS_IOR_MASK )
		    Cs_srv_block.cs_wtstatistics.cs_bior_time +=
			Cs_sm_cb->css_qcount - scb->cs_ppid;
		else
		    Cs_srv_block.cs_wtstatistics.cs_biow_time +=
			Cs_sm_cb->css_qcount - scb->cs_ppid;
	    }
	    else if (scb->cs_memory & CS_DIO_MASK)
	    {
		if ( scb->cs_memory & CS_IOR_MASK )
		    Cs_srv_block.cs_wtstatistics.cs_dior_time +=
			Cs_sm_cb->css_qcount - scb->cs_ppid;
		else
		    Cs_srv_block.cs_wtstatistics.cs_diow_time +=
			Cs_sm_cb->css_qcount - scb->cs_ppid;
	    }
	    else
	    {
		if (scb->cs_thread_type == CS_USER_THREAD)
		{
		    if (scb->cs_memory & CS_LOCK_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lk_time +=
			    Cs_sm_cb->css_qcount - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_LKEVENT_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lke_time +=
			    Cs_sm_cb->css_qcount - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_LGEVENT_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lge_time +=
			    Cs_sm_cb->css_qcount - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_LOG_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lg_time +=
			    Cs_sm_cb->css_qcount - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_TIMEOUT_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_tm_time +=
			    Cs_sm_cb->css_qcount - scb->cs_ppid;
		    }
		}
	    }

	    /*
	    ** Turn on 'inkernel' to protect against async resumes.
	    */
	    Cs_srv_block.cs_inkernel = 1;
	    scb->cs_inkernel = 1;
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_mask |= CS_TO_MASK;
	    /* delete from timeout list */
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		scb->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		scb->cs_rw_q.cs_q_next;

	    /* and put on tail of ready que by priority */
	    scb->cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[scb->cs_priority];
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;

	    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);
	    scb->cs_inkernel = 0;
	    Cs_srv_block.cs_inkernel = 0;
	    if (Cs_srv_block.cs_async)
		 CS_move_async(scb);
	}

#ifdef MCT
	   MCT_RELSPIN(&scb->busy);
#endif /* MCT */
    }
    Cs_srv_block.cs_toq_cnt = Cs_srv_block.cs_q_per_sec;
#ifdef MCT
    MCT_RELSPIN(&Cs_srv_block.busy);
#endif /* MCT */

#ifdef	UNIX
    if (mintim && *mintim < 0)
    {
	*mintim = 0;
    }
#endif
}

#ifdef xCL_077_BSD_MMAP
    /* this ifdef has to change to take into account mprotect() */

    i4      zero_mem_fd = -1;   /* file descriptor to file of "zeros" */
    i4      do_mmap = 1;	/* debug boolean for mmap overflow protection */
    bool    mmap_check = TRUE;	/* do we need to check for II_DO_MMAP ? */
#endif


/*{
** Name: CS_alloc_stack	- Allocate a stack area for a session
**
** Description:
**      This routine allocates a stack area for a particular session. 
**      The routine will allocate the space for the stack which is 
**      currently set in the Cs_srv_block, change the protection on the 
**      pages at either end of the stack so that they cannot be read or 
**      written from user mode (to catch stack overflow and underflow),
**      place the stack into the known stack pool, marking it as in use, 
**      and initialize the sp register save area for the scb passed in.
**
**	SUN4 stack overflow protection:
**	On sun4's we allocate one additional page of memory per stack
**	(similar to the VMS implementation).  This additional page is refered
**	to as the "protection page".  The mmap() and/or mprotect() system calls
**	are used to make any access to this page (read or write) cause an 
**	access violation signal to be raised.  This change will catch some
**	set of problem caused when a stack frame crosses the stack overflow
**	boundary in between stack overflow checks already made by CS. 
**
** Inputs:
**      scb                             Scb to which this stack is to initally
**                                      belong.
**
** Outputs:
**	Returns:
**	    OK
**	    E_CS000B_NO_MEMORY		if cannot allocate a stack.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Nov-1986 (fred)
**          Created.
**      21-Apr-89 (arana)
**	    Use ME_ALIGN_MACRO to clean up pyramid-specific stack
**	    initialization.
**	12-jun-89 (rogerk)
**	    Added allocated_pages argument to MEget_pages calls.
**	12-feb-91 (mikem)
**	    Added stack overflow protection to CS_alloc_stack for sun4 based
**	    systems.  See CS_alloc_stack function header for description.
**	12-jan-94 (nrb/swm)
**	    Bug #58873
**	    On axp_osf machine range page size differs (increasing on bigger
**	    machines). Fix protection code for read-only stack page to use
**	    the max. of system page size and ME_PAGESIZE.
**	19-sep-95 (nick)
**	    Correct problems in stack guard page code ( from fixes done
**	    in ingres63p ) and also remove axp_osf only code to generalize
**	    things a bit.  Includes ...
**
**	    Stack overflow protection wasn't working as mmap() returns a
**	    caddr_t which is unsigned so even on failure the test
**	    "< (caddr_t)0" never triggers.  Changed to "== (caddr_t)-1" which
**	    works and is portable.
**
**	    File descriptor for stack overflow protection wasn't set
**	    correctly ( missing parenthesis ).
**      20-Jun-97(radve01)
**          Too restrictive PROT_NONE(non addresable) on stack guard page
**          changed to PROT_READ(read-only)
**	03-nov-1997 (canor01)
**	    When creating a stack for use with OS threads, first reap
**	    any threads that have already exited to guarantee that the
**	    stack is available for use.  Pass cs_begin as the stack 
**	    address to use by CS_create_thread.
**	22-jul-2002 (devjo01)
**	    Corrected fix to bug 107165, by ignoring return codes from
**	    CS_join_thread.  It is expected that thread may be terminated.
**      25-Jun-2003 (hanal04) Bug 110345 INGSRV 2357
**          We must not reference through the CS_SCB identified by
**          stk_hdr->cs_used when reaping stacks. The CS_SCB has already
**          been deallocated. Use the new copy held in the CS_STK_CB
**          to prevent attempts to CS_join_thread() on the wrong thread ID.
*/
STATUS
CS_alloc_stack(CS_SCB *scb, CL_ERR_DESC *errcode)
{
    i4             status = -1;
    CS_STK_CB		*stk_hdr;
    SIZE_TYPE		npages;
    i4		size;
    STATUS		*statptr;
# ifdef axp_osf
    i4			mmap_pagesize = (i4) sysconf(_SC_PAGESIZE) ;
# else
    i4			mmap_pagesize = ME_MPAGESIZE;
# endif /* axp_osf */

     /* this declaration must go at the start of a {} block */
     /* see below for companion ifdef.			    */
# if (defined(int_lnx) && defined(OS_THREADS_USED) && !defined(NO_INTERNAL_THREADS)) || \
    (defined(int_rpl) && defined(OS_THREADS_USED) && !defined(NO_INTERNAL_THREADS)) || \
    (defined(ibm_lnx) && defined(OS_THREADS_USED)) || \
    (defined(axp_lnx) && defined(OS_THREADS_USED)) || \
     defined(usl_us5)
     /* Include this code if you need stack pages from the server stack */
     CS_STK_CB *CSget_stk_pages();
#endif

    if (mmap_pagesize < ME_MPAGESIZE)
	mmap_pagesize = ME_MPAGESIZE;

    size = Cs_srv_block.cs_stksize;

# ifdef OS_THREADS_USED
    if ( CS_is_mt() )
    {
        for (stk_hdr = Cs_srv_block.cs_stk_list->cs_prev;
	        stk_hdr != Cs_srv_block.cs_stk_list;
	        stk_hdr = stk_hdr->cs_prev)
        {
	    if ( stk_hdr->cs_reapable == TRUE )
	    {
		CS_join_thread(stk_hdr->cs_thread_id, (void **)&statptr );
		stk_hdr->cs_used = 0;
		stk_hdr->cs_reapable = FALSE;
	    }
        }
    }
# endif /* OS_THREADS_USED */

    /* if true multithreaded: */
    /* NOTE: This search is protected must be protected via a semaphore	    */
    /* outside of this routine.  No provision internally is made.  This is  */
    /* normally done with the admin thread active semaphore */
    
    for (stk_hdr = Cs_srv_block.cs_stk_list->cs_next;
	    stk_hdr != Cs_srv_block.cs_stk_list && stk_hdr->cs_used;
	    stk_hdr = stk_hdr->cs_next)
    {
	/* Do nothing -- just search for a free stack area */
    }
#ifdef	CS_STACK_SHARING
    if ((stk_hdr == Cs_srv_block.cs_stk_list) &&
			(Cs_srv_block.cs_stk_count >=
					    Cs_srv_block.cs_max_active))
    {
	/* In this case, we search the wait list for sessions who can	    */
	/* lose their stacks `easily'					    */
	CS_SCB		    *victim;

	for (victim = Cs_srv_block.cs_wt_list->cs_rw_q.cs_q_next;
		victim != Cs_srv_block.cs_wt_list;
		victim = victim->cs_rw_q.cs_q_next)
	{
	    if (    (victim->cs_mask & CS_NOXACT_MASK)
		&&  (victim->cs_memory & CS_BIO_MASK)
							/* victim !in xact  */
		&&  (victim->cs_mode == CS_INPUT)	/* and doing an FE  */
							/* related read */
		)
		break;
	}
	if (victim == Cs_srv_block.cs_wt_list)
	    return(E_CS002F_NO_FREE_STACKS);
	victim->cs_mode = CS_READ;		/* this is a read who lost  */
						/* the stack -- for	    */
						/* CS_setup()		    */
	stk_hdr = victim->cs_stk_area;
	victim->cs_stk_area = NULL;
    }
    else
#endif
    if (stk_hdr == Cs_srv_block.cs_stk_list)
    {
#ifdef xCL_077_BSD_MMAP
	/* this ifdef has to change to take into account mprotect() */

	PTR     prot_page_ptr;
#endif
	SIZE_TYPE internal_stacksize = size;

	internal_stacksize =
	    (size + sizeof(CS_STK_CB) + ME_MPAGESIZE) / ME_MPAGESIZE;

#ifdef xCL_077_BSD_MMAP
	/* this ifdef has to change to take into account mprotect() */

	/*
	** When mmap_pagesize is the same as ME_MPAGESIZE we only need
	** to add one more protection page to the end of the stack.
	**
	** When mmap_pagesize is greater than ME_MPAGESIZE we need to
	** increase the internal_stacksize to accommodate a larger
	** protection page (i.e. mmap_pagesize) AND to allow for the
	** alignment of the protection page on an mmap_pagesize boundary.
	** (mmap_pagesize needs to be represented as 'n' ME_MPAGESIZE pages).
	*/
	if (mmap_pagesize == ME_MPAGESIZE)
		/* add one more "protection page" to end of stack */
		internal_stacksize++;
	else
		/*
		** add 2 * 'n' "protection pages" to end of stack
		** 'n' for protection and 'n' for alignment
		*/
		internal_stacksize += ((mmap_pagesize / ME_MPAGESIZE) * 2) ;
#endif /* xCL_077_BSD_MMAP */

	/* We need to allocate a new stack */
# if (defined(int_lnx) && defined(OS_THREADS_USED) && !defined(NO_INTERNAL_THREADS)) || \
    (defined(int_rpl) && defined(OS_THREADS_USED) && !defined(NO_INTERNAL_THREADS)) || \
    (defined(ibm_lnx) && defined(OS_THREADS_USED)) || \
    (defined(axp_lnx) && defined(OS_THREADS_USED)) || \
     defined(usl_us5)
        /* Include this code if you need stack pages from the server stack */

        npages = (size+sizeof(CS_STK_CB)+ME_MPAGESIZE) / ME_MPAGESIZE;

        stk_hdr = CSget_stk_pages(npages);
# else
#ifdef MCT
	if (ME_page_sem)
	    gen_Psem(ME_page_sem);
#endif
        status = MEget_pages(ME_MZERO_MASK, internal_stacksize,
				0, (PTR *)&stk_hdr, &npages, errcode);
#ifdef MCT
	if (ME_page_sem)
	    gen_Vsem(ME_page_sem);
#endif
        if ( status != OK )
	    return(E_CS002F_NO_FREE_STACKS);
# endif	/* stack pages */

	stk_hdr->cs_begin = (char *)stk_hdr + sizeof(*stk_hdr);
	stk_hdr->cs_size = npages * ME_MPAGESIZE;
# if defined(ibm_lnx)
	stk_hdr->cs_end = (char *)stk_hdr + stk_hdr->cs_size - 1024;

	stk_hdr->cs_orig_sp = (PTR)((i4 *)stk_hdr->cs_end);
# else
	stk_hdr->cs_end = (char *)stk_hdr + stk_hdr->cs_size;

	stk_hdr->cs_orig_sp = (PTR)((i4 *)stk_hdr->cs_end-2);
# endif /* ibm_lnx */

#ifdef xCL_077_BSD_MMAP
	/* this ifdef has to change to take into account mprotect() */

	/* 
	** The "protection page" begins on next higher page boundary above
	** the stk_hdr structure in memory.  Adjust the "beginning" of stack
	** memory to start at the next higher page boundary above the protected
	** page; so the stk_hdr is separated from the stack itself by the
	** protected page.
	*/
	prot_page_ptr = (PTR)ME_ALIGN_MACRO(
		    (char *)stk_hdr + sizeof(*stk_hdr), mmap_pagesize);
	stk_hdr->cs_begin = (char *)prot_page_ptr + mmap_pagesize;

	if (mmap_check)
	{
	    char *mmapstr;

	    NMgtAt("II_DO_MMAP", &mmapstr);
	    if (mmapstr && *mmapstr)
	    {
		if ((STcompare(mmapstr, "FALSE") == 0) || 
			(STcompare(mmapstr, "false") == 0))
		    do_mmap = 0;
	    }

	    mmap_check = FALSE;
	}

#if defined(sparc_sol) || defined(usl_us5) || \
    defined(axp_osf) || defined(any_aix) || \
    defined(LNX) || defined(OSX)

	/* 
	** Try to remap the "protection page" so no real memory is used by it
	** and protect it from read/write access to detect stack overflow.
	*/
# ifdef MAP_ANONYMOUS
        /* 
        ** On OSF the MAP_ANONYMOUS flag can be specified to mmap() in
        ** combination with PROT_NONE, and with fd set to -1. This
        ** maps a page of zeroes which cannot be accessed into the
        ** virtual address space at the specified address.
	** >>>PROT_NONE changed to PROT_READ!!!
        */
        if (do_mmap)
        {
/* PROT_READ doesn't have the desired effect on S/390 Linux */
#ifdef ibm_lnx 
            if (mmap(prot_page_ptr, mmap_pagesize, PROT_NONE,
                MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, (off_t)0)
#else
            if (mmap(prot_page_ptr, mmap_pagesize, PROT_READ,
                MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, (off_t)0)
#endif
		== (caddr_t)-1)
            {
                perror("mmap MAP_ANONYMOUS");
            }
        }
# else /* MAP_ANONYMOUS */
        /*
	** On SunOS 4.1 & SystemVr4, /dev/zero is a special device that always
	** contains zeros.  We use the mmap() call to map in a page of zeros
	** copy-on-write and protect the page at the same time.  Mmap remaps
	** the virtual page and frees any real memory used for that page. Map
	** also protects the page before it can be faulted; so, no real
	** memory is associated with the virtual page.
	**
	** If mmap() fails, just use mprotect() to protect the page.  The
	** real memory will be freed by the pager eventually anyway.
	*/
	if (zero_mem_fd >= 0 || 
	    ((zero_mem_fd = open("/dev/zero", O_RDWR)) >= 0))
	{
	    if (do_mmap)
	    {
		if (mmap(prot_page_ptr, mmap_pagesize, PROT_READ,
		    MAP_PRIVATE|MAP_FIXED, zero_mem_fd, (off_t)0)
			== (caddr_t)-1)
		{
		    if (mprotect(prot_page_ptr, mmap_pagesize, PROT_READ) < 0)
		    {
			perror("mprotect");
		    }
		}
	    }
	}
# endif /* MAP_ANONYMOUS */
	else
#endif /* su4_u42 sun_u42 dr6_us5 su4_us5 su4_cmw */
	{
	    /*
	    **  Not sure what to do for non-OSF, non-Sun, non-SystemVr4 
	    **  systems.
	    */
	}
#endif /* xCL_077_BSD_MMAP */


	/* Now that the stack header is filled in, lets add it to the list  */
	/* of known stacks.  It is added to the end of the list as the	    */
	/* newest thread is the least likely to give up its stack...	    */

	stk_hdr->cs_next = Cs_srv_block.cs_stk_list;
	stk_hdr->cs_prev = Cs_srv_block.cs_stk_list->cs_prev;
	stk_hdr->cs_next->cs_prev = stk_hdr;
	stk_hdr->cs_prev->cs_next = stk_hdr;
	Cs_srv_block.cs_stk_count++;
    }
    scb->cs_stk_area = stk_hdr;
    stk_hdr->cs_used = scb->cs_self;
    scb->cs_stk_size = size;

# ifdef OS_THREADS_USED
    if ( CS_is_mt() )
    {
        stk_hdr->cs_reapable = FALSE;
# ifndef axp_osf
# ifdef ibm_lnx
        scb->cs_registers[CS_SP] = (long)stk_hdr->cs_begin;
# else
# if defined(i64_lnx) || defined(a64_lnx) || \
     defined(NO_INTERNAL_THREADS)
	/* no ingres threading support !! */
# else
        scb->cs_sp = (long)stk_hdr->cs_begin;
# endif
# endif /* ibm_lnx */
# else /* set axp's stack pointer */
# define        SAVEREG_FRAME_SIZE      160
    scb->cs_sp = (long)stk_hdr->cs_orig_sp - (long)SAVEREG_FRAME_SIZE;
# endif
    }
    else
    {
# endif /* OS_THREADS_USED */

# if defined(sparc_sol)
# define got1
    scb->cs_registers[CS_SP] = (long)stk_hdr->cs_orig_sp;
    CS_fudge_stack(scb);
# endif

# if defined(usl_us5) || defined(int_lnx) || defined(int_rpl)
# define got1
     scb->cs_sp = (long)stk_hdr->cs_orig_sp;
     CS_sqs_pushf(scb, CS_eradicate);
# endif

# if defined(ibm_lnx)
# define got1
  scb->cs_exsp = NULL;          /* initialize exception stack */
  scb->cs_registers[CS_SP] = (long)stk_hdr->cs_orig_sp;
  scb->cs_registers[CS_RETADDR] = (long)CS_ibmsetup;
  scb->cs_ca = (long)CS_ibmsetup;
  scb->cs_id = 100;
# endif /* ibm_lnx */

# if defined(sgi_us5)
# define got1
    scb->cs_sp = (long)stk_hdr->cs_orig_sp;
# endif

# if defined(any_hpux)
# define got1
/* Note: will set up the stack when assigning initial starting addr */
#endif

# if defined(any_aix) && defined(BUILD_ARCH32)
# define got1
    scb->cs_registers[CS_SP] = (long)stk_hdr->cs_orig_sp;
    CS_ris_stack(scb);
# endif

# if defined(any_aix) && defined(BUILD_ARCH64)
# define got1
     scb->cs_registers[CS_SP] = (long)stk_hdr->cs_orig_sp;
     CS_ris64_stack(scb);
# endif

#if defined(axp_osf) || defined(axp_lnx)
# define got1
    /*
    ** Most registers are saved on the stack at a context switch,
    ** so initially, allow for stack size adjustment (see csll.axp.asm
    ** for explanation of choice of value for SAVEREG_FRAME_SIZE)
    */
# define        SAVEREG_FRAME_SIZE      160
    scb->cs_sp = (long)stk_hdr->cs_orig_sp - (long)SAVEREG_FRAME_SIZE;
# endif
# ifdef OS_THREADS_USED
    }
# endif /* OS_THREADS_USED */

# if defined(i64_lnx) || defined(a64_lnx)

# define got1
	/* no ingres threading support !! */
# endif

# if defined(NO_INTERNAL_THREADS)
# define got1
	/* no ingres threading support !! */
# endif /* NO_INTERNAL_THREADS */

# ifndef got1
# error Missing_machine_dependant_stack_allocation!
# endif

    return(OK);
}

/*{
** Name: CS_deal_stack	- Deallocate a stack area
**
** Description:
**	This routine simply deallocates a stack created above.  The
**	stack needs to be fully decoupled from any use or else very
**	strange behaviour will ensue.
**
** Inputs:
**      stk_hdr                         Stack to be deleted.
**
** Outputs:
**      None
**	Returns:
**	    Status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-Nov-1987 (fred)
**          Created.
**	2-oct-93 (swm)
**	    Bug #56447
**	    Cast assignment to cs_used to CS_SID, now that cs_used type has
**	    changed so that CS_SID values can be stored without truncation.
**	03-nov-1997 (canor01)
**	    When deallocating stacks used by OS threads, the stack cannot
**	    be used until another thread has reaped it. Just mark it 
**	    reapable for now.
**	22-jul-2002 (devjo01)
**	    Corrected fix to bug 107165, so that we don't suffer memory
**	    leakage when running INTERNAL threads.
[@history_template@]...
*/
STATUS
CS_deal_stack(CS_STK_CB *stk_hdr)
{

# ifdef OS_THREADS_USED
    if ( CS_is_mt() )
    {
        stk_hdr->cs_reapable = TRUE;
    }
    else
# endif /* OS_THREADS_USED */
	stk_hdr->cs_used = (CS_SID)0;

    return(OK);
}

/*{
** Name: CS_setup()	- Place bottom level calls to indicated procedures
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
**	16-Nov-1998 (jenjo02)
**	    Added new masks to discriminate I/O CSsuspends for reads or
**	    writes.
*/
void
CS_setup(void)
{
    CS_SCB              *scb = Cs_srv_block.cs_current;
    STATUS		status;
    EX_CONTEXT		excontext;
    CL_ERR_DESC		errdesc;
    char		*cp;
    i4			nevents, msec;
    i4		tim;
    i4		poll_ret;
    i4		qcnt;

    /* Catch escaped exceptions */
    
    if (EXdeclare(cs_handler, &excontext))
    {
	if (Cs_incomp || Cs_srv_block.cs_current != scb)
	{
	    cp = "Fatal error inside event completion code - server FATAL";
	    TRdisplay(cp);
	    ERsend(ER_ERROR_MSG, cp, STlength(cp), &errdesc);
	    if (scb->cs_mode == CS_TERMINATE)
	    {
		/* Very very bad recursion.  Exit straight away */
		_exit(1);
	    }
	    scb->cs_mode = CS_TERMINATE;
	    _VOID_ CSterminate(CS_CRASH, 0);
	}

	scb->cs_mode = CS_TERMINATE;
	if (scb->cs_mask & CS_FATAL_MASK)
	{
	    (*Cs_srv_block.cs_elog)(E_CS00FF_FATAL_ERROR, NULL, 0, 0);
	    EXdelete();
	}
	else
	{
	    (*Cs_srv_block.cs_elog)(E_CS0014_ESCAPED_EXCPTN, NULL, 0, 0);
	}
	scb->cs_mask |= CS_FATAL_MASK;
    }

    /*
    ** Set session control block pointer to the server control block.
    ** This field was not set at thread initialization because the
    ** value may not have been known.
    */
    scb->cs_svcb = Cs_srv_block.cs_svcb;

    if  ((scb != (CS_SCB *)&Cs_admin_scb) && (scb != (CS_SCB *)&Cs_idle_scb))
    {
	if ( Cs_srv_block.cs_scbattach )
	    (*Cs_srv_block.cs_scbattach)( scb );

	for (;;)
	{
	    switch (scb->cs_mode)
	    {
		case CS_INITIATE:
		case CS_TERMINATE:
		case CS_INPUT:
		case CS_OUTPUT:
		case CS_READ:
		    break;

		default:
		    (*Cs_srv_block.cs_elog)(E_CS000C_INVALID_MODE, NULL, 0, 0);
		    scb->cs_mode = CS_TERMINATE;
		    continue;
	    }

	    
	    if (scb->cs_mode == CS_INPUT)
	    {
#ifdef	CS_SYNC
		status = (*Cs_srv_block.cs_read)(scb, TRUE);
#else
		status = (*Cs_srv_block.cs_read)(scb, 0);
		if (status == OK)
		    status = CSsuspend(CS_INTERRUPT_MASK | CS_BIOR_MASK, 0, 0);
#endif
		if ((status) &&
			(status != E_CS0008_INTERRUPTED)    &&
			(status != E_CS001E_SYNC_COMPLETION))
		    scb->cs_mode = CS_TERMINATE;
	    }
	    else if (scb->cs_mode == CS_READ)
	    {
		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;

		scb->cs_mode = CS_INPUT;

		/* Must complete the post-processing that would have been   */
		/* done by CSsuspend() -- we never got to end...	    */

#ifdef MCT
		MCT_GETSPIN(&scb->busy);
#endif /* MCT */
		scb->cs_mask &= ~(CS_EDONE_MASK | CS_IRCV_MASK
					| CS_TO_MASK | CS_ABORTED_MASK
					| CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);
		scb->cs_memory = 0;
#ifdef MCT
	 	MCT_RELSPIN(&scb->busy);
#endif /* MCT */

		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
	    }

	    status =
		(*Cs_srv_block.cs_process)(scb->cs_mode,
			    scb,
			    &scb->cs_nmode);
	    if (status)
		scb->cs_nmode = CS_TERMINATE;
	    if (scb->cs_mode == CS_TERMINATE)
		break;

	    if ((scb->cs_nmode == CS_OUTPUT) || (scb->cs_nmode == CS_EXCHANGE))
	    {
		do
		{
#ifdef	CS_SYNC
		    status = (*Cs_srv_block.cs_write)(scb, TRUE);
#else
		    status = (*Cs_srv_block.cs_write)(scb, 0);
		    if (status == OK)
			status = CSsuspend(CS_INTERRUPT_MASK | 
					   CS_BIOW_MASK, 0, 0);
#endif
		} while ((status == OK) || (status == E_CS0008_INTERRUPTED));
		if (	(status) &&
			(status != E_CS0008_INTERRUPTED) &&
			(status != E_CS001D_NO_MORE_WRITES) )
		    scb->cs_nmode = CS_TERMINATE;
	    }
	    
	    if ((scb->cs_mask & CS_DEAD_MASK) == 0)
	    {
		switch (scb->cs_nmode)
		{
		    case CS_INITIATE:
		    case CS_INPUT:
		    case CS_OUTPUT:
		    case CS_TERMINATE:
			scb->cs_mode = scb->cs_nmode;
			break;

		    case CS_EXCHANGE:
			scb->cs_mode = CS_INPUT;
			break;

		    default:
			(*Cs_srv_block.cs_elog)(E_CS000D_INVALID_RTN_MODE, NULL, 0, 0);
			scb->cs_mode = CS_TERMINATE;
			break;
		}
	    }
	    else
	    {
		scb->cs_mode = CS_TERMINATE;
	    }
	}
	if (scb->cs_sem_count)
	{
	    (*Cs_srv_block.cs_elog)(E_CS001C_TERM_W_SEM, NULL, 0, 0);
	}
	EXdelete();
    }
    else if (scb == (CS_SCB *)&Cs_admin_scb)
    {
	auto char	*str;

	NMgtAt("II_POLLDELAY", &str);
	if (str && *str)
	{
	    CVan(str, &Cs_polldelay);
	}
	if (scb->cs_mode == CS_TERMINATE)
	{
	    (VOID) CSterminate(CS_KILL, 0);
	}
	CS_admin_task(CS_OUTPUT, scb, &scb->cs_nmode, 0);
    }
    else    /* job is the idle job */
    {
	if (scb->cs_mode == CS_TERMINATE)
	    (VOID) CSterminate(CS_KILL, 0);
	for (;;)
	{
	    Cs_srv_block.cs_state = CS_IDLING;
	    if (Cs_srv_block.cs_user_sessions == 0)
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
		    CSterminate(CS_START_CLOSE, 0);
		    CS_swuser();
		}

		/*
		** If there are no more threads, then terminate the server.
		*/
		if ((Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) &&
		    (Cs_srv_block.cs_num_sessions == 0))
		{
		    break;
		}
	    }

	    if (Cs_incomp)
		CS_breakpoint();
	    Cs_incomp = 1;
	    i_EXsetothersig(SIGUSR2, iiCLintrp );
	    while ((CS_ACLR(&Cs_svinfo->csi_wakeme)),
		   CS_find_events(&tim, &nevents) == OK && nevents == 0 &&
		   Cs_srv_block.cs_ready_mask == (CS_PRIORITY_BIT >> CS_PIDLE))
	    {
		tim = 90000;
		Cs_nextpoll = Cs_polldelay;
		CS_checktime();
		msec = Cs_sm_cb->css_qcount - Cs_lastquant;
		if (msec)
		{
		    Cs_lastquant = Cs_sm_cb->css_qcount;
		}
		CS_toq_scan(msec, &tim);

#ifdef xCS_WAIT_INFO_DUMP
		CS_display_sess_short();
#endif /* xCS_WAIT_INFO_DUMP */

		if ( Cs_srv_block.cs_ready_mask ==
                      (CS_PRIORITY_BIT >> CS_PIDLE) )
		{
		    /* save quantum timer before the poll */
		    qcnt = Cs_sm_cb->css_qcount;

		    if ((poll_ret = iiCLpoll(&tim)) == E_FAILED)
			TRdisplay("CS_setup: iiCLpoll failed..serious\n");

		/* 
		** When sessions which block on cross process semaphores, they
		** spin in a loop which eventually degrades into a spin calling
		** CSsuspend() with a 1 second timeout.  In some timing 
		** specific cases the RCP can get stuck in a state (eg. 
		** waiting on the LGK semaphore held by one of these sessions),
		** where it can not call CS_ADD_QUANTUM() to bump the quantum 
		** count.  The end result is that the sessions sleeping for 1 
		** second never get woken up.  This a timing/semaphore deadlock
		** situation will eventually lead to the installation "hanging"
		** waiting on the LGK semaphore.
		*/
		    if ((poll_ret == E_TIMEOUT) && 
			(qcnt == Cs_sm_cb->css_qcount))
		    {
			CS_ADD_QUANTUM(tim);
		    }
		}

		CS_TAS(&Cs_svinfo->csi_events_outstanding);
	    }
	    CS_TAS(&Cs_svinfo->csi_wakeme);
	    Cs_incomp = 0;
	    i_EXsetothersig(SIGUSR2, SIG_IGN );

	    if (Cs_srv_block.cs_state == CS_IDLING)
	    {
		Cs_srv_block.cs_state = CS_PROCESSING;
		CS_swuser();
	    }
	    if ((Cs_srv_block.cs_state != CS_IDLING)
		&& (Cs_srv_block.cs_state != CS_PROCESSING))
	    {
		break;
	    }
	}
	Cs_srv_block.cs_state = CS_CLOSING;

	CSterminate(CS_KILL, 0);
    }
    EXdelete();
}


# ifdef OSX
/*
** Usually these functions would be in csll.mac.asm however as these will
** eventually be migrated,  as __inline__ functions, to csnormal.h so it 
** was decided to keep them here for now.
*/
/*
** Name: CS_aclr()  - Atomic Clear Operation
**
** Description:
**	ASM function for atomically clearing the zeroth bit
**	at the specified address.
**
** History:
**	Jun-2005 (hanje04)
**	    Created.
**	05-Feb-2007 (hanje04)
**	    No longer needed, defined using system macros in csnormal.h
** 
__asm__ void
CS_aclr( CS_ASET *addr )    // -> register (r3)
{
	sync
	li r4,0        // Is there a better way to zero a register?
	stw r4,0(r3)    // *addr = 0
	blr
}
*/
# endif /* OSX */

# if defined(OSX) || defined(ppc_lnx) || \
     (defined(int_lnx) && defined(NO_INTERNAL_THREADS)) || \
     (defined(int_rpl) && defined(NO_INTERNAL_THREADS))
/*
** Name: CS_swuser() 
**
** Description:
**	Dummy context switch function, only needed for satisfy linker
**	as we don't support INTERNAL_THREADS. 
**
** History:
**	26-Mar-2007 (hanje04)
**	    Created.
**	14-July-2009 (toumi01)
**	    Change #ifdefs to use this for any platforms where we need
**	    a dummy CS_swuser because INTERNAL_THREADS are not supported.
** 
*/
void
CS_swuser(void)
{
        return;
}
# endif /* OSX || ppc_lnx || ( int_lnx || int_rpl && NO_INTERNAL_THREADS ) */

/*{
** Name: CS_swuser	- Do a rescheduling context switch
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

# if defined(any_hpux)
VOID
CS_swuser(void)
/**********************************************************************
CS_swuser switches to a new user.  This routine isolates the machine
   independent stuff from the the machine specific stuff.
**********************************************************************/
    {
    extern long   ex_sptr;
    CS_SCB        *new_scb, *old_scb, dummy;

# ifdef OS_THREADS_USED
    if (Cs_srv_block.cs_mt)
    {
        CSMT_swuser();
        return;
    }
# endif /* OS_THREADS_USED */

    /* if server is in kernel stat, no task switching */
    if (Cs_srv_block.cs_inkernel)
        return;

    /* get the next thread from the scheduler */
    old_scb = Cs_srv_block.cs_current;
    new_scb = CS_xchng_thread(old_scb);

    /* if haven't changed threads, then done */
    if (new_scb == old_scb)
        return;

    /* This case can be handled more efficiently, but will assume not common*/
    if (old_scb == NULL)
       old_scb = &dummy;

    /* switch to the new exception stack */
    old_scb->cs_exsp = ex_sptr;
    ex_sptr = new_scb->cs_exsp;

    /* change to the new thread */
    CS_swap_thread(&old_scb->cs_mach, &new_scb->cs_mach);
    }


# endif	/* hp8_us5, hp8_bls */ 

/*{
** Name: CS_find_scb	- Find the scb given the session id
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICS_find_scb, removed cs_mt test.
**      08-Feb-2001 (hanal04) Bug 103872 INGSRV 1368
**          Do not assume sid is a valid pointer value. Search the SCB
**          list for a match. We do not have a global event list to scan
**          so we must add an exception handler if we resort to casting
**          sid.
**	23-jan-2002 (devjo01)
**	    Backout previous change to this routine.  b103872 only
**	    occurs form iimonitor with bad user input.  Put validation,
**	    and exception handler there, and not burden this routine
**	    which needs to be efficient, and except for iimonitor, is
**	    always called with a "guaranteed" valid parameter.
**
[@history_template@]...
*/
CS_SCB	*
IICS_find_scb(sid)
CS_SID                sid;
{
    register	CS_SCB	*scb;

    if (sid == CS_ADDER_ID)
	return((CS_SCB *) 0);
    scb = (CS_SCB *)sid;
    if (scb->cs_type == CS_SCB_TYPE)
	return(scb);

    /* If we are here, then it better be an event control block */

    {
      CS_SID temp_sid = ((CSEV_CB *)scb)->sid;
      scb = (CS_SCB *)temp_sid;
    }

    if (scb && scb->cs_type == CS_SCB_TYPE)
	return(scb);
    return((CS_SCB *)0);
}

/*{
** Name: CS_eradicate	- Eradicate the current thread
**
** Description:
**      This routine is called by the current thread to cause itself 
**      to be removed from the system.  This routine always removes 
**      the thread in control, so care should be taken with its 
**      use.  To remove other threads, use CSremove(). 
** 
**	This is done by placing removing the thread from any ready
**	or wait queues, placing the thread on the list of threads
**	to be evaporated by the admin thread, and by scheduling some
**	other thread.
**
** Inputs:
**      none                            None
**
** Outputs:
**	Returns:
**	    Doesn't
**	Exceptions:
**	    none
**
** Side Effects:
**	    The current thread is removed.
**
** History:
**      12-Nov-1986 (fred)
**          Created.
**	 7-nov-1988 (rogerk)
**	    If eradicating last thread for a particular priority then turn off
**	    the ready mask for that priority.
**	    Call CS_ssprsm directly instead of through an AST.
**	13-Jun-1995 (jenjo02)
**	    Decrement cs_num_active if non-internal thread was on rdy queue.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	27-mar-2001 (devjo01)
**	    Protect queue manipulations with 'inkernel'.
[@history_template@]...
*/
VOID
CS_eradicate(void)
{
    CS_SCB              *scb = Cs_srv_block.cs_current;

    CSp_semaphore(TRUE, &Cs_admin_scb.csa_sem);

    if ( scb->cs_state == CS_COMPUTABLE &&
	 (scb->cs_thread_type == CS_USER_THREAD ||
	  scb->cs_thread_type == CS_MONITOR) )
    {
	Cs_srv_block.cs_num_active--;
    }

    Cs_srv_block.cs_inkernel = 1;
    scb->cs_inkernel = 1;
    scb->cs_state = CS_FREE;
    /* remove from whatever queue(s) it is on */
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb->cs_rw_q.cs_q_next;

#ifdef MCT
    MCT_GETSPIN(&Cs_srv_block.busy);
#endif /* MCT */

    scb->cs_next->cs_prev = scb->cs_prev;
    scb->cs_prev->cs_next = scb->cs_next;

#ifdef MCT
    MCT_RELSPIN(&Cs_srv_block.busy);
#endif /* MCT */

    /* 
    ** If this was the last thread at its priority, then turn off the ready
    ** bit for that priority.
    */
    if ((scb->cs_rw_q.cs_q_next == Cs_srv_block.cs_rdy_que[scb->cs_priority]) &&
	(scb->cs_rw_q.cs_q_prev == Cs_srv_block.cs_rdy_que[scb->cs_priority]))
    {
	Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> scb->cs_priority);
    }
    scb->cs_inkernel = 0;
    Cs_srv_block.cs_inkernel = 0;
    if (Cs_srv_block.cs_async)
	CS_move_async(scb);

    /*
    ** The admin thread will finish deleting the thread.
    ** Add the scb to its list of threads to delete and signal its condition
    ** variable to wake it up.
    */

    scb->cs_rw_q.cs_q_next = Cs_admin_scb.csa_which;
    Cs_admin_scb.csa_which = scb;
    Cs_admin_scb.csa_mask |= CSA_DEL_THREAD;
    Cs_admin_scb.csa_scb.cs_state = CS_COMPUTABLE;
    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> CS_PADMIN);

    CSv_semaphore(&Cs_admin_scb.csa_sem);
    Cs_srv_block.cs_current = NULL;	/* BEWARE of this window in which   */
    CS_swuser();			/* no one is running */
}


/*{
** Name: CS_default_output_fcn	- default output function for 
**                                CS_dump_stack.
**
** Description:
**      This routine logs messages to the errlog.log
**
** Inputs:
**      arg1	   - ptr to argument list.
**      msg_len    - length of buffer.
**      msg_buffer - ptr to message buffer.
**      
** Outputs:
**      Returns:
**          Doesn't
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-Oct-2002 (hanal04) Bug 108986
**          Created.
*/
static STATUS
CS_default_output_fcn( PTR arg1, i4 msg_len, char *msg_buffer)
{
 CL_ERR_DESC err_code;
 
 ERsend (ER_ERROR_MSG, msg_buffer, msg_len, &err_code);
 return (OK);
}
 


/*{
** Name: CS_rcv_request	- Prime system to receive a request for new session
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
**	18-apr-1998 (canor01)
**	    Return status from CSMT_rcv_request.
**	04-Oct-2000 (jenjo02)
**	    Removed cs_mt test; this function is only called
**	    from CS code.
**
[@history_template@]...
*/

STATUS
CS_rcv_request(void)
{
    STATUS	    status;

    status = (*Cs_srv_block.cs_saddr)(Cs_srv_block.cs_crb_buf, 0);
    if (status)
    {
	CL_ERR_DESC err;

	SETCLERR(&err, status, 0);
	(*Cs_srv_block.cs_elog)(E_CS0011_BAD_CNX_STATUS, &err, 0, 0);
	return(FAIL);
    }
    return(OK);
}

/*{
** Name: CS_del_thread	- Delete the thread.
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
**	18-nov-1996 (canor01)
**	    Update parameters passed to iiCLgetpwnam().
**      02-may-1997 (canor01)
**          Sessions created with the CS_CLEANUP_MASK will be shut
**          down before any other system threads.  When they exit, they
**          must call CSterminate() to allow the normal system threads
**          to exit.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Deleted test for cs_mt; this function is only called
**	    from CS code.
**	16-dec-2002 (somsa01)
**	    Changed type of ac_btime to match the type of cs_connect.
*/
STATUS
CS_del_thread(CS_SCB *scb)
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
    i4			acctstat = 0;
    auto LOCATION	loc;
    auto char		*fname;
    char		username[25];
    struct passwd	*p_pswd, pswd;
    char                pwnam_buf[sizeof(struct passwd)];

    struct {
        	char    ac_flag;	/* Unused */
        	char    ac_stat;        /* Unused */
        	unsigned short ac_uid;  /* Session's OS real uid */
        	unsigned short ac_gid;  /* Unused */
        	short   ac_tty;         /* Unused */
        	i4      ac_btime;       /* Session start time */
        	u_i2  ac_utime;       /* Session CPU time (secs*60) */
        	u_i2  ac_stime;       /* Unused */
        	u_i2  ac_etime;       /* Session elapsed time (secs*60) */
        	u_i2  ac_mem;         /* average memory usage (unused) */
        	u_i2  ac_io;          /* I/O between FE and BE */
        	u_i2  ac_rw;          /* I/O to disk by BE */
        	char    ac_comm[8];     /* "IIDBMS" */
	    }			acc_rec;

#ifdef MCT
    gen_Psem(&CL_acct_sem);
#endif
    if ((ac_ifi < 0) && (ac_ifi > -3))
    {
	/* Attempt to open iiacct, if necessary */
	if (Cs_srv_block.cs_mask & CS_ACCNTING_MASK)
    	{
#ifdef MCT
	    gen_Psem(&NM_loc_sem);
#endif
	    /* Session accounting requested, so locate iiacct */
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
#ifdef MCT
	    gen_Vsem(&NM_loc_sem);
#endif
    	}
	else
	    ac_ifi = -3;		/* Session accounting not requested */
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
	MEmove(	(u_i2)sizeof("IIDBMS"), (PTR)"IIDBMS", EOS,
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
        (*Cs_srv_block.cs_elog)(acctstat, NULL, 0, 0);
#ifdef MCT
    gen_Vsem(&CL_acct_sem);
#endif

    /* Decrement session count.  If user thread decrement user session count */
    Cs_srv_block.cs_num_sessions--;
    if (scb->cs_thread_type == CS_USER_THREAD ||
	scb->cs_thread_type == CS_MONITOR)
    {
	Cs_srv_block.cs_user_sessions--;
    }

    /* remove from known thread tree */
    CS_detach_scb( scb );

    /* if we're a cleanup thread, let the other system threads die too */
    if ( scb->cs_cs_mask & CS_CLEANUP_MASK )
    {
	i4 active_count = 0;
        CSterminate(CS_CLOSE, &active_count);
    }

    status = CS_deal_stack(scb->cs_stk_area);
    status = (*Cs_srv_block.cs_scbdealloc)(scb);
    /*
    ** we used to return acctstat here if set. Since we now log from here
    ** (and the returned status is ignored anyway) just return status.
    ** (sweeney)
    */
    return(status);
}

/*{
** Name: CS_compress	- Produce "pseudo-floating point" number
**
** Description:
**      This routine converts a time_t type into a pseudo-floating point
**      number, with a 3-bit exponent, and a 13-bit mantissa.
**
** Inputs:
**      val                             number to convert
**
** Outputs:
**	Returns:
**	    u_i2			converted number
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**	14-feb-1991 (ralph)
**	    Written.
**	06-jun-1991 (ralph)
**	    Changed to return type int
[@history_template@]...
*/
u_i2
CS_compress(time_t val)
{
    register u_i2 exp = 0, rnd = 0;

    while (val >= 8192)
    {
	exp++;
	rnd = val & 04;
	val >>= 3;
    }
    if (rnd)
    {
	val++;
	if (val >= 8192)
	{
            val >>= 3;
            exp++;
        }
    }
    return((u_i2)(exp<<13) + val);
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
**	25-Jan-1999 (jenjo02)
**	    When adding a thread, test cs_user_sessions count instead
**	    of cs_num_sessions (count of internal AND external sessions).
**	    Dynamic internal threads cause cs_num_sessions to no longer
**	    be deterministic.
**	04-Oct-2000 (jenjo02)
**	    Deleted test for cs_mt; this function is only called
**	    from CS code.
*/

static STATUS
CS_admin_task(i4 mode, CS_SCB *ascb, i4 *next_mode, PTR io_area)
{
    STATUS              status = OK;
    CS_ADMIN_SCB	*scb;
    CS_SCB		*dead_scb;
    CS_SCB		*next_dead;
    CL_ERR_DESC		error;
    i4			task_added;

    if (ascb != &Cs_admin_scb.csa_scb)
    {
	(*Cs_srv_block.cs_elog)(E_CS0012_BAD_ADMIN_SCB, NULL, 0, 0);
	return(FAIL);
    }
    scb = (CS_ADMIN_SCB *) &Cs_admin_scb;
    if (mode != CS_INITIATE)
    {
	for (;;)
	{
	    task_added = 0;

	    /* Lock the scb, and add or delete it */

            CSp_semaphore(TRUE, &scb->csa_sem);
	    if (scb->csa_mask & CSA_ADD_THREAD)
	    {
		task_added = 1;
		if (((Cs_srv_block.cs_user_sessions + 1) >
			    Cs_srv_block.cs_max_sessions)   ||
		    (Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK))
		{
		    (*Cs_srv_block.cs_reject)(Cs_srv_block.cs_crb_buf, 1);
		}
		else
		{
		    status = CSadd_thread(0, Cs_srv_block.cs_crb_buf,
					  CS_USER_THREAD, 
					  (CS_SID*)NULL, &error);
		}
	    }

	    if (scb->csa_mask & CSA_DEL_THREAD)
	    {
		for (dead_scb = scb->csa_which;
			dead_scb != NULL;
			dead_scb = next_dead)
		{
		    next_dead = dead_scb->cs_rw_q.cs_q_next;
		    if ( Cs_srv_block.cs_scbdetach )
			(*Cs_srv_block.cs_scbdetach)( dead_scb );
		    status = CS_del_thread(dead_scb);
		}
		scb->csa_which = NULL;
		scb->csa_mask &= ~CSA_DEL_THREAD;
	    }

            CSv_semaphore(&scb->csa_sem);

	    /* after unlocking it, clean up.  Note that the check
	       below for "task_added" doesn't check status --
	       failure to add a thread this time doesn't mean
	       we should stop listening for a new one.*/

	    if ((scb->csa_mask & CSA_ADD_THREAD) && (task_added))
	    {
		scb->csa_mask &= ~CSA_ADD_THREAD;
		task_added = 0;
#ifdef	CS_SYNC
		if (Cs_srv_block.cs_user_sessions < Cs_srv_block.cs_max_active)
#endif
		status = CS_rcv_request();
	    }

	    /* Note: may get here if completion for ADDER ID
	       happens before CS_rcv_request returns! */

	    if (scb->csa_mask & CSA_ADD_THREAD)
		continue;
	    
	    /* This below is like a CSsuspend, but it's a special
	       case waiting for resume of CS_ADDER_ID */

	    scb->csa_scb.cs_state = CS_UWAIT;
	    Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> CS_PADMIN);
	    CS_swuser();
	}
    }
    else
    {
	*next_mode = (CS_SHUTDOWN_MASK | CS_FINALSHUT_MASK);
    }
    return(status);
}

/*
** Name: cs_handler	- top level exception handler for normal threads
**
** Description:
**	This handler deals with exceptions which excape up out of the
**	thread that generated them.  Such events are thread fatal.
**	Log what we know about the exception, and return EXDECLARE
**	so the thread will try too shutdown.
**
** Future Directions:
**	It would be nice not to call CSterminate from a signal handler.
**	Check out EX_EXIT.
**
** Inputs:
**	exargs		EX arguments (this is an EX handler)
** Outputs:
**	Returns:
**	    EXDECLARE
**
** Side Effects:
**	    Logs exception and begins abnormal thread termination
**
** History:
**	4-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Moved the guts of the EXCHLDDIED case to CSsigchld.
**	16-Apr-91 (anton)
**	    Removed signal handleing switch - it was obsolete and wrong
**	    Correct code is in CSsigterm below
**	    Left check for EXCHLDDIED as that was still used for slave
**		death handling.  Now handled in CSsigchld but left here
**		as well for safety.
**	    Set buffer length from EX_MAX_SYS_REP as per 6.3 CL SPEC
**	    Added description
**	15-Sep-1999 (hanch04)
**	    Print out the hostname and session id when handling exceptions.
**	15-Nov-1999 (jenjo02)
**	    Append thread_id to session_id if OS threads.
*/
STATUS
cs_handler(EX_ARGS *exargs)
{
   
    /* liibi01 Cross integration from 6.4 ( Bug 64041 ).
    ** buf needs to be at least EX_MAX_SYS_REP plus GCC_L_PORT (
    ** for the cs_name ) plus 24 ( for TMstr() ) plus some for
    ** making the output look like ule_format() generated.
    */

    char        buf[ER_MAX_LEN];
    i4          msg_language = 0;
    i4          msg_length;
    i4          length;
    i4          i;
    CL_ERR_DESC error;
    CS_SCB      *scb;
    
    CSget_scb(&scb);

    ERmsg_hdr(Cs_srv_block.cs_name, scb->cs_self, buf);
    length = STlength( buf );

    ERlangcode( (char *)NULL, &msg_language );
    ERslookup(
			    E_CS0014_ESCAPED_EXCPTN,
			    (CL_ERR_DESC *)0,
			    0,
			    (char *)NULL,
			    buf + length,
			    sizeof(buf) - length,
			    msg_language,
			    &msg_length,
			    &error,
			    0,
			    (ER_ARGUMENT *)0 ) ;
    ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);

    buf[ length ] = '\0';
    if (!EXsys_report(exargs, buf ))
    {
	STprintf(buf + STlength(buf), "Exception is %x", exargs->exarg_num);
	for (i = 0; i < exargs->exarg_count; i++)
	    STprintf(buf+STlength(buf), ",a[%d]=%x",
		     i, exargs->exarg_array[i]);
    }
#ifdef OS_THREADS_USED
    if ( CS_is_mt() )
	TRdisplay("cs_handler (session %p:%d): %t\n",
		scb->cs_self, *(i4*)&scb->cs_thread_id,
	        STlength(buf), buf);
    else
#else
	TRdisplay("cs_handler (session %p): %t\n", scb->cs_self,
		  STlength(buf), buf);
#endif /* OS_THREADS_USED */

    if (exargs->exarg_num == EXCHLDDIED)
    {
	TRdisplay("Slave died, server shutdown from cs_handler\n");
	_VOID_ CSterminate(CS_KILL, &i);
    }
    return(EXDECLARE);
}

/*
** Name: CSsigchld - deal with SIGCHLD
**
** Description:
**	This routine is called upon receipt of SIGCHLD.  It waits for
**	the dead child, and then resets the signal handler for SIGCHLD.
**	It must be done in this order because of System V signal semantics.
**	If a signal trap for SIGCHLD is set up before the wait, an infinite
**	loop occurs.  If the wait fails, the dead child is not a slave, or
**	initialization is taking place, simply return, ignoring the death
**	of child event.  If this is not the case, i.e. a slave died,
**	shut the server down.
**
** Future Directions:
**	It would be nice not to call CSterminate from a signal handler.
**	Check out EX_EXIT.
**
** Inputs:
**      sig		signal number
** Outputs:
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    Waits for a dead child.  May shut down the server if critical
**		slave process dies
**
** History:
**	4-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Added this function.
**	16-Apr-91 (anton)
**	    Shut down the server from here instead of using EXCHLDDIED
**		exception.
**	18-Oct-94 (nanpr01)
**	    Handler should be set if pid != -1. Otherwise in BSD it will
**          go into a loop. Bug # 64154 
**      04-Dec-1998 (kosma01)
**          Do not reset handler for sqs_ptx. It is not necessary for
**          sigaction on sqs_ptx. Reseting the handler from within the
**          handler uses a loop between CA_Licensing and CSsigchld() and
**          PCreap().
**      01-Jan-2000 (bonro01)
**          Enable (kosma01) change from 10-Dec-1998 for dgi.
*/

void
CSsigchld(i4 sig)
{
	i4 pid,i;

	/* wait for dead child */

	pid = PCreap();

	/* ignore errors */

	if (pid <= 0)
	    return;

	/* ignore if not a slave */

	for (i = 0; i < NSVSEMS; i++)
	    if (pid == CSslavepids[i])
		break;
	if (i == NSVSEMS)
	    return;

	/* ignore if initializing */

	if (Cs_srv_block.cs_state == CS_INITIALIZING)
	    return;

	TRdisplay("Slave died, server shutdown\n");
	_VOID_ CSterminate(CS_KILL, &i);
}

/*{
** Name: CSsigterm - Special UNIX signal server termination handler
**
** Description:
**	This function evades the usual EX system to directly handle
**	several UNIX signals which are documented to begin server
**	termination.  This handler is set up in CSdispatch.
**
** Future Directions:
**	It would be nice not to call CSterminate from a signal handler.
**	Check out EX_EXIT.
**
** Inputs:
**	sig	UNIX signal number to handle
**
** Outputs:
**	none
**
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-jan-88 (anton)
**	    Created.
**	16-Apr-91 (anton)
**	    Added function header comment
**	    Fix cases - we are switching off SIGnals not EXceptions
**	31-jan-92 (bonobo)
**	    Deleted double-question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	15-Nov-2010 (kschendel)
**	    void, not int returning.
*/
void
CSsigterm(i4 sig)
{
    i4		i = 0;

    /* Signals below are in decreaseing severity */
    switch (sig) {
# ifdef	SIGEMT
    /* doesn't exist on AIX/RT */
    case SIGEMT:
	(VOID) CSterminate(CS_CRASH, &i);
	break;
# endif
    case SIGQUIT:
	(VOID) CSterminate(CS_KILL, &i);
	break;
    case SIGTERM:
	(VOID) CSterminate(CS_CLOSE, &i);
	break;
    case SIGHUP:
	(VOID) CSterminate(CS_COND_CLOSE, &i);
	break;
    }
}

/*{
** Name: CS_fmt_scb	- Format debugging scb (whole VMS dump)
**
** Description:
**      This routine formats the interesting information from the VMS 
**      session control block passed in.  This routine is called by
**	the monitor task to aid in debugging.
**
** Inputs:
**      scb                             The scb to format
**      iosize                          Size of area to fill
**      area                            ptr to area to fill
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-Apr-1987 (fred)
**          Created.
**      11-Nov-1998 (merja01)
**          Correct SEGV for platforms who don't use cs_stk_area.  This
**          would occur on axp_osf when running debug session under
**          iimonitor.
**	15-Nov-1999 (jenjo02)
**	    Show thread_id when OS threads used.
[@history_template@]...
*/
VOID
CS_fmt_scb(CS_SCB *scb, i4 iosize, char *area)
{
    char		*buf = area;
    CS_STK_CB		*stk_hdr;

    if (iosize < 512)
    {
	STprintf(buf, "Insufficient space\r\n");
	return;
    }
    for (;;)
    {
#ifdef OS_THREADS_USED
	if ( CS_is_mt() )
	    STprintf(buf, "---Dump of scb for session %p:%d---\r\n",
		    scb, *(i4*)&scb->cs_thread_id);
	else
#endif /* OS_THREADS_USED */
	    STprintf(buf, "---Dump of scb for session %p---\r\n", scb);

	buf += STlength(buf);
	STprintf(buf, "cs_next: %p\tcs_prev: %p\r\n",
	    scb->cs_next, scb->cs_prev);
	buf += STlength(buf);
	STprintf(buf, "cs_length(%d.+): %d.\tcs_type(%x): %x\r\n",
	    sizeof(CS_SCB), scb->cs_length,
	    CS_SCB_TYPE, scb->cs_type);
	buf += STlength(buf);
	STprintf(buf, "cs_tag: %4s\tcs_self: %p\r\n",
	    &scb->cs_tag, scb->cs_self);
	buf += STlength(buf);

# if defined(sgi_us5)
# define got2
        TRdisplay(buf, "SP: %x PC:%x EXSP: %x\r\n",
            scb->cs_sp, scb->cs_pc, scb->cs_exsp);
        buf += STlength(buf);
        if (buf - area >= iosize)
            break;
# endif

# if defined(sparc_sol)
# define got2
	STprintf(buf, "SP: %x PC:%x EXSP: %x\r\n",
	    scb->cs_registers[CS_SP], scb->cs_pc, scb->cs_exsp);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
# endif

# if defined(usl_us5) || defined(int_lnx) || defined(int_rpl)
# define got2
	STprintf(buf, "SP: %x FP: %x EXSP: %x \r\n",
	    scb->cs_sp, scb->cs_fp, scb->cs_exsp );
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
	STprintf(buf, "%%EBX: %x %%ESI: %x %%EDI: %x\r\n",
	    scb->cs_ebx, scb->cs_esi, scb->cs_edi);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
# endif

# if defined(ibm_lnx)
# define got2
	STprintf(buf, "FP: %x RETADDR: %x EXSP: %x \r\n",
	    scb->cs_registers[CS_SP],
	scb->cs_registers[CS_RETADDR], scb->cs_exsp);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
# endif /* ibm_lnx */

# if defined(any_hpux)
# define got2
        STprintf(buf, "RP: %x  SP: %x\n",
            scb->cs_mach.hp.cs_jmp_buf[0], scb->cs_mach.hp.cs_jmp_buf[1]);
# endif	/* hp8_us5, hp8_bls */

#if defined(any_aix)
# define got2
	STprintf(buf, "SP: 0x%x,  EXSP: 0x%x\r\n",
	    scb->cs_registers[CS_SP], scb->cs_exsp);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
# endif

#if defined(axp_osf) || defined(axp_lnx)
# define got2
        TRdisplay(buf, "SP: %p PC:%p EXSP: %p\r\n",
            scb->cs_sp, scb->cs_pc, scb->cs_exsp);
        buf += STlength(buf);
        if (buf - area >= iosize)
            break;
# endif

# if defined(i64_lnx) || defined(a64_lnx)
# define got2
	/* no ingres threading support !! */
# endif

# if defined(NO_INTERNAL_THREADS)
# define got2
	/* no ingres threading support !! */
# endif /* NO_INTERNAL_THREADS */

# ifndef got2
Missing_machine_dependant code!
# endif

/* check if stk_area null to prevent segv */
   if (scb->cs_stk_area != NULL)
	{
	  stk_hdr = scb->cs_stk_area;
	  STprintf(buf, "Stk begin: %p\tstk end: %p\r\n",
	      stk_hdr->cs_begin,
	      stk_hdr->cs_end);
	  buf += STlength(buf);
	  if (buf - area >= iosize)
	      break;
	  STprintf(buf, "Stk size: %d.\tStk orig sp: %p\r\n",
	      stk_hdr->cs_size - 1024,
	      stk_hdr->cs_orig_sp);
	  buf += STlength(buf);
    }
	break;

	/* NOTREACHED */
    }
}

void
CS_breakpoint(void)
{
    /* Just a place to put a debugger breakpoint */
    return;
}

/*
** Name: CS_update_cpu
**
** Description: Update Server or Session CPU usage.
**
** Inputs:
**	*cpumeasure	if Session CPU delta wanted.
**	*pageflts	if available, a place to return server page faults.
**
** Outputs:
**	*cpumeasure	contains CPU time expended, in ms, since last call.
**	*pageflts	contains server page faults, if available.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	Server CPU time may be updated.
**
** History:
**	25-Sep-1995 (jenjo02)
**	    Added this comment block.
**	    Added code to get real thread time when available.
**	20-sep-2002 (toumi01) bug 108746
**	    In CS_update_cpu, if we are running an OS threaded server
**	    with a 1:1 process:thread model, the result of getrusage()
**	    should be considered the cpu time for the thread.  This
**	    fixes reports of OPF hangs (e.g. issue 12043982) where a
**	    complex query optimization would fail to time out because
**	    of faulty cpu stats (mix and match process stats would
**	    result in negative time computations). Also, modify
**	    CS_update_cpu to avoid a threading race condition by
**	    copying Cs_srv_block.cs_cpu to old_cpu for later comparison
**	    with the new_cpu value.
**	18-Jul-2005 (hanje04)
**	    BUG 114860:ISS 1475660
**	    In CS_update_cpu, if we're running MT on Linux (and therefore
**	    1:1 threading model) Cs_srv_block.cs_cpu has to be a sum of the
**	    CPU usage for each thread. Added cs_cpu to CS_SCB to track
**	    thread CPU usage so that Cs_srv_block.cs_cpu can be correctly
**	    maintained.
[@history_template@]...
*/
void
CS_update_cpu(
i4	*pageflts,
i4	*cpumeasure)
{
    static i4		err_recorded = 0;
    i4		new_cpu;
    i4		old_cpu = Cs_srv_block.cs_cpu;


#ifdef CS_thread_info
    /*
    ** If session CPU time wanted and this is MT, use CS_thread_info
    ** to get the real thread time. In all other cases, accumulate
    ** process (server) time in Cs_srv_block.cs_cpu
    */
    if (cpumeasure && Cs_srv_block.cs_mt)
    {
	/*
	** Session CPU time wanted and real thread time
	** is available.
	*/
	CS_THREAD_INFO_CB	thread_info;

	(VOID)CS_thread_info(&thread_info);

	/* we are measuring cpu in 1/100 seconds */
	*cpumeasure = CS_THREAD_CPU_MS(thread_info) / 10;

	/* No information available about page faults */
	if (pageflts)
	    *pageflts = 0;
	return;
    }
#endif /* Cs_thread_info */

#ifdef xCL_003_GETRUSAGE_EXISTS
    {
	struct rusage	rudesc;
	CL_ERR_DESC		err;

	/* getrusage() returns process time */
	if (getrusage(RUSAGE_SELF, &rudesc) && errno)
	{
	    if (!err_recorded)
	    {
		SETCLERR(&err, 0, 0); 
		err_recorded = 1;
		(*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &err, 0, 0);
	    }
	    return;
	}

	if (pageflts)
	    *pageflts = rudesc.ru_majflt;

	/* we are measuring cpu in 1/100 seconds */
	new_cpu = (rudesc.ru_utime.tv_usec+rudesc.ru_stime.tv_usec)/10000 +
	    (rudesc.ru_utime.tv_sec+rudesc.ru_stime.tv_sec)*100;
    }

# else	/* can't use GETRUSAGE */

    {
	struct tms	tms;
	CL_ERR_DESC	err;
     
	/* check the errno rather than the return value of times.  In sys V
	world the function can succeed and return a negative value */
	if (times( &tms ) && errno)
	{
	    if (!err_recorded)
	    {
		SETCLERR(&err, 0, 0); 
		err_recorded = 1;
		(*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &err, 0, 0);
	    }
	    return;
	}
	/* we are measuring cpu in 1/100 seconds */
	new_cpu = (tms.tms_utime + tms.tms_stime) * 100 / HZ;

	/* No information available about page faults */
	if (pageflts)
	    *pageflts = 0;
    }

# endif /*  xCL_003_GETRUSAGE_EXISTS */

    /* Update the current thread's CPU time count */
    if (cpumeasure)
    {
#ifdef LNX
	/* if 1:1 OS threading model just use thread = process value */
	if (Cs_srv_block.cs_mt)
	    *cpumeasure = new_cpu;
	else
#endif
	    *cpumeasure += new_cpu - old_cpu;
    }

    /* Update server process cpu with current value */
# ifdef LNX
    if (Cs_srv_block.cs_mt)
    {
	CS_SCB *pSCB;
	/*
	** Since we're updating Cs_srv_block.cs_cpu using
	** thread CPU usage we have to assume we can be called
	** more than once per thread. Therefore we must use
	** the delta to update the server CPU usage and NOT the
	** total thread CPU usage
	*/
	/* get the current values */
	CSget_scb((CS_SCB**)&pSCB);
	old_cpu=pSCB->cs_cpu;

	/* update server and thread with new values */
	Cs_srv_block.cs_cpu += new_cpu - old_cpu;
	pSCB->cs_cpu=new_cpu;
    }
    else
# endif
	Cs_srv_block.cs_cpu = new_cpu;

    return;
}

/*
** Name: CS_move_async	- Put values set by asynchronous routines into data
**			  structs looked at by synchronous routines.
**
** Description:
**	This routine is called by non-AST dirven CS routines to complete the
**	actions taken by AST driven calls.
**
**	Since an AST routine may be fired off at virtually any time during the
**	server's operation, some precautions must be made to assure that 
**	AST routines don't stomp on information that is currently being used
**	by some non-ast routine.
**
**	Whenever a CS routine needs to set/use any sever block information that
**	is also used by AST routines, it sets the cs_inkernel flag of the
**	server control block (and of the scb if it is using that as well).
**	This will prevent any AST routine from playing with any of that
**	information.  This will also prevent the server from switching 
**	threads if the quantum timer goes off.  (Disabling ast's is
**	also sufficient to insure safety and some routines that are not
**	performance critical may do this instead of using cs_inkernel.)
**
**	If an AST routine is activated while cs_inkernel is set, then any
**	information that needs to be set by that routine is stored in special
**	asnyc fields of the scb or server control block.  These fields are
**	used only by this routine (which runs with ast disabled) and AST
**	driven routines (which are atomic).  The AST routine also sets the
**	cs_async flag which specifies that there is async information to be
**	put into the server control block or an scb.
**
**	When a CS routine that has set cs_inkernel is finished, it resets
**	cs_inkernel and checks the cs_async flag.  If set, then this routine
**	is called to move the async information into the server control block
**	and/or the scb that was busy.  This routine must disable asts while
**	in progress.
**
** Inputs:
**	scb	- session to check for async information.
**
** Outputs:
**      None.
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sessions may be added to ready queue.
**
** History:
**       7-Nov-1988 (rogerk)
**          Created.
**	13-Jun-1995 (jenjo02)
**	    Increment cs_num_active when non-internal thread put on ready queue.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	23-mar-2001 (devjo01)
**	    Fixed hole where CSresume called by a signal handler would
**	    corrupt cs_as_list.  This was because UNIX does not mask
**	    signal delivery during this routine, so concurrent updates
**	    were possible.   VMS disabled AST delivery in this routine
**	    to avoid this problem.   However, a better solution is to
**	    unhook async list while inkernel is off, then process the
**	    now private list.
[@history_template@]...
*/
VOID
CS_move_async(CS_SCB *scb)
{
    CS_SCB	*nscb, *ascb;
    i4		new_ready;

#ifdef MCT
    MCT_GETSPIN(&Cs_srv_block.busy);  /*TTT*/
    MCT_GETSPIN(&scb->busy);  /*TTT*/
#endif /* MCT */

    /*
    ** Check to see if 'cs_async' set.  If so, clear it before
    ** asserting 'inkernel', so we can detect the case where more
    ** asynchronous resumes occured during the window in which
    ** we hold 'inkernel' in this routine.
    */
    while ( Cs_srv_block.cs_async )
    {
	Cs_srv_block.cs_async = 0;
	new_ready = 0;

	/*
	** Copy head of current cs_as_list BEFORE we assert 'inkernel'.
	** There may be asynchronous resumes updating this list once
	** 'inkernel' is turned on. 
	*/
	nscb = Cs_srv_block.cs_as_list->cs_as_q.cs_q_next;
	Cs_srv_block.cs_as_list->cs_as_q.cs_q_next = 
	 Cs_srv_block.cs_as_list->cs_as_q.cs_q_prev = 
	 Cs_srv_block.cs_as_list;

	/*
	** NOW turn on 'inkernel', so we can update ready queues,
	** from our now private list.
	*/
	Cs_srv_block.cs_inkernel = 1;
	scb->cs_inkernel = 1;

	/*
	** Move any sessions that became ready while inkernel off of the
	** wait queue and onto the ready queue.
	*/
	while ( ( ascb = nscb ) != Cs_srv_block.cs_as_list)
	{
	    /* Take scb off of PRIVATE async list */
	    nscb = ascb->cs_as_q.cs_q_next;

	    ascb->cs_as_q.cs_q_next = 0;
	    ascb->cs_as_q.cs_q_prev = 0;

	    /* Take off of whatever queue (wait or timeout) it is on now */
	    ascb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev= ascb->cs_rw_q.cs_q_prev;
	    ascb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next= ascb->cs_rw_q.cs_q_next;

	    /* now link last into the ready queue */
	    ascb->cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[ascb->cs_priority];
	    ascb->cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[ascb->cs_priority]->cs_rw_q.cs_q_prev;
	    ascb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = ascb;
	    Cs_srv_block.cs_rdy_que[ascb->cs_priority]->cs_rw_q.cs_q_prev =
		    ascb;

	    Cs_srv_block.cs_ready_mask |= 
		    (CS_PRIORITY_BIT >> ascb->cs_priority);
	    if ((ascb->cs_thread_type == CS_USER_THREAD ||
		 ascb->cs_thread_type == CS_MONITOR) &&
	      (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
		Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;

#ifdef	CS_STACK_SHARING
	    if (ascb->cs_stk_area)
	    {
#endif
		ascb->cs_state = CS_COMPUTABLE;
#ifdef	CS_STACK_SHARING
	    }
	    else
	    {
		ascb->cs_state = CS_STACK_WAIT;
		/*
		** XXXX Why is this not done in the computable state above?
		** It is always turned off in the CSattn case.
		*/
		ascb->cs_mask &= ~(CS_INTERRUPT_MASK);	/* turn off	    */
							/* interruptability */
	    }
#endif
	    new_ready = 1;
	}

	/*
	** If someone tried to signal the admin thread while CS was busy
	** then mark the admin thread as ready.
	*/
	if (Cs_srv_block.cs_asadd_thread)
	{
	    Cs_admin_scb.csa_mask |= CSA_ADD_THREAD;
	    Cs_admin_scb.csa_scb.cs_state = CS_COMPUTABLE;
	    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> CS_PADMIN);
	    Cs_srv_block.cs_asadd_thread = 0;
	    new_ready = 1;
	}

	/*
	** If some session was made ready and the server is idling then
	** wake up the idle job so it can switch itself out and the new
	** thread in.
	*/
	if (new_ready && Cs_srv_block.cs_state == CS_IDLING)
	{
	    /*
	    ** VMS would call sys$wake here, so UNIX version
	    ** should send current process a SIGUSR2.
	    */
	    CS_wake( 0 );
	}

	/*
	** Move any information stored in the scb.  cs_asmask hold bits that
	** should be turned on in cs_mask and cs_asunmask holds bits that
	** should be turned off.
	*/
	if (scb->cs_async)
	{
	    scb->cs_mask |= scb->cs_asmask;
	    scb->cs_mask &= ~(scb->cs_asunmask);

	    scb->cs_asmask = 0;
	    scb->cs_asunmask = 0;
	    scb->cs_async = 0;
	    Cs_srv_block.cs_scb_async_cnt--;
	}

	/*
	** Turn off 'inkernel', then check for any asynchronous
	** resumes that occured while processing the last batch.
	*/
	scb->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;
    } /* while */

#ifdef MCT
    MCT_RELSPIN(&scb->busy);  /*TTT*/
    MCT_RELSPIN(&Cs_srv_block.busy);  /*TTT*/
#endif /* MCT */

}

/*
** Name: CS_change_priority - change threads priority.
**
** Description:
**      Bump the specified session's priority to the new value.  The
**      session must currently be on the server's ready queue.
**
**      This routine takes the thread off of the ready queue of its
**      current priority, changes the threads priority value and puts
**      it on its new ready queue.
**
**      Since this routine changes the priority queues, the caller is
**      expected to have set the cs_inkernel field of the server control
**      block to prevent an asynchronous event from coming in and trying
**      to traverse the queues while we are changing them.
**
**      Note:  This routine should probably bump the threads 'base_priority'
**      value if that value becomes meaningful.  Currently, the 'base_priority'
**      is used to hold information not related to priority values.
**
** Inputs:
**      scb             - session to change.
**      new_priority    - priority to change to.
**
** Outputs:
**      None.
**      Returns:
**          none
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      19-jun-1989 (rogerk)
**          Created for Terminator.
**	 9-Jun-1995 (jenjo02)
**	    Turn off CS_PRIORITY_BIT if old priority queue empty,
**	    turn on CS_PRIORITY_BIT in new priority queue if
**	    thread is computable.
**	27-mar-2001 (devjo01)
**	    Make sure 'inkernel' is set when manipulating queues.
[@history_template@]...
*/
VOID
CS_change_priority(
    CS_SCB      *scb,
    i4          new_priority)
{
    i4	oinkernal;

    oinkernal = Cs_srv_block.cs_inkernel;
    Cs_srv_block.cs_inkernel = 1;
#ifdef MCT
    MCT_GETSPIN(&scb->busy);
#endif /* MCT */

    /* Take off current priority queue */
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb->cs_rw_q.cs_q_next;

    /* 
    ** If this was the only thread at its priority, then turn off the ready
    ** bit for that priority.
    */
    if ((scb->cs_rw_q.cs_q_next == Cs_srv_block.cs_rdy_que[scb->cs_priority]) &&
	(scb->cs_rw_q.cs_q_prev == Cs_srv_block.cs_rdy_que[scb->cs_priority]))
    {
	Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> scb->cs_priority);
    }

    /* Set new priority */
    scb->cs_priority = new_priority;

    /* Put on new priority queue */
    scb->cs_rw_q.cs_q_prev =
        Cs_srv_block.cs_rdy_que[new_priority]->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_rdy_que[new_priority];
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;

    /* If thread is computable, set the ready mask */
    if (scb->cs_state == CS_COMPUTABLE)
	Cs_srv_block.cs_ready_mask |= CS_PRIORITY_BIT >> new_priority;

    if ( !oinkernal )
    {
	Cs_srv_block.cs_inkernel = 0;
	if (Cs_srv_block.cs_async)
	    CS_move_async(scb);
    }

#ifdef MCT
    MCT_RELSPIN(&scb->busy);
#endif /* MCT */
}

/*
** Name: CS_wake().
**
** Description:
**      Wake a process.
**
** Inputs:
**      pid             - Process ID or 0 for self.
**
** Outputs:
**      None.
**      Returns:
**          none
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      23-mar-2001 (devjo01)
**          Created.
*/
VOID
CS_wake( PID pid )
{
    if ( !pid )
	pid = Cs_srv_block.cs_pid;
    kill( pid, SIGUSR2 );
}


#ifdef	CS_PROFILING

#define ARCDENSITY      5       /* density of routines */
#define MINARCS         50      /* minimum number of counters */
#define HISTFRACTION    2       /* fraction of text space for histograms */


struct phdr {
        int *lpc;
        int *hpc;
        int ncnt;
};

struct cnt {
        int *pc;
        long ncall;
} *countbase;

static void CSprofend(void)
{
    chdir("/tmp");
    monitor(0);
}

void CSprofstart(void)
{
    int	range, cntsiz, monsize; 
    SIZE_TYPE	pages;
    SIZE_TYPE	alloc_pages;
    char	*buf;
    extern int	*etext;
    CL_ERR_DESC	err_code;

    range = (int)&etext - 0x2000;
    cntsiz = range * ARCDENSITY / 100;
    if (cntsiz < MINARCS)
	cntsiz = MINARCS;
    monsize = (range + HISTFRACTION - 1) / HISTFRACTION
	+ sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
    monsize = (monsize + 1) & ~1;
    pages = (monsize+ME_MPAGESIZE-1)/ME_MPAGESIZE;
#ifdef MCT
    if (ME_page_sem)
	gen_Psem(ME_page_sem);
#endif
    MEget_pages(ME_NO_MASK, pages, 0, (PTR *)&buf, &alloc_pages, &err_code);
#ifdef MCT
    if (ME_page_sem)
	gen_Vsem(ME_page_sem);
#endif
    monitor(0x2000, &etext, buf, pages*ME_MPAGESIZE, cntsiz);
    moncontrol(1);
    PCatexit(CSprofend);
}
#else
void CSprofstart(void)
{
}
#endif

# ifdef sparc_sol
 
/*
** Coded CS_su4_eradicate() and CS_su4_setup() in assembler to overcome
** the reliance on the compiler generated code of the C flavours of these
** two routines.  We don't know what instructions the compiler will generate
** and this caused problems when V2 Solaris compiler was superseded with V3.
** By coding in assembler, we always know the correct offset for the return
** address when faking the stack frame.
*/
 
void    CS_su4_eradicate();
STATUS  CS_su4_setup();
 

/*
** Name: CS_fudge_stack()	- fudge stack for Sun 4
**
** Description:
**	This Sun4-only routine initializes the stack space for a newly
**	created thread.  It fudges the thread's stack such that when
**	CS_swuser is initially called it will "return" into the CS_setup
**	function.  Also, the stack is set up so that if CS_setup returns,
**	it will "return" directly into the CS_eradicate function, which
**	will eradicate the thread.  CS_eradicate does not return.
**
**	NOTE:  Rather than coding CS_fudge_stack to fudge the stack to
**	directly "return" into CS_setup and CS_eradicate, it instead
**	indirectly returns into them through the use of the dummy functions
**	CS_su4_setup and CS_su4_eradicate.  This allows this code to
**	be independent of the stack size of CS_setup and CS_eradicate.
**	(See the comment above).
**
** Inputs:
**      scb                             the thread's scb
**
** Outputs:
**      none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sets the value of scb->cs_registers[CS_SP] and initializes
**	    a portion of a thread's stack space.
**
** History:
**      09-feb-89 (russ)
**          Created.
**	03-may-95 (hanch04)
**	    Added changes made by nick
**          Change entry point of CS_su4_eradicate() to match new
**          assembler version for su4_us5.
*/

static void
CS_fudge_stack(CS_SCB *scb)
{
	register i4   i,*ip;
	register char *sp,*fp;

	/*
	** Allocate dummy stack for CS_su4_eradicate, which should
	** get called when CS_su4_setup returns.  CS_su4_eradicate
	** calls CS_eradicate.  CS_eradicate never returns.
	*/

	fp  = (char *) scb->cs_registers[CS_SP];
	sp = fp - SA(MINFRAME);

	/* zero the saved registers */
	ip = (i4 *)sp;
	for (i=0;i<16;i++) {
		*(ip + i) = 0;
	}

	/*
	** Set frame pointer in dummy register save area, so
	** that when CS_su4_eradicate is called, it will have a proper
	** frame pointer. ip + 14 == %fp == %i6 == %r30
	*/

	*(ip + 14)  = (i4)fp;

	/*
	** Allocate dummy stack for CS_su4_setup, which calls
	** CS_setup.  This must be fudged so that CS_su4_setup
	** returns to CS_su4_eradicate.
	*/

	fp = sp;
	sp = fp - SA(MINFRAME);

	/* zero the saved registers */
	ip = (i4 *)sp;
	for (i=0;i<16;i++) {
		*(ip + i) = 0;
	}

	/*
	** Set frame pointer in dummy register save area, so
	** that CS_su4_setup will have the proper frame pointer
	** and when CS_su4_eradicate is called, it will have a
	** proper stack pointer. ip + 14 == %fp == %i6 == %r30
	*/

	*(ip + 14)  = (i4)fp;

	/*
	** Set entry point of CS_su4_eradicate, so that CS_su4_setup
	** will return to it. ip + 15 == %i7 == %r31
	*/

        *(ip + 15)  = (i4)CS_su4_eradicate - 4;

	/*
	** Set stack pointer value in scb, so that CS_swuser can
	** point to this new stack for it's invocation of
	** CS_setup.
	*/

	scb->cs_registers[CS_SP] = (long)sp;
}

# endif /* sparc_sol */


/*{
** Name: CSdump_stack()	- dump out stack trace.
**
** Description:
**	Externally visible way to get a stack trace
**	of the caller's session.
**
** Inputs:
**	none
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**	25-Apr-2003 (jenjo02) SIR 111028
**	    Invented
*/
VOID 
CSdump_stack( void )
{
    /* Dump this thread's stack, non-verbose */
    CS_dump_stack(NULL, NULL,NULL,0,0);
}

/*{
** Name: CS_dump_stack()	- dump out stack trace.
**
** Description:
**	This routine calls a system specific routine which prints out a stack
**	trace of the given session.  This may not be possible to do on all
**	systems so it is not mandatory that this routine be implemented.
**	This routine is meant as a debugging aide and is not necessary for
**	the proper functioning of the server.  Providing an implementation
**	of this function will make diagnosis of AV's in the server much easier
**	in the field.
**
**	Currently this routine is called in two places.  It is called by 
**	the EXsys_report() and it is called by the iimonitor "debug" and
**	"stacks" commands.
**
**	If no implementation of this routine exists for a given platform, 
**	calling this routine will silently result in no work done (ie. the
**	function will simply return doing nothing).
**
** Inputs:
**	scb				session id of stack to dumped.  If the
**					value input is NULL then the current
**					stack is assumed.
**      pContext | ucp | stack_p        Platform specific argument for the
**                                      context or stack pointer.
**	output_arg			Passed as-is to output_fcn.  If the
**					supplied output_fcn doesn't require
**					this arg, it can be NULL.
**	output_fcn			Function to call to perform the output.
**                                      This routine will be called as
**                                          (*output_fcn)(output_arg,
**                                                          length,
**                                                          buffer)
**					where buffer and length contain the
**                                      output string
**	verbose				Should we print some information about 
**					where this diagnostic is coming from?
**
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**
** History:
**      12-feb-91 (mikem)
**          Created.
**      31-dec-1993 (andys)
**          Add verbose argument.
**	19-sep-1995
**	    Added stack_p parameter and add dr6_us5 to the list.
**      22-Oct-2002 (hanal04) Bug 108986
**          Use CS_default_output_fcn() as a default output
**          function if one is not supplied by the caller.
*/
void
CS_dump_stack(CS_SCB *scb, void *context, PTR arg, TR_OUTPUT_FCN *fcn, i4 verbose)
{

    if(fcn == NULL)
    {
	arg = NULL;
	fcn = CS_default_output_fcn;
    }

# if defined(axp_osf)
	CS_axp_osf_dump_stack(scb, (CONTEXT *)context, arg, fcn, verbose);
# endif /*axp_osf */

/* These stack dump functions have not been modified to work in 64-bit yet */
# if ! defined(LP64)

# if defined(any_hpux) && !defined(HPUX_DO_NOT_INCLUDE) && !defined(i64_hpu)
    CS_hp_dump_stack( scb, (ucontext_t *)context, arg, fcn, verbose );
# endif /* hp8_us5 */

# endif /* LP64 */

# if defined(int_lnx) || defined(int_rpl) || defined(a64_lnx)
    CS_lnx_dump_stack( scb, (ucontext_t *) context, arg, fcn, verbose );
# endif /* int_lnx */

# if defined(sparc_sol) || defined(a64_sol)
    CS_sol_dump_stack(scb, (ucontext_t *) context, arg, fcn, verbose);
# endif

# if defined(i64_hpu)
    CS_i64_hpu_dump_stack( arg, fcn, verbose );
# endif /* i64_hpu */

    return;
}

# define		MAX_STK_UNWIND	50

# if defined(sparc_sol) || defined(a64_sol)

#if defined(__sparc)
#define FRAME_PTR_REGISTER REG_SP
#define PC_REGISTER REG_PC
#define CHECK_FOR_SIGFRAME(fp, oldctx) ((fp) + SA(sizeof (struct frame)) \
        == (oldctx))

#elif defined(__amd64)
#define FRAME_PTR_REGISTER      REG_RBP
#define PC_REGISTER             REG_RIP
#define CHECK_FOR_SIGFRAME(fp, oldctx) ((((fp) + sizeof (struct frame)) + \
        2 * sizeof (long) == (oldctx)) && \
        (((struct frame *)fp)->fr_savpc == (greg_t)-1))

#elif defined(__i386)
#define FRAME_PTR_REGISTER EBP
#define PC_REGISTER EIP
#define CHECK_FOR_SIGFRAME(fp, oldctx) ((((fp) + sizeof (struct frame)) + \
        3 * sizeof (int) == (oldctx)) && \
        (((struct frame *)fp)->fr_savpc == (greg_t)-1))
#else
#error no arch defined
#endif

static int
read_as(int fd, struct frame *fp, struct frame **savefp, uintptr_t *savepc)
{
	uintptr_t	newfp;

	if ((uintptr_t)fp & (sizeof (void *) -1))
		return(-1); /* misaligned */

	if ((pread(fd, (void *)&newfp, sizeof(fp->fr_savfp),
	    (off_t)&fp->fr_savfp) != sizeof(fp->fr_savfp)) ||
	    pread(fd, (void *)savepc, sizeof(fp->fr_savpc),
	    (off_t)&fp->fr_savpc) != sizeof(fp->fr_savpc))
		return(-1);

	if (newfp != 0)
		newfp += STACK_BIAS;

	*savefp = (struct frame *)newfp;
	return(0);
}

/*{
** Name: CS_sol_dump_stack()	- dump Solaris stack trace.
**
** Description:
**	Solaris implementation of CS_dump_stack().
**
**	Special assemply routine CS_getsp() is called to flush the register
**	cache.  Following this "C" code can safely traverse the stack
**	given the saved stack pointer found in the scb.  A stack dump is
**	printed using the given output_fcn().
**
** Inputs:
**      scb                             session id of stack to dumped.
**                                      If scb is zero, dump the current stack.
**	stack_p				The current stack.
**	output_arg			Passed to output_fcn.
**      output_fcn                      Function to call to perform the output.
**                                      This routine will be called as
**                                          (*output_fcn)(arg,
**                                                          length,
**                                                          buffer)
**                                      where buffer is the length character
**                                      output string.
**      verbose                         Should we print some information about
**                                      where this diagnostic is coming from?
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
** History:
**	17-Sep-2007 (bonro01)
**	    Created by Robert Bonchik
*/

static VOID
CS_sol_dump_stack( CS_SCB *scb, ucontext_t *ucp, PTR output_arg,
	TR_OUTPUT_FCN *output_fcn, i4 verbose)
{
    char        name[GCC_L_PORT];
    char        verbose_string[80];
    PID         pid;            /* our process id                           */
    i4		stack_depth = MAX_STK_UNWIND;
    struct frame *fp;
    int		fd;
    struct frame *savefp;
    uintptr_t	savepc;
    ucontext_t	uc;
    ucontext_t *oldctx;
    int		sig;
#if defined(__sparc)
    int	signo = 0;
    uintptr_t	special_pc = NULL;
    int		special_size = 0;
    extern void _thr_sighndlrinfo(void (**func)(), int *funcsize);
#pragma weak _thr_sighndlrinfo

    if (_thr_sighndlrinfo != NULL)
    {
	_thr_sighndlrinfo((VOID (**)())&special_pc,&special_size);
    }
#endif 

    /* scb must be NULL if ucp is passed so that current scb is used */
    if (scb && ucp)
	return;

    if (scb)
    {
#if defined(sparc_sol)
        /* Only Solaris Sparc internal threads sets cs_registers */
        /* which allows us to stack trace all internal threads.  */
	if( !CS_is_mt() )
	{
	    fp = (struct frame *) scb->cs_registers[0];
	    oldctx = NULL;
	}
	else
#endif
	{
	    /* When running with OS threads we can only trace stack */
	    /* for the current scb because we need to get the       */
	    /* current context to get the frame pointer.            */
	    CS_SCB  *cur_scb;
	    CSget_scb(&cur_scb);
	    if( scb == cur_scb )
	    {
		/* cancel backtrace if getcontext fails */
		if( getcontext(&uc) )
		    return;
		ucp = &uc;
		oldctx = ucp->uc_link;
		fp = (struct frame *)
		    ((uintptr_t)ucp->uc_mcontext.gregs[FRAME_PTR_REGISTER]
		    + STACK_BIAS);
	    }
	    else
	    {
		/* can only backtrace current scb */
		return;
	    }
	}
    }
    else
    {
	CSget_scb(&scb);
	/* Use the context passed in from the singal handler */
	/* Otherwise get the current context */
	if(!ucp)
	{
	    /* cancel backtrace if getcontext fails */
	    if( getcontext(&uc) )
		return;
	    ucp = &uc;
	}
        oldctx = ucp->uc_link;
        fp = (struct frame *)
	     ((uintptr_t)ucp->uc_mcontext.gregs[FRAME_PTR_REGISTER]
	     + STACK_BIAS);
    }

    if (verbose)
    {
        STncpy(name, Cs_srv_block.cs_name, sizeof(Cs_srv_block.cs_name));
	name[ sizeof(Cs_srv_block.cs_name) ] = EOS;

        PCpid(&pid);
	STprintf(verbose_string,"%-8.8s::[%-16.16s, %08.8x]: Stack dump pid %d: ",
	    PMhost(), name, scb->cs_self, pid);
    }
    else
    {
        verbose_string[0] = EOS;
    }

    if ((fd = open("/proc/self/as", O_RDONLY)) < 0)
	return;


    while ((stack_depth-- > 0) && fp)
    {
	long pc;

	sig = 0;

	if (read_as(fd, fp, &savefp, &savepc) != 0)
	{
		close(fd);
		return;
	}

	if (savefp == NULL)
		break;

	if (oldctx != NULL &&
	    CHECK_FOR_SIGFRAME((uintptr_t)savefp, (uintptr_t)oldctx))
	{
#if defined(__i386) || defined(__amd64)
		sig = *((int *)(savefp + 1));
#endif

#if defined(__sparc)
		if (_thr_sighndlrinfo == NULL)
			sig = fp->fr_arg[0];
		else
			sig = signo;
#endif

		savefp = (struct frame *)
		    ((uintptr_t)oldctx->uc_mcontext.gregs[FRAME_PTR_REGISTER] +
		    STACK_BIAS);
		savepc = oldctx->uc_mcontext.gregs[PC_REGISTER];

		oldctx = oldctx->uc_link;
	}

#if defined(__sparc)
	if (_thr_sighndlrinfo &&
	      savepc >= special_pc && savepc <
	      (special_pc + special_size))
	    signo = fp->fr_arg[0];
#endif

       {
            char sym[80];
	    char buf[128+128+80+80+50+1];	/* +50 is slop */
	    char traceline[128];
	    char registerline[128];
	    char stackline[80];
            long addr = 0, offset=0;
            int  len;

            sym[0] = EOS;
            len = DIAGSymbolLookup(savepc,&addr,&offset,sym,sizeof(sym));
            if (len)
            {
		STprintf(traceline, "0x%p: %s+0x%lx [0x%p]",
		  savefp, sym, offset, savepc);
            }
            else
	    { 
		STprintf(traceline, "0x%p:  [0x%p]",
		  savefp, savepc);
	    }

#if defined(sparc_sol)
	    /* Solaris Sparc registers are neatly saved in six argument */
	    /* registers in the stack frame structure */
	    STprintf(registerline, "(%lx,%lx,%lx,%lx,%lx,%lx) ",
	      savefp->fr_arg[0], savefp->fr_arg[1],
	      savefp->fr_arg[2], savefp->fr_arg[3],
	      savefp->fr_arg[4], savefp->fr_arg[5]);
#endif

#if defined(a64_sol)
	    /* Solaris Intel registers are not so easily found. There are */
	    /* no structures to access these args because the args are    */
	    /* pushed on the stack and their number is variable.          */
	    /* Mapping these to a structure would be risky because trying */
	    /* to access args at the end of the stack would most likely   */
	    /* run past the end of the stack memory and SEGV. This        */
	    /* function instead gets up to six args by reading the current*/
	    /* process address space special file.                        */
	    /* Registers for 64bit Intel are not attempted because there  */
	    /* can be a variable number of arguments and each arg can be  */
	    /* 4 or 8 bytes and padded depending on alignment.            */
#if defined(LP64)
	    registerline[0] = EOS;
#else
            {
            int arg[6]={0,0,0,0,0,0},count;

            for(count=0; count < 6; count++)
            {
            if (pread(fd, (void *)&arg[count], sizeof(int),
               ((off_t)savefp)+8+count*4) != sizeof(int))
               break;
            }
	    STprintf(registerline, "(%x,%x,%x,%x,%x,%x) ",
	      arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
	    }
#endif
#endif /* a64_sol */

	    if(sig)
		STprintf(stackline, "[Signal %d]", sig);
	    else
		stackline[0] = EOS;
		
	    TRformat(output_fcn, output_arg, buf, sizeof(buf) - 1, "%s%s%s%s",
		     verbose_string, traceline, registerline,stackline);
        }

	fp = savefp;
    }
    close(fd);
}

# endif /* su4_us5 su9_us5 a64_sol */


# if defined(any_hpux) && !defined(HPUX_DO_NOT_INCLUDE) && !defined(i64_hpu)
 
/*{
** Name: CS_hp_dump_stack()	- HP-UX exception stack trace.
**
** Description:
**	HP implementation of CS_dump_stack().  This routine is
**	a modified version of the su4 implementation.
**
**	Arguments printed with each stack entry are the four fixed
**	arguments, arg word 0 thru 3, required for each stack frame
**	whether or not they are actually used - the routine currently
**	makes no effort to examine the unwind_table entry to determine
**	if the args were stored in the stack frame or not.  When a
**	context is passed to the routine, the first set of argument
**      values are taken from the context & put in the stack, so these
**	should be accurate.
**
** Inputs:
**      scb                     On HP, currently can't produce anything
**                              useful (especially when running with
**				OS threads), so for now these calls
**				comming from monitor will just return.
**	ucp			The pointer to passed ucontext_t when
**                              performing an exception stack trace
**	output_arg		Passed to output_fcn.
**      output_fcn              Function to call to perform the output.
**                              This routine will be called as
**                              (*output_fcn)(output_arg,
**                                            length,
**                                            buffer)
**                              where buffer is the length character
**                              output string.
**      verbose                 Should we print some information about
**                              where this diagnostic is coming from?
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
** History:
**      06-nov-98 (muhpa01)
**          Created.
**	08-aug-00 (hayke02)
**	    Remove distinction between hp8_us5 and hpb_us5 for ucp
**	    assignments.
*/

static VOID
CS_hp_dump_stack( CS_SCB *scb, ucontext_t *ucp, PTR output_arg,
	TR_OUTPUT_FCN *output_fcn, i4 verbose )
{
    char        buf[128];
    char        name[GCC_L_PORT];
    char        verbose_string[80];
#if defined(LP64)
    uint64_t    *sp, pc;
    char        pc_string[80];
#else
    u_i4   *sp, dp, sr4, pc, rp;
#endif
    i4		stack_depth = MAX_STK_UNWIND;
    PID         pid;            /* our process id */
    PTR		sym;
    CUR_FRAME_DEF	cur_frame;
    PREV_FRAME_DEF	prev_frame;
    i4		rc;

    /*
    ** Currently on HP, when given an scb, we can't produce
    ** anything useful, so we'll just return.  Maybe this can be
    ** fixed in the future.  This will only affect calls from
    ** monitor.
    */
    if ( scb )
	return;	
    else if ( ucp )
    {
#if defined(LP64)
        cur_frame.cur_frsize = 0;
	cur_frame.curgp = 0;
	cur_frame.curmrp = 0;
        cur_frame.cursp = ucp->uc_mcontext.ss_wide.ss_64.ss_sp;
        cur_frame.currp = ucp->uc_mcontext.ss_wide.ss_64.ss_rp;
        cur_frame.r3 = (unsigned long) ucp->uc_mcontext.ss_wide.ss_64.ss_gr3;
        cur_frame.r4 = (unsigned long) ucp->uc_mcontext.ss_wide.ss_64.ss_gr4;
        pc = ucp->uc_mcontext.ss_wide.ss_64.ss_pcoq_head;
#else
	sp = (u_i4 *)ucp->uc_mcontext.ss_narrow.ss_sp;
	dp = ucp->uc_mcontext.ss_narrow.ss_dp;
	rp = ucp->uc_mcontext.ss_narrow.ss_rp;
	sr4 = ucp->uc_mcontext.ss_narrow.ss_sr4;
	pc = ucp->uc_mcontext.ss_narrow.ss_pcoq_head;
#endif  /* LP64 */
	pc &= ~3;

#if !defined(LP64)
	*(sp - 9) = ucp->uc_mcontext.ss_narrow.ss_arg0;
	*(sp - 10) = ucp->uc_mcontext.ss_narrow.ss_arg1;
	*(sp - 11) = ucp->uc_mcontext.ss_narrow.ss_arg2;
	*(sp - 12) = ucp->uc_mcontext.ss_narrow.ss_arg3;
#endif  /* LP64 */
    }	
    else
	return;		/* nothing to do for now */ 

    if (verbose)
    {
        STncpy( name, Cs_srv_block.cs_name, sizeof(Cs_srv_block.cs_name) );
	name[ sizeof(Cs_srv_block.cs_name) ] = EOS;

        PCpid( &pid );
	STprintf(verbose_string,"%-8.8s::[%-16.16s, %s]: Stack dump pid %d: ",
	    PMhost(), name, "NO SID", pid);
    }
    else
        verbose_string[0] = EOS;

#if defined(LP64)
    cur_frame.currlo = pc;
#else
    cur_frame.curdp = dp;
    cur_frame.cursp = (unsigned int)sp;
    cur_frame.currls = sr4;
    cur_frame.currlo = pc;
    cur_frame.toprp = rp;
    cur_frame.cur_frsize = 0;
#endif

    /*
    ** in case stack is corrupt, only print 100 levels so that we don't
    ** loop here forever.
    */
    sp = cur_frame.cursp;
    while ( ( stack_depth-- > 0 ) && sp )
    {
	pc = cur_frame.currlo & ~3;
#if defined(LP64)
        STprintf(pc_string,"0x%016lx ", pc);
#endif

	sym = (PTR)DIAGSymbolLookupAddr( pc, 0 );
	if ( sym )
        {
#if defined(LP64)
                TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
                        "%s %s %s", verbose_string, pc_string, sym);
#else
        	TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
            		"%s%08x: %s(%x,%x,%x,%x)",
	    		verbose_string,
            		cur_frame.cursp, sym,
            		*(sp - 9), *(sp - 10), *(sp - 11), *(sp - 12) );
#endif
        }
	else
        {
#if defined(LP64)
                TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
                        "%s %s", verbose_string, pc_string);
#else
        	TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
            		"%s%08x: %x(%x,%x,%x,%x)",
	    		verbose_string,
            		cur_frame.cursp, cur_frame.currlo,
            		*(sp - 9), *(sp - 10), *(sp - 11), *(sp - 12) );
#endif
        }

	if ( rc = U_get_previous_frame( &cur_frame, &prev_frame ) )
		break;

	if ( prev_frame.prevrlo == 0 )
		break;
	else
	{
		cur_frame.currlo = prev_frame.prevrlo;
		cur_frame.cur_frsize = prev_frame.prev_frsize;
		cur_frame.cursp = prev_frame.prevsp;
#if defined(LP64)
                cur_frame.r3 = prev_frame.r3;
		cur_frame.r4  = prev_frame.r4;
#else
		cur_frame.currls = prev_frame.prevrls;
		cur_frame.curdp = prev_frame.prevDP;
		cur_frame.cur_r19 = prev_frame.prev_r19;
#endif
	}
	sp = cur_frame.cursp;
    }
}
# endif /* hp8_us5 */

# if defined(i64_hpu)
/*{
** Name: CS_i64_hpu_dump_stack()     - HP Itanium exception stack trace.
**
** Description:
**      Basic HP Itanium stack dump for current thread only.
**      This will only dump the stack for the current thread.
**
** Inputs:
**	output_arg		Passed to output_fcn.
**	output_fcn		Outputter routine, called as
**				  (*output_fcn)(output_arg, len, buffer);
**				with output in buffer of length len.
**      verbose                 Should we print some information about
**                              where this diagnostic is coming from?
**
** Outputs:
**      Returns:
**          VOID
** History:
**      20-Aug-2008 (hanal04)
**          Created.
*/
static VOID
CS_i64_hpu_dump_stack(PTR output_arg, TR_OUTPUT_FCN *output_fcn, i4 verbose)
{
    char		buf[128];
    char		verbose_string[80];
    _Unwind_Context	*context;
    _UNW_ReturnCode	status;
    PTR			sym;
    uint64_t		pc;
    char		pc_string[80];
    char		name[GCC_L_PORT];
    PID			pid; /* our process id */

    if (verbose)
    {
        STncpy( name, Cs_srv_block.cs_name, sizeof(Cs_srv_block.cs_name) );
        name[ sizeof(Cs_srv_block.cs_name) ] = EOS;

        PCpid( &pid );
        STprintf(verbose_string,"%-8.8s::[%-16.16s, %s]: Stack dump pid %d: ",
            PMhost(), name, "NO SID", pid);
    }
    else
        verbose_string[0] = EOS;

    context = _UNW_createContextForSelf();
    if(context == NULL)
    {
        TRdisplay("_UNW_createContextForSelf() failed at %s:%d\n", 
                  __FILE__, __LINE__);
        return;
    }
  
    /* _UNW_currentContext starts from caller */
    status = _UNW_currentContext(context);
    if (status != _UNW_OK)
    {
        TRdisplay("_UNW_currentContext() failed at %s:%d with status = %d\n", 
                  __FILE__, __LINE__, status);
        return;
    }

    while(status == _UNW_OK)
    {
        pc = _UNW_getIP(context);
        STprintf(pc_string,"0x%016lx ", pc);
        
        /* DIAGObjectRead() of the iidbms always gets a symbol_count of 0
        ** so DIAGSymbolLookupAddr() always returns NULL. The call is made
        ** in case we manage to get i64_hpu to successfully load the
        ** symbol table details with functions names.
        */
        sym = (PTR)DIAGSymbolLookupAddr( pc, 0 );

        if(sym)
        {
             TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
                    "%s %s %s", verbose_string, pc_string, sym);
        }
        else
        {
            TRformat( output_fcn, output_arg, buf, sizeof(buf) - 1,
                   "%s %s ", verbose_string, pc_string);
        }
        status = _UNW_step(context);
    }
 
    return; 
}
# endif /* i64_hpu */

# if defined(usl_us5) || defined(int_lnx) || defined(int_rpl)
/*
** Name: CS_sqs_pushf()	- put return address of routine on indicated stack 
**      for Symmetry
**
** Description:
**	This is a Sequent-only routine.  This pushes the return 
**	address of a passed routine on the passed scb's stack.  Hence
**      it sets up the stack so a called routine will "return"
**      to the routine put on the stack.  This is purposely isolated 
**	so that future modifications will be made easier.
**
** Inputs:
**      scb                             the thread's scb
**
** Outputs:
**      none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sets the value of scb->cs_sp  and initializes
**	    a portion of a thread's stack space.
**
** History:
**      16-feb-89 (markd)
**          Created.
*/
void
CS_sqs_pushf( CS_SCB *scb, void (*retfunc)(void) )
{
 	 scb->cs_sp-=sizeof(long);
 	 *(long *)scb->cs_sp = (long)retfunc; 
}

# endif

# if defined(any_aix) && defined(BUILD_ARCH32)

/*      Stack                                           ^
 *              |               |        next link area |
 *              |---------------|
 *              |               |
 *              | args area     |
 *              |               |
 *              |---------------|
 *              |               |
 *              | local stk     |
 *              |               |
 *              |---------------|       \
 *              | gprs          |       }  reg save area
 *              |               |       /
 *              | fprs          |
 *              |---------------|<------------------------- caller's sp
 *              | back chain    |
 *              | cond register |       \
 *              | link register |       }  link area
 *              | reserved      |       /
 *              | reserved      |
 *              | TOC           |
**   History:
**	20-jun-93 (vijay)
**	    ris_us5 specific: jump into the middle of CS_ris_setup from swuser.
**	    Else dbx is getting confused about the stack and becoming non
**	    cooperative. Save floats.
**
*/

CS_ris_stack(scb)
CS_SCB *scb;

{
        long    *fp;
        int     i;
        long    *func;

        fp = (long *) scb->cs_registers[CS_SP];

        /* ris_setup's stack area: */
	for (i=0; i<16; i++) *(fp--) = 0;

        /* ris_setup's link area */

        *(fp--);                /* save TOC here if necessary */
        fp--;                   /* reserved */
        fp--;                   /* reserved */
	func = (long *) CS_ris_setup; 
	func=(long *)*func + 2; /* two instructions from the beginning */
        *(fp--) = (long)func;	/* saved link register of the callee.
                                 * NOTE: saving *(func_name) since func_name
                                 * is only a TOC pointer to the func code */
        *(fp--) = 0;            /* saved CR */
        *(fp--) = (long)fp + 18*8 + 19*4 + 6*4;
				/* back chain points to end of next link area.
                                 * 18 fpr + 19 gprs + 6 regs in link area
                                 */

        /* sw_user's stack area */

        for (i = 13; i < 32; i++)
                *(fp--) = 0;    /* save GPRS */

	for (i = 14; i < 32; i++)
	{
		*(fp--) = 0;    /* save FPRS */
		*(fp--) = 0;    /* save FPRS */
	}


        /* swuser's link area */

        *(fp--) = 0;            /* should save TOC ?*/
        fp--;                   /* reserved */
        fp--;                   /* reserved */
        *(fp--) = NULL;         /* saved link register of the callee */
        *(fp--) = 0;            /* saved CR */
        *(fp--) = 0;            /* back chain */

        /*      end of swusers stack    */

        scb->cs_registers[CS_SP] = (int)(fp+1);/* frame pointer */
        scb->cs_registers[CS_TOC] = 0;  /* save TOC here if needed */
}
# endif /* 32-bit power AIX */


# if defined(any_aix) && defined(BUILD_ARCH64)
/*      
**   Routine:  CS_ris64_stack() 
**             This routine is based on CS_ris_stack(), some modifications 
**             were made to address the 64-bit requirements.
**
**   History:
**      05-Jul-2000 (hweho01)
**           
**          Changes for the rs/6000 64-bit platform specific:
**          1) Changed the number of byte of each register from 4 bytes to 
**             8 bytes for AIX 64-bit platform.
**          2) Changed the cast of fp (frame pointer) from int to long, 
**             so it will match the type of cs_registers in CS_SCB.  
*/

CS_ris64_stack(scb)
CS_SCB *scb;

{
        long    *fp;
        int     i;
        long    *func;

        fp = (long *) scb->cs_registers[CS_SP];

        /* ris_setup's stack area: */
	for (i=0; i<16; i++) *(fp--) = 0;

        /* ris_setup's link area */

        *(fp--);                /* save TOC here if necessary */
        fp--;                   /* reserved */
        fp--;                   /* reserved */
	func = (long *) CS_ris_setup; 
	func=(long *)*func + 1; /* two instructions from the beginning */
                                /* that is 8 bytes for 64-bit platform */
        *(fp--) = (long)func;	/* saved link register of the callee.
                                 * NOTE: saving *(func_name) since func_name
                                 * is only a TOC pointer to the func code */
        *(fp--) = 0;            /* saved CR */

        *(fp--) = (long)fp + 18*8 + 19*8 + 6*8 + 8;
				/* back chain points to end of next link area.
                                 * 18 fpr + 19 gprs + 6 regs + 8 bytes for 
                                 * alignment padding.
                                 */

        /* sw_user's stack area */

        *(fp--) = 0;            /* alignment padding */
        for (i = 13; i < 32; i++)
                *(fp--) = 0;    /* save GPRS */

	for (i = 14; i < 32; i++)
	{
		*(fp--) = 0;    /* save FPRS */
	}

        /* swuser's link area */

        *(fp--) = 0;            /* should save TOC ?*/
        fp--;                   /* reserved */
        fp--;                   /* reserved */
        *(fp--) = NULL;         /* saved link register of the callee */
        *(fp--) = 0;            /* saved CR */
        *(fp--) = 0;            /* back chain */

        /*      end of swusers stack    */

        scb->cs_registers[CS_SP] = (long)(fp+1);/* frame pointer */
        scb->cs_registers[CS_TOC] = 0;  /* save TOC here if needed */
        scb->cs_regs_stored = 0x0L;     /* Set the flag for register restore */
                                        /* to zero initially, it will be     */
                                        /* updated to non-zero in CS_swuser  */
                                        /* when registers are retored in the */
                                        /* stack of scb.                     */
}
# endif /* 64-bit power AIX  */


#ifdef axp_osf
/*
** Name: CS_axp_osf_dump_stack	- Dump out Stack for Digital UNIX
**
** Description:
**  This routine uses the functions in libmld.a and libexc.a to 
**  unwind the stack and extract diagnostic information.  The
**  stack dump is written to errlog.log during exception handling
**  and is called in exsysrep.   
**
**   Inputs:
**       CS_SCB        -   Pointer to target SCB, or NULL if current
**			   SCB is the target.   Stack back traces
**			   for other sessions are only supported
**			   if INTERNAL threads are used.
**       CONTEXT       -   The context of the stack that received the
**                         the exception.  May be NULL if dumping
**			   stack from another session.
**	output_arg	- Anything that output-fcn might need
**      output_fcn	- TRformat fcn style utility used to send output
**
**    11-Nov-1998 (merja01)
**      Changed routine to be called through CS_dump_stack.  Added
**      scb info and check to passed scb.  Also, implemented use of
**      generic output_fcn.
**    18-oct-1999 (somsa01)
**	Fixed typo.
**    07-Mar-2000 (hweho01)
**      Defined storage for verbose_string.
**    05-jan-2001 (devjo01)
**      Replace STcpy with STlcopy.  STcpy was wrong macro to use, since
**      a limiting length was specified.
**    12-aug-2002 (devjo01)
**	- Enable dumping of other sessions. 
**      - Merge in hanal04's code for determining base address.  
**	- Fix minor memory leak, and only read object list once.
**	- Only print registers values that should be accurate for that
**	  call frame.
**	- Add 'verbose' parameter.
*/        

/* Variable implicitly generated by linker. */
extern unsigned long _procedure_string_table;
extern unsigned long _procedure_table_size;
extern unsigned long _procedure_table;
extern unsigned long _ftext;    /* Start address of the program text.*/
extern unsigned long _etext;    /* First address above the program text.*/

VOID
CS_axp_osf_dump_stack(CS_SCB *pScb, CONTEXT *pContext, PTR output_arg,
	TR_OUTPUT_FCN *output_fcn, i4 verbose)
{

#   define	AXP_REG_S0	 9
#   define	AXP_REG_S1	10
#   define	AXP_REG_S2	11
#   define	AXP_REG_S3	12
#   define	AXP_REG_S4	13
#   define	AXP_REG_S5	14
#   define	AXP_REG_S6	15
#   define	AXP_REG_RA	26
#   define	AXP_REG_PV	27
#   define	AXP_REG_GP	29
#   define	AXP_REG_SP	30

    CS_SCB		*cur_scb;
    void                *prpd;
    CONTEXT		 context;
    char		 verbose_string[80];
    char		 msg_buffer[256] ;
    EX_CONTEXT           ex_context;

    CSget_scb(&cur_scb);

    if (pScb == NULL)
    {
	/* If no session context specified, default to current session. */
	pScb = cur_scb;
    }
    else if (pScb != cur_scb)
    {
	if ( CS_is_mt() )
	{
	    /* Cannot dump stack for another OS thread */
	    return;
	}

	/* Make sure SCB has stack (stack caching) */ 
	if ( !pScb->cs_stk_area )
	{
	    /* Stack sharing is on, and this SCB is not in the server. */
	    return;
	}
    }

    if ( verbose )
    {
	STprintf(msg_buffer,"-----------BEGIN STACK TRACE------------");
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);

	/* Format session and Process info */
	STlcopy(Cs_srv_block.cs_name, msg_buffer, sizeof(Cs_srv_block.cs_name));
	STprintf(verbose_string,
	  "%-8.8s::[%-16.16s, %08.8x]: pid %d: ",
	  PMhost(), msg_buffer, pScb->cs_self, pScb->cs_pid);
    }
    else
    {
	verbose_string[0] = '\0';
    }

    for ( ; /* Something to break out of */ ; )
    {
	/* << out-dent one level */

    /*
    ** Try to protect ourselves against unwinding a corrupted stack.
    */
    if ( EXdeclare(CS_null_handler, &ex_context) )
    {
	STprintf(msg_buffer," Cannot wind further back.  Stack corrupted?");
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
	break;
    }


    if (pContext == NULL)
    {
	if ( pScb != cur_scb )
	{
	    /*
	    ** SAVEREG_FRAME_SIZE definition, and _saved_regs structure
	    ** must match that used by save_regs macro within CS_swuser
	    ** in csll.axp.asm.   Only minimum registers required are
	    ** restored.
	    */
#	    define SAVEREG_FRAME_SIZE      160

	    struct _saved_regs {		
		unsigned long	sr_ra;
		unsigned long	sr_at;
		unsigned long	sr_gp;
		unsigned long	sr_pv;
		unsigned long	sr_saved[7];
	    }	*saved_regs;

	    /* Set minimum number of fields to spoof other session */
	    saved_regs = (struct _saved_regs *)pScb->cs_sp;
	    context.sc_pc = pScb->cs_pc;
	    context.sc_regs[AXP_REG_SP] = pScb->cs_sp + SAVEREG_FRAME_SIZE;
	    context.sc_regs[AXP_REG_S0] = saved_regs->sr_saved[0];
	    context.sc_regs[AXP_REG_S1] = saved_regs->sr_saved[1];
	    context.sc_regs[AXP_REG_S2] = saved_regs->sr_saved[2];
	    context.sc_regs[AXP_REG_S3] = saved_regs->sr_saved[3];
	    context.sc_regs[AXP_REG_S4] = saved_regs->sr_saved[4];
	    context.sc_regs[AXP_REG_S5] = saved_regs->sr_saved[5];
	    context.sc_regs[AXP_REG_S6] = saved_regs->sr_saved[6];
	    context.sc_regs[AXP_REG_RA] = saved_regs->sr_ra;
	}
	else
	{
	    exc_capture_context( &context );
	}
    }
    else
    {
	context = *pContext;
    }

    if ( NULL == ObjList )
    {
	/* Load object information (once) */
	static  char objpath[] = "/ingres/bin/iidbms";
	char   *fullpath;
	i4	pathsize;

	/* Load object information */
	pathsize = STlength(getenv("II_SYSTEM")) + STlength(objpath);
	if ( pathsize >= sizeof(msg_buffer) )
	    fullpath = (char *)MEreqmem(0, pathsize + 1, TRUE, NULL);
	else
	    fullpath = msg_buffer;
	if ( NULL == fullpath )
	{
	    STprintf(msg_buffer,
	      "Error allocating memory in CS_axp_osf_dump_stack" );
	    (*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
	    return;
	}
	STprintf(fullpath, "%s%s", getenv("II_SYSTEM"), objpath);

	add_obj(&ObjList, fullpath);

	if ( fullpath != msg_buffer )
	    MEfree(fullpath);
    }

    /*
    ** Special cases for bogus program counter values. If the program
    ** counter is zero or the fault occurred when we were trying to
    ** fetch an instruction (because the program counter itself was bad)
    ** then we cannot unwind the stack.
    */

    if (context.sc_pc == 0)
    {
        STprintf(msg_buffer, "%sPC is zero - stack trace not available.",
	  verbose_string);
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
    }
    else if (context.sc_pc == context.sc_traparg_a0)
    {
        STprintf(msg_buffer, "%sbad PC (%p) - stack trace not available.",
          verbose_string, context.sc_traparg_a0);
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
    }
    else
    {
	pRPDR		 runTimePDPtr;
	long int	 procedureTableSize;
	char		*procedureNameTable;
        unsigned int	 stk_cnt = 0;
	ulong		 address;
	struct obj	*obj;

	runTimePDPtr = (pRPDR)&_procedure_table;
	procedureTableSize = (long int)&_procedure_table_size;
	procedureNameTable = (char *)&_procedure_string_table;

        /*
	** Loop through up to MAX_STK_UNWIND stack frames.
        */
        while ( (context.sc_regs[AXP_REG_RA] != 0) &&
		(context.sc_pc != 0) &&
	        (stk_cnt++ < MAX_STK_UNWIND) )
	{
	    /* 
	    ** Look up the procedure name.
	    */
	    {
		char *            procedureName;
		long int          currentProcedure;
		char *		  ptemp;
		ulong		  mld_handle, mld_proc;
		char		  linenotxt[64];
		char		  offsettxt[32];

		procedureName = "<unknown>";
		linenotxt[0] = '\0';
		offsettxt[0] = '\0';

		/*
		** The table of run time procedure descriptors is ordered by
		** procedure start address. Search the table (linearly) for the
		** first procedure with a start address larger than the one for
		** which we are looking.
		*/
		for (currentProcedure = 0;
		     currentProcedure < procedureTableSize;
		     currentProcedure++)
		{

		    /*
		    ** Because the way the linker builds the table we need 
		    ** to make special cases of the first and last entries.
		    ** The linker uses 0 for the address of the procedure in
		    ** the first entry, here we substitute the start address
		    ** of the text segment. The linker uses -1 for the address
		    ** of the procedure in the last entry, here we substitute
		    ** the end address of the text segment. For all other
		    ** entries the procedure address is obtained from the
		    ** table.
		    */

		    if (currentProcedure == 0)
		    {
			/* first entry */
			address = (unsigned long int)&_ftext;
		    }
		    else if (currentProcedure == procedureTableSize - 1)
		    {
			/* last entry */
			address = (unsigned long int)&_etext;
		    }
		    else
		    {
			/* other entries */
			address = runTimePDPtr[currentProcedure].adr;
		    }

		    if (address > context.sc_pc)
		    {
			if (currentProcedure != 0)
			{
			    /* PC is in this image in preceeding procedure */
			    procedureName =
			     &procedureNameTable
			      [runTimePDPtr[currentProcedure - 1].irpss];

			    address = runTimePDPtr[currentProcedure - 1].adr;
			    STprintf( offsettxt,"+0x%x", 
				      context.sc_pc - address );

			    /*
			    ** Try to get additional info that will be
			    ** present if compiled for debug.
			    */
			    obj = address_to_obj(ObjList, context.sc_pc);
			    if ( obj )
			    {
				mld_proc = address_to_procedure(obj,
					    context.sc_pc);
				if ( OBJ_FAIL != mld_proc )
				{
				    mld_handle = procedure_to_file(obj,
								   mld_proc);
				    if ( OBJ_FAIL != mld_handle )
				    {
					ptemp = file_name(obj, mld_handle);
					if ( NULL != ptemp )
					{
					    mld_handle = address_to_line(obj,
							    context.sc_pc);
					    if ( OBJ_FAIL != mld_handle )
					    {
						STprintf( linenotxt,
							  " (%s:%d)", ptemp, 
							  mld_handle );
					    }
					}
				    }
				}
			    }

			}
			break; /* stop searching */
		    }
		}

		/* Print a frame. */
		STprintf( msg_buffer, "%s%d %p %p = %s%s%s", verbose_string,
			  stk_cnt, context.sc_sp,
			  context.sc_pc, procedureName,
			  offsettxt, linenotxt );
		(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
	    }

		STprintf( msg_buffer,
		 "  $s0(r9)  0x%p  $s1(r10) 0x%p  $s2(r11) 0x%p",
		  context.sc_regs[9], context.sc_regs[10],
		  context.sc_regs[11] );
		(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
		STprintf( msg_buffer,
		 "  $s3(r12) 0x%p  $s4(r13) 0x%p  $s5(r14) 0x%p  $s6(r15) 0x%p",
		  context.sc_regs[12], context.sc_regs[13],
		  context.sc_regs[14], context.sc_regs[15] );
		(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);

	    if ( address == (ulong)CS_setup )
	    {
		/* No need to unwind further */
		break;
	    }

	    prpd = find_rpd(context.sc_pc);
	    if ( !prpd ) break;
	    unwind(&context, prpd);
	}
    }

    break;

    /* Restore correct indentation */
    }

    if ( verbose )
    {
	STprintf(msg_buffer, "-----------END STACK TRACE----------");
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
    }

    EXdelete();
} /*axp_osf specific stack dump routine */

#endif /* axp_osf */

#if defined(int_lnx) || defined(int_rpl) || defined(a64_lnx)
/*
** Name: CS_lnx_dump_stack	- Diagnostic stack unwind for Linux.
**
** Description:
**  Stack unwind routine for x86 Linux.   This underlying assumption
**  is that the stack is growing downward, and that the call frame
**  is ordered as follows:
**
**	[ param n    ]		
**	   . . .
**	[ param 2    ]		|
**	[ param 1    ]		|  Stack growth direction
**	[ ret_addr   ]		v
**	[ callers BP ] <- BP for called frame.
**
**  Limitation exists in that any procedure that does not have a local
**  stack frame (e.g. in-line function) will not show up in the trace-back.
**
**  Routine assumes all functions have four parameters, since the
**  actual number of parameters passed is not easily determined,
**  and that the parameters are pushed in reverse order on the stack
**  prior to the call.  (Standard for C on this platform).  If fewer
**  parameters are passed, what is dumped is actually part of the
**  callers autovar space.
**
**
**	x86_64 note:
**
**	The x84_64 ABI makes it almost impossible to do a stack
**	backtrace in its default mode, because it omits the push
**	of the frame pointer.  Debuggers like gdb rely on extra
**	"unwind" information emitted by the compiler into a separate
**	ELF section called .eh_frame;  in theory, we could figure
**	out the stack from the unwind tables, IF we knew how to get
**	at them, and IF we know what they meant!  In the absence of
**	a general purpose unwinder library call that works on the
**	x86_64 (and note that backtrace() does NOT work), the
**	best options seem to be a) punt, or b) compile Ingres with
**	-fno-omit-frame-pointer.
**
**	This Sun blog entry:
**	http://blogs.sun.com/eschrock/date/20041119
**	has convinced me that the (minor!) benefits of omitting the frame
**	pointer are marginal compared with the problems, painfully
**	experienced over the last several months, of having not the
**	foggiest clue what the DBMS server might have been doing when
**	it pukes a segv into the error log.
**
**	So, I'm going to arrange for Ingres to be compiled with
**	-fno-omit-frame-pointer for the x86_64 and we can then use
**	the same backtrace mechanism as the x86 (32) does.
**	The routine parameter info is useless (args are passed in
**	registers), so it will be omitted, but the really important
**	stuff is the callback chain anyway.
**	(kschendel Nov 2006)
**  
**   Inputs:
**       pScb          -   Pointer to target SCB, or NULL if current
**			   SCB is the target.   Stack back traces
**			   for other sessions are only supported
**			   if INTERNAL threads are used.
**       ucp           -   User context pointer provided by OS.      
**	output_arg	- Anything that output-fcn might need
**      output_fcn	- TRformat fcn style utility used to send output
**	 verbose       -   More text if non-zero.
**
**    20-Oct-2004 (devjo01)
**	Initial version. 
**    02-Oct-2007 (hanje04 for kschedel)
**	Fix up for AMD64 and stop it indefinitly looping.
**    22-Jul-2008 (hanal04) Bug 120532
**        If the pass ucp is NULL and we are trying to report the stack
**        for this this thread then get the context information by calling
**        getcontext().
**	24-Jul-2008 (kschendel)
**	    Guard against garbage BP's, causing another indefinite loop.
**
*/        
# include <execinfo.h> /* Prototype for backtrace_symbols */

/* As register names vary accoss processors define our own here */
# if defined(int_lnx) || defined(int_rpl)
# define REG_IP REG_EIP /* Instruction Pointer */
# define REG_BP REG_EBP /* Base Pointer */
# define REGTYPE u_i4
# elif defined(a64_lnx)
# define REG_IP REG_RIP /* Instruction Pointer */
# define REG_BP REG_RBP /* Base Pointer */
# define REGTYPE long
# else
# define REG_IP DEFINE_REGIP_FIRST
# define REG_BP DEFINE_REGBP_FIRST
# define REGTYPE DEFINE_REGTYPE_FIRST
# endif

/* Display formats need to vary for 32bit vs. 64bit so define them here */
# define EXCEPT_FMT "%-8.8s::[%-16.16s, %p]: pid %d:"
# define SYMBUF_FMT "[%p]"
# define STATIC_TRACE_FMT "%d: %p %s <static function, no info available>"
# ifdef LP64
/* params are garbage on x64_64, don't even try */
# define STACK_TRACE_FMT "%d: %p %s( ... )"
# else
# define STACK_TRACE_FMT "%d: %p %s(%p %p %p %p)"
# endif
VOID
CS_lnx_dump_stack(CS_SCB *pScb, ucontext_t *ucp, PTR output_arg,
	TR_OUTPUT_FCN *output_fcn, i4 verbose)
{

/* Largest plausible single stack frame to avoid runaway */
#define                        MAX_STK_FRAME   32768   /* >32K is insane */


    int			 frame, numframes;
    int			 getsymbols, haveexception;
    void		*retaddrs[MAX_STK_UNWIND];
    char		**symbols = NULL;
    char		*symbol, verbose_string[80];
    char		 msg_buffer[512], symbuf[33];
    CS_SCB		*cur_scb;
    EX_CONTEXT           ex_context;
    ucontext_t		 lucp;
    struct call_frame
    {
	struct call_frame *next_frame;
	void *ret_addr;
	REGTYPE  params[4];	/* As many putative params as you like */
    }	*currentframe;
    
    CSget_scb(&cur_scb);

    haveexception = 0;
    if (pScb == NULL)
    {
	/*
	** If no session context specified, assume stack trace triggered
	** by an exception in the context of the current session.
	*/
	pScb = cur_scb;
	haveexception = 1;

        if(!ucp)
        {
            if(getcontext(&lucp))
                return;
            ucp = &lucp;
        }
    }

    if ( pScb != cur_scb )
    {
	return;
    }

    if ( verbose )
    {
	STprintf(msg_buffer,"-----------BEGIN STACK TRACE------------");
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);

	/* Format session and Process info */
	STlcopy(Cs_srv_block.cs_name, msg_buffer, sizeof(Cs_srv_block.cs_name));
	STprintf(verbose_string, EXCEPT_FMT,
	  PMhost(), msg_buffer, pScb->cs_self, pScb->cs_pid);
    }
    else
    {
	verbose_string[0] = '\0';
    }

    getsymbols = verbose;
    for ( ; ; )
    {
	/*
	** Try to protect ourselves against corrupted memory.
	*/
	if ( EXdeclare(CS_null_handler, &ex_context) )
	{
	    STprintf(msg_buffer,"Exception in stack unwind");
	    (*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
	    if ( !getsymbols )
	    {
		/* Two tries max */
		break;
	    }
	    STprintf(msg_buffer,"Trying again without symbols");
	    (*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
	    symbols = NULL; /* Don't try to recover any malloced mem. */
	    getsymbols = 0;
	    EXdelete();
	    continue;
	}

	/*
	** Obtain up to MAX_STK_UNWIND program counter values.
	** First PC address is that of the exception.  Subsequent
	** addresses are obtained by unwinding the call stack starting
	** at the frame pointer in the ucontext record.
	**
	** Double-check the currentframe pointer we get.  Ideally
	** cs-handler should learn how to not infinite loop, but this is
	** a quick fixup for now.
	*/
	retaddrs[0] = (void*)ucp->uc_mcontext.gregs[REG_IP];

	for ( numframes = 0, currentframe =
	      (struct call_frame *)(ucp->uc_mcontext.gregs[REG_BP]);
	      abs((SCALARP) currentframe) > 0x10000 &&
	      ++numframes < MAX_STK_UNWIND &&
	      currentframe < currentframe->next_frame &&
	      currentframe + MAX_STK_FRAME > currentframe->next_frame;
	      currentframe = currentframe->next_frame )
	{
	    retaddrs[numframes] = currentframe->ret_addr;
	}

	if ( getsymbols )
	    symbols = backtrace_symbols( retaddrs, numframes );

	currentframe =
	     (struct call_frame *)(ucp->uc_mcontext.gregs[REG_BP]);
	for ( frame = 0; frame < numframes; frame++ )
	{
	    bool staticfunc = FALSE;

	    if ( symbols )
	    {
		/* first check for missing static functions */
		symbol = STrindex( symbols[frame], "(", 0 );
		if ( NULL == symbol )
		    staticfunc = TRUE;

		symbol = symbols[frame];

	 	/* now strip off binary location for stack dump */
		symbol = STrindex( symbols[frame], "/", 0 );
		if ( NULL == symbol )
		    symbol = symbols[frame];
		else
		    symbol++;

	    }
	    else
	    {
		STprintf( symbuf, SYMBUF_FMT, retaddrs[frame] );
		symbol = symbuf;
	    }

#if defined(LP64)
	    /* params are garbage on x64_64, don't even try */
	    STprintf( msg_buffer, "%s%d:%p %s( ... )",
	      verbose_string, frame, currentframe, symbol);
#else
	    if ( staticfunc == TRUE )
		STprintf( msg_buffer, STATIC_TRACE_FMT, frame, currentframe, symbol );
		/* No function info for static functions so state that */
	    else
		STprintf( msg_buffer, STACK_TRACE_FMT,
		    frame, currentframe, symbol,
		    currentframe->params[0], currentframe->params[1], 
		    currentframe->params[2], currentframe->params[3] );
#endif

	    (*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
	    currentframe = currentframe->next_frame;
	}

	if ( symbols ) free(symbols);
	break;
    }

    if ( verbose )
    {
	STprintf(msg_buffer, "-----------END STACK TRACE----------");
	(*output_fcn)(output_arg, STlength(msg_buffer), msg_buffer);
    }

    EXdelete();
}

#endif /*defined(int_lnx) || defined(int_rpl)*/


#ifdef xCS_WAIT_INFO_DUMP
/*{
** Name: CS_display_sess_short() - print out what sessions are waiting on
**
** Description:
**	TRdisplay() what all sessions are waiting on.  This function is called
**	to display all the sessions state when the server has nothing to do, as
**	an aid to in determining performance throughput bottlenecks (ie. on a
**	multiprocessor).
**
**	It currently only dumps this information every 100th time the server
**	has to wait.  This should be changed to be a server configurable item.
**	The code as well as the call to it has been ifdef'd out with the
**	xCS_WAIT_INFO_DUMP flag.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
** History:
**      23-mar-90 (mikem)
**          Created.
**	28-Jan-91 (anton)
**	    Add CScondition support
*/
static VOID
CS_display_sess_short()
{
    CS_SCB		*an_scb;
    char		state_buf[132];
    char		*str = &state_buf[0];
    static int		count = 0;

    count++;
    if ((count % 100) != 0)
	return;

    /*
    ** Lock Cs_srv_block to ensure the integrity of cs_known_list.
    ** Don't worry about locking the scb, since we are just displaying
    ** snapshot information it's not critical.
    */
#ifdef MCT
    MCT_GETSPIN(&Cs_srv_block.busy);
#endif /* MCT */


    for (an_scb = Cs_srv_block.cs_known_list->cs_next;
	an_scb && an_scb != Cs_srv_block.cs_known_list;
	an_scb = an_scb->cs_next)
    {
	switch (an_scb->cs_state)
	{
	    case CS_FREE:
		*str++ = 'F';
		*str++ = 'R';
		*str++ = ' ';
		break;

	    case CS_COMPUTABLE:
		*str++ = 'C';
		*str++ = 'P';
		*str++ = ' ';
		break;

	    case CS_STACK_WAIT:
		*str++ = 'S';
		*str++ = 'W';
		*str++ = ' ';
		break;

	    case CS_UWAIT:
		*str++ = 'U';
		*str++ = 'W';
		*str++ = ' ';
		break;

	    case CS_EVENT_WAIT: 
		if (an_scb->cs_memory & CS_DIO_MASK)
		{
		    *str++ = 'D';
		    *str++ = 'I';
		    *str++ = ' ';
		}
		else if (an_scb->cs_memory & CS_BIO_MASK)
		{
		    *str++ = 'B';
		    *str++ = 'I';
		    *str++ = ' ';
		}
		else if (an_scb->cs_memory & CS_LOCK_MASK)
		{
		    *str++ = 'L';
		    *str++ = 'K';
		    *str++ = ' ';
		}
		else if (an_scb->cs_memory & CS_LKEVENT_MASK)
		{
		    *str++ = 'K';
		    *str++ = 'E';
		    *str++ = ' ';
		}
		else if (an_scb->cs_memory & CS_LGEVENT_MASK)
		{
		    *str++ = 'G';
		    *str++ = 'E';
		    *str++ = ' ';
		}
		else if (an_scb->cs_memory & CS_LOG_MASK)
		{
		    *str++ = 'L';
		    *str++ = 'G';
		    *str++ = ' ';
		}
		else if (an_scb->cs_memory & CS_TIMEOUT_MASK)
		{
		    *str++ = 'T';
		    *str++ = 'M';
		    *str++ = ' ';
		}
		break;

	    case CS_MUTEX:
	    {
		*str++ = 'M';
		*str++ = 'U';
		*str++ = ' ';
		break;
	    }
		
	    case CS_CNDWAIT:
	    {
		*str++ = 'C';
		*str++ = 'N';
		*str++ = ' ';
		break;
	    }
		
	    default:
	    {
		*str++ = '?';
		*str++ = '?';
		*str++ = ' ';
		break;
	    }
	}
    }
    *str = EOS;

    TRdisplay("%@@ %s\n", state_buf);
}
#endif /* xCS_WAIT_INFO_DUMP */
