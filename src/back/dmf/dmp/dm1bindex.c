/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dm0p.h>
#include    <dm0l.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1r.h>
#include    <dm1p.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dm2f.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dml.h>

/*
**  NO_OPTIM = dr6_us5 nc4_us5 i64_aix
*/

/**
**
**  Name: DM1BINDEX.C - Routines needed to manipulate BTREE indices.
**
**  Description:
**      This file contains all the routines needed to manipulate BTREE
**      index and leaf pages.   Index and leaf pages of BTREEs have
**      records which  consist of a key and tid.  In the case of a
**      leaf page, the tid identifies a tuple.  In the case of an 
**      index page, the tid associated with a key  identifies a page 
**      which contains records with keys less then or equal to this key.
**      Note that in the last entry of an index page only the tid is
**      significant since the key can be considered the infinite key.
**
**      This can be seen as follows:
**
**                          index page 0
**                        +-------------------+
**                        | header info       |
**                        |-------------------|
**                        | line table        |
**                        |-------------------|
**                        |r1 key = "DEF"     |-----------------+
**                        |   tid = pg 6,ln 0 |                 |
**                        |-------------------|                 |
**                +-------|r0 key = "ABC"     |                 |
**                |       |   tid = pg 5,ln 0 |                 |
**                |       +-------------------+                 |
**                |       |rn key = <infinite>|                 |
**                |       |   tid = pg 7,ln 0 |                 |
**                v       +-------------------+                 v
**        leaf page 5                                    leaf page 6        
**      +-------------------+                          +-------------------+
**      | header info       |                          | header info       |
**      |-------------------|                          |-------------------|
**      | line table        |                          | line table        |
**      |-------------------|                          |-------------------|
**      |r0 key = "AAA"     |                          |r1 key = "DEF"     |
**      |   tid = pg 9,ln 12|                          |   tid = pg 10,ln 4|
**      |-------------------|                          |-------------------|
**      |r1 key = "ABC"     |                          |r0 key = "DEF"     |
**      |   tid = pg 9,ln 2 |                          |   tid = pg 11,ln 1|
**      +-------------------+                          +-------------------+
**
**	For primary relations, the leaf record tid points to a tuple 
**      within the same file.	For secondary indexes, it points to a
**      tuple in the associated	primary relation. 
**
**	Each page contains a line table (referred to as bt_sequence).
**
**	Note that entries are stored on the page in random order; however, the
**	line table (sequence) offsets for the entries are kept in correct
**	sorted order.
**
**	For Btrees which do not perform entry compression, the line table
**	contains a set of pointers that are fixed at the time the page is
**	formatted, but are shuffled about on insertions and deletions. An
**	initial segment of this array of pointers represents (key, tid)
**	addresses that are currently in use, in the order of key value.  The
**	length of this segment is recorded in bt_kids.
**
**	For Btrees which perform entry compression, the line table entries and
**	offsets are NOT fixed at format time; rather they must be calculated
**	at space allocation time, just as is done for compressed data pages.
**
**	The first two slots of the line table
**      are reserved for the high and low range of keys on the page.
**
**	The major source of complication in the following routines
**	is that duplicate keys may overflow into overflow pages at 
**	the leaf level. In this case, a leaf and a chain of overflow
**	pages are monopolized by a single duplicate key.
**	It is the transformation of a regular leaf page, to "duplicate"
**	leaf page that leads to long-winded code.
**
**	Note also that each leaf page contains a low key value and a high
**	key value which define the range of the keys that may possibly
**	fall into that page. These are used in moving to the "next" higher
**	leaf page in a btree. They occupy the first 2 positions
**	of the line table.
**
**      The routines contained in this file are:
**
**      dm1bxchain	- Follows an overflow page to get the
**			  next key,tid pair.
**      dm1bxcompare    - Compares two keys.
**      dm1bxdelete	- Deletes a key,tid pair from a page.
**      dm1bxdupkey	- Find the key that is duplicated for an
**                        overflow chain.
**      dm1bxformat	- Formats an empty page.
**      dm1bxinsert	- Inserts a key,tid pair onto a page.
**      dm1bxjoin       - Joins an index or overflow page for 
**                        modify to merge.
**      dm1bxnewroot    - Creates a new root.
**      dm1bxsearch	- Searches for a key or key,tid pair
**                        on a page.
**      dm1bxsplit	- Split a page into two pages.
**
**
**  History:
**      06-oct-85 (jennifer)
**          Changed for Jupiter.
**      02_apr_87 (ac)
**          Added readlock = nolock support.
**	23-feb-89 (mmm)
**	    bug fix for bug number 4780.  Data appended to a btree secondary
**	    index using a fast commit server would sometimes be lost following
**	    REDO recovery (ie. server crash prior to data being forced to disk).
**
**	    dm1bxformat(), did not initialize the page_log_addr field in
**	    btree secondary index pages, thus it was set to whatever value 
**	    happened to be in memory.  This would cause redo recovery to be 
**	    incorrect in some cases.  
**
**	    If a server crash occurs between the time the last page added to a 
**	    btree secondary index is forced to disk with the unitialized log 
**	    address, and when the page is written with new data "put" on it 
**	    (either by a consistency point or the buffer manager forcing 
**	    it to disk) then REDO recovery uses this field to determine if it 
**	    needs to redo puts to the secondary index.  In some cases on unix 
**	    the value that happened to be at that address was a log address 
**	    greater than the current, so REDO recovery thought it had nothing 
**	    to do.  To fix this the field is initialized to zero's which is 
**	    guaranteed smaller than any legal log address.
**
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-1990 (bryanp)
**	    Bug 34011 -- split processing sometimes corrupts btrees. We must
**	    re-mark the page as DMPP_MODIFY when we shift keys onto it in
**	    dm1bxrshift.
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	18-dec-90 (rogerk)
**	    Fixed two btree space allocation bugs.  In dm1bxdupkey, make sure
**	    to leave proper page fixed upon return.  In lfdiv, don't bypass
**	    work if asked to split a leaf with only 1 entry.
**	14-jan-1991 (bryanp)
**	    Support for Btree index compression. Re-worked index page access
**	    into a layer of dm1cx* routines and re-wrote this file to use those
**	    routines. The management of fixed versus variable entry sizes is
**	    largely contained in the dm1cx* routines. One entire
**	    routine (dm1bxrshift()) was moved wholesale out of this file and
**	    into dm1cidx.c
**	14-jun-1991 (Derek)
**	    Minor changes to support new allocation stuff and a few minor
**	    cleanups.
**	8-jul-1991 (bryanp)
**	    During a leaf page split in a Btree primary table, release the
**	    sibling page mutex while logging the data page allocation.
**	8-jul-1991 (bryanp)
**	    B38405: Don't log DMF external errors during split processing.
**	10-sep-1991 (bryanp)
**	    B37642,B39322: Check for infinity == FALSE on LRANGE/RRANGE entries.
**	24-oct-1991 (jnash)
**           Add return statement at end of dm1bxformat to satisfy lint.
**	25-oct-1991 (bryanp)
**	    REDO recovery problems. We must call dm1p_force_map
**	    to force the FMAP page for the newly allocated overflow page before
**	    ending the mini-transaction.
**	 3-nov-1991 (rogerk)
**	    Moved dm1p_force_map calls in dm1bxsplit together down near end of
**	    mini transaction to attempt to only force the map page once rather
**	    than twice.
**	20-Nov-1991 (rmuth)
**	    Added DM1P_MINI_TRAN, when inside a mini-transaction pass this
**	    flag to dm1p_getfree() so it does not allocate a page deallocated
**	    earlier by this transaction. see dm1p.c for explaination why.
**	28-apr-1992 (bryanp)
**	    B43636: Handle splitting a page with 0 entries properly.
**	07-jul-92 (jrb)
**	    Prototype DMF.
**	16-jul-1992 (kwatts)
**	    MPF project: interface change, dm1cxget/dm1cxformat/dm1cxcomp_rec
**	    now require a TCB parameter.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: 
**	    - Changed arguments to dm0p_mutex and dm0p_unmutex calls.
**	    - Move dm1b.h to be included before dm0l.h
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	      - Changed to not follow the leaf's overflow chain in search of 
**		matching rows in DM1C_TID mode.  The callers will now be 
**		required to call this routine again with the overflow pages 
**		to continue searching.
**	      - Changed the buffer pointer to be passed by value since it is 
**		no longer changed.
**	      - Changed dm1cxformat to not require TCB or RCB.
**	02-jan-1993 (jnash)
**	    More reduced logging work.  Add LGreserve calls.
**	13-jan-1993 (mikem)
**	    Lint directed changes.  Fixed "=" vs. "==" problem in dm1bxsplit().
**	    Previous to this change errors in dm1cxrshift() were ignored.
**	16-feb-1993 (jnash)
**	    Move LGreserve prior to dm0l_bi() to fix mutexing problem.
**	18-feb-1993 (walt)
**	    Defined a i4 'n' in dm1bxnewroot for its CSterminate call.  
**	    (The call was possibly cut and pasted from dm1bxsplit which has 
**	    an 'n' defined for it.)
**	30-feb-1993 (rmuth)
**	    Fix mutexing problem in dm1bxjoin.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-apr-1993 (jnash)
**	    In dm1bxinsert() and dm1bxdelete(), LGreserve() space for only for 
**	    the keylen used.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (rogerk)
**	    Fixed some error handling and added error messages where they
**	    had previously been left out.
**	21-jun-1993 (rogerk)
**	    Fix call to dm1b_rcb_update in dm1bxsplit.
**	    Removed key copy from btput CLRs.  Take this into account when
**	    reserving space for the btput log record.
**	    Added trace point to bypass dm1bxfree_ovfl processing.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	  - Added dm1b_rcb_update call to dm1bxfree_ovfl to check for other 
**	    RCB's which are positioned on the page being deleting.
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Fix problems with splits of empty leaf pages which have non-empty
**	    overflow chains.  The routine was assuming that a split_pos of
**	    zero implied that all duplicate rows were being moved to the
**	    sibling page.  But this did not turn out to be the case if we
**	    were splitting right but the leaf had no children (in which case
**	    the split pos was set to bt_kids, but that value was also zero).
**	    Added split direction field to the split log record.
**	  - Fixed bug in logging of index delete operations (during joins).
**	    When deleting from a non-leaf page, the key deleted is actually
**	    the key in entry (child - 1).  So we need to log this key value,
**	    not the one in the child entry.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	28-mar-1994 (jnash)
**	    During index block splits, release lock on unfix of original
**	    index page if not leaf split.  Leaving the index locked was 
**	    causing deadlocks during during UNDO and pass-abort recovery.
**	9-may-94 (robf)
**          Add check for adt_kkcmp() failure. This routine could always
**	    return a failure status, but is more likely in a secure system
**	    when security labels are being processed. We check to prevent
**	    problems like redefinition of security labels being silently
**	    ignored.
**     19-apr-95 (stial01)
**          Modified dm1bxsearch() to support finding last entry on page for
**          MANMANX
**	30-aug-1995 (cohmi01)
**	    Put back partial support for DM1C_NEXT for use by DMR_AGGREGATE.
**	06-mar-1996 (stial01 for bryanp) (bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	    When reserving space for DM0L_BI and DM0L_AI records, allow for
**		variable sized pages.
**	    Pass page_size argument to dm0l_bi, dm0l_ai.
**	    Pass page_size argument to the dm1cx* accessor functions.
**	    Use tbio_page_size, not DM_PG_SIZE, to compute table's page size.
**	    Add page_size argument to dm1bxformat, since the tcb passed to
**		dm1bxformat may not have the correct page size in it
**		(particularly during an index build, when the tcb passed to
**		dm1bxformat is the *base table* tcb).
**      01-jan-1996 (stial01)
**          Add page_size argument to dm1bxinsert()
**          When called from dm1bbput, we are inserting to a secondary index,
**          using the tcb from the primary table. We need to have the correct
**          page size.
**      01-may-1996 (stial01)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          When updating the leaf page set DM0L_ROW_LOCK flag if row locking
**          dm1bxdelete() Pass reclaim_space to dm1cxdel()
**          dm1bxnewroot() Pass reclaim_space = TRUE to dm1cxdel()
**          dm1bxovfl_alloc: If row locking, alloc overflow pg in mini trans
**          Added dm1bxclean()
**          Check for E_DB_WARNING return code from dm1cxget()
**      17-dec-96 (stial01)
**          dm1bxclean() init tup_addr, tup_hdr which inexplicably got dropped
**      06-jan-97 (dilma04)
**          Removed unneeded DM1P_PHYSICAL flag when calling dm1p_getfree()
**          from dm1bxovfl_alloc().
**	10-jan-1997 (nanpr01)
**	    Added back the spaceholder for page in DM0L_BI, DM0L_AI and
**	    DM0L_BTSPLIT structure.
**	21-jan-1997 (i4jo01)
**	    Recognize overflow leaf pages when doing overflow joins, so
**	    that they are not treated as index pages (b80089).
**      24-jan-97 (dilma04)
**          Fix bug 80267: Do not set DM0L_K_ROW_LOCK flag if page size is 2k.
**      23-jan-1997 (stial01)
**          dm1bxsplit() when row locking, request locks for new sibling
**          page for all lock lists holding intent lock on page being split. 
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-1997 (stial01)
**          dm1bxdelete() reclaim_space was changed from input to output param.
**          Don't reclaim space if deleting last key on leaf with overflow chain
**          dm1bxinsert() LGreserve space for key in CLR if row locking
**          dm1bxdupkey(), get_dup_key() Fatal error if we look for duplicate
**          key on overflow pages for dm1b_v2_index pages.
**          Changes to dm1bxclean: Test DMPP_CLEAN in page status to 
**          avoid the overhead of checking leaf entries unecessarily.
**          Prepare list of committed transactions instead of bitmap of
**          committed entries. When LK_PH_PAGE locks are replaced with short
**          duration mutex, we release mutex while calling LG for transaction
**          status, during which time the entries on the leaf page can shift.
**      07-apr-97 (stial01)
**          dm1bxchain() NonUnique primary btree (V2), dups can span leaf
**          dm1bxdelete() Removed code to keep overflow key on leaf,
**              This is not needed anymore.
**          dm1bxdupkey() Overflow key value processing, return immediately if
**              NonUnique primary btree (V2) index.
**          dm1bxovfl_alloc() NonUnique primary btree (V2) dups span leaf pages,
**          so we don't have to worry about row locking in overflow page code.
**          division() NonUnique primary btree (V2) dups span leaf pages,
**          so we don't have to adjust split position to keep dups together
**          To reduce fan out for NonUnique primary btree (V2), when
**          splitting a leaf page and LRANGE == RRANGE, divide at pos 1
**          get_dup_key() no longer called for Non unique primary btree (V2)
**          dm1bxclean() Removed code to keep overflow key on leaf,
**              This is not needed anymore.
**      21-apr-97 (stial01)
**          dm1bxinsert(): If row locking and bid reserved, don't validate
**          insert request or allocate space.
**          dm1bxsplit(): If row locking, reserve space for new row
**          dm1bxclean() delete leaf entries when page locking -> rcb_update
**              Do not clean the page if r->rcb_k_duration == RCB_K_READ
**          Added dm1bxreserve()
**      29-apr-1997 (stial01)
**          dm1bxsplit() Get dup key value if split pos 0 and 2k page size
**	09-May-1997 (jenjo02)
**	    Added rcb_lk_id to LKpropagate() call.
**      21-may-97 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**          dm1bxchain() New buf_locked parameter.
**          dm1bxsplit() RCB update only if leaf split. If row locking and
**          new record belongs on sibpage, unlock curpage buffer, lock sibpage
**          buffer. Return E_DB_ERROR if error.
**          dm1bxclean() Must be called with X page lock or BUF_MLOCK on buffer
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
**          Pass new attribute, key parameters to dm1bxformat
**      17-jun-97 (stial01)     
**          dm1bxinsert(), dm1bxdelete(): Set DM0L_BT_UNIQUE for unique indexes
**      18-jun-97 (stial01)
**          Do not set DM0L_ROW_LOCK when updating index pages
**      30-jun-97 (stial01)     
**          dm1bxfree_ovfl(): Set DM0L_BT_UNIQUE for unique indexes
**      29-jul-97 (stial01)
**          dm1bxdelete() Space reclamation for leaf entries dependent on 
**          page size not lockmode, validate child position
**          dm1bxinsert() Removed some unecessary reposition logic, validate
**          child position
**          dm1bxsplit() Changed args to dm1b_rcb_update
**          dm1bxclean(),dm1bxreserve() Call dm1b_rcb_update.
**	24-jun-1997 (walro03)
**	    Unoptimize for ICL DRS 6000 (dr6_us5).  Compiler error: expression
**	    causes compiler loop: try simplifying.
**      13-jan-1997 (stial01)
**          Set DMPP_CLEAN on sibpage if it was set on split page (B88076)
**      15-jan-98 (stial01)
**          dm1bxclean() Fixed dm1cxclean parameters, error handling
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**      07-jul-98 (stial01)
**          Deferred update: new update_mode if row locking
**      26-aug-98 (stial01)
**          dm1bxsearch() if DM1C_TID/DM1C_EXACT and tidp is special reserved 
**          tid, check tranid as well.
**          dm1bxinsert() Added more tidp,tranid validity checks for reserved
**          leaf entries.
**          Fixed LKpropagate error handling.
**      15-oct-98 (matbe01)
**          Added NO_OPTIM for NCR (nc4_us5) to fix error E_QE007E.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-99 (stial01)
**          dm1bxsplit() Log kperpage
**      12-apr-1999 (stial01)
**          Btree secondary index: non-key fields only on leaf pages
**          Different att/key info for LEAF vs. INDEX pages
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      15-nov-1999 (stial01)
**          Fixed args to dm1cxget_deleted, messed up integrating 12-nov changes
**          to head revision.
**	21-nov-1999 (nanpr01)
**	    Code optimization/reorganization based on quantify profiler output.
**      21-dec-1999 (stial01)
**          dm1bxdelete,dm1bxinsert set DM1C_ROWLK for etabs > 2k
**          dm1bxclean() For etabs check if delete committed
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_btput, dm0l_btdel.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026) 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Types SIR 102955)
**	    dm1bxclean() Move bulk of code to re-written dm1cxclean
**      04-may-2001 (stial01)
**          Increment leaf clean count if not logging (B103111)
**      10-may-2001 (stial01)
**          Added rcb_*_nextleaf to optimize btree reposition, btree statistics
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      28-mar-2003 (stial01)
**          Child on DMPP_INDEX page may be > 512, so it must be logged
**          separately in DM0L_BTPUT, DM0L_BTDEL records (b109936)
**      28-may-2003 (stial01)
**          dm1bx_lk_convert() keep current leaf mutexed while locks on sibling
**          leaf are converted and propagated.  (B110332)
**      05-aug-2003 (stial01)
**          dm1bxsplit() use atts_array,atts_count to compress desc_key (b110437
**      02-jan-2004 (stial01)
**          Changes for REDO recovery of etab btree data pages if NOBLOBLOGGING:
**          dm1bxinsert() Added return_lsn parameter, set DM0L_NOBLOBLOGGING
**          flag in the DM0LBTPUT log record.
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      09-mar-2004 (stial01)
**          dm1bxclean() Don't clean deleted keys if nologging (b111931)
**      13-may-2004 (stial01)
**          Remove unecessary code for NOBLOBLOGGING redo recovery
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      29-sep-2005 (stial01)
**          server trace point dm723 to verify update lock protocols (b115299)
**	13-Apr-2006 (jenjo02)
**	    Add support for CLUSTERED primary tables, and evisceration of
**	    non-key attributes from range entries.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**      29-sep-2006 (stial01)
**          Support multiple RANGE formats: OLD format:leafkey,NEW format:ixkey
**	22-Feb-2008 (kschendel) SIR 122739
**	    Changes throughout for restructured rowaccess stuff.
**	    *Note* One important change is that the compression-type sent
**	    to BTxx log records is now the real compression type, and not
**	    the phony dm1cx yes/no flag.  Fortunately, TCB_C_STANDARD which
**	    is the only currently valid key compression is 1, which is also
**	    the old DM1CX_COMPRESSED value.  So, old log records remain valid.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1r_?, dm1b_? functions converted to DB_ERROR *, use new
**	    form uleFormat.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      10-sep-2009 (stial01)
**          DM1B_DUPS_ON_OVFL_MACRO true if btree can have overflow pages
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Feed btflags instead of ixklen to btput, btdel, btfree log records.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**      04-dec-2009 (stial01)
**          Test tcb_dups_on_ovfl instead of DM1B_DUPS_ON_OVFL_MACRO,
**          dm1bxsplit() new mode parameter so we can split without 
**          allocating (reserve) if mode is DM1C_SPLIT_DUPS
**      11-jan-2010 (stial01)
**          Use macros for setting and checking position information in rcb
**	15-Jan-2010 (jonj,stial01)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    New macros to lock/unlock pages, LG_LRI replaces LG_LSN,
**	    sensitized to crow_locking().
**	23-Feb-2010 (stial01)
**          dm1bxclean() pass rcb to dm1cxclean
**	15-Apr-2010 (stial01)
**          dm1bxdelete() More validation of entry to be deleted
**/

/*
**  Forward and/or External function references.
*/

static	DB_STATUS binary_search(
    DMP_RCB      *rcb,
    DMPP_PAGE   *page,
    i4      direction,
    char         *key,
    i4	 keyno,
    DM_LINE_IDX  *pos,
    DB_ERROR	 *dberr);

static	DB_STATUS division(
    DMP_RCB             *rcb,
    DMPP_PAGE          *buffer,
    DM_LINE_IDX         *divide,
    i4             keyno,
    DB_ERROR	 *dberr);
    
static DB_STATUS get_range_entries(
    DMP_RCB		*r,
    DMPP_PAGE		*cur,
    char		*range[],
    i4		range_len[],
    i4		infinity[],
    DB_ERROR	 *dberr);

static DB_STATUS update_range_entries(
    DMP_RCB		*rcb,
    DMPP_PAGE		*cur,
    DMPP_PAGE		*sib,
    char		*range[],
    i4		range_len[],
    i4		infinity[],
    char		*midkey,
    i4		midkey_len,
    DB_ERROR	 *dberr);
    
static DB_STATUS update_overflow_chain(
    DMP_RCB	    *r,
    DMP_PINFO	    *lfPinfo,
    DB_ERROR	 *dberr);

static DB_STATUS get_dup_key(
    DMP_RCB             *rcb,
    DMPP_PAGE          *current,
    char		*splitkeybuf,
    DB_ERROR	 *dberr);

static DB_STATUS dm1bx_lk_convert(
    DMP_RCB *r,
    i4   insert_direction,
    DMP_PINFO *currentPinfo,
    DMP_PINFO *siblingPinfo,
    LK_LKID   *sib_lkid,
    LK_LKID   *data_lkid,
    DB_ERROR	 *dberr);


/*{
** Name: dm1bxchain - Follows an overflow chain for a BTREE.
**
** Description:
**      This routines follows an btree overflow chain to get 
**      the next (key,tid) pair for the key specified.  It 
**      uses as a starting point an input bid, it updates this
**      value if it finds another entry.  You
**      can only follow an overflow chain forward.
**
**      This routine assumes that the page indicated by leaf is fixed
**      on entry and exit.  This page may change as chain is
**      followed. 
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf                            Pointer to the leaf page 
**                                      associated with this chain.
**      bid				A TID which points to position
**                                      along the chain to start searching.
**      start                           Flag to indicate if starting at
**                                      leaf (i.e. beginning of chain).
**
** Outputs:
**      bid                             Pointer to an area to return
**                                      bid of qualifying record.
**      err_code                        Pointer to an area to return 
**                                      the following errors when 
**                                      E_DB_ERROR is returned.
**                                      E_DM0055_NONEXT;
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    Leaves last page accessed fixed.
**
** History:
**      08-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	24-may-93 (rogerk)
**	    Added traceback error message.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
**      07-apr-1997 (stial01)
**          dm1bxchain() NonUnique primary btree (V2), dups can span leaf
**          New key parameter.
**      21-may-97 (stial01)
**          dm1bxchain() New buf_locked parameter.
**          No more LK_PH_PAGE locks, buffer locked with dm0p_lock_buf_macro()
**      12-jun-97 (stial01)
**          dm1bxchain() Pass tlv to dm1cxget instead of tcb.
**	18-dec-98 (thaju02)
**	    fix overflow page with correct fix action. (b94647)
*/
DB_STATUS
dm1bxchain(
    DMP_RCB		*rcb,
    DMP_PINFO		*leafPinfo,
    DM_TID		*bid,
    i4		start,
    char                *key,
    DB_ERROR            *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_TABLE_IO        *tbio = &t->tcb_table_io;
    DB_STATUS           s = E_DB_OK;
    DM_PAGENO           next;
    ADF_CB              *adf_cb;
    i4             	compare;
    char		*KeyBuf = NULL, *AllocKbuf = NULL;
    char                *tempkey;
    char                temp_buf[DM1B_MAXSTACKKEY];
    i4             	tempkeylen;
    i4			fix_action;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE		**leaf;

    CLRDBERR(dberr);

    leaf = &leafPinfo->page;

    if (start)
    {
	/* If the leaf page has entries, point it to first. */

	if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf) > 0)
	{
	    bid->tid_tid.tid_line = 0; 
	    return(E_DB_OK); 
	}
    }
    else
    {
	/* 
	** If not starting and the current page has more entries 
	** then point to next one on this page.
	*/

	if ((i4)(bid->tid_tid.tid_line) < 
		(DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf) - 1))
	{
	    bid->tid_tid.tid_line++; 
	    return (E_DB_OK); 
	}
    }

    adf_cb = r->rcb_adf_cb;

    /* 
    ** Get the next page with our dupkey on it
    **
    ** If there is no next, then indicate end of scan
    */
    for ( ; ; )
    {
	if (t->tcb_dups_on_ovfl)
	{
	    next = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *leaf);
	}
	else
	{
	    next = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, *leaf);
	    if (next == 0)
		break;

	    if (key != NULL)
	    {
		/*
		** Don't read the next leaf page if our key doesn't
		** match the RRANGE key
		*/
		if ( !KeyBuf && (s = dm1b_AllocKeyBuf(t->tcb_rngklen, temp_buf,
					    &KeyBuf, &AllocKbuf, dberr)) )
		    break;
		tempkey = KeyBuf;
		tempkeylen = t->tcb_rngklen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			*leaf, t->tcb_rng_rac,
			DM1B_RRANGE,
			&tempkey, (DM_TID *)0, (i4*)0, &tempkeylen,
			NULL, NULL, adf_cb);
		if (s != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
		       t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_RRANGE);
		    SETDBERR(dberr, 0, E_DM9C1B_DM1BXCHAIN);
		    break;
		}
		/* Compare range key :: leafkey */
		adt_ixlcmp(adf_cb, t->tcb_keys, 
				t->tcb_rngkeys, tempkey, 
				t->tcb_leafkeys, key, &compare);
		if (compare != DM1B_SAME)
		    next = 0;
	    }
	}

	if (next == 0)
	    break;

	dm0pUnlockBuf(r, leafPinfo);

        /* Free previous page, if not leaf page. */
	s = dm0p_unfix_page(r, DM0P_UNFIX, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	/* Get next page on chain. */
	fix_action = DM0P_READ;
	if (r->rcb_access_mode == RCB_A_WRITE)
	    fix_action = DM0P_WRITE;
	s = dm0p_fix_page(r, next, fix_action, leafPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	if (r->rcb_access_mode == RCB_A_WRITE)
	{
	    /*
	    ** Lock buffer for update.
	    **
	    ** This will swap from CR->Root if leafPinfo->page 
	    ** is a CR page.
	    */
	    dm0pLockBufForUpd(r, leafPinfo);
	}
	else
	{
	    /*
	    ** Lock buffer for get.
	    **
	    ** This may produce a CR page and overlay leafPinfo->page
	    ** with its pointer.
	    */
	    dm0pLockBufForGet(r, leafPinfo, &s, dberr);
	    if (s == E_DB_ERROR)
		break;
	}

	if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf))
	{
	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
	    /*
	    ** Set the result bid to point to the first entry on this
	    ** overflow page
	    */
	    bid->tid_tid.tid_line = 0;
	    bid->tid_tid.tid_page = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);
	    return (E_DB_OK);
	}

	/*
	** This was an empty page. Advance to the next overflow/leaf page
	*/
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if (s != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C1B_DM1BXCHAIN);
	return (E_DB_ERROR);   
    }

    SETDBERR(dberr, 0, E_DM0055_NONEXT);
    return (E_DB_ERROR);   
}

/*{
** Name: dm1bxdelete - Deletes a (key,tid) pair from a page.
**
** Description:
**      This routines delete a (key,tid) pair from a page.
**
**      In doing this delete it must compress the line table, and may or may
**      not reclaim the record space. The space management is handled by the
**	dm1cx* layer.
**
**	If this is an index page, then the keyspace is not lost, but combined
**	with the next lower entry.  That is, if the previous line table entry
**	indicated the highest key on its associated leaf page was "abc", the
**	one being deleted indicated "def" then the previous entry is changed to
**	indicate it's key space includes "def".
**
**	When we are deleting an entry on an index page as a result of a join,
**	we can think of this as combining two entries. We want the tidp from
**	the previous entry combined with the key from this entry. Since
**	shuffling tidp's around can be done easily (they're all fixed length,
**	so we can do this in place), but moving keys around is hard (they may
**	be different sizes, so this may require a delete and an insert to
**	update a key), it is vastly preferable to implement this operation on
**	index pages where child > 0 as "delete the (child-1) entry, but save its
**	tidp and update the child entry to contain that tidp".
**
**	The log_updates flag should be set to TRUE for most invocations of
**	this routine.  It indicates that the index update needs to be logged
**	for recovery purposes.  It should be set to TRUE if the update
**	operation is logged (and recovered) in some other mechanism (such as
**	using Before and After Images on the updated pages) by the caller
**	of this routine.  Note that if the table or session has a nologging
**	attribute, then no log records will be written regardless of the
**	setting of this flag.
**
**	The mutex_requried flag should be set to TRUE for most invocations of
**	this routine.  It indicates that the index page passed in is shared
**	by other threads and must be protected while updated.  It can be
**	set to FALSE if the caller has a private copy of the page (eg during
**	build processing).
**      
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      buffer                          Pointer to the page containing
**                                      the record to delete.
**      child				A LINE_ID portaion of a TID which 
**                                      points to the record to delete.
**	log_updates			Indicates to log the update.
**	mutex_required			Indicates to mutex page while updating.
**      reclaim_space			Conditional space reclamation
**
** Outputs:
**	err_code			Set if this routine returns E_DB_ERROR:
**					E_DM93B7_BXDELETE_ERROR
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR
**
** History:
**      09-oct-85 (jennifer)
**          Converted for Jupiter.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression. Noticed that
**	    for index entry combining of entries it was cleaner to delete the
**	    previous entry and shuffle its tidp than to delete this entry and
**	    move its key, and implemented this operation that way. Added new
**	    err_code argument since this function now returns a status.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-apr-1993 (jnash)
**	    Correct amount of space reserved by LGreserve. 
**	26-jul-1993 (rogerk)
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Shifted some code around to make error handling after the BI call
**	    a little easier.
**	  - Fixed bug in logging of index delete operations (during joins).
**	    When deleting from a non-leaf page, the key deleted is actually
**	    the key in entry (child - 1).  So we need to log this key value,
**	    not the one in the child entry.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Subtract size of tuple header from index entry length for leaf
**	    page only.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**	    Set DM0L_ROW_LOCK flag if row locking is enabled.
**          dm1bxdelete() Pass reclaim_space to dm1cxdel()
**      24-jan-97 (dilma04)
**          Fix bug 80267: Do not set DM0L_K_ROW_LOCK flag if page size is 2k.
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-1997 (stial01)
**          dm1bxdelete() reclaim_space was changed from input to output param.
**          Don't reclaim space if deleting last key on leaf with overflow chain
**          Init flags parameter for dm1cxdel.
**      07-apr-97 (stial01)
**          dm1bxdelete() Removed code to keep overflow key on leaf,
**          This is not needed anymore.
**      21-may-97 (stial01)
**          dm1bxdelete() Added flags arg to dm0p_unmutex call(s).
**      17-jun-97 (stial01)
**          dm1bxdelete() set DM0L_BT_UNIQUE for unique indexes
**      18-jun-97 (stial01)
**          dm1bxdelete() Do not set DM0L_ROW_LOCK when updating index pages
**      29-jul-97 (stial01)
**          dm1bxdelete() Added consistency check to validate child position
**      21-apr-98 (stial01)
**          dm1bxdelete() Set DM0L_PHYS_LOCK if extension table (B90677)
**      07-jul-98 (stial01)
**          dm1bxdelete() Deferred update: new update_mode if row locking
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
DB_STATUS
dm1bxdelete(
    DMP_RCB	    *rcb,
    DMP_PINFO	    *bufferPinfo,
    DM_LINE_IDX	    child,
    i4	    log_updates,
    i4	    mutex_required,
    i4         *reclaim_space,
    DB_ERROR	    *dberr)
{
    DMPP_PAGE		*b;
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_DCB		*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    i4		pos, out; 
    i4		update_mode;
    i4             dmcx_flag;
    i4		compression_type;
    i4		loc_config_id;
    i4		loc_id;
    i4		key_len;
    char		*key;
    LG_LSN		lsn;
    DM_TID		prevtid;
    i4			prevpartno;
    DM_TID		delbid;
    DM_TID		deltid;
    i4			delpartno;
    DM_LINE_IDX		childkey;
    bool		log_required;
    bool		index_update;
    bool		mutex_done = FALSE;
    bool		critical_section = FALSE;
    CL_ERR_DESC	    	sys_err;
    i4                  compare = DM1B_SAME;
    i4		    *err_code = &dberr->err_code;
    LG_LRI		lri;
    bool		bad_tid = FALSE;
    bool		is_deleted = FALSE;

    CLRDBERR(dberr);

    b = bufferPinfo->page;

    /*
    ** Setup information needed for dm1cx calls and the log record write below.
    */
    delbid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b);
    delbid.tid_tid.tid_line = child;
    compression_type = DM1B_INDEX_COMPRESSION(r);
    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, b) & 
			DMPP_INDEX) != 0);
    log_required = (log_updates && r->rcb_logging);

    /*
    ** Set the partition number of the base Table;
    ** If a global index, this will be overriden below
    ** by dm1cxtget.
    */
    delpartno = t->tcb_parent_tcb_ptr->tcb_partno;

    /* Deletes are always direct updates */
    update_mode = DM1C_DIRECT;

    /* Set update_mode DM1C_ROWLK, so that tranid goes in tuple hdr */
    if (!index_update && t->tcb_rel.relpgtype != TCB_PG_V1 &&
	(row_locking(r) || crow_locking(r) || t->tcb_extended))
	    update_mode |= DM1C_ROWLK;

    /*
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b));
    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

    /*
    ** Validate the delete request.  Check that the requested entries 
    ** exist on the given page and that they can be deleted.  We do this
    ** because it is very bad to find out AFTER having logged the update
    ** that the requested row does not exist or cannot be deleted.  Note
    ** that we do not expect such a condition to occur here (it would
    ** indicate a bug), but if we catch it before the dm0l call then the
    ** effect of the bug will be to cause the query to fail rather than to
    ** cause the database to go inconsistent.
    */
    {
	/*
	** If child not on this page, do nothing and return. We don't currently
	** treat this as an error.
	**
	** Note: since we don't really think this case should ever occur (and
	** it seems like it would be bad if it did), then write a warning 
	** so we can track its occurrence.  We'd take it out but we're afraid.
	*/
	if ((i4)child >= 
		(i4)DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b))
	{
	    uleFormat(NULL, E_DM9C1C_BTDELBYPASS_WARN, (CL_ERR_DESC*)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		0, child, 0, DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b), 
		0, DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b));
	    return (E_DB_OK);
	}

	/*
	** Adjust the key position for non-leaf page updates.
	** On index page deletes, the child value actually indicates
	** the position of the BID to remove.  They key at that position
	** will be preserved however (and collapsed down into the previous
	** btree slot).  The key being deleted is the one at (child - 1).
	**
	** If we are deleting from an index page then set the childkey
	** position back one slot (if there is one) to make sure that we
	** get the proper key to log.
	*/
	childkey = child;
	if ((index_update) && (child != 0))
	    childkey--;

	if (!index_update && dmpp_vpt_test_free_macro(tbio->tbio_page_type,
		DM1B_VPT_BT_SEQUENCE_MACRO(tbio->tbio_page_type, b), 
			(i4)child + DM1B_OFFSEQ))
	{
	    /* must have repositioned to WRONG/deleted entry */
	    is_deleted = TRUE;
	}

	/*
	** Look up pointer to the delete victim's key.  In addition to
	** validating the existence of the entry, the key is needed in the
	** log call below.  Also get the length of the (possibly compressed) 
	** key entry and its tidp.
	*/
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, b, childkey, &key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, b, child, 
			&deltid, &delpartno);
	if (!index_update && r->rcb_currenttid.tid_i4 != deltid.tid_i4)
	{
	   bad_tid = TRUE;
	}

	/* Non-compressed keys: we just use the fixed size key length */
	key_len = index_update ? t->tcb_ixklen : t->tcb_klen;
	if (compression_type != DM1CX_UNCOMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, b, 
		childkey,&key_len);
	}

	/* One last check to make sure its the right one to delete */
	if (!index_update)
	{
	    status = dm1b_compare_key(r, b, child, r->rcb_fet_key_ptr,  
		t->tcb_keys, &compare, dberr);
	}

	if ((status != E_DB_OK) || is_deleted || bad_tid ||
	    DMPP_VPT_IS_CR_PAGE(tbio->tbio_page_type, b) ||
	    (compare != DM1B_SAME) ||
	    ((i4) child >= 
	    (i4) DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b)) || 
	    (key < ((char*)b + DM1B_VPT_OVER(tbio->tbio_page_type))) || 
	    (key > ((char*)b + tbio->tbio_page_size)) ||
	    (key_len < 0) || 
	    (key_len > DM1B_MAXLEAFLEN))
	{
	    uleFormat(NULL, E_DM9C1D_BXBADKEY, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		0, child, 0, DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, b),
		0, DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, b), 0, key_len,
		0, (key - (char *)b));


	    TRdisplay("dm1bxdelete (%d,%d) tid (%d,%d) currenttid (%d,%d)\n",
		delbid.tid_tid.tid_page, delbid.tid_tid.tid_line,
		deltid.tid_tid.tid_page, deltid.tid_tid.tid_line,
		r->rcb_currenttid.tid_tid.tid_page,
		r->rcb_currenttid.tid_tid.tid_line);

	    dmd_prindex(rcb, b, 1);

	    uleFormat(NULL, E_DM93B7_BXDELETE_ERROR, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
	    return (E_DB_ERROR);
	}

    }

    for (;;)
    {
	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, b, dberr);
	    if (status != E_DB_OK)
	        break;
	}

	/*
	** Reserve the log space for the btree index delete log record and for
	** its CLR should the transaction be rolled back.
	** This must be done previous to mutexing the page since the reserve
	** request may block.
	*/
	if (log_required)
	{
	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		(2 * (sizeof(DM0L_BTDEL) - (DM1B_MAXLEAFLEN - key_len))),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
		break;
	    }
	}

	/*
	** If the buffer resides in the DBMS buffer cache, then we have to mutex
	** it while we change it to prevent other threads from looking at it
	** while it is inconsistent.
	*/
	if (mutex_required)
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, bufferPinfo);
	    mutex_done = TRUE;
	}

	/*
	** Delete to Btree Leaf Page, page type != TCB_PG_V1:
	**
	**     Just mark the leaf entry deleted.
	**     We use passive_rcb_update to maintain btree cursor position.
	**     Passive rcb update requires that deletes DO NOT reclaim space.
	**     If another cursor is currently positioned on the leaf entry
	**     we are deleting, we will need to reposition to the deleted
	**     at GET-NEXT time.
	*/
	if ( (tbio->tbio_page_type != TCB_PG_V1) && !index_update)
	    *reclaim_space = FALSE;
	else
	    *reclaim_space = TRUE;

	/*
	** Log the Delete to the Btree Index Page.
	** The LSN of the log record must be written to the updated page.
	** If the log write fails then return without updating the page.
	*/
	if (log_required)
	{
	    i4 dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);
	    u_i4 dm0l_btflag = 0;

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs, extension tables
	    ** and index pages. The same lock protocol must be observed during 
	    ** recovery.
	    */
	    if ( NeedPhysLock(r) || index_update )
		dm0l_flag |= DM0L_PHYS_LOCK;
	    else if (row_locking(r))
		dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
		dm0l_flag |= DM0L_CROW_LOCK;

	    if (t->tcb_rel.relstat & TCB_UNIQUE)
		dm0l_btflag |= DM0L_BT_UNIQUE;
	    if (t->tcb_dups_on_ovfl)
		dm0l_btflag |= DM0L_BT_DUPS_ON_OVFL;
	    if (t->tcb_rng_has_nonkey)
		dm0l_btflag |= DM0L_BT_RNG_HAS_NONKEY;

	    if (!index_update && DMZ_TBL_MACRO(23))
	    {
		status = dbg_dm723(r, &delbid, &deltid, dm0l_flag, err_code);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, *err_code);
		    break;
		}
	    }

	    /* Get CR info from page, pass to dm0l_btdel */
	    DM1B_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, b, &lri);

	    status = dm0l_btdel(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid,
		&t->tcb_rel.relid, t->tcb_relid_len,
		&t->tcb_rel.relowner, t->tcb_relowner_len,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_index_rac.compression_type, 
		t->tcb_rel.relloccount, loc_config_id,
		&delbid, child,
		&deltid, key_len, key, (LG_LSN *) 0, &lri, 
		delpartno, dm0l_btflag, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Now that the update has been logged, we must succeed in changing
	    ** the page.  If we encounter an error now, we must crash and let
	    ** the page contents be rebuilt via REDO recovery.
	    */
	    critical_section = TRUE;

	    /*
	    ** Write the LSN and other info about the record to the page
	    ** being updated.
	    ** This marks the version of the page for REDO processing and
	    ** CR undo.
	    */
	    DM1B_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, b, &lri);
        }
	else if (!index_update)
	    DM1B_VPT_INCR_BT_CLEAN_COUNT_MACRO(tbio->tbio_page_type, b);

	/*
	** Proceed with the delete.  The delete semantics depend on whether
	** this is a leaf or index page.
	**
	** For leaf deletes, simply delete the entry at the 'child' position.
	**
	** INDEX (NON-LEAF) PROCESSING:
	**
	**     If this is an index page, then we are actually combining the
	**     child and (child-1) entries by deleting the tid value from the
	**     'child' position and the key value from the previous position.
	**
	**     As long as child > 0, then we want the key from the child entry 
	**     and the tidp from the (child-1) entry to end up as the new 
	**     (child-1) entry. 
	**
	**     If child == 0, however, then there is no previous entry to 
	**     combine with and we simply delete the specified child entry.
	**
	*/
	childkey = child;
	if ((index_update) && (child > 0))
	{
	    /*
	    ** We are merging two child pages, and are deleting the entry
	    ** for the right child from the parent. In this case, the
	    ** entries for BOTH children are now found by following the
	    ** left child's pointer, so we need to combine the entries to
	    ** result in a new entry which indicates that all entries <=
	    ** the right child's key are now found in the left child. That
	    ** is, we must end up with the right child's key and the left
	    ** child's tidp, and must delete one of the entries.
	    **
	    ** Since it's dramatically easier to manipulate tidps than to
	    ** manipulate keys (which may be compressed), we implement
	    ** this as:
	    **     extract the (child-1) entry's tidp
	    **     update the (child) entry's tidp with this value
	    **     delete the (child-1) entry
	    */
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, b, 
			(child-1), &prevtid, &prevpartno );
	    status = dm1cxtput(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, b,
			child, &prevtid, prevpartno );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, r, b, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
		SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
		break;
	    }

	    /* Set childkey to (child - 1) for dm1cxdel call below. */
	    childkey--;
	}

	dmcx_flag = 0;
	if (*reclaim_space == TRUE)
	    dmcx_flag |= DM1CX_RECLAIM;

	/*
	** Delete the key,tid pair.
	** Use 'childkey' position which will either be the caller supplied
	** child position (if leaf page) or the adjusted position of the
	** delete-key.
	*/
	status = dm1cxdel(tbio->tbio_page_type, tbio->tbio_page_size, b,
		    update_mode, compression_type,
		    &r->rcb_tran_id, r->rcb_slog_id_id, r->rcb_seq_number, 
		    dmcx_flag, childkey);

	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, r, b, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
	    SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
	    break;
	}

	/*
	** Unmutex the leaf page now that we are finished updating it.
	*/
	if (mutex_required)
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, bufferPinfo);

	return (E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93B7_BXDELETE_ERROR);
    }

    /*
    ** If an error occurred while updating the index page leaving the page's
    ** contents possibly trashed.  If we are logging updates, then we're
    ** hosed.  We crash the server and hope that REDO recovery will correctly
    ** rebuild the page contents.  Note that our inconistent page cannot
    ** be written to the database (since we crash while holding the mutex).
    **
    ** If we are not logging (just doing a load operation), then we can
    ** return an error and presumably let the trashed pages be tossed.
    */
    if (critical_section)
	dmd_check(E_DM93B7_BXDELETE_ERROR);

    if (mutex_done)
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, bufferPinfo);

    return (E_DB_ERROR);
}

/*{
** Name: dm1bxdupkey - Gets the duplicated key from BTREE chain.
**
** Description:
**	This routine returns the duplicate key value for a leaf overflow chain.
**	In a leaf overflow chain, all entries must have the same key value.
**
**	The routine follows the chain and returns the key value of the first
**	entry it finds.  The primary (first of the chain) leaf page is passed
**	in as an argument and must be currently fixed.  It may be temporarily
**	unfixed as the chain is traversed but will be left fixed on return.
**
**	If the chain contains no entries, E_DM0056_NOPART is returned.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      leaf                            Pointer to the leaf page 
**                                      associated with this chain.
**      dupspace            		A pointer to an area where the key
**                                      can be returned.
**
** Outputs:
**      state                           Pointer to an area to return
**                                      result of scan for entry with key.
**                                      Will be E_DB_OK or E_DM0056_NOPART.
**      err_code                        Pointer to an area to return 
**                                      the errors as a result
**                                      of accessing new overflow pages.
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**	Exceptions:
**
** Side Effects:
**
** History:
**      09-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	18-dec-90 (rogerk)
**	    Fixed problem following dm1bxchain call which does not return
**	    E_DB_OK.  In this case we would not refix the correct leaf
**	    page into the callers 'leaf' argument.  This would cause us
**	    to insert a record onto the last overflow page if an append
**	    were done to an empty leaf with an empty overflow chain.
**	    (This would not cause any noticible 'bugs', but the system
**	    assumes that inserts are done to LEAF pages)
**	22-jan-1991 (bryanp)
**	    Call dm1cx routines to support index compression.
**	06-may-1996 (nanpr01)
**	    Changed all page header access as macro with New Page Format
**	    project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Check for E_DB_WARNING return code from dm1cxget()
**      27-feb-97 (stial01)
**          If dm1b_v2_index (page size != DM_COMPAT_PGSIZE (2k)):
**          We never reclaim space when we delete the last
**          duplicated key from a leaf page that has an overflow chain.
**          We should never have to look to the overflow pages for
**          the duplicated key value.
**       07-apr-97 (stial01)
**          dm1bxdupkey() Overflow key value processing, return immediately if
**          NonUnique primary btree (V2) index.
**       21-may-97 (stial01)
**          dm1bxdupkey() Pass buf_locked = FALSE to dm1bxchain.
**      12-jun-97 (stial01)
**          dm1bxdupkey() Pass tlv to dm1cxget instead of tcb.
*/
DB_STATUS
dm1bxdupkey(
    DMP_RCB	       *rcb,
    DMP_PINFO	       *leafPinfo,
    char               dupspace[],
    DB_STATUS          *state,
    DB_ERROR           *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM_TID          tempbid[1]; 
    DM_PAGENO       lfpageno; 
    char            *dupkey;
    i4	    klen;
    DB_STATUS       s;
    DB_STATUS       savestatus;
    i4         local_err_code;
    i4         fix_action;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMPP_PAGE	    **leaf;

    CLRDBERR(dberr);

    if (t->tcb_dups_on_ovfl == FALSE)
    {
	*state = E_DM0056_NOPART;
	return (E_DB_OK);
    }

    leaf = &leafPinfo->page;

    dupkey = NULL; 

    /* Make sure this is really leaf with overflow chain. */

    if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, *leaf) != 0)
    {
	/*
	** If leaf page has entries, use it for getting key.
	*/

        if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, *leaf) != 0)
        {
	    dupkey = dupspace;
	    klen = t->tcb_klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			(DM_LINE_IDX)0, &dupkey, (DM_TID *)NULL,  (i4*)NULL,
			&klen,
			NULL, NULL, r->rcb_adf_cb);

	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
		s = E_DB_OK;

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (DM_LINE_IDX)0);
		SETDBERR(dberr, 0, E_DM93B0_BXDUPKEY_ERROR);
		return (s);
	    }
	    if (dupkey != dupspace)
		MEcopy(dupkey, klen, dupspace);
        }
        else
        {
	    /* 
	    ** Here in dm1bxdupkey(), if t->tcb_rel.relpgtype != TCB_PG_V1
	    ** we should always be able to get the dupkey from the leaf 
	    ** at the head of the overflow chain because
	    ** in dm1cidx xdelete()we never delete the last dupkey from the leaf
	    ** It is an error not to find a key
	    */
	    if (t->tcb_rel.relpgtype != TCB_PG_V1)
	    {
		TRdisplay("dm1bxdupkey dup not found on %d (unexpected)\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, *leaf));
	    }

            /* Save leaf page pointer so we can get it back. */
            lfpageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf);

            /* Follow chain to get first page with entry. */

            tempbid->tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf); 

	    /*
	    ** Look for the next row on this overflow chain.
	    ** Note that this call may return with a different page
	    ** fixed in 'leaf'.
	    */
            s = dm1bxchain(r, leafPinfo, tempbid, DM1C_NEXT, (char *)NULL, 
			    &local_dberr); 
            if (s == E_DB_OK)
            {
		dupkey = dupspace;
		klen = t->tcb_klen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *leaf,
			&t->tcb_leaf_rac,
			(DM_LINE_IDX)0, &dupkey,
			(DM_TID *)NULL,  (i4*)NULL,
			&klen, NULL, NULL, r->rcb_adf_cb);

		if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
		    s = E_DB_OK;

		if (s != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, *leaf,
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			(DM_LINE_IDX)0);
		    SETDBERR(dberr, 0, E_DM93B0_BXDUPKEY_ERROR);
		    return (s);
		}
		if (dupkey != dupspace)
		    MEcopy(dupkey, klen, dupspace);
            }
	    savestatus = s;	    

	    /*
	    ** Make sure old leaf page is still fixed.
	    */
	    if (leafPinfo->page == NULL ||
		(lfpageno != DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *leaf)))
	    {
		dm0pUnlockBuf(r, leafPinfo);

		if (leafPinfo->page != NULL)
		{
		    s = dm0p_unfix_page (r, DM0P_UNFIX, leafPinfo,  dberr);
		    if (s != E_DB_OK)
			return (s);
		}
		fix_action = DM0P_WRITE;
		if (r->rcb_access_mode == RCB_A_READ)
		    fix_action = DM0P_READ;
		s = dm0p_fix_page(r, lfpageno, fix_action, leafPinfo, dberr);
		if (s != E_DB_OK)
		    return (s);

		/*
		** Lock buffer for update.
		**
		** This will swap from CR->Root if "leaf" is a CR page.
		*/
		dm0pLockBufForUpd(r, leafPinfo);
	    }		    
            if (savestatus != E_DB_OK && local_dberr.err_code != E_DM0055_NONEXT)
	    {
		*dberr = local_dberr;
		return (savestatus);
	    }
        }
    }
    if (dupkey == NULL)
        *state = E_DM0056_NOPART;
    return (E_DB_OK);
}

/*{
** Name: dm1bxformat - Format a BTREE page.
**
** Description:
**      This routine formats an empty BTREE page.
**      For data pages it calls the general data page formatting
**      routine. For BTREE index pages this routines calls the index formatting
**	routine.
**
**	NOTE: Before the Btree index compression project, index pages were NOT
**	filled with zeroes as part of the formatting. Therefore, the unused
**	sections of the DMPP_PAGE structure do NOT have any reliable contents
**	in old Btrees. Beware of this compatibility problem if you attempt to
**	use some of the bt_unused space in the page...
**
** Inputs:
**	page_type			Page type for this btree.
**	page_size			Page size for this btree.
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**					MAY BE NULL if no rcb available, in
**					which case use zero tranid and seq num.
**	tcb				Pointer to table control block. Even
**					when rcb is NULL, tcb must be
**					supplied. NOTE: This tcb may not be the
**					tcb whose pages we're formatting; for
**					example, during an index build the tcb
**					is the base table's TCB.
**      buffer                          Pointer to the page to format.
**	page_size			Page size for this btree.
**      kperpage                        Value indicating maximum keys per page. 
**      klen                            Value indicating key length.
**      page			        The PAGENO part of a TID which 
**                                      indicates the logical page number
**                                      of page formatting.
**      pagetype                        Value indicating type of page 
**                                      formatiing.
**	compression_type		Value indicating index compression:
**					    DM1CX_UNCOMPRESSED
**					    DM1CX_COMPRESSED
**
** Outputs:
**	err_code			Set to an error code if an error occurs
**
**	Returns:
**
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-oct-85 (jennifer)
**          Converted for Jupiter.
**	23-feb-89 (mmm)
**	    bug fix for bug number 4780.  Data appended to a btree secondary
**	    index using a fast commit server would sometimes be lost following
**	    REDO recovery (ie. server crash prior to data being forced to disk).
**
**	    dm1bxformat(), did not initialize the page_log_addr field in
**	    btree secondary index pages, thus it was set to whatever value 
**	    happened to be in memory.  This would cause redo recovery to be 
**	    incorrect in some cases.  
**
**	    If a server crash occurs between the time the last page added to a 
**	    btree secondary index is forced to disk with the unitialized log 
**	    address, and when the page is written with new data "put" on it 
**	    (either by a consistency point or the buffer manager forcing 
**	    it to disk) then REDO recovery uses this field to determine if it 
**	    needs to redo puts to the secondary index.  In some cases on unix 
**	    the value that happened to be at that address was a log address 
**	    greater than the current, so REDO recovery thought it had nothing 
**	    to do.  To fix this the field is initialized to zero's which is 
**	    guaranteed smaller than any legal log address.
**	10-apr-1990 (Derek)
**	    Undo (mmm) fix because the free list routine do correctly
**	    initialize the log_addr at allocation time.  Also remove
**	    calls to dm1c_format() because the allocation routine
**	    does this also.
**	24-jan-1991 (bryanp)
**	    Call dm1cx routines to support index compression. Added err_code
**	    argument to pass back error codes, and compression_type argument
**	    to pass in compression information.
**	24-oct-1991 (jnash)
**	     Add return at end of routine to satisfy lint.
**	13-jul-1992 (kwatts)
**	    MPF changes. Added the tcb as a parameter that must always be
**	    present.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Changed dm1cxformat to no longer take 
**	    TCB parameter so it could be used by recovery.
**	24-may-1993 (rogerk)
**	    Removed code which handled request to format a data page
**	    instead of an index page after verifying that this routine is
**	    only called on btree specific page types.
**	06-mar-1996 (stial01 for bryanp) (bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dm1cxformat.
**	    Add page_size argument to dm1bxformat, since the tcb passed to
**		dm1bxformat may not have the correct page size in it
**		(particularly during an index build, when the tcb passed to
**		dm1bxformat is the *base table* tcb).
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format 
**	    project.
**	    Added loc_plv pointer parameter and pass to dm1cxformat().
**      12-jun-97 (stial01)
**          dm1bxformat() new attribute, key parameters
**          Pass new attribute, key parameters to dm1cxformat
**      07-jul-98 (stial01)
**          dm1bxformat() Deferred update: new update_mode if row locking
**	04-Jan-2004 (jenjo02)
**	    Accept and pass tidsize on to dm1cxformat().
**	25-Apr-2006 (jenjo02)
**	    New parm, Clustered, for primary Btree Leaf pages.
*/
DB_STATUS
dm1bxformat(
    i4			page_type,
    i4			page_size,
    DMP_RCB             *rcb,
    DMP_TCB             *tcb,
    DMPP_PAGE          *buffer,
    i4             kperpage,
    i4             klen,
    DB_ATTS             **atts_array,
    i4             atts_count,
    DB_ATTS             **key_array,
    i4             key_count,
    DM_PAGENO           page,
    i4             indexpagetype,
    i4		compression_type,
    DMPP_ACC_PLV	*loc_plv,
    i2			tidsize,
    i4		   	Clustered,
    DB_ERROR		*dberr)
{
    DMP_RCB	    *r = rcb;
    DMPP_PAGE	    *b = buffer;
    DB_TRAN_ID	    temp_tran_id;
    i4	    sequence_number; 
    DB_TRAN_ID	    *tran_id;
    DB_STATUS	    s;

    CLRDBERR(dberr);
    
    DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, b, 0); 
    DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, b, 0); 

    DM1B_VPT_SET_PAGE_PAGE_MACRO(page_type, b, page);

    if (rcb != 0)
    {
	tran_id = &rcb->rcb_tran_id;
	sequence_number = rcb->rcb_seq_number;
    }
    else
    {
	MEfill(sizeof(DB_TRAN_ID), '\0', (PTR) &temp_tran_id);
	tran_id = &temp_tran_id;
	sequence_number = 0;
    }

    s = dm1cxformat(page_type, page_size,  b, 
			loc_plv, compression_type, 
			kperpage, klen, atts_array, atts_count,
			key_array, key_count, page, indexpagetype, TCB_BTREE,
			tidsize, Clustered);
    if (s != E_DB_OK)
    {
	dm1cxlog_error( E_DM93E5_BAD_INDEX_FORMAT, r, b, 
		page_type, page_size, (DM_LINE_IDX)0 );
	SETDBERR(dberr, 0, E_DM93B8_BXFORMAT_ERROR);
    }
    return (s);
}

/*{
** Name: dm1bxinsert - Insert a (key,tid) pair into a BTREE index page.
**
** Description:
**      This routine inserts an index record into an index page.
**      The record is a (key,tid) pair.  The position of where to
**      insert the record is indicated with a line table identifier
**      of where to place this record.
**
**	If index compression is in use, the index entry passed to this routine
**	should be UNCOMPRESSED. This routine will compress the entry if needed
**	before inserting it on the page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**					MAY BE NULL if no rcb available, in
**					which case the update mode is assumed
**					to be direct.
**	tcb				Pointer to table control block. Even
**					when rcb is NULL, tcb must be
**					supplied.
**	rac				DMP_ROWACCESS for leaf or index entry,
**					contains attr info, etc
**	klen				Length of key entry for this table.
**					This length should NOT include the tid
**					portion of the (key,tid) pair. We will
**					allocate an additional TidSize amount
**					of space in this routine.
**      buffer                          Pointer to the page to format.
**      key                             Pointer to an area containing the
**                                      key part of record to insert.
**      tid                             Pointer to an area containing
**                                      the tid part of the record.
**	partno				The TID's partition number
**					if Global Secondary index leaf.
**      child                           The LINEID part of a TID used
**                                      to indicate where on this page the
**                                      insertion is to take place.
**
** Outputs:
**	err_code			Set to an error code if an error occurs
**
**	Returns:
**
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-oct-85 (jennifer)
**          Converted for Jupiter.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression. Added atts_array
**	    atts_count, compression_type, and err_code arguments.
**	    Compress the entry, if needed, before inserting it on the page.
**	13-jul-1992 (kwatts)
**	    MPF changes. Added the tcb as a parameter that must always be
**	    present.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	13-jan-1993 (mikem)
**	    Fixed "used before set error" in dm1bxinsert().  "status" was
**	    being returned in the E_DM93B9_BXINSERT_ERROR error case at top
**	    of function before it was ever initialized; changed it to return
**	    E_DB_ERROR instead.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-apr-1993 (jnash)
**	    LGreserve space for only for the keylen used.
**	17-may-1993 (rogerk)
**	    Changed some of the error handling logic to give trace back
**	    messages and to pass external errors back up to the caller.
**	    Added new error messages.
**	21-jun-1993 (rogerk)
**	    Removed key copy from btput CLRs.  Take this into account when
**	    reserving space for the btput log record.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp) (bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	06-may-1996 (nanpr01)
**	    Changed all page header access as macro for New Page Format
**	    project.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**      24-jan-97 (dilma04)
**          Fix bug 80267: Do not set DM0L_K_ROW_LOCK flag if page size is 2k.
**      27-feb-97 (stial01)
**          dm1bxinsert() DO LGreserve space for key in CLR if row locking
**      21-apr-97 (stial01)
**          If row locking and bid reserved, don't validate insert request 
**          with dm1cxhas_room, don't allocate space with dm1cxallocate
**      21-may-97 (stial01)
**          dm1bxinsert() Added flags arg to dm0p_unmutex call(s).
**      17-jun-97 (stial01)
**          dm1bxinsert() set DM0L_BT_UNIQUE for unique indexes
**      18-jun-97 (stial01)
**          dm1bxinsert() Do not set DM0L_ROW_LOCK when updating index pages
**      29-jul-97 (stial01)
**          dm1bxinsert() Removed some unecessary reposition logic
**          Added consistency check to validate child position
**      22-aug-97 (stial01)
**          Skip dm1bxsearch consistency check when inserting into index pages
**      21-apr-98 (stial01)
**          dm1bxinsert() Set DM0L_PHYS_LOCK if extension table (B90677)
**      07-jul-98 (stial01)
**          dm1bxinsert() Deferred update: new update_mode if row locking
**      26-aug-98 (stial01)
**          dm1bxinsert() Added more tidp,tranid validity checks for reserved
**          leaf entries.
**      24-sep-98 (stial01)
**          dm1bxinsert() Fixed validity checks added 26-aug-98
**	26-Feb-2005 (schka24)
**	    dm1bxreserve doesn't put a partition number into the reserved
**	    entry tid (large page), so don't check it when the reservation
**	    is checked.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
DB_STATUS
dm1bxinsert(
    DMP_RCB	    *rcb,
    DMP_TCB	    *tcb,
    i4		    page_type,
    i4		    page_size,
    DMP_ROWACCESS   *rac,
    i4	    klen,
    DMP_PINFO	    *bufferPinfo,
    char	    *key,
    DM_TID	    *tid,
    i4		    tid_partno,
    DM_LINE_IDX	    child,
    i4	    log_updates,
    i4	    mutex_required,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_DCB		*d = tcb->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &tcb->tcb_table_io;
    DMPP_PAGE		*b;
    DM_TID		temptid; 
    i4			temppartno;
    i4			compression_type;
    DM_TID		bid; 
    DB_STATUS		status = E_DB_OK;
    i4                  compare = DM1B_SAME;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    i4			update_mode;
    i4			loc_config_id;
    i4			loc_id;
    char		*Ckey, *AllocCkey = NULL;
    char		compressed_key[DM1B_MAXSTACKKEY];
    i4			crec_size;
    DB_TRAN_ID		*tran_id;
    u_i2		lg_id;
    LG_LSN		lsn;
    i4			sequence_number;
    i4			reserve_size;
    bool		index_update;
    bool                ovfl_update;
    bool		logging_needed;
    bool		mutex_done;
    bool		critical_section;
    DM_TID              localtid;
    i4			localpartno;
    DM_LINE_IDX         check_pos;
    i4             	mode;
    char                *save_key = key; /* Save ptr to uncompressed key */
    u_i4		row_low_tran = 0;
    u_i2		row_lg_id;
    i2			TidSize;
    i4		    *err_code = &dberr->err_code;
    LG_LRI		lri;

    b = bufferPinfo->page;

    /*
    ** This routine is sometimes called without an RCB argument (god knows
    ** somebody has to keep the patch release groups in business).  If the
    ** logging or mutex options are specified, this better not be one of
    ** those cases.  If it is, we will certainly blow up.
    */
    if (((r == NULL) && ((log_updates) || (mutex_required))) ||
	    (r && DMPP_VPT_IS_CR_PAGE(page_type, b)))
    {
	TRdisplay("dm1bxinsert called with zero rcb: bad arguments\n");
	SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
	return (E_DB_ERROR);
    }

    /* set up dm1cx-style "compression type", which isn't, but whatever... */
    compression_type = DM1CX_UNCOMPRESSED;
    if (rac->compression_type != TCB_C_NONE)
	compression_type = DM1CX_COMPRESSED;

    /* Extract the Tid size from the page */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, b);

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_INDEX) != 0);
    ovfl_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_CHAIN) != 0);
    logging_needed = ((log_updates) && (r->rcb_logging));


    /*
    ** Since some of the error paths in this routine do not set an error_code,
    ** we zero out the err_code here so that we can tell when it has a valid
    ** value.
    */
    CLRDBERR(dberr);

    /*
    ** If inserting to a parent index node as part of a split operation,
    ** save the page pointer at the current insert position.  An insert
    ** to a non-Leaf page actually updates two entries on the page.
    ** The new key is inserted to (possibly the middle of) the page, and
    ** all entries after it are moved down.  But the new page pointer (TID)
    ** is inserted into the next position, as in this example:
    **
    **             Before Insert                      After Insert
    **
    **   pos 1   KEY (20)  TID (13,0)             KEY (20)  TID (13,0)
    **   pos 2   KEY (30)  TID (15,0)             KEY (25)  TID (15,0)
    **   pos 3   KEY (40)  TID (18,0)             KEY (30)  TID (21,0)
    **   pos 4    ------------------              KEY (40)  TID (18,0)
    */
    if (index_update)
    {
	/*
	** Save tid of the original page that was split (i.e. one inserting
	** next to).  This value needs to be placed with new
	** key.
	*/
        dm1cxtget(page_type, page_size, b, child, &temptid, &temppartno);
    }


    /*
    ** Get session/statement information for the following updates.
    ** If called during LOAD operations (no rcb available) then no
    ** transaction id or sequence number is written to the page.
    */
    if (r)
    {
	if (r->rcb_update_mode == RCB_U_DEFERRED)
	    update_mode = DM1C_DEFERRED;
	else
	    update_mode = DM1C_DIRECT;

	/* Set DM1C_ROWLK if row locking */
	if (!index_update && tcb->tcb_rel.relpgtype != TCB_PG_V1 &&
	    (row_locking(r) || crow_locking(r) || tcb->tcb_extended))
		update_mode |= DM1C_ROWLK;

	tran_id = &r->rcb_tran_id;
	lg_id = r->rcb_slog_id_id;
	sequence_number = r->rcb_seq_number;
    }
    else
    {
	update_mode = 0;
	tran_id = (DB_TRAN_ID *)NULL;
	lg_id = 0;
	sequence_number = 0;
    }

    /*
    ** If the index uses compressed key entries then compress the key
    ** to be inserted.
    */
    if (compression_type != DM1CX_UNCOMPRESSED)
    {
	if ( status = dm1b_AllocKeyBuf(klen, compressed_key,
				    &Ckey, &AllocCkey, dberr) )
	    return(status);
	status = dm1cxcomp_rec(rac, key, klen,
				Ckey, &crec_size );
	if (status != E_DB_OK)
	{
	    /* Discard any allocated key buffer */
	    if ( AllocCkey )
		dm1b_DeallocKeyBuf(&AllocCkey, &Ckey);
	    dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP, r, b, 
		page_type, page_size, child );
	    SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
	    return (status);
	}

	key = Ckey;
	klen = crec_size;
    }

    /*
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b));
    loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
    bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b);
    bid.tid_tid.tid_line = child;

    /* TIDs of Clustered Leaf entries are the entry's BID */
    if ( DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, b) & DMPP_CLUSTERED )
	tid->tid_i4 = bid.tid_i4;

    /*
    ** Validate the insert request.
    ** It is very bad to find out AFTER having logged the update that the 
    ** row cannot be inserted. Note that we do not expect such a condition
    ** to occur here (it would indicate a bug), but if we catch it before
    ** the dm0l call then the effect of the bug will be to cause the query
    ** to fail rather than to cause the database to go inconsistent.
    **
    ** (note that the validation of child value below occurs before the child
    ** bt_kids value is incremented to include the new row being inserted, so
    ** we check for child > bt_kids rather than >=)
    ** 
    ** This check is bypassed when there is no rcb.  In this case we are 
    ** being called from LOAD and the tcb information is not correct (#!%*&@).
    */
    if (r)
    {
	if ((index_update == FALSE) &&
	    DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC))
	{
	    char            tmpkeybuf[DM1B_MAXSTACKKEY];
	    char            *tmpkey, *AllocKbuf;
	    i4         	    tmpkey_len;
	    DM_TID          tmptid;
	    i4		    tmppartno;
	    ADF_CB          *adf_cb = r->rcb_adf_cb;
	    DB_STATUS       get_status, cmp_status;

	    /*
	    ** One last check to make sure this key matches 
	    ** reserved key, reserved tid, reserved tranid.
	    ** dm1bxreserve doesn't put a partition number into the
	    ** reserved tid, don't check that.
	    */
	    get_status = dm1b_AllocKeyBuf(tcb->tcb_klen, tmpkeybuf,
					&tmpkey, &AllocKbuf, dberr);
	    tmpkey_len = tcb->tcb_klen;
	    if ( get_status == E_DB_OK )
		get_status = dm1cxget(page_type, page_size, b,
		    rac,
		    child, &tmpkey, (DM_TID *)&tmptid, &tmppartno,
		    &tmpkey_len, &row_low_tran, &row_lg_id, adf_cb);
	    if (get_status != E_DB_ERROR)
		cmp_status = adt_kkcmp(adf_cb, tcb->tcb_keys, 
		    tcb->tcb_leafkeys, save_key, tmpkey, &compare);

	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &tmpkey);
		 
	    if (get_status != E_DB_WARN || cmp_status != E_DB_OK ||
		compare != DM1B_SAME)
	    {
		status = E_DB_ERROR;
	    }
	    else if (tmptid.tid_tid.tid_page != 0 && 
		tmptid.tid_i4 != r->rcb_pos_info[RCB_P_ALLOC].tidp.tid_i4)
	    {
		status = E_DB_ERROR;
	    }
	    else if ( (row_locking(r) || crow_locking(r))
			&& row_low_tran != r->rcb_tran_id.db_low_tran)
	    {
		status = E_DB_ERROR;
	    }		
	    
	    if (status != E_DB_OK)
	    {
		TRdisplay("Key not found %d %d %d (%d,%d %d) (%d) (%d)\n",
			get_status, cmp_status, compare,
			tmptid.tid_tid.tid_page, tmptid.tid_tid.tid_line,
			tmppartno,
			row_low_tran, r->rcb_tran_id.db_low_tran);
	    }
	}
	else
	{
	    /*
	    ** Check that the space exists for the requested entry
	    */
	    if ((klen < 0) || (klen > DM1B_MAXLEAFLEN) ||
		((i4)child > 
		    (i4) DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b)) ||
		(dm1cxhas_room(page_type, page_size, b,
		    compression_type, (i4)100, 
		    index_update ? tcb->tcb_kperpage : tcb->tcb_kperleaf,
		    klen + TidSize) == FALSE))
		status = E_DB_ERROR;

	    /*
	    ** Leaf page: Make sure we are inserting at the correct place
	    **
	    ** If inserting onto leaf overflow (CHAIN), the position was
	    ** not determined with dm1bxsearch, it is just added at the end
	    ** 
	    ** If inserting onto an index page during split processing,
	    ** the insert position was determined in btree_search,
	    ** using the key being inserted by the user. However we are
	    ** inserting the split descriminator key and its insert position
	    ** may differ if there are duplicate keys on index pages.
	    ** The insert position for the descriminator key may
	    ** be at the beginning of duplicate keys, while the 
	    ** insert position for the key being inserted to the leaf
	    ** may be a the end of the duplicate keys.
	    ** Either position is correct, but they will not match.
	    **
	    ** DM1B_DUPS_ON_OVFL == FALSE
	    ** dups span leaf pages not overflow pages, so there can be 
	    ** duplicates on index pages)
	    */
	    if ((ovfl_update == FALSE) && (index_update == FALSE))
	    {
		status = dm1bxsearch(r, b, DM1C_OPTIM, DM1C_EXACT, save_key,
		    tcb->tcb_keys, &localtid, &localpartno,
		    &check_pos, NULL, dberr);
		if (DB_FAILURE_MACRO(status) && (dberr->err_code == E_DM0056_NOPART))
		{
		    status = E_DB_OK;
		    CLRDBERR(dberr);
		}
		if (check_pos != child)
		    status = E_DB_ERROR;
	    }
	}

	if (status == E_DB_ERROR || compare != DM1B_SAME)
	{
	    TRdisplay("E_DM9C1E_BXKEY_ALLOC status %d compare %d\n", status,
		compare);
	    uleFormat(NULL, E_DM9C1E_BXKEY_ALLOC, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		0, child, 0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, b),
		0, DM1B_VPT_GET_BT_KIDS_MACRO(page_type, b), 0, klen);
	    dmd_prindex(rcb, b, 1);
	    SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
	    return (E_DB_ERROR);
	}
    }

    mutex_done = FALSE;
    critical_section = FALSE;
    for (;;)
    {
	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	** Bypass when insert is called by build routines.
	*/
	if ((r) && (d->dcb_status & DCB_S_BACKUP))
	{
	    status = dm0p_before_image_check(r, b, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Reserve logfile space for the insert record and its CLR.  This
	** ensures that space will be available for abort processing of
	** this update.  We reserve the space before mutexing the page to
	** avoid stalling (if logfull) while holding any page mutexes.
	*/
	if (logging_needed)
	{
	    reserve_size = 2 * (sizeof(DM0L_BTPUT) - (DM1B_MAXLEAFLEN - klen));

	    /*
	    ** Don't need space for key in the CLR if page locking
	    ** However for now we'll always put it there
	    **
	    **  if (!(row_locking(r)))
	    **      reserve_size -= klen;
	    */

	    cl_status = LGreserve(0, r->rcb_log_id, 2, reserve_size, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** If the buffer resides in the DBMS buffer cache, then we have to mutex
	** it while we change it to prevent other threads from looking at it
	** while it is inconsistent.
	*/
	if (mutex_required)
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, bufferPinfo);
	    mutex_done = TRUE;
	}

	/*
	** Log the Insert to the Btree Index Page.
	** The LSN of the log record must be written to the updated page.
	*/
	if (logging_needed)
	{
	    i4 dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);
	    u_i4 dm0l_btflag = 0;

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs, extension tables
	    ** and index pages. The same lock protocol must be observed during 
	    ** recovery.
	    */
	    if ( NeedPhysLock(r) || index_update )
		dm0l_flag |= DM0L_PHYS_LOCK;
	    else if (row_locking(r))
		dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
		dm0l_flag |= DM0L_CROW_LOCK;

	    if (tcb->tcb_rel.relstat & TCB_UNIQUE)
		dm0l_btflag |= DM0L_BT_UNIQUE;
	    if (tcb->tcb_dups_on_ovfl)
		dm0l_btflag |= DM0L_BT_DUPS_ON_OVFL;
	    if (tcb->tcb_rng_has_nonkey)
		dm0l_btflag |= DM0L_BT_RNG_HAS_NONKEY;

	    if (!index_update && DMZ_TBL_MACRO(23))
	    {
		status = dbg_dm723(r, &bid, tid, dm0l_flag, err_code);
		if (status != E_DB_OK)
		{ 
		    SETDBERR(dberr, 0, *err_code);
		    break;
		}
	    }

	    /* Get CR info from page, pass to dm0l_btput */
	    DM1B_VPT_GET_PAGE_LRI_MACRO(page_type, b, &lri);

	    status = dm0l_btput(r->rcb_log_id, dm0l_flag, &tcb->tcb_rel.reltid,
		&tcb->tcb_rel.relid, tcb->tcb_relid_len,
		&tcb->tcb_rel.relowner, tcb->tcb_relowner_len,
		tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize,
		tcb->tcb_index_rac.compression_type, 
		tcb->tcb_rel.relloccount, loc_config_id,
		&bid, child, tid, klen, key, (LG_LSN *) 0, &lri, 
		tid_partno, dm0l_btflag, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Now that we have logged the update, we have committed ourselves
	    ** to being able to process the action.  Any failure now causes us
	    ** to crash the server to let REDO recovery attempt to fix the db.
	    */
	    critical_section = TRUE;

	    /*
	    ** Write the LSN and other info about the insert record to the page
	    ** being updated.
	    ** This marks the version of the page for REDO processing and
	    ** CR undo.
	    */
	    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, b, &lri);
	}
	else if (!index_update)
	    DM1B_VPT_INCR_BT_CLEAN_COUNT_MACRO(page_type, b);

	/*
	** Allocate space on the page for the new key,tid pair.
	*/
	if ( (index_update == TRUE) || (r == NULL) 
	    || DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC) == FALSE)
	{
	    status = dm1cxallocate(page_type, page_size, b,
			update_mode, compression_type, tran_id,
		        sequence_number, child, klen + TidSize );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, r, b,
			page_type, page_size, child );
		break;
	    }
	}

	/*
	** Insert the new key,tid,partno values.
	*/
	status = dm1cxput( page_type, page_size, b,
		    compression_type, update_mode, tran_id, lg_id,
		    sequence_number, child, key, klen, tid, tid_partno );
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, r, b, 
		    page_type, page_size, child);
	    break;
	}

	/*
	** If the insert is to a non-leaf index page then adjust the TID
	** pointers on the new entry and the old one which occupied its
	** spot (as described above).
	*/
	if (index_update)
	{
	    /*
	    ** The tidp of the old 'child' record, saved in temptid, goes 
	    ** with the new child record we just inserted.  The tidp passed 
	    ** to us goes with the (child+1) entry.
	    */
	    status = dm1cxtput(page_type, page_size, b, child + 1, 
				tid, tid_partno );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93EB_BAD_INDEX_TPUT, r, b,
			page_type, page_size, child + 1 );
		break;
	    }
	    status = dm1cxtput(page_type, page_size, b, child, 
				&temptid, temppartno );
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93EB_BAD_INDEX_TPUT, r, b,
			    page_type, page_size, child );
		break;
	    }
	}
	else if (r && (r->rcb_update_mode == RCB_U_DEFERRED || crow_locking(r)))
	{
	    /*
	    ** Do deferred put proessing if crow_locking so we can
	    ** ignore changes by our own transaction without mvcc undo
	    */
	    critical_section = FALSE;
	    status = dm1cx_dput(r, b, child, tid, err_code);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E4_BAD_INDEX_PUT, r, b, 
			page_type, page_size, child );
		break;
	    }
	}

	/*
	** Unmutex the leaf page now that we are finished updating it.
	*/
	if (mutex_done)
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, bufferPinfo);

	return (E_DB_OK);
    }

    /*
    ** An error occurred while updating the index page leaving the page's
    ** contents possibly trashed.  If we are logging updates, then we're
    ** hosed.  We crash the server and hope that REDO recovery will correctly
    ** rebuild the page contents.  Note that our inconistent page cannot
    ** be written to the database (since we crash while holding the mutex).
    **
    ** If we are not logging (just doing a load operation), then we can
    ** return an error and presumably let the trashed pages be tossed.
    */

    if (critical_section)
	dmd_check(E_DM93B9_BXINSERT_ERROR);

    if (mutex_done)
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, bufferPinfo);


    if ((dberr->err_code == 0) || (dberr->err_code > E_DM_INTERNAL))
    {
	if (dberr->err_code)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
	SETDBERR(dberr, 0, E_DM93B9_BXINSERT_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1bxjoin - Joins index pages.
**
** Description:
**      This routine joins two BTREE sibling index or overflow pages. 
**      The right sibling is merged into the left. 
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      parent                          Pointer to the parent of page 
**                                      spliting.
**      pos 			        The LINEID part of a TID which 
**                                      indicates the position in the parent
**                                      that points to this leaf page.
**      current                         Pointer to current leaf page to split.
**      sibling                         Pointer to page to split to.
**      mode                            A value indicating type of join to
**                                      perform, must be DM1B_SIBJOIN or
**                                      DM1B_OVFJOIN.
**
** Outputs:
**	err_code			Set to an error code if an error occurs
**
**	Returns:
**
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-oct-85 (jennifer)
**          Converted for Jupiter.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression. Added err_code
**	    argument.
**	10-sep-1991 (bryanp)
**	    B37642,B39322: Check for infinity == FALSE on LRANGE/RRANGE entries.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Rewrote with new logging protocols.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	16-feb-1993 (jnash)
**	    Move LGreserve prior to dm0l_bi() to fix mutexing problem.
**	30-feb-1993 (rmuth)
**	    Whilst logging the after image the code should perform the 
**	    following sequence, dm0p_unmutex; LGreserve; dm0p_mutex; log rest;
**	    instead the dm0p_mutex call was a dm0p_unmutex call causing an
**	    error, change to dm0p_mutex.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	    Subtract tuple header size from index entry length for leaf pages.
**      01-may-1996 (stial01)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**          Pass reclaim_space = TRUE to dm1bxdelete()
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**	21-jan-1997 (i4jo01)
**	    Recognize overflow leaf pages when doing overflow joins, so
**	    that they are not treated as index pages (b80089).
**      21-may-97 (stial01)
**          dm1bxjoin() Added flags arg to dm0p_unmutex call(s).
**      18-jun-97 (stial01)
**          dm1bxjoin() Do not set DM0L_ROW_LOCK when updating index pages
*/
DB_STATUS
dm1bxjoin(
    DMP_RCB         *rcb,
    DMP_PINFO	    *parentPinfo,
    i4	    pos,
    DMP_PINFO      *currentPinfo,
    DMP_PINFO      *siblingPinfo,
    i4	    mode,
    DB_ERROR	    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_DCB		*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_PAGE		**par;
    DMPP_PAGE		**cur;
    DMPP_PAGE		**sib;
    LG_LSN		lsn;
    DM_TID		curtid;
    i4			curpartno;
    DM_TID		infinity; 
    char        	*key_ptr;
    i4		key_len;
    i4		child;
    i4		compression_type;
    i4		dm0l_flag;
    i4		loc_id;
    i4		cur_loc_cnf_id;
    i4		sib_loc_cnf_id;
    DB_STATUS		s = E_DB_OK;
    STATUS		cl_status;
    bool		pages_mutexed = FALSE;
    bool		leaf_join;
    CL_ERR_DESC	    	sys_err;
    i4             reclaim_space;
    i2			TidSize;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if ( parentPinfo )
	par = &parentPinfo->page;
    else
        par = NULL;

    cur = &currentPinfo->page;
    sib = &siblingPinfo->page;

    compression_type = DM1B_INDEX_COMPRESSION(r);
    leaf_join = ((DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, *cur) & 
			(DMPP_LEAF|DMPP_CHAIN)) != 0);

    /* Extract the TID size from the page */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(tbio->tbio_page_type, *cur);

    /*
    ** The last entry on an index page has a pageno field, but no key
    ** field. When we join two index pages, the entry which was formerly
    ** the last entry on the current page will no longer be the last
    ** entry. Therefore, we must set its key value properly. We do this
    ** by copying the parent's value down to this entry, preserving the
    ** pageno portion of the entry on 'cur'.
    */
    if (! leaf_join)
    {
	key_len = t->tcb_ixklen;
	if (compression_type == DM1CX_COMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, *sib, pos, &key_len);
	}
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, *par, pos, &key_ptr);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, *cur,
		  (DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, *cur) - 1),
		  &curtid, &curpartno);
    }

    /*
    ** Get information on the locations to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, *cur));
    cur_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, *sib));
    sib_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;

    for (;;)
    {
	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, *cur, dberr);
	    if (s != E_DB_OK)
	        break;
	    s = dm0p_before_image_check(r, *sib, dberr);
	    if (s != E_DB_OK)
	        break;
	}

	/************************************************************
	**            Log the Join operation.
	*************************************************************
	**
	** A JOIN operation is logged in two phases:  Before Image
	** Log Records are written prior to updating the page.  These
	** are used only for UNDO processing.
	**
	** After Image Log Records are written after the operation is 
	** complete to record the final contents of the pages.
	** These log records must be written BEFORE the mutexes are
	** released on the pages.  The LSNs of these log records are
	** written to the page_log_addr fields as well as to their split
	** addresses.
	*/

	if (r->rcb_logging & RCB_LOGGING)
	{
	    /* Reserve log file space for 2 BIs and their CLRs  */
	    cl_status = LGreserve(0, r->rcb_log_id, 4,
		4*(sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		break;
	    }

	    /*
	    ** Mutex the pages before beginning the udpates.  This must be done 
	    ** prior to the writing of the log record to satisfy logging 
	    ** protocols.
	    **
	    ** Note that only the current and sibling are updated as part of
	    ** this "operation".  The update to the parent page is done below
	    ** and is logged and recovered separately.
	    */

	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, siblingPinfo);


	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    if ( ! leaf_join)
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
	        dm0l_flag |= DM0L_CROW_LOCK;

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, cur_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *cur), 
		t->tcb_rel.relpgsize,
		*cur,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, *cur, lsn);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, sib_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *sib), 
		t->tcb_rel.relpgsize,
		*sib,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, *sib, lsn);
	}
	else
	{
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, siblingPinfo);
	}

	/*
	** Update the key value of the last entry on the current page
	** if it is a non-leaf page.  The last entry of an index page has
	** a special format (no key) and after this join, this entry will
	** no longer be last.
	*/
	if ( ! leaf_join)
	{
	    child = DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, *cur) - 1;
	    s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, *cur,
			DM1C_DIRECT, compression_type,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, child,
			key_ptr, key_len, &curtid, curpartno);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, r, *cur, 
			tbio->tbio_page_type, tbio->tbio_page_size, child);
		SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		break;
	    }
	}

	/*
	** Now we shift all the entries from the sibling page over to the
	** current page, and adjust cur->bt_kids appropriately.
	*/
	s = dm1cxlshift(tbio->tbio_page_type, tbio->tbio_page_size, *cur, *sib,
			    compression_type,
			    leaf_join ? t->tcb_klen + TidSize :
			    t->tcb_ixklen + TidSize);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E8_BAD_INDEX_LSHIFT, (DMP_RCB *)r, *cur, 
			tbio->tbio_page_type, tbio->tbio_page_size, 0 );
	    dm1cxlog_error( E_DM93E8_BAD_INDEX_LSHIFT, (DMP_RCB *)r, *sib, 
			tbio->tbio_page_type, tbio->tbio_page_size, 0 );
	    SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
	    break;
	}


	/*
	** For Leaf Page joins, the range keys and sideways pointers must
	** be updated.
	*/
	if ((mode == DM1B_SIBJOIN) && (leaf_join))
	{
	    /*
	    ** The new RRANGE for the current page must now be set to the old 
	    ** RRANGE from the sibling.
	    */
	    key_len = t->tcb_rngklen;
	    if (compression_type == DM1CX_COMPRESSED)
	    {
		dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, *sib, 
			DM1B_RRANGE, &key_len);
	    }
	    dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, *sib, 
			DM1B_RRANGE, &key_ptr);
	    dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, *sib, 
			DM1B_RRANGE, &infinity, (i4*)NULL);

	    s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, *cur,
			    DM1C_DIRECT, compression_type, (DB_TRAN_ID *)NULL,
			    (u_i2)0, (i4)0, DM1B_RRANGE, key_ptr, key_len, 
			    &infinity, (i4)0);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, *cur,
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_RRANGE);
		SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		break;
	    }

	    /*
	    ** Remove 'sib' from the linked list of leaf page sideways ptrs:
	    */
	    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, *cur,
			DM1B_VPT_GET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, *sib));
	    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, *sib, 0);
	}

	/*
	** For overflow page joins, the sibling must be unlinked from the 
	** overflow chain.
	*/
	if (mode != DM1B_SIBJOIN)
	{
	    DM1B_VPT_SET_PAGE_OVFL_MACRO(tbio->tbio_page_type, *cur,
			DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, *sib)); 
	}

	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, *cur, DMPP_MODIFY);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, *sib, DMPP_MODIFY);

	/************************************************************
	**            Log the Join operation.
	*************************************************************
	**
	** Complete the Logging for the operation by logging the After
	** images of the updated pages.  These will be used during forward
	** (redo,rollforward) recovery to reconstruct the pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    if (pages_mutexed)
	    {
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, siblingPinfo);
		pages_mutexed = FALSE;
	    }

	    /* Reserve log file space for 2 AIs */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		2*(sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
		break;
	    }

	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, siblingPinfo);
	    pages_mutexed = TRUE;

	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    if ( ! leaf_join)
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, cur_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *cur), 
		t->tcb_rel.relpgsize,
		*cur,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, sib_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, *sib), 
		t->tcb_rel.relpgsize,
		*sib,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, *cur, lsn);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, *sib, lsn);

	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, *cur, lsn);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, *sib, lsn);
	}

	/*
	** Unmutex the pages since the update actions described by
	** the newroot log record are complete.
	**
	** Following the unmutexes, these pages may be written out
	** to the database at any time.
	*/
	pages_mutexed = FALSE;
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, siblingPinfo);

	/*
	** As part of a sibling join, we must combine the parent page's
	** entries for the two pages which were just joined. The new parent
	** page entry will point to 'cur', but will contain the keyspace for
	** both 'cur' and 'sib'. This processing is handled by dm1bxdelete().
	*/
	if (mode == DM1B_SIBJOIN)
	{
	    s = dm1bxdelete(r, parentPinfo, (DM_LINE_IDX) (pos+1), 
		    (i4)TRUE, (i4)TRUE, &reclaim_space, dberr); 
	    if (s != E_DB_OK)
		break;
	}

	return (E_DB_OK);
    }

    /*
    ** Error Handling.
    */
    if (pages_mutexed)
    {
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, siblingPinfo);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93BB_BXJOIN_ERROR);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1bxnewroot - Creates a new BTREE root do to split.
**
** Description:
**      This routines takes the old root, moves its data to a new
**      page and points root to it thus creating a new root with 
**      only one entry.  It is assumed that the root of the tree
**      is locked by the caller.  The header page is locked and 
**      unlocked at the beginning and end of this routine assuring 
**      no other thread(user) can be splitting root at same time.
**      This also serializes access, therefore no other pages need
**      to be locked during split.  It is also assumed that
**      only the root is fixed upon entry and exit.
**
**      The split occurs as follows:
**      New page is obtained, free list information and page number
**      of new page is saved for recovery.  Old root is copied to
**      new page.  New page is updated with it correct page number.
**      New page number and saved information is logged for recovery.
**      New page and header are written.
**      Root is updated to indicate it has one record which points
**      to new page.  Root is written.  Log indicating end of split
**      is written.  Old root is now updated and is new root.
**      New page is it's only child and has all old information
**      on it.
**
**	NOTE: Prior to the index compression project, when we wanted to update
**	the root page to indicate it had exactly one entry which pointed to
**	the new page, all we had to do was set bt_kids to 1 and update the
**	tid portion of entry 0 to point to the new page (the key portion of
**	entry 0 doesn't matter -- the last key on an index page need not be
**	set). However, as part of the index compression project this algorithm
**	had to be changed. We must now explicitly delete each entry on the
**	page, then allocate a new entry and set the tid portion of the new
**	entry to point to the new page. This is considerably more involved;
**	luckily, dm1bxnewroot is only called rarely.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      root                            Pointer to a pointer to current
**                                      root page.
**
** Outputs:
**      err_code                        Pointer to an area used to return
**                                      error codes if return value not
**                                      E_DB_OK.  Error codes returned are:
**                                      E_DM
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
**	    none
**
** History:
**      17-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	28-jan-1991 (bryanp)
**	    Added support for Btree index compression. Instead of simply
**	    resetting bt_kids to 1 and putting the new page's pageno into the
**	    tid portion of entry zero, we now explicitly delete each entry on
**	    the page, then allocate a new entry 0 and set its tid to point to
**	    the new page. This process recovers the space on a compressed page.
**	19-aug-1991 (bryanp)
**	    Merged allocation project changes in.
**	25-oct-1991 (bryanp)
**	    Since the newroot operation is performed in a mini-transaction,
**	    we must ensure that the page allocation is permanent before we
**	    complete the mini-transaction. Call dm1p_force_map to do so.
**	20-Nov-1991 (rmuth)
**	    Added DM1P_MINI_TRAN, when inside a mini-transaction pass this
**	    flag to dm1p_getfree() so it does not allocate a page deallocated
**	    earlier by this transaction. see dm1p.c for explaination why.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Fixes. Added dm0p_pagetype call.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	18-feb-1993 (walt)
**	    Defined a i4 'n' for the CSterminate call.  (The call was 
**	    possibly cut and pasted from dm1bxsplit which has an 'n' defined 
**	    for it.)
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jul-1993 (rogerk)
**	    Removed unused update_mode flag.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp) (bryanp)
**	    Use dm0m_lcopy to copy pages around, since they may be larger than
**		can be copied by MEcopy.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format 
**	    project.
**      01-may-1996 (stial01)
**          LGreserve adjustments for removal of bi_page from DM0L_BI
**          LGreserve adjustments for removal of ai_page from DM0L_AI
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Pass reclaim_space = TRUE to dm1cxdel()
**      27-feb-97 (stial01)
**          Init flags parameter for dm1cxdel.
**      21-may-97 (stial01)
**          dm1bxnewroot() Added flags arg to dm0p_unmutex call(s).
*/
DB_STATUS
dm1bxnewroot(
    DMP_RCB	*rcb, 
    DMP_PINFO	*rootPinfo,
    DB_ERROR    *dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *newroot = (DMPP_PAGE *)NULL; 
    DMPP_PAGE	    *newpage, *rootpage; 
    DMP_PINFO	    newrootPinfo;
    DM_TID	    son; 
    DM_PAGENO	    avail; 
    DB_STATUS	    s = E_DB_OK;
    STATUS	    cl_status;
    LG_LSN	    prev_lsn;
    LG_LSN	    lsn;
    i4	    compression_type;
    i4	    dm0l_flag;
    i4	    alloc_flag;
    i4	    child;
    i4	    loc_id;
    i4	    root_loc_cnf_id;
    i4	    indx_loc_cnf_id;
    i4	    n;
    i4	    local_error;
    bool	    pages_mutexed = FALSE;
    CL_ERR_DESC	    sys_err;
    i2		    TidSize;
    i4		    partno;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    newrootPinfo.page = NULL;

    /*
    ** Note: root is already exclusively locked by caller.
    */

    if (DMZ_AM_MACRO(5))
    {
	(VOID) TRdisplay("in newroot: relation is %t\n", 
                       sizeof(DB_TAB_NAME), &t->tcb_rel.relid); 
	/* %%%% Place call to debug routine to print locks. */
    }

    compression_type = DM1B_INDEX_COMPRESSION(r);

    /* Extract partition number from indexed table or this table */
    if ( t->tcb_rel.relstat & TCB_INDEX )
	partno = t->tcb_parent_tcb_ptr->tcb_partno;
    else
	partno = t->tcb_partno;

    for (;;)
    {
	/*
	**************************************************************
	**           Start Split Transaction
	**************************************************************
	**
	** Splits are performed within a Mini-Transaction.
	** Should the transaction which is updating this table abort after
	** we have completed the split, then the split will NOT be backed
	** out, even though the insert which caused the split will be.
	**
	** We commit splits so that we can release the index page locks
	** acquired during the split and gain better concurrency.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    s = dm0l_bm(r, &lsn, dberr);
	    if (s != E_DB_OK)
		break;
	}
	STRUCT_ASSIGN_MACRO(lsn, prev_lsn);

	/*
	** Allocate the new root page.
	** The allocate call must indicate the it is performed from within
	** a mini-transation so that we will not be allocated any pages that
	** were previously deallocated in this same transaction.  (If we were
	** and our transaction were to abort it would be bad: the undo of the 
	** deallocate would fail when it found the page was not currently free
	** since the split would not have been rolled back.)
	*/
	alloc_flag = (DM1P_MINI_TRAN | DM1P_PHYSICAL);

	/*
	** dm1p_getfree may MLOCK newroot's buffer; it will be
	** unlocked by dm0p_unfix_page.
	*/
	s = dm1p_getfree(r, alloc_flag, &newrootPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	newroot = newrootPinfo.page;

	/* Inform buffer manager of new page's type */
	dm0p_pagetype(tbio, newroot, r->rcb_log_id, DMPP_INDEX);

	newpage = newroot; 
	rootpage = rootPinfo->page; 

	/*
	** Save the new page number and build TID entry for the sole root page
	** entry (which will point to the new index page).
	*/
	son.tid_tid.tid_page = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage); 
	son.tid_tid.tid_line = 0;

	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, 
						  rootpage));
	root_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		(i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, 
						  newpage));
	indx_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;


	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, rootpage, dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_before_image_check(r, newpage, dberr);
	    if (s != E_DB_OK)
		break;
	}


	/************************************************************
	**            Log the Newroot operation.
	*************************************************************
	**
	** A NEWROOT operation is logged in two phases:  Before Image
	** Log Records are written prior to updating the pages.  These
	** are used only for UNDO processing.
	**
	** After Image Log Records are written after the operation is 
	** complete to record the final contents of the pages.
	** These log records must be written BEFORE the mutexes are
	** released on the pages.  The LSNs of these log records are
	** written to the page_log_addr fields as well as to their split
	** addresses.
	*/

	if (r->rcb_logging & RCB_LOGGING)
	{
	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);
	    dm0l_flag |= DM0L_PHYS_LOCK;

	    /* 
	    ** Reserve log file space for 2 BIs and their CLRs
	    */
	    cl_status = LGreserve(0, r->rcb_log_id, 4,
		4*(sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
		break;
	    }

	    /*
	    ** Mutex the pages before beginning the udpates.  This must be done 
	    ** prior to the writing of the log record to satisfy logging 
	    ** protocols.
	    */
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, rootPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, root_loc_cnf_id,
		DM0L_BI_NEWROOT, 0, t->tcb_rel.relpgsize,
		rootpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, rootpage, lsn);

	    s = dm0l_bi(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, root_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newpage), 
		t->tcb_rel.relpgsize,
		newpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** "newpage" is new and has no page update history, so just 
	    ** record the bi lsn, leaving the rest of the LRI information
	    ** on the page zero.
	    */
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, newpage, lsn);

	}
	else
	{
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, rootPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
	}

	/*
	** Copy old root to the new page.
	*/
	MEcopy((PTR)rootpage, tbio->tbio_page_size, (PTR)newpage); 

	/* Extract size of TID */
	TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(tbio->tbio_page_type, newpage);

	/*
	** Make new page point to itself.
	*/
	DM1B_VPT_SET_PAGE_PAGE_MACRO(tbio->tbio_page_type, newpage, 
				 son.tid_tid.tid_page); 
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, newpage, DMPP_MODIFY);

	/* 
	** Delete all entries (other than the LRANGE/RRANGE entries) on the
	** root page, thus reclaiming space if this index is compressed.
	** Allocate a new first entry and set it to point to the new page.
	** At the end of this, the root page is updated to contain only one
	** child, which is the new page. The new page, in turn, points to all
	** the former children of the root.
	**
	** Note that the root page is now no longer a sprig (it may not have
	** been a sprig before).
	*/
	DM1B_VPT_CLR_PAGE_STAT_MACRO(tbio->tbio_page_type, rootpage, DMPP_SPRIG);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, rootpage, DMPP_MODIFY);

	while (DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, rootpage) > 0)
	{
	    child = DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, rootpage) - 1;
	    s = dm1cxdel(tbio->tbio_page_type, tbio->tbio_page_size, rootpage,
		    DM1C_DIRECT, compression_type,
		    (DB_TRAN_ID *) 0, (u_i2)0, (i4) 0, (i4)DM1CX_RECLAIM, child);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, r, rootpage, 
			tbio->tbio_page_type, tbio->tbio_page_size, child);
		SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
		break;
	    }
	}
	if (s != E_DB_OK)
	    break;

	/*
	** allocate a new entry to point to the new page and set the entry's
	** tid to point to the new page. The new entry only has to be long
	** enough to contain a TID -- no key value is needed.
	*/
	s = dm1cxallocate(tbio->tbio_page_type, tbio->tbio_page_size, rootpage,
			    DM1C_DIRECT, compression_type, (DB_TRAN_ID *)NULL, 
			    (i4)0, (i4)0, TidSize);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, r, rootpage, 
			tbio->tbio_page_type, tbio->tbio_page_size, (i4)0);
	    SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
	    break;
	}
	s = dm1cxtput(tbio->tbio_page_type, tbio->tbio_page_size, rootpage, 
		0, &son, partno);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, r, rootpage, 
			tbio->tbio_page_type, tbio->tbio_page_size, (i4)0);
	    SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
	    break;
	}

	/************************************************************
	**            Log the Newroot operation.
	*************************************************************
	**
	** Complete the Logging for the operation by logging the After
	** images of the updated pages.  These will be used during forward
	** (redo,rollforward) recovery to reconstruct the pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    if (pages_mutexed)
	    {
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, rootPinfo);
		dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
		pages_mutexed = FALSE;
	    }

	    /* Physical locks are used on btree index pages */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);
	    dm0l_flag |= DM0L_PHYS_LOCK;

	    /* Reserve log file space for 2 AIs */
	    cl_status = LGreserve(0, r->rcb_log_id, 2,
		2*(sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + t->tcb_rel.relpgsize),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
		break;
	    }

	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, rootPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
	    pages_mutexed = TRUE;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, root_loc_cnf_id,
		DM0L_BI_NEWROOT, 0, t->tcb_rel.relpgsize,
		rootpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    s = dm0l_ai(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relloccount, indx_loc_cnf_id,
		DM0L_BI_NEWROOT, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, newpage), 
		t->tcb_rel.relpgsize,
		newpage,
		(LG_LSN *) 0, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, rootpage, lsn);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, newpage, lsn);

	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, rootpage, lsn);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, newpage, lsn);
	}

	/*
	** Unmutex the pages since the update actions described by
	** the newroot log record are complete.
	**
	** Following the unmutexes, these pages may be written out
	** to the database at any time.
	*/
	pages_mutexed = FALSE;
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, rootPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);


#ifdef xDEV_TEST
	if (DMZ_CRH_MACRO(18) || DMZ_CRH_MACRO(22) || DMZ_CRH_MACRO(22))
	{
	    TRdisplay("dm1b_newroot: simulating crashing during root split\n");
	    TRdisplay("    For table %t in database %t.\n",
		   sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		   sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	    if (DMZ_CRH_MACRO(23))
	    {
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, rootPinfo, dberr);
		TRdisplay("Old Root page Forced.\n");
	    }
	    if (DMZ_CRH_MACRO(18))
	    {
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, rootPinfo, dberr);
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, &newrootPinfo, dberr);
		TRdisplay("New and Old Root pages Forced.\n");
	    }
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** End the Mini Transaction used during the Newroot operation.  After
	** this point the split cannot be undone.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    s = dm0l_em(r, &prev_lsn, &lsn, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Unfix the new index page and leave the root page (page 0) fixed
	** by the caller.
	*/
	s = dm0p_unfix_page(r, DM0P_RELEASE, &newrootPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1b_newroot: returning from newroot: son is %d\n",
			son.tid_tid.tid_page); 
	}

	return (E_DB_OK);
    }

    /*
    ** Error handling:
    **
    */

    if (pages_mutexed)
    {
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, rootPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &newrootPinfo);
    }

    if (newrootPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &newrootPinfo, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93BD_BXNEWROOT_ERROR);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1bxsearch - Search for a key or a key,tid pair on a page.
**
** Description:
**      This routines searches a BTREE index page for a key or 
**      key,tid pair.
**
**      This routine returns the line table position where the 
**      search ended.  It also assumes that the page pointed to by 
**      buffer is only one fixed on entry and exit.  This page
**      may change if search mode is DM1C_TID.
**
**      The semantics of this routine are determined by a combination of the
**	'mode' and 'direction' arguments. Most combinations of these arguments
**	are vestigial artifacts of old implementations. The following are the
**	only current legal combinations, together with their meaning:
**
**      DM1C_LAST/DM1C_NEXT         Find the last entry on the page
**	DM1C_EXTREME/DM1C_PREV	    Find the lowest entry on the page. Used to
**				    position to the start of the Btree for a
**				    scan.
**	DM1C_FIND/DM1C_EXACT	    Locate the position on this page where
**				    this entry is located, or the position where
**				    it would be inserted if it's not found. If
**				    the page is empty, or if this entry is
**				    greater than all entries on this page, then
**				    return E_DM0056_NOPART.
**	DM1C_SPLIT/DM1C_EXACT	    Same semantics as FIND/EXACT. Used when the
**				    caller is in a splitting pass.
**	DM1C_OPTIM/DM1C_EXACT	    Same semantics as FIND/EXACT. Used when the
**				    caller is in an optimistic allocation pass.
**	DM1C_RANGE/DM1C_PREV	    Same semantics as FIND/EXACT. Used when the
**				    caller is positioning the Btree for all
**				    entries >= a given key, or like a partial
**				    key.
**	DM1C_TID/DM1C_EXACT	    Locate a specific entry on the page. The
**				    entry is located by comparing the TIDP of
**				    the entry on the page with the passed-in
**				    TID. If necessary, the search will follow
**				    the page's overflow chain.
**
**	TID/EXACT only makes sense on a leaf page, whereas FIND/EXACT is used
**	for non-leaf index pages.
**
**	All other combinations of 'mode' and 'direction' are illegal and are
**	rejected.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      buffer                          Pointer to the page to search.
**      mode                            Value indicating type of search
**                                      to perform.  Must be DM1C_SPLIT,
**                                      DM1C_JOIN, DM1C_FIND, 
**                                      DM1C_EXTREME, DM1C_RANGE, 
**                                      DM1C_OPTIM, or DM1C_TID. 
**      direction                       Value to indicate the direction
**                                      of the search or range.  Must
**                                      DM1C_NEXT or DM1C_PREV or DM1C_EXACT
**      key                             Pointer to the key value.
**      keyatts                         Pointer to an array desribing the
**                                      attributes of the key.
**      keyno                           Value indicating number of 
**                                      attributes to use in the search. This is
**					not necessarily all of the attributes
**					in the actual index entry.
**	tid				If mode == DM1C_TID, this value is the
**					TID which we use to search for the
**					exact entry to return.
**
** Outputs:
**      pos                             Pointer to an area to return
**                                      the position of the line table 
**                                      where search ended.
**      tid                             Pointer to an area to return
**                                      the tid of the record pointed to
**                                      by line table position.
**	record				Optional pointer to an area to
**					return leaf entry.
**      err_code                        Pointer to an area to return 
**                                      the following errors when 
**                                      E_DB_ERROR is returned.
**                                      E_DM0056_NOPART
**					E_DM93BA_BXSEARCH_BADPARAM
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
**	    May alter the pages fixed if following leaf chain and get
**          fatal errors.
**
** History:
**      10-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	18-may-88 (teg)
**	    Added logic to decrement position if direction == DM1C_NEXT
**	14-Nov-88 (teg)
**	    added initialiation of adf_cb.adb_maxstring to DB_MAXSTRING
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression. Changed comments.
**	    Revised code to remove ancient, buggy code and support only that
**	    which is needed (e.g., no support for DM1C_NEXT)
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	      - Changed to not follow the leaf's overflow chain in search of 
**		matching rows in DM1C_TID mode.  The callers will now be 
**		required to call this routine again with the overflow pages 
**		to continue searching.
**	      - Changed the buffer pointer to be passed by value since it is 
**		no longer changed.
**      19-apr-1995 (stial01)
**          Added DM1C_LAST/DM1C_NEXT to support finding last entry on page
**          for MANMANX.
**	30-aug-1995 (cohmi01)
**	    Put back partial support for DM1C_NEXT for use by DMR_AGGREGATE.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Check for E_DB_WARNING return code from dm1cxget()
**      12-jun-97 (stial01)
**          dm1bxsearch() Pass tlv to dm1cxget instead of tcb.
**      26-aug-98 (stial01)
**          dm1bxsearch() if DM1C_TID/DM1C_EXACT and tidp is special reserved
**          tid, check tranid as well.
**	05-Jun-2006 (jenjo02)
**	    For DM1C_TID/DM1C_EXACT searches on Clustered Table, return
**	    the found entry's BID as TID rather than using it as a 
**	    compare condition.
**	    Added optional char *record output parm, used to return the
**	    found leaf entry (row) for Clustered tables.
**
*/  
DB_STATUS
dm1bxsearch(
    DMP_RCB		*rcb,
    DMPP_PAGE		*page,
    i4			mode,
    i4			direction,
    char                *key,
    i4			keyno,
    DM_TID		*tid,
    i4			*partno,
    DM_LINE_IDX         *pos,
    char		*record,
    DB_ERROR		*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_ROWACCESS   *rac;
    char	    *keypos; 
    char	    *KeyBuf = NULL, *AllocKbuf = NULL; 
    char	    keybuf[DM1B_MAXSTACKKEY];
    i4	    	    keylen;
    DM_TID	    buftid; 
    DB_STATUS	    s; 
    i4	    	    compare;
    ADF_CB	    *adf_cb;
    DB_ATTS         **keyatts;
    CL_ERR_DESC	    sys_err;
    u_i4	    row_low_tran = 0;
    u_i2	    row_lg_id;
    bool            is_index;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    adf_cb = r->rcb_adf_cb;
    s = E_DB_OK;
    is_index = ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
			DMPP_INDEX) != 0);

    /*
    ** Set up att/key arrays depending on whether this is a leaf or index page
    */
    if (is_index)
    {
	rac = &t->tcb_index_rac;
	keyatts = t->tcb_ixkeys;
    }
    else
    {
	rac = &t->tcb_leaf_rac;
	keyatts = t->tcb_leafkeys;
    }

    /*
    ** Return the partition number of the base Table.
    ** If this a Global index, this partition 
    ** number will be overriden by the found
    ** index entry by dm1cxtget/dm1cxget.
    */
    if ( t->tcb_rel.relstat & TCB_INDEX )
	*partno = t->tcb_parent_tcb_ptr->tcb_partno;
    else
	*partno = t->tcb_partno;


    switch (mode)
    {
    case DM1C_LAST:    
	if (direction == DM1C_NEXT)
	{
	    if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
	    {
		*pos = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page) - 1;
		dm1cxtget (t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
			(*pos), tid, partno);
	    }
	    else
		*pos = 0;
	}
	else
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	}
	break;

    case DM1C_EXTREME:

	/* For the extreme case get first entry on the page */

	if (direction == DM1C_PREV)
	{
	    *pos = 0;
	    if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
		dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
			(*pos), tid, partno );
	}
	else
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	}
        break; 

    case DM1C_FIND:
    case DM1C_SPLIT:
    case DM1C_OPTIM:

	if (direction != DM1C_EXACT)
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	    break;
	}

	/* 
        ** Binary search the page to find the position of a
        ** key that is greater than or equal to one specified.
        ** If find multiple matching entries, get lowest such entry.
        */
        
	s = binary_search(r, page, direction, key, keyno, pos, dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

        if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
		(*pos), tid, partno );
        break; 

    case DM1C_RANGE:

	if (direction != DM1C_PREV && direction != DM1C_NEXT)
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	    break;
	}

	/* 
        ** Binary search the page to find the position of a
        ** key that is greater than or equal to one specified.
        ** If find multiple matching entries, get lowest such entry.
        */
        
	s = binary_search(r, page, direction, key, keyno, pos, dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

        if (*pos != DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, 
		(*pos), tid, partno );
        break; 

    case DM1C_TID:

	if (direction != DM1C_EXACT)
	{
	    uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    s = E_DB_ERROR;
	    break;
	}

	/* 
        ** Binary search the page to find the position of a
        ** key that is greater than or equal to one specified.
        ** If find multiple matching entries, get lowest or highest 
        ** depending on direction.
        */
        
	s = binary_search(r, page, direction, key, keyno, pos, dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    break;
	}

	/* 
	** If the mode is DM1C_TID, then want the exact row matching the key
	** and TID values, not just the first entry matching the given key.
	** Search from this position onward, until we either locate the entry 
	** whose TIDP matches the indicated TID or run out of entries.  Note
	** that we extract the index entry using the rowaccess which
	** describe the entire entry, but compare keys using keyatts and keyno,
	** which describe only the key if this is a secondary index.
	*/
	for (; (i4)*pos < 
		(i4)DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page);
		(*pos)++)
	{
	    if ( is_index )
	    {
		keypos = KeyBuf;
		keylen = t->tcb_ixklen;
	    }
	    else
	    {
		keypos = (record) ? record : KeyBuf;
		keylen = t->tcb_klen;
	    }
	    if ( !keypos ) 
	    {
		if ( s = dm1b_AllocKeyBuf(keylen, keybuf,
					    &KeyBuf, &AllocKbuf, dberr) )
		    return(s);
		keypos = KeyBuf;
	    }
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page,
			rac, *pos, 
			&keypos, (DM_TID *)&buftid, partno,
			&keylen, &row_low_tran, &row_lg_id, adf_cb);

	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
		s = E_DB_OK;

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, page, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, *pos);
		SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	    }
	    else
	    {
		s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, keypos, key, &compare);
		if ( s != E_DB_OK )
		{
		    uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
		}
		else if ( compare != DM1B_SAME )
		{
		    SETDBERR(dberr, 0, E_DM0056_NOPART);
		    s = E_DB_ERROR;
		}
	    }

	    if ( s )
	    {
		/* Discard any allocated key buffer */
		if ( AllocKbuf )
		    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
		return(s);
	    }
		

	    /*
	    ** (jenjo02)
	    **
	    ** I don't know that this TID compare is needed anymore.
	    **
	    ** In unique indexes, the key compare just done is
	    ** sufficient; in non-unique indexes, the TID is part
	    ** of the key, hence matches if the key matches.
	    **
	    ** Maybe old-style index keys (without the TID) are
	    ** the exception, so I'll leave the compare here
	    ** but won't bother comparing partition numbers as
	    ** old indexes won't be part of partitioning anyway.
	    */

	    /* CLUSTERED leaf? Entry's bid is the tid */
	    if ( t->tcb_rel.relstat & TCB_CLUSTERED && !is_index )
		tid->tid_i4 = buftid.tid_i4;

	    if ( tid->tid_tid.tid_page == buftid.tid_tid.tid_page 
		 && tid->tid_tid.tid_line == buftid.tid_tid.tid_line )
	    {
		if (tid->tid_tid.tid_page == 0 
		    && tid->tid_tid.tid_line == DM_TIDEOF)
		{
		    /* Looking for reserved index entry */
		    if (row_low_tran != r->rcb_tran_id.db_low_tran)
			continue;
		}

		if ( !is_index && record && record != keypos )
		    MEcopy(keypos, keylen, record);

		/* Discard any allocated key buffer */
		if ( AllocKbuf )
		    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

		return (E_DB_OK); 
	    }
	}

	/*
	** If we get here, we've exhausted the whole overflow chain. 'page'
	** is still fixed at this point and points to the last page on the
	** chain.
	*/

	/* Discard any allocated key buffer */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

        break; 

    case DM1C_JOIN:
    case DM1C_POSITION:
    case DM1C_SETTID:
    default:
	uleFormat(NULL, E_DM93BA_BXSEARCH_BADPARAM, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, mode, 0, direction );
	SETDBERR(dberr, 0, E_DM93BC_BXSEARCH_ERROR);
	s = E_DB_ERROR;
	break;
    } /* end case statement */

    if (s != E_DB_OK)
	return (s);

    if (*pos == DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page))
    {
	SETDBERR(dberr, 0, E_DM0056_NOPART);
        return (E_DB_ERROR);
    }    
    return(E_DB_OK); 
}

/*{
** Name: dm1bxsplit - Splits a BTREE index page.
**
** Description:
**      This routine takes a BTREE index page and splits it.
**      The sibling is locked after it is obtained from the free list.
**      At the end of dm1bxsplit, the sibling may have changed to
**      the current page passed down by the caller (depending on the
**      key value being inserted.)  Note always split to the right.
**
**	We perform split processing in two phases, because there are actually
**	two types of splits: index splits, in which an index (non-leaf) page
**	is split into two index pages, and leaf splits, in which a leaf may
**	be split either into two leaves or into a leaf and an overflow. This
**	routine performs the processing common to all the splits, and calls
**	either idxdiv or lfdiv to perform the type-specific split processing.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      current                         Pointer to a pointer to a page to split.
**                                      Upon return will point to the page
**                                      which will contain the key causing
**                                      split. (May be old current or sibling).
**      pos                             Value indicating position on
**                                      parent page where new page will
**                                      inserted.
**      parent                          Pointer to parent page.
**      key                             Key that is causing split.
**      keyatts                         Pointer to a list describing 
**                                      attributes of key.
**      keyno                           Number of attributes in key. Note that
**					this may be less than the total number
**					of attributes in the index entry.
**
** Outputs:
**      err_code                        Pointer to an area used to return
**                                      error codes if return value not
**                                      E_DB_OK.  Error codes returned are:
**                                      E_DM
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
**	    none
**
** History:
**      17-oct-85 (jennifer)
**          Converted for Jupiter.
**      02-apr-87 (ac)
**          Added read/nolock support.
**	14-Nov-88 (teg)
**	    added initialiation of adf_cb.adb_maxstring to DB_MAXSTRING
**	 4-dec-88 (rogerk)
**	    If encounter an error, must break to bottom rather than returning
**	    so that cleanup can be done.
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression.
**	8-jul-1991 (bryanp)
**	    During a leaf page split in a Btree primary table, release the
**	    sibling page mutex while logging the data page allocation. This
**	    avoids a resource deadlock: if the dm0l_balloc call blocks because
**	    the logfile is full, the blocked thread is holding a critical
**	    resource (a mutex on a modified page); this can stall the Fast
**	    Commit Thread, which can result in a locked-up system.
**	19-aug-1991 (bryanp)
**	    Merged in allocation project changes.
**	22-oct-91 (jrb)
**	    Initialized status variable "s" to E_DB_OK; previously it was
**	    garbage and the code was checking its value.
**	25-oct-1991 (bryanp)
**	    Since the split operation is performed in a mini-transaction,
**	    we must ensure that the page allocation is permanent before we
**	    complete the mini-transaction. Call dm1p_force_map to do so.
**	 3-nov-1991 (rogerk)
**	    Moved dm1p_force_map calls together down near end of mini
**	    transaction to attempt to only force the map page once rather
**	    than twice.
**	20-Nov-1991 (rmuth)
**	    Added DM1P_MINI_TRAN, when inside a mini-transaction pass this
**	    flag to dm1p_getfree() so it does not allocate a page deallocated
**	    earlier by this transaction. see dm1p.c for explaination why.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Fixes. Added dm0p_pagetype call.
**	17-dec-1992 (rogerk)
**	    Fix problem with splitting index when new insert key exactly
**	    matches the key at the split position. When comparing keys to
**	    determine the insert direction, check for <= instead of just <.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Fix call to dm1b_rcb_update to pass DM1C_MSPLIT mode instead
**	    DM1C_SPLIT.  The latter does nothing.
**	26-jul-1993 (rogerk)
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	  - Fix problems with splits of empty leaf pages which have non-empty
**	    overflow chains.  The routine was assuming that a split_pos of
**	    zero implied that all duplicate rows were being moved to the
**	    sibling page.  But this did not turn out to be the case if we
**	    were splitting right but the leaf had no children (in which case
**	    the split pos was set to bt_kids, but that value was also zero).
**	    Added split direction field to the split log record.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	28-mar-1994 (jnash)
**	    During index block splits, release lock on unfix of original
**	    index page if not leaf split.  Leaving the index locked was 
**	    causing deadlocks during UNDO and pass-abort recovery.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format 
**	    project.
**	    Added plv parameter to dm1bxformat routine call.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled;
**          Add arguments to dm1b_rcb_update call.
**          Check for E_DB_WARNING return code from dm1cxget()
**	10-jan-1997 (nanpr01)
**	    A pervious submission changed the log record for VPS but
**	    did not reserve enough space.
**      23-jan-1997 (stial01)
**          dm1bxsplit() when row locking, request locks for new sibling
**          page for all lock lists holding intent lock on page being split. 
**      21-apr-1997 (stial01)
**          dm1bxsplit(): If row locking, reserve space for new row
**      29-apr-1997 (stial01)
**          dm1bxsplit() Get dup key value if split pos 0 and 2k page size
**	09-May-1997 (jenjo02)
**	    Added rcb_lk_id to LKpropagate() call.
**      21-may-97 (stial01)
**          dm1bxsplit() Added flags arg to dm0p_unmutex call(s).
**          Add TRdisplay for LKpropagate.
**          RCB update only if leaf split.
**          If row locking and new record belongs on sibpage, unlock curpage  
**          buffer, lock sibpage buffer.
**          Return E_DB_ERROR if error.
**      12-jun-97 (stial01)
**          dm1bxsplit() Pass tlv to dm1cxget instead of tcb.
**          Pass new attribute, key parameters to dm1bxformat
**      18-jun-97 (stial01)
**          dm1bxsplit() Do not set DM0L_ROW_LOCK when updating index pages
**      29-jul-97 (stial01)
**          dm1bxsplit() Changed args to dm1b_rcb_update
**      13-jan-1997 (stial01)
**          Set DMPP_CLEAN on sibpage if it was set on split page (B88076)
**      21-apr-98 (stial01)
**          dm1bxsplit() Set DM0L_PHYS_LOCK if extension table (B90677)
**      26-aug-98 (stial01)
**          Fixed LKpropagate error handling.
**      22-sep-98 (stial01)
**          dm1bxsplit() If row locking, downgrade/convert lock on new page(s)  
**          after mini transaction is done (B92909)
**      09-feb-99 (stial01)
**          dm1bxsplit() Log kperpage
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
DB_STATUS
dm1bxsplit(
    DMP_RCB     *rcb,
    DMP_PINFO	*currentPinfo,
    i4     	pos,
    DMP_PINFO	*parentPinfo,
    char        *key,
    i4		keyno,
    i4		mode,
    DB_ERROR   	*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_TAB_ID       tabid = t->tcb_rel.reltid;
    DMPP_PAGE	    *sibling = (DMPP_PAGE *)NULL;
    DMPP_PAGE	    *data = (DMPP_PAGE *)NULL;
    DMPP_PAGE	    *parpage, *sibpage, *curpage;
    DMP_PINFO	    siblingPinfo, dataPinfo;
    ADF_CB	    *adf_cb;
    DMP_ROWACCESS   *rac;
    DB_ATTS         **keyatts;
    DM_LINE_IDX	    split;
    LG_LSN	    prev_lsn;
    LG_LSN	    lsn;
    LG_LRI	    lri;
    DM_TID	    bid, newbid;
    DB_STATUS	    s = E_DB_OK;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4         	    keys_given = t->tcb_keys;
    char	    *desc_key = NULL;
    char            *index_desc_key = NULL;
    char	    *log_desc_key = NULL;
    char	    *keyptr;
    i4		    NumKeyBufs;
    i4		    KeyBufSize;
    char	    AllKeys[DM1B_MAXSTACKKEY];
    char	    *AllocKbuf;
    char	    *KeyBuf;
    char            *DescKeyBuf;
    char	    *CompressBuf;
    char	    *LrangeBuf;
    char	    *RrangeBuf;
    char            *range_keys[2]; 
    i4	    	    range_lens[2];
    i4	    	    infinities[2]; 
    i4	    	    split_keylen;
    i4	    	    keylen;
    i4	    	    desckeylen;
    i4	    	    ptype = 0; 
    i4	    	    compare;
    i4	    	    alloc_flag;
    i4	    	    dm0l_flag;
    i4	    	    unfix_action;
    i4	    	    compression_type;
    i4	    	    insert_direction;
    i4	    	    n;
    i4	    	    loc_id;
    i4	    	    cur_loc_cnf_id;
    i4	    	    sib_loc_cnf_id;
    i4	    	    dat_loc_cnf_id;
    i4	    	    local_error;
    bool	    leaf_split;
    bool	    direction_computed = FALSE;
    bool	    pages_mutexed = FALSE;
    bool	    critical_section = FALSE;
    LK_LKID         sib_lkid, data_lkid;
    i2		    TidSize;
    i4		    Clustered;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    siblingPinfo.page = NULL;
    dataPinfo.page = NULL;

    parpage = parentPinfo->page;
    curpage = currentPinfo->page;

    /*
    ** Extract the TID size from the page.
    **
    ** For (non)LEAF pages this will be sizeof(TID),
    ** sizeof(TID8) for LEAF pages in Global indexes.
    */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(tbio->tbio_page_type, curpage);

    Clustered = DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage)
			& DMPP_CLUSTERED;

    adf_cb = r->rcb_adf_cb;
    compression_type = DM1B_INDEX_COMPRESSION(r);
    leaf_split = ((DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) & 
			DMPP_LEAF) != 0);

    /*
    ** Set up att/key arrays depending on whether this is a leaf or index page
    */
    if (leaf_split)
    {
	rac = &t->tcb_leaf_rac;
	keyatts = t->tcb_leafkeys;
	split_keylen = t->tcb_klen;
	dmf_svcb->svcb_stat.btree_leaf_split++;
	/* Leaf split needs range key buffers */
	NumKeyBufs = 5;
    }
    else
    {
	rac = &t->tcb_index_rac;
	keyatts = t->tcb_ixkeys;
	split_keylen = t->tcb_ixklen;
	dmf_svcb->svcb_stat.btree_index_split++;
	NumKeyBufs = 3;
    }

    /*
    ** Choose page type of the new sibling page based on the type of the page
    ** being split.
    */
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) & DMPP_SPRIG)
	ptype = DM1B_PSPRIG;
    else if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) & 
		DMPP_INDEX)
	ptype = DM1B_PINDEX; 
    else if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) & 
		DMPP_LEAF)
	ptype = DM1B_PLEAF; 

    if (DMZ_AM_MACRO(5))
    {
	TRdisplay("dm1bxsplit:table is %~t, parent is %d, current is %d\n", 
                       sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
                       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, parpage),
		       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, curpage));
	/* %%%% Place call to debug routine to print locks. */
    }

    /* Manufacture a Key buffer with room for NumKeyBufs keys */
    KeyBufSize = DB_ALIGN_MACRO(split_keylen);

    if ( s = dm1b_AllocKeyBuf(NumKeyBufs * KeyBufSize, AllKeys, &KeyBuf, 
				&AllocKbuf, dberr) )
	return(s);

    /* Carve it up into individual key spaces */
    DescKeyBuf  = KeyBuf + KeyBufSize;
    CompressBuf = DescKeyBuf + KeyBufSize;

    if ( leaf_split )
    {
	LrangeBuf = CompressBuf + KeyBufSize;
	RrangeBuf = LrangeBuf + KeyBufSize;
    }
    else
    {
	LrangeBuf = NULL;
	RrangeBuf = NULL;
    }

    
    for (;;)
    {
	/*
	** For Leaf Page splits, extra work and checks are required.  Leaf
	** page splits are different than simple index splits in the following
	** ways:
	**
	**       - Leaf pages have high and low range key values.
	**
	**       - Leaf pages may have duplicate key values on them.  Because
	**         of this, the split position may have to be adjusted from
	**         the midpoint position to make sure that all duplicate keys
	**         remain on the same leaf.  The resulting split position may
	**         end up being position 0.
	**
	**       - Because of duplicate keys, the split position of a leaf
	**         may be position zero (ALL current keys will be moved to the
	**         new sibling page) or position bt_kids (NO current keys will
	**         be moved to the new sibling page).
	**
	**       - Leaf pages have sideways pointers which must be updated.
	**
	** The following additional checks must be made for leaf splits:
	**
	**       - We must save the range keys on the current leaf to use
	**         in updating the sibling page.  The range keys will
	**         eventually be updated in the current leaf as well.
	**
	**       - We must determine whether to use the key just previous
	**         to the split position as the descriminator key (with which
	**         to update the parent index) or to use the insert key.
	*/

	/*
	** Find the spot at which to split the page.
	*/
	s = division(r, curpage, &split, keyno, dberr);
	if (s != E_DB_OK)
	    break;

	/*
	** If division returned to split from position 0, then this means
	** that the current page is full of duplicate entries (here 'full'
	** may mean zero or one entries if there is an overflow chain) and
	** the new key must be insert on a different leaf than the duplicate
	** keys.
	**
	** We must do some extra work to determine which way to move the
	** duplicate keys and where to put the new insert key.
	**
	** If the new key is greater than the duplicates, we will split off
	** an empty sibling and put the new key on it.  In this case the
	** split position should be bt_kids, the insert direction RIGHT and
	** the descriminator key is the duplicate key value.
	**
	** If the new key is less than the duplicates, we will split by moving
	** all the duplicate keys to the new sibling, emptying the current
	** leaf for the new insert key.  In this case the split position
	** will be 0, the insert direction LEFT and the descriminator key is
	** the new insert key.
	*/
	if (split == 0 && t->tcb_dups_on_ovfl)
	{
	    s = get_dup_key(rcb, curpage, KeyBuf, dberr);
	    if (s != E_DB_OK)
		break;

	    direction_computed = TRUE;

	    s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, key, KeyBuf, &compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }
	    if (compare > DM1B_SAME)
	    {
		split = DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, curpage);
		insert_direction = DM1B_RIGHT;
		desc_key = KeyBuf;
	    }
	    else if (compare < DM1B_SAME)
	    {
		split = 0;
		insert_direction = DM1B_LEFT;
		desc_key = key;
	    }
	    else if (mode != DM1C_SPLIT_DUPS)
	    {
		/*
		** We should have done an add-overflow operation, not a split!
		*/
		TRdisplay("Btree Split: called with a full duplicate page\n");
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }
	}

	/*
	** Extract the key just before the the split position to use as the
	** descriminator key.  It will become the high range key for the
	** current page, the low range key for the new sibling and will be
	** insert into the parent page.
	**
	** Note these edge cases have already been computed above.
	** (we have to be a little careful with these edge cases because
	** if bt_kids == 0 it can be tricky telling them apart)
	**
	**     split position 0       (LEFT)    : use insert key
	**     split position bt_kids (RIGHT)   : use duplicate chain key
	*/
	if (desc_key == NULL)
	{
	    keyptr = KeyBuf;
	    keylen = split_keylen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, curpage,
			rac, split - 1, 
			&keyptr, (DM_TID *)NULL, (i4*)NULL, 
			&keylen, NULL, NULL, adf_cb);

	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
		s = E_DB_OK;

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, curpage, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, split - 1);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }

	    desc_key = keyptr;
	}

	/*
	** Compare the key at the split position with the new key being
	** insert into the index.  This tells us where the new key will
	** be inserted - to the old page or new split page (or their
	** descendents).  Left means the old page, Right means the new page.
	**
	** Note these edge cases have already been computed above.
	** (we have to be a little careful with these edge cases because
	** if bt_kids == 0 it can be tricky telling them apart)
	**
	**     split position 0       (LEFT)   : insert left
	**     split position bt_kids (RIGHT)  : insert right
	*/
	if (direction_computed == FALSE)
	{
	    s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, key, desc_key, &compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }
	    if (compare <= DM1B_SAME)
		insert_direction = DM1B_LEFT;
	    else
		insert_direction = DM1B_RIGHT;
	}


	/*
	** For Leaf Page splits, extract the Low and High Range Keys.
	*/ 
	if (leaf_split)
	{
	    range_keys[DM1B_LEFT] = LrangeBuf;
	    range_keys[DM1B_RIGHT] = RrangeBuf;
	    s = get_range_entries(r, curpage, range_keys, range_lens,
			infinities, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Special check for inserts of increasing key values.
	    **
	    ** If this split is to the Highest range LEAF page in the table 
	    ** (the one with the infinity Rrange value) and our new key is 
	    ** greater than the highest valued key currently on the leaf, then
	    ** adjust the split position to be at the highest value (bt_kids)
	    ** so that a new empty leaf will be created for the new key and no
	    ** old keys will be moved over in the split.
	    **
	    ** Use 2nd key buffer to avoid overwriting our current descriminator
	    ** key if it turns out that the last key on the page is greater
	    ** than the insert key.
	    **
	    ** Can't use this optimization if splitting a full duplicate leaf.
	    */
	    if ((infinities[1]) && (split != 0) && 
		(split != DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type,curpage)))
	    {
		/* Get highest valued key on the Leaf */

		keyptr = CompressBuf;
		keylen = t->tcb_klen;
		s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,curpage,
		    rac,
		    DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, curpage) - 1,
		    &keyptr, (DM_TID *)NULL, (i4*)NULL,
		    &keylen, NULL, NULL, adf_cb);

		if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
		    s = E_DB_OK;

		if (s != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, curpage, 
		     tbio->tbio_page_type, tbio->tbio_page_size,
		     DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, curpage) - 1);
		    SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		    break;
		}

		/*
		** Compare the high key on leaf with insert the key.  If the
		** insert key is greater then adjust the split position.
		*/
		s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, key, keyptr, 
				    &compare);
	        if (s != E_DB_OK)
	        {
		    uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		    break;
	        }
		if (compare > DM1B_SAME)
		{
		    split = DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, 
						   curpage);
		    insert_direction = DM1B_RIGHT;
		    MEcopy((PTR) keyptr, keylen, (PTR) KeyBuf);
		    desc_key = KeyBuf;
		}
	    }
	}

	/*
	** If leaf split, and leaf att count != index att count
	** or CLUSTERED,
	** we need to prepare descriminator key 
	** Leaf pages contain key and non-key fields
	** New style index pages do not have non-key fields
	*/
	if ( t->tcb_rel.relstat & TCB_CLUSTERED ||
	    rac->att_count != t->tcb_index_rac.att_count )
	{
	    index_desc_key = DescKeyBuf;
	    dm1b_build_key(t->tcb_keys, t->tcb_leafkeys, desc_key,
		t->tcb_ixkeys, index_desc_key);
	}
	else
	    index_desc_key = desc_key;

	/*
	** Prepare descriminator key for logging.  It must be logged in its
	** raw format - compressed if compression is used.
	*/
	log_desc_key = desc_key;
	desckeylen = split_keylen;
	if (rac->compression_type != TCB_C_NONE)
	{
	    s = dm1cxcomp_rec(rac, desc_key,
			split_keylen, CompressBuf, &desckeylen);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP, (DMP_RCB *)r,
				(DMPP_PAGE *)NULL, 0, 0, 0 );
		SETDBERR(dberr, 0, E_DM93BF_BXUPDRANGE_ERROR);
		break;
	    }
	    log_desc_key = CompressBuf;
	}

	/*
	**************************************************************
	**           Start Split Transaction
	**************************************************************
	**
	** Splits are performed within a Mini-Transaction.
	** Should the transaction which is updating this table abort after
	** we have completed the split, then the split will NOT be backed
	** out, even though the insert which caused the split will be.
	**
	** We commit splits so that we can release the index page locks
	** acquired during the split and gain better concurrency.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    s = dm0l_bm(r, &lsn, dberr);
	    if (s != E_DB_OK)
		break;
	}
	STRUCT_ASSIGN_MACRO(lsn, prev_lsn);

	/*
	** Allocate the new sibling page.
	** The allocate call must indicate the it is performed from within
	** a mini-transation so that we will not be allocated any pages that
	** were previously deallocated in this same transaction.  (If we were
	** and our transaction were to abort it would be bad: the undo of the 
	** deallocate would fail when it found the page was not currently free
	** since the split would not have been rolled back.)
	*/
	alloc_flag = DM1P_MINI_TRAN;
	if (DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage) & 
			DMPP_INDEX)
	    alloc_flag |= DM1P_PHYSICAL;

	/*
	** dm1p_getfree may MLOCK the sibling page's buffer; it will be
	** unlocked by dm1bx_lk_convert or dm0p_unfix_page.
	*/
	s = dm1p_getfree(r, alloc_flag, &siblingPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	sibling = siblingPinfo.page;

	/* Save lockid for sibling */
	STRUCT_ASSIGN_MACRO(r->rcb_fix_lkid, sib_lkid);

	sibpage = sibling; 

	/* Inform buffer manager of new page's type */
	dm0p_pagetype(tbio, sibling, r->rcb_log_id, DMPP_INDEX);

	/*
	** If the split adds a new leaf page to a base table,
	** then we also need to allocate a new data page.
	*/
	if ((leaf_split) &&
	    ((t->tcb_rel.relstat & TCB_INDEX) == 0))
	{
	    /*
	    ** dm1p_getfree may MLOCK the data page's buffer; it will be
	    ** unlocked by dm0p_unfix_page.
	    */
	    s = dm1p_getfree(r, DM1P_MINI_TRAN, &dataPinfo, dberr);
	    if (s != E_DB_OK)
		break;

	    data = dataPinfo.page;

	    /* Inform buffer manager of new page's type */
	    dm0p_pagetype(tbio, data, r->rcb_log_id, DMPP_DATA);

	    /* Save lockid for new data */
	    STRUCT_ASSIGN_MACRO(r->rcb_fix_lkid, data_lkid);
	}

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("Btree Split: page %d, sibling %d, (data %d)\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, curpage),
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, sibling), 
		(data ? 
		 DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data) : -1));
	    TRdisplay("     : split pos %d, insert dir %w, page type %v\n", 
		split, "LEFT,RIGHT", insert_direction, 
		",,,,,DATA,LEAF,,,,,,CHAIN,INDEX,,SPRIG", 
		DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage));
	    TRdisplay("     : Insert Key is:\n");
	    dmd_prkey(r, key, leaf_split ? DM1B_PLEAF : DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
	    TRdisplay("     : Key at Split position is:\n");
	    dmd_prkey(r, desc_key, leaf_split ? DM1B_PLEAF : DM1B_PINDEX, (DB_ATTS**)NULL, (i4)0, (i4)0);
	}

	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, 
						   sibpage));
	sib_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
						   curpage));
	cur_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	dat_loc_cnf_id = 0;
	if (data)
	{
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
			(i4)DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
							  data));
	    dat_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	}

	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, sibpage, dberr);
	    if (s != E_DB_OK)
		break;
	    s = dm0p_before_image_check(r, curpage, dberr);
	    if (s != E_DB_OK)
		break;
	    if (data)
	    {
	        s = dm0p_before_image_check(r, data, dberr);
	        if (s != E_DB_OK)
		    break;
	    }
	}

	/************************************************************
	**            Log the Split operation.
	*************************************************************
	**
	** The LSN of the split log record is written to the current
	** and new leaf pages (as well as the new data page if there is
	** one.  The LSN is not written to the parent index page as that
	** page is not recovered through this log record.  The update
	** to the parent page will be logged separately.
	**
	** We also write the LSN of the split log record to the current
	** and new page's bt_split_lsn field.  Btree Put and Delete Recovery
	** routines use this value to determine whether a page has been
	** split since the logged update has been performed.  An update
	** cannot be recovered physically if a subsequent split has been
	** performed.
	*/

	if (r->rcb_logging & RCB_LOGGING)
	{
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs, extension tables
	    ** and index pages. The same lock protocol must be observed during 
	    ** recovery.
	    */
	    if ( NeedPhysLock(r) || !leaf_split )
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2, 
		2 * (sizeof(DM0L_BTSPLIT) - sizeof(DMPP_PAGE) + 
		t->tcb_rel.relpgsize + desckeylen), &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
		break;
	    }

	    /*
	    ** Mutex the new and old index pages (and data page if allocated)
	    ** prior to beginning the udpates.  This must be done prior to
	    ** the writing of the log record to satisfy logging protocols.
	    */
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &siblingPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	    if (data)
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);

	    DM1B_VPT_GET_PAGE_LRI_MACRO(tbio->tbio_page_type, curpage, &lri);

	    s = dm0l_btsplit(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		leaf_split ? t->tcb_kperleaf : 
		t->tcb_kperpage,
		t->tcb_index_rac.compression_type, t->tcb_rel.relloccount,
		cur_loc_cnf_id, sib_loc_cnf_id, dat_loc_cnf_id,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, curpage), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, sibpage), 
		(data ? 
		 DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, data) : 0), 
		split, insert_direction, split_keylen, 
		t->tcb_rngklen,		/* max range entry length */
		desckeylen, log_desc_key,
		curpage, (LG_LSN *) 0, &lri, dberr);
	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_PAGE_LRI_MACRO(tbio->tbio_page_type, curpage, &lri);

	    /*
	    ** data is new and has no page update history, so just 
	    ** record the bi lsn, leaving the rest of the LRI information
	    ** on the page zero.
	    */
	    if (data)
		DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, data, lri.lri_lsn);

	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, curpage, lri.lri_lsn);
	}
	else
	{
	    pages_mutexed = TRUE;
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &siblingPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	    if (data)
		dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);
	    if (leaf_split)
		DM1B_VPT_INCR_BT_CLEAN_COUNT_MACRO(tbio->tbio_page_type, curpage);
	}

	/*
	** Operation is now at a critical point.  Since we have logged the
	** split we must now succeed with the update or we will have to
	** fail over to REDO recovery since we will be unable to count on
	** the cached pages being in a consistent state.
	*/
	critical_section = TRUE;

	/*
	** Format the new sibling page.
	*/
	s = dm1bxformat(tbio->tbio_page_type, tbio->tbio_page_size,
	    r, t, sibpage,
	    leaf_split ? t->tcb_kperleaf : t->tcb_kperpage,
	    split_keylen, rac->att_ptrs, rac->att_count,
	    keyatts, t->tcb_keys,
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, sibpage),
	    ptype, compression_type, t->tcb_acc_plv, 
	    TidSize, Clustered, dberr);
	if (s != E_DB_OK) 
	    break;

	DM1B_VPT_SET_PAGE_TRAN_ID_MACRO(tbio->tbio_page_type, sibpage, 
	    DM1B_VPT_GET_PAGE_TRAN_ID_MACRO(tbio->tbio_page_type, curpage));
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, sibpage, DMPP_MODIFY);

	/* We may have moved deleted tuples to sibpage, update page_stat */
	if (((DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage)
		& DMPP_CLEAN) != 0) )
	    DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, sibpage, DMPP_CLEAN);

	if (r->rcb_logging & RCB_LOGGING)
	{
	    /*
	    ** sib is new but it gets cur page update history
	    ** This is a special case for MVCC because keys right shift
	    */
	    DM1B_VPT_SET_PAGE_LRI_MACRO(tbio->tbio_page_type, sibpage, &lri);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(tbio->tbio_page_type, sibpage, lri.lri_lsn);
	}

	/*
	** Move the appropriate entries from the current page to the
	** sibling page.
	*/
	s = dm1cxrshift(tbio->tbio_page_type, tbio->tbio_page_size, 
			curpage, sibpage, compression_type,
			split, split_keylen + TidSize);
	if (s != E_DB_OK)
	    break;

	/*
	** If a new associated data page was allocated, then format it as an
	** associated data page and link it to the newly allocated leaf page.
	*/
	if (data)
	{
	    DMPP_VPT_INIT_PAGE_STAT_MACRO(tbio->tbio_page_type, data, 
			DMPP_DATA | DMPP_ASSOC | DMPP_PRIM | DMPP_MODIFY);
	    DM1B_VPT_SET_BT_DATA_MACRO(tbio->tbio_page_type, sibpage, 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, data)); 
	}

	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, curpage, DMPP_MODIFY);

	/******************************************************************
	**           LEAF PAGE PROCESSING
	*******************************************************************
	**
	** If splitting a leaf page, then update the range keys on the
	** leaf and sibling page.  The current leaf's range keys span from
	** its old lrange value to the new descriminator key value.  The new
	** sibling page's range keys span from the descriminator key value to
	** the leaf's old rrange value.
	**
	** Also set the new and old leaf's sideways pointers.  Since we always
	** split to the right, it's always: curpage -> sibling -> nextleaf.
	**
	** If the old leaf had an overflow chain, and the duplicate keys
	** just moved in the split, then the overflow chain must be moved
	** over to the new sibling leaf.  (Also, information on the overflow
	** leaf pages will need to be updated, but that will be done and
	** logged by the update_overflow_chain call below).
	*/
	if (leaf_split)
	{
	    if (t->tcb_rng_rac == &t->tcb_leaf_rac)
	    {
		s = update_range_entries(r, curpage, sibpage, range_keys, 
		    range_lens, infinities, desc_key, split_keylen, 
		    dberr);
	    }
	    else
	    {
		s = update_range_entries(r, curpage, sibpage, range_keys, 
		    range_lens, infinities, index_desc_key, t->tcb_ixklen,
		    dberr); 
	    }

	    if (s != E_DB_OK)
		break;

	    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, sibpage, 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, curpage));
	    DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, curpage, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, sibpage));

	    if ((DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, curpage)) && 
		(insert_direction == DM1B_LEFT))
	    {
		DM1B_VPT_SET_PAGE_OVFL_MACRO(tbio->tbio_page_type, sibpage, 
		      DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, curpage));
		DM1B_VPT_SET_PAGE_OVFL_MACRO(tbio->tbio_page_type, curpage, 0);
	    }

	    /*
	    ** If row locking, while we have the page mutexed: 
	    **  Reserve space on curpage or sibpage for key being inserted
	    **  If this fails, the split should succeed so long as the
	    **  reserve did not alter the page.
	    */
	    if ((row_locking(r) || crow_locking(r)) && mode != DM1C_SPLIT_DUPS)
	    {
		DM_TID          resbid;
		DMP_PINFO       *respage;
		DM_TID          localtid;
		DM_LINE_IDX     resline;
		i4		localpartno;

		if (insert_direction == DM1B_LEFT)
		{
		    /* Reserve space on curpage */ 
		    respage = currentPinfo;
		}
		else
		{
		    /* Reserve space on sibpage */
		    respage = &siblingPinfo;
		}
		resbid.tid_tid.tid_page = 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, respage->page);

		/*
		** Find the correct spot on the leaf for the new record.
		**
		** This call may return NOPART if the leaf is empty - in that 
		** case it will return with lineno set to the first position (0)
		*/
		s = dm1bxsearch(r, respage->page, DM1C_OPTIM, DM1C_EXACT, key,
			keys_given, &localtid, &localpartno,
			&resline, NULL, dberr);

		if (s == E_DB_ERROR)
		{
		    if (dberr->err_code != E_DM0056_NOPART)
		    {
			dm1cxlog_error(E_DM93C5_BXSPLIT_RESERVE, r, respage->page, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,resline);
			break;
		    }
		    s = E_DB_OK;
		}

		/* 
		** There are cases where split does not make sufficient
		** space and we need to split again
		*/
		if (dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			respage->page, compression_type, 
			(i4)100 /* fill factor */,
			t->tcb_kperleaf, 
			(t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion + TidSize)) == TRUE)
		{
		    resbid.tid_tid.tid_line = resline;
		    s = dm1bxreserve(r, respage, &resbid, key, TRUE, 
			    FALSE, dberr);
		    if (s != E_DB_OK) 
		    {
			dm1cxlog_error(E_DM93C5_BXSPLIT_RESERVE, r, respage->page, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,resline);
			break;
		    }
		}
	    }
	}


	/*
	** Unmutex the pages since the update actions described by
	** the split log record are complete.
	**
	** Following the unmutexes, these pages may be written out
	** to the database at any time.
	*/
	critical_section = FALSE;
	pages_mutexed = FALSE;
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &siblingPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	if (data)
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);


	/*
	** If we just split a leaf which had a duplicate overflow chain,
	** then update the overflow chain's range keys and nextpage
	** pointers (if the overflow chain moved to the sibling page
	** then we update it there).
	** Only need to update_overflow_chain range keys for V1
	** V2 created overflow pages with range keys = dupkey
	*/
	if (leaf_split)
	{
	    if (DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, curpage))
	    {
		s = update_overflow_chain(r, currentPinfo, dberr);
	    }
	    else if (DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, sibpage))
		s = update_overflow_chain(r, &siblingPinfo, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Insert the new Descriptor Key into the parent index page
	** to complete the split operation.
	** If splitting a leaf check if leaf attcnt is same as index attcnt
	*/
	bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
							sibpage);
	bid.tid_tid.tid_line = 0;
	s = dm1bxinsert(r, t, t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		&t->tcb_index_rac,
		t->tcb_ixklen, parentPinfo,
		index_desc_key, &bid, (i4)0,
		pos, (i4) TRUE, (i4) TRUE, dberr);
	if (s != E_DB_OK)
	    break;

#ifdef xDEV_TEST
	if (DMZ_CRH_MACRO(19) || DMZ_CRH_MACRO(20) || DMZ_CRH_MACRO(21))
	{
	    TRdisplay("dm1b_split: CRASH! Splitting table %t, db %t.\n",
		       sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		       sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	    if (DMZ_CRH_MACRO(20))
	    {
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, currentPinfo, dberr);
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, &siblingPinfo, dberr);
		TRdisplay("Current and Sibling Forced.\n");
	    }
	    if (DMZ_CRH_MACRO(21))
	    {
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, currentPinfo, dberr);
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, &siblingPinfo, dberr);
		(VOID) dm0p_unfix_page(r, DM0P_FORCE, parentPinfo, dberr);
		TRdisplay("Current, Sibling, and Parent Forced.\n");
	    }
	    CSterminate(CS_CRASH, &n);
	}
#endif

	/*
	** End the Mini Transaction used during the SPLIT operation.  After
	** this point the split cannot be undone.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    s = dm0l_em(r, &prev_lsn, &lsn, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Unfix the data page and the one of the current,sibling pages
	** which does not now hold the key range of the insert key.
	**
	** We leave this routine with the parent page and the index page
	** (current or sibling) with holds the key range of the insert key
	** still fixed.
	*/
	if (data)
	{
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Unfix action is dependent upon page type - non-leaf pages are
	** temporarily locked and released from our transaction at unfix time.
	*/
	unfix_action = DM0P_RELEASE;
	if (leaf_split)
	    unfix_action = DM0P_UNFIX;

	/*
	** Store current and sibling page numbers before unfixing to be used
	** rcb_update call below.
	*/
	bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
							curpage);
	bid.tid_tid.tid_line = 0;
	newbid.tid_tid.tid_page = 
			DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,sibpage);
	newbid.tid_tid.tid_line = 0;

        /*
        ** Check for any oustanding RCB's that may have had their position
        ** pointers affected by our shuffling tuples around.
	** RCB update is required only when a leaf is split
        */
	if (leaf_split)
	{
	    s = dm1b_rcb_update(r, &bid, &newbid, split, DM1C_MSPLIT,
			(i4)0, (DM_TID *)NULL, (DM_TID *)NULL, (char *)NULL, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** If row locking, convert PHYSICAL X locks to LOGICAL IX locks for
	** the new leaf and data page, and propogate IX locks for new leaf.
	**
	** The current leaf buffer MUST stay locked while the lock on the 
	** new leaf is converted and propagated. This, and the X lock on 
	** the parent index page prevents other sessions from making lock
	** requests on the new leaf until after the propagate is finished.
	*/
	if (leaf_split && (row_locking(r) || crow_locking(r)))
	{
	    if ( !dm0pBufIsLocked(currentPinfo) )
	    {
		SETDBERR(dberr, 0, E_DM93C8_BXLOCKERROR_ERROR);
		s = E_DB_ERROR;
	    }
	    else
	    {
		/*
		** If crow_locking(), this will release the X page locks,
		** but not propagate.
		**
		** It will also unlock the sibling page's buffer.
		*/
		s = dm1bx_lk_convert(r, insert_direction, currentPinfo, 
			&siblingPinfo, &sib_lkid, &data_lkid, dberr);
	    }
	    
	    if (s != E_DB_OK)
	    {
		TRdisplay("BTREE SPLIT LOCK CONVERT ERROR %d buf_locked %d\n",
			dberr->err_code, dm0pBufIsLocked(currentPinfo));
		break;
	    }
	}

	/*
	** Unfix the new sibling if the insert key belongs on the left side
	** of the split.  Unfix the old page and return the new sibling to
	** the caller if the insert key belongs on the right side of the split.
	*/
	if (insert_direction == DM1B_LEFT)
	{
	    s = dm0p_unfix_page(r, unfix_action, &siblingPinfo, dberr);
	    if (s != E_DB_OK)
		break;
	}
	else
	{
	    if ( dm0pBufIsLocked(currentPinfo) )
	    {
		/*
		** Lock buffer for update.
		**
		** This will swap from CR->Root if "sibling" is a CR page.
		*/
		dm0pLockBufForUpd(r, &siblingPinfo);
		dm0pUnlockBuf(r, currentPinfo);
	    }
	    s = dm0p_unfix_page(r, unfix_action, currentPinfo, dberr);
	    if (s != E_DB_OK)
	    {
		dm0pUnlockBuf(r, &siblingPinfo);
		break;
	    }
	    *currentPinfo = siblingPinfo;
	    curpage = currentPinfo->page; 
	    sibling = NULL;
	}

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1bxsplit: returning from split, ");
	    TRdisplay("table is %t, parent is %d, current is %d, \n", 
                       sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
                       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, parpage),
		       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, curpage));
	    /* %%%% Place call to debug routine to print locks. */
	}

	/* Discard any allocated key buffer */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

	return (E_DB_OK);    
    }

    /*
    ** Error handling:
    **
    **     If we have failed while in the middle of performing the split,
    **     then the pages are left in an inconsistent state.  We fail over
    **     to REDO processing to let the page contents be reconstructed.
    */

    if (critical_section)
	dmd_check(E_DM93B2_BXSPLIT_ERROR);

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if (pages_mutexed)
    {
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &siblingPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, currentPinfo);
	if (data)
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &dataPinfo);
    }

    if (siblingPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &siblingPinfo, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }
    if (dataPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &dataPinfo, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93B2_BXSPLIT_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm1bxovfl_alloc - Allocates new leaf overflow page.
**
** Description:
**
** Inputs:
**
** Outputs:
**      err_code
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
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-may-1996 (nanpr01 & thaju02)
**	    Changed the page header access as macro for New Page Format
**	    project.
**	    Subtract tuple header size from index entry length for leaf pages.
**	    Added the plv parameter to dm1bxformat call.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**          If row locking, alloc overflow pg in mini transaction
**      06-jan-97 (dilma04)
**          Removed unneeded DM1P_PHYSICAL flag when calling dm1p_getfree().
**      04-feb-97 (stial01)
**          Tuple headers are always on LEAF and overflow (CHAIN) pages
**      07-apr-97 (stial01)
**          dm1bxovfl_alloc() NonUnique primary btree (V2) dups span leaf pages,
**          so we don't have to worry about row locking in overflow page code.
**      21-may-97 (stial01)
**          dm1bxovfl_alloc() Added flags arg to dm0p_unmutex call(s).
**      12-jun-97 (stial01)
**          dm1bxovfl_alloc() Pass new attribute, key parameters to dm1bxformat
*/
DB_STATUS
dm1bxovfl_alloc(
DMP_RCB		*rcb,
DMP_PINFO	*leafPinfo,
char		*key,
DM_TID		*bid,
DB_ERROR	*dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_DCB		*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_PAGE		*ovfl;
    DMP_PINFO		ovflPinfo;
    DMPP_PAGE		*leaf;
    i4		mainpageno;
    i4		nextpageno;
    i4		nextovfl;
    char		*lrange_key;
    char		*rrange_key;
    i4		lrange_keylen;
    i4		rrange_keylen;
    DM_TID		lrange_tid; 
    DM_TID		rrange_tid; 
    LG_LSN		lsn;
    LG_LSN		prev_lsn;
    LG_LSN		bm_lsn;
    i4		loc_id;
    i4		leaf_loc_cnf_id;
    i4		ovfl_loc_cnf_id;
    i4		compression_type;
    i4		dm0l_flag;
    i4		local_error;
    bool		pages_mutexed = FALSE;
    bool		critical_section = FALSE;
    DB_STATUS		status;
    STATUS		cl_status;
    CL_ERR_DESC	    	sys_err;
    i4		dm1p_flag = 0;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;
    DM_TID		alloc_bid;
    LK_LKID	    ovfl_lkid;
    LG_LRI		lri;

    CLRDBERR(dberr);

    leaf = leafPinfo->page;
    DMP_PINIT(&ovflPinfo);

    for (;;)
    {
	/*
	** Get the information necessary to format the new overflow page:
	**
	**   - mainpage and nextpage values from the leaf page
	**   - the leaf page's range keys
	**
	** The leaf page's range keys will be propogated to the overflow
	** page.  The keys need not be uncompressed in order to insert
	** them on the overflow page.
	*/
	compression_type = DM1B_INDEX_COMPRESSION(r);
	mainpageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, leaf);
	nextpageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, leaf);
	nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, leaf);

	lrange_keylen = t->tcb_rngklen;
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, leaf, 
		DM1B_LRANGE, &lrange_key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, leaf, 
		DM1B_LRANGE, &lrange_tid, (i4*)NULL);
	if (compression_type == DM1CX_COMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, leaf, 
		    DM1B_LRANGE, &lrange_keylen);
	}

	rrange_keylen = t->tcb_rngklen;
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, leaf, 
		DM1B_RRANGE, &rrange_key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, leaf, 
		DM1B_RRANGE, &rrange_tid, (i4*)NULL);
	if (compression_type == DM1CX_COMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, leaf,
		DM1B_RRANGE, &rrange_keylen);
	}

	/*
	**************************************************************
	**           Start overflow alloc Transaction
	**************************************************************
	**
	** Overflow pages are allocated within a Mini-Transaction.
	** Should the transaction which is updating this table abort after
	** we have completed the alloc, then the alloc will NOT be backed
	** out, even though the insert which caused the alloc will be.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    status = dm0l_bm(r, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}
	STRUCT_ASSIGN_MACRO(lsn, prev_lsn);

	/*
	** Allocate the new overflow page.
	** The allocate call must indicate the it is performed from within
	** a mini-transation so that we will not be allocated any pages that
	** were previously deallocated in this same transaction.  (If we were
	** and our transaction were to abort it would be bad: the undo of the 
	** deallocate would fail when it found the page was not currently free
	** since the overflow alloc would not have been rolled back.)
	*/
	dm1p_flag = DM1P_MINI_TRAN;

	/*
	** Allocate a new page for the leaf overflow.
	** (If we ever allow SCONCUR btree tables then this allocation must
	** be done from within a Mini Transaction and use physical locks).
	*/

	/*
	** dm1p_getfree may pin ovfl page's buffer; it will be
	** unpinned by dm0p_unfix_page.
	*/
	status = dm1p_getfree(r, dm1p_flag, &ovflPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	ovfl = ovflPinfo.page;

	/* Save lockid for overflow */
	STRUCT_ASSIGN_MACRO(r->rcb_fix_lkid, ovfl_lkid);

	/* Inform buffer manager of new page's type */
	dm0p_pagetype(tbio, ovfl, r->rcb_log_id, DMPP_CHAIN);

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("Btree Add Overflow Page: overflow page %d, leaf %d\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, leaf)); 
	}


	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, leaf));
	leaf_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl));
	ovfl_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Online Backup Protocols: Check if BIs must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, ovfl, dberr);
	    if (status != E_DB_OK)
		break;
	    status = dm0p_before_image_check(r, leaf, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Log the overflow operation.
	** The LSN of this log record will be written to the leaf and
	** new overflow page.
	**
	** After writing the log record we must succeed in making the
	** updates.  Any error during the udpate will leave the pages
	** in an inconsistent state and the server will shut down in order
	** to allow REDO recovery to reconstruct the page contents.
	*/
	if (r->rcb_logging)
	{
	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2, 
		2 * ( sizeof(DM0L_BTOVFL) - (DM1B_KEYLENGTH - lrange_keylen) -
				(DM1B_KEYLENGTH - rrange_keylen)), 
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9C21_BXOVFL_ALLOC);
		break;
	    }

	    /*
	    ** Mutex the leaf and new overflow pages for the update.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	    pages_mutexed = TRUE;

	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* Get CR info from page, pass to dm0l_btovfl */
	    DM1B_VPT_GET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, leaf, &lri);

	    status = dm0l_btovfl(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_index_rac.compression_type, 
		t->tcb_rel.relloccount, t->tcb_klen, 
		leaf_loc_cnf_id, ovfl_loc_cnf_id, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leaf), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, ovfl),
		mainpageno, nextpageno, nextovfl,
		lrange_keylen, lrange_key, &lrange_tid,
		rrange_keylen, rrange_key, &rrange_tid,
		(LG_LSN *)NULL, &lri, 
		DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, leaf),
		dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Write the LSN of the update overflow record to the page being 
	    ** updated. This marks the version of the page for REDO processing.
	    */
	    DM1B_VPT_SET_PAGE_LRI_MACRO(t->tcb_rel.relpgtype, leaf, &lri);
	}
	else
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	    pages_mutexed = TRUE;
	}

	critical_section = TRUE;

	/*
	** Format the new page as an overflow leaf page.
	** Initialize its range keys, mainpage, nextpage pointers
	** and tidsize from the leaf's values.
	*/
	status = dm1bxformat(tbio->tbio_page_type, tbio->tbio_page_size,
		    r, t, ovfl, t->tcb_kperleaf, t->tcb_klen,
		    t->tcb_leaf_rac.att_ptrs, t->tcb_leaf_rac.att_count,
		    t->tcb_leafkeys, t->tcb_keys,
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl),
		    DM1B_POVFLO, compression_type, t->tcb_acc_plv, 
		    DM1B_VPT_GET_BT_TIDSZ_MACRO(tbio->tbio_page_type, leaf),
		    DM1B_VPT_GET_PAGE_STAT_MACRO(tbio->tbio_page_type, leaf)
				& DMPP_CLUSTERED,
		    dberr);
	if (status != E_DB_OK) 
	    break;

	DM1B_VPT_SET_PAGE_MAIN_MACRO(tbio->tbio_page_type, ovfl, mainpageno);
	DM1B_VPT_SET_PAGE_OVFL_MACRO(tbio->tbio_page_type, ovfl, nextovfl);
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, ovfl, nextpageno);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, ovfl, DMPP_MODIFY);

	/*
	** "ovfl" is new and has no page update history, so just 
	** record the ovfl lsn, leaving the rest of the LRI information
	** on the page zero.
	*/
	if (r->rcb_logging)
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, ovfl, lri.lri_lsn);

	/*
	** Update the overflow page's left and right range keys.
	*/
	status = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, ovfl,
			DM1C_DIRECT, compression_type,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_LRANGE, lrange_key, lrange_keylen, 
			&lrange_tid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, r, ovfl, 
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_LRANGE);
	    SETDBERR(dberr, 0, E_DM9C1F_BXOVFL_ALLOC);
	    break;
	}

	status = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, ovfl,
			DM1C_DIRECT, compression_type, 
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
			DM1B_RRANGE, rrange_key, rrange_keylen, 
			&rrange_tid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, r, ovfl, 
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_RRANGE);
	    SETDBERR(dberr, 0, E_DM9C1F_BXOVFL_ALLOC);
	    break;
	}

	/*
	** Link the overflow page to the parent leaf page.
	*/
	DM1B_VPT_SET_PAGE_OVFL_MACRO(tbio->tbio_page_type, leaf, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl));
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, leaf, DMPP_MODIFY);

	if ((row_locking(r) || crow_locking(r)) 
	    && DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC) == FALSE)
	{
	    /*
	    ** If row locking reserve space on the overflow page
	    **    Allocate leaf entry = TRUE
	    **    Mutex required = FALSE, buf is mutexed already
	    ** Reserve the BID where the new key should be added:
	    ** the first entry on the newly allocated page
	    */
	    alloc_bid.tid_tid.tid_page = 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl);
	    alloc_bid.tid_tid.tid_line = 0;
	    status = dm1bxreserve(r, &ovflPinfo, &alloc_bid, key, TRUE, FALSE, dberr);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93C6_ALLOC_RESERVE, r, leaf, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (i4)0);
		SETDBERR(dberr, 0, E_DM9C1F_BXOVFL_ALLOC);
		break;
	    }
	}

	/*
	** Unmutex the pages as the update is complete.
	*/
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	pages_mutexed = FALSE;
	critical_section = FALSE;

	/*
	** End the Mini Transaction used during the ovflow operation.  After
	** this point the overflow cannot be undone.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    status = dm0l_em(r, &prev_lsn, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** If row locking, convert PHYSICAL X locks to LOGICAL IX locks for
	** the new overflow.
	** No need to progate any locks from the leaf to the new overflow
	** because no keys were moved
	**
	*/
	if (row_locking(r) || crow_locking(r))
	{
	    if ( !dm0pBufIsLocked(leafPinfo) )
	    {
		SETDBERR(dberr, 0, E_DM93C8_BXLOCKERROR_ERROR);
		status = E_DB_ERROR;
	    }
	    else
	    {
		DM_PAGENO ovf_page;

		/*
		** Downgrade X to IX, convert PHYSICAL to logical
		** 
		** The buffer for the new ovfl is locked.
		** No waiting or escalations must occur during the lock 
		** conversion(s).
		** The phys to logical lock conversion in dm1r_lk_convert will
		** pass the LK_IGNORE_LOCK_LIMIT flag which should guarantee 
		its success.
		*/
		ovf_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, ovfl);
		status = dm1r_lk_convert(r, ovf_page, &ovflPinfo, &ovfl_lkid, dberr); 
		if (status != E_DB_OK)
		{
		   uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		   break;
		}
	    }
	}

	/*
	** Unfix the new overflow page and leave with the original leaf fixed.
	*/
	status = dm0p_unfix_page(r, DM0P_UNFIX, &ovflPinfo, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** This routine is also serves to allocate space for the new key
	** for which this overflow operation is being performed.  Return
	** the BID at which the new key should added: the first entry on
	** the newly allocated page.
	*/
	bid->tid_tid.tid_page = 
			DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, leaf);
	bid->tid_tid.tid_line = 0;

	return (E_DB_OK);
    }

    /*
    ** Error Handling:
    **
    ** If we have encountered an error while a page was still mutexed, then
    ** we must fail to allow REDO recovery to run.
    */
    if (critical_section)
    {
	uleFormat(NULL, E_DM9C1F_BXOVFL_ALLOC, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	dmd_check(E_DM9C20_CRITICAL_SECTION);
    }

    if (pages_mutexed)
    {
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
    }

    if (ovflPinfo.page)
    {
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
	SETDBERR(dberr, 0, E_DM9C1F_BXOVFL_ALLOC);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm1bxfree_ovfl - Deallocates empty leaf overflow page.
**
** Description:
**
** Inputs:
**
** Outputs:
**      err_code
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
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Added trace point to bypass dm1bxfree_ovfl processing.  This
**	    is used for btree testing to simulate empty overflow chains which
**	    can exist in pre 6.5 databases.
**	26-jul-1993 (rogerk)
**	  - Added dm1b_rcb_update call to check for other RCB's which are
**	    positioned on the page we are deleting.  They are adjusted to
**	    point back to the end of the previous page.
**	  - Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-may-1996 (nanpr01 & thaju02)
**	    Changed all page header access as macro for New Page Format 
**	    project.
**	    Subtract tuple header size from index entry length for leaf pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled;
**          Add arguments to dm1b_rcb_update. 
**      04-feb-97 (stial01)
**          Tuple headers are always on LEAF and overflow (CHAIN) pages
**      21-may-97 (stial01)
**          dm1bxfree_ovfl() Added flags arg to dm0p_unmutex call(s).
**      30-jun-97 (stial01)     
**          dm1bxfree_ovfl(): Set DM0L_BT_UNIQUE for unique indexes
*/
DB_STATUS
dm1bxfree_ovfl(
DMP_RCB		*rcb,
i4		dupkey_len,
char		*duplicate_key,
DMP_PINFO	*ovflPinfo,
DB_ERROR	*dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_DCB		*d = t->tcb_dcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_PAGE		*ovfl;
    DMPP_PAGE		*prev_ovfl = NULL;
    DMP_PINFO		prevPinfo;
    DM_PAGENO		nextpg;
    char		*lrange_key;
    char		*rrange_key;
    i4		lrange_keylen;
    i4		rrange_keylen;
    DM_TID		lrange_tid; 
    DM_TID		rrange_tid; 
    DM_TID		bid; 
    DM_TID		newbid; 
    LG_LSN		lsn;
    i4		loc_id;
    i4		prev_loc_cnf_id;
    i4		ovfl_loc_cnf_id;
    i4		compression_type;
    i4		local_error;
    bool		pages_mutexed = FALSE;
    bool		critical_section = FALSE;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC	    	sys_err;
    LG_LSN		ovfl_lsn;
    LG_LSN		prev_lsn;
    i4		ovfl_clean_count;
    i4		prev_clean_count;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_RCB		*next_rcb;

    /* dm1bxfree_ovfl() V1 pages only */
    CLRDBERR(dberr);

    /*
    ** dm1bxfree_ovfl() we don't free overflow pages when row locking
    **
    ** Check for free overflow chain bypass trace point.  This is used
    ** for testing empty overflow chains.
    */
    if (DMZ_AM_MACRO(17))
	return (E_DB_OK);

    /* Can't dealloc pages unless mvcc is disabled for db, need to run modify */
    if (MVCC_ENABLED_MACRO(r))
	return (E_DB_OK);

    /* Check that the page is not fixed in any related RCB */
    for (next_rcb = r->rcb_rl_next;
	next_rcb != r;
	next_rcb = next_rcb->rcb_rl_next)
    {
	if (next_rcb->rcb_other.page != ovflPinfo->page)
	    continue;

	/*
	** Skip dealloc if another cursor on the ovfl
	** (at the very least we'd have to unfix next_rcb->other.page)
	*/
	return (E_DB_OK);
    }

    ovfl = ovflPinfo->page;
    prevPinfo.page = NULL;

    for (;;)
    {
	/*
	** Scan the overflow chain from the leaf looking for the page
	** which points to this overflow page.  If we reach the end
	** then something is severely wrong.
	*/
	nextpg = DM1B_VPT_GET_PAGE_MAIN_MACRO(tbio->tbio_page_type, ovfl);
	while (nextpg > 0)
	{
	    status = dm0p_fix_page(r, nextpg, DM0P_WRITE, &prevPinfo, dberr);
	    if (status != E_DB_OK)
		break;
	
	    /*
	    ** Break if we have found the page that points to the overflow page 
	    ** we are deallocating.
	    */
	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, prevPinfo.page) == 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl))
		break;

	    /*
	    ** Otherwise continue to the next overflow page.
	    */
	    nextpg = DMPP_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, prevPinfo.page);
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &prevPinfo, dberr);
	    if (status != E_DB_OK)
		break;
	}

	if (status != E_DB_OK)
	    break;

	if (prevPinfo.page == NULL)
	{
	    uleFormat(NULL, E_DM9C22_BXOVFL_INCONSIST, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 2, 
		0, DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl), 0, 
		DM1B_VPT_GET_PAGE_MAIN_MACRO(tbio->tbio_page_type, ovfl));
	    SETDBERR(dberr, 0, E_DM9C23_BXOVFL_FREE);
	    break;
	}

	prev_ovfl = prevPinfo.page;

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("Btree Deallocate Overflow: ovfl %d, prev %d, leaf %d\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, prev_ovfl), 
		DM1B_VPT_GET_PAGE_MAIN_MACRO(tbio->tbio_page_type, ovfl)); 
	}

	/*
	** Get log information necessary to reconstruct the overflow page
	** in undo recovery:
	**
	**   - mainpage, nextpage and nextovfl values.
	**   - the overflow page's range keys
	*/
	compression_type = t->tcb_rng_rac->compression_type;

	lrange_keylen = t->tcb_rngklen;
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, ovfl, 
		DM1B_LRANGE, &lrange_key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, ovfl,
		DM1B_LRANGE, &lrange_tid, (i4*)NULL);
	if (compression_type != TCB_C_NONE)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, ovfl,
		DM1B_LRANGE, &lrange_keylen);
	}

	rrange_keylen = t->tcb_rngklen;
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, ovfl, 
		DM1B_RRANGE, &rrange_key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, ovfl, 
		DM1B_RRANGE, &rrange_tid, (i4*)NULL);
	if (compression_type != TCB_C_NONE)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, ovfl,
		DM1B_RRANGE, &rrange_keylen);
	}


	/*
	** Get information on the locations to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, ovfl));
	ovfl_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, 
		 (i4)DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, 
						   prev_ovfl));
	prev_loc_cnf_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    status = dm0p_before_image_check(r, ovfl, dberr);
	    if (status != E_DB_OK)
	        break;
	    status = dm0p_before_image_check(r, prev_ovfl, dberr);
	    if (status != E_DB_OK)
	        break;
	}

	/*
	** Log the deallocate overflow operation.
	** The LSN of this log record will be written to the current and
	** previous overflow pages.
	**
	** After writing the log record we must succeed in making the
	** updates.  Any error during the update will leave the pages
	** in an inconsistent state and the server will shut down in order
	** to allow REDO recovery to reconstruct the page contents.
	*/
	if (r->rcb_logging)
	{
	    i4		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);
	    u_i4	dm0l_btflag = 0;

	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2, 
		2 * (sizeof(DM0L_BTFREE) - 
			    (DM1B_KEYLENGTH - lrange_keylen) -
			    (DM1B_KEYLENGTH - rrange_keylen) -
			    (DM1B_KEYLENGTH - dupkey_len)),
		&sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM9C23_BXOVFL_FREE);
		break;
	    }

	    /*
	    ** Mutex the leaf and new overflow pages for the update.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, ovflPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &prevPinfo);
	    pages_mutexed = TRUE;

	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    if (t->tcb_rel.relstat & TCB_UNIQUE)
		dm0l_btflag |= DM0L_BT_UNIQUE;
	    if (t->tcb_dups_on_ovfl)
		dm0l_btflag |= DM0L_BT_DUPS_ON_OVFL;
	    if (t->tcb_rng_has_nonkey)
		dm0l_btflag |= DM0L_BT_RNG_HAS_NONKEY;

	    status = dm0l_btfree(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_index_rac.compression_type, 
		t->tcb_rel.relloccount, t->tcb_klen,
		ovfl_loc_cnf_id, prev_loc_cnf_id, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, ovfl), 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, prev_ovfl),
		DM1B_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, ovfl), 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, ovfl), 
		DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ovfl),
		dupkey_len, duplicate_key,
		lrange_keylen, lrange_key, &lrange_tid,
		rrange_keylen, rrange_key, &rrange_tid,
		(LG_LSN *)NULL, &lsn, 
		DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, ovfl),
		dm0l_btflag,
		dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Write the LSN of the update overflow record to the page being 
	    ** updated. This marks the version of the page for REDO processing.
	    */
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, ovfl, lsn);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, prev_ovfl, lsn);
	}
	else
	{
	    /*
	    ** Mutex the leaf and new overflow pages for the update.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, ovflPinfo);
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &prevPinfo);
	    pages_mutexed = TRUE;
	}

	critical_section = TRUE;

	/*
	** Unlink the overflow page from its previous page.
	*/
	DM1B_VPT_SET_PAGE_OVFL_MACRO(tbio->tbio_page_type, prev_ovfl, 
		DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, ovfl));
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, prev_ovfl, DMPP_MODIFY);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, ovfl, DMPP_MODIFY);

	/*
	** Unmutex the pages as this part of the update is complete.
	*/
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, ovflPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &prevPinfo);
	pages_mutexed = FALSE;
	critical_section = FALSE;

	/*
	** Before unfixing the pages, build bid values for the rcb_update 
	** call below.
	*/
	bid.tid_tid.tid_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, 
							ovfl);
	bid.tid_tid.tid_line = 0;
	newbid.tid_tid.tid_page = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, prev_ovfl);
	newbid.tid_tid.tid_line = DM_TIDEOF - 1;

	/*
	** Get lsn,clean count for rcb update
	*/
	DM1B_VPT_GET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype,
			ovfl, ovfl_lsn);
	ovfl_clean_count = 
		DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(t->tcb_rel.relpgtype, ovfl);

	DM1B_VPT_GET_PAGE_LOG_ADDR_MACRO(t->tcb_rel.relpgtype,
			prev_ovfl, prev_lsn);
	prev_clean_count = 
		DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(t->tcb_rel.relpgtype, prev_ovfl);
 
        /*
        ** Unfix the previous overflow page.
        */
        status = dm0p_unfix_page(r, DM0P_UNFIX, &prevPinfo, dberr);
        if (status != E_DB_OK)
            break;
 
	/*
	** Check for any oustanding RCB's that may have had their position
	** pointers affected by the removal of the page from the overflow
	** chain.  Any other cursor positioned on the deallocated page should
	** be adjusted back to point to the end of the previous page so that
	** their next get call will read forward to the next appropriate page.
	*/
	status = dm1b_rcb_update(r, &bid, &newbid, 0, DM1C_MDEALLOC,
			(i4)0,
			(DM_TID *)NULL, (DM_TID *)NULL, (char *)NULL, dberr);
	if (status != E_DB_OK)
	    break;

        /*
	** Complete the operation by marking the deallocated page as free.
	** Note that this action is logged and recovered separately by
	** the page deallocate routines.
	**
	** This action will result in the overflow page being unfixed.
	*/
	status = dm1p_putfree(r, ovflPinfo, dberr);
	if (status != E_DB_OK)
            break;

	return (E_DB_OK);
    }

    /*
    ** Error Handling:
    **
    ** If we have encountered an error while a page was still mutexed, then
    ** we must fail to allow REDO recovery to run.
    */
    if (critical_section)
    {
	uleFormat(NULL, E_DM9C23_BXOVFL_FREE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	dmd_check(E_DM9C20_CRITICAL_SECTION);
    }

    if (pages_mutexed)
    {
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, ovflPinfo);
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &prevPinfo);
    }

    if (prevPinfo.page)
    {
	status = dm0p_unfix_page(r, DM0P_UNFIX, &prevPinfo, &local_dberr);
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
	SETDBERR(dberr, 0, E_DM9C23_BXOVFL_FREE);
    }

    return (E_DB_ERROR);
}

/*{
** Name: binary_search - Perform a binary search on a BTREE index page
**
** Description:
**      This routines examines the BTREE index page to determine the 
**      position of the supplied key. This routine has some complexity due to
**	the fact that a page may have multiple duplicate keys.
**
**	The semantics of this routine depend on whether the provided key
**	matches one of the entries on the page or not.
**
**	If the provided key does NOT match any of the entries on the page, then
**	this routine returns the location on the page where this key would be
**	inserted. This will be a value between 0 and bt_kids. As a special case
**	of this, if the page is empty we return *pos == 0.
**
**	If the provided key DOES match one or more of the entries on the page,
**	then the value returned in 'pos' depends on the value of 'direction':
**	    DM1C_NEXT		Return the position immediately FOLLOWING the
**				last occurrence of the key on the page. This
**				will be a value between 1 and bt_kids
**	    DM1C_PREV		Return the position of the first occurrence
**				of the key on the page. This will be a value
**				between 0 and (bt_kids-1).
**
**	Note that an INDEX page never has duplicate entries; only leaf and
**	overflow pages have duplicate entries.
**
**	A direction value of DM1C_EXACT may be passed to this routine; it is
**	treated identically to DM1C_PREV.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      page                            Pointer to the page to search 
**      direction                       Value to indicate the direction
**                                      of search when duplicate keys are 
**                                      found:
**					    DM1C_NEXT
**					    DM1C_PREV
**					    DM1C_EXACT
**	key				The specified key to search for.
**      keyno                           Value indicating how many attributes
**                                      are in key. Note that this may be LESS
**					than the total number of attributes in
**					the key if we are performing a partial
**					range search.
**
** Outputs:
**      pos                             Pointer to an area to return result.
**					See description above for the value
**					which is returned under various
**					combinations of input key and direction.
**	err_code			Set to an error code if an error occurs
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_ERROR			Error uncompressing an entry
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-oct-85 (jennifer)
**          Converted for Jupiter.
**	14-Nov-88 (teg)
**	    added initialiation of adf_cb.adb_maxstring to DB_MAXSTRING
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression. Changed comments.
**	    Added err_code argument.
**	06-may-1996 (nanpr01)
**	    Changed all page header access as macro for New Page Format
**	    project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Check for E_DB_WARNING return code from dm1cxget()
**      12-jun-97 (stial01)
**          binary_search() Pass tlv to dm1cxget instead of tcb.
**	20-nov-2006 (wanfr01)
**	    SIR 117586
**	    changed arguments for dmd_prkey
*/
static	DB_STATUS
binary_search(
    DMP_RCB      *rcb,
    DMPP_PAGE   *page,
    i4      direction,
    char         *key,
    i4	 keyno,
    DM_LINE_IDX  *pos,
    DB_ERROR	 *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_ROWACCESS	*rac;
    i4			right, left, middle;
    i4			compare;
    DB_STATUS		s;
    ADF_CB		*adf_cb;
    char		key_buf[DM1B_MAXSTACKKEY];
    char		*key_ptr, *KeyBuf, *AllocKbuf;
    i4			keylen;
    DB_ATTS             **keyatts;
    bool                is_index;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* If page has no records, nothing to search. */

    if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page) == 0)
    {
        *pos = 0;
        return (E_DB_OK);
    }

    is_index = 
        ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & DMPP_INDEX) != 0);

    /*
    ** Set up att/key arrays depending on whether this is a leaf or index page
    */
    if (is_index)
    {
	rac = &t->tcb_index_rac;
	keyatts = t->tcb_ixkeys;
    }
    else
    {
	rac = &t->tcb_leaf_rac;
	keyatts = t->tcb_leafkeys;
    }

    adf_cb = rcb->rcb_adf_cb;

    /* 
    ** Start binary search of keys in record on page.  This binary_search is
    ** different from those you'll see in textbooks because we continue
    ** searching when a matching key is found.
    **
    ** Note that we use atts_array and atts_count to extract the index entry,
    ** since these describe the entire entry, but use keyatts and keyno to
    ** perform the comparison, since these describe the search criteria.
    */
   
    left = 0;
    right = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, page) - 1;
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & DMPP_INDEX)
        right--;

    if ( s = dm1b_AllocKeyBuf((is_index) ? t->tcb_ixklen : t->tcb_klen,
				key_buf, &KeyBuf, &AllocKbuf, dberr) )
	return(s);

    
    while (left <= right)
    {
        middle = (left + right)/2;

	key_ptr = KeyBuf;
	keylen = is_index ? t->tcb_ixklen : t->tcb_klen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page,
		     rac, middle,
		     &key_ptr, (DM_TID *)NULL, (i4*)NULL,
		     &keylen, 
		     NULL, NULL, adf_cb);

	if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
	    s = E_DB_OK;

	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, rcb, page, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, middle);
	    SETDBERR(dberr, 0, E_DM93B3_BXBINSRCH_ERROR);
	}

	if ( s == E_DB_OK )
	{
	    s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, key_ptr, key, &compare );
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93B3_BXBINSRCH_ERROR);
	    }
	    else
	    {
		if (compare > DM1B_SAME)
		    right = middle - 1;
		else if (compare < DM1B_SAME)
		    left = middle + 1;
		else if (direction == DM1C_NEXT)
		    left = middle + 1;
		else
		    right = middle - 1;
	    }
	}

	if ( s )
	{
	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
	    return(s);
	}
    }
    
    *pos = left;
    
    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if (DMZ_AM_MACRO(9))
    {
	s = TRdisplay("binary_search: searched for %s occurrence of key\n",
                   (direction == DM1C_NEXT) ? "last" : "first");   
        dmd_prkey(rcb, key, is_index ? DM1B_PINDEX : DM1B_PLEAF, (DB_ATTS**)NULL, (i4)0, (i4)0);
        s = TRdisplay("\n in \n");
        dmd_prindex(rcb, page, (i4)0);
        s = TRdisplay("position found is %d\n", *pos);
    }

    return (E_DB_OK);
}

/*{
** Name: division - Finds the point of division for BTREE split.
**
** Description:
**      This routines examines the BTREE page to determine what is 
**      the best point to split the page.  It tries to make the
**      division point closest to the center such that the keys
**      to the left are strictly less than those to the right.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      buffer                          Pointer to the page want to split. 
**      keyatts				Pointer to a structure describing
**                                      the attributes making up the key.
**      keyno                           Value indicating how many attributes
**                                      are in key.
**
** Outputs:
**      divide                          Pointer to an area to return
**                                      LINE_ID of TID indicating which
**                                      record position is the division 
**                                      point.
**	err_code			Set to an error code if an error occurs
**                                      
**	Returns:
**
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-oct-85 (jennifer)
**          Converted for Jupiter.
**	14-Nov-88 (teg)
**	    added initialiation of adf_cb.adb_maxstring to DB_MAXSTRING
**	24-Apr-89 (anton)
**	    Added local collation support
**	19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	5-dec-1990 (bryanp)
**	    Call dm1cx routines to support index compression. Added err_code
**	    argument. Some changes to the key comparison loop were necessary
**	    since the midpoint determined by dm1cxmidpoint() is not necessarily
**	    exactly at the middle of the page.
**	14-dec-1992 (rogerk)
**	    Added check for split of page with 0 or 1 entries that is not
**	    a leaf page with an overflow chain.  All other types are currently
**	    unexpected.
**	06-mar-1996 (stial01 for bryanp) (bryanp)
**	    Pass page_size argument to dm1cxhas_room, dm1cxmidpoint.
**	06-may-1996 (nanpr01)
**	    Changed all page header access as macro for New Page Format
**	    project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Check for E_DB_WARNING return code from dm1cxget()
**      07-apr-97 (stial01)
**          division() NonUnique primary btree (V2) dups span leaf pages,
**          so we don't have to adjust split position to keep dups together
**          To reduce fan out for NonUnique primary btree (V2), when
**          splitting a leaf page and LRANGE == RRANGE, divide at pos 1
**      12-jun-97 (stial01)
**          division() Pass tlv to dm1cxget instead of tcb.
*/
static	DB_STATUS
division(
    DMP_RCB             *rcb,
    DMPP_PAGE          *buffer,
    DM_LINE_IDX         *divide,
    i4             	keyno,
    DB_ERROR		*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMPP_PAGE	    *b = buffer;
    DMP_ROWACCESS   *rac;
    char	    *midkey, *rkey, *lkey; 
    i4		    KeyBufSize;
    char	    AllKeys[DM1B_MAXSTACKKEY];
    char	    *AllocKbuf;
    char	    *MidKeyBuf;
    char	    *RkeyBuf;
    char	    *LkeyBuf;
    i4         	    middle, left, right; 
    i4	    	    compare;
    i4	    	    keylen;
    ADF_CB	    *adf_cb;
    DB_STATUS	    s;
    i4	    	    compression_type;
    DB_ATTS         **keyatts;
    bool	    all_keys_same;
    bool            is_index;
    char            *range_keys[2];
    i4	    	    range_lens[2];
    i4	    	    infinities[2]; 
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    *divide = 0;

    adf_cb = r->rcb_adf_cb;
    compression_type = DM1B_INDEX_COMPRESSION(r);
    is_index = 
        ((DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, b) & DMPP_INDEX) != 0);

    /*
    ** Set up att/key arrays depending on whether this is a leaf or index page
    */
    if (is_index)
    {
	rac = &t->tcb_index_rac;
	keyatts = t->tcb_ixkeys;
	keylen = t->tcb_ixklen;
    }
    else
    {
	rac = &t->tcb_leaf_rac;
	keyatts = t->tcb_leafkeys;
	keylen = t->tcb_klen;
    }

    /* Manufacture a Key buffer to contain all 3 keys */
    KeyBufSize = DB_ALIGN_MACRO(keylen);

    if ( s = dm1b_AllocKeyBuf(3 * KeyBufSize , AllKeys,
				&MidKeyBuf, &AllocKbuf, dberr) )
	return(s);

    LkeyBuf = MidKeyBuf + KeyBufSize;
    RkeyBuf = LkeyBuf + KeyBufSize;

    /*
    ** NonUnique primary btree where dups span leaf pages
    **
    ** To reduce fan out for NonUnique primary btree (V2), when
    ** splitting a leaf page and LRANGE == RRANGE, divide at pos 1
    **
    ** Note index pages are still being split at the midpoint which 
    ** is not optimum.
    */
    if ( (!is_index)
        && ((t->tcb_rel.relstat & TCB_UNIQUE) == 0)
	&& ((t->tcb_rel.relstat & TCB_INDEX) == 0)
	&& (t->tcb_dups_on_ovfl == FALSE))
    {
	compare = -1;
	range_keys[DM1B_LEFT] = LkeyBuf;
	range_keys[DM1B_RIGHT] = RkeyBuf;
	s = get_range_entries(r, b, range_keys, range_lens,
		    infinities, dberr);
	if (s != E_DB_OK)
	    SETDBERR(dberr, 0, E_DM93B4_BXDIVISION_ERROR);
	else if ( (infinities[DM1B_LEFT] == FALSE) 
		   && (infinities[DM1B_RIGHT] == FALSE))
	{
	    s = adt_kkcmp(adf_cb, (i4)keyno, t->tcb_rngkeys, LkeyBuf, RkeyBuf, 
			&compare);
	    if (s != E_DB_OK)
	    {
		uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM93B4_BXDIVISION_ERROR);
	    }
	    else if (compare == DM1B_SAME)
		*divide = 1;
	}

	if ( s || compare == DM1B_SAME )
	{
	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &MidKeyBuf);
	    return(s);
	}
    }

    /*
    ** If there is one or less records on the page, then return a split
    ** position of zero.  A split position of zero indicates that all current
    ** entries on the page are duplicates of each other (which is sort of
    ** true in this case).  The only time it is currently possible to be
    ** called to split a page with less than two entries is to split a 
    ** leaf page that has an overflow chain.
    */
    if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b) <= 1)
    {
	if (( !(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, b) & DMPP_LEAF))
		|| (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, b) == 0))
	{
	    TRdisplay("Btree split called unexpectedly on page with %d kids.\n",
			DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b));
	    TRdisplay("\tPage number %d, page overflow %d, page_stat %x\n",
			DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, b), 
			DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, b), 
			DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, b));
	}

	/* Discard any allocated key buffer */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &MidKeyBuf);
    
        return (E_DB_OK);
    }

    /* 
    ** Start at mid point and go to right to find split point. 
    ** For unique keys or index pages (which have unique keys)
    ** Then middle is always the dividng point.
    **
    ** NonUnique primary btree (V2) dups span leaf pages
    ** We don't have to adjust split position to keep duplicates together
    **
    ** Note that we use the rowaccessor to extract the index entry,
    ** but we use keyatts and keyno to perform the comparisons about where to
    ** split. The former describe the entire index entry, the latter describe
    ** just the indexing criteria.
    */

    dm1cxmidpoint(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, b, 
		compression_type, &middle);
    *divide = middle; 
    if ( !(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, b) & DMPP_LEAF) || 
                (t->tcb_rel.relstat & TCB_UNIQUE) ||
		t->tcb_dups_on_ovfl == FALSE)
    {
	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1bxsplit: dividing page %d at kid %d (of %d)\n",
			DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, b), 
			*divide, 
			DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b));
	}

	/* Discard any allocated key buffer */
	if ( AllocKbuf )
	    dm1b_DeallocKeyBuf(&AllocKbuf, &MidKeyBuf);

        return (E_DB_OK); 
    }

    /*
    ** After this point, don't "return" until the end to make
    ** sure that the Key buffers get deallocated.
    */

    midkey = MidKeyBuf;
    keylen = is_index ? t->tcb_ixklen : t->tcb_klen;
    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, b, 
		rac, middle,
		&midkey, (DM_TID *)NULL, (i4*)NULL,
		&keylen, NULL, NULL, adf_cb);

    if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
	s = E_DB_OK;

    if (s != E_DB_OK)
	dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, b, 
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, middle);

    right = left = middle; 

    all_keys_same = FALSE;
    for (++right, --left; s == E_DB_OK; ++right, --left)
    {
	if (left < 0 && 
		right >= DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b))
	{
	    all_keys_same = TRUE;
	    break;
	}

	if (right >= DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b))
	    right = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b) - 1;
	if (left < 0)
	    left = 0;

	rkey = RkeyBuf;
	keylen = is_index ? t->tcb_ixklen : t->tcb_klen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, b,
		    rac, right,
		    &rkey, (DM_TID *)NULL, (i4*)NULL,
		    &keylen, NULL, NULL, adf_cb);

	if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
	    s = E_DB_OK;

	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, b,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, right);
	    break;
	}

	lkey = LkeyBuf;
	keylen = is_index ? t->tcb_ixklen : t->tcb_klen;
	s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, b,
			rac, left,
			&lkey, (DM_TID *)NULL, (i4*)NULL,
			&keylen, NULL, NULL, adf_cb);

	if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
	    s = E_DB_OK;

	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, b, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, left);
	    break;
	}

	s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, lkey, rkey, &compare);
	if (s != E_DB_OK)
	{
	    uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    break;
	}
	if (compare != DM1B_SAME)
	    break;
    }

    /*
    ** There are 2 ways to break out of the above loop:
    **	all_keys_same == TRUE:
    **	    all keys on page are the same: set divide to 0
    **
    **	all_keys_same == FALSE:
    **	    key at left != key at right. All keys between left+1 and right-1 are
    **	    the same (and = to the midkey) If the key at left equals the midkey,
    **	    then all keys from left through right-1 are the same, and we should
    **	    split at right. If left != midkey, then we should split at left+1.
    */

    if ( s == E_DB_OK && all_keys_same )
    {
	*divide = 0;
    }
    else if ( s == E_DB_OK )
    {
	if (left < 0)
	    left = 0;
	if (right >= DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b))
	    right = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b) - 1;

	*divide = right;
	s = adt_kkcmp(adf_cb, (i4)keyno, keyatts, midkey, lkey, &compare);
	if (s != E_DB_OK)
	    uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	else if (compare != DM1B_SAME)
	    *divide = left + 1;
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &MidKeyBuf);

    if ( s == E_DB_OK && DMZ_AM_MACRO(5) )
    {
	TRdisplay("dm1bxsplit: dividing page %d at kid %d (of %d) -- (2)\n",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, b), 
		    *divide, 
		    DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, b));
	TRdisplay("          : All keys are %sthe same\n",
		    all_keys_same ? "" : "NOT ");
	TRdisplay("          : left=%d, right=%d, compare=%d\n",
		    left, right, compare);
    }

    if ( s )
	SETDBERR(dberr, 0, E_DM93B4_BXDIVISION_ERROR);

    return (s);
}

/*
** Name: get_range_entries	- Retrieve the LRANGE/RRANGE entries from page
**
** Description:
**	This little subroutine simply extracts the LRANGE and RRANGE entries
**	from the indicated page.
**
**	The LRANGE entry is placed in array member 0, which is also referred
**	to as DM1B_LEFT. The RRANGE entry is placed in array member 1, which
**	is also referred to as DM1B_RIGHT.
**
** Inputs:
**	r			Record Control Block
**	cur			page to extract entries from
**	range			Array of two key buffers
**	range_len		Array of two key lengths
**	infinity		Array of two tids
**
** Outputs:
**	err_code		Set if an error occurs
**	range[0]
**	range_len[0]
**	infinity[0]		Gets the LRANGE entry
**	range[1]
**	range_len[1]
**	infinity[1]		Gets the RRANGE entry
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	31-jan-1991 (bryanp)
**	    Created.
**	10-sep-1991 (bryanp)
**	    B37642,B39322: Check for infinity == FALSE on LRANGE/RRANGE entries.
**	06-may-1996 (nanpr01)
**	    Added page_size parameter in dm1cxlog_error routine for New Page
**	    format project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      12-jun-97 (stial01)
**          get_range_entries() Pass tlv to dm1cxget instead of tcb.
*/
static DB_STATUS
get_range_entries(
    DMP_RCB		*r,
    DMPP_PAGE		*cur,
    char		*range[],
    i4		range_len[],
    i4		infinity[],
    DB_ERROR		*dberr)
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    char	*key_ptr;
    DB_STATUS	s = E_DB_OK;
    i4	compression_type;

    CLRDBERR(dberr);

    compression_type = t->tcb_rng_rac->compression_type;

    for (;;)
    {
	key_ptr = (char *)range[DM1B_LEFT];
	range_len[DM1B_LEFT] = t->tcb_rngklen;
	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, cur, 
		DM1B_LRANGE, (DM_TID *)&infinity[DM1B_LEFT], (i4*)NULL );
	if (infinity[DM1B_LEFT] == FALSE)
	{
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, cur,
			t->tcb_rng_rac, DM1B_LRANGE,
			&key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&range_len[DM1B_LEFT], NULL, NULL, r->rcb_adf_cb);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, cur, 
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_LRANGE);
		SETDBERR(dberr, 0, E_DM93BE_BXGETRANGE_ERROR);
		break;
	    }
	    if (key_ptr != (char *)range[DM1B_LEFT])
		MEcopy(key_ptr, range_len[DM1B_LEFT], (char *)range[DM1B_LEFT]);
	}
	else if (compression_type != TCB_C_NONE)
	{
	    range_len[DM1B_LEFT] = 0;
	}

	key_ptr = (char *)range[DM1B_RIGHT];
	range_len[DM1B_RIGHT] = t->tcb_rngklen;
	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, cur, 
		DM1B_RRANGE, (DM_TID *)&infinity[DM1B_RIGHT], (i4*)NULL );
	if (infinity[DM1B_RIGHT] == FALSE)
	{
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, cur,
			t->tcb_rng_rac, DM1B_RRANGE,
			&key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&range_len[DM1B_RIGHT], NULL, NULL, r->rcb_adf_cb);
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, r, cur, 
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, DM1B_RRANGE);
		SETDBERR(dberr, 0, E_DM93BE_BXGETRANGE_ERROR);
		break;
	    }
	    if (key_ptr != (char *)range[DM1B_RIGHT])
		MEcopy(key_ptr, range_len[DM1B_RIGHT], (char *)range[DM1B_RIGHT]);
	}
	else if (compression_type != TCB_C_NONE)
	{
	    range_len[DM1B_RIGHT] = 0;
	}
	break;
    } 

    if (DMZ_AM_MACRO(5))
    {
	TRdisplay("dm1bxsplit: before split, keyrange is:\n");
	dmdprbrange(r, cur);
    }

    return (s);
}

/*
** Name: update_range_entries	    - update the range entries following split
**
** Description:
**	When a leaf split is performed, the LRANGE and RRANGE entries on the
**	resulting pages must be updated to reflect the new key ranges contained
**	on those pages. That action is performed here.
**
** Inputs:
**	rcb			Record Control Block
**	cur			current (left-hand) page of the split
**	sib			new sibling (right-hand) page of the split
**	range			low and high range keys
**	range_len		low and high range key lengths
**	infinity		infinity values for low and high range keys
**	midkey			middle key of the division
**	midkey_len		length of midkey
**
** Outputs:
**	err_code		Set if an error occurs.
**
** Returns:
**	E_DB_OK			range entries updated
**	E_DB_ERROR		error occurred
**
** History:
**	31-jan-1991 (bryanp)
**	    Created.
**	10-sep-1991 (bryanp)
**	    B37642, B39322: Don't try to compress the infinity key values. No
**	    actual key value exists, and the compress call fails.
**	06-mar-1996 (stial01 for bryanp) (bryanp)
**	    Added multiple page size support.
**	06-may-1996 (nanpr01)
**	    Changed all the page header access as macro for New Page Format
**	    project.
*/
static DB_STATUS
update_range_entries(
    DMP_RCB		*rcb,
    DMPP_PAGE		*cur,
    DMPP_PAGE		*sib,
    char		*range[],
    i4			range_len[],
    i4			infinity[],
    char		*midkey,
    i4			midkey_len,
    DB_ERROR		*dberr)
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS	    s = E_DB_OK;
    i4	    	    midkey_infinity;
    i4	    	    compression_type;
    char	    AllKeys[DM1B_MAXSTACKKEY];
    char	    *KeyBuf, *AllocKbuf = NULL;
    i4		    KeyBufSize;
    char	    *CompLrangeKeyBuf;
    char	    *CompRrangeKeyBuf;
    char	    *CompMiddleKeyBuf;
    char	    *lrange_key;
    i4	    	    lrange_keylen;
    char	    *rrange_key;
    i4	    	    rrange_keylen;
    char	    *middle_key;
    i4	    	    middle_keylen;
    i4         	    update_mode;

    CLRDBERR(dberr);

    compression_type = DM1B_INDEX_COMPRESSION(r);

    /*
    ** If compressed keys, manufacture a Key buffer with room
    ** for 3 compressed keys.
    */
    if ( compression_type != DM1CX_UNCOMPRESSED )
    {
	KeyBufSize = DB_ALIGN_MACRO(t->tcb_rngklen);

	if ( s = dm1b_AllocKeyBuf(3 * KeyBufSize, AllKeys, &KeyBuf,
				    &AllocKbuf, dberr) )
	    return(s);

	/* Carve it into 3 key spaces */
	CompLrangeKeyBuf = KeyBuf;
	CompRrangeKeyBuf = CompLrangeKeyBuf + KeyBufSize;
	CompMiddleKeyBuf = CompRrangeKeyBuf + KeyBufSize;
    }

    update_mode = DM1C_DIRECT;
    if (row_locking(r) || crow_locking(r))
	update_mode |= DM1C_ROWLK;

    for (;;)
    {
	/*
	** Update minimum and maximum key values on old and new pages. The
	** previous range of values on the page which we split is now
	** broken into two subranges, separated by the split key, 'midkey'.
	** The old page, 'cur', now contains the lower half of the range,
	** which is {range[DM1B_LEFT] .. midkey}. The new page, 'sib',
	** now contains the upper half of the range, which is
	** {midkey .. range[DM1B_RIGHT]}.
	**
	** The midkey is known not to be either -infinity or +infinity.
	**
	** If the index uses compression, we must compress the entries before
	** we update them on the page.
	*/

	if (compression_type != DM1CX_UNCOMPRESSED)
	{
	    if ( infinity[DM1B_LEFT] == FALSE )
	    {
		lrange_key = CompLrangeKeyBuf;
		s = dm1cxcomp_rec(t->tcb_rng_rac, range[DM1B_LEFT],
				    range_len[DM1B_LEFT],
				    lrange_key, &lrange_keylen );
	    }
	    else
	    {
		lrange_keylen = 0;
	    }

	    if ( infinity[DM1B_RIGHT] == FALSE && s == E_DB_OK )
	    {
		rrange_key = CompRrangeKeyBuf;
		s = dm1cxcomp_rec(t->tcb_rng_rac, range[DM1B_RIGHT],
				    range_len[DM1B_RIGHT],
				    rrange_key, &rrange_keylen );
	    }
	    else
	    {
		rrange_keylen = 0;
	    }

	    if ( s == E_DB_OK )
	    {
		middle_key = CompMiddleKeyBuf;
		s = dm1cxcomp_rec(t->tcb_rng_rac, midkey, midkey_len,
				    middle_key, &middle_keylen );
	    }

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP, (DMP_RCB *)r,
				(DMPP_PAGE *)NULL, 0, 0, 0 );
		break;
	    }
	}
	else
	{
	    lrange_key = range[DM1B_LEFT];
	    lrange_keylen = range_len[DM1B_LEFT];
	    rrange_key = range[DM1B_RIGHT];
	    rrange_keylen = range_len[DM1B_RIGHT];
	    middle_key = midkey;
	    middle_keylen = midkey_len;
	}

	midkey_infinity = FALSE;

	s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, cur,
			    update_mode, compression_type, &r->rcb_tran_id, 
			    r->rcb_slog_id_id, r->rcb_seq_number,
			    DM1B_LRANGE, lrange_key, lrange_keylen, 
			    (DM_TID *)(&infinity[DM1B_LEFT]), (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE, r, cur,
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_LRANGE );
	    break;
	}
	s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, cur,
			    update_mode, compression_type, &r->rcb_tran_id, 
			    r->rcb_slog_id_id, r->rcb_seq_number,
			    DM1B_RRANGE, middle_key, middle_keylen,
			    (DM_TID *)&midkey_infinity, (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE, r, cur,
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_RRANGE );
	    break;
	}

	s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, sib,
			    update_mode, compression_type, &r->rcb_tran_id, 
			    r->rcb_slog_id_id, r->rcb_seq_number,
			    DM1B_LRANGE, middle_key, middle_keylen,
			    (DM_TID *)&midkey_infinity, (i4)0 );
	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE, r, sib,
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_LRANGE );
	    break;
	}
	s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, sib,
			    update_mode, compression_type, &r->rcb_tran_id, 
			    r->rcb_slog_id_id, r->rcb_seq_number,
			    DM1B_RRANGE, rrange_key, rrange_keylen, 
			    (DM_TID *)(&infinity[DM1B_RIGHT]), (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E6_BAD_INDEX_REPLACE, r, sib,
		    tbio->tbio_page_type, tbio->tbio_page_size, DM1B_RRANGE);
	    break;
	}

	if (DMZ_AM_MACRO(5))
	{
	    TRdisplay("dm1bxsplit: after split, curpage keyrange is:\n");
	    dmdprbrange(r, cur);
	    TRdisplay("dm1bxsplit: after split, sibpage keyrange is:\n");
	    dmdprbrange(r, sib);
	}
	break;
    }

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
    
    if ( s )
	SETDBERR(dberr, 0, E_DM93BF_BXUPDRANGE_ERROR);
    return (s);
}

/*
** Name: update_overflow_chain
**
** Description:
**	This routine is called when a leaf page has been split. It travels
**	down the leaf's overflow chain, setting the following information on
**	each overflow page:
**
**	    page_main		set to point back to the leaf page
**	    bt_nextpage		set to point to the next leaf page
**	    LRANGE entry	set to the LRANGE entry of the leaf
**	    RRANGE entry	set to the RRANGE entry of the leaf
**
**	When this routine is called, 'lfpage' should be fixed , but lfpage's 
**	overflow chain should NOT be.
**
**	routine will fix and unfix each page on the overflow chain and will
**	return with all overflow chain pages unfixed, but 'lfpage' still fixed.
**
** Inputs:
**	r			RCB for this table
**	lfpage			the leaf page whose chain needs updating
**
** Outputs:
**	err_code		Set if an error occurs
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	14-dec-1990 (bryanp)
**	    Moved out of lfdiv for greater modularity as part of the compressed
**	    index project.
**	8-jul-1991 (bryanp)
**	    B38405: Don't log DMF external errors during split processing.
**	10-sep-1991 (bryanp)
**	    B37642, B39322: Don't try to compress the infinity key values. No
**	    actual key value exists, and the compress call fails.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-may-1996 (nanpr01 & thaju02)
**	    Changed all page header access as macro for New Page Format
**	    project.
**	    Subtract tuple header size from index entry length for leaf pages.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**      04-feb-97 (stial01)
**          Tuple headers are always on LEAF and overflow (CHAIN) pages
**      21-may-97 (stial01)
**          update_overflow_chain() Added flags arg to dm0p_unmutex call(s).
**      21-apr-98 (stial01)
**          update_overflow_chain()Set DM0L_PHYS_LOCK if extension table(B90677)
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
static DB_STATUS
update_overflow_chain(
    DMP_RCB	    *r,
    DMP_PINFO	    *lfPinfo,
    DB_ERROR	    *dberr)
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMPP_PAGE	    *lfpage;
    DMPP_PAGE	    *ovflpage;
    DMP_PINFO	    ovflPinfo;
    LG_LSN	    lsn;
    DM_PAGENO	    mainpageno; 
    DM_PAGENO	    nextpageno;
    DM_PAGENO	    nextovfl;
    char	    *lrange_key;
    char	    *rrange_key;
    char	    *olrange_key;
    char	    *orrange_key;
    i4	    lrange_keylen;
    i4	    rrange_keylen;
    i4	    olrange_keylen;
    i4	    orrange_keylen;
    DM_TID	    lrange_tid; 
    DM_TID	    rrange_tid; 
    DM_TID	    olrange_tid; 
    DM_TID	    orrange_tid; 
    i4	    compression_type;
    i4	    loc_id;
    i4	    loc_config_id;
    i4	    dm0l_flag;
    i4	    local_err_code;
    DB_STATUS	    s = E_DB_OK;
    STATUS	    cl_status;
    bool	    ovfl_mutexed = FALSE;
    bool	    critical_section = FALSE;
    CL_ERR_DESC	    sys_err;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    /* update_overflow_chain() V1 pages only */
    CLRDBERR(dberr);

    compression_type = DM1B_INDEX_COMPRESSION(r);

    lfpage = lfPinfo->page;

    ovflPinfo.page = NULL;

    /*
    ** Get the range values on the leaf page that we will be propogating
    ** to the overflow chain.  The keys do not need to be uncompressed
    ** since we are only going to log them (which we do in compressed form
    ** if the index uses compression) and reinsert them onto the overflow
    ** page.
    */
    mainpageno = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, lfpage);
    nextpageno = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, lfpage);

    lrange_keylen = t->tcb_rngklen;
    dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, lfpage, 
	DM1B_LRANGE, &lrange_key);
    dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, lfpage, 
	DM1B_LRANGE, &lrange_tid, (i4*)NULL);
    if (compression_type == DM1CX_COMPRESSED)
    {
	dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, lfpage, 
		DM1B_LRANGE, &lrange_keylen);
    }

    rrange_keylen = t->tcb_rngklen;
    dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, lfpage, 
		DM1B_RRANGE, &rrange_key);
    dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, lfpage, 
		DM1B_RRANGE, &rrange_tid, (i4*)NULL);
    if (compression_type == DM1CX_COMPRESSED)
    {
	dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, lfpage,
		DM1B_RRANGE, &rrange_keylen);
    }

    if (DMZ_AM_MACRO(5))
    {
	TRdisplay("dm1bxsplit: updating ovfl chain on page %d\n",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, lfpage));
	TRdisplay("dm1bxsplit: set page_main=%d, nextpage=%d, keyrange:\n",
		    mainpageno, nextpageno);
	dmdprbrange(r, lfpage);
    }

    nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, lfpage);	
    do	
    {
	/*
	** Update overflow chain with new information.
	** For each page on the overflow chain:
	**
	**     - Fix the page into the cache
	**     - Mutex the page
	**     - Log the update_overflow action
	**     - Change the page's bt_nextpage pointer to match the parent's
	**     - Change the page's page_main pointer to point to the parent
	**     - Change the page's range keys to match the parent leaf's
	**     - Unmutex and Unfix the page.
	*/

	s = dm0p_fix_page(r, nextovfl, DM0P_WRITE, &ovflPinfo, dberr);
	if (s != E_DB_OK)
	    break;

	/*
	** Lock buffer for update.
	*/
	dm0pLockBufForUpd(r, &ovflPinfo);

	ovflpage = ovflPinfo.page;

	/*
	** Get information on the location to which the update is being made.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, nextovfl);
	loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	/*
	** Get current range values on the overflow page so we can log them
	** for for undo processing.  The values are logged compressed if
	** compression is used so we don't need to uncompress them or make
	** copies of them.
	*/
	olrange_keylen = t->tcb_rngklen;
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage, 
		DM1B_LRANGE, &olrange_key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage,
		DM1B_LRANGE, &olrange_tid, (i4*)NULL);
	if (compression_type == DM1CX_COMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage,
		DM1B_LRANGE, &olrange_keylen);
	}

	orrange_keylen = t->tcb_rngklen;
	dm1cxrecptr(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage, 
		DM1B_RRANGE, &orrange_key);
	dm1cxtget(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage,
		DM1B_RRANGE, &orrange_tid, (i4*)NULL);
	if (compression_type == DM1CX_COMPRESSED)
	{
	    dm1cx_klen(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage,
		DM1B_RRANGE, &orrange_keylen);
	}

	/*
	** Online Backup Protocols: Check if BI must be logged before update.
	*/
	if (d->dcb_status & DCB_S_BACKUP)
	{
	    s = dm0p_before_image_check(r, ovflpage, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Log the update overflow operation.
	** After writing the log record we must succeed in making the
	** updates.  Any error during the udpate will leave the page
	** in an inconsistent state and the server will shut down in order
	** to allow REDO recovery to reconstruct the page contents.
	*/
	if (r->rcb_logging)
	{
	    /* Reserve space for log and CLR  */
	    cl_status = LGreserve(0, r->rcb_log_id, 2, 
		  2 * (sizeof(DM0L_BTUPDOVFL) - 
			    (DM1B_KEYLENGTH - lrange_keylen) -
			    (DM1B_KEYLENGTH - rrange_keylen) -
			    (DM1B_KEYLENGTH - orrange_keylen) -
			    (DM1B_KEYLENGTH - olrange_keylen)), &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		  	(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM93C4_BXUPDOVFL_ERROR);
		break;
	    }

	    /*
	    ** Mutex the page during the update operation. This must be done
	    ** BEFORE logging the udpate.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);
	    ovfl_mutexed = TRUE;

	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* 
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flag |= DM0L_PHYS_LOCK;

	    s = dm0l_btupdovfl(r->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_index_rac.compression_type, 
		t->tcb_rel.relloccount, loc_config_id, 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, ovflpage), 
		mainpageno, 
		DM1B_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, ovflpage),
	        nextpageno, 
		DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, ovflpage),
		lrange_keylen, lrange_key, &lrange_tid,
		olrange_keylen, olrange_key, &olrange_tid,
		rrange_keylen, rrange_key, &rrange_tid,
		orrange_keylen, orrange_key, &orrange_tid,
		(LG_LSN *)NULL, &lsn, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Write the LSN of the update overflow record to the page being 
	    ** updated. This marks the version of the page for REDO processing.
	    */
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(tbio->tbio_page_type, ovflpage, lsn);
	}
	else
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);
	    ovfl_mutexed = TRUE;
	}

	critical_section = TRUE;

	DM1B_VPT_SET_PAGE_MAIN_MACRO(tbio->tbio_page_type, ovflpage, mainpageno);
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(tbio->tbio_page_type, ovflpage, nextpageno);
	DM1B_VPT_SET_PAGE_STAT_MACRO(tbio->tbio_page_type, ovflpage, DMPP_MODIFY);	

	s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage,
			DM1C_DIRECT, compression_type,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4) 0,
			DM1B_LRANGE, lrange_key, lrange_keylen, 
			&lrange_tid, (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, r,ovflpage, 
		       tbio->tbio_page_type, tbio->tbio_page_size, DM1B_LRANGE);
	    SETDBERR(dberr, 0, E_DM93C4_BXUPDOVFL_ERROR);
	    break;
	}

	s = dm1cxreplace(tbio->tbio_page_type, tbio->tbio_page_size, ovflpage,
			DM1C_DIRECT, compression_type,
			(DB_TRAN_ID *)NULL, (u_i2)0, (i4) 0,
			DM1B_RRANGE, rrange_key, rrange_keylen, 
			&rrange_tid, (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, r, ovflpage, 
		       tbio->tbio_page_type, tbio->tbio_page_size, DM1B_RRANGE);
	    SETDBERR(dberr, 0, E_DM93C4_BXUPDOVFL_ERROR);
	    break;
	}

	/*
	** Save page's overflow pointer for next loop.
	*/
	nextovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(tbio->tbio_page_type, ovflpage);

	/* Release the mutex and pin, if any,  on the page. */

	dm0pUnmutex(tbio, (i4)DM0P_MUNPIN, r->rcb_lk_id, &ovflPinfo);
	ovfl_mutexed = FALSE;
	critical_section = FALSE;

	s = dm0p_unfix_page(r, DM0P_UNFIX, &ovflPinfo, dberr);
	if (s != E_DB_OK)
	    break;

    } while (nextovfl != 0); 

    /*
    ** If the above loop completed normally, then we're finished.  Otherwise
    ** fall through to error handling.
    */ 
    if (s == E_DB_OK)
	return (E_DB_OK);

    /*
    ** If we have encountered an error while a page was still mutexed, then
    ** we must fail to allow REDO recovery to run.
    */
    if (critical_section)
    {
	uleFormat(NULL, E_DM93C4_BXUPDOVFL_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	dmd_check(E_DM9C20_CRITICAL_SECTION);
    }

    if (ovfl_mutexed)
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &ovflPinfo);

    if (ovflPinfo.page)
    {
	dm0pUnlockBuf(r, &ovflPinfo);
	s = dm0p_unfix_page(r, DM0P_UNFIX, &ovflPinfo, &local_dberr);
	if (s != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93C4_BXUPDOVFL_ERROR);
    }

    return (E_DB_ERROR);
}

/*{
** Name: get_dup_key	- find the splitkey when splitting an ovfl chain.
**
** Description:
**	A special case arises when we are splitting the empty leaf page which
**	heads up a non-empty overflow chain. The keyvalue which should be used
**	for the split is the duplicate key value which currently resides on
**	this non-empty overflow chain. Since the leaf page at the head of the
**	chain is empty, we must search down the chain to find the first
**	non-empty page (there must be one, else we wouldn't be splitting this
**	leaf page), and return the first key value on that page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	current				leaf page which heads this chain.
**
** Outputs:
**	splitkeybuf			key buffer which will be filled with
**					the duplicate key value which resides
**					on this overflow chain.
**      err_code                        Pointer to an area to return 
**                                      the following errors when 
**                                      E_DB_ERROR is returned.
**                                      E_DM0055_NONEXT;
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
**	28-apr-1992 (bryanp)
**	    Created to fix bug B43636.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: As part of reworking split operations,
**	    changed this routine to look first on the current leaf to find
**	    the duplicate key and then look down the overflow chain.
**	17-dec-1992 (rogerk)
**	    Fixed bug in previous change that caused AV: temppage was being
**	    referenced instead of leafpage.
**	06-may-1996 (nanpr01)
**	    Changed all page header access as macro for New Page Format 
**	    project.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      27-feb-97 (stial01)
**          If dm1b_v2_index (page size != DM_COMPAT_PGSIZE (2k)):
**          We never reclaim space when we delete the last
**          duplicated key from a leaf page that has an overflow chain.
**          We should never have to look to the overflow pages for
**          the duplicated key value.
**          We can copy the duplicated key value from deleted tuples.
**      07-apr-97 (stial01)
**          get_dup_key() no longer called for Non unique primary btree (V2)
**      12-jun-97 (stial01)
**          get_dup_key() Pass tlv to dm1cxget instead of tcb.
*/
static DB_STATUS
get_dup_key(
    DMP_RCB             *rcb,
    DMPP_PAGE          *current,
    char		*splitkeybuf,
    DB_ERROR            *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB	        *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO	*tbio = &t->tcb_table_io;
    DMPP_PAGE          *leafpage = current; 
    DMP_PINFO		tempPinfo;
    DB_STATUS           s = E_DB_OK;
    DB_STATUS           local_status;
    char		*key_ptr;
    i4		keylen;
    i4		next_pageno;
    i4		local_error;
    bool		found_key = FALSE;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    bool		temp_locked = FALSE;

    CLRDBERR(dberr);

    if (t->tcb_dups_on_ovfl == FALSE)
    {
	TRdisplay("get_dup_key called for V2 format table");
	SETDBERR(dberr, 0, E_DM9C24_BXGETDUP_KEY);
	return (E_DB_ERROR);
    }

    tempPinfo.page = NULL;

    while (leafpage != NULL)
    {
	if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, leafpage))
	{
	    /*
	    ** Set the result key to the value of the first entry on this
	    ** overflow page:
	    */
	    key_ptr = splitkeybuf;
	    keylen = t->tcb_klen;
	    s = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage,
			&t->tcb_leaf_rac,
			(DM_LINE_IDX)0,
			&key_ptr, (DM_TID *)NULL, (i4*)NULL,
			&keylen, NULL, NULL, r->rcb_adf_cb);

	    if (t->tcb_rel.relpgtype != TCB_PG_V1 && s == E_DB_WARN)
		s = E_DB_OK;

	    if (s != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, r, leafpage,
		    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, (DM_LINE_IDX)0);
		SETDBERR(dberr, 0, E_DM93B6_BXLFDIV_ERROR);
		break;
	    }

	    /*
	    ** Make sure we fill the caller's buffer with the key value:
	    */
	    if (key_ptr != splitkeybuf)
		MEcopy(key_ptr, keylen, splitkeybuf);

	    found_key = TRUE;
	    break;
	}

	/*
	** This page is empty page. Advance to the next overflow page.
	*/
	next_pageno = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, leafpage);

	/*
	** Unfix the current chain page - but be careful not to unfix the
	** caller's leaf page.
	*/

	if (leafpage == tempPinfo.page)
	{
	    dm0pUnlockBuf(r, &tempPinfo);
	    s = dm0p_unfix_page(r, DM0P_UNFIX, &tempPinfo, dberr);
	    if (s != E_DB_OK)
		break;
	}

	/*
	** Get the next overflow page - if there is one.
	*/
	if (next_pageno != 0)
	{
	    s = dm0p_fix_page(r, next_pageno, DM0P_WRITE, &tempPinfo, dberr);
	    if (s != E_DB_OK)
		break;

	    /*
	    ** Lock buffer for update.
	    */
	    dm0pLockBufForUpd(r, &tempPinfo);
	}

	leafpage = tempPinfo.page;
    }

    /*
    ** Make sure we do not leave a chain page locally fixed.
    */
    if (tempPinfo.page != NULL)
    {
	dm0pUnlockBuf(r, &tempPinfo);
	local_status = dm0p_unfix_page(r, DM0P_UNFIX, &tempPinfo, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (s != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C24_BXGETDUP_KEY);
	return (E_DB_ERROR);
    }

    if (found_key)
	return (E_DB_OK);

    SETDBERR(dberr, 0, E_DM0055_NONEXT);
    return (E_DB_ERROR);   
}

/*{
** Name: dm1bxclean	- Remove committed deleted leaf entries.
**
** Description:
**      This routine prepares a bit map of committed deleted leaf entries.
**      Then it mutexes the page if necessary and removes the committed
**      deleted entries.
**
**	The mutex_requried flag should be set to TRUE for most invocations of
**	this routine.  It indicates that the index page passed in is shared
**	by other threads and must be protected while updated.  It can be
**	set to FALSE if the caller has a private copy of the page (eg during
**	build processing).
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**      leaf				leaf page to be cleaned
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
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      17-dec-96 (stial01)
**          Added init tup_addr, tup_hdr which inexplicably got dropped
**      27-feb-1997 (stial01)
**          Changes to dm1bxclean: Test DMPP_CLEAN in page status to 
**          avoid the overhead of checking leaf entries unecessarily.
**          Prepare list of committed transactions instead of bitmap of
**          committed entries. When LK_PH_PAGE locks are replaced with short
**          duration mutex, we release mutex while calling LG for transaction
**          status, during which time the entries on the leaf page can shift.
**      07-apr-1997 (stial01)
**          dm1bxclean() Removed code to keep overflow key on leaf,
**          This is not needed anymore.
**      21-apr-1997 (stial01)
**          dm1bxclean() delete leaf entries when page locking -> rcb_update
**          Do not clean the page if r->rcb_k_duration == RCB_K_READ
**      21-may-97 (stial01)
**          dm1bxclean() Added flags arg to dm0p_unmutex call(s).
**          The caller must have X page lock or BUF_MLOCK on the buffer.
**      12-jun-97 (stial01)
**          dm1bxclean() Pass tlv to dm1cxget instead of tcb.
**      29-jul-97 (stial01)
**          dm1bxclean() removed dm1cxdel-rcb_update that was for 
**          page locking/active rcb_update. We always use passive rcb update
**          if page size != DM_COMPAT_PGSIZE, so we should NEVER clean
**          deleted entries without updating clean count on page.
**      15-jan-98 (stial01)
**          dm1bxclean() Fixed dm1cxclean parameters, error handling
**      12-feb-98 (stial01)
**          FIXED dm1cxget error handling.
**	15-jan-1999 (nanpr01)
**	    pass pointer to pointer in dm0m_deallocate routine.
**      15-apr-99 (stial01)
**          As of 07-jul-98, we don't put tranid in tuphdr when page locking
**          Primary non-uniq btree: do not clean deleted records if any cursor
**          is positioned on this leaf.
**          Also defer cleaning if there is still room for one more key.
**	20-oct-1999 (nanpr01)
**	    Optimized the code to call dm1cxget_deleted instead of dm1cxget
**	    to minimize the tuple header copy. 
*/
DB_STATUS
dm1bxclean(
    DMP_RCB             *rcb,
    DMP_PINFO           *leafPinfo,
    DB_ERROR            *dberr)
{
    DMP_RCB         *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *leaf;
    DB_STATUS       status = E_DB_OK;
    i4         clean_count;
    DM_TID          localbid;
    bool	    update_mode;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    leaf = leafPinfo->page;

    if (t->tcb_rel.relpgtype == TCB_PG_V1)
	return (E_DB_OK);

    if ((DM1B_V2_GET_PAGE_STAT_MACRO(leaf) & DMPP_CLEAN) == 0)
	return (E_DB_OK);

    /*
    ** If row locking or physical page locking and uncompressed,
    ** defer cleaning until there is no more space
    */
    if ((row_locking(r) || crow_locking(r) || 
    		(t->tcb_extended && r->rcb_lk_type == RCB_K_PAGE))
	&& DM1B_INDEX_COMPRESSION(r) == DM1CX_UNCOMPRESSED
	&& DM1B_V2_GET_BT_KIDS_MACRO(leaf) < t->tcb_kperleaf)
	return (E_DB_OK);

    /*
    ** Don't clean the page if we do not have a lock
    **    Currently in dm2a.c, a rcb is allocated where the base rcb has
    **    rcb_access_mode = RCB_A_READ, rcb_k_duration RCB_K_READ.
    **    The rcb allocated inherits rcb_k_duration RCB_K_READ,
    **    but is assigned rcb_access_mode RCB_A_WRITE.
    */
    if (r->rcb_k_duration == RCB_K_READ)
	return (E_DB_OK);

    /*
    ** dm1cxclean must check if the delete is committed,
    ** as the deleted key may be needed for repositioning
    ** e.g. update x set prim_uniq_key = prim_uniq_key + 100 (change 441368)
    */
    update_mode = (DM1C_DIRECT | DM1C_ROWLK);

    clean_count = DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(t->tcb_rel.relpgtype, leaf);

    /*
    ** if nologging and this session is doing base_delete_put...
    ** we can't clean the deleted key as it might be needed for reposition
    **
    ** NB: "nologging" is not permitted when crow_locking
    */
    if (!r->rcb_logging &&
	(row_locking(r) || t->tcb_seg_rows))
	return (E_DB_OK);

    /*
    ** If the buffer resides in the DBMS buffer cache, then we have
    ** to mutex it while we change it to prevent other threads from  
    ** looking at it while it is inconsistent.
    */
    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);

    status = dm1cxclean(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf,
		DM1B_INDEX_COMPRESSION(r), update_mode, r);
    if (status != E_DB_OK)
	SETDBERR(dberr, 0, E_DM93C7_BXCLEAN_ERROR);

    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);

    /*
    ** Find any other RCB's whose table position pointers may have been
    ** affected by this delete and update their positions accordingly.
    **
    */
    if (status == E_DB_OK &&
	DM1B_VPT_GET_BT_CLEAN_COUNT_MACRO(t->tcb_rel.relpgtype, leaf)
	!= clean_count)
    {
	localbid.tid_tid.tid_page = DM1B_V2_GET_PAGE_PAGE_MACRO(leaf);
	localbid.tid_tid.tid_line = DM_TIDEOF;
	status = dm1b_rcb_update(r, &localbid, (DM_TID *)NULL, (i4)0,
		    DM1C_MCLEAN, (i4)TRUE, (DM_TID *)NULL, (DM_TID *)NULL, 
		    (char *)NULL, dberr);
    }

    /*
    ** Handle error reporting and recovery.
    */
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM93C7_BXCLEAN_ERROR);
    }
    return (status);
}


/*{
** Name: dm1bxreserve   - Reserve leaf entry
**
** Description:
**      This routine reserves space on leaf by inserting the leaf entry
**      and then marking it deleted.
**      The page must be mutexed by the caller
**
** Inputs:
**      rcb                             Pointer to record access context
**      leaf				The leaf page 
**      bid                             The bid->tid_tid.tid_line indicates
**                                      where on the leaf the insertion is 
**                                      to take place.
**      key                             The key to be inserted
**      allocate                        Allocate entry TRUE/FALSE
**      alloc_tidp			tidp to use for entry we reserve
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
**      21-apr-97 (stial01)
**          Created.
**      29-jul-97 (stial01)
**          dm1bxreserve() Call dm1b_rcb_update
**      05-oct-98 (stial01)
**          dm1bxreserve() Moved initialization of update_mode
*/
DB_STATUS
dm1bxreserve(
    DMP_RCB         *rcb,
    DMP_PINFO       *leafPinfo,
    DM_TID          *bid,
    char            *key,
    bool            allocate,
    bool            mutex_required,
    DB_ERROR        *dberr)
{
    DMP_RCB         *r = rcb;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DMPP_PAGE	    *leaf;
    DB_STATUS       s;
    DM_TID          base_tid;
    bool            has_room;
    DM_LINE_IDX     child;
    i4         	    keylen = t->tcb_klen; 
    char	    *AllocKbuf = NULL, *KeyBuf;
    char            CompressedKey[DM1B_MAXSTACKKEY];
    i4         	    crec_size;
    i4         	    dmcx_flag = 0;
    bool            mutex_done = FALSE;
    i4         	    update_mode;
    i2		    TidSize;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Return if entry already reserved */
    if (DM1B_POSITION_VALID_MACRO(r, RCB_P_ALLOC))
	return (E_DB_OK);

    leaf = leafPinfo->page;

    /*
    ** If we do not have the bid->tid_tid.tid_page fixed,
    ** this is an error
    */
    child = bid->tid_tid.tid_line;
    if (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leaf) !=
	bid->tid_tid.tid_page ||
	DMPP_VPT_IS_CR_PAGE(t->tcb_rel.relpgtype, leaf))
    {
	dm1cxlog_error(E_DM93EE_BAD_INDEX_RESERVE, r, leaf, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
	return (E_DB_ERROR);
    }

    /* Extract TID size from the leaf */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(t->tcb_rel.relpgtype, leaf);

    do 
    {
	if (mutex_required)
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);
	    mutex_done = TRUE;
	}

	/*
	** If the index uses compressed key entries then compress the key
	** to be inserted.
	*/
	if (DM1B_INDEX_COMPRESSION(r) != DM1CX_UNCOMPRESSED)
	{
	    if ( s = dm1b_AllocKeyBuf(keylen, CompressedKey,
					&KeyBuf, &AllocKbuf, dberr) )
		break;
	    s = dm1cxcomp_rec(&t->tcb_leaf_rac,
		    key, keylen,
		    KeyBuf, &crec_size );
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E1_BAD_INDEX_COMP, r, leaf,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child );
		break;
	    }

	    key = KeyBuf;
	    keylen = crec_size;
	}

	/*
	** Reserve is always DIRECT update
	** If deferred, we'll do deferred processing when we do update
	** Same as in dm1r_allocate
	*/
	update_mode = DM1C_DIRECT | DM1C_ROWLK;

	if (allocate == TRUE)
	{
	    /*
	    ** Make sure there is room, caller should have done this already
	    */
	    has_room = dm1cxhas_room(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			leaf, DM1B_INDEX_COMPRESSION(r),
			(i4)100 /* fill factor */,
			t->tcb_kperleaf, 
			(t->tcb_klen + t->tcb_leaf_rac.worstcase_expansion + TidSize));
	    if (has_room == FALSE)
	    {
		uleFormat(NULL, E_DM9C1E_BXKEY_ALLOC, (CL_ERR_DESC*)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		    0, child, 0, 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, leaf),
		    0, DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, leaf), 0, 
		    keylen);
		s = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Allocate space on the page for the new key,tid pair.
	    */
	    s = dm1cxallocate(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf,
				update_mode, DM1B_INDEX_COMPRESSION(r),
				&r->rcb_tran_id, r->rcb_seq_number, child,
				keylen + TidSize );
	    if (s != E_DB_OK)
	    {
		dm1cxlog_error( E_DM93E0_BAD_INDEX_ALLOC, r, leaf, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child );
		break;
	    }
	}

	/*
	** Insert the new key,tid values.
	*/
	s = dm1cxput(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf,
			    DM1B_INDEX_COMPRESSION(r), update_mode, 
			    &r->rcb_tran_id, r->rcb_slog_id_id, r->rcb_seq_number, 
			    child, key, keylen, 
			    &r->rcb_alloc_tidp, (i4)0);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E4_BAD_INDEX_PUT, r, leaf, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
	    break;
	}

	/*
	** Mark leaf entry deleted
	*/
	s = dm1cxdel(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leaf,
			update_mode, DM1B_INDEX_COMPRESSION(r),
			&r->rcb_tran_id, r->rcb_slog_id_id, r->rcb_seq_number, 
			dmcx_flag, child);
	if (s != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, r, leaf, 
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
	    break;
	}

#ifdef xDEV_TEST
	TRdisplay("dm1bxreserve(%d): %~t, leaf %d btkids %d \n", 
		       r->rcb_tran_id.db_low_tran,
                       sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
                       DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, leaf),
		       DM1B_VPT_GET_BT_KIDS_MACRO(tbio->tbio_page_type, leaf));
#endif

	/*
	** Increment clean count if we allocated space, 
	** causing entries to shift
	** Note dm1cxallocate, dm1cxput, dm1cxdel set the MODIFY bit
	*/
	if (allocate == TRUE)
	    DM1B_VPT_INCR_BT_CLEAN_COUNT_MACRO(t->tcb_rel.relpgtype, leaf);

	DM1B_POSITION_SAVE_MACRO(r, leaf, bid, RCB_P_ALLOC);

	s = dm1b_rcb_update(r, bid, (DM_TID *)NULL, (i4)0,
		    DM1C_MRESERVE,
		    (i4)0, (DM_TID *)NULL, (DM_TID *)NULL, (char *)NULL, dberr);


    } while (FALSE);

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    if (mutex_done == TRUE)
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, leafPinfo);

    if (s != E_DB_OK)
    {
	dm1cxlog_error(E_DM93EE_BAD_INDEX_RESERVE, r, leaf, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, child);
    }

    return (s);
}


/*{
** Name: dm1bx_lk_convert   Converts locks on newly allocated pages 
**                          (PHYSICAL X -> LOGICAL IX)
**                          Also propogates lock on leaf page that was split.
**
** Description:
**      This routine converts locks on newly allocated pages
**
** Inputs:
**      rcb                             Pointer to record access context
**      insert_direction
**      current
**      sibling
**      sib_lkid
**      data_lkid
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
**      05-oct-98 (stial01)
**          dm1bx_lk_convert() Test if (still) row locking before LKpropagate
*/
static DB_STATUS
dm1bx_lk_convert(
DMP_RCB *r,
i4   insert_direction,
DMP_PINFO *currentPinfo,
DMP_PINFO *siblingPinfo,
LK_LKID   *sib_lkid,
LK_LKID   *data_lkid,
DB_ERROR  *dberr)
{
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_DCB         *d = t->tcb_dcb_ptr;
    DB_TAB_ID       tabid = t->tcb_rel.reltid;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM_PAGENO       sib_page;
    DM_PAGENO       data_page;
    bool	    data_locked = FALSE;
    LK_LOCK_KEY     curlockkey;
    LK_LOCK_KEY     siblockkey;
    CL_ERR_DESC      sys_err;
    STATUS          cl_status;
    DB_STATUS       s;
    i4		    local_err;
    STATUS	    local_status;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);



    s = E_DB_OK;
    sib_page = DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, siblingPinfo->page);
    data_page = DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, siblingPinfo->page);

    /* 
    ** For new leaf and new data:
    ** Downgrade X to IX, convert PHYSICAL to logical
    ** 
    ** The buffer for the current leaf page is locked.
    ** No waiting or escalations must occur during the lock conversion(s).
    ** The physical to logical lock conversion in dm1r_lk_convert will
    ** pass the LK_IGNORE_LOCK_LIMIT flag which should guarantee its success.
    */
    local_status = dm1r_lk_convert(r, sib_page, siblingPinfo,
    				sib_lkid, &local_dberr); 
    if (local_status != E_DB_OK)
    {
	uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &local_err, 0);
	TRdisplay("Btree split: lock on new LEAF failed %d\n", local_dberr.err_code);
	s = E_DB_ERROR;
    }

    /* 
    ** New data only for primary btree
    */
    if ((t->tcb_rel.relstat & TCB_INDEX) == 0)
    {
	local_status = dm1r_lk_convert(r, data_page, (DMP_PINFO*)NULL,
					data_lkid, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &local_err, 0);
	    TRdisplay("Btree split: lock on new DATA failed %d\n", local_dberr.err_code);
	    s = E_DB_ERROR;
	}
    }

    /*
    ** Propagate all intent page locks on the current page to the
    ** sibling leaf page (if still row locking)
    ** This should always succeed, because we have the parent index page locked
    ** and also the current leaf is mutexed.
    */
    if (row_locking(r))
    {
	curlockkey.lk_type = LK_PAGE;
	curlockkey.lk_key1 = (i4)d->dcb_id;
	curlockkey.lk_key2 = tabid.db_tab_base;
	curlockkey.lk_key3 = tabid.db_tab_index;
	curlockkey.lk_key4 = 
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, currentPinfo->page);
	curlockkey.lk_key5 = 0;
	curlockkey.lk_key6 = 0;

	/* build lock key for sibpage */

	siblockkey.lk_type = LK_PAGE;
	siblockkey.lk_key1 = (i4)d->dcb_id;
	siblockkey.lk_key2 = tabid.db_tab_base;
	siblockkey.lk_key3 = tabid.db_tab_index;
	siblockkey.lk_key4 = DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type,
			    siblingPinfo->page);
	siblockkey.lk_key5 = 0;
	siblockkey.lk_key6 = 0;

	/*
	** Call LKpropagate to acquire all granted and converted
	** locks which exist on curpage for sibpage on the correct
	** lock lists.  Must use MULTITHREAD flag to propagate locks.
	*/
#ifdef xDEV_TEST
	TRdisplay("LKpropagate  %~t %d %d\n",  
	    sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, currentPinfo->page),
	    DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, siblingPinfo->page));
#endif

	cl_status = LKpropagate(LK_MULTITHREAD, r->rcb_lk_id, &curlockkey, 
			    &siblockkey, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    TRdisplay("Btree split: LKpropagate ERROR  %~t %d %d\n",  
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, currentPinfo->page),
		DM1B_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, siblingPinfo->page));
	    SETDBERR(dberr, 0, E_DM93C8_BXLOCKERROR_ERROR);
	    s = E_DB_ERROR;
	}
    }

    if (s != E_DB_OK)
	SETDBERR(dberr, 0, E_DM93C8_BXLOCKERROR_ERROR);

    return (s);
}
