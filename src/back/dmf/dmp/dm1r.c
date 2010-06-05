/*
** Copyright (c) 1985, 2010 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <mh.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <tm.h>
#include    <lk.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dmpl.h>
#include    <dm1c.h>
#include    <dm1h.h>
#include    <dm1p.h>
#include    <dm1r.h>
#include    <dm2f.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dma.h>
#include	<dmccb.h>
#include	<dmxcb.h>
#include    <dml.h>
#include    <lkdef.h>
#include    <dmxe.h>
#include	<dmpepcb.h>
#include    <dmve.h>
#include    <dmfcrypt.h>

/**
**
**  Name: DM1R.C - Routines to access and update SEQ, HASH, and ISAM tables.
**
**  Description:
**      This file contains all the common routines needed to access and
**      update SEQuential, HASH, and ISAM tables. 
**
**      The routines defined in this file are:
**      dm1r_delete         - Deletes a record from a table.
**      dm1r_get            - Gets a record from a table. 
**      dm1r_put            - Puts a record to a table.
**      dm1r_rcb_update     - Update RCB's to track deletes and replaces.
**      dm1r_replace        - Replaces a record.
**	dm1r_count	    - Count tuples in anything other than a btree
**	dm1r_cvt_row	    - Convert tuple row to current row_version format
**			      for pages that contain row_version and column in
**			      the row has been added or dropped. 
**	dm1r_lock_row	    - Request the row-level lock for user table.
**      dm1r_unlock_row     - Release the row-level lock for user table.
**	dm1r_lock_value     - Request the key-value lock for user table.
**	dm1r_unlock_value   - Release the key-value lock for user table.
**	dm1r_crow_lock      - Acquire a Consistent Read row lock.
**
**  History:
**      12-dec-85 (jennifer)
**          Converted for Jupiter.
**	30-sep-1988 (rogerk)
**	    Add support for new Hash Overflow algorithms.  If we delete a
**	    tuple from a hash overflow chain, make sure that the primary
**	    page's DMPP_FULLCHAIN bit is turned off.  Set rcb_hash_nofull
**	    each time we visit a primary page that does not have FULLCHAIN
**	    status set.
**	24-Apr-89 (anton)
**	    Added local collation support
**	29-may-89 (rogerk)
**	    Added checks for bad rows in dm1c_get calls.  Added trace flags
**	    to allow us to delete rows by tid that cannot be fetched.
**	29-sep-89 (paul)
**	    Modified dm1r_get to not reset rcb_lowtid (scan location) on
**	    multi-cursor updates. If two cursors updated the same tuple with
**	    the first update moving the row, the second cursor's current
**	    position (rcb_lowtid) would be moved to the new location of
**	    the row. This is the wrong thing to do.
**	22-jan-90 (rogerk)
**	    Make sure we send byte-aligned tuples to qualification function.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	18-dec-90 (jas)
**	    Smart Disk project integration:
**	    Force DMPP_DATA on when putting a tuple onto a page, to make
**	    sure that all data pages have DMPP_DATA set.
**	23-jul-91 (markg)
**	    Added fix for bug 38679 to dm1r_get where an incorrect error was
**	    being returned if there had been a failure while attempting to
**	    qualify a tuple.
**	14-jun-1991 (Derek)
**	    Changed arguments to changed DM1C_* functions for allocation
**	    project.
**	19-aug-91 (rogerk)
**	    Modified above fix in dm1r_get to not log internal errors when the 
**	    qualify function returns an error since we cannot distiguish
**	    between user errors and internal errors. When the USER_ERROR return
**	    value from the qualification function becomes supported, we
**	    should change back to logging internal error messages.
**	7-July-1992 (rmuth)
**	    DMF prototyping.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex calls.
**	26-oct-1992 (jnash) merged 3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	13-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Within dm1r_delete, request row lock for system catalog rows. 
**	      - Call tuple level vectors to perform compression operations,
**		rather than allowing the dm1c code to do it.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	05-nov-1992 (jnash)
**	    Temp hack to bypass locking until new recovery code in place. 
**	14-dec-1992 (rogerk & jnash)
**	    Reduced logging project.
**	      - Changed to follow new update protocols: dm1r put/del/rep
**		routines must now log their respective page updates.  Mutexes
**		must be taken on pages while logging and updating them.
**	      - Within dm1r_delete, request row lock for system catalog rows. 
**	      - Call tuple level vectors to perform compression operations,
**		rather than allowing the dm1c code to do it.
**	      - Reduced Logging Project: Changed arguments to dm0p_mutex and
**		dm0p_unmutex calls.
**	      - Removed calls to dmd_prrecord made with compressed rows.
**	      - Removed temporary use of rcb_recovery flag.
**	      - Added handling of journal status for catalog updates.  When a
**		core catalog is updated (by a put, delete or replace), the 
**		journal status of the log record depends on the journal state 
**		of the user table which is being altered, not the journal 
**		status of the actual system catalog.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-dec-92 (robf)
**	    Removed obsolete dm0a.h
**	05-jan-1993 (jnash)
**	    More reduced logging work.  Add LGreserve calls.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Extract row locking code into dm1r_lock_row(), so that cluster
**		    recovery code can more easily re-acquire row locks.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**       1-Jul-1993 (fred)
**          Added dm1c_pget() function where required to allow for the
**          rcb_f_qual function's being run on large objects.  This
**          change applies to dm1r_get().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Took out references to rcb_usertab_jnl which used to dictate
**	    whether or not to journal the system catalog update.
**	23-aug-1993 (rmuth)
**	    When getting a record from a table, if using tid access make
**	    sure page requested is not a FHDR or FMAP.
**      13-sep-93 (smc)
**          Commented lint pointer truncation warning.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Deletions of core
**	    catalog rows are always journaled; insertions of core catalog rows
**	    are never journaled; replaces on core catalog rows are sometimes
**	    journaled.  Full explanation of non journaled table handling 
**	    given in DMRFP.
**	18-oct-1993 (jnash)
**	    Include compression factor in replace log record LGreserve().
**	11-dec-1993 (rogerk)
**	    Changes to replace compression algorithms.  Added new parameters
**	    to the dm0l_repl_comp_info call and moved logic which determines
**	    when to do before image compression down into that routine.
**	23-may-1994 (andys)
**	    When fixing a page for a get by tid, pass the DM0P_USER_TID
**	    flag to dm0p_fix_page so that misleading errors will not be
**	    logged. An example of when this might happen is when
**	    optimizedb does a "create table mufc as select * from shme where 
**	    tid = -1" [bug 49726]
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to 
**          this change the routine was called once for LGreserve, and once for
**          dm0l_replace().
**	24-jun-1994 (mikem)
**	    Bug #63556
**	    Fix integration error introduced in 23-may integration.  A line
**	    of code was meant to be moved but the integration duplicated it.
**      04-apr-1995 (dilma04)
**          When moving from one page to the next, clear RCB_PAGE_HAD_RECORDS.
**      22-may-1995 (cohmi01)
**          Added dm1r_count() for agg optimization project.
**	29-jun-1995 (cohmi01)
**	    Use 'tuplecount' accessor routines in dm1r_count().
**	01-Aug-1995 (cohmi01)
**	    dm1r_get - If reading from secindx, and other criteria are met,
**	    request prefetch of data pages (of base table) pointed to by
**	    sec index.
**      05-sep-1995 (cohmi01)
**          Add dummy parms to dm1r_count() for compatibilty with dm2r_get().
**	11-jan-1995 (dougb)
**	    Remove debugging code which should not exist in generic files.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_getfree.
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_reclen.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**	11-jun-96 (nick)
**	    Fetch tuple size as well as tuple in dm1r_replace().
**	 1-jul-96 (nick)
**	    Lack of proper error handling in dm1r_count() caused server
**	    failure later on down the line. 
**	13-Aug-1996 (prida01)
**	    Give relatts correct types for calls to dm1c_p{*}
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass buffer for data
**	    uncompression.
**      01-nov-1996 (loera01)
**          In dm1r_cvt_row(), for variable-length datatypes, if no data
**          exists for the column, set the first two bytes to zero.  Also
**	    fixed handling of the null flag.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Renamed dm1r_lock_row -> dm1r_lock_sc_row for system catalog
**          Added new dm1r_lock_row 
**          Added dm1r_lock_value() for key value locking
**          Added dm1r_allocate()
**          Added get_unpack_lock()
**          Delete accessor: added reclaim_space 
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**      12-dec-96 (dilma04)
**          dm1r_lock_row: Remove setting of DM0P_PHYSICAL flag;  
**          dm1r_put: In case of non-unique hash table release value lock. 
**      20-jan-96 (stial01)
**          dm1r_lock_row, dm1r_lock_value: Log everything but a new lock status
**      22-jan-97 (dilma04)
**          - Fix bug 79961: release shared row locks if doing get by tid and
**          isolation level is read committed;
**          - Fix bug 80112: do not lock rows if doing get to update secondary
**          index;
**          - Add flag argument to get_unpack_lock().
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - test rcb_iso_level for isolation level;
**          - use macro to check for serializable transaction.
**      26-feb-97 (dilma04)
**          Change dm1r_get(), dm1r_lock_row() and get_unpack_lock() to support
**          lock escalation for row locking and lower isolation levels.
**      27-feb-1997 (stial01)
**          Changes to dm1r_allocate(): Test DMPP_CLEAN in page status to 
**          avoid the overhead of checking for deleted tuples unecessarily.
**          Prepare list of committed transactions and a bitmap of committed
**          deleted tuples. When LK_PH_PAGE locks are replaced with short
**          duration mutex, we release mutex while calling LG for transaction
**          status, during which time the space can be reclaimed by another.
**          transaction. 
**          Also if row locking, reserve allocated slot/space by putting
**          deleted tuple on the page.
**      10-mar-1997 (stial01)
**          dm1r_put() Changes for tables with blob columns, high compression
**          dm1r_lock_value() Pass rcb_tupbuf to dm1h_keyhash
**          dm1r_allocate() Added record arg, changes for blob/high compression
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - if row lock can be released before the end of transaction, set
**          rcb_release_row flag in dm1r_get();
**          - if rcb_release_row flag is set, release row lock in dm1r_lock_row
**          before requesting a new row lock; 
**          - removed checks for new_lock before calling dm1r_unlock_row();
**          - added support for releasing locks by lock id and for releasing
**            exclusive locks;
**          - if releasing row lock when scanning secondary index, release      
**          also intended lock from base table page. This fixes bug 80602.      
**      21-may-97 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      29-may-97 (stial01)
**          dm1r_get() Unlock buffer if user interrupt or force abort.
**      24-jun-97 (dilma04)
**          Bug 83300. Moved code to release redundant value lock from
**          dm1r_put() to dm1r_allocate(), since pointer to uncompressed
**          record is required for dm1r_unlock_value() call. Also, value 
**          lock should not be released from table "with noduplicates".
**      29-jul-97 (stial01)
**          dm1r_get() Apply key qualification before locking row (B84213)
**          Removed get_unpack_lock() Added get_record(), next()
**      29-jul-97 (stial01)
**          dm1r_put, dm1r_delete: Added opflag paramater
**      20-aug-97 (stial01)
**          dm1r_get() Init lock_err
**      29-sep-97 (stial01)
**          dm1r_lock_row() Get data page lock if btree and data page is 
**          not already fixed.
**          dm1r_lock_value() Escalate to table locking it this lock request
**          exceeded 'locks per transaction' limit;
**          dm1r_get() Don't issue E_DM9C2A_NOROWLOCK if row LK_CONVERT fails
**          Added dm1r_check_lock(), call this to check if row lock held.
**      30-oct-97 (stial01)
**          dm1r_get() Copy record into caller buffer BEFORE calling dm1c_pget.
**          Fixed another place where we copied tuple and didn't fix rec_ptr
**      12-nov-97 (stial01)
**          B87015: Output a lock trace message if tracing is enabled
**          Removed 29-sep-97 change to dm1r_lock_row.
**          Consolidated rcb fields into rcb_csrr_flags.
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**      28-may-98 (stial01)
**          Support VPS system catalogs. 
**          Moved page clean from dm1r_allocate to dm1r_clean and simplified.
**	23-Jun-1998 (jenjo02)
**	    Added LK_VALUE parm to dm0p_unlock_page() prototype.
**      07-jul-98 (stial01)
**          Deferred update: new update_mode if row locking
**          Defer row lock request for committed data (B84213)
**          Moved page clean from dm1r_allocate to dm1r_clean and simplified.
**	11-Aug-1998 (jenjo02)
**	    Count tuple adds and deletes in RCB, not TCB.
**      20-Jul-98 (wanya01)
**          dm1r_get() Copy rec to called buf BEFORE calling dm1c_pget.
**          This is addtional changes for b86862. The previous changes was
**          only made for routine when retrieving record by sequence, not
**          when retrieving record by TID.
**      06-oct-98 (stial01)
**          (B92468) Physical page locking for tcb_extended, set page tranid
**          when deleting, check page tranid before allocating.
**      12-nov-98 (stial01)
**          dm1r_rowlk_access() if (opflag & DM1C_GET_SI_UPD), no row locking
**      13-Nov-98 (stial01)
**          Renamed DM1C_SI_UPD->DM1C_GET_SI_UPD, no row locks if index update
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      10-dec-1998 (stial01)
**          Fix TRdisplay arguments
**      22-jan-1999 (schte01)
**          Changed variable named "try" to "tries" to avoid conflict with 
**          the #define for try in c_excpt.h
**	06-apr-1999 (somsa01)
**	    In dm1r_rowlk_access(), fixed typo where we would return without
**	    an error code; the error should be E_DB_OK.
**      28-may-1999 (stial01)
**          Only call dm1r_check_lock if DMZ_SES_MACRO(33)
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**      05-Oct-1998 (horda03)
**          B83046: Report E_DM0047 (Ambiguous replace) following only if the
**          record was obtained for modification (and it has previously been
**          modified by the transaction).
**      20-sep-99 (stial01)
**          dm1r_delete() If tcb_extended, just put tranid, don't set deferred
**      20-oct-99 (stial01)
**          Removed unecessary dm1r_rowlk_access before dm1c_pget
**      21-oct-1999 (stial01)
**          Fixed error handling after LKrelease (row/value)
**      21-oct-1999 (nanpr01)
**          More CPU optimization. Do no copy tuple header when we do not
**	    need them.
**	05-dec-99 (nanpr01)
**	    Optimized the code to reduce the tuple header copy from the page.
**      21-dec-1999 (stial01)
**          dm1r_allocate() for vps etabs
**      18-jab-2000 (gupsh01)     
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table.
**      31-jan-2000 (stial01)
**          To enforce phantom protection we need row lock for non-key quals.
**          (B100260). Also fixed locked coupling for CS/RR in dm1r_lock_row.
**      02-feb-2000 (stial01)
**          Additional changes for B100260 in dm1r_rowlk_access
**      09-feb-2000 (stial01)
**          DO NOT downgrade lock unless RCB_UPD_LOCK is set. (B100409)
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_put, dm0l_del, dm0l_rep.
**      05-jun-2000 (stial01)
**          Set DM1C_ROWLK in update flags when physical page locking (b101708)
**      25-may-2000 (stial01)
**          Delete dm1r_lock_sc_row, space is reserved with tranid       
**          Limited further when we should reclaim space during delete
**          Added compression type to put,delete,replace log records
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Changed args to dm1r_allocate, dmpp_allocate, dmpp_isnew
**          Moved space reclamation code to allocate page accessors
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_row_access() if neither
**	    C2 nor B1SECURE.
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**          dm1r_lock_row() Don't do any cursor related code if DM1R_ALLOC_TID
**	06-Feb-2001 (jenjo02)
**	    Pushed secondary index searches from dm2r (si_replace(),
**	    si_delete()) into dm1r_get() for efficiency.
**	    When row-locking by TID (dm1r_get()), record was being
**	    copied from page to caller's buffer twice.
**	    Consolidated code in dm1r_get().
**	19-may-2001 (somsa01)
**	    In dm1r_get(), fixed typo caused by cross-integrations.
**	22-May-2001 (hanje04)
**	    Removed crossing of fix for B83046 from to because is 
**	    causes problems when doing and insert as select on the
**	    same table.
**      26-Jun-2001 (horda03)
**	    (hanje04) X-Integ fo 451897
**          READLOCK=NOLOCK is not being honored. Bug 104534
**      06-Sep-2001 (hanal04) Bug 83046 INGSRV 477
**          Cross integrate horda03's oping20 fix to prevent ambiguous 
**          replace errors during a select query.
**	26-May-2002 (hanje04)
**	    Back out fix for bug 83046 again, brought in by X-Integ.
**	20-Aug-2002 (jenjo02)
**	    Support TIMEOUT=NOWAIT, new LK_UBUSY status from LKrequest()
**	    for ROW and VALUE locks.
**	    Write E_DM9066_LOCK_BUSY info message if DMZ_SES_MACRO(35).
**	    Return E_DM006B_NOWAIT_LOCK_BUSY.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**      24-apr-2003 (stial01)
**          dm1r_get() support DMF projection of subset of columns. (b110061)
**      28-may-2003 (stial01)
**          dm1r_lk_convert() Pass DM0P_IGNORE_LKLIMIT for lock conversion.
**          dm1r_allocate() Continue logic if lock NOWAIT fails.
**      10-jul-2003 (stial01)
**          Fixed row lock error handling during allocate (b110521)
**      15-sep-2003 (stial01)
**          Don't use row locking or physical page locking protocols for 
**          etabs if table locking (b110923)
**	01-dec-2003 (rigka01) problem INGSRV2589, but 111245
**          Wrong level of redirection in erlookup call for displaying
**	    either message E_DM9072 or E_DM9073 
**      02-jan-2004 (stial01)
**          Reduced logging for btree etabs if noblobjournaling,nobloblogging
**      14-jan-2004 (stial01)
**          Cleaned up 02-jan-2004 changes.
**	22-Jan-2004 (jenjo02)
**	    Added support for Partitioned tables, Global indexes
**	    with DM_TID8s, row-locking of TID8s.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      14-may-2004 (stial01)
**          Removed support for NOBLOBJOURNALING (blob journaling status must
**          be same as base table journaling status).
**      19-may-2004 (stial01)
**          Reworked fix for b111560 when cross integrating 467665.
**          Perform deferred update processing for replace/delete old row,
**          so that a subsequent get by tid (of the deleted tid)
**          can report ambiguous update. Also, dm1r_get (BYTID) should
**          return E_DM0047_CHANGED_TUPLE, instead of E_DM0044_DELETED_TID
**          if the row was deleted by the current cursor.
**      22-jun-2004 (stial01)
**          If DMPP_VPT_PAGE_HAS_SEGMENTS always alloc 1st segment on empty page
**      25-jun-2004 (stial01)
**          Modularize get processing for compressed, altered, or segmented rows
**      8-jul-2004 (stial01)
**          Changes for DMPP_VPT_PAGE_HAS_SEGMENTS and readnolock
**      27-aug-2004 (stial01)
**          Removed SET NOBLOBLOGGING statement
**      04-oct-2004 (stial01)
**          dm1r_get_segs() fix consistency check for record length 
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**      10-jan-2005 (stial01)
**          Fixed lock failure error handling after calls to dm1c_get.
**          dm1r_get_segs() fixed error handling. (b113716)
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      16-mar-05 (stial01)
**          dm1r_put_segs() fixed (deadlock) error handling. (b114093)
**	09-May-2005 (thaju02)
**	    dm1r_cvt_row(): for alter table alter column, store attr 
**	    ptr if row version >= ver_added. (B114478)
**	04-Jun-2005 (thaju02)
**	    Moved rcb_tup_adds++ from put() to dm1r_put().
**	    Moved rcb_tup_adds-- from delete() to dm1r_delete().
**      11-jan-2005 (horda03) Bug 111437/INGSRV2630
**          Memory corruption occurs when copying coupons into result record
**          when the results record is a projected key column. This is because
**          coupons or not part of the project key column, so no space has been
**          reserved in the record for them.
**      13-May-2005 (horda03) Bug 114508/INGSRV3301
**          During ROLLBACK physical locks taken in violation of the
**          physical lock protocol may be converted to logical locks
**          which may cause a E_DMA00D error.
**          Add dm1r_check_page_lock() to verify lock protocol.
**          Also updated dm1r_check_lock, to verify that logical row locks are held
**          when updating a row.
**      12-Sep-2005 (stial01)
**          dm1r_cvt_row(): remove dependency on ordinal_id, which may not 
**          be correct for ver_altcol after drop column. 
**          Also when skipping dropped altered column, increment src 
**          using att_from->length (b115192)
**      29-sep-2005 (stial01)
**          server trace point dm723 to verify update lock protocols (b115299)
**      31-mar-2006 (stial01)
**          Cleanup DMPP_VPT_PAGE_HAS_SEGMENTS TRdisplay's
**      30-Jan-2003 (hanal04) Bug 109530 INGSRV 2077
**          Added support for nullable date columns added via
**          ALTER TABLE to dm1r_cvt_row().
**	22-Feb-2008 (kschendel) SIR 122739
**	    Row accessor restructuring, reflect here (many places).
**	    Kill unused DM0L_REPLACE flag.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1s_?, dm1r_? functions converted to DB_ERROR *, 
**	    use new form uleFormat.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_?, dmf_adf_error functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dm1c_?, dm0p_? functions converted to DB_ERROR *
**	25-Aug-2009 (kschendel) 121804
**	    Need dm1p.h to satisfy gcc 4.3.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    New macros to lock/unlock pages, LG_LRI replaces LG_LSN,
**	    sensitized to crow_locking().
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Reinstated dm1r_crowlk_access().
**	01-apr-2010 (toumi01) SIR 122403
**	    Add decryption call.
*/

/*
**  function prototypes
*/

static DB_STATUS
dm1r_rowlk_access(
	DMP_RCB        *rcb,
	i4        get_status,
	i4        access,
	u_i4		*row_low_tran,
	u_i2		*row_lg_id,
	DM_TID         *tid,
	DM_TID         *tid_to_lock,
	i4		partno_to_lock,
	LK_LOCK_KEY    *save_lock_key,
	i4        opflag,
	DB_ERROR        *dberr);

static DB_STATUS
dm1r_crowlk_access(
	DMP_RCB		*rcb,
	i4		access,
	DM_TID		*tid,
	DM_TID		*tid_to_lock,
	LK_LOCK_KEY	*save_lock_key,
	i4		opflag,
	DB_ERROR        *dberr);

static DB_STATUS
dm1r_check_page_lock(
    DMP_RCB     *rcb,
    i4          page);


static DB_STATUS
put(
	DMP_RCB		*rcb, 
	DMP_PINFO	*dataPinfo,
	DM_TID		*tid, 
	char		*record, 
	char		*key,
	i4		record_size,
	i4		opflag,
	DMPP_SEG_HDR	*seg_hdr,
	DB_ERROR        *dberr);

static DB_STATUS
delete(
	DMP_RCB		*rcb, 
	DMP_PINFO	*pinfo,
	DM_TID		*tid, 
	i4		flag,
	i4		opflag,
	DB_ERROR        *dberr);

static DB_STATUS
replace(
	DMP_RCB		*rcb, 
	DM_TID		*old_tid, 
	char		*old_row,
	i4		old_record_size,
	DMP_PINFO	*newdataPinfo,
	DM_TID		*new_tid, 
	char		*new_row,
	i4		new_record_size,
	i4		opflag,
	i4		delta_start,
	i4		delta_end,
	DB_ERROR        *dberr);

static DB_STATUS
dm1r_put_segs(
	DMP_RCB	*rcb, 
	DMP_PINFO	*dataPinfo,
	DM_TID		*tid, 
	char		*record, 
	char		*key,
	i4		record_size,
	i4		opflag,
	DB_ERROR        *dberr);

static DB_STATUS
dm1r_del_segs(
	DMP_RCB		*rcb, 
	DM_TID		*tid, 
	i4		flag,
	i4		opflag,
	DB_ERROR        *dberr);

static DB_STATUS
dm1r_replace_segs(
	DMP_RCB		*rcb, 
	DM_TID		*old_tid, 
	char		*old_row,
	i4		old_record_size,
	DMP_PINFO	*newdataPinfo,
	DM_TID		*new_tid, 
	char		*new_row,
	i4		new_record_size,
	i4		opflag,
	DB_ERROR        *dberr);


/*{
** Name: dm1r_delete - Deletes a record from a table.
**
** Description:
**    This routine deletes the record identified by tid from a
**    table.
**
**    After deleting from a full hash overflow page, turn off the
**    FULLCHAIN bit on the hash primary page.
**
**    If deleting from a user table and row locking, the row must be locked
**    by the caller.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      tid                             The tid of the record to delete.
**      flag                            A value indicating type of delete.
**                                      If it is DM1C_BYTID then this is
**                                      a random delete, otherwise it is
**                                      a delete on current cursor with
**                                      tid in rcb->rcb_currenttid.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM003C_BAD_TID, E_DM0044_DELETED_TID.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      12-dec-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	30-sep-1988 (rogerk)
**	    Added support for DMPP_FULLCHAIN for hash tables.  If delete tuple
**	    from hash overflow page, make sure the FULLCHAIN bit is not set
**	    anymore on the primary page.
**	08-jun-92 (kwatts)
**	    6.5 MPF changes. dmpp_get_offset_macro, dm1c_reclen, dm1c_del,
**	    and calculation of free space all become accessor calls.
**	31-aug-1992 (jnash)
**	    Reduced logging project.
**	      - LKrequest row lock for system catalog rows.
**      03-nov-92 (jnash)
**          Reduced logging project.  Pass line number instead of tid to 
**	    dmpp_reclen.
**	05-nov-1992 (jnash)
**	    Temp hack to bypass locking until new recovery code in place. 
**	14-dec-92 (rogerk)
**	    Reduced Logging Project:  This routine now logs updates!
**	    Moved log writing from dm2r_delete to this routine to fit in
**	    better with page oriented logging protocols.  Log records
**	    are now written while the page mutex is held.
**	14-dec-92 (jnash)
**	    Added handling of journal status for catalog updates.  When a
**	    core catalog is updated (by a put, delete or replace), the journal
**	    status of the log record depends on the journal state of the user
**	    table which is being altered, not the journal status of the actual
**	    system catalog.
**	    Removed temporary use of rcb_recovery flag.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Extract row locking code into dm1r_lock_row(), so that cluster
**		    recovery code can more easily re-acquire row locks.
**	26-jul-1993 (rogerk)
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Change error handling to give traceback errors.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Took out references to rcb_usertab_jnl which used to dictate
**	    whether or not to journal system catalog updates.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Deletion of core
**	    catalog rows are always journaled.  Full explanation of non
**	    journal table handling given in DMRFP.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_getfree.
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_reclen.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**	06-may-1996 (nanpr01)
**	    Change all page header access as macro for New Page Format
**	    project.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Alter Table support:
**              Extract row_version from row being deleted
**                  and log it properly.
**              When deleting a row from an altered table, call the reclen
**                  accessor rather than assuming that the row is of size
**                  relwid.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**	    Pass reclaim_space to delete accessor.
**	25-feb-1996 (nanpr01)
**	    Alter Table drop column bug : corrupting data table because
**	    correct row length is not getting logged.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DM1R_LK_CHECK flag if calling dm1r_lock_row() just to
**          make sure that we already have the row lock. 
**      21-may-97 (stial01)
**          dm1r_delete() Added flags arg to dm0p_unmutex call(s)
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          If row locking, don't bother to check/update the FULLCHAIN status,
**          since the delete does not reclaim space.
**      29-jul-97 (stial01)
**          Added opflag parameter, to log original operation mode if REPLACE
**      21-apr-98 (stial01)
**          dm1r_delete() Set DM0L_PHYS_LOCK if extension table (B90677)
**      07-jul-98 (stial01)
**          dm1r_delete() Deferred update: new update_mode if row locking
**	25-sep-1998 (nanpr01)
**	    Donot reclaim spaces for large pages on delete.
**      06-oct-98 (stial01)
**          dm1r_delete() Set RCB_U_DEFERRED if tcb_extended (B92468)
**	14-dec-98 (thaju02)
**	    use update_mode rather than rcb->rcb_update_mode when calling
**	    dmpp_delete. 
**      20-sep-99 (stial01)
**          dm1r_delete() If tcb_extended, just put tranid, don't set deferred
**      18-jab-2000 (gupsh01)     
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          Check for lock protocol errors.
**	04-Jun-2005 (thaju02)
**	    Moved rcb_tup_adds-- from delete() to dm1r_delete().
**	02-Feb-2010 (jonj)
**	    Deleted leftover mvcc debug TRdisplay.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: add row_tran_id, row_lg_id to dm0l_del() prototype
**	26-Feb-2010 (jonj)
**	    CRIB being used now anchored in RCB.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
**	15-Apr-2010 (jonj)
**	    Additional diagnostics before dmd_check on deleted tid.
*/
DB_STATUS
dm1r_delete(
    DMP_RCB     *rcb, 
    DM_TID      *tid, 
    i4     flag,
    i4     opflag,
    DB_ERROR	*dberr )
{
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DB_STATUS	s;

    if (t->tcb_seg_rows)
    {
	s = dm1r_del_segs(rcb, tid, flag, opflag, dberr);
    }
    else
    {
	s = delete(rcb, &rcb->rcb_data, tid, flag, opflag, dberr); 
    }

    if (s == E_DB_OK)
	rcb->rcb_tup_adds--;

    return (s);
}

static DB_STATUS
delete(
    DMP_RCB		*rcb, 
    DMP_PINFO		*pinfo,
    DM_TID		*tid, 
    i4			flag,
    i4			opflag,
    DB_ERROR		*dberr )
{
    DMP_RCB	    *r = rcb;					
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS	    status = 0;
    LG_LSN	    lsn;
    char	    *record;
    i4         update_mode;
    i4         dm0l_flags;
    i4         record_size;
    i4         loc_id;
    i4         loc_config_id;
    i4         row_version = 0;	
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4         reclaim_space;
    DMPP_SEG_HDR	seg_hdr;
    DMPP_SEG_HDR	*seg_hdr_ptr;
    i4		    *err_code = &dberr->err_code;
    LG_LRI		lri;
    u_i4		row_tran_id;
    u_i2		row_lg_id;

    CLRDBERR(dberr);


    if (t->tcb_seg_rows)
	seg_hdr_ptr = &seg_hdr;
    else
	seg_hdr_ptr = NULL;

    for (;;)
    {
	/* Ensure lock on TID if CROW locking */
	if ( crow_locking(r) )
	{
	    status = dm1r_crow_lock(r, 0, tid, pinfo, dberr);
	    if ( status )
		break;
	}

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root
	*/
	dm0pLockBufForUpd(r, pinfo);

	/*
	** Consistency checks: make sure input data makes sense.
	*/
	if ((pinfo->page == NULL) || (tid->tid_tid.tid_page != 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page)) ||
	    (tid->tid_tid.tid_line >= 
	      (u_i2) DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, pinfo->page)))
	{
	    /* We're crashing, leave the buffer locked for a better view */
	    dmd_check(E_DMF010_DM1R_NO_PAGE);
	}

	if (row_locking(r) && ((t->tcb_rel.relstat & TCB_INDEX) == 0) &&
	    DMZ_SES_MACRO(33))
	{
	    DB_STATUS    tmp_status;

	    /* Make sure we already have the lock if delete from base table */
	    tmp_status = dm1r_check_lock(r, tid);
	    if (tmp_status != E_DB_OK)
	    {
		TRdisplay("dm1r_delete: No row lock for %~t\n",
		    sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
		SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
		status = E_DB_ERROR;
		break;
	    }
	}
        else if ( (!row_locking(r)) && (r->rcb_lk_type == RCB_K_PAGE) &&
                  DMZ_SES_MACRO(33))
        {
           DB_STATUS    tmp_status;

           /* Make certain that there is a logical page lock */

           tmp_status = dm1r_check_page_lock( r, tid->tid_tid.tid_page);

           if (tmp_status != E_DB_OK)
           {
              TRdisplay("dm1r_delete: No logical page lock for %~t\n",
                  sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
		SETDBERR(dberr, 0, E_DM9C2C_NOPAGELOCK);
              status = E_DB_ERROR;
              break;
           }
        }


	/*
	** Make sure there is a row at the indicated tid and find its length.
	*/
	status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
	    t->tcb_rel.relpgsize, pinfo->page, tid, &record_size,
	    &record, &row_version, &row_tran_id, &row_lg_id, seg_hdr_ptr);

	if (status != E_DB_OK)
	{
	    /* We're crashing, leave the buffer locked for a better view */

	    if ( r->rcb_crib_ptr )
	    {
		TM_STAMP	tim;
		char		err_buffer[512];

		TMget_stamp(&tim);
		TMstamp_str(&tim, err_buffer);
		STprintf(err_buffer + STlength(err_buffer),
		    " delete %d: status %d tid [%d,%d] record %p row_tran_id %x row_lg_id %d\n"
		    " crib xid %x rcb tranid %x crib_bos_tranid %x\n"
		    " log_id %d low_lsn %x commit %x bos %x\n",
			__LINE__,
			status,
			tid->tid_tid.tid_page, tid->tid_tid.tid_line,
			record,
			row_tran_id, row_lg_id,
			r->rcb_crib_ptr->crib_xid_array[row_lg_id],
			r->rcb_tran_id.db_low_tran,
			r->rcb_crib_ptr->crib_bos_tranid,
			r->rcb_slog_id_id,
			r->rcb_crib_ptr->crib_low_lsn.lsn_low,
			r->rcb_crib_ptr->crib_last_commit.lsn_low,
			r->rcb_crib_ptr->crib_bos_lsn.lsn_low);
		ERlog(err_buffer, STlength(err_buffer), &sys_err);
		TRdisplay(err_buffer);
	    }

	    /* Additional diagnostics, print page contents */
	    if ( r->rcb_other.page )
	    {
	        TRdisplay("\n%@ RCB other.page 0x%x:\n", r->rcb_other.page);
		dmd_prindex(r, r->rcb_other.page, (i4)0);
	    }
	    if ( r->rcb_other.CRpage )
	    {
	        TRdisplay("\n%@ RCB other.CRpage 0x%x:\n", r->rcb_other.CRpage);
		dmd_prindex(r, r->rcb_other.CRpage, (i4)0);
	    }

	    if ( r->rcb_data.page )
	    {
	        TRdisplay("\n%@ RCB data.page 0x%x:\n", r->rcb_data.page);
		dmd_prdata(r, r->rcb_data.page);
	    }
	    if ( r->rcb_data.CRpage )
	    {
	        TRdisplay("\n%@ RCB data.CRpage 0x%x:\n", r->rcb_data.CRpage);
		dmd_prdata(r, r->rcb_data.CRpage);
	    }

	    dmd_check(E_DMF011_DM1R_BAD_TID);
	}

	if (row_version != t->tcb_rel.relversion
	  || t->tcb_data_rac.compression_type != TCB_C_NONE
	  || seg_hdr_ptr)
	    (*t->tcb_acc_plv->dmpp_reclen)(t->tcb_rel.relpgtype,
	     t->tcb_rel.relpgsize, pinfo->page, tid->tid_tid.tid_line, &record_size);
	else
	    record_size = t->tcb_rel.relwid;

	/*
	** Build parameters and flags to LG and page routines.
	** We do not need to do deferred update processing for deletes
	*/
	update_mode = DM1C_DIRECT;
	if ( row_locking(r) | crow_locking(r) || NeedPhysLock(r) )
	    update_mode |= DM1C_ROWLK;

	/*
	** Get information on the location to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
					    (i4) tid->tid_tid.tid_page);
	loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, pinfo->page, dberr);
	    if (status != E_DB_OK)
	        break;
	}

	/*
	** Log the Delete operation.
	*/
	if (r->rcb_logging)
	{
	    /* 
	    ** Reserve space for log and clr.
	    */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		    (2 * (sizeof(DM0L_DEL) + record_size)),
		    &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9C60_DM1R_DELETE);
		break;
	    }

	    /*
	    ** Lock the buffer so nobody else can look at it while we are 
	    ** changing it.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);

	    /*
	    ** Log journal and lock modes.
	    */
	    dm0l_flags = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flags |= DM0L_MINI;

	    /*
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same lock protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flags |= DM0L_PHYS_LOCK;

	    /*
	    ** Deletes on core catalogs must indicate the journal status
	    ** of base table for which the update is being made.
	    */
	    if (t->tcb_sysrel)
	    {
		if (is_journal(r) && ( ! r->rcb_usertab_jnl))
		    dm0l_flags |= DM0L_NONJNL_TAB;
	    }
         
            if (row_locking(r))
                dm0l_flags |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flags |= DM0L_CROW_LOCK;

	    if (DMZ_TBL_MACRO(23))
	    {
		DM_TID		base_tid;

		if (t->tcb_rel.relstat & TCB_INDEX)
		    MEcopy((record + record_size) - sizeof(DM_TID), 
				sizeof(DM_TID), &base_tid);
		else
		    STRUCT_ASSIGN_MACRO(*tid, base_tid);
		status = dbg_dm723(r, tid, &base_tid, dm0l_flags, err_code);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, *err_code);
		    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
		    break;
		}
	    }

	    /* Get CR info from page, pass to dm0l_del */
	    DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);

	    status = dm0l_del(r->rcb_log_id, dm0l_flags,
		    &t->tcb_rel.reltid,
		    &t->tcb_rel.relid, t->tcb_relid_len,
		    &t->tcb_rel.relowner, t->tcb_relowner_len,
		    tid, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    t->tcb_rel.relcomptype, loc_config_id,
		    t->tcb_rel.relloccount, record_size, record,
		    (LG_LSN *) 0, row_version, seg_hdr_ptr,
		    row_tran_id,
		    row_lg_id,
		    &lri, dberr);
	    if (status != E_DB_OK)
	    {
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
		break;
	    }

		if((d->dcb_status & DCB_S_JOURNAL) && 
			!(t->tcb_rel.relstat & TCB_JOURNAL))
		{
			if(r && r->rcb_xcb_ptr)
				r->rcb_xcb_ptr->xcb_flags |= XCB_NONJNLTAB;	
		}

	    /*
	    ** Write the LSN and info about the delete record to the page
	    ** being updated.
	    ** This marks the version of the page for REDO processing and
	    ** CR undo.
	    */
	    DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);
	}
	else
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);

	/* donot reclaim space */
	if (t->tcb_rel.relpgtype != TCB_PG_V1 ||
	    t->tcb_data_rac.compression_type == TCB_C_NONE ||
	    NeedPhysLock(r) )
	{
	    /* Physical locking */
	    reclaim_space = FALSE;
	}
	else
	    reclaim_space = TRUE;

	/*
	** 2k etabs should always put tranid on page to reserve space
	** Don't add overhead of deferred update processing
	** We don't need deferred update processing for etabs, just
	** the tranid
	**
	** NB: neither row nor crow locking are possible with 2k etabs.
	*/
	if (t->tcb_extended && t->tcb_rel.relpgtype == TCB_PG_V1 &&
		r->rcb_lk_type != RCB_K_TABLE)
	    DMPP_VPT_SET_PAGE_TRAN_ID_MACRO(t->tcb_rel.relpgtype, pinfo->page, r->rcb_tran_id);

	/*
	** Do deferred put proessing if crow_locking so we can
	** ignore changes by our own transaction without mvcc undo
	*/
	if (r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
	{
	    status = (*t->tcb_acc_plv->dmpp_dput)(r, pinfo->page, tid, err_code);
	    if (status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, *err_code);
		break;
	    }
	}

	/*
	** Remove the delete-row from the page.
	*/
        (*t->tcb_acc_plv->dmpp_delete)(t->tcb_rel.relpgtype, 
		t->tcb_rel.relpgsize, pinfo->page, update_mode, reclaim_space,
		&r->rcb_tran_id, r->rcb_slog_id_id, tid, record_size);

	/*
	** Release the buffer lock to allow other threads to see the page.
	**
	** Note that DM0P_MUNPIN will restore Root->CR
	*/
	dm0pUnmutex(tbio, (i4)DM0P_MUNPIN, r->rcb_lk_id, pinfo);

	/*
	** Update any other rcb's in our transaction that may be currently
	** pointing at this record to indicate that the record has been deleted.
	*/
	dm1r_rcb_update(r, tid, (DM_TID *)NULL, DM1C_MDELETE, dberr);

	/*
	** If this is a hash table, make sure that this hash chain is no longer
	** marked as being full.
	**
	** If (c)row locking don't bother to check/update the FULLCHAIN status,
	** since the delete does not reclaim space.
	*/
	if ( (!row_locking(r) && !crow_locking(r))
	    && (r->rcb_tcb_ptr->tcb_rel.relspec == TCB_HASH))
	{
	    DM_PAGENO   bucket;
	    i4	    space;

	    /*
	    ** Don't bother to check/update the FULLCHAIN status of the
	    ** bucket if:
	    **   - the delete is on a primary page.
	    **   - rcb_hash_nofull is set to this bucket (rcb_hash_nofull
	    **     holds the last visited primary page which did not have
	    **     FULLCHAIN set)
	    **         Note rcb_hash_nofull may be incorrect when row locking,
	    **         so we ignore it.
	    **   - the page was not full previous to the delete.
	    */
	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page) >= 
			r->rcb_tcb_ptr->tcb_rel.relprim)
	    {
		bucket = r->rcb_tcb_ptr->tcb_rel.relprim - 1;
		if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, pinfo->page) > 0)
		    bucket = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, 
							pinfo->page) - 1;
		if (r->rcb_hash_nofull != bucket)
		{
		    space = (*t->tcb_acc_plv->dmpp_getfree)(pinfo->page,
					t->tcb_rel.relpgsize);
		    if (space < (record_size * 2))
			dm1h_nofull(rcb, bucket);
		}
	    }
	}

	return (E_DB_OK);
    }

    /*
    ** Unlock buffer.
    **
    ** This will restore Root->CR
    */
    dm0pUnlockBuf(r, pinfo);


    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C60_DM1R_DELETE);
    }

    return (status);
}

/*{
** Name: dm1r_get - Get a record from a table.
**
** Description:
**      This routine takes the the tid of the next record to
**      retrieve.  Get either retrieves the next sequential record 
**      after the current positon indicated in rcb->rcb_lowtid  or
**      retrieves the record specified by input tid if 
**      the opflag indicates a get by tid(DM1C_BYTID).
**      In get next mode(DM1C_GETNEXT), this routine returns records 
**      until end of scan.  End of scan is determined
**      by the high tid value in the rcb (rcb->rcb_hightid).  It points
**      to the first page past the end point.  These are set by the higher
**      level postioning routine.
**      
**      This routine sets rcb_fetchtid so you can track deletes and replaces
**	when using multiple rcb's per transaction on same table.
**
**      This routine assumes that the following variables in the RCB are
**      for the use of scanning and will not be destroyed by other operations:
**      rcb->rcb_data.page	 - A pointer to last data page assessed.
**      rcb->rcb_lowtid          - A pointer to current record position.
**      rcb->rcb_currentid       - A pointer to return actual tid of 
**                                 retrieved record.  
**      rcb->rcb_hightid         - A pointer to the highest tid that should be 
**                                 retrieved.
**
**	If we follow to a new hash primary page, we set rcb_hash_nofull if
**	that chain is not full.  This is an optimization that will keep us
**	from having to update the primary page if a delete is done from that
**	page's overflow chain.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	tid				If opflag == DM1C_BYTID then this is
**					the record to get.
**      opflag                          Value indicating type of retrieval.
**                                      Must be either DM1C_GETNEXT or
**                                      DM1C_BYTID.
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**					E_DM003C_BAD_TID
**                                      E_DB0044_DELETED_TID
**                                      E_DM0055_NONEXT
**					E_DM008A_ERROR_GETTING_RECORD
**					E_DM0148_USER_QUAL_ERROR
**					E_DM938B_INCONSISTENT_ROW
**					E_DM93A2_DM1R_GET
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**      12-dec-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	30-sep-1988 (rogerk)
**	    Add support for FULLCHAIN status on hash primary pages.  When
**	    move to a new hash primary page - set rcb_hash_nofull if that
**	    chain is not full.
**	 7-nov-88 (rogerk)
**	    Send compression type to dm1c_get() routine.
**	    Check for high qual before low as we use high for exact match.
**	24-Apr-89 (anton)
**	    Added local collation support
**	29-may-89 (rogerk)
**	    Added checks for bad rows in dm1c_get calls.  Added trace flags
**	    to allow us to delete rows by tid that cannot be fetched.
**	22-jan-90 (rogerk)
**	    Make sure we send byte-aligned tuples to qualification function.
**	    Use IFDEF for byte-aligment to determine whether to test for
**	    align restriction.
**	    Don't copy tuple to caller's buffer if it was already materialized
**	    there by dm1c_get or copied there for alignment restrictions.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	23-jul-91 (markg)
**	    Added fix for bug 38679 where an incorrect error was being 
**	    returned if there had been a failure while attempting to 
**	    qualify a tuple. In the event of a serious failure we now 
**	    write E_DM93A3_QUALIFY_ROW_ERROR to the error log and return 
**	    E_DM93A2_DM1R_GET. In the event of a user error being detected 
**	    by the qualification function, we write nothing to the error log
**	    and return E_DM0148_USER_QUAL_ERROR .
**	19-aug-91 (rogerk)
**	    Modified above fix to not log internal errors when the qualify
**	    function returns an error since we cannot distiguish between
**	    user errors and internal errors.  When the USER_ERROR return
**	    value from the qualification function becomes supported, we
**	    should change back to logging internal error messages.
**	08-jun-92 (kwatts)
**	    6.5 MPF changes. dmpp_get_offset_macro, dm1c_get, and checking
**	    for new tuple in deferred update mode become accessor calls.
**	13-oct-1992 (jnash)
**	     Reduced logging project.  Move calls to compression 
**	     routines out of dm1c and up to this level.  dmpp_get
**	     now always returns a pointer to the record, never
**	     copying it to the user buffer.  
**	21-may-1993 (robf)
**	     Log errors coming back from adt routines prior to calling
**	     dmd_check so we have a better idea of  whats happening.
**       1-Jul-1993 (fred)
**          Added dm1c_pget() function where required to allow for the
**          rcb_f_qual function's being run on large objects.
**	23-aug-1993 (rmuth)
**	    When getting a record from a table, if using tid access make
**	    sure page requested is not a FHDR or FMAP.
**	23-may-1994 (andys)
**	    When fixing a page for a get by tid, pass the DM0P_USER_TID
**	    flag to dm0p_fix_page so that misleading errors will not be
**	    logged. An example of when this might happen is when
**	    optimizedb does a "create table mufc as select * from shme where 
**	    tid = -1" [bug 49726]
**      04-apr-1995 (dilma04) 
**          When moving from one page to the next, clear RCB_PAGE_HAD_RECORDS.
**	06-may-1996 (nanpr01)
**	    Change all page header access as macro for New Page Format
**	    project.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Alter table support:
**              In dm1r_get, if page has row version numbers, extract the
**                  row version number and convert the row to the current
**                  version, if necessary.
**	13-Aug-1996 (prida01)
**	    Give relatts correct type of i4 when calling dm1c_pget.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass buffer for data
**	    uncompression.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**	    Lock row if row locking. Moved common code to get_unpack_lock()
**      22-jan-97 (dilma04)
**          - Fix bug 79961: release shared row locks if doing get by tid
**           and isolation level is read committed;
**          - Add flag argument to get_unpack_lock() call. Fix bug 80112: set
**          flag = DM1R_LK_NOLOCK if it is a get to update secondary index.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - test rcb_iso_level for isolation level;
**          - use macro to check for serializable transaction.
**      26-feb-97 (dilma04)
**          Lock Escalation for Row Locking and Lower Isolation Levels:
**          - when searching for a qualifying record with row locking, 
**          first, acquire a physical row lock, then, depending on whether 
**          the lock has to be held until the end of transaction, either 
**          convert it to a logical lock, or release; 
**          - when doing get by tid, request a row lock in physical mode
**          only if this lock has to be released after record is returned.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - removed checks for new_lock before calling dm1r_unlock_row(),
**          instead just check if row is locked;
**          - moved code to clear RCB_PAGE_HAD_RECORDS flag to dm0p_fix_page(); 
**          - added support for releasing exclusive row locks; 
**          - if row lock can be released before the end of transaction, set
**          rcb_release_row flag, but do not release the lock in this routine;
**          - pass DM1R_LK_CONVERT flag to dm1r_lock_row() when converting a
**          physical row lock to a logical lock;
**          - return E_DM9C2A_NOROWLOCK error code, if a new_lock is returned 
**          by dm1r_lock_row() called with DM1R_LK_CONVERT flag.	
**       21-may-97 (stial01)
**          dm1r_get() Pass buf_locked param to get_unpack_lock.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      29-may-97 (stial01)
**          dm1r_get() Unlock buffer if user interrupt or force abort.
**      29-jul-97 (stial01)
**          dm1r_get() Apply key qualification before locking row (B84213):
**      20-aug-97 (stial01)
**          dm1r_get() Init lock_err
**      29-sep-97 (stial01)
**          dm1r_get() Don't issue E_DM9C2A_NOROWLOCK if row LK_CONVERT fails
**         (limit exceeds trans lock limit->escalate->deadlock) is possible.
**         and we should not issue a different message when row locking.
**      30-oct-97 (stial01)
**          B86862: Copy record into caller buffer BEFORE calling dm1c_pget.
**          Fixed another place where we copied tuple and didn't fix rec_ptr
**      07-jul-98 (stial01)
**          dm1r_get() Defer row lock request for committed data (B84213)
**      12-aug-98 (stial01)
**          dm1r_get() Re-check low key is only necessary for ISAM tables.
**          (we use rcb_ll_given to re-check ISAM low key, AND to re-position
**          to BTREE Low key).
**      20-Jul-98 (wanya01)
**          dm1r_get() Copy rec to called buf BEFORE calling dm1c_pget.
**          This is addtional changes for b86862. The previous changes was
**          only made for routine when retrieving record by sequence, not
**          when retrieving record by TID.
**      11-jan-99 (stial01)
**          Fixed bad error handling from 07-jul-1998 changes (change 436735)
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      05-Oct-1998 (horda03)
**          B83046: Report E_DM0047 (Ambiguous replace) following only if the
**          record was obtained for modification (and it has previously been
**          modified by the transaction).
**	06-Feb-2001 (jenjo02)
**	    Pushed secondary index searches from dm2r (si_replace(),
**	    si_delete()) into dm1r_get() for efficiency.
**	    When row-locking by TID (dm1r_get()), record was being
**	    copied from page to caller's buffer twice.
**	    Consolidated GETNEXT and BYTID to get rid of all that
**	    duplicated and, in some places, inconsistent code.
**	19-may-2001 (somsa01)
**	    Fixed typo caused by cross-integrations.
**	22-May-2001 (hanje04)
**	    Removed crossing of fix for B83046 from to because is 
**	    causes problems when doing and insert as select on the
**	    same table.
**	11-mar-2003 (thaju02)
**	    Unfix iirelation pages during rcb_f_qual processing to avoid
**	    deadlock server crash. (b109719)
**	13-Apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	10-may-2004 (thaju02)
**	    Added skip_reading_page. For rolldb of online modify, do not read 
**	    page if not in rnl lsn array.
**	05-Aug-2004 (jenjo02)
**	    Don't do BYTE_ALIGN copy if rec_ptr == record.
**      11-jan-2005 (horda03) Bug 111437/INGSRV2630
**          Don't fill in coupons for Extension tables when the output
**          record is a projected key column.
**	25-jul-06 (toumi01)
**	    Make the gcc 4.1.0 compiler happy, avoiding strange syntax error.
**	12-Feb-2007 (kschendel)
**	    Allow user aborts during long scans with no qualifying rows.
**	11-Apr-2008 (kschendel)
**	    Change qual-function call to speed up the normal CX case.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Updated pget call.
*/

DB_STATUS
dm1r_get(
    DMP_RCB     *rcb,
    DM_TID	*tid,
    char	*record,
    i4		opflag, 
    DB_ERROR	*dberr)
{
	DMP_RCB		*r = rcb;
	DMP_TCB		*t = r->rcb_tcb_ptr;
	DMP_TABLE_IO    *tbio = &t->tcb_table_io;
	DML_SCB		*scb;
	DB_STATUS	s;		/* Variable used for return status. */
	DB_STATUS       get_status;
	DB_STATUS       lock_status;
        i4         	fix_action;     /* Type of access to page. */
	i4         	record_size;    /* Size of record retrieved. */
	DM_TID          localtid;
        i4         	compare;
	char		*rec_ptr;
	i4         	local_err;
        i4         	new_lock;
	LK_LOCK_KEY     save_lock_key;
	DM_PAGENO       nextpageno;
	ADF_CB          *adf_cb = r->rcb_adf_cb;
	ADF_CB		*dummy;
	ADF_CB		**qual_adfcb;	/* points to scb_qfun_adfcb or dummy */
	i4		row_version = 0;
	u_i4		row_low_tran = 0;
	u_i2		row_lg_id;
	DM_TID          tid_to_lock;
	DM_TID          unlock_tid;
	DM_TID 		ii_si_tid;
	DM_TID8		tid8;
	u_i2		next_line_on_page;
	i4		page_stat;
	bool		skip_reading_page = FALSE;
	DB_ERROR	local_dberr;
	i4	        *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;

    save_lock_key.lk_type = 0;

#ifdef xDEBUG
    if (DMZ_AM_MACRO(21))
    {
	TRdisplay("dm1r_get: Starting tid = %d,%d,flag = %x\n", 
                r->rcb_lowtid.tid_tid.tid_page, 
                r->rcb_lowtid.tid_tid.tid_line, 
                opflag); 
	TRdisplay("dm1r_get: limit page is %d\n", 
                          r->rcb_hightid.tid_tid.tid_page);
    }
#endif

    /* Either variety (BYTID, GETNEXT) must supply "tid" ptr */
    if ( tid == NULL )
    {
	SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	return (E_DB_ERROR);
    }

    /*
    ** If secondary index search (si_delete(),si_replace())
    ** save the TID we need to find.
    */
    if ( opflag & DM1C_GET_SI_UPD && t->tcb_rel.relstat & TCB_INDEX )
	ii_si_tid.tid_i4 = tid->tid_i4;

    scb = NULL;
    qual_adfcb = &dummy;
    if (r->rcb_xcb_ptr)
    {
	scb = r->rcb_xcb_ptr->xcb_scb_ptr;
	qual_adfcb = &scb->scb_qfun_adfcb;
    }
		 
    /*
    ** GETNEXT comes back here for each new page.
    ** BYTID never does.
    */
    for (;;skip_reading_page = FALSE)
    {
	if ( opflag & DM1C_GETNEXT )
	{
	    localtid.tid_i4 = r->rcb_lowtid.tid_i4;

	    if (row_locking(r) && serializable(r))
		fix_action |= DM0P_PHANPRO; /* need phantom protection */
	}
	else
	{
	    /* This is a get by tid */

	    localtid.tid_i4 = tid->tid_i4;

	    /*
	    ** Or in DM0P_USER_TID so that dm0p_fix_page won't log errors
	    ** to errlog.log.
	    */
	    fix_action |= DM0P_USER_TID;
	}

	/*  Get the correct data page. */
	if ( (r->rcb_data.page) &&
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page) 
			!= localtid.tid_tid.tid_page )
	{
	    /* Unlock buffer */
	    dm0pUnlockBuf(r, &r->rcb_data);

	    if ( s = dm0p_unfix_page(r, DM0P_UNFIX,
			  &r->rcb_data, dberr) )
		return (s);
	}

	if (r->rcb_data.page == NULL)
	{
	    s = dm0p_fix_page(r, 
			  (DM_PAGENO)localtid.tid_tid.tid_page, 
			  fix_action, &r->rcb_data, 
			  dberr);

	    if ((t->tcb_dcb_ptr->dcb_status & DCB_S_ROLLFORWARD) &&
		(r->rcb_rnl_online) && 
		(s == E_DB_INFO) && 
		(dberr->err_code == E_DM9CB2_NO_RNL_LSN_ENTRY))
	    {
		skip_reading_page = TRUE;
		CLRDBERR(dberr);
		s = E_DB_OK;
	    }

	    if (s != E_DB_OK)
	    {
		/*	03-apr-87 (rogerk)
		**	If dm0p_fix_page returns a caller error 
		**      ( < E_DM_INTERNAL), don't
		**	    map the error to BAD_TID.
		*/
		if ( opflag & DM1C_BYTID && dberr->err_code > E_DM_INTERNAL )
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		return (s);
	    }

	    /*
	    ** If following to a new hash primary page, set the
	    ** rcb_hash_nofull if this chain is not full.  This is
	    ** an optimization to prevent us from having to update
	    ** this page if a delete is done on this page's ovfl chain.
	    **
	    ** Skip this for (c)row locking, as deletes do not reclaim
	    ** space, and we may have concurrent activity on the page
	    */
	    if ( opflag & DM1C_GETNEXT &&
	         ! row_locking(r) &&
	         ! crow_locking(r) &&
		localtid.tid_tid.tid_page < t->tcb_rel.relprim &&
		((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_data.page) & DMPP_FULLCHAIN) == 0))
	    {
		r->rcb_hash_nofull = localtid.tid_tid.tid_page;
	    }
	}

	/*
	** Check to see if interrupt occurred once per page.
	*/
	CScancelCheck(r->rcb_sid);
	if (*(r->rcb_uiptr))
	{
	    /* check for user interrupt. */
	    if (*(r->rcb_uiptr) & RCB_USER_INTR)
	    {
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
		return (E_DB_ERROR);
	    }

	    /* check to see if force abort occurred. */
	    if (*(r->rcb_uiptr) & RCB_FORCE_ABORT)
	    {
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
		return (E_DB_ERROR);
	    }
	}

	if ( row_locking(r) || crow_locking(r) )
	{
	    if ( opflag & DM1C_BYTID && row_locking(r) )
	    {
		if (t->tcb_rel.relstat & TCB_INDEX) 
		{
		    /*
		    ** If get by tid on index from DMF during update, the base 
		    ** table row should be locked.
		    ** 
		    ** Otherwise, the usefulness of get by tid on a secondary
		    ** index is questionable, so we won't add extra code for
		    ** row locking in this case.
		    */
		    if ((opflag & DM1C_GET_SI_UPD) == 0)
		    {
			TRdisplay("Get by tid on index, escalate to table lock\n");
			s = dm0p_escalate(r, (i4)0, dberr);
			if (s != E_DB_OK)
			    return (E_DB_ERROR);
		    }
		}
		else
		{
		    /*
		    ** Get by tid in base table, get the row lock before
		    ** we lock the buffer
		    */
		    lock_status  = dm1r_lock_row(r, 
				    r->rcb_iso_level == RCB_CURSOR_STABILITY ?
				    (i4)(DM1R_LK_PHYSICAL) : 0,
				    &localtid,
				    &new_lock, 
				    &save_lock_key,
				    dberr);

		    if (lock_status == E_DB_ERROR)
			return (E_DB_ERROR);
		}
	    }

	    /*
	    ** Lock buffer for get.
	    **
	    ** This may replace rcb_data.page with a CR page
	    */
	    dm0pLockBufForGet(r, &r->rcb_data, &s, dberr);
	    if ( s )
		return(s);
	}

	/* Capture invariants for this page */
	next_line_on_page =
	    DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype,
			r->rcb_data.page);

	if ( opflag & DM1C_BYTID )
	{
	    /*
	    ** If the page read is not a data page (it is an isam index page,
	    ** FHDR or FMAP page ), then return BAD_TID.
	    **
	    ** If the tid specified gives an invalid position on this page 
	    ** (tid_line is greater than next line) return BAD_TID.
	    ** 
	    ** If the tuple at tid has been changed, moved or removed by the same
	    ** cursor/statement, return CHANGED_TUPLE.
	    **
	    ** If the tuple at tid used to exist, but has been moved or removed by
	    **   a previous cursor/statement, return DELETED_TID.
	    **
	    **  03-apr-87 (rogerk)
	    **  Make sure we return CHANGED_TUPLE on an ambiguous replace.  Don't
	    **  check for DELETED_TID after we have already decided it is a
	    **  CHANGED_TUPLE case.
	    **  
	    */
	    page_stat = DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
					r->rcb_data.page);
	    if ( page_stat & (DMPP_DIRECT | DMPP_FHDR | DMPP_FMAP) ||
		localtid.tid_tid.tid_line >= next_line_on_page )
	    {
		/* Unlock buffer */
		dm0pUnlockBuf(r, &r->rcb_data);
		SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		return (E_DB_ERROR);
	    }
	}

	for (; !skip_reading_page ;)
	{
	    /* While there are records left on this page (GETNEXT) */
	    if ( opflag & DM1C_GETNEXT )
	    {
		if ( ++(r->rcb_lowtid.tid_tid.tid_line) 
			< next_line_on_page )
		    localtid.tid_i4 = r->rcb_lowtid.tid_i4;
		else
		    break;	/* To next page */
	    }

	    record_size = r->rcb_proj_relwid;

	    s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, r->rcb_data.page, 
		    &localtid, &record_size,
		    &rec_ptr, &row_version, &row_low_tran, 
		    &row_lg_id, (DMPP_SEG_HDR *)NULL);

	    /* Skip deleted record if GETNEXT, error if BYTID */
	    if ( (get_status = s) == E_DB_WARN )
	    {
		if ( opflag & DM1C_GETNEXT )
		{
		    if (!rec_ptr ||
			DMPP_SKIP_DELETED_ROW_MACRO(r, r->rcb_data.page, 
			row_lg_id, row_low_tran))
			continue;
		    s = E_DB_OK;
		}
		else
		{
		    if  ((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
			&& (opflag & DM1C_GET_SI_UPD) == 0
			&& (*t->tcb_acc_plv->dmpp_isnew)(r, r->rcb_data.page,
				    (i4)localtid.tid_tid.tid_line))
			SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
		    else
			SETDBERR(dberr, 0, E_DM0044_DELETED_TID);
		    /* Unlock buffer */
		    dm0pUnlockBuf(r, &r->rcb_data);
		    return (E_DB_ERROR);
		}
	    }

	    /* RCB_CONSTRAINT should see new rows */
	    if  ((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
		&& (r->rcb_state & RCB_CONSTRAINT) == 0
		&& (opflag & DM1C_GET_SI_UPD) == 0
		&& (*t->tcb_acc_plv->dmpp_isnew)(r, r->rcb_data.page,
			    (i4)localtid.tid_tid.tid_line))
	    {
		if (opflag & DM1C_GETNEXT)
		    continue;
		/* Unlock buffer */
		dm0pUnlockBuf(r, &r->rcb_data);
		SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
		return (E_DB_ERROR);
	    }

	    if ( opflag & DM1C_GETNEXT )
		r->rcb_s_scan++;

	    /* Additional processing if compressed, altered, or segmented */
	    if (s == E_DB_OK && 
		(t->tcb_data_rac.compression_type != TCB_C_NONE || 
		row_version != r->rcb_proj_relversion ||
		t->tcb_seg_rows))
	    {
		s = dm1c_get(r, r->rcb_data.page, &localtid, record, dberr);

		if (s && dberr->err_code != E_DM938B_INCONSISTENT_ROW)
		    return (s);
		rec_ptr = record;
	    }

	    if (s != E_DB_OK)
	    {
		/*
		** DM1C_GET returned an error - this indicates that
		** something is wrong with the tuple at the current tid.
		**
		** DM1C_GETNEXT:
		** If user is running with special PATCH flag, then skip
		** this row and go on to the next, otherwise return with
		** an error.
		**
		** DM1C_BYTID:
		** If user is running with special PATCH flag, then return
		** with garbage row, but at least no error.  This should allow
		** us to do a delete by tid.
		*/
		if ( opflag & DM1C_GETNEXT && DMZ_REC_MACRO(1) )
		    continue;
		/* Unlock buffer */
		dm0pUnlockBuf(r, &r->rcb_data);

		if ( opflag & DM1C_BYTID && DMZ_REC_MACRO(2) )
		{
		    r->rcb_currenttid.tid_i4 = localtid.tid_i4;
		    r->rcb_fetchtid.tid_i4 = localtid.tid_i4;
		    r->rcb_state &= ~RCB_FETCH_REQ;    
		    return (E_DB_OK);
		}
		dm1c_badtid(r, &localtid);
		if ( DMZ_REC_MACRO(1) )
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		else
		    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
		return (E_DB_ERROR);
	    }

	    /* If there are encrypted table or si columns, decrypt. */
	    if ( t->tcb_rel.relencflags & TCB_ENCRYPTED ||
		(t->tcb_parent_tcb_ptr && 
		 t->tcb_parent_tcb_ptr->tcb_rel.relencflags & TCB_ENCRYPTED) )
	    {
		s = dm1e_aes_decrypt(r, &t->tcb_data_rac, rec_ptr, record,
			r->rcb_erecord_ptr, dberr);
		if (s != E_DB_OK)
		    return(s);
		rec_ptr = record;
	    }

	    if ( opflag & DM1C_GETNEXT &&
		(r->rcb_hl_given || r->rcb_ll_given) )
	    {
		/* Check for high qualification. */
		if (r->rcb_hl_given)
		{
		    s = adt_keytupcmp(adf_cb, (i4)r->rcb_hl_given, 
			r->rcb_hl_ptr, t->tcb_key_atts, rec_ptr, 
			(i4 *)&compare);
		    if ((s == E_DB_OK) && (compare < 0 || 
			(compare > 0 && r->rcb_hl_op_type == RCB_EQ)))
			continue;
		}

		/* Check for low qualification. (ISAM only) */
		if (s == E_DB_OK && 
			t->tcb_table_type == TCB_ISAM && r->rcb_ll_given)
		{
		    s = adt_keytupcmp(adf_cb,(i4)r->rcb_ll_given, 
			r->rcb_ll_ptr, t->tcb_key_atts, rec_ptr, 
			(i4 *)&compare);
		    if (s == E_DB_OK && compare > 0)
			continue;
		}

		if (s != E_DB_OK)
		{
		    uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, 
			(CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    /* Unlock buffer */
		    dm0pUnlockBuf(r, &r->rcb_data);
		    dmd_check(E_DMF012_ADT_FAIL);
		}
	    }

	    /*
	    ** If the table being examined has extensions,
	    ** then we must fill in the indirect pointers that
	    ** exist on the page.  For large objects (the only
	    ** ones which exist at present), the coupons used
	    ** in the server contain the rcb of the table open
	    ** instance from which they are fetched.  These
	    ** are filled in, for tables with extensions, below.
	    ** This must be done before the rcb_f_qual
	    ** function is invoked, since it may contain
	    ** requests to examine the large objects in this record.
	    **
	    ** NOTE:  Invoking the following rcb_f_qual (GETNEXT)
	    ** function may involve indirect recursion within
	    ** DMF (in that a qualification involving a large
	    ** object will go read some collection of the
	    ** extension tables).  Any internal concurrency
	    ** issues must take this into account.  In particular
	    ** we unfix catalog pages in case the recursive qual function
	    ** asks for catalog pages.
            **
            ** Don't get coupon details if this is a projected key
            ** column. If we ever support extensions in indices
            ** (e.g. as data columns, then the coupon details
            ** should be added in the dm1r_cvt_rows when getting
	    */

	    if ( (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) &&
                 !r->rcb_proj_misc)
	    {
		/*
		** We must not pass dm1c_pget a pointer into the page
		** dm1c_pget() will change the rcb pointer in the coupon.
		** This causes recovery errors when we do consistency
		** checks on the tuple vs logged tuple. 
		** It is also bad to update the page without a mutex.
		*/
		if (rec_ptr != record)
		{
		    MEcopy(rec_ptr, record_size, record);
		    rec_ptr = record;
		}

		s = dm1c_pget(t->tcb_atts_ptr,
			      r, rec_ptr, dberr);
		if (s)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			       (char *)NULL, (i4)0, (i4 *)NULL, err_code,
			       0);
		    /* Unlock buffer */
		    dm0pUnlockBuf(r, &r->rcb_data);
		    SETDBERR(dberr, 0, E_DM9B01_GET_DMPE_ERROR);
		    return(E_DB_ERROR);
		}
	    }
	    
	    if ( opflag & DM1C_GETNEXT )
	    {
		/*
		** Get tid to lock
		*/
		if ((t->tcb_rel.relstat & TCB_INDEX) == 0)
		    tid_to_lock.tid_i4 = localtid.tid_i4;
		else
		{
		    /*
		    ** Extract the TID value out of the secondary index.
		    ** Use copy in case tidp is not aligned in ix tuple
		    */

		    /*
		    ** If fetching a secondary index record for update/delete,
		    ** GETNEXT until the right one is found. The tid sought
		    ** was passed in and sits in ii_si_tid.
		    **
		    ** For Global indexes, we must also match on 
		    ** partition number...
		    */
		    if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
		    {
			MEcopy((char*)(rec_ptr + record_size - sizeof(DM_TID8)),
			    sizeof(DM_TID8), (char*)&tid8);

			if ( opflag & DM1C_GET_SI_UPD &&
			    (r->rcb_base_rcb->rcb_tcb_ptr->tcb_partno !=
				tid8.tid.tid_partno ||
			     ii_si_tid.tid_i4 != tid8.tid_i4.tid) )
			{
			    continue;
			}

			r->rcb_partno = tid8.tid.tid_partno;
			tid_to_lock.tid_i4 = tid8.tid_i4.tid;
		    }
		    else
		    {
			MEcopy((char*)(rec_ptr + record_size - sizeof(DM_TID)),
			    sizeof(DM_TID), &tid_to_lock.tid_i4);

			if ( opflag & DM1C_GET_SI_UPD &&
			     tid_to_lock.tid_i4 != ii_si_tid.tid_i4 )
			{
			    continue;
			}
		    }
		}

		/* 
		** Check for caller qualification function.
		**
		** This qualification function requires that tuples start
		** on aligned boundaries.  If the tuple is not currently
		** aligned, then we need to make it so.
		**
		** (CX qualifications don't care about alignment, and it
		** might be appropriate to skip this copy for ade-execute-cx.
		** By-hand qualifications such as used by QEU might care
		** about alignment, however.)
		**
		** THIS IS A TEMPORARY SOLUTION:  We check here if the
		** tuple is aligned on an ALIGN_RESTRICT boundary, and
		** if not we copy it to the tuple buffer, which we know
		** is aligned.  But we only make this check if the machine
		** is a BYTE_ALIGN machine - since even on non-aligned
		** machines, ALIGN_RESTRICT is usually defined to some
		** restrictive value for best performance.  We do not
		** want to copy tuples here when we don't need to.  The
		** better solution is to have two ALIGN_RESTRICT boundaries,
		** one for best performance and one which necessitates
		** copying of data.
		*/
		if ( r->rcb_f_qual )
		{
		    /*
		    ** If row locking check if we need row locked before
		    ** applying NON KEY qualification
		    */
		    if (row_locking(r))
		    {
		        s = dm1r_rowlk_access(r, get_status, ROW_ACC_NONKEY, 
			    &row_low_tran, &row_lg_id, &localtid, &tid_to_lock, 
			    r->rcb_partno,
			    &save_lock_key, opflag, dberr);
			if (s == E_DB_WARN)
			    continue;
			if (s == E_DB_ERROR)
			{
			    dm0pUnlockBuf(r, &r->rcb_data);
			    return (E_DB_ERROR);
			}
		    }

#ifdef BYTE_ALIGN
		    /* lint truncation warning if size of ptr > int, 
		       but code valid */
		    if ( rec_ptr != record &&
		        (i4)rec_ptr & (i4)(sizeof(ALIGN_RESTRICT) - 1) )
		    {
			MEcopy(rec_ptr, record_size, record);
			rec_ptr = record;
		    }
#endif
		    r->rcb_s_qual++;

                    /* b109719, INGSRV2111 */
		    if ((t->tcb_rel.reltid.db_tab_base == DM_B_RELATION_TAB_ID) &&
			r->rcb_data.page)
		    {
                        DB_STATUS local_status = E_DB_OK; 
                        i4        local_err = 0;  

                        if (rec_ptr != record)
                        {
                            MEcopy(rec_ptr, record_size, record);
                            rec_ptr = record; 
                        }

		        local_status = dm0p_unfix_page(r, DM0P_UNFIX,
		               &r->rcb_data, &local_dberr);
		        if (local_status != E_DB_OK)
                        {
			    *dberr = local_dberr;
		            return (local_status);
                        }
		    }

		    *qual_adfcb = adf_cb;
		    /* Clean these in case non-ADF error */
		    adf_cb->adf_errcb.ad_errclass = 0;
		    adf_cb->adf_errcb.ad_errcode = 0;
		    *(r->rcb_f_rowaddr) = rec_ptr;
		    *r->rcb_f_retval = ADE_NOT_TRUE;
		    s = (*r->rcb_f_qual)(adf_cb, r->rcb_f_arg);
		    *qual_adfcb = NULL;		/* Not an a qual fcn now */
		    if (s != E_DB_OK)
		    {
			s = dmf_adf_error(&adf_cb->adf_errcb, s, scb, dberr);
			/* Unlock buffer */
			dm0pUnlockBuf(r, &r->rcb_data);
			if (s != E_DB_OK)
			    return (s);
		    }
		    if (*r->rcb_f_retval != ADE_TRUE)
		    {
			/* Row does not qualify, go to next one */

                        /* b109719, INGSRV2111 */
                        if ((r->rcb_data.page == NULL) &&
                            (t->tcb_rel.reltid.db_tab_base == DM_B_RELATION_TAB_ID))
			{
			    DB_STATUS local_status = E_DB_OK; 
			    i4 	  local_err = 0; 
			    local_status = dm0p_fix_page(r, 
					(DM_PAGENO)localtid.tid_tid.tid_page, 
					fix_action, &r->rcb_data, 
					&local_dberr);

			    if (local_status != E_DB_OK)
			    {
				*dberr = local_dberr;
				return (local_status);
			    }
			}
			continue;
		    }
		}

		/*
		** This row qualifies. If row locking, make sure it is locked.
		** Convert physical to logical if repeatable read
		*/
		if (row_locking(r))
		{
		    s = dm1r_rowlk_access(r, get_status, ROW_ACC_RETURN, 
			&row_low_tran, &row_lg_id, &localtid, &tid_to_lock,
			r->rcb_partno,
			&save_lock_key, opflag, dberr);
		    if (s == E_DB_ERROR)
		    {
			dm0pUnlockBuf(r, &r->rcb_data);
			return (E_DB_ERROR);
		    }
		    if (s == E_DB_WARN)
			continue;
		}
		else if ( NeedCrowLock(r) )
		{
		    s = dm1r_crowlk_access(r, ROW_ACC_RETURN, 
					&localtid,
		    			&tid_to_lock,
		    			&save_lock_key, 
					opflag,
					dberr);
		    if ( s == E_DB_WARN )
			continue;
		    if (s == E_DB_ERROR)
		    {
			dm0pUnlockBuf(r, &r->rcb_data);
			return (E_DB_ERROR);
		    }
		}
	    } /* if GETNEXT */

	    /* 
	    ** Notify security system of row access, and request if
	    ** we can access it. This does any row-level security auditing
	    ** as well as MAC arbitration.
	    **
	    ** If the internal rcb flag is set then bypass 
	    ** security label checking.  This is only set for
	    ** DDL operation such as modify and index where you 
	    ** want all records, not just those you dominate.
	    */
	    if ( r->rcb_internal_req != RCB_INTERNAL && 
		    dmf_svcb->svcb_status & SVCB_IS_SECURE )
	    {
		i4       access;

		access = DMA_ACC_SELECT;
		/* 
		** If get by exact key tell DMA this
		*/
		if ( opflag & DM1C_BYTID )
		    access |= DMA_ACC_BYTID;
		else if ( (r->rcb_ll_given) && 
		      (r->rcb_ll_given == r->rcb_hl_given) &&
		      (r->rcb_ll_given == t->tcb_keys) &&
		      (r->rcb_ll_op_type == RCB_EQ) &&
		      (r->rcb_hl_op_type == RCB_EQ)
		   )
			access |= DMA_ACC_BYKEY;
		       
		s = dma_row_access(access, r, (char*)rec_ptr, (char*)NULL,
			    &compare, dberr);

		if ( s || compare != DMA_ACC_PRIV )
		{
		    if ( opflag & DM1C_GETNEXT && s == E_DB_OK )
			/*
			** Skip this row and continue
			*/
			continue;

		    uleFormat(NULL, E_DM938A_BAD_DATA_ROW, (CL_ERR_DESC *)NULL, 
			    ULE_LOG, NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, 4, sizeof(DB_DB_NAME), 
			    &t->tcb_dcb_ptr->dcb_name, sizeof(DB_TAB_NAME), 
			    &t->tcb_rel.relid, sizeof(DB_OWN_NAME), 
			    &t->tcb_rel.relowner, 0, localtid.tid_i4);
		    if ( opflag & DM1C_BYTID )
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		    /* Unlock buffer */
		    dm0pUnlockBuf(r, &r->rcb_data);
		    return (E_DB_ERROR);
		}
	    }

	    /*  Qualifying record found. return it. */

#ifdef xDEBUG
	    if (DMZ_AM_MACRO(21))
	    {
		if ( opflag & DM1C_GETNEXT )
		    TRdisplay("dm1r_get: Ending tid = %d,%d,currenttid = %d,%d\n", 
			r->rcb_lowtid.tid_tid.tid_page,
			r->rcb_lowtid.tid_tid.tid_line,
			localtid.tid_tid.tid_page,
			localtid.tid_tid.tid_line); 
		else
		    TRdisplay("dm1r_get: by tid = %d,%d\n", 
			localtid.tid_tid.tid_page, 
			localtid.tid_tid.tid_line); 
		dmd_prrecord(r, rec_ptr);
	    }
#endif

	    /* 
	    ** Finally, copy the record into the caller's buffer
	    ** unless it was already copied there.
	    */
	    if (rec_ptr != record)
		MEcopy(rec_ptr, record_size, record);

	    /* Unlock buffer */
	    dm0pUnlockBuf(r, &r->rcb_data);

	    if (row_locking(r)
		&& (r->rcb_iso_level == RCB_CURSOR_STABILITY)
		&& (r->rcb_state & RCB_CSRR_LOCK)
		&& (r->rcb_row_lkid.lk_unique))
	    {
		r->rcb_csrr_flags |= RCB_CS_ROW;
	    }

	    tid->tid_i4 = localtid.tid_i4;
	    r->rcb_currenttid.tid_i4 = localtid.tid_i4;
	    r->rcb_fetchtid.tid_i4 = localtid.tid_i4;
	    r->rcb_state &= ~RCB_FETCH_REQ;

	    return (E_DB_OK);
	}

	/* Only DM1C_GETNEXT gets here... */

	/* End of current page, calculate the next page. */
	if (r->rcb_tcb_ptr->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    /*
	    ** Lock buffer for get.
	    **
	    ** This may replace rcb_data.page with a CR page
	    */
	    dm0pLockBufForGet(r, &r->rcb_data, &s, dberr);
	    if ( s )
	        return(s);
	}

	/* Unlock row before we go to next page */
	if (row_locking(r) && (save_lock_key.lk_type == LK_ROW))
	{
	    if (r->rcb_iso_level != RCB_SERIALIZABLE)
		_VOID_ dm1r_unlock_row(r, &save_lock_key, &local_dberr);
	    save_lock_key.lk_type = 0;
	}

	if (row_locking(r) && !dm0pBufIsLocked(&r->rcb_data))
	    dmd_check(E_DM93F5_ROW_LOCK_PROTOCOL);

	if ((nextpageno = DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
					r->rcb_data.page)) == 0)
	{
	    if ((nextpageno = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
					r->rcb_data.page)) == 0 ||
		nextpageno > r->rcb_hightid.tid_tid.tid_page)
	    {
		/* Unfix/unlock last page in scan. */
                if (r->rcb_data.page)
                {
		    s = dm0p_unfix_page(r, DM0P_UNFIX,
		        &r->rcb_data, dberr);
		    if (s != E_DB_OK)
		        return (s);
                }

		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	}

	r->rcb_lowtid.tid_tid.tid_page = nextpageno;
	r->rcb_lowtid.tid_tid.tid_line = DM_TIDEOF;
    }
}

/*{
** Name: dm1r_put - Puts a record into a table.
**
** Description:
**      This routine puts a record into a table. The position of where
**      to put the record and the check for duplicates has already been
**      done prior to calling this routine.  This routine assumes that
**      the page containing the data has already been fixed, it always
**      leaves the page fixed.
**      This routine keeps page fixed after the put. 
**	This was added to optimize set appends when the
**	records will fall on the same page as the previous one.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      data                            Pointer to a pointer to the page
**                                      containing record to delete.
**      tid                             The TID of the record to insert.
**                                      Only set for data pages, TID for 
**                                      secondary indices is part of record.
**      record                          Pointer to the record to insert.
**      record_size                     Size in bytes of record. 
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**                                      those that can be returned by
**                                      dmop_fix_page and dm0p_unfix_page.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      12-dec-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	08-jun-92 (kwatts)
**	    6.5 MPF changes. dm1c_put becomes an accessor call.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project:  This routine now logs updates!
**	    Moved log writing from dm2r_put to this routine to fit in
**	    better with page oriented logging protocols.  Log records
**	    are now written while the page mutex is held.
**	    Took out the old SD code that turned on the page DMPP_DATA bit
**	    for all pages onto which an insert was done.
**	    Removed dmd_prrecord call when the row is compressed.
**	14-dec-92 (jnash)
**	    Reduced Logging Project: New params to dmpp_put.
**	    Added handling of journal status for catalog updates.  When a
**	    core catalog is updated (by a put, delete or replace), the journal
**	    status of the log record depends on the journal state of the user
**	    table which is being altered, not the journal status of the actual
**	    system catalog.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Change error handling to give traceback errors.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Took out references to rcb_usertab_jnl which used to dictate
**	    whether or not to journal system catalog updates.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Inserts of core
**	    catalog rows are not journaled if the base table which they 
**	    describe is not.  Full explanation of non journal table handling 
**	    given in DMRFP.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format 
**	    project.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO to the put accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          For Alter Table support,
**              Pass tcb_rel.relversion to dmpp_put.
**              Log the version of the row as well as the row.
**      12-dec-96 (dilma04)
**          Row-Level Locking Project:
**          As an optimization in case of row locking and non-unique hash 
**          table, release the value lock acquired for phantom protection.
**          The value lock is not needed anymore after a row is inserted
**          and locked.
**      10-mar-1997 (stial01)
**          Changes for tables with blob columns, high compression
**          Recent changes in blob processing make it possible for the allocate
**          length to be greater than put length. This can happen for
**          tables with blobs columns and high compression. 
**          If row locking, during the allocate we reserve 'allocate length'.
**          We do not allow the allocate to reclaim space.
**          If row locking when we put a record, if the allocate length
**          is greater than the put length, we first delete the 'reserved'
**          tuple. (instead of putting on top of the reserved tuple)
**      21-may-1997 (stial01)
**          dm1r_put() Added flags arg to dm0p_unmutex call(s)
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          Tables with blobs: delete reserved tuple after mutexing page.
**      24-jun-97 (dilma04)
**          Bug 83300. Moved code to release redundant value lock from
**          this routine to dm1r_allocate(), since pointer to uncompressed
**          record is required for dm1r_unlock_value() call. 
**      29-jul-97 (stial01)
**          Added opflag parameter, to log original operation mode if REPLACE
**      21-apr-98 (stial01)
**          dm1r_put() Set DM0L_PHYS_LOCK if extension table (B90677)
**      07-jul-98 (stial01)
**          dm1r_put() Deferred update: new update_mode if row locking
**      18-jab-2000 (gupsh01)     
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table.
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Flag PUT operations generated by sysmod so that iirealtion
**          tuples which represent non-journalled tables can be identifed
**          during rollforward processing.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          Check for lock protocol errors.
**	04-Jun-2005 (thaju02)
**	    Moved rcb_tup_adds++ from put() to dm1r_put().
**	20-Feb-2008 (kschendel) SIR 122739
**	    Test for compresses-coupons rather than compression type.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
DB_STATUS
dm1r_put(
    DMP_RCB	*rcb, 
    DMP_PINFO   *dataPinfo,
    DM_TID      *tid, 
    char        *record, 
    char        *key,
    i4     record_size,
    i4     opflag,
    DB_ERROR	*dberr )
{
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DB_STATUS	s;

    if (t->tcb_seg_rows)
    {
	s = dm1r_put_segs(rcb, dataPinfo, tid, record, key, record_size, 
		opflag, dberr);
    }
    else
    {
	s = put(rcb, dataPinfo, tid, record, key, record_size, opflag, 
		(DMPP_SEG_HDR *)NULL, dberr);
    }
  
    if (s == E_DB_OK)
	rcb->rcb_tup_adds++;

    return (s);
}

static DB_STATUS
put(
    DMP_RCB		*rcb, 
    DMP_PINFO   	*pinfo,
    DM_TID      	*tid, 
    char        	*record, 
    char        	*key,
    i4			record_size,
    i4			opflag,
    DMPP_SEG_HDR	*seg_hdr,
    DB_ERROR		*dberr )
{
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS	    status;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4         update_mode;
    i4         dm0l_flags;
    i4         loc_id;
    i4         loc_config_id;
    bool	logging = rcb->rcb_logging;
    i4		    *err_code = &dberr->err_code;
    LG_LRI	    lri;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** Consistency checks: make sure input data makes sense.
	** If row locking, we can look at page_page when the buffer is
	** not locked, since it is only set when the page is formatted
	*/
	if (pinfo->page == NULL || tid->tid_tid.tid_page != 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page))
	    dmd_check(E_DMF010_DM1R_NO_PAGE);

	/*
	** Build parameters and flags to LG and page routines.
	*/
	update_mode = DM1C_DIRECT;
	if (rcb->rcb_update_mode == RCB_U_DEFERRED)
	    update_mode = DM1C_DEFERRED;
	if ( row_locking(rcb) || crow_locking(rcb) || NeedPhysLock(rcb) )
	    update_mode |= DM1C_ROWLK;

	/* 
	** NB: If (c)row locking, we assume that dm1r_allocate has
	**     acquired the necessary (C)ROW lock on TID.
	*/

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if pinfo->page is
	** a CR page.
	*/
	dm0pLockBufForUpd(rcb, pinfo);
	
        if ((rcb->rcb_lk_type == RCB_K_PAGE) &&
                  DMZ_SES_MACRO(33))
        {
           DB_STATUS    tmp_status;

           /* Make certain that there is a logical page lock */

           tmp_status = dm1r_check_page_lock( rcb, tid->tid_tid.tid_page);

           if (tmp_status != E_DB_OK)
           {
              TRdisplay("dm1r_put: No logical page lock for %~t\n",
                  sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
		SETDBERR(dberr, 0, E_DM9C2C_NOPAGELOCK);
              status = E_DB_ERROR;
              break;
           }
        }

	/*
	** Get information on the location to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
					    (i4) tid->tid_tid.tid_page);
	loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(rcb, pinfo->page, dberr);
	    if (status != E_DB_OK)
	        break;
	}

	/*
	** NOBLOBLOGGING optimization
	** If nojournaling and FAST COMMIT is off, No REDO recovery is possible.
	** When bulk loading onto btree disassoc data pages,
	** DM0L_PUT logging is not needed for UNDO recovery (rollback)
	** All we need to log is the page allocations in case the 
	** insert is rolled back.
	*/
	if (t->tcb_extended &&
		    !is_journal(rcb) &&
		    ((d->dcb_status & DCB_S_FASTCOMMIT) == 0) &&
		    rcb->rcb_bulk_misc)
	{

	    logging = FALSE;

	    /* Protocol checks */
	    if (t->tcb_rel.relpgtype == DM_COMPAT_PGTYPE
		|| t->tcb_rel.relspec != TCB_BTREE
		|| t->tcb_rel.relcomptype != TCB_C_NONE
		|| (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pinfo->page)
			& DMPP_ASSOC))
	    {
		TRdisplay("nojournaling, NOBLOBLOGGING insert protocol error\n");
		SETDBERR(dberr, 0, E_DM9243_DM1R_PUT);
		status = E_DB_ERROR;

	    }
	}

	/*
	** Log the Insert operation.
	*/
	if (logging)
	{
	    /* 
	    ** Reserve logfile space for the log and the CLR.  The CLR does not 
	    ** include the row, just the id.  
	    */
	    cl_status = LGreserve(0, rcb->rcb_log_id, 2,
		sizeof(DM0L_PUT) + record_size + sizeof(DM0L_PUT),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, rcb->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9243_DM1R_PUT);
		break;
	    }

	    /*
	    ** Lock the buffer so nobody else can look at it while we are 
	    ** changing it.
	    */
	    dm0pMutex(tbio, (i4)0, rcb->rcb_lk_id, pinfo);

	    /*
	    ** Log journal and lock modes.
	    */
	    dm0l_flags = (is_journal(rcb) ? DM0L_JOURNAL : 0);

	    if ( rcb->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flags |= DM0L_MINI;

	    /*
	    ** The journal status of core catalog inserts depends upon
	    ** the journal status of the underlying user table.
	    */
	    if (t->tcb_sysrel)
		dm0l_flags = (rcb->rcb_usertab_jnl ? DM0L_JOURNAL : 0);
            else
	    {
		if (rcb->rcb_temp_iirelation)
                {
                    if (is_journal(rcb) && ( ! rcb->rcb_usertab_jnl))
		    {
			/* Bug 107828 - We are performing a PUT of a 
			** row into iirtemp which is a copy of an iirelation
			** tuple which describes a nn-journalled table.
			** Proably as a result of a sysmod.
			** Flag the journals so rollforwarddb through
			** journals can take corrective actions for
			** non-journal tables represented by iirelation
			** entries which are always journalled.
			*/
                        dm0l_flags |= (DM0L_NONJNL_TAB | DM0L_TEMP_IIRELATION);
		    }
                }
	    }

	    /*
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same lock protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(rcb) )
		dm0l_flags |= DM0L_PHYS_LOCK;
            else if (row_locking(rcb))
                dm0l_flags |= DM0L_ROW_LOCK;
	    else if ( crow_locking(rcb) )
                dm0l_flags |= DM0L_CROW_LOCK;

	    if (DMZ_TBL_MACRO(23))
	    {
		DM_TID		base_tid;

		if (t->tcb_rel.relstat & TCB_INDEX)
		    MEcopy((record + record_size) - sizeof(DM_TID), 
				sizeof(DM_TID), &base_tid);
		else
		    STRUCT_ASSIGN_MACRO(*tid, base_tid);

		status = dbg_dm723(rcb, tid, &base_tid, dm0l_flags, err_code);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, *err_code);
		    dm0pUnmutex(tbio, (i4)0, rcb->rcb_lk_id, pinfo);
		    break;
		}
	    }

	    /* Get CR info from page, pass to dm0l_put */
	    DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);

	    status = dm0l_put(rcb->rcb_log_id, dm0l_flags,
		    &t->tcb_rel.reltid, 
		    &t->tcb_rel.relid, t->tcb_relid_len,
		    &t->tcb_rel.relowner, t->tcb_relowner_len,
		    tid, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    t->tcb_rel.relcomptype, loc_config_id,
		    t->tcb_rel.relloccount, record_size, record, 
		    (LG_LSN *) 0, t->tcb_rel.relversion, 
		    seg_hdr, &lri, dberr);

	    if (status != E_DB_OK)
	    {
		dm0pUnmutex(tbio, (i4)0, rcb->rcb_lk_id, pinfo);
		break;
	    }
		
	    if((d->dcb_status & DCB_S_JOURNAL) && 
		    !(t->tcb_rel.relstat & TCB_JOURNAL))
	    {
		    if(rcb && rcb->rcb_xcb_ptr)
			    rcb->rcb_xcb_ptr->xcb_flags |= XCB_NONJNLTAB;	
	    }

	    /*
	    ** Write the LSN and other info about the insert record to the page
	    ** being updated.
	    ** This marks the version of the page for REDO processing and
	    ** CR undo.
	    */
	    DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, pinfo->page, &lri);


	}
	else
	    dm0pMutex(tbio, (i4)0, rcb->rcb_lk_id, pinfo);

	/*
	** Table with extensions (blobs) and high compression
	** If row locking we may have reserved more space than we
	** need. Delete reserved tuple first
	** (This is done because the coupon value may cause the row
	** to compress differently, larger/smaller.)
	*/
	if ((t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) 
	    && (t->tcb_data_rac.compresses_coupons))
	{
	    i4	    get_rec_size;
	    char            *get_rec;
	    DB_STATUS       get_status;

	    get_status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, pinfo->page, tid, &get_rec_size,
		    &get_rec, NULL, NULL, NULL, (DMPP_SEG_HDR *)NULL);

	    if (get_status == E_DB_WARN && get_rec != NULL)
	    {
		/* Deleted record */
		(*t->tcb_acc_plv->dmpp_reclen)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, pinfo->page, tid->tid_tid.tid_line,
			&get_rec_size);
		if (get_rec_size != record_size)
		{
		    (*t->tcb_acc_plv->dmpp_delete)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, pinfo->page, (i4)update_mode,
			(i4)TRUE, (DB_TRAN_ID *)NULL, (u_i2)0, tid, get_rec_size);
		}
	    }
	}

	/*
	** Write the new record to the data page.
	*/
	(*t->tcb_acc_plv->dmpp_put)(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    pinfo->page, update_mode, &rcb->rcb_tran_id, 
		    rcb->rcb_slog_id_id, tid, record_size,
		    record, t->tcb_rel.relversion, seg_hdr);

	/*
	** Do deferred put proessing if crow_locking so we can
	** ignore changes by our own transaction without mvcc undo
	*/
	if (rcb->rcb_update_mode == RCB_U_DEFERRED || crow_locking(rcb))
	{
	    status = (*t->tcb_acc_plv->dmpp_dput)(rcb, pinfo->page, 
					tid, err_code);
	    if (status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, *err_code);
		break;
	    }
	}

	/*
	** Release the buffer lock to allow other threads to see the page.
	**
	** Note that DM0P_MUNPIN will restore Root->CR
	*/
	dm0pUnmutex(tbio, (i4)DM0P_MUNPIN, rcb->rcb_lk_id, pinfo);

#ifdef xDEBUG
	if (DMZ_AM_MACRO(23))
	{
	    TRdisplay("dm1r_put: Returning, final data page is%d.\n",
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page)); 
	    TRdisplay("dm1r_put: tid = %d,%d.", 
			tid->tid_tid.tid_page, tid->tid_tid.tid_line);
	    if (t->tcb_data_rac.compression_type == TCB_C_NONE)
		dmd_prrecord(rcb, record);
	}
#endif

	return (E_DB_OK);
    }

    /*
    ** Unlock buffer.
    **
    ** This will restore Root->CR
    */
    dm0pUnlockBuf(rcb, pinfo);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9243_DM1R_PUT);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1r_replace - Replaces a record in a table.
**
** Description:
**    This routine replaces a record identified by tid from a
**    table.  This routine assumes that the new space has already 
**    been allocated and that duplicates have been checked.
**    If the old tid and new tid are identical and data is 
**    identical, this routine isn't called.
**
**    After deleting from a full hash overflow page, turn off the
**    FULLCHAIN bit on the hash primary page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**                                      index record.
**      tid                             A pointer to an area containing
**					the TID of the record to replace.
**      record                          A pointer to an area containing
**                                      the record to replace.
**      record_size                     Value indicating size of record
**                                      in bytes.
**      newdata                         A pointer to a pointer to the page
**                                      where new record is to be inserted.
**      newtid                          A pointer to an area containing
**                                      the TID of the new record if 
**                                      replace moved record.
**      newrecord                       A pointer to an area containing
**                                      the new record.
**      newrecord_size                  Value indicating size of record
**                                      in bytes.
**      opflag                          Value indicating if this a 
**                                      replace by tid
**                                      or by cursor position.  Must be
**                                      DM1C_BYTID OR DM1C_BYPOSITION.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**                                      E_DM0044_DELETED_TID, 
**                                      E_DM002A_BAD_PARAMETER.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      12-dec-85 (jennifer)
**          Converted for Jupiter.
**      02-Apr-87 (ac)
**          Added read/nolock support.
**	30-sep-1988 (rogerk)
**	    Added support for DMPP_FULLCHAIN for hash tables.  If delete tuple
**	    from hash overflow page, make sure the FULLCHAIN bit is not set
**	    anymore on the primary page.
**	08-jun-92 (kwatts)
**	    6.5 MPF changes. dm1c_del and dm1c_put become an accessor calls.
**	    Calculation of freespace becomes an accessor call.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project:  This routine now logs updates!
**	    Moved log writing from dm2r_replace to this routine to fit in
**	    better with page oriented logging protocols.  Log records
**	    are now written while the page mutex is held.  Took out the
**	    unfixing of the 'newdata' page after the replace as it was not
**	    not consistent with other dm1r calls and seemed to make no sense
**	    (changed newdata ptr to be passed by value instead of by ref).
**	    Removed dmd_prrecord call when the row is compressed.
**	14-dec-92 (jnash)
**	    Added handling of journal status for catalog updates.  When a
**	    core catalog is updated (by a put, delete or replace), the journal
**	    status of the log record depends on the journal state of the user
**	    table which is being altered, not the journal status of the actual
**	    system catalog.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.  
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Change error handling to give traceback errors.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Took out references to rcb_usertab_jnl which used to dictate
**	    whether or not to journal system catalog updates.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Replaces on core
**	    catalogs are journaled if the base table is journaled except for
**	    iirelation updates which are ALWAYS journaled.  Note that journal
**	    recovery of iirelation updates is done with special vodoo that
**	    only rolls forward selected fields in the iirelation row.  Full
**	    explanation of non journal table handling given in DMRFP.
**	18-oct-1993 (jnash)
**	    Include compression factor in replace log record LGreserve().
**	11-dec-1993 (rogerk)
**	    Changes to replace compression algorithms.  Added new parameters
**	    to the dm0l_repl_comp_info call and moved logic which determines
**	    when to do before image compression down into that routine.
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to 
**          this change the routine was called once for LGreserve, and once for
**          dm0l_replace().
**	24-jun-1994 (mikem)
**	    Bug #63556
**	    Fix integration error introduced in 23-may integration.  A line
**	    of code was meant to be moved but the integration duplicated it.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize to dmpp_getfree.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**	06-may-1996 (nanpr01)
**	    Change all page header access as macro for New Page Format 
**	    project.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get/put accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**	11-jun-96 (nick)
**	    When fetching the old tuple for logging purposes, obtain the
**	    current size as well.  If we don't do this, differences between
**	    the old_record_size input and the real size can arise because
**	    of garbage in compressed tuples resulting in different compressed
**	    lengths.
**      18-jul-1996 (ramra01 for bryanp)
**          For Alter Table support, extract row version for old row being
**              replaced. Log old version properly.
**          Pass tcb_rel.relversion to dmpp_put.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**          Pass reclaim_space to delete accessor.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DM1R_LK_CHECK flag if calling dm1r_lock_row() just to
**          make sure that we already have the row lock.
**      21-may-1997 (stial01)
**          dm1r_replace() Added flags arg to dm0p_unmutex call(s)
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          If row locking, don't bother to check/update the FULLCHAIN status,
**          when row locking we only do in place replace.
**      21-apr-98 (stial01)
**          dm1r_replace() Set DM0L_PHYS_LOCK if extension table (B90677)
**      07-jul-98 (stial01)
**          dm1r_replace() Deferred update: new update_mode if row locking
**      18-jab-2000 (gupsh01)     
**          Added support for setting transaction flag when the transaction
**          changed a non-journaled table.
**      13-May-2005(horda03) Bug 114508/INGSRV3301
**          Check for lock protocol errors.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: add row_tran_id, row_lg_id to dm0l_rep() prototype
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
dm1r_replace(
    DMP_RCB	*rcb, 
    DM_TID	*old_tid, 
    char	*old_row,
    i4		old_record_size,
    DMP_PINFO	*newdataPinfo,
    DM_TID	*new_tid, 
    char	*new_row,
    i4		new_record_size,
    i4		opflag,
    i4		delta_start,
    i4		delta_end,
    DB_ERROR	*dberr )
{
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DB_STATUS	s;

    if (t->tcb_seg_rows)
    {
	/* delta_start, delta_end ignored */
	s = dm1r_replace_segs(rcb, old_tid, old_row, old_record_size, 
			newdataPinfo, new_tid, new_row, new_record_size,
			opflag, dberr);
    }
    else
    {
	s = replace(rcb, old_tid, old_row, old_record_size, 
			newdataPinfo,
			new_tid, new_row, new_record_size,
			opflag, delta_start, delta_end, dberr);
    }
    return (s);
}

DB_STATUS
replace(
    DMP_RCB		*rcb, 
    DM_TID		*old_tid, 
    char		*old_row,
    i4			old_record_size,
    DMP_PINFO		*newdataPinfo,
    DM_TID		*new_tid, 
    char		*new_row,
    i4			new_record_size,
    i4			opflag,
    i4			delta_start,
    i4			delta_end,
    DB_ERROR		*dberr )
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    char	    *old_rec_ptr;
    DB_STATUS       status;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    LG_LSN	    lsn;
    i4         update_mode;
    i4         dm0l_flags;
    i4	    loc_id;
    i4	    oloc_config_id;
    i4	    nloc_config_id;
    i4	    rec_size;
    i4	    res_space;
    i4	    comp_odata_len;
    i4	    comp_ndata_len;
    i4	    diff_offset;
    i4	    row_version = 0;	
    char	    *comp_odata;
    char	    *comp_ndata;
    bool	    samepage_replace;
    bool	    inplace_replace;
    bool	    comp_orow;
    i4         reclaim_space;
    i4		    *err_code = &dberr->err_code;
    LG_LRI	    lri;
    DMP_PINFO	    *newPinfo;
    DMPP_PAGE	    *new_page;
    u_i4	    row_tran_id;
    u_i2	    row_lg_id;

    CLRDBERR(dberr);
 
#ifdef xDEBUG
    if (DMZ_AM_MACRO(24))
    {
	TRdisplay("dm1r_replace: tid = %d,%d, new_tid = %d,%d. ", 
	    old_tid->tid_tid.tid_page, old_tid->tid_tid.tid_line, 
	    new_tid->tid_tid.tid_page, new_tid->tid_tid.tid_line); 
	if (t->tcb_data_rac.compression_type == TCB_C_NONE)
	{
	    dmd_prrecord(r, old_row);
	    TRdisplay("dm1r_replace: New record. ");
	    dmd_prrecord(r, new_row);
	}
    }
#endif

    /*
    ** Consistency checks: make sure input data makes sense.
    */
    if ((r->rcb_data.page == NULL) || 
	(old_tid->tid_tid.tid_page != 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_data.page)))
    {
	dmd_check(E_DMF010_DM1R_NO_PAGE);
    }

    samepage_replace = (old_tid->tid_tid.tid_page == new_tid->tid_tid.tid_page);
    inplace_replace = (old_tid->tid_i4 == new_tid->tid_i4);

    /*
    ** Check if a separate page was passed in for the new version of the
    ** replace tuple.  If there wasn't, then the replace must be to the
    ** same page.  If this is the case, then verify that the tids refer
    ** to the same page.
    */
    if ( newdataPinfo->page == NULL )
    {
	if ( ! samepage_replace)
	    dmd_check(E_DMF011_DM1R_BAD_TID);
	newPinfo = &r->rcb_data;;
    }
    else
        newPinfo = newdataPinfo;

    new_page = newPinfo->page;

    for (;;)
    {

	if (row_locking(r) || crow_locking(r) )
	{
	    if (!inplace_replace)
	    {
		SETDBERR(dberr, 0, E_DM9C2B_REPL_ROWLK);
		status = E_DB_ERROR;
		break;
	    }

	    /* Ensure lock on TID if CROW locking */
	    if ( crow_locking(r) )
	    {
		/* Lock the TID if CROW locking */
	        status = dm1r_crow_lock(r, 0, old_tid, &r->rcb_data, dberr);
		if ( status )
		    break;
	    }
	    else
	    /* Make sure we already have the lock if replace to base table */
	    if ((t->tcb_rel.relstat & TCB_INDEX) == 0 && DMZ_SES_MACRO(33))
	    {
		DB_STATUS    tmp_status;

		tmp_status = dm1r_check_lock(r, old_tid);
		if (tmp_status != E_DB_OK)
		{
		    TRdisplay("dm1r_replace: No row lock for %~t\n",
			sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
		    SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if rcb_data.page
	    ** is a CR page.
	    */
	    dm0pLockBufForUpd(r, &r->rcb_data);
	    /*
	    ** NB: if crow_locking, "r->rcb_data.page" may now point to Root,
	    ** keep "new_page" in sync, ditto when we unlock.
	    */
	    new_page = r->rcb_data.page;
	}
        else if ( (r->rcb_lk_type == RCB_K_PAGE) &&
                  DMZ_SES_MACRO(33))
        {
           DB_STATUS    tmp_status;

           /* Make certain that there is a logical page lock */

           tmp_status = dm1r_check_page_lock( r, new_tid->tid_tid.tid_page);

           if (tmp_status != E_DB_OK)
           {
              TRdisplay("dm1r_replace: No logical page lock for %~t\n",
                  sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
		SETDBERR(dberr, 0, E_DM9C2C_NOPAGELOCK);
              status = E_DB_ERROR;
              break;
           }
        }

	/*
	** Get pointer to the genuine row on the old page.  The copy of the
	** row we need to log must be the actual non-compressed version.
	*/
	status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, r->rcb_data.page, old_tid, &rec_size,
		&old_rec_ptr, &row_version, &row_tran_id, &row_lg_id,
		(DMPP_SEG_HDR *)NULL);
	if (status != E_DB_OK)
	{
	    /* Crash with old_page still locked */
	    dmd_check(E_DMF011_DM1R_BAD_TID);
	}

	rec_size = old_record_size;
	if (t->tcb_data_rac.compression_type)
	{
	    (*t->tcb_acc_plv->dmpp_reclen)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, r->rcb_data.page, 
		old_tid->tid_tid.tid_line, &rec_size);
	}

	/*
	** Build parameters and flags to LG and page routines.
	*/
	update_mode = DM1C_DIRECT;
	if (r->rcb_update_mode == RCB_U_DEFERRED)
	    update_mode = DM1C_DEFERRED;	
	if ( row_locking(r) || crow_locking(r) || NeedPhysLock(r) )
	    update_mode |= DM1C_ROWLK;

	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
				    (i4) old_tid->tid_tid.tid_page);
	oloc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
				    (i4) new_tid->tid_tid.tid_page);
	nloc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, r->rcb_data.page, dberr);
	    if (status != E_DB_OK)
	        break;
	    if ( ! samepage_replace)
	    {
	        status = dm0p_before_image_check(r, new_page, dberr);
	        if (status != E_DB_OK)
		    break;
	    }
	}

	/*
	** Log the Replace operation.
	*/
	if (r->rcb_logging)
	{
	    /*
	    ** Calculate compressed row image lengths for the space reservation
	    ** call.  The comp_odata_len and comp_ndata_len values returned
	    ** here should match the compression done in dm0l_rep.
	    */
	    dm0l_repl_comp_info((PTR) old_rec_ptr, rec_size,
		(PTR) new_row, new_record_size, &t->tcb_rel.reltid,
		old_tid, new_tid, delta_start, delta_end, 
		&comp_odata, &comp_odata_len, &comp_ndata, &comp_ndata_len, 
		&diff_offset, &comp_orow);

	    /* 
	    ** Reserve logfile space for log and its CLR.  The CLR does not
	    ** include the new log record.  Space reserved for the replace
	    ** log record written below takes into account the replace log
	    ** record compression.  The reservation for the CLR, however,
	    ** reserves space for a full before image, even if DM902 is set.
	    */ 
	    res_space = (sizeof(DM0L_REP) + comp_odata_len + comp_ndata_len +
			sizeof(DM0L_REP) + rec_size);
	    cl_status = LGreserve(0, r->rcb_log_id, 2, res_space, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9244_DM1R_REPLACE);
		break;
	    }

	    /*
	    ** Lock the buffer(s) so nobody can look at them while they are 
	    ** changing.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &r->rcb_data);

	    if ( !samepage_replace )
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, newPinfo);

	    /*
	    ** Log journal and lock modes.
	    */
	    dm0l_flags = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flags |= DM0L_MINI;

	    if (t->tcb_sysrel)
	    {
		/* 
		** The journal state of core catalog updates depends on
		** the journal status of the underlying user table - except
		** for iirelation updates which are always journaled.  In
		** the latter case the journal flags must indicate the user 
		** table journal state.
		*/
		dm0l_flags = (r->rcb_usertab_jnl ? DM0L_JOURNAL : 0);
		if ((t->tcb_rel.reltid.db_tab_base == DM_B_RELATION_TAB_ID) &&
		    (t->tcb_rel.reltid.db_tab_index == DM_I_RELATION_TAB_ID) &&
		    (is_journal(r)) &&
		    ( ! r->rcb_usertab_jnl))
		{
		    dm0l_flags = (DM0L_JOURNAL | DM0L_NONJNL_TAB);
		}
	    }

	    /*
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same lock protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flags |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flags |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flags |= DM0L_CROW_LOCK;

	    if (DMZ_TBL_MACRO(23))
	    {
		DM_TID		base_tid;

		if (t->tcb_rel.relstat & TCB_INDEX)
		    MEcopy((old_rec_ptr + old_record_size) - sizeof(DM_TID), 
				sizeof(DM_TID), &base_tid);
		else
		    STRUCT_ASSIGN_MACRO(*old_tid, base_tid);

		status = dbg_dm723(r, old_tid, &base_tid, dm0l_flags, err_code);

		if (t->tcb_rel.relstat & TCB_INDEX)
		    MEcopy((new_row + new_record_size) - sizeof(DM_TID), 
				sizeof(DM_TID), &base_tid);
		else
		    STRUCT_ASSIGN_MACRO(*new_tid, base_tid);
		if (status == E_DB_OK)
		     status = dbg_dm723(r, new_tid, &base_tid, dm0l_flags, err_code);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, *err_code);

		    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &r->rcb_data);
		    if ( ! samepage_replace)
			dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, newPinfo);
		    break;
		}
	    }

	    /* Get CR info from page, pass to dm0l_rep */
	    DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page, &lri);

	    status = dm0l_rep(r->rcb_log_id, dm0l_flags,
		    &t->tcb_rel.reltid,
		    &t->tcb_rel.relid, t->tcb_relid_len,
		    &t->tcb_rel.relowner, t->tcb_relowner_len,
		    old_tid, new_tid, t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, t->tcb_rel.relcomptype,
		    oloc_config_id, nloc_config_id, t->tcb_rel.relloccount, 
		    rec_size, new_record_size, old_rec_ptr, new_row, 
		    (LG_LSN *) 0, 
		    comp_odata, comp_odata_len, comp_ndata, comp_ndata_len, 
		    diff_offset, comp_orow, row_version, t->tcb_rel.relversion,
		    row_tran_id,
		    row_lg_id,
		    &lri, dberr);	

	    if (status != E_DB_OK)
	    {
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &r->rcb_data);
		if ( ! samepage_replace)
		    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, newPinfo);
		break;
	    }

		if((d->dcb_status & DCB_S_JOURNAL) && 
			!(t->tcb_rel.relstat & TCB_JOURNAL))
		{
			if(r && r->rcb_xcb_ptr)
				r->rcb_xcb_ptr->xcb_flags |= XCB_NONJNLTAB;	
		}

	    /*
	    ** Write the LSN and info about the replace record to the page
	    ** being updated.
	    ** This marks the version of the page for REDO processing and
	    ** CR undo.
	    */
	    DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page, &lri);
	    if ( ! samepage_replace)
		DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, new_page, &lri);
	}
	else
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &r->rcb_data);
	    if ( ! samepage_replace)
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, newPinfo);
	}

	if (t->tcb_rel.relpgtype != TCB_PG_V1 ||
	    t->tcb_data_rac.compression_type == TCB_C_NONE ||
	    NeedPhysLock(r) )
	{
	    /* Physical locking */
	    reclaim_space = FALSE;
	}
	else
	    reclaim_space = TRUE;

	/*
	** If this is an inplace replace operation then just plop the new row
	** right on top of the old one.  Otherwise we have to delete the old
	** row and then insert the new one.
	*/
	if ( ! inplace_replace)
	{
	    /*
	    ** Do deferred put proessing if crow_locking so we can
	    ** ignore changes by our own transaction without mvcc undo
	    */
	    if (r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
	    {
		status = (*t->tcb_acc_plv->dmpp_dput)(r, r->rcb_data.page,
				    old_tid, err_code);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, *err_code);
		    break;
		}
	    }
	    (*t->tcb_acc_plv->dmpp_delete)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, r->rcb_data.page, update_mode, 
		    reclaim_space, &r->rcb_tran_id, 
		    r->rcb_slog_id_id,
		    old_tid, rec_size);
	}
	
	(*t->tcb_acc_plv->dmpp_put)(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
	    new_page, update_mode, &rcb->rcb_tran_id, r->rcb_slog_id_id,
	    new_tid,  new_record_size, new_row,
	    t->tcb_rel.relversion, (DMPP_SEG_HDR *)NULL);

	/*
	** Do deferred put proessing if crow_locking so we can
	** ignore changes by our own transaction without mvcc undo
	*/
	if (r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
	{
	    status = (*t->tcb_acc_plv->dmpp_dput)(r, new_page,
				new_tid, err_code);
	    if (status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, *err_code);
		break;
	    }
	}

	/*
	** Release the buffer lock to allow other threads to see the page.
	**
	** Note that DM0P_MUNPIN will restore Root->CR
	*/
	dm0pUnmutex(tbio, (i4)DM0P_MUNPIN, r->rcb_lk_id, &r->rcb_data);
	if ( ! samepage_replace)
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, newPinfo);


#ifdef xDEBUG
	if (DMZ_AM_MACRO(24))
	    TRdisplay("dm1r_replace: Replace finished.\n");
#endif

	/*
	** Update any other rcb's in our transaction that may be currently
	** pointing at this record to indicate that the record has been deleted.
	*/
	dm1r_rcb_update(r, old_tid, new_tid, DM1C_MREPLACE, dberr);

	/*
	** If this is a hash table, make sure that this hash chain that the
	** tuple was removed from is no longer marked as being full.
	**
	** If (c)row locking don't bother to check/update the FULLCHAIN status.
	** When row locking we only do in place replace.
	*/
	if ( (!row_locking(r) && !crow_locking(r))
	    && (r->rcb_tcb_ptr->tcb_rel.relspec == TCB_HASH))
	{
	    DM_PAGENO   bucket;
	    i4	    space;

	    /*
	    ** Don't bother to check/update the FULLCHAIN status of the
	    ** bucket if:
	    **   - the delete and insert are to same page.
	    **   - the delete is on a primary page.
	    **   - rcb_hash_nofull is set to this bucket (rcb_hash_nofull
	    **     holds the last visited pimary page which did not have
	    **     FULLCHAIN set)
	    **   - the page was not full previous to the delete.
	    */
	    if ((DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page) >= 
		  r->rcb_tcb_ptr->tcb_rel.relprim) && (! samepage_replace))
	    {
		bucket = r->rcb_tcb_ptr->tcb_rel.relprim - 1;
		if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,r->rcb_data.page) > 0)
		    bucket = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, 
						r->rcb_data.page) - 1;
		if (r->rcb_hash_nofull != bucket)
		{
		    space = (*t->tcb_acc_plv->dmpp_getfree)(r->rcb_data.page,
					t->tcb_rel.relpgsize);
		    if (space < (rec_size * 2))
			dm1h_nofull(r, bucket);
		}
	    }
	}

	return (E_DB_OK);
    }

    /*
    ** Unlock buffer.
    **
    ** This will restore Root->CR
    */
    dm0pUnlockBuf(r, &r->rcb_data);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9244_DM1R_REPLACE);
    }

    return (status);
}

/*{
** Name: dm1r_rcb_update - Updates all RCB's of a single transaction.
**
** Description:
**    This routine updates all the RCB's associated with one transaction.
**    The information in the RCB relating to current postion may change
**    depending upon the action taken with the current RCB.  For example
**    if the current RCB deletes a record, any other RCB pointing
**    to that record as fetched, must be marked as pointing to 
**    a deleted record.  Or if you are replacing a record, all other
**    RCB's pointing to that record as fetched, must be told to 
**    refetch and if it moves, to point to the new one.
**    Therefore, all other RCB's associated with this transaction
**    must have their current fetch pointers updated if they were pointing to
**    the record we are operating on.
**
**    For example, if one RCB fetched
**    a record (tid = 2,3) and another RCB fetches the same record
**    each is pointing to 2,3 for the next operation.  If the first one
**    does a replace which moves it to 14,4, then the other one must
**    know to refetch the record at 14,4 instead of using the one
**    it fetched previously.  The is handled by tracking the fetched tid
**    in rcb_fetchtid and by indicating the state with rcb_state
**    set to RCB_FETCH_REQ on.  These are both cleared by the
**    next get.
**    
**
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      tid                             A pointer to a TID indicating 
**                                      position on data page for
**                                      record changed.
**      newtid                          A pointer to a TID indicating
**                                      position on data page for
**                                      replaces which move the record.
**                                      Or the newbid after a split.
**      opflag                          A value indicating type of operation
**                                      which changed an index record. Must
**                                      be DM1C_MDELETE or 
**                                      DM1C_MREPLACE.
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      01-oct-86 (jennifer)
**          Created for Jupiter.
*/
DB_STATUS
dm1r_rcb_update(
    DMP_RCB        *rcb, 
    DM_TID         *tid,
    DM_TID         *newtid,
    i4         opflag,
    DB_ERROR       *dberr )
{
    DMP_RCB	*r = rcb;
    DM_TID      localtid;
    DM_TID      localnewtid;
    DMP_RCB     *next_rcb;

    CLRDBERR(dberr);

    switch (opflag)
    {
    case DM1C_MDELETE:

	localtid.tid_i4 = tid->tid_i4;
	next_rcb = r;
	do
	{
	    if (next_rcb->rcb_fetchtid.tid_i4 == localtid.tid_i4)
	    {
		/* For any rcb which has already fetched this
		** tid, then mark it as deleted by placing an
                ** illegal tid in the fetched tid.
                */

		next_rcb->rcb_fetchtid.tid_tid.tid_page = 0;
		next_rcb->rcb_fetchtid.tid_tid.tid_line = DM_TIDEOF;
		next_rcb->rcb_state |= RCB_FETCH_REQ;
	    }
	} while ((next_rcb = next_rcb->rcb_rl_next) != r);
        return (E_DB_OK);

    case DM1C_MREPLACE:
        localtid.tid_i4 = tid->tid_i4;
	localnewtid.tid_i4 = newtid->tid_i4;
	next_rcb = r;
	do
	{

	    if (next_rcb->rcb_fetchtid.tid_i4 == localtid.tid_i4)
	    {
		/* For any rcb which has already fetched this
		** tid, then track where it has moved to.
                */

		if (localtid.tid_i4 != localnewtid.tid_i4)
		    next_rcb->rcb_fetchtid.tid_i4 = localnewtid.tid_i4;
		next_rcb->rcb_state |= RCB_FETCH_REQ;
	    }
	} while ((next_rcb = next_rcb->rcb_rl_next) != r);
        return (E_DB_OK);

    default:
	return (E_DB_OK);
    }
}

/* 
** Name: dm1r_crow_lock	    - acquire a logical Consistent Read row lock
**
** Description:
**
**	Rows in MVCC tables are logically X locked when about to
**	be updated.
**
**	Qualifying rows in a constraint are logically X locked.
**
**	Unlike regular row locking, Consistent Read (CROW) locks never
**	have an covering page lock.
**
**	When the lock is granted, we check the row on the Root page
**	for update conflicts. If the row was updated by a later-starting
**	transaction or by a transaction that was uncommitted when this
**	transaction began, then an update conflict exists, the first
**	updater wins, the lock is released, and this update cannot proceed:
**
**	    SERIALIZABLE	return E_DM0020_UNABLE_TO_SERIALIZE
**				to abort the transaction.
**	    READ_COMMITTED	return E_DM0029_ROW_UPDATE_CONFLICT.
**				When QEF sees this error code, it will
**				retry the statement.
**
**
** Inputs:
**	rcb		    - The RCB control block
**	tid		    - the row we're locking.
**	flags		    - LK_NOWAIT, LK_PHYSICAL or zero.
**			      LK_NOWAIT is used whenever the buffer
**			      is locked, to avoid deadlock.
**			      LK_PHYSICAL is used when duplicate
**			      checking.
**	page		    - pointer to pointer to page where TID lives,
**			      NULL when LK_PHYSICAL duplicate checking.
**
** Outputs:
**	dberr		    - reason for error return
**
** Returns:
**	E_DB_OK	                    Lock is granted.
**	E_DB_ERROR	            Lock is not granted.
**
** Side Effects:
**      Table may be escalated to X lock.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Invented.
**	10-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Fix misuse of LK_LOCK_TYPE_MEANING to
**	    RCB_LK_TYPE.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Update conflict resolution was being skipped
**	    if the row was pre-locked, like by an update cursor.
**	26-Feb-2010 (jonj)
**	    CRIB to use now anchored in RCB.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC: Blob support: simply return if tcb_extended;
**	    the row containing the blob column has already been locked
**	    and checked for update conflicts.
*/
DB_STATUS
dm1r_crow_lock(
DMP_RCB		*rcb,
i4		flags,
DM_TID 		*tid,
DMP_PINFO	*pinfo,
DB_ERROR	*dberr)
{
    DMP_RCB	*r = rcb;
    DML_XCB	*xcb = r->rcb_xcb_ptr;
    LG_CRIB	*crib = r->rcb_crib_ptr;
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_DCB	*d = t->tcb_dcb_ptr;
    DB_TAB_ID	row_reltid;
    DB_TAB_NAME *relid;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    DB_STATUS	status, get_status, local_status;
    i4		msgid = 0;
    DB_ERROR	local_dberr;
    i4		local_err;
    LK_LOCK_KEY lock_key;
    LK_LKID	lock_id;
    u_i4	row_low_tran;
    u_i2	row_lg_id;
    i4		record_size;
    i4		i;
    char	*rec_ptr;
    u_i4	RootPageStat;
    u_i4	LastTranid;
    DMP_PINFO	*PinfoPtr;
    DML_XCB	    *next_r_queue;
    DMP_RCB	    *next_rcb;

    CLRDBERR(dberr);

    /*
    ** If not CROW locking, we shouldn't be here.
    **
    ** If blob etab, just go back.
    */
    if ( !crow_locking(r) || t->tcb_extended )
        return(E_DB_OK);

    /* If this tid is already logically crow-locked and conflict-checked, return */
    if ( r->rcb_crow_tid.tid_i4 == tid->tid_i4 )
        return(E_DB_OK);

    lock_id.lk_unique = 0;
    lock_id.lk_common = 0;

    /* Determine TID's reltid */
    relid = &t->tcb_parent_tcb_ptr->tcb_rel.relid;	/* for locktrace */
    if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
    {
	DMT_PHYS_PART	*pp;

	/*
	** Then we have to find the reltid of the
	** TID's partition.
	**
	** Caller of dm1r_crow_lock() must have placed
	** the partition number of the TID's row
	** into "rcb_partno".
	**
	** We use that to index into the Master
	** TCB's partition array to extract the
	** reltid of the partition.
	*/

	pp = &t->tcb_parent_tcb_ptr->tcb_pp_array[r->rcb_partno];
	row_reltid = pp->pp_tabid;

	/* Partition TCB ought to be open, but be careful;  this bit is
	** just for lock-tracing and messages.
	*/
	if ( pp->pp_tcb != NULL )
	    relid = &pp->pp_tcb->tcb_rel.relid;

    }
    else
    {
	/* Use base table / partition reltid */
	row_reltid = t->tcb_parent_tcb_ptr->tcb_rel.reltid;
    }

    /* Construct the lock key */

    /* Compatible with LK_ROW locks, but without covering page locks */
    lock_key.lk_type = LK_ROW;
    lock_key.lk_key1 = d->dcb_id;
    lock_key.lk_key2 = row_reltid.db_tab_base;
    lock_key.lk_key3 = row_reltid.db_tab_index;
    lock_key.lk_key4 = tid->tid_tid.tid_page;
    lock_key.lk_key5 = tid->tid_tid.tid_line;
    lock_key.lk_key6 = 0;

    /* Output a lock trace message if tracing is enabled. */
    if (DMZ_SES_MACRO(1))
	dmd_lock(&lock_key, r->rcb_lk_id, LK_REQUEST, flags, LK_X,
	    r->rcb_timeout, &r->rcb_tran_id, relid, &d->dcb_name);

    /* Request a logical/physical lock on the row. */

    cl_status = LKrequest(flags | LK_STATUS, r->rcb_lk_id, &lock_key, LK_X,
	    (LK_VALUE *)NULL, &lock_id, r->rcb_timeout, &sys_err);

    for (;;)
    {
	/* If lock was granted... */
	if ( cl_status == OK || cl_status == LK_NEW_LOCK || cl_status == LK_INTR_GRANT )
	{
	    status = E_DB_OK;

	    /*
	    ** Extract the row's low_tran and check if the row has been
	    ** updated inconsistent with our CR point in time.
	    **
	    ** But we only need to do conflict resolution once;
	    ** when we've done it, set rcb_crow_tid to this tid
	    ** so we don't do it again.
	    */

	    if ( pinfo &&
	         crow_locking(r) &&
		 !(flags & LK_PHYSICAL) &&
		 lock_id.lk_unique &&
		 r->rcb_crow_tid.tid_i4 != tid->tid_i4 )
	    {
		bool	wePinned = FALSE;

		/* Remember that we've done conflict resolution on this TID */
	        r->rcb_crow_tid.tid_i4 = tid->tid_i4;

		/*
		** Root page must be pinned while we peek at it 
		** if compressed, in which case the tuple header
		** offset can change underneath us while in dmpp_get.
		*/

		/*
		** If we need to pin Root, do so.
		**
		** This will swap from CR->Root if pinfo->page
		** is a CR page.
		*/
		if (t->tcb_data_rac.compression_type != TCB_C_NONE &&
			    !dm0pBufIsLocked(pinfo) )
		{
		    dm0pLockRootForGet(r, pinfo);
		    wePinned = TRUE;
		}

		/* Extract page stat from Root for debugging */
		RootPageStat = DMPP_VPT_GET_PAGE_STAT_MACRO(
			t->tcb_rel.relpgtype, pinfo->Rootpage);

		/* Get TID's low_tran, lg_id from Root row */
		get_status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, pinfo->Rootpage, tid, &record_size,
			&rec_ptr, NULL, &row_low_tran, &row_lg_id, NULL);

		/*
		** Unpin Root buffer if we had to pin it.
		**
		** This will restore Root->CR
		*/
		if ( wePinned )
		    dm0pUnlockBuf(r, pinfo);

		/*
		** NB: rcb_tran_id is the "real" transaction id used
		**     to mark page/row updates.
		**
		**	   crib_bos_tranid is an artificial tranid acquired
		**	   to mark the beginning of a statement when Read Committed
		**	   and will never appear in any page or row as
		**	   the "tranid of update".
		**
		**	   When Serializable, crib_bos_tranid will be the
		**	   real rcb_tran_id.
		*/	   
		if ( row_low_tran && row_low_tran != r->rcb_tran_id.db_low_tran )
		{
		    /*
		    ** If row has been updated by a later-starting transaction,
		    ** then it wins the race.
		    **
		    ** As we only cache only db_low_tran and discard db_high_tran,
		    ** watch for db_low_tran wrap. svcb_last_tranid is the 
		    ** last transaction id assigned by LG.
		    **
		    ** If READ_COMMITTED, return E_DM0029_ROW_UPDATE_CONFLICT
		    ** to QEF. That'll induce it to retry the statement with
		    ** a fresh CRIB and crib_bos_tranid.
		    */
		    VOLATILE_ASSIGN_MACRO(dmf_svcb->svcb_last_tranid, LastTranid);

		    if ( row_low_tran > crib->crib_bos_tranid ||
			 (row_low_tran < crib->crib_bos_tranid &&
			  crib->crib_bos_tranid > LastTranid &&
			  row_low_tran <= LastTranid) )
		    {
			if ( r->rcb_iso_level == RCB_SERIALIZABLE ||
			     r->rcb_state & RCB_CURSOR )
			{
			    SETDBERR(dberr, 0, E_DM0020_UNABLE_TO_SERIALIZE);
			}
			else
			    SETDBERR(dberr, 0, E_DM0029_ROW_UPDATE_CONFLICT);
			status = E_DB_ERROR;
		    }

		    /*
		    ** If there were uncommitted transactions at our CR point in time
		    ** and this update was made by one of them, then the earlier
		    ** transaction wins.
		    **
		    ** If READ_COMMITTED, return E_DM0029_ROW_UPDATE_CONFLICT
		    ** to QEF. That'll induce it to retry the statement with
		    ** a fresh CRIB and new crib_bos_tranid.
		    */
		    else if ( crib->crib_lgid_low )
		    {
			if ( row_lg_id )
			{
			    if ( CRIB_XID_WAS_ACTIVE(crib, row_lg_id, row_low_tran) )
			    {
				if ( r->rcb_iso_level == RCB_SERIALIZABLE ||
				     r->rcb_state & RCB_CURSOR )
				{
				    SETDBERR(dberr, 0, E_DM0020_UNABLE_TO_SERIALIZE);
				}
				else
				    SETDBERR(dberr, 0, E_DM0029_ROW_UPDATE_CONFLICT);
				status = E_DB_ERROR;
			    }
			}
			else
			{
			    /*
			    ** row_lg_id unknown (V3/V4), have to do it the slow way,
			    ** scan the crib xid_array for matching tran_id.
			    */
			    for ( i = crib->crib_lgid_low;
				  i <= crib->crib_lgid_high && status == E_DB_OK;
				  i++ )
			    {
				if ( crib->crib_xid_array[i] == row_low_tran )
				{
				    if ( r->rcb_iso_level == RCB_SERIALIZABLE ||
					 r->rcb_state & RCB_CURSOR )
				    {
					SETDBERR(dberr, 0, E_DM0020_UNABLE_TO_SERIALIZE);
				    }
				    else
					SETDBERR(dberr, 0, E_DM0029_ROW_UPDATE_CONFLICT);
				    status = E_DB_ERROR;
				}
			    }
			}
		    }

		    if ( get_status && status == E_DB_OK )
		    {
			/* This is always due to wrongheaded make_consistent */
			TRdisplay("%@ dm1r_crow_lock:%d rcb %p tran %x page %p DELETED TID: [%d,%d]"
				  " row_lg_id %d row_low_tran %x %w\n",
				__LINE__,
				r,
				r->rcb_tran_id.db_low_tran,
				pinfo->Rootpage,
				tid->tid_tid.tid_page, 
				tid->tid_tid.tid_line, 
				row_lg_id,
				row_low_tran,
				RCB_ISO_LEVEL, r->rcb_iso_level);

			for (next_r_queue = (DML_XCB*) xcb->xcb_rq_next;
			    next_r_queue != (DML_XCB*) &xcb->xcb_rq_next;
			    next_r_queue = next_r_queue->xcb_q_next)
			{
			    next_rcb = (DMP_RCB *)((char *)next_r_queue -
				((char *)&((DMP_RCB*)0)->rcb_xq_next));
			    TRdisplay("open rcb %x tcb %x for %d,%d\n",
				next_rcb, next_rcb->rcb_tcb_ptr, 
				next_rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base,
				next_rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_index);
			}

			dmd_check(E_DM0029_ROW_UPDATE_CONFLICT);
		    }

		    /*
		    ** On seeing E_DM0029_ROW_UPDATE_CONFLICT, 
		    ** QEF will retry the statement.
		    **
		    ** I don't know if QEF will use the same statement sequence
		    ** number or not, but to ensure that the CRIB
		    ** is refreshed, decrement the CRIB's crib_sequence.
		    **
		    ** See dmt_open...
		    */
		    if ( status & dberr->err_code == E_DM0029_ROW_UPDATE_CONFLICT )
		    {
			/*
			** If retrying and bos_lsn hasn't advanced, then
			** abandon this statement by aborting the transaction.
			*/
			if ( LSN_EQ(&crib->crib_bos_lsn, &xcb->xcb_retry_bos_lsn) )
			    SETDBERR(dberr, 0, E_DM0020_UNABLE_TO_SERIALIZE);
			else
			{
			    /*
			    ** Remember the bos_lsn on which we're about to
			    ** retry. 
			    **
			    ** If we end up back here during retry and the 
			    ** bos_lsn hasn't advanced, then this statement
			    ** is futile and won't make any progress.
			    */

			    xcb->xcb_retry_bos_lsn = crib->crib_bos_lsn;

			    crib->crib_sequence--;

			    /* Count another ReadCommitted retry stat */
			    xcb->xcb_s_rc_retry++;
			    dmf_svcb->svcb_stat.rc_retry++;

			    /*
			    ** Before returning to let QEF/SCF retry the statement,
			    ** undo any changes made by this statement.
			    **
			    ** crib_bos_lsn holds the transaction's last LSN
			    ** at the time the statement started; this is where
			    ** we need to roll back to, noting that it may be
			    ** the same as crib_last_commit if this transaction
			    ** hasn't done any LGwrites up to this point.
			    **
			    ** To initiate a rollback to savepoint, we must pass
			    ** a savepoint id to dmxe_abort. It doesn't matter what
			    ** the spid is as long as it's non-zero, so we'll use
			    ** crib_bos_tranid as the savepoint id.
			    */
			    if ( LSN_GTE(&crib->crib_bos_lsn, &crib->crib_last_commit) )
			    {
				local_status = dmxe_abort(xcb->xcb_odcb_ptr,
							  d,
							  &r->rcb_tran_id,
							  r->rcb_log_id,
							  r->rcb_lk_id,
							  (is_journal(r)) ? DMXE_JOURNAL : 0,
							  (i4)crib->crib_bos_tranid,
							  &crib->crib_bos_lsn,
							  &local_dberr);

				/* If that failed, then abort the transaction */
				if ( local_status )
				{
				    uleFormat(&local_dberr, 0, &sys_err, ULE_LOG,
					NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
				    SETDBERR(dberr, 0, E_DM0020_UNABLE_TO_SERIALIZE);
				}
			    }
			}
		    }
		    else
			xcb->xcb_retry_bos_lsn.lsn_high = 
			    xcb->xcb_retry_bos_lsn.lsn_low = 0;

		    if ( status )
		    {
			r->rcb_crow_tid.tid_tid.tid_page = 0;
			r->rcb_crow_tid.tid_tid.tid_line = DM_TIDEOF;

			if ( d->dcb_status & DCB_S_MVCC_TRACE )
			    TRdisplay("%@ dm1r_crow_lock:%d %x tran %x %w op %d get_status %d %~t %w\n"
				  "\t PAGE: Root %v page %v\n"
				  "\t ROW:  TID [%d,%d] row_low_tran %x row_lg_id %d\n"
				  "\t CRIB: bos_tran %x low_lsn %x commit %x bos_lsn %x\n"
				  "\t       seq %d lg_low %d lg_high %d\n",
				    dberr->err_line,
				    dberr->err_code,
				    r->rcb_tran_id.db_low_tran,
				    RCB_LK_TYPE, r->rcb_lk_type,
				    r->rcb_dmr_opcode, get_status,
				    sizeof(DB_TAB_NAME), relid,
				    RCB_ISO_LEVEL, r->rcb_iso_level,
				    PAGE_STAT, RootPageStat,
				    PAGE_STAT, DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pinfo->Rootpage),
				    tid->tid_tid.tid_page,
				    tid->tid_tid.tid_line,
				    row_low_tran, row_lg_id,
				    crib->crib_bos_tranid,
				    crib->crib_low_lsn.lsn_low,
				    crib->crib_last_commit.lsn_low,
				    crib->crib_bos_lsn.lsn_low,
				    crib->crib_sequence,
				    crib->crib_lgid_low,
				    crib->crib_lgid_high);

			/*
			** If unable to serialize, log the error now so we can
			** report the TID, table, and db.
			*/
			if ( dberr->err_code == E_DM0020_UNABLE_TO_SERIALIZE )
			{
			    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 4,
				0, tid->tid_tid.tid_page,
				0, tid->tid_tid.tid_line,
				sizeof(DB_TAB_NAME), relid,
				sizeof(DB_DB_NAME), &d->dcb_name);
			}
		    }
		}
	    }

	    /*
	    ** If physical lock granted (dupcheck) or retryable update conflict,
	    ** then release the (logical) lock, if we acquired one and it's "new".
	    **
	    ** LK_RELLOG let's LKrelease know it's ok to release this single logical
	    ** lock rather than just ignoring the release.
	    */
	    if ( flags & LK_PHYSICAL || 
		    (dberr->err_code == E_DM0029_ROW_UPDATE_CONFLICT &&
		     cl_status == LK_NEW_LOCK && lock_id.lk_unique) )
	    {
		(void) LKrelease(LK_RELLOG, r->rcb_lk_id, &lock_id,
				(LK_LOCK_KEY*)NULL, (LK_VALUE*)NULL,
				&sys_err);
	    }
	    
	    break;
	}
	else if (cl_status == LK_NOLOCKS)
	{
	    uleFormat(NULL, E_DM004B_LOCK_QUOTA_EXCEEDED, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    msgid = I_SX2739_LOCK_LIMIT;
	}
	else if (cl_status == LK_DEADLOCK)
	{

	    if ( d->dcb_status & DCB_S_MVCC_TRACE )
		TRdisplay("%@ dm1r_crow_lock:%d tran %x %~t [%d,%d] row deadlock\n",
			__LINE__,
			r->rcb_tran_id.db_low_tran,
			sizeof(DB_TAB_NAME), relid,
			tid->tid_tid.tid_page,
			tid->tid_tid.tid_line);

	    uleFormat(NULL, E_DM905A_ROW_DEADLOCK, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 5,
		0, tid->tid_tid.tid_page, 0, tid->tid_tid.tid_line,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &d->dcb_name, 
		0, LK_X);

	    /*
	    ** DM304 enables tracing of lockstat on deadlock.
	    ** This occurs always in the RCP, as it should never encounter 
	    ** deadlocks.
	    */
	    if ((DMZ_LCK_MACRO(4)) || 
		    (d->dcb_status & DCB_S_RECOVER))
	    {
		dmd_lkrqst_trace(dmd_put_line, &lock_key, r->rcb_lk_id, 
		    LK_REQUEST, flags, LK_X, 
		    r->rcb_timeout, &r->rcb_tran_id,
		    relid, &d->dcb_name);
		dmd_lock_info(DMTR_LOCK_LISTS | DMTR_LOCK_USER_LISTS |
			DMTR_LOCK_SPECIAL_LISTS | DMTR_LOCK_RESOURCES );
	    }

	    SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
	    msgid = I_SX2753_ROW_DEADLOCK;
	}
	else if (cl_status == LK_TIMEOUT)
	{
	    char            msgbuf[64];

	    STprintf(msgbuf, "CROW %d (of page %d)", lock_key.lk_key5,
		lock_key.lk_key4);
	    uleFormat(NULL, E_DM9043_LOCK_TIMEOUT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 6,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &d->dcb_name,
		0, LK_X, sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, msgbuf, 0, sys_err.moreinfo[0].data.string);
	    SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	     msgid = I_SX2738_LOCK_TIMEOUT;
	}
	else if (cl_status == LK_RETRY)
	{
	    uleFormat(NULL, E_DM9047_LOCK_RETRY, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 3,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &d->dcb_name, 
		0, LK_X);
	    SETDBERR(dberr, 0, E_DM004F_LOCK_RETRY);
	}
	else if (cl_status == LK_INTERRUPT)
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	else if ( cl_status == LK_BUSY )
	    SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
	else
	{
	    TRdisplay("CROW lock failed for table [%d,%d] Tid %d,%d status %d,%d\n",
		    lock_key.lk_key2, lock_key.lk_key3, lock_key.lk_key4,
		    lock_key.lk_key5, cl_status, sys_err);
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    uleFormat(NULL, E_DM905B_BAD_LOCK_ROW, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 6,
		0, tid->tid_tid.tid_page, 0, tid->tid_tid.tid_line,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &d->dcb_name, 0,
		LK_X, 0, r->rcb_lk_id);
	    SETDBERR(dberr, 0, E_DM9078_ROW_LOCK_ERROR);
	}

	/*
	** Write an audit record if needed 
	*/
	if ( msgid != 0 && dmf_svcb->svcb_status & SVCB_C2SECURE )
	{
	    (void)dma_write_audit(
		     SXF_E_RESOURCE,
		     SXF_A_FAIL | SXF_A_LIMIT,
		     (char*)&t->tcb_rel.reltid,	/* Table name */
		     sizeof(DB_TAB_NAME),		/* Table name */
		     &t->tcb_rel.relowner,
		     msgid,
		     FALSE, /* Not force */
		     &local_dberr, NULL);
	}

	status = E_DB_ERROR;

	/*
	** if allocate nowait, we have buffers locked
	** return and let caller unlock the buffers and do the escalate
	*/
	if ( dberr->err_code == E_DM004B_LOCK_QUOTA_EXCEEDED &&
	     !(flags & LK_NOWAIT) )
	{
	    /*
	    ** Escalate to table level locking because 
	    ** 'locks per transaction' limit exceeded.
	    **
	    ** As part of the escalation process, we must ensure that
	    ** the RCBs and "page" pointers are set to the Root page,
	    ** not the CR page, as we'll no longer be crow locking
	    ** once we've escalated.
	    */
	    uleFormat(NULL, E_DM9041_LK_ESCALATE_TABLE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		       NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 
		       2, sizeof(DB_TAB_NAME), relid, 
		       sizeof(DB_DB_NAME), &d->dcb_name);

	    if ( d->dcb_status & DCB_S_MVCC_TRACE )
		TRdisplay("%@ dm1r_crow_lock:%d tran %x op %d %~t [%d,%d] escalating to Table\n",
			__LINE__,
			r->rcb_tran_id.db_low_tran,
			r->rcb_dmr_opcode,
			sizeof(DB_TAB_NAME), relid,
			tid->tid_tid.tid_page,
			tid->tid_tid.tid_line);

	    /* NULL if not one of The Big Two */
	    PinfoPtr = NULL;

	    if ( pinfo )
	    {
		if ( pinfo->page == r->rcb_data.page )
		    PinfoPtr = &r->rcb_data;
		else if ( pinfo->page == r->rcb_other.page )
		    PinfoPtr = &r->rcb_other;
	    }

	    status = dm0p_escalate(r, 0, dberr);

#ifdef xDEBUG
	    if (status != E_DB_OK)
		TRdisplay("dm1r_crow_lock escalate error %d\n", dberr->err_code);
#endif

	    /* Escalate failure may return E_DB_FATAL, map it to E_DB_ERROR */
	    if (status == E_DB_OK)
	    {
		/*
		** OK, now we hold a LK_X on the Table.
		**
		** If "page" was one of the Big Two and a CR page,
		** dm0p_escalate has unfixed the CR page and replaced
		** its pointer with Root.
		**
		** If "page" isn't one of those two, do the UNFIX_CR
		** here.
		**
		** Once all that's taken care of, loop -way- back with 
		** what's now the Root page and check it for update
		** conflicts.
		*/
		if ( d->dcb_status & DCB_S_MVCC_TRACE )
		    TRdisplay("%@ dm1r_crow_lock:%d tran %x op %d %~t [%d,%d] escalated to Table\n",
			__LINE__,
			r->rcb_tran_id.db_low_tran,
			r->rcb_dmr_opcode,
			sizeof(DB_TAB_NAME), relid,
			tid->tid_tid.tid_page,
			tid->tid_tid.tid_line);

		if ( pinfo )
		{
		    if ( PinfoPtr )
			*pinfo = *PinfoPtr;
		    else if ( DMPP_VPT_IS_CR_PAGE(t->tcb_rel.relpgtype, pinfo->page) )
		    {
			status = dm0p_uncache_fix(&t->tcb_table_io,
						  DM0P_UNFIX_CR,
						  r->rcb_lk_id,
						  r->rcb_log_id,
						  &r->rcb_tran_id,
						  pinfo,
						  dberr);
			if ( status )
			    break;
		    }
		    /* "page" is now the Root page, if it wasn't before */

		    cl_status = LK_NEW_LOCK;
		    continue;
		}
	    }
	    else
	        status = E_DB_ERROR;
	}
	break;
    }

    return(status);
}


/*{
** Name: dm1r_crowlk_access	      - locks a qualifying row in an
**			      		update cursor or constraint.
**
** Description:
**
**	For update cursors and constraints, we X lock all qualifying rows.
**
** Inputs:
**      rcb             
**	access		   ROW_ACC_RETURN
**      tid_to_lock        Tid to lock (if reading secondary index, may be
**                         different from tid)
**      save_lock_key      Previous locked tid. Unlock if different from
**                         tid_to_lock.
** 
**
** Outputs:
**	E_DB_OK if row locked.
**	E_DB_WARN if row is locked, but buffer lock was released
**		  and reacquired; row must be requalified.
**	E_DB_ERROR if row not locked.
**
**
** Exceptions:
**
** Side effects:
**	Crow locking may be escalated to Table if locks per transaction
**	is exceeded.
**
** History:
**	11-Nov-2009 (jonj)
**	    SIR 121619 MVCC: Created for update cursors, constraints
*/
static DB_STATUS
dm1r_crowlk_access(
DMP_RCB        *r,
i4	       access,
DM_TID         *tid,
DM_TID         *tid_to_lock,
LK_LOCK_KEY    *save_lock_key,
i4	       opflag,
DB_ERROR       *dberr)
{
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    CL_ERR_DESC	    sys_err;
    DB_STATUS       status = E_DB_OK;
    DB_TAB_ID	    row_reltid;

    CLRDBERR(dberr);

    /*
    ** No row locks needed when updating secondary indexes
    ** We should already have the base table page/row lock.
    ** And we don't want to get uneccessary locks looking for the 
    ** index entry to update (HASH/ISAM).
    */
    if ( opflag & DM1C_GET_SI_UPD )
	return (E_DB_OK);

    /* Determine TID's reltid */
    if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX &&
	 tid->tid_i4 != tid_to_lock->tid_i4 )
    {
	/* reltid is that of the tid's partition */
	row_reltid = t->tcb_parent_tcb_ptr->tcb_pp_array[r->rcb_partno].pp_tabid;
    }
    else
	row_reltid = t->tcb_parent_tcb_ptr->tcb_rel.reltid;

    /*
    ** If requalifying the same TID after releasing buffer lock,
    ** and we have the lock, then all is well.
    */
    if ( save_lock_key->lk_type == LK_ROW )
    {
	if ( save_lock_key->lk_key1 == d->dcb_id &&
	     save_lock_key->lk_key2 == row_reltid.db_tab_base &&
	     save_lock_key->lk_key3 == row_reltid.db_tab_index &&
	     save_lock_key->lk_key4 == tid_to_lock->tid_tid.tid_page &&
	     save_lock_key->lk_key5 == tid_to_lock->tid_tid.tid_line )
	{
	    return(E_DB_OK);
	}
	/* Release the logical lock of previously qualified row */
	(void) LKrelease(LK_RELLOG, r->rcb_lk_id, (LK_LKID*)NULL,
			save_lock_key, (LK_VALUE*)NULL,
			&sys_err);
    }

    /* Remember the lock we're attempting */
    save_lock_key->lk_type = LK_ROW;
    save_lock_key->lk_key1 = t->tcb_dcb_ptr->dcb_id;
    save_lock_key->lk_key2 = row_reltid.db_tab_base;
    save_lock_key->lk_key3 = row_reltid.db_tab_index;
    save_lock_key->lk_key4 = tid_to_lock->tid_tid.tid_page;
    save_lock_key->lk_key5 = tid_to_lock->tid_tid.tid_line;
    save_lock_key->lk_key6 = 0;

    /*
    ** Get a logical X row lock on the tid.
    **
    ** We don't care about update conflict resolution, so
    ** pass a NULL page pointer.
    **
    ** Note that the table may be escalated if we've taken
    ** too many logical locks.
    */
    status = dm1r_crow_lock(r, 
			    (dm0pBufIsLocked(&r->rcb_data)) ? LK_NOWAIT : 0,
			    tid_to_lock,
			    (DMP_PINFO*)NULL,
			    dberr);

    /* Don't assume all non-zero lock_status means "LK_BUSY" */
    if ( status && dberr->err_code == E_DM004C_LOCK_RESOURCE_BUSY )
    {
	/*
	** We'd have to wait, so buffer must be locked.
	**
	** Unlock the buffer, then get the lock, this time waiting.
	*/
	dm0pUnlockBuf(r, &r->rcb_data);

	status = dm1r_crow_lock(r, 0,
				tid_to_lock,
				(DMP_PINFO*)NULL,
				dberr);

	if ( status == E_DB_OK )
	{
	    /* Must return with the buffer locked */
	    dm0pLockBufForGet(r, &r->rcb_data, &status, dberr); 
	    if ( status == E_DB_OK )
	    {
		/*
		** The row is locked, reposition to re-get same row
		*/
		if (r->rcb_lowtid.tid_tid.tid_line == 0)
		    r->rcb_lowtid.tid_tid.tid_line = DM_TIDEOF;
		else
		    r->rcb_lowtid.tid_tid.tid_line--;

		status = E_DB_WARN;
	    }
	}
    }

    return(status);
}


/* 
** Name: dm1r_lock_row	    - acquire a row-level lock for user table.
**
** Description:
**
**      This routine requests a row-level lock for the user table.
**      User table can be isolated with row level granularity if such
**      granularity level is set up with SET LOCKMODE statement. 
**	Serializable transaction can acquire row level locks for Btree
**	and Hash table only. Other data structures exploit  page locking 
**	for phantom protection. Transaction running with the lower  
** 	isolation level may use row level locking for any user table. 
**
**	System catalogs employ special form of row-level locking which is 
**	implemented by dm1r_lock_sc_row().
**
** Inputs:
**	rcb		    - The RCB control block
**      flags               - Lock flags:
**                              DM1R_LK_NOWAIT
**                              DM1R_LK_PHYSICAL
**	tid		    - the row we're locking.
**	lock_key	    - key of previously locked row
**
** Outputs:
**	err_code	    - reason for error return
**      new_lock            - Indicates a new lock resource was needed
**	lock_key	    - key of new lock
**
** Returns:
**	E_DB_OK	                    Lock is granted.
**	E_DB_ERROR	            Lock is not granted.
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created. 
**      12-dec-96 (dilma04)
**          Remove setting of DM0P_PHYSICAL flag. Page lock will be 
**          requested in physical mode anyway.
**      20-jan-96 (stial01)
**          dm1r_lock_row: Log everything but a new lock status
**      26-feb-97 (dilma04)
**          Lock Escalation for Row Locking and Lower Isolation Levels:
**          - request a physical row lock if DM1R_LK_PHYSICAL flag is set;
**          - escalate to table locking it this lock request exceeded
**          'locks per transaction' limit;
**          - add auditing and change code structure to make it clearer.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - if isolation level is CS and the previous row has not been 
**          updated, unlock it before the next lock request;
**          - return error if flag is DM1R_LK_CHECK or DM1R_LK_CONVERT, 
**          but we are not locking the same row for the second time;
**          - added lock_id argument for dm0p_trans_page_lock() and
**          LKrequest() calls;
**          - return E_DB_OK if LKrequest() returns LK_LOG_LOCK status;
**          - if DM1R_LK_CONVERT flag is set pass DM0P_LK_CONVERT to
**          dm0p_trans_page_lock().
**      29-sep-97 (stial01)
**          Get data page lock if btree and data page is not already fixed.
**      12-nov-97 (stial01)
**          B87015: Output a lock trace message if tracing is enabled
**          Removed 29-sep-97 change to dm1r_lock_row. (Caused problems with 
**          isolation level read committed) 
**      22-sep-98 (stial01)
**          dm1r_lock_row() if LK_NOWAIT then escalate DM0P_NOWAIT (B92909)
**      22-nov-98 (stial01)
**          Check for DM1R_LK_PKEY_PROJ
**      18-feb-99 (stial01)
**          dm1r_lock_row() Map all errors to E_DB_ERROR status
**	01-apr-99 (nanpr01)
**	    Added the flag DM1R_CLEAR_LOCKED_TID for not setting the locked 
**	    tid and DM1R_ALLOC_TID for not doing anything with rcb_locked_tid.
**      28-jan-2000 (stial01)
**          dm1r_lock_row DO NOT UNLOCK previous row lock unless current
**          row lock request succeeds.  (Necessary for Cursor stability 
**          protocols)
**	25-Apr-2001 (thaju02)
**	    Modified parameters passed to ule_format for error E_DM9043.
**	    (B104562/INGSRV1438)
**	20-Aug-2002 (jenjo02)
**	    Support TIMEOUT=NOWAIT, new LK_UBUSY status from LKrequest().
**	    Write E_DM9066_LOCK_BUSY info message if DMZ_SES_MACRO(35).
**	    Return E_DM006B_NOWAIT_LOCK_BUSY.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**	26-Dec-2003 (jenjo02)
**	    If Global index, row locks must be based
**	    on Partition's reltid, caller must have set
**	    rcb_partno to index entries partition number.
**	    Added *lock_key parm. On input, it's the key of the
**	    last row locked; on output, it's the new key.
**	28-Jul-2004 (schka24)
**	    Fix typo in partition compare added above.
**	    Show proper partition name in locktrace for global index case.
**	    Clean up var decl indentation.
**      29-Sep-2004 (fanch01)
**          Add LK_LOCAL flag since row locking is always confined to one node.
**	17-Feb-2005 (schka24)
**	    More oopsen in the same area - global index points to master
**	    via parent ptr, not pmast ptr.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
*/
DB_STATUS
dm1r_lock_row(
DMP_RCB		*rcb, 
i4         	flags,
DM_TID 		*tid, 
i4         	*new_lock,
LK_LOCK_KEY	*lock_key,
DB_ERROR	*dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_DCB	*d = t->tcb_dcb_ptr;
    i4		lock_list;
    LK_LKID	lock_id;
    i4		row_lock_mode;
    i4		page_lock_mode;
    bool	row_locked = FALSE;
    i4		access_mode;
    i4		action = 0;
    i4		page_lock_action = 0;
    STATUS	cl_status;
    CL_ERR_DESC	sys_err;
    i4		lk_flags = LK_STATUS | LK_LOCAL;
    i4		n;
    DB_STATUS	status = E_DB_OK;
    i4		msgid = 0;
    LK_LKID	save_old_lkid;
    DM_TID	save_old_tid;
    DB_TAB_ID	save_old_reltid;
    LK_LKID	save_new_lkid;
    bool	cs_locked_tid = FALSE;
    DB_TAB_ID	row_reltid;
    DB_TAB_NAME *relid;
    DB_ERROR	local_dberr;
    i4		local_err;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    *new_lock = FALSE;

    /*
    ** We do not need to lock the row, if the user has set NOREADLOCK or the 
    ** row is already protected by the page lock (in S or X mode). Otherwise,  
    ** a row-level lock should be acquired.
    */         
    if (row_locking(r) == 0) 
        return(E_DB_OK);

    lock_id.lk_common = 0;
    lock_id.lk_unique = 0;

    /* Determine TID's reltid */
    relid = &t->tcb_parent_tcb_ptr->tcb_rel.relid;	/* for locktrace */
    if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
    {
	DMT_PHYS_PART *pp;

	/*
	** Then we have to find the reltid of the
	** TID's partition.
	**
	** Caller of dm1r_lock_row() must have placed
	** the partition number of the TID's row
	** into "rcb_partno".
	**
	** We use that to index into the Master
	** TCB's partition array to extract the
	** reltid of the partition.
	*/

	/* Tell dm0p_trans_page_lock() to use rcb_reltid */
	action |= DM0P_RCB_RELTID;

	pp = &t->tcb_parent_tcb_ptr->tcb_pp_array[r->rcb_partno];
	r->rcb_reltid = &pp->pp_tabid;
	STRUCT_ASSIGN_MACRO(*r->rcb_reltid, row_reltid);

	/* Partition TCB ought to be open, but be careful;  this bit is
	** just for lock-tracing and messages.
	*/
	if (pp->pp_tcb != NULL)
	    relid = &pp->pp_tcb->tcb_rel.relid;

    }
    else
    {
	/* Use base table / partition reltid */
	STRUCT_ASSIGN_MACRO(
	    t->tcb_parent_tcb_ptr->tcb_rel.reltid, row_reltid);
    }

    if ((flags & DM1R_ALLOC_TID) == 0)
    {
	/* Convert has to be in the exact same place */
	if ( flags & DM1R_LK_CONVERT &&
		(tid->tid_i4 != r->rcb_locked_tid.tid_i4 ||
		 row_reltid.db_tab_base != 
		    r->rcb_locked_tid_reltid.db_tab_base ||
		 row_reltid.db_tab_index != 
		    r->rcb_locked_tid_reltid.db_tab_index) )
	{
	    if ((i4) r->rcb_locked_tid.tid_i4 == -1)
	    {
		/* Make sure we already have the lock */
		status = dm1r_check_lock(r, tid);
		if (status == E_DB_OK)
		    return(E_DB_OK);
	    }
	    TRdisplay(
	    "LK_CONVERT error Id: %x Tran_id %x,%x %~t (%d,%d)(%d,%d)(%d,%d)\n",
		r->rcb_lk_id,
		r->rcb_tran_id.db_high_tran, r->rcb_tran_id.db_low_tran,
		sizeof(DB_TAB_NAME),  relid,
		row_reltid.db_tab_base, row_reltid.db_tab_index,
		tid->tid_tid.tid_page, tid->tid_tid.tid_line,
		r->rcb_locked_tid.tid_tid.tid_page, 
		r->rcb_locked_tid.tid_tid.tid_line);
	    SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
	    return (E_DB_ERROR);
	}

	/* 
	** If isolation level is serializable or repeatable read and the 
	** previous row has not been updated, and we are holding a update mode
	** lock, then downgrade it to shared lock.
	** If the record was modified, we already hold the appropriate lock in
	** correct lock mode.
	*/
	if (((r->rcb_iso_level == RCB_SERIALIZABLE) || 
	     (r->rcb_iso_level == RCB_REPEATABLE_READ)) &&
	     (tid->tid_i4 != r->rcb_locked_tid.tid_i4 ||
	      row_reltid.db_tab_base != r->rcb_locked_tid_reltid.db_tab_base ||
	      row_reltid.db_tab_index != r->rcb_locked_tid_reltid.db_tab_index
	     ) &&
	    ((i4) r->rcb_locked_tid.tid_i4 != -1) &&
	    ((r->rcb_state & RCB_UPD_LOCK)) &&
	    ((r->rcb_state & RCB_ROW_UPDATED) == 0) &&
	    (r->rcb_lk_mode == LK_IX))
	{
	    status = dm1r_lock_downgrade(r, dberr);
	    if (status != E_DB_OK)
		return(E_DB_ERROR);
	}

	/* 
	** If isolation level is CS and the previous row has not been updated, 
	** save the tid and the lkid so that we can unlock the row 
	** AFTER we lock the next row
	*/
	if ((r->rcb_state & RCB_CSRR_LOCK) && (r->rcb_csrr_flags & RCB_CS_ROW)
		&& (tid->tid_i4 != r->rcb_locked_tid.tid_i4 ||
		    row_reltid.db_tab_base != 
			r->rcb_locked_tid_reltid.db_tab_base ||
		    row_reltid.db_tab_index != 
			r->rcb_locked_tid_reltid.db_tab_index ))
	{
	    save_old_tid.tid_i4 = rcb->rcb_locked_tid.tid_i4;
	    STRUCT_ASSIGN_MACRO(rcb->rcb_locked_tid_reltid, save_old_reltid);
	    STRUCT_ASSIGN_MACRO(rcb->rcb_row_lkid, save_old_lkid);
	    cs_locked_tid = TRUE;
	}

	if ((flags & DM1R_LK_PHYSICAL) 
		&& (r->rcb_iso_level == RCB_SERIALIZABLE))
	{
	    flags &= ~DM1R_LK_PHYSICAL;
	    TRdisplay("PHYSICAL row lock requested when serializable\n");
	}
    }
 
    if (flags & DM1R_LK_PHYSICAL)
    {
	lk_flags |= LK_PHYSICAL;
    }

    if (flags & DM1R_LK_NOWAIT)
    {
        action |= DM0P_NOWAIT;
        lk_flags |= LK_NOWAIT;
    }

    if (flags & DM1R_LK_CONVERT)
        action |= DM0P_LK_CONVERT; 

    if ((r->rcb_k_mode == RCB_K_IX) || (r->rcb_k_mode == RCB_K_X))
    {
	if (r->rcb_state & RCB_UPD_LOCK)
	    row_lock_mode = LK_U;
	else
            row_lock_mode = LK_X;
        page_lock_mode = LK_IX;
    }
    else
    {
        row_lock_mode = LK_S;
        page_lock_mode = LK_IS;
    }

    /*
    ** There is the possibility that we have not requested an intended 
    ** lock on the base table page. We have to get it here before 
    ** requesting the row lock.
    ** This can happen if locking a row in a base table while scanning an index.
    **
    ** This can also happen if (flags & DM1R_LK_PKEY_PROJ)
    ** in which case we are projecting primary keys from the leaf pages
    ** and will not lock/read data pages
    **
    ** But don't re-request the lock if we've already done this. The pin
    ** count will get incremented and the lock won't be released when
    ** we think it should.
    */
    if ( (t->tcb_rel.relstat & TCB_INDEX || flags & DM1R_LK_PKEY_PROJ) &&
        !(r->rcb_csrr_flags & RCB_CS_BASEDATA) )
    {
        status = dm0p_trans_page_lock(r, 
                     t->tcb_parent_tcb_ptr, 
                     tid->tid_tid.tid_page,
		     action,
		     page_lock_mode, 
                     &page_lock_action, &lock_id, dberr);
        if (status != E_DB_OK)
	    return(E_DB_ERROR);

        /*
	** Check if the above page lock resulted in lock escalation
	*/
	if (row_locking(r) == 0)
	    return (E_DB_OK);

	STRUCT_ASSIGN_MACRO(lock_id, r->rcb_base_lkid);
	r->rcb_csrr_flags |= RCB_CS_BASEDATA;
    }

    lock_list = r->rcb_lk_id;
 
    lock_key->lk_type = LK_ROW;
    lock_key->lk_key1 = (i4)t->tcb_dcb_ptr->dcb_id;
    lock_key->lk_key2 = row_reltid.db_tab_base;
    lock_key->lk_key3 = row_reltid.db_tab_index;
    lock_key->lk_key4 = tid->tid_tid.tid_page;
    lock_key->lk_key5 = tid->tid_tid.tid_line;
    lock_key->lk_key6 = 0;

    /* Output a lock trace message if tracing is enabled. */
    if (DMZ_SES_MACRO(1))
	dmd_lock(lock_key, lock_list, LK_REQUEST, lk_flags, row_lock_mode, 
	    r->rcb_timeout, &r->rcb_tran_id, relid, &d->dcb_name);

    /*
    ** Return lock_id into lock_id, not rcb_row_lkid.
    **
    ** If LK_NOWAIT and the lock can't be immediately granted,
    ** "lock_id" is zeroed, not what we want to happen to rcb_row_lkid.
    */
    /* Seed lock_id with whatever's in rcb_row_lkid */
    lock_id = r->rcb_row_lkid;

    cl_status = LKrequest(lk_flags, lock_list, lock_key, row_lock_mode,
        (LK_VALUE *)NULL, &lock_id, r->rcb_timeout, &sys_err);

    if (cl_status == OK || cl_status == LK_LOG_LOCK || cl_status == LK_NEW_LOCK)
    {
	/* Ok, we got the lock, now set rcb_row_lkid */
	r->rcb_row_lkid = lock_id;

	/* If Cursor stability, we may have a lock to release */
	if (cs_locked_tid)
	{

	    STRUCT_ASSIGN_MACRO(rcb->rcb_row_lkid, save_new_lkid);
	    STRUCT_ASSIGN_MACRO(save_old_lkid, r->rcb_row_lkid);
	    STRUCT_ASSIGN_MACRO(save_old_reltid, r->rcb_locked_tid_reltid);
	    rcb->rcb_locked_tid.tid_i4 = save_old_tid.tid_i4;
	    
	    status = dm1r_unlock_row(r, (LK_LOCK_KEY *)NULL, dberr);

	    /* Restore the rcb */
	    STRUCT_ASSIGN_MACRO(save_new_lkid, rcb->rcb_row_lkid);
	    STRUCT_ASSIGN_MACRO(row_reltid, r->rcb_locked_tid_reltid);
	    rcb->rcb_locked_tid.tid_i4 = tid->tid_i4;
	    r->rcb_csrr_flags |= RCB_CS_ROW;

	    if (status != E_DB_OK)
		return (E_DB_ERROR);
	}

	if (flags & DM1R_CLEAR_LOCKED_TID)
	    r->rcb_locked_tid.tid_i4 = -1;
	else if ((flags & DM1R_ALLOC_TID) == 0)
	{
	    r->rcb_locked_tid.tid_i4 = tid->tid_i4;
	    STRUCT_ASSIGN_MACRO(row_reltid, r->rcb_locked_tid_reltid);
	}
	r->rcb_state &= ~RCB_ROW_UPDATED;

	if (cl_status == LK_NEW_LOCK)
	    *new_lock = TRUE;

        return (E_DB_OK);
    }

    if (cl_status == LK_NOLOCKS)
    {
        uleFormat(NULL, E_DM004B_LOCK_QUOTA_EXCEEDED, &sys_err, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
        msgid = I_SX2739_LOCK_LIMIT;
    }
    else if (cl_status == LK_DEADLOCK)
    {
        uleFormat(NULL, E_DM905A_ROW_DEADLOCK, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
	    0, tid->tid_tid.tid_page, 0, tid->tid_tid.tid_line,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 
	    0, row_lock_mode);

	/*
	** DM304 enables tracing of lockstat on deadlock.
	** This occurs always in the RCP, as it should never encounter 
	** deadlocks.
	*/
	if ((DMZ_LCK_MACRO(4)) || 
		(t->tcb_dcb_ptr->dcb_status & DCB_S_RECOVER))
	{
	    dmd_lkrqst_trace(dmd_put_line, lock_key, lock_list, 
		LK_REQUEST, lk_flags, row_lock_mode, 
		r->rcb_timeout, &r->rcb_tran_id,
		relid, &t->tcb_dcb_ptr->dcb_name);
	    dmd_lock_info(DMTR_LOCK_LISTS | DMTR_LOCK_USER_LISTS |
		    DMTR_LOCK_SPECIAL_LISTS | DMTR_LOCK_RESOURCES );
	}

	SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
        msgid = I_SX2753_ROW_DEADLOCK;
    }
    else if (cl_status == LK_TIMEOUT)
    {
	char            msgbuf[64];

	STprintf(msgbuf, "ROW %d (of page %d)", lock_key->lk_key5,
	    lock_key->lk_key4);
	uleFormat(NULL, E_DM9043_LOCK_TIMEOUT, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name,
	    0, row_lock_mode, sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
	    0, msgbuf, 0, sys_err.moreinfo[0].data.string);
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
         msgid = I_SX2738_LOCK_TIMEOUT;
    }
    else if (cl_status == LK_RETRY)
    {
	uleFormat(NULL, E_DM9047_LOCK_RETRY, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 
	    0, row_lock_mode);
	SETDBERR(dberr, 0, E_DM004F_LOCK_RETRY);
    }
    else if (cl_status == LK_INTERRUPT)
    {
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    }
    else if (cl_status == LK_INTR_GRANT)
    {
	row_locked = TRUE;
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    }
    else if (cl_status == LK_INTR_FA)
    {
        SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
    }
    else if ( cl_status == LK_BUSY )
    {
	SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
    }
    else if ( cl_status == LK_UBUSY )
    {
	/* Output message iff DMZ_SES_MACRO(35) set in SCB */
	if ( r->rcb_xcb_ptr && DMZ_MACRO(r->rcb_xcb_ptr->xcb_scb_ptr->scb_trace, 35) )
	{
	    char            msgbuf[64];

	    STprintf(msgbuf, "ROW %d (of page %d)", lock_key->lk_key5,
		lock_key->lk_key4);
	    uleFormat(NULL, E_DM9066_LOCK_BUSY, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name,
		0, row_lock_mode, sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, msgbuf, 0, sys_err.moreinfo[0].data.string);
	}
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
    }
    else
    {
	TRdisplay("Row lock failed for table [%d,%d] Tid %d,%d status %d,%d\n",
		lock_key->lk_key2, lock_key->lk_key3, lock_key->lk_key4,
		lock_key->lk_key5, cl_status, sys_err);
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM905B_BAD_LOCK_ROW, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
	    0, tid->tid_tid.tid_page, 0, tid->tid_tid.tid_line,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 0,
	    row_lock_mode, 0, lock_list);
	SETDBERR(dberr, 0, E_DM9078_ROW_LOCK_ERROR);
    }
 
    /*
    ** If the lock request was interrupted after being granted, and
    ** it is a temporary physical lock, we need to release it before
    ** returning with an error.
    **
    ** Note: the return status from the unlock call is ignored since
    ** the error is logged inside unlock_row anyway.
    */
    if ((row_locked) && (lk_flags & LK_PHYSICAL))
    {
        _VOID_ dm1r_unlock_row(r, lock_key, &local_dberr);
    }

    /* Show lock not held */
    lock_key->lk_type = 0;

    /*
    ** Write an audit record if needed 
    */
    if ( msgid != 0 && dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
        (void)dma_write_audit(
                 SXF_E_RESOURCE,
                 SXF_A_FAIL | SXF_A_LIMIT,
                 (char*)relid,			/* Table name */
                 sizeof(DB_TAB_NAME),		/* Table name */
                 &t->tcb_rel.relowner,
                 msgid,
                 FALSE, /* Not force */
                 &local_dberr, NULL);
    }

    if (dberr->err_code != E_DM004B_LOCK_QUOTA_EXCEEDED)
         return (E_DB_ERROR);

    /*
    ** if allocate nowait, we have buffers locked
    ** return and let caller unlock the buffers and do the escalate
    */
    if ( (flags & DM1R_ALLOC_TID) && (flags & DM1R_LK_NOWAIT))
	return (E_DB_ERROR);

    /*
    ** Escalate to table level locking because 
    ** 'locks per transaction' limit exceeded.
    */
    uleFormat(NULL, E_DM9041_LK_ESCALATE_TABLE, (CL_ERR_DESC *)NULL, ULE_LOG, 
               NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 
               2, sizeof(DB_TAB_NAME), relid, 
               sizeof(DB_DB_NAME), &d->dcb_name);

    /*
    ** Do not WAIT to escalate if NOWAIT specified
    ** We cannot WAIT if we have buffers locked (B92909)
    */
    if (flags & DM1R_LK_NOWAIT)
	action = DM0P_NOWAIT;
    else
	action = 0;

    status = dm0p_escalate(r, action, dberr);

#ifdef xDEBUG
    if (status != E_DB_OK)
	TRdisplay("dm1r_lock_row escalate error %d\n", dberr->err_code);
#endif

    /* Escalate failure may return E_DB_FATAL, map it to E_DB_ERROR */
    if (status == E_DB_OK)
	return (E_DB_OK);
    else
	return (E_DB_ERROR);
}


/*
** Name: dm1r_unlock_row      - release a row level lock for user table.
**
** Description:
**
**      This routine releases a physical row-level lock. It has to do with
**	user tables only. Locks acquired on system catalogs are dissolved
**      at transaction commit.
**
** Inputs:
**      rcb                 - The RCB control block
**      lock_key            - the lock key we're unlocking.
**
** Outputs:
**      err_code            - reason for error return
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - added support for releasing locks by lock id;
**          - if releasing row lock when scanning secondary index, release 
**          also intended lock from base table page. This fixes bug 80602. 
**      12-nov-97 (stial01)
**          B87015: Output a lock trace message if tracing is enabled
**          Pass lockKEY even if unlocking with lockID (for set lock_trace)
**	12-feb-1999 (nanpr01)
**	    When rows are unlocked, initialize the rcb_locked_tid.
**	01-apr-1999 (nanpr01)
**	    Optimized code.
**      02-jun-1999 (stial01)
**          dm1r_unlock_row() Fixed error handling after LKrelease
**      01-dec-2003 (rigka01) problem INGSRV2589, but 111245
**          Wrong level of redirection in erlookup call for displaying
**          either message E_DM9072 or E_DM9073
**	22-Jan-2004 (jenjo02)
**	    dm1r_unlock_row() now takes (LK_LOCK_KEY*) instead
**	    of (DM_TID*) as input parm.
**	20-Sep-2005 (jenjo02)
**	    If "*lock_key" describes the same lock as rcb_locked_tid,
**	    set rcb_locked_tid.tid_i4 = -1 to indicate that
**	    that lock has been released.
**	4-Jul-2008 (kibro01) b120597
**	    If unlocking an index (but the row lock is taken on the base
**	    table), adjust the table name to be that of the base table
**	    to match the original lock.
*/
DB_STATUS
dm1r_unlock_row(
DMP_RCB         *rcb,
LK_LOCK_KEY     *lock_key,
DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    LK_LOCK_KEY     *lkey;
    LK_LOCK_KEY	    local_lock_key;
    LK_LKID         *lock_id_p;
    STATUS          cl_status;
    CL_ERR_DESC     sys_err;
    DB_STATUS       status;
    i4              local_error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    if (row_locking(rcb) == 0)
        return(E_DB_OK);

    if ( !(lkey = lock_key) )
    {
	lkey = &local_lock_key;

	lkey->lk_type = LK_ROW;
	lkey->lk_key1 = (i4)t->tcb_dcb_ptr->dcb_id;
	lkey->lk_key2 = r->rcb_locked_tid_reltid.db_tab_base;
	lkey->lk_key3 = r->rcb_locked_tid_reltid.db_tab_index;
	lkey->lk_key4 = r->rcb_locked_tid.tid_tid.tid_page;
	lkey->lk_key5 = r->rcb_locked_tid.tid_tid.tid_line;
	lkey->lk_key6 = 0;

	lock_id_p = &rcb->rcb_row_lkid;
    }
    else 
	lock_id_p = (LK_LKID*)NULL;
    
    /* Output a lock trace message if tracing is enabled. */
    /* FIXME the relid here is potentially wrong in the global index
    ** case, but there's no simple quick-fix.  Perhaps compare reltids
    ** and use "(n,n)" instead of table name if different.
    */
    if (DMZ_SES_MACRO(1))
    {
	DMP_TCB *unlock_tcb = t;
	if (t->tcb_rel.relstat & TCB_INDEX)
	{
	    unlock_tcb = t->tcb_parent_tcb_ptr;
	}

        dmd_lock(lkey, rcb->rcb_lk_id, LK_RELEASE, LK_SINGLE,
            0, 0, &r->rcb_tran_id, &unlock_tcb->tcb_rel.relid,
	    &t->tcb_dcb_ptr->dcb_name);
    }

    cl_status = LKrelease((i4)0, r->rcb_lk_id, lock_id_p, 
			    lkey, (LK_VALUE *)NULL, &sys_err);

    if ( status = cl_status )
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	if ( !lock_key )
	    SETDBERR(dberr, 0, E_DM9073_BAD_UNLOCK_ROW_BY_LKID);
	else
	    SETDBERR(dberr, 0, E_DM9072_BAD_UNLOCK_ROW);
	uleFormat(dberr, 0, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5, 
	    0, lkey->lk_key5, 0, lkey->lk_key4,
	    sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 0, r->rcb_lk_id);
	SETDBERR(dberr, 0, E_DM9076_ERROR_RELEASING_LOCK);
        status = E_DB_ERROR;
    }
    else if (r->rcb_state & RCB_CSRR_LOCK) 
    {
	r->rcb_csrr_flags &= ~RCB_CS_ROW;

        /* 
        ** Bug 80602. If scanning secondary index, release intended
        ** lock on base table page.
	**
	** Note that we unlock by rcb_base_lkid, so the rest
	** of the parms are superfluous and may be inaccurate.
        */
	if (r->rcb_csrr_flags & RCB_CS_BASEDATA)
        {
	    status = dm0p_unlock_page(r->rcb_lk_id, t->tcb_dcb_ptr,
		&t->tcb_rel.reltid, 
		lkey->lk_key4, LK_PAGE,
		&t->tcb_parent_tcb_ptr->tcb_rel.relid, &r->rcb_tran_id,
		&r->rcb_base_lkid, (LK_VALUE *)NULL, dberr);

	    r->rcb_csrr_flags &= ~RCB_CS_BASEDATA;
        }
    }
    if ( !lock_key || 
	(lock_key->lk_key2 == r->rcb_locked_tid_reltid.db_tab_base &&
	 lock_key->lk_key3 == r->rcb_locked_tid_reltid.db_tab_index &&
	 lock_key->lk_key4 == r->rcb_locked_tid.tid_tid.tid_page &&
	 lock_key->lk_key5 == r->rcb_locked_tid.tid_tid.tid_line) )
    {
	r->rcb_locked_tid.tid_i4 = -1;
    }
    lkey->lk_type = 0;

    return (status);
}


/*
** Name: dm1r_lock_value      - acquire a key value lock.
**
** Description:
**
** Inputs:
**      rcb                 - The RCB control block
**      flags               - Lock flags:
**                              DM1R_LK_NOWAIT
**      tuple               - Pointer to the tuple
**
** Outputs:
**      err_code            - reason for error return
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      20-jan-96 (stial01)
**          dm1r_lock_value: Log everything but a new lock status
**      10-mar-97 (stial01)
**          dm1r_lock_value() Pass rcb_tupbuf to dm1h_keyhash
**      29-sep-97 (stial01)
**          dm1r_lock_value() Escalate to table locking it this lock request
**          exceeded 'locks per transaction' limit;
**      12-nov-97 (stial01)
**          B87015: Output a lock trace message if tracing is enabled
**       7-jan-99 (stial01)
**          dm1r_lock_value() Removed buffer parameter for dm1h_keyhash
**	29-mar-1999 (nanpr01)
**	    Reshuffled some code.
**	20-Aug-2002 (jenjo02)
**	    Support TIMEOUT=NOWAIT, new LK_UBUSY status from LKrequest().
**	    Write E_DM9066_LOCK_BUSY info message if DMZ_SES_MACRO(35).
**	    Return E_DM006B_NOWAIT_LOCK_BUSY.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**	28-Jul-2004 (schka24)
**	    Take value locks against partitioned master (they are conceptually
**	    table-wide locks).  Watch out for tid8 in key length fixup.
**      29-Sep-2004 (fanch01)
**          Add LK_LOCAL flag since row locking is always confined to one node.
**      21-may-2009 (huazh01)
**          dm1h_keyhash() now includes DMP_RCB and err_code in the parameter
**          list. 
**          (b122075)
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
*/
DB_STATUS
dm1r_lock_value(
DMP_RCB         *rcb,
i4         flags,
char            *tuple,
DB_ERROR        *dberr)
{
    DMP_TCB     *t = rcb->rcb_tcb_ptr;
    LK_LOCK_KEY lock_key;
    i4		lock_list;
    i4		lock_mode;
    STATUS      cl_status;
    CL_ERR_DESC sys_err;
    i4		lk_flags = LK_LOCAL;
    DB_STATUS	status;
    DB_ATTS	**atts_array;
    i4		atts_count;
    i4		msgid = 0;
    DB_TAB_NAME	*relid;
    bool	value_locked = FALSE;
    i4		klen;
    char	*lock_value = (char *)NULL;
    i4		i;
    i4		local_err;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Note no value lock acquired, yet */
    rcb->rcb_val_lkid.lk_common = 0;
    rcb->rcb_val_lkid.lk_unique = 0;
 
    if ( !row_locking(rcb) && !crow_locking(rcb) )
	return (E_DB_OK);

    if (flags & DM1R_PHANPRO_VALUE)
	lock_mode = LK_S;    /* Shared lock for phantom protection */
    else if ((rcb->rcb_k_mode == RCB_K_IX) || (rcb->rcb_k_mode == RCB_K_X))
        lock_mode = LK_X;    /* Exclusive lock for update */
    else
        lock_mode = LK_S;    /* Shared lock for read */
 
    /* MVCC does not need LK_S phantom protection nor S locks */
    if ( crow_locking(rcb) && lock_mode != LK_X )
        return(E_DB_OK);

    if (flags & DM1R_LK_PHYSICAL)
        lk_flags |= LK_PHYSICAL;

    if (flags & DM1R_LK_NOWAIT)
	lk_flags |= LK_NOWAIT;

    atts_array = t->tcb_key_atts;
    atts_count = t->tcb_keys;

    klen = t->tcb_klen;
    /* Don't include tidp when hashing key */
    if ( t->tcb_rel.relstat & TCB_INDEX &&
	(t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0))
    {
	atts_count--;
	/* Could be a tid4 or tid8, use column length */
	klen -= t->tcb_atts_ptr[t->tcb_rel.relatts].length;
    }

    lock_list = rcb->rcb_lk_id;
    lock_key.lk_type = LK_VAL_LOCK;
    lock_key.lk_key1 = (i4)t->tcb_dcb_ptr->dcb_id;
    lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
    lock_key.lk_key3 = t->tcb_rel.reltid.db_tab_index;
    relid = &t->tcb_rel.relid;
    /* Value lock against master if partitioned */
    if ( t->tcb_rel.relstat2 & TCB2_PARTITION )
    {
	lock_key.lk_key3 = 0;
	relid = &t->tcb_pmast_tcb->tcb_rel.relid;
    }

    if (klen <= (sizeof(lock_key.lk_key4) + sizeof(lock_key.lk_key5) +
		sizeof(lock_key.lk_key6)))
    {
	lock_key.lk_key4 = 0;
	lock_key.lk_key5 = 0;
	lock_key.lk_key6 = 0;
	lock_value = (char *)&lock_key.lk_key4;

	for (i = 0; lock_value && i < atts_count; i++)
	{
	    /* Specifically avoid nullables, they have to go thru hashprep */
	    switch (atts_array[i]->type)
	    {
		case ATT_INT:
		case ATT_FLT:
		case ATT_MNY:
		case ATT_CHA:
		    MEcopy(tuple + atts_array[i]->offset, 
			    atts_array[i]->length, lock_value);
		    lock_value += atts_array[i]->length;
		    break;
		default:
		    lock_value = (char *)NULL;
		    break;
	    }
	}
    }

    /* Only hash key if it was greater than 12 bytes or nullable */
    if (!lock_value)
    {
	status = dm1h_keyhash(rcb, atts_array, atts_count, tuple,
		    &lock_key.lk_key4, &lock_key.lk_key5, &lock_key.lk_key6, 
                    dberr);

	if (status != E_DB_OK)
	    return (status);
    }

    /* Output a lock trace message if tracing is enabled. */
    if (DMZ_SES_MACRO(1))
	dmd_lock(&lock_key, lock_list, LK_REQUEST, lk_flags, lock_mode, 
	    rcb->rcb_timeout, &rcb->rcb_tran_id, relid, 
	    &t->tcb_dcb_ptr->dcb_name);

    cl_status = LKrequest(lk_flags, lock_list, &lock_key, 
      lock_mode, (LK_VALUE *)NULL, &rcb->rcb_val_lkid, 
      rcb->rcb_timeout, &sys_err);

    if (cl_status == OK || cl_status == LK_LOG_LOCK || cl_status == LK_NEW_LOCK)
	return (E_DB_OK);
 
    /* Log everything but a new lock status. */
    if (cl_status == LK_NOLOCKS)
    {
	uleFormat(dberr, E_DM004B_LOCK_QUOTA_EXCEEDED, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	msgid = I_SX2739_LOCK_LIMIT;
    }
    else if (cl_status == LK_DEADLOCK)
    {
	uleFormat(NULL, E_DM905C_VALUE_DEADLOCK, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
	    0, lock_key.lk_key4, 0, lock_key.lk_key5,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 
	    0, lock_mode);

	/*
	** DM304 enables tracing of lockstat on deadlock.
	** This occurs always in the RCP, as it should never encounter 
	** deadlocks.
	*/
	if ((DMZ_LCK_MACRO(4)) || 
		(t->tcb_dcb_ptr->dcb_status & DCB_S_RECOVER))
	{
	    dmd_lkrqst_trace(dmd_put_line, &lock_key, lock_list, 
		LK_REQUEST, lk_flags, lock_mode, rcb->rcb_timeout, 
		&rcb->rcb_tran_id, relid, 
		&t->tcb_dcb_ptr->dcb_name);
	    dmd_lock_info(DMTR_LOCK_LISTS | DMTR_LOCK_USER_LISTS |
		    DMTR_LOCK_SPECIAL_LISTS | DMTR_LOCK_RESOURCES );
	}

	SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
	msgid = I_SX2753_ROW_DEADLOCK;
    }
    else if (cl_status == LK_TIMEOUT)
    {
	char            msgbuf[64];

	STprintf(msgbuf, "Value %d %d %d", lock_key.lk_key4, lock_key.lk_key5,
		lock_key.lk_key6);
	uleFormat(NULL, E_DM9043_LOCK_TIMEOUT, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 
	    0, lock_mode, 0, "?", 0, msgbuf, 
	    0, sys_err.moreinfo[0].data.string);
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	msgid = I_SX2738_LOCK_TIMEOUT;
    }
    else if (cl_status == LK_RETRY)
    {
	uleFormat(NULL, E_DM9047_LOCK_RETRY, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 
	    0, lock_mode);
	SETDBERR(dberr, 0, E_DM004F_LOCK_RETRY);
    }
    else if (cl_status == LK_INTERRUPT)
    {
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    }
    else if (cl_status == LK_INTR_GRANT)
    {
	value_locked = TRUE;
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    }
    else if (cl_status == LK_INTR_FA)
    {
        SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
    }
    else if ( cl_status == LK_BUSY )
    {
	SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
    }
    else if ( cl_status == LK_UBUSY )
    {
	/* Output message iff DMZ_SES_MACRO(35) set in SCB */
	if ( rcb->rcb_xcb_ptr && DMZ_MACRO(rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_trace, 35) )
	{
	    char            msgbuf[64];

	    STprintf(msgbuf, "Value %d %d %d", lock_key.lk_key4, lock_key.lk_key5,
		    lock_key.lk_key6);
	    uleFormat(NULL, E_DM9066_LOCK_BUSY, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 
		0, lock_mode, 0, "?", 0, msgbuf, 
		0, sys_err.moreinfo[0].data.string);
	}
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
    }
    else
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM905D_BAD_LOCK_VALUE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
	    0, lock_key.lk_key4, 0, lock_key.lk_key5, 
	    sizeof(DB_TAB_NAME), relid,
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 0, lock_mode,
	    0, lock_list);
	SETDBERR(dberr, 0, E_DM901C_BAD_LOCK_REQUEST);
    }

    /*
    ** If the lock request was interrupted after being granted, and
    ** it is a temporary physical lock, we need to release it before
    ** returning with an error.
    **
    ** Note: the return status from the unlock call is ignored since
    ** the error is logged inside unlock_row anyway.
    */
    if ((value_locked) && (lk_flags & LK_PHYSICAL))
    {
	_VOID_ dm1r_unlock_value(rcb, tuple, &local_dberr);
    }

    /*
    ** Write an audit record if needed 
    */
    if ( msgid != 0 && dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	(void)dma_write_audit(
		 SXF_E_RESOURCE,
		 SXF_A_FAIL | SXF_A_LIMIT,
		 (char*) relid,			/* Table name */
		 sizeof(DB_TAB_NAME),		/* Table name */
		 &t->tcb_rel.relowner,
		 msgid,
		 FALSE, /* Not force */
		 &local_dberr, NULL);
    }

    if ( dberr->err_code != E_DM004B_LOCK_QUOTA_EXCEEDED )
	 return (E_DB_ERROR);

    /*
    ** Escalate to table level locking because 
    ** 'locks per transaction' limit exceeded.
    */
    uleFormat(NULL, E_DM9041_LK_ESCALATE_TABLE, (CL_ERR_DESC *)NULL, ULE_LOG, 
	       NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 
	       2, sizeof(DB_TAB_NAME), relid, 
	       sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);

    status = dm0p_escalate(rcb, (i4)0, dberr);

    return (status);
}


/*
** Name: dm1r_unlock_value      - release a key value lock.
**
** Description:
**
**      This routine releases a key-value lock. 
**
** Inputs:
**      rcb                 - The RCB control block
**      tuple               - Pointer to the tuple 
**
** Outputs:
**      err_code            - reason for error return
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      12-nov-97 (stial01)
**          B87015: Output a lock trace message if tracing is enabled
**       7-jan-99 (stial01)
**          dm1r_unlock_value() Removed buffer parameter for dm1h_keyhash
**      05-may-99 (nanpr01)
**          If rcb->rcb_lk_id is set, use it to release lock instead of  
**          rehashing the key.
**      02-jun-1999 (stial01)
**          dm1r_unlock_value() Fixed error handling after LKrelease
**	05-Apr-2004 (jenjo02)
**	    Fixed garbled ule_format tokens.
**	28-Jul-2004 (schka24)
**	    Same fixes as for lock: unlock against master, watch out for
**	    tid8 vs tid4.
**      21-may-2009 (huazh01)
**          dm1h_keyhash() now includes DMP_RCB in the parameter list. 
**          (b122075)
*/
DB_STATUS
dm1r_unlock_value(
DMP_RCB         *rcb,
char            *tuple,
DB_ERROR        *dberr)
{
    DMP_TCB         *tcb = rcb->rcb_tcb_ptr;
    LK_LOCK_KEY     lock_key;
    DB_ATTS         **atts_array;
    i4         	    atts_count;
    i4         lock_list;
    STATUS          cl_status;
    CL_ERR_DESC     sys_err;
    DB_STATUS       status;
    i4		klen;
    char	*lock_value = (char *)NULL;
    DB_TAB_NAME	*relid;
    i4		i;
    i4		local_error;
    bool	release_by_lkid = FALSE;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    if (!row_locking(rcb) && !crow_locking(rcb) )
    {
	rcb->rcb_val_lkid.lk_common = 0;
	rcb->rcb_val_lkid.lk_unique = 0;
	return (E_DB_OK);
    }

    /* MVCC does not need LK_S phantom protection nor S locks */
    if ( crow_locking(rcb) && rcb->rcb_k_mode != RCB_K_IX )
    {
	rcb->rcb_val_lkid.lk_common = 0;
	rcb->rcb_val_lkid.lk_unique = 0;
        return(E_DB_OK);
    }

    lock_list = rcb->rcb_lk_id;
    /* Output a lock trace message if tracing is enabled. */
    if ((DMZ_SES_MACRO(1)) || 
	(rcb->rcb_val_lkid.lk_common == 0 && rcb->rcb_val_lkid.lk_unique == 0))
    {
    	lock_key.lk_type = LK_VAL_LOCK;
    	lock_key.lk_key1 = (i4)tcb->tcb_dcb_ptr->dcb_id;
    	lock_key.lk_key2 = tcb->tcb_rel.reltid.db_tab_base;
    	lock_key.lk_key3 = tcb->tcb_rel.reltid.db_tab_index;
	relid = &tcb->tcb_rel.relid;
	/* Lock against master if this is a partition */
	if ( tcb->tcb_rel.relstat2 & TCB2_PARTITION )
	{
	    lock_key.lk_key3 = 0;
	    relid = &tcb->tcb_pmast_tcb->tcb_rel.relid;
	}

	atts_array = tcb->tcb_key_atts;
	atts_count = tcb->tcb_keys;

	klen = tcb->tcb_klen;
	/* Exclude tidp if this is an index */
	if ( tcb->tcb_rel.relstat & TCB_INDEX &&
	    (tcb->tcb_atts_ptr[tcb->tcb_rel.relatts].key != 0))
	{
	    atts_count--;
	    /* Could be tid4 or tid8, use att length */
	    klen -= tcb->tcb_atts_ptr[tcb->tcb_rel.relatts].length;
	}

	if (klen <= (sizeof(lock_key.lk_key4) + sizeof(lock_key.lk_key5) +
		    sizeof(lock_key.lk_key6)))
	{
	    lock_key.lk_key4 = 0;
	    lock_key.lk_key5 = 0;
	    lock_key.lk_key6 = 0;
	    lock_value = (char *)&lock_key.lk_key4;

	    for (i = 0; lock_value && i < atts_count; i++)
	    {
		switch (atts_array[i]->type)
		{
		    case ATT_INT:
		    case ATT_FLT:
		    case ATT_MNY:
		    case ATT_CHA:
			MEcopy(tuple + atts_array[i]->offset, 
				atts_array[i]->length, lock_value);
			lock_value += atts_array[i]->length;
			break;
		    default:
			lock_value = (char *)NULL;
			break;
		}
	    }
	}

	/* Only hash key if it was greater than 12 bytes or nullable */
	if (!lock_value)
	{
	    status = dm1h_keyhash(rcb, atts_array, atts_count, tuple,
		    &lock_key.lk_key4, &lock_key.lk_key5, &lock_key.lk_key6, 
                    dberr);
	}
	else
	{
	    status = E_DB_OK;
	}

    	if (status == E_DB_OK)
    	{
	    dmd_lock(&lock_key, lock_list, LK_RELEASE, LK_SINGLE,
		0, 0, &rcb->rcb_tran_id, relid, 
		&tcb->tcb_dcb_ptr->dcb_name);
	}
        else
        {
            return (E_DB_ERROR);
        }


	/* Unlock by key because of locktrace/tracpoint must clear lkid */
	rcb->rcb_val_lkid.lk_common = 0; 
	rcb->rcb_val_lkid.lk_unique = 0;

        cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL,
                (LK_LOCK_KEY *)&lock_key, (LK_VALUE *)NULL, &sys_err);
 
    }
    else
    {
	release_by_lkid = TRUE;
        cl_status = LKrelease((i4)0, lock_list, &rcb->rcb_val_lkid,
                (LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);
    }
	
    if (cl_status == OK)
	status = E_DB_OK;
    else
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	if (release_by_lkid)
	    uleFormat(NULL, E_DM9075_BAD_UNLOCK_VALUE_BY_LKID, &sys_err, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &tcb->tcb_dcb_ptr->dcb_name, 
		0, rcb->rcb_lk_id);
	else
	    uleFormat(NULL, E_DM9074_BAD_UNLOCK_VALUE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		0, lock_key.lk_key4, 0, lock_key.lk_key5, 0, lock_key.lk_key6,
		sizeof(DB_TAB_NAME), relid,
		sizeof(DB_DB_NAME), &tcb->tcb_dcb_ptr->dcb_name, 
		0, rcb->rcb_lk_id);
	SETDBERR(dberr, 0, E_DM9076_ERROR_RELEASING_LOCK);
        status = E_DB_ERROR;
    }
    return (status);
}



/*{
** Name: dm1r_count - count records in a table.
**
** Description:
**
**	This function counts how many records there are in a non-btree
**	table, for use by count(*) agg function, as part of DMR_COUNT
**	request processing.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	direction			Ignored, just for compatibility
**					with dm2r_get parms since dm1r_count
**					may be called interchangably via ptr.
**	record				Ignored, just for compatibility
**					with dm2r_get parms since dm1r_count
**					may be called interchangably via ptr.
**
** Outputs:
**	countptr			Ptr to where to put total count of  
**					records in the table.  
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**					E_DM008A_ERROR_GETTING_RECORD
**					E_DM93A2_DM1R_GET
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	22-may-1995 (cohmi01)
**	    Created for agg optimization project.
**	29-jun-1995 (cohmi01)
**	    Use 'tuplecount' accessor routines for proper access to page.
**      05-sep-1995 (cohmi01)
**          Add dummy parms for compatibilty with dm2r_get parms so this
**          function may be called interchangably with it via function ptr.
**	11-jan-1995 (dougb)
**	    Remove debugging code which should not exist in generic files.
**	 1-jul-96 (nick)
**	    Error handling was screwed ; this would cause nasties later on
**	    such as buffer still fixed / mutexed etc.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          In dm0p_fix_page() call change DM0P_READ flag to DM0P_LOCKREAD
**          to request a page lock even if row locking is enabled. 
**      26-Jun-2001 (horda03) Bug 104534.
**          Calling dmop_fix_page() with DM0P_LOCKREAD has the effect of
**          locking the page even if READLOCK=NOLOCK is enabled. Honour
**          the READLOCK=NOLOCK status, if row locking not enabled.
**	13-Feb-2007 (kschendel)
**	    Toss in an occasional CScancelCheck for cancel checking.
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_?, dm1?_count functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Use dm0pLockBufForGet in addition to DM0P_LOCKREAD.
*/
DB_STATUS
dm1r_count(
    DMP_RCB         *rcb,
    i4         *countptr,
    i4	    direction,
    void	    *record,
    DB_ERROR        *dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*tcb = rcb->rcb_tcb_ptr;
    DB_STATUS	s;
    DM_PAGENO	nextpno, nextovfl, nextdata;
    i4		action;
    i4	count = 0;
    i4		local_err;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    action = (row_locking(r)) ? DM0P_LOCKREAD : DM0P_READ;

    /* prime the pump, read in the first page */
    nextpno = (DM_PAGENO)r->rcb_lowtid.tid_tid.tid_page;
    s = dm0p_fix_page(r, nextpno, action, &r->rcb_data, dberr);
    if ( s != E_DB_OK )
	return s;

    while (r->rcb_data.page)
    {
        /* Housekeeping - check for user interrupt or force abort */
	CScancelCheck(r->rcb_sid);	/* Stupid cancel check */
	if (*(r->rcb_uiptr))
	{
	    if (*(r->rcb_uiptr) & RCB_USER_INTR)
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    else if (*(r->rcb_uiptr) & RCB_FORCE_ABORT)
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
    	    s = E_DB_ERROR;
	    break;
    	}

	/*
	** Lock buffer for get.
	**
	** This may produce a CR page and overlay rcb_data.page
	** with its pointer.
	*/
	dm0pLockBufForGet(r, &r->rcb_data, &s, dberr);
	if ( s != E_DB_OK )
	    break;

	/* keep count of tuples on each page */          
        count += (*tcb->tcb_acc_plv->dmpp_tuplecount)(r->rcb_data.page, 
			tcb->tcb_rel.relpgsize);

        /* Make note of pagenum of next ovflo and data pages, if any */        
	nextdata = DMPP_VPT_GET_PAGE_MAIN_MACRO(tcb->tcb_rel.relpgtype, r->rcb_data.page);
	nextovfl = DMPP_VPT_GET_PAGE_OVFL_MACRO(tcb->tcb_rel.relpgtype, r->rcb_data.page);

	/* NB: dm0p_unfix_page releases pin lock */
	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
	if ( s != E_DB_OK )
	    break;

	/* move to next overflow page, if none, move on to next data page */
	if ((nextpno = nextovfl) == 0)
	{
	    if ((nextpno = nextdata) == 0)
	    {
	        /* No more data to count. Successful termination */   
		s = E_DB_OK;
		break;                       
	    }
	}

	/* get next desired, page, may be main or overflow */ 
    	s = dm0p_fix_page(r, nextpno, action, &r->rcb_data, dberr);
	if ( s != E_DB_OK )
	    break;     
    }

    if (s == E_DB_OK)
    {
	*countptr = count;
	return (E_DB_OK);
    }


    /*  Handle error reporting and recovery. */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93A2_DM1R_GET);
    }

    if (r->rcb_data.page)
    {
	if (dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, &local_dberr) != E_DB_OK)
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, 
		&local_err, 0);
    }
	
    return (s);
}


/*
** Name: dm1r_cvt_row - Convert a tuple record as returned from disk to the   
**		        current format based on the add or drop attribute 
**
** Description:
**    This routine takes an uncompressed tuple record, as retieved from the
**    data page, and moves it to the supplied destination buffer while 
**    modifying according to whether the column attribute indicates an added
**    or dropped column.
**
** Inputs:
**      atts_array                      Pointer to a list of pointers to
**                                      attribute descriptors.
**      atts_count                      Value indicating number of attributes
**      src                             Pointer to an area containing record
**                                      to convert.
**	dst_maxlen			Size of the destination area. If, during
**					the conversion operation, we detect
**					that we are exceeding the destination
**					area size, we abort the conversion
**					operation and return an error.
**
** Outputs:
**      dst                             Pointer to an area to return
**                                      converted record.
**      dst_size                        Size of converted record.
**      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Bad conversion.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	18-jul-1996 (ramra01 for bryanp)
**	   Alter Table support: created. 
**      01-nov-1996 (loera01)
**          For variable-length datatypes, if no data exists for the 
**          column, set the first two bytes to zero.  Also fixed 
**          handling of the null flag.
**	6-aug-99 (inkdo01)
**	    Added code to insert default value for not null with default
**	    added columns.
**      10-apr-2002 (thaju02)
**          Update of a blob column preceded by alter table add long
**          varchar/byte column and modify, results in duplicate logical
**          keys within coupon. Select query fails with E_AD7004, 
**          E_SC0216, E_SC0206. (b107428, INGSRV1738)
**	20-apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	09-May-2005 (thaju02)
**	    For alter table alter column, store attr ptr if 
**	    row version >= ver_added. (B114478)
**	03-Aug-2006 (gupsh01)
**	    Add new ANSI date/time types.
**      30-Jan-2003 (hanal04) Bug 109530 INGSRV 2077
**          Added support for nullable date columns added via
**          ALTER TABLE.
**	29-Aug-2008 (kschendel) Bug 120690
**	    Set exception ADF cb pointer around cvinto calls to handle
**	    arithmetic exceptions (e.g. overflow) during conversion.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Make sure that atts not present originally in the row are
**	    zeroed (or blanked)!  Original switch was missing a default.
*/
DB_STATUS	
dm1r_cvt_row(
			DB_ATTS     **atts_array,
			i4     atts_count,
			char	    *src_param,
			char	    *dst_param,
			i4	    dst_maxlen,
			i4     *dst_size,
			i4	    row_version,
			ADF_CB	*rcb_adf_cb)
{
    u_char	*src = (u_char *) src_param;
    u_char	*dst = (u_char *) dst_param;
    DB_ATTS	*att;
    DB_ATTS	*att_from = (DB_ATTS *)NULL;
    DB_ATTS	*att_to = 0; 
    i4	i;
    i4	width;
    i4	cum_width;
    i2		type;
    u_i2	att_len;
    i4  		err_code;
    DB_DATA_VALUE	fromvalue;
    DB_DATA_VALUE	tovalue;
    STATUS		stat;
    bool		got_sid = FALSE;
    CS_SID		sid;
    DML_SCB		*scb;


    width = 0;
    cum_width = 0;

    for (i = atts_count; i > 0; i--)
    {
	att = atts_array[atts_count - i];
	type = att->type;
	att_len = att->length;

	if ( ((i4)(cum_width + att_len) > dst_maxlen) &&
	   (!att->ver_dropped) )
	      return (E_DB_ERROR);		/* buffer overflow */

	if ( (att->ver_dropped) ||
	   (att->ver_added > row_version) )

	{
	   if (width > 0)
	   {
	      MEcopy((char *)src, width, (char *)dst);
	      src += width;
	      dst += width;
	      width = 0;
	   }

	   if (att->ver_dropped > att->ver_added)
	   {
	      if (att->ver_dropped > row_version)
	      {	
		if (att->ver_altcol)
		{
		  if ( (row_version >= att->ver_added) && 
		       (att->ver_dropped == att->ver_altcol) )
		    att_from = att;
		}	
		else 
		{
		  if (att_from
/* redundant check - if att_from gets reset properly
			&& MEcmp(&att->name, &att_from->name,
				sizeof(DB_ATT_NAME)) == 0
*/
			)
		      src += att_from->length;
		  else
		      src += att_len;

		  att_from = (DB_ATTS *)NULL;
		}

	      }

	      continue;
	   }

	   if ( (att->ver_added > row_version) &&
	      (att->ver_added > att->ver_dropped))
	   {
	      if (att_from
/* redundant check - if att_from gets reset properly
		    && MEcmp(&att->name, &att_from->name,
			sizeof(DB_ATT_NAME)) == 0
*/
		)
	      {

		att_to = att;

		if (! got_sid)
		{
		    CSget_sid(&sid);
		    scb = GET_DML_SCB(sid);
		    got_sid = TRUE;
		}
		if (scb != NULL)
		    scb->scb_qfun_adfcb = rcb_adf_cb;
		rcb_adf_cb->adf_errcb.ad_errclass = 0;
		rcb_adf_cb->adf_errcb.ad_errcode = 0;
		fromvalue.db_datatype = att_from->type;
		fromvalue.db_length = att_from->length;
		fromvalue.db_prec = att_from->precision;
		fromvalue.db_data = (PTR)MEreqmem (0, fromvalue.db_length,
                                                TRUE, NULL);
		fromvalue.db_collID = att_from->collID;
		MEcopy ((char *)src, fromvalue.db_length, 
				(char *)fromvalue.db_data);

		tovalue.db_datatype = att_to->type;
		tovalue.db_length = att_to->length;
		tovalue.db_prec = att_to->precision;
		tovalue.db_data = (PTR)MEreqmem (0, tovalue.db_length,
                                                TRUE, NULL);
		tovalue.db_collID = att_to->collID;

		/* FIXME!! (but not here)
		** This fails for numerous reasons if the old or new type
		** is a blob type.  The most difficult-to-fix reason is
		** that if we convert old-blob to new-blob, and then eventually
		** later rewrite the row, we have to delete the old etab
		** rows!  or, if the row ISN'T rewritten, we'd have to delete
		** the new version.  It's all just too hard to keep track of,
		**
		** The parser should be fixed to disallow alter table alter
		** column when the new or old type is blob.
		*/
		stat = adc_cvinto (rcb_adf_cb, &fromvalue, &tovalue);
		if (scb != NULL)
		    scb->scb_qfun_adfcb = rcb_adf_cb;
		if (stat != E_DB_OK)
		{
        	   err_code = E_DM019D_ROW_CONVERSION_FAILED;
		   uleFormat(NULL, err_code, (CL_ERR_DESC *)NULL, 
			ULE_LOG, NULL, (char *)NULL,
            		(i4)0, (i4 *)NULL, &err_code, 0);

		   /* clean up first */
                   if (fromvalue.db_data)
                     MEfree (fromvalue.db_data);

                    if (tovalue.db_data)
                      MEfree (tovalue.db_data);

        	  return (err_code);
		}
		MEcopy ( (char *)tovalue.db_data, tovalue.db_length, 
					(char *)dst );
		dst += att_len;
		src += att_from->length;
		cum_width += att_len; 
		att_from = (DB_ATTS *)NULL;

		/* clean up first */
                if (fromvalue.db_data)
                  MEfree (fromvalue.db_data);

                if (tovalue.db_data)
                  MEfree (tovalue.db_data);

		continue;
	      }
	      else 
	      {
		/* Only other possibility is that the column was added
		** after this row was inserted, so fake up a default value.
		** Generate zeros and null-indicator set if nullable;
		** blanks for C, CHAR, NCHAR;  zeros for everything else.
		** FIXME someday we need to be able to do value defaults,
		** if some higher level can discover what those are and
		** put them in some accessible place.
		*/
		if (type < 0)
		{
		    MEfill(att_len-1, 0, dst);
		    dst[att_len-1] = ADF_NVL_BIT;
		}
		else
		{
		    switch (type)
		    {
		    case DB_CHA_TYPE:
		    case DB_CHR_TYPE:
			MEfill(att_len, ' ', dst);
			break;

		    case DB_NCHR_TYPE:
			{
			    i4 char_len = att_len / sizeof(UCS2);
			    u_i2 *p = (u_i2 *) dst;
			    u_i2 u_ch = U_BLANK;

			    while (--char_len >= 0)
			    {
				I2ASSIGN_MACRO(u_ch, *(i2 *)p);
				++p;
			    }
			}
			break;

		    case DB_DEC_TYPE:
			/* Need to include decimal plus-sign */
			MEfill(att_len-1, 0, dst);
			dst[att_len-1] = MH_PK_PLUS;
			break;

		    default:
			MEfill(att_len, 0, dst);
			break;
		    }
		}

		dst += att_len;
		cum_width += att_len;
		continue;
	     }	
	   }
	}
	else
	{
	   width += att_len;
	   cum_width += att_len;
	}
    }

    if (width > 0)
    {
       MEcopy((char *)src, width, (char *)dst);
    }

    *dst_size = cum_width;
    return (E_DB_OK);
}


/*{
** Name: dm1r_allocate - Allocates space for a record on data page
**
** Description:
**      This routine checks the current page for space.
**      It will first mutex the page and remove committed deleted records.
**         If page locking, deleted records may unconditionally be deleted.
**         If row locking,  deleted records may be deleted if the transaction
**         is no longer active.
**      If row locking and a tid is allocated, the tid will be locked.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      data                            Pointer to a pointer to the page
**					to allocate from.
**      record                          Pointer to the record.
**      record_size                     Amount of space needed
**
** Outputs:
**      tid				Pointer to an area to return the tid
**					of the allocated space.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      01-jul-2000 (stial01)
**          Re-written... page type dependent code moved to dmpp_allocate
*/
DB_STATUS
dm1r_allocate(
    DMP_RCB	*rcb, 
    DMP_PINFO   *pinfo,
    char        *record, /* if compression, this is compressed record */
    i4     record_size,
    DM_TID      *tid, 
    DB_ERROR	*dberr )
{
    DMP_RCB         *r = rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS	    status = E_DB_OK;
    i4         new_lock;
    LK_LOCK_KEY	    save_lock_key;
    i4         length;
    i4         update_mode;
    i4              flags;
    i4		line_start;
    i4		retry_cnt = 0;
    i4		page_cleaned = FALSE;
    i4		orig_size = record_size;
    i4		max;
    i4		rem;
    DMPP_SEG_HDR	seg_hdr;
    DMPP_SEG_HDR	*seg_hdr_ptr = NULL;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (t->tcb_seg_rows)
    {
	max = dm2u_maxreclen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize);

#ifdef xDEBUG
	TRdisplay("dm1r_allocate record_size %d max %d\n", record_size, max);
#endif

	/*
	** Rows that span pages cannot share pages with other rows
	** Set record size such that we will always allocate a new page
	*/
	record_size = max;
    }

    /*
    ** Consistency checks: make sure input data makes sense.
    */
    if (pinfo->page == NULL)
	dmd_check(E_DMF010_DM1R_NO_PAGE);

    /* We don't expect to be called with a CR page */
    if ( crow_locking(r) )
    {
        if ( DMPP_VPT_IS_CR_PAGE(t->tcb_rel.relpgtype, pinfo->page) )
	    TRdisplay("%@ dm1r_allocate:%d called with CR page %x, this is bad\n",
			__LINE__, pinfo->page);
    }

    /* Allocate space */
    for (line_start = 0; ; )
    {
	status = (*t->tcb_acc_plv->dmpp_allocate)
			(pinfo->page, r, record, record_size, tid, line_start);

	if (status == E_DB_WARN && !page_cleaned &&
	    (t->tcb_rel.relcomptype != TCB_C_NONE || t->tcb_rel.relversion) &&
	    (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pinfo->page) & 
								DMPP_CLEAN))
	{
	    i4	clean_bytes = 0;

	    /* try page clean once */
	    /* if row/crow/phys locking can only clean committed deletes */
	    page_cleaned = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
	    status = (*t->tcb_acc_plv->dmpp_clean)(
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, pinfo->page,
		    &r->rcb_tran_id, 0, &clean_bytes);
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
	    if (status != E_DB_OK)
		break;
	    if (clean_bytes >= record_size)
		continue;
	}

	if (status != E_DB_OK || (!row_locking(r) && !crow_locking(r)))
	    break;

	/* Get an LK_X CROW lock on tid */
	if ( crow_locking(r) )
	{
	    /* Can't wait if buffer is locked */
	    status = dm1r_crow_lock(r, 
	    		(dm0pBufIsLocked(pinfo)) ? LK_NOWAIT : 0, 
			tid, NULL, dberr);
	}
	else if ((t->tcb_rel.relstat & TCB_INDEX) == 0)
	{
	    flags = DM1R_ALLOC_TID;
	    if (dm0pBufIsLocked(pinfo))
		flags |= DM1R_LK_NOWAIT; /* can't wait if buffer is locked */
	    status = dm1r_lock_row(r, flags, tid,
					&new_lock, &save_lock_key,
					dberr);
	}

	if ( DB_FAILURE_MACRO(status) )
	{
	    if (dberr->err_code == E_DM004B_LOCK_QUOTA_EXCEEDED)
	    {
		/*
		** Escalate to table level locking because 
		** 'locks per transaction' limit exceeded.
		*/

		/* Unlock buffer before escalation */
		dm0pUnlockBuf(r, pinfo);

		uleFormat(NULL, E_DM9041_LK_ESCALATE_TABLE, (CL_ERR_DESC *)NULL, 
			   ULE_LOG, 
			   NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 
			   2, sizeof(DB_TAB_NAME), &t->tcb_rel.relid, 
			   sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
		status = dm0p_escalate(r, 0, dberr);

		if (status == E_DB_OK)
		{
		    line_start = 0;
		    continue;
		}
		else
		    break;
	    }

	    else if (dberr->err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	    {
		/*
		** NOWAIT Row lock request can fail if this is a btree
		** and another session locked the deleted leaf entry.
		** The other session will release the lock once it
		** sees the delete is committed.
		**
		** Try to find another deleted row on this data page.
		*/
		line_start = tid->tid_tid.tid_line + 1;
		retry_cnt++;
		if (line_start <= DM_TIDEOF)
		    continue;
		else
		{
		    status = E_DB_INFO;
		    break;
		}
	    }
	    /* Other lock errors are deadlock related */
	    TRdisplay("dm1r_allocate lock error %d (%d,%d)\n", dberr->err_code,
		    tid->tid_tid.tid_page, tid->tid_tid.tid_line);
	    break;
	}

	/* Reserve space on page if still (c)row locking */
	if (status == E_DB_OK && (row_locking(r) || crow_locking(r)))
	{
#ifdef xDEBUG
	    TRdisplay("Reserve table (%d %d) tid %d %d\n", 
		    t->tcb_rel.reltid.db_tab_base,
		    t->tcb_rel.reltid.db_tab_index,
		    tid->tid_tid.tid_page, tid->tid_tid.tid_line);
#endif

	    /*
	    ** Reserve is always DIRECT update
	    ** If deferred, we'll do deferred processing when we do update
	    ** Same as in dm1bxreserve.
	    */
	    update_mode = DM1C_DIRECT | DM1C_ROWLK;

	    /*
	    ** Mutex the page while we change it to prevent other
	    ** threads from looking at it while it is inconsistent
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);

	    /*
	    ** Write the new record to the data page.
	    */
	    (*t->tcb_acc_plv->dmpp_put)(t->tcb_rel.relpgtype, 
		    t->tcb_rel.relpgsize, pinfo->page, update_mode, 
		    &r->rcb_tran_id, r->rcb_slog_id_id, tid, record_size, record,
		    t->tcb_rel.relversion, (DMPP_SEG_HDR *)NULL);

	    /*
	    ** Mark the new record as 'deleted'
	    ** reclaim space = FALSE
	    */
	    (*t->tcb_acc_plv->dmpp_delete)(t->tcb_rel.relpgtype, 
			t->tcb_rel.relpgsize, pinfo->page, update_mode, 
			    (i4)FALSE, &r->rcb_tran_id, r->rcb_slog_id_id, 
			    tid, record_size);

	    /* Unmutex the buffer */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);
	}

	break; /* always */
    }

    if (status == E_DB_ERROR && dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9318_DM1R_ALLOCATE);
    }

    if (status == E_DB_ERROR && dberr->err_code == 0)
    {
	TRdisplay("dm1r_allocate Error no errcode\n");
    }
    return (status);
}

/*{
** Name: dm1r_check_lock - Check row locked for specified tid. 
**
** Description:
**
**	This routine is a utility used to check if a row is
**      logically locked.
**
** Inputs:
**      rcb             
**      tid
**
** Outputs:
**	E_DB_OK if row locked.
**	E_DB_ERROR if row not locked.
**
**
** Exceptions:
**		None
**
** Side effects:
**		None
**
** History:
**      29-sep-1997 (stial01)
**          Created from dm1cs_check_lock()
**      08-dec-1998 (stial01)
**          Added more trace information when lock is not found
**      11-dec-1998 (stial01)
**          Retry LKshow, sometimes it fails even if lock exists.
**      28-dec-1998 (stial01)
**          dm1r_check_lock() Use new LKshow flag LK_S_OWNER_GRANT.
**          First try is dirty, second (and last) try is semaphore protected.
**	12-feb-1999 (nanpr01)
**	    Support update mode lock.
**      22-jan-1999 (schte01)
**          Changed variable named "try" to "tries" to avoid conflict with 
**          the #define for try in c_excpt.h
**      13-may-2005 (horda03) Bug 114508/INGSRV3301
**          It is also a protocol errorif a physical row lock is being
**          used to protect the update.
**	28-Jul-2004 (schka24)
**	    Check against proper partition, if partitioned.
**	17-Feb-2005 (schka24)
**	    Above used wrong link to master, indexes link via parent ptr,
**	    not pmast ptr.
**	27-Jul-2007 (kibro01) b114508/b118857
**	    Corrected error in original cross-integration
*/
DB_STATUS
dm1r_check_lock(
    DMP_RCB     *rcb,
    DM_TID      *tid)
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = r->rcb_tcb_ptr;
    LK_LOCK_KEY         lock_key;
    LK_LKB_INFO         info_buf;
    CL_ERR_DESC          sys_err;
    u_i4		info_result;
    STATUS              s;
    i4             row_lock_mode;
    i4                  flag;
    bool                protocol_err = FALSE;
    i4			reltidx;

    flag = LK_S_OWNER_GRANT;

    reltidx = t->tcb_rel.reltid.db_tab_index;
    if (reltidx > 0)
	reltidx = 0;			/* Base table, not index */
    if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
    {
	/* We're a global index, check against proper partition */

	if ( t->tcb_parent_tcb_ptr->tcb_pp_array )
	    reltidx = t->tcb_parent_tcb_ptr->tcb_pp_array[r->rcb_partno].pp_tabid.db_tab_index;
    }

    if ((r->rcb_k_mode == RCB_K_IX) || (r->rcb_k_mode == RCB_K_X))
	if (r->rcb_state & RCB_UPD_LOCK)
	    row_lock_mode = LK_U;
	else
	    row_lock_mode = LK_X;
    else
	row_lock_mode = LK_S;

    lock_key.lk_type = LK_ROW;
    lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
    lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
    lock_key.lk_key3 = reltidx;
    lock_key.lk_key4 = tid->tid_tid.tid_page;
    lock_key.lk_key5 = tid->tid_tid.tid_line;
    lock_key.lk_key6 = 0;
    /* Length of zero returned if no matching key */
    s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key, 
		sizeof(info_buf), (PTR)&info_buf, &info_result, 
		(u_i4 *)NULL, &sys_err);

    if (s == OK && (info_result != 0))
    {
       if (info_buf.lkb_grant_mode == LK_X || 
           info_buf.lkb_grant_mode == LK_U || row_lock_mode == LK_S)
       {
          if (t->tcb_sysrel || t->tcb_extended ||
              !(info_buf.lkb_flags & LKB_PHYSICAL) )
          {
             /* Allow physical locks on system catalogues and
             ** BLOB extension tables.
             */
             return (E_DB_OK);
          }

          /* Not a logical lock, so protocol error. */

          protocol_err = TRUE;
       }
    }

    
    if (!protocol_err)
    {
       /* See if we have a table lock */
       lock_key.lk_type = LK_TABLE;
       lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
       lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
       lock_key.lk_key3 = t->tcb_rel.reltid.db_tab_index;
       lock_key.lk_key4 = 0;
       lock_key.lk_key5 = 0;
       lock_key.lk_key6 = 0;

       /* Length of zero returned if no matching key */
       s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key, 
		   sizeof(info_buf), (PTR)&info_buf, &info_result, 
		   (u_i4 *)NULL, &sys_err);

       if (s == E_DB_OK && (info_result != 0))
       {
          if ((info_buf.lkb_grant_mode == LK_X) || 
              (info_buf.lkb_grant_mode == LK_S && row_lock_mode == LK_S) ||
              (info_buf.lkb_grant_mode == LK_U && row_lock_mode == LK_U))
          {
             if (t->tcb_sysrel || t->tcb_extended ||
                 !(info_buf.lkb_flags & LKB_PHYSICAL) )
             {
                /* Allow physical locks on system catalogues and
                ** BLOB extension tables.
                */
                return (E_DB_OK);
             }
	  }
       }
    }

    TRdisplay("CHECK LOCK Id: %x Tran_id %x,%x Req:%3w %~t (%d,%d)(%d,%d) flags:%v\n",
	r->rcb_lk_id,
	r->rcb_tran_id.db_high_tran, r->rcb_tran_id.db_low_tran,
	"N,IS,IX,S,SIX,U,X", row_lock_mode,
	sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid,
	t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
	tid->tid_tid.tid_page, tid->tid_tid.tid_line,
        "PHYS,TEMP,CANCEL,ENQW,CONCANC,PROP,ON_DLOCK,DEADLOCK,ON_LLB_W,IS_RSB",
        info_buf.lkb_flags);
    TRflush();

    dmd_lock_info(DMTR_LOCK_USER_LISTS);
    return (E_DB_ERROR);
}

/*
** Name: dm1r_check_page_lock - Check logical page locked.
**
** Description:
**
**    This routine is a utility used to check if a page is locked
**      with a logical lock.
**
** Inputs:
**      rcb
**      page
**
** Outputs:
**    E_DB_OK if page locked.
**    E_DB_ERROR if page not locked.
**
**
** Exceptions:
**            None
**
** Side effects:
**            None
**
** History:
**      13-may-2005 (horda03)
**          Created from dm1r_check_lock()
*/
DB_STATUS
dm1r_check_page_lock(
    DMP_RCB     *rcb,
    i4          page)
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = r->rcb_tcb_ptr;
    LK_LOCK_KEY         lock_key;
    LK_LKB_INFO         info_buf;
    CL_ERR_DESC          sys_err;
    u_i4                info_result;
    STATUS              s;
    i4                  lock_mode;
    i4                  flag;
    bool                protocol_err = FALSE;

    flag = LK_S_OWNER_GRANT;

    if ((r->rcb_k_mode == RCB_K_IX) || (r->rcb_k_mode == RCB_K_X))
      if (r->rcb_state & RCB_UPD_LOCK)
          lock_mode = LK_U;
      else
          lock_mode = LK_X;
    else
      lock_mode = LK_S;

    lock_key.lk_type = LK_PAGE;
    lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
    lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
    lock_key.lk_key3 = t->tcb_rel.reltid.db_tab_index;
    lock_key.lk_key4 = page;
    lock_key.lk_key5 = 0;
    lock_key.lk_key6 = 0;

    /* Length of zero returned if no matching key */
    s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key,
              sizeof(info_buf), (PTR)&info_buf, &info_result,
              (u_i4 *)NULL, &sys_err);

    if (s == OK && (info_result != 0))
    {
       if (info_buf.lkb_grant_mode == LK_X ||
           info_buf.lkb_grant_mode == LK_U || lock_mode == LK_S)
       {
          if (t->tcb_sysrel || t->tcb_extended ||
              !(info_buf.lkb_flags & LKB_PHYSICAL) )
          {
             /* Allow physical locks on system catalogues and
             ** BLOB extension tables.
             */
             return (E_DB_OK);
          }

          /* Not a logical lock, so protocol error. */

          protocol_err = TRUE;
       }
    }

    if (!protocol_err)
    {
       /* See if we have a table lock */
       lock_key.lk_type = LK_TABLE;
       lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
       lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
       lock_key.lk_key3 = t->tcb_rel.reltid.db_tab_index;
       lock_key.lk_key4 = 0;
       lock_key.lk_key5 = 0;
       lock_key.lk_key6 = 0;

       /* Length of zero returned if no matching key */
       s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key,
                 sizeof(info_buf), (PTR)&info_buf, &info_result,
                 (u_i4 *)NULL, &sys_err);

       if (s == E_DB_OK && (info_result != 0))
       {
          if ((info_buf.lkb_grant_mode == LK_X) ||
              (info_buf.lkb_grant_mode == LK_S && lock_mode == LK_S) ||
              (info_buf.lkb_grant_mode == LK_U && lock_mode == LK_U))
          {
             if (t->tcb_sysrel || t->tcb_extended ||
                 !(info_buf.lkb_flags & LKB_PHYSICAL) )
             {
                /* Allow physical locks on system catalogues and
                ** BLOB extension tables.
                */
                return (E_DB_OK);
             }
          }
       }
    }

    TRdisplay("CHECK PAGE LOCK Id: %x Tran_id %x,%x Req:%3w %~t (%d,%d)(%d,%d) flags:%v\n",
        r->rcb_lk_id,
        r->rcb_tran_id.db_high_tran, r->rcb_tran_id.db_low_tran,
        "N,IS,IX,S,SIX,U,X", lock_mode,
        sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid,
        t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
        page, 0,
        "PHYS,TEMP,CANCEL,ENQW,CONCANC,PROP,ON_DLOCK,DEADLOCK,ON_LLB_W,IS_RSB",
        info_buf.lkb_flags);

    TRflush();

    dmd_lock_info(DMTR_LOCK_USER_LISTS);
    return (E_DB_ERROR);
}


/*{
** Name: dm1r_rowlk_access - Row locking access checking primatives
**
** Description:
**
**	This routine determines if a row lock is needed for the specified
**      row access type, and if it is, it gets the row lock.
**
**      For HEAP/HASH/ISAM tables, the following types of access are defined:
**
**      ROW_ACC_NONKEY:   Need to do non key qualification.
**                        Since we choose to do IN PLACE REPLACE for NON KEY
**                        updates, we cannot do NON-KEY qualifications on
**                        uncommitted data.
**
**      ROW_ACC_RETURN:   About to return qualified record.
**                        If cursor stability, we need PHYSICAL row lock,
**                        otherwise we need LOGICAL row lock.	
**
** Inputs:
**      rcb             
**      get_status         return code from get
**      access             Type of row access
**      row_low_tran
**      tid                Tid of record we read
**      tid_to_lock        Tid to lock (if reading secondary index, may be
**                         different from tid)
**      save_lock_key      Previous locked tid. Unlock if different from
**                         tid_to_lock.
**      opflag             Value indicating type of retrieval
** 
**
** Outputs:
**	E_DB_OK if row locked.
**	E_DB_ERROR if row not locked.
**
**
** Exceptions:
**		None
**
** Side effects:
**              The buffer is assumed to be locked. If this routine gets an
**              error the buffer will always be unlocked before returning.
**              If no error, when we return the buffer is always locked.
**
** History:
**      07-jul-1998 (stial01)
**          Created.
**      31-aug-1998 (stial01)
**          If convert NOWAIT fails, try convert WAIT.
**      05-oct-1998 (stial01)
**          Use new accessor to clear tranid in tuple header
**      12-nov-98 (stial01)
**          dm1r_rowlk_access() if (opflag & DM1C_GET_SI_UPD), no row locking
**	06-apr-1999 (somsa01)
**	    Fixed typo where we would return without an error code; the error
**	    should be E_DB_OK.
**	22-Jan-2004 (jenjo02)
**	    If locking entry in Global index, caller must
**	    supply tid_to_lock's partition number.
**	17-Feb-2005 (schka24)
**	    Oops, follow parent ptr to get from index to master, not the
**	    pmast ptr (which is for partitions).
*/
static DB_STATUS
dm1r_rowlk_access(
    DMP_RCB        *rcb,
    i4        get_status,
    i4        access,
    u_i4	   *row_low_tran,
    u_i2	   *row_lg_id,
    DM_TID         *tid,
    DM_TID         *tid_to_lock,
    i4		   partno_to_lock,
    LK_LOCK_KEY    *save_lock_key,
    i4        opflag,
    DB_ERROR       *dberr)
{
    DMP_RCB         *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS       s = E_DB_OK;
    i4         new_lock;
    DB_STATUS       lock_status;
    i4         flags;
    bool            needlock;
    DM_TID          unlock_tid;
    DB_TAB_ID	    row_reltid;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    /*
    ** Skip rows deleted by current transaction
    ** NOTE: This row could've been deleted by a previous transaction
    ** that had the same low tranid (tranids can recycle). 
    ** In both cases, the deleted row can be skipped.
    */
    if (get_status == E_DB_WARN && *row_low_tran == r->rcb_tran_id.db_low_tran)
	return (E_DB_WARN);

    /*
    ** No row locks needed when updating secondary indexes
    ** We should already have the base table page/row lock.
    ** And we don't want to get uneccessary locks looking for the 
    ** index entry to update (HASH/ISAM).
    */
    if (opflag & DM1C_GET_SI_UPD)
	return (get_status);

    /* 
    ** Do we need a lock for the specified row access ?
    */
    if ( (access == ROW_ACC_RETURN) ||
	(access == ROW_ACC_NONKEY && r->rcb_iso_level == RCB_SERIALIZABLE))
    {
	needlock = TRUE;
    }
    else
    {
	/*
	** Check if row is committed
	**
	** Non key qualifications can be applied to committed data without
	** locking (all row locking inserts put low tranid in tuple header)
	*/
	if ( !IsTranActive(*row_lg_id, *row_low_tran) )
	{
	    /* Reset row_low_tran to avoid extra calls to LG */
	    *row_low_tran = 0;
	    *row_lg_id = 0;

	    /* If committed delete... skip it */
	    if (get_status == E_DB_WARN)
		return (E_DB_WARN);

	    /* Data is committed... We don't need to lock yet */
	    return (E_DB_OK);
	}
	else
	{
	    /*
	    ** Uncommitted data....
	    ** We need the row lock before certain access types
	    */
	    needlock = TRUE;
	}
    }

    if (needlock == FALSE)
	return (E_DB_OK);

    if (tid_to_lock == NULL)
	return (E_DB_ERROR);

    /* Determine TID's reltid */
    if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX &&
	 tid->tid_i4 != tid_to_lock->tid_i4 )
    {
	/*
	** Then we have to find the reltid of the
	** TID's partition.
	*/
	STRUCT_ASSIGN_MACRO(
	    t->tcb_parent_tcb_ptr->tcb_pp_array[partno_to_lock].pp_tabid,
		row_reltid);
    }
    else
	STRUCT_ASSIGN_MACRO(
	    t->tcb_parent_tcb_ptr->tcb_rel.reltid, row_reltid);

    /*
    ** Try to get row locked NOWAIT
    ** Always request PHYSICAL if the request is conditional, since
    ** we don't know if the row qualifies yet.
    */
    if (r->rcb_iso_level == RCB_SERIALIZABLE)
	flags = 0;
    else if (r->rcb_iso_level == RCB_CURSOR_STABILITY 
	    || access != ROW_ACC_RETURN)
	flags = DM1R_LK_PHYSICAL;
    else
	flags = 0; /* Default is logical row lock */

    /*
    ** Do we have a row locked
    */
    if (save_lock_key->lk_type == LK_ROW)
    {
	/* Is it the one we want, did we just need physical */
	if ( save_lock_key->lk_key2 == row_reltid.db_tab_base &&
	     save_lock_key->lk_key3 == row_reltid.db_tab_index &&
	     save_lock_key->lk_key4 == tid_to_lock->tid_tid.tid_page &&
	     save_lock_key->lk_key5 == tid_to_lock->tid_tid.tid_line )
	{
	    if (r->rcb_iso_level == RCB_SERIALIZABLE)
		return (E_DB_OK);
	    if (flags & DM1R_LK_PHYSICAL)	
		return (E_DB_OK);
	    else
		flags = DM1R_LK_CONVERT;
	}
	else
	{
	    if (r->rcb_iso_level != RCB_SERIALIZABLE)
		_VOID_ dm1r_unlock_row(r, save_lock_key, &local_dberr);
	    save_lock_key->lk_type = 0;
	}
    }

    /* Set rcb_partno for dm1r_lock_row */
    r->rcb_partno = partno_to_lock;

    lock_status  = dm1r_lock_row(r, (i4)(DM1R_LK_NOWAIT | flags),
			tid_to_lock,
			&new_lock, save_lock_key, dberr);

    if (lock_status == E_DB_OK)
    {
	if (get_status == E_DB_WARN)
	    return (E_DB_WARN);
	else
	    return (E_DB_OK);
    }
    /* Don't assume all non-zero lock_status means "LK_BUSY" */
    else if ( dberr->err_code != E_DM004C_LOCK_RESOURCE_BUSY )
        return(lock_status);

    /*
    ** If we had a PHYSICAL lock, CONVERT NOWAIT may not be successful.
    ** When converting a PHYSICAL lock to logical, the lock becomes
    ** countable and may require escalation which will not be done 
    ** when DM1R_LK_NOWAIT is specified.
    ** 
    ** Otherwise, when we WAIT, always request PHYSICAL row lock,
    ** We have to requalify the row, and if it no longer qualifies,
    ** we want to be able to release it.
    */
    if (r->rcb_iso_level != RCB_SERIALIZABLE && flags != DM1R_LK_CONVERT)
	flags = DM1R_LK_PHYSICAL;

    /*
    ** Need to WAIT for lock, unlock buffer first
    */
    dm0pUnlockBuf(r, &r->rcb_data);

    /*
    ** When we WAIT, always request PHYSICAL row lock,
    ** We have to requalify the row, and if it no longer qualifies,
    ** we want to be able to release it.
    */
    lock_status  = dm1r_lock_row(r, flags, tid_to_lock,
					&new_lock, save_lock_key, 
					dberr);

    if (lock_status == E_DB_ERROR)
	return (E_DB_ERROR);

    /*
    ** Must return with the buffer locked
    */
    dm0pLockBufForGet(r, &r->rcb_data, &s, dberr); 

    /*
    ** The row is locked, reposition to re-get same row
    */
    if (r->rcb_lowtid.tid_tid.tid_line == 0)
	r->rcb_lowtid.tid_tid.tid_line = DM_TIDEOF;
    else
	r->rcb_lowtid.tid_tid.tid_line--;

    return (E_DB_WARN);
}


/*{
** Name: dm1r_lk_convert    Converts locks on newly allocated pages 
**                          (PHYSICAL X -> LOGICAL IX) (row_locking)
**			    or releases physical X lock on crow_locking page.
**
** Description:
**      This routine converts locks on newly allocated row_locking pages
**	or releases the physical X lock on a crow_locking page.
**
** Inputs:
**      rcb                             Pointer to record access context
**	pageno				Page number of allocated page
**      page				Pointer to allocated page, may be NULL
**	BufLocked			Pointer to bool noting if page's
**					buffer is MLOCKed.
**      lk_id
**
** Outputs:
**      err_code
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      22-sep-98 (stial01)
**          Created.
**		29-Sep-2004 (fanch01)
**      	Add LK_LOCAL flag since row locking is always confined to one node.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    When crow locking, no page lock to convert.
*/
DB_STATUS
dm1r_lk_convert(
DMP_RCB     *r,
DM_PAGENO    pageno,
DMP_PINFO    *pinfo,
LK_LKID      *lkid,
DB_ERROR     *dberr)
{
    DMP_TCB     *t = r->rcb_tcb_ptr;
    i4	lock_action = 0;
    STATUS		cl_status;
    CL_ERR_DESC	sys_err;
    LK_VALUE	lk_value; /* fake, but must have one */
    DB_STATUS   status = E_DB_OK;

    CLRDBERR(dberr);

    /* If page's buffer is pinned, unpin it first */
    if ( pinfo && dm0pBufIsLocked(pinfo) )
        dm0pUnlockBuf(r, pinfo);

    if ( row_locking(r) )
    {
	/* 
	** If row_locking, the physical X lock will be converted to 
	** a logical IX.
	*/
	cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT | LK_LOCAL,
			r->rcb_lk_id, (LK_LOCK_KEY *)NULL, LK_IX,
			&lk_value, lkid, (i4)0, &sys_err);
	if (cl_status != OK)
	{
	    SETDBERR(dberr, 0, cl_status);
	    status = E_DB_ERROR;
	}
	else
	{

	    /* Now convert PHYSICAL lock to logical (DM0P_IGNORE_LKLIMIT) */
	    status = dm0p_trans_page_lock(r, t, pageno,
		    DM0P_WRITE | DM0P_IGNORE_LKLIMIT, 
		    LK_IX, &lock_action, lkid, dberr);
	}
    }

    return (status);
}

/*{
** Name: dm1r_lock_downgrade Converts update locks shared mode.
**
** Description:
**      This routine converts update lock to shared lock.
**
** Inputs:
**      rcb                             Pointer to record access context
**      pageno
**      lk_id
**
** Outputs:
**      err_code
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      22-feb-99 (nanpr01)
**          Created.
**	05-may-1999 (nanpr01)
**	    Once the lock is downgraded, we donot need to downgrade it.
**      17-Oct-2003 (hanal04) Bug 111116 INGSRV2554
**          Return E_DB_OK if LKrequest() returns LK_LOG_LOCK status.
**  29-Sep-2004 (fanch01)
**      Add LK_LOCAL flag since row locking is always confined to one node.
*/
DB_STATUS
dm1r_lock_downgrade(
DMP_RCB     *r,
DB_ERROR    *dberr)
{
    DMP_TCB     *t = r->rcb_tcb_ptr;
    LK_LOCK_KEY lock_key;
    i4	lock_list = r->rcb_lk_id;
    i4	lock_mode = LK_S;
    i4  lk_flags = LK_STATUS | LK_LOCAL;
    CL_ERR_DESC	sys_err;
    DB_STATUS   status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if ((r->rcb_state & RCB_UPD_LOCK) == 0)
    {
	TRdisplay("Error calling dm1r_lock_downgrade for %~t\n",
	    sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
	return (E_DB_ERROR);
    }
    lock_key.lk_type = LK_ROW;
    lock_key.lk_key1 = (i4)t->tcb_dcb_ptr->dcb_id;
    lock_key.lk_key2 = r->rcb_locked_tid_reltid.db_tab_base;
    lock_key.lk_key3 = r->rcb_locked_tid_reltid.db_tab_index;
    lock_key.lk_key4 = r->rcb_locked_tid.tid_tid.tid_page;
    lock_key.lk_key5 = r->rcb_locked_tid.tid_tid.tid_line;
    lock_key.lk_key6 = 0;

    /* Output a lock trace message if tracing is enabled. */
    /* FIXME wrong table name if partitioned and global index, no
    ** trivial fix -- similar problem elsewhere.
    */
    if (DMZ_SES_MACRO(1))
	dmd_lock(&lock_key, lock_list, LK_REQUEST, lk_flags, lock_mode,
		r->rcb_timeout, &r->rcb_tran_id, &t->tcb_rel.relid, 
		&t->tcb_dcb_ptr->dcb_name);
 
    status = LKrequest(lk_flags, lock_list, &lock_key, lock_mode,
            (LK_VALUE *)NULL, &r->rcb_row_lkid, r->rcb_timeout, &sys_err);
    if (status == LK_LOG_LOCK)
    {
        status = E_DB_OK;
    }
    if (status != E_DB_OK)
    {
	TRdisplay("Failed to downgrade on Table [%d,%d] Tid %d,%d status %d,%d\n",
		lock_key.lk_key2, lock_key.lk_key3, lock_key.lk_key4, 
		lock_key.lk_key5, status, sys_err);
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM905B_BAD_LOCK_ROW, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
            (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6, 0, lock_key.lk_key4, 
	    0, lock_key.lk_key5, sizeof(DB_TAB_NAME), &t->tcb_rel.relid, 
	    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name, 0, lock_mode, 
	    0, lock_list);
	/* Caller may choose to ignore this error... log the error here */
	uleFormat(NULL, E_DM9077_ERROR_LOCK_DOWNGRADE, (CL_ERR_DESC *)NULL, ULE_LOG,
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9077_ERROR_LOCK_DOWNGRADE);
	status = E_DB_ERROR;
    }
    else
    {
        r->rcb_locked_tid.tid_i4 = -1;
	r->rcb_locked_tid_reltid.db_tab_base = 0;
	r->rcb_locked_tid_reltid.db_tab_index = 0;
    }
    return (status);
}


/*{
** Name: dm1r_get_segs - Get record when row spans pages.
**
** Description:
**      Get record when row spans pages.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      data				Page containing first segment
**      first_seg_hdr			Segment header of first segment
**      first_seg			Pointer to first segment data
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**					E_DM938B_INCONSISTENT_ROW
**
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**      10-feb-2004 (stial01)
**          Added for 256K rows, rows spanning pages.
**	28-jul-2008 (gupsh01)
**	    In case the record is compressed, expand the
**	    record boundary checking to allow for expansion
**	    in worst case for char and nchar types.
*/
DB_STATUS
dm1r_get_segs(
DMP_RCB		*rcb,
DMPP_PAGE	*data,
DMPP_SEG_HDR	*first_seg_hdr,
char		*first_seg,
char		*record,
DB_ERROR	*dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_SEG_HDR	seg_hdr;
    DM_TID		seg_tid;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_stat;
    i4			row_version = 0;	
    char		*seg;
    char		*rec;
    char		*endrec;
    i4			record_size;
    i4			fix_action;
    i4			seg_num;
    DM_PAGENO		firstpno;
    i4			adjusted_reclen;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO		segPinfo;
    DMPP_PAGE		**segpage = &segPinfo.page;

    CLRDBERR(dberr);

    *segpage = NULL;

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;

    /* More segments for this row to get */

    /* If the record is compressed adjust the record length
    ** for expansion in record length.
    */
    adjusted_reclen = t->tcb_rel.reltotwid + t->tcb_data_rac.worstcase_expansion;

    rec = record;
    endrec = record + adjusted_reclen;

#ifdef xDEBUG
    TRdisplay("dm1r_get_segs 1st seg (seghdr %d,%d) \n", 
	first_seg_hdr->seg_len, first_seg_hdr->seg_next);
#endif

    /* Process first segment */
    STRUCT_ASSIGN_MACRO(*first_seg_hdr, seg_hdr);
    MEcopy(first_seg, first_seg_hdr->seg_len, rec);
    rec += seg_hdr.seg_len;

    for (seg_num = 2; seg_hdr.seg_next; seg_num++)
    {
	status = dm0p_fix_page(r, seg_hdr.seg_next, fix_action, &segPinfo, 
		dberr);
	if (status != E_DB_OK)
	    break;

	seg_tid.tid_tid.tid_page = seg_hdr.seg_next;
	seg_tid.tid_tid.tid_line = 0;

	status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
	    t->tcb_rel.relpgsize, *segpage, &seg_tid, 
	    &record_size, &seg, &row_version, NULL, NULL, &seg_hdr);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    break;
	}

#ifdef xDEBUG
	TRdisplay("dm1r_get_segs seg %d (seghdr %d,%d) tid (%d,%d)\n", 
	    seg_num, seg_hdr.seg_len, seg_hdr.seg_next,
	    seg_tid.tid_tid.tid_page, seg_tid.tid_tid.tid_line);
#endif

	/* consistency check before MEcopy */
	if (rec + seg_hdr.seg_len > record + adjusted_reclen)
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    status = E_DB_ERROR;
	    break;
	}

	MEcopy(seg, seg_hdr.seg_len, rec);
	rec += seg_hdr.seg_len;

	status = dm0p_unfix_page(r, DM0P_UNFIX, &segPinfo, dberr);
	if (status != E_DB_OK)
	    break;
    }

    if (*segpage)
    {
	local_stat = dm0p_unfix_page(r, DM0P_UNFIX, &segPinfo, &local_dberr);
	if (local_stat != E_DB_OK)
	{
	    if (status == E_DB_OK)
	    {
		status = local_stat;
		*dberr = local_dberr;
	    }
	}
    }

    /* 
    ** If readnolock, refix 1st page into another readnolock buffer
    ** and make sure it is unchanged, otherwise this get should
    ** return E_DB_WARN, the row is inconsistent
    */
    if (data == r->rcb_1rnl_page || data == r->rcb_2rnl_page)
    {
	firstpno = DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, data);
        status = dm0p_fix_page(r, firstpno, fix_action, &segPinfo, dberr);

	if (status != E_DB_OK || (!LSN_EQ(
	    DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, data),
	    DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, *segpage))))
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	   status = E_DB_WARN;
	}
		
	if (status == E_DB_OK)
	    local_stat = dm0p_unfix_page(r, DM0P_UNFIX, &segPinfo, &local_dberr);
    }

    return (status);
}


/*{
** Name: dm1r_put_segs - Puts a record into a table when row spans pages
**
** Description:
**      Puts a record into a table when row spans pages
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      data                            Pointer to a pointer to the page
**                                      where first segment is to be inserted.
**      tid                             The TID of the record to insert.
**                                      Only set for data pages, TID for 
**                                      secondary indices is part of record.
**      record                          Pointer to the record to insert.
**      record_size                     Size in bytes of record. 
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**                                      those that can be returned by
**                                      dmop_fix_page and dm0p_unfix_page.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      10-feb-2004 (stial01)
**          Added for 256K rows, rows spanning pages.
*/
static DB_STATUS
dm1r_put_segs(
    DMP_RCB	*rcb, 
    DMP_PINFO   *pinfo,
    DM_TID      *tid, 
    char        *record, 
    char        *key,
    i4     record_size,
    i4     opflag,
    DB_ERROR	*dberr )
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DM_PAGENO		seg2_pageno = 0;
    DM_TID		seg_tid;
    DMPP_PAGE		*segpage = NULL;
    DMP_PINFO		segPinfo;
    DM_PAGENO		seg_next;
    i4			seg_len;
    i4			max;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_stat;
    i4			local_err;
    i4			rem;
    i4			orig_size = record_size;
    i4			first_seg_size;
    DMPP_PAGE		*newpage = NULL;
    DMP_PINFO		newPinfo;
    DM_TID		newtid;
    DMPP_PAGE		*putpage = NULL;
    DMP_PINFO		putPinfo;
    DMPP_SEG_HDR 	seg_hdr;
    char		*seg;
    i4			seg_num;
    i4			dm1p_flag = 0;
    bool		alloc_page;
    i4			alloc_cnt;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* More segments for this row */
    max = dm2u_maxreclen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize);

    /* Determine size of 1st segment (should be same as dm1r_allocate)*/
    rem = record_size % max;
    if (rem)
	first_seg_size = rem;
    else
	first_seg_size = max;

    /* Put segments 2-n first */
    /* Put segment 1 last */
    if (orig_size > max)
    {

	/*
	** Call allocation code to get a free page from the free maps.
	** This will possibly trigger a File Extend as well.
	** Page allocations for additional segments never need to be
	** done in a mini transaction
	*/
	for (seg_num = 2, seg = record + first_seg_size, 
			alloc_page = TRUE, alloc_cnt = 0;  ; )
	{
	    putpage = newpage;
	    newpage = NULL;
	    if (alloc_page)
	    {
		alloc_cnt++;
		switch (t->tcb_rel.relspec)
		{
		    case TCB_HEAP:
			/* Fall through */
		    default:
			status = dm1p_getfree(r, dm1p_flag, &newPinfo, dberr);
			newpage = newPinfo.page;
			/* FIX ME I don't think we need dm0p_before_image_check
			** here after dm1p_getfree- 
			** it will be done in put, before put
			*/
			break;
		}
		if (status != E_DB_OK)
		    break;
		seg_next = 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newpage);
		if (!seg2_pageno)
		    seg2_pageno = seg_next;
	    }
	    else
	    {
		newpage = NULL;
		seg_next = 0;
	    }

	    if (record + first_seg_size + (alloc_cnt * max) 
			>= record + orig_size)
	    {
		alloc_page = FALSE;
	    }

	    if (putpage)
	    {
		DMP_PINIT(&putPinfo);
		putPinfo.page = putpage;
		seg_hdr.seg_len = max;
		seg_hdr.seg_next = seg_next;
		seg_tid.tid_tid.tid_page = 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, putpage);
		seg_tid.tid_tid.tid_line = 0;
#ifdef xDEBUG
		TRdisplay("dm1r_put_segs seg %d (seghdr %d,%d) tid (%d,%d)\n", 
		seg_num, seg_hdr.seg_len, seg_hdr.seg_next,
		seg_tid.tid_tid.tid_page, seg_tid.tid_tid.tid_line);
#endif
		seg_num++;
		status = put(r, &putPinfo, &seg_tid, seg, (char *)NULL, max,
			    (i4)opflag, &seg_hdr, dberr);
		if (status != E_DB_OK)
		    break;
		status = dm0p_unfix_page(r, DM0P_UNFIX, &putPinfo, dberr);
		putpage = NULL;
		if (status != E_DB_OK)
		    break;

		seg += max;
	    }

	    if (seg_next == 0)
		break;
	}
	if (putpage)
	{
	    local_stat = dm0p_unfix_page(r, DM0P_UNFIX, &putPinfo, &local_dberr);
	    if (local_stat != E_DB_OK && status == E_DB_OK)
	    {
		status = local_stat;
		*dberr = local_dberr;
	    }
	}
	if (newpage)
	{
	    local_stat = dm0p_unfix_page(r, DM0P_UNFIX, &newPinfo, &local_dberr);
	    if (local_stat != E_DB_OK && status == E_DB_OK)
	    {
		status = local_stat;
		*dberr = local_dberr;
	    }
	}
    }

    if (status == E_DB_OK)
    {
	/* Put first seg */
	seg_hdr.seg_next = seg2_pageno;
	seg_hdr.seg_len = first_seg_size; 
#ifdef xDEBUG
	TRdisplay("dm1r_put_segs 1st seg (seghdr %d,%d) tid (%d,%d)\n", 
	    seg_hdr.seg_len, seg_hdr.seg_next,
	    tid->tid_tid.tid_page, tid->tid_tid.tid_line);
#endif
	status = put(r, pinfo, tid, record, key, first_seg_size,
		    opflag, &seg_hdr, dberr);

	/*
	** TCB_HEAP: Allocate/link a new DMPP_OVFL to data
	** if we added any disassociated data pages
	*/
	if (status == E_DB_OK && t->tcb_rel.relspec == TCB_HEAP && alloc_cnt)
	{
	    DM_TID	temptid;
	    status = dm1s_allocate(r, pinfo, &temptid, record, key, 
			max, DM1C_DUPLICATES, dberr);
	}
    }

    return (status);
}


/*{
** Name: dm1r_delete_segs - Deletes a record from a table when row spans pages.
**
** Description:
**    Deletes a record from a table when row spans pages.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      tid                             The tid of the record to delete.
**      flag                            A value indicating type of delete.
**                                      If it is DM1C_BYTID then this is
**                                      a random delete, otherwise it is
**                                      a delete on current cursor with
**                                      tid in rcb->rcb_currenttid.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM003C_BAD_TID, E_DM0044_DELETED_TID.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      10-feb-2004 (stial01)
**          Added for 256K rows, rows spanning pages.
*/
static DB_STATUS
dm1r_del_segs(
    DMP_RCB     *rcb, 
    DM_TID      *tid, 
    i4     flag,
    i4     opflag,
    DB_ERROR	*dberr )
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_SEG_HDR	seg_hdr;
    DM_TID		seg_tid;
    DMP_PINFO		segPinfo;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_stat;
    i4			row_version = 0;	
    char		*record;
    i4			record_size;
    i4			fix_action;
    i4			seg_num = 1;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    fix_action = DM0P_WRITE;

    /* More segments for this row to del */

    dm0pLockBufForUpd(r, &r->rcb_data);

    /*
    ** Make sure there is a row at the indicated tid and find its length.
    */
    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
	t->tcb_rel.relpgsize, r->rcb_data.page, tid, 
	&record_size, &record, &row_version, NULL, NULL, &seg_hdr);

    dm0pUnlockBuf(r, &r->rcb_data);
    if (status != E_DB_OK)
    {
	dmd_check(E_DMF011_DM1R_BAD_TID);
    }

#ifdef xDEBUG
    TRdisplay("dm1r_del_segs 1st seg (seghdr %d,%d) tid (%d,%d)\n", 
	seg_hdr.seg_len, seg_hdr.seg_next,
	tid->tid_tid.tid_page, tid->tid_tid.tid_line);
#endif

    /* Delete first segment */
    status = delete(r, &r->rcb_data, tid, flag, opflag, dberr); 

    /* Delete segments 2-n */
    segPinfo.page = NULL;
    for (seg_num = 2; status == E_DB_OK && seg_hdr.seg_next; seg_num++)
    {
	status = dm0p_fix_page(r, seg_hdr.seg_next, DM0P_WRITE, &segPinfo,
			dberr);
	if (status != E_DB_OK)
	    break;

	seg_tid.tid_tid.tid_page = seg_hdr.seg_next;
	seg_tid.tid_tid.tid_line = 0;

	status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
	    t->tcb_rel.relpgsize, segPinfo.page, &seg_tid, 
	    &record_size, &record, &row_version, NULL, NULL, &seg_hdr);
	if (status != E_DB_OK)
	{
	    break;
	}

#ifdef xDEBUG
	TRdisplay("dm1r_del_segs seg %d (seghdr %d,%d) tid (%d,%d)\n", 
	    seg_num, seg_hdr.seg_len, seg_hdr.seg_next,
	    seg_tid.tid_tid.tid_page, seg_tid.tid_tid.tid_line);
#endif

	/* Delete segment */
	status = delete(r, &segPinfo, &seg_tid, flag, opflag, dberr);
	if (status != E_DB_OK)
	{
	    break;
	}

	/*
	** Return page to the free list.
	** Can't do this for TCB_HEAP because allocate may
	** need to search backwards from lastpage for DMPP_OVFL page
	*/
	if (t->tcb_rel.relspec != TCB_HEAP)
	{
	    status = dm1p_putfree(r, &segPinfo, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &segPinfo, dberr);

	    if (status != E_DB_OK)
	    {
		if (status == E_DB_OK)
		    break;
	    }
	}
    }

    if (segPinfo.page)
    {
	local_stat = dm0p_unfix_page(r, DM0P_UNFIX, &segPinfo, &local_dberr);
	if (local_stat != E_DB_OK)
	{
	    /* FIX ME NEW ERROR */
	    if (status == E_DB_OK)
	    {
		status = local_stat;
		*dberr = local_dberr;
	    }
	}
    }

#ifdef xDEBUG
    if (status != E_DB_OK)
    {
	TRdisplay("dm1r_del_segs: seg %d tran %d status %d errcode %d\n",
	seg_num, r->rcb_tran_id.db_low_tran, status, dberr->err_code);
    }
#endif

    return (status);
}

static DB_STATUS
dm1r_replace_segs(
	DMP_RCB		*rcb, 
	DM_TID		*old_tid, 
	char		*old_row,
	i4		old_record_size,
	DMP_PINFO	*newdataPinfo,
	DM_TID		*new_tid, 
	char		*new_row,
	i4		new_record_size,
	i4		opflag,
	DB_ERROR	*dberr )
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_PAGE		*old_segpage = NULL;
    DMPP_SEG_HDR	old_seg_hdr;
    DM_TID		old_seg_tid;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_stat;
    i4			row_version = 0;	
    i4			*row_ver_ptr;
    char		*buf;
    char		*oldseg;
    char		*record;
    i4			record_size;
    i4			fix_action = 0;
    i4			old_seg_num = 1;
    DMPP_PAGE		*newpage;

    CLRDBERR(dberr);

    /* More segments for this row to del */
    TRdisplay("dm1r_replace_segs: Row spans pages \n");
    return (E_DB_ERROR);
}

DB_STATUS
dbg_dm723(
    DMP_RCB     *rcb,
    DM_TID      *tid,
    DM_TID	*base_tid,
    i4		dm0l_flags,
    i4		*err_code)
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = r->rcb_tcb_ptr;
    DMP_DCB		*d = t->tcb_dcb_ptr;
    LK_LOCK_KEY         lock_key;
    LK_LKB_INFO         info_buf;
    CL_ERR_DESC          sys_err;
    u_i4		info_result;
    STATUS              s = E_DB_OK;
    i4             row_lock_mode;
    i4                  flag;
    i4			reltidx;

    /* Make sure we hold the locks that DMVE routines need for rollback */
    for ( ; ; )
    {
	if (d->dcb_status & DCB_S_EXCLUSIVE)
	{
	    /* We have X database lock */
	    s = E_DB_OK;
	    break;
	}

	flag = LK_S_OWNER_GRANT;

	/* See if we have a table lock */
	lock_key.lk_type = LK_TABLE;
	lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
	lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
	lock_key.lk_key3 = 0; /* LK_TABLE lock is ALWAYS has reltidx = 0 */
	lock_key.lk_key4 = 0;
	lock_key.lk_key5 = 0;
	lock_key.lk_key6 = 0;

	/* Length of zero returned if no matching key */
	s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key, 
		    sizeof(info_buf), (PTR)&info_buf, &info_result, 
		    (u_i4 *)NULL, &sys_err);

	if (s != E_DB_OK || info_result == 0 ||
		(info_buf.lkb_grant_mode != LK_X &&
		info_buf.lkb_grant_mode != LK_IX) &&
		info_buf.lkb_grant_mode != LK_SIX)
	{
	    /* Must have X or IX table lock */
	    s = E_DB_ERROR;
	    break;
	}

	if (info_buf.lkb_grant_mode == LK_X)
	{
	    /* We have X table lock */
	    s = E_DB_OK;
	    break;
	}

	/* Don't have exclusive table lock, need a page lock too */
	lock_key.lk_type = LK_PAGE;
	lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
	lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
	lock_key.lk_key3 = t->tcb_rel.reltid.db_tab_index;
	lock_key.lk_key4 = tid->tid_tid.tid_page;
	lock_key.lk_key5 = 0;
	lock_key.lk_key6 = 0;

	/* Length of zero returned if no matching key */
	s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key, 
		    sizeof(info_buf), (PTR)&info_buf, &info_result, 
		    (u_i4 *)NULL, &sys_err);

	if (s != E_DB_OK || info_result == 0 ||
		(info_buf.lkb_grant_mode != LK_X &&
		info_buf.lkb_grant_mode != LK_IX) &&
		info_buf.lkb_grant_mode != LK_SIX)
	{
	    /* Must have X or IX page lock */
	    s = E_DB_ERROR;
	    break;
	}

	/* LKB_PHYSICAL 0x01 in lkdef.h */
	if (info_buf.lkb_grant_mode == LK_X &&
		( (info_buf.lkb_flags & 0x01) == 0 || 
		  (dm0l_flags & DM0L_PHYS_LOCK)) )
	{
	    /* We have X page lock */
	    s = E_DB_OK;
	    break;
	}

	/* Don't have X page lock, need row lock */

	reltidx = t->tcb_rel.reltid.db_tab_index;
	if (reltidx > 0)
	    reltidx = 0;			/* Base table, not index */
	if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    /* We're a global index, check ROW lock against proper partition */

	    if ( t->tcb_parent_tcb_ptr->tcb_pp_array )
		reltidx = t->tcb_parent_tcb_ptr->tcb_pp_array[r->rcb_partno].pp_tabid.db_tab_index;
	}

	lock_key.lk_type = LK_ROW;
	lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
	lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
	lock_key.lk_key3 = reltidx;
	lock_key.lk_key4 = base_tid->tid_tid.tid_page;
	lock_key.lk_key5 = base_tid->tid_tid.tid_line;
	lock_key.lk_key6 = 0;

	/* Length of zero returned if no matching key */
	s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key, 
		    sizeof(info_buf), (PTR)&info_buf, &info_result, 
		    (u_i4 *)NULL, &sys_err);

	if (s != E_DB_OK || info_result == 0 || info_buf.lkb_grant_mode != LK_X)
	{
	    s = E_DB_ERROR;
	    break;
	}

	s = E_DB_OK;
	break; /* ALWAYS */
    }

    if (s != E_DB_OK)
    {
	TRdisplay("Update lock protocol error: %~t (%d,%d)(%d,%d) iso %d lktype %d\n",
	    sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid,
	    t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
	    tid->tid_tid.tid_page, tid->tid_tid.tid_line,
	    r->rcb_iso_level, r->rcb_lk_type);

	/*  lock_key.lk_type has type of lock we failed to hold */
	uleFormat(NULL, E_UL0017_DIAGNOSTIC, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		30, "DMF update lock protocol error");

	s = E_DB_OK;
    }

    /* For now don't fail */
    s = E_DB_OK;
    return (s);
}


/*
** FIX ME: DMPP_VPT_PAGE_HAS_SEGMENTS(page_type):
**   upgradedb, auditdb, verifydb and dp changes
**   dm1r_replace -> replace segs - dm0l_row_unpack
**   grep DB_MAXTUP,DM_MAXTUP
*/


