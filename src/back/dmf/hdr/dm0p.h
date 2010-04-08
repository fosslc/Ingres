/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/

/**
** Name: DM0P.H - This defines the interface to the buffer manager.
**
** Description:
**      This file describes the interface to the buffer manager.  The
**	structures and constants and functions needed to perform buffer
**	requests are described.
**
** History:
**      22-oct-1985 (derek)
**          Created new for Jupiter.
**	 5-oct-1988 (rogerk)
**	    Added DM0P_NOWAIT flag for dm0p_fix_page calls.
**	28-feb-1989 (rogerk)
**	    Added support for shared buffer manager.
**	    Added DM0P_SHARED_BUFMGR flag for dm0p_allocate calls.
**	    Added flags for dm0p_close_page, dm0p_dbache, dm0p_tblcache routines
**	    Added buffer manager event types.
**	    Added extern defines for new routines.
**	25-sep-1989 (rogerk)
**	    Added extern defines for new routines - dm0p_pass_abort and
**	    dm0p_recover_state.
**	6-Oct-1989 (ac)
**	    Added DM0P_MASTER to create the buffer manager lock list as master
**	    lock list for RCP and CSP's buffer managers.
**	14-aug-1990 (rogerk)
**	    Added extern definition for dm0p_set_valid_stamp routine.
**	29-apr-1991 (Derek)
**	    Added new constants for dm0p_fix_page/dm0p_unfix_page for
**	    allocation project. DM0P_NOLOCK, DM0P_FMFHSYNC and DM0P_MUTEX in
**	    fix and DM0P_MRELEASE in unfix.  See dm0p.c for usage.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1990 (rogerk)
**		Added support for fixed page priorites.
**		Added definition of DM0P_NOPRIORITY to indicate no priority set.
**	19-Nov-1991 (rmuth)
**	    Removed DM0P_FHFMSYNC, used by dm0p_fix_page.
**	07-apr-1992 (jnash)
**	    Add DM0P_LOCK_CACHE to dm0p_allocate flags param to 
**	    permit locking shared cache.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project:
**	      - Added prototypes and flag constants for new BM routines.
**            - Add prototypes for checksum routines: dm0p_insert_checksum,
**		dm0p_validate_checksum and dm0p_compute_checksum.
**            - Removed DM0P_MUTEX and DM0P_MRELEASE options to fix/unfix.
**	      - Changed unfix flags to be bit flags that can now be or'd
**		together for dm0p_uncachefix_page operations.
**	      - Added DM0P_TOSS flag to dm0p_force_pages options and removed
**		archaic DM0P_INV_FORCE flag.
**	      - Removed dm0p_tran_invalidate and dm0p_invalidate routines.
**	08-nov-1992 (kwatts) change for 6.4 bug fix: 18-nov-91 (kwatts)
**	    Added DM0P_CLFORCE as new dm0p_close_page action for Smart Disk.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Added dm0p_pagetype call.
**	03-Mar-1993 (keving)
**	    Added dm0p_register_loghandles and have_log arg to dm0p_allocate
**	    for Cluster recovery
**	24-may-1993 (rogerk)
**	    Added dm0p_1toss_page call for recovery testing.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    Instead of calling before_image_check on every dm0p_fixpage 
**	    operation, we now expect the modifier of the page to call 
**	    dm0p_before_image_check before changing its contents.
**	    Made before_image_check external and added dm0p_ prefix.
**	18-oct-1993 (rmuth)
**	    Add prototype for dm0p_mo_init().
**	24-may-1994 (andys)
**	    Add DM0P_USER_TID which is used to indicate to dm0p_fix_page 
**	    not to log errors if this fix_page fails, as this may be a 
**	    get by a user specified tid. [bug 49726]
**	09-may-1995 (cohmi01)
**	    Add defines, functions for IOMASTER server and prefetch logic.   
**	07-jun-1995 (cohmi01)
**	    Prefetch structures moved to dmp.h
**	29-feb-96 (nick)
**	    Added DM0P_FCWAIT_EVENT.
**	30-may-1996 (pchang)
**	    Added DM0P_NOESCALATE action for dm0p_fix_page(), to be used by
**	    session already holding an X lock on the FHDR when fixing pages
**	    other than an FMAP (i.e. without the DM0P_NOLOCK action).  Lock 
**	    escalation in such situation may lead to unrecoverable deadlock 
**	    involving the table-level lock and the FHDR. (B76463)
**	    Hence, any routine that is called with the FHDR already fixed
**	    should use this flag when fixing non-FMAP pages.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument to format_routine sub-prototype of
**		dm0p_cachefix_page prototype.
**      06-mar-1996 (stial01)
**          Added DM0P_NEWSEG, DM0P_DEFER_PGALLOC
**          changed DM0P_FWAIT_EVENT
**	20-mar-1996 (nanpr01)
**	    Added the page_size parameter for checksum routines for
**	    Variable page size project.
**      06-may-1996 (nanpr01)
**	    Page header access macro needs the page size for New Page Format
**	    project. 
**	26-jul-96 (nick)
**	    Add dm0p_trans_page_lock(). Remove declaration of static function
**	    dm2r_signal_prefetch() which a) doesn't exist and b) doesn't 
**	    belong here anyway.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Added DM0P_DMCM to dm0p_allocate flags parameter to support
**          the Distributed Multi-Cache Management (DMCM) protocol.
**          Added FUNC_EXTERN dm0p_dmcm_callback, the routine passed as a  
**          parameter to LKrequestWithCallback - to support DMCM - that is 
**          called when locking contentions occur.
**          Added FUNC EXTERN dm0p_count_pages; used to check that no dirty
**          pages for a table are left in the cache after its TCB has been
**          flushed.
**      01-aug-1996 (nanpr01)
**	    Added lock_key parameter since lockid may not be enough in
**	    the call back routine to identify the page.
**	30-Aug-1996 (jenjo02)
**	    Mutex granularity project. Added mode to dm0p_mlock().
**	    Added dm0p_minit(), dm0p_match() prototypes.
**	    Moved dm0p_mlock(), dm0p_munlock(), dm0p_minit(), dm0p_match()
**	    prototypes to dm0p.c, the only module in which they're used.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          - add lock_type argument to dm0p_lock_page() and dm0p_unlock_page();
**          - add DM0P_PHANPRO and DM0P_X_LATCH to support phantom protection
**          - changed arguments of dm0p_trans_page_lock()
**	05-Feb-1997 (jenjo02)
**	    Added DM0P_GWAIT_EVENT for busy group waits.
**      26-feb-97 (dilma04)
**          Add DM0P_LK_CONVERT action flag for dm0p_trans_page_lock() call
**          to convert a page lock from physical to logical mode.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Removed dm0p_stbl_priority(), dm0p_gtbl_priority() function
**	    prototypes.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          add lock_id argument to dm0p_lock_page(), dm0p_unlock_page()
**          and dm0p_trans_page_lock() function prototypes.
**      21-May-1997 (stial01)
**          Renamed DM0P_X_LATCH -> DM0P_IX_LOCK. This flag is specified when
**          the page has been locked with an IX page lock.
**          Added flags arg to dm0p_unmutex prototype.
**          Added dm0p_lock_buf_macro and dm0p_unlock_buf_macro.
**      29-May-1997 (stial01)
**          Rename DM0P_IX_LOCK -> DM0P_INTENT_LK
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to prototypes for dm0p_close_page(),
**	    dm0p_toss_pages(), dm0p_wpass_abort(), dm0p_tblcache().
**	03-Sep-1997 (jenjo02)
**	    Cross-integration of 427407. page_number in dm0p_1toss_page(),
**	    dm0p_trans_page_lock(), dm0p_latch_page() should be DM_PAGENO
**	    (u_i4), not i4.
**	13-May-1998 (jenjo02)
**	    Changes for per-cache, dynamic WriteBehind threads.
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page(), dm0p_unlock_page(),
**	    used by FHDR/FMAPs.
**	    Added DM0P_CREATE_COND flag for dm0p_fix_page() for those special
**	    FHDR/FMAP condition in which an unformatted page is read and it's
**	    ok.
**	20-Jul-1998 (jenjo02)
**	    Repaired define of DM0P_WBFLUSH_EVENT, which was 1 instead of
**	    (1 * DM0P_EVENT_SHIFT).
**	12-aug-1998 (somsa01)
**	    Added dm0p_alloc_pgarray() definition.
**	01-Sep-1998 (jenjo02)
**	    Renamed DM0P_CREATE_COND to DM0P_CREATE_NOFMT to signify its
**	    new use.
**      22-sep-98 (stial01)
**          Added flags to dm0p_escalate prototype (B92909)
**	29-Oct-1998 (jenjo02)
**	    Added DM0P_PHYSICAL_IX flag.
**	30-Aug-1999 (jenjo02)
**	    Added (internal) DM0P_CACHE_ALLOCATED flag for dm0p_allocate().
**	11-Oct-1999 (jenjo02)
**	    Added new external functions, dm0p_lock_buf(), dm0p_unlock_buf(),
**	    changed dm0p_lock|unlock_buf_macro to call these functions.
**      22-May-2000 (islsa01)
**          Declare dm0p_check_logfull to EXTERN. (B90402)
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**	08-Sep-2000 (jenjo02)
**	    Removed dm0p_compute_checksum() prototype. It's a
**	    static function defined in dm0p.c.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	31-Jan-2003 (jenjo02)
**	    Modified prototypes for dm0p_flush_pages(), 
**	    dm0p_wbflush_wait() to accomodate Cache Flush Agents.
**      28-may-2002 (stial01)
**          Renamed DM0P_ESCALATE -> DM0P_IGNORE_LKLIMIT
**      09-Nov-2006 (horda03)
**          Added DM0P_NOT_MATERIALIZED flag for fix_tabio_ptr
**           to indicate tabio not required if table isn't materialized.
**      19-Jun-2002 (horda03) Bug 108074
**          Only need to flush FMAP/FHDR (and other SCONCUR) pages to
**          disk if there is no exclusive table level lock against the
**          table. If we flush these pages to disk during a FORCE_ABORT
**          the likely result is a full TX loog file, as there can be
**          a significant amount of unused space in the Log buffers
**          written to disk. Added a new action DM0P_TABLE_LOCKED_X, to
**          indicate that a table is locked Exclusively.
**	29-Dec-2003 (jenjo02)
**	    Added DM0P_RCB_RELTID flag to pass to dm0p_trans_page_lock()
**	    when intent locking the data page in a global index.
**	11-Nov-2005 (jenjo02)
**	    Delete prototype for dm0p_check_logfull(); it's again a
**	    static in dm0p.c
**	6-Jan-2006 (kschendel)
**	    Update dm0p-allocate params, add connect-only, remove buckets.
**	28-Feb-2007 (jonj)
**	    Add DM0P_NOLKSTALL flag for dm0p_cachefix_page.
**	06-Mar-2007 (jonj)
**	    Add redo_lsn to dm0p_cachefix_page prototype for Cluster REDO.
**	21-May-2007 (kschendel) b122121
**	    Add routine to note CP thread arrivals (or departures).
**	11-Oct-2007 (jonj)
**	    Add DM0P_CSP_REDO for online/offline CSP REDO.
**	08-Jan-2008 (kibro01) b119663
**	    Add dm0p_set_lg_force
**      14-jan-2008 (stial01)
**          Added prototype for dm0p_unfix_invalid_table()
**      28-jan-2008 (stial01)
**          Changed prototype for dm0p_unfix_invalid_table()
**	20-may-2008 (joea)
**	    Add relowner parameter to dm0p_lock_page.
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	05-jun-2009 (thaju02) B118454
**	    Add lock_list param to dm0p_set_valid_stamp prototype.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: 
**	    Add DM0P_MVCC flag for dm0p_cachefix_page, DM0P_UNFIX_CR
**	    for dm0p_uncache_fix.
**	    Replace DMPP_PAGE **page parameter with DMP_PINFO *pinfo
**	    in dm0p_fix_page, dm0p_unfix_page, dm0p_cachefix_page,
**	    dm0p_uncache_fix.
**	    Replace functions dm0p_lock_buf, dm0p_unlock_buf, dm0p_mutex,
**	    dm0p_unmutex with dm0pPinBuf, dm0pUnpinBuf, dm0pMutex,
**	    dm0pUnmutex.
**	    New macros, dm0pLockBufForGet, dm0pLockBufForUpd,
**	    dm0pLockRootForGet, dm0pUnlockBuf.
**	    Removed obsolete prototype for dm0p_latch_page()
**	    Removed dm0p_page_lock_mode, static in dm0p.c
**	01-Feb-2010 (jonj)
**	    SIR 121619 MVCC: 
**	    Changed dm0pLockBufForGet/Upd, dm0pUnlockBuf macros
**	    to "if" instead of conditional statements, which Solaris
**	    didn't like.
**/

/*
**  Defines of other constants.
*/

/*
**     Constants used as action values for dm0p_fix_page, dm0p_unfix_page,
**     dm0p_cachefix_page, dm0p_uncachefix_page and dm0p_trans_page_lock calls.
*/

#define		DM0P_READ		0x000000L
#define		DM0P_UNFIX		0x000000L
#define		DM0P_PHYSICAL		0x000001L
#define		DM0P_WRITE		0x000002L
#define		DM0P_PHANPRO		0x000004L
#define		DM0P_CREATE		0x000008L
#define		DM0P_NOREADAHEAD	0x000010L
#define		DM0P_LOCKREAD		0x000020L
#define		DM0P_TEMPLOCK		0x000040L
#define		DM0P_NOWAIT		0x000080L
#define		DM0P_NOLOCK		0x000100L
#define		DM0P_READAHEAD		0x000200L
#define		DM0P_NOESCALATE		0x000400L
#define		DM0P_READNOLOCK		0x000800L
#define		DM0P_FORCE		0x001000L
#define		DM0P_RELEASE		0x002000L
#define		DM0P_USER_TID		0x004000L
#define		DM0P_HI_PRIORITY	0x008000L
#define		DM0P_INTENT_LK		0x010000L
#define		DM0P_LK_CONVERT		0x020000L
#define		DM0P_CREATE_NOFMT	0x040000L
#define		DM0P_PHYSICAL_IX	0x080000L
#define		DM0P_IGNORE_LKLIMIT	0x100000L
#define         DM0P_TABLE_LOCKED_X	0x200000L
#define         DM0P_RCB_RELTID		0x400000L
#define         DM0P_NOLKSTALL		0x800000L
#define         DM0P_CSP_REDO		0x1000000L
#define         DM0P_MVCC		0x2000000L	/* Fix with MVCC protocols,
							** no page locks, 
							** (internal use only) */
#define         DM0P_UNFIX_CR		0x4000000L	/* Unfix the CR page only,
							** leave Root fixed */

#define		DM0P_RPHYS	(DM0P_READ | DM0P_PHYSICAL)
#define		DM0P_WPHYS	(DM0P_WRITE | DM0P_PHYSICAL)
#define		DM0P_SCRATCH	(DM0P_WRITE | DM0P_CREATE)

/*
**      Values used as action constants to dm0p_close_page.
*/

#define                 DM0P_CLOSE	1L
#define                 DM0P_CLCACHE	2L
#define                 DM0P_CLFORCE	3L

/*
**      Values used as action constants to dm0p_force_pages.
*/
/*  #define		DM0P_FORCE	0x1000   defined above */
/*  #define		DM0P_TOSS	3L       defined below */

/*
**      Values used as action constants to dm0p_allocate.
*/
#define			DM0P_SHARED_BUFMGR	0x0001L
#define			DM0P_MASTER		0x0002L
#define			DM0P_LOCK_CACHE		0x0004L
#define			DM0P_IOMASTER   	0x0008L	/* I/O server */
#define                 DM0P_DMCM               0x0010L /* ICL phil.p */

/*
**	Action values used for dm0p cache routines (dm0p_dbcache, dm0p_tblcache)
*/
#define			DM0P_VERIFY		1
#define			DM0P_MODIFY		2
#define			DM0P_TOSS		3
#define			DM0P_TOSS_IGNORE	4 /* ignore errors */

/*
**	Event constants used in DM0P event wait/signal routines.
*/
#define			DM0P_EVENT_SHIFT	0x10000
#define			DM0P_WBFLUSH_EVENT	(1 * DM0P_EVENT_SHIFT)
#define			DM0P_FWAIT_EVENT	(2 * DM0P_EVENT_SHIFT)
#define			DM0P_BIO_EVENT		(3 * DM0P_EVENT_SHIFT)
#define			DM0P_PGMUTEX_EVENT	(4 * DM0P_EVENT_SHIFT)
#define			DM0P_BMCWAIT_EVENT	(5 * DM0P_EVENT_SHIFT)
#define			DM0P_FCWAIT_EVENT	(6 * DM0P_EVENT_SHIFT)
#define			DM0P_GWAIT_EVENT	(7 * DM0P_EVENT_SHIFT)

/*
**	Buffer Manager priorities used by clients of the buffer manager to
**	indicate how pages should be weighted in the cache.
*/
#define			DM0P_NOPRIORITY		-1

/*
**	Flags to Table IO get/build routines.
*/
#define			DM0P_TBIO_WAIT		0x01
#define			DM0P_TBIO_MUTEX_HELD	0x02
#define			DM0P_TBIO_NOPARTIAL	0x04
#define  		DM0P_TBIO_BUILD   	0x08	/* build if not there */
#define 		DM0P_NOT_MATERIALIZED   0x10

/*
**	Constants for indicating what to do during a write_along interval
*/
#define			DM0P_WA_SINGLE		0x01  /* write single bufs */
#define			DM0P_WA_GROUPS		0x02  /* write group bufs */


/*
** Flags for dm0p_allocate() cache_flags parameter 
*/
#define DM0P_NEWSEG          0x01 /* Alloc cache in its own memory segment */ 
#define DM0P_DEFER_PGALLOC   0x02 /* Defer allocation of page array */
#define DM0P_CACHE_ALLOCATED 0x04 /* Cache has been allocated (internal use) */


/*
** Flags for dm0pMutex(), dm0pUnmutex flags parameter
*/
#define DM0P_MUNPIN		0x01	/* dm0pUnmutex only, release pinlocks */
#define DM0P_MINCONS_PG		0x02	/* Might be Inconsistent (dm1p) */

/* For dm0pPinBuf, get exclusive pin lock */
#define DM0P_MPINX		0x80000	/* dm0pPinBuf only */


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dm0p_allocate(
		i4		flags,
		bool		connect_only,
		i4		buffers[DM_MAX_CACHE],
		i4		free_limit[DM_MAX_CACHE],
		i4		modify_limit[DM_MAX_CACHE],
		i4		group[DM_MAX_CACHE],
		i4		gpages[DM_MAX_CACHE],
		i4		writebehind[DM_MAX_CACHE],
		i4		wb_start[DM_MAX_CACHE],
		i4		wb_end[DM_MAX_CACHE],
		i4		cache_flags[DM_MAX_CACHE],
		i4		dbc_size,
		i4		tbc_size,
		char		*cache_name,
		DB_ERROR	*dberr);

FUNC_EXTERN STATUS dm0p_start_wb_threads(void);

FUNC_EXTERN i4   dm0p_bmid(void);

FUNC_EXTERN DB_STATUS dm0p_cachefix_page(
		DMP_TABLE_IO	    *tbio,
		DM_PAGENO	    page_number,
		i4		    access_mode,
		i4		    action,
		i4		    lock_list,
		i4		    log_id,
		DB_TRAN_ID	    *tran_id,
		DMPP_PAGE	    *readnolock_buffer,
		VOID		    (*format_routine)(
					i4		page_type,
					i4		page_size,
					DMPP_PAGE       *page,
					DM_PAGENO       pageno,
					i4              stat,
					i4		fill_option),
		DMP_PINFO	    *pinfo,
		LG_LSN		    *redo_lsn,
		DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0p_close_page(
		DMP_TCB		    *tcb,
		i4		    lock_list,
		i4		    log_id,
		i4		    action,
		DB_ERROR	    *dberr);

FUNC_EXTERN VOID      dm0p_count_connections(void);

FUNC_EXTERN VOID      dm0p_cpdone(void);

FUNC_EXTERN DB_STATUS dm0p_cp_flush(
		i4		lock_list,
		i4		log_id,
		DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS dm0p_dbcache(
		i4	    dbid,
		i4	    open,
		i4	    write_mode,
		i4	    lock_list,
		DB_ERROR    *dberr);

FUNC_EXTERN DB_STATUS dm0p_deallocate(DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm0p_escalate(
		DMP_RCB            *rcb,
		i4            flags,
		DB_ERROR           *dberr);

FUNC_EXTERN DB_STATUS dm0p_fix_page(
		DMP_RCB            *rcb,
		DM_PAGENO          page_number,
		i4		   action,
		DMP_PINFO          *pinfo,
		DB_ERROR           *dberr);

FUNC_EXTERN DB_STATUS dm0p_flush_pages(
		i4	    lock_list,
		i4     log_id,
		char	    *cfa,
		DB_ERROR           *dberr);

FUNC_EXTERN DB_STATUS dm0p_force_pages(
		i4		    lock_list,
		DB_TRAN_ID          *tran_id,
		i4		    lx_id,
		i4		    action,
		DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS dm0p_lock_page(
		i4		    lock_list,
		DMP_DCB		    *dcb,
		DB_TAB_ID	    *table_id,
		DM_PAGENO	    page_number,
                i4             lock_type,
		i4		    lock_mode,
		i4		    lock_action,
		i4		    lock_timeout,
		DB_TAB_NAME	    *relid,
		DB_OWN_NAME	    *relowner,
		DB_TRAN_ID	    *tran_id,
                LK_LKID             *lock_id,
		i4		    *new_lock,
		LK_VALUE	    *lock_value,
		DB_ERROR            *dberr);

FUNC_EXTERN void dm0p_note_cpthread(bool adding);

FUNC_EXTERN VOID      dm0p_pass_abort(i4 recovery_started);

FUNC_EXTERN bool      dm0p_recover_state(i4 lg_id);

FUNC_EXTERN DB_STATUS	dm0p_register_loghandles(
		DMP_LCTX	    **lctx_array,
		i4		    num_logs,
		DB_ERROR            *dberr);

FUNC_EXTERN DB_STATUS dm0p_reparent_pages(
		DMP_TCB	    *old_parent,
		DMP_TCB	    *new_parent,
		DB_ERROR    *dberr);

FUNC_EXTERN VOID      dm0p_set_valid_stamp(
		i4	    db_id,
		i4	    table_id,
		i4     page_size,
		i4	    validation_stamp,
		i4	    lock_list);

FUNC_EXTERN DB_STATUS dm0p_tblcache(
		i4	    dbid,
		i4	    tabid,
		i4	    action,
		i4	    lock_list,
		i4	    log_id,
		DB_ERROR    *dberr);

FUNC_EXTERN VOID      dm0p_toss_pages(
		i4	    dbid,
		i4	    tabid,
		i4	    lock_list,
		i4	    log_id,
		i4	    toss_modified);

FUNC_EXTERN VOID      dm0p_1toss_page(
		i4	    dbid,
		DB_TAB_ID   *table_id,
		DM_PAGENO   page_number,
		i4	    lock_list);

FUNC_EXTERN DB_STATUS dm0p_uncache_fix(
		DMP_TABLE_IO    *tbio,
		i4         action,
		i4         lock_list,
		i4         log_id,
		DB_TRAN_ID      *tran_id,
		DMP_PINFO       *pinfo,
		DB_ERROR   	*dberr);

FUNC_EXTERN DB_STATUS dm0p_unfix_page(
		DMP_RCB		   *rcb,
		i4            action,
		DMP_PINFO          *pinfo,
		DB_ERROR   	   *dberr);

FUNC_EXTERN DB_STATUS dm0p_unlock_page(
		i4		    lock_list,
		DMP_DCB		    *dcb,
		DB_TAB_ID	    *table_id,
		DM_PAGENO	    page_number,
                i4             lock_type,
		DB_TAB_NAME	    *relid,
		DB_TRAN_ID	    *tran_id,
                LK_LKID             *lock_id,
		LK_VALUE	    *lock_value,
		DB_ERROR   	    *dberr);

FUNC_EXTERN DB_STATUS dm0p_wbflush_wait(
		char	    *cfa,
		i4	    lock_list,
		DB_ERROR    *dberr);

FUNC_EXTERN VOID      dm0p_wpass_abort(
		i4	    dbid,
		i4	    lock_list,
		i4	    log_id);

FUNC_EXTERN VOID dm0p_insert_checksum(
		DMPP_PAGE       *page,
		i4              relpgtype,
		DM_PAGESIZE	page_size);

FUNC_EXTERN DB_STATUS dm0p_validate_checksum(
		DMPP_PAGE       *page,
		i4		relpgtype,
		DM_PAGESIZE	page_size);

FUNC_EXTERN VOID dm0p_pagetype(
		DMP_TABLE_IO	*tbio,
		DMPP_PAGE       *page,
		i4		log_id,
		i4		page_type);

FUNC_EXTERN DB_STATUS dm0p_before_image_check(
		DMP_RCB         *rcb,
		DMPP_PAGE       *page,
		DB_ERROR        *dberr);

FUNC_EXTERN STATUS dm0p_mo_init( 
	        DMP_LBMCB *lbmcb,
	        DM0P_BM_COMMON  *bm_common );

FUNC_EXTERN DB_STATUS dm0p_write_along(
		i4     lock_list,
		i4     log_id,
		i4     *num_flushes,
		i4     duties,
		DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm0p_trans_page_lock(
    		DMP_RCB	*rcb,
		DMP_TCB *tcb,
    		DM_PAGENO page_number,
    		i4	action,
    		i4	lock_mode,
    		i4	*lock_action,
                LK_LKID *lock_id,
		DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm0p_dmcm_callback(
                i4              lock_mode,
                PTR             ptr,
                LK_LKID         *lockid,
		LK_LOCK_KEY     *lock_key);

FUNC_EXTERN i4 dm0p_count_pages(
                  i4        db_id,
                  i4        table_id);

FUNC_EXTERN DB_STATUS dm0p_alloc_pgarray(
		i4    page_size,
		DB_ERROR   *dberr);

FUNC_EXTERN bool dm0p_set_lg_force(
		DMP_TABLE_IO	*tbio,
		DMP_RCB		*rcb);

FUNC_EXTERN VOID dm0p_unfix_invalid_table(
		DMP_DCB		*dcb,
		DB_TAB_ID	*tab_id,
		i4		flags);
#define DM0P_INVALID_UNFIX		0x01
#define DM0P_INVALID_UNLOCK		0x02
#define	DM0P_INVALID_UNMODIFY		0x04

FUNC_EXTERN VOID      dm0pPinBuf(
		DMP_TABLE_IO	    *tbio,
		i4		    flags,
		i4		    lock_list,
		DMP_PINFO           *pinfo);

FUNC_EXTERN VOID      dm0pUnpinBuf(
		DMP_TABLE_IO	    *tbio,
		i4		    lock_list,
		DMP_PINFO           *pinfo);

FUNC_EXTERN VOID      dm0pMutex(
		DMP_TABLE_IO	    *tbio,
		i4             	    flags,
		i4		    lock_list,
		DMP_PINFO           *pinfo);

FUNC_EXTERN VOID      dm0pUnmutex(
		DMP_TABLE_IO	    *tbio,
		i4             	    flags,
		i4		    lock_list,
		DMP_PINFO           *pinfo);

FUNC_EXTERN DB_STATUS dm0pMakeCRpage(
		DMP_RCB		*rcb,
		DMP_PINFO	*pinfo,
		DB_ERROR	*dberr);

/*
** Read-share lock a buffer/page for consistent read.
** If pinfo->page is a CR page, the page is not pinned.
** If the (root) page is inconsistent with the CRIB, materialize a
** CR page, overlaying pinfo->page with its pointer; neither the CR
** nor Root page will be pinned.
** If the Root page is consistent with the CRIB,
** it will be pinned.
**
** NB: this macro requires
**
**	#include <dmccb.h> (required by dml.h)
**	#include <dmxcb.h> (required by dml.h)
**	#include <dml.h>
*/
#define dm0pLockBufForGet(r, pinfo, status, dberr) \
    if ( row_locking(r) || (crow_locking(r) && \
         !DMPP_VPT_IS_CR_PAGE(r->rcb_tcb_ptr->tcb_rel.relpgtype, \
	 	((DMP_PINFO*)pinfo)->page)) ) \
    { \
        dm0pPinBuf(&r->rcb_tcb_ptr->tcb_table_io, 0, r->rcb_lk_id, \
			(DMP_PINFO*)pinfo); \
	*status = E_DB_OK; \
	if ( RootPageIsInconsistent(r, (DMP_PINFO*)pinfo) ) \
	    *status = dm0pMakeCRpage(r, (DMP_PINFO*)pinfo, dberr); \
    } \
    else \
	*status = E_DB_OK;

/*
** Read-excl lock a buffer.
** If pinfo->page is a CR page, dm0pPinBuf will swap pinfo->page to Root
** and save the CR page pointer in pinfo->CRpage.
*/
#define dm0pLockBufForUpd(r, pinfo) \
    if ( row_locking(r) || crow_locking(r) ) \
	dm0pPinBuf(&r->rcb_tcb_ptr->tcb_table_io, DM0P_MPINX, r->rcb_lk_id, \
			(DMP_PINFO*)pinfo);

/*
** Read-share lock a Root buffer/page for read.
** If pinfo->page is a CR page, dm0pPinBuf will swap pinfo->page to Root
** and save the CR page pointer in pinfo->CRpage.
*/
#define dm0pLockRootForGet(r, pinfo) \
     if ( row_locking(r) || crow_locking(r) ) \
	dm0pPinBuf(&r->rcb_tcb_ptr->tcb_table_io, 0, r->rcb_lk_id, \
			(DMP_PINFO*)pinfo);

/*
** Release a read-share/excl lock if pinfo->pincount is not zero.
** The pin count will be reduced by 1
** pinfo->page may be restored from pinfo->CRpage.
*/
#define dm0pUnlockBuf(r, pinfo) \
     if ( ((DMP_PINFO*)pinfo)->pincount ) \
	dm0pUnpinBuf(&r->rcb_tcb_ptr->tcb_table_io, r->rcb_lk_id, \
			(DMP_PINFO*)pinfo);

/*
** Macro to test if buffer is locked (pincount != 0)
*/
#define dm0pBufIsLocked(pinfo) \
    (((DMP_PINFO*)pinfo)->pincount) 

/*
** Macro to test if a buffer lock is needed on pinfo->page
**
** TRUE if row locking, or crow locking and pinfo->page is a Root page,
** FALSE otherwise.
*/
#define dm0pNeedBufLock(r, pinfo) \
    (row_locking(r) || \
     (crow_locking(r) && \
      !DMPP_VPT_IS_CR_PAGE(r->rcb_tcb_ptr->tcb_rel.relpgtype, \
      	((DMP_PINFO*)pinfo)->page)))
