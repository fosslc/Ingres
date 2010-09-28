/*
** Copyright (c) 1992, 2004 Ingres Corporation
**
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <dbms.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>	/* to enable inclusion of lkdef.h, below */
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <lkdef.h>	/* to pick up LK_csp_rcp_online prototype */
#include    <dm0c.h>	/* Needed by dm0j.h */
#include    <dm0j.h>	/* where DM0J_HEADER lives */

/**
**
**  Name: LGalter.C - Implements the LGalter function of the logging system
**
**  Description:
**	This module contains the code which implements LGalter.
**	
**	    LGalter -- Alter characteristics of the logging system
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	7-oct-1992 (bryanp)
**	    Include bt.h.
**	16-oct-1992 (jnash) merged 29-jul-1992 (rogerk)
**	    Reduced logging Project: Changed actions associated with some of
**	    the Consistency Point protocols. The system now requires
**	    that all servers participate in Consistency Point flushes, not
**	    just fast commit servers:
**	      - Added notion of CPAGENT process status.  This signifies that
**		the process controls a data cache that must be flushed when
**		a consistency point is signaled.  Currently, all DBMS servers
**		will have this status and no other process type will.
**	      - Added alter entry point LG_A_CPAGENT which causes the CPAGENT
**		process status to be defined in the current process.
**	      - Changed the LG_A_BCPDONE case to signal the CPFLUSH event if 
**		there are any CPAGENT servers active rather than just 
**		fast commit ones.
**	      - Changed the calls to check_fc_servers in LG_A_CPFDONE and
**		LG_A_CPWAITDONE cases to check_cp_servers.
**	      - removed references to obsolete lgd->lgd_n_servers counter.
**	27-oct-1992 (jnash) merged 19-oct-1992 (bryanp)
**	    Fix LG_A_DBRECOVER so it doesn't corrupt the LPB list.
**	19-oct-1992 (bryanp)
**	    Fix LG_A_DBRECOVER so it doesn't corrupt the LPB list.
**	20-oct-1992 (bryanp)
**	    Fix LG_A_DISABLE_DUAL_LOGGING.
**	18-jan-1992 (rogerk)
**	    Changed LG_A_CONTEXT to no longer be strictly a 2PC command.  It
**	    now restores the transaction context similar to before, but does
**	    not set the transaction into WILLING COMMIT state.  2PC recovery
**	    must now do a LG_A_WILLING_COMMIT following the LG_A_CONTEXT.  This
**	    change was made to allow LG_A_CONTEXT to be used to restore normal
**	    transaction context for Undo recovery.
**	    Added LG_A_LXB_OWNER alter to allow the RCP to take ownership
**	    of transactions that it must recover.
**	18-jan-1993 (rogerk)
**	    Added CL_CLEAR_ERR call.
**	18-jan-1992 (rogerk)
**	    Changed LG_A_ARCHIVE alter command. This is now signaled at the
**	    end of each archive cycle.  It now does the processing which used
**	    to be done by LG_A_PURGE and LG_A_BOF commands.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added LG_A_DBCONTEXT: used to restore lg database information
**		to the state a database was in previously for restart recovery.
**		This used to be done by special flags to LGadd.
**	    Removed LG_A_JCP_ADDR_SET, LG_A_EBACKUP, and LG_A_SONLINE_PURGE 
**		alter cases.  These are no longer used.
**		Removed corresponding ebackup and fbackup database states.
**	    Renamed LG_A_ARCHIVE to LG_A_ARCHIVE_DONE.
**	    Added new LG_A_ARCHIVE entrypoint which causes an archive event.
**	26-apr-1993 (andys)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and lga_low with la_offset.
**	    LG_allocate_buffers takes an explicit log page size. We can no
**	    longer assume that the lfb's lg_header is valid.
**	24-may-1993 (bryanp)
**	    Add support for 2K page size.
**	    Fix system journal window handling bug in LG_A_DBCONTEXT.
**	21-jun-1993 (bryanp)
**	    CSP/RCP process linkage: when logging system goes online, tell CSP.
**	21-jun-1993 (rogerk)
**	    Changed LG_A_CONTEXT processing to mark restored transaction as
**	    active, recover only if it is a protected transaction.  Nonprotect
**	    transactions should never be transitioned to active state.  Rolldb
**	    restores non-protect transactions during undo processing.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new archive_window and database jnl/dmp_la fields.
**	23-aug-1993 (bryanp)
**	    Removed old, unused ifdef's for resource allocation monitoring.
**	    Work on making LG_A_ABORT operate correctly.
**	18-oct-1993 (rogerk)
**	    Added new protocols of disowning transactions from the caller's
**	    context when Willing Commit disassociations and Pass Abort calls
**	    are made.   Changed LG_A_PASS_ABORT and LG_A_REASSOC cases to
**	    call LG_orphan_xact to release the current transaction from its
**	    current process context and link it to the RCP.  Changed the
**	    LG_A_OWNER case to call LG_adopt_xact to restore ownership of
**	    the orphaned transaction.
**	    Made logfull limit maximum be 96% to give the logging system
**	    some additional space for recovery in case logspace reservation
**	    algorithms fail.
**	18-oct-1993 (rogerk)
**	    Removed unused LG_A_DMU alter operation.
**	    Added LG_A_UNRESERVE_SPACE call to free up space allocated by
**	    the LG_RS_FORCE flag to LGreserve.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changing %x to %p for pointer values in LG_allocate_buffers().
**	31-jan-1994 (bryanp) B56472
**	    Set lfb_active_log to lgh_active_logs in alter_header. Thus, when
**		a caller forcibly alters the entire log header, the in-memory
**		lfb_active_log will remain in sync. (Perhaps, the entire
**		lfb_active_log field should be replaced by
**		lfb_header.lgh_active_logs, but not today.)
**	28-mar-94 (mikem)
**	    Bug #60089
**	    Change LG_A_FORCE_ABORT option to correctly pick up the second 
**	    element from the item buffer.  Prior to this change, due to a 
**	    problem with address arithmetic, the abort handler would be set
**	    to whatever was 16 bytes (on a sun) from the beginning of the item 
**	    buffer (rather than sizeof(LG_ID) bytes).  The result of the bug
**	    was that the server would get an AV anytime a force abort was
**	    encountered.
**	13-apr-94 (swm)
**	    Bug #62663
**	    Change LG_A_FORCE_ABORT option to correctly pick up the second
**	    element from the item buffer. The item buffer is declared as an
**	    array of PTR in back/dml/dmcstart.c, not as LG_ID; though in fact,
**	    the first item in the array is an LG_ID and the second item is a
**	    pointer to the force abort call back function. Prior to this
**	    change the rcp would core dump with a bus error on axp_osf, other
**	    platforms such as sun were unaffected because the size of an
**	    LG_ID happens to be the same as the size of a pointer.
**	23-may-94 (jnash)
**	    Add LGalter(LG_A_LFLSN) to set the last forced lsn.  This should
**	    be called prior to doing log forces.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      20-may-95 (hanch04)
**	    removed NO_OPTIM for su4_us5, sun patch, Hi
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be named
**	    in calls to LKMUTEX.C. Many new semaphores defined
**	    to ease load on single lgd_mutex.
**	    lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**	    (LDBs) to ensure that semaphores now embedded in those
**	    structures are not corrupted. Similarly, sbk_table 
**	    replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**	    lpdb_table (LPDs).
**    06-mar-1996 (stial01 for bryanp)
**        Added support for large page sizes. In particular, since DMF may
**            now request the logging of a 66,000 byte log record, we must
**            ensure that there are sufficient number of log page buffers
**            allocated to support that request.
**	11-Sep-1996 (jenjo02)
**	    In LG_A_BCNT, acquire and release local_lfb.lfb_mutex when calling
**	    LG_allocate_buffers() rather than letting that function acquire
**	    and release the mutex. 
**	13-Dec-1996 (jenjo02)
**	    To avoid deadlocking with LK, don't allow new protected update 
**	    transactions to begin while stalling a DB for checkpoint.
**	    Bug 67888, previously addressed by defining a new lock type
**	    of LK_CKP_TXN and taking that lock each time a transaction
**	    or CKPDB starts. The LK_CKP_TXN lock has proven to be very
**	    contentious, so this solution is offered in its place.
**
**	    When a checkpoint is beginning on a database, delay the 
**	    checkpoint until all protected update transactions (active or
**	    inactive) have completed. At the same time, don't allow
**	    new update transactions to start until the stall condition
**	    has cleared. The fledging LXBs are allocated, left
**	    incomplete, and suspended. When the stall condition clears,
**	    the waiting LXBs are resumed, completed, and placed on
**	    the inactive queue.
**	09-Jan-1996 (jenjo02)
**	    In LG_A_CKPERROR, call LG_signal_event() instead of LG_resume()
**	    to wake up online backup event waiters.
**	22-Sep-1997 (jenjo02)
**	    Repositioned code in LG_A_DBRECOVER such that the lpb_q_mutex is
**	    released prior to calling LG_last_cp_server() which calls
**	    LG_check_cp_servers() which tries to acquire the same mutex
**	    and deadlocks.
**	13-Nov-1997 (jenjo02)
**	    Revision of 13-Dec-1996 change. While online checkpoint is stalled
**	    txns attempting to write a delayed BT will be allowed to procede;
**	    those txns are holding locks and to make them wait on the CKPDB stall
**	    is to set ourselves up for a deadlock.
**      08-Dec-1997 (hanch04)
**	    Calculate number of blocks from new block field in 
**	    LG_LA for support of logs > 2 gig
**	18-Dec-1997 (jenjo02)
**	    Added LG_my_pid GLOBAL.
**	29-Jan-1998 (jenjo02)
**	    Changed lpb_gcmt_sid to lpb_gcmt_lxb, removed lpb_gcmt_asleep.
**	24-Aug-1998 (jenjo02)
**	    LG_A_BLKS; allocate and initialize transaction hash buckets.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity, changed to CS_CPID structure
**	    from PID and SID.
**	11-Dec-1998 (jenjo02)
**	    LG_A_ECPDONE: resume stalled threads AFTER clearing stall conditions.
**	16-Dec-1998 (kinpa04)
**	    In the LG_A_DBRECOVER case for a process that has multiple
**	    databases, the LG_unmutex was being attempted on the wrong
**	    LDB mutex after running the lpb_lpd chain.
**	22-Dec-1998 (jenjo02)
**	    Moved define of RAWIO_ALIGN from lgdef.h to dicl.h (DI_RAWIO_ALIGN)
**	02-Mar-1999 (jenjo02)
**	    LG_A_FORCE_ABORT 1st "item" is now pointer to log_id instead of
**	    log_id itself, which caused 64-bit problems.
**	15-Mar-1999 (jenjo02)
**	    New CKPDB stall scheme, see LG_A_SBACKUP.
**	26-Apr-1999 (jenjo02)
**	    In LG_A_DBRECOVER, remove LDB assignment that could corrupt the
**	    pointer to the LDB being recovered, leave ldb_mutex locked, and
**	    cause recovery failures.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. Store the ldb_eback_lsn in lgalter call
**	    right before resetting the DCB_S_BACKUP.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    When initializing, all but the first log buffer go on the
**	    wait queue, the first becomes the "current" buffer.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED log transactions in LG_A_PASS_ABORT,
**	    LG_A_SERVER_ABORT, LG_A_WILLING_COMMIT, LG_A_REASSOC,
**	    added LG_A_PREPARE.
**	14-Mar-2000 (jenjo02)
**	    Removed static LG_alter() function and the unnecessary level
**	    of indirection it induced.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-Apr-2001 (hanje04)
**          Removed unnecissary check to see if Tx is ACTIVE & PROTECT
**          before aborting it. Some Tuxedo trasactions have INACTIVE status
**          and therefore could not be aborted. (bug 104433)
**      01-feb-2001 (stial01)
**          hash on UNIQUE low tran (Variable Page Type SIR 102955)
**	29-may-2001 (devjo01)
**	    remove call to LGc_rcp_enqueue. (s103715)
**	08-Apr-2002 (devjo01)
**	    Add 'lfb_first_lbb' to remember beginning of contiguous
**	    allocation for LBB and associated log buffers.
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	08-Apr-2004 (jenjo02)
**	    Added LG_A_BMINI, LG_A_EMINI to single-thread
**	    mini-transactions in shared log contexts.
**	    Because a thread may now have to wait for a mini
**	    to end, reinstituted LG_alter() static function 
**	    to set up the CSsuspend.
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      07-dec-2004 (stial01)
**          LK_CKP_TXN: key change to include db_id
**          LK_CKP_TXN: key change remove node id, LKrequest LK_LOCAL instead
**      23-may-2005 (stial01)
**          LG_A_DBRECOVER: send dbms recovery complete msg.
**	28-may-2005 (devjo01)
**	    Rename dmfcsp_dbms_rcvry_cmpl to LG_rcvry_cmpl() as code
**	    has been moved to here from DMFCSP to prevent linkage
**	    problems in IPM.
**	26-Jul-2005 (jenjo02)
**	    Hold associated mutex across LG_resume call.
**      31-mar-2006 (stial01)
**          LG_A_SJSWITCH: Don't return error if LDB_JSWITCH already set
**          Removed LG_A_DJSWITCH, not used anymore
**      08-Jun-2005 (horda03) Bug 114666/INGDBA322
**          Hold the lgd->lgd_ldb_q_mutex while setting the LDB_BACKUP
**          flag. This prevents window where the LGD_CKP_SBACKUP flag
**          is cleared in error by a terminating CKPDB.
**	20-Sep-2006 (kschendel)
**	    lg-resume signature changed, update calls here.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      31-jan-2008 (stial01)
**          Added -rcp_stop_lsn, to set II_DMFRCP_STOP_ON_INCONS_DB
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      05-may-2009
**          Fixes for II_DMFRCP_PROMPT_ON_INCONS_DB / Db_incons_prompt
**      18-jun-2009 (stial01) b121849
**          LG_A_XA_DISASSOC compare GTRID only if looking for other branches
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** Forward declarations for static functions:
*/
static VOID	alter_header(LFB *lfb, LG_HEADER *header);

static STATUS LG_alter(
		    i4              flag,
		    PTR		    item,
		    i4		    l_item,
		    CL_ERR_DESC	    *sys_err);

/*{
** Name: LGalter	- Alter log information
**
** Description:
**      This routine informs the log system of a requested change to a log
**	information variable.  Most of the alterable pieces of information
**	are sizes of various internal data structures and can only be changed
**	when no process is currently using the log system.
**      The type of information can be altered includes:  
**	The size of various tables, the size and number of log buffers, 
**	and the contents of the log file header.
**
**	Alter is also used as a mechanism to signal events to the Logging
**	System, and to drive event protocols used by the Recovery System.
**
** Inputs:
**      flag		Alter option - one of following:
**			    LG_A_BCNT - change count of log buffers used by the
**				logging system.  Can only be called during
**				logging system startup.
**			    LG_A_BLKS - change number of transactions that LG
**				will allow to be active at one time.  Cannot be
**				set while the logging system is in use.
**			    LG_A_LDBS - change number of databases that LG will
**				allow to be open at one time.  Cannot be set
**				while the logging system is in use.
**			    LG_A_HEADER - write new log header.
**			    LG_A_BOF - set new beginning of file point for the
**				wrap-around log file.
**			    LG_A_ARCHIVE - signal an archive event.
**			    LG_A_ARCHIVE_DONE - turn off the archive event.
**			    LG_A_EOF - set new end of file point for the
**				wrap-around log file.
**			    LG_A_STAMP - set cluster log sequence stamp.
**			    LG_A_CPFDONE - used by server to indicate that it
**				is finished with work required by CPFLUSH event.
**			    LG_A_CPWAITDONE - used by server to indicate that
**				it is ready for the next CPFLUSH event.
**			    LG_A_FORCE_ABORT - set address of routine in server
**				to be called to force abort a transaction.
**			    LG_A_SBACKUP - calls to drive online backup 
**			    LG_A_RESUME	    protocols.
**			    LG_A_DBACKUP
**			    LG_A_DBRECOVER - signal completion of recovery on
**				specified database.
**			    LG_A_DB - mark database inconsistent.
**			    LG_A_PURGEDB - 
**			    LG_A_OPENDB - mark database open.
**			    LG_A_CLOSEDB - mark database closed.
**			    LG_A_CPA - set the current consistency point
**				address.
**			    LG_A_ONLINE - mark logging system on/off line.
**			    LG_A_ACPOFFLINE - mark archiver on/off line.
**			    LG_A_BCPDONE - Signal that BCP record is written.
**			    LG_A_ECPDONE - Signal completion of Consistency
**				Point.
**			    LG_A_RECOVERDONE - Signal that RCP is done with
**				recovery and installation is consistent.
**			    LG_A_M_ABORT - turn off manual abort status.
**			    LG_A_M_COMMIT - turn off manual commit status.
**			    LG_A_SHUTDOWN - shut down recovery system.
**			    LG_A_PASS_ABORT - signal recovery system to take
**				over the abort of a transaction.
**			    LG_A_SERVER_ABORT - mark transaction status to
**				be server-abort.
**			    LG_A_OFF_SERVER_ABORT - mark transaction status to
**				not be server-abort.
**			    LG_A_COMMIT - turn on manual commit status.
**			    LG_A_ABORT - turn on manual abort status.
**			    LG_A_PREPARE - prepares transaction for
**				willing commit.
**			    LG_A_WILLING_COMMIT - mark transaction in willing
**				commit state.
**			    LG_A_OWNER - adopt willing commit transaction.
**			    LG_A_CONTEXT - restore the context of a transaction
**				during recovery to its state prior to a crash.
**			    LG_A_REASSOC - set transaction state to wait for
**				reassociation.
**			    LG_A_NODEID - set logging system cluster node id.
**			    LG_A_BUFMGR_ID - Register server's buffer manager
**				id.  A server that connects to a shared buffer
**				manager is expected to make this call to record
**				the id of the buffer manager it is using.  This
**				enables the recovery system to clean up after
**				a crash involving a shared buffer manager.
**                          LG_A_RCPRECOVER - mark system status that the RCP
**                              is doing recovery.
**			    LG_A_CPNEEDED - signal logging system to perform
**				a new consistency point.
**			    LG_A_CKPERROR - signal that checkpoint on the given
**				database has failed.  Item indicates the db.
**			    LG_A_ENABLE_DUAL_LOGGING - turn dual logging on.
**			    LG_A_DISABLE_DUAL_LOGGING - turn dual logging off.
**			    LG_A_DUAL_LOGGING - clear dual logging status bit
**			    LG_A_SERIAL_IO - request serial I/O mode
**			    LG_A_TP_ON - set an LG testpoint on.
**			    LG_A_TP_OFF - set an LG testpoint off.
**			    LG_A_CPAGENT - mark current process as needing
**				to participate in Consistency Points.  This
**				means it is a server with a Consistency Point
**				thread.
**			    LG_A_LXB_OWNER - set lxb as owned by calling proc.
**				Used by the RCP to gain ownership of xacts it
**				must recover.
**			    LG_A_DBCONTEXT - restore ldb information to state
**				previous to crash for restart recovery.
**			    LG_A_LFLSN - Set last forced lsn.
**			    LG_A_SJSWITCH - signal logging system to perform
**				a journal switch.
**			    LG_A_BMINI - Begin a mini-transaction.
**			    LG_A_EMINI - End a mini-transaction.
**			    LG_A_OPTIMWRITE - Toggle optimized writes.
**			    LG_A_OPTIMTRACE - Toggle optimized writes
**					      tracing.
**			    LG_A_JFIB
**			    LG_A_JFIB_DB - Set the contents of a LG_JFIB
**					   from values in the config file.
**			    LG_A_ARCHIVE_JFIB - Check LG_JFIB at archive SOC
**			    LG_A_RBBLOCKS - Set readbackward block count in LGD
**			    LG_A_RFBLOCKS - Set readforward block count in LGD
**
**	item				Pointer to buffer containing new value
**					based on the flag setting.
**	l_item				Length of the buffer.
**
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM
**	    other
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	16-oct-1992 (jnash) merged 29-jul-1992 (rogerk)
**	    6.5 Recovery Project: Changed actions associated with some of
**	    the Consistency Point protocols. The system now requires
**	    that all servers participate in Consistency Point flushes, not
**	    just fast commit servers:
**	      - Added notion of CPAGENT process status.  This signifies that
**		the process controls a data cache that must be flushed when
**		a consistency point is signaled.  Currently, all DBMS servers
**		will have this status and no other process type will.
**	      - Added alter entry point LG_A_CPAGENT which causes the CPAGENT
**		process status to be defined in the current process.
**	      - Changed the LG_A_BCPDONE case to signal the CPFLUSH event if 
**		there are any CPAGENT servers active rather than just 
**		fast commit ones.
**	      - Changed the calls to check_fc_servers in LG_A_CPFDONE and
**		LG_A_CPWAITDONE cases to check_cp_servers.
**	      - removed references to obsolete lgd->lgd_n_servers counter.
**	27-oct-1992 (jnash) merged 19-oct-1992 (bryanp)
**	    Fix LG_A_DBRECOVER so it doesn't corrupt the LPB list.
**	20-oct-1992 (bryanp)
**	    Fix LG_A_DISABLE_DUAL_LOGGING.
**	18-jan-1992 (rogerk)
**	    Changed LG_A_CONTEXT to no longer be strictly a 2PC command.  It
**	    now restores the transaction context similar to before, but does
**	    not set the transaction into WILLING COMMIT state.  2PC recovery
**	    must now do a LG_A_WILLING_COMMIT following the LG_A_CONTEXT.  This
**	    change was made to allow LG_A_CONTEXT to be used to restore normal
**	    transaction context for Undo recovery.
**	    Added LG_A_LXB_OWNER alter to allow the RCP to take ownership
**	    of transactions that it must recover.
**	18-jan-1992 (rogerk)
**	    Changed LG_A_ARCHIVE alter command. This is now signaled at the
**	    end of each archive cycle.  It now does the processing which used
**	    to be done by LG_A_PURGE and LG_A_BOF commands.  Added new
**	    ldb_j_last_la field.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added LG_A_DBCONTEXT: used to restore lg database information
**		to the state a database was in previously for restart recovery.
**		This used to be done by special flags to LGadd.
**	    Removed LG_A_JCP_ADDR_SET, LG_A_EBACKUP, and LG_A_SONLINE_PURGE 
**		alter cases.  These are no longer used.
**		Removed corresponding ebackup and fbackup database states.
**	    Renamed LG_A_ARCHIVE to LG_A_ARCHIVE_DONE.
**	    Added new LG_A_ARCHIVE entrypoint which causes an archive event.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Revive LG_A_NODEID -- it turns out it's still needed.
**		When we signal the backup to start, we record the current EOF in
**		    the ldb_sbackup field of the LDB. This will be used by
**		    higher-level DMF protocols to decide whether a page has
**		    been changed since the backup began. DMF, however, needs
**		    an LSN, not an LG_LA, to make this check, since it wishes
**		    to compare a log sequence number on a page against the LSN
**		    of the start of the backup. Therefore, we also set
**		    ldb->ldb_sback_lsn to be the most recent LSN that we have
**		    assigned.
**		Record the CSP's PID in lgd_csp_pid.
**		Various LGalter calls now must pass additional information
**		    indicating which logfile is to be process (this is an issue
**		    in a multi-node logging system).
**	24-may-1993 (bryanp)
**	    Add support for 2K page size.
**	    Fix system journal window handling bug in LG_A_DBCONTEXT.
**	21-jun-1993 (bryanp)
**	    CSP/RCP process linkage: when logging system goes online, tell CSP.
**	21-jun-1993 (rogerk)
**	    Changed LG_A_CONTEXT processing to mark restored transaction as
**	    active, recover only if it is a protected transaction.  Nonprotect
**	    transactions should never be transitioned to active state.  Rolldb
**	    restores non-protect transactions during undo processing.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new archive_window and database jnl/dmp_la fields.
**	23-aug-1993 (bryanp)
**	    Removed old, unused ifdef's for resource allocation monitoring.
**	    Work on making LG_A_ABORT operate correctly.
**	18-oct-1993 (rogerk)
**	    Added new protocols of disowning transactions from the caller's
**	    context when Willing Commit disassociations and Pass Abort calls
**	    are made.  Changed LG_A_PASS_ABORT and LG_A_REASSOC cases to
**	    call LG_orphan_xact to release the current transaction from its
**	    current process context and link it to the RCP.  Changed the
**	    LG_A_OWNER case to call LG_adopt_xact to restore ownership of
**	    the orphaned transaction and to turn off LXB_REASSOC_WAIT.
**	    Made pass-abort of failures during rollback of a willing commit
**	    transaction work by turning on the man-abort flag.
**	    Made CP space calculation in ECPDONE case not overflow on
**	    MAXI4 size logfiles.  Fixed ECPDONE to use true logspace 
**	    calculation to determine if logfull can be turned off rather than
**	    using the space used since the last CP.
**	    Removed unused LG_A_DMU alter operation.
**	    Added LG_A_UNRESERVE_SPACE call to free up space allocated by
**	    the LG_RS_FORCE flag to LGreserve.
**	10-oct-93 (swm)
**	    Bug #56441
**	    Change LG_A_FORCE_ABORT option to expect a two element buffer
**	    containing an lg process id and an abort handler function
**	    pointer.
**	28-mar-94 (mikem)
**	    Bug #60089
**	    Change LG_A_FORCE_ABORT option to correctly pick up the second 
**	    element from the item buffer.  Prior to this change, due to a 
**	    problem with address arithmetic, the abort handler would be set
**	    to whatever was 16 bytes (on a sun) from the beginning of the item 
**	    buffer (rather than sizeof(LG_ID) bytes).  The result of the bug
**	    was that the server would get an AV anytime a force abort was
**	    encountered.
**	13-apr-94 (swm)
**	    Bug #62663
**	    Change LG_A_FORCE_ABORT option to correctly pick up the second
**	    element from the item buffer. The item buffer is declared as an
**	    array of PTR in back/dml/dmcstart.c, not as LG_ID; though in fact,
**	    the first item in the array is an LG_ID and the second item is a
**	    pointer to the force abort call back function. Prior to this
**	    change the rcp would core dump with a bus error on axp_osf, other
**	    platforms such as sun were unaffected because the size of an
**	    LG_ID happens to be the same as the size of a pointer.
**	23-may-94 (jnash)
**	    Add LGalter(LG_A_LFLSN) to set the last forced lsn.  This should
**	    be called prior to doing any log forces.  Also fixed error
**	    message text noted in LG_A_UNRESERVE_SPACE.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	05-Jan-1996 (jenjo02)
**	    lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**	    (LDBs) to ensure that semaphores now embedded in those
**	    structures are not corrupted. Similarly, sbk_table 
**	    replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**	    lpdb_table (LPDs).
**	28-Feb-1996 (jenjo02)
**	    Added lgd_n_cpagents to keep count of non-VOID cpagent
**	    LPBs. Use this number in LG_A_BCPDONE instead of locking
**	    and walking the lpb queue.
**    06-mar-1996 (stial01 for bryanp)
**        Added support for large page sizes. In particular, since DMF may
**            now request the logging of a 66,000 byte log record, we must
**            ensure that there are sufficient number of log page buffers
**            allocated to support that request.
**	11-Sep-1996 (jenjo02)
**	    In LG_A_BCNT, acquire and release local_lfb.lfb_mutex when calling
**	    LG_allocate_buffers() rather than letting that function acquire
**	    and release the mutex. 
**	09-Jan-1996 (jenjo02)
**	    In LG_A_CKPERROR, call LG_signal_event() instead of LG_resume()
**	    to wake up online backup event waiters.
**	13-Nov-1997 (jenjo02)
**	    Added LG_A_SBACKUP_STALL which sets LDB_STALL_RW to stall all
**	    newly starting read-write txns.
**	12-Nov-1998 (kitch01)
**	    (hanje04) X-Integ of 439570
**		Bug 90140. Reset the LDB_CLOSE_WAIT flag and resume the open
**		event after the database has been succesfully closed.
**      08-Jan-1998 (hanch04)
**          Update the lgh_last_lsn when updating the lfb_forced_lsn
**          during lgalter with LG_A_LFLSN
**	11-Dec-1998 (jenjo02)
**	    LG_A_ECPDONE: resume stalled threads AFTER clearing stall conditions.
**	02-Mar-1999 (jenjo02)
**	    LG_A_FORCE_ABORT 1st "item" is now pointer to log_id instead of
**	    log_id itself, which caused 64-bit problems.
**	15-Mar-1999 (jenjo02)
**	    New CKPDB stall scheme, see LG_A_SBACKUP.
**	26-Apr-1999 (jenjo02)
**	    In LG_A_DBRECOVER, remove LDB assignment that could corrupt the
**	    pointer to the LDB being recovered, leave ldb_mutex locked, and
**	    cause recovery failures.
**	26-Apr-2000 (thaju02)
**	    In LG_A_SBACKUP, set LDB_CKPLK_STALL in ldb_status. Added
**	    case for LG_A_LBACKUP. (B101303)
**	02-May-2003 (jenjo02)
**	    Mutex current_lbb while fetching lgh_end
**	    to ensure consistency in LG_A_SBACKUP.
**	08-Apr-2004 (jenjo02)
**	    Added LG_A_BMINI, LG_A_EMINI to single-thread
**	    mini-transactions in shared log contexts.
**	    Because a thread may now have to wait for a mini
**	    to end, reinstituted LG_alter() static function 
**	    to set up the CSsuspend.
**	17-May-2004 (jenjo02)
**	    Fixed LG_A_UNRESERVE_SPACE to reference SHARED LXB
** 	    rather than HANDLE; the reserve is maintained in
**	    the sLXB.
**	07-Apr-2005 (schka24)
**	    Altering the context of an already-active transaction to active
**	    is a bad idea, because it introduces loops into the active
**	    transaction queues.  Stop doing that.
**	6-Jan-2006 (kschendel)
**	    Reservation totals need to be u_i8 for large logs, fixes here.
**	15-Mar-2006 (jenjo02)
**	    LXB wait reason defines moved to lg.h
**	21-Mar-2006 (jenjo02)
**	    Add LG_A_OPTIMWRITE, LG_A_OPTIMWRITE_TRACE
**      03-Apr-2006 (hanal04) Bug 115932
**	    Acquire the lgd_ldb_q_mutex and lgd_lxbb_mutex when initiating
**	    LG_A_BCPDONE and then release then when LG_A_BCPDONE completes.
**	    This prevent DBs being added to the CP_DB list and TXs being added
**	    to the CP_TX list whilst dm0l_bcp() constructs the two lists.
**	21-Jun-2006 (jonj)
**	    Changes for XA integration:
**	      o Add LG_A_LOCKID, LG_A_TXN_STATE, LG_A_XA_DISASSOC.
**	      o Modify LG_A_SHUTDOWN to abort orphaned XA transactions.
**	      o Modify LG_A_COMMIT/ABORT to deal with orphaned XA transactions.
**	01-Aug-2006 (jonj)
**	    Deal with TMJOINed connections to XA transaction branches when
**	    handling LG_A_XA_DISASSOC.
**	24-Aug-2006 (jonj)
**	    LG_A_SERVER_ABORT: Discard disassociated handles, then see what's
**	    left before deciding what to do next.
**	28-Aug-2006 (jonj)
**	    LG_A_BLKS: allocate/setup space for (new) LFD structure.
**	    Add LG_A_FILE_DELETE to enqueue an XA transaction branch's
**	    file delete information to the Global Transaction.
**	11-Sep-2006 (jonj)
**	    In LG_A_BCPDONE, take lgd_mutex before any others to avoid
**	    deadlock with LG_archive_complete.
**	    Oops, change all those "LK_mutex/unmutex" to LG.
**	26-Oct-2006 (bonro01)
**	    Back out change 481849 for bug 115932.
**	01-Nov-2006 (jonj)
**	    Repair BCPSTALL mechanism (bug 115932), ldb_q_mutex/ldb_mutex
**	    order.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Set offset to lgd_xid_array and its size.
**	    In LG_A_CONTEXT, insert LXB onto the LDB active queue so it
**	    can be removed from there by LG_end_tran().
**	    Add LG_A_JFIB, LG_A_JFIB_DB.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add LG_A_RBBLOCKS, LG_A_RFBLOCKS for
**	    buffered log reads.
**	06-Jul-2010 (jonj) SIR 122696
**	    Back off LG_BKEND_SIZE when computing minimum number
**	    of buffers (LG_A_BCNT).
*/
STATUS
LGalter(
i4                 flag,
PTR		    item,
i4		    l_item,
CL_ERR_DESC	    *sys_err)
{
    STATUS	status;

    /* Currently, only LG_A_BMINI might sleep... */
    while ( (status = LG_alter(flag, item, l_item, sys_err))
		== LG_MUST_SLEEP )
    {
	    CSsuspend(CS_LOG_MASK, 0, 0);
    }

    return(status);
}



static STATUS
LG_alter(
i4                 flag,
PTR		    item,
i4		    l_item,
CL_ERR_DESC	    *sys_err)
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LDB	*ldb;
    register LDB	*this_ldb;
    register LDB	*next_ldb;
    register LDB	*prev_ldb;
    register LFB	*lfb = 0;
    register LPB	*lpb;
    register LPB	*next_lpb;
    register LPB	*prev_lpb;
    register LPD	*lpd;
    register LPD	*next_lpd;
    register LPD	*prev_lpd;
    register LXB	*lxb;
    register LXB	*next_lxb;
    register LXB	*prev_lxb;
    register LFD	*lfd;
    LGK_EXT		*ext;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		lxb_offset;
    SIZE_TYPE		lpb_offset;
    SIZE_TYPE		ldb_offset;
    SIZE_TYPE		*lfbb_table;
    SIZE_TYPE		*ldbb_table;
    SIZE_TYPE		*lpbb_table;
    SIZE_TYPE		*lxbb_table;
    SIZE_TYPE		*lpdb_table;
    i4			value;
    u_i8		reserved_space;
    i4			used_blocks;
    LG_I4ID_TO_ID	lx_id;
    LG_I4ID_TO_ID	db_id;
    LG_I4ID_TO_ID	lg_id;
    LG_I4ID_TO_ID	xid1, xid2;
    i4			err_code;
    LG_HDR_ALTER_SHOW	*hdr_struct;
    LG_RECOVER		*recov_struct;
    STATUS		status;
    LBB			*current_lbb;
    LXB			*slxb;
    LXBQ		*lxbq, *next_lxbq, *prev_lxbq;
    LG_CONTEXT	    	*ctx;
    LG_LSN		*lsn;
    i4			tmp_status;
    i4			i;
    LG_JFIB		*jfib, *cnf_jfib;
    LG_DATABASE		*db;

    LG_WHERE("LGalter")

    CL_CLEAR_ERR(sys_err);

    switch (flag)
    {
    case LG_A_BCNT:
	/*
	**  Only allow the buffer count to be changed if it has never been
	**  set before and the buffer size is a legal value.
	*/

	if (l_item != sizeof(i4) 	|| 
	    lgd->lgd_local_lfb.lfb_buf_cnt 		||
	    item == 0 ||
	    (lgd->lgd_local_lfb.lfb_header.lgh_size != 2048 &&
	     lgd->lgd_local_lfb.lfb_header.lgh_size != 4096 &&
	     lgd->lgd_local_lfb.lfb_header.lgh_size != 8192 &&
	     lgd->lgd_local_lfb.lfb_header.lgh_size != 16384 &&
	     lgd->lgd_local_lfb.lfb_header.lgh_size != 32768 &&
	     lgd->lgd_local_lfb.lfb_header.lgh_size != 65536))
	{
	    uleFormat(NULL, E_DMA400_LG_BAD_BCNT_ALTER, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL,
			&err_code, 4,
			0, l_item, 0, lgd->lgd_local_lfb.lfb_buf_cnt, 0, item,
			0, lgd->lgd_local_lfb.lfb_header.lgh_size);
	    return (LG_BADPARAM);
	}
	/* there must be enough buffers allocated to write a reasonably
	** sized log record in a worst-case split scenario; This worst case is
	** reckoned to be 3 buffers plus enough buffers to write an LG_MAX_RSZ
	** sized log record.
	*/
	if (*(i4 *)item <
		(LG_MAX_RSZ /
		    (lgd->lgd_local_lfb.lfb_header.lgh_size-LG_BKEND_SIZE
		    	- sizeof(LBH))) + 3)
	{
	    uleFormat(NULL, E_DMA400_LG_BAD_BCNT_ALTER, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL,
			&err_code, 4,
			0, l_item, 0, lgd->lgd_local_lfb.lfb_buf_cnt,
			0, *(i4 *)item,
			0, lgd->lgd_local_lfb.lfb_header.lgh_size);
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_local_lfb.lfb_mutex))
	   return(status);
	status = (LG_allocate_buffers( lgd, &lgd->lgd_local_lfb,
				    lgd->lgd_local_lfb.lfb_header.lgh_size,
					*(u_i4 *)item));
	LG_unmutex(&lgd->lgd_local_lfb.lfb_mutex);
	return(status);

	break;

    case LG_A_BLKS:
	if (lgd->lgd_lpb_inuse || lgd->lgd_lxb_inuse || lgd->lgd_lpd_inuse)
	{
	    uleFormat(NULL, E_DMA46C_LGALTER_SBKS_INUSE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lgd->lgd_lpb_inuse, 0, lgd->lgd_lxb_inuse,
			    0, lgd->lgd_lpd_inuse);
	    return (LG_MEM_INUSE);
	}

	if (lgd->lgd_lpbb_table || lgd->lgd_lxbb_table || lgd->lgd_lpdb_table)
	{
	    uleFormat(NULL, E_DMA46D_LGALTER_SBKTAB_INUSE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			    0, lgd->lgd_lpbb_table);
	    return (LG_MEM_INUSE);
	}

	value = *(i4 *)item;

	/*
	** Allocate contiguous space for lpbb_table, lxbb_table,
	** lpdb_table, and lfdb_table, each getting "value" number of slots.
	**
	** Add to this the size of the transaction hash buckets,
	** one bucket (LXBQ) per "value".
	*/
	ext = (LGK_EXT *) lgkm_allocate_ext((sizeof(SIZE_TYPE) * (4 * value))
					+ (sizeof(LXBQ) * value)
					+ (sizeof(u_i4) * value));
	if (ext == 0)
	{
	    uleFormat(NULL, E_DMA46E_LGALTER_SBKTAB_NOMEM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			0, sizeof(SIZE_TYPE) * value);
	    return (LG_EXCEED_LIMIT);
	}

	lgd->lgd_lpbb_table = LGK_OFFSET_FROM_PTR(ext);
	lgd->lgd_lpbb_count = 0;
	lgd->lgd_lpbb_size = value -1;
	lgd->lgd_lpbb_free = 0;

	lgd->lgd_lxbb_table = lgd->lgd_lpbb_table + (1 * (sizeof(SIZE_TYPE) * value));
	lgd->lgd_lxbb_count = 0;
	lgd->lgd_lxbb_size = value -1;
	lgd->lgd_lxbb_free = 0;

	lgd->lgd_lpdb_table = lgd->lgd_lpbb_table + (2 * (sizeof(SIZE_TYPE) * value));
	lgd->lgd_lpdb_count = 0;
	lgd->lgd_lpdb_size = value -1;
	lgd->lgd_lpdb_free = 0;

	lgd->lgd_lfdb_table = lgd->lgd_lpbb_table + (3 * (sizeof(SIZE_TYPE) * value));
	lgd->lgd_lfdb_count = 0;
	lgd->lgd_lfdb_size = value -1;
	lgd->lgd_lfdb_free = 0;

	lgd->lgd_lxh_buckets = lgd->lgd_lpbb_table + (4 * (sizeof(SIZE_TYPE) * value));


	/* Initialize the transaction hash buckets as empty lists */
	{
	    i4		i;
	    LXBQ	*bucket = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxh_buckets);

	    for (i = 0; i < lgd->lgd_lxbb_size; i++)
	    {
		bucket[i].lxbq_next = bucket[i].lxbq_prev =
			LGK_OFFSET_FROM_PTR(&bucket[i].lxbq_next);
	    }
	}

	/* Set offset to active tranid array, one u_i4 for each possible LXB */
	lgd->lgd_xid_array = lgd->lgd_lxh_buckets + (sizeof(LXBQ) * lgd->lgd_lxbb_size);
	lgd->lgd_xid_array_size = lgd->lgd_lxbb_size * sizeof(u_i4);
	MEfill(lgd->lgd_xid_array_size, 0, LGK_PTR_FROM_OFFSET(lgd->lgd_xid_array));
	TRdisplay("%@ lgalter:%d lgd_xid_array %p, size %d\n",
		__LINE__, 
		(PTR)LGK_PTR_FROM_OFFSET(lgd->lgd_xid_array),
		lgd->lgd_xid_array_size);


	break;

    case LG_A_LDBS:
	if (lgd->lgd_ldb_inuse)
	{
	    uleFormat(NULL, E_DMA46F_LGALTER_LBKS_INUSE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			0, lgd->lgd_ldb_inuse);
	    return (LG_MEM_INUSE);
	}

	if (lgd->lgd_lfbb_table || lgd->lgd_ldbb_table)
	{
	    uleFormat(NULL, E_DMA470_LGALTER_LBKTAB_INUSE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			0, lgd->lgd_lfbb_table);
	    return (LG_MEM_INUSE);
	}

	value = *(i4 *)item;

	/*
	** Allocate contiguous space for lfbb_table and ldbb_table,
	** each getting "value" number of slots.
	*/
	ext = (LGK_EXT *) lgkm_allocate_ext(sizeof(SIZE_TYPE) * (2 * value ));
	if (ext == 0)
	{
	    uleFormat(NULL, E_DMA471_LGALTER_LBKTAB_NOMEM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			0, sizeof(i4) * value);
	    return (LG_EXCEED_LIMIT);
	}

	lgd->lgd_lfbb_table = LGK_OFFSET_FROM_PTR(ext);
	lgd->lgd_lfbb_count = 1;
	lgd->lgd_lfbb_size = value -1;
	lgd->lgd_lfbb_free = 0;

	/*  Initialize the local LFB embedded in the LGD. */

	lfbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lfbb_table);
	lfbb_table[1] = LGK_OFFSET_FROM_PTR(&lgd->lgd_local_lfb);
	lgd->lgd_local_lfb.lfb_id.id_id = 1;
	lgd->lgd_local_lfb.lfb_id.id_instance = -1;
	lgd->lgd_local_lfb.lfb_type = LFB_TYPE;

	lgd->lgd_ldbb_table = lgd->lgd_lfbb_table + (sizeof(SIZE_TYPE) * value);
	lgd->lgd_ldbb_count = 1;
	lgd->lgd_ldbb_size = value -1;
	lgd->lgd_ldbb_free = 0;

	/*  Initialize the special non-database LDB for recovery/archiver. */

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldbb_table[1] = LGK_OFFSET_FROM_PTR(&lgd->lgd_spec_ldb);
	lgd->lgd_spec_ldb.ldb_id.id_id = 1;
	lgd->lgd_spec_ldb.ldb_id.id_instance = -1;
	lgd->lgd_spec_ldb.ldb_type = 0;
	break;

    case LG_A_NODELOG_HEADER:

	/*
	** Set log header information.
	** Also set the address that the log is guaranteed forced to to be
	** the new EOF.  LG_A_HEADER should only be called when the
	** guaranteed EOF is known.
	*/
	if (l_item != sizeof(LG_HDR_ALTER_SHOW))
	{
	    uleFormat(NULL, E_DMA401_LG_BAD_HDRSIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL,
			&err_code, 2,
			0, l_item, 0, sizeof(LG_HDR_ALTER_SHOW));
	    return(LG_BADPARAM);
	}
	hdr_struct = (LG_HDR_ALTER_SHOW *)item;
	lg_id.id_i4id = hdr_struct->lg_hdr_lg_id;

	/*	Check the lg_id. */

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_HEADER");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_HEADER");
	    return (LG_BADPARAM);
	}
	lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

	alter_header(lfb, &hdr_struct->lg_hdr_lg_header);

        break;

    case LG_A_HEADER:
	alter_header(&lgd->lgd_local_lfb, (LG_HEADER *)item);
	break;

    case LG_A_BOF:
	/* NOT USED ANYMORE */
	uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
	    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
	    0, l_item, 0, sizeof(LG_LA), 0, "LG_A_BOF");
	return (LG_BADPARAM);

    case LG_A_ARCHIVE:
	/*
	** Signal an archive event.
	*/
	LG_signal_event(LGD_ARCHIVE, 0, FALSE);
	break;

    case LG_A_ARCHIVE_DONE:
	/*
	** Do archive cycle post processing. This alter call expects an ad-hoc
	** argument list containing:
	**	offset  0: a log address
	**	offset  8: the other log address.
	**
	** This alter code operates ONLY on the local log -- it does not allow
	** for specification of any other log in a multi-node logging system.
	** This is OK; this call is used only by the local archiver, NOT by the
	** DMFCSP cross-node archiving.
	*/
	if (l_item != (2 * sizeof(LG_LA)))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_A_ARCHIVE_DONE");
	    return (LG_BADPARAM);
	}

	LG_archive_complete(&lgd->lgd_local_lfb, (LG_LA *)item, 
		(LG_LA *)((char*)item + sizeof(LG_LA)));

	break;

    case LG_A_EOF:

	/*  Set end of log file address. */

	if (l_item != (sizeof(LG_LA) + sizeof(i4)))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA) + sizeof(i4),
			0, "LG_A_EOF");
	    return (LG_BADPARAM);
	}
	lg_id.id_i4id = *(LG_LGID *)item;

	/*	Check the lg_id. */

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_EOF");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_EOF");
	    return (LG_BADPARAM);
	}
	lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);
        
        /* Make certain we get the Current LBB. */

	while (current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
        {
	   if (status = LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
	       return(status);

           if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
           {
	      /* current_lbb is still the "CURRENT" lbb. */
              break;
           }

           (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
           TRdisplay( "LG_alter(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                      lfb->lfb_header.lgh_end.la_sequence,
                      lfb->lfb_header.lgh_end.la_block,
                      lfb->lfb_header.lgh_end.la_offset);
#endif
        }
	lfb->lfb_header.lgh_end = *(LG_LA *)((char *)item + sizeof(i4));
	(VOID)LG_unmutex(&current_lbb->lbb_mutex);

	break;

    case LG_A_STAMP:
	return (LG_BADPARAM);
	break;

    case LG_A_CPAGENT:

	/*
	** LG_A_CPAGENT : Process participates in Consistency Points
	**
	** Inputs:
	**	item	- lg_id of the calling process
	**
	** This call is made by the Consistency Point thread in a process
	** which maintains a data cache and must participate in Consistency
	** Point protocols.  The calling process' status is set to LPB_CPAGENT.
	**
	** When a CP is signalled by the Ingres installation, the Logging
	** System will wait for all processes marked CPAGENT to respond.
	*/

	/*
	** Validate the parameters: item should point to a valid lg_id.
	** Find the process control block (lpd) of the calling process.
	*/
	if (l_item == 0)
	    return (LG_BADPARAM);

	lg_id.id_i4id = *(LG_LGID *)item;
	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	    return (LG_BADPARAM);

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	/*
	** Set CPAGENT status in the calling process.
	*/
	if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	    return(status);
	lpb->lpb_status |= LPB_CPAGENT;
	(VOID)LG_unmutex(&lpb->lpb_mutex);

	/*
	** Keep a count of CPAGENT LPBs in the LGD so that it won't
	** be necessary to run (and lock) the lpb queue just to count
	** them.
	*/
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	lgd->lgd_n_cpagents++;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_A_CPFDONE:

	/*
	** LG_A_CPFDONE : Part of Consistency Point protocols.
	**
	** Inputs:
	**	item	- lg_id of the calling process
	**
	** When a Consistency Point is taken by the installation, the following
	** steps are performed:
	**
	**     The Begin Consistency Point (BCP) records are written.
	**     The buffer caches of each active server are flushed.
	**     The End Consistency Point (ECP) record is written.
	**
	** Since the cache flushing requires actions by many processes, a
	** multi-process handshaking protocol is used to make sure all servers
	** respond once and only once.
	**
	** The protocol follows these steps:
	**
	**   - Each server listens for the CPFLUSH event.
	**   - Upon receiving the CPFLUSH signal, each server flushes the
	**     dirty pages from its data cache, signals CPFDONE, and waits
	**     for the CPWAKEUP event.
	**   - The Logging System counts occurrences of the CPFDONE signal.
	**     When the last server responds with CPFDONE, then the Logging
	**     System raises the CPWAKEUP event.
	**   - Upon receiving the CPWAKEUP event, each server sends the
	**     CPWAITDONE signal and rearms its CPFLUSH wait.
	**   - The Logging System counts occurrences of the CPWAITDONE signal.
	**     When the last server responds with CPWAITDONE, then the
	**     Consistency Point operation can continue with the writing of
	**     the ECP record.
	**
	** This CPFDONE event is signaled by each server after having flushed
	** their data cache.  The servers condition state should be set to
	** CPWAIT until all servers complete the cache flush.
	**
	** If this is the last server completing its cache flush, then the
	** CPWAKEUP event should be raised.
	*/

	/*
	** Validate the parameters: item should point to a valid lg_id.
	** Find the process control block (lpd) of the calling process.
	*/
	if (l_item == 0)
	{
	    uleFormat(NULL, E_DMA473_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, "LG_A_CPFDONE");
	    return (LG_BADPARAM);
	}

	lg_id.id_i4id = *(LG_LGID *)item;

	/*	Check the lg_id. */

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_CPFDONE");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_CPFDONE");
	    return (LG_BADPARAM);
	}

	/* Mark the visit bit */
	if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	    return(status);
	lpb->lpb_cond = LPB_CPWAIT;
	(VOID)LG_unmutex(&lpb->lpb_mutex);

	/*
	** If all servers have responded with CPFDONE, then signal them
	** to wakeup and wait for the next cpflush event.
	*/
	if (LG_check_cp_servers(LPB_CPWAIT))
	{
	    LG_signal_event(LGD_CPWAKEUP, LGD_CPFLUSH, FALSE);
	}

	break;

    case LG_A_CPWAITDONE:

	/*
	** LG_A_CPWAITDONE : Part of Consistency Point protocols.
	**
	** Inputs:
	**	item	- lg_id of the calling process
	**
	** See description of above CPFDONE for further context.
	**
	** This CPWAITDONE event is signaled by each server after having 
	** received the CPWAKEUP event.  The servers condition state should 
	** be set to CPREADY to indicate that they are ready to receive a
	** new CPFLUSH command.
	**
	** If this is the last server responding to the CPWAKEUP event, then
	** the ECP log record should be written.  (Note: New servers have
	** their condition states initialized to CPREADY).
	*/

	/*
	** Validate the parameters: item should point to a valid lg_id.
	** Find the process control block (lpd) of the calling process.
	*/
	if (l_item == 0)
	{
	    uleFormat(NULL, E_DMA473_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, "LG_A_CPWAITDONE");
	    return (LG_BADPARAM);
	}

	lg_id.id_i4id = *(LG_LGID *)item;
	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_CPWAITDONE");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_CPWAITDONE");
	    return (LG_BADPARAM);
	}

	/*
	** If all servers now in CPREADY state then we are ready for the
	** ECP to be written.  
	*/
	if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	    return(status);
	lpb->lpb_cond = LPB_CPREADY;
	(VOID)LG_unmutex(&lpb->lpb_mutex);

	if (LG_check_cp_servers(LPB_CPREADY))
	{
	    LG_signal_event(LGD_ECP, LGD_CPWAKEUP, FALSE);
	}
	break;


    case LG_A_FORCE_ABORT:

	/*
	** Set the force abort handler of the server.
	**
	** Item ptr should point to pair of pointers.
	** First is pointer to lg process id, 
	** second is force abort handler.
	*/

	if (l_item < 2 * sizeof(PTR))
	{
	    uleFormat(NULL, E_DMA473_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, "LG_A_FORCE_ABORT");
	    return (LG_BADPARAM);
	}

	lg_id.id_i4id = **(LG_LGID **)item;

	/*	Check the lg_id. */

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_FORCE_ABORT");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_FORCE_ABORT");
	    return (LG_BADPARAM);
	}
	item = (PTR)((char *)item + sizeof(PTR));
	if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	    return(status);
	lpb->lpb_force_abort = *(VOID (**)()) item;
	(VOID)LG_unmutex(&lpb->lpb_mutex);
	break;

    case LG_A_SBACKUP:

        if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_SBACKUP");
	    return (LG_BADPARAM);
	}

        db_id.id_i4id = *(LG_DBID *)item;

        /*  Check the db_id. */
        if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, "LG_A_SBACKUP");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

        if (lpd->lpd_type != LPD_TYPE ||
            lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
        {
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_SBACKUP");
            return (LG_BADPARAM);
        }

        ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

        /* Need to hold the lgd->lgd_ldb_q_mutex when setting the LDB_BACKUP
        ** flag.
        */

	/*
	** When both the lgd_ldb_q and ldb must be mutexed, always take
	** the lgd_ldb_q_mutex, then ldb_mutex.
	*/
        
        if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
           return status;

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);
        
        if (ldb->ldb_status & LDB_BACKUP)
	{
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    (VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status, 0, "LG_A_SBACKUP");
            return (LG_BADPARAM);
	}

        ldb->ldb_status |= (LDB_BACKUP | LDB_STALL | LDB_CKPLK_STALL);


        (VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);


        /*
	** CKPDB is starting.
	**
	** For each active transaction in this database, request a SHR lock
	** on LK_CKP_TXN on behalf of that transaction.
	**
	**    Assumptions/precautions:
	**	o The txn may be waiting on a lock, so we must use the
	**	  LK_MULTITHREAD flag to allow this lock request to
	**	  proceed on a waiting lock list.
	**	o The lock must be immediately grantable, there should be
	**	  no conflicts, but we'll use the LK_NOWAIT flag just in
	**	  case; if a wait would be required, some sort of protocol
	**	  error has happened and we'll abort the checkpoint.
	**	o Because a txn's lock list is released after it disconnects
	**	  from LG, we should be guaranteed that the lock list exists.
	**	o LG assigns transaction ids, and locking uses that transaction
	**	  id as "lbb_name" - that's the handle we'll use to find the
	**	  lock list.
	**
	** When we return from here to CKPDB, it will request an EXCL lock
	** on LK_CKP_TXN for this database and will stall by waiting for 
	** the active transactions to complete and release their SHR locks.
	**
	** If there are no current active transactions in the CKPDB database,
	** CKPDB will be granted its EXCL lock and will run immediately.
	**
	** Any log transactions which become active while the database is stalled
	** for CKPDB will acquire a SHR lock on LK_CKP_TXN, hence wait 
	** until CKPDB unstalls them by releasing its EXCL lock.
	**
	** ldb_lxbo_count accurately reflects the number of currently
	** active, protected transactions in this database.
        **
        ** Ignore non-protect transactions, as the checkpoint process
        ** itself will start a non-protect transaction just by opening
        ** the database.  Non-protect transactions cannot be used to
        ** manage database updates, so they will not effect the backup.
        */
	
	/* The LXB queue mutex protects ldb_lxbo_count */
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lxb_q_mutex))
	    return(status);

        /*
        ** If there are no protected update transactions in this database,
        ** then go ahead and signal the backup to start.  If there are
        ** protected update transactions, then wait until all of them complete to
        ** start the backup.
	**
	** When we signal the backup to start, we record the current EOF in
	** the ldb_sbackup field of the LDB. This will be used by higher-level
	** DMF protocols to decide whether a page has been changed since the
	** backup began. DMF, however, needs an LSN, not an LG_LA, to make this
	** check, since it wishes to compare a log sequence number on a page
	** against the LSN of the start of the backup. Therefore, we also set
	** ldb->ldb_sback_lsn to be the most recent LSN that we have assigned.
        */
        if (ldb->ldb_lxbo_count == 0)
        {
	    /* Must hold current lbb mutex for lgh_end integrity */
	    while(current_lbb = (LBB *)LGK_PTR_FROM_OFFSET(
				lgd->lgd_local_lfb.lfb_current_lbb))
            {
	       if (status = LG_mutex(SEM_EXCL, &current_lbb->lbb_mutex))
		   return(status);

               if ( current_lbb == (LBB *)LGK_PTR_FROM_OFFSET(lgd->lgd_local_lfb.lfb_current_lbb) )
               {
		  /* current_lbb is still the "CURRENT" lbb. */
                  break;
               }

               /* The lbb is the CURRENT LBB, so release hte mutex and repeat. To get
               ** the Log EOF, we must hold the lbb_mutex of the current lbb.
               */
               (VOID)LG_unmutex(&current_lbb->lbb_mutex);

#ifdef xDEBUG
               TRdisplay( "LG_alter(2):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                          lgd->lgd_local_lfb.lfb_header.lgh_end.la_sequence,
                          lgd->lgd_local_lfb.lfb_header.lgh_end.la_block,
                          lgd->lgd_local_lfb.lfb_header.lgh_end.la_offset);
#endif
            }
 
            STRUCT_ASSIGN_MACRO(lgd->lgd_local_lfb.lfb_header.lgh_end,
				ldb->ldb_sbackup);

	    ldb->ldb_sback_lsn = lgd->lgd_local_lfb.lfb_header.lgh_last_lsn;
	    (VOID)LG_unmutex(&current_lbb->lbb_mutex);

	    /* now, unconditionally signal LGD_CKP_SBACKUP.  We used to
	    ** signal CPNEEDED is some cases and wait until that was finished.
	    ** This is no longer necessary since ckpdb itself is going to
	    ** handle forcing a consistency point.
	    */
	    (VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    LG_signal_event(LGD_CKP_SBACKUP, 0, FALSE);
        }
	else
	{
	    LK_LOCK_KEY		lock_key;
	    LK_LKID		lock_id;
	    LK_LLID		lock_list_id;
	    LXB			*lxb;
	    LPD			*my_lpd;
	    SIZE_TYPE		ldb_offset = LGK_OFFSET_FROM_PTR(ldb);
	    SIZE_TYPE		lxb_offset;

	    /*
	    ** Prepare the lock key as:
	    **
	    **	type:   LK_CKP_TXN
	    **  key1-5: 1st 20 bytes of database name
	    **  key6: dbid
	    **
	    **	(see also conforming code in dmfcpp, dm0l, lkshow, dmfcsp)
	    **  Note source is ldb_buffer which contains dbname, dbowner, dbid.
	    */
	    lock_key.lk_type	= LK_CKP_TXN;
	    MEfill(sizeof(LK_LKID), 0, &lock_id);
	    MEcopy((char *)&ldb->ldb_buffer[0], 20, (char *)&lock_key.lk_key1);
	    MEcopy((char *)&ldb->ldb_buffer[64], 4, (char *)&lock_key.lk_key6);

	    TRdisplay("%@ LGALTER: CKP_TXN key %~t %d \n",
		20, (char*)(&lock_key.lk_key1), lock_key.lk_key6);

	    /* Only active+protect txns are on the LXB queue */
	    for (lxb_offset = lgd->lgd_lxb_next;
		 lxb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);
		 lxb_offset = lxb->lxb_next)
	    {
		lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);
		my_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);

		if (my_lpd->lpd_ldb == ldb_offset)
		{
		    /* Call LK_show to find this txn's locklist. */
		    status = LKshow(LK_S_LIST_NAME, 0, 
				(LK_LKID*)&lxb->lxb_lxh.lxh_tran_id,
				(LK_LOCK_KEY *)NULL, sizeof(lock_list_id),
				(PTR)&lock_list_id, 0, 0, sys_err);

		    /* Place CKP_TXN lock on transaction's lock list */
		    if (status == OK)
			status = LKrequest(LK_NOWAIT | LK_MULTITHREAD | LK_LOCAL,
				    lock_list_id,
				    &lock_key, LK_S, (LK_VALUE*)NULL, &lock_id,
				    0, sys_err);

		    if (status)
		    {
			/* Failure, unset the stall condition */
			uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, sys_err,
				    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
				    0, lock_list_id, 
				    0, lxb->lxb_lxh.lxh_tran_id.db_high_tran,
				    0, lxb->lxb_lxh.lxh_tran_id.db_low_tran,
				    0, "LG_A_SBACKUP");
			ldb->ldb_status &= ~(LDB_BACKUP | LDB_STALL |
					LDB_CKPLK_STALL);
			status = LG_BADPARAM;
			break;
		    }
		}
	    }

	    (VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	}
	return(status);

    case LG_A_CKPERROR:
	/*
	** Signal (from the ACP) that there has been an error in an online ckp.
	**
	** LGalter(LG_A_CKPERROR, item, l_item)
	**
	**     inputs: item	- database id.
	**     inputs: l_item	- length of a database id.
	**
	**     Tells the online checkpoint process that the ACP has encountered
        **     an error while performing some part of the checkpoint process
        **     (the most common problem being running out of disk space on the
	**     "dump" disk.
	*/

	db_id.id_i4id = *(LG_DBID *)item;

	/*  Check the db_id. */

	if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_ldbb_count,
			0, "LG_A_CKPERROR");
	    return (LG_BADPARAM);
	}

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);

	if (ldb->ldb_type != LDB_TYPE ||
	    ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_CKPERROR");
            return (LG_BADPARAM);
        }

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

	/*
	** Make sure the database is in backup state.
	*/
	if ((ldb->ldb_status & LDB_BACKUP) == 0)
	{
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status, 0, "LG_A_CKPERROR");
	    return(LG_BADPARAM);
	}

	ldb->ldb_status |= LDB_CKPERROR;

	(VOID)LG_unmutex(&ldb->ldb_mutex);

	/*
	** Resume any waiters on the online backup state.
	**
	** Calling LG_signal_event() with a NULL event just
	** induces a scan of the event wait queue - just what we want.
	*/
	LG_signal_event(0, 0, FALSE);

	/*
	** Check if the system backup state can be reset.
	*/
	LG_cleanup_checkpoint((LPB *)NULL);
	break;

    case LG_A_FBACKUP:

        if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_FBACKUP");
	    return (LG_BADPARAM);
	}

        db_id.id_i4id = *(LG_DBID *)item;

        /*  Check the db_id. */

        if (db_id.id_lgid.id_id == 0 || (i4) db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_ldbb_count,
			0, "LG_A_FBACKUP");
	    return (LG_BADPARAM);
	}

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);

        if (ldb->ldb_type != LDB_TYPE ||
            ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
        {
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_FBACKUP");
            return (LG_BADPARAM);
        }

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

	/*
	** Make sure the database is in backup state.
	*/
	if ((ldb->ldb_status & LDB_BACKUP) == 0)
	{
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status, 0, "LG_A_FBACKUP");
            return (LG_BADPARAM);
	}

	/* 
	** Turn off database FBACKUP status.
	*/
	ldb->ldb_status |= LDB_FBACKUP;

	(VOID)LG_unmutex(&ldb->ldb_mutex);

        break;

    case LG_A_DBACKUP:

        if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_DBACKUP");
	    return (LG_BADPARAM);
	}

        db_id.id_i4id = *(LG_DBID *)item;

        /*  Check the db_id. */
        if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, "LG_A_DBACKUP");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

        if (lpd->lpd_type != LPD_TYPE ||
            lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
        {
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_DBACKUP");
            return (LG_BADPARAM);
        }

        ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

	ldb->ldb_eback_lsn = lgd->lgd_local_lfb.lfb_header.lgh_last_lsn;

        if ((ldb->ldb_status & LDB_BACKUP) &&
            (ldb->ldb_status & LDB_STALL) == 0 &&
            (ldb->ldb_status & LDB_CKPDB_PEND) == 0 &&
            (ldb->ldb_status & LDB_FBACKUP))
        {
	    /* 
	    ** Turn off process BACKUP status.
	    */
	    ldb->ldb_status &= ~(LDB_BACKUP | LDB_FBACKUP | LDB_CKPERROR);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);

	    /*
	    ** Check if the system backup state can be reset.
	    */
	    LG_cleanup_checkpoint((LPB *)NULL);
        }
        else
	{
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status, 0, "LG_A_DBACKUP");
            return (LG_BADPARAM);
	}

        break;

    case LG_A_SJSWITCH:

        if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_SJSWITCH");
	    return (LG_BADPARAM);
	}

        db_id.id_i4id = *(LG_DBID *)item;

        /*  Check the db_id. */
        if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, "LG_A_SJSWITCH");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

        if (lpd->lpd_type != LPD_TYPE ||
            lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
        {
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_SJSWITCH");
            return (LG_BADPARAM);
        }

	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	/* Signal a journal switch */
	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);
	
	tmp_status = ldb->ldb_status;

	ldb->ldb_status |= LDB_JSWITCH;
	(VOID)LG_unmutex(&ldb->ldb_mutex);
	    
	LG_signal_event(LGD_ARCHIVE, LGD_JSWITCHDONE, FALSE);

	if (tmp_status & LDB_JSWITCH)
	{
	    /* LDB_JSWITCH already set is unexpected, but not fatal */
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status,
			0, "LG_A_SJSWITCH");
	}

	break;

    case LG_A_RESUME:

        if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_RESUME");
	    return (LG_BADPARAM);
	}

        db_id.id_i4id = *(LG_DBID *)item;

        /*  Check the db_id. */

        if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, "LG_A_RESUME");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

        if (lpd->lpd_type != LPD_TYPE ||
            lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
        {
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_RESUME");
            return (LG_BADPARAM);
        }

        ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

        if ((ldb->ldb_status & LDB_BACKUP) == 0 ||
            (ldb->ldb_status & LDB_STALL) == 0)
	{
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status, 0, "LG_A_RESUME");
            return (LG_BADPARAM);
	}

        if (ldb->ldb_status & LDB_STALL)
        {
            ldb->ldb_status &= ~(LDB_STALL | LDB_CKPDB_PEND);
            LG_resume(&lgd->lgd_ckpdb_event, NULL);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
        }
        else    
	{
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    uleFormat(NULL, E_DMA476_LGALTER_DBSTATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_id.id_id, 0, ldb->ldb_id.id_instance,
			0, ldb->ldb_status, 0, "LG_A_RESUME");
            return (LG_BADPARAM);
	}

        break;


    case LG_A_DBRECOVER:

	if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_DBRECOVER");
	    return (LG_BADPARAM);
	}

	db_id.id_i4id = *(LG_DBID *)item;

	/*  Check the db_id. */

	if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_ldbb_count,
			0, "LG_A_DBRECOVER");
	    return (LG_BADPARAM);
	}

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	/* save offset so it can be checked later in lpd loop */
	ldb_offset = ldbb_table[db_id.id_lgid.id_id];
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);

	if (ldb->ldb_type != LDB_TYPE ||
	    ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_DBRECOVER");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
	    return(status);

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	    return(status);

	end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);

	/* Find all runaway processes (LPBs) which reference (LPD) this LDB */
	for (lpb_offset = lgd->lgd_lpb_next;
	    lpb_offset != end_offset;)
	{
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
	    lpb_offset = lpb->lpb_next;

	    if ((lpb->lpb_status & LPB_RUNAWAY) == 0)
		continue;

	    /*
	    ** Release the lpb_q_mutex before we lock the lpb
	    ** to avoid deadlocking with other code which
	    ** is attacking the queue inside-out (LPB->lpb_q).
	    */
	    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

	    if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
		return(status);

	    /*
	    ** Check status again after semaphore wait.
	    ** If it's changed, restart the lpb_q search.
	    */
	    if ((lpb->lpb_type != LPB_TYPE ||
	       (lpb->lpb_status & LPB_RUNAWAY) == 0))
	    {
		(VOID)LG_unmutex(&lpb->lpb_mutex);
		if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
		    return(status);
		lpb_offset = lgd->lgd_lpb_next;
		continue;
	    }

	    /* Is the DB open in this process? */
	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpb->lpb_lpd_next);
	    while (lpd != (LPD *)&lpb->lpb_lpd_next)
	    {
		if (lpd->lpd_type != LPD_TYPE ||
		    lpd->lpd_ldb != ldb_offset)
		{
		    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_next);
		    continue;
		}

		/* 
		** A process (LPB) will have but a single reference (LPD)
		** to the database being recovered, so once we've done
		** what we need to here, we can break out of the LPB->LPD
		** loop.
		*/

		/*
		**  Turn off the LDB_RECOVER bit, so that if signal ClOSEDB
		**  hasn't been called (LDB_CLOSEDB_DONE hence not set), this
		**  case might call signal CLOSEDB, eventually, rcp will do
		**  an LGalter on LG_A_CLOSEDB to deallocate the LDB structure.
		**  However, if the LG_A_CLOSEDB has occurred already (the
		**  LDB_CLOSEDB_DONE bit was hence set), then, this case will
		**  simply deallocate the LDB structure.
		*/

		ldb->ldb_status &= ~LDB_RECOVER;

		/* 
		** If there are still transactions in the database,
		** mark the LPB to be void
		** but don't delete the LDB, LPD, or LPB.
		*/
		if (lpd->lpd_lxb_count)
		{
		    lpb->lpb_status |= LPB_VOID;

		    /*
		    ** If this is a CPAGENT lpb, decrement the
		    ** count of cpagents - we don't want to include
		    ** VOID ones in the count.
		    */
		    if (lpb->lpb_status & LPB_CPAGENT)
		    {
			if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
			    return(status);
			lgd->lgd_n_cpagents--;
			(VOID)LG_unmutex(&lgd->lgd_mutex);
		    }
		    break;
		}

		/*	Remove LPD from LPB queue. */
		next_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_next);
		prev_lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpd->lpd_prev);
		next_lpd->lpd_prev = lpd->lpd_prev;
		prev_lpd->lpd_next = lpd->lpd_next;

		LG_deallocate_cb(LPD_TYPE, (PTR)lpd);

		/* One less database opened in this process */
		lpb->lpb_lpd_count--;

		lgd->lgd_stat.remove++;

		/*
		** the long-awaited waiter for open event  can be resumed now.
		*/

		if (ldb->ldb_status & LDB_OPENDB_PEND)
		{
		    ldb->ldb_status &= ~(LDB_OPENDB_PEND | LDB_OPN_WAIT);
		    LG_resume(&lgd->lgd_open_event, NULL);
		}    

		/* One less process that has this database open */
		if (--ldb->ldb_lpd_count == 0) 
		{
		    if ((ldb->ldb_status & LDB_NOTDB) == 0)
		    {
			if (ldb->ldb_status & LDB_JOURNAL &&
			    ldb->ldb_j_first_la.la_block &&
			    ldb->ldb_j_first_la.la_offset)
			{
			    ldb->ldb_status |= LDB_PURGE;
			    LG_signal_check();
			}
			else if (ldb->ldb_status & LDB_CLOSEDB_DONE)
			{
			    /*  Remove LDB from active queue. */
			    ldb->ldb_type = 0;

			    next_ldb=(LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_next);
			    prev_ldb=(LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_prev);
			    next_ldb->ldb_prev = ldb->ldb_prev;
			    prev_ldb->ldb_next = ldb->ldb_next;

			    /*
			    ** This releases the ldb_mutex and decrements
			    ** lgd_ldb_inuse.
			    */
			    LG_deallocate_cb(LDB_TYPE, (PTR)ldb);

			    /*
			    ** Remember that the LDB has been vaporized.
			    */
			    ldb = (LDB *)NULL;

			    if (lgd->lgd_ldb_inuse == 1 &&
				(lgd->lgd_status & LGD_START_SHUTDOWN))
			    {
				LG_signal_event(LGD_ACP_SHUTDOWN, 0, FALSE);
			    }
			}
			else
			{
			    ldb->ldb_status |= LDB_CLOSEDB_PEND;
			    LG_signal_event(LGD_CLOSEDB, 0, FALSE);
			}
		    }
		}
		break;
	    } /* while (lpd != (LPD *)&lpb->lpb_lpd_next) */

	    /* Don't remove the LPB if it still has open databases */
	    if (lpb->lpb_lpd_count == 0)
	    {
		/*	Remove from active LPB queue. */
		if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
		    return(status);

		next_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb->lpb_next);
		prev_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb->lpb_prev);
		next_lpb->lpb_prev = lpb->lpb_prev;
		prev_lpb->lpb_next = lpb->lpb_next;

		(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

		/*
		** If this is a CPAGENT lpb, decrement the
		** count of cpagents.
		*/
		if ((lpb->lpb_status & (LPB_CPAGENT | LPB_VOID))
					== LPB_CPAGENT)
		{
		    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
			return(status);
		    lgd->lgd_n_cpagents--;
		    (VOID)LG_unmutex(&lgd->lgd_mutex);
		}

#if defined(conf_CLUSTER_BUILD)
		if ( CXcluster_enabled() && LG_my_pid != lpb->lpb_pid)
		{
		    STATUS		msg_status;

		    msg_status = LG_rcvry_cmpl(
					lpb->lpb_pid, lpb->lpb_id.id_id);
		}
#endif

		/*
		**  Deallocate the LPB, freeing the lpb_mutex
		**  and decrementing lgd_lpb_inuse.
		*/
		LG_deallocate_cb(LPB_TYPE, (PTR)lpb);
		lpb = (LPB *)NULL;

		if ((lgd->lgd_lpb_inuse == 1) &&
		    (lgd->lgd_status & LGD_START_SHUTDOWN))
		{
		    LG_signal_event(LGD_IMM_SHUTDOWN, 0, FALSE);
		}
		else
		{
		    /*
		    ** If Logging System is in the middle of a consistency 
		    ** point and is waiting for all the servers to respond
		    ** before continuing, the system may now be able to proceed.
		    */
		    LG_last_cp_server();
		}
	    }
	    else
	    {
	        if (lpb->lpb_status & LPB_VOID)
		    LG_last_cp_server();
		(VOID)LG_unmutex(&lpb->lpb_mutex);
	    }
	    /* Relock the LPB queue, then continue the loop */
	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
		return(status);
	}

	(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

	if (ldb)
	    (VOID)LG_unmutex(&ldb->ldb_mutex);

	(VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);

	break;

    case LG_A_DB:
    case LG_A_OPENDB:
    case LG_A_CLOSEDB:

	/* Change the status of a database. */

	if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID),
			0, "LG_A_DB/PURGEDB/OPENDB/CLOSEDB");
	    return (LG_BADPARAM);
	}

	db_id.id_i4id = *(LG_DBID *)item;

	/*  Check the db_id. */

	if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	{
#ifdef xDEBUG
	    LG_debug_wacky_ldb_found(lgd, (LDB *)NULL);
#endif
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_ldbb_count,
			0, "LG_A_DB/PURGEDB/OPENDB/CLOSEDB");
	    return (LG_BADPARAM);
	}

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);

	if (ldb->ldb_type != LDB_TYPE ||
	    ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
	{
#ifdef xDEBUG
	    LG_debug_wacky_ldb_found(lgd, (LDB *)ldb);
#endif
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			0, db_id.id_lgid.id_instance,
			0, "LG_A_DB/PURGEDB/OPENDB/CLOSEDB");
	    return (LG_BADPARAM);
	}
	if ( status = LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
	    return(status);

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);

	if (flag == LG_A_DB)
	{
	    ldb->ldb_status |= LDB_INVALID;
	    lgd->lgd_stat.inconsist_db++;
	    if (ldb->ldb_status & LDB_OPENDB_PEND)
	    {
		ldb->ldb_status &= ~(LDB_OPENDB_PEND | LDB_OPN_WAIT);
		LG_resume(&lgd->lgd_open_event, NULL);
	    }
	}
	else if (flag == LG_A_OPENDB)
	{
	    /*
	    ** If the recovery is in progress, then don't resume
	    ** the waiters. 
	    ** The waiters will be resumed when the recovery process
	    ** is done (rcp will do LG_A_DBRECOVER).
	    */

	    if ((ldb->ldb_status & LDB_RECOVER) == 0)
	    {
		ldb->ldb_status &= ~(LDB_OPENDB_PEND | LDB_OPN_WAIT);
		LG_resume(&lgd->lgd_open_event, NULL);
	    }
	    else
	    {
		ldb->ldb_status |= LDB_OPN_WAIT;
	    }
	}
	else if (flag == LG_A_CLOSEDB)
	{
	    ldb->ldb_status &= ~LDB_CLOSEDB_PEND;
	    /* Bug 90140. Reset the LDB_CLOSE_WAIT flag and resume the open event */
	    if (ldb->ldb_status & LDB_CLOSE_WAIT)
	    {
	       ldb->ldb_status &= ~ LDB_CLOSE_WAIT;
	       LG_resume(&lgd->lgd_open_event, NULL);
	    }
	    if (ldb->ldb_lpd_count == 0)
	    {
		/*
		**  If recovery is still in progress, delay the deallocation
		**  of LDB strcture until recovery is finished. By that time,
		**  rcp will call LG_A_DBRECOVER, and it will deallocate
		**  the LDB structure if the LDB_CLOSEDB_DONE bit is set.
		*/

		if (ldb->ldb_status & LDB_RECOVER)
		    ldb->ldb_status |= LDB_CLOSEDB_DONE;
		else 
		{
		    /*  Remove LDB from active queue. */
		    ldb->ldb_type = 0;

		    next_ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_next);
		    prev_ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb->ldb_prev);
		    next_ldb->ldb_prev = ldb->ldb_prev;
		    prev_ldb->ldb_next = ldb->ldb_next;

		    /*
		    ** Deallocate the LDB. This unmutexes the LDB
		    ** and decrements lgd_ldb_inuse.
		    */
		    LG_deallocate_cb(LDB_TYPE, (PTR)ldb);

		    /*
		    ** Note that LDB is history
		    */
		    ldb = (LDB*)NULL;

		    /*  Change various counters. */

		    if (lgd->lgd_ldb_inuse == 1 &&
			(lgd->lgd_status & LGD_START_SHUTDOWN))
		    {
			LG_signal_event(LGD_ACP_SHUTDOWN, 0, FALSE);
		    }
		}
	    }
	}

	if (ldb)
	    (VOID)LG_unmutex(&ldb->ldb_mutex);

	(VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);

	break;

    case LG_A_CPA:
	/*  Set the current consistency point address. */

	if (l_item != sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_A_CPA");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	lgd->lgd_local_lfb.lfb_header.lgh_cp = *(LG_LA *)item;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_A_ONLINE:
    {
	STATUS	status;

	/*  Alter the log system online status. */

	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_ONLINE");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	lgd->lgd_status &= ~LGD_ONLINE;

	if (*(i4 *)item)
	    lgd->lgd_status |= LGD_ONLINE;

	(VOID)LG_unmutex(&lgd->lgd_mutex);

#if 0
	if (!(lgd->lgd_status & LGD_ONLINE))
	{
	    /* in the case of the UNIX implementation, taking the logging system
	    ** offline means destroying the shared memory segment associated 
	    ** with it.  We will explicitly destroy it here and assume that 
	    ** only the rcp will call this code in shutdown mode.
	    **
	    ** If the destroy fails, the failure will be noted in the log file.
	    */
	    CS_des_installation();
	}

	if ((status = LGK_destroy()) != OK)
	{
	    TRdisplay("LGK_destroy() failed with status %x, called from LG_alter().\n", 
		      status);
	}
#endif
	break;
    }

    case LG_A_ACPOFFLINE:

	/*  Alter the logging system start acp status. */
	/*  Currently, only setting the LGD_START_ARCHIVER status. */

	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_ACPOFFLINE");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	if (*(i4 *)item)
	    LG_signal_event(LGD_START_ARCHIVER, 0, TRUE);
	else
	    lgd->lgd_status &= ~LGD_START_ARCHIVER;

	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_A_BCPDONE:

	/*
	** LG_A_BCPDONE : Consistency Point protocols.
	**
	** When a Consistency Point is taken by the installation, the following
	** steps are performed:
	**
	**	The Begin Consistency Point (BCP) records are written.
	**	The buffer caches of each active server are flushed.
	**	The End Consistency Point (ECP) record is written.
	**
	** The BCP records list the current activity of the installation
	** (what databases are open, what transactions are in progress) and
	** require us to briefly stall the installation while the information
	** is collected and logged.
	**
	** This alter point serves two purposes:
	**
	**	Indicate the start of the BCP write (item == FALSE)
	**	Indicate the completion of the BCP write (item == TRUE)
	**
	** When called with "item == FALSE", it marks the start of the writing
	** of the BCP record.  At this time we stall the system until the
	** LGalter call is made again with "item == TRUE".
	**
	** When called with "item == TRUE", it indicates that the BCP records
	** have been written to the log file.  The stall mask is turned
	** off to allow work to proceed in the system.  The CP protocols
	** are continued by requesting all servers in the installation to
	** flush dirty pages from their data caches.
	**
	** If there are no active servers in the installation, then we
	** skip straight to the signaling the End Consistency Point operation.
	*/

	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_BCPDONE");
	    return (LG_BADPARAM);
	}

	/* lgd_mutex protects lgd_status */
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	/*
	** Check for request type:
	**     item == TRUE:  BCP written, unstall system and signal CPFLUSH
	**     item == FALSE: Stall system so BCP can be written.
	*/

	if (*(i4 *)item)
	{
	    /*
	    ** Instead of locking and walking the lpb_q to find
	    ** LPB_CPAGENTs, check the count in the lgd; it
	    ** represents the number of non-VOID cpagents.
	    */

	    /*
	    ** Servers that are marked LPB_VOID are remnants of dead
	    ** processes that are currently being cleaned up (they're
	    ** dead ... they're all messed up).  They should not be
	    ** signaled for a CPFlush as they will never respond to
	    ** the signal.
	    */

	    /*
	    ** Turn off the LGD_CPNEEDED flag and un-stall the system
	    ** since the BCP is finished being written.
	    */

	    if (lgd->lgd_n_cpagents)
		LG_signal_event(LGD_CPFLUSH, LGD_CPNEEDED | LGD_BCPSTALL, TRUE);
	    else
		LG_signal_event(LGD_ECP, LGD_CPNEEDED | LGD_BCPSTALL, TRUE);

	    /* Resume any sessions waiting on the BCPSTALL condition. */
	    LG_resume(&lgd->lgd_wait_stall, NULL);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	}
	else
	{
	    /*
	    ** Start of routine to write BCP.
	    ** Turn off BCPDONE status and stall LGwrites until
	    ** the BCP is complete.
	    **
	    ** We're not "stalled" until two things happen:
	    **
	    **    o new LGwrites are blocked, accomplished by
	    **      setting LGD_BCPSTALL, which is checked by
	    **	    LGwrite and suspends the issuing transaction.
	    **    o letting in-process LGwrites complete. This
	    **	    assures a static snapshot of the logging
	    **	    system state.
	    **	    While in LGwrite, each associated lxb_mutex
	    **	    is held for the duration of the write. Here
	    **	    we simply acquire each lxb_mutex, which won't
	    **	    be granted until the LGwrite completes. We
	    **	    don't really care if the transaction is doing
	    **	    a LGwrite or not, just that we can momentarily
	    **	    get, then release, its mutex.
	    **	    Note that we only mutex protected, writable
	    **	    transactions; non-protected transactions (other
	    **	    than the RCP, which is us) won't be doing
	    **	    LGwrites and are of no concern.
	    */
	    lgd->lgd_status &= ~(LGD_BCPDONE);
	    /* This stalls new LGwrites */
	    lgd->lgd_status |= LGD_BCPSTALL;
	    (VOID)LG_unmutex(&lgd->lgd_mutex);

	    /* Let in-process writes complete */
	    lxbb_table = (SIZE_TYPE*)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    for ( i = 1; i <= lgd->lgd_lxbb_count; i++ )
	    {
		lxb = (LXB*)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
		if ( lxb->lxb_type == LXB_TYPE && 
		     lxb->lxb_status & LXB_PROTECT &&
		    !(lxb->lxb_status & LXB_READONLY) )

		{
		    /* This will block if in LGwrite (possibly elsewhere, too) */
		    (VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		}
	    }
	}
	break;
    
    case LG_A_ECPDONE:

	/* Acknowledgement of ECP completion. */
	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_ECPDONE");
	    return (LG_BADPARAM);
	}

	/* lock lgd_mutex to block lgd_status, all that BOF/EOF stuff */
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	if (*(i4 *)item)
	{
	    /*
	    ** If Delay_bcp is set then instead of signaling the ECP complete,
	    ** start the Consistency Point protocol all over again. (Delay_bcp
	    ** is set when somebody requests an immediate BCP and then waits for
	    ** the ECPDONE event - like checkpoint.  We cannot signal the ECP
	    ** until the newly requested BCP is complete.)
	    */
	    if (lgd->lgd_delay_bcp)
	    {
		lgd->lgd_delay_bcp = 0;
		lgd->lgd_no_bcp = 1;
		LG_signal_event(LGD_CPNEEDED, LGD_ECP, TRUE);
	    }
	    else
	    {
		lgd->lgd_no_bcp = 0;
		LG_signal_event(LGD_ECPDONE, LGD_ECP, TRUE);
	    }

	    /*
	    ** Check if we can move the BOF forward to the new Consistency
	    ** Point.  Wake up the archiver if the correct number of
	    ** consistency points have been executed.
	    */
	    LG_compute_bof(&lgd->lgd_local_lfb,
		lgd->lgd_local_lfb.lfb_header.lgh_cpcnt *
			    lgd->lgd_local_lfb.lfb_header.lgh_l_cp);

	    if (lgd->lgd_status & (LGD_FORCE_ABORT | LGD_LOGFULL))
	    {
		if (lgd->lgd_no_bcp == 0)
		{
		    /*
		    ** By the time this consistency point is finished, it may
		    ** already be time for another consistency point to be
		    ** started.  Normally, the next consistency point will be
		    ** signaled on the next allocate_lbb() call.
		    ** However, the logging system may have hit the logfull state
		    ** while we were executing the consistency point, and may
		    ** now be waiting for a new consistency point in order for
		    ** the archiver to clean up space in the log file.  In this
		    ** case, allocate_lbb() will never be called (because we
		    ** are in LOGFULL state) so we need to signal the new
		    ** consistency point here.
		    */
		    if (lgd->lgd_local_lfb.lfb_header.lgh_end.la_sequence > 
				lgd->lgd_local_lfb.lfb_header.lgh_cp.la_sequence)
		    {
			used_blocks = 
			    lgd->lgd_local_lfb.lfb_header.lgh_count + 1 -
			    (lgd->lgd_local_lfb.lfb_header.lgh_cp.la_block - 
				  lgd->lgd_local_lfb.lfb_header.lgh_end.la_block);
		    }
		    else
		    {
			used_blocks =
			    lgd->lgd_local_lfb.lfb_header.lgh_end.la_block - 
			    lgd->lgd_local_lfb.lfb_header.lgh_cp.la_block + 1;
		    }

		    if (used_blocks > lgd->lgd_local_lfb.lfb_header.lgh_l_cp)
		    {
			if (lgd->lgd_no_bcp == 0)
			{
			    lgd->lgd_no_bcp = 1;
			    LG_signal_event(LGD_CPNEEDED, LGD_ECPDONE, TRUE);
			}
		    }
		}

		/*
		** If logfull was signalled because of a consistency point
		** stall condition, then we can turn off logfull now.
		*/
		used_blocks = LG_used_logspace(&lgd->lgd_local_lfb, TRUE);
		if (used_blocks <= lgd->lgd_local_lfb.lfb_header.lgh_l_logfull)
		{
		    lgd->lgd_status &= ~LGD_LOGFULL;
		}
	    }

	    /* Awaken any threads waiting on BCP or LOGFULL */
	    LG_resume(&lgd->lgd_wait_stall, NULL);
	}
	else
	    lgd->lgd_status &= ~(LGD_ECPDONE);

	(VOID)LG_unmutex(&lgd->lgd_mutex);

	break;

    case LG_A_RECOVERDONE:
	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_RECOVERDONE");
	    return (LG_BADPARAM);
	}
	
	/* lock lgd_mutex to block lgd_status, all that BOF/EOF stuff */
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	if (*(i4 *)item)
	{
	    /*
	    ** Signal new Consistency Point that will be processed after
	    ** recovery is complete.   If a CP is already in progress,
	    ** set delay_bcp to start a new one when the pending one is
	    ** complete.  The new BCP will prevent us from having to 
	    ** re-process recovery work that we have just completed.
	    */
	    if (!lgd->lgd_no_bcp)
	    {
		lgd->lgd_no_bcp = 1;
		LG_signal_event(LGD_CPNEEDED, LGD_RECOVER | LGD_ECPDONE, TRUE);
	    }
	    else
	    {
		lgd->lgd_delay_bcp = 1;
		lgd->lgd_status &= ~LGD_RECOVER;
	    }
	}
	else
	    lgd->lgd_status |= LGD_RECOVER;

	(VOID)LG_unmutex(&lgd->lgd_mutex);

	break;

    case LG_A_M_ABORT:
	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_M_ABORT");
	    return (LG_BADPARAM);
	}

	if (*(i4 *)item)
	{
	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
		return(status);
	    lgd->lgd_status &= ~LGD_MAN_ABORT;
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	}
	break;

    case LG_A_M_COMMIT:
	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_M_COMMIT");
	    return (LG_BADPARAM);
	}

	if (*(i4 *)item)
	{
	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
		return(status);
	    lgd->lgd_status &= ~LGD_MAN_COMMIT;
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	}
	break;

    case LG_A_SHUTDOWN:
	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_SHUTDOWN");
	    return (LG_BADPARAM);
	}

	if ( *(i4*)item )
	{
	    LXB		*slxb, *handle_lxb;
	    LXBQ	*handle_lxbq;
	    LXBQ	*lxbq;
	    i4		AbortXA;
	    bool	ActiveXA;
	    i4		ShutDown = LGD_START_SHUTDOWN;

	    if ( lgd->lgd_lpb_inuse == 1 )
		ShutDown |= LGD_IMM_SHUTDOWN;
	    else if ( lgd->lgd_ldb_inuse == 1 )
		ShutDown |= LGD_ACP_SHUTDOWN;

	    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_mutex);
	    LG_signal_event(ShutDown, 0, TRUE);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);

	    /* 
	    ** If we can't shutdown immediately because of database(s)
	    ** still in use, then check to see if there may be 
	    ** disassociated XA transactions holding up the show.
	    ** Disassociated XA transactions have XA_ENDed but not
	    ** yet committed.
	    **
	    ** Search the list of SHARED transactions. For each,
	    ** if all of the HANDLES are in a disassociated state
	    ** (LXB_ORPHAN), then we forcibly abort them and rollback
	    ** the global transaction. If any of the handles are
	    ** not disassociated (belong to some active server session)
	    ** we hope that things will eventually work out, though
	    ** it's possible that shutdown will hang indefinitely
	    ** if those active handles don't cooperate.
	    **
	    ** Note that those in WILLING_COMMIT cannot be aborted
	    ** nor committed and will cause shutdown to hang 
	    ** awaiting manual disposition using LARTOOL,
	    ** but that's expected behavior.
	    */
	    if ( !(ShutDown & LGD_IMM_SHUTDOWN) )
	    {
		(VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
		lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lgd->lgd_slxb.lxbq_next);

		AbortXA = 0;
		
		/* For each SHARED transaction */
		while ( lxbq != (LXBQ*)&lgd->lgd_slxb.lxbq_next )
		{
		    slxb = (LXB*)((char*)lxbq - CL_OFFSETOF(LXB,lxb_slxb));

		    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

		    /* Only SHARED XA txns will be Orphaned */
		    if ( slxb->lxb_status & LXB_SHARED &&
			 slxb->lxb_status & LXB_ORPHAN &&
			 (slxb->lxb_status & (LXB_ABORT | LXB_WILLING_COMMIT)) == 0 )
		    {
			/* Look for active handles */
			ActiveXA = FALSE;
			handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);
			while ( handle_lxbq != (LXBQ*)&slxb->lxb_handles && !ActiveXA )
			{
			    handle_lxb = (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
			    if ( handle_lxb->lxb_status & LXB_ORPHAN )
				handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxbq->lxbq_next);
			    else
			    {
				/* Look no furthur */
				ActiveXA = TRUE;
			    }
			}
			/* All inactive? Abort the SHARED and all HANDLES */
			if ( !ActiveXA )
			{
			    slxb->lxb_status |= LXB_MAN_ABORT;
			    AbortXA++;
			}
		    }
		    lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_next);
		    (VOID)LG_unmutex(&slxb->lxb_mutex);
		}
		(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

		/* If any to abort, signal the RCP to do it */
		if ( AbortXA )
		    LG_signal_event(LGD_MAN_ABORT, 0, FALSE);
	    }
	}
	else
	{
	    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_mutex);
	    /* Shutdown the recovery process immediately. */
	    LG_signal_event(LGD_IMM_SHUTDOWN, LGD_START_SHUTDOWN, TRUE);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	}


	break;

    case LG_A_PASS_ABORT:
	/*
	** Disown transaction and mark it PASS_ABORT.  The transaction will
	** be orphaned by its current server until it can be rolled back
	** by the RCP.
	**
	** LGalter(LG_A_PASS_ABORT, item)
	**
	**     inputs: item	- lx_id - lg transaction identifier
	**
	** This call is made when a server fails in its attempt to undo
	** a transaction.  The RCP makes an attempt to execute the undo
	** in the hopes that the problem is temporary or resource related.
	** If the RCP fails in its undo attempt, the database will be left
	** inconsistent.
	**
	** While in an orphaned state, the transaction is linked to its own
	** orphan LPD and to the RCP's LPB.
	*/
	/* Change the status of a database. */

	if (l_item < sizeof(LG_LXID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LXID), 0, "LG_A_PASS_ABORT");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_PASS_ABORT");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_PASS_ABORT");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	    return(status);

	/*
	** If handle to a SHARED LXB, remove the handle from
	** the list, transform it into a regular non-HANDLE
	** LXB.
	** When no more handles exist, PASS_ABORT the SHARED LXB.
	*/
	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

	    next_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_next);
	    prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_prev);
	    next_lxbq->lxbq_prev = lxb->lxb_handles.lxbq_prev;
	    prev_lxbq->lxbq_next = lxb->lxb_handles.lxbq_next;

	    lxb->lxb_status &= ~LXB_SHARED_HANDLE;
	    lxb->lxb_shared_lxb = 0;
	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	    /* If handles remain, return */
	    if ( --slxb->lxb_handle_count )
	    {
		(VOID)LG_unmutex(&slxb->lxb_mutex);
		break;
	    }

	    /* Removed SHARED LXB from LGD list */
	    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
	    next_lxbq = 
		(LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_next);
	    prev_lxbq = 
		(LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_prev);
	    next_lxbq->lxbq_prev = slxb->lxb_slxb.lxbq_prev;
	    prev_lxbq->lxbq_next = slxb->lxb_slxb.lxbq_next;
	    (VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

	    /* Switch to SHARED LXB for orphaning */
	    slxb->lxb_status &= ~LXB_SHARED;
	    lxb = slxb;
	}

	/*
	** Set the transaction in pass-abort state.
	**
	** If aborting a Willing Commit transaction, then set the MAN_ABORT
	** flag, otherwise the RCP will refuse the abort.
	**
	** Can't abort a SHARED txn until all handles are gone.
	*/
	if ( (lxb->lxb_status & LXB_SHARED) == 0 )
	{
	    lxb->lxb_status |= LXB_PASS_ABORT;
	    if (lxb->lxb_status & LXB_WILLING_COMMIT)
		lxb->lxb_status |= LXB_MAN_ABORT;

	    /* Disown the transaction if it isn't already */
	    if ( (lxb->lxb_status & LXB_ORPHAN) == 0 )
		status = LG_orphan_xact(lxb);

	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	    if (status != OK)
		return (status);

	    /* Signal the RCP to recover the pass abort transaction */
	    LG_signal_event(LGD_RECOVER, 0, FALSE);
	}
	else
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	break;

    case LG_A_SERVER_ABORT:
    case LG_A_OFF_SERVER_ABORT:
	if (l_item < sizeof(LG_LXID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LXID),
			0, "LG_A_SERVER_ABORT/LG_A_OFF_SERVER_ABORT");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_SERVER_ABORT/LG_A_OFF_SERVER_ABORT");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_SERVER_ABORT/LG_A_OFF_SERVER_ABORT");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	    return(status);

	/* If handle to SHARED LXB, fetch and mutex the SHARED LXB */
	slxb = lxb;

	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    if ( lxb->lxb_shared_lxb == 0 )
	    {
		xid1.id_lgid = lxb->lxb_id;
		uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, xid1.id_i4id,
		    0, lxb->lxb_status,
		    0, "LG_A_SERVER_ABORT/LG_A_OFF_SERVER_ABORT");
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }
	    
	    slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    if ( slxb->lxb_type != LXB_TYPE ||
		(slxb->lxb_status & LXB_SHARED) == 0 )
	    {
		xid1.id_lgid = slxb->lxb_id;
		xid2.id_lgid = lxb->lxb_id;
		uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, slxb->lxb_type,
		    0, LXB_TYPE,
		    0, xid1.id_i4id,
		    0, slxb->lxb_status,
		    0, xid2.id_i4id,
		    0, "LG_A_SERVER_ABORT/LG_A_OFF_SERVER_ABORT");
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }

	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
	}

	if ( flag == LG_A_SERVER_ABORT )
	{
	    lxb->lxb_status |= LXB_SERVER_ABORT;

	    /*
	    ** Also immediately mark the SHARED LXB as in abort.
	    ** This prevents the (shared) txn from being selected for
	    ** force-abort.
	    */
	    slxb->lxb_status |= LXB_SERVER_ABORT;

	    /*
	    ** If XA Global Transaction, discard disassociated handles,
	    ** then see what's left:
	    */
	    if ( slxb->lxb_status & LXB_SHARED && slxb->lxb_status & LXB_ORPHAN )
	    {
		LXBQ	*lxbq;
		LXB	*handle_lxb;
		LG_I4ID_TO_ID log_id;
		i4	lock_id;

		lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);

		while ( lxbq != (LXBQ*)&slxb->lxb_handles )
		{
		    handle_lxb = (LXB*)((char*)lxbq - CL_OFFSETOF(LXB,lxb_handles));
		    lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);

		    /* Note this will -never- be "our" lxb */
		    if ( handle_lxb->lxb_status & LXB_ORPHAN )
		    {
			(VOID)LG_unmutex(&slxb->lxb_mutex);
			(VOID)LG_mutex(SEM_EXCL, &handle_lxb->lxb_mutex);

			if ( handle_lxb->lxb_type == LXB_TYPE &&
			     handle_lxb->lxb_shared_lxb == LGK_OFFSET_FROM_PTR(slxb) &&
			     handle_lxb->lxb_status & LXB_ORPHAN )
			{
			    handle_lxb->lxb_status |= LXB_SERVER_ABORT;
			    log_id.id_lgid = handle_lxb->lxb_id;
			    lock_id = handle_lxb->lxb_lock_id;

			    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);

			    /* Toss the handle, reducing slxb's handle count */
			    status = LGend((LG_LXID)log_id.id_i4id, 0, sys_err);
			    if ( status != OK )
			    {
				uleFormat(NULL, status, sys_err, ULE_LOG, NULL,
				    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
				uleFormat(NULL, E_DM900E_BAD_LOG_END, sys_err, ULE_LOG, NULL,
				    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
				    0, log_id.id_i4id);
			    }

			    /* ...and its lock list handle, if any */
			    if ( lock_id )
			    {
				status = LKrelease(LK_ALL, lock_id, (LK_LKID *)NULL, 
						    (LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, sys_err);
				if ( status != OK )
				{
				    uleFormat(NULL, status, sys_err, ULE_LOG, NULL,
					(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
				    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, sys_err, ULE_LOG, NULL,
					(char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, lock_id);
				}
			    }
			}
			else
			    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);

			(VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
			lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);
		    }
		}
	    }

	    /*
	    ** If this is the last (only) handle to this LXB,
	    ** notify caller that recovery may procede.
	    **
	    ** Without this notification, it's expected that the
	    ** handle txn will simply LGend itself, reducing the
	    ** number of handles (see dmxe_abort()).
	    */
	    if ( slxb->lxb_handle_count <= 1 )
	    {
		status = LG_CONTINUE;

#ifdef LG_SHARED_DEBUG
		if ( lxb->lxb_status & LXB_SHARED_HANDLE )
		    TRdisplay("%@ HANDLE lxb %x LG_A_SERVER_ABORT on SHARED %x, handles %d\n",
			*(u_i4*)&lxb->lxb_id, *(u_i4*)&slxb->lxb_id,
			slxb->lxb_handle_count);
#endif /* LG_SHARED_DEBUG */
	    }
	    else if ( slxb->lxb_status & LXB_SHARED )
	    {
		/* 
		** If shared transaction and there are additional
		** handle LXBs and this is not an XA transaction,
		** manually abort the remainder. If XA, we assume
		** that rollbacks (dmxe_abort()) will be issued 
		** for the remaining connections by the Transaction
		** Server; if not XA, we can make no such assumption!
		*/
		if ( slxb->lxb_dis_tran_id.db_dis_tran_id_type !=
		       DB_XA_DIS_TRAN_ID_TYPE ||
		     slxb->lxb_status & LXB_ORPHAN )
		{
		    /* But don't do it again if we already have */
		    if ( (slxb->lxb_status & LXB_MAN_ABORT) == 0 )
		    {
			slxb->lxb_status |= LXB_MAN_ABORT;
			LG_abort_transaction(slxb);
		    }
		}
	    }

	}
	else
	{
	    lxb->lxb_status &= ~LXB_SERVER_ABORT;

	    /* Unmark the SHARED LXB as in abort */
	    slxb->lxb_status &= ~LXB_SERVER_ABORT;

#ifdef LG_SHARED_DEBUG
	    if ( lxb->lxb_status & LXB_SHARED_HANDLE )
		TRdisplay("%@ HANDLE lxb %x LG_A_OFF_SERVER_ABORT on SHARED %x, handles %d\n",
		    *(u_i4*)&lxb->lxb_id, *(u_i4*)&slxb->lxb_id,
		    slxb->lxb_handle_count);
#endif /* LG_SHARED_DEBUG */

	}

	if ( slxb != lxb )
	    (VOID)LG_unmutex(&slxb->lxb_mutex);
	(VOID)LG_unmutex(&lxb->lxb_mutex);

	return(status);

    case LG_A_COMMIT:
    case LG_A_ABORT:
	/* 
	** Alter the status of a transaction to MANUAL COMMIT 
	** or MANUAL ABORT.
	*/

	if (l_item < sizeof(LG_LXID))
	    return (LG_BADPARAM);
	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	    return (LG_BADPARAM);

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	    return(status);

	/* Recheck LXB after waiting for mutex */
	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    return (LG_BADPARAM);
	}

	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	/*
	** Avoid disaster by disallowing COMMIT/ABORT of internal transactions,
	** either nefarious or due to a typo.
	*/
	if ( ldb->ldb_status & LDB_NOTDB )
	{
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    return (LG_BADPARAM);
	}

	slxb = lxb;

	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    /* Fetch shared LXB */
	    slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    if ( !slxb || slxb->lxb_type != LXB_TYPE || 
		(slxb->lxb_status & LXB_SHARED) == 0 )
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }
	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
	}

	/*
	** As originally designed, LARTOOL only was to allow users to manually
	** commit or abort a transaction which was in WILLING COMMIT state.
	** Unfortunately, the first release of LARTOOL allowed users to abort
	** ANY transactions, and we fear that users have grown accustomed to
	** this functionality, so we are hesitant to remove it. If we ever
	** become brave, we can simply remove the 'if flag == LG_A_COMMIT'
	** test following this comment...
	*/
	if (flag == LG_A_COMMIT)
	{
	    /*
	    ** unless the (SHARED) transaction is in WILLING COMMIT state, reject the
	    ** operation.
	    **
	    ** Note that Disassociated XA transactions are never put in a
	    ** WILLING_COMMIT state, and can only be committed here when
	    ** all attached handles have XA_PREPAREd or if optimized for
	    ** one-phase COMMIT (no PREPARE is done).
	    */
	    if ( (slxb->lxb_status & LXB_SHARED && slxb->lxb_status & LXB_ORPHAN &&
		  slxb->lxb_handle_preps != slxb->lxb_handle_count &&
		  (slxb->lxb_status & LXB_ONEPHASE) == 0) ||
		 ((slxb->lxb_status & LXB_ORPHAN) == 0 &&
		  (slxb->lxb_status & LXB_WILLING_COMMIT) == 0) )
	    {
		(VOID)LG_unmutex(&slxb->lxb_mutex);
		if ( lxb != slxb )
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		return (LG_BADPARAM);
	    }
	}

	/*
	** Check the owner of the transaction, either the server or the rcp.
	**
	** If the rcp owns the transaction, then signal it to do the manual
	** commit or abort.
	**
	** If the server owns the transaction, then signal a force-abort to
	** cause the transation to be terminated.  Manual commit cannot be
	** issued on a server-owned transaction.
	*/

	if ((lpb->lpb_status & LPB_MASTER) == 0 &&
	    (lpb->lpb_status & LPB_RUNAWAY) == 0 &&
	    (lxb->lxb_status & LXB_REASSOC_WAIT) == 0
	   )
	{
	    /*
	    ** Ask the force-abort thread to please abort this transaction.
	    */

	    if (flag == LG_A_ABORT)
	    {
		lxb->lxb_status |= LXB_MAN_ABORT;

		/* Abort the transaction */
		LG_abort_transaction(lxb);
	    }
	    else
	    {
		/* 
		** Don't allow manually commit a transaction if the transaction
		** is owned by a server. User will have to kill the FE 
		** connection, then manually commit or abort the transaction.
		*/

		(VOID)LG_unmutex(&slxb->lxb_mutex);
		if ( lxb != slxb )
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		return (LG_SYNCHRONOUS);
	    }
	}
	else
	{
	    if (flag == LG_A_ABORT)
	    {
		/* Mark the (SHARED) txn for manual abort */
		slxb->lxb_status |= LXB_MAN_ABORT;
		LG_signal_event(LGD_MAN_ABORT, 0, FALSE);
	    }
	    else
	    {
		/* Mark the (SHARED) txn for manual commit */
		slxb->lxb_status |= LXB_MAN_COMMIT;
		LG_signal_event(LGD_MAN_COMMIT, 0, FALSE);
	    }
	}

	(VOID)LG_unmutex(&slxb->lxb_mutex);
	if ( lxb != slxb )
	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	break;

    case LG_A_PREPARE:
    case LG_A_WILLING_COMMIT:
	/*
	** Prepare a transaction for WILLING_COMMIT or
	** alter the status of a transaction to WILLING COMMIT.
	*/
     
	if (l_item < sizeof(LG_CONTEXT))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_CONTEXT),
			0, "LG_A_WILLING_COMMIT/LG_A_PREPARE");
	    return (LG_BADPARAM);
	}

	ctx = (LG_CONTEXT *)item;
	lx_id.id_i4id = ctx->lg_ctx_id;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_WILLING_COMMIT/LG_A_PREPARE");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_WILLING_COMMIT/LG_A_PREPARE");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	    return(status);

	/*
	** If preparing for willing commit and this is a handle
	** to a shared txn, count another preparer. When all 
	** have prepared, return LG_CONTINUE to indicate
	** that the willing commit process may now procede.
	** When that is complete, we'll be called back with 
	** LG_A_WILLING_COMMIT to mark the (shared) transaction 
	** in a willing commit state.
	*/
	slxb = lxb;

	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    if ( lxb->lxb_shared_lxb == 0 )
	    {
		xid1.id_lgid = lxb->lxb_id;
		uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, xid1.id_i4id,
		    0, lxb->lxb_status,
		    0, "LG_A_WILLING_COMMIT/LG_A_PREPARE");
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }
	    
	    slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    if ( slxb->lxb_type != LXB_TYPE ||
		(slxb->lxb_status & LXB_SHARED) == 0 )
	    {
		xid1.id_lgid = slxb->lxb_id;
		xid2.id_lgid = lxb->lxb_id;
		uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, slxb->lxb_type,
		    0, LXB_TYPE,
		    0, xid1.id_i4id,
		    0, slxb->lxb_status,
		    0, xid2.id_i4id,
		    0, "LG_A_WILLING_COMMIT/LG_A_PREPARE");
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }

	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
	}

	/* Just preparing for WILLING COMMIT? */
	if ( flag == LG_A_PREPARE )
	{
	    if ( (lxb->lxb_status & LXB_PREPARED) == 0 )
	    {
		/* Flag handle as in willing commit */
		lxb->lxb_status |= LXB_PREPARED;

		/* Count another prepared handle */
		if ( ++slxb->lxb_handle_preps >= slxb->lxb_handle_count )
		{
		    /* Let caller know that all have prepared */
		    status = LG_CONTINUE;
		}

#ifdef LG_SHARED_DEBUG
		if ( lxb->lxb_status & LXB_SHARED_HANDLE )
		    TRdisplay("%@ HANDLE lxb %x PREPARED on SHARED %x, count %d preps %d\n",
			*(u_i4*)&lxb->lxb_id, *(u_i4*)&slxb->lxb_id,
			slxb->lxb_handle_count, slxb->lxb_handle_preps);
#endif /* LG_SHARED_DEBUG */
	    }
	}
	else
	/* Set txn as now in willing commit state */
	if ( slxb->lxb_handle_preps >= slxb->lxb_handle_count )
	{
	    if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	    {
		slxb->lxb_status |= LXB_WILLING_COMMIT;

#ifdef LG_SHARED_DEBUG
		TRdisplay("%@ SHARED lxb %x in WILLING_COMMIT, count %d preps %d\n",
		    *(u_i4*)&slxb->lxb_id,
		    slxb->lxb_handle_count, slxb->lxb_handle_preps);
#endif /* LG_SHARED_DEBUG */

	    }
	    else
	    {
		/* Non-shared txn is now in WILLING COMMIT state */
		lxb->lxb_status |= (LXB_WILLING_COMMIT | LXB_DISTRIBUTED);
		lxb->lxb_dis_tran_id = ctx->lg_ctx_dis_eid;
	    }
	}
	
	if ( slxb != lxb )
	    (VOID)LG_unmutex(&slxb->lxb_mutex);

	(VOID)LG_unmutex(&lxb->lxb_mutex);
	return(status);

    case LG_A_OWNER:
	/*
	** Adopt a willing commit transaction into the caller's session
	** and process context. 
	**
	** LGalter(LG_A_OWNER, item)
	**
	**     inputs: item	- LG_TRANSACTION struct:
	**			    tr_id	- internal lxb id of the xact
	**					  to adopt
	**			    tr_pr_id	- internal lpb id of the caller
	**			    tr_lpd_id	- internal lpd id of the caller
	**
	** This call is made when a process wishes to resolve a willing
	** commit transaction that had previously been orphaned by a
	** LGalter(LG_A_REASSOC) call.
	*/
	{
	    LG_TRANSACTION	*tr;
	    LPB			*old_lpb;
	   
	    if (l_item < sizeof(LG_TRANSACTION))
	    {
		uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_TRANSACTION),
			    0, "LG_A_OWNER");
		return (LG_BADPARAM);
	    }

	    tr = (LG_TRANSACTION *)item;

	    /*
	    ** Check the lxb id parameter.
	    */
	    lx_id.id_i4id = tr->tr_id;

	    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_A_OWNER");
		return (LG_BADPARAM);
	    }

	    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	    if (lxb->lxb_type != LXB_TYPE ||
		lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			    0, lx_id.id_lgid.id_instance,
			    0, "LG_A_OWNER");
		return (LG_BADPARAM);
	    }

	    if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
		return(status);

	    /*
	    ** Check the lpb id parameter.
	    */
	    lg_id.id_i4id = tr->tr_pr_id;

	    if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			    0, "LG_A_OWNER(2)");
		return (LG_BADPARAM);
	    }

	    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	    if (lpb->lpb_type != LPB_TYPE ||
		lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			    0, lg_id.id_lgid.id_instance, 0, "LG_A_OWNER(2)");
		return (LG_BADPARAM);
	    }
	    
	    /*
	    ** Check the lpd id parameter.
	    */
	    db_id.id_i4id = tr->tr_lpd_id;

	    if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			    0, "LG_A_OWNER(3)");
		return (LG_BADPARAM);
	    }

	    lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

	    if (lpd->lpd_type != LPD_TYPE ||
		lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			    0, db_id.id_lgid.id_instance, 0, "LG_A_OWNER(3)");
		return (LG_BADPARAM);
	    }
	    
	    /*
	    ** Verify that the transaction is left orphaned in reassoc wait.
	    */
	    if (((lxb->lxb_status & LXB_REASSOC_WAIT) == 0) ||
		((lxb->lxb_status & LXB_ORPHAN) == 0))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_A_OWNER");
		uleFormat(NULL, E_DMA48D_ADOPT_NONORPHAN, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lx_id.id_lgid.id_id, 0, lxb->lxb_status);
		return (LG_BADPARAM);
	    }

	    /* Change the conext of the transaciton. */
	    lxb->lxb_status &= ~LXB_REASSOC_WAIT;
	    lxb->lxb_status |= LXB_RESUME;

	    /*
	    ** Link the transaction to the caller's context.
	    */
	    status = LG_adopt_xact(lxb, lpd);

	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	    if (status != OK)
		return (status);
	}
        break;

    case LG_A_CONTEXT:
	/*
	** Restore the context of a transaction to its state prior
	** to a crash so that the transaction can be recovered.
	**
	** This is used by the recovery system during crash restarts
	** to rebuild xacts that must be Undone or restored to Willing
	** Commit state.
	**
	** Normally, context is restored only once per tx.  But, if the
	** transaction is a willing-commit transaction, recovery may
	** set context twice -- the second time with the correct distributed
	** transaction ID.  It is important to avoid doing any operation
	** twice, if that operation is not idempotent.
	*/

	if (l_item < sizeof(LG_CONTEXT))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_CONTEXT),
			0, "LG_A_CONTEXT");
	    return (LG_BADPARAM);
	}

	ctx = (LG_CONTEXT *)item;
	lx_id.id_i4id = ctx->lg_ctx_id;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_CONTEXT");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id.id_lgid.id_instance,
		    0, "LG_A_CONTEXT");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	    return(status);

	lxb->lxb_first_lga = ctx->lg_ctx_first;
	lxb->lxb_last_lga = ctx->lg_ctx_last;
	lxb->lxb_lxh.lxh_tran_id = ctx->lg_ctx_eid;
	lxb->lxb_dis_tran_id = ctx->lg_ctx_dis_eid;
	lxb->lxb_cp_lga = ctx->lg_ctx_cp;

	/* 
	** Make sure the next transaction ID in the current logging system
	** is greater than that of this restored transaction.
	**
	** Note that lxbb_mutex protects the system-wide transaction id.
	*/
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex))
	    return(status);

	if (ctx->lg_ctx_eid.db_high_tran >
		    lgd->lgd_local_lfb.lfb_header.lgh_tran_id.db_high_tran)
	{
	    lgd->lgd_local_lfb.lfb_header.lgh_tran_id.db_high_tran =
					ctx->lg_ctx_eid.db_high_tran;
	    lgd->lgd_local_lfb.lfb_header.lgh_tran_id.db_low_tran =
					ctx->lg_ctx_eid.db_low_tran + 1024;
	}
	else if (ctx->lg_ctx_eid.db_low_tran >
		    lgd->lgd_local_lfb.lfb_header.lgh_tran_id.db_low_tran)
	{   
	    lgd->lgd_local_lfb.lfb_header.lgh_tran_id.db_low_tran =
				ctx->lg_ctx_eid.db_low_tran + 1024;
	}

	(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

	/*
	** Check the transaction type (PROTECT or NONPROTECT) to determine
	** whether to make transaction active or not.
	**
	** It is assumed that the reason for restoring this transaction
	** context is as part of recovering a transaction which has
	** previously logged some updates.  Therefore, the transaction
	** is marked in RECOVER state and set as active.
	**
	** During rollforward processing, transactions are restored in
	** NONPROTECT mode and are not made active.
	**
	** If the transaction is already ACTIVE, don't do it again.  (else
	** the active queues get snarled.)
	*/
	if ((lxb->lxb_status & (LXB_PROTECT | LXB_INACTIVE)) == (LXB_PROTECT | LXB_INACTIVE))
	{
	    LXBQ	*bucket_array;
	    LXBQ	*lxbq; 
	    LXBQ	*prev_lxbq;

	    bucket_array = (LXBQ *)LGK_PTR_FROM_OFFSET(
				lgd->lgd_lxh_buckets);
	    
	    lxbq = &bucket_array[lxb->lxb_lxh.lxh_tran_id.db_low_tran
			    % lgd->lgd_lxbb_size];

	    /*
	    ** Set transaction in active, recover state.
	    */
	    lxb->lxb_status &= ~LXB_INACTIVE;
	    lxb->lxb_status |= (LXB_ACTIVE | LXB_RECOVER);

	    /*
	    ** Insert the LXB on the end of the active queue
	    ** and insert on the active transaction hash queue.
	    ** Only active+protect txns appear on the active queue.
	    */
	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lxb_q_mutex))
		return(status);
	
	    lxb->lxb_next = LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);
	    lxb->lxb_prev = lgd->lgd_lxb_prev;
	    prev_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_prev);
	    prev_lxb->lxb_next = 
		    lgd->lgd_lxb_prev = LGK_OFFSET_FROM_PTR(lxb);
	    lgd->lgd_protect_count++;

	    /* Insert LXB on the tail of active transaction hash queue */
	    lxb->lxb_lxh.lxh_lxbq.lxbq_next = 
		    LGK_OFFSET_FROM_PTR(&lxbq->lxbq_next);
	    lxb->lxb_lxh.lxh_lxbq.lxbq_prev = lxbq->lxbq_prev;
	    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
	    prev_lxbq->lxbq_next = lxbq->lxbq_prev = 
		    LGK_OFFSET_FROM_PTR(&lxb->lxb_lxh.lxh_lxbq);

	    (VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	}

	/* If MVCC, insert LXB last on LDB's list of open transactions */
	lpd = (LPD*)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	ldb = (LDB*)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	if ( ldb->ldb_status & LDB_MVCC &&
	     lxb->lxb_status & LXB_PROTECT )
	{
	    lxbq = &ldb->ldb_active_lxbq;
	    lxb->lxb_active_lxbq.lxbq_next = 
		    LGK_OFFSET_FROM_PTR(&lxbq->lxbq_next);
	    lxb->lxb_active_lxbq.lxbq_prev = lxbq->lxbq_prev;
	    prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
	    prev_lxbq->lxbq_next = lxbq->lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lxb->lxb_active_lxbq);
	}
	else
	    lxb->lxb_active_lxbq.lxbq_next = 
		lxb->lxb_active_lxbq.lxbq_prev = 0;

	/* Set tranid "open" in xid array */
	SET_XID_ACTIVE(lxb);
	
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	break;

    case LG_A_DBCONTEXT:
	{
	    LG_DATABASE	    *db;

	    /*
	    ** Restore the context of a database to its state prior
	    ** to a crash during installation restart recovery.
	    **
	    ** Inputs:
	    **     db.db_id	    - internal LG dbid for the recovered db.
	    **     db.db_f_jnl_la   - Log address of 1st journal record.
	    **     db.db_l_jnl_la   - Log address of last journal record.
	    **
	    ** Currently, only the database journal window information
	    ** is restored.
	    **
	    ** Since this code is only executed during coldstart recovery,
	    ** it's not necessary to utilise mutexes to block concurrent
	    ** updaters.
	    */

	    if (l_item < sizeof(LG_DATABASE))
	    {
		uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_DATABASE),
			    0, "LG_A_DBCONTEXT");
		return (LG_BADPARAM);
	    }

	    db = (LG_DATABASE *)item;
	    db_id.id_i4id = db->db_id;

	    /*
	    ** Lookup the LDB using the passed in DBID
	    */
	    if (db_id.id_lgid.id_id == 0 || (i4) db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, db_id.id_lgid.id_id, 0, lgd->lgd_ldbb_count,
			    0, "LG_A_DBCONTEXT");
		return (LG_BADPARAM);
	    }

	    ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);
	    lfb = (LFB *)LGK_PTR_FROM_OFFSET(ldb->ldb_lfb_offset);

	    if (ldb->ldb_type != LDB_TYPE ||
		ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			    0, db_id.id_lgid.id_instance, 0, "LG_A_DBCONTEXT");
		return (LG_BADPARAM);
	    }

	    ldb->ldb_j_first_la = db->db_f_jnl_la;
	    ldb->ldb_j_last_la = db->db_l_jnl_la;

	    /*
	    ** Check if we must adjust the system archive window.
	    */
	    if (ldb->ldb_j_first_la.la_sequence != 0 &&
		((lfb->lfb_archive_window_start.la_sequence == 0) ||
		(LGA_LT(&ldb->ldb_j_first_la, &lfb->lfb_archive_window_start))))
	    {
		lfb->lfb_archive_window_start = ldb->ldb_j_first_la;
	    }
	    if (LGA_GT(&ldb->ldb_j_last_la, &lfb->lfb_archive_window_end))
		lfb->lfb_archive_window_end = ldb->ldb_j_last_la;

	    /*
	    ** Check if we must adjust the archive window prevcp value.
	    ** If the current prevcp value is not set or is AFTER the new
	    ** start window value, then we must set the prevcp to some CP
	    ** address previous to the new start value.
	    **
	    ** Since we don't keep track of the addresses of all CP's in
	    ** the system, we set the prevcp value to the log file BOF, which
	    ** we know is previous to the start window address.  This means
	    ** that the BOF will not be able to be moved forward at all until
	    ** at least one archive cycle is completed.
	    */
	    if ((lfb->lfb_archive_window_prevcp.la_sequence == 0) ||
		(LGA_LT(&lfb->lfb_archive_window_start,
					&lfb->lfb_archive_window_prevcp)))
	    {
		lfb->lfb_archive_window_prevcp = lfb->lfb_header.lgh_begin;
	    }
	}
	break;

    case LG_A_REASSOC:
    case LG_A_XA_DISASSOC:
	/*
	** Disown transaction and mark it REASSOC_WAIT.  The transaction will
	** remain in the system in an orphaned state until it can be adopted
	** by a new owner.
	**
	** LGalter(LG_A_REASSOC||LG_A_XA_DISASSOC, item)
	**
	**     inputs: item	- lx_id - lg transaction identifier
	**
	** This call is made when a Willing Commit transaction loses its
	** DBMS session owner.  The transaction is left in an orphaned
	** state until a new owner is created which can complete the 2pc
	** protocol.
	**
	** Or when an XA branch transaction does a xa_end(TMSUCCESS).
	**
	** That new owner (either a dbms thread or the rcp) will adopt the
	** transaction via an LG_A_OWNER alter call.
	**
	** While in an orphaned state, the transaction is linked to its own
	** orphan LPD and to the RCP's LPB.
	*/
	if (l_item < sizeof(LG_LXID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LXID),
			0, (flag == LG_A_REASSOC) 
				? "LG_A_REASSOC"
				: "LG_A_XA_DIASSOC");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, (flag == LG_A_REASSOC) 
				? "LG_A_REASSOC"
				: "LG_A_XA_DISASSOC");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, (flag == LG_A_REASSOC) 
				? "LG_A_REASSOC"
				: "LG_A_XA_DISASSOC");
	    return (LG_BADPARAM);
	}
	    
	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	    return(status);

	/*
	** If handle to a SHARED LXB, remove the handle from
	** the list, transform it into a regular non-HANDLE
	** LXB.
	** When no more handles exist, orphan the SHARED LXB.
	*/
	if ( flag == LG_A_REASSOC && lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

	    next_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_next);
	    prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_handles.lxbq_prev);
	    next_lxbq->lxbq_prev = lxb->lxb_handles.lxbq_prev;
	    prev_lxbq->lxbq_next = lxb->lxb_handles.lxbq_next;

	    lxb->lxb_status &= ~LXB_SHARED_HANDLE;
	    lxb->lxb_shared_lxb = 0;
	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	    /* If handles remain, return */
	    if ( --slxb->lxb_handle_count )
	    {
		(VOID)LG_unmutex(&slxb->lxb_mutex);
		break;
	    }

	    /* Removed SHARED LXB from LGD list */
	    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
	    next_lxbq = 
		(LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_next);
	    prev_lxbq = 
		(LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_prev);
	    next_lxbq->lxbq_prev = slxb->lxb_slxb.lxbq_prev;
	    prev_lxbq->lxbq_next = slxb->lxb_slxb.lxbq_next;
	    (VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

	    /* Switch to SHARED LXB for orphaning */
	    slxb->lxb_status &= ~LXB_SHARED;
	    lxb = slxb;
	}
	else if ( flag == LG_A_XA_DISASSOC && lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    LXBQ	*lxbq;
	    LXB		*handle_lxb;
	    /* 
	    ** An XA Branch can't be disassociated until all associations
	    ** are ended (i.e., those created via TMJOIN).
	    **
	    ** If there are any such matching Branch LXBs, just
	    ** discard this LXB's log/lock contexts. When the last
	    ** association terminates, then we'll orphan the Branch.
	    */
	    slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

	    lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);

	    while ( lxbq != (LXBQ*)&slxb->lxb_handles )
	    {
		handle_lxb = (LXB*)((char*)lxbq - CL_OFFSETOF(LXB,lxb_handles));
		lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);

		if ( lxb != handle_lxb && 
		    (handle_lxb->lxb_status & LXB_REASSOC_WAIT) == 0 &&
		    XA_XID_GTRID_EQL_MACRO(lxb->lxb_dis_tran_id, handle_lxb->lxb_dis_tran_id) )
		{
		    /*
		    ** There's at least one other association.
		    ** Mark our LXB with LXB_REASSOC_WAIT so that another
		    ** thread right behind us won't find it,
		    ** return LG_CONTINUE to signal the caller that it
		    ** should discard its log/lock contexts.
		    */
		    lxb->lxb_status |= LXB_REASSOC_WAIT;
		    (VOID)LG_unmutex(&slxb->lxb_mutex);
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    return(LG_CONTINUE);
		}
	    }
	    (VOID)LG_unmutex(&slxb->lxb_mutex);
	}

	/*
	** Set the transaction in reassoc state while it is orphaned.
	*/
	lxb->lxb_status |= LXB_REASSOC_WAIT;
	lxb->lxb_status &= ~LXB_RESUME;
	/*
	** Disown transaction.
	*/
	status = LG_orphan_xact(lxb);

	(VOID)LG_unmutex(&lxb->lxb_mutex);

	if (status != OK)
	    return (status);

	break;

    case LG_A_NODEID:
	/*
	** LG_A_NODEID is called by the DMFCSP process in a cluster installation
	** to notify the logging system of the local node's cluster node id.
	** This node ID number is used for things like managing the array of
	** node journal files in the config file, setting the open count bitmask
	** properly, etc. The logging system itself never uses this field,
	** it just stores it away and provides it on demand (through LGshow)
	** to callers who need to know the local node's node ID.
	*/
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4),
			0, "LG_A_NODEID");
	    return (LG_BADPARAM);
	}

	lgd->lgd_cnodeid = *(i4 *)item;

	/*
	** Since we "know" that this call is made only by the CSP process,
	** surreptitiously record its PID.
	*/
	lgd->lgd_csp_pid = LG_my_pid;

	break;

    case LG_A_BUFMGR_ID:
	/*
	** Item ptr should point to pair of items.
	** First is lg process id, second is buffer manager id.
	*/
	if (l_item < (sizeof(LG_LGID) + sizeof(i4)))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LGID) + sizeof(i4),
			0, "LG_A_BUFMGR_ID");
	    return (LG_BADPARAM);
	}

	lg_id.id_i4id = *(LG_LGID *)item;
	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_BUFMGR_ID");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_BUFMGR_ID");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	    return(status);

	lpb->lpb_status |= LPB_SHARED_BUFMGR;
	lpb->lpb_bufmgr_id = *(i4 *)((char *)item + sizeof(LG_LGID));

	(VOID)LG_unmutex(&lpb->lpb_mutex);

	break;

    case LG_A_RCPRECOVER:
        /*
        ** RCP is doing recovery
        **
        ** LGalter(LG_A_RCPRECOVER, item)
        **
        **     inputs: item     - (i4) TRUE/FALSE - whether to turn
        **                        RECOVERY status on or off.
        **
        **     Marks system status RCP_RECOVER to show that the RCP is doing
        **     recovery.  This allows servers to check to see if the
        **     installation is undergoing recovery.
        */
        if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4),
			0, "LG_A_RCPRECOVER");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

        if (*(i4 *)item)
            lgd->lgd_status |= LGD_RCP_RECOVER;
        else
            lgd->lgd_status &= ~LGD_RCP_RECOVER;
	
	(VOID)LG_unmutex(&lgd->lgd_mutex);

        break;

    case LG_A_CPNEEDED:
	/*
	** Signal the system to perform a Consistency Point.
	**
	** LGalter(LG_A_CPNEEDED, item)
	**
	**     inputs: item	- Not used.
	**
	**     Tells logging system to perform a Consistency Point.  This
	**     may be used to force servers to flush unwritten buffers to
	**     the database or in order to allow the archiver to process
	**     records in the new CP window.
	*/

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	/*
	** If system is already executing a Consistency Point, then set
	** the delay CP flag to signal a new CP when the current one is
	** complete.
	*/
	if (lgd->lgd_no_bcp)
	{
	    lgd->lgd_delay_bcp = 1;
	}
	else
	{
	    lgd->lgd_no_bcp = 1;
	    LG_signal_event(LGD_CPNEEDED, LGD_ECPDONE, TRUE);
	}

	(VOID)LG_unmutex(&lgd->lgd_mutex);

	break;

    case LG_A_DISABLE_DUAL_LOGGING:

	/*  
	** Disable dual logging. If a non-zero log id is given, then disable
	** dual logging for the LFB to which that log id is attached. If the
	** log id is 0, then disable dual logging for the local log.
	*/

	if (l_item != (2 * sizeof(i4)))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, (2 * sizeof(i4)),
			0, "LG_A_DISABLE_DUAL_LOGGING");
	    return (LG_BADPARAM);
	}
	lg_id.id_i4id = *(i4 *)item;

	if (lg_id.id_lgid.id_id == 0 && lg_id.id_lgid.id_instance == 0)
	{
	    lfb = &lgd->lgd_local_lfb;
	}
	else
	{
	    /*	Check the lg_id. */

	    if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_DISABLE_DUAL_LOGGING");
		return (LG_BADPARAM);
	    }

	    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	    if (lpb->lpb_type != LPB_TYPE ||
		lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_DISABLE_DUAL_LOGGING");
		return (LG_BADPARAM);
	    }
	    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);
	}

	if (*(i4 *)((char *)item + sizeof(i4)))
	    LG_disable_dual_logging(LGD_II_LOG_FILE, lfb);
	else
	    LG_disable_dual_logging(LGD_II_DUAL_LOG, lfb);

	break;

    case LG_A_ENABLE_DUAL_LOGGING:
    
	/*  
	** Enable dual logging. This call is made by the RCP at startup if the
	** user has requested dual logging. For now, we assume that the user is
	** rational in making this request and we set the logging system into
	** dual logging state. The RCP will subsequently open the log file as
	** master and we will verify the validity of the log file(s) at that
	** point, possibly degrading the system to single file logging.
	**
	** This LGalter() call is illegal if the log is already online.
	**
	** This call may also be made by RCPCONFIG if the logging system is
	** not online. RCPCONFIG uses this call to request opening of both
	** log files in order to enable/disable dual logging.
	*/

	if (l_item != sizeof(i4) || (lgd->lgd_status & LGD_ONLINE) != 0)
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4),
			0, "LG_A_ENABLE_DUAL_LOGGING");
	    return (LG_BADPARAM);
	}
	lg_id.id_i4id = *(i4 *)item;

	if (lg_id.id_lgid.id_id == 0 && lg_id.id_lgid.id_instance == 0)
	{
	    lfb = &lgd->lgd_local_lfb;
	}
	else
	{
	    if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			    0, "LG_A_ENABLE_DUAL_LOGGING");
		return (LG_BADPARAM);
	    }

	    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	    if (lpb->lpb_type != LPB_TYPE ||
		lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			    0, lg_id.id_lgid.id_instance, 0, "LG_A_ENABLE_DUAL_LOGGING");
		return (LG_BADPARAM);
	    }

	    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);
	}

	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	    return(status);

	lfb->lfb_status |= LFB_DUAL_LOGGING;
	lfb->lfb_active_log = (LGD_II_LOG_FILE | LGD_II_DUAL_LOG);

	(VOID)LG_unmutex(&lfb->lfb_mutex);

	break;

    case LG_A_DUAL_LOGGING:
    
	/*  
	** Clear disable/enable dual logging status.
	*/

	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4),
			0, "LG_A_DUAL_LOGGING");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

	lgd->lgd_status &= ~LGD_DISABLE_DUAL_LOGGING;

	(VOID)LG_unmutex(&lgd->lgd_mutex);

	break;

    case LG_A_FABRT_SID:
	/*
	** This thread is the force_abort thread for this server -- set the
	** "force abort session ID" for this server.
	*/
	if (l_item < sizeof(LG_LGID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LGID),
			0, "LG_A_FABRT_SID");
	    return (LG_BADPARAM);
	}

	lg_id.id_i4id = *(LG_LGID *)item;

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, "LG_A_FABRT_SID");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 0, "LG_A_FABRT_SID");
	    return (LG_BADPARAM);
	}

	CSget_cpid(&lpb->lpb_force_abort_cpid);

	break;

    case LG_A_GCMT_SID:
	/*
	** This thread is the Group Commit thread for this server -- set the
	** "Group Commit LXB" for this server.
	*/
	if (l_item < sizeof(LG_LXID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LXID),
			0, "LG_A_GCMT_SID");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_REASSOC");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_REASSOC");
	    return (LG_BADPARAM);
	}

	lxb->lxb_status |= LXB_WAIT;
	lxb->lxb_wait_reason = LG_WAIT_EVENT;

	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

	lpb->lpb_gcmt_lxb = LGK_OFFSET_FROM_PTR(lxb);

	break;

    case LG_A_SERIAL_IO:
	/*
	** User is requesting 'serial mode' I/O.
	**
	** LGalter(LG_A_SERIAL_IO, item)
	**
	**     inputs: item	- (i4) TRUE/FALSE - whether to turn
	**			  serial mode I/O on or off.
	**
	**	Serial Mode I/O means that we don't have I/Os outstanding to
	**	both log files at the same time. The goal of serial mode I/O
	**	is to minimize the risk of a power failure damaging both logs.
	**	The price is a considerable decrease in logfile throughput.
	*/
	if (l_item != sizeof(i4))
	    return (LG_BADPARAM);

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	lgd->lgd_serial_io = *(i4 *)item;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_A_TP_ON:
    case LG_A_TP_OFF:
	/*
	** Automated testpoint support. These two LGalter() codes are used to
	** turn on or off a testpoint by testpoint number. Other sections of
	** LG and DMF then query the testpoints and take special action if
	** a testpoint is on.
	**
	** For some testpoints, we take special action, typically to pick a
	** random block number in the log file at which to simulate an error.
	**
	** You can't set a write-failure testpoint until the master has come
	** up.
	*/
	if (l_item < sizeof(i4) || (*(i4 *)item) < 0 ||
	    (*(i4 *)item) >= LGD_MAX_NUM_TESTS)
	{
	    return (LG_BADPARAM);
	}
	if (flag == LG_A_TP_OFF)
	{
	    BTclear((i4)(*(i4 *)item),&lgd->lgd_test_bitvec[0]);
	}
	else
	{
	   switch ((i4)(*(i4 *)item))
	   {
		case LG_T_DATAFAIL_PRIMARY:
		case LG_T_PWRBTWN_PRIMARY:
		case LG_T_PARTIAL_AFTER_PRIMARY:
		case LG_T_PARTIAL_DURING_PRIMARY:
		case LG_T_PARTIAL_DURING_BOTH:
		case LG_T_DATAFAIL_DUAL:
		case LG_T_PWRBTWN_DUAL:
		case LG_T_PARTIAL_AFTER_DUAL:
		case LG_T_PARTIAL_DURING_DUAL:
		case LG_T_QIOFAIL_PRIMARY:
		case LG_T_QIOFAIL_DUAL:
		case LG_T_IOSBFAIL_PRIMARY:
		case LG_T_IOSBFAIL_DUAL:
		case LG_T_QIOFAIL_BOTH:
		case LG_T_IOSBFAIL_BOTH:
		    /* pick a random block in the log */
		    if (lgd->lgd_master_lpb == 0 ||
			lgd->lgd_local_lfb.lfb_header.lgh_count == 0)
			return (LG_BADPARAM);
		    lgd->lgd_test_badblk = (i4)
			(TMsecs() %
			 (lgd->lgd_local_lfb.lfb_header.lgh_count * lgd->lgd_local_lfb.lfb_header.lgh_size /
			    VMS_BLOCKSIZE)
			);
		    if (lgd->lgd_test_badblk <
				(lgd->lgd_local_lfb.lfb_header.lgh_size / VMS_BLOCKSIZE))
			lgd->lgd_test_badblk += 
				(lgd->lgd_local_lfb.lfb_header.lgh_size / VMS_BLOCKSIZE);
		    break;

		default:
		    /* no special testpoint processing needed */
		    break;
	    }
	   BTset(  (i4)(*(i4 *)item),&lgd->lgd_test_bitvec[0]);
	}

	break;

    case LG_A_GCMT_THRESHOLD:
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4),
			0, "LG_A_GCMT_THRESHOLD");
	    return (LG_BADPARAM);
	}


	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	lgd->lgd_gcmt_threshold = *(i4 *)item;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_A_GCMT_NUMTICKS:
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(i4), 0, "LG_A_GCMT_NUMTICKS");
	    return (LG_BADPARAM);
	}

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	lgd->lgd_gcmt_numticks = *(i4 *)item;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_A_LXB_OWNER:
	{
	    LG_TRANSACTION	*tr;
	   
	    /*
	    ** Change the ownership of the transaction to the RCP so the
	    ** the transaction can be recovered.
	    */

	    if (l_item < sizeof(LG_TRANSACTION))
	    {
		uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_TRANSACTION),
			    0, "LG_A_LXB_OWNER");
		return (LG_BADPARAM);
	    }

	    tr = (LG_TRANSACTION *)item;

	    /*	Check the lx_id. */

	    lx_id.id_i4id = tr->tr_id;
	    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_A_LXB_OWNER");
		return (LG_BADPARAM);
	    }

	    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	    if (lxb->lxb_type != LXB_TYPE ||
		lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			    0, lx_id.id_lgid.id_instance,
			    0, "LG_A_LXB_OWNER");
		return (LG_BADPARAM);
	    }

	    /*	Check the lg_id. */

	    lg_id.id_i4id = tr->tr_pr_id;
	    if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			    0, "LG_A_LXB_OWNER(2)");
		return (LG_BADPARAM);
	    }

	    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	    if (lpb->lpb_type != LPB_TYPE ||
		lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
			    0, lg_id.id_lgid.id_instance, 0, "LG_A_LXB_OWNER(2)");
		return (LG_BADPARAM);
	    }
	    
	    /*
	    ** Change the context of the transaction.
	    ** Note that the lxb_pid here is the actual system PID of the
	    ** process, not the internal LG id.
	    */

	    if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
		return(status);
	    /*
	    ** It's the RCP thread calling this function, so 
	    ** assign its cpid to this LXB.
	    */
	    CSget_cpid(&lxb->lxb_cpid);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	}
        break;

    case LG_A_UNRESERVE_SPACE:
	/*
	** Free up transaction reserved space that was allocated for log
	** forces that might be needed during rollback.
	**
	** LGalter(LG_A_UNRESERVE_SPACE, item)
	**
	**     inputs: item	- lx_id - lg transaction identifier
	**
	** This call is made when rolling back an operation which specified
	** the LG_RS_FORCE flag in its LGreserve call to preallocate a log
	** log buffer to ensure that it could do a log force during undo
	** recovery.
	**
	** During undo recovery this call is made to free up that log space
	** since it is no longer needed.
	**
	*/
	if (l_item < sizeof(LG_LXID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LXID),
			0, "LG_A_UNRESERVE_SPACE");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_UNRESERVE_SPACE");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_UNRESERVE_SPACE");
	    return (LG_BADPARAM);
	}

	/*
	** If handle to SHARED LXB, fetch the SHARED LXB;
	** that's where the reserved space is maintained.
	*/
	slxb = lxb;

	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    if ( lxb->lxb_shared_lxb == 0 )
	    {
		xid1.id_lgid = lxb->lxb_id;
		uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, xid1.id_i4id,
		    0, lxb->lxb_status,
		    0, "LG_A_UNRESERVE_SPACE");
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }
	    
	    slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    if ( slxb->lxb_type != LXB_TYPE ||
		(slxb->lxb_status & LXB_SHARED) == 0 )
	    {
		xid1.id_lgid = slxb->lxb_id;
		xid2.id_lgid = lxb->lxb_id;
		uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, slxb->lxb_type,
		    0, LXB_TYPE,
		    0, xid1.id_i4id,
		    0, slxb->lxb_status,
		    0, xid2.id_i4id,
		    0, "LG_A_UNRESERVE_SPACE");
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_BADPARAM);
	    }
	}

	/*
	** If the process is not using log space reservations, then bypass
	** the unreserve.
	*/
	if (slxb->lxb_status & LXB_NORESERVE)
	    break;

	lfb = (LFB *)LGK_PTR_FROM_OFFSET(slxb->lxb_lfb_offset);

	if (status = LG_mutex(SEM_EXCL, &slxb->lxb_mutex))
	    return(status);

	/* 
	** Update log file space reservation counters.
	*/
	reserved_space = LG_calc_reserved_space(slxb, 0, 0, LG_RS_FORCE);

	/*
	** Check for more log space being used than was actually reserved.
	** This can lead to logfull hanging problems if more space is needed
	** in restart recovery than was actually prereserved.
	**
	** We log an error in this case, but continue with the expectation
	** that everyting will probably be OK.  Even if we have not reserved
	** enough space, we are likely still not at a critical logfull
	** condition.
	**
	** (Note that returning an error in this condition is probably the
	** worst thing we could do since logspace reservation bugs will
	** likely only lead to problems if the transaction aborts).
	*/

	if (slxb->lxb_reserved_space >= reserved_space)
	{
	    slxb->lxb_reserved_space -= reserved_space;
	}
	else
	{
	    uleFormat(NULL, E_DMA490_LXB_RESERVED_SPACE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, slxb->lxb_lxh.lxh_tran_id.db_high_tran,
		    0, slxb->lxb_lxh.lxh_tran_id.db_low_tran,
		    sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space,
		    sizeof(slxb->lxb_reserved_space), &slxb->lxb_reserved_space,
		    0, slxb->lxb_sequence,
		    sizeof(reserved_space), &reserved_space);
	    slxb->lxb_reserved_space = 0;
	}
	(VOID)LG_unmutex(&slxb->lxb_mutex);

	/* lfb_mutex blocks concurrent update of lfb_reserved_space */
	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	    return(status);

	if (lfb->lfb_reserved_space >= reserved_space)
	{
	    lfb->lfb_reserved_space -= reserved_space;
	}
	else
	{
	    uleFormat(NULL, E_DMA491_LFB_RESERVED_SPACE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
		    sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space);
	    lfb->lfb_reserved_space = 0;
	}

	(VOID)LG_unmutex(&lfb->lfb_mutex);

	break;

    case LG_A_LFLSN:
	/*
	** Set logging system last forced LSN.
	**
	** LGalter(LG_A_LFLSN, item)
	**     inputs: item               	- logging system id
	**             item + sizeof(i4) 	- LSN of last log in logfile.
	**
	** This call is by the RCP to setup the LSN of the last (forced) 
	** record in the log file.  
	**
	*/

	if (l_item != sizeof(i4) + sizeof(LG_LSN))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		0, l_item, 0, sizeof(i4) + sizeof(LG_LSN),
		0, "LG_A_LFLSN");
	    return (LG_BADPARAM);
	}

	lg_id.id_i4id = *(i4 *)item;

	if (lg_id.id_lgid.id_id == 0 && lg_id.id_lgid.id_instance == 0)
	{
	    lfb = &lgd->lgd_local_lfb;
	}
	else
	{
	    /*  Check the lg_id. */

	    if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
		    0, "LG_A_LFLSN");
		return (LG_BADPARAM);
	    }

	    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	    if (lpb->lpb_type != LPB_TYPE ||
		lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
		    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
		    0, lg_id.id_lgid.id_instance, 0, "LG_A_LFLSN");
		return (LG_BADPARAM);
	    }

	    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);
	}


	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex))
	    return(status);
	lfb->lfb_forced_lsn = *(LG_LSN *)((char *)item + sizeof(i4));
	lfb->lfb_header.lgh_last_lsn = 
	    *(LG_LSN *)((char *)item + sizeof(i4));
	(VOID)LG_unmutex(&lfb->lfb_wq_mutex);

	break;

    case LG_A_LBACKUP:
    
	if (l_item < sizeof(LG_DBID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DBID), 0, "LG_A_LBACKUP");
	    return (LG_BADPARAM);
	}

	db_id.id_i4id = *(LG_DBID *)item;

	/*  Check the db_id. */
	if (db_id.id_lgid.id_id == 0 || (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, "LG_A_LBACKUP");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

	if (lpd->lpd_type != LPD_TYPE ||
	    lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_LBACKUP");
	    return (LG_BADPARAM);
	}

	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	    return(status);
        
	ldb->ldb_status &= ~LDB_CKPLK_STALL;
	LG_resume(&lgd->lgd_wait_stall, NULL);
	(VOID)LG_unmutex(&ldb->ldb_mutex);
	break;

    case LG_A_BMINI:

	if (l_item < sizeof(LG_LSN))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LSN),
			0, "LG_A_BMINI");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_BMINI");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_BMINI");
	    return (LG_BADPARAM);
	}

	/* Get to the LDB belonging to caller's transaction */
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	/*
	** We'll return the txn's last LSN here, 
	** marking the start of the mini-transaction.
	*/
	lsn = (LG_LSN*)item;

	(VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);

	/*
	** This is an issue only with Shared log
	** contexts.
	**
	** Mini-transactions must be single-threaded
	** to prevent non-mini-related log records
	** from being interspersed with mini-related
	** log records; if a rollback occurs, the
	** non-mini log records will be skipped if
	** recovery "jumps" from the EMINI CLR
	** to the BMINI LSN.
	*/
	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

	    /*
	    ** If another handle is in a MINI, then
	    ** this handle must wait. 
	    **
	    ** If it's us, allow the nesting.
	    */
	    if ( slxb->lxb_status & LXB_IN_MINI &&
		 (lxb->lxb_status & LXB_IN_MINI) == 0 )
	    {
		lxb->lxb_wait_reason = LG_WAIT_MINI;
		LG_suspend(lxb, &lgd->lgd_wait_mini, &status);
		(VOID)LG_unmutex(&slxb->lxb_mutex);
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		return(LG_MUST_SLEEP);
	    }
	    
	    /* Note in the SHARED list that MINI is in progress */
	    slxb->lxb_status |= LXB_IN_MINI;

	    /* For shared txns, the last LSN is in the sLXB */
	    *lsn = slxb->lxb_last_lsn;
	    (VOID)LG_unmutex(&slxb->lxb_mutex);
	}
	else
	    *lsn = lxb->lxb_last_lsn;

	/* This LXB is in a (nested) mini-transaction */
	lxb->lxb_status |= LXB_IN_MINI;
	lxb->lxb_in_mini++;

	(VOID)LG_unmutex(&lxb->lxb_mutex);

	break;

    case LG_A_EMINI:

	if (l_item < sizeof(LG_LXID))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LXID),
			0, "LG_A_EMINI");
	    return (LG_BADPARAM);
	}

	lx_id.id_i4id = *(LG_LXID *)item;

	/*	Check the lx_id. */

	if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_A_EMINI");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	if (lxb->lxb_type != LXB_TYPE ||
	    lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			0, lx_id.id_lgid.id_instance,
			0, "LG_A_EMINI");
	    return (LG_BADPARAM);
	}

	/* Get to the LDB belonging to caller's transaction */
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	(VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);

	/*
	** Keep LXB in IN_MINI state until all
	** nested minis are complete.
	*/
	if ( lxb->lxb_status & LXB_IN_MINI &&
	     --lxb->lxb_in_mini == 0 )
	{
	    lxb->lxb_status &= ~LXB_IN_MINI;

	    /*
	    ** If shared log context, there may be others
	    ** waiting for this MINI to end.
	    */
	    if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	    {
		slxb = (LXB*)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

		(VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

		slxb->lxb_status &= ~LXB_IN_MINI;

		/* Wake up waiting handles, if any */
		if ( lgd->lgd_wait_mini.lwq_count )
		    LG_resume(&lgd->lgd_wait_mini, NULL);
		(VOID)LG_unmutex(&slxb->lxb_mutex);
	    }
	}

	(VOID)LG_unmutex(&lxb->lxb_mutex);

	break;

    case LG_A_OPTIMWRITE:
    case LG_A_OPTIMWRITE_TRACE:

	/*
	** Toggles whether writes of log pages are optimized
	** using superblocks ( > 1 contiguous page at a time )
	** and if those writes are to be traced.
	**
	** DM1442 turns optimization on or off,
	** DM1443 toggles tracing.
	**
	** If turning optimization on and it's already
	** on, just reset the stats.
	**
	** Ignore completely if multi-partition log,
	** optimized writes unavailable.
	*/
	if ( lgd->lgd_local_lfb.lfb_header.lgh_partitions == 1 )
	{
	    if ( status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex) )
		return(status);

	    if ( flag == LG_A_OPTIMWRITE )
	    {
		if ( l_item )
		{
		    if ( lgd->lgd_state_flags & LGD_OPTIMWRITE )
		    {
			/* Already on, reset stats */
			lgd->lgd_stat.optimwrites = 0;
			lgd->lgd_stat.optimpages = 0;
		    }
		    else
			lgd->lgd_state_flags |= LGD_OPTIMWRITE;
		}
		else
		    lgd->lgd_state_flags &= ~(LGD_OPTIMWRITE | LGD_OPTIMWRITE_TRACE);
	    }
	    else if ( l_item )
		lgd->lgd_state_flags |= LGD_OPTIMWRITE_TRACE;
	    else
		lgd->lgd_state_flags &= ~LGD_OPTIMWRITE_TRACE;

	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	}

	break;

    case LG_A_LOCKID:
	{
	    LG_TRANSACTION	*tr;

	    /*
	    ** When beginning a transaction, the log context is established
	    ** first, then the locking context.
	    **
	    ** Some XA operations (XA_PREPARE, XA_COMMIT, XA_ROLLBACK) 
	    ** need to acquire both contexts; this alter function 
	    ** stuffs the lock context identifier (lock id) in the LXB
	    ** for later retrieval by LGshow.
	    **
	    ** We make no judgements about its value, just doing what 
	    ** we're told.
	    **
	    ** LGalter(LG_A_LOCKID, item)
	    **
	    **	inputs: item	- LG_TRANSACTION structure:
	    **				tr_id		- log context
	    **				tr_lock_id	- lock context to stow
	    */
	    if (l_item < sizeof(LG_TRANSACTION))
	    {
		uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_ID),
			    0, "LG_A_LOCKID");
		return (LG_BADPARAM);
	    }

	    tr = (LG_TRANSACTION*)item;

	    /*	Check the lx_id. */
	    lx_id.id_i4id = tr->tr_id;

	    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_A_LOCKID");
		return (LG_BADPARAM);
	    }

	    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	    if (lxb->lxb_type != LXB_TYPE ||
		lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			    0, lx_id.id_lgid.id_instance,
			    0, "LG_A_LOCKID");
		return (LG_BADPARAM);
	    }

	    /* Set the lock id */
	    lxb->lxb_lock_id = tr->tr_lock_id;
	}
	break;

    case LG_A_TXN_STATE:
	{
	    LG_TRANSACTION	*tr;

	    /*
	    ** When a thread's association to an XA branch is normally
	    ** ending (xa_end(TMSUCCESS)) there are things about the
	    ** DMF transaction that need to be remembered DMF-wise
	    ** for later xa_prepare, xa_commit, xa_rollback calls.
	    **
	    ** This alter gives DMF the opportunity to conveniently 
	    ** save that state information in the log context, and
	    ** retrieve it later.
	    **
	    ** LGalter(LG_A_TXN_STATE, item)
	    **
	    **	inputs: item	- LG_TRANSACTION structure:
	    **				tr_id		- log context
	    **				tr_txn_state	- state information
	    */
	    if (l_item < sizeof(LG_TRANSACTION))
	    {
		uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_ID),
			    0, "LG_A_TXN_STATE");
		return (LG_BADPARAM);
	    }

	    tr = (LG_TRANSACTION*)item;

	    /*	Check the lx_id. */
	    lx_id.id_i4id = tr->tr_id;

	    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_A_TXN_STATE");
		return (LG_BADPARAM);
	    }

	    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	    if (lxb->lxb_type != LXB_TYPE ||
		lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			    0, lx_id.id_lgid.id_instance,
			    0, "LG_A_TXN_STATE");
		return (LG_BADPARAM);
	    }

	    /* Save the state information for DMF */
	    lxb->lxb_txn_state = tr->tr_txn_state;
	}
	break;

    case LG_A_FILE_DELETE:
	{
	    LG_FDEL	*fdel;

	    /*
	    ** LGalter(LG_A_FILE_DELETE) is called by dmx_commit
	    ** when XA_ENDing a transaction branch, once for 
	    ** each XCCB_F_DELETE on the XCB's XCCB queue.
	    **
	    ** The file deletes must be delayed until the Global
	    ** Transaction commits so that an ensuing rollback of the 
	    ** GT will succeed.
	    **
	    ** The input LG_FDEL structure names the path+file
	    ** to be deleted.
	    */

	    if (l_item < sizeof(LG_FDEL))
	    {
		uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_ID),
			    0, "LG_A_FILE_DELETE");
		return (LG_BADPARAM);
	    }

	    fdel = (LG_FDEL*)item;

	    /*	Check the lx_id. */

	    lx_id.id_i4id = fdel->fdel_id;
	    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_A_FILE_DELETE");
		return (LG_BADPARAM);
	    }

	    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	    if (lxb->lxb_type != LXB_TYPE ||
		lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
			    0, lx_id.id_lgid.id_instance,
			    0, "LG_A_FILE_DELETE");
		return (LG_BADPARAM);
	    }

	    (VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);

	    /* Get to the shared Global Txn LXB */
	    if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	    {
		if ( lxb->lxb_shared_lxb == 0 )
		{
		    xid1.id_lgid = lxb->lxb_id;
		    uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, xid1.id_i4id,
			0, lxb->lxb_status,
			0, "LG_A_FILE_DELETE");
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    return(LG_BADPARAM);
		}
		
		slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

		if ( slxb->lxb_type != LXB_TYPE ||
		    (slxb->lxb_status & LXB_SHARED) == 0 ||
		    (slxb->lxb_status & LXB_ORPHAN) == 0 )
		{
		    xid1.id_lgid = slxb->lxb_id;
		    xid2.id_lgid = lxb->lxb_id;
		    uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, slxb->lxb_type,
			0, LXB_TYPE,
			0, xid1.id_i4id,
			0, slxb->lxb_status,
			0, xid2.id_i4id,
			0, "LG_A_FILE_DELETE");
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    return(LG_BADPARAM);
		}

		(VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

		/* Allocate a LFD to contain the file info */
		if ( (lfd = (LFD*)LG_allocate_cb(LFD_TYPE)) == 0 ) 
		{
		    (VOID)LG_unmutex(&slxb->lxb_mutex);
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    return(LG_EXCEED_LIMIT);
		}

		STRUCT_ASSIGN_MACRO(*fdel, lfd->lfd_fdel);

		/* Link the LFD to the GT's LXB */
		lfd->lfd_next = slxb->lxb_lfd_next;
		slxb->lxb_lfd_next = LGK_OFFSET_FROM_PTR(lfd);

		(VOID)LG_unmutex(&slxb->lxb_mutex);

	    }
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	}
	break;

    case LG_A_RCP_IGNORE_DB:
    case LG_A_RCP_IGNORE_TABLE:
    case LG_A_RCP_IGNORE_LSN:
    case LG_A_RCP_STOP_LSN:
	/*
	** Signal the system to resume rcp recovery.
	**
	** LGalter(LG_A_RCP_IGNORE_DB, item)
	** LGalter(LG_A_RCP_IGNORE_TABLE, item)
	** LGalter(LG_A_RCP_IGNORE_LSN, item)
	** LGalter(LG_A_RCP_STOP_LSN, item)
	**
	*/
	err_code = 0;
	if (l_item != sizeof(LG_RECOVER))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, l_item, 0, sizeof(LG_RECOVER), 0, "LG_A_RCP_IGNORE");
	    return(LG_BADPARAM);
	}
	recov_struct = (LG_RECOVER *)item;
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);

#ifdef xDEBUG
TRdisplay("Signal RCP recovery event %d:\n-rcp_continue_ignore_lsn=%d,%d\n-rcp_continue_ignore_table=%~t\n-rcp_continue_ignore_db=%~t -rcp_stop_lsn=%d,%d\n",
	    flag,
	    lgd->lgd_recover.lg_lsn.lsn_high,
	    lgd->lgd_recover.lg_lsn.lsn_low,
	    DB_TAB_MAXNAME, &lgd->lgd_recover.lg_tabname[0],
	    DB_DB_MAXNAME, &lgd->lgd_recover.lg_dbname[0],
	    lgd->lgd_recover.lg_lsn.lsn_high,
	    lgd->lgd_recover.lg_lsn.lsn_low);
#endif

	if (lgd->lgd_status & LGD_ONLINE)
	{
	    MEfill(sizeof(LG_RECOVER), 0, recov_struct);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    return (E_DB_ERROR);
	}
	else if ( (flag == LG_A_RCP_IGNORE_LSN || flag == LG_A_RCP_STOP_LSN)
		&& !(LSN_EQ(&recov_struct->lg_lsn, &lgd->lgd_recover.lg_lsn)))
	{
	    STRUCT_ASSIGN_MACRO(lgd->lgd_recover, *recov_struct);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    return(LG_BADPARAM);
	}
	else if (flag == LG_A_RCP_IGNORE_TABLE &&
		MEcmp(&recov_struct->lg_tabname[0],
		&lgd->lgd_recover.lg_tabname[0], DB_TAB_MAXNAME))
	{
	    STRUCT_ASSIGN_MACRO(lgd->lgd_recover, *recov_struct);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    return(LG_BADPARAM);
	}
	else if (flag == LG_A_RCP_IGNORE_DB &&
		MEcmp(&recov_struct->lg_dbname[0],
		&lgd->lgd_recover.lg_dbname[0], DB_DB_MAXNAME)) 
	{
	    STRUCT_ASSIGN_MACRO(lgd->lgd_recover, *recov_struct);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    return(LG_BADPARAM);
	}

	/* valid input, wakeup the rcp */
	lgd->lgd_recover.lg_flag = flag;

	(VOID)LG_unmutex(&lgd->lgd_mutex);
	LG_signal_event(LG_E_RECOVER, 0, FALSE);
	break;

    case LG_A_INIT_RECOVER:
	if (l_item != sizeof(LG_RECOVER))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, l_item, 0, sizeof(LG_RECOVER), 0, "LG_A_INIT_RECOVER");
	    return(LG_BADPARAM);
	}
	recov_struct = (LG_RECOVER *)item;
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	STRUCT_ASSIGN_MACRO(*recov_struct, lgd->lgd_recover);
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	
	break;

    case LG_A_JFIB:
    case LG_A_JFIB_DB:

	/*
	** LG_A_JFIB		write ldb_jfib journal information
	**			from config files (dm2d_open_db()).
	** LG_A_JFIB_DB		same as LG_A_JFIB, but jfib_db_id is
	**			a LDB id, not a LPD id (dm0c_close()).
	**
	** Called whenever a MVCC database is opened (dm2d)
	** or whenever the config journal information for a MVCC
	** database is changed (e.g., archiver EOC).
	** 
	** 
	** "item" is a pointer to a LG_JFIB with the following elements
	** filled in from the config file journal entry:
	**
	**	jfib_bksz	from	jnl_bksz
	**	jfib_maxcnt	from	jnl_maxcnt
	**	jfib_ckpseq	from	jnl_ckp_seq
	**	jfib_jfa.filseq	from	jnl_fil_seq
	**	jfib_jfa.block	from	jnl_blk_seq
	*/
	if (l_item < sizeof(LG_JFIB))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_JFIB), 
			0, (flag == LG_A_JFIB) ? "LG_A_JFIB"
					       : "LG_A_JFIB_DB");
	    return (LG_BADPARAM);
	}

	cnf_jfib = (LG_JFIB*)item;
	db_id.id_i4id = *(i4*)&cnf_jfib->jfib_db_id;

        /*  Check the db_id. */
	if ( flag == LG_A_JFIB_DB )
	{
	    if (db_id.id_lgid.id_id == 0 
		|| (i4) db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, db_id.id_i4id, 0, lgd->lgd_ldbb_count,
			    0, "LG_A_JFIB_DB");
		return (LG_BADPARAM);
	    }

	    ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);

	    if (ldb->ldb_type != LDB_TYPE ||
		ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			    0, db_id.id_lgid.id_instance, 0, "LG_A_JFIB_DB");
		return (LG_BADPARAM);
	    }
	}
	else
	{
	    if (db_id.id_lgid.id_id == 0 
		|| (i4)db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	    {
		uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, db_id.id_i4id, 0, lgd->lgd_lpdb_count,
			    0, "LG_A_JFIB");
		return (LG_BADPARAM);
	    }

	    lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

	    if (lpd->lpd_type != LPD_TYPE ||
		lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			    0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			    0, db_id.id_lgid.id_instance,
			    0, "LG_A_JFIB");
		return (LG_BADPARAM);
	    }

	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	}

	/* Do nothing if LDB not opened to use journaled MVCC protocols */
	if ( ldb->ldb_status & LDB_MVCC && ldb->ldb_status & LDB_JOURNAL )
	{
	    /* Block log writers while working on the JFIB */
	    if ( status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex) )
	        return(status);

	    jfib = &ldb->ldb_jfib;

	    /*
	    ** If the config journal file is the same as that known
	    ** to the JFIB there's nothing to do except remember the
	    ** config's last-written block.
	    **
	    ** The filseq's will be equal when:
	    **
	    **    o Start of Archive cycle and the previous filseq
	    **	    is full and the Archiver is creating the next
	    **	    filseq, block 0 and it's "caught up" to the JFIB
	    **	    filseq.
	    **
	    **	  o End of Archiver cycle, and it's updating the
	    **	    config file with the last block written.
	    */
	    if ( cnf_jfib->jfib_jfa.jfa_filseq == jfib->jfib_jfa.jfa_filseq )
	    {
		/*
		** The Archiver will always start archiving on the
		** next block. If that's the same as the JFIB block,
		** abandon the JFIB block and move up to the next.
		*/
		if ( cnf_jfib->jfib_jfa.jfa_block >= jfib->jfib_jfa.jfa_block )
		{
		    TRdisplay("%@ LGalter:%d cnf: %d,%d %d max %d"
			" jfib: %d,%d\n",
			    __LINE__,
			    cnf_jfib->jfib_jfa.jfa_filseq, cnf_jfib->jfib_jfa.jfa_block,
			    cnf_jfib->jfib_cycles,
			    cnf_jfib->jfib_maxcnt,
			    jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block);
		    jfib->jfib_jfa.jfa_block = cnf_jfib->jfib_jfa.jfa_block+1;
		    jfib->jfib_next_byte = sizeof(DM0J_HEADER);
		    jfib->jfib_bytes_left = jfib->jfib_bksz - sizeof(DM0J_HEADER);
		}

		if ( cnf_jfib->jfib_jfa.jfa_block > jfib->jfib_jfa.jfa_block &&
		    !(ldb->ldb_status & LDB_PURGE) )
		{
		    TRdisplay("%@ LGalter:%d YIKES cnf: %d,%d %d max %d"
			" jfib: %d,%d\n",
			    __LINE__,
			    cnf_jfib->jfib_jfa.jfa_filseq, cnf_jfib->jfib_jfa.jfa_block,
			    cnf_jfib->jfib_cycles,
			    cnf_jfib->jfib_maxcnt,
			    jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block);
		    jfib->jfib_jfa.jfa_block = cnf_jfib->jfib_jfa.jfa_block;
		    jfib->jfib_next_byte = sizeof(DM0J_HEADER);
		    jfib->jfib_bytes_left = jfib->jfib_bksz - sizeof(DM0J_HEADER);
		}

		/*
		** If the config block is less than maxcnt, then
		** the same filseq will be used for the next cycle.
		**
		** Because the next cycle will start on jnl_blk_seq+1
		** and that block has already been "filled"
		** by LGwrite, we'll loose some accuracy
		** in our predictive journal file address algorithm.
		**
		** LGwrite simulates packing log records into
		** consecutive blocks, but the archiver may well
		** leave partially full blocks (it'll do its last
		** write, say, in block 10, then on the next
		** cycle it'll start with block 11). This throws off
		** the exactitude of LGwrite's file+block guess
		** a bit, but there's nothing to be done about it.
		** Journal undo knows how to deal with it.
		*/

		if ( cnf_jfib->jfib_jfa.jfa_block &&
		     cnf_jfib->jfib_jfa.jfa_block != jfib->jfib_cnf_jfa.jfa_block )
		    TRdisplay("%@ LGalter:%d cnf: %d,%d %d max %d"
			" jfib: %d,%d\n",
			    __LINE__,
			    cnf_jfib->jfib_jfa.jfa_filseq, cnf_jfib->jfib_jfa.jfa_block,
			    cnf_jfib->jfib_cycles,
			    cnf_jfib->jfib_maxcnt,
			    jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block);

		if ( cnf_jfib->jfib_jfa.jfa_block + ++jfib->jfib_cycles >= cnf_jfib->jfib_maxcnt )
		{
		    TRdisplay("%@ LGalter:%d cnf: %d,%d %d max %d"
			" jfib: %d,%d\n",
			    __LINE__,
			    cnf_jfib->jfib_jfa.jfa_filseq, cnf_jfib->jfib_jfa.jfa_block,
			    jfib->jfib_cycles,
			    cnf_jfib->jfib_maxcnt,
			    jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block);
		    /* Force switch to next file, below */
		    jfib->jfib_jfa.jfa_filseq = 0;
		}
	    }

	    /*
	    ** Not the same filseq as the JFIB.
	    **
	    ** Only overwrite JFIB if new config journal information is
	    ** radically different from what we know or if
	    ** the new journal file sequence is greater than
	    ** what we already know.
	    **
	    ** Exclude check of maxcnt; if it's been changed,
	    ** it'll become effective on the start of the next
	    ** cycle.
	    */
	    if ( cnf_jfib->jfib_bksz != jfib->jfib_bksz ||
		 cnf_jfib->jfib_ckpseq != jfib->jfib_ckpseq ||
		 cnf_jfib->jfib_jfa.jfa_filseq > jfib->jfib_jfa.jfa_filseq )
	    {
		if ( jfib->jfib_jfa.jfa_filseq != 0 )
		    TRdisplay("%@ LGalter:%d cnf: %d,%d %d bksz %d max %d"
			    " was jfib: %d,%d bksz %d max %d\n",
				__LINE__,
				cnf_jfib->jfib_jfa.jfa_filseq, cnf_jfib->jfib_jfa.jfa_block,
				jfib->jfib_cycles,
				cnf_jfib->jfib_bksz,
				cnf_jfib->jfib_maxcnt,
				jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block,
				jfib->jfib_bksz,
				jfib->jfib_maxcnt);

		/* Refresh JFIB stuff from config */
		jfib->jfib_bksz = cnf_jfib->jfib_bksz;
		jfib->jfib_maxcnt = cnf_jfib->jfib_maxcnt;
		jfib->jfib_ckpseq = cnf_jfib->jfib_ckpseq;

		/* If the config file is full, switch JFIB to next */
		if ( cnf_jfib->jfib_jfa.jfa_block >= cnf_jfib->jfib_maxcnt )
		{
		    jfib->jfib_jfa.jfa_filseq = cnf_jfib->jfib_jfa.jfa_filseq+1;
		    jfib->jfib_jfa.jfa_block = 1;
		    jfib->jfib_cycles = 0;
		}
		else
		{
		    /* Set JFIB filseq,block from config */
		    jfib->jfib_jfa.jfa_filseq = cnf_jfib->jfib_jfa.jfa_filseq;
		    /* First journal record goes on next block (never block zero) */
		    jfib->jfib_jfa.jfa_block = cnf_jfib->jfib_jfa.jfa_block+1;
		}

		/* Init empty jfib_jfa.jfib_block: */
		jfib->jfib_next_byte = sizeof(DM0J_HEADER);
		jfib->jfib_bytes_left = jfib->jfib_bksz - sizeof(DM0J_HEADER);

		/* Supply the node id */
		jfib->jfib_nodeid = lgd->lgd_cnodeid;
	    }
	    else
	    {
		if ( cnf_jfib->jfib_jfa.jfa_filseq != jfib->jfib_jfa.jfa_filseq &&
		     cnf_jfib->jfib_jfa.jfa_block &&
		     cnf_jfib->jfib_jfa.jfa_block != jfib->jfib_jfa.jfa_block )
		    TRdisplay("%@ LGalter:%d cnf: %d,%d %d max %d jfib: %d,%d\n",
			    __LINE__,
			    cnf_jfib->jfib_jfa.jfa_filseq, cnf_jfib->jfib_jfa.jfa_block,
			    jfib->jfib_cycles,
			    cnf_jfib->jfib_maxcnt,
			    jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block);
	    }
	    
	    /* Remember the current config filseq, block in the JFIB */
	    jfib->jfib_cnf_jfa = cnf_jfib->jfib_jfa;

	    /* Set JFIB maxcnt to config's value (it may have been altered) */
	    jfib->jfib_maxcnt = cnf_jfib->jfib_maxcnt;

	    /* Turn JFIB loose to LGwrite */
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	}

	break;

    case LG_A_ARCHIVE_JFIB:

	/*
	** Called by the archiver when it has found a MVCC database
	** that needs to be JSWITCHed or has log records that need to
	** be archived.
	**
	** "item" is a pointer to the archiver's LG_DATABASE 
	** structure just retrieved by it from LGshow.
	**
	** If the JFIB appears to be "full" or its being forcibly
	** JSWITCHed, then advance the JFIB to the next journal
	** file sequence.
	*/

	if (l_item < sizeof(LG_DATABASE))
	{
	    uleFormat(NULL, E_DMA472_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DATABASE), 
			0, "LG_A_ARCHIVE_JFIB");
	    return (LG_BADPARAM);
	}

	db = (LG_DATABASE*)item;

	db_id.id_i4id = *(LG_DBID*)&db->db_id;

	if (db_id.id_lgid.id_id == 0 
		|| (i4) db_id.id_lgid.id_id > lgd->lgd_ldbb_count)
	{
	    uleFormat(NULL, E_DMA474_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_i4id, 0, lgd->lgd_ldbb_count,
			0, "LG_A_ARCHIVE_JFIB");
	    return (LG_BADPARAM);
	}

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);

	if (ldb->ldb_type != LDB_TYPE ||
	    ldb->ldb_id.id_instance != db_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA475_LGALTER_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, ldb->ldb_type, 0, ldb->ldb_id.id_instance,
			0, db_id.id_lgid.id_instance, 0, "LG_A_ARCHIVE_JFIB");
	    return (LG_BADPARAM);
	}

	/* Double-check journaled MVCC db, ignore if archiver erred */
	if ( ldb->ldb_status & LDB_MVCC && ldb->ldb_status & LDB_JOURNAL )
	{
	    /* The ldb_mutex keeps LGwrite at bay */
	    if ( LG_mutex(SEM_EXCL, &ldb->ldb_mutex) )
		return(status);

	    jfib = &ldb->ldb_jfib;
	    
	    /*
	    ** SOC:
	    **
	    ** Check for filseq switch if archiver and JFIB are the same filseq
	    ** or the archiver is about to switch to the JFIB filseq because
	    ** its current file is full.
	    **
	    ** If the archiver is about to switch to a JFIB filseq that's
	    ** already full, then advance JFIB filseq.
	    */
	    if ( db->db_jfib.jfib_jfa.jfa_filseq == jfib->jfib_cnf_jfa.jfa_filseq ||
	         (db->db_jfib.jfib_cnf_jfa.jfa_filseq+1 == jfib->jfib_jfa.jfa_filseq &&
		  db->db_jfib.jfib_jfa.jfa_block >= jfib->jfib_maxcnt) )
	    {
		/*
		** If journal file is being switched or JFIB is "full"
		** or will become full at the end of this archive cycle,
		** then advance the JFIB to the next filseq, block 1.
		*/
		if ( ldb->ldb_status & LDB_JSWITCH ||
		     db->db_jfib.jfib_jfa.jfa_block + db->db_jfib.jfib_cycles 
		     	>= jfib->jfib_maxcnt )
		{
		    jfib->jfib_next_byte = sizeof(DM0J_HEADER);
		    jfib->jfib_bytes_left = jfib->jfib_bksz - sizeof(DM0J_HEADER);
		    jfib->jfib_jfa.jfa_filseq++;
		    jfib->jfib_jfa.jfa_block = 1;
		    jfib->jfib_cycles = 0;
		}
	    }

	    TRdisplay("%@ LGalter:%d SOC: (%d,%d)..(%d,%d) %d jfib: %d,%d\n",
			    __LINE__,
			db->db_jfib.jfib_cnf_jfa.jfa_filseq, db->db_jfib.jfib_cnf_jfa.jfa_block,
			db->db_jfib.jfib_jfa.jfa_filseq, db->db_jfib.jfib_jfa.jfa_block,
			db->db_jfib.jfib_cycles,
			jfib->jfib_jfa.jfa_filseq, jfib->jfib_jfa.jfa_block);

	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	}
	break;

    case LG_A_RBBLOCKS:

	/*
	** Set the number of blocks to be allocated by dm0l_allocate 
	** for read backward (like dmxe, any not SVCB_SINGLEUSER or
	** the recovery server).
	**
	** This should be a relatively small number, and defaults to 1.
	*/
	value = *(i4 *)item;

	if ( value <= 0 || value > lgd->lgd_local_lfb.lfb_header.lgh_count )
	    return(LG_BADPARAM);

	lgd->lgd_rbblocks = value;
	break;

    case LG_A_RFBLOCKS:

	/*
	** Set the number of blocks to be allocated by dm0l_allocate 
	** for read forward (archiver, jsp things - any SVCB_SINGLEUSER,
	** recovery server).
	**
	** This should be a larger number than RBBLOCKS, but defaults to 1.
	*/
	value = *(i4 *)item;

	if ( value <= 0 || value > lgd->lgd_local_lfb.lfb_header.lgh_count )
	    return(LG_BADPARAM);

	lgd->lgd_rfblocks = value;
	break;


    default:
	return (LG_BADPARAM);
    }

    return (OK);
}

/*
** Name: LG_allocate_buffers		-- allocate the LBB's for an LFB.
**
** History:
**	26-jul-1993 (bryanp)
**	    Cease immediately the evil overwriting of LGK_EXT memory headers.
**	    We apologize for our previous transgressions...
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changing %x to %p for pointer values.
**	22-Jan-1996 (jenjo02)
**	    Caller must hold lfb_mutex.
**	    Initialize LBB's LWQ  mutex.
**	24-Aug-1999 (jenjo02)
**	    When initializing, all but the first log buffer go on the
**	    wait queue, the first becomes the "current" buffer.
**	20-Mar-2000 (jenjo02)
**	    Allocate space for all LBBs and buffers in one chunk, divide
**	    up into LBBs followed by log-page-size-aligned buffers.
**	08-Apr-2002 (devjo01)
**	    Add 'lfb_first_lbb' to remember beginning of contiguous
**	    allocation for LBB and associated log buffers.
**	21-Mar-2006 (jenjo02)
**	    Order free queue low to high, init (new) lfb_fq_leof
**	    to last buffer.
**	20-Sep-2006 (kschendel)
**	    Move buffer-wait LWQ into lfb, init it here.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Init lbb_id, number buffers 1-n
*/
STATUS
LG_allocate_buffers( LGD *lgd, LFB *lfb, i4 logpg_size, 
			i4 num_buffers)
{
    i4		i;
    LGK_EXT	*ext;
    LBB		*lbb;
    LBH		*lbh;
    LBB		*prev_lbb;
    PTR		aligned_buffer;
    i4		err_code;
    STATUS	status;
    char	sem_name[CS_SEM_NAME_LEN];

    /* Initialize the buffer-wait LWQ in the LFB first.  This would
    ** be a little slicker in an "init LFB" routine, but this comes
    ** pretty close -- an LFB isn't much good without buffers, anyway.
    */
    lfb->lfb_wait_buffer.lwq_lxbq.lxbq_next =
	lfb->lfb_wait_buffer.lwq_lxbq.lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lfb->lfb_wait_buffer.lwq_lxbq);
    lfb->lfb_wait_buffer.lwq_count = 0;
    /* We don't use the lfb_wait_buffer lwq_mutex, we use lfb_fq_mutex
    ** instead.  Don't bother initing this lwq mutex.
    */

    ext = (LGK_EXT *)lgkm_allocate_ext((num_buffers * (sizeof(LBB)+logpg_size))
				       + logpg_size);
    if (ext == 0)
    {
	(VOID)LG_unmutex(&lfb->lfb_mutex);
	uleFormat(NULL, E_DMA46B_LGALTER_ALLOC_LBB, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
		    0, (num_buffers * (sizeof(LBB) - sizeof(LGK_EXT) +
					logpg_size)) + logpg_size);
	return (LG_EXCEED_LIMIT);
    }

    lbb = (LBB *)((char *)ext + sizeof(LGK_EXT));
    aligned_buffer = (PTR)ME_ALIGN_MACRO(
		(char *)lbb + (i4)(num_buffers * sizeof(LBB)),
		logpg_size);

    for (i = num_buffers; i > 0; i--)
    {
	lbb->lbb_buffer = LGK_OFFSET_FROM_PTR(aligned_buffer);
#ifdef xDEBUG
	TRdisplay("Log block at %p has buffer set to %d\n",
		    lbb, lbb->lbb_buffer);
#endif
	lbb->lbb_lfb_offset = LGK_OFFSET_FROM_PTR(lfb);

	lbb->lbb_owning_lxb = 0;

	/* Set id of buffer, 1 thru num_buffers */
	lbb->lbb_id = ++lfb->lfb_buf_cnt;

	if (status = LG_imutex(&lbb->lbb_mutex,
	    STprintf(sem_name, "LBB mutex %8d", lbb->lbb_id)))
	    return(status);

	lbb->lbb_wait.lxbq_next =
		lbb->lbb_wait.lxbq_prev =
			    LGK_OFFSET_FROM_PTR(&lbb->lbb_wait);
	lbb->lbb_wait_count = 0;

	if ( lfb->lfb_buf_cnt > 1 )
	{
	    /* All but the first buffer go on the free queue */
	    lfb->lfb_fq_count++;
	    lbb->lbb_state = LBB_FREE;

	    /* Insert each last on the queue (FIFO) */
	    lbb->lbb_prev = lfb->lfb_fq_prev;
	    lbb->lbb_next = LGK_OFFSET_FROM_PTR(&lfb->lfb_fq_next);
	    prev_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_fq_prev);
	    prev_lbb->lbb_next = lfb->lfb_fq_prev = LGK_OFFSET_FROM_PTR(lbb);
	}
	else
	{
	    /*
	    ** Make the first buffer the current buffer,
	    ** prepare it for use by LGwrite()
	    */
	    lfb->lfb_current_lbb = lfb->lfb_first_lbb =
	     LGK_OFFSET_FROM_PTR(lbb);

	    lbb->lbb_state = LBB_CURRENT;
	    lbb->lbb_next_byte = lbb->lbb_buffer + sizeof(LBH);
	    lbb->lbb_bytes_used = sizeof(LBH);

	    lbb->lbb_first_lsn.lsn_high = lbb->lbb_first_lsn.lsn_low =
		lbb->lbb_last_lsn.lsn_high = lbb->lbb_last_lsn.lsn_low = 0;

	    /*	Calculate the new LGA for this page. */

	    lbb->lbb_lga.la_sequence = lfb->lfb_header.lgh_end.la_sequence;
	    lbb->lbb_lga.la_block = lfb->lfb_header.lgh_end.la_block + 1;
	    lbb->lbb_lga.la_offset = 0;

	    /*	Handle wrap around. */

	    if (lbb->lbb_lga.la_block >= lfb->lfb_header.lgh_count)
	    {
		lbb->lbb_lga.la_sequence++;
		lbb->lbb_lga.la_block  = 1;
	    }

	    lbh = (LBH *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
	    lbh->lbh_address = lbb->lbb_lga;
	    lbh->lbh_checksum = 0;

	    /*	Set begin if never set. */

	    if (lfb->lfb_header.lgh_begin.la_block == 0) 
		lfb->lfb_header.lgh_begin = lbb->lbb_lga;
	}

	lbb++;
	aligned_buffer += logpg_size;
    }

    /* Set logical end of free queue to last buffer */
    lfb->lfb_fq_leof = lfb->lfb_fq_prev;

    return (OK);
}

/*
** Name: alter_header		-- Fill in new logfile header information.
**
** History:
**	18-oct-1993 (rogerk)
**	    Made logfull limit maximum be 96% to give the logging system
**	    some additional space for recovery in case logspace reservation
**	    algorithms fail.
**	31-jan-1994 (bryanp) B56472
**	    Set lfb_active_log to lgh_active_logs in alter_header. Thus, when
**		a caller forcibly alters the entire log header, the in-memory
**		lfb_active_log will remain in sync. (Perhaps, the entire
**		lfb_active_log field should be replaced by
**		lfb_header.lgh_active_logs, but not today.)
**	23-Jan-1996 (jenjo02)
**	    Lock lfb_mutex and lfb_wq_mutex.
*/
static VOID
alter_header(register LFB *lfb, LG_HEADER *header)
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    i4		logfull_max;

    if (LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	return;
    if (LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex))
	return;

    lfb->lfb_header = *header;

    /*
    ** Update redundant lfb_active_log information:
    */
    lfb->lfb_active_log = lfb->lfb_header.lgh_active_logs;

    /*
    ** Update log address that log file is guaranteed to be forced to.
    */
    if (LGA_LT(&lfb->lfb_forced_lga, &lfb->lfb_header.lgh_end))
    {
	STRUCT_ASSIGN_MACRO(lfb->lfb_header.lgh_end, lfb->lfb_forced_lga);
    }

    /*
    ** Don't allow logfull or force_abort limits to exceed LG_LOGFULL_MAX.
    */
    logfull_max = (lfb->lfb_header.lgh_count * LG_LOGFULL_MAX) / 100;
    if (lfb->lfb_header.lgh_l_logfull > logfull_max)
	lfb->lfb_header.lgh_l_logfull = logfull_max;

    if (lfb->lfb_header.lgh_l_abort > logfull_max)
	lfb->lfb_header.lgh_l_abort = logfull_max;

    /*
    ** Update fields that are computed from values in the log header:
    **    lgd_cpstall     - when to stall at LOGFULL until CP finished.
    **    lgd_check_stall - when to start checking for stall conditions.
    **
    ** lgd_cpstall defaults to 10% before the force_abort limit.
    */
    lgd->lgd_cpstall = lfb->lfb_header.lgh_l_abort - 
				    (lfb->lfb_header.lgh_count / 10);
    if (lgd->lgd_cpstall < 0)
	lgd->lgd_cpstall = lfb->lfb_header.lgh_l_abort;

    /*
    ** LGD_CHECK_STALL gives a point at which we must begin checking
    ** for force-abort and logfull conditions.  It is figured out
    ** as startup time to avoid computing all the logfull parameters
    ** each time a new log buffer is written - we only do these
    ** calculations when we need to.
    **
    ** lgd_check_stall is set to 10% before the most restrictive limit:
    **      - logfull limit
    **      - force_abort limit
    **      - (logsize - 1 buffer for each possible transaction)
    */
    lgd->lgd_check_stall = lfb->lfb_header.lgh_count - lgd->lgd_lxbb_size;
    if (lfb->lfb_header.lgh_l_abort < lgd->lgd_check_stall)
	lgd->lgd_check_stall = lfb->lfb_header.lgh_l_abort;
    if (lfb->lfb_header.lgh_l_logfull < lgd->lgd_check_stall)
	lgd->lgd_check_stall = lfb->lfb_header.lgh_l_logfull;

    lgd->lgd_check_stall = lgd->lgd_check_stall -
				    (lfb->lfb_header.lgh_count / 10);

    (VOID)LG_unmutex(&lfb->lfb_wq_mutex);
    (VOID)LG_unmutex(&lfb->lfb_mutex);
    return;
}
