/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
**
*/

/**
** Name: DM0PBMCB.H - Buffer manager internal definitions.
**
** Description:
**      This file defines types and constants that are internal to the buffer
**	buffer.  These constants are located here so that debugging code can
**	get access to the structures.
**
** History:
**      06-dec-1985 (derek)
**          Created new for Jupiter.
**      04-apr-87 (ac)
**	    Added definition of read/nolock support.
**	30-sep-88 (rogerk)
**	    Added new fields to the buffer manager control block for
**	    write behind support.
**	30-jan-89 (rogerk)
**	    Added support for Shared Memory Buffer Manager.
**	    Changed pointers stored in buffer manager to be index values.
**	    Broke BMCB into two control blocks, BMCB and LBMCB.
**	    Added DM0P_BMCACHE definition.
**	    Added new DM0P_BUFFER, DM0P_GROUP and DM0P_BMCB fields.
**	10-apr-89 (rogerk)
**	    Added bm_name field to bmcb to hold cache name.
**	10-jul-89 (rogerk)
**	    Added BUF_EXTENDPAGE buffer status to fix multi-server extend bug.
**	25-sep-1989 (rogerk)
**	    Added BM_PASS_ABORT to bm_status.
**	14-aug-90 (rogerk)
**	    Added buf_tcb_stamp field to DM0P_BUFFER struct.
**	29-apr-1991 (Derek)
**	    Added new buf_sts constants to DM0P_BUFFER for allocation project.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    4-feb-1991 (rogerk)
**		Added support for fixed buffer priorities.
**	 3-nov-1991 (rogerk)
**	    Added fixes for File Extend recovery.
**	    Added buffer status PAUNFIX which allows us to tell whether the
**	    WUNFIX condition of a buffer is because of the type of buffer or
**	    because a pass-abort operation is in progress.  This tells us
**	    whether we can reset the WUNFIX status after the pass-abort is
**	    complete.
**      6-jan-1992 (jnash)
**          Remove BM_PAUNFIX, add BM_TUNFIX in modifying File Extend and
**          Btree index page pass abort recovery.
**	07-jul-1992 (mikem)
**          Prototyping DMF.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added buffer manager fields for the
**	    transaction context used to force the log file.
**	    Added BUF_WCOMMIT flag and removed old BUF_FORCE_COMMIT.
**	24-feb-1993 (keving)
**	    => Added changes to allow a buffer manager to work with 
**	    => multiple logs
**	18-oct-1993 (rmuth)
**	    Collect BM statistics within the DMP_BMCB. This is usefull
**	    in a shared cache environment in non-shared cache they will
**	    be the same as DMP_LBMCB.
**	22-feb-1994 (andys)
**	    Integrate change from ingres63p:
**       	3-may-1993 (andys)
**          	    Add BUF_MUTEX_OK as part of fix for bug 46951.
**	25-mar-96 (nick)
**	    Added BUF_FCWAIT and appropriate statistic for its wait event.
**      06-mar-1996 (stial01)
**          Changes to support multiple page sizes.
**          Split DMP_BMCB into DM0P_BMCB, DM0P_BM_COMMON
**          Changed DMP_LBMCB, made some fields arrays.
**          Added DMP_BMSCB, DM0P_BSTAT, DM0P_BID. 
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Distributed Multi Cache Management (DMCM) protocol.
**          New buffer statuses of BUF_DMCM and BUF_NULL_TUNFIX. New buffer
**          structure entries buf_lock_mode and buf_pid. New buffer manager
**          structure entry bm_dmcm. New local buffer manager field
**          lbm_dmcm_lock_list.
**          Added ANB's asyncio amendments.
**          Incorporate ITB's gather-write changes.
**          Added NO_ACTION to the list of async I/O actions and a
**          buffer flag to indicate a gather write has been queued.
**	    Added WRITE_ALONG flag so that dm0p_write_along can
**	    do gather writes.
**	30-Aug-1996 (jenjo02)
**	    Mutex granularity project. Added a host of new mutexes
**	    to replace singular bmcb_mutex.
**          Defined new structure to contain BM semaphores, DM0P_SEMAPHORE.
**	    New "head-of-list" structures defined, DM0P_BID_HEAD, DM0P_STATE_HEAD,
**	    DM0P_HASH_HEAD.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Added BUF_X_LATCH.
**	05-Feb-1997 (jenjo02)
**	    Added G_WAIT to group status flags..
**	24-Feb-1997 (jenjo02)
**	    Removed bm_eq from BMCB. It served no purpose.
**	14-Mar-1997 (jenjo02)
**	    Added G_RBUSY, G_BUSY to group status flags.
**	    Reinstated lbm_lxid_mutex.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Replace BMCB_MAX_PRI with widely known DB_MAX_TABLEPRI.
**	    Removed bmcb_tblpsize, bmcb_tblpriority, lbm_tblpriority.
**	    Added new stats for fixed priority buffers.
**	    Changed all stat fields to u_i4.
**	    Removed bmcb_dmcm, made it a flag in bmcb_status.
**	    Removed BUF_MUSTLOCK status bit, buf_pid.
**	01-May-1997 (jenjo02)
**	    Added new stats for group waits.
**      21-May-1997 (stial01)
**          Renamed BUF_X_LATCH -> BUF_IX_LOCK. This is set when the buffer
**          has been fixed for write with an IX lock.
**          Added buf_lock_list to buffer, added BUF_MLOCK status.
**      29-May-1997 (stial01)
**          Change buf_sts to u_i4, Rename BUF_IX_LOCK -> BUF_INTENT_LK.
**	21-Nov-1997 (jenjo02)
**	    Removed single (bm_fq) and group (bm_gfq) buffer fixed state queues.
**	19-Jan-1998 (jenjo02)
**	    Renamed DM0P_TRQ to DM0P_BLQ to make it more generic. Added
**	    bmcb_tabq_buckets (table hash queues) in which BUF_VALID pages
**	    of a table are remembered.
**	20-May-1998 (jenjo02)
**	    Dynamic, cache-specific WriteBehind thread alterations.
**	01-Sep-1998 (jenjo02)
**	    Moved mutex wait stats out bmcb/lbm in to DM0P_BSTAT where they
**	    can be tracked by cache.
**	27-Apr-1999 (jenjo02)
**	  o Replaced i4 buf_index with DM0P_BID buf_id to ease cross-cache 
**	    identification. buf_id contains both the cache and buffer indexes
**	    to completely identify a buffer.
**	  o Added buf_cp_next to link CP candidate buffers across caches.
**	  o Added buf_page_offset so it doesn't have to be constantly
**	    computed.
**	  o Added bmcb_cp_next, bmcb_cpbufs to link CP candidate buffers 
**	    across caches.
**	29-Jun-1999 (jenjo02)
**	    Changes necessary to implement GatherWrite.
**	12-Jul-1999 (jenjo02)
**	    Replaced CP queue with array of buffer BIDs to pre-sort
**	    CP buffers.
**	24-Aug-1999 (jenjo02)
**	    Added 2 new buf_type values, BMCB_FHDR, BMCB_FMAP, previously 
**	    indistinguishable from BMCB_ROOT.
**	    Added cache stats by buf_type to DM0P_BSTAT. Added
**	    2 new stats, bs_reclaim (number of reclaims of BUF_FREE buffers) and
**	    bs_replace (reclaim of BUF_VALID free or modified buffer). Fixed-priority
**	    stats removed. Added sth_count to DM0P_STATE_HEAD, obviating the need
**	    for bm_gfcount and bm_gmcount.
**	10-Jan-2000 (jenjo02)
**	    Implemented group priority queues. Changed some buffer/group/bm
**	    variables to volatile to keep unmutexed list scans from being
**	    over-optimized by compilers.
**	    Inlined CSp|v_semaphore function calls within dm0p_mlock|munlock
**	    macros. dm0p_mlock|munlock_fcn() is called only if the inlined
**	    semaphore request fails (to trap the error).
**	    Got rid of static bm_fcount, lcount, mcount, state queue counts
**	    and contentious bm_count_mutex. New macros DM0P_COUNT_? will
**	    tally the counts when needed.
**	20-Apr-2000 (jenjo02)
**	    Moved CACHENAME_SUFFIX, CACHENM_MAX defines here from dm0p.c for
**	    exposure in cacheutil.c. Single cross-cache CP array changed to
**	    per-cache arrays.
**	23-May-2000 (jenjo02)
**	    Added temporary bms_line to semaphore structure, modified
**	    semaphore macros to store line number of last p|v_sem to
**	    help track any elusive mutex errors.
**	22-Sep-2000 (jenjo02)
**	    Added g_lock_list to identify who set BUSY status.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-Jun-2002 (horda03) Bug 108074
**          Prevent FORCE_ABORT transaction filling TX log
**          file by not flushing SCONCUR pages unless we
**          really need to. Added BUF_WUNFIX_REQ.
**      20-aug-2002 (devjo01)
**          Added BMCL_DCMC_CBACK.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      23-Oct-2002 (hanch04)
**          Changed page_offset to SIZE_TYPE to support
**          buffer manager cache > 2 gig.
**	31-Jan-2003 (jenjo02)
**	    Changes for Cache Flush Agent concepts.
**	29-Mar-2004 (hanje04)
**	    Redefine BMCB_CLUSTERED to be 0x0100 to stop it clashing with
**	    BMCB_CFA_TRACE.
**	01-apr-2004 (devjo01)
**	    Add BMCB_USE_CLM.  Flag indicate a fast Cluster Long Message
**	    facility is available.
**	09-Apr-2004 (hanje04)
**	    Removed >> integration tags.
**	3-May-2005 (schka24)
**	    Set mutex hint after taking the actual mutex, not before.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	28-nov-2005 (devjo01) b115583
**	    Add buf_pid, lbm_pid for use in VMS clusters.
**	04-Apr-2008 (jonj)
**	    Add BMCB_PAGE_TYPES define.
**	15-Mar-2010 (smeke01) b119529
**	    Added prototypes for new functions dmd_summary_statistics().
**/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _DM0P_BSTAT      DM0P_BSTAT;
typedef struct _DM0P_STATE	DM0P_STATE;
typedef struct _DM0P_STATE_HEAD	DM0P_STATE_HEAD;
typedef struct _DM0P_BLQ	DM0P_BLQ;
typedef struct _DM0P_BID        DM0P_BID;
typedef struct _DM0P_BLQ_HEAD   DM0P_BLQ_HEAD;
typedef struct _DM0P_BUFFER	DM0P_BUFFER;
typedef struct _DM0P_GROUP	DM0P_GROUP;
typedef struct _DM0P_BMCACHE	DM0P_BMCACHE;
typedef struct _DM0P_BMCB       DM0P_BMCB;
typedef struct _DM0P_HASH_HEAD  DM0P_HASH_HEAD;
typedef struct _DM0P_SEMAPHORE  DM0P_SEMAPHORE;
typedef struct _DM0P_CLOSURE	DM0P_CLOSURE;
typedef struct _DM0P_CFA	DM0P_CFA;

/*
**  This constant is used to define the end of a buffer state queue.
**  Its value must be a non-legal buffer index value.
*/

#define			BUF_ENDLIST	-1

/*
**  This value is used to define a bucket value for buffers which do not
**  belong to any hash bucket.
*/

#define			BUF_NOBUCKET	-1

/*
** Define the number and types of pages. These values are used to
** maintain buffer priorities and for statistics gathering.
*/
#define			BMCB_PTYPES     	6
#define			BMCB_FHDR	0
#define			BMCB_FMAP	1
#define			BMCB_ROOT	2
#define			BMCB_INDEX      3
#define			BMCB_LEAF	4
#define			BMCB_DATA	5
/* String values of above bits: */
#define	BMCB_PAGE_TYPES "\
FHDR,FMAP,ROOT,INDEX,LEAF,DATA"

/* Macros to count the elements in a queue of type DM0P_STATE_HEAD */

/* Count the elements in a single queue */
#define		DM0P_COUNT_QUEUE(q, c) \
	{ \
	    i4	i; \
	    for ( i = c = 0; \
		  i < DB_MAX_TABLEPRI; \
		  c += q[i].sth_count, i++ ); \
	}

/* Count the total number of free, modified, and fixed buffers */
#define		DM0P_COUNT_ALL_QUEUES(free,mod,fix) \
	{ \
	    register i4			i; \
	    register DM0P_STATE_HEAD	*f = &bm->bm_fpq[0]; \
	    register DM0P_STATE_HEAD	*m = &bm->bm_mpq[0]; \
	    for ( i = free = mod = fix = 0; \
		  i < DB_MAX_TABLEPRI; \
		  free += f->sth_count, \
		  fix  += f->sth_lcount + m->sth_lcount, \
		  mod  += m->sth_count, \
		  f++,m++,i++ ); \
	}

/* Count the total number of free, modified, and fixed groups */
#define		DM0P_COUNT_ALL_GQUEUES(free,mod,fix) \
	{ \
	    register i4			i; \
	    register DM0P_STATE_HEAD	*f = &bm->bm_gfq[0]; \
	    register DM0P_STATE_HEAD	*m = &bm->bm_gmq[0]; \
	    for ( i = free = mod = fix = 0; \
		  i < DB_MAX_TABLEPRI; \
		  free += f->sth_count, \
		  fix  += f->sth_lcount + m->sth_lcount, \
		  mod  += m->sth_count, \
		  f++,m++,i++ ); \
	}

/* Macros to inline p|v semaphore calls */

/* the funky "else" thing is just an easy way to generate 0 if
** CSp_semaphore succeeds;  both the BM_xxx are nonzero.
** It's hard to read, we're doing (sem->bms_locked = expr) == 0.
*/
#define		dm0p_mlock(mode,sem) \
	( ( CSp_semaphore(mode, &(sem)->bms_semaphore) ) \
	    ? dm0p_mlock_fcn(mode, sem, __LINE__) : \
	      ( ((sem)->bms_locked = ((mode) ? BM_EXCL : BM_SHARE)) == 0))

#define		dm0p_munlock(sem) \
	(sem)->bms_locked = BM_UNLOCKED, \
	( CSv_semaphore(&(sem)->bms_semaphore) ) \
	    ? dm0p_munlock_fcn(sem, __LINE__) : 0

/* Suffix for LOCATION filenames describing shared cache memory segments */
#define		CACHENAME_SUFFIX	".dat"

/* Maximum length for cache name. Anything longer is truncated to this length */
#define		CACHENM_MAX		8


/*}
** Name: DM0P_SEMAPHORE - Buffer manager semaphore definition.
**
** Description:
**	Defines a semaphore plus an additional, portable, field which 
**	identifies the state of the semaphore, locked or unlocked.
**
** History:
**	12-Sep-1996 (jenjo02)
**	    Invented.
*/
struct _DM0P_SEMAPHORE
{
    i4		    	    bms_locked;	    /* State of the semaphore:  */
#define		BM_UNLOCKED	0x0000 	    /*   Unlocked		*/
#define		BM_EXCL		0x0001 	    /*   Locked EXCL		*/
#define		BM_SHARE	0x0002 	    /*   Locked SHARE		*/
#define		BM_LOCKED	(BM_EXCL | BM_SHARE)
    CS_SEMAPHORE	    bms_semaphore;  /* The semaphore itself.    */
};

/*}
** Name: DM0P_STATE - Buffer state queue definition.
**
** Description:
**      This structure defines the queue headder for the buffer state queue.
**	The buffer state queue links buffers with the same state.  The state
**	is either modified or free.  The modified and free queues are both
**	arrays of queues by priority.
**
**	The queue header contains index values giving the position of the
**	indicated buffer in the buffer pool.
**
** History:
**     06-jun-1986 (Derek)
**          Created for Jupiter.
**     20-jan-1989 (rogerk)
**	    Changed pointers to be indexes into buffer pool to make
**	    Buffer Manager position independent.
**     30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
*/
struct _DM0P_STATE
{
    i4		    stq_next;	    /* Next state queue entry. */
    i4		    stq_prev;	    /* Previous state queue entry. */
};

/*}
** Name: DM0P_STATE_HEAD - state queue header..
**
** Description:
**      This structure defines the queue header used to anchor a state queue.
**
** History:
**     30-Aug-1996 (jenjo02)
**	    Invented.
**	24-Aug-1999 (jenjo02)
**	    Added 2 new buf_type values, BMCB_FHDR, BMCB_FMAP, previously 
**	    indistinguishable from BMCB_ROOT.
**	    Added cache stats by buf_type to DM0P_BSTAT. Added
**	    2 new stats, bs_reclaim (number of reclaims of BUF_FREE buffers) and
**	    bs_replace (reclaim of BUF_VALID free or modified buffer). Fixed-priority
**	    stats removed. Added sth_count to DM0P_STATE_HEAD, obviating the need
**	    for bm_gfcount and bm_gmcount.
**	10-Jan-2000 (jenjo02)
**	    Added sth_lcount to keep scattered tally of fixed buffers/groups.
*/
struct _DM0P_STATE_HEAD
{
    DM0P_STATE		    sth_stq;	    /* Head of state queue */
    i4		    	    sth_count;	    /* Number of elements on queue */
    i4		    	    sth_lcount;	    /* Number of fixed elements */
    DM0P_SEMAPHORE	    sth_mutex;	    /* A mutex to protect the queue */
};

/*}
** Name: DM0P_HASH_HEAD - Buffer hash queue header..
**
** Description:
**      This structure defines the header used to anchor a hash queue.
**
** History:
**     30-Aug-1996 (jenjo02)
**	    Created for mutex granularity project in memory of 
**	    Jerry Garcia.
*/
struct _DM0P_HASH_HEAD
{
    i4		    hash_next;	    /* Next hash queue entry. */
    DM0P_SEMAPHORE  hash_mutex;	    /* A mutex to protect the queue */
};

/*}
** Name: DM0P_BID 
**
** Description: Buffer id.  
**
** History:
**     06-mar-1996 (stial01)
**          Created to specify next buffer on a queue, where 
**          buffers can span multiple buffer caches.
*/
struct _DM0P_BID
{
    i4         bid_cache;     /* which buffer cache */
    i4         bid_index;    /* index into buffer array */
};

/*}
** Name: DM0P_BLQ_HEAD 
**
** Description: The head of a cross-cache buffer list.
**
** History:
**	30-Aug-1996 (jenjo02)
**	    Created .
**	19-Jan-1998 (jenjo02)
**	    Renamed from DM0P_BID_HEAD
*/
struct _DM0P_BLQ_HEAD
{
    DM0P_BID	    blqh_bid;     /* first buffer id in list */
    DM0P_SEMAPHORE  blqh_mutex;   /* A mutex to protect the list */
};


/*}
** Name: DM0P_BLQ - Buffer link queue definition.
**
** Description:
**	This structure defines an entry in a buffer link queue.
**	A buffer link queue links buffers with some commonality.
**
**	The queue contains index values giving the position of the
**	indicated buffer in the buffer pool.
**
** History:
**     28-sep-1987 (jennifer)
**          Created.
**     20-jan-1989 (rogerk)
**	    Changed pointer to be indexes into buffer pool to make the
**	    Buffer Manager position independent.
**	19-Jan-1998 (jenjo02)
**	    Renamed from DM0P_TRQ.
*/
struct _DM0P_BLQ
{
    DM0P_BID  	    blq_next;	    /* Next queue entry. */
    DM0P_BID  	    blq_prev;	    /* Previous queue entry. */
};

/*}
** Name: DM0P_BUFFER - Internal structure of a buffer descriptor.
**
** Description:
**      This structure is used internally by the buffer manager to maintain
**	information about the use of a page by a transaction.  If the page
**	is not currently being used by a transaction, it can still have
**	valid contents.  The external caller has a truncated view of the
**	buffer descriptor.  This allows for a thin wall of protection between
**	the buffer manager and the caller.
**
**	The buffer status indicators mean the following:
**
**	    BUF_FREE			The buffer is on
**					the priority zero free queue.
**	    BUF_VALID			The buffer has legal contents and is on
**					the hash queue.
**	    BUF_FIXED			The buffer is fixed and on
**					the hash queue.
**	    BUF_MODIFIED		The buffer has legal contents, needs to
**					be written to disk, in on the hash queue
**					and on the modified priority queue if
**					not also BUF_FIXED.
**	    BUF_WRITE			The buffer is being accessed for write.
**					Only valid if BUF_FIXED is set.
**	    BUF_LOCKED			The buffer has a cache lock set.
**	    BUF_CACHED			The buffer is locked in null mode on the
**					hash queue and the free priority queue.
**	    BUF_RBUSY			The buffer has a READ I/O is progress.
**	    BUF_WBUSY			The buffer has a WRITE I/O is progress.
**	    BUF_TEMP_FIXED
**	    BUF_RECLAIM			Free or modified buffer is being reclaimed
**					for use by fault_page().
**	    BUF_INVALID			The buffer has encountered an I/O or lock
**					error.  This flag is only cleared by
**					dm0p_invalidate, dm0p_tran_invalidate,
**					or dm0p_force_pages.  Thus this page
**					cannot be accessed until the transaction
**					is aborted.
**	    BUF_MUTEX			A transaction currently holds a mutex on
**					this page.
**	    BUF_MWAIT			Some number of transactions are waiting
**					for the mutex.
**	    BUF_IOWAIT			Some number of transactions are waiting
**					for I/O to complete.
**	    BUF_FCWAIT			Threads are waiting for FCBUSY to go.
**	    BUF_MUSTLOCK		Flag used by group read code to 
**					determine whether a lock is required 
**					and whether the lock has been granted 
**					when recoverying from errors.
**	    BUF_FCBUSY			The buffer is in use by the fast commit
**					thread and cannot be changed or tossed.
**	    BUF_WCOMMIT			Write to disk on COMMIT.
**	    BUF_WUNFIX			Write to disk on UNFIX. May be cleared
**                                      if page protected by table X lock.
**          BUF_WUNFIX_REQ              Write to disk on UNFIX.
**	    BUF_PAWRITE			Write to disk when pass abort signaled.
**          BUF_TUNFIX                  Toss buffer on UNFIX - used in PASS
**                                      ABORT protocols.
**	    BUF_NOTXLOCK		Page is not locked by the transaction.
**	    BUF_PRIOFIX			The buffer's priority should remain
**					fixed.  It should not be decremented
**					over time, nor incremented on reference.
**	    BUF_MUTEX_OK		Buffer is mutexed while it is being 
**			                forced in dm0p_close_page.
**	    BUF_HI_TPRIO		Something to do with readahead.
**          BUF_NULL_TUNFIX             Used for DMCM. System catalogs are 
**                                      tossed and their locks converted to
**                                      NULL at the end of transaction.
**          BUF_DMCM                    Used to indicate that the buffer is
**                                      participating in the DMCM protocol.
**          BUF_INTENT_LK               Buffer is locked with INTENT lock.
**	    BUF_TEMPTABLE		Page in buffer belongs to a temporary
**					table.
**	    BUF_WBBUSY			The buffer is being readied to be forced
**					by a WriteBehind thread.
**	    BUF_GW_QUEUED		Buffer is scheduled for GatherWrite.
**
**	The buf_tcb_stamp holds the TCB validation stamp (obtained from a
**	tcb lock value) of the buffer's parent table.  This is written in
**	the buffer header when a page is faulted into the cache.  When the
**	buffer manager decides to toss a random page from the cache, it uses
**	this value to make sure that the TCB it has is the correct version.
**	It is possible that the table has been modified by another server
**	connected to the same cache and that we are holding an old version
**	of the TCB.
**
** History:
**     14-oct-1985 (derek)
**          Created new for Jupiter.
**     30-mar-1987 (ac)
**          Added read/nolock support.
**     30-jan-1989 (rogerk)
**	    Changed pointers to be indexes into buffer pool to make the
**	    Buffer Manager position independent.  Took out buf_tcb pointer and
**	    replaced with buf_tabid.  Added buf_index, buf_dbid, buf_cpcount,
**	    buf_hash_bucket, buf_tr_bucket fields.  Took out buf_newvalue.
**	10-jul-89 (rogerk)
**	    Added BUF_EXTENDPAGE buffer status to fix multi-server extend bug.
**	    We won't force buffer in fast commit thread that is marked
**	    BUF_EXTENDPAGE.  Newly allocated pages are marked BUF_EXTENDPAGE
**	    until they are forced to disk.
**	14-aug-90 (rogerk)
**	    Added buf_tcb_stamp field (used one of the pad fields).  Used to
**	    verify TCB's when a page needs to be tossed from a shared cache.
**	    The TCB's validation stamp is written here when a page is faulted
**	    into the cache.
**	29-apr-1991 (Derek)
**	    Added new buf_sts constants NOTXLOCK,WUNFIX,PAWRITE.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    4-feb-1991 (rogerk)
**		Added support for fixed buffer priorities.
**	 3-nov-1991 (rogerk)
**	    Added fixes for File Extend recovery.
**	    Added buffer status PAUNFIX which allows us to tell whether the
**	    WUNFIX condition of a buffer is because of the type of buffer or
**	    because a pass-abort operation is in progress.  This tells us
**	    whether we can reset the WUNFIX status after the pass-abort is
**	    complete.
**      6-jan-1992 (jnash)
**          Remove BM_PAUNFIX, add BM_TUNFIX in modifying File Extend and
**          Btree index page pass abort recovery.
**	26-oct-1992 (rogerk)
**	    Added WCOMMIT flag and removed FORCE_COMMIT.
**	29-feb-96 (nick)
**	    Added BUF_FCWAIT.
**      01-aug-96 (nanpr01 for ICL phil.p)
**          Added buf_lock_mode for use by the Distributed Multi-Cache
**          Management Protocol. Each time a succesful cache lock
**          request is made, the mode is copied into this field; so that 
**          we always know the mode at which a cached page is locked.
**          Added pid for use by the DMCM protocol. When a server makes a
**          lock request providing a callback, the servers process id is
**          stored in the buffer. This enables us to make a sensible
**          decision about buffers when this server is shut down.
**          Added buffer statuses BUF_NULL_TUNFIX and BUF_DMCM.
**	13-Sep-1996 (jenjo02)
**	    Added buf_mutex.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Added BUF_X_LATCH.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Removed BUF_MUSTLOCK status bit, buf_pid.
**      25-apr-97 (stial01)
**          Added buf_lock_list, Renamed BUF_X_LATCH, BUF_IX_LOCK.
**          Added BUF_MLOCK_LIST: If set, only buf_lock_list or the 
**          buffer manager can mutex the buffer.
**	26-Mar-1998 (jenjo02)
**	    Renamed obsolete BUF_EXTENDPAGE to BUF_TEMPTABLE. 
**	27-Apr-1999 (jenjo02)
**	  o Replaced i4 buf_index with DM0P_BID buf_id to ease cross-cache 
**	    identification. buf_id contains both the cache and buffer indexes
**	    to completely identify a buffer.
**	  o Added buf_cp_next to link CP candidate buffers across caches.
**	  o Added buf_page_offset so it doesn't have to be constantly
**	    computed.
**	12-Jul-1999 (jenjo02)
**	    Removed buf_cp_next.
**	10-Jan-2000 (jenjo02)
**	    Made buf_sts, buf_priority volatile.
**      19-Jun-2002 (horda03) Bug 108074.
**          Added BUF_WUNFIX_REQ. This indicates that the buffer
**          should normally be writen to disk on UNFIX (BUF_WUNFIX).
**          But to prevent FORCE_ABORT filling TX log file, the
**          BUF_WUNFIX flag will be cleared if the page is protected
**          by an X table level lock.
**	31-Jan-2003 (jenjo02)
**	    Added buf_cpfcb abstraction for cp_sort().
**	18-Aug-2003 (devjo01)
**	    When crossing fix for 108074 to main, recycled bit for
**	    BUF_TEMP_FIXED into BUF_WUNFIX_REQ.  BUF_TEMP_FIXED was
**	    set but never checked, and all 32 bits are in use!
**      18-Sep-2003 (jenjo02) SIR 110923
**          Added buf_fp_action, buf_fp_llid in support of 
**          GatherWrite, complete_f_p() use by groups.
**	01-Mar-2004 (jenjo02)
**	    Added buf_partno for pages from Partitions.
**	27-Apr-2005 (schka24)
**	    Make buf_hash_bucket volatile since it's set unmutexed for groups.
**	11-Sep-2006 (jonj)
**	    Add buf_log_id, the log handle under which the buffer
**	    was put on the transaction queue, used by dm0p_force_pages().
**	10-Oct-2007 (jonj)
**	    Add BUF_STS define, finally.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add buf_page_lsn, buf_page_tranid,
**	    buf_cr_root, buf_cr_next, buf_cr_prev, buf_cr_iso, buf_cr_noundo,
**	    buf_create, buf_pincount, buf_mpend.
**	29-Jun-2010 (jonj)
**	    Added buf_cr_undo, buf_cr_where to aid debugging.
*/
struct _DM0P_BUFFER
{
    DM0P_SEMAPHORE  buf_mutex;		/* A mutex to protect the buffer */
    volatile i4	    buf_hash_bucket;	/* Hash queue bucket page is in */
    i4	    	    buf_next;		/* Next buffer on the hash chain. */
    i4	    	    buf_prev;		/* Previous buffer on hash chain. */
    DM0P_STATE	    buf_state;		/* The buffer state queue. */
    i4	    	    buf_txnq_bucket;	/* Transaction bucket page is in */
    DM0P_BLQ	    buf_txnq;		/* The buffer tran queue. */
    i4	    	    buf_tabq_bucket;	/* Table bucket page is in */
    DM0P_BLQ	    buf_tabq;		/* The buffer table queue. */
    u_i4    	    buf_cpcount;	/* When last FC thread looked at buf */
    DM0P_BID	    buf_id;		/* This buffer's cross-cache identifier. */
    SIZE_TYPE	    buf_page_offset;	/* Offset to page in cache */
    DB_TAB_ID	    buf_tabid;		/* The table the page belongs to */
    i4	    	    buf_dbid;		/* The database the page belongs to */
    volatile u_i4   buf_sts;		/* Buffer status indicators. */
#define			BUF_FREE	    0x00000
#define                 BUF_VALID	    0x00001	
#define			BUF_MODIFIED	    0x00002
#define			BUF_FIXED	    0x00004
#define			BUF_WRITE	    0x00008
#define			BUF_LOCKED	    0x00010
#define			BUF_CACHED	    0x00020
#define			BUF_IOERROR	    0x00040
#define			BUF_RECLAIM	    0x00080
#define			BUF_RBUSY	    0x00100
#define			BUF_WBUSY	    0x00200
#define			BUF_BUSY	    (BUF_RBUSY | BUF_WBUSY)
#define			BUF_WUNFIX_REQ	    0x00400
#define			BUF_INVALID	    0x00800
#define			BUF_MUTEX	    0x01000
#define			BUF_MWAIT	    0x02000
#define			BUF_IOWAIT	    0x04000
#define			BUF_WBBUSY	    0x08000
#define			BUF_WCOMMIT	    0x10000
#define			BUF_FCBUSY	    0x20000
#define			BUF_TEMPTABLE	    0x40000
#define			BUF_NOTXLOCK	    0x80000
#define			BUF_WUNFIX	    0x100000
#define			BUF_PAWRITE	    0x200000
#define			BUF_PRIOFIX	    0x400000
#define                 BUF_TUNFIX          0x800000
#define                 BUF_MUTEX_OK        0x1000000
#define                 BUF_HI_TPRIO        0x2000000	/* temporary hi prty */
#define			BUF_FCWAIT	    0x4000000
#define                 BUF_INTENT_LK       0x8000000
#define                 BUF_NULL_TUNFIX     0x10000000
#define                 BUF_DMCM            0x20000000
#define                 BUF_GW_QUEUED       0x40000000
#define                 BUF_MLOCK           0x80000000
/* String values of above bits: */
#define	BUF_STS "\
VALID,MODIFIED,FIXED,WRITE,LOCKED,CACHED,IOERROR,RECLAIM,RBUSY,WBUSY,\
WUNFIX_REQ,INVALID,MUTEX,MWAIT,IOWAIT,WBBUSY,WCOMMIT,FCBUSY,TEMPTABLE,\
NOTXLOCK,WUNFIX,PAWRITE,PRIOFIX,TUNFIX,MUTEX_OK,HI_TPRIO,FCWAIT,\
INTENT_LK,NULL_TUNFIX,DMCM,GW_QUEUED,MLOCK"

    i2		    buf_type;		/* The type of page. Used for
					** priority adjustment. */
    i2              buf_relpgtype;      /* iirelation relpgtype (TCB_PG_*) */
    volatile i4	    buf_priority;	/* Buffer priority for reuse. */
    i4		    buf_refcount;	/* Buffer reference count. */
    i4	    	    buf_owner;		/* Group owner of the buffer. */
#define			BUF_SELF	-1L /* Not a group buffer */
    DB_TRAN_ID	    buf_tran_id;	/* Transaction id of last owner. */
    i4         	    buf_log_id;	        /* LogId that put buf on txn queue */
    LK_LKID	    buf_lk_id;		/* Cache lockid. */
    LK_VALUE	    buf_oldvalue;	/* Old and current lock value. */
    LK_LOCK_KEY	    buf_lock_key;	/* Key to use for cache locking. */
    i4	    	    buf_tcb_stamp;	/* The table's TCB validation stamp */
    i4         	    buf_lock_list;      /* Lock list that has buffer mutexed */
    i4         	    buf_lock_mode;      /* mode at which cache lock is held */
    i4		    buf_cpindex;	/* CPindex of this buffer,
					   for complete_cp_flush */
    PTR		    buf_cpfcb;		/* CP sort key abstraction */
    i4		    buf_fp_action;	/* Write completion action */
    i4		    buf_fp_llid;	/* Write completion "owner" */
    i4		    buf_partno;		/* Table's partition number */
    PID		    buf_pid;		/* Who allocated cache lock. */
    i4		    buf_cr_root;	/* Root buffer of CR page,
    					   BUF_ENDLIST if not CR page */
    i4		    buf_cr_next;	/* Next CR version of root page */
    i4		    buf_cr_prev;	/* Prev CR version of root page */
    i4		    buf_pincount;	/* Count of pins on buffer */
    i4		    buf_mpend;		/* Count of pending dm0pMutex */
    LG_LSN	    buf_cr_blsn;	/* CR bos_lsn page restored to */
    LG_LSN	    buf_cr_clsn;	/* CR last_commit page restored to */
    LG_LSN	    buf_page_lsn;	/* Copy of page LSN */
    DB_TRAN_ID	    buf_page_tranid;	/* Copy of page transaction id */
    i4		    buf_cr_undo;	/* Count of undos applied */
    i4		    buf_cr_noundo;	/* Count of skipped undos */
    i4		    buf_cr_iso;		/* CR isolation level when materialized */
    i4		    buf_cr_where;	/* dm0pMakeCRpage __LINE__ */
    i4	    	    buf_create;		/* dm0p __LINE__ where buf_create set */
};


/*}
** Name: DM0P_GROUP - Internal structure of a group descriptor
**			for readahead support.
**
** Description:
**      This structure is used internally by the buffer manager to maintain
**	information about the a group of pages that read by an I/O operation.
**
**	The group status indicators mean the following:
**
**	    G_FREE			The group is on the group free
**					queue.
**	    G_FIXED			The group is on the group fixed
**					queue.
**	    G_MODIFIED			The group is on the group modified
**					queue.
**	    G_WBUSY			The group is in write busy state. 
**	    G_RBUSY			The group is in read busy state. 
**	    G_WAIT			A thread is waiting on the group.
** History:
**     22-Jan-1987 (ac)
**          Created new for Jupiter.
**     30-jan-1989 (rogerk)
**	    Changed pointers to be indexes into buffer pool to make the
**	    Buffer Manager position independent.  Added g_mutex field.
**	13-Sep-1996 (jenjo02)
**	    Added g_mutex.
**	05-Feb-1997 (jenjo02)
**	    Added G_WAIT.
**	14-Mar-1997 (jenjo02)
**	    Added G_RBUSY, G_BUSY to group status flags.
**	10-Jan-2000 (jenjo02)
**	    Added g_valcount, g_new_pri, g_priority,
**	    g_pages. 
**	    Made g_sts volatile.
**	22-Sep-2000 (jenjo02)
**	    Added g_lock_list to identify who set BUSY status.
**	15-Nov-2002 (jenjo02)
**	    Added G_SKIPPED flag for gfault_page().
**	04-Apr-2008 (jonj)
**	    Add G_STS define.
*/
struct _DM0P_GROUP
{
    DM0P_SEMAPHORE  g_mutex;		/* A mutex to protect the group */
    DM0P_STATE	    g_state;		/* The group state queue. */
    volatile i4	    g_sts;		/* Group status indicators. */
#define			G_FREE		0x0000
#define			G_FIXED		0x0001
#define			G_MODIFIED	0x0002
#define			G_WBUSY		0x0004
#define			G_RBUSY		0x0008
#define			G_BUSY		(G_WBUSY | G_RBUSY)
#define			G_WAIT		0x0010 /* waiting on BUSY group */
#define			G_SKIPPED	0x0020 /* Skipped by gfault_page() */
/* String values of above bits: */
#define	G_STS "\
FIXED,MODIFIED,WBUSY,RBUSY,WAIT,SKIPPED"

    i4	    	    g_index;		/* Index to descriptor in group array */
    i4         	    g_pgsize;           /* Page size for group */
    i4		    g_refcount;		/* Number of fixed pages in group */
    i4		    g_modcount;		/* Number of modified pages in group */
    i4		    g_valcount;		/* Number of valid pages in group */
    i4		    g_pages;		/* Number of contiguous pages in group */
    i4		    g_new_pri;		/* New priority to assign to group */
    volatile i4	    g_priority;		/* Priority of this group */
    i4		    g_buffer;		/* Index to the buffers array for the
					** first buffer in this group. */
    i4		    g_lock_list;	/* Lock list that set BUSY status */
};

/*}
** Name: DM0P_BMCACHE - Database or Table cache entry.
**
** Description:
**      This structure is used internally by the buffer manager to maintain
**	information about databases and tables that have cached pages in the
**	buffer manager.
**
**	The cache entries are used for performance reasons only.  Pages can
**	exist in the buffer cache that do not have a corresponding entry in
**	the database or table cache list.
**
**	The cache entries are used by the buffer manager to enable it to
**	keep pages cached for tables and databases that are not open by
**	any sessions connected to the buffer manager.  Each cache entry has
**	a lock that is maintained on the buffer manager lock list.  The locks
**	values are checked at database or table open time to tell if the
**	cached pages for that database or table are still valid.
**
**	The cache entries are kept in an LRU list.  Each time a new descriptor
**	is needed, the least recently used entry is tossed out (meaning that
**	the next time that database/table is opened, all corresponding pages
**	will be tossed from the cache).
**
** History:
**      6-Feb-1989 (rogerk)
**          Created new for Terminator.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**	 4-feb-1991 (rogerk)
**	    Added support for fixed buffer priorities - added bmc_priority.
**	13-Sep-1996 (jenjo02)
**	    Added bmc_mutex.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Removed bmc_priority.
**	04-Apr-2008 (jonj)
**	    Add BMC_STS define.
*/
struct _DM0P_BMCACHE
{
    DM0P_SEMAPHORE  bmc_mutex;	    /* A mutex to protect the cache */
    DM0P_STATE	    bmc_state;	    /* List of other cache entries */
    i4	    	    bmc_sts;	    /* Entry status */
#define			BMC_FREE	0x0000
#define			BMC_VALID	0x0001
#define			BMC_BUSY	0x0002
#define			BMC_WAIT	0x0004
/* String values of above bits: */
#define	BMC_STS "\
VALID,BUSY,WAIT"

    i4	    	    bmc_dbid;	    /* Database this entry referes to */
    i4	    	    bmc_tabid;	    /* Base table this entry refers to */
    i4	    	    bmc_index;	    /* Index of this entry in cache array */
    i4	    	    bmc_refcount;   /* Number of references to this entry */
    LK_LKID	    bmc_lkid;	    /* Cache lock */
    LK_VALUE	    bmc_lkvalue;    /* Cache lock value */
};

/*}
** Name: DM0P_BSTAT - Buffer Statistics 
**
** Description:
**      This control block contains buffer cache statistics.
**
** History:
**      06-mar-1996 (stial01)
**          Created for multiple page sizes.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added new stats for fixed priority buffers, bs_toss.
**	    Changed all stats to u_i4.
**	01-May-1997 (jenjo02)
**	    Added new stats for group waits.
**	20-May-1998 (jenjo02)
**	    Added new stats for cache-specific WriteBehind threads.
**	24-Aug-1999 (jenjo02)
**	    Added cache stats by buf_type to DM0P_BSTAT. Added
**	    2 new stats, bs_reclaim (number of reclaims of BUF_FREE buffers) and
**	    bs_replace (reclaim of BUF_VALID free or modified buffer). Fixed-priority
**	    stats removed.
**	10-Jan-2000 (jenjo02)
**	    Keep group stats by page type.
**	09-Apr-2001 (jenjo02)
**	    Added bs_gw_pages, bs_gw_io.
**	31-Jan-2003 (jenjo02)
**	    Changed some "wb" names to "cfa" for Cache Flush Agents.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add host of stats for Consistent Read.
**	19-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bs_lreadio stat to distinguish reads from
**	    log buffers from physical i/o reads.
**	15-Mar-2010 (smeke01) b119529
**	    Updated most DM0P_BSTAT fields to be u_i8.
*/
struct _DM0P_BSTAT
{
    u_i4       	    bs_pgsize;              /* For dm0pmo.c         */
    u_i8	    bs_fix[BMCB_PTYPES+1];    /* Number of fix calls. */
    u_i8	    bs_hit[BMCB_PTYPES+1];    /* Page found is cache. */
    u_i8	    bs_iowait[BMCB_PTYPES+1]; /* Stall for I/O completion. */
    u_i8	    bs_mwait[BMCB_PTYPES+1];  /* Waits for page mutex. */
    u_i8       	    bs_fcwait[BMCB_PTYPES+1]; /* Stall for FC completion. */
    u_i8	    bs_check[BMCB_PTYPES+1];  /* Cached page still valid. */
    u_i8	    bs_refresh[BMCB_PTYPES+1];/* Cached page was invalid. */
    u_i8	    bs_unfix[BMCB_PTYPES+1];  /* Number of unfix calls. */
    u_i8	    bs_dirty[BMCB_PTYPES+1];  /* Dirty page unfixed. */
    u_i8	    bs_force[BMCB_PTYPES+1];  /* Force page for reuse */
    u_i8	    bs_toss[BMCB_PTYPES+1];   /* Toss page from cache */
    u_i8	    bs_reads[BMCB_PTYPES+1];  /* Number of reads. */
    u_i8	    bs_writes[BMCB_PTYPES+1]; /* Number of writes. */
    u_i8       	    bs_syncwr[BMCB_PTYPES+1]; /* single sync write counter */
    u_i8       	    bs_reclaim[BMCB_PTYPES+1];/* free buffer reclamations */
    u_i8       	    bs_replace[BMCB_PTYPES+1];/* Toss page to make room */
    u_i8	    bs_greads[BMCB_PTYPES+1]; /* Number of group reads. */
    u_i8	    bs_gwrites[BMCB_PTYPES+1];/* Number of group writes. */
    u_i8       	    bs_gsyncwr;             /* group sync write counter */
    u_i8	    bs_fwait;		    /* Number of free buffer waiters. */
    u_i8       	    bs_gwait;               /* Stall for busy group. */
    u_i4       	    bs_cfa_active;          /* Number of active CacheFlushAgents */
    u_i4       	    bs_cfa_hwm;             /* HWM of CacheFlushAgents */
    u_i4       	    bs_cache_flushes;       /* Number of Cache flush cycles */
    u_i4       	    bs_cfa_cloned;          /* Number of cloned Agents */
    u_i8	    bs_wb_flushed;	    /* Number of WB pages flushed this cycle */
    u_i8	    bs_wb_gflushed;	    /* Number of WB groups flushed this cycle */
    u_i8	    bs_gw_pages;	    /* Pages written by GatherWrite */
    u_i8	    bs_gw_io;	    	    /* Physical I/O rqd for bs_gw_pages */
    /* Added for MVCC: */
    u_i8	    bs_pwait[BMCB_PTYPES+1];  /* Waits for page pinlock. */
    u_i8	    bs_crreq[BMCB_PTYPES+1];  /* Number of CR requests. */
    u_i8	    bs_crhit[BMCB_PTYPES+1];  /* Number of CR hits. */
    u_i8	    bs_craloc[BMCB_PTYPES+1]; /* Number of CR buffers allocated */
    u_i8	    bs_crmat[BMCB_PTYPES+1];  /* Number of CR materialized */
    u_i8	    bs_nocr[BMCB_PTYPES+1];   /* Could not make consistent */
    u_i8	    bs_crtoss[BMCB_PTYPES+1]; /* Toss CR page from cache */
    u_i8	    bs_roottoss[BMCB_PTYPES+1];/* Toss CR root page from cache */
    u_i8	    bs_lread[BMCB_PTYPES+1];  /* Logical reads from log */
    u_i8	    bs_lreadio[BMCB_PTYPES+1];/* Physical reads from log */
    u_i8	    bs_lundo[BMCB_PTYPES+1];  /* Number of undos from log */
    u_i8	    bs_jread[BMCB_PTYPES+1];  /* Journal blocks read */
    u_i8	    bs_jundo[BMCB_PTYPES+1];  /* Number of undos from journals */
    u_i8	    bs_jhit[BMCB_PTYPES+1];   /* Number of jread that were spot on */
};

/*}
** Name: DM0P_BMCB - Buffer manager control block.
**
** Description:
**      This control block contains the cache specific information needed
**      to manage the flow of buffer from disk to memory and back.
**
**	This control block is placed into shared memory in installations
**	where servers share common buffer managers.
**
**	This control block and corresponding buffer descriptors and pages
**	are referenced through the Local Buffer Manager Control Block.
**
** History:
**     15-oct-1985 (derek)
**          Created new for Jupiter.
**     30-sep-1988 (rogerk)
**	    Added bm_wbstart, bm_wbend, bm_wbflush, bm_fcflush fields.
**	    Added BM_WBWAIT and BM_FCFLUSH flags for write behind threads.
**     20-jan-1989 (rogerk)
**	    Made Buffer Manager position independent for shared memory.
**	    Broke up into BMCB and LBMCB (local bmcb).
**	    Added database and table cache arrays.
**	    Added fields: bm_id, bm_lock_identifier, bm_srv_count, bm_cpcount,
**	    bm_cpindex, bm_cpcheck, bm_dbclist, bm_tblclist, bm_dbcsize,
**	    bm_tblcsize.
**     10-apr-1989 (rogerk)
**	    Added bm_name field to hold buffer manager name.
**     25-sep-1989 (rogerk)
**	    Added BM_PASS_ABORT, BM_PREPASS_ABORT to bm_status.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Added support for fixed buffer priorities.
**		Added bm_tblpsize and bm_tblpindex fields.
**	24-feb-1993 (keving)
**	    => Added changes to allow a buffer manager to work with 
**	    => multiple logs
**	18-oct-1993 (rmuth)
**	    Collect BM statistics within the DMP_BMCB. This is usefull
**	    in a shared cache environment in non-shared cache they will
**	    be the same as DMP_LBMCB.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	03-dec-93 (swm)
**          Bug #58635
**	    Changed bm_1_reserved, bm_2_reserved in DMP_BMCB from i4 to
**	    i4 * for consistency with DM_SVCB.
**	19-apr-1995 (cohmi01)
**	    Add BM_BATCHMODE indicator for bm_status.
**	10-may-1995 (cohmi01)
**	    Move batchmode indicator to svcb, instead of BM. Added
**	    BM_IOMASTER indicator, means an IOMASTER server is attached to BM.
**	24-oct-1995 (thaju02/stoli02)
**	    Added bm_syncwr and bm_gsyncrwr forced synchronous writes counters.
**      06-mar-1996 (stial01)
**          Modified for multiple page sizes:
**          Moved page size independent fields into DM0P_BM_COMMON
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced field bm_dmcm. When deallocating a buffer manager
**          we need to know if it supports the Distributed Multi-Cache
**          Management (DMCM) protocol.
**	13-Sep-1996 (jenjo02)
**	    Changed bm_fq, bm_gfq, bm_eq, bm_geq, bm_gmq, 
**	    bm_fpq, bm_mpq to type DM0P_STATE_HEAD.
**	    Added bm_clock_mutex, bm_count_mutex.
**	24-Feb-1997 (jenjo02)
**	    Removed bm_eq from BMCB. It served no purpose.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Replace BMCB_MAX_PRI with widely known DB_MAX_TABLEPRI.
**	21-Nov-1997 (jenjo02)
**	    Removed single (bm_fq) and group (bm_gfq) buffer fixed state queues.
**	20-May-1998 (jenjo02)
**	    Cache-specific WriteBehind thread additions.
**	    Added bm_cache_ix, bm_gwbstart, bm_gwbend, bm_wb_mutex,
**	    bm_wb_thread_start, bm_wb_thread_end, BM_WBWAIT,
**	    BM_WB_IN_CACHE.
**	24-Aug-1999 (jenjo02)
**	    Removed bm_gfcount, bm_gmcount, sth_count used instead.
**	10-Jan-2000 (jenjo02)
**	    Added group priority queues.
**	    Removed bm_fcount, bm_mcount, bm_lcount, bm_gfcount, bm_gmcount,
**	    bm_glcount, bm_count_mutex.
**	15-Nov-2002 (jenjo02)
**	    Renamed bm_wb_mutex to bm_mutex to generically protect 
**	    all non-statics in BMCB, including bm_status and WB variables.
**	31-Jan-2003 (jenjo02)
**	    Added bm_cpagents, bm_cpabufs, bm_cfa_mask,
**	    BM_WB_SINGLE, BM_WB_GROUP, BM_WB_CP (reason Primary
**	    Agent was signalled).
**	04-Apr-2008 (jonj)
**	    Add BM_STATUS define.
*/

struct _DM0P_BMCB
{
    i4		    bm_cache_ix;	    /* cache index number */
    i4              bm_pgsize;              /* page size for this buffer cache*/
    i4	    	    bm_hshcnt;		    /* The number of hash buckets. */
    i4	    	    bm_bufcnt;		    /* The number of single and 
					    ** group buffers. */
    i4         	    bm_first;               /* Index of first buffer with  */ 
					    /* respect to all buffer caches */
    i4	    	    bm_sbufcnt;		    /* Single buffer count. */
    i4	    	    bm_gcnt;		    /* The number of groups. */
    i4	    	    bm_gpages;		    /* The maximum number of pages in a 
					    ** group. */
    i4	    	    bm_flimit;		    /* Lower limit on free list size. */
    i4	    	    bm_mlimit;		    /* Upper limit on modify list size. */
    i4	    	    bm_wbstart;		    /* Number of pages on modify list
					    ** at which to start asynchronous
					    ** write behind. */
    i4	    	    bm_wbend;		    /* Number of pages on modify list
					    ** at which to stop asynchronous
					    ** write behind. */
    i4	    	    bm_gwbstart;	    /* Number of groups on modify list
					    ** at which to start asynchronous
					    ** write behind. */
    i4	    	    bm_gwbend;		    /* Number of groups on modify list
					    ** at which to stop asynchronous
					    ** write behind. */
    i4	    	    bm_wb_optim_free;	    /* optimal number of free pages */
    DM0P_SEMAPHORE  bm_mutex;	    	    /* Generic BMCB mutex */
    i4		    bm_cpindex;	    	    /* Starting CP index in this cache */
    i4		    bm_cpbufs;	    	    /* Number of bufs in CP array */
    i4		    bm_cpagents;	    /* Number of CP agents needed */
    i4		    bm_cpabufs;		    /* Number of bufs per Agent */
    u_i4	    bm_cfa_mask;	    /* Mask of active CacheFlush agents */

    DM0P_BSTAT      bm_stats;               /* Cache statistics */
    i4	    	    bm_status;		    /* Cache status. */
#define			BM_GROUP           0x0001
#define                 BM_FWAIT           0x0002
#define                 BM_ALLOCATED       0x0004
#define                 BM_WBWAIT          0x0008
#define                 BM_WB_IN_CACHE     0x0010
#define                 BM_WB_SINGLE       0x0020
#define                 BM_WB_GROUP        0x0040
#define                 BM_WB_CP           0x0080
/* String values of above bits: */
#define	BM_STATUS "\
GROUP,FWAIT,ALLOCATED,WBWAIT,WB_IN_CACHE,WB_SINGLE,\
WB_GROUP,WB_CP"

    DM0P_SEMAPHORE  bm_clock_mutex;	    /* Protection for clock buffer */
    i4	    	    bm_clock;		    /* Point to the next buffer for
					    ** low down the priority level 
					    ** of the buffer by one. */

    DM0P_STATE_HEAD bm_gfq[DB_MAX_TABLEPRI];/* Priority Queue of free groups(groups
					    ** that have pages not fixed and not
					    ** modified. */
    DM0P_STATE_HEAD bm_gmq[DB_MAX_TABLEPRI];/* Priority Queue of modified groups
					    ** (groups that have pages not fixed but 
					    ** modified, ie waiting to be 
					    ** flushed out). */
    DM0P_STATE_HEAD bm_fpq[DB_MAX_TABLEPRI];/* Priority queue of free buffers.*/

    DM0P_STATE_HEAD bm_mpq[DB_MAX_TABLEPRI];/* Priority queue of modify buffers. */
};

/*}
** Name: DM0P_BM_COMMON - Buffer Manager Common Data.
**
** Description:
**      This control block contains all of the cache independent information
**      needed to manage the flow of buffer from disk to memory and back.
**
**      This control block is referenced through the Local Buffer Manager
**      Control Block.
**
**	In a shared cache buffer manager, this control block is always in
**	the first (0'th) memory segment.
**
** History:
**     06-mar-1996 (stial01)
**          Created for multiple page sizes: moved page size independent fields
**          from DMP_BMCB to here.
**	13-Sep-1996 (jenjo02)
**	    Removed bmcb_mutex.
**	    Added bmcb_cp_mutex, bmcb_status_mutex.
**	    Changed bmcb_dbclist, tblclist to type DM0P_STATE_HEAD.
**	    Changed bmcb_tran_buckets to type DM0P_BID_HEAD.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Removed bmcb_tblpsize, bmcb_tblpriority.
**	    Changed stats to u_i4.
**	    Removed bmcb_dmcm, made it a flag in bmcb_status.
**	19-Jan-1998 (jenjo02)
**	    Added bmcb_tabq_buckets to define cross-cache BUF_VALID
**	    pages belonging to a table.
**	01-Sep-1998 (jenjo02)
**	    Moved mutex wait stats out bmcb/lbm in to DM0P_BSTAT where they
**	    can be tracked by cache.
**	27-Apr-1999 (jenjo02)
**	  o Added bmcb_cp_next, bmcb_cpbufs to link CP candidate buffers 
**	    across caches.
**	12-Jul-1999 (jenjo02)
**	    Removed bmcb_cp_next.
**	31-Jan-2003 (jenjo02)
**	    Added BMCB_CFA_TRACE, set/unset by DM414
**	4-Jan-2005 (kschendel)
**	    Add segment count to make shared caches self describing.
**	    (A process connecting to the cache can map segment zero,
**	    find the bm-common, map the remaining segments, and use
**	    the BMS control blocks to set everything up.)
**	15-Mar-2006 (jenjo02)
**	    Added bmcb_cp_lsn.
**	21-May-2007 (kschendel) b122121
**	    Add CP thread count since not all sharing DMF servers have CP
**	    threads.  (e.g. fastload)
**	04-Apr-2008 (jonj)
**	    Add BMCB_NO_GLC, BMCB_STATUS defines.
*/

struct _DM0P_BM_COMMON
{
    DB_NAME	    bmcb_name;		    /* Buffer manager name. */
    i4	    	    bmcb_id;		    /* Unique Buffer Manager id */
    i4	    	    bmcb_dbcsize;	    /* Number of entries in dbclist. */
    i4	    	    bmcb_tblcsize;	    /* Number of entries in tblclist. */
    i4         	    bmcb_totbufcnt;         /* total # bufs in all caches */
    i4		    bmcb_segcnt;	    /* Number of (shared) memory segs
					    ** making up this BM; irrelevant
					    ** for non-shared caches.
					    */
    i4		    bmcb_gw_dicb_size;	    /* size of one of DI's GW control blocks,
					    ** zero if GatherWrite not enabled */

    DM0P_SEMAPHORE  bmcb_cp_mutex;	    /* Mutex to protect cp fields: */
    i4	    	    bmcb_srv_count;	    /* Number of servers using PBMCB */
    i4	    	    bmcb_cpthread_count;    /* Number of CP threads attached */
    u_i4    	    bmcb_cpcount;	    /* Current CP identifier */
    i4		    bmcb_cpbufs;	    /* Total buffers to process */
    i4         	    bmcb_cpindex;	    /* Current CP queue buffer */ 
    i4	    	    bmcb_cpcheck;	    /* Number FC threads done with CP */

    u_i4	    bmcb_lockreclaim;	    /* Cache lock reclaims. */
    u_i4	    bmcb_fcflush;           /* Number of Fast Commit flushes */
    u_i4	    bmcb_bmcwait;           /* Waits for db/tbl cache entry */
    DM0P_SEMAPHORE  bmcb_status_mutex;	    /* Mutex to protect bmcb_status */
    i4	    	    bmcb_status;	    /* Buffer manager status. */
#define			BMCB_FCFLUSH	    0x0001
#define			BMCB_SHARED_BUFMGR  0x0002
#define			BMCB_PASS_ABORT	    0x0004
#define			BMCB_PREPASS_ABORT  0x0008
#define                 BMCB_IOMASTER       0x0010 /* IOmaster server up */
#define                 BMCB_DMCM           0x0020 /* DMCM protocol in use */
#define			BMCB_IS_MT	    0x0040 /* OS threads in use */
#define			BMCB_CFA_TRACE	    0x0080 /* Trace cache flush activity */
#define                 BMCB_CLUSTERED      0x0100 /* Clustering enabled. */
#define                 BMCB_USE_CLM	    0x0200 /* Use CXmsg_redeem. */
#define                 BMCB_NO_GLC 	    0x0400 /* No GLC for Cluster (VMS only) */
/* String values of above bits: */
#define	BMCB_STATUS "\
FCFLUSH,SHARED,PASS_ABORT,PREPASS_ABORT,IOMASTER,\
DMCM,MT,CFA_TRACE,CLUSTERED,USE_CLM,NO_GLC"

    DM0P_STATE_HEAD bmcb_dbclist;	    /* LRU queue of db cache entries */
    DM0P_STATE_HEAD bmcb_tblclist;	    /* LRU queue of tbl cache entries */
    i4	    	    bmcb_reference[BMCB_PTYPES];/* Priority for reference. */
    i4	    	    bmcb_rereference[BMCB_PTYPES];/* Priority increment for
					    ** re-reference. */
    i4	    	    bmcb_maximum[BMCB_PTYPES];/* Max priority by page type .*/
    LG_LSN	    bmcb_last_lsn;	    /* Last flushed log record. */
    LG_LSN	    bmcb_cp_lsn;	    /* Forced LSN at last CP */
    LK_UNIQUE	    bmcb_lock_key;	    /* Lock list identifier for the
					    ** shared lock list used for all
					    ** cache locks. */
#define			BMCB_TRAN_MAX	    64
    DM0P_BLQ_HEAD   bmcb_txnq_buckets[BMCB_TRAN_MAX];	    
					    /* Array of tran buckets. */
#define			BMCB_TABLE_MAX	    128
    DM0P_BLQ_HEAD   bmcb_tabq_buckets[BMCB_TABLE_MAX];	    
					    /* Array of table buckets. */
};

/*}
** Name: DM0P_CLOSURE - Buffer manager closure control block.
**
** Description:
**	This control block contains buffer manager information that
**	is used when the GatherWrite facility is in use.
**
** 	It contains information passed to force_page() which is needed
**	when complete_f_p() is called upon completion of the write.
**
**	When GatherWrite is enabled, one of these structures
**	is allocated during BM startup for each buffer in each cache
**	and the resulting array pointed to by lbm->lbm_gw_closure.
**	
**	In addition to info for dm0p, it contains an embedded structure
**	used by the dm2f GatherWrite functions.
**
** History:
**     01-aug-1996 (nanpr01 for ICL anb)
**          Created.
**	07-May-1999 (jenjo02)
**	    Removed lsn, which isn't needed, added log_id, which is.
**	    Prefixed element names with "bmcl_" for clarity.
**	20-aug-2002 (devjo01)
**	    Added BMCL_DMCM_CBACK.
**	04-Apr-2008 (jonj)
*	    Add BMCL_EVCOMP define.
*/

struct _DM0P_CLOSURE
{
    DM0P_BUFFER         *bmcl_buf;   /* The buffer being written */
    DMP_TABLE_IO	*bmcl_tbio;  /* TBIO used to write page */
    STATUS              bmcl_status; /* Status of write, set by dm2f */
    i4             	bmcl_pages;  /* Number of contiguous pages */
    i4             	bmcl_operation;   /* force_page() operation */
    i4             	bmcl_lock_list;   /* The locking system tx id */
    i4             	bmcl_log_id; /* The logging system id */
    i4             	bmcl_evcomp; /* Identifier of GatherWrite user: */
#define BMCL_NO_GATHER		0
#define BMCL_CLOSE_PAGE		1
#define BMCL_FORCE_PAGES	2
#define BMCL_CP_FLUSH		3
#define BMCL_TOSS_PAGES		4
#define BMCL_FLUSH_PAGES	5
#define BMCL_WRITE_ALONG	6
#define BMCL_WPASS_ABORT	7
#define BMCL_DMCM_CBACK 	8
/* String values of above bits: */
#define	BMCL_EVCOMP "\
NO_GATHER,CLOSE_PAGE,FORCE_PAGES,CP_FLUSH,TOSS_PAGES,\
FLUSH_PAGES,WRITE_ALONG,WPASS_ABORT,DMCM_CBACK"

    DMP_CLOSURE		bmcl_dm2f;    /* dm2f's gatherWrite space */
};

/*}
** Name: DMP_LBMCB - Local buffer manager control block.
**
** Description:
**	This control block contains buffer manager information that
**	is local to one server, rather than what can be shared across
**	multiple servers.
**
**	All pointers to Buffer Manager structures that can be kept in
**	shared memory should be referenced through this control block.
**
**	The local buffer manager control block is linked off of the
**	DMF server control block.
**
** History:
**     20-jan-1989 (rogerk)
**          Created new for Terminator.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Added support for fixed buffer priorities.
**		Added lbm_tblpriority cache.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Added buffer manager fields for the
**	    transaction context used to force the log file.
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	03-dec-93 (swm)
**          Bug #58635
**	    Changed lbm_1_reserved, lbm_2_reserved in DMP_BMCB from i4 to
**	    i4 * for consistency with DM_SVCB.
**      06-mar-1996 (stial01)
**          Modified in support of variable page sizes.
**	25-mar-96 (nick)
**	    Added lbm_fcwait.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Added lbm_dmcm_lock_list.
**          For support of the Distributed Multi-Cache Management (DMCM)
**          protocol, this lock list is used for forcing pages in the DMCM
**          callback routine dm0p_dmcm_callback.
**	13-Sep-1996 (jenjo02)
**	    Changed lbm_buckets to type DM0P_HASH_HEAD.
**	14-Mar-1997 (jenjo02)
**	    Reinstated lbm_lxid_mutex.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Removed lbm_tblpriority.
**	    Changed stat fields to u_i4.
**	01-Sep-1998 (jenjo02)
**	    Moved mutex wait stats out bmcb/lbm in to DM0P_BSTAT where they
**	    can be tracked by cache.
**	29-Jun-1999 (jenjo02)
**	    Added GatherWrite-specific elements.
**	12-Jul-1999 (jenjo02)
**	    Added lbm_cp_array, used to pre-sort CP-eligible buffers.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define LBMCB_CB as DM_LBMCB_CB.
*/

struct _DMP_LBMCB
{
    i4	    	    *lbm_1_reserved;	    /* Not used. */
    i4	    	    *lbm_2_reserved;	    /* Not used. */
    SIZE_TYPE	    lbm_length;  	    /* Length of control block. */
    i2              lbm_type;		    /* Type of control block for
                                            ** memory manager. */
#define                 LBMCB_CB	    DM_LBMCB_CB
    i2              lbm_s_reserved;	    /* Reserved for future use. */
    PTR             lbm_l_reserved;	    /* Reserved for future use. */
    PTR             lbm_owner;		    /* Owner of control block for
                                            ** memory manager. LBMCB will be
                                            ** owned by the server. */
    i4         	    lbm_ascii_id;	    /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 LBMCB_ASCII_ID	    CV_C_CONST_MACRO('L', 'B', 'M', 'C')
    i4		    lbm_status;		    /* Server status bits: */
#define			LBM_GW_ENABLED	0x0001	/* GatherWrite in use */
    DM0P_BM_COMMON  *lbm_bm_common;	    /* Pointer to bm common data*/
    DM0P_BMCACHE    *lbm_dbcache;	    /* Database cache entries. */
    DM0P_BMCACHE    *lbm_tblcache;	    /* Table cache entries. */
    LK_LLID	    lbm_lock_list;	    /* Lock list for cache locks. */
    LK_LKID	    lbm_bm_lockid;	    /* Holds Buffer Manager Lock. */
    LG_DBID	    lbm_bm_dbid;	    /* Holds Buffer DBadd Id. */
    i4	    	    lbm_num_logs;	    /* Number of logs attached to this 
					    ** Buffer Manager */
    LG_LXID	    lbm_bm_lxid[DM_CNODE_MAX];	    /* Holds Buffer Log Ids. */
    DM0P_SEMAPHORE  lbm_lxid_mutex;	    /* Mutex protects use of lbm_bm_lxids */
    u_i4	    lbm_lockreclaim;	    /* Cache lock reclaims. */
    u_i4	    lbm_fcflush;	    /* Number of Fast Commit flushes */
    u_i4	    lbm_bmcwait;	    /* Waits for db/tbl cache entry */
    DM0P_BMCB       *lbm_bmcb[DM_MAX_CACHE]; /* Array of cache information */
    DM0P_BUFFER	    *lbm_buffers[DM_MAX_CACHE]; /* Array of buffers. */
    DM0P_GROUP	    *lbm_groups[DM_MAX_CACHE];  /* Array of groups. */
    DM0P_HASH_HEAD  *lbm_buckets[DM_MAX_CACHE]; /* Array of buckets. */
    DMPP_PAGE	    *lbm_pages[DM_MAX_CACHE];   /* Array of pages. */
    i4	    	    *lbm_cp_array[DM_MAX_CACHE]; /* Array of CP buffer identifiers */
    DM0P_BSTAT      lbm_stats[DM_MAX_CACHE];    /* Array of statistics */

    /* The next 2 fields are used when GatherWrite is enabled */
    DM0P_CLOSURE    *lbm_gw_closure[DM_MAX_CACHE]; /* Array of closure structures. */
    char	    *lbm_gw_dicb[DM_MAX_CACHE];/* Array of DI GW control blocks */

    LK_LLID         lbm_dmcm_lock_list;     /* DMCM lock list */
    PID		    lbm_pid;		    /* Process ID or LBM owner. */
};


/*}
** Name: DMP_BMSCB - Buffer Manager Segment control block.
**
** Description:
**
**      This control block is the first control block in Buffer Manager 
**      memory segments. Note that multiple pieces of memory are allocated
**      when the installation is configured with multiple buffer caches that
**      are to be allocated separately. The BMSCB contains the offsets 
**      to interesting sections of buffer manager memory allocated contiguous
**      with the BMSCB.
**
**	This control block is placed into shared memory in installations
**	where servers share common buffer managers.
**
**	This control block and corresponding buffer descriptors and pages
**	are referenced through the Local Buffer Manager Control Block.
**
** History:
**     06-mar-1996 (stial01)
**          Created to support of multiple caches in multiple memory segments.
**	08-Jul-1999 (jenjo02)
**	    Replaced bms_tblpriority, unused for a long time, with
**	    bms_cp_array.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define BMSCB_CB as DM_BMSCB_CB.
*/
struct _DMP_BMSCB
{
    i4	    	    *bms_1_reserved;	    /* Not used. */
    i4	    	    *bms_2_reserved;	    /* Not used. */
    SIZE_TYPE	    bms_length;  	    /* Length of control block. */
    i2              bms_type;		    /* Type of control block for
					    ** memory manager. */
#define              BMSCB_CB               DM_BMSCB_CB
    i2              bms_s_reserved;         /* Reserved for future use. */
    PTR             bms_l_reserved;         /* Reserved for future use. */
    PTR             bms_owner;              /* Owner of control block for
					    ** memory manager.  BMSCB will be
					    ** owned by the server. */
    i4         	    bms_ascii_id;           /* Ascii identifier for aiding
					    ** in debugging. */
#define              BMSCB_ASCII_ID       CV_C_CONST_MACRO('B', 'M', 'S', 'C')
    DB_NAME	    bms_name;		    /* Buffer manager name.         */
    SIZE_TYPE       bms_bmcb[DM_MAX_CACHE]; /* offset to DM0P_BMCB(s) */
    SIZE_TYPE       bms_bm_common;          /* offset to DM0P_BM_COMMON */
					    /*     (If allocated)           */
    SIZE_TYPE       bms_dbcache;            /* offset to dbcache            */
					    /*     (If allocated)           */
    SIZE_TYPE       bms_tblcache;           /* offset to tblcache           */
					    /*     (If allocated)           */
    SIZE_TYPE       bms_segsize;            /* Size of this piece of bm mem */
};


/*}
** Name: DM0P_CFA - Cache Flush Agent
**
** Description:
**
**	This structure is passed to a Cache Flush Agent to define
**	the task(s) it is to perform.
**
** History:
**	31-Jan-2003 (jenjo02)
**	    Created.
*/
struct _DM0P_CFA
{
    i4	    	    cfa_cache_ix;	    /* Assigned cache */
    i4		    cfa_id;		    /* Cache agent id */
    i4		    cfa_type;		    /* Agent type */
#define		CFA_PRIMARY	    0  	    /* Primary agent */
#define		CFA_CLONE	    1  	    /* Cloned agent */
    i4		    cfa_tasks;		    /* Assigned task(s): */
#define		CFA_SINGLE	    0x0001  /* Flush single pages */
#define		CFA_GROUP	    0x0002  /* Flush groups */
#define		CFA_CP	    	    0x0004  /* Assist CP */
    i4		    cfa_CPflushed;	    /* CP buffers flushed */
    i4		    cfa_CPstart;	    /* Starting CP buffer */
    u_i4    	    cfa_CPcount;	    /* Which CP */
    i4		    cfa_Sflushed;	    /* Single buffers flushed */
    i4		    cfa_Spflushed[DB_MAX_TABLEPRI];
    i4		    cfa_Gflushed;	    /* Groups flushed */
    i4		    cfa_Gpflushed[DB_MAX_TABLEPRI];
};

FUNC_EXTERN VOID dmd_summary_statistics(
			DM0P_BSTAT 	*bs,
			i4 		pgsize,
			i4 		indent);
