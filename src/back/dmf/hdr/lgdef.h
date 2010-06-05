/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: LGDEF.H - Definitions of internal LG types and constants.
**
** Description:
**      This file defines the types and constants that the routines
**	in LG operate with.  These types describes processes,
**	databases, transactions and log buffers.
**
** History: 
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	16-oct-1992 (jnash) merged 10-oct-1992 (rogerk)
**	    6.5 Recovery Project: Added LPB_CPAGENT process status to LPB
**	    structure and added removed obsolete lgd_n_servers field from
**	    the LGD structure.  Chage name of
**	    LG_check_fc_servers to LG_check_cp_servers,
**	    LG_last_fc_server to LG_last_cp_server.
**	7-oct-1992 (bryanp)
**	    Add prototypes for the LGMO code.
**	2-nov-1992 (bryanp)
**	    Many new error messages as well.
**	18-jan-1993 (bryanp)
**	    Add globalref for LG_logs_opened.
**	04-jan-1993 (jnash)
**	    Reduced logging project, continued.  Add lgd_reserved_space, 
**	    lxb_reserved_space, LG_used_logspace() FUNC_EXTERN, 
**	    E_DMA480_LGWRITE_FILE_FULL, E_DMA481_WRITE_PAST_BOF errors.
**	18-jan-1993 (rogerk)
**	    Added LGA comparison macros.  Copied from LSN macros in dm0l.h.
**	    Changed LG_set_bof to LG_archive_complete.  Added ldb_j_last_la and
**	    ldb_d_last_la fields to hold the log address of the last written
**	    log records for the database that require archive processing.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed LG_E_1PURGE_ACTIVE, LG_E_2PURGE_COMPLETE event states.
**	    Removed EBACKUP, PRG_PEND, ONLINE_PURGE database states.
**	    Added LXB_NORESERVE status.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp)
**	    6.5 cluster Recovery Project:
**	    - Created LFB (LogFileBlock) to track info about a log file.
**	    - Renamed lga_high with la_sequence, and lga_low with la_offset
**	      in LG_LA macros.
**	    - Moved LG_LA macros to lg.h from lgdef.h
**	    - Re-instated lgd_cnodeid to hold this node's cluster node ID.
**	    - Add ldb_sback_lsn to the LDB to hold the LSN of the backup start.
**	    - Add lgd_csp_id to track the CSP Process ID.
**	    - New error messages.
**	24-may-1993 (bryanp)
**	    Add experimental support for 2048 log file page size.
**	21-jun-1993 (bryanp)
**	    Add func_externs for CSP/RCP process linkage routines.
**	    Add func_externs for LGLSN.C routines.
**	    Add messsage ids for LGerase messages.
**	26-jul-1993 (bryanp)
**	    Cleaned up some uses of "short" by making them "i2" (or "nat"
**		where possible. The i2's and u_i2's in the page header are
**		the primary reason for 64K being the max log page size.
**	    Added prototype for LG_dealloc_lfb_buffers().
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added a new field - lfb_archive_window_prevcp - 
**	    which gives the consistency point address before which we know 
**	    there is no archiver processing required.  Consolidated the system
**	    journal and dump windows into a single lfb_archive_window.
**	23-aug-1993 (bryanp)
**	    Add LXB_LOGSPACE_WAIT for tracking "hard" logfull wait states.
**	    Add E_DMA48C_LG_BAD_CHECKSUM.
**	18-oct-1993 (rmuth)
**	    Add prototype for LG_queue_write, LG_pgyback_write, LG_do_write.
**	18-oct-1993 (rogerk)
**	    Fixes to 2PC recovery.
**	    Added LXB_ORPHAN transaction status for xacts which have been
**	    disassociated from their dbms sessions.
**	    Added lpb_cache_factor for logspace reservation work.
**	    Added LG_orphan_xact and LG_adopt_xact routines.
**	    Added LG_calc_reserved_space routine.
**	    Added argument to LG_used_logspace.
**	    Added new logfull error messages.
**	    Added logfull limit maximum value : LG_LOGFULL_MAX.
**	    Made lfb_reserved_space, lxb_reserved_space unsigned.
**	    Removed unused lgd_dmu_cnt, lgd_stall_limit, lxb_dmu_cnt fields.
**	31-jan-1994 (mikem)
**	    bug #56478
**	    0 logwriter threads caused installation to hang, fixed by adding
**	    special case code to handle synchonous "in-line" write's vs. 
**	    asynchronous writes.  The problem was that code that scheduled
**	    "synchronous" writes with direct calls to "write_block" did not
**	    handle correctly the mutex being released during the call.  In the 
**	    synchronous case one must hold the LG mutex across the DIwrite()
**	    call.
**	31-jan-1994 (jnash)
**	    Add E_DMA492_LGOPEN_BLKSIZE_MISMATCH.
**	01-feb-1994 (mikem)
**	    Fix integration problem, incorrect prototype for LG_do_write() was
**	    removed.  Fixed LG_check_logfull() prototype to match code change
**	    made this integration.
**	15-apr-1994 (chiku)
**	    Bug56704: Changed definition of LG_allocate_lbb().
**	23-may-1994 (bryanp) B58498
**	    Add prototypes for LGintrcvd, LGintrack.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Added a host of new mutexes
**	    to logging structures to reduce single-threading of
**	    LG via lgd_mutex.
**	    Altered lkmutex.c functions to accept a mutex pointer
**	    as input, dropped sys_err parm, which wasn't being
**	    used anyway.
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs). For this reason, the restriction that
**	    these structures "headers" must align is lifted and
**	    padding added for that reason has been eliminated.
**	    Added lgd_status_is_locked boolean to LG_signal_event()
**	    prototype. 
**	    Added lfb_mutex_is_locked boolean to LG_compute_bof()
**	    prototype.
**	    Turned off LG_DUAL_LOGGING_TESTBED as the default.
**	26-Apr-1996 (jenjo02)
**	    Added lxb_rlbb_count, reserved_lbb_count
**	    to LG_allocate_lbb() prototype to support spanned log records.
**	29-Apr-1996 (jenjo02)
**	    Added release_curr_lbb_mutex parm to LG_queue_write()
**	    prototype.
**	29-May-1996 (jenjo02)
**	    Added lxb parameter to prototypes for LG_allocate_lbb(),
**	    LG_check_logfull() and LG_abort_oldest().
**	10-Jul-1996 (jenjo02)
**	    Added buf_util to LGD to track utilization of
**	    log records.
**	16-Jul-1996 (jenjo02)
**	    Gave lgd_status its own mutex, lgd_status_mutex.
**	05-Sep-1996 (jenjo02)
**	    Removed "release_mutex" parm from LG_do_write() prototype.
**	01-Oct-1996 (jenjo02)
**	    Added lgd_stat.max_wq and lgd_stat.max_wq_count.
**	06-Dec-1996 (jenjo02)
**	    Removed lbb_last_used, lbb_total_ticks, moved them to 
**	    lgd_timer_lbb_last_used, lgd_timer_lbb_total_ticks.
**	09-Jan-1997 (jenjo02)
**	    Added lxb_event_mask, lxb_events, to LXB.
** 	10-jun-1997 (wonst02)
** 	    Added LDB_READONLY flag for readonly databases.
**	03-Jul-1997 (shero03)
**	    Added LDB_JSWITCH for journal switch in checkpoint
**	13-Nov-1997 (jenjo02)
**	    Added LDB_STALL_RW flag to tell LGbegin() to stall all
**	    new read_write txns while online checkpoint is stalled.
**      08-Dec-1997 (hanch04)
**          Removed lfb_last_lsn, moved it to the log header,
**          for support of logs > 2 gig
**	17-Dec-1997 (jenjo02)
**	  o Renamed lgd_ilwt (queue of idle logwriters) to lgd_lwlxb
**	    (queue of logwriters).
**	  o Added LG_my_pid GLOBALREF.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Add structure LG_IO, change DI_IO references to LG_IO,
**	    Add LG_READ/LG_WRITE macros to replace direct calls to
**	    DIread()/DIwrite().
**	    Replaced lpb_gcmt_sid with lpb_gcmt_lxb.
**	24-Aug-1998 (jenjo02)
**	    Added LXH structure to provide a quick mechanism for
**	    determining if a give DB_TRAN_ID is active in the logging
**	    system. As log transactions become active, they are added
**	    to this hash queue, hashed on transaction id, and
**	    removed when the logging transaction ends.
**	16-Nov-1998 (jenjo02)
**	    Changed cross-process identity to CS_CPID structure instead
**	    of PID and SID.
**	22-Dec-1998 (jenjo02)
**	    Moved define of RAWIO_ALIGN to dicl.h (DI_RAWIO_ALIGN).
**	19-Feb-1999 (jenjo02)
**	    Added prototypes for LG_suspend_lbb(), LG_resume_lbb().
**	    Added LXB parm to LG_queue_write() prototype. 
**	26-May-1999 (thaju02)
**	    Added (PID *) to LG_lpb_shrbuf_nuke prototype. (B96725)
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. 
**	24-Aug-1999 (jenjo02)
**	    Removed lfb_cp_mutex, lfb_current_lbb_mutex.
**	    Added LBB_CURRENT, lbb_instance, lbb_wait_count, renamed 
**	    lbb_lbh_mutex to lbb_mutex.
**	    Replaced lgd_lxb_aq_mutex, lgd_lxb_iq_mutex, with lgd_lxb_q_mutex,
**	    eliminated lgd_lxb_inext, lgd_lxb_prev. 
**	    Removed obsolete lgd_n_logwriters.
**	    Collapsed lgd_wait_free, lgd_wait_split into a single buffer
**	    wait queue, lgd_wait_buffer.
**	    Removed lgd_timer_lbb, lgd_timer_lbb_last_used, 
**	    lgd_timer_lbb_total_ticks.
**	    Removed lgd_status_mutex, use lgd_mutex in its place.
**	    Added lxb_last_lsn, lxb_wait_lsn.
**	    Realigned all those nat/longnat to i4 lines.
**      30-sep-1999 (jenjo02)
**	    Added lxb_first_lsn for LG_S_OLD_LSN function
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED log transactions, list of
** 	    LXBs in abort state instead of just one.
**      07-Dec-1999 (hanal04) Bug 99540 INGSRV 1053
**          Added LFB_WQ_MUTEX_HELD as a flag to LG_queue_write(). This
**          flag indicates that the lfb_wq_mutex mutex is already held
**          by the caller.
**      08-Dec-1999 (hanal04) Bug 97015 INGSRV 819
**	    (hanje04) X-Integ of 444135 from oping20. Changed to E_DMA495
**	    because E_DMA493 already exists in main.
**          Added E_DMA493 to report a failure to detect the end of
**          the lwq. This new error prevents a tight spin which would
**          hang the installation.
**	02-Feb-2000 (jenjo02)
**	    Inline CSp|v_semaphore calls. LG_mutex, LG_unmutex are
**	    now macros calling LG_mutex_fcn(), LG_unmutex_fcn() iff
**	    a semaphore error occurs.
**	17-Mar-2000 (jenjo02)
**	    Change LG_queue_write prototype to return STATUS instead
**	    of VOID.
**	26-Apr-2000 (thaju02)
**	    Added LDB_CKPLK_STALL. (B101303)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2000 (jenjo02)
**	    Added "bytes" stat to LFB, needed to correct "kbytes"
**	    written stat.
**	04-Oct-2000 (jenjo02)
**	    Added LGcp_resume macro to call CSresume directly
**	    if local waiter.
**	17-sep-2001 (somsa01)
**	    Backing out changes for bug 97015, as they are not needed in
**	    2.6+.
**	08-apr-2002 (devjo01)
**	    Add lfb_first_lbb to record beginning of allocation extent
**	    for contiguously allocatted LBB's and their associated log
**	    buffers.
**      23-Oct-2002 (hanch04)
**	    Added definition for LG_size
**	22-Oct-2002 (jenjo02)
**	    Added "*status" to LG_allocate_lxb() prototype,
**	    E_DMA495_LG_BAD_SHR_TXN_STATE.
**	17-Dec-2003 (jenjo02)
**	    Added functionality for LGconnect() for Partitioned
**	    Table and Parallel Query project.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      05-may-2005 (stial01)
**          Added LDB_CSP_RECOVER
**	28-may-2005 (devjo01)
**	    Add prototype for LG_rcvry_cmpl.
**      13-jun-2005 (stial01)
**          Removed LDB_CSP_RECOVER, added lfb_nodename
**	25-Oct-2005 (hanje04)
**	    Move GLOBALREF of Lg_di_cbs after defn of LG_IO to prevent compiler
**	    errors with GCC 4.0.
**	01-Aug-2006 (jonj)
**	    Pass *lpd instead of ldb_offset to LG_allocate_lxb().
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory. Add DMA499.
**	10-Apr-2007 (jonj)
**	    Add prototype for LG_lpb_node_nuke().
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      28-feb-2008 (stial01)
**          Added prototype for assign_logwriter()
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR
**	10-Nov-2009 (kschendel) SIR 122757
**	    Pass (aligned) buffer to LG_write_log_headers.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

#undef LG_DUAL_LOGGING_TESTBED 

/*
** These three constants are used by LGopen and its 'children' to communicate
** about a log which is going to be disabled.
*/
#define			DISABLE_NO_LOG		0
#define			DISABLE_PRIMARY_LOG	1
#define			DISABLE_DUAL_LOG	2

/*
** These constants are used by the LG_MASTER log file analysis routines which
** are invoked by LGopen:
*/
#define			LOG_HEADER_IO_SIZE	VMS_BLOCKSIZE

#define			VMS_BLOCKSIZE		512


/* We need extra space reserved while logfull:
** There are two parts involved: static and dynamic:
** Static part consists of 2 DM0L_ECP, 1 DM0L_ECP, and
** the dynamic part consists of (num of curr tran)*abort_record (DM0L_ET).
**
** Plus the LG_RECORD size attached to every log record.
** We could define those dm0l type here, or we figure out the approx. size
** of each log type (used here). Different compiler may give slight different 
** size (depends on how the optimization is done), but we can ignore it.
**
**  DM0L_ECP: 32 bytes
**  DM0L_BCP: 228 bytes
**  DM0L_ET:  44 bytes
**  LG_RECORD: 36 bytes
**  Hence the static part (LGSTLSSZ) total: 2*44 + 1*228 + 3*36 = 424
**  And the dynamic overhead (LGSTLDSZ): sizeof(LG_RECORD)+sizeof(DM0L_ET)-4
*/

#define			LGSTLSSZ		(318+3*(sizeof(LG_RECORD)))
#define			LGSTLDSZ		(sizeof(LG_RECORD)+40)


/*
** Defines for log page size, used by both lgextern & lgintern.
*/

#define		    LGD_MIN_BLK_SIZE	    2048    /* min log file blk size */
#define		    LGD_MAX_BLK_SIZE	    32768   /* max log file blk size */
#define		    LGD_MIN_DISK_BLK_SIZE   2048    /* min unix raw disk blk 
						    ** size.  This is disk con-
						    ** troller specific, but
						    ** 2K should be unix 
						    ** portable.
						    */

GLOBALREF   bool    LG_is_mt;
GLOBALREF   PID     LG_my_pid;
GLOBALREF   u_i4   LG_logs_opened;
#define			LG_PRIMARY_LOG_OPENED	0x01
#define			LG_DUAL_LOG_OPENED	0x02
#define			LG_BOTH_LOGS_OPENED	\
			    (LG_PRIMARY_LOG_OPENED|LG_DUAL_LOG_OPENED)
/*
** definitions used to index into the two-member array of LG_IO control blocks:
*/
#define		LG_PRIMARY_DI_CB	0
#define		LG_DUAL_DI_CB		1

/*
** Logfull limit maximum.  The system logfull limit cannot be set beyond this.
*/
#define		LG_LOGFULL_MAX		96

/*
** Mode definitions for LG_mutex()
*/
#define		SEM_SHARE 		0
#define		SEM_EXCL		1

/* Macros to request/release LG mutexes */
#ifdef LGLK_NO_INTERRUPTS
#define		LG_mutex(mode, sem) \
	LG_mutex_fcn(mode, sem, __FILE__, __LINE__)
#define		LG_unmutex(sem) \
	LG_unmutex_fcn(sem, __FILE__, __LINE__)
#else
#define		LG_mutex(mode, sem) \
	( ( CSp_semaphore(mode, sem) ) \
	 ? LG_mutex_fcn(mode, sem, __FILE__, __LINE__) : OK )

#define		LG_unmutex(sem) \
	( ( CSv_semaphore(sem) ) \
	 ? LG_unmutex_fcn(sem, __FILE__, __LINE__) : OK )
#endif /* LGLK_NO_INTERRUPTS */

/*
** Debugging macros, etc
*/
#undef LG_MUTEX_TRACE		/* displays p/v semaphore calls */
#undef LG_TRACE			/* lots of TRdisplays, identifies functions */

#ifdef LG_TRACE
#define LG_WHERE(whereat) \
    { \
	TRdisplay("%@ %s \n", whereat); \
    }
#else
#define LG_WHERE(whereat) ;
#endif

#undef	LG_SHARED_DEBUG		/* Display debug info for SHARED txns */

/* Read blocks from partitioned log files */
#define LG_READ(lg_io, num_blocks, block_no, buffer, sys_err) \
	DIread(&lg_io->di_io[(block_no) % lg_io->di_io_count], num_blocks, \
	       (block_no) / lg_io->di_io_count, buffer, sys_err)

/* Write blocks to partitioned log files */
#define LG_WRITE(lg_io, num_blocks, block_no, buffer, sys_err) \
	DIwrite(&lg_io->di_io[(block_no) % lg_io->di_io_count], num_blocks, \
	       (block_no) / lg_io->di_io_count, buffer, sys_err)

/* If local waiter, call CSresume directly */
#define		LGcp_resume(cpid) \
	( (cpid)->pid == LG_my_pid ) \
	 ? CSresume((cpid)->sid) \
	 : CScp_resume(cpid)


/*
**  Forward and/or External typedef/struct references.
*/

/*  Embedded structures. */

typedef struct _LBH	LBH;
typedef struct _LGC	LGC;
typedef struct _LG_ID	LG_ID;
typedef struct _LG_IO	LG_IO;
typedef struct _LRH	LRH;
typedef struct _LWQ	LWQ;
typedef struct _LXBQ	LXBQ;

/*  Dynamic control blocks. */

typedef struct _LBB	LBB;
typedef struct _LDB	LDB;
typedef struct _LGD	LGD;
typedef struct _LPB	LPB;
typedef struct _LPD	LPD;
typedef struct _LXB	LXB;
typedef struct _LXH	LXH;
typedef struct _LFB	LFB;
typedef struct _LFD	LFD;


/*}
** Name: LG_IO - Logging system I/O block
**
** Description:
**      This structure describes the format of a logging system I/O block.
**
** History:
**	26-Jan-1998 (jenjo02)
**	    Created for Partitioned Log File Project.
*/
struct _LG_IO
{
    i4		di_io_count;		/* Number of log files */
    DI_IO	di_io[LG_MAX_FILE];
};


GLOBALREF   LG_IO   Lg_di_cbs[2];

/*}
** Name: LG_ID - Logging control block identifier.
**
** Description:
**      This structure describes the format of a logging system identifier.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Add i4 to LG_ID converter union.  Originally this was meant
**	    to allow -O2 with gcc, but it has since become apparent that
**	    strict type aliasing (as defined by C99) will *never* work
**	    in Ingres.  The i4-to-LG_ID stuff was particularly egregious,
**	    though, so I am leaving it in.
**	    (The key to -O2 turned out to be -fno-strict-aliasing -fwrapv.)
*/
struct _LG_ID
{
    u_i2	id_instance;		/* The instance of this identifier. */
    u_i2	id_id;			/* The identifier. */
};

/* For proper type punning, need a conversion from an i4 (for example,
** LG_LXID or LG_DBID) to an LG_ID (pair of i2's).
*/
typedef union _LG_I4ID_TO_ID
{
    i4		id_i4id;		/* As an i4 */
    LG_ID	id_lgid;		/* As an LG_ID struct */
} LG_I4ID_TO_ID;

/*}
** Name: LXBQ - Link LXB blocks.
**
** Description:
**      This structure decribes a link to a LXB block used for queueing LXB's
**	to various lists.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
*/
struct _LXBQ
{
    SIZE_TYPE	lxbq_next;
    SIZE_TYPE	lxbq_prev;
};

/*}
** Name: LWQ - Logging wait queue.
**
** Description:
*      This structure is used to queue LXB's onto a wait queue by
**	the internal suspend routine.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	22-Jan-1996 (jenjo02)
**	    Added lwq_mutex.
*/
struct _LWQ
{
    LXBQ            lwq_lxbq;           /* Queue of waiting LXB's. */
    i4	    	    lwq_count;		/* Count of waiting LXB's. */
    CS_SEMAPHORE    lwq_mutex;		/* mutex to protect wait queue */
};

/*
** Name: LFB		- the Log File context Block
**
** Description:
**	The LFB holds information about a particular logfile (or pair of log
**	files, in the dual logging case). In the non-cluster case, there is only
**	one log file, and the LFB information is directly reachable because the
**	local log's LFB is a component of the lgd (lgd_local_lfb).
**
**	In the cluster case, there is an LFB per logfile (pair) that is being
**	accessed, since during cluster restart recovery the DMFCSP process
**	needs to simultaneously manage the recovery of multiple logfiles.
**
**	These logfile blocks are allocated by LGopen and freed by LGclose.
**
**	LFB Fields:
**
**	lfb_archive_window_start  : Log Address of start of the Archive Window.
**	lfb_archive_window_end    : Log address of end of the Archive Window.
**
**		Together the archive_window_start and archive_window_end values
**		form the active portion of the log file which requires archive 
**		processing.
**
**		The end log address is updated by new Log Writes and the begin
**		is moved forward by the archiver as it processes journal and 
**		dump records.
**
**		This archive window will always be the union of all of the 
**		individual database journal and dump windows.
**
**		Note that in the logging system tradition, the start window
**		log address is really the log address of the log record
**		just previous to the first dump or journal record.  This
**		is because when a client of LG wants to read the log file in
**		forward reading mode, and positions to a Log Address and starts
**		reading, the first record returned is the record AFTER the
**		specified log addresss.
**
**		The end window log address is the actual log address of the
**		last journal or dump log record.
**
**	lfb_archive_window_prevcp : CP previous to the Archive Window.
**
**		This value gives the Log Address of the most recent Consistency
**		Point previous to the start of the current archive window 
**		(lfb_archive_window_start).
**
**		Its value is set whenever the archive_window_start value is 
**		initialized or updated.  It marks the spot to which the Log 
**		File BOF cannot be moved forward before more Archiver work
**		is performed.
**
** History:
**	26-apr-1993 (bryanp)
**	    Split this information out from the LGD to support cluster log
**	    needs.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Added a new field - lfb_archive_window_prevcp - 
**	    which gives the consistency point address before which we know 
**	    there is no archiver processing required.  Consolidated the system
**	    journal and dump windows into a single lfb_archive_window.
**	18-oct-1993 (rogerk)
**	    Made lfb_reserved_space unsigned.
**	05-Jan-1996 (jenjo02)
**	    Added lfb_mutex, lfb_fq_count,
**	    lfb_wq_mutex, lfb_wq_count, lfb_current_lbb_mutex.
**	    Removed lfb_wasted_count.
**	10-Jul-1996 (jenjo02)
**	    Added buf_util to LGD to track utilization of
**	    log records.
**	24-Aug-1999 (jenjo02)
**	    Removed lfb_cp_mutex, lfb_current_lbb_mutex.
**	08-apr-2002 (devjo01)
**	    Add lfb_first_lbb to record beginning of allocation extent.
**	5-Jan-2006 (kschendel)
**	    Reserved-space needs to be larger for giant logs.
**	21-Mar-2006 (jenjo02)
**	    Added lfb_fq_leof.
**	15-Jan-2010 (jonj)
**	    lfb_next should be SIZE_TYPE, not i4. Deleted unused lfb_prev.
*/
struct _LFB
{
    SIZE_TYPE  	    lfb_next;           /* Next LFB on free queue */
    i4		    lfb_type;		/* Type of control block. */
#define			LFB_TYPE	    17
    LG_ID	    lfb_id;		/* Control block identifier. */
    CS_SEMAPHORE    lfb_mutex;		/* A mutex to protect parts of the LFB */
    u_i4	    lfb_status;
#define			LFB_USE_DIIO		    0x000001L
#define			LFB_PRIMARY_LOG_OPENED	    0x000002L
#define			LFB_DUAL_LOG_OPENED	    0x000004L
#define			LFB_DUAL_LOGGING	    0x000008L
#define			LFB_DISABLE_DUAL_LOGGING    0x000010L
    LG_HEADER	    lfb_header;
    LG_LA	    lfb_forced_lga;
    LG_LSN	    lfb_forced_lsn;
    LG_LA           lfb_archive_window_start;	/* Start of active acp window */
    LG_LA           lfb_archive_window_end;	/* End of active acp window */
    LG_LA           lfb_archive_window_prevcp;	/* CP previous to the start
						** of the current acp window. */
    u_i4	    lfb_active_log;
#define		    LGD_II_LOG_FILE	0x01
#define		    LGD_II_DUAL_LOG	0x02
    SIZE_TYPE	    lfb_current_lbb;	/* Current buffer. */
    SIZE_TYPE	    lfb_first_lbb;	/* 1st LBB in contiguous alloc. ext. */
    CS_SEMAPHORE    lfb_fq_mutex;	/* A mutex to protect the fq */
    i4	    	    lfb_fq_count;	/* count of buffers on fq */
    SIZE_TYPE	    lfb_fq_next;	/* First free buffer. */
    SIZE_TYPE	    lfb_fq_prev;	/* Last free buffer. */
    LWQ		    lfb_wait_buffer;	/* Free buffer wait queue.
					** (ie lxb's waiting for a buffer).
					** Note! This queue is protected by
					** lfb_fq_mutex, and the mutex embedded
					** into this LWQ is not used.
					*/
    SIZE_TYPE	    lfb_fq_leof;	/* Logical end of free queue */
    CS_SEMAPHORE    lfb_wq_mutex;	/* A mutex to protect the wq */
    i4	    	    lfb_wq_count;	/* count of buffers on wq */
    SIZE_TYPE	    lfb_wq_next;	/* First buffer to write. */
    SIZE_TYPE	    lfb_wq_prev;	/* Last buffer to write. */
    LG_IO	    lfb_di_cbs[2];	/* primary and dual LG_IO structs */
    i4	    	    lfb_buf_cnt;	/* Count of buffers. */
    u_i8    	    lfb_reserved_space;	/* Bytes of logfile space reserved */
    u_i4	    lfb_channel_blk;	/* 
					** Last block written/read to/from the
					** file associated with the main channel
					*/
    u_i4	    lfb_dual_channel_blk; /*
					** Last block written/read to/from the
					** file associated with the dual channel
					*/
    i4	    	    lfb_l_filename;	/* Length of log file name. */
    char	    lfb_filename[128];	/* Log file name. */
    i4		    lfb_l_nodename;	/* Length of node name. */
    char	    lfb_nodename[80];	/* Node name. */
					/* CX_MAX_NODE_NAME_LEN + 1 (73) */
    struct
    {
      i4	    split;		/* Log splits by transaction. */
      i4	    write;		/* Log writes by transaction. */
      i4	    force;		/* Log force by transaction. */
      i4	    wait;		/* Log waits. */
      i4	    end;		/* transactions ended on this log */
      i4	    writeio;		/* actual write I/O's to this log */
      i4	    bytes;		/* modulo kb bytes written */
      i4	    kbytes;		/* kb written to this log */
      i4	    log_readio;		/* Number of read from the II_LOG_FILE. */
      i4	    dual_readio;	/* Number of read from the II_DUAL_LOG. */
      i4	    log_writeio;	/* Number of write complete to the II_LOG_FILE. */
      i4	    dual_writeio;	/* Number of write complete to the II_DUAL_LOG. */
    }		    lfb_stat;
};

/*}
** Name: LBH - Log buffer header.
**
** Description:
**      This defines the format of a log buffer header.
**
**	lbh_address:
**	    the log address of the last complete log record written to the
**	    page. This log address is used to position to a full record on an
**	    arbitrary page. If the log address offset is 0, then either nothing
**	    was written to the page or a very long log record that spans
**	    multiple log pages was written to this page. Initialized by
**	    LG_allocate_lbb() and set by LG_write().
**	lbh_checksum:
**	    The checksum of the log page. The checksum is used to guarantee
**	    that the log was successfully written and subsequently read from
**	    the log file. The checksum includes the log page header and chunks
**	    of data from various sections of the log page. The contents of the
**	    whole log page are not included because of the cost in CPU.
**	    Set by chk_sum() call from write_block().
**	lbh_used:
**	    The number of bytes used on this log page, <= lgh_size. Set by
**	    queue_write().
**	lgh_extra:
**	    An extra field that is initialized to zero by queue_write().
**	lbh_hlps:
**	    High word of log page stamp, for cluster support. Set by
**	    get_pagestamp() call from write_block().
**	lbh_llps:
**	    Low word of log page stamp, for cluster support. Set by
**	    get_pagestamp() call from write_block().
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	26-jul-1993 (bryanp)
**	    Made lbh_used into u_i2 and lbh_extra into i2 (they were "short").
**	    Also, lbh_hlps and lbh_llps are no longer used, but I have not
**		removed them from the LBH because that affects more code than
**		I want to touch at this late date.
*/
struct _LBH
{
    LG_LA	    lbh_address;	/* Log buffer address. */
    i4	    	    lbh_checksum;	/* Checksum of buffer. */
    u_i2	    lbh_used;		/* Number of used bytes on page. */
    i2		    lbh_extra;		/* Extra: must be 0 */

    i4         	    lbh_hlps;           /* log page stamp high value. */
    i4         	    lbh_llps;           /* log page stamp low value. */
};

/*}
** Name: LGC - Log context
**
** Description:
**      The log context holds position information between calls to LGposition
**	and LGread.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	26-jul-1993 (bryanp)
**	    Turned lgc_direction and lgc_position into nat's (were short's).
**	10-oct-93 (swm)
**	    Bug #56439
**	    Changed lgc_check type from i4 to PTR. The check can be
**	    done as a direct pointer comparison rather than as a (i4)
**	    checksum.
**	14-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added lgc_bufid, lgc_bufcnt, lgc_current_lbb
**	    to enable reading from the log buffers, 
**	    lgc_blocks, lgc_numblocks, lgc_curblock, lgc_readblocks,
**	    lgc_firstblock for bulk reads,
**	    lgc_readio count of physical I/O done.
*/
struct _LGC
{
    i4	    	    lgc_status;		/* Status of context. */
#define			LGC_VALID	0x0001
#define			LGC_PAGE	0x0002
    PTR		    lgc_check;          /* Check value. */
    i4	    	    lgc_size;		/* Block size. */
    i4	    	    lgc_count;		/* Number of blocks. */
    i4	    	    lgc_low;		/* Low offset for current buffer. */
    i4	    	    lgc_offset;		/* Current offset in buffer. */
    i4	    	    lgc_high;		/* High offset for current buffer. */
    i4	    	    lgc_min;		/* Minimum legal lga. */
    i4	    	    lgc_max;		/* Maximum legal lga. */
    i4		    lgc_direction;	/* Read direction. */
    i4		    lgc_position;	/* Read position. */
    LG_LA	    lgc_lga;		/* Next lga address. */
    i4		    lgc_bufid;		/* Next lga buffer id */
    i4		    lgc_readio;		/* Count of physical I/O reads */
    i4		    lgc_bufcnt;		/* Number of LBB buffers */
    LG_LA	    lgc_current_lga;	/* Address of current record. */
    bool	    lgc_current_lbb;	/* Read from LBB_CURRENT buffer */
    LRH		    *lgc_record;	/* Location of split record. */
    LBH		    *lgc_buffer;	/* The log buffer. */
    char	    *lgc_blocks;	/* Log block pool */
    i4		    lgc_numblocks;	/* How many blocks can fit */
    i4		    lgc_curblock;	/* Current block */
    i4		    lgc_firstblock;	/* First block to read */
    i4		    lgc_readblocks;	/* Number of blocks read */
};

/*}
** Name: LRH - Log record header.
**
** Description:
**      This defines the format of a log record header.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	26-jul-1993 (bryanp)
**	    Changed lrh_sequence from unsigned short to u_i2, lrh_extra from
**		short to i2.
*/
struct _LRH
{
    i4         	    lrh_length;         /* Length of the log record. */
    u_i2	    lrh_sequence;	/* Sequence number for this tran_id. */
    i2		    lrh_extra;
    LG_LA	    lrh_prev_lga;	/* Previous LGA for transaction. */
};

/*}
** Name: LXH - Logging transaction hash queue entry.
**
** Description:
**      This structure describes the format of a logging system 
**	transaction hash queue entry. It's embedded in each LXB.
**	
**	An LXB is added to this hash queue when it becomes active
**	and is removed when the transaction ends.
**
**	These queues are protected with the lgd_lxb_aq_mutex.
**
** History:
**	24-Aug-1998 (jenjo02)
**	    created.
*/
struct _LXH
{
    LXBQ	lxh_lxbq;		/* next/prior entries in list */
    DB_TRAN_ID	lxh_tran_id;		/* Transaction id */
};

/*}
** Name: LBB - A log buffer.
**
** Description:
**      This structure describes the format of a log buffer header.
**
**	This structure maps only the beginning portion of each log buffer.
**	Actual log buffers are larger than the size of this buffer -- there is
**	an actual log page in aligned space following this structure, and the
**	lbb_buffer field can be used to locate that actual log page.
**
**	Note the distinction between lbb_next_byte and lbb_bytes_used:
**	    lbb_next_byte is the offset into the LG/LK shared memory of the
**			  next byte to be used in this buffer.
**	    lbb_bytes_used is the number of bytes in this buffer which have
**			  been used.
**	Use lbb_next_byte to get a process-specific character pointer for
**	copying stuff into the buffer. Use lbb_bytes_used to track the amount
**	of the buffer which has been used.
**
**	Since there can be multiple I/O operations in progress, and since the
**	actual writes can complete out of order, tracking the system-wide
**	forced LGA value is somewhat tricky. We use the lbb_forced_lsn to do
**	this. The lbb_forced_lsn is initialized to be the lbh_address when the
**	buffer is placed on the write queue. When the buffer completes, we
**	determine whether there exists a "greatest previous not-yet-written
**	buffer". If there exists a gprev_lbb, then we set the gprev_lbb to be
**	the maximum of this buffer's lbb_forced_lsn and the gprev_lbb's
**	lbb_forced_lsn. If there does NOT exist a gprev_lbb, then we can now
**	set the lgd_forced_lsn to this buffer's lbb_forced_lsn.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	16-feb-1993 (bryanp)
**	    Added LG_LSN fields for use in tracking force-by-lsn.
**	05-Apr-1996 (jenjo02)
**	    Added lbb_lbh_mutex, lbb_writers.
**	24-Aug-1999 (jenjo02)
**	    Added LBB_CURRENT, lbb_instance, lbb_wait_count, renamed 
**	    lbb_lbh_mutex to lbb_mutex.
**	15-Mar-2006 (jenjo02)
**	    Added lbb_commit_count - number of txns that have
**	    placed an ET in this buffer (and hence will not do
**	    any more LGwrites).
**	21-Mar-2006 (jenjo02)
**	    Deleted lbb_space, which isn't used.
**      01-may-2008 (horda03) bug 120349
**          Added LBB_STATE define.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added lbb_id.
*/
struct _LBB
{
    SIZE_TYPE	    lbb_next;		/* Next log buffer. */  
    SIZE_TYPE	    lbb_prev;		/* Previous log buffer. */
    i4		    lbb_id;		/* Buffer id */
    u_i4	    lbb_instance;	/* Instance of buffer */
    i4	    	    lbb_state;		/* State. */
#define			LBB_FREE		    0x0001L
#define			LBB_MODIFIED		    0x0002L
#define			LBB_FORCE		    0x0004L
#define			LBB_PRIMARY_LXB_ASSIGNED    0x0008L
#define			LBB_DUAL_LXB_ASSIGNED	    0x0010L
#define			LBB_PRIM_IO_INPROG	    0x0020L
#define			LBB_DUAL_IO_INPROG	    0x0040L
#define			LBB_IO_INPROG (LBB_PRIM_IO_INPROG | LBB_DUAL_IO_INPROG)
#define			LBB_PRIM_IO_CMPLT	    0x0080L
#define			LBB_DUAL_IO_CMPLT	    0x0100L
#define			LBB_IO_CMPLT (LBB_PRIM_IO_CMPLT | LBB_DUAL_IO_CMPLT)
#define			LBB_PRIM_IO_NEEDED	    0x0200L
#define			LBB_DUAL_IO_NEEDED	    0x0400L
#define			LBB_ON_WRITE_QUEUE	    0x0800L
#define			LBB_CURRENT	    	    0x1000L

#define LBB_STATE "\
FREE,MODIFIED,FORCE,PRIM_ASSIGNED,DUAL_ASSIGNED,\
PRIM_INPROG,DUAL_INPROG,PRIM_COMPLETE,DUAL_COMPLETE,\
PRIM_NEEDED,DUAL_NEEDED,ON_WRITE_QUEUE,CURRENT"

    i4		    lbb_writers; 	/* Count of concurrent writers */
    i4	    	    lbb_next_byte;	/* offset of next byte in buffer. */
    i4	    	    lbb_bytes_used;	/* Bytes used in buffer. */
    SIZE_TYPE	    lbb_lfb_offset;	/* file for this buffer */
    LXBQ	    lbb_wait;		/* Write completion wait queue. */
    i4		    lbb_wait_count;	/* Count of waiting LXBs */
    i4		    lbb_commit_count;	/* txns waiting for ET */
    LG_LA	    lbb_lga;		/* Starting LGA for this page. */
    LG_LA	    lbb_forced_lga;	/* Resulting forced lga when this page
					** completes.
					*/
    LG_LSN	    lbb_forced_lsn;	/* Resulting forced lsn when this page
					** completes.
					*/
    LG_LSN	    lbb_first_lsn;	/* lsn of first record on this page. */
    LG_LSN	    lbb_last_lsn;	/* lsn of last record on this page. */
    STATUS	    lbb_io_status;	/* IO status result. */
    STATUS	    lbb_dual_io_status;	/* IO status for dual log */
    i4	    	    lbb_resume_cnt;	/* Number of writes complete before 
					** resuming the waiters.
					*/
    SIZE_TYPE	    lbb_owning_lxb;	/* logwriter thread which has been
					** selected to write this buffer.
					** This field is used only for logstat
					** reporting and, in the dual logging
					** case, is not guaranteed to mean 
					** much (since two logwriters may be
					** assigned at once...) */
    SIZE_TYPE	    lbb_buffer;		/* offset to aligned buffer space */
    CS_SEMAPHORE    lbb_mutex;		/* mutex to block updates */
};

/*}
** Name: LDB - Log Database Block.
**
** Description:
**      This structures describes the information recorded for a database
**	attached to the log file.
**
**	Notes on LDB status values:
**
**	    LDB_ACTIVE	    : Database is active.
**	    LDB_JOURNAL	    : Database is journaled.
**	    LDB_INVALID	    : LDB control block is invalid.
**	    LDB_NOTDB	    : LDB control block does not describe an actual
**			      database - rather it is used as an administrative
**			      control block to allow a session to connect to
**			      the logging system and make LG/LK calls without
**			      actually opening a database.  This is done by the
**			      RCP, ACP, as well as DMF utility functions.
**	    LDB_PURGE	    : Log records for this database are being purged
**			      to the journal files.  This operation is normally
**			      signaled when a database is closed.  When the ACP
**			      finishes purging the log records, then a CLOSE
**			      operation will probably be signaled.
**	    LDB_OPENDB_PEND : There is an OPENDB operation pending on this
**			      database.  The RCP should recognize this event and
**			      process the open.  Until the RCP finishes the
**			      open, no transactions can update the database.
**	    LDB_CLOSEDB_PEND: There is a CLOSEDB operation pending on this
**			      database.  The RCP should recognize this event
**			      and process the close.  If the database is
**			      journaled then all log records must be flushed to
**			      the jnl files before the close can be completed.
**			      Some utility operations (such as DESTROYDB,
**			      CHECKPOINT, ROLLFORWARD) must wait for the close
**			      to be completed before running.
**	    LDB_RECOVER	    : The database is being recovered.
**	    LDB_FAST_COMMIT : The database is open with Fast Commit.  If a
**			      server with this database open crashes, then REDO
**			      recovery must be performed.  Also, all other
**			      servers with this database open must be shut down
**			      in order for REDO recovery to be performed.
**	    LDB_PRETEND_CONSISTENT: The database is open even though it is
**			      inconsistent.  This is used by destroydb and
**			      rollforward in order to do work on an inconsistent
**			      database.
**	    LDB_CLOSEDB_DONE:
**	    LDB_OPN_WAIT    : There are transactions waiting for the OPENDB
**			      event to complete so that they can go forward.
**	    LDB_STALL	    : The database is undergoing online backup and is
**			      currently stalled until all active transactions
**			      complete so that the Start Backup log record can
**			      be written.  As soon as the SBACKUP record is
**			      written, all stalled transactions will continue.
**	    LDB_BACKUP	    : The database is undergoing online backup.
**	    LDB_CKPDB_PEND  : 
**	    LDB_FBACKUP	    : The database is undergoing online backup and is
**			      in the FBACKUP (Final Backup) state.  All work
**			      backup work has been performed and all that is
**			      left is for the checkpoint process to update the
**			      config file marking the checkpoint complete.
**	    LDB_CKPERROR    : An error occured on this database during online
**			      backup.  After the checkpoint process has reported
**			      the error, this status will be turned off.
**	    LDB_STALL_RW    : While online checkpoint is stalled (LDB_STALL),
**			      stall any newly starting read_write txns if this
**			      flag is set.
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project: Added ldb_j_last_la and ldb_d_last_la
**	    fields to hold the log address of the last written log records
**	    for the database that require archive processing.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed EBACKUP, PRG_PEND, ONLINE_PURGE database states.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**	    - Add ldb_sback_lsn to the LDB to hold the LSN of the backup start.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    The journal and dump windows are now tracked using the actual
**	    log addresses of the first and last records which define the
**	    window rather than the address of the CP previous to those
**	    log records.  Dump and Journal windows now tracked with two fields:
**	    ldb_[j/d]_first_la and ldb_[j/d]_last_la.
**	05-Jan-1996 (jenjo02)
**	    Added ldb_mutex.
** 	10-jun-1997 (wonst02)
** 	    Added LDB_READONLY flag for readonly databases.
**	13-Nov-1997 (jenjo02)
**	    Added LDB_STALL_RW flag to tell LGbegin() to stall all
**	    new read_write txns while online checkpoint is stalled.
**	12-nov-1998 (kitch01)
**		Added LDB_CLOSE_WAIT flag to wait the open of a database if
**		a close is pending.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ldb_active_lxbq, ldb_last_commit,
**	    ldb_first_la, LDB_MVCC, ldb_last_lsn, ldb_lgid_low,
**	    ldb_lgid_high.
*/
struct _LDB
{
    SIZE_TYPE	    ldb_next;           /* Next LDB in logging system */
    SIZE_TYPE	    ldb_prev;	        /* Prev LDB in logging system */
    i4		    ldb_type;		/* Type of control block. */
#define			LDB_TYPE	    6
    i4		    ldb_lpd_count;	/* Number of LPD references. */
    LG_ID	    ldb_id;		/* Control block identifier. */
    CS_SEMAPHORE    ldb_mutex;		/* A mutex to protect the LDB */
    u_i4	    ldb_status;		/* Status. */
#define                 LDB_ACTIVE          0x00000001
#define                 LDB_JOURNAL         0x00000002
#define                 LDB_INVALID         0x00000004
#define                 LDB_NOTDB           0x00000008
#define                 LDB_PURGE           0x00000010
#define                 LDB_OPENDB_PEND     0x00000020
#define                 LDB_CLOSEDB_PEND    0x00000040
#define                 LDB_RECOVER         0x00000080
#define                 LDB_FAST_COMMIT     0x00000100
#define                 LDB_PRETEND_CONSISTENT 0x00000200
#define                 LDB_CLOSEDB_DONE    0x00000400
#define                 LDB_OPN_WAIT        0x00000800
#define                 LDB_STALL           0x00001000
#define                 LDB_BACKUP          0x00002000
#define                 LDB_CKPDB_PEND      0x00004000
#define			LDB_READONLY	    0x00008000	
#define                 LDB_FBACKUP         0x00010000
#define			LDB_CKPERROR	    0x00020000
#define			LDB_JSWITCH	    0x00040000
#define			LDB_STALL_RW	    0x00080000
#define			LDB_CKPLK_STALL	    0x00100000
#define			LDB_CLOSE_WAIT	    0x00200000
#define			LDB_MVCC	    0x00400000
    i4              ldb_lxbo_count;     /* Number of ongoing xact for ckpdb */
    i4		    ldb_lxb_count;	/* Number of transactions. */
    SIZE_TYPE	    ldb_lfb_offset;	/* logfile for this database */
    LXBQ	    ldb_active_lxbq;	/* List of active txns in this db */
    LG_LA	    ldb_j_first_la;	/* First journal log record. */
    LG_LA	    ldb_j_last_la;	/* Last journal log record. */
    LG_LA           ldb_d_first_la;     /* First dump log record. */
    LG_LA           ldb_d_last_la;      /* Last dump log record. */
    LG_LA           ldb_sbackup;        /* Start backup lga */
    LG_LA	    ldb_first_la;	/* First LGA written in DB */
    LG_LSN	    ldb_sback_lsn;	/* Start backup LSN */
    LG_LSN	    ldb_eback_lsn;	/* End backup LSN for online ckp */
    u_i2	    ldb_lgid_low;	/* Lowest open LXB in DB */
    u_i2	    ldb_lgid_high;	/* Highest open LXB in DB */
    LG_LSN	    ldb_last_commit;	/* LSN of last commit in DB */
    LG_LSN	    ldb_last_lsn;	/* LSN of last write in DB */
    i4	    	    ldb_database_id;	/* External Database Id. */
    i4		    ldb_l_buffer;	/* Length of the information buffer. */
    char	    ldb_buffer[LG_DBINFO_MAX];
					/* Database information buffer. */
    LG_JFIB	    ldb_jfib;		/* Journal info for simulation in
    					** LGwrite and CR undo */
    struct
    {
      i4	    read;		/* Reads by database. */
      i4	    write;		/* Writes by database. */
      i4	    begin;		/* Begins by database. */
      i4	    end;		/* Ends by database. */
      i4	    force;		/* Force by database. */
      i4	    wait;		/* Wait by database. */
    }		ldb_stat;
};

/*}
** Name: LGD - The Log Database.
**
** Description:
**      This structure contains all the global context required to manage the
**	lock database.  This information includes: the process lists, the
**	database lists, and the transaction lists.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	16-oct-1992 (jnash) merged 10-oct-1992 (rogerk)
**	    Reduced logging Project: Removed obsolete lgd_n_servers field.
**	19-oct-1992 (bryanp)
**	    Add lgd_gcmt_threshold field.
**	07-jan-1993 (jnash)
**          Reduced logging project, continued.  Add lgd_reserved_space.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed EBACKUP, FBACKUP, PRG_PEND, ONLINE_PURGE database states.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**	    - much of the LGD is now in the LFB, so that information specific
**		to a particular logfile can be tracked in that logfile.
**	    - Re-instated lgd_cnodeid to hold this node's cluster node ID.
**	    - Add lgd_csp_id to track the CSP Process ID.
**	26-jul-1993 (bryanp)
**	    Made lgd_force_id a i4  (was an unsigned short).
**	18-oct-1993 (rogerk)
**	    Removed unused lgd_dmu_cnt, lgd_stall_limit fields.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Added a host of new mutexes
**	    to reduce single-threading of LG via sole lgd_mutex.
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	    Added lgd_archiver_lpb so we don't have to lock and search
**	    the lpb queue to find out if the Archiver is running.
**	    Added lgd_n_cpagents to keep track of the number of
**	    non-LPB_VOID CPAGENT LPBs so we don't have to lock and 
**	    walk the LPB queue to count them.
**	01-Mar-1996 (jenjo02)
**	    Added lgd_header_lbb_mutex.
**	22-Apr-1996 (jenjo02)
**	    Added lgd_stat.no_logwriter to count the number of times
**	    a buffer needed to be assigned to a logwriter thread but
**	    none were available.
**	10-Jul-1996 (jenjo02)
**	    Added buf_util to LGD to track utilization of
**	    log records.
**	16-Jul-1996 (jenjo02)
**	    Gave lgd_status its own mutex, lgd_status_mutex.
**	01-Oct-1996 (jenjo02)
**	    Added lgd_stat.max_wq and lgd_stat.max_wq_count.
**	17-Dec-1997 (jenjo02)
**	    Renamed lgd_ilwt (queue of idle logwriters) to lgd_lwlxb
**	    (queue of logwriters).
**	24-Aug-1998 (jenjo02)
**	    Added lgd_lxh_buckets, offset to transaction hash buckets.
**	    Each bucket is a LXBQ structure, and the number of buckets
**	    equal to the maximum number of transactions in the
**	    logging system, lgd_lxbb_size.
**	24-Aug-1999 (jenjo02)
**	    Replaced lgd_lxb_aq_mutex, lgd_lxb_iq_mutex, with lgd_lxb_q_mutex,
**	    eliminated lgd_lxb_inext, lgd_lxb_prev. 
**	    Removed obsolete lgd_n_logwriters.
**	    Collapsed lgd_wait_free, lgd_wait_split into a single buffer
**	    wait queue, lgd_wait_buffer.
**	    Removed lgd_timer_lbb, lgd_timer_lbb_last_used, 
**	    lgd_timer_lbb_total_ticks.
**	    Removed lgd_status_mutex, use lgd_mutex in its place.
**	15-Dec-1999 (jenjo02)
**	    In support of SHARED transactions, added lgd_slxb,
**	    replace lgd_force_id with a queue of LXBs to be aborted,
**	    lgd_lxb_abort, and a count, lgd_lxb_aborts.
**	08-Apr-2004 (jenjo02)
**	    Added lgd_wait_mini, queue of Handle LXBs awaiting 
**	    completion of Mini-transaction.
**	15-Mar-2006 (jenjo02)
**	    Deleted lgd_stat.wait, free_wait, stall_wait, bcp_stall_wait,
**	    now embededd in stat.wait[] array along with other 
**	    wait reason counters.
**	21-Mar-2006 (jenjo02)
**	    Add lgd_state_flags for optimized writes and because
**	    lgd_status is full. Add optimwrites, optimpages to
**	    lgd_stats.
**	28-Aug-2006 (jonj)
**	    Add lgd_lfdb_? to support new LFD structure.
**	20-Sep-2006 (kschendel)
**	    Move the wait-buffer queue to the LFB.
**	11-Sep-2006 (jonj)
**	    Add lgd_wait_commit wait queue for LOGFULL_COMMIT.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lgd_xid_array, lgd_xid_array_size.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add lgd_rbblocks, lgd_rfblocks for
**	    buffered log reads.
*/
struct _LGD
{
    CS_SEMAPHORE    lgd_mutex;		/* mutex to protect LGD itself */
    u_i4    	    lgd_status;		/* Logging System status mask.
					** Definitions in lgdstat.h */
    u_i4	    lgd_state_flags;	/* Odds and ends that don't
					** really belong in lgd_status: */
#define		LGD_OPTIMWRITE		0x000001L /* Optimize writes */
#define		LGD_OPTIMWRITE_TRACE	0x000002L /* Trace optimized writes */

    LFB		    lgd_local_lfb;
    SIZE_TYPE	    lgd_master_lpb;	/* Master LPB process. */
    SIZE_TYPE  	    lgd_archiver_lpb;	/* Archiver's LPB */
    CS_SEMAPHORE    lgd_lpb_q_mutex;	/* LPB queue mutex */
    SIZE_TYPE	    lgd_lpb_next;	/* Next active LPB. */
    SIZE_TYPE	    lgd_lpb_prev;	/* Last active LPB. */
    CS_SEMAPHORE    lgd_ldb_q_mutex;	/* LDB queue mutex */
    SIZE_TYPE	    lgd_ldb_next;	/* Next active LDB. */
    SIZE_TYPE	    lgd_ldb_prev;	/* Last active LDB. */
    CS_SEMAPHORE    lgd_lxb_q_mutex;	/* LXB queue mutex */
    SIZE_TYPE	    lgd_lxb_next;	/* Oldest active+protect LXB. */
    SIZE_TYPE	    lgd_lxb_prev;	/* Newest active+protect LXB. */
    LXBQ	    lgd_slxb;		/* Queue of SHARED LXBs */
    SIZE_TYPE	    lgd_lxh_buckets;	/* Offset to txn hash buckets */

    i4	    	    lgd_protect_count;	/* Number of protect transactions. */
    i4	    	    lgd_n_cpagents; 	/* Num of non-VOID CPAGENT LPBs */
    i4	    	    lgd_no_bcp;		/* CP in progress, don't start new one*/
    i4	    	    lgd_delay_bcp;	/* Start new CP when current one done */

    CS_SEMAPHORE    lgd_lfbb_mutex;  	/* Mutex to protect LFBB */
    SIZE_TYPE	    lgd_lfbb_table;	/* Pointer to LFBB offset table. */
    i4	    	    lgd_lfbb_count;	/* Allocated LFBB entries. */
    i4	    	    lgd_lfbb_size;	/* Maximum LFBB entries. */
    SIZE_TYPE	    lgd_lfbb_free;	/* LFBB free list. */

    CS_SEMAPHORE    lgd_ldbb_mutex;  	/* Mutex to protect LDBB */
    SIZE_TYPE	    lgd_ldbb_table;	/* Pointer to LDBB offset table. */
    i4	    	    lgd_ldbb_count;	/* Allocated LDBB entries. */
    i4	    	    lgd_ldbb_size;	/* Maximum LDBB entries. */
    SIZE_TYPE	    lgd_ldbb_free;	/* LDBB free list. */
    i4	    	    lgd_ldb_inuse;	/* Number of LDB's in use. */

    CS_SEMAPHORE    lgd_lpbb_mutex;  	/* Mutex to protect LPBB */
    SIZE_TYPE	    lgd_lpbb_table;	/* Pointer to LPBB offset table. */
    i4	    	    lgd_lpbb_count;	/* Allocated LPBB entries. */
    i4	    	    lgd_lpbb_size;	/* Maximum LPBB entries. */
    SIZE_TYPE	    lgd_lpbb_free;	/* LPBB free list. */
    i4	    	    lgd_lpb_inuse;	/* Number of LPB's in use. */

    CS_SEMAPHORE    lgd_lxbb_mutex;  	/* Mutex to protect LXBB */
    SIZE_TYPE	    lgd_lxbb_table;	/* Pointer to LXBB offset table. */
    i4	    	    lgd_lxbb_count;	/* Allocated LXBB entries. */
    i4	    	    lgd_lxbb_size;	/* Maximum LXBB entries. */
    SIZE_TYPE	    lgd_lxbb_free;	/* LXBB free list. */
    i4	    	    lgd_lxb_inuse;	/* Number of LXB's in use. */

    i4		    lgd_rbblocks;	/* Number of blocks to allocate in
    					** LCTX when reading backward */
    i4		    lgd_rfblocks;	/* Number of blocks to allocate in
    					** LCTX when reading forward */

    SIZE_TYPE	    lgd_xid_array;	/* Offset to tranid array */
    i4		    lgd_xid_array_size; /* Size of array */
/* Macro to set tranid of active LXB "x" in xid_array */
#define SET_XID_ACTIVE(x) \
    {\
	u_i4	*xid = (u_i4*)LGK_PTR_FROM_OFFSET(lgd->lgd_xid_array);\
	xid[(x)->lxb_id.id_id] = (x)->lxb_lxh.lxh_tran_id.db_low_tran;\
    }
/* Macro to clear LXB "x" tranid in xid_array */
#define SET_XID_INACTIVE(x) \
    {\
	u_i4	*xid = (u_i4*)LGK_PTR_FROM_OFFSET(lgd->lgd_xid_array);\
	xid[(x)->lxb_id.id_id] = 0;\
    }

    CS_SEMAPHORE    lgd_lpdb_mutex;  	/* Mutex to protect LPDB */
    SIZE_TYPE	    lgd_lpdb_table;	/* Pointer to LPDB offset table. */
    i4	    	    lgd_lpdb_count;	/* Allocated LPDB entries. */
    i4	    	    lgd_lpdb_size;	/* Maximum LPDB entries. */
    SIZE_TYPE	    lgd_lpdb_free;	/* LPDB free list. */
    i4	    	    lgd_lpd_inuse;	/* Number of LPD's in use. */

    CS_SEMAPHORE    lgd_lfdb_mutex;  	/* Mutex to protect LFDB */
    SIZE_TYPE	    lgd_lfdb_table;	/* Pointer to LFDB offset table. */
    i4	    	    lgd_lfdb_count;	/* Allocated LFDB entries. */
    i4	    	    lgd_lfdb_size;	/* Maximum LFDB entries. */
    SIZE_TYPE	    lgd_lfdb_free;	/* LFDB free list. */
    i4	    	    lgd_lfd_inuse;	/* Number of LFD's in use. */

    i4	    	    lgd_cnodeid;	/* node ID number for this local node.
					** If not a cluster, this  will be zero.
					** If a cluster, this number will be
					** set by the CSP through LGalter. The
					** logging system doesn't actually
					** use this field; it just stores it for
					** use by clients.
					*/
    PID		    lgd_csp_pid;	/* The CSP's Process ID. */
    LWQ		    lgd_lwlxb;		/* Queue of LogWriter LXBs */
    LWQ		    lgd_wait_stall;	/* Stall wait queue. */
    LWQ		    lgd_wait_event;	/* Event wait queue. */
    LWQ		    lgd_open_event;	/* Open Event wait queue. */
    LWQ             lgd_ckpdb_event;    /* Online Backup stall queue */
    LWQ		    lgd_wait_mini;	/* queue of Handles waiting
					** for minitransaction 
					** completion */
    LWQ		    lgd_wait_commit;	/* queue of Handles waiting
					** for LOGFULL_COMMIT 
					** completion */
    struct
    {
      i4	    add;		/* Log add calls. */
      i4	    remove;		/* Log remove calls. */
      i4	    begin;		/* Log transaction begins. */
      i4	    end;		/* Log transaction end. */
      i4	    write;		/* Log write calls. */
      i4	    split;		/* Log splits. */
      i4	    force;		/* Log force calls. */
      i4	    readio;		/* Log read I/O's. */
      i4	    writeio;		/* Log write I/O's. */
      i4	    group_force;	/* Number of group forces. */
      i4	    group_count;	/* Number of forces in group. */
      i4	    inconsist_db;	/* Number of inconsistent databases
					** occurred. */
      i4	    pgyback_check;	/* Number of pgyback_write checks. */
      i4	    pgyback_write;	/* Writes initiated by piggy-back. */
      i4	    pgyback_write_time; /* Writes due to numticks */
      i4	    pgyback_write_idle; /* Writes do to inactivity */
      i4	    kbytes;		/* K bytes written to log file. */
      i4	    log_readio;		/* Number of read from the II_LOG_FILE. */
      i4	    dual_readio;	/* Number of read from the II_DUAL_LOG. */
      i4	    log_writeio;	/* Number of write complete to the II_LOG_FILE. */
      i4	    dual_writeio;	/* Number of write complete to the II_DUAL_LOG. */
      i4	    optimwrites;	/* Number of optimized writes */
      i4	    optimpages;		/* Number of optimized pages */
      i4	    no_logwriter;	/* times lw needed but all were busy */
      i4	    max_wq;		/* maximum length of write queues */
      i4	    max_wq_count;	/* #times we hit that max */
      i4	    buf_util_sum;	/* Sum of all buffer buckets */
      i4	    buf_util[10];       /* histogram of buffer fullness */
      i4	    wait[LG_WAIT_REASON_MAX+1];
      i4	    stat_misc[72];	/* Misc. stats */
    }		    lgd_stat;
    LDB		    lgd_spec_ldb;	/* Special NOTDB LDB. */
    LXBQ	    lgd_lxb_abort;	/* List of LXBs to be aborted */
    i4		    lgd_lxb_aborts;     /* Number of LXBs on that list */
    i4	    	    lgd_cpstall;	/* Maximum log file can grow to while
					** executing a consistency point. */
    i4	    	    lgd_check_stall;	/* Point at which we should begin
					** checking for various stall and
					** logfull conditions.  This is
					** used for performance reasons and
					** should be the lesser of the various
					** stall limits : lgh_l_logfull,
					** lgh_l_abort, and lgd_cpstall.  */
    i4	    	    lgd_serial_io;	/* If non-zero, this specifies that
					** the dual logging I/Os should be
					** performed serially rather than in
					** parallel. Default is parallel mode.
					*/
    SIZE_TYPE	    lgd_header_lbb;	/* Address of the magic reserved LBB
					** used for flushing the log file
					** headers when dual logging fails.
					*/
    i4	    	    lgd_gcmt_threshold; /* Threshold value at which group
					** commit should be performed. As soon
					** as there are more active protected
					** transactions than this we'll start
					** doing group commit.
					*/
    i4	    	    lgd_gcmt_numticks;	/* Number of piggy-back write "ticks"
					** after which a buffer has waited
					** long enough and should be forced
					** to disk.
					*/
    LG_RECOVER	    lgd_recover;	/* For rcpconfig -rcp_ignore stuff */

    i4	    	    lgd_test_badblk;	/* if non-zero, then this field contains
					** a (random) block number which has
					** been selected to simulate a write
					** error or read error for automated
					** testing.
					*/
#define	LGD_MAX_NUM_TESTS   1024
    char	    lgd_test_bitvec[(LGD_MAX_NUM_TESTS/BITSPERBYTE)];
					/* lgd_test_bitvec is a BT bitvector
					** of LG testpoints. The testpoints
					** are indentified by LG_T_* equates
					** in <lg.h>.
					*/
};

/*}
** Name: LPB - Log process block.
**
** Description:
**      This structure defines information about a process that has the logging
**	system open.
**
**	WARNING: The LPB, LXB, and LPD structures must all share a common
**	header, or the allocate/deallocate code in LGCBMEM.C will break!
**
**	Each process has its own queue of logfile buffers which it is going to
**	write. These buffers are taken from and returned to the system-wide
**	queue of buffers maintained in the LGD.
**
**      The process status fields indicate the following:
**
**          LPB_ACTIVE          - process is still active
**          LPB_MASTER          - process is master recovery process (RCP)
**          LPB_ARCHIVER        - process is archiver process (ACP)
**          LPB_FCT             - process is a fast commit server
**          LPB_RUNAWAY         - process is a server that has died abnormally
**                                and therefore may require recovery
**          LPB_SLAVE           - process is a server
**          LPB_CKPDB           - process is checkpoint process
**          LPB_VOID            - process is dead and recovered, but there is
**                                still some transaction stuff left to resolve,
**                                so we need to leave the lpb around.
**          LPB_SHARED_BUFMGR   - process is a server that is connected to a
**                                shared buffer manager.
**          LPB_IDLE            - process is idle : used when process is
**                                connected to a shared buffer manager to
**                                indicate that the process can exit without
**                                causing the buffer manager to be corrupted.
**          LPB_DYING           - process has been killed by logging system but
**                                its rundown routine has not been run yet.
**          LPB_DEAD            - process has died : used when logging system
**                                waiting for some other event (normally the
**                                death of another server) before recovering
**                                the process.
**	    LPB_CPAGENT 	- process participates in Consistency Point
**				  protocols.  When a CP is signaled, LG must
**				  wait for it to respond that it has flushed
**				  its data cache.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	16-oct-1992 (jnash) merged 10-oct-1992 (rogerk)
**	    Reduced logging project.   Add LPB_CPAGENT process status.
**	08-sep-93 (swm)
**	    Changed type of session ids. from i4  to CS_SID to match recent
**	    CL interface revision.
**	18-oct-1993 (rogerk)
**	    Added lpb_cache_factor for logspace reservation work.
**	05-Jan-1996 (jenjo02)
**	    Added lpb_mutex.
**	26-Jan-1998 (jenjo02)
**	    Replaced lpb_gcmt_sid with lpb_gcmt_lxb, removed lpb_gcmt_asleep.
*/
struct _LPB
{
    SIZE_TYPE	    lpb_next;		    /* Next transaction for this process. */
    SIZE_TYPE	    lpb_prev;		    /* Next transaction for this process. */
    i4		    lpb_type;		    /* The type of control block. */
#define			LPB_TYPE	    4
    i4		    lpb_lpd_count;	    /* Count of open databases. */
    LG_ID	    lpb_id;		    /* Control block identifier. */
    CS_SEMAPHORE    lpb_mutex;		    /* Mutex to protect LPB */
    u_i4	    lpb_status;		    /* State of the process. */
#define			LPB_ACTIVE	    0x000001
#define			LPB_MASTER	    0x000002
#define			LPB_ARCHIVER	    0x000004
#define			LPB_FCT		    0x000008
#define			LPB_RUNAWAY	    0x000010
#define			LPB_SLAVE	    0x000020
#define                 LPB_CKPDB           0x000040
#define			LPB_VOID	    0x000080
#define                 LPB_SHARED_BUFMGR   0x000100
#define                 LPB_IDLE            0x000200
#define                 LPB_DEAD            0x000400
#define                 LPB_DYING           0x000800
#define                 LPB_FOREIGN_RUNDOWN 0x001000
#define			LPB_CPAGENT	    0x002000
    PID		    lpb_pid;		    /* Process id. */
    i4		    lpb_cond;		    /* Fast Commit server condition */
#define			LPB_CPREADY	    1	/* ready for next CP */
#define			LPB_CPWAIT	    2	/* waiting for CPWAKEUP */
#define			LPB_RECOVER	    3	/* being recovered */
    SIZE_TYPE	    lpb_lpd_next;	    /* Next active database. */
    SIZE_TYPE	    lpb_lpd_prev;	    /* Last active database. */
    i4         	    lpb_bufmgr_id;          /* Id of process' Buffer Manager */
    i4         	    lpb_cache_factor;       /* Parameter based on the process'
					    ** Buffer Manager cache size. Used
					    ** in logspace reserve algorithms */
    SIZE_TYPE	    lpb_lfb_offset;	    /* logfile for this process */
    VOID	    (*lpb_force_abort)();   /* This process's force abort
					    ** handler. This function pointer
					    ** is valid only when used by the
					    ** same process which set it.
					    */
    CS_CPID	    lpb_force_abort_cpid;   /* session ID of this process's
					    ** force abort special thread
					    */
    SIZE_TYPE	    lpb_gcmt_lxb;	    /* LXB of this process's
					    ** group commit special thread
					    */
    struct
    {
      i4	    readio;		    /* Log read io's per process. */
      i4	    write;		    /* Log writes per process. */
      i4	    force;		    /* Log force per process. */
      i4	    wait;		    /* Log waits. */
      i4	    begin;                  /* Log begin per process. */
      i4	    end;		    /* Log end per process. */
    }		    lpb_stat;
};

/*}
** Name: LPD - Log process database reference
**
** Description:
**      This structures describes a process reference to an existing
**	database. The LPD is a "connector block" between an LPB and an LDB
**	and contains a pointer to these blocks. A process's LPDs are kept
**	on a list. The LPD, in turn, is the head of the list of all those
**	transactions which this process has active against this database.
**
**	WARNING: The LPB, LXB, and LPD structures must all share a common
**	header, or the allocate/deallocate code in LGCBMEM.C will break!
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
*/
struct _LPD
{
    SIZE_TYPE	    lpd_next;          /* Next LPD by LPB. */
    SIZE_TYPE	    lpd_prev;		/* Previous LPD by LPB. */
    i4		    lpd_type;		/* Type of control block. */
#define			LPD_TYPE	    9
    i4		    lpd_lxb_count;	/* Count of active transactions. */
    LG_ID	    lpd_id;		/* Control block identifier. */
    SIZE_TYPE	    lpd_ldb;		/* offset to Database referenced. */
    SIZE_TYPE	    lpd_lpb;		/* offset to Process referenced. */
    LXBQ	    lpd_lxbq;		/* Queue of LXB for this process. */
    struct
    {
      i4	    write;		/* Log writes by database. */
      i4	    force;		/* Log forces by database. */
      i4	    wait;		/* Log waits. */
      i4	    begin;		/* Log begins by database. */
      i4	    end;		/* Log ends by database. */
    }		    lpd_stat;
};

/*}
** Name: LXB - Log Transaction Block.
**
** Description:
**      This structures defines information about a transaction in progress.
**
**	WARNING: The LPB, LXB, and LPD structures must all share a common
**	header, or the allocate/deallocate code in LGCBMEM.C will break!
**
**      Lxb status definitions:
**	    LXB_INACTIVE      - Transactions are marked inactive until the first
**	    LXB_ACTIVE		log record is written.  The first log record
**				must be a begin transaction record.  The tran
**				is then marked as LXB_ACTIVE.  NONPROTECT 
**				transactions are never marked ACTIVE.
**	    LXB_PROTECT	      - Transaction is protected from the Archiver.  The
**				archiver will not advance the BOF past this
**				begin transaction record for this transaction
**				until the transaction has ended.
**	    LXB_JOURNAL	      - Records for this transaction should be archived
**				to the journal file.
**	    LXB_READONLY      - No database changes will be made inside this
**				transaction.  There should be no log writes made
**				on behalf of it.
**	    LXB_NOABORT	      - This transaction will never be chosen as the
**				oldest transaction to force abort.  Only used
**				in special places (archiver) where we know we
**				will not need to force abort the transaction.
**	    LXB_RECOVER	      - Transaction needs recovery (usually because the
**				server has died).
**	    LXB_FORCE_ABORT   - Transaction has been chosen as the oldest tran
**				to abort when the log has become full.
**	    LXB_SESSION_ABORT - Transaction is marked SESSION_ABORT when RCP
**				is going to be signalled to abort the single
**				transaction.
**	    LXB_SERVER_ABORT  -
**	    LXB_PASS_ABORT    -
**	    LXB_DBINCONST     - If the database becomes inconsistent while a
**			        transaction is active, it is marked DBINCONST.
**	    LXB_WAIT	      - Session is suspended waiting for some log
**				event to complete (usually a log write).
**	    LXB_NORESERVE     - Transaction has defined itself as one which
**				will write log records for which it has not
**				preallocated space (via LGreserve).  Used by
**				offline recovery processing where the space
**				reservations done by the original updates have
**				been lost.
**	    LXB_ORPHAN	      - Transaction has been disassociated from its
**				DBMS session and currently has no owner.  The
**				transaction must be adopted before it can
**				be recovered or terminated.
**	    LXB_SHARED	      - Transaction is SHARED, referenced by sharing
**				lxb_handles list of LXB_HANDLEs. The SHARED
**				transaction contains the effects of all the
**				sharing transactions and is the unit of 
**				recovery.
**	    LXB_SHARED_HANDLE - A handle to a SHARED transaction. Handles
**				point to the SHARED LXB via lxb_shared_lxb,
**				and are always NONPROTECT (never needed by
**				recovery).
**	    LXB_ET	      - The ET record has been written for this 
**				(shared) transaction. Used to ensure that
**				referencing handles do not also write an ET.
**	    LXB_PREPARED      - Transaction has issued prepare to commit.
**
**	The lxb_wait_reason field describes the reason that the transaction
**	is waiting as defined in lg.h "LG_WAIT_xxx".
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	8-nov-92 (ed)
**	    remove DB_MAXNAME dependency
**      07-jan-1993 (jnash)
**          Reduced logging project.  Add lxb_reserved_space.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:  Added LXB_NORESERVE status.
**	18-oct-1993 (rogerk)
**	    Fixes to 2PC recovery.
**	    Added LXB_ORPHAN transaction status for xacts which have been
**	    disassociated from their dbms sessions.
**	    Made lxb_reserved_space unsigned.
**	    Removed unused lgd_dmu_cnt.
**	05-Jan-1996 (jenjo02)
**	    Added lxb_mutex. Removed lxb_wasted_count.
**	26-Apr-1996 (jenjo02)
**	    Added lxb_rlbb_count, count of reserved LBBs.
**	09-Jan-1997 (jenjo02)
**	    Added lxb_event_mask, lxb_events, to LXB.
**	18-Aug-1998 (jenjo02)
**	    Replaced lxb_tran_id with lxb_lxh LXH structure, in which
**	    lxb_tran_id is embedded as lxh_tran_id.
**	24-Aug-1999 (jenjo02)
**	    Added lxb_last_lsn, lxb_wait_lsn.
**	15-Dec-1999 (jenjo02)
**	    In support of SHARED transactions, added lxb_slxb,
**	    lxb_handle_count, lxb_handle_preps,
**	    lxb_handles, lxb_shared_lxb, lxb_lgd_abort.
**	08-Apr-2004 (jenjo02)
**	    Added LXB_IN_MINI, LXB_MINI_WAIT, lxb_in_mini.
**	20-Apr-2004 (jenjo02)
**	    Added LG_assign_tran_id() prototype.
**	5-Jan-2006 (kschendel)
**	    Reserved space needs to be larger, it's bytes.
**	15-Mar-2006 (jenjo02)
**	    lxb_wait_reason defines moved to lg.h.
**	21-Jun-2006 (jonj)
**	    Add lxb_lock_id, lxb_txn_state, LXB_ONEPHASE 
**	    for XA integration, deprecated LG_xa_abort().
**	28-Aug-2006 (jonj)
**	    Add lxb_lfd_next, queue of LFDs, normally empty,
**	    used only by XA_STARTed Global Transaction LXBs.
**	05-Sep-2006 (jonj)
**	    Add LXB_ILOG to permit internal logging in an
**	    otherwise READONLY transaction.
**	11-Sep-2006 (jonj)
**	    Add lxb_handle_wts, lxb_handle_ets for LOGFULL_COMMIT
**	    protocols.
**	    Deleted obsolete LXB_SAVEABORT to free up a bit.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lxb_active_lxbq
*/
struct _LXB
{
    SIZE_TYPE	    lxb_next;          /* Next youngest transaction. */
    SIZE_TYPE	    lxb_prev;		/* Next oldest transaction. */
    i4		    lxb_type;		/* Type of control block. */
#define			LXB_TYPE	    5
    LG_ID	    lxb_id;		/* Control block identifier. */
    CS_SEMAPHORE    lxb_mutex;		/* Mutex to protect LXB */
    u_i4	    lxb_status;		/* Status - See above. */
#define			LXB_INACTIVE		0x00000001
#define			LXB_ACTIVE		0x00000002
#define			LXB_CKPDB_PEND		0x00000004
#define			LXB_PROTECT		0x00000008
#define			LXB_JOURNAL		0x00000010
#define			LXB_READONLY		0x00000020
#define			LXB_NOABORT		0x00000040
#define			LXB_RECOVER		0x00000080
#define			LXB_FORCE_ABORT		0x00000100
#define			LXB_SESSION_ABORT	0x00000200
#define			LXB_SERVER_ABORT	0x00000400
#define			LXB_PASS_ABORT		0x00000800
#define			LXB_ABORT	    (LXB_RECOVER | LXB_FORCE_ABORT | LXB_SESSION_ABORT | LXB_SERVER_ABORT | LXB_PASS_ABORT)
#define			LXB_DBINCONST		0x00001000
#define			LXB_WAIT		0x00002000
#define			LXB_DISTRIBUTED		0x00004000
#define			LXB_WILLING_COMMIT	0x00008000
#define			LXB_REASSOC_WAIT	0x00010000
#define			LXB_RESUME		0x00020000
#define			LXB_MAN_ABORT		0x00040000
#define			LXB_MAN_COMMIT		0x00080000
#define			LXB_LOGWRITER_THREAD	0x00100000
#define			LXB_NORESERVE		0x00400000
#define			LXB_ORPHAN		0x00800000
#define			LXB_WAIT_LBB		0x01000000
#define			LXB_SHARED		0x02000000
			    /* This LXB is shared */
#define			LXB_SHARED_HANDLE	0x04000000
			    /* Handle LXB to SHARED LXB */
#define			LXB_ET			0x08000000
			    /* ET written for this TXN */
#define			LXB_PREPARED		0x10000000
			    /* Prepared to commit */
#define			LXB_IN_MINI		0x20000000
			    /* Shared LXB is in mini-transaction */
#define			LXB_ONEPHASE		0x40000000
			    /* XA one-phase commit optimization */
#define			LXB_ILOG		0x80000000
			    /* Readonly txn doing internal logging */
    LXBQ	    lxb_owner;		/* Owner LPD queue of LXB's. */
    LXBQ	    lxb_wait;		/* Write wait queue.. */
    SIZE_TYPE	    lxb_wait_lwq;       /* Head of wait queue (LWQ) or,
					** if waiting on buffer I/O (LXB_WAIT_LBB)
					** this will instead be that buffer.
					*/
    i4	    	    lxb_wait_reason;	/* reason why transaction is waiting
					** as defined in lg.h LG_WAIT_xxx
					*/
    char	    *lxb_wait_reason_string; /* the literal reason why we're waiting */
    SIZE_TYPE	    lxb_lpd;		/* Database for this transaction. */
    i4	    	    lxb_sequence;	/* Sequence number. */
    i4	    	    lxb_in_mini;	/* >0 if in mini-transaction */
    i4	    	    lxb_event_mask;	/* Which events are being waited on */
    i4	    	    lxb_events;		/* Mask of current events */
    LG_LA	    lxb_first_lga;	/* Address of begin record. */
    LG_LA	    lxb_last_lga;	/* Last log record. */
    LG_LSN	    lxb_first_lsn;	/* First log record. */
    LG_LSN	    lxb_last_lsn;	/* Last log record. */
    LG_LSN	    lxb_wait_lsn;	/* LSN which must be forced */
    LG_LA	    lxb_cp_lga;		/* CP interval for this tran. */
    LXH		    lxb_lxh;		/* Active transaction hash entry. */
    DB_DIS_TRAN_ID  lxb_dis_tran_id;	/* Distributed transaction identifier */
    SIZE_TYPE	    lxb_assigned_buffer;/* Buffer which this thread has been
					** assigned
					*/
    i4		    lxb_assigned_task;	/* what to do with this buffer: */
#define			LXB_WRITE_PRIMARY   1
#define			LXB_WRITE_DUAL	    2
    CS_CPID	    lxb_cpid;		/* The session id owning this transaction */
    i4	    	    lxb_rlbb_count;     /* count of LBBs reserved for 
					** spanned write */
    i4		    lxb_lock_id;	/* Txn lock list id associated with log context */
    i4		    lxb_txn_state;	/* DMF XA txn state at time of xa_end */
    i4		    lxb_l_user_name;	/* Length of the user name. */
    u_i8    	    lxb_reserved_space;	/* Logfile space reserved by xact */
    SIZE_TYPE	    lxb_lfb_offset;	/* logfile for this transaction */
    char	    lxb_user_name[DB_OWN_MAXNAME]; /* User name of the transaction. */

    /* The next 6 fields are used when status & LXB_SHARED */
    LXBQ	    lxb_slxb;		/* LGD queue of SHARED LXBs */
    i4		    lxb_handle_count;	/* number of HANDLE LXBs */
    i4		    lxb_handle_preps;	/* count of those PREPAREd */
    i4		    lxb_handle_wts;	/* count of those writing */
    i4		    lxb_handle_ets;	/* count of those writing ETs */
    LXBQ	    lxb_handles;	/* queue of HANDLE LXBs */

    /* The next field is used when status & LXB_SHARED_HANDLE */
    SIZE_TYPE	    lxb_shared_lxb;	/* pointer to shared LXB */

    LXBQ	    lxb_lgd_abort;	/* LGD queue of LXBs to be aborted */
    SIZE_TYPE	    lxb_lfd_next;	/* Queue of XA post-commit deletes */

    LXBQ	    lxb_active_lxbq;	/* List of LXBs active in LDB */

    struct
    {
      i4	    split;		/* Log splits by transaction. */
      i4	    write;		/* Log writes by transaction. */
      i4	    force;		/* Log force by transaction. */
      i4	    wait;		/* Log waits. */
    }		    lxb_stat;
};

/*}
** Name: LFD - Log File Delete post-commit container
**
** Description:
**	Describes a file to be deleted post-commit.
**	Used only by XA transactions to accrete file deletions
**	which cannot otherwise be executed before the final commit
**	of the global XA transaction.
**
**	These are enqueued FIFO on the Global Transaction's LXB,
**	put there by LGalter(LG_A_FILE_DELETE).
**
** History:
**	25-Aug-2006 (jonj)
**	    Invented.
*/
struct _LFD
{
    SIZE_TYPE	    lfd_next;           /* Next LFD by LXB. */
    i4		    lfd_type;		/* Type of control block. */
#define			LFD_TYPE	    7
    LG_FDEL	    lfd_fdel;		/* The path+file to be deleted */
};

/*
** Functions for use by the LG code. Declared at the end since they depend on
** the type definitions declared earlier in this header file.
*/
/* in LGALTER.C */
FUNC_EXTERN	STATUS	LG_allocate_buffers(
			    LGD *lgd,
			    LFB *lfb,
			    i4 logpg_size,
			    i4 num_buffers);
/* In LGCLUSTR.C: */
FUNC_EXTERN	STATUS	LGc_csp_enqueue(void);
FUNC_EXTERN	STATUS	LGc_rcp_enqueue(bool is_rcp);

/* In LGOPEN.C: */
FUNC_EXTERN	STATUS	LG_build_file_name( i4  which_log, char *nodename,
					    char file_name[][128],
					    i4  *l_file_name, 
					    char path_name[][128],
					    i4  *l_path_name,
					    i4  *num_files);

/* In LGMISC.C: */
FUNC_EXTERN	VOID	LG_archive_complete(
			    LFB *lfb,
			    LG_LA *lga,
			    LG_LA *purge_lga);
FUNC_EXTERN	bool	LG_check_cp_servers(i4 value);
FUNC_EXTERN	VOID	LG_last_cp_server(void);
FUNC_EXTERN	VOID	LG_signal_check(void);
FUNC_EXTERN	VOID	LG_abort_oldest(LXB *lxb);
FUNC_EXTERN	VOID	LG_abort_transaction(LXB *lxb);
FUNC_EXTERN	VOID	LG_compute_bof(
			    LFB *lfb,
			    i4 archive_interval);
FUNC_EXTERN	STATUS	LG_check_dead(CL_ERR_DESC *sys_err);
FUNC_EXTERN	VOID	LG_lpb_shrbuf_disconnect(i4 bm_id);
FUNC_EXTERN	VOID	LG_lpb_shrbuf_nuke(i4 bm_id, PID *nuke_me);
FUNC_EXTERN	VOID	LG_lpb_node_nuke( PID *nuke_me);
FUNC_EXTERN	VOID	LG_disable_dual_logging(i4 active_log, LFB *lfb);
FUNC_EXTERN	STATUS	LG_check_logfull(LFB *lfb, LXB *lxb);
FUNC_EXTERN	VOID	LG_cleanup_checkpoint(LPB *lpb);
FUNC_EXTERN	STATUS	LG_force_abort(CL_ERR_DESC *sys_err);
FUNC_EXTERN	i4	LG_used_logspace(LFB *lfb, bool include_reserved);
FUNC_EXTERN	STATUS	LG_orphan_xact(LXB *lxb);
FUNC_EXTERN	STATUS	LG_adopt_xact(LXB *lxb, LPD *lpd);

/* In LGmutex.c */
FUNC_EXTERN	STATUS	LG_imutex(
			    CS_SEMAPHORE *mutex,
			    char	 *mutex_name);
FUNC_EXTERN	STATUS	LG_mutex_fcn(
			    i4		  mode,
			    CS_SEMAPHORE *mutex,
			    char	 *file,
			    i4		  line);
FUNC_EXTERN	STATUS	LG_unmutex_fcn(
			    CS_SEMAPHORE *mutex,
			    char	 *file,
			    i4		  line);
FUNC_EXTERN void	LGintrack(void);
FUNC_EXTERN void	LGintrcvd(void);

/* In LGREMOVE.C: */
FUNC_EXTERN	STATUS	LG_remove(LG_ID *db_id);

/* In LGRESERVE.C: */
FUNC_EXTERN	i4 LG_calc_reserved_space(
				LXB		*lxb,
				i4		num_records,
				i4		log_bytes,
				i4		flag);


/* In LGCBMEM.C: */
FUNC_EXTERN	PTR	LG_allocate_cb( i4  cb_type );
FUNC_EXTERN	VOID	LG_deallocate_cb( i4  cb_type, PTR cb );
FUNC_EXTERN	STATUS	LG_allocate_lbb( LFB *lfb,
					 i4 *reserved_lbb_count,	 
					 LXB *lxb,
					 LBB **lbbo);
FUNC_EXTERN	LXB *	LG_allocate_lxb( i4  flags,
					 LFB *lfb,
					 LPD *lpd,
					 DB_DIS_TRAN_ID *dis_tran_id,
					 LXB **shared_lxb,
					 STATUS *status);
FUNC_EXTERN	VOID	LG_assign_tran_id(LFB *lfb,
					 DB_TRAN_ID *tran_id,
					 bool mutex_held);
				
FUNC_EXTERN	VOID	LG_dealloc_lfb_buffers( LFB *lfb );

FUNC_EXTERN	VOID	LG_end_tran( LXB *lxb, i4 flag);

FUNC_EXTERN	STATUS	LG_write_headers(
			    LG_ID *lx_id,
			    LFB *lfb,
			    STATUS *async_status);
FUNC_EXTERN	VOID	LG_inline_hdr_write(LBB *lbb);

FUNC_EXTERN	VOID	LG_signal_event(i4 set_event,
					i4 unset_event,
					bool    lgd_status_is_locked);
FUNC_EXTERN	VOID	LG_resume(LWQ *lwq, LFB *lfb);
FUNC_EXTERN	VOID	LG_suspend(LXB *lxb, LWQ *lwq,
				    STATUS *indirect_stat_addr);
FUNC_EXTERN	VOID	LG_resume_lbb(LXB *lxb, LBB *lbb, i4 status);
FUNC_EXTERN	VOID	LG_suspend_lbb(LXB *lxb, LBB *lbb,
				    STATUS *indirect_stat_addr);

FUNC_EXTERN	STATUS	LG_meminit(CL_ERR_DESC *sys_err);
FUNC_EXTERN	VOID	LG_rundown(i4 pid);

FUNC_EXTERN	u_i4   LGchk_sum(PTR buffer, i4 size);

FUNC_EXTERN	STATUS	LG_queue_write(LBB *lbb, LPB *lpb, LXB *lxb,
					i4	flags);
#define		KEEP_LBB_MUTEX	0x0001

FUNC_EXTERN	STATUS	LG_write_block(LG_LXID xact_id,
				    CL_ERR_DESC *sys_err);
FUNC_EXTERN	STATUS  LG_do_write(LXB *lxb,
				CL_ERR_DESC *sys_err);
FUNC_EXTERN	VOID	LG_dump_logwriters(LGD *lgd);

FUNC_EXTERN	STATUS	LG_write_log_headers(LG_HEADER *header,
				LG_IO *di_io_primary, LG_IO *di_io_dual,
				char *buffer,
				CL_ERR_DESC *sys_err );
FUNC_EXTERN	STATUS	LG_get_log_page_size(
				char *path,
				char *file,
				i4 *page_size,
				CL_ERR_DESC *sys_err);

FUNC_EXTERN	STATUS	LGmo_attach_lg(void);

FUNC_EXTERN     STATUS  LG_pgyback_write( LG_LXID transaction_id, 
					  i4  timer_interval_ms,
					  CL_ERR_DESC *sys_err);


/* debugging function in lgshow.c: */
FUNC_EXTERN VOID	LG_debug_wacky_ldb_found(LGD *lgd, LDB *ldb);

/*
** Routines from LGLSN.C:
*/
FUNC_EXTERN	STATUS	LGlsn_init(CL_ERR_DESC *sys_err);
FUNC_EXTERN	STATUS	LGlsn_term(CL_ERR_DESC *sys_err);
FUNC_EXTERN	STATUS	LGlsn_next(LG_LSN   *lsn, CL_ERR_DESC *sys_err);

/*
** Routines from LGSIZE.C:
*/
FUNC_EXTERN	VOID	LG_size(i4 	    object,
				SIZE_TYPE   *size);

/*
** Routines from LGINIT.C
*/
FUNC_EXTERN	STATUS  LG_rcvry_cmpl(PID pid, i2 lg_id_id);

/*
** Routines from LGWRITE.C
*/
FUNC_EXTERN	VOID	assign_logwriter(LGD *lgd, LBB *lbb, LPB *lpb);

/*
** Name: LG internal error messages
**
** Description:
**
** These messages are defined in ERDMF.MSG and are used by the LG code for
** logging internal errors to the error log.
*/
#define	    E_DMA400_LG_BAD_BCNT_ALTER		    (E_DM_MASK | 0xA400L)
#define	    E_DMA401_LG_BAD_HDRSIZE		    (E_DM_MASK | 0xA401L)
#define	    E_DMA402_LG_WRITE_BADPARM		    (E_DM_MASK | 0xA402L)
#define	    E_DMA403_LG_WRITE_BAD_ID		    (E_DM_MASK | 0xA403L)
#define	    E_DMA404_LG_WRITE_BAD_XACT		    (E_DM_MASK | 0xA404L)
#define	    E_DMA405_LG_WRITE_BAD_RLEN		    (E_DM_MASK | 0xA405L)
#define	    E_DMA406_LG_WRITE_BAD_STAT		    (E_DM_MASK | 0xA406L)
#define	    E_DMA407_LG_WRITE_NOT_FIRST		    (E_DM_MASK | 0xA407L)
#define	    E_DMA408_LGPOSN_BAD_ID		    (E_DM_MASK | 0xA408L)
#define	    E_DMA409_LGPOSN_BAD_XACT		    (E_DM_MASK | 0xA409L)
#define	    E_DMA40A_LGPOSN_BAD_PARM		    (E_DM_MASK | 0xA40AL)
#define	    E_DMA40B_LGPOSN_BAD_LGA		    (E_DM_MASK | 0xA40BL)
#define	    E_DMA40C_LGPOSN_NULL_LGA		    (E_DM_MASK | 0xA40CL)
#define	    E_DMA40D_LGCLOSE_BAD_ID		    (E_DM_MASK | 0xA40DL)
#define	    E_DMA40E_LGCLOSE_BAD_PROC		    (E_DM_MASK | 0xA40EL)
#define	    E_DMA40F_LGADD_BAD_ID		    (E_DM_MASK | 0xA40FL)
#define	    E_DMA410_LGADD_BAD_PROC		    (E_DM_MASK | 0xA410L)
#define	    E_DMA411_LGADD_BAD_LEN		    (E_DM_MASK | 0xA411L)
#define	    E_DMA412_LGBEGIN_BAD_ID		    (E_DM_MASK | 0xA412L)
#define	    E_DMA413_LGBEGIN_BAD_DB		    (E_DM_MASK | 0xA413L)
#define	    E_DMA414_LGBEGIN_BAD_LEN		    (E_DM_MASK | 0xA414L)
#define	    E_DMA415_LGEND_BAD_ID		    (E_DM_MASK | 0xA415L)
#define	    E_DMA416_LGEND_BAD_XACT		    (E_DM_MASK | 0xA416L)
#define	    E_DMA417_LG_DB_CKPERROR		    (E_DM_MASK | 0xA417L)
#define	    E_DMA418_LGFORCE_BAD_ID		    (E_DM_MASK | 0xA418L)
#define	    E_DMA419_LGFORCE_BAD_XACT		    (E_DM_MASK | 0xA419L)
#define	    E_DMA41A_LGREM_BAD_ID		    (E_DM_MASK | 0xA41AL)
#define	    E_DMA41B_LGREM_BAD_DB		    (E_DM_MASK | 0xA41BL)
#define	    E_DMA41C_LGREM_DB_ACTIVE		    (E_DM_MASK | 0xA41CL)
#define	    E_DMA41D_LGSHOW_STAT_SIZE		    (E_DM_MASK | 0xA41DL)
#define	    E_DMA41E_LGSHOW_HDR_SIZE		    (E_DM_MASK | 0xA41EL)
#define	    E_DMA41F_LGSHOW_LGLA_SIZE		    (E_DM_MASK | 0xA41FL)
#define	    E_DMA420_LGSHOW_STS_SIZE		    (E_DM_MASK | 0xA420L)
#define	    E_DMA421_LGSHOW_BCNT_SIZE		    (E_DM_MASK | 0xA421L)
#define	    E_DMA422_LGSHOW_NODEID_SIZE		    (E_DM_MASK | 0xA422L)
#define	    E_DMA423_LGSHOW_DB_SIZE		    (E_DM_MASK | 0xA423L)
#define	    E_DMA424_LGSHOW_BAD_ID		    (E_DM_MASK | 0xA424L)
#define	    E_DMA425_LGSHOW_BAD_DB		    (E_DM_MASK | 0xA425L)
#define	    E_DMA426_LGSHOW_LGID_SIZE		    (E_DM_MASK | 0xA426L)
#define	    E_DMA427_LGSHOW_XACT_SIZE		    (E_DM_MASK | 0xA427L)
#define	    E_DMA428_LGSHOW_BAD_ID		    (E_DM_MASK | 0xA428L)
#define	    E_DMA429_LGSHOW_PROC_SIZE		    (E_DM_MASK | 0xA429L)
#define	    E_DMA42A_LGSHOW_LDBS_SIZE		    (E_DM_MASK | 0xA42AL)
#define	    E_DMA42B_LGCPOSN_BAD_PARM		    (E_DM_MASK | 0xA42BL)
#define	    E_DMA42C_LGCPOSN_BAD_LGA		    (E_DM_MASK | 0xA42CL)
#define	    E_DMA42D_LGCPOSN_NULL_LGA		    (E_DM_MASK | 0xA42DL)
#define	    E_DMA42E_LG_MUTEX			    (E_DM_MASK | 0xA42EL)
#define	    E_DMA42F_LG_UNMUTEX			    (E_DM_MASK | 0xA42FL)
#define	    E_DMA430_LG_IMUTEX			    (E_DM_MASK | 0xA430L)
#define	    E_DMA431_LG_RMUTEX			    (E_DM_MASK | 0xA431L)
#define	    E_DMA432_LG_BAD_INIT		    (E_DM_MASK | 0xA432L)
#define	    E_DMA433_LG_INIT_MO_ERROR		    (E_DM_MASK | 0xA433L)
#define	    E_DMA434_LGK_VERSION_MISMATCH	    (E_DM_MASK | 0xA434L)
#define	    E_DMA435_WRONG_LGKMEM_VERSION	    (E_DM_MASK | 0xA435L)
#define	    E_DMA436_NOT_ENOUGH_LDBS		    (E_DM_MASK | 0xA436L)
#define	    E_DMA437_NOT_ENOUGH_SBKS		    (E_DM_MASK | 0xA437L)
#define	    E_DMA438_LPB_ALLOC_FAIL		    (E_DM_MASK | 0xA438L)
#define	    E_DMA439_LXB_ALLOC_FAIL		    (E_DM_MASK | 0xA439L)
#define	    E_DMA43A_LPD_ALLOC_FAIL		    (E_DM_MASK | 0xA43AL)
#define	    E_DMA43B_NOT_ENOUGH_LBKS		    (E_DM_MASK | 0xA43BL)
#define	    E_DMA43C_LDB_ALLOC_FAIL		    (E_DM_MASK | 0xA43CL)
#define	    E_DMA43D_SBK_LIMIT_REACHED		    (E_DM_MASK | 0xA43DL)
#define	    E_DMA43E_LBK_LIMIT_REACHED		    (E_DM_MASK | 0xA43EL)
#define	    E_DMA43F_LG_SHMEM_NOMORE		    (E_DM_MASK | 0xA43FL)
#define	    E_DMA440_LGBEGIN_NO_LXBS		    (E_DM_MASK | 0xA440L)
#define	    E_DMA441_LFD_ALLOC_FAIL		    (E_DM_MASK | 0xA441L)
/*
** These error messages have both an LG_* code as well as an E_DMxxx code due
** to the code's history. This is not a tradition which should be continued;
** new messages should have ONLY the E_DMxxxx code.
*/
#define	    E_DMA441_LG_SHUTTING_DOWN		    (E_DM_MASK | 0xA441L)
#define LG_SHUTTING_DOWN			    E_DMA441_LG_SHUTTING_DOWN
#define	    E_DMA442_LG_MUST_SLEEP		    (E_DM_MASK | 0xA442L)
#define	LG_MUST_SLEEP				    E_DMA442_LG_MUST_SLEEP
#define	    E_DMA443_LG_NOMASTER		    (E_DM_MASK | 0xA443L)
#define LG_NOMASTER				    E_DMA443_LG_NOMASTER
#define	    E_DMA444_LG_BADFILENAME		    (E_DM_MASK | 0xA444L)
#define LG_BADFILENAME				    E_DMA444_LG_BADFILENAME
#define	    E_DMA445_LG_BADFCT			    (E_DM_MASK | 0xA445L)
#define LG_BADFCT				    E_DMA445_LG_BADFCT
#define	    E_DMA446_LG_MULTARCH		    (E_DM_MASK | 0xA446L)
#define	LG_MULTARCH				    E_DMA446_LG_MULTARCH
#define	    E_DMA447_LG_NOT_ONLINE		    (E_DM_MASK | 0xA447L)
#define LG_NOT_ONLINE				    E_DMA447_LG_NOT_ONLINE
#define	    E_DMA448_LG_MEM_INUSE		    (E_DM_MASK | 0xA448L)
#define	LG_MEM_INUSE				    E_DMA448_LG_MEM_INUSE
#define	    E_DMA449_LG_CANCELGRANT		    (E_DM_MASK | 0xA449L)
#define LG_CANCELGRANT				    E_DMA449_LG_CANCELGRANT
#define	    E_DMA44A_LG_SYNCH			    (E_DM_MASK | 0xA44AL)
#define LG_SYNCH				    E_DMA44A_LG_SYNCH
#define	    E_DMA44B_LG_MULT_MASTER		    (E_DM_MASK | 0xA44BL)

#define	    E_DMA44C_LG_WBLOCK_BAD_ID		    (E_DM_MASK | 0xA44CL)
#define	    E_DMA44D_LG_WBLOCK_BAD_XACT		    (E_DM_MASK | 0xA44DL)
#define	    E_DMA44E_LG_WBLOCK_BAD_WRITE	    (E_DM_MASK | 0xA44EL)
#define	    E_DMA44F_LG_WB_BLOCK_INFO		    (E_DM_MASK | 0xA44FL)
#define	    E_DMA450_LGEVENT_SYNC_ERROR		    (E_DM_MASK | 0xA450L)
#define	    E_DMA451_LG_EVENT_BAD_ID		    (E_DM_MASK | 0xA451L)
#define	    E_DMA452_LG_EVENT_BAD_XACT		    (E_DM_MASK | 0xA452L)
#define	    E_DMA453_LG_CANCEL_BAD_ID		    (E_DM_MASK | 0xA453L)
#define	    E_DMA454_LG_CANCEL_BAD_XACT		    (E_DM_MASK | 0xA454L)
#define	    E_DMA455_LGREAD_HDR_READ		    (E_DM_MASK | 0xA455L)
#define	    E_DMA456_LGREAD_SYNC_ERROR		    (E_DM_MASK | 0xA456L)
#define	    E_DMA457_LGREAD_IO_ERROR		    (E_DM_MASK | 0xA457L)
#define	    E_DMA458_LGREAD_CKSUM_ERROR		    (E_DM_MASK | 0xA458L)
#define	    E_DMA459_LGREAD_WRONG_PAGE		    (E_DM_MASK | 0xA459L)
#define	    E_DMA45A_LGREAD_BADFORMAT		    (E_DM_MASK | 0xA45AL)
#define	    E_DMA45B_LG_READ_BAD_ID		    (E_DM_MASK | 0xA45BL)
#define	    E_DMA45C_LG_READ_BAD_XACT		    (E_DM_MASK | 0xA45CL)
#define	    E_DMA45D_LGREAD_DISABLE_PRIM	    (E_DM_MASK | 0xA45DL)
#define	    E_DMA45E_LGREAD_DISABLE_DUAL	    (E_DM_MASK | 0xA45EL)
#define	    E_DMA45F_LGREAD_LA_RANGE		    (E_DM_MASK | 0xA45FL)
#define	    E_DMA460_LGREAD_EOF_REACHED		    (E_DM_MASK | 0xA460L)
#define	    E_DMA461_LGREAD_INMEM_EOF		    (E_DM_MASK | 0xA461L)
#define	    E_DMA462_LGREAD_BAD_CXT		    (E_DM_MASK | 0xA462L)
#define	    E_DMA463_LGREAD_MAP_PAGE		    (E_DM_MASK | 0xA463L)
#define	    E_DMA464_LGREAD_BADFORMAT		    (E_DM_MASK | 0xA464L)
#define	    E_DMA465_LGREAD_BADFORMAT		    (E_DM_MASK | 0xA465L)
#define	    E_DMA466_LGFABRT_SYNC_ERROR		    (E_DM_MASK | 0xA466L)
#define	    E_DMA467_DISABLE_LOGGING		    (E_DM_MASK | 0xA467L)
#define	    E_DMA468_LGCDEAD_SYNC_ERROR		    (E_DM_MASK | 0xA468L)
#define	    E_DMA469_PROCESS_HAS_DIED		    (E_DM_MASK | 0xA469L)
#define	    E_DMA46A_LGALTER_SYNC_ERROR		    (E_DM_MASK | 0xA46AL)
#define	    E_DMA46B_LGALTER_ALLOC_LBB		    (E_DM_MASK | 0xA46BL)
#define	    E_DMA46C_LGALTER_SBKS_INUSE		    (E_DM_MASK | 0xA46CL)
#define	    E_DMA46D_LGALTER_SBKTAB_INUSE	    (E_DM_MASK | 0xA46DL)
#define	    E_DMA46E_LGALTER_SBKTAB_NOMEM	    (E_DM_MASK | 0xA46EL)
#define	    E_DMA46F_LGALTER_LBKS_INUSE		    (E_DM_MASK | 0xA46FL)
#define	    E_DMA470_LGALTER_LBKTAB_INUSE	    (E_DM_MASK | 0xA470L)
#define	    E_DMA471_LGALTER_LBKTAB_NOMEM	    (E_DM_MASK | 0xA471L)
#define	    E_DMA472_LGALTER_BADPARAM		    (E_DM_MASK | 0xA472L)
#define	    E_DMA473_LGALTER_BADPARAM		    (E_DM_MASK | 0xA473L)
#define	    E_DMA474_LGALTER_BADPARAM		    (E_DM_MASK | 0xA474L)
#define	    E_DMA475_LGALTER_BADPARAM		    (E_DM_MASK | 0xA475L)
#define	    E_DMA476_LGALTER_DBSTATE		    (E_DM_MASK | 0xA476L)
#define	    E_DMA477_LGOPEN_LOGFILE_NAME	    (E_DM_MASK | 0xA477L)
#define	    E_DMA478_LGOPEN_LOGFILE_PATH	    (E_DM_MASK | 0xA478L)
#define	    E_DMA479_LGOPEN_PATH_INFO		    (E_DM_MASK | 0xA479L)
#define	    E_DMA47A_LGOPEN_PATH_ERR		    (E_DM_MASK | 0xA47AL)
#define	    E_DMA47B_LGOPEN_DUALLOG_NAME	    (E_DM_MASK | 0xA47BL)
#define	    E_DMA47C_LGOPEN_DUALLOG_PATH	    (E_DM_MASK | 0xA47CL)
#define	    E_DMA47D_LGOPEN_DPATH_INFO		    (E_DM_MASK | 0xA47DL)
#define	    E_DMA47E_LGOPEN_DPATH_ERR		    (E_DM_MASK | 0xA47EL)
#define	    E_DMA47F_LGOPEN_SIZE_MISMATCH	    (E_DM_MASK | 0xA47FL)
#define	    E_DMA480_LGWRITE_FILE_FULL	    	    (E_DM_MASK | 0xA480L)
#define	    E_DMA481_WRITE_PAST_BOF		    (E_DM_MASK | 0xA481L)
#define	    E_DMA482_LG_LSN_INIT		    (E_DM_MASK | 0xA482L)
#define	    E_DMA483_LG_LSN_TERM		    (E_DM_MASK | 0xA483L)
#define	    E_DMA484_LG_LSN_NEXT_FAIL		    (E_DM_MASK | 0xA484L)
#define	    E_DMA485_LG_LSN_ENQ_FAIL		    (E_DM_MASK | 0xA485L)
#define	    E_DMA486_LG_LSN_LKSB_FAIL		    (E_DM_MASK | 0xA486L)
#define	    E_DMA487_LG_LSN_SYNC_FAIL		    (E_DM_MASK | 0xA487L)
#define	    E_DMA488_LG_LSN_INIT_INFO		    (E_DM_MASK | 0xA488L)
#define	    E_DMA489_LG_LSN_DEQ_FAIL		    (E_DM_MASK | 0xA489L)
#define	    E_DMA48A_LGERASE_BAD_PROC		    (E_DM_MASK | 0xA48AL)
#define	    E_DMA48B_LGERASE_BAD_ID		    (E_DM_MASK | 0xA48BL)
#define	    E_DMA48C_LG_BAD_CHECKSUM		    (E_DM_MASK | 0xA48CL)
#define	    E_DMA48D_ADOPT_NONORPHAN		    (E_DM_MASK | 0xA48DL)
#define	    E_DMA48E_LOGFULL_EXCEEDED		    (E_DM_MASK | 0xA48EL)
#define	    E_DMA48F_LOGFILE_FULL		    (E_DM_MASK | 0xA48FL)
#define	    E_DMA490_LXB_RESERVED_SPACE		    (E_DM_MASK | 0xA490L)
#define	    E_DMA491_LFB_RESERVED_SPACE		    (E_DM_MASK | 0xA491L)
#define	    E_DMA492_LGOPEN_BLKSIZE_MISMATCH	    (E_DM_MASK | 0xA492L)
#define	    E_DMA493_LG_NO_SHR_TXN	    	    (E_DM_MASK | 0xA493L)
#define	    E_DMA494_LG_INVALID_SHR_TXN	    	    (E_DM_MASK | 0xA494L)
#define	    E_DMA495_LG_BAD_SHR_TXN_STATE    	    (E_DM_MASK | 0xA495L)
#define	    E_DMA496_LGCONNECT_BAD_ID    	    (E_DM_MASK | 0xA496L)
#define	    E_DMA497_LGCONNECT_BAD_ID    	    (E_DM_MASK | 0xA497L)
#define	    E_DMA498_LGCONNECT_NO_LXBS    	    (E_DM_MASK | 0xA498L)
#define	    E_DMA499_DEAD_PROCESS_INFO	    	    (E_DM_MASK | 0xA499L)
#define	    E_DMA810_LOG_PARTITION_MISMATCH	    (E_DM_MASK | 0xA810L)
