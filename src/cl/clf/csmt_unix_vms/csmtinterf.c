/*
** Copyright (c) 1987, 2008 Ingres Corporation
**
*/
/* NO_OPTIM = rs4_us5 ris_u64 r64_us5 */
# include <compat.h>
# ifdef OS_THREADS_USED
# include <errno.h>
# include <gl.h>
# include <st.h>
# include <clconfig.h>
# include <systypes.h>
# include <clsigs.h>
# include <iicommon.h>
# include <fdset.h>
# include <cs.h>
# include <er.h>
# include <ex.h>
# include <id.h>
# include <me.h>
# include <nm.h>
# include <pc.h>
# include <tm.h>
# include <tr.h>
# include <nm.h>
# include <me.h>
# include <si.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>
# include <mu.h>
# include <st.h>
# include <gc.h>

# include <csinternal.h>
# include <csev.h>
# include <cssminfo.h>
# include <rusage.h>
# include <clpoll.h>
# include <machconf.h>
# include <pwd.h>
# include <diracc.h>
# include <handy.h>
# include <meprivate.h>
#if defined(VMS)
# include <descrip.h>
# include <efndef.h>
# include <iledef.h>
# include <iosbdef.h>
# include <jpidef.h>
# include <lnmdef.h>
# include <starlet.h>
# include <queuemgmt.h>
# include <assert.h>
# include <builtins.h>
#endif

# include <lk.h>

#if defined(sparc_sol)
#include <sys/procfs.h>
#endif

#if defined(LNX)
#include <linux/unistd.h>
#endif

# include "csmtmgmt.h"
# include "csmtlocal.h"
# include "csmtsampler.h"

/*
NO_OPTIM = rs4_us5 i64_aix
*/

GLOBALREF       CS_SERV_INFO    *Cs_svinfo;

/**
**
**  Name: CSINTERFACE.C - Control System External Interface
**
**  Description:
**	This module contains those routines which provide the user interface to
**	system dispatching services to the DBMS (only, at the moment).
**
**	    CSinitiate() - Specify startup characteristics, and allow control
**			system to initialize itself.
**	    CSterminate() - Cancel all threads and prepare for shutdown.
**	    CSalter() - Alter some/all of the system characteristics specified
**			by CSinitiate().
**	    CSdispatch() - Have dispatcher take over operation of the system.
**	    CSremove() - Remove a thread from the system (forcibly).
**	    CSsuspend() - Suspend a thread pending completion of an operation
**	    CSresume()	- Resume a suspended thread
**	    CScancelled() - Inform the CS module that an event for which
**			    a thread suspended will never occur
**	    CSadd_thread() - Add a thread to the server.
**	    CSstatistics() - Obtain runtime statistics for the current thread
**	    CSattn() - Inform the dispatcher that an unusual event has occurred
**	    CSget_sid() - Obtain thread id for the current thread
**	    CSget_scb() - Obtain pointer to current thread control block
**          CSfind_scb() - Obtain pointer to external thread control block
**	    CSmonitor() - Perform CL level monitor functions.
**	    CSintr_ack() - Clear CS interruupt status
**	    CSdefine_lrregion() - Define the Limited Resource Region (meaning is
**				application dependent)
**	    CSenter_lrregion() - Enter resource limited region.
**	    CSexit_lrregion() - Exit resource limited region.
**	    CSawait_lrregion() - Await an exit from a LRR
**	    CSdump_statistics() - Dump CS statistics to Log File.
**	    CSaccept_connect() - Returns TRUE if the server can accept another
**				connection request, otherwise it returns FALSE.
**	    CSswitch()	- poll for thread switch
**	    CSget_cpid() - Obtain cross-process thread identity.
**
**
**  History:
**	27-Oct-1986 (fred)
**	    Created.
**	15-jan-1987 (fred)
**	    Added command line parsing and passing info block to server startup.
**	16-jul-1987 (fred)
**	    Added GCF support
**	11-Nov-1987 (fred)
**	    Add code for semaphore statistics collection & reporting
**	11-Nov-1987 (fred)
**	    Add code for RLR's -- resource limited regions.
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
**	20-JAN-89 (eric)
**	    Added CS_accept_connect() for GCA rollover support.
**	06-feb-89 (mikem)
**	    LGK_initialize() takes a CL_ERR_DESC now.
**	23-Feb-89 (fredv)
**	    Use clsigs.h instead of signal.h
**	12-jun-89 (rogerk)
**	    Added cross process semaphore support.  Added cs_pid to server
**	    control block and session control blocks.  Added CSi_semaphore
**	    routine.  Added cross process support in CSp and CSv_seaphore.
**	    Added SOLE_CACHE, CACHE_NAME, SHARED_CACHE, DBCACHE_SIZE,
**	    TBLCACHE_SIZE, and RULE_DEPTH options.
**      29-jun-1989 (rogerk)
**          Fixed startup parameters for "shared_cache" option.
**	 6-jul-1989 (rogerk)
**	    In CSdispatch, moved place where server state is set to CS_IDLING
**	    down to after the server startup routines are run.
**	    Removed 'ifdef NOTYET' code for cross-process semaphores.
**	24-jul-1989 (rogerk)
**	    Initialize cs_ef_mask field in CSadd_thread.
**       7-Aug-1989 (FPang)
**          Put up SIGPIPE exception handler in CSdispatch for servers
**          that don't use di (i.e. STAR).
**	31-Oct-1989 (anton)
**	    Add psedo-quantum support
**	 3-Nov-1989 (anton)
**	    sort option list - quantum flag fix
**	6-Dec-1989 (anton)
**	    Check Cs_incomp flag for illegal semaphore use.
**	    added MINSWITCH
**	22-Jan-1990 (fredp)
**	    Integrated the following change: 23-Aug-89 (GordonW)
**	    Added close(fd) to close off pipe file.
**	30-Jan-1990 (anton)
**	    Allow subsecond suspend
**     	    Fix idle job CPU spin in semaphore cases
**	26-Feb-1990 (anton)
**	    Added special name server registration stuff
**	    Added 'noflatten' and 'opf.memory'
**	    Changed 'nopublic' to default and added 'define'
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
**	02-apr-1990 (mikem)
**	    If the state is CS_INITIALIZING then this is being called
**	    from a non-dbms server (ie. the rcp, acp, createdb, ...).
**	    We never want to call CSsuspend() from CSp_semaphore in this case, 
**	    so the code will spin on the semaphore instead.
**	13-Apr-1990 (kimman)
**	    Adding ds3_ulx specific information.
**	31-may-90 (blaise)
**	    Integrated changes from termcl:
**		Changes to make CSp_semaphore return an error if a multiprocess
**		semaphore is held by a process which has exited (and so will
**		never be granted). Add new routine CS_sem_owner_dead(), and
**		call it to check whether the process owner still exists (after
**		spinning for some amount of time).
**	4-june-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Add large amounts of machnie-specific code;
**		Fix bug in CSmonitor routine affecting machines with a session
**		id greater than a 32-bit signed number - replace calls to
**		CVahxl with new function CVuahxl to handle this;
**		Add error message for a failed iiCLpoll call;
**		Add error message when session limit is hit;
**		Modify quantum time slicing.
**	08-Aug-90 (GordonW)
**		TERMCL changes, change # 20515.
** 	04-sep-90 (jkb) 
**		Add sqs_ptx to the list of defines for CS_sqs_pushf
**	01-oct-90 (chsieh)
** 	        Added bu2_us5 and bu3_us5 to #ifdef's used by bul_us5. 
**      01-oct-90   (chsieh)
**              Change the logic in CSp_semaphore. 
**              Cut down the max loop count to 100, and do a iiCLpoll for 1 
**              mili second if the loop count exceeds 100.
**
**              With this change the performance improved alot on BULL DPX2/210.
**	03-Oct-90 (anton)
**	    Adding reentrant CL support
**	    Adding CS_CONDITION support
**	07-Dec-90 (anton)
**	    Correcting cpu measurement code
**	    Bugs 20254, 32677, 35024
**	14-dec-1990 (mikem)
**	    Initialize Cs_numprocessors with the number of processors on the
**	    machine, will be used to determine whether or not to spin in 
**	    the CSp_semaphore() routine.
**	14-dec-90 (mikem)
**	    Do not spin on cross process semaphores when running on a single
**	    processor machine.  Call II_nap() instead.
**	18-dec-90 (teresa)
**		added the following RDF startup flags:
**		    rdf.max_tbls, rdf.memory, rdf.tbl_cols, rdf.tbl_idxs
**	07-Jan-90 (anton)
**	    Default is not to set II_DBMS_SERVER
**	    Fix server name copy length
**	    Clean up CS_CONDITION support
**      10-dec-90 (neil)
**          Alerters: Added CSfind_scb.
**	08-jan-91 (ralph)
**	   Added the following entries to CS_ARGTAB:
**		opf.cpufactor	  - CSO_OPCPUFACTOR
**		opf.timeoutfactor - CSO_OPTIMEOUT
**		opf.repeatfactor  - CSO_OPREPEAT
**		opf_sortmax	  - CSO_OPSORTMAX
**		dmf.scanfactor	  - CSO_DMSCANFACTOR
**		qef.sort_size	  - CSO_QESORTSIZE
**		events		  - CSO_EVENTS
**      21-jan-1991 (seputis)
**          Added OPF startup flags, EXACTKEY, RANGEKEY, NONKEY,
**              ACTIVE, AGGREGATE_FLATTEN, COMPLETE, FLATTEN
**      12-feb-1991 (ralph)
**          Added iirundbms startup flags, [NO]ECHO and [NO]SPAWN
**	12-feb-1991 (mikem)
**	    On systems which support it, made the debug option print a hex
**	    stack trace of the thread (called "CS_dump_stack()").
**	11-mar-1991 (gautam)
**	    Changed CS_set_server_connect to pass the cs_name alone
**	    for UNIX domain socket support
**	09-apr-1991 (seng)
**	    Added CS information for RS/6000 (ris_us5).
**	15-apr-1991 (vijay)
**	    Added SIGDANGER handling for ris_us5. We need to give an error
**	    message about swap space going low.
**	30-may-1991 (bryanp)
**	    B37501: Call II_nap, rather than CSsuspend, if CSp_semaphore() spins
**	    too long on a CS_SEM_MULTI-type semaphore.
**      25-jul-1991 (fpang)
**          Added -server_class star, and -max_ldb_connections, the former
**          identifies the server as a star server, and the latter will
**          increase max file descriptor limit. (STAR server only).
**	29-jul-1991 (bryanp)
**	    B38197: Fix thread hang when interrupt is followed by quantum switch
**	25-sep-1991 (mikem) integrated following change: 14-aug-1991 (ralph)
**	    Use CSO_RANGEKEY instead of CSO_EXACTKEY for -opf.rangekey
**          CSmonitor() - Drive CSattn() when IIMONITOR REMOVE
**	25-sep-1991 (mikem) integrated following change: 19-aug-1991 (rogerk)
**	    Changed spin lock in cross process semaphore code to use a
**	    "snooping" spin lock algorithm.
**	15-oct-1991 (mikem)
**	    6.4->6.5 merge (see above) and 80 column code formatting changes.
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**      12-mar-1992 (jnash)
**	    If DMF cache locking ifdef's set, check for process
**          running as ROOT in CSinitiate, and if so setuid
**	    to Ingres.  Also, include dmf.lock_cache in CS_ARGTAB.
**	06-apr-1992 (mikem)
**        SIR #43445
**          Changed the looping properties of CSp_semaphore() when operating
**          on a cross process semaphore.  The code now loops Cs_max_sem_loops
**          (defaults to 200) loops before going into II_yield_cpu_time_slice().
**          Changed the macro "II_yield_cpu_time_slice()" into a procedure now
**          located in handy!iinap.c.
**	02-jul-92 (swm)
**	    Added entries for dra_us5 (a 486 running SVR4).
**	28-jul-92 (fpang)
**	    Added support for new rdf startup flags, rdf.ddbs, rdf.avg_ldbs,
**	    rdf.cluster_nodes, rdf.netcost, rdf.nodecost. See Teresa's SYBIL RDF
**	    Design Changes for more details.
**	6-Jul-92 (daveb)
**	    Maintain SPTREEs of scb's, semaphores, and conditions, for
**	    use by MO methods.  Implement CSr_semaphore and
**	    CSn_semaphore, and sketch CSa_semaphore and CSd_semaphore.
**	9-Jul-92 (daveb)
**	    Sem_tree won't work, use CS_sem_attach scheme instead.
**	10-sep-92 (bonobo)
**	    Fixed a typo.
**	23-sep-1992 (bryanp)
**	    LGK_initialize no longer needs to be called from CSinitiate().
**      18-sep-92 (ed)
**          bug 46521 - opf.active was not being set
**	1-oct-92 (daveb)
**	    Set up MU semaphores to use CS sems when we CSinitiate.
**	5-nov-1992 (bryanp)
**	    Add new LG/LK server startup parameters. Also, CSinitiate now just
**	    quietly returns OK if called twice.
**	4-Dec-1992 (daveb)
**	    prototyped CScnd_name.  Give writable things a set-method.
**	    Check and log scribble-check failures in CSr_semaphore.
**	    Attach/detach sems iff the tree is there to use.
**	13-dec-92 (mikem)
**	    su4_us5 port.  Changed ifdef of maximum working set option to
**	    look for both xCL_034_GETRLIMIT_EXISTS and RLIMIT_RSS.  On su4_us5
**	    xCL_034_GETRLIMIT_EXISTS is true but no RLIMIT_RSS option exists.
**	18-jan-1993 (bryanp)
**	    Add support for -CPTIMER startup parameter.
**	1-feb-1993 (markg)
**	    Add support for the sxf.memory startup parameter.
**	15-mar-1993 (ralph)
**	    Added new FIPS-related server startup parameters, including:
**	    CSO_CSRDIRECT, CSO_CSRDEFER, CSO_FLATOPT, CSO_FLATNONE,
**	    CSO_FLATNAGG, CSO_FLATNSING
**	26-apr-1993 (bryanp)
**	    Removed code which attempts to set Cs_resumed when cs_state is
**	    CS_IDLING. This, I believe, was a confusion which dates from the
**	    original porting of the VMS CL to Unix. In this port, I think that
**	    we just replaced every SYS$WAKE call with Cs_resumed = 1, but in
**	    the VMS CL there are two different flavors of SYS$WAKE: one which
**	    interacts with the SYS$HIBER call in CSsuspend, and one which
**	    interacts with the SYS$HIBER call in CS_setup. The effect of all the
**	    extra assignments to Cs_resumed is just that Cs_resumed was getting
**	    set to 1 when it should have been 0. The result is that the first
**	    CSsuspend() call made after CSterminate() has reset the system back
**	    to CS_INITIALIZING state was immediately returning (because
**	    Cs_resumed was stuck high) instead of properly waiting for its
**	    matching CSresume(). In the Unix CL, our idle thread wakes up
**	    automatically when the ready mask indicates that threads are
**	    runnable; we do not have the concept, as VMS does, of AST code
**	    needing to awaken the main thread of control, so the SYS$WAKE calls
**	    which are performed when the system is idling should have been
**	    removed, not turned into Cs_resumed = 1 statements.
**	26-apr-1993 (fredv)
**	    Moved <clconfig.h>, <systypes.h>, <clsigs.h> up to avoid
**		redefine of EXCONTINUE by AIX's system header context.h.
**		The same change was done in 6.4 but didn't make into 6.5.
**	24-may-1993 (bryanp)
**	    There's a bunch of old VMS code in this file inside of xDEBUG
**	    ifdefs, which causes me problems when I try to compile it with
**	    xDEBUG. So I changed those ifdefs to xVMS_DEBUG. Now I can happily
**	    compile this module with xDEBUG defined...!
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (jnash)
**	    xDEBUG TRdisplay of CSinitiate() setuid() swapping.
**	26-jul-1993 (bryanp)
**	    In CSms_thread_nap, if ms <= 0, just return immediately.
**	9-aug-93 (robf)
**          Add su4_cmw
**	16-aug-93 (ed)
**	    add <st.h>
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	23-aug-1993 (andys)
**          In CS_dispatch call CS_cleanup_wakeup_events to clean up
**          any events left over from another process with the same pid.
**	    [bug 52761 in 6.4, this code is very different]
**      25-aug-93 (swm)
**          Added support for Alpha AXP OSF (axp_osf).
**	1-Sep-93 (seiwald)
**	    CS option revamp: moved all option processing out to
**	    CSoptions(), and moved iirundbms.c's resource fiddling
**	    into here and into CSoptions().  CS_STARTUP_MSG and 
**	    CS_OPTIONS gone.
**      01-sep-93 (smc)
**          Added casts to CS_SID, rewrote CS_addrcmp to be portable to
**	    machines where size of ptr > sizeof int.
**	20-sep-93 (mikem)
**	    Added support for new, per semaphore, semaphore contention 
**	    statistics gathering to CSp_semaphore().  Also added a few
**	    new statistics fields to the server level, semaphore contention
**	    statistics.
**	18-oct-1993 (bryanp)
**	    Add some trace statements when CSinitiate fails. A failure in
**	    CSinitiate is typically NOT well reported by mainline code, so we
**	    propose that extra messages for these errors are acceptable.
**	    Add extra argument to CS_cleanup_wakeup_events to indicate whether
**	    the caller already holds the system segment spinlock.
**	18-oct-1993 (kwatts)
**	    Now that most CL routines have an II in front of them, CSresume,
**	    CSget_sid, and CSsuspend need wrappers to continue support of
**	    the old names. This is for the ICL Smart Disk module linkage.
**	22-oct-93 (robf)
**          Enhance CSaltr_session to change priority, CS_AS_PRIORITY
**      31-dec-1993 (andys)
**          In CSinitiate, set up the global variable in EX which enables
**          CS_dump_stack to be called from EXsys_report if we are a dbms
**          server.
**	31-Aug-1993 (daveb)
**	    MOon_off( MO_DISABLE ) if CSinitiate(0,0,0);
**	15-Dec-1993 (daveb) 58424
**	    STlcopy for CSn_semaphore is wrong.  It'll write an EOS
**  	    into the cs_scribble_check that follows.
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x's to %p's (for pointer values) in CSp_semaphore(), 
**	    CSv_semaphore() and CSr_semaphore().
**	14-oct-93 (swm)
**	    Bug #56445
**	    Assign sem or cnd pointers to new cs_sync_obj (synchronisation
**	    object) rather than overloading cs_memory which is a i4;
**	    pointers do not fit into i4s on all machines!
**	    Also, now that semaphores are exclusive, remove unnecessary
**	    code that negates the sem address.
**	    Also, since semaphores are exclusive eliminate the tests for a
**	    negated semaphore address in TRdisplays.
**	2-oct-93 (swm)
**	    Bug #56447
**	    Cast assignment to cs_used to CS_SID, now that cs_used type has
**	    changed so that CS_SID values can be stored without truncation.
**	31-jan-1994 (bryanp) B56917
**	    CSoptions now returns STATUS, not VOID, and its return code is now
**		checked.
**	31-jan-1994 (jnash)
**	    Add new errors returned from CSinitiate().
**	25-feb-1994(ajc)
**	    Added hp8_bls entries inline with hp8*. Also added to NO_OPTIM.
**	21-mar-94 (dkh)
**	    Add call to EXsetclient() to retain the trap all behavior
**	    for the dbms.
**	28-mar-94 (mikem)
**	    Bug #58257
**	    During server startup of the rcp following machine/installation
**	    crash the rcp does log file scans while in the CS_INITIALIZING
**	    state.  Before this change all I/O's done during this time were
**	    taking a fixed 200 ms. as the "wakeme" bit was never set, so the
**	    di slaves never woke up the server when they were done.
**	20-apr-94 (mikem)
**	    Broke out all semaphore related routines into a new file 
**	    "cssem.c", this was done to give compiler optimizers a chance
**	    at these high traffic routines (many optimizer are disabled in
**	    csinterface.c because they can't handle the stack trickiness 
**	    done by other modules.
**      20-apr-94 (mikem)
**          Bug #57043
**          Added a call to CS_checktime() to CSswitch().  This call
**          is responsible for keeping the pseudo-quantum clock up to date, 
**          this clock is used to maintain both quantum and timeout queues in
**          CS.  See csclock.c for more information.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  Move do_mo_sems object
**  	    from csinterface.c to csmosem.c
**      13-feb-1995 (chech02)
**          Added rs4_us5 for AIX 4.1
**	13-Apr-1995 (abowler)
**	    Renamed setsig to EXsetsig - bug 52338
**	25-Apr-1995 (jenjo02)
**	    Added GLOBALDEF for Cs_current_scb for use by (new)
**	    CSp|v_semaphore macros.
**	27-Apr-1995 (jenjo02)
**	    Added total server cpu time to CSdump_statistics().
**	3-may-1995 (hanch04)
**	    Added changes made by nick
**          Fixed problem where CS would crap out when compiled under
**          su4_us5 V3 compiler.  We were faking the return address of
**          of CS_su4_setup() into CSdispatch() as CS_su4_setup+4.  Due
**          to compiler changes, this had changed to CS_su4_setup-4.  To
**          avoid trying to determine compiler versions, CS_su4_setup() now
**          coded in assembler to maintain the offset at a consistent value.
**          Integrated BryanP change from OpenIngres to remove erronous
**          setting of Cs_resumed = 1 in several places ; this had the effect
**          of causing the next CSsuspend() to return immediately.
**	13-May-1995 (smiba01)
**	    Added dr6_ues support.  These changes were originally made in
**	    ingres63p library.  (28-apr-93 (ajc)).
**	10-july-1995 (thaju02)
**	    Modified CSsuspend.  Added timeout usage for single threaded mode.
**	14-jul-1995 (morayf)
**	    Added sos_us5 to odt_us5 ifdefs.
**	15-jul-95 (popri01)
**	    Added nc4_us5 to sqs_pushf user ifdefs
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	31-july-1995 (thaju02)
**	    Modified CSsuspend.  For single threaded mode of execution,
**	    added extra check such that we only will timeout if the timeout
**	    value specified is non-zero.  This check had to be added 
**	    due to the acp server crashing (unable to handle a lock request
**	    timeout).
**      22-Aug-1995 (jenjo02)
**          Count only non-CS_INTRNL_THREADs in cs_num_active.
**      24-aug-95 (wonst02)
**          Add CSfind_sid() to return a SID given an SCB pointer.
**	19-sep-1995 (nick)
**	    Define an alternate stack in CSdispatch if we're running with
**	    stack overflow checking.
**      21-Sep-1995 (johna)
**          Back out the changes for timeout by thaju02 (418541 & 419351)
**      22-sep-1995 (pursch)
**          Add pym_us5 to a few code paths, fix flags, and
**          integrate from 6404mip_us500 csinterface.c.
**      10-nov-1995 (murf)
**              Added sui_us5 to all areas specifically defined with usl_us5.
**              Reason, to port Solaris for Intel.
**	25-oct-1995 (stoli02/thaju02)
**	    Added event control block wait and count statistics.
**	12-Jan-1996 (jenjo02)
**	    Fixed cs_wtstatistics, which previously were useless.
**	13-Mch-1996 (prida01)
**	    Add Ex_diag_link for ex to cs calls
**	06-dec-1995 (morayf)
**	    Added SNI RMx00 port (rmx_us5) conditionals, like ds3_ulx.
**	    Really add the pym_us5 code - it appeared to be missing.
**	03-jun-1996 (canor01)
**	    Preliminary support for operating system threads. Most of the
**	    code in this file comes from the NT version of csinterface.c.
**	    Initialize thread-local storage for use by CS.  Add a semaphore 
**	    to delay startup of individual threads until server is completely
**	    initialized.  CS_SID is a thread id, not a (CS_SCB *). Add 
**	    cs_access_sem to scb to synchronize updates to states and flags
**	    (similar to MCT's 'busy' flag).  Cannot use Cs_srv_block.cs_current
**	    global to reference current CS_SCB; it must be stored in thread-
**	    local storage.
**	13-sep-196 (canor01)
**	    When forcing the shutdown of the server, make an attempt to 
**	    kill off the user threads first.
**	16-sep-1996 (canor01)
**	    If the server is already in a CS_TERMINATING state, don't
**	    try to call CSterminate again on exit.
**	27-sep-1996 (canor01)
**	    Protect against multiple sessions attempting to shutdown
**	    the server simultaneously.
**	04-oct-1996 (canor01)
**	    Above change not protective enough.  Single-thread CSterminate().
**	10-oct-1996 (canor01)
**	    Make call to GC_set_blocking() in CSdispatch() to let GC layer
**	    know this is a multi-threaded server using blocking I/O.
**	29-oct-1996 (canor01)
**	    Increase file descriptor table size to the maximum since
**	    slaves are not used.
**	11-nov-1996 (canor01)
**	    Don't let threads be awakened by a "remove session" unless
**	    they are in an interruptible wait state.
**	    Protect cs_user_sessions from simultaneous access.
**	12-nov-1996 (canor01)
**	    Allow multi-threaded server to use iiCLpoll() instead of
**	    blocking I/O.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions 
**          from "MEtls..." to "ME_tls..." to avoid conflict with new 
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      13-dec-1996 (canor01)
**          If a session is waiting on an Ingres condition, wake it up
**          for shutdown.
**	18-dec-1996 (canor01)
**	    Session id with POSIX threads is a (CS_SCB*), since the thread
**	    id can be an arbitrary structure.
**	18-dec-1996 (wonst02)
**	    Allow third parameter to be a ptr to an LK_LOCK_KEY when sleep
**	    mask is CS_LOCK_MASK, so iimonitor can track locks.
**	08-jan-1997 (canor01)
**	    Fix typo in previous submission for POSIX threads.
**      22-jan-1997 (canor01)
**          Server becomes single-threaded in a CS_TERMINATING state, and
**          must use single-threaded suspend/resume to allow slave events to
**          complete.
**	28-jan-1997 (wonst02)
**	    Call CSMT_tls_ routines instead of ME_tls_. 
**	10-feb-1997 (canor01)
**	    Peek at the socket connections for possible completions
**	    of asynchronous BIO calls (such as expedited channel) on
**	    every CSsuspend().
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          rename file to csmtintef.c and moved all calls to CSMT...
**	19-feb-1997 (canor01)
**	    Make sure CS_tls_init gets called for pseudo-servers.
**      28-feb-1997 (hanch04)
**          Move Cs_known_list_sem to cs
**      07-mar-1997 (canor01)
**          Make all functions dummies unless OS_THREADS_USED is defined.
**      06-mar-1997 (reijo01)
**          Changed a check for "ingres" to a check for SystemAdminUser.
**          Changed II_DBMS_SERVER message to use generic system name.
**      20-mar-1997 (canor01)
**          Add support for CSsuspend() timeouts in nanoseconds.
**	25-Mar-1997 (jenjo02)
**	    Fixed bad test of CS_LOCK_MASK in CSMTsuspend().
**	11-apr-1997 (canor01)
**	    Restore the quantum check in CSMTswitch() to limit 
**	    calls to poll() to when the quantum has expired.
**      14-Apr-1997 (bonro01/mosjo01) 
**          Added code to initialize an accounting semaphore that
**          was not being initialized.
**          Added code to get the error number from 'errno' instead
**          of from the posix functions return value for dg8_us5
**          and dgi_us5.
**	02-may-1997 (canor01)
**	    If a thread is created with the CS_CLEANUP_MASK set in
**	    its thread_type, transfer the mask to the cs_cs_mask.
**	    These threads will be terminated and allowed to finish
**	    processing before any other system threads exit.
**	09-may-1997 (muhpa01)
**	    Incorporated changes for POSIX_DRAFT_4 implementation of
**	    OS_THREADS (hp8_us5 port)
**	20-May-1997 (jenjo02)
**	  - Moved MUset_funcs() to before NMstIngAt() call so that
**	    the MU semaphores will be converted before any threads
**	    are dispatched.
**	  - In CSsuspend(), do asych poll only for user threads.
**	22-May-1997 (jenjo02)
**	   Before polling for asych messages, release cs_event_sem. If an asych
**	   message has arrived, it may drive CSMTattn_scb(), which will try
**	   to acquire cs_event_sem (CSMTsuspend()).
**	27-May-1997 (jenjo02)
**	    Removed call to MUset_funcs(). With OS threads, MU uses perfectly good
**	    opsys mutexes which need no conversion to CS_SEMAPHORE type. The 
**	    conversion was causing a race condition which is avoided by not doing
**	    the unnecessary conversion.
**      23-may-1997 (reijo01)
**          Print out II_DBMS_SERVER message only if there is a listen address.
**	10-jun-1997 (canor01)
**	    Allow multiple calls to CSMT_tls_init(), in case initial access
**	    comes in from an unexpected path.
**	26-Jun-1997 (jenjo02)
**	    Initialize TLS before mapping system segment and possibly returning
**	    with an error. Error handling depends on TLS having been initialized.
**	20-Aug-1997 (kosma01)
**	    Provide a forward function declaration for the AIX's sigdanger.
**	    Added a GLOBALREF for semaphore CS_acct_sem, and initialized it.
**	    Cast variables being assigned into scb->cs_self for POSIX thread
**	    builds. CS_SID for POSIX thread is defined as a scalarp in csnormal.h.
**	    The AIX 4.1 compiler complained of an illegal interger to pointer 
**	    assignment.
**	05-sep-1997 (canor01)
**	    Do not initialize scb's cs_length field if the allocator has
**	    already done so.
**	16-sep-1997 (canor01)
**	    If a call to CScnd_signal() is made before the waiting thread
**	    has gone to sleep on the condition object, it is possible to
**	    lose the signal.  Added CND_WAITING state to synchronize.
**      23-sep-1997 (canor01)
**          Print startup message to stderr.
**	30-sep-1997 (canor01)
**	    In CScnd_wait(), protect access to scb flags with cs_access_sem
**	    and double-check CS_ABORTED_MASK.
**	10-Dec-1997 (jenjo02)
**	    Added cs_get_rcp_pid to list of server functions.
**      7-jan-97 (stephenb)
**          Add active user high water mark to dump statistics.
**	16-Feb-98 (fanra01)
**	    Add initialisation of the attach and detach functions.
**	18-mar-1998 (canor01)
**	    If thread creation fails due to an out-of-resources situation,
**	    make sure the correct error gets logged and mutexes released.
**	20-Mar-1998 (jenjo02)
**	    Removed cs_access_sem, utilizing cs_event_sem instead to 
**	    protect cs_state and cs_mask.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSMTadd_thread() prototype.
**	13-Apr-1998 (muhpa01)
**	    Added hpb_us5 to list of platforms using ETIMEDOUT value
**	    returned from CS_cond_timedwait.  Also, in CSMTterminate(), exit
**	    the idle thread before calling cs_shutdown() to prevent EBADF
**	    return from select to the admin thread.
**      06-may-1998 (canor01)
**          Initialize CI function to retrieve user count.  This is only
**          valid in a server, so initialize it here.
**	24-Jun-1998 (allmi01)
**	    Crooect lock / unlock of mutex to prevent server hangup on startup.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
**	    Eliminate cs_cnt_mutex, adjust counts while holding Cs_known_list_sem
**	    instead.
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of suspends.
**	    In CSMTsuspend(), don't do a timed wait if this wait is 
**	    noninterruptable.
**	    Fixed up computation of thread CPU time, moved CS_thread_info()
**	    to CS_update_cpu().
**	05-nov-1998 (devjo01)
**	    Reenable stack_dumps now that b89582 is resolved.
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
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	22-Feb-1999 (schte01)
**       For USE_IDLE_THREAD in CSMTsuspend, test cs_state for
**       CS_INITIALIZING or CS_TERMINATING (indicating a psuedo_server)
**       before possible cond_wait. Fix CSMTresume to test cs_state before
**       trying to get scb based on sid. (CSMTresume is not always called
**       with a valid sid (e.g., when it is called as an event completion
**       routine from CS_find_events).
**	15-mar-1999 (hanch04)
**	    Allocate the full stack size.  If stack size is zero, default
**	    to the OS stack size.
**	15-mar-1999 (muhpa01)
**	    Removed code in CSMTterminate() for hpb_us5 which dealt with
**	    exit of the idle thread.
**	23-mar-1999 (hanch04)
**	    Set stack size to 65536 default because some OS default are
**	    too small.
**	31-Mar-1999 (bonro01)
**	    A previous fix created a mismatch between the CSMTsuspend
**	    and CSMTresume routines for platforms that DON'T use
**	    USE_IDLE_THREAD.  The bug is that CSMTsuspend would wait on
**	    a condition while the CSMTresume routine would SKIP the signal
**	    condition and set Cs_resumed=1 instead. This fix is to ifdef
**	    the code using USE_IDLE_THREAD in CSMTresume so that it tests
**	    cs_state before trying to get scb based on sid only when using
**	    the Idle Thread.
**	01-Apr-1999 (kosma01)
**	    Remove declaration of TRdisplay() as it was conflicting with the
**	    definition in gl/hdr/hdr/tr.h on sgi_us5.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	27-Apr-1999 (ahaal01)
**	    Set NO_OPTIM for AIX (rs4_us5).
**      22-Jun-1999 (popri01) added Siemens specific SIGVTALRM - virtual
**          time alarm signal to handle getittimer and setittimer functions.
**      22-Jun-1999 (podni01) added rmx_us5 to the list of the platforms for
**          special return code handling from thread-manipulating functions.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	17-Nov-1999 (jenjo02)
**	    Cleaned up thread/server CPU time computation. getrusage()
**	    was being called way too often and should only be called
**	    when the server process time is needed, not when thread
**	    CPU time is wanted and available.
**	08-Dec-1999 (podni01)
**	    Put back all POSIX_DRAFT_4 related changes; replaced POSIX_DRAFT_4
**	    with DCE_THREADS to reflect the fact that Siemens (rux_us5) needs 
**	    this code for some reason removed all over the place.
**      13-Dec-1999 (hweho01)
**          Added support for AIX 64-bit platform (ris_u64).
**	24-May-2000 (toumi01)
**	    Added int_lnx to list of platforms using ETIMEDOUT value
**	    returned from CS_cond_timedwait.
**	28-jul-2000 (somsa01)
**	    When printing out the server id, use the new SIstd_write()
**	    function.
**	15-aug-2000 (somsa01)
**	    Added ibm_lnx to list of platforms using ETIMEDOUT value
**	    returned from CS_cond_timedwait.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Deleted cs_bio_done, cs_bio_time, cs_bio_waits, cs_dio_done,
**	    cs_dio_time, cs_dio_waits from Cs_srv_block, cs_dio, cs_bio
**	    from CS_SCB. Kept the MO objects for them and added MO
**	    methods to compute their values.
**      18-Sep-2000 (hanal04) Bug 102042 Problem INGDBA67.
**          Ensure the active session count (cs_num_active) actually
**          reflects the count of ACTIVE user sessions, rather than
**          the number of user sessions.
**	04-Oct-2000 (jenjo02)
**	    Implemented CS_FUNCTIONS function vector table for
**	    CS functions which have both Ingres-thread (CS) and OS-thread
**	    (CSMT) components. This eliminates CS->CSMT context
**	    switching for those functions. CS? function components
**	    renamed to IICS? to avoid conflict with function vector
**	    macros. See cs.h for a complete list. Deleted FUNC_EXTERNs
**	    prototyped elsewhere.
**	14-mar-2001 (somsa01)
**	    Added HP-UX equivalents for suspending/resuming a session
**	    thread.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	24-aug-2001 (abbjo03)
**	    Back out change to tracking of cs_num_active (18-sep-2000) because
**	    subsequent invocations of IPM or iimonitor cause the count of
**	    active sessions to keep on increasing.
**	03-Dec-2001 (hanje04)
**	    Added IA64 to list of platforms using EDTIMEOUT.
**	22-May-2002 (jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	03-Oct-2002 (somsa01)
**	    In CSMTdispatch(), to be consistent with the way we print
**	    out "PASS" and "FAIL" in CSdispatch(), print out to stdout.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	01-nov-2002 (somsa01)
**	    In CSMTinitiate(), added using CS_set_default_stack_size() to
**	    set the default thread stack size for the current process.
**	    Also, on HP-UX, pass in cs_stkcache to CS_create_thread().
**	12-nov-2002 (horda03) Bug 105321
**	    Ensure there are no CP resumes pending for the session.
**      03-Dec-2002 (hanje04)
**          In CSMTinitiate() the Linux pre compiler doesn't like the #ifdef
**          being used to call the CS_create_thread macro with a extra 
**          parameter for HP. Add a completely separate call as for DCE_THREADS
**	13-Feb-2003 (hanje04)
**	    BUG 109555
**	    In CSMTinitiate() after obtaining server PID set PGID on Linux
**	    (with setpgrp()) so that we have a constant value for referencing 
**	    the server as a whole once we start creating threads.
**	03-mar-2003 (devjo01) b109753
**	    Correct faulty logic involving 'cnd_mutex_sem' which was causing
**	    a memory leak on Tru64, but is wrong on all platforms.
**	29-Apr-2003 (hanje04) b110147
**	    On Linux, in CSMTterminate use exit() instead of _exit() to 
**	    'stop server' other wise only the thread exits.
**      21-Jul-2003 (hanje04)
**          BUG 10995
**          Define server PID to always be PGID on Linux.
**      23-Jul-2003 (hweho01)
**          Turned off optimizer for 64-bit build on AIX.
**          Process abended on log file initialization.  
**          Compiler : VisualAge 5.023.
**      27-Oct-2003 (hanje04)
**          BUG 110542 - Linux Only - Ammendment
**          Minor changes to Linux altstack code to get it working on S390
**          Linux as well. Don't declare altstack in CSMTdispatch for Linux.
**	18-Nov-2003 (devjo01)
**	    Replace calls to CScas_long with CScas4.
**	23-jan-2004 (devjo01)
**	  - Don't use CAS to update csev_state if not a clustered
**	    build.  Slightly reduces overhead, and avoids need
**	    to code C&S routines for platforms which will not
**	    support clustering.
**	  - Fix compile warning in non-lnx, non-HP call to CS_create_thread.
**	13-feb-2004 (devjo01)
**	    Correct omissions in previous submission.  In particular
**	    Handle C&S reference in CSMTresume, and make CSMTresume_from_AST
**	    conditional on conf_CLUSTER_BUILD.
**	4-apr-2004  (mutma03)  
**	    Added CSMTpoll_for_AST to check all suspended threads for missed AST
**	    due to suspend/resume race condition and signals the thread .
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	2-Sep-2004  (mutma03)  
**	    Removed CSMTpoll_for_AST and added CSMTsuspend_for_AST
**	    to suspend for a completion AST from DLM.
**	16-nov-2004 (devjo01)
**	    Modify check of sem_timedwait return status to compensate for
**	    known issue where some Linux distros are not POSIX compliant.
**    14-Mar-2005 (mutma03)
**          Fix CSsuspend_for_AST to not to return spurious E_CS0008_INTERRUPTED
**          during timed wait.
**      15-Mar-2005 (bonro01)
**          Added test for NO_INTERNAL_THREADS to
**          support Solaris AMD64 a64_sol.
**	18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	31-Jan-2007 (kiria01) b117581
**	    Use global cs_mask_names, cs_state_names and
**	    ev_state_mask_names. Use event_state private masks to
**	    reduce confusion.
**	13-Feb-2007 (kschendel)
**	    Add a better cancel checker.
**	4-Oct-2007 (bonro01)
**	    Moved CS_dump_stack() prototype to csmtlocal.h
**	    Removed duplicate assignment of Ex_print_stack.
**	05-Oct-2007 (hanje04)
**	    SIR 114907
**	    Quiet compiler warnings from pthread_create() and add Mac OSX to
**	    list of platforms checking ETIMEOUT instead of ETIME in
**	    cancel checker as ETIME is undefined here.
**     11-Mar-2008 (horda03) Bug 120095
**          CS_cond_timedwait returns ETIMEOUT code on hpb_us5, missed
**          compiler directive added (2 locations). Use of ETIME means that
**          ACP,RCP and DBMS log files fill with SUSP:<>Returned bad status 238...
**          messages.
**	22-Aug-2008 (wanfr01) Bug 120806
**	    Extend fix for bug 120095 for i64_hpu
**	01-dec-2008 (joea)
**	    In CSMTsuspend, add a missing xCL_USE_ETIMEDOUT.
**	02-dec-2008 (joea)
**	    Disable profiling on VMS.
**	05-dec-2008 (joea)
**	    VMS port: Implement cs_define code.  Exclude Unix signal, event
**	    system and iiCLpoll code.
**	09-Feb-2009 (smeke01) b119586
**	    Enable stats gathering for admin thread.
**      19-mar-2009 (stegr01)
**          Add support for VMS itanium posix threading
**          exclude calls to IICLpoll and replace with calls to
**          execute ASTs if any are waiting in CSMTsuspend
**          CSMTresume checks to see if it is resuming from an AST
**          whilst running on the same thread to avoid a pthread bug
**      20-mar-2009 (stegr01)
**          replace TMet() by TMnow()
**     20-Apr-2009 (horda03) Bug 121826
**          In CSMTsuspend, prevent hangs during CLpoll calls, when caller
**          has supplied a timeout.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Mostly comment updates.  Clear IRPENDING if we return "interrupted"
**	    because of IRCV.  Clear old wait-mask from cs_mask before setting
**	    new mask from caller.  Be sure to never return "interrupted" from
**	    a non-interruptable wait, doing so will surely screw up the caller.
**	    Stop fooling with EDONE in cs_mask, it's not set there in CSMT.
**	    (it's set in event_state instead.)
**      04-nov-2010 (joea)
**          Complete prototype for CSMTattn_scb.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    CSMT_swuser();	/* suspend one thread, resume one */ 
FUNC_EXTERN i4      CSsigterm();        /* terminate due to Unix signal */
FUNC_EXTERN STATUS  CS_set_server_connect();
FUNC_EXTERN VOID    CS_move_async();
FUNC_EXTERN void    CS_cpres_event_handler();
FUNC_EXTERN STATUS    CSoptions( CS_SYSTEM *cssb );
typedef void FP_Callback(CS_SCB **scb);
FUNC_EXTERN void    FP_set_callback( FP_Callback fun);

# if defined(any_aix)
FUNC_EXTERN TYPESIG CSsigdanger();       /* handler for sigdanger */
# endif /* power aix */

FUNC_EXTERN VOID CS_mo_init(void);
i4 CS_addrcmp( const char *a, const char *b );
FUNC_EXTERN VOID GC_set_blocking(bool b);

static void CSMTattn_scb(i4 eid, CS_SCB *scb);
static i4  CSMT_usercnt(void);


/*
**  Defines of other constants.
*/
/* This constant sets the amount of time CSstatistics will let a thread
   run before forceing a reschedule.  This value is on the high side
   on the assumption that dmfrcp (who updates the in-core clock) may miss
   clock ticks because the server in question is running the cpu flat out. */
# define	STATSWITCH 1000	/* 1 second */
/* This constant is the minimum time slice a user may specify */
# define      MINSWITCH  1    /* minimal */
/*
** UNINT_TMO_INCR_CAP - limit the duration of the uninterruptable condition wait
** timeout so that the auxiliary state variables will be checked at leat this
** often.
*/
#define UNINT_TMO_INCR_CAP 30

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF CS_SEMAPHORE        Cs_known_list_sem;
GLOBALREF CS_SEMAPHORE        CS_acct_sem;
GLOBALREF CS_SYNCH	      Cs_utility_sem;

GLOBALREF CS_SCB	      Cs_known_list_hdr;
GLOBALREF CS_SYSTEM	      Cs_srv_block; /* Overall ctl struct */
GLOBALREF CS_SCB	      Cs_to_list_hdr;
GLOBALREF CS_SCB	      Cs_wt_list_hdr;
GLOBALREF CS_SCB	      Cs_as_list_hdr;
GLOBALREF CS_STK_CB	      Cs_stk_list_hdr;
GLOBALREF CS_SCB	      Cs_idle_scb;
GLOBALREF CS_ADMIN_SCB	      Cs_admin_scb;

GLOBALREF CS_SMCNTRL	      *Cs_sm_cb;
GLOBALREF i4	      Cs_lastquant;
GLOBALREF i4		      Cs_incomp;
GLOBALREF CS_SEMAPHORE        *ME_page_sem;
GLOBALREF VOID                (*Ex_print_stack)();
GLOBALREF VOID                (*Ex_diag_link)();

static i4	got_ex	ZERO_FILL;
static bool	Cs_resumed ZERO_FILL;
static CS_SCB	cs_ticker_scb;

#if defined(VMS)
static i4 CSMT_execute_asts(CS_SCB* scb);
#endif

GLOBALREF i4	      Cs_numprocessors;
GLOBALREF i4	      Cs_max_sem_loops;
GLOBALREF i4	      Cs_desched_usec_sleep;
/*
** The following GLOBALDEFs are used by the CSp|v_semaphore macros
** to locate the current thread's SCB and pid.
** The companion GLOBALREFs can be found in cl!hdr!hdr csnormal.h, 
** which also houses the macros.
*/
GLOBALREF CS_SCB	      **Cs_current_scb;
GLOBALREF i4      	      *Cs_current_pid;

GLOBALREF char		      cs_server_class[];

/****************************************************************************
** The CS multi-tasking semaphore allows all the various threads to
** cooperate on when they will begin. Until we are ready to 'really' start
** multi-threading, we will have this semaphore set, and no added threads
** will run (they will all block in CS_setup). Once we clear it, which we
** do when we are ready to multi-task, all the threads will run.
****************************************************************************/
GLOBALDEF CS_SYNCH CsMultiTaskSem   ZERO_FILL;

# if defined(DCE_THREADS)
GLOBALDEF CS_THREAD_ID   Cs_thread_id_init ZERO_FILL;
# endif 

GLOBALREF CS_SYNCH CsStackSem;

/****************************************************************************
** Thread local storage
****************************************************************************/
GLOBALDEF ME_TLS_KEY TlsIdxSCB ZERO_FILL;


GLOBALREF MO_CLASS_DEF CS_int_classes[];



/*{
** Name: CSinitiate - Initialize the CS and prepare for operation
**
** Description:
**	This routine is called at system startup time to allow the CS module
**	to initialize itself, and to allow the calling code to provide the
**	appropriate personality information to the module.
**
**	The personality information is provided by the single parameter, which
**	is a pointer to a CS_CB structure.  This structure contains personality
**	information including the "size" of the server in terms of number of
**	allowable threads, the size and number of stacks to be partitioned
**	amongst the threads, and the addresses of the routines to be called
**	to allocate needed resources and perform the desired work.  (The calling
**	sequences for these routines are described in <cs.h>.)
**
** Inputs:
**	argc				Ptr to argument count for host call
**	argv				Ptr to argument vector from host call
**	ccb				Call control block, of which the
**					    following fields are interesting
**					If zero, the system is initialized to
**					allow standalone programs which
**					may use CL components which call
**					CS to operate.
**	.cs_scnt			Number of threads allowed
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
**				    >>	scb->cs_username is expected to be
**				    >>	filled with the name of the invoking
**				    >>	user.
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
**      .cs_facility    		Routine to call to return the facility 
**					  for sampler
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
**					  return E_CS001D_NO_MORE_WRITES.
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
**	.cs_get_rcp_pid			Routine to return the RCP's pid,
**					    pid = (*cs_get_rcp_pid)(), used
**					    by CSMTp_semaphore().
**
** Outputs:
**	none
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-Oct-1986 (fred)
**	    Initial Definition.
**	15-jan-1987 (fred)
**	    Added command line parsing
**	24-feb-1987 (fred)
**	    Added AIC & PAINE handling.
**	29-sep-1988 (thurston)
**	    Added semaphoring information to the ERinit() call.
**	31-oct-1988 (rogerk)
**	    Check startup options for CSO_F_CS options - CSO_SESS_ACCTNG and
**	    CSO_CPUSTATS.
**	 7-nov-1988 (rogerk)
**	    Added async ready list.
**	 06-feb-89 (mikem)
**	    LGK_initialize() takes a CL_ERR_DESC now.
**	 09-mar-89 (russ)
**	    Modified the code to read the startup message.  This message
**	    now comes from a pipe, rather than a socket.
**      12-jun-1989 (rogerk)
**          Set process id (cs_pid) in the cs_system control block.
**          Set cs_pid in idle thread scb.
**	31-Oct-1989 (anton)
**	    Added quantum support
**	6-Dec-1989 (anton)
**	    Added MINSWITCH minimum time switch value
**	22-Jan-1990 (fredp)
**	    Integrated the following change: 23-Aug-89 (GordonW)
**	    Added close(fd) to close off pipe file.
**	26-Feb-1990 (anton)
**	    Don't register with name server when debugging
**	    (unless explicitly asked for)
**	14-dec-1990 (mikem)
**	    Initialize Cs_numprocessors with the number of processors on the
**	    machine, will be used to determine whether or not to spin in
**	    the CSp_semaphore() routine.
**	02-jul-1991 (johnr/rog)
**          Added CS_hp3_broken_tas to determine the correct test and set
**          instruction to use on the various hp3 platforms.
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**      12-mar-1992 (jnash)
**          For Sun4, if process running as ROOT, setuid
**          to Ingres.  It will be switched back to root when necessary.
**	4-Jul-92 (daveb)
**	    set up known element trees, and put idle, admin and added scbs
**	    into cs_sem_tree.  Put sems and cnd's intheir respective trees,
**	    and remove on their demise.  (SCB's are removed in CS_del_thread in
***	    cshl.c).
**	9-Jul-92 (daveb)
**	    Sem_tree won't work, use CS_sem_attach scheme instead.
**	23-sep-1992 (bryanp)
**	    LGK_initialize no longer needs to be called from CSinitiate().
**	5-nov-1992 (bryanp)
**	    Add new LG/LK server startup parameters. Also, CSinitiate now just
**	    quietly returns OK if called twice.
**	13-Jan-1993 (daveb)
**	    Rename MO objects to include a .unix component.
**
**  Design:  <Vax/VMS>
**
**	This routine first initializes the system control area, Cs_srv_block.
**	This involves the clearing of the structure, moving the tag information
**	into the first field in the structure, initializing the set of priority
**	queues which form the basis of the internal scheduling algorithm,
**	taking note of the appropriate set of system specific identification
**	information (process id, site id, startup time, etc.).	Finally, the
**	personality information is placed into the system control area, and that
**	area is marked as initialized.	The startup parameters are read from the
**	server startup program (server specific) via a mailbox.
**
**	The mailbox name is passed over as the process name (i.e., the process
**	name identifies the mailbox).  The server will find its process name,
**	assign a channel to it, then read the startup message.	In CSdispatch(),
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	26-jul-1993 (jnash)
**	    xDEBUG TRdisplay of CSinitiate() setuid() swapping.
**	26-jul-1993 (bryanp)
**	    Pass num-slaves argument to CS_event_init().
**	18-oct-1993 (bryanp)
**	    Add some trace statements when CSinitiate fails. A failure in
**	    CSinitiate is typically NOT well reported by mainline code, so we
**	    propose that extra messages for these errors are acceptable.
**      31-dec-1993 (andys)
**          When we know are a dbms server, initialise Ex_print_stack to
**          point to CS_dump_stack; this is used by EXsys_report.
**	31-Aug-1993 (daveb)
**	    MOon_off( MO_DISABLE ) if CSinitiate(0,0,0);
**	2-oct-93 (swm)
**	    Bug #56447
**	    Cast assignment to cs_used to CS_SID, now that cs_used type has
**	    changed so that CS_SID values can be stored without truncation.
**	31-jan-1994 (bryanp) B56917
**	    CSoptions now returns STATUS, not VOID, and its return code is now
**		checked.
**	31-jan-1994 (jnash)
**	    Add new errors.  As we have no sys_err parameter, errors 
**	    returned as the value of function.  
**	20-jan-1997 (wonst02)
**	    Call CS_tls_get instead of ME_tls_get.
**	19-feb-1997 (canor01)
**	    Make sure CS_tls_init gets called for pseudo-servers.
**	    Call CSMT_tls_get instead of ME_tls_get.
**      06-mar-1997 (reijo01)
**          Changed a check for "ingres" to a check for SystemAdminUser.
**	26-Jun-1997 (jenjo02)
**	    Initialize TLS before mapping system segment and possibly returning
**	    with an error. Error handling depends on TLS having been initialized.
**	05-sep-1997 (canor01)
**	    Do not initialize scb's cs_length field if the allocator has
**	    already done so.
**	10-Dec-1997 (jenjo02)
**	    Added cs_get_rcp_pid to list of server functions.
**	19-jan-1998 (canor01)
**	    Allow stack dumps to be printed from a threaded server.
**	25-apr-1998 (devjo01)
**	    Reenable stack_dumps.
**	24-jan-2002 (devjo01)
**	    Init new IIresume_from_AST member of CS_FUNCTIONS.
**	01-nov-2002 (somsa01)
**	    Added using CS_set_default_stack_size() to set the default
**	    thread stack size for the current process.
**      13-Feb-2003 (hanje04)
**          BUG 109555
**          In CSMTinitiate() after obtaining server PID set PGID on Linux
**          (with setpgrp()) so that we have a constant value for referencing
**          the server as a whole once we start creating threads.
**       7-Oct-2003 (hanal04) Bug 110889 INGSRV2521
**          Calling EXsetclient here is too late. It needs to be done
**          before any EXdeclare(), or EXsetup() call.
**          Ignoring SIGINT here is not limited to EX_INGRES_DBMS server 
**          types. auditdb needs to be able to catch SIGINT and is an
**          EX_INGRES_TOOL. Moved EXsetclient() call to scd_main()
**          and moved the SIG_IGN for SIGINT to i_EXestablish().
**      18-Aug-2004 (hanal04) Bug 112820 INGSRV2933
**          Prevent spurious exceptions in SP structures (among others)
**          during server initialisation by taking the CsMultiTaskSem
**          as soon as we enter this function. Also added the necessary
**          release calls if we exit before the original acquisition point.
**	6-Jul-2005 (schka24)
**	    Move utility sem init to here for e.g. rcpstat, so that
**	    CSadjust-counter works for callers that don't run the
**	    dispatcher loop.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	07-Oct-2008 (smeke01) b121633
**	    Save the 0S thread-id for the admin thread.
**	09-Feb-2009 (smeke01) b121621
**	    On Unix platforms make details of the admin thread available 
**	    to IMA. 
**	09-Feb-2009 (smeke01) b119586
**	    Enable stats gathering for admin thread.
*/
STATUS
CSMTinitiate(i4 *argc, char ***argv, CS_CB *ccb )
{
    i4			i;
    i4			status = 0;
    char		*cp;
    CS_SCB		*idle_scb;
    CS_SYSTEM		*cssb = &Cs_srv_block;
    CL_ERR_DESC		errcode;
    char                user_name[33];
    char                *user_name_ptr = user_name;
    struct passwd       *p_pswd, pswd;
    struct passwd       *getpwnam();
    char		pwnam_buf[512];
    i4			fd_wanted;
    CS_SCB		*pSCB;
    CS_SCB		**ppSCB = NULL;

    /* Prevent all other threads from moving until we release CsMultiTaskSem */
    CS_synch_init( &CsMultiTaskSem );
    CS_synch_lock( &CsMultiTaskSem );


#ifdef VMS

    /*
    ** if we're in CSMTinitiate then we are ipso fact using OS threads
    ** so initialise the AST dispatcher thread and the AST jacket functions
    ** a NULL ccb indicates a single threaded server (e.g. dmfacp)
    ** whilst a non-null ccb is a true multithreaded server (i.e. DBMS)
    */

    IIMISC_initialise_astmgmt(ccb);
#endif

    /* Initialize OS-threaded function vectors */
    Cs_fvp->IIp_semaphore = CSMTp_semaphore;
    Cs_fvp->IIv_semaphore = CSMTv_semaphore;
    Cs_fvp->IIi_semaphore = CSMTi_semaphore;
    Cs_fvp->IIr_semaphore = CSMTr_semaphore;
    Cs_fvp->IIs_semaphore = CSMTs_semaphore;
    Cs_fvp->IIa_semaphore = CSMTa_semaphore;
    Cs_fvp->IId_semaphore = CSMTd_semaphore;
    Cs_fvp->IIn_semaphore = CSMTn_semaphore;
    Cs_fvp->IIw_semaphore = CSMTw_semaphore;

    Cs_fvp->IIget_sid = CSMTget_sid;
    Cs_fvp->IIfind_sid = CSMTfind_sid;
    Cs_fvp->IIset_sid = CSMTset_sid;
    Cs_fvp->IIget_scb = CSMTget_scb;
    Cs_fvp->IIfind_scb = CSMTfind_scb;
    Cs_fvp->II_find_scb = CSMT_find_scb;
    Cs_fvp->IIget_cpid = CSMTget_cpid;
    Cs_fvp->IIswitch = CSMTswitch;
    Cs_fvp->IIstatistics = CSMTstatistics;
    Cs_fvp->IIadd_thread = CSMTadd_thread;
    Cs_fvp->IIcancelled = CSMTcancelled;
    Cs_fvp->IIcnd_init = CSMTcnd_init;
    Cs_fvp->IIcnd_free = CSMTcnd_free;
    Cs_fvp->IIcnd_wait = CSMTcnd_wait;
    Cs_fvp->IIcnd_signal = CSMTcnd_signal;
    Cs_fvp->IIcnd_broadcast = CSMTcnd_broadcast;
    Cs_fvp->IIattn = CSMTattn;
    Cs_fvp->IIdispatch = CSMTdispatch;
    Cs_fvp->IIterminate = CSMTterminate;
    Cs_fvp->IIintr_ack = CSMTintr_ack;
    Cs_fvp->IIremove = CSMTremove;
    Cs_fvp->IIaltr_session = CSMTaltr_session;
    Cs_fvp->IIdump_statistics = CSMTdump_statistics;
    Cs_fvp->IImonitor = CSMTmonitor;
    Cs_fvp->IIsuspend = CSMTsuspend;
    Cs_fvp->IIresume = CSMTresume;
#ifdef conf_CLUSTER_BUILD
    Cs_fvp->IIsuspend_for_AST = CSMTsuspend_for_AST;
    Cs_fvp->IIresume_from_AST = CSMTresume_from_AST;
#else
    /*
    ** If not a cluster build, "CSresume_from_AST" is never
    ** called in an AST/Signal handler context.
    */
    Cs_fvp->IIsuspend_for_AST = CSMTsuspend;
    Cs_fvp->IIresume_from_AST = CSMTresume;
#endif
    Cs_fvp->IIcancelCheck = CSMTcancelCheck;

/*
** If required facilities exist, check for server running as root and
** reset id back to ingres.
** Note that this will have to be changed when a user other than
** Ingres can own an installation.
*/

#if defined(xCL_MLOCK_EXISTS) && \
    defined(xCL_077_BSD_MMAP) && \
    defined(xCL_SETEUID_EXISTS)

    /*
    ** If server was run by root, or if the effective uid is root,
    ** set it to INGRES.  We'll set it to root when we need to lock
    ** the DMF cache.
    **
    ** It would be nice if we could set both real and effective to
    ** Ingres and then set only the effective to root when we need to,
    ** but there is no way to do this.  POSIX_SAVID_IDS
    ** might appear to do this, but do not work this way for ROOT.
    */

    IDname(&user_name_ptr);
    if ( (STcompare(user_name, "root") == 0) || (geteuid() == 0) )
    {
#ifdef xDEBUG
        TRdisplay("CSMTinitiate: old uid: %d, old euid %d\n",
			getuid(), geteuid());
#endif

        if( !( p_pswd = iiCLgetpwnam( SystemAdminUser, &pswd, 
				      pwnam_buf, sizeof(pwnam_buf) ) ) )
        {
            TRdisplay("CSMTinitiate: getpwnam() failure\n");
            CS_synch_unlock( &CsMultiTaskSem );
            return(E_CS0035_GETPWNAM_FAIL);
        }

	if (seteuid(p_pswd->pw_uid) == -1)
	{
	    TRdisplay("CSMTinitiate: seteuid() error %d\n", errno);
            CS_synch_unlock( &CsMultiTaskSem );
	    return(E_CS0036_SETEUID_FAIL);
	}

#ifdef xDEBUG
        TRdisplay("CSMTinitiate: euid changed to %d\n", geteuid());
#endif

    }
#endif

#ifndef VMS
    NMgtAt("II_PROF", &cp);
    if (cp && *cp)
    {
	CS_recusage();
	CSprofstart();
    }
#endif

    if (cssb->cs_state)
    {
        CS_synch_unlock( &CsMultiTaskSem );
	return(OK);
    }

    /* MEfill(sizeof(Cs_srv_block), '\0', (PTR) cssb); */

#if defined(LNX)
    setpgrp();
#endif
    /*
    ** Store Process Id of this server process.
    */
    PCpid(&Cs_srv_block.cs_pid);

    cssb->cs_state = CS_INITIALIZING;

    CS_synch_init(&Cs_utility_sem);	/* for CSadjust_counter */

    /*
    ** Initialize Thread Local Storage before anything else.
    ** If we fail for some reason, other facilities may
    ** depend on TLS existing.
    */
    CSMT_tls_init(ccb ? ccb->cs_scnt : 1, &status);
    CSMT_tls_get( (PTR *)&ppSCB, &status );
    if ( ppSCB == NULL )
    {
	ppSCB = (CS_SCB **) MEreqmem(0,sizeof(CS_SCB **),TRUE,NULL);
	CSMT_tls_set( (PTR)ppSCB, &status );
    }
    pSCB = *ppSCB = (CS_SCB *)&Cs_admin_scb;

#if defined(VMS)
    if ((status = CS_cp_mbx_create(ccb ? ccb->cs_scnt : 0, &errcode)) != OK)
    {
        TRdisplay("CSMTinitiate: Error (%x) in CS_cp_mbx_create\n", status);
        CS_synch_unlock( &CsMultiTaskSem );

        return (status);
    }

    if (status = CS_map_sys_segment(&errcode))
    {
        i4 nserv = MAXSERVERS/4;
        i4 nusers = nserv * 150;
        i4 err_code = 0;

        if (status = CS_create_sys_segment(nserv, nusers, &err_code))
        {
            TRdisplay("CSMTinitiate: Error (%x) in CS_create_sys_segment\n", status);
            CS_synch_unlock( &CsMultiTaskSem );
            return(E_CS0037_MAP_SSEG_FAIL);
        }

        if (status = CS_map_sys_segment(&errcode))
        {
        TRdisplay("CSMTinitiate: Error (%x) in CS_map_sys_segment\n", status);
            CS_synch_unlock( &CsMultiTaskSem );
            return(E_CS0037_MAP_SSEG_FAIL);
        }
    }
#else
    if (status = CS_map_sys_segment(&errcode))
    {
	TRdisplay("CSMTinitiate: Error (%x) in CS_map_sys_segment\n", status);
        CS_synch_unlock( &CsMultiTaskSem );
	return(E_CS0037_MAP_SSEG_FAIL);
    }
#endif /* VMS */

    if (ii_sysconf(IISYSCONF_PROCESSORS, &Cs_numprocessors))
	Cs_numprocessors = 1;
    if (ii_sysconf(IISYSCONF_MAX_SEM_LOOPS, &Cs_max_sem_loops))
	Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
    if (ii_sysconf(IISYSCONF_DESCHED_USEC_SLEEP, &Cs_desched_usec_sleep))
	Cs_desched_usec_sleep = DEF_DESCHED_USEC_SLEEP;

    /* No MO in pseudo-servers */

    CS_mo_init();
    (void) MOclassdef( MAXI2, CS_int_classes );

    /* pass semaphores used to synchronize access to message files */
    ERinit(ER_SEMAPHORE, CSMTp_semaphore, CSMTv_semaphore,
	   CSMTi_semaphore, CSMTn_semaphore);

    /* Allow stack dumps to be printed to errlog.log */
    Ex_print_stack = CS_dump_stack;
    Ex_diag_link = CSdiag_server_link;

    pSCB = (CS_SCB *)&Cs_admin_scb;

    Cs_srv_block.cs_known_list	     = &Cs_known_list_hdr;

    Cs_known_list_hdr.cs_next        = pSCB;
    Cs_known_list_hdr.cs_prev        = pSCB;
    Cs_known_list_hdr.cs_type        = CS_SCB_TYPE;
    Cs_known_list_hdr.cs_owner       = (PTR) CS_IDENT;
    Cs_known_list_hdr.cs_tag         = CS_TAG;
    Cs_known_list_hdr.cs_stk_size    = 0;
    Cs_known_list_hdr.cs_state       = CS_COMPUTABLE;
    Cs_known_list_hdr.cs_self        = (CS_SID)&Cs_known_list_hdr;
# if defined(DCE_THREADS)
    CS_thread_id_assign( Cs_known_list_hdr.cs_thread_id, Cs_thread_id_init);
# else
    Cs_known_list_hdr.cs_thread_id   = 0;
# endif /* DCE_THREADS */
    Cs_known_list_hdr.cs_client_type = 0;

    pSCB->cs_type        = CS_SCB_TYPE;
    pSCB->cs_owner       = (PTR) CS_IDENT;
    pSCB->cs_tag         = CS_TAG;
    if ( pSCB->cs_length == 0 )
        pSCB->cs_length  = sizeof(CS_SCB);
    pSCB->cs_state       = CS_COMPUTABLE;
    pSCB->cs_self        = CS_ADDER_ID;
    CS_thread_id_assign(pSCB->cs_thread_id, CS_get_thread_id());
    pSCB->cs_os_pid      = CS_get_os_pid();
    pSCB->cs_os_tid      = CS_get_os_tid();
    pSCB->cs_next        = &Cs_known_list_hdr;
    pSCB->cs_prev        = &Cs_known_list_hdr;
    pSCB->cs_client_type = 0;
    pSCB->cs_thread_type = CS_INTRNL_THREAD;
    pSCB->cs_pid         = Cs_srv_block.cs_pid;
    pSCB->cs_priority    = CS_PADMIN;

    MEmove(12,
           " <admin job> ",
           ' ',
           sizeof(pSCB->cs_username),
           pSCB->cs_username);
	  

#ifdef VMS
    {
        char username[31+1];
        i4 l = min(sizeof(username)-1, sizeof(pSCB->cs_username));
        STlcopy (pSCB->cs_username, username, l);
        username[l] = 0;
        pthread_setname_np(pthread_self(), username, 0);
    }
#endif

    if (ccb == NULL)
    {	/* a pseudo-server */
	/* initialize events for pseudo-servers */
	status = CS_event_init(argc ? *argc : 0, 0);
	if (status)
	    TRdisplay("CSMTinitiate: Error (%x) in CS_event_init\n", status);
	else
	{
	    /* Allocate and init a event wakeup block from shared memory */
	    status = CSMT_get_wakeup_block(pSCB);
	    if (status)
		TRdisplay("CSMTinitiate: Error (%x) in CSMT_get_wakeup_block\n", status);
	}
	if (status)
	    status = E_CS0038_CS_EVINIT_FAIL;

        CS_synch_unlock( &CsMultiTaskSem );
	return(status);
    }

    /* set admin job priority here */

    CS_thread_setprio( pSCB->cs_thread_id, pSCB->cs_priority, &status );

    /* Set up known element trees for SCBs and Conditions.
       Semaphore were handled by CS_mo_init before the
       pseudo-server early return so there sems would
       be known. */

    CS_synch_init( &Cs_srv_block.cs_scb_mutex );
    SPinit( &Cs_srv_block.cs_scb_tree, CS_addrcmp );
    Cs_srv_block.cs_scb_ptree = &Cs_srv_block.cs_scb_tree;
    Cs_srv_block.cs_scb_tree.name = "cs_scb_tree";
    MOsptree_attach( Cs_srv_block.cs_scb_ptree );

    CS_synch_init( &Cs_srv_block.cs_cnd_mutex );
    SPinit( &Cs_srv_block.cs_cnd_tree, CS_addrcmp );
    Cs_srv_block.cs_cnd_ptree = &Cs_srv_block.cs_cnd_tree;
    Cs_srv_block.cs_cnd_tree.name = "cs_cnd_tree";
    MOsptree_attach( Cs_srv_block.cs_cnd_ptree );

    CS_scb_attach( pSCB );

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
    cssb->cs_format = ccb->cs_format;
    cssb->cs_diag = ccb->cs_diag;
    cssb->cs_facility = ccb->cs_facility;
    cssb->cs_max_sessions = ccb->cs_scnt;
    cssb->cs_max_active = ccb->cs_ascnt;
    cssb->cs_stksize = 65536; /* ignoring ccb->cs_stksize */
    cssb->cs_stkcache = ccb->cs_stkcache;
    cssb->cs_max_ldb_connections = 0;
    cssb->cs_quantum = STATSWITCH;
    cssb->cs_isstar = 0;
    cssb->cs_get_rcp_pid = ccb->cs_get_rcp_pid;
    cssb->cs_scbattach = ccb->cs_scbattach;
    cssb->cs_scbdetach = ccb->cs_scbdetach;
    cssb->cs_format_lkkey = ccb->cs_format_lkkey;

    /* 
    ** Get CS options 
    */

    if (status = CSoptions( cssb ))
    {
        CS_synch_unlock( &CsMultiTaskSem );
	return (status);
    }

    /*
    ** Now do the post option massaging of the option values.
    ** Round stack size to page size. 
    */

    if(cssb->cs_quantum < MINSWITCH )
	cssb->cs_quantum = MINSWITCH;

    if( cssb->cs_max_active > cssb->cs_max_sessions )
	cssb->cs_max_active = cssb->cs_max_sessions;

    cssb->cs_stksize = (cssb->cs_stksize + ME_MPAGESIZE-1) & ~(ME_MPAGESIZE-1);
    CS_set_default_stack_size(cssb->cs_stksize, cssb->cs_stkcache);

    /* be sure fds 0, 1, and 2 are not corruptible */

    i = open("/dev/null", 2);

    while (i < 2 && i >= 0)
	i = dup(i);

    if (i > 2)
	close(i);

    /*
    ** Increase file descriptor table to the maximum for the
    ** server, since file opens will be done here instead of in
    ** slaves.
    */
    i = iiCL_increase_fd_table_size( TRUE, 0 );

    if( i < cssb->cs_max_sessions )
    {
	(*Cs_srv_block.cs_elog)( E_CS002E_CS_SHORT_FDS, (CL_ERR_DESC *)0, 1,
			sizeof( i ), (PTR)&i );
	cssb->cs_max_sessions = i;
    }

    /* Reinitialize TLS if max sessions has changed */
    if (cssb->cs_max_sessions != ccb->cs_scnt)
    {
	CSMT_tls_init(cssb->cs_max_sessions, &status);
	CSMT_tls_get( (PTR *)&ppSCB, &status );
	if ( ppSCB == NULL )
	{
	    ppSCB = (CS_SCB **) MEreqmem(0,sizeof(CS_SCB **),TRUE,NULL);
	    CSMT_tls_set( (PTR)ppSCB, &status );
	}
	*ppSCB = (CS_SCB *)&Cs_admin_scb;
    }

    CS_synch_init( &CsStackSem );

#ifdef  USE_IDLE_THREAD
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
    idle_scb->cs_priority = CS_PADMIN;
    idle_scb->cs_thread_type = CS_INTRNL_THREAD;
    idle_scb->cs_pid = Cs_srv_block.cs_pid;
    idle_scb->cs_self = (CS_SID)idle_scb;
    /* Use local wakeup block embedded in SCB */
    CSMT_get_wakeup_block(idle_scb);

    MEmove( 12,
	    " <idle job> ",
	    ' ',
	    sizeof(idle_scb->cs_username),
	    idle_scb->cs_username);

    idle_scb->cs_stk_size = Cs_srv_block.cs_stksize;

    idle_scb->cs_next = Cs_srv_block.cs_known_list;
    idle_scb->cs_prev = Cs_srv_block.cs_known_list->cs_prev;
    Cs_srv_block.cs_known_list->cs_prev->cs_next = idle_scb;
    Cs_srv_block.cs_known_list->cs_prev          = idle_scb;
    /* Count the IdleThread in cs_num_sessions */
    Cs_srv_block.cs_num_sessions = 1;

#if defined(rux_us5)
    CS_create_thread( idle_scb->cs_stk_size, 
                      CSMT_setup,
                      idle_scb, &idle_scb->cs_thread_id,
                      CS_DETACHED, &status );
#elif defined(any_hpux)
    CS_create_thread( idle_scb->cs_stk_size, NULL, 
		      CSMT_setup,
                      idle_scb, &idle_scb->cs_thread_id, 
		      CS_DETACHED,
		      Cs_srv_block.cs_stkcache,
		      &status );
#else
    CS_create_thread( idle_scb->cs_stk_size, NULL,
                      (void *)CSMT_setup,
                      idle_scb, &idle_scb->cs_thread_id,
                      CS_DETACHED,
                      &status );
#endif
# if defined(DCE_THREADS)
    if ( !CS_thread_id_equal( idle_scb->cs_thread_id, Cs_thread_id_init ) )
# else
    if (idle_scb->cs_thread_id)
# endif /* DCE_THREADS */
    {
        /* set thread priority here */

        CS_scb_attach( idle_scb );
    }
#endif /* USE_IDLE_THREAD */

    /* The ticker start-up isn't interlocked.  Init the ticker output to
    ** zero, to cover the wildly remote possibility of a cancel check
    ** happening before the ticker can start maintaining the clock.
    */
    Cs_srv_block.cs_ticker_time = 0;

    /* Create and start the wall-clock ticker thread.  This thread doesn't
    ** do anything other than keep time, so it's not recorded in the
    ** known-sessions list, or counted.  (It's not even necessary to shut
    ** it down cleanly at termination!)
    */
    cs_ticker_scb.cs_type = CS_SCB_TYPE;
    cs_ticker_scb.cs_client_type = 0;
    cs_ticker_scb.cs_owner = (PTR)CS_IDENT;
    cs_ticker_scb.cs_tag = CS_TAG;
    cs_ticker_scb.cs_length = sizeof(CS_SCB);
    cs_ticker_scb.cs_stk_area = 0;
    cs_ticker_scb.cs_stk_size = Cs_srv_block.cs_stksize;
    cs_ticker_scb.cs_state = CS_COMPUTABLE;
    cs_ticker_scb.cs_priority = CS_PADMIN;
    cs_ticker_scb.cs_thread_type = CS_INTRNL_THREAD;
    cs_ticker_scb.cs_pid = Cs_srv_block.cs_pid;
    cs_ticker_scb.cs_self = (CS_SID)&cs_ticker_scb;
    cs_ticker_scb.cs_evcb = NULL;

    MEmove( 14,
	    "<clock ticker>",
	    ' ',
	    sizeof(cs_ticker_scb.cs_username),
	    cs_ticker_scb.cs_username);

#if defined(rux_us5)
    CS_create_thread( cs_ticker_scb.cs_stk_size, 
                      CSMT_ticker,
                      &cs_ticker_scb, &cs_ticker_scb.cs_thread_id,
                      CS_DETACHED, &status );
#elif defined(any_hpux)
    CS_create_thread( cs_ticker_scb.cs_stk_size, NULL, 
		      CSMT_ticker,
                      &cs_ticker_scb, &cs_ticker_scb.cs_thread_id, 
		      CS_DETACHED,
		      Cs_srv_block.cs_stkcache,
		      &status );
#else
    CS_create_thread( cs_ticker_scb.cs_stk_size, NULL,
                      (void *)CSMT_ticker,
                      &cs_ticker_scb, &cs_ticker_scb.cs_thread_id,
                      CS_DETACHED,
                      &status );
#endif

    Cs_stk_list_hdr.cs_next = Cs_stk_list_hdr.cs_prev =
        &Cs_stk_list_hdr;
    Cs_stk_list_hdr.cs_size = 0;
    Cs_stk_list_hdr.cs_begin = Cs_stk_list_hdr.cs_end =
            Cs_stk_list_hdr.cs_orig_sp = 0;
    Cs_stk_list_hdr.cs_used = (CS_SID)-1;
    Cs_stk_list_hdr.cs_ascii_id = CS_STK_TAG;
    Cs_stk_list_hdr.cs_type = CS_STK_TYPE;
    cssb->cs_stk_list = &Cs_stk_list_hdr;

    return(OK);
}

/*{
** Name: CSMTterminate()	- Prepare to and terminate the server
**
** Description:
**	This routine is called to prepare for and/or perform the shutdown
**	of the server.	This routine can be called to inform the server that
**	shutdown is pending so new connections should be disallowed, or that
**	shutdown is to be performed immediately, regardless of state.
**	The latter is intended to be used in cases of major failure's which
**	prohibit the continued operation of the server.	 The former is used in
**	the normal course of events, when a request to shutdown has been issued
**	from the monitor task.	There is, actually, a third category which is
**	"between" these two, which is to shutdown immediately iff there are
**	no current threads.
**
**	This routine operates as follows.  If the request is to gradually shut
**	the server down, then the server state is marked as CS_SHUTDOWN.  This
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
**	mode				one of
**					    CS_CLOSE
**					    CS_CND_CLOSE
**					    CS_START_CLOSE
**					    CS_KILL
**					or  CS_CRASH
**					as described above.
**
** Outputs:
**	active_count			filled with number of connected threads
**					if the CS_CLOSE or CS_CND_CLOSE are requested
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
**	28-Oct-1986 (fred)
**	    Initial Definition.
**	29-Oct-1987 (fred)
**	    Fixed so that internal sessions will not be removed
**	20-sep-1988 (rogerk)
**	    Added CS_START_CLOSE closing state.
**	23-Oct-1992 (daveb)
**	    prototyped
**	13-sep-196 (canor01)
**	    When forcing the shutdown of the server, make an attempt to
**	    kill off the user threads first.
**	31-oct-1996 (canor01)
**	    When a thread is in the final stages of shutdown, increase its
**	    priority to the max to increase the odds that it can complete
**	    its work before any errant running thread can cause trouble.
**	11-nov-1996 (canor01)
**	    Protect cs_user_sessions from simultaneous access.
**	02-may-1997 (canor01)
**	    If there are any threads with the CS_CLEANUP_MASK set, they
**	    should be allowed to die before signaling any of the other
**	    system threads.
**	20-Aug-1997 (kosma01)
**	    Created variable tid of type POSIX thread id, to correspond
**	    to variable sid, the AIX compiler complained of illegal integer
**	    to pointer assignments and comparisons.
**	25-Mar-1998 (jenjo02)
**	    Defer statistics dumping until we're really going to shut down
**	    to keep them from being dumped more than once.
**	21-Oct-2009 (smeke01) b122765
**	    Corrected active_count assignment.
*/
STATUS
CSMTterminate(i4 mode, i4  *active_count)
{
    i4			status = 0;
    CS_SCB		*my_scb;
    CS_SCB		*scb;

    /* Who's calling terminate? */
    CSMTget_scb(&my_scb);

    CS_synch_lock(&CsMultiTaskSem);
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
	    if (active_count != NULL)
	    {
		CSMTp_semaphore(1, &Cs_known_list_sem);
                *active_count = Cs_srv_block.cs_user_sessions;
		CSMTv_semaphore(&Cs_known_list_sem);
		if ( *active_count > 0 )
	        {
		    CS_synch_unlock(&CsMultiTaskSem);
		    return(E_CS0003_NOT_QUIESCENT);
	        }
	    }
	    /* fall through */

	case CS_START_CLOSE:
            /*
             * If FINALSHUT is on, then we've been here before
             *   so, just return.
             */
            if (Cs_srv_block.cs_mask & CS_FINALSHUT_MASK)
	    {
		CS_synch_unlock(&CsMultiTaskSem);
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
            CSMTp_semaphore(1, &Cs_known_list_sem);
            for (scb = Cs_srv_block.cs_known_list->cs_prev;
                scb != Cs_srv_block.cs_known_list;
                scb = scb->cs_prev)
            {
                if (scb->cs_cs_mask & CS_CLEANUP_MASK)
		{
		    if ( (scb->cs_mask & CS_ABORTED_MASK) == 0 )
                        CSMTattn_scb(CS_SHUTDOWN_EVENT, scb);
                    CSMTv_semaphore(&Cs_known_list_sem);
	            CS_synch_unlock(&CsMultiTaskSem);
                    return (E_CS0003_NOT_QUIESCENT);
		}
            }
            CSMTv_semaphore(&Cs_known_list_sem);

	    /*
	    ** Interrupt any server threads with the shutdown event -
	    ** then return to caller.  Server should be shut down when
	    ** last server thread has gone away.
	    **
	    ** Turn off AST's so the scb list will not be mucked with
	    ** while we are cycling through it.
	    */
	    CSMTp_semaphore(1, &Cs_known_list_sem);
	    for (scb = Cs_srv_block.cs_known_list->cs_prev;
		scb != Cs_srv_block.cs_known_list;
		scb = scb->cs_prev)
	    {
		if (scb != &Cs_idle_scb)
		    CSMTattn_scb(CS_SHUTDOWN_EVENT, scb);
	    }
	    CSMTv_semaphore(&Cs_known_list_sem);

	    Cs_srv_block.cs_mask |= CS_FINALSHUT_MASK;

#ifdef USE_IDLE_THREAD
	    /* If this isn't the IdleThread calling, wake it up */
	    if (my_scb != &Cs_idle_scb)
		CS_thread_kill( Cs_idle_scb.cs_thread_id, SIGUSR2 );
#endif /* USE_IDLE_THREAD */

	    /*
	    ** Return and wait for server tasks to terminate before shutting
	    ** down server.
	    */
	    if (Cs_srv_block.cs_num_sessions > 0)
	    {
		CS_synch_unlock(&CsMultiTaskSem);
		return(E_CS0003_NOT_QUIESCENT);
	    }

	    /*
	    ** All threads but the Admin thread, including the Idle Thread,
	    ** have gone away, continue with shutdown.
	    */
	    break;

	case CS_KILL:
	case CS_CRASH:
            /*    
             *  KILL is called by the IdleThread when it
             *  has been signalled and has ended.
             *  KILL also is called by iimonitor to shut immediately.
             *  The difference is that in the first case all
             *  the threads have disappeared, and the final 
             *  cleanup should be done.
             *  In the second case the threads are to be removed.
             */
            if ((Cs_srv_block.cs_mask & CS_FINALSHUT_MASK) &&
                (Cs_srv_block.cs_num_sessions > 0))
	    {
		CS_synch_unlock(&CsMultiTaskSem);
                return (E_CS0003_NOT_QUIESCENT);   
	    }
            Cs_srv_block.cs_mask |= CS_SHUTDOWN_MASK;
            Cs_srv_block.cs_mask |= CS_FINALSHUT_MASK;

	    Cs_srv_block.cs_state = CS_TERMINATING;
	    break;

	default:
	    CS_synch_unlock(&CsMultiTaskSem);
	    return(E_CS0004_BAD_PARAMETER);
    }

    Cs_srv_block.cs_mask |= CS_NOSLICE_MASK;
    /*Cs_srv_block.cs_state = CS_INITIALIZING;*/	/* to force no sleeping */

    /*
    ** At this point, it is known that the request is to shutdown the server,
    ** and that that request should be honored.	 Thus, proceed through the known
    ** thread list, terminating each thread as it is encountered.  If there
    ** are no known threads, as should normally be the case, this will simply
    ** be a very fast process.
    */

    status = 0;

    if ((Cs_srv_block.cs_num_sessions > 0) &&     
        (Cs_srv_block.cs_state != CS_TERMINATING))
    {
	/* Forced terminate, and there are active user threads */
        Cs_srv_block.cs_state = CS_TERMINATING;

	CSMTp_semaphore(1, &Cs_known_list_sem);

        for (scb = Cs_srv_block.cs_known_list->cs_prev;
	    scb != Cs_srv_block.cs_known_list;
	    scb = scb->cs_prev)
        {
	    /*
	    ** for each known thread, if it is not recognized, ask it to die
	    **
	    ** The Admin thread does not appear on the know list,
	    ** the Idle thread does.
	    */
#ifdef USE_IDLE_THREAD
	    if (scb == (CS_SCB *)&Cs_idle_scb)
	    {
		if (scb != my_scb)
		    CS_thread_kill ( scb->cs_thread_id, SIGUSR2 );
		continue;
	    }
#endif /* USE_IDLE_THREAD */

	    if (mode != CS_CRASH)
	    {
	        status = CSMTremove(scb->cs_self);
	    }
    
	    (*Cs_srv_block.cs_disconnect)(scb);

	    if (status)
	    {
	        (*Cs_srv_block.cs_elog)(status, 0, 0);
	    }

	    /*
	    ** Ensure that the calling thread also ends quickly
	    */
	    if (scb == my_scb)
	        scb->cs_mode = CS_TERMINATE;
        }

	if (active_count)
	{
	    *active_count = Cs_srv_block.cs_user_sessions - 1;
	    if (*active_count < 0)
		*active_count = 0;
	}

        if (Cs_srv_block.cs_user_sessions <= 1)
            status = OK;
        else
            status = E_CS0003_NOT_QUIESCENT;

	CSMTv_semaphore(&Cs_known_list_sem);
	CS_synch_unlock(&CsMultiTaskSem);
	return (status);
    }

    /* Dump server statistics */
    CSMTdump_statistics(TRdisplay);

    if (mode != CS_CRASH)
    {
	/* The Idle thread should not be here */

	/* Change caller's priority to the max */
	my_scb->cs_priority = CS_LIM_PRIORITY;
    	CS_thread_setprio( my_scb->cs_thread_id, my_scb->cs_priority, &status );

	/*
	** Give extant user threads a chance to terminate before we
	** continue with shutdown. If we just CSMTremove() them, we
	** may call the shutdown function before they have a chance
	** to end themselves. The shutdown function frees memory and
	** other resources which the terminating threads count on
	** being there. To avoid getting stuck in here with threads
	** that won't terminate, run through the list a finite
	** number of times, then give up.
	**
	** Note that if the session running this code is the Admin
	** thread (normal shutdown), the number of sessions should be
	** zero at this point. If we're not the Admin thread, we're
	** likely a monitor thread doing a stop server, so the
	** count of "user" sessions will be a minimum of one (ourselves).
	** There's no point in CSMTremove-ing ourselves.
	*/
	{
	    i4		tries_remaining = 10;

	    while (Cs_srv_block.cs_user_sessions > 1 && tries_remaining)
	    {
		CSMTp_semaphore(1, &Cs_known_list_sem);

		for (scb = Cs_srv_block.cs_known_list->cs_prev;
		     scb != Cs_srv_block.cs_known_list;
		     scb = scb->cs_prev)
		{
		    if (scb != my_scb &&
		        scb->cs_thread_type == CS_USER_THREAD &&
		       (scb->cs_mask & CS_DEAD_MASK) == 0)
		    {
			status = CSMTremove(scb->cs_self);
			(*Cs_srv_block.cs_disconnect)(scb);
			if (status)
			    (*Cs_srv_block.cs_elog)(status, 0, 0);
		    }
		}
		/*
		** Unlock the know list, then sleep to give the 
		** threads a chance to terminate.
		*/
		CSMTv_semaphore(&Cs_known_list_sem);
		sleep(1);
		tries_remaining--;
	    }
	}

	/* Signal the Admin thread if it's not us */
	if (my_scb->cs_self != CS_ADDER_ID)
	    CSMTremove( CS_ADDER_ID );

	/*
	** If this is not the Admin thread (normal shutdown) or
	** all threads have ended, complete the shutdown process.
	*/
	if (my_scb->cs_self != CS_ADDER_ID ||
	    Cs_srv_block.cs_num_sessions <= 0)
	{
	    status = (*Cs_srv_block.cs_shutdown)();
	    if (status)
		(*Cs_srv_block.cs_elog)(status, 0, 0);
	    else
		(*Cs_srv_block.cs_elog)(E_CS0018_NORMAL_SHUTDOWN, 0, 0);
	}
    }

    CS_synch_unlock(&CsMultiTaskSem);

    if (my_scb->cs_self != CS_ADDER_ID)
    {
	/* Forced shutdown */
	if (Cs_srv_block.cs_num_sessions <= 0)
	    PCexit(0);
#ifdef LNX
	exit(0);
#else
	_exit(0);
#endif
    }
    
    /* Normal shutdown via Admin thread */
    if (Cs_srv_block.cs_num_sessions <= 0)
	return(status);
    return(E_CS0003_NOT_QUIESCENT);
}

/*{
** Name: CSMTdispatch()	- Begin multi-user operation
**
** Description:
**	This routine is called to actually begin the operation of the
**	server.	 It is separated from CSinitiate() so that the server
**	code can regain control in the middle to perform any logging or
**	runtime initialization which it desires to do, or to allow the
**	initiate code to be called once while dispatch is called repeatedly
**	as is the case in some environments.
**
**	The routine operates as follows.  First the dispatcher begins
**	to initialize itself to be ready to add threads.  It builds
**	session control blocks for the dispatcher special threads (if any).
**	At the moment, this is limited to the administrative thread, which
**	is used to start and stop threads.
**
**	When the dispatcher is ready to accept addthread requests, it calls
**	the user supplied initialize server routine (no parameters).
**	It is assumed that a non-zero return indicates failure.	 In the
**	event of failure, the error returned is logged, along with the
**	a message indicating failure to start up.  Then the user supplied
**	termination routine is called, thus allowing any necessary cleanup
**	to occur.
**
**	If the initiation routine succeeds (i.e. returns zero (0)), then the
**	communication subsystem is called to begin operation.
**
**	The dispatcher then begins operation by causing the idle job to be
**	resumed.  The idle job is a thread like any other which operates at
**	a low priority, sleeping when the server has nothing else to do.
**
**	If the dispatcher/idle thread notices that something nasty has occurred,
**	then it will break out of its normal operational loop.	Here, there is
**	nothing to do but shutdown, so the user supplied shutdown routine is
**	called, and a message logged that the server is dying off.  Then, the
**	exit handler is deleted and the process exits.
**
** Inputs:
**	none
**
** Outputs:
**	none
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
**	28-Oct-1986 (fred)
**	    Created.
**	26-sep-1988 (rogerk)
**	    Fill in new cs_info_cp priority fields.
**	20-oct-1988 (rogerk)
**	    Turn off ast's before calling CS_quantum, not after.
**	31-oct-1988 (rogerk)
**	    Don't put startup options that are CS specific (those in which
**	    cso_facility == CSO_F_CS) into the server's startup option list.
**	 7-nov-1988 (rogerk)
**	    Call CS_ssprsm directly instead of through an AST.
**      12-jun-1989 (rogerk)
**          Set cs_pid in admin thread scb.
**	 6-jul-1989 (rogerk)
**	    Moved place where server state is set to CS_IDLING down to after
**	    the server startup routines are run.  This is because lock requests
**	    may occur during server startup which will cause the startup
**	    routines to suspend.  The server must be in CS_INITIALIZING state
**	    when this happens until everything is ready to run other threads.
**       7-Aug-1989 (FPang)
**          Put up SIGPIPE exception handler in CSdispatch for servers
**          that don't use di (i.e. STAR).
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**	6-jul-92 (daveb)
**	    Change for(;;){} to do{} while( false ) to shut up Sun acc warning.
**	23-Oct-1992 (daveb)
**	    prototyped
**	23-aug-1993 (andys)
**          In CS_dispatch call CS_cleanup_wakeup_events to clean up
**          any events left over from another process with the same pid.
**	    [bug 52761 in 6.4, this code is very different]
**	    Tidy up history.
**	18-oct-1993 (bryanp)
**	    New argument to CS_cleanup_wakeup_events to indicate whether caller
**	    already holds the system segment spinlock.
**	3-may-1995 (hanch04)
**	    Added changes made by nick
**          Changed entry point into CS_su4_setup() for su4_us5 to a known,
**          non-variable value.
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	19-sep-1995 (nick)
**	    Define alternate stack for stack overflow trapping.
**  12-nov-96 (kinpa04)
**      On axp_osf, 10 pages are needed on the altinate stack to handle
**      a SEGV produced by an overflow on the regular stack.
**	16-sep-1996 (canor01)
**	    If the server is already in a CS_TERMINATING state, don't
**	    try to call CSterminate again on exit.
**	10-oct-1996 (canor01)
**	    Make call to GC_set_blocking() in CSdispatch() to let GC layer
**	    know this is a multi-threaded server using blocking I/O.
**	12-nov-1996 (canor01)
**	    Allow multi-threaded server to use iiCLpoll() instead of
**	    blocking I/O.
**	14-nov-1996 (canor01)
**	    Release system mutexes on failed server startup.
**      06-mar-1997 (reijo01)
**          Changed II_DBMS_SERVER message to use generic system name.
**	20-May-1997 (jenjo02)
**	    Moved MUset_funcs() to before NMstIngAt() call so that
**	    the MU semaphores will be converted before any threads
**	    are dispatched.
**	27-May-1997 (jenjo02)
**	    Removed call to MUset_funcs(). With OS threads, MU uses perfectly good
**	    opsys mutexes which need no conversion to CS_SEMAPHORE type. The 
**	    conversion was causing a race condition which is avoided by not doing
**	    the unnecessary conversion. Also removed superfluous TMnow() call which
**	    was planted here solely to convert a TM semaphore.
**      23-may-1997 (reijo01)
**          Print out II_DBMS_SERVER message only if there is a listen address.
**	20-Aug-1997 (kosma01)
**	    Initialize the CS accounting semaphore.
**      23-sep-1997 (canor01)
**          Print startup message to stderr.
**	16-Nov-1998 (jenjo02)
**	    CsMultiTaskSem is held on entry, blocking all threads in
**	    CS_setup until we release it.
**	03-Oct-2002 (somsa01)
**	    To be consistent with the way we print out "PASS" and "FAIL" in
**	    CSdispatch(), print out to stdout.
**	21-May-2003 (bonro01)
**	    Add support for HP Itanium (i64_hpu)
**	4-mar-04 (toumi01)
**	    Init Cs_utility_sem (new for CSadjust_counter function).
**	7-oct-2004 (thaju02)
**	    Change npages to SIZE_TYPE.
**	6-Jul-2005 (schka24)
**	    Remove utility sem init, we need it earlier.
**      13-jan-2004 (wanfr01)
**          Bug 64899, INGSRV87
**          Enabled Alternate stack for AIX so large in clauses won't
**          kill the server
**      26-Nov-2009 (hanal04) Bug 122938
**          Add missing call to FP_set_callback() so that FP_get_scb()
**          will run correctly for an OS threaded DBMS.
**
**  Design:
**
**	The server begins as follows:
**	the system state is set to idle, ast's are enabled, and
**	Control is transfered to the idle thread, which is a special case
**	in CSHL\CS_setup(). Here, the Vax/VMS service sys$hiber() is called.
**	At this point the code is quite simple.	 The remainder constitutes the
**	idle task, which is a task of very, very low priority.	It normally
**	sits hibernating.  When events in the system occur, AST's are delivered
**	to the process.	 These notice that a hibernate has taken place, and then
**	awaken the process (via sys$wake()).
**
**	Thus, the remainder of the idle thread code is to enable user mode
**	ast's, set the state to idle, and hibernate.  Upon reawakening,
**	it is known that the AST level routine which awakened the process,
**	must have placed something to do in queue, so an AST is declared
**	to change the operating thread.	 At some point, the idle task
**	will again become eligible for execution, and control will return
**	to the point after the $DCLAST() call.	AT this point, some checks
**	are made to assure us that nothing untoward has happened (such as
**	a request for shutdown having happened).  Assuming that nothing out
**	of the ordinary has occurred, control returns to the top of the loop
**	(which is the top of this paragraph), and the idle task takes the
**	process out of competition for system resources.
**
*/
STATUS
CSMTdispatch(void)
{
    STATUS		status = 0;
    STATUS		ret_val;
    i4			i;
    EX_CONTEXT		excontext;
    CS_INFO_CB		csib;
    FUNC_EXTERN STATUS	cs_handler(EX_ARGS *);
    CSSL_CB		*slcb;
    SYSTIME		stime;
    CS_SCB		*AdminSCB = (CS_SCB *)&Cs_admin_scb;


    if (Cs_srv_block.cs_process == 0)
    {
	/* We're going to exit, so release mutexes */
	CS_synch_unlock( &CsMultiTaskSem );
	return(FAIL);
    }

/* Altstack is declared in CSMTsetup for Linux */
#if (defined(xCL_077_BSD_MMAP) || defined(any_aix)) && !defined(LNX)
    {
	PTR		    ex_stack;
	SIZE_TYPE	    npages;
	CL_ERR_DESC	    err_code;

#if defined(i64_hpu)
	if (MEget_pages(ME_MZERO_MASK, 64, 0, &ex_stack, &npages, 
#else
	if (MEget_pages(ME_MZERO_MASK, 10, 0, &ex_stack, &npages, 
#endif
		&err_code) != OK)
	    return(FAIL);

    	if (EXaltstack(ex_stack, (npages * ME_MPAGESIZE)) != OK)
	{
	    (void)MEfree_pages(ex_stack, npages, &err_code);
	    /* We're going to exit, so release mutexes */
	    CS_synch_unlock( &CsMultiTaskSem );
	    return(FAIL);
	}
    }
# endif /* ifdef xCL_077_BSD_MMAP */

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
    ret_val = CSMTw_semaphore( &Cs_known_list_sem,
                             CS_SEM_SINGLE,
                             "Cs_known_list_sem");

    if (ret_val) 
    {
	/* We're going to exit, so release mutexes */
	CS_synch_unlock( &CsMultiTaskSem );
        return FAIL;
    }

    ret_val = CSMTw_semaphore( &CS_acct_sem,
                             CS_SEM_SINGLE,
                             "Cs_acct_sem");
    if (ret_val) 
    {
	/* We're going to exit, so release mutexes */
	CS_synch_unlock( &CsMultiTaskSem );
        return FAIL;
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

#if defined(USE_BLOCKING_IO) && !defined(VMS)
    GC_set_blocking(TRUE);	/* use blocking I/O */
# endif /* USE_BLOCKING_IO */

    /* declare shutdown signals here */
# ifdef SIGEMT
    /* doesn't exist on AIX/RT */
    i_EXsetothersig(SIGEMT, CSsigterm);
# endif
# ifdef SIGDANGER
    /* But this does... */
    i_EXsetothersig(SIGDANGER, CSsigdanger);
# endif
    i_EXsetothersig(SIGQUIT, CSsigterm);
    i_EXsetothersig(SIGTERM, CSsigterm);
    i_EXsetothersig(SIGHUP, CSsigterm);
    i_EXsetothersig(SIGINT, SIG_IGN);
    i_EXsetothersig(SIGPIPE, SIG_IGN);
#if defined(rmx_us5) || defined(rux_us5)
    i_EXsetothersig(SIGVTALRM, SIG_IGN);
#endif

    do
    {
        FP_set_callback( CSMTget_scb );

	ret_val = CSinstall_server_event(&slcb, 0, 0, CS_cpres_event_handler);
	if (ret_val != OK)
	    break;

	/*
	** Cleanup any wakeup events hanging around from a previous server
	*/
	CS_cleanup_wakeup_events(Cs_srv_block.cs_pid, (i4)0);

	/* Allocate and init a event wakeup block for the Admin thread */
	ret_val = CSMT_get_wakeup_block(AdminSCB);
	if (ret_val != OK)
	    break;

	/*
	** Make sure CS is ready at this point to start accepting
	** CSadd_thread calls.	The server startup routines may need
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
	csib.cs_name[0] = EOS;

	ret_val = (*Cs_srv_block.cs_startup)(&csib);
	if (ret_val != OK)
	{
	    SIstd_write(SI_STD_OUT, "FAIL\n");
	    /* We're going to exit, so release mutexes */
            CS_synch_unlock( &CsMultiTaskSem );
	    break;
	}

	STncpy( Cs_srv_block.cs_name, csib.cs_name,
		sizeof(Cs_srv_block.cs_name) - 1);
	Cs_srv_block.cs_name[ sizeof(Cs_srv_block.cs_name) - 1 ] = EOS;

	if (Cs_srv_block.cs_define)
	{
#if defined(VMS)
	    int         jobtype;
	    int		master_pid;
	    IOSB	iosb;
	    ILE3	jpiget[] = {
		{ sizeof(jobtype), JPI$_JOBTYPE, &jobtype, 0 },
		{ sizeof master_pid, JPI$_MASTER_PID, &master_pid, 0 },
		{ 0, 0, 0, 0 }
	    };
	    static $DESCRIPTOR( jobtabnam, "LNM$JOB" );
            static $DESCRIPTOR( systabnam, "LNM$SYSTEM" );
	    static $DESCRIPTOR( lognam, "II_DBMS_SERVER" );
	    ILE3 itmlst[2] = { { 0, LNM$_STRING, NULL, NULL },
			    { 0, 0, NULL, NULL } };

	    status = sys$getjpiw(EFN$C_ENF, 0, 0, jpiget, &iosb, 0, 0);
	    if (status & 1)
		status = iosb.iosb$w_status;
	    if ((status & 1) == 0)
		sys$exit(status);

	    itmlst[0].ile3$w_length = STlength( csib.cs_name );
	    itmlst[0].ile3$ps_bufaddr = csib.cs_name;

	    /*
	    ** Define a system logical only when we are currently running
	    ** detached (or, in a bogus DETNET DECterm session!) and at the
	    ** top of the current job (not a process spawned by the debugger
	    ** or iirundbms).
	    */
	    if (jobtype == JPI$K_DETACHED && Cs_srv_block.cs_pid == master_pid)
		status = sys$crelnm(NULL, &systabnam, &lognam, NULL, itmlst);
	    else
		status = sys$crelnm(NULL, &jobtabnam, &lognam, NULL, itmlst);
	    if (!(status & 1))
		sys$exit(status);
#else
	    if (NMstIngAt("II_DBMS_SERVER", csib.cs_name) != OK)
	        (*Cs_srv_block.cs_elog)(E_CS002D_CS_DEFINE, (CL_ERR_DESC *)0, 0);
#endif
	}

	CS_set_server_connect(csib.cs_name);

        /*
        ** Echo listen address so iirundbms knows we're alive,
        ** but only if there is a listen address.
        */
        if (csib.cs_name[0])
        {
	    char	buffer[512];

	    STprintf(buffer, "\n%s_DBMS_SERVER = %s\n", SystemVarPrefix,
		     csib.cs_name);
	    SIstd_write(SI_STD_OUT, buffer);
	    SIstd_write(SI_STD_OUT, "PASS\n");
        }

	/*								    */
	/* Memory pointers are saved and restored, because mainline code    */
	/* depends on this.  On Vax/VMS systems, this dependency is	    */
	/* unnecessary, but it is necessary on MVS where all the threads    */
	/* are really separate tasks sharing an address space.	Possibly    */
	/* useful in some UNIX implementations as well, but not this one.   */

	Cs_srv_block.cs_svcb = csib.cs_svcb;	    /* save pointer to mem */

	/*
	** Initialize the CL semaphore system.
	** Save the address of the mainline ME page allocator semaphore and
	** setup the semaphore system to use CSsemaphore routines instead of
	** dummy routines.
	*/
	ME_page_sem = csib.cs_mem_sem;
	gen_useCSsems(CSMTp_semaphore, CSMTv_semaphore, 
                      CSMTi_semaphore, CSMTn_semaphore);

	Cs_srv_block.cs_state = CS_PROCESSING;

	/* release any threads waiting in CS_setup */
	CS_synch_unlock( &CsMultiTaskSem );

	/* We are the main task, so let's begin acting like one */

        if (CSMT_admin_task())
        {
            (*Cs_srv_block.cs_elog) (E_CS00FE_RET_FROM_IDLE, 0, 0);
            ret_val = E_CS00FF_FATAL_ERROR;
        }
        else
            ret_val = OK;

        break;

    } while( FALSE );

    if ( ((status & 1) == 0) || (ret_val) )
    {
	if ((status & 1) != 0)
	{
	    status = 0;
	}
	if ( status || ret_val )
	    (*Cs_srv_block.cs_elog)(ret_val, status, 0);
    }

    if ( Cs_srv_block.cs_state != CS_TERMINATING )
        Cs_srv_block.cs_state = CS_CLOSING;

    while (CSMTterminate(CS_KILL, &i))
	sleep(1);
    
    /* Free Admin thread wakeup cb, if any */
    CSMT_free_wakeup_block(AdminSCB);

    PCexit(((status & 1) || (ret_val == 0)) ? 0 : status);
}

/*{
** Name: CSMTsuspend	- Suspend current thread pending some event
**
** Description:
**	This procedure is used to suspend activity for a particular
**	thread pending some event which will be delivered via a
**	CSresume() call.  Optionally, this call will also schedule
**	timeout notification.  If the event is to be resumed following
**	an interrupt, then the CS_INTERRUPT_MASK should be specified.
**
**	A note on IRPENDING vs IRCV, at least as they apply to csmt_unix:
**	IRPENDING implies that an interrupt was delivered to a session that
**	wasn't waiting.  We desire the session to explicitly see an
**	"interrupted" return from a suspend (unless it removes the interrupt
**	notice with CSintr_ack);  so IRPENDING stays on until some wait
**	occurs that can be returned from with interrupt.  (Non-interruptable
**	suspends, or suspends that are already satisfied, don't count.)
**
**	The IRCV flag does almost the same thing, except it's meant for
**	breaking out of a suspend that is already in progress.  IRCV
**	is set if the interrupt finds the session not in a COMPUTABLE
**	state, and with the interrupt mask set.  (Typically this will
**	happen due to a poll check while suspended, although it can also
**	happen if the interrupt is imposed by a 3rd-party session,
**	e.g. query kill.)
**
**	In contrast to other CS implementations, CSMT always sets EDONE
**	as EV_EDONE_MASK in the evcb->event_state.  It NEVER sets
**	CS_EDONE_MASK in the cs_mask, nor does it depend on CS_EVENT_WAIT
**	in the cs_state.  (The latter is set but is not the EDONE
**	indicator.)
**
** Inputs:
**	mask				Mask of events which can terminate
**					the suspension.	 Must be a mask of
**					  CS_INTERRUPT_MASK - can be interrupted
**					  CS_TIMEOUT_MASK - event will time out
**					along with the reason for suspension:
**					(CS_DIO_MASK, CS_BIO_MASK, CS_LOCK_MASK)
**                                        CS_NANO_TO_MASK - timeout values
**                                      are in nanoseconds, zero is zero
**	to_cnt				number of seconds after which event
**					will time out.	Zero means no timeout
**                                      (unless CS_NANO_TO_MASK is set).
**	ecb				Pointer to a system specific event
**					control block.	Can be zero if not used.
**
** Outputs:
**	none
**	Returns:
**	    OK
**	    E_CS0004_BAD_PARAMETER	if mask contains invalid bits or
**					    contains timeout and to_cnt < 0
**	    E_CS0008_INTERRUPTED	if resumed due to interrupt
**					(only allowed if mask includes the
**					CS_INTERRUPT_MASK)
**	    E_CS0009_TIMEOUT		if resumed due to timeout
**	Exceptions:
**	    none
**
** Side Effects:
**	    Task is suspended awaiting completion of event
**
** History:
**	30-Oct-1986 (fred)
**	    Created
**	30-Jan-1987 (fred)
**	    Added ECB parameter for MVS.
**	09-mar-1987 (fred)
**	    Added saving what we are waiting for for the monitor.
**	30-Jan-1990 (anton)
**	    Allow subsecond suspends by passing negative number of
**	    milliseconds.
**	29-jul-1991 (bryanp)
**	    B38197: While running stress tests, a timing-related bug was found:
**	    If a CSsuspend call returned E_CS0008_INTERRUPTED, it was returning
**	    with the thread still on the ready queue, but in CS_EVENT_WAIT
**	    state. The thread was now vulnerable to getting hung: if a quantum
**	    switch occurs before the next time the thread calls CSsuspend, the
**	    thread will be hung, since CS_xchng_thread will never re-schedule
**	    a CS_EVENT_WAIT thread. The fix is simply to ensure that the
**	    thread is in CS_COMPUTABLE state when it returns from CSsuspend.
**	02-jul-92 (swm)
**	    Added entries for dra_us5 (a 486 running SVR4). Copied references
**	    to dr6_us5 for dr6_uv1; dr6_uv1 is the same as dr6_us5 but with
**	    secure extensions to SVR4.
**	23-Oct-1992 (daveb)
**	    prototyped
**	22-jan-1993 (sweeney)
**	    Added usl_us5 (yet another Intel x86 running SVR4).
**	25-apr-94 (mikem)
**	    Bug #58257
**	    During server startup of the rcp following machine/installation
**	    crash the rcp does log file scans while in the CS_INITIALIZING
**	    state.  Before this change all I/O's done during this time were
**	    taking a fixed 200 ms. as the "wakeme" bit was never set, so the
**	    di slaves never woke up the server when they were done.
**      10-july-1995 (thaju02)
**          Modified CSsuspend.  Added timeout usage for single threaded mode.
**	25-jul-95 (allmi01)
**	    Added support for dgi_us5 (DG-UX on Intel based platforms) following
**	    usl_us5 (UNIXWARE)
**      31-july-1995 (thaju02)
**          Modified CSsuspend.  For single threaded mode of execution,
**          added extra check such that we only will timeout if the timeout
**          value specified is non-zero.  This check had to be added 
**          due to the acp server crashing (unable to handle a lock request
**          timeout).
**	01-Dec-1995 (jenjo02)
**	    Defined 2 new wait reasons, CS_LGEVENT_MASK and CS_LKEVENT_MASK
**	    to statistically distinguish these kinds of waits from other
**	    LG/LK waits. Also fixed wait times to use Cs_sm_cb->cs_qcount
**	    instead of Cs_srv_block.cs_quantums which isn't maintained
**	    by any code!
**	16-Jan-1996 (jenjo02)
**	    CS_LOG_MASK is no longer the default wait reason and must
**	    be explicitly stated in the CSsuspend() call.
**	11-nov-1996 (canor01)
**	    Don't let threads be awakened by a "remove session" unless
**	    they are in an interruptible wait state.
**	12-nov-1996 (canor01)
**	    Allow multi-threaded server to use iiCLpoll() instead of
**	    blocking I/O.
**	18-dec-1996 (wonst02)
**	    Allow third parameter to be a ptr to an LK_LOCK_KEY when sleep
**	    mask is CS_LOCK_MASK, so iimonitor can track locks.
**      22-jan-1997 (canor01)
**          Server becomes single-threaded in a CS_TERMINATING state, and
**          must use single-threaded suspend/resume to allow slave events to
**          complete.
**	10-feb-1997 (canor01)
**	    Peek at the socket connections for possible completions
**	    of asynchronous BIO calls (such as expedited channel) on
**	    every CSsuspend().
**      20-mar-1997 (canor01)
**          Add support for timeouts in nanoseconds.
**	25-Mar-1997 (jenjo02)
**	    Fixed bad test of CS_LOCK_MASK in CSMTsuspend().
**	20-May-1997 (jenjo02)
**	   In CSsuspend(), do asych poll only for user threads.
**	22-May-1997 (jenjo02)
**	   Before polling for asych messages, release cs_event_sem. If an asych
**	   message has arrived, it may drive CSMTattn_scb(), which will try
**	   to acquire cs_event_sem.
**	09-Jun-1997 (merja01)
**	   Changed errno value for axp_osf CS_cond_timedwait
**	   from ETIME to ETIMEDOUT.
**	20-Aug-1997 (kosma01)
**	   From merja01, changed errno value for rs4_us5 CS_cond_timedwait
**	   from ETIME to ETIMEDOUT.
**	02-jan-1998 (canor01)
**	    If we get a timeout, set the scb->cs_state back to CS_COMPUTABLE,
**	    since this is not done anywhere else.
**	19-May-1998 (fucch01/kosma01)
**	   Added sgi_us5 to platforms that check for ETIMEDOUT instead of 
**	   ETIME for status of CS_cond_timedwait.  This was causing the 
**	   creation of the iidbdb to hang, preventing the initial ingbuild
**	   from properly completing.
**	24-Sep-1998 (jenjo02)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to account
**	    for those types of suspends.
**	    In CSMTsuspend(), don't do a timed wait if this wait is 
**	    noninterruptable.
**	16-Nov-1998 (jenjo02)
**	    Added new event wait masks to distinguish Log I/O from other
**	    Disk I/O, reads from writes. Corresponding stats added as
**	    well.
**	22-Feb-1999 (schte01)
**       For USE_IDLE_THREAD in CSMTsuspend, test cs_state for
**       CS_INITIALIZING or CS_TERMINATING (indicating a psuedo_server)
**       before possible cond_wait. 
**	01-Apr-1999 (kosma01)
**	   Added sgi_us5 to a second location where platforms that check for
**	   ETIMEOUT instead of ETIME as the status of CS_cond_timed().
**      18-Sep-2000 (hanal04) Bug 102042 Problem INGDBA67.
**         Decrement the active session count (cs_num_active) when
**         suspending user sessions.
**	16-aug-2001 (toumi01)
**	   i64_aix uses ETIMEOUT instead of ETIME
**	24-aug-2001 (abbjo03)
**	    Back out change to tracking of cs_num_active (18-sep-2000) since it
**	    has side effects on subsequent invocations.
**      7-Jan-2002 (wanfr01)
**          Bug 108641, INGSRV 1875
**          Rewrite of fix for bug 100847.
**	    Someone can hang the server by connecting directly to the listen
**          port of the server.  By default the admin thread will wait
**          indefinitely for the connect info.
**          Added code so if the admin thread gets a connect attempt, but 
**          does not receive the connect information from the client it can
**          timeout the connection as per the association_timeout setting
**	    (passed in via the to_cnt parameter)
**      19-aug-2002 (devjo01)
**          Don't set sid in ecb if CS_LKEVENT_MASK.
**	23-jan-2004 (devjo01)
**	    Don't use CAS to update csev_state if not a clustered
**	    build.  Slightly reduces overhead, and avoids need
**	    to code C&S routines for platforms which will not
**	    support clustering.
**	01-Aug-2005 (jenjo02)
**	    csi_wakeme now cleared by CS_find_events while holding
**	    csi_spinlock when nevents == 0.
**	28-Nov-2006 (kiria01) b115062
**	    Replaced a fixed CS_cond_wait with a timed variant plus trace
**	    information to log a potential lost signal causing a thread to
**	    be left in EDONE state.
**	14-Feb-2007 (kschendel)
**	    Remove time accounting based on bogus pseudo-clock.  If someone
**	    decides that the wait times are important, the system highres
**	    timer (if any) should be used.
**	19-Dec-2007 (kiria01) b119662
**	    The incrementing timer was being increased by lots of unexpected
**	    wakeups of the sleeping events causing the timer increment to
**	    overflow get adjusted negatively. Then the condition wait (on
**	    some platforms) returned an error status which was not checked
**	    for.
**	    Now we move the increment so that it is only increased when it
**	    really had personally expired. Additionally we cap the value to
**	    UNINT_TMO_INCR_CAP so it cannot become -ve even if ingres is up
**	    for more than 68 years.
**	    We move the TMet() out of the test loop as there is no need to
**	    reassert the exact time. We extend the checks to break the 'non-
**	    interuptible' case by checking for CS_IRCV_MASK too. This was
**	    omitted before as not possible but on analysis might be valid
**	    in non-USER thread contexts.
**      20-mar-2009 (stegr01)
**          replace TMet() by TMnow()
*/
STATUS
CSMTsuspend(i4 mask, i4  to_cnt, PTR ecb)
{
    CS_SCB		*scb;
    STATUS		rv = OK;
    i4             timeout;
    CS_COND_TIMESTRUCT	ts, to;
    SYSTIME		time_now;
    STATUS		status;
    i4			cevs;

    if ((mask & ~(CS_TIMEOUT_MASK |
		    CS_NANO_TO_MASK |
		    CS_INTERRUPT_MASK |
		    CS_DIO_MASK |
		    CS_BIO_MASK |
		    CS_LOCK_MASK |
		    CS_LOG_MASK |
		    CS_LKEVENT_MASK |
		    CS_LGEVENT_MASK |
		    CS_LIO_MASK |
		    CS_IOR_MASK |
		    CS_IOW_MASK
		  )) != 0)
	return(E_CS0004_BAD_PARAMETER);
#ifdef	CS_STRICT_TIMEOUT
    if ((mask & CS_TIMEOUT_MASK) && (to_cnt < 0))
	return(E_CS0004_BAD_PARAMETER);
#endif

    if (ecb && (mask & (CS_LOCK_MASK|CS_LKEVENT_MASK)) == 0)
    {
	CSMTget_sid((CS_SID*)&((CSEV_CB *)ecb)->sid);
    }
#ifdef USE_IDLE_THREAD
    if (Cs_srv_block.cs_state != CS_INITIALIZING &&
        Cs_srv_block.cs_state != CS_TERMINATING)
  {
#endif /* USE_IDLE_THREAD */
    CSMTget_scb( &scb );

    if ( scb  && scb->cs_evcb )
    {
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
	** Check current event state to see if asynchronous event
	** we are about to suspend for has already completed. 
	**
	** If not, then use atomic compare and swap to set event
	** state to wait.  If event state is successfully set
	** to wait, then 
	*/

#ifdef USE_IDLE_THREAD
	{
	    static i4		reset_wakeme = FALSE;
	    i4			nevents;

	    /* Just check for events, don't reset csi_wakeme if none */
	    CS_find_events(&reset_wakeme, &nevents);
	}
#endif /* USE_IDLE_THREAD */

	CS_synch_lock( &scb->cs_evcb->event_sem );

#ifdef conf_CLUSTER_BUILD
	/*
	** If running clustered Ingres, we have to allow for the
	** possibility of a session being resumed from within a
	** signal handler.  This is because the typical 3rd party
	** DLM implementation will invoke the asynchronous completion
	** function registered when the DLM call was made as an
	** signal handler, and an essential task for this handler
	** is to resume the session presumable suspended waiting
	** for the asynchronous DLM call to complete.  Since we cannot
	** use a mutex to synchronize the update of the cs_evcb,
	** for cluster builds only we introduce an atomic compare
	** and swap for this field.
	*/
	do
	{
	    cevs = scb->cs_evcb->event_state;
	    if (EV_EDONE_MASK & cevs)
	    {
		/* event already posted */
		break;
	    }
	} while ( CScas4(&scb->cs_evcb->event_state, cevs, EV_WAIT_MASK) ); 

#else
	cevs = scb->cs_evcb->event_state;
	if ( !(EV_EDONE_MASK & cevs) )
	{
	    scb->cs_evcb->event_state = EV_WAIT_MASK;
	}
#endif

	if (EV_EDONE_MASK & cevs)
	{
	    /*
	    ** Then there is nothing to do, the event is already done.
	    ** In this case, we can ignore the suspension.
	    ** Leave IRPENDING on if it's set, so that eventually the
	    ** session will see it.  (or, clear it via intr-ack.)
	    */
	    scb->cs_mask &= ~(CS_IRCV_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK 
				    | CS_NANO_TO_MASK | CS_TIMEOUT_MASK);
	    scb->cs_state = CS_COMPUTABLE;

	    /*
	    ** At least one resume has been delivered, so we can simply
	    ** set clear the event state without a compare and swap.
	    ** If Suspend was called in a circumstance in which more
	    ** than one resume may be delivered to the suspending thread,
	    ** it is the callers responsibility to assure that CScancel
	    ** is called after any possibility of the other pending
	    ** resumes being delivered, and before CSsuspend is called
	    ** again, or there will exist the possibility that a
	    ** resume intended for this suspend instance will be applied
	    ** to the next suspend.  This is NOT a restriction added
	    ** by the new CAS logic, but does bear repeating.
	    */
	    scb->cs_evcb->event_state = 0;

	    CS_synch_unlock( &scb->cs_evcb->event_sem );

	    return(OK);
	}
	else if ((scb->cs_mask & CS_IRPENDING_MASK) &&
		(mask & CS_INTERRUPT_MASK))
	{
	    /*
	    **	Then an interrupt is pending.  Pretend we just got it
	    ** 
	    ** Suspend aborted, but resume may not have been received.
	    ** Caller MUST call CScancel after assuring that any pending
	    ** resume has either been received, or has itself been
	    ** aborted.
	    */
	    scb->cs_mask &= ~(CS_IRCV_MASK | CS_IRPENDING_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK 
				    | CS_NANO_TO_MASK | CS_TIMEOUT_MASK);
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_evcb->event_state = 0;

	    CS_synch_unlock( &scb->cs_evcb->event_sem );
	    return(E_CS0008_INTERRUPTED);
	}

	scb->cs_state = CS_EVENT_WAIT;

	/* Interrupt and timeout bits go into cs-mask, what we're waiting
	** for goes into cs-memory.
	** Clear old wait-mask state from cs_mask first as a precaution,
	** although they really should never be on at entry.
	*/
	scb->cs_mask &= ~(CS_INTERRUPT_MASK | CS_TIMEOUT_MASK | CS_NANO_TO_MASK);
	scb->cs_mask |= (mask & (CS_INTERRUPT_MASK | CS_TIMEOUT_MASK |
				  CS_NANO_TO_MASK));
	scb->cs_memory = (mask & (CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
				  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
				  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
				  CS_LOG_MASK | CS_TIMEOUT_MASK));

	/* if doing time accounting, could get start-time here */

	scb->cs_timeout = 0;
	if (mask & CS_TIMEOUT_MASK)
	{
	    if (to_cnt == 0)
	    {
		/* Timeout specified, but no timeout value, ignore timeout */
		scb->cs_memory &= ~(CS_TIMEOUT_MASK | CS_NANO_TO_MASK);
		mask &= ~(CS_TIMEOUT_MASK | CS_NANO_TO_MASK);
	    }
	    else
	    {
		if ( mask & CS_NANO_TO_MASK )
		{
		    scb->cs_timeout = to_cnt / 1000000;
		    if ( to_cnt % 1000000 )
			scb->cs_timeout++;
		    to.tv_sec = to_cnt / 1000000000;
		    to.tv_nsec = to_cnt % 1000000000;
		}
		else
		{
		    i4  msecs;
#ifndef	CS_STRICT_TIMEOUT
		    if (to_cnt < 0)
			scb->cs_timeout = - to_cnt;
		    else
#endif
			scb->cs_timeout = to_cnt * 1000;
		    to.tv_sec = scb->cs_timeout / 1000;
		    msecs = scb->cs_timeout % 1000;
		    to.tv_nsec = msecs * 1000000;
		}
	    }
	}

# if !defined(USE_BLOCKING_IO) && !defined(VMS)  /* use CLpoll */
    /*
    **  This code is not assembled on VMS
    */
        if (mask & CS_BIO_MASK )
	{
	    STATUS status=0;
	    int admin_counter = 0;
	    int admin_connect = 0;
	    timeout = 2000;

	    while ( scb->cs_evcb->event_state == EV_WAIT_MASK &&
		    !(scb->cs_mask & CS_TO_MASK) &&
		    !(scb->cs_mask & CS_IRCV_MASK) &&
                    !((admin_connect == 1) && (status == E_TIMEOUT) && (admin_counter++>=(to_cnt/timeout)))
		)
	    {
		CS_synch_unlock( &scb->cs_evcb->event_sem );
	        status = iiCLpoll(&timeout);
		CS_synch_lock( &scb->cs_evcb->event_sem );

                if (scb->cs_self == CS_ADDER_ID)
                {
                   if ((status != E_TIMEOUT) && (to_cnt))
                    admin_connect = 1;
                }
                else if ( (status == E_TIMEOUT) && scb->cs_timeout)
                {
                   /* Clpoll() has timed out and the caller specified a timeout,
                   ** so decrement the counter, and Timeout if the specified time
                   ** has elapsed.
                   */
                   scb->cs_timeout -= timeout;

                   if (scb->cs_timeout <= 0)
                   {
                      scb->cs_mask |= CS_TO_MASK;
                   }
                }
	    }
            if ((admin_connect) && (status == E_TIMEOUT))
	    {
		TRdisplay ("%@   Admin thread timed out a connect request\n");
                scb->cs_mask |= CS_TO_MASK;
	    }
	}
	else
# endif /* ! USE_BLOCKING_IO */

	{
	    i4 unint_tmo = 2;	/* Increasing timer value */
	    i4 wakeup_bits = CS_TO_MASK;

	    /* Supplement wakup mask if needed */
	    if (mask & CS_INTERRUPT_MASK)
	       wakeup_bits |= CS_IRCV_MASK;

	    if ( mask & CS_TIMEOUT_MASK &&
		    scb->cs_thread_type == CS_USER_THREAD)
	    {
                /* do a quick poll for asynchronous messages */
#ifdef VMS
                CSMT_execute_asts(scb);
#else
	        timeout = 0;
		CS_synch_unlock( &scb->cs_evcb->event_sem );
		VOLATILE_ASSIGN_MACRO(Cs_srv_block.cs_ticker_time, scb->cs_last_ccheck);
	        iiCLpoll(&timeout);
		CS_synch_lock( &scb->cs_evcb->event_sem );
#endif
 	    }

 	    /* Get the time just at the start of the loop */
	    TMet(&time_now);
	    ts.tv_sec  = time_now.TM_secs;
	    ts.tv_nsec = time_now.TM_msecs * 1000000;

	    while ( scb->cs_evcb->event_state == EV_WAIT_MASK &&
		    !(scb->cs_mask & wakeup_bits))
	    {
		if ( mask & (CS_TIMEOUT_MASK | CS_NANO_TO_MASK) )
		{
		    ts.tv_sec  += to.tv_sec;
		    ts.tv_nsec += to.tv_nsec;
		    if ( ts.tv_nsec > 1000000000 ) /* overflow? */
		    {
			ts.tv_nsec %= 1000000000;
			ts.tv_sec += 1;
		    }
#if defined(VMS)
                    while (TRUE)
                    {
                        CSMT_execute_asts(scb);
                        if (scb->cs_evcb->resume_pending)
                        {
                            status = OK;
                            break;
                        }
                        else
                        {
#endif
		    status = CS_cond_timedwait( &scb->cs_evcb->event_cond,
					        &scb->cs_evcb->event_sem,
					        &ts );
#if defined(VMS)
                            if (scb->cs_evcb->ast_pending) continue;
                            break;
                        }
                    }
#endif
#if defined (dg8_us5) || defined (dgi_us5) || defined(rmx_us5)||defined(rux_us5)
                    if (status)
                        status = errno;     
#endif /* dg8_us5 dgi_us5 */

		    /* Note that EINTR shouldn't happen, including it here
		    ** is defensive (or bogus, depending on your point of
		    ** view).  Treat it as a timeout.
		    */

# if defined(any_aix) || defined(axp_osf) || defined(sgi_us5) || \
     defined(thr_hpux) || \
     defined(LNX) || defined(xCL_USE_ETIMEDOUT)

		    if ( status == ETIMEDOUT || status == EINTR )
#elif defined (dg8_us5) || defined (dgi_us5)||defined(rmx_us5)||defined(rux_us5)

                    if ( status == EAGAIN || status == EINTR )
#else
		    if ( status == ETIME || status == EINTR )
# endif
		    {
			scb->cs_mask |= CS_TO_MASK;
			scb->cs_state = CS_COMPUTABLE;
		    }
		}
		else
		{
		    /* Do do a timed wait if interruptable */
		    if (scb->cs_thread_type == CS_USER_THREAD &&
		        mask & CS_INTERRUPT_MASK)
		    {
			ts.tv_sec += 2;
#if defined(VMS)
                        while (TRUE)
                        {
                           CSMT_execute_asts(scb);
                           if (scb->cs_evcb->resume_pending)
                           {
                              status = OK;
                              break;
                           }
                           else
                           {
#endif
			status = CS_cond_timedwait( &scb->cs_evcb->event_cond,
						    &scb->cs_evcb->event_sem,
						    &ts );
#if defined(VMS)
                               if (scb->cs_evcb->ast_pending) continue;
                               break;
                           }
                         }
#endif
#if defined (dg8_us5) || defined (dgi_us5) || defined(rmx_us5)||defined(rux_us5)
                        if (status)
                         status = errno;
#endif /* dg8_us5 dgi_us5 */

# if defined(any_aix) || defined(axp_osf) || defined(sgi_us5) || \
     defined(thr_hpux) || defined(LNX) || \
     defined(xCL_USE_ETIMEDOUT)
			if ( status == ETIMEDOUT )
#elif defined (dg8_us5) || defined (dgi_us5)||defined(rmx_us5)||defined(rux_us5)
                        if ( status == EAGAIN )
#else
			if ( status == ETIME )
# endif
			{
			    /* do a quick poll for asynchronous messages */
#if defined(VMS)
                            CSMT_execute_asts(scb);
#else
			    timeout = 0;
			    CS_synch_unlock( &scb->cs_evcb->event_sem );
			    VOLATILE_ASSIGN_MACRO(Cs_srv_block.cs_ticker_time,
					scb->cs_last_ccheck);
			    iiCLpoll(&timeout);
			    CS_synch_lock( &scb->cs_evcb->event_sem );
#endif
			    status = OK;
			}
		    }
		    else
		    {
			/*
			** Do a timed wait anyway in case we have lost
			** a signal call. If we are not signalled we'll
			** log the fact.
			** This block replaces a simple CS_cond_wait and
			** the diagnostics are intended to log the event that
			** for whatever reaSON, THE EXPECTED signal did not
			** release the wait.
			*/
			ts.tv_sec += unint_tmo;
#if defined(VMS)
                        while (TRUE)
                        {
                            CSMT_execute_asts(scb);
                            if (scb->cs_evcb->resume_pending)
                            {
                                status = OK;
                                break;
                            }
                            else
                            {
#endif
			status = CS_cond_timedwait( &scb->cs_evcb->event_cond,
						    &scb->cs_evcb->event_sem,
						    &ts );
#if defined(VMS)
                                if (scb->cs_evcb->ast_pending) continue;
                                break;
                            }
                        }
#endif
#if defined (dg8_us5) || defined (dgi_us5) || defined(rmx_us5)||defined(rux_us5)
                        if (status)
                         status = errno;
#endif /* dg8_us5 dgi_us5 */

# if defined(any_aix) || defined(axp_osf) || defined(sgi_us5) || \
     defined(thr_hpux) || defined(LNX) || \
     defined(OSX) || defined(xCL_USE_ETIMEDOUT)
			if ( status == ETIMEDOUT )
#elif defined (dg8_us5) || defined (dgi_us5)||defined(rmx_us5)||defined(rux_us5)
                        if ( status == EAGAIN )
#else
			if ( status == ETIME )
# endif
			{
			    /* Allow a loop around to recheck the auxiliary variable */
			    if (scb->cs_evcb->event_state != EV_WAIT_MASK ||
					(scb->cs_mask & wakeup_bits))
				TRdisplay("SUSP:[%p]No signal but state changed in %d secs cs_mask:%v cs_state:%w ev:%v\n",
					scb->cs_self,
					unint_tmo,
					cs_mask_names, scb->cs_mask,
					cs_state_names, scb->cs_state,
					event_state_mask_names, scb->cs_evcb->event_state);

			    status = OK;
			    /* Increase the timeout by 1.5 */
			    unint_tmo = min(unint_tmo + unint_tmo/2, UNINT_TMO_INCR_CAP);
			}
			else if (status == OK &&
				scb->cs_evcb->event_state == EV_WAIT_MASK &&
				!(scb->cs_mask & wakeup_bits))
			{
			    TRdisplay("SUSP:[%p]Signalled but with no state change %d secs cs_mask:%v\n",
					scb->cs_self, unint_tmo, cs_mask_names, scb->cs_mask);
			}
			else if (status == OK &&
				scb->cs_evcb->event_state == EV_WAIT_MASK &&
				(scb->cs_mask & CS_IRCV_MASK))
			{
			    TRdisplay("SUSP:[%p]Signalled but with CS_IRCV_MASK at %d secs cs_mask:%v TAKEN\n",
					scb->cs_self, unint_tmo, cs_mask_names, scb->cs_mask);
			}
			else if (status != OK)
			{
			    TRdisplay("SUSP:[%p]Returned bad status: %d %d secs cs_mask:%v\n",
					scb->cs_self, status, unint_tmo, cs_mask_names, scb->cs_mask);
			}
		    }
		}
	    }
	}

	/* time passes, things get done.  Finally we resume ... */

        if ((status == E_TIMEOUT) && (scb->cs_self == CS_ADDER_ID))
        {
            rv = E_CS0009_TIMEOUT;
        } 
	else if ( scb->cs_evcb->event_state & EV_EDONE_MASK )
	{
	    rv = OK;
	}
	else if (scb->cs_mask & CS_IRCV_MASK)
	{
	    /* Note that IRCV should only happen if an attention was
	    ** set while the session was in event wait with interrupts on.
	    ** Since we're going to tell the caller that an interrupt
	    ** occurred, we don't need IRPENDING any more if it was on
	    ** from earlier somehow.
	    */
	    rv = E_CS0008_INTERRUPTED;
	    scb->cs_mask &= ~CS_IRPENDING_MASK;
	}
	else if (scb->cs_mask & CS_ABORTED_MASK)
	{
	    rv = E_CS000F_REQUEST_ABORTED;
	}
	else if (scb->cs_mask & CS_TO_MASK)
	{
	    rv = E_CS0009_TIMEOUT;
	}
	else 
	{
	    /* we shouldn't be here */
	    (*Cs_srv_block.cs_elog)(E_CS0019_INVALID_READY, 0, 0);
	    rv = E_CS0019_INVALID_READY;
	}

	scb->cs_mask &= ~(CS_IRCV_MASK
			| CS_TO_MASK | CS_ABORTED_MASK
			| CS_INTERRUPT_MASK
			| CS_NANO_TO_MASK | CS_TIMEOUT_MASK);

	scb->cs_state = CS_COMPUTABLE;
	scb->cs_evcb->event_state = 0;
	CS_synch_unlock( &scb->cs_evcb->event_sem );
	
	/*
	** Time accounting removed, just count waits.
	*/
	{
	    if ((mask & CS_LIOR_MASK) == CS_LIOR_MASK)
	    { 
		Cs_srv_block.cs_wtstatistics.cs_lior_waits++;
	    }
	    else if ((mask & CS_LIOW_MASK) == CS_LIOW_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_liow_waits++;
	    } 
	    else if (mask & CS_LOG_MASK) 
	    {
		Cs_srv_block.cs_wtstatistics.cs_lg_waits++;
	    }
	    else if ((mask & CS_DIOR_MASK) == CS_DIOR_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_dior_waits++;
	    }
	    else if ((mask & CS_DIOW_MASK) == CS_DIOW_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_diow_waits++;
	    } 
	    else if (scb->cs_thread_type == CS_USER_THREAD)
	    {
		if ((mask & CS_BIOR_MASK) == CS_BIOR_MASK)
		{ 
		    Cs_srv_block.cs_wtstatistics.cs_bior_waits++;
		}
		else if ((mask & CS_BIOW_MASK) == CS_BIOW_MASK)
		{ 
		    Cs_srv_block.cs_wtstatistics.cs_biow_waits++;
		}
		else if (mask & CS_LOCK_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_lk_waits++;
		}
		else if (mask & CS_LKEVENT_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_lke_waits++;
		}
		else if (mask & CS_LGEVENT_MASK)
		{
		    Cs_srv_block.cs_wtstatistics.cs_lge_waits++;
		}
		else if (mask & CS_TIMEOUT_MASK)
		    Cs_srv_block.cs_wtstatistics.cs_tm_waits++;
	    }
	}
    }
#ifdef USE_IDLE_THREAD
    }                     /* end of if cs_state is CS_INITIALIZING or CS_TERM*/
#endif /* USE_IDLE_THREAD */
    else
    {
	i4	tim, nevents, reset_wakeme;
	i4	start_time;

	start_time = TMsecs();

        if ( mask & CS_NANO_TO_MASK )
            to_cnt /= 1000000000; /* back to seconds */

	while (!Cs_resumed)
	{
#ifdef USE_IDLE_THREAD
	    reset_wakeme = TRUE;
	    CS_find_events(&reset_wakeme, &nevents);
	    if (nevents == 0)
	    {
		/* CS_find_events cleared csi_wakeme */
#else
	    {
		CS_ACLR(&Cs_svinfo->csi_wakeme);
#endif /* USE_IDLE_THREAD */
		tim = 200;
#if defined(VMS)
                PCsleep(tim);
#else
		if(iiCLpoll(&tim) == E_FAILED)
                        TRdisplay("CSMTsuspend: iiCLpoll failed...serious\n");
#endif
		CS_TAS(&Cs_svinfo->csi_wakeme);
                if ((to_cnt) && (mask & CS_TIMEOUT_MASK) &&
		    ((TMsecs() - start_time) >= to_cnt))
                {
                    rv = E_CS0009_TIMEOUT;
                    break;
                }

	    }
	}
	Cs_resumed = FALSE;
    }
    return(rv);
}

#if defined(VMS)
static i4
CSMT_execute_asts(CS_SCB* scb)
{
     CS_UNLOCK_LONG(&scb->cs_evcb->resume_pending);


    if (scb->cs_evcb->ast_pending)
    {
         CS_UNLOCK_LONG(&scb->cs_evcb->ast_pending);
         CS_LOCK_LONG(&scb->cs_evcb->ast_in_progress);
         CS_synch_unlock( &scb->cs_evcb->event_sem );
         IIMISC_executePollAst ((SRELQHDR *)&scb->cs_evcb->astqueue);
         CS_synch_lock( &scb->cs_evcb->event_sem );
         CS_UNLOCK_LONG(&scb->cs_evcb->ast_in_progress);

    }

    return (scb->cs_evcb->resume_pending);
}
#endif

/*{
** Name: CSMTresume()	- Mark thread as done with event
**
** Description:
**	This routine is called to note that the session identified
**	by the parameter has completed some event.  And will resume
**	thread if it has suspended.
**
**	In contrast to other CS implementations, CSMT always sets the
**	CS_EDONE_MASK in the evcb->event_state, NEVER in the cs_mask.
**
** Inputs:
**	sid				thread id for which event has
**					completed.
** Outputs:
**	none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    The OS thread associated with this SCB for this thread has
**	    it's event condition variable signaled if event state is
**	    waiting, and no other resume has been called.
** History:
**         Increment the active session count (cs_num_active) when
**         resuming user sessions.
**	24-aug-2001 (abbjo03)
**	    Back out change to tracking of cs_num_active (18-sep-2000) since it
**	    has side effects on subsequent invocations.
**	23-jan-2002 (devjo01)
**	    - Use compare & swap logic to update cs_evcb->event_state,
**	      removing update of scb->cs_mask & use of mutexes.
**	    - Remove ancient inaccurate comments inherited from CSresume
**	      when mode was "cloned" from csinterface.c.
**	    - Strip out history duplicated from CSresume.
**	17-Dec-2003 (jenjo02)
**	    Watch for bogus resume issued on session
**	    in CS_CNDWAIT state; ignore the resume.
*/
VOID
CSMTresume(CS_SID sid)
{
    CS_SCB		*scb;
#ifdef VMS
    CS_SCB              *curr_scb = NULL;
#endif
    i4			cevs, nevs;

#ifdef USE_IDLE_THREAD
   if (Cs_srv_block.cs_state == CS_INITIALIZING ||
       Cs_srv_block.cs_state == CS_TERMINATING)
   {
       Cs_resumed = 1;
   }
   else
   {
#endif /* USE_IDLE_THREAD */
    scb = CSMT_find_scb(sid);	    /* locate the scb for this thread */
#ifdef VMS
    CSMTget_scb(&curr_scb);
#endif

    if (scb && scb->cs_evcb )
    {
	CS_synch_lock( &scb->cs_evcb->event_sem );
	if ( scb->cs_state != CS_CNDWAIT )
	{
#ifdef conf_CLUSTER_BUILD
	    do
	    {
		cevs = scb->cs_evcb->event_state;
		if (EV_EDONE_MASK & cevs)
		{
		    /* EDONE Already set, no work required. */
		    break;
		}
		
		nevs = cevs | EV_EDONE_MASK;
	    } while ( CScas4( &scb->cs_evcb->event_state, cevs, nevs ) );
#else
	    cevs = scb->cs_evcb->event_state;
	    scb->cs_evcb->event_state |= EV_EDONE_MASK;
#endif
	    if (cevs == EV_WAIT_MASK)
	    {
		CS_cond_signal( &scb->cs_evcb->event_cond );

#if defined(VMS)
                 if ((scb->cs_evcb->ast_in_progress) && (curr_scb == scb))
                 {
                    if (!scb->cs_evcb->resume_pending) CS_LOCK_LONG(&scb->cs_evcb->resume_pending);
                 }
#endif
	    }
	}
	CS_synch_unlock( &scb->cs_evcb->event_sem );
    }
#ifdef USE_IDLE_THREAD
   }
#else
   else if (Cs_srv_block.cs_state == CS_INITIALIZING ||
       Cs_srv_block.cs_state == CS_TERMINATING)
   {
       Cs_resumed = 1;
   }
#endif /* USE_IDLE_THREAD */
}

#ifdef conf_CLUSTER_BUILD

/*{
** Name: CSMTresume_from_AST()	- Mark thread as done with event
**
** Description:
**	This routine is almost identical to CSMTresume, but must
**	only be called from a VMS AST, or a UNIX signal handler.
**
** Inputs:
**	sid				thread id for which event has
**					completed.
** Outputs:
**	none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    The OS thread associated with this SCB for this thread has
**	    it's event condition variable signaled if event state is
**	    waiting, and no other resume has been called.
**
** History:
**	23-jan-2002 (devjo01)
**	    Initial version created from newly modified CSMTresume.
*/
VOID
CSMTresume_from_AST(CS_SID sid)
{
    CS_SCB		*scb;
    i4			cevs, nevs;

    scb = CSMT_find_scb(sid);	    /* locate the scb for this thread */

    if (scb && scb->cs_evcb )
    {
#if defined(xCL_SUSPEND_USING_SEM_OPS)
	    sem_post( &scb->cs_evcb->event_wait_sem);
#endif
    }
}


/*{
** Name: CSMTsuspend_for_AST()	- Suspend current thread for AST 
**
** Description:
**	This procedure is used to suspend activity for a particular
**	thread pending some event which will be delivered via a
**	CSresume_from_AST() call.  Optionally, this call will also schedule
**	timeout notification.  If the event is to be resumed following
**	an interrupt, then the CS_INTERRUPT_MASK should be specified.
**
** Inputs:
**	mask				Mask of events which can terminate
**					the suspension.	 Must be a mask of
**					  CS_INTERRUPT_MASK - can be interrupted
**					  CS_TIMEOUT_MASK - event will time out
**					along with the reason for suspension:
**					(CS_DIO_MASK, CS_BIO_MASK, CS_LOCK_MASK).
**					  CS_NANO_TO_MASK - timeout values
**					are in nanoseconds, zero is zero
**	to_cnt				number of seconds after which event
**					will time out.	Zero means no timeout
**					(unless CS_NANO_TO_MASK is set).
**	ecb				Pointer to a system specific event
**					control block.	Can be zero if not used.
**					If CS_LOCK_MASK, this may instead be
**					a pointer to a LK_LOCK_KEY for iimonitor.
**
** Outputs:
**	none
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
**
** History:
**	2-Sep-2004 (mutma03) 
**	    Created.
**	16-nov-2004 (devjo01)
**	    - Modify check of sem_timedwait return status to compensate for
**	    known issue where some Linux distros are not POSIX compliant.
**	    On error, rather than passing exact status in errno and
**	    returning -1, some codelines were returning the status, and
**	    leaving errno unset.
**	    - Remove some TRdisplay calls left over from development.
**	    - Make entire body of routine conditional on
**          xCL_SUSPEND_USING_SEM_OPS.
*/
STATUS
CSMTsuspend_for_AST(i4 mask, i4  to_cnt, PTR ecb)
{
# if 	defined(xCL_SUSPEND_USING_SEM_OPS)
    CS_SCB		*scb;
    STATUS		rv = OK;
    i4             timeout;
    CS_COND_TIMESTRUCT	ts, to;
    SYSTIME		time_now;
    STATUS		status;
    i4			cevs;

    CSMTget_scb( &scb );

    if ( scb  && scb->cs_evcb )
    {
	if (ecb)
	scb->cs_sync_obj = ecb;


	scb->cs_memory = (mask & (CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
				  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
				  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
				  CS_LOG_MASK | CS_TIMEOUT_MASK));

	scb->cs_mask |= (mask & ~(CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
                                  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
                                  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
                                  CS_LOG_MASK));

	scb->cs_state = CS_EVENT_WAIT;
	scb->cs_evcb->event_state = EV_AST_WAIT_MASK;
	scb->cs_timeout = 0;
	if (mask & CS_TIMEOUT_MASK)
	{
	    if (to_cnt == 0)
	    {
		/* Timeout specified, but no timeout value, ignore timeout */
		scb->cs_memory &= ~CS_TIMEOUT_MASK;
		mask &= ~CS_TIMEOUT_MASK;
	    }
	    else
	    {
	        /* compute the end timeout value */
		i4  msecs;
		if (to_cnt < 0)
		    scb->cs_timeout = - to_cnt;
		else
		    scb->cs_timeout = to_cnt * 1000;
		to.tv_sec = scb->cs_timeout / 1000;
		msecs = scb->cs_timeout % 1000;
		to.tv_nsec = msecs * 1000000;

		TMet(&time_now);
		ts.tv_sec  = time_now.TM_secs;
		ts.tv_nsec = time_now.TM_msecs * 1000000;

		ts.tv_sec  += to.tv_sec;
		ts.tv_nsec += to.tv_nsec;
		if ( ts.tv_nsec > 1000000000 ) /* overflow? */
		{
		    ts.tv_nsec %= 1000000000;
		    ts.tv_sec += 1;
		}
	    }
	}

	do
	{
	    if ( mask & (CS_TIMEOUT_MASK | CS_NANO_TO_MASK) )
	    {
		status = sem_timedwait( &scb->cs_evcb->event_wait_sem, &ts );
                if ( -1 == status )
                    status = errno;     
		if ( status == ETIMEDOUT )
		{
		    status = E_CS0009_TIMEOUT;
		}
		if ( ( status == EINTR ) && (scb->cs_mask & CS_IRCV_MASK) &&
                   (mask & CS_INTERRUPT_MASK) )
		{
		    status = E_CS0008_INTERRUPTED;
		}
	    }
	    else
	    {
		/* Don't do a timed wait if noninterruptable */
		if (scb->cs_thread_type == CS_USER_THREAD &&
		    mask & CS_INTERRUPT_MASK)
		{
		    TMet(&time_now);
		    ts.tv_sec  = time_now.TM_secs;
		    ts.tv_nsec = time_now.TM_msecs * 1000000;

		    ts.tv_sec += 2;
		    status = sem_timedwait(&scb->cs_evcb->event_wait_sem, &ts);
		    if ( -1 == status )
			status = errno;

		    if ((status == EINTR) && (scb->cs_mask & CS_IRCV_MASK))
		    {
			status = E_CS0008_INTERRUPTED;
		    }

		    if (status == ETIMEDOUT)
		    {
			/* do a quick poll for asynchronous messages */
			timeout = 0;
			VOLATILE_ASSIGN_MACRO(Cs_srv_block.cs_ticker_time,
					scb->cs_last_ccheck);
			iiCLpoll(&timeout);
			status = EINTR;
		    }
		}
		else
		{
		    status = sem_wait( &scb->cs_evcb->event_wait_sem );
		    if ( -1 == status )
			status = errno; 
		}
	    }
	}
	while (status == EINTR);

	/* time passes, things get done.  Finally we resume ... */
	if (scb->cs_mask & CS_ABORTED_MASK)
	{
	    rv = E_CS000F_REQUEST_ABORTED;
	}
	else if (status == OK ||
	         status == E_CS0009_TIMEOUT ||
	         status == E_CS0008_INTERRUPTED)
	{
	    if ( scb->cs_mask & CS_IRCV_MASK )
	    {
		 status = E_CS0008_INTERRUPTED;
	    }
	    rv = status;
	}
	else
	{
	    rv = E_CS00FF_FATAL_ERROR;
	}

	scb->cs_mask &= ~(CS_IRCV_MASK
			| CS_TO_MASK | CS_ABORTED_MASK
			| CS_INTERRUPT_MASK
			| CS_NANO_TO_MASK | CS_TIMEOUT_MASK);

	scb->cs_state = CS_COMPUTABLE;
	scb->cs_evcb->event_state = 0;
        sem_init( &scb->cs_evcb->event_wait_sem,0,0 );
    }
    return(rv);
#else
    /* Should never be called if xCL_SUSPEND_USING_SEM_OPS nt defined. */
    return E_CS00FF_FATAL_ERROR;
#endif
}

#endif /* conf_CLUSTER_BUILD */

/*{
** Name: CSMTget_ast_ctx()  - save SCB context for AST delivery
**
** Description:
**
** Inputs:
**  ctx       address of ast context block
**  scb       address of session control block
**
** Outputs:
**  none
**
**  Returns:
**      none
**  Exceptions:
**      none
**
** Side Effects:
**      The AST context block is added to ast queue and
**      the event condition variable is signalled
**      N.B. This will only ever be called  from the ast_dispatcher thread
**
** History:
**  15-May-2008 (stegr01)
**      Initial version created
*/

#if defined(VMS)
i4
IICSMTget_ast_ctx(u_i2 ssrv, u_i4 ret_pc,
                  CS_SCB**         ast_scb,
                  pthread_cond_t** evt_cv,
                  void**           ast_qhdr,
                  unsigned int**   ast_pending)
{
    CS_SCB* scb = NULL;
    i4 sts = EFAIL;

    assert (CL_OFFSETOF(CS_SM_WAKEUP_CB, astqueue)%8 == 0);

    CSMTget_scb(&scb);



    if (scb && scb->cs_evcb)
    {
        *ast_scb         = scb;
        *ast_qhdr        = &scb->cs_evcb->astqueue;
        *evt_cv          = &scb->cs_evcb->event_cond;
        *ast_pending     = &scb->cs_evcb->ast_pending;
        sts = OK;
    }

    return (sts);
}


i4
IICSMTget_ast_ctx2(i4              tid,
                   CS_SCB**         ast_scb,
                   pthread_cond_t** evt_cv,
                   void**           ast_qhdr,
                   unsigned int**   ast_pending)
{
    CS_SCB* scb = NULL;
    i4 sts = EFAIL;

    /*
    ** assume that the issuing thread is the admin thread if the SCB is null
    ** This routines is called from astmgmt, when the AST dispatcher thread
    ** finds no SCB present.
    */

/*    if (tid == 1)           */
/*    {                       */
        scb = &Cs_admin_scb;
/*    }                       */



    if (scb && scb->cs_evcb)
    {
        *ast_scb         = scb;
        *ast_qhdr        = &scb->cs_evcb->astqueue;
        *evt_cv          = &scb->cs_evcb->event_cond;
        *ast_pending     = &scb->cs_evcb->ast_pending;
        sts = OK;
    }

    return (sts);
}
#endif

/*{
** Name: CScancelled	- Event has been canceled
**
** Description:
**	This routine is called by the client routines when an event for
**	which the client had suspended will not be CSresume()'d in the
**	future (it may have already been).  This call is intended to be
**	used whenever a CSsuspend() returns an interrupted or timed out
**	status.	 In these cases, on some systems, the CSresume() call
**	may appear in the future, because, for example, the event occurred
**	just after the timeout value has expired.  In this case, the
**	dispatcher's scheduler cannot tell which event has completed,
**	and if it is not told when a sequence is complete, the dispatcher
**	cannot determine when a thread can be resumed correctly.
**
**	Thus, it must be the case that if a CSsuspend() routine returns
**	anything other than OK (i.e. that the event completed normally),
**	then:  a) the caller must take whatever steps necessary to ensure
**	that any CSresume that might happen, either HAS happened or will
**	NOT happen;  and then b) the caller calls CScancelled to clear
**	the resume if it did occur.
**
** Inputs:
**	ecb			Pointer sized NOT USED, can be NULL.
**
** Outputs:
**	none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-Oct-1986 (fred)
**	    Created on Jupiter.
**	 7-nov-1988 (rogerk)
**      11-mar-2005 (mutma03)
**          If using SEM_OPS, clear sem count, just in case.
**	    Set inkernel while altering cs_mask.
*/
/* ARGSUSED */
VOID
CSMTcancelled(ecb)
PTR		ecb;
{
    CS_SCB	*scb;

    CSMTget_scb( &scb );
    if (scb && scb->cs_evcb)
    {
	CS_synch_lock( &scb->cs_evcb->event_sem );
	scb->cs_evcb->event_state = 0;
	scb->cs_mask &= ~(CS_IRCV_MASK      | 
			  CS_TO_MASK        | 
			  CS_ABORTED_MASK   | 
			  CS_INTERRUPT_MASK | 
			  CS_NANO_TO_MASK   | 
			  CS_TIMEOUT_MASK);
	CS_synch_unlock( &scb->cs_evcb->event_sem );
# if    defined(xCL_SUSPEND_USING_SEM_OPS)
        sem_init( &scb->cs_evcb->event_wait_sem,0,0 );
#endif
	/* note: don't set Cs_resumed to FALSE in the multi-thread case */
	/* many threads could have been resumed and only some canceled. */
    }
    else if (Cs_srv_block.cs_state == CS_INITIALIZING)
	Cs_resumed = FALSE;
}

/*{
** Name: CSMTadd_thread - Add a thread to the server space.
**
** Description:
**	This routine is called to add a thread to the server known thread
**	list.  The thread is created, placed in the ready queue at the
**	appropriate priority, and is placed in competition for resources.
**	The only parameter given specifies the priority for this thread.
**	A value of zero (0) for this parameter indicates that "normal"
**	priority is to be used.
**
**	This routine performs queue manipulation functions, and, therefore,
**	must run in such a manner as to not be interrupted.
**
** Inputs:
**	priority			Priority for new thread.  A value
**					of zero (0) indicates that "normal"
**					priority is to be used.
**	crb				A pointer to the connection
**					request block for the thread.
**					This is the block which was sent to
**					request that a new thread be added.
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
**
** Outputs:
**	thread_id			optional pointer to CS_SID into which
**					the new thread's SID will be returned.
**	error				Filled in with an OS
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
**	11-Nov-1986 (fred)
**	    Created.
**	15-May-1988 (rogerk)
**	    Added thread type argument.
**	    Set AST state to state it was when routine called rather than
**	    always to on.
**	31-oct-1988 (rogerk)
**	    If server is writing accouting records for each session, then
**	    turn on the session mask to collect CPU statistics.
**	 7-nov-1988 (rogerk)
**	    Initialize async portion of scb.
**      28-feb-1989 (rogerk)
**          Set cs_pid field of scb.
**	24-jul-1989 (rogerk)
**	    Initialize cs_ef_mask field.
**	27-feb-92 (daveb)
**	    Initialize new cs_client_type field in SCB's, as part of fix
**	    for B38056.
**	28-Oct-1992 (daveb)
**	    Attach new threads in CSadd_thread.
**	26-apr-1993 (bryanp)
**	    Removed setting of Cs_resumed = 1 in the non-initializing case. See
**	    large comment at head of this file.
**	3-may-1995 (hanch04)
**          Changed entry point into CS_su4_setup() for su4_us5 to a known,
**          non-variable value.
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	12-feb-1996 (canor01)
**	    thread_type_cs should be thread_type
**	31-oct-1996 (canor01)
**	    Add additional error message if thread creation fails.
**	02-may-1997 (canor01)
**	    If a thread is created with the CS_CLEANUP_MASK set in
**	    its thread_type, transfer the mask to the cs_cs_mask.
**	    These threads will be terminated and allowed to finish
**	    processing before any other system threads exit.
**	20-Aug-1997 (kosma01)
**	    Cast variables assigned to scb->cs_self. On POSIX thread
**	    platforms, cs_self is defined as a scalarp, whereas the
**	    *scb points to a large structure.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSMTadd_thread() prototype.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
**	16-Nov-1998 (jenjo02)
**	    Allocate a wakeup block for each session.
**	22-May-2002 (jenjo02)
**	    Count CS_MONITOR sessions in cs_user_sessions to be
**	    consistent with CSadd_thread().
**       3-Dec-2004 (hanal04) Bug 113291 INGSRV3011
**          When we fail to create a new thread reject the association.
**	23-jan-2005 (devjo01)
**	    Fix compile warning in non-lnx, non-HP call to CS_create_thread.
*/
STATUS
CSMTadd_thread(priority, crb, thread_type, session_id, error)
i4		   priority;
PTR		   crb;
i4		   thread_type;
CS_SID		   *session_id;
CL_ERR_DESC	   *error;
{
    STATUS		status;
    CS_SCB		*scb;
    PTR			stkaddr = NULL;

    if ( error )
	CL_CLEAR_ERR( error );

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
	return(status);

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
    scb->cs_ppid = 0;       /* hack, really used for idle time */
    scb->cs_ef_mask = 0;
    scb->cs_cs_mask = 0;
    scb->cs_last_ccheck = 0;
    /* Set cancel check to (almost) (u_i4) infinity if not a user thread.
    ** Allow a little to avoid wraparound in CSMTcancelCheck.
    */
    if (scb->cs_thread_type != CS_USER_THREAD)
	scb->cs_last_ccheck = 0xffffff00;

    if ( thread_type & CS_CLEANUP_MASK )
	scb->cs_cs_mask |= CS_CLEANUP_MASK;

    /* Allocate and init a event wakeup block */
    if ( status = CSMT_get_wakeup_block(scb) )
    {
        SETCLERR(error, status, ER_threadcreate);
	TRdisplay( "failed to get wakeup block, status = %d\n", status);
        (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, error, 0);
	status = E_CS000E_SESSION_LIMIT;
	CSMT_del_thread( scb );
    }

    /*
    ** If collecting CPU stats for all dbms sessions, then turn on CPU
    ** statistic collecting.  Otherwise, cpu stats will only be collected
    ** for this thread if a CSaltr_session(CS_AS_CPUSTATS) call is made.
    */
    if (Cs_srv_block.cs_mask & CS_CPUSTAT_MASK)
	scb->cs_mask |= CS_CPU_MASK;

# if !defined(xCL_NO_STACK_CACHING) && !defined(any_hpux)
    if ( Cs_srv_block.cs_stkcache == TRUE )
    {
        CS_synch_lock( &CsStackSem );
        status = CS_alloc_stack( scb, error );
        CS_synch_unlock( &CsStackSem );
        if ( status != OK )
        {
            return (status);
        }
    }
    else
# endif  /* !xCL_NO_STACK_CACHING && !HP */
    {
	scb->cs_stk_area = NULL;
# ifdef ibm_lnx
	scb->cs_registers[CS_SP] = 0;
# else
# if !defined(NO_INTERNAL_THREADS)
	scb->cs_sp = 0;
# endif
# endif /* ibm_lnx */
    }

    CSMTp_semaphore(1, &Cs_known_list_sem);       /* exclusive */
    scb->cs_next = Cs_srv_block.cs_known_list;
    scb->cs_prev = Cs_srv_block.cs_known_list->cs_prev;
    Cs_srv_block.cs_known_list->cs_prev->cs_next = scb;
    Cs_srv_block.cs_known_list->cs_prev          = scb;

    ++Cs_srv_block.cs_num_sessions;
    if (scb->cs_thread_type == CS_USER_THREAD ||
	scb->cs_thread_type == CS_MONITOR)
    {
	Cs_srv_block.cs_user_sessions++;
    }
    CSMTv_semaphore(&Cs_known_list_sem);
    CS_synch_lock( &scb->cs_evcb->event_sem );  /* effectively suspend thread */

#if defined(rux_us5)
    CS_create_thread( Cs_srv_block.cs_stksize, CSMT_setup,
                      scb, &scb->cs_thread_id, 
		      (Cs_srv_block.cs_stkcache) ? CS_JOINABLE : CS_DETACHED,
		      &status );
# else
# if defined(ibm_lnx) || (defined(a64_sol) && defined(NO_INTERNAL_THREADS))
    CS_create_thread( Cs_srv_block.cs_stksize, 
		      (PTR)NULL, 
		      CSMT_setup,
		      scb, &scb->cs_thread_id, 
		      (Cs_srv_block.cs_stkcache) ? CS_JOINABLE : CS_DETACHED, 
		      &status );
# elif defined(any_hpux)
    CS_create_thread( Cs_srv_block.cs_stksize, 
		      (PTR)scb->cs_sp, 
		      CSMT_setup,
		      scb, &scb->cs_thread_id, 
		      CS_DETACHED, 
		      Cs_srv_block.cs_stkcache,
		      &status );
# else
    CS_create_thread( Cs_srv_block.cs_stksize,
                      (PTR)scb->cs_sp,
                      (void *(*)(void *))CSMT_setup,
                      scb, &scb->cs_thread_id,
		      (Cs_srv_block.cs_stkcache) ? CS_JOINABLE : CS_DETACHED, 
                      &status );
# endif /* ibm_lnx */
# endif

# if defined(DCE_THREADS)
    if ( !CS_thread_id_equal( scb->cs_thread_id, Cs_thread_id_init ) )
# else
    if (scb->cs_thread_id)
# endif /* DCE_THREADS */
    {
	/* If wanted, return session_id to thread creator */
	if (session_id)
	    *session_id = scb->cs_self;

	CS_scb_attach( scb );

	CS_synch_unlock( &scb->cs_evcb->event_sem );  /* unsuspend thread */
    }
    else
    {
        SETCLERR(error, status, ER_threadcreate);
	CS_synch_unlock( &scb->cs_evcb->event_sem );  /* unsuspend thread */
	TRdisplay( "failed to create thread, status = %d, errno = %d\n", 
                   status, errno );
        (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, error, 0);
	status = (*Cs_srv_block.cs_reject)(crb, -1);
	status = E_CS000E_SESSION_LIMIT;
	CSMT_del_thread( scb );
    }

    return (status);
}

/*{
** Name: CSMTremove	- Cause a thread to be terminated
**
** Description:
**	This routine is used to cause the dispatcher to remove
**	a thread from competition for resources.  It works by
**	setting the next state for the thread to CS_TERMINATE
**	and, from there, letting "nature take its course" in having
**	each of the threads terminate themselves.  This call is intended
**	for use only by the monitor code.
**
**	Note that this routine does not evaporate a thread at any
**	point other than when it has returned.	The reason that this
**	decision was made is to allow a thread to clean up
**	after itself.  Randomly evaporating threads will inevitably
**	result in a trashed server because the thread which has been
**	evaporated will have been holding some resource.  The CS module,
**	as previously stated, is cooperative;  it cannot go clean up
**	a thread's resources since it does not know what/where they
**	are.
**
** Inputs:
**	sid				The thread id to remove
**
** Outputs:
**	none
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
**	13-Nov-1986 (fred)
**	    Created
**	 2-Nov-1988 (rogerk)
**	    Be sure to re-enable AST's before returning with error.
**	23-Oct-1992 (daveb)
**	    prototyped
**	26-apr-1993 (bryanp)
**	    Removed setting of Cs_resumed = 1 in the non-initializing case. See
**	    large comment at head of this file.
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**      18-Sep-2000 (hanal04) Bug 102042 Problem INGDBA67.
**         Maintain cs_num_active count. 
**	14-mar-2001 (somsa01)
**	    Added HP-UX equivalents for suspending/resuming a session
**	    thread.
**	24-aug-2001 (abbjo03)
**	    Back out change to tracking of cs_num_active (18-sep-2000) since it
**	    has side effects on subsequent invocations.
**	06-Feb-2003 (jenjo02)
**	    BUG 109586: Cleanup CS_CONDITION handling, mutex it
**	    while changing its state.
**	12-Aug-2004 (jenjo02)
**	    Avoid potential CS_CNDWAIT mutex deadlock.
*/
STATUS
CSMTremove(CS_SID sid)
{
    CS_SCB		*scb;
    CS_SCB		*cscb;

    scb = CSMT_find_scb(sid);
    if (scb == 0)
    {
	return(E_CS0004_BAD_PARAMETER);
    }

    CS_synch_lock( &scb->cs_evcb->event_sem );

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
# ifdef sparc_sol
	    thr_continue( scb->cs_thread_id );
# else
# ifdef thr_hpux
	    pthread_continue( scb->cs_thread_id );
# endif /* hpb_us5 */
# endif /* su4_us5 */
	}
	else if (scb->cs_state == CS_MUTEX)
	{
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_mask |= CS_ABORTED_MASK;
	}
	else if (scb->cs_state == CS_EVENT_WAIT)
	{
	    scb->cs_mask |= CS_ABORTED_MASK;
	    if (scb->cs_mask & CS_INTERRUPT_MASK)
	    {
		scb->cs_state = CS_COMPUTABLE;
		if (scb->cs_evcb->event_state & EV_WAIT_MASK)
		{
		    scb->cs_evcb->event_state &= ~EV_WAIT_MASK;
		    CS_cond_signal( &scb->cs_evcb->event_cond );
		}
		else
	        if (scb->cs_evcb->event_state & EV_AST_WAIT_MASK)
		{
		    scb->cs_evcb->event_state &= ~EV_AST_WAIT_MASK;
#if defined(xCL_SUSPEND_USING_SEM_OPS)
		    sem_post( &scb->cs_evcb->event_wait_sem );
#endif
		}
	    }
	}
	else if (scb->cs_state == CS_CNDWAIT)
	{
	    CS_CONDITION	*cp;
	    CS_SEMAPHORE	*cnd_mtx;

	    /* remove from condition queue */
	    cp = scb->cs_cnd;
	    cnd_mtx = cp->cnd_mtx;

	    /*
	    ** There's a small yet annoying possibility of a 
	    ** mutex deadlock. All other uses of conditions
	    ** first take the cnd_mutex, then the event_sem.
	    ** To avoid a deadlock, release the event_sem,
	    ** take the cnd_mutex, then check that the
	    ** SCB is still in a condition wait on the
	    ** same condition; if not, do nothing.
	    */
	    CS_synch_unlock( &scb->cs_evcb->event_sem );

	    if ( cp->cnd_mtx == cnd_mtx &&
	        (CSMTp_semaphore(1, cnd_mtx)) == OK )
	    {
		CS_synch_lock( &scb->cs_evcb->event_sem );

		if ( scb->cs_state == CS_CNDWAIT && cp == scb->cs_cnd )
		{
		    cp->cnd_next->cnd_prev = cp->cnd_prev;
		    cp->cnd_prev->cnd_next = cp->cnd_next;

		    scb->cs_mask |= CS_ABORTED_MASK;
		    scb->cs_state = CS_COMPUTABLE;
		    scb->cs_cnd = NULL;
		    scb->cs_sync_obj = NULL;
		    CS_cond_signal( &scb->cs_evcb->event_cond );
		}

		CS_synch_unlock( &scb->cs_evcb->event_sem );
		CSMTv_semaphore(cnd_mtx);
	    }
	    return(OK);
	}
    }
    CS_synch_unlock( &scb->cs_evcb->event_sem );

    return(OK);
}
/*{
** Name: CSMTcnd_init - initialize condition object
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	16-sep-1997 (canor01)
**	    Initialize state to CND_WAITING.
**	17-Dec-2003 (jenjo02)
**	    Removed MO stuff, added cnd_mtx.
*/
STATUS
CSMTcnd_init( CS_CONDITION *cnd )
{
    cnd->cnd_mtx = NULL;
    cnd->cnd_waiter = NULL;
    cnd->cnd_next = cnd->cnd_prev = cnd;
    cnd->cnd_name = "";

    return(OK);
}
/*{
** Name: CSMTcnd_free - free condition object
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	17-Dec-2003 (jenjo02)
**	    Removed MO stuff.
*/
STATUS
CSMTcnd_free(CS_CONDITION	*cnd)
{
    if ( !cnd || cnd->cnd_waiter != NULL || cnd != cnd->cnd_next )
	return(FAIL);

    return(OK);
}
/*{
** Name: CSMTcnd_wait - condition wait
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
**	Condition waits are always "interruptable" in the sense that
**	an interrupt delivered while waiting is always honored;  we
**	return with an ABORTED indication.  In addition, a pending
**	interrupt status upon entry causes an immediate ABORTED return.
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	16-sep-1997 (canor01)
**	    If a call to CScnd_signal() is made before the waiting thread
**	    has gone to sleep on the condition object, it is possible to
**	    lose the signal.  Added CND_WAITING state to synchronize.
**	30-sep-1997 (canor01)
**	    Protect access to scb flags with cs_access_sem.  Double-check 
**	    CS_ABORTED_MASK.
**      18-Sep-2000 (hanal04) Bug 102042 Problem INGDBA67.
**          Maintain cs_num_active count.
**	24-aug-2001 (abbjo03)
**	    Back out change to tracking of cs_num_active (18-sep-2000) since it
**	    has side effects on subsequent invocations.
**	06-Feb-2003 (jenjo02)
**	    BUG 109586: cnd_waiters incr must be mutexed, release SCB's 
**	    event_sem while waiting so CSMT_remove, CSMT_attn can have
**	    it.
**	03-mar-2003 (devjo01)
**	    Remove usage of 'cnd_mutex_sem'.  All critical data items
**	    are actually protected by 'cnd_event_sem', and 'cnd_mutex_sem'
**	    is just harmful (leaks mem on Tru64) baggage.
**	17-Dec-2003 (jenjo02)
**	    Rearchitected to eliminate persistent race conditions.
**	    External semaphore is acquired even if REQUEST_ABORTED.
**	12-Feb-2007 (kschendel)
**	    For user threads only, do timed waits and wake up
**	    every now and then for a poll, to see user cancels.
**	    (Otherwise things like massive threaded modifies ignore the
**	    cancel till nearly the end, not much use!)
**	    Note that this change makes all condition-wait calls
**	    potentially interruptable, caller needs to deal with it.
*/
STATUS
CSMTcnd_wait( CS_CONDITION	*cnd, CS_SEMAPHORE	*mtx )
{
    CS_SCB		*scb;
    STATUS		status = OK, mstatus;

    CSMTget_scb(&scb);

    if (!cnd || !(cnd->cnd_mtx = mtx) )
	return FAIL;

    CS_synch_lock( &scb->cs_evcb->event_sem );

    if (scb->cs_mask & CS_IRPENDING_MASK)
    {
	/* interrupt received while thread was computable */
	scb->cs_mask &= ~CS_IRPENDING_MASK;
	scb->cs_mask |= CS_ABORTED_MASK;
    }
    if (scb->cs_mask & CS_ABORTED_MASK)
    {
	scb->cs_mask &= ~CS_ABORTED_MASK;
	CS_synch_unlock( &scb->cs_evcb->event_sem );
        return E_CS000F_REQUEST_ABORTED;
    }


    scb->cs_state = CS_CNDWAIT;
    scb->cs_cnd   = &scb->cs_cndq;
    scb->cs_sync_obj = (PTR)cnd;

    /* Add this SCB (last) to list of waiters */
    scb->cs_cndq.cnd_name = cnd->cnd_name;
    scb->cs_cndq.cnd_mtx = mtx;
    scb->cs_cndq.cnd_waiter = scb;
    scb->cs_cndq.cnd_next = cnd;
    scb->cs_cndq.cnd_prev = cnd->cnd_prev;
    cnd->cnd_prev->cnd_next = &scb->cs_cndq;
    cnd->cnd_prev = &scb->cs_cndq;

    /* Release the semaphore, then wait */
    if ( status = CSMTv_semaphore(mtx) )
    {
	/* Something wrong with semaphore, abort */
	scb->cs_cnd = NULL;
	scb->cs_sync_obj = NULL;
	scb->cs_state = CS_COMPUTABLE;
	/*
	** If the semaphore was not held, then the
	** cnd queue may well be corrupted now, but
	** try to repair it anyway...
	*/
	scb->cs_cndq.cnd_next->cnd_prev = scb->cs_cndq.cnd_prev;
	scb->cs_cndq.cnd_prev->cnd_next = scb->cs_cndq.cnd_next;
	CS_synch_unlock( &scb->cs_evcb->event_sem );
    }
    else
    {
#if defined(VMS) && !defined(POSIX_THREADS)
        CS_ssprsm( FALSE ); /* wait we are restarted and are runable */
#else
	CS_COND_TIMESTRUCT ts;
	i4 timeout;
	STATUS wait_status;
	SYSTIME time_now;

	/* Wait 'til brought out of our reverie */
	/* Unreadable ifdefs, gak.  Only do the timed-wait thing for
	** posix threads platforms, blow it off for any others (if there
	** are any others left...
	*/
	while ( scb->cs_state == CS_CNDWAIT &&
		!(scb->cs_mask & CS_ABORTED_MASK) )
	{
#ifdef POSIX_THREADS
	    if (scb->cs_thread_type != CS_USER_THREAD)
	    {
#endif
		CS_cond_wait( &scb->cs_evcb->event_cond,
			  &scb->cs_evcb->event_sem );
#ifdef POSIX_THREADS
	    }
	    else
	    {
		/* Do timed waits, so that we can occasionally poll for
		** cancels from the front-end.  This is stupid but that's
		** how it works...
		** Hardwire in a 5-second timeout, and don't worry about
		** the nanoseconds (so it might be as short as a 4.000000001
		** second timeout, big deal).
		** Have to recompute each time since we don't know how long
		** the wait REALLY took...
		*/
		TMet(&time_now);
		ts.tv_sec = time_now.TM_secs;
		ts.tv_nsec = 0;
		ts.tv_sec += 5;
		wait_status = CS_cond_timedwait(&scb->cs_evcb->event_cond,
                          &scb->cs_evcb->event_sem, &ts);
		if (wait_status == ETIMEDOUT)
		{
		    /* Take a quick peek for async messages, will set
		    ** ABORTED if we're still in CNDWAIT.
		    */
		    CS_synch_unlock( &scb->cs_evcb->event_sem );
		    VOLATILE_ASSIGN_MACRO(Cs_srv_block.cs_ticker_time,
					scb->cs_last_ccheck);
#ifdef VMS
                    CSMT_execute_asts( scb );
#else
		    timeout = 0;
		    iiCLpoll(&timeout);
#endif
		    CS_synch_lock( &scb->cs_evcb->event_sem);
		}
	    }
#endif
	}
#endif /* VMS*/

	if ( scb->cs_mask & CS_ABORTED_MASK )
	    status = E_CS000F_REQUEST_ABORTED;

	scb->cs_mask &= ~CS_ABORTED_MASK;
	scb->cs_state = CS_COMPUTABLE;
	CS_synch_unlock( &scb->cs_evcb->event_sem );

	mstatus = CSMTp_semaphore(1, mtx);
    }

    return ( (status) ? status : mstatus );
}
/*{
** Name: CSMTcnd_signal - condition signal
**
** Description:
**	Signal that one waiter on the condition should be set runable.
**
** Inputs:
**	cond			CS_CONDITION to wake up.
**	sid			CS_SID of specific session
**				to wake up, or zero.
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
**	semaphore is released.	See examples.
**
** History:
**	11-Apr-90 (anton)
**	    Created.
**	23-Oct-1992 (daveb)
**	    prototyped
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	03-mar-2003 (devjo01)
**	    Remove init of 'cnd_mutex_sem'.
**	17-Dec-2003 (jenjo02)
**	    Added CS_SID to awaken a specific session.
**	17-Jan-2006 (toumi01)
**	    Try to trap and print diagnostics for unexpected signaling
**	    sanity check failures that cause sporadic SEGVs on calls
**	    to CS_synch_unlock and thence to pthread_mutex_unlock.
**	25-Jan-2006 (toumi01)
**	    For the above sanity check and diagnostics write the most
**	    serious message (skipping the unlock) to errlog.log.
**	01-Feb-2006 (toumi01)
**	    Further refine efforts to avoid SEGV on pthread_mutex_unlock
**	    by testing child condition before taking a mutex and bailing
**	    earlier out if it is not in the condition to receive the
**	    signal as the parent had planned. This also avoids unneeded
**	    messages to the dbms log when the situation is really not a
**	    problem, just an artifact of the loose parent / child
**	    coordination.
*/
STATUS
CSMTcnd_signal( CS_CONDITION *cnd, CS_SID sid )
{
    CS_SCB		*scb = NULL;
    CS_CONDITION	*cndp;

    if (!cnd || cnd->cnd_waiter != NULL )
        return FAIL;

    /* Caller must hold cnd_mtx */

    /* Signal specific thread? */
    if ( sid )
    {
	scb = CSMT_find_scb(sid);

	if ( scb && scb->cs_evcb )
	{
	    CS_SYNCH *lock_event_sem = &scb->cs_evcb->event_sem;

	    if ( scb->cs_sync_obj != (PTR)cnd ||
		 scb->cs_state != CS_CNDWAIT )
		return(FAIL);

	    CS_synch_lock( &scb->cs_evcb->event_sem );

	    /* Nothing to do if not waiting on this "cnd";  might have
	    ** already awakened this target and it's not moving yet.
	    */
	    if ( scb->cs_sync_obj != (PTR)cnd ||
		 scb->cs_state != CS_CNDWAIT )
	    {
		CS_SYNCH *unlock_event_sem = &scb->cs_evcb->event_sem;

		if (lock_event_sem == unlock_event_sem)
		{
		    CS_synch_unlock( unlock_event_sem );
		}
		else
		{
		    char	msg[150];
		    CL_ERR_DESC	error;
		    STprintf(&msg[0], "CSMTcnd_signal: lock_event_sem[%p] != unlock_event_sem[%p]; skipping CS_synch_unlock",
			lock_event_sem, unlock_event_sem);
		    TRdisplay("%s\n", &msg);
		    ERlog(msg, STlength(msg), &error);
		    TRdisplay("CSMTcnd_signal: scb: %p, scb->cs_sync_obj: %p, scb->cs_state: %d\n",
			scb, scb->cs_sync_obj, scb->cs_state);
		}
		return(FAIL);
	    }
	    cndp = scb->cs_cnd;
	}
	else
	    return(FAIL);
    }
    else if ( (cndp = cnd->cnd_next) != cnd )
    {
	/* Signal first thread on queue */
	scb = cndp->cnd_waiter;
	CS_synch_lock( &scb->cs_evcb->event_sem );
    }

    if ( scb && cndp )
    {
	/* Remove from condition queue */
	cndp->cnd_next->cnd_prev = cndp->cnd_prev;
	cndp->cnd_prev->cnd_next = cndp->cnd_next;

	/* Adjust target's state so that it knows that the wakeup is
	** for real -- not something spurious.  This also has the
	** effect of sending just ONE wakeup to a waiting process.
	** If the target is slow waking up, and the higher level
	** signaller goes to whack at it again, we'll see that the
	** target isn't in condition-wait any more and skip the
	** redundant signal.
	** This assumes of course that a condition wakeup will always
	** wake up the target, reliably!
	*/
	scb->cs_state = CS_COMPUTABLE;
	scb->cs_cnd = NULL;
	scb->cs_sync_obj = NULL;

	/* Wake it up */
	CS_cond_signal( &scb->cs_evcb->event_cond );
	CS_synch_unlock( &scb->cs_evcb->event_sem );
    }

    return OK;
}
/*{
** Name: CSMTcnd_broadcast - wake all waiters on a condition
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	17-Dec-2003 (jenjo02)
**	    Rearchitected to use condition queue rather than
**	    clumsy CS_cond_broadcast which induced all sorts
**	    of race conditions.
**	31-Jan-2006 (kschendel)
**	    Avoid redundant wakeups a la CSMTcnd_signal.
*/
STATUS
CSMTcnd_broadcast( CS_CONDITION	*cnd )
{
    CS_CONDITION	*cndq = cnd;
    CS_SCB		*scb;

    /* Caller must hold cnd_mtx */

    if ( !cnd || cnd->cnd_waiter != NULL )
        return FAIL;

    /* Drain the queue... */
    while ( (cndq = cndq->cnd_next) != cnd )
    {
	scb = cndq->cnd_waiter;

	/* Defend against vanishing threads */
	if (scb->cs_evcb != NULL)
	{
	    CS_synch_lock( &scb->cs_evcb->event_sem );

	    /* If target isn't in a wait state, we've probably already
	    ** hit it with a signal.  It should be waking up, no point
	    ** in doing this again.
	    */
	    if (scb->cs_sync_obj == (PTR) cnd
	      && scb->cs_state == CS_CNDWAIT)
	    {

		/* Adjust target's state so that it knows that this is the
		** real thing.  (See above CScnd_signal).
		*/
		scb->cs_state = CS_COMPUTABLE;
		scb->cs_cnd = NULL;
		scb->cs_sync_obj = NULL;

		/* Wake it up */
		if(scb->cs_evcb->event_state == EV_AST_WAIT_MASK)
#if defined(xCL_SUSPEND_USING_SEM_OPS)
		    sem_post(&scb->cs_evcb->event_wait_sem);
#else
		    ;
#endif
		else
		    CS_cond_signal( &scb->cs_evcb->event_cond );
	    }
	    CS_synch_unlock( &scb->cs_evcb->event_sem );
	}
    }

    /* Queue is empty now */
    cnd->cnd_next = cnd->cnd_prev = cnd;

    return (OK);
}
/*{
** Name: CSMTstatistics	- Fetch statistics of current thread or entire server
**
** Description:
**	This routine is called to return the ``at this moment'' statistics
**	for this particular thread or for the entire server.  It returns a
**	structure which holds the statistics count at the time the call is made.
**
**	If the 'server_stats' parameter is zero, then statistics are returned
**	for the current thread.	 If non-zero, then CSstatistics returns
**	the statistics gathered for the entire server.
**
**	The structure returned is a TIMERSTAT block, as defined in the <tm.h>
**	file in the CLF\TM component.	Thus, this calls looks very much
**	like a TMend() call.  There are two main differences.  The first is
**	that there is no equivalent to the TMstart() call.  This is done
**	implicitly at server and thread startup time.  Secondly, this routine
**	returns a TIMERSTAT block holding only the current statistics
**	count (TMend returns a TIMER block which holds three TIMERSTAT blocks
**	- one holding the starting statistics count (set at TMstart), one
**	holding the statistics count at TMend time, and one holding the
**	difference).
**
**	NOTE:  CPU statistics are not kept by default on a thread by thread
**	basis.	If CSstatistics is called for a thread not doing CPU statistics
**	then the stat_cpu field will be zero.  A caller wishing to collect CPU
**	times for a thread should first call CSaltr_session to start CPU
**	statistics collecting.	Subsequent CSstatistics calls will return the
**	CPU time used since that time.
**
** Inputs:
**	timer_block			The address of the timer block to fill
**	server_stats			Zero - return statistics for current
**					    session.
**					Non-zero - return statistics for entire
**					    server.
**
** Outputs:
**	*timer_block			Filled in with...
**	    .stat_cpu			the CPU time in milliseconds
**	    .stat_dio			count of disk i/o requests
**	    .stat_bio			count of communications requests
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
**	13-Feb-1987 (fred)
**	    Created.
**	21-oct-1988 (rogerk)
**	    Added server_stats parameter.
**	31-Oct-1989 (anton)
**	    modified quantum time sliceing
**	07-Dec-1990 (anton)
**	    Only add in cputime if session is keeping statistics Bug 35024
**	10-sep-98 (stephenb)
**	    Single threaded CPU requests currently pass back values for
**	    the whole server process when using OS threads (because we 
**	    can't control the context switching). Fix this using a threaded
**	    OS call.
**	24-Sep-1998 (jenjo02)
**	    Fixed up computation of thread CPU time, moved CS_thread_info()
**	    to CS_update_cpu().
**	17-Nov-1999 (jenjo02)
**	    Get thread CPU time if !server_stats, not based on CS_CPU_MASK.
*/
STATUS
CSMTstatistics(timer_block, server_stats)
TIMERSTAT	*timer_block;
i4		server_stats;
{
    CS_SCB	*scb;

    if (timer_block == 0)
	return(E_CS0004_BAD_PARAMETER);

    CSMTget_scb( &scb );

    if (server_stats)
    {
	/* Return statistics for entire server. */

	/* Update cpu time for server process */
	CS_update_cpu(&timer_block->stat_pgflts, (i4 *)0);

	timer_block->stat_dio = 
		Cs_srv_block.cs_wtstatistics.cs_dior_done +
		Cs_srv_block.cs_wtstatistics.cs_diow_done;
	timer_block->stat_bio = 
		Cs_srv_block.cs_wtstatistics.cs_bior_done +
		Cs_srv_block.cs_wtstatistics.cs_biow_done;

	/* multiply by 10 since cpu time is returned in 10ms ticks */
	timer_block->stat_cpu = 10 * Cs_srv_block.cs_cpu;
    }
    else
    {
	/* Return statistics for this session only. */

	/* Update cpu time for this thread */
	CS_update_cpu(&timer_block->stat_pgflts, &scb->cs_cputime);

	timer_block->stat_dio = scb->cs_dior + scb->cs_diow;
	timer_block->stat_bio = scb->cs_bior + scb->cs_biow;

	/* multiply by 10 since cpu time is returned in 10ms ticks */
	timer_block->stat_cpu = 10 * scb->cs_cputime;
    }

    return(OK);
}

/*{
** Name: CSMTattn - Indicate async event to CS thread or server
**
** Description:
**	This routine is called to indicate that an unexpected asynchronous
**	event has occurred, and that the CS module should take the appropriate
**	action.	 This is typically used in indicate that a user interrupt has
**	occurred, but may be used for other purposes.
**
** Inputs:
**	eid				Event identifier.  This identifies the
**					event type for the server.
**	sid				thread id for which the event occurred.
**					In some cases, the event may be for
**					the server, in which case this parameter
**					should be zero (0).
**
** Outputs:
**	none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-1987 (fred)
**	    Created on Jupiter.
**	19-Nov-1987 (fred)
**	    Added CS_TERMINATE check to prevent interrupts during session
**	    termination.
**	30-Nov-1987 (fred)
**	    Added eid parameter.
**	23-Oct-1992 (daveb)
**	    prototyped
**	26-apr-1993 (bryanp)
**	    Removed setting of Cs_resumed = 1 in the non-initializing case. See
**	    large comment at head of this file.
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	31-oct-1996 (canor01)
**	    Call cs_attn to set thread's flags before waking the thread.
**      13-dec-1996 (canor01)
**          If a session is waiting on an Ingres condition, wake it up
**          for shutdown.
**	24-aug-2001 (abbjo03)
**	    Back out change to tracking of cs_num_active (18-sep-2000) since it
**	    has side effects on subsequent invocations.
**	06-Feb-2003 (jenjo02)
**	    BUG 109586: Cleanup CS_CONDITION handling, mutex it
**	    while changing its state.
**	17-Dec-2003 (jenjo02)
**	    CS_CNDWAIT sessions are abortable any time, not just
**	    at shutdown.
**	12-Aug-2004 (jenjo02)
**	    Avoid potential CS_CNDWAIT mutex deadlock.
**	11-Sep-2006 (jonj)
**	    A FAIL return from CS_RCVR_EVENT (force-abort) instructs
**	    us to nothing else with the session's state or mask,
**	    just ignore the event and return.
**	23-Feb-2007 (kschendel) SIR 122890
**	    Check for cndwait first since interruptable doesn't matter when
**	    in condition-wait.  Set aborted before releasing event mutex,
**	    so that if session drops out of cndwait while we play with the
**	    mutexes, we don't lose the interrupt entirely.  (Never observed,
**	    but looks possible.)
*/
VOID
CSMTattn(i4 eid, CS_SID sid)
{
    CS_SCB		*scb = 0;

    if (sid != (CS_SID)0)
	scb = CSMT_find_scb(sid);
    CSMTattn_scb( eid, scb );
}
static VOID
CSMTattn_scb( i4  eid, CS_SCB *scb )
{
    if (scb && (scb->cs_mode != CS_TERMINATE))
    {
	CS_synch_lock( &scb->cs_evcb->event_sem );

        if ((*Cs_srv_block.cs_attn)(eid, scb))
        {
	    if ( eid == CS_RCVR_EVENT )
	    {
		/* DMF will handle this - do nothing */
		CS_synch_unlock( &scb->cs_evcb->event_sem );
		return;
	    }
            /* Unset the interrupt pending mask. */
            scb->cs_mask &= ~CS_IRPENDING_MASK;
        }

	if ( scb->cs_state == CS_CNDWAIT )
	{
	    CS_CONDITION	*cp;
	    CS_SEMAPHORE	*cnd_mtx;

	    /* remove from condition queue */
	    cp = scb->cs_cnd;
	    cnd_mtx = cp->cnd_mtx;

	    /*
	    ** There's a small yet annoying possibility of a 
	    ** mutex deadlock. All other uses of conditions
	    ** first take the cnd_mutex, then the event_sem.
	    ** To avoid a deadlock, release the event_sem,
	    ** take the cnd_mutex, then check that the
	    ** SCB is still in a condition wait on the
	    ** same condition; if not, don't signal the cvar.
	    ** Set ABORTED first so that it can be seen while
	    ** we unlatch the event mutex.
	    */
	    scb->cs_mask |= CS_ABORTED_MASK;
	    CS_synch_unlock( &scb->cs_evcb->event_sem );

	    if ( cp->cnd_mtx == cnd_mtx &&
	        (CSMTp_semaphore(1, cnd_mtx)) == OK )
	    {
		CS_synch_lock( &scb->cs_evcb->event_sem );

		if ( scb->cs_state == CS_CNDWAIT && cp == scb->cs_cnd )
		{
		    cp->cnd_next->cnd_prev = cp->cnd_prev;
		    cp->cnd_prev->cnd_next = cp->cnd_next;

		    scb->cs_state = CS_COMPUTABLE;
		    scb->cs_cnd = NULL;
		    scb->cs_sync_obj = NULL;
		    CS_cond_signal( &scb->cs_evcb->event_cond );
		}

		CS_synch_unlock( &scb->cs_evcb->event_sem );
		CSMTv_semaphore(cnd_mtx);
	    }
	    return;
	}
	else if ((scb->cs_state != CS_COMPUTABLE) &&
		(scb->cs_mask & CS_INTERRUPT_MASK))
	{
	    /*
	    ** This thread should be resumed.
	    ** By setting IRCV and signaling the event condition variable,
	    ** we break the session out of CSsuspend if it's in there.
	    **
	    ** cs_state gets hacked on without mutex protection by such
	    ** places as DI;  however they shouldn't be setting the
	    ** interrupt mask.  So, in theory, this can only happen if
	    ** the session is in CSMTsuspend or if it's in one of the
	    ** bio input/output sections in the csmthl main loop.
	    */

	    scb->cs_mask &= ~CS_INTERRUPT_MASK;
	    scb->cs_mask |= CS_IRCV_MASK;
	    scb->cs_state = CS_COMPUTABLE;
	    if (scb->cs_evcb->event_state & EV_WAIT_MASK)
	    {
		scb->cs_evcb->event_state &= ~EV_WAIT_MASK;
		CS_cond_signal( &scb->cs_evcb->event_cond );
	    }
#if defined(xCL_SUSPEND_USING_SEM_OPS)
	    else 
	        if (scb->cs_evcb->event_state & EV_AST_WAIT_MASK)
	        {
		    scb->cs_evcb->event_state &= ~EV_AST_WAIT_MASK;
		    sem_post( &scb->cs_evcb->event_wait_sem );
	        }
#endif
	}
	else /* DMF FORCE ABORT ISSUE if (scb->cs_state == CS_COMPUTABLE) */
	{
	    /*
	    ** Mark session has interrupt pending.
	    */
	    scb->cs_mask |= CS_IRPENDING_MASK;
	}
	CS_synch_unlock( &scb->cs_evcb->event_sem );
    }
	/* else thread is being deleted */
    return;
}

/*{
** Name: CSMTget_sid	- Get the current session id
**
** Description:
**	This routine returns the current session id.
**
** Inputs:
**
** Outputs:
**	*sidptr				    session id.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-1986 (fred)
**	    Created.
**	23-Oct-1992 (daveb)
**	    prototyped
**	20-jan-1997 (wonst02)
**	    Call CSMT_tls_get instead of ME_tls_get.
**	24-feb-1997 (canor01)
**	    For System V threads, session id == thread id.
**	24-Sep-1998 (jenjo02)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always 
**	    a pointer to the thread's SCB, never OS thread_id, which is 
**	    contained in cs_thread_id.
*/
VOID
CSMTget_sid(CS_SID *sidptr)
{
    CS_SCB	    **ppSCB;
    STATUS	    status;

    if (sidptr)
    {
	CSMT_tls_get( (PTR *)&ppSCB, &status );
#if defined(VMS)
        *sidptr = (status) ? 0 :
                  (!ppSCB) ? 0 : (*ppSCB)->cs_self;
#else
	*sidptr = (*ppSCB)->cs_self;
#endif
    }
}

/*{
** Name: CSMTget_cpid	- Get the current session's cross-process identity.
**
** Description:
**	This routine returns the current session's cross-process identity.
**
** Inputs:
**
** Outputs:
**	*cpid				    cross-process session id.
**		.sid				session id
**		.pid				process id
**		.wakeup				offset to shared memrory 
**						wakeup block.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Oct-1998 (jenjo02)
**	    created.
*/
VOID
CSMTget_cpid(CS_CPID *cpid)
{
    CS_SCB	    **ppSCB;
    STATUS	    status;

    if (cpid)
    {
	CSMT_tls_get( (PTR *)&ppSCB, &status );
#ifdef VMS
       if (ppSCB == NULL)
       {
          cpid->wakeup = 0;
          cpid->sid    = 0;
          cpid->pid    = Cs_srv_block.cs_pid;

          return;
       }
#endif
#ifdef USE_IDLE_THREAD
	cpid->wakeup = 0;
#else
	cpid->wakeup = ((i4)((char *)(*ppSCB)->cs_evcb -
			    (char *)Cs_sm_cb));
#endif /* USE_IDLE_THREAD */
	cpid->sid    = (*ppSCB)->cs_self;
	cpid->pid    = Cs_srv_block.cs_pid;
    }
}

/*{
** Name: CSMTget_scb  - Get the current scb
**
** Description:
**	This routine returns a pointer to the current scb.
**
** Inputs:
**
** Outputs:
**	*scbptr				 thread control block pointer.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-1986 (fred)
**	    Created.
**	23-Oct-1992 (daveb)
**	    prototyped
**      18-jun-93 (ed)
**          changed to match CL spec
**	20-jan-1997 (wonst02)
**	    Call CSMT_tls_get instead of ME_tls_get.
*/
VOID
CSMTget_scb(CS_SCB **scbptr)
{
    CS_SCB **ppSCB = NULL;
    CS_SCB *pSCB = NULL;
    STATUS status;

    if (scbptr)
    {
        CSMT_tls_get( (PTR *)&ppSCB, &status );
	if ( ppSCB )
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

/*{
** Name: CSMTfind_scb	- Given a session thread id, return the session CB.
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
CSMTfind_scb(CS_SID sid)
{
    return ( CSMT_find_scb( sid ) );
} /* CSfind_scb */


/*{
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
**/
CS_SID
CSMTfind_sid(CS_SCB *scb)
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
** History:
**	20-jan-1997 (wonst02)
**	    Call CSMT_tls_get instead of ME_tls_get.
**	10-jun-1997 (canor01)
**	    Allow multiple calls to CSMT_tls_init(), in case initial access
**	    comes in from an unexpected path.
**
****************************************************************************/
VOID
CSMTset_scb(CS_SCB *scb)
{
    CS_SCB **ppSCB;
    STATUS status;

    CSMT_tls_init(1, &status);
    CSMT_tls_get( (PTR *)&ppSCB, &status );
    if ( ppSCB == NULL )
    {
        ppSCB = (CS_SCB **) MEreqmem(0,sizeof(CS_SCB **),TRUE,NULL);
        CSMT_tls_set( (PTR)ppSCB, &status );
    }
    *ppSCB = scb;
}

/*{
** Name: CSMTset_sid	- Set the current thread id (DEBUG ONLY)
**
** Description:
**	This routine is provided for module tests only.	 It sets the current
**	thread to that provided.
**
** Inputs:
**	scb				thread id.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-1986 (fred)
**	    Created.
*/
VOID
CSMTset_sid(scb)
CS_SCB		   *scb;
{
    
    Cs_srv_block.cs_current = scb;
}

/*{
** Name: CSMTintr_ack	- Acknowledge receipt of an interrupt
**
** Description:
**	This routine turns off the CS_IRPENDING_MASK bit in the thread's
**	state mask.  This bit is set if a thread is informed of an interrupt
**	when it is running.  A typical use might be when a session sees
**	some facility-level flag indicating an interrupt, BEFORE it has
**	to CSsuspend which would return an interrupt status due to IRPENDING.
**	Such a session can ACK the interrupt, thus avoiding a spurious
**	interrupt-status return from a suspend during its abort processing.
**
** Inputs:
**	none
**
** Outputs:
**	none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Sep-1987 (fred)
**	    Created.
**	23-Feb-2007 (kschendel) SIR 122890
**	    Clear IRCV too, just in case.
*/
VOID
CSMTintr_ack()
{
    CS_SCB	*scb;

    CSMTget_scb( &scb );

    /*
    /* Safely clear the pending-interrupt flags.  IRCV shouldn't be on,
    ** but clear it too anyway as it has a meaning similar to IRPENDING.
    ** (IRCV shouldn't be on because it applies to suspended sessions.)
    */
    CS_synch_lock( &scb->cs_evcb->event_sem );
    scb->cs_mask &= ~(CS_IRPENDING_MASK | CS_IRCV_MASK);
    CS_synch_unlock( &scb->cs_evcb->event_sem );

}

/*{
** Name: CSMTaltr_session - Alter CS characteristics of session.
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
**		    (*(i4 *)item) is zero, CS will no longer collect CPU
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
**	    CS_AS_PRIORITY - Set priority to indicates value
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
**	None
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
**	21-oct-1988 (rogerk)
**	    Created.
**	07-Dec-1990 (anton)
**	    Clean up cpu measurement - was updateing cpu time when it should
**	    not.  Bug 35024
**	23-Oct-1992 (daveb)
**	    prototyped
**	22-oct-93 (robf)
**          Add CS_AS_PRIORITY
**	17-Nov-1999 (jenjo02)
**	    When turning on CPU collection, it's not necessary to
**	    call CS_update_cpu when thread information is available.
*/
STATUS
CSMTaltr_session(CS_SID session_id, i4  option, PTR item)
{
    CS_SCB	    *scb = 0;
    STATUS	    status;

    if (session_id)
	scb = CSMT_find_scb(session_id);
    else
	CSMTget_scb( &scb );

    if (scb == 0)
	return (E_CS0004_BAD_PARAMETER);

    switch (option)
    {
      case CS_AS_CPUSTATS:

	if (item && (*(i4 *)item))
	{
	    if ((scb->cs_mask & CS_CPU_MASK) == 0)
	    {
#ifndef CS_thread_info
		/*
		** If turning on cpu stats for this session for the first time,
		** update server cpu count so that this thread will get charged
		** for the correct cpu time the first time it is swapped out.
		*/
		CS_update_cpu((i4 *)0, (i4 *)0);
#endif /* CS_thread_info */

		/*
		** Set inkernel while changing the scb's cs_mask as this field
		** is also set by AST driven routines.
		*/

		CS_synch_lock( &scb->cs_evcb->event_sem );
		scb->cs_mask |= CS_CPU_MASK;
		CS_synch_unlock( &scb->cs_evcb->event_sem );

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

	    CS_synch_lock( &scb->cs_evcb->event_sem );
	    scb->cs_mask &= ~(CS_CPU_MASK);
	    scb->cs_cputime = 0;
	    CS_synch_unlock( &scb->cs_evcb->event_sem );

	}
	break;

      case CS_AS_PRIORITY:
	if (item && (*(i4 *)item))
	{
	    i4  priority= *(i4*)item;
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

	    CS_synch_lock( &scb->cs_evcb->event_sem );

	    /* Change priority */
	    scb->cs_priority = priority;
    	    CS_thread_setprio( scb->cs_thread_id, scb->cs_priority, &status );

	    CS_synch_unlock( &scb->cs_evcb->event_sem );

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
** Name: CSMTdump_statistics  - Dump CS statistics to Log File.
**
** Description:
**	This call is used to dump the statistics collected by the CS system.
**	It is intended to be used for performance monitoring and tuning.
**
**	Since there are no external requirements for what server statistics
**	are gathered by CS, the amount of information dumped by this routine
**	is totally dependent on what particular statistics are kept by this
**	version of CS.	Therefore, no mainline code should make any
**	assumptions about the information (if any) dumped by this routine.
**
**	This function will format text into a memory buffer and then use a
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
**	None
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
**	20-Sep-1988 (rogerk)
**	    Created.
**	31-oct-1088 (rogerk)
**	    Added print_fcn argument.
**	12-jun-1989 (rogerk)
**	    Added dumping of cross-process semaphore statistics.
**	27-Apr-1995 (jenjo02)
**	    Added total server cpu time to CSdump_statistics().
**  24-oct-1995 (thaju02/stoli02)
**		Added EVCBs wait count to CSdump_statistics().
**      7-jan-97 (stephenb)
**          Add active user high water mark to dump statistics.
**	24-Sep-1998 (jenjo02)
**	    Show server CPU time as seconds and fractions of seconds.
**	16-Nov-1998 (jenjo02)
**	    Added new stats for Log I/O, I/O reads/writes.
**	17-Nov-1999 (jenjo02)
**	    Call CS_update_cpu to get server cpu time.
**	03-Dec-1999 (vande02)
**	    Changed datatype from i4 to i4 on CSMTdump_statistics and
**	    added the arguments to match prototype in csmtlocal.h.  Also,
**	    changed the above external function reference in the same way.
**	14-Feb-2006 (kschendel)
**	    Remove some things (like idle stats) that no longer apply to MT,
**	    or aren't tracked any more.
*/
VOID
CSMTdump_statistics(i4 (*print_fcn)(char *ptr, ...))
{
    char	buffer[150];

    /* Update cpu time for server process */
    CS_update_cpu((i4 *)0, (i4 *)0);

    STprintf(buffer, "========================================================\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "SERVER STATISTICS:\n====== ==========\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Server CPU seconds: %d.%d\n",
	    Cs_srv_block.cs_cpu / 100, Cs_srv_block.cs_cpu % 100);
    (*print_fcn)(buffer);
    STprintf(buffer, "Active users HighWater mark: %d\n",
            Cs_srv_block.cs_hwm_active);
    (*print_fcn)(buffer);
    STprintf(buffer, "Semaphore Statistics:\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "--------- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "    Single process  Shared: %9u. Collisions: %9u.\n",
		Cs_srv_block.cs_smstatistics.cs_smss_count,
		Cs_srv_block.cs_smstatistics.cs_smssx_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "                      Excl: %9u. Collisions: %9u.\n",
		Cs_srv_block.cs_smstatistics.cs_smsx_count,
		Cs_srv_block.cs_smstatistics.cs_smsxx_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Multi process   Shared: %9u. Collisions: %9u.\n",
		Cs_srv_block.cs_smstatistics.cs_smms_count,
		Cs_srv_block.cs_smstatistics.cs_smmsx_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "                      Excl: %9u. Collisions: %9u.\n",
		Cs_srv_block.cs_smstatistics.cs_smmx_count,
		Cs_srv_block.cs_smstatistics.cs_smmxx_count);
    (*print_fcn)(buffer);
    STprintf(buffer, "--------- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Wait Statistics:\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "               Requests  Wait State  Zero Wait\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "               --------  ----------  ---------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "    BIOR:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_bior_done,
		    Cs_srv_block.cs_wtstatistics.cs_bior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_bior_done -
			Cs_srv_block.cs_wtstatistics.cs_bior_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    BIOW:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_biow_done,
		    Cs_srv_block.cs_wtstatistics.cs_biow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_biow_done -
			Cs_srv_block.cs_wtstatistics.cs_biow_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    DIOR:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_dior_done,
		    Cs_srv_block.cs_wtstatistics.cs_dior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_dior_done -
			Cs_srv_block.cs_wtstatistics.cs_dior_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    DIOW:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_diow_done,
		    Cs_srv_block.cs_wtstatistics.cs_diow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_diow_done -
			Cs_srv_block.cs_wtstatistics.cs_diow_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LIOR:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lior_done,
		    Cs_srv_block.cs_wtstatistics.cs_lior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lior_done -
			Cs_srv_block.cs_wtstatistics.cs_lior_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LIOW:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_liow_done,
		    Cs_srv_block.cs_wtstatistics.cs_liow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_liow_done -
			Cs_srv_block.cs_wtstatistics.cs_liow_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Log:     %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lg_done,
		    Cs_srv_block.cs_wtstatistics.cs_lg_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lg_done -
			Cs_srv_block.cs_wtstatistics.cs_lg_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LGevent: %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lge_done,
		    Cs_srv_block.cs_wtstatistics.cs_lge_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lge_done -
			Cs_srv_block.cs_wtstatistics.cs_lge_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Lock:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lk_done,
		    Cs_srv_block.cs_wtstatistics.cs_lk_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lk_done -
			Cs_srv_block.cs_wtstatistics.cs_lk_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LKevent: %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lke_done,
		    Cs_srv_block.cs_wtstatistics.cs_lke_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lke_done -
			Cs_srv_block.cs_wtstatistics.cs_lke_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Time:    %9u.  %9u. %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_tm_done,
		    Cs_srv_block.cs_wtstatistics.cs_tm_waits,
		    Cs_srv_block.cs_wtstatistics.cs_tm_done -
			Cs_srv_block.cs_wtstatistics.cs_tm_waits);
    (*print_fcn)(buffer);
    STprintf(buffer, "    EVCB:    %9u.  %9u.\n",
		    Cs_srv_block.cs_wtstatistics.cs_event_count,
		    Cs_srv_block.cs_wtstatistics.cs_event_wait);
    (*print_fcn)(buffer);
    STprintf(buffer, "========================================================\n");
    (*print_fcn)(buffer);
}

/*{
** Name: CSMTswitch - Poll for possible asynchronous messages
**
** Description:
**
**	** This is a deprecated routine if OS-threading (CS_is_mt). **
**	Don't use it;  use CScancelCheck instead.
**
**      This call will determine if the inquiring session has
**      used more than an allotted quantum of CPU and poll
**	for asynchronous messages if so.
**
**	The only requirement here is that the session be interruptable;
**	how often we check is less significant than the fact that we
**	do check. 
**
**      How this quantum measurement is managed, updated and
**      checked is system specific.  However, it is essential that
**      this call imposes "negligible" overhead.
**
**      When a session has consumed a quantum of the CPU resource and
**      has invoked CSswitch(), it will poll for asychronous messages.
**	This is the mechanism which allows a session to be interrupted
**	instead of running indefinitely without intervening CSMTsuspend-s
**	(which also detect interrupts).
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
**	Session may be interrupted.
**
** History:
**      11-dec-89 (anton)
**          Created.
**      20-apr-94 (mikem)
**          Bug #57043
**          Added a call to CS_checktime() to CSswitch().  This call
**          is responsible for keeping the pseudo-quantum clock up to date,
**          this clock is used to maintain both quantum and timeout queues in
**          CS.  See csclock.c for more information.
**      27-jan-1997 (canor01)
**          Function not needed with operating-system threads.
**	24-mar-1997 (canor01)
**	    Since the OS-thread version of the server is not in an
**	    async BIO wait state during every task switch, do the
**	    check during common programmed switch points.
**	11-apr-1997 (canor01)
**	    Restore the quantum check to limit calls to poll() to when
**	    the quantum has expired.
**	03-Nov-1998 (jenjo02)
**	    Rewrote to use real thread CPU consumption instead of unreliable
**	    and unthreadsafe shared memory clock.
**	29-Jan-2005 (schka24)
**	    Fix unit confusion, we were testing cs_ppid in ms units but
**	    storing 10ms ticks.
*/
VOID
CSMTswitch()
{
    if (Cs_srv_block.cs_state != CS_INITIALIZING)
    {
	/* Only check every system-wide numcalls_before_update */
	if ( ++Cs_sm_cb->css_clock.cscl_calls_since_upd ==
	        Cs_sm_cb->css_clock.cscl_numcalls_before_update )
	{
	    i4	thread_cpu_time;

	    Cs_sm_cb->css_clock.cscl_calls_since_upd = 0;

	    /* Get expended thread cpu time in 10ms ticks */
	    CS_update_cpu((i4 *)0, &thread_cpu_time);
	    /* Everything else is in ms units, convert now */
	    thread_cpu_time = thread_cpu_time * 10;

	    /* Don't check SCB until thread has used up the minimum */
	    if ( thread_cpu_time > Cs_srv_block.cs_quantum )
	    {
		CS_SCB	*scb;

		CSMTget_scb(&scb);

		/* Has thread used a quantum since last check? */
		if ( thread_cpu_time - scb->cs_ppid >=
			Cs_srv_block.cs_quantum )
		{
		    i4  	timeout = 0;

		    /* Remember CPU when we last polled */
		    scb->cs_ppid = thread_cpu_time;
		    VOLATILE_ASSIGN_MACRO(Cs_srv_block.cs_ticker_time,
					scb->cs_last_ccheck);

		    /* do a quick poll for asynchronous messages */
#ifdef VMS
                    CSMT_execute_asts( scb );
#else
		    iiCLpoll( &timeout );
#endif
		}
	    }
	}
    }
    return;
}

/*
** Name: CScancelCheck - Check for cancel from user thread's client
**
** Description:
**	One of the unfortunate realities of the current GCA design
**	for OS-threaded Unix is that a thread has to occasionally poll
**	for async cancel messages from its front-end client.
**	Traditionally, this has been done via calls to CSswitch.
**
**	CSswitch is not a very good solution, though, when all we
**	need is the occasional cancel check.  It runs on CPU time
**	usage, for historical reasons, and that requires a system
**	call.  To limit system calls, it only checks every N calls;
**	but since there's no parameters, N is system-wide.
**	The result is that when the server is busy, it issues
**	get-cpu-time system calls at a high rate, but at the same
**	time is very responsive to cancels.  When there's only one
**	or two queries running, it may take a while to make up the
**	N calls, and the server may seem very unresponsive to cancels.
**
**	This routine is an attempt to paper over some of the cancel
**	check issues, without serious overhaul (such as running all
**	session pipe fd's through a message distributor thread, or
**	coordinating with some sort of async-check thread while a
**	user thread is running a query).
**
**	The caller passes the CS session ID, which it should have
**	easily available in most if not all cases.  The session ID
**	is of course just the CS scb address, and we can quickly
**	compare a last-checked-time in the SCB with a current wall-
**	clock time maintained by a trivial time-ticker thread.
**	Every N seconds of elapsed (not CPU) time, a quick poll is
**	done.  Of course, it's up to the calling facility to check
**	its facility-specific "attention" flags afterward, to see if
**	anything interesting happened.
**
**	Note that there will be times when the session is polled
**	without updating the SCB last-checked time.  This is
**	perfectly OK and will simply result in an extra poll or two.
**	We're more interesting in avoiding bazillions of get-cpu-time
**	system calls while a session is churning thru some sort
**	of massive operation (a query plan or something).
**
**	Choice of when-to-check time N is of interest.  Historically,
**	this was indirectly controlled by the "quantum" config
**	parameter.  Quantum is defined to be in CPU milliseconds,
**	though, and it's not useful here.  Unless demand arises for
**	a configurable cancel-check timer, I'm just going to hard
**	code a reasonable looking value.
**
** Inputs:
**	sid			CS_SID thread's session ID;
**				NULL/zero allowed, causes immediate return.
**				The thread doesn't have to be a
**				real user thread.
**
** Outputs:
**	None
**
** Side Effects:
**	May call iiCLpoll, which may find its way into "expedited
**	completion", which ends up calling CSattn, which may set
**	various facility attention or interrupt flags.
**
** History*
**	13-Feb-2007 (kschendel)
**	    Cancel-responsiveness project: finally do something that is
**	    marginally better than CSswitch for cancel checks.
*/

/* Hardwired!! 4 seconds feels just about right to me. */
#define CANCEL_CHECK_TIME 4


void
CSMTcancelCheck(CS_SID sid)
{
#if defined(VMS)
    CSMTswitch();
#else
    CS_SCB *scb = (CS_SCB *) sid;
    u_i4 time_now;

    if (scb != NULL && sid != CS_ADDER_ID
      && Cs_srv_block.cs_state != CS_INITIALIZING)
    {
	VOLATILE_ASSIGN_MACRO(Cs_srv_block.cs_ticker_time, time_now);
	if (scb->cs_last_ccheck + CANCEL_CHECK_TIME <= time_now)
	{
	    i4 timeout = 0;

	    scb->cs_last_ccheck = time_now;
	    iiCLpoll(&timeout);
	}
    }
#endif
} /* CSMTcancelCheck */

/*{
** Name: CSnoresnow()   - Check if thread has been CSresumed
**
** Descriptions:
**     This routine was added initially as a diagnostic tool for debugging
**     bug 48904.
**
**	In CSMT land, this routine is pretty much the same as a call to
**	CScancelled.  It used to check for EDONE and write a diagnostic,
**	but since it was checking in the wrong place (!), the diagnostic
**	was never seen.  (CSMT puts EDONE in the event block event_state,
**	rather than in the scb cs_mask.)
**
** Inputs:
**     descrip    - Text to identidy position of the call.
**
** Outputs:
**     Prints message to ERRLOG if edone bit set for thread
**     Returns:
**      n Nothing
**
** Side effects:
**     If this routine is called at the wrong time, the thread may end
**     up hanging waiting for a CSresume that will never happen.
**
** History:
**	23-Feb-2007 (kschendel) SIR 122890
**	    Test EDONE in the right place, and only call cancelled if
**	    it was (unexpectedly) set.
**
*/
void
CSMTnoresnow(descrip, pos)
char *descrip;
int  pos;
{
    CS_SCB              *scb;

    CSMTget_scb( &scb );

    if (scb && scb->cs_evcb)
    {
	if (scb->cs_evcb->event_state & EV_EDONE_MASK)
	{
	    TRdisplay("%@ CSnoresnow(%s) found EDONE; state %x, cs_mask %x\n",
		descrip, scb->cs_evcb->event_state, scb->cs_mask);
	    CSMTcancelled(0);
	}
    }
}


static i4
CSMT_usercnt()
{
    int count;

    CSMTp_semaphore(0, &Cs_known_list_sem);
    count = Cs_srv_block.cs_user_sessions;
    CSMTv_semaphore(&Cs_known_list_sem);

    return (count);
}
# endif /* OS_THREADS_USED */
