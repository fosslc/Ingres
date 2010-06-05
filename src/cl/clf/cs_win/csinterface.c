/*
** Copyright (c) 1987, 2004 Ingres Corporation
**
*/
#pragma intrinsic(_InterlockedCompareExchange64)
#include    <compat.h>
#include    <ci.h>
#include    <cs.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <gl.h>
#include    <me.h>
#include    <mo.h>
#include    <mu.h>
#include    <nm.h>
#include    <pc.h>
#include    <qu.h>
#include    <si.h>
#include    <sp.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <csinternal.h>
#include    <machconf.h>
#include    <clconfig.h>
#include    <meprivate.h>

#include    "cslocal.h"
#include    "csmgmt.h"

/*
**  Name: CSINTERFACE.C - Control System External Interface
**
**  Description:
**  This module contains those routines which provide the user interface to
**  system dispatching services to the DBMS (only, at the moment).
**
**   CSinitiate()        - Specify startup characteristics, and allow
**                         control system to initialize itself.
**   CSterminate()       - Cancel all threads and prepare for shutdown.
**   CSalter()           - Alter some/all of the system characteristics 
**                         specified by CSinitiate().
**   CSdispatch()        - Have dispatcher take over operation of the system.
**   CSremove()          - Remove a thread from the system (forcibly).
**   CSsuspend()         - Suspend a thread pending completion of an
**                         operation
**   CSresume()          - Resume a suspended thread
**   CScancelled()       - Inform the CS module that an event for which
**                         a thread suspended will never occur
**   CSadd_thread()      - Add a thread to the server.
**   CSstatistics()      - Obtain runtime statistics for the current thread
**   CSattn()            - Inform the dispatcher that an unusual event
**                         has occurred
**   CSget_sid()         - Obtain session id for the current thread
**   CSget_scb()         - Obtain pointer to current thread control block
**   CSget_svrid()       - Get the current server name
**   CSfind_sid()	 - Obtain thread id for the given control block
**   CSfind_scb()        - Obtain pointer to thread control block given sid
**   CSintr_ack()        - Clear CS interruupt status
**   CSdefine_lrregion() - Define the Limited Resource Region (meaning is
**                         application dependent)
**   CSenter_lrregion()  - Enter resource limited region.
**   CSexit_lrregion()   - Exit resource limited region.
**   CSawait_lrregion()  - Await an exit from a LRR
**   CSdump_statistics() - Dump CS statistics to Log File.
**   CSget_cpid()	 - Obtain cross-process thread identity.
**   CSadjust_i8counter() - Thread safe i8 counter adjustment.
**
**  History:
**      15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**	15-jul-95 (emmag)
**	    Number of processors should be read from the environment
**	    variable II_NUM_OF_PROCESSORS.
**	17-jul-95 (canor01)
**	    initialize MCT semaphores for MCT only
**	20-jul-95 (sarjo01)
**	    Added FreeConsole() after server writes startup message. 
**          This makes server immune to ctrl-C done from console where
**	    ingstart was performed.
**	25-jul-95 (emmag)
**	    Drop the priority of the read-ahead threads to the lowest
**	    available.
**	10-Aug-1995 (shero03 & wonst02)
**	    Add MO support 
**	19-aug-95 (emmag)
**	    Set thread priority based on the thread type.   Start off
**	    by setting the thread priority for LOG-WRITER and GROUP-COMMIT
**	    to the highest available.  Priority of FAST-COMMIT, EVENT,
**	    and WRITE-BEHIND is set to higher than normal.
**	24-aug-95 (wonst02)
**	    Add CSfind_sid() to return a SID given an SCB pointer.
**	31-aug-1995 (shero03)
**	    Add pid and cputime to the SCB.
**	14-sep-1995 (canor01)
**	    All MCT semaphores are defined in mutex.c
**	10-oct-1995 (wonst02)
**	    Add cs_facility to return facility to sampler.
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**	01-feb-1996 (canor01)
**	    Change admin thread to use an Asynchronous LISTEN.
**	08-feb-1996 (canor01)
**	    Ensure that pseudo-servers also keep their cs_pid in SCB
**	15-feb-96 (emmag)
**	    Re-add call to FreeConsole.
**	21-feb-1996 (canor01)
**	    Remove unused CsThreadsRunningEvent.
**	26-feb-1996 (canor01)
**	    Initialize global shared memory semaphore. Initialize
**	    statistics block to zeroes in CSstatistics.
**	19-mar-1996 (canor01)
**	    Replace "__declspec(thread)" thread-local storage definition
**	    for the pSCB with the TlsAlloc() version.
**	23-apr-1996 (canor01)
**	    Get number of processors from system.
**	23-apr-1996 (canor01)
**	    Do not release a suspended thread on an CS_IRPENDING_MASK.  They
**	    need to be in sync with possible recovery operations.  But
**	    do release it when marked CS_COMPUTABLE.
**	24-apr-1996 (emmag)
**	    Use ii_sysconf to determine the number of CPUs available.
**	30-apr-1996 (canor01)
**	    Give the cross-process event for the main thread a unique
**	    name based on its thread id, not CS_ADDER_ID, which would
**	    be the same for all servers.
**	01-may-1996 (canor01)
**	    Add debugging code to allow breaking into CSsuspend().
**      13-dec-1996 (canor01)
**          If a session is waiting on an Ingres condition, wake it up
**          for shutdown.
**	31-dec-96 (mcgem01)
**	    Interpret a negative timeout value passed to CSsuspend as a
**	    millisecond sleep time, as is done on UNIX.
**	13-feb-1997 (cohmi01)
**	    Move SetEvent() in CSremove() done when thread was computable to
**	    be done only when an otherwise unrecognized non-computable state
**	    occurs, otherwise future event-waits by the sid being removed
**	    will return prematurely.  (Bug 80740)
**	07-mar-1997 (canor01)
**	    Add CS_is_mt() function to return multi-threading status 
**	    (always true for current NT CS).
**      06-mar-1997 (reijo01)
**          Changed II_DBMS_SERVER message to use generic system name.
**	24-mar-1997 (canor01)
**	    Add CS_NANO_TO_MASK to CSsuspend() to allow timeouts in 
**	    nanoseconds.
**      10-apr-1997 (somsa01)
**          Added CsMultiTaskSem, which is a mutex. What was happening was
**          that, in CSterminate(), there was contention between the actual
**          shutdown thread (iimonitor) and the "Dead Process Detector" thread
**          (or perhaps some other system thread(s)) for shutting down a
**          server's system threads. This would cause extra invalid threads to
**          stay on the LXB wait queue, thus causing application errors with
**          rcpconfig.exe, iiacp.exe, or both. The CsMultiTaskSem mutex solves
**          this synchronization problem.  (Bug 81268)
**      02-may-1997 (canor01)
**          If a thread is created with the CS_CLEANUP_MASK set in
**          its thread_type, transfer the mask to the cs_cs_mask.
**          These threads will be terminated and allowed to finish
**          processing before any other system threads exit.
**	05-may-1997 (canor01)
**	    Do not call FreeConsole() within the server.  Some server
**	    applications need to write messages (csphil, mksyscf).
**	    Detach the process when called from iirun.
**	24-apr-1997 (canor01)
**	    Updated CSstatistics() to use GetProcessTimes() and
**	    GetThreadTimes() for CPU statistics.
**	15-may-97 (mcgem01)
**	    Add include for mu.h and cslocal.h, also removed an unused
**	    local variable.
**	23-may-1997 (reijo01)
**	    Print out II_DBMS_SERVER message only if there is a 
**	    listen address.
**	05-jun-1997 (canor01)
**	    Use a local status to retrieve result of writing the 
**	    II_DBMS_SERVER message.
**	03-Jul-1997 (radve01)
**	    Basic, debugging related info, printed to dbms log at system
**	    initialization.
**      08-jul-1997 (canor01)
**          Maintain semaphores on known semaphore list.
**      04-aug-97 (mcgem01)
**          Back out previous submission, due to a problem with the
**          maintenance of the known semaphore list.
**	22-jul-1997 (somsa01)
**	    Removed the printout of MElimit, as we do not use the process space
**	    limit on NT.
**      22-jul-1997 (canor01)
**          Add CSkill() to generate exception in a thread.
**      27-aug-1997 (canor01)
**          Release final thread-local storage before ending process.
**      05-sep-1997 (canor01)
**          Do not initialize scb's cs_length field if the allocator has
**          already done so.
**      19-sep-1997 (canor01)
**          Make CSattn() call CSattn_scb(), which is a faster entry
**          point when we already have the scb.
**      23-sep-1997 (canor01)
**          Print startup message to stderr.
**      27-oct-1997 (canor01)
**          Restore missing line in CSattn() that was inadvertently deleted
**          in previous cross-integration.
**	03-dec-1997 (canor01)
**	    Get correct page fault statistics.
**	12-feb-1998 (canor01)
**	    Initialize cs_di_event for each thread.
**	23-feb-1998 (somsa01)
**	    In CSterminate(), removed the setting of cs_state to
**	    CS_INITIALIZING, since this is not needed anymore.
**	23-feb-1998 (canor01)
**	    For Jasmine, after writing startup message to stderr, dup stderr
**	    onto stdout so it can be used if needed.
**      31-Mar-1998 (jenjo02)
**          Added *thread_id output parm to CSadd_thread() prototype.
**	06-may-1998 (canor01)
**	    Initialize CI function to retrieve user count.  This is only 
**	    valid in a server, so initialize it here.
**	22-jun-1998 (canor01)
**	    Protect access to session count fields.
**	23-jun-1998 (canor01)
**	    Clean up typo in locking session count fields.
**	25-jun-1998 (macbr01)
**	    Bug 91641.  Was not showing lock names in NT version of jasmonitor 
**	    in the Top Locks Analysis.  Put the passed in ecb (lock name) into 
**	    the scb in the casae of a lock suspend so that jasmonitor can 
**	    print out the lock name in the Top Lock Analysis.  Fixed compiler 
**	    warnings on NT.
**	30-jun-1998 (canor01)
**	    Release CsMultiTaskSem before doing cleanup thread processing.
**	07-jul-1998 (canor01)
**	    Fix mis-matched lock/unlock.  Consolidate history entries.
**	26-Jul-1998 (rajus01)
**	    Added new TIMEOUT event. Removed references to GClisten2().
**	    Added timeout check for GClisten requests.
**	    Kept the definitions for Asynchronus Events consistent with
**	    definitions in gclocal.h.
**	29-sep-98 (mcgem01 for jenjo02)
**	    Removed call to MUset_funcs(). With OS threads, MU uses 
**	    perfectly good opsys mutexes which need no conversion to 
**	    CS_SEMAPHORE type.  The conversion was causing a race condition  
**	    which is avoided by not doing the unnecessary conversion.
**      08-oct-1998 (canor01 for jenjo02)
**          Eliminate cs_cnt_mutex, adjust counts while holding 
**          Cs_known_list_sem instead.
**	13-oct-1998 (canor01)
**	    Remove excess CSp_semaphore call introduced in previous change.
**	07-nov-1998 (canor01)
**	    CSsuspend() can now be awakened by an asynchronous BIO that timed
**	    out.
**	16-Nov-1998 (jenjo02)
**	    Added CSget_cpid() external function, CS_CPID structure,
**	    modified CScp_resume() to pass pointer to CS_CPID instead
**	    of PID, SID.
**      26-mar-1999 (canor01)
**          Get the address of CancelIo() function dynamically so process
**          will load on Windows 95 which does not have this function.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	18-oct-1999 (somsa01)
**	    Added CSget_svrid().
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of suspends.
**	    Changed CS_SID to also hold the thread's id.
**	10-jan-2000 (somsa01)
**	    Enabled usage of CS_BIOR_MASK, CS_BIOW_MASK, CS_DIOR_MASK,
**	    CS_DIOW_MASK, CS_LIOR_MASK and CS_LIOW_MASK.
**	12-jun-2000 (rigka01) bug#99542
**	    Add code to enable "set session with priority=x" for Windows NT 
**	28-jul-2000 (somsa01)
**	    When printing out the server id, use the new SIstd_write()
**	    function.
**	19-sep-2000 (somsa01)
**	    The last cross-integration accidentally changed the name of
**	    one of CSadd_thread()'s parameters.
**	10-oct-2000 (somsa01)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**	12-feb-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	22-jan-2004 (somsa01)
**	    Correct race conditions with CS condition objects.
**	23-jan-2004 (somsa01 for jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	28-jan-2004 (penga03)
**	    Ported CSresume_from_AST() and CSadjust_counter() to Windows.
**	28-jan-2004 (somsa01)
**	    Added return prototype to CSadjust_counter().
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	22-Jun-2004 (drivi01)
**	    Removed licensing.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**      20-Jul-2004 (lakvi01)
**              SIR#112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	07-Oct-2004 (somsa01)
**	    Added stub for IICSsuspend_for_AST().
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**	15-jul-2008 (smeke01) b120487
**	    Added missing ReleaseMutex() to CS_AS_PRIORITY case in 
**	    CSaltr_session().
**  17-Feb-2009 (fanra01)
**      BUG 122443
**      Minor correction as part of performance investigations.
**      28-Jan-2010 (horda03) Bug 121811
**          Enable Evidence Sets on Windows.
**	06-Apr-2010 (drivi01)
**	    Turn off optimization in this module to avoid
**          segmentation violations on x64.
**	18-May-2010 (drivi01)
**	    Added CSadjust_i8counter function which
**          will make calls to intrinsic InterlockedCompareExchange64
**          function.
*/
#ifdef WIN64
#pragma optimize ("",off)
#endif

/****************************************************************************
**
**  Defines
**
****************************************************************************/

# define DEF_PROCESSORS 1
# define CS_EVENT_NAME_W2K "%sINGRES_THREAD_ID_%d"
# define CS_INIT_SEM_NAME_W2K "%sINGRES_INIT_SEM_%s"
/*
** Defines for Asynchronous Events.
*/

#define NUM_EVENTS      5

#define LISTEN          0
#define SHUTDOWN        1
#define GCC_COMPLETE    2
#define SYNC_EVENT	3
#define TIMEOUT		4	/* Timeout event */

/****************************************************************************
**
**  Forward and/or External function references.
**
****************************************************************************/

i4 CS_addrcmp( PTR a, PTR b );

FUNC_EXTERN VOID   CSoptions(CS_SYSTEM *cssb );
FUNC_EXTERN VOID CS_dump_stack();
FUNC_EXTERN VOID GVshobj(char **ObjectPrefix);

typedef void FP_Callback(CS_SCB **scb);
FUNC_EXTERN void FP_set_callback( FP_Callback fun);

static VOID CSattn_scb( i4 eid, CS_SCB *scb );

static i4  CS_usercnt( void );

/****************************************************************************
**
** Definition of all global variables owned by this file.
**
****************************************************************************/

GLOBALREF CS_SYSTEM        Cs_srv_block;
GLOBALREF CS_SCB           Cs_main_scb;
GLOBALREF CS_SCB           Cs_known_list_hdr;
GLOBALREF CS_SEMAPHORE     Cs_known_list_sem;
GLOBALREF PTR              Cs_old_last_chance;
GLOBALREF i4               nonames_opt;
GLOBALREF CS_SEMAPHORE     *ME_page_sem;
GLOBALREF CRITICAL_SECTION SemaphoreInitCriticalSection;
GLOBALREF i4               Cs_numprocessors;
GLOBALREF i4     	   Cs_max_sem_loops;
GLOBALREF i4     	   Cs_desched_usec_sleep;
GLOBALREF i4		   dio_resumes;
GLOBALREF HANDLE	   hAsyncEvents[NUM_EVENTS];
GLOBALREF HANDLE	   hSignalShutdown;
GLOBALREF HANDLE	   CS_init_sem;
GLOBALREF VOID             (*Ex_print_stack)(); /* NULL or CS_dump_stack */
GLOBALREF VOID             (*Ex_diag_link)();

GLOBALREF VOID (*GCevent_func[NUM_EVENTS])();

GLOBALREF i4     	   Cs_lastquant;

static BOOL (WINAPI *pfnGetProcessMemoryInfo)( 
					HANDLE Process, 
					PPROCESS_MEMORY_COUNTERS ppsmemCounters,
					DWORD cb ) = NULL;
static BOOL (FAR WINAPI *pfnCancelIo)(HANDLE hFile) = NULL;

/****************************************************************************
**
** The CS multi-tasking semaphore allows all the various threads to
** cooperate on when they will begin. Until we are ready to 'really' start
** multi-threading, we will have this semaphore set, and no added threads
** will run (they will all block in CS_setup). Once we clear it, which we
** do when we are ready to multi-task, all the threads will run.
**
****************************************************************************/
GLOBALREF HANDLE CsMultiTaskEvent;
GLOBALREF HANDLE CsMultiTaskSem;


/****************************************************************************
**
** Thread local storage
**
****************************************************************************/
FACILITYREF DWORD TlsIdxSCB;


/****************************************************************************
**
** Name: CSinitiate - Initialize the CS and prepare for operation
**
** Description:
**
** This routine is called at system startup time to allow the CS module
** to initialize itself, and to allow the calling code to provide the
** appropriate personality information to the module.
**
** The personality information is provided by the single parameter, which
** is a pointer to a CS_CB structure.  This structre contains personality
** information including the "size" of the server in terms of number of
** allowable threads, the size and number of stacks to be partitioned
** amongst the threads, and the addresses of the routines to be called
** to allocate needed resources and perform the desired work.  (The calling
** sequences for these routines are described in <cs.h>.)
**
** Inputs:
**  argc                Ptr to argument count for host call
**  argv                Ptr to argument vector from host call
**  ccb                 Call control block, of which the following fields are
**                      interesting (If zero, the system is initialized to
**                      allow standalone programs which may use CL components
**                      which call CS to operate):
**      .cs_scnt        Number of threads allowed
**      .cs_ascnt       Number of active threads allowed
**      .cs_stksize     Size of stacks in bytes
**      .cs_shutdown    Server shutdown
**                      rv = (*cs_shutdown)();
**      .cs_startup     Server startup
**                      rv = (*cs_startup)(&csib);
**                           where csib has the following fields:
**                           - csib.cs_scnt          -- connected threads
**                           - csib.cs_ascnt         -- active threads
**                           - csib.cs_multiple      -- are objects in this
**                                                      server served by
**                                                      multiple servers
**                           - csib.cs_cursors       -- # of cursors per thread
**                                                      allowed
**                           - csib.cs_min_priority  -- minimum priority a CS 
**                                                      client can start a
**                                                      thread with.
**                           - csib.cs_max_priority  -- maximum priority a CS 
**                                                      client can start a
**                                                      thread with.
**                           - csib.cs_norm_priority -- default priority for a 
**                                                      session if no priority
**                                                      is specified at startup
**                                                      time.
**                           - csib.cs_optcnt        -- number of options
**                           - csib.cs_option        -- array of option blocks
**                                                      describing application
**                                                      specific options.
**              >>           - csib.cs_name          -- expected to be filled
**              >>                                      with the server name
**      .cs_scballoc    Routine to allocate SCB's
**                      rv = (*cs_scballoc)(&place to put scb, buffer);
**                               where buffer is that filled by (*cs_saddr)()
**                  >>  scb->cs_username is expected to be filled with the
**                  >>  name of the invoking user.
**      .cs_scbdealloc  Routine to dealloc  SCB's
**                      rv = (*cs_scbdealloc)(&scb alloc'd above);
**      .cs_elog        Routine to log errors
**                      rv = (*cs_elog)(internal error, &system_error,
**                                      p1_length, &p1_value, ...);
**      .cs_process     Routine to do major processing
**                      rv = (*cs_process)(mode, &scb, &next_mode);
**                           where mode and next mode are one of
**                           CS_INITIATE,
**                           CS_INPUT,
**                           CS_OUTPUT,
**                           CS_EXCHANGE, or
**                           CS_TERMINATE
**                      nexe_mode is filled by the callee.
**      .cs_attn        Routine to call to handle async events
**                      (*cs_attn)(event_type, scb for event)
**      .cs_format      Routine to call to format scb's for monitor
**      .cs_facility    Routine to call to return the facility for sampler
**      .cs_read        Routine to call to get input
**                      (*cs_read)(scb, sync)
**                           where here and below, the <sync> parameter
**                           indicates whether the service should operate
**                           synchronously or not.  If not, then the caller
**                           will suspend, unless otherwise noted.
**      .cs_write       Routine to call to place output
**                      (*cs_write)(scb, sync) will be called repeatedly as
**                      long as it continues to return OK.  To normally
**                      terminate the writing return E_CS001D_NO_MORE_WRITES.
**                      (This assumes users "ask short questions requiring
**                      long answers".)
**      .cs_saddr       Routine to call to request threads.
**                      (*cs_saddr)(buffer, sync)
**                      >>> In this case, the thread to be resumed should be
**                      CS_ADDER_ID.
**      .cs_reject      Routine to call to reject threads.
**                      (*cs_reject)(buffer, sync)
**                           where buffer is that used on the previous
**                           (*cs_saddr)() call.
**      .cs_disconnect  Routine to call to disconnect communications with
**                      partner.
**                      (*cs_disconnect)(scb)
**
** Outputs:
**      none
**  Returns:
**      STATUS
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**  Design:  <OS/2>
**
**  This routine first initializes the system control area, Cs_srv_block.
**  This involves the clearing of the structure, moving the tag information
**  into the first field in the structure, taking note of the appropriate
**  set of system specific identification information.  Finally, the
**  personality information is placed into the system control area, and that
**  area is marked as initialized.  The startup parameters are read from the
**  server startup program (server specific) via a pipe.
**
**  In CSdispatch(), when the server has completed initialization, the
**  server will return a message to iirundbms indicating the number that
**  identifies this server.  This also serves as the indication that the
**  server has started up correctly.
**
**
**  History:
**	24-jun-95 (emmag)
**	    Modified to use Thread local storage, based on the work done
**	    by Ed Andrews in 6.4 for NT.
**	25-jun-95 (emmag & canor01)
**	    Fixed to work with the new Async GC.  Don't even think of
**	    changing the call to WaitForSingleObjectEx!   
**	25-jun-95 (canor01)
**	    Removed DeleteCriticalSection's in CSterminate as they're 
**	    required later by CSp_semaphore.   
**	07-jul-1995 (shero03)
**	    CSremove should be called with a SIB not a SCB
**	12-jul-95 (canor01)
**	    Added CSset_scb() as a corollary function to CSset_sid(),
**	    also for debugging purposes.
**	13-jul-1995 (shero03)
**	    Wait until all threads have stopped before yanking the 
**	    system facilities.
**	15-jul-95 (emmag)
**	    Set up a NULL DACL and use it for security.
**	    Set the number of processors based on the environment variable.
**	21-jul-1995 (wonst02)
**	    Add MO (Managed Objects) and initialize.
**	12-sep-1995 (canor01)
**	    CSinitiate now just quietly returns OK if called twice (from
**	    unix code 5-nov-1992).
**      2-jul-1997 (canor01)
**          Initialize Ex_dump_stack to call CS_dump_stack().
**      05-sep-1997 (canor01)
**          Do not initialize scb's cs_length field if the allocator has
**          already done so.
**	04-dec-1997 (canor01)
**	    Initialize the priority fields of the csib: on Alpha, they
**	    were defaulting to negative numbers.
**      16-Feb-98 (fanra01)
**          Add initialisation of the attach and detach functions.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	22-jan-2004 (somsa01)
**	   Removed initialization of ConditionCriticalSection.
**	02-oct-2007 (thaju02)
**	    Initialize cs_scb_mutex and cs_cnd_mutex. (B119243)
**
****************************************************************************/
STATUS
CSinitiate(i4   *argc,
           char  ***argv,
           CS_CB *ccb)
{
	DWORD          		status = 0;
	char           		szEventName[MAX_PATH];
	char           		szMutexName[MAX_PATH];
	char			*inst_id;
	SECURITY_ATTRIBUTES	sa;
	CS_SCB			**ppSCB;
	CS_SCB			*pSCB;
	char			*ObjectPrefix;

	iimksec (&sa);
	GVshobj(&ObjectPrefix);

	if (Cs_srv_block.cs_state)
		return (OK);

	if (ii_sysconf(IISYSCONF_PROCESSORS, &Cs_numprocessors))
	    Cs_numprocessors = 1;
	if (ii_sysconf(IISYSCONF_MAX_SEM_LOOPS, &Cs_max_sem_loops))
	    Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
	if (ii_sysconf(IISYSCONF_DESCHED_USEC_SLEEP, &Cs_desched_usec_sleep))
	    Cs_desched_usec_sleep = 20000;

	MEfill(sizeof(Cs_srv_block), '\0', &Cs_srv_block);

	Cs_srv_block.cs_state = CS_INITIALIZING;

        InitializeCriticalSection(&SemaphoreInitCriticalSection);

# ifdef xCL_NEED_SEM_CLEANUP
        /* initialize known semaphore lists */
        QUinit(&Cs_srv_block.cs_multi_sem);
        QUinit(&Cs_srv_block.cs_single_sem);
        CS_synch_init( &Cs_srv_block.cs_semlist_mutex );
# endif /* xCL_NEED_SEM_CLEANUP */

	/* Initialize shared memory semaphore mutex */
	NMgtAt( "II_INSTALLATION", &inst_id );
    sprintf( szMutexName, CS_INIT_SEM_NAME_W2K, ObjectPrefix, inst_id );

	CS_init_sem = CreateMutex(&sa,
	                          FALSE,
	                          szMutexName);
	if ( CS_init_sem == NULL )
	    return ( E_CS00FF_FATAL_ERROR );


	/* Initialize MO (Managed Objects) */

	CS_mo_init();
	(void) MOclassdef( MAXI2, CS_int_classes );

	ERinit((i4) ER_SEMAPHORE, CSp_semaphore, CSv_semaphore,
		CSi_semaphore, CSn_semaphore);

	/* initialize the known SCB queue to empty */

	Cs_srv_block.cs_known_list    = &Cs_known_list_hdr;
	Cs_srv_block.cs_pid		 = GetCurrentProcessId();

	/* Initialize TLS */
	if (TlsIdxSCB == 0)
	{
	    TlsIdxSCB = TlsAlloc();
	    TlsSetValue( TlsIdxSCB, NULL );
	}
        ppSCB = TlsGetValue( TlsIdxSCB );
        if (ppSCB == NULL)
        {
            ppSCB = (CS_SCB **) MEreqmem(0,sizeof(CS_SCB **),TRUE,NULL);
            TlsSetValue( TlsIdxSCB, ppSCB );
        }

	*ppSCB			      = &Cs_main_scb;

	pSCB = *ppSCB;

	Cs_known_list_hdr.cs_next        = pSCB;
	Cs_known_list_hdr.cs_prev        = pSCB;
	Cs_known_list_hdr.cs_type        = CS_SCB_TYPE;
	Cs_known_list_hdr.cs_owner       = (PTR)CS_IDENT;
	Cs_known_list_hdr.cs_tag         = CS_TAG;
	Cs_known_list_hdr.cs_stk_size    = 0;
	Cs_known_list_hdr.cs_state       = CS_COMPUTABLE;
	Cs_known_list_hdr.cs_self        = (CS_SID)&Cs_known_list_hdr;
	Cs_known_list_hdr.cs_thread_id   = 0;
	Cs_known_list_hdr.cs_client_type = 0;


	DuplicateHandle (GetCurrentProcess(),
			GetCurrentThread(),
			GetCurrentProcess(),
			&pSCB->cs_thread_handle,
			(DWORD) NULL,
			TRUE,
			DUPLICATE_SAME_ACCESS);

	pSCB->cs_type        = CS_SCB_TYPE;
	pSCB->cs_owner       = (PTR)CS_IDENT;
	pSCB->cs_tag         = CS_TAG;
        if ( pSCB->cs_length == 0 )
            pSCB->cs_length  = sizeof(CS_SCB);
	pSCB->cs_state       = CS_COMPUTABLE;
	pSCB->cs_self        = CS_ADDER_ID;
	pSCB->cs_thread_id   = GetCurrentThreadId();
	pSCB->cs_next        = &Cs_known_list_hdr;
	pSCB->cs_prev        = &Cs_known_list_hdr;
	pSCB->cs_client_type = 0;
	pSCB->cs_thread_type = CS_INTRNL_THREAD;
	pSCB->cs_pid	     = Cs_srv_block.cs_pid;

	MEmove(12,
	       " <main job> ",
	       ' ',
	       sizeof(pSCB->cs_username),
	       pSCB->cs_username);

    	sprintf(szEventName, CS_EVENT_NAME_W2K, ObjectPrefix, pSCB->cs_thread_id);

	pSCB->cs_event_sem  = CreateEvent(&sa,
	                                      FALSE,
	                                      FALSE,
	                                      szEventName);
	pSCB->cs_di_event = CreateEvent(&sa,FALSE,FALSE,NULL);

	pSCB->cs_access_sem = CreateMutex(&sa,
	                                      FALSE,
	                                      NULL);

	if ((pSCB->cs_access_sem == NULL) ||
	    (pSCB->cs_event_sem  == NULL)) {
		return (E_CS0001_INVALID_STATE);
	}

	if (ccb == NULL)
		return (OK);

	/* Set up known element trees for SCBs and Conditions.
	   Semaphores were handled by CS_mo_init before the
	   pseudo-server early return so there sems would
	   be known. */

	CS_synch_init( &Cs_srv_block.cs_scb_mutex );
	SPinit( (SPTREE *)&Cs_srv_block.cs_scb_tree,
		(SP_COMPARE_FUNC *)CS_addrcmp );
	Cs_srv_block.cs_scb_ptree = &Cs_srv_block.cs_scb_tree;
	Cs_srv_block.cs_scb_tree.name = "cs_scb_tree";
	MOsptree_attach( Cs_srv_block.cs_scb_ptree );

	CS_synch_init( &Cs_srv_block.cs_cnd_mutex );
	SPinit( (SPTREE *)&Cs_srv_block.cs_cnd_tree,
		(SP_COMPARE_FUNC *)CS_addrcmp );
	Cs_srv_block.cs_cnd_ptree = &Cs_srv_block.cs_cnd_tree;
	Cs_srv_block.cs_cnd_tree.name = "cs_cnd_tree";
	MOsptree_attach( Cs_srv_block.cs_cnd_ptree );

	CS_scb_attach( pSCB );

	/*
	 * copy over supplied information
	 */

	Cs_srv_block.cs_shutdown     = ccb->cs_shutdown;
	Cs_srv_block.cs_startup      = ccb->cs_startup;
	Cs_srv_block.cs_scballoc     = ccb->cs_scballoc;
	Cs_srv_block.cs_scbdealloc   = ccb->cs_scbdealloc;
	Cs_srv_block.cs_read         = ccb->cs_read;
	Cs_srv_block.cs_write        = ccb->cs_write;
	Cs_srv_block.cs_saddr        = ccb->cs_saddr;
	Cs_srv_block.cs_reject       = ccb->cs_reject;
	Cs_srv_block.cs_process      = ccb->cs_process;
	Cs_srv_block.cs_disconnect   = ccb->cs_disconnect;
	Cs_srv_block.cs_elog         = ccb->cs_elog;
	Cs_srv_block.cs_attn         = ccb->cs_attn;
	Cs_srv_block.cs_format       = ccb->cs_format;
        Cs_srv_block.cs_diag         = ccb->cs_diag;
	Cs_srv_block.cs_facility     = ccb->cs_facility;
	Cs_srv_block.cs_max_sessions = ccb->cs_scnt;
	Cs_srv_block.cs_max_active   = ccb->cs_ascnt;
	Cs_srv_block.cs_stksize      = ccb->cs_stksize;
	Cs_srv_block.cs_mask         = 0;
	Cs_srv_block.cs_pid	     = GetCurrentProcessId();

        Cs_srv_block.cs_scbattach    = ccb->cs_scbattach;
        Cs_srv_block.cs_scbdetach    = ccb->cs_scbdetach;

	/*
	** Get CS options
	*/

	CSoptions(&Cs_srv_block);

	/*
	** Store some values
	*/

	if (Cs_srv_block.cs_max_active >  Cs_srv_block.cs_max_sessions) 
	{
		Cs_srv_block.cs_max_active =  Cs_srv_block.cs_max_sessions;
	}

	Cs_srv_block.cs_stksize = (Cs_srv_block.cs_stksize + 0xfff) & ~0xfff;

	return (OK);
}

/****************************************************************************
**
** Name: CSterminate()  - Prepare to and terminate the server
**
** Description:
**  This routine is called to prepare for and/or perform the shutdown
**  of the server.  This routine can be called to inform the server that
**  shutdown is pending so new connections should be disallowed, or that
**  shutdown is to be performed immediately, regardless of state.
**  The latter is intended to be used in cases of major failure's which
**  prohibit the continued operation of the server.  The former is used in
**  the normal course of events, when a request to shutdown has been issued
**  from the monitor task.  There is, actually, a third category which is
**  "between" these two, which is to shutdown immediately iff there are
**  no current threads.
**
**  This routine operates as follows.  If the request is to gradually shut
**  the server down, then the server state is marked as CS_SHUTDOWN.  This
**  will prevent any new users from connecting to the server, and will
**  cause this routine to be called again (with CS_START_CLOSE mode) when
**  the last user thread has exited.  At this point, if there are connected
**  user threads, control returns.
**
**  If there are no connected user threads, then each remaining thread is
**  a specialized server thread.  Each of these sessions is interrupted
**  with the CS_SHUTDOWN_EVENT.  This should cause the termination of any
**  server-controlled threads.  The server is then marked in CS_FINALSHUT
**  state. This state will cause the server to be terminated when the last
**  thread is exited.  At this point, if there are still server threads
**  active, control returns.
**
**  If there are no threads active, the server shutdown routine is called
**  and the process is terminated.
**
**  If CSterminate is called with CS_KILL, then any connected threads
**  are removed, the server shutdown routine is called and the process is
**  terminated.
**
**  If CSterminate is called with CS_CRASH mode, then the process is
**  just terminated.
**
** Inputs:
**      mode        one of
**                      CS_CLOSE
**                      CS_CND_CLOSE
**                      CS_START_CLOSE
**                      CS_KILL
**                      CS_CRASH
**                  as described above.
**
** Outputs:
**      active_count   filled with number of connected threads
**                     if the CS_CLOSE or CS_CND_CLOSE are requested
**  Returns:
**      Doesn't if successful
**      E_CS003_NOT_QUIESCENT if there are still threads active.
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
**  History:
**	15-feb-1996 (canor01)
**	    Must have shared semaphore for Cs_known_list_sem since it
**	    will be called again in CSattn().
**      02-may-1997 (canor01)
**          If there are any threads with the CS_CLEANUP_MASK set, they
**          should be allowed to die before signaling any of the other
**          system threads.
**      10-apr-1997 (somsa01)
**          Added CsMultiTaskSem, as we now use a mutex to control
**          synchronization between multiple threads during shutdown.
**	23-feb-1998 (somsa01)
**	    Removed the setting of cs_state to CS_INITIALIZING, since this
**	    is not needed anymore.
**
****************************************************************************/
STATUS
CSterminate(i4 mode,
            i4 *active_count)
{
	i4    status = 0;
	CS_SCB *scb;
	DWORD	tid;

        /*
        ** Take control of CsMultiTaskSem, so that we can clean up the
        ** threads properly without any contention. If there are any errors,
        ** we will release this mutex.
        */
        CS_synch_lock( &CsMultiTaskSem );

	switch (mode) 
	{
	case CS_CLOSE:
		Cs_srv_block.cs_mask |= CS_SHUTDOWN_MASK;
		/* fall thru */

	case CS_COND_CLOSE:
		/*
		 * If there are still user session active, then return -
		 * server will shutdown when last user session exits.  If
		 * there are no more user sessions, fall through to the
		 * CS_START_CLOSE state.
		 */
	 	if ( active_count != NULL )
		{
                    CSp_semaphore(1, &Cs_known_list_sem);
		    *active_count = Cs_srv_block.cs_user_sessions;
                    CSv_semaphore(&Cs_known_list_sem);
		    if (*active_count > 0)
		    {
                        CS_synch_unlock( &CsMultiTaskSem );
			return (E_CS0003_NOT_QUIESCENT);
		    }
		}
		/* fall thru */

	case CS_START_CLOSE:
		/*
		 * If FINALSHUT is on, then we've been here before
		 *   so, just return.
		 */
		if (Cs_srv_block.cs_mask & CS_FINALSHUT_MASK)
		{
                    CS_synch_unlock( &CsMultiTaskSem );
		    return (E_CS0003_NOT_QUIESCENT);
		}

                /*
                ** If there are any sessions with the CS_CLEANUP_MASK
                ** set, it means they need to finish their work
                ** before any of the other system sessions are told
                ** to shutdown.  When they exit, they will make
                ** another call to CSterminate() to signal the other
                ** system threads.
                */
		CSp_semaphore(1, &Cs_known_list_sem);
                for (scb = Cs_srv_block.cs_known_list->cs_prev;
                    scb != Cs_srv_block.cs_known_list;
                    scb = scb->cs_prev)
                {
                    if (scb->cs_cs_mask & CS_CLEANUP_MASK)
                    {
                        if ( (scb->cs_mask & CS_ABORTED_MASK) == 0 )
                            CSattn_scb(CS_SHUTDOWN_EVENT, scb);
                        CSv_semaphore(&Cs_known_list_sem);
                        CS_synch_unlock( &CsMultiTaskSem );
                        return (E_CS0003_NOT_QUIESCENT);
                    }
                }
		CSv_semaphore(&Cs_known_list_sem);

		/*
		 * Interrupt any server threads with the shutdown event -
		 * then return to caller.  Server should be shut down when
		 * last server thread has gone away.
		 */

		CSp_semaphore(1, &Cs_known_list_sem);
		for (scb = Cs_srv_block.cs_known_list->cs_next;
		     scb != Cs_srv_block.cs_known_list;
		     scb = scb->cs_next) 
		{
		    if (scb->cs_self != CS_ADDER_ID)
			CSattn_scb(CS_SHUTDOWN_EVENT, scb);
		}
		CSv_semaphore(&Cs_known_list_sem);

		Cs_srv_block.cs_mask |= CS_FINALSHUT_MASK;

		/*
		 * Return and wait for server tasks to terminate before
		 * shutting down server.
		 */
		if (Cs_srv_block.cs_num_sessions > 0)
		{
		        CS_synch_unlock( &CsMultiTaskSem );
			return (E_CS0003_NOT_QUIESCENT);
		}

		break;

	case CS_KILL:
	case CS_CRASH:
		/*
		 *  KILL is called after the main thread has
		 *  been signalled and admin_task has ended.
		 *  KILL also is called by iimonitor to shut immediately.
		 *  The difference is that in the first case all
		 *  the threads have disappeared, and the final
		 *  cleanup should be done.
		 *  In the second case the threads are to be removed.
		 */
		if ((Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) &&
		    (Cs_srv_block.cs_num_sessions > 0))
		{
		    CS_synch_unlock( &CsMultiTaskSem );
		    return (E_CS0003_NOT_QUIESCENT);
		}
		Cs_srv_block.cs_mask |= CS_SHUTDOWN_MASK;
		Cs_srv_block.cs_mask |= CS_FINALSHUT_MASK;

	        Cs_srv_block.cs_state = CS_TERMINATING;

		break;

	default:
	        CS_synch_unlock( &CsMultiTaskSem );
		return (E_CS0004_BAD_PARAMETER);
	}

	Cs_srv_block.cs_mask |= CS_NOSLICE_MASK;

	if (Cs_srv_block.cs_num_sessions == 0)
	{
	    /* Dump server statistics */

	    CSdump_statistics(TRdisplay);
	}

	/*
	 * At this point, it is known that the request is to shutdown the
	 * server, and that that request should be honored.  Thus, proceed
	 * through the known thread list, terminating each thread as it is
	 * encountered.  If there are no known threads, as should normally
	 * be the case, this will simply be a very fast process.
	 */

	if ((Cs_srv_block.cs_num_sessions > 0) &&
	    (Cs_srv_block.cs_state != CS_TERMINATING))
	{	
	    Cs_srv_block.cs_state = CS_TERMINATING;

	    /*
	    ** Signal the Server that Shutdown is in process
	    */
	    CSattn(CS_SHUTDOWN_EVENT, CS_ADDER_ID);

	    tid = GetCurrentThreadId();

	    for (scb = Cs_srv_block.cs_known_list->cs_next;
	         scb != Cs_srv_block.cs_known_list;
	         scb = scb->cs_next)
	    {

	        status = 0;
	        /*
	         * for each known thread, tell it to die
	         */
	        if (scb->cs_self == CS_ADDER_ID)
		    continue;

	        if (mode != CS_CRASH) {
	            status = CSremove(scb->cs_self);
		    if (status) {
	                (*Cs_srv_block.cs_elog) (status, 0, 0);
		    }
	        }
		
	        (*Cs_srv_block.cs_disconnect) (scb);

		/*
	        **  Ensure that this terminating thread ends quickly
	        */
		if (scb->cs_thread_id == tid)
		    scb->cs_mode = CS_TERMINATE;
	    }


	    if (active_count)
	    {
                CSp_semaphore(1, &Cs_known_list_sem);
	        *active_count = Cs_srv_block.cs_user_sessions - 1;
                CSv_semaphore(&Cs_known_list_sem);
	    }

	    /*
            ** Now that we have cleaned up the threads correctly, release
            ** CsMultiTaskSem.
            */
            CS_synch_unlock( &CsMultiTaskSem );

            CSp_semaphore(1, &Cs_known_list_sem);
	    if (Cs_srv_block.cs_user_sessions == 1)
	        status = OK;
	    else
                status = E_CS0003_NOT_QUIESCENT;
            CSv_semaphore(&Cs_known_list_sem);
	    return (status);
	}

	if (mode != CS_CRASH) {
	    status = (*Cs_srv_block.cs_shutdown) ();
	    if (status)
		(*Cs_srv_block.cs_elog) (status, 0, 0);
	    else
		(*Cs_srv_block.cs_elog) (E_CS0018_NORMAL_SHUTDOWN, 0, 0);
	}

//	DeleteCriticalSection(&SemaphoreInitCriticalSection);

        /*
        ** Just in case it hasn't been released, let's release
        ** CsMultiTaskSem.
        */
        CS_synch_unlock( &CsMultiTaskSem );

        /* for the sake of neatness, release final tls before terminating */
        ME_tls_destroyall( &status );

        if (Cs_srv_block.cs_num_sessions <= 0)
	    PCexit(0);
	exit(0);
}

/****************************************************************************
**
** Name: CSalter()  - Alter server operational characteristics
**
** Description:
**  This routine accepts basically the same set of operands as the CSinitiate()
**  routine;  the arguments passed in here replace those specified to
**  CSinitiate.  Only non-procedure arguments can be specified;  the processing
**  routine, scb allocation and deallocation, and error logging routines
**  cannot be altered.
**
**  Arguments are left unchanged by specifying the value CS_NOCHANGE.
**  Values other than CS_NOCHANGE will be assumed as intending to replace
**  the value currently in force.
**
** Inputs:
**      ccb             the call control block, with
**      .cs_scnt        Number of threads allowed
**      .cs_ascnt       Nbr of active threads allowed
**      .cs_stksize     Size of stacks in bytes
**      All other fields are ignored.
**
** Outputs:
**      none
**  Returns:
**      OK or
**      E_CS0004_BAD_PARAMETER
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
STATUS
CSalter(CS_CB *ccb)
{
	i4 prop_scnt;
	i4 prop_ascnt;

	if (!ccb ||
	    (ccb->cs_stksize != CS_NOCHANGE) &&
	    (ccb->cs_stksize < 512))
		return (E_CS0004_BAD_PARAMETER);

	if (ccb->cs_scnt  != CS_NOCHANGE ||
	    ccb->cs_ascnt != CS_NOCHANGE) {

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
			return (E_CS0004_BAD_PARAMETER);
	}

	if (ccb->cs_stksize != CS_NOCHANGE)
		Cs_srv_block.cs_stksize = ccb->cs_stksize;

	if (ccb->cs_scnt != CS_NOCHANGE)
		Cs_srv_block.cs_max_sessions = ccb->cs_scnt;

	if (ccb->cs_ascnt != CS_NOCHANGE)
		Cs_srv_block.cs_max_active = ccb->cs_ascnt;

	return (OK);
}

/****************************************************************************
**
** Name: CSdispatch()   - Begin multi-user operation
**
** Description:
**  This routine is called to actually begin the operation of the
**  server.  It is separated from CSinitiate() so that the server
**  code can regain control in the middle to perform any logging or
**  runtime initialization which it desires to do, or to allow the
**  initiate code to be called once while dispatch is called repeatedly
**  as is the case in some environments.
**
**  The routine operates as follows.  First the dispatcher begins
**  to initialize itself to be ready to add threads.  It builds
**  session control blocks for the dispatcher special threads (if any).
**  At the moment, this is limited to the administrative thread, which
**  is used to start and stop threads.
**
**  When the dispatcher is ready to accept addthread requests, it calls
**  the user supplied initialize server routine (no parameters).
**  It is assumed that a non-zero return indicates failure.  In the
**  event of failure, the error returned is logged, along with the
**  a message indicating failure to start up.  Then the user supplied
**  termination routine is called, thus allowing any necessary cleanup
**  to occur.
**
**  If the initiation routine succeeds (i.e. returns zero (0)), then the
**  communication subsystem is called to begin operation.
**
**  The dispatcher then begins operation by causing the idle job to be
**  resumed.  The idle job is a thread like any other which operates at
**  a low priority, sleeping when the server has nothing else to do.
**
**  If the dispatcher/idle thread notices that something nasty has occurred,
**  then it will break out of its normal operational loop.  Here, there is
**  nothing to do but shutdown, so the user supplied shutdown routine is
**  called, and a message logged that the server is dying off.  Then, the
**  exit handler is deleted and the process exits.
**
** Inputs:
**      none
**
** Outputs:
**      none
**  Returns:
**      Never, under normal circumstances
**      Should a call be made to CSdispatch() before the required
**      call to CSinitiate() is made, an E_CS0005_NO_INITIATE is
**      returned.
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	10-oct-1995 (canor01)
**	    Set up MU semaphores to use CS semaphores in 
**	    multi-threaded environment.
**      06-mar-1997 (reijo01)
**          Changed II_DBMS_SERVER message to use generic system name.
**	05-may-1997 (canor01)
**	    Do not call FreeConsole() within the server.  Some server
**	    applications need to write messages (csphil, mksyscf).
**	    Detach the process when called from iirun.
**	23-may-1997 (reijo01)
**	    Print out II_DBMS_SERVER message only if there is a 
**	    listen address.
**	05-jun-1997 (canor01)
**	    Use a local status to retrieve result of writing the 
**	    II_DBMS_SERVER message.
**	03-Jul-1997 (radve01)
**	    Basic, debugging related info, printed to dbms log at system
**	    initialization.
**	22-jul-1997 (somsa01)
**	    Removed the printout of MElimit, as we do not use the process space
**	    limit on NT.
** 	16-apr-1998 (wonst02)
** 	    Set ret_val = E_CS00FF_FATAL_ERROR where it was uninitialized.
**	11-Mar-1999 (shero03)
**	    Initialize the FP localcallback routine.
**  18-Dec-2006 (wonca01) BUG 117262
**      Change cs_elog() parameter from STATUS to
**      CL_ERR_DESC in order for a valid error code to 
**      reported.
**  17-Feb-2009 (fanra01)
**      Changed type of err_code from pointer to actual structure and pass
**      it to the macros as a pointer to the structure.
**
****************************************************************************/
GLOBALREF	char	*MEalloctab;
GLOBALREF	char	*MEbase;

STATUS
CSdispatch()
{
	i4              status = 0;
	STATUS          ret_val;
	i4              i;
	EX_CONTEXT      excontext;
	CS_INFO_CB      csib;
	FUNC_EXTERN i4  cs_handler(EX_ARGS *);
	static i4       got_ex;
	STATUS          Retc;
	char            szBuf[256];
	SECURITY_ATTRIBUTES	sa;
	CL_ERR_DESC		err_code;

	iimksec (&sa);

	if (Cs_srv_block.cs_process == 0)
		return FAIL;

	/*
	** We are still not yet ready to multi-task, but *cs_startup will
	** make some add_thread calls. In order to prevent those added
	** threads from running 'too soon', we use a semaphore which we
	** won't release until multi-tasking can begin. So we now prevent
	** any added threads from getting going, in co-operation with
	** CS_setup, who will make each thread wait for this semaphore
	** before beginning.
	*/

	/* Need to create the various global (to cs) semaphores. */

	CsMultiTaskEvent      =CreateEvent(&sa, TRUE, FALSE, NULL);
	CsMultiTaskSem        =CreateMutex(&sa, FALSE, NULL);
	Retc                  =CSw_semaphore(&Cs_known_list_sem, 
				             CS_SEM_SINGLE,
					     "Cs_known_list sem");

	if (!CsMultiTaskEvent ||
	    Retc) {
		return FAIL;
	}

	if (EXdeclare(cs_handler, &excontext) != OK) {
		if (!got_ex++) {
			(*Cs_srv_block.cs_elog) (E_CS0014_ESCAPED_EXCPTN, 0, 0);
		}
		EXdelete();
		return (E_CS0014_ESCAPED_EXCPTN);
	}

        Ex_print_stack = CS_dump_stack;
        Ex_diag_link = CSdiag_server_link;

	FP_set_callback( CSget_scb);

	for (;;) {
		/*
		 * Call server startup routines.  The server startup
		 * routine may call CSadd_thread to create specialized
		 * sessions.
		 */
		Cs_srv_block.cs_state = CS_IDLING;

		csib.cs_scnt     = Cs_srv_block.cs_max_sessions;
		csib.cs_ascnt    = Cs_srv_block.cs_max_active;
                csib.cs_min_priority = CS_MINPRIORITY;
                csib.cs_max_priority = CS_MAXPRIORITY;
                csib.cs_norm_priority = CS_DEFPRIORITY;

		if (Cs_srv_block.cs_startup(&csib) != OK) {
			SIstd_write(SI_STD_OUT, "\nFAIL\n");
			ret_val = E_CS00FF_FATAL_ERROR;
			break;
		}

		/* if startup routine provided a cs_mem_sem, use it */

		if (csib.cs_mem_sem)
			ME_page_sem = csib.cs_mem_sem;

		STlcopy(csib.cs_name,
		        Cs_srv_block.cs_name,
		        sizeof(Cs_srv_block.cs_name)-1);

		sprintf(szBuf,
		        "%s_DBMS_SERVER",
			SystemVarPrefix);

		if ((Cs_srv_block.cs_define) &&
		    (NMstIngAt(szBuf, csib.cs_name) != OK)) 
		{
			(*Cs_srv_block.cs_elog)(E_CS002D_CS_DEFINE,
			                        (CL_ERR_DESC *) 0,
			                        0);
		}

# ifdef MCT
		/* initialize MCT semaphores */
		gen_useCSsems(IICSp_semaphore, IICSv_semaphore,
			      IICSi_semaphore, IICSn_semaphore);

# endif /* MCT */

		/*
		 * Signal startup program - RUNDBMS - that we have started
		 * up and pass the dbms process name.
		 * Only print server id if there is a valid listen address.
		 */

		if ( csib.cs_name[0] != EOS )
		{
		    STATUS wstatus;

		    sprintf(szBuf,
		            "\n%s_DBMS_SERVER = %s\n",
			    SystemVarPrefix,
		            csib.cs_name);

		    wstatus = SIstd_write(SI_STD_OUT, szBuf);

		    if (!wstatus)
			status = GetLastError();

		    SIstd_write(SI_STD_OUT, "PASS\n");
		}

		/*
		 * Memory pointers are saved and restored, because mainline
		 * code depends on this.  On Vax/VMS systems, this dependency is
		 * unnecessary, but it is necessary on MVS where all the
		 * threads are really separate tasks sharing an address space.
		 * Possibly useful in other implementations as well.
		 */

		Cs_srv_block.cs_svcb = csib.cs_svcb;/* save pointer to mem */

		/*
		** Save dynamic stuff we've got from sequencer
		*/
		Cs_srv_block.sc_main_sz  = csib.sc_main_sz;
		Cs_srv_block.scd_scb_sz  = csib.scd_scb_sz;
		Cs_srv_block.scb_typ_off = csib.scb_typ_off;
		Cs_srv_block.scb_fac_off = csib.scb_fac_off;
		Cs_srv_block.scb_usr_off = csib.scb_usr_off;
		Cs_srv_block.scb_qry_off = csib.scb_qry_off;
		Cs_srv_block.scb_act_off = csib.scb_act_off;
		Cs_srv_block.scb_ast_off = csib.scb_ast_off;

		/*
		** Print ad-hoc debugging info to dbms log
		*/
		TRdisplay("->\n^^^\n");
		TRdisplay("The server startup: pid= ^^^ %d\n",
			Cs_srv_block.cs_pid);
		TRdisplay("             connect_id= ^^^ %s\n",
			Cs_srv_block.cs_name);
		TRdisplay("The Server Block         ^^^ @%0p, size is ^^^ %d(%x hex)\n",
			&Cs_srv_block,
			(long)(&Cs_srv_block.srv_blk_end -
			    (volatile PTR *)&Cs_srv_block),
			(long)(&Cs_srv_block.srv_blk_end -
			    (volatile PTR *)&Cs_srv_block));
		TRdisplay("Event mask               ^^^ @%0p\n",
			&Cs_srv_block.cs_event_mask);

		TRdisplay("Current thread pointer   ^^^ @%0p\n",
			&Cs_srv_block.cs_current);
		TRdisplay("Max # of active sessions ^^^ %d\n",
			Cs_srv_block.cs_max_active);
		TRdisplay("The thread stack size is ^^^ %d\n",
			Cs_srv_block.cs_stksize);
		TRdisplay("Total # of sessions      ^^^ @%0p\n",
			&Cs_srv_block.cs_num_sessions);

		TRdisplay("# of user sessions       ^^^ @%0p\n",
			&Cs_srv_block.cs_user_sessions);

		TRdisplay("HWM of active threads    ^^^ @%0p\n",
			&Cs_srv_block.cs_hwm_active);

		TRdisplay("Current # of act.threads ^^^ @%0p\n",
			&Cs_srv_block.cs_num_active);

		TRdisplay("Curr. # of stacks alloc. ^^^ @%0p\n",
			&Cs_srv_block.cs_stk_count);
		TRdisplay("Time-out event wait list ^^^ @%0p\n",
			&Cs_srv_block.cs_to_list);
		TRdisplay("Non time-out event wait  ^^^ @%0p\n",
			&Cs_srv_block.cs_wt_list);
		TRdisplay("Global state of the sys. ^^^ @%0p\n",
			&Cs_srv_block.cs_state);
		TRdisplay(" 00-uninitialized, 10-initializing, 20-processing, 25-switching,\n");
		TRdisplay(" 30-idling, 40-closing, 50-terminating, 60-in error\n");
		TRdisplay("In kernel indicator      ^^^ @%0p\n",
			&Cs_srv_block.cs_inkernel);
		TRdisplay("Async indicator          ^^^ @%0p\n",
			&Cs_srv_block.cs_async);
		TRdisplay("->->\n");
		TRdisplay("The Sequencer Block      ^^^ @%0p,  size is ^^^ %d(%x hex)\n",
			Cs_srv_block.cs_svcb, Cs_srv_block.sc_main_sz,
			Cs_srv_block.sc_main_sz);
		TRdisplay("->->->\n");
		TRdisplay("The Session Control Block (SCD_SCB) size is ^^^ %d(%x hex)\n",
			Cs_srv_block.scd_scb_sz, Cs_srv_block.scd_scb_sz);
		TRdisplay("... session_type offset  ^^^ %d(%x hex)\n",
			Cs_srv_block.scb_typ_off, Cs_srv_block.scb_typ_off);
		TRdisplay(" 00-user session, 01-monitor session, 02-fast commit thread,\n");
		TRdisplay(" 03-write behind thread, 04-server initiator, 05-event thread,\n");

		TRdisplay(" 06-2PC recovery, 07-recovery thread, 08-log-writer thread,\n");
		TRdisplay(" 09-dead process detector, 10-group commit, 11-force abort,\n");
		TRdisplay(" 12-audit thread, 13-consistency point, 14-terminator thread,\n");
		TRdisplay(" 15-security audit, 16-write-along thread, 17-read-ahead thread\n");
		TRdisplay("... current facility off ^^^ %d(%x hex)\n",
			Cs_srv_block.scb_fac_off, Cs_srv_block.scb_fac_off);
		TRdisplay(" 0-None, 1-CLF, 2-ADF, 3-DMF, 4-OPF, 5-PSF, 6-QSF, 7-RDF,\n");
		TRdisplay(" 8-SCF, 9-ULF, 10-DUF, 11-GCF, 12-RQF, 13-TPF, 14-GWF, 15-SXF\n");
		TRdisplay("... user info offset is  ^^^ %d(%x hex)\n",
			Cs_srv_block.scb_usr_off, Cs_srv_block.scb_usr_off);
		TRdisplay("... query text offset is ^^^ %d(%x hex)\n",
			Cs_srv_block.scb_qry_off, Cs_srv_block.scb_qry_off);
		TRdisplay("... current activity off ^^^ %d(%x hex)\n",
			Cs_srv_block.scb_act_off, Cs_srv_block.scb_act_off);
		TRdisplay("... alert status offset  ^^^ %d(%x hex)\n",
			Cs_srv_block.scb_ast_off, Cs_srv_block.scb_ast_off);
		TRdisplay(" 0-default, 1-wait for interrupt(event), 2-thread was interrupted,");
		TRdisplay("\n 3-send pending, 4-ignore GCA completion\n");
		TRdisplay("->->->->\n");
		TRdisplay("Page allocation bitmap   ^^^ @%0p\n", &MEalloctab);
		TRdisplay("Private memory low value ^^^ @%0p\n", &MEbase);
		TRdisplay("Current memory low value= ^^^ %0p\n", MEbase);
		TRdisplay("->->->->->\n");

		/*
		 * Saving this pointer into the CS global memory is the last
		 * part of getting the server ready to multi-task. Any
		 * threads which have been held up because they needed this
		 * value in order to proceed may now be let loose on the
		 * world. Clearing the semaphore will unblock any threads
		 * which are hanging in their CS_setup call.
		 */

		if (!SetEvent(CsMultiTaskEvent)) {
			TRdisplay("Unable to begin full multi-tasking!\n");
			ret_val = E_CS00FF_FATAL_ERROR;
			break;
		}

		if (CS_admin_task())
		{
		    (*Cs_srv_block.cs_elog) (E_CS00FE_RET_FROM_IDLE, 0, 0);
		    ret_val = E_CS00FF_FATAL_ERROR;
		}
		else
		    ret_val = OK;

		break;
	}

	if (status || ret_val) {
		CLEAR_ERR(&err_code);
		SETWIN32ERR ( &err_code, status, ER_create);
		(*Cs_srv_block.cs_elog) (ret_val, &err_code, 0);
	}

	Cs_srv_block.cs_state = CS_CLOSING;

        while( CSterminate(CS_KILL, &i) )
	    Sleep(500);

	PCexit(((status & 9) || (ret_val == 0)) ? 0 : status);
}

/****************************************************************************
**
** Name: CSsuspend  - Suspend current thread pending some event
**
** Description:
**      This procedure is used to suspend activity for a particular
**      thread pending some event which will be delivered via a
**      CSresume() call.  Optionally, this call will also schedule
**      timeout notification.  If the event is to be resumed following
**      an interrupt, then the CS_INTERRUPT_MASK should be specified.
**
** Inputs:
**      mask        Mask of events which can terminate
**                  the suspension.  Must be a mask of
**                  CS_INTERRUPT_MASK - can be interrupted
**                  CS_TIMEOUT_MASK - event will time out
**                  along with the reason for suspension:
**                  (CS_DIO_MASK, CS_BIO_MASK, CS_LOCK_MASK).
**		    CS_NANO_TO_MASK - timeouts are in nanoseconds
**      to_cnt      number of seconds after which event
**                  will time out.  Zero means no timeout
**		    (unless CS_NANO_TO_MASK is set).
**      ecb         Pointer to a system specific event
**                  control block.  Can be zero if not used.
**
** Outputs:
**      none
**  Returns:
**      OK
**      E_CS0004_BAD_PARAMETER  if mask contains invalid bits or
**                              contains timeout and to_cnt < 0
**      E_CS0008_INTERRUPTED    if resumed due to interrupt
**      E_CS0009_TIMEOUT        if resumed due to timeout
**  Exceptions:
**      none
**
** Side Effects:
**      Task is suspended awaiting completion of event
**
** History:
**	12-sep-1995 (canor01)
**	    Do not continue the thread on WAIT_IO_COMPLETION, just go
**	    back into an alertable state to allow all async completion
**	    routines to finish.
**      01-Dec-1995 (jenjo02)
**          Defined 2 new wait reasons, CS_LGEVENT_MASK and CS_LKEVENT_MASK
**          to statistically distinguish these kinds of waits from other
**          LG/LK waits. Also fixed wait times to use Cs_sm_cb->cs_qcount
**          instead of Cs_srv_block.cs_quantums which isn't maintained
**          by any code!
**	03-jan-1996 (canor01)
**	    Correctly wait for a WAIT_TIMEOUT, as well as other valid
**	    completion.
**	16-Jan-1996 (jenjo02)
**	    CS_LOG_MASK is no longer the default wait reason and must
**	    be explicitly stated in the CSsuspend() call.
**	09-feb-1996 (canor01)
**	    The main thread must wait for the hSignalShutdown event
**	    which is its signal to shut down.
**	31-dec-96 (mcgem01)
**	    Interpret a negative timeout value as milliseconds, as is
**	    done on UNIX.
**	24-mar-1997 (canor01)
**	    Add CS_NANO_TO_MASK to allow timeouts in nanoseconds.
**	25-jun-1998 (macbr01)
**	    Bug 91641.  Was not showing lock names in NT version of jasmonitor 
**          in the Top Locks Analysis.  Put the passed in ecb (lock name) into 
**	    the scb in the casae of a lock suspend so that jasmonitor can 
**	    print out the lock name in the Top Lock Analysis.
**	07-nov-1998 (canor01)
**	    CSsuspend() can now be awakened by an asynchronous BIO that timed
**	    out.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of suspends.
**	10-jan-2000 (somsa01)
**	    Enabled usage of CS_BIOR_MASK, CS_BIOW_MASK, CS_DIOR_MASK,
**	    CS_DIOW_MASK, CS_LIOR_MASK and CS_LIOW_MASK, and now collect
**	    elapsed time info. Also, set cs_sync_obj always.
**
****************************************************************************/
STATUS
CSsuspend(i4  mask,
          i4  to_cnt,
          PTR ecb)
{
	CS_SCB *scb;
	STATUS rv = OK;
	ULONG  status;
	HANDLE eventarray [5];
        i4  to_compl_event = 4;
        i4  listen_to_event = 3;
        ASYNC_TIMEOUT *asto = NULL;
	i4	start_time, end_time, elapsed_ms;

	
	if (mask & CS_TIMEOUT_MASK) 
	{
	    if ( mask & CS_NANO_TO_MASK )
	    {
		to_cnt /= 1000000;  /* convert to milliseconds */
	    }
	    else
	    {
		if (to_cnt == 0)
		    to_cnt = -1;
	        else if (to_cnt < 0)
		    to_cnt = - to_cnt;
	        else
		    to_cnt *= 1000;
	    }
	}

	CSget_scb (&scb);

	eventarray [0] = scb->cs_event_sem; 
	eventarray [1] = hAsyncEvents[LISTEN];
	eventarray [2] = hSignalShutdown;
	eventarray [3] = hAsyncEvents[TIMEOUT];

	if (ecb)
	    scb->cs_sync_obj = ecb;

	if ((mask & CS_LIOR_MASK) == CS_LIOR_MASK)
	{
	    scb->cs_lior++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_done++;
	}
	else if ((mask & CS_LIOW_MASK) == CS_LIOW_MASK)
	{
	    scb->cs_liow++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_done++;
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
	else if (mask & CS_LOG_MASK)
	{
            scb->cs_logs++;
	    Cs_srv_block.cs_wtstatistics.cs_lg_done++;
	}
	else if (mask & CS_LOCK_MASK)
	{
	    scb->cs_locks++;
	    Cs_srv_block.cs_wtstatistics.cs_lk_done++;
        }
        else if (mask & CS_LKEVENT_MASK)
        {
            scb->cs_lkevent++;
            Cs_srv_block.cs_wtstatistics.cs_lke_done++;
        }
        else if (mask & CS_LGEVENT_MASK)
        {
            scb->cs_lgevent++;
            Cs_srv_block.cs_wtstatistics.cs_lge_done++;
        }
	else if (mask & CS_TIMEOUT_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_tm_done++;
	}
	/* We don't expect DIO (handled by DI), so check it last */
	else if ((mask & CS_DIOR_MASK) == CS_DIOR_MASK)
	{
	    scb->cs_dior++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_done++;
	}
	else if ((mask & CS_DIOW_MASK) == CS_DIOW_MASK)
	{
	    scb->cs_diow++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_done++;
	}

	/*
	 * The ordering here is important since CSresume runs asynchronously
	 * and the resume we are about to suspend for may be called at any
	 * time in the middle of while we are executing this routine. If
	 * CSresume is called and the state is not CS_EVENT_WAIT, then the
	 * event completion will just be marked in the cs_mask field.
	 * cs_access_sem is taken on the scb to gnsure that CSresume and
	 * CSsuspend do not collide.
	 */

	WaitForSingleObject(scb->cs_access_sem, INFINITE);
	if (scb->cs_mask & CS_EDONE_MASK) {
		/*
		 * Then there is nothing to do, the event is already done.
		 * In this case, we can ignore the suspension
		 */
		scb->cs_mask &= ~(CS_EDONE_MASK |
		                  CS_IRCV_MASK |
		                  CS_TO_MASK |
		                  CS_ABORTED_MASK |
		                  CS_INTERRUPT_MASK |
		                  CS_NANO_TO_MASK |
		                  CS_TIMEOUT_MASK);

		scb->cs_state = CS_COMPUTABLE;
		ResetEvent(scb->cs_event_sem);
		ReleaseMutex(scb->cs_access_sem);
		return (OK);

	} else if ((scb->cs_mask & CS_IRPENDING_MASK) &&
	           (mask & CS_INTERRUPT_MASK)) {
		/*
		 * Then an interrupt is pending.  Pretend we just got it
		 */
		scb->cs_mask &= ~(CS_EDONE_MASK     |
		                  CS_IRCV_MASK      |
		                  CS_IRPENDING_MASK |
		                  CS_TO_MASK        |
		                  CS_ABORTED_MASK   |
		                  CS_INTERRUPT_MASK |
		                  CS_NANO_TO_MASK |
		                  CS_TIMEOUT_MASK);

		scb->cs_state = CS_COMPUTABLE;
		ResetEvent(scb->cs_event_sem);
		ReleaseMutex(scb->cs_access_sem);

		return (E_CS0008_INTERRUPTED);
	}

	scb->cs_state   = CS_EVENT_WAIT;
	scb->cs_mask   |= (mask & ~(CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
                                  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
				  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
				  CS_LOG_MASK));
	scb->cs_memory  = (mask &  (CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
                                  CS_LKEVENT_MASK | CS_LGEVENT_MASK | 
				  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
				  CS_TIMEOUT_MASK));

	ReleaseMutex(scb->cs_access_sem);

	/* mark the time we suspended */
	start_time = CS_checktime();

	while (TRUE)
	{
	    int eventcount = 1;
# ifdef xDEBUG
	    DWORD waitcount = Cs_desched_usec_sleep;
# else
	    DWORD waitcount = INFINITE;
# endif

	    /* the admin thread should also wait for LISTEN completion */
	    if ( scb->cs_self == CS_ADDER_ID )
	    {
                listen_to_event = 3;
		eventcount = 4;
	    }
            if ( gc_compl_key )
            {
                ME_tls_get( gc_compl_key, (PTR *)&asto, &status );
                if ( asto )
                {
                    eventarray[eventcount] = asto->event;
                    to_compl_event = eventcount;
                    eventcount++;
                }
            }

	    if ( to_cnt || (mask & CS_NANO_TO_MASK) )
		waitcount = to_cnt;

	    status = WaitForMultipleObjectsEx( eventcount,
					       eventarray,
					       FALSE,
	                                       waitcount,
	                                       TRUE);
	    /*
	    ** Go on our merry way if:
	    **    1) our event has been signaled,
	    **    2) the time to wait has expired,
	    **    3) the thread is COMPUTABLE
	    ** or 4) this is the admin thread and a request to connect
	    **       has been received
	    */
            WaitForSingleObject(scb->cs_access_sem, INFINITE);
	    if ( status == WAIT_OBJECT_0 ||
		 (status == WAIT_TIMEOUT && (mask & CS_TIMEOUT_MASK)) || 
		 (scb->cs_state == CS_COMPUTABLE) )
	    {
		ResetEvent(scb->cs_event_sem);
		break;
	    }
	    else if (status == WAIT_OBJECT_0 + 1 && scb->cs_self == CS_ADDER_ID)
	    {
		(*GCevent_func[LISTEN])();
		ResetEvent(scb->cs_event_sem);
		break;
	    }
	    else if (status == WAIT_OBJECT_0 + 2 && scb->cs_self == CS_ADDER_ID)
	    {
		/* SHUTDOWN event received */
		break;
	    }
	    else if (status == WAIT_OBJECT_0 + 3 && scb->cs_self == CS_ADDER_ID)
	    {
		(*GCevent_func[TIMEOUT])();
		ResetEvent(scb->cs_event_sem);
		break;
	    }
            else if (status == WAIT_OBJECT_0 + to_compl_event && asto != NULL)
            { 
                if (pfnCancelIo == NULL)
                {
                    HANDLE hDll = NULL;
                    /*
                    ** Get a handle to the kernel32 dll and get the proc 
                    ** address for CancelIo
                    */
                    if ((hDll = LoadLibrary( TEXT("kernel32.dll") )) != NULL)
                    {
                        pfnCancelIo = (BOOL (FAR WINAPI *)(HANDLE hFile))
                                        GetProcAddress( hDll, TEXT("CancelIo") );
                        FreeLibrary( hDll );
                    }
                }
                if (pfnCancelIo != NULL)
                    (*pfnCancelIo)(asto->fd);
		break;
	    }
            ReleaseMutex(scb->cs_access_sem);
        } /* loop exits holding scb->cs_access_sem */

	/*
	 * time passes, things get done.  Finally we resume, based on some
	 * thread posting the cs_event_sem.
	 */

	if (scb->cs_mask & CS_EDONE_MASK) 
	{
		rv = OK;
	} 
	else if (scb->cs_mask & CS_IRCV_MASK) 
	{
		rv = E_CS0008_INTERRUPTED;
	} 
	else if (status == WAIT_TIMEOUT) 
	{
		rv = E_CS0009_TIMEOUT;
	} 
	else if (scb->cs_mask & CS_ABORTED_MASK) 
	{
		rv = E_CS000F_REQUEST_ABORTED;
	} 
	else if (status == WAIT_ABANDONED) 
	{
		(*Cs_srv_block.cs_elog) (E_CS0019_INVALID_READY, 0, 0);
		rv = E_CS0019_INVALID_READY;
	} 
	else 
	{
		rv = OK;
	}

	scb->cs_state  = CS_COMPUTABLE;
	scb->cs_memory = 0;
	scb->cs_mask   &= ~(CS_EDONE_MASK     |
	                    CS_IRCV_MASK      |
	                    CS_TO_MASK        |
	                    CS_ABORTED_MASK   |
	                    CS_INTERRUPT_MASK |
	                    CS_NANO_TO_MASK   |
	                    CS_TIMEOUT_MASK);

	/*
	** Don't collect statistics for dbms task threads - they skew the
	** server statistics.
	*/

	end_time = CS_checktime();
	elapsed_ms = end_time - start_time;
	if ((mask & CS_LIOR_MASK) == CS_LIOR_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_lior_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_lior_time += elapsed_ms;
	}
	else if ((mask & CS_LIOW_MASK) == CS_LIOW_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_liow_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_liow_time += elapsed_ms;
	}
	else if (mask & CS_LOG_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_lg_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_lg_time += elapsed_ms;
	}
	else if ((mask & CS_DIOR_MASK) == CS_DIOR_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_dior_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_dior_time += elapsed_ms;
	}
	else if ((mask & CS_DIOW_MASK) == CS_DIOW_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_diow_waits++;
	    Cs_srv_block.cs_wtstatistics.cs_diow_time += elapsed_ms;
	}
	else if (scb->cs_thread_type == CS_USER_THREAD)
	{
	    if ((mask & CS_BIOR_MASK) == CS_BIOR_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_bior_waits++;
		Cs_srv_block.cs_wtstatistics.cs_bior_time += elapsed_ms;
	    }
	    else if ((mask & CS_BIOW_MASK) == CS_BIOW_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_biow_waits++;
		Cs_srv_block.cs_wtstatistics.cs_biow_time += elapsed_ms;
	    }
	    else if (mask & CS_LOCK_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_lk_waits++;
		Cs_srv_block.cs_wtstatistics.cs_lk_time += elapsed_ms;
	    }
	    else if (mask & CS_LKEVENT_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_lke_waits++;
		Cs_srv_block.cs_wtstatistics.cs_lke_time += elapsed_ms;
	    }
	    else if (mask & CS_LGEVENT_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_lge_waits++;
		Cs_srv_block.cs_wtstatistics.cs_lge_time += elapsed_ms;
	    }
	    else if (mask & CS_TIMEOUT_MASK)
		Cs_srv_block.cs_wtstatistics.cs_tm_waits++;
	}

	ReleaseMutex(scb->cs_access_sem);

	return (rv);
}

/*
** Name: CSresume() - Mark thread as done with event
**
** Description:
**      This routine is called to note that the session identified
**      by the parameter has completed some event.
**
** Inputs:
**      sid         thread id for which event has completed.
**                  Certain sid's with special meaning
**                  may cause other operations to happen.
**                  For instance, an sid of CS_ADDER_ID
**                  will cause a new thread to be added.
**
** Outputs:
**      none
**  Returns:
**      none
**  Exceptions:
**      none
**  History:
**	22-jan-2004 (somsa01)
**	    Watch for bogus resume issued on session in CS_CNDWAIT state;
**	    ignore the resume.
*/
VOID
CSresume(CS_SID sid)
{
    CS_SCB	*scb;

    scb = CSfind_scb (sid);

    if (scb)
    {
	WaitForSingleObject(scb->cs_access_sem, INFINITE);
	if (scb->cs_state != CS_CNDWAIT)
	{
	    scb->cs_mask |= CS_EDONE_MASK;
	    if (scb->cs_state == CS_EVENT_WAIT) 
	    {
		scb->cs_state = CS_COMPUTABLE;
		SetEvent(scb->cs_event_sem);
		if (scb->cs_memory & CS_DIO_MASK)
		    ++dio_resumes;
	    }
	}

	ReleaseMutex(scb->cs_access_sem);
    }
}

/****************************************************************************
**
** Name: CScp_resume() - Mark thread as done with event
**
** Description:
**      This routine is called to note that the thread identified
**      by the parameter has completed some event.
**
** Inputs:
**	cpid		- pointer to CS_CPID with
**	   .pid		- the indicated process
**	   .sid		- the indicated session
**	   .tid		- the indicated thread
**
** Outputs:
**      none
** Returns:
**      none
** Exceptions:
**      none
**
** History:
**	16-Nov-1998 (jenjo02)
**	    Prototype changed to pass CS_CPID* instead of PID, SID.
**	20-dec-1999 (somsa01)
**	    Changed the CS_CPID structure to also hold the thread's id.
**
****************************************************************************/
VOID
CScp_resume(CS_CPID    *cpid)
{
	HANDLE hEvent;
	char   szEventName[MAX_PATH];
	char   *ObjectPrefix;

	GVshobj(&ObjectPrefix);
    sprintf(szEventName, CS_EVENT_NAME_W2K, ObjectPrefix, cpid->tid);
	        
	if ((hEvent = OpenEvent(EVENT_MODIFY_STATE,
	                        FALSE,
	                        szEventName)) != NULL) {
		SetEvent(hEvent);
		CloseHandle(hEvent);
	}
	else
		CS_breakpoint();
}

/*{
** Name: IICSsuspend_for_AST()  - wait for AST
**
** Description:
**      This routine is called to suspend for AST completion from DLM
**
** Inputs:
**
** Outputs:
**	none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
** History:
**      07-Oct-2004 (somsa01)
**          Created.
*/
STATUS
IICSsuspend_for_AST(i4 mask, i4  to_cnt, PTR ecb)
{
}

/****************************************************************************
**
** Name: CScancelled    - Event has been canceled
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
**  ecb             Pointer to a system specific event
**                  control block.  Can be zero if not used.
**
** Outputs:
**      none
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
VOID
CScancelled(PTR ecb)
{
	CS_SCB *scb;

	if (Cs_srv_block.cs_state != CS_INITIALIZING) {
		CSget_scb(&scb);
		WaitForSingleObject(scb->cs_access_sem, INFINITE);
		scb->cs_mask &= ~(CS_EDONE_MASK     |
		                  CS_IRCV_MASK      |
		                  CS_TO_MASK        |
		                  CS_ABORTED_MASK   |
		                  CS_INTERRUPT_MASK |
		                  CS_NANO_TO_MASK |
		                  CS_TIMEOUT_MASK);
		ReleaseMutex(scb->cs_access_sem);
	}
}

/*
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
**  priority        Priority for new thread.  A value
**                  of zero (0) indicates that "normal"
**                  priority is to be used.  Ignored in OS/2
**
**  crb             A pointer to the connection
**                  request block for the thread.
**                  This is the block which was sent to
**                  request that a new thread be added.
**                  This is a buffer that was passed to
**                  (*cs_saddr)() and is provided to the
**                  scb allocator for transfer of
**                  interesting information to the newly
**                  created scb.
**
**  thread_type     Non-negative value by which caller can
**                  specify to the thread initiate code
**                  what the function of the thread is.  A
**                  type of zero indicates a normal user
**                  thread (this is the type value that CS
**                  uses when it wants to add a new user).
**                  The meaning of any(other values is
**                  coordinated between the add_thread
**                  caller and SCF.
**
**
** Outputs:
**      thread_id   optional pointer to CS_SID into which
**                  the new thread's SID will be returned.
**      error       Filled in with an OS error if appropriate
**
**  Returns:
**      OK, or
**      E_CS0002_SCB_ALLOC_FAIL -   Unable to allocate scb for new thread
**      E_CS0004_BAD_PARAMETER  -   Invalid value for priority
**      E_CS000E_SESSION_LIMIT  -   Too many threads have been requested
**
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	08-feb-1996 (canor01)
**	    Specify a particular thread to use a different startup routine
**	    so it can be tracked by the Microsoft profiler.
**      02-may-1997 (canor01)
**          If a thread is created with the CS_CLEANUP_MASK set in
**          its thread_type, transfer the mask to the cs_cs_mask.
**          These threads will be terminated and allowed to finish
**          processing before any other system threads exit.
**      31-oct-1996 (canor01)
**          Add additional error message if thread creation fails.
**      05-sep-1997 (canor01)
**          Do not initialize scb's cs_length field if the allocator has
**          already done so.
**	09-jan-1998 (canor01)
**	    Maintain cs_hwm_active when adding threads.
**      31-Mar-1998 (jenjo02)
**          Added *thread_id output parm to CSadd_thread() prototype.
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	19-sep-2000 (somsa01)
**	    The last cross-integration accidentally changed the name of
**	    one of CSadd_thread()'s parameters.
**	23-jan-2004 (somsa01)
**	    Count CS_MONITOR sessions in cs_user_sessions to be consistent
**	    with CSadd_thread().
**  18-dec-2006 (wonca01) BUG 117262
**      Use passed in parameter CL_SYS_ERR *error when reporting the 
**      OS error via cs_elog().
*/
STATUS
CSadd_thread(i4         priority,
             PTR        crb,
             i4         thread_type,
	     CS_SID     *session_id,
             CL_SYS_ERR *error)
{
	STATUS status;
	CS_SCB *scb;
	void   CS_prf_setup(), CS_setup();
	char   szEventName[MAX_PATH];
	SECURITY_ATTRIBUTES	sa;

	iimksec (&sa);

	if (error)
		error->errnum = 0;

	if (status = (*Cs_srv_block.cs_scballoc)(&scb, crb, 
				      thread_type & ~(CS_CLEANUP_MASK)))
		return (status);

	scb->cs_connect     = TMsecs();
	scb->cs_type        = CS_SCB_TYPE;
	scb->cs_owner       = (PTR)CS_IDENT;
        if ( scb->cs_length == 0 )
            scb->cs_length  = sizeof(CS_SCB);
	scb->cs_tag         = CS_TAG;
	scb->cs_mask        = 0;
	scb->cs_state       = CS_COMPUTABLE;
	scb->cs_self        = (CS_SID)scb;
	scb->cs_mode        = CS_INITIATE;
	scb->cs_thread_type = thread_type & ~(CS_CLEANUP_MASK);
	scb->cs_pid	    = GetCurrentProcessId();
        scb->cs_cs_mask = 0;
        if ( thread_type & CS_CLEANUP_MASK )
            scb->cs_cs_mask |= CS_CLEANUP_MASK;

	/*
	 * If collecting CPU stats for all dbms sessions, then turn on CPU
	 * statistic collecting.  Otherwise, cpu stats will only be collected
	 * for this thread if a CSaltr_session(CS_AS_CPUSTATS) call is made.
	 */

	if (Cs_srv_block.cs_mask & CS_CPUSTAT_MASK)
		scb->cs_mask |= CS_CPU_MASK;

	/*
	 * Now the thread is complete, place it on the appropriate queue
	 * and let it run via the normal course of events.
	 */

	CSp_semaphore(1, &Cs_known_list_sem);       /* exclusive */
	scb->cs_next = Cs_srv_block.cs_known_list;
	scb->cs_prev = Cs_srv_block.cs_known_list->cs_prev;
	Cs_srv_block.cs_known_list->cs_prev->cs_next = scb;
	Cs_srv_block.cs_known_list->cs_prev          = scb;

	++Cs_srv_block.cs_num_sessions;

	if (thread_type == CS_USER_THREAD || thread_type == CS_MONITOR)
	    Cs_srv_block.cs_user_sessions++;

	CSv_semaphore(&Cs_known_list_sem);

	scb->cs_thread_handle = CreateThread(&sa,
					     Cs_srv_block.cs_stksize,
#ifdef PRF_USR
	/* 
	** The Microsoft profiler will profile only the first thread
	** in a process or the first thread in a process that calls
	** a function (given the profiler on the command line).
	** Enable profiling on a particular thread type by compiling
	** with PRF_USR defined and the thread type specified below
	** (optionally with count of user sessions for a particular
	** user thread).
	*/
	(thread_type == CS_USER_THREAD) && (Cs_srv_block.cs_user_sessions == 40) ?
					     (LPTHREAD_START_ROUTINE) CS_prf_setup :    
#endif
					     (LPTHREAD_START_ROUTINE) CS_setup,
					     scb,
					     CREATE_SUSPENDED,
					     &scb->cs_thread_id);

	if (scb->cs_thread_handle) 
	{
		char	*ObjectPrefix;

		GVshobj(&ObjectPrefix);

                /* If wanted, return session_id to thread creator */
                if (session_id)
                    *session_id = scb->cs_self;

		if (thread_type == CS_GROUP_COMMIT)
			SetThreadPriority (scb->cs_thread_handle, 
				THREAD_PRIORITY_HIGHEST);
		else if (thread_type == CS_LOGWRITER)
			SetThreadPriority (scb->cs_thread_handle,
				THREAD_PRIORITY_HIGHEST);
		else if (thread_type == CS_FAST_COMMIT)
			SetThreadPriority (scb->cs_thread_handle,
				THREAD_PRIORITY_ABOVE_NORMAL);
		else if (thread_type == CS_EVENT)
			SetThreadPriority (scb->cs_thread_handle,
				THREAD_PRIORITY_ABOVE_NORMAL);
		else if (thread_type == CS_WRITE_BEHIND)
			SetThreadPriority (scb->cs_thread_handle,
				THREAD_PRIORITY_ABOVE_NORMAL);
		else if (thread_type == CS_READ_AHEAD)
			SetThreadPriority (scb->cs_thread_handle,
				THREAD_PRIORITY_LOWEST);

   	    sprintf(szEventName, CS_EVENT_NAME_W2K, ObjectPrefix,
				scb->cs_thread_id);


		scb->cs_event_sem = CreateEvent(&sa,
		                                FALSE,
		                                FALSE,
		                                szEventName);

		scb->cs_di_event = CreateEvent(&sa,FALSE,FALSE,NULL);

		scb->cs_access_sem = CreateMutex(&sa,
		                                 FALSE,
		                                 NULL);

		if ((scb->cs_access_sem == NULL) ||
		    (scb->cs_event_sem  == NULL)) {
			Cs_srv_block.cs_scbdealloc(scb);
			return E_CS0002_SCB_ALLOC_FAIL;
		}

		CS_scb_attach(scb);

		ResumeThread(scb->cs_thread_handle);

	} else {
		if (error)
			error->errnum = GetLastError();
		else 
		{
			CLEAR_ERR (error);
			SETWIN32ERR (error, status, ER_create);
		}
                (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, error, 0);
		status = E_CS000E_SESSION_LIMIT;
	}

	return (status);
}

/*
** Name: CSremove   - Cause a thread to be terminated
**
** Description:
**  This routine is used to cause the dispatcher to remove
**  a thread from competition for resources.  It works by
**  setting the next state for the thread to CS_TERMINATE
**  and, from there, letting "nature take its course" in having
**  each of the threads terminate themselves.  This call is intended
**  for use only by the monitor code.
**
**  Note that this routine does not evaporate a thread at any
**  point other than when it has returned.  The reason that this
**  decision was made is to allow a thread to clean up
**  after itself.  Randomly evaporating threads will inevitably
**  result in a trashed server because the thread which has been
**  evaporated will have been holding some resource.  The CS module,
**  as previously stated, is cooperative;  it cannot go clean up
**  a thread's resources since it does not know what/where they
**  are.
**
** Inputs:
**      sid                             The thread id to remove
**
** Outputs:
**      none
**  Returns:
**      OK
**      E_CS0004_BAD_PARAMETER  thread does not exist
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	13-feb-1997 (cohmi01)
**	    Move SetEvent() that was done when thread was computable to be
**	    done only when an otherwise unrecognized non-computable state
**	    occurs, otherwise future event-waits by the sid being removed
**	    will return prematurely.  (Bug 80740)
**	22-jan-2004 (somsa01)
**	    Cleanup CS_CNDWAIT handling, mutex it while changing its state.
*/
STATUS
CSremove(CS_SID sid)
{
    CS_SCB *scb;

    scb = CS_find_scb(sid);
    TRdisplay("CSremove(%d), scb=0x%x\n", sid, scb);
    if (scb == 0)
	return (E_CS0004_BAD_PARAMETER);

    WaitForSingleObject(scb->cs_access_sem, INFINITE);
    scb->cs_mask |= CS_DEAD_MASK;

    if (scb->cs_state != CS_COMPUTABLE)
    {
	/*
	** In this case, we need to figure out what needs to be
	** remedied in order to fix things up.  If the thread is
	** awaiting a semaphore, it is safe to return an error
	** saying that the request has been aborted. The same
	** should be true of general event wait.
	*/
	if (scb->cs_state == CS_UWAIT)
	{
	    scb->cs_state = CS_COMPUTABLE;
	    ResumeThread((HANDLE) scb->cs_thread_handle);
	}
	else if (scb->cs_state == CS_CNDWAIT)
	{
	    CS_CONDITION	*cp = scb->cs_cnd;

	    /* Remove from condition queue */
	    CSp_semaphore(1, cp->cnd_mtx);
	    cp->cnd_next->cnd_prev = cp->cnd_prev;
	    cp->cnd_prev->cnd_next = cp->cnd_next;
	    CSv_semaphore(cp->cnd_mtx);

	    scb->cs_mask |= CS_ABORTED_MASK;
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_cnd = NULL;
	    scb->cs_sync_obj = NULL;
	    SetEvent(scb->cs_event_sem);
	}
	else if (scb->cs_state == CS_MUTEX)
	{
	    scb->cs_state  = CS_COMPUTABLE;
	    scb->cs_mask |= CS_ABORTED_MASK;
	}
	else if (scb->cs_state == CS_EVENT_WAIT)
	{
	    scb->cs_mask |= CS_ABORTED_MASK;
	    if (scb->cs_mask & CS_INTERRUPT_MASK)
	    {
		scb->cs_state = CS_COMPUTABLE;
		SetEvent(scb->cs_event_sem);
	    }
	} 
	else
	{
	    /*
	    ** Fall-thru for unexpected or uninteresting cs_state value. Just
	    ** wake up the thread and let it figure it out
	    */
	    SetEvent(scb->cs_event_sem);  
	}
    }

    ReleaseMutex(scb->cs_access_sem);
    TRdisplay("Leaving CSremove\n");
    return(OK);
}

/****************************************************************************
**
** Name: CSstatistics   - Fetch statistics of current thread or entire server
**
** Description:
**  This routine is called to return the ``at this moment'' statistics
**  for this particular thread or for the entire server.  It returns a
**  structure which holds the statistics count at the time the call is made.
**
**  If the 'server_stats' parameter is zero, then statistics are returned
**  for the current thread.  If non-zero, then CSstatistics returns
**  the statistics gathered for the entire server.
**
**  The structure returned is a TIMERSTAT block, as defined in the <tm.h>
**  file in the CLF\TM component.   Thus, this calls looks very much
**  like a TMend() call.  There are two main differences.  The first is
**  that there is no equivalent to the TMstart() call.  This is done
**  implicitly at server and thread startup time.  Secondly, this routine
**  returns a TIMERSTAT block holding only the current statistics
**  count (TMend returns a TIMER block which holds three TIMERSTAT blocks
**  - one holding the starting statistics count (set at TMstart), one
**  holding the statistics count at TMend time, and one holding the
**  difference).
**
**  NOTE:  CPU statistics are not kept by default on a thread by thread
**  basis.  If CSstatistics is called for a thread not doing CPU statistics
**  then the stat_cpu field will be zero.  A caller wishing to collect CPU
**  times for a thread should first call CSaltr_session to start CPU
**  statistics collecting.  Subsequent CSstatistics calls will return the
**  CPU time used since that time.
**
** Inputs:
**  timer_block     The address of the timer block to fill
**  server_stats    Zero - return statistics for current session.
**                  Non-zero - return statistics for entire server.
**
** Outputs:
**      *timer_block                    Filled in with...
**          .stat_cpu                   the CPU time in milliseconds
**          .stat_dio                   count of disk i/o requests
**          .stat_bio                   count of communications requests
**  Returns:
**      OK
**      E_CS0004_BAD_PARAMETER  Invalil timer_block pointer passed.
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	24-apr-1997 (canor01)
**	    Updated to use GetProcessTimes() and GetThreadTimes()    
**	    for CPU statistics.
**	05-Feb-2001 (thaju02)
**	    Compute stat_cpu using 64-bit kernel and user times, to
**	    account for possible rollover into upper 32-bits for 
**	    kernel and user times. (B103883)
**
****************************************************************************/
STATUS
CSstatistics(TIMERSTAT *timer_block,
             i4        server_stats)
{
	CS_SCB *scb;
	FILETIME    CreationTime;
	FILETIME    ExitTime;    
	FILETIME    KernelTime;  
	FILETIME    UserTime;    
	PROCESS_MEMORY_COUNTERS pmc;
	bool        retval;    
	STATUS      status = OK;

	if ( pfnGetProcessMemoryInfo == NULL )
	{
	    HANDLE hDll;
	    if ((hDll = LoadLibrary (TEXT("psapi.dll"))) != NULL)
	    {
    		pfnGetProcessMemoryInfo = 
			               (BOOL (WINAPI *)( HANDLE Process, 
					PPROCESS_MEMORY_COUNTERS ppsmemCounters,
					DWORD cb)) 
					  GetProcAddress(hDll, 
						TEXT("GetProcessMemoryInfo"));
	    }
	}
        if (timer_block == 0)
            return (E_CS0004_BAD_PARAMETER);

        if (server_stats) 
	{
	    if ( pfnGetProcessMemoryInfo != NULL )
	    {
	        (*pfnGetProcessMemoryInfo)( GetCurrentProcess(), &pmc, 
					 sizeof(pmc) );
                timer_block->stat_pgflts = pmc.PageFaultCount;
	    }
	    else
	    {
                timer_block->stat_pgflts = 0;
	    }
            timer_block->stat_cpu = TMcpu();
            timer_block->stat_dio =
		Cs_srv_block.cs_wtstatistics.cs_dior_done +
		Cs_srv_block.cs_wtstatistics.cs_diow_done;
            timer_block->stat_bio =
		Cs_srv_block.cs_wtstatistics.cs_bior_done +
		Cs_srv_block.cs_wtstatistics.cs_biow_done;
        } 
	else 
	{
            CSget_scb(&scb);
            retval = GetThreadTimes( scb->cs_thread_handle,
                                      &CreationTime,
                                      &ExitTime,    
                                      &KernelTime,  
                                      &UserTime );  
            if ( retval == 0 )
            {
                status = GetLastError();
                timer_block->stat_cpu = 0;
            }
            else
            {   
		LARGE_INTEGER	kern_lint;
		LARGE_INTEGER	user_lint;
                /*
                ** Interval measured in 64-bit value of 100-nanoseconds
		** Compute using 64-bit values for kernel and user times. 
		** (B103883)
                */
		kern_lint.LowPart = KernelTime.dwLowDateTime;
		kern_lint.HighPart = KernelTime.dwHighDateTime;
		user_lint.LowPart = UserTime.dwLowDateTime;
		user_lint.HighPart = UserTime.dwHighDateTime;

                timer_block->stat_cpu = 
			(i4)((kern_lint.QuadPart + user_lint.QuadPart) 
				/ HUNDREDNANO_PER_MILLI);
            }
            timer_block->stat_dio = scb->cs_dior + scb->cs_diow;
            timer_block->stat_bio = scb->cs_bior + scb->cs_biow;
        }

        return (status);
}

/*
** Name: CSattn - Indicate async event to CS thread or server
**
** Description:
**      This routine is called to indicate that an unexpected asynchronous
**      event has occurred, and that the CS module should take the appropriate
**      action.  This is typically used in indicate that a user interrupt has
**      occurred, but may be used for other purposes.
**
** Inputs:
**  eid             Event identifier.  This identifies the event type for
**                  the server.
**  sid             thread id for which the event occurred.  In some cases,
**                  the event may be for the server, in which case this
**                  parameter should be zero (0).
**
** Outputs:
**  none
**
** Returns:
**  VOID
**
** Exceptions:
**  none
**
** Side Effects:
**  none
**
** History:
**      31-oct-1996 (canor01)
**          Call cs_attn to set thread's flags before waking the thread.
**      13-dec-1996 (canor01)
**          If a session is waiting on an Ingres condition, wake it up
**          for shutdown.
**      19-sep-1997 (canor01)
**          Make CSattn() call CSattn_scb(), which is a faster entry
**          point when we already have the scb.
**	22-jan-2004 (somsa01)
**	    Cleanup CS_CNDWAIT condition handling.
**	11-Sep-2006 (jonj)
**	    A FAIL return from CS_RCVR_EVENT (force-abort) instructs
**	    us to nothing else with the session's state or mask,
**	    just return.
**	    Delete extra (blocking?) WaitForSingleObject(cs_access_sem)
**	    as it's already held.
*/
VOID
CSattn(i4     eid,
       CS_SID sid)
{
    CS_SCB *scb = 0;

    if (sid)
        scb = CS_find_scb(sid);

    CSattn_scb( eid, scb );
    return;

}

static VOID
CSattn_scb( i4  eid, CS_SCB *scb )
{
    if (scb && (scb->cs_mode != CS_TERMINATE)) 
    {
	WaitForSingleObject(scb->cs_access_sem, INFINITE);
		
	if ((*Cs_srv_block.cs_attn) (eid, scb)) 
	{
	    if ( eid == CS_RCVR_EVENT )
	    {
		/* DMF will handle this - do nothing */
		ReleaseMutex(scb->cs_access_sem);
		return;
	    }

	    /* Unset the interrupt pending mask. */
	    scb->cs_mask &= ~CS_IRPENDING_MASK;
	}

	if ((scb->cs_state != CS_COMPUTABLE) &&
	    (scb->cs_mask & CS_INTERRUPT_MASK)) 
	{
	    scb->cs_mask &= ~CS_INTERRUPT_MASK;
	    scb->cs_mask |= CS_IRCV_MASK;
	    scb->cs_state = CS_COMPUTABLE;
	    SetEvent(scb->cs_event_sem);
	} 
	else if (scb->cs_state == CS_CNDWAIT)
	{
	    CS_CONDITION	 *cp = scb->cs_cnd;

	    /* remove from condition queue */
	    CSp_semaphore(1, cp->cnd_mtx);
	    cp->cnd_next->cnd_prev = cp->cnd_prev;
	    cp->cnd_prev->cnd_next = cp->cnd_next;
	    CSv_semaphore(cp->cnd_mtx);

	    scb->cs_mask |= CS_ABORTED_MASK;
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_cnd = NULL;
	    scb->cs_sync_obj = NULL;
	    SetEvent(scb->cs_event_sem);
	} 
	else 
	{
	    /*
	    ** DMF FORCE ABORT ISSUE if (scb->cs_state == CS_COMPUTABLE)
	    */
	    scb->cs_mask |= CS_IRPENDING_MASK;
	}

	ReleaseMutex(scb->cs_access_sem);
	ResumeThread((HANDLE) scb->cs_thread_handle);
    }

    /* else thread is being deleted */
}

/****************************************************************************
**
** Name: CSget_sid  - Get the current session id
**
** Description:
**  This routine returns the current session id.
**
** Inputs:
**
** Outputs:
**      *sidptr                             session id.
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	15-dec-1999 (somsa01 for jenjo02, part II)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**
****************************************************************************/
VOID
CSget_sid(CS_SID *sidptr)
{
    CS_SCB **ppSCB;

    if (sidptr)
    {
	ppSCB = TlsGetValue( TlsIdxSCB );
	*sidptr = (*ppSCB)->cs_self;
    }
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
**      VOID
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	16-Oct-1998 (jenjo02)
**	    created.
**	20-dec-1999 (somsa01)
**	    Added holding of the thread's id.
[@history_template@]...
*/
VOID
CSget_cpid(CS_CPID *cpid)
{
    if (cpid)
    {
	CSget_sid(&cpid->sid);
	if (cpid->sid == CS_ADDER_ID)
	    cpid->tid = ((CS_SCB *)&Cs_main_scb)->cs_thread_id;
	else if ((CS_SCB *)&cpid->sid)
	    cpid->tid = (*(CS_SCB *)cpid->sid).cs_thread_id;
	else
	    cpid->tid = 0;

	cpid->pid = Cs_srv_block.cs_pid;
    }
}

/****************************************************************************
**
** Name: CSget_scb  - Get the current scb
**
** Description:
**  This routine returns a pointer to the current scb.
**
** Inputs:
**
** Outputs:
**      *scbptr                          thread control block pointer.
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**    24-jun-95 (emmag)
**        Must check for the case where pSCB is NULL.
**
****************************************************************************/
VOID
CSget_scb(CS_SCB **scbptr)
{
    CS_SCB **ppSCB;
    CS_SCB *pSCB;

    if (scbptr) 
    {
	if ( TlsIdxSCB == 0 )
	{
	   *scbptr = (CS_SCB *) 0;
	}
	else
	{
	   ppSCB = TlsGetValue( TlsIdxSCB );
	   pSCB = *ppSCB;
	   if ((pSCB           == NULL) ||
		(pSCB->cs_state == CS_CLOSING) ||
		(pSCB->cs_state == CS_TERMINATING)) 
	   {
		*scbptr = (CS_SCB *) 0;
	   } 
	   else 
	   {
		*scbptr = pSCB;
	   }
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
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-oct-1999 (somsa01)
**          Created from UNIX equivalent.
*/
VOID
CSget_svrid(char *cs_name)
{
    STcopy( Cs_srv_block.cs_name, cs_name);
    return;
}

/****************************************************************************
**
** Name: CSfind_scb - Given a session thread id, return the session CB.
**
** Description:
**  This routine is called by one thread to find the session cb (CS_SCB)
**  of another thread.
**
**  Once the CB is returned to the caller the caller should make sure
**  they're not fiddling with a session that's about to evaporate the
**  data structures that are being accessed.
**
**  If the session ID does not correspond to an active thread then NULL
**  is returned.  It is up to the caller to verify that a non-NULL
**  session CB was returned.  It is not an internal CL error to pass in
**  an invalid session ID, but caller will be notified through a NULL
**  session CB.
**
** Inputs:
**  sid             Known thread id for which we want
**                  the corresponding session SCB.
** Outputs:
**  Returns:
**      CS_SCB          Pointer to session CB or NULL if an
**                  active session CB was not found.
**  Exceptions:
**      None
**
** Side Effects:
**  None
**
****************************************************************************/
CS_SCB *
CSfind_scb(CS_SID sid)
{
	return CS_find_scb(sid);
}

/******************************************************************************
**
** Name: CSfind_sid    - Return the SID of the given SCB.
**
** Description:
**	A trivial routine, but it keeps other components out of the CS_SCB.
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
******************************************************************************/
CS_SID 
CSfind_sid(CS_SCB *scb)
{
	return (scb->cs_self);
}

/****************************************************************************
**
** Name: CSset_scb  - Set the current scb (DEBUG ONLY)
**
** Description:
**      This routine is provided for module tests only.  It sets the current
**      thread to that provided.
**
** Inputs:
**      scb                             scb
**
** Outputs:
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
VOID
CSset_scb(CS_SCB *scb)
{
	CS_SCB **ppSCB;

	ppSCB = TlsGetValue( TlsIdxSCB );
	*ppSCB = scb;
}

/****************************************************************************
**
** Name: CSset_sid  - Set the current thread id (DEBUG ONLY)
**
** Description:
**      This routine is provided for module tests only.  It sets the current
**      thread to that provided.
**
** Inputs:
**      scb                             thread id.
**
** Outputs:
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
VOID
CSset_sid(CS_SCB *scb)
{
}

/****************************************************************************
**
** Name: CSms_thread_nap - put this thread to sleep for ms millisecs
**
****************************************************************************/
VOID
CSms_thread_nap(i4  ms)
{
	Sleep(ms);
}

/****************************************************************************
**
** Name: CSintr_ack - Acknowledge receipt of an interrupt
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
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
VOID
CSintr_ack()
{
	CS_SCB         *scb;

	CSget_scb(&scb);

	WaitForSingleObject(scb->cs_access_sem, INFINITE);
	scb->cs_mask &= ~(CS_IRPENDING_MASK);
	ReleaseMutex(scb->cs_access_sem);

}

/****************************************************************************
**
** Name: CS_addrcmp - Address comparision function for SPTREEs
**
****************************************************************************/

i4 
CS_addrcmp( PTR a, PTR b )
{
    if ( a == b )
        return 0;
    else
        return( a > b ) ? 1 : -1;
}


#ifdef  not_yet_implemented

/****************************************************************************
**
** Name: CSdefine_lrregion  - Define a Limited Resource Region
**
** Description:
**  This routine defines a limited resource region.
**  The region is defined in terms of number of concurrent threads
**  to be allowed to access the region.  Attempts to access the region
**  by more than this number of threads will result in suspension of the
**  new threads pending exit from the region by other threads.
**
** Inputs:
**      lrr_id        Identifier of region being defined
**      thread_count  Number of concurrent threads allowed
**
** Outputs:
**      None
**  Returns:
**      STATUS
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
STATUS
CSdefine_lrregion(i4  lrr_id,
                  i4  thread_count)
{
}

/****************************************************************************
**
** Name: CSenter_lrsection  - Enter Limited Resource Region
**
** Description:
**  This call controls access to a limited resource region.  An LRR is
**  is a concept which is completely under control of
**  the application.  The CSdefine_lrregion() call is used to define a
**  particular region in terms of the number of concurrent threads which
**  are allowed to access the region simultaneously.  Entering the region
**  causes the concurrent use count to increase if it is not already at
**  the defined maximum.  If it is already at the defined maximum, the
**  thread asking to be allowed in is placed on a waiting list and suspended
**  awaiting another thread's exit from the limited resource region.
**
**  At exit time, all threads which are awaiting this region are
**  reactivated.  This causes all of the to compete for the region on a
**  priority based scheme, thus preventing a single low priority thread
**  from monopolizing the entire server.
**
** Inputs:
**      lrr_id   Identifier of the controlled region.
**
** Outputs:
**      None
**  Returns:
**      Status
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
STATUS
CSenter_lrregion(i4  lrr_id)
{
}

/****************************************************************************
**
** Name: CSexit_lrregion    - Exit Limited Resource Region
**
** Description:
**  This routine is called when the underlying application exits
**  a limited resource region.  (See CSenter_lrregion() for explanation.)
**  If there are threads waiting this region, then they are restarted.
**
** Inputs:
**      lrr_id                          Identifier for Limited Resource Region
**
** Outputs:
**  Returns:
**      VOID
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
VOID
CSexit_lrregion(i4  lrr_id)
{
}

/****************************************************************************
**
** Name: CSawait_lrregion   - Await an exit from an LRR
**
** Description:
**      This routine is called when an application needs more resources
**      then are currently available in a limited resource region, even
**      though the usage of the region is at or below the defined limits.
**      This routine will cause the thread to be suspended awaiting another
**      thread's exit from the LRR.  If no threads are currently in the
**      region, an error is returned.
**
** Inputs:
**      lrr_id                          Identifier for the desired region.
**
** Outputs:
**      None
**  Returns:
**      STATUS
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
****************************************************************************/
STATUS
CSawait_lrregion(i4  lrr_id)
{
}
#endif

/****************************************************************************
**
** Name: CSaltr_session - Alter CS characteristics of session.
**
** Description:
**  This routine is called to alter some characteristic of a
**  particular server thread.
**
**  If 'session_id' is specified (non-zero) then that indicates the
**  session to alter.  If 'session_id' is zero, then the current session
**  (the one from which the call is made) is altered.
**
**  Session Alter Options include:
**
**      CS_AS_CPUSTATS - Turn CPU statistics collecting ON/OFF for the
**          specified session.
**
**          Item points to a value of type 'i4 '.
**
**          If the value pointed at by the parameter 'item'
**          (*(i4  *)item) is zero, CS will no longer collect CPU
**          statistics for the specified session, and any calls to
**          CSstatistics by that session will return a cpu time of zero.
**          If the value pointed at by the parameter 'item' is non-zero,
**          then CS will begin (continue) to keep track of cpu time
**          accumulated by that session.
**
**          If the CS_AS_CPUSTATS request is to turn on CPU collecting
**          and this option is already enabled for this thread, then
**          the return value E_CS001F_CPUSTATS_WASSET is returned.
**
**
** Inputs:
**  session_id  - session to alter (zero indicates current session)
**  option      - alter option - one of following:
**          CS_AS_CPUSTATS - turn ON/OFF collecting of CPU stats.
**  item        - pointer to value used to alter session characteristic.
**            Meaning is specific to each alter option.
**
** Outputs:
**      None
**  Returns:
**      OK
**      E_CS0004_BAD_PARAMETER  Invalid parameter passed.
**      E_CS001F_CPUSTATS_WASSET    Cpu statistics already being collected.
**  Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	12-jun-2000 (rigka01) bug#99542
**	    Add code to enable "set session with priority=x" for Windows NT 
**	15-jul-2008 (smeke01) b120487
**	    Added missing ReleaseMutex() to CS_AS_PRIORITY case.
**
****************************************************************************/
STATUS
CSaltr_session(CS_SID session_id,
               i4  option,
               PTR item)
{
	CS_SCB *scb;

	if (session_id)
		scb = CS_find_scb(session_id);
	else
		CSget_scb(&scb);

	if (scb == 0)
		return (E_CS0004_BAD_PARAMETER);

	switch (option) {
	case CS_AS_CPUSTATS:
#ifdef xGET_CPUTIME
		WaitForSingleObject(scb->cs_access_sem, INFINITE);
		if (item && (*(i4  *) item)) {
			if ((scb->cs_mask & CS_CPU_MASK) == 0) {
				scb->cs_mask |= CS_CPU_MASK;
			} else {
				ReleaseMutex(scb->cs_access_sem);
				return (E_CS001F_CPUSTATS_WASSE \);
			}
		} else {
			scb->cs_mask &= ~(CS_CPU_MASK);
			scb->cs_cputime = 0;
		}
		ReleaseMutex(scb->cs_access_sem);
#endif				/* xGET_CPUTIME */
		break;

	case CS_AS_PRIORITY:
	if (item && (*(i4 *)item))
	  {
		i4 priority= *(i4 *)item;
		/* 
		** Setting session priority.
		*/
		if (priority >= CS_LIM_PRIORITY)
		{
			return(E_CS0004_BAD_PARAMETER);
		}
		else if (priority > CS_MAXPRIORITY)
		{
			priority = CS_MAXPRIORITY;
		}
		else if (priority == 0)
		{
			priority = CS_DEFPRIORITY;
		}
		else if (priority < CS_MINPRIORITY)
		{
			return(E_CS0004_BAD_PARAMETER);
		}
		WaitForSingleObject(scb->cs_access_sem, INFINITE);
		scb->cs_priority = priority;
		switch (priority)
		{
		  case CS_MAXPRIORITY:
                    SetThreadPriority (scb->cs_thread_handle,
                         THREAD_PRIORITY_HIGHEST);
		    break;
		  case CS_MINPRIORITY:
                    SetThreadPriority (scb->cs_thread_handle,
                         THREAD_PRIORITY_LOWEST);
		    break;
		  case CS_DEFPRIORITY:
                    SetThreadPriority (scb->cs_thread_handle,
                         THREAD_PRIORITY_NORMAL);
		    break;
		  default:
                    if ((priority > CS_MINPRIORITY) && (priority < CS_DEFPRIORITY))
		    {
                      SetThreadPriority (scb->cs_thread_handle,
                         THREAD_PRIORITY_BELOW_NORMAL);
		    }
                    else /* between norm and max priority */
		    {
                      SetThreadPriority (scb->cs_thread_handle,
                         THREAD_PRIORITY_ABOVE_NORMAL);
		    }
		    break;
		}
		ReleaseMutex(scb->cs_access_sem);
	  }
	  break;
	default:
		return (E_CS0004_BAD_PARAMETER);
	}

	return (OK);
}

/****************************************************************************
**
** Name: CSdump_statistics  - Dump CS statistics to Log File.
**
** Description:
**  This call is used to dump the statistics collected by the CS system.
**  It is intended to be used for performance monitoring and tuning.
**
**  Since there are no external requirements for what server statistics
**  are gathered by CS, the amount of information dumped by this routine
**  is totally dependent on what particular statistics are kept by this
**  version of CS.  Therefore, no mainline code should make any
**  assumptions about the information (if any) dumped by this routine.
**
**  This function will format text into a memory buffer and then use a
**  caller provided function to dispose of the buffer contents.  The
**  caller can pass in "TRdisplay" as the print function in order to
**  set the output to the server trace file.
**
**  The print function is called like:
**
**      (*fcn)(buffer)
**
**  where buffer will be a null-terminated character string.
**
** Inputs:
**  print_fcn   - function to call to print the statistics information.
**
** Outputs:
**      None
**  Returns:
**      Status
**  Exceptions:
**      none
**
** Side Effects:
**  The print function is called many times to output character buffers.
**  Whatever side effects the caller-supplied print_fcn has.
**
** History:
**	10-jan-2000 (somsa01)
**	    Added more statistics.
**
****************************************************************************/
VOID
CSdump_statistics(STATUS(*print_fcn)(char *, ...))
{
	char            buffer[150];

	sprintf(buffer, "========================================================\n");
	print_fcn(buffer);
	sprintf(buffer, "SERVER STATISTICS:\n====== ==========\n");
	print_fcn(buffer);
	sprintf(buffer, "---- -----------\n");
	print_fcn(buffer);
	sprintf(buffer, "          Requests  Wait State  Avg. Wait  Zero Wait  Wait Time\n");
	print_fcn(buffer);
	sprintf(buffer, "          --------  ----------  ---------  ---------  ---------\n");
	print_fcn(buffer);

	sprintf(buffer,
		"    BIOR:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_bior_done,
	        Cs_srv_block.cs_wtstatistics.cs_bior_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_bior_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_bior_time /
	          Cs_srv_block.cs_wtstatistics.cs_bior_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_bior_done -
	        Cs_srv_block.cs_wtstatistics.cs_bior_waits,
	        Cs_srv_block.cs_wtstatistics.cs_bior_time);
	print_fcn(buffer);

	sprintf(buffer,
		"    BIOW:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_biow_done,
	        Cs_srv_block.cs_wtstatistics.cs_biow_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_biow_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_biow_time /
	          Cs_srv_block.cs_wtstatistics.cs_biow_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_biow_done -
	        Cs_srv_block.cs_wtstatistics.cs_biow_waits,
	        Cs_srv_block.cs_wtstatistics.cs_biow_time);
	print_fcn(buffer);

	sprintf(buffer,
		"    DIOR:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_dior_done,
	        Cs_srv_block.cs_wtstatistics.cs_dior_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_dior_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_dior_time /
	          Cs_srv_block.cs_wtstatistics.cs_dior_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_dior_done -
	        Cs_srv_block.cs_wtstatistics.cs_dior_waits,
	        Cs_srv_block.cs_wtstatistics.cs_dior_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    DIOW:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_diow_done,
	        Cs_srv_block.cs_wtstatistics.cs_diow_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_diow_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_diow_time /
	          Cs_srv_block.cs_wtstatistics.cs_diow_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_diow_done -
	        Cs_srv_block.cs_wtstatistics.cs_diow_waits,
	        Cs_srv_block.cs_wtstatistics.cs_diow_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    LIOR:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_lior_done,
	        Cs_srv_block.cs_wtstatistics.cs_lior_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_lior_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_lior_time /
	          Cs_srv_block.cs_wtstatistics.cs_lior_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_lior_done -
	        Cs_srv_block.cs_wtstatistics.cs_lior_waits,
	        Cs_srv_block.cs_wtstatistics.cs_lior_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    LIOW:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_liow_done,
	        Cs_srv_block.cs_wtstatistics.cs_liow_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_liow_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_liow_time /
	          Cs_srv_block.cs_wtstatistics.cs_liow_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_liow_done -
	        Cs_srv_block.cs_wtstatistics.cs_liow_waits,
	        Cs_srv_block.cs_wtstatistics.cs_liow_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    Log:     %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_lg_done,
	        Cs_srv_block.cs_wtstatistics.cs_lg_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_lg_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_lg_time /
	          Cs_srv_block.cs_wtstatistics.cs_lg_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_lg_done -
	        Cs_srv_block.cs_wtstatistics.cs_lg_waits,
	        Cs_srv_block.cs_wtstatistics.cs_lg_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    LGevent: %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_lge_done,
	        Cs_srv_block.cs_wtstatistics.cs_lge_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_lge_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_lge_time /
	          Cs_srv_block.cs_wtstatistics.cs_lge_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_lge_done -
	        Cs_srv_block.cs_wtstatistics.cs_lge_waits,
	        Cs_srv_block.cs_wtstatistics.cs_lge_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    Lock:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_lk_done,
	        Cs_srv_block.cs_wtstatistics.cs_lk_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_lk_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_lk_time /
	          Cs_srv_block.cs_wtstatistics.cs_lk_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_lk_done -
	        Cs_srv_block.cs_wtstatistics.cs_lk_waits,
	        Cs_srv_block.cs_wtstatistics.cs_lk_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    LKevent: %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_lke_done,
	        Cs_srv_block.cs_wtstatistics.cs_lke_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_lke_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_lke_time /
	          Cs_srv_block.cs_wtstatistics.cs_lke_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_lke_done -
	        Cs_srv_block.cs_wtstatistics.cs_lke_waits,
	        Cs_srv_block.cs_wtstatistics.cs_lke_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    Time:    %8d.   %8d.  %8d.  %8d.  %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_tm_done,
	        Cs_srv_block.cs_wtstatistics.cs_tm_waits,
	        (Cs_srv_block.cs_wtstatistics.cs_tm_waits ?
	         (Cs_srv_block.cs_wtstatistics.cs_tm_time /
	          Cs_srv_block.cs_wtstatistics.cs_tm_waits) : 0),
	        Cs_srv_block.cs_wtstatistics.cs_tm_done -
	        Cs_srv_block.cs_wtstatistics.cs_tm_waits,
	        Cs_srv_block.cs_wtstatistics.cs_tm_time);
	print_fcn(buffer);

	sprintf(buffer,
	        "    BIO: %8d., DIO: %8d., Log: %8d., Lock: %8d., Semaphore: %8d.\n",
	        Cs_srv_block.cs_wtstatistics.cs_bio_idle,
	        Cs_srv_block.cs_wtstatistics.cs_dio_idle,
	        Cs_srv_block.cs_wtstatistics.cs_lg_idle,
	        Cs_srv_block.cs_wtstatistics.cs_lk_idle,
	        Cs_srv_block.cs_wtstatistics.cs_tm_idle);
	print_fcn(buffer);

	sprintf(buffer, "---- -----------\n");
	print_fcn(buffer);
	sprintf(buffer, "========================================================\n");
	print_fcn(buffer);

}

bool
CS_is_mt()
{
    return ( TRUE );
}

void
CSswitch()
{
    SleepEx(0,TRUE);
}

/* This seems a bit excessive, calling SleepEx a brazillion times during
** query execution.  FIXME should be able to do a ticker thread a la
** unix csmt, and call SleepEx only occasionally.
*/
void
CScancelCheck(CS_SID sid)
{
    if (sid != 0 && sid != CS_ADDER_ID)
	SleepEx(0,TRUE);	/* Same as CSswitch, for now */
}

static i4 
CS_usercnt()
{
    int count;

    CSp_semaphore(1, &Cs_known_list_sem);
    count = Cs_srv_block.cs_user_sessions;
    CSv_semaphore(&Cs_known_list_sem);

    return (count);
}

VOID
CSresume_from_AST(CS_SID sid)
{
    CSresume(sid);
}

i4
CSadjust_counter( i4 *pcounter, i4 adjustment )
{
    i4		oldvalue, newvalue;

    do
    {
	oldvalue = *pcounter;
	newvalue = oldvalue + adjustment;
    } while ( CScas4( pcounter, oldvalue, newvalue ) );

    return newvalue;
}

/*
** Name: CSadjust_i8counter() - Thread-safe counter adjustment.
**
** Description:
**	This routine provides a means to accurately adjust an i8 counter
**	which may possibly be undergoing simultaneous adjustment in
**	another session, or in a Signal Handler, or AST, without
**	protecting the counter with a mutex (if possible)
**
**	This is done, by taking advantage of the atomic compare and
**	swap routine.   It will call an intrinsic function available
**      in Visual Studio compiler which will increment the values.
**	
**	NOTE: This function is defined out to a compiler built-in in
**	      csnormal.h on platforms which use GCC
**
** Inputs:
**	pcounter	- Address of an i8 counter.
**	adjustment	- Amount to adjust by.
**
** Outputs:
**	None
**
** Returns:
** 	Value counter assumed after callers update was applied.
**
** History:
**	18-May-2010 (drivi01)
**		Created.
*/
i8
CSadjust_i8counter(i8 *pcounter, i8 adjustment)
{
	i8 oldvalue;
	do
	{
		oldvalue = *pcounter;
	}
	while(_InterlockedCompareExchange64(pcounter, (oldvalue + adjustment), oldvalue) != oldvalue);

	return (*pcounter);
}

