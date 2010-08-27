/*
**Copyright (c) 2004 Ingres Corporation
**
NO_OPTIM=dgi_us5 int_lnx int_rpl ris_u64 i64_aix
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <dbms.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0c.h>
#include    <dm0j.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGWRITE.C - Implements the LGwrite function of the logging system
**
**  Description:
**	This module contains the code which implements LGwrite.
**	
**	    LGwrite -- write a log record to the transaction log file.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	19-oct-1992 (bryanp)
**	    Disable group commit for now until the VMS CL can handle it.
**	20-oct-1992 (bryanp)
**	    Pick up group commit threshold from the lgd_gcmt_threshold value.
**	    Pick up group commit numticks from the lgd_gcmt_numticks value.
**      04-jan-1993 (jnash)
**          Reduced Logging project.
**	      - Modify action taken when log full detected: it is now a bug 
**	        logfull occurs in LGwrite -- it should happen in LGreserve.  
**	      - Decrement reserved space in lxb, lgd when LGwrite takes place.
**	18-jan-1993 (rogerk)
**	    Update ldb_j_last_la and ldb_d_last_la when write a log record.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed the format of log records to include the database id,
**		transaction id and LSN in the log record itself.  The first
**		field of every log record is a DM0L_HEADER structure with
**		this information.  Changed this module to include dm0l.h and
**		to understand the format of the log record header.  Removed
**		the database and transaction id from the LRH that is prepended
**		to all log records by LG.  This routine now is responsible
**		for writing the database id, transaction id and LSN into the
**		log record header before writing it to the log file.
**	    Changed to return the log record's LSN to the caller rather than
**		the Log Adress assigned to the record (if needed, we can
**		always change this to return both the LSN and the LA).
**	    Changed to update the database/system journal window whenever
**		journal log records are written, even if written by a
**		non-protect transaction.  Also changed to get the session id
**		in case a non-protect transaction does a suspend.
**	    Added LXB_NORESERVE flag.  Transactions which specify this will
**		make updates without reserving space.  This is used during
**		offline recovery processing.
**	    Include reservation factor for log buffer overhead as in LG_reserve.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp)
**	    Cluster 6.5 Project I
**	    LGwrite & LG-write now generate and return an LSN.
**	    Eliminate size variable from LGwrite for clarity and correctness.
**	    Renamed stucture members of LG_LA to new names. This means
**		replacing lga_high with la_sequence, and lga_low with la_offset.
**          Incorporate dual logging testbed instrumentation.
**	    Re-instate group commit, and fix some group commit bugs. We were
**		failing to properly initialize the lpb_gcmt_sid field, so
**		sometimes we would try to awaken a bogus sid.
**	    In multi-node log situations, the group commit threads will only
**		process the local lfb, so we must ensure that we only activate
**		group commit for log page forces on the local LFB, and then
**		only when there exists at least one group commit thread in the
**		system.
**	24-may-1993 (bryanp)
**	    Correct the arguments to PCforce_exit().
**	    Add support for 2K page size.
**	    Don't stall recovering transactions if the db is in opendb_pend.
**	    Add CL_CLEAR_ERR call to clear the CL_ERR_DESC.
**	21-jun-1993 (bryanp)
**	    Pass correct arguments to LGlsn() routines.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    When selecting a logwriter from the list of idle logwriters, choose
**		one in this server if possible. This will hopefully minimize
**		context switching in a system with surplus logwriter threads.
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	23-aug-1993 (bryanp)
**	    When we hit WRITE_PAST_BOF, use a special wait_reason.
**	18-oct-1993 (rmuth)
**	    Prototype LG_pgyback_write.
**	18-oct-1993 (rogerk)
**	    Work to better handle logfull and stall conditions.
**	    Added new LG_calc_reserved_space routine to calculate how much
**	    space is reserved for each log record write.  This new routine
**	    take into account log space overhead hueristics and helps make
**	    sure that the same amount of space is freed in the lgwrite call
**	    as was reserved in the lgreserve request.
**	11-Jan-1993 (rmuth)
**	    b57887, If running with only the dual log we could assign
**	    multiple log writer threads to write the same log buffer.
**	19-jan-1994 (bryanp)
**	    B56706, B58260: If DIwrite fails, and we're dual logging, then let
**		the error be handled by write_complete, rather than causing the
**		logwriter thread to die.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the 
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in write_block(),
**	    write_complete(), LG_dump_logwriters(), 
**	    LG_check_logwriters() and dump_wait_queue().
**	31-jan-1994 (bryanp)
**	    B56474: Once an EMER_SHUTDOWN has been signaled, write no more
**		buffers.
**	    B56475: Emergency shutdowns are used by the dual logging testbed
**		which wants to simulate a system crash with the logfile
**		in a certain
**		state; in order to make this work, we must "freeze" the logfile
**		state by refusing to write any more.
**	    Pass write_primary to LG_check_write_test_2 so it can tell if this
**		is a dual write request or a primary write request.
**	    B57715: Refuse attempts to write objects of negative size.
**	    B56707: If write_block can't re-acquire the mutex, it should quit. 
**	15-apr-1994 (chiku)
**	    Bug56702: Change LG_write to check and return dead logfull condi-
**	    tion return code.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      13-jun-1995 (chech02)
**          Added LGK_misc_sem semaphore to protect critical sections.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LKMUTEX functions. Many new semaphores
**	    added to augment singular lgd_mutex.
**	16-Jan-1996 (jenjo02)
**	    Explicitly identify CS_LOG_MASK in CSsuspend() calls.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	01-Feb-1996 (jenjo02)
**	    Added lfb_fq_count, lfb_wq_count, eliminating the need
**	    for count_free_q_pages(), which was removed.
**	13-Mar-1996 (jenjo02)
**	    Moved incrementing of lpb_stat.write to caller of write_block()
**	    from LG_write() to better track writes by process.
**      06-mar-1996 (stial01 for bryanp)
**          In support of multiple page sizes, allow for very long log record
**              objects:
**              Changed some MEcopy calls to use an internal "copy_long"
**                  subroutine.
**              Change split processing to check for a sufficient number of
**                  available buffers to write the log record (Dependent on the
**                  record's length -- to write a LG_MAX_RSZ record you may
**                  need MANY free buffers).
**              Correctly cast to (u_i2), not (i2), when setting lbh_used.
**	03-jun-1996 (canor01)
**	    Removed LGK_misc_sem since code globals being updated are
**	    only updated at startup, shutdown, and under the LG mutex.
**	29-jul-96 (nick)
**	    Only print reservation space warnings if they are likely to be 
**	    true.
**	11-Sep-1996 (jenjo02)
**	    Reorganized LG_write() to minimize the time that lfb_current_lbb_mutex
**	    is held. Specifically, when the log record image is being copied
**	    into the buffer, release the mutex to allow concurrent access to
**	    the buffer by other threads during the time it takes to do the copy.
**	08-Oct-1996 (jenjo02)
**	    New flag, LGD_CLUSTER_SYSTEM, in lgd_status obviates need to
**	    constantly call LGcluster().
**	    Lock gcmt's lpb_mutex while looking at timer_lbb to avoid
**	    missing gcmt buffers.
**	17-Oct-1996 (jenjo02)
**	    Corrected sizeof(LBH) to sizeof(LRH) when checking for "full enough"
**	    log buffer. Reinstated dropped line of code, record_length -= move,
**	    when writing spanned buffers.
**	06-Dec-1996 (jenjo02)
**	    Allow LG_LAST writes to be handled by gcmt thread.
**	13-Dec-1996 (jenjo02)
**	    ldb_lxbo_count now counts both active and inactive non_READONLY 
**	    protected txns.
**	    Protected, update LXBs, active or inactive, are allowed to complete before
**	    starting online checkpoint. Newly starting non-READONLY protected 
**	    txns are prevented from starting by LGbegin(), suspended on LXB_CKPDB_PEND.
**	08-Jan-1997 (jenjo02)
**	    Instead of calling LG_calc_reserved_space() to estimate the log space
**	    used by a LGwrite(), keep track of the actual space used while the
**	    log record is constructed and decrement lxb and lfb _reserved_space
**	    by that more accurate amount.
**	28-Jan-1997 (jenjo02)
**	    Backed out 08-Jan change. We ended up unreserving less space per log
**	    write than was LGreserve-d. During long-running txns, the extra 
**	    reserved space accumulated such that the log looked full, or nearly
**	    full, when it was not.
**	11-Mar-1997 (jenjo02)
**	    Force write of buffer if all potential writers are waiting
**	    or if stalled on BCP.
**      17-Mar-1997 (mosjo01/linke01)
**          compiler stuck when compiling this file, added NO_OPTIM for dgi_us5
**      10-Sep-1997 (hweho01)
**          Avoid the overflow warning from compiler, set 'null_prt' to 123, 
**          so it is limited to 1 byte storage.
**	23-Sep-1997 (kosma01)
**	    Added No Optim for AIX 4.1 platform. When trying to create
**	    two databases concomitantly, the createdbs would lose connection
**          to the dbms, and the dbms would crash. 
**	11-Nov-1997 (jenjo02)
**	  o Backed out 13-Dec-1996 change. Inactive transactions that hold 
**	    locks are allowed to become active and will NOT be stalled on CKPDB.
**	    Those transactions are identified by the LG_DELAYBT flag.
**	    Inactive transactions marked for CKPDB stall
**	    will otherwise be prevented from becoming active until the
**	    CKPDB stall is cleared.
**	    ldb_lxbo_count is once again only the count of active, protected
**	    transactions in a database.
**	  o Added new functionality to LGwrite() via a new flag LG_NOT_RESERVED.
**	    This flag allows the LGwrite() caller to piggyback log reservation
**	    with the LGwrite() call, obviating the need for a preceding call
**	    to LGreserve(). The space "reserved" by this type of call is the
**	    size of the log record being written, no more and no less.
**	18-Nov-1997 (hanch04)
**	    Removed cast of la_offset
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**	    Keep track of block number and offset into the current block.
**	    The lsn is not based on the LG_LA anymore, is incremental,
**          for support of logs > 2 gig
**	17-Dec-1997 (jenjo02)
**	  o Changed write_complete() to return status instead of VOID to
**	    catch mutex failures which may occur during recovery operations.
**	  o Replaced idle LogWriter queue with a static queue of all
**	    LogWriter threads.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Replace DIread/write with LG_READ/WRITE macros, DI_IO structures
**	    with LG_IO structures.
**	    Replaced lpb_gcmt_sid with lpb_gcmt_lxb, removed lpb_gcmt_asleep.
**	    LG_queue_write() will now write the buffer immediately (without waiting
**	    for a logwriter thread to do it) when possible.
**	19-feb-1998 (toumi01)
**	    Add Linux (lnx_us5) to NO_OPTIM list because of internal compiler
**	    error.
**	24-Aug-1998 (jenjo02)
**	    lxb_tran_id now embedded in (new) LXH structure within LXB.
**	    When transaction becomes active, insert LXB on (new) transaction
**	    hash queue.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	15-Mar-1999 (jenjo02)
**	    Return LG_CKPDB_STALL status instead of suspending thread
**	    when stalling for CKPDB.
**	19-Feb-1999 (jenjo02)
**	    Fixed a host of leaks which could lead to premature resume of
**	    LXB before buffer force was complete, leading to LG_ENDOFILE
**	    errors during rollback.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	    Dropped lgd_lxb_aq_mutex and inactive LXB queue.
**	    lgd_wait_free and lgd_wait_split queues consolidated into
**	    lgd_wait_buffer queue.
**	30-Sep-1999 (jenjo02)
**	    Added lxb_first_lsn for LG_S_OLD_LSN function
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	15-Dec-1999 (jenjo02)
**	    Add support for shared log transactions (LXB_SHARED/LXB_SHARED_HANDLE).
**	17-Mar-2000 (jenjo02)
**	    Release lxb_mutex early in LGwrite (also LGforce) to allow the 
**	    possibility of async write_complete without a resulting 
**	    lxb_mutex block.
**      30-Jun-2000 (horda03) X-Int of fix for bug 48904 (VMS only)
**        30-mar-94 (rachael)
**           Added call to CSnoresnow to provide a temporary fix/workaround
**           for sites experiencing bug 48904. CSnoresnow checks the current
**           thread for being CSresumed too early.  If it has been, that is if 
**           scb->cs_mask has the EDONE bit set, CScancelled is called.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Corrected computation and accumulation of Kbytes 
**	    written stat.
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LGcp_resume macro.
**      01-feb-2001 (stial01)
**          hash on UNIQUE low tran (Variable Page Type SIR 102955)
**      26-Feb-2001 (hweho01)
**	    Add ris_u64 to NO_OPTIM list because of optimizer error.
**          Symptom: unable to start recovery server (dmfrcp). 
**          Compiler: IBM C/C++ compiler version 3.6.6.6.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	17-sep-2001 (somsa01)
**	    Due to last cross-integration, changed lbb_lbh_mutex to
**	    lbb_mutex.
**	17-sep-2001 (somsa01)
**	    Backed out fix for bug 104569, as it is not needed in 2.6+.
**	12-nov-2002 (horda03) Bug 105321 INGSRV 1502
**	    Detect and resolve LXB_WAIT errors due to rogue resumes.
**	03-Jan-2003 (jenjo02)
**	    Incorporated the intent of fix 459484 for bug 104589,
**	    which -is- needed, especially when dual logging is used.
**	    "lbb_state" must not be altered unless lbb_mutex is held.
**      23-Jul-2003  (hweho01)
**	    Add r64_us5 to NO_OPTIM list because of optimizer error.
**          Symptom: unable to start recovery server (dmfrcp). 
**          Compiler : VisualAge 5.023. 
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**          Removed unnecessary copy_long routine
**      10-jun-2004 (horda03) INGSRV2838/Bug 112382
**          When updating the Archive window for a DB, need to ensure
**          that the LBB mutex taken is really for the CURRENT lbb.
**          If it isn't, then the Archiver Start window will be set
**          incorrectly and a E_DMA464 will occur.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      22-Oct-2003 (hweho01)
**          Removed rs4_us5 and r64_us5 from NO_OPTIM list. 
**          Compiler : VisualAge C 6.006. 
**	20-Sep-2006 (kschendel)
**	    The entire sequence of getting a free buffer and putting the
**	    LXB on a wait queue has to be mutex protected.  Otherwise,
**	    all buffers could become free between the failure return from
**	    the allocation request, and the queueing of the LXB.  If in
**	    addition the waiting LXB is the recovery thread which has just
**	    turned on BCPSTALL, nobody is left to wake up the mistakenly
**	    suspended thread, and the logging system hangs.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**      19-nov-2007 (stial01 for jenjo02)
**          LG_queue_write() If there is only one logwriter configured,
**          queue writes to the logwriter.
**      28-feb-2008 (stial01) 
**          LG_queue_write() INACTIVE transaction must assign or write buffer
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**      27-jun-2008 (stial01)
**          Changes to fix lgh_last_lsn after incremental rollforwarddb
**      16-jul-2008 (stial01)
**          New lgh_last_lsn can be LTE tmsecs
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: LG_LSN output param replaced with pointer
**	    to new LG_LRI structure, which is populated with the log
**	    record's LSN, LGA, journal sequence, and journal offset.
*/


/*
** Forward declarations of static functions:
*/
static STATUS	LG_write(
		i4                 *flag,
		LG_ID		    *lx_id,
		i4		    num_objects,
		LG_OBJECT	    *objects,
		LG_LRI		    *lri,
		STATUS		    *async_status,
		CL_ERR_DESC	    *err_code);

static	STATUS		write_block(LBB *lbb, 
				    i4 write_primary,
				    LXB *lxb,
				    STATUS *async_status,
				    CL_ERR_DESC *sys_err);
static	STATUS		write_complete(
				    LBB *FirstLBB,
				    LXB *lxb,
				    i4 primary,
				    i4 num_pages,
				    STATUS *async_status);

static	VOID		dump_wait_queue(LBB *lbb, char *comment);
static	LXB		*choose_logwriter( VOID );


/*{
** Name: LGwrite	- Write record to log file.
**
** Description:
**      Writes a record to the log file.  The log address for this record is
**	returned.  This address can be used to read a record from the
**	log file, or it can be used in a call to LGforce to insure that
**	the record has made it to disk safely.  Setting the flag to LG_FORCE
**	will guarantee that the log record has safely made it to disk before
**	the LGwrite call completes.  This can save a call to LGforce. 
**	Setting the flag to LG_LAST to indicate the last log record written
**	for the current transactoin. This will remove the transaction context,
**	lxb, from the logging system and thus save a call to LGend.
**
**	The log record is passed in as an array of LG_OBJECT structs.  Each of
**	these entries gives the size and ptr to a data block.  When all of
**	these data blocks are written contiguously, they make up a log record.
**
** Inputs:
**	flag				Either LG_FORCE, LG_LAST or 0.
**					LG_LOGFULL_COMMIT bit may be added.
**	lx_id				Transaction identifier.
**	num_objects			Number of entries in the objects array.
**	objects				Array of size,data entries that make up
**					log record.
**
** Outputs:
**      lsn				Log sequence number assigned to record.
**					NOTE: returns zero if no log write
**					because of shared txn disconnect (in
**					which case nobody cares about the lsn;
**					zero tells the caller what happened).
**	sys_err				Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM                 Parameter(s) in error.
**	    LG_WRITEERROR               Error writing log.
**	Exceptions:
**	    none
**
** Side Effects:
**          A Consistency Point(CP) can be signalled, the archiver signalled,
**          or an abort signalled depending on the limit on the 
**          log file such as how many blocks have been written since
**          last CP and how full the log file is.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**      04-jan-1993 (jnash)
**          Reduced logging project.  Include comment regarding modified
**	    behavior at logfull.  We have chosen not to make this change
**	    at this time, comment included for documentation.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed to return the log record's LSN to the caller rather than
**		the Log Adress assigned to the record.
**	24-may-1993 (bryanp)
**	    Add CL_CLEAR_ERR call to clear the CL_ERR_DESC.
**	16-Jan-1996 (jenjo02)
**	    Explicitly identify CS_LOG_MASK in CSsuspend() calls.
**	26-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex.
**      30-Jun-2000 (horda03) X-Int of fix for bug 48904 (VMS only)
**        30-mar-94 (rachael)
**           Added call to CSnoresnow to provide a temporary fix/workaround
**           for sites experiencing bug 48904. CSnoresnow checks the current
**           thread for being CSresumed too early.  If it has been, that is if 
**           scb->cs_mask has the EDONE bit set, CScancelled is called.
**      19-Sep-2002 (horda03)
**           Detect and handle LXB_WAIT errors caused by a Rogue Resume.
*/
STATUS
LGwrite(
i4                  flag,
LG_LXID		    lx_id,
i4		    num_objects,
LG_OBJECT	    *objects,
LG_LRI		    *lri,
CL_ERR_DESC	    *sys_err)
{
    STATUS	status;
    i4		internal_flag = flag;
    STATUS	async_status = 0;
    i4	err_code;
    SIZE_TYPE   *lxbb_table;
    LXB         *lxb;
    LGD         *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LG_I4ID_TO_ID xid;
    i4          loop_cnt;

    CL_CLEAR_ERR(sys_err);

    if ((num_objects <= 0) 		|| 
	(num_objects > LG_MAX_OBJECTS)	||
	(lri == (LG_LRI *) NULL)		||
	(objects == (LG_OBJECT *) (NULL)))
    {
	/*
	** Other possible checks include checking to see if there is read
	** access to every element in the object array, and whether there is
	** write access to the lsn pointer.
	*/
	uleFormat(NULL, E_DMA402_LG_WRITE_BADPARM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, num_objects, 0, lri, 0, objects);
	return(LG_BADPARAM);
    }

    xid.id_i4id = lx_id;
    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[xid.id_lgid.id_id]);

    /* Remove any Rogue Resumes */
    CSnoresnow( "LGwrite()", 1);

    do 
    {
        loop_cnt = 0;

	/*
	** in the case where internal_flag & LG_LAST is true LG_write
	** will change it's value expecting the retry operation to
	** try with a different flag.
	*/
	status = LG_write(&internal_flag, &xid.id_lgid, num_objects, 
			    objects, lri, &async_status, sys_err);

	/* Returns include (others are errors to be returned to caller):
	**	LG_SYNCH	- completed without having to wait.
	**	LG_MUST_SLEEP	- could not get some resource so wait until it
	**			  is around and then redo the call.
	**     (LG_NOSPACE	- LG bug -- LGwrite should not encounter
	**			  log file space problems.)
	**	OK	- finished work but need to wait for an event.
	**			  when event arives then work is done.
	*/

	if (status == OK)
	{
            do
            {
               if (loop_cnt++)
               {
                  TRdisplay( "LGwrite():: LXB status = %08x. Loop = %d\n",
                             lxb->lxb_status, loop_cnt - 1);
               }

	       CSsuspend(CS_LOG_MASK, 0, 0);
            }
            while (lxb->lxb_status & LXB_WAIT);

	    if (async_status)
		status = LG_WRITEERROR;
	    else
		status = OK;
	}
	else if (status == LG_MUST_SLEEP)
	{
            do
            {
               if (loop_cnt++)
               {
                  TRdisplay( "LGwrite(1):: LXB status = %08x. Loop = %d\n",
                             lxb->lxb_status, loop_cnt - 1);
               }

	       CSsuspend(CS_LOG_MASK, 0, 0);
            }
            while (lxb->lxb_status & LXB_WAIT);
	}
	else
	{
	    break;
	}

    } while (status == LG_MUST_SLEEP);

    if (status == LG_SYNCH)
	status = OK;

    return(status);
}

/*{
** Name: LG_write	- Write log file.
**
** Description:
**      This routines will write a record to the log file.  The record
**	can be associated with a transaction by passing in a valid lx_id.
**	Special write operations are available for updating the log
**	file header.  Only the log master can perform these operations.
**
**	The log record is passed to this routine in an array of LG_OBJECTS.
**	Each entry in this array gives the size and ptr to the data making
**	up the objects.  When all of the objects are written continguously,
**	they make up one log record.
**
**	Every log record must have as its first element a DM0L_HEADER
**	structure.  This entire structure must be contained in the first
**	LG_OBJECT of the LGwrite call.  The following fields of the log
**	record header will be filled in by LGwrite before writing the record
**	to the log file:
**
**		database_id	: filled in with the external database id
**		transaction_id	: set to the external transaction id
**		LSN		: the log record's LSN, generated by this call
**
**	The LSN (Log Sequence Number) assigned to the log record is returned
**	to the LGwrite caller.
**
** Inputs:
**	flag				Zero or combination of the following:
**					LG_FORCE  - Current log buffer should
**					    be force to disk after writing this
**					    record.
**					LG_LAST - This is the last record for
**					    this transaction and it should be
**					    removed from the logging system.
**					LG_FIRST - This is the first record for
**					    this transaction.  It is an error
**					    to write a non LG_FIRST record for
**					    a transaction marked LXB_INACTIVE.
**					LG_LOGFULL_COMMIT - May be added
**					    when writing an ET to preserve 
**					    the log context after commit.
**      lg_id                           Log identifier.
**	lx_id				Transaction identifier.
**      num_objects			Number of entries in objects array.
**	objects				Array of size,data entries.
**
** Outputs:
**      lsn                             Log sequence number assigned to record.
**					NOTE: returns zero if no log write
**					because of shared txn disconnect, in
**					which case nobody cares about the lsn;
**					zero tells the caller what happened
**					without inventing a new status.
**	Returns:
**	    LG_BADPARAM		Bad parameters.
**	    OK			Sucessful but wait.
**	    LG_SYNCH			Synchronous sucess.
**	    LG_MUST_SLEEP			Wait and restart.
**	    LG_DB_INCONSISTENT			Inconsistent database.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	20-oct-1992 (bryanp)
**	    Pick up group commit threshold from the lgd_gcmt_threshold value.
**	04-jan-1993 (jnash)
**          Reduced logging project.
**	      - Modify action taken when log full detected:
**		its now a bug when it occurs here -- it should
**		happen in LGreserve.
**	      - Decrement space reserved for log record after write.
**	      - Maintain new lxb_last_lsn and lrh_lsn fields based
**		on cluster state.
**	      - (Maybe) Return LG_NOSPACE on log full, not LG_MBFULL.
**	18-jan-1993 (rogerk)
**	    Update ldb_j_last_la and ldb_d_last_la to the log address of
**	    the log record just written.
**	15-mar-1993 (rogerk & jnash)
**	    Reduced Logging - Phase IV:
**	    Changed the format of log records to include the database id,
**		transaction id and LSN in the log record itself.  The first
**		field of every log record is a DM0L_HEADER structure with
**		this information.  Changed this module to include dm0l.h and
**		to understand the format of the log record header.  Removed
**		the database and transaction id from the LRH that is prepended
**		to all log records by LG.  This routine now is responsible
**		for writing the database id, transaction id and LSN into the
**		log record header before writing it to the log file.
**	    Changed to return the log record's LSN to the caller rather than
**		the Log Adress assigned to the record (if needed, we can
**		always change this to return both the LSN and the LA).
**	    Changed to update the database/system journal window whenever
**		journal log records are written, even if written by a
**		non-protect transaction.  Also changed to get the session id
**		in case a non-protect transaction does a suspend.
**	    Changed to allow nonprotect transactions to log its first update
**		without signifying the LG_FIRST flag.
**	    Added LXB_NORESERVE flag.  Transactions which specify this will
**		make updates without reserving space.  This is used during
**		offline recovery processing by the RCP and Rollforward.
**	    Include reservation factor for log buffer overhead as in LG_reserve.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**	    Substantial changes for non-local logfile processing. Isolate log
**		information into new LFB control block.
**	    Call LGlsn_next to generate an LSN.
**	    In multi-node log situations, the group commit threads will only
**		process the local lfb, so we must ensure that we only activate
**		group commit for log page forces on the local LFB, and then
**		only when there exists at least one group commit thread in the
**		system.
**	24-may-1993 (bryanp)
**	    Add support for 2K page size.
**	    Don't stall recovering transactions if the db is in opendb_pend.
**	21-jun-1993 (bryanp)
**	    Pass correct arguments to LGlsn() routines.
**	26-jul-1993 (rogerk)
**	  - Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added a new field - lfb_archive_window_prevcp - 
**	    which gives the consistency point address before which we know 
**	    there is no archiver processing required.  Consolidated the system
**	    journal and dump windows into a single lfb_archive_window.
**	  - Moved the initialization of the dump and journal windows outside
**	    of the first-log-record processing.  Since the archiver can now
**	    process uncommitted transactions, it can close the journal and
**	    dump windows while transactions are in progress. Those transactions
**	    must re-initialize the journal/dump windows on their next log
**	    record writes.
**	23-aug-1993 (bryanp)
**	    When we hit WRITE_PAST_BOF, use a special wait_reason.
**	18-oct-1993 (rogerk)
**	    Work to better handle logfull and stall conditions.
**	    Removed the normal check for logfull stalling here.  We now expect
**	    this to be caught in the LGreserve call.  Reorganized some of the
**	    ckpdb_pending and opendb_pend code and added traces for unexpected
**	    conditions.
**	    Took out the check for logfile wrap-around (EOF exceeds BOF) -
**	    allocate_lbb does not return null lbb's in this case anymore.
**	    Changed logspace reservation checks to work with unsigned values.
**	    Added new LG_calc_reserved_space routine to calculate how much
**	    space is reserved for each log record write.  This new routine
**	    take into account log space overhead hueristics and helps make
**	    sure that the same amount of space is freed in the lgwrite call
**	    as was reserved in the lgreserve request.
**	31-jan-1994 (bryanp)
**	    B57715: Refuse attempts to write objects of negative size. The
**		code previously validated the overall computed record length
**		for being non-negative, but in at least one example a caller
**		managed to pass in 5 objects: 4 objects of positive length and
**		one object of negative length. The overall record length was
**		computed to be positive, but the negative length object caused
**		the later MEcopy() call to blast 65000 bytes of shared memory.
**	15-apr-1994 (chiku)
**	    Bug56702: Change LG_write to check and return dead logfull condi-
**	    tion return code.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	26-Jan-1996 (jenjo02)
**	    Many new mutexes added to relieve the load on singular
**	    lgd_mutex.
**	01-Feb-1996 (jenjo02)
**	    Added lfb_fq_count, lfb_wq_count, eliminating the need
**	    for count_free_q_pages(), which was removed.
**	    Redid how spanned LBBs are accounted for. Since the singular
**	    lgd_mutex no longer guarantees that the lfb_fq is blocked
**	    for the duration of all spanned LBB writes, we now 
**	    reserve the correct number of LBBs on the free queue
**	    in advance, thus guaranteeing that we'll have enough
**	    to complete the spanned write while not tying up the
**	    free queue.
**	13-Mar-1996 (jenjo02)
**	    Moved incrementing of lpb_stat.write to caller of write_block()
**	    from LG_write() to better track writes by process.
**	26-Jun-1996 (jenjo02)
**	    Count only active, protected txns in lgd_protect_count.
**	29-jul-96 (nick)
**	    Reduce frequency of some error codes.
**	11-Sep-1996 (jenjo02)
**	    Reorganized LG_write() to minimize the time that lfb_current_lbb_mutex
**	    is held. Specifically, when the log record image is being copied
**	    into the buffer, release the mutex to allow concurrent access to
**	    the buffer by other threads during the time it takes to do the copy.
**	08-Oct-1996 (jenjo02)
**	    New flag, LGD_CLUSTER_SYSTEM, in lgd_status obviates need to
**	    constantly call LGcluster().
**	17-Oct-1996 (jenjo02)
**	    Corrected sizeof(LBH) to sizeof(LRH) when checking for "full enough"
**	    log buffer. Reinstated dropped line of code, record_length -= move,
**	    when writing spanned buffers.
**	08-Jan-1997 (jenjo02)
**	    Instead of calling LG_calc_reserved_space() to estimate the log space
**	    used by a LGwrite(), keep track of the actual space used while the
**	    log record is constructed and decrement lxb and lfb _reserved_space
**	    by that more accurate amount.
**	28-Jan-1997 (jenjo02)
**	    Backed out 08-Jan change. We ended up unreserving less space per log
**	    write than was LGreserve-d. During long-running txns, the extra 
**	    reserved space accumulated such that the log looked full, or nearly
**	    full, when it was not.
**	11-Mar-1997 (jenjo02)
**	    Force write of buffer if all potential writers are waiting
**	    or if stalled on BCP.
**	11-Nov-1997 (jenjo02)
**	    Backed out 13-Dec-1996 change. Inactive transactions that hold 
**	    locks are allowed to become active and will NOT be stalled on CKPDB.
**	    Those transactions are identified by the LG_DELAYBT flag. 
**	    Inactive transactions marked for CKPDB stall
**	    will otherwise be prevented from becoming active until the
**	    CKPDB stall is cleared.
**	    ldb_lxbo_count is once again only the count of active, protected
**	    transactions in a database.
**	  o Added new functionality to LGwrite() via a new flag LG_NOT_RESERVED.
**	    This flag allows the LGwrite() caller to piggyback log reservation
**	    with the LGwrite() call, obviating the need for a preceding call
**	    to LGreserve(). The space "reserved" by this type of call is the
**	    size of the log record being written, no more and no less.
**	24-Aug-1998 (jenjo02)
**	    lxb_tran_id now embedded in (new) LXH structure within LXB.
**	    When transaction becomes active, insert LXB on (new) transaction
**	    hash queue.
**	15-Mar-1999 (jenjo02)
**	    Return LG_CKPDB_STALL status instead of suspending thread
**	    when stalling for CKPDB.
**	19-Feb-1999 (jenjo02)
**	    Fixed a host of leaks which could lead to premature resume of
**	    LXB before buffer force was complete, leading to LG_ENDOFILE
**	    errors during rollback.
**	19-Jul-2001 (jenjo02)
**	    Wrap updating of archive window log addresses with lgd_mutex
**	    instead of ldb_mutex and lgd_mutex. This closes a hole 
**	    in which lfb_archive_window_start could assume a value
**	    that is not the union of all open databases, leading to
**	    DM9845 failures in the archiver.
**	27-sep-2001 (thaju02)
**	    If we detect logfull and we have previously reserved logspace, 
**	    avoid suspending session, as this may lead to a hang situation.
**	    (b97058)
**	07-Aug-2002 (jenjo02)
**	    If first XA txn must wait for log split while trying to
**	    write BT, BT will not get written after wait. Instead of
**	    checking LXB_ACTIVE, check lxb_first_lsn to insure that
**	    BT has made it. This solves E_DMA455_LGREAD_HDR_READ
**	    error during rollback. (BUG 108466)
**	    When deciding which LSN to wait for completion, check
**	    that it's the ET being written rather than relying on
**	    "LG_LAST"; sometimes LG_LAST is not present when the
**	    ET is being written.
**	15-Oct-2002 (jenjo02)
**	    Closed a hole uncovered on Linux whereby a thread could
**	    obtain an incorrect mid-span value of the log EOF
**	    and set that value as the archiver window start. The
**	    archiver would then fail with E_DMA464 when trying to
**	    read the incomplete log record. (BUG 108944)
**	18-Oct-2002 (jenjo02)
**	    Oops, that last change removed the update of EOF
**	    during a span; LG_allocate_lbb() needs the updated
**	    value to assign the next LGA.
**	22-Oct-2002 (jenjo02)
**	    Return LG_BADPARAM if attempt to write to aborting
**	    SHARED transaction.
**	08-Apr-2004 (jenjo02)
**	    Removed 22-Oct-2002 check of aborting SHARED txn
**	    which was functionally correct but produced a lot
**	    of really ugly and scary errors in the log.
**	    During force-abort of a SHARED transaction all
**	    connected Handles also get force-aborted, so we'll
**	    just let them procede through logging until the
**	    force-abort gets discovered at a higher level.
**	    Added support for IN_MINI. Handles which are not
**	    themselves in a MINI can't do log writes while
**	    another Handle is in a MINI and must wait until
**	    a LG_A_EMINI is signalled.
**	20-Apr-2004 (jenjo02)
**	    Added LG_LOGFULL_COMMIT support, tagged onto write
**	    of ET. Writes ET but preserves the log context
**	    (see LG_end_tran).
**	03-May-2004 (jenjo02)
**	    Delay the writing of the ET until the last HANDLE
**	    commits rather than when the first handle commits.
**	    This is orthogonal with the way rollback/abort works.
**      10-Jun-2004 (horda03) INGSRV2838/Bug 112382
**          To determine the EOF of the TX log file, ensure the
**          lbb_mutex acquired is for the CURRENT lbb.
**	22-Jul-2004 (schka24)
**	    Since log-tracing is done in the caller (unfortunately),
**	    pass back a zero LSN when we disconnect from a shared txn
**	    with no log write.  This way the caller knows what happened
**	    and I don't have to invent / check for a new return status.
**	    (Yes, this is something of a hack.  So YOU fix it. :-)
**	5-Jan-2006 (kschendel)
**	    Make reserve-space counter larger, for giant logs.
**	15-Mar-2006 (jenjo02)
**	    o Exclude lbb_commit_count from lgd_protect_count when
**	      deciding whether to write the buffer -now- or later.
**	      Those txn's which have committed in this buffer
**	      won't be doing any more writes, even though they're
**	      still counted in lgd_protect_count.
**	    o If local gcmt thread is busy, try the one in the RCP.
**	    o lxb_wait_reason defines moved to lg.h
**	    o count log record splits even when they don't have
**	      to wait for an available buffer.
**	11-Sep-2006 (jonj)
**	    Reworked LOGFULL_COMMIT protocols to handle shared
**	    transactions.
**	    Deal with LG_2ND_BT flag - it means the transaction is
**	    writing a second, journaled, BT record and we must let
**	    it be written in the case of SHARED transactions.
**	01-Nov-2006 (jonj)
**	    When setting ldb_j_first, ldb_d_first, lfb_archive_window_start
**	    check that our snapshot EOF may be earlier than one set by a later
**	    thread while we were stalled on a mutex to avoid E_DM9845's
**	    in the ACP.
**      21-May-2007 (wanfr01)
**          Bug 118368
**          Recheck archiver window after you write the record
**          out and reset if necessary.
**	07-Sep-2007 (jonj)
**	    BUG 119091 SD 121503
**	    When picking a group commit thread, note that in the RCP
**	    master_lpb's gcmt_lxb will be the same as lpb's gcmt_lxb.
**	    Correct type of gcmt_offset is SIZE_TYPE, not i4.
**	06-Nov-2007 (jonj)
**	    When bailing out on a mutex error, be sure to unwind other
**	    held mutexes to alleviate mutex lockups in LG_rundown and
**	    transaction recovery.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: On commit, record LSN of ET in ldb_last_commit.
**	    Populate LG_LRI. On each write to MVCC journaled database, compute
**	    JFA, set transaction inactive when it commits.
**	17-Mar-2010 (jonj)
**	    SIR 121619 MVCC: whether journaling or not, the transaction
**	    must be put on the LDB's active transaction's list when doing
**	    its first write.
**	06-May-2010 (jonj)
**	    When optimized writes are enabled with a single logwriter,
**	    don't initiate group commit - it's a performance killer.
**	06-Jul-2010 (jonj) SIR 122696
**	    Last 4 bytes of non-header pages now reserved for la_sequence,
**	    excluded from "available" and "bytes_used".
*/
static STATUS
LG_write(
i4                  *flag,
LG_ID		    *lx_id,
i4		    num_objects,
LG_OBJECT	    *objects,
LG_LRI		    *lri,
STATUS		    *async_status,
CL_ERR_DESC	    *sys_err)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB	*lxb, *handle_lxb;
    LXB			*next_lxb;
    LXB			*prev_lxb;
    LBB			*lbb;
    LBB			*span_lbb;
    register LBH	*lbh;
    register LRH	*lrh;
    register LDB        *ldb;
    LPD                 *lpd;
    LPB			*lpb;
    register LFB	*lfb;
    LPB			*master_lpb;
    register char	*buf_ptr;
    register char	*obj_ptr;
    register i4	obj_num;
    register i4	obj_bytes_left;
    i4		record_length;
    i4		size_lr;
    i4		avail;
    i4		move_amount;
    SIZE_TYPE	*lxbb_table;
    i4		rec_end;
    i4		*size_ptr;
    i4		err_code;
    u_i8	reserved_space;
    i4		log_space_basis;
    DM0L_HEADER		*dm0l_header;
    STATUS		status, return_status;
    i4		span_bytes;
    LG_LA	lgh_end_now;
    LG_I4ID_TO_ID xid1, xid2;
    LG_JFIB	*jfib;
    i4		jfib_size;
    LXBQ	*lxbq, *next_lxbq, *prev_lxbq;

#define MAX_MECOPY_LENGTH	65535	/* max MEcopy length */

    LG_WHERE("LG_write")

    /*	Check the lx_id. */

    if (lx_id->id_id == 0 || (i4)lx_id->id_id > lgd->lgd_lxbb_count)
    {
	uleFormat(NULL, E_DMA403_LG_WRITE_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lx_id->id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    handle_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);

    if (handle_lxb->lxb_type != LXB_TYPE ||
	handle_lxb->lxb_id.id_instance != lx_id->id_instance)
    {
	uleFormat(NULL, E_DMA404_LG_WRITE_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, handle_lxb->lxb_type, 
		    0, handle_lxb->lxb_id.id_instance,
		    0, lx_id->id_instance);
	return (LG_BADPARAM);
    }

    /* Lock the LXB's mutex */
    if (status = LG_mutex(SEM_EXCL, &handle_lxb->lxb_mutex))
	return(status);

    /*
    ** If LXB is supposed to be waiting or it's a readonly txn
    ** and not a legitimate internal log write,
    ** then that's bad.
    */
    if ( (handle_lxb->lxb_status & LXB_WAIT || handle_lxb->lxb_wait_lwq)
	||
	 (handle_lxb->lxb_status & LXB_READONLY &&
	  !(handle_lxb->lxb_status & LXB_ILOG) &&
	      !(*flag & LG_ILOG)) )
    {
	uleFormat(NULL, E_DMA406_LG_WRITE_BAD_STAT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1, 0, 
		    handle_lxb->lxb_status);
	LG_unmutex(&handle_lxb->lxb_mutex);
	return (LG_BADPARAM);
    }

    /* If internal logging in a readonly txn, note it LXB */
    if ( handle_lxb->lxb_status & LXB_READONLY && *flag & LG_ILOG )
	handle_lxb->lxb_status |= LXB_ILOG;

    /* 
    ** The LG_LAST flag indicates that this is not really a request to write
    ** a log record, but is instead a request to call LG_end_tran.
    **
    ** Note that if the flag value is not equal to LG_LAST but includes that
    ** value in its bitmask then the request falls through to do a logforce
    ** below.  In this case, it is expected that LGwrite will call us back
    ** following the force completion with flag set to exactly LG_LAST.
    */
    if ( (*flag & (LG_FORCE | LG_FIRST | LG_LAST | LG_ILOG | LG_2ND_BT)) == LG_LAST )
    {
	/*
	** LG_end_tran will free the LXB, 
	** hence the lxb_mutex
	*/
	(VOID)LG_end_tran(handle_lxb, *flag & LG_LOGFULL_COMMIT);
	return (LG_SYNCH);
    }

    if (num_objects < 0 || num_objects > LG_MAX_OBJECTS)
    {
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return (LG_BADPARAM);
    }

    /* Calculate total size of this log record from pieces in object array */

    record_length = 0;
    for (obj_num = 0; obj_num < num_objects; obj_num++)
    {
	if (objects[obj_num].lgo_size > LG_MAX_RSZ ||
	    objects[obj_num].lgo_size <= 0)
	{
	    /* B57715: Refuse rqsts which pass negative object lengths */
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    uleFormat(NULL, E_DMA405_LG_WRITE_BAD_RLEN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, objects[obj_num].lgo_size, 0, obj_num);
	    return (LG_BADPARAM);
	}
	record_length += objects[obj_num].lgo_size;
    }
    if (record_length > LG_MAX_RSZ || record_length <= 0)
    {
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	uleFormat(NULL, E_DMA405_LG_WRITE_BAD_RLEN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, record_length, 0, LG_MAX_RSZ);
	return (LG_BADPARAM);
    }

    /* Save record_length for log reservation calculations later */
    log_space_basis = record_length;

    /*
    ** Make move_amount be an increment of sizeof(i4) and add an extra
    ** sizeof(i4) bytes to write the size value.
    */
    move_amount = ((record_length + (sizeof(i4) -1)) &
	~(sizeof(i4) -1)) + sizeof(i4);

    /*
    ** size_lr will be move_amount plus the size of an LRH.
    */
    size_lr = sizeof(LRH) + move_amount;

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lfb_offset);
    lpd = (LPD *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_lpd);
    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    /*
    ** If LXB is a handle to a SHARED LXB, then use the actual shared
    ** LXB for the write request. Remember the handle LXB as it will be
    ** the one suspended if need arises.
    **
    ** The HANDLE LXBs:
    **		o are flagged NON-PROTECT, are not recoverable
    **		o have affinity to a thread within a process
    **		o may be used to write log buffers
    **	 	o may be suspended/resumed
    **		o never appear on the active txn / hash queues
    **		o contains no log space reservation info
    **
    ** The SHARED LXB:
    **		o is flagged PROTECT and is recoverable
    **		o has no thread or process affinity
    **	  	o is never used for I/O
    **		o contains all the LSN/LGA info about the
    **		  transaction.
    **		o is never suspended
    **		o appears on the active txn and hash queues
    **		o contains log space reservation info
    */
    lxb = handle_lxb;
    if ( handle_lxb->lxb_status & LXB_SHARED_HANDLE )
    {
	if ( handle_lxb->lxb_shared_lxb == 0 )
	{
	    xid1.id_lgid = handle_lxb->lxb_id;
	    uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, xid1.id_i4id,
			0, handle_lxb->lxb_status,
			0, "LG_write");
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_BADPARAM);
	}

	lxb = (LXB *)LGK_PTR_FROM_OFFSET(handle_lxb->lxb_shared_lxb);

	if ( lxb->lxb_type != LXB_TYPE ||
	    (lxb->lxb_status & LXB_SHARED) == 0 )
	{
	    xid1.id_lgid = lxb->lxb_id;
	    xid2.id_lgid = handle_lxb->lxb_id;
	    uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lxb->lxb_type,
			0, LXB_TYPE,
			0, xid1.id_i4id,
			0, lxb->lxb_status,
			0, xid2.id_i4id,
			0, "LG_write");
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_BADPARAM);
	}
	if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	{
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}
	/* Both the handle and shared LXBs are now mutexed */

	/*
	** Note in SHARED that internal logging in a READONLY
	** transaction is ok. Later connecting LXBs will have
	** this status flag copied from this SHARED LXB.
	*/
	if ( handle_lxb->lxb_status & LXB_ILOG )
	    lxb->lxb_status |= LXB_ILOG;

	/*
	** Any number of handles may attempt to write a BT/ET,
	** but we can allow only one of each to be written on
	** behalf of the shared transaction.
	**
	** If writing ET, delay it until the last HANDLE 
	** commits. If the SHARED transaction never became
	** active, don't write the ET.
	**
	** LOGFULL_COMMIT is different because the handles
	** aren't going away. In this case we keep track
	** of the number of handles that have actually done
	** log writes and only let the ET be written when
	** the last writer is writing its ET. 
	** Until that happens, the other writing handles
	** are stalled in WAIT_COMMIT and will be
	** resumed by LG_end_tran when the real ET is written.
	**
	** If writing BT and its already been done (txn's first
	** lsn has been recorded), return the LSN of the BT
	** to the caller and just go back synchronously.
	*/
	if ( *flag & LG_LAST && *flag & LG_LOGFULL_COMMIT )
	{
	    /* If this handle wrote anything... */
	    if ( handle_lxb->lxb_stat.write )
	    {
		/* Set ET received, just for logstat */
		handle_lxb->lxb_status |= LXB_ET;
		
		/* If not all writing handles have ET'd... */
		if ( ++lxb->lxb_handle_ets < lxb->lxb_handle_wts )
		{
		    handle_lxb->lxb_wait_reason = LG_WAIT_COMMIT;
		    LG_suspend(handle_lxb, &lgd->lgd_wait_commit, async_status);
		    LG_unmutex(&lxb->lxb_mutex);
		    (VOID)LG_end_tran(handle_lxb, *flag & LG_LOGFULL_COMMIT);
		    return(OK);
		}
		/* fall thru to write the real ET, reset txn */
	    }
	    else
	    {
		/* Not a writer, ignore ET */
		lri->lri_lsn.lsn_high = lri->lri_lsn.lsn_low = 0;
		LG_unmutex(&lxb->lxb_mutex);
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return(LG_SYNCH);
	    }
	}
	else if ( *flag & LG_LAST && 
	     (lxb->lxb_status & LXB_INACTIVE || lxb->lxb_handle_count > 1) )
	{
	    /* Nothing cares about the lsn if we're just disconnecting.
	    ** Return a zero so tracing in the caller knows what didn't
	    ** happen.
	    */
	    lri->lri_lsn.lsn_high = lri->lri_lsn.lsn_low = 0;

	    LG_unmutex(&lxb->lxb_mutex);
	    (VOID)LG_end_tran(handle_lxb, *flag & LG_LOGFULL_COMMIT);
	    return(LG_SYNCH);
	}
	if ( *flag & LG_FIRST && !(*flag & LG_2ND_BT) )
	{
	    /* If (first) BT already written, just return */
	    if ( lxb->lxb_status & LXB_ACTIVE )
	    {
		/* Return LSN of BT to caller */
		lri->lri_lsn = lxb->lxb_first_lsn;
		LG_unmutex(&lxb->lxb_mutex);
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return(LG_SYNCH);
	    }
	}

	/*
	** If SHARED transaction is in a mini-transaction
	** and it's not us, we must wait for the 
	** mini-transaction to end (LG_A_EMINI) before
	** doing any more log writes.
	*/
	if ( lxb->lxb_status & LXB_IN_MINI &&
	    (handle_lxb->lxb_status & LXB_IN_MINI) == 0 )
	{
	    handle_lxb->lxb_wait_reason = LG_WAIT_MINI;
	    LG_suspend(handle_lxb, &lgd->lgd_wait_mini, async_status);
	    LG_unmutex(&lxb->lxb_mutex);
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(LG_MUST_SLEEP);
	}
    }

    /* 
    ** Check if the database the transaction running against is inconsistent.
    ** Note: only allow recovery process to write the dm0l_et log record for 
    ** the transactions running on a inconsistent database.
    */
    if ((ldb->ldb_status & LDB_INVALID) && (lxb->lxb_status & LXB_PROTECT))
    {
	/* flag both the handle and shared LXBs */
	lxb->lxb_status |= LXB_DBINCONST;
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	if ( handle_lxb != lxb )
	{
	    handle_lxb->lxb_status |= LXB_DBINCONST;
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	}
	return(LG_DB_INCONSISTENT);
    }
	
    /*
    ** Check for various stall conditions:
    **
    **   - Opendb pending: this indicates that the database is being opened
    **     by the current process and the RCP has not yet processed the open
    **     request.  Write operations on the database are stalled until the
    **     the open request is complete.
    **
    ** LDB_OPENDB_PEND is, unfortunately, overloaded. Normally it means that
    ** the database is awaiting confirmation from the RCP that it may be
    ** opened, and all log writes are stalled until that occurs. However, the
    ** database can also go into opend_pend case if it is newly-readded to the
    ** logging system at a time when it is currently being recovered. This
    ** is done to stall new updates while we complete the recovery. This is
    ** sort of a "re-open pending" state. In this state, we must allow
    ** recovering transactions to write their log records, but new transactions
    ** should be stalled.
    */
    if (ldb->ldb_status & LDB_OPENDB_PEND)
    {
	/* Lock the LDB, check again */
	if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
	{
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}
	if (ldb->ldb_status & LDB_OPENDB_PEND &&
	   (handle_lxb->lxb_status & LXB_ABORT) == 0)
	{
	    handle_lxb->lxb_wait_reason = LG_WAIT_OPENDB;
	    LG_suspend(handle_lxb, &lgd->lgd_open_event, async_status);
	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_MUST_SLEEP);
	}
	(VOID)LG_unmutex(&ldb->ldb_mutex);
    }	

    /*
    ** If log space reservation is being piggybacked onto this call,
    ** check for LOGFULL. This normally would have been done by a previous
    ** call to LGreserve().
    **
    ** Note that with this piggyback function, it's not necessary to
    ** adjust lxb_reserved_space or lfb_reserved_space - we know
    ** exactly how much we'll use.
    */
    reserved_space = 0;
    if (*flag & LG_NOT_RESERVED)
    {
	LG_check_logfull(lfb, lxb);
    }
    else if ((handle_lxb->lxb_status & LXB_NORESERVE) == 0)
    {
	/* Calculate the reserved log space used by this log record */
	reserved_space = LG_calc_reserved_space(lxb, 1, 
				log_space_basis, 0);
    }


    /*
    ** Check for various stall conditions:
    **
    **   - BCP Stall: this indicates that a Consistency Point is in progress
    **     and the RCP is in the middle of writing out a description of the
    **     current LG state into a group of BCP records.  These BCP records
    **     must occur contiguously in the logfile so we briefly stall all log
    **     writes until the BCP is complete.
    **
    **	 - If this is a piggbacked LG_NOT_RESERVED, we may be in a LOGFULL state:
    **
    **     o if we have reached the LOGFULL limit, we need to stall until more
    **       logspace is made available.
    **     o if we have reached the cpstall limit during a Consistency Point
    **       (generally 90% of the force_abort limit) then we need to stall until
    **       the CP is finished. This condition looks identical to normal LOGFULL
    **       to this routine.
    **
    **       The RCP is never stalled from the above conditions.
    **       The ACP and aborting transactions are allowed to continue during LOGFULL
    **       since they actually free up space rather then using it.
    */
    if ( lgd->lgd_status & LGD_STALL_MASK && !(lpb->lpb_status & LPB_MASTER) )
    {
	/*
	** Lock the status and check it again.
	*/
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	{
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}
	if ( lgd->lgd_status & LGD_BCPSTALL )
	{
	    handle_lxb->lxb_wait_reason = LG_WAIT_BCP;
	    LG_suspend(handle_lxb, &lgd->lgd_wait_stall, async_status);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_MUST_SLEEP);
	}
	else if (*flag & LG_NOT_RESERVED &&
	       (lpb->lpb_status & LPB_ARCHIVER) == 0 &&
	       (handle_lxb->lxb_status & LXB_ABORT) == 0)
	{
	    /* Log file is full and we must wait */
	    handle_lxb->lxb_wait_reason = LG_WAIT_LOGFULL;
	    LG_suspend(handle_lxb, &lgd->lgd_wait_stall, async_status);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_MUST_SLEEP);
	}
	LG_unmutex(&lgd->lgd_mutex);
    }

    if (lxb->lxb_status & LXB_INACTIVE)
    {
	/*
	**  If this is the first record for a protected transaction, then
	**  move from the inactive to the active queue.  A protected
	**  transaction causes the archiver to stall reading past the
	**  transactions records until the transaction has committed or
	**  aborted.  Only transaction on the active queue will impede
	**  the archiver.  Non-protected transaction are used by the recovery
	**  system and the archiver to log short term information that
	**  is protected by the latest consistency point, or for system
	**  transactions that do not need recovery protection.
	*/

	if (lxb->lxb_status & LXB_PROTECT)
	{
	    /*
	    ** Make sure that the first record written is a Begin Trans record.
	    ** 
	    ** Let MASTER do whatever it wants - it may write End transaction
	    ** records for transactions it is aborting.
	    **
	    ** Also let NONPROTECT transaction get away with not writing a BT
	    ** record as they write special records not usually associated
	    ** with transaction protocols (CP's, Opendb's).
	    **
	    ** Note: rollforwarddb also takes advantage of this so that it
	    ** can log rollback records like the RCP.  It would probably
	    ** be better to have a single xact attribute that marked a recovery
	    ** operation.
	    */
	    if (((*flag & LG_FIRST) == 0) &&
		((lpb->lpb_status & LPB_MASTER) == 0))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		uleFormat(NULL, E_DMA407_LG_WRITE_NOT_FIRST, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, *flag, 0, lpb->lpb_status);
		return (LG_BADPARAM);
	    }

	    /*
	    ** Check for various stall conditions:
	    **
	    **   - Checkpoint pending: this indicates that an online checkpoint has
	    **     been signaled and the logging system is preventing transactions
	    **	   from becoming active until all other active transactions have
	    **	   completed.
	    **	
	    **	   Don't stall the checkpoint process from doing its online
	    **	   backup work (LPB_CKPDB).
	    **
	    **	   Return LG_CKPDB_STALL status to the LGwrite() caller. Upon
	    **	   receiving that status, they will make a SHR lock request
	    **	   on LK_CKP_TXN for this database.
	    **
	    **     CKPDB has either been granted or has requested an EXCL lock
	    **	   on LK_CKP_TXN for this database. When it is ready to unstall
	    **	   it will release the EXCL lock, allowing these stalled log
	    **	   writers to run.
	    **
	    **	   By utilizing locks instead of LGsuspend, all players become
	    **	   visible to locking's deadlock detection, and if a deadlock
	    **	   develops around CKPDB, it will be discovered and an 
	    **	   offending transaction aborted to free up the deadlock.
	    **
	    **	   Note that this check is also made in LGreserve. Unless this
	    **	   is a "LG_NOT_RESERVED" call, this check should already
	    **	   been done.
	    */
	    if ( ldb->ldb_status & LDB_BACKUP &&
		 ldb->ldb_status & LDB_STALL &&
		(lpb->lpb_status & LPB_CKPDB) == 0 )
	    {
		/* Lock the LDB, check again */
		if (status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex))
		{
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    if ( handle_lxb != lxb )
			(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		    return(status);
		}
		if (ldb->ldb_status & LDB_BACKUP &&
		    ldb->ldb_status & LDB_STALL)
		{
		    ldb->ldb_status |= LDB_CKPDB_PEND;
		    handle_lxb->lxb_status |= LXB_CKPDB_PEND;
		    (VOID)LG_unmutex(&ldb->ldb_mutex);
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    if ( handle_lxb != lxb )
			(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		    return (LG_CKPDB_STALL);
		}
		(VOID)LG_unmutex(&ldb->ldb_mutex);
	    }

	    /*
	    **  Insert (shared) LXB onto the end of the active queue and
	    **  into the active transaction hash queue.
	    */
	    {
		LXBQ	*bucket_array;

		bucket_array = (LXBQ *)LGK_PTR_FROM_OFFSET(
				    lgd->lgd_lxh_buckets);
		
		lxbq = &bucket_array[lxb->lxb_lxh.lxh_tran_id.db_low_tran
				% lgd->lgd_lxbb_size];

		lxb->lxb_status &= ~LXB_INACTIVE;
		handle_lxb->lxb_status &= ~LXB_CKPDB_PEND;
		lxb->lxb_status |= LXB_ACTIVE;

		if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lxb_q_mutex))
		{
		    (VOID)LG_unmutex(&lxb->lxb_mutex);
		    if ( handle_lxb != lxb )
			(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		    return(status);
		}
	    
		lxb->lxb_next = LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);
		lxb->lxb_prev = lgd->lgd_lxb_prev;
		prev_lxb = (LXB *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxb_prev);
		prev_lxb->lxb_next =
		    lgd->lgd_lxb_prev = LGK_OFFSET_FROM_PTR(lxb);


		/* count another active, protected txn for ckpdb */
		lgd->lgd_protect_count++;
		ldb->ldb_lxbo_count++;

		/* Insert LXB on the tail of active transaction hash queue */
		lxb->lxb_lxh.lxh_lxbq.lxbq_next = 
			LGK_OFFSET_FROM_PTR(&lxbq->lxbq_next);
		lxb->lxb_lxh.lxh_lxbq.lxbq_prev = lxbq->lxbq_prev;
		prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
		prev_lxbq->lxbq_next = lxbq->lxbq_prev = 
			LGK_OFFSET_FROM_PTR(&lxb->lxb_lxh.lxh_lxbq);

		(VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	    }
	}
    }

    /*
    ** Note: lfb_current_lbb's lbb_mutex (which we'll soon hold)
    **	     is used to protect the following fields:
    **
    **		lfb_header.lgh_end
    **		lgh_last_lsn
    **		lfb_archive_window_end
    **		ldb_j_last_la
    **	 	ldb_d_last_la
    **
    **	     lgd_mutex protects:
    **
    **	   	lfb_archive_window_start
    **	 	lfb_archive_window_prevcp
    **		lfb_header.lgh_begin
    **		lfb_header_lgh_cp
    **		ldb_j_first_la
    **		ldb_d_first_la
    */

    /*
    ** Initialize the dump and journal windows if they are not yet set.
    ** The start window addresses are set using the current eof value.
    ** NOTE THAT THIS IS THE EOF BEFORE THE NEW RECORD IS WRITTEN and
    ** will not be the same value as the end window address which will
    ** be set below to the new eof after the record is written.
    */
    /*
    ** If the database is journaled and its current journal window is
    ** not set, then initialize the start value (the end value will
    ** be updated below).
    */
    if ( (lxb->lxb_status & LXB_JOURNAL && ldb->ldb_j_first_la.la_block == 0)
       || (ldb->ldb_status & LDB_BACKUP && ldb->ldb_d_first_la.la_block == 0) )
    {
	LG_LA	lgh_end_now;

	/*
	** Ensure we use a consistent value for log EOF.
	** 
	** Current LBB's mutex must be taken to ensure that
	** the log eof address is consistent and not in the
	** midst of a log split.
	**
	** Also, better make certain that the lbb mutex we get is
	** for the CURRENT lbb. It could have moved on.
	*/
	while (lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb))
	{
	    if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	        return(status);
	    }

 	    if ( lbb == (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb) )
	    {
		break;
            }

	    /* The lbb is not the CURRENT LBB, so release the mutex and repeat. To get
	    ** the Log EOF, we must hold the lbb_mutex of the current lbb.
	    */
	    (VOID)LG_unmutex(&lbb->lbb_mutex);

#ifdef xDEBUG
	    TRdisplay( "LG_write():: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
		      lfb->lfb_header.lgh_end.la_sequence,
		      lfb->lfb_header.lgh_end.la_block,
		      lfb->lfb_header.lgh_end.la_offset);
#endif
        }

	lgh_end_now = lfb->lfb_header.lgh_end;
	(VOID)LG_unmutex(&lbb->lbb_mutex);

	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	{
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}

	/* Recheck after waiting for lgd_mutex */

	/*
	** We may have waited a "long" time for the mutex during which time
	** another thread also in this bit of code ran, got a later EOF value
	** and stuffed it into ldb_j_first, ldb_d_first, and/or 
	** lfb_archive_window_start. Now they won't be zero, but we must
	** establish the lowest LGA for each container or the ACP might throw
	** E_DM9845/46 errors.
	*/
	if ( lxb->lxb_status & LXB_JOURNAL && 
		(ldb->ldb_j_first_la.la_block == 0 ||
		 LGA_LT(&lgh_end_now, &ldb->ldb_j_first_la)) )
	    ldb->ldb_j_first_la = lgh_end_now;

	/*
	** If the database is undergoing online backup, then this record needs
	** to be included in the current dump window.  Check if the dump
	** window beginning mark has been set.
	*/
        if ( ldb->ldb_status & LDB_BACKUP &&
		(ldb->ldb_d_first_la.la_block == 0 ||
		 LGA_LT(&lgh_end_now, &ldb->ldb_d_first_la)) )
	    ldb->ldb_d_first_la = lgh_end_now;


	/*
	** We have just set the database journal or dump windows. If the
	** system archive window is not initialized, then set the system
	** archive window start and also store the address of the latest
	** consistency point.  This CP address marks the spot past which the
	** Log File BOF cannot be moved forward without first performing
	** archiver processing.
	*/
	if ( lfb->lfb_archive_window_start.la_block == 0 ||
		 LGA_LT(&lgh_end_now, &lfb->lfb_archive_window_start) )
	{
	    lfb->lfb_archive_window_start = lgh_end_now;
	    lfb->lfb_archive_window_prevcp = lfb->lfb_header.lgh_cp;
	}
	(VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    /*
    **  Get the current buffer.  If there is no current buffer,     
    **  get one from the free queue. If there are none on the
    **  free queue, wait for one to appear.
    */
    lxb->lxb_rlbb_count = 0;

    for (;;)
    {
	lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb);
	    
	if ((lbb->lbb_state & LBB_CURRENT) == 0)
	{
	    /* If successful, returns with lbb_mutex locked, LBB_CURRENT set.
	    ** If *not* successful, but no error, returns with
	    ** lfb_fq_mutex held.
	    */
	    if (status = LG_allocate_lbb(lfb, &lxb->lxb_rlbb_count, 
				    lxb, &lbb))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return(status);
	    }

	    /* Short on buffers? */
	    if (lbb == (LBB *)NULL)
	    {
		handle_lxb->lxb_wait_reason = LG_WAIT_FREEBUF;
		LG_suspend(handle_lxb, &lfb->lfb_wait_buffer, async_status);
		(VOID)LG_unmutex(&lfb->lfb_fq_mutex);
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return (LG_MUST_SLEEP);
	    }
	    /* New current is mutexed */
	}
	else
	{
	    /* Mutex the apparent current buffer */
	    if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return(status);
	    }
	}

	if (lbb->lbb_state & LBB_CURRENT)
	    break;

	/* No longer the current buffer, try again */
	(VOID)LG_unmutex(&lbb->lbb_mutex);
    }

    /* Save the current EOF value (before the LG_write was done) */
    lgh_end_now = lfb->lfb_header.lgh_end;

    /* Decide if record fits into the current page. */

    span_bytes = size_lr - (lfb->lfb_header.lgh_size-LG_BKEND_SIZE - lbb->lbb_bytes_used);

    if (span_bytes > 0)
    {
	/* 
	**  Record will span multiple pages. If there are no free buffers, 
	**  then wait for the free buffer(s).
	**
	**  Check for free buffers. When we span a log record across pages,
	**  the pages must be contiguous pages. Therefore, once we start
	**  building this record into a sequence of logfile pages, we want
	**  to ensure that there are enough free log pages currently available
	**  to contain the entire log record.
	**
	**  This means at least one more log page must be free; two more if the
	**  page size is 4096; four more if the page size is 2048
	*/

	/*
	** Compute the number of additional LBBs we'll need 
	** to contain this record.
	*/
	lxb->lxb_rlbb_count = span_bytes / (lfb->lfb_header.lgh_size-LG_BKEND_SIZE -
					sizeof(LBH));
	lxb->lxb_rlbb_count += (span_bytes % (lfb->lfb_header.lgh_size-LG_BKEND_SIZE -
					sizeof(LBH)))
			              ? 1 : 0;

	/* Lock the free queue */
	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
	{
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return(status);
	}

	if (lxb->lxb_rlbb_count > lfb->lfb_fq_count)
	{
	    /*	Wait for free buffers to continue split log record.
	    ** The initial buffer we got can just stay CURRENT;  we
	    ** did nothing (else) to it yet.  Upon resume we'll start
	    ** over from the beginning.
	    */
	    lxb->lxb_rlbb_count = 0;
	    handle_lxb->lxb_wait_reason = LG_WAIT_SPLIT;
	    LG_suspend(handle_lxb, &lfb->lfb_wait_buffer, async_status);
	    (VOID)LG_unmutex(&lfb->lfb_fq_mutex);
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	    return (LG_MUST_SLEEP);
	}
	/*
	** Reserve the number of additional buffers we need
	** by decrementing the number of free buffers by
	** that amount.
	** Note that lbb (lfb_current_lbb) is still our
	** "primary" LBB and will be used first and is not
	** included in the "reserved" count.
	*/
	lfb->lfb_fq_count   -= lxb->lxb_rlbb_count;

	(VOID)LG_unmutex(&lfb->lfb_fq_mutex);

	/* Leave current_lbb marked LBB_FORCE for GCMT */

	/*	Update split counters. */
	lgd->lgd_stat.split++;
	lxb->lxb_stat.split++;
	lfb->lfb_stat.split++;
    }

    /*
    **  Build log record header into the current buffer.
    **  There is always room for a header in the current log buffer.
    */
    lrh = (LRH *)LGK_PTR_FROM_OFFSET(lbb->lbb_next_byte);
    lrh->lrh_length = size_lr;
    lrh->lrh_extra = 0;
    lrh->lrh_prev_lga = lxb->lxb_last_lga;
    lrh->lrh_sequence = ++lxb->lxb_sequence;

    lbb->lbb_bytes_used += sizeof(LRH);
    lbb->lbb_next_byte += sizeof(LRH);

    /*
    ** Compute LSN for this record.  This computation is currently made based
    ** on the Log Address which will be assigned to the record but is not
    ** dependent on it being the same or similar in value.
    **
    ** WE ARE NOW USING THE START OF THE LOG RECORD AS THE LSN RATHER THAN
    ** THE END OF THE LOG RECORD TO ENSURE THAT IT HAS A DIFFERENT VALUE
    ** THAN THE LOG ADDRESS.  Soon, we expect that the VAXcluster worker bees
    ** will completely diassociate these values.  (Until this is done, there
    ** is actually a window of vulnerability because we are still doing log
    ** forces using the LSN value, and the LSN being returned here is less
    ** than the log address of the log record - so if we: 1) write a log
    ** record which spans pages & 2) the buffer manager decides to force
    ** the updated page out of the cache & 3) the last log buffer has not
    ** been forced yet & 4) we finish forcing the 2nd to last log buffer &
    ** 5) we write out the updated page & 6) the last log buffer has still
    ** not been written out & 7) we crash : then we will end up with a recovery
    ** problem because the page will not be in the expected state).
    */

    /*
    ** Presumption: lfb_header "last" LGA/LSN are only updated here while
    **		    the current LBB's mutex is held EXCL, there is only one
    **		    current LBB, therefore that mutex is a sufficient block
    **		    to prevent concurrent changes to lfb_header.
    */

    dm0l_header = (DM0L_HEADER *) objects[0].lgo_data;

    if (lgd->lgd_status & LGD_CLUSTER_SYSTEM)
    {
	/* Preserver lsn_last_lsn in case of error */
	dm0l_header->lsn = lfb->lfb_header.lgh_last_lsn;
	status = LGlsn_next(&lfb->lfb_header.lgh_last_lsn, sys_err);
	if (status)
	{
	    /* Restore dubious preserved value */
	    lfb->lfb_header.lgh_last_lsn = dm0l_header->lsn;
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    /*
	    ** What? return (status); ? Must do some cleanup first, no?
	    */
	}
    }
    else
    {
	if (dm0l_header->type == DM0LOPENDB &&
		(dm0l_header->flags & DM0L_SPECIAL) &&
		    LSN_LT(&lfb->lfb_header.lgh_last_lsn, &dm0l_header->lsn))
	{
	    LG_LSN	tmsec_lsn;

	    tmsec_lsn.lsn_high = TMsecs();
	    tmsec_lsn.lsn_low = 0;

	    /* Can't move installation lsn past TMsecs! */
	    if (LSN_LTE(&dm0l_header->lsn, &tmsec_lsn))
	    {
		TRdisplay("DM0LOPENDB DM0L_SPECIAL advance Last LSN (%x,%x) to (%x,%x) tmsecs %x\n", 
		lfb->lfb_header.lgh_last_lsn.lsn_high,
		lfb->lfb_header.lgh_last_lsn.lsn_low,
		dm0l_header->lsn.lsn_high, dm0l_header->lsn.lsn_low,
		tmsec_lsn.lsn_high);

		STRUCT_ASSIGN_MACRO(dm0l_header->lsn, lfb->lfb_header.lgh_last_lsn);
	    }
	}

	if(++lfb->lfb_header.lgh_last_lsn.lsn_low == 0)
	    ++lfb->lfb_header.lgh_last_lsn.lsn_high;
    }

    /*
    ** Return the LSN of the record being written to the caller. 
    ** Also record this LSN in the caller's log record header, 
    ** and save the LSN in the LXB structure for information tracking purposes.
    */
    lxb->lxb_last_lsn = lri->lri_lsn = dm0l_header->lsn = lfb->lfb_header.lgh_last_lsn;

    /* Record LSN of last write to this DB for MVCC */
    ldb->ldb_last_lsn = lri->lri_lsn;

    /*
    ** Now we're done possibly waiting on stuff.
    */

    /*
    ** Update external log record header with transaction, database,
    ** and lxb_id.id_id.
    **
    ** Note that this lg_id will be that of the SHARED LXB, not the
    ** caller's handle, when shared log transactions are being used.
    **
    ** Each log record written by the server contains a DM0L_HEADER object
    ** as its first element.
    **
    ** This header is different than the LG record header (LRH) filled in just
    ** above.  Each log record is preceded by an LRH in the log file, but the
    ** LRH is not actually part of the log record.
    */
    dm0l_header->database_id = ldb->ldb_database_id;
    dm0l_header->tran_id = lxb->lxb_lxh.lxh_tran_id;
    dm0l_header->lg_id = lri->lri_lg_id = lxb->lxb_id.id_id;

    /* Ok, done filling out the dm0l_header and lri now */

    /* Record first LSN in this buffer */
    if (lbb->lbb_first_lsn.lsn_high == 0)
	lbb->lbb_first_lsn = lri->lri_lsn;

    /* Record the LSN/LGA of the txn's BT record */
    if (lxb->lxb_first_lsn.lsn_high == 0)
    {
	lxb->lxb_first_lsn = lri->lri_lsn;

	if ( handle_lxb != lxb )
	    /* Copy to handle LXB, just for info */
	    handle_lxb->lxb_first_lsn = lxb->lxb_first_lsn;

	/*
	** Record the log record address of the xact's BT record.
	*/
	lxb->lxb_first_lga = lfb->lfb_header.lgh_end;

	/*
	** Save the address of the last Consistency Point.  This marks the
	** spot past which the Log File BOF cannot be moved forward while
	** this transaction remains active.
	*/
	lxb->lxb_cp_lga = lfb->lfb_header.lgh_cp;
    }

    /*
    ** One last bit of work to to while we hold current_lbb mutex:
    **
    ** If MVCC-eligible and COMMIT, record the commit LSN
    ** as ldb_last_commit, remove transaction from the LDB's active list.
    **
    ** If first write of the transaction, place the transaction on the
    ** LDB's active list.
    **
    ** If journaling, simulate a journal write
    ** placing into the lri the projected jnl file sequence 
    ** and block offset of where this log record might end up when 
    ** it's eventually journaled.
    **
    ** We do this while holding the ldb_mutex to keep LGalter(LG_A_JFIB)
    ** and LGshow CRIB refreshes at bay.
    */
    if ( ldb->ldb_status & LDB_MVCC
	    &&
	    ((dm0l_header->flags & DM0L_JOURNAL &&
	      ldb->ldb_status & LDB_JOURNAL)
	      ||
	      LSN_EQ(&lxb->lxb_first_lsn, &lri->lri_lsn)
	      ||
             (*flag & LG_LAST || dm0l_header->type == DM0LET)) )
    {
	if ( status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex) )
	    return(status);

	/* If commit, record commit LSN in ldb_last_commit for MVCC */
	if ( *flag & LG_LAST || dm0l_header->type == DM0LET )
	{
	    ldb->ldb_last_commit = lri->lri_lsn;
	    
	    /*
	    ** It may take a while for the ET to hit the disk,
	    ** but in MVCC-terms, this transaction is no longer active
	    ** as of right now, and we don't want others about to
	    ** refresh their CRIBs thinking this one is still "active"
	    */

	    /* Set xid slot "inactive" (0) in xid array */
	    SET_XID_INACTIVE(lxb);

	    /*
	    ** Remove LXB from LDB's active queue, if on it.
	    ** It should be, but we check anyway.
	    */
	    if ( lxb->lxb_active_lxbq.lxbq_next != 0 )
	    {
		next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_active_lxbq.lxbq_next);
		prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_active_lxbq.lxbq_prev);
		next_lxbq->lxbq_prev = lxb->lxb_active_lxbq.lxbq_prev;
		prev_lxbq->lxbq_next = lxb->lxb_active_lxbq.lxbq_next;

		lxb->lxb_active_lxbq.lxbq_next = 
			lxb->lxb_active_lxbq.lxbq_prev = 0;
	    }

	}
	/* else, if first write of txn... */
	else if ( LSN_EQ(&lxb->lxb_first_lsn, &lri->lri_lsn) )
	{
	    /* Insert LXB last on LDB's list of active transactions */
	    lxbq = &ldb->ldb_active_lxbq;
	    lxb->lxb_active_lxbq.lxbq_next = 
		    LGK_OFFSET_FROM_PTR(&lxbq->lxbq_next);
	    lxb->lxb_active_lxbq.lxbq_prev = lxbq->lxbq_prev;
	    prev_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
	    prev_lxbq->lxbq_next = lxbq->lxbq_prev =
		LGK_OFFSET_FROM_PTR(&lxb->lxb_active_lxbq);
	}

	/*
	** If this is a journaled log record, simulate
	** a journal write in the JFIB
	*/
	if ( dm0l_header->flags & DM0L_JOURNAL && ldb->ldb_status & LDB_JOURNAL )
	{
	    jfib = &ldb->ldb_jfib;
	    jfib_size = dm0l_header->length;

	    /*
	    ** Return the anticipated journal file sequence and block
	    ** of the beginning of this log record in the LRI
	    **
	    ** Assume at least some of this record will fit on the current block.
	    */
	    lri->lri_jfa = jfib->jfib_jfa;

	    if ( jfib->jfib_bytes_left < sizeof(i4) )
	    {
		/* Record will start on next block */
		lri->lri_jfa.jfa_block++;
	    }

	    /* Entire log record is guaranteed to fit in a journal file */
	    while ( jfib_size )
	    {
		if ( jfib->jfib_bytes_left > jfib_size ) 
		{
		    jfib->jfib_next_byte += jfib_size;
		    jfib->jfib_bytes_left -= jfib_size;
		    jfib_size = 0;
		}
		else
		{
		    /* Fit what we can, then start another page */
		    if ( jfib->jfib_bytes_left >= sizeof(i4) )
			jfib_size -= jfib->jfib_bytes_left;

		    jfib->jfib_jfa.jfa_block++;
		    jfib->jfib_next_byte = sizeof(DM0J_HEADER);
		    jfib->jfib_bytes_left = jfib->jfib_bksz - sizeof(DM0J_HEADER);
		}
	    }
	}

	(VOID)LG_unmutex(&ldb->ldb_mutex);
    }
    else
    {
        lri->lri_jfa.jfa_filseq = 0;
	lri->lri_jfa.jfa_block = 0;
    }

    /*
    **	Add record to buffer.  The record could span multiple buffers
    **	and we could run out of buffers before the last byte of the record
    **	has made it into the buffer.  This loop handles these special
    **	requirements.
    */
    obj_num = 0;
    obj_bytes_left = objects[0].lgo_size;
    obj_ptr = (char *)objects[0].lgo_data;

    span_lbb = lbb;

    while (span_lbb->lbb_bytes_used + move_amount > 
    		lfb->lfb_header.lgh_size-LG_BKEND_SIZE)
    {
	/*
	**  Record will span multiple pages. Place the next part 
	**  of the record on the current page, then utilize 
	**  our "reserved" buffers for the remainder.  
	**
	**  Note that we must retain the EXCL lock on the current buffer's mutex
	**  until the entire record has been written to prevent other
	**  threads from interleaving their log records in our 
	**  "reserved" buffers (the spanned record must be written
	**  in contiguous space).
	*/

	avail = lfb->lfb_header.lgh_size-LG_BKEND_SIZE - span_lbb->lbb_bytes_used;
	move_amount -= avail;
	if (avail > record_length)
	    avail = record_length;
	record_length -= avail;

	span_lbb->lbb_bytes_used = lfb->lfb_header.lgh_size-LG_BKEND_SIZE;
	
	/*
	** Update the EOF (while holding current LBB's mutex).
	**
	** This must be done here because LG_allocate_lbb(),
	** below, needs it to assign the next buffer's LGA.
	*/

	lfb->lfb_header.lgh_end.la_sequence = span_lbb->lbb_lga.la_sequence;
	lfb->lfb_header.lgh_end.la_block = span_lbb->lbb_lga.la_block;
	lfb->lfb_header.lgh_end.la_offset = span_lbb->lbb_bytes_used;

	/* Copy objects that will fit completely into buffer */
	buf_ptr = (char *)LGK_PTR_FROM_OFFSET(span_lbb->lbb_next_byte);

	while (obj_bytes_left <= avail)
	{
	    MEcopy((PTR)obj_ptr, obj_bytes_left, (PTR)buf_ptr);

	    buf_ptr += obj_bytes_left;
	    avail -= obj_bytes_left;
	    if (++obj_num < num_objects)
	    {
		obj_bytes_left = objects[obj_num].lgo_size;
		obj_ptr = (char *)objects[obj_num].lgo_data;
	    }
	    else
	    {
		/* 
		** All objects fit. All that remains is
		** the bookend record-size word which
		** will be placed into the next buffer.
		*/
		obj_bytes_left = avail = 0;
		break;
	    }
	}

	/* Move as much of the last object as will fit */
	if (avail > 0)
	{
	    MEcopy((PTR)obj_ptr, avail, (PTR)buf_ptr);

	    obj_bytes_left -= avail;
	    obj_ptr += avail;
	}

	/*
	** Queue this LBB to the write queue.
	** If it's the first buffer of the span ("current_lbb"),
	** it won't get written yet, an it's lbb_mutex will
	** be retained to block other threads from allocating
	** a new current until the "span" is complete.
	*/
	if (span_lbb == lbb)
	{
	    /* lbb_forced_lsn is the last complete record on the page */
	    span_lbb->lbb_forced_lsn = span_lbb->lbb_last_lsn;
	    /* lbb_last_lsn is the last LSN to appear on the page */
	    span_lbb->lbb_last_lsn = lri->lri_lsn;

	    LG_queue_write(span_lbb, lpb, handle_lxb, KEEP_LBB_MUTEX);
	}
	else
	{
	    /* lbb_first/last_lsn already set to lri_lsn, below */
	    /* lbb_forced_lsn is intentionally left zero */
	    LG_queue_write(span_lbb, lpb, handle_lxb, 0);
	}

	/*
	**  Current lbb has been queued, allocate one of our
	**  reserved LBBs to continue.
	**
	**  If there were a bug, and there were no LBB at this point, then we'll
	**  crash on the assignment to lbb->lbb_first_lsn. This is not
	**  graceful, but crashing the installation is probably the only legal
	**  thing we can do if we get partway through a spanned log record and
	**  are unable to complete the write.
	*/

	/*
	** Call LG_allocate_lbb() to grab one of our reserved LBBs
	** of off the free queue, returning with span_lbb 
	** pointing to that buffer.
	**
	** If successful, it'll decrement lxb_rlbb_count by one.
	**
	** Note that we still hold the original "current" LBB's mutex
	** and it's still the LBB referenced by lfb_current_lbb, blocking
	** all other writers.
	*/
	status = LG_allocate_lbb(lfb, &lxb->lxb_rlbb_count,
			lxb, &span_lbb);
	if (status != OK || span_lbb == NULL)
	{
	    TRdisplay("%@ PANIC!! LG_allocate_lbb() failed! status %d\n",status);
	    CS_breakpoint();
	    /*
	    ** This is not good, but handle it as best we can
	    ** by returning the remaining reserved LBBs to the
	    ** free queue before we return with the error.
	    ** LG-allocate-lbb returned holding the fq mutex.
	    */
	    if (status == OK)
	    {
		/* Non error, but no free buffer */
		lfb->lfb_fq_count   += lxb->lxb_rlbb_count;
		LG_resume(&lfb->lfb_wait_buffer, lfb);
		(VOID)LG_unmutex(&lfb->lfb_fq_mutex);
	    }
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    if ( handle_lxb != lxb )
		(VOID)LG_unmutex(&handle_lxb->lxb_mutex);

	    return(status);
	}

	span_lbb->lbb_first_lsn = span_lbb->lbb_last_lsn = lri->lri_lsn;
    } /* while record is too big */

    /* 
    ** "span_lbb" is now "lfb_current_lbb". 
    **
    ** If this log record spanned more than one buffer, the
    ** first buffer of the series ("lbb") is still mutexed,
    ** the intermediate buffers queued for write and unmutexed,
    ** and the last buffer ("span_lbb") is also still mutexed.
    */


    /*
    **	The remaining portion of the record fits in the current
    **	page.  Copy the remaining part of the record, 
    **  then see if we can write it.
    */

    buf_ptr = (char *)LGK_PTR_FROM_OFFSET(span_lbb->lbb_next_byte);

    /*
    ** At the end of the record we place the record's size in bytes.
    ** This allows us to easily read the logfile both backwards and
    ** forwards.
    */
    rec_end = span_lbb->lbb_next_byte + move_amount - sizeof(i4);
    size_ptr = (i4 *)LGK_PTR_FROM_OFFSET(rec_end);

    *size_ptr = size_lr;

    /* Bump the next_byte and bytes_used in the LBB. */
    span_lbb->lbb_next_byte += move_amount; 
    span_lbb->lbb_bytes_used += move_amount;

    /*	Calculate the final lga of this record, update system-wide EOF */

    lfb->lfb_header.lgh_end.la_sequence = span_lbb->lbb_lga.la_sequence;
    lfb->lfb_header.lgh_end.la_block = span_lbb->lbb_lga.la_block;
    lfb->lfb_header.lgh_end.la_offset = span_lbb->lbb_lga.la_offset +
	(span_lbb->lbb_bytes_used - sizeof(i4));

    /* Update last LGA written by transaction */
    lxb->lxb_last_lga = lfb->lfb_header.lgh_end;

    /* Return LGA of this record in the caller's LRI */
    lri->lri_lga = lxb->lxb_last_lga;
    /* ...and the id of the buffer containing it */
    lri->lri_bufid = span_lbb->lbb_id;

    /* 
    ** If first write to the DB, record this LGA as
    ** "first_lga", as there can't be any MVCC log records
    ** of interest before this. This is usually the LGA of
    ** an OPENDB record.
    */
    if ( ldb->ldb_first_la.la_block == 0 )
        ldb->ldb_first_la = lri->lri_lga;

    /* Update last LGA in log buffer header */
    lbh = (LBH *)LGK_PTR_FROM_OFFSET(span_lbb->lbb_buffer);
    lbh->lbh_address = lri->lri_lga;

    /*
    ** Update the database and system journal and dump windows.
    */
    if (lxb->lxb_status & LXB_JOURNAL)
    {
	ldb->ldb_j_last_la = lri->lri_lga;
	lfb->lfb_archive_window_end = lri->lri_lga;
    }
    if (ldb->ldb_status & LDB_BACKUP)
    {
	ldb->ldb_d_last_la = lri->lri_lga;
	lfb->lfb_archive_window_end = lri->lri_lga;
    }

    /* The last complete LSN to appear on this page */
    span_lbb->lbb_last_lsn = lri->lri_lsn;
    span_lbb->lbb_forced_lsn = lri->lri_lsn;

    if (lbb != span_lbb)
    {
	/*
	** The last span_lbb is now the new current,
	** release the original current for writing
	** and release its mutex, 
	** unblocking lfb_current_lbb waiters.
	**
	** We had to wait to release the old "current"
	** until the log EOF was finally computed
	** for the last chunk of a "span".
	*/
	LG_queue_write(lbb, lpb, handle_lxb, 0);

	/* From here on, "span_lbb" is our LBB */
	lbb = (LBB*)NULL;
    }

    /*
    ** Now we've "reserved" space in the buffer. 
    ** Bump the count of transactions actively writing 
    ** in this buffer and release the mutex to allow
    ** other threads access to the LBB while we do the copy.
    ** The buffer will get written when all writers have
    ** completed their copies.
    */
    span_lbb->lbb_writers++;
    
    /*
    ** If there's less than a LRH-sized piece left, the
    ** buffer is "full enough" and must be written.
    ** Given that we're about to release the current_lbb_mutex
    ** while we copy the record data into the buffer, we
    ** don't want another thread to assume that there's
    ** room for a LRH in this already full buffer, so
    ** queue it for write, inserting it onto the write queue
    ** and removing it as the current buffer.
    */
    if (span_lbb->lbb_bytes_used + sizeof(LRH) > 
    	lfb->lfb_header.lgh_size-LG_BKEND_SIZE)
    {
	/*
	**  Queue buffer to write queue.
	**
	**  Note that LG_queue_write() will just put the buffer
	**  on the write queue but won't write it because 
	**  lbb_writers is not zero.
	**
	**  The buffer's LBB_CURRENT status will be reset, though
	**  lfb_current_lbb will continue to point to this buffer.
	**  Threads blocked on lbb_mutex will awaken, notice LBB_CURRENT
	**  has been reset, and will attempt a new LBB allocation.
	*/
	LG_queue_write(span_lbb, lpb, handle_lxb, 0);
    }
    else
	/* Still room, let other threads have at it */
	(VOID)LG_unmutex(&span_lbb->lbb_mutex);

    /* Copy what's left in the current object first, then the rest */
    if (obj_bytes_left > 0)
    {
	MEcopy((PTR)obj_ptr, obj_bytes_left, (PTR)buf_ptr);

	buf_ptr += obj_bytes_left;
    }
    for (obj_num++; obj_num < num_objects; obj_num++)
    {
	MEcopy((PTR)objects[obj_num].lgo_data, objects[obj_num].lgo_size,
		   (PTR)buf_ptr);

	buf_ptr += objects[obj_num].lgo_size;
    }


    /*  Change various counters. */
    lgd->lgd_stat.write++;
    lxb->lxb_stat.write++;
    ldb->ldb_stat.write++;
    lfb->lfb_stat.write++;

    /* If first write for this handle, count another writer */
    if ( handle_lxb != lxb && (handle_lxb->lxb_stat.write++) == 0 )
	lxb->lxb_handle_wts++;

    /*
    ** In case we calculated wrong, return any unused
    ** LBBs reserved for spanning to the free queue
    ** and wake up any waiters on buffers.
    */
    if (lxb->lxb_rlbb_count)
    {
	(VOID)LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex);
	lfb->lfb_fq_count   += lxb->lxb_rlbb_count;
	LG_resume(&lfb->lfb_wait_buffer, lfb);
	(VOID)LG_unmutex(&lfb->lfb_fq_mutex);
    }
	
    /*
    ** If we have reserved space to give back, do it now.
    ** If this txn is ending (LG_LAST), give up all remaining
    ** space reserved by the txn. This avoids another mutex
    ** and update of lfb_reserved_space in LGend().
    */
    if (reserved_space || *flag & LG_LAST)
    {
	/* 
	** Reduce txn's log file reserved space.
	**
	** Check for more log space being used than was actually reserved.
	** This can lead to logfull hanging problems if more space is needed
	** in restart recovery than was actually prereserved.
	**
	** We log an error in this case, but continue with the expectation
	** that everything will probably be OK.  Even if we have not reserved
	** enough space, we are likely still not at a critical logfull
	** condition.
	**
	** (Note that returning an error in this condition is probably the
	** worst thing we could do since logspace reservation bugs will
	** likely only lead to problems if the transaction aborts).
	**
	** The above is no longer completely true ; due to attempts to account
	** for 'used but empty' logfile space, the calculation of reserved 
	** space may drift either direction between the original calculation
	** and the actual write ( typically by a couple of bytes ).
	*/

	if (reserved_space)
	{
	    if (lxb->lxb_reserved_space >= reserved_space)
	    {
		lxb->lxb_reserved_space -= reserved_space;
	    }
	    else
	    {
		if (((lpb->lpb_status & LPB_MASTER) == 0) &&
		    ((reserved_space / 2) > lxb->lxb_reserved_space))
		{
		    /* Note the u_i8's passed as pointers for errmsg */
		    uleFormat(NULL, E_DMA490_LXB_RESERVED_SPACE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lxb->lxb_lxh.lxh_tran_id.db_high_tran,
			0, lxb->lxb_lxh.lxh_tran_id.db_low_tran,
			sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space,
			sizeof(lxb->lxb_reserved_space), &lxb->lxb_reserved_space,
			0, lxb->lxb_sequence,
			sizeof(reserved_space), &reserved_space);
		}
		lxb->lxb_reserved_space = 0;
	    }
	}

	/* If txn is ending, give back all excess reserved space */
	if (*flag & LG_LAST)
	{
	    reserved_space += lxb->lxb_reserved_space;
	    lxb->lxb_reserved_space = 0;
	}

	if (reserved_space)
	{
	    /* lfb_mutex blocks concurrent update of lfb_reserved_space */
	    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
	    {
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		if ( handle_lxb != lxb )
		    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
		return(status);
	    }
	    if (lfb->lfb_reserved_space >= reserved_space)
	    {
		lfb->lfb_reserved_space -= reserved_space;
	    }
	    else
	    {
		if (((lpb->lpb_status & LPB_MASTER) == 0) &&
		    ((reserved_space / 2) > lfb->lfb_reserved_space))
		{
		    uleFormat(NULL, E_DMA491_LFB_RESERVED_SPACE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
			sizeof(lfb->lfb_reserved_space), &lfb->lfb_reserved_space);
		}
		lfb->lfb_reserved_space = 0;
	    }
	    (VOID)LG_unmutex(&lfb->lfb_mutex);
	}
    }

    /*
    ** By now the "current" LBB may have been queued for
    ** write by another thread or is "full enough", in which 
    ** case its state of LBB_CURRENT has been reset.
    ** We bumped lbb_writers so LG_queue_write() hasn't 
    ** actually written it yet and the buffer is on the  
    ** write queue waiting to be written.
    */
    if (status = LG_mutex(SEM_EXCL, &span_lbb->lbb_mutex))
    {
	(VOID)LG_unmutex(&lxb->lxb_mutex);
	if ( handle_lxb != lxb )
	    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
	return(status);
    }
	    
    /*
    ** If we need to wait for the buffer to hit the disk,
    ** attach ourselves to this buffer's wait queue.
    ** If the log record spanned over more than one buffer,
    ** this buffer is the last of those buffers. Semantics in
    ** write_complete() guarantee that we'll wait until
    ** all of the spanned buffers' I/O completes.
    */

    if (*flag & LG_FORCE)
    {
	/*
	** If writing the ET (commit), txn must wait for all of
	** its log pages to hit the disk.
	**
	** (Don't rely solely on "LG_LAST" to signal the ET, also
	**  check the log record directly. Under some conditions
	**  (RCP) LG_LAST may not be present when the ET is
	**  being written.)
	**
	** Otherwise, it need only wait for the current
	** LSN's pages to hit the disk.
	*/
	if ( *flag & LG_LAST || dm0l_header->type == DM0LET )
	{
	    handle_lxb->lxb_wait_reason = LG_WAIT_LASTBUF;
	    handle_lxb->lxb_wait_lsn = lxb->lxb_first_lsn;
	    span_lbb->lbb_commit_count++;

	    /* Note that the ET has been written */
	    lxb->lxb_status |= LXB_ET;
	}
	else
	{
	    handle_lxb->lxb_wait_reason = LG_WAIT_FORCED_IO;
	    handle_lxb->lxb_wait_lsn = lxb->lxb_last_lsn;
	}
	    
	LG_suspend_lbb(handle_lxb, span_lbb, async_status);
			
	/* We'll have to wait for I/O completion */
	return_status = OK;
    }
    else
	/* Not LG_FORCE, no need to wait for I/O */
	return_status = LG_SYNCH;

    /* Release the LXB */
    (VOID)LG_unmutex(&lxb->lxb_mutex);
    if ( handle_lxb != lxb )
	(VOID)LG_unmutex(&handle_lxb->lxb_mutex);

    /* 
    **  Remove ourselves as a writer in this buffer.
    **
    **  If no more writers and the buffer's been placed
    **  on the write queue by us or another thread,
    **  call LG_queue_write() to 
    **  perform or schedule the write.
    */
    if (--span_lbb->lbb_writers == 0 && 
	span_lbb->lbb_state & LBB_ON_WRITE_QUEUE)
    {
	/*
	**  Write or schedule the writing of this buffer,
	**  releasing lbb_mutex in the process.
	*/
	return_status = LG_queue_write(span_lbb, lpb, handle_lxb, 0);
    }
    else if ( *flag & LG_FORCE )
    {
	/*
	** Mark the current buffer with force write request (unless already
	** done by a previous force call).  The buffer will
	** be written out when it becomes full or when the timer
	** has expired.
	**
	** If there are less active (protected) transactions than
	** lgd->lgd_gcmt_threshold in the logging system, then queue the 
	** log write immediately rather waiting for piggyback commit.
	** Usually lgd->lgd_gcmt_threshold is set to at least 1 since there
	** is no use in doing group commit if there is only one write 
	** transaction active.
	**
	** Other values of lgd->lgd_gcmt_threshold may provide better
	** overall throughput through the system at the cost of more 
	** log I/O of less full log I/O buffers.
	**
	** The group commit threads in the system are all bound to the
	** local log, so we only initiate group commit if this page is
	** a local log file page. Non-local logfile access (cluster recovery
	** access) never uses group commit.
	**
	** When selecting a group commit thread to oversee the buffer during
	** group commit, we pick the thread in this process if this process
	** has a group commit thread; otherwise we pick the R.CP's group
	** commit thread. The primitive nature of this selection algorithm
	** means that there may be times when we decide not to initiate
	** group commit for a particular buffer even though group commit
	** threads exist in the logging system (just neither in this
	** process nor in the RCP). To ensure that group commit is always
	** used, the RCP should be started with a group commit thread.
	*/
	/* 
	** Whether already marked for FORCE or not, 
	** if all potential writers to the buffer (active, protected
	** transactions) are now waiting for it to be written, 
	** write the buffer now rather than waiting for
	** the group commit thread to wake up and notice.
	**
	** If the logging system is stalled waiting for a BCP
	** to be written, then this must be master trying to
	** write the BCP (it's the only LGwrite allowed to
	** proceed when stalled on a BCP), so write it.
	**
	** If running with optimized writes and a single logwriter,
	** then don't use group commit - it kills performance.
	*/
	if (
	    (lgd->lgd_state_flags & LGD_OPTIMWRITE &&
	     lgd->lgd_lwlxb.lwq_count == 1) ||
	    span_lbb->lbb_wait_count >= 
		lgd->lgd_protect_count - span_lbb->lbb_commit_count ||
	    lgd->lgd_status & LGD_BCPSTALL
	   )
	{
	    return_status = LG_queue_write(span_lbb, lpb, handle_lxb, 0);
	}
	else if ((span_lbb->lbb_state & LBB_FORCE) == 0)
	{
	    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);

	    /*
	    ** If this is the local log file, and if there exists a group
	    ** commit thread able to process this buffer, and if there are
	    ** at least lgd_gcmt_threshold number of protected transactions,
	    ** then initiate group commit processing for this buffer.
	    */
	    if (lgd->lgd_protect_count > lgd->lgd_gcmt_threshold &&
		lfb == &lgd->lgd_local_lfb &&
		(lpb->lpb_gcmt_lxb != 0 || master_lpb->lpb_gcmt_lxb != 0))
	    {
		SIZE_TYPE	gcmt_offset;

		span_lbb->lbb_state |= LBB_FORCE;

		(VOID)LG_unmutex(&span_lbb->lbb_mutex);

		if ((gcmt_offset = lpb->lpb_gcmt_lxb) == 0)
		     gcmt_offset = master_lpb->lpb_gcmt_lxb;

		/* Awaken the GCMT thread, if it's asleep */
		while (gcmt_offset)
		{
		    LXB	*gcmt_lxb = (LXB*)LGK_PTR_FROM_OFFSET(gcmt_offset);
		    if (status = LG_mutex(SEM_EXCL, &gcmt_lxb->lxb_mutex))
		    {
			(VOID)LG_unmutex(&lxb->lxb_mutex);
			if ( handle_lxb != lxb )
			    (VOID)LG_unmutex(&handle_lxb->lxb_mutex);
			return(status);
		    }
		    if (gcmt_lxb->lxb_status & LXB_WAIT)
		    {
			gcmt_lxb->lxb_status &= ~LXB_WAIT;

			LGcp_resume(&gcmt_lxb->lxb_cpid);
			gcmt_offset = 0;
		    }
		    else
		    {
			/* In the RCP, these will be the same */
			if ( gcmt_offset == lpb->lpb_gcmt_lxb &&
			     gcmt_offset != master_lpb->lpb_gcmt_lxb )
			{
			    gcmt_offset = master_lpb->lpb_gcmt_lxb;
			}
			else
			    gcmt_offset = 0;
		    }
		    (VOID)LG_unmutex(&gcmt_lxb->lxb_mutex);
		}
	    }
	    else
	    {
		/* LG_queue_write will free lbb_mutex */
		return_status = LG_queue_write(span_lbb, lpb, handle_lxb, 0);
	    }
	}
	else
	    (VOID)LG_unmutex(&span_lbb->lbb_mutex);
    }
    else
	(VOID)LG_unmutex(&span_lbb->lbb_mutex);

    if ( *flag & LG_FORCE )
    {
	/*
	** Must wait for this page to hit the disk.
	**
	** We may have written it ourselves synchronously, in which
	** case we don't need to wait (LG_SYNCH), or may have given
	** it to a gcmt or logwriter thread to write. By now, gcmt
	** or logwriter -may- have completed all necessary I/O and set 
	** return_status to LG_SYNCH; if not yet completed, return_status
	** will be equal to OK (must wait).
	*/
	if (*flag & LG_LAST)
	{
	    /* ET. If I/O complete, end the transaction. */
	    if ( return_status == LG_SYNCH )
	    {
		/*
		** LG_end_tran will free the LXB, 
		** hence the lxb_mutex
		*/
		LG_mutex(SEM_EXCL, &handle_lxb->lxb_mutex);
		(VOID)LG_end_tran(handle_lxb, *flag & LG_LOGFULL_COMMIT);
	    }
	    else
	    {
		/*
		** After this page hits disk, we want to make one last pass through
		** LG_write to call LG_end_tran(). Setting *flag to LG_LAST is the
		** mechanism for triggering this.
		*/
		/* Off all -but- LOGFULL_COMMIT */
		*flag &= LG_LOGFULL_COMMIT;
		/* ... add LG_LAST */
		*flag |= LG_LAST;
		return_status = LG_MUST_SLEEP;
	    }
	}
    }
    else
	/* No I/O wait needed */
	return_status = LG_SYNCH;

    /*
    ** Before you exit, check the dump and journal windows again.
    ** The Archiver was processing using the ldb_j_end_la which you just
    ** changed, and may have reset the window again.
    **
    ** Bad form: Taking the lgd_mutex unconditionally here creates an
    **		 all-thread block. Rather, dirty-check for a mutex need
    **		 first, then take it and check again.
    */

    if ( (lxb->lxb_status & LXB_JOURNAL && ldb->ldb_j_first_la.la_block == 0)
       || (ldb->ldb_status & LDB_BACKUP && ldb->ldb_d_first_la.la_block == 0) )
    {
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	if ( (lxb->lxb_status & LXB_JOURNAL && ldb->ldb_j_first_la.la_block == 0)
	   || (ldb->ldb_status & LDB_BACKUP && ldb->ldb_d_first_la.la_block == 0) )
	{
	    if (lfb->lfb_archive_window_start.la_block == 0)
	    {
		lfb->lfb_archive_window_start = lgh_end_now;
		lfb->lfb_archive_window_prevcp = lfb->lfb_header.lgh_cp;
	    }
	}
	(VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    return (return_status);
}

/*{
** Name: LG_queue_write	- Queue write request to writer.
**
** Description:
**      This routine queues an LBB to the write queue and awakens a logwriter
**	thread in the appropriate process. If no logwriter thread is available,
**	the buffer is queued and will be written once an already started I/O
**	completes.
**
**	When possible, the buffer will be written by this thread instead of 
**	waiting for a logwriter to do it.
**
** Inputs:
**      lbb                             The lbb to queue.
**	lpb				Which process is making this call.
**	lxb				Which txn is making this call (optional)
**					If we decide to write the buffer immediately
**					and there is no furthur I/O that the txn
**					needs to wait on and it's placed itself
**				    	on this buffer's wait queue, it'll be 
**					removed and LXB_WAIT reset.
**	flags				If KEEP_LBB_MUTEX, the 
**					buffer's lbb_mutex will not be
**					released, otherwise it will.
**
** Outputs:
**	Returns:
**	    OK 		If write not yet complete
**	    LG_SYNCH	Write has completed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-oct-1993 (rmuth)
**	    Prototype.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  Added release_mutex argument to write_block().
**	01-Feb-1996 (jenjo02)
**	    If the LBB we're asked to queue is lfb_current_lbb,
**	    clear lfb_current_lbb and free lfb_current_lbb_mutex.
**	13-Mar-1996 (jenjo02)
**	    Moved incrementing of lpb_stat.write to caller of write_block()
**	    from LG_write() to better track writes by process.
**      06-mar-1996 (stial01 for bryanp)
**          Correctly cast to (u_i2), not (i2), when setting lbh_used.
**	29-Apr-1996 (jenjo02)
**	    Added "release_curr_lbb_mutex" parm. If TRUE,
**	    release that mutex before queueing the write.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from write_block().
**	19-Feb-1999 (jenjo02)
**	    Insert buffer onto write queue, even if it's not ready to
**	    be written just yet, so that LGforce() can find it if 
**	    need be.
**	17-Mar-2000 (jenjo02)
**	    Return STATUS insted of VOID, OK if buffer write not yet
**	    complete, LG_SYNCH if it is.
**	29-Sep-2000 (jenjo02)
**	    Corrected computation and accumulation of Kbytes 
**	    written stat.
**	13-Aug-2001 (jenjo02)
**	    Leave LBB_FORCE flag on until buffer is ready to be
**	    written so GCMT can have a better chance of writing it.
**	03-Jan-2003 (jenjo02)
**	    Mutex lbb_mutex while calling write_block() to protect
**	    lbb_status change by write_block() (BUG 104569).
**	02-Jul-2003 (jenjo02)
**	    B110520. Set lbb_forced_lsn/lga only when initially
**	    installing LBB on the write queue. Once on the write queue
**	    the LBB "forced" values may be modified if the buffer
**	    is picked as "gprev" by another thread.
**	21-Mar-2006 (jenjo02)
**	    Moved LGchk_sum here from write_block.
**	06-Jul-2010 (jonj) SIR 122696
**	    Write la_sequence in last 4 bytes of non-header pages,
**	    lbh_used changed from u_i2 to i4.
*/
STATUS
LG_queue_write(
    LBB *lbb, 
    LPB *lpb,
    LXB *lxb,
    i4  flags)
{
    LGD			*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LFB			*lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);
    LBB			*next_lbb;
    LBB			*prev_lbb;
    LBH			*lbh;
    i4		err_code;
    STATUS		status, async_status;
    CL_ERR_DESC		sys_err;
    i4			bucket;
    bool		write_buf;

    LG_WHERE("LG_queue_write")

    /* The buffer's lbb_mutex must be locked */

    /*
    ** This buffer cannot be left in limbo. For LGforce to be 
    ** able to find it, it must be either the current buffer
    ** or appear on the write queue.
    */
    
    /*	Insert buffer onto the tail of the write queue, if not already */
    if ((lbb->lbb_state & LBB_ON_WRITE_QUEUE) == 0)
    {
	/* Set/update the forced LSN/LGA if not a log header */
	if (lbb->lbb_lga.la_block)
	{
	    lbh = (LBH *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
	    lbb->lbb_forced_lga = lbh->lbh_address;

	    /* Update lbh, histogram and stats */
	    lbh->lbh_used = lbb->lbb_bytes_used;

	    /* Bookend la_sequence on the end of the page */
	    *(u_i4*)((char*)lbh + lfb->lfb_header.lgh_size-LG_BKEND_SIZE) = 
		lbh->lbh_address.la_sequence;

	    if ( (lfb->lfb_stat.bytes += lbb->lbb_bytes_used) >= 1024 )
	    {
		lgd->lgd_stat.kbytes += lfb->lfb_stat.bytes / 1024;
		lfb->lfb_stat.kbytes += lfb->lfb_stat.bytes / 1024;
		lfb->lfb_stat.bytes = lfb->lfb_stat.bytes % 1024;
	    }
	    /*
	    ** Increment appropriate buffer-utilization histogram
	    */
	    bucket = (lbb->lbb_bytes_used * 10) / lfb->lfb_header.lgh_size;
	    if (bucket == 10)
		bucket--;
	    lgd->lgd_stat.buf_util[bucket]++;
	    lgd->lgd_stat.buf_util_sum++;
	}

	/* Lock the write queue */
	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex))
	    return(status);

	lbb->lbb_next =  LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);
	lbb->lbb_prev = lfb->lfb_wq_prev;
	prev_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_wq_prev);
	prev_lbb->lbb_next =
	    lfb->lfb_wq_prev = LGK_OFFSET_FROM_PTR(lbb);

	if (++lfb->lfb_wq_count > lgd->lgd_stat.max_wq)
	{
	    lgd->lgd_stat.max_wq = lfb->lfb_wq_count;
	    lgd->lgd_stat.max_wq_count = 1;
	}
	else if (lfb->lfb_wq_count == lgd->lgd_stat.max_wq)
	    lgd->lgd_stat.max_wq_count++;

	(VOID)LG_unmutex(&lfb->lfb_wq_mutex);

	lbb->lbb_state |=  LBB_ON_WRITE_QUEUE;
    }

    /*
    ** If there are active writers in the buffer,
    ** or lbb_mutex is not to be released, return
    */
    if (lbb->lbb_writers || flags & KEEP_LBB_MUTEX)
    {
	if ((flags & KEEP_LBB_MUTEX) == 0)
	{
	    lbb->lbb_state &= ~(LBB_CURRENT | LBB_FORCE);
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	}
	return(OK);
    }


    /* The buffer is on the write queue and ready to be written */

    /* Put checksum on non-header pages */
    if (lbb->lbb_lga.la_block)
    {
	lbh = (LBH *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
	lbh->lbh_checksum = LGchk_sum((PTR)lbh,
				      lfb->lfb_header.lgh_size);

    }

    /* Remove buffer from GCMT's watchful eye */
    lbb->lbb_state &= ~(LBB_CURRENT | LBB_FORCE);
    

    /*
    ** Figure out if we'll write this block to one or two log files, and set
    ** the lbb_resume_cnt appropriately.
    */
    if (lfb->lfb_status & LFB_DUAL_LOGGING)
    {
	lbb->lbb_resume_cnt = 2;
	lbb->lbb_state |= (LBB_PRIM_IO_NEEDED | LBB_DUAL_IO_NEEDED);
    }
    else if (lbb->lbb_lga.la_block == 0 &&
	     (lfb->lfb_status & LFB_DISABLE_DUAL_LOGGING) != 0)
    {
	/*
	** we're writing the log file header because we're in the midst of
	** disabling dual logging, so go ahead and write the header to both log
	** files. This helps to ensure that if we crash and then restart, we
	** are able to examine EITHER header and learn the state of the two log
	** files.
	*/
	lbb->lbb_resume_cnt = 2;
	lbb->lbb_state |= (LBB_PRIM_IO_NEEDED | LBB_DUAL_IO_NEEDED);
    }
    else
    {
	/*
	** We're only writing to one log file, either because this is a
	** non-header page or because dual logging is completely disabled.
	*/
	lbb->lbb_resume_cnt = 1;
	if (lfb->lfb_active_log == LGD_II_LOG_FILE)
	    lbb->lbb_state |= LBB_PRIM_IO_NEEDED;
	else
	    lbb->lbb_state |= LBB_DUAL_IO_NEEDED;
    }

    /*
    ** Always give the buffer to a logwriter if there is ONE logwriter in the
    ** installation.
    **
    ** Write the buffer immediately if:
    **
    **  o This is a MT installation and
    **	  this LXB is waiting on the buffer
    **	  or is an internal (inactive) thread
    **	  (GCMT, for example).
    **    - or -
    **  o there are no logwriter threads
    **	o LFB_USE_DIIO is indicated
    **
    ** In all other cases, queue the buffer to a 
    ** logwriter thread.
    */
    if (
	  ( LG_is_mt && lgd->lgd_lwlxb.lwq_count != 1
	    && lxb && lxb->lxb_status & (LXB_WAIT_LBB | LXB_INACTIVE) )
        ||
	  (lgd->lgd_lwlxb.lwq_count == 0 || lfb->lfb_status & LFB_USE_DIIO)
       )
    {
	write_buf = TRUE;
    }
    else
    {
	/*
	** Assign buffer to a logwriter thread.
	*/
	write_buf = FALSE;

        assign_logwriter(lgd, lbb, lpb);

	/* 
	** INACTIVE transaction could be the RCP doing CP processing
        ** Since other transactions are stalled (lgd->lgd_status & LGD_STALL_MASK)
        ** Do the write if we couldn't assign and the logwriter won't see it
	*/
	if (lgd->lgd_lwlxb.lwq_count == 1
	    && (lbb->lbb_state & LBB_PRIMARY_LXB_ASSIGNED) == 0
	    && lxb && (lxb->lxb_status & LXB_INACTIVE) )

	{
	    LXBQ *lwq;
	    LXB *lwlxb;

	    lwq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lwlxb.lwq_lxbq.lxbq_next);
	    lwlxb = (LXB *)((char *)lwq - CL_OFFSETOF(LXB,lxb_wait));
	    if (lwlxb->lxb_assigned_buffer == 0)
		write_buf = TRUE;
	}

	if (!write_buf)
	{
	    /* Release the buffer and threads blocked on its mutex */
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	    async_status = OK;
	}
    }

    if (write_buf)
    {
	u_i4	local_lbb_instance = lbb->lbb_instance;
	i4	local_lbb_state = lbb->lbb_state;

	if (lbb->lbb_state & LBB_PRIM_IO_NEEDED)
	{
	    lbb->lbb_state |= LBB_PRIMARY_LXB_ASSIGNED;
	    /* Show the LXB which is writing this buffer (informational only ) */
	    if (lxb)
		lbb->lbb_owning_lxb = LGK_OFFSET_FROM_PTR(lxb);
	    lpb->lpb_stat.write++;

	    /* write_block() will release lbb_mutex */
	    status = write_block(lbb, (i4)1, lxb, &async_status, &sys_err);
	    if (status)
	    {
		(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
		PCexit(FAIL);
	    }
	    /* Retake the mutex for dual check, if dual possible */
	    if ( local_lbb_state & LBB_DUAL_IO_NEEDED )
		(VOID)LG_mutex(SEM_EXCL, &lbb->lbb_mutex);
	}
	/* A LW thread may have already picked buffer for dual */
	if ( local_lbb_state & LBB_DUAL_IO_NEEDED &&
	     local_lbb_instance == lbb->lbb_instance &&
	    (lbb->lbb_state & (LBB_DUAL_IO_NEEDED | LBB_DUAL_LXB_ASSIGNED))
		== LBB_DUAL_IO_NEEDED )
	{
	    lbb->lbb_state |= LBB_DUAL_LXB_ASSIGNED;
	    /* Show the LXB which is writing this buffer (informational only ) */
	    if (lxb)
		lbb->lbb_owning_lxb = LGK_OFFSET_FROM_PTR(lxb);
	    lpb->lpb_stat.write++;

	    /* write_block() will release lbb_mutex */
	    status = write_block(lbb, (i4)0, lxb, &async_status, &sys_err);
	    if (status)
	    {
		(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
		PCexit(FAIL);
	    }
	}
	else if ( local_lbb_state & LBB_DUAL_IO_NEEDED )
		(VOID)LG_unmutex(&lbb->lbb_mutex);
    }


    return(async_status);
}

/*
** Name: LG_pgyback_write   -- Group Commit Protocol
**
** Description:
**	This routine is called by the group commit thread to implement the
**	group commit protocol.
**
** Inputs:
**	transaction_id		Transaction ID for this group commit thread.
**
** Outputs:
**	sys_err			reason for error return
**
** Returns:
**	OK			Situation normal, call back soon
**	LG_NO_BUFFER		no group commit buffer is active, go to sleep.
**	other			something is wrong
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	20-oct-1992 (bryanp)
**	    Pick up group commit numticks from the lgd_gcmt_numticks value.
**	26-apr-1993 (bryanp)
**	    Note that the group commit threads are always connected only to the
**	    local LFB. Even though there may be non-local LFB's in use during
**	    multi-node cluster recovery, these LFB's never participate in
**	    group commit. The LFB used in this routine always turns out to be
**	    the lgd_local_lfb.
**	18-oct-1993 (rmuth)
**	    Prototype.
**	26-Jan-1996 (jenjo02)
**	    Mutex granularity project. New semaphores to augment
**	    lone lgd_mutex.
**	28-Jun-1996 (jenjo02)
**	    Added pgyback_write_time, pgyback_write_idle stats to aid
**	    in determining what phenomena drives gcmt.
**	08-Oct-1996 (jenjo02)
**	    Lock gcmt's lpb_mutex while looking at timer_lbb to avoid
**	    missing gcmt buffers.
**	25-Feb-1998 (jenjo02)
**	    Use gcmt's LXB instead of LPB as mutexing device.
**	    Moved ms sleep loop here from dmcgcmt to simplify the interface
**	    and control over sleeps/mutexes.
*/
STATUS
LG_pgyback_write(
    LG_LXID transaction_id, 
    i4  timer_interval_ms,
    CL_ERR_DESC *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LBB    	    *lbb;
    register LXB    *lxb;
    register LFB    *lfb;
    LG_I4ID_TO_ID   lx_id;
    STATUS	    status;
    LPD		    *lpd;
    LPB		    *lpb;
    SIZE_TYPE	    *lxbb_table;
    SIZE_TYPE	    lbb_offset;
    i4	    bytes_used;	
    i4		    num_ticks = 0;

    CL_CLEAR_ERR(sys_err);

    LG_WHERE("LG_pgyback_write")

    /*	Check the lx_id. */

    lx_id.id_i4id = transaction_id;
    if (lx_id.id_lgid.id_id == 0 || (i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
    {
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
    {
	return (LG_BADPARAM);
    }

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

    for (;;)
    {
	/* As long as there's a current LBB to watch... */
	lbb_offset = lfb->lfb_current_lbb;
	lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);

	if ((lbb->lbb_state & LBB_FORCE) == 0)
	{
	    /* No timer lbb to watch. Go back to a long sleep */
	    if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
		return(status);
		
	    /* Did a new current appear while we waited for mutex? */
	    if (lbb_offset != lfb->lfb_current_lbb ||
		lbb->lbb_state & LBB_FORCE)
	    {
		num_ticks = 0;
		LG_unmutex(&lxb->lxb_mutex);
		continue;
	    }
	    
	    lxb->lxb_status |= LXB_WAIT;
	    lxb->lxb_wait_reason = LG_WAIT_EVENT;
	    lxb->lxb_stat.wait++;
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    return(LG_NO_BUFFER);
	}

	/* If first peek at this buffer, save the starting bytes used */
	if (num_ticks == 0)
	    bytes_used = lbb->lbb_bytes_used;

	/* Sleep a tick, then check for activity in the buffer */
	CSms_thread_nap(timer_interval_ms);

	/* If, after waiting, the buffer is gone, start over */
	if (lbb_offset != lfb->lfb_current_lbb ||
	    (lbb->lbb_state & LBB_FORCE) == 0)

	{
	    num_ticks = 0;
	    continue;
	}

	/*
	** Take a dirty look at the buffer.
	** If there are active writers in the buffer, or
	** the buffer's changed since we last looked at it,
	** or we haven't watched it long enough,
	** sleep another tick.
	*/
	if (lbb->lbb_writers)
	    continue;

	num_ticks++;
	
	if ( bytes_used != lbb->lbb_bytes_used
	     && num_ticks < lgd->lgd_gcmt_numticks 
	   )
	{
	    bytes_used = lbb->lbb_bytes_used;
	    continue;
	}

	/*  
	** Queue write of the current page if there are force write requests
	** pending. 
	*/

	/*
	** Check if the timer lbb is still the current lbb.  If it is not, then
	** somebody else has already queued the write of the timer lbb (presumably
	** another session filled up the block causing it to be placed on
	** the write queue and a new current lbb to be allocated).  In this
	** case we will reset the timer lbb pointer to indiciate that there is
	** nobody waiting for piggyback_commit to write it. (Only the current_lbb
	** can be waiting for group commit - any time a buffer is removed from 
	** being the current_lbb, it is immediately queued to the write queue).
	**
	** When we enter here, a "tick" has already expired. If the
	** buffer hasn't changed in that interval, or if sufficient
	** ticks have elapsed, write it out.
	*/
	/*
	** Buffer was not modified during the last tick,
	** or we've waited too long for it to get written.
	**
	** Lock the current (timer) LBB and check it again.
	*/
	lgd->lgd_stat.pgyback_check++;

	if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
	    return(status);

	/* Still current and marked for force? */
	if ( lbb_offset == lfb->lfb_current_lbb &&
	     lbb->lbb_state & LBB_FORCE &&
	     lbb->lbb_state & LBB_CURRENT )
	{
	    if ( lbb->lbb_writers == 0 &&
		(bytes_used == lbb->lbb_bytes_used ||
		 num_ticks >= lgd->lgd_gcmt_numticks
		)
	       )
	    {
		lgd->lgd_stat.pgyback_write++;
		lxb->lxb_stat.write++;

		if (num_ticks >= lgd->lgd_gcmt_numticks)
		    lgd->lgd_stat.pgyback_write_time++;
		else
		    lgd->lgd_stat.pgyback_write_idle++;

		/*
		** LG_queue_write will reset LBB_CURRENT | LBB_FORCE
		** and free lbb_mutex.
		*/
		LG_queue_write(lbb, lpb, lxb, 0);
		num_ticks = 0;
	    }   
	    else
	    {
		/*
		** Buffer isn't ready to write yet. There are still
		** active writers in the buffer or the buffer's
		** changed while we waited for the mutex.
		** Wait a tick, then try again.
		*/
		bytes_used = lbb->lbb_bytes_used;
		(VOID)LG_unmutex(&lbb->lbb_mutex);
	    }
	}
	else
	{
	    /* 
	    ** The buffer we dirty-peeked at is no longer the
	    ** current lbb - it's been written. 
	    */
	    num_ticks = 0;
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	}
    }
}

/*{
** Name: write_block	- Write block to log file.
**
** Description:
**	This routine is called to write a block to the log file.
**
**	For automated testing purposes, this routine has the ability to
**	simulate write failures. This fault simulation works as follows:
**	    Automated testpoint tools cause LG to pick a block # and a file #.
**	    Then, the next time we write to that block, we simulate a data
**	    failure by scrambling the checksum which is written to that block.
**	    To simulate a power failure, we shut the system down by
**	    signalling LGD_IMM_SHUTDOWN. To simulate a partial write, we alter
**	    the size of the write.
**
**	If an I/O cannot be initiated (the SYS$QIO fails), we treat the error
**	as though there was an I/O error on the write. This is controversial:
**	errors in SYS$QIO are more likely due to resource exhaustion in the
**	process or the system (no virtual memory, no AST quota, etc.) than to
**	actual problems with the disk. However, it does simplify error
**	handling to treat the two classes of errors identically.
**
**	With the advent of logwriter threads and the ability of many processes
**	to be writing to the logfile simultaneously, there comes extra
**	complexity in this routine, especially in the case of dual logging
**	failure scenarios. As one part of this complexity, if we encounter
**	an error while writing to the disabled log, then we interpret that as
**	meaning, "the write was queued up to both logs, but this log has failed
**	and has already been disabled, so this I/O error is not unexpected and
**	no further action is required."
**
** Inputs:
**      MyLBB                           Log buffer block.
**	write_primary			write primary or dual?
**	lxb				Transaction info for the invoking
**					thread.
**
** Outputs:
**	Returns:
**	    sys_err			System-specific error status.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	21-oct-1992 (bryanp)
**	    Dual Logging bugfixes.
**	19-nov-1992 (bryanp)
**	    More dual logging bugfixes.
**	26-apr-1993 (bryanp)
**          Added dual logging testbed instrumentation.
**	19-jan-1994 (bryanp)
**	    B56706, B58260: If DIwrite fails, and we're dual logging, then let
**		the error be handled by write_complete, rather than causing the
**		logwriter thread to die. If we're dual logging, then once we
**		have copied the status code into lbb_io_status, we can reset
**		the "status" field to OK.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the 
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  Added "release_mutex" argument.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	31-jan-1994 (bryanp)
**	    B56474: Once an EMER_SHUTDOWN has been signaled, write no more
**		buffers.
**	    B56475: Emergency shutdowns are used by the dual logging testbed
**		which wants to simulate a system crash with the logfile
**		in a certain
**		state; in order to make this work, we must "freeze" the logfile
**		state by refusing to write any more.
**	    B56721: Pass write_primary to LG_check_write_test_1 so it can tell
**		if this is a dual write request or a primary write request.
**	    B56538: Pass write_primary to LG_check_write_test_2 so it can tell
**		if this is a dual write request or a primary write request.
**	    B56707: If write_block can't re-acquire the mutex, it should quit.
**	26-Jan-1996 (jenjo02)
**	    Mutex granularity project. 
**	    Set lbb_state to LBB_???_IO_INPROG while write is in progress.
**	05-Apr-1996 (jenjo02)
**	    To prevent simultaneous update of the LBH by dual_logging
**	    threads, lock the lbb's lbb_lbh_mutex.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from write_block().
**      26-Mar-2001 (hanal04, thaju02, horda03)
**          Added flags parameter which is passed on to write_complete().
**      30-Apr-2001 (wanfr01)
**          Bug 104569, INGSRV 1440
**          Protect lbb_needing_io so its state doesn't get overwritten,
**          which may result in it being prematurely written to disk.
**	03-Jan-2003 (jenjo02)
**	    lbb_mutex must be held on entry, will be released on return
**	    (BUG 104569).
**	21-Mar-2006 (jenjo02)
**	    Added notion of writing superblocks rather than a
**	    page-at-time if optimized writes are configured.
**	13-Sep-2007 (kibro01) b119055
**	    Check write_primary to determine type of mutex required
**	    rather than do_primary_write, as originally intended.
*/
static STATUS
write_block(
register LBB        *MyLBB,
i4		    write_primary,
LXB		    *lxb,
STATUS		    *async_status,
CL_ERR_DESC	    *sys_err)
{
    LGD		*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LFB		*lfb = (LFB *)LGK_PTR_FROM_OFFSET(MyLBB->lbb_lfb_offset);
    i4		size = 2048;
    STATUS	status;
    CL_ERR_DESC	local_sys_err;
    STATUS	dual_status;
    LBH		*lbh;
    i4		num_pages, num_written;
    bool        do_primary_write = TRUE;
    bool        do_dual_write = TRUE;
    bool        force_primary_io_failure = FALSE;
    bool        force_dual_io_failure = FALSE;
    i4          save_checksum;
    i4          save_size;
    bool        sync_write = FALSE;
    i4		pageno;
    i4		pagesize = lfb->lfb_header.lgh_size;
    i4		err_code;
    LG_IO	*di_io;
    i4		which_assigned, which_inprog, which_needed;
    i4		WqIsMutexed;
    LBB		*FirstLBB, *LastLBB, *CheckLBB;
    SIZE_TYPE	EndOfWriteQueue = LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);
    SIZE_TYPE	lxb_offset = (lxb) ? LGK_OFFSET_FROM_PTR(lxb)
				   : 0;

    LG_WHERE("write_block")
	
    if (lgd->lgd_master_lpb == 0 ||   /* master has already exited */
	lgd->lgd_status & LGD_EMER_SHUTDOWN)
    {
	(VOID)LG_unmutex(&MyLBB->lbb_mutex);
	return (LG_WRITEERROR);
    }

    if ( write_primary )
    {
	which_assigned = LBB_PRIMARY_LXB_ASSIGNED;
	which_inprog = LBB_PRIM_IO_INPROG;
	which_needed = LBB_PRIM_IO_NEEDED;
    }
    else
    {
	which_assigned = LBB_DUAL_LXB_ASSIGNED;
	which_inprog = LBB_DUAL_IO_INPROG;
	which_needed = LBB_DUAL_IO_NEEDED;
    }

    /* LBB_PRIM|DUAL_IO_INPROG keeps other threads away from LBB */
    MyLBB->lbb_state |= which_inprog;
    (VOID)LG_unmutex(&MyLBB->lbb_mutex);

    /*
    ** If configured for optimized writes (the default is to use
    ** them, trace point DM1442 can be used to toggle the
    ** setting), look for additional unassigned adjacent buffers 
    ** on both sides of MyLBB waiting to be written
    ** and create a superblock from them.
    **
    ** Note that CBF and LGalter disallow enabling write
    ** optimization for multi-partition log files.
    **
    ** At the end of this exercise, we'll have plucked out
    ** a range of contiguous log pages to write,
    **
    **		FirstLBB---->MyLBB---->LastLBB
    **
    ** which we'll write with one I/O, starting with FirstLBB 
    ** for "num_pages".
    **
    ** This presumes (rightly so) that the write queue
    ** is ordered low LGA --> high LGA and that adjacent
    ** LBBs and their pages are contiguous to each other.
    ** This order is established in the free queue and
    ** maintained by write_complete().
    **
    ** Note that only buffers that share the same "which_needed"
    ** "lbb_resume_count",and "lbb_lfb_offset" as "MyLBB" 
    ** are considered.
    **
    ** Note also that defining LG_DUAL_LOGGING_TESTBED will
    ** break this, so don't.
    */

    /* Center the page range */
    FirstLBB = LastLBB = MyLBB;
    num_pages = 0;

    if ( lxb && lfb->lfb_wq_count > 1 && 
	 lgd->lgd_state_flags & LGD_OPTIMWRITE )
    {
	LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);
	WqIsMutexed = TRUE;
	
	/* Find leftmost contiguous eligible buffer */
	while ( FirstLBB->lbb_prev != EndOfWriteQueue )
	{
	    /* There are some before us */
	    CheckLBB = (LBB*)LGK_PTR_FROM_OFFSET(FirstLBB->lbb_prev);

	    if ( CheckLBB->lbb_resume_cnt == MyLBB->lbb_resume_cnt &&
		 CheckLBB->lbb_lfb_offset == MyLBB->lbb_lfb_offset &&
	         CheckLBB->lbb_buffer + pagesize ==
			FirstLBB->lbb_buffer &&
	         CheckLBB->lbb_lga.la_block + 1 ==
			FirstLBB->lbb_lga.la_block &&
		   CheckLBB->lbb_state & which_needed &&
		 !(CheckLBB->lbb_state & which_assigned) )
	    {
		/*
		** Release the write queue. As buffers are
		** appended to the end of the write queue as
		** they become ready to be written, this lets
		** more be added as we wait for the lbb_mutex.
		** Of course, it also creates the opportunity
		** for buffers which we'd otherwise add to the
		** superblock to be stolen by a concurrent thread.
		*/
		LG_unmutex(&lfb->lfb_wq_mutex);
		WqIsMutexed = FALSE;
		LG_mutex(SEM_EXCL, &CheckLBB->lbb_mutex);
		if ( CheckLBB->lbb_resume_cnt == MyLBB->lbb_resume_cnt &&
		     CheckLBB->lbb_lga.la_block + 1 ==
			    FirstLBB->lbb_lga.la_block &&
		     CheckLBB->lbb_state & which_needed &&
		    !(CheckLBB->lbb_state & which_assigned) )
		{
		    /* It's ours now, look for more */
		    CheckLBB->lbb_state |= (which_inprog | which_assigned);
		    CheckLBB->lbb_owning_lxb = lxb_offset;
		    FirstLBB = CheckLBB;
		    num_pages++;
		    LG_unmutex(&CheckLBB->lbb_mutex);
		    LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);
		    WqIsMutexed = TRUE;
		}
		else
		{
		    /*
		    ** Buffer was stolen while we waited 
		    ** for its mutex, no point in continuing.
		    */
		    LG_unmutex(&CheckLBB->lbb_mutex);
		    break;
		}
	    }
	    else
		break;
	}

	if ( !WqIsMutexed )
	{
	    LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);
	    WqIsMutexed = TRUE;
	}

	/* Find rightmost contiguous eligible buffer */
	while ( LastLBB->lbb_next != EndOfWriteQueue )
	{
	    /* There are some after us */
	    CheckLBB = (LBB*)LGK_PTR_FROM_OFFSET(LastLBB->lbb_next);

	    if ( CheckLBB->lbb_resume_cnt == MyLBB->lbb_resume_cnt &&
		 CheckLBB->lbb_lfb_offset == MyLBB->lbb_lfb_offset &&
	         LastLBB->lbb_buffer + pagesize ==
			CheckLBB->lbb_buffer &&
	         LastLBB->lbb_lga.la_block + 1 ==
			CheckLBB->lbb_lga.la_block &&
		   CheckLBB->lbb_state & which_needed &&
		 !(CheckLBB->lbb_state & which_assigned) )
	    {
		/*
		** Release the write queue. As buffers are
		** appended to the end of the write queue as
		** they become ready to be written, this lets
		** more be added as we wait for the lbb_mutex.
		** Of course, it also creates the opportunity
		** for buffers which we'd otherwise add to the
		** superblock to be stolen by a concurrent thread.
		*/
		LG_unmutex(&lfb->lfb_wq_mutex);
		WqIsMutexed = FALSE;
		LG_mutex(SEM_EXCL, &CheckLBB->lbb_mutex);
		if ( CheckLBB->lbb_resume_cnt == MyLBB->lbb_resume_cnt &&
		     LastLBB->lbb_lga.la_block + 1 ==
			    CheckLBB->lbb_lga.la_block &&
		     CheckLBB->lbb_state & which_needed &&
		    !(CheckLBB->lbb_state & which_assigned) )
		{
		    /* It's ours now, look for more */
		    CheckLBB->lbb_state |= (which_inprog | which_assigned);
		    CheckLBB->lbb_owning_lxb = lxb_offset;
		    LastLBB = CheckLBB;
		    num_pages++;
		    LG_unmutex(&CheckLBB->lbb_mutex);
		    LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);
		    WqIsMutexed = TRUE;
		}
		else
		{
		    /*
		    ** Buffer was stolen while we waited 
		    ** for its mutex, no point in continuing.
		    */
		    LG_unmutex(&CheckLBB->lbb_mutex);
		    break;
		}
	    }
	    else
		break;
	}
	if ( WqIsMutexed )
	    LG_unmutex(&lfb->lfb_wq_mutex);
    }


    /* The physical write starts from FirstLBB for num_pages */
    lbh = (LBH *)LGK_PTR_FROM_OFFSET(FirstLBB->lbb_buffer);
    pageno = FirstLBB->lbb_lga.la_block;
    num_written = ++num_pages;

    /* Count each page as a page write */

    lgd->lgd_stat.writeio += num_pages;
    lfb->lfb_stat.writeio += num_pages;

    status = OK;

    /*
    ** Trace if superblock and tracing enabled.
    ** Trace point DM1443 toggles tracing.
    */
    if ( num_pages > 1 )
    {
	/* Tally the stats */
	lgd->lgd_stat.optimwrites++;
	lgd->lgd_stat.optimpages += num_pages;

	if ( lgd->lgd_state_flags & LGD_OPTIMWRITE_TRACE )
	    TRdisplay("%@ LG OptimWrite: %2d pages %cFirstLGA %d %cMyLGA %d LastLGA %d t@%d\n",
		    num_pages,
		    (FirstLBB != MyLBB) ? '*' : ' ',
		    FirstLBB->lbb_lga.la_block,
		    (FirstLBB == MyLBB) ? '*' : ' ',
		    MyLBB->lbb_lga.la_block,
		    LastLBB->lbb_lga.la_block,
		    CS_get_thread_id());
    }

#ifdef LG_DUAL_LOGGING_TESTBED
    LG_check_write_test_1( lgd, FirstLBB, write_primary, &size, &do_primary_write,
                            &do_dual_write, &force_primary_io_failure,
                            &force_dual_io_failure, &save_checksum,
                            &save_size, &sync_write);
#endif

    /*
    **	Issue the QIO to write the block, synchronously if this is the log file
    **	header in a dual-logging environment (or if we are simulating a data
    **	corruption error on the log and must ensure the I/O's are serial in
    **	order to ensure the damage is done to one log only.
    **
    **	For most 'normal' situations, the I/O is done asynchronously.
    **
    **  NB: the above comments are quite dated; all DI writes are
    **      synchronous.
    */


    if (write_primary && do_primary_write)
    {
	if (lfb->lfb_status & LFB_USE_DIIO)
	    di_io = &lfb->lfb_di_cbs[LG_PRIMARY_DI_CB];
	else
	{
	    di_io = &Lg_di_cbs[LG_PRIMARY_DI_CB];
        }

#ifdef LG_TRACE
	TRdisplay("write_block: writing primary block at %p with buffer %p lxb: %x checksum: %x\n           : lbh @ %p, pageno: %d, num_pages: %d\n",
			FirstLBB, FirstLBB->lbb_buffer, lxb, lbh->lbh_checksum,
			lbh, pageno, num_pages);
#endif

        if (force_primary_io_failure)
            status = FAIL;
        else
	{
	    status = LG_WRITE(di_io, &num_written,
			    pageno, (char *)lbh, sys_err);
	}

	if (status)
	{
	    uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	    uleFormat(NULL, E_DMA44F_LG_WB_BLOCK_INFO, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, "PRIMARY", 0, num_pages, 0, pageno, 0, lbh,
		    0, lfb->lfb_header.lgh_size,
		    0, FirstLBB->lbb_lga.la_offset);

	    /* Now continue on to perform I/O error processing */
	}

	if (status != OK)
	{
	    /*
	    ** An I/O error has occurred. If this log has already been disabled,
	    ** then all the necessary actions have already been taken and no
	    ** further error processing is needed.
	    */
	    if ((lfb->lfb_status & LFB_DUAL_LOGGING) == 0 &&
		(lfb->lfb_active_log == LGD_II_DUAL_LOG))
	    {
		status = OK;
	    }
	}

#ifdef LG_TRACE
	TRdisplay("write_block: LG_WRITE completed block at %p with buffer %p lxb %x\n           : lbh @ %p, pageno: %d, num_pages: %d\n",
			FirstLBB, FirstLBB->lbb_buffer, lxb,
			lbh, pageno, num_pages);
#endif
	FirstLBB->lbb_io_status = status;
	if (status == OK && num_pages != num_written)
	    FirstLBB->lbb_io_status = LG_WRITEERROR;

	if (lfb->lfb_status & LFB_DUAL_LOGGING)
	    status = OK;    /* B56706, B58260 -- ok to reset status now */

	status = write_complete(FirstLBB,
			lxb, (i4) 1, num_pages, async_status);
    }

#ifdef LG_DUAL_LOGGING_TESTBED
    LG_check_write_test_2( lgd, FirstLBB, write_primary,
			    &size, &save_checksum, &save_size );
#endif

    if (!write_primary && do_dual_write)
    {
	if (lfb->lfb_status & LFB_USE_DIIO)
	    di_io = &lfb->lfb_di_cbs[LG_DUAL_DI_CB];
	else
	{ 
	    di_io = &Lg_di_cbs[LG_DUAL_DI_CB];
        }
#ifdef LG_TRACE
	TRdisplay("write_block: writing dual block at %p with buffer %p lxb: %x checksum: %x\n",
			FirstLBB, FirstLBB->lbb_buffer, lxb, lbh->lbh_checksum);
	TRdisplay("           : lbh @ %p, pageno: %d, num_pages: %d\n",
			lbh, pageno, num_pages);
#endif

        if (force_dual_io_failure)
            status = FAIL;
        else
            status = LG_WRITE(di_io, &num_written,
                            pageno, (char *)lbh, sys_err);

	if (status)
	{
	    uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	    uleFormat(NULL, E_DMA44F_LG_WB_BLOCK_INFO, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, "DUAL", 0, num_pages, 0, pageno, 0, lbh,
		    0, lfb->lfb_header.lgh_size,
		    0, FirstLBB->lbb_lga.la_offset);

	    /* Now continue on to perform I/O error processing */
	}

	if (status != OK)
	{
	    /*
	    ** An I/O error has occurred. If this log has already been disabled,
	    ** then all the necessary actions have already been taken and no
	    ** further error processing is needed.
	    */
	    if ((lfb->lfb_status & LFB_DUAL_LOGGING) == 0 &&
		(lfb->lfb_active_log == LGD_II_LOG_FILE))
	    {
		status = OK;
	    }
	}

	FirstLBB->lbb_dual_io_status = status;
	if (status == OK && num_pages != num_written)
	    FirstLBB->lbb_dual_io_status = LG_WRITEERROR;

	if (lfb->lfb_status & LFB_DUAL_LOGGING)
	    status = OK;    /* B56706, B58260 -- ok to reset status now */

	status = write_complete(FirstLBB,
			lxb, (i4) 0, num_pages, async_status);
    }

    return (status);
}

/*{
** Name: write_complete	- Cleanup after write completes.
**
** Description:
**	This routine is invoked once a logpage I/O has completed.
**
**	If an I/O error occurs, and we have a backup log file (i.e., dual
**	logging is currently active), then we fail over to the other log file
**	and mark this log file as being no longer in use.
**
**	If an I/O error occurs, and there is NO backup log file to use, it's
**	not exactly clear what we should do. In earlier releases of Ingres,
**	we simply kept running; if there were any threads waiting for the
**	completion of this block, they were awakened with the I/O error status,
**	but other than that we disregarded the error.
**
**	The current behavior upon I/O error is more extreme: we resume any
**	waiters with the I/O completion status, but we then signal an
**	immediate hard shutdown of the logging system. An I/O error in the
**	logfile jeopardizes recovery and journalling; we don't want to treat
**	such an error lightly. Note that the shutdown which we use is
**	"simulated powerfail", not "immediate shutdown", for the following
**	reasons:
**	    1) lgd_header->lgh_end is suspect -- it does NOT reflect the true
**		EOF, and we do NOT want this in-memory header flushed to disk.
**	    2) if there are subsequent lbb's queued to the write queue, we do
**		NOT want to perform those writes -- no I/O past this point
**		can be allowed.
**	    3) Due to (2), if the RCP is currently waiting for the write
**		completion of one of the pending buffers, it may not be able
**		to respond to the IMM_SHUTDOWN.
**
**	If the I/O succeeds, we update the system wide log address to which
**	the log file is guaranteed forced so that users of the WAL protocol
**	can test to see whether their chosen log record has made it to disk
**	yet.
**
** Inputs:
**      lbb                             Log buffer block.
**	lxb				The LXB of the thread which wrote 
**					this block.
**	primary				non-zero if this was a write to the
**					primary logfile.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	22-oct-1992 (bryanp)
**	    Dual logging bug fixes: after completion of 1st I/O on page, check
**	    to see if another I/O needs to be scheduled.
**      26-apr-1993 (bryanp)
**          Added Dual logging testbed instrumentation.
**	24-may-1993 (bryanp)
**	    Correct the arguments to PCforce_exit().
**	26-jul-1993 (bryanp)
**	    Moved clearing of lxb_assigned_buffer here. We clear this field
**		after we have discharged our responsibilities with respect to
**		the buffer. We may then go on to immediately re-schedule the
**		logwriter to write another buffer.
**	11-Jan-1993 (rmuth)
**	    b58392, When checking if another buffer waiting on the write 
**	    queue check the correct combination of PRIMARY and DUAL flags.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	01-Feb-1996 (jenjo02)
**	    Added lfb_wq_count, which can be tested instead of resorting
**	    to a scan of the write queue.
** 	02-Oct-1996 (jenjo02)
**	    Maintain lock on write queue while logwriter queue is 
**	    updated.
**	17-Dec-1997 (jenjo02)
**	    Changed function to return status to deal with mutex failures
**	    which may occur during recovery operations.
**	24-Aug-1999 (jenjo02)
**	    Hold LBB's lbb_mutex while manipulating this LBB. Hold the write
**	    queue mutex only as long as necessary.
**	    Redid handling of "gprev" buffer. Only those transactions which
**	    still have I/O not yet completed need to continue to wait on
**	    "gprev" - the others can be resumed now.
**	13-Aug-2001 (jenjo02)
**	    o If a thread is writing a spanned buffer, it may encounter
**	      the first buffer of the span as "gprev" and try to mutex
**	      it even though it already holds that buffer's mutex.
**	    o Gprev_lbb's mutex cannot be released until the thread 
**	      doing the write of this buffer has decided whether all
**	      of the thread's I/O has completed yet; releasing gprev
**	      sooner opens a window in which a second thread can 
**	      reschedule the first thread onto another "gprev" and
**	      lead the first thread to believe all of its I/O has
**	      completed when it has not.
**	18-Sep-2001 (jenjo02)
**	   If initial scan for lbb_needing_io finds nothing, rescan after
**	   all work on the current buffer has completed; releasing
**	   the write queue mutex allows other buffers to appear which
**	   may need I/O and we don't want to miss them.
**	03-Jan-2003 (jenjo02)
**	   Moved setting of LBB_????_IO_CMPLT in lbb_state here
**	   from write_block() for mutex protection (BUG 104569).
**	02-Jul-2003 (jenjo02)
**	   Do not remove LBB from write queue until after we've
**	   determined the "gprev" state; other threads may need
**	   to pick this LBB as their "gprev" so it must be
**	   visible. B110520.
**      11-Nov-2003 (wanfr01)
**       Implementation of Jon Jensen's fix for bug 111215
**      11-Nov-2003 (wanfr01)
**	   INGSRV2703, Bug 111764.
**	   The fix for bug 111215 caused a regression where a transaction
**	   may wait on a freed buffer.  Backing out part of that fix.
**	21-Mar-2006 (jenjo02)
**	    o Do write_complete on superblocks ( >1 contiguous LBBs).
**	    o Simplified gprev scan.
**	    o Wait til we're done with write_complete to scour
**	      the write queue looking for more logwriter work.
**	    o Excised assign_logwriter_task() static function, 
**	      incorporating its simple code into the "find-another- 
**	      buffer-for-a-logwriter" code block.
**	03-May-2006 (jenjo02)
**	    Check wait_lbb->lbb_wait_lsn :: gprev_lbb->lbb_forced_lsn
**	    instead of gprev_lbb->lbb_last_lsn. This caused LGforce on a
**	    transaction's last LSN (dmxe_abort) to not wait when it should
**	    and produced read-eof errors during rollback.
*/
static STATUS
write_complete(
LBB 	*FirstLBB, 
LXB 	*lxb, 
i4 	primary,
i4	num_pages,
STATUS	*async_status)
{
    LGD			*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LFB 		*lfb = (LFB*)
			    LGK_PTR_FROM_OFFSET(FirstLBB->lbb_lfb_offset);
    CL_ERR_DESC		sys_err;
    SIZE_TYPE		lbb_offset, end_offset;
    LBB			*lbb;
    LBB			*NextLBB, *LastLBB;
    LBB			*scan_lbb;
    LBB			*gprev_lbb;
    LPB			*master_lpb;
    LPB			*lpb;
    LXBQ		*next_lxbq, *prev_lxbq;
    LXBQ		*wait_lxbq;
    LBB			*next_lbb;
    LBB			*prev_lbb;
    STATUS		status;
    u_i4		gprev_instance;
    i4			gprev_is_mutexed;
    SIZE_TYPE		wq_end_offset = 
			    LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);
    SIZE_TYPE		fq_end_offset = 
			    LGK_OFFSET_FROM_PTR(&lfb->lfb_fq_next);
    i4			pages_remaining;
    i4			bufs_freed = 0;

    LG_WHERE("write_complete")

#ifdef LG_TRACE
    TRdisplay("write_complete: lbb: %x, pageno: %d resume_cnt: %d, primary: %d lxb: %x\n",
			lbb, lbb->lbb_lga.la_block, lbb->lbb_resume_cnt, primary, lxb);
#endif
      
    /* Count one I/O completion for all pages */
    if (primary)
    {
	lgd->lgd_stat.log_writeio++;
	lfb->lfb_stat.log_writeio++;
    }
    else
    {
	lgd->lgd_stat.dual_writeio++;
	lfb->lfb_stat.dual_writeio++;
    }

    /*
    ** Do write-completion on each LBB in the superblock,
    ** from FirstLBB for num_pages.
    **
    ** Note that all LBBs in the superblock have the same
    ** lbb_resume value (1 or 2) so we don't mix freeable
    ** with "more I/O needed" pages.
    **
    ** Freeable LBBs are individually removed from the 
    ** write queue as they are processed, but are not 
    ** reinserted on to the free queue until all LBBs in
    ** the superblock have been handled, and then as 
    ** a block. This cuts down on the frequency of needing
    ** to mutex and manipulate the free queue.
    */

    lbb = LastLBB = FirstLBB;
    pages_remaining = num_pages;

    /* Assume all I/O is complete */
    *async_status = LG_SYNCH;

    while ( pages_remaining-- )
    {
	/* Lock this LBB's mutex */
	LG_mutex(SEM_EXCL, &lbb->lbb_mutex);

	/* If the first fails, they all fail */
	lbb->lbb_io_status = FirstLBB->lbb_io_status;
	lbb->lbb_dual_io_status = FirstLBB->lbb_dual_io_status;
      
	if (primary)
	    lbb->lbb_state |= LBB_PRIM_IO_CMPLT;
	else
	    lbb->lbb_state |= LBB_DUAL_IO_CMPLT;

#ifdef LG_DUAL_LOGGING_TESTBED
	if (primary)
	    LG_check_write_complete_test( lgd, lbb );
	else
	    LG_check_dual_complete_test( lgd, lbb );
#endif

	/* Lock the write queue */
	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex))
	    return(status);

	NextLBB = (LBB*)LGK_PTR_FROM_OFFSET(lbb->lbb_next);

	if (--lbb->lbb_resume_cnt == 0)
	{
	    if ((primary != 0 && lbb->lbb_io_status != OK) ||
		(primary == 0 && lbb->lbb_dual_io_status != OK))
	    {
		/* 
		** Disable logging if this write failed but other write was OK
		*/

		if (lfb->lfb_status & LFB_DUAL_LOGGING)
		{
		    LG_disable_dual_logging(primary ? LGD_II_DUAL_LOG :
						    LGD_II_LOG_FILE,
					lfb);

		    /* Make sure the OK status get returned. */
		    if (primary)
			lbb->lbb_io_status = lbb->lbb_dual_io_status;
		}
	    }
	    else
	    {
		/*
		** this write succeeded; record last I/O for read optimization.
		** If this was the I/O to the dual log file, then ensure that the
		** overall status of the write is "OK" (the primary might have
		** failed prior to this).
		**
		** Note that lfb_channel_blk is updated under the
		** protection of lfb_wq_mutex.
		*/
		if (primary)
		{
		    lfb->lfb_channel_blk = lbb->lbb_lga.la_block;
		}
		else
		{
		    lfb->lfb_dual_channel_blk = lbb->lbb_lga.la_block;
		    lbb->lbb_io_status = lbb->lbb_dual_io_status;
		}
	    }

	    /*
	    ** ASSERT: (lbb->lbb_io_status == OK) if and only if at least one
	    **	    I/O completed properly.
	    */

	    /*
	    ** Scan the write queue looking for log buffers which have not yet
	    ** been written which are "before" this log buffer (that is, have lower
	    ** log addresses). Select the highest such buffer. Call this buffer
	    ** the "greatest previous log buffer". There may be no such buffer;
	    ** that is, we may be processing the completion of the lowest-numbered-
	    ** not-yet-totally-written log page.
	    **
	    ** Note that the "greatest previous log buffer" is only an issue when
	    ** this buffer was NOT the logfile header. The logfile header buffer
	    ** is special and does not have a previous buffer.
	    */

	    /*
	    ** Our LBB remains on the write queue, visible to others,
	    ** while we determine gprev.
	    */

	    /*
	    ** Further assertion:
	    **
	    ** Buffers are always placed on the write queue in 
	    ** ascending LGA order, therefore any potential "gprev"
	    ** buffers can only appear before this LBB on the 
	    ** write queue. 
	    **
	    ** In fact, the "greatest previous" LGA can only be
	    ** the (non-header) buffer directly preceding this LBB.
	    */

	    gprev_is_mutexed = FALSE;
	    gprev_lbb = (LBB*)NULL;

	    if ( lbb->lbb_prev != wq_end_offset &&
		 lbb->lbb_io_status == OK && 
		 lbb->lbb_lga.la_block )
	    {
		scan_lbb = lbb;

		/* If there's a previous... */
		while ( scan_lbb->lbb_prev != wq_end_offset )
		{
		    scan_lbb = (LBB*)LGK_PTR_FROM_OFFSET(scan_lbb->lbb_prev);
		    /* ...and it's not the log header...*/
		    if ( scan_lbb->lbb_lga.la_block == 0 )
			continue;
		    else
		    {
			/*
			** We have a gprev. Release the write queue and our
			** LBB, lock gprev, LBB, and WQ (to avoid deadlocks),
			** then make sure we have the same gprev after the
			** mutex wait. If not, recheck lbb_prev.
			*/
			gprev_lbb = scan_lbb;

			/*
			** Handle the special situation whereby this non-logwriter
			** LXB is writing the 2nd buffer of a spanned log
			** record and it encounters its still-current 
			** buffer as gprev. In that case, gprev is still
			** mutexed by this LXB, so don't try to take it
			** again.
			*/
			if ( gprev_lbb->lbb_state & LBB_CURRENT &&
			     lxb && 
			    !(lxb->lxb_status & LXB_LOGWRITER_THREAD) )
			{
			    break;
			}

			gprev_instance = gprev_lbb->lbb_instance;

			/* Release write queue and LBB mutexes */
			LG_unmutex(&lfb->lfb_wq_mutex);
			/* Other threads may need our LBB as their "gprev" */
			LG_unmutex(&lbb->lbb_mutex);

			LG_mutex(SEM_EXCL, &gprev_lbb->lbb_mutex);

			/* Reaquire write queue and LBB mutexes */
			LG_mutex(SEM_EXCL, &lbb->lbb_mutex);
			LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);

			/* Gprev LBB the same? */
			if (gprev_lbb->lbb_instance != gprev_instance)
			{
			    /* ...no, try again */
			    LG_unmutex(&gprev_lbb->lbb_mutex);
			    gprev_lbb = (LBB*)NULL;
			    scan_lbb = lbb;
			    continue;
			}
			gprev_is_mutexed = TRUE;
		    }
		    break;
		}
	    }

	    /* Remove LBB from write queue */

	    next_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb->lbb_next);
	    prev_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb->lbb_prev);
	    next_lbb->lbb_prev = lbb->lbb_prev;
	    prev_lbb->lbb_next = lbb->lbb_next;
	    lfb->lfb_wq_count--;

	    /*
	    ** If gprev_lbb is not 0, then it is set to the "greatest previous"
	    ** log buffer and its lbb_mutex is locked.
	    */

#if 0
	    if (gprev_lbb)
		TRdisplay("write_complete: adopt waiters to a previous buffer\n");
#endif

	    /*
	    ** If this was not the logfile header, then update the variables which
	    ** we use to manage the highest known system-wide forced log address (in
	    ** lgd_forced_lsn).
	    */

	    if (lbb->lbb_lga.la_block && lbb->lbb_io_status == OK)
	    {
		if ( gprev_lbb == (LBB*)NULL )
		{
		    /*
		    ** The system-wide forced log address is now the log address
		    ** which was forced by the completion of this buffer:
		    **
		    ** Note that lfb_forced_lsn/lga are updated under
		    ** the protection of lfb_wq_mutex.
		    */
		    if ( lbb->lbb_forced_lsn.lsn_high != 0 )
			lfb->lfb_forced_lsn = lbb->lbb_forced_lsn;
		    lfb->lfb_forced_lga = lbb->lbb_forced_lga;
		}
		else
		{
		    /*
		    ** Set gprev_lbb->lbb_forced_lsn to
		    **	max(gprev_lbb->lbb_forced_lsn, lbb->lbb_forced_lsn)
		    **
		    ** Once gprev_lbb's write completes, the system-wide forced
		    ** log address will be updated (or some yet earlier lbb's
		    ** lbb_forced_lsn field will get updated).
		    */

		    if (LSN_GT(&lbb->lbb_forced_lsn, &gprev_lbb->lbb_forced_lsn))
		    {
			gprev_lbb->lbb_forced_lsn = lbb->lbb_forced_lsn;
		    }

		    if (LGA_GT(&lbb->lbb_forced_lga,
					&gprev_lbb->lbb_forced_lga))
		    {
			gprev_lbb->lbb_forced_lga = lbb->lbb_forced_lga;
		    }
		}
	    }

	    /* Now it's safe to release the write queue */
	    (VOID)LG_unmutex(&lfb->lfb_wq_mutex);

	    /*
	    ** The write queue is unlocked. LBB and gprev_lbb, if any,
	    ** are mutexed 
	    */

	    /*
	    **  Check for write completion waiters. If there is a greatest previous
	    **  log buffer, re-schedule these waiters to wait for the I/O
	    **  completion of that log buffer. Otherwise, their I/O wait is now
	    **  fully complete, so resume them.
	    */
	    
	    if (lbb->lbb_wait_count)
	    {
		if (gprev_lbb)
		{
		    /*
		    ** This exquisite section of code looks at all the waiting LXBs
		    ** on the just-written buffer ("lbb") and transfers those which 
		    ** still need I/O to the wait queue of the "gprev_lbb"
		    ** buffer. If any remain on the "lbb" wait queue after this
		    ** process, all their I/O is complete and they may be resumed.
		    ** We don't need wait queue mutexes to do this because we hold
		    ** both the "lbb" and "gprev_lbb" mutexes.
		    */
		    SIZE_TYPE	lxbq_offset;
		    LXBQ	*lxbq;
		    LXB	*wait_lxb;

		    for (lxbq_offset = lbb->lbb_wait.lxbq_next;
			lxbq_offset != LGK_OFFSET_FROM_PTR(&lbb->lbb_wait);
			)
		    {
			lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq_offset);
			lxbq_offset = lxbq->lxbq_next;

			/*  Calculate waiting LXB address. */

			wait_lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

			/*
			** If the LSN that this LXB must wait for is less than or equal
			** to gprev_lbb's forced LSN, then it must await the completion
			** of more I/O, so transfer it to the gprev_lbb's wait queue,
			** otherwise all needed I/O for the LXB has been completed
			** and it can be resumed.
			*/
			if ( LSN_LTE(&wait_lxb->lxb_wait_lsn, &gprev_lbb->lbb_forced_lsn) )
			{
			    /*  Remove from LBB's wait queue. */
			    next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
			    prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_prev);
			    next_lxbq->lxbq_prev = lxbq->lxbq_prev;
			    prev_lxbq->lxbq_next = lxbq->lxbq_next;
			    lbb->lbb_wait_count--;

			    /* Insert onto gprev_lbb's wait queue */
			    wait_lxb->lxb_wait.lxbq_prev = gprev_lbb->lbb_wait.lxbq_prev;
			    wait_lxb->lxb_wait.lxbq_next = 
				LGK_OFFSET_FROM_PTR(&gprev_lbb->lbb_wait.lxbq_next);
			    prev_lxbq = 
				(LXBQ *)LGK_PTR_FROM_OFFSET(gprev_lbb->lbb_wait.lxbq_prev);
			    prev_lxbq->lxbq_next =
				gprev_lbb->lbb_wait.lxbq_prev = 
				    LGK_OFFSET_FROM_PTR(&wait_lxb->lxb_wait);

			    wait_lxb->lxb_wait_lwq = LGK_OFFSET_FROM_PTR(gprev_lbb);

			    gprev_lbb->lbb_wait_count++;
			}
		    }
		}

		/*
		** If the thread which induced this write is scheduled to
		** wait on this just-written buffer, remove it and un-wait it;
		** a wait won't be necessary.
		**
		** If there was a "greater previous" buffer that this LXB must
		** wait on, the LXB has been rescheduled to wait on that buffer,
		** above.
		*/

		if ( lxb && lxb->lxb_status & LXB_WAIT_LBB )
		{
		    if ( lxb->lxb_wait_lwq == LGK_OFFSET_FROM_PTR(lbb) )
		    {
			/* Remove LXB from LBB's wait queue */
			next_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait.lxbq_next);
			prev_lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxb->lxb_wait.lxbq_prev);
			next_lxbq->lxbq_prev = lxb->lxb_wait.lxbq_prev;
			prev_lxbq->lxbq_next = lxb->lxb_wait.lxbq_next;
			lbb->lbb_wait_count--;

			lxb->lxb_status &= ~(LXB_WAIT | LXB_WAIT_LBB);
			lxb->lxb_wait_reason = 0;
			lxb->lxb_wait_lwq = 0;

			/* LXB's I/O is complete */
			*async_status = LG_SYNCH;
		    }
		    else
			/* LXB must wait for more I/O to complete */
			*async_status = OK;
		}

		/*
		** We can't release gprev's mutex until we're no longer
		** concerned with this LXB's wait status. Another thread
		** can acquire gprev and move the waiting LXBs (including
		** this one) to yet another LBB and change this LXB's
		** lxb_wait_lwq value while we're looking at it, above.
		** Holding the mutex until now keeps this from occuring.
		*/
		if ( gprev_is_mutexed )
		    LG_unmutex(&gprev_lbb->lbb_mutex);

		/* Still waiters? */
		if (lbb->lbb_wait_count)
		{
		    lgd->lgd_stat.group_force++;
		    lgd->lgd_stat.group_count += lbb->lbb_wait_count;

		    /* Wakeup anyone waiting for write completion on this buffer. */

		    LG_resume_lbb(lxb, lbb, lbb->lbb_io_status);
#if 0
		    TRdisplay("write completion waiters have been resumed\n");
#endif
		}
	    } /* if (lbb->lbb_wait_count) */
	    else if ( gprev_is_mutexed )
		LG_unmutex(&gprev_lbb->lbb_mutex);

	    if ((lbb->lbb_io_status != OK))
	    {
		/*
		** This I/O failed, and there is no backup log file to fail over
		** to. Therefore, we have a fatal error in the logging system and
		** we must request that the RCP shut things down before allowing
		** things to progress any further.
		*/
		LG_signal_event(LGD_EMER_SHUTDOWN | LGD_IMM_SHUTDOWN, 
					    LGD_START_SHUTDOWN,
					    FALSE);
	    }

	    if (lgd->lgd_status & LGD_EMER_SHUTDOWN)    /* automated testing */
	    {
		if (lgd->lgd_master_lpb)
		{
		    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);
		    (void) PCforce_exit(master_lpb->lpb_pid, &sys_err);
		}
	    }

	    /*
	    ** All I/O for this buffer has completed. Mark the LBB
	    ** as free and return it to the free queue. 
	    */
#ifdef LG_TRACE
	    TRdisplay("Marking free block at %p with buffer %p, pageno %d\n",
			    lbb, lbb->lbb_buffer, lbb->lbb_lga.la_block);
#endif

	    /*	Mark buffer as free. */

	    lbb->lbb_state = LBB_FREE;
	    lbb->lbb_instance++;

	    if ( bufs_freed++ )
	    {
		/* Add to list of now-free contiguous buffers */
		LastLBB->lbb_next = LGK_OFFSET_FROM_PTR(lbb);
		lbb->lbb_prev = LGK_OFFSET_FROM_PTR(LastLBB);
	    }
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	}
	else /* if (--lbb->lbb_resume_cnt == 0) */
	{
	    /*
	    ** the other I/O has not completed yet. We've already decremented the
	    ** resume count, so we just check to see if we must disable dual
	    ** logging, record the last block accessed for read optimization, and
	    ** we're done.
	    */

	    /* Must wait for more I/O to complete */
	    *async_status = OK;

	    if (primary)
	    {
		if ((lbb->lbb_io_status != OK))
		    LG_disable_dual_logging(LGD_II_DUAL_LOG, lfb);
		else
		    lfb->lfb_channel_blk = lbb->lbb_lga.la_block;
	    }
	    else
	    {
		if ((lbb->lbb_dual_io_status != OK))
		    LG_disable_dual_logging(LGD_II_LOG_FILE, lfb);
		else
		    lfb->lfb_dual_channel_blk = lbb->lbb_lga.la_block;
	    }

	    /* Release the write queue and buffer */
	    LG_unmutex(&lfb->lfb_wq_mutex);
	    LG_unmutex(&lbb->lbb_mutex);
	}

	/* On to next LBB, if any, remembering "last" LBB */
	LastLBB = lbb;
	lbb = NextLBB;

    } /* while ( pages_remaining ) */

    /* If all LBBs were freed... */
    
    if ( bufs_freed == num_pages )
    {
	/*
	** Return block of free buffer(s) to the free queue,
	** FirstLBB .... LastLBB.
	**
	** Insert the block in order by FirstLBB's lbb_offset:
	*/

	if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
	    return(status);

	lbb_offset = LGK_OFFSET_FROM_PTR(FirstLBB);

	/*
	** The free queue rotates around its "logical end"
	** such that returning buffers less than the lowest
	** buffer at the current head of the free queue are
	** reinserted after the logical eof.
	**
	** As requests for free buffers are always satisfied
	** from the head of the free queue, this ensures that
	** all buffers will be utilized round-robin.
	** 
	** This, combined with the fact that modified buffers
	** are always inserted FIFO on the write queue, 
	** guarantees that write_block() will be able to 
	** write multiple log pages in a superblock 
	** as often as possible.
	*/
	if ( lbb_offset > lfb->lfb_fq_next &&
	     lbb_offset < lfb->lfb_fq_leof )
	{
	    next_lbb = (LBB*)LGK_PTR_FROM_OFFSET(lfb->lfb_fq_leof);
	    end_offset = fq_end_offset;
	}
	else
	{
	    next_lbb = (LBB*)&lfb->lfb_fq_next;
	    end_offset = lfb->lfb_fq_leof; 
	}

	/* Scan backwards to find insertion point */
	while ( next_lbb->lbb_prev != end_offset &&
		next_lbb->lbb_prev > lbb_offset )
	{
	    next_lbb = (LBB*)LGK_PTR_FROM_OFFSET(next_lbb->lbb_prev);
	}

	prev_lbb = (LBB*)LGK_PTR_FROM_OFFSET(next_lbb->lbb_prev);

	/* Insert LBB block between prev and next */
	LastLBB->lbb_next = prev_lbb->lbb_next;
	FirstLBB->lbb_prev = LGK_OFFSET_FROM_PTR(prev_lbb);
	prev_lbb->lbb_next = lbb_offset;
	next_lbb->lbb_prev = LGK_OFFSET_FROM_PTR(LastLBB);

	/* If fq was empty, reset logical eof to LastLBB in block */
	if ( lfb->lfb_fq_leof == fq_end_offset )
	    lfb->lfb_fq_leof = LGK_OFFSET_FROM_PTR(LastLBB);

	lfb->lfb_fq_count += bufs_freed;

	/* Wakeup anyone waiting on (one or more) free buffers */

	LG_resume(&lfb->lfb_wait_buffer, lfb);
	(VOID)LG_unmutex(&lfb->lfb_fq_mutex);
    }


    if ( lxb && lxb->lxb_status & LXB_LOGWRITER_THREAD )
    {
	/*
	** Should this logwriter thread write another buffer? Or should it go
	** back to sleep...? Search the write queue to see if there exists a buffer
	** which needs I/O processing. If so, assign this transaction to
	** process that buffer and resume ourselves.
	*/

	/* Assume we won't find another buffer to write */
	lxb->lxb_assigned_buffer = 0;
	lxb->lxb_assigned_task = 0;
	lbb = (LBB*)NULL;

	LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);

	for (lbb_offset = lfb->lfb_wq_next;
	    lbb_offset != wq_end_offset;
	    lbb_offset = scan_lbb->lbb_next)
	{
	    scan_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);
	    if ( (scan_lbb->lbb_state & 
		  (LBB_PRIM_IO_NEEDED | LBB_PRIMARY_LXB_ASSIGNED))
					    == LBB_PRIM_IO_NEEDED
		||
		 (scan_lbb->lbb_state & 
		  (LBB_DUAL_IO_NEEDED | LBB_DUAL_LXB_ASSIGNED))
					    == LBB_DUAL_IO_NEEDED
	       )
	    {
		lbb = scan_lbb;
		break;
	    }
	}
	/*
	** Release the write queue, mutex and
	** recheck this buffer.
	*/
	LG_unmutex(&lfb->lfb_wq_mutex);

	if ( lbb )
	{
	    /* Lock the LBB's mutex, check it again */
	    LG_mutex(SEM_EXCL, &lbb->lbb_mutex);
	    if ( (lbb->lbb_state & 
		  (LBB_PRIM_IO_NEEDED | LBB_PRIMARY_LXB_ASSIGNED))
					    == LBB_PRIM_IO_NEEDED
		||
		  (lbb->lbb_state & 
		  (LBB_DUAL_IO_NEEDED | LBB_DUAL_LXB_ASSIGNED))
					    == LBB_DUAL_IO_NEEDED
	       )
	    {
		lfb = (LFB*)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);

		if ((lbb->lbb_state &
		    (LBB_PRIM_IO_NEEDED | LBB_PRIMARY_LXB_ASSIGNED))
				    == LBB_PRIM_IO_NEEDED)
		{

		    lbb->lbb_state |= LBB_PRIMARY_LXB_ASSIGNED;

		    if ((lbb->lbb_state &
			    (LBB_PRIM_IO_NEEDED | LBB_PRIMARY_LXB_ASSIGNED)) ==
			    (LBB_PRIM_IO_NEEDED | LBB_PRIMARY_LXB_ASSIGNED) ||
			(lfb->lfb_active_log == LGD_II_LOG_FILE))
			lxb->lxb_assigned_task = LXB_WRITE_PRIMARY;
		    else
			lxb->lxb_assigned_task = LXB_WRITE_DUAL;
		}
		else
		{

		    lxb->lxb_assigned_task   = LXB_WRITE_DUAL;
		    lbb->lbb_state |= LBB_DUAL_LXB_ASSIGNED;
		}

		lxb->lxb_assigned_buffer =
				LGK_OFFSET_FROM_PTR(lbb);
		lbb->lbb_owning_lxb = LGK_OFFSET_FROM_PTR(lxb);
	    }
	    /* After mutex, no longer needs I/O, give up */
	    LG_unmutex(&lbb->lbb_mutex);
	}
    }	    

    return(status);
}

/*
** Name: LG_write_block		- LogWriter thread write-a-block routine
**
** Description:
**	This routine is called by the LogWriter threads when they are awoken
**	to write a log page to the log file. It locates the buffer which this
**	thread has been assigned to write, and then
**	calls write_block to actually write that page out.
**
**	In a dual logging environment there will be multiple I/O requests,
**	one for the primary log file and one for the dual log file.
**	The log page is not removed from the queue until both threads have
**	written the page.
**
** Inputs:
**	xact_id		    Transaction ID from LGbegin
**
** Outputs:
**	sys_err		    Reason for error return.
**
** Returns:
**	STATUS
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the 
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  Added "release_mutex" argument to LG_do_write.
**	26-Jan-1996 (jenjo02)
**	    Acquire/release lxb_mutex instead of lgd_mutex.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from LG_do_write().
*/
STATUS
LG_write_block(
LG_LXID		    xact_id,
CL_ERR_DESC	    *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB    *lxb;
    LG_I4ID_TO_ID   lx_id;
    SIZE_TYPE	    *lxbb_table;
    STATUS	    status = OK;
    i4	    err_code;

    CL_CLEAR_ERR(sys_err);

    LG_WHERE("LG_write_block")

    /*	Check the lx_id. */

    lx_id.id_i4id = xact_id;
    if ((i4)lx_id.id_lgid.id_id > lgd->lgd_lxbb_count || lx_id.id_lgid.id_id == 0)
    {
	uleFormat(NULL, E_DMA44C_LG_WBLOCK_BAD_ID, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 2,
				0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id.id_lgid.id_instance)
    {
	uleFormat(NULL, E_DMA44D_LG_WBLOCK_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id.id_lgid.id_instance);
	return (LG_BADPARAM);
    }

    /* Do writes without holding LXB's mutex */
    status = LG_do_write(lxb, sys_err);

    /* Prepare to go back to sleep */
    if (status = LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
	return(status);

    lxb->lxb_status |= LXB_WAIT;
    lxb->lxb_wait_reason = LG_WAIT_WRITEIO;
    lxb->lxb_stat.wait++;

    (VOID)LG_unmutex(&lxb->lxb_mutex);
		

    if (status)
    {
	uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
    }

    return (status);
}

/*
** Name: LG_do_write
**
** History:
**	26-jul-1993 (bryanp)
**	    Removed too-soon clearing of lxb_assigned_buffer. The logwriter
**		can't clear this field until it has discharged its
**		responsibilities with respect to the buffer, otherwise if the
**		server should die during the DIwrite call the rundown code might
**		fail to adopt the I/O because it thinks it's already been
**		completed!.
**      31-jan-1994 (mikem)
**          bug #56478
**          0 logwriter threads caused installation to hang, fixed by adding
**          special case code to handle synchonous "in-line" write's vs.
**          asynchronous writes.  The problem was that code that scheduled
**          "synchronous" writes with direct calls to "write_block" did not
**          handle correctly the mutex being released during the call.  In the 
**          synchronous case one must hold the LG mutex across the DIwrite()
**          call.  
**	26-Jan-1996 (jenjo02)
**	    lxb_mutex must be held on entry to this function.
**	    Moved setting of lbb_state to LBB_???_IO_INPROG to
**	    write_block() for consistency.
**	05-Mar-1996 (jenjo02)
**	    Count a write in logwriter's lxb_stat.write so we can see
**	    how much work they're doing. Note that this stat
**	    has already been incremented in the originating 
**	    transaction's LXB, so it's doubly counted, but, hey, 
**	    it's only a stat...
**	13-Mar-1996 (jenjo02)
**	    Moved incrementing of lpb_stat.write to caller of write_block()
**	    from LG_write() to better track writes by process.
**	13-Mar-1996 (jenjo02)
**	    Instead of letting lw thread suspend, then immediately
**	    resume, let LG_do_write() spin on 
**	    while (lxb->lxb_assigned_buffer).
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" argument from LG_do_write().
**	17-Dec-1997 (jenjo02)
**	    Clear lxb_assigned_buffer on error.
**	03-Jan-2003 (jenjo02)
**	    Mutex lbb_mutex to protect lbb_status change by
**	    write_block() (BUG 104569).
*/
STATUS
LG_do_write(
    register LXB 	*lxb, 
    CL_ERR_DESC *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LBB		    *lbb;
    LPD		    *lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    LPB		    *lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
    LFB		    *lfb;
    i4	    err_code;
    STATUS	    status = OK, async_status;
    i4		    write_primary;

    CL_CLEAR_ERR(sys_err);

    /*
    ** If there is a block assigned to this transaction, call write_block.
    */

#if 0
    TRdisplay("Logwriter thread looking for a block\n");
#endif

    /*
    ** As long as there are buffers to write, write them.
    */
    while (lxb->lxb_assigned_buffer)
    {
	lbb = (LBB *)LGK_PTR_FROM_OFFSET(lxb->lxb_assigned_buffer);
	lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);
	lxb->lxb_assigned_buffer = 0;

	/* Lock the LBB; write_block will release it */
	if ( status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex) )
	    return(status);

	lxb->lxb_stat.write++;
	lpb->lpb_stat.write++;

	write_primary = (lxb->lxb_assigned_task == LXB_WRITE_PRIMARY);

	/* write_block will not return until I/O has completed */
	status = write_block(lbb,
			    write_primary,
			    lxb,
			    &async_status,
			    sys_err);
	if (status)
	{
	    uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	    uleFormat(NULL, E_DMA44E_LG_WBLOCK_BAD_WRITE, sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 5,
			0, lbb, 0, lbb->lbb_buffer,
			0, lxb->lxb_id.id_id, 0, lxb->lxb_id.id_instance,
			0, write_primary ? "PRIMARY" : "DUAL" );
	    break;
	}

#ifdef LG_TRACE
	TRdisplay("Logwriter %x done: block %p, buffer %p, pageno %d status %x next %p\n",
		    lxb, lbb, lbb->lbb_buffer, lbb->lbb_lga.la_block,
		    status, lxb->lxb_assigned_buffer);
#endif
    }

    return (status);
}

static	VOID
dump_wait_queue(LBB *lbb, char *comment)
{
    LXBQ	    *lxbq = &lbb->lbb_wait;
    LXB		    *lxb;
    SIZE_TYPE	    lxbq_offset;
    SIZE_TYPE	    end_offset;

    TRdisplay("There are currently %d waiters on %s\n", lbb->lbb_wait_count,
		comment);
    TRdisplay("    LBB @%p   LXBQ @%p\n", lbb, lxbq);

    end_offset = LGK_OFFSET_FROM_PTR(&lbb->lbb_wait);

    for (lxbq_offset = lbb->lbb_wait.lxbq_next;
	lxbq_offset != end_offset;
	lxbq_offset = lxbq->lxbq_next)
    {
	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq_offset);

	/*  Calculate LXB address. */

	lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

	TRdisplay("   LXB @ %p: PID %d Sid %x status %v (%x)\n",
			lxb, lxb->lxb_cpid.pid, lxb->lxb_cpid.sid,
		"INACTIVE,ACTIVE,CKPDB_PEND,PROTECT,JOURNAL,READONLY,NOABORT,\
RECOVER,FORCE_ABORT,SESSION_ABORT,SERVER_ABORT,PASS_ABORT,DBINCONSISTENT,WAIT,\
DISTRIBUTED,WILLING_COMMIT,RE-ASSOC,RESUME,MAN_ABORT,MAN_COMMIT",
		lxb->lxb_status, lxb->lxb_status);
	TRdisplay("              : Wait Reason %w (%x) User %t\n",
		"(not waiting),FORCE,SPLIT,HDRIO,CKPDB,OPENDB,BCPSTALL,\
LOGFULL,FREEBUF,LASTBUF,FORCED_IO,EVENT,WRITEIO,MINI,\
14,15,16,17,18,19,20",
		lxb->lxb_wait_reason, lxb->lxb_wait_reason,
		lxb->lxb_l_user_name, &lxb->lxb_user_name[0]); 
    }
}

/*
** Name: assign_logwriter	    - assign logwriter thread(s) to this buffer
**
** Description:
**	This routine is called to assign a log writer thread or perhaps two
**	logwriter threads to the job of writing this buffer to disk. We pick
**	an appropriate idle logwriter thread and assign it to write the block.
**	If dual logging is currently active, we attempt to assign a second
**	logwriter thread to write to the dual log in parallel.
**
**	If all the logwriter threads in the system are currently busy, then we
**	leave this buffer on the write queue; write_completion logwriter logic
**	will assign the buffer to some logwriter when it completes the write
**	which is in progress.
**
** Inputs:
**	lgd			    - The LGD structure.
**	lbb			    - The log page to write.
**	lpb			    - which process is making this call.
**
** Outputs:
**	None
**
** Side Effects:
**	One or more logwriter threads may get woken up.
**
** Returns:
**	VOID
**
** History:
**	27-oct-1992 (bryanp)
**	    Created for the new logging system.
**	26-jul-1993 (bryanp)
**	    When selecting a logwriter from the list of idle logwriters, choose
**		one in this server if possible. This will hopefully minimize
**		context switching in a system with surplus logwriter threads.
**	    Added LPB argument.
**	11-Jan-1993 (rmuth)
**	    b57887, If running with only the dual log we could assign
**	    multiple log writer threads to write the same log buffer.
**	    Assign lbb_state to either LBB_PRIMARY(DUAL)_LXB_ASSIGNED
**	    accordingly.
**	22-Apr-1996 (jenjo02)
**	    Added lgd_stat.no_logwriter to count number of times
**	    a logwriter couldn't be assigned to the buffer.
**	01-Oct-1996 (jenjo02)
**	    When called, buffer has been placed on the write queue,
**	    which is still locked.
**	18-Mar-2004 (jenjo02)
**	    Use LBB's LFB, not local LFB.
*/
VOID
assign_logwriter(LGD *lgd, register LBB *lbb, LPB *lpb)
{
    LXB		*prim_lw_lxb;
    LXB		*dual_lw_lxb = (LXB *)NULL;
    LFB		*lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);
    LBB		*prev_lbb;

    LG_WHERE("assign_logwriter")
#ifdef LG_TRACE
    TRdisplay("assign_logwriter: lbb: %x, pageno: %d, resume_cnt: %d\n",
			lbb, lbb->lbb_lga.la_block, lbb->lbb_resume_cnt);
#endif

    /*
    ** Select a logwriter LXB, returning with lxb_mutex held.
    */
    if (prim_lw_lxb = choose_logwriter())
    {
	prim_lw_lxb->lxb_assigned_buffer = LGK_OFFSET_FROM_PTR(lbb);
	lbb->lbb_owning_lxb = LGK_OFFSET_FROM_PTR(prim_lw_lxb);

	if ((lbb->lbb_resume_cnt == 2) ||
	    (lfb->lfb_active_log == LGD_II_LOG_FILE))
	{
	    prim_lw_lxb->lxb_assigned_task = LXB_WRITE_PRIMARY;
	    lbb->lbb_state |= LBB_PRIMARY_LXB_ASSIGNED;
	}
	else
	{
	    prim_lw_lxb->lxb_assigned_task = LXB_WRITE_DUAL;
	    lbb->lbb_state |= LBB_DUAL_LXB_ASSIGNED;
	}

	(VOID)LG_unmutex(&prim_lw_lxb->lxb_mutex);

	if (lbb->lbb_resume_cnt == 2)
	{
	    if (dual_lw_lxb = choose_logwriter())
	    {
		dual_lw_lxb->lxb_assigned_task   = LXB_WRITE_DUAL;
		dual_lw_lxb->lxb_assigned_buffer = LGK_OFFSET_FROM_PTR(lbb);
		lbb->lbb_owning_lxb = LGK_OFFSET_FROM_PTR(dual_lw_lxb);
		lbb->lbb_state |= LBB_DUAL_LXB_ASSIGNED;

		(VOID)LG_unmutex(&dual_lw_lxb->lxb_mutex);
	    }
	}

    }
    else
    {
	/* No logwriters available */
	lbb->lbb_state &= ~(LBB_PRIMARY_LXB_ASSIGNED | LBB_DUAL_LXB_ASSIGNED);
	lgd->lgd_stat.no_logwriter++;
    }

    /*
    ** Wait til both logwriters are assigned and the buffer
    ** put on the write queue before we resume
    ** them. We don't want the primary logwriter disturbing
    ** the lbb while we're in the process of selecting a
    ** dual logwriter.
    */
    if (prim_lw_lxb)
    {
	LGcp_resume(&prim_lw_lxb->lxb_cpid);
	if (dual_lw_lxb)
	    LGcp_resume(&dual_lw_lxb->lxb_cpid);
    }

    return;
}

/*
** Name: choose_logwriter		- pick a logwriter
**
** Description:
**	Pick an idle logwriter to give some work to do.
**	Look for one in the same process as the caller;
**	failing that, just pick the first one.
**
** Inputs:
**	none
**
** Outputs:
**	None
**
** Returns:
**	pointer to logwriter's LXB with lxb_mutex locked, 
**	0 if none could be chosen.
**
** History:
**	30-Jan-1996 (jenjo02)
**	    Added this descriptor, new mutexes.
**	    lxb_mutex of selected logwriter is locked upon exit.
**	17-Dec-1997 (jenjo02)
**	  o Replaced idle LogWriter queue with a static queue of all
**	    LogWriter threads.
**	  o Rewrote queue searching method, added check for LogWriter
**	    process in distress.
**	  o Prototype now passes nothing, use LG_my_pid instead of LPB's pid.
**	20-Apr-1999 (jenjo02)
**	    Release mutex if LXB fails un-busy test.
*/
static	LXB		*
choose_logwriter( VOID )
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXBQ	*lxbq;
    SIZE_TYPE		lxbq_offset;
    SIZE_TYPE		end_offset;
    register LXB	*lxb = (LXB *)NULL;
    LXB			*alternate_lxb = (LXB *)NULL;

    LG_WHERE("choose_logwriter")

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lwlxb.lwq_lxbq.lxbq_next);

    while (lxb == (LXB *)NULL)
    {
	lxbq_offset = lgd->lgd_lwlxb.lwq_lxbq.lxbq_next;

	while (lxbq_offset != end_offset)
	{
	    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq_offset);

	    lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

	    /* Find a LogWriter that's waiting for work */
	    if ((lxb->lxb_status & LXB_LOGWRITER_THREAD) == 0 ||
		(lxb->lxb_status & LXB_WAIT) == 0 ||
		 lxb->lxb_wait_reason != LG_WAIT_WRITEIO)
	    {
		if (lxb->lxb_status & LXB_LOGWRITER_THREAD)
		    lxbq_offset = lxbq->lxbq_next;
		else
		    lxbq_offset = lgd->lgd_lwlxb.lwq_lxbq.lxbq_next;
		lxb = (LXB *)NULL;
		continue;
	    }

	    /* Lock the LXB's mutex, check again */
	    if (LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
		return((LXB *)NULL);

	    if ((lxb->lxb_status & LXB_LOGWRITER_THREAD) == 0 ||
		(lxb->lxb_status & LXB_WAIT) == 0 ||
		 lxb->lxb_wait_reason != LG_WAIT_WRITEIO)
	    {
		if (lxb->lxb_status & LXB_LOGWRITER_THREAD)
		    lxbq_offset = lxbq->lxbq_next;
		else
		    lxbq_offset = lgd->lgd_lwlxb.lwq_lxbq.lxbq_next;
		(VOID)LG_unmutex(&lxb->lxb_mutex);
		lxb = (LXB *)NULL;
		continue;
	    }

	    if (lxb->lxb_cpid.pid == LG_my_pid ||
		lgd->lgd_lwlxb.lwq_count == 1)
	    {
		/*
		** Found an idle logwriter in this process. 
		** Or there is only one logwriter in this installation
		** Pick it, Wilson.
		*/
		break;
	    }
	    /*
	    ** In case we can't find an idle logwriter in this process,
	    ** our fallback position will be to use any that's available.
	    */
	    if (alternate_lxb == (LXB *)NULL)
		alternate_lxb = lxb;

	    lxbq_offset = lxbq->lxbq_next;
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	    lxb = (LXB *)NULL;
	}

	/*
	** If we didn't pick a LXB in this process but found one in another
	** process, lock its mutex, then recheck its availability.
	*/
	if (lxb == (LXB *)NULL)
	{
	    if (lxb = alternate_lxb)
	    {
		if (LG_mutex(SEM_EXCL, &lxb->lxb_mutex))
		    return((LXB *)NULL);

		/*
		** Recheck our "other_pid" selection, avoiding LogWriters in
		** processes that are dead or dying.
		*/
		if (lxb->lxb_status & LXB_LOGWRITER_THREAD &&
		    lxb->lxb_status & LXB_WAIT &&
		    lxb->lxb_wait_reason == LG_WAIT_WRITEIO)
		{
		    LPD		*alternate_lpd;
		    LPB		*alternate_lpb;

		    alternate_lpd = (LPD*)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
		    alternate_lpb = (LPB*)LGK_PTR_FROM_OFFSET(alternate_lpd->lpd_lpb);

		    if ((alternate_lpb->lpb_status & 
				(LPB_RUNAWAY | LPB_DEAD | LPB_DYING)) == 0)
		    {
			break;
		    }
		}

		/* Alternate LXB is no longer a candidate, restart the search */
		LG_unmutex(&lxb->lxb_mutex);
		lxb = alternate_lxb = (LXB *)NULL;
		continue;
	    }
	}
	break;
    }

    if (lxb)
    {
	lxb->lxb_status &= ~LXB_WAIT;
	lxb->lxb_wait_reason = 0;
    }

    /*
    ** Return with lxb_mutex locked if a logwriter was found.
    */
    return(lxb);
}

/*
** Debugging routine, not currently used. Can be used for debugging the
** logwriter selection routines to see if they are reasonably picking
** logwriters.
*/
VOID
LG_dump_logwriters(LGD *lgd)
{
    LXBQ	*lxbq;
    LXB		*lxb;
    LG_I4ID_TO_ID xid;

    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lwlxb.lwq_lxbq.lxbq_next);

    while (lxbq != (LXBQ *)&lgd->lgd_lwlxb.lwq_lxbq.lxbq_next)
    {
	lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

	xid.id_lgid = lxb->lxb_id;
	TRdisplay("%@ LXBQ @ %p -- ID %x, PID=%x, SID=%x\n",
		    lxbq, xid.id_i4id,
		    lxb->lxb_cpid.pid, lxb->lxb_cpid.sid);

	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
    }
    TRflush();
    return;
}

/*
** Debugging routine. Not currently called by anyone. Was used to verify the
** lxbq used to manage logwriters when it seemed to be getting corrupted (it
** ended up being actually a lgcbmem.c bug).
*/
VOID
LG_check_logwriters(LGD *lgd)
{
    LXBQ	*lxbq;
    LXB		*lxb;
    char	*null_ptr = 0;
    LG_I4ID_TO_ID xid;

    lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_lwlxb.lwq_lxbq.lxbq_next);

    while (lxbq != (LXBQ *)&lgd->lgd_lwlxb.lwq_lxbq.lxbq_next)
    {
	lxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_wait));

	if (lxb->lxb_type != LXB_TYPE ||
	    PCis_alive(lxb->lxb_cpid.pid) == FALSE)
	{
	    xid.id_lgid = lxb->lxb_id;
	    TRdisplay("%@ DEAD PID on LXBQ @ %p -- ID %x, PID=%x, SID=%x\n",
		    lxbq, xid.id_i4id,
		    lxb->lxb_cpid.pid, lxb->lxb_cpid.sid);
	    *null_ptr = 123;
	}

	lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lxbq->lxbq_next);
    }
    return;
}
