/*
** Copyright (c) 1986, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <st.h>
#include    <ex.h>
#include    <me.h>
#include    <pc.h>
#include    <er.h>
#include    <si.h>
#include    <tr.h>
#include    <gc.h>
#include    <evset.h>

#include    <times.h>
#ifdef OS_THREADS_USED
#include    <signal.h>
#endif
#include    <exhdef.h>
#include    <iosbdef.h>
#include    <va_rangedef.h>
#include    <gen64def.h>
#include    <efndef.h>
#include    <iledef.h>
#include    <jpidef.h>
#include    <libicb.h>
#include    <prtdef.h>
#include    <psldef.h>
#include    <sjcdef.h>
#include    <ssdef.h>
#include    <stsdef.h>
#include    <vadef.h>
#include    <pdscdef.h>
#include    <syidef.h>
#include    <iledef.h>
#include    <efndef.h>
#include    <lib$routines.h>
#include    <starlet.h>

#include    <csinternal.h>
#ifdef OS_THREADS_USED
#include    <clconfig.h>
#include    <csev.h>
#include    <astjacket.h>
#endif

#ifdef KPS_THREADS
#include     <kpbdef.h>
#include     <lib_bigpage.h>
#include     <exe_routines.h>
extern u_i4  MMG$GL_PAGE_SIZE;
extern u_i4  SGN$GL_KSTACKPAG;
#endif

#include    "cslocal.h"
#include    "cssampler.h"

#define HZ 50 /* usually found in sys/param.h on U*X */

#if defined(ALPHA)
#define LIB_GET_CURR_INVO_CONTEXT lib$get_curr_invo_context
#define LIB_GET_PREV_INVO_CONTEXT lib$get_prev_invo_context
#elif defined(IA64)
#define LIB_GET_CURR_INVO_CONTEXT lib$i64_get_curr_invo_context
#define LIB_GET_PREV_INVO_CONTEXT lib$i64_get_prev_invo_context
#else
#error "Unrecognised platform."
#endif 

/**
**
**  Name: CSHL.C - Control System "High Level" Functions
**
**  Description:
**      This module contains those routines which provide system dispatching 
**      services to the DBMS (only, at the moment).  "High level" refers to
**      those routines which can be implemented in C as opposed to macro.
**
**	    CSvms_uic() - Return VMS UIC.
**          CS_xchng_thread() - Pick next task and return it.
**          CS_quantum() - Timer AST for the VMS CS Module.
**          CS_toq_scan() - Scan timeout queue.
**          CS_alloc_stack() - Allocate a thread's stack.
**          CS_deal_stack() - Deallocate a thread's stack.
**          CS_setup() - Run the thread thru its appropriate functions based on
**		    the modes set by the mainline code.
**          CS_find_scb() - Find the SCB belonging to a particular thread id.
**          CS_eradicate() - Fully shutdown the thread -- removing it from any
**		    competition in the server.
**          CS_dump_scb() - Dump the high level scb contents.
**          CS_rcv_request() - Prime system to receive a request for new 
**		    thread addition.
**          CS_del_thread() - Final processing for deletion of a thread.
**          CS_admin_task() - Thread addition and deletion thread.
**          CS_exit_handler() - Final CS server processing.
**          CS_fmt_scb() - Format an SCB for display.
**          CS_last_chance() - VMS last chance exception handler to catch
**		    smashed stacks.
**	    CS_move_async() - Process async event info that occurred while
**		    the cs_inkernel flag was set.
**	    CS_change_priority() - Change threads priority.
**          CS_DIAG_dump_stack() - dump stack of all threads to DIAG file
**          CS_DIAG_dump_query() - dump current query of all threads
**	    CSdump_stack() - External way to produce CS_dump_stack()
**
**  History:
**      23-Oct-1986 (fred)
**          Created.
**      21-jan-1987 (fred)
**          Added cputime/thread support
**      19-Nov-1987 (fred)
**          Added last chance exception handling support.
**	31-oct-1988 (rogerk)
**	    Only collect CPU stats when exchange thread if old or new session
**	    has CS_CPU_MASK set.
**	    Only write records to accounting file if CS_ACCNTING_MASK set and
**	    it is a user session.
**	 7-nov-1988 (rogerk)
**	    Added non-ast task switching and non-ast methods of protecting
**	    against asynchronous routines. Call CS_ssprsm directly instead 
**	    of through AST's.  Added new inkernel protection.  When turn
**	    of inkernel flag, check if need to call CS_move_async.
**	28-feb-1989 (rogerk)
**	    Added CS_change_priority routine to support cross process semaphores
**	10-apr-1989 (rogerk)
**	    Took out setting of inkernel while changing thread's priority in
**	    CS_change_priority().  Assume that caller has set it.
**	15-May-1989 (rogerk)
**	    Added allocated_pages argument to MEget_pages call.
**      07-jun-89 (fred)
**	    Added parameter count to (cs_elog) call for bad stack.  Want to
**	    print out username.
**      27-jul-89 (fred)
**	    Make CSsuspend() call on writes from CS_setup() interruptable to
**	    improve force abort handling
**      15-aud-89 (jennifer)
**          Add parameter to ERsend call to tell type of message.
**	 5-sep-1989 (rogerk)
**	    Don't swap out thread in CS_quantum if it has CS_NOSWAP set.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		New CS_GWFIO_MASK indicates that a session is suspended on
**		Gateway I/O. It is informational only, and is used to
**		distinguish Gateway suspensions from normal Ingres suspensions.
**	    Fred also put in a bunch of stuff for the CPU governor, but there
**	    were no comments in the 6.3 History section.
**	09-nov-91 (ralph)
**	    6.4->6.5 merge:
**	    20-jun-91 (ralph)
**		Too few parameters for msg E_CS00FD_THREAD_STK_OVFL in
**		CS_last_chance
**	17-nov-92 (walt)
**		Added global variable Cs_cactus_fp for CS_alloc_stack to use when
**		setting up new threads.
**		Also, now get the memory page size dynamically with a SYS$GETSYI
**		call rather than hardcoded 512's.
**	18-dec-92 (wolf) 
**	    Remove #include <in.h>
**	05-may-1993 (walt)
**	    Alpha version - print out alpha-numbered registers in CS_fmt_scb.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	13-jul-95 (dougb)
**	    Correct name in CS_xchng_thread prototype and comments.  Change
**	    name of quantum_ticks to CS_lastquant -- matching Unix CL.  Have
**	    CS_xchng_thread() skip priorities with no ready threads.
**	27-jul-95 (dougb)
**	    Correct a few problems uncovered by previous change.  In
**	    particular, CS_eradicate() and CS_change_priority() must
**	    maintain the ready mask in a consistent fashion.
**	    Update CS_move_async() to handle a NULL parameter -- removing
**	    conditions from calls in csinterface.c.
**	    Make queue inserts consistent throughout for easier reading.
**	31-jul-95 (dougb)
**	    Integrate some of Jon Jenson's Unix CL change 419331:
**		9-Jun-1995 (jenjo02)
**	    In CS_xchng_thread(), don't remove/insert thread on rdy queue if
**	    it's already last on the queue.
**	05-dec-1995 (whitfield)
**	    Appropriated CS_vms_dump_stack and CS_dump_stack from VAX CL.
**	07-dec-95 (dougb)
**	    Use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.
**	31-dec-95 (whitfield)
**	    Changed references to VAX/VMS to VMS since this code concerns
**	    Alpha VMS as well.  Also, integrated Doug Bunting's change to
**	    use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.
**	04-jan-96 (dougb)
**	    Remove some incredibly old code in CS_deal_stack().  Add a few
**	    comments from the Unix version of CS_xchng_thread().  Carry
**	    over a few other Unix changes to cs_handler().  Integrate some
**	    of the recent Unix CL changes:
**	  ancient history:
**		28-Jan-91 (anton)
**	    Added CS_CNDWAIT state to CS_dump_scb().
**		16-Apr-91 (anton)
**	    In cs_handler(), set buffer length from EX_MAX_SYS_REP as per
**	    6.3 CL SPEC.  Added description.
**	  change 415512:
**		29-Nov-94 (liibi01)
**	    Bug 64041, cross integation from 6.4.  Add server name and time
**	    stamp to error text produced in cs_handler().  Add header file
**	    gcccl.h for constant GCC_L_PORT.
**	  change 419596:
**		09-Jun-95 (jenjo02)
**	    In CS_xchng_thread(), don't scan ready queues if the only
**	    runnable thread is the idle thread.
**	  change 421045:
**		19-sep-1995 (nick)
**	    In CS_admin_task(), remove magic number 12; set next_mode using
**	    the correct mask values instead.
**	24-jan-96 (dougb) bug 71590
**	    Correct use of EXsys_report() -- use EXsigarr_sys_report().
**	    Restore ASTs before exiting CS_last_chance().  Call cs_elog
**	    routine with a pointer to the status -- avoid an ACCVIO.
**	    Change interface to CS_ssprsm() instead of using stub routine
**	    CS_restart().  New parameter to CS_ssprsm() indicates that no regs
**	    should be saved nor should a new thread be chosen -- just restore
**	    given context.  Allows us to keep the server running after a
**	    non-fatal error in one thread.  (Earlier code always failed in an
**	    illegal call to SYS$UNWIND().)
**	    Remove local (and incorrect) definition of MCH vector.
**	    Add a few more xDEBUG TRdisplay() calls.
**	19-feb-96 (duursma)
**	    Integrated some pieces of UNIX 423116 change.
**	01-aug-97 (teresak) Added changes for OS threads.
**          12-dec-1996 (canor01)
**           To support the sampler thread, give a monitor session a
**           thread type of SC_MONITOR so it won't show up as a user
**           session to the sampler, but count it among user threads
**           internally.
**          18-feb-1997 (hanch04)
**           As part of merging of Ingres-threaded and OS-threaded servers,
**           check Cs_srv_block to see if server is running OS threads.
**          02-may-1997 (canor01)
**           Sessions created with the CS_CLEANUP_MASK will be shut
**           down before any other system threads.  When they exit, they
**           must call CSterminate() to allow the normal system threads
**           to exit.
**      11-mar-1998 (kinte01)
**         Create routine CS_CP_SEM_cleanup for VMS
**	17-mar-1998 (kinte01)
**	   Add cs_hwm_active & cs_num_active as they were not previously 
**	   being set
**	19-may-1998 (kinte01)
**	   Cross-integrate Unix change 434548
**         13-Feb-98 (fanra01)
**           Made changes to move attch of scb to MO just before we start
**           running.
**           This is so the SID is initialised to be used as the instance.
**      20-may-1998 (kinte01)
**         Cross-integrate Unix change 435120
**         31-Mar-1998 (jenjo02)
**           Added *thread_id output parm to CSadd_thread().
**	11-jun-1998 (kinte01)
**	   Cross-integrate change 435890 from oping12
**         18-May-1998 (horda03)
**            X-Integrated change 427896 to enable Evidence Set to be
**            produced.
**      01-jul-1998 (kinte01)
**         Changed diag_qry to cs_diag_qry to comply with coding standards.
**      19-may-99 (kinte01)
**          Added casts to quite compiler warnings & change nat to i4
**          where appropriate
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**      22-Aug-2000 (horda03)
**         Use P1 space to store session stacks. This makes ~1GB of data
**         available for stacks, and frees the space in P0 (where the
**         Global sections are mapped). If a stack area is near the P0
**         boundary, then a call to SYS$GOTO_UNWIND may cause an EXIT
**         UNWIND and a DBMS server will terminate.
**         (102291)
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**      01-Dec-2000 (horda03)
**         CSSAMPLING accvio's due to differences in use of cs_memory
**         and cs_sync_obj on VMS to UNIX. Bug 56445 introduced the
**         field cs_sync_obj in CS_SCB to hold the semaphore ptr being
**         addressed in CS routine rather than the miss use of cs_memory.
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	12-apr-2002 (kinte01)
**	   Cleaned up more compiler warnings
**	13-aug-2002 (abbjo03)
**	   Remove leftover TRdisplay debug lines.
**	16-oct-2002 (abbjo03)
**	   Add casts to assignment to SP/AL registers to avoid warnings.
**      18-oct-2002 (horda03) Bug 108966
**         Prevent INGSTART ACCVIO when required privilege not set.
**      22-Oct-2002 (hanal04) Bug 108986
**          Added CS_default_output_fcn() as a default output
**          function to be used by CS_dump_stack().
**	25-Apr-2003 (jenjo02) SIR 111028
**	    Added external function CSdump_stack(), wrapper to
**	    CS_dump_stack().
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	04-nov-2005 (abbjo03)
**	    Split CS_SCB bio and dio counts into reads and writes.
**	29-apr-2007 (jonj/joea)
**	    In CS_last_chance, only copy a good cs_orig_sp.  In CS_move_async,
**	    only reenable ASTs if they were enabled on entry.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	14-oct-2008 (joea)
**	    Integrate 29-sep-2000 change to unix version. Split cs_bio_time
**	    and cs_dio_time in cs_wtstatistics into read/write counters. 
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**	21-nov-2008 (joea)
**	    Rename CS_find_scb to IICS_find_scb to support OS threads.
**	01-dec-2008 (joea)
**	    Remove prototypes for CSp/v_semaphore.
**	08-dec-2008 (joea)
**	    Port CS_update_cpu from Unix CL.  Rename CS_lastquant to really
**	    match the Unix CL.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**          replace homegrown itemlists by iledef
**          use efn$c_enf in all syncvh service calls
**      4-feb-2010 (stegr01/joea)
**          Add support for KP Services.
**      9-mar-2010 (joea)
**          In CS_alloc_stack, if exe$kp_user_alloc_kpb returns an error don't
**          use lib$stop, but instead report it and return E_CL25F5 upstream.
**          In memstk_allock, report when a memory allocation error occurs.
**          In thread_end, return status instead of using lib$stop when an
**          error occurs.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN CS_SCB  *CS_xchng_thread(); /* Pick next thread to run */
FUNC_EXTERN i4     CS_quantum();       /* timer AST to pick next thread */
FUNC_EXTERN void    CS_toq_scan();      /* Scan request timeout queue */
FUNC_EXTERN void    CS_ssprsm();	/* suspend a task, resume another */
FUNC_EXTERN void    CS_ast_ssprsm();	/* suspend current task, resume next */
FUNC_EXTERN void    CS_jmp_start();	/* start a session on empty stack */
FUNC_EXTERN void    CS_eradicate();	/* unconditionally wipe out a session */
FUNC_EXTERN void    CS_fmt_scb();	/* debug format an scb */
FUNC_EXTERN void    CS_move_async();	/* process async event info */
FUNC_EXTERN void    CS_default_output_fcn(PTR arg1, i4 msg_length,
					  char *msg_buffer);
FUNC_EXTERN void    CS_change_priority(); /* change session priority */
FUNC_EXTERN STATUS  CS_admin_task();	/* add and delete threads */
FUNC_EXTERN void    CS_detach_scb();    /* make an SCB unknown to MO/IMA */
FUNC_EXTERN bool
EXsigarr_sys_report( i4 *sigarr,	/* Report errors, call fr VMS hndlr */
		    char *buffer );

#ifdef KPS_THREADS
static i4 kpb_alloc(i4 *bufsiz, KPB_PPQ kpb);
static i4 memstk_alloc(KPB_PQ kpb, i4 stack_pages);
static i4 thread_end(KPB_PQ kpb, i4 status);
#endif


/*
** Definition of all global variables used by this file.
*/

GLOBALREF CS_SYSTEM           Cs_srv_block;
GLOBALREF CS_ADMIN_SCB	      Cs_admin_scb;
GLOBALREF CS_SCB	      Cs_idle_scb;
GLOBALREF CS_SCB              Cs_repent_scb;
GLOBALREF PTR		      Cs_save_exsp;
#ifdef OS_THREADS_USED
GLOBALREF i4                  CSslavepids[];
#endif
GLOBALDEF ILE3     	      Cs_cpu_jpi[] = {
				{sizeof(i4), JPI$_CPUTIM, 0, 0},
				{ 0, 0, 0, 0}
			      };

GLOBALDEF i8 	Cs_cactus_fp;	/* CSdispatch's fp, saved for new threads. */
GLOBALDEF i4	Cs_lastquant;
GLOBALDEF i4                   Cs_nextpoll ZERO_FILL;
GLOBALDEF i4                   Cs_polldelay = 500;
GLOBALDEF i4                   Cs_incomp ZERO_FILL;

GLOBALDEF i4 CS_sched_quantum [CS_LIM_PRIORITY]; /* Last quantum when a session
                                                 ** was scheduled
                                                 */



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

/*
**	Items for an Alpha sys$getsyi runtime call to obtain the memory page size
**	for this system.
*/
i2		cpu_pagesize;
unsigned short	cpu_pagesize_len;
ILE3	itmlst[2] = 
{
		{4, SYI$_PAGE_SIZE, &cpu_pagesize, &cpu_pagesize_len},
		{0, 0, 0, 0}
};


/*{
** Name: CSvms_uic	- Return VMS UIC for internal CL usage.
**
** Description:
**      This routine returns the VMS UIC for the current thread.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
**	Returns:
**	    VMS UIC as an int.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-Feb-1988 (fred)
**          Created on Jupiter.
*/
i4
CSvms_uic()
{
    if (Cs_srv_block.cs_current)
	return(Cs_srv_block.cs_current->cs_uic);
    else
	return(0);
}			    

/*
** Name: CS_xchng_thread	- Return the next thread to be active
**
** Description:
**      This routine is called at process level, and will return the next 
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
**	priority			Highest ready priority
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
**	19-jul-93 (walt)
**	    Nolonger use event flag zero.  Call lib$get_ef for an
**	    event flag instead.
**	17-jul-95 (dougb)
**	    Skip priorities with no ready threads (from Unix CL).
**	31-jul-95 (dougb)
**	    Integrate some of Jon Jenson's Unix CL change 419331:
**		9-Jun-1995 (jenjo02)
**	    Don't remove/insert thread on rdy queue if it's already last
**	    on the queue.
**	04-jan-95 (dougb)
**	    Integrate some more of Jon Jenson's Unix CL changes:
**		9-Jun-1995 (jenjo02)
**	    Don't scan ready queues if the only runnable thread is the
**	    idle thread.
**	05-jul-00 (devjo01)
**	    Turn of AST delivery during thread selection.   Correct
**	    comments in function banner which are almost completely
**	    out of sync. with current usage and implementation.
**      04-Nov-2009 (horda03) Bug 122849
**          Give lower priority threads the chance to execute. The next
**          thread selected will be the Highest Priority COMPUTABLE
**          thread unless there is a lower priority thread that has
**          ben waiting for over 2 quantums, then a thread from
**          the highest priority queue that has been waiting the longest
**          will be chosen.
*/

CS_SCB	*
CS_xchng_thread(scb, priority)
CS_SCB             *scb;
i4		   priority;
{
    i4                 i;
    i4		new_cpu;
    CS_SCB              *new_scb;
    CS_SCB              *pri_scb = NULL;
    CS_SCB		*old_scb = Cs_srv_block.cs_current;
#ifdef	CS_STACK_SHARING
    i4			oo_stacks = 0;
#endif
    i4                 nevents, tim;
    i4			reenableasts;
    i4                  new_priority;
    i4                  oldest_quantum;

    /*
    ** Make sure AST's are disabled, so an AST delivered
    ** CSresume will not see queues & cs_state in a
    ** transient state. (bug 102014).
    */
    reenableasts = (sys$setast(0) != SS$_WASCLR);

    /*
    ** If the idle thread is the only thing on the ready queues,
    ** don't waste time scanning for that which won't be found.
    */
    if ( CS_PIDLE == priority )
    {
	pri_scb = (CS_SCB*)&Cs_idle_scb;
	new_priority = CS_PIDLE;
    }
    else for (i = priority; i >= 0; i--)
    {
	/* wander thru the queues looking for work */

	/* Skip to next priority if nothing is ready here. */
	if ( !( Cs_srv_block.cs_ready_mask & ( CS_PRIORITY_BIT >> i )))
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

		status = CS_alloc_stack(new_scb);
		if (status == OK)
		{
		    new_scb->cs_state = CS_COMPUTABLE;
		    break;
		}
		else if (status == E_CS000B_NO_FREE_STACKS)
		{
		    oo_stacks = 1;  /* We are out of stacks */
		    continue;
		}
	    }
#endif
	}

	if (new_scb != Cs_srv_block.cs_rdy_que[i])
	{
	    /* then there is a ready job here.
            ** Select the job if it is the HIGHEST priority job,
            ** or it is the oldest job waiting to be scheduled
            ** (over 2 quantums old).
            */

            if (!pri_scb)
            {
               pri_scb = new_scb;

               oldest_quantum = CS_lastquant -1;

               new_priority = i;

               if (CS_sched_quantum [i] == CS_NO_SCHED)
               {
                  /* Indicate the quantum where the ready job was detected. */
                  CS_sched_quantum [i] = CS_lastquant;
               }
            }
            else if (CS_sched_quantum [i] == CS_NO_SCHED)
            {
                /* Not the HP ready job, and not seen before so
                ** Indicate the quantum where the ready job was detected.
                */
                CS_sched_quantum [i] = CS_lastquant;
            }
            else if ( (i != CS_PIDLE) && (CS_sched_quantum [i] < oldest_quantum))
            {
               /* This ready job has been waiting long enough, select it unless a
               ** lower priority ready job has been waiting longer.
               */
               pri_scb = new_scb;

               oldest_quantum = CS_sched_quantum [i];

               new_priority = i;
            }
	}
	else
	{
	    /* Remember that nothing is ready at this priority. */
	    Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> i);
            CS_sched_quantum [i] = CS_NO_SCHED;
	}
    }

    if (pri_scb == NULL)
    {
	/*
	** Serious problems.  The idle job is always ready.
	*/
	(*Cs_srv_block.cs_elog)(E_CS0007_RDY_QUE_CORRUPT, 0, 0);
	Cs_srv_block.cs_state = CS_ERROR;
	if (reenableasts) sys$setast(1);
	return(scb);
    }

    Cs_srv_block.cs_current = pri_scb;
    if (pri_scb == (CS_SCB *)&Cs_idle_scb)
    {
	Cs_srv_block.cs_state = CS_IDLING;
    }
    else
    {
        Cs_srv_block.cs_state = CS_PROCESSING;

        CS_sched_quantum [new_priority] = CS_lastquant;
    }

    /*
    ** If newly picked thread is already last on its ready queue,
    ** don't unhook/rehook into the same position!.
    */
    if ( pri_scb->cs_rw_q.cs_q_next != Cs_srv_block.cs_rdy_que[new_priority] )
    {
	pri_scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
	    pri_scb->cs_rw_q.cs_q_prev;
	pri_scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
	    pri_scb->cs_rw_q.cs_q_next;

	/* and put on tail of ready que by priority */
	pri_scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_rdy_que[new_priority];
	pri_scb->cs_rw_q.cs_q_prev =
	    Cs_srv_block.cs_rdy_que[new_priority]->cs_rw_q.cs_q_prev;
	pri_scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = pri_scb;
	pri_scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = pri_scb;
    }

    if (reenableasts) sys$setast(1);

    /*
    ** If the old or new thread needs to collect CPU statistics, then we need
    ** to figure get the DBMS CPU usage.  If the new session is collecting
    ** statistics, then we need to store the current DBMS CPU time so that
    ** when we later swap out the session, we can calculate its cpu usage.
    ** If the old session is collecting cpu stats, then increment its cpu
    ** usage.
    */
    if ((pri_scb->cs_mask & CS_CPU_MASK) || 
	(old_scb && old_scb->cs_mask & CS_CPU_MASK))
    {
	IOSB iosb;

	Cs_cpu_jpi[0].ile3$ps_bufaddr = (PTR)&new_cpu;

	i = sys$getjpiw(EFN$C_ENF, 0, 0, Cs_cpu_jpi, &iosb, 0, 0);
	if (i & 1)
	    i = iosb.iosb$w_status;
	if (( i & 1) == 0)
	{
	    (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &i, 0);
	}
	else
	{
	    /* Update the exitting thread's CPU time count */
	    if (old_scb && (old_scb->cs_mask & CS_CPU_MASK))
		old_scb->cs_cputime += (new_cpu - Cs_srv_block.cs_cpu);
	    Cs_srv_block.cs_cpu = new_cpu;
	}
    }

#ifdef CS_MIDQUANTUM
    /* Not being suspended mid quantum so don't set it. */
    pri_scb->cs_mask &= ~CS_MID_QUANTUM_MASK;	/* make sure it is off */
#endif

    return(pri_scb);
}

/*
** Name: CS_quantum	- Timer AST for VMS System
**
** Description:
**      This routine is normally called as an AST.  It notices that 
**      the timer has expired, and if there is new work to be done 
**      it declares and AST to be delivered to cause the new thread 
**      to be scheduled.  It also reschedules itself to be delivered 
**      the current time quantum in the future.  If the current task 
**      was started (via an event completion AST) in the middle of a 
**      quantum, then it is granted dispensation for having only half 
**      a quantum of time, and is not descheduled until the next quantum 
**      expiration. 
**
**	If the current session has CS_NOSWAP set in its status mask, then
**	it is not swapped out in this quantum.  This is used by threads
**	while holding Cross Process Semaphores (spin locks).
** 
**      If the calling mode parameter is 0, then this call is the initial 
**      call to start up the timer.  In this case, no checks are made, 
**      the timer service is simply invoked to expire one quantum in the 
**      future.
**
** Inputs:
**      mode                            0 indicates initial (non AST) call
**                                      CS_IDLE_ID indicates "normal" operation
**
** Outputs:
**      none
**	Returns:
**	    SS$_NORMAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-Oct-1986 (fred)
**          Initial Definition.
**	20-Sep-1988 (rogerk)
**	    Don't add idle statistics for dbms task threads.
**	    Since they are expected to be non-runnable most of the time,
**	    they tend to skew the server statistics.
**	 7-nov-1988 (rogerk)
**	    Changed CS_ssprsm call to CS_ast_ssprsm.  The normal CS_ssprsm
**	    routine does not expect to be called from AST level.
**	    If the server control block is busy (cs_inkernel set) then don't
**	    look at any of the scb lists as they might not be consistent.
**	    Therefore, don't call CS_toq_scan or CS_ast_ssprsm if inkernel.
**	    Added check before switching threads to check if the next session
**	    to be resumed is the same one currently running.
**	 5-sep-1989 (rogerk)
**	    Don't swap out thread if it has CS_NOSWAP set in status mask.
**	    Threads holding spin locks will mark themselves noswap.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		New CS_GWFIO_MASK indicates that a session is suspended on
**		Gateway I/O. It is informational only, and is used to
**		distinguish Gateway suspensions from normal Ingres suspensions.
**	11-Jan-90 (anton)
**	    If a session was waiting on a mutex an idle counter for
**	    a cssuspend might be incremented.  Fixed.
**	    Condition waits will be counted as if semaphore waits.
**	23-dec-93 (walt)
**	    To implement UNIX-style quantum switching CS_quantum will increment
**	    a counter (quantum_ticks) each time it's entered.  The counter
**	    will be zeroed by CS_ssprsm each time it switches threads.  If
**	    CSswitch sees a value greater than 1 it will cause a thread switch.
**	    This scheme is necessary because the previous VMS method which
**	    involved changing an AST's return is impossible on Alpha because
**	    a read/write VAX/VMS control block is read-only on Alpha/VMS.
**	13-jul-95 (dougb)
**	    Change name of quantum_ticks to CS_lastquant -- matching Unix CL.
*/
i4
CS_quantum(mode)
i4                mode;
{
    i4                 status;
    CS_SCB		*scb;
    
    if (mode != 0)
    {
	Cs_lastquant++;
	
	Cs_srv_block.cs_quantums += 1;
	if (Cs_srv_block.cs_state == CS_IDLING)
	{
	    Cs_srv_block.cs_idle_time += 1;

	    for (scb = Cs_srv_block.cs_known_list->cs_next;
		scb != Cs_srv_block.cs_known_list;
		scb = scb->cs_next)
	    {
		/* for each known thread, if it is not recognized,	    */
		/* increment its idle count				    */

		if ((scb->cs_self > 0) && (scb->cs_self < CS_FIRST_ID))
		    continue;
		if ((scb == &Cs_idle_scb)
			|| (scb == (CS_SCB *)&Cs_admin_scb)
			|| (scb == &Cs_repent_scb)
			|| (scb->cs_state != CS_EVENT_WAIT))
		    continue;
		if (scb->cs_state == CS_MUTEX
		 || scb->cs_state == CS_CNDWAIT)
		{
		    Cs_srv_block.cs_wtstatistics.cs_tm_idle++;
		    continue;
		}
		else if (scb->cs_state != CS_EVENT_WAIT)
		{ 
		    continue;
		}
		
		/*
		** Don't add idle statistics for dbms task threads.
		** Since they are expected to be non-runnable most of the time,
		** they tend to skew the server statistics.
		*/
		if (scb->cs_thread_type != 0)
		    continue;

		if (scb->cs_memory & CS_BIO_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_bio_idle++;
		}
		else if (scb->cs_memory & CS_DIO_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_dio_idle++;
		}
		else if (scb->cs_memory & CS_LOCK_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_lk_idle++;
		}
                else if (scb->cs_memory & CS_GWFIO_MASK)
                {
                    Cs_srv_block.cs_wtstatistics.cs_gwfio_idle++;
                }
		else
		{
		    Cs_srv_block.cs_wtstatistics.cs_lg_idle++;
		}

	    }
	}

	Cs_srv_block.cs_toq_cnt--;
	if (Cs_srv_block.cs_inkernel == 0)
	{

	    /* If enougth quanta have passed that we need to check for	    */
	    /* timeouts, then go do that, first.			    */
	    if (Cs_srv_block.cs_toq_cnt <= 0)
		CS_toq_scan();

	    /*
	    **  We must check that there exists a current scb.
	    **  Just after deleting a task, there is a window in which
	    **  there is no task running.  Thus, if there is no task
	    **  running, then just go on, a new one will be rescheduled
	    **  as soon as this AST exists
	    */

	    if (scb = Cs_srv_block.cs_current)
	    {
#ifdef CS_MIDQUANTUM
		if (scb->cs_mask & CS_MID_QUANTUM_MASK)
		{
		    /*
		    ** If this session started mid quantum, turn off that fact
		    ** and let him go on.
		    */
		    scb->cs_mask &= ~CS_MID_QUANTUM_MASK;
		}
		else
#endif
		/*
		** Check if session can be swapped out.  Sessions marked
		** NOSWAP should not be switched due to quantum timer.
		*/
		if ((scb->cs_mask & CS_NOSWAP) == 0)
		{
		    if (Cs_srv_block.cs_state != CS_IDLING)
		    {
			/*
			** Suspend this thread and resume the next ready
			** one.  Don't need to do this if the next thread
			** to be scheduled is the one currently running.
			*/
			if (((Cs_srv_block.cs_mask & CS_NOSLICE_MASK) == 0)
				&&
			     ((Cs_srv_block.cs_rdy_que[scb->cs_priority]->
				cs_rw_q.cs_q_next != scb) ||
			     (Cs_srv_block.cs_rdy_que[scb->cs_priority]->
				cs_rw_q.cs_q_prev != scb) ||
                             (scb->cs_state != CS_COMPUTABLE) ||
			     ((Cs_srv_block.cs_ready_mask &
			       ((CS_PRIORITY_BIT >> scb->cs_priority) - 1))
					 != 0)))
			{
                            if (scb == &Cs_repent_scb)
                                sys$wake(0,0);
                            else
                                CS_ast_ssprsm();
			}
		    }
		    else
		    {
			if (Cs_srv_block.cs_ready_mask & ~CS_PRIORITY_BIT)
			{
			    /* the idle task will wake up and schedule the AST itself */
			    sys$wake(0, 0);
			}
		    }
		}
	    }
	}
#ifdef CS_MIDQUANTUM
	else
	{
	    /*
	    ** mark the fact that we didn't kill the session.
	    ** Then a session will voluntarily suspend itself
	    ** when it notices that it missed suspension.
	    */
	    Cs_srv_block.cs_inkernel = -1;
	}
#endif
    }

    /* Since timers are one shots, request another timer */
    
    status = sys$setimr(EFN$C_ENF, Cs_srv_block.cs_bquantum, CS_quantum, CS_IDLE_ID, 0);
    if (status & 1)
	return(OK);

    Cs_srv_block.cs_state = CS_ERROR;
    Cs_srv_block.cs_error_code = E_CS0006_TIMER_ERROR;
    (*Cs_srv_block.cs_elog)(E_CS0006_TIMER_ERROR, &status, 0);
    return(status);
}

/*
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
**      none
**
** Outputs:
**      none
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-Oct-1986 (fred)
**          Created on Jupiter.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		New CS_GWFIO_MASK indicates that a session is suspended on
**		Gateway I/O. It is informational only, and is used to
**		distinguish Gateway suspensions from normal Ingres suspensions.
**		The RMS Gateway does not currently use CS's timeout facility
**		for thread suspension timeouts; this codes is being added to
**		support future gateways which may wish to make use of this
**		facility (and for completeness).
**	19-feb-1996 (duursma)
**	    Partly integrated UNIX change 423116.
*/
void
CS_toq_scan()
{
    CS_SCB              *scb;
    CS_SCB              *next_scb;

    for (scb = Cs_srv_block.cs_to_list->cs_rw_q.cs_q_next;
	 scb != Cs_srv_block.cs_to_list;
	 scb = next_scb)
    {
	next_scb = scb->cs_rw_q.cs_q_next;
	if (Cs_srv_block.cs_q_per_sec > 0)
	{
	    scb->cs_timeout -= 1;
	}
	else
	{
	    /* value is negative, ergo subtraction */ 
	    scb->cs_timeout += Cs_srv_block.cs_q_per_sec;
	}
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
		if (scb->cs_memory & CS_IOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_bior_time +=
			Cs_srv_block.cs_quantums - scb->cs_ppid;
		else
		    Cs_srv_block.cs_wtstatistics.cs_biow_time +=
			Cs_srv_block.cs_quantums - scb->cs_ppid;
	    }
	    else if (scb->cs_memory & CS_DIO_MASK)
	    {
		if (scb->cs_memory & CS_IOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_dior_time +=
			Cs_srv_block.cs_quantums - scb->cs_ppid;
		else
		    Cs_srv_block.cs_wtstatistics.cs_diow_time +=
			Cs_srv_block.cs_quantums - scb->cs_ppid;
	    }
            else if (scb->cs_memory & CS_GWFIO_MASK)
            {
                Cs_srv_block.cs_wtstatistics.cs_gwfio_time +=
                    Cs_srv_block.cs_quantums - scb->cs_ppid;
            }
	    else
	    {
		if (scb->cs_thread_type == CS_USER_THREAD)
		{
		    if (scb->cs_memory & CS_LOCK_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lk_time +=
			    Cs_srv_block.cs_quantums - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_LKEVENT_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lke_time +=
			    Cs_srv_block.cs_quantums - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_LGEVENT_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lge_time +=
			    Cs_srv_block.cs_quantums - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_LOG_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_lg_time +=
			    Cs_srv_block.cs_quantums - scb->cs_ppid;
		    }
		    else if (scb->cs_memory & CS_TIMEOUT_MASK)
		    {
			Cs_srv_block.cs_wtstatistics.cs_tm_time +=
			    Cs_srv_block.cs_quantums - scb->cs_ppid;
		    }
		}
	    }
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_mask |= CS_TO_MASK;
	    /* delete from timeout list */
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		scb->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		scb->cs_rw_q.cs_q_next;

	    /* and put on tail of ready que by priority */
	    scb->cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[scb->cs_priority];
	    scb->cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	    Cs_srv_block.cs_ready_mask
		|= (CS_PRIORITY_BIT >> scb->cs_priority);

            if (scb->cs_thread_type != CS_INTRNL_THREAD &&
              (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
	}
    }
    Cs_srv_block.cs_toq_cnt = Cs_srv_block.cs_q_per_sec;

    /*
    ** Check status of server governor.  If the governor
    ** is enabled, the determine the ratio of cpu to
    ** wall-clock time since the last quantum.  If it
    ** is not acceptable (i.e. above the governed limit,
    ** then
    **	1) set the quantum time to the adjusted new time
    **	    which makes the governed percentage correct,
    **	2) promote the idle thread to a high priority
    **	    so the server will stall until the quantum
    **	    goes off again.
    */

    if (Cs_srv_block.cs_gpercent > 0)
    {
	static i4	hundred_k = 100000;
	GENERIC_64  now;
	u_i8	    elapsed_time;
	i4	    et_cpu_units;
	i4	    remainder;
	
	i4	    cpu_used;
	i4	    status;

	status = sys$gettim(&now);
	if (status & 1)
	{
	    status = lib$sub_times(&now, Cs_srv_block.cs_int_time,
				    &elapsed_time);
	    if (status & 1)
	    {
		IOSB iosb;

		Cs_cpu_jpi[0].ile3$ps_bufaddr = &cpu_used;
		status = sys$getjpiw(EFN$C_ENF, 0, 0, Cs_cpu_jpi, &iosb, 0, 0);
		if (status & 1)
		    status = iosb.iosb$w_status;
		if ( status & 1)
		{
		    /*
		    **	At this point, we know the elapsed time that
		    **	the server has been up, and we know the
		    **	total cpu time used by this server.  Unfortui4ely,
		    **	the units (thanx DEC!) are different:
		    **	    elapsed_time is in 100 nano-second units,
		    **	    cpu_used is in 10 millisecond units,
		    **	    the conversion factor being 100,000.
		    **	    (100 nano-seconds = 1/10 microsecond
		    **	    10 millseconds = 10,000 microseconds,
		    **		== 100000 1/10ths microseconds.
		    **
		    **	To avoid overflow problems, we will convert the
		    **	elapsed_time to cpu-time units...
		    */

		    status = lib$ediv(&hundred_k, &elapsed_time,
					&et_cpu_units, &remainder);
		    if (status & 1)
		    {
			if ((((cpu_used - Cs_srv_block.cs_int_cpu)
					    * 100) / (-et_cpu_units)) >
				Cs_srv_block.cs_gpercent)
			{
			    if (Cs_srv_block.cs_gpriority == 0)
			    {
				/* Make the server stall */
				Cs_repent_scb.cs_state
					= CS_COMPUTABLE;
				Cs_srv_block.cs_ready_mask |=
				    (CS_PRIORITY_BIT >>
					Cs_repent_scb.cs_priority);
			    }
			    else
			    {
				if ((Cs_srv_block.cs_mask & CS_REPENTING_MASK)
					    == 0)
				    status = sys$setpri(0, 0,
						Cs_srv_block.cs_gpriority,
						&Cs_srv_block.cs_norm_priority,
						0, 0);
			    }
			    Cs_srv_block.cs_mask |= CS_REPENTING_MASK;
			}
			else
			{
			    /*
			    **	We passed, and can continue working.
			    **	In case we had previously failed, make
			    **	sure the repentance thread is stalled out.
			    **
			    **	Also, since we are OK, and potentially
			    **	doing very little, we need to reset the start
			    **	time so that we have a new base from which to
			    **	calculate.
			    **
			    **	This ensures that if the server runs overnight
			    **	doing nothing, it will not then be able to
			    **	consume the entire CPU the next day.  Which
			    **	isn't what we want.  Instead, we ensure that
			    **	at each timeout queue interval, we used no more
			    **	than the given percentage.
			    */
			    
			    MEcopy((char *)&now, sizeof(now),
				(char *) Cs_srv_block.cs_int_time);
			    Cs_srv_block.cs_int_cpu = cpu_used;
				
			    if (Cs_srv_block.cs_gpriority == 0)
			    {
				Cs_repent_scb.cs_state = CS_UWAIT;
				if ((Cs_repent_scb.cs_rw_q.cs_q_next ==
					Cs_srv_block.cs_rdy_que[
					    Cs_repent_scb.cs_priority]) &&
				    (Cs_repent_scb.cs_rw_q.cs_q_prev ==
					Cs_srv_block.cs_rdy_que[
					    Cs_repent_scb.cs_priority]))
				{
				    Cs_srv_block.cs_ready_mask &=
					~(CS_PRIORITY_BIT >>
					    Cs_repent_scb.cs_priority);
				}
			    }
			    else
			    {
				if (Cs_srv_block.cs_mask & CS_REPENTING_MASK)
				    status = sys$setpri(0, 0,
						Cs_srv_block.cs_norm_priority,
						0, 0, 0);
			    }
			    Cs_srv_block.cs_mask &= ~CS_REPENTING_MASK;
			}
		    }
		    else
		    {
			/*
			** If there were errors, then come out of repent mode,
			** which will let us run -- someone will notice that the
			** server is over its limit and call.  Otherwise, we
			** would just sit and stall.
			*/
			
			if (Cs_srv_block.cs_gpriority == 0)
			{
			    Cs_repent_scb.cs_state = CS_UWAIT;
			    if ((Cs_repent_scb.cs_rw_q.cs_q_next ==
				    Cs_srv_block.cs_rdy_que[
					Cs_repent_scb.cs_priority]) &&
				(Cs_repent_scb.cs_rw_q.cs_q_prev ==
				    Cs_srv_block.cs_rdy_que[
					Cs_repent_scb.cs_priority]))
			    {
				Cs_srv_block.cs_ready_mask &=
				    ~(CS_PRIORITY_BIT >>
					Cs_repent_scb.cs_priority);
			    }
			}
			else
			{
			    if (Cs_srv_block.cs_mask & CS_REPENTING_MASK)
				status = sys$setpri(0, 0,
					    Cs_srv_block.cs_norm_priority,
					    0, 0, 0);
			}
			Cs_srv_block.cs_mask &= ~CS_REPENTING_MASK;

			/* Must be overflow problem -- start again */
			MEcopy((char *)&now, sizeof(now), (char *) Cs_srv_block.cs_int_time);
		    }
		}
	    }
	}
    }
}

/*
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
** Inputs:
**      scb                             Scb to which this stack is to initally
**                                      belong.
**
** Outputs:
**      scb.cs_register[12] (ap)        set to contain the original sp
**      scb.cs_register[13] (fp)        set to contain the original sp
**      scb.cs_register[14] (sp)        set to contain the original sp
**	scb.cs_register[15] (pc)	set to CS_jmp_start
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
**	15-May-1989 (rogerk)
**	    Added pages argument to MEget_pages call.
**	18-nov-1992 (walt)
**		Updated for Alpha.  Get hardware memory page size at runtime, and
**		use all the pages returned by MEget_pages. (The request may have
**		gotten rounded while being granted.)
**	23-jul-1993 (walt)
**	    Make the sys$getsyi a sys$getsyiw.  (Especially because it's using
**	    event flag zero.)
**	07-dec-1995 (dougb)
**	    Use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.
**	24-jan-1996 (dougb)
**	    Add TRdisplay() calls to show what thread we're starting where.
**      22-Aug-2000 (horda03)
**          Obtain pages for Stack from the P1 area.
**          (102291)
*/
STATUS
CS_alloc_stack(
CS_SCB             *scb)
{
    VA_RANGE		mem;		/* memory range requested */
    VA_RANGE		retmem;		/* memory range actually effected */
    i4             status;
    CS_STK_CB		*stk_hdr;
    char		*region[2];
    i4		size;
    i4			pages;
    PTR			addr;
    CL_ERR_DESC		sys_err;

	/* Alpha-style CS_jmp_start address */
    PDSCDEF		*proc_desc = (PDSCDEF *)CS_jmp_start;

    size = Cs_srv_block.cs_stksize;
	
#ifdef KPS_THREADS
    /*
    ** For KPS threading we always allocate a new stack on thread exit,
    ** the thread's stack will always be deallocated.
    ** we needn't fill in any of the stack header stuff
    ** since we won't need any internal knowledge of the stack
    ** other that we can't get from the KPB
    ** We have allocated 8 longwords to the user parameter area although
    ** we currently only use the first longword as a flag if the flag is
    ** TRUE we have called exe$kp_end - otherwise it's FALSE
    */
    {
        /* should assert here that ME_MPAGESIZE == 512 */
        i4 pagelets = (size + ME_MPAGESIZE - 1) / ME_MPAGESIZE
                      + 1 + MMG$GL_PAGE_SIZE / ME_MPAGESIZE;
        i4 flags = KP$M_VEST | KP$M_SAVE_FP;
#ifdef i64_vms
        flags |= KP$M_SET_STACK_LIMITS;
#endif

        status = exe$kp_user_alloc_kpb(
                    (KPB **)&scb->kpb,      /* where to store the KPB address */
                    flags,                  /* flags */
                    32,                     /* parameter area size */
                    kpb_alloc,              /* KPB allocation routine */
                    pagelets * VA$C_PAGELET_SIZE,   /* stack size in bytes */
                    memstk_alloc,           /* memory stack alloc routine */
                    KPB$K_MIN_IO_STACK * 4, /* RSE stack size in bytes */
                    exe$kp_alloc_rse_stack_p2, /* No RSE stack alloc rtn reqd */
                    (void (*)())thread_end);   /* rtn called at thread end */
        if (!(status & STS$M_SUCCESS))
        {
            char buf[ER_MAX_LEN];
            i4 msg_lang = 0;
            i4 msg_len;
            CL_ERR_DESC error;

            ERlangcode(NULL, &msg_lang);
            ERslookup(E_CL25F5_KPB_ALLOC_ERROR, NULL, ER_TIMESTAMP, NULL,
                  buf, sizeof(buf), msg_lang, &msg_len, &error, 0, NULL);
            ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);
            return E_CL25F5_KPB_ALLOC_ERROR;
        }

        /*
        ** get the parameter area pointer
        ** we use the first longword as a backlink to our owner SCB
        ** When exe$kp_end is called (and hence our user thread_end function)
        ** we'll need the SCB address to cache the stack(s)
        */
        *(CS_SCB **)scb->kpb->kpb$ps_prm_ptr = scb;
    }
#else
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
	    return(E_CS000B_NO_FREE_STACKS);
	victim->cs_mode = CS_READ;		/* this is a read who lost  */
						/* the stack -- for	    */
						/* CS_setup()		    */
	stk_hdr = victim->cs_stk_area;
	victim->cs_stk_area = 0;
    }
    else
#endif
    if (stk_hdr == Cs_srv_block.cs_stk_list)
    {
	/* We need to allocate a new stack.  On Alpha, part of doing this 
	** calculation is getting the memory page size for this system.
	*/
	IOSB iosb;

	status = sys$getsyiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if ((status & 1) == 0)
		return(status);
	status = MEget_pages(ME_MZERO_MASK|ME_USE_P1_SPACE,
	    (size+ME_MPAGESIZE-1)/ME_MPAGESIZE + 1 + cpu_pagesize/ME_MPAGESIZE, 
		0, &addr, &pages, &sys_err);
	if (status)
	    return(status);
	/* change guard pages to be writable only in supervisor mode */
	/* we guard only against overflow -- as we use the bottom page */
	
	mem.va_range$ps_start_va = mem.va_range$ps_end_va = addr;
	/*  On Alpha this will protect one hardware memory page (>512 bytes)  */
	status = sys$setprt(&mem, &retmem, PSL$C_USER, PRT$C_URSW, 0);
	if ((status & 1) == 0)
	    return(status);
	/*  Use all the pages that were actually allocated. */
	stk_hdr = (CS_STK_CB *) ((char *) addr + (pages * ME_MPAGESIZE) - 512);
	stk_hdr->cs_begin = addr;
	stk_hdr->cs_end = (char *) stk_hdr + 511;
	stk_hdr->cs_size = pages * ME_MPAGESIZE; 
	/*
	**	On Alpha, the stack must start out octaword aligned.  (Subtract 128
	**	rather than the previously subtracted 4.)
	*/
	stk_hdr->cs_orig_sp = (PTR) ((char *) stk_hdr - 128);

	/* Now that the stack header is filled in, lets add it to the list  */
	/* of known stacks.  It is added to the end of the list as the	    */
	/* newest thread is the least likely to give up its stack...	    */

	stk_hdr->cs_next = Cs_srv_block.cs_stk_list;
	stk_hdr->cs_prev = Cs_srv_block.cs_stk_list->cs_prev;
	stk_hdr->cs_next->cs_prev = stk_hdr;
	stk_hdr->cs_prev->cs_next = stk_hdr;
	Cs_srv_block.cs_stk_count++;
    }
    scb->cs_stk_area = (char *) stk_hdr;
    stk_hdr->cs_used = (CS_SID)scb->cs_self;
    scb->cs_stk_size = size;
 
    scb->cs_registers[CS_ALF_SP] = (i8)stk_hdr->cs_orig_sp;
    scb->cs_registers[CS_ALF_FP] = Cs_cactus_fp;
#endif

    scb->cs_registers[CS_ALF_AI] = 0;
    scb->cs_registers[CS_ALF_PV] = (i8)proc_desc;
    scb->cs_registers[CS_ALF_RA] = proc_desc->pdsc$q_entry;

    /*
    ** Ensure every thread (not just the idle thread -- which was
    ** current when CSdispatch() declared a handler) sees the handler(s)
    ** for the top portion of the stack.
    **
    ** Note:  We'll have problems if any thread attempts to EXdelete()
    ** these handlers before every other thread is dead.  Shouldn't
    ** happen since CS_jmp_start() is always on the stack and will
    ** hiberi4e forever instead of returning to CSdispatch().  In
    ** addition, CSdispatch() kills the process immediately if it ever
    ** regains control.
    */
    scb->cs_exsp = Cs_save_exsp;

# ifdef xDEBUG
    TRdisplay( "Starting SCB %p using SP: %p, FP: %p\n",
	      scb, scb->cs_registers [CS_ALF_SP],
	      scb->cs_registers [CS_ALF_FP] );
    TRdisplay( "\tAI: %x, PV: %x and RA: %x\n",
	      scb->cs_registers [CS_ALF_AI],
	      scb->cs_registers [CS_ALF_PV],
	      scb->cs_registers [CS_ALF_RA] );
# endif /* xDEBUG */

    return(OK);
}

#ifdef KPS_THREADS
static i4
kpb_alloc(i4 *bufsiz, KPB_PPQ kpb)
{
    i4 status = lib$get_vm(bufsiz, (void **)kpb);
    return status;
}

static i4
memstk_alloc(KPB_PQ kpb, i4 stack_pages)
{
    i4 status;
    i4 local_pages;
    i4 alloc_pages;
    u_i4 ret_prot;
    uint64 return_length;
    uint64 ret_len;
    uint64 region_length;
    uint64 create_length;
    VOID_PQ va;
    VOID_PQ ret_va;
    VOID_PQ create_va;
    GENERIC_64 region_id;

    /*
    ** these stack pages are expressed in units of CPU h/w pages
    ** On an Alpha this will typically be 8 Kbytes per page
    */
    alloc_pages = max(stack_pages, SGN$GL_KSTACKPAG);
    
    /* 
    ** allow for 2 guard pages - one at the top and 1 at the bottom of
    ** the stack
    */
    local_pages = alloc_pages + 2;
    region_length = $sys_pages_to_bytes(local_pages);
    
    /* First, create a region in P1 space */
    status = sys$create_region_64(region_length, VA$C_REGION_UCREATE_UOWN,
                                  VA$M_P1_SPACE, &region_id, &ret_va, &ret_len);
    if (!(status & STS$M_SUCCESS))
    {
        char buf[ER_MAX_LEN];
        i4 msg_lang = 0;
        i4 msg_len;
        CL_ERR_DESC error;
        ER_ARGUMENT er_arg;

        er_arg.er_size = sizeof(region_length);
        er_arg.er_value = &region_length;
        ERlangcode(NULL, &msg_lang);
        ERslookup(E_CL25F6_MEM_STK_ALLOC_ERROR, NULL, ER_TIMESTAMP, NULL,
                  buf, sizeof(buf), msg_lang, &msg_len, &error, 1, &er_arg);
        ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);
        return status;
    }
    
    /* the mapped pages will not include the 2 guard pages */
    create_length = $sys_pages_to_bytes(alloc_pages);
    create_va = (CHAR_PQ) ret_va + MMG$GL_PAGE_SIZE;
    status = sys$cretva_64(&region_id, create_va, create_length, PSL$C_USER,
                           VA$M_NO_OVERMAP, &va, &return_length);
    if (status & STS$M_SUCCESS)
    {
        /*
        ** Succesfully created address space so store the stack start
        ** address. Remember that we grow downwards.
        */
        kpb->kpb$q_mem_region_id = region_id.gen64$q_quadword;
        kpb->kpb$is_stack_size = create_length;
        kpb->kpb$pq_stack_base = (CHAR_PQ) va + kpb->kpb$is_stack_size;
    }
    else
    {
        sys$delete_region_64 (&region_id, PSL$C_USER, &ret_va, &ret_len);
        kpb->kpb$q_mem_region_id = 0;
    }

    return status;
}

static i4
thread_end(KPB_PQ kpb, i4 status)
{
    i4 sts;
    i4 bufsiz = kpb->kpb$iw_size;
    u_i4* bufadr = (u_i4 *)kpb;
    i4 asts_enabled = (sys$setast(0) != SS$_WASCLR);

    /* get back our thread's SCB address */
    CS_SCB* scb = *(CS_SCB **)kpb->kpb$ps_prm_ptr;

    /* Deallocate the stack(s). Let the system rtn(s) do it for us. */
#ifdef i64_vms
    sts = exe$kp_dealloc_rse_stack_p2(kpb);
    if (!(sts & STS$M_SUCCESS))
    {
        if (asts_enabled)
            sys$setast(1);
        return sts;
    }
#endif
    sts = exe$kp_dealloc_mem_stack_user(kpb);
    if (!(sts & STS$M_SUCCESS))
    {
        if (asts_enabled)
            sys$setast(1);
        return sts;
    }
 
    /* deallocate the KPB */
    sts = lib$free_vm(&bufsiz, &bufadr); 
    if (asts_enabled)
        sys$setast(1);

    return sts;
}
#endif


/*
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
**	4-jan-1995 (dougb)
**	    Remove incredibly old commented-out code.
*/
STATUS
CS_deal_stack(stk_hdr)
CS_STK_CB        *stk_hdr;
{
    stk_hdr->cs_used = 0;

    return(OK);
}

/*
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
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
*/
STATUS
CS_setup()
{
    CS_SCB              *scb = Cs_srv_block.cs_current;
    STATUS		(*rtn)();
    STATUS		status;
    EX_CONTEXT		excontext;
    FUNC_EXTERN STATUS	cs_handler();

    /* Catch escaped exceptions */
    
    if (EXdeclare(cs_handler, &excontext))
    {
	scb->cs_mode = CS_TERMINATE;
	if (scb->cs_mask & CS_FATAL_MASK)
	{
	    (*Cs_srv_block.cs_elog)(E_CS00FF_FATAL_ERROR, 0, 0);
	    EXdelete();
	    return(OK);
	}
	else
	{
	    (*Cs_srv_block.cs_elog)(E_CS0014_ESCAPED_EXCPTN, 0, 0);
	}
	scb->cs_mask |= CS_FATAL_MASK;
    }

    /*
    ** Set session control block pointer to the server control block.
    ** This field was not set at thread initialization because the
    ** value may not have been known.
    */
    scb->cs_svcb = Cs_srv_block.cs_svcb;

    if  ((scb != (CS_SCB *)&Cs_admin_scb)
            && (scb != (CS_SCB *)&Cs_idle_scb)
            && (scb != &Cs_repent_scb))
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
		    rtn = Cs_srv_block.cs_process;
		    break;

		default:
		    (*Cs_srv_block.cs_elog)(E_CS000C_INVALID_MODE, 0, 0);
		    scb->cs_mode = CS_TERMINATE;
		    continue;
	    }

	    
	    if (scb->cs_mode == CS_INPUT)
	    {
		status = (*Cs_srv_block.cs_read)(scb, 0);
		if (status == OK)
		    status = CSsuspend(CS_INTERRUPT_MASK | CS_BIO_MASK, 0, 0);
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

		scb->cs_mask &= ~(CS_EDONE_MASK | CS_IRCV_MASK
					| CS_TO_MASK | CS_ABORTED_MASK
					| CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);
		scb->cs_memory = 0;

		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
	    }		

	    status =
		(*rtn)(scb->cs_mode,
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
		    status = (*Cs_srv_block.cs_write)(scb, 0);
		    if (status == OK)
			status = CSsuspend(CS_INTERRUPT_MASK | CS_BIO_MASK, 0, 0);
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
			(*Cs_srv_block.cs_elog)(E_CS000D_INVALID_RTN_MODE, 0, 0);
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
	    (*Cs_srv_block.cs_elog)(E_CS001C_TERM_W_SEM, 0, 0);
	}
	EXdelete();
	return(OK);
    }
    else if (scb == (CS_SCB *)&Cs_admin_scb)
    {
	CS_admin_task(CS_OUTPUT, scb, &scb->cs_nmode, 0);
    }
    else    /* job is the idle job */
    {
	for (;;)
	{
            if (scb != &Cs_repent_scb)
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
	    }
            else
            {
                Cs_srv_block.cs_state = CS_REPENTING;
            }
	    sys$setast(1);	    /* make sure ast's are on */
	    sys$hiber();
            if ((Cs_srv_block.cs_state == CS_IDLING)
                || (Cs_srv_block.cs_state == CS_REPENTING))
	    {
		Cs_srv_block.cs_state = CS_PROCESSING;
#ifdef KPS_THREADS
                exe$kp_stall_general(scb->kpb);
#else
		CS_ssprsm( FALSE );
#endif
	    }
	    if ((Cs_srv_block.cs_state != CS_IDLING)
                    && (Cs_srv_block.cs_state != CS_PROCESSING)
                    && (Cs_srv_block.cs_state != CS_REPENTING))
		break;
	}
	Cs_srv_block.cs_state = CS_CLOSING;

	CSterminate(CS_KILL, 0);

	sys$canexh(&Cs_srv_block.cs_exhblock);
	sys$exit(SS$_NORMAL);
    }
    EXdelete();
    return(OK);
}

/*
** Name: CS_find_scb	- Find the scb given the session id
**
** Description:
**      This session hashes the session id to obtain the session 
**      control block.  At the moment, VMS systems use
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
*/
CS_SCB	*
IICS_find_scb(
CS_SID  sid)
{
    if (sid != CS_ADDER_ID)
	return((CS_SCB *) sid);
    else
	return((CS_SCB *) 0);
}

/*
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
**	27-jul-1995 (dougb)
**	    Check if this is last thread at a priority before updating the
**	    scb's ready/wait queue links.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
*/
void
CS_eradicate()
{
    CS_SCB              *scb = Cs_srv_block.cs_current;

    CSp_semaphore(TRUE, &Cs_admin_scb.csa_sem);
    if (scb->cs_state == CS_COMPUTABLE &&
        scb->cs_thread_type != CS_INTRNL_THREAD)
        Cs_srv_block.cs_num_active--;

    sys$setast(0);	/* make sure we are not interrupted */

    scb->cs_state = CS_FREE;
    /* remove from whatever queue(s) it is on */
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb->cs_rw_q.cs_q_next;

    scb->cs_next->cs_prev = scb->cs_prev;
    scb->cs_prev->cs_next = scb->cs_next;

    /* 
    ** If this was the last thread at its priority, then turn off the ready
    ** bit for that priority.
    */
    if ((scb->cs_rw_q.cs_q_next == Cs_srv_block.cs_rdy_que[scb->cs_priority])
	&&
	(scb->cs_rw_q.cs_q_prev == Cs_srv_block.cs_rdy_que[scb->cs_priority]))
    {
	Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> scb->cs_priority);
    }

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
    sys$setast(1);

#ifdef KPS_THREADS
    exe$kp_end(scb->kpb);
#else
    CS_ssprsm( FALSE );
#endif
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
void
CS_default_output_fcn(PTR arg1, i4 msg_len, char *msg_buffer)
{
    CL_ERR_DESC err_code;
 
    ERsend (ER_ERROR_MSG, msg_buffer, msg_len, &err_code);
}
 
/*
** Name: CS_dump_scb	- Display the contents of an scb
**
** Description:
**      This routine simply displays the scb via TRdisplay().  It is intended 
**      primarily for interactive debugging.
**
** Inputs:
**      sid                             sid of scb in question
**      scb                             scb in question.  Either will do
**                                      If scb is given, it is used.
**
** Outputs:
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Nov-1986 (fred)
**          Created.
**	04-jan-95 (dougb)
**	    Integrate a really old Unix CL change:
**		28-Jan-1991 (anton)
**	    Added CS_CNDWAIT state.
**      09-may-2003 (horda03) Bug 104254
**         removed references to cs_sm_root.
*/
void
CS_dump_scb(sid, scb)
i4                sid;
CS_SCB             *scb;
{

    if (scb == (CS_SCB *)0)
    {
	scb = CS_find_scb(sid);
	if (scb == (CS_SCB *)0)
	{
	    TRdisplay("No SCB found for sid %x%< (%d.)\n", sid);
	    return;
	}
    }
    TRdisplay("\n>>>CS_SCB found at %x<<<\n\n", scb);
    TRdisplay("cs_next: %x\tcs_prev: %x\n", scb->cs_next, scb->cs_prev);
    TRdisplay("cs_length: %d.\tcs_type: %x\n", scb->cs_length, scb->cs_type);
    TRdisplay("cs_self: %x%< (%d.)\n", scb->cs_self);
    TRdisplay("cs_stk_area: %x\tcs_stk_size: %x%< (%d.)\n", scb->cs_stk_area,
	    scb->cs_stk_size);
    TRdisplay("cs_state: %w %<(%x)\tcs_mask: %v %<(%x)\n",
	    cs_state_names,
	    scb->cs_state,
	    cs_mask_names,
	    scb->cs_mask);
    TRdisplay("cs_priority: %d.\tcs_base_priority: %d.\n",
	    scb->cs_priority, scb->cs_base_priority);
    TRdisplay("cs_timeout: %d.\t\n",
		    scb->cs_timeout);
    TRdisplay("cs_sem_count: %d.\n",
			scb->cs_sem_count);
    TRdisplay("\tcs_sm_next: %x\n", scb->cs_sm_next);
    TRdisplay("cs_mode: %w%<(%x)\tcs_nmode: %w%<(%x)\n",
		    "<invalid>,CS_INITATE,CS_INPUT,CS_OUTPUT,CS_TERMINATE,CS_EXCHANGE",
		    scb->cs_mode,
		    "<invalid>,CS_INITATE,CS_INPUT,CS_OUTPUT,CS_TERMINATE,CS_EXCHANGE",
		    scb->cs_nmode);
    TRdisplay("cs_rw_q.cs_q_next: %x\tcs_rw_q.cs_q_prev: %x\n",
		scb->cs_rw_q.cs_q_next, scb->cs_rw_q.cs_q_prev);
}

/*
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
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    A new session is created.
**
** History:
**      24-Nov-1986 (fred)
**          Created.
*/

CS_rcv_request()
{
    STATUS	    status;
    status = (*Cs_srv_block.cs_saddr)(Cs_srv_block.cs_crb_buf, 0);
    if (status)
    {
	(*Cs_srv_block.cs_elog)(E_CS0011_BAD_CNX_STATUS, &status, 0);
	return(FAIL);
    }
    return(OK);
}

/*
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
**	28-Oct-1992 (daveb)
**	    detach it from the known thread tree before deallocating.
*/
CS_del_thread(scb)
CS_SCB		*scb;
{
    i4			status;
    CS_STK_CB		*stk_hdr;
    ILE3		itmlst[] = {
			    { 0, SJC$_ACCOUNTING_MESSAGE, 0, 0},
			    { 0, 0, 0, 0}
			};
    IOSB		iosb;
#ifdef KPS_THREADS
    KPB_PQ kpb = scb->kpb;
#endif

    struct  {
		char	record_type[16];
		char	user_name[12];
		int	uic;
		int	connect_time;
		int	cpu_ticks;
		int	bio_cnt;
		int	dio_cnt;
	    }		    acc_rec;


    /* Decrement session count.  If user thread decrement user session count */
    Cs_srv_block.cs_num_sessions--;
    if (scb->cs_thread_type == CS_USER_THREAD ||
        scb->cs_thread_type == CS_MONITOR)
	Cs_srv_block.cs_user_sessions--;

    if ((Cs_srv_block.cs_mask & CS_ACCNTING_MASK) &&
	(scb->cs_thread_type == CS_USER_THREAD))
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
	status = sys$sndjbcw(EFN$C_ENF, SJC$_WRITE_ACCOUNTING, 0, itmlst, &iosb, 0, 0);
    }

    /* remove from known thread tree */

    CS_detach_scb( scb );

    /* if we're a cleanup thread, let the other system threads die too */
    if ( scb->cs_cs_mask & CS_CLEANUP_MASK )
        CSterminate(CS_CLOSE, NULL);

#ifdef KPS_THREADS
    /*
    ** Stack and KPB deallocation occurs in the thread_end() function
    ** called by exe$kp_end() - either explicitly or implicitly at thread
    ** end.
    */
    status = (*Cs_srv_block.cs_scbdealloc)(scb);
#else
    status = CS_deal_stack((CS_STK_CB *) scb->cs_stk_area);
    status = (*Cs_srv_block.cs_scbdealloc)(scb);
#endif

    return(status);
}

/*
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
**	04-jan-96 (dougb)
**	    Integrate a Nick Ireland Unix change (below).  Also, copy over a
**	    few useful comments from the Unix version of this routine.
**		19-sep-1995 (nick)
**	    Remove magic number 12; set next_mode using the correct
**	    mask values instead.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
*/
STATUS
CS_admin_task(mode, scb, next_mode, io_area)
i4                mode;
CS_ADMIN_SCB	   *scb;
i4                *next_mode;
PTR                io_area;
{
    STATUS              status = OK;
    CS_SCB		*dead_scb;
    CS_SCB		*next_dead;
    CL_ERR_DESC		error;
    i4			uic;
    i4			size;
    i4			task_added;

    if (scb != &Cs_admin_scb)
    {
	(*Cs_srv_block.cs_elog)(E_CS0012_BAD_ADMIN_SCB, 0, 0);
	return(FAIL);
    }
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
		if (((Cs_srv_block.cs_num_sessions + 1) >
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

	    /*
	    ** After unlocking it, clean up.  Note that the check
	    ** below for "task_added" doesn't check status --
	    ** failure to add a thread this time doesn't mean
	    ** we should stop listening for a new one.
	    */
	    if ((scb->csa_mask & CSA_ADD_THREAD) && (task_added))
	    {
		scb->csa_mask &= ~CSA_ADD_THREAD;
		task_added = 0;
		status = CS_rcv_request();
	    }

	    /*
	    ** Note: may get here if completion for ADDER ID
	    ** happens before CS_rcv_request returns!
	    */

	    /*
	    **	AST's must be disabled when we check for having to add
	    **	a new thread.  If not disabled, then the bit could
	    **	be set just after the test.  If there are no current threads,
	    **	then admin task will never be called again...
	    */
	    sys$setast(0);
	    if (scb->csa_mask & CSA_ADD_THREAD)
	    {
		sys$setast(1);
		continue;
	    }

	    /*
	    ** This below is like a CSsuspend, but it's a special
	    ** case waiting for resume of CS_ADDER_ID.
	    */
	    scb->csa_scb.cs_state = CS_UWAIT;
	    Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> CS_PADMIN);
	    sys$setast(1);
#ifdef KPS_THREADS
            exe$kp_stall_general(((CS_SCB *)scb)->kpb);
#else
	    CS_ssprsm( FALSE );
#endif
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
**	so the thread will try to shutdown.
**
** Inputs:
**	exargs		EX arguments (this is an EX handler)
**
** Outputs:
**	Returns:
**	    EXDECLARE
**
** Side Effects:
**	    Logs exception and begins abnormal thread termination
**
** History:
**	04-jan-95 (dougb)
**	    Integrate parts of a few Unix changes (below).  Carry over
**	    a few other changes such as dumping of the EX arguments.  And,
**	    don't call STlength() as often as in the Unix version.
**		16-Apr-91 (anton)
**	    Set buffer length from EX_MAX_SYS_REP as per 6.3 CL SPEC.
**	    Added description.
**		29-Nov-94 (liibi01)
**          Bug 64041, cross integation from 6.4.  Add server name and time
**          stamp to error text produced here.
**      23-sep-2002 (horda03)
**          Fix ACCVIO in INGSTART. If error reported due to insufficient
**          privilege then as this is an Ingstart, no SCB exists.
*/
STATUS
cs_handler(exargs)
EX_ARGS	    *exargs;
{
    /*
    ** liibi01 Cross integration from 6.4 ( Bug 64041 ).
    ** buf needs to be at least EX_MAX_SYS_REP plus GCC_L_PORT
    ** (for the cs_name) plus 24 (for TMstr()) plus some for
    ** making the output look like ule_format() generated.
    */
    char	host_name [7];
    char        buf [EX_MAX_SYS_REP+GCC_L_PORT+50];
    i4		i;
    CL_ERR_DESC	error;
    i4		length;
    SYSTIME     stime;
    CS_SCB      *scb = 0;

    /*
    ** Note: Unix version does not include host name or sid in
    ** the output string.  Unix version also allows the cs_name
    ** output field to vary in length.
    */
    GChostname( host_name, sizeof host_name );
    STprintf( buf, "%6.6s::[%16.16s, %8p]: ",
	     host_name, Cs_srv_block.cs_name, Cs_srv_block.cs_current );
    TMnow( &stime );
    TMstr( &stime, buf + STlength(buf) );
    length = STlength( buf );
    buf [length++] = ' ';
    buf [length] = EOS;

    CSget_scb(&scb);

    if ( !EXsys_report( exargs, buf + length ))
    {
	STprintf( buf + length, "Exception is %x (%d.)",
		 exargs->exarg_num, exargs->exarg_num );
	for ( i = 0; i < exargs->exarg_count; i++ )
	    STprintf( buf + STlength( buf ), ",a[%d]=%x",
		     i, exargs->exarg_array [i] );
    }

    TRdisplay( "cs_handler (thread %p): %t\n",
	      scb ? scb->cs_self : 0, STlength(buf), buf );
    ERsend( ER_ERROR_MSG, buf, STlength( buf ), &error );

    return(EXDECLARE);
}


/*
** Name: CS_exit_handler	- Terminate the server as gracefully as possible
**
** Description:
**      This routine is called to cause the system to exit forcibly.
**	It merely notes that the server is not dying voluntarily or nicely.
**
** Inputs:
**      handler                         bool:  is called as an exit handler
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-jan-1987 (fred)
**          Created.
*/
void
CS_exit_handler(handler)
i4                handler;
{
    i4			status;
    CS_SCB              *scb;

    if (handler)
	(*Cs_srv_block.cs_elog)(E_CS0015_FORCED_EXIT, 0, 0);
    
    sys$canexh(&Cs_srv_block.cs_exhblock);
}

/*
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
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-Apr-1987 (fred)
**          Created.
*/
void
CS_fmt_scb(scb, iosize, area)
CS_SCB             *scb;
i4                iosize;
char               *area;
{
    i4                 length = 0;
    char		*buf = area;
    CS_STK_CB		*stk_hdr;

    if (iosize < 512)
    {
	STprintf(buf, "Insufficient space\r\n");
	return;
    }
    sys$setast(0);
    for (;;)
    {
	STprintf(buf, "---Dump of scb for session %x---\r\n", scb);
	buf += STlength(buf);
	STprintf(buf, "cs_next: %x\tcs_prev: %x\r\n",
	    scb->cs_next, scb->cs_prev);
	buf += STlength(buf);
	STprintf(buf, "cs_length(%d.+): %d.\tcs_type(%x): %x\r\n",
	    sizeof(CS_SCB), scb->cs_length,
	    CS_SCB_TYPE, scb->cs_type);
	buf += STlength(buf);
	STprintf(buf, "cs_tag: %4s\tcs_self: %x\r\n",
	    &scb->cs_tag, scb->cs_self);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
	STprintf(buf, "R0:%x R1:%x R2:%x R3:%x\r\n",
	    scb->cs_registers[0],
	    scb->cs_registers[1],
	    scb->cs_registers[2],
	    scb->cs_registers[3]);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
	STprintf(buf, "R4:%x R5:%x R6:%x R7:%x\r\n",
	    scb->cs_registers[4],
	    scb->cs_registers[5],
	    scb->cs_registers[6],
	    scb->cs_registers[7]);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
	STprintf(buf, "R8:%x R9:%x R10:%x R11:%x\r\n",
	    scb->cs_registers[8],
	    scb->cs_registers[9],
	    scb->cs_registers[10],
	    scb->cs_registers[11]);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
	STprintf(buf, "AI:%x RA:%x PV:%x FP:%x SP:%x\r\n",
	    scb->cs_registers[25],
	    scb->cs_registers[26],
	    scb->cs_registers[27],
	    scb->cs_registers[29],
	    scb->cs_registers[30]);
	buf += STlength(buf);
	if (buf - area >= iosize)
	    break;
	stk_hdr = (CS_STK_CB *) scb->cs_stk_area;
	if (stk_hdr)
	{
	    STprintf(buf, "Stk begin: %x\tstk end: %x\r\n",
		stk_hdr->cs_begin,
		stk_hdr->cs_end);
	    buf += STlength(buf);
	    if (buf - area >= iosize)
		break;
	    STprintf(buf, "Stk size: %d.\tStk orig sp: %x\r\n",
		stk_hdr->cs_size - 1024,
		stk_hdr->cs_orig_sp);
	}
	else
	{
	    STprintf(buf, "No stack currently allocated\r\n");
	}
	buf += STlength(buf);
	break;
    }
    sys$setast(1);
}

STATUS
CS_breakpoint(void)
{
	return(OK);
}

/*
** Name: CS_last_chance	- VMS Last chance exception handler.
**
** Description:
**      This routine is called by VMS after it recognizes that no handler 
**      has accepted the exception.  In the CS environment, this should 
**      only occur when no handler can be called because the thread stack 
**      is trashed or exhausted.  Here we will try to set things up so that 
**      only the thread is killed.
**
**	If the system is in a state where one cannot reliably just kill the
**	thread, the action is marked as fatal, and the entire server will be
**	killed.  If CS_last_chance is entered after having previously found
**	a fatal error, then it must be the case that the server is trashed
**	enough so that the server cannot die a `i4ural' death.  In this case,
**	we call sys$exit() directly, without trying to invoke others.
**
** Inputs:
**      sig_vec				Vector describing the exception
**
** Outputs:
**      None.
**	Returns:
**	    Exception returns (SS$_CONTINUE after unwinding)
**	Exceptions:
**	    none
**
** Side Effects:
**	    Causes the thread in action now to be killed.
**
** Issues:
**	??? 1. The user handler routines for the current thread are not run
**	??? here.  Since they are now maintained in a list hanging off the
**	??? SCB, it should be possible to call each handler with the
**	??? EX_UNWIND status.  Of course, the context structures in the list
**	??? may have been overwritten by the last-chance handler.  Careful...
**
**	??? 2. When we hit the guard page due to excessive recursion, the
**	??? stack is reset to the top of our virtual memory and we start
**	??? re-using the memory for the "mainline" code.  This means that
**	??? Cs_cactus_fp no longer points to a valid call frame (that memory
**	??? has been overwritten).  Currently, leaving everything alone seems
**	??? to work correctly.  An IIMONITOR "stacks" command after recursion
**	??? has killed one thread shows an invalid frame on the top of the
**	??? stack.  But, nothing bad occurs.  While the stack-based handlers
**	??? declared by frontstart.c and CSdispatch() are no longer visisble,
**	??? they are *very* hard to reach once the server is up and running.
**
**	??? 3a. The top of every thread's EX handler stack (scb->cs_exsp)
**	??? now describes a handler which will never be invoked.  That's the
**	??? handler declared by CSdispatch().  (Shouldn't cause problems.)
**
**	??? What is troublesome is the possibility that we're "just lucky" and
**	??? losing a valid Cs_cactus_fp will cause access violations later.
**	??? It may be worthwhile changing Cs_cactus_fp to something valid (say,
**	??? the FP when in CS_last_chance()) and correcting the values stored
**	??? on the stack of every current thread.  That requires the
**	??? equivalent work of the IIMONITOR "stacks" command, followed by a
**	??? lib$put_invo_registers() call for each thread when the frame for
**	??? cs_jmp_start() appears.
**
**	??? 4. It may also be possible (assuming we include chfdef.h) to update
**	??? the signal and mechanism vectors we're passed so that our caller
**	??? "calls" CSssprsm() after we return SS$_CONTINUE.  That would mean
**	??? that the caller might be able to perform additional cleanup we're
**	??? preventing by never returning.  Seems more complicated...
**
** History:
**      19-Nov-1987 (fred)
**          Created on Jupiter.
**      20-jun-91 (ralph)
**          Too few parameters for msg E_CS00FD_THREAD_STK_OVFL in CS_last_chance
**	07-dec-95 (dougb)
**	    Use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.
**	31-dec-95 (whitfield)
**	    Use ANSI fn definitions.
**	24-jan-96 (dougb) bug 71590
**	    Correct use of EXsys_report() -- use EXsigarr_sys_report().
**	    Restore ASTs before exiting CS_last_chance().  Call cs_elog
**	    routine with a pointer to the status -- avoid an ACCVIO.
**	    Change interface to CS_ssprsm() instead of using stub routine
**	    CS_restart().  New parameter to CS_ssprsm() indicates that no regs
**	    should be saved nor should a new thread be chosen -- just restore
**	    given context.  Allows us to keep the server running after a
**	    non-fatal error in one thread.  (Earlier code always failed in an
**	    illegal call to SYS$UNWIND().)
**	    Remove use of incorrect MCH structure definition.
**	29-apr-2007 (joea)
**	    Only copy the SP if the cs_stk_area pointer is not null.
*/
int
CS_last_chance(EX_ARGS	*sig_vec)
{
    static  i4		previous_fatal_last_chance = 0;
    
    CS_SCB		*scb = Cs_srv_block.cs_current;
    i4			fatal = 0;
    i4			status;
    char		buf[256];
    bool		was_set;
    CL_ERR_DESC		cl_stat;

    /* Alpha-style CS_jmp_start address */
    PDSCDEF		*proc_desc = (PDSCDEF *)CS_jmp_start;

    /* to prevent a thread switch in here */
    was_set = ( SS$_WASSET == sys$setast( 0 ));

    EXsigarr_sys_report((i4 *)sig_vec, buf );
    TRdisplay("CS_last_chance: %t\n", STlength(buf), buf);
    ERsend(ER_ERROR_MSG, buf, STlength(buf), &cl_stat);

    if (scb->cs_sem_count)
    {
	(*Cs_srv_block.cs_elog)(E_CS00FC_CRTCL_RESRC_HELD, 0, 0);
	fatal++;
    }
    else if (lib$ast_in_prog())
    {
	(*Cs_srv_block.cs_elog)(E_CS00FB_UNDELIV_AST, 0, 0);
	fatal++;
    }
    else if (scb->cs_mask & CS_BAD_STACK_MASK)
    {
	(*Cs_srv_block.cs_elog)(E_CS00F9_2STK_OVFL, 0, 0);
	fatal++;
    }
    scb->cs_mask |= CS_BAD_STACK_MASK;

    (*Cs_srv_block.cs_elog)(E_CS00FD_THREAD_STK_OVFL, 0, 2,
		sizeof(scb->cs_username), scb->cs_username,
		sizeof(scb->cs_stk_size), &scb->cs_stk_size);

    if (previous_fatal_last_chance)
    {
	if ( was_set )
	    sys$setast( 1 );

	if (previous_fatal_last_chance == 1)
	{
	    previous_fatal_last_chance += 1;
	    sys$canexh(&Cs_srv_block.cs_exhblock);
	    sys$exit(SS$_ABORT);
	}
	else if (previous_fatal_last_chance == 2)
	{
	    previous_fatal_last_chance += 1;
	    sys$exit(SS$_ABORT);
	}
	else
	{
	    sys$delprc(0, 0);
	}
    }

    if (!fatal)
    {
	/* In this case, we can attempt to salvage the server by	    */
	/* simply setting this session up to terminate.  We will reset its  */
	/* stack, and hope that it wasn't holding any critical resources    */
	/* which we don't know about. We also point it back at the original */
	/* jumping off point (CS_jmp_start) which will not really be used.  */
	/* The actual point at which it will start will be CS_ssprsm, as    */
	/* specified below in the sys$unwind() call.  This is done as a	    */
	/* safety measure.						    */

	/*
	** With regards to the last comment:  CS_jmp_start() *will* get
	** called.  The CS_restart() equivalent on Alpha (calling
	** CS_ssprsm() with a TRUE parameter) just restores the context it
	** is given.  That means that CS_ssprsm() is going to "return" into
	** a "call" to CS_jmp_start(), in exactly the same fashion that the
	** thread was originally started.  This is also what happens on
	** Vax/VMS when CS_restart() (an alternate entry to CS_ssprsm()
	** which can't exist on Alpha) is called.
	**
	** Note that neither CS_last_chance() nor CS_ssprsm() will ever
	** really return to their caller.  If the base frame had changed
	** something about the process, we'd be out of luck...
	*/

	scb->cs_state = CS_COMPUTABLE;
	scb->cs_self = (CS_SID)scb;
	scb->cs_mode = CS_TERMINATE;
	scb->cs_mask |= CS_DEAD_MASK;

	/*
	** We set everything up in the SCB as CS_alloc_stack() did it
	** originally.  That is, other than the fact that this thread is
	** about to die...
	*/
	if (scb->cs_stk_area)
	    scb->cs_registers [CS_ALF_SP]
		= (i8)((CS_STK_CB *) scb->cs_stk_area)->cs_orig_sp;
	scb->cs_registers [CS_ALF_FP] = Cs_cactus_fp;

	scb->cs_registers [CS_ALF_AI] = 0;
	scb->cs_registers [CS_ALF_PV] = (i8)proc_desc;
	scb->cs_registers [CS_ALF_RA] = proc_desc->pdsc$q_entry;

	/*
	** Ensure every thread (not just the idle thread -- which was
	** current when CSdispatch() declared a handler) sees the handler(s)
	** for the top portion of the stack.
	**
	** Note:  We'll have problems if any thread attempts to EXdelete()
	** these handlers before every other thread is dead.  Shouldn't
	** happen since CS_jmp_start() is always on the stack and will
	** hibernate forever instead of returning to CSdispatch().  In
	** addition, CSdispatch() kills the process immediately if it ever
	** regains control.
	*/
	scb->cs_exsp = Cs_save_exsp;

# ifdef xDEBUG
	TRdisplay( "Restarting SCB %p using SP: %p, FP: %p\n",
		  scb, scb->cs_registers [CS_ALF_SP],
		  scb->cs_registers [CS_ALF_FP] );
	TRdisplay( "\tAI: %x, PV: %x and RA: %x\n",
		  scb->cs_registers [CS_ALF_AI],
		  scb->cs_registers [CS_ALF_PV],
		  scb->cs_registers [CS_ALF_RA] );
# endif /* xDEBUG */

	/*
	** Just call CS_ssprsm() directly.  We know that we have room on the
	** stack to do this since EXsigarr_sys_report(), TRdisplay(), et cetera
	** didn't access violate.  It certainly is simpler than mucking with
	** our caller's context (or the passed vectors), attempting to have it
	** call CS_ssprsm() with a parameter.  There may be no real difference
	** between us calling CS_ssprsm() directly and convincing our caller to
	** do that for us.
	*/
	if ( was_set )
	    sys$setast( 1 );
#ifdef KPS_THREADS
        exe$kp_end(scb->kpb);
#else
	CS_ssprsm( TRUE );
#endif

	/* NOTREACHED -- CS_ssprsm() should not return. */
	fatal++;
	status = SS$_INSFARG;		/* What status to log? */
	(*Cs_srv_block.cs_elog)(E_CS00FA_UNWIND_FAILURE, &status, 0);
    }
    if (fatal)
    {
	previous_fatal_last_chance++;
	(*Cs_srv_block.cs_elog)(E_CS00FF_FATAL_ERROR, 0, 0);
	
	Cs_srv_block.cs_state = CS_CLOSING;
	CSterminate(CS_KILL, 0);

	sys$canexh(&Cs_srv_block.cs_exhblock);
	if ( was_set )
	    sys$setast( 1 );
	sys$exit(SS$_ABORT);
    }

    if ( was_set )
	sys$setast( 1 );
    return (SS$_CONTINUE);
}

/*{
** Name: CS_vms_dump_stack()	- dump stack trace.
**
** Description:
**	ALPHA/VMS implementation of CS_dump_stack().
**
**      lib$get calls used to get stack info.
**
** Inputs:
**      scb                             session id of stack to dumped.
**      output_fcn                      Function to call to perform the output.
**                                      This routine will be called as
**                                          (*output_fcn)( newline_present,
**							  length,
**							  buffer )
**                                      where buffer is the length character
**                                      output string, and newline_present
**                                      indicates whether a newline needs to
**                                      be added to the end of the string.
**      verbose                         Should we print some information about
**                                      where this diagnostic is coing from?
**                                      For this implementation, verbose also 
**					determines if the saved registers are
**					dumped.  The argument registers
**					are *never* dumped since they aren't 
**					on the stack.
**
** Outputs:
**	none.
**
**	Returns:
**	    void
** History:
**	13-mar-1995 
**		Created by changing VAX/VMS version.
**      05-dec-1995 (whitfield)
**		Adapted VAX CS_stack_dump to Alpha.  Added more things to
**		what gets dumped.  Use ANSI fn definitions.
**	24-jan-1996 (dougb)
**		Correct a few header comments and the call to ERsend() in
**		dump_handler().  Add a TRformat() call in CS_vms_dump_stack()
**		if its handler gets invoked.
*/
static STATUS
dump_handler( EX_ARGS *exargs )
{
    char	buf[256];
    CL_ERR_DESC	error;
    char	*ptr = NULL;
    i4		i = 0;

    if ( !EXsys_report( exargs, buf ))
    {
	STprintf( buf, "Exception is %x (%d.)",
		 exargs->exarg_num, exargs->exarg_num );
    }

# ifdef xDEBUG
    TRdisplay( "dump_handler(): %s\n", buf );
# endif /* xDEBUG */
    ERsend( ER_ERROR_MSG, buf, STlength(buf), &error );

    return( EXDECLARE );
}

static void
CS_vms_dump_stack(CS_SCB  *scb,
		  i4      (*output_fcn)(),
		  i4      verbose)
{

    /* allocate INVO_CONTEXT_BLK which should be quadword aligned */
    INVO_CONTEXT_BLK	stack_context;
    PDSCDEF	*proc_desc;

    char        buf [512];      /* Enough room for verbose output.          */
    char        verbose_string[80];
    char	close_string [4];
    char	open_string [4];
    i4		stack_depth = 100;
    PID		pid;		/* our process id 			    */
    EX_CONTEXT	context;

    if ( EXdeclare( dump_handler, &context ))
    {
	TRformat( output_fcn, 1, buf, sizeof( buf ) - 1,
		 "CS_vms_dump_stack(): Unable to continue." );

	EXdelete();
	return;
    }

    if (scb)
    {
	/*
	** Start stack dump from where-ever the thread was last suspended.
	*/
#if defined(ALPHA)
	MEcopy((char *)scb->cs_registers, sizeof scb->cs_registers, 
	       (PTR)stack_context.libicb$q_ireg);
	MEcopy((char *)scb->cs_flt_registers, sizeof scb->cs_flt_registers, 
	       (PTR)stack_context.libicb$q_freg);
#elif defined(i64_vms)
        MEcopy((char *)scb->cs_registers, sizeof scb->cs_registers,
               (PTR)stack_context.libicb$ih_ireg);
        MEcopy((char *)scb->cs_flt_registers, sizeof scb->cs_flt_registers,
               (PTR)stack_context.libicb$fo_f2_f31);
#else
#error "CS_vms_dump_stack(): Missing code for this platform."
#endif

	/* Be safe and initialize context block header. */
	stack_context.libicb$l_context_length = LIBICB$K_INVO_CONTEXT_BLK_SIZE;
	stack_context.libicb$v_fflags_bits = 0;
	stack_context.libicb$b_block_version = LIBICB$K_INVO_CONTEXT_VERSION;

	/*
	** We know CS_ssprsm() is our current frame.  Use its procedure
	** descriptor and entry point.
	*/
	proc_desc = (PDSCDEF *)CS_ssprsm;
#if defined(ALPHA)
	stack_context.libicb$ph_procedure_descriptor = proc_desc;
	stack_context.libicb$q_program_counter = proc_desc->pdsc$q_entry;
#elif defined(i64_vms)
         stack_context.libicb$ih_pc = proc_desc->pdsc$q_entry;
#endif
    }
    else
    {
#if defined(axm_vms)
	/* Be safe and initialize context block header. */
	stack_context.libicb$l_context_length = LIBICB$K_INVO_CONTEXT_BLK_SIZE;
	stack_context.libicb$v_fflags_bits = 0;
	stack_context.libicb$b_block_version = LIBICB$K_INVO_CONTEXT_VERSION;

	/* Init the register fields as well. */

	MEfill( sizeof stack_context.libicb$q_ireg, '\0',
	       (PTR)stack_context.libicb$q_ireg );
	MEfill( sizeof stack_context.libicb$q_freg, '\0',
	       (PTR)stack_context.libicb$q_freg );
#elif defined(i64_vms)
        MEfill(sizeof(stack_context), 0, &stack_context);
        stack_context.libicb$l_context_length = LIBICB$K_INVO_CONTEXT_BLK_SIZE;
        stack_context.libicb$b_block_version = LIBICB$K_INVO_CONTEXT_VERSION;
#endif

	/*
	** Dumping current thread -- start from this call frame.
	*/
	LIB_GET_CURR_INVO_CONTEXT(&stack_context);
    }
    
    if (verbose)
    {
	PCpid(&pid);

	STprintf(verbose_string, " Stack dmp pid %x session %x:",
		pid, scb ? scb : Cs_srv_block.cs_current);
	STcopy( " ", open_string );
	close_string [0] = EOS;
    }
    else
    {
	verbose_string [0] = EOS;
	STcopy( " (", open_string );
	STcopy( " )", close_string );
    }

    /*
    ** in case stack is corrupt, only print 100 levels so that we don't
    ** loop here forever.
    ** NOTE: we don't dump the current frame (this one or CS_ssprsm()).
    */
    while ((stack_depth-- > 0) && LIB_GET_PREV_INVO_CONTEXT(&stack_context) )
    {
	if ( verbose && 99 == stack_depth )
	    TRformat( output_fcn, 1, buf, sizeof( buf ) - 1,
		     "%s", verbose_string );

	/*
	** Note:  When lining up the output, open_string (the initial '%s')
	** always includes one character (an open paren or space) on the
	** same line as the PC value.
	** Each reg is 8 bytes but we only want first 4 bytes
	*/
#if defined(ALPHA)
	TRformat( output_fcn, 1, buf, sizeof(buf) - 1,
		 "%\
s    pc: %x    pv: %x    fp: %x    sp: %x%s",
		 open_string,
		 stack_context.libicb$q_program_counter,
		 stack_context.libicb$ph_procedure_descriptor,
		 stack_context.libicb$q_ireg[29],
		 stack_context.libicb$q_ireg[30],
		 close_string );
#elif defined(i64_vms)
        TRformat( output_fcn, 1, buf, sizeof(buf) - 1,
                 "%\
s    pc: %x    sp: %x%s",
                 open_string,
                 stack_context.libicb$ih_pc,
                 stack_context.libicb$ih_sp, /* SP */
                 close_string );
#endif

	if ( verbose )
	{
	    /*
	    ** Set things up for later rounds.  In particular, we print the
	    ** verbose_string only once -- so the open paren moves down to the
	    ** PC output line.
	    */
	    if ( 99 == stack_depth )
		STcopy( " (", open_string );

#if defined(ALPHA)
	    TRformat( output_fcn, 1, buf, sizeof(buf) - 1,
		     "\
 r2-r15: %x %x %x %x %x %x %x\n\
         %x %x %x %x %x %x %x\n\
  f2-f9: %x %x %x %x %x %x %x\n\
         %x    icb r_fflags_dsc: %x <%v> ",
		     /* r2-r15 */
		     stack_context.libicb$q_ireg[2],
		     stack_context.libicb$q_ireg[3],
		     stack_context.libicb$q_ireg[4],
		     stack_context.libicb$q_ireg[5],
		     stack_context.libicb$q_ireg[6],
		     stack_context.libicb$q_ireg[7],
		     stack_context.libicb$q_ireg[8],
		     stack_context.libicb$q_ireg[9],
		     stack_context.libicb$q_ireg[10],
		     stack_context.libicb$q_ireg[11],
		     stack_context.libicb$q_ireg[12],
		     stack_context.libicb$q_ireg[13],
		     stack_context.libicb$q_ireg[14],
		     stack_context.libicb$q_ireg[15],
		     /* f2-f9 */
		     stack_context.libicb$q_freg[2],
		     stack_context.libicb$q_freg[3],
		     stack_context.libicb$q_freg[4],
		     stack_context.libicb$q_freg[5],
		     stack_context.libicb$q_freg[6],
		     stack_context.libicb$q_freg[7],
		     stack_context.libicb$q_freg[8],
		     stack_context.libicb$q_freg[9],
		     /* Flags */
		     stack_context.libicb$v_fflags_bits,
		     "EXCEPT,AST,BOTTOM,BASE",
		     stack_context.libicb$v_fflags_bits );
#elif defined(__ia64)
            TRformat( output_fcn, 1, buf, sizeof(buf) - 1,
                     "\
 r2-r15: %x %x %x %x %x %x %x\n\
         %x %x %x %x %x %x %x\n\
  f0-f9: %x %x %x %x %x %x %x\n\
         %x %x %x   icb r_fflags_dsc: %x <%v> ",
                     /* r2-r15 */
                     stack_context.libicb$ih_ireg[2],
                     stack_context.libicb$ih_ireg[3],
                     stack_context.libicb$ih_ireg[4],
                     stack_context.libicb$ih_ireg[5],
                     stack_context.libicb$ih_ireg[6],
                     stack_context.libicb$ih_ireg[7],
                     stack_context.libicb$ih_ireg[8],
                     stack_context.libicb$ih_ireg[9],
                     stack_context.libicb$ih_ireg[10],
                     stack_context.libicb$ih_ireg[11],
                     stack_context.libicb$ih_ireg[12],
                     stack_context.libicb$ih_ireg[13],
                     stack_context.libicb$ih_ireg[14],
                     stack_context.libicb$ih_ireg[15],
                     /* f2-f9 */
                     stack_context.libicb$fo_f2_f31[0][0],
                     stack_context.libicb$fo_f2_f31[1][0],
                     stack_context.libicb$fo_f2_f31[2][0],
                     stack_context.libicb$fo_f2_f31[3][0],
                     stack_context.libicb$fo_f2_f31[4][0],
                     stack_context.libicb$fo_f2_f31[5][0],
                     stack_context.libicb$fo_f2_f31[6][0],
                     stack_context.libicb$fo_f2_f31[7][0],
                     stack_context.libicb$fo_f2_f31[8][0],
                     stack_context.libicb$fo_f2_f31[9][0],
                     /* Flags */
                     stack_context.libicb$v_fflags_bits,
                     "BOTTOM,HANDLER,PROLOGUE,EPILOGUE,MEM_STK_FRAME,REG_STK_FRAME",
                     stack_context.libicb$v_fflags_bits );
#endif
	}
    }

    EXdelete();
#endif
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
**	EXsys_report() and it is called by the iimonitor "debug" command.
**
**	If no implementation of this routine exists for a given platform, 
**	calling this routine will silently result in no work done (ie. the
**	function will simply return doing nothing).
**
** Inputs:
**	scb				session id of stack to dumped.  If the
**					value input is NULL then the current
**					stack is assumed.
**	output_fcn			Function to call to perform the output.
**                                      This routine will be called as
**                                          (*output_fcn)( newline_present,
**							  length,
**							  buffer )
**                                      where buffer is the length character
**                                      output string, and newline_present
**                                      indicates whether a newline needs to
**                                      be added to the end of the string.
**      verbose                         Should we print some information about
**                                      where this diagnostic is coing from?
**
** Outputs:
**	none.
**
**	Returns:
**	    void
**
** History:
**      13-dec-1993 (andys)
**          Created from the UNIX version.
**      05-dec-1995 (whitfield)
**          Adapted VAX CS_stack_dump to Alpha.  Use ANSI fn definitions.
**	24-jan-1996 (dougb)
**	    Correct routine parameters -- stack_p isn't used on Alpha.
**      22-Oct-2002 (hanal04) Bug 108986
**          Use CS_default_output_fcn() as a default output
**          function if one is not supplied by the caller.
*/
void
CS_dump_stack( CS_SCB    *scb,
	      i4	 (*output_fcn)(),
	      i4	 verbose )
{
    if(output_fcn == NULL)
    {
        output_fcn = CS_default_output_fcn;
    }
    CS_vms_dump_stack(scb, output_fcn, verbose);
}

/*{
** Name: CSdump_stack() - dump out stack trace.
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
**	    void
**
** History:
**	25-Apr-2003 (jenjo02) SIR 111028
**	    Invented
*/
void
CSdump_stack( void )
{
    /* Dump this thread's stack, non-verbose */
    CS_dump_stack(0, 0, 0);
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
**	08-dec-2008 (joea)
**	    Ported from Unix version.
*/
void
CS_update_cpu(
i4	*pageflts,
i4	*cpumeasure)
{
    static i4		err_recorded = 0;
    i4		new_cpu;
    i4		old_cpu = Cs_srv_block.cs_cpu;
    struct tms	tms;
    CL_ERR_DESC	err;
     
    /* check the errno rather than the return value of times.  In sys V
    ** world the function can succeed and return a negative value
    */
    if (times(&tms) && errno)
    {
	if (!err_recorded)
	{
	    SETCLERR(&err, 0, 0); 
	    err_recorded = 1;
	    (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &err, 0);
	}
	return;
    }
    /* we are measuring cpu in 1/100 seconds */
    /* VMS times() reports in 10-millisecond units */
    new_cpu = (tms.tms_utime + tms.tms_stime);

    /* No information available about page faults */
    if (pageflts)
	*pageflts = 0;

    /* Update the current thread's CPU time count */
    if (cpumeasure)
	*cpumeasure += new_cpu - old_cpu;

    /* Update server process cpu with current value */
    Cs_srv_block.cs_cpu = new_cpu;
}
/*
** Name: CS_move_async	- Put values set by asynchronous routines into data
**			  structs looked at by synchronous routines.
**
** Description:
**	This routine is called by non-AST driven CS routines to complete the
**	actions taken by AST driven calls.
**
**	Since an AST routine may be fired off at virtually any time during the
**	server's operation, some precautions must be made to assure that 
**	AST routines don't stomp on information that is currently being used
**	by some non-AST routine.
**
**	Whenever a CS routine needs to set/use any server block information that
**	is also used by AST routines, it sets the cs_inkernel flag of the
**	server control block (and of the scb if it is using that as well).
**	This will prevent any AST routine from playing with any of that
**	information.  This will also prevent the server from switching 
**	threads if the quantum timer goes off.  (Disabling AST's is
**	also sufficient to insure safety and some routines that are not
**	performance critical may do this instead of using cs_inkernel.)
**
**	If an AST routine is activated while cs_inkernel is set, then any
**	information that needs to be set by that routine is stored in special
**	asnyc fields of the scb or server control block.  These fields are
**	used only by this routine (which runs with AST disabled) and AST
**	driven routines (which are atomic).  The AST routine also sets the
**	cs_async flag which specifies that there is async information to be
**	put into the server control block or an scb.
**
**	When a CS routine that has set cs_inkernel is finished, it resets
**	cs_inkernel and checks the cs_async flag.  If set, then this routine
**	is called to move the async information into the server control block
**	and/or the scb that was busy.  This routine must disable ASTs while
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
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.  Handle a NULL scb
**	    parameter (in case we are called before threading is enabled or
**	    in a single-threaded program?).
**      29-Nov-2000 (horda03)
**          Only increment the Count of Active Sessions when the session
**          being "moved" to the ready queue is not CS_COMPUTABLE.
**          Bug 90741.
**	27-apr-2007 (jonj/joea)
**	    Re-enable ASTs only if they were enabled on entry.
*/
void
CS_move_async(
CS_SCB	    *scb)
{
    CS_SCB	*nscb;
    i4		new_ready = 0;
    i4		reenable_asts;

    reenable_asts = (sys$setast(0) != SS$_WASCLR);
    Cs_srv_block.cs_inkernel = 1;
    if (scb)
	scb->cs_inkernel = 1;

    if (Cs_srv_block.cs_async)
    {
	/*
	** Move any sessions that became ready while inkernel off of the
	** wait queue and onto the ready queue.
	*/
	while ((nscb = Cs_srv_block.cs_as_list->cs_as_q.cs_q_next) !=
		Cs_srv_block.cs_as_list)
	{
	    /* Take scb off of async list */
	    nscb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev =
		    nscb->cs_as_q.cs_q_prev;
	    nscb->cs_as_q.cs_q_prev->cs_as_q.cs_q_next =
		    nscb->cs_as_q.cs_q_next;

	    nscb->cs_as_q.cs_q_next = 0;
	    nscb->cs_as_q.cs_q_prev = 0;

	    /* Take off of whatever queue (wait or timeout) it is on now */
	    nscb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		nscb->cs_rw_q.cs_q_prev;
	    nscb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		nscb->cs_rw_q.cs_q_next;

	    /* now link into the ready queue */
	    nscb->cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[nscb->cs_priority];
	    nscb->cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[nscb->cs_priority]->cs_rw_q.cs_q_prev;
	    nscb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = nscb;
	    nscb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = nscb;
	    Cs_srv_block.cs_ready_mask |= 
		    (CS_PRIORITY_BIT >> nscb->cs_priority);

            if (nscb->cs_thread_type != CS_INTRNL_THREAD &&
              (nscb->cs_state != CS_COMPUTABLE) &&
              (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;

#ifdef	CS_STACK_SHARING
	    if (nscb->cs_stk_area)
	    {
#endif
		nscb->cs_state = CS_COMPUTABLE;
#ifdef	CS_STACK_SHARING
	    }
	    else
	    {
		nscb->cs_state = CS_STACK_WAIT;
		/*
		** XXXX Why is this not done in the computable state above?
		** It is always turned off in the CSattn case.
		*/
		nscb->cs_mask &= ~(CS_INTERRUPT_MASK);	/* turn off	    */
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
	    sys$wake(0, 0);

	/*
	** Move any information stored in the scb.  cs_asmask hold bits that
	** should be turned on in cs_mask and cs_asunmask holds bits that
	** should be turned off.
	*/
	if ( NULL != scb && scb->cs_async )
	{
	    scb->cs_mask |= scb->cs_asmask;
	    scb->cs_mask &= ~(scb->cs_asunmask);

	    scb->cs_asmask = 0;
	    scb->cs_asunmask = 0;
	    scb->cs_async = 0;
	    Cs_srv_block.cs_scb_async_cnt--;
	}

	/*
	** If this is the last scb with cs_async turned on then reset the
	** cs_async flag in the server control block.
	*/
	if (Cs_srv_block.cs_scb_async_cnt <= 0)
	{
	    Cs_srv_block.cs_async = 0;
	    Cs_srv_block.cs_scb_async_cnt = 0;
	}
    }

    if (scb)
	scb->cs_inkernel = 0;
    Cs_srv_block.cs_inkernel = 0;
    if (reenable_asts)
	sys$setast(1);
}

/*
** Name: CS_change_priority - change threads priority.
**
** Description:
**	Bump the specified session's priority to the new value.  The
**	session must currently be on the server's ready queue.
**
**	This routine takes the thread off of the ready queue of its
**	current priority, changes the threads priority value and puts
**	it on its new ready queue.
**
**	Since this routine changes the priority queues, the caller is
**	expected to have set the cs_inkernel field of the server control
**	block to prevent an asynchronous event from coming in and trying
**	to traverse the queues while we are changing them.
**
**	Note:  This routine should probably bump the threads 'base_priority'
**	value if that value becomes meaningful.  Currently, the 'base_priority'
**	is used to hold information not related to priority values.
**
** Inputs:
**	scb		- session to change.
**	new_priority	- priority to change to.
**
** Outputs:
**      None.
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-feb-1989 (rogerk)
**          Created for Terminator.
**	10-apr-1989 (rogerk)
**	    Took out setting of inkernel while changing thread's priority.
**	    Assume that caller is protecting from asynchronous events.
**	27-jul-1995 (dougb)
**	    Since CS_ssprsm() will ignore priorities without the ready bit
**	    set and will clear the bit for priorities where nothing is
**	    runable, make sure the ready mask is consistent after this call.
**	31-jul-95 (dougb)
**	    Integrate some of Jon Jenson's Unix CL change 419331:
**		9-Jun-1995 (jenjo02)
**	    Only set the ready mask bit for runable threads.
*/
void
CS_change_priority(scb, new_priority)
CS_SCB	    *scb;
i4	    new_priority;
{
    /* Take off current priority queue */
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =	scb->cs_rw_q.cs_q_next;

    /*
    ** If this was the last thread at its priority, then turn off the ready
    ** bit for that priority.
    */
    if ((scb->cs_rw_q.cs_q_next == Cs_srv_block.cs_rdy_que[scb->cs_priority])
	&&
	(scb->cs_rw_q.cs_q_prev == Cs_srv_block.cs_rdy_que[scb->cs_priority]))
    {
	Cs_srv_block.cs_ready_mask &= ~(CS_PRIORITY_BIT >> scb->cs_priority);
    }

    /* Set new priority */
    scb->cs_priority = new_priority;

    /* Put on new priority queue */
    scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_rdy_que[new_priority];
    scb->cs_rw_q.cs_q_prev =
	Cs_srv_block.cs_rdy_que[new_priority]->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;

    /* If thread is computable, set the ready mask */
    if ( CS_COMPUTABLE == scb->cs_state )
    {
	Cs_srv_block.cs_ready_mask |= ( CS_PRIORITY_BIT >> new_priority );
    }
}

/*{
** Name: CS_DIAG_dump_stack().
**
** Description:
**
**      Routine to dump the stack of each thread in the server
**      to an output file.  The dump is made to a binary file
**      which can be used by iiexcept to produce a symbolic
**      stack trace.    
**
** Inputs:
**      OutFile         Name of evidence file in evidence set,
**                      to which the dump is to be written.
**
** Outputs:
**      none.
**
**      Returns:
**          void
** History:
**      4-Sep-1995 (mckba01)
**              Initial version, created for VMS port of
**              ICL Diagnostics.
**      9-Jul-1995 (mckba01)
**              Changed value for maximium stack depth.
**              The previous value was causing truncated
**              stack traces.
*/
void
CS_DIAG_dump_stack(OutFile)
FILE *OutFile;
{
        char            buf[512];
        char            verbose_string[80];
        u_i4       *sp, *getsp();
        i4             stack_depth = 500;
        i4             reg_saved ;     /* number of registers saved in stack frame */
        u_i4       save_mask;      /* VMS mask of reg's saved in stack frame   */
        CS_SCB          *cur_scb = Cs_srv_block.cs_current;
        CS_SCB          *scan_scb;
        u_i4       stack_start;    /* highest poss stack address               */
        u_i4       stack_end;      /* lowest poss stack address                */
        PID             pid;            /* our process id                           */
        EV_STACK_ENTRY  StackRec;
        i4             count;

        /* allocate INVO_CONTEXT_BLK which should be quadword aligned */
        
        INVO_CONTEXT_BLK    stack_context;
	PDSCDEF		*proc_desc;                           
        EX_CONTEXT      context;


        if ( EXdeclare( dump_handler, &context ))
        {
            EXdelete();
            return;
        }
 
        for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
        scan_scb && scan_scb != Cs_srv_block.cs_known_list;
        scan_scb = scan_scb->cs_next)
        {
            switch(scan_scb->cs_state)
            {                           
            case CS_FREE:
                STcopy("CS_FREE",buf);
                break;
            case CS_COMPUTABLE:
                STcopy("CS_COMPUTABLE",buf);
                break;
            case CS_EVENT_WAIT:
                if (scan_scb->cs_memory & CS_DIO_MASK)
                    STprintf(buf,"CS_EVENT_WAIT (DIO)");
                else if(scan_scb->cs_memory & CS_BIO_MASK)
                    STprintf(buf,"CS_EVENT_WAIT (BIO)");
                else if(scan_scb->cs_memory & CS_LOCK_MASK)
                    STprintf(buf,"CS_EVENT_WAIT (LOCK)");
                else
                    STprintf(buf,"CS_EVENT_WAIT (LOG)");
                break;
            case CS_MUTEX:
                STprintf(buf,"CS_MUTEX (%x)",scan_scb->cs_sync_obj); 
                break;
            case CS_CNDWAIT:
                STprintf(buf,"CS_CNDWAIT (%x)",scan_scb->cs_sync_obj); 
                break;
            case CS_STACK_WAIT:
                STcopy("CS_STACK_WAIT",buf);
                break;
            case CS_UWAIT:
                STcopy("CS_UWAIT",buf);
                break;
            default:
                STcopy("Unknown State",buf);
                break;           
            }


            if (cur_scb != scan_scb)
            {
                /*
                ** Start stack dump from where-ever the thread was last suspended.
                */

#if defined(ALPHA)
                MEcopy((char *)scan_scb->cs_registers, sizeof scan_scb->cs_registers, 
                    (PTR)stack_context.libicb$q_ireg);
                MEcopy((char *)scan_scb->cs_flt_registers, sizeof scan_scb->cs_flt_registers, 
                    (PTR)stack_context.libicb$q_freg);
#elif defined(i64_vms)
                MEcopy((char *)scan_scb->cs_registers, sizeof scan_scb->cs_registers,
                    (PTR)stack_context.libicb$ih_ireg);
                MEcopy((char *)scan_scb->cs_flt_registers, sizeof scan_scb->cs_flt_registers,
                    (PTR)stack_context.libicb$fo_f2_f31);
#endif

                /* Be safe and initialize context block header. */
                stack_context.libicb$l_context_length = LIBICB$K_INVO_CONTEXT_BLK_SIZE;
                stack_context.libicb$v_fflags_bits = 0;
                stack_context.libicb$b_block_version = LIBICB$K_INVO_CONTEXT_VERSION;

                /*
                ** We know CS_ssprsm() is our current frame.  Use its procedure
                ** descriptor and entry point.
                */

                proc_desc = (PDSCDEF *)CS_ssprsm;
#if defined(ALPHA)
                stack_context.libicb$ph_procedure_descriptor = proc_desc;
                stack_context.libicb$q_program_counter = proc_desc->pdsc$q_entry;
#elif defined(i64_vms)
                stack_context.libicb$ih_pc = proc_desc->pdsc$q_entry;
#endif
            }
            else
            {
                /* Be safe and initialize context block header. */
                stack_context.libicb$l_context_length = LIBICB$K_INVO_CONTEXT_BLK_SIZE;
                stack_context.libicb$v_fflags_bits = 0;
                stack_context.libicb$b_block_version = LIBICB$K_INVO_CONTEXT_VERSION;

                /* Init the register fields as well. */
#if defined(ALPHA)
                MEfill( sizeof stack_context.libicb$q_ireg, '\0',
                    (PTR)stack_context.libicb$q_ireg );
                MEfill( sizeof stack_context.libicb$q_freg, '\0',
                    (PTR)stack_context.libicb$q_freg );
#elif defined(i64_vms)
                MEfill( sizeof stack_context.libicb$ih_ireg, '\0',
                    (PTR)stack_context.libicb$ih_ireg );
                MEfill( sizeof stack_context.libicb$fo_f2_f31, '\0',
                    (PTR)stack_context.libicb$fo_f2_f31);
#endif

                /*
                ** Dumping current thread -- start from this call frame.
                */

                LIB_GET_CURR_INVO_CONTEXT(&stack_context);
            }

            STprintf(StackRec.Vstring,"\n\nSession %08x  (%-24s) cs_state: %s\n",scan_scb,scan_scb->cs_username,buf);
            stack_end = (u_i4) ((CS_STK_CB *)scan_scb->cs_stk_area)->cs_end;
            stack_start = (u_i4) ((CS_STK_CB *)scan_scb->cs_stk_area)->cs_begin;       
      
            while ((stack_depth-- > 0) && LIB_GET_PREV_INVO_CONTEXT(&stack_context) )
            {
#if defined(ALPHA)
                StackRec.sp = stack_context.libicb$q_ireg[30];
                StackRec.pc = stack_context.libicb$q_program_counter;
                StackRec.args[0] = stack_context.libicb$q_ireg[16]; 
                StackRec.args[1] = stack_context.libicb$q_ireg[17]; 
                StackRec.args[2] = stack_context.libicb$q_ireg[18]; 
                StackRec.args[3] = stack_context.libicb$q_ireg[19]; 
                StackRec.args[4] = stack_context.libicb$q_ireg[20]; 
                StackRec.args[5] = stack_context.libicb$q_ireg[21];
#elif defined(i64_vms)
                StackRec.sp = stack_context.libicb$ih_sp;
                StackRec.pc = stack_context.libicb$ih_pc;
                StackRec.args[0] = stack_context.libicb$ih_ireg[16];
                StackRec.args[1] = stack_context.libicb$ih_ireg[17];
                StackRec.args[2] = stack_context.libicb$ih_ireg[18];
                StackRec.args[3] = stack_context.libicb$ih_ireg[19];
                StackRec.args[4] = stack_context.libicb$ih_ireg[20];
                StackRec.args[5] = stack_context.libicb$ih_ireg[21];
#endif

                SIwrite((i4)sizeof(StackRec), (char *)&StackRec, &count,
			OutFile);
            }
        }
        EXdelete();
}
 



/*{
** Name: CS_DIAG_dump_query().
**
** Description:
**
**      Routine to dump the current query of each thread in the server
**      to an output file.  The dump is made to a an output
**      file in an evidence set.
**
** Inputs:
**      OutFile         Name of evidence file in evidence set,
**                      to which the dump is to be written.
**
** Outputs:
**      none.
**
**      Returns:
**          void
** History:
**      12-Sep-1995 (mckba01)
**              Initial version, created for VMS port of
**              ICL Diagnostics.
*/
void
CS_DIAG_dump_query(OutFile)
FILE    *OutFile;
{
        char            buf[1000];
        CS_SCB          *scan_scb;

        for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
        scan_scb && scan_scb != Cs_srv_block.cs_known_list;
        scan_scb = scan_scb->cs_next)
        {
                /*      
                ** Use field within the CS_SCB "cs_diag_qry" to
                ** access the current query
                */
                STprintf(buf,"Session %08x  : %s\n",scan_scb,scan_scb->cs_diag_qry);
                SIputrec(buf,OutFile);
        }
}

/*
** Name: CS_cp_sem_cleanup()
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**      system.  This is necessary for operating systems that retain
**      a portion of the underlying synchronization object (mutex or
**      semaphore) in the kernel that is not automatically released
**      by releasing shared memory or exiting all processes using the
**      object.
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
**  11-mar-1998 (kinte01)
**      dummy routine on VMS
*/
STATUS
CS_cp_sem_cleanup(
    char            *key,
    CL_ERR_DESC     *err_code)
{
    CL_CLEAR_ERR(err_code);
return( OK );
}


/*
** Name: CS_remove_all_sems - delete known semaphores at shutdown
**
** Description:
**      Walks the known semaphore list and removes semaphores from the
**      system.  This is necessary for operating systems that retain
**      a portion of the underlying synchronization object (mutex or
**      semaphore) in the kernel that is not automatically released
**      by releasing shared memory or exiting all processes using the
**      object.
**
** Inputs:
**      type            CS_SEM_MULTI - to release cross-process sems
**                      CS_SEM_SINGLE - to release private sems
**
** Outputs:
**      none.
**
** Returns:
**      OK              - operation succeeded
**      other system-specific return
**
** History:
**      08-jul-1997 (canor01)
**          Created.
*/
STATUS
CSremove_all_sems( i4 type )
{
return( OK );
}

#ifdef OS_THREADS_USED
/*{
** Name: CS_compress    - Produce "pseudo-floating point" number
**
** Description:
**      This routine converts a time_t type into a pseudo-floating point
**      number, with a 3-bit exponent, and a 13-bit mantissa.
**
** Inputs:
**      val                             number to convert
**
** Outputs:
**      Returns:
**          comp_t                      converted number
**      Exceptions:
**
**
** Side Effects:
**          none
**
** History:
**      14-feb-1991 (ralph)
**          Written.
**      06-jun-1991 (ralph)
**          Changed to return type int
[@history_template@]...
*/
typedef u_i2 comp_t;

comp_t
CS_compress(val)
register time_t val;
{
    register exp = 0, rnd = 0;

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
    return((comp_t)(exp<<13) + val);
}


#ifdef  CS_PROFILING

#define ARCDENSITY      5       /* density of routines */
#define MINARCS         50      /* minimum number of counters */
#define HISTFRACTION    2       /* fraction of text space for histograms */


struct phdr {
        i4 *lpc;
        i4 *hpc;
        i4 ncnt;
};

struct cnt {
        i4 *pc;
        long ncall;
} *countbase;

CSprofend()
{
    chdir("/tmp");
    monitor(0);
}

CSprofstart()
{
    i4 range, cntsiz, monsize; 
    SIZE_TYPE   pages;
    SIZE_TYPE   alloc_pages;
    char        *buf;
    extern i4  *etext;
    CL_ERR_DESC err_code;

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
CSprofend()
{
}

CSprofstart()
{
}
#endif


/*
** Name: CSsigchld - deal with SIGCHLD
**
** Description:
**      This routine is called upon receipt of SIGCHLD.  It waits for
**      the dead child, and then resets the signal handler for SIGCHLD.
**      It must be done in this order because of System V signal semantics.
**      If a signal trap for SIGCHLD is set up before the wait, an infinite
**      loop occurs.  If the wait fails, the dead child is not a slave, or
**      initialization is taking place, simply return, ignoring the death
**      of child event.  If this is not the case, i.e. a slave died,
**      shut the server down.
**
** Future Directions:
**      It would be nice not to call CSterminate from a signal handler.
**      Check out EX_EXIT.
**
** Inputs:
**      sig             signal number
** Outputs:
**      Returns:
**          void
**      Exceptions:
**          none
**
** Side Effects:
**          Waits for a dead child.  May shut down the server if critical
**              slave process dies
**
** History:
**      4-june-90 (blaise)
**          Integrated changes from 61 and ingresug:
**              Added this function.
**      16-Apr-91 (anton)
**          Shut down the server from here instead of using EXCHLDDIED
**              exception.
**      18-Oct-94 (nanpr01)
**          Handler should be set if pid != -1. Otherwise in BSD it will
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
CSsigchld(sig)
i4 sig;
{
        i4 pid,i;

        /* wait for dead child */

#ifdef VMS
        pid = 0;
#else
        pid = PCreap();
#endif

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
**      This function evades the usual EX system to directly handle
**      several UNIX signals which are documented to begin server
**      termination.  This handler is set up in CSdispatch.
**
** Future Directions:
**      It would be nice not to call CSterminate from a signal handler.
**      Check out EX_EXIT.
**
** Inputs:
**      sig     UNIX signal number to handle
**
** Outputs:
**      none
**
**      Returns:
**          0 - irrelavent UNIX signal handler
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      01-jan-88 (anton)
**          Created.
**      16-Apr-91 (anton)
**          Added function header comment
**          Fix cases - we are switching off SIGnals not EXceptions
**      31-jan-92 (bonobo)
**          Deleted double-question marks; ANSI compiler thinks they
**          are trigraph sequences.
*/
i4
CSsigterm(sig)
i4     sig;
{
    i4          i = 0;

    /* Signals below are in decreaseing severity */
    switch (sig) {
# ifdef SIGEMT
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
    return(0);
}
#endif
