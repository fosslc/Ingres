/*
** Copyright (c) 1986, 2009 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <me.h>
#include    <st.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <ci.h>
#include    <mu.h>
#include    <cv.h>
#include    <nm.h>

#include    <sp.h>
#include    <mo.h>

#include    <machconf.h>
#include    <descrip.h>
#include    <exhdef.h>
#include    <efndef.h>
#include    <iledef.h>
#include    <jpidef.h>
#include    <lnmdef.h>
#include    <psldef.h>
#include    <ssdef.h>
#include    <uaidef.h>
#include    <chfdef.h>
#include    <lib$routines.h>
#include    <starlet.h>

#ifdef KPS_THREADS
#include    <kpbdef.h>
#include    <exe_routines.h>
#endif

#include    <csinternal.h>
# include   "cslocal.h"
#include    "cssampler.h"

#include    <astjacket.h>

/**
**
**  Name: CSINTERFACE.C - Control System External Interface
**
**  Description:
**      This module contains those routines which provide the user interface to
**	system dispatching services to the DBMS (only, at the moment).
**
**          CSinitiate() - Specify startup characteristics, and allow control
**			system to initialize itself.
**          CSterminate() - Cancel all threads and prepare for shutdown.
**          CSalter() - Alter some/all of the system characteristics specified
**			by CSinitiate().
**          CSdispatch() - Have dispatcher take over operation of the system.
**          CSremove() - Remove a thread from the system (forcibly).
**	    CSsuspend() - Suspend a thread pending completion of an operation
**	    CSresume()	- Resume a suspended thread
**	    CScancelled() - Inform the CS module that an event for which
**			    a thread suspended will never occur
**	    CSadd_thread() - Add a thread to the server.
**	    CSstatistics() - Obtain runtime statistics for the current thread
**	    CSattn() - Inform the dispatcher that an unusual event has occurred
**	    CSget_sid()	- Obtain thread id for the current thread
**	    CSget_scb() - Obtain pointer to current thread control block
**	    CSfind_sid() - Obtain thread id for the given control block
**	    CSfind_scb() - Obtain pointer to external thread control block
**          CSp_semaphore() - Perform a "P" operation, claiming the semaphore.
**          CSv_semaphore() - Perform a "V" operation, releasing the semaphore.
**	    CSi_semaphore() - Initialize semaphore.
**	    CSw_semaphore() - Initialize and Name a semaphore
**			      (CSi_ + CSn_) in one operation.
**	    CSr_semaphore() - Remove a semaphore.
**	    CSa_semaphore() - Attach to a CS_SEM_MULTI semaphore.
**	    CSd_semaphore() - Detach from a CS_SEM_MULTI semaphore.
**	    CSn_semaphore() - Give a name to a semaphore.
**          CSintr_ack() - Clear CS interruupt status
**	    CSdump_statistics() - Dump CS statistics to Log File.
**          CSaccept_connect() - Returns TRUE if the server can accept another
**				connection request, otherwise it returns FALSE.
**	    CSswitch()	- poll for thread switch
**	    CSget_cpid() - Obtain cross-process thread identity.
**          CSnoresnow() - Cancel any outstanding CSresume's.
**          CSadjust_counter() - Thread-safe counter adjustment.
**
**
**  History:
**      27-Oct-1986 (fred)
**          Created.
**      15-jan-1987 (fred)
**          Added command line parsing and passing info block to server startup.
**      15-jan-1987 (fred)
**          Added command line parsing and passing info block to server startup.
**      16-jul-1987 (fred)
**          Added GCF support
**      11-Nov-1987 (fred)
**          Add code for semaphore statistics collection & reporting
**      11-Nov-1987 (fred)
**          Add code for RLR's -- resource limited regions.
**	20-Sep-1988 (rogerk)
**	    Added CSdump_statistics routine.  Skipped some collecting of
**	    wait statistics for DBMS task threads as they tended to skew the
**	    statistics gathering.
**	31-oct-1988 (rogerk)
**	    Added CSaltr_session routine.
**	    Added cso_facility field to CS_OPTIONS struct.  CS should check
**	    for any CSO_F_CS startup options.
**	    Added option (CSO_SESS_ACCTNG) to write records to the accounting
**	    file for each dbms session.
**	    Ifdef'ed lib$ast_in_prog call in CSsuspend.
**	    Fixed ast problem in CSdispatch.
**	    Added CS_CPU_MASK to mark whether CPU stats should be collected
**	    for a thread.
**	    Added ability to CSstatistics to return server statistics.
**	    Made CSstatistics use local struct for jpiw call instead of sharing
**	    one with CS_xchng_thread.
**	    Added print function argument to CSdump_statistics routine.
**	 7-nov-1988 (rogerk)
**	    Reset ast's before leaving CSremove.
**	    Add return code from CSaltr_sessions indicating that CPU stat
**	    collecting was already enabled.
**	    Added non-ast task switching and non-ast methods of protecting
**	    against asynchronous routines. Call CS_ssprsm directly instead 
**	    of through AST's.  Added new inkernel protection.  When turn
**	    of inkernel flag, check if need to call CS_move_async.
**	16-dec-1988 (fred)
**	    set cs_async field when listen completes while dispatcher is in
**	    kernel state.  BUG 4255.
**      20-JAN-89 (eric)
**          Added CS_accept_connect() for GCA rollover support.
**	28-feb-89 (rogerk)
**	    Added cross process semaphore support.  Added cs_pid to server
**	    control block and session control blocks.  Added CSi_semaphore
**	    routine.  Added cross process support in CSp and CSv_semaphore.
**	20-mar-89 (rogerk)
**	    Zero cs_owner field when release cross-process semaphore to fix
**	    race condition.
**	10-apr-89 (rogerk)
**	    Set inkernel while setting/releasing cross process semaphores to
**	    make sure that we don't switch threads in the middle of setting
**	    the semaphore owner fields.
**	    Changed maximum spin lock loop count to 100,000 - if reach max,
**	    reset loop counter to allow thread to compete for semaphore after
**	    sleeping a second.
**	17-apr-89 (rogerk)
**	    Added CS_clear_sem macro to unset semaphore value with an
**	    interlocked instruction.
**      23-may-89 (fred)
**	    Altered order of operations in CSdispatch.  See comment there for
**	    further details.
**      07-jun-89 (fred)
**	    Fixed CSterminate() to obtain the cs_next pointer before allowing
**	    an SCB to be dealloacted.  Failure to do so results in using
**	    dangling pointers (or could...).
**	 5-sep-1989 (rogerk)
**	    Added count of sleeps while waiting for cross-process semaphore.
**	    Mark session to not be swapped out by quantum timer while holding
**	    cross process semaphore.
**       18-Dec-1989 (anton)
**	    Adding CSswitch as a NOOP for bug 6143 UNIX quantum
**	04-jan-1989 (ralph)
**	    Change constant limit on checked_for_asts from 20 to 100 in
**	    CSp_semaphore.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    06-apr-1990 (fred)
**		Added support for CPU governing and additional monitoring
**		features
**	    23-apr-90 (bryanp)
**		Added support for a new mask to CSsuspend: CS_GWFIO_MASK,
**		which is used by non-SQL gateways to indicate that a session
**		has been suspended on non-Ingres I/O. This may include external
**		lock requests as well, so eventually CS_GWFIO_MASK suspensions
**		may be allowed to timeout or to be interruptable (RMS does not
**		currently require this, but CS has been enhanced to fully
**		support it).
**	10-dec-90 (neil)
**	    Alerters: Added CSfind_scb & modified CSattn to allow calling from
**	    a thread.
**	06-Dec-90 (anton)
**	    Clean up copying of server name
**	    Add definition of cs_mem_sem
**	    Add CS_CONDITION support
**	09-nov-91 (ralph)
**	    6.4->6.5 merge:
**	    29-jul-1991 (bryanp)
**		B38197: Fix thread hang when interrupt is followed by quantum
**		switch
**	    23-Jul-91 (rog)
**		Changed sys$asctoid() to sys$getuai() because the former has
**		trouble finding certain UIC's, and added uaidef.h.
**	    18-aug-91 (ralph)
**		CSmonitor() - Drive CSattn() when IIMONITOR REMOVE
**	    19-aug-1991 (rogerk)
**		Changed spin lock in cross process semaphore code to use a
**		"snooping" spin lock algorithm.
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**	3-jun-1992 (bryanp)
**	    Added CSr_semaphore() implementation.
**	4-jun-1992 (bryanp)
**	    Made Mike's 23-mar-1990 Unix CL changes to this file. His comments:
**
**	    Fixes to this file were necessary to make these semaphores work
**	    correctly when called from the dmfrcp, rather than the server.
**
**          - Changed the code to refer to "Cs_srv_block.cs_pid" rather than
**            to "scb->cs_pid" when it wanted the current process' pid.  The
**            old method did not work for some situations in non-dbms processes.
**          - Some code refered to the "scb" control block to keep track of
**            performance stats.  This code was changed to not be called in
**            the cases where the "scb" did not exist (was NULL).
**
**          If the state is CS_INITIALIZING then this is being called
**          from a non-dbms server (ie. the rcp, acp, createdb, ...).
**          We never want to call CSsuspend() from CSp_semaphore in this case,
**          so the code will spin on the semaphore instead.
**	6-Jul-92 (daveb)
**	    Maintain SPTREEs of scb's, semaphores, and conditions, for
**	    use by MO methods.  Implement CSr_semaphore and
**	    CSn_semaphore, and sketch CSa_semaphore and CSd_semaphore.
**	9-Jul-92 (daveb)
**	    Sem_tree won't work, use CS_sem_attach scheme instead.
**	1-oct-92 (daveb)
**	    Set up MU semaphores to use CS sems when we CSinitiate.
**	28-oct-92 (daveb)
**	    Remove CSmonitor; it's too big to be here.
**	7-oct-1992 (bryanp)
**	    If this is the Recovery Server, set the process name to DMFRCPxx.
**	15-oct-1992 (bryanp)
**	    Allow CSresume to be called at normal level as well as AST level.
**	20-oct-1992 (bryanp)
**	    Add support for E_CS0029_SEM_OWNER_DEAD in CSp_semaphore().
**	29-Oct-1992 (daveb)
**	    Add MUset_funcs call before threading.
**	14-nov-1992 (walt)
**	    Changes for Alpha.  Mainly replacing VAX register references to be
**	    Alpha register references.
**	9-Dec-1992 (daveb)
**	    Change .cl.clf classid's to .clf
**	10-Dec-1992 (daveb)
**	    Duh, actually initialize the CND tree key before inserting it!
**	14-Dec-1992 (daveb)
**	    re-enable the delete, which was commented out for
**	    no good reason.
**	13-Jan-1993 (daveb)
**	    Rename exp.clf.cs to exp.clf.vms.cs in all MO objects.
**	26-apr-93 (walt)
**	    Fixed cut-and-paste error in CSdispatch from prev integration.  The
**	    initial return address for the admin thread (....[CS_ALF_RA]) was
**	    missing the righthand side of its assignment statement.  The next
**	    statement in the flow was also an assignment statement.  So the
**	    admin thread's startup return address was being set to 3
**	    (=CS_OUTPUT) by syntactically being part of a continued assignment
**	    statment.
**	26-apr-1993 (bryanp)
**	    Improved support for CS_TIMEOUT_MASK in CSsuspend when cs_state
**	    is CS_INITIALIZING.
**	24-may-1993 (bryanp)
**	    Fixed race-condition bugs in CSms_thread_nap.
**	    Added the uni-processor optimization from the Unix CSp_semaphore
**		into the VMS CSp_semaphore: If on a uniprocessor machine, then
**		multi-process semaphores should not spin.
**	    Have CSterminate call PCexit, not sys$exit. This makes PCatexit()
**		work much better in server code.
**	15-jul-1993 (walt)
**	    Update csinterface.c with changes that have accumulated in the
**	    VAX cl.
**	19-jul-93 (walt)
**	    In CSstatistics nolonger use event flag zero.  Call lib$get_ef for
**	    an event flag instead.
**      16-jul-93 (ed)
**	    added gl.h
**	26-jul-1993 (bryanp)
**	    In CSms_thread_nap, if ms <= 0, just return immediately.
**	    In CSp_semaphore(CS_SEM_MULTI), incorporate the Unix tunability.
**	30-jul-1993 (walt)
**	    Added changes from VAX version prior to linking them in piccolo.
**      11-aug-93 (ed)
**          added missing includes
**	18-oct-1993 (rachael) Bug 55351,55353
**          A thread in the server has requested a cross-process semaphore and
**	    found it owned by another process. The requesting server has
**          descheduled itself -- immediately if on a SP machine, after looping
**          for some period of time on a MP machine -- to allow the owning
**          server to run, use,  and then relinquish the semaphore.
**
**          The descheduling is accomplished using two system services: a
**	    "schedule wakeup" call (sys$schdwk), followed by a "hibernate" call
**	    (sys$hiber). The sys$schdwk schedules the awakening of a process
**          after a requested amount of time.  The sys$hiber calls then places
**          the requesting process into a hibernating state.  When the wakeup
**          fires, the hibernating process wakes up and re-requests the
**          semaphore. This sequence of events will occur until the request is
**          granted or it is determined that it will be impossible to grant the
**          request, e.g. the owning process has crashed while holding
**	    the semaphore.
**  		
**	    The problem occurred when another thread in the requesting server
**	    also requested this semaphore.  The first thread had not been marked
**          NOSWAP, so it was swapped out just after scheduling its wakeup, and
**          before the call to sys$hiber. The second thread then would attempt
**          to acquire the semaphore.  The timing was such that the second
**          thread would "handle" the first thread's wakeup, along with its
**	    own. When the first thread was swapped back in, it made the call to
**          sys$hiber without knowing that there was no scheduled wakeup.
**
**	    The fix is to mark the requesting thread NOSWAP before the
**	    descheduling.
**	18-oct-1993 (walt)
**	    In CSaltr_session get an event flag number from lib$get_ef rather
**	    than use event flag zero in the sys$getjpiw call.  
**	18-Oct-93 (seiwald)
**	    CS option revamp: moved all option processing out to
**	    CSoptions(), and moved iirundbms.c's resource fiddling
**	    into here and into CSoptions().  CS_STARTUP_MSG and 
**	    CS_OPTIONS gone.
**	19-Oct-93 (seiwald)
**	    No more csdbms.h.
**	25-oct-1993 (bryanp)
**	    Repair arguments to SYS$CRELNM -- the itemlist structure was using
**		ints where it meant shorts.
**	28-Oct-1993 (rachael)
**	    In CSp_semaphore, do not call code that refers to an scb when
**	    called by a non-dbms server.  
**	15-Dec-1993 (daveb) 58424
**	    STlcopy for CSn_semaphore is wrong.  It'll write an EOS
**  	    into the cs_scribble_check that follows.
**	31-jan-1994 (bryanp) B56917
**	    CSoptions now returns STATUS and its status is now checked.
**      22-Feb-1994 (rachael) b58745
**          CSp_semaphore now checks for the semaphore owner pid
**          being 0 in addition to the check for whether the process
**          is alive.  This is necessary because a pid of 0 has special
**          meaning and the semaphore owner may be incorrectly thought dead
**          when the pid passed to PCisalive is 0.
**	29-mar-1994 (walt)
**	    Resurrect CSswitch and have it switch threads if the current thread
**	    has been in longer than a quantum.  This scheme is needed on the
**	    Alphas because the control block used in VAX/VMS to force an Ingres
**	    reschedule turned out to be readonly in Alpha/VMS.
**	25-apr-1994 (bryanp) B57419
**	    If cs_define is set, indicating that the caller wishes us to define
**		the logical name II_DBMS_SERVER automatically, then check to
**		see whether we are a subprocess or a detached process. If we are
**		a subprocess, define the logical name in the LNM$JOB table,
**		otherwise define the logical name in the LNM$SYSTEM table. This
**		restores some functionality accidentally broken by the pm-izing
**		of iirundbms.
**	    Made another attempt to close some of the holes relating to B58745.
**		In particular, added code to allow for the semaphore being
**		released by process B and acquired by process C just as
**		process A is checking to see if the owner is dead.
**	23-may-1994 (eml)
**	    Removed code that constructed termination mailbox and notified
**	    IIRUNDBMS of successful server startup. This code is now in
**	    GCregister in the routine GCsignal_startup().
**	13-jul-95 (dougb)
**	    Change name of quantum_ticks to CS_lastquant -- matching Unix CL.
**	    Add call to CSswitch() to CSstatistics() for additional CLF
**	    time slicing.
**	20-jul-95 (dougb)
**	    Correct problems uncovered by code copied from Unix CL (in
**	    cshl.c) during previous change:  CSp_semaphore() and
**	    CSv_semaphore() must maintain the ready mask in a consistent
**	    fashion.
**	    CS_move_async() now handles a NULL parameter; removed conditions
**	    from calls in CS[pv]_semaphore().
**	    Make a few other tests before defining II_DBMS_SERVER in the
**	    system logical name table.
**	    Link Admin and Repent scb's together to assist new "show
**	    internal sessions" monitor command.
**	    Make queue inserts consistent throughout for easier reading.
**      24-aug-95 (wonst02)
**          Add CSfind_sid() to return a SID given an SCB pointer.
**	10-oct-95 (duursma)
**	    Added CSset_exsp() and CSget_exsp() routines and call to
**	    i_EX_initfunc() to use new (Unix-like) EX facility.
**	07-dec-95 (dougb)
**	    Use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.  Also, status arg to Cs_srv_block.cs_elog
**          was changed to pass address rather than value.
**	21-dec-95 (dougb)
**	    Correct typo in wonst02's change of 24-aug-95.
**	31-dec-95 (whitfield)
**	    Changed references to VAX/VMS to VMS since this code concerns
**          Alpha VMS as well.  Also, enabled exception stack dumping.
**	02-jan-96 (dougb)
**	    Add CSw_semaphore() to finish jenjo02's change of 24-Apr-1995.
**	    Update the header comments for the *_semaphore() routines.  It
**	    also appears that daveb's fix for bug 59127 has never been
**	    cross-integrated.  The Unix change descriptions were:
**		17-May-94 (daveb) 59127
**  	    Fix stat initialization, add code to debug leaks.
**  	    Semaphores are not attached for IMA stat purposes
**  	    until they are named.  
**		11-May-1995 (jenjo02)
**	    Added CSw_semaphore(), which combines the functionality
**	    of CSi and CSn to initialize AND name a semaphore with
**	    one call.
**	24-jan-96 (dougb) bug 71590
**	    Correct prototype for CS_last_chance().  Don't set cs_registers[]
**	    in this module -- that's done for us by CS_alloc_stack().
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**	    In handler():  Don't interpret VMS mechanism vector as an array
**	    of longwords.  Get structure information from the chfdef.h
**	    system header file.  Don't try to $UNWIND during an unwind.
**	15-feb-1996 (duursma)
**	    Partial integration of UNIX CL change 423116: add CSs_semaphore();
**	    gutted its body since we don't do semaphore stats on VMS.
**	26-feb-1996 (duursma)
**	    Added debugging routine dump_deadlocked_semaphore().
**	30-jul-1996 (boama01)
**	    Chgd CSp_semaphore() and CSv_semaphore() to use new cs_test_set
**	    fld in sem instead of old cs_value for MULTI sems; the old fld
**	    was i2 and not on a long/quad boundary, causing the Memory Barrier
**	    BBSSI/BBCCI instructions in CS_test_set() and CS_clear_sem() to
**	    NOT block concurrent multiprocess access; the new fld is sized
**	    and aligned properly, and preceded by dummy long, to permit
**	    correct behavior (see !alpha!cl!hdr!hdr cscl.h).  This should
**	    ONLY be necessary for Alpha (not VAX).
**	    Also chgd dump_deadlocked_semaphore() to dump new fld.
**	09-jan-1996 (chech02)
**	   A negative value of timeout passed in CSsuspend is considered
**	   a millisecond for OI 2.0.
**      30-jul-1997 (teresak)
**	   Made some cross-integrations for facility codes and additional
**	   debugging info.
**         12-dec-1996 (canor01)
**          Add cs_facility field to CS_SCB to track facility for sampler
**          thread.
**         03-Jul-97 (radve01)
**          Basic, debugging related info, printed to dbms log at system
**          initialization.
**      30-jul-1997 (teresak)
**         Added changes found in original build area. Changes referencing
**         these changes were added in another integration plan by the
**         original authors.
**         Added call to retain the facility and to modify the declaration
**         of CS_SAMPLER function to func_extern as it has been replaced by
**         the Unix version.
**         5-dec-96 (stephenb)
**          Add code for performance profiling, new static cs_facilities
**         02-may-1997 (canor01)
**          If a thread is created with the CS_CLEANUP_MASK set in
**          its thread_type, transfer the mask to the cs_cs_mask.
**          These threads will be terminated and allowed to finish
**          processing before any other system threads exit.
**          and new routines CSfac_stats() and CScollect_stats().
**	04-Dec-1997 (kinte01)
**	   Add new exp.clf variables for VMS (cs.num_processors)
**	20-feb-1998 (kinte01)
**	   Cross-integrate Unix change 433682
**         7-jan-98 (stephenb)
**            Add active user high water mark to dump statistics.
**          Cross integrate changes from vaxvms_1201 into AlphaVMS CL
**              21-oct-1997 (rosga02)
**          Added fix to bug 86090 (contact 5786169) but done it differently
**          than in UNIX, because in VMS we do not maintain cs_sync_obj
**          pointer, so we cannot traverce a chain of holding semaphors.
**          Instead we check if a holding semaphore is not in a computable
**          state and the number of spins is already greater than (MAX - 1000)
**          we just lower our priority by 4 but not lower than 4.
**          Also, reversed order of TRdisplay() and lib$signal(SS$_DEBUG).
**             semaphore, try a combination of both sleeping and
**             switching to another sesssion in the same process.
**             Note: For multiprocessor machines also try to spin some time
**                   before sleeping
**           - Allow to control sleeping/switching ratio externally to adjust
**             to different user environment as follows by using logical
**             II_SLEEP_RATIO as in following example:
**                $ define II_SLEEP_RATIO  5 (5 sleeps  for 1 switch)
**                $ define II_SLEEP_RATIO -5 (5 swithes for 1 sleep)
**             Default is 1 sleep for 1 switch
**           - Change some tuning constants
**           - Report exessesive waiting time in the errlog.log to diagnose
**             hanging problems
**           Also, added some other changes to better report exceptional
**           situations.
**              19-jan-1998 (rosga02)
**          Do not switch sessions if cs is not initialized yet.
**	03-aug-1998 (kinte01)
**		Add additional parameters to ERinit call - i_sem_func &
**		n_sem_func
**	19-may-1998 (kinte01)
**		Cross-integrate Unix change 434548
**		16-Feb-98 (fanra01)
**		  Add initialisation of the attach and detach functions.
**	20-may-1998 (kinte01)
**		Cross-integrate Unix change 435120
**		31-Mar-1998 (jenjo02)
**		  Added *thread_id output parm to CSadd_thread().
**	11-aug-1998 (kinte01)
**		integrate change 423131
**          01-Dec-1995 (jenjo02)
**             Defined 2 new wait reasons, CS_LGEVENT_MASK and CS_LKEVENT_MASK
**             to statistically distinguish these kinds of waits from other
**             LG/LK waits. 
**          16-Jan-1996 (jenjo02)
**             CS_LOG_MASK is no longer the default wait reason and must
**             be explicitly stated in the CSsuspend() call.
**          25-Mar-1997 (jenjo02)
**             Third parameter may be a ptr to a LK_LOCK_KEY when sleep
**             mask is CS_LOCK_MASK (for iimonitor) instead of an ecb.
**	11-sep-1998 (kinte01)
**		cross integrate Unix change 436233
**          06-may-1998 (canor01)
**             Initialize CI function to retrieve user count.  This is only
**             valid in a server, so initialize it here.
**	16-Nov-1998 (jenjo02)
**	    Added CSget_cpid() external function, used by LG/LK, 
**	    CScp_resume().
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**      12-Mar-1999 (shero03)
**          Set the static vector in FP with the address of CSget_scb.
**	22-Mar-1999 (kinte01)
**	    09-jan-1997 (canor01)
**		Do not register semaphores with MO; delete calls to
**		CS_sem_attach().
**      22-Jun-1999 (horda03)
**              Cross-Integrate changes 440975 and 441954.
**              29-Jan-1999 (hanal04, horda03)
**                 Add support for shared access to multiprocess semaphores in
**                 CSp_semaphore() and CSv_semaphore(). b95062.
**                 Ensure CSs_semaphore() fills in the semaphore name in the
**                 stats variable as this is used in errlog entries.
**              18-May-1999 (hanal04 & horda03)
**                 The above change resulted in the CSP hanging during node
**                 failure recovery. The hang was caused when a deadlock situation
**                 occured on a Mutex, but because this is not a DBMS server
**                 the Cs_srv_block.cs_state is set to CS_INITIALIZING. b96998.
**	15-Jul-1999 (kinte01)
**		Add new counts to MO block
**	14-Apr-2000 (devjo01)
**	        Allow CSp_semaphore to handle complicated scheduling deadlocks
**	        involving cross process mutexes. (b101216, plus X of 95607).
**      20-Apr-2000 (horda03) X-Int of Bug 48904 from ingres12.
**          30-mar-1994 (rachael) Bug 48904 diagnostics 
**           Added routine CSnoresnow and CSnoreslk which looks to see if the scb passed in
**           has the edone bit set.  If it does, it prints a message to 
**           II_DBMS_LOG and calls CScancelled.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**      16-Aug-2000 (horda03)
**          In CSp_semaphore, another thread can only be scheduled if the CS_STATE is
**          NOT CS_INITIALIZING. This state indicates that the CSp_routine has been
**          invoked by a server which does not contain thread (e.g. ACP, JSP).
**          (102351)
**      01-Dec-2000 (horda03)
**         CSSAMPLING accvio's due to differences in use of cs_memory
**         and cs_sync_obj on VMS to UNIX. Bug 56445 introduced the
**         field cs_sync_obj in CS_SCB to hold the semaphore ptr being
**         addressed in CS routine rather than the miss use of cs_memory.
**      22-Mar-2001 (horda03)
**          Previous attempt at fixing Bug 90741 (Active Session count)
**          Change 448354 only solved part of the problem. In servers with
**          high contention for CPU, the active sessions count increases.
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	07-feb-2001 (kinte01)
**	    Add casts to quiet compiler warnings
**	13-mar-2001 (kinte01)
**	    Add CSget_svrid to return the server name
**      02-Aug-2001 (horda03) Bug 95062
**          On a Single CPU machine, a session was spinning waiting for a
**          mutex. It should have relinquished the processor.
**      23-Sep-2001 (bolke01)  
**          Code scanned the and identified that there were several 
**          instances that allowed the async calling to be wrong
**          inkernal flags were in wrong sequence.
**      23-Sep-2001 (bolke01)  
**          Stopped EDONE message from being written to the errlog
**	    Problem INGSRV 1665 (b106882) 
**	16-oct-2001 (kinte01)
**	    Clean up more compiler warnings
**	07-nov-2001 (kinte01)
**	    Add more compiler casts
**      14-Mar-2003 (bolke01) Bug 110235 INGSRV 2263
**          Ensure that in in_kernel flags are set when checking the cs_mask
**	    in CSsuspend. to prevent unknown AST's interupting this  
**	    critical process section.
**	    When returning from CSp_semaphore always call CS_move_async() if
**	    the Cs_srv_block.cs_async flag is set.
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      23-oct-2003 (devjo01)
**          Added 'CSadjust_counter()', init 'cs_format_lkkey', and within
**	    CSsuspend, set cs_synch_obj to the ecb if CS_LKEVENT_MASK set.
**	    This is a hand cross of applicable 103715 changes to the UNIX
**	    CL in main.
**	11-Jun-2004 (hanch04)
**	    Removed reference to CI for the open source release.
**      16-Sep-2004 (horda03) Bug 113019/INGSRV2961
**          If an AST rsumes a session while non-AST code is resuming the
**          CS_ADDER_ID (Admin) session, the Ready mask bit set for the session
**          resumed in the AST may be lost when the non-AST code resumes.
**          The non-AST code, doesn't protect against ASTs updating the
**          CS_READY_MASK.
**	23-sep-2004 (abbjo03/horda03) Bug 112982/INGSRV112982
**	    Remove tests for CS_ABORTED_MASK in CSp_semaphore.
**          Also, when a session owns a mutex the priority of the session
**          is set to SEM_OWNER_PRI. Once all mutexes owned by the
**          session is released, the priority is reset to the original
**          value.
**      20-oct-2004 (horda03) Bug 112983/INGSRV3009
**          Interface to CS_cp_mbx_create now requires number of sessions
**          supported by the process.
**	21-dec-2004 (abbjo03)
**	    Add IICSsuspend_for_AST. Change CScas_long to CScas4 in
**	    CSadjust_counter. Remove ancient unimplemented functions.
**	01-feb-2005 (abbjo03)
**	    Fix the debugging code in CSdispatch.
**	14-apr-2005 (jenjo02/abbjo03)
**	    In CScnd_wait, fix an instance of cs_memory which should've been
**	    changed to cs_sync_obj.  In CSattn, change the test for CS_CND_WAIT
**	    to not require CS_SHUTDOWN_EVENT.
**	28-apr-2005 (devjo01)
**	    - Correct subtle error in CSadjust_counter which could lead to
**	    a lost update.
**	    - Remove IICSsuspend_for_AST.  A macro maps it to CSsuspend
**	    for VMS.
**	12-may-2005 (abbjo03)
**	    Back out 23-sep-2004 change since it interferes with CUT.
**	16-jun-2005 (abbjo03)
**	    In CSget_sid(), return CS_ADDER_ID if cs_current is the idle or the
**	    admin thread.  In CSdispatch(), use CS_ADDER_ID for the admin
**	    thread's cs_self.
**	17-jun-2005 (abbjo03)
**	    Partial undo of 16-jun change:  the CSdispatch() appears to hang
**	    the server after a couple of sessions.
**	04-nov-2005 (abbjo03)
**	    Split CS_SCB bio and dio counts into reads and writes.
**      18-oct-2004 (horda03) Bug 113246/INGSRV3003
**          A non-threaded server can report a TIMEOUT even when no timeout
**          interval was specified.
**	04-oct-2006 (abbjo03)
**	    Apply the 16-jun-2005 change to CSget_cpid() which uses similar
**	    logic.
**	08-Feb-2007 (kiria01) b117581
**	    Use global cs_mask_names and cs_state_names.
**	15-mar-2007 (jonj)
**	    In CSp_semaphore, schedule another thread instead of hibernating
**	    as there may be other threads that need to run for the blocking
**	    process to release the mutex that this thread needs.
**	02-may-2007 (jonj/joea)
**	    Replace CS_test_set/CS_clear_sem by CS_TAS/CS_ACLR.  Remove
**	    #ifdef VMS clutter in CSp/v_semaphore.
**	25-may-2007 (jonj/joea)
**	    Ensure ASTs are on in yield_cpu_time_slice.  In CSp_semaphore,
**	    for cross-process semaphores, allow CPU yields after rescheduling.
**      20-Jun-2008 (horda03) Bug 120474
**          A session which doesn't suspend itself regularly will hog the process
**          not allowing other sessions within the server to be scheduled. This is
**          due to CSswitch only calling CS_ssprm every 2 quantum (i.e default is
**          2 seconds), this is an age is computer time. No do it each time when
**          a user thread invokes the function.
**      23-Jul-2008 (horda03) Bug 120474
**          CS_ssprm should only be invoked in BE servers.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	14-oct-2008 (joea)
**	    Integrate 16-nov-1998 and 29-sep-2000 changes from unix version.
**	    Split bio/dio stats into read/write.  Add log i/o stats.  Add
**	    new MO objects for new stats.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**	21-nov-2008 (joea)
**	    Prepare for support of both OS and internal threads by renaming
**	    CSxxx functions to IICSxxx.  Change Cs_numprocessors,
**	    Cs_desched_usec_sleep and Cs_max_sem_loops GLOBALDEFs and use
**	    ii_sysconf to get their values.
**	24-nov-2008 (joea)
**	    Add the CS_functions and Cs_fvp variables for supporting both
**	    OS and internal threads.  Add missing cp local in CSinitiate.
**	02-dec-2008 (joea)
**	    Implement CS_num_active/CS_hwm_active.
**	09-dec-2008 (joea)
**	    Add Cs_utility_sem for CSMT.  Rename CS_lastquant to Cs_lastquant
**	    to really match the Unix CL.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**	30-jun-2009 (joea)
**	    Change CSget_exsp to match Unix.
**	10-Feb-2010 (smeke01) b123226, b113797
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
**      21-jan-2010 (stegr01/joea)
**          Add support for KP Services.
**      18-Feb-2009 (horda03)
**          Assign cs_diag.
**      24-feb-2010 (joea)
**          Check the active bit as well in thread_scheduler and return a
**          proper status.  Correct CS_addrcmp argument types and rewrite to
**          match Unix version.
**      10-mar-2010 (joea)
**          In CSadd_thread, if CS_alloc_stack returns an error, call
**          cs_scbdealloc and cs_reject before returning to the caller.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN STATUS  CSinitiate();	/* Prepare to start the server */
FUNC_EXTERN STATUS  CSalter();          /* Alter the server characteristics */
FUNC_EXTERN void    CS_ssprsm();	/* suspend one thread, resume one */ 
FUNC_EXTERN void    CS_exit_handler();
#ifdef ALPHA
FUNC_EXTERN void    CS_cactus_save();
#endif
FUNC_EXTERN void    CS_move_async();
#if defined(__STDARG_H__)
FUNC_EXTERN i4  TRdisplay( char *, ... );
#else
FUNC_EXTERN STATUS  TRdisplay();
#endif
FUNC_EXTERN void i_EX_initfunc(
    void (*setfunc)(EX_CONTEXT *),
    EX_CONTEXT **(*getfunc)(void));	/* Init the EX facility */

FUNC_EXTERN void CS_mo_init(void);
FUNC_EXTERN void CS_rcv_request();
FUNC_EXTERN void CS_scb_attach();
FUNC_EXTERN void CS_change_priority();
FUNC_EXTERN STATUS CS_alloc_stack();
FUNC_EXTERN STATUS CS_cp_mbx_create();
typedef void FP_Callback(CS_SCB **scb);
FUNC_EXTERN void    FP_set_callback( FP_Callback fun);

i4 CS_addrcmp( const char *a, const char *b );

/*
**  Defines of other constants.
*/

/*
**      One second in VMS time in units that the quantum is measured in
**	This is 10,000,000 100 ns units, negative because it is a delta time.
*/

#define                 CS_ONE_SECOND	     (-10 * 1000 * 1000)
#define                 CS_ONE_MILLISECOND   (-10 * 1000 )

/*
**	Number of spin loops we will do while waiting for a cross-process
**	semaphore before we start doing one second sleeps.  Currently
**	100,000 loops (about 1 million instructions).
*/

#define			CS_MAX_SEM_LOOPS    100000
#define			CS_MIN_REDUCED_PRIORITY  4
#define			CS_PRIORITY_DECREMENT	 4

#ifdef KPS_THREADS
#define CS_stall()  exe$kp_stall_general(scb->kpb)
#else
#define CS_stall()  CS_ssprsm(FALSE)
#endif

#ifdef PERF_TEST
/*
** constats and defines for performance testing
*/
typedef struct _CS_FACILITIES
{
    char        facility_name[6];
    int         facility_id;
    int         sub_facility_id;
} CS_FACILITIES;

static const CS_FACILITIES cs_facilities[] =
{
    "NONE",     0,              0,
    "CLF",      CS_CLF_ID,      0,
    "ADF",      CS_ADF_ID,      0,
    "DMF",      CS_DMF_ID,      0,
    "OPF",      CS_OPF_ID,      0,
    "PSF",      CS_PSF_ID,      0,
    "QEF",      CS_QEF_ID,      0,
    "QSF",      CS_QSF_ID,      0,
    "RDF",      CS_RDF_ID,      0,
    "SCF",      CS_SCF_ID,      0,
    "ULF",      CS_ULF_ID,      0,
    "DUF",      CS_DUF_ID,      0,
    "GCF",      CS_GCF_ID,      0,
    "RQF",      CS_RQF_ID,      0,
    "TPF",      CS_TPF_ID,      0,
    "GWF",      CS_GWF_ID,      0,
    "SXF",      CS_SXF_ID,      0,
};
#endif /* PERF_TEST */

static i4  CS_usercnt( void );


/*
** Definition of all global variables owned by this file.
*/

#ifdef OS_THREADS_USED
/*
** This defines the CS/CSMT function vector table. Depending on
** whether Ingres is configured to run with Ingres or OS threads
** either CSinitiate() or CSMTinitiate() will initialize it with
** the appropriate function pointers.
*/
static    CS_FUNCTIONS        CS_functions                   ZERO_FILL;
GLOBALDEF CS_FUNCTIONS        *Cs_fvp = &CS_functions;
#endif /* OS_THREADS_USED */

GLOBALDEF CS_SYSTEM           Cs_srv_block ZERO_FILL; /* Overall ctl struct */
GLOBALDEF CS_SEMAPHORE        Cs_known_list_sem              ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_queue_hdrs[CS_LIM_PRIORITY] ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_known_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_to_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_wt_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_as_list_hdr		     ZERO_FILL;
GLOBALDEF CS_STK_CB	      Cs_stk_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_idle_scb		     ZERO_FILL;
GLOBALDEF CS_SCB              Cs_repent_scb                  ZERO_FILL;
GLOBALDEF CS_ADMIN_SCB	      Cs_admin_scb		     ZERO_FILL;
GLOBALDEF CS_SYNCH	      Cs_utility_sem	             ZERO_FILL;
GLOBALDEF PTR		      Cs_save_exsp		     ZERO_FILL;
GLOBALDEF PTR		      Cs_old_last_chance;
GLOBALREF void                (*Ex_print_stack)();
static	short		      Cs_signal ZERO_FILL;

GLOBALREF VOID                (*Ex_diag_link)();

/* process_wait_state used for non-threaded processes to synchronise
** process suspension with cross process resumes.
*/
static	volatile i4		process_wait_state = 0;
#define  CS_PROCESS_NOWAIT 0    /* Process not in HIBER state */
#define  CS_PROCESS_WAIT   1    /* Process is suspended in HIB */
#define  CS_PROCESS_WOKEN  2    /* Process was resumed via cs_cp_resume */
static	i4		      got_ex	ZERO_FILL;
static	i4			in_timeout_suspend ZERO_FILL;
GLOBALDEF i4		Cs_numprocessors = DEF_PROCESSORS;
GLOBALDEF i4		Cs_desched_usec_sleep = DEF_DESCHED_USEC_SLEEP;
GLOBALDEF i4		Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
static  i4                     Cs_sleep_ratio = 1;

static void CSset_exsp(EX_CONTEXT *);	/* set current exception stack ptr */
static EX_CONTEXT **CSget_exsp(void);	/* get curr exception stack ptr */

static	void	CS_ms_timeout(CS_SCB *scb);
static	void	yield_cpu_time_slice(i4 owning_pid, i4 sleep_time);

#ifdef KPS_THREADS
static STATUS thread_scheduler(void);
#endif

GLOBALREF   i4     Cs_lastquant;      /* defined in cshl.c */

static MO_GET_METHOD CSmo_bio_done_get;
static MO_GET_METHOD CSmo_bio_waits_get;
static MO_GET_METHOD CSmo_bio_time_get;
static MO_GET_METHOD CSmo_dio_done_get;
static MO_GET_METHOD CSmo_dio_waits_get;
static MO_GET_METHOD CSmo_dio_time_get;
static MO_GET_METHOD CSmo_lio_done_get;
static MO_GET_METHOD CSmo_lio_waits_get;
static MO_GET_METHOD CSmo_lio_time_get;

GLOBALDEF MO_CLASS_DEF CS_int_classes[] =
{
  { 0, "exp.clf.vms.cs.num_processors", sizeof(Cs_numprocessors),
        MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
        (PTR)&Cs_numprocessors, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.scballoc",
	sizeof( Cs_srv_block.cs_scballoc ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_scballoc ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.scbdealloc",
	sizeof( Cs_srv_block.cs_scbdealloc ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_scbdealloc ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.elog",
	sizeof( Cs_srv_block.cs_elog ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_elog ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.process",
	sizeof( Cs_srv_block.cs_process ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_process ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.attn",
	sizeof( Cs_srv_block.cs_attn ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_attn ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.startup",
	sizeof( Cs_srv_block.cs_startup ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_startup ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.shutdown",
	sizeof( Cs_srv_block.cs_shutdown ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_shutdown ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.format",
	sizeof( Cs_srv_block.cs_format ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_format ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.saddr",
	sizeof( Cs_srv_block.cs_saddr ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_saddr ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.reject",
	sizeof( Cs_srv_block.cs_reject ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_reject ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.disconnect",
	sizeof( Cs_srv_block.cs_disconnect ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_disconnect ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.read",
	sizeof( Cs_srv_block.cs_read ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_read ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.write",
	sizeof( Cs_srv_block.cs_write ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_write ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.max_sessions",
	sizeof( Cs_srv_block.cs_max_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_max_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.num_sessions",
	sizeof( Cs_srv_block.cs_num_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_num_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.user_sessions",
	sizeof( Cs_srv_block.cs_user_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_user_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.max_active",
	sizeof( Cs_srv_block.cs_max_active ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_max_active ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.cs_hwm_active",
	sizeof( Cs_srv_block.cs_hwm_active ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_hwm_active ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.cursors",
	sizeof( Cs_srv_block.cs_cursors ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_cursors ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.num_active",
	sizeof( Cs_srv_block.cs_num_active ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_num_active ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.stksize",
	sizeof( Cs_srv_block.cs_stksize ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stksize ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.stk_count",
	sizeof( Cs_srv_block.cs_stk_count ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stk_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.ready_mask",
	sizeof( Cs_srv_block.cs_ready_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_ready_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.known_list",
	sizeof( Cs_srv_block.cs_known_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_known_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wt_list",
	sizeof( Cs_srv_block.cs_wt_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_wt_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.to_list",
	sizeof( Cs_srv_block.cs_to_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_to_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.current",
	sizeof( Cs_srv_block.cs_current ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_current ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.next_id",
	sizeof( Cs_srv_block.cs_next_id ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_next_id ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.error_code",
	sizeof( Cs_srv_block.cs_error_code ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_error_code ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.state",
	sizeof( Cs_srv_block.cs_state ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_state ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.mask",
	sizeof( Cs_srv_block.cs_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.aquantum",
	sizeof( Cs_srv_block.cs_aquantum ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.aq_length",
	sizeof( Cs_srv_block.cs_aq_length ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.bquantum0",
	sizeof( Cs_srv_block.cs_bquantum[0] ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum[0] ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.bquantum1",
	sizeof( Cs_srv_block.cs_bquantum[1] ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum[1] ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.toq_cnt",
	sizeof( Cs_srv_block.cs_toq_cnt ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_toq_cnt ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.q_per_sec",
	sizeof( Cs_srv_block.cs_q_per_sec ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_q_per_sec ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.quantums",
	sizeof( Cs_srv_block.cs_quantums ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_quantums ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.idle_time",
	sizeof( Cs_srv_block.cs_idle_time ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_idle_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.cpu",
	sizeof( Cs_srv_block.cs_cpu ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_cpu ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.svcb",
	sizeof( Cs_srv_block.cs_svcb ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_svcb ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.stk_list",
	sizeof( Cs_srv_block.cs_stk_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stk_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.gca_name",
	sizeof( Cs_srv_block.cs_name ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_name ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.pid",
	sizeof( Cs_srv_block.cs_pid ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_pid ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.smsx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smsx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.smxx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smxx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smxx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.smcx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smcx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smcx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.smx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.sms_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_sms_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_sms_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.smc_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smc_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smc_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.smcl_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smcl_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smcl_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index }, 

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.cs_smsp_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smsp_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsp_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.cs_smmp_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmp_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmp_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.sem_stats.cs_smnonserver_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smnonserver_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smnonserver_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bio_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bio_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bio_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bio_time",
	0, MO_READ, 0,
	0, CSmo_bio_time_get, MOnoset, NULL, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.biow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bio_waits",
	0, MO_READ, 0,
	0, CSmo_bio_waits_get, MOnoset, NULL, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.biow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bio_done",
	0, MO_READ, 0,
	0, CSmo_bio_done_get, MOnoset, NULL, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.bior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.biow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dio_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dio_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dio_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dio_time",
	0, MO_READ, 0,
	0, CSmo_dio_time_get, MOnoset, NULL, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.diow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dio_waits",
	0, MO_READ, 0,
	0, CSmo_dio_waits_get, MOnoset, NULL, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.diow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dio_done",
	0, MO_READ, 0,
	0, CSmo_dio_done_get, MOnoset, NULL, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.dior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.diow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lio_time",
	0, MO_READ, 0,
	0, CSmo_lio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lio_waits",
	0, MO_READ, 0,
	0, CSmo_lio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lio_done",
	0, MO_READ, 0,
	0, CSmo_lio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.liow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.liow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.liow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lg_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lg_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lg_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lg_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lk_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lk_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lk_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.lk_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.tm_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.tm_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.tm_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.vms.cs.srv_block.wait_stats.tm_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

# if 0
  { 0, "exp.clf.vms.cs.srv_block.optcnt",
	sizeof( Cs_srv_block.cs_optcnt ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_optcnt ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  /* Need CS_options_idx and MOdouble_get methods */

  { 0, "exp.clf.vms.cs.srv_block.options.index",
	sizeof( Cs_srv_block.cs_option.cso_index ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_index ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.vms.cs.srv_block.options.value",
	sizeof( Cs_srv_block.cs_option.cso_value ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_value ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.vms.cs.srv_block.options.facility",
	sizeof( Cs_srv_block.cs_option.cso_facility ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_facility ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.vms.cs.srv_block.options.strvalue",
	sizeof( Cs_srv_block.cs_option.cso_strvalue ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_strvalue ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.vms.cs.srv_block.options.float",
	sizeof( Cs_srv_block.cs_option.cso_float ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_float ),
	MOdouble_get, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },
# endif
  { 0 }
};

/*{
** Name: CSinitiate - Initialize the CS and prepare for operation
**
** Description:
**      This routine is called at system startup time to allow the CS module 
**      to initialize itself, and to allow the calling code to provide the 
**      appropriate personality information to the module. 
** 
**      The personality information is provided by the single parameter, which 
**      is a pointer to a CS_CB structure.  This structure contains personality
**      information including the "size" of the server in terms of number of 
**      allowable threads, the size and number of stacks to be partitioned  
**      amongst the threads, and the addresses of the routines to be called 
**      to allocate needed resources and perform the desired work.  (The calling 
**      sequences for these routines are described in <cs.h>.) 
** 
** Inputs:
**	argc				Ptr to argument count for host call
**	argv				Ptr to argument vector from host call
**      ccb                             Call control block, of which the
**					    following fields are interesting
**					If zero, the system is initialized to
**					allow standalone programs which
**					may use CL components which call
**					CS to operate.
**      .cs_scnt			Number of threads allowed
**	.cs_ascnt			Nbr of active threads allowed
**	.cs_stksize			Size of stacks in bytes
**	.cs_shutdown			Server shutdown
**					    rv = (*cs_shutdown)();
**	.cs_startup			Server startup
**					    rv = (*cs_startup)(&csib);
**					where csib has the following fields
**					    csib.cs_scnt -- connected threads
**					    csib.cs_ascnt -- active threads
**					    csib.cs_multiple -- are objects in this server
**							    served by multiple servers
**					    csib.cs_cursors -- # of cursors per thread allowed
**					    csib.cs_min_priority -- minimum
**						priority a CS client can start
**						a thread with.
**					    csib.cs_max_priority -- maximum
**						priority a CS client can start
**						a thread with.
**					    csib.cs_norm_priority -- default
**						priority for a session if no
**						priority is specified at startup
**						time.
**					    csib.cs_optcnt -- number of options
**					    csib.cs_option --
**						array of option blocks
**						describing application
**						specific options.
**				    >>	    csib.cs_name is expected to be
**				    >>	    filled with the server name
**	.cs_scballoc			Routine to allocate SCB's
**					rv = (*cs_scballoc)(&place to put scb,
**								buffer);
**					    where buffer is that filled by
**					    (*cs_saddr)() (see below)
**				    >>  scb->cs_username is expected to be
**				    >>  filled with the name of the invoking
**				    >>  user.
**	.cs_scbdealloc			Routine to dealloc  SCB's
**					    rv = (*cs_scbdealloc)(&scb alloc'd above);
**	.cs_elog			Routine to log errors
**					    rv = (*cs_elog)(internal error,
**							&system_error,
**							p1_length, &p1_value, ...);
**	.cs_process			Routine to do major processing
**					    rv = (*cs_process)(mode,
**							&scb,
**							&next_mode);
**					where mode and next mode are one of 
**					CS_INITIATE,
**					CS_INPUT,
**					CS_OUTPUT,
**					CS_EXCHANGE, or
**					CS_TERMINATE
**					    nmode is filled by the callee.
**	.cs_attn			Routine to call to handle async events
**					    (*cs_attn)(event_type,
**							scb for event)
**	.cs_format			Routine to call to format scb's for monitor
**	.cs_read			Routine to call to get input	    
**					    (*cs_read)(scb, sync)
**					 where here and below, the <sync>
**					  parameter indicates whether the
**					  service should operate synchronously
**					  or not.  If not, then the caller
**					  will suspend, unless otherwise
**					  noted.
**	.cs_write			Routine to call to place output
**					    (*cs_write)(scb, sync)
**					  Will be called repeatedly as long
**					  as it continues to return OK.
**					  To normally terminate the writing,
**				          return E_CS001D_NO_MORE_WRITES.
**					  (This assumes users ``ask short
**					  questions requiring long answers''.)
**	.cs_saddr			Routine to call to request threads.
**					    (*cs_saddr)(buffer, sync)
**					>>> In this case, the thread to be
**					>>> resumed should be CS_ADDR_ID.
**	.cs_reject			Routine to call to reject threads.
**					    (*cs_reject)(buffer, sync)
**					    where buffer is that used on the
**					    previous (*cs_saddr)() call.
**	.cs_disconnect			Routine to call to disconnect
**					communications with partner.
**					    (*cs_disconnect)(scb)
**
** Outputs:
**      none
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-Oct-1986 (fred)
**          Initial Definition.
**      15-jan-1987 (fred)
**          Added command line parsing
**      24-feb-1987 (fred)
**          Added AIC & PAINE handling.
**	29-sep-1988 (thurston)
**	    Added semaphoring information to the ERinit() call.
**	31-oct-1988 (rogerk)
**	    Check startup options for CSO_F_CS options - CSO_SESS_ACCTNG and
**	    CSO_CPUSTATS.
**	 7-nov-1988 (rogerk)
**	    Added async ready list.
**	28-feb-1989 (rogerk)
**	    Set process id (cs_pid) in the cs_system control block.
**	    Set cs_pid in idle thread scb.
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**	4-Jul-92 (daveb)
**	    set up known element trees, and put idle, admin and added scbs
**	    into cs_sem_tree.  Put sems and cnd's intheir respective trees,
**	    and remove on their demise.  (SCB's are removed in CS_del_thread in
**	    cshl.c).
**	9-Jul-92 (daveb)
**	    Sem_tree won't work, use CS_sem_attach scheme instead.
**	23-sep-1992 (bryanp)
**	    LGK_initialize no longer needs to be called from CSinitiate().
**	24-may-1993 (bryanp)
**	    Added the uni-processor optimization from the Unix CSp_semaphore
**		into the VMS CSp_semaphore: If on a uniprocessor machine, then
**		multi-process semaphores should not spin. In CSinitiate, we
**		call $SYI to find out whether we are on a uniprocessor.
**	26-jul-1993 (bryanp)
**	    Added NMgtAt() control over Cs_numprocessors, Cs_max_sem_loops,
**		and Cs_desched_usec_sleep. This is to make the VMS multi-p
**		process semaphore support more "tunable" in the ways that the
**		Unix support is.
**	31-jan-1994 (bryanp) B56917
**	    CSoptions now returns STATUS and its status is now checked.
**	27-jul-1995 (dougb)
**	    Make queue inserts consistent throughout for easier reading.
**	10-oct-95 (duursma)
**	    Added call to i_EX_initfunc() to use new (Unix-like) EX facility.
**	07-dec-95 (dougb)
**	    Use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.  Also, status arg to Cs_srv_block.cs_elog
**          was changed to pass address rather than value.
**	31-dec-95 (whitfield)
**	    Changed references to VAX/VMS to VMS since this code concerns
**          Alpha VMS as well.  Also, enabled exception stack dumping.
**	02-jan-96 (dougb)
**	    Attach the idle thread when we're almost ready to go.  Clean up the
**	    history comments (combine two sections).
**	24-jan-96 (dougb)
**	    Group history comments together.
**      20-oct-2004 (horda03) Bug 112983/INGSRV3009
**          The number of CP resume messages that can be posted to the CP resume
**          mailbox, is now dependent on the number of sessions the process
**          can handle.
**      04-Nov-2009 (horda03) Bug 122849
**          Initialise CS_sched_quantum.
**
**  Design:  <VMS>
**
**      This routine first initializes the system control area, Cs_srv_block. 
**      This involves the clearing of the structure, moving the tag information 
**      into the first field in the structure, initializing the set of priority 
**      queues which form the basis of the internal scheduling algorithm, 
**      taking note of the appropriate set of system specific identification 
**      information (process id, site id, startup time, etc.).  Finally, the
**      personality information is placed into the system control area, and that 
**      area is marked as initialized.  The startup parameters are read from the
**	server startup program (server specific) via a mailbox.
**
**	The mailbox name is passed over as the process name (i.e., the process
**	name identifies the mailbox).  The server will find its process name,
**	assign a channel to it, then read the startup message.  In CSdispatch(),
**	when the server has completed initialization, the server will return
**	a message via its VMS termination channel to indicate that the server
**	has started up correctly.  The termination channel is used to allow
**	the server startup program to correctly catch VMS errors starting
**	the process (e.g. no more processes, insufficient privilege...)
**
**	Once the area has been so initialized, control is returned to the
**	administrative thread which is considered to be in control when
**	this routine is called.
**
** History:
*/
STATUS
CSinitiate(i4 *argc, char ***argv, CS_CB *ccb)
{
    i4                 i;
    i4			status = 0;
    CL_ERR_DESC		sys_err;
    CS_SCB		*idle_scb;
    CS_SYSTEM		*cssb = &Cs_srv_block;
    char		*sem_parms;
    char		*cp;

    if (cssb->cs_state)
	return(OK);

    MEfill(sizeof(Cs_srv_block), '\0', (PTR) cssb); 

#ifdef OS_THREADS_USED
    Cs_srv_block.cs_mt = TRUE;
    CS_synch_init(&Cs_srv_block.cs_semlist_mutex);

    cp = NULL;
    NMgtAt("II_THREAD_TYPE", &cp);
    if (cp && *cp)
    {
	if (STcasecmp(cp, "INTERNAL") == 0)
	{
#if defined (i64_vms)
	    char        buf[ER_MAX_LEN];
	    i4          msg_language = 0;
	    i4          msg_length;
	    i4          length;
	    CL_ERR_DESC error;

	    ERlangcode(NULL, &msg_language);
	    ERslookup(E_CL2557_CS_INTRN_THRDS_NOT_SUP, NULL, ER_TIMESTAMP,
                      NULL, buf, sizeof(buf), msg_language, &msg_length,
                      &error, 0, (ER_ARGUMENT *)0);
	    ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);
#else
	    Cs_srv_block.cs_mt = FALSE;
#endif /* i64_vms */
        }
    }

    if (Cs_srv_block.cs_mt)
        return CSMTinitiate(argc, argv , ccb);

    /* Initialize Ingres-threaded function vectors */
    Cs_fvp->IIp_semaphore = IICSp_semaphore;
    Cs_fvp->IIv_semaphore = IICSv_semaphore;
    Cs_fvp->IIi_semaphore = IICSi_semaphore;
    Cs_fvp->IIr_semaphore = IICSr_semaphore;
    Cs_fvp->IIs_semaphore = IICSs_semaphore;
    Cs_fvp->IIa_semaphore = IICSa_semaphore;
    Cs_fvp->IId_semaphore = IICSd_semaphore;
    Cs_fvp->IIn_semaphore = IICSn_semaphore;
    Cs_fvp->IIw_semaphore = IICSw_semaphore;

    Cs_fvp->IIget_sid = IICSget_sid;
    Cs_fvp->IIfind_sid = IICSfind_sid;
    Cs_fvp->IIset_sid = IICSset_sid;
    Cs_fvp->IIget_scb = IICSget_scb;
    Cs_fvp->IIfind_scb = IICSfind_scb;
    Cs_fvp->II_find_scb = IICS_find_scb;
    Cs_fvp->IIget_cpid = IICSget_cpid;
    Cs_fvp->IIswitch = IICSswitch;
    Cs_fvp->IIstatistics = IICSstatistics;
    Cs_fvp->IIadd_thread = IICSadd_thread;
    Cs_fvp->IIcancelled = IICScancelled;
    Cs_fvp->IIcnd_init = IICScnd_init;
    Cs_fvp->IIcnd_free = IICScnd_free;
    Cs_fvp->IIcnd_wait = IICScnd_wait;
    Cs_fvp->IIcnd_signal = IICScnd_signal;
    Cs_fvp->IIcnd_broadcast = IICScnd_broadcast;
    Cs_fvp->IIattn = IICSattn;
    Cs_fvp->IIdispatch = IICSdispatch;
    Cs_fvp->IIterminate = IICSterminate;
    Cs_fvp->IIintr_ack = IICSintr_ack;
    Cs_fvp->IIremove = IICSremove;
    Cs_fvp->IIaltr_session = IICSaltr_session;
    Cs_fvp->IIdump_statistics = IICSdump_statistics;
    Cs_fvp->IImonitor = IICSmonitor;
    Cs_fvp->IIsuspend = IICSsuspend;
    Cs_fvp->IIresume = IICSresume;
    Cs_fvp->IIresume_from_AST = IICSresume; 
    Cs_fvp->IIsuspend_for_AST = IICSsuspend_for_AST; 
    Cs_fvp->IIcancelCheck = IICScancelCheck; 

#endif /* OS_THREADS_USED */

    /*
    ** Store Process Id of this server process.
    */
    PCpid(&Cs_srv_block.cs_pid);

    cssb->cs_state = CS_INITIALIZING;

    if ((status = CS_cp_mbx_create(ccb ? ccb->cs_scnt : 0, &sys_err)) != OK)
	return (status);

    /*
    ** Figure out if this is a uniprocessor or a multiprocessor, and set
    ** Cs_numprocessors accordingly. This value is used by CSp_semaphore to
    ** determine whether multi process semaphores should spinloop or sleep
    */
    if (ii_sysconf(IISYSCONF_PROCESSORS, &Cs_numprocessors))
	Cs_numprocessors = 1;
#ifdef xDEBUG_PROCESSORS
    TRdisplay("CSinitiate: This machine appears to have %d processors\n",
		Cs_numprocessors);
#endif

    /*
    ** Allow the user to control multi-process semaphore behavior:
    **	    II_MAX_SEM_LOOPS		- controls Cs_max_sem_loops
    **	    II_DESCHED_USEC_SLEEP	- controls Cs_desched_usec_sleep
    **      Cs_sleep_ratio              - controls a ratio of sleep/switch sess
    **                                    +5 - 5 sleep  / 1 switch
    **                                    -5 - 5 switch / 1 sleep
    */
    if (ii_sysconf(IISYSCONF_MAX_SEM_LOOPS, &Cs_max_sem_loops))
	Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
    if (ii_sysconf(IISYSCONF_DESCHED_USEC_SLEEP, &Cs_desched_usec_sleep))
	Cs_desched_usec_sleep = DEF_DESCHED_USEC_SLEEP;
    NMgtAt("II_SLEEP_RATIO", &sem_parms);
    if (sem_parms && *sem_parms)
        (void)CVal(sem_parms, &Cs_sleep_ratio);
    if (Cs_desched_usec_sleep <= 10000)
        Cs_desched_usec_sleep = Cs_max_sem_loops * 10 * Cs_sleep_ratio;
    if (Cs_desched_usec_sleep <= 5000)
        Cs_desched_usec_sleep = 5000 * Cs_sleep_ratio;

    /* Let MO rip now, so non-thread stuff is visible in
       pseudo-servers, like semaphores and global stats  */

    CS_mo_init();
    (void) MOclassdef( MAXI2, CS_int_classes );

    if (ccb == NULL)
    {
	return(OK);
    }

#ifdef OS_THREADS_USED
    /* On Hybrid platforms (i.e. where OS and Internal threads are possible
    ** the CS*_semaphore function names are #define'd to the CS/CSMT function
    ** vector table, so don't use the "&" operator.
    */
    ERinit((i4)ER_SEMAPHORE, CSp_semaphore, CSv_semaphore,
           CSi_semaphore, CSn_semaphore);			/* {@fix_me@}
								** Add ASYNC
								** later
								*/
#else
    ERinit((i4)ER_SEMAPHORE, &CSp_semaphore, &CSv_semaphore,
           &CSi_semaphore, &CSn_semaphore);			/* {@fix_me@}
								** Add ASYNC
								** later
								*/
#endif

    /* Allow stack dumps to be printed to errlog.log    */
    Ex_print_stack = CS_dump_stack;
    Ex_diag_link = CSdiag_server_link;

    /* initialize all queues to empty */

    cssb->cs_known_list = &Cs_known_list_hdr;
    Cs_known_list_hdr.cs_next = Cs_known_list_hdr.cs_prev =
	&Cs_known_list_hdr;
    Cs_known_list_hdr.cs_type = CS_SCB_TYPE;
    Cs_known_list_hdr.cs_client_type = 0;
    Cs_known_list_hdr.cs_owner = (PTR)CS_IDENT;
    Cs_known_list_hdr.cs_tag = CS_TAG;
    Cs_known_list_hdr.cs_stk_area = 0;
    Cs_known_list_hdr.cs_stk_size = 0;
    Cs_known_list_hdr.cs_state = CS_COMPUTABLE;
    Cs_known_list_hdr.cs_priority = 0;
    Cs_known_list_hdr.cs_self = CS_SCB_HDR_ID;

    cssb->cs_wt_list = &Cs_wt_list_hdr;
    Cs_wt_list_hdr.cs_rw_q.cs_q_next =
	Cs_wt_list_hdr.cs_rw_q.cs_q_prev =
	    &Cs_wt_list_hdr;
    Cs_wt_list_hdr.cs_type = CS_SCB_TYPE;
    Cs_wt_list_hdr.cs_client_type = 0;
    Cs_wt_list_hdr.cs_owner = (PTR)CS_IDENT;
    Cs_wt_list_hdr.cs_tag = CS_TAG;
    Cs_wt_list_hdr.cs_stk_area = 0;
    Cs_wt_list_hdr.cs_stk_size = 0;
    Cs_wt_list_hdr.cs_state = CS_COMPUTABLE;
    Cs_wt_list_hdr.cs_priority = 0;
    Cs_wt_list_hdr.cs_self = CS_SCB_HDR_ID;

    cssb->cs_to_list = &Cs_to_list_hdr;
    Cs_to_list_hdr.cs_rw_q.cs_q_next =
	Cs_to_list_hdr.cs_rw_q.cs_q_prev =
	    &Cs_to_list_hdr;
    Cs_to_list_hdr.cs_type = CS_SCB_TYPE;
    Cs_to_list_hdr.cs_client_type = 0;
    Cs_to_list_hdr.cs_owner = (PTR)CS_IDENT;
    Cs_to_list_hdr.cs_tag = CS_TAG;
    Cs_to_list_hdr.cs_stk_area = 0;
    Cs_to_list_hdr.cs_stk_size = 0;
    Cs_to_list_hdr.cs_state = CS_COMPUTABLE;
    Cs_to_list_hdr.cs_priority = 0;
    Cs_to_list_hdr.cs_self = CS_SCB_HDR_ID;

    cssb->cs_as_list = &Cs_as_list_hdr;
    Cs_as_list_hdr.cs_as_q.cs_q_next =
	Cs_as_list_hdr.cs_as_q.cs_q_prev =
	    &Cs_as_list_hdr;
    Cs_as_list_hdr.cs_type = CS_SCB_TYPE;
    Cs_as_list_hdr.cs_client_type = 0;
    Cs_as_list_hdr.cs_owner = (PTR)CS_IDENT;
    Cs_as_list_hdr.cs_tag = CS_TAG;
    Cs_as_list_hdr.cs_stk_area = 0;
    Cs_as_list_hdr.cs_stk_size = 0;
    Cs_as_list_hdr.cs_state = CS_COMPUTABLE;
    Cs_as_list_hdr.cs_priority = 0;
    Cs_as_list_hdr.cs_self = CS_SCB_HDR_ID;

    for (i = 0; i < CS_LIM_PRIORITY; i++)
    {
	cssb->cs_rdy_que[i] = &Cs_queue_hdrs[i];
	Cs_queue_hdrs[i].cs_rw_q.cs_q_next =
	    Cs_queue_hdrs[i].cs_rw_q.cs_q_prev =
		&Cs_queue_hdrs[i];
	Cs_queue_hdrs[i].cs_type = CS_SCB_TYPE;
	Cs_queue_hdrs[i].cs_client_type = 0;
	Cs_queue_hdrs[i].cs_owner = (PTR)CS_IDENT;
	Cs_queue_hdrs[i].cs_tag = CS_TAG;
	Cs_queue_hdrs[i].cs_stk_area = 0;
	Cs_queue_hdrs[i].cs_stk_size = 0;
	Cs_queue_hdrs[i].cs_state = CS_COMPUTABLE;
	Cs_queue_hdrs[i].cs_priority = i;
	Cs_queue_hdrs[i].cs_self = CS_SCB_HDR_ID;

        CS_sched_quantum [i] = CS_NO_SCHED;
    }

    /* Set up known element trees for SCBs and Conditions.
       Semaphore were handled by CS_mo_init before the
       pseudo-server early return so there sems would
       be known. */

    SPinit( &Cs_srv_block.cs_scb_tree, CS_addrcmp );
    Cs_srv_block.cs_scb_ptree = &Cs_srv_block.cs_scb_tree;
    Cs_srv_block.cs_scb_tree.name = "cs_scb_tree";
    MOsptree_attach( Cs_srv_block.cs_scb_ptree );

    SPinit( &Cs_srv_block.cs_cnd_tree, CS_addrcmp );
    Cs_srv_block.cs_cnd_ptree = &Cs_srv_block.cs_cnd_tree;
    Cs_srv_block.cs_cnd_tree.name = "cs_cnd_tree";
    MOsptree_attach( Cs_srv_block.cs_cnd_ptree );

    /* copy over supplied information */

    cssb->cs_next_id = CS_FIRST_ID;	/* skip the special threads */
    cssb->cs_shutdown = ccb->cs_shutdown;
    cssb->cs_startup = ccb->cs_startup;	    /* for unix, mostly */
    cssb->cs_scballoc = ccb->cs_scballoc;
    cssb->cs_scbdealloc = ccb->cs_scbdealloc;
    cssb->cs_read = ccb->cs_read;
    cssb->cs_write = ccb->cs_write;
    cssb->cs_saddr = ccb->cs_saddr;
    cssb->cs_reject = ccb->cs_reject;
    cssb->cs_process = ccb->cs_process;
    cssb->cs_disconnect = ccb->cs_disconnect;
    cssb->cs_elog = ccb->cs_elog;
    cssb->cs_attn = ccb->cs_attn;
    cssb->cs_facility = ccb->cs_facility; 
    cssb->cs_format = ccb->cs_format;
    cssb->cs_diag = ccb->cs_diag;
    cssb->cs_max_sessions = ccb->cs_scnt;
    cssb->cs_max_active = ccb->cs_ascnt;
    cssb->cs_stksize = ccb->cs_stksize;
    cssb->cs_cursors = 16;
    cssb->cs_scbattach = ccb->cs_scbattach;
    cssb->cs_scbdetach = ccb->cs_scbdetach;
    cssb->cs_format_lkkey = ccb->cs_format_lkkey;

    /*
    ** Get CS options 
    */

    if (status = CSoptions( cssb ))
    	return (status);

    /*
    ** Init the EX facility to use our retrieval routines.  Can't be done
    ** until we know for sure we're going to be multi-threaded.
    */
    i_EX_initfunc( CSset_exsp, CSget_exsp );

    /*
    ** Now do the post option massaging of the option values.
    ** Round stack size to page size.
    */

    cssb->cs_stksize = (cssb->cs_stksize + 0x1ff) & ~0x1ff;

    /* Allow stack dumps to be printed to errlog.log    */
    Ex_print_stack = CS_dump_stack;

    /* Now allocate an SCB for the idle thread */
    idle_scb = &Cs_idle_scb;

    idle_scb->cs_type = CS_SCB_TYPE;
    idle_scb->cs_client_type = 0;
    idle_scb->cs_owner = (PTR)CS_IDENT;
    idle_scb->cs_tag = CS_TAG;
    idle_scb->cs_length = sizeof(CS_SCB);
    idle_scb->cs_stk_area = 0;
    idle_scb->cs_stk_size = 0;
    idle_scb->cs_state = CS_COMPUTABLE;
    idle_scb->cs_priority = CS_PIDLE;    /* idle = lowest priority */
    idle_scb->cs_thread_type = CS_INTRNL_THREAD;
    idle_scb->cs_self = (CS_SID)idle_scb;
    idle_scb->cs_pid = Cs_srv_block.cs_pid;
    MEmove( 12,
	    " <idle job> ",
	    ' ',
	    sizeof(idle_scb->cs_username),
	    idle_scb->cs_username);

    idle_scb->cs_rw_q.cs_q_next = cssb->cs_rdy_que[idle_scb->cs_priority];
    idle_scb->cs_rw_q.cs_q_prev =
	cssb->cs_rdy_que[idle_scb->cs_priority]->cs_rw_q.cs_q_prev;
    idle_scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = idle_scb;
    idle_scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = idle_scb;

    idle_scb->cs_next =
	cssb->cs_known_list->cs_next;
    idle_scb->cs_prev =
	cssb->cs_known_list->cs_prev;
    cssb->cs_known_list->cs_next = idle_scb;
    cssb->cs_known_list->cs_prev = idle_scb;
	/* the startup code is the null task (for now -- changes in	    */
	/*  CSdispatch()) */
    cssb->cs_current = idle_scb;

    CS_scb_attach( idle_scb );

    Cs_stk_list_hdr.cs_next = Cs_stk_list_hdr.cs_prev =
	&Cs_stk_list_hdr;
    Cs_stk_list_hdr.cs_size = 0;
    Cs_stk_list_hdr.cs_begin = Cs_stk_list_hdr.cs_end =
	    Cs_stk_list_hdr.cs_orig_sp = 0;
    Cs_stk_list_hdr.cs_used = (CS_SID)-1;
    Cs_stk_list_hdr.cs_ascii_id = CS_STK_TAG;
    Cs_stk_list_hdr.cs_type = CS_STK_TYPE;
    cssb->cs_stk_list = &Cs_stk_list_hdr;

    Cs_srv_block.cs_exhblock.exh$g_func = CS_exit_handler;
    Cs_srv_block.cs_exhblock.exh$l_argcount = 1;
    Cs_srv_block.cs_exhblock.exh$gl_value = &Cs_srv_block.cs_exhblock.exh$l_status;
    
    status = sys$dclexh(&Cs_srv_block.cs_exhblock);

    if (status & 1)
	return(OK);
    else
    {
	(*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &status, 0);
	sys$exit(status);
    }
}

/*{
** Name: CSterminate()	- Prepare to and terminate the server
**
** Description:
**      This routine is called to prepare for and/or perform the shutdown 
**      of the server.  This routine can be called to inform the server that
**      shutdown is pending so new connections should be disallowed, or that 
**      shutdown is to be performed immediately, regardless of state.  
**      The latter is intended to be used in cases of major failure's which 
**      prohibit the continued operation of the server.  The former is used in 
**      the normal course of events, when a request to shutdown has been issued 
**      from the monitor task.  There is, actually, a third category which is 
**      "between" these two, which is to shutdown immediately iff there are
**	no current threads. 
** 
**      This routine operates as follows.  If the request is to gradually shut
**      the server down, then the server state is marked as CS_SHUTDOWN.  This
**	will prevent any new users from connecting to the server, and will
**	cause this routine to be called again (with CS_START_CLOSE mode) when
**	the last user thread has exited.  At this point, if there are connected
**	user threads, control returns.
**
**	If there are no connected user threads, then each remaining thread is
**	a specialized server thread.  Each of these sessions is interrupted
**	with the CS_SHUTDOWN_EVENT.  This should cause the termination of any
**	server-controlled threads.  The server is then marked in CS_FINALSHUT
**	state. This state will cause the server to be terminated when the last
**	thread is exited.  At this point, if there are still server threads
**	active, control returns.
**
**	If there are no threads active, the server shutdown routine is called
**	and the process is terminated.
**
**	If CSterminate is called with CS_KILL, then any connected threads
**	are removed, the server shutdown routine is called and the process is
**	terminated.
**
**	If CSterminate is called with CS_CRASH mode, then the process is
**	just terminated.
**
** Inputs:
**      mode                            one of
**                                          CS_CLOSE
**                                          CS_CND_CLOSE
**					    CS_START_CLOSE
**					    CS_KILL
**					or  CS_CRASH
**                                      as described above.
**
** Outputs:
**      active_count                    filled with number of connected threads
**                                      if the CS_CLOSE or CS_CND_CLOSE are requested
**	Returns:
**	    Doesn't if successful
**	    E_CS003_NOT_QUIESCENT if there are still threads active.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-Oct-1986 (fred)
**          Initial Definition.
**      29-Oct-1987 (fred)
**          Fixed so that internal sessions will not be removed
**	20-sep-1988 (rogerk)
**	    Added CS_START_CLOSE closing state.
**	24-may-1993 (bryanp)
**	    Have CSterminate call PCexit, not sys$exit. This makes PCatexit()
**		work much better in server code.
**      06-aug-1997 (teresak)
**          Cross-integrate changes from Unix CL
**          02-may-1997 (canor01)
**            If a thread is created with the CS_CLEANUP_MASK set in
**            its thread_type, transfer the mask to the cs_cs_mask.
**            These threads will be terminated and allowed to finish
**            processing before any other system threads exit.
*/
STATUS
IICSterminate(
i4      mode,
i4      *active_count)
{
    i4                 status = 0;
    CS_SCB		*scb;
    CS_SCB		*scb_next;

    switch (mode)
    {
	case CS_CLOSE:
	    Cs_srv_block.cs_mask |= CS_SHUTDOWN_MASK;
	    /* fall thru -- difference is not marking the state */

	case CS_COND_CLOSE:
	    /*
	    ** If there are still user session active, then return -
	    ** server will shutdown when last user session exits.
	    ** If there are no more user sessions, fall through to the
	    ** CS_START_CLOSE state.
	    */
	    if (*active_count = Cs_srv_block.cs_user_sessions)
		return(E_CS0003_NOT_QUIESCENT);

	case CS_START_CLOSE:
            /*
            ** If there are any sessions with the CS_CLEANUP_MASK
            ** set, it means they need to finish their work
            ** before any of the other system sessions are told
            ** to shutdown.  When they exit, they will make
            ** another call to CSterminate() to signal the other
            ** system threads.
            ** Turn off AST's so the scb list will not be mucked with
            ** while we are cycling through it.
            */
	    sys$setast(0);

            for (scb = Cs_srv_block.cs_known_list->cs_prev;
                scb != Cs_srv_block.cs_known_list;
                scb = scb->cs_prev)
            {
                if (scb->cs_cs_mask & CS_CLEANUP_MASK)
                {
                    CSattn(CS_SHUTDOWN_EVENT, scb->cs_self);
                    sys$setast(1);
                    return (E_CS0003_NOT_QUIESCENT);
                }
            }
             /*
            ** Interrupt any server threads with the shutdown event -
            ** then return to caller.  Server should be shut down when
            ** last server thread has gone away.
            **
            ** Turn off AST's so the scb list will not be mucked with
            ** while we are cycling through it.
            */
	    for (scb = Cs_srv_block.cs_known_list->cs_next;
		scb != Cs_srv_block.cs_known_list;
		scb = scb->cs_next)
	    {
		if (scb != &Cs_idle_scb)
		    CSattn(CS_SHUTDOWN_EVENT, scb->cs_self);
	    }
	    /* 
	    ** any reference to the cs_mask shouldbe protected by in_kernel or
	    ** having AST's off
	    */
	    sys$setast(1);

	    /*
	    ** Return and wait for server tasks to terminate before shutting
	    ** down server.
	    */
	    Cs_srv_block.cs_mask |= CS_FINALSHUT_MASK;
	    if (Cs_srv_block.cs_num_sessions > 0)
		return(E_CS0003_NOT_QUIESCENT);

	case CS_KILL:
	case CS_CRASH:
	    Cs_srv_block.cs_state = CS_TERMINATING;
	    break;

	default:
	    return(E_CS0004_BAD_PARAMETER);
    }

    Cs_srv_block.cs_mask |= CS_NOSLICE_MASK;
    Cs_srv_block.cs_state = CS_INITIALIZING;	/* to force no sleeping */

    /* Dump server statistics */
    CSdump_statistics(TRdisplay);

    /*
    ** At this point, it is known that the request is to shutdown the server,
    ** and that that request should be honored.  Thus, proceed through the known
    ** thread list, terminating each thread as it is encountered.  If there
    ** are no known threads, as should normally be the case, this will simply
    ** be a very fast process.
    */

    status = 0;

    for (scb = Cs_srv_block.cs_known_list->cs_next,
	    scb_next = scb->cs_next;
	scb != Cs_srv_block.cs_known_list;
	scb = scb_next,
	    scb_next = scb->cs_next)
    {
	/* for each known thread, if it is not recognized, ask it to die */
	if ((scb->cs_self > 0) && (scb->cs_self < CS_FIRST_ID))
	    continue;
	if ((scb == &Cs_idle_scb) || (scb == &Cs_admin_scb))
	    continue;

	if (mode != CS_CRASH)
	{
	    status = CSremove((CS_SID)scb);
	}

	(*Cs_srv_block.cs_disconnect)(scb);

	if (status)
	{
	    (*Cs_srv_block.cs_elog)(status, 0, 0);
	}
    }
    if (mode != CS_CRASH)
    {
	status = (*Cs_srv_block.cs_shutdown)();
	if (status)
	    (*Cs_srv_block.cs_elog)(status, 0, 0);
	else
	    (*Cs_srv_block.cs_elog)(E_CS0018_NORMAL_SHUTDOWN, 0, 0);
    }
    sys$canexh(&Cs_srv_block.cs_exhblock);    
    PCexit(OK);
}

/*{
** Name: CSalter()	- Alter server operational characteristics
**
** Description:
**      This routine accepts basically the same set of operands as the CSinitiate() 
**      routine;  the arguments passed in here replace those specified to 
**      CSinitiate.  Only non-procedure arguments can be specified;  the processing 
**      routine, scb allocation and deallocation, and error logging routines 
**      cannot be altered. 
** 
**      Arguments are left unchanged by specifying the value CS_NOCHANGE. 
**      Values other than CS_NOCHANGE will be assumed as intending to replace 
**      the value currently in force.
**
** Inputs:
**      ccb                             the call control block, with
**      .cs_scnt			Number of threads allowed
**	.cs_ascnt			Nbr of active threads allowed
**	.cs_stksize			Size of stacks in bytes
**				All other fields are ignored.
**
** Outputs:
**      none
**	Returns:
**	    OK or 
**	    E_CS0004_BAD_PARAMETER
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-Oct-1986 (fred)
**          Initial Definition.
*/
STATUS
CSalter(ccb)
CS_CB              *ccb;
{
    i4			prop_scnt;
    i4			prop_ascnt;

    if ((ccb->cs_stksize != CS_NOCHANGE) && (ccb->cs_stksize < 512))
	return(E_CS0004_BAD_PARAMETER);

    if (ccb->cs_scnt != CS_NOCHANGE || ccb->cs_ascnt != CS_NOCHANGE)
    {
	if (ccb->cs_scnt != CS_NOCHANGE)
	    prop_scnt = ccb->cs_scnt;
	else
	    prop_scnt = Cs_srv_block.cs_max_sessions;
	if (ccb->cs_ascnt != CS_NOCHANGE)
	    prop_ascnt = ccb->cs_ascnt;
	else
	    prop_ascnt = Cs_srv_block.cs_max_active;

	/* If trying to change these and they are invalid */
	if (prop_scnt < prop_ascnt)
	    return(E_CS0004_BAD_PARAMETER);
    }

    if (ccb->cs_stksize != CS_NOCHANGE)
	Cs_srv_block.cs_stksize = ccb->cs_stksize;
    if (ccb->cs_scnt != CS_NOCHANGE)
	Cs_srv_block.cs_max_sessions = ccb->cs_scnt;
    if (ccb->cs_ascnt != CS_NOCHANGE)
	Cs_srv_block.cs_max_active = ccb->cs_ascnt;
    return(OK);
}

/*{
** Name: CSdispatch()	- Begin multi-user operation
**
** Description:
**      This routine is called to actually begin the operation of the 
**      server.  It is separated from CSinitiate() so that the server 
**      code can regain control in the middle to perform any logging or 
**      runtime initialization which it desires to do, or to allow the
**	initiate code to be called once while dispatch is called repeatedly
**	as is the case in some environments.
**
**      The routine operates as follows.  First the dispatcher begins
**	to initialize itself to be ready to add threads.  It builds
**	session control blocks for the dispatcher special threads (if any).
**	At the moment, this is limited to the administrative thread, which
**	is used to start and stop threads.
**
**	When the dispatcher is ready to accept addthread requests, it calls
**      the user supplied initialize server routine (no parameters). 
**      It is assumed that a non-zero return indicates failure.  In the 
**      event of failure, the error returned is logged, along with the 
**      a message indicating failure to start up.  Then the user supplied 
**      termination routine is called, thus allowing any necessary cleanup 
**      to occur. 
** 
**      If the initiation routine succeeds (i.e. returns zero (0)), then the 
**      communication subsystem is called to begin operation.
** 
**	The dispatcher then begins operation by causing the idle job to be
**	resumed.  The idle job is a thread like any other which operates at
**	a low priority, sleeping when the server has nothing else to do.
**
**      If the dispatcher/idle thread notices that something nasty has occurred,
**	then it will break out of its normal operational loop.  Here, there is
**	nothing to do but shutdown, so the user supplied shutdown routine is
**	called, and a message logged that the server is dying off.  Then, the 
**      exit handler is deleted and the process exits.
** 
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    Never, under normal circumstances
**	    Should a call be made to CSdispatch() before the required
**	    call to CSinitiate() is made, an E_CS0005_NO_INITIATE is
**	    returned.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-Oct-1986 (fred)
**          Created.
**	26-sep-1988 (rogerk)
**	    Fill in new cs_info_cp priority fields.
**	20-oct-1988 (rogerk)
**	    Turn off ast's before calling CS_quantum, not after.
**	31-oct-1988 (rogerk)
**	    Don't put startup options that are CS specific (those in which
**	    cso_facility == CSO_F_CS) into the server's startup option list.
**	 7-nov-1988 (rogerk)
**	    Call CS_ssprsm directly instead of through an AST.
**	28-feb-1989 (rogerk)
**	    Set cs_pid in admin thread scb.
**      23-may-1989 (fred)
**          Changed order of server startup operations.  Now, AST's are enabled
**	    while the underlying mainline code initiate function is called.
**	    During this time, the Cs_srv_block.cs_state value is
**	    CS_INITIALIZING.  This allows the server startup code to suspend
**	    and/or resume as necessary (i/o, locks, etc.).  This fix is
**	    necessary to allow the VAXCluster CS to operate correctly in the
**	    presence of shared global caches and user defined ADT's.  The
**	    creation of the quantum AST is also postponed till just before real
**	    dispatching begins.  This keeps the quantum from interfering with
**	    server startup.  It also precludes us from having to put code in
**	    CS_quantum() to not timeslice until the server is up...
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**	7-oct-1992 (bryanp)
**	    If this is the Recovery Server, set the process name to DMFRCPxx.
**	29-Oct-1992 (daveb)
**	    Add MUset_funcs call before threading.
**	24-nov-92 (walt)
**	    Moved definition of proc_desc down into this function so it wouldn't
**	    be a global symbol.
**	26-apr-93 (walt)
**	    Fixed cut-and-paste error from prev integration.  The initial return
**	    address for the admin thread (....[CS_ALF_RA]) was missing the
**	    righthand side of its assignment statement.  The next statement in
**	    the flow was also an assignment statement.  So the admin thread's
**	    startup return address was being set to 3 (=CS_OUTPUT) by
**	    syntactically being part of a continued assignment statment.
**	25-oct-1993 (bryanp)
**	    Repair arguments to SYS$CRELNM -- the itemlist structure was using
**		ints where it meant shorts.
**	25-apr-1994 (bryanp) B57419
**	    If cs_define is set, indicating that the caller wishes us to define
**		the logical name II_DBMS_SERVER automatically, then check to
**		see whether we are a subprocess or a detached process. If we are
**		a subprocess, define the logical name in the LNM$JOB table,
**		otherwise define the logical name in the LNM$SYSTEM table. This
**		restores some functionality accidentally broken by the pm-izing
**		of iirundbms.
**	20-jul-95 (dougb)
**	    Make a few other tests before defining II_DBMS_SERVER in the system
**	    logical name table.  The JOBTYPE may be DETACHED for *any* process
**	    running from a DECNET DECterm.  Queue the Admin and Repent scb's
**	    together to assist the "show internal sessions" monitor command.
**	    Make queue inserts consistent throughout.
**	07-dec-95 (dougb)
**	    Call cs_elog routine with a pointer to the status -- avoid ACCVIO.
**	    Use Cs_save_exsp to ensure EX handler stack remains consistent
**	    for all threads.
**	02-jan-96 (dougb)
**	    Attach the admin and repent threads when we're almost ready to go.
**	    Init csib.cs_name just to be on the safe side (done on Unix).
**	24-jan-96 (dougb) bug 71590
**	    Correct prototype for CS_last_chance().  Don't set cs_registers[]
**	    in this module -- that's done for us by CS_alloc_stack().
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**
**  Design:
**
**	The server begins as follows:
**	the system state is set to idle, ast's are enabled, and
**	Control is transfered to the idle thread, which is a special case
**	in CSHL\CS_setup(). Here, the VMS service sys$hiber() is called.
**	At this point the code is quite simple.  The remainder constitutes the  
**      idle task, which is a task of very, very low priority.  It normally 
**      sits hibernating.  When events in the system occur, AST's are delivered 
**      to the process.  These notice that a hibernate has taken place, and then 
**      awaken the process (via sys$wake()). 
** 
**      Thus, the remainder of the idle thread code is to enable user mode 
**      ast's, set the state to idle, and hibernate.  Upon reawakening, 
**      it is known that the AST level routine which awakened the process, 
**      must have placed something to do in queue, so an AST is declared 
**      to change the operating thread.  At some point, the idle task 
**      will again become eligible for execution, and control will return 
**      to the point after the $DCLAST() call.  AT this point, some checks 
**      are made to assure us that nothing untoward has happened (such as 
**      a request for shutdown having happened).  Assuming that nothing out 
**      of the ordinary has occurred, control returns to the top of the loop 
**      (which is the top of this paragraph), and the idle task takes the 
**      process out of competition for system resources. 
*/
STATUS
IICSdispatch()
{
    STATUS              status = 0;
    STATUS		ret_val;
    i4			i;
    int                 jobtype;
    int			master_pid;
    IOSB		iosb;
    ILE3		jpiget[] = {
	{ sizeof(jobtype), JPI$_JOBTYPE, &jobtype, 0 },
	{ sizeof master_pid, JPI$_MASTER_PID, &master_pid, 0 },
	{ 0, 0, 0, 0 }
    };
    EX_CONTEXT		excontext;
    CS_INFO_CB		csib;
    FUNC_EXTERN	STATUS	cs_handler();
    FUNC_EXTERN int	Cs_last_chance();
    bool		recovery_server = FALSE;


    if (Cs_srv_block.cs_process == 0)
    {
	return(FAIL);
    }

    if (EXdeclare(cs_handler, &excontext) != OK)
    {
	if (!got_ex++)
	{
	    (*Cs_srv_block.cs_elog)(E_CS0014_ESCAPED_EXCPTN, 0, 0);
	}
	EXdelete();
	return(E_CS0014_ESCAPED_EXCPTN);
    }
	
/*
**  For Alpha move the CS_cactus_save call to this earlier spot so the results 
**  will be available to CS_alloc_stack when the initial threads are built 
**  below.
*/
#ifdef ALPHA
    CS_cactus_save();	/* set up for VMS Debugging */
#endif

    for (;;)
    {
	/*									    */
	/*	Here, we declare a last chance exception handler.  This handler	    */
	/*	is called when either all handlers on the stack have 'ignored' the  */
	/*	problem, or when the stack is not in any shape (e.g. is exhausted)  */
	/*	to take any more handlers.  This handler will kill off the current   */
	/*	session (as that one is dangerous), note that a stack fault	    */
	/*	occurred, and go back to sleep */

	status = sys$setexv(2, /* means last chance -- no VMS constants */
		     Cs_last_chance, 
		     PSL$C_USER, 
		     &Cs_old_last_chance); /* usually dbg's, but calling it */
					   /* hoses the server (dbg does)   */
	if ((status & 1) == 0)
	    break;

        status = sys$gettim(Cs_srv_block.cs_int_time);
        if ((status & 1) == 0)
            break;
	/*
	** At this point, we are about ready to go.  To get going,
	** we call the timer AST to have it set itself up to be
	** moving along.  To do this, the binary time representation
	** of the timer quantum must first be placed into the system
	** control block.  This time, which may be altered, is used
	** by the timer to requeue itself for the switching threads
	** when they time out.
	*/

	{
	    struct dsc$descriptor_s	delta_time;
	      
	    if (STbcompare("none", 4,
			    Cs_srv_block.cs_aquantum, Cs_srv_block.cs_aq_length,
			    TRUE) == 0)
	    {
		/* Default timer quantum is 1 second */
		
		MEcopy("0 00:00:01.00", 14, Cs_srv_block.cs_aquantum);
		Cs_srv_block.cs_aq_length = 13;
		Cs_srv_block.cs_mask |= CS_NOSLICE_MASK;
                Cs_srv_block.cs_gpercent = 0;
	    }
	    delta_time.dsc$b_dtype = DSC$K_DTYPE_T;
	    delta_time.dsc$b_class = DSC$K_CLASS_S;
	    delta_time.dsc$w_length = Cs_srv_block.cs_aq_length;
	    delta_time.dsc$a_pointer = Cs_srv_block.cs_aquantum;

	    status = sys$bintim(&delta_time, Cs_srv_block.cs_bquantum);
	    if ((status & 1) == 0)
	    {
		char	buffer[16];
		
		if (Cs_srv_block.cs_aq_length < (sizeof(buffer) - 2))
		{
		    MEcopy("0 ", 2, buffer);
		    MEcopy((char *)Cs_srv_block.cs_aquantum,
			    Cs_srv_block.cs_aq_length,
			    &buffer[2]);
		    MEcopy(buffer,
				Cs_srv_block.cs_aq_length + 2,
				Cs_srv_block.cs_aquantum);
		    Cs_srv_block.cs_aq_length += 2;	/* sizeof("0 ") */
		    delta_time.dsc$w_length = Cs_srv_block.cs_aq_length;
		    status = sys$bintim(&delta_time, Cs_srv_block.cs_bquantum);
		}
	    }
		
	    if ((status & 1) == 0)
		break;

	    if ((Cs_srv_block.cs_bquantum[1] == -1)
	      && (Cs_srv_block.cs_bquantum[0] >= CS_ONE_SECOND))
	    {
		Cs_srv_block.cs_toq_cnt = 
		    Cs_srv_block.cs_q_per_sec =
			CS_ONE_SECOND / Cs_srv_block.cs_bquantum[0];
	    }
	    else if (Cs_srv_block.cs_bquantum[1] == -1)
	    {
		/*
		** In this case, the quantum is longer than one second.
		** Therefore, we compute the number of seconds per quantum,
		** and use this value to scale the timeout queue
		*/
		Cs_srv_block.cs_toq_cnt = 
		    Cs_srv_block.cs_q_per_sec = 
			- (Cs_srv_block.cs_bquantum[0] / CS_ONE_SECOND);
                Cs_srv_block.cs_mask |= CS_LONGQUANTUM_MASK;
                if (Cs_srv_block.cs_gpercent)
                {
                    (*Cs_srv_block.cs_elog)(E_CS0010_QUANTUM_INVALID, 0, 0);
                    EXdelete();
                    return(E_CS0010_QUANTUM_INVALID);
                }
	    }
	    else
	    {
		/* if the time quantum is much greater than one second,	    */
		/* then I didn't want to figure out 8 byte arithmetic,	    */
		/* so I'm declaring these illegal.  If you time quantum	    */
		/* is that long, then why bother to have one.		    */
		/* This period is somewhat longer than seven (7) minutes.   */

		(*Cs_srv_block.cs_elog)(E_CS0010_QUANTUM_INVALID, 0, 0);
		EXdelete();
		return(E_CS0010_QUANTUM_INVALID);
	    }
	}

	/*
	** Set up Cs_save_exsp so that every thread (not just the current,
	** idle thread) sees the handlers for the top portion of the stack.
	*/
	Cs_save_exsp = (PTR)*CSget_exsp();

	{
	    CL_ERR_DESC	    error;

	    /*
	    ** NOTE: Admin thread may always reside on the ready queue for its
	    ** priority, but it is not always runable.  In fact, it won't run
	    ** initially.  Previous CS_COMPUTABLE state was confusing.
	    */
	    Cs_admin_scb.csa_scb.cs_type = CS_SCB_TYPE;
	    Cs_admin_scb.csa_scb.cs_client_type = 0;
	    Cs_admin_scb.csa_scb.cs_length = sizeof(CS_ADMIN_SCB);
	    Cs_admin_scb.csa_scb.cs_owner = (PTR)CS_IDENT;
	    Cs_admin_scb.csa_scb.cs_tag = CS_TAG;
	    Cs_admin_scb.csa_scb.cs_state = CS_UWAIT;
	    Cs_admin_scb.csa_scb.cs_priority = CS_PADMIN;
	    Cs_admin_scb.csa_scb.cs_thread_type = CS_INTRNL_THREAD;
	    Cs_admin_scb.csa_scb.cs_self = (CS_SID)&Cs_admin_scb;
	    Cs_admin_scb.csa_scb.cs_pid = Cs_srv_block.cs_pid;

	    /* Extra help for new "show internal sessions" monitor command. */
	    MEmove( 17, " <Administrator> ", ' ',
		   sizeof Cs_admin_scb.csa_scb.cs_username,
		   Cs_admin_scb.csa_scb.cs_username );
	    Cs_admin_scb.csa_scb.cs_next = &Cs_repent_scb;

	    ret_val = CS_alloc_stack(&Cs_admin_scb, error);
	    if (ret_val != OK)
	    {
		EXdelete();
		return(ret_val);
	    }

	    CS_scb_attach( &Cs_admin_scb.csa_scb );

	    /*
	    ** Now the thread is complete, place it on the appropriate queue
	    ** and let it run via the normal course of events.
	    */

	    Cs_admin_scb.csa_scb.cs_mode = CS_OUTPUT;
	    Cs_admin_scb.csa_scb.cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[Cs_admin_scb.csa_scb.cs_priority];
	    Cs_admin_scb.csa_scb.cs_rw_q.cs_q_prev =
  Cs_srv_block.cs_rdy_que[Cs_admin_scb.csa_scb.cs_priority]->cs_rw_q.cs_q_prev;
	    Cs_admin_scb.csa_scb.cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		&Cs_admin_scb;
	    Cs_admin_scb.csa_scb.cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		&Cs_admin_scb;
	    /*								    */
	    /*	Now we must allocate the same stuff for the idle job.	    */
	    /*	The idle job runs on its own stack (smaller) rather than    */
	    /*	on the VMS default stack.  This allows us to recover from   */
	    /*	the case where a session overflows its stack and VMS	    */
	    /*	(kindly) resets us to our ``normal'' (what do they know in  */
	    /*	Spitbrook) user mode stack.  By running on our own stack,   */
	    /*	when this happens, we recover by trashing the offending	    */
	    /*	thread, and letting nature take its course.		    */

	    ret_val = CS_alloc_stack(&Cs_idle_scb, error);
	    if (ret_val != OK)
	    {
		EXdelete();
		return(ret_val);
	    }
	    /*
	    **	Now allocate the repent session.  This is used when CPU
	    **	governing is operational
	    */
	    Cs_repent_scb.cs_type = CS_SCB_TYPE;
	    Cs_repent_scb.cs_length = sizeof(CS_SCB);
	    Cs_repent_scb.cs_owner = (PTR)CS_IDENT;
	    Cs_repent_scb.cs_tag = CS_TAG;
	    Cs_repent_scb.cs_state = CS_UWAIT;
	    Cs_repent_scb.cs_priority = CS_DEFPRIORITY + 1;
	    Cs_repent_scb.cs_thread_type = CS_INTRNL_THREAD;
	    Cs_repent_scb.cs_self = (CS_SID)&Cs_repent_scb;
	    Cs_repent_scb.cs_pid = Cs_srv_block.cs_pid;
	    MEmove( 15,
		    " <Time Bandit> ",
		    ' ',
		    sizeof(Cs_repent_scb.cs_username),
		    Cs_repent_scb.cs_username);

	    /* Extra help for new "show internal sessions" monitor command. */
	    Cs_repent_scb.cs_next = &Cs_idle_scb;

	    ret_val = CS_alloc_stack(&Cs_repent_scb, error);
	    if (ret_val != OK)
	    {
		EXdelete();
		return(ret_val);
	    }

	    CS_scb_attach( &Cs_repent_scb );

	    /*
	    ** Now the thread is complete, place it on the appropriate queue
	    ** and let it run via the normal course of events.
	    */

	    Cs_repent_scb.cs_mode = CS_OUTPUT;
	    Cs_repent_scb.cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[Cs_repent_scb.cs_priority];
	    Cs_repent_scb.cs_rw_q.cs_q_prev =
	Cs_srv_block.cs_rdy_que[Cs_repent_scb.cs_priority]->cs_rw_q.cs_q_prev;
	    Cs_repent_scb.cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		&Cs_repent_scb;
	    Cs_repent_scb.cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		&Cs_repent_scb;
		/* the startup code is the null task (for now -- changes in */
		/*  CSdispatch()) */
	    Cs_srv_block.cs_stk_count = 0;	/* Ignore system stacks	    */

	    /*
	    ** Now the thread is complete, place it on the appropriate queue
	    ** and let it run via the normal course of events.
	    **
	    ** Set CS state to IDLING so a wakeup will be signaled if
	    ** CSadd_thread calls are made during server startup.
	    */

	    Cs_idle_scb.cs_mode = CS_COMPUTABLE;
	}

	FP_set_callback( CSget_scb );

	/*
	** Make sure CS is ready at this point to start accepting
	** CSadd_thread calls.  The server startup routines may need
	** to start specialized threads to run dbms tasks.
	*/

	Cs_srv_block.cs_ready_mask = (CS_PRIORITY_BIT >> CS_PIDLE);

	/*
	** Call server startup routines.
	** The server startup routine may call CSadd_thread to create
	** specialized sessions.
	*/
	csib.cs_scnt = Cs_srv_block.cs_max_sessions;
	csib.cs_ascnt = Cs_srv_block.cs_max_active;
	csib.cs_min_priority = CS_MINPRIORITY;
	csib.cs_max_priority = CS_MAXPRIORITY;
	csib.cs_norm_priority = CS_DEFPRIORITY;
	csib.cs_name [0] = EOS;

	ret_val = (*Cs_srv_block.cs_startup)(&csib);
	if (ret_val != OK)
	    break;
	STlcopy(csib.cs_name, Cs_srv_block.cs_name,
	    sizeof(Cs_srv_block.cs_name)-1);

	status = sys$getjpiw(EFN$C_ENF, 0, 0, jpiget, &iosb, 0, 0);
	if (status & 1)
	    status = iosb.iosb$w_status;
	if ((status & 1) == 0)
	    sys$exit(status);

	if( Cs_srv_block.cs_define )
	{
	    static $DESCRIPTOR( jobtabnam, "LNM$JOB" );
            static $DESCRIPTOR( systabnam, "LNM$SYSTEM" );
	    static $DESCRIPTOR( lognam, "II_DBMS_SERVER" );
	    ILE3 itmlst[2] = { { 0, LNM$_STRING, NULL, NULL },
			    { 0, 0, NULL, NULL } };

	    itmlst[0].ile3$w_length = STlength( csib.cs_name );
	    itmlst[0].ile3$ps_bufaddr = csib.cs_name;

	    /*
	    ** Define a system logical only when we are currently running
	    ** detached (or, in a bogus DETNET DECterm session!) and at the
	    ** top of the current job (not a process spawned by the debugger
	    ** or iirundbms).
	    */
	    if ( JPI$K_DETACHED == jobtype
		&& Cs_srv_block.cs_pid == master_pid )
		status = sys$crelnm( NULL, &systabnam, &lognam, NULL, itmlst );
	    else
		status = sys$crelnm( NULL, &jobtabnam, &lognam, NULL, itmlst );
	    if ( !(status & 1) )
		sys$exit( status );
	}

	/* Memory pointers are saved and restored, because mainline code    */
	/* depends on this.  On VMS systems, this dependency is  	    */
	/* unnecessary, but it is necessary on MVS where all the threads    */
	/* are really separate tasks sharing an address space.  Possibly    */
	/* useful in some UNIX implementations as well.			    */

	Cs_srv_block.cs_svcb = csib.cs_svcb;	    /* save pointer to mem */


                /*
                ** Save dynamic stuff we've got from sequencer
                */
                Cs_srv_block.sc_main_sz  = csib.sc_main_sz;
                Cs_srv_block.scd_scb_sz =  csib.scd_scb_sz;
                Cs_srv_block.scb_typ_off = csib.scb_typ_off;
                Cs_srv_block.scb_fac_off = csib.scb_fac_off;
                Cs_srv_block.scb_usr_off = csib.scb_usr_off;
                Cs_srv_block.scb_qry_off = csib.scb_qry_off;

                /*
                ** Print ad-hoc debugging info to dbms log
                */
TRdisplay("->\n^^^\n");
TRdisplay("The server startup: pid= ^^^ %d\n", Cs_srv_block.cs_pid);
TRdisplay("             connect_id= ^^^ %s\n", Cs_srv_block.cs_name);
TRdisplay("The Server Block         ^^^ @%0p, size is ^^^ %d (%x hex)\n",
                 &Cs_srv_block, (long)&Cs_srv_block.srv_blk_end - (long)&Cs_srv_block,
                                (long)&Cs_srv_block.srv_blk_end - (long)&Cs_srv_block);
TRdisplay("Current thread pointer   ^^^ @%0p\n", Cs_srv_block.cs_current);
TRdisplay("Max # of active sessions ^^^ %d\n", Cs_srv_block.cs_max_active);
TRdisplay("The thread stack size is ^^^ %d\n", Cs_srv_block.cs_stksize);
TRdisplay("Total # of sessions      ^^^ %d\n", Cs_srv_block.cs_num_sessions);
TRdisplay("# of user sessions       ^^^ %d\n", Cs_srv_block.cs_user_sessions);
TRdisplay("HWM of active threads    ^^^ %d\n", Cs_srv_block.cs_hwm_active);
TRdisplay("Current # of act.threads ^^^ %d\n", Cs_srv_block.cs_num_active);
TRdisplay("Curr. # of stacks alloc. ^^^ %d\n", Cs_srv_block.cs_stk_count);
TRdisplay("Time-out event wait list ^^^ @%0p\n", Cs_srv_block.cs_to_list);
TRdisplay("Non time-out event wait  ^^^ @%0p\n", Cs_srv_block.cs_wt_list);
TRdisplay("Global state of the sys. ^^^ %0x\n", Cs_srv_block.cs_state);
TRdisplay(" 00-uninitialized, 10-initializing, 20-processing, 25-switching,\n");
TRdisplay(" 30-idling, 40-closing, 50-terminating, 60-in error\n");
TRdisplay("In kernel indicator      ^^^ %d\n", Cs_srv_block.cs_inkernel);
TRdisplay("Async indicator          ^^^ %d\n", Cs_srv_block.cs_async);
TRdisplay("->->\n");
TRdisplay("The Sequencer Block      ^^^ @%0p,  size is ^^^ %d (%x hex)\n",
        Cs_srv_block.cs_svcb, Cs_srv_block.sc_main_sz, Cs_srv_block.sc_main_sz);
TRdisplay("->->->\n");
TRdisplay("The Session Control Block (SCD_SCB) size is ^^^ %d (%x hex)\n",
            Cs_srv_block.scd_scb_sz, Cs_srv_block.scd_scb_sz);
TRdisplay("... session_type offset  ^^^ %d (%x hex)\n",
            Cs_srv_block.scb_typ_off, Cs_srv_block.scb_typ_off);
TRdisplay(" 00-user session, 01-monitor session, 02-fast commit thread,\n");
TRdisplay(" 03-write behind thread, 04-server initiator, 05-event thread,\n");
TRdisplay(" 06-2PC recovery, 07-recovery thread, 08-log-writer thread,\n");
TRdisplay(" 09-dead process detector, 10-group commit, 11-force abort,\n");
TRdisplay(" 12-audit thread, 13-consistency point, 14-terminator thread,\n");
TRdisplay(" 15-security audit, 16-write-along thread, 17-read-ahead thread\n");
TRdisplay("... current facility off ^^^ %d (%x hex)\n",
            Cs_srv_block.scb_fac_off, Cs_srv_block.scb_fac_off);
TRdisplay(" 0-None, 1-CLF, 2-ADF, 3-DMF, 4-OPF, 5-PSF, 6-QSF, 7-RDF,\n");
TRdisplay(" 8-SCF, 9-ULF, 10-DUF, 11-GCF, 12-RQF, 13-TPF, 14-GWF, 15-SXF\n");
TRdisplay("... user info offset is  ^^^ %d (%x hex)\n",
            Cs_srv_block.scb_usr_off, Cs_srv_block.scb_usr_off);
TRdisplay("... query text offset is ^^^ %d (%x hex)\n",
        Cs_srv_block.scb_qry_off, Cs_srv_block.scb_qry_off);
TRdisplay("... current activity off ^^^ %d (%x hex)\n",
            Cs_srv_block.scb_act_off, Cs_srv_block.scb_act_off);
TRdisplay("... alert status offset  ^^^ %d (%x hex)\n",
            Cs_srv_block.scb_ast_off, Cs_srv_block.scb_ast_off);
TRdisplay(" 0-default, 1-wait for interrupt(event), 2-thread was interrupted,");
TRdisplay("\n 3-send pending, 4-ignore GCA completion\n");
TRdisplay("->->->->\n");



	/*
	** Turn off ast's before calling CS_quantum so that quantum AST
	** will not be handled until we are ready for it.
	*/
	sys$setast(0);
	ret_val = CS_quantum(0);
	if (ret_val)
	    break;

	CS_rcv_request();    /* set up to be adding thread's */

	status = 1;
	ret_val = 0;

	/*
	** ??? Setting cs_current to NULL thread is not necessary for
	** ??? CS_ssprsm().  Is this still required for AST code which runs
	** ??? in the window from here 'til CS_xchng_thread() is called for
	** ??? the first time?  (Outstanding AST's include the quantum timer
	** ??? and the communications request.)  Might be OK to leave
	** ??? cs_current as idle_scb and pass TRUE to CS_ssprsm() below.
	*/
	Cs_srv_block.cs_current = NULL;
	Cs_srv_block.cs_state = CS_IDLING;
    	sys$setast(1);	    /* Turn AST's on */

	/* Committed; set up right semaphores for MU */
	MUset_funcs( MUcs_sems() );

#ifdef KPS_THREADS
        while (TRUE)
        {
            status = thread_scheduler();
            if (status)
                break;
        }
#else
	CS_ssprsm( FALSE );
#endif

	(*Cs_srv_block.cs_elog)(E_CS00FE_RET_FROM_IDLE, 0, 0);
	ret_val = E_CS00FF_FATAL_ERROR;
	break;
    }
    if ( ((status & 1) == 0) || (ret_val) )
    {
	if ((status & 1) != 0)
	{
	    status = 0;
	}
	(*Cs_srv_block.cs_elog)(ret_val, &status, 0);
    }

    Cs_srv_block.cs_state = CS_CLOSING;

    CSterminate(CS_KILL, &i);

    sys$canexh(&Cs_srv_block.cs_exhblock);
    sys$exit(((status & 1) || (ret_val == 0)) ? SS$_NORMAL : status);
}

#ifdef KPS_THREADS
static STATUS
thread_scheduler()
{
    i4 sts;
    CS_SCB *scb;

    /* set the inkernel flag */
    Cs_srv_block.cs_inkernel = TRUE;

    scb = pick_new_thread(Cs_srv_block.cs_current, Cs_srv_block.cs_ready_mask);

    /* deal with any async stuff from an AST that may have arrived */

    /* now clear the inkernel flag */
    Cs_srv_block.cs_inkernel = FALSE;

    if (Cs_srv_block.cs_async)
        Cs_move_async(scb);

    /* clear the quantum tick counter */
    Cs_lastquant = 0; 

    /*
    ** If the valid bit isn't set then we either haven't yet started the
    ** thread or the thread has ended,
    ** otherwise we must be resuming this thread
    **
    **  +------------------+-----------+------------+
    **  | Thread_state     | valid_bit | active_bit |
    **  +------------------+-----------+------------+
    **  | not yet started  | 0         | 0          |
    **  +------------------+-----------+------------+
    **  | running          | 1         | 1          |
    **  +------------------+-----------+------------+
    **  | stalled          | 1         | 0          |
    **  +------------------+-----------+------------+
    **  | ended            | 0         | 0          |
    **  +------------------+-----------+------------+
    **  
    ** When a thread has been started there are only 2 valid KP calls that can
    ** be made from that thread, viz EXE$KP_STALL_GENERAL and EXE$KP_END
    ** These two calls will return to the PC immediately after the call to 
    ** EXE$KP_START or EXE$KP_RESTART within this thread (typically that will
    ** be after exe$kp_restart).
    ** If the return is due to a call to exe$kp_end then the KPB and stacks
    ** for the previous thread will no longer be valid - and no further
    ** reference can be made to them. The call to exe$kp_end is made within
    ** the context of the current thread in the function CS_eradicate. Note
    ** that we cannot delete one thread from another thread.
    ** The "dead" thread's SCB will have been removed from the set of
    ** schedulable threads so it will never be restarted again within this
    ** function.
    */
    if (scb->kpb->kpb$v_valid && !scb->kpb->kpb$v_active)
    {
        /*
        ** we assume that the thread is valid and inactive so this must be
        ** a request to resume the thread
        */
        sts = exe$kp_restart(scb->kpb, SS$_NORMAL);
    }
    else
    {
        /*
        ** If the valid bit is clear then this kpb has not been used for a call
        **  to exe$kp_start yet (we assume that the inactive bit is clear as
        **  well - since we should never be starting a dead thread)
        **
        ** The thread entry point is whatever Cs_alloc_stack setup for us
        */
        sts = exe$kp_start(scb->kpb, scb->cs_registers[CS_ALF_PV],
                           (__int64)KPREG$K_HLL_REG_MASK);
    }
    /*
    ** When the thread next suspends (or ends) this is where it will return
    ** to.  If the thread ends then the SCB will already have been removed
    ** from the queue so we should never attempt to restart an SCB here 
    */ 
    
    return (sts & 1) == 0 ? FAIL : OK;
}
#endif

/*{
** Name: CSsuspend	- Suspend current thread pending some event
**
** Description:
**      This procedure is used to suspend activity for a particular 
**      thread pending some event which will be delivered via a 
**      CSresume() call.  Optionally, this call will also schedule 
**      timeout notification.  If the event is to be resumed following 
**      an interrupt, then the CS_INTERRUPT_MASK should be specified.
**
** Inputs:
**      mask                            Mask of events which can terminate
**                                      the suspension.  Must be a mask of
**                                        CS_INTERRUPT_MASK - can be interrupted
**                                        CS_TIMEOUT_MASK - event will time out
**					along with the reason for suspension:
**                                      (CS_DIO_MASK    -- Ingres disk I/O
**                                       CS_BIO_MASK    -- InterProcess Comm.
**                                       CS_LOCK_MASK   -- LK lock wait
**                                       CS_GWFIO_MASK  -- Gateway suspension
**                                       <none of above>-- Logging system wait
**                                      )
**      to_cnt	                        number of seconds after which event
**                                      will time out.  Zero means no timeout.
**	ecb				Pointer to a system specific event
**					control block.  Can be zero if not used.
**
** Outputs:
**      none
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	if mask contains invalid bits or 
**					    contains timeout and to_cnt < 0
**	    E_CS0008_INTERRUPTED	if resumed due to interrupt
**	    E_CS0009_TIMEOUT		if resumed due to timeout
**	Exceptions:
**	    none
**
** Side Effects:
**	    Task is suspended awaiting completion of event
**
** History:
**      30-Oct-1986 (fred)
**          Created
**      30-Jan-1987 (fred)
**          Added ECB parameter for MVS.
**      09-mar-1987 (fred)
**          Added saving what we are waiting for for the monitor.
**	20-oct-1988 (rogerk)
**	    Ifdef'd out lib$ast_in_prog call.
**	 7-nov-1988 (rogerk)
**	    Call CS_ssprsm directly instead of through AST's.
**	    Added new inkernel protection.  Set cs_inkernel flag in both
**	    the server block and scb to make the routine behave as if atomic.
**	    When done, turn off the inkernel flags and check to see if we
**	    CS_move_async is needed.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		Added support for new CS_GWFIO_MASK to indicate that the
**		session is suspended due to a non-SQL Gateway wait of some sort
**		(I/O, lock, or other...).
**      29-jul-1991 (bryanp)
**          B38197: While running stress tests, a timing-related bug was found:
**          If a CSsuspend call returned E_CS0008_INTERRUPTED, it was returning
**          with the thread still on the ready queue, but in CS_EVENT_WAIT
**          state. The thread was now vulnerable to getting hung: if a quantum
**          switch occurs before the next time the thread calls CSsuspend, the
**          thread will be hung, since CS_xchng_thread will never re-schedule
**          a CS_EVENT_WAIT thread. The fix is simply to ensure that the
**          thread is in CS_COMPUTABLE state when it returns from CSsuspend.
**	26-apr-1993 (bryanp)
**	    Improved support for CS_TIMEOUT_MASK in CSsuspend when cs_state
**	    is CS_INITIALIZING.
**	27-jul-1995 (dougb)
**	    Make queue inserts consistent throughout.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**	09-jan-1996 (chech02)
**	   A negative value of timeout passed in CSsuspend is considered
**	   a millisecond for OI 2.0.
**	11-aug-1998 (kinte01)
**		integrate change 423131
**          01-Dec-1995 (jenjo02)
**             Defined 2 new wait reasons, CS_LGEVENT_MASK and CS_LKEVENT_MASK
**             to statistically distinguish these kinds of waits from other
**             LG/LK waits. 
**          16-Jan-1996 (jenjo02)
**             CS_LOG_MASK is no longer the default wait reason and must
**             be explicitly stated in the CSsuspend() call.
**          25-Mar-1997 (jenjo02)
**             Third parameter may be a ptr to a LK_LOCK_KEY when sleep
**             mask is CS_LOCK_MASK (for iimonitor) instead of an ecb.
**	16-Nov-1998 (jenjo02)
**	    Allow new suspend masks CS_LIO_MASK, CS_IOR_MASK, CS_IOW_MASK.
**          14-Mar-2003 (bolke01) Bug 110235 INGSRV 2263
**	       Ensure that in in_kernel flags are set when checking the cs_mask
**	       in CSsuspend. to prevent unknown AST's interupting this  
**	       critical process section.
**	23-oct-2003 (devjo01)
**		If CS_LKEVENT_MASK is set, 'ecb' is overloaded 
**		to point to additional info about the LKEVENT.
**		(similar to the way CS_LOCK_MASK is handled.)
**          19-oct-2004 (horda03) Bug 113246/INGSRV3003
**             For non-threaded applications, disable ASTs then check to see
**             if a CP resume has already occured. If it has then clear
**             the pending wakeup, and resume processing. If not, then if
**             timeout value is specified schedule the process wakeup, and
**             hibernte process. When the process finally resumes, if the
**             process_wait_state is CS_PROCESS_WAIT then the process must
**             have timedout.
*/
STATUS
IICSsuspend(
i4      mask,
i4      to_cnt,
PTR     ecb)
{
    CS_SCB              *scb;
    STATUS		rv = OK;
    i4		time[2];
    i4		status;

#ifdef xDEBUG    
    if (lib$ast_in_prog())   /* primarily for debugging -- remove later */
    {
	TRdisplay("CSsuspend() called at AST level -- MAJOR FAILURE\n");
    }
#endif

    if ((mask & ~(CS_TIMEOUT_MASK |
		    CS_INTERRUPT_MASK |
		    CS_DIO_MASK |
		    CS_BIO_MASK |
                    CS_LOCK_MASK |
		    CS_LOG_MASK |
		    CS_LKEVENT_MASK |
		    CS_LGEVENT_MASK |
                    CS_GWFIO_MASK |
		    CS_LIO_MASK |
		    CS_IOR_MASK |
		    CS_IOW_MASK
		  )) != 0)
	return(E_CS0004_BAD_PARAMETER);

    if (Cs_srv_block.cs_state != CS_INITIALIZING)
    {
	scb = Cs_srv_block.cs_current;

	if ((mask & CS_DIOR_MASK) == CS_DIOR_MASK)
	{
	    scb->cs_dior++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_done++;
	}
	else if ((mask & CS_DIOW_MASK) == CS_DIOW_MASK)
	{
	    scb->cs_diow++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_done++;
	}
	else if ((mask & CS_BIOR_MASK) == CS_BIOR_MASK)
	{
	    scb->cs_bior++;
	    Cs_srv_block.cs_wtstatistics.cs_bior_done++;
	}
	else if ((mask & CS_BIOW_MASK) == CS_BIOW_MASK)
	{
	    scb->cs_biow++;
	    Cs_srv_block.cs_wtstatistics.cs_biow_done++;
	}
	else if ((mask & CS_LIOR_MASK) == CS_LIOR_MASK)
	{
	    scb->cs_lior++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_done++;
	}
	else if ((mask & CS_LIOW_MASK) == CS_LIOW_MASK)
	{
	    scb->cs_liow++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_done++;
	}
	else if (mask & CS_LOCK_MASK)
	{
	    scb->cs_locks++;
	    Cs_srv_block.cs_wtstatistics.cs_lk_done++;
            if (ecb)
                scb->cs_sync_obj = ecb;
	}
        else if (mask & CS_LKEVENT_MASK)
        {
            scb->cs_locks++;
            Cs_srv_block.cs_wtstatistics.cs_lke_done++;
            if (ecb)
                scb->cs_sync_obj = ecb;
        }
        else if (mask & CS_LGEVENT_MASK)
        {
	    scb->cs_base_priority++;	/* temp to hold log counts */
            Cs_srv_block.cs_wtstatistics.cs_lge_done++;
        }
        else if (mask & CS_LOG_MASK)
        {
	    scb->cs_base_priority++;	/* temp to hold log counts */
            Cs_srv_block.cs_wtstatistics.cs_lg_done++;
        }
        else if (mask & CS_TIMEOUT_MASK)
        {
            Cs_srv_block.cs_wtstatistics.cs_tm_done++;
        }
        else if (mask & CS_GWFIO_MASK)
        {
            scb->cs_gwfio++;
            Cs_srv_block.cs_wtstatistics.cs_gwfio_done++;
        }
	else
	{
	    scb->cs_base_priority++;	/* temp to hold log counts */
	    Cs_srv_block.cs_wtstatistics.cs_lg_done++;
	}

	/*
	** Note: The ordering of the following events is important:
	**
	**    1) Set scb->cs_state = CS_EVENT_WAIT
	**    2) Set scb->cs_inkernel
	**    3) Check for CS_EDONE_MASK
	**
	** The ordering here is important since CSresume runs asynchronously
	** and the resume we are about to suspend for may be called at any
	** time in the middle of while we are executing this routine.
	** If CSresume is called and the state is not CS_EVENT_WAIT, then
	** the event completion will just be marked int the cs_mask field
	** and the scb is assumed to already be on the ready queue.  If
	** however, cs_inkernel is set for the scb, then the cs_mask field
	** cannot be altered by the asynchronous routine, and the EDONE_MASK
	** is set in the cs_async_mask field.  We must make sure that if we
	** see that CS_EDONE_MASK is not set here, that the CSresume call
	** (even if it was called just previous to our checking cs_mask) puts
	** the scb on the ready list (or async ready list).  We do this by
	** setting CS_EVENT_WAIT before setting cs_inkernel.
	*/

	/* Turn inkernel on while changing cs_mask */

	Cs_srv_block.cs_inkernel = 1;
	scb->cs_state = CS_EVENT_WAIT;
	scb->cs_inkernel = 1;
	if (scb->cs_mask & CS_EDONE_MASK)
	{
	    /*
	    ** Then there is nothing to do, the event is already done.
	    ** In this case, we can ignore the suspension
	    */
	    scb->cs_mask &= ~(CS_EDONE_MASK | CS_IRCV_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);
	    scb->cs_state = CS_COMPUTABLE;
	/* Turn inkernel off before returning */

	    scb->cs_inkernel = 0;
	    Cs_srv_block.cs_inkernel = 0;

	    if (Cs_srv_block.cs_async)
		CS_move_async(scb);

	    return(OK);
	}
	else if ((scb->cs_mask & CS_IRPENDING_MASK) &&
		    (mask & CS_INTERRUPT_MASK))
	{
	    /*
	    **	Then an interrupt is pending.  Pretend we just got it
	    **	The in_kernel flags are still both 1
	    */	
	    scb->cs_mask &= ~(CS_EDONE_MASK | CS_IRCV_MASK | CS_IRPENDING_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);
            scb->cs_state = CS_COMPUTABLE;
	/* Turn inkernel off before returning */
	    scb->cs_inkernel = 0;
	    Cs_srv_block.cs_inkernel = 0;
	    if (Cs_srv_block.cs_async)
		CS_move_async(scb);

	    return(E_CS0008_INTERRUPTED);
	}
	    
	/* The in_kernel flags are still both 1 */
	scb->cs_state = CS_EVENT_WAIT;
        scb->cs_mask |= (mask &
                ~(CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK | CS_GWFIO_MASK |
                                  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
                                  CS_LOG_MASK));
        scb->cs_memory = (mask &
                 (CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK | CS_GWFIO_MASK |
                                  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
                                  CS_LOG_MASK | CS_TIMEOUT_MASK));

	/*
	** Don't collect statistics for dbms task threads - they skew the
	** server statistics.
	*/
	if ((mask & CS_DIOR_MASK) == CS_DIOR_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_dior_waits++;
	else if ((mask & CS_DIOW_MASK) == CS_DIOW_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_diow_waits++;
	else if ((mask & CS_LIOR_MASK) == CS_LIOR_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_lior_waits++;
	else if ((mask & CS_LIOW_MASK) == CS_LIOW_MASK)
	    Cs_srv_block.cs_wtstatistics.cs_liow_waits++;
        else if (mask & CS_GWFIO_MASK)
            Cs_srv_block.cs_wtstatistics.cs_gwfio_waits++;
	else
	{
            if (scb->cs_thread_type == CS_USER_THREAD)
            {
		if ((mask & CS_BIOR_MASK) == CS_BIOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_bior_waits++;
		else if ((mask & CS_BIOW_MASK) == CS_BIOW_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_biow_waits++;
                else if (mask & CS_LOCK_MASK)
                    Cs_srv_block.cs_wtstatistics.cs_lk_waits++;
                else if (mask & CS_LKEVENT_MASK)
                    Cs_srv_block.cs_wtstatistics.cs_lke_waits++;
                else if (mask & CS_LGEVENT_MASK)
                    Cs_srv_block.cs_wtstatistics.cs_lge_waits++;
                else if (mask & CS_LOG_MASK)
                    Cs_srv_block.cs_wtstatistics.cs_lg_waits++;
                else if (mask & CS_TIMEOUT_MASK)
                    Cs_srv_block.cs_wtstatistics.cs_tm_waits++;
            }

	}

	scb->cs_ppid = Cs_srv_block.cs_quantums;
	
	scb->cs_timeout = 0;
	if (mask & CS_TIMEOUT_MASK)
	{
         if (to_cnt < 0)
         {
            scb->cs_timeout = (-to_cnt)/1000;
            if (scb->cs_timeout == 0)
                scb->cs_timeout = 1;
         }
         else
            scb->cs_timeout = to_cnt;
        }


	/* Now take this scb out of the ready queue */
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = 
	    scb->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
	    scb->cs_rw_q.cs_q_next;

        if (scb->cs_thread_type != CS_INTRNL_THREAD)
            Cs_srv_block.cs_num_active--;

	/* if this was the last block, then turn off the ready bit */

	if ((scb->cs_rw_q.cs_q_next ==
			Cs_srv_block.cs_rdy_que[scb->cs_priority])
		&& (scb->cs_rw_q.cs_q_prev ==
			Cs_srv_block.cs_rdy_que[scb->cs_priority]))
	    Cs_srv_block.cs_ready_mask &=
			~(CS_PRIORITY_BIT >> scb->cs_priority);

	/*
	** Now install the scb on the correct wait que.  If there is no timeout,
	** then the scb goes on the simple wait list.  However, if there is a
	** timeout, then the scb goes on the timeout list, which is scanned and
	** updated once per second (or once per quantum, whichever is longer).
	*/
	 
	if (scb->cs_timeout)
	{
	    scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_to_list->cs_rw_q.cs_q_next;
	    scb->cs_rw_q.cs_q_prev = Cs_srv_block.cs_to_list;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	}
	else
	{
	    scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_wt_list->cs_rw_q.cs_q_next;
	    scb->cs_rw_q.cs_q_prev = Cs_srv_block.cs_wt_list;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	}
	/* now done, so go install the next thread */

	scb->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;
	if (Cs_srv_block.cs_async)
	{
	    CS_move_async(scb);
	}

	CS_stall();

        /* Turn inkernel on while checking cs_mask 
	** in addition to it being set whilst chnging it
	*/

        Cs_srv_block.cs_inkernel = 1;
        scb->cs_inkernel = 1;

	/* time passes, things get done.  Finally we resume ... */

	if (scb->cs_mask & CS_EDONE_MASK)
	{
	    rv = OK;
	}
	else if (scb->cs_mask & CS_IRCV_MASK)
	{
	    rv = E_CS0008_INTERRUPTED;
	}
	else if (scb->cs_mask & CS_TO_MASK)
	{
	    rv = E_CS0009_TIMEOUT;
	}
	else if (scb->cs_mask & CS_ABORTED_MASK)
	{
	    rv = E_CS000F_REQUEST_ABORTED;
	}
	else
	{
	    /* we shouldn't be here */
	    (*Cs_srv_block.cs_elog)(E_CS0019_INVALID_READY, 0, 0);
	    rv = E_CS0019_INVALID_READY;
	}

        /* Turn inkernel on while checking cs_mask 
	** above and stay in in_kernel whilst changing it 
	*/

	scb->cs_memory = 0;
	scb->cs_mask &= ~(CS_EDONE_MASK | CS_IRCV_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);

	scb->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;
	if (Cs_srv_block.cs_async)
	    CS_move_async(scb);
    }
    else
    {
        /* Lets see if the session has already been resumed.
        ** Close an AST window where a process can be resumed
        ** before it is suspended.
        */

        sys$setast(0);

        if (process_wait_state != CS_PROCESS_WOKEN)
        {
           /* Process hasn't already received the wake up call
           */

           process_wait_state = CS_PROCESS_WAIT;

           /* If to_cnt is 0, then there is no wakeup time to schedule */

	   if (to_cnt && (mask & CS_TIMEOUT_MASK) )
	   {
	      time[1] = -1;
	      if (to_cnt < 0) 
		time[0] = CS_ONE_MILLISECOND * (-to_cnt);
	      else
	        time[0] = CS_ONE_SECOND * to_cnt;

	      status = sys$schdwk(0, 0, time, 0);
	      if ((status & 1) == 0)
	      {
                CL_ERR_DESC          error;
                char                err_buffer[150];
                STprintf(err_buffer, "sys$schdwk error status %x \n", status);
        	ERlog(err_buffer, STlength(err_buffer), &error);
		PCexit(FAIL);
	      }
 	   }
        }
        

        /* Note enable AST prior to sending the process to sleep */
	sys$setast(1);          /* make sure ast's are on */
        sys$hiber();

	/*
	** If we woke up due to the schdwk expiring, then process_wait_state
	** will be CS_PROCESS_WAIT, so return timed-out from CSsuspend:
	*/
	if (process_wait_state == CS_PROCESS_WAIT)
        {
	      rv = E_CS0009_TIMEOUT;
        }
	process_wait_state = CS_PROCESS_NOWAIT;
    }
    return(rv);
}

bool
CS_is_mt( void )
{
    return( Cs_srv_block.cs_mt );
}

/*
** Name: CSadjust_counter() - Thread-safe counter adjustment.
**
** Description:
**      This routine provides a means to accurately adjust a counter
**      which may possibly be undergoing simultaneous adjustment in
**      another session, or in a Signal Handler, or AST, without
**      protecting the counter with a mutex.
**
**      This is done, by taking advantage of the atomic compare and
**      swap routine.   We first calculate the new value based on
**      a "dirty" read of the existing value.  We then use
**      compare and swap logic to attempt to update the counter.
**      If the counter results had changed before our update is applied,
**      update is canceled, and a new "new" value must be calculated
**      based on the new "old" value returned.  Even under very heavy
**      contention, the caller will eventually get to update the counter
**      before someone else gets in to this tight loop.
**
**      This routime will work properly under both INTERNAL (Ingres)
**      and OS threads.
**
** Inputs:
**      pcounter        - Address of an i4 counter.
**      adjustment      - Amount to adjust by.
**
** Outputs:
**      None
**
** Returns:
**      Value counter assumed after callers update was applied.
**
** History:
**      29-aug-2002 (devjo01)
**          created.
**	28-apr-2005 (devjo01)
**	    Correct subtle error in CSadjust_counter which could lead to
**	    a lost update. (fetching oldvalue was outside loop!)
*/
i4
CSadjust_counter( i4 *pcounter, i4 adjustment )
{
    i4          oldvalue, newvalue;

    do
    {
	oldvalue = *pcounter;
        newvalue = oldvalue + adjustment;
    } while ( CScas4( pcounter, oldvalue, newvalue ) );

    return newvalue;
} /* CSadjust_counter */


#ifdef PERF_TEST
<FF>
/*
** Name: CSfac_stats - collect stats on a per-facility basis
**
** Description:
**      This routine collects stats on a per-facility basis, and places
**      the results in the CS_SYSTEM struct, tracing is enabled through the
**      use of SCF trace points.
**      NOTE: updates are made to the global Cs_srv_block here without
**      semaphore protection, so the stats are not guarunteed to be 100%
**      accurate, most tests so far have been single user so it's not
**      an issue. CSp_semaphore() uses up alot of CPU which realy defeats
**      the point of the profiling. But if accurate multi-user values are
**      required, you'll have to add a new CS_MUTEX to CS_SYSTEM to protect
**      the CS_PROFILE values. In 2.0 we could use CS_synch_lock, but it's
**      not available in earlier releases.
**
** Inputs:
**      start   start or end collecting
**      fac_id  facility id to collect for
**
** Outputs:
**      None
**
** Returns:
**      None
**
** History:
**      28-nov-96 (stephenb)
**          (re)created.
*/
void
CSfac_stats(
        bool    start,
        int     fac_id)
{
    int                 i;
    struct tms          time_info;
    int                 errnum = errno;
    f8                  cpu_used;

    if (Cs_srv_block.cs_cpu_zero == 0   /* profiling not enabled */
        || fac_id == 0)                 /* nothing to profile */
        return;


    if (start)
    {
        if (Cs_srv_block.cs_profile[fac_id].cs_active)
        {
            /* overloaded call, just log it and return */
            Cs_srv_block.cs_profile[fac_id].cs_overload += 1;
            return;
        }
        /* get stats from system */
        if (times(&time_info) < 0)
        {
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &errnum, 0);
            return;
        }
        /* activate counting */
        Cs_srv_block.cs_profile[fac_id].cs_active = TRUE;
        /* add to call count */
        Cs_srv_block.cs_profile[fac_id].cs_calls += 1;
        /* set initial value */
        Cs_srv_block.cs_profile[fac_id].cs_start_cpu =
            time_info.tms_utime + time_info.tms_stime;
        Cs_srv_block.cs_profile[fac_id].cs_start_cpu =
        Cs_srv_block.cs_profile[fac_id].cs_start_cpu / HZ;
    }
    else /* end counting */
    {
        if (Cs_srv_block.cs_profile[fac_id].cs_active == FALSE)
        {
            /* Oops...coding inconsistency */
            (*Cs_srv_block.cs_elog)(E_CS0046_CS_NO_START_PROF, 0, 1,
                sizeof(fac_id), &fac_id);
            return;
        }
        /* stop counting */
        Cs_srv_block.cs_profile[fac_id].cs_active = FALSE;
        /* get stats from system */
        if (times(&time_info) < 0)
        {
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &errnum, 0);
            return;
        }
        /* calculate cpu usage */
        cpu_used = time_info.tms_utime + time_info.tms_stime;
        cpu_used = cpu_used / HZ;
        cpu_used -= Cs_srv_block.cs_profile[fac_id].cs_start_cpu;
        /* and add to current count */
        Cs_srv_block.cs_profile[fac_id].cs_cum_cpu += cpu_used;
        /* check for deductions to other facilities */
        for (i = 1; i <= CS_MAX_FACILITY; i++)
        {
            if ((cs_facilities[i].sub_facility_id &&
                cs_facilities[i].sub_facility_id == fac_id) ||
                (cs_facilities[i].sub_facility_id == 0 &&
                cs_facilities[i].facility_id == fac_id))
                /* our own facility */
                continue;
            if (Cs_srv_block.cs_profile[i].cs_active)
                Cs_srv_block.cs_profile[i].cs_cum_cpu -= cpu_used;
        }
    }
    return;
}
<FF>
/*
** Name: CScollect_stats - start/stop stats collection or print current stats.
**
** Description:
**      This routine starts or stops per-facility stats collection, or if
**      start is requested and we are alread collecting, it will print out
**      the current stats values
**
** Inputs:
**      start   start/stop or print
**
** Outputs:
**      None
**
** Returns:
**      void
**
** Side Effects:
**      may print stats to II_DBMS_LOG
**
** History:
**      28-nov-96 (stephenb)
**          (re)created.
*/
void
CScollect_stats(bool    start)
{
    int                 i,j;
    int                 errnum = errno;
    struct tms          time_info;
    f8                  total_used;

    if (start && Cs_srv_block.cs_cpu_zero)
    {
        /* stats collection already running, print out current status */
        /* dont bother mutexing this, we're just prining current approx */
        /* stats */
        TRdisplay("Fac\tSub-fac\t\tCalls\tOverloaded\tCPUsec\n");
        for (i = 1; i <= CS_MAX_FACILITY; i++)
        {
            if (cs_facilities[i].sub_facility_id == 0 &&
                Cs_srv_block.cs_profile[i].cs_calls) /* only print if called 
*/
            {
                /* a main facility */
                TRdisplay("%s\t\t\t%d\t%d\t\t%.2f\n",
                    cs_facilities[i].facility_name,
                    Cs_srv_block.cs_profile[i].cs_calls,
                    Cs_srv_block.cs_profile[i].cs_overload,
                    Cs_srv_block.cs_profile[i].cs_cum_cpu);
                /* search for sub-facilities */
                for (j = 1; j <= CS_MAX_FACILITY; j++)
                {
                    if (cs_facilities[j].sub_facility_id &&
                        cs_facilities[j].facility_id ==
                        cs_facilities[i].facility_id &&
                        Cs_srv_block.cs_profile[j].cs_calls)
                    {
                        /* a sub-facility of this main */
                        TRdisplay("\t%s\t\t%d\t%d\t\t%.2f\n",
                            cs_facilities[j].facility_name,
                            Cs_srv_block.cs_profile[j].cs_calls,
                            Cs_srv_block.cs_profile[j].cs_overload,
                            Cs_srv_block.cs_profile[j].cs_cum_cpu);
                    }
                }
            }
        }
        if (times(&time_info) < 0)
        {
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &errnum, 0);
            return;
        }
        else
        {
            total_used = time_info.tms_utime + time_info.tms_stime;
            total_used = total_used  / HZ;
            total_used -= Cs_srv_block.cs_cpu_zero;
            TRdisplay("----------------------------------------------------\n"
);
            TRdisplay("Total\t\t\t\t\t\t%.2f\n",total_used);
        }

    }
    else if (start)
    {
        /* start stats collection, cs_cpu_zero could hold the base CPU usage
        ** at start time, but for now we'll just use it as an indicator */
        if (times(&time_info) < 0)
        {
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &errnum, 0);
            return;
        }
        else
        {
            Cs_srv_block.cs_cpu_zero  = time_info.tms_utime +
                time_info.tms_stime;
            Cs_srv_block.cs_cpu_zero = Cs_srv_block.cs_cpu_zero / HZ;
            TRdisplay("Per facility profiling enabled at %@\n");
        }
    }
    else /* end stats collection */
    {
        Cs_srv_block.cs_cpu_zero = 0;
        /* zero stats */
        for (i = 1; i <= CS_MAX_FACILITY; i++)
        {
            Cs_srv_block.cs_profile[i].cs_active = FALSE;
            Cs_srv_block.cs_profile[i].cs_calls = 0;
            Cs_srv_block.cs_profile[i].cs_overload = 0;
            Cs_srv_block.cs_profile[i].cs_start_cpu = 0;
            Cs_srv_block.cs_profile[i].cs_cum_cpu = 0;
        }
        TRdisplay("Per facility profiling disabled at %@\n");
    }
    return;
}
#endif /* PERF_TEST */

#ifdef xDEV_TRACE
<FF>
/*
** Name: CSfac_trace - collect trace info on a per-facility basis
**
** Description:
**      This routine, when enabled, will add the caller's facility and parm
**      to the memory trace table to be printed later.
**      Initially, the tracing will be at the facility id.  Later,
**      if this proves to be useful, more details can be added.
**
** Inputs:
**      start   start or end collecting
**      fac_id  facility id
**      ptr     addr of additional info to trace e.g. addr(parms)
**
** Outputs:
**      None
**
** Returns:
**      None
**
** History:
**      21-may-1997 (shero03)
**          created.
*/
void
CSfac_trace(
        int     where,
        int     fac_id,
        int     opt_parm,
        PTR     opt_addr)
{
    int                 i;
    CS_SCB              *scbptr;
    CS_DEV_TRCENTRY     *curr_entry;

    if (fac_id == 0)                    /* nothing to profile */
        return;

    if (Cs_srv_block.cs_dev_trace_strt == NULL)
    {
        /* getmain an area of say 5,000 trace entries */
        /* initialize them to nulls */
        curr_entry = (CS_DEV_TRCENTRY *)malloc( sizeof(CS_DEV_TRCENTRY)
                                                * CS_DEV_TRACE_ENTRIES);
        MEfill(sizeof(CS_DEV_TRCENTRY) * CS_DEV_TRACE_ENTRIES, 0, curr_entry);
        Cs_srv_block.cs_dev_trace_strt = curr_entry;
        Cs_srv_block.cs_dev_trace_end = curr_entry + CS_DEV_TRACE_ENTRIES;
        curr_entry =
        Cs_srv_block.cs_dev_trace_curr = Cs_srv_block.cs_dev_trace_end - 1;
    }
    else
        curr_entry = Cs_srv_block.cs_dev_trace_curr;

    CSget_scb(&scbptr);

    curr_entry->cs_dev_trc_scb = scbptr;
    curr_entry->cs_dev_trc_fac = fac_id;
    curr_entry->cs_dev_trc_parm = opt_parm;
    curr_entry->cs_dev_trc_at  = where;
    curr_entry->cs_dev_trc_opt = opt_addr;

    curr_entry--;
    if (curr_entry < Cs_srv_block.cs_dev_trace_strt)
       curr_entry = Cs_srv_block.cs_dev_trace_end - 1;
    Cs_srv_block.cs_dev_trace_curr = curr_entry;

    return;
}
#endif /* xDEV_TRACE */



static i4
CS_usercnt()
{
    return(Cs_srv_block.cs_user_sessions);
}



/*{
** Name: CSresume()	- Mark thread as done with event
**
** Description:
**      This routine is called to note that the session identified 
**      by the parameter has completed some event.
**
**	This routine is sometimes called at normal level and sometimes it is
**	a delivered AST. Thus it must be careful since the world may be in
**      an inconsistent state, and it cannot use semaphores, since it may 
**      be being called by the o/s.
**
** Inputs:
**      sid                             thread id for which event has
**					completed.
**					Certain sid's with special meaning
**					may cause other operations to happen.
**					For instance, an sid of CS_ADDR_ID
**					will cause a new thread to be added.
**
** Outputs:
**      none
**	Returns:
**	    none -- actually system specific.  On VMS, it is void.
**	Exceptions:
**	    none
**
** Side Effects:
**	    The SCB for this thread is moved to the ready que, if it is not
**	    already there (via an interrupt or timeout).
** History:
**      30-Oct-1986 (fred)
**          Created.
**	 7-nov-1988 (rogerk)
**	    Before storing information in the scb or in the server control
**	    block, check the cs_inkernel flag to see if that control block
**	    is currently busy.  If so, write into the async portion of the
**	    control block.
**	16-dec-1988 (fred)
**	    set cs_async field when listen completes while in kernel state.
**	    BUG 4255.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		Added support for CS_GWFIO_MASK to indicate Gateway suspension.
**	15-oct-1992 (bryanp)
**	    Use cs_inkernel to mark critical sections, thus allowing this
**	    routine to be called at non-AST level. Needed by LG/LK.
**	26-apr-1993 (bryanp)
**	    When resuming in the CS_INITIALIZING case, cancel any scheduled
**	    wakeup which may be outstanding.
**	27-jul-1995 (dougb)
**	    Make queue inserts consistent throughout.
**      19-oct-2004 (horda03) Bug 113246/INGSRV3003
**          For non-threaded applications, indicate process has been
**          resumed.
*/
void
IICSresume(
CS_SID  sid)
{
    CS_SCB              *scb;
                                              
    if (Cs_srv_block.cs_state != CS_INITIALIZING)
    {
	scb = CS_find_scb(sid);	    /* locate the scb for this thread */
	if (scb)
	{
	    i4		elapsed_time;

#ifdef	xDEBUG
	    if (scb == &Cs_idle_scb)
	    {
		if (Cs_signal)
		    lib$signal(SS$_DEBUG);
	    }
#endif

	    if (Cs_srv_block.cs_inkernel == 0)
	    {
		/*
		** If the server is not currently inkernel, then go ahead
		** and enter inkernel mode. Once we've set the server in-kernel,
		** we not that we will not get time-sliced out. The only
		** operations which might interrupt us are AST-delivered
		** subroutines, but these operations will observe that the
		** server inkernel bit is set and will perform their work
		** on the async queue.
		**
		** If the server is not currently inkernel, but at the end of
		** our operations we discover that the server's cs_async field
		** is set, then this invocation must be a non-AST call to
		** CSresume which was interrupted by some asynchronous
		** processing, so we call CSmove_async to complete the
		** asynchronous processing before we return.
		**
		** An AST_delivered CSresume call will run from start to
		** completion without being interrupted by any other AST
		** routines (because they're all user mode AST routines). So
		** if it observes that cs_inkernel is off, it will set it on,
		** do its work, clear it, and return, but it will never find
		** that cs_async is set.
		*/

		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;
		/*
		** If this session is not listed in event wait state, then the
		** event has completed before that session had a chance to
		** suspend itself.  Just mark that the event has completed -
		** there is no need to move the session to the ready queue.
		*/
		if (scb->cs_state != CS_EVENT_WAIT)
		{
		    /* Then mark the event as already completed and move on */
		    scb->cs_mask |= CS_EDONE_MASK;
		    scb->cs_inkernel = 0;
		    Cs_srv_block.cs_inkernel = 0;
		    if (Cs_srv_block.cs_async)
			CS_move_async(scb);
		    return;
		}
		/*
		** Otherwise, we have some work to do.  We need to take the
		** event off whatever wait queue it is on, and requeue it in
		** the appropriate spot in the ready que.  Also, we need to
		** mark the appropriate bit in the ready mask to say that a
		** a job of this priority is now ready to run.  Finally, we
		** need to check to see of the server is currently idle.  If
		** it is, the process is in hibernation waiting for something
		** interesting to do, so we need to perform a wakeup so that
		** the process gets a kick and learns that there is something
		** new to do.
		*/

		/*
		** Don't gather statistics on wait times for the DBMS task
		** threads.  They are expected to be waiting and they only
		** throw off the statistics.
		*/
		elapsed_time = Cs_srv_block.cs_quantums - scb->cs_ppid;

		if ((scb->cs_memory & CS_DIOR_MASK) == CS_DIOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_dior_time += elapsed_time;
		else if ((scb->cs_memory & CS_DIOW_MASK) == CS_DIOW_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_diow_time += elapsed_time;
		else if ((scb->cs_memory & CS_LIOR_MASK) == CS_LIOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_lior_time += elapsed_time;
		else if ((scb->cs_memory & CS_LIOW_MASK) == CS_LIOW_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_liow_time += elapsed_time;
		else if (scb->cs_memory & CS_GWFIO_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_gwfio_time += elapsed_time;
		else
		{
		    if (scb->cs_thread_type == CS_USER_THREAD)
		    {
			if ((scb->cs_memory & CS_BIOR_MASK) == CS_BIOR_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_bior_time +=
				elapsed_time;
			else if ((scb->cs_memory & CS_BIOW_MASK) == CS_BIOW_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_biow_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LOCK_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lk_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LKEVENT_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lke_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LGEVENT_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lge_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LOG_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lg_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_TIMEOUT_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_tm_time +=
				elapsed_time;
		    }
		}

		/*
		** Since the Server was not running inkernel, we can move
		** the scb off the wait list and put it into the ready queue.
		*/
		scb->cs_mask |= CS_EDONE_MASK;
		scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		    scb->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		    scb->cs_rw_q.cs_q_next;

		/* now link into the ready queue */

		scb->cs_rw_q.cs_q_next =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority];
		scb->cs_rw_q.cs_q_prev =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
		scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
		Cs_srv_block.cs_ready_mask |= 
			(CS_PRIORITY_BIT >> scb->cs_priority);

                if (scb->cs_thread_type != CS_INTRNL_THREAD &&
                  (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                    Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;

#ifdef	CS_STACK_SHARING
		if (scb->cs_stk_area)
		{
#endif
		    scb->cs_state = CS_COMPUTABLE;
		    if (Cs_srv_block.cs_state == CS_IDLING)
			sys$wake(0, 0);
#ifdef	CS_STACK_SHARING
		}
		else
		{
		    scb->cs_state = CS_STACK_WAIT;
		    scb->cs_mask &= ~(CS_INTERRUPT_MASK);	/* turn off	    */
							    /* interruptability */
		}
#endif
		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
		return;
	    }
	    else
	    {
		/*
		** The server IS currently inkernel. This means that we are
		** an ast-driven invocation of CSresume which has interrupted
		** some non-ast-driven CS code which is part-way through
		** some sensitive manipulations of the main server lists. We
		** must be careful not to directly touch those lists. Instead
		** we perform out operations on special "async" portions of
		** the control blocks. Once we return and the session which
		** was interrupted gets control again and completes its
		** critical section, it will observe (by checking the cs_async
		** field) that we have put stuff on the async lists and it
		** will call CSmove_async to process those pending operations.
		**
		** Now, it may be the case that the server is inkernel but this
		** particular session is not inkernel. In that case, we can
		** go ahead and work with the fields in this session's control
		** block, but we cannot do any manipulations of the ready queue.
		*/

		/*
		** If this session is not listed in event wait state, then the
		** event has completed before that session had a chance to
		** suspend itself.  Just mark that the event has completed - there
		** is no need to move the session to the ready queue.
		*/
		if (scb->cs_state != CS_EVENT_WAIT)
		{
		    /* Then mark the event as already completed and move on */
		    if (scb->cs_inkernel == 0)
		    {
			scb->cs_mask |= CS_EDONE_MASK;
			return;
		    }

		    /*
		    ** If this SCB is busy, then store the information in the
		    ** async part of the scb.
		    */
		    scb->cs_asmask |= CS_EDONE_MASK;
		    scb->cs_asunmask &= ~CS_EDONE_MASK;
		    if (scb->cs_async == 0)
		    {
			scb->cs_async = 1;
			Cs_srv_block.cs_scb_async_cnt++;
		    }
		    Cs_srv_block.cs_async = 1;
		    return;
		}

		/*
		** Otherwise, we have some work to do.  We need to take the
		** event off whatever wait queue it is on, and requeue it in
		** the appropriate spot in the ready que.  Also, we need to
		** mark the appropriate bit in the ready mask to say that a
		** a job of this priority is now ready to run.  Finally, we
		** need to check to see of the server is currently idle.  If
		** it is, the process is in hibernation waiting for something
		** interesting to do, so we need to perform a wakeup so that
		** the process gets a kick and learns that there is something
		** new to do.
		*/

		/*
		** Don't gather statistics on wait times for the DBMS task
		** threads.  They are expected to be waiting and they only
		** throw off the statistics.
		*/
		elapsed_time = Cs_srv_block.cs_quantums - scb->cs_ppid;

		if ((scb->cs_memory & CS_DIOR_MASK) == CS_DIOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_dior_time += elapsed_time;
		else if ((scb->cs_memory & CS_DIOW_MASK) == CS_DIOW_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_diow_time += elapsed_time;
		else if ((scb->cs_memory & CS_LIOR_MASK) == CS_LIOR_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_lior_time += elapsed_time;
		else if ((scb->cs_memory & CS_LIOW_MASK) == CS_LIOW_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_liow_time += elapsed_time;
		else if (scb->cs_memory & CS_GWFIO_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_gwfio_time += elapsed_time;
		}
		else
		{
		    if (scb->cs_thread_type == CS_USER_THREAD)
		    {
			if ((scb->cs_memory & CS_BIOR_MASK) == CS_BIOR_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_bior_time +=
				elapsed_time;
			else if ((scb->cs_memory & CS_BIOW_MASK) == CS_BIOW_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_biow_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LOCK_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lk_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LKEVENT_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lke_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LGEVENT_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lge_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_LOG_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_lg_time +=
				elapsed_time;
			else if (scb->cs_memory & CS_TIMEOUT_MASK)
			    Cs_srv_block.cs_wtstatistics.cs_tm_time +=
				elapsed_time;
		    }
		}

		/*
		** Since the server is inkernel, we can't muck with any of
		** the control block lists, so we put the scb on the async
		** ready list.  This list is touched only by atomic routines
		** which are protected against thread switching and AST driven
		** routines.
		**
		** When the session which is running inkernel returns from
		** inkernel mode, it will move the scb onto the ready queue.
		*/
		/*
		** Put this session on the async ready queue.
		** It will be moved onto the server ready queue by the
		** session that is running inkernel.
		*/
		if (scb->cs_as_q.cs_q_next == 0)
		{
		    scb->cs_as_q.cs_q_next =
			Cs_srv_block.cs_as_list->cs_as_q.cs_q_next;
		    scb->cs_as_q.cs_q_prev = Cs_srv_block.cs_as_list;
		    scb->cs_as_q.cs_q_prev->cs_as_q.cs_q_next = scb;
		    scb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev = scb;
		}
		Cs_srv_block.cs_async = 1;

		/*
		** Mark the event as complete in the SCB.  If the scb is busy
		** then store the completion information in the async portion
		** of the SCB.  The event completion will be marked in the
		** cs_mask when the scb->cs_inkernel flag is turned off.
		*/
		if (scb->cs_inkernel)
		{
		    if (scb->cs_async == 0)
		    {
			scb->cs_async = 1;
			Cs_srv_block.cs_scb_async_cnt++;
		    }
		    scb->cs_asmask |= CS_EDONE_MASK;
		    scb->cs_asunmask &= ~CS_EDONE_MASK;
		}
		else
		{
		    scb->cs_mask |= CS_EDONE_MASK;
		}

		return;
	    }
	}
	else if (sid == CS_ADDER_ID)
	{
	    /*
	    ** The admin thread is being resumed to add a new user thread.
	    ** If the server block is not inuse then mark the admin thread
	    ** ready and turn on its priority's ready bit.
	    **
	    ** If the server block is busy then mark the async add-thread
	    ** flag.  The admin thread will be set to ready when the inkernel
	    ** flag is turned off.
	    */
	    if (Cs_srv_block.cs_inkernel == 0)
	    {
                Cs_srv_block.cs_inkernel = 1;

		Cs_admin_scb.csa_mask |= CSA_ADD_THREAD;
		Cs_admin_scb.csa_scb.cs_state = CS_COMPUTABLE;
		Cs_srv_block.cs_ready_mask |=
				(CS_PRIORITY_BIT >> CS_PADMIN);

                Cs_srv_block.cs_inkernel = 0;
 
                if (Cs_srv_block.cs_async)
                   CS_move_async(Cs_srv_block.cs_current);

		if (Cs_srv_block.cs_state == CS_IDLING)
		{
		    /* the idle task will wake up and schedule the AST itself */
		    sys$wake(0, 0);
		}
	    }
	    else
	    {
		Cs_srv_block.cs_asadd_thread = 1;
		Cs_srv_block.cs_async = 1;
	    }
	}
    }
    else
    {
	sys$canwak(0,0);        /* Cancel any scheduled wakeups */
	process_wait_state = CS_PROCESS_WOKEN;
	sys$wake(0,0);
    }
}

/*{
** Name: CScancelled	- Event has been canceled
**
** Description:
**      This routine is called by the client routines when an event for 
**      which the client had suspended will not be CSresume()'d in the 
**      future (it may have already been).  This call is intended to be 
**      used whenever a CSsuspend() returns an interrupted or timed out 
**      status.  In these cases, on some systems, the CSresume() call 
**      may appear in the future, because, for example, the event occurred 
**      just after the timeout value has expired.  In this case, the 
**      dispatcher's scheduler cannot tell which event has completed, 
**      and if it is not told when a sequence is complete, the dispatcher 
**      cannot determine when a thread can be resumed correctly. 
** 
**      Thus, it must be the case that if a CSsuspend() routine returns 
**      anything other than OK (i.e. that the event completed normally), 
**      than a CScancelled() call must occur before the next CSsuspend() 
**      call is made.  And furthermore, the CScancelled() call indicates 
**      that the CSresume() call which was queued previously will not 
**      occur (it may have already, or it may never occur, but it may not 
**      occur after the CSresume() call is made).
**
** Inputs:
**	ecb				Pointer to a system specific event
**					control block.  Can be zero if not used.
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
**      30-Oct-1986 (fred)
**          Created on Jupiter.
**	 7-nov-1988 (rogerk)
**	    Set inkernel while altering cs_mask.
*/
void
IICScancelled(
PTR     ecb)
{
    if (Cs_srv_block.cs_state != CS_INITIALIZING)
    {
	Cs_srv_block.cs_inkernel = 1;
	Cs_srv_block.cs_current->cs_inkernel = 1;
	Cs_srv_block.cs_current->cs_mask &=
			    ~(CS_EDONE_MASK | CS_IRCV_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);

	Cs_srv_block.cs_current->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;

	if (Cs_srv_block.cs_async != 0)
	    CS_move_async(Cs_srv_block.cs_current);
    }
}

/*
** Name: dump_deadlocked_semaphore()
**
** Debugging routine to provide extra information on a deadlocked semaphore.
**
** Inputs:
**	sem	the semaphore which is deadlocked, ie. already owned by the
**		very same thread that it trying to acquire it.
**	which	indicates which place in the code we were called from; increment
**		with each new call site.
**
** History:
**	26-feb-1996 (duursma)
**	    Created.
**	30-jul-1996 (boama01)
**	    Added new cs_test_set field.
*/
static
void dump_deadlocked_semaphore(CS_SEMAPHORE *sem, i4 which)
{
    TRdisplay("%@ Semaphore Deadlock %d, value %d count %d owner %p next %p \
list %p type %d pid %d sem_name %s sem_scribble_check %d sem_init_addr %p \
sem_init_pid %p test_set %d\n",
	      which, sem->cs_value, sem->cs_count, sem->cs_owner, sem->cs_next,
	      sem->cs_list, sem->cs_type, sem->cs_pid, sem->cs_sem_name,
	      sem->cs_sem_scribble_check, sem->cs_sem_init_addr,
	      sem->cs_sem_init_pid, sem->cs_test_set);
}



/*{
** Name: CSp_semaphore()	- perform a "P" operation on a semaphore
**
** Description:
**      This routine is called to allow a thread to request a semaphore 
**      and if it is not available, to queue up for the semaphore.  The 
**      thread requesting the semaphore will be returned to the ready 
**      queue when the semaphore is V'd. 
** 
**      At the time that the semaphore is granted, the fact that a semaphore 
**      is held by the thread is noted in the thread control block, to prevent 
**      the stopping of the thread (via the monitor) while a semaphore is held.
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
**      sem                             A pointer to a CS_SEMAPHORE for which
**                                      the thread wishes to wait.
**
** Outputs:
**      none
**	Returns:
**	    OK				- semaphore granted.
**	    E_CS000F_REQUEST_ABORTED	- request interrupted.
**	    E_CS0017_SMPR_DEADLOCK	- semaphore already held by requestor.
**	    E_CS0029_SEM_OWNER_DEAD	- Owner of MP semaphore has died.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-Nov-1986 (fred)
**          Created.
**      14-jan-1987 (fred)
**          Added shared/exclusive boolean.
**      11-Nov-1987 (fred)
**          Add support for semphore collision collection & reporting.
**	01-oct-88 (eric)
**	    Added checks for ASTs that are disabled or in progress.
**	 7-nov-88 (rogerk)
**	    Call CS_ssprsm directly instead of through AST's.
**	    Added new inkernel protection.  Set cs_inkernel flag in both
**	    the server block and scb to make the routine behave as if atomic.
**	    When done, turn off the inkernel flags and check to see if we
**	    CS_move_async is needed.
**	28-feb-89 (rogerk)
**	    Added cross-process semaphore support.  Loop until lock is granted.
**	10-apr-89 (rogerk)
**	    Set inkernel while setting cross process semaphore to prevent us
**	    from thread switching in the middle of setting the pid/owner fields.
**	    If exceed max loops trying to get semaphore, reset loop counter
**	    after sleeping 1 second.
**	 5-sep-1989 (rogerk)
**	    Mark session to not be swapped out by quantum timer while holding
**	    cross process semaphore.
**	04-jan-1989 (ralph)
**	    Change constant limit on checked_for_asts from 20 to 100.
**      19-aug-1991 (rogerk)
**          Changed spin lock in cross process semaphore code to try to
**          avoid tying up the memory bus with interlock instructions by
**          looping on the test/set call.  We now loop just looking for
**          when the semaphore becomes free.  Only when we see it is free
**          do we try to grab it with a test/set call.  This "snooping"
**          technique seems to be the standard for implementing spin locks.
**	4-jun-1992 (bryanp)
**	    Made Mike's 23-mar-1990 Unix CL changes to this file. His comments:
**
**	    Fixes to this file were necessary to make these semaphores work
**	    correctly when called from the dmfrcp, rather than the server.
**
**          - Changed the code to refer to "Cs_srv_block.cs_pid" rather than
**            to "scb->cs_pid" when it wanted the current process' pid.  The
**            old method did not work for some situations in non-dbms processes.
**          - Some code refered to the "scb" control block to keep track of
**            performance stats.  This code was changed to not be called in
**            the cases where the "scb" did not exist (was NULL).
**
**          If the state is CS_INITIALIZING then this is being called
**          from a non-dbms server (ie. the rcp, acp, createdb, ...).
**          We never want to call CSsuspend() from CSp_semaphore in this case,
**          so the code will spin on the semaphore instead.
**	20-oct-1992 (bryanp)
**	    For a multi-process semaphore, check periodically to see if the
**	    semaphore owner is still alive. If the owner is dead, return
**	    CS_SEM_OWNER_DEAD.
**	24-may-1993 (bryanp)
**	    Added the uni-processor optimization from the Unix CSp_semaphore
**		into the VMS CSp_semaphore: If on a uniprocessor machine, then
**		multi-process semaphores should not spin.
**	21-jun-1993 (bryanp)
**	    Repaired unfortunate interaction between CS_SEM_OWNER_DEAD change
**		and uni-processor optimization change, so that hopefully we
**		don't spuriously decide that the semaphore owner is dead just
**		because he was releasing the semaphore (and setting cs_pid to 0)
**		at the instant we happened to check.
**	26-jul-1993 (bryanp)
**	    Incorporate more Unix-isms: allow runtime control over number of
**		loops, time to sleep, etc.
**	18-oct-1993 (rachael) Bug 55351,55353
**          A thread in the server has requested a cross-process semaphore and
**	    found it owned by another process. The requesting server has
**          descheduled itself -- immediately if on a SP machine, after looping
**          for some period of time on a MP machine -- to allow the owning
**          server to run, use,  and then relinquish the semaphore.
**
**          The descheduling is accomplished using two system services: a
**	    "schedule wakeup" call (sys$schdwk), followed by a "hibernate" call
**	    (sys$hiber). The sys$schdwk schedules the awakening of a process
**          after a requested amount of time.  The sys$hiber calls then places
**          the requesting process into a hibernating state.  When the wakeup
**          fires, the hibernating process wakes up and re-requests the
**          semaphore. This sequence of events will occur until the request is
**          granted or it is determined that it will be impossible to grant the
**          request, e.g. the owning process has crashed while holding
**	    the semaphore.
**  		
**	    The problem occurred when another thread in the requesting server
**	    also requested this semaphore.  The first thread had not been marked
**          NOSWAP, so it was swapped out just after scheduling its wakeup, and
**          before the call to sys$hiber. The second thread then would attempt
**          to acquire the semaphore.  The timing was such that the second
**          thread would "handle" the first thread's wakeup, along with its
**	    own. When the first thread was swapped back in, it made the call to
**          sys$hiber without knowing that there was no scheduled wakeup.
**
**	    The fix is to mark the requesting thread NOSWAP before the
**	    descheduling.
**	28-Oct-1993 (rachael)
**	    Do not call code that refers to an scb when called by a
**	    non-dbms server.  
**      22-Feb-1994 (rachael) b58745
**          CSp_semaphore now checks for the semaphore owner pid
**          being 0 in addition to the check for whether the process
**          is alive.  This is necessary because a pid of 0 has special
**          meaning and the semaphore owner may be incorrectly thought dead
**          when the pid passed to PCisalive is 0.
**	25-apr-1994 (bryanp)
**	    Made another attempt to close some of the holes relating to B58745.
**		In particular, added code to allow for the semaphore being
**		released by process B and acquired by process C just as
**		process A is checking to see if the owner is dead.
**	27-jul-1995 (dougb)
**	    Since CS_ssprsm() will ignore priorities without the ready bit set
**	    and will clear the bit for priorities where nothing is runable,
**	    make sure the ready mask is always consistent when not inkernel.
**	    CS_move_async() now handles a NULL parameter, remove conditions
**	    from calls to this routine.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**	30-jul-1996 (boama01)
**	    Use new cs_test_set fld in sem instead of old cs_value for MULTI
**	    sems; the old fld would not allow Memory Barrier BBSSI instruction
**	    in CS_test_set() to block concurrent multiprocess access on AXP;
**	    the new fld is sized and aligned to permit correct behavior.
**      22-Jun-1999 (horda03)
**          Cross-Integrate changes 440975 and 441954.
**      26-Jan-1999 (hanal04, horda03)
**	    Add support for shared access to multiprocess semaphores.
**	    b95062.
**	18-May-1999 (hanal04, horda03)
**	    Check for deadlock even if the server state is CS_INITIALIZING.
**	    b96998.
**	14-Apr-2000 (devjo01)
**	    Cross changes for b95607 from 1.2, and enhance them to address
**	    INGSRV1153/b101216.
**      16-Aug-2000 (horda03)
**          Only schedule the next thread when the server supports Ingres
**          threads.
**          (102351)
**      02-Aug-2001 (horda03) Bug 95062
**          If there is a single CPU on the system, then need to relinquish the
**          CPU if the mutex is held by a sessionin a different process.
**      14-Mar-2003 (bolke01) Bug 110235 INGSRV 2263
**	    When returning from CSp_semaphore always call CS_move_async() if
**	    the Cs_srv_block.cs_async flag is set.
**	23-sep-2004 (abbjo03)
**	    Remove tests for CS_ABORTED_MASK.
**	12-may-2005 (abbjo03)
**	    Back out 23-sep-2004 change because it interferes with CUT.
**	02-may-2007 (jonj/joea)
**	    Replace CS_test_set/CS_clear_sem by CS_TAS/CS_ACLR.  Remove
**	    #ifdef VMS clutter.
**	25-may-2007 (jonj/joea)
**	    For cross-process semaphores, allow CPU yields after rescheduling.
*/
STATUS
IICSp_semaphore(
i4                exclusive,
CS_SEMAPHORE       *sem)
{
    CS_SCB              *scb = Cs_srv_block.cs_current;
    CS_SCB              *newscb, *next_scb;
    i4                 new_priority;
    i4                 save_priority;
    i4                 status;
    i4             loops = 0;
    i4             yield_loops = 0;
    i4             yield_cpu_loops = 0;
    i4             real_loops;
    PID                 owner_pid;
    i4                 yield_loops_dlock = 0;
    i4                 inner_trace_count = 0;
    i4                 trace_count = 0;
    i4                 sleep_count = 0;
    i4                 dlock_count = 0;
#ifdef xDEBUG
    i4                 checked_for_asts = 0;
#endif
    CL_ERR_DESC          error;
    char                err_buffer[150];
    TM_STAMP            tim;
    i4                 schedule_next_thread, retune_priority;
    i4			cpu_yields;

    if ((Cs_srv_block.cs_state != CS_INITIALIZING) &&
        (sem->cs_type == CS_SEM_SINGLE) &&
        (sem->cs_owner != scb))
    {
        for (;;)
        {
           Cs_srv_block.cs_inkernel = 1;
           scb->cs_inkernel = 1;
           if (sem->cs_value == 0)
           {
              if (exclusive && (sem->cs_count == 0))
              {
                 sem->cs_value = 1;
                 sem->cs_owner = scb;
                 scb->cs_sem_count++;
 
                 Cs_srv_block.cs_smstatistics.cs_smx_count++;
                 break;
              }
              else if (!exclusive && !sem->cs_excl_waiters)
              /* grant share only if there are no exclusive waiters */
              {
                 sem->cs_count++;
                 scb->cs_sem_count++;
 
                 Cs_srv_block.cs_smstatistics.cs_sms_count++;
 
                 /* Now, go move everyone back to ready */
                 while (next_scb = sem->cs_next)
                 {
                    if (next_scb->cs_state == CS_MUTEX)
                    {
                       next_scb->cs_state = CS_COMPUTABLE;
                       Cs_srv_block.cs_ready_mask |=
                             (CS_PRIORITY_BIT >> next_scb->cs_priority);
                    }
#ifdef  xDEBUG
                    else
                            TRdisplay("CS_MUTEX p BUG (tell CLF/CS owner): scb:%p, cs_state: %w%<(%x)\n",
                                    next_scb,
                                    cs_state_names,
                                    next_scb->cs_state);
#endif
                    sem->cs_next = next_scb->cs_sm_next;
                    next_scb->cs_sm_next = 0;
                 }
                 break;
 
              }
              else /* cannot have right now.  Wait */
              {
                 sem->cs_excl_waiters = 1; /* at least one excl waiter */
                 Cs_srv_block.cs_smstatistics.cs_smsx_count++;
              }
           }
           else /* if (sem->cs_value == 0) */
           {
              Cs_srv_block.cs_smstatistics.cs_smxx_count++;
           }
#ifdef xDEBUG
                {
                    i4             enbflg;
 
                    checked_for_asts += 1;
 
                    if (checked_for_asts >= 100)
                    {
                        lib$signal(SS$_DEBUG);
                    }
 
                    /* If we are in the middle of processing an ast then 
                    ** signal the debugger, since this will cause this 
                    ** routine to infinite loop.
                    */
                    if (lib$ast_in_prog() != 0)
                    {
                        lib$signal(SS$_DEBUG);
 
                        /* EJLFIX: Make a new error message for this */
                        return(E_CS0017_SMPR_DEADLOCK);
                    }
 
                    /* if AST's are disabled then signal the debugger for the 
                    ** same reason as above.
                    */
                    enbflg = 1;
                    if (sys$setast(enbflg) == SS$_WASCLR)
                    {
                        lib$signal(SS$_DEBUG);
 
                        /* now reset the AST level to the way it was. We 
                        ** don't want to turn off AST's when someone has 
                        ** turned them on.
                        */
                        enbflg = 0;
                        sys$setast(enbflg);
 
                        /* EJLFIX: Make a new error message for this */
                        return(E_CS0017_SMPR_DEADLOCK);
                    }
                }
#endif
           scb->cs_state = CS_MUTEX;
           scb->cs_sm_next = sem->cs_next;
           sem->cs_next = scb->cs_self;
           if ( !exclusive ) 
              scb->cs_mask |= CS_SMUTEX_MASK;
           scb->cs_sync_obj = (i4)sem;
 
           scb->cs_inkernel = 0;
           Cs_srv_block.cs_inkernel = 0;
           if (Cs_srv_block.cs_async)
              CS_move_async(scb);
 
           CS_stall();
           
           /* Turn inkernel on while changing cs_mask */
           Cs_srv_block.cs_inkernel = 1;
           scb->cs_inkernel = 1;

	   scb->cs_mask &= ~CS_SMUTEX_MASK;

           if (scb->cs_mask & CS_ABORTED_MASK)
           {
              Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);
              scb->cs_inkernel = 0;
              Cs_srv_block.cs_inkernel = 0;
              if (Cs_srv_block.cs_async)
                 CS_move_async(scb);
              return(E_CS000F_REQUEST_ABORTED);
           }
        } /* for (;;) */
 
        scb->cs_inkernel = 0;
        Cs_srv_block.cs_inkernel = 0;
        if (Cs_srv_block.cs_async)
            CS_move_async(scb);
        return(OK);
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
        schedule_next_thread = FALSE;
        if (scb)
        {
              save_priority = new_priority = scb->cs_priority;
	      retune_priority = FALSE;
        }

        yield_loops = loops = 0;
        cpu_yields = 0;
 
        Cs_srv_block.cs_inkernel = 1;
	for (real_loops = 0; ; real_loops++)
        {
           /*
           ** Don't actually do the TEST/SET if the semaphore is already
           ** owned.  This avoids locking the bus for the interlocked
           ** instruction during the busy loop.
           */
           if ((sem->cs_test_set == 0) && (CS_TAS(&sem->cs_test_set)))
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
		    /*
		    ** Note that this session should not be swapped out if the 
		    ** quantum timer goes off while holding this semaphore.
		    ** Also reset priority if needed.
		    **
		    ** Turn on inkernel field in order to update scb fields.
		    */
                    scb->cs_inkernel = 1;
                    if (scb->cs_priority != save_priority)
                    {
                       CS_change_priority(scb, save_priority);
                    }
                    scb->cs_mask |= CS_NOSWAP;
                    scb->cs_sem_count++;
                    scb->cs_inkernel = 0;
                 } 
                 Cs_srv_block.cs_inkernel = 0;
                 if (Cs_srv_block.cs_async && scb)
                    CS_move_async(scb);
                 return(OK);
              } /* for (;;) */
           }
           /* 
           ** in_kernel for Server is currently set to 1 
           */
           /*
           ** Semaphore is blocked.
           **
           ** Spin/redispatch til it becomes free.
           */
           Cs_srv_block.cs_smstatistics.cs_smcl_count++;
           if (loops++ == 0)
                Cs_srv_block.cs_smstatistics.cs_smcx_count++;
            
           /*
           ** If semaphore is held by another thread in this same process,
           ** then put this waiting thread at the end of the ready queue so
           ** the holding thread can be activated again to release the sem.
           ** Make sure we wait at a similar or lower priority than the holder,
           ** so that we don't loop forever while the holder never gets run.
           */
           if (scb)
           {
              if (sem->cs_pid == Cs_srv_block.cs_pid)
              {
                 /*
                 ** Semaphore owned by another thread in the same process.
                 **
                 ** Immediately redispatch another thread.
                 */
                 if (sem->cs_owner == scb)
                 {
                    Cs_srv_block.cs_inkernel = 0;
                    if (Cs_srv_block.cs_async)
                        CS_move_async(scb);
                    dump_deadlocked_semaphore(sem, 1);
                    return(E_CS0017_SMPR_DEADLOCK);
                 }
 
                 /*
                 ** Check priority of semaphore holder, must have inkernel set.
                 ** Also check for a possbilbe bug 86090 situation and lower
                 ** current threat priority by 4
                 */
 
                 if (sem->cs_owner && sem->cs_owner->cs_state == CS_COMPUTABLE)
                 {
                    new_priority = sem->cs_owner->cs_priority;
		    retune_priority = FALSE;
                 }

                 if (real_loops > (CS_MAX_SEM_LOOPS - 1000))
                 {
                    /* Waiting on non-computable thread for too long, force retune. */
	            retune_priority = TRUE;
                 }

                 schedule_next_thread = TRUE;
              }
              else
              {
                 /* We may need to allow other threads in this server to execute when
                 ** the mutex is owned by another process.
		 **
                 ** Schedule another thread if we're not initializing,
		 ** and the mutex pid "owner" is null, or we've been reduced
		 ** to hibernating. Even if we have to hibernate, there may
		 ** be threads on this process which must run in order for the
		 ** blocking process to be able to release the mutex that we need.
                 */
		 if ( (!sem->cs_pid || cpu_yields) && Cs_srv_block.cs_state != CS_INITIALIZING )
		     schedule_next_thread = TRUE;
		else
		     schedule_next_thread = FALSE;


                 /* If we need to wait for another thread, and we've been waiting too
                 ** long, then drop the priority of this thread.
                 */

                 if ( schedule_next_thread && (real_loops > (CS_MAX_SEM_LOOPS - 1000)))
                 {
                     /* we waiting on non-computable thread for too long */
	             retune_priority = TRUE;
                 }

                 /* Semaphore is owned by a different process. Or there is a EXCLUSIVE
                 ** request for the semaphore pending.
                 */
 
                 if (Cs_numprocessors <= 1)
                 {
                    /* SP machine */ 
                    Cs_srv_block.cs_smstatistics.cs_smsp_count++;
                    loops = 0;     /* Force yield of CPU */
                 }
                 else if (loops > Cs_max_sem_loops)
                 {
                    /* MP machine */
                    Cs_srv_block.cs_smstatistics.cs_smmp_count++;
 
                    /* Reset loop count to give this thread another "spin"
                    ** quantum to compete with other processes for semaphore.
		    ** Also allow resheduling of another thread in this
		    ** server, since another session in this server may
		    ** be blocking the holder of the mutex in the other
		    ** server.
                    */
		    schedule_next_thread = (Cs_srv_block.cs_state != CS_INITIALIZING);
                    loops = 0;
                 }

                 if ( (!schedule_next_thread) && loops )
                 {
                    /* The semaphore is owned exclusively by another process. We don't
                    ** allow other threads in this process to run as they may get
                    ** access to the semaphore before us, unless excessive spinning
		    ** suggests the presence of a cross process scheduling deadlatch,
		    ** in which case fairness must take back seat to expedience.
                    */
                    continue;
                 }
 
                 /*
                 ** Once in a great while (every 500 spins),
                 ** see if the owning process has died. We could
                 ** check more often, but the knowledge of its demise
                 ** will probably lead to our own untimely passing,
                 ** so we'll try to get as much work done as we can
                 ** in the meantime.
                 */
                 if (yield_loops++ > 500)
                 {
                    yield_loops = 0;
                    if ((owner_pid = sem->cs_pid) != 0 &&
                        PCis_alive(owner_pid) == FALSE &&
                        owner_pid == sem->cs_pid)
                    {
                       /*
                       ** If we had to move down the threads priority, restore it now.
	               */
	               if (scb->cs_priority != save_priority)
	               {
	                   scb->cs_inkernel = 1;
	                   CS_change_priority(scb, save_priority);
	                   scb->cs_inkernel = 0;
	               }
	               Cs_srv_block.cs_inkernel = 0;
                       if (Cs_srv_block.cs_async)
                          CS_move_async(scb);
 
                       return(E_CS0029_SEM_OWNER_DEAD);
                    }
                 }
              }

              if (schedule_next_thread)
              {
                 /* The Mutex is held by another thread in this process
                 ** so give it a chance to RUN, or prolonged delay in
	         ** granting mutex suggests a deadlock caused by this
		 ** session preventing another session in this server
		 ** from running and releasing a resource which may
		 ** be the reason the session we are waiting on is
		 ** blocked.
                 */

	         /*
	         ** If indicated, retune session priority.
		 **
		 ** Basically, we will incrementally reduce priority,
		 ** until we reach our minimum.  If already at minumum,
		 ** we go back to our original priority, and start process
		 ** again, until semaphore is granted, or some other exit
		 ** condition applies.
	         **
	         ** Retuning is indicated if we've have to yield to 
		 ** another thread more than once, and we don't know
		 ** the exact priority of the blocking thread, or
	         ** we've been spinning a long, long time.
	         */
		 if ( retune_priority )
		 {
		    if ( scb->cs_priority <= CS_MIN_REDUCED_PRIORITY )
		    {
		       /* 
		       ** We've already been incrementally reduced to the min,
		       ** jump back up to the saved priority, to see if it
		       ** will help.  We'll walk back down the priorities
		       ** if this was useless.
		       */
		       new_priority = save_priority;
                    }
		    else
		    {
                       new_priority = scb->cs_priority - CS_PRIORITY_DECREMENT;
                       if (new_priority < CS_MIN_REDUCED_PRIORITY)
                           new_priority = CS_MIN_REDUCED_PRIORITY;
                    }
                 }

                 /*
                 ** If we want to adjust priority of this thread, do it now.
                 */
                 if (scb->cs_priority != new_priority)
                 {
#ifdef xDEBUG
      		    TRdisplay("%@ TR: Lower priority from %d to %d to avoid CS deadlock \n",
                        scb->cs_priority, new_priority);
#endif
                    scb->cs_inkernel = 1;
                    CS_change_priority(scb, new_priority);
                    scb->cs_inkernel = 0;
                 }

                 /*
                 ** Allow next runnable thread to continue - when this thread
                 ** is selected again to run it will retry the semaphore.
                 ** Turn off inkernel / turn back on when regain control.
                 **
                 ** Set this thread's cs_mask to CS_MUTEX_MASK,
                 ** leave its state as COMPUTABLE, and point the
                 ** scb to the semaphore we're trying to snatch.
                 ** This to provide some useful info about why
                 ** this thread is computable but not apparently
                 ** doing anything.
                 */
		 /* 
                 ** Reduce setting / unsetting of in_kernel flags
                 ** by keeping in_kernel set over !exclusive test
                 */
                 scb->cs_inkernel = 1;
                 scb->cs_mask |= CS_MUTEX_MASK;

                 if ( !exclusive ) 
		 { /* kept in_kernel prior to !exclusive test */
			 scb->cs_mask |= CS_SMUTEX_MASK;
		 }
                 scb->cs_inkernel = 0;
                 Cs_srv_block.cs_inkernel = 0;
                 scb->cs_sync_obj = (i4)sem;
                 if (Cs_srv_block.cs_async)
                    CS_move_async(scb);

                 CS_stall();

		 /* Moved the setting of the Cs_srv_block.cs_inkernel above
		 ** setting the cs_mask, changes to the cs_mask should be
		 ** protected.  Added the scb->cs_inkernel protection
		 */
                 Cs_srv_block.cs_inkernel = 1;

                 scb->cs_inkernel = 1;
                 scb->cs_mask &= ~(CS_MUTEX_MASK|CS_SMUTEX_MASK);


                 if (scb->cs_mask & CS_ABORTED_MASK)
                 {
                    if (scb->cs_priority != save_priority)
                    {
                       CS_change_priority(scb, save_priority);
                    }
		    scb->cs_inkernel = 0;
                    Cs_srv_block.cs_ready_mask
                            |= (CS_PRIORITY_BIT >> scb->cs_priority);
                    Cs_srv_block.cs_inkernel = 0;
                    if (Cs_srv_block.cs_async)
                       CS_move_async(scb);

                    return(E_CS000F_REQUEST_ABORTED);
                 }
                 else
                 {
                       scb->cs_inkernel = 0;
                 }
		 /*
		 ** We've had to yield at least once, change default to
		 ** allow retuning of priority.
		 */
		 retune_priority = TRUE;

		 /* After rescheduling, allow cpu_yields again */
		 cpu_yields = 0;
              }
           }
 
           /* 
           ** in_kernel for Server is current set to 1 
           */
           if ( (schedule_next_thread || Cs_srv_block.cs_state == CS_INITIALIZING) 
			&& trace_count++ > Cs_max_sem_loops)
           {
               trace_count = 0;
               if (inner_trace_count++ > Cs_max_sem_loops)
               {
                    /* display message to help to diagnose a possible hang */
                    TMget_stamp(&tim);
                    TMstamp_str(&tim,err_buffer);
                    STprintf(err_buffer+STlength(err_buffer),
        " Sem Wait: pid %x ses %x, spid %x sses %x adr %x t %d s %d c %d %0.30s ",
        Cs_srv_block.cs_pid, scb, sem->cs_pid, sem->cs_owner, sem, sem->cs_type,
        sem->cs_test_set, sem->cs_count, sem->cs_sem_name);
                    ERlog(err_buffer, STlength(err_buffer), &error);

                    inner_trace_count = 0;
                }
           }
 
           /*
           ** If the semaphore is held for much longer than is normally
           ** expected, then rather than hogging up all the CPU on the
           ** machine, try sleeping for a short period between attempts.
           ** Turn off inkernel / turn back on when regain control.
           */
 
           /* If schedule_next_thread is false then sem->cs_pid is not
           ** equal to this process' pid and it is non-zero. Therefore
           ** semaphore exclusively owned bu another process. On an SP
           ** machine we should reschedule asap.
           */

           if ( ((Cs_numprocessors <= 1) && (schedule_next_thread == FALSE)) ||
                (yield_cpu_loops++ > CS_MAX_SEM_LOOPS) )
           {
              yield_cpu_loops = 0;

              if ((Cs_srv_block.cs_state == CS_INITIALIZING) || !scb)
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
                 ** from a non-dbms server (ie. the rcp, acp, createdb,...).
                 ** We never want to call CSsuspend() in this case, so
                 ** we will sleep awhile so as not to chew up all cycles on
                 ** the machine.
                 */
                 if (Cs_srv_block.cs_state == CS_INITIALIZING)
                 {
                    Cs_srv_block.cs_smstatistics.cs_smnonserver_count++;
                    if (scb)
                    {
                       scb->cs_inkernel = 1;
                       scb->cs_mask |= CS_NOSWAP;
                       scb->cs_inkernel = 0;
               
                       yield_cpu_time_slice(Cs_srv_block.cs_pid, Cs_desched_usec_sleep);
                     
                       scb->cs_inkernel = 1;
                       scb->cs_mask &= ~CS_NOSWAP;
                       scb->cs_inkernel = 0;
                    }
                    else
                    {
                       yield_cpu_time_slice(Cs_srv_block.cs_pid, Cs_desched_usec_sleep);
                    }
                 }
                 continue;
              }
 
              /*
              ** We can't CSsuspend; a CSresume may be pending or about to
              ** occur. If there are other runnable threads, we'd like to
              ** let them continue to run, but CS_ssprsm() calls have looped
              ** a LONG time at this point (CS_MAX_SEM_LOOPS already), so
              ** we'll instead force the entire server to sleep for 
              ** Cs_desched_usec_sleep seconds and then try again. (B37501)
              */

              if (scb)
              {
                 scb->cs_inkernel = 1;
                 scb->cs_mask |= CS_NOSWAP;
                 scb->cs_inkernel = 0;
 
                 yield_cpu_time_slice(Cs_srv_block.cs_pid, Cs_desched_usec_sleep);
 
                 scb->cs_inkernel = 1;
                 scb->cs_mask &= ~CS_NOSWAP;
                 scb->cs_inkernel = 0;
              }
              else
	      {
                 yield_cpu_time_slice(Cs_srv_block.cs_pid, Cs_desched_usec_sleep); 
              }
	      cpu_yields++;

              /* reset counters so that long time sleeper still gets a
              ** chance to get semaphore.
              */
              loops = 0;
              real_loops -= Cs_max_sem_loops;
           }
           else
              continue;
        } /* for(real_loops = 0;;real_loops++)  */
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
 
    return(OK); 
}

/*{
** Name: CSv_semaphore	- Perform a "V" operation, releasing the semaphore
**
** Description:
**      This routine is used by a running thread to release a semaphore which
**      it currently owns.  The act of doing this decrements the thread's
**	owned semaphore count.  It will allow the threads awaiting this
**	semaphore to try once again.
**
**	If the semaphore type is CS_SEM_MULTI then it is a cross process
**	semaphore.  The semaphore may be kept in shared memory and there may
**	be threads in other servers waiting on its release.
**
** Inputs:
**      sem                             A pointer to the semaphore in question.
**
** Outputs:
**      none
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
**      05-Nov-1986 (fred)
**          Created on Jupiter.
**	31-oct-1988 (rogerk)
**	    Make sure we don't exit without resetting cs_inkernel.
**	 7-nov-1988 (rogerk)
**	    Added new inkernel protection.  Set cs_inkernel flag in both
**	    the server block and scb to make the routine behave as if atomic.
**	    When done, turn off the inkernel flags and check to see if we
**	    CS_move_async is needed.
**	28-feb-89 (rogerk)
**	    Added cross-process semaphore support.
**	20-mar-89 (rogerk)
**	    Zero cs_owner field when release cross-process semaphore to fix
**	    race condition.
**	10-apr-89 (rogerk)
**	    Set inkernel while releasing cross process semaphore to prevent us
**	    from thread switching in the middle of setting the pid/owner fields.
**	17-apr-89 (rogerk)
**	    Added CS_clear_sem macro to unset semaphore value with an
**	    interlocked instruction.
**	 5-sep-89 (rogerk)
**	    Turn off scb status that indicates the session should not be swapped
**	    out while holding cross process semaphore.
**	4-jun-1992 (bryanp)
**	    Made Mike's 23-mar-1990 Unix CL changes to this file. His comments:
**
**	    Fixes to this file were necessary to make these semaphores work
**	    correctly when called from the dmfrcp, rather than the server.
**
**          - Changed the code to refer to "Cs_srv_block.cs_pid" rather than
**            to "scb->cs_pid" when it wanted the current process' pid.  The
**            old method did not work for some situations in non-dbms processes.
**          - Some code refered to the "scb" control block to keep track of
**            performance stats.  This code was changed to not be called in
**            the cases where the "scb" did not exist (was NULL).
**
**          If the state is CS_INITIALIZING then this is being called
**          from a non-dbms server (ie. the rcp, acp, createdb, ...).
**          We never want to call CSsuspend() from CSp_semaphore in this case,
**          so the code will spin on the semaphore instead.
**	27-jul-1995 (dougb)
**	    Since CS_ssprsm() will ignore priorities without the ready bit set
**	    and will clear the bit for priorities where nothing is runable,
**	    make sure the ready mask is always consistent when not inkernel.
**	    CS_move_async() now handles a NULL parameter, remove conditions
**	    from calls to this routine.
**	30-jul-1996 (boama01)
**	    Use new cs_test_set fld in sem instead of old cs_value for MULTI
**	    sems; the old fld would not allow Memory Barrier BBCCI instruction
**	    in CS_clear_sem() to block concurrent multiprocess access on AXP;
**	    the new fld is sized and aligned to permit correct behavior.
**      29-Jan-1999 (hanal04, horda03)
**          Add support for shared access to multiprocess semaphores.
**          b95062.
**	15-apr-2000 (devjo01)
**	    Make sure NOSWAP is reset, once last semaphore is released.  This
**	    was not so important in the past, since Repent and Idle SCB's
**	    would never obtain a semaphore, and thus the wake in CS_quantum
**	    would never be suppressed by this flag.   With the addition of
**	    the "CP event sem", ANY thread might get this bit set.   Bit is
**	    sort of useless now, since CS_quantum NEVER preemptively switches
**	    sessions (CS_ast_ssprsm is a NOP), so it is VERY tempting to 
**	    speed up and simplify this by ripping it out entirely.
**      14-Mar-2003 (bolke01) Bug 110235 INGSRV 2263
**	    When returning from CSv_semaphore always call CS_move_async() if
**	    the Cs_srv_block.cs_async flag is set.
**	02-may-2007 (jonj/joea)
**	    Replace CS_test_set/CS_clear_sem by CS_TAS/CS_ACLR.  Remove
**	    #ifdef VMS clutter.
*/
STATUS
IICSv_semaphore(
CS_SEMAPHORE       *sem)
{
    CS_SCB              *scb = Cs_srv_block.cs_current;
    CS_SCB              *newscb, *next_scb;
 
    Cs_srv_block.cs_inkernel = 1;       /* ensure inkernel in on */
    if (scb)
        scb->cs_inkernel = 1;
 
    if ((Cs_srv_block.cs_state != CS_INITIALIZING) &&
        (sem->cs_type == CS_SEM_SINGLE))
    {
        if (scb->cs_sem_count == 0)
        {
            scb->cs_sem_count = 0;
	    /* reversed order of resetting inkernal flags to match usage */
            scb->cs_inkernel = 0;
            Cs_srv_block.cs_inkernel = 0;
            if (Cs_srv_block.cs_async)
               CS_move_async(scb);
            return(E_CS000A_NO_SEMAPHORE);
        }
 
        if (sem->cs_value)
        {
            /* if held exclusively, then just release */
            sem->cs_value = 0;
            sem->cs_owner = 0;
        }
        else
        {
            if (sem->cs_count-- == 0)
            {
                sem->cs_count = 0;
                scb->cs_inkernel = 0;
                Cs_srv_block.cs_inkernel = 0;
                if (Cs_srv_block.cs_async)
                    CS_move_async(scb);
                return(E_CS000A_NO_SEMAPHORE);
            }
        }
        if (sem->cs_count == 0)
        { /* move waiters back to ready */
            sem->cs_excl_waiters = 0; /* all get an equal opportunity */
            while (next_scb = sem->cs_next)
            {
                if (next_scb->cs_state == CS_MUTEX)
                {
                    next_scb->cs_state = CS_COMPUTABLE;
                    Cs_srv_block.cs_ready_mask |=
                        ( CS_PRIORITY_BIT >> next_scb->cs_priority );
                }
#ifdef  xDEBUG
                else
                    TRdisplay("CS_MUTEX v BUG (tell CLF/CS owner): scb: %x, cs_state: %w%<(%x)\n",
                            next_scb,
                            cs_state_names,
                            next_scb->cs_state);
#endif
                sem->cs_next = next_scb->cs_sm_next;
                next_scb->cs_sm_next = 0;
            }
        }

        /* Make sure we can be swapped out if quantum exhausted,
        ** once our last mutex is released.
        */
	if ( --scb->cs_sem_count == 0) scb->cs_mask &= ~CS_NOSWAP;
        scb->cs_inkernel = 0;
        Cs_srv_block.cs_inkernel = 0;
        if (Cs_srv_block.cs_async)
            CS_move_async(scb);
        return(OK);
    }
    else if (sem->cs_type == CS_SEM_MULTI)
    {
        /* 
        ** If scb is not NULL then scb->cs_inkernel will be set to 1
        */
        /*
        ** Cross proces semaphore.
        ** Release the semaphore value and reset cs_owner/cs_pid.
        ** Any waiters for this lock will be spinning trying to acquire it.
        ** The first to request it after we reset the value will be granted.
        **
        ** Note that waiters cannot be suspended in MUTEX state in order to
        ** wait for this semaphore as we are not able to wake them up when we
        ** release the semaphore since they may be in a different process.
        */
 
        if ((sem->cs_test_set) && (sem->cs_count == 0))
        {
            /*
            ** Held exclusively. Make sure this thread holds the
            ** semaphore.
            */
 
            if ((sem->cs_owner == scb) &&
                (sem->cs_pid == Cs_srv_block.cs_pid))
            {
                /*
                ** Reset cs_owner/cs_pid
                */
                sem->cs_owner = NULL;
                sem->cs_pid = 0;
            }
            else                   
            {
 
               if (scb)
               {
                   scb->cs_inkernel = 0;
		   /* moved resetting of Cs_srv_block.cs_inkernel into if  
                   ** statement and also added an extra one in an else statement
		   ** statement if there is no scb
		   */
                   Cs_srv_block.cs_inkernel = 0;
 
                   if (Cs_srv_block.cs_async)
                      CS_move_async(scb);
                }
		else
		{
                   Cs_srv_block.cs_inkernel = 0;
		}

                return(E_CS000A_NO_SEMAPHORE);
            }
        }
        else
        {
            if (scb && scb->cs_sem_count == 0)
            {
		/* reversed order of inkernal resets*/
                scb->cs_inkernel = 0;
                Cs_srv_block.cs_inkernel = 0;
                if (Cs_srv_block.cs_async)
                    CS_move_async(scb);
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
                scb->cs_inkernel = 0;
                Cs_srv_block.cs_inkernel = 0;
                if (Cs_srv_block.cs_async)
                    CS_move_async(scb);
                return(E_CS000A_NO_SEMAPHORE);
            }
        }
 
        /*
        ** Release the semaphore value.
        */
        CS_ACLR(&sem->cs_test_set);
 
        if (scb)
        {
	    if ( (--scb->cs_sem_count) == 0) 
            { 
               scb->cs_mask &= ~CS_NOSWAP;
	    }
            /* moved Cs_srv_block.cs_inkernel after scb->cs_inkernel 
	    ** and added an else condition
	    */
            scb->cs_inkernel = 0;
            Cs_srv_block.cs_inkernel = 0;
            if (Cs_srv_block.cs_async)
                CS_move_async(scb);
        }
	else
	{
             Cs_srv_block.cs_inkernel = 0;
	}
        return(OK);
    }
    else if (sem->cs_owner == (CS_SCB *)1)
    {
        sem->cs_owner = 0;
        sem->cs_next = 0;
        if (scb)
        {
	    /* moved Cs_srv_block.cs_inkernel after scb->cs_inkernel 
	    ** and added an else condition
	    */
            scb->cs_inkernel = 0;
            Cs_srv_block.cs_inkernel = 0;
            if (Cs_srv_block.cs_async)
                CS_move_async(scb);
        }
	else
	{
	    Cs_srv_block.cs_inkernel = 0;
	}
        return(OK);
    }
    else
    {
        /*
        **  Something's wrong, e.g., thread does not own semaphore.
        */
        if (scb)
        {
	    /* moved Cs_srv_block.cs_inkernel after scb->cs_inkernel 
	    ** and added an else condition
	    */
            scb->cs_inkernel = 0;
            Cs_srv_block.cs_inkernel = 0;
            if (Cs_srv_block.cs_async)
                CS_move_async(scb);
        }
	else
	{
            Cs_srv_block.cs_inkernel = 0;
	}
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
**      28-Feb-1989 (rogerk)
**          Created for Terminator.
**	4-jul-92 (daveB)
**	    Add to new cs_sem_tree, and clear out new name field.
**	9-Jul-92 (daveb)
**	    Sem_tree won't work, use CS_sem_attach scheme instead.
**	2-jan-96 (dougb)
**	    Cross-integrate following Unix change (move attach to CSn):
**		17-May-1994 (daveb) 59127
**	    Move attach to name; only named sems will be attached.
**	    Initialize all fields to zero, and only explicitly fill
**	    in those that may be non-zero.  This will clear out the
**	    stats fields.  Add ifdef-ed out leak-diagnostic call
*/
STATUS
IICSi_semaphore(
CS_SEMAPHORE	    *sem,
i4		    type)
{
    if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
	return (E_CS0004_BAD_PARAMETER);

    /* Clear everything out, including stats */
    MEfill( sizeof(*sem), 0, (char *)sem ); 

    /* Then do stuff that may be non-zero */
    sem->cs_type = type;
    sem->cs_sem_init_pid = Cs_srv_block.cs_pid;
    sem->cs_sem_init_addr = sem;
    sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

    return (OK);
}

/*{
** Name: CSw_semaphore	- Initialize and Name Semaphore
**			  in one felled swoop.
**
** Description:
**	This routine is used to initialize a semaphore for use by the
**	CSp_semaphore and CSv_semaphore routines.  This should always
**	be called before the first use of a semaphore and needs only
**	be called once.  It is illegal to call CSw_semaphore on a
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
**	02-jan-1996 (dougb)
**	    Copied from Unix CL (part of change 417555):
**		24-Apr-1995 (jenjo02)
**          Created.
*/
STATUS
IICSw_semaphore(CS_SEMAPHORE *sem, i4 type, char *string)
{
    if ((type != CS_SEM_SINGLE) && (type != CS_SEM_MULTI))
	return (E_CS0004_BAD_PARAMETER);

    /* Clear everything out, including stats */
    MEfill( sizeof(*sem), 0, (char *)sem ); 

    /* Then do stuff that may be non-zero */
    sem->cs_type = type;
    sem->cs_sem_init_pid = Cs_srv_block.cs_pid;
    sem->cs_sem_init_addr = sem;
    sem->cs_sem_scribble_check = CS_SEM_LOOKS_GOOD;

    STmove( string, EOS, sizeof( sem->cs_sem_name ), sem->cs_sem_name );
    
    return( OK );
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
**	3-jun-1992 (bryanp)
**	    Created. VMS will use a no-op routine for now.
*/
STATUS
IICSr_semaphore(CS_SEMAPHORE *sem)
{

    sem->cs_sem_scribble_check = CS_SEM_WAS_REMOVED;
    return (OK);
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
**	2-jan-96 (dougb)
**	    Cross-integrate following Unix change.  Further, make the
**	    semaphore validity checks consistent.
**		17-May-1994 (daveb) 59127
**	    Check validity of semaphore as best you can before
**	    attaching.
*/
STATUS
IICSa_semaphore(CS_SEMAPHORE *sem)
{
    STATUS clstat = OK;

    if ( CS_SEM_MULTI == sem->cs_type
	&& EOS != *sem->cs_sem_name
	&& CS_SEM_LOOKS_GOOD == sem->cs_sem_scribble_check )
	clstat =  OK;

    return( clstat );
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
**	    created on VMS.
**	2-jan-96 (dougb)
**	    Make the semaphore validity checks consistent.
*/
STATUS
IICSd_semaphore(CS_SEMAPHORE *sem)
{
    STATUS clstat = OK;

    return( clstat );
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
**	2-jan-96 (dougb)
**	    Cross-integrate following Unix change.  Further, make the
**	    semaphore validity checks consistent.
**		17-May-1994 (daveb) 59127
**	    Attach as sem is named, if sem looks good and has
**	    no name already.  Check to see it looks good.
*/
void
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
**	15-feb-1996 (duursma)
**	    Added to Alpha CL (partial integration of 423116).
**	    Gutted until we decide what to do with semaphore stats on VMS.
**      29-Jan-1999 (hanal04, horda03)
**          At the very least we should ensure the semaphore name is
**          copied to the stats variable as this is used to report
**          mutex errors in LG/LK.
*/
STATUS
IICSs_semaphore(
    i4		 options,
    CS_SEMAPHORE *sem,
    CS_SEM_STATS *stats,
    i4		 length )
{
    MEcopy(sem->cs_sem_name, CS_SEM_NAME_LEN, stats->name);
    return( OK );
}

/*{
** Name: CSadd_thread - Add a thread to the server space.
**
** Description:
**      This routine is called to add a thread to the server known thread 
**      list.  The thread is created, placed in the ready queue at the 
**      appropriate priority, and is placed in competition for resources. 
**      The only parameter given specifies the priority for this thread. 
**      A value of zero (0) for this parameter indicates that "normal" 
**      priority is to be used. 
** 
**      This routine performs queue manipulation functions, and, therefore, 
**      must run in such a manner as to not be interrupted.
**
** Inputs:
**      priority                        Priority for new thread.  A value
**                                      of zero (0) indicates that "normal"
**                                      priority is to be used.
**      crb                             A pointer to the connection
**                                      request block for the thread.
**                                      This is the block which was sent to
**                                      request that a new thread be added.
**					This is a buffer that was passed to
**					(*cs_saddr)() and is provided to the
**					scb allocator for transfer of
**					interesting information to the newly
**					created scb.
**	thread_type			Non-negative value by which caller can
**					specify to the thread initiate code
**					what the function of the thread is.  A
**					type of zero indicates a normal user
**					thread (this is the type value that CS
**					uses when it wants to add a new user).
**					The meaning of any other values is
**					coordinated between the add_thread
**					caller and SCF.
**
** Outputs:
**      thread_id                       optional pointer to CS_SID into
**                                      which new thread's SID is returned.
**      error				Filled in with an OS
**					    error if appropriate
** 
**	Returns:
**	    OK, or
**	    E_CS0002_SCB_ALLOC_FAIL -	Unable to allocate scb for new thread
**	    E_CS0004_BAD_PARAMETER  -	Invalid value for priority
**	    E_CS000E_SESSION_LIMIT  -	Too many threads have been requested
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-Nov-1986 (fred)
**          Created.
**	15-May-1988 (rogerk)
**	    Added thread type argument.
**	    Set AST state to state it was when routine called rather than
**	    always to on.
**	31-oct-1988 (rogerk)
**	    If server is writing accouting records for each session, then
**	    turn on the session mask to collect CPU statistics.
**	 7-nov-1988 (rogerk)
**	    Initialize async portion of scb.
**	28-feb-1989 (rogerk)
**	    Set cs_pid field of scb.
**      23-jul-1991 (rog)
**          Changed sys$asctoid() to sys$getuai() because the former has
**          trouble finding certain UIC's.
**	28-Oct-1992 (daveb)
**	    Attach new threads in CSadd_thread.
**	24-nov-92 (walt)
**	    Moved definition of proc_desc down into this function so it wouldn't
**	    be a global symbol.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
**	24-jan-96 (dougb) bug 71590
**	    Don't set cs_registers[] in this module -- that's done for us by
**	    CS_alloc_stack().
**	20-may-1998 (kinte01)
**	    Cross-integrate Unix change 435120
**	    31-Mar-1998 (jenjo02)
**		Added *thread_id output parm to CSadd_thread().
*/
STATUS
IICSadd_thread(
i4              priority,
PTR             crb,
i4              thread_type,
CS_SID          *thread_id,
CL_ERR_DESC     *error)
{
    STATUS              status;
    CS_SCB		*scb;
    i4			asts_enabled;

    CL_CLEAR_ERR(error);

    if (priority >= CS_LIM_PRIORITY)
    {
	return(E_CS0004_BAD_PARAMETER);
    }
    else if (priority == 0)
    {
	priority = CS_DEFPRIORITY;
    }

    status = (*Cs_srv_block.cs_scballoc)(&scb, crb, 
					thread_type & ~(CS_CLEANUP_MASK));
    if (status != OK)
    {
	return(status);
    }

    /* scb->cs_svcb = Cs_srv_block.cs_svcb;  VALUE MAY NOT BE VALID YET */
    scb->cs_connect = TMsecs();		    /* mark time of connection */

    /*
    ** Now the thread is complete, place it on the appropriate queue
    ** and let it run via the normal course of events.
    */

    scb->cs_type = CS_SCB_TYPE;
    /*
    ** Do NOT fill in client_type here, because it may have been set
    ** by the scb_allocator above.
    */
    scb->cs_owner = (PTR)CS_IDENT;
    scb->cs_thread_type = thread_type & ~(CS_CLEANUP_MASK);
    scb->cs_tag = CS_TAG;
    scb->cs_mask = 0;
    scb->cs_state = CS_COMPUTABLE;
    scb->cs_priority = priority;
    scb->cs_self = (CS_SID)scb;
    scb->cs_mode = CS_INITIATE;
    scb->cs_stk_area = 0;
    scb->cs_pid = Cs_srv_block.cs_pid;
    if ( thread_type & CS_CLEANUP_MASK )
        scb->cs_cs_mask |= CS_CLEANUP_MASK;

    /* Initialise the in_kernel flag in case we get an AST 
    ** called resume before anything happens
    */
    scb->cs_inkernel = 0; 

    if (thread_id)
        *thread_id = (CS_SID)scb;

    /*
    ** Initialize fields used by asynchronous CS routines.
    */
    scb->cs_async = 0;
    scb->cs_asmask = 0;
    scb->cs_asunmask = 0;
    scb->cs_as_q.cs_q_next = 0;
    scb->cs_as_q.cs_q_prev = 0;

    /*
    ** If collecting CPU stats for all dbms sessions, then turn on CPU
    ** statistic collecting.  Otherwise, cpu stats will only be collected
    ** for this thread if a CSaltr_session(CS_AS_CPUSTATS) call is made.
    */
    if (Cs_srv_block.cs_mask & CS_CPUSTAT_MASK)
	scb->cs_mask |= CS_CPU_MASK;

    {
	/* Get VMS UIC for username on whose behalf we e running.  This is  */
	/* used for general accounting and file ownership purposes	    */

	struct dsc$descriptor	desc;
        ILE3  itmlst[] = {
                            { sizeof(scb->cs_uic), UAI$_UIC, &scb->cs_uic, 0 },
                            { 0, 0, 0, 0 }
                    };


        desc.dsc$w_length = sizeof(scb->cs_username);
	desc.dsc$a_pointer = &scb->cs_username;

        status = sys$getuai(0, 0, &desc, itmlst, 0, 0, 0);

	if ((status & 1) == 0)
	    scb->cs_uic = 0;
    }

    /*
    ** Turn off AST's while we set up this thread to be scheduled.
    ** Remember the previous AST state so we can restore it before
    ** returning.  This is necessary since this routine may be called
    ** during server initialization - when AST's must be kept off until
    ** the server is ready to do task switching.
    */
    status = sys$setast(0);
    asts_enabled = (status == SS$_WASSET);

    status = CS_alloc_stack(scb, error);
    if (status != OK)
    {
#ifdef CS_STACK_SHARING
	if (status == E_CS000B_NO_FREE_STACKS)
	{
	    scb->cs_state = CS_STACK_WAIT;
	}
	else
#endif
	{
	    if (asts_enabled)
		sys$setast(1);
            SETCLERR(error, status, ER_threadcreate);
            TRdisplay("failed to allocate session stack, status = %x\n",
                      status);
            (*Cs_srv_block.cs_elog)(status, error, 0);
            (*Cs_srv_block.cs_reject)(Cs_srv_block.cs_crb_buf, status);
            (*Cs_srv_block.cs_scbdealloc)(scb);
            return status;
	}
    }

    scb->cs_rw_q.cs_q_next =
	Cs_srv_block.cs_rdy_que[scb->cs_priority];
    scb->cs_rw_q.cs_q_prev =
	Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb; 
    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> priority);

    scb->cs_next =
	Cs_srv_block.cs_known_list;
    scb->cs_prev =
	Cs_srv_block.cs_known_list->cs_prev;
    Cs_srv_block.cs_known_list->cs_prev->cs_next = scb;
    Cs_srv_block.cs_known_list->cs_prev = scb;

    /* Increment session count.  If user thread increment user session count. */
    if (scb->cs_thread_type != CS_INTRNL_THREAD)
    {
       Cs_srv_block.cs_num_sessions++;
       if (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active)
          Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
       if (thread_type == CS_USER_THREAD)
	  Cs_srv_block.cs_user_sessions++;
    }
    /* If AST's were previously enabled, then re-enable them */
    if (asts_enabled)
	sys$setast(1);

    if ((Cs_srv_block.cs_state == CS_IDLING)
	    && (scb->cs_state == CS_COMPUTABLE))
	sys$wake(0, 0);

    CS_scb_attach( scb );

    return(OK);
}

/*{
** Name: CSremove	- Cause a thread to be terminated
**
** Description:
**      This routine is used to cause the dispatcher to remove 
**      a thread from competition for resources.  It works by  
**      setting the next state for the thread to CS_TERMINATE 
**      and, from there, letting "nature take its course" in having 
**      each of the threads terminate themselves.  This call is intended
**	for use only by the monitor code.
**
**	Note that this routine does not evaporate a thread at any
**	point other than when it has returned.  The reason that this
**	decision was made is to allow a thread to clean up
**	after itself.  Randomly evaporating threads will inevitably
**	result in a trashed server because the thread which has been
**	evaporated will have been holding some resource.  The CS module,
**	as previously stated, is cooperative;  it cannot go clean up
**	a thread's resources since it does not know what/where they
**	are.
**
** Inputs:
**      sid                             The thread id to remove
**
** Outputs:
**      none
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	thread does not exist
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Nov-1986 (fred)
**          Created
**	 2-Nov-1988 (rogerk)
**	    Be sure to re-enable AST's before returning with error.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
**      09-may-2003 (horda03) Bug 104254
**          Delete via ipm of a session waiting on a mutex results in
**          SIGSEGV. Removed reference to scb->cs_sm_root.
*/
STATUS
IICSremove(
CS_SID  sid)
{
    CS_SCB              *scb;
    CS_SCB		*cscb;

    sys$setast(0);
    scb = CS_find_scb(sid);
    if (scb == 0)
    {
	sys$setast(1);
	return(E_CS0004_BAD_PARAMETER);
    }
    scb->cs_mask |= CS_DEAD_MASK;
    if (scb->cs_state != CS_COMPUTABLE)
    {
	/*
	** In this case, we need to figure out what needs to
	** be remedied in order to fix things up.  If the
	** thread is awaiting a semaphore, it is safe to return
	** an error saying that the request has been aborted.
	** The same should be true of general event wait.
	*/

	if (scb->cs_state == CS_UWAIT)
	{
	    scb->cs_state = CS_COMPUTABLE;
	}
	else if (scb->cs_state == CS_MUTEX)
	{
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_mask |= CS_ABORTED_MASK;
	}
	else if (scb->cs_state == CS_EVENT_WAIT)
	{
	    if (scb->cs_mask & CS_INTERRUPT_MASK)
		scb->cs_state = CS_COMPUTABLE;
	    scb->cs_mask |= CS_ABORTED_MASK;
	}
	else if (scb->cs_state == CS_CNDWAIT)
	{
	    CS_CONDITION	*cp;

	    /* remove from condition queue */
	    cp = scb->cs_cnd;
	    cp->cnd_next->cnd_prev = cp->cnd_prev;
	    cp->cnd_prev->cnd_next = cp->cnd_next;
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_mask |= CS_ABORTED_MASK;
	}
	/* Now job is ready, put him back in the ready queue */
	if (scb->cs_state == CS_COMPUTABLE)
	{
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb->cs_rw_q.cs_q_next;

            /*
            ** Link this scb last in the ready queue:
            */

	    scb->cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[scb->cs_priority];
	    scb->cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);
            if (scb->cs_thread_type != CS_INTRNL_THREAD &&
              (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;

	    if (Cs_srv_block.cs_state == CS_IDLING)
		sys$wake(0, 0);
	}
    }
    sys$setast(1);
    return(OK);
}
/*{
** Name: CScnd_init - initialize condition object
**
** Description:
**	Prepare a CS_CONDITION structure for use.
**
** Inputs:
**	cond			CS_CONDITION structure to prepare
**
** Outputs:
**	cond			Prepared CS_CONDITION structure
**
**	Returns:
**	    OK
**	    not OK		Usage error
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none.
**
** History:
**	25-Jul-90 (anton)
**	    more CL committee changes
**	20-Jun-90 (anton)
**	    CL committee changes
**	11-Apr-90 (anton)
**	    Created.
**	4-jul-92 (daveb)
**	    Insert into cs_cnd_tree, and init the name.
**	10-Dec-1992 (daveb)
**	    Duh, actually initialize the tree key before inserting it!
**	17-Dec-2003 (jenjo02)
**	    Removed MO registration.
*/
STATUS
IICScnd_init(
CS_CONDITION	*cnd)
{
    cnd->cnd_waiter = NULL;
    cnd->cnd_next = cnd->cnd_prev = cnd;
    cnd->cnd_name = "";

    return(OK);
}
/*{
** Name: CScnd_free - free condition object
**
** Description:
**	Release any internal state or objectes related to the condition
**
**	Note that the condition object should not be used again
**	unless re-initialized.
**
** Inputs:
**	cond			CS_CONDITION structure to free
**
** Outputs:
**	none
**
**	Returns:
**	    OK
**	    not OK		Usage error
**				Possibly due to freeing a condition
**				on which some other session is waiting
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none.
**
** History:
**	25-Jul-90 (anton)
**	    more CL committee changes
**	15-Jun-90 (anton)
**	    CL committee changes
**	11-Apr-90 (anton)
**	    Created.
**	4-jul-92 (daveb)
**		Delete from cs_cnd_tree.
**	14-Dec-1992 (daveb)
**	    re-enable the delete, which was commented out for
**	    no good reason.
**	17-Dec-2003 (jenjo02)
**	    Removed MO deregistration.
*/
STATUS
IICScnd_free(
CS_CONDITION	*cnd)
{
    if (cnd->cnd_waiter != NULL || cnd != cnd->cnd_next)
	return(FAIL);	/* FIXME either E_CSxxxx_FAIL or E_CSxxxx_INVALID */

    return(OK);
}
/*{
** Name: CScnd_wait - condition wait
**
** Description:
**	Wait for a condition to be signaled, releasing
**	a semaphore while waiting and getting the semaphore again
**	once woken.  Users of CScnd_wait must be prepared for
**	CScnd_wait to return without the condition actually being
**	met.  Typical use will be the body of a while statement
**	which performs the actual resource check.
**
**	A CScnd_wait may return even when the condition is not met.
**	CScnd_wait may even return without being signaled to wake a waiter.
**	This requires the user to check that whatever the session
**	is waiting for has happened.
**
**	When waiting on a condition, the same mutex should be
**	specified on each wait.	 (i.e. a mutex can be associated with
**	several conditions, but a condition can be associated with
**	only one mutex.)
**
**	Internally to CScnd_wait, a session may be in either of two
**	wait states:
**	    1) waiting for the condition to be signalled or broadcast, and
**	    2) waiting to re-acquire the semaphore.
**
**	These two wait states should be distinguished by CS, so that
**	users of CS (such as iimonitor) can identify in which state a
**	session is currently blocked.
**
** Inputs:
**	cond			CS_CONDITION to wait upon.
**	mutex			CS_SEMAPHORE to release while waiting
**
** Outputs:
**	mutex			CS_SEMAPHORE released and reacquired
**
**	Returns:
**	    OK
**	    not OK			Usage error
**
**	    E_CS000F_REQUEST_ABORTED	Thread was aborted during the wait
**
**	Exceptions:
**	    none
**
** Side Effects:
**	The mutex must be released atomically with the wait upon the
**	condition and reacquired before return.
**
** History:
**	25-Jul-90 (anton)
**	    more CL committee changes
**	15-Jun-90 (anton)
**	    CL committee changes
**	11-Apr-90 (anton)
**	    Created.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**	17-Dec-2003 (jenjo02)
**	    Reacquire mtx even if "aborted". This is like
**	    real conditions work.
**	14-apr-2005 (jenjo03/abbjo03)
**	    Change of 1-dec-2000 missed an instance of cs_memory (or perhaps
**	    it was missed or re-added by cross-integrations).   
*/
STATUS
IICScnd_wait(
CS_CONDITION	*cnd,
CS_SEMAPHORE	*mtx)
{
    CS_CONDITION	cndq;
    CS_SCB		*scb;
    STATUS		status, mstatus;

    if ( !cnd || !mtx)
	return(FAIL);

    cndq.cnd_waiter = scb = Cs_srv_block.cs_current;

    Cs_srv_block.cs_inkernel = 1;
    scb->cs_inkernel = 1;

    scb->cs_state = CS_CNDWAIT;
    scb->cs_cnd = &cndq;
    scb->cs_sync_obj = (PTR)cnd;

    cndq.cnd_name = cnd->cnd_name;
    cndq.cnd_next = cnd;
    cndq.cnd_prev = cnd->cnd_prev;
    cnd->cnd_prev->cnd_next = &cndq;
    cnd->cnd_prev = &cndq;

    /* remove from run queue */

    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
	scb->cs_rw_q.cs_q_prev;
    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
	scb->cs_rw_q.cs_q_next;
    if (scb->cs_thread_type != CS_INTRNL_THREAD)
        Cs_srv_block.cs_num_active--;

    /* if this was the last block, then turn off the ready bit */

    if ((scb->cs_rw_q.cs_q_next ==
	 Cs_srv_block.cs_rdy_que[scb->cs_priority])
	&& (scb->cs_rw_q.cs_q_prev ==
	    Cs_srv_block.cs_rdy_que[scb->cs_priority]))
	Cs_srv_block.cs_ready_mask &=
	    ~(CS_PRIORITY_BIT >> scb->cs_priority);

    /* release the semaphore - normally this will take us out of
       kernel mode - we call this while in kernel in order to release
       the semaphore and wait on the condition in one indivisable
       operation */
    status = CSv_semaphore(mtx);

    if (status || Cs_srv_block.cs_inkernel)
    {
	/* some thing went wrong - abort */

	Cs_srv_block.cs_inkernel = 1;
	scb->cs_inkernel = 1;

	scb->cs_cnd = NULL;
	scb->cs_state = CS_COMPUTABLE;

	cndq.cnd_next->cnd_prev = cndq.cnd_prev;
	cndq.cnd_prev->cnd_next = cndq.cnd_next;

	/* make runable */

	scb->cs_rw_q.cs_q_next =
	    Cs_srv_block.cs_rdy_que[scb->cs_priority];
	scb->cs_rw_q.cs_q_prev =
	    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	Cs_srv_block.cs_ready_mask |=
	    (CS_PRIORITY_BIT >> scb->cs_priority);
	scb->cs_state = CS_COMPUTABLE;
 
        if (scb->cs_thread_type != CS_INTRNL_THREAD &&
          (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
            Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
 
	scb->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;
	if (Cs_srv_block.cs_async)
	{
	    CS_move_async(scb);
	}

	if (status == OK)
	{
	    status = FAIL;
	}
    }
    else
    {
	CS_stall();	/* wait we are restarted and are runable */

	if (scb->cs_mask & CS_ABORTED_MASK)
	    status = E_CS000F_REQUEST_ABORTED;
	mstatus = CSp_semaphore(1, mtx);
    }

    return (status) ? status : mstatus;
}
/*{
** Name: CScnd_signal - condition signal
**
** Description:
**	Signal that at least one waiter on the condition should be set runable.
**
** Inputs:
**	cond			CS_CONDITION to wake up.
**
** Outputs:
**	none
**
**	Returns:
**	    OK
**	    not OK		Usage error
**
**	Exceptions:
**	    none
**
** Side Effects:
**	The woken session will reacquire a semaphore.
**	Typically, the signaler will hold that semaphore when
**	signaling so the woken thread will not really run until the
**	semaphore is released.
**
** History:
**	11-Apr-90 (anton)
**	    Created.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
**	17-Dec-2003 (jenjo02)
**	    Added (optional) SID to prototype so that one
**	    may signal a specific condition waiter rather
**	    the first.
*/
STATUS
IICScnd_signal(
CS_CONDITION	*cnd,
CS_SID		sid)
{
    CS_SCB		*scb;
    CS_CONDITION	*cndp = NULL;
    STATUS		status = OK;

    Cs_srv_block.cs_inkernel = 1;

    if (!cnd || cnd->cnd_waiter != NULL)
	status = FAIL;

    /* Signal specific session? */
    if ( status == OK && sid )
    {
	scb = CS_find_scb(sid);

	/* Error if not waiting on this "cnd" */
	if ( scb && scb->cs_sync_obj == (PTR)cnd &&
	    scb->cs_state == CS_CNDWAIT )
	{
	    cndp = scb->cs_cnd;
	}
	else
	    status = FAIL;
    }
    if (status == OK && cnd->cnd_next != cnd)
    {
	/* If not specific, take the first */
	if ( !cndp )
	    cndp = cnd->cnd_next;

	scb = cndp->cnd_waiter;

	scb->cs_state = CS_COMPUTABLE;
	scb->cs_cnd = NULL;

	cndp->cnd_next->cnd_prev = cndp->cnd_prev;
	cndp->cnd_prev->cnd_next = cndp->cnd_next;

	/* make runable */

	scb->cs_rw_q.cs_q_next =
	    Cs_srv_block.cs_rdy_que[scb->cs_priority];
	scb->cs_rw_q.cs_q_prev =
	    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);
	scb->cs_state = CS_COMPUTABLE;
        if (scb->cs_thread_type != CS_INTRNL_THREAD &&
          (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
            Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
    }

    Cs_srv_block.cs_inkernel = 0;

    if (Cs_srv_block.cs_async)
    {
	CS_move_async(Cs_srv_block.cs_current);
    }

    return status;
}
/*{
** Name: CScnd_broadcast - wake all waiters on a condition
**
** Description:
**	Set runable all waiters on the condition.
**
** Inputs:
**	cond			CS_CONDITION to wake up.
**
** Outputs:
**	none
**
**	Returns:
**	    OK
**	    not OK		Usage error
**
**	Exceptions:
**	    none
**
** Side Effects:
**	The woken sessions will reacquire a semaphore.
**	Typically, the signaler will hold that semaphore when
**	signaling so the woken threads will not really run until the
**	semaphore is released.	And then they will run only as
**	the semaphore becomes available.  See examples.
**
** History:
**	11-Apr-90 (anton)
**	    Created.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
*/
STATUS
IICScnd_broadcast(
CS_CONDITION	*cnd)
{
    CS_SCB		*scb = NULL;
    CS_CONDITION	*cndp;
    STATUS		status = OK;

    Cs_srv_block.cs_inkernel = 1;

    if (cnd->cnd_waiter != NULL)
	status = FAIL;

    if (status == OK)
    {
	while (cnd->cnd_prev != cnd)
	{
	    cndp = cnd->cnd_prev;
	    scb = cndp->cnd_waiter;

	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_cnd = NULL;

	    cndp->cnd_next->cnd_prev = cndp->cnd_prev;
	    cndp->cnd_prev->cnd_next = cndp->cnd_next;

	    /* make runable */

	    scb->cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[scb->cs_priority];
	    scb->cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	    scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	    Cs_srv_block.cs_ready_mask |=
		(CS_PRIORITY_BIT >> scb->cs_priority);
	    scb->cs_state = CS_COMPUTABLE;

            if (scb->cs_thread_type != CS_INTRNL_THREAD &&
              (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
	}
    }

    Cs_srv_block.cs_inkernel = 0;

    if (Cs_srv_block.cs_async)
    {
	CS_move_async(Cs_srv_block.cs_current);
    }

    return status;
}
/*{
** Name: CScnd_name - attach a name to a CS_CONDITION strucuture
**
** Description:
**	Attach a name to a CS_CONDITION for debugging and monitoring
**	uses.
**
** Inputs:
**	cond			CS_CONDITION to name
**	name			String containing name
**				The space for the string must be left
**				until a subsequent CScnd_name or
**				a CScnd_free.
**
** Outputs:
**	none
**
**	Returns:
**	    OK
**	    not OK		Usage error
**
**	Exceptions:
**	    none
**
** Side Effects:
**	The CL may register the condition with a monitoring service
**	not part of CS.
**
** History:
**	25-Jul-90 (anton)
**	    more CL committee changes
**	20-Jun-90 (anton)
**	    Created.
*/
STATUS
CScnd_name(cond, name)
CS_CONDITION	*cond;
char		*name;
{
	cond->cnd_name = name;
	return OK;
}
/*{
** Name: CScnd_get_name - get the name bound to a CS_CONDITION strucuture
**
** Description:
**	Return the name which may have been bound to the CS_CONDITION
**	structure.
**
** Inputs:
**	cond			CS_CONDITION to name
**
** Outputs:
**	none
**
**	Returns:
**		NULL			No name has been bound
**		pointer to string	pointer to the name
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	20-Jun-90 (anton)
**	    Created.
*/
char	*
CScnd_get_name(cond)
CS_CONDITION	*cond;
{
	return cond->cnd_name;
}

/*{
** Name: CSstatistics	- Fetch statistics of current thread or entire server
**
** Description:
**      This routine is called to return the ``at this moment'' statistics
**      for this particular thread or for the entire server.  It returns a
**	structure which holds the statistics count at the time the call is made.
**
**	If the 'server_stats' parameter is zero, then statistics are returned
**	for the current thread.  If non-zero, then CSstatistics returns
**	the statistics gathered for the entire server.
**
**	The structure returned is a TIMERSTAT block, as defined in the <tm.h>
**      file in the CLF\TM component.   Thus, this calls looks very much 
**      like a TMend() call.  There are two main differences.  The first is
**	that there is no equivalent to the TMstart() call.  This is done
**	implicitly at server and thread startup time.  Secondly, this routine
**	returns a TIMERSTAT block holding only the current statistics
**	count (TMend returns a TIMER block which holds three TIMERSTAT blocks
**	- one holding the starting statistics count (set at TMstart), one
**	holding the statistics count at TMend time, and one holding the
**	difference).
**
**	NOTE:  CPU statistics are not kept by default on a thread by thread
**	basis.  If CSstatistics is called for a thread not doing CPU statistics
**	then the stat_cpu field will be zero.  A caller wishing to collect CPU
**	times for a thread should first call CSaltr_session to start CPU
**	statistics collecting.  Subsequent CSstatistics calls will return the
**	CPU time used since that time.
**
** Inputs:
**      timer_block                     The address of the timer block to fill
**	server_stats			Zero - return statistics for current
**					    session.
**					Non-zero - return statistics for entire
**					    server.
**
** Outputs:
**      *timer_block                    Filled in with...
**          .stat_cpu                   the CPU time in milliseconds
**          .stat_dio                   count of disk i/o requests
**          .stat_bio                   count of communications requests
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	Invalid timer_block pointer passed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Feb-1987 (fred)
**          Created.
**	21-oct-1988 (rogerk)
**	    Added server_stats parameter.
**	17-jul-1995 (dougb)
**	    Add call to CSswitch() for additional CLF time slicing.
*/
STATUS
IICSstatistics(
TIMERSTAT       *timer_block,
i4		server_stats)
{
    i4		    new_cpu;
    i4		    i;
    TIMER	    now;
    IOSB	    iosb;
    ILE3	    jpiget[] = {
				    {sizeof(i4), JPI$_CPUTIM, 0, 0},
				    { 0, 0, 0, 0}
				};
    CS_SCB	    *scb;

    if (timer_block == 0)
	return(E_CS0004_BAD_PARAMETER);

    /*
    ** This is how we time slice OPF - OPF does CSstatistics while
    ** evaluating plans to see if it has a plan that will run quicker
    ** than it has spent finding other plans.
    */
    CSswitch();

    TMend(&now);
    jpiget[0].ile3$ps_bufaddr = &new_cpu;
    i = sys$getjpiw(EFN$C_ENF, 0, 0, jpiget, &iosb, 0, 0);
    if (i & 1)
	i = iosb.iosb$w_status;
    if (( i & 1) == 0)
    {
	(*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &i, 0);
	return(E_CS0016_SYSTEM_ERROR);
    }

    scb = Cs_srv_block.cs_current;

    if (server_stats)
    {
	/* Return statistics for entire server. */

	timer_block->stat_pgflts = now.timer_end.stat_pgflts;
	timer_block->stat_dio =
			Cs_srv_block.cs_wtstatistics.cs_dior_done + 
			Cs_srv_block.cs_wtstatistics.cs_diow_done;
	timer_block->stat_bio =
			Cs_srv_block.cs_wtstatistics.cs_bior_done + 
			Cs_srv_block.cs_wtstatistics.cs_biow_done;

	/* multiply by 10 since cpu time is returned in 10ms ticks */
	timer_block->stat_cpu = 10 * new_cpu;
    }
    else
    {
	/* Return statistics for this session only. */

	timer_block->stat_pgflts = now.timer_end.stat_pgflts;
	timer_block->stat_dio = scb->cs_dior + scb->cs_diow;
	timer_block->stat_bio = scb->cs_bior + scb->cs_biow;

	/* multiply by 10 since cpu time is returned in 10ms ticks */
	timer_block->stat_cpu = 10 * (Cs_srv_block.cs_current->cs_cputime
				+ (new_cpu - Cs_srv_block.cs_cpu));
    }

    return(OK);
}

/*{
** Name: CSattn	- Indicate async event to CS thread or server
**
** Description:
**      This routine is called to indicate that an unexpected asynchronous  
**      event has occurred, and that the CS module should take the appropriate 
**      action.  This is typically used in indicate that a user interrupt has 
**      occurred, but may be used for other purposes.
**
**	This routine expects to be called in such a way so that it may not
**	be interrupted.  This is because it manipulates the server session
**	lists and if switched out may leave them in an inconsistent state.
**
** Inputs:
**	eid				Event identifier.  This identifies the
**					event type for the server.
**      sid                             thread id for which the event occurred.
**                                      In some cases, the event may be for
**                                      the server, in which case this parameter
**                                      should be zero (0).
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
**      24-feb-1987 (fred)
**          Created on Jupiter.
**      19-Nov-1987 (fred)
**          Added CS_TERMINATE check to prevent interrupts during session
**          termination.
**      30-Nov-1987 (fred)
**          Added eid parameter.
**	10-Dec-1990 (neil)
**	    Modified to turn off AST's if they are currently on.  This allows
**	    the routine to be called from another thread.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
**	24-jan-96 (dougb) bug 71590
**	    Update handler() prototype.
**	14-apr-2005 (jenjo02/abbjo03)
**	    Do not require CS_SHUTDOWN_EVENT when testing for CS_CNDWAIT.
**	11-Sep-2006 (jonj)
**	    Some events may not want the session interrupted,
**	    force-abort with LOGFULL_COMMIT, for instance.
*/
void
IICSattn(
i4      eid,	    
CS_SID  sid)
{
    CS_SCB              *scb = 0;
    static int		handler( struct chf$signal_array *sig,
				struct chf$mech_array *mech );
    STATUS		status;
    i4			asts_enabled = 0;

    /*
    ** If we are not running in AST mode, we have to make sure that AST's aren't
    ** enabled, so turn them off right now (and turn them back on later).
    */
    if (lib$ast_in_prog() == 0)
    {
	status = sys$setast(0);
	asts_enabled = (status == SS$_WASSET);
    }

    if (sid != (CS_SID)0)
	scb = CS_find_scb(sid);
    if (scb && (scb->cs_mode != CS_TERMINATE))
    {
	/*
	** If it's a force-abort, check if it will be
	** handled by DMF and should be ignored here.
	*/
	if ( eid == CS_RCVR_EVENT && (*Cs_srv_block.cs_attn)(eid, scb) )
	    return;
	
	if ((scb->cs_state != CS_COMPUTABLE) &&
		(scb->cs_mask & CS_INTERRUPT_MASK))
	{
	    /*
	    ** This thread should be resumed.
	    ** If the Server is not running inkernel, then we can move
	    ** the scb off the wait list and put it into the ready queue.
	    ** If the server is inkernel, then we can't muck with any of
	    ** the control block lists, so we put the scb on the async
	    ** ready list.  This list is touched only by atomic routines
	    ** which are protected against thread switching and AST driven
	    ** routines.
	    **
	    ** When the session which is running inkernel returns from
	    ** inkernel mode, it will move the scb onto the ready queue.
	    */
	    if (Cs_srv_block.cs_inkernel == 0)
	    {
		scb->cs_mask &= ~CS_INTERRUPT_MASK;
		scb->cs_mask |= CS_IRCV_MASK;

		scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		    scb->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		    scb->cs_rw_q.cs_q_next;

		/* now link into the ready queue */

		scb->cs_rw_q.cs_q_next =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority];
		scb->cs_rw_q.cs_q_prev =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
		scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
		Cs_srv_block.cs_ready_mask |=
		    (CS_PRIORITY_BIT >> scb->cs_priority);
                if (scb->cs_thread_type != CS_INTRNL_THREAD &&
                  (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                    Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
#ifdef	CS_STACK_SHARING
		if (scb->cs_stk_area)
#endif
		{
		    scb->cs_state = CS_COMPUTABLE;

		    if (Cs_srv_block.cs_state == CS_IDLING)
			sys$wake(0, 0);
		}
#ifdef CS_STACK_SHARING
		else
		{
		    scb->cs_state = CS_STACK_WAIT;
		}
#endif
	    }
	    else
	    {
		/*
		** Put this session on the async ready queue.
		** It will be moved onto the server ready queue by the
		** session that is running inkernel.
		*/
		if (scb->cs_as_q.cs_q_next == 0)
		{
		    scb->cs_as_q.cs_q_next =
			Cs_srv_block.cs_as_list->cs_as_q.cs_q_next;
		    scb->cs_as_q.cs_q_prev = Cs_srv_block.cs_as_list;
		    scb->cs_as_q.cs_q_prev->cs_as_q.cs_q_next = scb;
		    scb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev = scb;
		}
		Cs_srv_block.cs_async = 1;

		if (scb->cs_inkernel == 0)
		{
		    scb->cs_mask &= ~CS_INTERRUPT_MASK;
		    scb->cs_mask |= CS_IRCV_MASK;
		}
		else
		{
		    if (scb->cs_async == 0)
		    {
			scb->cs_async = 1;
			Cs_srv_block.cs_scb_async_cnt++;
		    }
		    scb->cs_asmask |= CS_IRCV_MASK;
		    scb->cs_asunmask &= ~CS_IRCV_MASK;
		    scb->cs_asunmask |= CS_INTERRUPT_MASK;
		    scb->cs_asmask &= ~CS_INTERRUPT_MASK;

		}
	    }
	}
        else if (scb->cs_state == CS_CNDWAIT)
        {
            CS_CONDITION        *cp;

            /* remove from condition queue */
            cp = scb->cs_cnd;
            cp->cnd_next->cnd_prev = cp->cnd_prev;
            cp->cnd_prev->cnd_next = cp->cnd_next;
            scb->cs_cnd = NULL;

            scb->cs_state = CS_COMPUTABLE;
            scb->cs_mask |= CS_ABORTED_MASK;

            /*
            ** Link this scb last in the ready queue:
            */
            scb->cs_rw_q.cs_q_next =
                Cs_srv_block.cs_rdy_que[scb->cs_priority];
            scb->cs_rw_q.cs_q_prev =
                Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
            scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
            Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev =
                scb;
            Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);
            if ( scb->cs_thread_type != CS_INTRNL_THREAD &&
                 (++Cs_srv_block.cs_num_active > Cs_srv_block.cs_hwm_active))
                Cs_srv_block.cs_hwm_active = Cs_srv_block.cs_num_active;
        }
	else /* DMF FORCE ABORT ISSUE if (scb->cs_state == CS_COMPUTABLE) */
	{
	    /*
	    ** Mark session has interrupt pending.
	    ** If the session control block is busy, then store the bit in
	    ** the async portion of the SCB.  The interrupt pending bit will
	    ** be set in the cs_mask field when the cs_inkernel flag is
	    ** turned off.
	    */
	    if (scb->cs_inkernel == 0)
	    {
		scb->cs_mask |= CS_IRPENDING_MASK;
	    }
	    else
	    {
		if (scb->cs_async == 0)
		{
		    scb->cs_async = 1;
		    Cs_srv_block.cs_scb_async_cnt++;
		}
		scb->cs_asmask |= CS_IRPENDING_MASK;
		scb->cs_asunmask &= ~CS_IRPENDING_MASK;
		Cs_srv_block.cs_async = 1;
	    }
	}
	lib$establish(handler);

	/* CS_RCVR_EVENT we've already checked */
	if ( eid != CS_RCVR_EVENT && (*Cs_srv_block.cs_attn)(eid, scb) )
	{
	    /* Unset the interrupt pending mask. */

	    if (scb->cs_inkernel == 0)
	    {
		scb->cs_mask &= ~CS_IRPENDING_MASK;
	    }
	    else
	    {
		if (scb->cs_async == 0)
		{
		    scb->cs_async = 1;
		    Cs_srv_block.cs_scb_async_cnt++;
		}
		scb->cs_asmask &= ~CS_IRPENDING_MASK;
		scb->cs_asunmask |= CS_IRPENDING_MASK;
		Cs_srv_block.cs_async = 1;
	    }
	}
    }
	/* else thread is being deleted */

    if (asts_enabled)
	sys$setast(1);

    return;
}

/*
** handler - local VMS handler for exceptions in cs_attn routine.
**
** Description:
**	Local VMS handler for exceptions which occur during a call to
**	Cs_srv_block.cs_attn.
**
** Inputs:
**	sig	- VMS signal array.
**	mech	- VMS mechanism structure.
**
** Outputs:
**	None.
**
** Returns:
**	SS$_CONTINUE	- Ignored by VMS caller.
**
** Side Effects:
**	Logs the escaped exception.  Unwinds the stack.
**
** History:
**	24-jan-96 (dougb) bug 71590
**	    Don't interpret VMS mechanism vector as an array of longwords.
**	    Get structure information from the chfdef.h system header file.
**	    Don't try to $UNWIND during an unwind.  Add history comments.
**	    Use ANSI-style fn definition.  Make routine static.
*/
static int
handler( struct chf$signal_array *sig, struct chf$mech_array *mech )
{
    unsigned int tst_cond = SS$_UNWIND;

    if ( !lib$match_cond( sig->chf$l_sig_name, &tst_cond ))
    {
	unsigned int depth;

	(*Cs_srv_block.cs_elog)( E_CS0014_ESCAPED_EXCPTN, 0, 0 );

	/*
	** I suspect this will unwind to the offending call or instruction
	** in the Cs_srv_block.cs_attn routine.  That routine should be
	** one below the establisher (CSattn(), above).  According to the
	** Calling Standard, "To unwind to the establisher, the depth from
	** the call to the handler should be specified."
	*/
	depth = mech->chf$q_mch_depth - 1;
	sys$unwind( &depth, 0 );
    }

    return SS$_CONTINUE;	/* Ignored. */
}

/*{
** Name: CSget_sid	- Get the current thread id
**
** Description:
**	This routine returns the current thread id.
**
** Inputs:
**
** Outputs:
**      *sidptr                             thread id.
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-feb-1986 (fred)
**          Created.
*/
void
IICSget_sid(
CS_SID  *sidptr)
{
    CS_SID	    sid = (CS_SID)0;
    CS_SCB	    *scb = Cs_srv_block.cs_current;

    if (scb)
    {
	if (scb == &Cs_idle_scb || scb == &Cs_admin_scb)
	    sid = CS_ADDER_ID;
	else
	    sid = scb->cs_self;
    }
    if (sidptr != (CS_SID *)0)
	*sidptr = sid;
}

/*{
** Name: CSget_cpid	- Get the current thread's cross-process id.
**
** Description:
**	This routine returns the current thread's
**	cross-process identity.
**
** Inputs:
**
** Outputs:
**	*cpid				    CP thread id.
**
** Returns:
**      void
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	16-Oct-1998 (jenjo02)
**	    created.
*/
void
IICSget_cpid(CS_CPID *cpid)
{
    if (cpid)
    {
	CS_SCB	    *scb;
	if (scb = Cs_srv_block.cs_current)
	{
	    if (scb == &Cs_idle_scb || scb == (CS_SCB *)&Cs_admin_scb)
		cpid->sid = CS_ADDER_ID;
	    else
		cpid->sid = scb->cs_self;
	    cpid->pid = Cs_srv_block.cs_pid;
	}
	else
	{
	    cpid->sid = 0;
	    cpid->pid = 0;
	}
    }
}

/*{
** Name: CSget_scb  - Get the current scb
**
** Description:
**	This routine returns a pointer to the current scb.
**
** Inputs:
**
** Outputs:
**      *scbptr                          thread control block pointer.
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-feb-1986 (fred)
**          Created.
**      24-jun-93 (ed)
**          fixed compiler warning
*/
void
IICSget_scb(
    CS_SCB		**scbptr)
{
    if (scbptr)
    {
	if ((Cs_srv_block.cs_state == CS_CLOSING)
		|| (Cs_srv_block.cs_state == CS_TERMINATING))
	{
	    *scbptr = (CS_SCB *)0;
	}
	else
	{
	    *scbptr = (CS_SCB *)Cs_srv_block.cs_current;
	}
    }
}

/*{
** Name: CSget_svrid  - Get the current server name
**
** Description:
**	This routine returns a pointer to the current server name.
**
** Inputs:
**
** Outputs:
**	*csname		
**	Returns:
**	    void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-sep-1999 (hanch04)
**	    Created.
*/
void
CSget_svrid(char *cs_name)
{
    STcopy( Cs_srv_block.cs_name, cs_name);
    return;
}


/*{
** Name: CSfind_scb	- Given a session thread id, return the session CB.
**
** Description:
**	This routine is called by one thread to find the session cb (CS_SCB)
**	of another thread.
**
**	Since most CS implementations use "hardware hashing" of thread
**	ids (the thread id is the address of CS_SCB)  this routine will
**	just return it's argument.  It is assumed that the caller has
**	permission to access any other thread in the server.
**	
**	Once the CB is returned to the caller the caller should make sure
**	they're not fiddling with a session that's about to evaporate the
**	data structures that are being accessed.
**
** 	If the session ID does not correspond to an active thread then NULL
**	is returned.  It is up to the caller to verify that a non-NULL
**	session CB was returned.  It is not an internal CL error to pass in
**	an invalid session ID, but caller will be notified through a NULL
**	session CB.
**
** Inputs:
**	sid				Known thread id for which we want
**					the corresponding session SCB.
** Outputs:
**	Returns:
**	    CS_SCB 			Pointer to session CB or NULL if an
**					active session CB was not found.
**	Exceptions:
**	    None
**
** Side Effects:
**	None
**
** History:
**	10-dec-90 (neil)
**	    Written to support event thread alert notification.
*/
CS_SCB	*
IICSfind_scb(
CS_SID  sid)
{
    CS_SCB	*scb;

    scb = (CS_SCB *)sid;
    if (   scb == (CS_SCB *)0
	|| scb->cs_type != (i2)CS_SCB_TYPE
	|| (scb->cs_mask & (CS_DEAD_MASK|CS_FATAL_MASK))
       )
    {
	scb = (CS_SCB *)0;
    }
    return (scb);
} /* CSfind_scb */

/******************************************************************************
**
** Name: CSfind_sid    - Return the SID of the given SCB.
**
** Description:
**      A trivial routine, but it keeps other components out of the CS_SCB.
**
** Inputs:
**      scb - the SCB pointer
**
** Outputs:
**
** Returns:
**      The session id (thread ID)
**
** Exceptions:
**
** Side Effects:
**      none
**
** History:
**	21-dec-95 (dougb)
**	    Correct typo in wonst02's change of 24-aug-95, which created this
**	    routine.  Fill in rest of header comments.
*/
CS_SID
IICSfind_sid(
CS_SCB  *scb)
{
    return (scb->cs_self);
}

/*{
** Name: CSset_sid	- Set the current thread id (DEBUG ONLY)
**
** Description:
**      This routine is provided for module tests only.  It sets the current 
**      thread to that provided.
**
** Inputs:
**      scb                             thread id.
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
**      26-feb-1986 (fred)
**          Created.
*/
void
IICSset_sid(
CS_SCB  *scb)
{

    Cs_srv_block.cs_current = scb;
}


/*{
** Name: CSintr_ack	- Acknowledge receipt of an interrupt
**
** Description:
**      This routine turns off the CS_IRPENDING_MASK bit in the thread's 
**      state mask.  This bit is set if a thread is informed of an interrupt 
**      when it is running.  The next interruptable request will terminate 
**      with an interrupt after this happens, so this routine allows a thread 
**      to explicitly clear the flag when it knows about the interrupt.
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
**      16-Sep-1987 (fred)
**          Created.
*/
void
IICSintr_ack()
{
    /*
    ** Set inkernel while changing the scb's cs_mask as this field
    ** is also set by AST driven routines.
    */
    Cs_srv_block.cs_inkernel = 1;
    Cs_srv_block.cs_current->cs_inkernel = 1;

    Cs_srv_block.cs_current->cs_mask &= ~(CS_IRPENDING_MASK);

    Cs_srv_block.cs_current->cs_inkernel = 0;
    Cs_srv_block.cs_inkernel = 0;
    if (Cs_srv_block.cs_async)
	CS_move_async(Cs_srv_block.cs_current);
}

/*{
** Name: CSaltr_session - Alter CS characteristics of session.
**
** Description:
**	This routine is called to alter some characteristic of a
**	particular server thread.
**
**	If 'session_id' is specified (non-zero) then that indicates the
**	session to alter.  If 'session_id' is zero, then the current session
**	(the one from which the call is made) is altered.
**
**	Session Alter Options include:
**
**	    CS_AS_CPUSTATS - Turn CPU statistics collecting ON/OFF for the
**		    specified session.
**
**		    Item points to a value of type 'nat'.
**
**		    If the value pointed at by the parameter 'item' 
**		    (*(nat *)item) is zero, CS will no longer collect CPU
**		    statistics for the specified session, and any calls to
**		    CSstatistics by that session will return a cpu time of zero.
**		    If the value pointed at by the parameter 'item' is non-zero,
**		    then CS will begin (continue) to keep track of cpu time
**		    accumulated by that session.
**
**		    If the CS_AS_CPUSTATS request is to turn on CPU collecting
**		    and this option is already enabled for this thread, then
**		    the return value E_CS001F_CPUSTATS_WASSET is returned.
**
**	    CS_AS_PRIORITY - Set priority to indicated value.
**
**		    Item points to a value of type 'nat'.
**
**		    CS will set the session priority to the indicated
**		    value, if possible
**
** Inputs:
**	session_id  - session to alter (zero indicates current session)
**	option	    - alter option - one of following:
**			CS_AS_CPUSTATS - turn ON/OFF collecting of CPU stats.
**	item	    - pointer to value used to alter session characteristic.
**		      Meaning is specific to each alter option.
**
** Outputs:
**      None
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	Invalid parameter passed.
**	    E_CS001F_CPUSTATS_WASSET	Cpu statistics already being collected.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-oct-1988 (rogerk)
**          Created.
**	18-oct-1993 (walt)
**	    Get an event flag number from lib$get_ef rather than use event
**	    flag zero in the sys$getjpiw call.  
**	02-feb-1995 (wolf)
**	    Added CS_AS_PRIORITY, from RobF's Secure 2.0/ES 1.1 chg 414229.
*/
STATUS
IICSaltr_session(
CS_SID		session_id,
i4		option,
PTR		item)
{
    CS_SCB	    *scb = 0;
    i4	    new_cpu;
    i4		    i;
    ILE3	   jpiget[] = {
				    {sizeof(i4), JPI$_CPUTIM, 0, 0},
				    { 0, 0, 0, 0}
				};

    if (session_id)
	scb = CS_find_scb(session_id);
    else
	scb = Cs_srv_block.cs_current;

    if (scb == 0)
	return (E_CS0004_BAD_PARAMETER);

    switch (option)
    {
      case CS_AS_CPUSTATS:
	if (item && (*(i4 *)item))
	{
	    /*
	    ** If turning on cpu stats for this session for the first time,
	    ** update server cpu count so that this thread will get charged
	    ** for the correct cpu time the first time it is swapped out.
	    */
	    if ((scb->cs_mask & CS_CPU_MASK) == 0)
	    {
		IOSB iosb;

		jpiget[0].ile3$ps_bufaddr = &new_cpu;
		i = sys$getjpiw(EFN$C_ENF, 0, 0, jpiget, &iosb, 0, 0);
		if (i & 1)
		    i = iosb.iosb$w_status;

		/*
		** Set inkernel while changing the scb's cs_mask as this field
		** is also set by AST driven routines.
		*/
		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;

		if (( i & 1) == 0)
		    (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, &i, 0);
		else
		    Cs_srv_block.cs_cpu = new_cpu;
		scb->cs_mask |= CS_CPU_MASK;

		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
	    }
	    else
	    {
		return (E_CS001F_CPUSTATS_WASSET);
	    }
	}
	else
	{
	    /*
	    ** Set inkernel while changing the scb's cs_mask as this field
	    ** is also set by AST driven routines.
	    */
	    Cs_srv_block.cs_inkernel = 1;
	    scb->cs_inkernel = 1;

	    scb->cs_mask &= ~(CS_CPU_MASK);
	    scb->cs_cputime = 0;

	    scb->cs_inkernel = 0;
	    Cs_srv_block.cs_inkernel = 0;
	    if (Cs_srv_block.cs_async)
		CS_move_async(scb);
	}
	break;

      case CS_AS_PRIORITY:
        if (item && (*(i4 *)item))
        {
            i4 priority= *(i4*)item;
            /*
            ** Setting session priority.
            */
            if (priority >= CS_LIM_PRIORITY)
            {
                    return(E_CS0004_BAD_PARAMETER);
            }
            else if (priority == 0)
            {
                priority = CS_DEFPRIORITY;
            }
           /* Set  in kernel mode */
            Cs_srv_block.cs_inkernel = 1;
            scb->cs_inkernel = 1;

            /* Change priority */
            scb->cs_priority = priority;

            scb->cs_inkernel = 0;
            Cs_srv_block.cs_inkernel = 0;
            if (Cs_srv_block.cs_async)
                    CS_move_async(scb);
        }
        else
                return (E_CS0004_BAD_PARAMETER);
        break;

      default:
	return (E_CS0004_BAD_PARAMETER);
    }

    return (OK);
}
 
/*{
** Name: CSdump_statistics  - Dump CS statistics to Log File.
**
** Description:
**	This call is used to dump the statistics collected by the CS system.
**	It is intended to be used for performance monitoring and tuning.
**
**	Since there are no external requirements for what server statistics
**	are gathered by CS, the amount of information dumped by this routine
**	is totally dependent on what particular statistics are kept by this
**	version of CS.  Therefore, no mainline code should make any
**	assumptions about the information (if any) dumped by this routine.
**
**      This function will format text into a memory buffer and then use a
**	caller provided function to dispose of the buffer contents.  The
**	caller can pass in "TRdisplay" as the print function in order to
**	set the output to the server trace file.
**
**	The print function is called like:
**
**	    (*fcn)(buffer)
**
**	where buffer will be a null-terminated character string.
**
** Inputs:
**	print_fcn	- function to call to print the statistics information.
**
** Outputs:
**      None
**	Returns:
**	    Status
**	Exceptions:
**	    none
**
** Side Effects:
**	The print function is called many times to output character buffers.
**	Whatever side effects the caller-supplied print_fcn has.
**
** History:
**      20-Sep-1988 (rogerk)
**          Created.
**	31-oct-1088 (rogerk)
**	    Added print_fcn argument.
**	28-feb-1989 (rogerk)
**	    Added dumping of cross-process semaphore statistics.
**	09-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (bryanp)
**		Added support for CS_GWFIO_MASK used to indicate Gateway wait.
**	20-feb-1998 (kinte01)
**	   Cross-integrate Unix changes
**         27-Apr-1995 (jenjo02)
**            Added total server cpu time to CSdump_statistics().
**         24-oct-1995 (thaju02/stoli02)
**            Added EVCBs wait count to CSdump_statistics().
**         7-jan-98 (stephenb)
**            Add active user high water mark to dump statistics.
*/
void
IICSdump_statistics(
i4	    (*print_fcn)(char *, ...))
{
    char	buffer[150];

    STprintf(buffer, "========================================================\n");
    (*print_fcn)(buffer, "========================================================\n");
    STprintf(buffer, "SERVER STATISTICS:\n====== ==========\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Server process CPU time: %d. ms\n",
            Cs_srv_block.cs_cpu * 10);
    (*print_fcn)(buffer);
    STprintf(buffer, "Idle job CPU time: %d. ms\n",
	    Cs_idle_scb.cs_cputime * 10);
    (*print_fcn)(buffer);
    STprintf(buffer, "Active users HighWater mark: %d\n",
            Cs_srv_block.cs_hwm_active);
    (*print_fcn)(buffer);
    STprintf(buffer, "Semaphore Statistics:\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "--------- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "    Exclusive requests:   %8d. Shared: %8d.  Multi-process: %8d.\n",
		Cs_srv_block.cs_smstatistics.cs_smx_count,
		Cs_srv_block.cs_smstatistics.cs_sms_count,
		Cs_srv_block.cs_smstatistics.cs_smc_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Exclusive collisions: %8d. Shared: %8d.  Multi-process: %8d.\n",
		Cs_srv_block.cs_smstatistics.cs_smxx_count,
		Cs_srv_block.cs_smstatistics.cs_smsx_count,
		Cs_srv_block.cs_smstatistics.cs_smcx_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Multi-Process Wait loops: %8d.\r\n",
		Cs_srv_block.cs_smstatistics.cs_smcl_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "--------- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Wait Statistics: (Waits expressed in %d%s of seconds)\n",
				(Cs_srv_block.cs_q_per_sec >= 0 ?
				    Cs_srv_block.cs_q_per_sec :
					-Cs_srv_block.cs_q_per_sec),
				(Cs_srv_block.cs_q_per_sec >= 0 ?
				    "-ths" : "s")
				);
    (*print_fcn)(buffer);
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "          Requests  Wait State  Avg. Wait  Zero Wait  Wait Time\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "          --------  ----------  ---------  ---------  ---------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "    BIOR: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_bior_done,
		    Cs_srv_block.cs_wtstatistics.cs_bior_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_bior_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_bior_time / 
			    Cs_srv_block.cs_wtstatistics.cs_bior_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_bior_done - 
			Cs_srv_block.cs_wtstatistics.cs_bior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_bior_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    BIOW: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_biow_done,
		    Cs_srv_block.cs_wtstatistics.cs_biow_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_biow_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_biow_time / 
			    Cs_srv_block.cs_wtstatistics.cs_biow_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_biow_done - 
			Cs_srv_block.cs_wtstatistics.cs_biow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_biow_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    DIOR: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_dior_done,
		    Cs_srv_block.cs_wtstatistics.cs_dior_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_dior_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_dior_time / 
			    Cs_srv_block.cs_wtstatistics.cs_dior_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_dior_done - 
			Cs_srv_block.cs_wtstatistics.cs_dior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_dior_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    DIOW: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_diow_done,
		    Cs_srv_block.cs_wtstatistics.cs_diow_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_diow_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_diow_time / 
			    Cs_srv_block.cs_wtstatistics.cs_diow_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_diow_done - 
			Cs_srv_block.cs_wtstatistics.cs_diow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_diow_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LIOR: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lior_done,
		    Cs_srv_block.cs_wtstatistics.cs_lior_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_lior_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_lior_time / 
			    Cs_srv_block.cs_wtstatistics.cs_lior_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_lior_done - 
			Cs_srv_block.cs_wtstatistics.cs_lior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lior_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LIOW: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_liow_done,
		    Cs_srv_block.cs_wtstatistics.cs_liow_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_liow_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_liow_time / 
			    Cs_srv_block.cs_wtstatistics.cs_liow_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_liow_done - 
			Cs_srv_block.cs_wtstatistics.cs_liow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_liow_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Log: %8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lg_done,
		    Cs_srv_block.cs_wtstatistics.cs_lg_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_lg_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_lg_time / 
			    Cs_srv_block.cs_wtstatistics.cs_lg_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_lg_done - 
			Cs_srv_block.cs_wtstatistics.cs_lg_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lg_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LGevent: %8d.   %8d.  %8d.   %8d.   %8d.\n",
                    Cs_srv_block.cs_wtstatistics.cs_lge_done,
                    Cs_srv_block.cs_wtstatistics.cs_lge_waits,
                    (Cs_srv_block.cs_wtstatistics.cs_lge_waits ?
                        (Cs_srv_block.cs_wtstatistics.cs_lge_time /
                            Cs_srv_block.cs_wtstatistics.cs_lge_waits) : 0),
                    Cs_srv_block.cs_wtstatistics.cs_lge_done -
                        Cs_srv_block.cs_wtstatistics.cs_lge_waits,
                    Cs_srv_block.cs_wtstatistics.cs_lge_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Lock:%8d.   %8d.  %8d.  %8d.  %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lk_done,
		    Cs_srv_block.cs_wtstatistics.cs_lk_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_lk_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_lk_time / 
			    Cs_srv_block.cs_wtstatistics.cs_lk_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_lk_done - 
			Cs_srv_block.cs_wtstatistics.cs_lk_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lk_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LKevent: %8d.   %8d.  %8d.   %8d.   %8d.\n",
                    Cs_srv_block.cs_wtstatistics.cs_lke_done,
                    Cs_srv_block.cs_wtstatistics.cs_lke_waits,
                    (Cs_srv_block.cs_wtstatistics.cs_lke_waits ?
                        (Cs_srv_block.cs_wtstatistics.cs_lke_time /
                            Cs_srv_block.cs_wtstatistics.cs_lke_waits) : 0),
                    Cs_srv_block.cs_wtstatistics.cs_lke_done -
                        Cs_srv_block.cs_wtstatistics.cs_lke_waits,
                    Cs_srv_block.cs_wtstatistics.cs_lke_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Time:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
                    Cs_srv_block.cs_wtstatistics.cs_tm_done,
                    Cs_srv_block.cs_wtstatistics.cs_tm_waits,
                    (Cs_srv_block.cs_wtstatistics.cs_tm_waits ?
                        (Cs_srv_block.cs_wtstatistics.cs_tm_time /
                            Cs_srv_block.cs_wtstatistics.cs_tm_waits) : 0),
                    Cs_srv_block.cs_wtstatistics.cs_tm_done -
                        Cs_srv_block.cs_wtstatistics.cs_tm_waits,
                    Cs_srv_block.cs_wtstatistics.cs_tm_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    EVCB:    %8d.   %8d.  %8d.\n",
                    Cs_srv_block.cs_wtstatistics.cs_event_count,
                    Cs_srv_block.cs_wtstatistics.cs_event_wait,
                    (Cs_srv_block.cs_wtstatistics.cs_event_count ?
                        (Cs_srv_block.cs_wtstatistics.cs_event_wait /
                            Cs_srv_block.cs_wtstatistics.cs_event_count) : 0));
    (*print_fcn)(buffer);
    STprintf(buffer, "  GWFIO: %8d.   %8d.  %8d.  %8d.  %8d.\n",
                    Cs_srv_block.cs_wtstatistics.cs_gwfio_done,
                    Cs_srv_block.cs_wtstatistics.cs_gwfio_waits,
                    (Cs_srv_block.cs_wtstatistics.cs_gwfio_waits ?
                        (Cs_srv_block.cs_wtstatistics.cs_gwfio_time /
                            Cs_srv_block.cs_wtstatistics.cs_gwfio_waits) : 0),
                    Cs_srv_block.cs_wtstatistics.cs_gwfio_done -
                        Cs_srv_block.cs_wtstatistics.cs_gwfio_waits,
                    Cs_srv_block.cs_wtstatistics.cs_gwfio_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Idle Statistics:\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "    Idle quanta: %d./%d. (%d. %%)\n",
			Cs_srv_block.cs_idle_time,
			Cs_srv_block.cs_quantums,
			(Cs_srv_block.cs_quantums ?
			    ((Cs_srv_block.cs_idle_time * 100) /
				Cs_srv_block.cs_quantums) : 100));
    (*print_fcn)(buffer);
    STprintf(buffer,
"    BIO: %8d., DIO: %8d., Log: %8d., Lock: %8d., GWF: %8d., Semaphore: %8d.\n",
			Cs_srv_block.cs_wtstatistics.cs_bio_idle,
			Cs_srv_block.cs_wtstatistics.cs_dio_idle,
			Cs_srv_block.cs_wtstatistics.cs_lg_idle,
			Cs_srv_block.cs_wtstatistics.cs_lk_idle,
                        Cs_srv_block.cs_wtstatistics.cs_gwfio_idle,
			Cs_srv_block.cs_wtstatistics.cs_tm_idle);
    (*print_fcn)(buffer);
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "========================================================\n");
    (*print_fcn)(buffer);
}

/*{
** Name: CSACCEPT_CONNECT - Return TRUE if the server can handle another user
**
** Description:
**      This routine returns TRUE if the server can accept another connection
**	request. (thread). This routine was 
**	invented to be passed into GCA to solve
**	the GCA rollover problem. The GCA rollover problem is where GCA needs
**      to determine whether a user can use a dbms server before the GCA_LISTEN 
**      completes. If the user can't use a DBMS, then GCA rolls over to the 
**      next possible DBMS on it's list. If the user can use the DBMS server 
**      then, obviously, the listen completes. By necessity, this routine
**	is a duplicate of the similiar check in CS_admin_task().
**       
**      CAUTION: If you change the input (in this case, the lack of input) or 
**      the output (like, the fact that it returns TRUE or FALSE), then 
**      you must coordinate the change with what GCA expects as input and 
**      output. The address of this routine is passed into GCF from SCF. 
**      This is just a long-winded, detailed, way of saying that this routine 
**      is a part of the CL spec, and interface changes need to be approved. 
**        
**      This routine first sets a semaphore in the admin tasks scb, to insure 
**      that we are looking at a consistent set of data. We then check if 
**      the server is at the limit for the number of threads or if the server 
**      is shutting down. If one of these is true, then tell the caller 
**      that we cannot accept any more connections, otherwise returne that 
**      we are still accepting connections. 
**
** Inputs:
**	None.
**
** Outputs:
**	none.
**
**	Returns:
**	    TRUE or FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-89 (eric)
**          written
*/
i4
CSaccept_connect()
{
    i4	    ret;

    if (((Cs_srv_block.cs_num_sessions + 1) >
		Cs_srv_block.cs_max_sessions)   ||
	(Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK))
    {
	ret = FALSE;
    }
    else
    {
	ret = TRUE;
    }
    return(ret);
}
/*{
** Name: CSswitch - Poll for possible session switch
**
** Description:
**      This call will determine if the inquiring session has
**      used more than an allotted quantum of CPU and yield
**      the CPU if so.
**
**      How this quantum measurement is managed, updated and
**      checked is system specific.  However, it is essential that
**      this call imposes "negligible" overhead.  This function
**      may be called very freuently and should not do much more than
**      examine a few memory locaations before returning for the case that
**      a switch does not occur.
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
**      Retuns:
**          none
**
**      Exceptions:
**          none
**
** Side Effects:
**      Thread (session) switch will occur when the "quantum" value
**      has been exceeded.
**
** History:
**     11-Dec-89 (anton)
**          Created.
**     29-mar-94 (walt)
**	    Activate for Alpha/VMS.  If the current thread has been in more
**	    than a quantum, switch to another thread.
**	13-jul-95 (dougb)
**	    Change name of quantum_ticks to CS_lastquant -- matching Unix CL.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**	13-Feb-2007 (kschendel)
**	    Added CScancelCheck, maps to CSswitch for internal threads.
**      20-jun-2008 (horda03) Bug 120474
**          For all non-internal threads, call CS_ssprsm each time.
**      23-Jul-2008 (horda03) Bug 120474
**          Only call CS_ssprsm on threaded servers.
*/
void
IICSswitch()
{
    if ( (Cs_srv_block.cs_state != CS_INITIALIZING) &&
           ((Cs_srv_block.cs_current->cs_thread_type != CS_INTRNL_THREAD) ||
	    (Cs_lastquant > ALLOWED_QUANTUM_COUNT) ) )
    {
#ifdef KPS_THREADS
        CS_SCB* scb = Cs_srv_block.cs_current;
        if (scb)
            exe$kp_stall_general(scb->kpb);
#else
	CS_ssprsm( FALSE );
#endif
    }
}

void
IICScancelCheck(CS_SID sid)
{
    IICSswitch();
}

/*{
** Name: CS_swuser      - Do a rescheduling context switch
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
**      Returns:
**          never
**      Exceptions:
**
**
** Side Effects:
**          The current thread is preempted by another thread.
**
** History:
**	13-mar-2001 (kinte01)
**	   Currently on VMS this routine just places a call to CSswitch
**	   The call in dmcwrite.c used to be a CSswitch but it has been
**	   replaced.
*/
void
CS_swuser()
{
	CSswitch();
	return;
}

/*
** Name: CSset_exsp	- Set the current exception stack pointer.
**
** Description:
**	This routine sets the current exception stack pointer in the
**	thread private data structure CS_SCB.
** Inputs:
**	EX_CONTEXT		*exsp.
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
**	26-aug-89 (rexl)
**	    written.
**	04-oct-95 (duursma)
**	    Copied this routine from UNIX CL.  Use ANSI-style declaration.
*/

static void
CSset_exsp( EX_CONTEXT	*exsp )
{
    CS_SCB	*scb;

    CSget_scb( &scb );
    scb->cs_exsp = (PTR) exsp;
    return;
}

/*
** Name: CSget_exsp	- Get the current exception stack pointer.
**
** Description:
**		This routine returns a pointer to the current exception
**		stack context for the current thread.
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    EX_CONTEXT *
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-aug-89 (rexl)
**	    written.
**	04-oct-95 (duursma)
**	    Copied this routine from UNIX CL.
**
*/

static EX_CONTEXT **
CSget_exsp()
{
    CS_SCB	*scb;

    CSget_scb( &scb );
    return( (EX_CONTEXT **)&scb->cs_exsp );
}

/*
** Name: CSms_thread_nap	-- put this thread to sleep for some millisecs
**
** Description:
**	This routine puts the current thread to sleep for the indicated number
**	of milliseconds.
**
** Inputs:
**	ms			-- number of milliseconds to sleep.
**
** Outputs:
**	None
**
** Returns:
**	void
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Fixed race condition bugs -- must set the session up as sleeping
**	    before calling $setimr, otherwise the timer may expire before
**	    we mark the thread as sleeping!
**	26-jul-1993 (bryanp)
**	    If timeout is zero or negative, just return immediately. This is
**		not actually used by any current callers, but it gives this
**		routine more well-defined semantics.
**	27-jul-95 (dougb)
**	    Make queue inserts consistent throughout.
**	24-jan-1996 (dougb) bug 71590
**	    CS_ssprsm() now takes a single parameter -- just restore context.
**      22-Mar-2001 (horda03) Bug 90741
**          As this function changes the state of the session to CS_EVENT_WAIT
**          the active session count must be decremented.
*/
void
CSms_thread_nap(i4	ms)
{
    if (ms <= 0)
	return;

#ifdef OS_THREADS_USED
#ifdef ALPHA
     /* Alpha can be Internal or OS threaded */

     if (Cs_srv_block.cs_mt)
     {
#endif /* ALPHA */

        CSsuspend(CS_TIMEOUT_MASK, - ms, 0 );
        return;

#ifdef ALPHA
     }
     else
#endif /* ALPHA */
#endif /* OS_THREADS_USED */

#if defined(ALPHA) || !defined(OS_THREADS_USED)
    if (Cs_srv_block.cs_state != CS_INITIALIZING)
    {
        CS_SCB *scb;
        i4     status;
        i4     delta_timeout[2];

	scb = Cs_srv_block.cs_current;

	delta_timeout[0] = ms * -10000;
	delta_timeout[1] = -1;

	Cs_srv_block.cs_inkernel = 1;
	scb->cs_state = CS_EVENT_WAIT;
	scb->cs_inkernel = 1;

	/* Now take this scb out of the ready queue */
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = 
	    scb->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
	    scb->cs_rw_q.cs_q_next;

        /* If this session isn't an Internal thread, then decrment the
        ** number of active session count.
        */
        if (scb->cs_thread_type != CS_INTRNL_THREAD)
           Cs_srv_block.cs_num_active--;

	/* if this was the last block, then turn off the ready bit */

	if ((scb->cs_rw_q.cs_q_next ==
			Cs_srv_block.cs_rdy_que[scb->cs_priority])
		&& (scb->cs_rw_q.cs_q_prev ==
			Cs_srv_block.cs_rdy_que[scb->cs_priority]))
	    Cs_srv_block.cs_ready_mask &=
			~(CS_PRIORITY_BIT >> scb->cs_priority);

	/*
	** We put the thread on the regular wait queue, rather than the timeout
	** queue because the timeout handling is done specially.
	*/

	scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_wt_list->cs_rw_q.cs_q_next;
	scb->cs_rw_q.cs_q_prev = Cs_srv_block.cs_wt_list;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;

	/*
	** Now that the thread is correctly marked as sleeping, schedule a
	** timer to awaken it after the specified number of milliseconds.
	** If we fail to set a timer, we'll just return immediately. The
	** effect is that the millisecond sleep passed *awfully* quickly...
	*/
	status = sys$setimr(EFN$C_ENF, delta_timeout,
			    CS_ms_timeout, scb, 0);
	if ((status & 1) == 0)
	    CS_ms_timeout(scb);

	/* now done, so go install the next thread */

	scb->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;
	if (Cs_srv_block.cs_async)
	{
	    CS_move_async(scb);
	}

	CS_stall();
	/* time passes, things get done.  Finally we resume ... */
    }

    return;
#endif /* ALPHA || !OS_THREADS_USED */
}
static void
CS_ms_timeout(CS_SCB *scb)
{
    /*
    ** If the Server is not running inkernel, then we can move
    ** the scb off the wait list and put it into the ready queue.
    ** If the server is inkernel, then we can't muck with any of
    ** the control block lists, so we put the scb on the async
    ** ready list.  This list is touched only by atomic routines
    ** which are protected against thread switching and AST driven
    ** routines.
    **
    ** When the session which is running inkernel returns from
    ** inkernel mode, it will move the scb onto the ready queue.
    */
    if (Cs_srv_block.cs_inkernel == 0)
    {
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		    scb->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		    scb->cs_rw_q.cs_q_next;

	/* now link into the ready queue */

	scb->cs_rw_q.cs_q_next =
	    Cs_srv_block.cs_rdy_que[scb->cs_priority];
	scb->cs_rw_q.cs_q_prev =
	    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);

        /* The count of active sessions was decremented before the session
        ** started its nap, so increment the session count provided the
        ** session was awakened by another function (CSresume for instance).
        */
        if ( (scb->cs_thread_type != CS_INTRNL_THREAD) &&
             (scb->cs_state != CS_COMPUTABLE))
           Cs_srv_block.cs_num_active++;

	scb->cs_state = CS_COMPUTABLE;
	if (Cs_srv_block.cs_state == CS_IDLING)
	    sys$wake(0, 0);
	return;
    }
    else
    {
	/*
	** Put this session on the async ready queue.
	** It will be moved onto the server ready queue by the
	** session that is running inkernel.
	*/
	if (scb->cs_as_q.cs_q_next == 0)
	{
	    scb->cs_as_q.cs_q_next =
		Cs_srv_block.cs_as_list->cs_as_q.cs_q_next;
	    scb->cs_as_q.cs_q_prev = Cs_srv_block.cs_as_list;
	    scb->cs_as_q.cs_q_prev->cs_as_q.cs_q_next = scb;
	    scb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev = scb;
	}
	Cs_srv_block.cs_async = 1;

	return;
    }
}

/* Address comparison function for SPTREEs */

i4
CS_addrcmp( const char *a, const char *b )
{
    if (a == b)
        return 0;
    else
        return (a > b) ? 1 : -1;
}

static	void
yield_cpu_time_slice(i4 owning_pid, i4 sleep_time)
{
    i4	time[2];
    i4	status;
    i4	no_asts;
    
    /* Make sure ASTs are enabled while we sleep */
    no_asts = (sys$setast(1) == SS$_WASCLR);

    sleep_time /= 1000;	    /* convert micro-seconds to milliseconds */

    time[1] = -1;
    time[0] = sleep_time * -10000;

    status = sys$schdwk(0, 0, time, 0);
    if ((status & 1) == 0)
    {
        CL_ERR_DESC          error;
        char                err_buffer[150];
        STprintf(err_buffer, "sys$schdwk error status %x in process %x\n",
                 status, owning_pid);
        ERlog(err_buffer, STlength(err_buffer), &error);
	PCexit(FAIL);
    }
    sys$hiber();

    if ( no_asts )
	sys$setast(0);

    return;
}


/*{
** Name: CSnoresnow()   - Check if thread has been CSresumed 
**                        
** Descriptions:
**      This routine was added initially as a diagnostic tool for debugging
**      bug 48904. It now is also serving as a workaround in that when it finds
**      a thread that has been CSresumed too early, it writes a message to 
**      the II_DBMS_LOG stating from which routine it was called and then
**      calls CScancelled.  This has the effect of "fixing" the bug; all it
**      really does is fix up the damage that has been done, not prevent 
**      from happening.
**
** Inputs:
**      descrip   - Text to identidy position of the call.
**
**      pos       - Unique position marker.
**
** Outputs:
**      Prints message to II_DBMS_LOG if edone bit set for thread
**      Returns:
**         Nothing
**
** Side effects:
**      If this routine is called at the wrong time, the thread may end
**      up hanging waiting for a CSresume that will never happen.  
**
*/
void
CSnoresnow(descrip, pos)
char *descrip;
int  pos;
{
    CS_SCB              *scb;

    scb = Cs_srv_block.cs_current;

    if (scb != NULL)
    {
        Cs_srv_block.cs_inkernel = 1;
        scb->cs_inkernel = 1;

        if (scb->cs_mask & CS_EDONE_MASK)
        {
            TRdisplay("[%x, %x] %@ EDONE bit set prior to call from %s Pos %d msk %08x\n",
                      Cs_srv_block.cs_pid, scb, descrip, pos, scb->cs_mask);

            CScancelled(0);
        }

        scb->cs_inkernel = 0;
        Cs_srv_block.cs_inkernel = 0;
        if (Cs_srv_block.cs_async)
            CS_move_async(scb);
    }
}

/*{
** Name:	CSmo_bio_????_get -- MO get methods for sum of BIO
**				     read/write time, waits, done.
**
** Description:
**	Returns the sum of BIO read/write time, waits, done.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with BIO sum, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
static STATUS
CSmo_bio_time_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_bior_time +
		        Cs_srv_block.cs_wtstatistics.cs_biow_time),
		lsbuf, sbuf));
}
static STATUS
CSmo_bio_waits_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_bior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_biow_waits),
		lsbuf, sbuf));
}
static STATUS
CSmo_bio_done_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_bior_done +
		        Cs_srv_block.cs_wtstatistics.cs_biow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CSmo_dio_????_get -- MO get methods for sum of DIO
**				     read/write time, waits, done.
**
** Description:
**	Returns the sum of DIO read/write time, waits, done.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with DIO sum, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
static STATUS
CSmo_dio_time_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_dior_time +
		        Cs_srv_block.cs_wtstatistics.cs_diow_time),
		lsbuf, sbuf));
}
static STATUS
CSmo_dio_waits_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_dior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_diow_waits),
		lsbuf, sbuf));
}
static STATUS
CSmo_dio_done_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_dior_done +
		        Cs_srv_block.cs_wtstatistics.cs_diow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CSmo_lio_????_get -- MO get methods for sum of LIO
**				     read/write time, waits, done.
**
** Description:
**	Returns the sum of LIO read/write time, waits, done.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with LIO sum, unsigned integer.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	20-Sep-2000 (jenjo02)
**	    created.
*/
static STATUS
CSmo_lio_time_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_lior_time +
		        Cs_srv_block.cs_wtstatistics.cs_liow_time),
		lsbuf, sbuf));
}
static STATUS
CSmo_lio_waits_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_lior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_liow_waits),
		lsbuf, sbuf));
}
static STATUS
CSmo_lio_done_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8) (Cs_srv_block.cs_wtstatistics.cs_lior_done +
		        Cs_srv_block.cs_wtstatistics.cs_liow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CS_num_active
**		CS_hwm_active
**
**	External functions to compute and return the count of
**	user/iimonitor sessions in CS_COMPUTABLE state.
**
** Description:
**
**	Computes and returns cs_num_active as the number of
**	CS_COMPUTABLE, CS_USER_THREAD and CS_MONITOR thread 
**	types. Also updates cs_hwm_active, the high-water
**	mark.
**
**	cs_num_active, cs_hwm_active are of no intrinsic use
**	to the server, so we computed them only when requested
**	by MO or IIMONITOR.
**
** Inputs:
**	none
**
** Outputs:
**	cs_num_active   Current number of CS_COMPUTABLE user+iimonitor
**			threads.
**	cs_hwm_active	HWM of that value.
**	
**
** Returns:
**	CS_num_active : The number of active threads, as defined above.
**	CS_hwm_active : The hwm of active threads.
**
** History:
**	22-May-2002 (jenjo02)
**	    Created to provide a consistent and uniform
**	    method of determining these values for both
**	    MT and Ingres threaded servers.
*/
i4	
CS_num_active(void)
{
    CS_SCB	*scb;
    i4		num_active = 0;

    CSp_semaphore(1, &Cs_known_list_sem);

    for ( scb = Cs_srv_block.cs_known_list->cs_prev;
	  scb != Cs_srv_block.cs_known_list;
	  scb = scb->cs_prev )
    {
	if ( scb->cs_state == CS_COMPUTABLE &&
	    (scb->cs_thread_type == CS_USER_THREAD ||
	     scb->cs_thread_type == CS_MONITOR) )
	{
	    num_active++;
	}
    }

    if ( (Cs_srv_block.cs_num_active = num_active)
	    > Cs_srv_block.cs_hwm_active )
	Cs_srv_block.cs_hwm_active = num_active;

    CSv_semaphore(&Cs_known_list_sem);

    return(num_active);
}

i4	
CS_hwm_active(void)
{
    /* First, recompute active threads, hwm */
    CS_num_active();
    return(Cs_srv_block.cs_hwm_active);
}
