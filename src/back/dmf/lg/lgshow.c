/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGSHOW.C - Implements the LGshow function of the logging system
**
**  Description:
**	This module contains the code which implements LGshow.
**	
**	External Routines:
**
**	    LGshow -- Retrieve information from the logging system
**
**	Internal Routines:
**
**	    map_dbstatus_to_world
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	7-oct-1992 (bryanp)
**	    Include bt.h.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Add LPB_CPAGENT support.
**	11-jan-1993 (jnash)
**	    Reduced logging project.  Add LG_A_RES_SPACE.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed LG_S_DBID to qualify database using the external database id
**		rather than the ldb_buffer structure.
**	    Consolidated the code to fill in tx and db info blocks into routines
**		format_db_info and format_tr_info.
**	    Removed LG_S_CDATABASE, LG_S_ABACKUP, LG_S_LDATABASE, LG_S_JNLDB,
**		and LG_S_PURJNLDB cases.
**	    Changed LG_S_DATABASE to return information about a specific
**		database identified by its lpd id.  The lpd id is now passed
**		in by the db.db_id field of the database structure.
**	    Added new db information fields: db_database_id, db_l_dmp_la.
**	    Added new tx information fields: tr_database_id.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Document the recording of the cluster node id in LG.
**		Expose the ldb_sback_lsn through the db_sback_lsn.
**		Add LG_S_CSP_ID to show the CSP's PID.
**	24-may-1993 (bryanp)
**	    Map ALL database status bits for callers.
**	24-may-1993 (andys)
**	    In LG_S_FBUFFER, LG_S_WBUFFER cases, don't go off traversing
**	    queues of buffers if offset is NULL.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Clear the cl_err_desc field to avoid formatting garbage errors.
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Dumped old lfb journal and dump windows and added new archive
**	    window information.
**	18-oct-1993 (rogerk)
**	    Added space reservation stuff to transaction info returned.
**	    Made lxb_reserved_space, lfb_reserved_space unsigned.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in 
**	    LG_debug_wacky_ldb_found().
**	24-feb-94 (swm)
**	    Bug #60425
**	    Corrected truncating cast of session id. from i4 to CS_SID
**	    in format_tr_info().
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**	08-Dec-1995 (jenjo02)
**	    Added LG_S_MUTEX function to LGshow() to return statistics
**	    about the logging system semaphore(s).
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LGMUTEX functions. Many new semaphores
**	    added to suppliment singular lgd_mutex.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	22-Apr-1996 (jenjo02)
**	    Added lgd|lgs_stat.no_logwriter count of number of times
**	    a logwriter thread was needed but none were available.
**	10-Jul-1996 (jenjo02)
**	    Added lgs_timer_write_time, lgs_timer_write_idle stats 
**	    to show what is driving gcmt thread log writes.
**	    Added lgs_buf_util in which log buffer utilization histograms
**	    are returned.
**	01-Oct-1996 (jenjo02)
**	    Added lgs_max_wq (max length of write queue) and
**	    lgs_max_wq_count (number of time that max hit).
**	25-Oct-1996 (jenjo02)
**	    In LG_S_OPENDB:LG_S_CLOSEDB, delay releasing ldb queue
**	    mutex until after lgd_status has been updated.
**	06-Nov-1996 (jenjo02)
**	    Removed lbb_last_used, lbb_total_ticks, replaced with
**	    lgd_timer_lbb_last_used, lgd_timer_lbb_total_ticks.
**	24-feb-1997 (canor01)
**	    If LGshow is called before it has attached to the
**	    logging system, return a failure instead of a SEGV.
** 	13-jun-1997 (wonst02)
** 	    Added LDB_READONLY to handle readonly databases.
**	14-Nov-1997 (jenjo02)
**	    Added LDB_STALL_RW for ckpdb.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed
**          for support of logs > 2 gig
**	10-Dec-1997 (jenjo02)
**	    Added LGrcp_pid() external function which returns the
**	    pid of the RCP. Used by CSMTp_semaphore().
**	16-Dec-1997 (jenjo02)
**	    In LG_S_OPENDB don't return LDBs that are in a LDB_CLOSEDB_PEND
**	    state in addition to LDB_OPENDB_PEND.
**	17-Dec-1997 (jenjo02)
**	  o Replaced idle LogWriter queue with a static queue of all
**	    LogWriter threads.
**	24-Aug-1998 (jenjo02)
**	    Rewrote LG_S_ETRANSACTION to find active transactions using
**	    the (new) active transaction hash queue.
**	25-aug-1998 (rigka01) bug #91054
**	    CP allowed to occur during writing of spanned log record
**	    causes CP address to be set to the middle of the record. 
**	    Cause a CP to wait until the EOF becomes legitimized
**	    by the completion of the writing of a record.  
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	15-Mar-1999 (jenjo02)
**	    Removed LDB_STALL_RW status bit.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. return the ldb_eback_lsn in lgshow call.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    lfb_current_lbb_mutex replaced by individual LBB's lbb_mutex.
**	    Dropped lgd_lxb_aq_mutex and inactive LXB queue.
**	    lgd_wait_free and lgd_wait_split queues consolidated into
**	    lgd_wait_buffer queue.
**	17-Sep-1999 (jenjo02)
**	    LG_S_STAMP must return LSN, not LGA, since expansion of
**	    LGA to 3 elements (sequence,block,offset) means that LGAs
**	    and LSNs are no longer (coincidentally) equivalent values. 
**	30-Sep-1999 (jenjo02)
**	    Added LG_S_OLD_LSN function to return the first LSN of
**	    the oldest active transaction.
**	05-dec-1999 (nanpr01 for jenjo02)
**	    Oldest first active LSN call always returns an LSN. When there are
**	    more than one active transactions, it returns the first LSN of
**	    of the oldest active transaction. But when there is no other active
**	    transaction, it returns the LSN of the last committed transaction.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED log transactions.
**	14-Mar-2000 (jenjo02)
**	    Added LG_S_LASTLSN to return last written LSN of a transaction.
**	    Removed static LG_show() function and the unnecessary level of
**	    indirection it induced. Merged history information from the two
**	    functions.
**	05-Jun-2000 (jenjo02)
**	    Bug 101737, race condition in LG_S_OPENDB/CLOSEDB whereby LGD_CLOSEDB
**	    may get turned off just after the server has placed a database in
**	    LDB_CLOSEDB_PEND status and turned on LGD_CLOSEDB. RCP will loop forever
**	    because the signal (LGD_CLOSEDB) has been reset but the database is
**	    still marked LDB_CLOSEDB_PEND.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Added LG_S_SAMP_STAT which returns logging stats useful
**	    to the sampler.
**      01-feb-2001 (stial01)
**          hash/compare UNIQUE low tran (Variable Page Type SIR 102955)
**	24-sep-2001 (devjo01)
**	    Use CXnode_number as source of node # for LGshow(LG_A_NODEID, .. ).
**	    s103715.
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      11-Jun-2004 (horda03) Bug 112382/INGSRV2838
**          Ensure that the current LBB is stil "CURRENT" after
**          the lbb_mutex is acquired. If not, re-acquire.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**      11-may-2007 (stial01)
**          Added LG_S_COMMIT_WILLING, LG_S_ROLLBACK_WILLING
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      28-Jan-2010 (ashco01) Bug 122492
**          Setup lpbb_table & lpb in LG_S_COMMIT_WILLING and LG_S_ROLLBACK_WILLING
**          cases also. 
*/

/*
** Forward declarations of static functions:
*/
static VOID	map_dbstatus_to_world(i4 internal_status,
				    i4 *external_status);
static VOID	format_lbb(LBB *lbb, LG_BUFFER *buf);
static VOID	format_db_info(LDB *ldb, LG_DATABASE *db);
static VOID	format_tr_info(LXB *lxb, LDB *ldb, LPB *lpb, LPD *lpd, 
				LG_TRANSACTION *tr);
static DB_STATUS
xa_term_tran(
i4	    abort,
i4	    data);


/*{
** Name: LGshow		- Show log information
**
** Description:
**      This routine allows the call to get information about the
**	log system.  The size of the return buffer must be at least
**	as large as the size of the object being returned.
**
** Inputs:
**      flag                            The information requested. Any
**					one of the following constants:
**					LG_S_STAT	- Show the statistics
**							  information of the
**							  logging system.
**					LG_A_HEADER	- Show the content
**							  of the log file
**							  header.
**					LG_S_DATABASE	- Show the information
**							  of database(s) in the
**							  logging system.
**					LG_N_DATABASE	- Show the information
**							  of the next database
**							  in the logging system.
**					LG_S_BACKUP  	- Show the information
**							  of the next database
**							  being backed up in
**							  the logging system.
**					LG_S_ADATABASE	- Show the information
**							  of a particular 
**							  database in the 
**							  logging system.
**					LG_S_TRANSACTION - Show the information
**							   of transaction(s) in 
**							   the logging system.
**					LG_N_TRANSACTION - Show the information
**							   of the next 
**							   transaction in the 
**							   logging system.
**					LG_S_ATRANSACTION - Show the information
**							    of a particular
**							    transaction in the
**							    logging system.
**					LG_S_ETRANSACTION - Show the information
**							    of a particular
**							    transaction given
**							    EXTERNAL tran id
**					LG_S_STRANSACTION - Show the information
**							    of a HANDLE LXB's
**							    SHARED transaction.
**					LG_S_PROCESS	  - Show the information
**							    of process(es) in 
**							    the logging system.
**					LG_S_APROCESS	  - Show the information
**							    of a particular 
**							    process in the 
**							    logging system.
**					LG_N_PROCESS	  - Show the information
**							    of the next process
**							    in the logging 
**							    system.
**					LG_A_BOF	  - Show the beginning
**							    address of the log 
**							    file.
**					LG_A_EOF	  - Show the end of file
**							    log address.
**					LG_A_CPA	  - Show the address of
**							    the last consistency
**							    point.
**					LG_A_LDBS	  - Show the number of
**							    LBK entries.
**					LG_S_DIS_TRAN	  - Show if a distributed
**							    transaction already
**							    exists in the logging
**							    system.
**					LG_S_MAN_COMMIT	  - Show the next transaction
**							    that needs to be manually
**							    committed.
**					LG_A_CONTEXT	  - Show the context of a 
**							    distributed transaction.
**					LG_A_RES_SPACE	  - Show reserved logfile
**							    Space.
**					LG_S_CSP_PID	  - CSP process ID.
**					LG_S_MUTEX	  - Show sem stats.
**					LG_S_OLD_LSN	  - Show oldest LSN
**					LG_S_LASTLSN	  - Show last LSN of txn
**					LG_S_XA_PREPARE	  - Associate an XA transaction
**							    for XA_PREPARE.
**					LG_S_XA_COMMIT	  - Associate an XA transaction
**							    for XA_COMMIT.
**					LG_S_XA_COMMIT_1PC  - Associate an XA transaction
**							    for one-phase XA_COMMIT.
**					LG_S_XA_ROLLBACK  - Associate an XA transaction
**							    for XA_ROLLBACK.
**					LG_S_XID_ARRAY_PTR- return pointer to
**							    lgd_xid_array.
**					LG_S_XID_ARRAY_SIZE - return size of
**							    lgd_xid_array.
**
**					LG_S_CRIB_BT	  
**					LG_S_CRIB_BS      - populate a LG_CRIB	  
**					LG_S_JFIB
**					LG_S_JFIB_DB	  - return the contents
**							    of a LDB's LG_JFIB
**
**					LG_S_RBBLOCKS	  - return the configured
**							    readbackward_blocks
**					LG_S_RFBLOCKS	  - return the configured
**							    readforward_blocks
**      item                            Pointer to buffer to return value.
**      l_item                          Length of the buffer.
**
** Outputs:
**      length                          Resulting length of item.
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM
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
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Add LPB_CPAGENT support.
**	11-jan-1993 (jnash)
**	    Reduced logging project.  Add LG_A_RES_SPACE, to show
**	    amount of reserved logfile space.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed LG_S_DBID to qualify database using the external database id
**		rather than the ldb_buffer structure.
**	    Consolidated the code to fill in tx and db info blocks into routines
**		format_db_info and format_tr_info.
**	    Removed LG_S_CDATABASE, LG_S_ABACKUP, LG_S_LDATABASE, LG_S_JNLDB,
**		and LG_S_PURJNLDB cases.
**	    Changed LG_S_DATABASE to return information about a specific
**		database identified by its lpd id.  The lpd id is now passed
**		in by the db.db_id field of the database structure.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster support:
**		Document the recording of the cluster node id in LG.
**		Expose the ldb_sback_lsn through the db_sback_lsn.
**		Add LG_S_CSP_PID to show the CSP's PID.
**	26-jul-1993 (bryanp)
**	    Clear the cl_err_desc field to avoid formatting garbage errors.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Dumped old lfb journal and dump windows and added new archive
**	    window information.
**	18-oct-1993 (rogerk)
**	    Changed lfb_reserved_space to be unsigned.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	08-Dec-1995 (jenjo02)
**	    Added LG_S_MUTEX function to LGshow() to return statistics
**	    about the logging system semaphore(s).
**	25-Jan-1995 (jenjo02)
**	    Lots of new mutexes added. Where possible, use SHARED
**	    semaphores or none at all.
**	25-Jan-1996 (jenjo02)
**	    Removed acquisition and release of lgd_mutex.
**	22-Apr-1996 (jenjo02)
**	    Added lgd|lgs_stat.no_logwriter count of number of times
**	    a logwriter thread was needed but none were available.
**	01-Oct-1996 (jenjo02)
**	    Added lgs_max_wq (max length of write queue) and
**	    lgs_max_wq_count (number of time that max hit).
**	25-Oct-1996 (jenjo02)
**	    In LG_S_OPENDB:LG_S_CLOSEDB, delay releasing ldb queue
**	    mutex until after lgd_status has been updated.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
** 	    Added LG_S_ETRANSACTION
**	24-feb-1997 (canor01)
**	    If LGshow is called before it has attached to the
**	    logging system, return a failure instead of a SEGV.
**	16-Dec-1997 (jenjo02)
**	    In LG_S_OPENDB don't return LDBs that are in a LDB_CLOSEDB_PEND
**	    state in addition to LDB_OPENDB_PEND.
**	17-Dec-1997 (jenjo02)
**	    Replaced idle LogWriter queue with a static queue of all
**	    LogWriter threads.
**	24-Aug-1998 (jenjo02)
**	    Rewrote LG_S_ETRANSACTION to find active transactions using
**	    the (new) active transaction hash queue.
**	25-aug-1998 (rigka01) bug #91054
**	    CP allowed to occur during writing of spanned log record
**	    causes CP address to be set to the middle of the record. 
**	    Cause a CP to wait until the EOF becomes legitimized by the
**	    completion of the writing of a record by taking and 
**	    releasing shared mutex on lfb_current_lbb_mutex in LG_A_EOF.
**	12-nov-1998 (kitch01)
**		Bug 90140. In LG_S_OPENDB, ignore any database that has the
**		LDB_CLOSE_WAIT flag set. This ensures that the database close
**		is completed before the next open is processed.
**	30-Sep-1999 (jenjo02)
**	    Added LG_S_OLD_LSN function to return the first LSN of
**	    the oldest active transaction.
**	14-Mar-2000 (jenjo02)
**	    Added LG_S_LASTLSN to return last written LSN of a transaction.
**	    Removed static LG_show() function and the unnecessary level of
**	    indirection it induced.
**	21-May-2002 (jenjo02)
**	    Added lost code for bug 91054 to block CP during spanned
**	    log write.
**	08-Jul-2002 (jenjo02)
**	    Rewrote LG_S_FORCE_ABORT to look directly at txn's LXB
**	    status rather than searching lgd_lxb_abort list; LXB may
**	    likely be in force abort state yet no longer on the list. (b108208)
**	02-May-2003 (jenjo02)
**	    Make sure that an LXB has a database connection pieces
**	    before calling format_tr_info() B110177, INGSRV2249.
**	22-Jul-2005 (jenjo02)
**	    Fix OPENDB/CLOSEDB race condition exposed by MAC port.
**	    Ensure that LDB's are mutexed before checking status.
**	5-Jan-2006 (kschendel)
**	    Reserved space totals have to be u_i8 for large logs.
**	15-Mar-2006 (jenjo02)
**	    Added LG_S_FORCED_LGA/LSN for logstat.
**	    lgd_stat.wait cleanup.
**	21-Mar-2006 (jenjo02)
**	    Add optimwrites, optimpages for optimized log writes.
**	26-Jun-2006 (jonj)
**	    Add LG_S_XA_PREPARE/COMMIT/COMMIT_1PC/ROLLBACK for
**	    XA integration.
**	01-Aug-2006 (jonj)
**	    Rewrite of LG_S_XA_? to better interpret the XA spec and
**	    TMJOINed associations.
**	24-Aug-2006 (jonj)
**	    If Global or Branch LXB is marked for abort, return
**	    "not found" (NOTA) - LG_S_XA_?.
**	11-Sep-2006 (jonj)
**	    Return lxb_status in tr_status to LG_S_FORCE_ABORT caller.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add LG_S_XID_ARRAY_PTR, LG_S_XID_ARRAY_SIZE,
**	    LG_S_CRIB_BT, LG_S_CRIB_BS, LG_S_JFIB, LG_S_JFIB_DB.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add LG_S_RBBLOCKS, LG_S_RFBLOCKS for
**	    buffered reads.
*/
STATUS
LGshow(
i4		    flag,
PTR		    item,
i4		    l_item,
i4		    *length,
CL_ERR_DESC	    *sys_err)
{
    LGD			*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LG_STAT		*stat;
    i4                 i, j;
    LG_DATABASE		*db;
    LG_TRANSACTION	*tr;
    LG_PROCESS		*pr;
    LG_BUFFER		*buf;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		ldb_offset;
    SIZE_TYPE		lxb_offset;
    SIZE_TYPE		lpb_offset;
    SIZE_TYPE		lbb_offset;
    LDB			*ldb;
    LPB			*lpb;
    LXB			*lxb, *MyLXB;
    LPD                 *lpd;
    LXB			*nlxb;
    LXB			*slxb;
    LPD                 *nlpd;
    LDB			*nldb;
    LBB			*lbb;
    LFB			*lfb;
    LG_MUTEX		*mstat;
    CS_SEM_STATS	*lstat;
    LG_I4ID_TO_ID	db_id;
    LG_I4ID_TO_ID	lx_id;
    LG_I4ID_TO_ID	lpb_id;
    LG_I4ID_TO_ID	lg_id;
    LG_I4ID_TO_ID	xid;
    SIZE_TYPE		*ldbb_table;
    SIZE_TYPE		*lxbb_table;
    SIZE_TYPE		*lpbb_table;
    SIZE_TYPE		*lpdb_table;
    SIZE_TYPE		*lfbb_table;
    i4		err_code;
    STATUS		status;
    LG_HDR_ALTER_SHOW	*hdr_struct;
    LG_RECOVER		*recov_struct;
    LG_LSN		*lsn;
    LXBQ		*lxbq;
    SIZE_TYPE		lxbq_offset;
    LG_CRIB		*crib;
    LG_JFIB		*jfib;
    u_i4		*xidArray;
    LG_LSN		low_lsn;
    LG_LSN		last_commit;
    LG_LSN		bos_lsn;
    u_i2		lgid_low;
    u_i2		lgid_high;

    CL_CLEAR_ERR(sys_err);

    if ( lgd == NULL )
	return ( LG_BADPARAM );

    switch (flag)
    {
    case LG_S_STAT:
    case LG_S_SAMP_STAT:
	if (l_item < sizeof(LG_STAT))
	{
	    uleFormat(NULL, E_DMA41D_LGSHOW_STAT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(LG_STAT));
	    return (LG_BADPARAM);
	}
	*length = sizeof(lgd->lgd_stat);
	stat = (LG_STAT *)item;

	if ( flag == LG_S_SAMP_STAT )
	{
	    /* 
	    ** This is the sampler calling. Quickly return
	    ** only what it is interested in.
	    */
	    stat->lgs_readio  = lgd->lgd_stat.readio;
	    stat->lgs_writeio = lgd->lgd_stat.writeio;
	    stat->lgs_kbytes  = lgd->lgd_stat.kbytes;
	    stat->lgs_end     = lgd->lgd_stat.end;
	}
	else
	{
	    stat->lgs_begin = lgd->lgd_stat.begin;
	    stat->lgs_end = lgd->lgd_stat.end;
	    stat->lgs_add = lgd->lgd_stat.add;
	    stat->lgs_remove = lgd->lgd_stat.remove;
	    stat->lgs_write = lgd->lgd_stat.write;
	    stat->lgs_readio = lgd->lgd_stat.readio;
	    stat->lgs_writeio= lgd->lgd_stat.writeio;
	    stat->lgs_force = lgd->lgd_stat.force;
	    stat->lgs_split = lgd->lgd_stat.split;
	    stat->lgs_group = lgd->lgd_stat.group_force;
	    stat->lgs_group_count = lgd->lgd_stat.group_count;
	    stat->lgs_inconsist_db = lgd->lgd_stat.inconsist_db;
	    stat->lgs_check_timer = lgd->lgd_stat.pgyback_check;
	    stat->lgs_timer_write = lgd->lgd_stat.pgyback_write;
	    stat->lgs_timer_write_time = lgd->lgd_stat.pgyback_write_time;
	    stat->lgs_timer_write_idle = lgd->lgd_stat.pgyback_write_idle;
	    stat->lgs_kbytes = lgd->lgd_stat.kbytes;
	    stat->lgs_log_readio = lgd->lgd_stat.log_readio;
	    stat->lgs_dual_readio = lgd->lgd_stat.dual_readio;
	    stat->lgs_log_writeio = lgd->lgd_stat.log_writeio;
	    stat->lgs_dual_writeio = lgd->lgd_stat.dual_writeio;
	    stat->lgs_optimwrites = lgd->lgd_stat.optimwrites;
	    stat->lgs_optimpages = lgd->lgd_stat.optimpages;
	    stat->lgs_no_logwriter = lgd->lgd_stat.no_logwriter;
	    stat->lgs_max_wq = lgd->lgd_stat.max_wq;
	    stat->lgs_max_wq_count = lgd->lgd_stat.max_wq_count;
	    stat->lgs_buf_util_sum = lgd->lgd_stat.buf_util_sum;
	    MEcopy((PTR)&lgd->lgd_stat.wait[0],
		   sizeof(lgd->lgd_stat.wait),
		   (PTR)&stat->lgs_wait[0]);
	    MEcopy((PTR)&lgd->lgd_stat.buf_util[0],
		   sizeof(lgd->lgd_stat.buf_util),
		   (PTR)&stat->lgs_buf_util[0]);

	    /* stat_misc is to be used to add stat's while debuggin performance
	    ** problems.  Stat's that are to be maintained officially can be
	    ** added by name into the stat struct, and the code recompiled to
	    ** accept the new struct.  stat_misc is a convenience to make it 
	    ** easy to add stats to lg and get them out from LGshow().
	    */
	    MEcopy((PTR)&lgd->lgd_stat.stat_misc[0], 
		   sizeof(lgd->lgd_stat.stat_misc), 
		   (PTR)&stat->lgs_stat_misc[0]);
	}

	break;

    case LG_A_HEADER:
	if (l_item < sizeof(LG_HEADER))
	{
	    uleFormat(NULL, E_DMA41E_LGSHOW_HDR_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(LG_HEADER));
	    return (LG_BADPARAM);
	}
	*length = sizeof(LG_HEADER);
	*(LG_HEADER *)item = lgd->lgd_local_lfb.lfb_header;
	break;

    case LG_A_NODELOG_HEADER:

	/*
	** Return the header for a specific node's log. The particular  node
	** to use is indicated by the LG-ID in the LG_HDR_ALTER_SHOW structure
	** passed to us.
	*/
	if (l_item != sizeof(LG_HDR_ALTER_SHOW))
	{
	    uleFormat(NULL, E_DMA41E_LGSHOW_HDR_SIZE, (CL_ERR_DESC *)NULL,
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

	*length = sizeof(LG_HEADER);
	hdr_struct->lg_hdr_lg_header = lfb->lfb_header;
	break;

    case LG_S_ACP_START:
	if (l_item < sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_S_ACP_START");
	    return (LG_BADPARAM);
	}
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_archive_window_start;
	break;

    case LG_S_ACP_END:
	if (l_item < sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_S_ACP_END");
	    return (LG_BADPARAM);
	}
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_archive_window_end;
	break;

    case LG_S_ACP_CP:
	if (l_item < sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_S_ACP_CP");
	    return (LG_BADPARAM);
	}
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_archive_window_prevcp;
	break;

    case LG_S_LGSTS:
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(i4));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_status;
	break;

    case LG_A_BCNT:
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA421_LGSHOW_BCNT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(i4));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_local_lfb.lfb_buf_cnt;
	break;

    case LG_S_STAMP:
	if ( (l_item + sizeof(i4)) < sizeof(LG_LSN))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LSN), 0, "LG_S_STAMP");
	    return (LG_BADPARAM);
	}

	if ( lgd->lgd_status & LGD_CLUSTER_SYSTEM )
	{
	    /* 
	    ** Use the cluster wide log page time stamp as the 
	    ** cluster wide commit time stamp for the 
	    ** commit transaction
	    */
#if 0
	    *(i4 *)item = lgc_stamp[0];
	    *(i4 *)&item[4] = lgc_stamp[1];
#endif
	}	    
	else
	{
	    /* Return the last LSN, not LGA */
	    *(LG_LSN *)item = lgd->lgd_local_lfb.lfb_header.lgh_last_lsn;
	}
	break;

    case LG_A_NODEID:
	/*
	** The logging system makes the cluster node ID of this local node
	** available to clients through this LGshow call. Clients use the
	** node ID to access the correct node journal files, to manage the
	** dsc_open_count bitmask, etc.
	*/
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA422_LGSHOW_NODEID_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(i4));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = CXnode_number(NULL);
	break;

    case LG_S_OPENDB:
    case LG_S_CLOSEDB:
    case LG_S_BACKUP:
	{
	    
	    i4		lgd_status;

	    i = l_item / sizeof(LG_DATABASE);
	    *length = i * sizeof(LG_DATABASE);
       
	    db = (LG_DATABASE *)item;

	    /*
	    ** Assume we'll find no databases in OPENDB_PEND or CLOSEDB_PEND
	    ** state.
	    **
	    ** Note that while we're doing this, LDBs which we've 
	    ** passed by which were not in either state may, by the
	    ** time we're done, now be in OPENDB/CLOSEDB and
	    ** the appropriate lgd_status bits set.
	    ** This being the case, we don't want to turn off 
	    ** LGD_OPENDB/LGD_CLOSEDB if we didn't find any ourselves;
	    ** just leave those bits as-is so the next event wait
	    ** won't.
	    */
	    LG_mutex(SEM_EXCL, &lgd->lgd_mutex);
	    lgd->lgd_status &= ~(LGD_OPENDB | LGD_CLOSEDB);
	    (VOID)LG_unmutex(&lgd->lgd_mutex);

	    lgd_status = 0;

	    ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	    for ( j = 1; j <= lgd->lgd_ldbb_count; j++ )
	    {
		ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[j]);
		
		if ( status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex) )
		    return(status);

		if ( ldb->ldb_type == LDB_TYPE && 
			(ldb->ldb_status & LDB_NOTDB) == 0 )
		{
		    /*
		    ** We must continue checking all LDBs for OPENDB/CLOSEDB 
		    ** pending even if "i" becomes zero.
		    */
		    if ( ldb->ldb_status & LDB_OPENDB_PEND )
			lgd_status |= LGD_OPENDB;
		    if ( ldb->ldb_status & LDB_CLOSEDB_PEND )
			lgd_status |= LGD_CLOSEDB;

		    /*  Check for restriction on database types. */

		    /*
		    ** If looking for OPENDB_PENDing databases, ignore those
		    ** which have been already marked for close. This race
		    ** condition can occur if the DB opener is a NOLOGGING
		    ** transaction and therefore won't try to write an OPENDB
		    ** log record, which would cause it to stall waiting for
		    ** the RCP to complete the database open. In this case,
		    ** the FE/Server may get ahead of the RCP and close the
		    ** database before the open has been handled.
		    */
		    if ( i > 0 &&
			   ( (flag == LG_S_OPENDB &&
				(ldb->ldb_status & (LDB_OPENDB_PEND |
						    LDB_OPN_WAIT | 
						    LDB_CLOSEDB_PEND |
						    LDB_CLOSE_WAIT)) 
				    == LDB_OPENDB_PEND) ||
			     (flag == LG_S_CLOSEDB &&
				 ldb->ldb_status &  LDB_CLOSEDB_PEND) ||
			     (flag == LG_S_BACKUP &&
				 ldb->ldb_status &  LDB_BACKUP) ) )
		    {
			format_db_info(ldb, db);

			db++;
			i--;
		    }
		}
		/* Release the LDB */
		(VOID)LG_unmutex(&ldb->ldb_mutex);
	    }

	    /* If any found, set the lgd_status bits */
	    if ( lgd_status )
	    {
		LG_mutex(SEM_EXCL, &lgd->lgd_mutex);
		lgd->lgd_status |= lgd_status;
		(VOID)LG_unmutex(&lgd->lgd_mutex);
	    }

	    /* *length == 0 if none were found */
	    *length -= i * sizeof (LG_DATABASE);

	    break;
	}

    case LG_N_DATABASE:
	if (l_item < sizeof(LG_DATABASE))
	{
	    uleFormat(NULL, E_DMA423_LGSHOW_DB_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DATABASE), 0, "LG_N_DATABASE");
	    return (LG_BADPARAM);
	}

	/* *length denotes the index to the lgd_ldbb_table to get the next LDB */

	i = *length + 1;
	*length = 0;	    /* Assume no more next */

	for (; i <= lgd->lgd_ldbb_count; i++)	
	{
	    db = (LG_DATABASE *)item;
    
	    ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[i]);
	    if (ldb->ldb_type == LDB_TYPE)
	    {
		format_db_info(ldb, db);
		*length = i;
		break;
	    }
	}

	break;

    case LG_S_DATABASE:
	/*
	** Show the information of a particular database identified by
	** its internal lpd id.
	**
	** Inputs:
	**	db.db_id	- lpd of requested database.
	*/

        *length = 0;
        if (l_item < sizeof(LG_DATABASE))
	{
	    uleFormat(NULL, E_DMA423_LGSHOW_DB_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DATABASE), 0, "LG_S_DATABASE");
	    return (LG_BADPARAM);
	}

	db = (LG_DATABASE *)item;
	db_id.id_i4id = db->db_id;

	/*  Check the db_id. */
	if (db_id.id_lgid.id_id == 0 || (i4) db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, "LG_S_DATABASE");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

	if (lpd->lpd_type != LPD_TYPE ||
	    lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA425_LGSHOW_BAD_DB, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
			0, lpd->lpd_type, 0, lpd->lpd_id.id_instance,
			0, db_id.id_lgid.id_instance,
			0, "LG_S_DATABASE");
	    return (LG_BADPARAM);
	}
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	format_db_info(ldb, db);
	*length = sizeof (LG_DATABASE);

        break;

    case LG_S_ADATABASE:
	if (l_item < sizeof(LG_DATABASE))
	{
	    uleFormat(NULL, E_DMA423_LGSHOW_DB_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DATABASE), 0, "LG_S_ADATABASE");
	    return (LG_BADPARAM);
	}
	*length = 0;

	/* Show the information of a particular database. */

	db_id.id_i4id = *(i4 *) item;
	db = (LG_DATABASE *)item;

	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb  = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[db_id.id_lgid.id_id]);
	
	if (ldb->ldb_id.id_id == db_id.id_lgid.id_id && 
		ldb->ldb_id.id_instance == db_id.id_lgid.id_instance)
	{
	    format_db_info(ldb, db);
	    *length = sizeof (LG_DATABASE);
	}
   
	break;

    case LG_S_TRANSACTION:
	tr = (LG_TRANSACTION *)item;
	i = l_item / sizeof(LG_TRANSACTION);
	*length = i * sizeof(LG_TRANSACTION);

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);

	for (j = 1; 
	     i && j <= lgd->lgd_lxbb_count;
	     j++, tr++)
	{
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[j]);
	    if ( lxb->lxb_type == LXB_TYPE )
	    {
		if ( lxb->lxb_lpd )
		{
		    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
		    if ( lpd->lpd_ldb && lpd->lpd_lpb )
		    {
			ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
			lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
			format_tr_info(lxb, ldb, lpb, lpd, tr);
			i--;
		    }
		}
	    }
	}

	*length -= i * sizeof (LG_TRANSACTION);
	break;

    case LG_N_TRANSACTION:
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, "LG_N_TRANSACTION");
	    return (LG_BADPARAM);
	}

	/* *length denotes the index to the lgd_lxbb_table to get the next LXB */

	i = *length + 1;
	*length = 0;	    /* Assume no more next */

	for (; i <= lgd->lgd_lxbb_count; i++)	
	{
	    tr = (LG_TRANSACTION *)item;
    
	    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
	    if ( lxb->lxb_type == LXB_TYPE )
	    {
		if ( lxb->lxb_lpd )
		{
		    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
		    if ( lpd->lpd_ldb && lpd->lpd_lpb )
		    {
			ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
			lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
			format_tr_info(lxb, ldb, lpb, lpd, tr);
			*length = i;
			break;
		    }
		}
	    }
	}
	break;

    case LG_S_ATRANSACTION:
    case LG_S_STRANSACTION:
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, "LG_S_ATRANSACTION/LG_S_STRANSACTION");
	    return (LG_BADPARAM);
	}

	*length = 0;

	/* Show the information of a particular transaction. */
   
	lx_id.id_i4id = *(i4 *) item;
	tr = (LG_TRANSACTION *) item;
	if (lx_id.id_lgid.id_id == 0 || (i4) lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
		    0, "LG_S_ATRANSACTION/LG_S_STRANSACTION");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb  = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);
	
	if (lxb->lxb_type == LXB_TYPE &&
	    lxb->lxb_id.id_id == lx_id.id_lgid.id_id && 
		lxb->lxb_id.id_instance == lx_id.id_lgid.id_instance)
	{
	    /* Switch to SHARED LXB if that's what's wanted */
	    if ( flag == LG_S_STRANSACTION &&
		 lxb->lxb_status & LXB_SHARED_HANDLE )
	    {
		if ( lxb->lxb_shared_lxb == 0 )
		{
		    xid.id_lgid = lxb->lxb_id;
		    uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, xid.id_i4id,
			0, lxb->lxb_status,
			0, "LG_S_STRANSACTION");
		    return (LG_BADPARAM);
		}

		lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

		if ( lxb->lxb_type != LXB_TYPE ||
		     (lxb->lxb_status & LXB_SHARED) == 0 )
		{
		    xid.id_lgid = lxb->lxb_id;
		    uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lxb->lxb_type,
			0, LXB_TYPE,
			0, xid.id_i4id,
			0, lxb->lxb_status,
			0, lx_id.id_i4id,
			0, "LG_S_STRANSACTION");
		    return (LG_BADPARAM);
		}
	    }

	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);
	    format_tr_info(lxb, ldb, lpb, lpd, tr);
	    *length = sizeof(LG_TRANSACTION);
	}
   
	break;

    case LG_S_LASTLSN:
	if (l_item < sizeof(LG_LSN))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LSN),
			0, "LG_S_LASTLSN");
	    return (LG_BADPARAM);
	}

	*length = 0;

	/* Show the last written LSN of a particular transaction. */
   
	lx_id.id_i4id = *(i4 *) item;
	if (lx_id.id_lgid.id_id == 0 || (i4) lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
		    0, "LG_S_LASTLSN");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb  = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	lsn = (LG_LSN *)item;
	*length = sizeof(LG_LSN);
	
	if (lxb->lxb_type == LXB_TYPE &&
	    lxb->lxb_id.id_id == lx_id.id_lgid.id_id && 
		lxb->lxb_id.id_instance == lx_id.id_lgid.id_instance)
	{
	    /* Switch to SHARED LXB */
	    if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	    {
		if ( lxb->lxb_shared_lxb == 0 )
		{
		    xid.id_lgid = lxb->lxb_id;
		    uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, xid.id_i4id,
			0, lxb->lxb_status,
			0, "LG_S_LASTLSN");
		    return (LG_BADPARAM);
		}

		lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

		if ( lxb->lxb_type != LXB_TYPE ||
		     (lxb->lxb_status & LXB_SHARED) == 0 )
		{
		    xid.id_lgid = lxb->lxb_id;
		    uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lxb->lxb_type,
			0, LXB_TYPE,
			0, xid.id_i4id,
			0, lxb->lxb_status,
			0, lx_id.id_i4id,
			0, "LG_S_LASTLSN");
		    return (LG_BADPARAM);
		}
	    }

	    /* Return last written LSN of transaction */
	    (VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);
	    *lsn = lxb->lxb_last_lsn;
	    (VOID)LG_unmutex(&lxb->lxb_mutex);
	}
   
	break;

    case LG_S_OLD_LSN:
	if (l_item < sizeof(LG_LSN))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LSN),
			0, "LG_S_OLD_LSN");
	    return (LG_BADPARAM);
	}

	lsn = (LG_LSN *)item;
	*length = sizeof(LG_LSN);

	/* Extract the first LSN of the oldest active transaction */
	do
	{
	    *lsn = lgd->lgd_local_lfb.lfb_header.lgh_last_lsn;

	    lxb_offset = lgd->lgd_lxb_next;

	    if ( lxb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next) )
	    {
		lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);
		*lsn = lxb->lxb_first_lsn;
	    }
	/* Repeat until we get a stable oldest transaction... */
	} while ( lxb_offset != lgd->lgd_lxb_next );
   
	break;

    case LG_S_FORCE_ABORT:
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, "LG_S_FORCE_ABORT");
	    return (LG_BADPARAM);
	}

	*length = 0;
	tr = (LG_TRANSACTION *) item;
	lg_id.id_i4id = tr->tr_id;

	/*	Check the lg_id. */

	if (lg_id.id_lgid.id_id == 0 || (i4) lg_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_S_FORCE_ABORT");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lg_id.id_lgid.id_id]);

	if ( lxb->lxb_status & LXB_ABORT &&
	     tr->tr_eid.db_high_tran == 
		lxb->lxb_lxh.lxh_tran_id.db_high_tran &&
	     tr->tr_eid.db_low_tran == 
		lxb->lxb_lxh.lxh_tran_id.db_low_tran )
	{
	    /* Return transaction status for caller to ponder */
	    tr->tr_status = lxb->lxb_status;
	    *length = sizeof(LG_TRANSACTION);
	}
	break;

    case LG_S_PROCESS:
	pr = (LG_PROCESS *)item;
	i = l_item / sizeof(LG_PROCESS);
	*length = i * sizeof(LG_PROCESS);

	if (status = LG_mutex(SEM_SHARE, &lgd->lgd_lpb_q_mutex))
	    return(status);

	end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);

	for (lpb_offset = lgd->lgd_lpb_prev;
	    lpb_offset != end_offset;
	    lpb_offset = lpb->lpb_prev, pr++)
	{
	    if (i == 0)
		break;
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
	    if (lpb->lpb_type != LPB_TYPE)
		continue;
	    i--;
	    xid.id_lgid = lpb->lpb_id;
	    pr->pr_id = xid.id_i4id;
	    pr->pr_pid = lpb->lpb_pid;
	    pr->pr_bmid = 0;
	    if (lpb->lpb_status & LPB_SHARED_BUFMGR)
		pr->pr_bmid = lpb->lpb_bufmgr_id;
	    pr->pr_type = 0;
	    if (lpb->lpb_status & LPB_MASTER)
		pr->pr_type |= PR_MASTER;
	    if (lpb->lpb_status & LPB_CKPDB)
		pr->pr_type = PR_CKPDB;
	    if (lpb->lpb_status & LPB_ARCHIVER)
		pr->pr_type |= PR_ARCHIVER;
	    if (lpb->lpb_status & LPB_FCT)
		pr->pr_type |= PR_FCT;
	    if (lpb->lpb_status & LPB_RUNAWAY)
		pr->pr_type |= PR_RUNAWAY;
	    if (lpb->lpb_status & LPB_SLAVE)
		pr->pr_type |= PR_SLAVE;
	    if (lpb->lpb_status & LPB_CPAGENT)
		pr->pr_type |= PR_CPAGENT;
	    pr->pr_stat.database = lpb->lpb_lpd_count;
	    pr->pr_stat.write = lpb->lpb_stat.write;
	    pr->pr_stat.force = lpb->lpb_stat.force;
	    pr->pr_stat.wait = lpb->lpb_stat.wait;
	    pr->pr_stat.begin = lpb->lpb_stat.begin;
	    pr->pr_stat.end = lpb->lpb_stat.end;
	}
	(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
	*length -= i * sizeof (LG_PROCESS);
	break;

    case LG_S_APROCESS:
	if (l_item < sizeof(LG_PROCESS))
	{
	    uleFormat(NULL, E_DMA429_LGSHOW_PROC_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_PROCESS),
			0, "LG_S_APROCESS");
	    return (LG_BADPARAM);
	}
	*length = 0;

	/* Show the information of a particular proces. */

	lpb_id.id_i4id = *(i4 *)item;
	pr = (LG_PROCESS *) item;

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb  = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lpb_id.id_lgid.id_id]);
	
	if (lpb->lpb_type == LPB_TYPE &&
	    lpb->lpb_id.id_id == lpb_id.id_lgid.id_id && 
		lpb->lpb_id.id_instance == lpb_id.id_lgid.id_instance)
	{
	    xid.id_lgid = lpb->lpb_id;
	    pr->pr_id = xid.id_i4id;
	    pr->pr_pid = lpb->lpb_pid;
	    pr->pr_bmid = 0;
	    if (lpb->lpb_status & LPB_SHARED_BUFMGR)
		pr->pr_bmid = lpb->lpb_bufmgr_id;
	    pr->pr_type = 0;
	    if (lpb->lpb_status & LPB_MASTER)
		pr->pr_type = PR_MASTER;
	    if (lpb->lpb_status & LPB_CKPDB)
		pr->pr_type = PR_CKPDB;
	    pr->pr_stat.database = lpb->lpb_lpd_count;
	    pr->pr_stat.write = lpb->lpb_stat.write;
	    pr->pr_stat.force = lpb->lpb_stat.force;
	    pr->pr_stat.wait = lpb->lpb_stat.wait;
	    pr->pr_stat.begin = lpb->lpb_stat.begin;
	    pr->pr_stat.end = lpb->lpb_stat.end;
	    *length = sizeof(LG_PROCESS);
	}
	break;

    case LG_N_PROCESS:
	if (l_item < sizeof(LG_PROCESS))
	{
	    uleFormat(NULL, E_DMA429_LGSHOW_PROC_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_PROCESS),
			0, "LG_N_PROCESS");
	    return (LG_BADPARAM);
	}

	/* *length denotes the index to the lgd_lpbb_table to get the next LPB */

	i = *length + 1;
	*length = 0;	    /* Assume no more next */

	for (; i <= lgd->lgd_lpbb_count; i++)	
	{
	    pr = (LG_PROCESS *)item;
	    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	    lpb  = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[i]);
    
	    if (lpb->lpb_type == LPB_TYPE)
	    {
		xid.id_lgid = lpb->lpb_id;
		pr->pr_id = xid.id_i4id;
		pr->pr_pid = lpb->lpb_pid;
		pr->pr_bmid = 0;
		if (lpb->lpb_status & LPB_SHARED_BUFMGR)
		    pr->pr_bmid = lpb->lpb_bufmgr_id;
		pr->pr_type = 0;
		if (lpb->lpb_status & LPB_MASTER)
		    pr->pr_type = PR_MASTER;
                if (lpb->lpb_status & LPB_ARCHIVER)
                    pr->pr_type |= PR_ARCHIVER;
                if (lpb->lpb_status & LPB_CKPDB)
                    pr->pr_type |= PR_CKPDB;
                if (lpb->lpb_status & LPB_FCT)
                    pr->pr_type |= PR_FCT;
                if (lpb->lpb_status & LPB_RUNAWAY)
                    pr->pr_type |= PR_RUNAWAY;
                if (lpb->lpb_status & LPB_SLAVE)
                    pr->pr_type |= PR_SLAVE;
		if (lpb->lpb_status & LPB_VOID)
		    pr->pr_type |= PR_VOID;
		if (lpb->lpb_status & LPB_SHARED_BUFMGR)
		    pr->pr_type |= PR_SBM;
		if (lpb->lpb_status & LPB_IDLE)
		    pr->pr_type |= PR_IDLE;
		if (lpb->lpb_status & LPB_DEAD)
		    pr->pr_type |= PR_DEAD;
		if (lpb->lpb_status & LPB_DYING)
		    pr->pr_type |= PR_DYING;
		if (lpb->lpb_status & LPB_CPAGENT)
		    pr->pr_type |= PR_CPAGENT;
		pr->pr_stat.database = lpb->lpb_lpd_count;
		pr->pr_stat.write = lpb->lpb_stat.write;
		pr->pr_stat.force = lpb->lpb_stat.force;
		pr->pr_stat.wait = lpb->lpb_stat.wait;
		pr->pr_stat.begin = lpb->lpb_stat.begin;
		pr->pr_stat.end = lpb->lpb_stat.end;
		*length = i;
		break;
	    }
	}
	break;

    case LG_A_BOF:
	if (l_item != sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_A_BOF");
	    return (LG_BADPARAM);
	}
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_header.lgh_begin;
	*length = sizeof(LG_LA);
	break;

    case LG_A_EOF:
	if (l_item != sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_A_EOF");
	    return (LG_BADPARAM);
	}
	/*
	** Mutex current LBB to ensure that EOF is correct in
	** case a spanned write is in progress (Bug 91054).
	*/
	while (lbb = (LBB *)
		LGK_PTR_FROM_OFFSET(lgd->lgd_local_lfb.lfb_current_lbb))
        {
	   if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
              return status;

           if ( lbb == (LBB *)LGK_PTR_FROM_OFFSET(lgd->lgd_local_lfb.lfb_current_lbb) )
           {
	      /* current_lbb is still the "CURRENT" lbb. */
              break;
           }

           (VOID)LG_unmutex(&lbb->lbb_mutex);

#ifdef xDEBUG
           TRdisplay( "LGshow(1):: Scenario for INGSRV2838 handled. EOF=<%d, %d, %d>\n",
                      lgd->lgd_local_lfb.lfb_header.lgh_end.la_sequence,
                      lgd->lgd_local_lfb.lfb_header.lgh_end.la_block,
                      lgd->lgd_local_lfb.lfb_header.lgh_end.la_offset);
#endif
        }
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_header.lgh_end;
	LG_unmutex(&lbb->lbb_mutex);

	*length = sizeof(LG_LA);
	break;

    case LG_A_CPA:
	if (l_item != sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_A_CPA");
	    return (LG_BADPARAM);
	}
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_header.lgh_cp;
	*length = sizeof(LG_LA);
	break;

    case LG_A_LDBS:
	if (l_item != sizeof(i4))
	{
	    uleFormat(NULL, E_DMA42A_LGSHOW_LDBS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(i4));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_ldbb_size;
	*length = sizeof(i4);
	break;

    case LG_S_DIS_TRAN:
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, "LG_S_DIS_TRAN");
	    return (LG_BADPARAM);
	}

	tr = (LG_TRANSACTION *)item;
	*length = 0;
	lg_id.id_i4id = tr->tr_id;

	/*	Check the lg_id. */

	if (lg_id.id_lgid.id_id == 0 || (i4) lg_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			0, "LG_S_DIS_TRAN");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	nlxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lg_id.id_lgid.id_id]);

	/* 
	** If handle to a SHARED LXB, we expect this dis_tran_id
	** to be in the logging system already, but the transaction
	** should not be PREPAREd.
	*/
	if ( nlxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    if ( nlxb->lxb_status & LXB_PREPARED )
	    {
		*length = sizeof(LG_TRANSACTION);
	    }
	    break;
	}

	nlpd = (LPD *)LGK_PTR_FROM_OFFSET(nlxb->lxb_lpd);
	nldb = (LDB *)LGK_PTR_FROM_OFFSET(nlpd->lpd_ldb);

	if (status = LG_mutex(SEM_SHARE, &lgd->lgd_lxb_q_mutex))
	    return(status);

	end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);

	/*
	** Find matching active transaction matching both
	** the distributed txn id and database id.
	*/
	for (lxb_offset = lgd->lgd_lxb_next;
	     lxb_offset != end_offset; lxb_offset = lxb->lxb_next)
	{
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);
	    if (lxb->lxb_type == LXB_TYPE &&
	        DB_DIS_TRAN_ID_EQL_MACRO(lxb->lxb_dis_tran_id ,
					    tr->tr_dis_id) &&
		(lxb->lxb_id.id_id != lg_id.id_lgid.id_id ||
		    lxb->lxb_id.id_instance != lg_id.id_lgid.id_instance)
		)
	    {
		lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
		ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

		if (ldb->ldb_id.id_id == nldb->ldb_id.id_id && 
		    ldb->ldb_id.id_instance == nldb->ldb_id.id_instance)
		{
		    *length = sizeof(LG_TRANSACTION);
		    break;
		}
	    }
	}
	(VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	break;

    case LG_S_DBID:
	/*
	** Show the information of a particular database identified by
	** its external database id.
	**
	** Inputs:
	**	db.db_database_id	- external dbid of requested db
	*/
	if (l_item < sizeof(LG_DATABASE))
	{
	    uleFormat(NULL, E_DMA423_LGSHOW_DB_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_DATABASE), 0, "LG_S_DBID");
	    return (LG_BADPARAM);
	}
	*length = 0;
	db = (LG_DATABASE *)item;

	if (status = LG_mutex(SEM_SHARE, &lgd->lgd_ldb_q_mutex))
	    return(status);

	end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);

	for (ldb_offset = lgd->lgd_ldb_next;
	    ldb_offset != end_offset;
	    ldb_offset = ldb->ldb_next)
	{
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);
	    if (ldb->ldb_type != LDB_TYPE ||
	        ldb->ldb_database_id != db->db_database_id)
		continue;

	    format_db_info(ldb, db);
	    *length = sizeof(LG_DATABASE);
	    break;
	}
	(VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);
	break;

    case LG_A_CONTEXT:
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, "LG_A_CONTEXT");
	    return (LG_BADPARAM);
	}

	tr = (LG_TRANSACTION *)item;
	db_id.id_i4id = tr->tr_db_id;

	*length = 0;

	if (status = LG_mutex(SEM_SHARE, &lgd->lgd_lxb_q_mutex))
	    return(status);

	end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);

	for (lxb_offset = lgd->lgd_lxb_next;
	     lxb_offset != end_offset;
	     lxb_offset = lxb->lxb_next)
	{
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);
	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

	    if (lxb->lxb_type == LXB_TYPE &&
	        DB_DIS_TRAN_ID_EQL_MACRO(lxb->lxb_dis_tran_id ,
					    tr->tr_dis_id) &&
		ldb->ldb_id.id_id == db_id.id_lgid.id_id &&
		ldb->ldb_id.id_instance == db_id.id_lgid.id_instance)
	    {
		/* Only return necessary information. */

		format_tr_info(lxb, ldb, lpb, lpd, tr);
		*length = sizeof(LG_TRANSACTION);
		break;
	    }
	}
	(VOID)LG_unmutex(&lgd->lgd_lxb_q_mutex);
	break;

    case LG_S_ETRANSACTION:
	{
	    LXBQ	*bucket_array;
	    LXBQ	*lxbq;
	    LXH		*lxh;
	    i4	lxh_offset;

	    /*
	    ** Determine if a transaction is active in the
	    ** logging system.
	    **
	    ** Inputs:
	    **		tr_eid 	transaction of interest
	    ** Outputs:
	    **		*length zero if transaction of interest
	    **			is not known to the logging system
	    **			non-zero if transaction is active.
	    */
	    if (l_item < sizeof(LG_TRANSACTION))
	    {
		uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, l_item, 0, sizeof(LG_TRANSACTION),
			    0, "LG_S_ETRANSACTION");
		return (LG_BADPARAM);
	    }

	    *length = 0;
	    tr = (LG_TRANSACTION *)item;

	    bucket_array = (LXBQ *)LGK_PTR_FROM_OFFSET(
				lgd->lgd_lxh_buckets);
		
	    lxbq = &bucket_array[tr->tr_eid.db_low_tran % lgd->lgd_lxbb_size];

	    end_offset = LGK_OFFSET_FROM_PTR(&lxbq->lxbq_next);

	    /*
	    ** New active transactions are added to the end of the hash list,
	    ** search from the back.
	    */
	    lxh_offset = lxbq->lxbq_prev;

	    while (lxh_offset != end_offset)
	    {
		lxh = (LXH *)LGK_PTR_FROM_OFFSET(lxh_offset);
		lxh_offset = lxh->lxh_lxbq.lxbq_prev;

		if (tr->tr_eid.db_low_tran == lxh->lxh_tran_id.db_low_tran)
		{
		    *length = sizeof(LG_TRANSACTION);
		    break;
		}

		/*
		** Because we're reading the hash list dirty, recheck the
		** current LXB and LXH; if anything's changed, restart
		** from the end of the list.
		*/
		lxb = (LXB *)((char *)lxh - CL_OFFSETOF(LXB,lxb_lxh));
		
		if (lxb->lxb_type != LXB_TYPE ||
		   (lxb->lxb_status & LXB_ACTIVE) == 0 ||
		   lxh_offset != lxh->lxh_lxbq.lxbq_prev)
		{
		    lxh_offset = lxbq->lxbq_prev;
		}
	    }

	    break;
	}

    case LG_S_MAN_COMMIT:
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, "LG_S_MAN_COMMIT");
	    return (LG_BADPARAM);
	}

	tr = (LG_TRANSACTION *)item;
	*length = 0;

	lxbb_table = (SIZE_TYPE*)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);

	for ( i = 1; *length == 0 && i <= lgd->lgd_lxbb_count; i++ )
	{
	    lxb = (LXB*)LGK_PTR_FROM_OFFSET(lxbb_table[i]);

	    if ( lxb->lxb_type == LXB_TYPE &&
		 lxb->lxb_status & LXB_MAN_COMMIT )
	    {
		(VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);

		if ( lxb->lxb_type == LXB_TYPE &&
		     lxb->lxb_status & LXB_MAN_COMMIT )
		{
		    /* Only return the necessary information. */
		    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
		    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
		    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

		    format_tr_info(lxb, ldb, lpb, lpd, tr);
		    *length = sizeof(LG_TRANSACTION);
		}
		(VOID)LG_unmutex(&lxb->lxb_mutex);
	    }
	}
	break;		

    case LG_S_DUAL_LOGGING:
	if (l_item < sizeof(i4))
	    return (LG_BADPARAM);
	*(i4 *)item = lgd->lgd_local_lfb.lfb_active_log;
	break;

    case LG_S_TP_VAL:
	/*
	** Automated testpoint support. Return the value of a particular
	** testpoint, whose testpoint number is INPUT to LGshow in *length.
	*/
	if (l_item < sizeof(i4) || (*length) < 0 ||
	    (*length) >= LGD_MAX_NUM_TESTS)
	{
	    return (LG_BADPARAM);
	}
	*(i4 *)item = BTtest((i4)(*length),(char *)lgd->lgd_test_bitvec);

	break;

    case LG_S_TEST_BADBLK:
	if (l_item < sizeof(i4))
	    return (LG_BADPARAM);
	*(i4 *)item = lgd->lgd_test_badblk;
	break;

    case LG_S_FBUFFER:
	buf = (LG_BUFFER *)item;
	i = l_item / sizeof(LG_BUFFER);
	*length = i * sizeof(LG_BUFFER);

	if (status = LG_mutex(SEM_SHARE, &lgd->lgd_local_lfb.lfb_fq_mutex))
	    return(status);

	if (lgd->lgd_local_lfb.lfb_fq_count)
	{
	    for (lbb_offset = lgd->lgd_local_lfb.lfb_fq_next;
		lbb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_local_lfb.lfb_fq_next);
		lbb_offset = lbb->lbb_next)
	    {
		lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);
		if (i == 0)
		    break;
		i--;

		format_lbb(lbb, buf);

		buf++;
	    }
	}
	*length -= i * sizeof (LG_BUFFER);
	(VOID)LG_unmutex(&lgd->lgd_local_lfb.lfb_fq_mutex);
	break;

    case LG_S_WBUFFER:
	buf = (LG_BUFFER *)item;
	i = l_item / sizeof(LG_BUFFER);
	*length = i * sizeof(LG_BUFFER);

	if (lgd->lgd_local_lfb.lfb_wq_count)
	{
	    if (status = LG_mutex(SEM_SHARE, &lgd->lgd_local_lfb.lfb_wq_mutex))
		return(status);
	    {
		for (lbb_offset = lgd->lgd_local_lfb.lfb_wq_next;
		    lbb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_local_lfb.lfb_wq_next);
		    lbb_offset = lbb->lbb_next)
		{
		    lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);
		    if (i == 0)
			break;
		    i--;

		    format_lbb(lbb, buf);

		    buf++;
		}
	    }
	    (VOID)LG_unmutex(&lgd->lgd_local_lfb.lfb_wq_mutex);
	}
	*length -= i * sizeof (LG_BUFFER);
	break;


    case LG_S_CBUFFER:
	buf = (LG_BUFFER *)item;
	*length = 0;

	lbb = (LBB *)
		LGK_PTR_FROM_OFFSET(lgd->lgd_local_lfb.lfb_current_lbb);

	if (lbb->lbb_state & LBB_CURRENT)
	{
	    *length = 1 * sizeof(LG_BUFFER);
	    format_lbb(lbb, buf);
	}
	break;

    case LG_A_RES_SPACE:
	if (l_item < sizeof(u_i8))
	    return (LG_BADPARAM);
	*(u_i8 *)item = lgd->lgd_local_lfb.lfb_reserved_space;
	break;

    case LG_S_CSP_PID:
	if (l_item < sizeof(i4))
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(i4));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_csp_pid;
	break;

    case LG_S_MUTEX:
	if (l_item < sizeof(LG_MUTEX))
	{
	    uleFormat(NULL, E_DMA41D_LGSHOW_STAT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(LG_MUTEX));
	    return (LG_BADPARAM);
	}
	*length = sizeof(LG_MUTEX);
	mstat = (LG_MUTEX *)item;
	mstat->count = 0;

	lstat = &mstat->stats[mstat->count];
	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lpb_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_ldb_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lxb_q_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lwlxb.lwq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_wait_stall.lwq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_wait_event.lwq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_open_event.lwq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_ckpdb_event.lwq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}


	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_ldbb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	/*
	** Report stats on NOTDB ldb separately
	*/
	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_spec_ldb.ldb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	for (i = 2; i <= lgd->lgd_ldbb_count; i++)
	{
	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[i]);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, &ldb->ldb_mutex,
				lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}


	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lfbb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	/*
	** Report stats on Local lfb separately
	*/
	lfb = &lgd->lgd_local_lfb;
	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lfb->lfb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lfb->lfb_fq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lfb->lfb_wq_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	for (lbb_offset = lfb->lfb_fq_next;
	     lbb_offset != LGK_OFFSET_FROM_PTR(&lfb->lfb_fq_next);
	     lbb_offset = lbb->lbb_next)
	{
	    lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb_offset);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, &lbb->lbb_mutex,
				lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	lfbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lfbb_table);
	for (i = 2; i <= lgd->lgd_lfbb_count; i++)
	{
	    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lfbb_table[i]);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, &lfb->lfb_mutex,
				lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}


	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lpbb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	for (i = 1; i <= lgd->lgd_lpbb_count; i++)
	{
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[i]);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, &lpb->lpb_mutex,
				lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}


	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lxbb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}
	status = FAIL;
	MEfill(sizeof(*lstat), 0, (char *)lstat);
	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	for (i = 1; i <= lgd->lgd_lxbb_count; i++)
	{
	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[i]);
	    if (status = CSs_semaphore(CS_ROLL_SEM_STATS, &lxb->lxb_mutex,
				lstat, sizeof(*lstat)))
		break;
	}
	if (status == OK)
	{
	    mstat->count++;
	    lstat++;
	}

	if ((CSs_semaphore(CS_INIT_SEM_STATS, &lgd->lgd_lpdb_mutex, 
				lstat, sizeof(*lstat))) == OK)
	{
	    mstat->count++;
	    lstat++;
	}


	break; 

    case LG_S_FORCED_LGA:
	if (l_item < sizeof(LG_LA))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LA), 0, "LG_S_FORCED_LGA");
	    return (LG_BADPARAM);
	}
	*(LG_LA *)item = lgd->lgd_local_lfb.lfb_forced_lga;
	break;

    case LG_S_FORCED_LSN:
	if (l_item < sizeof(LG_LSN))
	{
	    uleFormat(NULL, E_DMA41F_LGSHOW_LGLA_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_LSN), 0, "LG_S_FORCED_LSN");
	    return (LG_BADPARAM);
	}
	*(LG_LSN *)item = lgd->lgd_local_lfb.lfb_forced_lsn;
	break;

    case LG_S_XA_PREPARE:
    case LG_S_XA_COMMIT:
    case LG_S_XA_COMMIT_1PC:
    case LG_S_XA_ROLLBACK:

	/*
	**	Do something to a XA distributed transaction Branch.
	**
	**	For all but rollback, all associations to the Branch
	**	must have been ended.
	**
	**     inputs: item	- LG_TRANSACTION struct:
	**			    tr_dis_id	- Distributed transaction id 
	**					  Branch of interest
	**			    tr_pr_id	- internal lpb id of the caller
	**			    tr_lpd_id	- internal lpd id of the database
	**
	*/
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, (flag == LG_S_XA_PREPARE) ? "LG_S_XA_PREPARE" :
			   (flag == LG_S_XA_COMMIT)  ? "LG_S_XA_COMMIT"  :
			   "LG_S_XA_ROLLBACK");
	    return (LG_BADPARAM);
	}

	tr = (LG_TRANSACTION *)item;

	/* Length == -1 means "not found" */
	*length = -1;

	/*
	** Check the lpb id parameter.
	*/
	lg_id.id_i4id = tr->tr_pr_id;

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, (flag == LG_S_XA_PREPARE) 	 ? "LG_S_XA_PREPARE" :
			   (flag == LG_S_XA_COMMIT)  	 ? "LG_S_XA_COMMIT"  :
			   (flag == LG_S_XA_COMMIT_1PC)  ? "LG_S_XA_COMMIT_1PC"  :
			   "LG_S_XA_ROLLBACK");
	    return (LG_BADPARAM);
	}

	lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
	lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);

	if (lpb->lpb_type != LPB_TYPE ||
	    lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lpb->lpb_id.id_instance,
			0, lg_id.id_lgid.id_instance, 
			0, (flag == LG_S_XA_PREPARE) 	 ? "LG_S_XA_PREPARE" :
			   (flag == LG_S_XA_COMMIT)  	 ? "LG_S_XA_COMMIT"  :
			   (flag == LG_S_XA_COMMIT_1PC)  ? "LG_S_XA_COMMIT_1PC"  :
			   "LG_S_XA_ROLLBACK");
	    return (LG_BADPARAM);
	}
	/*
	** Check the lpd id parameter.
	*/
	lg_id.id_i4id = tr->tr_lpd_id;

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, (flag == LG_S_XA_PREPARE) 	 ? "LG_S_XA_PREPARE" :
			   (flag == LG_S_XA_COMMIT)  	 ? "LG_S_XA_COMMIT"  :
			   (flag == LG_S_XA_COMMIT_1PC)  ? "LG_S_XA_COMMIT_1PC"  :
			   "LG_S_XA_ROLLBACK");
	    return (LG_BADPARAM);
	}

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD*)LGK_PTR_FROM_OFFSET(lpdb_table[lg_id.id_lgid.id_id]);

	if (lpd->lpd_type != LPD_TYPE ||
	    lpd->lpd_id.id_instance != lg_id.id_lgid.id_instance ||
	    lpd->lpd_lpb != LGK_OFFSET_FROM_PTR(lpb) )
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lpd->lpd_id.id_instance,
			0, lg_id.id_lgid.id_instance, 
			0, (flag == LG_S_XA_PREPARE) 	 ? "LG_S_XA_PREPARE" :
			   (flag == LG_S_XA_COMMIT)  	 ? "LG_S_XA_COMMIT"  :
			   (flag == LG_S_XA_COMMIT_1PC)  ? "LG_S_XA_COMMIT_1PC"  :
			   "LG_S_XA_ROLLBACK");
	    return (LG_BADPARAM);
	}

	ldb = (LDB*)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);

	lxb = (LXB*)NULL;
	nlxb = (LXB*)NULL;
	slxb = (LXB*)NULL;

	/* 
	** Return a HANDLE LXB that matches the Branch XID
	** identified by tr->tr_dis_id, and database.
	*/

	status = OK;

	/* Search the SHARED LXB list, match on GTRID, database */

	(VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
	lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lgd->lgd_slxb.lxbq_next);

	while ( lxbq != (LXBQ*)&lgd->lgd_slxb.lxbq_next && !slxb )
	{
	    LPD		*slpd;
	    LXBQ	*handle_lxbq;

	    slxb = (LXB*)((char*)lxbq - CL_OFFSETOF(LXB,lxb_slxb));
	    slpd = (LPD*)LGK_PTR_FROM_OFFSET(slxb->lxb_lpd);
	    lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_next);

	    /* If the GT is marked for abort, return "branch not found" (NOTA) */
	    if ( slpd->lpd_ldb == lpd->lpd_ldb &&
		XA_XID_GTRID_EQL_MACRO(tr->tr_dis_id, slxb->lxb_dis_tran_id) &&
		!(slxb->lxb_status & LXB_ABORT) )
	    {
		/* Found the Global Transaction, now find the Branch */
		(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);
		(VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
		slpd = (LPD*)LGK_PTR_FROM_OFFSET(slxb->lxb_lpd);

		/* Recheck it all after waiting for mutex */
		if ( slxb->lxb_type == LXB_TYPE &&
		     slxb->lxb_status & LXB_SHARED &&
		     slpd->lpd_ldb == lpd->lpd_ldb &&
		    XA_XID_GTRID_EQL_MACRO(tr->tr_dis_id, slxb->lxb_dis_tran_id) &&
		     !(slxb->lxb_status & LXB_ABORT) )
		{
		    handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);
		    while ( handle_lxbq != (LXBQ*)&slxb->lxb_handles && !nlxb )
		    {
			nlxb = (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
			handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxbq->lxbq_next);

			/* Ignore Handle's marked for abort as if they don't exist (NOTA) */
			if ( DB_DIS_TRAN_ID_EQL_MACRO(tr->tr_dis_id, nlxb->lxb_dis_tran_id) )
			{
			    (VOID)LG_unmutex(&slxb->lxb_mutex);
			    (VOID)LG_mutex(SEM_EXCL, &nlxb->lxb_mutex);
			    if ( nlxb->lxb_type == LXB_TYPE &&
				 nlxb->lxb_status & LXB_SHARED_HANDLE &&
				 DB_DIS_TRAN_ID_EQL_MACRO(nlxb->lxb_dis_tran_id,
							    tr->tr_dis_id) &&
				 !(nlxb->lxb_status & LXB_ABORT) )
			    {
				/* Success! */
				break;
			    }
			    (VOID)LG_unmutex(&nlxb->lxb_mutex);
			    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);
			    handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);
			}
			nlxb = (LXB*)NULL;
		    }
		}
		else
		{
		    (VOID)LG_unmutex(&slxb->lxb_mutex);
		    (VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
		    lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(lgd->lgd_slxb.lxbq_next);
		    slxb = (LXB*)NULL;
		}
	    }
	    else
		slxb = (LXB*)NULL;
	}

	/* If Branch found... */
	if ( nlxb )
	{
	    /* (re)Mutex the SHARED LXB */
	    (VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

	    /* Length == 0 means found but doesn't meet criteria  */
	    *length = 0;

	    if ( flag == LG_S_XA_PREPARE )
	    {
		/*
		** XA_PREPARE: 	the Branch must be orphaned and
		**		not already prepared.
		**		If using 1PC, PREPAREs are
		**		not allowed.
		*/
		if ( !(slxb->lxb_status & LXB_ONEPHASE) &&
		     (nlxb->lxb_status & (LXB_ORPHAN | LXB_PREPARED))
				== LXB_ORPHAN )
		{
		    lxb = nlxb;
		}
	    }
	    else if ( flag == LG_S_XA_COMMIT )
	    {
		/*
		** XA_COMMIT: 	the Branch must be orphaned
		** 		and prepared.
		**		If using 1PC, reject, must
		**		use LG_S_XA_COMMIT_1PC
		**		for all (?).
		*/
		if ( !(slxb->lxb_status & LXB_ONEPHASE) &&
		     nlxb->lxb_status & LXB_PREPARED &&
		     nlxb->lxb_status & LXB_ORPHAN )
		{
		    lxb = nlxb;
		}
	    }
	    else if ( flag == LG_S_XA_COMMIT_1PC )
	    {
		/*
		** XA_COMMIT_1PC: There must have been no PREPAREs
		** 		  on the global transaction and
		**		  the Branch must be orphaned.
		*/
		if ( slxb->lxb_handle_preps == 0 &&
		     nlxb->lxb_status & LXB_ORPHAN )
		{
		    lxb = nlxb;
		    /* Note one-phase commit in global txn */
		    slxb->lxb_status |= LXB_ONEPHASE;
		}
	    }
	    else if ( flag == LG_S_XA_ROLLBACK )
	    {
		/*
		** XA_ROLLBACK:	If not orphaned, then this is
		**		a rollback before xa_end(TMSUCCESS)
		**		and we forcibly abort the entire
		**		global transaction.
		**		If orphaned, adopt the branch,
		**		let recovery proceed.
		*/
		if ( nlxb->lxb_status & LXB_ORPHAN )
		    lxb = nlxb;
		else
		{
		    /* Abort the global transaction */
		    LG_unmutex(&nlxb->lxb_mutex);

		    slxb->lxb_status |= LXB_MAN_ABORT;
		    LG_abort_transaction(slxb);

		    LG_unmutex(&slxb->lxb_mutex);
		    return(OK);
		}
	    }

	    /* Release the SHARED LXB */
	    LG_unmutex(&slxb->lxb_mutex);

	    if ( lxb )
	    {
		/*
		** Orphaned transaction branches are owned by the RCP.
		** Adopt the transaction by transferring 
		** ownership to the calling process'
		** database context (lpd).
		*/

		lxb->lxb_status &= ~LXB_REASSOC_WAIT;
		lxb->lxb_status |= LXB_RESUME;

		(VOID) LG_adopt_xact(lxb, lpd);

		format_tr_info(lxb, ldb, lpb, lpd, tr);
		*length = sizeof(LG_TRANSACTION);
	    }
	    (VOID)LG_unmutex(&nlxb->lxb_mutex);
	}
	else if ( slxb )
	    /* Found the GTRID, but not the Branch */
	    (VOID)LG_unmutex(&slxb->lxb_mutex);
	else
	    /* Didn't even find the GTRID */
	    (VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

	break;

    case LG_S_COMMIT_WILLING:
    case LG_S_ROLLBACK_WILLING: 
	/*
	** This could be xa_rollback/xa_commit after xa_recover
	** If the installation is started after abnormal shutdown,
	** the RCP owns WILLING_COMMIT transactions.
	**
	**     inputs: item	- LG_TRANSACTION struct:
	**			    tr_dis_id	- Distributed transaction id 
	**					  Branch of interest
	**			    tr_pr_id	- internal lpb id of the caller
	**			    tr_lpd_id	- internal lpd id of the database
	**
	*/
	if (l_item < sizeof(LG_TRANSACTION))
	{
	    uleFormat(NULL, E_DMA427_LGSHOW_XACT_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, l_item, 0, sizeof(LG_TRANSACTION),
			0, (flag == LG_S_COMMIT_WILLING) ? "LG_S_COMMIT_WILLING" :
			   "LG_S_ROLLBACK_WILLING");
	    return (LG_BADPARAM);
	}

	tr = (LG_TRANSACTION *)item;

	/* Length == -1 means "not found" */
	*length = -1;

	/*
	** Check the lpb id parameter.
	*/
	lg_id.id_i4id = tr->tr_pr_id;

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpbb_count)
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpbb_count,
			0, (flag == LG_S_COMMIT_WILLING) ? "LG_S_COMMIT_WILLING" :
			   "LG_S_ROLLBACK_WILLING");
	    return (LG_BADPARAM);
	}

	/*
	** Check the lpd id parameter.
	*/
	lg_id.id_i4id = tr->tr_lpd_id;

	if (lg_id.id_lgid.id_id == 0 || (i4)lg_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lg_id.id_lgid.id_id, 0, lgd->lgd_lpdb_count,
			0, (flag == LG_S_XA_PREPARE) 	 ? "LG_S_XA_PREPARE" :
			   (flag == LG_S_XA_COMMIT)  	 ? "LG_S_XA_COMMIT"  :
			   (flag == LG_S_XA_COMMIT_1PC)  ? "LG_S_XA_COMMIT_1PC"  :
			   "LG_S_XA_ROLLBACK");
	    return (LG_BADPARAM);
	}

        lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
        lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id.id_lgid.id_id]);
 
        if (lpb->lpb_type != LPB_TYPE ||
            lpb->lpb_id.id_instance != lg_id.id_lgid.id_instance)
        {
            ule_format(E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)0,
                        ULE_LOG, NULL, 0, 0, 0, &err_code, 3,
                        0, lpb->lpb_id.id_instance,
                        0, lg_id.id_lgid.id_instance, 
                        0, (flag == LG_S_COMMIT_WILLING) ? "LG_S_COMMIT_WILLING" :
                                                   "LG_S_ROLLBACK_WILLING");
            return (LG_BADPARAM);
        }

	lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	lpd = (LPD*)LGK_PTR_FROM_OFFSET(lpdb_table[lg_id.id_lgid.id_id]);

	if (lpd->lpd_type != LPD_TYPE ||
	    lpd->lpd_id.id_instance != lg_id.id_lgid.id_instance ||
	    lpd->lpd_lpb != LGK_OFFSET_FROM_PTR(lpb) )
	{
	    uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lpd->lpd_id.id_instance,
			0, lg_id.id_lgid.id_instance, 
			0, (flag == LG_S_COMMIT_WILLING)  ? "LG_S_COMMIT_WILLING" :
			   "LG_S_ROLLBACK_WILLING");
	    return (LG_BADPARAM);
	}

	slxb = (LXB*)NULL;
	nlxb = (LXB*)NULL;
	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);

	for (j = 1; j <= lgd->lgd_lxbb_count; j++)
	{
	    slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[j]);
	    if ( slxb->lxb_type == LXB_TYPE )
	    {
		LPD         *slpd;

		slpd = (LPD*)LGK_PTR_FROM_OFFSET(slxb->lxb_lpd);
		if ( slpd->lpd_ldb == lpd->lpd_ldb &&
		    XA_XID_GTRID_EQL_MACRO(tr->tr_dis_id, slxb->lxb_dis_tran_id) &&
			(slxb->lxb_status & LXB_WILLING_COMMIT) &&
			(slxb->lxb_status & LXB_ORPHAN))
		{
		    i4	*data = (i4 *)&slxb->lxb_id;

		    if (flag == LG_S_XA_ROLLBACK)
			status = xa_term_tran(1, *data);
		    else
			status = xa_term_tran(0, *data);

		    if (status == E_DB_OK)
			/* Length == 0 means transaction found */
			*length = sizeof(LG_TRANSACTION);
		    else
			/* Length == 0 means found but doesn't meet criteria */
			*length = 0;

		    return (OK);
		}
	    }
	}

	/* transaction not found, but no error */
	break;

    case LG_S_SHOW_RECOVER:
	if (l_item < sizeof(LG_RECOVER))
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(i4));
	    return (LG_BADPARAM);
	}
	recov_struct = (LG_RECOVER *)item;
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	STRUCT_ASSIGN_MACRO(lgd->lgd_recover, *recov_struct);
	(VOID)LG_unmutex(&lgd->lgd_mutex);
	break;

    case LG_S_XID_ARRAY_PTR:
	/* Return pointer to active xid array */
        if ( l_item < sizeof(PTR) )
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(PTR));
	    return (LG_BADPARAM);
	}

	*(char**)item = LGK_PTR_FROM_OFFSET(lgd->lgd_xid_array);
	break;

    case LG_S_XID_ARRAY_SIZE:
	/* Return size of active xid array */
        if ( l_item < sizeof(lgd->lgd_xid_array_size) )
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(lgd->lgd_xid_array_size));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_xid_array_size;
	break;

    case LG_S_CRIB_BT:
    case LG_S_CRIB_BS:
    case LG_S_CRIB_BS_LSN:

        /* Return point-in-time Consistent Read Info */

	/*
	** LG_S_CRIB_BT		Begin Transaction, init CRIB
	** LG_S_CRIB_BS		Begin Statement, refresh CRIB
	** LG_S_CRIB_BS_LSN	Begin Statement, only update crib_bos_lsn
	*/

        if ( l_item < sizeof(LG_CRIB) )
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(LG_CRIB));
	    return (LG_BADPARAM);
	}
	crib = (LG_CRIB*)item;

	lx_id.id_i4id = *(i4*)&crib->crib_log_id;

	if (lx_id.id_lgid.id_id == 0 || (i4) lx_id.id_lgid.id_id > lgd->lgd_lxbb_count)
	{
	    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lx_id.id_i4id, 0, lgd->lgd_lxbb_count,
		    0, "LG_S_CRIB");
	    return (LG_BADPARAM);
	}

	/* Also check that crib_xid_array PTR is not null */
	if ( crib->crib_xid_array == NULL )
	{
	    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lx_id.id_i4id, 0, lgd->lgd_lxbb_count,
		    0, "CRIB_XID_ARRAY");
	    return (LG_BADPARAM);
	}

	lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
	lxb  = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id.id_lgid.id_id]);

	/* Switch to shared LXB */
	if ( lxb->lxb_status & LXB_SHARED_HANDLE )
	{
	    if ( lxb->lxb_shared_lxb == 0 )
	    {
		uleFormat(NULL, E_DMA493_LG_NO_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, *(u_i4*)&lxb->lxb_id,
		    0, lxb->lxb_status,
		    0, (flag == LG_S_CRIB_BT) 
		    	? "LG_S_CRIB_BT" 
		        : (flag == LG_S_CRIB_BS) 
			? "LG_S_CRIB_BS" : "LG_S_CRIB_BS_LSN");
		return (LG_BADPARAM);
	    }

	    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);

	    if ( lxb->lxb_type != LXB_TYPE ||
		 (lxb->lxb_status & LXB_SHARED) == 0 )
	    {
		uleFormat(NULL, E_DMA494_LG_INVALID_SHR_TXN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
		    0, lxb->lxb_type,
		    0, LXB_TYPE,
		    0, *(u_i4*)&lxb->lxb_id,
		    0, lxb->lxb_status,
		    0, *(u_i4*)&lx_id,
		    0, (flag == LG_S_CRIB_BT) 
		    	? "LG_S_CRIB_BT" 
		        : (flag == LG_S_CRIB_BS) 
			? "LG_S_CRIB_BS" : "LG_S_CRIB_BS_LSN");
		return (LG_BADPARAM);
	    }
	}

	MyLXB = lxb;

	/* Get to the LDB belonging to caller's transaction */
	lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	lfb = (LFB *)LGK_PTR_FROM_OFFSET(ldb->ldb_lfb_offset);

	/*
	** If just updating crib_bos_lsn and this transaction
	** has done log writes that are advancing the cause,
	** then we don't need a mutex - just set bos_lsn
	** to last_lsn.
	*/
	if ( flag == LG_S_CRIB_BS_LSN &&
	     MyLXB->lxb_last_lsn.lsn_high != 0 &&
	     !LSN_EQ(&crib->crib_bos_lsn, &MyLXB->lxb_last_lsn) )
	{
	    crib->crib_bos_lsn = MyLXB->lxb_last_lsn;
	}
	else
	{
	    /* Prevent new/ending DB transactions while we make the CRIB */
	    if ( status = LG_mutex(SEM_EXCL, &ldb->ldb_mutex) )
		return(status);

	    /* If not just updating crib_bos_lsn, refresh the CRIB */
	    if ( flag != LG_S_CRIB_BS_LSN )
	    {
		/*
		** If transaction's tranid wanted (BT), set in CRIB,
		** otherwise if BS, get a new tranid (used when isolation level
		** is read-committed and a new statement is starting).
		**
		** This is the tranid that will be used as the basis for
		** row update conflict resolution.
		*/
		if ( flag == LG_S_CRIB_BT )
		{
		    crib->crib_bos_tranid = lxb->lxb_lxh.lxh_tran_id.db_low_tran; 
		    crib->crib_sequence = 1;
		}
		else /* LG_S_CRIB_BS */
		{
		    /* This should never happen */
		    if ( crib->crib_lgid_low == 0 ||
			 crib->crib_lgid_high == 0 )
		    {
			(VOID)LG_unmutex(&ldb->ldb_mutex);
			uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
				0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
				0, "LG_S_CRIB");
			return (LG_BADPARAM);
		    }

		    /* Preserve current CRIB contents */
		    low_lsn = crib->crib_low_lsn;
		    last_commit = crib->crib_last_commit;
		    bos_lsn = crib->crib_bos_lsn;
		    lgid_low = crib->crib_lgid_low;
		    lgid_high = crib->crib_lgid_high;
		}

		/* Return LSN of last commit in this DB */
		crib->crib_last_commit = ldb->ldb_last_commit;

		/*
		** Set low/high bounds of open txns in DB.
		**
		** As this transaction itself is still active
		** even thought it may not have done any log writes,
		** these should never be zero (which would indicate
		** there are no open transactions in this DB).
		*/
		crib->crib_lgid_low = ldb->ldb_lgid_low;
		crib->crib_lgid_high = ldb->ldb_lgid_high;

		/* This should never happen */
		if ( crib->crib_lgid_low == 0 ||
		     crib->crib_lgid_high == 0 )
		{
		    (VOID)LG_unmutex(&ldb->ldb_mutex);
		    uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lx_id.id_lgid.id_id, 0, lgd->lgd_lxbb_count,
			    0, "LG_S_CRIB");
		    return (LG_BADPARAM);
		}

		/* Assume low_lsn the same as last_commit */
		crib->crib_low_lsn = crib->crib_last_commit;

		/*
		** If there's an oldest active lxb (has actually done
		** log writes) and its first LSN is less than last_commit,
		** set low_lsn to that LSN.
		*/
		lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(ldb->ldb_active_lxbq.lxbq_next);
	        if ( lxbq != &ldb->ldb_active_lxbq ) 
		{
		    lxb = (LXB*)((char*)lxbq - CL_OFFSETOF(LXB,lxb_active_lxbq));

		    if ( LSN_LT(&lxb->lxb_first_lsn, &crib->crib_last_commit) )
			crib->crib_low_lsn = lxb->lxb_first_lsn;
		}

		/* The installation's xid_array of all transactions/db's */
		xidArray = (u_i4*)LGK_PTR_FROM_OFFSET(lgd->lgd_xid_array);

		if ( flag == LG_S_CRIB_BT )
		{
		    /* BT: Copy entire xid_array to CRIB */
		    MEcopy((PTR)xidArray, 
		    	   lgd->lgd_xid_array_size,
			   (PTR)crib->crib_xid_array);
		}
		/*
		** LG_S_CRIB_BS
		**
		** If nothing about the CRIB has changed since last BS,
		** leave the xid_array as it was, otherwise
		** refresh the xid_array from the lgd_xid_array.
		*/
		else if ( !LSN_EQ(&crib->crib_low_lsn, &low_lsn) ||
			  !LSN_EQ(&crib->crib_last_commit, &last_commit) )
		{
		    DB_TRAN_ID	tranid;

		    LG_assign_tran_id(lfb, &tranid, FALSE);
		    crib->crib_bos_tranid = tranid.db_low_tran;

		    /* Clear that portion of the CRIB xid_array last used */
		    if ( lgid_low == lgid_high )
			SET_CRIB_XID(crib, lgid_low, 0);
		    else if ( crib->crib_lgid_low != lgid_low ||
			      crib->crib_lgid_high != lgid_high )
		    {
			MEfill(sizeof(u_i4) * (lgid_high - lgid_low +1),
				0, (PTR)&crib->crib_xid_array[(u_i2)lgid_low]);
		    }

		    /* Copy that portion now in use */
		    MEcopy((PTR)&xidArray[(u_i2)crib->crib_lgid_low],
			   sizeof(u_i4) * (crib->crib_lgid_high -
					   crib->crib_lgid_low + 1),
		       (PTR)&crib->crib_xid_array[(u_i2)crib->crib_lgid_low]);
		}
	    }

	    /*
	    ** Set last LSN of this transaction in CRIB.
	    **
	    ** This identifies the LSN at the time this statement
	    ** began for statement undo consistency. 
	    **
	    ** If this transaction isn't active (hasn't done any
	    ** LGwrites) or the bos_lsn hasn't moved,
	    ** set bos_lsn to ldb_last_lsn.
	    */
	    if ( MyLXB->lxb_last_lsn.lsn_high == 0 ||
		 LSN_EQ(&crib->crib_bos_lsn, &MyLXB->lxb_last_lsn) )
	    {
		crib->crib_bos_lsn = ldb->ldb_last_lsn;
	    }
	    else
		crib->crib_bos_lsn = MyLXB->lxb_last_lsn;

	    (VOID)LG_unmutex(&ldb->ldb_mutex);
	}

	break;

    case LG_S_JFIB:
    case LG_S_JFIB_DB:

        /* Return known journal info for a MVCC-eligible database */
        if ( l_item < sizeof(LG_JFIB) )
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(LG_JFIB));
	    return (LG_BADPARAM);
	}

	jfib = (LG_JFIB*)item;
	*length = 0;

	db_id.id_i4id = *(i4*)&jfib->jfib_db_id;

	/* Locate by LPD id? Then it should be known to LG */
	if ( flag == LG_S_JFIB )
	{
	    if (db_id.id_lgid.id_id == 0 || (i4) db_id.id_lgid.id_id > lgd->lgd_lpdb_count)
	    {
		uleFormat(NULL, E_DMA428_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, db_id.id_i4id, 0, lgd->lgd_lpdb_count,
			0, "LG_S_JFIB");
		return (LG_BADPARAM);
	    }

	    lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);
	    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lpdb_table[db_id.id_lgid.id_id]);

	    if (lpd->lpd_type != LPD_TYPE ||
		lpd->lpd_id.id_instance != db_id.id_lgid.id_instance)
	    {
		uleFormat(NULL, E_DMA424_LGSHOW_BAD_ID, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			    0, lpd->lpd_id.id_instance,
			    0, db_id.id_lgid.id_instance, 
			    0, "LG_S_JFIB");
		return (LG_BADPARAM);
	    }

	    ldb = (LDB *)LGK_PTR_FROM_OFFSET(lpd->lpd_ldb);
	}
	else
	{
	    /* Locate by external db id, may not be known to LG, that's ok  */
	    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldb_q_mutex))
		return(status);

	    for (ldb_offset = lgd->lgd_ldb_next;
		ldb_offset != end_offset;)
	    {
		ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);
		if (ldb->ldb_type == LDB_TYPE &&
		    *(i4*)&ldb->ldb_database_id == *(i4*)&db_id)
		{
		    break;
		}
		ldb_offset = ldb->ldb_next;
		ldb = (LDB*)NULL;
	    }
	    (VOID)LG_unmutex(&lgd->lgd_ldb_q_mutex);
	}

	/* Return length = 0 if DB not found, or not journaled MVCC-enabled */
	if ( ldb && ldb->ldb_status & LDB_MVCC && ldb->ldb_status & LDB_JOURNAL )
	{
	    /* Return contents to caller */
	    *jfib = ldb->ldb_jfib;
	    /* Set ldb_id so LGalter(LG_A_JFIB_DB) will work */
	    jfib->jfib_db_id = *(LG_LGID*)&ldb->ldb_id;
	    *length = sizeof(LG_JFIB);
	}

	break;

    case LG_S_RBBLOCKS:
	/* Return number of readbackward_blocks */
        if ( l_item < sizeof(lgd->lgd_rbblocks) )
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(lgd->lgd_rbblocks));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_rbblocks;
	break;

    case LG_S_RFBLOCKS:
	/* Return number of readforward_blocks */
        if ( l_item < sizeof(lgd->lgd_rfblocks) )
	{
	    uleFormat(NULL, E_DMA420_LGSHOW_STS_SIZE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, l_item, 0, sizeof(lgd->lgd_rfblocks));
	    return (LG_BADPARAM);
	}
	*(i4 *)item = lgd->lgd_rfblocks;
	break;

    default:
	return (LG_BADPARAM);
    }

    return (OK);
}

/*{
** Name: map_dbstatus_to_world()	- map status's in one place.
**
** Description:
**	Yanks out some common code from LG_show.
**
** Inputs:
**      internal_status                 internal to LG database status
**
** Outputs:
**      external_status                 external to LG database status
**
** History:
**      03-jan-89 (mikem)
**	    just yanked some repeated code into a little subroutine.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added checkpoint error status (LDB_CKPERROR).
**	    Removed EBACKUP, ONLINE_PURGE, PRG_PEND database states.
**	24-may-1993 (bryanp)
**	    Map ALL database status bits for callers.
** 	13-jun-1997 (wonst02)
** 	    Added LDB_READONLY to handle readonly databases.
**	02-Jul-1997 (shero03)
**	    Added ADB_JSWITCH for journal switching in dmfcpp
**	14-Nov-1997 (jenjo02)
**	    Added LDB_STALL_RW for ckpdb.
**	15-Mar-1999 (jenjo02)
**	    Removed LDB_STALL_RW status bit.
**	26-Apr-2000 (thaju02)
**	    Added LDB_CKPLK_STALL. (B101303)
*/
static VOID
map_dbstatus_to_world(
i4            internal_status,
i4            *external_status)
{

    *external_status = 0;

    if (internal_status & LDB_ACTIVE)
	*external_status |= DB_ACTIVE;
    if (internal_status & LDB_JOURNAL)
	*external_status |= DB_JNL;
    if (internal_status & LDB_INVALID)
	*external_status |= DB_INVALID;
    if (internal_status & LDB_NOTDB)
	*external_status |= DB_NOTDB;
    if (internal_status & LDB_PURGE)
	*external_status |= DB_PURGE;
    if (internal_status & LDB_OPENDB_PEND)
	*external_status |= DB_OPENDB_PEND;
    if (internal_status & LDB_CLOSEDB_PEND)
	*external_status |= DB_CLOSEDB_PEND;
    if (internal_status & LDB_RECOVER)
	*external_status |= DB_RECOVER;
    if (internal_status & LDB_FAST_COMMIT)
	*external_status |= DB_FAST_COMMIT;
    if (internal_status & LDB_PRETEND_CONSISTENT)
	*external_status |= DB_PRETEND_CONSISTENT;
    if (internal_status & LDB_CLOSEDB_DONE)
	*external_status |= DB_CLOSEDB_DONE;
    if (internal_status & LDB_OPN_WAIT)
	*external_status |= DB_OPN_WAIT;
    if (internal_status & LDB_STALL)
	*external_status |= DB_STALL;
    if (internal_status & LDB_BACKUP)
	*external_status |= DB_BACKUP;
    if (internal_status & LDB_CKPDB_PEND)
	*external_status |= DB_CKPDB_PEND;
    if (internal_status & LDB_FBACKUP)
	*external_status |= DB_FBACKUP;
    if (internal_status & LDB_CKPERROR)
	*external_status |= DB_CKPDB_ERROR;
    if (internal_status & LDB_READONLY)
	*external_status |= DB_READONLY;
    if (internal_status & LDB_JSWITCH)
	*external_status |= DB_JSWITCH;
    if (internal_status & LDB_CKPLK_STALL)
	*external_status |= DB_CKPLK_STALL;
    if (internal_status & LDB_MVCC)
	*external_status |= DB_MVCC;

    return;
}

/*
** History:
**	15-Mar-2006 (jenjo02)
**	    Add buf_n_writers, buf_n_commit, buf_first_lsn,
**	    buf_last_lsn, buf_forced_lsn.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added lbb_id, buf_id.
*/
static VOID
format_lbb(LBB *lbb, LG_BUFFER *buf)
{
    LGD		    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    SIZE_TYPE	    lxb_offset;
    LXB		    *lxb;
    LFB		    *lfb;
    SIZE_TYPE	    lbb_offset;
    LG_I4ID_TO_ID   xid;

    lbb_offset = LGK_OFFSET_FROM_PTR(lbb);
    buf->buf_id = lbb->lbb_id;
    buf->buf_state = lbb->lbb_state;
    buf->buf_offset = lbb_offset;
    buf->buf_next_offset = lbb->lbb_next;
    buf->buf_prev_offset = lbb->lbb_prev;
    buf->buf_next_byte = lbb->lbb_next_byte;
    buf->buf_bytes_used = lbb->lbb_bytes_used;
    buf->buf_n_waiters = lbb->lbb_wait_count;
    buf->buf_n_writers = lbb->lbb_writers;
    buf->buf_n_commit = lbb->lbb_commit_count;
    buf->buf_lga = lbb->lbb_lga;
    buf->buf_forced_lga = lbb->lbb_forced_lga;
    buf->buf_resume_cnt = lbb->lbb_resume_cnt;
    buf->buf_first_lsn = lbb->lbb_first_lsn;
    buf->buf_last_lsn = lbb->lbb_last_lsn;
    buf->buf_forced_lsn = lbb->lbb_forced_lsn;

    buf->buf_assigned_owner =
	buf->buf_prim_owner = buf->buf_prim_task =
	buf->buf_dual_owner = buf->buf_dual_task = 0;

    if (lbb->lbb_owning_lxb)
    {
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lbb->lbb_owning_lxb);
	xid.id_lgid = lxb->lxb_id;
	buf->buf_assigned_owner = xid.id_i4id;	
    }

    for (lxb_offset = lgd->lgd_lxb_next;
	lxb_offset != LGK_OFFSET_FROM_PTR(&lgd->lgd_lxb_next);
	lxb_offset = lxb->lxb_next)
    {
	lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb_offset);
	if (lxb->lxb_type == LXB_TYPE)
	{
	    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lbb->lbb_lfb_offset);
	    if (lxb->lxb_assigned_buffer == lbb_offset)
	    {
		if ((lfb->lfb_status & LFB_DUAL_LOGGING) == 0 ||
		    (lxb->lxb_assigned_task == LXB_WRITE_PRIMARY))
		{
		    xid.id_lgid = lxb->lxb_id;
		    buf->buf_prim_owner = xid.id_i4id;
		    buf->buf_prim_task = lxb->lxb_assigned_task;
		}
		if ((lfb->lfb_status & LFB_DUAL_LOGGING) != 0 &&
		    (lxb->lxb_assigned_task == LXB_WRITE_DUAL))
		{
		    xid.id_lgid = lxb->lxb_id;
		    buf->buf_dual_owner = xid.id_i4id;
		    buf->buf_dual_task = lxb->lxb_assigned_task;
		}
	    }
	}
    }

    if (lbb_offset == lgd->lgd_local_lfb.lfb_current_lbb &&
	 lbb->lbb_state & LBB_FORCE)
    {
	/* For the current LBB, show group commit info */
	buf->buf_timer_lbb = lbb_offset;
	buf->buf_last_used = buf->buf_bytes_used;
	buf->buf_total_ticks = 0;
	buf->buf_tick_max = lgd->lgd_gcmt_numticks;
    }
    else
    {
	buf->buf_timer_lbb = 0;
	buf->buf_last_used = 0;
	buf->buf_total_ticks = 0;
	buf->buf_tick_max = 0;
    }


    return;
}

/*{
** Name: format_db_info()	- Fill in LG_DATABASE information block.
**
** Description:
**	Fills in LG_DATABASE block with information from the LDB.
**
** Inputs:
**	ldb	- Logging System Database Block
**
** Outputs:
**	db	- output database information
**
** History:
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: created to reduce code duplication.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Expose the ldb_sback_lsn through the db_sback_lsn.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Changed db journal and dump windows to track log record addresses
**	    rather than consistency point addresses.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Copy ldb_last_commit, ldb_first_la,
**	    ldb_jfib, ldb_last_lsn contents, ldb_lgid_low, ldb_lgid_high.
*/
static VOID
format_db_info(
LDB		*ldb,
LG_DATABASE	*db)
{
    LG_I4ID_TO_ID xid;

    xid.id_lgid = ldb->ldb_id;
    db->db_id = xid.id_i4id;
    db->db_database_id = ldb->ldb_database_id;

    db->db_l_buffer = ldb->ldb_l_buffer;
    MECOPY_VAR_MACRO((PTR)ldb->ldb_buffer, ldb->ldb_l_buffer, 
	      (PTR)db->db_buffer);

    map_dbstatus_to_world(ldb->ldb_status, &db->db_status);

    db->db_f_jnl_la = ldb->ldb_j_first_la;
    db->db_l_jnl_la = ldb->ldb_j_last_la;
    db->db_f_dmp_la = ldb->ldb_d_first_la;
    db->db_l_dmp_la = ldb->ldb_d_last_la;
    db->db_first_la = ldb->ldb_first_la;

    STRUCT_ASSIGN_MACRO(ldb->ldb_sbackup, db->db_sbackup);

    db->db_sback_lsn = ldb->ldb_sback_lsn;
    db->db_eback_lsn = ldb->ldb_eback_lsn;
    db->db_last_commit = ldb->ldb_last_commit;
    db->db_last_lsn = ldb->ldb_last_lsn;

    db->db_lgid_low = ldb->ldb_lgid_low;
    db->db_lgid_high = ldb->ldb_lgid_high;

    /* copy JFIB contents */
    db->db_jfib = ldb->ldb_jfib;

    db->db_stat.trans = ldb->ldb_lxb_count;
    db->db_stat.read = ldb->ldb_stat.read;
    db->db_stat.write = ldb->ldb_stat.write;
    db->db_stat.begin = ldb->ldb_stat.begin;
    db->db_stat.end = ldb->ldb_stat.end;
    db->db_stat.force = ldb->ldb_stat.force;
    db->db_stat.wait = ldb->ldb_stat.wait;
}

/*{
** Name: format_tr_info()	- Fill in LG_TRANSACTION information block.
**
** Description:
**	Fills in LG_TRANSACTION block with information from the LXB.
**
** Inputs:
**	lxb	- Logging System Transaction Block
**	ldb	- Logging System Database Block
**	lpb	- Logging System Process Block
**	lpd	- Logging System Process Database Block
**
** Outputs:
**	tx	- output database information
**
** History:
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: created to reduce code duplication.
**	18-oct-1993 (rogerk)
**	    Added space reservation info to transaction info returned.
**	24-feb-94 (swm)
**	    Bug #60425
**	    Corrected truncating cast of session id. from i4 to CS_SID
**	    in format_tr_info().
**	15-Dec-1999 (jenjo02)
**	    Added tr_handle_count, tr_handle_preps,
**	    tr_shared_id for SHARED transactions.
**	15-Mar-2006 (jenjo02)
**	    Add tr_first_lsn, tr_last_lsn, tr_wait_lsn,
**	    tr_wait_buf.
**	21-Jun-2006 (jonj)
**	    Add tr_lock_id, tr_txn_state for XA integration.
**	11-Sep-2006 (jonj)
**	    Add tr_handle_wts, tr_handle_ets for LOGFULL_COMMIT.
*/
static VOID
format_tr_info(
LXB		*lxb,
LDB		*ldb,
LPB		*lpb,
LPD		*lpd,
LG_TRANSACTION	*tr)
{
    LG_I4ID_TO_ID xid;

    tr->tr_status = lxb->lxb_status;
    tr->tr_wait_reason = lxb->lxb_wait_reason;
    xid.id_lgid = lxb->lxb_id;
    tr->tr_id = xid.id_i4id;
    xid.id_lgid = ldb->ldb_id;
    tr->tr_db_id = xid.id_i4id;
    xid.id_lgid = lpb->lpb_id;
    tr->tr_pr_id = xid.id_i4id;
    tr->tr_sess_id = *(CS_SID *)&lxb->lxb_cpid.sid;
    xid.id_lgid = lpd->lpd_id;
    tr->tr_lpd_id = xid.id_i4id;

    tr->tr_eid = lxb->lxb_lxh.lxh_tran_id;
    tr->tr_dis_id = lxb->lxb_dis_tran_id;
    tr->tr_lock_id = lxb->lxb_lock_id;
    tr->tr_txn_state = lxb->lxb_txn_state;
    tr->tr_database_id = ldb->ldb_database_id;

    tr->tr_l_user_name = lxb->lxb_l_user_name;
    MECOPY_VAR_MACRO((PTR)lxb->lxb_user_name, lxb->lxb_l_user_name,
	(PTR)tr->tr_user_name);
    tr->tr_reserved_space = lxb->lxb_reserved_space;

    tr->tr_first = lxb->lxb_first_lga;
    tr->tr_last = lxb->lxb_last_lga;
    tr->tr_cp = lxb->lxb_cp_lga;
    tr->tr_first_lsn = lxb->lxb_first_lsn;
    tr->tr_last_lsn = lxb->lxb_last_lsn;
    tr->tr_wait_lsn = lxb->lxb_wait_lsn;
    if ( lxb->lxb_status & LXB_WAIT_LBB )
	tr->tr_wait_buf = lxb->lxb_wait_lwq;
    else
	tr->tr_wait_buf = 0;


    tr->tr_shared_id = 0;

    if ( lxb->lxb_status & LXB_SHARED )
    {
	tr->tr_handle_count = lxb->lxb_handle_count;
	tr->tr_handle_preps = lxb->lxb_handle_preps;
	tr->tr_handle_wts   = lxb->lxb_handle_wts;
	tr->tr_handle_ets   = lxb->lxb_handle_ets;
    }
    else
    {
	tr->tr_handle_count = 0;
	tr->tr_handle_preps = 0;
	tr->tr_handle_wts   = 0;
	tr->tr_handle_ets   = 0;

	if ( lxb->lxb_status & LXB_SHARED_HANDLE &&
	     lxb->lxb_shared_lxb )
	{
	    LXB	*slxb = (LXB *)LGK_PTR_FROM_OFFSET(lxb->lxb_shared_lxb);
	    xid.id_lgid = slxb->lxb_id;
	    tr->tr_shared_id = xid.id_i4id;
	}
    }


    tr->tr_stat.write = lxb->lxb_stat.write;
    tr->tr_stat.split = lxb->lxb_stat.split;
    tr->tr_stat.force = lxb->lxb_stat.force;
    tr->tr_stat.wait = lxb->lxb_stat.wait;
}

/*
** Name: LG_debug_wacky_ldb_found	- debugging routine to dump LDB queues
**
** Description:
**	For a while, we were having problems with corruption of the LFB/LDB
**	large block queues, and this debugging code helped to track those
**	problems down. I believe that these bugs have now been fixed, but it's
**	useful to leave this code in at least for a while in case they
**	re-occur (the bugs involved attempts to deallocate the lgd_local_lfb,
**	which has an un-initialized lfb_id).
**
** History:
**	26-apr-1993 (bryanp)
**	    Checked this code in in case it should ever become useful.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Changed db journal and dump windows to track log record addresses
**	    rather than consistency point addresses.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
VOID
LG_debug_wacky_ldb_found(LGD *lgd, LDB *wacky_ldb)
{
    SIZE_TYPE	end_offset;
    SIZE_TYPE	ldb_offset;
    LDB		*ldb;
    i4	i;
    SIZE_TYPE	*ldbb_table;

    TRdisplay("The LDB at %p is wacky! It looks like this:\n", wacky_ldb);
    if (wacky_ldb)
    {
	TRdisplay("   ID=%x DB=%x (%~t,%~t,%t)\n",
		wacky_ldb->ldb_id, wacky_ldb->ldb_database_id,
		DB_MAXNAME, &wacky_ldb->ldb_buffer[0],
		DB_MAXNAME, &wacky_ldb->ldb_buffer[DB_MAXNAME],
		50, &wacky_ldb->ldb_buffer[DB_MAXNAME + DB_MAXNAME + 8]);
	TRdisplay("   Status: (%x) %v\n",
		wacky_ldb->ldb_status,
"ACTIVE,JOURNAL,INVALID,NOTDB,PURGE,OPEN_DB_PEND,CLOSE_DB_PEND,RECOVER,\
FAST_COMMIT,PRETEND_CONSISTENT,CLOSED_DONE,OPN_WAIT,STALL,BACKUP,\
CKPDB_PEND,READONLY,FBACKUP,CKPERROR,JSWITCH", wacky_ldb->ldb_status);

	TRdisplay("    FirstLA(%d,%d,%d) lastLA(%d,%d,%d)\n",
		wacky_ldb->ldb_j_first_la.la_sequence,
		wacky_ldb->ldb_j_first_la.la_block,
		wacky_ldb->ldb_j_first_la.la_offset,
		wacky_ldb->ldb_j_last_la.la_sequence,
		wacky_ldb->ldb_j_last_la.la_block,
		wacky_ldb->ldb_j_last_la.la_offset);
	TRdisplay("    DFirstLA(%d,%d,%d) DlastLA(%d,%d,%d)\n",
		wacky_ldb->ldb_d_first_la.la_sequence,
		wacky_ldb->ldb_d_first_la.la_block,
		wacky_ldb->ldb_d_first_la.la_offset,
		wacky_ldb->ldb_d_last_la.la_sequence,
		wacky_ldb->ldb_d_last_la.la_block,
		wacky_ldb->ldb_d_last_la.la_offset);

	TRdisplay(
"    Count: %d, read:%d write:%d begin:%d end:%d force:%d wait:%d\n",
	    wacky_ldb->ldb_lxb_count, wacky_ldb->ldb_stat.read,
	    wacky_ldb->ldb_stat.write,
	    wacky_ldb->ldb_stat.begin, wacky_ldb->ldb_stat.end,
	    wacky_ldb->ldb_stat.force,
	    wacky_ldb->ldb_stat.wait);
    }

TRdisplay("Now we'll walk the ldb list:\n");

    end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_ldb_next);

    for (ldb_offset = lgd->lgd_ldb_prev;
	ldb_offset != end_offset;
	ldb_offset = ldb->ldb_prev)
    {
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldb_offset);

	TRdisplay("   ID=%x DB=%x (%~t,%~t,%t)\n",
		ldb->ldb_id, ldb->ldb_database_id,
		DB_MAXNAME, &ldb->ldb_buffer[0],
		DB_MAXNAME, &ldb->ldb_buffer[DB_MAXNAME],
		50, &ldb->ldb_buffer[DB_MAXNAME + DB_MAXNAME + 8]);
	TRdisplay("   Status: (%x) %v\n",
		ldb->ldb_status,
"ACTIVE,JOURNAL,INVALID,NOTDB,PURGE,OPEN_DB_PEND,CLOSE_DB_PEND,RECOVER,\
FAST_COMMIT,PRETEND_CONSISTENT,CLOSED_DONE,OPN_WAIT,STALL,BACKUP,\
CKPDB_PEND,READONLY,FBACKUP,CKPERROR,JSWITCH", ldb->ldb_status);

    }

TRdisplay("And, finally, we'll walk the ldbb table:\n");

    for (i = 0; i <= lgd->lgd_ldbb_count; i++)	
    {
	ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);
	ldb = (LDB *)LGK_PTR_FROM_OFFSET(ldbb_table[i]);
	if (ldb->ldb_type == LDB_TYPE)
	{
	    TRdisplay(" Block %d is an LDB: \n", i);
	    TRdisplay("   ID=%x DB=%x (%~t,%~t,%t)\n",
		ldb->ldb_id, ldb->ldb_database_id,
		DB_MAXNAME, &ldb->ldb_buffer[0],
		DB_MAXNAME, &ldb->ldb_buffer[DB_MAXNAME],
		50, &ldb->ldb_buffer[DB_MAXNAME + DB_MAXNAME + 8]);
	    TRdisplay("   Status: (%x) %v\n",
		ldb->ldb_status,
"ACTIVE,JOURNAL,INVALID,NOTDB,PURGE,OPEN_DB_PEND,CLOSE_DB_PEND,RECOVER,\
FAST_COMMIT,PRETEND_CONSISTENT,CLOSED_DONE,OPN_WAIT,STALL,BACKUP,\
CKPDB_PEND,READONLY,FBACKUP,CKPERROR,JSWITCH", ldb->ldb_status);
	}
	else
	    TRdisplay(" Block %d is NOT an LDB: \n", i);
    }
}

/*{
** Name: LGrcp_pid()	- return RCP's pid.
**
** Description:
**	Returns RCP's pid.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	pid	- pid of RCP, zero if not yet known.
**
** History:
**	03-Dec-1997 (jenjo02)
**	    created.
*/
PID
LGrcp_pid()
{
    LGD		*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LPB		*lpb;

    /* Has logging initialized yet and is master known? */
    if (lgd == (LGD*)NULL || lgd->lgd_master_lpb == 0)
	return((PID)0);

    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb); 

    return(lpb->lpb_pid);
}


/*{
**  Name: xa_term_tran - Terminate a WILLING_COMMIT transaction.
**
**  Description:
**	    Terminate a WILLING_COMMIT transaction in the logging system.
**
** Inputs:
**	    abort			TRUE if to abort the transaction.
**	    data			The transaction ID to commit or abort.
** Outputs:
**	
**
**  History:
**      11-May-2007 (stial01)
**          Created from term_tran in lartool.
*/
static DB_STATUS
xa_term_tran(
i4	    abort,
i4	    data)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_stat;
    i4			err_code;
    i4			flag;

    if (abort)
	flag = LG_A_ABORT;
    else
	flag = LG_A_COMMIT;

    cl_stat = LGalter(flag, (PTR)&data, sizeof(data), &sys_err);
    if (cl_stat == OK)
	return (E_DB_OK);

    /* Error handling */
    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, flag);
    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &err_code, 1, 0, data);

    return(E_DB_ERROR);
}
