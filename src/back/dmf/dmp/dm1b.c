/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tm.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ade.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dm.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1r.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dm2f.h>
#include    <dma.h>
#include    <dm0m.h>
#include    <dmpepcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmxe.h>
#include    <dml.h>
#include    <dmfcrypt.h>

/*
**  NO_OPTIM = dr6_us5 nc4_us5 i64_aix
*/

/**
**
**  Name: DM1B.C - Routines to access and update BTREE tables.
**
**  Description:
**      This file contains all the routines needed to access and
**      update BTREE tables. 
**
**      The routines defined in this file are:
**      dm1b_allocate       - Allocates space for putting a BTREE record.
**      dm1b_delete         - Deletes a record from a BTREE table.
**      dm1b_bybid_get      - Gets a record by BID.
**      dm1b_get            - Gets a record from a BTREE table. 
**      dm1b_put            - Puts a record to a BTREE table.
**      dm1b_rcb_update     - Updates all RCB's in transaction.
**      dm1b_replace        - Replaces a BTREE record.
**      dm1b_search         - Searches a BTREE for a given key
**                            to determine positioning information.
**      dm1badupcheck       - Checks for duplicate records.
**	dm1b_dupcheck	    - Checks for duplicate records in unique indexes.
**	dm1b_count	    - Count tuples in a BTREE.
**
**  History:
**      21-oct-85 (jennifer,ac,rogerk)
**          Changed for Jupiter.
**	22-aug-88 (rogerk)
**	    Fixed unique secondary index replace bug in dm1b_allocate.
**	24-Apr-89 (anton)
**	    Added local collation support
**	16-may-89 (rogerk)
**	    Fixed readnolock problem in dm1b_rcb_update - don't update position
**	    of rcb that points to its own readnolock copy of a page since the
**	    readnolock copy did not change.
**	29-may-89 (rogerk)
**	    Added checks for bad rows in dm1c_get calls.  Added trace flags
**	    to allow us to delete rows by tid that cannot be fetched.
**	17-aug-1989 (Derek)
**	    Added checks on the security label for B1 systems.
**	29-sep-89 (paul)
**	    Ancillary problem related to bug 8054: Fix uninitialized variable
**	    causing Btree access violation when updating records with new
**	    keys from two cursors simultaneously.
**	22-jan-90 (rogerk)
**	    Make sure we send byte-aligned tuples to qualification function.
**	28-apr-1990 (rogerk)
**	    Verify in dm1b_replace that correct page is fixed.
**	 7-jun-1990 (rogerk)
**	    Fixed duplicate checking problem in dm1b_allocate with backout.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	18-dec-90 (rogerk)
**	    Made significant changes to btree duplicate checking and space
**	    allocation.  Created new routine - dm1b_dupcheck - to do duplicate
**	    checking.  Sorted through various problems - most having to do
**	    with confusion in trying to allocate space and check duplicates
**	    at the same time.  Fixed the following bugs:
**		- verify that duplicate checking begins on the correct
**		  page so that previous duplicate rows are not missed.
**		  (this was only a problem with unique secondary indexes).
**		- check if necessary for duplicate entries on following
**		  leaf pages.
**		  (this was only a problem with unique secondary indexes).
**		- Don't return NOPART from dm1b_allocate if find an empty
**		  overflow chain fixed on entry.
**		- Don't try to ignore TIDP field on 5.0 unique secondary
**		  indexes that don't include the TIDP as part of the key.
**		- Check if leaf passed in is actually a chain page and don't
**		  forget to check for duplicates in the parent leaf.
**	14-jan-91 (rogerk)
**	    Fixed problem in above fixes where we were checking if the
**	    insert tuple was withing the HIGH/LOW range of the currently
**	    fixed leaf.
**	23-jan-1991 (bryanp)
**	    Added support for Btree Index Compression.
**	23-apr-1991 (bryanp)
**	    Fix bug 37119: error path unfixed page twice, caused dmd_check
**	 6-may-1991 (rogerk)
**	    Added fix to dm1b_allocate for bug which caused deadlock during
**	    btree abort processing.  Bypass dm1bpfindpage if DM1C_TID mode.
**	14-jun-1991 (Derek)
**	    Enhancements for allocation project.  Included some fixes from
**	    6.4 to handle bugs I was running into (see dm1b_allocate).
**	    Minor changes to a few functions, major changes to findpage() and
**	    a new function check_reclaim() that is used to deallocate
**	    empty dis-associated data pages.
**	23-jul-91 (markg)
**	    Added fix for bug 38679 to dm1b_get where an incorrect error was 
**	    being returned if there had been a failure while attempting to 
**	    qualify a tuple. 
**	29-jul-1991 (bryanp)
**	    B37911: Initialize state before calling dm1bxdupkey.
**	19-aug-91 (rogerk)
**	    Modified above fix in dm1b_get to not log internal errors when the
**	    qualify function returns an error since we cannot distiguish
**	    between user errors and internal errors. When the USER_ERROR return
**	    value from the qualification function becomes supported, we
**	    should change back to logging internal error messages.
**	20-aug-1991 (bryanp)
**	    Merged 6.4 and 6.5 codelines.
**      23-oct-1991 (jnash)
**          Remove return status code check from calls to dmd_prrecord,
**	    dmd_prindex and dmd_prkey, caught by LINT.  Also added  
**	    associated function declarations.
**	 3-nov-91 (rogerk)
**	    Added fixes for recovery on File Extend operations.
**	    Added dm0p_bicheck calls to log Before Image records if necessary
**	    for the leaf page and old data page during an ASSOC operation.
**	    These records are needed for UNDO and DUMP processing.
**	10-feb-92 (jnash)
**	    Fix check in dm1b_delete for previously deleted tid, update
**	    dm1b_delete function header.
**	07-jul-92 (jrb)
**	    Prototype DMF.
**	05_jun_1992 (kwatts)
**	    MPF change, calls to dm1c_xxx now uses accessor.
**	16-sep-1992 (bryanp)
**	    Make DM609 format leaf page at end of search.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project:
**	      - Changed to use new recovery protocols.  Before Images are
**		no longer written and the pages updated by the allocate must
**		be mutexed while the dm0l_assoc log record is written and
**		the pages are updated.
**	      - Changed to pass new arguments to dm0l_assoc.
**	      - Add new param's to dmpp_allocate call.
**	      - Remove unused param's on dmpp_get calls.
**	      - Move compression out of dm1c layer, calling
**		tlv's here as required.  Removed some unneeded MEcopy's.
**	      - Use dmpp_tuplecount() instead of dmpp_isempty().
**	      - Changed arguments to dm0p_mutex and dm0p_unmutex calls.
**	      - Removed calls to dmd_prrecord when records are compressed.
**	      - Rewrote dm1b_delete, dm1b_put, dm1b_replace routines.  Took
**		out direct use of dmpp_put, dmpp_del routines.  Instead dm1r
**		routines are now called.  Pages are no longer mutexed in here,
**		but are mutexed and logged by dm1r and dm1bx routines.
**	      - Removed sreplace routine.  Consolidated base table and 2nd
**		index updates in dm1r_replace and dm1r_put.
**	      - Move dm1b.h to be included before dm0l.h
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Changed name of 'dupflag' parameter in dm1b_allocate to 
**		'opflag' since it was controlling more than duplicate checking.
**	      - Changed DM1C_TID option in dm1b_allocate to DM1C_REALLOCATE 
**		since the name TID gave no clue at all as to what it meant.
**	      - Changed dm1b_allocate to never do SPLITs while running in 
**		REALLOCATE mode, which is used during recovery.
**	      - Changed dm1bxsearch to not traverse leaf's overflow chain when
**		called in DM1C_TID mode.  That logic was moved to dm1b_search.
**		Now when calling dm1bxsearch in DM1C_TID mode we check for the
**		NOPART return status and go to the next overflow page ourselves
**		and do a dm1bxsearch on it.
**	      - Changed dm1bxsearch to take buffer pointer by reference rather 
**		than by value since it is no longer ever changed.
**	18-dec-92 (robf)
**	    Removed obsolete dm0a.h
**	28-dec-92 (mikem)
**	    Fixed bugs in dm1b_{put,delete}() introduced by above change.  Now 
**	    that these routines call dm1r_{put,delete}() to do the work required
**	    they no longer have to modify tcb_tup_adds itself.  The result was
**	    that the count as returned by dmt_show to be twice the number of 
**	    inserts/deletes.
**	02-jan-1993 (jnash)
**	    More reduced logging work.  Add LGreserve call.
**	18-jan-93 (rogerk)
**	    Added back updates to tcb_tup_adds for secondary index updates.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	26-may-1993 (robf)
**	    Secure 2.0:
**	        Remove ORANGE code.
**		Add new generic calls to dma_row_access() to do security access
**	21-jun-1993 (rogerk)
**	    Changed dm1b_rcb_update to give an error if called with an invalid
**	    opflag type rather than returning OK without doing anything.
**	    Fixed dm1b_find_ovfl_space to not reference unfixed overflow page.
**	    Added new comments and error messages.
**       1-Jul-1993 (fred)
**          Added dm1c_pget() function where required to allow for the
**          rcb_f_qual function's being run on large objects.  This
**          change applies to dm1r_bget().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	  - Added new dm1b_rcb_update mode - DM1C_MDEALLOC - used when an 
**	    overflow page is deallocated.
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Fixed bug in usage of newduplicate routine.  Leaf argument must
**	    be passed by reference since the leaf page can be temporarily
**	    unfixed by routines that newduplicate calls.  This was causing
**	    dmd_checks when a page was temporarily unfixed and concurrent
**	    access caused the page to be tossed.
**      13-sep-93 (smc)
**          Commented lint pointer truncation warning.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (kwatts) BUG #55844
**	    Simple coding error in dm1b_delete().
**	10-nov-1993 (pearl)
**	    Bug 54961.  When the search mode is DM1C_RANGE in dm1b_search(),
**	    change the algorithm such that if the key is not found on the 
**	    on an non-empty leaf page, and there is a next leaf
**	    page, then fix the next page.  If the leaf page is empty, and there
**	    are overflow pages, then go down the overflow chain until we find
**	    a key.  We can then determine whether the key is within the given
**	    range, and fix the next page if it is not.  If we are already at
**	    the rightmost leaf page, and the keys are not in range, then we
**	    have to position at the end of the overflow chain if there is one.
**      30-aug-1994 (cohmi01)
**          Added prev() function to read a btree backwards, called from
**          dm1b_get() just like next(). Added for FASTPATH release.
**	10-apr-1995 (rudtr01)
**	    Bug #52462 - dm1b_dupcheck() sometimes trashes the bid passed
**	    by the caller when searching the B-tree for duplicates.
**      10-apr-1995 (chech02) Bug# 64830 (merged the fixes from 6.4 by nick)
**          Unfix the page pointed to by leaf to avoid overwriting
**          this pointer when fixing the result page. #64830.
**      19-apr-1995 (stial01) Modified dm1b_search() to support select last
**          record from btree table (MANMANX).
**	18-may-1995 (cohmi01)
**	    Fix problem with prev() in which other_page_ptr got unfixed.
**	    Adjust prev() and select-last in dm1b_search to read duplicates
**	    in reverse order based on sequence in overflow chain.
**	22-may-1995 (cohmi01)
**	    Added dm1b_count() for agg optimization project.
**	24-may-1995 (wolf)
**	    Re-write the call to next() or prev() in dm1b_get to avoid
**	    compiler errors on VMS.
**	19-jun-95 (lewda02)
**	    No need to keep current leaf fixed while searching for prev
**	    leaf in prev() code.
**	11-aug-1995 (shust01)
**	    For DM1C_LAST, set pos to leafpage->bt_kids instead of DM_TIDEOF.
**	    DM_TIDEOF was causing problems when doing a NEXT after a LAST.
**	    This is in dm1b_search().
**	11-aug-95 (shust01)
**	    If DM_TIDEOF is set when we first enter prev(), it means
**	    we are beginning of table.  Since we are doing prev(), we can
**	    assume that we are going to go beyond the beginning of table,
**	    so just return an error.
**	30-jul-1995 (cohmi01)
**	    If prefetch is available, schedule prefetch of data pages pointed 
**	    to by new leaf. 1st data page will be skipped to provide overlap.
**	    Added dm1b_build_dplist(), called by dm2p_schedule_prefetch().
**	05-sep-1995 (cohmi01)
**	    Support rcb_f_qual for GETPREV. 
**	    Add dummy parms to dm1b_count() for compatibility with dm2r_get().
**	03-oct-1995 (cohmi01)
**	    Remove call to dm1cxhas_room() that was there to 'make sure we
**	    really have room on the leaf' before finding room on data page.
**	    This check was redundant (with initial call in this func and with
**	    call in dm1bxinsert) and caused false 'RSHIFT' split errors as
**	    it used the max size of key rather than the compressed size.
**	24-jan-96 (pchang)
**	    Fixed bug 73672 - Duplicate rows were being inserted into btree 
**	    tables with unique index.  In dm1b_dupcheck(), if we are positioned
**	    to the start of a new leaf when checking for duplicates in a unique
**	    index, and if we then have to go back to the previous leaf page in
**	    order to check the key value on its last entry for duplicates, the
**	    bid and leaf pointers will have changed after the search using the
**	    partial key (i.e. excluding the tidp) and must not be referenced
**	    as if they were the same pointers we had been given.
**	 5-mar-1996 (nick)
**	    If fetching by tid on base relation in dm1b_get(), OR 
**	    in DM0P_USER_TID. #57126
**	30-apr-1996 (kch)
**	    In the function dm1b_rcb_update(), if the current RCB deletes a
**	    record we now perform the leaf page line table compression for all
**	    other RCBs after the test for and marking of a fetched tid as
**	    deleted. This change fixes bug 74970.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	    Pass page_size argument to dmpp_allocate accessor.
**	    Pass page_size argument to dmpp_tuplecount accessor.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-mar-1996 (stial01)
**          Call dm1bxinsert with page_size parameter.
**	06-may-1996 (thaju02)
**	    New Page Format Project. Modify page header references to macros.
**	29-may-96 (nick)
**	    findpage() was mixing up location information when generating the
**	    values for the dm0l_assoc() call.
**      06-jun-1996 (prida01)
**          We need to check that btree's don't update rows ambiguously when
**          the cursor mode is deferred i.e. more than once per transaction.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**	 1-jul-96 (nick)
**	    dm1b_count() would leave a bogus page pointer lying around
**	    which would cause unfix errors later on. #76111
**	13-Aug-1996 (prida01)
**	    Add cast to i4 for calls to dm1c_p{*}
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**	25-sep-96 (cohmi01)
**	    In prev() save LRANGE key in local buffer. #78751.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added handling of deleted leaf entries (status == E_DB_WARN)
**          Added arguments to dm1b_rcb_update()
**          Do not reclaim space from delete if row locking
**          Call dm1bxclean() to reclaim space on leaf pages
**          dm1b_bybid_get(), dm1b_get() Reposition if necessary
**          prev(), next() Skip deleted leaf entries.
**          dm1b_replace() Decrement insert_position only if reclaiming space 
**          find_page() Call dm1r_allocate() instead of dmpp_allocate
**          find_page() If row locking, alloc assoc data page in mini trans.
**          Added dm1b_isnew() for RCB_U_DEFERRED
**          Added dm1b_reposition(), dm1b_position(), dm1b_save_bt_position()
**          If phantom protection is needed, set DM0P_PHANPRO fix action.
**      25-nov-96 (stial01)
**          dm1b_rcb_update() fixed args to dm1b_update_lsn()
**          dm1b_reposition() If rcb_repos_clean_count == DM1B_CC_INVALID, 
**          reposition to STARTING position.
**          dm1b_save_bt_position() Init clean_count for CURRENT position
**      12-dec-96 (dilma04, stial01)
**          dm1b_get: get data record after getting row lock;
**          dm1b_bybid_get: set DM0P_PHANPRO fix action if phantom 
**          protection is needed.
**      17-dec-96 (stial01)
**          passive_rcb_update() DM1C_MSPLIT: Set rcb_status |= RCB_FETCH_REQ 
**          Also added bid paramater,deleted unused lsn,clean count parameters
**          dm1b_bybid_get() Adjust local leaf pointers after reposition
**          dm1b_reposition() Reposition to next page, 'overflow_page' not init
**      06-jan-97 (dilma04)
**          Removed unneeded DM1P_PHYSICAL flag when calling dm1p_getfree()
**          from findpage().
**      22-jan-97 (dilma04)
**          - Fix bug 79961: release shared row locks if doing get by tid
**          and isolation level is read committed;
**          - change DM1C_BYTID_SI_UPD to DM1C_GET_SI_UPD to be consistent with
**          dm1r_get().
**      27-jan-97 (dilma04)
**          Do not set phantom protection flag if searching to update 
**          secondary index.    
**      04-feb-97 (stial01)
**          dm1b_delete(), dm1b_replace():
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - test rcb_iso_level for isolation level;
**          - use macro to check for serializable transaction.
**      26-feb-97 (dilma04)
**          Change dm1b_get() to support lock escalation for row locking
**          and lower isolation levels.
**      27-feb-97 (stial01)
**          Moved some common code to dm1b_compare_range()
**          dm1b_dupcheck() Removed check for deleted rows when duplicate 
**          checking on adjacent leaf(s).
**          dm1b_delete(), dm1b_replace() Space reclaim decision is made
**          by dm1bxdelete()
**          dm1b_count() Call dm1cxkeycount() if leaf needs cleaning.
**          Fix leaf page if it is not fixed on entry.
**      10-mar-97 (stial01)
**          dm1b_allocate() pass record to findpage()
**          dm1badupcheck() Pass relwid to dm0m_tballoc()
**          find_page() Added record param, pass to dm1r_allocate()
**      02-apr-97 (stial01)
**          dm1b_allocate()Put back dm1cxhas_room Consistency check after split
**          Added code to add overflow page or split again if the first
**          split did not make sufficient space.
**      07-apr-97 (stial01)
**          dm1b_allocate() NonUnique primary btree (V2), always split
**          dm1b_search() NonUnique primary btree (V2), dups can span leaf
**          dm1badupcheck() Additional key parameter to dm1bxchain
**          dm1b_reposition() NonUnique primary btree (V2), dups can span leaf
**          dm1b_compare_range() NonUnique primary btree (V2) dups can span leaf
**          Added dm1b_compare_key()
**      21-apr-97 (stial01)
**          dm1b_allocate() If row locking, reserve space for new row
**              If positioned at duplicate deleted key, reuse the entry
**          dm1b_dupcheck() added requalify_leaf paramater
**          dm1b_bybid_get() If page locking we must clean the page before
**              we adjust the bid.
**          dm1b_delete() Copy page lsn before we possibly dealloc ovfl page
**          dm1b_get() For primary btree, skip 'reserved' leaf entries
**          dm1b_replace() Copy page lsn before we possibly dealloc ovfl page
**          btree_search() Created from dm1b_search, now dm1b_search is
**              external interface for btree search.
**          dm1b_find_ovfl_space: Remove call to dm1bxclean, since we don't 
**              have overflow pages for non unique primary btree (V2) anymore.
**          dm1b_reposition: Search mode is DM1C_FIND for unique keys, DM1C_TID 
**              for non-unique keys, since we re-use unique entries in 
**              dm1b_allocate, the tidp part of the leaf entry may not match.
**          dm1b_requalify_leaf() created from code in dm1b_search
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - if row lock can be released before the end of transaction, set
**          rcb_release_row flag in dm1b_get();
**          - removed checks for new_lock before calling dm1r_unlock_row();
**          - added support for releasing locks by lock id and for releasing
**            exclusive locks.
**      21-may-97 (stial01)
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          dm1b_find_ovfl_spacel() not called for Non unique primary btree (V2)
**          newduplicate() no longer called for Non unique primary btree (V2)
**       06-jun-97 (stial01)
**          dm1b_get: BYTID base table: Get BYTID does not change btree leaf
**          position, so don't invalidate the position when row locking
**          by invalidating fet_clean_count.  QEF sometimes does positioned get,
**          followed by get BYTID, followed by positioned delete. 
**          The get BYTID should not affect the positioned get/delete.
**          (This happened from qeq_pdelete when deleting from referencing table
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**          next() ifdef DEV_TEST: validate attribute info on leaf
**      16-jun-97 (stial01)
**          btree_search() Unlock buffer before unfixing
**          btree_position() clarify search conditions possible
**      29-jul-97 (stial01)
**          More comments to rcb_update, created passive/active_rcb_update
**          Passive rcb update based on page size not lock mode for consistency.
**          Distinguish row_locking/set_row_locking.
**          Pass original operation mode (PUT/DEL/REPL) to dm1r* for logging.
**      20-aug-97 (stial01)
**          Reposition error handling
**      29-aug-97 (stial01)
**          dm1b_replace() Fixed condition for TRdisplay
**	24-jun-1997 (walro03)
**	    Unoptimize for ICL DRS 6000 (dr6_us5).  Compiler error: expression
**	    causes compiler loop: try simplifying.
**      29-sep-1997 (stial01)
**          dm1b_get() Lock data page/row before fixing data page.
**          dm1b_get() Don't issue E_DM9C2A_NOROWLOCK if row LK_CONVERT fails
**          btree_position() bug fix for B86295
**          btree_position() bug fix: dm2r_get->dm1b_get->next->E_DM002A
**      21-oct-1997 (stial01)
**          dm1b_allocate() B86589: Use leafpage ptr NOT leaf ptr ptr.
**      12-nov-97 (stial01)
**          Remove 29-sep-97: 'Lock data page/row before fixing data page'
**          The page was locked again when it was fixed. If isolation level
**          is read committed the page is locked twice and unlocked once.
**          Instead, unlock previous row, unfix previous data page
**          and fix data page NOWAIT. If the lock is busy, unmutex the leaf
**          page, wait for the data page, reposition.
**          Consolidated rcb fields into rcb_csrr_flags.
**      15-jan-98 (stial01)
**          dm1b_allocate() check status of dm1bxclean()
**      17-mar-98 (stial01)
**          dm1b_get() Copy rec to caller buf BEFORE calling dm1c_pget (B86862)
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**          Extend table in mini transaction if extension table (B90677)
**      15-jul-98 (stial01)
**          dm1b_position() Init clean count for current position
**      07-jul-98 (stial01)
**          B84213: Defer row lock request for committed data
**	11-Aug-1998 (jenjo02)
**	    Count secondary index adds/deletes in RCB, not TCB.
**      12-aug-98 (stial01)
**          next() ROW_ACC_KEY if checking high key only (not low key)
**          findpage() If leaf buffer is mutexed, Fix data page NOWAIT 
**          findpage() set DM1P_PHYSICAL if physical locking (blob extensions)
**          btree_position() Always reposition if trace point DM719
**      20-Jul-98 (wanya01)
**          dm1b_get() Copy rec to called buf BEFORE calling dm1c_pget.
**          This is addtional changes for b86862. The previous changes was
**          only made for routine when retrieving record by sequence, not
**          when retrieving record by TID.
**      19-Jan-99 (wanya01)
**          X-integrate change 431605 (Bug 76748)
**          The previous change made by nick on 29-may-96 was lost in oping20.
**          Recovery of a Btree association record when the relation resides on
**          multiple locations could fail.
**      13-Nov-98 (stial01)
**          Renamed DM1C_SI_UPD->DM1C_GET_SI_UPD, 
**          Renamed DM1C_SI_UPDATE->DM1C_FIND_SI_UPD
**      16-nov-98 (stial01)
**          Check for btree primary key projection
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      09-feb-1999 (stial01)
**          dm1b_kperpage() relcmptlvl param added for backwards compatability
**      12-apr-1999 (stial01)
**          Btree secondary index: non-key fields only on leaf pages
**          Different att/key info for LEAF vs. INDEX pages
**      15-apr-1999 (stial01)
**          Leaf cleaning only before allocate.
**          btree_reposition() Must find exact key,tidp if DM1C_BYTID specified,
**          or if repositioning on non-unique primary btree
**      05-may-1999 (nanpr01,stial01)
**          More concurrency if serializable with equal predicate:
**          Key value lock during position replaces DM0P_PHANPRO (phantom
**          protection, page lock) when fixing leaf
**          Key value locks acquired for duplicate checking are no longer held 
**          for duration of transaction. While duplicate checking, request/wait
**          for row lock if uncommitted duplicate key is found.
**      28-may-1999 (stial01)
**          Only call dm1r_check_lock if DMZ_SES_MACRO(33)
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	23-Sep-99 (thaju02)
**	    With read committed isolation level btree data page was
**	    being unlocked prior to being unfixed, resulting in dbms
**	    server crashing with E_DM9C01, E_DM9301. 
**	    Modified dm1b_get(). (b98766)
**      02-Nov-99 (stial01)
**          X-integrate change 438768.
**          Renamed DM1C_SI_UPD->DM1C_GET_SI_UPD, 
**          Renamed DM1C_SI_UPDATE->DM1C_FIND_SI_UPD
**	13-jul-00 (hayke02)
**	    Added NO_OPTIM for NCR (nc4_us5). This change fixes bug 102080.
**	 3-oct-2000 (hayke02)
**	    We now only call newduplicate() for non-unique base tables and
**	    pre-release 6 secondary indices (i.e. key does not contain tidp).
**	    Calls to adt_kkcmp() from newduplicate() now use t->tcb_keys
**	    and t->tcb_key_atts for secondary indices rather than 
**	    t->tcb_data_atts and t->tcb_rel.relatts. This change fixes bug
**	    102686.
**	12-Jan-2001 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**      04-aug-1999 (horda03)
**          A simple select statement on a table can fail with an E_DM0047
**          Ambiguous replace error. This was because the rcb_access_mode
**          was not taken into consideration. Bug 83046.
**      20-oct-99 (stial01)
**          dm1b_get() unlock buffer before unfixing,
**          Removed unecessary dm1r_rowlk_access before dm1c_pget
**          btree_position() If starting leaf unchanged restore p_lowtid
**          dm1r_rowlk_access() Reposition to last row locked
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
**	21-oct-1999 (nanpr01)
**	    More CPU Optimization to minimize tuple header copy.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	21-nov-1999 (nanpr01)
**	    Code optimization/reorganization based on quantify profiler output.
**	05-dec-1999 (nanpr01)
**	    Code optimization to reduce the tuple header copy overhead.
**      22-nov-1999 (stial01)
**          dm1b_rowlk_access() Reposition after lkwait if data page changed
**      24-nov-1999 (stial01)
**          Fixed klen for MEcopy (should be leaf klen, not index klen)
**      21-dec-1999 (stial01)
**          check_reclaim() for etabs > 2k
**      27-jan-2000 (stial01)
**          btree reposition changes (rcb_repos*)
**      31-jan-2000 (stial01)
**          To enforce phantom protection we need row lock for non-key quals.
**          (B100260).
**      09-feb-2000 (stial01)
**          Restore some code deleted on 31-jan-2000
**      02-may-2000 (stial01)
**          prev() Check for DM1C_RAAT flag 
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_assoc.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Pass crecord to dm1b_allocate, findpage gets crecord if compression
**          Pass rcb to allocate,isnew page accessors, 
**      28-nov-2000 (stial01)
**          check_reclaim() update etab data page reclamation (phys page locks)
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-aug-2001 (hayke02)
**	    (hanje04) X-INTEG of 451917 from main. Now obcelete.
**	    We now only assign relversion if t->tcb_rel.relversion is non-
**	    zero. This change fixes bug 105210.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_row_access() if neither
**	    C2 nor B1SECURE.
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Types SIR 102955)
**          dm1b_isnew -> dm1cx_isnew
**	06-Feb-2001 (jenjo02)
**	    Avoid doubly copying record from page to caller's buffer
**	    when dm1b_get-ting a table with extensions by tid.
**	02-may-2001 (kinte01)
**	    Update parameter buf_locked to be a bool to match findpage 
**	    prototype - corrects compiler warnings on VMS
**      10-may-2001 (stial01)
**          Added rcb_*_nextleaf to optimize btree reposition, btree statistics
**	19-may-2001 (somsa01)
**	    Fixed errors due to previous cross-integrations.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      09-jan-2002 (stial01)
**          active_rcb_update() Adjust rcb_p_lowtid (b106419)
**      25-mar-2002 (stial01 for huazh01)
**          dm1b_allocate() Do not always requalify leaf after dup checking  
**          A Nonunique primary btree having relpgtype DM_COMPAT_PGTYPE,
**          may have leaf overflow pages, and row locking is not supported.
**          The correct leaf has already been determined (b106984).
**      02-oct-2002 (stial01)
**          next() compare rcb_hl_ptr to RRANGE entry if empty leaf (b108852)
**      24-apr-2003 (stial01)
**          dm1b_get() support DMF projection of subset of columns. (b110061)
**      28-may-2003 (stial01)
**          ifdef some TRdisplays.
**      18-jun-2003 (stial01)
**          prev() clear rcb_other_page_ptr before unfixing leafpage (b109965)
**      10-jul-2003 (stial01)
**          Fixed row lock error handling during allocate (b110521)
**      04-aug-2003 (stial01)
**          dm1b_get() DM1C_BYTID, primary btree: set rcb_currenttid (b110637)
**      15-sep-2003 (stial01)
**          Skip allocate mini trans for etabs if table locking (b110923)
**      02-jan-2004 (stial01)
**          Added bulkpage(), dm1b_bulk_put() routines to support bulk put
**          blob segments onto btree disassoc data pages
**      14-jan-2004 (stial01)
**          Cleaned up 02-jan-2004 changes.
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      19-mar-2004 (stial01)
**          dm1b_bulk_put() Validate correct leaf for each segment.
**      13-may-2004 (stial01)
**          Remove unecessary code for NOBLOBLOGGING redo recovery.
**      25-jun-2004 (stial01)
**          Modularize get processing for compressed, altered, or segmented rows
**	11-aug-2004 (thaju02)
**	    Rollforward of online modify of btree, sort phase position may
**	    have to halt if pg lsn is not at rnl lsn. Do not report 
**	    E_DM9CB1_RNL_LSN_MISMATCH in btree_search(). (B112790)
**	02-dec-2004 (thaju02)
**	    Add dm1b_online_get function prototype. 
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**      10-jan-2005 (stial01)
**          Fixed lock failure error handling after calls to dm1c_get. (b113716)
**      11-jan-2005 (horda03) Bug 111437/INGSRV2630
**          Memory corruption occurs when copying coupons into result record
**          when the results record is a projected key column. This is because
**          coupons or not part of the project key column, so no space has been
**          reserved in the record for them.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      31-jan-2005 (thaju02)
**	    In dm1b_online_get:
**          Reset bid tid_line to DM_TIDEOF if assoc data pg already
**          read and moving onto the next leaf/overflow.
**          After reading leaf's assoc pg, set bit in btree map.
**    25-Oct-2005 (hanje04)
**        Add prototype for bulkpage() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	13-Apr-206 (jenjo02)
**	    Support CLUSTERED primary tables, in which the leaf entry
**	    is the row and which have no data pages.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**      29-sep-2006 (stial01)
**          Support multiple RANGE formats: OLD format:leafkey,NEW format:ixkey
**	22-Feb-2008 (kschendel) SIR 122739
**	    Changes throughout for new rowaccessor scheme.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1r_?, dm1b_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_?, dmf_adf_error functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dm1c_?, dm0p_? functions converted to DB_ERROR *
**      04-aug-2009 (stial01)
**          dm1b_online_get() unfix page withoug losing err_code (b122396)
**	26-Aug-2009 (kschendel) 121804
**	    Need dmxe.h to satisfy gcc 4.3.
**      10-sep-2009 (stial01)
**          DM1B_DUPS_ON_OVFL_MACRO true if btree can have overflow pages
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types, get rid of local page ptrs.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**      04-dec-2009 (stial01)
**          Call isolate_dup() before adding first overflow page.
**          Added dm1b_dbg_incr_decr() for use with trace point
**          findpage() Fix reposition code
**          dm1b_rowlk_access() added TRdisplays if trace point 
**      11-jan-2010 (stial01)
**          Use macros for setting and checking position information in rcb
**	15-Jan-2010 (jonj, stial01)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    New macros to lock/unlock pages, LG_LRI replaces LG_LSN,
**	    sensitized to crow_locking().
**      18-Feb-2010 (stial01)
**          btree_reposition() more diagnostics for E_DM93F6_BAD_REPOSITION
**          dm1b_position_save_fcn() added consistency checking for RCB_P_GET
**          dm1b_get() get row lock if NeedCrowLock
**	23-Feb-2010 (stial01)
**          btree_reposition() More diagnostics for E_DM93F6_BAD_REPOSITION
**          dm1b_get() crow_locking and constraint/Upd-cursor save cursor
**          position after locking the row
**          dm1b_rowlk_access() fixed lock_status checking
**	01-apr-2010 (toumi01) SIR 122403
**	    Add decryption calls.
**	15-Apr-2010 (stial01)
**          reposition for RCB_P_FETCH, should not reposition to deleted entry
**	10-May-2010 (stial01)
**          Init DMP_PINFO with DMP_PINIT, minor changes to TRdisplays
**	09-Jun-2010 (stial01)
**          TRdisplay more info when there is an error
**	13-Aug-2010 (kschendel) b124255
**	    Add a wee bit to some of the trace output, e.g. RCB address,
**	    makes it easier to watch things during stress tests.
**	16-Aug-2010 (jonj) SD 146221
**	    dm1r_crow_lock(flags) now DM1R_LK_? instead of LK_? to avoid
**	    collisions with lk.h defines.
**	31-Aug-2010 (miket) SIR 122403
**	    For dm1e_aes calls follow dmf convention of breaking or
**	    continuing with status test rather than returning on error.
**      01-Sep-2010 (stial01) SD 146583, B124340
**          Fixed redo_dupcheck handling.
*/


/*
**  Forward and/or External function references.
*/
static	DB_STATUS next(
	DMP_RCB            *rcb,
	u_i4		   *low_tran_ptr,
	u_i2		   *lg_id_ptr,
	i4             opflag,
	DB_ERROR           *dberr);

static  DB_STATUS prev(
      	DMP_RCB            *rcb,
	u_i4		   *low_tran_ptr,
	u_i2		   *lg_id_ptr,
	i4             opflag,
	DB_ERROR           *dberr);

static DB_STATUS findpage(
	DMP_RCB		*rcb,
	DMP_PINFO       *leafPinfo,
	DMP_PINFO       *dataPinfo,
	char            *record, /* if compression, this is compressed record */
	i4         need,
	char            *key,
	DM_TID		*tid,
	DM_TID          *bid,
	DB_ERROR           *dberr);

static DB_STATUS check_reclaim(
	DMP_RCB	    *rcb,
	DMP_PINFO   *dataPinfo,
	DB_ERROR           *dberr);

static DB_STATUS newduplicate(
	DMP_RCB		*rcb,
	DMP_PINFO	*leafPinfo,
	char		*key,
	i4		*duplicate_insert,
	DB_ERROR           *dberr);
/*
static DB_STATUS dm1b_dupcheck(
	DMP_RCB	*rcb,
	DMPP_PAGE   **leaf,
	DM_TID	*bid,
	char        *record,
	char        *leafkey,
	bool        *requalify_leaf,
	bool        *buf_locked,
	bool        *redo_dupcheck,
	DB_ERROR           *dberr);
*/
static DB_STATUS dm1badupcheck(
	DMP_RCB		*rcb,
	DMP_PINFO	*leafPinfo,
	DM_TID		*bid,
	DM_TID		*tid,
	char		*record,
	char		*leafkey,
	bool            *requalify_leaf,
	bool            *redo_dupcheck,
	DB_ERROR           *dberr);

static  DB_STATUS dm1b_find_ovflo_pg(
    	DMP_RCB     	*r,
    	i4		fix_action, 
    	DMP_PINFO	*currentPinfo, 
    	i4		whatpage,
    	DM_PAGENO	startpage, 
	DB_ERROR           *dberr);

static DB_STATUS btree_reposition(
	DMP_RCB     *rcb,
	DMP_PINFO   *leafPinfo,
	DM_TID      *bid,
	char        *key_ptr,
	i4	    pop,
	i4	    opflag,
	DB_ERROR           *dberr);

static DB_STATUS passive_rcb_update(
	DMP_RCB        *rcb,
	i4        opflag,
	DM_TID         *bid,
	DM_TID         *tid,
	DM_TID         *newtid,
	char           *newkey,
	DB_ERROR           *dberr);

static DB_STATUS active_rcb_update(
	DMP_RCB        *rcb,
	DM_TID         *bid_input,
	DM_TID         *newbid_input,
	i4        split_pos,
	i4        opflag,
	i4        del_reclaim_space,
	DM_TID         *tid,
	DM_TID         *newtid,
	char           *newkey,
	DB_ERROR           *dberr);

static DB_STATUS btree_search(
    DMP_RCB            *rcb,
    char               *key,
    i4            keys_given,
    i4            mode,
    i4            direction,
    DMP_PINFO          *leafPinfo,
    DM_TID             *bid,
    DM_TID             *tid,
    char	       *record,
    DB_ERROR           *dberr);

static DB_STATUS btree_position(
    DMP_RCB        *rcb,
    DB_ERROR           *dberr);

static DB_STATUS dm1b_requalify_leaf(
    DMP_RCB     *rcb,
    i4     mode,     
    DMP_PINFO   *leafPinfo,
    char        *key,
    i4     keys_given,
    DB_ERROR           *dberr);

static DB_STATUS dm1b_rowlk_access(
    DMP_RCB        *rcb,
    i4        get_status,
    i4        access,
    u_i4	    *row_low_tran,
    u_i2	    *row_lg_id,
    DM_TID         *tid_to_lock,
    i4		   partno_to_lock,
    LK_LOCK_KEY    *save_lock_key,
    i4        opflag,
    DB_ERROR           *dberr);

static DB_STATUS dm1b_online_get(
    DMP_RCB             *rcb,
    DM_TID              *rettid,
    char                *record,
    i4                  opflag,
    DB_ERROR           *dberr);

static DB_STATUS bulkpage(
    DMP_RCB	    *rcb,
    DMP_PINFO       *leafPinfo,
    DMP_PINFO       *dataPinfo,
    char            *record,
    i4         need,
    char            *key,
    DM_TID	    *tid,
    DM_TID          *bid,
    DB_ERROR           *dberr);

static DB_STATUS
VerifyHintTidAndKey(
    DMP_RCB     *rcb,
    DM_TID      *HintTid,
    char        *ClusterKey,
    char	*record,
    DM_TID      *TrueTid,
    DB_ERROR           *dberr);

static DB_STATUS
findpage_reposition(
    DMP_RCB     *rcb,
    DMP_PINFO   *leafPinfo,
    char        *key_ptr,
    DB_ERROR    *dberr);

static DB_STATUS
isolate_dup(
DMP_RCB		*rcb,
DMP_PINFO	*leafPinfo,
char		*dupkey,
DB_ERROR	*dberr);

static DB_STATUS
dm1b_dbg_incr_decr(
DMP_RCB		*rcb,
char		*key);




/* defines used by static  dm1b_find_ovflo_pg ()    		*/
#define	PREV_OVFL 	1	/* find previous page in ovflo chain 	    */
#define	LAST_OVFL	2	/* find last page in ovflo chain (pts to 0) */

/*{
** Name: dm1b_allocate - Allocates space for insertion into a BTREE table.
**
** Description:
**      This routine allocates space for the insertion of a record into
**      a BTREE table.  It searches for the appropriate leaf page 
**      and/or  checks for duplicate keys.  If the leaf page is full, 
**      it releases locks and searches  again, splitting
**      full pages on the way down.
** 
**	This routine will attempt to use any leaf page left in the
**	rcb. If a leaf page is found that is usable the leaf is
**	return along with the same data page or a new data page if
**	one is needed.  If no leaf and data page are available, leaf
**	will be fixed on return.  This should speed up 
**      puts where consecutive records will reside on the same
**      page.
**
**      Note, the tuple cannot be allocated here, since the
**      page before image must be logged before any changes
**      to the page.
**
**	Duplicate checking is performed unless the DM1C_NODUPLICATES flag
**	is passed in.  This flag should be specified if the table does not
**	allow duplicate rows or if the table is keyed unique - unless the
**	caller has already guaranteed that the row satisfies uniqueness
**	constraints.  The DM1C_DUPLICATES flag should be specified when:
**
**	    - the table allows duplicates and is not keyed unique - no
**	      duplicate constraints.
**	    - uniqueness has already been guaranteed.
**	    - the client is performing backout recovery - no duplicate
**	      checking is required since the database is being recovered to
**	      consistent state.  Checking for duplicates can cause errors
**	      since rows may be duplicated for short periods of time during
**	      logical undo - but when recovery is done, the table will be
**	      restored to a consistent point.
**
**	When called with the DM1C_ONLYDUPCHECK flag, no space will be allocated,
**	but duplicate checking will be performed.  This mode is normally
**	called on secondary indexes before performing the base table update
**	to check if the update will violate any uniqueness constraints.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      record                          Pointer to record to insert.
**      crecord				Compressed record (if compression)
**      need                            Value indicating amount of space 
**                                      needed, assumes compressed 
**                                      if neccessary.
**      opflag				Flag Options:
**					DM1C_DUPLICATES - duplicates allowed
**					    or tuple already guaranteed unique.
**					DM1C_NODUPLICATES - duplicates must
**					    be checked.
**					DM1C_ONLYDUPCHECK - check for dups
**					    but don't allocate space.
**					DM1C_REALLOCATE - used only during UNDO
**					    recovery actions.  It specifies to
**					    allocate space on the leaf but to 
**					    ignore the data pages.  No splits
**					    are ever done and duplicate checking
**					    is bypassed.
**
** Outputs:
**      leaf                            Pointer to an area used to return
**                                      pointer to leaf page.
**      data                            Pointer to an area used to return
**                                      pointer to data page.
**      bid                             Pointer to an area to return 
**                                      bid indicating where to insert
**                                      index record.
**      tid                             NOT RETURNED.
**      key                             Pointer to an area to return key.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**                                      E_DM0045_DUPLICATE_KEY.
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
**          Fixes and locks leaf  and data pages.
**
** History:
**      26-nov-85 (jennifer)
**          Converted for Jupiter.
**	22-aug-88 (rogerk)
**	    Took out check for duplicate keys on secondary indexes when 
**	    DM1C_DUPLICATES is specified.
**	14-nov-88 (teg)
**	    Initialized adf_cb.adf_maxstring = DB_MAXSTRING
**	29-nov-88 (rogerk)
**	    Took out check for duplicate keys when DM1C_DUPLICATES is
**	    specified.
**	24-Apr-89 (anton)
**	    Added local collation support
**	 7-jun-90 (rogerk)
**	    Fixed btree duplicate key error during abort processing.  We
**	    cannot do duplicate checking during abort processing since there
**	    may be duplicate rows that will be resolved later during backout.
**	    Changed the dupflag value DM1C_TID (used during abort processing)
**	    to also indicate no duplicate checking.  It might be better to
**	    allow different flags - one for driving duplicate checking and
**	    one to specify the type of action the routine should take - when
**	    this fix is moved to 6.5, we might explore that option.
**	    Also, I changed the code to not attempt to use already-fixed
**	    leaf pages when the DM1C_TID flag is set, since that code did
**	    not handle that flag.
**	    Also, I changed the code which reuses an already-fixed leaf to
**	    not do duplicate key checking if DM1C_DUPLICATES is specified.  This
**	    matches the code below which uses a newly-fixed leaf.  It seems
**	    like we could consolidate code here.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	18-dec-90 (rogerk)
**	    Rewrote large sections of this routine to fix problems with
**	    unique secondary indexes.  We were getting confused by trying
**	    allocate space and check for duplicates in one pass through
**	    the code, but duplicate checking needs to ignore the TIDP
**	    field in secondary indexes where space allocation needs to
**	    include it.  Added new routine dm1b_dupcheck to do duplicate
**	    checking.  We need to handle TIDP field correctly and need
**	    to check if duplicate checking is required on more than one
**	    leaf.  Documented that 'tid' arguement is not returned as it
**	    was not being set to anything meaningful before.  See history
**	    at top of file for more details.
**	14-jan-91 (rogerk)
**	    Fixed problem in above fixes where we were checking if the
**	    insert tuple was withing the HIGH/LOW range of the currently
**	    fixed leaf.  Substituted "||" for "&&".  We were checking if
**	    (tuple > high range AND tuple < low range).
**	23-jan-1991 (bryanp)
**	    Call dm1cx() routines to support Btree Index Compression.
**	29-apr-1991 (Derek)
**	    Integrate above change to fix bug.  Last part about 'tid' argument
**	    is not correct anymore.  A tid is returned for an allocation
**	    request.  The page is not updated but a TID is assigned.
**	 6-may-1991 (rogerk)
**	    Bypass call to dm1bpfindpage if called in DM1C_TID mode.  This
**	    mode currently means that only the index will be updated, not
**	    the table's data pages.  The call to dm1bpfindpage was causing
**	    us to deadlock during UNDO processing (which is when DM1C_TID
**	    is specified) because we were trying to access pages that were
**	    not touched during normal forward processing.  This is a fix for
**	    a bug which was introduced when the dm1b_allocate was reworked
**	    to handled secondary indexes correctly (dec 90).
**	7-jun-1991 (bryanp)
**	    B37911: Initialize state before calling dm1bxdupkey. We were
**	    erroneously thinking that a leaf page with a non-zero overflow page
**	    pointer was empty because the variable 'state' contained random
**	    stack garbage.
**      23-oct-1991 (jnash)
**          Remove return code check from call to dmd_prrecord, caught by LINT.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	      - Changed name of 'dupflag' parameter in dm1b_allocate to 
**		'opflag' since it was controlling more than duplicate checking.
**	      - Changed DM1C_TID option to DM1C_REALLOCATE since the name TID 
**		gave no clue at all as to what it meant.
**	      - Changed to never do SPLITs while running in REALLOCATE (old TID)
**		mode, which is used during recovery.
**	      - Changed dm1bxsearch to take buffer pointer by reference rather 
**		than by value since it is no longer ever changed.
**	26-jul-1992 (rogerk)
**	    Fixed bug in usage of newduplicate routine.  Leaf argument must
**	    be passed by reference since the leaf page can be temporarily
**	    unfixed by routines that newduplicate calls.  This was causing
**	    dmd_checks when a page was temporarily unfixed and concurrent
**	    access caused the page to be tossed.
**	03-oct-1995 (cohmi01)
**	    Remove call to dm1cxhas_room() that was there to 'make sure we
**	    really have room on the leaf' before finding room on data page.
**	    This check was redundant (with initial call in this func and with
**	    call in dm1bxinsert) and caused false 'RSHIFT' split errors as
**	    it used the max size of key rather than the compressed size.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	06-may-1996 (thaju02)
**	    New Page Format Project. Modify page header references to macros.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Call dm1bxclean() to reclaim space on leaf pages
**      27-feb-97 (stial01)
**          Moved some common code to dm1b_compare_range()
**      10-mar-97 (stial01)
**          Pass record to findpage
**      02-apr-97 (stial01)
**          Put back dm1cxhas_room Consistency check after split.
**          Added code to add overflow page or split again if the first
**          split did not make sufficient space.
**      07-apr-97 (stial01)
**          dm1b_allocate() NonUnique primary btree (V2), always split
**      21-apr-97 (stial01)
**          dm1b_reposition: Search mode is DM1C_FIND for unique keys, DM1C_TID 
**          for non-unique keys, since we re-use unique entries in 
**          dm1b_allocate, the tidp part of the leaf entry may not match.
**      21-may-97 (stial01)
**          dm1b_allocate() Buffer must be locked before dm1bxreserve/dm1bxclean
**          Check for Duplicate deleted entry if we ever were row locking.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          dm1b_allocate() Pass tlv to dm1cxget instead of tcb.
**      21-oct-97 (stial01)
**          dm1b_allocate() B86589: Use leafpage ptr NOT leaf ptr ptr. 
**	13-Apr-2006 (jenjo02)
**	    Support CLUSTERED Leaf pages, which contain the entire data row.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: must dm0p_lock_buf_macro when crow_locking
**	    to ensure the Root is locked.
*/
DB_STATUS
dm1b_allocate(
    DMP_RCB	*rcb,
    DMP_PINFO   *leafPinfo,
    DMP_PINFO   *dataPinfo,
    DM_TID      *bid,
    DM_TID      *tid,
    char        *record,
    char	*crecord,
    i4     need,
    char        *key,
    i4     opflag,
    DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    DB_STATUS       s = E_DB_OK;
    DB_STATUS       get_status;
    DB_STATUS	    state;
    DM_LINE_IDX     lineno;
    DM_TID	    localtid;
    i4		    localpartno;
    i4         	    local_error;
    i4    	    rcompare;
    char            *tmpkey = NULL, *AllocKbuf = NULL;
    char	    keybuf[DM1B_MAXSTACKKEY];
    i4	    	    tmpkey_len;
    i4	    	    compression_type;
    i4	    	    duplicate_insert = FALSE;
    bool	    have_leaf = FALSE;
    bool            correct_leaf;
    bool	    split_required = FALSE;
    DM_TID          base_tid;
    DM_TID8	    tid8;
    i4         	    samekey;
    i4         	    split_count = 0;
    bool            requalify_leaf;
    bool            redo_dupcheck;
    i4		    redo_dupcount = 0;
    char            *leafkey = key;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf, **data;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    data = &dataPinfo->page;

    
    /*
    ** Build btree index key from given record.  If the table is a secondary
    ** index then the key consists of the entire record.
    **
    ** Since we are building a full key, the keys_given is exactly the number
    ** of keys in the btree.
    */

    if ((t->tcb_rel.relstat & TCB_INDEX) &&
	(t->tcb_leaf_rac.att_count == t->tcb_index_rac.att_count))
    {
	/* Secondary Index, Record = Leaf Page entry = Index Page Entry.... */
	MEcopy(record, t->tcb_klen, key);

	/*
	** Extract the TID value out of the secondary index row.
	*/
	if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    MEcopy(key + t->tcb_rel.relwid - sizeof(DM_TID8),
		sizeof(DM_TID8), (char *)&tid8);
	    base_tid.tid_i4 = tid8.tid_i4.tid;
	}
	else
	    MEcopy(key + t->tcb_rel.relwid - sizeof(DM_TID),
		sizeof(DM_TID), (char *)&base_tid);
    }
    else
    {
	char            *pos;
	DB_ATTS         *att;
	i4		k;

	/*
	** If CLUSTERED, Leaf is row, keys at row offsets.
	** INDEX page keys are constructed with the keys  
	** concatenated and do not include non-key columns.
	*/
	if ( t->tcb_rel.relstat & TCB_CLUSTERED )
	{
	    leafkey = record;

	    /* Return the key in row format */
	    for (k = 0; k < t->tcb_keys; k++)
	    {
		att = t->tcb_leafkeys[k];
		MEcopy(record + att->offset, 
			att->length, 
			key + att->offset);
	    }
	}
	else
	{
	    if (t->tcb_rel.relstat & TCB_INDEX)
		leafkey = record;
	    /* Extract the key to INDEX format */
	    for (k = 0, pos = key; k < t->tcb_keys; k++)
	    {
		att = t->tcb_leafkeys[k];
		MEcopy(record + att->offset, att->length, pos);
		pos += att->length;
	    }
	}
    }

    /*
    ** Primary btree, data tid not allocated yet
    ** The leaf entry we reserve will have an invalid tidp.
    */
    if ((t->tcb_rel.relstat & TCB_INDEX) == 0)
    {
	base_tid.tid_tid.tid_page = 0;
	base_tid.tid_tid.tid_line = DM_TIDEOF;
    }

    compression_type = DM1B_INDEX_COMPRESSION(r);

    /* Init fields in rcb for bid reserved */
    DM1B_POSITION_INIT_MACRO(r, RCB_P_ALLOC);
    r->rcb_alloc_tidp.tid_i4 = base_tid.tid_i4;

    /*
    ** Search the Btree Index to find the place for the new record.
    **
    ** If searching in ONLYDUPCHECK mode, we will act as though we are
    ** actually allocating the specified record, but we will not actually
    ** allocate any space.  We search for the spot to insert the record
    ** so that we can check for duplicates there.  After the duplicate
    ** check is complete, we will return.  We will not split the btree
    ** index in ONLYDUPCHECK mode.  Note comments in dm1b_dupcheck for
    ** handling of TIDP field in unique-secondary index duplicate checking.
    **
    ** Performance Check :  Check the currently fixed leaf page to see if it
    ** is the one to which the new record should belong.  If sequential
    ** inserts are being done, then it is likely that we already have the
    ** correct leaf in memory.
    **
    ** Compare the record's key with the leaf's high and low range keys.  If
    ** it falls in between and there is space on the page, then we already 
    ** have the correct leaf fixed.
    */
    while (DB_SUCCESS_MACRO(s) && (*leaf != NULL))
    {
	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "leaf" is a CR page.
	*/
	dm0pLockBufForUpd(r, leafPinfo);

	/*
	** Compare the record's key with the leaf's high and low range keys.
	** If it falls in between and there is space on the page, then we
	** already have the correct leaf fixed.
	*/
	s = dm1b_compare_range( r, *leaf, leafkey, &correct_leaf, dberr);
	if (s != E_DB_OK)
	    break;

	if (correct_leaf == FALSE)
	    break;

	/*
	** We have the correct leaf is fixed. Remove committed deletes.
	** If row locking, the buffer will stay locked during the clean.
	*/
	s = dm1bxclean(r, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	/*
	** If this leaf has no room, then fall through to the search
	** code so a split pass can be done.
	*/
	if (dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			    compression_type, (i4)100 /* fill factor */,
			    t->tcb_kperleaf, (t->tcb_klen + 
			    t->tcb_leaf_rac.worstcase_expansion
			    + DM1B_VPT_GET_BT_TIDSZ_MACRO(
			      t->tcb_rel.relpgtype, *leaf)))
		      == FALSE)
	    break;

	/*
	** In the event we have an overflow chain page fixed, fall
	** through to search for a normal leaf on which to do the
	** insert.
	*/
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *leaf) & 
	    DMPP_CHAIN)
	    break;

	/*
	** If this leaf has an overflow chain, then the leaf is a duplicate
	** key chain, and only rows which match the duplicate key can be
	** inserted here.  If our key does not match the duplicate then fall 
	** through so we can split off a new non-duplicate leaf.
	**
	** Overflow pages are only possible if tcb_dups_on_ovfl
	**
	** Neither apply to CLUSTERED.
	*/
	if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *leaf) != 0)
	{
	    /*
	    ** Find the key for this duplicate key chain. Must initialize
	    ** 'state' first since dm1bxdupkey only sets it if chain is empty
	    ** (bug 37911).
	    */
	    state = E_DB_OK;
	    if ( s = dm1b_AllocKeyBuf(t->tcb_klen, keybuf, &tmpkey, &AllocKbuf, dberr) )
		break;
	    s = dm1bxdupkey(r, leafPinfo, tmpkey, &state, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;

	    /*
	    ** If state is not OK then no duplicate key was found (the
	    ** duplicate chain is empty) which means it is ok for us
	    ** to allocate space on it.
	    **
	    ** If the state is OK then we have to make sure that our
	    ** new record matches the duplicate key.
	    */
	    if (state == E_DB_OK)
	    {
		s  = adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys, 
				leafkey, tmpkey, &rcompare);
	        if (s != E_DB_OK)
	        {
		     uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM93D0_BALLOCATE_ERROR);
		     break;
	        }
		if (rcompare != DM1B_SAME)
		    break;
	    }
	}

	/*
	** We have verified that the correct leaf is fixed.  Find the correct
	** spot on the leaf for the new record.
	**
	** This call may return NOPART if the leaf is empty - in that case
	** it will return with lineno set to the first position (0).
	*/
	s = dm1bxsearch(r, *leaf, DM1C_OPTIM, DM1C_EXACT, leafkey, 
		t->tcb_keys, &localtid, &localpartno, &lineno, NULL, dberr); 
	if (DB_FAILURE_MACRO(s))
	{
	    if (dberr->err_code != E_DM0056_NOPART)
		break;
	    s = E_DB_OK;
	}

	/*
	** Set bid value to spot for the specified record.
	*/
	bid->tid_tid.tid_page = 
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);
	bid->tid_tid.tid_line = lineno;

	have_leaf = TRUE;
	break;
    }


    /*
    ** If we found that we did not already have the correct leaf page fixed,
    ** then do a search on the btree for where to insert the record.
    **
    ** If dm1b_search returns that there is no room for a new record, then
    ** we will have to do a SPLIT pass below.
    */
    while (DB_SUCCESS_MACRO(s) && (have_leaf == FALSE))
    {
	/*
	** Unfix any leaf/data pages left around.
	*/
	if (*leaf != NULL)
	{
	    dm0pUnlockBuf(r, leafPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	}
	if (DB_SUCCESS_MACRO(s) && (*data != NULL))
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, dberr);
	}
	if (DB_FAILURE_MACRO(s))
	    break;

	/*
	** Search the btree in OPTIM mode first.  If there is no room on
	** the required leaf page, then we will need to allocate a new leaf
	** page.  This may require a SPLIT pass through the index.
	**
	** Note that search returns with the leaf to which the given key
	** should be insert even if the search returns NOROOM.  This leaf
	** used below in duplicate checking.
	*/
	s = btree_search(r, leafkey, t->tcb_keys, DM1C_OPTIM, DM1C_EXACT, 
		leafPinfo, bid, &localtid, NULL, dberr); 
	if (DB_FAILURE_MACRO(s))
	{
	    /*
	    ** If NOROOM is returned then we will have to do a split
	    ** pass to allocate space.  Any other return code is unexpected.
	    **
	    ** If the mode is DM1C_REALLOCATE, then we are attempting to
	    ** reallocate space during UNDO recovery and should never split.
	    ** If no space if found we return NOROOM.
	    */
	    if ((opflag == DM1C_REALLOCATE) || (dberr->err_code != E_DM0057_NOROOM))
		break;

	    s = E_DB_OK;
	    split_required = TRUE;
	}

	break;
    }

    /*
    ** If duplicate checking is required, then call dm1b_dupcheck to do
    ** either/or duplicate-key or duplicate-row checking.
    **
    ** If the mode is ONLYDUPCHECK then we will return after this check.
    */
    for (  ; DB_SUCCESS_MACRO(s) &&
	  ((opflag == DM1C_NODUPLICATES) || (opflag == DM1C_ONLYDUPCHECK)); )
    {
	s = dm1b_dupcheck(r, leafPinfo, bid, record, leafkey, 
		&requalify_leaf, &redo_dupcheck, dberr);

	if (s != E_DB_OK)
	    break;

	if (redo_dupcheck)
	{
	    redo_dupcount++;
	    /*
	    ** for unique btree, redo_cnt should never be > 1
	    ** for non-unique btree (noduplicates) redo_count should 
	    ** never be > max concurrent transactions
	    */
#ifdef xDEBUG
	    if (redo_dupcount > dmf_svcb->svcb_xid_lg_id_max)
		TRdisplay("dupcheck redo_cnt %d mytran %x \n", 
		    redo_dupcount, r->rcb_tran_id.db_low_tran);
#endif
	}

	/*
	** If only checking duplicates, then report success.
	** If an error occurred, then we will fall through to the
	** error handling at the bottom of the routine.
	*/
	if (opflag == DM1C_ONLYDUPCHECK && !redo_dupcheck)
	{
	    dm0pUnlockBuf(r, leafPinfo);
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &tmpkey);
	    return (E_DB_OK);
	}

	if ( (t->tcb_rel.relpgtype != DM_COMPAT_PGTYPE) &&
		(redo_dupcheck || requalify_leaf))
	{
	    s = dm1b_requalify_leaf(r, DM1C_OPTIM, leafPinfo, 
				leafkey, t->tcb_keys, dberr);
	    if (s != E_DB_OK)
		break;

	    if (dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			*leaf, compression_type,
			(i4)100 /* fill factor */,
			t->tcb_kperleaf,
			(t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion
			    + DM1B_VPT_GET_BT_TIDSZ_MACRO(
			      t->tcb_rel.relpgtype, *leaf))))
		split_required = FALSE;
	    else
		split_required = TRUE;

	    /*
	    ** Find the correct spot on the leaf for the new record.
	    ** This call may return NOPART if the leaf is empty 
	    ** In that case it will return with lineno set to 0, 1st pos
	    */
	    s = dm1bxsearch(r, *leaf, DM1C_OPTIM, DM1C_EXACT, leafkey, 
		
		    t->tcb_keys, &localtid, &localpartno, 
		    &lineno, NULL, dberr);
	    if (DB_FAILURE_MACRO(s) && dberr->err_code == E_DM0056_NOPART)
		s = E_DB_OK;

	    /* Set bid value to spot for the specified record. */
	    if (s == E_DB_OK)
	    {
		bid->tid_tid.tid_page =
		   DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
					    *leaf);
		bid->tid_tid.tid_line = lineno;
	    }

	    if (redo_dupcheck)
		continue;
	}
		
	break;
    }

    /*
    ** On large pages we can have deleted keys
    ** If we have unique keys and we are positioned at a duplicate
    ** deleted key we MUST reuse the entry 
    **
    ** This can only happen with page type != TCB_PG_V1,
    ** if the same transaction deletes and then inserts the same unique key.
    **
    ** This needs to be checked here because duplicate checking
    ** and allocate space can be done in separate calls to dm1b_allocate
    **
    */
    if (DB_SUCCESS_MACRO(s)
	&& (t->tcb_rel.relstat & TCB_UNIQUE)
	&& (t->tcb_rel.relpgtype != TCB_PG_V1)
	&& ((i4)bid->tid_tid.tid_line < 
	   (i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
	   *leaf)))
    {
	samekey = DM1B_NOTSAME;
	lineno = bid->tid_tid.tid_line;
	tmpkey_len = t->tcb_klen;
	if ( !tmpkey )
	     s = dm1b_AllocKeyBuf(tmpkey_len, keybuf, &tmpkey, &AllocKbuf, dberr);
	if ( s == E_DB_OK )
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf, 
			    &t->tcb_leaf_rac,
			    lineno, &tmpkey, 
			    (DM_TID *)NULL, (i4*)NULL,
			    &tmpkey_len, NULL, NULL, adf_cb);

	get_status = s;
	if (s == E_DB_WARN && (t->tcb_rel.relpgtype != TCB_PG_V1) )
	{
	    s = E_DB_OK;
	}

	if (s == E_DB_OK)
	{
	    adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys,
			    tmpkey, leafkey, &samekey);
	}
	else if (s == E_DB_ERROR)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, lineno);
	}

	/* Toss allocated key buffer, if any */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &tmpkey);

	if ((s == E_DB_OK) && (samekey == DM1B_SAME))
	{

	    if (get_status == E_DB_WARN)
	    {
		/*
		** Reserve leaf entry
		**    Allocate leaf entry = TRUE
		**    Mutex required = TRUE, buf should already be locked
		*/
		s = dm1bxreserve(r, leafPinfo, bid, leafkey, FALSE, TRUE, 
				 dberr);
		if (s != E_DB_OK)
		    dm1cxlog_error(E_DM93C6_ALLOC_RESERVE, r, *leaf,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (i4)0);
	    }
	    else
	    {
		/*
		** When must doing allocate for REPLACE
		** In dm1b_replace, we will do dm1bxdelete,dm1bxinsert
		** The delete will mark this entry deleted,
		** the insert will reuse the entry.
		*/
		DM1B_POSITION_SAVE_MACRO(r, *leaf, bid, RCB_P_ALLOC);
	    }

	    /* No split required when we re-use a leaf entry */
	    split_required = FALSE;
	}
    }

    /*
    ** If we found no room in the btree index to insert the requested key,
    ** then we may have to perform a SPLIT.
    **
    ** First check for special duplicate key processing.  If the returned
    ** leaf contains all duplicates which match the insert key then rather
    ** than a split, we must insert the new key on an overflow leaf page.
    **
    ** We look for space on the 1st existing overflow page (if there is one).
    ** If there is no space there, then we allocate a new overflow leaf.
    */
    while (split_required && DB_SUCCESS_MACRO(s))
    {
	/*
	** If duplicate keys are not allowed in the index, then we never
	** have to worry about overflow pages.
	*/
	if ((t->tcb_rel.relstat & TCB_UNIQUE)
	    ||
	    ((t->tcb_rel.relstat & TCB_INDEX)
	    &&
	    (t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0)))
	    break;

	/* If this btree can't have overflow pages, split */
	if (!t->tcb_dups_on_ovfl)
	    break;

	/*
	** Check if the leaf which contains the keyrange for the insert key
	** is full of duplicate keys which match our new key.  If not, then
	** fall through to perform a SPLIT.  If it is then a split will not
	** help, we must find space on an overflow leaf.
	*/
	duplicate_insert = FALSE;
	s = newduplicate(r, leafPinfo, leafkey, &duplicate_insert, dberr);
	if (s != E_DB_OK)
	    break;

	/*
	** Not a duplicate key  - fall through to do a Split.
	*/
	if ( ! duplicate_insert)
	    break;

	/* Before adding overflow chain, isolate dupkey on its own leaf */
	s = isolate_dup(r, leafPinfo, leafkey, dberr);
	if (s != E_DB_OK)
	    break;

	split_required = FALSE;

	/*
	** Check for space on the first overflow leaf page (if there is
	** one).  We currently do not check past the first overflow page
	** for space to avoid scans down extremely long chains if many
	** duplicates exist.
	*/
	s = dm1b_find_ovfl_space(r, leafPinfo, leafkey,
		(t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion +
		 DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, *leaf)),
				 bid,dberr);
	if ((s != E_DB_OK) && (dberr->err_code != E_DM0057_NOROOM))
	    break;

	/*
	** If found space on an overflow page, then no split or page allocation
	** is required.  Turn off the 'split_required' flag and continue
	** down to the data page allocation code.
	**
	** At this point 'bid' will point to the allocated spot on the overflow
	** page, but the main leaf page (head of the chain) will still be
	** fixed in 'leaf'.  Even though we will be inserting to an overflow
	** page, the data row will be inserted on the main leaf's associated
	** data page.
	*/
	if (s == E_DB_OK)
	{
	    break;
	}

	/*
	** If no space was found in the overflow chain, then allocate a
	** new overflow page.  This page will be inserted as the first
	** element in the current leaf's overflow chain.
	**
	** After allocating a new page, this routine will allocate space
	** on that page for the new key (by returning the bid to which to
	** insert).
	**
	** At this point 'bid' will point to the allocated spot on the overflow
	** page, but the main leaf page (head of the chain) will still be
	** fixed in 'leaf'.  Even though we will be inserting to an overflow
	** page, the data row will be inserted on the main leaf's associated
	** data page.
	*/
	s = dm1bxovfl_alloc(r, leafPinfo, leafkey, bid, dberr);
	if (s != E_DB_OK)
	    break;

	break;
    }


    /*
    ** If we determined above that a split was required to allocate room
    ** in the btree index, then search the btree in SPLIT mode.
    */
    while (split_required && DB_SUCCESS_MACRO(s))
    {
	/*
	** Unfix any leaf/data pages left around.
	*/
	if (*leaf != NULL)
	{
	    dm0pUnlockBuf(r, leafPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	}
	if (DB_SUCCESS_MACRO(s) && (*data != NULL))
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, dberr);
	}
	if (DB_FAILURE_MACRO(s))
	    break;

	s = btree_search(r, leafkey, t->tcb_keys, DM1C_SPLIT, DM1C_EXACT, 
		leafPinfo, bid, &localtid, NULL, dberr); 
	if (DB_FAILURE_MACRO(s))
	    break;

	split_count++;

	/*
	** Don't check for space if we reserved leaf entry already
	*/
	if ((t->tcb_rel.relpgtype != TCB_PG_V1)
	    && DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC))
	{
	    bid->tid_i4 = r->rcb_pos_info[RCB_P_ALLOC].bid.tid_i4;
	    break;
	}

	/*
	** Consistency check:  make sure that the leaf we have selected
	** really has room on it for the insert.
	*/
	if (dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			    compression_type, (i4)100 /* fill factor */,
			    t->tcb_kperleaf, (t->tcb_klen + 
			    t->tcb_leaf_rac.worstcase_expansion 
			    + DM1B_VPT_GET_BT_TIDSZ_MACRO(
				t->tcb_rel.relpgtype, *leaf))) == TRUE)
	    break;

	/*
	** If duplicate keys are allowed, we can have overflow pages
	** If keys are compressed, splitting a page may not make
	** enough space for the new key because we don't always
	** split in the middle if there are duplicate keys on the leaf
	**
	** Depending on page type, relcmptlvl, dups might be on overflow pages
	*/
	if ( (t->tcb_dups_on_ovfl)
	    && ((t->tcb_rel.relstat & TCB_UNIQUE) == 0)
	    && !((t->tcb_rel.relstat & TCB_INDEX) &&
	    (t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0))
	    && (compression_type == DM1CX_COMPRESSED) )
	{
	    /*
	    ** Check if the leaf is full of duplicate keys
	    ** which match our new key. If so, add an overflow page
	    */
	    duplicate_insert = FALSE;
	    s = newduplicate(r, leafPinfo, leafkey, &duplicate_insert, dberr);
	    if (s != E_DB_OK)
		break;

	    if (duplicate_insert == TRUE)
	    {
		/*
		** Allocate an overflow page if our key is a duplicate.
		** 'bid' will point to the allocated spot on the new 
		** overflow page, but the main leaf page will still be 
		** fixed in 'leaf'
		** Even though we will be inserting to an overflow page,
		** the data row will be inserted on the main leaf's
		** associated data page.
		*/
		s = dm1bxovfl_alloc(r, leafPinfo, leafkey, bid, dberr);

		/* Fall through to error handling below */
		break;

	    }
	}

	if ( (compression_type == DM1CX_COMPRESSED) && 
	     ( (split_count <= 1) || (set_row_locking(r)) ))
	{
	    /*
	    ** Try to split once more
	    ** When we have compressed keys, we may not split the page evenly
	    ** When page locking we should only need to split once more
	    */
	    continue;
	}

	dm1cxlog_error(E_DM93E9_BAD_INDEX_RSHIFT, r, *leaf, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (i4)0);
	SETDBERR(dberr, 0, E_DM934A_BTREE_NOROOM);
	s = E_DB_ERROR;
	break;
    }

    if ((row_locking(r) || crow_locking(r))
	    && DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC) == FALSE
	    && DB_SUCCESS_MACRO(s))
    {
	/*
	** If leaf entry not yet reserved, do so now
	**    The correct leaf which describes BID must be fixed into leaf
	**    Allocate leaf entry = TRUE
	**    Mutex required = TRUE, buf should already be locked
	*/
	s = dm1bxreserve(r, leafPinfo, bid, leafkey, TRUE, TRUE, dberr);
	if (s != E_DB_OK)
	    dm1cxlog_error(E_DM93C6_ALLOC_RESERVE, r, *leaf, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (i4)0);

	/* Fall through to the error handling */
    }

    /* 
    ** Have now found the correct portion of the btree index for the
    ** new record - now find a spot on a data page.  Don't need to do this
    ** if this is a secondary index or if DM1C_REALLOCATE mode is specified
    ** or if this is a CLUSTERED base table.
    **
    **	    - If this is a secondary index then there are no data pages.
    **	    - If this is a CLUSTERED base table then there are no data pages.
    **	    - If DM1C_REALLOCATE mode, then the caller has requested to not
    **	      search for space on the data page, only an index entry will
    **	      be made.  This is used only in UNDO recovery where the index
    **	      and data page updates are aborted separately.
    */
    if (DB_SUCCESS_MACRO(s) && 
	    (t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED)) == 0 &&
	    opflag != DM1C_REALLOCATE)
    {
	if (dm0pNeedBufLock(r, leafPinfo) && !dm0pBufIsLocked(leafPinfo) )
	{
	    dmd_check(E_DM93F5_ROW_LOCK_PROTOCOL);
	}

	if (t->tcb_extended && r->rcb_bulk_misc && r->rcb_bulk_cnt > 0)
	{
	    /* Alloc one disassoc data page for rcb_bulk_cnt rows */
	    /* The subsequent PUT will need to deal with the leaf entries */
	    s = bulkpage(r, leafPinfo, dataPinfo,
		    t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
		    need, leafkey, tid, bid, dberr);
	}
	else
	{

	    s = findpage(r, leafPinfo, dataPinfo, 
		    t->tcb_rel.relcomptype == TCB_C_NONE ? record : crecord,
		    need, leafkey, tid, bid, dberr);
	}

	/* Fall through to check return status */
    }

    /* Unlock leaf before unfix */
    dm0pUnlockBuf(r, leafPinfo);

    /*
    ** Make sure that the correct leaf which describes the allocated BID
    ** is returned fixed from this routine.
    **
    ** In cases when we are inserting to an overflow page, we will have left
    ** the parent leaf page fixed for the findpage call above.  We must now
    ** fix the correct page for the index update.
    */
    if (DB_SUCCESS_MACRO(s) && (t->tcb_dups_on_ovfl) &&
	(DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) != 
	bid->tid_tid.tid_page))
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	if (DB_SUCCESS_MACRO(s))
	{
	    s = dm0p_fix_page(r, bid->tid_tid.tid_page, 
		    (DM0P_WRITE | DM0P_NOREADAHEAD), leafPinfo, dberr);
	}
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &tmpkey);

    if (DB_SUCCESS_MACRO(s))
	return (E_DB_OK);

    /*
    ** An error occurred.  Clean up and report errors.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM925E_DM1B_ALLOCATE);
    }

    if (leafPinfo->page != NULL)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, &local_dberr);
	if (DB_FAILURE_MACRO(s))
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dataPinfo->page != NULL)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, &local_dberr);
	if (DB_FAILURE_MACRO(s))
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1b_dupcheck - Check for duplicates in btree unique indexes.
**
** Description:
**	This routine checks if a given row will duplicate existing rows
**	in a btree table.  It does duplicate key or duplicate row checking
**	depending on the duplicate restrictions of the table.  Since
**	unique keys are more constraining than unique rows, we never need
**	to check both conditions.
**
**	If a row violates both unique KEY and unique ROW constraints, then
**	a unique KEY error will be returned.
**
**	The caller of this routine is required to search the btree to find
**	the spot at which to begin duplicate checking and pass the fixed
**	leaf page into this routine.
**
**	This routine may fix different leaf pages during duplicate checking,
**	but will return with the same leaf fixed upon return.
**
**	The caller is also required to pass in the position on that leaf
**	at which to start duplicate checking.
**
**	To check for duplicate rows, this routine needs to compare tuples
**	for each entry which has a key duplicating the insert key.  If the
**	leaf has an overflow chain, then that chain must be followed as
**	well.  Since keys are stored on leaves in sorted order, duplicate
**	checking may stop as soon as a non-duplicate KEY is encountered.
**
**	To check for duplicate keys, only one entry need be checked.  Since
**	rows are stored in sorted order, if the entry at the given position
**	does not duplicate the insert key then none will (there are
**	exceptions to this rule in unique 2nd index checking - see below).
**
**
**	This routine should only be called if duplicate checking is required:
**
**	    - base table with unique key constraint
**	    - base table that does not allow duplicates
**	    - unique secondary index.
**
**	This routine should never be called to check for duplicates in a
**	non-unique secondary index.
**
***************************************************************************
**
**	This routine does special processing on Release 6 unique secondary
**	indexes.  These secondary indexes are built with the TIDP field
**	included in the KEY of the secondary index (previous to release 6
**	secondary indexes had a tidp field, but it was not included by
**	default as part of the key values).
**
**	While the TIDP is included as part of the key, uniqueness constraints
**	are considered violated if a duplicate is added which is non-unique
**	in the fields other than the TIDP (it is impossible to add a row
**	which duplicates another's tidp field).
**
**	In order to do duplicate checking properly, we need to ignore the
**	the TIDP field when comparing keys in Rel 6 unique 2nd indexes.
**
**	This causes some other side effects as well.  The calling routine
**	is expected to pass in the leaf page at which the insert key would
**	be put.  This leaf is found using the TIDP as part of the search
**	key.  Since a key that is built excluding the tidp is actually
**	a partial key, and since it is possible for a partial key range
**	to span leaf pages, the caller may not have passed in the correct
**	leaf at which to begin duplicate checking. If the caller has
**	passed in the correct leaf, it may not have given the correct
**	position on the leaf at which to begin checking.
**
**	Also, duplicate checking may require following the nextpage pointer
**	to the next leaf.
**
**	Fortunately, it is impossible for our non-tidp partial key to span a 
**	wide range.  In fact, it can only span one row (more than one row 
**	would imply duplicate violations).  The only problem is predicting
**	the page at which that one row may be.  Since our range can only
**	span one row, we only need to look forward or backward one position
**	to do proper dupliate checking.
**
**	If however, there is no entry backwards, then we may need to search
**	to find the previous leaf.  If there is no other entry forwards, we
**	may need to go forward to the next leaf.
**
***************************************************************************
**
** Inputs:
**      rcb                             Pointer to record access context.
**	leaf				Pointer to currently fixed leaf page.
**	bid				Bid at which to start dup checking.
**      record                          Pointer to insert record which we
**					need to verify uniqueness.
**	key				Keyed portion of record.
**
** Outputs:
**      leaf                            Pointer to an area used to return
**                                      pointer to leaf page.
**      err_code                        Error code if failure:
**                                      E_DM0045_DUPLICATE_KEY
**					E_DM0046_DUPLICATE_RECORD
**                                      E_DM934B_DM1B_DUPCHECK
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
**      Fixes and locks leaf pages.
**
** History:
**      18-dec-90 (rogerk)
**          Written in order to sort out differences between duplicate
**	    checking and space allocation.  Most of this routine was
**	    taken out of dm1b_allocate.  Along the way some problems
**	    were fixed having to do with tidp handling in secondary indexes.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree Index Compression.
**	10-apr-1995 (rudtr01)
**	    Bug #52462 - dm1b_dupcheck() sometimes trashes the bid passed
**	    by the caller when searching the B-tree for duplicates.
**	24-jan-96 (pchang)
**	    Fixed bug 73672 - Duplicate rows were being inserted into btree 
**	    tables with unique index.  If we are positioned to the start of a
**	    new leaf when checking for duplicates in a unique index, and if we 
**	    then have to go back to the previous leaf page in order to check
**	    the key value on its last entry for duplicates, the bid and leaf 
**	    pointers will have changed after the search using the partial key
**	    (i.e. excluding the tidp) and must not be referenced as if they
**	    were the same pointers we had been given.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added handling of deleted leaf entries (status == E_DB_WARN)
**      27-feb-97 (stial01)
**          Since tidp is no longer part of key for unique indexes,
**          we never have to worry about visiting adjacent leaf pages
**          during unique duplicate checking for tables created with
**          the new 2.0 format (page size > 2k). Therefore we never have
**          to worry about 'deleted' records in this code.
**      21-apr-97 (stial01)
**          Added requalify_leaf output parameter, set if we unfix our leaf page
**      21-may-97 (stial01)
**          dm1b_dupcheck() New buf_locked parameter.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          dm1b_dupcheck() Pass tlv to dm1cxget instead of tcb.
**      22-jul-98 (stial01)
**          dm1b_dupcheck() Reset local leaf ptr after dm1badupcheck (b92116)
**	15-jan-1999 (nanpr01)
**	    Pass pointer to a pointer to dm0m_deallocate function.
**      05-may-1999 (nanpr01,stial01)
**          dm1b_dupcheck() Key value locks acquired for dupcheck are no longer 
**          held for duration of transaction. While duplicate checking,
**          request/wait for row lock if uncommitted duplicate key is found.
*/
DB_STATUS
dm1b_dupcheck(
    DMP_RCB	*rcb,
    DMP_PINFO   *leafPinfo,
    DM_TID	*bid,
    char        *record,
    char        *leafkey,
    bool        *requalify_leaf,
    bool        *redo_dupcheck,
    DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    DB_STATUS	    s = E_DB_OK;
    DB_STATUS       get_status;
    DM_PAGENO       next_page;
    DM_PAGENO	    save_page;
    DM_LINE_IDX     lineno;
    DM_TID	    tid;
    DM_TID	    localbid;
    DM_TID          localtid;
    i4         	    keys_given;
    i4	            compare;
    i4	    	    infinity;
    char            *tmpkey, *KeyPtr, *AllocKbuf = NULL;
    char	    keybuf[DM1B_MAXSTACKKEY];
    i4	    	    tmpkey_len;
    bool	    ignore_tidp = FALSE;
    u_i4	    row_low_tran = 0;
    u_i2	    row_lg_id;
    i4              new_lock = FALSE;
    i4              lock_flag = 0;
    LK_LOCK_KEY	    save_lock_key;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf;

    CLRDBERR(dberr);

    *requalify_leaf = FALSE;
    *redo_dupcheck = FALSE;

    leaf = &leafPinfo->page;

    /*
    ** First find the spot in the btree index at which records with this
    ** key belong so that we can do duplicate checking.
    **
    ** As a performance saving measure, check to see if the correct leaf
    ** is already fixed in the leaf pointer.  This may save us from
    ** unnecessarily searching down the btree index.
    **
    ** Note that Release 6 btree secondary indexes include the TIDP as part
    ** of the Key, but that this field should be ignored for duplicate
    ** key checking.  Release 6 btree secondary indexes can never REALLY
    ** have duplicate keys because of the TIDP, but we will return a dupkey
    ** error if keys are duplicate in the non-tidp fields.
    */

    /*
    ** Adjust the key to include only the fields needed for duplicate checking.
    ** If this is a Release 6 secondary index, then ignore the TIDP field
    ** for duplicate checking as including it will cause us to never see
    ** duplicates.  Pre-release 6 secondary indexes did not include the TIDP
    ** field as part of the keyed fields.
    **
    ** As of CA-OpenIngres 2.0, we do not include TIDP in the key of 
    ** UNIQUE indexes. This eliminates the overhead of duplicate 
    ** checking on adjacent leaves. This is necessary for row locking
    ** protocols that exist.
    **
    ** Check the tcb's attribute array to see if the TIDP is included in the
    ** key (the TIDP is the last attribute).  If the TIDP is listed as a
    ** keyed field, then decrement the count of keyed columns so that it
    ** will be ignored. (Decrementing the count causes us to ignore the TIDP
    ** since it is always the last keyed column).
    */
    keys_given = t->tcb_keys;
    if ((t->tcb_rel.relstat & TCB_INDEX) &&
	(t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0))
    {
	keys_given--;
	ignore_tidp = TRUE;
	if (t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    (VOID) uleFormat(NULL, E_DM9C29_BXV2KEY_ERROR, (CL_ERR_DESC*)NULL, ULE_LOG, 
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM934B_DM1B_DUPCHECK);
	    return (E_DB_ERROR);
	}
    }


    /*
    ** Make a local copy of the bid passed by the caller and use that.  If
    ** we have to search the B-tree again because we determined that there
    ** could be duplicates on the previous leaf (in secondary indices as we
    ** ignore the tidp as part of the key), the bid passed by the caller 
    ** would be trashed.  This would lead to a key entry being inserted 
    ** into the correct leaf page, but into the wrong line-entry, i.e. out
    ** of sort sequence.  This error would be detected only if the trashed
    ** bid's line value was outside the range of line-entries on the page.
    */
    STRUCT_ASSIGN_MACRO(*bid, localbid);

    /*
    ** Remember the currently fixed page so we can return with it fixed.
    ** The caller will probably want to insert a record on this page.
    */
    save_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

    /*
    ** Check for duplicate keys if required.
    ** Duplicate key AND duplicate row checking are never required as duplicate
    ** key checking is a more restrictive case (if there are no duplicate keys
    ** then there can be no duplicate rows).
    **
    ** If checking duplicate keys, then we need to verify that the correct
    ** leaf page was passed in on which to check for duplicates.  The caller
    ** will have found a spot at which to insert the new row - using its
    ** full key entry.  If this is a Release 6 unique secondary index, then
    ** we need to do duplicate checking ignoring the TIDP value.  This could
    ** mean that we are not correctly positioned to start duplicate checking
    ** since there could be a row with a different TIDP value that is on
    ** a different leaf page.
    **
    ** We assume that this 'wrong-leaf-fixed' situation can only occur
    ** when checking for unique keys in a Release 6 secondary index
    ** (The ignore-tidp condition does not occur in non unique or base tables).
    */
    while (t->tcb_rel.relstat & TCB_UNIQUE)
    {
	/* Set up temporary key work space, in the stack or elsewhere */
	if ( s = dm1b_AllocKeyBuf(t->tcb_klen, keybuf, &KeyPtr, &AllocKbuf, dberr) )
	    break;
	/*
	** Check if we have the correct leaf with which to start duplicate
	** checking.  If we need to ignore TIDP fields and we are positioned
	** to the start of a new leaf, then it is possible that the last
	** entry of the leaf previous to this one holds a key value equal
	** to our insert key, but with a smaller tidp value.
	**
	** Compare the insert key with the low range key on this leaf.
	** Only if the keys match (excluding the tidp of course) can two
	** duplicate key entries span leaf pages.  If the keys don't match
	** then we can assume we are on the correct leaf.
	*/
	if (ignore_tidp && (localbid.tid_tid.tid_line == 0))
	{
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf, 
			DM1B_LRANGE, (DM_TID *)&infinity, (i4*)NULL);
	    compare = DM1B_SAME + 1;
	    if (infinity == FALSE)
	    {
		tmpkey = KeyPtr;
		tmpkey_len = t->tcb_rngklen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			t->tcb_rng_rac,
			DM1B_LRANGE,
			&tmpkey, (DM_TID *)NULL, (i4*)NULL, &tmpkey_len,
			NULL, NULL, adf_cb);
		if (s != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_LRANGE);
		    SETDBERR(dberr, 0, E_DM93D1_BDUPKEY_ERROR);
		    break;
		}
		/* Compare range key :: leafkey */
		adt_ixlcmp(adf_cb, keys_given, 
				t->tcb_rngkeys, tmpkey, 
				t->tcb_leafkeys, leafkey, &compare);
	    }

	    /*
	    ** If the insert key matches the low range key, then we need
	    ** to check for duplicates on the previous leaf page.  Since
	    ** we don't know how to scan backwards, we search the btree
	    ** using a partial key (the insert key minus the tidp field)
	    ** to find the spot at which to start duplicate checking.
	    */
	    if (compare == DM1B_SAME)
	    {
		/*
		** Unfix the old leaf page.
		*/
		if (*leaf != NULL)
		    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;

		/*
		** Search for specified spot in the btree.
		** If the leaf is full, then NOROOM will be returned, but the
		** bid value will be set correctly.
		*/
		s = btree_search(r, leafkey, keys_given, DM1C_OPTIM, DM1C_EXACT,
			leafPinfo, &localbid, &tid, NULL, dberr);
		if (DB_FAILURE_MACRO(s))
		{
		    if (dberr->err_code != E_DM0057_NOROOM)
			break;
		    s = E_DB_OK;
		}
	    }
	}
	else if (ignore_tidp && (localbid.tid_tid.tid_line > 0))
	{
	    /*
	    ** Check if we have to look backwards one position. 
	    **
	    ** If we are ignoring the TIDP field for duplicate checking, then
	    ** even though we may have entered the routine with the correct
	    ** leaf fixed, we may be positioned to the line table entry 
	    ** immediately following a duplicate key.  This could happen if
	    ** the tid of the insert key is greater than the key of the
	    ** duplicate key.  If we are positioned in the middle of the
	    ** leaf page, then check the previous key for duplicates.
	    */
	    lineno = localbid.tid_tid.tid_line - 1;
	    tmpkey = KeyPtr;
	    tmpkey_len = t->tcb_klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			lineno, &tmpkey, (DM_TID *)NULL, (i4*)NULL,
			&tmpkey_len,
			NULL, NULL, adf_cb);

	    /*
	    ** Since TIDP is not part of 2.0 unique secondary indexes,
	    ** we don't ever expect to get a 'deleted' entry
	    */
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, *leaf,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, lineno );
		SETDBERR(dberr, 0, E_DM93D1_BDUPKEY_ERROR);
		break;
	    }
	    
	    adt_kkcmp(adf_cb, keys_given, t->tcb_leafkeys, tmpkey, leafkey, 
			    &compare );

	    if (compare == DM1B_SAME)
	    {
		s = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
		break;
	    }
	}

	/*
	** Check the key at bid to see if it is a duplicate.
	*/
	lineno = localbid.tid_tid.tid_line;

	if ((i4)lineno < 
	    (i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf))
	{
	    tmpkey = KeyPtr;
	    tmpkey_len = t->tcb_klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			lineno, &tmpkey, &localtid, &r->rcb_partno,
			&tmpkey_len,
			&row_low_tran, &row_lg_id, adf_cb);

	    get_status = s;

	    /*
	    ** If duplicate key checking and not V1 page, and positioned
	    ** at deleted key, duplicate checking is done
	    ** (we shouldn't have deleted keys on V1 pages)
	    */
	    if (s == E_DB_WARN && t->tcb_rel.relpgtype != TCB_PG_V1)
	    {
		s = E_DB_OK;
		if (DM1B_SKIP_DELETED_KEY_MACRO(r, *leaf, row_lg_id, row_low_tran))
		    break;
	    }

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, lineno);
		SETDBERR(dberr, 0, E_DM93D1_BDUPKEY_ERROR);
		break;
	    }

	    /* When row locking, we can compare keys without locking */
	    adt_kkcmp(adf_cb, keys_given, t->tcb_leafkeys, tmpkey, leafkey,
			      &compare);

	    if (compare == DM1B_SAME)
	    {
		if (DM1B_DUPCHECK_NEED_ROW_LOCK_MACRO(r, *leaf, row_lg_id, 
							row_low_tran))
		{
		    dm0pUnlockBuf(r, leafPinfo);

		    if ( crow_locking(r) )
		    {
			/* this will get PHYS lock and then unlock */
			s = dm1r_crow_lock(r, DM1R_LK_PHYSICAL, &localtid, NULL, dberr);
		    }
		    else
		    {
			/* Don't clear lock coupling in rcb */
			lock_flag = DM1R_LK_PHYSICAL |
				    DM1R_ALLOC_TID | DM1R_LK_PKEY_PROJ;

			/*
			** Note that if a Global SI, rcb_partno was set
			** by the dm1cxget() call, above.
			*/
			s = dm1r_lock_row(r, lock_flag, &localtid, 
					  &new_lock, &save_lock_key,
					  dberr);
			if (s == E_DB_OK)
			    _VOID_ dm1r_unlock_row(r, &save_lock_key, 
						   &local_dberr);
		    }
		    if ( s != E_DB_OK )
		        break;
		    *redo_dupcheck = TRUE;
		    break;
		}

		if (get_status == E_DB_OK) 
		{
		    s = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
		    break;
		}
	    }
	}

	/*
	** Check if we have to look forward to the next leaf.
	**
	** If we are positioned to the end of a leaf, then there are no
	** entries on this leaf to check for duplicates.  However, it is
	** possible that the 1st entry of the next leaf page holds a 
	** key that duplicates ours, but has a tidp value greater than
	** the insert record's tidp.
	**
	** This can only be true if we are ignoring the tidp field for
	** duplicate checking and the insert key matches the high range
	** key for this leaf.  If we are and it does then follow the
	** nextpage pointer and check the first entry on that leaf for
	** duplicates.  It is sufficient to check the first entry since
	** keys are stored in sorted order - and the 1st entry cannot
	** be less than the insert key since our insert key will match
	** the low range value for that leaf.
	*/
	if (ignore_tidp && ((i4)lineno >= 
	    (i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf)))
	{
	    next_page = 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

            dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf, 
			DM1B_RRANGE, (DM_TID *)&infinity, (i4*)NULL);
	    compare = DM1B_SAME - 1;
	    if (infinity == FALSE)
	    {
		tmpkey = KeyPtr;
		tmpkey_len = t->tcb_rngklen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    *leaf, t->tcb_rng_rac, 
		    DM1B_RRANGE,
		    &tmpkey, (DM_TID *)NULL, (i4*)NULL, &tmpkey_len,
		    NULL, NULL, adf_cb);
		if (s != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_RRANGE);
		    SETDBERR(dberr, 0, E_DM93D1_BDUPKEY_ERROR);
		    break;
		}
		/* Compare range key :: leafkey */
		adt_ixlcmp(adf_cb, keys_given, 
				t->tcb_rngkeys, tmpkey, 
				t->tcb_leafkeys, leafkey, &compare);
	    }

	    /*
	    ** If the insert key matches the high range key and there is
	    ** a next leaf page, then fix it and check its first entry
	    ** for duplicates.
	    */
	    if ((compare == DM1B_SAME) && (next_page != 0))
	    {
		/*
		** The insert key matched the high range - so follow to the
		** next leaf page and continue duplicate checking.
		*/
		dm0pUnlockBuf(r, leafPinfo);

		s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;

		s = dm0p_fix_page(r, next_page, 
                    (DM0P_WRITE | DM0P_NOREADAHEAD), 
		    leafPinfo, dberr);
		if (DB_FAILURE_MACRO(s))
		    break;

		/*
		** Lock buffer for update.
		**
		** This will swap from CR->Root if "leaf" is a CR page.
		*/
	        dm0pLockBufForUpd(r, leafPinfo);

		/*
		** If this leaf has entries, then check the first one.
		*/
		if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf) > 0)
		{
		    tmpkey = KeyPtr;
		    tmpkey_len = t->tcb_klen;
		    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			*leaf, &t->tcb_leaf_rac,
			0, &tmpkey,
			(DM_TID *)NULL, (i4*)NULL,
			&tmpkey_len, NULL, NULL, adf_cb);

		    /*
		    ** Since TIDP is not part of 2.0 unique secondary indexes,
		    ** we don't ever expect to get a 'deleted' entry
		    */
		    if (s != E_DB_OK)
		    {
			dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 0);
			SETDBERR(dberr, 0, E_DM93D1_BDUPKEY_ERROR);
			break;
		    }

		    adt_kkcmp(adf_cb, keys_given, t->tcb_leafkeys, 
				    tmpkey, leafkey, &compare);
		    if (compare == DM1B_SAME)
		    {
			s = E_DB_ERROR;
			SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			break;
		    }
		}
	    }
	}

	break;
    }

    /* Toss any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    /*
    ** Check for duplicate rows.
    ** Duplicate row checking is not required if this is a secondary index
    ** or if duplicate key checking was already done above.
    **
    ** The dm1badupcheck routine assumes that the correct leaf is fixed
    ** to use for duplicate checking.  There are no complexities of
    ** ignoring TIDP fields when doing duplicate row checking as we
    ** only need to check duplicate rows in base tables.
    */
    if (DB_SUCCESS_MACRO(s) && ((t->tcb_rel.relstat & TCB_UNIQUE) == 0))
    {
	s = dm1badupcheck(r, leafPinfo, &localbid, &tid, record, leafkey,
		requalify_leaf, redo_dupcheck, dberr);
    }

    while (DB_SUCCESS_MACRO(s))
    {
	/*
	** Make sure same leaf is fixed as on entry to the routine.
	*/
	if (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) != 
	    save_page)
	{
	    *requalify_leaf = TRUE;
	    dm0pUnlockBuf(r, leafPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;

	    s = dm0p_fix_page(r, save_page, (DM0P_WRITE | DM0P_NOREADAHEAD), 
		leafPinfo, dberr);
	    if (DB_FAILURE_MACRO(s))
		break;

	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if "leaf" is a CR page.
	    */
	    dm0pLockBufForUpd(r, leafPinfo);
	}

	return (E_DB_OK);
    }

    /*
    ** Clean up and report errors.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM934B_DM1B_DUPCHECK);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1b_bybid_get - Get a record from a BTREE table.
**
** Description:
**      This routine used the bid provided to determine the
**      the tid of the record and retrieves the record.
**      This routine was added to handle replaces which move
**      records.
** 
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             Bid of record to retrieve.
**      tid                             Pointer to area to return tid.
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM0055_NONEXT, E_DM003C_BAD_TID,
**                                      E_DB0044_DELETED_TID, and errors
**                                      returned fro dm1b_next.
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
**
** History:
**	02-oct-86 (jennifer)
**          Created for Jupiter.
**	 7-nov-88 (rogerk)
**	    Send compression type to dm1c_get routine.
**	29-may-89 (rogerk)
**	    Check status from dm1c_get calls.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree Index Compression.
**	20-aug-1991 (bryanp)
**	    Merged in allocation project changes.
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Remove unused param's on dmpp_get calls.
**	      - Move compression out of dm1c layer, calling
**		tlv's here as required.
**	      - Avoid copy of row if already uncompressed into caller's buffer.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      06-jun-1996 (prida01)
**          There is no check for ambiguous replace for btree's on deferred
**          update. dm1r_get already checks. Add the code for btree's.
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Reposition if necessary.
**      12-dec-96 (dilma04)
**          If phantom protection is needed, set DM0P_PHANPRO fix action.
**      17-dec-96 (stial01)
**          dm1b_bybid_get() Adjust local leaf pointers after reposition
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      21-apr-97 (stial01)
**          If page locking we must clean the page before we adjust the bid.
**          Changed parameters.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - set DM1R_LK_CHECK flag if calling dm1r_lock_row() just to
**          make sure that we already have the row lock;
**          - request a physical lock if isolation level is CS or RR.
**      21-may-97 (stial01)
**          dm1b_bybid_get() pass leaf to dm1b_reposition
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          If error, break to error handling (unlock buffers must be done)
**      12-jun-97 (stial01)
**          dm1b_bybid_get() Pass tlv to dm1cxget instead of tcb.
**      04-aug-1999 (horda03)
**          Only report E_DM0047 (Ambiguous replace) if the current query
**          is not a READ access. Bug 83046.
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      04-aug-1999 (horda03)
**          Only report E_DM0047 (Ambiguous replace) if the current query
**          is not a READ access. Bug 83046.
**	16-mar-2004 (gupsh01)
**	    Modified dm1r_cvt_row to include adf control block in parameter
**	    list.
*/
DB_STATUS
dm1b_bybid_get(
    DMP_RCB         *rcb,
    i4		    operation,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS	    s;		/* Variable used for return status. */
    i4         fix_action;     /* Type of access to page. */
    i4         record_size;    /* Size of record retrieved. */
    i4		    row_version = 0;	
    DM_TID          localtid;       /* Tid used for getting data. */
    DM_TID          *bid = &r->rcb_fetchtid;
    DM_TID          *tid = &r->rcb_currenttid;
    char            *record = r->rcb_record_ptr;
    char	    *rec_ptr = record; 
    i4		    *err_code = &dberr->err_code;

    DM_TID	    save_currenttid;
    DM_TID	    crtid;

    CLRDBERR(dberr);

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;

    save_currenttid.tid_i4 = r->rcb_currenttid.tid_i4;

    for (;;)
    {
	if (r->rcb_other.page && bid->tid_tid.tid_page != 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page))
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, 
                        &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}
	if (r->rcb_other.page == NULL)
	{
            if (row_locking(r) && serializable(r))
	        if ((r->rcb_p_hk_type != RCB_P_EQ) ||
		    (r->rcb_hl_given != t->tcb_keys))
                  fix_action |= DM0P_PHANPRO; /* phantom protection is needed */

	    s = dm0p_fix_page(r, (DM_PAGENO  )bid->tid_tid.tid_page, 
                    fix_action | DM0P_NOREADAHEAD, &r->rcb_other, dberr);
            fix_action &= ~DM0P_PHANPRO;
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "rcb_other.page" is a CR page.
	*/
	dm0pLockBufForUpd(r, &r->rcb_other);

	/*
        ** Check to see if reposition is needed
	** dm1b_bybid_get is called when we refetch a row for delete/update
	** The following reposition assumes that we have already gotten and
	** locked the record to be deleted/updated.
	*/ 
retry:
	if (t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    s = btree_reposition(r, &r->rcb_other, bid,
		 r->rcb_fet_key_ptr, RCB_P_FETCH, 0, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Reposition may have changed leaf page fixed
	*/

	if ((i4)bid->tid_tid.tid_line >= 
	    (i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page))
	{
	    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    s = E_DB_ERROR;
	    break;
	}

	/* If CLUSTERED, the Leaf entry is the row at the BID */
	if ( t->tcb_rel.relstat & TCB_CLUSTERED )
	{
	    /*
	    ** Get the row at the indicated BID.  If it is compressed
	    ** then uncompress it into the caller's buffer.
	    */
	    localtid.tid_i4 = bid->tid_i4;
	    record_size = t->tcb_rel.relwid;

	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			    r->rcb_other.page,
			    &t->tcb_leaf_rac,
			    (i4)bid->tid_tid.tid_line, &rec_ptr,
			    (DM_TID*)NULL, (i4*)NULL,
			    &record_size,
			    (u_i4*)NULL, (u_i2*)NULL, r->rcb_adf_cb);


	    /*  Make sure we return CHANGED_TUPLE on an ambiguous replace. */
	    if ( r->rcb_access_mode != RCB_A_READ &&
		(r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r)) &&
		dm1cx_isnew(r, r->rcb_other.page, (i4)bid->tid_tid.tid_line) )
	    {
		SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
		s = E_DB_ERROR;
		break;
	    }
	}
	else
	{
	    /*
	    ** Extract the TID pointed to by the entry at the specified BID.
	    */
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    r->rcb_other.page, 
		    bid->tid_tid.tid_line, &localtid, (i4*)NULL);

	    dm0pUnlockBuf(r, &r->rcb_other);

	    /*
	    ** Consistency check
	    ** If crow_locking we already locked the row at rcb_currenttid
	    ** The TID pointed to by this entry should match rcb_currenttid 
	    ** otherwise we have an update conflict
	    ** (which probably should have been detected already)
	    */
	    if (crow_locking(r) && r->rcb_currenttid.tid_i4 != localtid.tid_i4)
	    {
		TRdisplay("dm1b_bybid_get consistency check error %~t %d,%d %d,5d\n",
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    r->rcb_currenttid.tid_tid.tid_page,
		    r->rcb_currenttid.tid_tid.tid_line,
		    localtid.tid_tid.tid_page, localtid.tid_tid.tid_line);
		if ( r->rcb_iso_level == RCB_SERIALIZABLE ||
		     r->rcb_state & RCB_CURSOR )
		{
		    SETDBERR(dberr, 0, E_DM0020_UNABLE_TO_SERIALIZE);
		}
		else
		    SETDBERR(dberr, 0, E_DM0029_ROW_UPDATE_CONFLICT);
		s = E_DB_ERROR;
		break;
	    }

	    r->rcb_currenttid.tid_i4 = localtid.tid_i4; 

	    /*
	    ** Fix the data page specified by the extracted TID (if it is
	    ** not already fixed.
	    */
	    if (r->rcb_data.page && localtid.tid_tid.tid_page !=
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page))
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	    if (r->rcb_data.page == NULL)
	    {
		s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page, 
		    fix_action | DM0P_NOREADAHEAD, &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if "rcb_data.page" is a CR page.
	    */
	    dm0pLockBufForUpd(r, &r->rcb_data);

	    /*
	    ** Make sure we already have the lock
	    ** BYBID get is done for delete/replace, in both cases we  
	    ** should already have the row locked
	    */

	    if ((row_locking(r) || crow_locking(r)) 
			&& ((t->tcb_rel.relstat & TCB_INDEX) == 0) &&
		DMZ_SES_MACRO(33))
	    {
		DB_STATUS     tmp_status;

		tmp_status = dm1r_check_lock(r, tid);
		if (tmp_status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
		    s = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Find the row at the indicated TID.  If it is compressed
	    ** then uncompress it into the caller's buffer.
	    */
	    record_size = t->tcb_rel.relwid;

	    s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, r->rcb_data.page,
		    &localtid, &record_size,
		    &rec_ptr, &row_version, NULL, NULL, (DMPP_SEG_HDR *)NULL);

	    /*  Make sure we return CHANGED_TUPLE on an ambiguous replace. */
	    if ((r->rcb_access_mode != RCB_A_READ) &&
		((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r)) &&
		(*t->tcb_acc_plv->dmpp_isnew)(r, r->rcb_data.page, 
			(i4)tid->tid_tid.tid_line)))
	    {
		SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
		s = E_DB_ERROR;
		break;
	    }

	    /* Additional processing if compressed, altered, or segmented */
	    if (s == E_DB_OK && 
		(t->tcb_data_rac.compression_type != TCB_C_NONE ||
		row_version != t->tcb_rel.relversion ||
		t->tcb_seg_rows))
	    {
		s = dm1c_get(r, r->rcb_data.page, &localtid, record, dberr);
		if (s && dberr->err_code != E_DM938B_INCONSISTENT_ROW)
		    break;
		rec_ptr = record;
	    }

	    /* If there are encrypted columns, decrypt the record */
	    if (s == E_DB_OK &&
		t->tcb_rel.relencflags & TCB_ENCRYPTED)
	    {
		s = dm1e_aes_decrypt(r, &t->tcb_data_rac, rec_ptr, record,
			r->rcb_erecord_ptr, dberr);
		if (s != E_DB_OK)
		    break;
		rec_ptr = record;
	    }
	}

	/*
	** If the row was found, return it to the caller.  Copy the row
	** into the caller's buffer (unless it was already uncompressed into
	** the buffer).
	*/
	if (s == E_DB_OK)
	{
	    tid->tid_i4 = localtid.tid_i4;
	    if (rec_ptr != record)
		MEcopy(rec_ptr, record_size, record);
	}
	else
	{
	    char    keybuf[DM1B_MAXSTACKKEY];
	    char    *KeyPtr, *AllocKbuf;
	    i4 	    tmpkey_len;

	    if ( s = dm1b_AllocKeyBuf(t->tcb_klen, keybuf, &KeyPtr, &AllocKbuf, dberr) )
		break;
	    tmpkey_len = t->tcb_klen;
	    _VOID_ dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page,
			&t->tcb_leaf_rac,
			bid->tid_tid.tid_line,
			&KeyPtr, (DM_TID *)NULL, (i4*)NULL,
			&tmpkey_len,
			NULL, NULL, r->rcb_adf_cb);
	    dm1b_badtid(r, bid, tid, KeyPtr);

	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

	    s = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	}
	break;
    } 

    dm0pUnlockBuf(r, &r->rcb_data);
    dm0pUnlockBuf(r, &r->rcb_other);

    if (s == E_DB_OK)
	return (E_DB_OK);

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM925F_DM1B_GETBYBID);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1b_delete - Deletes a record from a BTREE table.
**
** Description:
**	This routine deletes the record identified by tid from btree table.
**
**	It can be used for both Btree base tables and Secondary Indexes.
**	On Secondary Indexes only the Leaf page is updated; on Base Tables
**	the Data page is updated as well.
**
**	The pages described by BID and TID will be fixed into the 
**	rcb_other.page and rcb_data.page pointers, if they are not
**	already.  Except in the case when a data page is reclaimed (see
**	next paragraph), these pages will be left fixed upon return.
**
**	If the row deleted is the last row on a disassociated data page
**	then that data page is made FREE and put into the table's free
**	space maps.  The page remains locked and cannot actually be reused
**	by another transaction until this transaction commits.  The page
**	also unfixed by this routine.
**
**	This routine has been coded to handle the deletion of a record
**	which has been fetched, but has already been deleted or
**	replaced by another opener during this same transaction (cursor 
**	case).
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             The bid of the leaf entry of the
**					record to delete.
**      tid                             The tid of the record to delete.
**
** Outputs:
**      err_code			Error status code:
**					    E_DM003C_BAD_TID
**					    E_DM0044_DELETED_TID
**					    E_DM9260_DM1B_DELETE
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      21-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree Index Compression.
**      10-feb-92 (jnash)
**          Fix bug in check for previously deleted tid, correct header.
**	14-dec-92 (rogerk)
**	    Reduced Logging Project: most of routine rewritten.
**	      - Consolidated secondary index and base table delete code.
**	      - Changed to call dm1r_delete to delete data rows so that the
**		new delete logging code will be called.
**	      - Added new arguments to dm1bxdelete for logging changes.
**	      - Removed old mutex calls as this is now done inside dm1bxdelete.
**	      - Changed to fix pages into rcb page pointers rather than locally
**		since this was required to use dm1r_delete (for data pages
**		anyway) and because it looked like all callers currently had 
**		the correct pages fixed in the rcb anyway.
**	      - Removed unused 'flag' parameter.
**	      - Added dm1bxfree_ovfl to free up empty overflow pages.
**	28-dec-92 (mikem)
**	    Fixed bug introduced by above change.  Now that this routine calls
**	    dm1r_delete() to delete it no longer has to decrement tcb_tup_adds
**	    itself.  It was doing that causing the row count as returned by
**	    dmt_show to reflect twice the number of deletes to btrees (ie. both
**	    this routine and dm1r_delete() was decrementing the count).
**	18-jan-93 (rogerk)
**	    Fixed bug introduced by above change.  We need to make sure we
**	    increment tcb_tup_adds once and only once.  Since dm1r_delete is
**	    called for base tables, the tuple count is incremented for us.
**	    For secondary indexes however, we must update the count ourselves.
**	13-oct-93 (robf)
**          Only unfix other page pointer if its not null.
**	18-oct-1993 (kwatts) BUG #55844
**	    The check round the unfix call for other_page_ptr was the wrong 
**	    way round, causing us to try to unfix a null page pointer.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to dm1cxlength
**      06-may-1996 (thaju02 & nanpr01)
**          New Page Format Project. Modify page header references to macros.
**	    Subtract tuple header size in index entry length for leaf pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add arguments to dm1b_rcb_update call.
**          Do not reclaim space from delete if row locking
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          Space reclaim decision is made by dm1bxdelete()
**      21-apr-97 (stial01)
**          Copy page lsn,clean count before we possibly deallocate ovfl page
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set DM1R_LK_CHECK flag if calling dm1r_lock_row() just to
**          make sure that we already have the row lock.
**      21-may-97 (stial01)
**          dm1b_delete() Row locking: Reposition, adjust local bid,leaf ptrs
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      29-jul-97 (stial01)
**          dm1b_delete() Replaced rcb_update parameter with opflag parameter.
**          Pass original operation mode DM1C_DELETE to dm1r_delete for logging.
**	13-Apr-2006 (jenjo02)
**	    Clustered primary has no data pages.
*/

DB_STATUS
dm1b_delete(
    DMP_RCB	*rcb,
    DM_TID	*bid,
    DM_TID	*tid,
    i4     opflag,
    DB_ERROR	*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM_TID	    localbid;	    /* BID pointer to key entry to delete. */
    DM_TID	    localtid;	    /* TID of data record to delete. */
    DB_STATUS	    s;
    i4	    	    n;
    i4	    	    duplicate_keylen;
    char	    *duplicate_key;
    char	    *AllocKbuf = NULL, *KeyPtr;
    char	    key_buf[DM1B_MAXSTACKKEY];
    bool	    BaseTable, NeedDataPage;
    i4         	    reclaim_space;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    BaseTable = (t->tcb_rel.relstat & TCB_INDEX) == 0;
    NeedDataPage = (t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) ) == 0;
    localbid.tid_i4 = bid->tid_i4;
    localtid.tid_i4 = tid->tid_i4;

    for (;;)
    {
	if ((row_locking(r) || crow_locking(r)) 
		&& BaseTable && DMZ_SES_MACRO(33))
	{
	    DB_STATUS     tmp_status;

	    tmp_status = dm1r_check_lock(r, tid);
	    if (tmp_status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
		s = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Make sure we have the correct leaf and data pages fixed.
	*/
	if ((r->rcb_other.page == NULL) || 
	    (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    r->rcb_other.page) != localbid.tid_tid.tid_page))
	{
	    if (r->rcb_other.page != NULL)
	    {
		s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
		    dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    s = dm0p_fix_page(r, (DM_PAGENO )localbid.tid_tid.tid_page, 
		(DM0P_WRITE | DM0P_NOREADAHEAD), &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "rcb_other.page" is a CR page.
	*/
	dm0pLockBufForUpd(r, &r->rcb_other);

	/*
        ** Check to see if reposition is needed
	** dm1b_bybid_get is called when we refetch a row for delete/update
	** The following reposition assumes that we have already gotten and
	** locked the record to be deleted/updated.
	*/ 
	if (t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    s = btree_reposition(r, &r->rcb_other, bid, 
		r->rcb_fet_key_ptr, RCB_P_FETCH, 0, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/* Re-init local bid in case we repositioned */
	localbid.tid_i4 = bid->tid_i4;

	/*
	** Sanity check on the leaf page bid.
	*/
	if ((i4)localbid.tid_tid.tid_line >= 
	    (i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
				 r->rcb_other.page))
	{
	    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    break;
	}

	/*
	** Make sure we have the correct data page fixed, if needed.
	** Note requirement by dm1r_delete that the data page pointer must be
	** fixed into rcb_data.page and not locally.
	*/
	if ( NeedDataPage )
	{
	    if ( r->rcb_data.page == NULL || 
		(DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
		    r->rcb_data.page) != localtid.tid_tid.tid_page) )
	    {
		if (r->rcb_data.page != NULL)
		{
		    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, 
					dberr);
		    if (s != E_DB_OK)
			break;
		}

		s = dm0p_fix_page(r, (DM_PAGENO)localtid.tid_tid.tid_page, 
		    (DM0P_WRITE | DM0P_NOREADAHEAD), &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    /*
	    ** Verify that the row we're positioned at is the correct one to delete.
	    */
	    /*
	    ** Look on the leaf page to get the TID of the row to which our BID
	    ** points. If the returned tid does not match the one passed into 
	    ** this routine then we can't proceed.
	    **
	    ** This case occurs when we were positioned to a row which was 
	    ** deleted by another RCB on the same table (presumably owned by 
	    ** the same session).  When this occurs then we will be left 
	    ** positioned to the NEXT leaf entry.  Checking the TID pointer 
	    ** in the current leaf entry catches this.
	    */
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			r->rcb_other.page,
			localbid.tid_tid.tid_line, &localtid, (i4*)NULL);
	    if ( localtid.tid_i4 != tid->tid_i4 ) 
	    {
		SETDBERR(dberr, 0, E_DM0044_DELETED_TID);
		break;
	    }
	}

	if (DMZ_AM_MACRO(2))
	{
	    TRdisplay("dm1b_delete: (%d,%d) bid = %d,%d, tid = %d,%d. \n", 
		     t->tcb_rel.reltid.db_tab_base,
		     t->tcb_rel.reltid.db_tab_index,
		     localbid.tid_tid.tid_page, localbid.tid_tid.tid_line, 
		     localtid.tid_tid.tid_page, localtid.tid_tid.tid_line); 
	    TRdisplay("dm1b_delete: Leaf of deletion\n");
	    dmd_prindex(r, r->rcb_other.page, (i4)0);
	}

	/*
	** If the table is a (non-clustered) Base Table (not a Secondary Index), then delete
	** the data row.  Also check if the data page can then be reclaimed.
	*/
	if ( NeedDataPage )
	{
	    /*
	    ** Delete record in data page.
	    */
	    s = dm1r_delete(r, &localtid, (i4)0, opflag, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Check whether page can be reclaimed.  If the last row has just
	    ** been deleted from a disassociated data page, then we can add
	    ** the page back to the free space.  Note: this call may unfix the
	    ** data page pointer.
	    */
	    s = check_reclaim(r, &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Before deleting the key, save a copy of it so if we end up deleting
	** the last row from an overflow chain page, we can free the page.
	*/
	if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) & DMPP_CHAIN) && 
	    (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page)== 1))
	{
	    dm1cxrecptr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page,
			localbid.tid_tid.tid_line, &duplicate_key);
	    duplicate_keylen = t->tcb_klen;
	    if (DM1B_INDEX_COMPRESSION(r) == DM1CX_COMPRESSED)
	    {
		dm1cx_klen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page,
			localbid.tid_tid.tid_line, &duplicate_keylen);
	    }
	    if ( s = dm1b_AllocKeyBuf(duplicate_keylen, key_buf,
					&KeyPtr, &AllocKbuf, dberr) )
		break;
	    MEcopy(duplicate_key, duplicate_keylen, KeyPtr);
	    duplicate_key = KeyPtr;
	}

	/*
	** Delete the entry from the Btree Index.
	*/
	s = dm1bxdelete(r, &r->rcb_other, (DM_LINE_IDX)localbid.tid_tid.tid_line,
		(i4)TRUE, (i4)TRUE, &reclaim_space, dberr); 
	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(2))
	{
	    TRdisplay("dm1b_delete: Leaf of deletion after deletion.\n");
	    dmd_prindex(r, r->rcb_other.page, (i4)0);
	}

	dm0pUnlockBuf(r, &r->rcb_other);

	/*
	** If this delete just removed the last entry from a leaf overflow
	** page then we can free the page.  Note that freeing the page also
	** causes it to be unfixed from rcb_other.page.
	**
	** Note if row locking/crow_locking, the entry is just marked deleted
	*/
	if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) & DMPP_CHAIN) && 
	    (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
			r->rcb_other.page) == 0))
	{
	    s = dm1bxfree_ovfl(r, duplicate_keylen, duplicate_key,
				&r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/* Discard any allocated key space */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

	/*
	** Find any other RCB's whose table position pointers may have been
	** affected by this delete and update their positions accordingly.
	**
	*/
	if (opflag == DM1C_MDELETE)
	{
	    s = dm1b_rcb_update(r, &localbid, (DM_TID *)NULL, (i4)0,
			DM1C_MDELETE,
			reclaim_space, tid, (DM_TID *)NULL, (char *)NULL, dberr);
	}

#ifdef xDEV_TEST
	/*
	** Btree Crash Tests
	*/
	if (DMZ_CRH_MACRO(1) && ( ! BaseTable))
	{
	    TRdisplay("dm1b_delete: CRASH! Secondary Index, Leaf not forced\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(2) && ( ! BaseTable))
	{
	    TRdisplay("dm1b_delete: CRASH! Secondary Index, Leaf forced.\n");
	    (VOID)dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,&local_dberr);
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(3) && (BaseTable))
	{
	    TRdisplay("dm1b_delete: CRASH! Updates not forced.\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(4) && (BaseTable))
	{
	    TRdisplay("dm1b_delete: CRASH! Leaf forced.\n");
	    (VOID)dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,&local_dberr);
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** Secondary index handling:
	** For secondary index updates, we must decrement rcb_tup_adds to
	** reflect our delete.  For base tables, this was done by dm1r_delete.
	*/
	if ( t->tcb_rel.relstat & TCB_INDEX )
	    r->rcb_tup_adds--;

	return (E_DB_OK);
    }

    /* Discard any allocated key space */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    dm0pUnlockBuf(r, &r->rcb_other);

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9260_DM1B_DELETE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1b_get - Get a record from a BTREE table.
**
** Description:
**      This routine takes the high range bid and the low range bid
**      for the next record want to retieve or the TID of the actual 
**      record.  This routine also requires a flag indicating the
**      type of retrieval desired.
**      
**	A "cursor position" in a btree is a "bid", a "btree indentifier",
**	as opposed to a tid. A bid identifies a leaf page entry in a btree.
**	A bid has the same structure as a tid.	The tid_tid.tid_page component 
**	identifies a leaf page; the tid_tid.tid_line component is a line table 
**	position in the leaf.
**
**      This routine assumes that the following variables in the RCB are
**      for the use of scanning and will not be destroyed by other operations:
**      rcb->rcb_other.page      - A pointer to current leaf.
**      rcb->rcb_data.page       - A pointer to last data page assessed.
**      rcb->rcb_lowtid          - A pointer to current leaf (bid) position.
**      rcb->rcb_currentid       - A pointer to return actual tid of 
**                                 retrieved record.  If opflag is DM1C_BYTID
**                                 then this points to exact tid want to 
**                                 retrieve.
**      rcb->rcb_hl_ptr          - A key used to qualify retrieved records. 
**                                 all values must be less than or equal to
**                                 this key.
**      rcb->rcb_hl_given        - A value indicating how many attributes are
**                                 in the key used to qualify records.
**                                 (May be partial key for ranges).
** 
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      tid                             Tid of record to retrieve if opflag
**                                      is set to DM1C_BYTID.
**      opflag                          Value indicating type of retrieval.
**                                      Must be DM1C_GETNEXT, DM1C_GETPREV or
**                                      DM1C_BYTID.
**
** Outputs:
**      record                          Pointer to an area used to return
**                                      pointer to the record.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM0055_NONEXT, E_DM003C_BAD_TID,
**                                      E_DB0044_DELETED_TID, and errors
**                                      returned fro dm1b_next.
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
**          Traverses the tree fixing, unfixing and locking pages and
**          updating scan information in the RCB.
**
** History:
**      21-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	 7-nov-88 (rogerk)
**	    Send compression type to dm1c_get routine.
**	29-may-89 (rogerk)
**	    Added checks for bad rows in dm1c_get calls.  Added trace flags
**	    to allow us to delete rows by tid that cannot be fetched.
**	02-jan-89 (walt)
**	    If we're reading with nolock and find a leaf page entry with
**	    no corresponding data page tuple, just continue on to the next
**	    row.   Fixes bug #4775.
**	22-jan-90 (rogerk)
**	    Make sure we send byte-aligned tuples to qualification function.
**	    Use IFDEF for byte-aligment to determine whether to test for
**	    align restriction.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree Index Compression.
**	23-jul-91 (markg)
**	    Added fix for bug 38679 where an incorrect error was being 
**	    returned if there had been a failure while attempting to 
**	    qualify a tuple. In the event of a serious failure we now 
**	    write E_DM93A3_QUALIFY_ROW_ERROR to the error log and return 
**	    E_DM9261_DM1B_GET. In the event of a user error being detected 
**	    by the qualification function we write nothing to the error log
**	    and return E_DM0148_USER_QUAL_ERROR.
**	19-aug-91 (rogerk)
**	    Modified above fix to not log internal errors when the qualify
**	    function returns an error since we cannot distiguish between
**	    user errors and internal errors.  When the USER_ERROR return
**	    value from the qualification function becomes supported, we
**	    should change back to logging internal error messages.
**      23-oct-1991 (jnash)
**          Remove return code check from call to dmd_prrecord, caught by
**          LINT.
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Remove unused param's on dmpp_get calls.
**	      - Move compression out of dm1c layer, calling
**		tlv's here as required.
**	      - Avoid copy of row if already uncompressed into caller's buffer.
**       1-Jul-1993 (fred)
**          Added dm1c_pget() function where required to allow for the
**          rcb_f_qual function's being run on large objects.
**	 5-mar-1996 (nick)
**	    If fetching by tid on base relation, OR in DM0P_USER_TID. #57126
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine 
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**	    Give relatts correct type when calling function dm1c_pget.
**	    make sure rec_ptr pointing to record memory before calls to 
**	    dm1c_pget.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Row locking: reposition if necessary.
**          Row locking: request row locks;
**          If phantom protection is needed, set DM0P_PHANPRO fix action. 
**      12-dec-96 (stial01)
**          Get data record after getting row lock
**      22-jan-97 (dilma04)
**          - Fix bug 79961: release shared row locks if doing get by tid
**          and isolation level is read committed;
**          - change DM1C_BYTID_SI_UPD to DM1C_GET_SI_UPD to be consistent with
**          dm1r_get().
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
**      21-apr-97 (stial01)
**          dm1b_get() For primary btree, skip 'reserved' leaf entries.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - clear RCB_LPAGE_HAD_RECORDS and RCB_LPAGE_UPDATED when
**          moving to a new leaf page;
**          - set rcb_release_data flag, if E_DM0055_NONEXT is returned 
**          by next() or prev(); 
**          - removed checks for new_lock before calling dm1r_unlock_row(),
**          instead just check if row is locked;
**          - added support for releasing exclusive row locks;
**          - if row lock can be released before the end of transaction, set
**          rcb_release_row flag, but do not release the lock in this routine;
**          - pass DM1R_LK_CONVERT flag to dm1r_lock_row() when converting a
**          physical row lock to a logical lock;
**          - return E_DM9C2A_NOROWLOCK error code, if a new_lock is returned
**          by dm1r_lock_row() called with DM1R_LK_CONVERT flag.
**      21-may-97 (stial01)
**          dm1b_get: BYTID Secondary index access: removed blob processing 
**          since you cannot create index on long varchars anyway.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          Disallow get by tid on btree secondary index when row locking.
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed 
**          while we wait for row lock, just unlock the buffer.
**       06-jun-97 (stial01)
**          dm1b_get: BYTID base table: Get BYTID does not change btree leaf
**          position, so don't invalidate the position when row locking
**          by invalidating fet_clean_count.  QEF sometimes does positioned get,
**          followed by get BYTID, followed by positioned delete. 
**          The get BYTID should not affect the positioned get/delete.
**          (This happened from qeq_pdelete when deleting from referencing table
**      12-jun-97 (stial01)
**          dm1b_get() Pass tlv to dm1cxget instead of tcb.
**      29-jul-97 (stial01)
**         Skip 'reserved' leaf entries for page size != DM_COMPAT_PGSIZE 
**      29-sep-97 (stial01)
**         Lock data page/row before fixing data page. We may have to wait for 
**         lock on data page, which we shouldn't do if the leaf is mutexed.
**         Don't issue E_DM9C2A_NOROWLOCK if row LK_CONVERT fails
**         (limit exceeds trans lock limit->escalate->deadlock) is possible.
**         and we should not issue a different message when row locking.
**      12-nov-97 (stial01)
**          Remove 29-sep-97: 'Lock data page/row before fixing data page'
**          The page was locked again when it was fixed. If isolation level
**          is read committed the page is locked twice and unlocked once.
**          Instead, unlock previous row, unfix previous data page
**          and fix data page NOWAIT. If the lock is busy, unmutex the leaf
**          page, wait for the data page, reposition.
**          Consolidated rcb fields into rcb_csrr_flags.
**      17-mar-98 (stial01)
**          dm1b_get() Copy rec to caller buf BEFORE calling dm1c_pget (B86862)
**      07-jul-98 (stial01)
**          dm1b_get() B84213: Defer row lock request for committed data
**          Deleted extra dm1cxget for secondary indexes for getting tidp.
**          Tidp is in the rcb from the next/prev call.
**      20-Jul-98 (wanya01)
**          dm1b_get() Copy rec to called buf BEFORE calling dm1c_pget.
**          This is addtional changes for b86862. The previous changes was 
**          only made for routine when retrieving record by sequence, not 
**          when retrieving record by TID. 
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**	23-Sep-99 (thaju02)
**	    With read committed isolation level btree data page was 
**	    being unlocked prior to being unfixed, resulting in dbms 
**	    server crashing with E_DM9C01, E_DM9301. Removed block of 
**	    code that set RCB_CS_DATA bit in rcb_csrr_flags if cursor 
**	    stabitility and *err_code is nonext.  (b98766)
**      20-oct-99 (stial01)
**          dm1b_get() unlock buffer before unfixing,
**          Removed unecessary dm1r_rowlk_access before dm1c_pget
**      22-May-2000 (islsa01)
**          Bug 90402 : calling a function dm0p_check_logfull to see if
**          logfile is full. If the logfile is full and the event is force
**          abort, retain the existing error message. If the  logfile is not
**          full and the event is force abort, then report a new error message
**          called E_DM011F_TRAN_ABORTED.
**      04-aug-1999 (horda03)
**          Only report E_DM0047 (Ambiguous replace) if the current query
**          is not a READ access. Bug 83046.
**	06-Feb-2001 (jenjo02)
**	    Avoid doubly copying record from page to caller's buffer
**	    when dm1b_get-ting a table with extensions by tid.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in parameter
**          list.
**	05-Aug-2004 (jenjo02)
**	    Don't do BYTE_ALIGN record copy if rec_ptr == record;
**	    for indexes, that will be true, but "record_size"
**	    is uninitialized.
**      11-jan-2005 (horda03) Bug 111437/INGSRV2630
**          Don't fill in coupons for Extension tables when the output
**          record is a projected key column.
**	11-Nov-2005 (jenjo02)
**	    Removed bogus dm0p_check_logfull().
**	13-Apr-2006 (jenjo02)
**	    Gets for Clustered tables don't involve data pages.
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
**	11-Apr-2008 (kschendel)
**	    Row qualification mechanism updated to run in DMF context.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Updated pget call.
**	    rcb_row_version dropped, fix here.
*/
DB_STATUS
dm1b_get(
    DMP_RCB         *rcb,
    DM_TID          *tid,
    char	    *record,
    i4	    	    opflag,
    DB_ERROR        *dberr)
{
    ADF_CB	    *dummy;
    ADF_CB	    **qual_adfcb;	/* scb qfun-adfcb or dummy */
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DML_SCB	    *scb;
    DB_STATUS	    s;		/* Variable used for return status. */
    DB_STATUS       get_status;
    DB_STATUS       data_get_status;
    i4         	    fix_action;     /* Type of access to page. */
    i4         	    record_size;    /* Size of record retrieved. */
    i4         	    local_err_code; 
    DM_TID          localtid;       /* Tid used for getting data. */
    DM_TID	    temptid;
    DM_TID	    *bid;
    DM_TID          localbid;
    i4		    row_version = 0;		
    DM_TID	    tid_to_lock;
    DM_TID          save_tid;
    i4	    	    rec_len;
    char	    *rec_ptr = record;
    u_i4	    row_low_tran = 0;
    u_i2	    row_lg_id;
    DB_STATUS       lock_status;
    LK_LOCK_KEY     save_lock_key;
    i4         	    new_lock = FALSE;
    i4         	    lock_flag = 0;
    bool            reposition = TRUE;
    bool	    dm601;
    DM_TID          unlock_tid;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (r->rcb_rnl_online)
    {
	return (dm1b_online_get(r, tid, record, opflag, dberr));
    }

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;

    if (row_locking(r))
	MEfill(sizeof(save_lock_key), 0, &save_lock_key);

    dm601 = (DMZ_AM_MACRO(1) != 0);
    if (dm601)
    {
	s = TRdisplay("dm1b_get:  %p Starting tid = %d,%d,flag = %x\n", r,
                r->rcb_lowtid.tid_tid.tid_page, 
                r->rcb_lowtid.tid_tid.tid_line, 
                opflag); 
	s = TRdisplay("dm1b_get: limit key is\n");
	if (r->rcb_hl_given > 0)
	    dmd_prkey(r, r->rcb_hl_ptr, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	else 
	    s = TRdisplay("no limit key\n");
    }

    scb = NULL;
    qual_adfcb = &dummy;
    if (r->rcb_xcb_ptr)
    {
	scb = r->rcb_xcb_ptr->xcb_scb_ptr;
	qual_adfcb = &scb->scb_qfun_adfcb;
    }

    /*	Loop through records that don't pass the qualification. */
    for (s = OK; (opflag & DM1C_GETNEXT)  || (opflag & DM1C_GETPREV); )
    {
	/* Unlock data buffer in case next operation advances to next leaf */
	dm0pUnlockBuf(r, &r->rcb_data);

	if (r->rcb_other.page && r->rcb_lowtid.tid_tid.tid_page != 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page))
	{
	    dm0pUnlockBuf(r, &r->rcb_other);

	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;
	}
	if (r->rcb_other.page == NULL)
	{
            if (row_locking(r) && serializable(r))
	        if ((r->rcb_p_hk_type != RCB_P_EQ) ||
		    (r->rcb_hl_given != t->tcb_keys))
                  fix_action |= DM0P_PHANPRO; /* phantom protection is needed */

	    s = dm0p_fix_page(r, (DM_PAGENO)r->rcb_lowtid.tid_tid.tid_page, 
		fix_action, &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		break;

            fix_action &= ~DM0P_PHANPRO;

            r->rcb_state &= ~RCB_LPAGE_HAD_RECORDS;
            r->rcb_state &= ~RCB_LPAGE_UPDATED;
	}

	/* Check for external interrupts */
	if ( *(r->rcb_uiptr) )
	{

	    /* check to see if user interrupt occurred. */
	    if ( *(r->rcb_uiptr) & RCB_USER_INTR )
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    /* check to see if force abort occurred */
	    else if ( *(r->rcb_uiptr) & RCB_FORCE_ABORT )
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);

	    if ( dberr->err_code )
	    {
		s = E_DB_ERROR;
		break;
	    }
	}

	if ( !dm0pBufIsLocked(&r->rcb_other) )
	{
	    /*
	    ** Lock buffer for get.
	    **
	    ** This may produce a CR page and overlay "rcb_other.page"
	    ** with its pointer.
	    */
	    dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
	    if ( s == E_DB_ERROR )
		break;
	}

	/*
	** Check to see if reposition is needed 
	**    If record returned, reposition to last record returned
	**    else validate starting position.
	*/
	if (reposition && t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    s = btree_reposition(r, &r->rcb_other,
		    &r->rcb_lowtid, r->rcb_repos_key_ptr,
		    RCB_P_GET, opflag, dberr);
	    if (s != E_DB_OK)
		break;
	}
	reposition = FALSE;

	/* invoke either next() or prev() function to get record */

	/*
	** If a Global Index, "next"/"prev" will set rcb_partno
	** to the TID's partition number, needed for
	** row locking.
	*/
	if (dm601)
	{
	    TRdisplay("dm1b_get %p about to next/prev bid (lowtid) %d,%d\n",
		r,r->rcb_lowtid.tid_tid.tid_page,r->rcb_lowtid.tid_tid.tid_line);
	}
	if (opflag & DM1C_GETNEXT) 
	    s = next(r, &row_low_tran, &row_lg_id, opflag, dberr);
	else
	    s = prev(r, &row_low_tran, &row_lg_id, opflag, dberr);

	get_status = s;

	if (s == E_DB_WARN && t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    s = E_DB_OK;
	}

	if (s != E_DB_OK)
	    break;

	/*
	** row locking only saves new position after locking record
	** likewise for crow locking constraint/update cursor processing 
	*/
	if (!row_locking(r) && !NeedCrowLock(r))
	{
	    DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_lowtid, RCB_P_GET);
	}

	/*
	** Now that we are here, whether it is a secondary
        ** index or not, then the leaf is pointing to 
        ** the next index record of the next record to obtain.
	*/
	if (t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED))
	{
	    localtid.tid_i4 = r->rcb_lowtid.tid_i4;

	    /* Save tid to lock, tidp in leaf entry */
	    tid_to_lock.tid_i4 = r->rcb_currenttid.tid_i4;

	    /*
	    ** If Global Index, next/prev, above set rcb_partno
	    ** to TID's partition number.
	    */

	    /* Copy the key/row to the caller's record buffer */
	    MEcopy(r->rcb_record_ptr, t->tcb_rel.relwid, record);
	    rec_ptr = record;

	    /*
	    ** Since this may be a 5.0 converted secondary which did NOT 
	    ** have the TIDP as the last column of the key, ensure that
	    ** the resulting record contains a TIDP in the last column
	    ** by manufacturing one here.
	    **
	    ** Ignore this comment about TIDs if CLUSTERED base table, not SI;
	    ** tcb_klen -will- equal relwid.
	    */
	    if (t->tcb_klen != t->tcb_rel.relwid)
		MEcopy((char *)&r->rcb_currenttid, sizeof(DM_TID), 
			record + t->tcb_rel.relwid - sizeof(DM_TID));

	    r->rcb_currenttid.tid_i4 = localtid.tid_i4; 
	    tid->tid_i4 = localtid.tid_i4;
	}
	else
	{
	    /*
	    ** Normal get next (non-Index, non-CLUSTERED).
	    */

	    localtid.tid_i4 = r->rcb_currenttid.tid_i4;

	    /* Save tid to lock, tid of data record */
	    tid_to_lock.tid_i4 = localtid.tid_i4;

	    /*
	    **  Check for primary key projection (optimization)
	    */
	    if (opflag & DM1C_PKEY_PROJECTION)
	    {
		i4 i;

		/* Copy key to tuple buffer */
		for (i = 0; i < t->tcb_keys; i++)
		{
		    DB_ATTS             *atr;
		    atr = *(t->tcb_leafkeys + i);

		    MEcopy(r->rcb_s_key_ptr + atr->key_offset,
			atr->length, record + atr->offset);
		}
		tid->tid_i4 = localtid.tid_i4;
	    }
	    else
	    {
		/*
		** If we need a different data page, unfix the old one
		** before we lock the new page/row
		*/
		if (r->rcb_data.page &&
		    (localtid.tid_tid.tid_page !=
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_data.page)))
		{
		    /* Unlock row before we go to next page */
		    if (row_locking(r) && (save_lock_key.lk_type == LK_ROW))
		    {
			if (r->rcb_iso_level != RCB_SERIALIZABLE)
			    _VOID_ dm1r_unlock_row(r, &save_lock_key, &local_dberr);
			save_lock_key.lk_type = 0;
		    }

		    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
		    if (s != E_DB_OK)
			break;
		}

		/*
		** Try to keep leaf buffer locked while we fix the data page
		**   (or reposition will be necessary)
		**
		** Since this is not a Global Index, partition
		** number is of no consequence.
		*/
		if (row_locking(r) && (r->rcb_data.page == NULL))
		{
		    s = dm1b_rowlk_access(r, get_status, ROW_ACC_NEWDATA, 
				NULL, NULL, &tid_to_lock, (i4)0,
				&save_lock_key, opflag, dberr);
		    if (s == E_DB_WARN)
			continue;
		    if (s == E_DB_ERROR)
			break;
		}

		if (r->rcb_data.page == NULL) 
		{
		    if (row_locking(r))
			dmd_check(E_DM93F5_ROW_LOCK_PROTOCOL);
		    s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page, 
			fix_action, &r->rcb_data, dberr);
		    if (s != E_DB_OK)
			break;
		}	

		/*
		** Get primary btree data record
		*/
		record_size = r->rcb_proj_relwid;

		/*
		** Lock buffer for get.
		**
		** This may produce a CR page and overlay "rcb_data.page"
		** with its pointer.
		*/
		dm0pLockBufForGet(r, &r->rcb_data, &s, dberr);
		if ( s == E_DB_ERROR )
		    break;

		s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, r->rcb_data.page, &localtid, &record_size,
		    &rec_ptr, &row_version, &row_low_tran, 
		    &row_lg_id, (DMPP_SEG_HDR *)NULL); 

		data_get_status = s;

		/*
		** Skip deleted records
		*/
		if (data_get_status == E_DB_WARN)
		{
		    if (!rec_ptr ||
			DMPP_SKIP_DELETED_ROW_MACRO(r, r->rcb_data.page, row_lg_id, row_low_tran))
			continue;
		    else
			s = E_DB_OK; /* can't skip uncommitted delete yet */
		}

		/* Additional processing if compressed, altered, or segmented */
		if (s == E_DB_OK &&
		    (t->tcb_data_rac.compression_type != TCB_C_NONE ||
		    row_version != r->rcb_proj_relversion ||
		    t->tcb_seg_rows))
		{
		    s = dm1c_get(r, r->rcb_data.page, &localtid, record, dberr);
		    if (s && dberr->err_code != E_DM938B_INCONSISTENT_ROW)
			break;
		    rec_ptr = record;
		}

		if (s != E_DB_OK)
		{
		    /*
		    ** DM1C_GET or UNCOMPRESS returned an error - this indicates
		    ** that something is wrong with the tuple at the current 
		    ** tid.
		    **
		    ** If user is running with special PATCH flags, then take
		    ** appropriate action - skip the row or return garbage,
		    ** otherwise return with an error.
		    */

		    char	key_buf[DM1B_MAXSTACKKEY];
		    char	*KeyPtr, *AllocKbuf;
		    i4		key_len;
		    DB_STATUS   local_status;

		    /*
		    ** If we reach this point while reading nolock, just go on
		    ** to the next record.	
		    */
		    if (r->rcb_k_duration & RCB_K_READ)
			continue;

		    if ( s = dm1b_AllocKeyBuf(t->tcb_klen, key_buf, &KeyPtr,
						&AllocKbuf, dberr) )
			break;
		    key_len = t->tcb_klen;
		    local_status = dm1cxget(t->tcb_rel.relpgtype, 
			t->tcb_rel.relpgsize, r->rcb_other.page,
			&t->tcb_leaf_rac,
			r->rcb_lowtid.tid_tid.tid_line, &KeyPtr,
			(DM_TID *)NULL, (i4*)NULL,
			&key_len, NULL, NULL, r->rcb_adf_cb);
		    if (t->tcb_rel.relpgtype != TCB_PG_V1 
					&& get_status == E_DB_WARN
					&& local_status == E_DB_WARN)
		    {
			local_status = E_DB_OK;
		    }
		    if (local_status != E_DB_OK)
		    {
			dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r,
			    r->rcb_other.page,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			    r->rcb_lowtid.tid_tid.tid_line );
			KeyPtr = "<UNKNOWN KEY>";
			key_len = 13;
		    }
		    dm1b_badtid(r, &r->rcb_lowtid, &localtid, KeyPtr);

		    /* Discard allocated key buffer, if any */
		    if ( AllocKbuf )
			dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);
			
		    /*
		    ** If the user is running with special PATCH flag, then skip
		    ** this row and go on to the next.
		    */
		    if (DMZ_REC_MACRO(1))
		    {
			continue;
		    }

		    /*
		    ** If the user is running with special PATCH flag to ignore
		    ** errors, then continue anyway - setting localtid to be
		    ** entry 0 on the page so that at least we probably won't
		    ** AV - although we will be returning a garbage row.
		    */
		    if (DMZ_REC_MACRO(2))
		    {
			localtid.tid_tid.tid_line = 0;
		    }
		    else
		    {
			s = E_DB_ERROR;
			SETDBERR(dberr, 0, E_DM9350_BTREE_INCONSIST);
			break;
		    }
		}

		/* If there are encrypted columns, decrypt the record */
		if (t->tcb_rel.relencflags & TCB_ENCRYPTED)
		{
		    s = dm1e_aes_decrypt(r, &t->tcb_data_rac, rec_ptr, record,
				r->rcb_erecord_ptr, dberr);
		    if (s != E_DB_OK)
			break;
		    rec_ptr = record;
		}

		tid->tid_i4 = localtid.tid_i4;

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
		** NOTE:  Invoking the following rcb_f_qual
		** function may involve indirect recursion within
		** DMF (in that a qualification involving a large
		** object will go read some collection of the
		** extension tables).  Any internal concurrency
		** issues must take this into account.
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
		    ** checks on the tuple vs logged tuple. It is also bad 
		    ** to update the page without a mutex. (B86862)
		    */
		    if (rec_ptr != record)
		    {
			MEcopy(rec_ptr, record_size, record);
			rec_ptr = record;
		    }

		    s = dm1c_pget(t->tcb_atts_ptr,
				  r,
				  rec_ptr,
				  dberr);
		    if (s)
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				   (char *)NULL, (i4)0, (i4 *)NULL, err_code,
				   0);
			SETDBERR(dberr, 0, E_DM9B01_GET_DMPE_ERROR);
			break;
		    }
		}
	    } /* end get row from data page */
	} /* end get primary btree data record */
	    
	/* 
	** Check for caller qualification function.
	**
	** This qualification function requires that tuples start on aligned
	** boundaries.  If the tuple is not currently aligned, then we need to
	** make it so.
	**
	** THIS IS A TEMPORARY SOLUTION:  We check here if the tuple is aligned
	** on an ALIGN_RESTRICT boundary, and if not we copy it to the tuple
	** buffer, which we know is aligned.  But we only make this check if
	** the machine is a BYTE_ALIGN machine - since even on non-aligned
	** machines, ALIGN_RESTRICT is usually defined to some restrictive
	** value for best performance.  We do not want to copy tuples here when
	** we don't need to.  The better solution is to have two ALIGN_RESTRICT
	** boundaries, one for best performance and one which necessitates
	** copying of data.
	*/
#ifdef xDEBUG
	r->rcb_s_scan++;
#endif
	if (r->rcb_f_qual)
	{
	    ADF_CB *adfcb = r->rcb_adf_cb;

#ifdef BYTE_ALIGN
            /* lint truncation warning if size of ptr > int, but code valid */
	    if ( rec_ptr != record &&
	        (i4)rec_ptr & (i4)(sizeof(ALIGN_RESTRICT) - 1) )
	    {
		MEcopy(rec_ptr, record_size, record);
		rec_ptr = record;
	    }
#endif

#ifdef xDEBUG
	    r->rcb_s_qual++;
#endif

	    /*
	    ** If (c)row locking check if we need row locked before
	    ** applying NON KEY qualification
	    ** If opflag & DM1C_PKEY_PROJECTION, this is really just a key 
	    ** qualification
	    */
	    if ( !(opflag & DM1C_PKEY_PROJECTION) )
	    {
		if ( row_locking(r) )
		{
		    s = dm1b_rowlk_access(r, get_status, ROW_ACC_NONKEY, &row_low_tran,
				&row_lg_id, &tid_to_lock, r->rcb_partno,
				&save_lock_key, opflag, dberr);
		    if (s == E_DB_WARN)
			continue;
		    if (s == E_DB_ERROR)
			break;
		}
	    }

	    *qual_adfcb = adfcb;
	    /* Clean these in case non-ADF error */
	    adfcb->adf_errcb.ad_errclass = 0;
	    adfcb->adf_errcb.ad_errcode = 0;
	    *(r->rcb_f_rowaddr) = rec_ptr;
	    *(r->rcb_f_retval) = ADE_NOT_TRUE;
	    s = (*r->rcb_f_qual)(adfcb, r->rcb_f_arg);
	    *qual_adfcb = NULL;
	    if (s != E_DB_OK)
	    {
		s = dmf_adf_error(&adfcb->adf_errcb, s, scb, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	    if (*r->rcb_f_retval != ADE_TRUE)
	    {
		/* Row does not qualify, go to next one */
		continue;
	    }
	    /* Else row qualifies */
	}

	if (dm601)
	{
	    TRdisplay("dm1b_get: %p Ending bid = %d,%d,currenttid = %d,%d\n",
                r, r->rcb_lowtid.tid_tid.tid_page, 
                r->rcb_lowtid.tid_tid.tid_line, 
                localtid.tid_tid.tid_page, 
                localtid.tid_tid.tid_line); 
	    dmd_prrecord(r, rec_ptr);
	}

	/*
	** If (c)row locking, this row qualifies, make sure it is locked.
	** Convert physical to logical if repeatable read
	** Unlock the buffers
	** MVCC only needs the row lock before returning when
	** the get is part of constraint processing or if this is an 
	** update cursor.
	*/
	if (row_locking(r) || NeedCrowLock(r))
	{
	    s = dm1b_rowlk_access(r, get_status, ROW_ACC_RETURN, &row_low_tran, 
			&row_lg_id, &tid_to_lock, r->rcb_partno,
			&save_lock_key, opflag, dberr);
	    if (s == E_DB_WARN)
		continue;
	    if (s == E_DB_ERROR)
		break;
	}

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
	if ( r->rcb_internal_req == 0 && dmf_svcb->svcb_status & SVCB_IS_SECURE )
	{
	    i4       access;
	    i4	     compare;

	    access = DMA_ACC_SELECT;
	    /* 
	    ** If get by exact key tell DMA this
	    */
	    if (  (r->rcb_ll_given) && 
		  (r->rcb_ll_given == r->rcb_hl_given) &&
		  (r->rcb_ll_given == t->tcb_keys) &&
		  (r->rcb_ll_op_type == RCB_EQ) &&
		  (r->rcb_hl_op_type == RCB_EQ) 
	       )			   
		    access |= DMA_ACC_BYKEY;
		   
	    s = dma_row_access(access, r, (char*)rec_ptr, (char*)NULL,
			&compare, dberr);
	    if (s != E_DB_OK)
		break;
	    if (compare != DMA_ACC_PRIV)
	    {
		continue;
	    }
	}

	/*
	** Copy the row to the caller's buffer if has not already been
	** copied there.  It should have already been copied:
	**     If the table is compressed
	**     If the table is a secondary index
	**     If the table is a CLUSTERED base table
	**     If byte-align restrictions required us to copy it before
	**         making adf compare calls.
	**     If row locking and we had to copy it before unmutexing the page
	*/
	if (rec_ptr != record)
	    MEcopy(rec_ptr, record_size, record);

	dm0pUnlockBuf(r, &r->rcb_data);

	if (row_locking(r)
	    && (r->rcb_iso_level == RCB_CURSOR_STABILITY)
	    && (r->rcb_state & RCB_CSRR_LOCK)
	    && (r->rcb_row_lkid.lk_unique))
	{
	    r->rcb_csrr_flags |= RCB_CS_ROW;
	}

	if (dm0pNeedBufLock(r, &r->rcb_other) && !dm0pBufIsLocked(&r->rcb_other))
	{
	    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
	    s = E_DB_ERROR;
	    break;
	}

	/* Check if position for rcb_lowtid was saved (it should have been) */
	if (r->rcb_pos_info[RCB_P_GET].bid.tid_i4 != r->rcb_lowtid.tid_i4)
	{
	    TRdisplay("DM1B_POSITION_CHECK %~t (%d,%d) (%d,%d)\n",
		t->tcb_relid_len, r->rcb_tcb_ptr->tcb_rel.relid.db_tab_name,
		r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_page,
		r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_line,
		r->rcb_lowtid.tid_tid.tid_page,
		r->rcb_lowtid.tid_tid.tid_line);
	    DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_lowtid, RCB_P_GET);
	}

	dm0pUnlockBuf(r, &r->rcb_other);

	r->rcb_state &= ~RCB_FETCH_REQ;
	r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;

	/* Maintain additional rcb_fet* only for update cursors */
	if (fix_action & DM0P_WRITE)
	{
	    DM1B_POSITION_SET_FETCH_FROM_GET(r);
	    if (DMZ_AM_MACRO(18))
		TRdisplay("setting fetch postiion\n");
	}

	return (E_DB_OK); 
    }

    if (s != E_DB_OK)
    {
	dm0pUnlockBuf(r, &r->rcb_data);
	dm0pUnlockBuf(r, &r->rcb_other);

	/* Unlock row if error */
	if (row_locking(r) && save_lock_key.lk_type == LK_ROW
	    && r->rcb_iso_level == RCB_SERIALIZABLE)
	{
	    _VOID_ dm1r_unlock_row(r, &save_lock_key, &local_dberr);
	}

	/*      Handle error reporting and recovery. */
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
	}
	return (E_DB_ERROR);
    }

    /*	Must be a get by TID. */
    
#ifdef xDEBUG
	r->rcb_s_scan++;
#endif

    for (;;)
    {
	/*	26-jun-87 (rogerk)
	**	If retrieve by tid, check to see if the tuple has already been
	**	touched by the current cursor/statement, or if the tuple has
	**	been moved.
	**	If retrieve by tid and fix_page returns error, return BAD_TID.
	**	If retrieve by tid and page is not data page, return BAD_TID.
	**	If retrieve by tid on secondary index or Clustered Table
	**	and page is not leaf page, return BAD_TID.
	*/
	if ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) )
	{
	    if ( t->tcb_rel.relstat & TCB_CLUSTERED )
	    {
		s = VerifyHintTidAndKey(r, tid, r->rcb_hl_ptr, 
					record, &localtid, dberr);
		if ( s )
		    break;
	    }
	    else
	    {
		/* Index: */
		localtid.tid_i4 = tid->tid_i4;

		/*
		** DM1C_GET_SI_UPD 
		**
		** Get by tid on secondary btree index is no longer 
		** done when updating secondary indexes. The leaf entry is  
		** located during the search, so the get by tid is unecessary.
		** 
		** If we did do a get by tid after the search we would need
		** reposition logic after we fix the leaf, since the leaf is 
		** unmutexed between the position and the get.
		**
		** The usefulness of get by tid on a secondary btree is very
		** questionable, because leaf entries can shift.
		** If row locking we shall return an error, because the results 
		** are always unpredictable in that case.
		** 
		*/
		if (opflag & DM1C_GET_SI_UPD)
		{
		    dmd_check(E_DM93F5_ROW_LOCK_PROTOCOL);
		}
		else
		{
		    if (set_row_locking(r))
		    {
			/* fix_action |= DM0P_LOCKREAD; */
			TRdisplay("Get by tid on secondary btree/cluster with row lock\n");
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
			break;
		    }
		    DM1B_POSITION_INVALIDATE_MACRO(r, RCB_P_FETCH);
		}

		/* If wrong leaf fixed, unfix it, fix the right one */
		if (r->rcb_other.page && tid->tid_tid.tid_page != 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
				r->rcb_other.page))
		{
		    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
		    if (s != E_DB_OK)
			break;
		}
		if (r->rcb_other.page == NULL)
		{
		    s = dm0p_fix_page(r, (DM_PAGENO)tid->tid_tid.tid_page, 
			fix_action | DM0P_NOREADAHEAD, &r->rcb_other, dberr);
		    if (s != E_DB_OK)
		    {
			if (dberr->err_code > E_DM_INTERNAL)
			    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
			break;
		    }
		}

		/*
		** If page is not a leaf page, or if not that many tids on the
		** page, then return BAD_TID.
		*/
		if (((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_other.page)  & (DMPP_LEAF | DMPP_CHAIN)) == 0) ||
		    ((i4)tid->tid_tid.tid_line >= 
		    (i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_other.page)))
		{
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		    break;
		}

		/*
		** Retrieve the key and tid from this index entry. Since this may
		** be a 5.0 converted secondary which did NOT have the TIDP as the
		** last column of the key, ensure that the resulting record contains
		** a TIDP in the last column by manufacturing one here.
		**
		** Regarding deleted entries:
		**     The tuple should exist when get by TID is specified
		*/
		rec_ptr = record;
		rec_len = t->tcb_klen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			    r->rcb_other.page,
			    &t->tcb_leaf_rac,
			    localtid.tid_tid.tid_line,
			    &rec_ptr, &temptid, (i4*)NULL,
			    &rec_len,
			    &row_low_tran, &row_lg_id, r->rcb_adf_cb);

		if (s != E_DB_OK )
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, r->rcb_other.page,
				    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				    localtid.tid_tid.tid_line );
		    break;
		}

		/* 
		** If this RCB is in deferred update mode and this record indicates
		** it is new, then this must be a bad tid.
		*/
		if ((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))&& 
		    dm1cx_isnew(r, r->rcb_other.page, (i4)bid->tid_tid.tid_line))
		{
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		    break;
		}

		if (rec_ptr != record)
		    MEcopy(rec_ptr, rec_len, record);
		if ( t->tcb_rel.relstat & TCB_INDEX && rec_len != t->tcb_rel.relwid )
		    MEcopy((char *)&temptid, sizeof(DM_TID), 
			    record + t->tcb_rel.relwid - sizeof(DM_TID));
	    }

	    r->rcb_currenttid.tid_i4 = localtid.tid_i4; 
	    tid->tid_i4 = localtid.tid_i4;
	    r->rcb_state &= ~RCB_FETCH_REQ;
	
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
	    if ( r->rcb_internal_req == 0 && dmf_svcb->svcb_status & SVCB_IS_SECURE )
	    {
		i4 compare;
		s = dma_row_access(DMA_ACC_SELECT|DMA_ACC_BYTID, r,
			(char*)rec_ptr, (char*)NULL, &compare, dberr);

		if ((s != E_DB_OK) || (compare != DMA_ACC_PRIV))
		{
			uleFormat(NULL, E_DM938A_BAD_DATA_ROW, (CL_ERR_DESC *)NULL, 
				ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				err_code, 4, sizeof(DB_DB_NAME), 
				&t->tcb_dcb_ptr->dcb_name, sizeof(DB_TAB_NAME), 
				&t->tcb_rel.relid, sizeof(DB_OWN_NAME), 
				&t->tcb_rel.relowner, 0, localtid.tid_i4);
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
			return (E_DB_ERROR);
		}
	    }

	    return (E_DB_OK);
	}

	/*  Neither a secondary index nor a Clustered Table. */

	localtid.tid_i4 = tid->tid_i4;
	tid_to_lock.tid_i4 = localtid.tid_i4;

	if (r->rcb_data.page == NULL) 
	{
	    s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page, 
		(fix_action | DM0P_USER_TID), &r->rcb_data, dberr);
	    if (s != E_DB_OK)
	    {
		if (dberr->err_code > E_DM_INTERNAL)
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		break;
	    }
	}	
	else if (localtid.tid_tid.tid_page !=
 	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
	    r->rcb_data.page))
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_fix_page(r, (DM_PAGENO )localtid.tid_tid.tid_page, 
		(fix_action | DM0P_USER_TID), &r->rcb_data, dberr);
	    if (s != E_DB_OK)
	    {
		if (dberr->err_code > E_DM_INTERNAL)
		    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
		break;
	    }
	}

	record_size = r->rcb_proj_relwid;

	/*
	** If row locking, request lock
	** We do this after fixing the page to make sure we first acquire
	** an intent lock on the page
	*/
	if (row_locking(r))
	{
	    /* Not a SI, so rcb_partno is of no consequence */
	    lock_status  = dm1r_lock_row(r, 
				r->rcb_iso_level == RCB_CURSOR_STABILITY ?
				(i4)(DM1R_LK_PHYSICAL) : 0,
				&tid_to_lock, 
				&new_lock, &save_lock_key, dberr);

	    if (lock_status == E_DB_ERROR)
		return (E_DB_ERROR);
	}

	/*
	** Lock buffer for get.
	**
	** This may produce a CR page and overlay "rcb_data.page"
	** with its pointer.
	*/
	dm0pLockBufForGet(r, &r->rcb_data, &s, dberr);
	if ( s == E_DB_ERROR )
	    break;

	/*
	** Make sure the tid is valid.
	** If the tid specifies an invalid position on the page,
	**   return BAD_TID.
	** If this is not a data page, return BAD_TID.
	** If the tuple has been changed, moved or removed by the same
	**   cursor/statement, return CHANGED_TUPLE.
	** If the tuple at tid used to exist, but has been moved or removed
	**   by a previous cursor/statement, return DELETED_TID.
	*/
	if (((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		r->rcb_data.page) & DMPP_DATA) == 0) || 
		(localtid.tid_tid.tid_line >= 
	    (u_i2)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, 
		r->rcb_data.page)))
	{
	    SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    break;
	}

	if (s == E_DB_OK)
	{
	    s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype, 
		t->tcb_rel.relpgsize, r->rcb_data.page, &localtid, &record_size,
		&rec_ptr, &row_version, &row_low_tran, 
		&row_lg_id, (DMPP_SEG_HDR *)NULL);
	}

	if ((r->rcb_access_mode != RCB_A_READ) &&
	    ((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r)) && 
	    (*t->tcb_acc_plv->dmpp_isnew)(r, r->rcb_data.page, 
		(i4)localtid.tid_tid.tid_line)))
	{
	    SETDBERR(dberr, 0, E_DM0047_CHANGED_TUPLE);
	    s = E_DB_ERROR;
	    break;
	}

	if (s == E_DB_WARN)
	{
	    SETDBERR(dberr, 0, E_DM0044_DELETED_TID);
	    break;
	}

	/* Additional get processing if compressed, altered, or segmented */
	if (s == E_DB_OK &&
	    (t->tcb_data_rac.compression_type != TCB_C_NONE ||
	    row_version != r->rcb_proj_relversion ||
	    t->tcb_seg_rows))
	{
	    s = dm1c_get(r, r->rcb_data.page, &localtid, record, dberr);
	    if (s && dberr->err_code != E_DM938B_INCONSISTENT_ROW)
		break;
	    rec_ptr = record;
	}

	/* If there are encrypted columns, decrypt the record */
	if (s == E_DB_OK &&
	    t->tcb_rel.relencflags & TCB_ENCRYPTED)
	{
	    s = dm1e_aes_decrypt(r, &t->tcb_data_rac, rec_ptr, record,
			r->rcb_erecord_ptr, dberr);
	    if (s != E_DB_OK)
		break;
	    rec_ptr = record;
	}

	if (s == E_DB_OK)
	{
	    /*
	    ** Row found at TID.  Copy to row into the callers buffer
	    ** (unless it was already uncompressed into the buffer).
	    */
	    if (rec_ptr != record)
	    {
		MEcopy(rec_ptr, record_size, record);
		rec_ptr = record;
	    }

	    dm0pUnlockBuf(r, &r->rcb_data);

	    tid->tid_i4 = localtid.tid_i4;
	    r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
	    r->rcb_currenttid.tid_i4 = localtid.tid_i4;
	    r->rcb_state &= ~RCB_FETCH_REQ;

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
	    ** NOTE:  Invoking the following rcb_f_qual
	    ** function may involve indirect recursion within
	    ** DMF (in that a qualification involving a large
	    ** object will go read some collection of the
	    ** extension tables).  Any internal concurrency
	    ** issues must take this into account.
            **
            ** Don't get coupon details if this is a projected key
            ** column. If we ever support extensions in indices
            ** (e.g. as data columns, then the coupon details
            ** should be added in the dm1r_cvt_rows when getting
	    */
	    
	    if ( (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) &&
                 !r->rcb_proj_misc )
	    {
                if (rec_ptr != record)
                {
                  MEcopy(rec_ptr, record_size, record);
                  rec_ptr = record;
                }

		s = dm1c_pget(t->tcb_atts_ptr,
			      r,
			      rec_ptr,
			      dberr);
		if (s)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			       (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9B01_GET_DMPE_ERROR);
		    break;
		}
	    }
	    
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
	    if ( r->rcb_internal_req == 0 && dmf_svcb->svcb_status & SVCB_IS_SECURE )
	    {
		i4 compare;
		s = dma_row_access(DMA_ACC_SELECT|DMA_ACC_BYTID, r,
			(char*)rec_ptr, (char*)NULL, &compare, dberr);

		if ((s != E_DB_OK) || (compare != DMA_ACC_PRIV))
		{
			uleFormat(NULL, E_DM938A_BAD_DATA_ROW, (CL_ERR_DESC *)NULL, 
				ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				err_code, 4, sizeof(DB_DB_NAME), 
				&t->tcb_dcb_ptr->dcb_name, sizeof(DB_TAB_NAME), 
				&t->tcb_rel.relid, sizeof(DB_OWN_NAME), 
				&t->tcb_rel.relowner, 0, localtid.tid_i4);
			SETDBERR(dberr, 0, E_DM003C_BAD_TID);
			break;
		}
	    }

            if (row_locking(r) &&  
                r->rcb_iso_level == RCB_CURSOR_STABILITY && 
                r->rcb_state & RCB_CSRR_LOCK && 
                r->rcb_row_lkid.lk_unique)
            {
                r->rcb_csrr_flags |= RCB_CS_ROW;
            }

	    return (E_DB_OK);
	}
	else
	{
	    /*
	    ** DM1C_GET returned an error - this indicates that
	    ** something is wrong with the tuple at the current tid.
	    **
	    ** If user is running with special PATCH flag, then return
	    ** with garbage row, but at least no error.  This should allow
	    ** us to do a delete by tid.
	    */
	    dm0pUnlockBuf(r, &r->rcb_data);
	    if (DMZ_REC_MACRO(2))
	    {
		tid->tid_i4 = localtid.tid_i4;
		r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
		r->rcb_state &= ~RCB_FETCH_REQ;
		return (E_DB_OK);
	    }
	    dm1c_badtid(rcb, tid);
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    if (DMZ_REC_MACRO(1))
		SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    return (E_DB_ERROR);
	}
    } 

    dm0pUnlockBuf(r, &r->rcb_data);
    dm0pUnlockBuf(r, &r->rcb_other);

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1b_put - Puts a record into a BTREE table.
**
** Description:
**	This routine puts a record into a BTREE table. 
**
**	It can be used for both Btree base tables and Secondary Indexes.
**	On Secondary Indexes only the Leaf page is updated; on Base Tables
**	the Data page is updated as well.
**
**	The BID parameter specifies the position in the Leaf Page to add 
**	the new key and the TID parameter specifies the position Data page
**	where the actual record is to be put (for base tables only). The 
**	Leaf and Data pointers must fix the pages identified by the passed 
**	in BID and TID values.  These pages will be left fixed upon completion
**	of this routine.
**
**	If the table uses data compression, then the supplied 'record' must
**	have already been compressed.
**
**	The supplied 'key' is expected to NOT be compressed even if key
**	compression is used.
**
**	The 'tid' parameter is required as input only for base tables. While
**	the TID of the related base table row is required by inserts to 
**	secondary indexes, the value is computed from the secondary index
**	row rather than passed as a parameter.
**
**	On exit, the 'tid' parameter holds the spot at which the new row
**	was insert.  For base tables this is always the same value was was
**	passed in.  For secondary indexes the 'tid' value returned is actually
**	the BID at which the secondary index key was insert.
**	
**      This routine leaves its supplied page pointers fixed upon exit
**	to optimize for set appends which cause consective keys to be updated.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	bid                             The BID indicating where the
**                                      index record is to be placed.
**      tid                             The TID of the record to insert.
**                                      Only set for data pages.
**      record                          Pointer to the record to insert.
**      record_size                     Size in bytes of record. 
**      key                             Pointer to the key for index.
**
** Outputs:
**      tid				Tid of the record for base tables;
**					Bid of the key for secondary indices.
**      err_code			Error status code:
**					E_DM9262_DM1B_PUT
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      22-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	20-dec-1990 (bryanp)
**	    Added support for Btree index compression:
**	    Check return code from dm1bxinsert(). Pass new arguments
**	    to dm1bxinsert.
**	14-dec-92 (rogerk)
**	    Reduced Logging Project: most of routine rewritten.
**	    Changed to call dm1r_put to insert data rows so that the logging
**	    code will be called.
**	    Added new arguments to dm1bxinsert for logging changes.  Removed
**	    old mutex calls as this is now done inside dm1bxinsert.
**	28-dec-92 (mikem)
**	    Fixed bug introduced by above change.  Now that this routine calls
**	    dm1r_put() to do inserts it no longer has to increment tcb_tup_adds
**	    itself.  It was doing that causing the row count as returned by
**	    dmt_show to be twice the number of inserts (ie. both this routine
**	    and dm1r_put() was incrementing the count).
**	18-jan-93 (rogerk)
**	    Fixed bug introduced by above change.  We need to make sure we
**	    increment tcb_tup_adds once and only once.  Since dm1r_put is
**	    called for base tables, the tuple count is incremented for us.
**	    For secondary indexes however, we must update the count ourselves.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add arguments to dm1b_rcb_update call.
**      21-may-97 (stial01)
**          dm1b_put() Row locking: Reposition, adjust local bid,leaf ptrs
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      29-jul-1997 (stial01)
**          dm1b_put() Replaced rcb_update parameter with opflag parameter
**          Pass original operation mode DM1C_PUT to dm1r_put for logging.
**	13-Apr-2006 (jenjo02)
**	    Clustered primaries don't do data pages.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: must dm0p_lock_buf_macro when crow_locking
**	    to ensure the Root is locked.
*/
DB_STATUS
dm1b_put(
    DMP_RCB	    *rcb,
    DMP_PINFO	    *leafPinfo,
    DMP_PINFO	    *dataPinfo,
    DM_TID	    *bid,
    DM_TID	    *tid,
    char	    *record,
    char	    *key,
    i4	    record_size,
    i4         opflag,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO        *tbio = &t->tcb_table_io;
    i4		n;
    DB_STATUS		s;
    bool		base_table;
    bool		NeedDataPage;
    i4			tid_partno = 0;
    DM_TID8		tid8;
    DB_ERROR	        local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE		**leaf, **data;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    data = &dataPinfo->page;

    NeedDataPage = (t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED)) == 0;

    /*
    ** For secondary indexes, we need to calculate the TID of the
    ** actual data row being inserted to the base table.  This TID is
    ** needed for the tid portion of the (key,tid) pair in the leaf update.
    */ 
    if ( t->tcb_rel.relstat & TCB_INDEX )
    {
	/*
	** Extract the TID value out of the secondary index row,
	** and the partition number if Global Index.
	*/
	if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    MEcopy((char*)(record + record_size - sizeof(DM_TID8)),
			sizeof(DM_TID8), (char*)&tid8);
	    tid->tid_i4 = tid8.tid_i4.tid;
	    tid_partno = tid8.tid.tid_partno;
	}
	else
	    MEcopy((char*)(record + record_size - sizeof(DM_TID)),
			sizeof(DM_TID), (char*)&tid->tid_i4);
    }

    for (;;)
    {
	/*
	** Verify that the correct leaf is fixed for this insert.
	*/
	if ((*leaf == NULL) || 
	    (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) != 
	    bid->tid_tid.tid_page))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, bid->tid_tid.tid_page, 0, 
		(*leaf ? DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
		*leaf): -1), 0, bid->tid_i4, 0, tid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9262_DM1B_PUT);
	    break;
	}

	/*
	** Do the insertion by placing the key on the leaf page and
	** the record on the data page.
	*/

	/*
	** Insert the record to the leaf's associated data page (if this
	** is a non-CLUSTERED base table).
	*/
	if ( NeedDataPage )
	{
	    s = dm1r_put(r, dataPinfo, tid, record, (char*)NULL, record_size, 
			(i4)DM1C_MPUT, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "leaf" is a CR page.
	*/
	dm0pLockBufForUpd(r, leafPinfo);

	/*
	** If a leaf entry was reserved, check to see if reposition is needed
	*/ 
	if (DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC))
	{
	    s = btree_reposition(r, leafPinfo, bid, key,
		    RCB_P_ALLOC, 0, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** For secondary indices, CLUSTERED, and data records, update the BTREE index.
	*/
	s = dm1bxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    &t->tcb_leaf_rac,
		    t->tcb_klen, leafPinfo, 
		    (t->tcb_rel.relstat & TCB_CLUSTERED) ? record : key, 
		    tid, tid_partno,
		    (DM_LINE_IDX)bid->tid_tid.tid_line, (i4)TRUE, 
		    (i4)TRUE, dberr); 
	if (s != E_DB_OK)
	    break;

	dm0pUnlockBuf(r, leafPinfo);

	if (opflag == DM1C_MPUT)
	{
	    s = dm1b_rcb_update(r, bid, (DM_TID *)NULL, (i4)0,
			DM1C_MPUT,
			(i4)0, tid, (DM_TID *)NULL, (char *)NULL, dberr);
	}

	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(3))
	{
	    TRdisplay("dm1b_put: (%d,%d) bid = %d,%d, tid = %d,%d, parnot = %d.\n", 
		 t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
		 bid->tid_tid.tid_page, bid->tid_tid.tid_line, 
		 tid->tid_tid.tid_page, tid->tid_tid.tid_line,
		 tid_partno); 
	    TRdisplay("dm1b_put: Leaf after insertion\n");
	    dmd_prindex(r, *leaf, (i4)0);
	}
	
#ifdef xDEV_TEST
	/*
	** Btree Crash Tests
	*/
	if (DMZ_CRH_MACRO(6))
	{
	    TRdisplay("dm1b_put: CRASH! Updates not forced.\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(5))
	{
	    TRdisplay("dm1b_put: CRASH! Leaf page forced.\n");
	    (VOID)dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,&local_dberr);
	    CSterminate(CS_CRASH, &n);
	}
#endif

	if (DMZ_AM_MACRO(18) && (t->tcb_rel.relstat & TCB_CLUSTERED) == 0)
	{
	    /*
	    ** trace point dm618:
	    ** test incr/decr key here
	    ** Incr/decr key is only done before adding overflow pages
	    ** Test the incr/decr here for any key inserted!
	    */
	    dm1b_dbg_incr_decr(r, key);
	}

	/*
	** Secondary index, CLUSTERED  handling:
	**
	** dm1b_put returns the TID of the just-inserted record.  For secondary
	** indexes and CLUSTERED we return the BID of the entry on the leaf page.
	**
	** For secondary index updates, we must increment rcb_tup_adds to
	** reflect our insert.  For base tables, this was done by dm1r_put.
	*/
	if ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) )
	{
	    tid->tid_i4 = bid->tid_i4; 
	    if ( t->tcb_rel.relstat & TCB_INDEX )
		r->rcb_tup_adds++;
	}

	return(E_DB_OK); 
    }

    dm0pUnlockBuf(r, leafPinfo);

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9262_DM1B_PUT);
    }
    return (E_DB_ERROR);
}



/*{
** Name: dm1b_bulk_put - Puts record(s) into a BTREE table.
**
** Description:
**	This routine puts the record(s) in rcb_bulk_misc into a BTREE table. 
**      It is used to efficiently put records into a BTREE blob extension table.
**      There are two cases where we see benefits:
**
**      1) BLOBLOGGING (default)
**      Inserting blob segments onto new disassociated btree data pages
**      allows us to do page allocations without starting a mini transaction.
**
**      2) nojournaling, NOBLOBLOGGING
**      Inserting blob segments onto new disassociated btree data pages
**      is the protocol used to guarantee that the etab table structure
**      is maintained in the event of rollback when nobloblogging.
**      NOTE when NOBLOBLOGGING, page allocations are logged, leaf inserts
**      are logged but data page inserts are not logged. 
**      Since the new disassociated data page will be updated only by 
**      the transaction that allocated the page, it is always safe to 
**      rollback the page allocation.
**
**	The BID parameter specifies the position in the Leaf Page to add 
**	the new key and the TID parameter specifies the position Data page
**	where the actual record is to be put (for base tables only). The 
**	Leaf and Data pointers must fix the pages identified by the passed 
**	in BID and TID values.  These pages will be left fixed upon completion
**	of this routine.
**
**	If the table uses data compression, then the supplied 'record' must
**	have already been compressed.
**      Currently dm1b_bulk_put is only supported for uncompressed etabs.
**
**	The supplied 'key' is expected to NOT be compressed even if key
**	compression is used.
**
**	The 'tid' parameter is required as input only for base tables. While
**	the TID of the related base table row is required by inserts to 
**	secondary indexes, the value is computed from the secondary index
**	row rather than passed as a parameter.
**
**	On exit, the 'tid' parameter holds the spot at which the new row
**	was insert.  For base tables this is always the same value was was
**	passed in.  For secondary indexes the 'tid' value returned is actually
**	the BID at which the secondary index key was insert.
**	
**      This routine leaves its supplied page pointers fixed upon exit
**	to optimize for set appends which cause consective keys to be updated.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	bid                             The BID indicating where the
**                                      index record is to be placed.
**      tid                             The TID of the record to insert.
**                                      Only set for data pages.
**      record                          Pointer to the record to insert.
**      record_size                     Size in bytes of record. 
**      key                             Pointer to the key for index.
**
** Outputs:
**      tid				Tid of the record for base tables;
**					Bid of the key for secondary indices.
**      err_code			Error status code:
**					E_DM9262_DM1B_PUT
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      02-jan-2004 (stial01)
**          Created.
*/
DB_STATUS
dm1b_bulk_put(
    DMP_RCB	    *rcb,
    DMP_PINFO	    *leafPinfo,
    DMP_PINFO	    *dataPinfo,
    DM_TID	    *bid,
    DM_TID	    *tid,
    char	    *record,
    char	    *key,
    i4	    record_size,
    i4         opflag,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO        *tbio = &t->tcb_table_io;
    DB_STATUS		s;
    DMPE_RECORD		dmpe_rec;
    char		*rec_ptr;
    i4			line;
    i4			dmpe_size;
    char		*dmpe_key;
    DMPP_PAGE		*misc_data;
    DM_TID		local_tid;
    DM_TID		local_bid;
    i4			compression_type;
    DM_PAGENO		data_pageno;
    DM_LINE_IDX		pos;
    DM_TID		search_tid;
    i4			search_partno;
    i2			TidSize;
    bool		correct_leaf;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE		**leaf, **data;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    data = &dataPinfo->page;

    /* Extract TID size from leaf page */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, *leaf);

    /* process each row in rcb_bulk_misc data page */
    misc_data = (DMPP_PAGE *)(r->rcb_bulk_misc + 1);

    STRUCT_ASSIGN_MACRO(*bid, local_bid);
    STRUCT_ASSIGN_MACRO(*tid, local_tid);
    data_pageno = DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *data);
    local_tid.tid_tid.tid_page = data_pageno; 
    compression_type = DM1B_INDEX_COMPRESSION(r);
#ifdef xDEBUG
    TRdisplay("dm1b_bulk_put r->rcb_bulk_cnt %d\n", r->rcb_bulk_cnt);
#endif

    for (line = 0; line < r->rcb_bulk_cnt; line++)
    {
	rec_ptr = (char *)&dmpe_rec;
	local_tid.tid_tid.tid_line = line;

	s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, misc_data, &local_tid, &dmpe_size,
		&rec_ptr, NULL, NULL, NULL, (DMPP_SEG_HDR *)NULL);

	if (t->tcb_data_rac.compression_type != TCB_C_NONE)
	    (*t->tcb_acc_plv->dmpp_reclen)(t->tcb_rel.relpgtype,
	     t->tcb_rel.relpgsize, misc_data, local_tid.tid_tid.tid_line, 
	     &dmpe_size);
	else
	    dmpe_size = t->tcb_rel.relwid;

	/* For etabs, the key is leading part of row */
	dmpe_key = (char *)rec_ptr;

	if (line != 0)
	{
	    /*
	    ** Need to allocate another leaf entry
	    ** Since prd_segment0 and prd_segment1 are part of the key,
	    ** the correct leaf for this segment may not be the 
	    ** same as the correct leaf for the previous segment
	    */
	    s = dm1b_compare_range( r, *leaf, dmpe_key, &correct_leaf,
			dberr);
	    if (s != E_DB_OK)
		break;
	    if (correct_leaf && 
		dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			*leaf, compression_type, (i4)100 /* fill factor */,
			t->tcb_kperleaf, (t->tcb_klen + 
			t->tcb_leaf_rac.worstcase_expansion + TidSize)) )
	    {
		/*
		** Correct leaf but need to find position
		** We cannot just increment local_bid.tid_tid.tid_line
		** as this does not take into consideration the
		** presence of duplicate deleted keys that may
		** exist when called from dmpe_replace
		*/
		s = dm1bxsearch(r, *leaf, DM1C_OPTIM, DM1C_EXACT, dmpe_key,
			t->tcb_keys, &search_tid, &search_partno, 
			&pos, NULL, dberr);
		/*
		** This call may return NOPART if the leaf is empty 
		** in that case it will return with lineo set to first position
		*/
		if (DB_FAILURE_MACRO(s) && dberr->err_code == E_DM0056_NOPART)
		    s = E_DB_OK;
		if (s!= E_DB_OK)
		    break;
		bid->tid_tid.tid_line = pos;
		local_bid.tid_tid.tid_line = pos;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(local_bid, *bid);
		s = btree_search(r, dmpe_key, 
			t->tcb_keys, DM1C_SPLIT, DM1C_EXACT,
			leafPinfo, bid, &search_tid, NULL, dberr);
		if (s != E_DB_OK)
		    break;
		STRUCT_ASSIGN_MACRO(*bid, local_bid);
	    }
	}

	/* Insert the row */
	s = dm1b_put(rcb, leafPinfo, dataPinfo, &local_bid, &local_tid, rec_ptr,
			dmpe_key, dmpe_size, opflag, dberr);

#ifdef xDEBUG
	TRdisplay("bulk put seg(%d,%d) line %d bid (%d,%d) tid (%d,%d)\n", 
	    ((DMPE_RECORD *)rec_ptr)->prd_segment0,
	    ((DMPE_RECORD *)rec_ptr)->prd_segment1,
	    line,
	    local_bid.tid_tid.tid_page, local_bid.tid_tid.tid_line,
	    local_tid.tid_tid.tid_page, local_tid.tid_tid.tid_line);
#endif

	if (s != E_DB_OK)
	    break;
    }

    if (s == E_DB_OK)
	return (E_DB_OK);

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9262_DM1B_PUT);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1b_replace - Replaces a record in a BTREE table.
**
** Description:
**	This routine replaces a record identified by tid from btree
**	table.  This routine assumes that the new space has already 
**	been allocated and that duplicates have been checked.
**
**	It can be used for both Btree base tables and Secondary Indexes.
**	On Secondary Indexes only the Leaf page is updated; on Base Tables
**	the Data page is updated as well.
**
**	If the 'inplace_udpate' flag is specified, then the routine assumes
**	that only the data row needs to be updated.  No change is made to
**	the btree index.  This is legal only when the udpate is only to
**	non-keyed columns of the table and the row does not need to move
**	to satisfy the replace.
**
**	The BID parameter specifies the position in the Leaf Page to add 
**	the new key and the TID parameter specifies the position Data page
**	where the actual record is to be put (for base tables only). The 
**	Leaf and Data pointers must fix the pages identified by the passed 
**	in BID and TID values.  Except in the case when a data page is
**	reclaimed (see next paragraph) these pages will be left fixed 
**	upon completion of this routine.
**
**	If the row replaced is the last row on a disassociated data page
**	and is moved off of that page, then that data page is made FREE 
**	and put into the table's free space maps.  The page remains locked 
**	and cannot actually be reused by another transaction until this 
**	transaction commits.  The page is also unfixed by this routine.
**
**	If the table uses data compression, then the supplied 'record' must
**	have already been compressed.
**
**	The supplied 'key' is expected to NOT be compressed even if key
**	compression is used.
**
**	If bid and newbid indicate the same leaf page, then the caller need
**	not fix a page into the 'newleaf' parameter (the parameter may point
**	to null, but may not be null itself).  Ditto for tid, newtid and
**	newdata.
**
*******************************************************************************
**	The following comment was in the original version of this file.
**	Its meaning is somewhat unclear, but is left here for archeological
**	reasons - perhaps someday it will shed light on some odd behaviour.
**
**	   "One other case must be handled by this routine. Since it 
**	    is possible that a record to be replaced has already been 
**	    replaced in a previous call to replace for the current replace
**	    transaction. In that case, if the new record exists and has 
**	    been added in the current transaction, it is assumed 
**	    optimistically that the present old record(rather than some 
**	    other old record was replaced previously to the current new 
**	    record. Otherwise, the old record was changed to something else, 
**	    and the statement is ambiguous."
**
**	I think this means that it should be OK for a query plan to generate
**	two identical dmr_replace requests (both replaces of the same row
**	to the same new value) during processing of a single update statement.
**	This would presumably occur when some cartesion product was built
**	during Query Processing and no sort is done to eliminate duplicates.
**	This comment seems to say that if two replaces are encountered in
**	the same statement that update the same row to the same new value that
**	no error should be generated.  But if two replaces are encountered in
**	the same statement that update the same row to different new values
**	then an ambiguous replace error should be generated.  In any case,
**	there seems to be no handling of this case in this code.
*******************************************************************************
**
** Inputs:
**      rcb				Record Control Block.
**      bid				Position of old entry on leaf page.
**      tid				Position of old record (if base table).
**      record				A pointer to an area containing
**					the record to replace.
**      record_size			Size of old record in bytes.
**      newbid				Position of new entry on leaf page.
**      newtid				Position of new record (if base table).
**      nrec				A pointer to an area containing
**					the new record.
**      nrec_size			Size of new record.
**      newkey				Pointer to newkey of nrec.
**
** Outputs:
**      err_code			Error status code:
**					    E_DM9263_DM1B_REPLACE 
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**      21-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	28-apr-1990 (rogerk)
**	    Verify that correct page is fixed into rcb_other_page_ptr.
**	    Added new error message to identify bug of having wrong
**	    page fixed.
**	23-jan-1991 (bryanp)
**	    Added support for Btree index compression: pass new arguments to
**	    dm1bxinsert().
**      23-oct-1991 (jnash)
**          Remove return code check from call to dmd_prrecord, caught by
**          LINT.
**	14-dec-1992 (rogerk & jnash)
**	    Reduced Logging Project: most of routine rewritten.
**	      - Added inplace_update parameter.
**	      - Consolidated secondary index and base table replace code and
**		removed the old sreplace() routine.
**	      - Changed to call dm1r_replace to update data pages so that the
**		new logging code will be called.
**	      - Added new arguments to dm1bxinsert, dm1bxdelete for logging
**		changes.
**	      - Removed old mutex calls done in this routine.
**	      - Removed calls to dmd_prrecord when records are compressed.
**	      - Added dm1bxfree_ovfl to free up empty overflow pages.
**	17-dec-1992 (rogerk)
*	    Fixed AV caused by use of 'leafpage' instead of 'leaf' when
**	    checking for deletion of duplicate key entries.
**	    Fixed braindead problem with checking for fixed data page on
**	    a btree secondary index.
**	06-mar-1996 (stial01 for bryanp)
**	    Multiple page size support: pass page_size to dm1cxlength.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**	    Subtract tuple header size in index entry length for leaf pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add arguments to dm1b_rcb_update call.
**          Do not reclaim space from delete if row locking
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          Space reclaim decision is made by dm1bxdelete()
**      21-apr-97 (stial01)
**          Copy page lsn,clean count before we possibly deallocate ovfl page
**      29-aug-97 (stial01)
**          dm1b_replace() Fixed condition for TRdisplay
**	01-oct-1998 (nanpr01)
**	    Update performance improvement.
**	13-Apr-2006 (jenjo02)
**	    Clustered tables don't have data pages.
*/
DB_STATUS
dm1b_replace(
    DMP_RCB         *rcb,
    DM_TID          *bid,
    DM_TID          *tid,
    char            *record,
    i4         	    record_size,
    DMP_PINFO	    *newleafPinfo,
    DMP_PINFO	    *newdataPinfo,
    DM_TID          *newbid,
    DM_TID          *newtid,
    char            *nrec,
    i4         	    nrec_size,
    char            *newkey,
    i4	    	    inplace_update,
    i4         	    delta_start,
    i4	    	    delta_end,
    DB_ERROR        *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DM_TID		datatid;
    i4			data_partno = 0;
    DM_LINE_IDX		insert_position;
    DB_STATUS		s;
    i4			n;
    i4			duplicate_keylen;
    char		*duplicate_key;
    char		*KeyPtr, *AllocKbuf = NULL;
    char		key_buf[DM1B_MAXSTACKKEY];
    bool		NeedDataPage;
    i4			reclaim_space;
    DM_TID              save_bid;
    DM_TID8		tid8;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE		**newleaf, **newdata;

    CLRDBERR(dberr);

    newleaf = &newleafPinfo->page;
    newdata = &newdataPinfo->page;

    NeedDataPage = ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) ) == 0;

    /*
    ** For secondary indexes, we need to calculate the TID of the
    ** actual data row being inserted to the base table.  This TID is
    ** needed for the tid portion of the (key,tid) pair in the leaf update.
    */ 
    if ( NeedDataPage )
	datatid = *newtid;
    else if ( t->tcb_rel.relstat & TCB_INDEX )
    {
	/*
	** Extract the TID value out of the new secondary index row,
	** and the partitioin number if Global Index.
	*/
	if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    MEcopy((char*)(nrec + nrec_size - sizeof(DM_TID8)),
			sizeof(DM_TID8), (char*)&tid8);
	    datatid.tid_i4 = tid8.tid_i4.tid;
	    data_partno = tid8.tid.tid_partno;
	}
	else
	{
	    MEcopy((char*)(nrec + nrec_size - sizeof(DM_TID)),
			sizeof(DM_TID), (char*)&datatid.tid_i4);
	}
    }
    else /* must be CLUSTERED */
	datatid = *newbid;

    if (DMZ_AM_MACRO(4))
    {
	TRdisplay("dm1b_replace: oldtid (%d,%d), newtid (%d,%d), partno %d\n", 
		tid->tid_tid.tid_page, tid->tid_tid.tid_line,
		datatid.tid_tid.tid_page, datatid.tid_tid.tid_line,
		data_partno); 
	if (t->tcb_data_rac.compression_type == TCB_C_NONE)
	{
	    s = TRdisplay("dm1b_replace: Old record. ");
	    dmd_prrecord(r, record);
	}
    }

    for (;;)
    {

	if (row_locking(r) && NeedDataPage && DMZ_SES_MACRO(33))
	{
	    DB_STATUS     tmp_status;

	    tmp_status = dm1r_check_lock(r, tid);
	    if (tmp_status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
		s = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Verify that correct leaf and data page pointers are fixed if
	** they are required:
	**
	**     Old leaf page - must be fixed in rcb_other.page
	**     New leaf page - must be fixed in newleaf
	**           - unless INPLACE replace requires no index update
	**
	**     Old data page - must be fixed in rcb_data.page
	**           - unless table is a secondary index 
	**	       or CLUSTERED base table
	**
	**     New data page - must be fixed in newdata
	**           - unless table is a secondary index OR
	**	       CLUSTERED base table
	**           - unless old and new TIDs point to the same page
	*/

	if (!inplace_update)
	{
	    /*
	    ** Check to see if reposition is needed
	    ** We might need to reposition here...
	    ** if the allocate caused a page split or page clean
	    **
	    ** dm1b_bybid_get is called when we refetch a row for delete/update
	    ** The following reposition assumes that we have already gotten and
	    ** locked the record to be deleted/updated.
	    */ 
	    save_bid.tid_i4 = bid->tid_i4;
	    if (t->tcb_rel.relpgtype != TCB_PG_V1)
	    {
		s = btree_reposition(r, &r->rcb_other, bid, 
		    r->rcb_fet_key_ptr, RCB_P_FETCH, 0, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	}

	if (( ! inplace_update) && 
	    ((r->rcb_other.page == NULL) || 
	    (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page) != 
	    bid->tid_tid.tid_page)))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, bid->tid_tid.tid_page, 0, (r->rcb_other.page ? 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page) : -1), 
		0, bid->tid_i4, 0, tid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
	    break;
	}

	if (( ! inplace_update) && 
	    ((*newleaf == NULL) || 
	    (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf) != 
	    newbid->tid_tid.tid_page)))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, newbid->tid_tid.tid_page, 0, 
		(*newleaf ? 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf) : -1), 
		0, newbid->tid_i4, 0, newtid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
	    break;
	}

	if ( NeedDataPage &&
	    ((r->rcb_data.page == NULL) || 
	    (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page) != 
	    tid->tid_tid.tid_page)))
	{
	    uleFormat(NULL, E_DM9396_BT_WRONG_PAGE_FIXED, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		0, tid->tid_tid.tid_page, 0, (r->rcb_data.page ? 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page) : -1), 
		0, bid->tid_i4, 0, tid->tid_i4);
	    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
	    break;
	}

	/*
	** Update the record on the data pages (if this is a non-CLUSTERED base table).
	** The dm1r_replace call handles inplace replaces as well as those
	** which move the row across data pages.
	*/
	if ( NeedDataPage )
	{
	    s = dm1r_replace(r, tid, record, record_size, newdataPinfo, newtid, 
			     nrec, nrec_size, (i4)0, 
			     delta_start, delta_end, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Check whether page can be reclaimed.  If the last row has just
	    ** been deleted from a disassociated data page (and we are not
	    ** about to add a new row to the same page), then we can add
	    ** the page back to the free space.  Note: this call may unfix the
	    ** data page pointer.
	    */
	    if (tid->tid_tid.tid_page != newtid->tid_tid.tid_page)
	    {
		s = check_reclaim(r, &r->rcb_data, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	}

	/*
	** If an index update is required (not inplace replace) then delete
	** the old leaf entry and insert the new key into its new spot.
	*/
	if ( ! inplace_update || t->tcb_rel.relstat & TCB_CLUSTERED )
	{
	    /* Cluster may well be replacing on the same leaf, same row */
	    if ( t->tcb_rel.relstat & TCB_CLUSTERED && !*newleaf )
		*newleafPinfo = r->rcb_other;

	    if (DMZ_AM_MACRO(4))
	    {
		TRdisplay("dm1b_replace: Leaf of deletion\n");
		dmd_prindex(r, r->rcb_other.page, (i4)0);
		if (*newleaf)
		{
		    TRdisplay("dm1b_replace: Leaf of insertion\n");
		    dmd_prindex(r, *newleaf, (i4)0);
		}
	    }

	    /*
	    ** Before deleting the key, save a copy of it so if we end up 
	    ** deleting the last row from an overflow chain page, we can 
	    ** free the page.
	    */
	    if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) & DMPP_CHAIN) && 
		(DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			r->rcb_other.page) != 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf)) && 
		(DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
			 r->rcb_other.page) == 1))
	    {
		dm1cxrecptr(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page, 
			bid->tid_tid.tid_line, &duplicate_key);
		duplicate_keylen = t->tcb_klen;
		if (DM1B_INDEX_COMPRESSION(r) == DM1CX_COMPRESSED)
		{
		    dm1cx_klen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page, bid->tid_tid.tid_line, &duplicate_keylen);
		}
		if ( s = dm1b_AllocKeyBuf(duplicate_keylen, key_buf,
					    &KeyPtr, &AllocKbuf, dberr) )
		    break;
		MEcopy(duplicate_key, duplicate_keylen, KeyPtr);
		duplicate_key = KeyPtr;
	    }

	    s = dm1bxdelete(r, &r->rcb_other, (DM_LINE_IDX)bid->tid_tid.tid_line,
		    (i4)TRUE, (i4)TRUE, &reclaim_space, dberr);
	    if (s != E_DB_OK)
		break;

#ifdef xDEV_TEST
	    /*
	    ** Btree Crash Tests
	    */
	    if (DMZ_CRH_MACRO(7))
	    {
		(VOID) dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,&local_dberr);
		TRdisplay("dm1b_replace: CRASH! Mid Update, Old Leaf forced\n");
		CSterminate(CS_CRASH, &n);
	    }
#endif

	    /*
	    ** Possibly adjust the BID at which to insert the new key.
	    ** If the new and old keys both end up on the same leaf page,
	    ** we must adjust newbid to account for deletion of old bid. 
	    **
	    ** This is not necessary if space is not reclaimed from the delete
	    */
	    insert_position = newbid->tid_tid.tid_line;
	    if ((DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newleaf) == 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page)) && 
		(newbid->tid_tid.tid_line > bid->tid_tid.tid_line))
	    {
		if (reclaim_space)
		    insert_position--;
	    }

	    /* CLUSTERED insert is the whole row */
	    s = dm1bxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			&t->tcb_leaf_rac,
			t->tcb_klen, 
			newleafPinfo, 
			(t->tcb_rel.relstat & TCB_CLUSTERED) ? nrec : newkey, 
			&datatid, data_partno,
			insert_position, 
			(i4)TRUE, (i4)TRUE, dberr); 
	    if (s != E_DB_OK)
		break;

	    if (DMZ_AM_MACRO(4))
	    {
		TRdisplay("dm1b_replace: Leaf after deletion\n");
		dmd_prindex(r, r->rcb_other.page, (i4)0);
		if (*newleaf)
		{
		    TRdisplay("dm1b_replace: Leaf after insertion\n");
		    dmd_prindex(r, *newleaf, (i4)0);
		}
	    }

	    /*
	    ** If this delete just removed the last entry from a leaf overflow
	    ** page then we can free the page.  This check must be done after		    
	    ** the insert call since the insert may be to the same page as the
	    ** delete. Note that freeing the page also causes it to be unfixed
	    ** from rcb_other.page.
	    **
	    ** Note if row locking, the entry is just marked deleted
	    */
	    if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) & DMPP_CHAIN) && 
		(DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
			 r->rcb_other.page) == 0))
	    {
		s = dm1bxfree_ovfl(r, duplicate_keylen, duplicate_key,
				&r->rcb_other, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

	}

	/*
	** Find any other RCB's whose table position pointers may have been
	** affected by this replace and update their positions accordingly
	** (and/or any RCB's which must refetch the updated row).
	**
	** Note that this is called with the original "newbid" value rather
	** than the possibly-adjusted "insert_position" value.  
	** The rcb_update code takes same-page replaces into account
	** in its calculations.
	*/
	s = dm1b_rcb_update(r, bid, newbid, (i4)0, DM1C_MREPLACE,
				reclaim_space, tid, newtid, newkey, dberr);
	if (s != E_DB_OK)
	    break;

#ifdef xDEV_TEST
	/*
	** Btree Crash Tests
	*/
	if (DMZ_CRH_MACRO(8))
	{
	    if ( ! inplace_update)
	    {
		(VOID) dm0p_unfix_page(r,DM0P_FORCE,&r->rcb_other,&local_dberr);
		(VOID) dm0p_unfix_page(r,DM0P_FORCE,newleafPinfo,&local_dberr);
	    }
	    TRdisplay("dm1b_replace: CRASH! Leafs forced\n");
	    CSterminate(CS_CRASH, &n);
	}
	if (DMZ_CRH_MACRO(9))
	{
	    TRdisplay("dm1b_replace: CRASH! Updates not forced\n");
	    CSterminate(CS_CRASH, &n);
	}
#endif

	return (E_DB_OK); 
    } 

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    /*
    ** Handle error reporting and recovery.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1b_rcb_update - Updates all RCB's of a single transaction.
**
** Description:
**
**  When a leaf page gets updated (PUT/DELETE/UPDATE/SPLIT/DEALLOC), 
**  dm1b_rcb_update() gets called to update leaf entry position
**  for all cursors (RCBs) positioned on the affected leaf page.
**
**      rcb_lowtid:   Current leaf entry position
**      rcb_fetchtid: Fetched record leaf entry position
**
**  When page locking, only other rcb's OF THIS TRANSACTION may need to 
**  be adjusted, since the page lock guarentees that only the transaction 
**  updating the leaf page may be positioned on that leaf page.
**
**  When row locking, rcb's for other sessions, possibly in other servers 
**  might need to be adjusted.
**
**  The 'active' approach to updating the position in a btree compares leaf
**  entry positions (bids) and adjusts the bids with the assumption
**  that page is locked and that bid has to adjusted only for the 
**    current operation (put/delete/replace/split/dealloc).
**  The 'active' approach does not work when row locking.
**
**      Consider the following:
**
**      Transaction-A                    Transaction-B
**      -----------------                ----------------
**      declare c1 cursor for 
**      select k from t where k > 1
**      fetch k into :i4var
**                                       Insert into t (k) values (1)        
**
**      delete from c1 where k = 0
**
**  Assume that there is only 1 leaf page.
**  When Transaction-A does the delete, it cannot correctly adjust the
**  bid for cursor c1 without also taking into account the insert that was
**  done by Transaction-B. 
**
**  When page locking, 'Same leaf entry' is determined by comparing bid.
**  When row locking, 'Same leaf entry' can only be determined by
**  comparing the key/tidp, as some operation from another transaction
**  may have invalidated our position.
**
**  Which RCB update?
**  -----------------
**
**    active_rcb_update:  For tables with TCB_PG_V1.
**
**    passive_rcb_update: For tables with page type != TCB_PG_V1. 
**
**    If page type != TCB_PG_V1, space reclamation = FALSE when
**    deleting leaf entries, and passive_rcb_update is used to maintain
**    btree cursor position. Passive rcb update requires that deletes
**    DO NOT reclaim space. If another cursor is currently positioned 
**    on the leaf entry we are deleting, we will need to reposition 
**    to the deleted entry at GET-NEXT time.
** 
**    Note passive_rcb_update will only work if space reclamation = FALSE.
**    Therefore passive_rcb_update is only supported for tables having
**    the new page format where leaf entries have tuple headers and
**    entries can be marked deleted.
**
**    Passive rcb update is the only method that will work when row locking.
**    When page locking, the active or passive method will work.
**    However which rcb update is dependent on page size not lock mode
**    because of special cases that arise due to lock escalation and the 
**    possibility of different lock modes for different cursors.
**    Whichever method is selected for a table, it must be consistently
**    used for the entire transaction for all open cursors.
**    For this reason we select the method based on page size, even
**    though the passive rcb update may incur more overhead when
**    page locking.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             A pointer to a TID indicating 
**                                      position on leaf page for
**                                      index record changed.
**      newbid                          A pointer to a TID indicating
**                                      position on leaf page for
**                                      replaces which move the record.
**                                      Or the newbid after a split.
**      opflag                          A value indicating type of operation
**                                      which changed an index record. One of:
**                                          DM1C_MDELETE
**                                          DM1C_MPUT
**                                          DM1C_MREPLACE
**                                          DM1C_MSPLIT
**                                          DM1C_MDEALLOC
**
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
**      08-jul-1997 (stial01)
**          Made wrapper to active_rcb_update, passive_rcb_update.
*/
DB_STATUS
dm1b_rcb_update(
    DMP_RCB        *rcb,
    DM_TID         *bid_input,
    DM_TID         *newbid_input,
    i4        split_pos,
    i4        opflag,
    i4        del_reclaim_space,
    DM_TID         *tid,
    DM_TID         *newtid,
    char           *newkey,
    DB_ERROR       *dberr)
{
    DMP_RCB	*orig_rcb = rcb;
    DM_TID      bid = *bid_input;
    DM_TID      newbid;
    DMP_RCB     *r = orig_rcb;
    DB_STATUS   status = E_DB_OK;

    CLRDBERR(dberr);

    /*
    **
    ** Which rcb update?
    ** 
    **    active_rcb_update:  For tables with TCB_PG_V1.
    **
    **    passive_rcb_update: For tables with page size != TCB_PG_V1. 
    **
    **    If page type != TCB_PG_V1, space reclamation = FALSE when
    **    deleting leaf entries, and passive_rcb_update is used to maintain
    **    btree cursor position. Passive rcb update requires that deletes
    **    DO NOT reclaim space. If another cursor is currently positioned 
    **    on the leaf entry we are deleting, we will need to reposition 
    **    to the deleted entry at GET-NEXT time.
    ** 
    */
    if (rcb->rcb_tcb_ptr->tcb_rel.relpgtype == TCB_PG_V1)
    {
	/*
	** Active rcb update adjusts BIDs
	*/
	status = active_rcb_update(rcb, 
		bid_input, newbid_input, split_pos, opflag,
		del_reclaim_space, tid, newtid, newkey, dberr);
    }
    else
    {
	if (opflag == DM1C_MPUT || opflag == DM1C_MCLEAN || 
		opflag == DM1C_MRESERVE || opflag == DM1C_MSPLIT)
	{
	    /*
	    ** No rcb update action because btree_reposition
	    ** can handle shifting of keys
	    */
	    return (E_DB_OK);
        }
	status = passive_rcb_update(rcb, opflag, bid_input,
			tid, newtid, newkey, dberr);
    }

    return (status);	
}
	

/*{
** Name: active_rcb_update - Updates all RCB's of a single transaction.
**
** Description:
**
**    The following is an 'active' approach to updating btree cursor position.
**    When an update invalidates the position of other cursors,
**    we immediately find those other cursors and adjust their position.
**
**    This routine updates all the RCB's associated with one transaction.
**    The information in the RCB relating to current postion may change
**    depending upon the action taken with the current RCB.  For example
**    if the current RCB deletes a record, the leaf page line table is
**    compressed.  Therefore, all other RCB's associated with this transaction
**    must have their current bid pointers updated if they were pointing to
**    the line table entry deleted or to any other line table entry 
**    on this page greater than the one deleted.  A similar operation must occur
**    for inserts and splits.  Only the current position pointer
**    is affected since the determination of when the search(scan) is
**    completed is based on a key value and not a limit bid.
**
**    In addition this is the routine which determines if the record
**    previously fetched is still valid.  If one cursor fetched
**    a record (tid = 2,3) and another cursor fetches the same record
**    each is pointing to 2,3 for the next operation.  If the first one
**    does a replace which moves it to 14,4, them the other one must
**    know to refetch the record at 14,4 instead of using the one
**    it fetched previously.  The is handled by tracking the fetched tid
**    in rcb_fetchtid and by indicating the state with rcb_state
**    set to RCB_FETCH_REQ on.  These are both cleared by the
**    next get.
**    
**    This routine has code to handle tracking replaces and
**    deletes of records already fetched.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      bid                             A pointer to a TID indicating 
**                                      position on leaf page for
**                                      index record changed.
**      newbid                          A pointer to a TID indicating
**                                      position on leaf page for
**                                      replaces which move the record.
**                                      Or the newbid after a split.
**      opflag                          A value indicating type of operation
**                                      which changed an index record. One of:
**                                          DM1C_MDELETE
**                                          DM1C_MPUT
**                                          DM1C_MREPLACE
**                                          DM1C_MSPLIT
**                                          DM1C_MDEALLOC
**
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
**      24-nov-85 (jennifer)
**          Created for Jupiter.
**	03-sep-87 (rogerk)
**	    Don't compare tid_line if it is DM_TIDEOF.  Logically this position
**	    is less than the first position, but it will compare greater than
**	    all positions.
**	    In SPLIT case, check and set the rcb's lowtid and fetchtid positions
**	    independent of each other.
**	16-may-87 (rogerk)
**	    Fix problem with readnolock RCB's.  If readnolock rcb has its own
**	    copy of the changed page, then don't reposition since the bids
**	    did not get shifted on the readnolock copy.
**	21-jun-1993 (rogerk)
**	    Changed routine to return an error if called with an invalid
**	    opflag type rather than returning OK without doing anything.
**	26-jul-1993 (rogerk)
**	    Added new mode - DM1C_MDEALLOC - which is used when an overflow
**	    page is deallocated.  It adjusts any rcb positioned onto that
**	    deallocated page to be positioned to the previous overflow page.
**	30-apr-1996 (kch)
**	    If the current RCB deletes a record (DM1C_MDELETE) we now
**	    perform the leaf page line table compression for all other RCBs
**	    after the test for and marking of a fetched tid as deleted. This
**	    change fixes bug 74970.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Adjust the lowtid/fetchtid lsn & clean_count to avoid reposition
**          New parameters
**          Call passive_rcb_update if row locking.
**      25-nov-96 (stial01)
**          dm1b_rcb_update() fixed args to dm1b_update_lsn()
**       5-oct-99 (wanfr01)
**	    Bug 99067, INGSRV 1029
** 	    break statement missing for DM1C_MDELETE.  This could cause
**          incorrect results if the processing in DM1C_MPUT adjusts the
**          tid_line value.
**	    
*/
static DB_STATUS
active_rcb_update(
    DMP_RCB        *rcb,
    DM_TID         *bid_input,
    DM_TID         *newbid_input,
    i4        split_pos,
    i4        opflag,
    i4        del_reclaim_space,
    DM_TID         *tid,
    DM_TID         *newtid,
    char           *newkey,
    DB_ERROR       *dberr)
{
    DMP_RCB	*orig_rcb = rcb;
    DM_TID      bid = *bid_input;
    DM_TID      newbid;
    DMP_RCB     *r = orig_rcb;
    DB_STATUS   status = E_DB_OK;

    CLRDBERR(dberr);

    do
    {
	/*
	** If this is a readnolock RCB that is positioned on its own
	** copy of the changed page, then don't update the position
	** pointers since the readnolock copy was not changed.
	*/
	if ((r->rcb_k_duration & RCB_K_READ) &&
	    (r->rcb_other.page != NULL) &&
	    (r->rcb_other.page == r->rcb_1rnl_page ||
	    r->rcb_other.page == r->rcb_2rnl_page) &&
	    (DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
	    r->rcb_other.page) == bid.tid_tid.tid_page))
	{
	     continue;
	}

	if (newbid_input)
	    newbid.tid_i4 = newbid_input->tid_i4;

	switch (opflag)
	{
	case DM1C_MDELETE:
	{

	    /* See if we need to adjust line number for next get */
	    if (r->rcb_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page
		&& r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
		&& r->rcb_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line
		&& del_reclaim_space)
	    {
		/* Adjust the line number for next get */
		r->rcb_lowtid.tid_tid.tid_line--; 	    
	    }

	    /* See if we need to adjust the line number for reposition */
	    if (r->rcb_p_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page
		&& r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF
		&& r->rcb_p_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line
		&& del_reclaim_space)
	    {
		/* Adjust the line number for reposition */
		r->rcb_p_lowtid.tid_tid.tid_line--; 	    
	    }

	    /*
	    ** See if we need to adjust line number of FETCHED row
	    **
	    ** If page_type != TCB_PG_V1, we don't reclaim 
	    ** space when deleting the last duplicated key value on 
	    ** a leaf page with an overflow chain
	    */ 
	    if (r->rcb_fetchtid.tid_tid.tid_page == bid.tid_tid.tid_page)
	    {
		if (r->rcb_fetchtid.tid_tid.tid_line == bid.tid_tid.tid_line)
		{
		    /* Mark fetched row as deleted by placing an illegal tid */
		    r->rcb_fetchtid.tid_tid.tid_page = 0;
		    r->rcb_fetchtid.tid_tid.tid_line = DM_TIDEOF;
		    r->rcb_state |= RCB_FETCH_REQ;
		}
		else if (r->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
		    && r->rcb_fetchtid.tid_tid.tid_line > bid.tid_tid.tid_line
		    && del_reclaim_space)
		{
		    /* Adjust line number of FETCHED row */
		    r->rcb_fetchtid.tid_tid.tid_line--; 	    
		}
	    }
            break;
	}	

	case DM1C_MPUT:
	{	
	    /* See if we need to adjust line number for next get */
	    if (r->rcb_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page
		&& r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
		&& r->rcb_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
	    {
		/* Adjust the line number for next get */
		r->rcb_lowtid.tid_tid.tid_line++; 	    
	    }

	    /* See if we need to adjust the line number for reposition */
	    if (r->rcb_p_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page
		&& r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF
		&& r->rcb_p_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
	    {
		/* Adjust the line number for reposition  */
		r->rcb_p_lowtid.tid_tid.tid_line++; 	    
	    }

	    /* See if we need to adjust line number of FETCHED row */
	    if (r->rcb_fetchtid.tid_tid.tid_page == bid.tid_tid.tid_page
		&& r->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
		&& r->rcb_fetchtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
	    {
		/* Adjust line number of FETCHED row */
		r->rcb_fetchtid.tid_tid.tid_line++;
	    }

	    break;

	}	

	case DM1C_MSPLIT:
	{	
	    if ((r->rcb_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
	     && r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
	     && (i4)r->rcb_lowtid.tid_tid.tid_line >= (i4)split_pos)
	    {
		/* Adjust the page, line number for next get */
		r->rcb_lowtid.tid_tid.tid_line -= split_pos;
		r->rcb_lowtid.tid_tid.tid_page =  newbid.tid_tid.tid_page;
	    }

	    /* See if we need to adjust the line number for reposition */
	    if ((r->rcb_p_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
	     && r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF
	     && (i4)r->rcb_p_lowtid.tid_tid.tid_line >= (i4)split_pos)
	    {
		/* Adjust the page, line number for reposition */
		r->rcb_p_lowtid.tid_tid.tid_line -= split_pos;
		r->rcb_p_lowtid.tid_tid.tid_page =  newbid.tid_tid.tid_page;
	    }

	    if (r->rcb_fetchtid.tid_tid.tid_page == bid.tid_tid.tid_page
	     && r->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
	     && (i4)r->rcb_fetchtid.tid_tid.tid_line >= (i4)split_pos)
	    {
		/* Adjust page, line number of FETCHED row */
		r->rcb_fetchtid.tid_tid.tid_line -= split_pos;
		r->rcb_fetchtid.tid_tid.tid_page = newbid.tid_tid.tid_page;
	    }

	    break;
	}	

	case DM1C_MREPLACE:
	{	
	    /* This is very tricky code.  The line table in BTREES is kept
	    ** sorted, therefore it compresses or expands depending on the
	    ** value of old and new bids.  If the old and new bids end up on
	    ** the same page it is even more complicated since the first part
	    ** the replace (old) can cause the linetable entries to compressed,
	    ** thereby changing the scan and fetch bids.  Then if it is
	    ** placed on same page, it causes the line table to expand,
	    ** thereby changing the scan and fetch bids again.
	    ** In any case unless you understand this code very well, 
	    ** you should probably not try to change it.
	    **
	    ** If page_type != TCB_PG_V1, we don't reclaim 
	    ** space when deleting the last duplicated key value on 
	    ** a leaf page with an overflow chain
	    */
	    if (bid.tid_i4 == newbid.tid_i4)
	    {
		/* Just doing a replace in place, update
		** the rcb state in those who have it fetched.
		*/
		if (r->rcb_fetchtid.tid_i4 == bid.tid_i4)
		{
		    MEcopy(newkey, r->rcb_tcb_ptr->tcb_klen, r->rcb_fet_key_ptr);
		    r->rcb_pos_info[RCB_P_FETCH].tidp.tid_i4 = newtid->tid_i4;
		    r->rcb_state |= RCB_FETCH_REQ;
		}
		break;
	    }

	    /*
	    ** Non in place REPLACE
	    **
	    ** If the fetchtid is set to the one we are
	    ** replacing, then update fetch to new one.
	    */
	    if (r->rcb_fetchtid.tid_i4 == bid.tid_i4)
	    {
		r->rcb_fetchtid.tid_i4 = newbid.tid_i4;

		/* Update low only depending on values of old and new */
		if ((r->rcb_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
		    && r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
		    && r->rcb_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			r->rcb_lowtid.tid_tid.tid_line--; 	    
		}

		if ((r->rcb_p_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
		    && r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF
		    && r->rcb_p_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			r->rcb_p_lowtid.tid_tid.tid_line--; 	    
		}

		/* If old and new are on the same page
		** and new is > old, then new would have 
		** been shifted by the delete, so do it.
		*/
		if (newbid.tid_tid.tid_page == bid.tid_tid.tid_page
		    && newbid.tid_tid.tid_line > bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			newbid.tid_tid.tid_line--;
		}

		if ((r->rcb_lowtid.tid_tid.tid_page == newbid.tid_tid.tid_page)
 	         && (r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF)
	         && (r->rcb_lowtid.tid_tid.tid_line >= newbid.tid_tid.tid_line))

		{
		    r->rcb_lowtid.tid_tid.tid_line++; 	    
		}

		if ((r->rcb_p_lowtid.tid_tid.tid_page == newbid.tid_tid.tid_page)
 	         && (r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF)
	         && (r->rcb_p_lowtid.tid_tid.tid_line >= newbid.tid_tid.tid_line))

		{
		    r->rcb_p_lowtid.tid_tid.tid_line++; 	    
		}

	    }
	    else
	    {
		/*
		** If fetch tid not the same, update both
		** low and fetch based on old and new values.
		*/

		if ((r->rcb_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
		    && r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF
		    && r->rcb_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			r->rcb_lowtid.tid_tid.tid_line--; 	    
		}

		if ((r->rcb_p_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
		    && r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF
		    && r->rcb_p_lowtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			r->rcb_p_lowtid.tid_tid.tid_line--; 	    
		}

		/*
		** If old and new are on the same page and new is > old,
		** then new would have been shifted by the delete, so do it.
		*/
		if (r->rcb_fetchtid.tid_tid.tid_page == bid.tid_tid.tid_page
		    && r->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF
		    && r->rcb_fetchtid.tid_tid.tid_line >= bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			r->rcb_fetchtid.tid_tid.tid_line--; 	    
		}

		if (newbid.tid_tid.tid_page == bid.tid_tid.tid_page
		    && newbid.tid_tid.tid_line > bid.tid_tid.tid_line)
		{
		    if (del_reclaim_space)
			newbid.tid_tid.tid_line--;
		}

		if ((r->rcb_lowtid.tid_tid.tid_page == newbid.tid_tid.tid_page)
		 && (r->rcb_lowtid.tid_tid.tid_line != DM_TIDEOF)
		 && (r->rcb_lowtid.tid_tid.tid_line >= newbid.tid_tid.tid_line))
		{
		    r->rcb_lowtid.tid_tid.tid_line++; 	    
		}

		if ((r->rcb_p_lowtid.tid_tid.tid_page == newbid.tid_tid.tid_page)
		 && (r->rcb_p_lowtid.tid_tid.tid_line != DM_TIDEOF)
		 && (r->rcb_p_lowtid.tid_tid.tid_line >= newbid.tid_tid.tid_line))
		{
		    r->rcb_p_lowtid.tid_tid.tid_line++; 	    
		}

		if ((r->rcb_fetchtid.tid_tid.tid_page == 
						newbid.tid_tid.tid_page)
		 && (r->rcb_fetchtid.tid_tid.tid_line != DM_TIDEOF)
		 && (r->rcb_fetchtid.tid_tid.tid_line >=
						newbid.tid_tid.tid_line))
		{
		    r->rcb_fetchtid.tid_tid.tid_line++; 	    
		}
	    }

	    /*
	    ** Compare fetched record:
	    **  It is sufficient to compare the data tid which is unique
	    **  If tidp matches, update the fetched key, fetched tidp.
	    **  Update rcb state in those who have updated row fetched.
	    **
	    */
	    if (r->rcb_pos_info[RCB_P_FETCH].tidp.tid_i4 == tid->tid_i4)
	    {
		MEcopy(newkey, r->rcb_tcb_ptr->tcb_klen, r->rcb_fet_key_ptr);
		r->rcb_pos_info[RCB_P_FETCH].tidp.tid_i4 = newtid->tid_i4;
	    }

	    r->rcb_state |= RCB_FETCH_REQ;
	    break;
	}	

	case DM1C_MDEALLOC:
	{
	    /*
	    ** Find any RCB's which are positioned to the deallocated page.
	    ** They must be changed so that their next get operation does not
	    ** attempt to interpret the data on this page.
	    **
	    ** Any such RCB is positioned back to the end of the previous page
	    ** so that its new overflow pointer can be followed to get to the
	    ** next real page.
	    */

	    if (r->rcb_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
	    {
		r->rcb_lowtid.tid_i4 = newbid.tid_i4;
	    }

	    if (r->rcb_p_lowtid.tid_tid.tid_page == bid.tid_tid.tid_page)
	    {
		r->rcb_p_lowtid.tid_i4 = newbid.tid_i4;
	    }

	    if (r->rcb_fetchtid.tid_tid.tid_page == bid.tid_tid.tid_page)
	    {
		r->rcb_fetchtid.tid_i4 = newbid.tid_i4;
	    }

	    break;
	}	

	default:
	    TRdisplay("DM1B_RCB_UPDATE called with unknown mode\n");
	    SETDBERR(dberr, 0, E_DM9C25_DM1B_RCB_UPDATE);
	    return (E_DB_ERROR);
	}

    } while ((r = r->rcb_rl_next) != orig_rcb);

    return (E_DB_OK);
}


/*{
** Name: passive_rcb_update - 'passive' RCB update.
**
** Description:
**
**  The following is a 'passive' approach to updating btree cursor position. 
**  When an update invalidates the position of other cursors, for most 
**  operations we do nothing.
**  Those other cursors must detect at GET-NEXT time that their cursor
**  position has been invalidated and reposition.
**
**      At GET-NEXT time cursors will adjust their own rcb_lowtid.
**        dm1b_get -> dm1b_reposition
**
**      At REPLACE-DELETE time cursors will adjust their own rcb_fetchtid.
**        dm2r_replace -> dm2r_fetch_record -> dm1b_bybid_get -> dm1b_reposition
**        dm2r_delete  -> dm2r_fetch_record -> dm1b_bybid_get -> dm1b_reposition
**
**  This method will be used when row locking. 
**  This method is not supported for 2k pages, since it only works if
**  space is not reclaimed when a leaf entry is deleted.
**
**  DM1C_MPUT: No passive rcb update action
**  DM1C_MSPLIT: No passive rcb update action 
**  DM1C_MCLEAN: No passive rcb update action
**  DM1C_MRESERVE: No passive rcb update action
**
**  DM1C_MDELETE:
**
**         Compare:
**              (delete tid)
**         To:
**              (fetched tid)
**
**         If another cursor has fetched the row being deleted, mark it
**         as deleted by placing an illegal tid in the fetched tid.
**
**              r->rcb_fetchtid.tid_tid.tid_page = 0;
**              r->rcb_fetchtid.tid_tid.tid_line = DM_TIDEOF;
**              r->rcb_state |= RCB_FETCH_REQ;
**
**  DM1C_MREPLACE:
**
**          Compare:
**              (replace tid)
**          To:
**              (fetchted tid)
**
**          If another cursor has fetched the row being updated,
**          update rcb state and invalidate the fetch position.
**         
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      opflag                          A value indicating type of operation
**                                      which changed an index record. One of:
**                                          DM1C_MDELETE
**                                          DM1C_MPUT
**                                          DM1C_MREPLACE
**                                          DM1C_MSPLIT
**      bid                             A pointer to a TID indicating 
**                                      position on leaf page for
**                                      index record changed.
**      tid                             To find RCB with same row 'fetched' 
**      newtid                          To update rcb_fet_tidp
**      newkey                          To update rcb_fet_key_ptr
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
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
**      17-dec-96 (stial01)
**          passive_rcb_update() DM1C_MSPLIT: Set rcb_status |= RCB_FETCH_REQ 
**          Also added bid paramater,deleted unused lsn,clean count parameters
*/
static DB_STATUS
passive_rcb_update(
    DMP_RCB        *rcb,
    i4        opflag,
    DM_TID         *bid,
    DM_TID         *tid,
    DM_TID         *newtid,
    char           *newkey,
    DB_ERROR       *dberr)
{
    DMP_RCB	*orig_rcb = rcb;
    DMP_RCB     *r = orig_rcb;
    DB_STATUS   status = E_DB_OK;

    CLRDBERR(dberr);

    /*
    ** The 'passive' approach to updating cursor is only supported 
    ** for page type != TCB_PG_V1.
    ** Since duplicate keys always span leaf pages not overflow pages 
    ** if page type != TCB_PG_V1, we never need to worry
    ** about DM1C_MDEALLOC overflow page operations.
    */
    if (opflag == DM1C_MDEALLOC)
    {
	TRdisplay("passive_rcb_update DM1C_MDEALLOC\n");
	SETDBERR(dberr, 0, E_DM9C25_DM1B_RCB_UPDATE);
	return (E_DB_ERROR);
    }

    do
    {
	/*
	** If this is a readnolock RCB skip it
	*/
	if (r->rcb_k_duration & RCB_K_READ)
	{
	     continue;
	}

	switch (opflag)
	{
	    case DM1C_MPUT:
	    case DM1C_MCLEAN:
	    case DM1C_MRESERVE:
	    case DM1C_MSPLIT:
	    {
		/*
		** No rcb update action because btree_reposition
		** can handle shifting of keys
		*/
		break;
	    }

	    case DM1C_MDELETE:
	    {
		/*
		** Compare fetched record:
		**  It is sufficient to compare the data tid which is unique
		**
		** For any rcb which has already fetched this tid, then 
		** mark it as deleted by placing an illegal tid in the
		** fetched tid.
		*/
		if (r->rcb_pos_info[RCB_P_FETCH].tidp.tid_i4 == tid->tid_i4)
		{
		    r->rcb_fetchtid.tid_tid.tid_page = 0;
		    r->rcb_fetchtid.tid_tid.tid_line = DM_TIDEOF;
		    r->rcb_state |= RCB_FETCH_REQ;
		    DM1B_POSITION_INVALIDATE_MACRO(r, RCB_P_FETCH);
		}

		break;
	    }
	    case DM1C_MREPLACE:
	    {

		/*
		** Compare fetched record:
		**  It is sufficient to compare the data tid which is unique
		**  Update rcb state in those who have updated row fetched.
		*/
		if (r->rcb_pos_info[RCB_P_FETCH].tidp.tid_i4 == tid->tid_i4)
		{
		    MEcopy(newkey, r->rcb_tcb_ptr->tcb_klen, r->rcb_fet_key_ptr);
		    r->rcb_pos_info[RCB_P_FETCH].tidp.tid_i4 = newtid->tid_i4;
		    r->rcb_state |= RCB_FETCH_REQ;
		    DM1B_POSITION_INVALIDATE_MACRO(r, RCB_P_FETCH);
		}

		break;
	    }

	    default:
	    {
		TRdisplay("DM1B_ROW_RCB_UPDATE called with unknown mode\n");
		SETDBERR(dberr, 0, E_DM9C25_DM1B_RCB_UPDATE);
		return (E_DB_ERROR);
	    }
	}

    } while ((r = r->rcb_rl_next) != orig_rcb);

    return (E_DB_OK);
}

/*{
** Name: dm1b_search - BTREE traversal routine.
**
** Description:
**      This routines searches the btree index pages starting from
**      the root page and finds the leaf page that should contain the
**      key that is specified.   The mode and direction of the
**      search can significantly alter the searching technique 
**      and may alter the types of locks obtained during the search.
**      
**      During the search there are the appropriate locks placed
**      on the current (child) page it is looking at and the parent of
**      this page. 
**
**	As you descend the tree, you request a lock on the child page while
**	holding a lock on the current page. Then you release the lock on the
**	current page, make the child page the current page, and iterate the
**	entire process.
**
**      In the end only the leaf page of the key specified
**      has a lock remaining on it.  The leaf locks are held until
**      end transaction.  
**
**	Note overflow pages are locked. However, since the leaf page lock is
**	held to end of transaction, the overflow page locking does not
**	typically matter (it might cause an escalation, but except for that the
**	overflow page locks of two distinct transactions are compatible if and
**	only if the leaf page locks they hold are compatible).
**
**      If the mode indicates pages should 
**      be split as you are searching, then write_physical 
**      locks are set, otherwise read_physical  locks are set on the
**      index pages.   Read or write locks are set on the leaf pages
**      depending on the accessmode indicated in the RCB.  To increase
**      concurrenacy, a special locking protocol is observed for
**      leaf pages.  When you have obtained the parent of a leaf
**      (indicated by DMPP_SPRIG in the page status field)  
**      and you are not searching in DM1C_SPLIT mode then
**      the child(leaf) page number is obtained, the parent lock
**      is released and the new child(leaf) is obtained.  
**      This allows the index locks to be released while waiting 
**      for a leaf lock.  After obtaining the leaf, it must be check
**      to insure it still contains the key value originally 
**      searched for.  If it does not, then the sideways pointer
**      must be used to obtain the correct leaf page.  This only
**      works since we never do joins, we always split to the
**      right and the leaf pages have sideways pointers.  When
**      splitting, it is the second pass, you already have a lock on 
**      the leaf you want to split, so you are guaranteed not to
**      have to wait for lock on leaf.  Split will happen immediately.
**      
**      At the completion of this routine only the leaf page
**      associated with the lowbid remains locked.  
**      The lowbid is the tid of the first leaf page entry associated
**      with the key specified.  
**
**      If mode is FIND, then this routine determines if an exact
**      match was found.  Since the leaf can contain entries for a
**      range of keys the leaf page may be the one fitting the
**      search criteria, but it may not contain an existing entry
**      for the key.  If the page does not have an exact match
**      then an error is returned indicating that the key was
**      not found.  It still keeps this page locked, since it is
**      the leaf page the key should be placed on or found on. 
**
**      If mode is OPTIM for optimistic, the search progresses
**      assuming that splits will not be required.  If the leaf
**      page is full then an error indicating no more room is
**      returned to the caller.
**
**      In all cases only the page indicated by leaf is fixed
**      on exit.  This is usually the leaf page associated with the
**      lowbid, but may be an overflow page if DM1C_TID is
**      indicated as mode. A page is fixed into 'leaf' under the following
**	conditions:
**	    E_DB_OK			page is fixed into leaf
**	    E_DB_ERROR/E_DM0056_NOPART	page is fixed into leaf
**	    E_DB_ERROR/E_DM0057_NOROOM	page is fixed into leaf
**	    <anything else>		no page is fixed.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      key                             Pointer to the key value.
**      keys_given                      Value indicating number of 
**                                      attributes in key. Note that this is
**					the number of attributes to be used for
**					searching, which is not necessarily the
**					same as the number of attributes in the
**					index entry (e.g., partial range search)
**      mode                            Value indicating type of search
**                                      to perform.  Must be DM1C_SPLIT, 
**                                      DM1C_JOIN, DM1C_FIND, DM1C_EXTREME, 
**                                      DM1C_RANGE, DM1C_OPTIM, DM1C_SETTID, 
**                                      DM1C_POSITION, DM1C_TID or 
**                                      DM1C_FIND_SI_UPD.  
**					(JOIN,SETTID, and POSITION are obsolete)
**      direction                       Value indicating direction of 
**                                      search.  Must be DM1C_PREV, 
**                                      DM1C_NEXT, or DM1C_EXACT.
**
** Outputs:
**      leaf                            Pointer to a pointer used to 
**                                      identify leaf page locked and 
**                                      fixed into memory.
**      lowbid                          Pointer to an area to return
**                                      bid of qualifying leaf record.
**      tid                             Pointer to an area to return
**                                      the tid of the matching tuple
**                                      on an exact search.  If mode
**                                      is DM1C_SETTID, then it is the tid
**                                      want to position to.
**      err_code                        Pointer to an area to return 
**                                      the following errors when 
**                                      E_DB_ERROR is returned.
**                                      E_DM0056_NOPART
**                                      E_DM0057_NOROOM
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**          E_DB_WARN
**	Exceptions:
**	    none
**
** Side Effects:
**	    Leaves the page returned in leaf fixed and locked.
**
** History:
**      06-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	12-sep-88 (rogerk)
**	    Don't pass 'current' pointer to the callers 'leaf' until final
**	    current value is determined and we are ready to return.  By using
**	    'leaf' and 'current' to store the same temporary values, we end
**	    up getting them out of synch on error conditions - which results
**	    in buffer manager errors.
**	14-nov-88 (teg)
**	    Initialized adf_cb.adf_maxstring = DB_MAXSTRING
**	 4-dec-88 (rogerk)
**	    Don't release locks on index pages when fail during split.
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree index compression.
**	23-apr-1991 (bryanp)
**	    B37119:
**	    As we walk the tree, we advance from level to level by fixing into
**	    'current', unfixing 'parent', then moving the fixed page from
**	    current into parent.  When we do this, we must clear current, so
**	    that we don't mistakenly have two pointers to the same page (this
**	    can cause us to try to unfix the page twice in the cleanup code
**	    at the bottom, causing dmd_checks).
**	16-sep-1992 (bryanp)
**	    Make DM609 format leaf page at end of search.
**	14-oct-1992 (jnash)
**	    Add (DM1B_INDEX *) cast to dmd_prindex param to quiet compiler.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	      - Changed dm1bxsearch to not traverse leaf's overflow chain when
**		called in DM1C_TID mode.  That logic was moved to this routine.
**		Now when calling dm1bxsearch in DM1C_TID mode we check for the
**		NOPART return status and go to the next overflow page ourselves
**		and do a dm1bxsearch on it.
**	      - Changed dm1bxsearch to take buffer pointer by reference rather
**		than by value since it is no longer ever changed.
**	10-nov-1993 (pearl)
**	    Bug 54961.  When the search mode is DM1C_RANGE in dm1b_search(),
**	    change the algorithm such that if the key is not found on the 
**	    on an non-empty leaf page, and there is a next leaf
**	    page, then fix the next page.  If the leaf page is empty, and there
**	    are overflow pages, then go down the overflow chain until we find
**	    a key.  We can then determine whether the key is within the given
**	    range, and fix the next page if it is not.  If we are already at
**	    the rightmost leaf page, and the keys are not in range, then we
**	    have to position at the end of the overflow chain if there is one.
**      10-apl-1995 (chech02) Bug# 64830 (merged the fixes from 6.4 by nick)
**          Unfix the page pointed to by leaf to avoid overwriting
**          this pointer when fixing the result page. #64830.
**      19-apr-1995 (stial01) New mode DM1C_LAST (select last), to position
**          to end of the page.
**	18-may-1995 (cohmi01)
**	    For DM1C_LAST, if duplicate chain, go to last page in chain.
**	11-aug-1995 (shust01)
**	    For DM1C_LAST, set pos to leafpage->bt_kids instead of DM_TIDEOF.
**	    DM_TIDEOF was causing problems when doing a NEXT after a LAST.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Call dm1bxclean() to reclaim space on leaf pages;
**          If phantom protection is needed, set DM0P_PHANPRO fix action.
**      27-jan-97 (dilma04)
**          If updading secondary index, search in DM1C_TID mode without 
**          phantom protection.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      07-apr-97 (stial01)
**          dm1b_search() NonUnique primary btree (V2), dups can span leaf
**      21-apr-97 (stial01)
**          btree_search() Created from dm1b_search, now dm1b_search is
**          external interface for btree search.
**      21-may-97 (stial01)
**          dm1b_search() Save position page lsn,clean count in rcb while 
**          leaf is mutexed
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          btree_search() Pass tlv to dm1cxget instead of tcb.
**      16-jun-97 (stial01)
**          btree_search() Unlock buffer before unfixing
**	17-dec-98 (thaju02)
**	    modified btree_search(). When traversing overflow chain, fix
**	    overflow pages with correct fix action (based off of the
**	    rcb_access_mode). (b94647)
**      21-Aug-99 (wanfr01)
**        Bug 98274, INGSRV 985:
**        If no rows for a duplicate key exist on the primary leaf page,
**        but rows exist on the next leaf page, then an exact match
**        will return 0 rows if the data page size is > 2K
**	05-Jun-2006 (jenjo02)
**	    Add optional char *record output parm to btree_search
**	    into which key-matching leaf entry is returned, used by 
**	    TID gets on Clustered tables.
*/
DB_STATUS
dm1b_search(
    DMP_RCB            *rcb,
    char               *key,
    i4            keys_given,
    i4            mode,
    i4            direction,
    DMP_PINFO          *leafPinfo,
    DM_TID             *bid,
    DM_TID             *tid,
    DB_ERROR           *dberr)
{
    DMP_RCB         *r = rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS       status;

    DM1B_POSITION_INIT_MACRO(r, RCB_P_START);
    DM1B_POSITION_INIT_MACRO(r, RCB_P_GET);

    status = btree_search(r, key, keys_given, mode, direction, leafPinfo, 
	bid, tid, NULL, dberr);

    /* Save start position */
    if ( (status == E_DB_OK) && (leafPinfo->page != NULL) && 
	(DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leafPinfo->page) 
	    == bid->tid_tid.tid_page))
	
    {
	DM1B_POSITION_SAVE_MACRO(r, leafPinfo->page, bid, RCB_P_START);

	if (mode == DM1C_FIND_SI_UPD ||
		(mode == DM1C_TID && direction == DM1C_EXACT))
	{
	    /* Save key position while the leaf is locked/pinned */
	    DM1B_POSITION_SAVE_MACRO(r, leafPinfo->page, bid, RCB_P_FETCH);

	    /* key passed by the caller is the leaf key, not the leaf entry */
	    MEcopy(key, t->tcb_klen, r->rcb_fet_key_ptr);
	}
    }

    dm0pUnlockBuf(r, leafPinfo);

    return (status);
}

static DB_STATUS
btree_search(
    DMP_RCB            *rcb,
    char               *key,
    i4            	keys_given,
    i4            	mode,
    i4            	direction,
    DMP_PINFO          *leafPinfo,
    DM_TID             *bid,
    DM_TID             *tid,
    char	       *record,
    DB_ERROR           *dberr)
{
    DMP_RCB	 *r = rcb;                                        
    DMP_TCB      *t = r->rcb_tcb_ptr;   /* Table control block. */
    DMP_TABLE_IO *tbio = &t->tcb_table_io;
    DM_TID       child;                 /* TID of child. */ 
    i4		 childpartno;
    i4      	 local_err_code;        /* Local error code. */
    i4      	 index_fix_action;	/* Type of action for fixing index
                                        ** pages into memory. */
    i4	 	 leaf_fix_action;	/* action for fixing leaf pages. */
    DM_LINE_IDX  pos;                   /* TID line position. */
    char	 *DupKey;               /* Variable to indicate duplicate
                                        ** key exists. */
    DB_STATUS    s, state;     		/* Routine return status. */
    i4      	 xmode;                 /* local search mode. */
    i4           samekey;               /* Variable to indicate if two
                                        ** keys matched. */
    ADF_CB	 *adf_cb;
    i4	 	 at_sprig_level;
    i4      	 fix_action;
    bool         search_next;
    DM_PAGENO    next;
    i4           compare;
    char	 *AllocTkey = NULL;
    char	 temp_buf[DM1B_MAXSTACKKEY];
    char         *index_key, *leaf_key;
    i4		 tidpartno;
    DB_ERROR	 local_dberr;
    i4		    *err_code = &dberr->err_code;
    i4		 orig_mode;
    DMP_PINFO	 sprigPinfo, parentPinfo, currentPinfo;
    DMPP_PAGE    **save_sprig = &sprigPinfo.page;
    DMPP_PAGE	 **parent = &parentPinfo.page;
    DMPP_PAGE	 **current = &currentPinfo.page;

    CLRDBERR(dberr);

    DMP_PINIT(&sprigPinfo);
    DMP_PINIT(&parentPinfo);
    DMP_PINIT(&currentPinfo);

    /*
    ** Key provided is leaf OR index key
    ** We need to build the key not provided when LEAF atts != INDEX atts
    ** or when the LEAF key provided belongs to a CLUSTERED base table.
    */
    if (!key ||
	(t->tcb_rel.relstat & TCB_CLUSTERED) == 0 &&
	    ((t->tcb_rel.relstat & TCB_INDEX) == 0 ||
	      t->tcb_leaf_rac.att_count == t->tcb_index_rac.att_count))
    {
	leaf_key = index_key = key;
    }
    else
    {
	/* Key provided is leaf key */
	leaf_key = key;
	if ( s = dm1b_AllocKeyBuf(t->tcb_klen, temp_buf,
				    &index_key, &AllocTkey, dberr) )
	    return(s);
	dm1b_build_key(t->tcb_keys, t->tcb_leafkeys, leaf_key,
			t->tcb_ixkeys, index_key);
    }

    /* Initialize pointers in case of errors. */

    adf_cb = r->rcb_adf_cb;
    
    /* 
    ** If this search is for a SPLIT then
    ** pages need to be temporarily locked for writing.
    ** Otherwise they will be temporarily locked for 
    ** reading.
    */
    orig_mode = mode;
    if (mode == DM1C_SPLIT_DUPS)
	mode = DM1C_SPLIT;

    if (mode != DM1C_SPLIT)
	index_fix_action = DM0P_RPHYS;
    else
	index_fix_action = DM0P_WPHYS;
    index_fix_action |= DM0P_NOREADAHEAD | DM0P_LOCKREAD;

    leaf_fix_action = DM0P_READ;            
    if (r->rcb_access_mode == RCB_A_WRITE)
	leaf_fix_action = DM0P_WRITE;
    if ((r->rcb_state & RCB_READAHEAD) == 0)
	leaf_fix_action |= DM0P_NOREADAHEAD;

    if (mode == DM1C_FIND_SI_UPD)
        mode = DM1C_TID;
    if (row_locking(r) && serializable(r) && (mode != DM1C_SPLIT) && 
        (mode != DM1C_OPTIM) && (mode != DM1C_TID) && (direction != DM1C_EXACT))
    {
         leaf_fix_action |= DM0P_PHANPRO;
    }

    for (;;)
    {
	if (leafPinfo && leafPinfo->page)
	{
# ifdef XDEBUG
	    TRdisplay("Data page fixed in call to dm1b_search(), unfixing ...\n");
# endif
	    dm0pUnlockBuf(r, leafPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	    if (s)
		break;
	}

	/* Get root page to start search. */

	s = dm0p_fix_page(r, (DM_PAGENO)DM1B_ROOT_PAGE, 
	    index_fix_action, &parentPinfo, dberr);     
	if (s != E_DB_OK) 
	    break;
   
	/* If splitting see if root page needs to be split. */

	if (mode == DM1C_SPLIT &&
	    dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *parent, 
		DM1B_INDEX_COMPRESSION(r),
		(i4)100 /* fill factor */,
		t->tcb_kperpage,
	       (t->tcb_ixklen + t->tcb_index_rac.worstcase_expansion 
		+ DM1B_VPT_GET_BT_TIDSZ_MACRO(
		    t->tcb_rel.relpgtype, *parent)))
			== FALSE)
	{
	    /* Split the root page, make parent the newroot. */

	    s = dm1bxnewroot(r, &parentPinfo, dberr);  
	    if (s != E_DB_OK)
		break;
	}

	/* 
	** Search down tree until find the index record indicating which
	** leaf page should contain this key.  The index page is the
	** parent page, and the leaf is the child. 
	*/

	xmode = mode;
	if (mode == DM1C_TID)
	    xmode = DM1C_FIND;
	do 
	{
	    s = dm1bxsearch(r, *parent, xmode, direction, index_key, 
		keys_given, &child, &childpartno, &pos, NULL, dberr); 
	    if (s != E_DB_OK && dberr->err_code != E_DM0056_NOPART) 
		break;

	    /* 
	    ** If reached parent of leaf, then get leaf page and
	    ** permanently fix it for read. If we are not splitting, then we
	    ** release the sprig page while we wait for the leaf page lock.
	    */

	    if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *parent) & 
		(DMPP_SPRIG|DMPP_INDEX)) == 0)
	    {
		dm1cxlog_error(E_DM93ED_WRONG_PAGE_TYPE, r, *parent,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 0);
		SETDBERR(dberr, 0, E_DMF014_DM1B_PARENT_INDEX);
		s = E_DB_ERROR;
		break;
	    }

	    at_sprig_level = 
		(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *parent) & 
		DMPP_SPRIG) != 0;

	    if (at_sprig_level) 
	    {	
		if (mode != DM1C_SPLIT)
		{	
		    s = dm0p_unfix_page(r, DM0P_RELEASE, &parentPinfo, dberr);
		    if (s != E_DB_OK)
			break;
		}	    
		else
		{
		    /*
		    ** save the sprig page pointer til we complete the
		    ** leaf page split (if needed).
		    */
		    sprigPinfo = parentPinfo;
		}
	    }

	    s = dm0p_fix_page(r, (DM_PAGENO)child.tid_tid.tid_page,
			(at_sprig_level ? leaf_fix_action : index_fix_action),
			&currentPinfo, dberr);     
	    if (s != E_DB_OK) 
		break;
	
	    if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *current) & 
		(DMPP_SPRIG|DMPP_INDEX|DMPP_LEAF)) == 0)
	    {
		dm1cxlog_error(E_DM93ED_WRONG_PAGE_TYPE, r, *current,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 0);
		SETDBERR(dberr, 0, E_DMF013_DM1B_CURRENT_INDEX);
		s = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If the parent page was a sprig page, then this page had better
	    ** be a leaf page.
	    */
	    if (at_sprig_level)
	    {
		if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *current) & 
		    DMPP_LEAF) == 0)
		{
		    dm1cxlog_error( E_DM93EA_NOT_A_LEAF_PAGE, r, *current,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 0);
		    s = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
		    break;
		}

		if (mode == DM1C_FIND_SI_UPD ||
			mode == DM1C_OPTIM || mode == DM1C_SPLIT)
		{
		    /*
		    ** Lock buffer for update.
		    **
		    ** This will swap from CR->Root if "current" is a CR page.
		    */
		    dm0pLockBufForUpd(r, &currentPinfo);
		}
		else
		{
		    /*
		    ** Lock buffer for get.
		    **
		    ** This may produce a CR page and overlay "current"
		    ** with its pointer.
		    */
		    dm0pLockBufForGet(r, &currentPinfo, &s, dberr);
		}
		if ( s == E_DB_ERROR )
		    break;

		/*
		** We have the correct leaf is fixed. Remove committed deletes.
		** If row locking, the buffer will stay locked during the clean.
		*/
		if ((mode == DM1C_OPTIM || mode == DM1C_SPLIT))
		{
		    s = dm1bxclean(r, &currentPinfo, dberr);
		    if (s != E_DB_OK)
			break;
		}

	    }

	    /* If splitting see if current page needs to be split. */

	    if ( mode == DM1C_SPLIT )
	    {
		i2	TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(
				    t->tcb_rel.relpgtype, *current);

		if ( dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
		    *current, DM1B_INDEX_COMPRESSION(r),
		    (i4)100 /* fill factor */,
		    at_sprig_level ? t->tcb_kperleaf : t->tcb_kperpage,
		    at_sprig_level ? 
		    (t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion + TidSize) :
		    (t->tcb_ixklen + t->tcb_index_rac.worstcase_expansion + TidSize)) == FALSE)
		{   
		    s = dm1bxsplit(r, &currentPinfo, (i4)pos, &parentPinfo, 
			at_sprig_level ? leaf_key : index_key,
			keys_given, orig_mode, dberr); 

		    if (s != E_DB_OK)
			break;
		}
	    }
        
	    /* 
	    ** If current page is index page, not parent of leaf,
	    ** then release the parent page, make current the parent
	    ** and continue search.
	    */

	    if (!at_sprig_level)
	    {
		s = dm0p_unfix_page(r, DM0P_RELEASE, &parentPinfo, dberr);     
		if (s != E_DB_OK)
		    break;
	    }

	    /*
	    ** Now advance to the next level and iterate back again.
	    */
	    parentPinfo = currentPinfo;
	    DMP_PINIT(&currentPinfo);

	} while (!(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *parent) & 
	    DMPP_LEAF));
	    
	if (s != E_DB_OK)
	    break;

	/*
	** parent now contains the needed leaf node's contents, and
	** save_sprig contains the leaf page's parent (sprig) page, if we
	** are in SPLIT mode.
	*/
    
	currentPinfo = parentPinfo;
	parentPinfo = sprigPinfo;

	/* 
	** Must requalify the leaf page, since it could have split
	** while you were waiting for it.  If searching for low, the
	** one you have is the lowest since always split to the right.
	** (mode == EXTREME, direction = PREV)
	*/
	if ( (*parent == NULL) 
	    && ((mode != DM1C_EXTREME) || (direction != DM1C_PREV)) )
	{
	    s = dm1b_requalify_leaf(r, mode, &currentPinfo,
		    leaf_key, keys_given, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/* 
	** If the leaf is the head of an overflow chain, 
	** find the key of the chain and save for comparing.
	*/

	/*
	** We're done with "index_key".
	** Reuse it for duplicates, if possible.
	*/
	if ( !AllocTkey && index_key != temp_buf )
	{
	    if ( s = dm1b_AllocKeyBuf(t->tcb_klen, temp_buf, &index_key, 
					&AllocTkey, dberr) )
	    break;
	}
	DupKey = index_key;

	state = E_DB_OK;
	s = dm1bxdupkey(r, &currentPinfo, DupKey, &state, dberr);
	if (s != E_DB_OK)
	    break;
	if ( state != E_DB_OK )
	    DupKey = NULL;

	/*
	** For all modes other than EXTREME must see if 
	** overflow chain key is the one we are looking for.
	** Compare overflow key with key searching for.
	** If they are not the same, then we cannot use
	** the leaf page found.  If splitting must create a new 
	** leaf page just for new key (same as a split).
	*/

	samekey = DM1B_NOTSAME;
	if (DupKey != NULL && mode != DM1C_EXTREME && mode != DM1C_LAST)
	    adt_kkcmp(adf_cb, keys_given, t->tcb_leafkeys, 
		    DupKey, leaf_key, &samekey);
	if (mode == DM1C_SPLIT && DupKey != NULL && samekey != DM1B_SAME)
	{
	    s = dm1bxsplit(r, &currentPinfo, (i4) pos, &parentPinfo, leaf_key, 
		    keys_given, orig_mode, dberr); 
	    if (s != E_DB_OK)
		break;
	}

	/* Discard any allocated key buffer, maintain DupKey as a boolean */
	if ( AllocTkey )
	    dm1b_DeallocKeyBuf(&AllocTkey, &index_key);
	
	/* 
	** Now have the leaf page meeting search criteria, get rid
	** of parent, no longer needed. 
	*/

	if (*parent != NULL)
	{
	    s = dm0p_unfix_page(r, DM0P_RELEASE, &parentPinfo, dberr);     
	    if (s != E_DB_OK) 
		break;
	}

	if (DMZ_AM_MACRO(9))
	{
	    TRdisplay("dm1b_search: reached bottom of search, leaf is %d\n", 
		     DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *current));
	    dmd_prindex(r, *current, (i4)0);
	}

	/* 
	** For exact or range searches find the leaf and check if the 
	** key actually exists on the leaf page.  (The previous search only
	** qualified that this is the leaf which might contain the key).
	**
	** If the key is found then the position is set to point to it.  If
	** no key is found then the position is set to the spot at which that
	** key value would belong.
	**
	** In some search modes, the dm1bxsearch call returns NOPART if the
	** indicated key value is not found.  This indicates that we may need
	** to continue searching on the leaf's overflow chain (if it has one).
	** In RANGE or EXACT search modes the loop below may alter our leaf
	** pointer to point to an overflow page rather than the original leaf.
	**
	** For NonUnique primary btree (V2), dups can span leaf pages.
	*/
	state = E_DB_OK;    
	if (direction == DM1C_EXACT || mode == DM1C_RANGE)
	{
	    for (;;)
	    {
		s = dm1bxsearch(r, *current, mode, direction, leaf_key, 
			    keys_given, tid, &tidpartno,
			    &pos, NULL, dberr); 

		/* If the row was found, then break out of the search loop */
		if (s == E_DB_OK)
		    break;

		/*
		** If NOPART was returned then we may need to continue searching
		** down the overflow chain.
		**
		** If we are in DM1C_TID mode (looking for an exact match) then
		** we keep searching until we either find the indicated row or
		** we reach the end of the overflow chain.
		**
		** If we are in RANGE mode (looking for the spot at which to
		** start scanning for matching rows) and NOPART is returned
		** then we may need to continue looking down the overflow chain
		** to set the range position.  If the current page has no keys
		** then we need to continue to the next overflow page since
		** we do not yet know whether the key range should include the
		** overflow duplicate key.  Once a page with a key is found we
		** determine immediately if the key range should include the
		** overflow chain since all keys on it are duplicates.
		**
		** If NOPART is returned in RANGE mode on a non-empty leaf or
		** overflow then we avoid searching all the way to the end of
		** the chain and set the position directly to the next leaf
		** page using the bt_nextpage pointer.  If we are already on
		** the rightmost leaf page (or chain) then we have no choice
		** but to follow the chain to the end since we do not have
		** a pointer to the last page.
		*/
	
		if ((mode == DM1C_TID || mode == DM1C_RANGE) &&
		    (dberr->err_code == E_DM0056_NOPART) &&
		    (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
		    *current) != 0))
		{
		    search_next = TRUE;
		    next = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
				*current);

		    if ((mode == DM1C_RANGE) &&
			(DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *current)
			> 0) &&
			(DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
			*current) != 0))
		    {
			search_next = FALSE;
			next = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
					*current);
			pos = 0;
		    }

		    s = dm0p_unfix_page(r, DM0P_UNFIX, &currentPinfo, dberr);
		    if (s != E_DB_OK)
			break;

                    s = dm0p_fix_page(r, next, leaf_fix_action,
				&currentPinfo, dberr);
		    if (s != E_DB_OK)
			break;

		    /*
		    ** If following the overflow chain, then continue to the
		    ** top of the loop to search the new page.
		    */
		    if (search_next)
			continue;
		}

		/*
		** NonUnique primary btree (V2), dups can span leaf
		** We may need to search additional leaf pages for the key
		*/
		if ( (!t->tcb_dups_on_ovfl) &&
		    (mode == DM1C_TID || mode == DM1C_RANGE) &&
		    ((t->tcb_rel.relstat & TCB_UNIQUE) == 0) &&
		    ((t->tcb_rel.relstat & TCB_INDEX) == 0) &&
		    (dberr->err_code == E_DM0056_NOPART) &&
		    (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
		    *current) != 0))
		{

		    if (mode == DM1C_RANGE)
		    {
			s = E_DB_OK;
			break;
		    }

		    /* mode == DM1C_TID */
		    s = dm1b_compare_key(r, *current, DM1B_RRANGE, leaf_key,
				keys_given, &compare, dberr);
		    if ((s != E_DB_OK) || (compare != DM1B_SAME))
			break;

		    next = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
				*current);

		    /*
		    ** If our key matches RRANGE, we should check
		    ** the next leaf
		    */
		    dm0pUnlockBuf(r, &currentPinfo);
		    s = dm0p_unfix_page(r, DM0P_UNFIX, &currentPinfo, dberr);
		    if (s != E_DB_OK)
			break;

                    s = dm0p_fix_page(r, next, leaf_fix_action,  
				&currentPinfo, dberr);
		    if (s != E_DB_OK)
			break;

		    if (mode == DM1C_FIND_SI_UPD ||
			    mode == DM1C_OPTIM || mode == DM1C_SPLIT)
		    {
			/*
			** Lock buffer for update.
			**
			** This will swap from CR->Root if "current" is a CR page.
			*/
			dm0pLockBufForUpd(r, &currentPinfo);
		    }
		    else
		    {
			/*
			** Lock buffer for get.
			**
			** This may produce a CR page and overlay "current"
			** with its pointer.
			*/
			dm0pLockBufForGet(r, &currentPinfo, &s, dberr);
		    }
		    if ( s == E_DB_ERROR )
			break;

		    continue;
		}
		break;
	    }

	    /*
	    ** If the above loop failed with an unexpected error, then
	    ** branch down to the error handling.
	    */
	    if ((s != E_DB_OK) && (dberr->err_code != E_DM0056_NOPART))
		break;
  
	    /* Save error code for return. */

	    state = dberr->err_code;
	}

	CLRDBERR(dberr);

	/* 
	** If mode is EXTREME, then the posotion must be set
	** to the first on the page for PREV and the last
	** on page for NEXT.
	*/

	if (mode == DM1C_EXTREME)
	{
	    if (direction == DM1C_PREV)
		pos = 0; 
	    else 
		pos = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
		    *current) - 1; 
	}

	if (mode == DM1C_LAST)
	{
	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *current))
	    {
		/* This leaf is the head of a duplicate overflow chain. */
		/* Find last page in chain so get-prev logic will work  */
		s = dm1b_find_ovflo_pg(r, leaf_fix_action, &currentPinfo, LAST_OVFL,
		    DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *current),
		    dberr);
		if (s != E_DB_OK)
		    break;
	    }
	    pos = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *current); 
	}

	/* 
	** Now in all cases the position on the leaf page 
	** is known and can be used to set limits. 
	*/
        bid->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    *current); 
	bid->tid_tid.tid_line = pos; 

	/*
	** Now that we have the final value for the leaf, pass it back
	** to the caller. Error handling following this point is a bit
	** complex. If we 'break', we will fall to the bottom and unfix fixed
	** pages; if we simply return (whether E_DB_OK or E_DB_ERROR),
	** we must have the correct page fixed.
	*/
	*leafPinfo = currentPinfo;

	/* 
	** If doing an optimistic search (not splitting on way down),
	** and there is no more room on the leaf page selected,
	** return an error indicating this.
	*/

	if (mode == DM1C_OPTIM)
	{
	    if (dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		*current, DM1B_INDEX_COMPRESSION(r),
		(i4)100 /* fill factor */,
		t->tcb_kperleaf, 
		(t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion + 
		  DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype,
		      *current))) == FALSE)
	    {
		SETDBERR(dberr, 0, E_DM0057_NOROOM);
		return(E_DB_ERROR);
	    }

	    /* if overflow chain and input key != chain key, cannot use page. */
	    if (DupKey != NULL && samekey != DM1B_SAME)
	    {
		SETDBERR(dberr, 0, E_DM0057_NOROOM);
		return(E_DB_ERROR);
	    }
	}

	/* 
	** If looking for an exact match, then make sure leaf record
	** key matches key we asked for. 
	*/

	if (mode == DM1C_FIND )
	{
	    if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *current) != 0)
	    {
		if (DupKey == NULL || samekey != DM1B_SAME)
		{
		    SETDBERR(dberr, 0, E_DM0056_NOPART);
		    return(E_DB_ERROR);
		}
	    }
	    else if (pos == DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
		    *current))
	    {
		if (t->tcb_dups_on_ovfl || (t->tcb_rel.relstat & TCB_UNIQUE))
		{
		    SETDBERR(dberr, 0, E_DM0056_NOPART);
		    return(E_DB_ERROR);
		}
		else
		{
		   s = dm1b_compare_key(r, *current, DM1B_RRANGE, leaf_key,
			keys_given, &compare, dberr);
		   if (s != E_DB_OK)
			break;

		   if (compare == DM1B_SAME)
		      s = E_DB_OK;
		   else
		   {
			SETDBERR(dberr, 0, E_DM0056_NOPART);
			return(E_DB_ERROR);
		   }
		}
	   } 
	   else
           {
		i4	    compare;
		char	    *entry_ptr, *AllocBuf = NULL;
		char	    entry_buf[DM1B_MAXSTACKKEY];
		i4	    entry_len;

		if ( record )
		    entry_ptr = record;
		else if ( s = dm1b_AllocKeyBuf(t->tcb_klen, entry_buf,
					    &entry_ptr, &AllocBuf, dberr) )
		    break;
		entry_len = t->tcb_klen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *current,
			&t->tcb_leaf_rac,
			pos, &entry_ptr, (DM_TID *)NULL, (i4*)NULL,
			&entry_len,
			NULL, NULL, adf_cb);

		if (s == E_DB_WARN && t->tcb_rel.relpgtype != TCB_PG_V1)
		{
		    s = E_DB_OK;
		}

		if (s != E_DB_OK)
		{
		    /* Discard any allocated key buffer */
		    if ( AllocBuf )
			dm1b_DeallocKeyBuf(&AllocBuf, &entry_ptr);
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *current,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, pos);
		    SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
		    break;
		}

		adt_kkcmp(adf_cb, keys_given, t->tcb_leafkeys,
				entry_ptr, leaf_key, &compare);

		/* Discard any allocated key buffer */
		if ( AllocBuf )
		    dm1b_DeallocKeyBuf(&AllocBuf, &entry_ptr);

		if (compare != DM1B_SAME)
		{
		    SETDBERR(dberr, 0, E_DM0056_NOPART);
		    return(E_DB_ERROR);
		}

		if ( record && entry_ptr != record )
		    MEcopy(entry_ptr, entry_len, record);
	    }
	}
	else
	{
	    if (mode == DM1C_TID)
		if ((dberr->err_code = state) != E_DB_OK)
                {
		    return (E_DB_ERROR);
                } 
	}

	return(E_DB_OK); 
    } 

    /* Discard any allocated index_key buffer */
    if ( AllocTkey )
	dm1b_DeallocKeyBuf(&AllocTkey, &index_key);

    /*	Handle error reporting and recovery. */

    if ((dberr->err_code > E_DM_INTERNAL) && (dberr->err_code != E_DM9CB1_RNL_LSN_MISMATCH))
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
    }

    /*
    ** If parent or current is still fixed, unfix it before returning. If
    ** either page is an index page, we can release the locks, unless we
    ** we in split mode, in which case we must hold all the locks.
    */

    if (*parent)
    {
	s = dm0p_unfix_page(r,
	    ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *parent) & 
	    DMPP_LEAF) || (mode == DM1C_SPLIT)) ? DM0P_UNFIX : DM0P_RELEASE,
	    &parentPinfo, &local_dberr);
	if (s)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &local_err_code, 0);
	}
    }
    if (*current)
    {

	dm0pUnlockBuf(r, &currentPinfo);
	s = dm0p_unfix_page(r,
	    ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *current) & 
	    DMPP_LEAF) || (mode == DM1C_SPLIT)) ? DM0P_UNFIX : DM0P_RELEASE,
	    &currentPinfo, &local_dberr);
	if (s)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &local_err_code, 0);
	}
    }

    return (E_DB_ERROR);

}

/*{
** Name: dm1badupcheck - Checks for duplicate record in BTREE.
**
** Description:
**      This routine check for duplicate records in a BTREE.
**      The given bid is the beginning of 0 or more duplicate keys
**      in the btree. These either span a segment of the given leaf, 
**      or the entire leaf and its associated overflow chain.
**
**      This routine assumes that the page indicated by leaf 
**      is only page fixed at entry an exit.
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf                            Pointer to the leaf page containing
**                                      possible duplicate.
**      bid                             Pointer to a value indicating where
**                                      the scan for duplicates should begin.
**      record                          Pointer to the record to insert.
**                                      list of key attribute pointers.
**      key                             Pointer to key to insert into leaf.
**
** Outputs:
**      tid                             Pointer to area to return tid if
**                                      duplicate found.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      The error codes are:
**                                      E_DM0046_DUPLICATE_RECORD, or
**                                      E_DM0045_DUPLICATE_KEY.
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
**          Fixes and unfixes overflow pages as goes down chain.
**
** History:
**      22-oct-85 (jennifer)
**          Converted for Jupiter.
**	 7-nov-88 (rogerk)
**	    Send compression type to dm1c_get routine.
**	14-nov-88 (teg)
**	    Initialized adf_cb.adf_maxstring = DB_MAXSTRING
**	29-nov-88 (rogerk)
**	    Make sure we unfix data page if encounter error.
**	24-Apr-89 (anton)
**	    Added local collation support
**	29-may-89 (rogerk)
**	    Added checks for bad rows in dm1c_get calls.
**	 1-mar-90 (walt)
**	    Fixed bug 20225 where first tuple of each overflow chain page
**	    (after the first page) wasn't being checked, and duplicate rows
**	    were being allowed into tables if they duplicated a row that
**	    wasn't being checked. 
**	    Fixed by setting leafpage variable after calling dm1bxchain
**	    rather than before, to make sure and get a new value of leaf if
**	    dm1bxchain has changed it.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree index compression.
**	    Be more efficient about fixing & unfixing data page(s) during
**	    search.
**	14-oct-1992 (jnash)
**	    Reduced logging project.
**	      - Add (unused) param's on dmpp_allocate calls.
**	      - Remove unused param's on dmpp_get calls.
**	      - Move compression out of dm1c layer, calling
**		tlv's here as required.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: allow for row versions on pages. Extract row
**          version and pass it to dmpp_uncompress. Call dm1r_cvt_row if
**          necessary to convert row to current version.
**	13-sep-1996 (canor01)
**	    Use the temporary tuple buffer allocated in the rcb for data
**	    uncompression.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added handling of deleted leaf entries (status == E_DB_WARN)
**      10-mar-97 (stial01)
**          Pass relwid to dm0m_tballoc()
**      07-apr-97 (stial01)
**          dm1badupcheck() Additional key parameter to dm1bxchain
**      21-may-97 (stial01)
**          dm1badupcheck() New buf_locked parameter.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          dm1badupcheck() Pass tlv to dm1cxget instead of tcb.
**	11-aug-97 (nanpr01)
**	    Add the t->tcb_comp_t->tcb_leafattcnt with relwid in dm0m_tballoc call.
**      05-oct-98 (stial01)
**          dm1badupcheck() If row locking, fix data page with readlock=nolock
**      05-may-1999 (nanpr01,stial01)
**          dm1badupcheck() Key value locks acquired for dupcheck are no longer 
**          held for duration of transaction. While duplicate checking,
**          request/wait for row lock if uncommitted duplicate key is found.
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in parameter
**          list.
**	22-Apr-2010 (kschendel) SIR 123485
**	    Process coupons if doing full row duplicate checking and there
**	    are blobs, otherwise dmpe now complains about lack of context.
*/
static DB_STATUS
dm1badupcheck(
    DMP_RCB		*rcb,
    DMP_PINFO		*leafPinfo,
    DM_TID		*bid,
    DM_TID		*tid,
    char		*record,
    char		*leafkey,
    bool                *requalify_leaf,
    bool                *redo_dupcheck,
    DB_ERROR		*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;   /* Table control block. */
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    i4              compare;
    char	    *nextkey;           /* Pointer to an area holding 
                                        ** next key from chain. */
    char	    *KeyPtr, *AllocKbuf = NULL;
    char	    key_buf[DM1B_MAXSTACKKEY];
    i4	    	    key_len;
    DM_PAGENO        pageno, savepageno; 
                                        /* Savepageno used to hold leaf
                                        ** page number so it can be
                                        ** refixed if go to overflow
                                        ** pages. */
    DB_STATUS	    s;                  /* Return status of called routines. */
    DB_STATUS       get_status;
    char            *wrec;
    char	    *wrec_ptr;
    i4         wrec_size;   /* Size of record retrieved off
                                        ** page. */
    i4         local_err_code;     
    i4		    row_version = 0;	
    DM_TID          localbid;
    DM_TID          localtid;
    ADF_CB          *adf_cb;
    u_i4	    row_low_tran = 0;
    u_i2	    row_lg_id = 0;
    i4              new_lock = FALSE;
    i4              lock_flag = 0;
    LK_LOCK_KEY	    save_lock_key;
    DB_ERROR	    error;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO	    dataPinfo;
    DMPP_PAGE	    **leaf;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
 
    /* Not for indexes */
    if (t->tcb_rel.relstat & TCB_INDEX)
        return (E_DB_OK);

    if (r->rcb_tupbuf == NULL)
	r->rcb_tupbuf = dm0m_tballoc((t->tcb_rel.relwid + 
					t->tcb_data_rac.worstcase_expansion));
    if ((wrec = r->rcb_tupbuf) == NULL)
    {
	SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
	return (E_DB_ERROR);
    }

    adf_cb = r->rcb_adf_cb;
    localbid.tid_i4 = bid->tid_i4;
    localtid.tid_i4 = tid->tid_i4;

    DMP_PINIT(&dataPinfo);

    /* Save leaf page number in case have to refix. */
    savepageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

    for (;;)
    {
	/* 
	** If index record pointer indicates record 
	** not on this page, go to overflow page.
	*/

	if ((i4)(localbid.tid_tid.tid_line) == 
	   DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf))
	{
	    s = dm1bxchain(r, leafPinfo, &localbid, (i4)FALSE, leafkey, dberr);
	    if (s != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    return (E_DB_OK);
		}
		break;
	    }
	}

	/*
	** Get key and tid of next record in chain.
	*/
	if ( s = dm1b_AllocKeyBuf(t->tcb_klen, key_buf,
				    &KeyPtr, &AllocKbuf, dberr) )
	    break;
	nextkey = KeyPtr;
	key_len = t->tcb_klen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			localbid.tid_tid.tid_line, &nextkey, 
			&localtid, &r->rcb_partno,
			&key_len, &row_low_tran, &row_lg_id, adf_cb);

	get_status = s;

	if (s == E_DB_WARN && t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    /* Deleted entry with different key ends dup checking */
	    s = E_DB_OK;
	}

	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
		    localbid.tid_tid.tid_line );
	    break;
	}

	/* Loop over matching leaf page keys checking corresponding records. */

	CLRDBERR(&error);

	for (s = E_DB_OK;;)
	{
	    adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys,
		    leafkey, nextkey, &compare);

	    if (compare != DM1B_SAME)
		break;

	    if (DM1B_DUPCHECK_NEED_ROW_LOCK_MACRO(r, *leaf, row_lg_id, row_low_tran))
	    {
		dm0pUnlockBuf(r, leafPinfo);

		if ( crow_locking(r) )
		{
		    /* this will get PHYS lock and then unlock */
		    s = dm1r_crow_lock(r, DM1R_LK_PHYSICAL, &localtid, NULL, dberr);
		}
		else
		{
		    /* Don't clear lock coupling in rcb */
		    /* Need to lock data page in advance of reading it */
		    /*
		    ** Note that for Global SI, dm1cxget, above,
		    ** extracted the partition number into rcb_partno,
		    ** needed by dm1r_lock_row.
		    */
		    lock_flag = DM1R_LK_PHYSICAL |
				DM1R_ALLOC_TID | DM1R_LK_PKEY_PROJ;
		    s = dm1r_lock_row(r, lock_flag, &localtid, 
					&new_lock, &save_lock_key, dberr);
		    if (s == E_DB_OK)
			_VOID_ dm1r_unlock_row(r, &save_lock_key, &local_dberr);
		}
		if ( s != E_DB_OK )
		    break;
		*redo_dupcheck = TRUE;
		break;
	    }

	    if (get_status == E_DB_OK)
	    {
		pageno = localtid.tid_tid.tid_page; 
		if (dataPinfo.page && DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, dataPinfo.page) 
		    != pageno)
		{
		    /*
		    ** Desired row is on different page. Unfix old data page.
		    */
		    dm0pUnlockBuf(r, &dataPinfo);
		    s = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, dberr);
		    if (s != E_DB_OK)
			break;
		}
		if (dataPinfo.page == NULL)
		{
		    s = dm0p_fix_page(r, pageno, DM0P_READ | DM0P_NOREADAHEAD,
					&dataPinfo, dberr);	
		    if (s != E_DB_OK)
			break;

		    /*
		    ** Lock buffer for update.
		    **
		    ** This will swap from CR->Root if "data" is a CR page.
		    */
		    dm0pLockBufForUpd(r, &dataPinfo);
		}

		wrec_size = t->tcb_rel.relwid;
		s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize, dataPinfo.page, &localtid, &wrec_size,
		    &wrec_ptr, &row_version, NULL, NULL, (DMPP_SEG_HDR *)NULL);

		/*
		** If row locking, we have a value lock, so we should
		** not need to look at deleted records
		*/
		if ((row_locking(r) || crow_locking(r)) && s == E_DB_WARN)
		{
		    s = E_DB_OK;
		    break;
		}

		/* Additional processing if compressed, altered, or segmented */
		if (s == E_DB_OK &&
		    (t->tcb_data_rac.compression_type != TCB_C_NONE ||
		    (t->tcb_rel.relencflags & TCB_ENCRYPTED) ||
		    row_version != t->tcb_rel.relversion ||
		    t->tcb_seg_rows))
		{
		    s = dm1c_get(r, dataPinfo.page, &localtid, wrec, dberr);
		    if (s && dberr->err_code != E_DM938B_INCONSISTENT_ROW)
			break;
		    wrec_ptr = wrec;
		}
		if (s == E_DB_OK && t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
		{
		    /* Apply context to short-term part of coupon in row
		    ** that we're checking against.
		    */
		    if (wrec_ptr != wrec)
		    {
			MEcopy(wrec_ptr, t->tcb_rel.relwid, wrec);
			wrec_ptr = wrec;
		    }
		    s = dm1c_pget(t->tcb_atts_ptr, r, wrec_ptr, dberr);
		}

		/* If there are encrypted columns, decrypt the record */
		if (s == E_DB_OK &&
		    t->tcb_rel.relencflags & TCB_ENCRYPTED)
		{
		    s = dm1e_aes_decrypt(r, &t->tcb_data_rac, wrec_ptr, wrec,
				r->rcb_erecord_ptr, dberr);
		    if (s != E_DB_OK)
			break;
		    wrec_ptr = wrec;
		}

		if (s != E_DB_OK)
		{
		    dm1b_badtid(r, &localbid, &localtid, leafkey);

		    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
		    break;
		}
		adt_tupcmp(adf_cb, (i4)t->tcb_rel.relatts, t->tcb_atts_ptr,
		    record, wrec_ptr, (i4 *)&compare);

		if (compare == 0)
		{
		    SETDBERR(&error, 0, E_DM0046_DUPLICATE_RECORD);
		    break;
		}
	    }

	    /* Get next duplicate key, repeat comparison. */	

	    s = dm1bxchain(r, leafPinfo, &localbid, (i4)FALSE, leafkey, dberr);
	    if (s != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		    s = E_DB_OK;
		break;
	    }
	    nextkey = KeyPtr;
	    key_len = t->tcb_klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
		    &t->tcb_leaf_rac,
		    localbid.tid_tid.tid_line, &nextkey, 
		    &localtid, &r->rcb_partno,
		    &key_len,
		    &row_low_tran, &row_lg_id, adf_cb);

	    get_status = s;

	    if (s == E_DB_WARN && t->tcb_rel.relpgtype != TCB_PG_V1)
	    {
		s = E_DB_OK;
	    }

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			localbid.tid_tid.tid_line );
		break;
	    }

	}

	/* Discard any allocated key buffer */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

	if (s != E_DB_OK)
	    break;

	if (dataPinfo.page)
	{
	    dm0pUnlockBuf(r, &dataPinfo);

	    s = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, &local_dberr);
	    if (s != E_DB_OK)
		break;
	}

	/* Fix the original leaf page. */

	if (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) != 
	    savepageno) 
	{
	    *requalify_leaf = TRUE;
	    dm0pUnlockBuf(r, leafPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, &local_dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_fix_page(r, savepageno, 
                    (DM0P_WRITE | DM0P_NOREADAHEAD), leafPinfo, &local_dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if "leaf" is a CR page.
	    */
	    dm0pLockBufForUpd(r, leafPinfo);
	}

	if (error.err_code == 0)
	    return (E_DB_OK);

	*dberr = error;
	tid->tid_i4 = localtid.tid_i4;
	return (E_DB_ERROR);
    } 

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    /*
    ** Don't leave data page fixed if encountered error. 
    */
    if (dataPinfo.page)
    {
	dm0pUnlockBuf(r, &dataPinfo);
	s = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, &local_dberr);
    }

    /*	Handle error reporting. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM925E_DM1B_ALLOCATE);
    }
    return (E_DB_ERROR); 
}

/*{
** Name: next - Gets next entry in BTREE.
**
** Description:
**      This routines searches the btree index pages starting from
**      the page and record indicated from the rcb->rcb_lowtid(bid). 
**      The leaf page associated with this bid is assumed to be fixed on 
**      entry and exitin rcb->rcb_other.page. The direction to search 
**      assumed to be next.  The limit of the search is based on high key 
**      value stored in rcb->rcb_hl_ptr. If an entry is found within 
**      the limits of search then the bid is updated to point to the 
**      new record.  When the next record's key is not less than or equal 
**      to the high key, then the end of scan is reached.
**
**      As you progress through the tree, old leaf pages are 
**      unfixed in memory as you acquire new ones.  They are
**      left locked.
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**
** Outputs:
**      err_code                        Pointer to an area to return 
**                                      the following errors when 
**                                      E_DB_ERROR is returned.
**                                      E_DM0055_NONEXT
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    Leaves last leaf page accessed fixed and locked and
**          updates the scan information in rcb.
**
**	    If Global Index, RCB's rcb_partno will contain
**	    the TID's partition number.
**
** History:
**      08-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	14-nov-88 (teg)
**	    Initialized adf_cb.adf_maxstring = DB_MAXSTRING
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	20-dec-1990 (bryanp)
**	    Call dm1cx() routines to support Btree index compression.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Skip deleted leaf entries;
**          If phantom protection is needed, set DM0P_PHANPRO fix action.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      21-may-97 (stial01)
**          next() New buf_locked parameter.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          next() Pass tlv to dm1cxget instead of tcb.
**          ifdef DEV_TEST: validate attribute info on leaf
**      07-jul-98 (stial01)
**          next() B84213: Defer row lock request for committed data
**      12-aug-98 (stial01)
**          next() ROW_ACC_KEY if checking high key only (not low key)
**      05-may-1999 (nanpr01)
**          next() If serializable with equal predicate, do not specify
**          (phantom protection) when fixing leaf
**      02-oct-2002 (stial01)
**          next() compare rcb_hl_ptr to RRANGE entry if empty leaf (b108852)
**	13-dec-04 (inkdo01)
**	    Tiny change to support union of prec/collID in DM1B_ATTR.
**	13-Apr-2006 (jenjo02)
**	    For CLUSTERED tables, the leaf entry is the row.
*/
static	DB_STATUS
next(
    DMP_RCB            *rcb,
    u_i4	       *low_tran_ptr,
    u_i2	       *lg_id_ptr,
    i4                 opflag,
    DB_ERROR           *dberr)
{
    DMP_RCB	 *r = rcb;
    DMP_TCB      *t = r->rcb_tcb_ptr;   /* Table control block. */
    DMP_TABLE_IO *tbio = &t->tcb_table_io;
    DM_TID       *bid;
    DM_TID       *tid;
    char         *key;
    char	 *rcbkeyptr;
    i4		 key_len;
    bool         start;                     /* Flag indicating start of
                                            ** chain. */
    i4      	 fix_action;                /* Fix action for pages. */
    DM_PAGENO    savepageno;
    DB_STATUS    s;			    /* Routine return status. */ 
    bool         getnextovfl;
    bool         getnextleaf;
    ADF_CB	 *adf_cb;
    DB_STATUS    get_status;
    DB_STATUS    tmp_status;
    i4		 compare;
    i4		 key_cmp_cnt = 0;
    i4		 leaf_fix_cnt = 0;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    adf_cb = r->rcb_adf_cb;
    bid = &r->rcb_lowtid;
    tid = &r->rcb_currenttid;

    if ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) )
	rcbkeyptr = r->rcb_record_ptr;
    else
	rcbkeyptr = r->rcb_s_key_ptr;

    key = rcbkeyptr;

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;
    if (row_locking(r) && serializable(r))
	if ((r->rcb_p_hk_type != RCB_P_EQ) || (r->rcb_hl_given != t->tcb_keys))
	    fix_action |= DM0P_PHANPRO; /* phantom protection is needed */
    if (opflag & DM1C_PKEY_PROJECTION)
	fix_action |= DM0P_NOREADAHEAD;
    start = FALSE; 
    getnextovfl = FALSE;
    getnextleaf = FALSE;
    if (bid->tid_tid.tid_line == DM_TIDEOF)
        start = TRUE; 
    if (bid->tid_tid.tid_page != 
	DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	r->rcb_other.page))
    {
	SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	return (E_DB_ERROR);
    }

    /* Get next qualifying record, or return end of scan. */
    for (;;)
    {
	if (start == TRUE )
	{
	    if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) > 0)
	    {
		bid->tid_tid.tid_line = 0;
		start = FALSE;
	    }
	    else if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
						r->rcb_other.page) != 0)
		getnextovfl = TRUE;
	    else if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
						r->rcb_other.page) != 0)
		getnextleaf = TRUE;
	    else
	    {
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);	
	    }
        } /* end if true */
	else
	{
	    /* 
	    ** If not starting and current page has more
	    ** entries than the current one, point to next one
	    ** on this page.
	    */
	    if ((i4)(bid->tid_tid.tid_line) < 
		DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
						r->rcb_other.page) - 1)
		bid->tid_tid.tid_line++; 

	    /* 
	    ** Need to go to overflow page, if there are no more, 
	    ** then indicate end of scan. 
	    */
	    else if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
						r->rcb_other.page) != 0)
		getnextovfl = TRUE;
	    else if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
						r->rcb_other.page) != 0)
		getnextleaf = TRUE;
	    else 
	    {
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	} /* end if else */
	if (getnextovfl == FALSE && getnextleaf == FALSE)
	{
	    /* Check entry to see if it is valid record. */
	    key = rcbkeyptr;
	    key_len = t->tcb_klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page,
			&t->tcb_leaf_rac,
			bid->tid_tid.tid_line, &key, tid, &r->rcb_partno,
			&key_len,
			low_tran_ptr, lg_id_ptr, adf_cb);

	    get_status = s;

	    if (get_status == E_DB_WARN)
	    {
		if (DM1B_SKIP_DELETED_KEY_MACRO(r, r->rcb_other.page, *lg_id_ptr, *low_tran_ptr))
		    continue;
		else
		    s = E_DB_OK; /* can't skip uncommitted deteted key yet */
	    }

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, r->rcb_other.page,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
				bid->tid_tid.tid_line );
		SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
		return (s);
	    }

	    /*
	    ** For primary btree, skip 'reserved' leaf entries
	    ** which have invalid tid
	    */
	    if ( (t->tcb_rel.relpgtype != TCB_PG_V1)
		&& ((t->tcb_rel.relstat & TCB_INDEX) == 0)
		&& (tid->tid_tid.tid_page == 0)
		&& (tid->tid_tid.tid_line == DM_TIDEOF))
	    {
		continue;
	    }

	    /* If CLUSTERED, this copies entire (uncompressed) row */
	    if ( key != rcbkeyptr )
		MEcopy(key, key_len, rcbkeyptr);

	    /*
	    ** NOTE if row locking we can do key qualification before 
	    ** locking the row.
	    ** We do not do in-place replace, and we do not reclaim space
	    ** for allocate by the same transaction...
	    */

	    /* 
	    ** Compare this record's key (key) with high key (rcb->rcb_hl_ptr).
	    ** If the record's key greater than the high, end of scan reached.
	    ** If the high key was not given (rcb->rcb_hl_given = 0) then
            ** this is a scan all, don't compare.
	    */
	    if (r->rcb_hl_given > 0)
	    {
		adt_kkcmp(adf_cb, r->rcb_hl_given, t->tcb_leafkeys,
				rcbkeyptr, r->rcb_hl_ptr, &compare);

		if (compare > DM1B_SAME)
		{
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    return (E_DB_ERROR);
		}

		key_cmp_cnt++;
		/* reset leaf_fix_cnt whenever we do a key comparison */
		leaf_fix_cnt = 0;
	    }

	    /* RCB_CONSTRAINT should see new rows */
	    if ((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
		&& (r->rcb_state & RCB_CONSTRAINT) == 0
		&& dm1cx_isnew(r, r->rcb_other.page, (i4)bid->tid_tid.tid_line))
                continue;
	    else	
                return(get_status);
	}

	/*
	** If we have fixed more than one leaf page and haven't found any
	** keys, compare the high key value to the RRANGE key on this leaf
	** to see if we've reached the end of the scan (b108852)
	*/
	if (r->rcb_hl_given > 0 && key_cmp_cnt == 0 && leaf_fix_cnt > 1)
	{
	    i4		 tempkeylen;
	    char	 *tempkey, *AllocKbuf;
	    char	 temp_buf[DM1B_MAXSTACKKEY];

	    if ( s = dm1b_AllocKeyBuf(t->tcb_rngklen, temp_buf,
					&tempkey, &AllocKbuf, dberr) )
		break;
	    tempkeylen = t->tcb_rngklen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page,
			t->tcb_rng_rac,
			DM1B_RRANGE,
			&tempkey, (DM_TID *)NULL, (i4*)NULL, &tempkeylen,
			NULL, NULL, adf_cb);
	    if (s == E_DB_OK)
	    {
		/* Compare range key :: leafkey */
		adt_ixlcmp(adf_cb, r->rcb_hl_given, 
			    t->tcb_rngkeys, tempkey, 
			    t->tcb_leafkeys, r->rcb_hl_ptr, &compare);

	    }

	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &tempkey);

	    if ( s )
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, r->rcb_other.page,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_RRANGE);
		SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
		break;
	    }

	    /* if (RRANGE > r->rcb_hl_ptr) end of scan */
	    if (compare > DM1B_SAME)
	    {
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	}
	
        /* Free previous page, get next one, either overflow or next leaf.*/

	if (getnextovfl == TRUE)
	    savepageno = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
						  r->rcb_other.page);
	else
	    savepageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
						  r->rcb_other.page);

	dm0pUnlockBuf(r, &r->rcb_other);

	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	if (s != E_DB_OK)
	    break;

	/* Get next page on chain. */
	s = dm0p_fix_page(r, savepageno, fix_action, &r->rcb_other, dberr);
	if (s != E_DB_OK)
	    break;
	leaf_fix_cnt++;

#ifdef xDEV_TEST
	/*
	** Validate leaf key description
	** Note when scanning btree table, we expect atr->key to be init
	** This is not the case when formatting the page during 
	** create index/modify
	*/
	if (t->tcb_rel.relpgtype != TCB_PG_V1)
	{
	    DM1B_ATTR    attdesc;
	    i4      page_attcnt;
	    i4      attno;
	    DB_ATTS      **atts;
	    DB_ATTS      *atr;           /* Ptr to current attribute */
	    i4      keyno = 1;

	    page_attcnt = DM1B_VPT_GET_BT_ATTCNT(t->tcb_rel.relpgtype, 
			r->rcb_other.page);
	    if (page_attcnt != t->tcb_leaf_rac.att_count)
		TRdisplay("BTREE leaf page key count error rac:%d page:%d\n",
			t->tcb_leaf_rac.att_count, page_attcnt);
	    for (attno = 1, atts = atts_array; attno <= page_attcnt; attno++)
	    {
		atr = *(atts++);
		DM1B_VPT_GET_ATTR(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
				r->rcb_other.page, attno, &attdesc);
		/* Compare to atts_array */
		if (attdesc.type != atr->type 
		    || attdesc.len != atr->length
		    || (attdesc.u.prec != atr->precision
			&& attdesc.u.collID != atr->coll_ID)
		    || attdesc.key != atr->key )
		{
		    TRdisplay(
		  "BTREE leaf page attdesc error (%d,%d,%d,%d,%d),(%d,%d,%d,%d)\n",
			atr->type, atr->length, atr->precision, atr->key, atr->collID,
			attdesc.type, attdesc.len, attdesc.u.prec, attdesc.key);
		}
		if (atr->key == keyno)
		    keyno++;
	    }
	    if (keyno <= t->tcb_keys)
		TRdisplay("BTREE leaf without all key information\n");
	}
#endif

	/*
	** Lock buffer for get.
	**
	** This may produce a CR page and overlay "r->rcb_other.page"
	** with its pointer.
	*/
	dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
	if ( s == E_DB_ERROR )
	    break;

	bid->tid_tid.tid_page = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page);
	bid->tid_tid.tid_line = (DM_TIDEOF);

	/*
	** It is always valid to move RCB_P_GET position to DM_TIDEOF
	** on a new leaf
	** (btree reposition can always start here)
	*/
        DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, bid, RCB_P_GET);

	start = TRUE;
	/* reset key_cmp_cnt whenever we start a new leaf */
	key_cmp_cnt = 0;
	getnextovfl = FALSE;
	getnextleaf = FALSE;
    }

    /*	Handle error reporting and recovery. */
    dm0pUnlockBuf(r, &r->rcb_other);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
    }
    return (E_DB_ERROR);
}


/*{
** Name: prev - Gets previous entry in BTREE. Reverse of next().
**
** Description:
**      This routines searches the btree index pages backwards starting
**      from the page and record indicated in the rcb->rcb_lowtid(bid).
**      The leaf page associated with this bid is assumed to be fixed upon
**      entry and exit in rcb->rcb_other.page. There is currently no
**      limit on the search (like next() has) but one could eventually be
**      added based on entries similar to the high key value stored
**      in rcb->rcb_hl_ptr. When an entry is found the bid is updated to
**      point to the previous record.  For now, end of scan is when the
**      beginning of the table is reached. Duplicate ovflo pages are 
**      assumed to contain unordered entries, and are therefore read forwards.
**
**      As you progress through the tree, old leaf pages are
**      unfixed in memory as you acquire new ones.  New ones are
**      left locked.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      the following errors when
**                                      E_DB_ERROR is returned.
**                                      E_DM0055_NONEXT
**
**      Returns:
**
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**      Exceptions:
**          none
**
** Side Effects:
**          Leaves last leaf page accessed fixed and locked and
**          updates the scan information in rcb.
**
**	    If Global Index, RCB's rcb_partno will contain
**	    the TID's partition number.
**
** History:
**      24-aug-94 (cohmi01)
**          Added for Manman/X support. Adapted from next().
**	16-may-95 (cohmi01)
**	    When search for prev leaf, dont pass addr of other_page_ptr,
**	    as dm1b_search will unfix it, we need it fixed, so use local.
**	    Fix backwards on duplicates to read in reverse order of chain.
**	19-jun-95 (lewda02)
**	    Cannot keep local page pointer.  No need to keep current leaf
**	    fixed while searching for prev leaf.  Just recall page number
**	    and compare with result.
**	11-aug-95 (shust01)
**	    If DM_TIDEOF is set when we first enter this routine, it means
**	    we are beginning of table.  Since we are doing prev(), we can
**	    assume that we are going to go beyond the beginning of table,
**	    so just return an error.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**	25-sep-96 (cohmi01)
**	    Save LRANGE key obtained from dm1cxget() in local buffer. 78751.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Skip deleted leaf entries;
**          If phantom protection is needed, set DM0P_PHANPRO fix action.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      21-may-97 (stial01)
**          prev() New buf_locked parameter.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          prev() Pass tlv to dm1cxget instead of tcb.
**      07-jul-98 (stial01)
**          prev() B84213: Defer row lock request for committed data
**	23-jul-1998 (nanpr01)
**	    Check for qualification in high key qualification is provided.
**      05-may-1999 (nanpr01)
**          prev() If serializable with equal predicate, do not specify
**          (phantom protection) when fixing leaf
**	13-Apr-2006 (jenjo02)
**	    For CLUSTERED tables, the leaf entry is the row.
*/
static DB_STATUS
prev(
    DMP_RCB            *rcb,
    u_i4               *low_tran_ptr,
    u_i2	       *lg_id_ptr,
    i4                 opflag,
    DB_ERROR           *dberr)
{
    DMP_RCB     *r = rcb;
    DMP_TCB     *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO *tbio = &t->tcb_table_io;
    DM_TID       *bid;
    DM_TID       *tid;
    char         *key;
    char         *rcbkeyptr;
    i4     	 key_len;
    char         *KeyPtr = NULL, *AllocKbuf = NULL;
    char         keybuf[DM1B_MAXSTACKKEY];
    i4     	 keys_given;        /* # keys less tidp */
    char         *tmpkey;
    i4     	 tmpkey_len;
    bool         start;                     /* how to access new pages */
    i4      	 fix_action;                /* Fix action for pages. */
    DB_STATUS    s;                         /* Routine return status. */
    bool         got_tid;                   /* did we have a legal tid */
    ADF_CB       *adf_cb;
    DM_PAGENO    nextpg;                 /* the next page in ovflo chain */
    DM_PAGENO    savepg;                 /* record cur page - for err msg */
    i4      	 infinity;
    DB_STATUS    get_status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Set up attributes array and count correctly, depending on whether
    ** this is a secondary index or not
    */
    /*
    ** Adjust the key to include only the fields needed for duplicate checking.
    ** If this is a Release 6 secondary index, then ignore the TIDP field
    ** for duplicate checking as including it will cause us to never see
    ** duplicates.  Pre-release 6 secondary indexes did not include the TIDP
    ** field as part of the keyed fields.
    **
    ** Check the tcb's attribute array to see if the TIDP is included in the
    ** key (the TIDP is the last attribute).  If the TIDP is listed as a
    ** keyed field, then decrement the count of keyed columns so that it
    ** will be ignored. (Decrementing the count causes us to ignore the TIDP
    ** since it is always the last keyed column).
    */
    keys_given = t->tcb_keys;

    if (t->tcb_rel.relstat & TCB_INDEX)
    {
        if (t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0)
            keys_given--;
    }


    adf_cb = r->rcb_adf_cb;
    bid = &r->rcb_lowtid;
    tid = &r->rcb_currenttid;

    if ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) )
	rcbkeyptr = r->rcb_record_ptr;
    else
	rcbkeyptr = r->rcb_s_key_ptr;

    fix_action = r->rcb_access_mode == RCB_A_READ ? DM0P_READ : DM0P_WRITE;

    if (row_locking(r) && serializable(r))
	if ((r->rcb_p_hk_type != RCB_P_EQ) || (r->rcb_hl_given != t->tcb_keys))
	    fix_action |= DM0P_PHANPRO; /* phantom protection is needed */

    if (opflag & DM1C_PKEY_PROJECTION)
	fix_action |= DM0P_NOREADAHEAD;

    /* 
    ** If we come into this routine as TIDEOF, then we are positioned
    ** at beginning of table.  Since we are going prev(), just return
    ** an error.
    */
    if (start = (bid->tid_tid.tid_line == DM_TIDEOF))
    {
	SETDBERR(dberr, 0, E_DM0055_NONEXT); /* reached begining of table */
        return (E_DB_ERROR);
    }

    if (bid->tid_tid.tid_page != 
	DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page))
    {
	SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
        return (E_DB_ERROR);
    }

    /* Get next qualifying record, or return end of scan. */

    for (;;)
    {
        got_tid = FALSE;
        if (start)
        {
            /* 'starting' on prev page - if any entries, use last one */
            if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
		 r->rcb_other.page) > 0)
            {
                bid->tid_tid.tid_line = 
		    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) -1;
                start = FALSE;
                got_tid = TRUE;
            }
        }
        else
        {
            /* not 'starting' a page - if entries left, use previous one */
            if ((i4)(bid->tid_tid.tid_line) > 0)
            {
                bid->tid_tid.tid_line--;
                got_tid = TRUE;
            }
        }

        if (got_tid)
        {
            /* Check entry to see if it is valid record, if so - return */
            key = rcbkeyptr;
            key_len = t->tcb_klen;
            s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page,
			&t->tcb_leaf_rac,
			bid->tid_tid.tid_line, 
			&key, tid, &r->rcb_partno,
			&key_len, low_tran_ptr, lg_id_ptr, adf_cb);

	    get_status = s;

	    if (get_status == E_DB_WARN)
	    {
		if (DM1B_SKIP_DELETED_KEY_MACRO(r, r->rcb_other.page, *lg_id_ptr, *low_tran_ptr))
		    continue;
		else
		    s = E_DB_OK; /* can't skip uncommitted deteted key yet */
	    }

            if (s != E_DB_OK)
            {
		/* Discard any allocated key buffer */
		if ( AllocKbuf )
		    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);
                dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, r->rcb_other.page,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			    bid->tid_tid.tid_line );
		SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
                return (s);
            }

	    /*
	    ** For primary btree, skip 'reserved' leaf entries
	    ** which have invalid tid
	    */
	    if ( (t->tcb_rel.relpgtype != TCB_PG_V1)
		&& ((t->tcb_rel.relstat & TCB_INDEX) == 0)
		&& (tid->tid_tid.tid_page == 0)
		&& (tid->tid_tid.tid_line == DM_TIDEOF))
	    {
		continue;
	    }

	    /* If CLUSTERED, this copies entire (uncompressed) row */
            if (key != rcbkeyptr)
                MEcopy(key, key_len, rcbkeyptr);

	    /*
	    ** Compare this record's key (key) with high key (rcb->rcb_hl_ptr).
	    ** If the record's key greater than the high, end of scan reached.
	    ** If the high key was not given (rcb->rcb_hl_given = 0) then
	    ** this is a scan all, don't compare.
	    */
	    if (r->rcb_ll_given > 0 && (opflag & DM1C_RAAT) == 0)
	    {
		i4         compare;

		adt_kkcmp(adf_cb, r->rcb_ll_given, t->tcb_leafkeys,
				rcbkeyptr, r->rcb_ll_ptr, &compare);
 
		if (compare < DM1B_SAME)
		{
		    /* Discard any allocated key buffer */
		    if ( AllocKbuf )
			dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    return (E_DB_ERROR);
		}
	    }

	    /* RCB_CONSTRAINT should see new rows */
            if ((r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r))
		&& (r->rcb_state & RCB_CONSTRAINT) == 0
		&& dm1cx_isnew(r, r->rcb_other.page, (i4)bid->tid_tid.tid_line))
                continue;
            else
	    {
		/* Discard any allocated key buffer */
		if ( AllocKbuf )
		    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);
                return(get_status);
	    }
        }

        /* If we are here, we did not 'get_tid' ok, need 'prev' page. */
	/* To handle duplicates: if current (exhausted) page is an    */
	/* ovflo page, get page pointing to it. If its a leaf, search */
	/* for its previous page using DM1B_LRANGE logic, then check  */
	/* if it has an ovflo chain - if so, find last page in chain. */

	if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page) & DMPP_CHAIN)
	{
	    /* This is a page in ovflo chain, but not the leaf that */
	    /* is at the head. Run thru chain starting from leaf,   */
	    /* till we find page that points to us (may be leaf)    */
	    s = dm1b_find_ovflo_pg(r, fix_action, &r->rcb_other, 
	    	PREV_OVFL, DM1B_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
		r->rcb_other.page), dberr);
	    if (s != E_DB_OK)
		break;
	}
        else
        {
            /* Need to find prev leaf, 1st check if we are at beginning */
            dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			r->rcb_other.page, 
			DM1B_LRANGE, (DM_TID *)&infinity, (i4*)NULL);
            if (infinity)
            {
		/* Discard any allocated key buffer */
		if ( AllocKbuf )
		    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);
		SETDBERR(dberr, 0, E_DM0055_NONEXT);  /* reached begining of table */
                return (E_DB_ERROR);
            }

            /* get LRANGE value of page, so prev pg can be searched for */
	    if ( !KeyPtr && (s = dm1b_AllocKeyBuf(t->tcb_rngklen, keybuf,
					    &KeyPtr, &AllocKbuf, dberr)) )
		break;
            tmpkey = KeyPtr;  /* where to put LRANGE key of this leaf */
            tmpkey_len = t->tcb_rngklen;
            s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    r->rcb_other.page,
		    t->tcb_rng_rac,
		    DM1B_LRANGE,
		    &tmpkey, (DM_TID *)NULL, (i4*)NULL, &tmpkey_len,
		    low_tran_ptr, lg_id_ptr, adf_cb);
	    /* key value needed long-term, save in our local buffer */
	    if (tmpkey != KeyPtr)
	    {
		MEcopy(tmpkey, tmpkey_len, KeyPtr);
		tmpkey = KeyPtr;  /* undo effect of dm1cxget */
	    }
            if (s != E_DB_OK)
            {
                dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, r->rcb_other.page,
		       t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_LRANGE);
		SETDBERR(dberr, 0, E_DM93D1_BDUPKEY_ERROR);
                break;
            }
            /* Search the btree for the previous leaf. Use the      */
            /* LRANGE key value, minus the tidp, in order to reduce */
            /* the lexical value so it locates the preceding page.  */
            /* To prevent problem with concurrent split, eg. prev page */
            /* split after getting getting LRANGE, but before reading, */
            /* keep old leaf locked until verifying that the new (prev)*/
            /* leaf actually points to 'old' leaf. Chase the forward   */
            /* chain if needed, to account for splits that occurred.   */

	    /* save pageno that the prev page should point to */
            savepg = DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_other.page) & DMPP_CHAIN ?
		DM1B_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_other.page) :
		/* out leafs page_page */
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_other.page);
		/* our page_page */
	    
	    /*
	    ** Must unfix our leaf now.  We only want one leaf fixed.
	    */
	    dm0pUnlockBuf(r, &r->rcb_other);

            s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
            if (s != E_DB_OK)
                break;

	    /* tmpkey is leaf key, we need index key to do a search */

	    /* FIXME: jenjo02 - I think this is bogus. btree_search
	    ** may think the key in ixkeybuf is a leaf key and
	    ** transform it again into another (wrong) INDEX key.
		dm1b_build_key(t->tcb_keys, t->tcb_leafkeys, tmpkey,
			t->tcb_ixkeys, ixkeybuf);

		s = btree_search(r, ixkeybuf, keys_given, DM1C_RANGE,
		   DM1C_PREV, &r->rcb_other, bid, tid, NULL, dberr);
	    ** end of FIXME */

            s = btree_search(r, tmpkey, keys_given, DM1C_RANGE,
               DM1C_PREV, &r->rcb_other, bid, tid, NULL, dberr);
            if (DB_FAILURE_MACRO(s))
            {
               if (dberr->err_code != E_DM0057_NOROOM)
                    break;
                s = E_DB_OK;
            }
            /* now make sure a split didnt happen in the interim */
            while (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page)
		&& DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_other.page)
		!= savepg)
            {
                nextpg = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, 
		    r->rcb_other.page);
		dm0pUnlockBuf(r, &r->rcb_other);
                s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
                if (s != E_DB_OK)
		    break;

                s = dm0p_fix_page(r, nextpg, fix_action, &r->rcb_other, dberr);
                if (s != E_DB_OK)
		    break;

		/*
		** Lock buffer for get.
		**
		** This may produce a CR page and overlay "rcb_other.page"
		** with its pointer.
		*/
		dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
		if ( s == E_DB_ERROR )
		    break;
            }
            if (s != E_DB_OK)
                break;   /* one of the calls in the while loop failed. */

	    if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
				r->rcb_other.page))
	    {
	    	/* This leaf is the head of an ovflo chain. Run thru the */
	    	/* entire chain, find last page of ovflo chain.          */
	    	s = dm1b_find_ovflo_pg(r, fix_action, &r->rcb_other, LAST_OVFL, 
		    DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
				r->rcb_other.page), dberr);
	    	if (s != E_DB_OK)
		    break;
	    }
        }   /* end - if page is a regular leaf and we re-searched index */
        start = TRUE;
        bid->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    r->rcb_other.page);
        bid->tid_tid.tid_line = (DM_TIDEOF);

	/*
	** It is always valid to move RCB_P_GET position to DM_TIDEOF
	** on a new leaf
	** (btree reposition can always start here)
	*/
        DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, bid, RCB_P_GET);


    }   /* end FORever - try to find prev tup on the (prev) page  */

    /*  Handle error reporting and recovery. */

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: findpage - Finds a page for a BTREE data insertion.
**
** Description:
**      This routines finds the associated data page of the found leaf
**      level page.  If this page has room for the record, it returns.
**      Otherwise, a new page is allocated and associated with the leaf
**      page.  The old full data page is diassociated.
**      The allocation of a new data page is logged for recovery.
**      If row locking, the leaf page is assumed to be locked by the caller 
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf                            Pointer to the leaf page.
**      record				record (if compression, compressed rec)
**      need                            Value indicating how much space
**                                      is needed on the data page.
** Outputs:
**      data                            Pointer to an area where data 
**                                      page pointer can be returned.
**	tid				Pointer to tid for allocated record.
**      err_code                        Pointer to an area to return
**                                      error code.  
**                                      The error codes are:
**                                      E_DM0019_BAD_FILE_WRITE and
**                                      errors from dm1bpgetfree.
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
**      25-oct-85 (jennifer)
**          Converted for Jupiter.
**       2-apr-87 (ac)
**          Added read/nolock support.
**	 3-nov-91 (rogerk)
**	    Added fixes for recovery on File Extend operations.
**	    Added dm0p_bicheck calls to log Before Image records if necessary
**	    for the leaf page and old data page during an ASSOC operation.
**	    These records are needed for UNDO and DUMP processing.
**	26-sep-1992 (jnash & rogerk)
**	    6.5 Recovery Project
**	      - Changed to use new recovery protocols.  Before Images are
**		no longer written and the pages updated by the allocate must
**		be mutexed while the dm0l_assoc log record is written and
**		the pages are updated.
**	      - Changed to pass new arguments to dm0l_assoc.
**	      - Add new param's to dmpp_allocate call.
**	      - Changed arguments to dm0p_mutex and dm0p_unmutex calls.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Fixes. Added dm0p_pagetype call.
**	02-jan-1993 (jnash)
**	    More reduced logging work.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dmpp_allocate accessor.
**	29-may-96 (nick)
**	    We were assigning the new page's configuration location number to 
**	    the old page's information and v.v. #76748 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**          Call dm1r_allocate() instead of dmpp_allocate
**          If row locking, allocate associated data page in mini transaction.
**      06-jan-97 (dilma04)
**          Removed unneeded DM1P_PHYSICAL flag when calling dm1p_getfree().
**      10-mar-97 (stial01)
**          findpage() Added record param, pass to dm1r_allocate()
**      21-may-97 (stial01)
**          findpage() Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      21-apr-98 (stial01)
**          findpage() Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**          findpage() extend table in mini transaction if extension table
**      19-Jan-99 (wanya01)
**          X-integrate change 431605 (Bug 76748) 
**          The previous change made by nick on 29-may-96 was lost in oping20.
**          Recovery of a Btree association record when the relation resides on 
**          multiple locations could fail.
**      12-aug-98 (stial01)
**          findpage() If leaf buffer is mutexed, Fix data page NOWAIT
**          findpage() set DM1P_PHYSICAL if physical locking (blob extensions)
**      22-sep-98 (stial01)
**          findpage() If row locking, downgrade/convert lock on new page  
**          after mini transaction is done (B92909)
**      05-oct-98 (stial01)
**          findpage() lock leaf buffer BEFORE reposition (12-aug-98 change)
**      18-feb-99 (stial01)
**          findpage() pass buf_locked to dm1r_allocate
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: must dm0p_lock_buf_macro when crow_locking
**	    to ensure the Root is locked.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/

static DB_STATUS
findpage(
    DMP_RCB	    *rcb,
    DMP_PINFO       *leafPinfo,
    DMP_PINFO       *dataPinfo,
    char            *record, /* if compression, this is compressed record */
    i4         need,
    char            *key,
    DM_TID	    *tid,
    DM_TID          *bid,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    CL_ERR_DESC      sys_err;
    LG_LRI	    lri_leaf;
    LG_LRI	    lri_odata;
    LG_LSN          lsn;
    LG_LSN          bm_lsn;
    DB_STATUS	    s;
    DB_STATUS	    local_status;
    STATUS	    cl_status;
    i4	    dm0l_flag;
    i4	    loc_id;
    i4	    leaf_conf_id;
    i4	    odata_conf_id;
    i4	    ndata_conf_id;
    i4	    local_err_code;
    i4         dm1p_flag = 0;
    bool            in_mini_trans = FALSE;
    i4         fix_action;     /* Type of access to page. */
    DM_PAGENO       assoc_datapg;
    DM_PAGENO	    prev_datapg = DM1B_ROOT_PAGE; /* impossible data page num */
    DM_PAGENO	    nextleaf;
    LK_LKID         newdata_lkid;
    i4		    retry;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    bool	    alloc_data = FALSE;
    bool	    alloc_bid_on_ovfl;
    DM_PAGENO	    alloc_bid_page;
    DM_PAGENO	    save_page;
    i4		    fp_repos = 0;
    i4		    dm1r_alloc = 0;
    DMPP_PAGE	    *orig_leaf = leafPinfo->page;
    i4		    buf_lock_err = 0;
    DMPP_PAGE	    **leaf, **data, **newdata;
    DMP_PINFO	    newdataPinfo;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    data = &dataPinfo->page;
    DMP_PINIT(&newdataPinfo);
    newdata = &newdataPinfo.page;

    /*
    ** On entry, leafpage is the correct leaf for findpage
    **
    ** if t->tcb_dups_on_ovfl and alloc_bid on an overflow page
    ** - the leaf fixed is the leaf at the head of the overflow chain
    */
    nextleaf = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf);
    save_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

    if (t->tcb_dups_on_ovfl && bid->tid_tid.tid_page != 
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf))
	alloc_bid_on_ovfl = TRUE;
    else
	alloc_bid_on_ovfl = FALSE;


    /*	See if the current data page, if any, is for the correct leaf. */

    for (retry = 0; ; retry++)
    {
	/* Lock leaf again and reposition */
	if ( !dm0pBufIsLocked(leafPinfo) )
	{
	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if "leaf" is a CR page.
	    */
	    dm0pLockBufForUpd(r, leafPinfo);
	}

	/*
	**
	** If leaf has split, reposition to correct leaf for findpage
	**
	** if t->tcb_dups_on_ovfl and alloc_bid on an overflow page
	** - the findpage leaf is the leaf at the head of the overflow chain
	** - reposition to leaf at the head of the overflow chain for dupkey
	** (yes the dupkey leaf can split, if the range keys allow 
	** keys other than the dupkey!)
	** This will swap from CR->Root if "leaf" is a CR page.
	*/
	if (nextleaf != 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf))
	{
	    if (!t->tcb_dups_on_ovfl || alloc_bid_on_ovfl == FALSE)
	    {
		/*	
		** dm1b_put will expect any leaf page fixed to match
		** the page specified by bid, reposition to alloc key
		*/
		s = btree_reposition(r, leafPinfo, bid, key,
		    RCB_P_ALLOC, 0, dberr);
	    }
	    else
	    {
		/*
		** If alloc bid was on an overflow page,
		** the findpage leaf is the head of the overflow chain
		** dm1b_put does not expect findpage to return with
		** the overflow page fixed
		*/
		s = findpage_reposition(r, leafPinfo, key, dberr);
		if (s != E_DB_OK)
		    break;
	    }

	    /* BTREE-ALLOCATE trace point dm608 */
	    if (DMZ_AM_MACRO(8) && save_page !=
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf))
		TRdisplay("findpage xid %x (%d,%d) repos leaf %d -> %d\n",
			r->rcb_tran_id.db_low_tran,
			t->tcb_rel.reltid.db_tab_base,
			t->tcb_rel.reltid.db_tab_index,
			save_page,
			DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));
	    save_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);
	
	    /* Re-init nextleaf */
	    nextleaf = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf);
	}

	assoc_datapg = DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, *leaf);

	/* break if this is same (full) data page */
	if (assoc_datapg == prev_datapg)
	{
	    alloc_data = TRUE;
	    break;
	}

	/*
	** Code below assumes that leaf is unlocked
	** - Fixing/lock data page may lock wait
	** - Alloc/row lock on data page may lock wait
	** When row locking, even if current leafpage splits while 
	** the buffer is unlocked, the assoc_datapg is a valid place 
	** to allocate space
	*/
	dm0pUnlockBuf(r, leafPinfo);

	if (*data && DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *data) 
	    !=  assoc_datapg)
	{
	    /*	Unfix this unusable page. */

	    s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, dberr);
	    if (s != E_DB_OK)
		return (s);
	}

	if (*data == 0)
	{
	    /* Fix the associated data page.    */
	    fix_action = DM0P_WRITE | DM0P_NOREADAHEAD;

	    s = dm0p_fix_page(r, assoc_datapg, fix_action, dataPinfo, dberr);
	    if (s != E_DB_OK)
		return (s);
	}

	/* Check if space is available on the associated data page. */

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "data" is a CR page.
	*/
	dm0pLockBufForUpd(r, dataPinfo);

	/* Record id, dbid and table id not needed for btrees */
	dm1r_alloc++;
	s = dm1r_allocate(r, dataPinfo, record, need, tid, dberr);

	if (s == E_DB_OK)
	{
	    /*
	    **  allocate on data page succeeded
	    **  break and return 
	    **  Its ok to return with leaf unlocked
	    */
	    break;
	}

	/*
	** If no space was found on this data page,
	** since we didn't keep the leaf buffer locked,
	** reposition to correct leaf before allocating a new data page
	*/
	if (s == E_DB_INFO)
	{
	    dm0pUnlockBuf(r, dataPinfo);
	    prev_datapg = assoc_datapg;
	    continue;
	}

	/* error */
        break;
    }

    /*
    **  dm1r_allocate() returned E_DB_INFO:
    **	Allocate a new associated data page, and mark the current associated
    **	data page as dis-associated.
    **  NOTE the leaf page fixed is the correct leaf for findpage
    **  If t->tcb_dups_on_ovfl, this is the head of the overflow chain
    **  (even though we may have allocated bid on an overflow)
    */
    for ( ; alloc_data == TRUE; )
    {
	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "data" is a CR page.
	*/
	dm0pLockBufForUpd(r, dataPinfo);

	if ( !dm0pBufIsLocked(leafPinfo) )
	    buf_lock_err++;

	if ( row_locking(r) || crow_locking(r) || NeedPhysLock(r) )
	{
	    if ( r->rcb_logging )
	    {
		s = dm0l_bm(r, &bm_lsn, dberr);
		if (s != E_DB_OK)
		    break;
		in_mini_trans = TRUE;
	    }

	    /*
	    ** Set flag to allocate call to indicate that the action is
	    ** part of a mini transaction.
	    */
	    dm1p_flag |= DM1P_MINI_TRAN;

	    if ( NeedPhysLock(r) )
		dm1p_flag |= DM1P_PHYSICAL;
	}

	/*
	** Call allocation code to get a free page from the free maps.
	** This will possibly trigger a File Extend as well.
	**
	** The newpage's buffer might be MLOCKed by dm1p_getfree and will
	** be unlocked by dm0p_unfix_page.
	*/
	s = dm1p_getfree(r, dm1p_flag, &newdataPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	STRUCT_ASSIGN_MACRO(r->rcb_fix_lkid, newdata_lkid);
	dm0p_pagetype(tbio, *newdata, r->rcb_log_id, DMPP_DATA);

	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, *leaf, dberr);
	    if (s != E_DB_OK)
	        break;
	    s = dm0p_before_image_check(r, *data, dberr);
	    if (s != E_DB_OK)
	        break;
	    s = dm0p_before_image_check(r, *newdata, dberr);
	    if (s != E_DB_OK)
	        break;
	}

	/*
	** Get information on the locations of the updates for the 
	** log call below.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
	    (i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));

	leaf_conf_id = tbio->tbio_location_array[loc_id].loc_config_id;

	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
	    (i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, (*data)));

	odata_conf_id = tbio->tbio_location_array[loc_id].loc_config_id;

	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
	    (i4) DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newdata));

	ndata_conf_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Log operation of adding new page on the overflow chain.
	** The Log Address of the OVERFLOW record is recorded on both
	** the new and old data pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2, 
		sizeof(DM0L_ASSOC) * 2, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9257_DM1B_FINDDATA);
		s = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Lock the buffers so nobody can look at them while they are 
	    ** changing.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, dataPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newdataPinfo);

	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same lock protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    /* Get CR info from leafpage, pass to dm0l_assoc */
	    DM1B_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, *leaf, &lri_leaf);
	    DMPP_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, *data, &lri_odata);

	    s = dm0l_assoc(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, 
		&t->tcb_rel.relid, t->tcb_relid_len,
		&t->tcb_rel.relowner, t->tcb_relowner_len,
		t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize,
		t->tcb_rel.relloccount,
		leaf_conf_id, odata_conf_id, ndata_conf_id,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf),
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, (*data)),
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newdata),
		(LG_LSN *)NULL, &lri_leaf, &lri_odata, dberr);
	    if (s != E_DB_OK)
	    {
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo); 
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, dataPinfo);
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newdataPinfo);
		break;
	    }

	    DM1B_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, *leaf, &lri_leaf);

	    DMPP_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, (*data), &lri_odata);
	    /*
	    ** "newdata" is new and has no page update history, so just 
	    ** record the assoc lsn, leaving the rest of the LRI information
	    ** on the page zero.
	    */
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, *newdata, lri_odata.lri_lsn);
	}
	else
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, dataPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newdataPinfo);
	}

	/* Format the new data page. */
	/* newdata->page_stat = 
	** DMPP_DATA | DMPP_ASSOC | DMPP_PRIM | DMPP_MODIFY;
	** Clear out newdata->page_stat, this may be unnecessary
	*/
	DMPP_VPT_INIT_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *newdata,
	    (DMPP_DATA | DMPP_ASSOC | DMPP_PRIM | DMPP_MODIFY));

	/* Mark the old data page as unassociated. */

	DMPP_VPT_CLR_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, (*data), DMPP_ASSOC);
	DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, (*data), DMPP_MODIFY);

	/* Associate new data page with leaf. */

        DM1B_VPT_SET_BT_DATA_MACRO(t->tcb_rel.relpgtype, *leaf, 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newdata)); 
	DM1B_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *leaf, DMPP_MODIFY);

	/*
	** Unlock the buffers after completing the updates.
	*/
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, dataPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newdataPinfo);

	/*
	** If within mini transaction, complete the Mini Transaction used
	** for the allocate. After this, the new page becomes a permanent
	** permanent addition to the table.
	*/
	if (in_mini_trans)
	{
	    if (r->rcb_logging)
	    {
		s = dm0l_em(r, &bm_lsn, &lsn, dberr);
		if (s != E_DB_OK)
		    break;
	    }
	}

#ifdef xDEBUG
	if (DMZ_AM_MACRO(7))
	{
	    TRdisplay("findpage: New data page is %d, leaf = %d\n",
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, (*data)), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));
	    TRdisplay("For table %t in database %t.\n",
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	 }
#endif

	/*
	** Now that the mini transaction is complete
	** it is safe to unlock the old assoc data page buffer
	*/
	dm0pUnlockBuf(r, dataPinfo);

	/*
	** Unfix the old associated data page and continue with the newly
	** allocated and associated one.
	*/
	s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, dberr);
	if (s != E_DB_OK)
	    break;
	*dataPinfo = newdataPinfo;
	DMP_PINIT(&newdataPinfo);

	/* 
	** If row locking, we have X lock on new data page, we can
	** unlock leaf buffer
	*/
	dm0pUnlockBuf(r, leafPinfo);

	/*
	** Allocate space on new page while we still hold X lock
	** and MLOCK newdata page. 
	*/
	s = dm1r_allocate(r, dataPinfo, record, need, tid, dberr);

	/*
	** Even if allocate failed, do the lock conversion below in
	** case we need to rollback 
	**
	** Note dm1r_lk_convert will also remove the MLOCK from newpage (data).
	*/
	if ( row_locking(r) || crow_locking(r) )
	{
	    local_status = dm1r_lk_convert(r, 
				DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *data),
	    			dataPinfo, &newdata_lkid, &local_dberr);

	    if (local_status != E_DB_OK)
	    {
		if (s == E_DB_OK)
		{
		    s = local_status;
		    *dberr = local_dberr;
		}
		else
		{
		    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&local_err_code, 0);
		}
	    }
	}

	break;
    } 

    dm0pUnlockBuf(r, dataPinfo);

    if (s != E_DB_OK)
    {
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9257_DM1B_FINDDATA);
	}

	/* Check if we have left pages fixed before returning */
	if (dataPinfo->page)
	    s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, &local_dberr);
	if (newdataPinfo.page)
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &newdataPinfo, &local_dberr);
	return (E_DB_ERROR);
    }

    return (s);
}


/*{
** Name: bulkpage - Allocate a new disassociated btree page for bulk insertion.
**
** Description:
**      This routines allocates a new disassociated btree page
**      to be used for bulk inserts.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf                            Pointer to the leaf page.
**      record				record (if compression, compressed rec)
**      need                            Value indicating how much space
**                                      is needed on the data page.
** Outputs:
**      data                            Pointer to an area where data 
**                                      page pointer can be returned.
**	tid				Pointer to tid for allocated record.
**      err_code                        Pointer to an area to return
**                                      error code.  
**                                      The error codes are:
**                                      E_DM0019_BAD_FILE_WRITE and
**                                      errors from dm1bpgetfree.
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
**      02-jan-2004 (stial01)
**          Created.
*/
static DB_STATUS
bulkpage(
    DMP_RCB	    *rcb,
    DMP_PINFO       *leafPinfo,
    DMP_PINFO       *dataPinfo,
    char            *record, /* if compression, this is compressed record */
    i4         need,
    char            *key,
    DM_TID	    *tid,
    DM_TID          *bid,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    CL_ERR_DESC      sys_err;
    DB_STATUS	    s;
    i4		    dm0l_flag;
    i4		    local_err_code;
    i4		    dm1p_flag = 0;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO	    newdataPinfo;
    DMPP_PAGE	    **leaf, **data, **newdata;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    data = &dataPinfo->page;
    newdata = &newdataPinfo.page;
    *newdata = NULL;

    /*	Allocate a new DISASSOCIATED data page */
    do
    {
	/*
	** Unfix the old associated data page
	*/
	if (*data)
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Call allocation code to get a free page from the free maps.
	** This will possibly trigger a File Extend as well.
	**
	** The newdata's buffer might be pinned by dm1p_getfree and
	** will be unpinned by dm0p_unfix_page.
	*/
	s = dm1p_getfree(r, dm1p_flag, &newdataPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	/* New page has been allocated and formatted as empty DMPP_DATA page */
	/* It is not associated with any leaf */

#ifdef xDEBUG
	if (DMZ_AM_MACRO(7))
	{
	    TRdisplay("findpage: New bulk data page is %d, leaf = %d\n",
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newdata), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));
	    TRdisplay("For table %t in database %t.\n",
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	 }
#endif

	/* Init tid to first line on new data page */
	tid->tid_tid.tid_page = 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *newdata);
	tid->tid_tid.tid_line = 0;

	*dataPinfo = newdataPinfo;
	*newdata = NULL;

	break;
    } while (FALSE); 

    if (s != E_DB_OK)
    {
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9257_DM1B_FINDDATA);
	}

	/* Check if we have left pages fixed before returning */
	if (dataPinfo->page)
	    s = dm0p_unfix_page(r, DM0P_UNFIX, dataPinfo, &local_dberr);
	if (newdataPinfo.page)
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &newdataPinfo, &local_dberr);
	return (E_DB_ERROR);
    }

    return (s);
}

/*{
** Name: check_reclaim - Determines if a data page can be reclaimed.
**
** Description:
**      This routines checks the data page that is paased in to determine
**	if the page is a DISASSOCiated data page and if the pages has no
**	more records.  If this is the case, the page is returned to the
**	free list and any rcb's that reference the page will have the
**	page UNFIXED.
**
**	If data == rcb->rcb_data.page then data must be a copy of
**	the pointer.  It can't be fixed independently.  This code will
**	zero rcb->rcb_data.page when it equals data, and it will
**	unfix data.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      data				Pointer to the data page.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.  
**                                      The error codes are:
**                                      E_DM0019_BAD_FILE_WRITE and
**                                      errors from dm1bpgetfree.
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
**      20-mar-1990 (Derek))
**          Created.
**	16-oct-1992 (jnash)
**	    Reduced logging project.  Substitute dmpp_tuplecount != 0
**	    for !dmpp_isempty().
**	14-dec-1992 (rogerk)
**	    Reduced logging project.  Don't zero out callers rcb_data_page_ptr
**	    field.  Check_reclaim is now called with that pointer.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_tuplecount.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          If row locking, only reclaim page if page lock can be gotten.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          add lock_id parameter to dm0p_trans_page_lock() call.
**      21-may-97 (stial01)
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
static DB_STATUS
check_reclaim(
    DMP_RCB	*rcb,
    DMP_PINFO   *dataPinfo,
    DB_ERROR    *dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMP_RCB	*next_rcb;
    DB_STATUS	status;
    i4     lock_action = 0;
    u_i4	row_low_tran = 0;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE   **data;

    CLRDBERR(dberr);

    data = &dataPinfo->page;

    /* Can't dealloc pages unless mvcc is disabled for db, need to run modify */
    if (MVCC_ENABLED_MACRO(r))
	return (E_DB_OK);

    /*
    ** Lock buffer for update.
    **
    ** This will swap from CR->Root if "data" is a CR page.
    */
    dm0pLockBufForUpd(r, dataPinfo);

    /*  Check to see if this is an empty disassociated data page. */
    if ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, (*data))
	 & DMPP_ASSOC) ||
	((*t->tcb_acc_plv->dmpp_tuplecount)(*data, t->tcb_rel.relpgsize) != 0))
    {

	dm0pUnlockBuf(r, dataPinfo);
	return (E_DB_OK);
    }

    /* Need page lock to reclaim page */
    if (row_locking(r))
    {
        status = dm0p_trans_page_lock(r, t,
			DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, (*data)),
			(i4)DM0P_NOWAIT, LK_X, &lock_action, (LK_LKID *)NULL,
                        dberr);

	dm0pUnlockBuf(r, dataPinfo);

	/* Skip page reclamation if we can't get page lock nowait */
        if (status != E_DB_OK)
            return(E_DB_OK);

	/* Now that we have page lock re-do tuplecount */
	if ((*t->tcb_acc_plv->dmpp_tuplecount)(*data, t->tcb_rel.relpgsize) 
		!= 0)
	    return (E_DB_OK);
    }
    else
	dm0pUnlockBuf(r, dataPinfo);

    /* 
    ** Special data page reclamation for btree blob etabs for which we 
    ** do physical page locking and can fit more than 1 row per page
    ** (Note core catalogs also do physical page locking, but they are HASH)
    ** The isempty accessor will return that the page is not empty if there
    ** are uncommitted deleted records
    */
    if ( NeedPhysLock(r) )
    {
	if (!(*t->tcb_acc_plv->dmpp_isempty)(*data, r))
	    return (E_DB_OK);
    }

    /*
    ** Check that the page is not fixed in any related RCB.  If it is
    ** then unfix it for that caller.
    */
    for (next_rcb = r->rcb_rl_next;
	next_rcb != r;
	next_rcb = next_rcb->rcb_rl_next)
    {
	if (next_rcb->rcb_data.page == *data)
	{
	    status = dm0p_unfix_page(next_rcb, DM0P_UNFIX,
		&next_rcb->rcb_data, dberr);
	    if (status != E_DB_OK)
		return (status);
	}
    }

    /*	Return page to the free list. */
    status = dm1p_putfree(r, dataPinfo, dberr);

    return (status);
}

/*{
** Name: dm1b_find_ovfl_space - Looks for space on a btree leaf overflow chain.
**
** Description:
**	Called when a duplicate key is being added to a leaf which already
**	has an overflow chain - this routine looks for a spot on the chain
**	to which to insert the new key.
**
**	If space is found then the routine returns E_DB_OK and the 'bid'
**	parameter is set to the allocated position.
**
**	If space is not found then E_DB_WARN (E_DM0057_NOROOM) is returned.
**	It is expected in this case that the caller will need to allocate
**	a new overflow page to the chain.
**
**	CURRENTLY THIS ROUTINE ONLY LOOKS AT THE FIRST OVERFLOW PAGE rather
**	than scanning down the entire list.  This is to avoid searches down
**	long overflow chains.  This means that deleted space on overflow
**	chains is generally not reclaimed until a page becomes compeletely
**	empty (at which time it is deallocated and put in the free space).
**
**      If row locking, search the entire chain...
**      If we don't find space we are going to have to split
**      which will require locking/fixing every overflow page anyway 
**      We want to acquire locks on all overflow pages before we try to split
**      to avoid lock waits while holding index page locks,
**      and prevent lock failures in split page mini transaction
**   
** Inputs:
**	rcb			Pointer to record access context.
**	leaf			Pointer to leaf (head of overflow chain).
**	space_required		Number of bytes required.
**
** Outputs:
**	bid			Set to spot at which to insert new key.
**      err_code		Failure status:
**				    E_DM0057_NOROOM - no space on chain.
**                                      
**	Returns:
**	    E_DB_OK		    Space found
**	    E_DB_WARN		    Space not found
**	    E_DB_ERROR		    Routine failed due to error
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-dec-1992 (rogerk)
**	    Created for Reduced logging project.
**	21-jun-1993 (rogerk)
**	    Corrected routine so that it no longer references the overflow
**	    page after it has been unfixed.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Call dm1bxclean() to reclaim space on leaf pages
**      21-apr-97 (stial01)
**          dm1b_find_ovfl_space: Remove call to dm1bxclean, since we don't 
**          have overflow pages for non unique primary btree (V2) anymore.
**      21-may-97 (stial01)
**          dm1b_find_ovfl_spacel() not called for Non unique primary btree (V2)
*/
DB_STATUS
dm1b_find_ovfl_space(
DMP_RCB		*rcb,
DMP_PINFO	*leafPinfo,
char		*leafkey,
i4		space_required,
DM_TID		*bid,
DB_ERROR	*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM_TID	    local_bid;
    bool	    has_room;
    i4	    compression_type;
    i4	    local_error;
    DB_STATUS	    status;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DM_PAGENO	    nextovfl;
    DMP_PINFO	    ovflPinfo;
    DMPP_PAGE	    **leaf, **ovfl;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;
    DMP_PINIT(&ovflPinfo);
    ovfl = &ovflPinfo.page;

    if (!t->tcb_dups_on_ovfl)
    {
	SETDBERR(dberr, 0, E_DM9C16_DM1B_FIND_OVFL_SP);
	return (E_DB_ERROR);
    }

    compression_type = DM1B_INDEX_COMPRESSION(r);
    nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *leaf);
    for (;;)
    {
	/*
	** Check if there are any overflow pages on which to allocate space.
	*/
	if (nextovfl == 0)
	{
	    status = E_DB_WARN;
	    SETDBERR(dberr, 0, E_DM0057_NOROOM);
	    break;
	}

	/*
	** Fix the overflow page in order to check for space there.
	*/
	status = dm0p_fix_page(r, nextovfl, (DM0P_WRITE | DM0P_NOREADAHEAD), 
                &ovflPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "ovfl" is a CR page.
	*/
	dm0pLockBufForUpd(r, &ovflPinfo);

	local_bid.tid_tid.tid_page = 
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *ovfl);
	local_bid.tid_tid.tid_line = 
	    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *ovfl);

	/*
	** Check for space to insert the new record.
	*/
	has_room = dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			*ovfl, compression_type, (i4)100, 
			t->tcb_kperleaf, space_required);

	if (has_room && (row_locking(r) || crow_locking(r)))
	{
	    status = dm1bxreserve(r, &ovflPinfo, &local_bid, leafkey, TRUE, TRUE, 
			dberr);
	    if (status != E_DB_OK)
		dm1cxlog_error(E_DM93C6_ALLOC_RESERVE, r, *ovfl, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (i4)0);
	}

	nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *ovfl);
	dm0pUnlockBuf(r, &ovflPinfo);

	status = dm0p_unfix_page(r, DM0P_UNFIX, &ovflPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	if (has_room)
	{
	    bid->tid_i4 = local_bid.tid_i4;
	    break;
	}
	else if (!row_locking(r) || nextovfl == 0)
	{
	    status = E_DB_WARN;
	    SETDBERR(dberr, 0, E_DM0057_NOROOM);
	    break;
	}

	/* continue to look for space on next overflow */
    }

    /*
    ** If there was no error, then return normally.
    ** Note that if no space was found, then we return E_DB_WARN here.
    */
    if (DB_SUCCESS_MACRO(status))
	return (status);

    /*
    ** Error handling: Unifix overflow page if left fixed.
    */
    if (ovflPinfo.page)
    {
	dm0pUnlockBuf(r, &ovflPinfo);

	status = dm0p_unfix_page(r, DM0P_UNFIX, &ovflPinfo, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C16_DM1B_FIND_OVFL_SP);
    }

    return (E_DB_ERROR);
}

/*{
** Name: newduplicate - Check if given key is a duplicate of a full duplicate
**			leaf page.
**
** Description:
**	Searches leaf page to check if the current current leaf page can be 
**	used to insert the given row.
**
**	This routine is called when an insert of a new key into the btree
**	index requires new space to be allocated.  It determines whether
**	the new insert requires a normal split or an overflow page addition.
**
**	If the new row matches all current key values on the page, then an
**	overflow page is needed.  If the row differs from ANY key value on
**	the page then a normal split is done.
**
**	The routine compares the insert key with the first and last keys on
**	the page rather than comparing with ALL keys.  This is sufficient
**	since the keys are stored in sorted order.  If the page already has
**	an overflow chain then the insert key is compared with only one
**	key (since all on the page must be duplicates).
**
**	This routine will return an error if given an empty leaf page.
**
** Inputs:
**      rcb			Pointer to record access context.
**      leaf			Pointer to the leaf page.
**	key			Insert key
**
** Outputs:
**	duplicate_insert	Indicates whether the insert key matches
**				ALL other keys on the page:
**				    TRUE - key matches all on page
**				    FALSE - key differs from at least 1 on page
**      err_code		Failure status
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-dec-1992 (rogerk)
**	    Created for Reduced logging project.
**	26-jul-1992 (rogerk)
**	    Fixed bug in usage of newduplicate routine.  Leaf argument must
**	    be passed by reference since the leaf page can be temporarily
**	    unfixed by routines that newduplicate calls.  This was causing
**	    dmd_checks when a page was temporarily unfixed and concurrent
**	    access caused the page to be tossed.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added handling of deleted leaf entries (status == E_DB_WARN)
**      21-may-97 (stial01)
**          newduplicate() no longer called for Non unique primary btree (V2)
**      12-jun-97 (stial01)
**          newduplicate() Pass tlv to dm1cxget instead of tcb.
*/
static DB_STATUS
newduplicate(
DMP_RCB		*rcb,
DMP_PINFO	*leafPinfo,
char		*key,
i4		*duplicate_insert,
DB_ERROR	*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    i4	    	    state = 0;
    i4	    	    child;
    i4   	    compare;
    i4	    	    key_len;
    char	    *KeyPtr, *AllocKbuf = NULL;
    char	    *key_ptr;
    char	    key_buffer[DM1B_MAXSTACKKEY];
    DB_STATUS	    status = E_DB_OK;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    if (!t->tcb_dups_on_ovfl)
    {
	SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	return (E_DB_ERROR);
    }

    if ( status = dm1b_AllocKeyBuf(t->tcb_klen, key_buffer,
				&KeyPtr, &AllocKbuf, dberr) )
	return(status);

    for (;;)
    {
	*duplicate_insert = FALSE;

	/*
	** If the leaf has an overflow chain, then we know that all entries
	** on it must be duplicates.  We can take any entry on the leaf or
	** or its chain to check for equality with the insert key.
	**
	** If the leaf has no overflow chain, then we must compare it against
	** the highest and lowest valued keys on the page.
	*/
	if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *leaf))
	{
	    state = E_DB_OK;
	    status = dm1bxdupkey(r, leafPinfo, KeyPtr, &state, dberr);
	    if ((status != E_DB_OK) || (state == E_DM0056_NOPART))
		break;

	    adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys,
		key, KeyPtr, &compare);
	    if (compare != DM1B_SAME)
		break;

	    *duplicate_insert = TRUE;
	}
	else
	{
	    /*
	    ** If there are no entries on the leaf page, then we can't check
	    ** for duplicate keys.  We should never have been called since
	    ** the page is clearly avaliable for insertion.
	    */
	    if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf) == 0)
	    {
		state = E_DM0056_NOPART;
		break;
	    }

	    /*
	    ** Compare the insert key with the first key on the page. If it 
	    ** matches then we must also check the last key.
	    */
	    key_ptr = KeyPtr;
	    key_len = t->tcb_klen;
	    child = 0;
	    status = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			child, &key_ptr, 
			(DM_TID *)NULL, (i4*)NULL,
			&key_len, NULL, NULL, adf_cb);

	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && status == E_DB_WARN)
		status = E_DB_OK;

	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, *leaf, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
		SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
		break;
	    }

	    adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys,
			key, key_ptr, &compare);
	    if (compare != DM1B_SAME)
		break;

	    /*
	    ** Compare the insert key with the last key on the page. If it 
	    ** matches then we must have a duplicate key.
	    */
	    key_ptr = KeyPtr;
	    key_len = t->tcb_klen;
	    child = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf) - 1;
	    status = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			child, &key_ptr, 
			(DM_TID *)NULL, (i4*)NULL,
			&key_len, NULL, NULL, adf_cb);

	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && status == E_DB_WARN)
		status = E_DB_OK;

	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, *leaf, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
		SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
		break;
	    }

	    adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys,
			key, key_ptr, &compare);
	    if (compare != DM1B_SAME)
		break;

	    *duplicate_insert = TRUE;
	}
	break;
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    if ((status == E_DB_OK) && (state == 0))
	return (E_DB_OK);

    /*
    ** If no duplicate key could be found to compare against the insert key
    ** then this routine should never have been called - the requested key
    ** should have been able to have been inserted on this leaf without any
    ** new space allocations.
    */
    if (state == E_DM0056_NOPART)
    {
	uleFormat(NULL, E_DM9C18_DM1B_NEWDUP_NOKEY, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3, 0,
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf),
	    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf),
	    DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *leaf));
	SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1b_badtid_fcn - Flag bad BTREE tid
**
** Description:
**	Called when btree access code finds a bad TID pointer on a leaf
**	page - logs information about the leaf.
**
** Inputs:
**     rcb                             Pointer to record access context
**                                     which contains table control
**                                     block (TCB) pointer, tran_id,
**                                     and lock information associated
**                                     with this request.
**     bid
**     tid				Pointer to the bad one
**     key		
**
** Outputs:
**     None
**
** History:
**      23-oct-1991 (jnash)
**	    Header created
**	23-oct-1991 (jnash)
**          LINT caught err_code initialization fixed.
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMprint macro calls to relaxed bool form
** 
*/
VOID
dm1b_badtid_fcn(
    DMP_RCB		*rcb,
    DM_TID		*bid,
    DM_TID		*tid,
    char		*key,
    PTR			FileName,
    i4			LineNumber)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = r->rcb_tcb_ptr;
    ADF_CB	*adf_cb = r->rcb_adf_cb;
    DB_STATUS	err_code;
    char	local_1buf[13];
    char	local_2buf[25];
    i4		k;
    DB_ERROR	dberr;

/*
** maybe insert is being rolled back
** we saw the leaf entry, but the PUT on data has been rolled back
** DEL
** BTDEL
** BTDEL CLR (insert key back)
**     now we read the key, but row not yet back on data
*/

    dberr.err_file = FileName;
    dberr.err_line = LineNumber;
    dberr.err_code = E_DM9351_BTREE_BAD_TID;
    dberr.err_data = 0;

    /*
    ** Btree is inconsistent.  Give error message indicating bad
    ** index entry and return BAD_TID.  Try to give useful 
    ** information by printing out 1st 12 bytes of Key.  Would be
    ** nice if ule_format would allow us to send a btye array 
    ** argument to be printed out in HEX format.
    */
    MEmove(t->tcb_klen, (PTR)key, ' ', 12, (PTR)local_1buf);
    MEfill(24, ' ', (PTR)local_2buf);
    for (k = 0; (k < 12) && (k < t->tcb_klen); k++)
    {
	if (!CMprint(&local_1buf[k]))
	    local_1buf[k] = '.';
	STprintf(&local_2buf[k*2], "%02x", (i4)key[k]);
    }
    local_1buf[12] = EOS;
    local_2buf[24] = EOS;

    uleFormat(&dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 9,
	sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
	sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
	sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name,
	0, bid->tid_tid.tid_page, 0, bid->tid_tid.tid_line,
	0, tid->tid_tid.tid_page, 0, tid->tid_tid.tid_line,
	12, local_1buf, 24, local_2buf);
}


/*{
** Name: dm1b_find_ovflo_pg  - find previous or last overflow page in chain.
**
** Description:
**	Called when handling a request to read previous or find-last-row
**	of a btree if a duplicate key overflow chain is detected.
**	This function can find the page in the chain that points to the
**	page currently fixed as described by the 'current' parameter.
**	It can also find the last page in chain, for initializing a
**	backwards scan of the chain.
**
** Inputs:
**    	 rcb                           	Pointer to record access context
**                                     	which contains table control
**                                     	block (TCB) pointer, tran_id,
**                                     	and lock information associated
**                                     	with this request.
**   	fix_action			Read/write action bits.          
**   	**current 			Ptr->ptr to page currently fixed.
**                                      this page will be unfixed.
**    	whatpage			Flag describing what page to find:
**					    PREV_OVFL - page before current
**					    LAST_OVFL - last page in chain
**    	startpage			Page number to start scanning from.
**    	*err_code
**
** Outputs:
**    	**current			Modified to point to page fixed
**					upon exit.
**
** History:
**      18-may-1995 (cohmi01)
**	    Created - for MANMANX read backwards.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**      21-may-97 (stial01)
**          dm1b_find_ovfl_pg() not called for Non unique primary btree (V2)
*/
static  DB_STATUS
dm1b_find_ovflo_pg(
    DMP_RCB     *r,
    i4	fix_action, 
    DMP_PINFO	*currentPinfo, 
    i4	whatpage,
    DM_PAGENO	startpage, 
    DB_ERROR	*dberr)
{
    DM_PAGENO	nextpg, targetpg;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DB_STATUS	s;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	**current;

    CLRDBERR(dberr);

    current = &currentPinfo->page;

    if (!t->tcb_dups_on_ovfl)
    {
	SETDBERR(dberr, 0, E_DM9C16_DM1B_FIND_OVFL_SP);
	return (E_DB_ERROR);
    }

    /* next page to fix, may be leaf or ovflo pointed to by leaf */
    nextpg = startpage;         

    /* number of page whose parent we are looking for. 		  	 */
    /* For PREV_OVFL its our pageno, for LAST_OVFL its 0 for last page	 */
    targetpg = (whatpage == PREV_OVFL) ? 
	DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype, 
	(*current)) : 0;


    do
    {
	dm0pUnlockBuf(r, currentPinfo);
	/* done with initial or current leafpage, unfix it now */
        s = dm0p_unfix_page(r, DM0P_UNFIX, currentPinfo, dberr);
	if ( s )
	    break;

	/* get the first/next page in dup chain */
        s = dm0p_fix_page(r, nextpg, fix_action, currentPinfo, dberr);
	if ( s )
	    break;

	/*
	** Lock buffer for get.
	**
	** This may produce a CR page and overlay "current"
	** with its pointer.
	*/
	dm0pLockBufForGet(r, currentPinfo, &s, dberr);
	if ( s == E_DB_ERROR )
	    break;

	nextpg = DM1B_VPT_GET_PAGE_OVFL_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
	    *current);
    } while (nextpg != targetpg);

    return s;
}

/*{
** Name: dm1b_count - count records in a btree table.
**
** Description:
**
** This function counts how many records there are in a btree
**      table, for use by count(*) agg function, as part of DMR_COUNT
**      request processing.
** Data pages are not read, the bt_kids field on the leaf pages is
**	used to count op how many records the leafs point to.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      direction                       Ignored, just for compatibility
**                                      with dm2r_get parms since dm1r_count
**                                      may be called interchangably via ptr.
**      record                          Ignored, just for compatibility
**                                      with dm2r_get parms since dm1r_count
**                                      may be called interchangably via ptr.
**
** Outputs:
**      countptr                        Ptr to where to put total count of
**                                      records in the table.
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**                                      Error codes returned are:
**                                      E_DM008A_ERROR_GETTING_RECORD
**                                      E_DM93A2_DM1R_GET
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-may-1995 (cohmi01)
**          Created for agg optimization project.
**	05-sep-1995 (cohmi01)
**	    Add dummy parms for compatibilty with dm2r_get parms so this
**	    function may be called interchangably with it via function ptr.
**      06-may-1996 (thaju02)
**          New Page Format Project. Modify page header references to macros.
**	 1-jul-96 (nick)
**	    If we broke out of the count, we'd leave the RCB pointing
**	    to a page we'd already unfixed ; subsequent attempts to
**	    unfix this page higher up had interesting results. #76111
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0P_LOCKREAD flags when fixing the page.
**      27-feb-97 (stial01)
**          dm1b_count() Call dm1cxkeycount() if leaf needs cleaning.
**          Fix leaf page if it is not fixed on entry.
**      14-July-98 (islsa01)
**          bug #90993 - lockmode readlock=nolock is not honored when executing
**          a select count(*) on a btree table.
**          Instead of passing DM0P_LOCKREAD, DM0P_READ should be passed to
**          dm0p_fix_page() to avoid locking all the leaf pages in where
**          row locking is not true until commit is done.
**	11-Nov-2005 (jenjo02)
**	    Removed bogus dm0p_check_logfull().
**	12-Feb-2007 (kschendel)
**	    Add occasional stupid CScancelCheck for cancel checking.
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_?, dm1?_count functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Use dm0pLockBufForGet in addition to DM0P_LOCKREAD
*/

DB_STATUS
dm1b_count(
    DMP_RCB         *rcb,
    i4	    *countptr,  
    i4         direction,
    void            *record,
    DB_ERROR        *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DB_STATUS	    s = E_DB_OK;	/* Variable used for return status. */
    DM_PAGENO	    nextleaf, nextovfl;
    i4	    count = 0;
    i4         local_err_code; 
    i4         fix_action;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If row locking is true, set fix_action to DM0P_LOCKREAD,
    ** otherwise set to DM0P_READ.
    */
    if (row_locking(r))
       fix_action = DM0P_LOCKREAD;
    else
       fix_action = DM0P_READ;

    /* 
    ** A previous position call already fixed the first leaf page.
    ** However upon leaving dmf, it may have been unfixed.
    */
    if (r->rcb_other.page == NULL)
    {
	s = dm0p_fix_page(r, (DM_PAGENO)r->rcb_lowtid.tid_tid.tid_page, 
	    fix_action, &r->rcb_other, dberr);
    }

    /*
    ** Loop through leaf pages and ovflo pages to add up bt_kids.
    */
    for ( ; (s == E_DB_OK) && r->rcb_other.page; )
    {
	/* Check for external interrupts */
	if ( *(r->rcb_uiptr) )
	{
	    /* check to see if user interrupt occurred. */
	    if ( *(r->rcb_uiptr) & RCB_USER_INTR )
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    /* check to see if force abort occurred */
	    else if ( *(r->rcb_uiptr) & RCB_FORCE_ABORT )
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);

	    if ( dberr->err_code )
	    {
		s = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Lock buffer for get.
	**
	** This may produce a CR page and overlay rcb_other.page
	** with its pointer.
	**
	** dm0p_unfix_page will release any pinlock acquired.
	*/
	dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
	if ( s != E_DB_OK )
	    break;

	/* keep count of tuples each leaf points to */
	if (t->tcb_rel.relpgtype == TCB_PG_V1)
	{
	    count += DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
			r->rcb_other.page);
	}
	else
	{
	    count += dm1cxkeycount(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			 r->rcb_other.page);
	}

	/* Make note of next leaf number and ovflo chain, if any. Release pg */
	nextleaf = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
			r->rcb_other.page);
	nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
			r->rcb_other.page);
	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	if ( s )
	    break;

	CScancelCheck(r->rcb_sid);

	/* 
	** If previous leaf is head of ovflo chain, pass thru and count
	** tups in this ovflo chain, then resume with next leaf page.
	** Don't request readahead, as we are not reading any data pages.
	*/
	while (nextovfl)       
	{
	    s = dm0p_fix_page(r, (DM_PAGENO)nextovfl, 
			    fix_action | DM0P_NOREADAHEAD,
			    &r->rcb_other, dberr);
	    if ( s )
		break;

	    dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
	    if ( s != E_DB_OK )
		break;

	    /* Add # of dups on page to count */
	    if (t->tcb_rel.relpgtype == TCB_PG_V1)
	    {
		count += DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
				r->rcb_other.page); 
	    }
	    else
	    {
		count += dm1cxkeycount(t->tcb_rel.relpgtype,
				t->tcb_rel.relpgsize, r->rcb_other.page);
	    }

	    /* note next page, if any */
	    nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
				r->rcb_other.page);

	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	    if ( s )
		break;
	}
	if (s != E_DB_OK)
	    break;

	/* now read in next leaf page if any */
	if (nextleaf == 0)
	    break;	/* end of scan, all entries counted */

	s = dm0p_fix_page(r, nextleaf, fix_action, &r->rcb_other, dberr);
	if (s != E_DB_OK)
	    break;
    }

    if (s == E_DB_OK)
    {
	*countptr = count;
	return (E_DB_OK);
    }

    /*	Handle error reporting and recovery. */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    }

    return (s);
}


/*{
** Name: btree_reposition - Determines if reposition to key is needed 
**
** Description:
**      This routines checks the page lsn and page clean count to determine
**      if the specified position (bid) is still valid
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf
**
**      bid                             Pointer to bid.
**      pop                             position operation
**      opflag                          opflag if operation is DM1C_MGET
**
** Outputs:
**      bid                             Pointed to to an area to return new
**                                      bid if position has been changed.
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
**      
**      10-jun-98 (stial01)
**          We NEVER need phantom protection repositioning to a key. This can
**          result in a deadlock requesting a SIX lock for a page that we
**          have only an IX lock on. (If our leaf entry moved because of 
**          a split, an IX page lock would have been propagated for us).
**      26-aug-98 (stial01)
**          btree_reposition() Set error status where compare != DM1B_SAME
**	26-Feb-2010 (jonj)
**	    CRIB now anchored in RCB.
**	29-Jun-2010 (jonj)
**	    Add more debug stuff to reposition error reporting.
*/
static DB_STATUS
btree_reposition(
    DMP_RCB     *rcb,
    DMP_PINFO   *leafPinfo,
    DM_TID      *bid,
    char	*key_ptr,
    i4		pop,	
    i4		opflag,
    DB_ERROR    *dberr)
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO        *tbio = &t->tcb_table_io;
    DM_PAGENO           pageno;
    DM_LINE_IDX         pos;
    DB_STATUS           s;
    i4                  compare;
    i4             fix_action;
    bool                correct_leaf = FALSE;
    i4             mode;
    bool                key_found = FALSE;
    bool		split_done = FALSE;
    i4			tidpartno;
    i4		    *err_code = &dberr->err_code;
    i4			local_error;
    DMP_POS_INFO   *savepos;
    DMP_POS_INFO   origpos;
    DMPP_PAGE		**leaf;
    i4			i;
    DM_TID		tmp_tid;
    i4			tmp_part;
    bool		is_deleted;
    /* ***temp */
    i4 line1,line2,line3,line4=0;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    if (t->tcb_rel.relpgtype == TCB_PG_V1) 
	return (E_DB_OK);

    /* point to position info for this operation */
    savepos = &r->rcb_pos_info[pop];
    origpos = *savepos;

    /* 
    ** Consistency check:
    ** Check for valid pop
    ** Input bid should always match saved bid
    */
    if (pop < RCB_P_MIN || pop > RCB_P_MAX ||
       (bid->tid_i4 != savepos->bid.tid_i4 && DM1B_POSITION_VALID_MACRO(r, pop)))
    {
	TRdisplay("btree_reposition consistency check %d (%d,%d),(%d,%d)\n",
	    pop, bid->tid_tid.tid_page, bid->tid_tid.tid_line,
	    savepos->bid.tid_tid.tid_page, savepos->bid.tid_tid.tid_line);
	SETDBERR(dberr, 0, E_DM93F6_BAD_REPOSITION);
	return (E_DB_ERROR);
    }

    /*
    ** If we did a NEXT/PREV to another leaf and have not yet
    ** returned a row... if repositioning we can start at the top/bot again
    ** If we are serializable, keys cannot be inserted to a leaf after we
    ** move to the next/prev leaf.
    ** If we are not serializable, keys phantoms can get inserted after we
    ** move to the next leaf
    */
    if (pop == RCB_P_GET && DM1B_POSITION_VALID_MACRO(r, pop)
	&& savepos->bid.tid_tid.tid_line == DM_TIDEOF)
    {
	TRdisplay("REPOSITION to TIDEOF (%d %d) leaf %d, startleaf %d\n",
	    t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
	    bid->tid_tid.tid_page, rcb->rcb_p_lowtid.tid_tid.tid_page);

	bid->tid_tid.tid_line = DM_TIDEOF;
	return (E_DB_OK);
    }

    if (DM1B_POSITION_VALID_MACRO(r, pop) == FALSE)
    {
	if (pop == RCB_P_GET && 
		 (opflag & DM1C_GETNEXT) || (opflag & DM1C_GETPREV))
	{
	    s = btree_position(r, dberr);
	    if (s == E_DB_OK)
		bid->tid_i4 = r->rcb_lowtid.tid_i4;
	    return (s);
	}
	else
	{
	    uleFormat(dberr, E_DM93F5_ROW_LOCK_PROTOCOL, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, err_code, 0);
	    return (E_DB_ERROR);
	}
    }

    /* FIX ME if opflag == DM1C_PREV, check nextpage */
    if ( !dm0pBufIsLocked(leafPinfo) )
    {
	if (pop == RCB_P_GET)
	{
	    /*
	    ** Lock buffer for get.
	    **
	    ** This may produce a CR page and overlay "leaf"
	    ** with its pointer.
	    */
	    dm0pLockBufForGet(r, leafPinfo, &s, dberr);
	    if ( s == E_DB_ERROR )
		return (E_DB_ERROR);
	}
	else
	{
	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if "leaf" is a CR page.
	    */
	    dm0pLockBufForUpd(r, leafPinfo);
	}
    }

    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf) != 
	bid->tid_tid.tid_page)
    {
	TRdisplay("WARNING reposition with bid page %d leaf page %d\n",
	    bid->tid_tid.tid_page,
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));
	SETDBERR(dberr, 0, E_DM93F6_BAD_REPOSITION);
	return (E_DB_ERROR);
    }

    /* See if leaf page has changed */
    if (DM1B_POSITION_PAGE_COMPARE_MACRO(t->tcb_rel.relpgtype, *leaf,
		savepos) == DM1B_SAME)
    {
	return (E_DB_OK);
    }

    /* Reposition to leaf/key specified */
    pageno = (DM_PAGENO)bid->tid_tid.tid_page;

    /*
    ** NOTE: Phantom protection is NEVER needed during reposition
    **       It can cause a deadlock and the reposition will fail
    */
    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;

    /* 
    **  DM1C_FIND/DM1C_EXACT    Locate the position on this page where
    **                          this entry is located, or the position where
    **                          it would be inserted if it's not found.
    **  DM1C_TID/DM1C_EXACT     Locate a specific entry on the page.
    **
    **  Non-unique primary btree: the only way to reposition correctly
    **  is to reposition to the exact record.
    **  If DM1C_BYTID specified, we also must position to the exact record
    **  specified.
    */
    if (pop != RCB_P_GET)
        mode = DM1C_TID;
    else if ((t->tcb_rel.relstat & TCB_UNIQUE) ||
                (t->tcb_rel.relstat & TCB_INDEX))
	mode = DM1C_FIND;
    else
	mode = DM1C_TID;

    dmf_svcb->svcb_stat.btree_repos++;
    if (!row_locking(r) && !crow_locking(r))
	dmf_svcb->svcb_stat.btree_pglock++;

    for (;;)
    {
	if (*leaf == NULL)
	{
	    /* ***temp */
	    line1 = __LINE__;

	    s = dm0p_fix_page(r, pageno, fix_action, leafPinfo, dberr);
	    if (s != E_DB_OK)
		break;
	    if (pop == RCB_P_GET)
	    {
		/*
		** Lock buffer for get.
		**
		** This may produce a CR page and overlay "leaf"
		** with its pointer.
		*/
		dm0pLockBufForGet(r, leafPinfo, &s, dberr);
		if ( s == E_DB_ERROR )
		    break;
	    }
	    else
	    {
		/*
		** Lock buffer for update.
		**
		** This will swap from CR->Root if "leaf" is a CR page.
		*/
		dm0pLockBufForUpd(r, leafPinfo);
	    }
	}

	if (r->rcb_access_mode == RCB_A_WRITE && !r->rcb_logging)
	    dmf_svcb->svcb_stat.btree_nolog++;

	/*
	** If leaf hasn't split, this must be correct leaf
	**
	** tcb_dups_on_ovfl and this is an overflow page
	** this must be the correct leaf ... 
	** keys on an overflow page can't rshift to another leaf
	*/
	if (savepos->nextleaf == 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf) ||
	(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *leaf) & DMPP_CHAIN))
	{
	    /* ***temp */
	    line2 = __LINE__;

	    correct_leaf = TRUE;
	}
	else
	{
	    dmf_svcb->svcb_stat.btree_repos_chk_range++;
	    split_done = TRUE;
	    s = dm1b_compare_key(r, *leaf, DM1B_LRANGE, key_ptr,  t->tcb_keys, 
		    &compare, dberr);
	    if (s != E_DB_OK)
		break;
	    if (compare < DM1B_SAME)
	    {
		s = E_DB_ERROR;
		break;
	    }

	    s = dm1b_compare_range( r, *leaf, key_ptr, &correct_leaf, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** If the key still falls within this key range, then we
	** potentially have a valid page.
	*/
	if (correct_leaf == TRUE)
	{
	    dmf_svcb->svcb_stat.btree_repos_search++;

	    if ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, *leaf) & 
		DMPP_CHAIN) == 0)
	    {
		/*
		** Search for the key on the this leaf page.
		*/
		s = dm1bxsearch(r, *leaf, mode, DM1C_EXACT,
		    key_ptr, t->tcb_keys, &savepos->tidp, &tidpartno,
		    &pos, NULL, dberr);
	    }
	    else
	    {
		/*
		** ALL keys on DMPP_CHAIN (overflow) are DUPS 
		** Look at each entry to make sure we position correctly:
		** We may have multiple entries with the same tid: 
		** 'dupkey' tid (5,5) DELETED
		** 'dupkey' tid (5,5) DELETED
		** 'dupkey' tid (5,5)
		** If repostition for RCB_P_FETCH, we must position to
		** the entry that is not already deleted!
		** If reposition for RCB_P_ALLOC, we probably should 
		** position to the entry reserved by this transaction
		*/
		s = E_DB_ERROR;
		for (pos = 0; 
			pos < DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
			    *leaf); pos++)
		{
		    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			*leaf, pos, &tmp_tid, &tmp_part);

		    if (tmp_tid.tid_i4 != savepos->tidp.tid_i4)
			continue;

		    /* RCB_P_FETCH ignore deleted entries! */
		    if (pop == RCB_P_FETCH &&
			dmpp_vpt_test_free_macro(t->tcb_rel.relpgtype, 
			DM1B_VPT_BT_SEQUENCE_MACRO(t->tcb_rel.relpgtype, *leaf),
			    (i4)pos + DM1B_OFFSEQ))
			continue;

		   /* The entry we are looking for */
		   s = E_DB_OK;
		   break;
		}
		if (s == E_DB_ERROR)
		    SETDBERR(dberr, 0, E_DM0056_NOPART);
	    }

	    if (s != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0056_NOPART)
		{
		    if ((t->tcb_rel.relstat & TCB_UNIQUE) ||
			(t->tcb_rel.relstat & TCB_INDEX) ||
			t->tcb_dups_on_ovfl)
			break;

		    /*
		    ** Non unique primary btree (V2), dups can span leaf pages
		    ** If our key matches RRANGE, we should check the next leaf
		    */
		    s = dm1b_compare_key(r, *leaf, DM1B_RRANGE, key_ptr,
				t->tcb_keys, &compare, dberr);
		    if (s != E_DB_OK)
			break;

		    if (compare != DM1B_SAME)
		    {
			s = E_DB_ERROR;
			break;
		    }

		    pageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
				*leaf);
		}
		else
		    /* A serious error */
		    break;
	    }
	    else
	    {
		/* Found the specified key */

		bid->tid_tid.tid_page =
		      DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
		      *leaf);
		bid->tid_tid.tid_line = pos;
		key_found = TRUE;
		break;
	    }
	}
	else
	{
	    /* Key was not found, go to the next page */
	    pageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
	   	  *leaf);
	}

	if (pageno == 0)
	{
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    s = E_DB_ERROR;
	    break;
	}

	dm0pUnlockBuf(r, leafPinfo);
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;
	/* ***temp */
	line3 = __LINE__;
	line4++;

	continue;
    }

    if (s != E_DB_OK || !key_found) 
    {
	/* Shut off MVCC tracing to make all of this stuff readable */
	t->tcb_dcb_ptr->dcb_status &= ~DCB_S_MVCC_TRACE;

	/* Show how we got here */
	if ( s )
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		       (char *)NULL, (i4)0, (i4 *)NULL, err_code,
		       0);

	TRdisplay("DM1B-REPOS Session ID 0x%p op %d page 0x%p (%d,%d) %~t\n"
	    "    pop %d tran %x lgid %d Bid:(%d,%d) status %d \n"
	    "    POS Bid:(%d,%d) Tid:(%d,%d) LSN=(%x,%x) Line %d\n"
	    "    POS cc %d nextleaf %d page_stat %x %v\n"
	    "    POS page 0x%p page tran %x\n"
	    "    POS row tran %x row lgid %d\n",
	    r->rcb_sid, r->rcb_dmr_opcode, leafPinfo->page,
	    t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
	    t->tcb_relid_len, t->tcb_rel.relid.db_tab_name,
	    pop, r->rcb_tran_id.db_low_tran, (i4)r->rcb_slog_id_id,
	    bid->tid_tid.tid_page, bid->tid_tid.tid_line, s,
	    savepos->bid.tid_tid.tid_page, savepos->bid.tid_tid.tid_line,
	    savepos->tidp.tid_tid.tid_page, savepos->tidp.tid_tid.tid_line,
	    savepos->lsn.lsn_high, savepos->lsn.lsn_low, savepos->line,
	    savepos->clean_count, savepos->nextleaf,
	    savepos->page_stat, PAGE_STAT, savepos->page_stat,
	    savepos->page, savepos->tran.db_low_tran,
	    savepos->row_low_tran, (i4)(savepos->row_lg_id));

	TRdisplay("DM1B-REPOS ");
        dmd_prkey(r, key_ptr, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);

	if ( leafPinfo->page )
	{
	    /* dmd_prindex prints page header, range entries, kids */
	    dmd_prindex(r, *leaf, (i4)0);
	    /* If CR leaf, do likewise */
	    if ( leafPinfo->CRpage )
		dmd_prindex(r, leafPinfo->CRpage, (i4)0);
	    dm0pUnlockBuf(r, leafPinfo);
	    dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	}

	if (crow_locking(r))
	{
	    LG_CRIB	*crib = r->rcb_crib_ptr;

	    TRdisplay("DM1B-REPOS CRIB: tran %x bos_tran %x lsn %x commit %x\n"
		"\t       bos_lsn %x seq %d lg_low %d lg_high %d\n",
		r->rcb_tran_id.db_low_tran,
		crib->crib_bos_tranid,
		crib->crib_low_lsn.lsn_low,
		crib->crib_last_commit.lsn_low,
		crib->crib_bos_lsn.lsn_low,
		crib->crib_sequence,
		crib->crib_lgid_low, crib->crib_lgid_high);

		dmd_pr_mvcc_info(r);
	}

	/* query will always be printed for E_UL0017 */
	uleFormat(NULL, E_UL0017_DIAGNOSTIC, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1, 10, "DM1B_REPOS");

	SETDBERR(dberr, 0, E_DM93F6_BAD_REPOSITION);
	return (E_DB_ERROR);
    }

    DM1B_POSITION_SAVE_MACRO(r, *leaf, bid, pop);

    /* should caller adjust rcb_currenttid ???? */
    if (pop == RCB_P_GET &&
	 (opflag & DM1C_GETNEXT) || (opflag & DM1C_GETPREV))
	r->rcb_currenttid.tid_i4 = savepos->tidp.tid_i4;

    return (E_DB_OK);
}



/*{
** Name: dm1b_position - Determines if position using key is necessary 
**
** Description:
**      This routines checks the page lsn and page clean count to determine
**      if our STARTING position is still valid
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**
** Outputs:
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
**      
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      21-may-97 (stial01)
**          btree_position() Created from dm1b_position, now dm1b_search is
**          external interface for btree search.
**      16-jun-97 (stial01)
**          btree_position() clarify search conditions possible by 
**          changing 'if' to 'else'
**      29-sep-97 (stial01)
**          btree_position() Added case RCB_P_QUAL_NEXT used by simple
**          aggregate MAX processing. (B86295)
**          btree_position() UNDO 16-jun-97 change:
**          Error running achilles ebugs with row locking:
**          dm2r_get->dm1b_get->next->E_DM002A, beteeen position and 1st get,
**          the page changed, -> dm1b_get-> btree_reposition->btree_position
**          If ll_given is 0 and r->rcb_p_hk_type == RCB_P_EQ
**          we must position using exact match or partial key, so
**          make sure to test for this case (if, not else if)
**      15-jul-98 (stial01)
**          dm1b_position() Init clean count for current position
**      26-jul-1998 (nanpr01)
**	    Implementing backward range scan on Btree.
**	13-aug-1998 (nanpr01)
**	    Fixup Btree reposition code for backward scan.
**      12-aug-98 (stial01)
**          btree_position() Always reposition if trace point DM719
**      05-may-1999 (nanpr01)
**          btree_position() If serializable with equal predicate, do not
**          specify (phantom protection) when fixing leaf
**      20-oct-99 (stial01)
**          btree_position() If starting leaf unchanged, restore p_lowtid
**      04-nov-99 (nanpr01)
**          There is no need to rest the readahead flag. Infact if we
**	    reset the flag here, the overriding noread-ahead decision will
**	    be lost.
*/          

DB_STATUS
dm1b_position(
DMP_RCB        *rcb,
DB_ERROR       *dberr)
{
    DMP_RCB             *r = rcb;
    DB_STATUS           status;

    DM1B_POSITION_INIT_MACRO(r, RCB_P_GET);
    DM1B_POSITION_INIT_MACRO(r, RCB_P_FETCH);

    status = btree_position(r, dberr);
    dm0pUnlockBuf(r, &r->rcb_other);

    return (status);
}
static DB_STATUS
btree_position(
DMP_RCB        *rcb,
DB_ERROR       *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO        *tbio = &t->tcb_table_io;
    DB_STATUS   	status;
    i4             fix_action;     /* Type of access to page. */
    bool                dm719 = FALSE;
    DM_TID              dm719_lowtid;
    i4		    *err_code = &dberr->err_code;
    DMP_POS_INFO	*savepos;

    CLRDBERR(dberr);

    if (t->tcb_rel.relpgtype == TCB_PG_V1) 
	return (E_DB_OK);

    if (r->rcb_p_flag == RCB_P_BYTID)
	return (E_DB_ERROR);

    /* point to position info for this operation */
    savepos = &r->rcb_pos_info[RCB_P_START];

    /* Make sure page for STARTING position is fixed (rcb_p_lowtid) */
    if (r->rcb_other.page && r->rcb_p_lowtid.tid_tid.tid_page != 
	DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page))
    {
	dm0pUnlockBuf(r, &r->rcb_other);
	status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }
    if (r->rcb_other.page == NULL)
    {
	fix_action = DM0P_WRITE;
	if (r->rcb_access_mode == RCB_A_READ)
	    fix_action = DM0P_READ;
        if (row_locking(r) && serializable(r))
	    if ((r->rcb_p_hk_type != RCB_P_EQ) ||
		 (r->rcb_hl_given != t->tcb_keys))
                fix_action |= DM0P_PHANPRO; /* phantom protection is needed */
 
	status = dm0p_fix_page(r, (DM_PAGENO)r->rcb_p_lowtid.tid_tid.tid_page, 
	    fix_action, &r->rcb_other, dberr);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    if ( !dm0pBufIsLocked(&r->rcb_other) )
    {
	/*
	** Lock buffer for get.
	**
	** This may produce a CR page and overlay "rcb_other.page"
	** with its pointer.
	*/
	dm0pLockBufForGet(r, &r->rcb_other, &status, dberr);
	if ( status == E_DB_ERROR )
	    return (E_DB_ERROR);
    }

    /* check to see if page has been changed */
    if (DM1B_POSITION_PAGE_COMPARE_MACRO(t->tcb_rel.relpgtype,
		r->rcb_other.page, savepos) == DM1B_SAME)
    {
	if (DMZ_TBL_MACRO(19))
	{
	    /* Always reposition if trace point DM719 */
	    dm719_lowtid.tid_i4 = r->rcb_p_lowtid.tid_i4;
	    dm719 = TRUE;
	}
	else
	{
	    /* btree_position, restore rcb_lowtid = rcb_p_lowtid */
	    r->rcb_lowtid.tid_i4 = r->rcb_p_lowtid.tid_i4;
	    return (E_DB_OK);
	}
    }

    /*
    ** Unfix any leaf pages left around
    */
    dm0pUnlockBuf(r, &r->rcb_other);
    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    if (r->rcb_p_flag == RCB_P_LAST)
    {
	/* Position to bottom */
	status = btree_search(r, (char *)NULL, (i4)0,
		DM1C_LAST, DM1C_NEXT, &r->rcb_other,
		&r->rcb_lowtid, &r->rcb_currenttid, NULL, dberr);
    }
    else if (r->rcb_p_flag == RCB_P_ALL)
    {
	/* Position for full scan */
	dmf_svcb->svcb_stat.rec_p_all++;

	status = btree_search(r, (char *)NULL, (i4)0, DM1C_EXTREME, 
	    DM1C_PREV, &r->rcb_other,
	    &r->rcb_lowtid, &r->rcb_currenttid, NULL, dberr);
	r->rcb_lowtid.tid_tid.tid_line--;
    }
    else if (r->rcb_p_flag == RCB_P_QUAL_NEXT)
    {
	/*
	** Btree will be read backwards starting at the end of 
	** the pages that satisfy the hi key qualification.
	*/
	status = btree_search(r, r->rcb_hl_ptr, r->rcb_hl_given,
	    DM1C_RANGE, DM1C_NEXT, &r->rcb_other,
	    &r->rcb_lowtid, &r->rcb_currenttid, NULL, dberr);
    }
    else
    {
	/*
	** At this point we have the following cases:
	**
	** No low key specified (RCB_P_QUAL), set low to beginning of file
	** ELSE No high key specified (RCB_P_ENDQUAL), set low to end of file
	** ELSE >= low key specified, (RCP_P_QUAL), set low to lowkey 
	** ELSE <= high key specified, (RCB_P_ENDQUAL), set low to high key
	** ELSE = high key specified (RCB_P_QUAL), set low to high key
	*/
	if (r->rcb_ll_given == 0 && (r->rcb_p_hk_type != RCB_P_EQ))
	{
    	    if (r->rcb_p_flag == RCB_P_QUAL)
	    {
	        /* Set low to beginning of file */
	        status = btree_search(r, (char *)NULL, (i4)0, 
		    DM1C_EXTREME, DM1C_PREV, &r->rcb_other,
		    &r->rcb_lowtid, &r->rcb_currenttid, NULL, dberr);
	        r->rcb_lowtid.tid_tid.tid_line--;
	        if (status != E_DB_OK)
		    return (status);
	        dmf_svcb->svcb_stat.rec_p_range++;
	    }
	}

	if (r->rcb_hl_given == 0 && (r->rcb_p_lk_type != RCB_P_EQ))
	{
    	    if (r->rcb_p_flag == RCB_P_ENDQUAL)
	    {
	        /* Set low to end of file */
	        status = btree_search(r, (char *)NULL, (i4)0, 
		    DM1C_LAST, DM1C_NEXT, &r->rcb_other,
		    &r->rcb_lowtid, &r->rcb_currenttid, NULL, dberr);

	        if (status != E_DB_OK)
		    return (status);
	        dmf_svcb->svcb_stat.rec_p_range++;
	    }
	}

	if (r->rcb_p_lk_type == RCB_P_GTE)
	{
    	    if (r->rcb_p_flag == RCB_P_QUAL)
	    {
	        /* Set lower bound. */
	        status = btree_search(r, r->rcb_ll_ptr, r->rcb_ll_given, 
		    DM1C_RANGE, DM1C_PREV, &r->rcb_other, 
		    &r->rcb_lowtid, &r->rcb_currenttid,  NULL, dberr);
	        r->rcb_lowtid.tid_tid.tid_line--;
	        if (status != E_DB_OK)
		    return (status);
	        dmf_svcb->svcb_stat.rec_p_range++;
	    }
	}

	if (r->rcb_p_hk_type == RCB_P_LTE)
	{
    	    if (r->rcb_p_flag == RCB_P_ENDQUAL)
	    {
	        /* Set lower bound. */
	        status = btree_search(r, r->rcb_hl_ptr, r->rcb_hl_given, 
		    DM1C_RANGE, DM1C_NEXT, &r->rcb_other, 
		    &r->rcb_lowtid, &r->rcb_currenttid, NULL, dberr);
	        if (status != E_DB_OK)
		    return (status);
	        dmf_svcb->svcb_stat.rec_p_range++;
	    }
	}

	if (r->rcb_p_hk_type == RCB_P_EQ)
	{
	    if (r->rcb_hl_given != t->tcb_keys)
	    {
		/* Exact match on partial key. */
		status = btree_search(r, r->rcb_hl_ptr, r->rcb_hl_given, 
		    DM1C_RANGE, DM1C_PREV,
		    &r->rcb_other, &r->rcb_lowtid, 
		    &r->rcb_currenttid, NULL, dberr);
		if (status != E_DB_OK)
		    return (status);
		r->rcb_lowtid.tid_tid.tid_line--;
	    }
	    else
	    {
		/*	Exact match on full key. */

		/* Btree position for exact match use readahead too. */
		status = btree_search(r, r->rcb_hl_ptr, r->rcb_hl_given, 
		    DM1C_FIND, DM1C_EXACT,
		    &r->rcb_other, &r->rcb_lowtid, 
		    &r->rcb_currenttid, NULL, dberr);
		if (status != E_DB_OK && dberr->err_code == E_DM0056_NOPART)
		{
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    return (status);
		}
		r->rcb_lowtid.tid_tid.tid_line--;

		if (status != E_DB_OK)
		    return (status);
		dmf_svcb->svcb_stat.rec_p_exact++;
	    }
	}
    }


    if (status == E_DB_OK)
    {
	/* Save position. */
	r->rcb_p_lowtid.tid_i4 = r->rcb_lowtid.tid_i4;
	r->rcb_p_hightid.tid_i4 = r->rcb_hightid.tid_i4;	
	r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);
	DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_p_lowtid, RCB_P_START);
    }

    if (dm719 && (dm719_lowtid.tid_i4 != r->rcb_lowtid.tid_i4))
    {
	TRdisplay("DM719 Position error (%d %d) (%d %d)\n",
	    dm719_lowtid.tid_tid.tid_page, dm719_lowtid.tid_tid.tid_line,
	    r->rcb_lowtid.tid_tid.tid_page, r->rcb_lowtid.tid_tid.tid_line);
	SETDBERR(dberr, 0, E_DM93F5_ROW_LOCK_PROTOCOL);
	return (E_DB_ERROR);
    }

    return (status);
}


DB_STATUS
dm1b_compare_range(
DMP_RCB	*rcb,
DMPP_PAGE  *leafpage,
char        *key,
bool        *correct_leaf,
DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DB_STATUS       s;
    i4   	    rcompare,lcompare;
    
    /*
    ** Compare the record's key with the leaf's high and low range keys.
    ** If it falls in between then we already have the correct leaf fixed.
    */
    s = dm1b_compare_key(r, leafpage, DM1B_RRANGE, key, t->tcb_keys,
		&rcompare, dberr);

    if (s == E_DB_OK)
    {
	s = dm1b_compare_key(r, leafpage, DM1B_LRANGE, key, t->tcb_keys,
		    &lcompare, dberr);

	if (s == E_DB_OK)
	{
	    if ((t->tcb_rel.relstat & TCB_UNIQUE) ||
		(t->tcb_rel.relstat & TCB_INDEX)  ||
		t->tcb_dups_on_ovfl)
	    {
		if ((rcompare > DM1B_SAME) || (lcompare <= DM1B_SAME))
		    *correct_leaf = FALSE;
		else
		    *correct_leaf = TRUE;
	    }
	    else
	    {
		/*
		** Non unique primary btree (V2), dups span leaf pages
		*/
		if ((rcompare > DM1B_SAME) || (lcompare < DM1B_SAME))
		    *correct_leaf = FALSE;
		else
		    *correct_leaf = TRUE;
	    }
	}
    }

    return (s);
}

DB_STATUS
dm1b_compare_key(
DMP_RCB     *rcb,
DMPP_PAGE  *leaf,
i4      lineno,
char        *key,
i4      keys_given,
i4          *compare,
DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    ADF_CB          *adf_cb = r->rcb_adf_cb;
    DB_STATUS       s;
    char            *tmpkey, *AllocKbuf;
    char            keybuf[DM1B_MAXSTACKKEY];
    i4         	    tmpkey_len;
    i4         	    infinity;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if ( (lineno == DM1B_LRANGE) || (lineno == DM1B_RRANGE))
    {
	/* Comparing leaf::range keys */

	if (lineno == DM1B_LRANGE)
	    *compare = DM1B_SAME + 1;
	else
	    *compare = DM1B_SAME - 1;
	
	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf, lineno, 
		    (DM_TID *)&infinity, (i4*)NULL);
	if (infinity == TRUE)
	{
	    /* not same */
	    return (E_DB_OK);
	}

	if ( s = dm1b_AllocKeyBuf(t->tcb_rngklen, keybuf,
				    &tmpkey, &AllocKbuf, dberr) )
	    return(s);

	tmpkey_len = t->tcb_rngklen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf,
		    t->tcb_rng_rac, lineno,
		    &tmpkey, (DM_TID *)NULL, (i4*)NULL,
		    &tmpkey_len, NULL, NULL, adf_cb);

	if (s == E_DB_WARN && (t->tcb_rel.relpgtype != TCB_PG_V1) )
	    s = E_DB_OK;

	if (s == E_DB_OK)
	{
	    /* Compare leaf key :: range key */
	    s = adt_ixlcmp(adf_cb, keys_given, 
			    t->tcb_leafkeys, key,
			    t->tcb_rngkeys, tmpkey, compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    }
	}
    }
    else
    {
	/* Comparing two leaf keys */
	if ( s = dm1b_AllocKeyBuf(t->tcb_klen, keybuf,
				    &tmpkey, &AllocKbuf, dberr) )
	    return(s);
	tmpkey_len = t->tcb_klen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf,
		    &t->tcb_leaf_rac, lineno,
		    &tmpkey, (DM_TID *)NULL, (i4*)NULL,
		    &tmpkey_len, NULL, NULL, adf_cb);

	if (s == E_DB_WARN && (t->tcb_rel.relpgtype != TCB_PG_V1) )
	    s = E_DB_OK;

	if (s == E_DB_OK)
	{
	    s = adt_kkcmp(adf_cb, keys_given, t->tcb_leafkeys, 
			    key, tmpkey, compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    }
	}
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &tmpkey);

    if (s == E_DB_ERROR)
    {
	dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, leaf,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, lineno );
    }

    return (s);
}

/*{
** Name: dm1b_requalify_leaf - Re-qualify leaf page, since it could have split
**
** Description:
**      This routine compares the key we are searching for to the high   
**      key on the page. If it is less than or equal, we have the correct
**      leaf page fixed. Otherwise we follow forward leaf pointers to
**      the correct leaf.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      mode                            search mode
**      leaf                            Pointer to leaf page
**      key                             Pointer to the key value.
**      keys_given                      Value indicating number of 
**                                      attributes in key. Note that this is
**					the number of attributes to be used for
**					searching, which is not necessarily the
**					same as the number of attributes in the
**					index entry (e.g., partial range search)
**
** Outputs:
**      err_code
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          None
**
** History:
**      
**      21-apr-97 (stial01)
**          Created from code in dm1b_search()
**      21-may-97 (stial01)
**          dm1b_requalify_leaf() New buf_locked parameter.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      05-may-1999 (nanpr01)
**          dm1b_requalify_leaf() If serializable with equal predicate,
**          do not specify DM0P_PHANPRO (phantom protection) when fixing leaf
*/
static DB_STATUS 
dm1b_requalify_leaf(
DMP_RCB     *rcb,
i4     mode,     
DMP_PINFO   *leafPinfo,
char        *key,
i4     keys_given,
DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    DM_PAGENO       nextpageno;
    i4              compare;
    char	    *tempkey, *KeyPtr = NULL, *AllocKbuf = NULL;
    char	    temp_buf[DM1B_MAXSTACKKEY];
    i4	    	    tempkeylen;
    i4         	    leaf_fix_action;       /* action for fixing leaf pages. */
    DB_STATUS       s;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf;
    i4		    orig_mode;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    /* 
    ** Must re-qualify the leaf page, since it could have split
    */
    orig_mode = mode;
    if (mode == DM1C_FIND_SI_UPD)
        mode = DM1C_TID;
    leaf_fix_action = DM0P_READ;            
    if (r->rcb_access_mode == RCB_A_WRITE)
	leaf_fix_action = DM0P_WRITE;
    if ((r->rcb_state & RCB_READAHEAD) == 0)
	leaf_fix_action |= DM0P_NOREADAHEAD;
    if (row_locking(r) && (r->rcb_k_duration == RCB_K_TRAN) &&
       (mode != DM1C_SPLIT) && (mode != DM1C_OPTIM) && (mode != DM1C_TID))
    {
	 if ((r->rcb_p_hk_type != RCB_P_EQ) || (r->rcb_hl_given != t->tcb_keys))
             leaf_fix_action |= DM0P_PHANPRO;
    }

    for (s = E_DB_OK;;)
    {
	if ( !dm0pBufIsLocked(leafPinfo) )
	{
	    /*
	    ** Lock buffer for get.
	    **
	    ** This may produce a CR page and overlay "leaf"
	    ** with its pointer.
	    */
	    if (orig_mode == DM1C_FIND_SI_UPD ||
			mode == DM1C_OPTIM || mode == DM1C_SPLIT)
	    {
		dm0pLockBufForUpd(r, leafPinfo);
	    }
	    else
	    {
		dm0pLockBufForGet(r, leafPinfo, &s, dberr);
		if ( s == E_DB_ERROR )
		    break;
	    }
	}

	if ((nextpageno = 
	    DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf)) 
	    == 0)
	    break;

	/* 
	** Get high key possible on page from page. 
	** Compare with key searched with. If less than
	** or equal, this is the correct leaf page.
	*/

	if (mode != DM1C_EXTREME && mode != DM1C_LAST)
	{
	    if ( !KeyPtr && (s = dm1b_AllocKeyBuf(t->tcb_rngklen, temp_buf,
				&KeyPtr, &AllocKbuf, dberr)) )
		break;
	    tempkey = KeyPtr;
	    tempkeylen = t->tcb_rngklen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			t->tcb_rng_rac,
			DM1B_RRANGE,
			&tempkey, (DM_TID *)NULL, (i4*)NULL, &tempkeylen,
			NULL, NULL, adf_cb);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_RRANGE);
		SETDBERR(dberr, 0, E_DM9264_DM1B_SEARCH);
		break;
	    }
	    /* Compare leafkey :: range key */
	    adt_ixlcmp(adf_cb, keys_given, 
			    t->tcb_leafkeys, key,
			    t->tcb_rngkeys, tempkey, &compare);
	    if (compare <= DM1B_SAME)
		break;
	}

	/*
	** Key we're searching for is greater than the highest key on this
	** page, so advance to the next page.
	*/
	dm0pUnlockBuf(r, leafPinfo);
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;
	s = dm0p_fix_page(r, nextpageno, leaf_fix_action, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    return (s);
}


/*{
** Name: dm1b_rowlk_access - BTREE Row locking access checking primatives
**
** Description:
**
**	This routine determines if a row lock is needed for the specified
**      row access type, and if it is, it gets the row lock.
**
**      For BTREE, the following types of access are defined:
**
**      ROW_ACC_NEWDATA:  Fix a new data page.
**                        Since the leaf buffer is locked, try to fix the
**                        data page with DM0P_NOWAIT specified. If the fix
**                        page fails with err_code E_DM004C_LOCK_RESOURCE_BUSY,
**                        unlock the leaf buffer and wait for the data page
**                        lock.
**
**      ROW_ACC_NONKEY:   Need to do non key qualification.
**                        If serializable, we need row lock to do non-key qual.
**                        For lower isolation levels, we can do non-key
**                        qualifications on committed data without a lock.
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
**			   Will be either OK or WARN, errors are pruned out
**			   by the caller
**      access             Type of row access
**      low_tran
**      tid_to_lock
**	partno_to_lock	   If Global SI, TID's partition number.
** 
**
** Outputs:
**	E_DB_OK if row locked.
**	E_DB_WARN if need to cycle around for another next
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
**      20-oct-99 (stial01)
**          dm1b_rowlk_access() Reposition to last row locked
**	22-Jan-2004 (jenjo02)
**	    Added handling of TID partition number for 
**	    Global indexes.
**	17-Feb-2005 (schka24)
**	    Fix typo in above, used wrong ptr to get to global index
**	    base table.  Somehow nobody tripped over it until now!
**	13-Aug-2010 (kschendel) b124255
**	    "Get" position was being saved prematurely after coming out
**	    of a lock-wait, before we've checked the data page stuff.
**	    If the data page turned out to not be valid, we need to
**	    reposition to the saved "get" position, which because of the
**	    premature save was the current position and not the last-locked
**	    position, ie it's the wrong place.  When we return back to
**	    the get main loop we'd miss records.
*/
static DB_STATUS
dm1b_rowlk_access(
    DMP_RCB        *rcb,
    i4        get_status,
    i4        access,
    u_i4	   *low_tran_ptr,
    u_i2	   *lg_id_ptr,
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
    bool            need_rowlock = FALSE;
    bool            need_xn_stat = FALSE;
    DM_TID          unlock_tid;
    i4         fix_action;     /* Type of access to page. */
    bool            need_reposition = FALSE;
    LG_LSN          page_lsn;
    LG_LSN          data_lsn;
    DM_TID          save_bid;
    bool            data_was_locked = FALSE;
    DB_TAB_ID	    row_reltid;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    i4		    lkwait = 0;
    DMP_POS_INFO    orig_posinfo;
    LK_LOCK_KEY     temp_lock_key;

    CLRDBERR(dberr);

    if (r->rcb_access_mode == RCB_A_READ)
	fix_action = DM0P_READ;
    else
	fix_action = DM0P_WRITE;

    /*
    ** Save page lsn, page clean count
    ** To be used to determine if we need to reposition after LKWAIT
    */
    DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_lowtid, RCB_P_TEMP);
    save_bid.tid_i4 = r->rcb_lowtid.tid_i4;
    STRUCT_ASSIGN_MACRO(r->rcb_pos_info[RCB_P_TEMP], orig_posinfo);

    /* 
    ** Do we need a lock for the specified row access ?
    */
    if ( (access == ROW_ACC_RETURN) ||
	(access == ROW_ACC_NONKEY && r->rcb_iso_level == RCB_SERIALIZABLE))
    {
	need_rowlock = TRUE;
    }
    else if (access == ROW_ACC_NEWDATA)
    {
	s = dm0p_fix_page(r, (DM_PAGENO )tid_to_lock->tid_tid.tid_page,
		fix_action | DM0P_NOWAIT, &r->rcb_data, dberr);

	if (s == E_DB_OK) 
	    return (E_DB_OK);

	if (dberr->err_code != E_DM004C_LOCK_RESOURCE_BUSY)
	    return (E_DB_ERROR);
    
	/* Need to WAIT for lock, unlock leaf buffer first */
	lkwait++;
	dm0pUnlockBuf(r, &r->rcb_other);
    
	s = dm0p_fix_page(r, (DM_PAGENO )tid_to_lock->tid_tid.tid_page,
		fix_action, &r->rcb_data, dberr);

	if (s != E_DB_OK)
	    return (E_DB_ERROR);

	/* The data page is fixed, reposition to re-get same row */
	need_rowlock = FALSE;
	need_reposition = TRUE;

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("ROW_ACC_NEWDATA %p LKWAIT %d, xid %x table %d bid %d,%d tid %d,%d acc %d\n",
		r, lkwait, r->rcb_tran_id.db_low_tran, t->tcb_rel.reltid.db_tab_base, 
		save_bid.tid_tid.tid_page, save_bid.tid_tid.tid_line,
		tid_to_lock->tid_tid.tid_page, tid_to_lock->tid_tid.tid_line, access);
	}
    }
    else
    {
	if (!IsTranActive(*lg_id_ptr, *low_tran_ptr))
	{
	    /* Reset tranid in tuple header to avoid extra calls to LG */
	    *low_tran_ptr = 0;
	    *lg_id_ptr = 0;

	    /* Return original get status so called can skip if it's a
	    ** committed delete, or use the row if it's ok.
	    ** Data is committed... We don't need to lock yet.
	    */
	    return (get_status);
	}
	else
	{
	    /* uncommitted data, we need the row lock */
	    need_rowlock = TRUE;
	}
    }

    if (need_rowlock == FALSE && need_reposition == FALSE)
	return (E_DB_OK);

    MEfill(sizeof(temp_lock_key), 0, &temp_lock_key);
    if (need_rowlock)
    {
	if (tid_to_lock == NULL)
	    return (E_DB_ERROR);


	/* Determine TID's reltid */
	if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    /*
	    ** Then we have to find the reltid of the
	    ** TID's partition.
	    **
	    ** Caller, knowing this is a Global index,
	    ** must have set "partno_to_lock" to this TID's
	    ** partition number, which we use to
	    ** index into the Master TCB's partition array
	    ** and extract the TID's reltid to be used
	    ** in the lock key instead of the parent TCB's
	    ** reltid.
	    **
	    ** Since it's a global index, the parent TCB is the
	    ** partitioned master.
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
	else if (r->rcb_iso_level == RCB_CURSOR_STABILITY || 
	    access != ROW_ACC_RETURN)
	    flags = DM1R_LK_PHYSICAL;
	else
	    flags = 0; /* Default is logical row lock */

	/*
	** If primary key projection optimization, we need dm1r_lock_row
	** to get intent data page lock as well as the row lock
	*/
	if (opflag & DM1C_PKEY_PROJECTION)
	    flags |= DM1R_LK_PKEY_PROJ;

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
		if (r->rcb_iso_level == RCB_SERIALIZABLE ||
			    (flags & DM1R_LK_PHYSICAL))
		{
		    DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_lowtid, RCB_P_GET);
		    return (E_DB_OK);
		}
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

	/* Pass partno on to dm1r_lock_row */
	r->rcb_partno = partno_to_lock;

	if (dm0pBufIsLocked(&r->rcb_other) || dm0pBufIsLocked(&r->rcb_data))
	    flags |= DM1R_LK_NOWAIT; /* can't wait if buffer is locked */

	if (crow_locking(r))
	{
	    /* pinfo == NULL -> no update conflict resolution */
	    lock_status = dm1r_crow_lock(r, flags, tid_to_lock, NULL, dberr);
	}
	else
	{
	    lock_status  = dm1r_lock_row(r, flags, 
			    tid_to_lock, &new_lock, &temp_lock_key, dberr);
	}

	/* if we waited and lock not granted, dont try again */
	if (lock_status == E_DB_ERROR && (flags & DM1R_LK_NOWAIT) == 0)
	    return (E_DB_ERROR);

	if (lock_status == E_DB_OK)
	{
	    /* copy lock key only if granted */
	    STRUCT_ASSIGN_MACRO(temp_lock_key, *save_lock_key);

	    /*
	    ** Save the key,tidp,lsn and clean count for reposition
	    ** to the last LOCKED record. 
	    */
	    DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_lowtid, RCB_P_GET);

	    return (get_status);
	}

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
	flags &= ~DM1R_LK_NOWAIT; /* now we wait */
	if (r->rcb_iso_level != RCB_SERIALIZABLE && flags != DM1R_LK_CONVERT)
	    flags = DM1R_LK_PHYSICAL;

	/*
	** Need to WAIT for lock, unlock buffer(s) first
	*/
	if ( dm0pBufIsLocked(&r->rcb_data) )
	{
	    data_was_locked = TRUE;
	    DMPP_VPT_GET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page, data_lsn);
	    dm0pUnlockBuf(r, &r->rcb_data);
	}

	dm0pUnlockBuf(r, &r->rcb_other);

	lkwait++;
	if (crow_locking(r))
	{
	    /* pinfo == NULL -> no update conflict resolution */
	    lock_status = dm1r_crow_lock(r, flags, tid_to_lock, NULL, dberr);
	}
	else
	{
	    lock_status  = dm1r_lock_row(r, flags, 
			tid_to_lock, &new_lock, &temp_lock_key, dberr);
	}

	if (lock_status == E_DB_ERROR)
	    return (E_DB_ERROR);

	/* copy lock key only if granted */
	STRUCT_ASSIGN_MACRO(temp_lock_key, *save_lock_key);
    }

    /*
    ** Re-Lock buffer for get.
    **
    ** This may produce a CR page and overlay "rcb_other.page"
    ** with its pointer.
    */
    dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
    if ( s == E_DB_ERROR )
	return (E_DB_ERROR);

    /*
    ** DONT try to reposition if the leaf hasn't changed
    ** Skip this entry if its deleted (and no rollback done)
    */
    if (DM1B_POSITION_PAGE_COMPARE_MACRO(t->tcb_rel.relpgtype, 
		r->rcb_other.page, &orig_posinfo) == DM1B_SAME)
    {
	bool data_valid = TRUE;

	/*
	** Leaf page hasn't changed... check if data page has
	*/
	if (data_was_locked)
	{
	    /*
	    ** Lock buffer for get.
	    **
	    ** This may produce a CR page and overlay "r->rcb_data.page"
	    ** with its pointer.
	    */
	    dm0pLockBufForGet(r, &r->rcb_data, &s, dberr);
	    if ( s == E_DB_ERROR )
		return (E_DB_ERROR);

	    DMPP_VPT_GET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, 
			r->rcb_data.page, page_lsn);
	    if (! LSN_EQ(&data_lsn, &page_lsn))
	    {
		dm0pUnlockBuf(r, &r->rcb_data);
		data_valid = FALSE;
	    }
	}
	if (data_valid)
	{
	    /*
	    ** Save position info 
	    ** for reposition to start of this leaf, or the last LOCKED record. 
	    */
	    DM1B_POSITION_SAVE_MACRO(r, r->rcb_other.page, &r->rcb_lowtid, RCB_P_GET);
	    return (get_status);
	}
    }

    if (DMZ_AM_MACRO(5))
    {
	if (DM1B_POSITION_VALID_MACRO(r, RCB_P_GET))
	{
	    TRdisplay("REPOSITION after LKWAIT, %p, xid %x table %d\n  get saved-bid %d,%d tid %d,%d acc %d\n",
		r, r->rcb_tran_id.db_low_tran, t->tcb_rel.reltid.db_tab_base,
		r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_page,
		r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_line,
		tid_to_lock->tid_tid.tid_page, tid_to_lock->tid_tid.tid_line, access);
	}
	else
	{
	    TRdisplay("REPOSITION after LKWAIT, %p, xid %x table %d\n  %s start saved-bid %d,%d tid %d,%d acc %d\n",
		r, r->rcb_tran_id.db_low_tran, t->tcb_rel.reltid.db_tab_base,
		(DM1B_POSITION_VALID_MACRO(r, RCB_P_START)) ? "valid" : "invalid",
		r->rcb_pos_info[RCB_P_START].bid.tid_tid.tid_page,
		r->rcb_pos_info[RCB_P_START].bid.tid_tid.tid_line,
		tid_to_lock->tid_tid.tid_page, tid_to_lock->tid_tid.tid_line, access);
	}
    }

    /*
    ** Try to reposition to last row locked.
    ** For example, assume keys of 31,31,31,32,32,32,33
    ** If we just looked at the first key having value 32 and had to 
    ** wait for the lock....while we wait more keys having value 32 may
    ** have been inserted/committed.
    ** Btree Data/Row lock coupling should be done here (not in dm0p/dm1r)
    */
    s = E_DB_OK;
    if (DM1B_POSITION_VALID_MACRO(r, RCB_P_GET) == FALSE)
    {
	/* We haven't locked any records yet */
	s = btree_position(r, dberr);
    }
    else
    {
	/* We need to position to last record locked */
	if (r->rcb_lowtid.tid_tid.tid_page !=
	    r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_page)
	{
	    /* To reposition, we have to go back to fetch leaf page */
	    dm0pUnlockBuf(r, &r->rcb_other);

	    s = dm0p_unfix_page(r, DM0P_UNFIX,&r->rcb_other, dberr);
	    if (s != E_DB_OK)
		return (E_DB_ERROR);

	    s = dm0p_fix_page(r, 
		    (DM_PAGENO)r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_page, 
		    fix_action, &r->rcb_other, dberr);
	    if (s != E_DB_OK)
		return (E_DB_ERROR);

	    /*
	    ** Lock buffer for get.
	    **
	    ** This may produce a CR page and overlay "rcb_other.page"
	    ** with its pointer.
	    */
	    dm0pLockBufForGet(r, &r->rcb_other, &s, dberr);
	    if ( s == E_DB_ERROR )
		return (E_DB_ERROR);
	}

#ifdef xDEBUG
	TRdisplay("Reposition last row locked op %d key %d tid %d,%d\n",
	    access, *((int *)r->rcb_repos_key_ptr), 
	    r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_page,
	    r->rcb_pos_info[RCB_P_GET].bid.tid_tid.tid_line);
#endif

	/* Reset lowtid back to last good position */
	STRUCT_ASSIGN_MACRO(r->rcb_pos_info[RCB_P_GET].bid, r->rcb_lowtid);

	s = btree_reposition(r, &r->rcb_other,
		&r->rcb_lowtid, r->rcb_repos_key_ptr,
		RCB_P_GET, opflag, dberr);


	if (s == E_DB_OK)
	{
	    /* Current position is invalid until we do NEXT/PREV */
	    return (E_DB_WARN);
	}
	else
	{
	    /* Previous row locked not found
	    ** If CS or RR, we may have unlocked it and it may have 
	    ** been deleted
	    ** Btree Data page/row locking coupling should be done in here
	    ** not in dm0p/dm1r
	    */
	    TRdisplay("Can't reposition to locked row \n");
	    return (E_DB_ERROR);
	}
    }

    if (s)
    {
	dm0pUnlockBuf(r, &r->rcb_other);
	if (dberr->err_code != E_DM0055_NONEXT)
	    TRdisplay("dm1b_rowlk_access error %d on table %d\n",
		dberr->err_code, t->tcb_rel.reltid.db_tab_base);
	return (E_DB_ERROR);
    }
    else
	return (E_DB_WARN);
}


/*{
** Name: dm1b_kperpage - Calculate keys per page
**
** Description:
**      This routine calculates keys per page which varies depending on
**      page size, attribute count, key length and page type (index vs. leaf).
**      Also the DMF table version is required for backwards compatibility.
**
** Inputs:
**      page_size          Page size
**      relcmptlvl         DMF table version
**      atts_cnt           Number of attributes
**      key_len            Key length
**      page_type          index vs. leaf
** 
** Outputs:
**      keys per page
**
**
** Exceptions:
**		None
**
** History:
**      07-Dec-98 (stial01)
**          Written.
**      09-Feb-99 (stial01)
**          dm1b_kperpage() relcmptlvl param added for backwards compatability
*/
i4
dm1b_kperpage(i4 page_type,
	i4 page_size,
        i4      relcmptlvl,
	i4 atts_cnt,
	i4 key_len,
	i4 indexpagetype,
	i2 tidsize,
	i4 Clustered)
{
    return (dm1cxkperpage(page_type, page_size, relcmptlvl, TCB_BTREE, atts_cnt, key_len, indexpagetype, tidsize, Clustered));
}

VOID
dm1b_build_key(
i4        keycnt,
DB_ATTS **srckeys,
char      *srcbuf,
DB_ATTS **dstkeys,
char      *dstbuf)
{
    i4		k;

    /* Don't assume keys are contiguous */
    for ( k = 0; k < keycnt; k++ )
	MEcopy(srcbuf + srckeys[k]->key_offset, srckeys[k]->length,
	       dstbuf + dstkeys[k]->key_offset);
}

VOID
dm1b_build_ixkey(
i4        keycnt,
DB_ATTS **srckeys,
char      *srcbuf,
DB_ATTS **dstkeys,
char      *dstbuf)
{
    i4		k;

    /* Transforms a Leaf key into an Index key */
    for ( k = 0; k < keycnt; k++ )
	MEcopy(srcbuf + srckeys[k]->key_offset, srckeys[k]->length,
	       dstbuf + dstkeys[k]->key_offset);
}


/*{
** Name: dm1b_online_get - Online get 
**
** Description:
**      This routine retrieves bid from leaf, fixes appropriate
**	data page, if first time reading data page, reads all
**	data page tuples. 
**	With btree, data page rows are scanned through leaf
**	page entries. For online operation - scanning btree
**	with readnolock, it is possible for a data page to 
**	be pointed to by 2 or more leaf pages. This prompted
**	the creation of a routine to scan a btree with 
**	readnolock, visiting tuples only once. To accomplish 
**	this, there were three possible scan methods considered:
**	-Option #1: Scan entire file. If data page, retrieve 
**	records. Similarto how verify -mrun modifies a btree
**	into a heap table. 
**	    disadvantage: inefficient sort
**	    advantage:    does not rely on searching tree
**	-Option #2: Scan tree through leaf page entries,
**	 retrieving rows off of first time read data page. 
**	    disadvantage: need to keep track of data pages
**			  already read
**	    advantage: data is somewhat sorted
**	-Option #3: Maintain a queue of readnolock buffers 
**	 to copy data pages into, and when queue is exhausted,
**	 victim is chosen, remaining rows to be read off.
**	    disadvantage: fixed number of readnolock buffers
**			  on queue. Depending on table size
**			  may need to read rows off of data
**			  page eventually. Need to maintain
**			  RNL queue, track data pages read &
**			  which rows read off data page.
**	    advantage: data is more sorted 
**			
**
**
** Inputs:
**      rcb		record control block
**      rettid		return tid ptr
**      record		record ptr
**      opflag		operation flag
**      err_code	error code
**
** Outputs:
**	tid of tuple retrieved
**      record
**	err_code
**
** Exceptions:
**	None
**
** History:
**	03-Feb-2004 (thaju02)
**	    Created.
**	07-may-2004 (thaju02)
**	    For rolldb of online modify, do not read from leaf/data if not
**	    in the rnl lsn array. Added E_DM9CB2_NO_RNL_LSN_ENTRY.
**	20-may-2004 (thaju02)
**	    Removed call to btree_reposition(), resulted in dbms looping. (B112392)
**	    If key/row has been deleted, skip it. (B112393)
**	04-jun-2004 (thaju02)
**	    For rolldb of online modify, added handling of E_DM9CB1 -
**	    rolldb requires the ability to halt/restart load. (B112428)
**	28-jun-2004 (thaju02)
**	    For rolldb, if leaf/data page lsn does not match the rnl lsn
**	    unfix the page, before we return. (B112566)
**	14-jul-2004 (thaju02)
**	    Check that data page is within btree read array range
**	    before we test page bit. (B112659)
**	27-aug-2004 (thaju02)
**	    Online modify of btree may fail with E_DM0055. Last row
**	    on assoc data is deleted. dm1b_online_get reads leaf pg,
**	    empty assoc data pg lsn not stored in rnl lsn array. 
**	    Sidefile processing encounters log record, no rnl lsn entry 
**	    found, log record to be "done". position_to_row unable to
**	    locate record in $online. (B112886)
**	19-oct-2004 (thaju02)
**	    Another instance of E_DM0055 due to empty assoc data pg. 
**	    Previous change has been centralized in NEXT_KEYTID check.
**	31-jan-2005 (thaju02)
**	    Reset bid tid_line to DM_TIDEOF if assoc data pg already 
**	    read and moving onto the next leaf/overflow.
**	    After reading leaf's assoc pg, set bit in btree map. 
**	    Modified TRdisplays to display table/partition name.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	11-Nov-2005 (jenjo02)
**	    Removed bogus dm0p_check_logfull().
**	13-Apr-2006 (jenjo02)
**	    CLUSTERED leaf entries -are- the row.
**	    
*/
DB_STATUS
dm1b_online_get(
    DMP_RCB		*rcb,
    DM_TID		*rettid,
    char		*record,
    i4			opflag,
    DB_ERROR		*dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;   
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMP_RNL_ONLINE	*rnl = r->rcb_rnl_online;
    DM_TID		*bid = &r->rcb_lowtid;
    DM_TID		*tid = &r->rcb_currenttid;
    i4			fix_action;     /* Type of access to page. */
    char		*rec_ptr = record;
    DM_TID		unlock_tid;
    i4			record_size;
    bool		reposition = TRUE;
    char		*key;
    char		*rcbkeyptr;
    i4			key_len;
    DB_STATUS		s = E_DB_OK;              /* return status. */
    ADF_CB		*adf_cb;
    DB_STATUS		get_status;
    i4			row_version = 0;
    DM_PAGENO		assoc;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Assumes: 
    ** we are retrieving all records w/ readnolock
    ** rcb_hl_given = 0
    */

   if ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) )
        rcbkeyptr = r->rcb_record_ptr;
    else
        rcbkeyptr = r->rcb_s_key_ptr;
    key = rcbkeyptr;

    fix_action = DM0P_WRITE;
    if (r->rcb_access_mode == RCB_A_READ)
        fix_action = DM0P_READ;

    if (DMZ_SRT_MACRO(13))
    {
        TRdisplay("dm1b_online_get: %~t entering bid = (%d,%d) \
tid = (%d,%d) rnl_btree_flags = 0x%x\n",
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
                bid->tid_tid.tid_page, bid->tid_tid.tid_line,
		tid->tid_tid.tid_page, tid->tid_tid.tid_line,
                rnl->rnl_btree_flags);
    }

    for (;;)
    {
	/* Check for external interrupts */
	if ( *(r->rcb_uiptr) )
	{
	    /* check to see if user interrupt occurred. */
	    if ( *(r->rcb_uiptr) & RCB_USER_INTR )
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    /* check to see if force abort occurred */
	    else if ( *(r->rcb_uiptr) & RCB_FORCE_ABORT )
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);

	    if ( dberr->err_code )
	    {
		s = E_DB_ERROR;
		break;
	    }
	}

	if (r->rcb_other.page)
	    assoc = DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page);

	if (rnl->rnl_btree_flags & RNL_NEED_LEAF)
	{
	    if (r->rcb_other.page && (rnl->rnl_btree_flags & RNL_NEXT))
	    {
		if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
				r->rcb_other.page) != 0)
		    bid->tid_tid.tid_page = DM1B_VPT_GET_PAGE_OVFL_MACRO(
				t->tcb_rel.relpgtype, r->rcb_other.page);
		else if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
				r->rcb_other.page) != 0)
		    bid->tid_tid.tid_page = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(
				t->tcb_rel.relpgtype, r->rcb_other.page);
		else
		{
		    if (DMZ_SRT_MACRO(13))
			TRdisplay("dm1b_online_get: finished scanning %~t\n",
				sizeof(DB_TAB_NAME), 
				t->tcb_rel.relid.db_tab_name);
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    return(E_DB_ERROR);
		}
		rnl->rnl_btree_flags &= ~RNL_NEXT;
	    } /* RNL_NEXT */

	    if (r->rcb_other.page && bid->tid_tid.tid_page != 
	       DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page))
	    {
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("dm1b_online_get: %~t UNFIX leaf page %d\n",
				sizeof(DB_TAB_NAME),
				t->tcb_rel.relid.db_tab_name,
				DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
				r->rcb_other.page));

	    	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	    	if (s != E_DB_OK)
		    break;
	    }
	    if (r->rcb_other.page == NULL)
	    {
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("dm1b_online_get: %~t FIX leaf page %d\n",
				sizeof(DB_TAB_NAME),
				t->tcb_rel.relid.db_tab_name,
				bid->tid_tid.tid_page);

	    	s = dm0p_fix_page(r, (DM_PAGENO)bid->tid_tid.tid_page,
                		fix_action, &r->rcb_other, dberr);

		if ((t->tcb_dcb_ptr->dcb_status & DCB_S_ROLLFORWARD) &&
		    (r->rcb_rnl_online) &&
		    (s == E_DB_INFO))
		{
		    if (dberr->err_code == E_DM9CB2_NO_RNL_LSN_ENTRY)
		    {
			/* skip reading this leaf page */
			rnl->rnl_btree_flags |= RNL_NEXT;
			CLRDBERR(dberr);
			s = E_DB_OK;
			continue;
		    }
	
		    if (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH)
		    {
			DB_STATUS	locstat = E_DB_OK;
			DB_ERROR	local_dberr;
			locstat = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
						&local_dberr);
			if (locstat)
			{
			    s = locstat;
			    *dberr = local_dberr;
			}
		    }
		}
	    	if (s != E_DB_OK)
		    break;

		assoc = DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page);
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("dm1b_online_get: %~t leaf page %d assoc is %d\n",
				sizeof(DB_TAB_NAME),
				t->tcb_rel.relid.db_tab_name,
				bid->tid_tid.tid_page, 
				DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page));
 
		/* FIX ME - do we need this? */
	    	r->rcb_state &= ~RCB_LPAGE_HAD_RECORDS;
	    	r->rcb_state &= ~RCB_LPAGE_UPDATED;
	    }

    	    if (bid->tid_tid.tid_line == DM_TIDEOF)
    	    {
	    	if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page) > 0)
	    	    bid->tid_tid.tid_line = -1;
	    	else
	    	{
	    	    rnl->rnl_btree_flags |= RNL_NEXT;
	    	    continue;
	    	}
    	    }

	    /* now we have a leaf with entries on it */
	    rnl->rnl_btree_flags &= ~RNL_NEED_LEAF;

    	} /* RNL_NEED_LEAF */

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("dm1b_online_get: %~t current bid is (%d, %d) rnl_btree_flags=0x%x\n",
			sizeof(DB_TAB_NAME),
			t->tcb_rel.relid.db_tab_name,
			bid->tid_tid.tid_page, bid->tid_tid.tid_line,
			rnl->rnl_btree_flags);

	if (rnl->rnl_btree_flags & RNL_NEED_KEYTID)
	{
	    if (++bid->tid_tid.tid_line >
                    (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
                    r->rcb_other.page) - 1))
	    {
		/* exhausted bid entries on leaf, need next ovfl/leaf */
		if (BTtest(assoc, (char *)rnl->rnl_btree_map))
		{
		    /* assoc data pg read, get next leaf/ovfl */
		    rnl->rnl_btree_flags |= (RNL_NEED_LEAF | RNL_NEXT);
		    bid->tid_tid.tid_line = DM_TIDEOF;
		    continue;
		}
		else
		{
		    /* read assoc data pg */
		    tid->tid_tid.tid_page = assoc;
		    tid->tid_tid.tid_line = 0;
		    rnl->rnl_btree_flags &= ~RNL_NEED_KEYTID;
		    rnl->rnl_btree_flags |= RNL_READ_ASSOC;
		    if (DMZ_SRT_MACRO(13))
		        TRdisplay("dm1b_online_get: %~t leaf %d's assoc pg %d \
needs to be read rnl_btree_flags=0x%x\n",
				sizeof(DB_TAB_NAME),
				t->tcb_rel.relid.db_tab_name,
				bid->tid_tid.tid_page, assoc, 
				rnl->rnl_btree_flags);
		}
	    }
	    else
	    {

		/* Retrieve key,tid from leaf */
		key = rcbkeyptr;
		key_len = t->tcb_klen;
	    
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				r->rcb_other.page,
				&t->tcb_leaf_rac,
				bid->tid_tid.tid_line, 
				&key, tid, &r->rcb_partno,
				&key_len, NULL, NULL, adf_cb);

		get_status = s;
		/* Online modify, readnolock, Skip deleted keys */
		if (get_status == E_DB_WARN) 
		    continue;

		if (s != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r,
				r->rcb_other.page,
				t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				bid->tid_tid.tid_line );
		    /* FIX ME - wrong err code */
		    SETDBERR(dberr, 0, E_DM9263_DM1B_REPLACE);
		    break;
		}

		/*
		** For primary btree, skip 'reserved' leaf entries
		** which have invalid tid
		*/
		if ((t->tcb_rel.relpgtype != TCB_PG_V1)
			&& ((t->tcb_rel.relstat & TCB_INDEX) == 0)
			&& (tid->tid_tid.tid_page == 0)
			&& (tid->tid_tid.tid_line == DM_TIDEOF))
		{
		    continue;
		}

		/* If CLUSTERED, this copies the complete row */
		if (key != rcbkeyptr)
		    MEcopy(key, key_len, rcbkeyptr);

		if ( t->tcb_rel.relstat & (TCB_INDEX | TCB_CLUSTERED) )
		{
		    /* Copy the key to the callers record buffer */
		    MEcopy(r->rcb_record_ptr, t->tcb_rel.relwid, record);
		    rec_ptr = record;
		    if (t->tcb_klen != t->tcb_rel.relwid)
			MEcopy((char *)tid, DM1B_TID_S, record + 
					    t->tcb_rel.relwid - DM1B_TID_S);

		    rettid->tid_i4 = bid->tid_i4;
		    bid->tid_tid.tid_line++;  /* setup for next get */
		    return(E_DB_OK);
		}
		else if ((tid->tid_tid.tid_page <= rnl->rnl_btree_xmap_cnt) &&
		    (BTtest(tid->tid_tid.tid_page, (char *)rnl->rnl_btree_map)))
		{
		    /* 
		    ** we have already read data page referenced in tid,
		    ** skip this data page and get next bid (key,tid)
		    */
		    if (DMZ_SRT_MACRO(13))
		    {
			TRdisplay("dm1b_online_get: %~t skip, previously read \
all rows on data page %d (bid = (%d,%d)) get next bid\n",
				    sizeof(DB_TAB_NAME),
				    t->tcb_rel.relid.db_tab_name,
				    tid->tid_tid.tid_page, 
				    bid->tid_tid.tid_page,
				    bid->tid_tid.tid_line);
		    }
		    continue;
		}
		else
		{
		    /* base table - setup to read all data pg records */
		    tid->tid_tid.tid_line = 0; 
		    rnl->rnl_btree_flags &= ~RNL_NEED_KEYTID;
		}

		if (DMZ_SRT_MACRO(13))
		{
		    TRdisplay("dm1b_online_get: %~t getting bid = (%d,%d) \
tid = (%d,%d) rnl_btree_flags = %x\n",
			    sizeof(DB_TAB_NAME),
			    t->tcb_rel.relid.db_tab_name,
			    bid->tid_tid.tid_page,bid->tid_tid.tid_line,
			    tid->tid_tid.tid_page, tid->tid_tid.tid_line,
			    rnl->rnl_btree_flags);
		}
	    }
	} /* RNL_NEED_KEYTID */

	/* 
	** Now we have a leaf page, fix data page and retrieve rows.
	** Normal get next.
	*/ 

        /*
        ** If we need a different data page, unfix the old one
        ** before we lock the new page/row
        */
	if (r->rcb_data.page && (tid->tid_tid.tid_page !=
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			r->rcb_data.page)))
	{
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("dm1b_online_get: %~t UNFIX data page %d\n",
		 	sizeof(DB_TAB_NAME),
			t->tcb_rel.relid.db_tab_name,
			DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			r->rcb_data.page));

	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);
	    if (s != E_DB_OK)
		break;
	}

	if (r->rcb_data.page == NULL)
	{
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("dm1b_online_get: %~t FIX data page %d\n",
			sizeof(DB_TAB_NAME),
			t->tcb_rel.relid.db_tab_name,
			tid->tid_tid.tid_page);

	    s = dm0p_fix_page(r, (DM_PAGENO )tid->tid_tid.tid_page,
			fix_action, &r->rcb_data, dberr);

	    if ((t->tcb_dcb_ptr->dcb_status & DCB_S_ROLLFORWARD) &&
		(r->rcb_rnl_online) &&
		(s == E_DB_INFO))
	    {
		if (dberr->err_code == E_DM9CB2_NO_RNL_LSN_ENTRY)
		{
		    if ( (rnl->rnl_btree_flags & RNL_READ_ASSOC) &&
			 (assoc == tid->tid_tid.tid_page) )
		    {
			rnl->rnl_btree_flags &= ~RNL_READ_ASSOC;
			BTset(tid->tid_tid.tid_page, 
				(char *)rnl->rnl_btree_map);
		    }

		    /* skip reading this data page */
		    rnl->rnl_btree_flags |= RNL_NEED_KEYTID;
		    CLRDBERR(dberr);
		    s = E_DB_OK;
		    continue;
		}

		if (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH)
		{
		    DB_STATUS       locstat = E_DB_OK;
		    DB_ERROR	local_dberr;
		    locstat = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data,
						&local_dberr);
		    if (locstat)
		    {
			s = locstat;
			*dberr = local_dberr;
		    }
		}
	    }

	    if (s != E_DB_OK)
		break;
	}

	/*
	** Get primary btree data record
	*/
	record_size = r->rcb_proj_relwid;

	s = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype, 
		    t->tcb_rel.relpgsize, r->rcb_data.page, tid, &record_size,
		    &rec_ptr, &row_version, NULL, NULL, (DMPP_SEG_HDR *)NULL); 

	/* Online modify, readnolock, Skip deleted records */
	if (s == E_DB_WARN)
	{
	    tid->tid_tid.tid_line++;
	    continue;
	}

	/* Additional get processing if compressed, altered, or segmented */
	if (s == E_DB_OK && 
	    (t->tcb_data_rac.compression_type != TCB_C_NONE ||
	    (t->tcb_rel.relencflags & TCB_ENCRYPTED) ||
	    row_version != r->rcb_proj_relversion ||
	    t->tcb_seg_rows))
	{
	    s = dm1c_get(r, r->rcb_data.page, tid, record, dberr);
	    if (s && dberr->err_code != E_DM938B_INCONSISTENT_ROW)
		break;
	    rec_ptr = record;
	}

	if (s != E_DB_OK)
	{
	    /*
	    ** DM1C_GET or UNCOMPRESS returned an error - this indicates
	    ** that something is wrong with the tuple at the current
	    ** tid.
	    ** If we reach this point while reading nolock, just go on
	    ** to the next record.
	    */
	    if (r->rcb_k_duration & RCB_K_READ)
	    {
		if (tid->tid_tid.tid_line >= 
			DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype,
			r->rcb_data.page))
		{
		    /*
		    ** note if this is the first time we are reading 
		    ** this data page
		    */
		    if (tid->tid_tid.tid_page > rnl->rnl_btree_xmap_cnt)
		    {
			u_i4		page_number = tid->tid_tid.tid_page;
			DMP_MISC	*new_xmap;
			char		*new_map;
			i4		new_cnt;
			i4		size;

			/* realloc bigger map */
			new_cnt = page_number + (page_number/5);
			if (new_cnt > rnl->rnl_dm1p_max)
			    new_cnt = rnl->rnl_dm1p_max;
			size = sizeof(DMP_MISC) + ((new_cnt + 1) / 8) + 1;

			/* Get LongTerm memory */
			s = dm0m_allocate( size, (DM0M_ZERO | DM0M_LONGTERM),
				(i4) MISC_CB,
				(i4) MISC_ASCII_ID, (char *) 0,
				(DM_OBJECT **) &new_xmap, dberr);
			if ( s != E_DB_OK )
			    break;

			if (DMZ_SRT_MACRO(13))
			{
			    TRdisplay("dm1b_online_get: %~t realloc \
size %d %p btree data pg read xmap %d -> %d\n",
				sizeof(DB_TAB_NAME), t->tcb_rel.relid,
				size, new_xmap, rnl->rnl_btree_xmap_cnt, 
				new_cnt);
			}

			new_map = (char *)&new_xmap[1];
			new_xmap->misc_data = (char *)new_map;
			MEcopy(rnl->rnl_btree_map, ((rnl->rnl_btree_xmap_cnt + 1) / 8) + 1, new_map);
			dm0m_deallocate((DM_OBJECT **)&rnl->rnl_btree_xmap);
			rnl->rnl_btree_xmap = new_xmap;
			rnl->rnl_btree_map = new_map;
			rnl->rnl_btree_xmap_cnt = new_cnt;
		    }

		    BTset(tid->tid_tid.tid_page, (char *)rnl->rnl_btree_map);

		    if (DMZ_SRT_MACRO(13))
			TRdisplay("dm1b_online_get: %~t finished retrieving all \
records from data page %d\n",
				sizeof(DB_TAB_NAME),
				t->tcb_rel.relid.db_tab_name,
				tid->tid_tid.tid_page);

		    /* read assoc data pg, get next leaf pg */
		    if (rnl->rnl_btree_flags & RNL_READ_ASSOC)
		    {
			rnl->rnl_btree_flags &= ~RNL_READ_ASSOC;
			rnl->rnl_btree_flags |= (RNL_NEED_LEAF | RNL_NEXT);
			bid->tid_tid.tid_line = DM_TIDEOF;
		    }
		    rnl->rnl_btree_flags |= RNL_NEED_KEYTID;  /* get next bid */
		}
		else
		    /* get next row on data page */
		    tid->tid_tid.tid_line++;
		continue;
	    }
	}

	/* If there are encrypted columns, decrypt the record */
	if (t->tcb_rel.relencflags & TCB_ENCRYPTED)
	{
	    s = dm1e_aes_decrypt(r, &t->tcb_data_rac, rec_ptr, record,
			r->rcb_erecord_ptr, dberr);
	    if (s != E_DB_OK)
		break;
	    rec_ptr = record;
	}

	rettid->tid_i4 = tid->tid_i4; /* return current tid to caller */
	tid->tid_tid.tid_line++; /* setup for next get */

	/*
	** Note: If the table being examined has extensions,
	** No need to fill in indirect pointers (dm1c_pget)
	*/
	
	/* Copy callers buffer */
	if (rec_ptr != record)
            MEcopy(rec_ptr, record_size, record);

	if (DMZ_SRT_MACRO(13))
	{
	    TRdisplay("dm1b_online_get: %~t RETRIEVED record at \
bid = (%d,%d) currenttid = (%d,%d)\n",
		sizeof(DB_TAB_NAME),
		t->tcb_rel.relid.db_tab_name,
                bid->tid_tid.tid_page,
                bid->tid_tid.tid_line,
                rettid->tid_tid.tid_page,
                rettid->tid_tid.tid_line);
            /* dmd_prrecord(r, rec_ptr); */
	}
	return(E_DB_OK);

    }  /* for (;;) */

    if ((dberr->err_code > E_DM_INTERNAL) && 
	(dberr->err_code != E_DM9CB1_RNL_LSN_MISMATCH))
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	/* FIX ME - need new error code */
	SETDBERR(dberr, 0, E_DM9261_DM1B_GET);
    }
    return (s);
}


/*{
** Name: VerifyHintTidAndKey - Resolves the Clustered Table Hint Tid and Cluster Key
**
** Description:
**      This routine takes a Hint TID from a secondary index on a Clustered
**	table and checks that the Hint is for the matching row based on
**	Cluster Key. If not, it first searches the current leaf page,
**	then the Btree by key. It does not expect to fail in this quest.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	HintTid				The Hint TID from the secondary
**					index row.
**	ClusterKey			The Key we hope to find there.
**
** Outputs:
**	record				The matching Clustered Table row.
**      TrueTid                         The TID (BID) where the key was found.
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          Fixes and locks data page.
**
** History:
**      
**	06-Jun-2006
**	    Created for get-by-tid on Clustered Table.
*/
static DB_STATUS
VerifyHintTidAndKey(
    DMP_RCB     *rcb,
    DM_TID      *HintTid,
    char        *ClusterKey,
    char	*record,
    DM_TID      *TrueTid,
    DB_ERROR    *dberr)
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    ADF_CB		*adf_cb;
    DM_LINE_IDX         pos;
    DB_STATUS           s;
    DM_TID		localtid;
    char		*rec_ptr;
    i4			rec_len;
    i4                  compare;
    i4             	fix_action;
    i4			tidpartno;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if ( t->tcb_rel.relpgtype == TCB_PG_V1 || !(t->tcb_rel.relstat & TCB_CLUSTERED) )
	return (E_DB_OK);

    /*
    ** Working in-to-out, 
    **
    ** First check the leaf entry Key denoted by HintTid.
    **
    ** Failing that, search the same leaf for the Key.
    **
    ** Failing that, search the entire Btree.
    **
    ** Whatever path is taken, "TrueTid" will be returned,
    ** set to the real BID of the found Key.
    **
    ** Some clever feedback mechanism could someday repair
    ** the Hint TID in the secondary index with its corrected
    ** value, or track the number of mishits in the SI
    ** and suggest that it be repaired when it exceeds a certain
    ** threshold.
    */

    /* If wrong leaf fixed, unfix it, fix the right one */
    if ( r->rcb_other.page &&
	  HintTid->tid_tid.tid_page != 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_other.page) )
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, dberr);
	if ( s )
	    return(s);
    }

    if ( r->rcb_other.page == NULL )
    {
	fix_action = (r->rcb_access_mode == RCB_A_READ)
			? DM0P_READ : DM0P_WRITE;

	s = dm0p_fix_page(r, (DM_PAGENO)HintTid->tid_tid.tid_page, 
			    fix_action | DM0P_NOREADAHEAD, 
			    &r->rcb_other, dberr);

	if ( s != E_DB_OK )
	{
	    if (dberr->err_code > E_DM_INTERNAL)
		SETDBERR(dberr, 0, E_DM003C_BAD_TID);
	    return(s);
	}
    }

    /* If page is a leaf and the Hint line number looks ok, check the key: */
    if ( DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		r->rcb_other.page) & DMPP_LEAF &&
	    (i4)HintTid->tid_tid.tid_line <
		(i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
		r->rcb_other.page) )
    {
	adf_cb = r->rcb_adf_cb;

	/* Retrieve the row from this entry */
	rec_ptr = record;
	rec_len = t->tcb_klen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		    r->rcb_other.page,
		    &t->tcb_leaf_rac,
		    HintTid->tid_tid.tid_line,
		    &rec_ptr, TrueTid, (i4*)NULL, &rec_len,
		    (u_i4*)NULL, NULL, adf_cb);

	if ( s == E_DB_OK )
	{
	    /* ***temp */
	    if ( r->rcb_hl_given == 0 )
	    {
		TRdisplay("%@ dm1b_get with SI Clustered Hint %x, cannot compare key, assuming OK\n",
				HintTid->tid_i4);
		if ( rec_ptr != record )
		    MEcopy(rec_ptr, rec_len, record);
		return(E_DB_OK);
	    }

	    s = adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys, 
			  rec_ptr, ClusterKey, &compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
		return(s);
	    }
	    if ( compare == DM1B_SAME )
	    {
		/* Success, return the Cluster Table row */
		if ( rec_ptr != record )
		    MEcopy(rec_ptr, rec_len, record);
		return(E_DB_OK);
	    }
	}
    }

    /* Not a key match at the Hint. Search this leaf in its entirety */
    if ( DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		    r->rcb_other.page) & DMPP_LEAF )
    {
	s = dm1bxsearch(r, r->rcb_other.page, DM1C_TID, DM1C_EXACT, ClusterKey,
			t->tcb_keys, TrueTid, &tidpartno, &pos, record, dberr);

	if ( s == E_DB_OK || dberr->err_code != E_DM0056_NOPART )
	    return(s);
    }

    /* Btree search and get on exact match */
    return( btree_search(r, ClusterKey, t->tcb_keys, 
		    DM1C_FIND, DM1C_EXACT,
		    &r->rcb_other, TrueTid, 
		    &localtid, record, dberr) );
}

/*{
** Name: dm1b_AllocKeyBuf - Set up space to work with Leaf entries.
**
** Description:
**
**	With Clustered tables, the size of a Leaf entry can be relatively
**	large, up to DM1B_MAXLEAFLEN, as it includes both key and non-key
**	attributes, i.e., the entire row.
**
**	DM1B_MAXLEAFLEN  now defines the maximum size of a Leaf entry.
**	DM1B_MAXSTACKKEY defines the largest Leaf we can allocate on the stack.
**	DM1B_KEYLENGTH   defines the maximum length of just the key attributes.
**
**	Unfortunately, prior to Ingres2007, DM1B_KEYLENGTH really meant
**	"the maximum size of the Leaf" as it didn't distinguish key and
**	non-key attributes.
**
**	Many places need to assemble or fetch Leaf entries on to the stack.
**	Previously this space was defined with "char key[DM1B_KEYLENGTH]"
**	and several functions needed multiple instances (up to 5) of these arrays.
**
**	When DM1B_KEYLENGTH was 2000 bytes, this wasn't a problem and didn't
**	cause stack overflow. However, with larger Clustered Tables of up to
**	DM1B_MAXLEAFLEN bytes, the stack could be quickly blown.
**
**	So this function was invented to avoid stack overflow by allocating
**	memory when the size needed exceeds DM1B_MAXSTACKKEY.
**
** Inputs:
**	LeafLen				Number of Leaf bytes needed.
**	StackLeaf			Pointer to local stack variable
**					of DM1B_MAXSTACKKEY bytes, or NULL.
** Outputs:
**	LeafBuf				Where to return a pointer to the
**					properly sized area. This will be
**					either StackKey or AllocBuf.
**	AllocBuf			If Klen is greater than DM!B_MAXSTACKKEY
**					a pointer to the MEreqmem'd space
**					will be returned here, and the caller
**					MUST call back to dm1b_DeallocKeyBuf()
**					to free the memory to avoid leaks.
**	err_code			E_DM9425_MEMORY_ALLOCATE if
**					MEreqmem failed.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          If memory is allocated it must be explicitly freed.
**
** History:
**      
**	13-Jun-2006
**	    Created to allow Leaf entries larger than DM1B_KEYLENGTH.
*/
DB_STATUS
dm1b_AllocKeyBuf(
i4		LeafLen,
char		*StackLeaf,
char		**LeafBuf,
char		**AllocBuf,
DB_ERROR	*dberr)
{
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
    
    if ( StackLeaf && LeafLen <= DM1B_MAXSTACKKEY ) 
    {
	*AllocBuf = NULL;
	*LeafBuf = StackLeaf;
    }
    else if ( !(*AllocBuf = MEreqmem(0, LeafLen, FALSE, NULL)) )
    {
	uleFormat(dberr, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC*)NULL, 
		    ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, LeafLen);
	*LeafBuf = NULL;
    }
    else
	*LeafBuf = *AllocBuf;
    
    return( (*LeafBuf) ? E_DB_OK : E_DB_ERROR );
}

/*{
** Name: dm1b_DeallocKeyBuf - Free memory allocated by dm1b_AllocKeyBuf
**
** Description:
**
**	Tosses Leaf memory allocated by dm1b_AllocKeyBuf.
**
** Inputs:
**	AllocBuf			Pointer to pointer of alloc'd mem.
** Outputs:
**	AllocBuf			Is set to NULL.
**	LeafBuf				Optional pointer to pointer to NULLify.
**
** Returns:
**          VOID
**
** Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      
**	13-Jun-2006
**	    Created to allow Leaf entries larger than DM1B_KEYLENGTH.
*/
VOID
dm1b_DeallocKeyBuf(
char		**AllocBuf,
char		**LeafBuf)
{
    if ( AllocBuf && *AllocBuf )
    {
	MEfree(*AllocBuf);
	*AllocBuf = NULL;
	if ( LeafBuf )
	    *LeafBuf = NULL;
    }
    return;
}


/*{
** Name: findpage_reposition - Determines if reposition needed for findpage
**
** Description:
**      Make sure the correct leaf is fixed for findpage
**      If this btree can have overflow pages, if space for the new
**      key was allocated on an overflow page, the correct leaf for findpage
**      is the leaf at the head of the overflow chain.
**      If that leaf has split since we last locked the buffer,
**      follow next leaf pointers to the correct leaf.
**
** Inputs:
**      rcb                             rcb
**      leaf
**      key_ptr                               
**
** Outputs:
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          May return with a different leaf page fixed.
**
** History:
**      
**      10-sep-2009 (stial01)
**          Created.
*/
static DB_STATUS
findpage_reposition(
    DMP_RCB     *rcb,
    DMP_PINFO   *leafPinfo,
    char        *key_ptr,
    DB_ERROR    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DM_PAGENO		pageno;
    DB_STATUS		s;
    i4			compare;
    i4			fix_action;
    bool                correct_leaf = FALSE;
    i4			*err_code = &dberr->err_code;
    DMPP_PAGE		**leaf;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    /*
    ** Reposition may be needed even if this is TCB_PG_V1 and page locking
    ** because this routine can get called from isolate_dup
    **
    ** if tcb_dups_on_ovfl and alloc_bid on an overflow page
    ** - the findpage leaf is the leaf at the head of the overflow chain
    */
    for (;;)
    {
	dmf_svcb->svcb_stat.btree_repos_chk_range++;
	s = dm1b_compare_key(r, *leaf, DM1B_LRANGE, key_ptr,
		t->tcb_keys, &compare, dberr);
	if (s != E_DB_OK)
	    break;
	if (compare < DM1B_SAME)
	{
	    s = E_DB_ERROR;
	    break;
	}

	s = dm1b_compare_range(r, *leaf, key_ptr, &correct_leaf, 
		dberr);
	if (s != E_DB_OK)
	    break;

	/* If the key still within this key range, we have correct leaf */
	if (correct_leaf == TRUE)
	    break;
	/* Go to the next leaf */
	pageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

	if (pageno == 0)
	{
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    s = E_DB_ERROR;
	    break;
	}

	dm0pUnlockBuf(r, leafPinfo);
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	fix_action = DM0P_WRITE;
	dmf_svcb->svcb_stat.btree_repos++;

	s = dm0p_fix_page(r, pageno, fix_action, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	/*
	** Lock buffer for update.
	**
	** This will swap from CR->Root if "leaf" is a CR page.
	*/
	dm0pLockBufForUpd(r, leafPinfo);

	continue;
    }

    /* findpage_reposition should never fail */
    if (s != E_DB_OK )
    {
	if ( leafPinfo->page )
	{
	    dm0pUnlockBuf(r, leafPinfo);
	    dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	}
	SETDBERR(dberr, 0, E_DM93F6_BAD_REPOSITION);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}


/*{
** Name: isolate_dup - Before adding overflow chain, split such that
**          LRANGE is decremented dupkey and
**          RRANGE is incremented dupkey
**  
** Description:
**
** Inputs:
**      rcb                             rcb
**      leaf
**      dupkey
**
** Outputs:
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**          May return with a different leaf page fixed.
**
** History:
**      
**      04-dec-2009 (stial01)
**          Created.
*/
static DB_STATUS
isolate_dup(
DMP_RCB		*rcb,
DMP_PINFO	*leafPinfo,
char		*dupkey,
DB_ERROR	*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    i4	    	    state = 0;
    i4	    	    child;
    i4   	    compare;
    i4	    	    key_len;
    char	    *KeyPtr, *AllocKbuf = NULL;
    char	    *key_ptr;
    char	    *decr_key, *incr_key;
    char	    key_buffer[DM1B_MAXSTACKKEY];
    DB_STATUS	    s = E_DB_OK;
    i4		    *err_code = &dberr->err_code;
    i4		    lr_split; /* splits to fix LRANGE */
    i4		    rr_split; /* splits to fix RRANGE */
    DM_TID	    localbid;
    DM_TID	    localtid;
    DM_PAGENO	    savepage;
    i4		    fix_action;
    DMPP_PAGE	    **leaf;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    if (!t->tcb_dups_on_ovfl)
    {
	SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	return (E_DB_ERROR);
    }

    if ( s = dm1b_AllocKeyBuf(t->tcb_klen, key_buffer,
				&KeyPtr, &AllocKbuf, dberr) )
	return(s);

    /* save dupkey leaf page number */
    savepage = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

    /*
    ** Decrement and get the next lowest key value, this must be LRANGE
    ** if LRANGE < decr_key, split with splitkey=decr_key
    */
    for (lr_split = 0; ; lr_split++)
    {
	/* Make a copy of dupkey */
	decr_key = KeyPtr;
	MEcopy(dupkey, t->tcb_klen, decr_key);

	/* Decrement and get the next lowest key value, this must be LRANGE */
	s = adt_key_decr(adf_cb, t->tcb_keys, t->tcb_leafkeys, t->tcb_klen, decr_key);
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    break;
	}

	/* Compare dupkey, decr_key */
	s = adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys, dupkey, decr_key, &compare);
	if (s != E_DB_OK || compare <= DM1B_SAME)
	    break;  /* dupkey not incremented */
	
	/* Compare LRANGE, decr_key */
	s = dm1b_compare_key(r, *leaf, DM1B_LRANGE, decr_key, t->tcb_keys, 
		    &compare, dberr);
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    break;
	}

	/* dont split if decr_key <= LRANGE) */
	if (compare <= DM1B_SAME)
	    break;

	if (lr_split || DMZ_AM_MACRO(18))
	{
	    TRdisplay("isolate_dupkey need to split %d to fix LRANGE \n",
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));
	    dmdprbrange(r, *leaf);
	    TRdisplay("    isolate_dupkey dup_key: \n");
	    dmd_prkey(r, dupkey, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	    TRdisplay("    isolate_dupkey decr_key: \n");
	    dmd_prkey(r, decr_key, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	}

 	if (lr_split)
	{
	   /* Previous lr_split failed */
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    s = E_DB_ERROR;
	    break;
	}

	/* split with splitkey=decr_key */
	s = btree_search(r, decr_key, t->tcb_keys, DM1C_SPLIT_DUPS, DM1C_EXACT, 
		leafPinfo, &localbid, &localtid, NULL, dberr); 
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    s = E_DB_ERROR;
	    break;
	}

	/* Reposition to correct leaf for dupkey */
	s = findpage_reposition(r, leafPinfo, dupkey, dberr);
	if (s != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    s = E_DB_ERROR;
	    break;
	}
    }

    /*
    ** if RRANGE != dup_key, split with splitkey=incr_key
    */
    for (rr_split = 0; s == E_DB_OK; rr_split++)
    {
	/* Compare RRANGE, dup_key */
	s = dm1b_compare_key(r, *leaf, DM1B_RRANGE, dupkey, t->tcb_keys, 
		    &compare, dberr);
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    break;
	}

	if (compare == DM1B_SAME)
	    break;

 	if (rr_split)
	{
	   /* Previous rr_split failed */
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    s = E_DB_ERROR;
	    break;
	}

	/* Make a copy of dupkey */
	incr_key = KeyPtr;
	MEcopy(dupkey, t->tcb_klen, incr_key);

	/* Increment and get the next highest key value */
	s = adt_key_incr(adf_cb, t->tcb_keys, t->tcb_leafkeys, t->tcb_klen, 
		incr_key);
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    break;
	}

	if (DMZ_AM_MACRO(18))
	{
	    TRdisplay("isolate_dupkey need to split %d to fix RRANGE \n",
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf));
	    dmdprbrange(r, *leaf);
	    TRdisplay("    isolate_dupkey dup_key: \n");
	    dmd_prkey(r, dupkey, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	    TRdisplay("    isolate_dupkey incr_key: \n");
	    dmd_prkey(r, incr_key, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
	}

	/* Compare dupkey, incr_key */
	s = adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys, dupkey, incr_key, &compare);
	if (s != E_DB_OK || compare >= DM1B_SAME)
	    break;  /* dupkey not incremented */

	/* Compare RRANGE, incr_key */
	s = dm1b_compare_key(r, *leaf, DM1B_RRANGE, incr_key, t->tcb_keys, 
		    &compare, dberr);
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    break;
	}

	/* dont split if incr_key >= RRANGE) */
	if (compare >= DM1B_SAME)
	    break;
	
	/* split with splitkey=incr_key */
	s = btree_search(r, incr_key, t->tcb_keys, DM1C_SPLIT_DUPS, DM1C_EXACT, 
		leafPinfo, &localbid, &localtid, NULL, dberr); 
	if (s)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    s = E_DB_ERROR;
	    break;
	}

	/* Reposition to correct leaf */
	if (savepage != DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf))
	{
	    /* split with splitkey=incr_key returned with next leaf fixed */
	    dm0pUnlockBuf(r, leafPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	    if (s != E_DB_OK)
		break;

	    fix_action = DM0P_WRITE;

	    s = dm0p_fix_page(r, savepage, fix_action, leafPinfo, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if "leaf" is a CR page.
	    */
	    dm0pLockBufForUpd(r, leafPinfo);
	}

	/*
	** Reposition to correct leaf for dupkey 
	** findpage_reposition can be called for any table with
	** tcb_dups_on_ovfl (including TCB_PG_V1)
	*/
	s = findpage_reposition(r, leafPinfo, dupkey, dberr);
	if (s != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
	    s = E_DB_ERROR;
	    break;
	}

	break;
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);

    if (s == E_DB_OK)
	return (E_DB_OK);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C17_DM1B_NEWDUP);
    }

    return (E_DB_ERROR);
}


/*{
** Name: dm1b_dbg_incr_incr_ - debug routine for testing increment/decrement
**  
** Description:
**
** Note:
**	Increment/decrement will work differently if the key is nullable
**	Min/max varies for character datatypes 
** 
** Below is an example of how to verify increment, decrement.
** The i4 test below  specifies values that can/cant be incremented/decremented
** 
** 	set trace point dm618;
** 	drop table test_i4;
** 	create table test_i4 (b i4 ); 
** 	modify test_i4 to btree on b ;
** 	insert into test_i4 values ( 44);
** 	insert into test_i4 values ( -2147483648);
** 	insert into test_i4 values ( 2147483647);
**      insert into tset_i4 values (null);
**
** Note:
** - increment/decrement will work differently if the key is nullable
** - min/max varies for character datatypes,for example char,varchar
**   allow 0x00, text, c do not.
** 
** Inputs:
**      rcb                             rcb
**      key
**
** Outputs:
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**
** History:
**      
**      04-dec-2009 (stial01)
**          Created.
*/
static DB_STATUS
dm1b_dbg_incr_decr(
DMP_RCB		*rcb,
char		*key)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    ADF_CB	    *adf_cb = r->rcb_adf_cb;
    i4	    	    key_len;
    char	    *KeyPtr, *AllocKbuf = NULL;
    char	    *key_ptr;
    char	    *dbg_key;
    char	    key_buffer[DM1B_MAXSTACKKEY];
    DB_STATUS	    s = E_DB_OK;
    i4		    local_error;
    DB_ERROR	    local_dberr;
    i4		    compare;

    CLRDBERR(&local_dberr);

    if ( s = dm1b_AllocKeyBuf(t->tcb_klen, key_buffer,
				&KeyPtr, &AllocKbuf, &local_dberr) )
	return(s);

    dbg_key = KeyPtr;

    TRdisplay("    ORIG-Key: %~t \n",
	sizeof(t->tcb_rel.relid), t->tcb_rel.relid.db_tab_name);
    dmd_prkey(r, key, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);

    MEcopy(key, t->tcb_klen, dbg_key);
    s = adt_key_incr(adf_cb, t->tcb_keys, t->tcb_leafkeys, t->tcb_klen, dbg_key);

    /* Compare key, incr_key */
    s = adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys, key, dbg_key, &compare);
    if (s != E_DB_OK)
    {
	uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }

    TRdisplay("    INCR-Key: %~t %s \n",
	sizeof(t->tcb_rel.relid), t->tcb_rel.relid.db_tab_name, 
	compare == DM1B_SAME ? "SAME" : compare < DM1B_SAME ? "incremented ok" : "ERROR key was decremented");
    dmd_prkey(r, dbg_key, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);

    /* get a new copy of key */
    MEcopy(key, t->tcb_klen, dbg_key);
    s = adt_key_decr(adf_cb, t->tcb_keys, t->tcb_leafkeys, t->tcb_klen, dbg_key);

    /* Compare key, decr_key */
    s = adt_kkcmp(adf_cb, t->tcb_keys, t->tcb_leafkeys, key, dbg_key, &compare);
    if (s != E_DB_OK)
    {
	uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }

    TRdisplay("    DECR-Key: %~t %s \n",
	sizeof(t->tcb_rel.relid), t->tcb_rel.relid.db_tab_name, 
	compare == DM1B_SAME ? "SAME" : compare > DM1B_SAME ? "decremented ok" : "ERROR key was incremented");
    dmd_prkey(r, dbg_key, DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);

    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyPtr);
    return (E_DB_OK);
}


/*{
** Name: dm1b_position_save_fcn - save btree position/reposition information
**  
** Description: Save information for btree reposition
** This includes the bid, tidp and some information from the leaf page 
** If saving position for get operation, save the leaf key also.
**
** This code has been consolidated to a routine with additional 
** consistenty checking and trace output.
**
** Inputs:
**      rcb                             rcb
**      key
**
** Outputs:
**      err_code                        Pointer to return error code in 
**                                      case of failure.
**
** Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** Exceptions:
**          none
**
** Side Effects:
**
** History:
**      
**      11-jan-2010 (stial01)
**          Created.
*/
VOID
dm1b_position_save_fcn(
DMP_RCB		*r,
DMPP_PAGE	*b,
DM_TID		*bidp,
i4		pop,
i4		line)
{									
    DMP_TCB	*t = r->rcb_tcb_ptr;
    DMP_POS_INFO *pos = &r->rcb_pos_info[pop];
    char	*tup_hdr;

    pos->bid.tid_i4 = (bidp)->tid_i4;			
    DM1B_VPT_GET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype, b, pos->lsn);					
    pos->clean_count =DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(t->tcb_rel.relpgtype,b);
    pos->nextleaf = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, b);
    pos->page_stat = DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, b);
    pos->valid = TRUE;					
    pos->line = line;				
    pos->page = b;				
    pos->tran = DM1B_VPT_GET_PAGE_TRAN_ID_MACRO(t->tcb_rel.relpgtype, b);
    pos->row_low_tran = 0;
    pos->row_lg_id = 0;
    if ((bidp)->tid_tid.tid_line != DM_TIDEOF)			
    {									
	DB_STATUS	entry_status;

	dm1cxtget(t->tcb_rel.relpgtype,			
	    t->tcb_rel.relpgsize, b, (bidp)->tid_tid.tid_line, 
	    &pos->tidp, (i4*)NULL);				

	/* save the transaction information for the entry */
	entry_status = dm1cx_txn_get(t->tcb_rel.relpgtype, b, 
		(i4)bidp->tid_tid.tid_line, &pos->row_low_tran, &pos->row_lg_id);

	/*
	** Save key if RCB_P_GET
	**
	** Other operations maintain position key differently:
	** RCB_P_ALLOC Uses the key provided to dm1b_allocate, dm1b_put
	** RCB_P_FETCH Copies the the key from caller in dm1b_search
	**             Otherwise copies from the RCP_P_GET rcb_repos_key.
        **             using DM1B_POSITION_SET_FETCH_FROM_GET_MACRO.
	** RCB_P_START Uses original position parameters
	*/
	if (pop == RCB_P_GET)			
	{
	    /*
	    ** Save the key,tidp,lsn and clean count for reposition
	    ** to the last LOCKED record. 
	    **
	    ** If CLUSTERED, extract and save just the keys from the
	    ** returned tuple, in "natural" format.
	    ** (although it would be ok to just copy t->tcb_klen bytes)
	    */
	    if ( t->tcb_rel.relstat & TCB_CLUSTERED )
	    {
		i4	k;
		DB_ATTS	*att;
		for ( k = 0; k < t->tcb_keys; k++ )
		{
		    att = t->tcb_leafkeys[k];
		    MEcopy(r->rcb_record_ptr + att->offset, att->length, 
			   r->rcb_repos_key_ptr + att->offset);
		}
	    }
	    else
		MEcopy((t->tcb_rel.relstat & TCB_INDEX) ? r->rcb_record_ptr :
		    r->rcb_s_key_ptr, t->tcb_klen, r->rcb_repos_key_ptr);

	    /*
	    ** Consistency check for RCB_P_GET
	    **
	    ** The leaf should be a CR-leaf
	    ** or the position should be the top of the leaf (bottome for prev)
	    ** or we should have a row lock for the row specified by the key
	    */
	    if (DMZ_AM_MACRO(18)) /* for now use dm618 */
	    {
	        if (DMPP_VPT_IS_CR_PAGE(t->tcb_rel.relpgtype, b) ||
			bidp->tid_tid.tid_line == DM_TIDEOF)
		{
		    /* save position on cr page or top of leaf */
		}
		else if (crow_locking(r) && b == r->rcb_other.page &&
			RootPageIsInconsistent(r, &r->rcb_other ) == FALSE)
		{
		    /* also ok so long as we don't position on deleted row */
		}
		else if (row_locking(r) || crow_locking(r))
		{
		    LK_LOCK_KEY         lock_key;
		    LK_LKB_INFO         info_buf;
		    CL_ERR_DESC         sys_err;
		    u_i4		info_result;
		    STATUS              s;
		    i4			flag;
		    i4			reltidx;

		    reltidx = t->tcb_rel.reltid.db_tab_index;
		    flag = LK_S_OWNER_GRANT;
		    if (reltidx > 0)
			reltidx = 0;		/* Base table, not index */
		    lock_key.lk_type = LK_ROW;
		    lock_key.lk_key1 = t->tcb_dcb_ptr->dcb_id;
		    lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
		    lock_key.lk_key3 = reltidx;
		    lock_key.lk_key4 = pos->tidp.tid_tid.tid_page;
		    lock_key.lk_key5 = pos->tidp.tid_tid.tid_line;
		    lock_key.lk_key6 = 0;
		    /* Length of zero returned if no matching key */
		    s = LKshow(flag, r->rcb_lk_id, (LK_LKID *)NULL, &lock_key, 
				sizeof(info_buf), (PTR)&info_buf, &info_result, 
				(u_i4 *)NULL, &sys_err);

		    if (s != OK || (info_result == 0))
		        TRdisplay("position save GET (%d,%d) without lock\n",
			lock_key.lk_key4, lock_key.lk_key5);
	        }
	    }
		
	}
    }

    /* For now use dm618 */
    if (DMZ_AM_MACRO(18))
    {
	TRdisplay("DM1B-SAVEPOS (%d,%d) %~t %p source-line %d\n"
	    "    pop %w (%d) tran %x \n"
	    "    POS Bid:(%d,%d) Tid:(%d,%d) LSN=(%x,%x)\n"
	    "    POS cc %d nextleaf %d page_stat %x %v\n"
	    "    POS page 0x%p tran %x\n",
	    t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
	    t->tcb_relid_len, t->tcb_rel.relid.db_tab_name, r, line,
	    "START,GET,FETCH,ALLOC,TEMP",pop,
	    pop, r->rcb_tran_id.db_low_tran,
	    pos->bid.tid_tid.tid_page, pos->bid.tid_tid.tid_line,
	    pos->tidp.tid_tid.tid_page, pos->tidp.tid_tid.tid_line,
	    pos->lsn.lsn_low, pos->lsn.lsn_high,
	    pos->clean_count, pos->nextleaf,
	    pos->page_stat, PAGE_STAT, pos->page_stat,
	    pos->page, pos->tran.db_low_tran);
    }
}
