/*
** Copyright (c) 1987, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <cm.h>
#include    <cv.h>
#include    <er.h>
#include    <pc.h>
#include    <tm.h>
#include    <cs.h>
#include    <tr.h>
#include    <si.h>
#include    <lo.h>
#include    <nm.h>
#include    <me.h>
#include    <st.h>
#include    <pm.h>
#include    <ex.h>	/* (For exception handling) */

#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <csp.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulx.h>
#include    <dmf.h>
#include    <scf.h>
#include    <dm.h>
#include    <lgkparms.h>
#include    <lgclustr.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0llctx.h>
#include    <dm0c.h>
#include    <dmve.h>
#include    <dmccb.h>
#include    <dmfcsp.h>
#include    <dmfrcp.h>
#include    <dmfacp.h>
#include    <dmfinit.h>
#include    <dmftrace.h>
#include    <dmfjsp.h>
#ifdef VMS
#include    <ssdef.h>
#include    <starlet.h>
#include    <astjacket.h>
#endif

/**
**
**  Name: DMFCSP.C - DMF Cluster Server Process.
**
**  Description:
**	Function prototypes in DMFCSP.H.
**
**      This module contains the CSP.  The CSP controls DMF installations
**	running of different machines in the sames cluster.  The CSP
**	coordinates machines joining the cluster and machine leaving the
**	cluster in a controlled or uncontrolled manner.  The CSP is responsible
**	for maintaining the consistency of the various DMF installations.
**	A CSP on every node communicates with a CSP on every other node.
**	The CSP on each node then controls the actions of the DMF installation
**	on the same node.
**
**  History:    
**      1-jun-1987 (derek)
**          Created.
**      1-jun-1988 (ac)
**          Added new states and misc fix.
**	31-aug-1989 (sandyh)
**	    Change lgh_status to LGH_OK from LGH_VALID after node_fail
**	    recovery. This prevents the RCP on re-boot from re-doing
**	    completed work, which could knock the db inconsistent.
**      30-Nov-1989 (ac)
**          Fixed several node join/leave/add problems.
**      6-Dec-1989 (ac)
**          Added work around to the VMS RMS internal error condition that
**	    occurs whenever there are many pending decnet messages to a
**	    remote node. This happens when the remote node has a much slower
**	    response time in processing network messages.
**      02-Jan-1990 (ac)
**          Don't bring down CSP if ACP died. ACP can be manually brought up.
**	    ACP will be brought down when RCP or CSP exit.
**	05-Jan-1990 (ac)
**	    Added CSP exit handler to cache any abnormal exit context.
**      12-Jan-1990 (ac)
**          Added code to have CSP more intelligently in sending continuous
**	    deadlock search messages. Maintain a cache of messages sent in
**	    CSP.
**	15-may-1990 (sandyh)
**	    Added MSG_A_ADD_ERROR to RUN_JOIN state. Also, did general cleanup
**	    of TRdisplays by adding iosb status output to all pertinent areas.
**	    Changed iosb output to text thru call to iosb_error().
**	25-jun-1990 (sandyh)
**	    fixed dcl_delete_node() to properly process deleting nodes from
**	    cluster configuration. Also, added SS$_PATHLOST & SS$_THIRDPARTY
**	    handler in read_complete() to insure that a node transition takes
**	    place so that the cluster will stay in sync. In the future this
**	    needs to be made a transparent operation.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	11-oct-1990 (bryanp)
**	    Bug #33725
**	    Correctly handle E_DB_INFO from dma_prepare() in csp_node_fail().
**	19-nov-1990 (bryanp)
**	    Bug #34179: Take Sandy's suggestion & handle all link failures the
**	    same way. We were, in some cases, ignoring a link failure,
**	    assuming that the associated node was in the process of failing and
**	    we would then take action when the node lock changed state, but it
**	    seems that in some cases the link fails but the node remains
**	    alive, causing a confused cluster (not all nodes can talk to each
**	    other, but all nodes remain running).
**      14-feb-1990 (jennifer)
**          Bug #34179:  Bryans's fix from above was a good attempt at 
**          solving this problem, however his action needed to be delayed
**          long enough to allow a re-mastering in case connections are
**          lost to the master.  If everyone kills themseleves right away
**          there is no-one left to become master if master node was lost.
**          A new state LEAVE_DELAY was added to allow a CSP to wait to 
**          see if the a node lock comes in indicating a node died not just
**          the decnet connection.  If the node lock comes in, normal 
**          processing occurs after the timer is cancelled.  If the 
**          timer goes off, then bryans algorithm is used to determine if
**          the CSP should kill itself.
**	25-feb-1990 (Derek)
**	    Fixed various timing problems found during testing of the above
**	    timeout fixes.  Tried to colve all problems that resulted in
**	    CSP's stuck in incorrect states, or master node count being off
**	    by one, or messages that needed to be handled but were not which
**	    caused the CSP to hang.  Didn't try and fix cosmetic errors in the
**	    log that didn't effect the correct functioning of the CSP.
**	25-mar-1991 (rogerk)
**	    Changed to start archiver by starting up a loginout process
**	    which runs acp_start1.com.  This was done as part of the
**	    Archiver Stability changes to allow the archiver to execute
**	    the acpexit script on termination.
**	26-mar-1991 (walt)
**	    If __ACCDEF_LOADED is defined, we are using the new 3.x C compiler
**	    and the sys$library:accdef structure previously called 'acc$record'
**	    is now called 'accdef'.  Added an ifdef in read_complete() to pick
**	    the correct one.
**	19-aug-1991 (rogerk)
**	    Set process type in the svcb.  This was done as part of the
**	    bugfix for B39017.  This allows low-level DMF code to determine
**	    the context in which it is running (Server, RCP, ACP, utility).
**	19-aug-1991 (rogerk)
**	    Bug # 37912
**	    Fixed bug in above LEAVE_DELAY fix.  Added CSP_LEAVE_DELAY state
**	    to the list of commonly handled states for the MSG_A_LEAVE_MASTER
**	    message handling (at the end of execute_msg is a list of common
**	    message types that are handled for many different states).
**	    This fixes a hanging problem in the CSP if a node failure is
**	    recognized first by a loss of decnet rather than the dropping of
**	    the master-csp lock.  The loss of DECNET puts us into LEAVE_DELAY
**	    state.  If we don't then handle the LEAVE_MASTER message, then
**	    we never elevate ourselves to become the true master, even though
**	    we have been elected as such.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5.
**	5-oct-1990 (bryanp)
**	    Added Cluster Dual Logging support.
**	    Renamed "NODE" to "CSP_NODE" to avoid conflict with <lo.h>.
**	10-apr-1992 (rogerk)
**	    Fix call to ERinit().  Added missing args: p_sem_func, v_sem_func.
**	    Even though we do not pass in semaphore routines, we still need
**	    to pass the correct number of arguments.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	25-june-1992 (jnash)
**	    Change name of CSP log file from II_CSP.LOG to IICSP.LOG.
**	21-jul-1992 (jnash)
**	    If II_DMFRCP_STOP_ON_INCONS_DB set, crash rather than
**	    marking a database inconsistent.  Also, replace check_event
**	    definition inadvertantly removed during dmf prototyping,
**	    this time with a parameter.
**	3-sep-1992 (bryanp)
**	    Changes for the new "portable" logging and locking system.
**	5-nov-1992 (bryanp)
**	    Check CSinitate return code.
**	14-dec-1992 (jnash)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project:
**	    Changed dmr_recover flag for CSP to always indicate CSP, either
**	    online or offline.  Node failure recovery now indicates a different
**	    mode than normal RCP online recovery.
**	16-mar-1993 (rmuth)
**	    include di.h
**	26-apr-1993 (bryanp/andys/keving)
**	    6.5 Cluster Project I:
**		Add real code to perform 6.5 recovery.
**		Get LG/LK Configuration parameters from PM, not rcpconfig.dat
**		Initialize SCF/CS at startup so that semaphores work properly.
**		Replaced call to dmr_get_lglk_parms with get_lgk_config
**		Added include of lgkparms.h to pick up RCONF definition
**		Change name of get_lgk_config to lgk_get_config.
**		Include dmfinit.h to pick up dmf_init prototype.
**		Pass correct arguments to dmf_init().
**		Fix put/get_action to remove extra sys$wake call
**	15-may-1993 (rmuth)
**	    Add support for new configuration tools.
**	     a. Remove the cluster.cnf stuff and replace with the PM
**	        system
**	     b. Issue a wraning message is the interactive interface is used
**	        to CREATE/ADD or DELETE nodes.
**	     c. Add the node running lock.
**	24-may-1993 (bryanp)
**	    Log useful error messages if lgk_get_config fails.
**	    Resource blocks are now configured separately from lock blocks.
**	21-jun-1993 (bryanp)
**	    Remove usage of CMKRNL. All locks are user mode locks, now.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	30-jun-1993 (shailaja)
**	    Fixed compiler warning on empty translation unit.
**	26-jul-1993 (bryanp)
**	    Added csp_lock_list argument to dmf_init()
**	    Replaced MAX_NODE_NAME_LENGTH with LGC_MAX_NODE_NAME_LEN.
**	    Use CL_OFFSETOF to calculate offsets.
**	    Check the return code from ERslookup -- if it fails, don't try to
**		display the formatted message.
**	    Check the return code from TRset_file -- if it fails, there's
**		probably some severe configuration errors, so stop immediately.
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**      11-aug-93 (ed)
**          added missing includes
**	23-aug-1993 (bryanp)
**	    Change default DMF buffer cache size from 256 buffers to 1024
**		buffers.
**	    Allow DMF buffer cache size to be settable via the
**		II_CSP_DMF_CACHE_SIZE variable.
**	20-sep-1993 (bryanp)
**	    Check enqueue limit when starting up.
**	    Call csp_archive for each log after recovering the log.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare call.
**	28-mar-1994 (bryanp) B60058
**	    Disable MSG_T_RESUME_PHYSICAL_LOCKS until we better understand how
**		to manage the locking semantics during node failure recovery.
**		This broadcast message was only partially implemented, anyway,
**		and when it occurred it was causing the CSP state machine to
**		crash.
**	21-aug-1995 (dougb)
**	    As a related update to the fix for bug #70696, handle the
**	    SS$_INSFMEM return from LKdeadlock().  The bug fix may increase
**	    the fan-out for a deadlock search (potentially, by a large
**	    amount).  Thus, it is a "bad" idea to have any fixed limits on
**	    the number of output messages.
**	22-aug-1995 (dougb)
**	    Also related to bug #70696:  Don't stall the entire CSP process
**	    while waiting to begin a global deadlock search.
**	    Remove unused and confusing routine csp_start().
**	    Don't use async system-services when we want the info immediately.
**	25-aug-1995 (dougb)
**	    Renamed routine from deadlock to gbl_deadlock to avoid confusion
**	    with LKRQST.C's static routine of same name.  That routine looks
**	    for a local deadlock condition.
**	    Added new do_getlki() support routine for gbl_deadlock() and
**	    deadlock_search().
**	28-aug-1995 (dougb)
**	    Use symbol exported by LK facility for lock compatibility
**	    instead of defining our own in gbl_deadlock().
**	    Added new disp_dlck_msg() support routine for do_getlki() and
**	    deadlock_search().
**	29-aug-1995 (dougb)
**	    Modify do_getlki() to request *all* requests for a particular
**	    resource.  This handles fact that LKdeadlock() now informs us
**	    of the maximum request mode, which may not be the mode of any
**	    local lock.  Information returned to gbl_deadlock() and
**	    deadlock_search() will continue to contain only blocking locks.
**	30-sep-1995 (dougb)
**	    in csp_initialize(), add TRdisplay() calls when LIB$GET_EF() returns
**	    a bogus value, or when a bad deadlock interval is specified.
**	14-Nov-1995 (teresak)
**	    Modify include of lkidef.h to explicitly include it from 
**	    sys$library. It is now included in the same manner that other
**	    VMS library header files are.
**	14-nov-1995 (duursma)
**	    Do not pass a NULL pointer to LKdeadlock's out_count (out) parameter,
**	    it will ACCVIO if/when LKdeadlock writes to it.
**	15-jan-1996 (duursma) bug #71340
**	    Fixes to detect convert/convert deadlocks (part of 71340).
**	    Changed do_getlki() to return converting locks ahead of the current
**	    lock in case we are looking for convert/convert deadlock.  It has
**	    a new parameter cvt_cvt which indicates whether we are.
**	    Also added some more debugging output in various places.
**      06-mar-1996 (stial01)
**          Variable page size project:
**           Additional CSP environment variables: II_CSP_DMF_CACHE_SIZE_pgsize
**           Call dmf_init with additional buffers parm (nil)
**           Fix calls to dm0p_allocate
**	03-jun-1996 (canor01)
**	    Semaphore protection of globals is unnecessary in single-threaded
**	    CSP server.
**	3-may-1996 (boama01)
**	    Add exception handler logic to trap VMS conditions; copied
**	    handler from dmfacp.c, added logic for ACCVIO msg formatting;
**	    EXdeclare/EXdelete calls added to csp_initiate; added static
**	    exception_noted to prevent recursive handling.
**	21-aug-1996 (boama01)
**	    Integration of many chgs for Cluster Support bugs uncovered in
**	    OI1.2/00(axp.vms/00) cluster and QA tests (no bug nos.):
**	    - Moved csp_debug GLOBALDEF to LKINIT.C for use by LK routines as
**	      well; here it is now a GLOBALREF.  Also moved its initialization
**	      down after the LKinitialize() call in csp_initialize().
**	    - Fix bug in do_getlki() array search of local_inf entries,
**	      causing garbage to be returned in lki_info; see rtn for details.
**	    - Inhibit new lock rqsts from being AST-delivered while in
**	      stalled-lock-completion phase of node recovery:
**	      * Before calls to LKalter() for LK_A_STALL_* (which stop stalling
**	        and complete stalled rqsts), set on LKD_CSP_HOLD_NEW_RQSTS;
**	        this causes next AST-delivered rqst to be put on "hold";
**	      * After these calls are done, allow AST-delivered rqsts again;
**	        if LKD_CSP_HELD_NEW_RQST is on, fire LK_csp_mbx_complete() to
**	        complete held request; otherwise just reset flag manually.
**	      * Added func prototype for LK_csp_mbx_hold(), which performs
**	        these operations; called from various recovery points.
**      05-aug-1997 (teresak)
**          Remove reference for VMS to exhdef.h being located in sys$library
**          as this is not a system header file but an Ingres header file
**          (located in jpt_clf_hdr).
**	18-aug-1997 (teresak)
**	    Remove extraneous comment terminator that was accidentally integrated with last
**	    integration
**	
**	10-oct-1996 (kamal03)
**		Made some changes specific for ALPHA conditional:
**		EX_CONTEXT structures are defined differently for VAX and
**		ALPHA.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	14-May-1998 (jenjo02)
**	    Add per-cache WriteBehind parameter to dm0p_allocate() call.
**	30-aug-1998 (kinte01)
**		Pass correct number of arguments to ERinit. Has been increased
**		Added in i_sem_func & n_sem_func
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0l_deallocate function.
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	09-May-2000 (bonro01/ahaal01)
**	    AIX system header context.h has a conflicting definition
**	    for jmpbuf which redefined it to __jmpbuf so we changed
**	    variable jmpbuf to iijmpbuf to avoid compiler errors.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-may-2001 (devjo01)
**	    Major rewrite  for s103715.   
**	    - Add dmfcsp_msg_monitor(), dmfcsp_msg_thread().
**	    - Remove vestiges of interactive interface.
**	30-may-2002 (devjo01)
**	    Add ALERT queue stuff.
**	02-Oct-2003 (devjo01)
**	    Tweak action queue handling.
**	24-oct-2003 (devjo01)
**	    change lki$l* variable definitions to lki_l* definitions
**	    to address badmember compiler errors
**	06-nov-2003 (devjo01)
**	    Change lki$b_* variable definitions to lki_b_* definitions.
**	    Should have been done with above changes, and why did
**	    compiler choke on 1st set, but let pass the 2nd?
**	    Also correct compiler warning for pointer type passed to
**	    CScas_ptr.
**	19-nov-2003 (devjo01)
**	    Changes to parameters to CScas_ptr.
**	29-dec-2003 (sheco02)
**	    Change CScas_ptr to CScasptr to resolve undefined symbol.
**	25-jan-2004 (devjo01)
**	    Supress entire module if conf_CLUSTER_BUILD not defined.
**	14-may-2004 (devjo01)
**	    Close CLM connection to foreign node if it crashes, or
**	    leaves.  Move CSP_MSG_2_LEAVE processing to main
**	    CSP thread.
**	19-may-2004 (devjo01)
**	    Correct code that assumed it was safe to format key values
**	    into unused lock value blocks.  Certain DLMs may clobber
**	    this memory even when instructed to ignore the LVB.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**      30-sep-2004 (stial01)
**          online cluster checkpoint: signal consistency point on remote node.
**          Fix non-cluster build related problem.
**      20-oct-2004 (stial01)
**          Fix in csp_ckp_type_names for CKP trace output
**      07-dec-2004 (stial01)
**          Change prefix in ckpdb trace output to CPP (same as in dmfcpp)
**          csp_ckp_lkrelease_all() release LK_CKP_TXN lock
**	08-jan-2005 (devjo01)
**	    - Always pass "sys_err" to crash() to avert SEGV when reporting
**	    gross cluster failure.
**	    - Pass CX_F_NO_DOMAIN to CSP lock call to indicate this lock
**	    is not part of any recovery domain.
**	08-feb-2005 (devjo01)
**	    - Correct way 'csp_debug' is initialized.
**	    - Remove use of legit, but buggy "%#.4{ %x%}" TR format spec.
**      21-mar-2005 (stial01)
**          csp_ckp_msg_init() after CXmsg_redeem, msg is at buf, not buf+8
**	31-Mar-2005 (jenjo02)
**	    Externalized csp_initialize(), now called from dmr_log_init,
**	    to perform ordered recovery on all the log files, including
**	    this process's local log.
**	22-Apr-2005 (fanch01)
**	    Added dbms class failover.  To maintain availability, dbms
**	    classes that were running on failed nodes (that do not appear
**	    elsewhere in the cluster) will be restarted on remaining
**	    cluster nodes.
**	06-May-2005 (fanch01)
**	    Added CMR (cluster member roster) rebuild after node failure.
**	    Several modes of failure have been seen with an incorrect CMR.
**	    1. Failure to start the recovery process due to an invalid LVB.
**	    2. Failure to acquire a deadman lock on an active node and
**	       subsequently recognize rcp failure on another node.
**      11-may-2005 (stial01)
**	    Added LK_CSP deadman locks for dbms servers, key2 = dbms server pid
**          CSP_MSG_3_RCVRY_CMP->CSP_MSG_12_DBMS_RCVR_CMPL, fixed TRdisplays
**	16-may-2005 (devjo01)
**	    - Remove last (unreferenced) vestiges of the old AST driven CSP
**	    code, plus some unused types, and forward refs.
**	    - Changed numerous structure references for CSP message block
**	    to reflect new compacted message structure.
**      24-may-2005 (stial01)
**	    Use PCcmdline instead of PCdocmdline.
**      26-May-2005 (fanch01)
**          Add long message class name functionality.  Removes limitations of
**          dbms class name length fitting into the LVB.  Also allows
**          reasonable use of 16-byte LVB esp for compatibility with VMS.
**	27-may-2005 (devjo01)
**	    - Rename dmfcsp_dbms_rcvry_cmpl to LG_rcvry_cmpl() and
**	    move to LGINIT to prevent linkage problems in IPM.
**	    - Remove unneeded VMS includes.
**      15-jun-2005 (stial01)
**          Added CSP_MAX_DBMS_MARSHALL
**	21-jun-2005 (devjo01)
**	    Add CX_F_IS_SERVER to CXjoin call.
**	20-jul-2005 (devjo01)
**	    'cx_key' is now a struct.  Get rid of all the other key buffers.
**      11-aug-2005 (stial01)
**         Add dcb_status to CKP_CSP_INIT msg so that remote node adds the
**         db to logging system with correct journaling status (b115034)
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    A valid "type" must be supplied on dm0m_allocate calls.
**	10-Nov-2005 (devjo01) b115521
**	   - Correct race condition where we could crash with a E_DM9901.
**	   - Have opc_send also write to error.log for VMS.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**	21-Dec-2007 (jonj)
**	    Rework problematic cluster checkpoint to prevent simultaneous
**	    checkpoints of the same DB from different nodes.
**	11-feb-2008 (joea)
**	    Add node context to recovery messages that end in the errlog.
**	    Use current Ingres spelling.
**      27-jun-2008 (stial01)
**          Added param to dm0l_opendb call.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      22-dec-2008 (stegr01)
**          Itanium VMS port
**      01-apr-2010 (stial01)
**          Changes for Long IDs, db_buffer holds (dbname, owner.. )
**      09-Aug-2010 (maspa05) b123189, b123960
**          Added param to dm0l_opendb call.
*/


#if defined(conf_CLUSTER_BUILD)

/*
** Some useful local macros
*/

/* Add to tail of an action queue */
# define	ACT_ENQUEUE(q,a) ( \
    ((ACTION*)(a))->act_next = ((ACTIONQ*)(q))->prev->act_next, \
    ((ACTION*)(a))->act_prev = ((ACTIONQ*)(q))->prev, \
    ((ACTIONQ*)(q))->prev->act_next = (a), \
    ((ACTIONQ*)(q))->prev = (a) )

/* Remove entry from an action queue */
# define	ACT_DEQUEUE(a) ( \
    ((ACTION*)(a))->act_next->act_prev = ((ACTION*)(a))->act_prev, \
    ((ACTION*)(a))->act_prev->act_next = ((ACTION*)(a))->act_next )

/* Initialize an action queue */
# define	ACT_QUEUE_INIT(q) \
    (((ACTIONQ*)(q))->next = (ACTION *)&((ACTIONQ*)(q))->nextprev, \
     ((ACTIONQ*)(q))->nextprev = NULL, \
     ((ACTIONQ*)(q))->prev = (ACTION *)(q))
    
# define	ACT_QUEUE_EMPTY(q) \
    (((ACTIONQ*)(q))->next->act_next == NULL)

# define	REPORT_ODDITY(msg) \
    (void)TRdisplay( ERx("%@ II_CSP [%s.%d] unlikely event: %s\n"), \
     __FILE__, __LINE__, (char*)(msg) )

/* number of significant class name characters */
#define CSP_MSG_CLASS_BYTES 32

/*
** Definition of all global variables owned by this file.
*/
typedef struct _ACTIONQ
{
    ACTION	*next;		/* Pointer to 1st item on queue, or self */
    ACTION	*nextprev;	/* Always NULL */
    ACTION	*prev;		/* Ptr to last item on queue, or &q.next */
}	ACTIONQ;

typedef struct _CSP_NODE_CLASS_INFO
{
    u_i1	count_have;		/* count of this class running */
    u_i1	count_want;		/* count of this class desired */
    u_i1	node;			/* node class is on */
    char	name[CSP_MSG_CLASS_BYTES];	/* class identifier */
} CSP_NODE_CLASS_INFO;

typedef struct _CSP_NODE_CLASS_INFO_QUEUE
{
    QUEUE	queue;			/* class info queue */
    CSP_NODE_CLASS_INFO	info;		/* info */
} CSP_NODE_CLASS_INFO_QUEUE;

typedef struct _CSP_NODE_CLASS_EVENT_QUEUE
{
    QUEUE	queue;			/* class event queue */
    CSP_CX_MSG	event;			/* event */
    char	class_name[CSP_MSG_CLASS_BYTES];	/* class name */
} CSP_NODE_CLASS_EVENT_QUEUE;

typedef struct _CSP_NODE_CLASS_ROSTER
{
    u_i4  	class_roster_id;	/* request identifier */
    u_i4	class_count;		/* active node/dbms class tuples */
    u_i1	class_roster_build;	/* building class roster? */
    u_i1	class_roster_master;	/* node who was master at time */
    CSP_NODE_CLASS_INFO_QUEUE classes;	/* dbms class roster snapshot */
    CSP_NODE_CLASS_EVENT_QUEUE class_events;/* queued/pending class events */
} CSP_NODE_CLASS_ROSTER;

typedef struct CSP_DBMS_INFO
{
    CX_REQ_CB   deadman_lock_req;	/* CB for DMN lock request */
    					/* THIS MUST BE FIRST FIELD */
					/* csp_dead_dbms_notify assumes this */
}	CSP_DBMS_INFO;

typedef struct CSP_DBMS_SYNC
{
    i2		node;
    i2		lg_id_id;
    PID		dbms_pid;
}	CSP_DBMS_SYNC;

/* long message class */
typedef struct _CSP_NODE_CLASS_LMSG
{
    CSP_NODE_CLASS_INFO	info;		/* class info */
    /* 8-byte padding */
    u_i1	pad[(sizeof(CSP_NODE_CLASS_INFO) + sizeof(u_i1)) % 8];
} CSP_NODE_CLASS_LMSG;

/* Max dbms per node (dbms plus JSP) (array indexed by lg_id) */
#define CSP_MAX_DBMS		128

/* Max dbms per node marshalled (dbms ONLY) */
#define CSP_MAX_DBMS_MARSHALL	16

typedef struct _CSP_NODE_INFO
{
    CS_ASET	node_active;		/* Node is joined to cluster */
    i4		node_failure;		/* Recovery needed for this node */
    CX_REQ_CB	deadman_lock_req;	/* CB for DMN lock request */
    CSP_NODE_CLASS_ROSTER	class_roster;	/* dbms classes node owns */
    CS_SEMAPHORE	node_active_sem;	/* protection for node_active */
    CSP_DBMS_INFO node_dbms[CSP_MAX_DBMS];	/* dbms info for this node */
} CSP_NODE_INFO;

typedef	struct _CSP
{
    i4		csp_state;		/* State of CSP Thread */
#define                 CSP_ST_1_START	1 
#define                 CSP_ST_2_JOINED	2
#define                 CSP_ST_3_TERMED	3
    CS_SID	csp_sid;		/* Session ID of CSP thread */
    i4		csp_self_node;		/* Node # for this CSP */
    i4		csp_master_node;	/* Who is "Master" CSP. */
    LK_UNIQUE	csp_tranid;		/* "dummy" tranid for CSP DLM calls */
    i4		csp_node_failure;	/* At least 1 node fail pending */
    i4		csp_ready_to_sleep;	/* If '1' CSP thread s/b resumed
					   if posting an action to it. */
    i4		csp_dbms_rcvr_cnt;
    CX_REQ_CB	csp_recovery_lock;	/* Lock to guarantee recovery
					   serialization. */
    /*
    ** Notes on the alert structures.  
    **
    ** Alerts are simply copied in a circular buffer, separated by
    ** 4 byte integer length values.  A "length" of -1 means wrap to 
    ** the start, a length of 0 means end of queue.
    */
# define	CSP_ALERT_QUEUE_WRAP	-1
# define	CSP_ALERT_QUEUE_EOF 	 0
# define	CSP_ALERT_QUEUE_SIZE	(128 * 640) /* FIX-ME */

    i4		csp_alert_pending;	/* Set if CSP alert action already
					   pending.  Surpresses redundant
					   CSP_ACT_6_ALERT actions.
					*/
    PTR		csp_alert_qbuf_start;	/* Start of memory for alerts */
    PTR		csp_alert_qbuf_end;	/* Used for wrap check. */
    PTR		csp_alert_qbuf_write;	/* Message thread copies alerts here */
    PTR		csp_alert_qbuf_read;	/* CSP thread picks up from here. */
    /*
    ** Notes on the usage of the action queue structures.
    **
    ** Maintaining the action queue is complicated by the need to
    ** allow both AST/signal routines & multiple user mode threads
    ** to enqueue and dequeue action structures.   AST/signal code
    ** cannot use a mutex to get exclusive access, since this may
    ** deadlock if the mutex is held by a thread.   Therefore we
    ** use compare and swap primatives to manipulate singlely linked
    ** lists (SLLs) of "free" and "posted" action structures.
    ** Main CSP thread then puts them on a queue which is maintained
    ** as a doublely linked list private to its own usage.
    */
    ACTIONQ	csp_action_queue;	/* Queue of actions waiting for
					   processing by CSP thread.  Only
					   manipulated by CSP thread, so
					   no protection needed. */
    ACTION *	csp_posted_actions;	/* SLL of posted actions, in
					   reverse order to how they
					   should be processed.  Maintained
					   using compare & swap logic */
    ACTION *	csp_free_actions;	/* SLL of free action blocks.
					   Maintained using compare &
					   swap logic */
# define NUM_ACT_BUFS	(8*CX_MAX_NODES)
    ACTION	csp_action_buffers[NUM_ACT_BUFS]; /* Far more buffers
					   than should ever be needed. */
    CSP_NODE_INFO csp_node_info[CX_MAX_NODES]; /* Per node CSP info */
    i1		csp_node_class_roster;	/* has a valid node class roster? */
}	CSP;


static	CSP			csp_global ZERO_FILL;	/* CSP data area. */

static	CX_CONFIGURATION	*csp_config;	/* Configuration file info. */

/*
** Following changed to a GLOBALREF from a GLOBALDEF, and switch moved to
** LKINIT.C in Locking system, so that LK routines can use the same debug
** switch for tracing CSP locking. (boama01, 08/21/96)
*/
GLOBALREF  i4	csp_debug;
GLOBALDEF  i4	csp_pending_msg;
static	   char csp_msg_type_names[] =
		  "0,JOIN,LEAVE,NODE_RCVRD,NEW_MASTER,RPTIVB,ALERT,CKP,SYNC,CLSTART,CLSTOP,DBSTART,DBMS_RCVRD";



/*
**  Definition of static variables and forward static functions.
*/


/*-------------------------CSP-routines----------------------------------*/
static	VOID	csp_deadman_notify( CX_REQ_CB *preq, i4 status );
static	i4	csp_find_master_node( VOID );
static	STATUS	csp_announce_new_master( i4 node );
/*----------------------------- cmr support ------------------------------*/
static	i4	csp_cmr_node_test(CSP_NODE_INFO *node_info,
                                     CX_CMO_CMR *pcmocmr, i4 node);
static	STATUS	csp_cmr_roster_update( i4 node, CX_CMO_CMR *cmr );
static	STATUS	csp_cmr_roster_update_cb( CX_CMO *oldval, CX_CMO *pnewval,
			PTR pnode, bool invalidin );

static	STATUS	    csp_restart(bool *local_log_recovered,
			i4 *errcode);	/* Handle crash restart. */
static	STATUS	    csp_node_fail(i4 *errcode); /* Handle node failure rcvry.*/
static	VOID	    csp_shutdown(VOID);		/* Handle shutdown. */
static	DB_STATUS   csp_archive(DMP_LCTX *lctx, i4 *err_code);
static	STATUS	csp_grab_recovery_lock(VOID);
static	STATUS	csp_release_recovery_lock(VOID);
static  VOID	csp_alert_overflow(VOID);

/*--------------------------Action-functions------------------------------*/

/* Take an action from "free" SLL, add info, and put on "posted" SLL. */
static	VOID	csp_put_action( i4 act, i4 node, i4 sync, CSP_CX_MSG *msg );
/* Take a singlely linked list (SLL) of actions and enqueue them */
static  VOID	csp_enqueue_actions( ACTION *action );

/*-----------------cluster online checkpoint functions---------------------*/


static VOID		csp_ckp_msg(CSP_CX_MSG *ckp_msg);
static DB_STATUS	csp_ckp_msg_init(CSP_CX_MSG *ckp_msg);
static DB_STATUS	csp_ckp_action(SCF_FTX *ftx);
static DB_STATUS	csp_ckp_init_jsx(CSP_CX_MSG *ckp_msg, DMF_JSX *jsx);
static VOID		csp_ckp_error(CSP_CX_MSG *ckp_msg);
static VOID		csp_ckp_status(CSP_CX_MSG *ckp_msg);
static VOID		csp_ckp_lkrequest(CSP_CX_MSG *ckp_msg);
static DB_STATUS	csp_ckp_lkrelease(CSP_CX_MSG *ckp_msg, i4 pnum);
static DB_STATUS	csp_ckp_lkconvert(CSP_CX_MSG *ckp_msg,
					    LK_LKID *lockid,
					    i4 high_value, i4 low_value);

static DB_STATUS	csp_ckp_abort(CSP_CX_MSG *ckp_msg);
static DB_STATUS	csp_ckp_closedb(CSP_CX_MSG *ckp_msg, i4 log_id);
static DB_STATUS	csp_ckp_opendb(CSP_CX_MSG *ckp_msg,
			    DB_DB_NAME *dbname, DB_OWN_NAME *dbowner,
			    DMP_LOC_ENTRY *dblocation, i4 dcb_status, i4 *log_id);
static DB_STATUS	csp_ckp_logid(CSP_CX_MSG *ckp_msg, i4 *log_id);
static DB_STATUS	csp_ckp_lkrelease_all(CSP_CX_MSG *ckp_msg);

/*----------------- per node class roster maintenance --------------------*/
static CSP_NODE_CLASS_INFO_QUEUE *csp_class_roster_class_find(
    CSP_NODE_CLASS_ROSTER *r,
    CSP_CX_MSG *c,
    char *class_name);
static CSP_NODE_CLASS_INFO_QUEUE *csp_class_roster_class_create(i4 *err_code);
static i4 csp_class_roster_class_destroy(CSP_NODE_CLASS_INFO_QUEUE *c);
static i4 csp_class_roster_class_insert(CSP_NODE_CLASS_ROSTER *r,
					CSP_NODE_CLASS_INFO_QUEUE *c);
static void csp_class_roster_classes_destroy(CSP_NODE_CLASS_ROSTER *r);
static CSP_NODE_CLASS_EVENT_QUEUE* csp_class_event_create(CSP_CX_MSG *v,
							  i4 *err_code,
							  char *class_name);
static void csp_class_event_destroy(CSP_NODE_CLASS_EVENT_QUEUE *event);
static i4 csp_class_roster_event_add(CSP_NODE_CLASS_ROSTER *r, CSP_CX_MSG *v,
				     char *class_name);
static i4 csp_class_roster_event_apply(CSP_NODE_CLASS_ROSTER *r,
				       CSP_CX_MSG *e,
				       char *class_name);
static i4 csp_class_roster_queue_destroy(CSP_NODE_CLASS_ROSTER *r);
static i4 csp_class_roster_queue_apply(CSP_NODE_CLASS_ROSTER *r);
static i4 csp_class_roster_queue(CSP_CX_MSG *e, char *class_name);
static i4 csp_class_roster_build(CSP_NODE_CLASS_ROSTER *roster, u_i4 roster_id);
static i4 csp_class_roster_destroy_node(CSP_NODE_CLASS_ROSTER *r, i4 node);
static i4 csp_class_roster_destroy(CSP_NODE_CLASS_ROSTER *roster);
static i4 csp_class_roster_failed_node(CSP_NODE_CLASS_ROSTER *r, i4 node);
static i4 csp_class_roster_copy(CSP_NODE_CLASS_ROSTER *roster_dst,
				CSP_NODE_CLASS_ROSTER *roster_src);
static i4 csp_class_roster_compare(CSP_NODE_CLASS_ROSTER *roster_a,
				   CSP_NODE_CLASS_ROSTER *roster_b);
static u_i4  csp_class_roster_nextid( void );
static i4 csp_class_roster_marshall(u_i1 node, PTR buffer);
static i4 csp_class_roster_unmarshall(CSP_NODE_CLASS_ROSTER *roster,
				      i4 count, PTR buffer);
static void csp_class_roster_print(CSP_NODE_CLASS_ROSTER *r);
static STATUS csp_dbms_start(char *class_name, PID *pid);
static STATUS csp_dbms_remote_start(char *class_name, i4 node);

/*----------------- DBMS deadman lock routines ---------------------------*/
static VOID csp_dead_dbms_notify( CX_REQ_CB *preq, i4 status );
static VOID csp_req_dbms_dmn(i1 node, PID pid, i2 lg_id_id);
static VOID csp_rls_dbms_dmn(i1 node, PID pid, i2 lg_id_id);
static VOID csp_rcvr_cmpl_msg(i1 node, PID pid, i2 lg_id_id);

/*--------------------------Miscellaneous---------------------------------*/
static STATUS try_lk_resume();

/* Send to CSP operator. */
static i4 opc_send(
    PTR		arg,
    i4		length,
    char	*buffer);

/* Terminate on fatal error. */
static VOID crash(
    i4	    message,
    CL_ERR_DESC	*status);

static	VOID	    dumpall(VOID);		/* Print all data structures. */

static	VOID	    csp_handler(VOID);	/* CSP exit handler. */

/* Exception handler and flag for system exceptions */
static STATUS csp_ex_handler(
    EX_ARGS	*ex_args);
static i4  exception_noted = 0;

/* Number of calls to queue_deadlock() since we last called gbl_deadlock(). */
static i4  csp_dlck_queued = 0;

/* Event flag and delay to use when scheduling a gbl_deadlock() call. */
static i4 csp_dlck_ef = 0;
static i4 csp_dlck_interval [2];


/*{
** Name: dmfcsp	- Cluster Server Process.
**
** Description:
**      This is the main routine of the CLuster Server "Process".
**	CSP now runs as a VITAL thread within the RCP.
**
** Inputs:
**      dmc_cb
**       .type                  Must be set to DMC_CONTROL_CB
**       .length                Must be at least sizeof(DMC_CB)
**
** Outputs:
**      dmc_cb
**       .error.err_code        Error code, if any.
**
**	Returns:
**	    OK
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-jun-1987 (Derek)
**          Created for Jupiter.
**	18-may-2001 (devjo01)
**	    Rewritten as part of s103715.
**	14-may-2004 (devjo01)
**	    Close CLM connection to foreign node if it crashes, or
**	    leaves.  Move CSP_MSG_2_LEAVE processing to main
**	    CSP thread.
**	19-may-2004 (devjo01)
**	    Format deadman lock key into 'deadman_lock_key';
**	08-jan-2005 (devjo01)
**	    Pass CX_F_NO_DOMAIN to CSP lock call to indicate this lock
**	    is not part of any recovery domain.
**	30-Mar-2005 (jenjo02)
**	    csp_initialize() call moved to dmr_log_init() to perform
**	    orchestrated cluster recovery on local log and all other
**	    nodes.
**	22-Apr-2005 (fanch01)
**	    Added dbms class failover.  dbms class roster initialization
**	    and additional action message handlers for class up, down
**	    events and dbms class execution.
**	10-Nov-2005 (devjo01) b115521
**	    Correct race condition where we could crash with a E_DM9901
**	    in the perfectly normal case of processing a LEAVE message
**	    after the departing node has released its instance of the
**	    deadmans lock.  This allows the local deadman lock instance
**	    to be granted, and CXdlm_cancel to "fail" with a Can't cancel
**	    code.
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**	17-Jan-2008 (jonj)
**	    The main loop's CSsuspend() must be interruptable.
**	    When this thread terminates, call CXmsg_shutdown()
**	    to properly terminate the messaging thread.
**	27-Aug-2008 (jonj)
**	    Terminate the big "while" loop when this CSP has been shutdown
**	    rather waiting on another CSsuspend. The interrupt signaled by
**	    CSterminate may have already been eaten.
*/
DB_STATUS
dmfcsp(DMC_CB *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    CSP			*csp = &csp_global;
    DB_STATUS		 final_status = E_DB_ERROR;
    STATUS               status;
    i4			 err_code;
    CL_ERR_DESC		 sys_err;
    ACTION		*action;
    ACTIONQ		 local_queue;
    EX_CONTEXT		 context;	/* (Exception handler context) */
    CX_CMO_CMR		 cmo_cmr;	/* Cluster member CMO */
    CSP_CX_MSG		 csp_msg;
    CSP_NODE_INFO	*node_info;
    CX_REQ_CB		*preq;
    i4			 node;
    i4			 ivb_seen_pending = 0;
    char		 lclass_name[CSP_MSG_CLASS_BYTES];
    i4			 lmbuf_len;
    CSP_NODE_CLASS_ROSTER *class_roster;

    CLRDBERR(&dmc->error);

    /*
    ** Declare exception handler.
    ** When an exception occurs, execution will continue here.  The
    ** exception handler will have logged an error message giving the
    ** exception type and hopefully useful information indicating where
    ** it occurred.  Here we must simply crash the CSP.
    */
    if (EXdeclare(csp_ex_handler, &context) == EXDECLARE)
    {
	EXdelete();
	crash(E_DM9930_CSP_EXCEPTION_ABORT, &sys_err);
    }

    {
	char	*envp;

	/*
	** May also be turned on at run-time with TRACE POINT DM303.
	*/
	NMgtAt(ERx("II_CSP_DEBUG"), &envp);
	if ( envp )
	    CVal(envp, &csp_debug );
    }

    if ( csp_debug )
    {
	_VOID_ uleFormat(NULL, E_DM9913_CSP_DEBUG_ON, (CL_ERR_DESC *)NULL, 
	  ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	_VOID_ TRdisplay(ERx("csp_debug %@: CSP thread started.\n"));
    }

    for ( ; /* Something to break out of */ ; )
    {

	/* By now, csp_initialize has been run, the logs recovered */

	if ( CXconfig_settings( CX_HAS_1ST_CSP_ROLE ) )
	{
	    /*
	    ** Current node establishes the cluster.  Class roster
	    ** is validated.
	    **
	    ** Also used to determine in the CMR loop if we should
	    ** claim master role.
	    **
	    */
	    csp->csp_node_class_roster = 1;
	}

	/*
	** No need to wake CSP if this is clear.  Prevents any possibility
	** of message thread resuming CSP thread while CSP thread is
	** suspended on lock completion within CXjoin.
	*/
	csp->csp_ready_to_sleep = 0;

	/*
	** Set session id for this thread.  Message threads will
	** not proceed until this is set.
	*/
	CSget_sid( &csp->csp_sid );

	/*
	** Wait until messaging sub-system is ready. 
	**
	** We need this to be up, before we get our DMN
	** locks, in order to be ready to modify our
	** set of DMN locks as other CSP's come and go.
	*/
	while ( !CXmsg_channel_ready( CSP_CHANNEL ) )
	{
	    /* Take a brief snooze */
	    status = CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
	}

	/*
	** Add self into CMR and take inventory of nodes that are up.
	**
	** CMR ('C'luster 'M'embership 'R'oster) is initialized by
	** first CSP or recovered by any node if the value is invalidated
	** due to node failure.
	*/
	err_code = csp_cmr_roster_update( csp->csp_self_node, &cmo_cmr );
	if ( err_code )
	{
	    break;
	}

	/*
	** Take out deadman locks and determine master node.
	**
	** Deadman locks will be taken out against nodes which appear
	** in the roster that have not been marked as "node_active".
	**
	** The master node is simply the lowest node active in the CMR.
	**
	*/
	csp->csp_master_node = 0;
	for ( node = 1; node <= CX_MAX_NODES; node++ )
	{
	    /* node configured? */
	    if ( csp_config->cx_nodes[node-1].cx_node_number )
	    {
		/* node up in roster? */
		if ( CX_GET_NODE_BIT( cmo_cmr.cx_cmr_members, node ) )
		{
		    CX_CMO_CMR lcmo_cmr;
		    node_info = &csp->csp_node_info[node-1];

		    /* marked as up, get deadman and test */
		    err_code = csp_cmr_node_test( node_info, &lcmo_cmr, node );
		    if ( err_code )
			break;

		    /* failed to get DMN lock? */
		    if ( !CX_GET_NODE_BIT ( lcmo_cmr.cx_cmr_members, node) )
		    {
			/* re-read the CMR to see if node left cluster */
			err_code = csp_cmr_roster_update( 0, &lcmo_cmr );
			if ( err_code )
			    break;

			if ( CX_GET_NODE_BIT( lcmo_cmr.cx_cmr_members, node) )
			{
			    node_info->node_failure = 1;
			    csp->csp_node_failure = 1;
			    /* node failed active test and in CMR - failed */
			    if ( csp_debug )
			    {
				_VOID_ TRdisplay(
				    ERx("csp_debug %@: Failed node (%d) in CMR\n"),
					node);
			    }
			}
		    }
		    else
		    {
			/* set the master if first encountered and not us */
			if (!csp->csp_node_class_roster &&
                            csp->csp_master_node == 0 &&
                            node != csp->csp_self_node)
			{
			    csp->csp_master_node = node;
			    if ( csp_debug )
			    {
				_VOID_ TRdisplay(
				    ERx("csp_debug %@: Potential master (%d) located in CMR\n"),
					csp->csp_master_node);
			    }
			}
		    }
		}
	    }
	}
	if ( err_code )
	{
	    break;
	}

	if ( csp_debug )
	{
#if 1 /* FIXME - SEGV sometimes seen with %#.4{ %x%} format */
	    _VOID_ TRdisplay(
             ERx("csp_debug %@: New CMR = %#.4{ %x%} invalid=%d\n"),
	     sizeof(cmo_cmr.cx_cmr_members)/4, &cmo_cmr.cx_cmr_members);
#else
	    _VOID_ TRdisplay(
             ERx("csp_debug %@: New CMR = %x\n"),
	     cmo_cmr.cx_cmr_members);
#endif
	}

	/*
	** Broadcast that node is ready to join cluster.
	*/
	csp_msg.csp_msg_node = (u_i1)csp->csp_self_node;
	csp_msg.csp_msg_action = CSP_MSG_1_JOIN;
	csp_msg.join.roster_id = csp_class_roster_nextid();

        csp->csp_node_info[csp->csp_self_node-1].class_roster.class_roster_id
	    = csp_msg.join.roster_id;
	err_code = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&csp_msg );

	if ( err_code )
	{
	    break;
	}

	if ( csp_debug )
	{
	    _VOID_ TRdisplay(ERx("csp_debug %@: Joining cluster as node %d.\n"),
		       csp->csp_self_node);
	}

	/*
	** If CMR scan failed to find a node other than the local node up
	** then local node is the master.
	*/
	if (csp->csp_master_node == 0)
	{
	    csp->csp_master_node = csp->csp_self_node;
	    if (csp->csp_node_class_roster)
	    {
		/*
		** First CSP, expected to take master role.
		*/
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
			ERx("csp_debug %@: First CSP, local node (%d) now master\n"),
			    csp->csp_master_node);
		}
	    }
	    else
	    {
		/*
		** Can only get here if all other CSP's were found
		** to have failed in the interval after we failed
		** to become 1st CSP.   This should be an extremely
		** rare event.  However to cover all gaps, we assume
		** the master role, and broadcast this, just to 
		** cover the theoretical case in which a 2nd CSP was
		** also starting in this interval.
		*/
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
			ERx("csp_debug %@: Master failure, local node (%d) now master\n"),
			    csp->csp_master_node);
		}
		err_code = csp_announce_new_master( csp->csp_self_node );
		if ( err_code )
		{
		    break;
		}
	    }
	}

	/*
	** OK everything is ready.  Finish enrollment into cluster.
	** This will release cluster config lock if we are the 1st
	** CSP, so other CSP's may now come on-line.
	*/
	err_code = CXjoin(CX_F_IS_SERVER);
	if ( err_code )
	    break;

	status = OK;
	break;
    }  /* end-for */

    if ( OK == status )
    {
	if ( csp_debug )
	{
	    _VOID_ TRdisplay(ERx("csp_debug %@: Have joined cluster.\n"));
	}

	csp->csp_state = CSP_ST_2_JOINED;

	if ( csp->csp_master_node == csp->csp_self_node &&
	     csp->csp_node_failure )
	{
	    /*
	    ** There is a pending node failure.
	    */
	    status = csp_node_fail(&err_code);
	    if ( status && err_code )
	    {
		_VOID_ uleFormat(NULL, err_code, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    }
	}
    }

    /*
    **	This is the main-loop of the CSP.  It is responsible for handling
    **	extraordinary events.  Normally the CSP thread sits idle.
    */
	
    /* When we've shutdown, just quit, don't maybe hang on another CSsuspend */
    while ( OK == status && csp->csp_state != CSP_ST_3_TERMED )
    {
	/*
	** Add any new "actions" posted to the end of the queue.
	** We only do this when we need to replenish the main
	** queue, so as to marginally reduce contention.
	*/
	if ( ACT_QUEUE_EMPTY(&csp->csp_action_queue) && 
	     csp->csp_posted_actions )
	{
	    /* Grab current list, leaving an empty list. */
	    do
	    {
		action = csp->csp_posted_actions;
	    } while ( CScasptr((PTR*)&csp->csp_posted_actions,
		    (PTR)action, NULL ) ); /* Do nothing */

	    /* Add action(s) to action queue in correct (reversed) order. */
	    csp_enqueue_actions( action );
	}
		
	if ( ACT_QUEUE_EMPTY(&csp->csp_action_queue) )
	{
	    /*
	    ** No action was found, if this is the second pass
	    ** through the queues, suspend this thread until
	    ** something happens.
	    */
	    if ( csp->csp_ready_to_sleep )
	    {
		if ( ivb_seen_pending )
		    status = CSsuspend( CS_INTERRUPT_MASK | CS_TIMEOUT_MASK, 1, 0 );
		else
		    status = CSsuspend( CS_INTERRUPT_MASK, 0, 0 );
		if ( E_CS0009_TIMEOUT == status )
		{
		    status = OK;
		    if ( !csp->csp_node_failure )
		    {
			/*
			** Revalidate locks.
			*/
			for ( ; /* something to break out of */ ; )
			{
			    err_code = CXstartrecovery(0);
			    if ( err_code )
				break;
			    err_code = CXfinishrecovery(0);
			    if ( err_code )
				break;
			    csp_msg.csp_msg_action = CSP_MSG_3_RCVRY_CMPL;
			    csp_msg.csp_msg_node = (u_i1)csp->csp_self_node;
			    CX_INIT_NODE_BITS(csp_msg.rcvry.node_bits);
			    err_code = CXmsg_send( CSP_CHANNEL, NULL,
			                         NULL, (PTR)&csp_msg );
			    break;
			}
			if ( err_code )
			{
			    status = E_DB_ERROR;
			    break;
			}
		    }
		    ivb_seen_pending = 0;
		}
		if ( status != OK )
		{
		    /* CSP shutdown has arrived */
		    final_status = OK;
		    break;
		}
		csp->csp_ready_to_sleep = 0;
	    }
	    else
	    {
		csp->csp_ready_to_sleep = 1;
	    }
	    continue;
	}

	/* Process received action */
	csp->csp_ready_to_sleep = 0;

	action = csp->csp_action_queue.next;
	ACT_DEQUEUE(action);

	if ( CSP_ST_2_JOINED == csp->csp_state )
	{
	    /*  Execute the action. */
	    switch (action->act_type)
	    {
	    case CSP_ACT_1_NEW_MASTER:
		/*
		** Another CSP with a lower node number has come on-line.
		** pass the baton.
		*/
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_1_NEW_MASTER node %d.\n"),
		      action->act_node );
		}
		err_code = status = csp_announce_new_master(action->act_node);
		break;

	    case CSP_ACT_2_NODE_FAILURE:
		/*
		** Received report of another node (CSP) failing.
		*/
		node = action->act_node;
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_2_NODE_FAILURE node %d.\n"),
		      node );
		}
		class_roster = &csp->csp_node_info[node-1].class_roster;

		/*
		** Disconnect from nodes CLM file.
		*/
		(void)CXmsg_clmclose( node );

		csp_class_roster_failed_node( class_roster, node );

		/*
		** If failed node was the MASTER CSP, then this
		** CSP needs to decide if it is the new MASTER CSP.
		*/
		if ( node == csp->csp_master_node )
		{
		    /*
		    ** If class roster was building that the now
		    ** defunct master was to respond to, all nodes
		    ** all nodes jump in and publish their list.
		    **
		    ** This may seem wasteful (it is) but it's not
		    ** guaranteed that the new master have a current
		    ** class roster.
		    **
		    ** Therefore determining who should respond to
		    ** the roster request is a bit problematic.  For
		    ** simplicity, all nodes with a current roster
		    ** respond to the request.  This causes no harm.
		    */
		    for ( node = 1;
			  node <= csp_config->cx_node_cnt; 
			  node++ )
		    {
			/*
			** need to append actions for each node building
			** a roster
			*/
			CX_NODE_INFO *cx_node_info =
			    &csp_config->cx_nodes[csp_config->cx_xref[node]];
			node_info =
			    &csp->csp_node_info[cx_node_info->cx_node_number-1];
			class_roster = &node_info->class_roster;

			/*
			** if we're building and the failed node was the one
			** designated to respond
			*/
			if ( class_roster->class_roster_build &&
			     class_roster->class_roster_master ==
			     action->act_node )
			{
			    /* allocate a buffer for message marshall */
			    i4 dbms_sync_len = 
		     sizeof(CSP_DBMS_SYNC)*CX_MAX_NODES*CSP_MAX_DBMS_MARSHALL;
			    DMP_MISC *lmbuf;

			    lmbuf_len = (class_roster->class_count *
			    sizeof(CSP_NODE_CLASS_LMSG)) + dbms_sync_len;

			    status = dm0m_allocate( 
					lmbuf_len + sizeof(DMP_MISC),
					0, MISC_CB, MISC_ASCII_ID,
					0, (DM_OBJECT**)&lmbuf, &dmc->error );
			    if ( status != E_DB_OK )
				break;

			    lmbuf->misc_data = (char*) (&lmbuf[1]);
			    csp_class_roster_marshall(node, (PTR) &lmbuf[1]);
			    status = CXmsg_stow(
				&csp_msg.class_sync.chit,
				(PTR) &lmbuf[1],
				lmbuf_len );
			    if ( status != OK )
				break;

			    dm0m_deallocate( (DM_OBJECT**)&lmbuf );

			    csp_msg.class_sync.roster_id =
				class_roster->class_roster_id;
			    csp_msg.class_sync.count =
				class_roster->class_count;
			    csp_msg.csp_msg_node = (u_i1)node;
			    csp_msg.csp_msg_action =
				CSP_MSG_8_CLASS_SYNC_REPLY;

			    status = CXmsg_send( CSP_CHANNEL, NULL, NULL,
						 (PTR)&csp_msg );
			    if ( status != OK )
				break;

			    _VOID_ CXmsg_release( &csp_msg.class_sync.chit );
			}
		    }
		    if ( status != E_DB_OK )
			break;

		    node = csp_find_master_node();
		    if ( csp->csp_self_node == node ) 
		    {
			if ( status = csp_announce_new_master(node) )
			{
			    /*
			    ** Failure to claim master role if qualified
			    ** is a fatal error, since other candidates
			    ** will expect you to succeed, and won't
			    ** take the role if you can't.   This
			    ** should never happen, but if it does,
			    ** we have no choice but to crash.  Other
			    ** candidates will presumably see this
			    ** CSP abruptly leave the cluster, and
			    ** a new master candidate will step
			    ** forward from among the survivors.
			    */
			    if ( csp_debug )
			    {
				_VOID_ TRdisplay(
			  ERx("csp_debug %@: Fail to claim master role\n") );
			    }
			    break;
			}
		    }
		}

		/*
		** If this CSP is currently the master node,
		** handle failure of another node. 
		*/
		if ( csp->csp_self_node == csp->csp_master_node )
		{
		    status = csp_node_fail(&err_code);
		    if ( status && err_code )
		    {
			_VOID_ uleFormat(NULL, err_code, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		    }
		}

		/* 
		** Node failure explains the IVB seen.  Wait for
		** regular recovery complete to address this.
		*/
		ivb_seen_pending = 0;
		break;

	    case CSP_ACT_3_RCVRY_CMPL:
		/* 
		** Master CSP has recovered the last disaster, CSP
		** can now release DMN locks held for recovered nodes, 
		** and unstall lock queues if all failed nodes have 
		** been recovered.
		*/
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		     ERx("csp_debug %@: CSP_ACT_3_RCVRY_CMPL\n"));
		}
		csp->csp_node_failure = 0;
		for ( node = 1; node <= CX_MAX_NODES; node++ )
		{
		    node_info = &csp->csp_node_info[node-1];
		    if ( node_info->node_failure )
		    {
			/* 
			** There is at least one node not yet recovered,
			** can't unstall yet.
			*/
			csp->csp_node_failure = 1;
			break;
		    }
		}

		/* Try to resume normal lock processing */
		status = try_lk_resume();
		break;

	    case CSP_ACT_4_SHUTDOWN:
		/*
		** CSP is being shutdown, exit cluster.
		*/ 
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_4_SHUTDOWN.\n") );
		}
		csp->csp_state = CSP_ST_3_TERMED;
		node_info = &csp->csp_node_info[csp->csp_self_node-1];
		CSp_semaphore(TRUE, &node_info->node_active_sem);
		node_info->node_active = 0;
		CSv_semaphore(&node_info->node_active_sem);

		if ( csp->csp_self_node == csp->csp_master_node )
		{
		    CXalter( CX_A_MASTER_CSP_ROLE, (PTR)0 );
		    node = csp_find_master_node();
		    if ( node ) 
		    {
			(VOID)csp_announce_new_master(node);
		    }
		}

		(void)csp_cmr_roster_update( -csp->csp_self_node, NULL );
		csp_msg.csp_msg_node = (u_i1)csp->csp_self_node;
		csp_msg.csp_msg_action = CSP_MSG_2_LEAVE;
		err_code = status = final_status =
		 CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&csp_msg );
		break;

	    case CSP_ACT_5_IVB_SEEN:
		/*
		** A process received a lock value block marked as
		** invalid.  Normally that will only occur as a side
		** effect of a node failure.  However, not all DLM's
		** will guarantee that the high priority DMN locks will
		** be granted first, so a normal lock request may be the
		** first to see a failure.   Worst still, not every DLM
		** will emit an invalid lock block ONLY in the node
		** failure case, so we need to handle an IVB outside a
		** failure context.   Strategy is to have master wait
		** a second.  That gives plenty of time for a DMN lock
		** to be granted.  Afterwards if no failure message is
		** received, master CSP will simply revalidate the
		** locks and send a recovery message with an empty
		** recovery set to unstall any locks stalled by an IVB.
		*/
		if ( !csp->csp_node_failure &&
		     csp->csp_self_node == csp->csp_master_node )
		{
		    ivb_seen_pending = 1;
		}
		break;
	    
	    case CSP_ACT_6_ALERT:
		/*
		** One or more alerts have been queued for propagation.
		*/
		{
		    PTR		 nextreadptr;
		    SCF_CB	 scf_cb;

		    csp->csp_alert_pending = 0;

		    nextreadptr = csp->csp_alert_qbuf_read;
		    while ( CSP_ALERT_QUEUE_EOF != *(i4 *)nextreadptr )
		    {
			if ( CSP_ALERT_QUEUE_WRAP == *(i4 *)nextreadptr )
			{
			    nextreadptr = csp->csp_alert_qbuf_start;
			    continue;
			}
			
			/* Process alert message */
			scf_cb.scf_length = sizeof(SCF_CB);
			scf_cb.scf_type = SCF_CB_TYPE;
			scf_cb.scf_facility = DB_DMF_ID;
			scf_cb.scf_session = DB_NOSESSION;
			scf_cb.scf_ptr_union.scf_buffer =
			 nextreadptr + sizeof(i4);

			status = scf_call( SCE_RESIGNAL, &scf_cb );
			if ( status )
			{
			    /*
			    ** Resignaling may legitimately fail
			    ** if not servers are up, or configured
			    ** to receive alerts (a very bad idea
			    ** in a cluster as RDF cache coherency
			    ** depends on this.
			    */
			    status = OK;
			    if ( E_SC0280_NO_ALERT_INIT !=
				 scf_cb.scf_error.err_code )
			    {
				REPORT_ODDITY( \
				 ERx("Unexpected resignal error"));
				(void)TRdisplay( \
				 ERx("scf_cb.scf_error.err_code = %d\n"),
				 scf_cb.scf_error.err_code );
			    }
			}

			/* Bump to next */
			nextreadptr = nextreadptr + sizeof(i4) +
			  *(i4 *)nextreadptr;
			csp->csp_alert_qbuf_read = nextreadptr = 
			 ME_ALIGN_MACRO(nextreadptr, sizeof(i4));
		    }
		}
		break;

	    case CSP_ACT_7_LEAVE:
		/*
		** Foreign CSP has broadcast its irrevokable intent to leave.
		*/
		node = action->act_node;
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_7_LEAVE node %d.\n"),
		      node );
		}
		class_roster =
		    &csp->csp_node_info[csp->csp_self_node-1].class_roster;

		/*
		** Disconnect from nodes CLM file.
		*/
		(void)CXmsg_clmclose( node );

		/*
		** Remove class roster entries for this node.
		**
		** There should be none!
		*/
                csp_class_roster_destroy_node(class_roster, node);

		/*
		** Discard deadman lock on remote CSP.
		*/
		node_info = &csp->csp_node_info[node-1];
		preq = &node_info->deadman_lock_req;
		if ( CX_NONZERO_ID( &preq->cx_lock_id ) )
		{
		    status = CXdlm_cancel( 0, preq );
		    if ( E_CL2C09_CX_W_CANT_CAN == status )
		    {
			/*
			** Expected race condition.  Remote node has
			** already dropped it's lock, allowing lock
			** to be granted.  In this case free it.
			*/
			status = CXdlm_release( 0, preq );
		    }
		    CX_ZERO_OUT_ID(&preq->cx_lock_id);
		}
		CSp_semaphore(TRUE, &node_info->node_active_sem);
		node_info->node_active = 0;
		CSv_semaphore(&node_info->node_active_sem);

		/*
		** Master master node should have already announced new
		** master node, so there should be no need to check
		** if the node leaving is the master, however ...
		*/
		if ( node == csp->csp_master_node )
		{
		    REPORT_ODDITY( \
		     ERx("Master node left without passing baton"));
		    if ( csp->csp_self_node == csp_find_master_node() )
		    {
			csp_put_action(CSP_ACT_1_NEW_MASTER,
				       csp->csp_self_node, 0, (CSP_CX_MSG *)0); 
		    }
		}
		break;

	    case CSP_ACT_8_CKP:
		{
		    CSP			*csp = &csp_global;
		    CSP_CX_MSG          ckp_msg;
		    DB_STATUS		status;
		    SCF_CB		scf_cb;
		    SCF_FTC		ftc;
		    i4			err_code;

		    STRUCT_ASSIGN_MACRO(action->act_msg, ckp_msg);

		    if ( csp_debug )
		    {
			_VOID_ TRdisplay(
			  ERx("csp_debug %@: CSP_ACT_8_CKP.\n") );
		    }

		    /* create factotum thread to do online checkpoint action */
		    /* For now assume there is no limit for such threads  */
		    scf_cb.scf_type = SCF_CB_TYPE;
		    scf_cb.scf_length = sizeof(SCF_CB);
		    scf_cb.scf_session = DB_NOSESSION;
		    scf_cb.scf_facility = DB_DMF_ID;
		    scf_cb.scf_ptr_union.scf_ftc = &ftc;

		    ftc.ftc_facilities = 0;
		    ftc.ftc_priority = SCF_CURPRIORITY;
		    ftc.ftc_data = (PTR) &ckp_msg;
		    ftc.ftc_data_length = sizeof(ckp_msg);
		    ftc.ftc_thread_name = "<csp_ckp_action>";
		    ftc.ftc_thread_entry = csp_ckp_action;
		    ftc.ftc_thread_exit =  NULL;

		    status = scf_call(SCS_ATFACTOT, &scf_cb);
		    if (status != E_DB_OK)
		    {
			uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char * )0, (i4)0, (i4 *)NULL, 
			    &err_code, 0);

			/* Remote nodes must close database */
			if (csp->csp_self_node != ckp_msg.ckp.ckp_node)
			{
			    status = csp_ckp_abort(&ckp_msg);
			}

			/* Message other nodes about failure */
			csp_ckp_error(&action->act_msg);
		    }

		    break;
		}

	    case CSP_ACT_9_CLASS_SYNC_REQUEST:
		node = action->act_msg.csp_msg_node;
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		  ERx("csp_debug %@: CSP_ACT_9_CLASS_SYNC_REQUEST node %d.\n"),
		      node );
		}
		class_roster = &csp->csp_node_info[node-1].class_roster;
		/*
		** As long as this isn't the joining node we need to package
		** up the node class roster for delivery.  All nodes with
		** a valid roster need to do this in case the master fails
		** to deliver the roster.
		**
		** The master will put the roster in a long message and
		** deliver it.
		*/
		csp_class_roster_destroy(class_roster);

		if ( csp->csp_self_node == node )
		{
		    /*
		    ** Set local class roster to build.
		    */
		    csp_class_roster_build(class_roster,
			   action->act_msg.join.roster_id);
		}
		else if ( csp->csp_self_node == csp->csp_master_node )
		{
		    /*
		    ** Master node, deliver class roster.
		    */
		    CSP_NODE_CLASS_ROSTER *my_class_roster =
			&csp->csp_node_info[csp->csp_self_node-1].class_roster;

		    /* allocate a buffer for message marshall */
		    if (my_class_roster->class_count > 0)
		    {
			i4 dbms_sync_len = 
		    sizeof(CSP_DBMS_SYNC)*CX_MAX_NODES*CSP_MAX_DBMS_MARSHALL;
			DMP_MISC *lmbuf;

			lmbuf_len = (my_class_roster->class_count *
			    sizeof(CSP_NODE_CLASS_LMSG)) + dbms_sync_len;

			status = dm0m_allocate( lmbuf_len + sizeof(DMP_MISC),
						0, MISC_CB, MISC_ASCII_ID,
						0, (DM_OBJECT**)&lmbuf, &dmc->error );
			if ( status != E_DB_OK )
			    break;
			lmbuf->misc_data = (char*) (&lmbuf[1]);

			csp_class_roster_marshall(csp->csp_self_node,
						  (PTR) &lmbuf[1]);
			status = CXmsg_stow(
			    &csp_msg.class_sync.chit,
			    (PTR) &lmbuf[1], lmbuf_len );
			if ( status != OK )
			    break;

			dm0m_deallocate( (DM_OBJECT**)&lmbuf );
		    }

		    csp_msg.class_sync.roster_id =
			action->act_msg.join.roster_id;
		    csp_msg.class_sync.count =
			my_class_roster->class_count;
		    /* dest node */
		    csp_msg.class_sync.node = node;
		    /* from node */
		    csp_msg.csp_msg_node = csp->csp_self_node;
		    csp_msg.csp_msg_action = CSP_MSG_8_CLASS_SYNC_REPLY;
		    status = CXmsg_send( CSP_CHANNEL, NULL, NULL,
					 (PTR) &csp_msg );
		    if ( status != OK )
			break;

		    if (my_class_roster->class_count > 0)
		    {
			CXmsg_release( &csp_msg.class_sync.chit );
		    }
		}
		else if ( csp->csp_node_class_roster )
		{
		    /*
		    ** Joined node with valid class roster.
		    */
		    CSP_NODE_CLASS_ROSTER *my_class_roster =
			&csp->csp_node_info[csp->csp_self_node-1].class_roster;

		    /* build node class roster */
		    csp_class_roster_build(class_roster,
				   action->act_msg.join.roster_id);

		    /* copy local node's list */
		    status = csp_class_roster_copy(class_roster,
						   my_class_roster);
		    if (status != OK)
			break;
		}
		break;

	    case CSP_ACT_10_CLASS_SYNC_REPLY:
		node = action->act_msg.class_sync.node;
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		  ERx("csp_debug %@: CSP_ACT_10_CLASS_SYNC_REPLY node %d.\n"),
		      node );
		}
		class_roster = &csp->csp_node_info[node-1].class_roster;
		if (class_roster->class_roster_build)
		{
		    /* shared roster processing */

		    /* ensure class roster is empty */
		    csp_class_roster_classes_destroy(class_roster);

		    if (action->act_msg.class_sync.count > 0)
		    {
			i4 dbms_sync_len = 
		    sizeof(CSP_DBMS_SYNC)*CX_MAX_NODES*CSP_MAX_DBMS_MARSHALL;
			DMP_MISC *lmbuf;

			lmbuf_len = 
			    (action->act_msg.class_sync.count *
			    sizeof(CSP_NODE_CLASS_LMSG)) + dbms_sync_len;

			/* reply is for someone else and I kept track of it */
			status = dm0m_allocate( lmbuf_len + sizeof(DMP_MISC),
						0, MISC_CB, MISC_ASCII_ID,
						0, (DM_OBJECT**)&lmbuf, &dmc->error );
			if ( status != E_DB_OK )
			    break;

			lmbuf->misc_data = (char*) (&lmbuf[1]);

			/* redeem msg using chit */
			status = CXmsg_redeem(
			    action->act_msg.class_sync.chit,
			    (PTR) &lmbuf[1], lmbuf_len, 0, &lmbuf_len );

			/* should potentially try again */
			if (status != OK)
			    break;

			/* unmarshall class roster */
			status = csp_class_roster_unmarshall(class_roster,
				     action->act_msg.class_sync.count,
				     (PTR) &lmbuf[1]);
			if (status != OK)
			    break;

			dm0m_deallocate( (DM_OBJECT**)&lmbuf );
		    }

		    /* apply queued events to roster */
		    csp_class_roster_queue_apply(class_roster);

		    /* dest node or cross check logic */
		    if (class_roster->class_roster_id ==
		        action->act_msg.class_sync.roster_id)
		    {
			if ( csp->csp_self_node == node )
			{
			    /* for local node */
			    /* mark our roster as being valid */
			    csp->csp_node_class_roster = 1;
			    class_roster->class_roster_build = 0;
			}
			else
			{
			    /* for other node and local node did not send */
			    CSP_NODE_CLASS_ROSTER *my_class_roster =
				&csp->csp_node_info[csp->csp_self_node-1].class_roster;
			    if (csp_class_roster_compare(my_class_roster,
							 class_roster))
			    {
				_VOID_ TRdisplay(
				    ERx("class roster: consistency error\n"));
				break;
				/* fanch01 FIXME */
			    }
			    else if (csp_debug)
			    {
				_VOID_ TRdisplay(
				    ERx("Node class roster cross check success\n"));
				/* fanch01 FIXME */
			    }
			    csp_class_roster_destroy(class_roster);
			}
		    }
		}
		break;

	    case CSP_ACT_11_CLASS_START:
		/* process dbms class start action */
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_11_CLASS_START.\n") );
		}
		/* process dbms class start action */
		status = CXmsg_redeem(
		    action->act_msg.class_event.class_name,
		    (PTR) lclass_name, sizeof(lclass_name), 0, &lmbuf_len );

		if (status != OK)
		    break;

		CXmsg_release(&action->act_msg.class_event.class_name);

		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: Classname = %t.\n"),
		      STnlength(lmbuf_len,lclass_name), lclass_name );
		}

		/* process dbms class start action */
		if (csp->csp_node_class_roster)
		{
		    class_roster =
			&csp->csp_node_info[csp->csp_self_node-1].class_roster;
		    csp_class_roster_event_apply(class_roster,
						 &action->act_msg,
						 lclass_name);
		}

		/* queue event onto any pending rosters */
		status = csp_class_roster_queue(&action->act_msg, lclass_name);
		break;

	    case CSP_ACT_12_CLASS_STOP:
		/* process dbms class stop action */
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_12_CLASS_STOP.\n") );
		}
		/* process dbms class start action */
		status = CXmsg_redeem(
		    action->act_msg.class_event.class_name,
		    (PTR) lclass_name, sizeof(lclass_name), 0, &lmbuf_len );

		if (status != OK)
		    break;

		CXmsg_release(&action->act_msg.class_event.class_name);

		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: Classname = %t.\n"),
		      STnlength(lmbuf_len,lclass_name), lclass_name );
		}

		if (csp->csp_node_class_roster)
		{
		    class_roster =
			&csp->csp_node_info[csp->csp_self_node-1].class_roster;
		    csp_class_roster_event_apply(class_roster,
						 &action->act_msg,
						 lclass_name);
		}

		/* queue event onto any pending rosters */
		status = csp_class_roster_queue(&action->act_msg, lclass_name);
		break;

	    case CSP_ACT_13_DBMS_START:
		/* process dbms class start request action */
		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: CSP_ACT_13_DBMS_START.\n") );
		}
		/* process dbms class start action */
		status = CXmsg_redeem(
		    action->act_msg.dbms_start.class_name,
		    (PTR) lclass_name, sizeof(lclass_name), 0, &lmbuf_len );

		if (status != OK)
		    break;

		CXmsg_release(&action->act_msg.dbms_start.class_name);

		if ( csp_debug )
		{
		    _VOID_ TRdisplay(
		      ERx("csp_debug %@: Classname = %t.\n"),
		      STnlength(lmbuf_len,lclass_name), lclass_name );
		}

		if (csp->csp_self_node ==
		    action->act_msg.dbms_start.node)
		{
		    PID pid;
		    status = csp_dbms_start( lclass_name, &pid);
		    if (status != E_DB_OK)
		    {
			/* log the error, but this cannot be fatal for us */
			TRdisplay(ERx("dbms class failed to start\n")); /* fanch01 FIXME*/
			status = E_DB_OK;
		    }
		    if ( csp_debug )
		    {
			_VOID_ TRdisplay(
			  ERx("csp_debug %@: Started %t with PID %d (0x%x).\n"),
			  lmbuf_len, lclass_name, pid, pid );
		    }
		}
		break;

	    default:
		break;
	    }
	}

	if ( action->act_resume )
	{
	    CSresume( action->act_resume );
	}

	if (status != E_DB_OK)
	{
	    if ( csp_debug )
	    {
		_VOID_ TRdisplay(
		  ERx("csp_debug %@: CRASH on status = 0x%x.\n"),
		  status );
	    }
	    crash(E_DM9901_CSP_ACTION_FAILURE, &sys_err);
	}

	/* Put completed action back on "free" SLL */
	{
	    ACTION *next;

	    do
	    {
		next = csp->csp_free_actions;
		action->act_next = next;
	    } while ( CScasptr((PTR*)&csp->csp_free_actions,
				(PTR)next, (PTR)action) );
	}
    }
    
    /* Shut down the messaging system thread */
    CXmsg_disconnect( CSP_CHANNEL ); 
    CXmsg_shutdown();

    TRdisplay("%@ %d dmfcsp thread has terminated with status %x\n",
		__LINE__, final_status);

    EXdelete();
    dmc_cb->error.err_code = err_code;
    return (final_status);
}


/*{
** Name: dmfcsp_msg_monitor	- Body of message monitor thread.
**
** Description:
**      Body of message monitor thread.  Effectively just a wrapper
**	for CXmsg_monitor.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**	    OK on normal termination, else passes up error code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-2001 (devjo01)
**          Created.
*/
DB_STATUS
dmfcsp_msg_monitor(DMC_CB *dmc_cb)
{
    STATUS	status = OK;

    CLRDBERR(&dmc_cb->error);

    /* Wait until CSP thread is ready before enabling message subsystem */
    while ( (CS_SID)0 == csp_global.csp_sid )
    {
	status = CSsuspend( CS_TIMEOUT_MASK, 1, 0 );
    }
    if ( OK == status || E_CS0009_TIMEOUT == status )
	status = CXmsg_monitor();
    if ( status != 0 )
    {
	SETDBERR(&dmc_cb->error, 0, status);
	status = E_DB_ERROR;
    }
    return status;
} /* dmfcsp_msg_monitor */



/*{
** Name: csp_message_handler	- routine to process CX messages received.
**
** Description:
**
**	This routine is called by CXmsg_connect to process messages
**	broadcast on the CSP_CHANNEL.    Most of the time, it hands
**	off responsibility for processing a message to the thread
**	running dmfcsp, since long running operations should not be
**	performed during message processing.  Message processing is
**	serialized, and we are guaranteed not to receive another
**	message while responding to this one.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**	    TRUE.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-2001 (devjo01)
**          Created.
**	14-may-2004 (devjo01)
**	    Have CSP_MSG_2_LEAVE case do its work in main CSP thread.
**	22-Apr-2005 (fanch01)
**	    Added dbms class failover.  Added additional action message
**	    handlers for class up, down events and dbms class execution.
**	04-Oct-2007 (jonj)
**	    When node joins cluster, ensure that it's CSID is filled in,
**	    if needed (VMS). If the node was offline during CSP startup
**	    the CSID is unknown.
*/
static bool
csp_message_handler( CX_MSG *msgblk, PTR pauxdata, bool invalid )
{
    CSP			*csp = &csp_global;
    STATUS		 status;
    CX_REQ_CB		*preq;
    CSP_CX_MSG		*pmsg = (CSP_CX_MSG *)msgblk;
    CSP_NODE_INFO	*node_info;
    CL_ERR_DESC		 sys_err;
    i4			 node, action, victim;
    PTR			 buffer, newwriteptr;
    i4			 buflen;
    i4			 dmn_i;
    CSP_DBMS_INFO	*dbms;
    i4			dbms_rcvr_cnt;
    PID			pid;
    i2			lg_id_id;

    node = (i4)pmsg->csp_msg_node;
    action = (i4)pmsg->csp_msg_action;

    if ( csp_debug )
    {
	TRdisplay(ERx("csp_debug %@: Rcvd %w(%d) MSG from node %d\n"),
		  csp_msg_type_names, action, action, node );
    }

    if ( node <= 0 || node > CX_MAX_NODES ||
	 !csp_config->cx_nodes[node-1].cx_node_number )
    {
	/* Invalid node number!  Ignore message, but log infraction. */
	uleFormat(NULL, E_DM991E_CSP_BAD_BROADCAST, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &status, 0);
	return FALSE;
    }
    node_info = &csp->csp_node_info[node-1];

    switch ( action )
    {
    case CSP_MSG_1_JOIN:
	/*
	** CSP is broadcasting it has joined cluster.
	*/

	/*
	** Regardless of whether the joining node has been active
	** treat the join as a request for a class synchronization.
	*/
	csp_put_action(CSP_ACT_9_CLASS_SYNC_REQUEST, node, 0, pmsg);

	if ( csp->csp_self_node != node )
	{
	    CSp_semaphore(TRUE, &node_info->node_active_sem);
	    if ( node_info->node_active )
	    {
		/*
		** Node already "joined" to cluster!
		**
		** This should be impossible, but we need to be careful
		** about race conditions between 'node's active status
		** being set during this CSP's initialization using
		** information obtained by querying the CMR, which
		** might conceivably be stale, and an attempt of a
		** Node recovering after a brief failure to broadcast
		** its join notification.
		**
		** I assert this IS impossible, because offending
		** node could not restart until Master CSP has performed
		** recovery on its behalf, since Master CSP (and all
		** other CSP's, including most importantly for the
		** sake of this argument the current CSP) will hold a 
		** DMN lock for this node until recovery is complete.
		**
		** If failed node should restart in this interval, it will 
		** stall in CXinitialize until all DMN locks are released.
		**
		** Master CSP will remove failed node from CMR when
		** recovery is complete, so until recovery is complete,
		** or all CSP's haved died, a failed CSP cannot rejoin
		** the cluster.
		**
		** After all that discussion, situation is probably
		** benign if it should occur.  We'll log a message,
		** but continue without crashing CSP.
		*/
		REPORT_ODDITY(ERx("join message received from active node"));
	    }
	    else
	    {

		/* Initialize dbms info for this node */
		MEfill(sizeof(node_info->node_dbms), '\0', &node_info->node_dbms);

		/* Ensure CSID, if needed, is filled in */
		(void) CXalter( CX_A_NEED_CSID, (PTR)node );

		/*
		** Request deadman lock on CSP.   We can do this 
		** here, since it is a quick non-blocking operation.
		*/
		preq = &node_info->deadman_lock_req;
		preq->cx_new_mode = LK_S;
		preq->cx_key.lk_type = LK_CSP;
		preq->cx_key.lk_key1 = node;
		preq->cx_key.lk_key2 = 0;
		preq->cx_user_func = csp_deadman_notify;
		status = CXdlm_request(
		  CX_F_STATUS | CX_F_PCONTEXT | CX_F_NODEADLOCK |
		  CX_F_PRIORITY | CX_F_IGNOREVALUE | CX_F_NOTIFY |
		  CX_F_NO_DOMAIN, preq, &csp->csp_tranid );
		if ( OK != status )
		{
		    if ( E_CL2C01_CX_I_OKSYNC == status )
		    {
			/* 
			** If this is grantable, 'node' must have died,
			** or will die, whilst joining the cluster.
			** It is a gross logic error for a CSP to continue
			** in the cluster once a it's deadman lock
			** is released/lost.  This being the case, Join
			** message can be ignored.
			*/
			(void)CXdlm_release( CX_F_IGNOREVALUE, preq );
		    }
		    else
		    {
			/* 
			** Bad news!  If we cannot place a DMN lock on
			** this CSP, then we cannot detect a failure by
			** this CSP, therefore the usefulness of current
			** CSP is compromised.   The safest, and simplest
			** thing to do, is to CRASH this node.  While
			** this may seem extreme, the chance of this
			** occuring is remote, and the logic to allow us
			** to tolerate this condition is non-trivial,
			** and may well introduce additional problems
			** for negligible benefit.
			*/
			CSv_semaphore(&node_info->node_active_sem);
			crash(E_DM991D_CSP_NO_NODE_LOCK, &sys_err);
		    }
		}
		else
		{
		    /*
		    ** Note that node is active. 
		    ** Lock will stay queued, until one of the following:
		    **
		    ** 1) Target CSP announces its intention to leave.
		    ** 2) Target CSP crashes.
		    ** 3) This CSP leaves the cluster, in which case
		    **    pending request is canceled.
		    */
		    
		    if ( node < csp->csp_self_node &&
			 csp->csp_self_node == csp->csp_master_node )
		    {
			/*
			** New candidate master has come on board.
			** Notify main CSP thread that it must pass the
			** baton.
			*/
			csp_put_action(CSP_ACT_1_NEW_MASTER, node, 0, pmsg);
		    } 
		    node_info->node_active = 1;
		}
	    }
	    CSv_semaphore(&node_info->node_active_sem);
	}
	break;

    case CSP_MSG_2_LEAVE:
	/*
	** CSP is broadcasting its irrevokable intent to leave cluster.
	*/
	if ( csp->csp_self_node != node )
	{
	    csp_put_action(CSP_ACT_7_LEAVE, node, 0, pmsg);
	}
	break;

    case CSP_MSG_3_RCVRY_CMPL:
	/*
	** Master CSP is announcing successful recovery of
	** one or more failed nodes.
	*/
	for ( victim = 1, node_info = csp->csp_node_info;
	      victim <= CX_MAX_NODES; 
	      victim++, node_info++ )
	{
	    if ( CX_GET_NODE_BIT(pmsg->rcvry.node_bits,victim) )
	    {
		if ( victim == csp->csp_self_node )
		{
		    /* I'm not dead ... I feel happy ... 
		    **
		    ** Ugh oh.  Gross logic error.  We should only
		    ** be in the recovery set if this node had failed.
		    **
		    ** Node can't come back online until all
		    ** the DMN locks held against it are released,
		    ** so it is not possible for a restarted
		    ** CSP to read its own obituary.
		    */
		    REPORT_ODDITY(ERx("Current CSP in recovery set"));
		    crash( E_DM9923_CSP_ACTION_ORDER, &sys_err );
		}

		/*
		** Node recovery complete (CSP_MSG_3_RCVRY_CMP)
		** Node recovery complete -> DBMS recovery complete
		** Discard dbms DMN lock on remote CSP.
		*/
		for (dmn_i = 0, dbms = &node_info->node_dbms[0]; 
				    dmn_i < CSP_MAX_DBMS; dmn_i++, dbms++)
		{
		    preq = &dbms->deadman_lock_req;
		    pid = preq->cx_key.lk_key2;
		    lg_id_id = dmn_i;

		    if (pid)
		    {
			TRdisplay("%@ CSP: %w->%w %d pid %d lgid %d\n", 
			csp_msg_type_names, CSP_MSG_3_RCVRY_CMPL,
			csp_msg_type_names, CSP_MSG_12_DBMS_RCVR_CMPL,
			victim, pid, lg_id_id);

			csp_rcvr_cmpl_msg(victim, pid, lg_id_id);
		    }
		}

		MEfill(sizeof(node_info->node_dbms), '\0', &node_info->node_dbms);

		/*
		** Discard DMN lock on remote CSP.
		**
		** DMN lock can't be released prior to MASTER 
		** announcing recovery is complete, since master 
		** could fail, and this node might become responsible 
		** for recovery of the original failed node(s) (as 
		** well as the newly failed master).
		**
		** Failed CSP's cannot come back on-line until all
		** DMN locks have been released.
		*/
		preq = &node_info->deadman_lock_req;
		if ( CX_NONZERO_ID( &preq->cx_lock_id ) )
		{
		    if ( preq->cx_old_mode != preq->cx_new_mode )
		    {
			/* Unlikely case */
			status = CXdlm_cancel( 0, preq );
		    }
		    status = CXdlm_release( 0, preq );
		}
		CSp_semaphore(TRUE, &node_info->node_active_sem);
		node_info->node_active = 0;
		CSv_semaphore(&node_info->node_active_sem);
		node_info->node_failure = 0;
	    }
	}
	csp_put_action(CSP_ACT_3_RCVRY_CMPL, node, 0, pmsg);
	break;

    case CSP_MSG_4_NEW_MASTER:
	/*
	** Broadcast of new master node.
	**
	** See csp_announce_new_master banner for when this is broadcast.
	*/
	CXalter( CX_A_MASTER_CSP_ROLE, (PTR)( node == csp->csp_self_node ) );
	csp->csp_master_node = node;
	break;

    case CSP_MSG_5_RPTIVB:
	/*
	** An LK lock client received a lock value block marked as
	** invalid by the DLM, pass this on for handling by CSP thread.
	*/
	csp_put_action(CSP_ACT_5_IVB_SEEN, node, 0, pmsg);
	break;

    case CSP_MSG_6_ALERT:
	/*
	** We need to propagate an SCF alert to this node.  Actual
	** event text is larger than can be held in the "msg" struct,
	** and needs to be pulled using a CX coupon ("chit").
	*/

	/*
	** Step one, get place to queue event text
	*/
	buflen = pmsg->alert.length;
	newwriteptr = csp->csp_alert_qbuf_write + buflen + sizeof(i4);
	newwriteptr = ME_ALIGN_MACRO(newwriteptr, sizeof(i4));
	if ( csp->csp_alert_qbuf_write < csp->csp_alert_qbuf_read &&
	     newwriteptr >= csp->csp_alert_qbuf_read )
	{
	    csp_alert_overflow();
	    break;
	}

	if ( newwriteptr >= csp->csp_alert_qbuf_end )
	{
	    /* Wrap buffer */
	    newwriteptr = csp->csp_alert_qbuf_start + buflen + sizeof(i4);
	    newwriteptr = ME_ALIGN_MACRO(newwriteptr, sizeof(i4));
	    if ( newwriteptr >=  csp->csp_alert_qbuf_read )
	    {
		csp_alert_overflow();
		break;
	    }
	    *(i4 *)csp->csp_alert_qbuf_start = CSP_ALERT_QUEUE_EOF;
	    *(i4 *)csp->csp_alert_qbuf_write = CSP_ALERT_QUEUE_WRAP;
	    csp->csp_alert_qbuf_write = csp->csp_alert_qbuf_start;
	}

	buffer = csp->csp_alert_qbuf_write + sizeof(i4);

	/*
	** Step two, redeem text using coupon into queue.
	*/
	status = CXmsg_redeem( pmsg->alert.chit, buffer,
			       buflen, 0, &buflen ); 
	if ( status )
	{
	    break;
	}

	/*
	** Step three, put in length and terminator, so CSP
	** thread can pick this up.  Order of assignments is
	** important.
	*/
	*(i4 *)newwriteptr = CSP_ALERT_QUEUE_EOF;
	*(i4 *)csp->csp_alert_qbuf_write = buflen;
	csp->csp_alert_qbuf_write = newwriteptr;

	/* 
	** Step four, kick CSP thread to perform a callback
	** into SCF to perform the sce_raise call for this
	** node.  Done only if needed, so there should be at
	** most one CSP_ACT_6_ALERT action pending per CSP.
	*/
	if ( !csp->csp_alert_pending )
	{
	    csp->csp_alert_pending = 1;
	    csp_put_action(CSP_ACT_6_ALERT, node, 0, pmsg);
	}
	break;

    case CSP_MSG_7_CKP:

	csp_ckp_msg(pmsg);
	break;

    case CSP_MSG_8_CLASS_SYNC_REPLY:
	/*
	** CSP is broadcasting dbms class synchronization
	** shortly after a node joins the cluster.
	*/
	csp_put_action(CSP_ACT_10_CLASS_SYNC_REPLY, node, 0, pmsg);
	break;

    case CSP_MSG_9_CLASS_START:
	/*
	** dmcstart is broadcasting dbms class startup.
	*/

	/* Request dbms deadman lock */
	csp_req_dbms_dmn(pmsg->csp_msg_node, pmsg->class_event.pid,
			pmsg->class_event.lg_id_id);

	csp_put_action(CSP_ACT_11_CLASS_START, node, 0, pmsg);
	break;

    case CSP_MSG_10_CLASS_STOP:
	/*
	** dmcstop is broadcasting dbms class shutdown.
	*/

	/* Release dbms deadman lock */
	csp_rls_dbms_dmn(pmsg->csp_msg_node, pmsg->class_event.pid,
			pmsg->class_event.lg_id_id);

	csp_put_action(CSP_ACT_12_CLASS_STOP, node, 0, pmsg);
	break;

    case CSP_MSG_11_DBMS_START:
	/*
	** Master node is requesting a server class start for recovery.
	*/
	csp_put_action(CSP_ACT_13_DBMS_START, node, 0, pmsg);
	break;

    case CSP_MSG_12_DBMS_RCVR_CMPL:
	/*
	** Recovery for some dbms complete 
	** MAY need to resume normal lock processing
	*/

	TRdisplay(ERx("%@ CSP: %w(%d) node %d pid %d lg_id %d\n"),
		  csp_msg_type_names, action, action, node, 
		  pmsg->class_event.pid,
		  pmsg->class_event.lg_id_id);
	csp_rcvr_cmpl_msg(pmsg->csp_msg_node, pmsg->class_event.pid,
			pmsg->class_event.lg_id_id);
	
	break;

    default:
	/* Invalid action!  Ignore message */
	REPORT_ODDITY(ERx("Invalid message type"));
	uleFormat(NULL, E_DM9910_CSP_BADMSG, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &status, 0);
	break;
    }
    return FALSE;  /* Never a "last" message */
} /* csp_message_handler */


static void
csp_alert_overflow()
{
    STATUS	status;

    REPORT_ODDITY(ERx("Alert queue overflow"));
    uleFormat(NULL, E_DM9912_CSP_READ_QUEUE, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		&status, 0);
} /* csp_alert_overflow */


/*{
** Name: dmfcsp_msg_thread	- Body of message connection thread.
**
** Description:
**
**	Wrapper for CXmsg_connect.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**	    OK on normal termination, else passes up error code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-2001 (devjo01)
**          Created.
*/
DB_STATUS
dmfcsp_msg_thread(DMC_CB *dmc_cb)
{
    i4		error;
    DB_STATUS	status = E_DB_OK;

    CLRDBERR(&dmc_cb->error);

    /* Connect to message system */
    if ( error = CXmsg_connect( CSP_CHANNEL, csp_message_handler, NULL ) ) 
    {
        SETDBERR(&dmc_cb->error, 0, error);
	status = E_DB_ERROR;
    }
    return status;
} /* dmfcsp_msg_thread */



/*{
** Name: dmfcsp_shutdown	- Handle shutdown.
**
** Description:
**      This routine handles CSP shutdown.
**
** Inputs:
**      None
**
** Outputs:
**      None
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    Routine will wait until CSP thread processes shutdown,
**	    then return.  On completion, CSP will have left the cluster.
**
** History:
**      22-jun-2001 (devjo01)
**          New for 103715.
*/
VOID
dmfcsp_shutdown(VOID)
{
    csp_put_action(CSP_ACT_4_SHUTDOWN, csp_global.csp_self_node, 1, (CSP_CX_MSG *)0);
}


/*{
** Name: csp_deadman_notify	- Routine called if dead man lock granted.
**
** Description:
**      Completion routine called if dead-man lock granted.   If granted,
**	this generally indicates that the process represented by the
**	lock key has abended, and recovery activity is required.
**
** Inputs:
**	preq		- pointer to DLM request block.
**	status		- final status for lock request.
**
** Outputs:
**	Returns:
**	    none.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    CSP thread activated to perform recovery.
**
** History:
**      14-may-2001 (devjo01)
**          Created.
*/
static VOID
csp_deadman_notify( CX_REQ_CB *preq, i4 status )
{
    i4		 node;

    /* check if DMN was canceled.  If so, then no recovery needed. */
    switch ( status )
    {
    case OK:
    case E_CL2C08_CX_W_SUCC_IVB:

	/* Lock was granted, we have a node failure */
	(void)LKalter( LK_A_STALL, 0, (CL_ERR_DESC *)NULL );
	node = preq->cx_key.lk_key1;
	csp_global.csp_node_failure = 1;
	csp_global.csp_node_info[node-1].node_failure = 1;
	csp_put_action( CSP_ACT_2_NODE_FAILURE, node, 0, (CSP_CX_MSG *)0); 
	break;
    case E_CL2C22_CX_E_DLM_CANCEL:
	/* Completion for canceled DMN.  No recovery needed. */
	break;
    default:
	/* Unexpected result!  We must crash ourselves */
	crash(E_DM991A_CSP_NODE_LOCK, NULL ); 
	break;
    }
} /* csp_deadman_notify */


/*{
** Name: csp_dead_dbms_notify	- Routine called if dead man lock granted.
**
** Description:
**      Completion routine called if dead-man lock granted.   If granted,
**	this generally indicates that the process represented by the
**	lock key has abended, and recovery activity is required.
**
** Inputs:
**	preq		- pointer to DLM request block.
**	status		- final status for lock request.
**
** Outputs:
**	Returns:
**	    none.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    CSP thread activated to perform recovery.
**
** History:
**      11-may-2005 (stial01)
**          Created from csp_deadman_notify
*/
static VOID
csp_dead_dbms_notify( CX_REQ_CB *preq, i4 status )
{
    CSP			*csp = &csp_global;
    CSP_DBMS_INFO	*dbms;
    i4		 node;
    PID		 pid;
    i4		 dbms_rcvr_cnt;

    dbms = (CSP_DBMS_INFO *)preq;
    TRdisplay("%@ CSP: DBMS Notify node %d pid %d status %d \n", 
        preq->cx_key.lk_key1, preq->cx_key.lk_key2, status);

    /* check if DMN was canceled.  If so, then no recovery needed. */
    switch ( status )
    {
    case OK:
    case E_CL2C08_CX_W_SUCC_IVB:

	/*
	** Lock was granted, we have a dbms failure
	**
	** (This may happen after the CSP_MSG_12_DBMS_RCVR_CMPL msg
	**  or while the CSP_MSG_12_DBMS_RCVR_CMPL msg is being processed
	**
	*/

	/* Increment count of dbms servers needing recovery */
	dbms_rcvr_cnt = CSadjust_counter(&csp->csp_dbms_rcvr_cnt, (i4)1);

	(void)LKalter( LK_A_STALL, 0, (CL_ERR_DESC *)NULL );
	break;

    case E_CL2C22_CX_E_DLM_CANCEL:
	/* Completion for canceled DMN.  No recovery needed. */
	break;
    default:
	/* Unexpected result!  We must crash ourselves */
	crash(E_DM991A_CSP_NODE_LOCK, NULL ); 
	break;
    }
} /* csp_dead_dbms_notify */


/*{
** Name: csp_grab_recovery_lock	- Get exclusive control of recovery.
**
** Description:
**      This routine is a double check of the MASTER CSP protocol.
**	By grabbing a special lock, we guarantee the serialization
**	of the recovery process.   However, this is, and should be
**	completely unneccesary.  MASTER CSP protocol will prevent
**	more than one CSP from thinking it is the master, so this
**	check should be completely redundant.   However, it is cheap,
**	and serves as a runtime guarantee of the coding logic.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    OK if lock granted synchronously, else E_DM991C_CSP_MASTER_SIX.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**      09-jul-2001 (devjo01)
**          Created.
**	19-may-2004 (devjo01)
**	    Reformat contents of lock key.  Since we are "borrowing" the
**	    memory allocated to cx_value for the key, we can't format this
**	    once during init, as some DLMs clobber cx_value even when
**	    CX_F_IGNOREVALUE is passed.
*/
static STATUS
csp_grab_recovery_lock(void)
{
    STATUS	 status;
    CX_REQ_CB	*preq = &csp_global.csp_recovery_lock;

    if ( csp_debug )
    {
	TRdisplay(ERx("csp_debug %@: Grabbing recovery lock.\n"));
    }

    preq->cx_new_mode = LK_X;

    status = CXdlm_request( CX_F_NOWAIT|CX_F_PCONTEXT|CX_F_NODEADLOCK|
			    CX_F_IGNOREVALUE, preq, &csp_global.csp_tranid );
    if ( status )
    {
	status = E_DM991C_CSP_MASTER_SIX;
    }
    return status;
} /* csp_grab_recovery_lock */
			    

/*{
** Name: csp_release_recovery_lock	- Yeild exclusive control of recovery.
**
** Description:
**      This routine is a double check of the MASTER CSP protocol.
**	By grabbing a special lock, we guarantee the serialization
**	of the recovery process.   However, this is, and should be
**	completely unneccesary.  MASTER CSP protocol will prevent
**	more than one CSP from thinking it is the master, so this
**	check should be completely redundant.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    OK if lock released, else E_DM991C_CSP_MASTER_SIX.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-jul-2001 (devjo01)
**          Created.
*/
static STATUS
csp_release_recovery_lock(void)
{
    STATUS	 status;
    CX_REQ_CB	*preq = &csp_global.csp_recovery_lock;

    if ( csp_debug )
    {
	TRdisplay(ERx("csp_debug %@: Releasing recovery lock.\n"));
    }

    status = CXdlm_release( CX_F_IGNOREVALUE, preq );
    if ( status )
    {
	status = E_DM991C_CSP_MASTER_SIX;
    }
    return status;
} /* csp_grab_recovery_lock */
			    

/*{
** Name: csp_put_action	- Post an action to CSP from AST/signal.
**
** Description:
**	Enqueue an action request to the CSP thread from a thread.
**
** Inputs:
**	act		- Action code.
**	node		- Node for which action is required.
**	sync		- If non-zero, suspend, pending resume
**			  from CSP thread.
**      msg     	- For some actions, the original msg may contain
**                        additional action args.
**
** Outputs:
**	Returns:
**	    none.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-2001 (devjo01)
**          Created.
**	02-Oct-2003 (devjo01)
**	    Reworked to use C&S logic.  This avoids an unprotected
**	    update of free list when called by an AST/signal handler.
*/
static VOID
csp_put_action( i4 act, i4 node, i4 sync, CSP_CX_MSG *msg )
{
    CSP		*csp = &csp_global;
    ACTION	*a, *next;

    /* First get a free request from head of free queue. */
    do
    {
	a = csp->csp_free_actions;
    } while ( a != NULL &&
	      CScasptr((PTR*)&csp->csp_free_actions,
			(PTR)a, (PTR)a->act_next) );

    if ( NULL == a )
    {
	/*
	** Should NEVER happen.  Enough action blocks should be in
	** circulation to allow for the worst case number of actions
	** to be inflight.
	*/

	/* Failure to put a message is fatal, CSP must crash. */
	crash( E_DM9918_CSP_WRITE_ERROR, 0 );
    }

    a->act_type = act;
    a->act_node = node;
    if (msg)
	STRUCT_ASSIGN_MACRO(*msg, a->act_msg);
    else
	MEfill(sizeof(a->act_msg), '\0', &a->act_msg);
    if ( sync )
	CSget_sid( &a->act_resume );
    else
	a->act_resume = (CS_SID)0;

    do
    {
	next = csp->csp_posted_actions;
	a->act_next = next;
    } while ( CScasptr((PTR*)&csp->csp_posted_actions, (PTR)next, (PTR)a) );

    if ( csp->csp_ready_to_sleep )
    {
	/* Wake up main CSP thread if not already awake. */
	csp->csp_ready_to_sleep = 0;
	CSresume( csp->csp_sid );
    }

    if ( sync )
    {
	(VOID)CSsuspend( 0, 0, 0 );
    }
} /* csp_put_action */


/*{
** Name: csp_enqueue_actions - Add actions to main queue.
**
** Description:
**	Take a SLL of actions, and enqueue them to main queue
**	in reverse order.  We need to reverse the list ordering to 
**	retain the correct chronological order for the actions.
**
** Inputs:
**	alist		- SSL of one or more actions.
**
** Outputs:
**	Returns:
**	    none.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-Oct-2003 (devjo01)
**          Created.
*/
static VOID
csp_enqueue_actions( ACTION *alist )
{
    if ( alist->act_next ) csp_enqueue_actions( alist->act_next );
    ACT_ENQUEUE( &csp_global.csp_action_queue, alist );
} /* csp_enqueue_actions */


/*{
** Name: csp_find_master_node	- Check known nodes for lowest active.
**
** Description:
**      This is used to determine which node currently "should"
**	be the master node.   The fine distinction between "should"
**	and "is" is to allow for the perfectly legal possibility 
**	of a new master candidate joining the cluster while the
**	current master is engaged in some activity.   Current
**	master will renounce the master role, when it is at 
**	a breakpoint.   The only time a node can decide for itself that 
**	it is the master node, is if is either the FIRST CSP, or
**	failure of the master node is detected.
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**	    Node number of candidate master node.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-2001 (devjo01)
**          Created.
*/
static i4
csp_find_master_node( VOID )
{
    CSP_NODE_INFO	*node_info;
    i4			 node;

    node_info = csp_global.csp_node_info;
    for ( node = 1; node <= CX_MAX_NODES; node++ )
    {
	if ( node_info->node_active && !node_info->node_failure )
	    return node;
	node_info++;
    }
    /* Should only happen when sole CSP is shutting down */
    return 0;
} /* csp_find_master_node */


/*{
** Name: csp_announce_new_master - Set new master CSP for cluster.
**
** Description:
**      This is used to broadcast the new CSP to the cluster.
**	This is required because until current MASTER CSP is ready
**      to yield control of the MASTER role, a new lower numbered
**	node added to the cluster is only a master in waiting.
**
**	This is called under the following circumstances.
**
**	By MASTER CSP when it has detected a new lower numbered
**	node has entered the cluster, and it is not engaged in recovery.
**
**	By MASTER CSP when it is starting to shutdown.
**
**	By lowest numbered node if failure of Master node was detected.
**
**	In the rare case in which all current CSP's die
**	while one or more new CSP's start up.  These CSP's are not
**	first CSP's, but will detect that CMR is inconsistent and
**	that the set of existing CSP's is empty when they go to get
**	the DMN locks on the other (failed) CSP's.  In this case
**	the first CSP to broadcast master status is the master, but
**	if another CSP with a lower node number was also starting 
**	in this interval, the new master CSP will immediately yield
**	master status when it sees the lower numbered node join.
**
**	FIRST CSP claims master status by default, and does not need
**	to announce this (there are no other CSP's connected to the
**	message system anyway).
**
** Inputs:
**	node		- New master node.
**
** Outputs:
**	Returns:
**	    OK, else error code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-2001 (devjo01)
**          Created.
*/
static STATUS
csp_announce_new_master( i4 node )
{
    CSP_CX_MSG	cspmsg;

    if ( csp_debug )
    {
	TRdisplay(ERx("csp_debug %@: Tell world %d is MASTER.\n"), node);
    }

    cspmsg.csp_msg_action = CSP_MSG_4_NEW_MASTER;
    cspmsg.csp_msg_node = (u_i1)node;
	     
    return CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&cspmsg );
} /* csp_announce_new_master */


/*{
** Name: csp_cmr_roster_update - Atomically update/query CMR.
**
** Description:
**      This function in conjunction with csp_cmr_roster_update_cb
**	performs an atomic operation on the CMO dedicated to maintaining
**	the 'C'luster 'M'embership 'R'oster.
**
** Inputs:
**	node		- >0 node number to add
**			  <0 node number (-node) to remove
**			  =0 null update, perform a read
**	cmr		- Output of new CMR value
**
** Outputs:
**	Returns:
**	    OK, else error code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-may-2001 (devjo01)
**          Created.
**      06-May-2005 (fanch01)
**          Added 'query' capability to consolidate CMR access in
**          one place.  This allows the CMR to be recovered by the
**          same code for any access.  Added output cmr argument.
**          Renamed for clarity.
*/
static STATUS
csp_cmr_roster_update( i4 node, CX_CMO_CMR *cmr )
{
    STATUS		status;
    CX_CMO_CMR		cmocmr, *usecmr;

    if ( csp_debug )
    {
	if ( node > 0 )
	{
	    TRdisplay(ERx("csp_debug %@: adding %d to CMR.\n"), node);
	}
	else if ( node < 0 )
	{
	    TRdisplay(ERx("csp_debug %@: removing %d from CMR.\n"), 
		  0 - node);
	} else
	{
	    TRdisplay(ERx("csp_debug %@: query CMR.\n"));
	}
    }

    usecmr = cmr ? cmr : &cmocmr;

    status = CXcmo_update( CX_CMO_CMR_IDX, (CX_CMO *) usecmr,
                           csp_cmr_roster_update_cb, (PTR)&node );
    if ( csp_debug )
    {
#if 0 /* FIXME - SEGV sometimes seen with %#.4{ %x%} format */
	_VOID_ TRdisplay(ERx("csp_debug %@: New CMR = %#.4{ %x%}\n"),
	 sizeof(cmocmr.cx_cmr_members)/4, &usecmr->cx_cmr_members);
#else
	_VOID_ TRdisplay(ERx("csp_debug %@: New CMR = %x\n"),
	 usecmr->cx_cmr_members);
#endif
    }
    return status;
} /* csp_cmr_roster_update */

/*
** Name: csp_cmr_node_test - Test to see if a node is up and take
**       deadman locks on up nodes.  Updates the CMR accordingly.
**
** Description:
**      This function is a helper function used during CMR recovery.
**	It will use the information in node_info->node_active to
**	determine whether:
**	  . the csp already has a deadman lock on the node
**	  . to perform an optimistic active test by taking a deadman
**	    lock to determine if the node is alive
**
** Inputs:
**	node_info	- node_info of node to test
**	pcmocmr		- CMR to update
**	node		- node number to test (must match node_info)
**
** Outputs:
**	Returns:
**	    OK, else error code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-May-2005 (fanch01)
**          Created.
*/
static i4
csp_cmr_node_test(CSP_NODE_INFO *node_info, CX_CMO_CMR *pcmocmr, i4 node)
{
    i4 err_code = OK;
    CX_REQ_CB *preq;

    CSp_semaphore(TRUE, &node_info->node_active_sem);
    if (!node_info->node_active)
    {
	/* records show node is down, try for a new deadman lock */
	preq = &node_info->deadman_lock_req;
	preq->cx_new_mode = LK_S;
	preq->cx_key.lk_type = LK_CSP;
	preq->cx_key.lk_key1 = node;
	preq->cx_key.lk_key2 = 0;
	preq->cx_user_func = csp_deadman_notify;

	err_code = CXdlm_request(
	  CX_F_STATUS | CX_F_PCONTEXT | CX_F_NODEADLOCK |
	  CX_F_PRIORITY | CX_F_IGNOREVALUE | CX_F_NOTIFY |
	  CX_F_NO_DOMAIN, preq, &csp_global.csp_tranid );

	if (OK == err_code)
	{
	    /* request is queued, node is up */
	    CX_SET_NODE_BIT(pcmocmr->cx_cmr_members, node);
	    node_info->node_active = 1;
	}
	else if (E_CL2C01_CX_I_OKSYNC == err_code)
	{
	    /* request granted, node is down */
	    if (CX_NONZERO_ID(&preq->cx_lock_id))
	    {
		(void)CXdlm_release(CX_F_IGNOREVALUE, preq);
	    }
	    err_code = OK;
	}
	else
	{
	    /* 
	    ** Fatal error, we must obtain all valid
	    ** DMN locks to join the cluster.
	    */
	    err_code = E_DM991D_CSP_NO_NODE_LOCK;
	}
    }
    else 
    {
	/* records show node is up, should already have deadman lock */
	CX_SET_NODE_BIT(pcmocmr->cx_cmr_members, node);
    }
    CSv_semaphore(&node_info->node_active_sem);
    return(err_code);
}

/*
** Name: csp_cmr_update_cb - Callback function for CMR updates.
**
** Description:
**      Atomic update callback handles update of CMR and possible
**	recovery of it if the CMO LVB is invalid or empty.
**
** Inputs:
**	oldval		- node_info of node to test
**	pcmocmr		- CMR to update
**	node		- node number to test (must match node_info)
**
** Outputs:
**	Returns:
**	    OK, else error code.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-May-2005 (fanch01)
**          Created.
*/
static STATUS
csp_cmr_roster_update_cb( CX_CMO *oldval, CX_CMO *pnewval,
                          PTR pnode, bool invalidin )
{
    CX_CMO_CMR		*pcmrcmo = (CX_CMO_CMR *) pnewval;
    STATUS		status = OK;
    i4			node;

    /* make sure there's a '1' somewhere in the lvb to mark it as populated */
    if (!((CX_CMO_CMR *) oldval)->cx_cmr_populated || invalidin)
    {
	/* initialize lvb */
	pcmrcmo->cx_cmr_recovery = 0;
	pcmrcmo->cx_cmr_populated = 1;
	(void)MEfill( sizeof(pcmrcmo->cx_cmr_extra), '\0',
		      (PTR)&pcmrcmo->cx_cmr_extra );

	/* if first one in, or invalid populate it */
	for ( node = 0; node < CX_MAX_NODES; node++ )
	{
	    /*
	    ** Note we even get a DMN on ourselves.  Our Deadman
	    ** lock is held in the context of a separate transaction,
	    ** but both specify no deadlock, so this is OK.  Obviously,
	    ** we should never have our own DMN granted to us, so this is
	    ** just a piece of paranoid double checking.
	    */
	    if ( csp_config->cx_nodes[node].cx_node_number )
	    {
		CSP_NODE_INFO *node_info = &csp_global.csp_node_info[node];
		status = csp_cmr_node_test(node_info, pcmrcmo, node+1);

		if ( status != OK )
		    break;
	    }
	}
    }
    else
    {
	/* lvb valid, just copy and optionally tweak bit */
	MEcopy( (PTR)oldval, sizeof(CX_CMO), (PTR)pnewval );
	node = *(i4 *)pnode;
	if ( node < 0 )
	{
	    node = 0 - node;
	    CX_CLR_NODE_BIT( pcmrcmo->cx_cmr_members, node );
	}
	else if (node > 0)
	{
	    CX_SET_NODE_BIT( pcmrcmo->cx_cmr_members, node );
	}
    }

    return(status);
}

   

/*{
** Name: csp_initialize	- Initialize the CSP.
**
** Description:
**      This routine initializes the CSP for operation. 
**	Mains steps are:
**	    Read configuration file for the RCP and read configuration file
**	    for the CSP.
**
**	    Using configuration information, initialize the LG/LK system and
**	    initialize any of the static CSP information.
**
**	    Prime the CSP's asynchronous state machine.
**
** Inputs:
**      None.
**
** Outputs:
**      recovery_done		Boolean set to TRUE if the local log
**				underwent recovery of any kind.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-jun-1987 (Derek)
**          Created.
**	05-Jan-1990 (ac)
**	    Added CSP exit handler to cache any abnormal exit context.
**      14-feb-1991 (jennifer)
**          Added initialization of csp_timeout used to delay node leaving
**          after a DECNET transition.
**	19-aug-1991 (rogerk)
**	    Set process type in the svcb.  This was done as part of the
**	    bugfix for B39017.  This allows low-level DMF code to determine
**	    the context in which it is running (Server, RCP, ACP, utility).
**	10-apr-1992 (rogerk)
**	    Fix call to ERinit().  Added missing args: p_sem_func, v_sem_func.
**	    Even though we do not pass in semaphore routines, we still need
**	    to pass the correct number of arguments.
**	18-jun-1992 (jnash)
**	    If II_DMFRCP_STOP_ON_INCONS_DB set, crash rather than
**	    marking a database inconsistent.
**	25-june-1992 (jnash)
**	    Change name of CSP log file from II_CSP.LOG to IICSP.LOG.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	5-nov-1992 (bryanp)
**	    Check CSinitate return code.
**	26-apr-1993 (bryanp/andys/keving)
**	    6.5 Cluster Project I:
**		Replace old calls to look at rcpconfig.dat with
**		    dmr_get_lglk_parms
**		Initialize SCF & CS at startup using hacky memory allocation.
**		Pass correct arguments to dmf_init().
**	24-may-1993 (bryanp)
**	    Log useful error messages if lgk_get_config fails.
**	    Resource blocks are now configured separately from lock blocks.
**	26-jul-1993 (bryanp)
**	    Added csp_lock_list argument to dmf_init()
**	    Check the return code from TRset_file -- if it fails, there's
**		probably some severe configuration errors, so stop immediately.
**	23-aug-1993 (bryanp)
**	    Change default DMF buffer cache size from 256 buffers to 1024
**		buffers.
**	    Allow DMF buffer cache size to be settable via the
**		II_CSP_DMF_CACHE_SIZE variable.
**	20-sep-1993 (bryanp)
**	    Check enqueue limit when starting up.
**	22-aug-1995 (dougb)
**	    Don't use async system-services when we want the info immediately.
**	    Init new module-level variable, csp_dlck_ef.
**	25-aug-1995 (dougb) bug #70696
**	    Init new module-level variable, csp_dlck_interval -- the minimum
**	    interval between calls to gbl_deadlock().
**	30-sep-1995 (dougb)
**	    Add TRdisplay() calls when LIB$GET_EF() returns a bogus value,
**	    or when a bad deadlock interval is specified.
**      06-mar-1996 (stial01)
**          Variable Page Size project:
**          Additional CSP environment variables: II_CSP_DMF_CACHE_SIZE_pgsize
**          Call dmf_init with additional buffers parm (nil)
**	21-aug-1996 (boama01)
**	    - Add exception handler setup and code to respond to exceptional
**	      situation (EXdeclare, EXdelete).
**	    - Moved setting of csp_debug until after LKinitialize() call,
**	      since that rtn now inits the switch to FALSE.
**	12-may-2001 (devjo01)
**	    Complete rewrite as part of 103715.
**	31-Mar-2005 (jenjo02)
**	    Externalized the function, called from dmr_log_init during
**	    startup on a cluster instead of dmr_recover.
**	01-Apr-2005 (jenjo02)
**	    Report failures here rather than in dmr_log_init for
**	    transparency.
**	22-Apr-2005 (fanch01)
**	    Added dbms class failover.  Initialize new csp elements.
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**	    
*/
STATUS
csp_initialize(bool *recovery_done)
{
    CSP		    *csp = &csp_global;
    CSP_NODE_INFO   *nodeinfo;
    STATUS	    status = E_DB_ERROR;
    char	    filename[DB_MAXNAME];
    CL_ERR_DESC     sys_err;
    DMP_MISC	    *new_object;
    i4		    i;
    i4		    errcode;
    char	    sem_name[CS_SEM_NAME_LEN];
    DB_ERROR	    local_dberr;

    errcode = OK;
    *recovery_done = FALSE;

    /*	Initialize the CSP database. */

    csp->csp_state = CSP_ST_1_START;
    csp->csp_self_node = CXnode_number(NULL);
    csp->csp_recovery_lock.cx_key.lk_type = LK_CSP;
    csp->csp_recovery_lock.cx_key.lk_key1 = -1;
    csp->csp_recovery_lock.cx_key.lk_key2 = 0;
    csp->csp_alert_pending = 0;
    ACT_QUEUE_INIT(&csp->csp_action_queue);
    csp->csp_posted_actions = NULL;
    csp->csp_free_actions = csp->csp_action_buffers;
    for ( i = 1; i < NUM_ACT_BUFS; i++ )
	csp->csp_action_buffers[i-1].act_next = &csp->csp_action_buffers[i];
    csp->csp_dbms_rcvr_cnt = 0;
    csp->csp_action_buffers[NUM_ACT_BUFS-1].act_next = NULL;

    for ( ; /* something to break out of */ ; )
    {
	for ( i = 0; i < CX_MAX_NODES; i++ )
	{
	    nodeinfo = &csp->csp_node_info[i];
	    STprintf(sem_name, "csp_node_active %d", i+1);
	    status = CSw_semaphore(&nodeinfo->node_active_sem, CS_SEM_SINGLE, sem_name);
	    if (status)
		break;

	    nodeinfo->node_active = 0;
            nodeinfo->class_roster.class_roster_id = 0;
	    QUinit((QUEUE *) &nodeinfo->class_roster.classes);
	    QUinit((QUEUE *) &nodeinfo->class_roster.class_events);
	    nodeinfo->class_roster.class_count = 0;
	    nodeinfo->class_roster.class_roster_build = 0;
	    nodeinfo->class_roster.class_roster_master = 0;
	    MEfill(sizeof(nodeinfo->node_dbms), '\0', &nodeinfo->node_dbms);
	}
	if (status)
	    break;

	errcode = CXunique_id( &csp->csp_tranid );
	if ( errcode ) break;
    
	/* Read cluster configuration from config.dat. */
	if ( CXcluster_nodes( NULL, &csp_config) )
	{
	    errcode = E_DM9902_CSP_CONFIG_ERROR;
	    break;
	}

	status = LKalter( LK_A_CSPID, 0, &sys_err );
	if ( status ) break;

	/*
	** If we're the Master, perform cluster-wide recovery
	** on all logs, including our own, otherwise only
	** dmr_recover our local log.
	**
	** For local-only recovery, the usual situation will
	** be such that another Master node has done cluster-wide
	** recovery, including our log, and set its status to
	** LGH_OK; the call to dmr_recover will verify that
	** and set the log status to LGH_VALID, just what we
	** require.
	*/
	if ( CXconfig_settings( CX_HAS_1ST_CSP_ROLE ) )
	{
	    /* Perform cluster wide recovery. */
	    TRdisplay("%@ CSP-R0: Initiating Cluster Restart Recovery.\n");
	    status = csp_restart(recovery_done, &errcode);
	}
	else
	{
	    /* Only local recovery needed */
	    TRdisplay("%@ CSP-R0: Initiating Local Restart Recovery.\n");
	    status = dmr_recover(RCP_R_STARTUP, recovery_done);
	}

	if (status)
	    break;

	/* FIX-ME  this should be sensitive to the configured # of events */
	status = dm0m_allocate( sizeof (DMP_MISC) + CSP_ALERT_QUEUE_SIZE, 
				0, MISC_CB, MISC_ASCII_ID,
			        0, (DM_OBJECT**)&new_object, &local_dberr );
	if ( status ) 
	{
	    errcode = local_dberr.err_code;
	    break;
	}

	csp->csp_alert_qbuf_read = csp->csp_alert_qbuf_write =
	  csp->csp_alert_qbuf_start = (PTR)( new_object + 1 );
	new_object->misc_data = (char*) csp->csp_alert_qbuf_read;
	*(i4 *)(csp->csp_alert_qbuf_write) = CSP_ALERT_QUEUE_EOF;

	csp->csp_alert_qbuf_end = csp->csp_alert_qbuf_start +
	    CSP_ALERT_QUEUE_SIZE - sizeof(i4);
	break;
    }
    
    if ( status )
    {
	if ( errcode )
	    _VOID_ uleFormat(NULL, errcode, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &errcode, 0);
	_VOID_ uleFormat(NULL, E_DM9900_CSP_INITIALIZE, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &errcode, 0);
    }

    return (status);
}


/*{
** Name: csp_recover_logs	- Recover multiple logs for the CSP.
**
** Description:
**	For each log in the installation this routine performs analysis,
**	redo and undo processing. It is based on the dmr_recover routine
**	used by the rcp.
**
** Inputs:
**	rcp
**	num_nodes
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**	26-apr-1993 (keving/bryanp)
**	    Created for the 6.5 VAX Cluster Recovery Project I
**	    This routine supports both restart recovery and node failure
**		recovery.
**	26-jul-1993 (bryanp)
**	    Cleaned up a bit by deleting some commented-out code.
**	20-sep-1993 (bryanp)
**	    Call csp_archive for each log after recovering the log.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the csp_archive call.
**	30-Mar-2005 (jenjo02)
**	    If nothing to recover, don't forget to get rid
**	    of the memory allocated for the RCP.
**	31-Mar-2005 (jenjo02)
**	    Added LCTX_WAS_RECOVERED status bit.
*/
static STATUS
csp_recover_logs(
DMP_LCTX**	    lctx_array,
i4		    rcp_recovery_type,
i4		    num_nodes,
i4		    *err_code)
{
    STATUS		status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    RCP			*rcp;
    DMP_LCTX		*lctx;
    LG_HEADER		*header;
    LG_HDR_ALTER_SHOW	alter_parms;
    i4			i;
    i4			logs = 0;
    i4			recover[CX_MAX_NODES];

    /* Create and initialise recovery context */

    status = dmr_alloc_rcp( &rcp, rcp_recovery_type, lctx_array, num_nodes );
    if (status != E_DB_OK)
    {
	uleFormat(NULL, E_DM940E_RCP_RCPRECOVER, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	crash(E_DM940E_RCP_RCPRECOVER, &sys_err);
    }

    /* For every node we need to recover */
    for (i = 0; i < num_nodes; i++)
    {
	lctx = lctx_array[i];
	header = &lctx->lctx_lg_header;	
  
	/* I think the lctx header is up to date ...*/

	/* No recovery needed - yet */
	lctx->lctx_status &= ~LCTX_WAS_RECOVERED;

	/*
	** If the log file indicates that the EOF is valid but that the last
	** consistency point address is not known, then scan backwards to
	** find the last CP.
	*/
  
	if (header->lgh_status == LGH_EOF_OK)
	{
	    if (status = dmr_get_cp(rcp, i))
	    {
		*err_code = rcp->rcp_dberr.err_code;
		TRdisplay("\nCouldn't find the last CP for node %d (%d).\n",
			    lctx->lctx_node_id, i);
		break;
	    }
    	}

	/*
	** Update the log file status to VALID now that the EOF, BOF and CP
	** addresses have all been found.  It is now ready to be used.
	**
	** Remember the previous status as it indicates whether or not
	** recovery processing is needed before bringing the system online.
	**
	** If recovery is done below, there will likely be update made to
	** the log file and the EOF moved forward.  If we crash during
	** recovery processing, we will need to scan for the new EOF during
	** CSP startup (indicated by the VALID status).
	*/
	if (header->lgh_status != LGH_OK)
	{
	    recover[logs++] = i;

	    /* Notify caller recovery done in this log */
	    lctx->lctx_status |= LCTX_WAS_RECOVERED;

	    header->lgh_status = LGH_VALID;
	    TRdisplay("%@ CSP: Marking Log File status VALID for node %d.\n",
			    lctx->lctx_node_id);

	    alter_parms.lg_hdr_lg_header = *header;
	    alter_parms.lg_hdr_lg_id = lctx->lctx_lgid;
	    status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
				sizeof(alter_parms),&sys_err);
	    if (status != OK)
	    {
		_VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		_VOID_ uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 1,
		    0, LG_A_NODELOG_HEADER);
		_VOID_ uleFormat(NULL, E_DM9405_RCP_GETCP, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		break;
	    }

	    status = LGforce(LG_HDR, lctx->lctx_lxid, 0, 0, &sys_err);
	    if (status != OK)
	    {
		_VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		_VOID_ uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 1,
		    0, lctx->lctx_lxid);
		_VOID_ uleFormat(NULL, E_DM9405_RCP_GETCP, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		break;
	    }

	}
	else
	    TRdisplay("%@ CSP: Log File status OK, no recovery needed for node %d.\n",
			    lctx->lctx_node_id);
	/*
	** Initialize LCTX fields from the Log Header.
	*/
	lctx->lctx_eof.la_sequence = header->lgh_end.la_sequence;
	lctx->lctx_eof.la_block    = header->lgh_end.la_block;
	lctx->lctx_eof.la_offset   = header->lgh_end.la_offset;
	lctx->lctx_bof.la_sequence = header->lgh_begin.la_sequence;
	lctx->lctx_bof.la_block    = header->lgh_begin.la_block;
	lctx->lctx_bof.la_offset   = header->lgh_begin.la_offset;
	lctx->lctx_cp.la_sequence  = header->lgh_cp.la_sequence;
	lctx->lctx_cp.la_block     = header->lgh_cp.la_block;
	lctx->lctx_cp.la_offset    = header->lgh_cp.la_offset;
    }

    if (status)
	crash(*err_code, &sys_err);


    if (logs == 0)
    {
	/* Clean up after ourselves lest we leak memory */
	dmr_cleanup(rcp);
	dm0m_deallocate((DM_OBJECT **)&rcp);
	return (E_DB_OK);
    }


    for (i = 0; i < logs; i++)
    {

	status = dmr_offline_context(rcp, recover[i]);
	if (status)
	{
	    *err_code = rcp->rcp_dberr.err_code;
	    break;
	}
    }
    if (status)
	crash(*err_code, &sys_err);

    /*
    **
    **	For each node, perform the standard restart analysis pass:
    **	- which databases were open at the time of the crash?
    **	- which databases used fast commit?
    **	- when was the last consistency point taken?
    **	- were there any non-REDO operations?
    **	- which transactions were in progress at the time of the crash?
    **
    **	    NOTE: this analysis is cumulative; at completion we have full
    **		  analysis information for all the nodes, including a unified
    **		  list of all open transactions on all nodes.
    */
    for (i = 0; i < logs; i++)
    {
	status = dmr_analysis_pass(rcp, recover[i]);
	if (status)
	{
	    *err_code = rcp->rcp_dberr.err_code;
	    break;
	}
    }
    if (status)
	crash(*err_code, &sys_err);

    /*
    **
    **	Re-populate the logging system by calling LGadd and LGbegin to
    **	create new transaction handles for each transaction that was open at
    **	the time of the crash. Each such transaction is associated with the
    **	same node it was associated with at the time of the crash.
    */
    status = dmr_init_recovery(rcp);
    if (status)
    {
	*err_code = rcp->rcp_dberr.err_code;
	crash(*err_code, &sys_err);
    }

    /*
    **	For each node, call the recovery routines to perform the REDO pass:
    **	- position this node's logfile to the point where REDO starts (the
    **	  last consistency point, or perhaps later if the databases have been
    **	  opened and closed since the last consistency opint).
    **	- begin reading the logfile forwards.
    **	- for each record, if the analysis pass indicated that this record needs
    **	  to be redone, then call the DMVE routines to redo it. For a database
    **	  opened with Fast Commit, this generally means that all operations
    **	  need to get redone, whereas if Fast Commit is *not* in use, then only
    **	  the operations of uncommitted transactions need to get redone.
    */
    for (i = 0; i < logs; i++)
    {
	status = dmr_redo_pass(rcp, recover[i]);
	if (status)
	{
	    *err_code = rcp->rcp_dberr.err_code;
	    break;
	}
    }
    if (status)
	crash(*err_code, &sys_err);

    /*
    **
    **	Finally, perform a unified UNDO pass, using this loop:
    **	- search down the unified list of all open transactions and locate the
    **	  one that has the greatest LSN.
    **	- Position that node's logfiles to that record (by LGA) and read it.
    **	- Call DMVE to undo this operation.
    **	- Update this transaction's next (previous) LSN and LGA
    */

    status = dmr_undo_pass(rcp);
    if (status)
    {
	*err_code = rcp->rcp_dberr.err_code;
	crash(*err_code, &sys_err);
    }

    status = dmr_complete_recovery(rcp, num_nodes);
    if (status)
    {
	*err_code = rcp->rcp_dberr.err_code;
	crash(*err_code, &sys_err);
    }


    /*
    ** Deallocate structures on the RDB and RTX queues.
    */
    dmr_cleanup(rcp);

    /*
    ** Deallocate the RCP
    */
    dm0m_deallocate((DM_OBJECT **)&rcp);

    /*
    ** For each log that was recovered, archive the logfile contents:
    */

    for (i = 0; i < logs; i++)
    {
	lctx = lctx_array[recover[i]];

	TRdisplay("%@ CSP: Now archiving logfile contents for node %d\n",
		    lctx->lctx_node_id);

	status = csp_archive(lctx, err_code);
	if (status)
	    break;
    }
    if (status)
	crash(status,&sys_err);

    return(status);
}

/*{
** Name: csp_restart	- This routine handles crash restart.
**
** Description:
**      This routine loops handles crash restart processing.  The
**	following steps are performed:
**
**	For each node, call LGopen to:
**	- open that node's log file
**	- open that node's dual log file, if it exists
**	- perform restart logfile analysis (locate EOF, repair partially-written
**	  blocks, etc.)
**	- return a handle to be used for logging operations for this node.
**
**	// Allocate DMF buffer manager, notifying it of ALL the node logfile
**	// handles.
**	
**	Above paragraph no longer true as of s103715.
**
**	For each node, perform the standard restart analysis pass:
**	- which databases were open at the time of the crash?
**	- which databases used fast commit?
**	- when was the last consistency point taken?
**	- were there any non-REDO operations?
**	- which transactions were in progress at the time of the crash?
**
**	    NOTE: this analysis is cumulative; at completion we have full
**		  analysis information for all the nodes, including a unified
**		  list of all open transactions on all nodes.
**
**	Re-populate the logging system by calling LGadd and LGbegin to
**	create new transaction handles for each transaction that was open at
**	the time of the crash. Each such transaction is associated with the
**	same node it was associated with at the time of the crash.
**
**	For each node, call the recovery routines to perform the REDO pass:
**	- position this node's logfile to the point where REDO starts (the
**	  last consistency point, or perhaps later if the databases have been
**	  opened and closed since the last consistency opint).
**	- begin reading the logfile forwards.
**	- for each record, if the analysis pass indicated that this record needs
**	  to be redone, then call the DMVE routines to redo it. For a database
**	  opened with Fast Commit, this generally means that all operations
**	  need to get redone, whereas if Fast Commit is *not* in use, then only
**	  the operations of uncommitted transactions need to get redone.
**
**	Finally, perform a unified UNDO pass, using this loop:
**	- search down the unified list of all open transactions and locate the
**	  one that has the greatest LSN.
**	- Position that node's logfiles to that record (by LGA) and read it.
**	- Call DMVE to undo this operation.
**	- Update this transaction's next (previous) LSN and LGA
**
**	Even more finally, we update each log header to LGH_OK and finish.The
**	RCP will come up, find no recovery required, write consistency 
**	points and turn the logging system on-line.
**
**	Almost true: we mark our local log header to LGH_VALID, not LGH_OK,
**	meaning the log has been recovered and is open for business.
**
**	    NOTES:
**		=> When DMVE calls wish to perform CLR logging, LGwrite will
**		    be called. It knows how to write to the appropriate
**		    logfile(s) without using logwriter threads and without Group
**		    Commit, but with full log record blocking and spanning
**		    services.
**		=> We don't write the abort records for individual transactions
**		    when we encounter their BT's; rather we wait til the end
**		    of the full UNDO pass and then write all the abort records.
**		=> No locking is performed during this UNDO pass. The entire
**		    installation is offline at this time.
**
** Inputs:
**
** Outputs:
**	local_log_recovered	TRUE if recovery was done on local log.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      02-jul-1987 (Derek)
**          Created for Jupiter.
**	5-oct-1990 (bryanp)
**	    Added cluster dual logging support.
**	16-feb-1993 (bryanp)
**	    6.5 Cluster project:
**	    completely rewritten.
**	11-mar-1993 (keving)
**	    => We only build recovery context from logs that
**	    => have appropriate contents. Logic stolen from
**	    => dmr_recover
**	26-apr-1993 (keving)
**	    Log file recovery rewritten to closely mimic the structure /
**	    algorithms used by the rcp when starting up.
**	23-aug-1993 (bryanp)
**	    Change default DMF buffer cache size from 256 buffers to 1024
**		buffers.
**	    Allow DMF buffer cache size to be settable via the
**		II_CSP_DMF_CACHE_SIZE variable.
**      06-mar-1996 (stial01)
**          Variable Page size project: fix call to dm0p_allocate
**	14-May-1998 (jenjo02)
**	    Add per-cache WriteBehind parameter to dm0p_allocate() call.
**	12-may-2001 (devjo01)
**	    Allocation of local page buffers no longer required, since
**	    the CSP is now part of the RCP.
**	31-Mar-2004 (jenjo02)
**	    Modified to be sensitive to local log, which, for one,
**	    has already been opened by startup, and which must be
**	    marked LGH_VALID rather than LGH_OK when recovery completes.
**	    Returns local_log_recovered = TRUE if local log needed
**	    recovery.
*/
static STATUS
csp_restart(bool *local_log_recovered, i4 *errcode)
{
    CSP			*csp = &csp_global;
    LG_HEADER		*header;
    STATUS		status = E_DB_ERROR;
    STATUS		local_status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    i4		i;
    i4		j;
    i4		actual_nodes;
    RCP			*rcp;
    i4		length;
    LG_LA		bcp_la;
    LG_LSN		bcp_lsn;
    i4		node_id;
    DMP_LCTX		*lctx_array[CX_MAX_NODES];
    DMP_LCTX		*lctx, *local_lctx;
    LG_HDR_ALTER_SHOW	alter_parms;
    CX_NODE_INFO	*node_info;
    i4			dm0l_flags;
    DB_ERROR		local_dberr;

    for ( ; /* something to break out off */ ; )
    {
	/*
	**	For each non-local node, call LGopen to:
	**	- open that node's log file
	**	- open that node's dual log file, if it exists
	**	- perform restart logfile analysis (locate EOF, 
	**	  repair partially-written blocks, etc.)
	**	- return a handle for logging operations for this node.
	**
	** In the DMP_LCTX, we record the cluster node id which designates this
	** node. This node ID number will be used by the recovery and archiving
	** code to perform operations such as locating the correct bit in the
	** dsc_open_count bitmask, accessing the proper node journal file, etc.
	**
	** The local log has already been LGopen'd by startup and we
	** don't want to repeat that, though we will get our own 
	** LCTX which will reference the local log structures.
	*/
	*errcode = csp_grab_recovery_lock();
	if ( *errcode )
	    break;

	/* Gather recovery set for locks */
    	*errcode = CXstartrecovery( 0 );
	if ( *errcode )
	    break;

	local_lctx = NULL;

	for ( actual_nodes = 0;
	      actual_nodes < csp_config->cx_node_cnt;
	      actual_nodes++ )
	{
	    node_info =
	     &csp_config->cx_nodes[csp_config->cx_xref[actual_nodes+1]];

	    TRdisplay(
	     "%@ CSP-R0: Obtaining Recovery Context for node %d (\"%t\").\n",
	       node_info->cx_node_number, node_info->cx_node_name_l,
	       node_info->cx_node_name );

	    /*
	    ** If local log, it's already been opened by dmr_rcp_init()
	    ** We'll get our own LCTX which uses this process's
	    ** log handle (lctx_lgid) to reference the correct LG
	    ** structures for the local log, ensuring that when we
	    ** LGalter the header, the local header known to this 
	    ** process will the the one altered.
	    */
	    if ( node_info->cx_node_number == csp->csp_self_node )
	        dm0l_flags = DM0L_RECOVER | DM0L_NOLOG;
	    else
	        dm0l_flags = DM0L_RECOVER;

	    local_status = dm0l_allocate(dm0l_flags, 0, 0,
		node_info->cx_node_name,
		node_info->cx_node_name_l,
		&lctx_array[actual_nodes],
		&local_dberr);
	    if (local_status != E_DB_OK)
	    {
		*errcode = local_dberr.err_code;
		break;
	    }

	    lctx_array[actual_nodes]->lctx_node_id = node_info->cx_node_number;

	    /* Remember local log's lctx */
	    if ( node_info->cx_node_number == csp->csp_self_node )
		local_lctx = lctx_array[actual_nodes];
	}
	if (local_status)
	    break;

	/*
	**	Notify DMF buffer manager, of ALL the node logfile
	**	handles.
	*/

	local_status = dm0p_register_loghandles( lctx_array,
						 actual_nodes, &local_dberr);
	if (local_status)
	{
	    *errcode = local_dberr.err_code;
	    break;
	}

	for (;;)
	{

	    local_status = csp_recover_logs(lctx_array, RCP_R_CSP_STARTUP,
					    actual_nodes, errcode);
	    if (local_status != E_DB_OK)
		break;

	    /* 
	    ** Each log has now been successfully recovered. For each log file 
	    ** update header danger points and change status to LGH_OK 
	    ** for all logs but our local log; that one must be set
	    ** to LGH_VALID (recovered and ready for business).
	    */

	    for (i = 0; i < actual_nodes; i++)
	    {
		lctx = lctx_array[i];
		
		TRdisplay("%@ CSP-R0: Marking Log File status %s for node %d.\n",
			    (lctx == local_lctx) ? "VALID" : "OK",
			    lctx->lctx_node_id);

		alter_parms.lg_hdr_lg_id = lctx->lctx_lgid;
		local_status = LGshow(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
			    sizeof(alter_parms), &length, &sys_err);
		if (local_status != OK)
		{
		    *errcode = E_DM9017_BAD_LOG_SHOW;
		    break;
		}

		if ( lctx == local_lctx )
		{
		    /* Inform caller if local recovery done */
		    *local_log_recovered = 
			(lctx->lctx_status & LCTX_WAS_RECOVERED)
					? TRUE : FALSE;
		    alter_parms.lg_hdr_lg_header.lgh_status = LGH_VALID;
		}
		else
		    alter_parms.lg_hdr_lg_header.lgh_status = LGH_OK;

		if (local_status = LGalter(LG_A_NODELOG_HEADER,
				    (PTR)&alter_parms, sizeof(alter_parms),
				    &sys_err))
		{
		    _VOID_ uleFormat(NULL, local_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, errcode, 0);
		    break;
		}
		local_status = LGforce(LG_HDR, lctx->lctx_lxid, 0, 0, &sys_err);
		if (local_status != OK)
		{
		    _VOID_ uleFormat(NULL, local_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, errcode, 0);
		    _VOID_ uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, errcode, 1,
			0, lctx->lctx_lxid);
		    break;
		}
	    }
	    break;
	}

	for (i = 0; local_status == E_DB_OK && i < actual_nodes; i++)
	{
	    local_status = dm0l_deallocate(&lctx_array[i],
				    &local_dberr);
	}
	if (local_status)
	{
	    *errcode = local_dberr.err_code;
	    break;
	}

	/* Clear recovery set for locks */
	*errcode = CXfinishrecovery( 0 );
	if (*errcode != E_DB_OK)
	{
	    break;
	}

	*errcode = csp_release_recovery_lock();
	if (*errcode != E_DB_OK)
	{
	    break;
	}

	/*	Restart recovery complete. */

	TRdisplay("%@ CSP-R0: Restart Recovery is complete.\n");

	uleFormat(NULL, I_DM9934_CSP_RESTART_RECOV_COMPL, NULL, ULE_LOG,
	    NULL, NULL, 0L, NULL, &local_status, 0);
	status = E_DB_OK;
	break;
    }

    if ( status != E_DB_OK )
	crash( *errcode, &sys_err );
    return (status);
}


/*
** Name: csp_archive -- perform archiving for a node's logfile.
**
** Description:
**	This routine is called by the CSP process to perform archiving
**	after a node or system failure
**
** Inputs:
**	lctx			- logfile context block
**
** Outputs:
**	err_code		- reason for error completion
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	26-apr-1993 (bryanp)
**	    Created for 6.5 Cluster project.
**	28-mar-1994 (bryanp) B59975
**	    Remove node_id from the dma_prepare call (the lctx carries this
**		information itself).
**	    Remove node_id as an argument to csp_archive, since it was only
**		there in order to be passed through to dma_prepare.
*/
static DB_STATUS
csp_archive(DMP_LCTX *lctx, i4 *err_code)
{
    ACB			*acb;
    ACB			*a;
    DB_STATUS		complete_status;
    i4		complete_err_code;
    STATUS		status;
    DB_ERROR		dberr;

    CLRDBERR(&dberr);

    /*
    ** Allocate and initialize the acb, init the  Archiver database.
    */
    status = dma_prepare(DMA_CSP, &acb, lctx, &dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	TRdisplay("%@ Error in CSP archiving -- Can't prepare Archiver.\n");

	*err_code = E_DM9851_ACP_INITIALIZE_EXIT; /* FIX -- need new msg */ 
	return(E_DB_ERROR);
    }

    a = acb;

    for (;;)
    {
	/*
	** Build archive context by analyzing this node's log file.
	*/
	status = dma_offline_context(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    TRdisplay("%@ CSP CSP_ARCHIVE: Archive Cycle Failed!\n");
	    *err_code = E_DM9851_ACP_INITIALIZE_EXIT; /* FIX -- need new msg */ 
	    break;
	}

	/*
	** Get config file information for each database, open journal 
	** and dump files, skip over CP and position just past it.
	*/
	status = dma_soc(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    TRdisplay("%@ CSP CSP_ARCHIVE: Archive Cycle dma_soc Failed!\n");
	    *err_code = E_DM9851_ACP_INITIALIZE_EXIT; /* FIX -- need new msg */ 
	    break;
	}

	/*
	** Copy the log to the journal and dump files.
	*/
	status = dma_archive(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    TRdisplay("%@ CSP CSP_ARCHIVE: Archive dma_archive Failed!\n");
	    *err_code = E_DM9851_ACP_INITIALIZE_EXIT; /* FIX -- need new msg */ 
	    break;
	}

	/*
	** Clean up at end of cycle.
	*/
	status = dma_eoc(a);
	if (status != E_DB_OK)
	{
	    uleFormat(&a->acb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    TRdisplay("%@ CSP CSP_ARCHIVE: Archive Cycle dma_eoc Failed!\n");
	    *err_code = E_DM9851_ACP_INITIALIZE_EXIT; /* FIX -- need new msg */ 
	    break;
	}

	TRdisplay("%@ CSP: Archive Cycle Completed Successfully.\n");

	break;
    }

    /*	Complete archive processing. */

    complete_status = dma_complete(&a, &dberr);
    if (complete_status != E_DB_OK)
    {
	uleFormat(&dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &complete_err_code, 0);

	TRdisplay("%@ Error terminating ACP -- Can't complete Archiver.\n");

	if ( status == E_DB_OK )
	{
	    status = complete_status;
	    *err_code = dberr.err_code;
	}
    }

    return (status);
}

/*{
** Name: csp_node_fail	- Handle node failure.
**
** Description:
**      This routine handles node failure recovery.
**
**	When a cluster node fails, the master node's CSP performs recovery of
**	the failed node's operations. Node failure recovery is conceptually
**	similar to restart recovery, with several major differences:
**
**	    - the installation is "online" (although some operations are
**		stalled) during node failure recovery.
**
**	    - only the operations from the failed node need to be recovered.
**
**	Node failure recovery uses the following algorithm:
**
**	    => Compute the set of nodes requiring recovery.
**
**	    => For each node requiring recovery, open that node's logfile (and
**		that node's dual log file, if it exists). Perform restart
**		analysis of the logfile, locating the EOF, repairing partially-
**		written blocks, etc. Locate the last CP in that node's logfile.
**
**	// following no longer true as of s103715.
**
**	    => Allocate a DMF buffer manager, notifying it of all the node
**		logfile handles.
**
** Inputs:
**
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
**	19-apr-1993 (bryanp)
**	    New version for 6.5 Cluster Recovery.
**	23-aug-1993 (bryanp)
**	    Change default DMF buffer cache size from 256 buffers to 1024
**		buffers.
**	    Allow DMF buffer cache size to be settable via the
**		II_CSP_DMF_CACHE_SIZE variable.
**	28-mar-1994 (bryanp) B60058
**	    Disable MSG_T_RESUME_PHYSICAL_LOCKS until we better understand how
**		to manage the locking semantics during node failure recovery.
**		This broadcast message was only partially implemented, anyway,
**		and when it occurred it was causing the CSP state machine to
**		crash.
**      06-mar-1996 (stial01)
**          Variable Page Size project: fix arguments to dm0p_allocate
**	21-aug-1996 (boama01)
**	    Inhibit new lock rqsts from being AST-delivered while in
**	    stalled-lock-completion phase of node recovery.  See module header
**	    history for details.
**	21-may-2001 (devjo01)
**	    Changes to allow this to be part of RCP. (s103715).
**	22-Apr-2005 (fanch01)
**	    Added dbms class failover.  On node failure the set of classes
**	    that no longer exist in the cluster are started via message
**	    broadcast.  Recovered dbms classes are distributed round robin
**	    fashion starting at the lowest surviving node number.
*/
static STATUS
csp_node_fail(i4 *err_code)
{
    CSP			*csp = &csp_global;
    i4			 length;
    i4			 i;
    STATUS		 status, local_status;
    CL_ERR_DESC		 sys_err;
    i4			 actual_nodes;
    DMP_LCTX		*lctx_array[CX_MAX_NODES];
    DMP_LCTX		*lctx;
    LG_HDR_ALTER_SHOW	 alter_parms;
    CX_NODE_INFO	*cx_node_info;
    CSP_NODE_INFO	*csp_node_info;
    CSP_CX_MSG		 msg;
    u_i1		 node_survivor_array[CX_MAX_NODES];
    u_i1		 node_dead_array[CX_MAX_NODES];
    i4			 node_survivor_count = 0;
    i4			 node_dead_count = 0;
    DB_ERROR		 local_dberr;

    status = E_DB_FATAL;

    uleFormat(NULL, I_DM9931_CSP_PERF_NODE_RECOV, NULL, ULE_LOG, NULL, NULL,
	0L, NULL, &local_status, 0);

    msg.csp_msg_action = CSP_MSG_3_RCVRY_CMPL;
    msg.csp_msg_node = (u_i1)csp->csp_self_node;
    CX_INIT_NODE_BITS(msg.rcvry.node_bits);

    for ( ; /* Something to break out of */ ; )
    {
	local_status = csp_grab_recovery_lock();
	if (E_DB_OK != local_status)
	{
	    *err_code = local_status;
	    break;
	}

	/* Gather recovery set for locks */
	local_status = CXstartrecovery( 0 );
	if (E_DB_OK != local_status)
	{
	    *err_code = local_status;
	    break;
	}

	/*  Compute the set of nodes needing recovery. */
	for ( actual_nodes = i = 0;
	      i < csp_config->cx_node_cnt; 
	      i++ )
	{
	    /* Find next node to work on. */
	    cx_node_info = &csp_config->cx_nodes[csp_config->cx_xref[i+1]];
	    if ( !csp->csp_node_info[cx_node_info->cx_node_number-1].
		 node_failure )
	    {
		node_survivor_array[node_survivor_count++] =
		    cx_node_info->cx_node_number;
		continue;
	    }
	    node_dead_array[node_dead_count++] = cx_node_info->cx_node_number;

	    uleFormat(NULL, I_DM9932_CSP_RECOV_NODE, NULL, ULE_LOG, NULL, NULL,
		0L, NULL, &local_status, 1, cx_node_info->cx_node_name_l,
		cx_node_info->cx_node_name);
	    CX_SET_NODE_BIT(msg.rcvry.node_bits, \
	     cx_node_info->cx_node_number);

	    /*  Open the log file for next node. */
	    local_status = dm0l_allocate(DM0L_RECOVER, 0, 0,
				cx_node_info->cx_node_name,
				cx_node_info->cx_node_name_l,
				(DMP_LCTX **)&lctx_array[actual_nodes],
				&local_dberr);
	    if (local_status != E_DB_OK)
	    {
		*err_code = local_dberr.err_code;
		break;
	    }

	    /*
	    ** In the DMP_LCTX, we record the cluster node ID which designates
	    ** this node. This node ID number will be used by the recovery and
	    ** archiving routines to perform operations such as locating the
	    ** correct bit in the dsc_open_count bitmask, accessing the proper
	    ** node journal file, etc.
	    */
	    lctx_array[actual_nodes]->lctx_node_id =
	     cx_node_info->cx_node_number;

	    actual_nodes++;
	}

	if (local_status != E_DB_OK)
	    break;

	if (0 == actual_nodes)
	{
	    /* Logical inconsistency? */
	    REPORT_ODDITY(ERx("csp_node_fail called, but no nodes failed"));
	}

	local_status = dm0p_register_loghandles( lctx_array, actual_nodes,
						 &local_dberr );
	if (local_status != E_DB_OK)
	{
	    *err_code = local_dberr.err_code;
	    break;
	}

	local_status = csp_recover_logs(lctx_array, RCP_R_CSP_ONLINE,
					actual_nodes, err_code);
	if (local_status != E_DB_OK)
	    break;

	/* 
	** Each log has now been successfully recovered. For each log file 
	** update header danger points and change status to LGH_OK 
	*/

	for (i = 0; i < actual_nodes; i++)
	{
	    lctx = lctx_array[i];
	    alter_parms.lg_hdr_lg_id = lctx->lctx_lgid;
	    local_status = LGshow(LG_A_NODELOG_HEADER, (PTR)&alter_parms,
		    sizeof(alter_parms), &length, &sys_err);
	    if (local_status != OK)
	    {
                *err_code = E_DM9017_BAD_LOG_SHOW;
		break;
	    }

	    TRdisplay("%@ Ingres/CSP marking Log File status OK for node %d.\n",
			    lctx->lctx_node_id);

	    alter_parms.lg_hdr_lg_header.lgh_status = LGH_OK;

	    if (local_status = LGalter(LG_A_NODELOG_HEADER,
				(PTR)&alter_parms, sizeof(alter_parms),
				&sys_err))
	    {
		_VOID_ uleFormat(NULL, local_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		*err_code = E_DM900B_BAD_LOG_ALTER;
		break;
	    }

	    local_status = LGforce(LG_HDR, lctx->lctx_lxid, 0, 0, &sys_err);
	    if (local_status != OK)
	    {
		_VOID_ uleFormat(NULL, local_status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		_VOID_ uleFormat(NULL, E_DM9010_BAD_LOG_FORCE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 1,
		    0, lctx->lctx_lxid);
		*err_code = 0; /* to prevent caller from logging err twice */ 
		break;
	    }

	    local_status = dm0l_deallocate(&lctx_array[i], &local_dberr);
	    if (local_status != OK)
	    {
		*err_code = local_dberr.err_code;
		break;
	    }
	}

	if (local_status != E_DB_OK)
	    break;

	/* Clear recovery set for locks */
	local_status = CXfinishrecovery( 0 );
	if (local_status != E_DB_OK)
	{
	    *err_code = local_status;
	    break;
	}

	/*
	** Tell everyone recovery is complete.  Other CSP's will release
	** their DMN locks for the failed node(s), allowing the failed
	** node(s) to start up again.
	*/
	local_status = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&msg );
	if (local_status != OK)
	{
	    (VOID) uleFormat(NULL, local_status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    *err_code = E_DM991E_CSP_BAD_BROADCAST;
	    break;
	}
	local_status = csp_release_recovery_lock();
	if (E_DB_OK != local_status)
	{
	    *err_code = local_status;
	    break;
	}

	/*
	** If current node is the new master node.  Master node
	** is responsible for recovery of dbms class servers.
	**
	** Send a dbms class start message to other nodes
	** to restore functionality.
	*/
	if (csp_global.csp_master_node == csp_global.csp_self_node)
	{
	    /* Bring up dead nodes round robin on survivors, starting at first survivor */
	    i4 node_survivor_next = 0;
	    CSP_NODE_CLASS_ROSTER *r =
		&csp->csp_node_info[csp->csp_self_node-1].class_roster;

	    for ( i = 0;
		  i < node_dead_count;
		  i++ )
	    {
		/* Find next node to work on. */
		cx_node_info = &csp_config->cx_nodes[node_dead_array[i] - 1];

		uleFormat(NULL, I_DM9933_CSP_RECOV_DBMS_CLASS, NULL, ULE_LOG,
		    NULL, NULL, 0L, NULL, &local_status, 1,
		    cx_node_info->cx_node_name_l, cx_node_info->cx_node_name);

		if (CX_GET_NODE_BIT(msg.rcvry.node_bits,
				    cx_node_info->cx_node_number))
		{
		    QUEUE *q, *nq, *e, *ne;
		    /* node was on failing list, see if classes need to be restarted */

		    /* find first node class entry */
		    for( q = r->classes.queue.q_next;
			 q != &r->classes.queue &&
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node <
			 cx_node_info->cx_node_number;
			 q = q->q_next )
		    {
			/* nothing */
		    }

		    /*
		    ** for each class entry
		    ** see if it's elsewhere in the cluster
		    */
		    for( /* nothing */;
			q != &r->classes.queue &&
			((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node ==
			cx_node_info->cx_node_number;
			q = nq )
		    {
			/* scan the entire class roster for have >0 */
			i4 started = 0;
			nq = q->q_next;
			for( e = r->classes.queue.q_next;
			     e != &r->classes.queue;
			     e = ne )
			{
			    ne = e->q_next;
			    if (!STncmp(
				((CSP_NODE_CLASS_INFO_QUEUE *) e)->info.name,
				((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.name,
				CSP_MSG_CLASS_BYTES))
			    {
				if (e != q)
				{
				    if (((CSP_NODE_CLASS_INFO_QUEUE *) e)->info.count_have == 0)
				    {
					csp_class_roster_class_destroy(
					    (CSP_NODE_CLASS_INFO_QUEUE *) e);
				    }
				    else 
				    {
					started +=
					    ((CSP_NODE_CLASS_INFO_QUEUE *) e)->info.count_have;
				    }
				}
			    }
			}
			if (started == 0)
			{
			    csp_dbms_remote_start(
				((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.name,
                                node_survivor_array[node_survivor_next++]);
			    node_survivor_next %= node_survivor_count;
			}
			csp_class_roster_class_destroy(
			    (CSP_NODE_CLASS_INFO_QUEUE *) q);
			r->class_count--;
		    }
		}
	    }
	}

	uleFormat(NULL, I_DM9935_CSP_NODE_RECOV_COMPL, NULL, ULE_LOG,
	    NULL, NULL, 0L, NULL, &local_status, 0);
	status = OK;
	break;
    }
    return (status);
} /* csp_node_fail */


/*{
** Name: opc_send	- Send message to CSP operator & trace log.
**
** Description:
**      This routine sends a message to the CSP operator.
**
** Inputs:
**	arg			Needed to match TR calling convention.
**      length			Length of char stream to send.
**	buffer			Pointer to message to send.
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
**      02-jun-1987 (Derek)
**          Created for Jupiter.
**	21-jun-1993 (bryanp)
**	    Trace operator console messages to iicsp.log.
**	21-may-2001 (devjo01)
**	    Use ERsend to write to console.
*/

static i4
opc_send(
PTR		    arg,
i4		    length,
char		    *buffer)
{
    CL_ERR_DESC		sys_err;

    TRdisplay("%@ :'%t'\n", length, buffer);
    ERsend(ER_ERROR_MSG, buffer, length, &sys_err ); 
# ifdef	VMS
    /* VMS supports a true and separate operators console */
    ERsend(ER_OPER_MSG, buffer, length, &sys_err ); 
# endif
    return OK;
}


/*{
** Name: crash	- Fatal error handler.
**
** Description:
**      This routine is called when a fatal error has been detected.  The
**	message parameter is used to lokkup the text of the failure message,
**	the the status parameter is used to lokkup an assocaited VMS
**	error code text.  A dump of the CSP database is also written to the
**	CSP log.
**
** Inputs:
**	message			    Error message number.
**	status			    VMS status code.
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
**      08-jun-1987 (Derek)
**          Created for Jupiter.
**	24-may-89 (jrb)
**	    Changed ERlookup to conform to latest interface.
**	26-oct-92 (andre)
**	    replaced call to ERlookup() with a call to ERslookup()
**	26-jul-1993 (bryanp)
**	    Check the return code from ERslookup -- if it fails, don't try to
**		display the formatted message.
*/
static VOID
crash(
i4	    	 message,
CL_ERR_DESC 	*status)
{
    CL_ERR_DESC	    err_code;
    char	    buffer[ER_MAX_LEN];
    i4	    buf_len;
    STATUS	    lookup_status;

    TRdisplay("EXIT....... Reason %x\n", message);
    lookup_status = ERslookup(message, status, 0, (char *) NULL,
		buffer, sizeof(buffer), -1,
	      &buf_len, &err_code, 0, (ER_ARGUMENT *)0);
    if (lookup_status == OK)
    {
	TRdisplay("%@ CSP FAILURE\n\tReason - %t\n", buf_len, buffer);
    }
    else
    {
	TRdisplay("%@ CSP FAILURE\n");
	TRdisplay("   (crash error %x looking up msg %x status %x)\n",
		    lookup_status, message, CL_GET_OS_ERR(status));
    }
    dumpall();
    PCexit(OK);
}

static VOID
dumpall(VOID)
{
    CSP		*csp = &csp_global;

    TRdisplay("%20*=%@ CSP DUMP%20*=\n");
#if 0 /* pre-s103715 */
//
//    TRdisplay("    Node: %s    Status = %v   State = %w\n",
//	CXnode_name(NULL),
//	"MASTER,ACP_FAILED,JOIN,RECOVER,4,5,6,7,8,9,10,11,EXIT,RUN,INTER,ACTION", csp->csp_status,
//	"0,START,RECOVER,START_NEXT,JOIN,JOIN_READY,ADD,ADD_READY,RUN,RUN_JOIN,LEAVE,STOP,LEAVE_DELAY",
//	csp->csp_state);
//    TRdisplay("    Nodes:    %2d    Max_nodes: %2d    Node_self: %2d  Master Node: %2d\n",
//	csp->csp_nodes, csp->csp_max_nodes, csp->csp_node_self, csp->csp_master_node);
//    TRdisplay("    Add node: %2d    Adds sent: %2d    Next_node: %2d  Join_node:   %2d\n",
//	csp->csp_addnode, csp->csp_add_nodes,	csp->csp_next_node, csp->csp_join_node);
//    TRdisplay("    DECnet channel: %x  Event channel: %x\n",
//	csp->csp_decnet_chan, csp->csp_connect_chan);
//    TRdisplay("    Maximum Message allocated: %d\n", csp->csp_max_msg);
//    TRdisplay("    Message hit: %d\n", csp->csp_msg_hit);    
//    TRdisplay("    Oldest message sent: %d\n", csp->csp_odst_msg);    
//    TRdisplay("    Message cache full: %d\n", csp->csp_round);    
//    TRdisplay("    Message Sequence: %d\n", csp->csp_msg_sequence);
//    TRdisplay("%70*-\n");
//    {
//	ACTION		*a = csp->csp_ai_next;
//	ACTION		*end = &csp->csp_ai_next;
//
//	for (; a != end; a = a->act_next)
//	{
//	    TRdisplay("    Action Type=%w\n",
//		"0,RESTART,NODE_FAIL,SHUTDOWN,START", a->act_type);
//	}
//    }
//    TRdisplay("%70*-\n");
//    {
//	MESSAGE		*m = csp->csp_mf_next;
//	MESSAGE		*end = &csp->csp_mf_next;
//	i4		fcount;
//	i4		bcount;
//	i4		wcount;
//
//	for (fcount = 0; m != end; m = m->msg_next, fcount++)
//	    ;
//
//	for (bcount = 0, m = csp->csp_mb_next, end = &csp->csp_mb_next;
//	    m != end;
//	    m = m->msg_next, bcount++)
//	{
//	    TRdisplay("   BUSY Seq:%d Source:%d Dest:%d Type=%w Action=%w\n",
//		m->msg_sequence, m->msg_source, m->msg_dest,
//	",START,JOIN,JOIN_ACK,JOIN_NAK,ADD,ADD_ACK,ADD_NAK,CONNECT,CONNECT_ACK,CONNECT_NAK\
//,RUN,RESET,MEMBER,MEMBER_ACK,MEMBER_NAK,DEADLOCK", 
//		m->msg_type,
//	"0,START,S_CONNECT,S_NEXT,S_ACCEPT,S_REJECT,S_RESTART,S_START,\
//S_MASTER,9,10,11,JOIN,J_SEND,J_ERROR,J_RECEIVE,\
//ADD,A_SEND,A_RECEIVE,A_ERROR,20,21,22,23,\
//AA_SEND,AA_RECEIVE,AA_ERROR,AN_SEND,AN_RECEIVE,AN_ERROR,30,31,\
//JA_SEND,JA_RECEIVE,JA_ERROR,JN_SEND,JN_RECEIVE,JN_ERROR,38,39,\
//CONNECT,C_SEND,C_RECEIVE,C_ACCEPT,C_REJECT,45,46,47,\
//LEAVE,L_RESTART,L_NODE,L_MASTER,52,53,54,55,\
//RUN,R_SEND,R_RECEIVE,R_ERROR,RESET,RE_SEND,RE_RECEIVE,RE_ERROR,\
//M_SEND,M_RECEIVE,M_ERROR,MA_SEND,MA_RECEIVE,MA_ERROR,MN_SEND,MN_RECEIVE,\
//MN_ERROR,73,74,75,D_RECEIVE,D_SEND,78,79,\
//READ,D_READ,DISCONNECT,CANCEL,84,85,86,87,TIMER_COMPLETE,LEAVE_DELAY,90,91,\
//92,93,94,95",
//		m->msg_action);
//	}
//	for (wcount = 0, m = csp->csp_mw_next, end = &csp->csp_mw_next;
//	    m != end;
//	    m = m->msg_next, wcount++)
//	{
//	    TRdisplay("   WAIT Seq:%d Source:%d Dest:%d Type=%w Action=%w\n",
//		m->msg_sequence, m->msg_source, m->msg_dest,
//	",START,JOIN,JOIN_ACK,JOIN_NAK,ADD,ADD_ACK,ADD_NAK,CONNECT,CONNECT_ACK,CONNECT_NAK\
//,RUN,RESET,MEMBER,MEMBER_ACK,MEMBER_NAK,DEADLOCK", 
//		m->msg_type,
//	"0,START,S_CONNECT,S_NEXT,S_ACCEPT,S_REJECT,S_RESTART,S_START,\
//S_MASTER,9,10,11,JOIN,J_SEND,J_ERROR,J_RECEIVE,\
//ADD,A_SEND,A_RECEIVE,A_ERROR,20,21,22,23,\
//AA_SEND,AA_RECEIVE,AA_ERROR,AN_SEND,AN_RECEIVE,AN_ERROR,30,31,\
//JA_SEND,JA_RECEIVE,JA_ERROR,JN_SEND,JN_RECEIVE,JN_ERROR,38,39,\
//CONNECT,C_SEND,C_RECEIVE,C_ACCEPT,C_REJECT,45,46,47,\
//LEAVE,L_RESTART,L_NODE,L_MASTER,52,53,54,55,\
//RUN,R_SEND,R_RECEIVE,R_ERROR,RESET,RE_SEND,RE_RECEIVE,RE_ERROR,\
//M_SEND,M_RECEIVE,M_ERROR,MA_SEND,MA_RECEIVE,MA_ERROR,MN_SEND,MN_RECEIVE,\
//MN_ERROR,73,74,75,D_RECEIVE,D_SEND,78,79,\
//READ,D_READ,DISCONNECT,CANCEL,84,85,86,87,TIMER_COMPLETE,LEAVE_DELAY,90,91,\
//92,93,94,95",
//		m->msg_action);
//	}
//
//	TRdisplay("\n    Free: %d Busy: %d Wait: %d\n",
//	    fcount, bcount, wcount);
//    }
//    TRdisplay("%70*-\n");
//    {
//	i4		i;
//
//	for (i = 0; i < CX_MAX_NODES; i++)
//	{
//	    if (csp->csp_node[i].node_status == 0)
//		continue;
//
//	    TRdisplay("  Node: %t - %t %2d Channel: %x Csid: %x Status: %v\n",
//		sizeof(csp->csp_node[i].cx_node_name), csp->csp_node[i].cx_node_name,
//		sizeof(csp->csp_node[i].node_cnode), 
//                csp->csp_node[i].node_cnode,
//		i, csp->csp_node[i].node_channel, csp->csp_node[i].node_csid,
//		"AVAIL,JOIN,MEMBER,CONNECT,FAIL", 
//                csp->csp_node[i].node_status);
//	}
//    }
//    TRdisplay("%70*-\n");
//    {
//	i4		i;
//
//	for (i = 0; i < CSP_CACHE_SIZE; i++)
//	{
//	    if ((csp->csp_msg_cache[i].node_id & 1) == 0)
//		continue;
//	    TRdisplay(" Node: %x orig_tran: <%x,%x>, stamp: %d tran: <%x,%x>\n",
//			csp->csp_msg_cache[i].node_id,
//			csp->csp_msg_cache[i].o_tran_id.lk_uhigh,
//			csp->csp_msg_cache[i].o_tran_id.lk_ulow,
//			csp->csp_msg_cache[i].o_stamp,
//			csp->csp_msg_cache[i].tran_id.lk_uhigh,
//			csp->csp_msg_cache[i].tran_id.lk_ulow);
//	}
//    }
#endif /* pre-s103715 */
    TRdisplay("%70*=\n");
}


/*{
** Name: csp_ex_handler	- CSP exception handler.
**
** Description:
**	Exception handler for Cluster Server Process.  This handler is
**	declared at CSP startup.  All exceptions which come through here will
**	cause the CSP to shutdown.
**
**	An error message including the exception type and PC (essentially
**	the VMS status message) will be written to the log file(s)
**	before exiting.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	EXDECLARE causes us to return execution to csp_initialize routine,
**	at the point of the EXdeclare() call.
**
** History:
**	 21-aug-1996 (boama01)
**	    Copied exception handler from DMFACP.C; modified for CSP log
**	    reporting, and to sense recursive exception handling.
*/
static STATUS
csp_ex_handler(
EX_ARGS	    *ex_args)
{

    /*
    ** If the global exception switch is set, we received another exception
    ** while attempting to report on the first.  In this case, do not attempt
    ** further processing and do not return; simply report and terminate.
    ** (This logic is similar to that in crash()).
    */
    if (exception_noted)
    {
	TRdisplay("%@ CSP RECURSIVE EXCEPTION HANDLED! Exiting immediately.\n");
	PCexit(ex_args->exarg_num);
    }
    else
    {
	char        err_msg[EX_MAX_SYS_REP];
	i4     err_code = E_DM9930_CSP_EXCEPTION_ABORT;
	exception_noted = 1;
# ifdef	VMS
	i4     astset = sys$setast(0);
# endif

	/*
	** Report the exception to the Ingres error log.
	*/
	ulx_exception( ex_args, DB_DMF_ID, err_code, TRUE );

	/*
	** Also report the error to the CSP log (just in case).  This logic is
	** similar to that used by ulx_exception() to format the system error
	** message.
	*/
	if (!EXsys_report( ex_args, err_msg ))
	    STcopy( ERx("(unable to format system message)"), err_msg );
	TRdisplay("%@ CSP SYSTEM EXCEPTION ENCOUNTERED:\n %s\n", err_msg);

# ifdef	VMS
	if (astset == SS$_WASSET) 
           sys$setast(1);
# endif
    }
    return (EXDECLARE);
}


/*{
** Name: csp_ckp_msg		- Process ckpdb message.
**
** Description:
**      Process ckpdb message.
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
**	12-Dec-2007 (jonj)
**	    Include ckpdb's node in LK_CKP_CLUSTER lock keys.
*/
static VOID
csp_ckp_msg(
CSP_CX_MSG	*ckp_msg)
{
    CSP		*csp = &csp_global;
    DB_STATUS	status;

    _VOID_ TRdisplay(
    ERx("%@ CPP %d %d::%x: %w(%d) Msg from node %d\n"),
	__LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	CKP_PHASE_NAME, ckp_msg->ckp.phase,
	ckp_msg->ckp.phase, 
	ckp_msg->csp_msg_node);

    switch (ckp_msg->ckp.phase)
    {
	case CKP_CSP_INIT:
	    /* On the local node, the dmfjsp ckpdb takes the STATUS lock */
	    if (csp->csp_self_node != ckp_msg->ckp.ckp_node)
	    {
		/*
		** Redeem long message using chit
		** Request LK_CKP_CLUSTER locks
		** Add database to logging system
		** NOTE if failure DO NOT call csp_ckp_error
		** We cannot send message from message handler
		** This creates a hang situation
		*/
		status = csp_ckp_msg_init(ckp_msg);
		if (status != E_DB_OK)
		{
		    /*
		    ** ckpdb will detect INIT error on the local node because 
		    ** the CKP_CSP_STATUS lock will not exist
		    ** ckpdb will detect INIT error on a remote node when the
		    ** next phase fails for this node.
		    */
		}
	    }
	    break;

	case CKP_CSP_STALL:
	case CKP_CSP_SBACKUP:
	case CKP_CSP_RESUME:
	case CKP_CSP_CP:
	case CKP_CSP_EBACKUP:
	case CKP_CSP_DBACKUP:

	    /*
	    ** Put checkpoint action...
	    ** On the local node, the dmfjsp ckpdb does the action
	    */
	    if (csp->csp_self_node != ckp_msg->ckp.ckp_node)
	    {
        
		TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Put action\n"),
		    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
		    CKP_PHASE_NAME, ckp_msg->ckp.phase,
		    ckp_msg->ckp.phase);

		csp_put_action(CSP_ACT_8_CKP, csp->csp_self_node, 0, ckp_msg);
	    }
	    break;

	case CKP_CSP_STATUS:

	    if (ckp_msg->ckp.ckp_var.status_rc == 0)
	    {
		/*
		** Checkpoint is done
		** Local node: release LK_CKP_CLUSTER CKP_CSP_STATUS lock
		** Remote node: nothing to do, phase locks already released
		*/
		if ( csp->csp_self_node == ckp_msg->ckp.ckp_node )
		    status = csp_ckp_lkrelease(ckp_msg, CKP_CSP_STATUS);
		return;
	    }
	    break;

	case CKP_CSP_ERROR:

	    /*
	    ** Some node had checkpoint failure
	    ** Only the local node processes this message
	    ** Process this message here in message handler 
	    ** while the caller still holds the phase lock
	    */
	    if ( csp->csp_self_node == ckp_msg->ckp.ckp_node )
	    {
		/* Local node, release all LK_CKP_CLUSTER CKP_CSP_STATUS lock */
		csp_ckp_lkrelease_all(ckp_msg);
	    }
	    break;

	case CKP_CSP_ABORT:

	    /*
	    ** A checkpoint error has been detected by ckpdb (dmfcpp)
	    ** (CKP_CSP_ERROR processing releases the 
	    ** LK_CKP_CLUSTER CKP_CSP_STATUS which triggers the CKP_CSP_ABORT
	    ** NOTE: we can get this message more than once, so this code
	    ** should assume nothing.
	    */
	    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) After phase %d\n"),
		    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
		    CKP_PHASE_NAME, ckp_msg->ckp.phase,
		    ckp_msg->ckp.phase,
		    ckp_msg->ckp.ckp_var.abort_phase);

	    if ( csp->csp_self_node != ckp_msg->ckp.ckp_node )
	    {
		/* Remote nodes may need to close db */
		csp_ckp_abort(ckp_msg);
	    }
	    /* Release LK_CKP_CLUSTER locks */
	    csp_ckp_lkrelease_all(ckp_msg);
	    break;

	default:
	    break;

	}

    return;
}


/*{
** Name: csp_ckp_action  Perform ckpdb action
**
** Description:
**	Perform ckpdb action
**
** Inputs:
**	ftx				Pointer to thread execution block
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      01-sep-2004 (stial01)
**          Created.
*/
static DB_STATUS
csp_ckp_action(SCF_FTX *ftx)
{
    CSP		*csp = &csp_global;
    CSP_CX_MSG  *ckp_msg;
    DB_STATUS	status = E_DB_OK;
    DB_STATUS	loc_status = E_DB_OK;
    DMF_JSX	jsx;
    i4          err_code;

    ckp_msg = (CSP_CX_MSG *)ftx->ftx_data;

    /* No action for local node */
    if (csp->csp_self_node == ckp_msg->ckp.ckp_node)
    {
	/* Release single LK_CKP_CLUSTER phase lock */
	status = csp_ckp_lkrelease(ckp_msg, ckp_msg->ckp.phase);
        return (E_DB_OK);
    }
    else
    {
	/* Build JSX context for this ckpdb */
	status = csp_ckp_init_jsx(ckp_msg, &jsx);
	if (status)
	{
	    csp_ckp_error(ckp_msg);
	    return (E_DB_ERROR);
	}

	status = online_cx_phase(ckp_msg->ckp.phase, &jsx);

        if (status != E_DB_OK)
	{
	    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Error checkpointing %~t\n"),
		__LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
		CKP_PHASE_NAME, ckp_msg->ckp.phase,
		ckp_msg->ckp.phase,
		sizeof(DB_DB_NAME), jsx.jsx_db_name.db_db_name);
	}

	if (status != E_DB_OK ||
	    ckp_msg->ckp.phase == CKP_CSP_DBACKUP)
	{
	    loc_status = csp_ckp_closedb(ckp_msg, jsx.jsx_log_id);
	    jsx.jsx_log_id = 0;
	    if (loc_status)
		status = E_DB_ERROR;
	}

	if (status)
	{
	    /* Send CKP_CSP_STATUS message, Release all LK_CKP_CLUSTER locks */
	    csp_ckp_error(ckp_msg);
	}
	else
	{
	    /* Release single LK_CKP_CLUSTER phase lock */
	    loc_status = csp_ckp_lkrelease(ckp_msg, ckp_msg->ckp.phase);
	}

	/* Send failure message (if we didn't already do so */
	if (loc_status != E_DB_OK && status == E_DB_OK)
	{
	    if (jsx.jsx_log_id)
		loc_status = csp_ckp_closedb(ckp_msg, jsx.jsx_log_id);
	    jsx.jsx_log_id = 0;

	    csp_ckp_error(ckp_msg);
	    status = E_DB_ERROR;
	}
    }

    return (status);

}

/*{
** Name: csp_ckp_msg_init - Process ckpdb CKP_CSP_INIT message.
**
** Description:
**      Process ckpdb CKP_CSP_INIT message.
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
**	21-Dec-2007 (jonj)
**	    No longer called for local node. STATUS lock is taken
**	    locally by the local ckpdb process.
*/
static DB_STATUS
csp_ckp_msg_init(
CSP_CX_MSG	*ckp_msg)
{
    CSP			*csp = &csp_global;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		loc_status;
    STATUS		cx_status = OK;
    STATUS		lk_status = OK;
    CL_ERR_DESC		sys_err;
    STATUS		loc_cl_status;
    CL_ERR_DESC		loc_sys_err;
    i4			err_code;
    i4			flags;
    i4			pnum;
    LK_LOCK_KEY		lockckey;
    LK_VALUE		lock_value;
    LK_LKID		lockid;
    i4			i;
    char                buf[sizeof(DB_DB_NAME) + sizeof(DB_OWN_NAME) + 
				sizeof(DMP_LOC_ENTRY) + 32 + 64];
    i4                  buflen;
    i4                  redeem_len;
    i4                  pid;
    char                *b;
    DB_DB_NAME		dbname;
    DB_OWN_NAME		dbowner;
    DMP_LOC_ENTRY	dblocation;
    i4			log_id = 0;
    bool		db_open = FALSE;
    i4			dcb_status;

    do
    {
	/* Redeem text using chit */
	buflen = sizeof(buf);
	cx_status = CXmsg_redeem( ckp_msg->ckp.ckp_var.init_chit, buf,
			       buflen, 0, &redeem_len ); 
	if ( cx_status )
	{
	    status = E_DB_ERROR;
	    break;
	}

	b = buf; /* callers message */

	/* extract dbname, dbowner, location from long message */
	MEcopy(b, sizeof(DB_DB_NAME), dbname.db_db_name);
	b += sizeof(DB_DB_NAME);
	MEcopy(b, sizeof(DB_OWN_NAME), dbowner.db_own_name);
	b += sizeof(DB_OWN_NAME);
	MEcopy(b, sizeof(DMP_LOC_ENTRY), (PTR)&dblocation);
	b += sizeof(DMP_LOC_ENTRY);
	MEcopy(b, sizeof(dcb_status), (PTR)&dcb_status);
	b += sizeof(dcb_status);

        /* extract ckpdb pid from long message */
	cx_status = CVal(b, &pid);
	if (cx_status)
	{
	    status = E_DB_ERROR;
	    break;
	}

	/* Remote nodes: Add database to logging system */
	status = csp_ckp_opendb(ckp_msg, 
		    &dbname, &dbowner, &dblocation, dcb_status, &log_id);
	if (status != E_DB_OK)
	    break;
	db_open = TRUE;

	/* Remote nodes: Request SHARE LK_CKP_CLUSTER phase locks */
	lockckey.lk_type = LK_CKP_CLUSTER;
	lockckey.lk_key1 = ckp_msg->ckp.ckp_node;
	lockckey.lk_key2 = ckp_msg->ckp.dbid;
	lockckey.lk_key3 = 0;
	lockckey.lk_key4 = 0;
	lockckey.lk_key5 = 0;
	lockckey.lk_key6 = 0;

	for (pnum = CKP_CSP_STALL; pnum <= CKP_CSP_DBACKUP; pnum++)
	{
	    lockckey.lk_key3 = pnum;

	    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrequest LK_S LK_CKP_CLUSTER (%w)\n"),
		__LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
		CKP_PHASE_NAME, ckp_msg->ckp.phase,
		ckp_msg->ckp.phase,
		CKP_PHASE_NAME, pnum);

	    lk_status = LKrequest(LK_PHYSICAL | LK_NOWAIT, 
		dmf_svcb->svcb_lock_list, 
		&lockckey, LK_S, (LK_VALUE * )0, &lockid, 0L, &sys_err);
	    if (lk_status != OK)
		break;
	}

    } while (FALSE);

    if (lk_status != OK)
    {
	uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 2,
		0, LK_S, 0, dmf_svcb->svcb_lock_list);
	status = E_DB_ERROR;
    }

    if (ckp_msg->ckp.flags & CKP_CRASH)
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Testing error handling\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase);
        status = E_DB_ERROR;
    }

    if (status != E_DB_OK)
    {
	if (db_open)
	    loc_status = csp_ckp_closedb(ckp_msg, log_id);

	/* Release any LK_CKP_CLUSTER locks */
	status = csp_ckp_lkrelease_all(ckp_msg);
    }

    return (status);
}

/*{
** Name: csp_ckp_error	- Process ckpdb error
**
** Description:
**	Process ckpdb error.
**	Send CKP_CSP_STATUS message (E_DB_ERROR)
**	Release LK_CKP_CLUSTER lock(s)
**      
**      NOTE: If this is a remote node 
**		(csp->csp_self_node != ckp_msg->ckp.ckp_node)
**      the caller of this routine must first close the database
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static VOID
csp_ckp_error(CSP_CX_MSG *ckp_msg)
{
    CSP			*csp = &csp_global;
    CSP_CX_MSG		err_msg;
    DB_STATUS		status;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    STATUS		loc_cl_status;
    CL_ERR_DESC		loc_sys_err;
    LK_LOCK_KEY		lockckey;
    LK_VALUE		lock_value;
    i4			err_code;

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) -> ERROR \n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase);

    if (csp->csp_self_node == ckp_msg->ckp.ckp_node)
    {
	/*
	** Local node, release LK_CKP_CLUSTER CKP_CSP_STATUS
	** Note we don't expect this routine to get called on the local node,
	** because on the local node the dmfjsp does the ckpdb work
	*/
	csp_ckp_lkrelease_all(ckp_msg);
    }

    /* Send CKP_CSP_ERROR message */
    err_msg.csp_msg_action = CSP_MSG_7_CKP;
    err_msg.csp_msg_node = (u_i1)csp->csp_self_node;
    err_msg.ckp.dbid = ckp_msg->ckp.dbid;
    err_msg.ckp.phase = CKP_CSP_ERROR;
    err_msg.ckp.ckp_node = ckp_msg->ckp.ckp_node;
    err_msg.ckp.ckp_var.status_rc = 1;
    cl_status = CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&err_msg);

    /* 
    ** The local node will have released the LK_CKP_CLUSTER CKP_CSP_STATUS lock
    ** in the message handler
    ** Now it is okay to release the phase lock
    ** The dmfjsp (ckpdb) is waiting for the phase lock, and will detect 
    ** the error when it tries look at the LK_CKP_CLUSTER CKP_CSP_STATUS lock
    */ 

    /* Release all LK_CKP_CLUSTER phase locks */
    status = csp_ckp_lkrelease_all(ckp_msg);

}

/*{
** Name: csp_ckp_lkrelease	- Release a specific LK_CKP_CLUSTER lock.
**
** Description:
**      Release LK_CKP_CLUSTER lock(s).
**
** Inputs:
**      ckp_msg				ckp message
**	pnum				
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
**	21-Dec-2007 (jonj)
**	    Re-form LK_CKP_CLUSTER lock keys.
*/
static DB_STATUS
csp_ckp_lkrelease(CSP_CX_MSG *ckp_msg, i4 pnum)
{
    CSP			*csp = &csp_global;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    LK_LOCK_KEY		lockckey;
    i4			err_code;
    DB_STATUS		status = E_DB_OK;

    /* Local STATUS lock is explicitly released by dmfcpp */
    if ( pnum == CKP_CSP_STATUS && ckp_msg->ckp.ckp_node
				== csp->csp_self_node )
	return(E_DB_OK);

    /* Release LK_CKP_CLUSTER locks */

    /* 
    ** lk_type = LK_CKP_CLUSTER
    ** lk_key1 = node of local checkpoint process unless CKP_CSP_STATUS,
    **		 then zero
    ** lk_key2 = dbid of the database being checkpointed
    ** lk_key3 = Phase number
    ** lk_key4 = remote node number if CKP_CSP_INIT, else zero
    ** lk_key5 = 0
    ** lk_key6 = 0
    */
    lockckey.lk_type = LK_CKP_CLUSTER;
    lockckey.lk_key1 = 0;
    lockckey.lk_key2 = ckp_msg->ckp.dbid;
    lockckey.lk_key3 = pnum;
    lockckey.lk_key4 = 0;
    lockckey.lk_key5 = 0;
    lockckey.lk_key6 = 0;

    if ( pnum != CKP_CSP_STATUS )
    {
	lockckey.lk_key1 = ckp_msg->ckp.ckp_node;

	if ( pnum == CKP_CSP_INIT )
	    lockckey.lk_key4 = csp->csp_self_node;
    }

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrelease LK_CKP_CLUSTER (%w)\n"),
	__LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	CKP_PHASE_NAME, ckp_msg->ckp.phase,
	ckp_msg->ckp.phase, 
	CKP_PHASE_NAME, pnum);

    cl_status = LKrelease(0, dmf_svcb->svcb_lock_list,
      (LK_LKID *)0, (LK_LOCK_KEY *)&lockckey, (LK_VALUE *)0, &sys_err);

    /* Report error when specific lock being released */
    if (cl_status != OK)
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrelease status %x for %w\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, 
	    CKP_PHASE_NAME, pnum);

	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1, 0, 
	    dmf_svcb->svcb_lock_list);
	status = E_DB_ERROR;
    }

    return (status);
}

/*{
** Name: csp_ckp_init_jsx - Initialize DMF_JSX context to perform ckpdb action.
**
** Description:
**	Initialize DMF_JSX context to perform ckpdb action.
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static DB_STATUS
csp_ckp_init_jsx(
CSP_CX_MSG	*ckp_msg,
DMF_JSX		*jsx)
{
    CSP			*csp = &csp_global;
    DB_STATUS		status;
    LG_DATABASE		db;
    i4			length;
    i4			*i4_ptr;
    DB_DB_NAME		*dbname;
    DB_OWN_NAME		*dbowner;
    i4			path_len;
    i4			err_code;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			log_id;

    /*
    ** What if remote node started after ckpdb started
    ** Do we prevent new transactions from starting on remote node?
    ** Can we tell if this node started after ckpdb started.
    */

    status = csp_ckp_logid(ckp_msg, &log_id);
    if (status)
        return (status);

    db.db_id = log_id; /* Internal LPD id  */
    cl_status = LGshow(LG_S_DATABASE, (PTR)&db, sizeof(db), &length, &sys_err);

    if (cl_status != OK || length == 0)
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Database %x is not known to the logging system"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, *(u_i4*)&db.db_id);

	if ( cl_status )
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Extract DB information out of info buffer where it has
    ** been packed into the db_buffer field.  Sanity check the
    ** size of the buffer to make sure it actually holds what
    ** we expect it to.
    */
    i4_ptr = (i4 *) &db.db_buffer[DB_DB_MAXNAME + DB_OWN_MAXNAME + 4];
    I4ASSIGN_MACRO(*i4_ptr, path_len);
    if (db.db_l_buffer < (DB_DB_MAXNAME + DB_OWN_MAXNAME + path_len + 8))
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LGshow DB buffer size mismatch (%d vs %d)\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase,
	    (DB_DB_MAXNAME + DB_OWN_MAXNAME + path_len + 8), db.db_l_buffer);
	return (E_DB_ERROR);
    }

    dbname = (DB_DB_NAME *) &db.db_buffer[0];
    dbowner = (DB_OWN_NAME *) &db.db_buffer[DB_DB_MAXNAME];
    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Init context for %~t\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase,
	    sizeof(dbname->db_db_name), dbname->db_db_name);

    /*
    ** Init DMF_JSX
    **
    ** JSX input required fields for the remote node are:
    ** 	jsx_db_id	External database id
    **  jsx_log_id	Log id
    **  jsx_db_name	Database name
    **  jsx_db_owner	Database owner
    **  jsx_node	self node number
    **  jsx_ckp_node	ckpdb node
    **  jsx_pid		PID of ckpdb proces on that node
    **  jsx_status	To turn trace on for cluster checkpoint
    **  jsx_status1	To indicate db/table checkpoint to SBACKUP
    **
    ** FIX ME if JSX1_STALL_TIME - need stall time in ckp_msg
    ** ?? new jsx_lock_list will be created...
    ** ?? transfer LK_CKP_TXN lock to server lock list after lock acquired
    */

    MEfill(sizeof(DMF_JSX), '\0', jsx);
    jsx->jsx_db_id = ckp_msg->ckp.dbid;
    jsx->jsx_log_id = log_id; /* Internal LPD id  */
    MEcopy(dbname, sizeof(DB_DB_NAME), jsx->jsx_db_name.db_db_name);
    MEcopy(dbowner, sizeof(DB_OWN_NAME), jsx->jsx_db_owner.db_own_name);
    jsx->jsx_node = csp->csp_self_node;
    jsx->jsx_ckp_node = ckp_msg->ckp.ckp_node;
    jsx->jsx_operation = JSX_O_CPP;
    jsx->jsx_next = (DMP_DCB*)&jsx->jsx_next;
    jsx->jsx_prev = (DMP_DCB*)&jsx->jsx_next;
    jsx->jsx_status = JSX_TRACE; /* FIX ME get from message */
    jsx->jsx_status1 = JSX1_CKPT_DB; /* assume database ckpdb */
    jsx->jsx_lock_list = dmf_svcb->svcb_lock_list;
    jsx->jsx_ckp_crash = 0;

    if (ckp_msg->ckp.phase == CKP_CSP_SBACKUP 
            && ckp_msg->ckp.ckp_var.sbackup_flag)
        jsx->jsx_status1 &= ~JSX1_CKPT_DB;

    if (ckp_msg->ckp.flags & CKP_CRASH)
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Init testing of error handling\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase);
        jsx->jsx_ckp_crash = ckp_msg->ckp.phase;
    }

    return (E_DB_OK);

}

/*{
** Name: csp_ckp_lkconvert - Update LK_CKP_CLUSTER CKP_CSP_INIT lock value
**
** Description:
**	Update LK_CKP_CLUSTER CKP_CSP_INIT lock (value)
**
** Inputs:
**      ckp_msg				ckp message
**      lockid				lock id to be converted
**	high_value			LK_VALUE (high)
**	low_value			LK_VALUE (low)
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
**	21-Dec-2007 (jonj)
**	    As Local dmfcpp handles its STATUS lock now, this
**	    will only be called for CKP_CSP_INIT locks
**	    taken on the remote nodes.
**	    Convert down to LK_S, not LK_N.
*/
static DB_STATUS
csp_ckp_lkconvert(CSP_CX_MSG *ckp_msg, 
LK_LKID *lockid,
i4 high_value, i4 low_value)
{
    CSP			*csp = &csp_global;
    STATUS		cl_status = OK;
    CL_ERR_DESC		sys_err;
    i4			err_code;
    LK_VALUE		lock_value;

    /*
    ** This routine gets called with the lockid for:
    **
    ** Remote node: Convert LK_CKP_CLUSTER CKP_CSP_INIT lock
    ** LK_CONVERT LK_X->LK_S to store LK_VALUE
    ** high value has zero, low value has database log_id
    */
    lock_value.lk_high = high_value;
    lock_value.lk_low = low_value;

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKconvert lockid %x.%x LK_S CKP_CSP_CLUSTER (%x,%x)\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    lockid->lk_unique, lockid->lk_common,
	    ckp_msg->ckp.phase, high_value, low_value);

    cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_NODEADLOCK, 
	dmf_svcb->svcb_lock_list, (LK_LOCK_KEY *)0, LK_S, 
	&lock_value, lockid, (i4)0, &sys_err);

    if (cl_status != E_DB_OK)
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 2,
		0, LK_S, 0, dmf_svcb->svcb_lock_list);
    }

    if (cl_status == E_DB_OK)
        return (E_DB_OK);
    else
	return (E_DB_ERROR);
}

/*{
** Name: csp_ckp_abort - Abort checkpoint on remote node
**
** Description:
**      Abort checkpoint on remote node
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static DB_STATUS
csp_ckp_abort(CSP_CX_MSG *ckp_msg)
{
    CSP			*csp = &csp_global;
    DB_STATUS		status = E_DB_OK;
    i4			phase = ckp_msg->ckp.ckp_var.abort_phase;
    DMF_JSX		jsx;
    i4			pnum;

    if (phase < CKP_CSP_DBACKUP)
    {
	/* Build JSX context for this ckpdb */
	status = csp_ckp_init_jsx(ckp_msg, &jsx);
	if (status == E_DB_OK && jsx.jsx_log_id)
	{
	    status = csp_ckp_closedb(ckp_msg, jsx.jsx_log_id);
	}
    }

    return (E_DB_OK);
}


/*{
** Name: csp_ckp_opendb - Open database, save logging context (internal LPD)
**
** Description:
**      Open database, save logging context (internal LPD)
**      This routine will only be done on the remote node,
**      since on the local node the dmfjsp does the ckpdb work.
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static DB_STATUS
csp_ckp_opendb(
CSP_CX_MSG		*ckp_msg, 
DB_DB_NAME		*dbname,
DB_OWN_NAME		*dbowner,
DMP_LOC_ENTRY		*dblocation,
i4			dcb_status,
i4			*log_id)
{
    CSP			*csp = &csp_global;
    i4			err_code;
    i4			open_flag;
    i4			open_flag2;
    LK_LOCK_KEY		lockckey;
    LK_LKID		lockid;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		loc_status;
    STATUS		lk_status = OK;
    CL_ERR_DESC		sys_err;
    bool		db_open = FALSE;
    DB_ERROR		local_dberr;

    open_flag = 0;
    open_flag2 = 0;
    *log_id = 0;

    if (dcb_status & DCB_S_JOURNAL)
	open_flag |= DM0L_JOURNAL;
    if ((dcb_status & DCB_S_FASTCOMMIT) ||
	(dcb_status & DCB_S_DMCM))
	open_flag |= DM0L_FASTCOMMIT;
    /* set the flag for a readonly database
     * not sure if this could ever happen but since we have the DCB status
     * may as well set the flag properly */
    if (dcb_status & DCB_S_RODB)
	open_flag2 |= DM0L_RODB;

    status = dm0l_opendb(dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	open_flag, open_flag2, dbname, dbowner, ckp_msg->ckp.dbid,
	&dblocation->physical, dblocation->phys_length, log_id, 
	(LG_LSN *)0, &local_dberr); 

    if (status)
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Add database ERROR\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase);
	return (status);
    }

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Add database log_id %d\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, *log_id);

    /* Request LK_CKP_CLUSTER CKP_CSP_INIT lock */
    lockckey.lk_type = LK_CKP_CLUSTER;
    lockckey.lk_key1 = ckp_msg->ckp.ckp_node;
    lockckey.lk_key2 = ckp_msg->ckp.dbid;
    lockckey.lk_key3 = CKP_CSP_INIT;
    lockckey.lk_key4 = csp->csp_self_node;
    lockckey.lk_key5 = 0;
    lockckey.lk_key6 = 0;
    MEfill(sizeof(LK_LKID), 0, &lockid);

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrequest LK_CKP_CLUSTER LK_X (%w) Node %d\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, 
	    CKP_PHASE_NAME, CKP_CSP_INIT,
	    csp->csp_self_node);
    lk_status = LKrequest(LK_PHYSICAL | LK_NOWAIT, dmf_svcb->svcb_lock_list, 
	    &lockckey, LK_X, (LK_VALUE * )0, &lockid, 0L, &sys_err);
    if (lk_status != OK)
    {
	uleFormat(NULL, lk_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 2,
		0, LK_X, 0, dmf_svcb->svcb_lock_list);
        status = E_DB_ERROR;
    }
    else
    {
	/*
	** For remote nodes the context needed is the internal LPD id returned
	** from the open database, store log_id in lock value
	**
	** Local nodes don't need any context, since checkpoint work is done
	** by the dmfjsp.
	*/

	/* Down-convert X->S to set lock value */
	status = csp_ckp_lkconvert(ckp_msg, &lockid, E_DB_OK, *log_id);
	if (status != E_DB_OK)
	{
	    lk_status = LKrelease(0, dmf_svcb->svcb_lock_list,
		(LK_LKID *)&lockid, (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	    status = E_DB_ERROR;
	}
    }

    if (status)
        loc_status = dm0l_closedb(*log_id, &local_dberr);

    return (status);
}


/*{
** Name: csp_ckp_closedb - Close database
**
** Description:
**      Close database
**      This routine will only be done on the remote node,
**      since on the local node the dmfjsp does the ckpdb work.
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static DB_STATUS
csp_ckp_closedb(CSP_CX_MSG *ckp_msg, i4 log_id)
{
    CSP			*csp = &csp_global;
    i4			err_code;
    LK_LOCK_KEY		lockckey;
    DB_STATUS		status = E_DB_OK;
    STATUS		lk_status = OK;
    CL_ERR_DESC		sys_err;
    DB_ERROR		local_dberr;

    /* Release LK_CKP_CLUSTER CKP_CSP_INIT lock */
    lockckey.lk_type = LK_CKP_CLUSTER;
    lockckey.lk_key1 = ckp_msg->ckp.ckp_node;
    lockckey.lk_key2 = ckp_msg->ckp.dbid;
    lockckey.lk_key3 = CKP_CSP_INIT;
    lockckey.lk_key4 = csp->csp_self_node;
    lockckey.lk_key5 = 0;
    lockckey.lk_key6 = 0;

    lk_status = LKrelease(0, dmf_svcb->svcb_lock_list,
		    (LK_LKID *)0, &lockckey, (LK_VALUE *)0, &sys_err);
    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKrelease LK_CKP_CLUSTER %w Node %d\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase,
	    CKP_PHASE_NAME, CKP_CSP_INIT,
	    csp->csp_self_node);

    /* Close database */
    status = dm0l_closedb(log_id, &local_dberr);
    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Close database status %d log_id %d\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, status, log_id);
    return(status);

}

/*{
** Name: csp_ckp_logid - Find the database logid from the open database
**
** Description:
**      Find the database logid from the open database (the internal LPD).
**      When the database was opened, the logid was stored in the
**      LK_CKP_CLUSTER CKP_CSP_INIT lock value - where it can be found on
**      subsequent ckpdb phases.
**      The existence of this lock with a non-zero logid in the lock value
**      indicates that the database is open.
**      This routine will only be done on the remote node,
**      since on the local node the dmfjsp does the ckpdb work.
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static DB_STATUS
csp_ckp_logid(CSP_CX_MSG *ckp_msg, i4 *log_id)
{
    CSP			*csp = &csp_global;
    i4			err_code;
    LK_LOCK_KEY		lockckey;
    DB_STATUS		status = E_DB_OK;
    STATUS		lk_status = OK;
    CL_ERR_DESC		sys_err;
    LK_VALUE		lock_value;

    /* Check value in LK_CKP_CLUSTER CKP_CSP_INIT lock */
    lockckey.lk_type = LK_CKP_CLUSTER;
    lockckey.lk_key1 = ckp_msg->ckp.ckp_node;
    lockckey.lk_key2 = ckp_msg->ckp.dbid;
    lockckey.lk_key3 = CKP_CSP_INIT;
    lockckey.lk_key4 = csp->csp_self_node;
    lockckey.lk_key5 = 0;
    lockckey.lk_key6 = 0;

    lock_value.lk_low = 0;
    lock_value.lk_high = 0;

    lk_status = LKrequest(LK_PHYSICAL, dmf_svcb->svcb_lock_list,
	&lockckey, LK_S, &lock_value, (LK_LKID *)0, 0L, &sys_err);

    if (lk_status != OK)
    {
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	return (E_DB_ERROR);
    }

    /* Release PHYSICAL lock to decrement reference count */
    lk_status = LKrelease(0, dmf_svcb->svcb_lock_list,
		    (LK_LKID *)0, &lockckey, (LK_VALUE *)0, &sys_err);

    *log_id = lock_value.lk_low;
    if (*log_id == 0 || lk_status != OK )
    {
	TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Failed, log_id %x, status %d\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, *log_id, lk_status);
	return (E_DB_ERROR);
    }
    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) Using log_id %x\n"), 
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase, *log_id);
    return (E_DB_OK);
}


/*{
** Name: csp_ckp_lkrelease_all - Release all LK_CKP_CLUSTER locks 
**
** Description:
**	Release all LK_CKP_CLUSTER locks
**
** Inputs:
**      ckp_msg				ckp message
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Side Effects:
**          none
**
** History:
**       01-sep-2004 (stial01)
**          created.
*/
static DB_STATUS
csp_ckp_lkrelease_all(CSP_CX_MSG *ckp_msg)
{
    CSP			*csp = &csp_global;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		lstatus;
    LK_LKID		lockid;
    u_i4		context;
    u_i4		length;
    LK_LKB_INFO		*lkb;
    LK_LKB_INFO		*lkb_info;
    LK_LKB_INFO		lkb_buffer[50];
    STATUS		cl_status = OK;
    CL_ERR_DESC		sys_err;
    i4			err_code;
    i4			i;
    bool		csp_ckp_lk;
    bool		lock_released;

    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKRelease LK_CKP_CLUSTER (ALL)\n"),
	    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
	    CKP_PHASE_NAME, ckp_msg->ckp.phase,
	    ckp_msg->ckp.phase);

    /*
    ** Call LKshow on this lock list until no more locks are returned.
    ** Check for any LK_CKP_CLUSTER locks that need to be released.
    */
    context = 0;
    for (;;)
    {
	cl_status = LKshow(LK_S_LIST_LOCKS, dmf_svcb->svcb_lock_list, 0, 0, 
			sizeof(lkb_buffer), (PTR)lkb_buffer, &length, 
			&context, &sys_err); 
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	    uleFormat(NULL, E_DM901D_BAD_LOCK_SHOW, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 1,
		0, dmf_svcb->svcb_lock_list);
	    status = E_DB_ERROR;
	    break;
	}

	if (length == 0)
	    break;

	/*
	** Look through each lock returned in the info buffer.
	*/
	lock_released = FALSE;
	lkb_info = (LK_LKB_INFO *) lkb_buffer;
	for (i = 0; i < ((i4)length / sizeof(LK_LKB_INFO)); i++)
	{
	    csp_ckp_lk = FALSE;
	    lkb = &lkb_info[i];

	    if ( lkb->lkb_key[0] == LK_CKP_CLUSTER 
		    && lkb->lkb_key[2] == ckp_msg->ckp.dbid )
	    {
		if (lkb->lkb_key[3] == CKP_CSP_STATUS)
		{
		    csp_ckp_lk = TRUE;
		}
		else if (lkb->lkb_key[1] == ckp_msg->ckp.ckp_node )
		{
		    TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKRelease LK_CKP_CLUSTER %w(%d)\n"),
			__LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
			CKP_PHASE_NAME, ckp_msg->ckp.phase,
			ckp_msg->ckp.phase, 
			CKP_PHASE_NAME, lkb->lkb_key[3], lkb->lkb_key[3]);
		    csp_ckp_lk = TRUE;
		}

	    }
	    else if (lkb->lkb_key[0] == LK_CKP_TXN
			&& lkb->lkb_key[6] == ckp_msg->ckp.dbid)
	    {
		csp_ckp_lk = TRUE;

		TRdisplay(ERx("%@ CPP %d %d::%x: %w(%d) LKRelease LK_CKP_TXN %~t %d\n"),
		    __LINE__, ckp_msg->ckp.ckp_node, ckp_msg->ckp.dbid,
		    CKP_PHASE_NAME, ckp_msg->ckp.phase,
		    ckp_msg->ckp.phase, 
		    20, (char *)&lkb->lkb_key[1], lkb->lkb_key[6]);
	    }

	    if (csp_ckp_lk)
	    {
		lockid.lk_unique = lkb->lkb_id;
		lockid.lk_common = lkb->lkb_rsb_id;
		cl_status = LKrelease((i4) 0, dmf_svcb->svcb_lock_list, &lockid,
				(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code,
			1, 0, dmf_svcb->svcb_lock_list);
		    status = E_DB_ERROR;
		    break;
		}
		lock_released = TRUE;
	    }
	}

	if (status != E_DB_OK)
	    break;

	/*
	** If there are more locks to check for this lock list then
	** loop back around for another LKshow call.
	**
	** If we have released any locks returned in the previous
	** LKshow call then we have to reset the show context and start
	** over from the beginning of the lock list.  This ensures
	** that we will not miss any locks as a result of changing the
	** lock list in the middle of showing it.
	*/
	if (context)
	{
	    if (lock_released)
		context = 0;
	    continue;
	}

	break;
    }

}

/*
** Name: csp_class_roster_class_find - Locate a dbms class roster entry
**
** Description:
**	Locate a dbms class roster entry using a class event CSP_CX_MSG
**
** Inputs:
**      r				class roster to search in
**	c				class to search for
**
** Outputs:
**      Returns:
**          CSP_NODE_CLASS_INFO_QUEUE*	pointer to located class in roster
**          NULL			class not found
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
static CSP_NODE_CLASS_INFO_QUEUE*
csp_class_roster_class_find(CSP_NODE_CLASS_ROSTER *r, CSP_CX_MSG *c,
			    char *class_name)
{
    /* locate a class record in a node class roster */
    QUEUE *q;
    for( q = r->classes.queue.q_next; q != &r->classes.queue; q = q->q_next )
    {
	int compare = c->csp_msg_node -
	    ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node;
	compare = compare == 0 ?
	    STncmp(class_name,
		   ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.name,
		   CSP_MSG_CLASS_BYTES) :
	    compare;

	if (compare == 0)
	{
	    /* match exists, return it */
	    return((CSP_NODE_CLASS_INFO_QUEUE*) q);
	}
	if (compare < 0)
	{
	    /* item would have been before this point, early out */
	    break;
	}
	/* else continue */
    }
    return(NULL);
}

/*
** Name: csp_class_roster_class_create - Allocate a dbms class entry
**
** Description:
**	Allocate a dbms class roster entry.
**
** Inputs:
**	none
**
** Outputs:
**      err_code			error code on failure
**      Returns:
**          NULL			allocation failed
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static CSP_NODE_CLASS_INFO_QUEUE*
csp_class_roster_class_create(i4 *err_code)
{
    /* allocate a class record */
    CSP_NODE_CLASS_INFO_QUEUE *cl;
    DM_OBJECT *dmobj;
    DB_ERROR	local_dberr;

    i4 status = dm0m_allocate ( sizeof(CSP_NODE_CLASS_INFO_QUEUE) +
				sizeof(DM_OBJECT), 0, 0, 0, NULL, &dmobj,
				&local_dberr);
    if (status != E_DB_OK)
	return(NULL);

    cl = (CSP_NODE_CLASS_INFO_QUEUE *) &dmobj[1];

    /* common initialization */
    cl->info.node = -1;
    cl->info.count_have = 0;
    cl->info.count_want = 0;
    cl->info.name[0] = EOS;
    QUinit(&cl->queue);

    return(cl);
}

/*
** Name: csp_class_roster_class_destroy - Deallocate a dbms class entry
**
** Description:
**	Deallocate a dbms class roster entry and remove it from any
**	queue it may be part of.
**
** Inputs:
**	c				class to deallocate
**
** Outputs:
**      Returns:
**          OK				success
**
** Side Effects:
**          Deallocated dbms class entry is removed from any queue
**          it may have been part of prior to deallocation.
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_class_destroy(CSP_NODE_CLASS_INFO_QUEUE *c)
{
    /* deallocate class record */
    DM_OBJECT *dmobj = ((DM_OBJECT *) c) - 1;
    QUremove( (QUEUE *) c);
    dm0m_deallocate(&dmobj);
    return(OK);
}

/*
** Name: csp_class_roster_class_insert - Insert a dbms class into a roster
**
** Description:
**	Inserts a dbms class roster entry into the specified roster at the
**	proper location.  This function maintains a sorted roster.
**
** Inputs:
**	r				roster to insert into
**	c				class to insert
**
** Outputs:
**      Returns:
**          OK				success
**          -1				error, duplicate class already exists
**
** Side Effects:
**          The class roster is maintained as a sorted list.
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_class_insert(CSP_NODE_CLASS_ROSTER *r,
			      CSP_NODE_CLASS_INFO_QUEUE *c)
{
    /* insert 'c' into appropriate location in class list */
    QUEUE *q = (QUEUE *) &r->classes;
    for( q = r->classes.queue.q_next; q != &r->classes.queue; q = q->q_next )
    {
	/* fanch01 FIXME add pid */
	int compare = c->info.node -
	    ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node;
	compare = compare == 0 ?
	    STncmp(c->info.name, ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.name,
		   CSP_MSG_CLASS_BYTES) :
	    compare;

	if (compare == 0)
	{
	    /* a match exists, caller messed up */
	    return(-1);
	    /* fanch01 FIXME create new error */
	}
	if (compare < 0)
	{
	    /* found the location */
	    break;
	}
	/* else continue */
    }
    QUinsert((QUEUE *) c, q->q_prev);
    return(OK);
}

/*
** Name: csp_class_roster_classes_destroy - Destroys class roster entries.
**
** Description:
**	Destroys (deallocates) all dbms class roster entries after removing
**	them from their class roster.  The result will be a class roster
**	with no dbms class entries.
**
** Inputs:
**	r				roster to destroy
**
** Outputs:
**      Returns:
**          none
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static void
csp_class_roster_classes_destroy(CSP_NODE_CLASS_ROSTER *r)
{
    /* locate a class record in a node class roster */
    QUEUE *q = (QUEUE *) &r->classes;
    for( q = r->classes.queue.q_next; q != &r->classes.queue; q = q->q_next )
    {
	csp_class_roster_class_destroy((CSP_NODE_CLASS_INFO_QUEUE *) q);
    }
    r->class_count = 0;
}

/*
** Name: csp_class_event_create - Allocates and initializes a class event.
**
** Description:
**	Allocates and initializes a class event with information provided
**	by the class event message.
**
** Inputs:
**	v				class event message
**
** Outputs:
**	err_code			error code on error
**      Returns:
**          CSP_NODE_CLASS_EVENT_QUEUE*	pointer to initialized class event
**          NULL			on allocation or initialization error
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
**       25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)

*/
static CSP_NODE_CLASS_EVENT_QUEUE *
csp_class_event_create(CSP_CX_MSG *v, i4 *err_code, char *class_name)
{
    /* create and insert class event with passed values */
    CSP_NODE_CLASS_EVENT_QUEUE *e;
    DMP_MISC *dmobj;
    DB_ERROR	local_dberr;

    i4 status = dm0m_allocate( sizeof(CSP_NODE_CLASS_EVENT_QUEUE) +
			       sizeof(DMP_MISC),
			       0, MISC_CB, MISC_ASCII_ID,
			       0, (DM_OBJECT**)&dmobj, &local_dberr );
    if ( status != E_DB_OK)
	return(NULL);
    e = (CSP_NODE_CLASS_EVENT_QUEUE *) &dmobj[1];
    dmobj->misc_data = (char*)e;
    MEcopy(v, sizeof(CSP_CX_MSG), &e->event);
    STncpy(e->class_name, class_name, CSP_MSG_CLASS_BYTES);
    QUinit(&e->queue);
    return(e);
}

/*
** Name: csp_class_event_destroy - Deallocates a dbms class event.
**
** Description:
**	Deallocates a class event.  This function does not remove the event
**	from the event queue.
**
** Inputs:
**	event				class event to deallocate
**
** Outputs:
**      Returns:
**          none
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static void
csp_class_event_destroy(CSP_NODE_CLASS_EVENT_QUEUE *event)
{
    /* remove class event */
    DM_OBJECT *dmobj = ((DM_OBJECT *) event) - 1;
    dm0m_deallocate(&dmobj);
}

/*
** Name: csp_class_roster_event_add - Adds a class event to a class roster.
**
** Description:
**	Allocates and initializes a class event and then adds it to the
**	specified roster's class event queue.
**
** Inputs:
**	r				class roster to insert into
**	v				class event message
**	class_name			class name
**
** Outputs:
**      Returns:
**          OK				success
**          error			allocation error value encoutered
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
static i4
csp_class_roster_event_add(CSP_NODE_CLASS_ROSTER *r, CSP_CX_MSG *v,
			   char *class_name)
{
    /* create and insert class event with passed values */
    i4 err_code;
    QUEUE *q = (QUEUE *) &r->class_events;
    CSP_NODE_CLASS_EVENT_QUEUE *e = csp_class_event_create(v, &err_code,
							   class_name);
    if (!e)
	return(err_code);
    QUinsert((QUEUE *) e, q->q_prev);
    return(OK);
}

/*
** Name: csp_class_roster_event_destroy - Destroys a dbms class event.
**
** Description:
**	Dellocates a dbms class event after removing it from any queue it
**	may be part of.
**
** Inputs:
**	e				class event to destroy
**
** Outputs:
**      Returns:
**          none
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static void
csp_class_roster_event_destroy(CSP_NODE_CLASS_EVENT_QUEUE *e)
{
    /* destroy and remove class event */
    QUremove( (QUEUE *) e );
    csp_class_event_destroy(e);
}

/*
** Name: csp_class_roster_event_apply - Applies an event to a class roster.
**
** Description:
**	Updates a roster's membership by applying a dbms class event.  The
**	event may be a start or stop event.  The roster creates, removes or
**	updates membership entries as appropriate.
**
** Inputs:
**	r				class roster to apply event to
**	e				class event to apply
**	class_name			class name
**
** Outputs:
**      Returns:
**          OK				success
**          other			failure due to roster consistency or
**          				an invalid action in the class event
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
static i4
csp_class_roster_event_apply(CSP_NODE_CLASS_ROSTER *r, CSP_CX_MSG *e,
			     char *class_name)
{
    i4 err_code = OK;
    /* update current node's class roster with specified event */
    CSP_NODE_CLASS_INFO_QUEUE *re;

    /* locate (if exists) the class roster entry */
    re = csp_class_roster_class_find(r, e, class_name);

    /* have current dbms class roster, update it */
    switch (e->csp_msg_action)
    {
	case CSP_MSG_9_CLASS_START:

	    if (re)
	    {
		/* update class roster entry */
		re->info.count_have++;
		re->info.count_want++;
	    }
	    else
	    {
		/* add new class roster entry */
		re = csp_class_roster_class_create(&err_code);
		if (!re)
		    break;
		MEcopy(class_name, CSP_MSG_CLASS_BYTES, re->info.name);
		re->info.node = e->csp_msg_node;
		re->info.count_have++;
		re->info.count_want++;
		r->class_count++;
		csp_class_roster_class_insert(r, re);
	    }
	    break;
	case CSP_MSG_10_CLASS_STOP:

	    if (re)
	    {
		/* update class roster entry */
		re->info.count_have--;
		re->info.count_want--;
		if (re->info.count_want == 0)
		{
		    r->class_count--;
		    csp_class_roster_class_destroy(re);
		}
	    }
	    else
	    {
		/* down event for non-existent class */
		/* fanch01 FIXME need new error code here */
		err_code = -1;
	    }
	    break;
	default:
	    /* fanch01 FIXME need new error code here */
	    err_code = -1;
	    break;
    }
    if (csp_debug)
    {
	csp_class_roster_print(r);
    }
    return(err_code);
}

/*
** Name: csp_class_roster_queue_destroy - Destroys a class roster event queue.
**
** Description:
**	Removes and deallocates all queued dbms class events associated with
**	the specified roster.
**
** Inputs:
**	r				class roster to destroy queued events
**
** Outputs:
**      Returns:
**          OK				success
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_queue_destroy(CSP_NODE_CLASS_ROSTER *r)
{
    QUEUE *q;
    /* destroy queued class events */
    for( q = r->class_events.queue.q_next; q != &r->class_events.queue;
	 q = r->class_events.queue.q_next )
    {
	csp_class_roster_event_destroy((CSP_NODE_CLASS_EVENT_QUEUE *) q);
    }
    return(OK);
}

/*
** Name: csp_class_roster_queue_apply - Apply the dbms class event queue.
**
** Description:
**	Applies all of the queued events associated with the class roster
**	to the roster.
**
** Inputs:
**	r				class roster to apply queued events
**
** Outputs:
**      Returns:
**          OK				success
**          other			on error
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_queue_apply(CSP_NODE_CLASS_ROSTER *r)
{
    /* apply (and destroy) class events attached to this roster */
    QUEUE *q;
    i4 status;
    /* destroy queued class events */
    for( q = r->class_events.queue.q_next; q != &r->class_events.queue;
	 q = r->class_events.queue.q_next )
    {
	status = csp_class_roster_event_apply(r,
                    &((CSP_NODE_CLASS_EVENT_QUEUE *) q)->event,
                    ((CSP_NODE_CLASS_EVENT_QUEUE *) q)->class_name);
	if (status != OK)
	    return(status);
	csp_class_roster_event_destroy((CSP_NODE_CLASS_EVENT_QUEUE *) q);
    }
    return(OK);
}

/*
** Name: csp_class_roster_queue - Queue a class event to building rosters.
**
** Description:
**	When a roster is "building" it is awaiting the long message sync
**	response.  Subsequent events to the sync request must be queued to
**	maintain consistency.  csp_class_roster_queue performs this
**	operation for all building class rosters.
**
** Inputs:
**	e				class roster event to queue
**	class_name			class name
**
** Outputs:
**      Returns:
**          OK				success
**          other			error code, fatal
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
static i4
csp_class_roster_queue(CSP_CX_MSG *e, char *class_name)
{
    /* add class event to any rosters marked pending */
    CSP_NODE_CLASS_ROSTER *r;
    i4 err_code, node;

    for ( node = 1;
	  node <= csp_config->cx_node_cnt; 
	  node++ )
    {
	/* need to append actions for each node building a roster */
	CX_NODE_INFO *cx_node_info =
	    &csp_config->cx_nodes[csp_config->cx_xref[node]];
	CSP_NODE_INFO *node_info =
	    &csp_global.csp_node_info[cx_node_info->cx_node_number-1];
	r = &node_info->class_roster;
	if ( r->class_roster_build )
	{
	    err_code = csp_class_roster_event_add(r, e, class_name);
	    if (err_code != OK)
		return(err_code);
	}
    }
    return(OK);
}

/*
** Name: csp_class_roster_build - Puts a class roster into building state
**
** Description:
**	Prepares a class roster for synchronization and sets the roster's
**	state to building.
**
** Inputs:
**	roster				class roster to build
**	roster_id			value expected in the sync reply
**
** Outputs:
**      Returns:
**          OK				success
**          other			error code, fatal
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_build(CSP_NODE_CLASS_ROSTER *roster, u_i4 roster_id)
{
    /* initialize class roster for build */
    if ( roster->class_roster_build )
    {
	/* building the roster, therefore it's already active, error */
	return(-1);
    }
    /* roster must be empty at this point! */
    roster->class_roster_id = roster_id;
    QUinit((QUEUE *) &roster->classes);
    QUinit((QUEUE *) &roster->class_events);
    roster->class_count = 0;
    roster->class_roster_build = 1;
    roster->class_roster_master = csp_global.csp_master_node;
    return(0);
}

/*
** Name: csp_class_roster_destroy_node - Removes specified node's records
**	from a roster.
**
** Description:
**	Remove dbms class roster entries that are associated with the
**	specified node number.
**
** Inputs:
**	r				class roster
**	node				node number to remove
**
** Outputs:
**      Returns:
**          0..n			number of class entries removed
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_destroy_node(CSP_NODE_CLASS_ROSTER *r, i4 node)
{
    /* locate a node's class records and remove */
    QUEUE *q = (QUEUE *) &r->classes;
    i4 found = 0;
    for( q = r->classes.queue.q_next; q != &r->classes.queue; q = q->q_next )
    {
	if (((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node == node)
	{
	    if (csp_debug)
	    {
		_VOID_ TRdisplay(ERx("Class roster consistency error: node %d  class '%s' have=%d want=%d exists during csp exit, removing\n"),
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node,
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.name,
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.count_have,
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.count_want);
	    }
	    csp_class_roster_class_destroy((CSP_NODE_CLASS_INFO_QUEUE *) q);
	    r->class_count--;
	    found++;
	}
    }

    if (csp_debug)
    {
	csp_class_roster_print(r);
    }
    return(found);
}

/*
** Name: csp_class_roster_failed_node - Set class attributes on node failure.
**
** Description:
**	Set dbms class attributes for the specified node appropriately for
**	node failure.  Currently this affects the dbms class "have" count 
**	by setting it to zero.
**
** Inputs:
**	r				class roster
**	node				node number to adjust
**
** Outputs:
**      Returns:
**          0..n			number of class entries adjusted
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_failed_node(CSP_NODE_CLASS_ROSTER *r, i4 node)
{
    /* locate a node's class records and remove */
    QUEUE *q = (QUEUE *) &r->classes;
    i4 found = 0;
    for( q = r->classes.queue.q_next; q != &r->classes.queue; q = q->q_next )
    {
	if (((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node == node)
	{
	    ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.count_have = 0;
	    found++;
	}
    }

    return(found);
}

/*
** Name: csp_class_roster_destroy - Removes all roster information.
**
** Description:
**	Removes and deallocates all resources (classes and events) associated
**	with specified class roster.  Roster values are set to initialized
**	state.
**
** Inputs:
**	r				class roster
**
** Outputs:
**      Returns:
**          OK				success
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_destroy(CSP_NODE_CLASS_ROSTER *roster)
{
    QUEUE *q;
    /* destroys a class roster by deallocating class events and dbms classes */

    /* destroy class event queue */
    csp_class_roster_queue_destroy(roster);

    /* remove class roster */
    csp_class_roster_classes_destroy(roster);

    roster->class_roster_id = 0;
    QUinit((QUEUE *) &roster->classes);
    QUinit((QUEUE *) &roster->class_events);
    roster->class_count = 0;
    roster->class_roster_build = 0;
    roster->class_roster_master = 0;
    return(OK);
}

/*
** Name: csp_class_roster_copy - Copies a dbms class roster.
**
** Description:
**	Copies a dbms class roster entries from one roster to another.
**	The destination roster should be empty prior to calling.
**
** Inputs:
**	roster_dst			destination class roster
**	roster_src			source class roster
**
** Outputs:
**      Returns:
**          OK				success
**          other			failure due to allocation or
**          				duplicated entries in the destination
**          				roster
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_copy(CSP_NODE_CLASS_ROSTER *roster_dst,
		      CSP_NODE_CLASS_ROSTER *roster_src)
{
    /* copy a class roster from one node to another */
    QUEUE *q;
    CSP_NODE_CLASS_INFO_QUEUE *c;
    i4 err_code;

    for( q = roster_src->classes.queue.q_next;
	 q != &roster_src->classes.queue; q = q->q_next )
    {
	c = csp_class_roster_class_create(&err_code);
	if (!c)
	    return(err_code);
	STRUCT_ASSIGN_MACRO(((CSP_NODE_CLASS_INFO_QUEUE *) q)->info, c->info);
	QUinit((QUEUE *) &c->queue);
	err_code = csp_class_roster_class_insert(roster_dst, c);
	if (err_code != OK)
	    return(err_code);
    }
    return(OK);
}

/*
** Name: csp_class_roster_compare - Compares dbms class rosters for equality.
**
** Description:
**	Compares two dbms class rosters for equality.  Equality is defined
**	as the two rosters having the same class entries with equal attribute
**	values.  Event queues and roster summary information is not considered.
**
** Inputs:
**	roster_a			class roster
**	roster_b			class roster
**
** Outputs:
**      Returns:
**          0				equal
**          other			not equal
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_compare(CSP_NODE_CLASS_ROSTER *roster_a,
			 CSP_NODE_CLASS_ROSTER *roster_b)
{
    /* compares two class rosters for equality, 0==equal */ 
    QUEUE *qa, *qb;
    for( qa = roster_a->classes.queue.q_next,
	 qb = roster_b->classes.queue.q_next;
	 qa != &roster_a->classes.queue;
	 qa = qa->q_next, qb = qb->q_next )
    {
	if (((CSP_NODE_CLASS_INFO_QUEUE *) qa)->info.count_have !=
	    ((CSP_NODE_CLASS_INFO_QUEUE *) qb)->info.count_have ||
	    ((CSP_NODE_CLASS_INFO_QUEUE *) qa)->info.count_want !=
	    ((CSP_NODE_CLASS_INFO_QUEUE *) qb)->info.count_want ||
	    ((CSP_NODE_CLASS_INFO_QUEUE *) qa)->info.node !=
	    ((CSP_NODE_CLASS_INFO_QUEUE *) qb)->info.node ||
	    STncmp(((CSP_NODE_CLASS_INFO_QUEUE *) qa)->info.name,
		   ((CSP_NODE_CLASS_INFO_QUEUE *) qb)->info.name,
		   CSP_MSG_CLASS_BYTES))
	{
	    return(1);
	}

	/* one of the two class lists ended? */
	if (qa == &roster_a->classes.queue ^ qb == &roster_b->classes.queue)
	{
	    return(1);
	}
    }
    return(0);
}

/*
** Name: csp_class_roster_nextid - Return a unique identifier.
**
** Description:
**	Generate a unique ID based on the Node number and a timestamp.
**
** Inputs:
**	none
**
** Outputs:
**      Returns:
**          unique class ident.
**
** Side Effects:
**          none
**
** History:
**       26-may-2005 (devjo01)
**          Created.
*/
u_i4
csp_class_roster_nextid(void)
{
    u_i4	ident = TMsecs() & 0x0FFFFFFF;

    ident |= ( (csp_global.csp_self_node - 1) << 28 );
    return ident;
}


/*
** Name: csp_class_roster_marshall - Pack a class roster into a buffer.
**
** Description:
**	Pack class roster membership information into a buffer.  The buffer
**	does not use pointers or otherwise reference to any data outside of
**	the buffer because the buffer's intended use is transport via disk
**	or network.
**
**	Allocation requirements for the buffer are calculated by:
**		roster_class_members * sizeof(CSP_NODE_CLASS_LMSG)
**
** Inputs:
**	node				node's roster to pack
**	buffer				buffer to pack roster into
**
** Outputs:
**      Returns:
**          OK				success
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_marshall(u_i1 node, PTR buffer)
{
    /* packs a roster into a buffer */
    CSP			*csp = &csp_global;
    CSP_NODE_CLASS_ROSTER *roster =
	&csp_global.csp_node_info[node-1].class_roster;
    CSP_NODE_CLASS_LMSG *pbuf;
    QUEUE *q;
    CSP_NODE_INFO	*node_info;
    CSP_DBMS_INFO	*dbms;
    CSP_DBMS_SYNC	dbms_sync[CX_MAX_NODES][CSP_MAX_DBMS_MARSHALL];
    i4			n,d,m;
    CX_REQ_CB		*preq;
    i4			dbms_cnt = 0;

    for (pbuf = (CSP_NODE_CLASS_LMSG *) buffer,
	 q = roster->classes.queue.q_next;
	  q != &roster->classes.queue;
	  pbuf++, q = q->q_next)
    {
	STRUCT_ASSIGN_MACRO(((CSP_NODE_CLASS_INFO_QUEUE *) q)->info, pbuf->info);
    }

    TRdisplay("%@ CSP: marshall dbms info\n");
    MEfill(sizeof(dbms_sync), '\0', &dbms_sync);

    for ( n = 1; n <= CX_MAX_NODES; n++ )
    {
	node_info = &csp->csp_node_info[n-1];
	dbms = &node_info->node_dbms[0];
	m = 0;
	for (d = 0; d < CSP_MAX_DBMS; d++, dbms++)
	{
	    preq = &dbms->deadman_lock_req;
	    /* FIX ME only init preq->cx_key in cxinit */
	    if (preq->cx_key.lk_key2)
	    {
	        dbms_cnt++;
		dbms_sync[n-1][m].node = n;
		dbms_sync[n-1][m].dbms_pid = preq->cx_key.lk_key2;
		dbms_sync[n-1][m].lg_id_id = d;
		TRdisplay("marshall node %d, pid %d, lg_id %d\n",
		    dbms_sync[n-1][m].node, dbms_sync[n-1][m].dbms_pid, 
		    dbms_sync[n-1][m].lg_id_id);
		m++;
	    }
	}
    }
    MEcopy(&dbms_sync, sizeof(dbms_sync), (PTR)pbuf);
    TRdisplay("%@ CSP: marshall (%d) dbms info done \n", dbms_cnt);

    return(OK);
}

/*
** Name: csp_class_roster_unmarshall - Unpack a class roster from a buffer.
**
** Description:
**	Unpack class roster membership information from a buffer.  The
**	specified roster should be empty before calling this function.
**
** Inputs:
**	roster				target roster to unpack into
**	count				count of dbms classes in the buffer
**	buffer				buffer to unpack from
**
** Outputs:
**      Returns:
**          OK				success
**          other			error, an allocation error, caller
**          				failed to pass an empty target roster,
**					or data consistency error
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static i4
csp_class_roster_unmarshall(CSP_NODE_CLASS_ROSTER *roster, i4 count,
			    PTR buffer)
{
    /* unpacks a roster from a buffer */
    CSP			*csp = &csp_global;
    CSP_NODE_CLASS_LMSG *pbuf;
    CSP_NODE_CLASS_INFO_QUEUE *q;
    i4 err_code;
    CSP_DBMS_SYNC	*dbms_sync;
    i4			n,d;
    i4			dbms_cnt = 0;

    for (pbuf = (CSP_NODE_CLASS_LMSG *) buffer;
	  count > 0;
	  pbuf++, count--)
    {
	q = csp_class_roster_class_create(&err_code);
	if (!q)
	    return(err_code);
	STRUCT_ASSIGN_MACRO(pbuf->info, ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info);
	QUinit((QUEUE *) &q->queue);
	err_code = csp_class_roster_class_insert(roster, q);
	if (err_code != OK)
	    return(err_code);
	roster->class_count++;
    }

    TRdisplay("%@ CSP: unmarshall dbms info\n");

    dbms_sync = (CSP_DBMS_SYNC *)pbuf;
    for ( n = 0; n < CX_MAX_NODES; n++ )
    {
	for (d = 0; d < CSP_MAX_DBMS_MARSHALL; d++)
	{
	    if (dbms_sync->dbms_pid)
	    {
	        dbms_cnt++;
		TRdisplay("unmarshall node %d, pid %d, lg_id %d\n",
		    dbms_sync->node, dbms_sync->dbms_pid, 
		    dbms_sync->lg_id_id);
		csp_req_dbms_dmn(dbms_sync->node, dbms_sync->dbms_pid,
				dbms_sync->lg_id_id);
	    }
	    dbms_sync++;
	}
    }

    TRdisplay("%@ CSP: unmarshall dbms (%d) info done\n", dbms_cnt);

    return(OK);
}

/*
** Name: csp_class_roster_print - Display membership of a class roster.
**
** Description:
**	For debugging purposes display the membership of a class roster
**	in the active log.
**
** Inputs:
**	roster				roster to display membership of
**
** Outputs:
**      Returns:
**          none
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static void
csp_class_roster_print(CSP_NODE_CLASS_ROSTER *r)
{
    /* locate a class record in a node class roster */
    QUEUE *q;
    _VOID_ TRdisplay(ERx("Node Server Class Roster  (%d entries)\n"),
		     r->class_count);
    for( q = r->classes.queue.q_next; q != &r->classes.queue; q = q->q_next )
    {
	_VOID_ TRdisplay(ERx("* Node %2d  Have %2d  Want %2d  Class: %s\n"),
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.node,
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.count_have,
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.count_want,
			 ((CSP_NODE_CLASS_INFO_QUEUE *) q)->info.name);
    }
}

/*
** Name: csp_dbms_start - Start a local dbms server.
**
** Description:
**	Starts a local dbms server process for the specified class.
**
**	Note: The pid is probably of little use as the dbms process is
**	expected to fork.
**
** Inputs:
**	class_name			server class to start
**	pid				child process id
**
** Outputs:
**      Returns:
**          OK				success
**          status			return value of PCspawn
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
*/
static STATUS
csp_dbms_start(char *class_name, PID *pid)
{
    char cmd[128];
    CL_ERR_DESC err_code;

    STprintf( cmd, ERx( "ingstart -iidbms=%s" ), class_name );
    return PCcmdline(NULL, cmd, 0, NULL, &err_code);
}

/*
** Name: csp_dbms_remote_start - Start a dbms server on a remote node.
**
** Description:
**	Sends a message on the CSP_CHANNEL requesting to start a dbms server
**	process on the specified node.
**
** Inputs:
**	class_name			server class to start
**	node				node to start the dbms process on
**
** Outputs:
**      Returns:
**          OK				success
**          status			return value of CXmsg_send
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
static STATUS
csp_dbms_remote_start(char *class_name, i4 node)
{
    CSP_CX_MSG class_msg;
    char lclass_name[CSP_MSG_CLASS_BYTES];
    STATUS status;

    STncpy(lclass_name, class_name, sizeof(lclass_name));
    status = CXmsg_stow( &class_msg.dbms_start.class_name,
	(PTR) lclass_name, sizeof(lclass_name) );
    if ( status != OK )
	return (status);

    class_msg.csp_msg_action = CSP_MSG_11_DBMS_START;
    class_msg.csp_msg_node = csp_global.csp_self_node;
    class_msg.dbms_start.node = node;
    return CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&class_msg );
}

/*
** Name: dmfcsp_class_start - Notify CSP of a starting dbms server.
**
** Description:
**	Sends a message on the CSP_CHANNEL indicating the start of a dbms
**	server process.
**
** Inputs:
**	class_name			server class that started
**	node				node dbms process started on
**	pid				pid of dbms process
**
** Outputs:
**      Returns:
**          OK				success
**          status			return value of CXmsg_send
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
STATUS
dmfcsp_class_start(char *class_name, i4 node, PID pid, i2 lg_id_id)
{
    CSP_DBMS_INFO	dbms_info;
    CSP_CX_MSG		class_msg;
    CX_REQ_CB		*preq;
    i4			err_code;
    LK_UNIQUE		csp_tranid;     /* "dummy" tranid for CSP DLM calls */
    char		lclass_name[CSP_MSG_CLASS_BYTES];
    STATUS		status;

    MEfill(sizeof(CSP_DBMS_INFO), '\0', &dbms_info);
    preq = &dbms_info.deadman_lock_req;
    preq->cx_new_mode = LK_X;
    preq->cx_key.lk_type = LK_CSP;
    preq->cx_key.lk_key1 = node;
    preq->cx_key.lk_key2 = pid;
    preq->cx_dlm_ws.cx_dlm_lock_id = 0;
    /* This dbms needs to generate a unique tranid to use for lock request */
    err_code = CXunique_id( &csp_tranid );
    if ( err_code )
        return (FAIL);
    err_code = CXdlm_request(
      CX_F_STATUS | CX_F_PCONTEXT | CX_F_NODEADLOCK |
      CX_F_PRIORITY | CX_F_IGNOREVALUE | CX_F_NO_DOMAIN,
      preq, &csp_tranid );
    if ( err_code == OK )
    {

    }
    else if ( E_CL2C01_CX_I_OKSYNC == err_code )
    {
        /* lock is grantable - this is what SHOULD happen */
    }
    else
    {
        /* Fatal error, dbms needs its own deadman lock */
	TRdisplay("%@ Error getting DMN for pid %d lg_id_id %d err_code %d \n", 
			pid, lg_id_id, err_code);
	return (FAIL);
    }

    STncpy(lclass_name, class_name, sizeof(lclass_name));
    status = CXmsg_stow( &class_msg.class_event.class_name,
	(PTR) lclass_name, sizeof(lclass_name) );
    if ( status != OK )
	return (status);

    class_msg.csp_msg_action = CSP_MSG_9_CLASS_START;
    class_msg.csp_msg_node = node;
    class_msg.class_event.pid = pid;
    class_msg.class_event.lg_id_id = lg_id_id;
    return CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&class_msg );
}

/*
** Name: dmfcsp_class_stop - Notify CSP of a stopping dbms server.
**
** Description:
**	Sends a message on the CSP_CHANNEL indicating the stop of a dbms
**	server process.
**
** Inputs:
**	class_name			server class that stopped
**	node				node dbms process stopped on
**	pid				pid of dbms process
**
** Outputs:
**      Returns:
**          OK				success
**          status			return value of CXmsg_send
**
** Side Effects:
**          none
**
** History:
**       22-Apr-2005 (fanch01)
**          Created.
**       26-May-2005 (fanch01)
**          Add long message class name functionality.
*/
STATUS
dmfcsp_class_stop(char *class_name, i4 node, PID pid, i2 lg_id_id)
{
    CSP_CX_MSG class_msg;
    char lclass_name[CSP_MSG_CLASS_BYTES];
    STATUS status;

    STncpy(lclass_name, class_name, sizeof(lclass_name));
    status = CXmsg_stow( &class_msg.class_event.class_name,
	(PTR) lclass_name, sizeof(lclass_name) );
    if ( status != OK )
	return (status);

    class_msg.csp_msg_action = CSP_MSG_10_CLASS_STOP;
    class_msg.csp_msg_node = node;
    class_msg.class_event.pid = pid;
    class_msg.class_event.lg_id_id = lg_id_id;
    return CXmsg_send( CSP_CHANNEL, NULL, NULL, (PTR)&class_msg );
}


/*{
** Name: csp_rcvr_cmpl_msg	- Process CSP_MSG_12_DBMS_RCVR_CMPL message.
**
** Description:
**      Process CSP_MSG_12_DBMS_RCVR_CMPL message.
**
** Inputs:
**      node				dbms stopped on this node
**      pid				dbms pid
**      lg_id_id			
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       11-may-2005 (stial01)
**          created.
**	10-Apr-2007 (jonj)
**	    Only decrement csp_dbms_rcvr_cnt if lock
**	    was granted.
*/
static VOID
csp_rcvr_cmpl_msg(i1 node, PID pid, i2 lg_id_id)
{
    CSP			*csp = &csp_global;
    STATUS		status;
    CSP_NODE_INFO	*node_info;
    CX_REQ_CB		*preq;
    CL_ERR_DESC		sys_err;
    CSP_DBMS_INFO	*dbms;
    i4			err_code;
    i4			i;
    i4			dbms_rcvr_cnt;
    bool		WasGranted;

    node_info = &csp->csp_node_info[node-1];

    if (lg_id_id >= CSP_MAX_DBMS)
    {
        REPORT_ODDITY(ERx("DBMS rcvr: invalid lg_id_id"));
	TRdisplay("%@ CSP: DBMS rcvr: node %d pid %d lgid %d\n", 
    		node, pid, lg_id_id);
	return;
    }

    /*
    ** Find slot in node_dbms array for this dbms
    **		Use lg_id_id to index into node_dbms
    ** We should still have the deadman lock on this dbms
    ** (we hold it until dbms shutdown - or deadman recovery complete)
    */
    dbms = &node_info->node_dbms[lg_id_id];
    preq = &dbms->deadman_lock_req;

    if (preq->cx_key.lk_key2 != pid)
    {
	REPORT_ODDITY( \
	     ERx("DBMS rcvr: invalid key2"));
	TRdisplay("%@ CSP: DBMS rcvr: node %d pid %d lgid %d key2 %d\n", 
    		node, pid, lg_id_id, preq->cx_key.lk_key2);
	return;
    }

    /*
    ** Discard deadman lock on this dbms.
    */
    if ( CX_NONZERO_ID( &preq->cx_lock_id ) )
    {
	/*
	** On VMS at least, dequeueing a not-yet-granted lock delivers the 
	** AST (csp_dead_dbms_notify) with a status of SS$_ABORT
	** (E_CL2C22_CX_E_DLM_CANCEL). Seeing the CANCEL,
	** csp_dead_dbms_notify() did -not- increment csp_dbms_rcvr_cnt,
	** hence we need to know here if the lock was granted or not
	** so that we don't wrongly decrement csp_dbms_rcvr_cnt and
	** never resume lock processing.
	*/

	/* If lock was granted, dbms failed, we did recovery */
	WasGranted = (preq->cx_old_mode == preq->cx_new_mode);

	status = CXdlm_release( 0, preq );
	if (status == OK)
	{
	    CX_ZERO_OUT_ID(&preq->cx_lock_id);

	    /* Decrement count of dbms servers needing recovery */
	    if ( WasGranted )
		dbms_rcvr_cnt = CSadjust_counter(&csp->csp_dbms_rcvr_cnt, (i4)-1);
	}
	else if (status == E_CL2C25_CX_E_DLM_NOTHELD)
	{
	    /* Got CSP_MSG_12_DBMS_RCVR_CMPL before notify (lock granted) */
	    status = CXdlm_cancel( 0, preq );

	    /* If cancel failed, lock has been granted since we first tried */
	    if (status == E_CL2C09_CX_W_CANT_CAN)
	    {
		status = CXdlm_release( 0, preq );
		if (status == OK)
		{
		    CX_ZERO_OUT_ID(&preq->cx_lock_id);

		    /* Decrement count of dbms servers needing recovery */
		    if ( WasGranted )
			dbms_rcvr_cnt = CSadjust_counter(&csp->csp_dbms_rcvr_cnt, (i4)-1);
		}
	    }
	}

	TRdisplay("%@ CSP: Release DMN node %d pid %d lgid %d rc %d\n",
			node, pid, lg_id_id, status);
		
    }
    else
    {
        TRdisplay("%@ CSP: Release DMN node %d pid %d lgid %d id %d\n",
	    node, pid, lg_id_id, preq->cx_dlm_ws.cx_dlm_lock_id);
    }

    preq->cx_key.lk_key1 = 0;
    preq->cx_key.lk_key2 = 0;

    /* Try to resume normal lock processing */
    try_lk_resume();

}


/*{
** Name: csp_req_dbms_dmn - Request dbms deadman lock (LK_CSP)
**
** Description:
**      Request dbms deadman lock (LK_CSP)
**
** Inputs:
**      node				dbms started on this node
**      pid				dbms pid
**      lg_id_id			
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       11-may-2005 (stial01)
**          created.
*/
static VOID
csp_req_dbms_dmn(i1 node, PID pid, i2 lg_id_id)
{
    CSP			*csp = &csp_global;
    CSP_NODE_INFO	*node_info;
    CX_REQ_CB		*preq;
    CL_ERR_DESC		sys_err;
    CSP_DBMS_INFO	*dbms;
    i4			err_code;
    i4			i;

    node_info = &csp->csp_node_info[node-1];

    if (lg_id_id >= CSP_MAX_DBMS)
    {
    	/* 
	** FIX ME 
	** lgcbmem.c expand_list (LPB_TYPE)
	** Disallow lgd_lpbb_count > CSP_MAX_DBMS 
	*/
        REPORT_ODDITY(ERx("DBMS start: invalid lg_id"));
	TRdisplay("%@ CSP: DBMS start: node %d pid %d lgid %d\n", 
    		node, pid, lg_id_id);
	return;
    }

    /*
    ** Find available slot in node_dbms array
    **		Use lg_id_id to index into node_dbms
    */
    dbms = &node_info->node_dbms[lg_id_id];
    preq = &dbms->deadman_lock_req;
    if (preq->cx_key.lk_key2 != 0)
    {
	REPORT_ODDITY( \
	     ERx("DBMS start: invalid key2 "));
	TRdisplay("%@ CSP: DBMS start: node %d pid %d lgid %d key2 %d\n", 
    		node, pid, lg_id_id, preq->cx_key.lk_key2);
	if (preq->cx_key.lk_key2 == pid)
	    return;
	else
	{
	    /*
	    ** This might happen if server dies, recovery completes, and
	    ** another server is started before we get the
	    ** CSP_MSG_12_DBMS_RCVR_CMPL for the dead server
	    ** If so, clean up the first server before starting another
	    */
	    csp_rcvr_cmpl_msg(node, preq->cx_key.lk_key2, lg_id_id);
	}
    }

    /* preq is &dbms->deadman_lock_req */
    preq->cx_new_mode = LK_S;
    preq->cx_key.lk_type = LK_CSP;
    preq->cx_key.lk_key1 = node;
    preq->cx_key.lk_key2 = pid;
    preq->cx_user_func = csp_dead_dbms_notify;
    preq->cx_dlm_ws.cx_dlm_lock_id = 0;
    err_code = CXdlm_request(
      CX_F_STATUS | CX_F_PCONTEXT | CX_F_NODEADLOCK |
      CX_F_PRIORITY | CX_F_IGNOREVALUE | CX_F_NOTIFY |
      CX_F_NO_DOMAIN, preq, &csp->csp_tranid );
    if ( err_code == OK )
    {
	TRdisplay("%@ CSP: Request DMN node %d pid %d lgid %d id %d\n",
		node, pid, lg_id_id, preq->cx_dlm_ws.cx_dlm_lock_id);
    }
    else if ( E_CL2C01_CX_I_OKSYNC == err_code )
    {
	/* 
	** If this is grantable, 'dbms' must have died.
	** message handle may have already canceled lock we just obtained
	*/
	preq->cx_key.lk_key1 = 0;
	preq->cx_key.lk_key2 = 0;
	if ( CX_NONZERO_ID( &preq->cx_lock_id ) )
	{
	    (void)CXdlm_release( CX_F_IGNOREVALUE, preq );
	}
    }
    else
    {
	/* 
	** Fatal error, we must obtain all valid
	** DMN locks for dbms to join the cluster.
	*/
	preq->cx_key.lk_key1 = 0;
	preq->cx_key.lk_key2 = 0;
	TRdisplay("%@ CSP: Request DMN node %d pid %d lgid %d error %d\n",
		node, pid, lg_id_id, err_code);
	crash(E_DM991A_CSP_NODE_LOCK, NULL );
    }

    return;
}


/*{
** Name: csp_rls_dbms_dmn - Release dbms deadman lock (LK_CSP)
**
** Description:
**      Release dbms deadman lock (LK_CSP)
**
** Inputs:
**      node				dbms stopped on this node
**      pid				dbms pid
**      lg_id_id			
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       11-may-2005 (stial01)
**          created.
*/
static VOID
csp_rls_dbms_dmn(i1 node, PID pid, i2 lg_id_id)
{
    CSP			*csp = &csp_global;
    CSP_NODE_INFO	*node_info;
    CX_REQ_CB		*preq;
    CL_ERR_DESC		sys_err;
    CSP_DBMS_INFO	*dbms;
    i4			err_code;
    STATUS		status;

    node_info = &csp->csp_node_info[node-1];

    if (lg_id_id >= CSP_MAX_DBMS)
    {
    	/* 
	** FIX ME 
	** lgcbmem.c expand_list (LPB_TYPE)
	** Disallow lgd_lpbb_count > CSP_MAX_DBMS 
	*/
        REPORT_ODDITY(ERx("DBMS stop: invalid lg_id"));
	TRdisplay("%@ CSP: DBMS stop: node %d pid %d lgid %d\n", 
    		node, pid, lg_id_id);
	return;
    }

    /*
    ** Find available slot in node_dbms array
    **		Use lg_id_id to index into node_dbms
    */
    dbms = &node_info->node_dbms[lg_id_id];
    preq = &dbms->deadman_lock_req;
    if (preq->cx_key.lk_key2 != pid)
    {
	REPORT_ODDITY( \
	     ERx("DBMS stop: invalid key2 "));
	TRdisplay("%@ CSP: DBMS stop: node %d pid %d lgid %d key2 %d\n", 
    		node, pid, lg_id_id, preq->cx_key.lk_key2);
    }

    /*
    ** Discard deadman lock on this dbms.
    ** For normal dbms server shutdown, we will need to 
    ** cancel the lock (because it hasn't been granted yet)
    */
    if ( CX_NONZERO_ID( &preq->cx_lock_id ) )
    {
	status = CXdlm_cancel( 0, preq );
	CX_ZERO_OUT_ID(&preq->cx_lock_id);
	TRdisplay("%@ CSP: Cancel  DMN node %d pid %d lgid %d rc %d\n", 
			node, pid, lg_id_id, status);
    }
    else
    {
        TRdisplay("%@ CSP: Cancel  DMN node %d pid %d lgid %d id %d\n",
	    node, pid, lg_id_id, preq->cx_dlm_ws.cx_dlm_lock_id);
    }

    preq->cx_key.lk_key1 = 0;
    preq->cx_key.lk_key2 = 0;

    return;

}


/*{
** Name: try_lk_resume	- Try to resume normal lock processing
**
** Description:
**      Resume normal lock processing only if we are not waiting for
**      completion of node or dbms recovery.
**
** Inputs:
**	    none
**
** Outputs:
**	Returns:
**	    none
**
** Side Effects:
**          none
**
** History:
**       11-may-2005 (stial01)
**          created.
*/
static STATUS
try_lk_resume()
{
    CSP			*csp = &csp_global;
    CL_ERR_DESC		sys_err;
    STATUS		status;
    STATUS		local_status;
    i4			err_code;

    for ( ; ; )
    {
	if ( !csp->csp_node_failure  && csp->csp_dbms_rcvr_cnt == 0)
	{
	    /*
	    ** Allow all normal lock processing to resume.
	    ** Must not be any outstanding node failures or dbms failures
	    */
	    TRdisplay("%@ CSP: Resume lock processing\n");

	    status = LKalter(LK_A_STALL_NO_LOCKS, 0, &sys_err);
	    if (status != OK)
	    {
		_VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		err_code = E_DM990A_CSP_STALL_CLEAR;
		break;
	    }

	    /* Restart any deadlock searches still required. */
	    status = LKalter(LK_A_CLR_DLK_SRCH, 0, &sys_err);
	    if (status != OK)
	    {
		_VOID_ uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
		err_code = E_DM990B_CSP_DEADLOCK_CLEAR;
		break;
	    }

	    uleFormat(NULL, I_DM9936_CSP_LOCK_PROC_RESUMED, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_status, 0);
	}
	else
	{
	    TRdisplay("%@ CSP: Can't resume lock processing yet (%d,%d)\n",
		csp->csp_node_failure, csp->csp_dbms_rcvr_cnt);
	}

	break; /* ALWAYS */
    }
}


#else /* conf_CLUSTER_BUILD */

/* a useless something to satisfy certain compilers. */
static i4 not_a_cluster_build;

#endif /* conf_CLUSTER_BUILD */

