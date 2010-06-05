/*
** Copyright (c) 2004 Ingres Corporation
**
*/
/*
NO_OPTIM=hp8_bls
*/
# include <compat.h>
# include <errno.h>
# include <gl.h>
# include <st.h>
# include <clconfig.h>
# include <systypes.h>
# include <clsigs.h>
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

# include <csinternal.h>
# include <csev.h>
# include <cssminfo.h>
# include <rusage.h>
# include <clpoll.h>
# include <machconf.h>
# include <pwd.h>
# include <diracc.h>
# include <handy.h>

# include "csmgmt.h"
# include "cslocal.h"
# include "cssampler.h"

# if defined(su4_u42) || defined(dr6_us5) || \
     defined(sparc_sol) || defined(su4_cmw)
void    CS_su4_setup();
# endif

# if defined(ds3_ulx) || defined(rmx_us5) || defined(pym_us5) || \
     defined(ts2_us5) || defined(sgi_us5) || defined(rux_us5)
int CS_ds3_setup()
{
        CS_setup();
        CS_eradicate();
}
# endif

#if defined(any_aix)
void CS_ris_setup()
{
	CS_setup();
	CS_eradicate();
}
FUNC_EXTERN  TYPESIG CSslave_danger( int );
#endif

#if defined(SIGDANGER) 
#ifdef xCL_011_USE_SIGVEC
FUNC_EXTERN TYPESIG  CSslave_danger( i4, i4,  struct sigcontext * );
#else
FUNC_EXTERN TYPESIG CSslave_danger( i4 );
#endif
#endif   /* SIGDANGER */



GLOBALREF       CS_SERV_INFO    *Cs_svinfo;

# if defined(sqs_ptx) || defined(usl_us5) || \
     defined(nc4_us5) || defined(dgi_us5) || defined(sos_us5) || \
     defined(sui_us5) || defined(int_lnx) || defined(int_rpl) || \
     defined(int_rpl)
void CS_sqs_pushf();
# endif /* sui_us5 etc. */

# if defined(ibm_lnx)
void CS_ibmsetup();
# endif /* ibm_lnx */

# if defined(hp3_us5)
GLOBALDEF bool CS_hp3_broken_tas;
#endif /* hp3_us5 */

#if defined(axp_osf) || defined(axp_lnx)
long CS_axp_setup()
{
        CS_setup();
        CS_eradicate();

	return 0;
}
# endif /* axp_osf */

/*
NO_OPTIM = dr6_us5 ibm_lnx int_lnx int_rpl i64_aix
*/

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
** 	    CSfind_sid() - Obtain thread id for the given control block
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
**	    CSadjust_counter() - Thread-safe counter adjustment.
**	    CSadjust_i8counter() - Thread safe i8 counter adjustment.
**
**          CSnoresnow() - Clear EDONE bit of the current session.
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
**	5-dec-96 (stephenb)
**	    Add code for performance profiling, new static cs_facilities
**	    and new routines CSfac_stats() and CScollect_stats().
**	12-dec-1996 (canor01)
**	    Add cs_facility field to CS_SCB to track facility for sampler
**	    thread.
**      13-dec-1996 (canor01)
**          If a session is waiting on an Ingres condition, wake it up
**          for shutdown.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          check Cs_srv_block to see if server is running OS threads.
**	    Remove MCT ifdefs, added CS_is_mt.
**	26-feb-1997 (canor01)
**	    Previous change for waking threads that are waiting on Ingres
**	    conditions did not put awakened thread back on ready queue.
**      28-feb-1997 (hanch04)
**	    Move Cs_known_list_sem here from csmt
**      06-mar-1997 (reijo01)
**          Changed a check for "ingres" to a check for SystemAdminUser.
**          Changed II_DBMS_SERVER message to use generic system name.
**	20-mar-1997 (canor01)
**	    Add support for CSsuspend() timeouts in nanoseconds.
**	25-Mar-1997 (jenjo02)
**	    Third parameter to CSsuspend() may be a ptr to a LK_LOCK_KEY 
**	    when sleep mask is CS_LOCK_MASK (for iimonitor) instead of an ecb.
**      02-may-1997 (canor01)
**          If a thread is created with the CS_CLEANUP_MASK set in
**          its thread_type, transfer the mask to the cs_cs_mask.
**          These threads will be terminated and allowed to finish
**          processing before any other system threads exit.
**	15-may-1997 (muhpa01)
**	    Changed calls to CS_create_thread() to CS_create_ingres_thread()
**	    so as not to conflict with OS_THREADS implementation. (hp8_us5 port)
**      23-may-1997 (reijo01)
**          Print out II_DBMS_SERVER message only if there is a listen address.
**	03-Jul-97 (radve01)
**	    Basic, debugging related info, printed to dbms log at system
**	    initialization.
**	29-jul-1997 (walro03)
**	    Update for Tandem NonStop (ts2_us5).
**	23-Sep-1997 (kosma01)
**	    ERsend (used only by AIX), did not have a flag argument. Added flag
**	    argument to indicate Operating System message.
**	01-oct-1997 (muhpa01)
**	    Initialize cs_scb_mutex & cs_cnd_mutex when compiled with
**	    OS_THREADS.  Needed when running with internal (ingres) threads
**	    on platforms requiring mutex initialization, like HP-UX.
**	7-jan-97 (stephenb)
**	    Add active user high water mark to dump statistics.
**	16-feb-1998 (toumi01)
**	    Added Linux (lnx_us5).
**	16-Feb-98 (fanra01)
**	    Add initialisation of the attach and detach functions.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread().
**      06-may-1998 (canor01)
**          Initialize CI function to retrieve user count.  This is only
**          valid in a server, so initialize it here.
**	07-jul-1998 (allmi01)
**	    Added entries for Silicon Graphics (sgi_us5).
**	17-sep-1998 (canor01)
**	    Add CS_atomic_increment() and CS_atomic_decrement() for 
**	    atomically incrementing and decrementing a CS_AINCR datatype.
**	05-Oct-1998 (jenjo02)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to
**	    correctly account for those types of suspends.
**	    In CSdump_statistics(), show server cpu time in seconds and 
**	    fractions of seconds.
**	16-Nov-1998 (jenjo02)
**	    Added a host of new SCB and servers statistics. Defined new
**	    suspend masks, Log I/O (CS_LIO_MASK) and flags to discriminate
**	    I/O reads from writes (CS_IOR_MASK, CS_IOW_MASK).
**	    New MO objects defined for new stats.
**	    Added new external function, CSget_cpid(), and structure,
**	    CS_CPID, used to identify a thread for cross-process 
**	    suspend/resume.
**      11-jan-1999 (canor01)
**          Replace duplicate error message.
**	01-Feb-1999 (jenjo02)
**	    Define cs_num_sessions to be the count of ALL threads, both internal
**	    system threads and external connected user threads.
**	    In CS_accept_connect(), check cs_user_sessions, the number of external 
**	    sessions, instead of cs_num_sessions, the total number of internal AND
**	    external sessions. With dynamic internal sessions, cs_num_sessions
**	    is no longer deterministic!
**	    Define cs_num_active to be the number of active CS_USER_THREAD sessions.
**	12-Mar-1999 (shero03)
**	    Set the static vector in FP with the address of CSget_scb.
**	22-Mar-1999 (kosma01)
**	    Remove declaration for TRdisplay(), on sgi_us5 its declaration
**	    conflicted with one from gl/hdr/hdr/tr.h.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	27-Apr-1999 (matbe01/ahaal01)
**	    Add casting to CS_thread_kill for POSIX_THREADS to eliminate
**	    compiler error.
**	10-may-1999 (walro03)
**	    Remove obsolete version string aco_u43, arx_us5, bu2_us5, bu3_us5,
**	    bul_us5, cvx_u42, dr3_us5, dr4_us5, dr5_us5, dr6_ues, dr6_uv1,
**	    dra_us5, ib1_us5, ix3_us5, odt_us5, ps2_us5, pyr_u42, rtp_us5,
**	    sqs_u42, sqs_us5, vax_ulx, x3bx_us5.
**	17-Jun-1999 (consi01)
**	    Cross integrate change 434533 for bug 89272 by natjo01 from 1.2
**	    code. When we receive SIGDANGER on AIX, call ERlog, not ERsend.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	24-Sep-1999 (hanch04)
**	    Added CSget_svrid to return the server name.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**      08-Feb-2001 (hanal04) Bug 103872 INGSRV 1368
**          CS_find_scb() now scans the cs_known_list to find an SCB.
**          Those SCBs which live outside of this list must be explicitly
**          tested for in CS_find_scb()
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**      07-Jan-2000 (allmi01)
**          For Ingres internal threads, rux_us5 need to set SIGVTALRM to
**          ignore.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**      14-feb-2000 (hweho01)
**          Added ris_u64 for AIX 64-bit platform.
**      16-May-2000 (hweho01)
**         1) Fixed the argument mismatch in ERlog() call. 
**         2) Removed the casting in CS_thread_kill() call. It was  
**            needed for AIX 4.2, but now it caused error on the 
**            current release.
**	14-jun-2000 (hanje04)
**	    Added Linux for OS/390 (ibm_lnx)
**	28-jul-2000 (somsa01)
**	    When printing out the server id, use the new SIstd_write()
**	    function.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Sep-2000 (hanje04)
**	   Added axp_lnx (Alpha Linux)
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
**	    macros. See cs.h for a complete list. Deleted FUNC_EXTERNs
**	    which are properly defined elsewhere.
**	18-oct-2000 (toumi01)
**	    Join the crowd and add NO_OPTIM for S/390 Linux (ibm_lnx).
**	    Init the admin thread scb saved stack pointer for ibm_lnx.
**	23-mar-2001 (devjo01)
**	    Put in calls to CS_wake, to jog server if a session
**	    becomes runable while server is idling.
**	16-aug-2001 (toumi01)
**	    add support for i64_aix
**	    no-op SIGDANGER for now (doesn't compile) - FIXME !!!
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      03-Dec-2001 (hanje04)
**          Added support for IA64 Linux.
**	22-May-2002 (jenjo02)
**	    Resolve long-standing inconsistencies and inaccuracies with
**	    "cs_num_active" by computing it only when needed by MO
**	    or IIMONITOR. Added MO methods, external functions
**	    CS_num_active(), CS_hwm_active(), callable from IIMONITOR,
**	    e.g. Works for both MT and Ingres-threaded servers.
**	29-Aug-2002 (devjo01)
**	    Added 'CSadjust_counter()'.
**	03-Oct-2002 (hanch04)
**	    Don't allow internal threads on Solaris 64-bit
**	03-Oct-2002 (somsa01)
**	    In IICSdispatch(), if cs_startup fails, make sure we print
**	    out "FAIL".
**      11-Oct-2002 (hweho01)
**          Added prototype definition of CSslave_danger(). 
**	12-nov-2002 (horda03) Bug 105321 INGSRV 1502
**	    Avoid the possibility of a spurious Resume, by removing any
**	    Cross Process resumes which are pending in the Wakeup block,
**	    as part of a CScancelled().
**      14-Nov-2002 (hanch04)
**          Removed NO_OPTIM for Solaris
**      07-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**	13-Feb-2003 (hanje04)
**	    BUG 109555
**	    In CSinitiate() after obtaining server PID set PGID on Linux
**	    (with setpgrp()) so that it is set correctly when using INTERNAL
**	    THREADS.
**	07-July-2003 (hanje04)
**	    BUG 110542 - Linux Only
**	    Enable use of alternate stack for SIGNAL handler in the event
** 	    of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**	    the alternate stack to be malloc'd so declare it on IICSdispatch
**	    stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**	    for OS threads.
**	21-Jul-2003 (hanje04)
** 	    BUG 10995
**	    Define server PID to always be PGID on Linux.
**	29-Sep-2003 (bonro01)
**	    Don't allow internal threads on HP Itanium 64-bit
**	27-Oct-2003 (hanje04)
**	    BUG 110542 - Linux Only - Ammendment
**	    Minor changes to Linux altstack code to get it working on S390
**	    Linux as well. Increase altstack size for INTERNAL THREADS in 
**	    CSdispatch.
**      27-Oct-2003 (hanje04) Bug 111164
**          For reasons unknown, the stored return address for threads on
**          S390 Linux was being set to CS_ibmsetup - 2 and not CS_ibmsetup.
**          Whilst this did work for a significant amount of time it now
**          causes the the server to return to the wrong fuction when switching
**          context on startup and everything falls appart.
**          Save return address as CS_ibmsetup for S390 Linux.
**	06-Oct-2003 (bonro01)
**	    Don't allow internal threads on 64-bit SGI.
**	4-Apr-2004  (mutma03)
**	    Added IICSpoll_for_AST to poll threads which missed AST due to 
**	    suspend/resume race condition.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	2-Sep-2004  (mutma03)
**	    Replaced IICSpoll_for_AST with IICSsuspend_for_AST
**	    which suspends the thread for a completion AST using sem_wait.
**      15-Mar-2005 (bonro01)
**          Added test for NO_INTERNAL_THREADS to
**          support Solaris AMD64 a64_sol.
**	06-Oct-2005 (hweho01)
**	    Don't allow internal threads on 64-bit AIX.
**      11-Jan-2007 (wonca01) BUG 117262
**          Correct cs_elog() parameter list to use CL_ERR_DESC* as
**          2nd parameter.
**	13-Feb-2007 (kschendel)
**	    Added CScancelCheck stuff, is just CSswitch on internal threads.
**	4-Oct-2007 (bonro01)
**	    Defined CS_dump_stack() in cslocal.h
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	21-Aug-2009 (kschendel) 121804
**	    Properly declare CS_cpres_event_handler.
**	07-Sep-2009 (frima01) SIR 122138
**	    Replace thr_hpux with any_hpux to be more generic.
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**      10-Feb-2010 (smeke01) b123249
**          Changed MOintget to MOuintget for unsigned integer values.
**      20-apr-2010 (stephenb)
**          add new function CSadjust_i8counter.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    CS_swuser();	/* suspend one thread, resume one */ 
FUNC_EXTERN STATUS  CS_setup();
FUNC_EXTERN i4	    CSsigterm();	/* terminate due to Unix signal */
FUNC_EXTERN STATUS  CS_set_server_connect();
FUNC_EXTERN VOID    CS_move_async();
FUNC_EXTERN i4	    CS_cpres_event_handler(void);
FUNC_EXTERN STATUS    CSoptions( CS_SYSTEM *cssb );
FUNC_EXTERN void    CS_clear_wakeup_block(PID pid, CS_SID sid);
typedef void FP_Callback(CS_SCB **scb);
FUNC_EXTERN void    FP_set_callback( FP_Callback fun);

#if defined(any_aix)
FUNC_EXTERN     TYPESIG CSsigdanger();       /* handler for sigdanger */
# endif /* aix */

FUNC_EXTERN VOID CS_mo_init(void);
FUNC_EXTERN VOID iiCLintrp();
FUNC_EXTERN VOID    CS_wake( PID pid );	
i4 CS_addrcmp( const char *a, const char *b );

#if defined(OS_THREADS_USED)
FUNC_EXTERN void CSMTnoresnow(char *, int);
#endif


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

static i4   CS_usercnt( void );


/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF CS_SYSTEM	      Cs_srv_block ZERO_FILL; /* Overall ctl struct */
GLOBALDEF CS_SEMAPHORE        Cs_known_list_sem              ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_queue_hdrs[CS_LIM_PRIORITY] ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_known_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_to_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_wt_list_hdr		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_as_list_hdr		     ZERO_FILL;
GLOBALDEF CS_STK_CB	      Cs_stk_list_hdr		     ZERO_FILL;

/* SCBs not in the Cs_known_list must be checked in CS_find_scb() */
GLOBALDEF CS_SCB	      Cs_idle_scb		     ZERO_FILL;
GLOBALDEF CS_SCB	      Cs_repent_scb		     ZERO_FILL;
GLOBALDEF CS_ADMIN_SCB	      Cs_admin_scb		     ZERO_FILL;
GLOBALDEF CS_SYNCH	      Cs_utility_sem	             ZERO_FILL;

# ifdef OS_THREADS_USED
/*
** This defines the CS/CSMT function vector table. Depending on
** whether Ingres is configured to run with Ingres or OS threads
** either CSinitiate() or CSMTinitiate() will initialize it with
** the appropriate function pointers.
*/
static    CS_FUNCTIONS	      CS_functions		     ZERO_FILL;
GLOBALDEF CS_FUNCTIONS	      *Cs_fvp = &CS_functions;
# endif /* OS_THREADS_USED */

GLOBALREF CS_SMCNTRL	      *Cs_sm_cb;
GLOBALREF i4	      Cs_lastquant;
GLOBALREF i4		      Cs_incomp;
GLOBALREF CS_SEMAPHORE        *ME_page_sem;
GLOBALREF VOID                (*Ex_print_stack)();
GLOBALREF VOID                (*Ex_diag_link)();

static i4		      got_ex	ZERO_FILL;
static bool		      Cs_resumed ZERO_FILL;
static i4		      dio_resumes, fast_resumes;

GLOBALDEF i4	      Cs_numprocessors = DEF_PROCESSORS;
GLOBALDEF i4	      Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
GLOBALDEF i4	      Cs_desched_usec_sleep = DEF_DESCHED_USEC_SLEEP;
/*
** The following GLOBALDEFs are used by the CSp|v_semaphore macros
** to locate the current thread's SCB and pid.
** The companion GLOBALREFs can be found in cl!hdr!hdr csnormal.h, 
** which also houses the macros.
*/
GLOBALDEF CS_SCB	      **Cs_current_scb = &(Cs_srv_block.cs_current);
GLOBALDEF i4      	      *Cs_current_pid = &(Cs_srv_block.cs_pid);

GLOBALREF char		      cs_server_class[];


static MO_GET_METHOD CSmo_bio_done_get;
static MO_GET_METHOD CSmo_bio_waits_get;
static MO_GET_METHOD CSmo_bio_time_get;
static MO_GET_METHOD CSmo_dio_done_get;
static MO_GET_METHOD CSmo_dio_waits_get;
static MO_GET_METHOD CSmo_dio_time_get;
static MO_GET_METHOD CSmo_lio_done_get;
static MO_GET_METHOD CSmo_lio_waits_get;
static MO_GET_METHOD CSmo_lio_time_get;
static MO_GET_METHOD CSmo_num_active_get;
static MO_GET_METHOD CSmo_hwm_active_get;

GLOBALDEF MO_CLASS_DEF CS_int_classes[] =
{
  /* These three need get methods to derive their values */

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bio_time",
	0, MO_READ, 0,
	0, CSmo_bio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bio_waits",
	0, MO_READ, 0,
	0, CSmo_bio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bio_done",
	0, MO_READ, 0,
	0, CSmo_bio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dio_time",
	0, MO_READ, 0,
	0, CSmo_dio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dio_waits",
	0, MO_READ, 0,
	0, CSmo_dio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dio_done",
	0, MO_READ, 0,
	0, CSmo_dio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lio_time",
	0, MO_READ, 0,
	0, CSmo_lio_time_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lio_waits",
	0, MO_READ, 0,
	0, CSmo_lio_waits_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lio_done",
	0, MO_READ, 0,
	0, CSmo_lio_done_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.num_active",
	0, MO_READ, 0,
	0, CSmo_num_active_get, MOnoset, (PTR)0, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.hwm_active",
	0, MO_READ, 0,
	0, CSmo_hwm_active_get, MOnoset, (PTR)0, MOcdata_index },

  /* Standard methods */

# if 0
  { 0, "exp.clf.unix.cs.debug", sizeof(CSdebug),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&CSdebug, MOcdata_index },
# endif

  { 0, "exp.clf.unix.cs.lastquant", sizeof(Cs_lastquant),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&Cs_lastquant, MOcdata_index },

  { 0, "exp.clf.unix.cs.dio_resumes", sizeof(dio_resumes),
	MO_READ, 0, 0, MOintget, MOnoset,
	(PTR)&dio_resumes, MOcdata_index },

  { 0, "exp.clf.unix.cs.num_processors", sizeof(Cs_numprocessors),
	MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&Cs_numprocessors, MOcdata_index },

  { 0, "exp.clf.unix.cs.max_sem_loops", sizeof(Cs_max_sem_loops),
	MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&Cs_max_sem_loops, MOcdata_index },

  { 0, "exp.clf.unix.cs.desched_usec_sleep", sizeof(Cs_desched_usec_sleep),
	MO_READ|MO_WRITE, 0, 0, MOintget, MOintset,
	(PTR)&Cs_desched_usec_sleep, MOcdata_index }, 

  { 0, "exp.clf.unix.cs.srv_block.scballoc",
	sizeof( Cs_srv_block.cs_scballoc ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_scballoc ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.scbdealloc",
	sizeof( Cs_srv_block.cs_scbdealloc ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_scbdealloc ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.elog",
	sizeof( Cs_srv_block.cs_elog ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_elog ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.process",
	sizeof( Cs_srv_block.cs_process ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_process ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.attn",
	sizeof( Cs_srv_block.cs_attn ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_attn ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.startup",
	sizeof( Cs_srv_block.cs_startup ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_startup ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.shutdown",
	sizeof( Cs_srv_block.cs_shutdown ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_shutdown ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.format",
	sizeof( Cs_srv_block.cs_format ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_format ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.saddr",
	sizeof( Cs_srv_block.cs_saddr ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_saddr ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.reject",
	sizeof( Cs_srv_block.cs_reject ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_reject ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.disconnect",
	sizeof( Cs_srv_block.cs_disconnect ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_disconnect ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.read",
	sizeof( Cs_srv_block.cs_read ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_read ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.write",
	sizeof( Cs_srv_block.cs_write ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_write ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.max_sessions",
	sizeof( Cs_srv_block.cs_max_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_max_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.num_sessions",
	sizeof( Cs_srv_block.cs_num_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_num_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.user_sessions",
	sizeof( Cs_srv_block.cs_user_sessions ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_user_sessions ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.max_active",
	sizeof( Cs_srv_block.cs_max_active ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_max_active ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.cursors",
	sizeof( Cs_srv_block.cs_cursors ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_cursors ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.stksize",
	sizeof( Cs_srv_block.cs_stksize ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stksize ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.stk_count",
	sizeof( Cs_srv_block.cs_stk_count ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stk_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.ready_mask",
	sizeof( Cs_srv_block.cs_ready_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_ready_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.known_list",
	sizeof( Cs_srv_block.cs_known_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_known_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wt_list",
	sizeof( Cs_srv_block.cs_wt_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_wt_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.to_list",
	sizeof( Cs_srv_block.cs_to_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_to_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.current",
	sizeof( Cs_srv_block.cs_current ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_current ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.next_id",
	sizeof( Cs_srv_block.cs_next_id ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_next_id ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.error_code",
	sizeof( Cs_srv_block.cs_error_code ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_error_code ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.state",
	sizeof( Cs_srv_block.cs_state ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_state ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.mask",
	sizeof( Cs_srv_block.cs_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.aquantum",
	sizeof( Cs_srv_block.cs_aquantum ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.aq_length",
	sizeof( Cs_srv_block.cs_aq_length ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.bquantum0",
	sizeof( Cs_srv_block.cs_bquantum[0] ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum[0] ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.bquantum1",
	sizeof( Cs_srv_block.cs_bquantum[1] ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_aquantum[1] ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.toq_cnt",
	sizeof( Cs_srv_block.cs_toq_cnt ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_toq_cnt ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.q_per_sec",
	sizeof( Cs_srv_block.cs_q_per_sec ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_q_per_sec ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.quantums",
	sizeof( Cs_srv_block.cs_quantums ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_quantums ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.idle_time",
	sizeof( Cs_srv_block.cs_idle_time ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_idle_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.cpu",
	sizeof( Cs_srv_block.cs_cpu ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_cpu ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.svcb",
	sizeof( Cs_srv_block.cs_svcb ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_svcb ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.stk_list",
	sizeof( Cs_srv_block.cs_stk_list ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_stk_list ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.gca_name",
	sizeof( Cs_srv_block.cs_name ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_name ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.pid",
	sizeof( Cs_srv_block.cs_pid ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_pid ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.event_mask",
	sizeof( Cs_srv_block.cs_event_mask ),
	MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_event_mask ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smsx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smsx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smxx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smxx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smxx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smcx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smcx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smcx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smx_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smx_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smx_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.sms_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_sms_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_sms_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smc_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smc_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smc_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smcl_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smcl_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smcl_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index }, 

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.cs_smsp_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smsp_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsp_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index }, 

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.cs_smmp_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smmp_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmp_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index }, 

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.cs_smnonserver_count",
	sizeof( Cs_srv_block.cs_smstatistics.cs_smnonserver_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smnonserver_count ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index }, 

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bio_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bio_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bio_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.bior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_bior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_bior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.biow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.biow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.biow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_biow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_biow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dio_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dio_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dio_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.dior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_dior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_dior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.diow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.diow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.diow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_diow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_diow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lior_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lior_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lior_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lior_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lior_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.liow_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.liow_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.liow_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_liow_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_liow_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lg_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lg_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lg_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lg_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lg_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lg_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lge_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lge_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lge_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lge_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lge_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lge_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lge_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lge_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lge_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lk_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lk_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lk_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lk_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lk_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lk_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lke_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lke_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lke_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lke_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lke_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lke_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.lke_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_lke_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_lke_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.tm_time",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_time ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_time ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.tm_waits",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_waits ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_waits ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.tm_idle",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_idle ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_idle ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.tm_done",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_tm_done ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_tm_done ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.event_wait",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_event_wait ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_event_wait ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.wait_stats.event_count",
	sizeof( Cs_srv_block.cs_wtstatistics.cs_event_count ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_wtstatistics.cs_event_count ),
	MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

# if 0
  { 0, "exp.clf.unix.cs.srv_block.optcnt",
	sizeof( Cs_srv_block.cs_optcnt ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_optcnt ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
# endif

# if 0
  /* Need CS_options_idx and MOdouble_get methods */

  { 0, "exp.clf.unix.cs.srv_block.options.index",
	sizeof( Cs_srv_block.cs_option.cso_index ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_index ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.unix.cs.srv_block.options.value",
	sizeof( Cs_srv_block.cs_option.cso_value ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_value ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.unix.cs.srv_block.options.facility",
	sizeof( Cs_srv_block.cs_option.cso_facility ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_facility ),
	MOintget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.unix.cs.srv_block.options.strvalue",
	sizeof( Cs_srv_block.cs_option.cso_strvalue ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_strvalue ),
	MOstrget, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },

  { 0, "exp.clf.unix.cs.srv_block.options.float",
	sizeof( Cs_srv_block.cs_option.cso_float ),
	MO_READ, 0,
	CL_OFFSETOF( CS_SYSTEM, cs_option.cso_float ),
	MOdouble_get, MOnoset, (PTR)&Cs_srv_block, CS_options_idx },
# endif

# ifdef OS_THREADS_USED
  { 0, "exp.clf.unix.cs.srv_block.facility",
        sizeof( Cs_srv_block.cs_facility ),
        MO_READ, 0, CL_OFFSETOF( CS_SYSTEM, cs_facility ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },

  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smssx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smssx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smssx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smsxx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smsxx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsxx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smsx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smsx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smsx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smss_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smss_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smss_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smmsx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmsx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmsx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smmxx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmxx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmxx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.smmx_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smmx_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smmx_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
 
  { 0, "exp.clf.unix.cs.srv_block.sem_stats.cs_smms_count",
        sizeof( Cs_srv_block.cs_smstatistics.cs_smms_count ),
        MO_READ, 0,
        CL_OFFSETOF( CS_SYSTEM, cs_smstatistics.cs_smms_count ),
        MOuintget, MOnoset, (PTR)&Cs_srv_block, MOcdata_index },
# endif /* OS_THREADS_USED */

  { 0 }
};

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
**	18-nov-1996 (canor01)
**	    Update parameters passed to iiCLgetpwnam().
**      06-mar-1997 (reijo01)
**          Changed a check for "ingres" to a check for SystemAdminUser.
**	04-Oct-2000 (jenjo02)
**	    Initialize CS_FUNCTIONS if running Ingres threads.
**	12-Feb-2001 (toumi01)
**	    Override stack size if Linux and linked with -pthread but
**	    running with II_THREAD_TYPE=INTERNAL.
**	    Also, add NO_OPTIM for int_lnx (joining ibm_lnx, et al.)
**	    because without it we core dump on rlimit call running csphil.
**	22-jan-2002 (devjo01)
**	    Init new IIresume_from_AST member with IICSresume, since
**	    for internal threads same processing is done for both
**	    normal resumes and resumes called from an AST/signal handler.
**      13-Feb-2003 (hanje04)
**          BUG 109555
**          In CSinitiate() after obtaining server PID set PGID on Linux
**          (with setpgrp()) so that it is set correctly when using INTERNAL
**          THREADS.
**       7-Oct-2003 (hanal04) Bug 110889 INGSRV2521
**          Calling EXsetclient here is too late. It needs to be done
**          before any EXdeclare(), or EXsetup() call.
**          Ignoring SIGINT here is not limited to EX_INGRES_DBMS server 
**          types. auditdb needs to be able to catch SIGINT and is an
**          EX_INGRES_TOOL. Moved EXsetclient() call to scd_main()
**          and moved the SIG_IGN for SIGINT to i_EXestablish().
**	30-Aug-2004 (hanje04)
**	    BUG 112936.
**	    IIDBMS SEGV's creating database when II_THREAD_TYPE set to INTERNAL.
**	    Tempory workaround for GA, silently dissable INTERNAL THREADS. i.e.
**	    ingore II_THREAD_TYPE and always use OS threads. FIX ME!!
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	 10-Aug-2007 (hweho01)
**	    Don't allow internal threads on Solaris AMD.
*/
STATUS
CSinitiate(i4 *argc, char ***argv, CS_CB *ccb )
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
    char		pwnam_buf[512];
    i4			fd_wanted;

#if defined(hp3_us5)
# include <sys/utsname.h>
    struct utsname      hp_uname;
#endif /* hp3_us5 */

    /* check for native threads */
    Cs_srv_block.cs_mt = FALSE;

    /* initialize known semaphore lists */
    QUinit(&Cs_srv_block.cs_multi_sem);
    QUinit(&Cs_srv_block.cs_single_sem);
 
# ifdef OS_THREADS_USED
    Cs_srv_block.cs_mt = TRUE;
    CS_synch_init( &Cs_srv_block.cs_semlist_mutex );

    cp = NULL;
/* 
** FIX ME!! iidbms SEGV's with INTERNAL threads, 
** temorarily disable
*/
# ifndef LNX
    NMgtAt( "II_THREAD_TYPE", &cp );
    if ( cp && *cp )
    {
	if ( STcasecmp(cp, "INTERNAL" ) == 0 )
	{
	    Cs_srv_block.cs_mt = FALSE;
#if (defined (sparc_sol) || defined(i64_hpu) || defined(sgi_us5) || defined(r64_us5) || defined(a64_sol)) && \
	defined(LP64)
	    {
    	        /*
    	        ** We can't run internal threads with a 64-bit 
		** Solaris server
    	        return( E_CL2557_CS_INTRN_THRDS_NOT_SUP );
    	        */
    
    	        char        buf[ER_MAX_LEN];
    	        i4          msg_language = 0;
    	        i4          msg_length;
    	        i4          length;
    	        CL_ERR_DESC error;

    	        Cs_srv_block.cs_mt = TRUE;
    	    	ERlangcode( (char *)NULL, &msg_language );
    	        ERslookup(
                            E_CL2557_CS_INTRN_THRDS_NOT_SUP,
                            (CL_ERR_DESC *)0,
                            ER_TIMESTAMP,
                            (char *)NULL,
                            buf,
                            sizeof(buf),
                            msg_language,
                            &msg_length,
                            &error,
                            0,
                            (ER_ARGUMENT *)0 ) ;
    	        ERsend(ER_ERROR_MSG, buf, STlength(buf), &error);
    	    }
#endif /* su9_us5 */
        }
    }

# endif /* NO INTERNAL THEADS ON LINUX */

    if ( (Cs_srv_block.cs_mt) )
    {
        return( CSMTinitiate( argc, argv , ccb ) );
    }

#ifdef LNX
{
    /*
    ** Linux programs linked with -pthread are constrained by glibc to
    ** a hard-coded stack size of (2*1024*1024)-(2* __getpagesize()),
    ** but we are not actually running threaded.  Therefore, override
    ** this setting with a hard-coded value that is the equivalent of
    ** the default Linux stack size of (8*1024*1024).
    */
    struct rlimit lim;
    lim.rlim_cur = (8*1024*1024);
    setrlimit(RLIMIT_STACK, &lim);
}
#endif /* Linux */

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

# endif /* OS_THREADS_USED */

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
        TRdisplay("CSinitiate: old uid: %d, old euid %d\n",
			getuid(), geteuid());
#endif

        if( !( p_pswd = iiCLgetpwnam( SystemAdminUser, &pswd,
				      pwnam_buf, sizeof(pwnam_buf) ) ) )
        {
            TRdisplay("CSinitiate: getpwnam() failure\n");
            return(E_CS0035_GETPWNAM_FAIL);
        }

	if (seteuid(p_pswd->pw_uid) == -1)
	{
	    TRdisplay("CSinitiate: seteuid() error %d\n", errno);
	    return(E_CS0036_SETEUID_FAIL);
	}

#ifdef xDEBUG
        TRdisplay("CSinitiate: euid changed to %d\n", geteuid());
#endif

    }
#endif

    NMgtAt("II_PROF", &cp);
    if (cp && *cp)
    {
	CS_recusage();
	CSprofstart();
    }

    if (cssb->cs_state)
	return(OK);

#if defined(LNX)
    setpgrp();
#endif
    /*
    ** Store Process Id of this server process.
    */
    PCpid(&Cs_srv_block.cs_pid);

#if defined(hp3_us5)
    /*
    ** Must be set before any calls to CStas()
    */
    uname(&hp_uname);
    CS_hp3_broken_tas = (hp_uname.machine[6] == '1'
                        || hp_uname.machine[6] == '2') ? 1 : 0;
#endif /* hp3_us5 */

    cssb->cs_state = CS_INITIALIZING;

    if (status = CS_map_sys_segment(&errcode))
    {
	TRdisplay("CSinitiate: Error (%x) in CS_map_sys_segment\n", status);
	return(E_CS0037_MAP_SSEG_FAIL);
    }

    if (ii_sysconf(IISYSCONF_PROCESSORS, &Cs_numprocessors))
	Cs_numprocessors = 1;
    if (ii_sysconf(IISYSCONF_MAX_SEM_LOOPS, &Cs_max_sem_loops))
	Cs_max_sem_loops = DEF_MAX_SEM_LOOPS;
    if (ii_sysconf(IISYSCONF_DESCHED_USEC_SLEEP, &Cs_desched_usec_sleep))
	Cs_desched_usec_sleep = DEF_DESCHED_USEC_SLEEP;

    /* No MO in pseudo-servers */

    CS_mo_init();
    (void) MOclassdef( MAXI2, CS_int_classes );

    if (ccb == NULL)
    {	/* a pseudo-server */
	status = CS_event_init(argc ? *argc : 0, 0);
	if (status)
	{
	    TRdisplay("CSinitiate: Error (%x) in CS_event_init\n", status);
	    status = E_CS0038_CS_EVINIT_FAIL;
	}
	return(status);
    }

    /* Add ASYNC later {@@fix_me@@} */
    ERinit(0, 0, 0, 0, 0);

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
    }

    /* Set up known element trees for SCBs and Conditions.
       Semaphore were handled by CS_mo_init before the
       pseudo-server early return so there sems would
       be known. */

# ifdef OS_THREADS_USED
    CS_synch_init( &Cs_srv_block.cs_scb_mutex );
# endif
    SPinit( &Cs_srv_block.cs_scb_tree, CS_addrcmp );
    Cs_srv_block.cs_scb_ptree = &Cs_srv_block.cs_scb_tree;
    Cs_srv_block.cs_scb_tree.name = "cs_scb_tree";
    MOsptree_attach( Cs_srv_block.cs_scb_ptree );

# ifdef OS_THREADS_USED
    CS_synch_init( &Cs_srv_block.cs_cnd_mutex );
# endif
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
    cssb->cs_format = ccb->cs_format;
    cssb->cs_diag = ccb->cs_diag;
    cssb->cs_facility = ccb->cs_facility;
#ifdef	CS_SYNC
    cssb->cs_max_sessions = 1;	/* ccb->cs_scnt */
#else
    cssb->cs_max_sessions = ccb->cs_scnt;
#endif
    cssb->cs_max_active = ccb->cs_ascnt;
    cssb->cs_stksize = 65536; /* ignoring ccb->cs_stksize */
    cssb->cs_max_active = 10;
    cssb->cs_max_sessions = cssb->cs_max_active;
    cssb->cs_max_ldb_connections = 0;
    cssb->cs_quantum = STATSWITCH;
    cssb->cs_isstar = 0;
    cssb->cs_scbattach = ccb->cs_scbattach;
    cssb->cs_scbdetach = ccb->cs_scbdetach;
    cssb->cs_format_lkkey = ccb->cs_format_lkkey;

    /* 
    ** Get CS options 
    */

    if (status = CSoptions( cssb ))
	return (status);

    /*
    ** Now do the post option massaging of the option values.
    ** Round stack size to page size. 
    */

    if(cssb->cs_quantum < MINSWITCH )
	cssb->cs_quantum = MINSWITCH;

    if( cssb->cs_max_active > cssb->cs_max_sessions )
	cssb->cs_max_active = cssb->cs_max_sessions;

    cssb->cs_stksize = (cssb->cs_stksize + ME_MPAGESIZE-1) & ~(ME_MPAGESIZE-1);

    /* be sure fds 0, 1, and 2 are not corruptable */

    i = open("/dev/null", 2);

    while (i < 2 && i >= 0)
	i = dup(i);

    if (i > 2)
	close(i);

    /*
    ** Figure out how big to increase the file descriptor table.
    ** If local server, fd_wanted = connected_sessions + CS_RESERVED_FDS
    ** If Star server, fd_wanted = (connected _sessions * 9) + CS_RESERVED_FDS
    **			or
    **		       fd_wanted = max_ldb_connections
    **
    ** Star will use at least 4 fds per session.
    */

    if( !cssb->cs_isstar )
    {
	/* Not a star server */

	fd_wanted = cssb->cs_max_sessions + CS_RESERVED_FDS;

	if( fd_wanted > iiCL_get_fd_table_size() )
	    fd_wanted = iiCL_increase_fd_table_size( FALSE, fd_wanted );

	i = fd_wanted - CS_RESERVED_FDS;
    }
    else
    {
	/* Star server.  */

	fd_wanted = cssb->cs_max_sessions * 9 + CS_RESERVED_FDS;

	if( cssb->cs_max_ldb_connections > fd_wanted )
	    fd_wanted = cssb->cs_max_ldb_connections;

	if( fd_wanted > iiCL_get_fd_table_size() )
	    fd_wanted = iiCL_increase_fd_table_size( FALSE, fd_wanted );

	i = (fd_wanted - CS_RESERVED_FDS) / 9;
    }

    if( i < cssb->cs_max_sessions )
    {
	(*Cs_srv_block.cs_elog)( E_CS002E_CS_SHORT_FDS, (CL_ERR_DESC *)0, 1,
			sizeof( i ), (PTR)&i );
	cssb->cs_max_sessions = i;
    }

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
    idle_scb->cs_priority = CS_PIDLE;	 /* idle = lowest priority */
    idle_scb->cs_thread_type = CS_INTRNL_THREAD;
    idle_scb->cs_self = (CS_SID)idle_scb;
    idle_scb->cs_pid = Cs_srv_block.cs_pid;
    MEmove( 12,
	    " <idle job> ",
	    ' ',
	    sizeof(idle_scb->cs_username),
	    idle_scb->cs_username);

    idle_scb->cs_rw_q.cs_q_next =
	cssb->cs_rdy_que[idle_scb->cs_priority]->cs_rw_q.cs_q_next;
    idle_scb->cs_rw_q.cs_q_prev =
	cssb->cs_rdy_que[idle_scb->cs_priority]->cs_rw_q.cs_q_prev;
    cssb->cs_rdy_que[idle_scb->cs_priority]->cs_rw_q.cs_q_next = idle_scb;
    cssb->cs_rdy_que[idle_scb->cs_priority]->cs_rw_q.cs_q_prev = idle_scb;

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
    return(OK);
}

/*{
** Name: CSterminate()	- Prepare to and terminate the server
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
**      02-may-1997 (canor01)
**          If there are any threads with the CS_CLEANUP_MASK set, they
**          should be allowed to die before signaling any of the other
**          system threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSterminate, removed cs_mt test.
[@history_template@]...
*/
STATUS
IICSterminate(i4 mode, i4  *active_count)
{
    i4			status = 0;
    CS_SCB		*scb;

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
	    if ( active_count )
	        *active_count = Cs_srv_block.cs_user_sessions;

	    if ( Cs_srv_block.cs_user_sessions )
		return(E_CS0003_NOT_QUIESCENT);

	case CS_START_CLOSE:
            /*
            ** If there are any sessions with the CS_CLEANUP_MASK
            ** set, it means they need to finish their work
            ** before any of the other system sessions are told
            ** to shutdown.  When they exit, they will make
            ** another call to CSterminate() to signal the other
            ** system threads.
            */
            for (scb = Cs_srv_block.cs_known_list->cs_prev;
                scb != Cs_srv_block.cs_known_list;
                scb = scb->cs_prev)
            {
                if (scb->cs_cs_mask & CS_CLEANUP_MASK)
                {
                    CSattn(CS_SHUTDOWN_EVENT, scb->cs_self);
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
	    Cs_srv_block.cs_inkernel = 1;
	    for (scb = Cs_srv_block.cs_known_list->cs_next;
		scb != Cs_srv_block.cs_known_list;
		scb = scb->cs_next)
	    {
		if (scb != &Cs_idle_scb)
		    CSattn(CS_SHUTDOWN_EVENT, scb->cs_self);
	    }

	    Cs_srv_block.cs_inkernel = 0;

	    if (Cs_srv_block.cs_async)
		CS_move_async(scb);

	    /*
	    ** Return and wait for server tasks to terminate before shutting
	    ** down server.
	    */
	    Cs_srv_block.cs_mask |= CS_FINALSHUT_MASK;
	    if (Cs_srv_block.cs_num_sessions > 0)
	    {
		return(E_CS0003_NOT_QUIESCENT);
	    }

	case CS_KILL:
	case CS_CRASH:
	    Cs_srv_block.cs_state = CS_TERMINATING;
	    break;

	default:
	    return(E_CS0004_BAD_PARAMETER);
    }

    Cs_srv_block.cs_inkernel = 1;
    Cs_srv_block.cs_mask |= CS_NOSLICE_MASK;
    Cs_srv_block.cs_state = CS_INITIALIZING;	/* to force no sleeping */

    /* Dump server statistics */
    CSdump_statistics(TRdisplay);

    /*
    ** At this point, it is known that the request is to shutdown the server,
    ** and that that request should be honored.	 Thus, proceed through the known
    ** thread list, terminating each thread as it is encountered.  If there
    ** are no known threads, as should normally be the case, this will simply
    ** be a very fast process.
    */

    status = 0;

    for (scb = Cs_srv_block.cs_known_list->cs_next;
	scb != Cs_srv_block.cs_known_list;
	scb = scb->cs_next)
    {
	/* for each known thread, if it is not recognized, ask it to die */
	if ((scb->cs_self > 0) && (scb->cs_self < CS_FIRST_ID))
	    continue;
	if ((scb == (CS_SCB *)&Cs_idle_scb) || (scb == (CS_SCB *)&Cs_admin_scb))
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

    PCexit(0);
    _exit(0);

    /* NOTREACHED */
}

/*{
** Name: CSalter()	- Alter server operational characteristics
**
** Description:
**	This routine accepts basically the same set of operands as the CSinitiate()
**	routine;  the arguments passed in here replace those specified to
**	CSinitiate.  Only non-procedure arguments can be specified;  the processing
**	routine, scb allocation and deallocation, and error logging routines
**	cannot be altered.
**
**	Arguments are left unchanged by specifying the value CS_NOCHANGE.
**	Values other than CS_NOCHANGE will be assumed as intending to replace
**	the value currently in force.
**
** Inputs:
**	ccb				the call control block, with
**	.cs_scnt			Number of threads allowed
**	.cs_ascnt			Nbr of active threads allowed
**	.cs_stksize			Size of stacks in bytes
**				All other fields are ignored.
**
** Outputs:
**	none
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
**	28-Oct-1986 (fred)
**	    Initial Definition.
**	23-Oct-1992 (daveb)
**	    prototyped
[@history_template@]...
*/
STATUS
CSalter( CS_CB *ccb )
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
**	12-nov-96 (kinpa04)
**		On axp_osf, 10 pages are needed on the altinate stack to handle
**		a SEGV produced by an overflow on the regular stack.
**      06-mar-1997 (reijo01)
**          Changed II_DBMS_SERVER message to use generic system name.
**      23-may-1997 (reijo01)
**          Print out II_DBMS_SERVER message only if there is a listen address.
**	03-Jul-97 (radve01)
**	    Basic, debugging related info, printed to dbms log at system
**	    initialization.
**	23-sep-1997 (canor01)
**	    Print Jasmine startup message to stderr.
**	12-Mar-1998 (shero03)
**	    Initialize the FP localcallback routine.
[@history_template@]...
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSdispatch, removed cs_mt test.
**	03-Oct-2002 (somsa01)
**	    If cs_startup fails, make sure we print out "FAIL".
**	07-July-2003 (hanje04)
**	    BUG 110542 - Linux Only
**	    Enable use of alternate stack for SIGNAL handler in the event
** 	    of SIGSEGV SIGILL and SIGBUS. pthread libraries do not like
**	    the alternate stack to be malloc'd so declare it on IICSdispatch
**	    stack for INTERNAL THREADS and localy for each thread in CSMT_setup
**	    for OS threads.
**	07-oct-2004 (thaju02)
**	    Change npages to SIZE_TYPE.
*/
GLOBALREF        char    *MEalloctab;
GLOBALREF        char    *MEbase;
GLOBALREF        char    *MElimit;

STATUS
IICSdispatch(void)
{
    STATUS		status = 0;
    STATUS		ret_val;
    i4			i;
    EX_CONTEXT		excontext;
    CS_INFO_CB		csib;
    FUNC_EXTERN STATUS	cs_handler();
    CSSL_CB		*slcb;
#ifdef LNX
    char                ex_altstack[CS_ALTSTK_SZ * 4] __attribute__  \
						(( aligned (sizeof(PTR) * 8) ));
#endif


    if (Cs_srv_block.cs_process == 0)
    {
	return(FAIL);
    }

# ifdef xCL_077_BSD_MMAP
    {
	PTR		    ex_stack;
	SIZE_TYPE	    npages;
	CL_ERR_DESC	    err_code;

#ifdef LNX
        ex_stack = ex_altstack;
        if (EXaltstack(ex_stack, CS_ALTSTK_SZ) != OK)
            return(FAIL);
#else
	if (MEget_pages(ME_MZERO_MASK, 10, 0, &ex_stack, &npages, 
		&err_code) != OK)
	    return(FAIL);

    	if (EXaltstack(ex_stack, (npages * ME_MPAGESIZE)) != OK)
	{
	    (void)MEfree_pages(ex_stack, npages, &err_code);
	    return(FAIL);
	}
#endif
    }
# endif /* ifdef xCL_077_BSD_MMAP */

    i_EX_initfunc( NULL, NULL ); /* default handling of EX stack */

    if (EXdeclare(cs_handler, &excontext) != OK)
    {
	if (!got_ex++)
	{
	    (*Cs_srv_block.cs_elog)(E_CS0014_ESCAPED_EXCPTN, 0, 0);
	}
	EXdelete();
	return(E_CS0014_ESCAPED_EXCPTN);
    }

    /* declare shutdown signals here */
# if !defined(ibm_lnx)
# ifdef SIGEMT
    /* doesn't exist on AIX/RT */
    i_EXsetothersig(SIGEMT, CSsigterm);
# endif
# endif /* ibm_lnx */
# ifdef SIGDANGER
    /* But this does... */
    i_EXsetothersig(SIGDANGER, CSsigdanger);
# endif
    i_EXsetothersig(SIGQUIT, CSsigterm);
    i_EXsetothersig(SIGTERM, CSsigterm);
    i_EXsetothersig(SIGHUP, CSsigterm);
    i_EXsetothersig(SIGINT, SIG_IGN);
    i_EXsetothersig(SIGPIPE, SIG_IGN);
#if defined(rux_us5)
    i_EXsetothersig(SIGVTALRM, SIG_IGN);
#endif

    do
    {
	{
	    CL_ERR_DESC	    error;

	    Cs_admin_scb.csa_scb.cs_type = CS_SCB_TYPE;
	    Cs_admin_scb.csa_scb.cs_client_type = 0;
	    Cs_admin_scb.csa_scb.cs_length = sizeof(CS_ADMIN_SCB);
	    Cs_admin_scb.csa_scb.cs_owner = (PTR)CS_IDENT;
	    Cs_admin_scb.csa_scb.cs_tag = CS_TAG;
	    Cs_admin_scb.csa_scb.cs_state = CS_COMPUTABLE;
	    Cs_admin_scb.csa_scb.cs_priority = CS_PADMIN;
	    Cs_admin_scb.csa_scb.cs_thread_type = CS_INTRNL_THREAD;
	    Cs_admin_scb.csa_scb.cs_self = (CS_SID)&Cs_admin_scb;
	    Cs_admin_scb.csa_scb.cs_pid = Cs_srv_block.cs_pid;

	    MEmove( 14, " <admin job> ", ' ',
		   sizeof(Cs_admin_scb.csa_scb.cs_username),
		   Cs_admin_scb.csa_scb.cs_username);

	    ret_val = CS_alloc_stack(&Cs_admin_scb, &error);

	    if (ret_val != OK)
	    {
		EXdelete();
		return(ret_val);
	    }

	    CSw_semaphore( &Cs_admin_scb.csa_sem, CS_SEM_SINGLE, "csa_sem");
	    CS_scb_attach( &Cs_admin_scb.csa_scb );

# if defined(sun_u42)
# define got1
	    *(--(long *)Cs_admin_scb.csa_scb.cs_registers[CS_SP])
		= (long)CS_setup;
# endif

# ifdef hp3_us5
# define got1
            /*
            ** Essentially, this is the same approach as the sun_us42 above.
            ** The hp compiler did not like the above statement.
            */
            {
            long *fake;

            fake = (long *)Cs_admin_scb.csa_scb.cs_registers[CS_SP];
            --fake;
            Cs_admin_scb.csa_scb.cs_registers[CS_SP] = (long)fake;
            *fake = (long)CS_setup;
            }
# endif /* hp3_us5 */

# if defined(ds3_ulx) || defined(rmx_us5) || defined(pym_us5) || \
     defined(ts2_us5) || defined(sgi_us5) || defined(rux_us5)
# define got1
            Cs_admin_scb.csa_scb.cs_pc = (long) CS_ds3_setup;
# endif
# if defined(su4_u42) || defined(dr6_us5) || defined(su4_cmw)
# define got1
	    Cs_admin_scb.csa_scb.cs_pc = (long)CS_su4_setup + 4;
# endif
# if defined(sparc_sol)
# define got1
	    Cs_admin_scb.csa_scb.cs_pc = (long)CS_su4_setup - 4;
#endif
# if defined(sqs_ptx) || \
     defined(usl_us5) || defined(nc4_us5) || defined(dgi_us5) || \
     defined(sos_us5) || defined(sui_us5) || defined(int_lnx) || \
     defined(int_rpl)
# define got1
	    CS_sqs_pushf( &Cs_admin_scb.csa_scb, CS_setup);
# endif

# if defined(ibm_lnx)
# define got1
    Cs_admin_scb.csa_scb.cs_id = 1;
    Cs_admin_scb.csa_scb.cs_exsp = NULL;
    Cs_admin_scb.csa_scb.cs_registers[CS_BP] = (long)CS_ibmsetup;
    Cs_admin_scb.csa_scb.cs_registers[CS_RETADDR] = (long)CS_ibmsetup;
    Cs_admin_scb.csa_scb.cs_ca = (long)CS_ibmsetup;
    {
	long *fake;
	Cs_admin_scb.csa_scb.cs_registers[CS_SP] = (long)&fake;
    }
# endif /* ibm_lnx */

# if defined(any_hpux) && !defined(i64_hpu)
# define got1

    /* Create a thread that starts with CS_setup and ends with CS_rteradicate*/
    {
    void CS_eradicate();

    CS_SCB *scb = &Cs_admin_scb.csa_scb;
    char *stack = scb->cs_stk_area + sizeof(CS_STK_CB);
    int size    = scb->cs_stk_size;

    CS_create_ingres_thread(&scb->cs_mach, CS_setup, CS_eradicate, stack, size);
    }
	
# endif	/* hp8_us5, hp8_bls */

# ifdef dg8_us5
# define got1
   Cs_admin_scb.csa_scb.cs_registers[ CS_PC ] = (long) CS_setup;
# endif /*dg8_us5*/

# if defined(any_aix) || defined(i64_hpu) || \
     defined(i64_lnx) || defined(a64_lnx)
# define got1   /* nothing needed */
# endif

#if defined(axp_osf) || defined(axp_lnx)
# define got1
            Cs_admin_scb.csa_scb.cs_pc = (long)CS_axp_setup;
            Cs_admin_scb.csa_scb.cs_pv = (long)CS_axp_setup;
# endif /* axp_osf */

#if defined(NO_INTERNAL_THREADS)
/* Internal threads not supported on this platform */
# define got1   /* nothing needed */
# endif /* NO_INTERNAL_THREADS */

# ifndef got1
Missing_machine_dependant_code!
# endif

	    /*
	    ** Now the thread is complete, place it on the appropriate queue
	    ** and let it run via the normal course of events.
	    */

	    Cs_admin_scb.csa_scb.cs_mode = CS_OUTPUT;
	    Cs_admin_scb.csa_scb.cs_rw_q.cs_q_next =
		Cs_srv_block.cs_rdy_que[Cs_admin_scb.csa_scb.cs_priority]->cs_rw_q.cs_q_next;
	    Cs_admin_scb.csa_scb.cs_rw_q.cs_q_prev =
		Cs_srv_block.cs_rdy_que[Cs_admin_scb.csa_scb.cs_priority]->cs_rw_q.cs_q_prev;
	    Cs_srv_block.cs_rdy_que[Cs_admin_scb.csa_scb.cs_priority]->cs_rw_q.cs_q_next =
			(CS_SCB *)&Cs_admin_scb;
	    Cs_srv_block.cs_rdy_que[Cs_admin_scb.csa_scb.cs_priority]->cs_rw_q.cs_q_prev =
			(CS_SCB *)&Cs_admin_scb;

	    /*								    */
	    /*	Now we must allocate the same stuff for the idle job.	    */
	    /*	The idle job runs on its own stack (smaller) rather than    */
	    /*	on the VMS default stack.  This allows us to recover from   */
	    /*	the case where a session overflows its stack and VMS	    */
	    /*	(kindly) resets us to our ``normal'' (what do they know in  */
	    /*	Spitbrook) user mode stack.  By running on our own stack,   */
	    /*	when this happens, we recover by trashing the offending	    */
	    /*	thread, and letting nature take its course.		    */

	    ret_val = CS_alloc_stack(&Cs_idle_scb, &error);

	    if (ret_val != OK)
	    {
		EXdelete();
		return(ret_val);
	    }

# if defined(sun_u42)
# define got2
	    *(--(long *)Cs_idle_scb.cs_registers[CS_SP]) = (long)CS_setup;
# endif

# ifdef hp3_us5
# define got2
            /*
            ** Essentially, this is the same approach as the sun_us42 above.
            ** The hp compiler did not like the above statement.
            */
            {
            long *fake;

            fake = (long *)Cs_idle_scb.cs_registers[CS_SP];
            --fake;
            Cs_idle_scb.cs_registers[CS_SP] = (long)fake;
            *fake = (long)CS_setup;
            }
# endif /* hp3_us5 */

# if defined(ds3_ulx) || defined(rmx_us5) || defined(pym_us5) || \
     defined(ts2_us5) || defined(sgi_us5) || defined(rux_us5)
# define got2
            Cs_idle_scb.cs_pc = (long) CS_ds3_setup;
# endif
# if defined(su4_u42) || defined(dr6_us5) || defined(su4_cmw)
# define got2
	    Cs_idle_scb.cs_pc = (long)CS_su4_setup + 4;
# endif
# if defined(sparc_sol)
# define got2
	    Cs_idle_scb.cs_pc = (long)CS_su4_setup - 4;
# endif
# if defined(sqs_ptx) || \
     defined(usl_us5) || defined(nc4_us5) || defined(dgi_us5) || \
     defined(sos_us5) || defined(sui_us5) || defined(int_lnx) || \
     defined(int_rpl)
# define got2
	    CS_sqs_pushf(&Cs_idle_scb, CS_setup);
# endif

# if defined(ibm_lnx)
# define got2
    Cs_idle_scb.cs_id = 2;
    Cs_idle_scb.cs_exsp = NULL;
    Cs_idle_scb.cs_registers[CS_BP] = (long)CS_ibmsetup;
    Cs_idle_scb.cs_registers[CS_RETADDR] = (long)CS_ibmsetup;
    Cs_idle_scb.cs_ca = (long)CS_ibmsetup;
# endif /* ibm_lnx */

# if defined(any_hpux) && !defined(i64_hpu)
# define got2

    /* Create a thread that starts with CS_setup and ends with CS_rteradicate*/
    {
    void CS_eradicate();

    CS_SCB *scb = &Cs_idle_scb;
    char *stack = scb->cs_stk_area + sizeof(CS_STK_CB);
    int size    = scb->cs_stk_size;

    CS_create_ingres_thread(&scb->cs_mach, CS_setup, CS_eradicate, stack, size);
    }
# endif	/* hp8_us5, hp8_bls */

# ifdef dg8_us5
# define got2
   Cs_idle_scb.cs_registers[ CS_PC ] = (long) CS_setup;
# endif /*dg8_us5*/

# if defined(any_aix) || defined(i64_hpu) || \
     defined(i64_lnx) || defined(a64_lnx)
# define got2   /* nothing needed */
# endif

#if defined(axp_osf) || defined(axp_lnx)
# define got2
            Cs_idle_scb.cs_pc = (long)CS_axp_setup;
            Cs_idle_scb.cs_pv = (long)CS_axp_setup;
# endif /* axp_osf */

#if defined(NO_INTERNAL_THREADS)
/* Internal threads not supported on Solaris AMD 64-bit */
# define got2   /* nothing needed */
# endif /* NO_INTERNAL_THREADS */

# ifndef got2
Missing_machine_dependant_code!
# endif

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

	ret_val = CSinstall_server_event(&slcb, 0, 0, CS_cpres_event_handler);
	if (ret_val != OK)
	    break;

	/*
	** Cleanup any wakeup events hanging around from a previous server
	*/
	CS_cleanup_wakeup_events(Cs_srv_block.cs_pid, (i4)0);

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
	    break;

	STncpy( Cs_srv_block.cs_name, csib.cs_name,
		sizeof(Cs_srv_block.cs_name) - 1);
	Cs_srv_block.cs_name[ sizeof(Cs_srv_block.cs_name) - 1 ] = EOS;

	if( Cs_srv_block.cs_define && 
	    NMstIngAt("II_DBMS_SERVER", csib.cs_name) != OK )
	{
	    (*Cs_srv_block.cs_elog)(E_CS002D_CS_DEFINE, (CL_ERR_DESC *)0, 0);
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
		** Save dynamic stuff we've got from sequencer
		*/
		Cs_srv_block.sc_main_sz  = csib.sc_main_sz;
		Cs_srv_block.scd_scb_sz =  csib.scd_scb_sz;
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
TRdisplay("The server startup: pid= ^^^ %d\n", Cs_srv_block.cs_pid);
TRdisplay("             connect_id= ^^^ %s\n", Cs_srv_block.cs_name);
TRdisplay("The Server Block         ^^^ @%0p, size is ^^^ %d(%x hex)\n",
		 &Cs_srv_block, (long)&Cs_srv_block.srv_blk_end - (long)&Cs_srv_block,
		      		   (long)&Cs_srv_block.srv_blk_end - (long)&Cs_srv_block);
TRdisplay("Event mask               ^^^ @%0p\n", &Cs_srv_block.cs_event_mask);
TRdisplay("Current thread pointer   ^^^ @%0p\n", &Cs_srv_block.cs_current);
TRdisplay("Max # of active sessions ^^^ %d\n", Cs_srv_block.cs_max_active);
TRdisplay("The thread stack size is ^^^ %d\n", Cs_srv_block.cs_stksize);
TRdisplay("Total # of sessions      ^^^ @%0p\n",&Cs_srv_block.cs_num_sessions);
TRdisplay("# of user sessions       ^^^ @%0p\n",&Cs_srv_block.cs_user_sessions);
TRdisplay("HWM of active threads    ^^^ @%0p\n", &Cs_srv_block.cs_hwm_active);
TRdisplay("Current # of act.threads ^^^ @%0p\n", &Cs_srv_block.cs_num_active);
TRdisplay("Curr. # of stacks alloc. ^^^ @%0p\n", &Cs_srv_block.cs_stk_count);
TRdisplay("Time-out event wait list ^^^ @%0p\n", &Cs_srv_block.cs_to_list);
TRdisplay("Non time-out event wait  ^^^ @%0p\n", &Cs_srv_block.cs_wt_list);
TRdisplay("Global state of the sys. ^^^ @%0p\n", &Cs_srv_block.cs_state);
TRdisplay(" 00-uninitialized, 10-initializing, 20-processing, 25-switching,\n");
TRdisplay(" 30-idling, 40-closing, 50-terminating, 60-in error\n");
TRdisplay("In kernel indicator      ^^^ @%0p\n", &Cs_srv_block.cs_inkernel);
TRdisplay("Async indicator          ^^^ @%0p\n", &Cs_srv_block.cs_async);
TRdisplay("->->\n");
TRdisplay("The Sequencer Block      ^^^ @%0p,  size is ^^^ %d(%x hex)\n",
        Cs_srv_block.cs_svcb, Cs_srv_block.sc_main_sz, Cs_srv_block.sc_main_sz);
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
if(Cs_sm_cb)
{
 TRdisplay("Shared memory(main segm) ^^^ @%0p\n", Cs_sm_cb);
 TRdisplay("          id=            ^^^ %d\n",
    						   Cs_sm_cb->css_css_desc.cssm_id);
 TRdisplay("     attached            ^^^ @%0p\n",
  							 Cs_sm_cb->css_css_desc.cssm_addr);
 TRdisplay("        size=            ^^^ %0x\n",
                			   Cs_sm_cb->css_css_desc.cssm_size);
 TRdisplay("System setup: ^^^ %d servers with  ^^^ %d wakeup blocks\n",
				 Cs_sm_cb->css_numservers, Cs_sm_cb->css_wakeup.css_numwakeups);
 TRdisplay("Min free wakeups ^^^ @%0p, Max used wakeups ^^^ @%0p\n",
          &Cs_sm_cb->css_wakeup.css_minfree, &Cs_sm_cb->css_wakeup.css_maxused);
 TRdisplay("->->->->->\n");
}
TRdisplay("Page allocation bitmap   ^^^ @%0p\n", &MEalloctab);
TRdisplay("Private memory low value ^^^ @%0p, high ^^^ @%0p\n",
										&MEbase, &MElimit);
TRdisplay("Current memory low value= ^^^ %0p, high= ^^^ %0p\n",
        									  MEbase, MElimit);
TRdisplay("->->->->->\n");

	/*
	** Initialize the CL semaphore system.
	** Save the address of the mainline ME page allocator semaphore and
	** setup the semaphore system to use CSsemaphore routines instead of
	** dummy routines.
	*/
	ME_page_sem = csib.cs_mem_sem;
	CS_rcv_request();    /* set up to be adding thread's */

	/*
	** Set server to be ready to run multi-user and call swuser to start
	** scheduling runable threads.
	*/
	status = 1;
	ret_val = 0;
	Cs_srv_block.cs_current = 0;
	Cs_srv_block.cs_state = CS_IDLING;

	/* Committed; set up right semaphores for MU */
	MUset_funcs( MUcs_sems() );

	CS_swuser();

	/* should never get here */
	(*Cs_srv_block.cs_elog)(E_CS00FE_RET_FROM_IDLE, 0, 0);
	ret_val = E_CS00FF_FATAL_ERROR;

    } while( FALSE );

    if ( ((status & 1) == 0) || (ret_val) )
    {
	CL_ERR_DESC *err_code;
	if ((status & 1) != 0)
	{
	    status = 0;
	}
	SETCLERR (err_code, status, 0);
	(*Cs_srv_block.cs_elog)(ret_val, status, 0);
    }

    Cs_srv_block.cs_state = CS_CLOSING;

    CSterminate(CS_KILL, &i);

    PCexit(((status & 1) || (ret_val == 0)) ? 0 : status);
}

/*{
** Name: CSsuspend	- Suspend current thread pending some event
**
** Description:
**	This procedure is used to suspend activity for a particular
**	thread pending some event which will be delivered via a
**	CSresume() call.  Optionally, this call will also schedule
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
**	20-mar-1997 (canor01)
**	    Add support for timeouts in nanoseconds.
**	25-Mar-1997 (jenjo02)
**	    Third parameter may be a ptr to a LK_LOCK_KEY when sleep
**	    mask is CS_LOCK_MASK (for iimonitor) instead of an ecb.
**	05-Oct-1998 (jenjo02)
**	    Added cs_lkevent, cs_logs, cs_lgevent stats to CS_SCB to
**	    correctly account for those types of suspends.
**	16-Nov-1998 (jenjo02)
**	    Defined new suspend masks, Log I/O (CS_LIO_MASK) and flags
**	    to discriminate I/O reads from writes (CS_IOR_MASK,
**	    CS_IOW_MASK) for BIO, DIO, and LIO.
**	    Added new stats to the SCB to count these new states, and
**	    their wait times.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSsuspend, removed cs_mt test.
**      19-aug-2002 (devjo01)
**          Don't set sid in ecb if CS_LKEVENT_MASK.
*/
STATUS
IICSsuspend(i4 mask, i4  to_cnt, PTR ecb)
{
    CS_SCB		*scb;
    STATUS		rv = OK;

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
	((CSEV_CB *)ecb)->sid = (CS_SID)Cs_srv_block.cs_current;
    }
    if (Cs_srv_block.cs_state != CS_INITIALIZING)
    {
	scb = Cs_srv_block.cs_current;

	if (ecb)
	    scb->cs_sync_obj = ecb;

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
	else if (mask & CS_LOG_MASK)
	{
	    scb->cs_logs++;
	    Cs_srv_block.cs_wtstatistics.cs_lg_done++;
	}
	else if (mask & CS_TIMEOUT_MASK)
	{
	    Cs_srv_block.cs_wtstatistics.cs_tm_done++;
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
	    scb->cs_inkernel = 0;
	    Cs_srv_block.cs_inkernel = 0;

	    if (Cs_srv_block.cs_async == 0)
	    {
		return(OK);
	    }

	    CS_move_async(scb);
	    return(OK);
	}
	else if ((scb->cs_mask & CS_IRPENDING_MASK) &&
		    (mask & CS_INTERRUPT_MASK))
	{
	    /*
	    **	Then an interrupt is pending.  Pretend we just got it
	    */
	    scb->cs_mask &= ~(CS_EDONE_MASK | CS_IRCV_MASK | CS_IRPENDING_MASK
				    | CS_TO_MASK | CS_ABORTED_MASK
				    | CS_INTERRUPT_MASK | CS_TIMEOUT_MASK);
	    scb->cs_state = CS_COMPUTABLE;
	    scb->cs_inkernel = 0;
	    Cs_srv_block.cs_inkernel = 0;

	    if (Cs_srv_block.cs_async)
		CS_move_async(scb);

	    return(E_CS0008_INTERRUPTED);
	}

	scb->cs_state = CS_EVENT_WAIT;
	scb->cs_mask |= (mask & ~(CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
				  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
				  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
				  CS_LOG_MASK ));
	scb->cs_memory = (mask & (CS_DIO_MASK | CS_BIO_MASK | CS_LOCK_MASK |
				  CS_LKEVENT_MASK | CS_LGEVENT_MASK |
				  CS_LIO_MASK | CS_IOR_MASK | CS_IOW_MASK |
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

	CS_checktime();
	scb->cs_ppid = Cs_sm_cb->css_qcount;

	scb->cs_timeout = 0;
	if (mask & CS_TIMEOUT_MASK)
	{
	    if ( mask & CS_NANO_TO_MASK )
	    {
		scb->cs_timeout = to_cnt / 1000000;
		if ( to_cnt % 1000000 )
		    scb->cs_timeout++;
            }
	    else
	    {
#ifndef	CS_STRICT_TIMEOUT
	        if (to_cnt < 0)
		    scb->cs_timeout = - to_cnt;
	        else
#endif
	            scb->cs_timeout = to_cnt * 1000;
	    }
	}

	/* Now take this scb out of the ready queue */
	scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
	    scb->cs_rw_q.cs_q_prev;
	scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
	    scb->cs_rw_q.cs_q_next;

	/* if this was the last block, then turn off the ready bit */

	if ((scb->cs_rw_q.cs_q_next ==
			Cs_srv_block.cs_rdy_que[scb->cs_priority])
		&& (scb->cs_rw_q.cs_q_prev ==
			Cs_srv_block.cs_rdy_que[scb->cs_priority]))
	    Cs_srv_block.cs_ready_mask &=
			~(CS_PRIORITY_BIT >> scb->cs_priority);

	/*
	** Now install the scb on the correct wait que.	 If there is no timeout,
	** then the scb goes on the simple wait list.  However, if there is a
	** timeout, then the scb goes on the timeout list, which is scanned and
	** updated once per second (or once per quantum, whichever is longer).
	*/

	if ((scb->cs_timeout > 0) || (mask & CS_NANO_TO_MASK))
	{
	    scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_to_list->cs_rw_q.cs_q_next;
	    scb->cs_rw_q.cs_q_prev = Cs_srv_block.cs_to_list;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	    Cs_srv_block.cs_to_list->cs_rw_q.cs_q_next = scb;
	}
	else
	{
	    scb->cs_rw_q.cs_q_next = Cs_srv_block.cs_wt_list->cs_rw_q.cs_q_next;
	    scb->cs_rw_q.cs_q_prev = Cs_srv_block.cs_wt_list;
	    scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev = scb;
	    Cs_srv_block.cs_wt_list->cs_rw_q.cs_q_next = scb;
	}
	/* now done, so go install the next thread */

	scb->cs_inkernel = 0;
	Cs_srv_block.cs_inkernel = 0;

	if (Cs_srv_block.cs_async)
	{
	    CS_move_async(scb);
	}

	CS_swuser();

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

	/* Turn inkernel on while changing cs_mask */

	Cs_srv_block.cs_inkernel = 1;
	scb->cs_inkernel = 1;

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
	i4	tim, nevents;
	i4	start_time;

	start_time = TMsecs();

	if ( mask & CS_NANO_TO_MASK )
	    to_cnt /= 1000000000;  /* convert to seconds */

	while (!Cs_resumed)
	{
	    tim = 0;
	    CS_find_events(&tim, &nevents);
	    if (nevents == 0)
	    {
		CS_ACLR(&Cs_svinfo->csi_wakeme);
		tim = 200;
		if(iiCLpoll(&tim) == E_FAILED)
                        TRdisplay("CSsuspend: iiCLpoll failed...serious\n");
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

/*{
** Name: CSresume()	- Mark thread as done with event
**
** Description:
**	This routine is called to note that the session identified
**	by the parameter has completed some event.  It expects to run
**	so that it cannot be interrupted, as it may have the world in
**	an inconsistent state, but cannot use semaphores, since it may
**	be being called by the o/s.   In particular, on VMS it is a delivered
**	AST.
**
** Inputs:
**	sid				thread id for which event has
**					completed.
**					Certain sid's with special meaning
**					may cause other operations to happen.
**					For instance, an sid of CS_ADDR_ID
**					will cause a new thread to be added.
**					This may be an ecb passed to CSsuspend.
**
** Outputs:
**	none
**	Returns:
**	    none -- actually system specific.  On VMS, it is VOID.
**	Exceptions:
**	    none
**
** Side Effects:
**	    The SCB for this thread is moved to the ready que, if it is not
**	    already there (via an interrupt or timeout).
**	--> Because this routine expects, in some environments, to operate
**	    in a protected mode, mainline code should not call this routine
**	    directly.
** History:
**	30-Oct-1986 (fred)
**	    Created.
**	 7-nov-1988 (rogerk)
**	    Before storing information in the scb or in the server control
**	    block, check the cs_inkernel flag to see if that control block
**	    is currently busy.	If so, write into the async portion of the
**	    control block.
**	16-dec-1988 (fred)
**	    set cs_async field when listen completes while in kernel state.
**	    BUG 4255.
**	23-Oct-1992 (daveb)
**	    prototyped
**	26-apr-1993 (bryanp)
**	    Removed setting of Cs_resumed = 1 in the non-initializing case. See
**	    large comment at head of this file.
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	01-Dec-1995 (jenjo02)
**	    Defined 2 new wait reasons, CS_LGEVENT_MASK and CS_LKEVENT_MASK
**	    to statistically distinguish these kinds of waits from other
**	    LG/LK waits. Also fixed wait times to use Cs_sm_cb->cs_qcount
**	    instead of Cs_srv_block.cs_quantums which isn't maintained
**	    by any code!
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSresume, removed cs_mt test.
*/
VOID
IICSresume(CS_SID sid)
{
    CS_SCB		*scb;

    if (Cs_srv_block.cs_state == CS_INITIALIZING)
    {
	Cs_resumed = 1;
    }
    else
    {
	scb = CS_find_scb(sid);	    /* locate the scb for this thread */
	if (scb)
	{
	    i4		elapsed_time;
	    
	    /*
	    ** If this session is not listed in event wait state, then the
	    ** event has completed before that session had a chance to
	    ** suspend itself.	Just mark that the event has completed - there
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
	    ** threads.	 They are expected to be waiting and they only
	    ** throw off the statistics.
	    */

	    CS_checktime();
	    elapsed_time = Cs_sm_cb->css_qcount - scb->cs_ppid;

	    if ((scb->cs_memory & CS_DIOR_MASK) == CS_DIOR_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_dior_time += elapsed_time;
		++dio_resumes;
		if (scb == Cs_srv_block.cs_current)
		    ++fast_resumes;
	    }
	    else if ((scb->cs_memory & CS_DIOW_MASK) == CS_DIOW_MASK)
	    {
		Cs_srv_block.cs_wtstatistics.cs_diow_time += elapsed_time;
		++dio_resumes;
		if (scb == Cs_srv_block.cs_current)
		    ++fast_resumes;
	    }
	    else if ((scb->cs_memory & CS_LIOR_MASK) == CS_LIOR_MASK)
		Cs_srv_block.cs_wtstatistics.cs_lior_time += elapsed_time;
	    else if ((scb->cs_memory & CS_LIOW_MASK) == CS_LIOW_MASK)
		Cs_srv_block.cs_wtstatistics.cs_liow_time += elapsed_time;
	    else
	    {
		if (scb->cs_thread_type == CS_USER_THREAD)
		{
		    if ((scb->cs_memory & CS_BIOR_MASK) == CS_BIOR_MASK)
			Cs_srv_block.cs_wtstatistics.cs_bior_time += elapsed_time;
		    else if ((scb->cs_memory & CS_BIOW_MASK) == CS_BIOW_MASK)
			Cs_srv_block.cs_wtstatistics.cs_biow_time += elapsed_time;
		    else if (scb->cs_memory & CS_LOCK_MASK)
			Cs_srv_block.cs_wtstatistics.cs_lk_time += elapsed_time;
		    else if (scb->cs_memory & CS_LKEVENT_MASK)
			Cs_srv_block.cs_wtstatistics.cs_lke_time += elapsed_time;
		    else if (scb->cs_memory & CS_LGEVENT_MASK)
			Cs_srv_block.cs_wtstatistics.cs_lge_time += elapsed_time;
		    else if (scb->cs_memory & CS_LOG_MASK)
			Cs_srv_block.cs_wtstatistics.cs_lg_time += elapsed_time;
		    else if (scb->cs_memory & CS_TIMEOUT_MASK)
			Cs_srv_block.cs_wtstatistics.cs_tm_time += elapsed_time;
		}
	    }

	    /*
	    ** If the Server is not running inkernel, then we can move
	    ** the scb off the wait list and put it into the ready queue.
	    ** If the server is inkernel, then we can't muck with any of
	    ** the control block lists, so we put the scb on the async
	    ** ready list.  On VMS this list is touched only by atomic routines
	    ** which are protected against thread switching and AST driven
	    ** routines.  On UNIX, this list is only used when 'inkernel'
	    ** is asserted, and is moved by non-AST (non-signal handler)
	    ** code to a private list when 'inkernel' is turned off.
	    **
	    ** When the session which is running inkernel returns from
	    ** inkernel mode, it will move the scb onto the ready queue.
	    */
	    if (Cs_srv_block.cs_inkernel == 0)
	    {
		/*
		** Since CSresume is called by both AST, and non-AST
		** routines, assume we are running non-AST, and assert
		** 'inkernel' here.  If we are in an asynchronous
		** completion routine, this is harmless, since flag
		** will be reset before control is returned to non-AST
		** code.
		*/
		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;
		scb->cs_mask |= CS_EDONE_MASK;
		scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		    scb->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next =
		    scb->cs_rw_q.cs_q_next;

		/* now link last into the ready queue */

		scb->cs_rw_q.cs_q_next =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority];
		scb->cs_rw_q.cs_q_prev =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
		Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev =
			scb;

		Cs_srv_block.cs_ready_mask |=
			(CS_PRIORITY_BIT >> scb->cs_priority);

#ifdef	CS_STACK_SHARING
		if (scb->cs_stk_area)
		{
#endif
		    scb->cs_state = CS_COMPUTABLE;
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
		    scb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev = scb;
		    Cs_srv_block.cs_as_list->cs_as_q.cs_q_next = scb;
		}
		Cs_srv_block.cs_async = 1;

		/*
		** Mark the event as complete in the SCB.  If the scb is busy
		** then store the completion information in the async portion
		** of the SCB.	The event completion will be marked in the
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
	    }
	    if (Cs_srv_block.cs_state == CS_IDLING)
	    {
		CS_wake( 0 );
	    }
	    return;
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
		Cs_admin_scb.csa_mask |= CSA_ADD_THREAD;
		Cs_admin_scb.csa_scb.cs_state = CS_COMPUTABLE;
		Cs_srv_block.cs_ready_mask |=
				(CS_PRIORITY_BIT >> CS_PADMIN);
	    }
	    else
	    {
		Cs_srv_block.cs_asadd_thread = 1;
		Cs_srv_block.cs_async = 1;
	    }
	}
    }
}

/*{
** Name: IICSsuspend_for_AST()	- wait for AST
**
** Description:
**	This routine is called to suspend for AST completion from DLM 
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
**	2-Sep-2004 (mutma03)
**	    Created.
*/
STATUS
IICSsuspend_for_AST(i4 mask, i4  to_cnt, PTR ecb)
{
}

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
**	than a CScancelled() call must occur before the next CSsuspend()
**	call is made.  And furthermore, the CScancelled() call indicates
**	that the CSresume() call which was queued previously will not
**	occur (it may have already, or it may never occur, but it may not
**	occur after the CSresume() call is made).
**
** Inputs:
**	ecb				Pointer to a system specific event
**					control block.	Can be zero if not used.
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
**	    Set inkernel while altering cs_mask.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICScancelled, removed cs_mt test.
**	12-nov-2002 (horda03) BUg 105321
**	    Ensure there are no CP resumes pending for the session.
[@history_template@]...
*/
/* ARGSUSED */
VOID
IICScancelled(ecb)
PTR		ecb;
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

        /* Remove any Resumes pending on the CP event queue. 105321 */
        CS_clear_wakeup_block(Cs_srv_block.cs_pid,
                              Cs_srv_block.cs_current->cs_self);

	/* note: don't set Cs_resumed to FALSE in the multi-thread case */
	/* many threads could have been resumed and only some canceled. */
    }
    else
	Cs_resumed = FALSE;
}

/*{
** Name: CSadd_thread - Add a thread to the server space.
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
**	thread_id			optional pointer to CS_SID into
**					which new thread's SID is returned.
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
**      02-may-1997 (canor01)
**          If a thread is created with the CS_CLEANUP_MASK set in
**          its thread_type, transfer the mask to the cs_cs_mask.
**          These threads will be terminated and allowed to finish
**          processing before any other system threads exit.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread().
**	25-Jan-1999 (jenjo02)
**	    Include internal AND external threads in cs_num_sessions count.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSadd_thread, removed cs_mt test.
[@history_template@]...
*/
STATUS
IICSadd_thread(priority, crb, thread_type, thread_id, error)
i4		   priority;
PTR		   crb;
i4		   thread_type;
CS_SID		   *thread_id;
CL_ERR_DESC	   *error;
{
    STATUS		status;
    CS_SCB		*scb;

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
    if ( thread_type & CS_CLEANUP_MASK )
        scb->cs_cs_mask |= CS_CLEANUP_MASK;

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

    status = CS_alloc_stack(scb, error);
    if (status != OK)
    {
#ifdef CS_STACK_SHARING
	if (status == E_CS002F_NO_FREE_STACKS)
	{
	    scb->cs_state = CS_STACK_WAIT;
	}
	else
#endif
	{
	    return(status);
	}
    }

# if defined(sun_u42)
# define got3
    *(--(long *)scb->cs_registers[CS_SP]) = (long)CS_setup;
# endif

# ifdef hp3_us5
# define got3
            /*
            ** Essentially, this is the same approach as the sun_us42 above.
            ** The hp compiler did not like the above statement.
            */
            {
            long *fake;

            fake = (long *)scb->cs_registers[CS_SP];
            --fake;
            scb->cs_registers[CS_SP] = (long)fake;
            *fake = (long)CS_setup;
            }
# endif /* hp3_us5 */

# if defined(ds3_ulx) || defined(rmx_us5) || defined(pym_us5) || \
     defined(ts2_us5) || defined(sgi_us5) || defined(rux_us5)
# define got3
            scb->cs_pc = (long) CS_ds3_setup;
# endif
# if defined(su4_u42) || defined(dr6_us5) || \
     defined(su4_cmw)
# define got3
	    scb->cs_pc = (long)CS_su4_setup + 4;
# endif
# ifdef sparc_sol
# define got3
            scb->cs_pc = (long)CS_su4_setup - 4;
# endif
# if defined(sqs_ptx) || \
     defined(usl_us5) || defined(nc4_us5) || defined(dgi_us5) || \
     defined(sos_us5) || defined(sui_us5) || defined(int_lnx) || \
     defined(int_rpl)
# define got3
	   CS_sqs_pushf(scb, CS_setup);
# endif

# if defined(ibm_lnx)
# define got3
    scb->cs_id = 3;
    scb->cs_exsp = NULL;
    scb->cs_registers[CS_BP] = (long)CS_ibmsetup;
    scb->cs_registers[CS_RETADDR] = (long)CS_ibmsetup;
    scb->cs_ca = (long)CS_ibmsetup;
# endif /* ibm_lnx */

# if defined(any_hpux) && !defined(i64_hpu)
# define got3

    /* Create a thread that starts with CS_setup and ends with CS_rteradicate*/
    {
    void CS_eradicate();

    char *stack = scb->cs_stk_area + sizeof(CS_STK_CB);
    int size    = scb->cs_stk_size;

    CS_create_ingres_thread(&scb->cs_mach, CS_setup, CS_eradicate, stack, size);
    }
# endif	/* hp8_us5, hp8_bls */

# ifdef dg8_us5
# define got3
   scb->cs_registers[ CS_PC ] = (long) CS_setup;
# endif /*dg8_us5*/

# if defined(any_aix) || defined(i64_hpu) || \
     defined(i64_lnx) || defined(a64_lnx)
# define got3   /* nothing needed */
# endif

#if defined(axp_osf) || defined(axp_lnx)
# define got3
            scb->cs_pc = (long)CS_axp_setup;
            scb->cs_pv = (long)CS_axp_setup;
# endif /* axp_osf */

#if defined(NO_INTERNAL_THREADS)
/* Internal threads not supported on Solaris AMD 64-bit */
# define got3   /* nothing needed */
# endif /* NO_INTERNAL_THREADS */

# ifndef got3
Missing_machine_dependant_code!
# endif

    /*
    ** Link new scb last in the ready queue:
    */
    Cs_srv_block.cs_inkernel = 1;
    scb->cs_inkernel = 1;
    scb->cs_rw_q.cs_q_next =
	Cs_srv_block.cs_rdy_que[scb->cs_priority];
    scb->cs_rw_q.cs_q_prev =
	Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next
	    = scb;
    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev = scb;

    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> priority);

    scb->cs_next =
	Cs_srv_block.cs_known_list;
    scb->cs_prev =
	Cs_srv_block.cs_known_list->cs_prev;
    Cs_srv_block.cs_known_list->cs_prev->cs_next = scb;
    Cs_srv_block.cs_known_list->cs_prev = scb;

    /* Increment session count.	 If user thread increment user session count. */
    /* cs_num_sessions tallies internal and external threads */
    Cs_srv_block.cs_num_sessions++;
    if (scb->cs_thread_type == CS_USER_THREAD ||
	scb->cs_thread_type == CS_MONITOR)
    {
	Cs_srv_block.cs_user_sessions++;
    }
    scb->cs_inkernel = 0;
    Cs_srv_block.cs_inkernel = 0;
    if (Cs_srv_block.cs_async)
	CS_move_async(scb);

    CS_scb_attach( scb );

    return(OK);
}

/*{
** Name: CSremove	- Cause a thread to be terminated
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
**	16-Mar-2001 (thaju02)
**	    Delete via ipm of a session waiting on a mutex results in
**	    SIGSEGV. Removed reference to scb->cs_sm_root.
**	    (B104254)
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSremove, removed cs_mt test.
[@history_template@]...
*/
STATUS
IICSremove(CS_SID sid)
{
    CS_SCB		*scb;
    CS_SCB		*cscb;

    scb = CS_find_scb(sid);
    if (scb == 0)
    {
	return(E_CS0004_BAD_PARAMETER);
    }

    Cs_srv_block.cs_inkernel = 1;
    scb->cs_inkernel = 1;
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
	    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev = scb;

	    Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);
	}
    }

    scb->cs_inkernel = 0;
    Cs_srv_block.cs_inkernel = 0;
    if (Cs_srv_block.cs_async)
	CS_move_async(scb);
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICScnd_init, removed cs_mt test.
**	17-Dec-2003 (jenjo02)
**	    Removed MO registration.
*/
STATUS
IICScnd_init( CS_CONDITION *cnd )
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICScnd_free, removed cs_mt test.
**	17-Dec-2003 (jenjo02)
**	    Removed MO deregistration.
*/
STATUS
IICScnd_free(CS_CONDITION	*cnd)
{
    if (cnd->cnd_waiter != NULL || cnd != cnd->cnd_next)
    {
	return(FAIL);	/* FIXME either E_CSxxxx_FAIL or E_CSxxxx_INVALID */
    }

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
**	23-Oct-1992 (daveb)
**	    prototyped
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICScnd_wait, removed cs_mt test.
**	17-Dec-2003 (jenjo02)
**	    Reacquire mtx even if "aborted". This is like
**	    real conditions work.
*/
STATUS
IICScnd_wait( CS_CONDITION	*cnd, CS_SEMAPHORE	*mtx )
{
    CS_CONDITION	cndq;
    CS_SCB		*scb;
    STATUS		status, mstatus;

    if ( !cnd || !mtx )
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
    /* CSv_semaphore now guarantees that inkernel will be reset 
       however the macro form won't guarantee it so we'll do
       it ourselves. */
    status = CSv_semaphore(mtx);
    scb->cs_inkernel = 0;
    Cs_srv_block.cs_inkernel = 0;
    if (Cs_srv_block.cs_async)
	CS_move_async(scb);
			    
    if (status)
    {
	/* some thing went wrong - abort */

	Cs_srv_block.cs_inkernel = 1;
	scb->cs_inkernel = 1;

	scb->cs_cnd = NULL;
	scb->cs_state = CS_COMPUTABLE;

	cndq.cnd_next->cnd_prev = cndq.cnd_prev;
	cndq.cnd_prev->cnd_next = cndq.cnd_next;

	/* make runable */

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

	Cs_srv_block.cs_ready_mask |=
	    (CS_PRIORITY_BIT >> scb->cs_priority);
	scb->cs_state = CS_COMPUTABLE;

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
	CS_swuser();	/* wait - when we are restarted we are runable */

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
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICScnd_signal, removed cs_mt test.
**	17-Dec-2003 (jenjo02)
**	    Added (optional) SID to prototype so that one
**	    may signal a specific condition waiter rather
**	    the first.
*/
STATUS
IICScnd_signal( CS_CONDITION *cnd, CS_SID sid )
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

	Cs_srv_block.cs_ready_mask |=
	    (CS_PRIORITY_BIT >> scb->cs_priority);
	scb->cs_state = CS_COMPUTABLE;
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
**	23-Oct-1992 (daveb)
**	    prototyped
**	14-Jun-1995 (jenjo02)
**	    Maintain cs_num_active, cs_hwm_active when adding or 
**	    removing non-internal threads on rdy queue.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICScnd_broadcast, removed cs_mt test.
*/
STATUS
IICScnd_broadcast( CS_CONDITION	*cnd )
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

	    Cs_srv_block.cs_ready_mask |=
		(CS_PRIORITY_BIT >> scb->cs_priority);
	    scb->cs_state = CS_COMPUTABLE;
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
**	4-Dec-1992 (daveb)
**	    prototyped CScnd_name.
*/
STATUS
CScnd_name( CS_CONDITION *cond, char *name )
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSstatistics, removed cs_mt test.
*/
STATUS
IICSstatistics(timer_block, server_stats)
TIMERSTAT	*timer_block;
i4		server_stats;
{
    CS_SCB	*scb;

    if (timer_block == 0)
	return(E_CS0004_BAD_PARAMETER);

    if (Cs_incomp)
    {
	/* If we get here, then CSstatistics has been called from completion
	   code.  We must be sure not to call CS_swuser. */
	CS_breakpoint();
    }
    else
    /* This is how we time slice OPF - OPF does CSstatistics while evaluating
       plans to see if it has a plan that will run quicker than it has
       spent finding other plans */
    if ((Cs_sm_cb->css_qcount - Cs_lastquant) > Cs_srv_block.cs_quantum)
    {
	CS_swuser();
    }

    scb = Cs_srv_block.cs_current;

    if (scb->cs_mask & CS_CPU_MASK)
    {
	CS_update_cpu(&timer_block->stat_pgflts, &scb->cs_cputime);
    }
    else
    {
	CS_update_cpu(&timer_block->stat_pgflts, (i4 *)0);
    }

    if (server_stats)
    {
	/* Return statistics for entire server. */

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

	timer_block->stat_dio = scb->cs_dior + scb->cs_diow;
	timer_block->stat_bio = scb->cs_bior + scb->cs_biow;

	/* multiply by 10 since cpu time is returned in 10ms ticks */
	timer_block->stat_cpu = 10 * scb->cs_cputime;

    }

    return(OK);
}

/*{
** Name: CSattn - Indicate async event to CS thread or server
**
** Description:
**	This routine is called to indicate that an unexpected asynchronous
**	event has occurred, and that the CS module should take the appropriate
**	action.	 This is typically used in indicate that a user interrupt has
**	occurred, but may be used for other purposes.
**
**	This routine expects to be called in such a way so that it may not
**	be interrupted.	 This is because it manipulates the server session
**	lists and if switched out may leave them in an inconsistent state.
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
**	13-dec-1996 (canor01)
**	    If a session is waiting on an Ingres condition, wake it up
**	    for shutdown.
**	26-feb-1997 (canor01)
**	    Previous change did not put awakened thread back on ready queue.
**	14-jan-2000 (somsa01)
**	    cs_num_active and cs_user_sessions should also reflect
**	    CS_MONITOR threads as well as CS_USER_THREAD threads.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSattn, removed cs_mt test.
**	11-Sep-2006 (jonj)
**	    Some events may not want the session interrupted,
**	    force-abort with LOGFULL_COMMIT, for instance.
**	    Also let CS_CNDWAIT states be interrupted at any
**	    time (parallel threads use these a lot and expect
**	    to be interrupted).
[@history_template@]...
*/
VOID
IICSattn(i4 eid, CS_SID sid)
{
    CS_SCB		*scb = 0;

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
		/*
		** Assert 'inkernel', since asynchronous resume may
		** occur IF this is being called from non-AST code.
		** If this is being called from AST code, flag
		** will be reset before return to non-AST code, so no
		** harm is done.
		*/
		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;
		scb->cs_mask &= ~CS_INTERRUPT_MASK;
		scb->cs_mask |= CS_IRCV_MASK;

		scb->cs_rw_q.cs_q_next->cs_rw_q.cs_q_prev =
		 scb->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = 
		 scb->cs_rw_q.cs_q_next;

		/* now link into the ready queue */

		/*
		** Link this scb last in the ready queue:
		*/
		scb->cs_rw_q.cs_q_next =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority];
		scb->cs_rw_q.cs_q_prev =
		    Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev;
		scb->cs_rw_q.cs_q_prev->cs_rw_q.cs_q_next = scb;
		Cs_srv_block.cs_rdy_que[scb->cs_priority]->cs_rw_q.cs_q_prev = scb;

		Cs_srv_block.cs_ready_mask |= (CS_PRIORITY_BIT >> scb->cs_priority);

#ifdef	CS_STACK_SHARING
		if (scb->cs_stk_area)
#endif
		{
		    scb->cs_state = CS_COMPUTABLE;
		}
#ifdef CS_STACK_SHARING
		else
		{
		    scb->cs_state = CS_STACK_WAIT;
		}
#endif
		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
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
		    scb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev = scb;
		    Cs_srv_block.cs_as_list->cs_as_q.cs_q_next = scb;
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
        else if ( scb->cs_state == CS_CNDWAIT )
        {
            CS_CONDITION        *cp;

            /* remove from condition queue */
            cp = scb->cs_cnd;
            cp->cnd_next->cnd_prev = cp->cnd_prev;
            cp->cnd_prev->cnd_next = cp->cnd_next;
	    scb->cs_cnd = NULL;

	    if ( 0 == Cs_srv_block.cs_inkernel )
	    {
		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;

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
		scb->cs_inkernel = 0;
		Cs_srv_block.cs_inkernel = 0;
		if (Cs_srv_block.cs_async)
		    CS_move_async(scb);
	    }
	    else
	    {
		/* Operate on async structures */
		if (scb->cs_as_q.cs_q_next == 0)
		{
		    scb->cs_as_q.cs_q_next =
			Cs_srv_block.cs_as_list->cs_as_q.cs_q_next;
		    scb->cs_as_q.cs_q_prev = Cs_srv_block.cs_as_list;
		    scb->cs_as_q.cs_q_next->cs_as_q.cs_q_prev = scb;
		    Cs_srv_block.cs_as_list->cs_as_q.cs_q_next = scb;
		}
		Cs_srv_block.cs_async = 1;

		if (scb->cs_inkernel == 0)
		{
		    scb->cs_mask |= CS_ABORTED_MASK;
		}
		else
		{
		    if (scb->cs_async == 0)
		    {
			scb->cs_async = 1;
			Cs_srv_block.cs_scb_async_cnt++;
		    }
		    scb->cs_asmask |= CS_ABORTED_MASK;
		}
	    }
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
	/* We may need to do an EXdeclare around this if.
	   But since it is costly and this is run on the idle stack
	   and the idle stack has the same handler, we will not bother.
	   However, not all machines can run xchng_thread on the idle stack
	   so those machines may want to declare that handler.  Note that
	   these exceptions "are not supposed to happen". */
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
    return;
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
**	*sidptr				    thread id.
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSget_sid, removed cs_mt test.
[@history_template@]...
*/
VOID
IICSget_sid(CS_SID *sidptr)
{
    CS_SID	    sid = (CS_SID)0;
    CS_SCB	    *scb;

    if (scb = Cs_srv_block.cs_current)
    {
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSget_cpid, removed cs_mt test.
**	13-Dec-2000 (toumi01)
**	    Restore #ifdef OS_THREADS_USED to allow non-OS-threaded builds.
[@history_template@]...
*/
VOID
IICSget_cpid(CS_CPID *cpid)
{
    CS_SCB	    *scb;

    if (cpid)
    {
# ifdef OS_THREADS_USED
	cpid->wakeup = 0;
# endif /* OS_THREADS_USED */

	if (scb = Cs_srv_block.cs_current)
	{
	    cpid->sid = scb->cs_self;
	    cpid->pid = Cs_srv_block.cs_pid;
	}
	else
	{
	    cpid->sid = CS_NULL_ID;
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSget_scb, removed cs_mt test.
[@history_line@]...
[@history_template@]...
*/
VOID
IICSget_scb(CS_SCB **scbptr)
{
    if (scbptr != (CS_SCB **)0)
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
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-sep-1999 (hanch04)
**	    Created.
[@history_line@]...
[@history_template@]...
*/
VOID
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSfind_scb, removed cs_mt test.
*/

CS_SCB	*
IICSfind_scb(CS_SID sid)
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
**
** History:
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSfind_sid, removed cs_mt test.
**/
CS_SID
IICSfind_sid(CS_SCB *scb)
{
    return (scb->cs_self);
}

/*{
** Name: CSset_sid	- Set the current thread id (DEBUG ONLY)
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSset_sid, removed cs_mt test.
[@history_template@]...
*/
VOID
IICSset_sid(scb)
CS_SCB		   *scb;
{
    Cs_srv_block.cs_current = scb;
}

/*{
** Name: CSintr_ack	- Acknowledge receipt of an interrupt
**
** Description:
**	This routine turns off the CS_IRPENDING_MASK bit in the thread's
**	state mask.  This bit is set if a thread is informed of an interrupt
**	when it is running.  The next interruptable request will terminate
**	with an interrupt after this happens, so this routine allows a thread
**	to explicitly clear the flag when it knows about the interrupt.
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSintr_ack, removed cs_mt test.
[@history_template@]...
*/
VOID
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
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSaltr_session, removed cs_mt test.
*/
STATUS
IICSaltr_session(CS_SID session_id, i4  option, PTR item)
{
    CS_SCB	    *scb = 0;

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
		CS_update_cpu((i4 *)0, (i4 *)0);

		/*
		** Set inkernel while changing the scb's cs_mask as this field
		** is also set by AST driven routines.
		*/

		Cs_srv_block.cs_inkernel = 1;
		scb->cs_inkernel = 1;

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
**	7-jan-97 (stephenb)
**	    Add active user high water mark to dump statistics.
**	05-Oct-1998 (jenjo02)
**	    Show server cpu time in seconds and fractions of seconds.
**	16-Nov-1998 (jenjo02)
**	    Show new stats for Log I/O, DIO/BIO reads and writes.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSdump_statistics, removed cs_mt test.
[@history_template@]...
*/
VOID
IICSdump_statistics(print_fcn)
i4	    (*print_fcn)(char *, ...);
{
    char	buffer[150];

    STprintf(buffer, "========================================================\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "SERVER STATISTICS:\n====== ==========\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Disk completions: dio_resumes %d fast_resumes %d\n",
	     dio_resumes, fast_resumes);
    (*print_fcn)(buffer);
    STprintf(buffer, "Server CPU seconds: %d.%d\n",
	    Cs_srv_block.cs_cpu / 100, Cs_srv_block.cs_cpu % 100);
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
    STprintf(buffer, "    Multi-Process Wait loops: %8d. Redispatches: %8d.\n",
		Cs_srv_block.cs_smstatistics.cs_smcl_count,
		Cs_srv_block.cs_smstatistics.cs_smmp_count);
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
    STprintf(buffer, "              Requests  Wait State  Avg. Wait	 Zero Wait  Wait Time\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "              --------  ----------  ---------	 ---------  ---------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "    BIOR:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_bior_done,
		    Cs_srv_block.cs_wtstatistics.cs_bior_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_bior_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_bior_time /
			    Cs_srv_block.cs_wtstatistics.cs_bior_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_bior_done -
			Cs_srv_block.cs_wtstatistics.cs_bior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_bior_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    BIOW:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_biow_done,
		    Cs_srv_block.cs_wtstatistics.cs_biow_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_biow_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_biow_time /
			    Cs_srv_block.cs_wtstatistics.cs_biow_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_biow_done -
			Cs_srv_block.cs_wtstatistics.cs_biow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_biow_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    DIOR:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_dior_done,
		    Cs_srv_block.cs_wtstatistics.cs_dior_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_dior_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_dior_time /
			    Cs_srv_block.cs_wtstatistics.cs_dior_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_dior_done -
			Cs_srv_block.cs_wtstatistics.cs_dior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_dior_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    DIOW:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_diow_done,
		    Cs_srv_block.cs_wtstatistics.cs_diow_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_diow_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_diow_time /
			    Cs_srv_block.cs_wtstatistics.cs_diow_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_diow_done -
			Cs_srv_block.cs_wtstatistics.cs_diow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_diow_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LIOR:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_lior_done,
		    Cs_srv_block.cs_wtstatistics.cs_lior_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_lior_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_lior_time /
			    Cs_srv_block.cs_wtstatistics.cs_lior_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_lior_done -
			Cs_srv_block.cs_wtstatistics.cs_lior_waits,
		    Cs_srv_block.cs_wtstatistics.cs_lior_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    LIOW:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
		    Cs_srv_block.cs_wtstatistics.cs_liow_done,
		    Cs_srv_block.cs_wtstatistics.cs_liow_waits,
		    (Cs_srv_block.cs_wtstatistics.cs_liow_waits ?
			(Cs_srv_block.cs_wtstatistics.cs_liow_time /
			    Cs_srv_block.cs_wtstatistics.cs_liow_waits) : 0),
		    Cs_srv_block.cs_wtstatistics.cs_liow_done -
			Cs_srv_block.cs_wtstatistics.cs_liow_waits,
		    Cs_srv_block.cs_wtstatistics.cs_liow_time);
    (*print_fcn)(buffer);
    STprintf(buffer, "    Log:     %8d.   %8d.  %8d.   %8d.   %8d.\n",
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
    STprintf(buffer, "    Lock:    %8d.   %8d.  %8d.   %8d.   %8d.\n",
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
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "Idle Statistics:\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "---- -----------\n");
    (*print_fcn)(buffer);
    STprintf(buffer, "	  Idle quanta: %d./%d. (%d. %%)\n",
			Cs_srv_block.cs_idle_time,
			Cs_srv_block.cs_quantums,
			(Cs_srv_block.cs_quantums ?
			    ((Cs_srv_block.cs_idle_time * 100) /
				Cs_srv_block.cs_quantums) : 100));
    (*print_fcn)(buffer);
    STprintf(buffer, "	  BIO: %8d., DIO: %8d., Log: %8d., Lock: %8d., Time: %8d.\n",
			Cs_srv_block.cs_wtstatistics.cs_bio_idle,
			Cs_srv_block.cs_wtstatistics.cs_dio_idle,
			Cs_srv_block.cs_wtstatistics.cs_lg_idle,
			Cs_srv_block.cs_wtstatistics.cs_lk_idle,
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
**	This routine returns TRUE if the server can accept another connection
**	request. (thread). This routine was
**	invented to be passed into GCA to solve
**	the GCA rollover problem. The GCA rollover problem is where GCA needs
**	to determine whether a user can use a dbms server before the GCA_LISTEN
**	completes. If the user can't use a DBMS, then GCA rolls over to the
**	next possible DBMS on it's list. If the user can use the DBMS server
**	then, obviously, the listen completes. By necessity, this routine
**	is a duplicate of the similiar check in CS_admin_task().
**
**	CAUTION: If you change the input (in this case, the lack of input) or
**	the output (like, the fact that it returns TRUE or FALSE), then
**	you must coordinate the change with what GCA expects as input and
**	output. The address of this routine is passed into GCF from SCF.
**	This is just a long-winded, detailed, way of saying that this routine
**	is a part of the CL spec, and interface changes need to be approved.
**
**	This routine first sets a semaphore in the admin tasks scb, to insure
**	that we are looking at a consistent set of data. We then check if
**	the server is at the limit for the number of threads or if the server
**	is shutting down. If one of these is true, then tell the caller
**	that we cannot accept any more connections, otherwise returne that
**	we are still accepting connections.
[@comment_line@]...
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
**	20-jan-89 (eric)
**	    written
**	25-Jan-1999 (jenjo02)
**	    Check cs_user_sessions, the number of external sessions,
**	    instead of cs_num_sessions, the total number of internal AND
**	    external sessions. With dynamic internal sessions, cs_num_sessions
**	    is no longer deterministic!
[@history_template@]...
*/
bool
CSaccept_connect(void)
{
    bool    ret;
    char    buf[ER_MAX_LEN];

    if (((Cs_srv_block.cs_user_sessions + 1) >
		Cs_srv_block.cs_max_sessions)	||
	(Cs_srv_block.cs_mask & CS_SHUTDOWN_MASK))
    {
	(void)ERreport((i4)E_CS000E_SESSION_LIMIT, buf);
        TRdisplay("%s", buf);
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
**      this call imposes "negligible" overhead.
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
**      20-apr-94 (mikem)
**          Bug #57043
**          Added a call to CS_checktime() to CSswitch().  This call
**          is responsible for keeping the pseudo-quantum clock up to date,
**          this clock is used to maintain both quantum and timeout queues in
**          CS.  See csclock.c for more information.
**	04-Oct-2000 (jenjo02)
**	    Renamed to IICSswitch, removed cs_mt test.
**	13-Feb-2007 (kschendel)
**	    Added cancel-check, maps to CSswitch on internal threads.
*/
VOID
IICSswitch()
{
    if (Cs_incomp)
    {
	CS_breakpoint();
	return;
    }

    CS_checktime();
    if (Cs_srv_block.cs_state != CS_INITIALIZING &&
         (Cs_sm_cb->css_qcount - Cs_lastquant) > Cs_srv_block.cs_quantum)
    {
	CS_swuser();
    }
}

void
IICScancelCheck(CS_SID sid)
{
    IICSswitch();
}

# if defined(ibm_lnx)
void CS_ibmsetup()
{
    CS_setup();
    CS_eradicate();
}
# endif /* ibm_lnx */

# ifdef SIGDANGER
/*{
** Name: CSsigdanger - exception handler for SIGDANGER.
**
** Description:
**	This is the exception hander for sigdangers. On AIX (RS/6000),
**	all processes receive this signal when swap space is going low.
**	It prints out a warning message in the errlog. If SIGDANGERs exist
**	on your system and you want to do something when you receive them,
**	you should add the code here.
**
** History:
**      2-apr-91 (vijay)
**          Created
*/
#ifdef  xCL_011_USE_SIGVEC

TYPESIG
CSsigdanger(signum, code, scp)
i4      signum;
i4      code;
struct sigcontext *scp;

# else

TYPESIG
CSsigdanger(signum)

# endif
{
      char            msg[128];
      CL_ERR_DESC     error;

# if !defined(xCL_011_USE_SIGVEC)
        /* re-establish handler */
        signal(signum, CSslave_danger);
# endif

# if defined(any_aix)
      STcopy("\nServer: SIGDANGER: swap space running low ?!\n", msg);
      ERlog( msg, STlength(msg), &error);
# endif
}
# endif /* SIGDANGER */


/* Address comparision function for SPTREEs */

i4
CS_addrcmp( const char *a, const char *b )
{
    if ( a == b )
        return 0;
    else
        return( a > b ) ? 1 : -1;
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
**	VOID
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the portable logging and locking system.
**	26-jul-1993 (bryanp)
**	    In CSms_thread_nap, if ms <= 0, just return immediately.
*/
VOID
CSms_thread_nap(i4	ms)
{
    if (ms <= 0)
	return;

    CSsuspend(CS_TIMEOUT_MASK, - ms, 0 );

    return;
}

/*
**
**	Function:
**		CSget_sid (remains CSget_sid as wrapper to II routine).
**
**	History:
**	18-oct-1993 (kwatts)
**	    Created as wrapper because of name changes.
*/
#ifdef CSget_sid
#undef CSget_sid
VOID
CSget_sid(CS_SID *sidptr)
{
	IICSget_sid(sidptr);
}
#endif

/*
**
**	Function:
**		CSresume (remains CSresume as wrapper to II routine).
**
**	History:
**	18-oct-1993 (kwatts)
**	    Created as wrapper because of name changes.
*/
#ifdef CSresume
#undef CSresume
VOID
CSresume(CS_SID sid)
{
    IICSresume(sid);
}
#endif

/*
**
**	Function:
**		CSsuspend (remains CSsuspend as wrapper to II routine).
**
**	History:
**	18-oct-1993 (kwatts)
**	    Created as wrapper because of name changes.
*/
#ifdef CSsuspend
#undef CSsuspend
STATUS
CSsuspend(i4 mask, i4  to_cnt, PTR ecb)
{
    return(IICSsuspend(mask, to_cnt, ecb));
}
#endif

bool
CS_is_mt()
{
    return( Cs_srv_block.cs_mt );
}


/*
** Name: CSadjust_counter() - Thread-safe counter adjustment.
**
** Description:
**	This routine provides a means to accurately adjust a counter
**	which may possibly be undergoing simultaneous adjustment in
**	another session, or in a Signal Handler, or AST, without
**	protecting the counter with a mutex.
**
**	This is done, by taking advantage of the atomic compare and
**	swap routine.   We first calculate the new value based on
**	a "dirty" read of the existing value.  We then use
**	CScas4() to attempt to update the counter.
**	If the counter results had changed before our update is applied,
**	update is canceled, and a new "new" value must be calculated
**	based on the new "old" value returned.  Even under very heavy
**	contention, the caller will eventually get to update the counter
**	before someone else gets in to this tight loop.
**
**	This routime will work properly under both INTERNAL (Ingres)
**	and OS threads.
**
** Inputs:
**	pcounter	- Address of an i4 counter.
**	adjustment	- Amount to adjust by.
**
** Outputs:
**	None
**
** Returns:
** 	Value counter assumed after callers update was applied.
**
** History:
**	29-aug-2002 (devjo01)
**	    created.
**	25-jan-2004 (devjo01)
**	    Put in alternative mutex implementation.
**	6-Jul-2005 (schka24)
**	    Use synch-lock instead of CSp,v so that we can init the
**	    utility mutex real early.
*/
#ifdef NEED_CSADJUST_COUNTER_FUNCTION /* might be defined in csnormal */
i4
CSadjust_counter( i4 *pcounter, i4 adjustment )
{
    i4		oldvalue, newvalue;

#if defined(conf_CAS_ENABLED)
    do
    {
	oldvalue = *pcounter;
	newvalue = oldvalue + adjustment;
    } while ( CScas4( pcounter, oldvalue, newvalue ) );
#elif defined(OS_THREADS_USED)
    CS_synch_lock(&Cs_utility_sem);
    newvalue = (*pcounter += adjustment );
    CS_synch_unlock(&Cs_utility_sem);
#else
    newvalue = (*pcounter += adjustment );
#endif
    return newvalue;
} /* CSadjust_counter */
#endif

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
**	swap routine.   We first calculate the new value based on
**	a "dirty" read of the existing value.  We then use
**	CScas8() to attempt to update the counter.
**	If the counter results had changed before our update is applied,
**	update is canceled, and a new "new" value must be calculated
**	based on the new "old" value returned.  Even under very heavy
**	contention, the caller will eventually get to update the counter
**	before someone else gets in to this tight loop.
**
**	This routime will work properly under both INTERNAL (Ingres)
**	and OS threads.
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
**	20-apr-2010 (stephenb)
**		Created from above CSadjust_counter
*/
#ifdef NEED_CSADJUST_I8COUNTER_FUNCTION /* might be defined in csnormal */
i8
CSadjust_i8counter( i8 *pcounter, i8 adjustment )
{
    i8		oldvalue, newvalue;

#if defined(conf_CAS8_ENABLED)
    do
    {
	oldvalue = *pcounter;
	newvalue = oldvalue + adjustment;
    } while ( CScas8( pcounter, oldvalue, newvalue ) );
#elif defined(OS_THREADS_USED)
    CS_synch_lock(&Cs_utility_sem);
    newvalue = (*pcounter += adjustment );
    CS_synch_unlock(&Cs_utility_sem);
#else
    newvalue = (*pcounter += adjustment );
#endif
    return newvalue;
} /* CSadjust_i8counter */
#endif


#ifdef PERF_TEST

/*
** Name: CSfac_stats - collect stats on a per-facility basis
**
** Description:
**	This routine collects stats on a per-facility basis, and places
**	the results in the CS_SYSTEM struct, tracing is enabled through the
**	use of SCF trace points.
**	NOTE: updates are made to the global Cs_srv_block here without
**	semaphore protection, so the stats are not guarunteed to be 100%
**	accurate, most tests so far have been single user so it's not
**	an issue. CSp_semaphore() uses up alot of CPU which realy defeats
**	the point of the profiling. But if accurate multi-user values are
**	required, you'll have to add a new CS_MUTEX to CS_SYSTEM to protect
**	the CS_PROFILE values. In 2.0 we could use CS_synch_lock, but it's
**	not available in earlier releases.
**
** Inputs:
**	start	start or end collecting
**	fac_id	facility id to collect for
**
** Outputs:
**	None
**
** Returns:
** 	None
**
** History:
**	28-nov-96 (stephenb)
**	    (re)created.
*/
VOID
CSfac_stats(
	bool	start,
	int	fac_id)
{
    int			i;
    struct tms		time_info;
    int			errnum = errno;
    f8			cpu_used;
    CL_ERR_DESC		*err_code;

    if (Cs_srv_block.cs_cpu_zero == 0	/* profiling not enabled */
	|| fac_id == 0)			/* nothing to profile */
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
	    SETCLERR (err_code, errnum, 0);
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, err_code, 0);
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
	    SETCLERR (err_code, errnum, 0);
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, err_code, 0);
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

/*
** Name: CScollect_stats - start/stop stats collection or print current stats.
**
** Description:
**	This routine starts or stops per-facility stats collection, or if 
**	start is requested and we are alread collecting, it will print out
**	the current stats values
**
** Inputs:
**	start	start/stop or print
**
** Outputs:
**	None
** 
** Returns:
**	VOID
**
** Side Effects:
**	may print stats to II_DBMS_LOG
**
** History:
**	28-nov-96 (stephenb)
**	    (re)created.
*/
VOID
CScollect_stats(bool	start)
{
    int			i,j;
    int			errnum = errno;
    struct tms		time_info;
    f8			total_used;
    CL_ERR_DESC		*err_code;

    if (start && Cs_srv_block.cs_cpu_zero)
    {
	/* stats collection already running, print out current status */
	/* dont bother mutexing this, we're just prining current approx */
	/* stats */
	TRdisplay("Fac\tSub-fac\t\tCalls\tOverloaded\tCPUsec\n");
	for (i = 1; i <= CS_MAX_FACILITY; i++)
	{
	    if (cs_facilities[i].sub_facility_id == 0 && 
		Cs_srv_block.cs_profile[i].cs_calls) /* only print if called */
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
	    SETCLERR(err_code,&errnum,0);
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, err_code, 0);
	    return;
        }
	else
	{
	    total_used = time_info.tms_utime + time_info.tms_stime;
	    total_used = total_used  / HZ;
	    total_used -= Cs_srv_block.cs_cpu_zero;
	    TRdisplay("----------------------------------------------------\n");
	    TRdisplay("Total\t\t\t\t\t\t%.2f\n",total_used);
	}

    }
    else if (start)
    {
	/* start stats collection, cs_cpu_zero could hold the base CPU usage
	** at start time, but for now we'll just use it as an indicator */
        if (times(&time_info) < 0)
        {
	    SETCLERR(err_code,&errnum,0);
            (*Cs_srv_block.cs_elog)(E_CS0016_SYSTEM_ERROR, err_code, 0);
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

/*
** Name: CSfac_trace - collect trace info on a per-facility basis
**
** Description:
**	This routine, when enabled, will add the caller's facility and parm
**	to the memory trace table to be printed later.
**	Initially, the tracing will be at the facility id.  Later, 
**	if this proves to be useful, more details can be added.
**
** Inputs:
**	start	start or end collecting
**	fac_id	facility id
**	ptr     addr of additional info to trace e.g. addr(parms)
**
** Outputs:
**	None
**
** Returns:
** 	None
**
** History:
**	21-may-1997 (shero03)
**	    created.
**	04-Dec-1997 (merja01)
**		Added eye_catcher
*/
VOID
CSfac_trace(
	int	where,
	int	fac_id,
	int	opt_parm,
	PTR     opt_addr)
{
    int			i;
    CS_SCB		*scbptr;
    CS_DEV_TRCENTRY	*curr_entry;

    if (fac_id == 0)			/* nothing to profile */
	return;

    if (Cs_srv_block.cs_dev_trace_strt == NULL)
    {
	char eye_catcher [24] = {"__INGRES_TRACE_FACILITY_"};
	int trace_buf_size = (sizeof(CS_DEV_TRCENTRY) * CS_DEV_TRACE_ENTRIES);
	/* getmain an area of say 5,000 trace entries */
	/* initialize them to nulls */
	curr_entry = (CS_DEV_TRCENTRY *)malloc( trace_buf_size + sizeof(eye_catcher));
	MEfill((trace_buf_size + sizeof(eye_catcher)), 0, curr_entry);
	Cs_srv_block.cs_dev_trace_strt = curr_entry;
	Cs_srv_block.cs_dev_trace_end = curr_entry + CS_DEV_TRACE_ENTRIES;

	curr_entry = 
	Cs_srv_block.cs_dev_trace_curr = Cs_srv_block.cs_dev_trace_end - 1;

	/* Add eye catcher to cs_dev_trace_end*/
	strncpy(Cs_srv_block.cs_dev_trace_end, eye_catcher, sizeof(eye_catcher));
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

/*
** Name: CSkill - forcibly cause a fatal exception in a thread
**
** Description:
**      When a thread cannot be removed any other way, CSkill
**      will raise an exception in the thread that can be handled
**      by any registered exception handlers.
**
** Inputs:
**      sid             session id of the thread
**      force           FALSE - will not kill thread holding semaphore
**                      TRUE  - will kill thread no matter what
**      waittime        number of milliseconds to wait before trying
**                      to kill it
**
** Returns:
**      OK              completed successfully
**      E_CS001C_TERM_W_SEM - if holding semaphore
**
** History:
**      22-jul-1997 (canor01)
**          Created.
*/
STATUS
CSkill( CS_SID sid, bool force, u_i4 waittime )
{
    CS_SCB              *scb;
    STATUS              retval;
    bool                active = FALSE;
    i4                  saved_state;
    i4                  i;
    QUEUE               *semlist;
    CS_SEMAPHORE        *sem;
 
    /* first, give it some time to complete */
    PCsleep( waittime );
 
    /* lock known scb list */
    CSp_semaphore( TRUE, &Cs_known_list_sem );
 
    /* see if the thread is still on the list */
    for (scb = Cs_srv_block.cs_known_list->cs_prev;
         scb != Cs_srv_block.cs_known_list;
         scb = scb->cs_prev)
    {
        if ( scb->cs_self == sid )
        {
            active = TRUE;
            break;
        }
    }
 
    if ( !active )
    {
        /* session is already gone */
        CSv_semaphore( &Cs_known_list_sem );
        return ( OK );
    }
 
    /* lock its state */
# ifdef OS_THREADS_USED
    if (Cs_srv_block.cs_mt)
	CS_synch_lock( &scb->cs_evcb->event_sem );
# endif /* OS_THREADS_USED */
    saved_state = scb->cs_state;
    scb->cs_state = CS_UWAIT;
    scb->cs_mask |= CS_DEAD_MASK;
# ifdef OS_THREADS_USED
    if (Cs_srv_block.cs_mt)
	CS_synch_unlock( &scb->cs_evcb->event_sem );
# endif /* OS_THREADS_USED */
 
    retval = OK;
 
    /* is it holding any semaphores? */
    if ( scb->cs_sem_count > 0 )
        retval = E_CS001C_TERM_W_SEM;
 
    /* generate an exception in the session */
    if ( force || retval == OK )
    {
# ifdef OS_THREADS_USED
	if (Cs_srv_block.cs_mt)
	    CS_thread_kill( scb->cs_self, SIGILL );
# else /* OS_THREADS_USED */
# endif /* OS_THREADS_USED */
    }
 
    /* release known scb list */
    CSv_semaphore( &Cs_known_list_sem );
 
    return ( retval );
}

static i4
CS_usercnt()
{
    return(Cs_srv_block.cs_user_sessions);
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
** Outputs:
**      Prints message to ERRLOG if edone bit set for thread
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
# ifdef OS_THREADS_USED
    if ( (Cs_srv_block.cs_mt) )
    {
       CSMTnoresnow( descrip, pos );
       return;
    }
# endif /* OS_THREADS_USED */

   if (Cs_srv_block.cs_current)
   {
      if (Cs_srv_block.cs_current->cs_mask & CS_EDONE_MASK)
      {
         TRdisplay("[%x, %x] %@  EDONE bit set prior to call from %s Pos %d",
                   Cs_srv_block.cs_pid, Cs_srv_block.cs_current, descrip, pos);

         CScancelled(0);
      }
   }
}

i4
CS_atomic_increment( CS_AINCR *aincr )
{
# ifdef OS_THREADS_USED
    i4 value;
    CS_synch_lock( &aincr->cs_mutex );
    aincr->cs_value++;
    value = aincr->cs_value;
    CS_synch_unlock( &aincr->cs_mutex );
    return ( value );
# else /* OS_THREADS_USED */
    (*aincr)++;
    return (*aincr);
# endif /* OS_THREADS_USED */
}

i4
CS_atomic_decrement( CS_AINCR *aincr )
{
# ifdef OS_THREADS_USED
    i4 value;
    CS_synch_lock( &aincr->cs_mutex );
    aincr->cs_value--;
    value = aincr->cs_value;
    CS_synch_unlock( &aincr->cs_mutex );
    return ( value );
# else /* OS_THREADS_USED */
    (*aincr)--;
    return (*aincr);
# endif /* OS_THREADS_USED */
}

STATUS
CS_atomic_init( CS_AINCR *aincr )
{
    STATUS status = OK;
# ifdef OS_THREADS_USED
    CS_cp_synch_init( &aincr->cs_mutex, &status );
    aincr->cs_value = 0;
    return (status);
# else /* OS_THREADS_USED */
    aincr = 0;
    return (status);
# endif /* OS_THREADS_USED */
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
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_bior_time +
		        Cs_srv_block.cs_wtstatistics.cs_biow_time),
		lsbuf, sbuf));
}
static STATUS
CSmo_bio_waits_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_bior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_biow_waits),
		lsbuf, sbuf));
}
static STATUS
CSmo_bio_done_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_bior_done +
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
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_dior_time +
		        Cs_srv_block.cs_wtstatistics.cs_diow_time),
		lsbuf, sbuf));
}
static STATUS
CSmo_dio_waits_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_dior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_diow_waits),
		lsbuf, sbuf));
}
static STATUS
CSmo_dio_done_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_dior_done +
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
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_lior_time +
		        Cs_srv_block.cs_wtstatistics.cs_liow_time),
		lsbuf, sbuf));
}
static STATUS
CSmo_lio_waits_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_lior_waits +
		        Cs_srv_block.cs_wtstatistics.cs_liow_waits),
		lsbuf, sbuf));
}
static STATUS
CSmo_lio_done_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)(Cs_srv_block.cs_wtstatistics.cs_lior_done +
		        Cs_srv_block.cs_wtstatistics.cs_liow_done),
		lsbuf, sbuf));
}

/*{
** Name:	CSmo_num_active_get
**		CSmo_hwm_active_get
**				    -- MO get methods for active
**				       sessions and the high water
**				       mark.
**
** Description:
**
**	Returns the number of active threads, or the
**	high water mark of active threads.
**
** Inputs:
**	offset		ignored.
**	size		ignored.
**	object		ignored.
**	lsbuf		length of output buffer.
**	sbuf		output buffer.
**
** Outputs:
**	sbuf		written with appropriate value.
**	
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	22-May-2002 (jenjo02)
**	    Created.
*/
static STATUS
CSmo_num_active_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)CS_num_active(),
		lsbuf, sbuf));
}
static STATUS
CSmo_hwm_active_get(
    i4 offset, i4 size, PTR object, i4 lsbuf, char *sbuf )
{
    return (MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)CS_hwm_active(),
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
