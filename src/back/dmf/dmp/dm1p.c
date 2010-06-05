/*
** Copyright (c) 1990, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
#include    <me.h>
#include    <di.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm0m.h>
#include    <dmpl.h>
#include    <dm1p.h>
#include    <dm1c.h>
#include    <dm1u.h>
#include    <dm2f.h>
#include    <dmxe.h>
#include    <dmftrace.h>
#include    <dudbms.h>

/**
**
**  Name: DM1P.C - Space management routines for disk pages.
**
**  Description:
**      This file contains all the routines needed to manipulate the
**	space management structures for pages.  These routines include
**	routine to assist in building a new file, managing space maps,
**	verifying and/or rebuilding space maps in event of corruption.
**
**      The external routines defined in this file are:
**
**	dm1p_free	    - Mark a range of pages free.
**	dm1p_getfree	    - Get a free page from free list.
**	dm1p_putfree	    - put a free page on free list.
**	dm1p_checkhwm	    - Check current highwater mark.
**	dm1p_lastused	    - Find last used non-FMAP or FHDR page.
**	dm1p_rebuild	    - Scan convert BTREE for empty data pages.
**	dm1p_dump	    - debug routine to dump bitmaps.
**	dm1p_verify	    - Routine called by verify.
**	dm1p_single_fmap_free - Mark a range of pages as free in a FMAP.
**	dm1p_used_range	    - Mark range of pages as used.
**	dm1p_init_fhdr	    - Initialise a FHDR page.
**	dm1p_fminit	    - Initialise a FMAP page.
**	dm1p_add_fmap	    - Initialise a FMAP and add to the FHDR.
**	dm1p_build_SMS	    - Build a tables FHDR/FAMp.
**	dm1p_add_extend	    - Add N pages to a table.
**
**  History:
**      14-mar-90 (jennifer)
**          Created.
**	08-jul-1991 (Derek)
**	    Added support for new DM1P_NOREDO and DM1P_NEEDBI flags for
**	    dm1p_getfree.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**      23-oct-91 (jnash)
**          Within dm1p_free, add error code param to dm1pxfree.
**	25-oct-1991 (bryanp)
**	    Added dm1p_force_map to handle map page issues during mini-xacts.
**      28-oct-91 (jnash)
**          Within verify_bitmap, initialize last_fmap to zero (caught by LINT).
**	31-oct-91 (rogerk)
**	    Fixed bug in dm1p_putfree when called on pre-6.5 format table.
**	    Uninitialized fhdr value caused us to try to unfix garbage pointer.
**	 3-oct-91 (rogerk)
**	    Added fixes for recovery of File Extend operations.
**	    Until we figure out how to properly do REDO recovery without
**	    forcing FHDR/FMAP updates done in MINI-XACTS, we must force updates
**	    made while in them.  Add support for forcing MAP/HEADER pages
**	    by adding a "force" flag to unfix_header and unfix_map routines.
**	    Also force FMAPs as well as FHDR during extend operation as
**	    we don't currently do any REDO recovery of the extend record.
**	    Added "force" flag to mark_free routine to specify that the fmap
**	    page should be forced if unfixed.
**	    Updated dm1p_force_map routine to use the new unfix_map/unfix_header
**	    interface and to force the header as well as the map pages.
**	20-Nov-1991 (rmuth)
**	    Removed the action type DM0P_FHFMPSYNC from dm0p_fix_page so
**	    took out the setting of this action.
**	    Altered scan_map() for new action DM1P_MINI_TRAN, see code
**	    for comments.
**	4-Dec-1991 (bryanp)
**          Changed extend()'s processing when it is allocating space for an
**          already-formatted FHDR so that it doesn't format an extra FMAP.
**	13-Dec-1991 (rmuth)
**	    Added new error messages E_DM93AB_MAP_PAGE_TO_FMAP, 
**	    E_DM92DD_DM1P_FIX_HEADER, E_DM92DE_DM1P_UNFIX_HEADER,
**	    E_DM92EE_DM1P_FIX_MAP, E_DM92EF_DM1P_UNFIX_MAP.
**	10-Feb-1992 (rmuth)
**	    Added another new error message, E_DM92ED_DM1P_ERROR_INFO.
**	    Altered dm1p_checkhwm() to set err_code if it has a problem
**	    unfixing the FHDR.
**      10-mar-1992 (bryanp)
**          Don't need to force fmap's & fhdrs for temporary tables.
**	23-March-1992 (rmuth)
**	    Added more error handling. Also corrected error handling in
**	    some routines which were causing random accesss violations.
**	31-March-1992 (rmuth)
**	    Added function prototypes.
**    	16-jul-1992 (kwatts)
**          MPF project. Make check for empty page an accessor call.
**	29-August-1992 (rmuth)
**	    Various file extend changes.
**		- Remove the on-the-fly conversion process.
**		- Removed dm1pxfmap, replaced by dm1p_add_fmap.
**		- Removed dm1pxfhdr, replaced by dm1p_init_fhdr.
**		- Exposed the trace points outside of xDEBUG.
**		- Add force_page, removing explict dm0p_unfix_page
**		  calls to force a FHDR to disc.
**		- Add dm1p_convert_table_to_65
**	11-sep-1992 (bryanp)
**	    If no RCB is passed to fhdr build routines, use default local
**	    accessors for page formatting.
**	17-Sept-1992 (rmuth)
**	    Used stack variables for FHDR/FMAP pages instead of calling
**	    dm0m_allocate. Need to create a new type if want to do this.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	30-October-1992 (rmuth)
**	    - extend() Changed for new file extend DI code, ie for temporary 
**	      tables use dmf_alloc_file() to extend a table for all other 
**	      tables use dm2f_galloc_file().
**	    - dm1p_getfree() set tbio_alloc,tbio_lpageno and tbio_checkeof.
**	10-November-1992 (rmuth)
**	    Remove calls to check_sum() this function has been retired
**	    see above Reduced logging Project.
**	14-dec-1992 (rogerk & jnash)
**	    Reduced Logging Project (phase 2): 
**	      - Initialize last page pointer before calling dm2f_alloc_file. If
**		we don't, then the allocate routine does not know how to 
**		proportion up the allocate among the underlying locations.
**	      - Took out check_sum routine and the use of special checksum logic
**		on FHDR and FMAP pages.  Ingres now checksums all pages through
**		the buffer manager.
**	      - Removed the DM0P_MUTEX fix action when fixing FMAP and FHDR
**		pages.  Code which updates these pages must now explicitly
**		mutex the pages while updating them.
**	      - Removed requirement that FHDR and FMAP pages be written at
**		unfix time when they are modified.  They are now recovered
**		through standard REDO recovery actions.
**	      - Removed Before Image handling when fixing FMAP and FHDR pages.
**		Before Images are now generated as needed by the Buffer Manager.
**	      - Changed to fix fmap/fhdr pages for READ if DM1P_FOR_UPDATE flag
**		is not passed to the fix_map/fix_header call.
**	      - Moved macro definitions to dm1p.h so they can be used by
**		recovery code.
**	      - Substitute dmpp_tuplecount() for dmpp_isempty().
**	      - Changed init_fmap to dm1p_fminit so it can be called by
**		recovery code.
**	      - Added dm0p_pagetype calls when scratch pages are fixed in the 
**		Buffer Manager and when put_page_build_SMS copies a new page
**		into the Buffer Manager.
**	      - Took out old references to DM1P_NOREDO and DM1P_NEEDBI.
**	      - Insert checksum prior to page write.
**	      - Added flag argument to extend routine to pass mini-xact status.
**	      - Removed dm1p_force_map routine.
**	      - Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-1992 (jnash)
**	    Reduced Logging Fix.  Add loc_cnf param to dm0l_extend, 
**	    required by rollforward.
**	14-dec-1992 (rogerk)
**	    When create new page in the buffer manager, set its page status
**	    to indicate that it is modified so that if the buffer manager
**	    decides to toss the page it will not lose its contents if the
**	    table is in deferred IO state.
**	02-jan-1993 (jnash)
**	    More reduced logging work.  Add LGreserve calls.
**	11-jan-1993 (mikem)
**	    Lint directed "used before set" fixes.
**	14-jan-1992 (rmuth)
**	     Add check to make sure table does not grow past 
**	     DM1P_MAX_TABLE_SIZE.
**	18-jan-1993 (rogerk)
**	    Set pages_mutexed flag correctly after mutexing pages in extend.
**	30-feb-1993 (rmuth)
**	     Various changes
**	      - dm1p_dump, When displaying pagetypes show a different type 
**	        for overflow data pages.
**	      - dm1p_dump, When display HASH tables display the pagetypes both
**	        in sequential and structure order, so can match overflow
**	        chains to hash buckets.
**	      - FHDR/FMAP rebuild code, add code to enable the following
**	        - The patch table code to add new FHDR/FMAP(s) to the end
**	          of the table marking all pages as used, dm1p_rebuild.
**	        - The table_repair code to add new FHDR/FMAP(s) to an
**	          existing table, dm1p_repair. This sets up some context
**	          and then calls dm1p_rebuild.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**	30-mar-1993 (rmuth)
**	    Add dm1p_add_extend.
**      26-apr-1993 (bryanp/andys)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Catch failures of dm0p_fix_page in scan_map
**		    by changing a do..while loop to a while loop which we
**		    will not enter if dm0p_fix_page failed.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-July-1993 (rmuth)
**	   - If relfhdr is corrupt and points to past end-of-table then
**	     catch this and pass back a warning. Used to rebuild table.
**	   - dm1p_build_SMS was comparing pages used and page numbers, this
**	     is incorrect as page numbers start from zero. Only showed up
**	     if table was extacly DM1P_FSEG size.
**	   - scan_map was sometimes not setting the fmap_hw_mark.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.  In most cases here, we
**	    need to pass 'force' flag to LGreserve to request reservation
**	    of space required for forces performed during recovery. 
**	18-oct-1993 (jnash)
**	    Avoid call to dm0p_before_image_check() if we do not update the
**	    fhdr.
**	 3-jan-1994 (rogerk)
**	    Changed dm1p_putfree to update page_tran_id on the page being freed
**	    so that the allocate code can correlate a potential free page to
**	    with the transaction that freed it.
**	24-jan-1994 (gmanning)
**	    Change unsigned i4 to u_i4 to compile in NT environment.
**	18-apr-1994 (jnash)
**	    fsync project.  Eliminate calls to dm2f_force_file().
**	8-apr-1994 (rmuth)
**	   - b61019, If adding the process of adding FHDR/FMAP(s) causes an
**	     a file extend we were incorrectly formatting the FHDR/FMAP(s).
**	   - b62487, fix_header(), fix_map() where incorrectly seting error
**	     code.
**	   - b58422, dm1p_log_error(), if encounter a DEADLOCK error suppress
**	     the information message as causes much unnecessary stress among
**	     users.
**	15-apr-1994 (kwatts)
**	   - b64114, always put an EXTEND log record in its own mini-xact,
**	     even if that means nesting minis.
**	04-jan-1995 (cohmi01)
**	    For RAW IO - add parm to dm1p_add_extend indicating whether
**	    adding x number of new pages, or x pages in total.
**	    Utilize unused 'flag' parm of extend() for same purpose.
**	    Change extend() to calc pages based on this new flag.
**      23-mar-1995 (chech02) bug 67533
**         - b67533, dm1p_convert_table_to_65(), update tcb_page_adds from
**           SMS struct. tcb_page_adds will be used to update relpages of
**           iirelation table.
**	14-sep-1995 (nick)
**	    Need for additional FMAPs was calculated incorrectly. #70798
**	26-feb-1996 (pchang)
**	    When rebuilding an existing table's FHDR/FMAPs after data had been
**	    loaded, existing FMAPs that contained free pages prior to loading
**	    were not rebuilt correctly, which resulted in FMAP inconsistency 
**	    and orphan data pages that could not be referenced (B74873). 
**	    While we're here, corrected a comment in dm1p_used_range().
**	30-may-96 (nick)
**	    Changes for verifydb.
**	30-may-1996 (pchang)
**          Added DM0P_NOESCALATE action when fixing free pages in scan_map().
**	    This is necessary because the rountine is called with the FHDR 
**	    fixed and lock escalation there may lead to unrecoverable deadlock 
**	    involving the table-level lock and the FHDR. (B76463)
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for multiple page sizes.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_read_file()
**	06-may-1996 (thaju02)
**	    New page format support: 
**		Change page header references to use macros.
**		Add page_size parameter to dm1p_init_fhdr, dm1p_add_fmap,
**		dm1p_single_fmap_free, dm1p_used_range, dm1p_fmfree, 
**		dm1p_fmused, dm1p_fminit.
**	21-may-1996 (nanpr01)
**          Account for 64-bit tid support and multiple FHDR pages.
**	02-jul-1996 (pchang)
**	    Added DM0P_NOESCALATE action when fixing the file header itself
**	    since lock escalation there may also lead to unrecoverable deadlock.
**	17-Jul-96 (pchang)
**	    When checking if our tbio_lpageno is stale in dm1p_getfree(), we 
**	    need to cast newpage->page_page to a i4 because tbio_lpageno 
**	    could contain the value -1 which would otherwise be converted 
**	    incorrectly into unsigned MAX_I4 (B76526). 
**	23-jul-96 (nick)
**	    Changes to solve combined #76463 and 77890.
**	03-sep-1996 (pchang)
**	    Fixed bug 78492.  Set new_physical_pages to old_physical_pages in
**	    extend() if no new physical allocation is needed for the extension.
**	    Fixed bug 77416.  We should take into account any unused pages that 
**	    exist in a table prior to the extension in extend().  Any new FMAPs
**	    should start to occupy from an unused page (if any) instead of a 
**	    newly allocated page.  Added last_used_page to dm0l_extend() and 
**	    dm0l_fmap() calls to enable undo and redo code to properly deal 
**	    with existing unused pages also.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled;
**          Changed arguments of dm0p_trans_page_lock().
**      12-dec-96 (dilma04)
**          If row locking, do not set DM0P_PHYSICAL flag when fixing a page 
**          for CREATE in scan_map(). This fixes bug 79611.
**      06-jan-97 (dilma04)
**          Change fix for b79611 because intent page locks are not requested
**          in PHYSICAL mode by default anymore.
**	23-jan-97 (hanch04)
**	    Added SMS_build_cb.SMS_page_size for upgrading
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          add lock_id parameter to dm0p_trans_page_lock() call.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_close_page() calls.
**      21-apr-98 (stial01)
**          Set DM0L_PHYS_LOCK in log rec if extension table (B90677)
**	01-Jul-1998 (jenjo02)
**	  o Redesigned FHDR/FMAP fixing/locking scheme to encourage thread
**	    concurrency during free page allocation (dm1p_getfree()) and
**	    deallocation (dm1p_putfree()). FHDR lock can no longer be assumed
**	    to be sufficient protection of the underlying FMAPs. FHDRs and
**	    FMAPs are now always fixed for UPDATE, but may be locked either
**	    LK_IS or LK_SIX depending on the needs of the current operation.
**	  o Added (DMPP_PAGE **) output parm to dm1p_lastused(). If passed by
**	    caller, the "lastused" page will be fixed while the FMAP is fixed
**	    and locked, closing a large window in dm1s whereby the "lastused"
**	    page could be deallocated by recovery before being fixed by the
** 	    caller of dm1p_lastused().
**	  o Generally cleaned up use of DMPP_ FHDR/FMAP page macros, keeping
**	    their use to a minimum by extracting the variables into local stack
**	    variables using the pagesize-dependent macros and then referencing
**	    the local variables.
**	27-Jul-1998 (jenjo02)
**	    Closed a window in scan_map in which stale free bit information was
**	    being used, resulting in the same free page being selected and
**	    formatted by multiple threads. When row locking, free page is now
**	    fixed X, not IX, and will be converted to IX after the FHDR/FMAP
**	    is updated.
**	    Changed FHDR/FMAP locking scheme to use IS/IX/SIX locks instead of
**	    S/X to furthur improve concurrency.
**	11-Aug-1998 (jenjo02)
**	    When row locking and DM1P_PHYSICAL flag is present, scan_map should
**	    not convert newly allocated page's X lock to IX, but leave it at X.
**	19-Aug-1998 (jenjo02)
**	    When converting rowlocking PHYSICAL pagelock from X to logical IX, 
**	    it must be done in two steps: 1st, explicitly convert PHYSICAL X to IX,
**	    2nd, implicitly convert PHYSICAL IX to logical.
**	01-Sep-1998 (jenjo02)
**	    Well, testing after the earlier concurrency effort uncovered a gaping
**	    hole. Thread A could pick free page 1 and attempt to fix it, but be
**	    put to sleep by the OS. While asleep, thread B also picks free page 1,
**	    fixes it, formats it, and ends its transaction. When thread A finally
**	    awakes, it still thinks that free page 1 is available and formats it.
**	    The solution is to pass a new dm0p flag, DM0P_CREATE_NOFMT, when 
**	    fixing the page. Instead of formatting the page, dm0p will mutex it
**	    and return the fixed page in that state, after which scan_map now
**	    retests the free bit for the page; if off (another thread stole the
**	    page), the page is unmutexed and unfixed, and another candidate 
**	    found. This also eliminates the need to use lock_values with FMAPs.
**	09-Sep-1998 (jenjo02)
**	    In dm1p_lastused(), if more than 32 used pages, the last allocated 
**	    page was being returned, a bug.
**	23-Sep-1998 (jenjo02)
**	    Whoops! If we must write a BI for the freshly allocated page, be
**	    sure to format it first!
**	02-Oct-1998 (jenjo02)
**	    In scan_map(), if FHDR must be relocked, recheck the free bit to
**	    make sure the allocated page has not been stolen.
**	29-Oct-1998 (jenjo02)
**	    dm1p_lastused():
**	    Added DM0P_PHYSICAL_IX flag to cause the refix of the last page
**	    to take an IX page lock instead of X when row locking. 
**	    Test failure of fix_page and allow certain errors (like last 
**	    page no longer exists).
**	16-Nov-1998 (jenjo02)
**	    Using IS/SIX locks for the FHDR leaves a window whereby the
**	    underlying FMAP can be left with no lock, hence no protection.
**	    FHDRs are now locked IX/X to close this window. Added defines
**	    for FHDR/FMAP read/update lock modes so they don't have to be
**	    hard-coded wherever used.
**	21-Jan-1998 (jenjo02)
**	    Avoid deadlock with CP by releasing allocated page's mutex before
**	    calling LGreserve().
**	22-Feb-1999 (jenjo02)
**	    Check DCB_S_EXCLUSIVE instead of dcb_ref_count when
**	    deciding to take share or update FHDR lock.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	20-April-1999 (thaal01) b91567
**	    Verifydb reports W_DM500C orphan data pages, when in reality
**	    these are the second and subsequent FMAP. dm1p_fminit() was calling the
**	    macro DM1P_SET_FMAP_PAGE_STAT_MACRO instead of DM1P_INIT_FMAP_PAGE_STAT_MACRO.
**	    During an extend of a table all pages are initialized as DATA pages.
**	    The previous macro did not reset this bit, and verifydb incorrectly
**	    interpreted it as a DATA page . 
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**      21-dec-1999 (stial01)
**          scan_map() Etabs: Make sure page deallocated is committed
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_alloc, dm0l_dealloc.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Fix references to sizeof V2 page elements.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Pass the fmap highwater mark to dm0l_fmap() in extend().
**      01-feb-2001 (stial01)
**         scan_map() Etabs: call dmxe_xn_active using page tranid
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      21-may-2001 (stial01)
**          Changes to fix FHDR fhdr_map initialization (B104754)
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      15-sep-2003 (stial01)
**          Don't use row locking or physical page locking protocols for 
**          etabs if table locking (b110923)
**      16-sep-2003 (stial01)
**          extend() Don't do dm2f_galloc_file for new heap etabs (SIR 110923)
**      18-sep-2003 (stial01)
**          extend() DON'T set rcb_galloc* for temp etabs
**      02-jan-2004 (stial01)
**          Removed alloc code for HEAP etabs (temporary trace point dm722)
**          Instead, for etabs keep doubling the relextend, to do fewer gallocs
**      12-apr-2004 (stial01)
**          scan_map - if blob etab and PHYS page locking,
**          don't reallocate pages deallocated by current transaction.
**      07-jan-2005 (stial01)
**          Unmutex new page before LKrequest (b113708)
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      14-mar-05 (stial01)
**          Fix up more DM0P_CREATE_NOFMT error handling (b114112)
**      05-oct-05 (stial01)
**          scan_map() set DMPP_MODIFY for new page during backup (b115344)
**	24-oct-2005 (devjo01 for jenjo02) 
**	    Make lock mode selection for the header page use the same
**	    conditions as are used in dm1p_getfree (change 439873).
**	19-Jun-2007 (stial01)
**         dm1p_log_error() Supress dm1p errors if E_DM0065_USER_INTR
**	20-may-2008 (joea)
**	    Add rel_owner param to dm0p_lock_page.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2f_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *, use
**	    new form uleFormat
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	15-Jan-2009 (jonj)
**	    Invalidate lock_id's before first use to avoid wasted
**	    implicit lock conversion search in LKrequest.
**	25-Aug-2009 (kschendel) 121804
**	    Need dmxe.h to satisfy gcc 4.3.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Delete a pile of unused code.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    New macros to lock/unlock pages, LG_LRI replaces LG_LSN,
**	    sensitized to crow_locking().
**/

/*
**  Forward and/or External function references.
*/

static DB_STATUS    extend( 
			DMP_RCB	    *rcb,
			DM1P_FHDR   *fhdr,
			DM1P_LOCK   *fhdr_lock,
			i4	    extend,
			i4	    flag,
			DB_ERROR    *dberr );

static DB_STATUS    find_free( 
			DMP_RCB	    *rcb,
			DM1P_FHDR   **free_header,
			DM1P_LOCK   *fhdr_lock,
			i4	    flag,
			DMP_PINFO   *pinfo,
			DB_ERROR    *dberr );

static DB_STATUS    fix_header( 
			DMP_RCB	    *rcb,
			DM1P_FHDR   **header,
			DM1P_LOCK   *fhdr_lock,
			DB_ERROR    *dberr );

static DB_STATUS    fix_map(
			DMP_RCB	    *rcb,
			i4	    pageno,
			DM1P_FMAP   **fmap,
			DM1P_LOCK   *fmap_lock,
			DM1P_FHDR   **fhdr,
			DM1P_LOCK   *fhdr_lock,
			DB_ERROR    *dberr );

static VOID	    dm1pLogErrorFcn(
			i4	    terr_code,
			DB_ERROR    *dberr,
			DB_TAB_NAME *table_name,
			DB_DB_NAME  *dbname, 
			PTR	    FileName,
			i4	    LineNumber);
#define	dm1p_log_error(terr_code,dberr,table_name,dbname) \
	dm1pLogErrorFcn(terr_code,dberr,table_name,dbname,__FILE__,__LINE__)

static i4	    print(
			PTR	    arg,
			i4	    count,
			char        *buffer );

static DB_STATUS    scan_map(
			DMP_RCB     *rcb,
			DM1P_FHDR   **free_header,
			DM1P_LOCK   *fhdr_lock,
			i4	    *map_index,
			i4     flag,
			DMP_PINFO   *pinfo,
			DB_ERROR    *dberr );

static DB_STATUS    test_free(
			DMP_RCB	    *rcb,
			DM1P_FHDR   *free_header,
			DM1P_LOCK   *fhdr_lock,
			DM1P_FMAP   **fmap_context,
			DM1P_LOCK   *fmap_lock,
			DM_PAGENO   pageno,
			DB_ERROR    *dberr );

static DB_STATUS    unfix_header(
			DMP_RCB     *rcb,
			DM1P_FHDR   **fhdr,
			DM1P_LOCK   *fhdr_lock,
			DM1P_FMAP   **fmap,
			DM1P_LOCK   *fmap_lock,
			DB_ERROR    *dberr );

static DB_STATUS    force_fpage(
			DMP_RCB     *rcb,
			DMP_PINFO   *pinfo,
			DB_ERROR    *dberr );

static DB_STATUS    unfix_map(
			DMP_RCB     *rcb,
			DM1P_FMAP   **fmap,
			DM1P_LOCK   *fmap_lock,
			DM1P_FHDR   **fhdr,
			DM1P_LOCK   *fhdr_lock,
			DB_ERROR    *dberr );

static DB_STATUS    display_bitmaps(
    			DMP_RCB	    *rcb,
    			DM_PAGENO    highwater_mark,
    			i4	    total_pages,
    			DM1P_FHDR   *fhdr,
			DM1P_LOCK   *fhdr_lock,
			DB_ERROR    *dberr );

static DB_STATUS    display_pagetypes(
    			DMP_RCB	    *rcb,
    			DM_PAGENO   highwater_mark,
    			i4	    total_pages,
			DB_ERROR    *dberr );

static DB_STATUS display_hash(
			DMP_RCB	    *rcb,
			DM_PAGENO   highwater_mark,
			DB_ERROR    *dberr );

static VOID 	    map_pagetype(
			DMPP_PAGE	*page,
			DMP_RCB		*rcb,
			char		*page_type);

static VOID 	    remap_pagetype(
			DMPP_PAGE	*page,
			DMP_RCB		*rcb,
			char		*page_type);

static DB_STATUS    put_page_build_SMS(
			DMPP_PAGE	  *page,
			DM1P_BUILD_SMS_CB *build_cb,
			DB_ERROR    *dberr );

static DB_STATUS    get_page_build_SMS(
			DMPP_PAGE	  *page,
			DM_PAGENO	  pageno,
			DM1P_BUILD_SMS_CB *build_cb,
			DB_ERROR    *dberr );

static DB_STATUS   lock_page(
    			DMP_RCB		  *r,
    			DM1P_LOCK	  *lock_id,
			DM_PAGENO	  pageno,
			DB_ERROR    *dberr );

static DB_STATUS   unlock_page(
    			DMP_RCB		  *r,
    			DM1P_LOCK	  *lock_id,
			DB_ERROR    *dberr );

static DB_STATUS relock_pages(
    			DMP_RCB		*r,
    			DM1P_LOCK	*lock1,
    			DM1P_LOCK	*lock2,
			DB_ERROR    *dberr );

static DB_STATUS unfix_nofmt_page(
			DMP_RCB		*rcb,
			DMP_PINFO	*pinfo,
			i4		pageno,
			DB_ERROR    *dberr );

/*  Special constants for fix_map/fix_header functions. */

#define	DM1P_FOR_SCRATCH	0x8000 /* must not conflict with LK_x lock modes */

#define FHDR_READ_LKMODE	LK_IX  /* How we read FHDRs */
#define FHDR_UPDATE_LKMODE	LK_X   /* How we update FHDRs */
#define FMAP_READ_LKMODE	LK_IS  /* How we read FMAPs */
#define FMAP_UPDATE_LKMODE	LK_SIX /* How we update FMAPs */

/*  Special constants for dm1p_dump(). */

#define	DM1P_P_FHDR	'H'
#define	DM1P_P_FMAP	'M'
#define	DM1P_P_FREE	'F'
#define	DM1P_P_ROOT	'r'
#define	DM1P_P_INDEX	'i'
#define	DM1P_P_SPRIG	's'
#define	DM1P_P_LEAF	'L'
#define	DM1P_P_OVFLEAF	'O'
#define DM1P_P_OVFDATA  'o'
#define	DM1P_P_ASSOC	'A'
#define DM1P_P_DATA	'D'
#define DM1P_P_EDATA	'd'
#define DM1P_P_EOVFDATA 'e'
#define DM1P_P_UNUSED	'U'
#define DM1P_P_UNKNOWN	'?'

/*{
** Name: dm1p_getfree - Allocates a page from the free list.
**
** Description:
**	This routine will allocate a free page to the caller.  If the
**	free list is empty it is automatically extended.  The conversion
**	from old free list format to new free list format happens
**	automatically.
**
**	The allocated page is returned as a buffer fixed for writing.
**	It is the responsibility of the caller to unfix the page.
**
**	Additionally, the caller is responsible for calling the routine
**	dm0p_pagetype to allow the buffer manager to do any initialization
**	required for the created page type.
**
**	This routine logs the allocation in such a way that it can be
**	undone by the transaction abort routines.  If the allocation needs
**	to be persistent, the caller must encapsulate the call of this
**	routine within a logging system mini-transaction.
**
**	To encourage thread concurrency when viable, the FHDR is locked LK_IX
**	and converted to LK_X if it's determined that the FHDR needs to be
**	updated. FMAPs are locked as well, LK_IS to read, 
**	LK_SIX to update; no lock is needed if the corresponding FHDR is locked
**	LK_X.
**
** Inputs:
**      rcb                             Pointer to record access context.
**	flag				Either 0 or
**					DM1P_PHYSICAL: a physical page lock
**					  is set on the newly allocated page.
**					DM1P_MINI_TRAN: Inside a mini-tran
**					  check have not used page before.
** Outputs:
**      newpage				Pointer to the returned fixed page.
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      E_DM0042_DEADLOCK,
**                                      E_DM0012_BAD_FILE_ALLOC.
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
**	Allocation bitmaps are updated, the FHDR may be updated,
**	the file may be extended.
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	24-jun-1991 (Derek)
**	    Add suport for new DM1P_NEEDBI flag.
**	20-Nov-1991 (rmuth)
**	    Add comment for DM1P_MINI_TRAN.
**	31-March-1992 (rmuth)
**	    Add function prototype, change err_code to i4 from
**	    DB_STATUS.
**	29-August-1992
**	    Add rcb parameter to dm1p_log_error
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	    - Set tbio_alloc,tbio_lpageno and tbio_checkeof;
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**	17-Jul-96 (pchang)
**	    When checking if our tbio_lpageno is stale, we need to cast 
**	    newpage->page_page to a i4 because tbio_lpageno could contain 
**	    the value -1 which would otherwise be converted incorrectly into 
**	    unsigned MAX_I4 (b76526). 
**	23-jul-96 (nick)
**	    Always lock the free page with a physical lock and then
**	    convert to logical after releaseing the FHDR if necessary.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Changed arguments for dm0p_trans_page_lock() call.
**      06-jan-97 (dilma04)
**          If converting a physical page lock to logical when row locking,
**          request it in IX mode.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          add lock_id parameter to dm0p_trans_page_lock() call.
**	01-Jul-1998 (jenjo02)
**	    Lock FHDR for shared access when viable.
**	15-jan-1999 (nanpr01 & jenjo02)
**	    When using cache locks(DMCM or multi-server non-fastcommit 
**	    protocol), get stronger mode covering locks to avoid cache
**	    deadlock.
**	02-Apr-2010 (jonj)
**	    SIR 121619 MVCC: extend() may return E_DM004C_LOCK_RESOURCE_BUSY
**	    if new FMAP collides with another page being allocated.
*/
DB_STATUS
dm1p_getfree(
    DMP_RCB	*rcb,
    i4	flag,
    DMP_PINFO   *pinfo,
    DB_ERROR	*dberr )
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DM1P_FHDR	    *fhdr = 0;
    DB_STATUS	    status;
    DB_STATUS	    local_status;
    i4	    local_flag;
    DM_PAGENO	    newpageno;
    DM1P_LOCK	    fhdr_lock;
    i4	    fhdr_extend_count;
    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    pinfo->page = (DMPP_PAGE*)NULL;

    /*
    ** If we're the only user of this database, or this is a temporary table,
    ** lock the FHDR for update, otherwise lock the FHDR for read
    ** to encourage concurrency; if we decide the FHDR needs updating,
    ** we'll relock it for update when needed.
    **
    ** For HEAPs, dm1p_getfree() is always called while the file's "lastused"
    ** page is fixed for update, so concurrent allocations are not possible 
    ** and we'll lock the FHDR/FMAPs for update.
    */
    if (r->rcb_lk_type == RCB_K_TABLE ||
	t->tcb_rel.relspec == TCB_HEAP ||
	d->dcb_status & DCB_S_EXCLUSIVE ||
	t->tcb_temporary == TCB_TEMPORARY ||
	((t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_MULTIPLE ||
	  t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_DMCM) &&
	  (t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_READONLY_DB) == 0)
	)
    {
	fhdr_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
    }
    else
    {
	fhdr_lock.fix_lock_mode = FHDR_READ_LKMODE;
    }

    /* Lock the FHDR and fix it for UPDATE */
    status = fix_header(r, &fhdr, &fhdr_lock, dberr);


    /*	Space allocation loop. */

    while (status == E_DB_OK)
    {
	/* Extract FHDR extend count */
	fhdr_extend_count = DM1P_VPT_GET_FHDR_EXTEND_CNT_MACRO(pgtype, fhdr);

	/*  Attempt an allocation. */

	status = find_free(r, &fhdr, &fhdr_lock, flag,
				pinfo, dberr);

	/*
	** See if our tbio_lpageno is stale, this can occur if 
	** another server is extending the table or we have
	** just extended the table and are using a page
	** from the new extension.
	*/
	if ( status == E_DB_OK )
	{	
	    newpageno = DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, pinfo->page);

            if ( t->tcb_table_io.tbio_lpageno < newpageno)
	    	t->tcb_table_io.tbio_lpageno = newpageno;

	    break;
	}
	else if ( status == E_DB_WARN )
	{
	    /*  
	    ** No space left, extend the file.
	    **
	    ** If we haven't locked the FHDR for update yet,
	    ** do so now.
	    **
	    ** If we have to wait for the FHDR update lock, another thread
	    ** may have extended the file in the interim. Check the
	    ** before and after extend counts; if they differ, start
	    ** over with find_free() instead of extending the file.
	    ** This prevents multiple threads from multiply extending
	    ** the file.
	    */
	    if (fhdr_lock.lock_value.lk_mode != FHDR_UPDATE_LKMODE)
	    {
		fhdr_lock.new_lock_mode = FHDR_UPDATE_LKMODE;
		status = relock_pages(r, &fhdr_lock, (DM1P_LOCK *)0, dberr);
	    }
	}
	
	if (status == E_DB_OK || status == E_DB_WARN)
	{
	    status = E_DB_OK;

	    /* If the file was extended by another thread, restart */
	    if (fhdr_extend_count != DM1P_VPT_GET_FHDR_EXTEND_CNT_MACRO(pgtype, fhdr))
		continue;

	    status = extend(r, fhdr, &fhdr_lock,
			    r->rcb_opt_extend, DM1P_ADD_NEW, dberr);
	    if (status == E_DB_OK && t->tcb_extended)
	    {
		i4 max_extend;

		max_extend = DM1P_VPT_MAXPAGES_MACRO(pgtype,pgsize)/100;
		if ((r->rcb_opt_extend * 2) < max_extend)
		    r->rcb_opt_extend *= 2;
	    }

	    if (status != E_DB_OK && 
		dberr->err_code != E_DM004C_LOCK_RESOURCE_BUSY &&
	        r->rcb_opt_extend > t->tcb_rel.relextend)
	    {
		r->rcb_opt_extend = t->tcb_rel.relextend;
		status = extend(r, fhdr, &fhdr_lock, 
			    r->rcb_opt_extend, DM1P_ADD_NEW, dberr);
	    }

	    if ( status == E_DB_OK || dberr->err_code == E_DM004C_LOCK_RESOURCE_BUSY )
	    {
		if ( status == E_DB_OK )
		{
		    /*
		    ** As extended table set tcb_lalloc, also as we know that no
		    ** other user can be changing the FHDR/FMAP(s) and we will
		    ** be setting tbio_lpageno above so set tcb_checkeof to 
		    ** FALSE.
		    */
		    t->tcb_table_io.tbio_lalloc     = 
			DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr) - 1;
		    t->tcb_table_io.tbio_checkeof   = FALSE;
		}

		status = E_DB_OK;

		if (fhdr_lock.fix_lock_mode == FHDR_READ_LKMODE)
		{
		    /* Reduce FHDR update lock to permit read concurrency */
		    fhdr_lock.new_lock_mode = FHDR_READ_LKMODE;
		    status = relock_pages(r, &fhdr_lock, (DM1P_LOCK *)0, dberr);
		}
	    }
	}
	/* loop back to retry find_free() */
    }

    /*	Unfix the FHDR page. */

    if (fhdr)
    {
	local_status  = unfix_header(r, &fhdr, &fhdr_lock, 
				     (DM1P_FMAP**)0, (DM1P_LOCK*)0,
				     &local_dberr);
	if (local_status > status)
	{
	    status = local_status;
	    *dberr = local_dberr;
	}
    }

    if (status != E_DB_OK)
    {
    	dm1p_log_error(E_DM92D0_DM1P_GETFREE, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    }

    return (status);
}

/*{
** Name: dm1p_putfree - Returns a page to the free list.
**
** Description:
**	This routine will return a page to the free list.
**	The page to be freed is marked as free and unfixed.
**	If the free list hasn't been converted then the page is the marking
**	of the 	FHDR and FMAP is skipped, the page is still marked as free and
**	unfixed.
**
**	This routine logs the deallocation in such a way that it can be
**	undone by the transaction abort routines.  If the deallocation needs
**	to be persistent, the caller must encapsulate the call of this
**	routine within a logging system mini-transaction.
**
** 	The FHDR is initially locked S with the assumption that the hint
**	for the FMAP won't need to be updated; if that proves false,
**	the FHDR will be relocked X.
**
** Inputs:
**      rcb                             Pointer to record access context.
**	freepage			Pointer to free data page pointer.
** Outputs:
**	freepage			Set to NULL by unfixing.
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      E_DM0042_DEADLOCK,
**                                      E_DM0012_BAD_FILE_ALLOC.
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
**	Allocation bitmaps are updated.
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	Halloween-91 (rogerk)
**	    Initialized fhdr local variable as its value is tested to
**	    determine if the fix_header call was successful.  Also, reset
**	    'status' value before dm0l_dealloc call since it could have
**	    been E_DB_INFO from above.
**	31-March-1992 (rmuth)
**	    Added function prototype. 
**	18-May-1992 (rmuth)
**	    Removed the on-the-fly conversion code, means fix_header() can
**	    no longer return E_DB_INFO so remove this.
**	29-August-1992
**	    - Removed on-the-fly conversion.
**	    - Add rcb parameter to dm1p_log_error
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Updated with new recovery protocols:
**	      - Each page updated as part of the allocate operation (fhdr,fmap
**		and free page) must be mutexed before the allocate log record
**		is written, and held until the page update is complete.
**	      - The updates on the fmap and fhdr pages are now performed
**		directly by this routine so that they can be accurately logged.
**		They are no longer updated by calling mark_free().
**	      - The buffer manager no longer mutexes fhdr and fmap pages for
**		us, we must request the page mutexes directly.
**	      - Removed checksums on fhdr and fmap pages - checksums are now
**		written by the buffer manager on all ingres pages.
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
**	    Add flag param to LGreserve() call.
**	 3-jan-1994 (rogerk)
**	    Update page_tran_id on the page being freed so that the allocate
**	    code can correlate a potential free page to with the transaction
**	    that freed it.  It is sometimes not allowed to allocate a page that
**	    was deallocated by your same transaction.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	9-mar-1998 (thaju02)
**	    fmap page is fixed with nolock; fhdr physical page lock protects
**	    fmap. dmf protocol error; unfix fmap before fhdr.
**      21-apr-98 (stial01)
**          dm1p_putfree() Set DM0L_PHYS_LOCK if extension table (B90677)
**	24-oct-2005 (devjo01 for jenjo02)
**	    Make lock mode selection for the header page use the same
**	    conditions as are used in dm1p_getfree.
**	28-Dec-2007 (kibro01) b119663
**	    Remove the LG_RS_FORCE flag, since it isn't needed in all
**	    circumstances.  Apply the same conditions that take place when
**	    rollback or recovery is performed.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
*/
DB_STATUS
dm1p_putfree(
    DMP_RCB	*rcb,
    DMP_PINFO	*freePinfo,
    DB_ERROR	*dberr )
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM1P_FHDR	    *fhdr = NULL;
    DM1P_FMAP	    *fmap = NULL;
    DMPP_PAGE	    *page = freePinfo->page;
    LG_LSN	    lsn;
    DB_STATUS	    status;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    i4	    fmap_index;
    i4	    fmap_bit;
    i4	    dm0l_flag;
    i4	    fhdr_loc_config_id;
    i4	    fmap_loc_config_id;
    i4	    free_loc_config_id;
    i4	    loc_id;
    i4	    local_error;
    i4	    	    fhdr_update;
    DM_PAGENO	    pageno = DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, page);
    DM_PAGENO	    fhdr_pageno = t->tcb_rel.relfhdr;
    DM_PAGENO	    fmap_pageno;
    i4	    fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    u_char	    *fhdr_map;
    i4	    fmap_sequence;
    DB_STATUS	    page_status;
    DB_STATUS	    fhdr_status;
    DB_STATUS	    fmap_status;
    DM1P_LOCK	    fhdr_lock, fmap_lock;
    i4      lg_force_flag;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(50))
	TRdisplay("dm1p_putfree: table %~t page %d\n",
	    sizeof(t->tcb_rel.relid), &t->tcb_rel.relid, pageno);

    /*
    ** If we're the only user of this database, or this is a temporary table,
    ** lock the FHDR for update, otherwise lock the FHDR for read
    ** to encourage concurrency; if we decide the FHDR needs updating,
    ** we'll relock it for update when needed.
    **
    ** b114660 - Extend criteria for increasing lock strength to match that
    ** in dm1p_getfree.
    */
    if (r->rcb_lk_type == RCB_K_TABLE ||
	t->tcb_rel.relspec == TCB_HEAP ||
	d->dcb_status & DCB_S_EXCLUSIVE ||
	t->tcb_temporary == TCB_TEMPORARY ||
	((t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_MULTIPLE ||
	  t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_DMCM) &&
	  (t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_READONLY_DB) == 0)
       )
    {
	fhdr_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
    }
    else 
    {
	fhdr_lock.fix_lock_mode = FHDR_READ_LKMODE;
    }
    
    fmap_index = pageno / fseg;
    fmap_bit = pageno % fseg;

    /*
    ** Fix the table FHDR and the appropriate FMAP for the page being freed.
    */
    status = fix_header(r, &fhdr, &fhdr_lock, dberr);

    while (status == E_DB_OK)
    {
	fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, fmap_index);
	fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map);

	/*
	** Determine if any FHDR information will be updated so we can
	** log the changes.
	*/
	if (VPS_TEST_FREE_HINT_MACRO(pgtype, fhdr_map) == 0)
	{
	    /*
	    ** If we didn't lock the FHDR for update, do so now.
	    */
	    if (fhdr_lock.lock_value.lk_mode != FHDR_UPDATE_LKMODE)
	    {
		fhdr_lock.new_lock_mode = FHDR_UPDATE_LKMODE;
		status = relock_pages(r, &fhdr_lock, (DM1P_LOCK *)0, dberr);

		/* If we had to wait for FHDR update lock, retest the condition */
		if (status == E_DB_WARN)
		{
		    status = E_DB_OK;
		    continue;
		}
	    }
	    fhdr_update = TRUE;
	}
	else
	{
	    fhdr_update = FALSE;
	    fhdr_status = E_DB_OK;
	    /* Unfix the FHDR */
	    status = unfix_header(r, &fhdr, &fhdr_lock,
					&fmap, &fmap_lock, dberr);
	}

	if (status != E_DB_OK)
	    break;

	/* Fix the appropriate FMAP for update */
	fmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
	status = fix_map(r, fmap_pageno, &fmap, &fmap_lock,
				&fhdr, &fhdr_lock, dberr);

	if (status != E_DB_OK)
	    break;

	fmap_sequence = DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap);

	/*
	** Log the deallocate action and write the deallocate log record's
	** LSN to each of the updated pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    /*
	    ** We only need BIs if the database is undergoing backup and
	    ** logging is enabled.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
		/*
		** Online Backup Protocols: Check if BIs must be logged before update.
		*/
		if (fhdr_update)
		    status = dm0p_before_image_check(r, (DMPP_PAGE *) fhdr, dberr);
		if (status == E_DB_OK)
		    status = dm0p_before_image_check(r, (DMPP_PAGE *) fmap, dberr);
		if (status == E_DB_OK)
		    status = dm0p_before_image_check(r, page, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Find physical location ID's for log record below.
	    */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fhdr_pageno);
	    fhdr_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fmap_pageno);
	    fmap_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, pageno);
	    free_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

	    /* Same logic as that which might actually force the page */
            if (dm0p_set_lg_force(tbio,rcb))
	    {
		lg_force_flag = LG_RS_FORCE;
	    } else
	    {
		dm0l_flag |= DM0L_FASTCOMMIT;
		lg_force_flag = 0;
	    }

	    cl_status = LGreserve(lg_force_flag, rcb->rcb_log_id, 2, 
		sizeof(DM0L_DEALLOC) * 2, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    err_code, 1, 0, rcb->rcb_log_id);
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM92D1_DM1P_PUTFREE);
		break;
	    }

	    /* 
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    status = dm0l_dealloc(rcb->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid,
		&t->tcb_rel.relid, t->tcb_relid_len,
		&t->tcb_rel.relowner, t->tcb_relowner_len,
		t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_rel.relloccount, 
		fhdr_pageno,
		fmap_pageno,
		pageno,
		fmap_sequence,
		fhdr_loc_config_id, fmap_loc_config_id, free_loc_config_id, 
		fhdr_update, (LG_LSN *)NULL, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}


	/*
	** Update appropriate FHDR hints if any need updating.
	** If this FMAP is listed in the fhdr as having no free pages, then
	** mark it to now contain at least one.
	*/
	if (fhdr_update)
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fhdr_lock.pinfo);
	    DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, DMPP_MODIFY);
	    VPS_SET_FREE_HINT_MACRO(pgtype, fhdr_map);
	    if (r->rcb_logging & RCB_LOGGING)
		DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(pgtype, fhdr, lsn);
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fhdr_lock.pinfo);

	    /* Unfix the FHDR */
	    fhdr_status = unfix_header(r, &fhdr, &fhdr_lock, 
					&fmap, &fmap_lock, dberr);
	}

	/*
	** Update FMAP to indicate that the page is now free.
	** Also update the nextbit field.
	**    The firstbit field is set to the freed page if there are no
	**    free bits on the page. (I.E. nextbit == FSEG)
	*/
	dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);

	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);
	BITMAP_SET_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap), fmap_bit);
	if (fmap_bit < (i4)DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap))
	    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, fmap_bit);
	if (fmap_bit > (i4)DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap))
	    DM1P_VPT_SET_FMAP_LASTBIT_MACRO(pgtype, fmap, fmap_bit);
	if (r->rcb_logging & RCB_LOGGING)
	    DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, fmap, lsn);

	/* Unmutex and unfix the FMAP and FHDR, if still fixed */
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);
	fmap_status = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);

	/*
	** Mutex and mark deallocated page as FREE.
	*/
	dm0pMutex(tbio, (i4)0, r->rcb_lk_id, freePinfo);
	
	DMPP_VPT_SET_PAGE_STAT_MACRO(pgtype, page, (DMPP_FREE | DMPP_MODIFY));

	/*
	** Update the page's tran_id field with the id of the transaction
	** doing the deallocate operation.  This tran_id may later be needed
	** in scan_map if the transaction requests to allocate a new page.
	*/
	DMPP_VPT_SET_PAGE_TRAN_ID_MACRO(pgtype, page, r->rcb_tran_id);
	DMPP_VPT_SET_PAGE_LG_ID_MACRO(pgtype, page, r->rcb_slog_id_id);
	if (r->rcb_logging & RCB_LOGGING)
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(pgtype, page, lsn);

	/* Unmutex and unfix the deallocated page */
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, freePinfo);
	page_status = dm0p_unfix_page(r, DM0P_UNFIX, freePinfo, dberr);

	if ((status = page_status) ||
	    (status = fmap_status) ||
	    (status = fhdr_status))
	{
	    break;
	}

	r->rcb_page_adds--;
	return (E_DB_OK);
    }

    /*
    ** Error handling, unfix the fhdr, fmap pages if left fixed.
    ** No pages have been left in a mutexed state.
    */
    if (fmap)
    {
	status = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (fhdr)
    {
	status = unfix_header(r, &fhdr, &fhdr_lock, 
				&fmap, &fmap_lock, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    dm1p_log_error(E_DM92D1_DM1P_PUTFREE, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    return (E_DB_ERROR);
}

/*{
** Name: dm1p_checkhwm - Updates the TCB with the current high water mark.
**
** Description:
**	This routine updates the current high water mark for a table.  The
**	buffer manage will not attempt to perform a group-read if the group
**	is beyond the area of the file that is known to have been written
**	at least once.  This routine is called at table open time the first
**	time the table is opened, or later if a previous group-read was
**	was squashed because it was operation beyond the highwater mark.
**
**	If table has not been converted, then don't do anything.  The code
**	that calls this will make a guestimate.
**
** Inputs:
**      rcb                             Pointer to record access context.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      E_DM0042_DEADLOCK,
**                                      
**	Returns:
**
**	    E_DB_OK
**	    E_DB_WARN		fhdr_hw_map is corrupt or FMAP checksum error.
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    tbio_lpageno is updated, and tbio_checkeof is reset.
** History:
**      02-feb-90 (Derek)
**          Created.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**	10-Feb-1992 (rmuth)
**	    Make sure we set err_code if we have a problem unfixing the 
**	    FHDR and no other unexpected error has already occurred.
**	29-August-1992 (rmuth)
**	    - Removed the on-the-fly conversion code
**	    - Add rcb parameter to dm1p_log_error
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	    - Set tbio_lalloc as we know no other user can be
**	      updating the table.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Change DM1P_FSEG references to DM1P_FSEG.
**	01-Jul-1998 (jenjo02)
**	    Lock FHDR and FMAP LK_S. Unfix/unlock FHDR before fixing
**	    FMAP.
**       7-June-2004 (hanal04) Bug 112264 INGSRV2813
**          The last ingres allocated page may reside in a different
**          FMAP to the fhdr_hwmap which actually indicates the first FMAP
**          with free pages. If the current FMAP was ever full scan
**          the FMAPs to ensure tbio_lpageno is set correctly.
**	28-Feb-2007 (kibro01) b117461
**	    hw-mark of fseg means the FMAP has not yet been used, so might
**	    not contain the last allocated page
*/
DB_STATUS
dm1p_checkhwm(
    DMP_RCB	*r,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DM1P_FHDR	    *fhdr = NULL;
    DM1P_FMAP	    *fmap = NULL;
    DM_PAGENO	    fmap_pageno;
    DB_STATUS	    status;
    DB_STATUS	    s;

    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    i4	    fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    i4	    fhdr_hwmap;
    i4	    fhdr_pages;
    i4      fhdr_count;
    i4      fmap_hw_mark;
    i4      fmap_inc = 0;
    i4      last_fmap = 0;
    i4      fmap_lastbit;

    DM1P_LOCK	    fhdr_lock, fmap_lock;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);
    
    /*
    ** If we're the only user of this database, or this is a temporary table,
    ** lock the FHDR for update, otherwise lock the FHDR for read
    ** to encourage concurrency.
    */
    if (r->rcb_lk_type == RCB_K_TABLE ||
	t->tcb_rel.relspec == TCB_HEAP ||
	d->dcb_status & DCB_S_EXCLUSIVE ||
	t->tcb_temporary == TCB_TEMPORARY)
    {
	fhdr_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
    }
    else 
    {
	fhdr_lock.fix_lock_mode = FHDR_READ_LKMODE;
    }

    /* Invalidate lock_id's */
    fhdr_lock.lock_id.lk_unique = 0;
    fmap_lock.lock_id.lk_unique = 0;

    /* fhdr_hwmap may not be the last FMAP with ingres allocated pages.
    ** loop over additional FMAPs if the current FMAP was full at some
    ** time in the past (fmap_lastbit == fmap_hw_mark)
    */
    for(;;)
    {
        /*	
        ** Lock and fix the FHDR page. 
        */
        status = fix_header(r, &fhdr, &fhdr_lock, dberr);

        if (status == E_DB_OK)
        {
	    /*  Check that the current FMAP block is correct. */
	    fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(pgtype, fhdr);
	    fhdr_pages = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);
	    fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype,fhdr);

            fhdr_hwmap += fmap_inc;
	    if((fhdr_hwmap > fhdr_count) || 
	       (last_fmap && (last_fmap != fhdr_hwmap - 1)))
	    {
	        /* The SMS has changed while we were in this loop, let's start
	        ** from the begining.
	        */
	        fmap_inc = 0;
	        last_fmap = 0;
                status = unfix_header(r, &fhdr, &fhdr_lock,
		        &fmap, &fmap_lock, dberr);
                if (status == OK)
	        {
                    continue;
	        }
	        else
	        {
		    break;
	        }
	    }
	    last_fmap = fhdr_hwmap;

	    if (fhdr_hwmap == 0 || 
	        DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype,fhdr) == 0 ||
	        (fmap_pageno =
		    VPS_NUMBER_FROM_MAP_MACRO(pgtype,
		    DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr,
		    (fhdr_hwmap - 1)))) == 0)
	    {
	        status = E_DB_WARN;
	    }
	    else
	    {
	        /* Unfix the FHDR before fixing the FMAP */
	        status = unfix_header(r, &fhdr, &fhdr_lock,
				    &fmap, &fmap_lock, dberr);
	        if (status == OK)
	        {
		    /* Fix the FMAP in the same mode we decided to fix 
		    ** the FHDR 
		    */
		    if (fhdr_lock.fix_lock_mode == FHDR_UPDATE_LKMODE)
		        fmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
		    else
		        fmap_lock.fix_lock_mode = FMAP_READ_LKMODE;

		    status = fix_map(r, fmap_pageno, &fmap, &fmap_lock,
				        &fhdr, &fhdr_lock, dberr);
		    if (status == E_DB_OK)
		    {
		        fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, 
					fmap);
		        fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, 
					fmap);

		        /* Check for a pre-allocation. fmap_hw_mark
			** of -1 indicates this FMAP has never had any
			** ingres allocated pages.
			*/
		        if((fmap_hw_mark != -1) && (fmap_hw_mark != fseg))
		        {
		            t->tcb_table_io.tbio_lpageno = 
			        (fhdr_hwmap - 1) * 
			        fseg + fmap_hw_mark;

		            t->tcb_table_io.tbio_lalloc = fhdr_pages - 1;

		            t->tcb_table_io.tbio_checkeof = FALSE;

		            if (DMZ_AM_MACRO(50))
			        TRdisplay("dm1p_checkhwm: table %~t, high water mark page %d\n",
			            sizeof(t->tcb_rel.relid),
			            &t->tcb_rel.relid,
			            t->tcb_table_io.tbio_lpageno);

		        }

		        /*  Unfix the FMAP and FHDR, if still fixed. */
		        status = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
		        if (status == E_DB_OK)
		        {
			    if((fhdr_hwmap < fhdr_count) &&
			       (fmap_lastbit == fmap_hw_mark) &&
			       (fmap_hw_mark != -1))

                            {
			        /* This FMAP's hwm indicates it was full at some
			        ** point. If we have more FMAPs check to see
			        ** if there are more allocated pages.
			        */
			        fmap_inc++;
                                continue;
			    }
		        }
		    }
	        }
	    }
        }
        break;
    }


    /*	Unfix the FHDR page if it hasn't been already. */

    if (fhdr)
    {
	s = unfix_header(r, &fhdr, &fhdr_lock, 
				&fmap, &fmap_lock, &local_dberr);
	if (s != E_DB_OK && (status == E_DB_OK || status == E_DB_WARN))
	{
	    *dberr = local_dberr;
	    status = s;
	}
    }

    if (DB_FAILURE_MACRO(status))
    {
	dm1p_log_error(E_DM92D4_DM1P_CHECKHWM, dberr,
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);
    }
    return (status);
}

/*{
** Name: dm1p_lastused - Find the last used page.
**
** Description:
**	This routine is used to find either the last used data page in
**	the table or the last used page. 
**
**	THERE IS A STRONG ASSUPTION IN THIS ROUTINE THAT THE ACCESS METHODS
**	WHICH CALL IT DO NOT EVER FREE PAGES ONCE ALLOCATED.  The routine
**	calculates the last used page by finding the first FREE page and
**	then subtracting one.
**
**	It should only be called on HEAP tables since that is the only data
**	structure which can guarantee that there can be no used pages after
**	the first free page (Hash and Isam tables can violate this condition
**	through rollback of allocate operations).
**
**	This function is used to support the heap access method which adds
**	new records at end of file and also bulk copy to EOF.
**
**	FIXME:  dm1p and dm1s are not playing all that well together.
**	If you extend a heap and then load it (non-recreate), the extent
**	is ignored, and the new rows go at the end (!!!).  A similar
**	problem occurs if you do a large non-recreate heap LOAD and roll
**	it back;  the used pages are never re-used.  This probably isn't
**	dm1p's problem per se, but I started noticing it after the fix for
**	120547, which probably uncovered something else.  (kschendel Nov 2009)
**
** Inputs:
**      rcb                             Pointer to record access context.
**	flag				DM1P_LASTUSED_DATA - Return the 
**					last used data page.
** Outputs:
**	page_number			Last page number.
**	lastused_page			If present, lastused page is fixed.
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      E_DM0042_DEADLOCK,
**                                      E_DM0012_BAD_FILE_ALLOC.
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
**	    None.
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	31-March-1992 (rmuth)
**	    Added function prototype.
**	29-August-1992 (rmuth)
**	    - Add rcb paramter to dm1p_log_error.
**	    - Add DM1P_LASTUSED_DATA 
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	14-dec-1992 (rogerk)
**	    Add check to make sure that we do not skip the last fmap page
**	    if there are no free pages on it.
**	24-jan-1994 (gmanning)
**	    Change unsigned i4 to u_i4 to compile in NT environment.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Change DM1P_MBIT reference to DM1P_MBIT_MACRO.
**		Change DM1P_FSEG reference to DM1P_FSEG_MACRO.
**	01-Jul-1998 (jenjo02)
**	    Added (DMPP_PAGE **) output parm to dm1p_lastused(). If passed by
**	    caller, the "lastused" page will be fixed while the FMAP is fixed
**	    and locked, closing a large window in dm1s whereby the "lastused"
**	    page could be deallocated by recovery before being fixed by the
** 	    caller of dm1p_lastused().
**	09-Sep-1998 (jenjo02)
**	    If more than 32 used pages, the last allocated page was being 
**	    returned, a bug.
**	29-Oct-1998 (jenjo02)
**	    Added DM0P_PHYSICAL_IX flag to cause the refix of the last page
**	    to take an IX page lock instead of X when row locking. 
**	    Test failure of fix_page and allow certain errors (like last 
**	    page no longer exists).
**      14-Jul-2008 (hanal04) Bug 120547
**          Empty FMAP pages may be ahead of the last data page. If
**          DM1P_LASTUSED_DATA was not specified we must scan over any
**          remaining FMAPs to find out.
*/ 
DB_STATUS
dm1p_lastused(
    DMP_RCB	*r,
    u_i4	flag,
    DM_PAGENO	*page_number,
    DMP_PINFO	*lastusedPinfo,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DM1P_FHDR	    *fhdr = NULL;
    DM1P_FMAP	    *fmap = NULL, *ifmap = NULL;
    i4	    fmap_block;
    i4	    index;
    i4	    bitmap_index;
    i4	    pageno = -1, fmap_pageno, max_sms_pageno = -1;
    DB_STATUS	    status;
    DB_STATUS	    s;

    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    i4	    fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    i4	    fhdr_count;
    u_char	    *fhdr_map;

    i4	    mbits = DM1P_MBIT_MACRO(pgtype, pgsize);

    DM1P_LOCK	    fhdr_lock, fmap_lock, ifmap_lock;
    i4	    fmap_firstbit;
    i4	    bit_index;
    bool    skip = FALSE;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fhdr_lock.lock_id.lk_unique = 0;
    fmap_lock.lock_id.lk_unique = 0;
    ifmap_lock.lock_id.lk_unique = 0;


    do
    {
	pageno = -1;

	/*
	** Lock the FHDR and FMAP(s) for update.
	** Heaps allocations are single-threaded anyway, so
	** there's nothing to be gained by using read locks.
	*/
	fhdr_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
	status = fix_header(r, &fhdr, &fhdr_lock, dberr);

	if (status == E_DB_OK)
	{
	    /*	Scan all maps for a free page. */
	    fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr);
            max_sms_pageno = DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(pgtype, fhdr);

	    for (index = 0; index < fhdr_count; index++)
	    {
		fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, index);
		/*
		** Ignore pages without hints set unless this is the last
		** FMAP, in which case it must describe the last used page.
		*/
		if (VPS_TEST_FREE_HINT_MACRO(pgtype, fhdr_map) == 0 &&
		    index < (fhdr_count - 1))
		{
                    if((flag & DM1P_LASTUSED_DATA) == 0)
                    {
                        /* We need to record the highest used page including
                        ** SMS page numbers, we'll continue after we've
                        ** fixed the fmap and grabbed its pageno.
                        */
                        skip = TRUE;
                    }
                    else
                    {
		        continue;
                    }
		}

		fmap_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
		status = fix_map(r,
				 VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map), 
				 &fmap, &fmap_lock,
				 &fhdr, &fhdr_lock, dberr);
		
		if (status != E_DB_OK)
		    break;

		fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);

                fmap_pageno = DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(pgtype, fmap);
                if(fmap_pageno > max_sms_pageno)
                    max_sms_pageno = fmap_pageno;

                if(skip)
                {
                    skip = FALSE;

                    /*  Unfix this FMAP block, move on to the next. */
                    status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL,
                                            (DM1P_LOCK *)NULL, dberr);
                    if (status != E_DB_OK)
                        break; 
                    continue;
                }
		
		/*  Start search at lowest free bit on page. */

		bitmap_index = fmap_firstbit / DM1P_BTSZ;
		bit_index = fmap_firstbit % DM1P_BTSZ;

		for (; bitmap_index < mbits; 
		     bitmap_index++, bit_index = 0)
		{
		    u_i4	map;

		    /*  Check if any bits on in this bitmap. */
		    map = DM1P_VPT_GET_FMAP_BITMAP_MACRO(pgtype, fmap, bitmap_index);

		    /*	Scan bitmap for lowest bit set. */
		    for (map >>= bit_index; map; map >>= 1, bit_index++)
		    {
			/*  Skip if bit is clear. */

			if ((map & 1) == 0)
			    continue;

			/*	Compute page number. */

			pageno = index * fseg +
			    bitmap_index * DM1P_BTSZ + bit_index - 1;
			break;
		    }
		    if (pageno >= 0)
			break;
		}

		/*
		**  If no pages found and we are in the last FMAP, then set
		**  pageno to the lastbit in the bitmap.
		*/

		if (pageno < 0 && index + 1 == fhdr_count)
		    pageno = index * fseg +
			DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap);

		/*  If we have found page then break out. */

		if (pageno >= 0)
		    break;

		/*  Unfix this FMAP block, move on to the next. */
		status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, 
					(DM1P_LOCK *)NULL, dberr);
		if (status != E_DB_OK)
		    break;
	    }

            if((flag & DM1P_LASTUSED_DATA) == 0)
            {
                /* Scan any remaining FMAPs to get the highest page number */
                for (index++; index < fhdr_count; index++)
                {
                    fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, index);
                    ifmap_lock.fix_lock_mode = FMAP_READ_LKMODE;
                    status = fix_map(r,
                                    VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map),
                                    &ifmap, &ifmap_lock,
                                    &fhdr, &fhdr_lock, dberr);

                    if (status != E_DB_OK)
                        break;

                    fmap_pageno = DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(pgtype, ifmap);
                    if(fmap_pageno > max_sms_pageno)
                        max_sms_pageno = fmap_pageno;

                    /*  Unfix this FMAP block, move on to the next. */
                    status = unfix_map(r, &ifmap, &ifmap_lock, (DM1P_FHDR **)NULL,
                                            (DM1P_LOCK *)NULL, dberr);
                    if (status != E_DB_OK)
                        break;

                }
            }
	}

	/*
	**	The following code works for HEAP files because the FHDR precedes
	**  all FMAP's and the FMAP's are also in ascending order.  This is
	**	not true for converted BTREE files, but is not used for them.
	*/

	if ((status == E_DB_OK) && ( flag & DM1P_LASTUSED_DATA ))
	{
	    /*
	    **  Move last page pointer backwards if it's sitting on an
	    **  FHDR or FMAP page.
	    */

	    for (index = fhdr_count; --index >= 0; )
	    {
		if (VPS_NUMBER_FROM_MAP_MACRO(pgtype,
		       DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, index)) == pageno)
		    pageno--;
	    }
	    if (pageno == DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(pgtype, fhdr))
		pageno--;
	}
        else
        {
             if((status == E_DB_OK) && (max_sms_pageno > pageno))
                 pageno = max_sms_pageno;
        }

	/* If instructed, fix the lastused page for WRITE */
	if (status == E_DB_OK && lastusedPinfo)
	{
	    /*
	    ** While we still have the FHDR and FMAP fixed and locked,
	    ** try to fix the lastused page, but with the NOWAIT option.
	    ** If the page is locked by another session (we would have to
	    ** wait for the lock), attempt the fix again, this time waiting
	    ** for the lock, then unfix the page, and re-find the possibly
	    ** new lastpage. While we wait for the lock, the "lastused" page
	    ** may be deallocated by recovery, so this assures that we will
	    ** always end up fixing the genuine lastpage.
	    **
	    ** When refixing with wait, add DM0P_PHYSICAL_IX flag. This causes
	    ** an IX lock to be taken instead of an X lock when row locking.
	    */
	    status = dm0p_fix_page(r, pageno, (DM0P_WRITE | DM0P_NOWAIT), 
				lastusedPinfo, dberr);
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_ERROR && dberr->err_code == E_DM004C_LOCK_RESOURCE_BUSY)
		{
		    /* Unfix, unlock FMAP and FHDR */
		    status = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
		    if (status == E_DB_OK)
			status = dm0p_fix_page(r, pageno, 
			    (DM0P_WRITE | DM0P_PHYSICAL | 
			     DM0P_PHYSICAL_IX | DM0P_NOESCALATE), 
						lastusedPinfo, dberr);
		    if (status == E_DB_OK)
			status = dm0p_unfix_page(r, DM0P_UNFIX, 
						lastusedPinfo, dberr);
		    /* fix_page may fail on acceptable errors */
		    else if (dberr->err_code == E_DM9202_BM_BAD_FILE_PAGE_ADDR ||
		       	     dberr->err_code == E_DM9206_BM_BAD_PAGE_NUMBER)
		    {
			CLRDBERR(dberr);
			status = E_DB_OK;
		    }
		}
	    }
	}
    } while (status == E_DB_OK && lastusedPinfo && lastusedPinfo->page == NULL);


    *page_number = pageno;

    /*	Unfix the FMAP page. */

    if (fmap)
    {
	s = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, &local_dberr);
	if (s != E_DB_OK && status == E_DB_OK)
	{
	    *dberr = local_dberr;
	    status = s;
	}
    }

    /*	Unfix the FHDR page. */

    if (fhdr)
    {
	s = unfix_header(r, &fhdr, &fhdr_lock,
				&fmap, &fmap_lock, &local_dberr);
	if (s != E_DB_OK && status == E_DB_OK)
	{
	    *dberr = local_dberr;
	    status = s;
	}
    }

    if (DB_FAILURE_MACRO(status))
    {
	dm1p_log_error(E_DM92D5_DM1P_LASTUSED, dberr, 
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);
    }
    return (status);
}

/*{
** Name: dm1p_dump - Dump bitmaps/pagetypes  to user session.
**
** Description:
**	This routine is called to display the bitmap for debugging purposes.
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	flag				Either:
**					    DM1P_D_BITMAP   - Dump bitmaps.
**					    DM1P_D_PAGETYPE - Dump pagetypes.
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	31-March-1992 (rmuth)
**	    Add function prototype, change err_code to i4 from DB_STATUS.
**	18-May-1992 (rmuth)
**	    Removed the on-the-fly conversion code, means fix_header() can
**	    no longer return E_DB_INFO so remove this.
**	29-August-1992 (rmuth)
**	    - Rewrote the routine added some extra information. Also made
**	      sure we do not try and fix pages passed the highwater mark.
**	    - Add rcb paramter to dm1p_log_error.
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	    - If a temporary table get the allocation from the TCB.
**	    - Total_pages is really last_pageno.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Change DM1P_FSEG references to DM1P_FSEG_MACRO.
**	14-jan-1998 (nanpr01)
**	    Distinguish the empty data and overflow pages for hash table
**	    for new VDBA display.
**       7-June-2004 (hanal04) Bug 112264 INGSRV2813
**          The last ingres allocated page may reside in a different
**          FMAP to the fhdr_hwmap which actually indicates the first FMAP
**          with free pages. If the current FMAP was ever full scan
**          the FMAPs to ensure highwater_mark is set correctly.
**	28-Feb-2007 (kibro01) b117461
**	    hw-mark of fseg means the FMAP has not yet been used, so might
**	    not contain the last allocated page
**      23-Feb-2009 (hanal04) Bug 121652
**          Correct calculation for highwater_mark.
*/
DB_STATUS
dm1p_dump(
    DMP_RCB	*rcb,
    i4	flag,
    DB_ERROR	*dberr )
{
    DMP_RCB	*r = rcb;
    DMP_TCB     *t = r->rcb_tcb_ptr;
    i4          pgtype = t->tcb_rel.relpgtype;
    DM1P_FHDR	*fhdr = 0;
    DM1P_FMAP	*fmap = 0;
    DB_STATUS	status, s;
    i4     last_pageno;
    DM_PAGENO   highwater_mark = 0;
    i4		fhdr_count;
    i4		fmap_hw_mark;
    i4		fmap_lastbit;
    i4		fhdr_hwmap;
    i4		fseg;
	
    DM1P_LOCK	fhdr_lock, fmap_lock;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fhdr_lock.lock_id.lk_unique = 0;
    fmap_lock.lock_id.lk_unique = 0;

    do
    {
	fhdr_lock.fix_lock_mode = FHDR_READ_LKMODE;
        status = fix_header(r, &fhdr, &fhdr_lock, dberr);
    	if ( status != E_DB_OK )
	    break;

        fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(t->tcb_rel.relpgtype,fhdr);
	fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(t->tcb_rel.relpgtype, fhdr);

	/*
	** Work out the tables highwater mark pageno from FHDR/FMAP(s).
	*/
	fmap_lock.fix_lock_mode = FMAP_READ_LKMODE;
	status = fix_map(
		    r, 
		    VPS_NUMBER_FROM_MAP_MACRO(t->tcb_rel.relpgtype,
		      DM1P_VPT_GET_FHDR_MAP_MACRO(t->tcb_rel.relpgtype, fhdr, 
		      (fhdr_hwmap - 1))), 
		    &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
	if (status != E_DB_OK)
	    break;

        fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(t->tcb_rel.relpgtype, 
									fmap);
        fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(t->tcb_rel.relpgtype, 
									fmap);

	fseg = DM1P_FSEG_MACRO(t->tcb_rel.relpgtype,t->tcb_rel.relpgsize);

        highwater_mark = 
            ( DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap) * fseg ) +
               fmap_hw_mark;

        /* Check subsequent FMAPs for allocated pages */
	while((fmap_lastbit == fmap_hw_mark) &&
	      (fhdr_hwmap < fhdr_count) &&
	      (fmap_hw_mark != -1))
        {
	    /* Unfix current FMAP */
	    status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, 
                              (DM1P_LOCK *)NULL, dberr);
	    if (status != E_DB_OK)
	        break;

            /* Check next FMAP */
            fhdr_hwmap++;
            status = fix_map(
		    r,
		    VPS_NUMBER_FROM_MAP_MACRO(t->tcb_rel.relpgtype,
		      DM1P_VPT_GET_FHDR_MAP_MACRO(t->tcb_rel.relpgtype, fhdr,
			(fhdr_hwmap - 1))),
                    &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
            if (status != E_DB_OK)
                break;

            fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(t->tcb_rel.relpgtype,
									fmap);
            fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(t->tcb_rel.relpgtype,
									fmap);
            if ((fmap_hw_mark != -1) && (fmap_hw_mark != fseg))
            {
		/* This FMAP has allocated pages adjust highwater_mark */
                highwater_mark = 
                    ( DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap) * fseg ) +
                       fmap_hw_mark;
            }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** Unfix current FMAP, leave FHDR fixed.
	*/
	status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, (DM1P_LOCK *)NULL,
				dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Determine the physical number of pages in the table
	*/
	if ( t->tcb_temporary == TCB_TEMPORARY )
	{
	    last_pageno = t->tcb_table_io.tbio_lalloc;
	}
	else
	{
	    status = dm2f_sense_file(
		    t->tcb_table_io.tbio_location_array,
	    	    t->tcb_table_io.tbio_loc_count,  
		    &t->tcb_rel.relid,
	    	    &t->tcb_dcb_ptr->dcb_name,
	    	    (i4 *)&last_pageno, 
		    dberr); 
	    if (status != E_DB_OK)
	    	break;
	}


        /*	
    	** Determine if this is a bitmap dump or a pagetype dump. 
    	*/
    	if ( flag & DM1P_D_BITMAP ) 
    	{
	    status = display_bitmaps(
				rcb,
				highwater_mark,
				last_pageno,
				fhdr,
				&fhdr_lock,
				dberr );
	    if ( status != E_DB_OK )
		break;
        }


        if  (flag & DM1P_D_PAGETYPE)
	{
	    status = display_pagetypes(
				rcb,
				highwater_mark,
				last_pageno,
				dberr );
	    if ( status != E_DB_OK )
		break;

	    /*
	    ** If a hash structure also dump the table showing the
	    ** relation of overflow pages to hash bucket pages
	    */
	    if ( rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_HASH )
	    {
	        status =  display_hash( rcb, highwater_mark, dberr);
		if ( status != E_DB_OK )
		    break;
	    }
        } 


    	/*
    	** Unfix the FHDR
    	*/
    	status = unfix_header(r, &fhdr, &fhdr_lock,
				&fmap, &fmap_lock, dberr);
    	if ( status != E_DB_OK )
		break;


    } while (FALSE);
   
    if ( status == E_DB_OK )
	return( E_DB_OK );

    if (fmap)
    {
	s = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, &local_dberr);
	if (s != E_DB_OK && status == E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }

    if (fhdr)
    {
	s = unfix_header(r, &fhdr, &fhdr_lock,
				&fmap, &fmap_lock, &local_dberr);
	if (s != E_DB_OK && status == E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }

    dm1p_log_error(E_DM92D7_DM1P_DUMP, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    return (status);
}

/*{
** Name: display_pagetypes - Display the tables pagetypes to the user session.
**
** Description:
**	This routine is called to display the tables page types to the
**	user session. It will scan the whole table upto the highwater mark
**	displaying each pages type.  It is used for debugging purposes.
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	highwater_mark			Last valid page in the table.
**	total_pages			Total pages on disc for the table.
**	fhdr				Tables FHDR
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**	3-June-1992 (rmuth)
**	    Created.
**	11-jan-1993 (mikem)
**	    Initialize next_char in display_pagetypes() to point to beginning 
**	    of "line_type" buffer at beginning of loop.  "used before set" 
**	    error shown up by lint.
**	08-feb-1993 (rmuth)
**	    - Display overflow data pages.
**	    - For ISAM index pages other than the root or bottom level
**	      display the depth.
*/
static DB_STATUS
display_pagetypes(
    DMP_RCB	*rcb,
    DM_PAGENO	highwater_mark,
    i4	total_pages,
    DB_ERROR	*dberr )
{
#define	    PAGE_PER_LINE   50
#define	    PAGE_PER_GROUP  10
    DMP_RCB	*r = rcb;
    DMP_TCB     *t = r->rcb_tcb_ptr;
    i4	page_number;
    char	line_type[64];
    char	*next_char = line_type;
    char	page_type;
    DMP_PINFO	pinfo;
    char	buffer[132];
    DB_STATUS	status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    do
    {
    	/*
    	** Display the page types for valid Ingres Pages 
    	*/
	r->rcb_state |= RCB_READAHEAD;

	for ( page_number = 0; page_number < (highwater_mark +1); page_number++)
	{
	    status = dm0p_fix_page( r, page_number,
				    DM0P_READ,
				    &pinfo,
				    dberr );
	    if (status != E_DB_OK)
		break;

	    map_pagetype( pinfo.page, r, &page_type);

	    if ( rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_HASH )
	    {
		/* 
		** This is being added for VDBA to print out the overflow
		** page distribution for hash tables.
		*/
	        remap_pagetype( pinfo.page, r, &page_type);
	    }

	    /* 
	    ** See if we need to print a line yet
	    */
	    if ((page_number % PAGE_PER_LINE) == 0)
	    {
		/* If first page then print header */
		if (page_number == 0)
		{
		    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		     "\nPAGE type DUMP for TABLE: %~t, Total pages: %d\n\n",
			sizeof(t->tcb_rel.relid),
			&t->tcb_rel.relid, 
			total_pages + 1);
		}
		else
		{
		    TRformat(print, 0, buffer, sizeof(buffer) - 1,
			"%4* %8d %t\n", 
			page_number - 50,
			next_char - line_type, 
			line_type);
		}

		next_char = line_type;
	    }
	    else
	    {
		/*
		** If end of a group then seperate from next group with 
		** a space
		*/
		if ((page_number % PAGE_PER_GROUP) == 0)
		{
		    *next_char++ = ' ';
		}
	    }

	    /* 
	    ** Add new page to output buffer
	    */
	    *next_char++ = page_type;

	    status = dm0p_unfix_page(r, DM0P_UNFIX, &pinfo, dberr);
	    if ( status != E_DB_OK )
		break;

	} /* for-loop */


	if ( status != E_DB_OK )
	    break;

	/* 
	** Display the pages not yet used
	*/
	page_type = DM1P_P_UNUSED;
	for ( page_number = highwater_mark + 1;
	      page_number <= total_pages ;
	      page_number++
	    )
	{
	    /* 
	    ** See if we need to print a line yet
	    */
    	    if ((page_number % PAGE_PER_LINE) == 0)
            {
	       TRformat(print, 0, buffer, sizeof(buffer) - 1,
		     	"%4* %8d %t\n", 
		     	page_number - 50,
		      	next_char - line_type, 
		     	line_type);

		next_char = line_type;
            }
	    else
	    {
		if ((page_number % PAGE_PER_GROUP) == 0)
		{
		    /*
		    ** If end of a group then seperate from next group with 
		    ** a space
		    */
		    *next_char++ = ' ';
		}
	    }

	    /* 
	    ** Add new page to output buffer
	    */
            *next_char++ = (char)page_type;

	}

        /*
        ** Display the last buffer
        */
        if (next_char != line_type)
        {
            TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    	"%4* %8d %t\n", 
         /* noifr01 24-March 1998: replaced page_number with (page_number -1) */
         /* since loop has exited and therefore page_number is 1 more than the*/
         /* real last page_number on the (last) line to be displayed          */
	    	(page_number -1)  / PAGE_PER_LINE * PAGE_PER_LINE,
	    	next_char - line_type, 
	    	line_type);
    	}


    } while (FALSE);

    if ( status != E_DB_OK )
	dm1p_log_error( E_DM92F4_DM1P_DISPLAY_PAGETYPES, dberr, 
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);

    return( status );
}

/*{
** Name: display_bitmaps - Display the FHDR/FMAP(s) to the user session.
**
** Description:
**	This routine is called to display the FHDR/FMAP(s) for
**	debugging purposes.
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	highwater_mark			Last valid page in the table.
**	total_pages			Total pages on disc for the table.
**	fhdr				Tables FHDR
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**	3-June-1992 (rmuth)
**	    Created.
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	06-may-1996 (thaju02)
**	    New Page Format Project: 
**		Change page header references to use macros.
**		Change DM1P_MBIT reference to DM1P_MBIT_MACRO.
**		Change DM1P_FSEG reference to DM1P_FSEG_MACRO.
*/
static DB_STATUS
display_bitmaps(
    DMP_RCB	*r,
    DM_PAGENO	highwater_mark,
    i4	total_pages,
    DM1P_FHDR	*fhdr,
    DM1P_LOCK   *fhdr_lock,
    DB_ERROR	*dberr )
{
#define	    PAGE_PER_LINE   50
#define	    PAGE_PER_GROUP  10
    DMP_TCB	*t = r->rcb_tcb_ptr;
    i4	pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    char	buffer[132];
    DM1P_FMAP	*fmap = 0;
    DB_STATUS	status, s;
    i4	map_index;
    i4	fseg = DM1P_FSEG_MACRO(pgtype, pgsize);

    DM1P_LOCK	fmap_lock;
    u_char	*fhdr_map;
    DM_PAGENO   fhdr_pageno = t->tcb_rel.relfhdr;
    DM_PAGENO	fmap_pageno;
    i4	fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr);
    i4	fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(pgtype, fhdr);
    i4	fhdr_extend_cnt = DM1P_VPT_GET_FHDR_EXTEND_CNT_MACRO(pgtype, fhdr);
    i4	fhdr_pages = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);
    i4	mbits = DM1P_MBIT_MACRO(pgtype, pgsize);
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    do
    {
        /*
        ** Display the FHDR header information
        */
        TRformat(print, 0, buffer, sizeof(buffer) - 1,
    	    "\nFHDR for TABLE: %~t @ Pageno: %d, Highwater FMAP: %d, FMAP's: %d\n\t\
Allocation: %d, Extend: %d, Number of Extends: %d\n",
      	    sizeof(t->tcb_rel.relid), &t->tcb_rel.relid,
	    fhdr_pageno,
	    fhdr_hwmap,
	    fhdr_count,
	    t->tcb_rel.relallocation,
	    t->tcb_rel.relextend,
	    fhdr_extend_cnt);

    	
	/* 
    	** Display the pages used information
    	*/
    	TRformat(print, 0, buffer, sizeof(buffer) -1,
	    "\nLast disc pageno: %d, Last FHDR/FMAP(s) pageno: %d\n\t\
Last used pageno %d, Pages never used: %d\n",
	    total_pages,
	    fhdr_pages - 1,
	    highwater_mark,
	    (total_pages - highwater_mark) );

   	/*
        ** Display the FMAP information
        */
        for (map_index = 0; map_index < fhdr_count; map_index++)
    	{
            i4	map_bits;
            i4	i;

	    fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, map_index);
	    fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map);

	    fmap_lock.fix_lock_mode = FMAP_READ_LKMODE;
            status = fix_map(
		    r, 
		    fmap_pageno,
		    &fmap, &fmap_lock, &fhdr, fhdr_lock, dberr);
            if (status != E_DB_OK)
	    	break;

	    /*  Compute bits set in FMAP page. */

	    for (map_bits = 0, i = 0; i < mbits; i++)
	    {
	    	u_i4	temp;
		u_i4	map = DM1P_VPT_GET_FMAP_BITMAP_MACRO(pgtype, fmap, i);
	    	/*
	    	** A little bit of magic that computes the number of 
		** bits set in a 36 bit word.  Old MIT hack from 
		** PDP-10 days.
		*/

		temp = (map >> 1) & 033333333333;

		temp = map - temp - ((temp >> 1) & 033333333333);

	    	map_bits += ((temp + (temp >> 3)) & 030707070707) % 077;
            }

            TRformat(print, 0, buffer, sizeof(buffer) - 1,
    		"\nFMAP[%d] @ Pageno: %d, Base Pageno: %d\n\tFirst free bit: %d, Highwater bit: %d\n\tLast Free bit: %d, Free Pages: %d, FHDR Hint: %w\n\n",
		map_index, 
		fmap_pageno,
		map_index * fseg,
		DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap),
		DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap), 
		DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap),
		map_bits,
		"NO,YES",
		VPS_TEST_FREE_HINT_MACRO(pgtype, fhdr_map) == DM1P_FHINT);

	    for (i = 0; i < mbits; i += 4)
            {
	    	if ( DM1P_VPT_GET_FMAP_BITMAP_MACRO(pgtype, fmap, i) == 0 &&
	             DM1P_VPT_GET_FMAP_BITMAP_MACRO(pgtype, fmap, i + 1) == 0 &&
	    	     DM1P_VPT_GET_FMAP_BITMAP_MACRO(pgtype, fmap, i + 2) == 0 &&
	    	     DM1P_VPT_GET_FMAP_BITMAP_MACRO(pgtype, fmap, i + 3) == 0)
		{
	    	    /*  Skip all empty lines. */
	    	    continue;
		}

	        TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    	    "    %8d    %x %x %x %x    %8d\n",
		    ((map_index * fseg) + ((i+4) * DM1P_BTSZ)) - 1,
	    	    DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap)[i + 3], 
		    DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap)[i + 2],
	    	    DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap)[i + 1], 
		    DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap)[i + 0],
		    (map_index * fseg) + ((i) * DM1P_BTSZ)
		    );
            }

            status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, (DM1P_LOCK *)NULL,
				dberr);
            if (status != E_DB_OK)
	    	break;

        }

        if (status != E_DB_OK)
	    break;

    } while (FALSE) ;

    if ( status == E_DB_OK )
	return( E_DB_OK );

    if (fmap)
    {
	s = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, (DM1P_LOCK *)NULL,
				&local_dberr);
	if (s != E_DB_OK && status == E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }

    dm1p_log_error( E_DM92F5_DM1P_DISPLAY_BITMAPS, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    return( status );
}

/*{
** Name: dm1p_verify - Verify the integrity and validity of bitmaps.
**
** Description:
**	This routine is called to verify the correctness of the
**	FHDR/FMAP(s).
**	The checks it performs are as follows :
**	   - Read the FHDR
**	   - Read all FMAP(s) pointed to by the FHDR
**	   - Make sure that there are no used pages that are marked
**	     as free in the FHDR/FMAP(s).
**
** Inputs:
**	dm1u				DM1U control block used to call
**					dm1u_talk for verbose flag.
**	rcb				Pointer to RCB for this table
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK	- Everything is OK.
**	    E_DB_ERROR  - A big error occured verifying bitmaps
**	    E_DB_INFO   - An inconsistency occured such as a used page is
**		          marked as free in the FMAP.
**	    E_DB_WARN	- A small error occurred reading bitmaps.
**
**	Exceptions:
**	    none
**
** History:
**	08-feb-1993 (rmuth)
**	    Created.
**	06-may-1996 (thaju02)
**	    New Page Format Project: 
**		Change page header references to use macros.
**		Change DM1P_FSEG references to DM1P_FSEG_MACRO.
**	 7-jun-96 (nick)
**	    Tolerate bad returns from the page fix and return E_DB_INFO
**	    rather than bailing out.
**      29-jan-2002 (thaju02)
**	    W_DM505C falsely reported by verifydb. Do not solely rely on 
**	    the DMPP_FREE bit. 
**	    In scan_map(), fmap is scanned for free page. Once free page
**	    is located, page is fixed for CREATE (page is zero filled, 
**	    page_page is set and page_stat DMPP_DATA bit is set). If we are 
**	    in sbackup, the before image is taken of page. After rolldb -j, 
**	    verifydb reports W_DM505C.
**	    (b101498)
**       7-June-2004 (hanal04) Bug 112264 INGSRV2813
**          The last ingres allocated page may not be on the last FMAP.
**          This is the case for large pre-allocations.
**          Set highwater_pageno for any FMAP that has allocated pages.
**          The FMAP loop will ensure the final value is based on the last
**          FMAP with allocated pages.
**	28-Feb-2007 (kibro01) b117461
**	    hw-mark of fseg means the FMAP has not yet been used, so might
**	    not contain the last allocated page
*/
DB_STATUS
dm1p_verify(
    DMP_RCB	*r,
    DM1U_CB	*dm1u_cb,
    DB_ERROR	*dberr )
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    DM1P_FHDR	*fhdr = 0;
    DM1P_FMAP	*fmap = 0;
    DMP_PINFO	pinfo;
    DB_STATUS	status, s;
    i4	i;
    i4     page_number;
    i4	highwater_pageno = 0;
    i4	bad_pages = 0;
    i4	failed_reads = 0;
    i4	page_stat;
    char	buffer[132];
    i4	fseg;

    DM1P_LOCK	fhdr_lock, fmap_lock;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fhdr_lock.lock_id.lk_unique = 0;
    fmap_lock.lock_id.lk_unique = 0;

    /*
    **	No logging of actions are performed, and readahead is enabled
    **	because we will be performing full scans of the table.
    */
    r->rcb_logging = 0;
    r->rcb_state |= RCB_READAHEAD;

    do
    {
	/*
	** Force all buffer manager pages to disk
	*/
	status = dm0p_close_page(t, r->rcb_lk_id, r->rcb_log_id,
				 DM0P_CLOSE, dberr);
	if (status != E_DB_OK)
	    break;

	
	/*
	** Make sure we can read the FHDR
	*/
	fhdr_lock.fix_lock_mode = FHDR_READ_LKMODE;
	status = fix_header( r, &fhdr, &fhdr_lock, dberr );
	if ( status != E_DB_OK )
	{
	    dm1u_talk( dm1u_cb, W_DM505A_CORRUPT_FHDR, 2,
	       sizeof(t->tcb_rel.relfhdr), &t->tcb_rel.relfhdr);
	    break;
	}
	else
	{
	    dm1u_talk( dm1u_cb, I_DM53FA_FHDR_PAGE, 2,
	       sizeof(DM_PAGENO),
	       DM1P_VPT_ADDR_FHDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, fhdr));
	}


	/*
	** Make sure we can read all the FMAP(s)
	*/
	for (i = 0; i < 
	    DM1P_VPT_GET_FHDR_COUNT_MACRO(t->tcb_rel.relpgtype, fhdr); 
	    i++ )
	{
	    fmap_lock.fix_lock_mode = FMAP_READ_LKMODE;
	    status = fix_map( r, VPS_NUMBER_FROM_MAP_MACRO(t->tcb_rel.relpgtype,
			DM1P_VPT_GET_FHDR_MAP_MACRO(t->tcb_rel.relpgtype, 
			fhdr, i)),
			&fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
	    if ( status != E_DB_OK )
	    {
		i4 fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(t->tcb_rel.relpgtype,
			DM1P_VPT_GET_FHDR_MAP_MACRO(t->tcb_rel.relpgtype, 
			fhdr, i));

	        dm1u_talk( dm1u_cb, W_DM505B_CORRUPT_FMAP, 4,
		           sizeof(fmap_pageno),  &fmap_pageno,
			   sizeof(i), &i);
		break;
	    }
	    else
	    {
		dm1u_talk( dm1u_cb, I_DM53F9_FMAP_PAGE, 2,
		       sizeof(DM_PAGENO),
		       DM1P_VPT_ADDR_FMAP_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
		       fmap));
	    }

	    fseg = DM1P_FSEG_MACRO(t->tcb_rel.relpgtype,t->tcb_rel.relpgsize);

	    /*
	    ** If this is the last FMAP then calculate the tables
	    ** highwater mark, highest numbered page we have wriiten to.
	    ** This is used when verifing that the page states match the
	    ** FMAP(s). We cannot use tbio_lpageno as this may not yet
	    ** be set.
	    **
	    ** Corrected previous logic. The last FMAP map be a preallocation.
	    ** Set highwater_pageno as long as there are allocated pages
	    ** on the current FMAP. If there are allocated pages on the
	    ** next FMAP highwater_pageno will be update in the next iteration
	    ** of the FMAP loop.
	    */
	    if ( (DM1P_VPT_GET_FMAP_HW_MARK_MACRO(t->tcb_rel.relpgtype, fmap) !=
									-1) &&
	         (DM1P_VPT_GET_FMAP_HW_MARK_MACRO(t->tcb_rel.relpgtype, fmap) !=
									fseg))
	    {
		highwater_pageno = 
		    i * fseg +
		    DM1P_VPT_GET_FMAP_HW_MARK_MACRO(t->tcb_rel.relpgtype, fmap);
	    }


	    /* Unfix the FMAP, leave the FHDR fixed */
	    status = unfix_map( r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, (DM1P_LOCK *)NULL,
				dberr );
	    if ( status != OK )
		break;
	}

	/* See if we encountered an error reading the FMAP(s) */
	if ( status != E_DB_OK )
	    break;


	/*
	** Make sure that we have not marked a used page as free in
	** the FMAP(s). Note this check may be making the verify
	** code overrcomplex but it seems like a reasonable check
	*/
	fmap_lock.fix_lock_mode = FMAP_READ_LKMODE;
	status = fix_map( r, VPS_NUMBER_FROM_MAP_MACRO(t->tcb_rel.relpgtype,
			DM1P_VPT_GET_FHDR_MAP_MACRO(t->tcb_rel.relpgtype, 
			fhdr, 0)),
			&fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
	if ( status != E_DB_OK )
	{
	    break;
	}

	for ( page_number = 0; 
	      page_number <= highwater_pageno;
	      page_number++ )
	{
	    LG_LSN	lsn;

	    status = dm0p_fix_page( r, page_number, DM0P_READ,
				    &pinfo, dberr);
	    if ( status != E_DB_OK )
	    {
		failed_reads++;
		status = E_DB_WARN;
		dm1u_talk( dm1u_cb, W_DM51FE_CANT_READ_PAGE, 1,
			   sizeof(page_number), &page_number);
		continue;
	    }

	    page_stat = DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				pinfo.page);
	    lsn.lsn_high = DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgsize, pinfo.page);
	    lsn.lsn_low = DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgsize, pinfo.page);

	    status = dm0p_unfix_page( r, DM0P_UNFIX, &pinfo, dberr);
	    if ( status != E_DB_OK )
		break;

	    /*
	    ** If this page is not a free page make sure it is marked
	    ** as used in the FMAP
	    */
	    if ( ((page_stat & DMPP_FREE) == 0) &&
		 ((lsn.lsn_high != 0) || (lsn.lsn_low != 0)) )
	    {
		/*
		** E_DB_INFO means the page is marked as used in the FMAP
		** which is what we expect as page is not free.
		*/
		status = test_free( r, fhdr, &fhdr_lock, &fmap, &fmap_lock, 
					page_number, dberr);
		if ( status != E_DB_INFO )
		{
		    if ( status == E_DB_OK )
		    {
			/*
			** Page is marked as free even though page status 
			** indicates that the page is used
			*/
			bad_pages++;
			dm1u_talk( dm1u_cb, W_DM505C_FMAP_INCONSISTENCY, 6,
			  sizeof(page_number), &page_number,
			  sizeof(i2),
			  DM1P_VPT_ADDR_FMAP_SEQUENCE_MACRO(t->tcb_rel.relpgtype, 
			  fmap),
			  sizeof(page_stat), &page_stat);
		    }
		    else
		    {
			/* Big error so bail */
		        break;
		    }

		}
	    }

	}

	/*
	** See if an error occured sanity checking bitmaps
	*/
	if ((status != E_DB_OK) && (status != E_DB_INFO) && 
		(status != E_DB_WARN))
	    break;

	/*
	** Unfix the FMAP and the FHDR
	*/
	status = unfix_map( r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr );
	if ( status != E_DB_OK )
	    break;

	/*
	** See if we encountered any pages which should have been
	** marked as used in the FHDR/FMAP but were really marked as
	** free
	*/
	if (bad_pages)
	{
	    dm1u_talk( dm1u_cb, W_DM505D_TOTAL_FMAP_INCONSIS, 2,
		       sizeof(bad_pages), &bad_pages);
	    return(E_DB_INFO);
	}
	else if (failed_reads)
	{
	    return(E_DB_INFO);
	}

    } while (FALSE);

    if (fmap)
    {
	s = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, &local_dberr );
	if (status == E_DB_OK && s != E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }

    if (fhdr)
    {
	s = unfix_header(r, &fhdr, &fhdr_lock,
				&fmap, &fmap_lock, &local_dberr);
	if (status == E_DB_OK && s != E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }

    if (status == E_DB_OK)
	return (status);

    dm1p_log_error(E_DM92D8_DM1P_VERIFY, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    return (status);
}

/*{
** Name: dm1p_repair - repair a tables FHDR and FMAP(s).
**
** Description:
**	This routine is called to try and repair a tables FHDR and
**	FMAP(s). 
**
**	
** Inputs:
**	rcb				Pointer to RCB for this table
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**	30-feb-1993 (rmuth)
**	    Created.
**	06-may-1996 (thaju02)
**	    New page format support:
**		Passed added parameter page_size in dm1p_init_fhdr routine.
*/
DB_STATUS
dm1p_repair(
    DMP_RCB	*rcb,
    DM1U_CB	*dm1u_cb,
    DB_ERROR	*dberr )
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_PINFO	fhdrPinfo;
    DB_STATUS	status, s;
    DM_PAGENO	last_alloc_pageno;
    i4	last_used_pageno;
    DM_PAGENO	ignore;
    i4	fhdr_pageno;
    DMP_PINFO	pinfo;
    char	buffer[132];
    i4		local_err;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    **	No logging of actions are performed, and readahead is enabled
    **	because we will be performing full scans of the table.
    */

    r->rcb_logging = 0;
    r->rcb_state |= RCB_READAHEAD;

    fhdrPinfo.page = NULL;

    do
    {
	/*	
	** Find the total number of pages in the file
	*/
	status = dm2f_sense_file(
	    t->tcb_table_io.tbio_location_array,
	    t->tcb_table_io.tbio_loc_count,  
	    &t->tcb_rel.relid,
	    &t->tcb_dcb_ptr->dcb_name,
	    (i4 *)&last_alloc_pageno, dberr); 
	if (status != E_DB_OK)
	    break;

	/*
	** Set tbio_lalloc field, used to make sure we do not try and
	** read past the end-of-file.
	*/
	t->tcb_table_io.tbio_lalloc = last_alloc_pageno;

	/*
	** Force all buffer manager pages to disk. 
	*/
	status = dm0p_close_page( t, r->rcb_lk_id, r->rcb_log_id, DM0P_CLOSE,
	    		         dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Reinitialise the FHDR so that we rebuild the FMAP(s)
	*/
        status = dm0p_fix_page( r, t->tcb_rel.relfhdr,
		                DM0P_WPHYS | DM0P_SCRATCH | DM0P_LOCKREAD, 
			        &fhdrPinfo, dberr); 
        if (status != E_DB_OK)
	    break;

	dm0p_pagetype( &t->tcb_table_io, fhdrPinfo.page,
		       r->rcb_log_id, DMPP_FHDR);
	dm1p_init_fhdr((DM1P_FHDR*)fhdrPinfo.page, &ignore, 
			t->tcb_rel.relpgtype, t->tcb_rel.relpgsize);

	status = dm0p_unfix_page(r, DM0P_UNFIX, &fhdrPinfo, dberr);
	if ( status != E_DB_OK )
	    break;

	/*
	** Read through the table looking for the first unformatted 
	** page we will use this as the highwater mark
	*/
	for ( last_used_pageno = 0; last_used_pageno <= last_alloc_pageno; 
	      last_used_pageno++)
	{
	    status = dm0p_fix_page( r, last_used_pageno, DM0P_WRITE, &pinfo,
				    dberr);
	    if (status != E_DB_OK)
	    {
	        if (dberr->err_code == E_DM9206_BM_BAD_PAGE_NUMBER)
	        {
		    last_used_pageno--;
		    status = E_DB_OK;
		    break;
	        }
		else if (dberr->err_code == E_DM920C_BM_BAD_FAULT_PAGE)
		{
		    dm1u_talk( dm1u_cb, W_DM51FE_CANT_READ_PAGE, 2,
			       sizeof(last_used_pageno),&last_used_pageno);
		}
		    else
		    {
			 /*
			 ** Big error so bail
			 */
			 break;
		     }
	    }
	    else
	    {
		status = dm0p_unfix_page(r, DM0P_UNFIX, &pinfo, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	}

	/*
	** Rebuild the FHDR and FMAP(s)
	*/

	if ( last_used_pageno > last_alloc_pageno )
	    last_used_pageno = last_alloc_pageno;

	fhdr_pageno = t->tcb_rel.relfhdr;

	status = dm1p_rebuild( r, last_alloc_pageno, last_used_pageno,
			       &fhdr_pageno, dberr );
	if ( status != E_DB_OK )
	    break;



    } while (FALSE);

    if (fhdrPinfo.page)
    {
	s = dm0p_unfix_page(r, DM0P_UNFIX, &fhdrPinfo, &local_dberr);
	if (status == E_DB_OK && s != E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }

    if ( status != OK )
	dm1p_log_error(E_DM93EF_DM1P_REPAIR, dberr, 
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);

    return (status);
}

/*{
** Name: dm1p_init_fhdr - Initialises a FHDR.
**
** Description:
**	This takes an empty page formatted by dm1c_format and converts
**	it into an empty FHDR page. It will also return the page number
**	of the next FMAP page.
**
** Inputs:
**	fhdr				Pointer to FHDR page to format.
**
** Outputs:
**	next_fmap_pageno		Pointer to returned next fmap page
**					page number..
**                                      
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** History:
**	22-April-1992 (rmuth)
**	    Created.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.  Removed check_sum of FHDR and FMAP pages.
**	06-may-1996 (thaju02)
**	    New page format support:
**		Change page header references to use macros.
**	        Added page_size parameter in this routine.
**      14-Aug-2008 (huazh01)
**          Instead of using DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO() to init
**          the fhdr, we ought to use DM1P_VPT_INIT_FHDR_PAGE_STAT_MACRO(). 
**          bug 120535.  
*/
VOID
dm1p_init_fhdr(
    DM1P_FHDR   *fhdr,
    DM_PAGENO	*next_fmap_pageno,
    i4		pgtype,
    DM_PAGESIZE	page_size )
{
    /*	Initialize the FHDR. */
    
    MEfill(DM1P_SIZEOF_FHDR_MAP_MACRO(pgtype, page_size), '\0', 
	DM1P_VPT_FHDR_MAP_MACRO(pgtype, fhdr));
    DM1P_VPT_SET_FHDR_PAGE_MAIN_MACRO(pgtype, fhdr, DM1P_FNEW);
    DM1P_VPT_SET_FHDR_COUNT_MACRO(pgtype, fhdr, 0);
    DM1P_VPT_SET_FHDR_PAGES_MACRO(pgtype, fhdr, 
		(DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(pgtype, fhdr) + 1));
    DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, 0);
    DM1P_VPT_SET_FHDR_EXTEND_CNT_MACRO(pgtype, fhdr, 0);
    DM1P_VPT_SET_FHDR_SPARE_MACRO(pgtype, fhdr, 0, 0);
    DM1P_VPT_SET_FHDR_SPARE_MACRO(pgtype, fhdr, 1, 0);
    DM1P_VPT_SET_FHDR_SPARE_MACRO(pgtype, fhdr, 2, 0);
    DM1P_VPT_SET_FHDR_SPARE_MACRO(pgtype, fhdr, 3, 0);
    DM1P_VPT_INIT_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, (DMPP_FHDR | DMPP_MODIFY));

    /*	Compute the page number of the next FMAP page. */

    *next_fmap_pageno = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);
}

/*{
** Name: dm1p_add_fmap - Initialise a FMAP page and add it to the FHDR.
**
** Description:
**	This takes an empty page formatted by dm1c_format and converts
**	it into an empty FMAP page which is then added to the FHDR.
**	It will also return the page number of the next FMAP page.
**
** Inputs:
**	fhdr				Pointer to FHDR.
**	fmap				Pointer to FMAP page to format.
**
** Outputs:
**	next_fmap_pageno		Pointer to returned next FMAP page 
**					number.
**                                      
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** History:
**	22-April-1992 (rmuth)
**	    Created.
**	14-dec-1992 (rogerk & jnash)
**	    Reduced Logging Project.  Removed check_sum of FHDR and FMAP pages.
**	06-may-1996 (thaju02)
**	    New page format support: 
**		Change page header references to use macros.
**		Add page_size parameter to this routine.
**		Pass added page size parameter to dm1p_fminit.
**		Change DM1P_FSEG references to DM1P_FSEG_MACRO.
*/
VOID
dm1p_add_fmap(
    DM1P_FHDR   *fhdr,
    DM1P_FMAP   *fmap,
    DM_PAGENO	*next_fmap_pageno,
    i4          pgtype,
    DM_PAGESIZE	pgsize )
{
    i4	fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    DM_PAGENO	fmap_pageno = DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(pgtype, fmap);
    DM_PAGENO	fhdr_pageno = DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(pgtype, fhdr);
    i4	fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr);
    i4	fhdr_pages = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);

    /*	Initialize the FMAP */

    dm1p_fminit(fmap, pgtype, pgsize);

    DM1P_VPT_SET_FMAP_SEQUENCE_MACRO(pgtype, fmap, fhdr_count);

    /*	Add FMAP to FHDR. */

    VPS_MAP_FROM_NUMBER_MACRO(pgtype,
	DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, fhdr_count), fmap_pageno);
    DM1P_VPT_SET_FHDR_COUNT_MACRO(pgtype, fhdr, ++fhdr_count);
    DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, DMPP_MODIFY);

    if (fmap_pageno >= fhdr_pages)
    {
	fhdr_pages = fmap_pageno + 1;
    	DM1P_VPT_SET_FHDR_PAGES_MACRO(pgtype, fhdr, fhdr_pages);
    }


    /*	Compute page number of next FMAP page. */

    *next_fmap_pageno = ((fhdr_pages + fseg - 1) / fseg) * fseg;

    if (*next_fmap_pageno < fhdr_pages)
	*next_fmap_pageno = fmap_pageno + 1;
}

/*{
** Name: dm1p_single_fmap_free - Updates FMAP and FHDR and marks pages free.
**
** Description:
**	This routine marks a range of pages in a single FMAP as free.
**	If the range specified spans more than one FMAP then this
**	routine will mark the appropriate pages in the current FMAP
**	as free and return the pageno of the next FMAP.
**	If the first_bit to last_bit range spans more then one FMAP or the 
**	FMAP past in is not correct,the first_free value is updated and 
**	new_map is set to tell the caller which FMAP page to call this routine 
**	with the next time.
**	A special return status of E_DB_INFO is used to tell the caller
**	of it's responsibility.  Otherwise the is markes the pages all
**	on the current FMAP and returns.
**
** Inputs:
**	fhdr				Pointer to FHDR.
**	fmap				Pointer to FMAP.
**	first_free			Pointer to first bit to free.
**	last_free			Last bit to free.
** Outputs:
**	first_free			Pointer to updated first bit to free.
**	new_map				Pointer to next FMAP page number.
**      err_code			Pointer to error return value.
**                                
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			Call again with new_map as the FMAP.
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	31-March-1992 (rmuth)
**	    Add function prototype, change err_code to i4 form DB_STATUS.
**	3-June-1992 (rmuth)
**	    Changed to dm1p_single_fmap_free, changed all pages to be of type
**	    DM_PAGENO.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.  Removed check_sum of FHDR and FMAP pages.
**	06-may-1996 (thaju02)
**	    New page format support:
**		Change page header references to use macros.
**		Add page_size parameter to this routine.
**		Change DM1P_FSEG references to DM1P_FSEG_MACRO.
**	14-Oct-2008 (kibro01) b120945
**	    Mark the FHDR page as modified when setting its free hint so that
**	    the last used data page will be correctly picked up as being on
**	    this FMAP.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Clarify the code a bit to avoid potential issues with DM_PAGENO
**	    being unsigned.  (first-1 is a giant positive if first == 0.)
*/
DB_STATUS
dm1p_single_fmap_free(
    DM1P_FHDR   *fhdr,
    DM1P_FMAP   *fmap,
    i4          pgtype,
    DM_PAGESIZE	pgsize,
    DM_PAGENO	*first_free,
    DM_PAGENO	*last_free,
    DM_PAGENO	*new_map,
    DB_ERROR	*dberr )
{
    DM_PAGENO   first = *first_free;
    DM_PAGENO   last = *last_free;
    DM_PAGENO	bit_pageno;		/* Page # corresponding to "bit" */
    i4	bit;
    i4	first_bitno, last_bitno;	/* Bits on fmap page, 0..fseg-1 */
    i4	map_index;
    i4	i;
    DB_STATUS	status = E_DB_OK;

    i4	fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    DM_PAGENO	fhdr_pageno = DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(pgtype, fhdr);
    i4	fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr);
    i4	fhdr_pages = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);
    i4	fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(pgtype, fhdr);
    u_char	*fhdr_map;

    DM_PAGENO	fmap_pageno = DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(pgtype, fmap);
    i4	fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);
    i4	fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap);
    i4	fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap);

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(50))
	TRdisplay("dm1p_single_fmap_free: first %d last %d\n", first, 
			last);

    map_index = first / fseg;

    if (map_index > fhdr_count)
    {
	SETDBERR(dberr, 0, E_DM92F2_DM1P_SINGLE_FMAP_FREE);
	return (E_DB_ERROR);
    }

    /* Compute starting fmap, and if caller handed us the wrong fmap,
    ** tell him what the right one is and return now.
    ** (In this way, callers don't need to match up first and fmaps.)
    */
    fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, map_index);

    if (VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map) != fmap_pageno)
    {
	*new_map = VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map);
	return (E_DB_INFO);
    }

    /* Clip first..last to this FMAP.  If there's more to do, set return
    ** so caller knows.
    */
    if (last / fseg > map_index)
    {
	last = map_index * fseg + fseg - 1;
	*first_free = last + 1;
	*new_map = VPS_NUMBER_FROM_MAP_MACRO(pgtype,
	    DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, map_index + 1));
	status = E_DB_INFO;
    }
    first_bitno = first % fseg;
    last_bitno = last % fseg;

    /*	Turn on free bits for range of pages, skip FHDR page. */

    bit_pageno = first;
    for (bit = first_bitno; bit <= last_bitno; bit++)
    {
	if (bit_pageno != fhdr_pageno)
	{
	    DM1P_VPT_SET_FMAP_BITMAP_MACRO(pgtype, fmap, 
		bit / DM1P_BTSZ, 
		1 << (bit % DM1P_BTSZ));
	}
	++ bit_pageno;
    }

    /*	Mark any FMAP pages in this range as used. */

    for (i = 0; i < fhdr_count; i++)
    {
	bit_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype, 
				DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, i));

	if (bit_pageno >= first && bit_pageno <= last)
	{
	    bit = bit_pageno % fseg;
	    DM1P_VPT_CLR_FMAP_BITMAP_MACRO(pgtype, fmap,
		bit / DM1P_BTSZ,
		(1 << (bit % DM1P_BTSZ)));
	}
    }

    /*	Update FHDR and FMAP. */

    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);
    DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, DMPP_MODIFY);

    if (first_bitno < fmap_firstbit)
	DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, first_bitno);

    if (fmap_hw_mark == fseg || first_bitno - 1 > fmap_hw_mark)
	DM1P_VPT_SET_FMAP_HW_MARK_MACRO(pgtype, fmap, first_bitno - 1);

    if (fmap_lastbit == fseg || last_bitno > fmap_lastbit)
	DM1P_VPT_SET_FMAP_LASTBIT_MACRO(pgtype, fmap, last_bitno);

    if (last + 1 > fhdr_pages)
	DM1P_VPT_SET_FHDR_PAGES_MACRO(pgtype, fhdr, last + 1);

    if (fhdr_hwmap == 0)
	DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, map_index + 1);

    /*	Set Free hint in FHDR for this FMAP. */

    VPS_SET_FREE_HINT_MACRO(pgtype, fhdr_map);

    /* status might be INFO from up above */
    return (status);
}

/*{
** Name: dm1p_used_range - Updates FMAP and FHDR and marks pages used.
**
** Description:
**	This marks the range of specified pages as used.  If the range spans
**	a bitmap block boundary a special status value is returned and the
**	correct next map block number is return with an updated starting
**	point value.  The caller is expected to materialize the correct
**	FMAP page and call this routine again.
**
** Inputs:
**	fhdr				Pointer to FHDR.
**	fmap				Pointer to FMAP.
**	first_used			Pointer to first used page.
**	last_used			Last page to mark as used.
** Outputs:
**	first_used			Updated first used page value.
**      new_map				Pointer to new FMAP page number.
**	err_code			Pointer to error return value.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			Call again with correct FMAP.
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	31-March-1992 (rmuth)
**	    Add function prototype, change err_code to i4 from DB_STATUS.
**	3-June-1992 (rmuth)
**	    - Used by more than just build routines, so changed named and
**	      made sure all pages are of type DM_PAGENO.
**	    - If incorrect fmap passed we should return E_DB_INFO not
**	      E_DB_OK.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.  Removed check_sum of FHDR and FMAP pages.
**	26-feb-1996 (pchang)
**	    Corrected a comment - Turn OFF free bits not ON.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Add page_size parameter to this routine.
**		Change DM1P_FSEG references to DM1P_FSEG_MACRO.
**      01-aug-2008 (thaju02)
**	    verifydb reports W_DM500A, W_DM500C and E_DM93A7 
**	    appears in errlog.log. Cast last_used to i4 in if-stmt 
**	    for fmap_hw_mark to be set correctly. (B120729)
**	16-Nov-2009 (kschendel) SIR 122890
**	    Clarify code a bit to help avoid any further i4 vs u_i4 errors.
*/
DB_STATUS
dm1p_used_range(
    DM1P_FHDR   *fhdr,
    DM1P_FMAP   *fmap,
    i4          pgtype,
    DM_PAGESIZE pgsize,
    DM_PAGENO	*first_used,
    DM_PAGENO	last_used,
    DM_PAGENO	*new_map,
    DB_ERROR	*dberr )
{
    i4	bit;
    i4	map_index;
    i4	first_bitno, last_bitno;
    DB_STATUS	status = E_DB_OK;
    DM_PAGENO	bit_pageno;

    i4	fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    DM_PAGENO	fhdr_pageno = DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(pgtype, fhdr);
    i4	fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr);
    i4	fhdr_pages = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);
    i4	fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(pgtype, fhdr);
    u_char	*fhdr_map;

    DM_PAGENO	fmap_pageno = DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(pgtype, fmap);
    i4	fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);
    i4	fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap);
    i4	fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap);

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(50))
	TRdisplay("dm1p_used_range: first %d last %d\n", *first_used, last_used);

    map_index = *first_used / fseg;

    if (map_index > fhdr_count)
    {
	SETDBERR(dberr, 0, E_DM92F6_DM1P_USED_RANGE);
	return (E_DB_ERROR);
    }

    fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, map_index);

    if (VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map) != fmap_pageno)
    {
	*new_map = VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map);
	return (E_DB_INFO);
    }

    if (last_used / fseg > map_index)
    {
	last_used = map_index * fseg - 1;
	*first_used = last_used + 1;
	*new_map = VPS_NUMBER_FROM_MAP_MACRO(pgtype,
	    DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, map_index + 1));
	status = E_DB_INFO;
    }
    first_bitno = *first_used % fseg;
    last_bitno = last_used % fseg;

    /* Turn off free bits for range of pages */

    for (bit = first_bitno; bit <= last_bitno; bit++)
    {
	DM1P_VPT_CLR_FMAP_BITMAP_MACRO(pgtype, fmap, 
	    bit / DM1P_BTSZ,
	    (1 << (bit % DM1P_BTSZ)));
    }

    /*	Update FHDR and FMAP. */

    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);

    /* If old firstbit was in the range of used, adjust it */
    if (fmap_firstbit >= first_bitno && last_bitno > fmap_firstbit)
	DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, last_bitno + 1);

    if (fmap_hw_mark == fseg || last_bitno > fmap_hw_mark)
	DM1P_VPT_SET_FMAP_HW_MARK_MACRO(pgtype, fmap, last_bitno);

    if (fmap_lastbit == fseg || last_bitno > fmap_lastbit)
	DM1P_VPT_SET_FMAP_LASTBIT_MACRO(pgtype, fmap, last_bitno);

    /*
    ** last_used holds a pageno which start from zero, fhdr_pages
    ** holds the number of pages, hence we add one.
    */
    if (last_used + 1 > fhdr_pages)
	DM1P_VPT_SET_FHDR_PAGES_MACRO(pgtype, fhdr, last_used + 1);

    /* 
    ** see if need to set fhdr highwater mark, this records the highest
    ** number FMAP with a valid page.
    */
    if (map_index >= fhdr_hwmap)
        DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, map_index + 1);

    /*	Set Free hint in FHDR for this FMAP. */

    VPS_SET_FREE_HINT_MACRO(pgtype, fhdr_map);
    DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, DMPP_MODIFY);

    return (status);
}

/*{
** Name: dm1p_free - Updates FMAP and FHDR for range of pages.
**
** Description:
**	This mark a range of page free in the allocation bitmaps.  It is
**	used to recover a copy into a heap that needs to be aborted.
**
** Inputs:
**	rcb			    Pointer to RCB.
**	first_free		    First page in range to mark free.
**	last_free		    Last page in range to mark free.
**
** Outputs:
**	err_code	Error return code.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**      23-oct-91 (jnash)
**          Add error code param to dm1pxfree.
**	13-Dec-1991 (rmuth)
**	    Added new error message E_DM93AA_MAP_PAGE_TO_FMAP.
**	23-April-1992 (rmuth)
**	    Added function prototype, changed err_code to (i4) from
**	    (DB_STATUS) corrected error handling.
**	18-May-1992 (rmuth)
**	    - Removed the on-the-fly conversion code
**	    - Add rcb parameter to dm1p_log_error.
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	06-may-1996 (thaju02)
**	    New page format support:
**		Change page header references to use macros.
**		Pass added page size parameter into dm1p_single_fmap_free().	
*/
DB_STATUS
dm1p_free(
    DMP_RCB	*rcb,
    i4	first_free,
    i4	last_free,
    DB_ERROR    *dberr )
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DM1P_FHDR	    *fhdr;
    DM1P_FMAP	    *fmap = 0;
    DM_PAGENO	    fmap_pageno;
    i4	    map_index;
    DB_STATUS	    status, s;
    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;

    DM1P_LOCK	    fhdr_lock, fmap_lock;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fhdr_lock.lock_id.lk_unique = 0;
    fmap_lock.lock_id.lk_unique = 0;
    
    /*	Fix the FHDR page. */

    fhdr_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
    status = fix_header(r, &fhdr, &fhdr_lock, dberr);
    if (status == E_DB_OK)
    {
	/*	Get first FMAP block address for range. */

	map_index = first_free / DM1P_FSEG_MACRO(pgtype, pgsize);
	if (map_index > (i4)DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr) ||
	    (fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype,
		DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, map_index))) == 0)
	{
	    uleFormat(NULL, E_DM93AB_MAP_PAGE_TO_FMAP, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4 )0, (i4 *)NULL, err_code, 7,
		sizeof(d->dcb_name), &d->dcb_name,
		sizeof(t->tcb_rel.relid), &t->tcb_rel.relid,
		sizeof(t->tcb_rel.relowner), &t->tcb_rel.relowner,
		0, first_free,
		0, map_index,
		0, fmap_pageno,
		0, DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr));
	    SETDBERR(dberr, 0, E_DM92DB_DM1P_FREE);
	    status = E_DB_ERROR;
	}

	for (;status == E_DB_OK;)
	{
	    DM_PAGENO	    new_map;

	    /*	Fix the first/next FMAP page. */
	    
	    fmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
	    status = fix_map(rcb, fmap_pageno, 
			    &fmap, &fmap_lock, &fhdr, &fhdr_lock, dberr);
	    if (status != E_DB_OK)
		break;

	    /*	Mark range of pages contained in this FMAP as free. */

	    status = dm1p_single_fmap_free(
				fhdr, 
				fmap, 
				t->tcb_rel.relpgtype,
				t->tcb_rel.relpgsize,
				(DM_PAGENO *)&first_free, 
				(DM_PAGENO *)&last_free, 
				&new_map, dberr);
	    if ( status != E_DB_INFO )
	       break;

	    /* Unfix the FMAP, leave the FHDR fixed */
	    status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL, (DM1P_LOCK *)NULL,
				dberr);
	    if (status != E_DB_OK )
		break;
	    
	    /*  Switch to next FMAP. */
	    fmap_pageno = new_map;
	}
    }

    if (fmap)
    {
	s = unfix_map(r, &fmap, &fmap_lock, &fhdr, &fhdr_lock, &local_dberr);
        if (s != E_DB_OK && status == E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }
    if (fhdr)
    {
	s = unfix_header(r, &fhdr, &fhdr_lock,
				&fmap, &fmap_lock, &local_dberr);
	if (s != E_DB_OK && status == E_DB_OK)
	{
	    status = s;
	    *dberr = local_dberr;
	}
    }
    if (status == E_DB_OK)
	return (status);

    dm1p_log_error(E_DM92DB_DM1P_FREE, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    return (status);
}
/*{
** Name: dm1p_fmfree - Updates FMAP for range of new free pages.
**
** Description:
**	This routine marks a range of pages free in a given FMAP page.
**	It is used both in extend and recovery of extend operations.
**
**	The routine can be given a broad range of free pages that spans
**	more than the range covered by the passed in fmap page.  In this
**	case this routine will only process those page numbers that lie
**	within the range of the given FMAP.
**
** Inputs:
**	fmap			    FMAP page to update.
**	first_free		    First page in range to mark free.
**	last_free		    Last page in range to mark free.
**
** Outputs:
**	none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** History:
**      14-dec-92 (rogerk)
**          Created as part of the Reduced Logging Project.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Add page_size parameter to this routine.
*/
VOID
dm1p_fmfree(
DM1P_FMAP	*fmap,
DM_PAGENO	first_free,
DM_PAGENO	last_free,
i4		pgtype,
DM_PAGESIZE	page_size)
{
    i4	first_bit;
    i4	last_bit;
    i4	bit;

    i4	fseg = DM1P_FSEG_MACRO(pgtype, page_size);
    i4	fmap_sequence = DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap);
    i4	fmap_firstbit;
    i4	fmap_lastbit;
    u_i4	*fmap_bitmap;

    /*
    ** Get first and last free pages that fall within the range of this FMAP.
    */
    if (first_free / fseg < fmap_sequence)
	first_free = fmap_sequence * fseg;

    if (last_free / fseg > fmap_sequence)
	last_free = (fmap_sequence + 1) * fseg - 1;

    /* Return if there are no pages to free */
    if (first_free > last_free)
	return;

    first_bit = first_free % fseg;
    last_bit = last_free % fseg;

    fmap_bitmap = DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap);
    fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);
    fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap);

    for (bit = first_bit; bit <= last_bit; bit++)
	BITMAP_SET_MACRO(fmap_bitmap, bit);

    /*
    ** Update page hints.
    */
    if (first_bit < fmap_firstbit)
	DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, first_bit);
    if (last_bit > fmap_lastbit || fmap_lastbit == fseg)
	DM1P_VPT_SET_FMAP_LASTBIT_MACRO(pgtype, fmap, last_bit);

    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);
    return;
}

/*{
** Name: dm1p_fmused - Updates FMAP for range of new used pages.
**
** Description:
**	This routine marks a range of pages used in a given FMAP page.
**	It is used both in extend and recovery of extend operations.
**
**	The routine can be given a broad range of used pages that spans
**	more than the range covered by the passed in fmap page.  In this
**	case this routine will only process those page numbers that lie
**	within the range of the given FMAP.
**
** Inputs:
**	fmap			    FMAP page to update.
**	first_used		    First page in range to mark used.
**	last_used		    Last page in range to mark used.
**
** Outputs:
**	none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** History:
**      14-dec-92 (rogerk)
**          Created as part of the Reduced Logging Project.
**	15-sep-95 (nick)
**	    Corrected comment.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Add page_size parameter to this routine.
**	27-Oct-2006 (kschendel) SIR 122890
**	    Update the first-free hint too.
*/
VOID
dm1p_fmused(
DM1P_FMAP	*fmap,
DM_PAGENO	first_used,
DM_PAGENO	last_used,
i4		pgtype,
DM_PAGESIZE	page_size)
{
    i4	first_bit;
    i4	last_bit;
    i4	bit;

    i4	fseg = DM1P_FSEG_MACRO(pgtype, page_size);
    i4	fmap_sequence = DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap);	
    i4	fmap_hw_mark, fmap_firstbit;
    u_i4	*fmap_bitmap;

    /*
    ** Get first and last used pages that fall within the range of this FMAP.
    */
    if (first_used / fseg < fmap_sequence)
	first_used = fmap_sequence;

    if (last_used / fseg > fmap_sequence)
	last_used = (fmap_sequence + 1) * fseg - 1;

    /* Return if there are no pages to mark used */
    if (first_used > last_used)
	return;

    first_bit = first_used % fseg;
    last_bit = last_used % fseg;

    fmap_bitmap = DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap);

    for (bit = first_bit; bit <= last_bit; bit++)
	BITMAP_CLR_MACRO(fmap_bitmap, bit);

    /*
    ** Update page hints.
    */
    fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap);

    if (last_bit > fmap_hw_mark || fmap_hw_mark == fseg)
	DM1P_VPT_SET_FMAP_HW_MARK_MACRO(pgtype, fmap, last_bit);

    /* If old firstbit was in the marked-used range, new firstbit is
    ** the next one.
    */
    fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);
    if (fmap_firstbit >= first_bit && last_bit > fmap_firstbit)
	DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, last_bit + 1);

    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);
    return;
}

/*{
** Name: extend - Extends a file, puts pages on free list.
**
** Description:
**      This routines assumes that the header page is already fixed and
**	locked LK_SIX.
**      This page holds the pointer to the free list.  This routine is
**	responsible for obtaining more pages for this file.  The number
**	of pages to allocate is stored in iirelation.relextend.  
**
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      header                          Pointer to the header page.
**	extend				Size to extend table by.
**	flag				DM1P_MINI_TRAN: Inside a mini-tran.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.   May be set to one of the
**                                      following error codes:
**                                      E_DM0042_DEADLOCK,
**                                      E_DM0012_BAD_FILE_ALLOC.
**                                      
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
**          none
**
** History:
**	05-mar-1991 (Derek)
**	    Created.
**	 3-oct-91 (rogerk)
**	    Added fixes for recovery of File Extend operations.
**	    Until we figure out how to properly do REDO recovery without
**	    forcing FHDR/FMAP we have to do more forcing of them.
**	    Force FMAPs as well as FHDR during extend operation as
**	    we don't currently do any REDO recovery of the extend record.
**	    Added "force" flag to mark_free routine to specify that the fmap
**	    page should be forced if unfixed.
**	19-Nov-1991 (rmuth)
**	    Removed the action type DM0P_FHFMPSYNC from dm0p_fix_page so
**	    took out the setting of this action.
**      4-dec-1991 (bryanp)
**          Changed extend()'s processing when it is allocating space for an
**          already-formatted FHDR so that it doesn't format an extra FMAP.
**      10-mar-1992 (bryanp)
**          Don't need to force fmaps & fhdrs for temporary tables.
**          Also, maintain tcb_lalloc after alloc'ing space, and use tcb_lalloc
**          rather than dm2f_sense_file for temporary tables.
**	29-August-1992 (rmuth)
**	    - Added function prototype, this involved the following
**		- Change header from (DMPP_PAGE *) to (DM1P_FHDR *).
**		- Use force_page, instead of explicit dm0p_unfix_page call.
**	    - Add parameter to dm1p_log_error.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	      - Put extend operation inside a Mini Transaction.  Added flag
**		argument to indicate if we are already inside one.
**	      - Added handling of cases where sense returns that there is
**		already more space in the table than the FHDR currently
**		reflects.  It is now possible for an extend request to proceed
**		without actually making a dm2f_alloc request.
**	      - Changed to allocate space BEFORE logging the extend action.
**		The extend action is never undone and is only rolled forward
**		if required (sense is used to check necessity).
**	      - Changed to not call dm1p_add_fmap or mark_free routines. Work
**		is not performed locally or in new dm1p_fmused and dm1p_fmfree
**		routines.
**	      - Added new dm0l_fmap log record when adding new fmap pages.
**	      - Removed check_sum of FHDR and FMAP pages.
**	      - Removed necessity to force newly modified FMAP and FHDR pages.
**	      - Removed setting of tbio_lpageno in this routine.  We should
**		not make any assumptions about formatted pages in this routine
**		since we are only adding unformatted space.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Fixes. Added dm0p_pagetype call.
**	14-dec-1992 (jnash)
**	    Reduced Logging Fix.  Add loc_cnf param to dm0l_extend, 
**	    required by rollforward.
**	03-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve() call.
**	04-jan-1992 (rmuth)
**	     Add check to make sure table does not grow past 
**	     DM1P_MAX_TABLE_SIZE.
**	18-jan-1993 (rogerk)
**	    Set pages_mutexed flag correctly after mutexing pages for FMAP
**	    addition.  Was setting to false when pages where mutexed which
**	    could cause us to leave the pages mutexed after an error.
**	30-feb-1993 (rmuth)
**	    Only log the begin/end mini if we have logging enabled.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**	30-mar-1993 (rmuth)
**	    Add the extend parameter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	15-apr-1994 (kwatts)
**	   - b64114, always put an EXTEND log record in its own mini-xact,
**	     even if that means nesting minis. Because all the logging is
**	     localised to this routine we can be sure that we are nesting
**	     and not interleaving minis (which would be *real* bad.)
**	14-sep-1995 (nick)
**	    Correct error in calculating number of new fmaps required - this 
**	    needs to be done by page # and not # of pages.  Bug #70798.
**	    Also add additional local variable to remove confusion in use 
**	    between page numbers and number of pages - use page_to_alloc as
**	    the page to allocate from and new_physical_pages to reflect the
**	    size after the allocation.  We used to use new_physical_pages for
**	    both.
**	15-sep-1995 (nick)
**	    Test to determine if the current FMAP needs updating were 
**	    incorrect ; the effect of this was that we called dm1p_fmused()
**	    and dm1p_fmfree() with no effect but in addition maked that the
**	    current FMAP now had free pages via the FHDR hint array.  This
**	    caused subsequent attempts to fetch a free page to scan the
**	    bitmap for this FMAP despite there being no free pages in it.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Pass added page size parameter into dm1p_fmfree(), 
**		dm1p_fmused().
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Pass added page size parameter to dm1p_fminit.
**	03-sep-1996 (pchang)
**	    Fixed bug 78492.  Set new_physical_pages to old_physical_pages if
**	    no new physical allocation is needed for the extension.
**	    Fixed bug 77416.  We should take into account any unused pages that 
**	    exist in a table prior to the extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.  Added last_used_page to dm0l_extend() and dm0l_fmap() calls
**	    to enable undo and redo code to properly deal with existing unused 
**	    pages also.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      21-apr-98 (stial01)
**          extend() Set DM0L_PHYS_LOCK if extension table (B90677)
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Pass the fmap highwater mark to dm0l_fmap() to ensure it
**          can be correctly restored during rollforward.
**	10-Nov-2003 (thaju02) INGSRV2593/B111276
**	    Pass highwater mark of new fmap to dm0l_fmap(), which should
**	    be zero, since the new fmap was just created/formatted.
**	28-Dec-2007 (kibro01) b119663
**	    Remove the LG_RS_FORCE flag, since it isn't needed in all
**	    circumstances.  Apply the same conditions that take place when
**	    rollback or recovery is performed.
**      26-Feb-2008 (hanal04) Bug 119920
**          Correct ule_format() parameters for E_DM92BF_DM1P_TABLE_TOO_BIG
**          to prevent SIGSEGVs.
**      23-Feb-2009 (hanal04) Bug 121652
**          The existing code assumed that fhdr_pages have been formatted.
**          If we have a large number of unformatted extends on the table
**          any new FMAPs need to be added to the end of the last used pages
**          of an FMAP that has been used at some point.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
**	02-Apr-2010 (jonj)
**	    SIR 121619 MVCC: extend() may return E_DM004C_LOCK_RESOURCE_BUSY
**	    if new FMAP collides with another page being allocated.
*/
static DB_STATUS
extend( 
DMP_RCB		*rcb, 
DM1P_FHDR	*fhdr, 
DM1P_LOCK	*fhdr_lock,
i4		extend,
i4		extend_flags,
DB_ERROR	*dberr )
{
    DM1P_FMAP	   *fmap = NULL, *ufmap = NULL, *emptyfmap = NULL;
    DMP_RCB	   *r = rcb;
    DMP_TCB        *t = rcb->rcb_tcb_ptr;
    DMP_DCB	   *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO   *tbio = &t->tcb_table_io;
    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    DB_STATUS	   status = E_DB_OK;
    STATUS	   cl_status;
    CL_ERR_DESC	   sys_err;
    DM_PAGENO	   first_page, ufirst_page;
    DM_PAGENO      ulast_page;
    DM_PAGENO	   first_free_page, ufirst_free_page;
    DM_PAGENO	   first_used_page;
    DM_PAGENO	   last_free_page, ulast_free_page;
    DM_PAGENO	   last_used_page;
    LG_LSN	   lsn;
    LG_LSN	   bmini_lsn;
    i4	    old_physical_pages;
    i4	    old_logical_pages;
    i4	    new_physical_pages;
    i4	    page_to_alloc;
    i4	    local_error;
    i4	    extend_amount;
    i4	    allocate_amount;
    i4	    extend_size = extend;
    i4	    fmap_needed;
    i4	    last_fmap, used_fmap, empty_fmap = 0;
    i4	    dm0l_flag;
    i4	    fhdr_loc_config_id;
    i4	    fmap_loc_config_id;
    i4      ufmap_loc_config_id;
    i4      emptyfmap_loc_config_id;
    i4      empty_loc_config_id;
    i4	    loc_id;
    i4	    loc_cnf[DM_LOC_MAX];
    i4	    i;
    i4		   hwmap_update = FALSE;
    i4		   freehint_update = FALSE;
    i4		   fmap_update = FALSE;
    i4		   in_mini = FALSE;

    i4	    fseg = DM1P_FSEG_MACRO(pgtype, pgsize);
    DM_PAGENO	    fhdr_pageno = t->tcb_rel.relfhdr;
    i4	    fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr);
    i4	    fhdr_pages = DM1P_VPT_GET_FHDR_PAGES_MACRO(pgtype, fhdr);
    i4    	    fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(pgtype, fhdr);
    u_char	    *fhdr_map;

    DM_PAGENO	    fmap_pageno, ufmap_pageno, emptyfmap_pageno;
    i4	    fmap_firstbit, ufmap_firstbit, emptyfmap_firstbit;
    i4	    fmap_lastbit, ufmap_lastbit, emptyfmap_lastbit;
    i4	    fmap_hw_mark, ufmap_hw_mark, emptyfmap_hw_mark;
    i4	    fmap_sequence, ufmap_sequence, emptyfmap_sequence;
    i4      last_used_sequnce;
    DB_STATUS	    fmap_status, ufmap_status = E_DB_OK;
    DM1P_LOCK	    fmap_lock, ufmap_lock, emptyfmap_lock;
    i4      lg_force_flag;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fmap_lock.lock_id.lk_unique = 0;
    ufmap_lock.lock_id.lk_unique = 0;
    emptyfmap_lock.lock_id.lk_unique = 0;

    do
    {
	/*
	** Sense the current EOF to determine how much we must extend the
	** file(s) to allocate the requested space.
	**
	** The requested space is assumed to be relextend pages beyond the
	** current EOF as determined by fhdr->fhdr_pages.  Note that
	** the actual physical EOF of the table (returned from dm2f_sense) 
	** may be greater than what is reflected in fhdr_pages.  In this
	** case, the extend operation may not have to actually allocate
	** any disk space but will simply add existing disk space to the
	** free maps and headers.
	**
	** We assume that the caller has acquired the table-extend lock so
	** that this routine is shielded from any concurrency complications.
	** 
	** For a temporary table, we don't use DI_ZERO_ALLOC, so we can't call
	** dm2f_sense_file to figure out the end of file. Instead, we just
	** use tcb_lalloc's current value and assume it's correct.
	*/
	old_logical_pages = fhdr_pages;		/* real pages in file */
	
	if (t->tcb_temporary)
	{
	    old_physical_pages = tbio->tbio_lalloc + 1;
	}
	else
	{
	    status = dm2f_sense_file(tbio->tbio_location_array, 
		    tbio->tbio_loc_count, &t->tcb_rel.relid, 
		    &t->tcb_dcb_ptr->dcb_name, &old_physical_pages, dberr);
	    if (status != E_DB_OK)
		break;
	    old_physical_pages++;	/* sense returns last page number */
	}

	if (extend_flags & DM1P_ADD_TOTAL)
	{
	    /* reduce amount to allocate by what we have already in table */
	    extend_size -= old_logical_pages;      
	}

	/*
	** Determine the number of pages needed to add to the free space.
	** Include any required FMAP pages to describe the new free pages.
	** Note that this calculation may be different than the actual
	** number of pages allocated through dm2f_alloc as there may be
	** pages physically allocated to the file but not yet reflected in
	** the free space.
	**
	** The extend amount is rounded down to the next extend_size boundary.
	**
	** Calculate fmap_needed on page numbers not number of pages.
	*/
	fmap_needed = ((old_logical_pages - 1 + extend_size + fseg) / fseg) 
				- fhdr_count;
	last_fmap = fhdr_count - 1;
        used_fmap = last_fmap - 1;

	/* if modify to add-extend, add fmaps, if setting size to a */
	if (extend_flags & DM1P_ADD_NEW)
	{
	    extend_amount = (fmap_needed + extend_size);
	    extend_amount = (extend_amount / extend_size) * extend_size;
	}
	else
	{
	    /* extend_flags must be DM1P_ADD_TOTAL. Dont add fmaps, reduce */
	    /* total new pages by fmap_needed, then recalc fmap_needed.    */
	    /* This is to prevent overallocation of pages beyond total,    */
	    /* in case we were at an 'fmap_needed' boundary.               */
	    extend_amount = extend_size - fmap_needed;
	    fmap_needed = ((old_logical_pages - 1 + extend_amount + fseg) / fseg) 
				- fhdr_count; 
	    /* re-add the new, possibly smaller num of fmap pages needed */
	    extend_amount += fmap_needed; 
	}
	allocate_amount = 
		    (old_logical_pages + extend_amount) - old_physical_pages;

	/*
	** Expand the physical file size to allocate the new pages.
	*/
	page_to_alloc = old_physical_pages;

	if (allocate_amount > 0)
	{
	    /*
	    ** Make sure extending the table is not going to cause us to
	    ** exceed the maximum table size.
	    */
	    if ( (old_physical_pages + allocate_amount) > 
		DM1P_VPT_MAXPAGES_MACRO(pgtype, pgsize))
	    {
    	        uleFormat( NULL, E_DM92BF_DM1P_TABLE_TOO_BIG,
			    (CL_ERR_DESC *)NULL, ULE_LOG,NULL,
	    		    (char *)NULL, (i4)0, (i4 *)NULL, 
			    err_code, 6,
			    0, DM1P_VPT_MAXPAGES_MACRO(pgtype, pgsize),
			    sizeof(t->tcb_dcb_ptr->dcb_name.db_db_name),
			    t->tcb_dcb_ptr->dcb_name.db_db_name,
	    		    sizeof( t->tcb_rel.relowner.db_own_name ),
			    t->tcb_rel.relowner.db_own_name,
	    		    sizeof( t->tcb_rel.relid.db_tab_name ),
			    t->tcb_rel.relid.db_tab_name,
			    0, old_physical_pages,
			    0, extend_size );
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM92DC_DM1P_EXTEND);
		break;
	    }

	    if (t->tcb_temporary)
	    {
		status = dm2f_alloc_file(tbio->tbio_location_array,
		    tbio->tbio_loc_count, &t->tcb_rel.relid,
		    &t->tcb_dcb_ptr->dcb_name, allocate_amount,
		    &page_to_alloc, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		status = dm2f_galloc_file(tbio->tbio_location_array,
		    tbio->tbio_loc_count, &t->tcb_rel.relid,
		    &t->tcb_dcb_ptr->dcb_name, allocate_amount,
		    &page_to_alloc, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /* allocate calls return last page # allocated so adjust */
	    new_physical_pages = page_to_alloc + 1;
	}
	else
	    new_physical_pages = old_physical_pages;

	tbio->tbio_lalloc = page_to_alloc;

	/*
	** Now that we have successfully added new space to the table,
	** we can update the free space header and maps to add the space
	** to the table.
	*/
	if (DMZ_AM_MACRO(50))
	{
	    TRdisplay("DM1P: extend table %~t size %d pages %d..%d\n",
    		sizeof(t->tcb_rel.relid), &t->tcb_rel.relid, 
		fhdr_pages, old_physical_pages, new_physical_pages-1);
	}

	/*
	** Fix the current last FMAP to reflect the newly added free pages.
	** Note that any new fmaps will be updated later below.
	*/
	fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype,
			DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, last_fmap));

	fmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
	status = fix_map(r, fmap_pageno, &fmap, &fmap_lock,
			    &fhdr, fhdr_lock, dberr);
	if (status != E_DB_OK)
	    break;

	fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);
	fmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap);
	fmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap);
	fmap_sequence = DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap);

	/*
	** Calculate ranges of new pages added to the table which must
	** be marked either free or used in the FMAPS.
        ** Checking for a sequence of empty, unused FMAPs which mean any 
        ** new FMAPs are may be allocated to earlier FMAPs instead of the 
        ** current last FMAP.
	*/
        if ( fmap_needed && (fmap_hw_mark == fseg))
        {
            /* We're going to add an FMAP but the current one has never
            ** been used. Work back to the latest fmap with used pages.
            */
            while(used_fmap >= 0)
            {
                ufmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype,
                          DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, used_fmap));
                if (status != E_DB_OK)
                    break;
	        ufmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
                status = fix_map(r, ufmap_pageno, &ufmap, &ufmap_lock,
                                &fhdr, fhdr_lock, dberr);
                if (status != E_DB_OK)
                    break;

                ufmap_lastbit = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, ufmap);
                ufmap_hw_mark = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, ufmap);
                ufmap_sequence = DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, ufmap);
	        ufmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, ufmap);

                if(ufmap_hw_mark < fmap_lastbit)
                {
                    /* We'll be adding the new FMAP to a different fmap
                    ** to the last_fmap to avoid unformatted pages in the
                    ** middle of the used space.
                    */
                    ufirst_page = (ufmap_sequence * fseg) + ufmap_hw_mark + 1;
                    ulast_page = (ufmap_sequence + 1) * fseg - 1;
                    first_used_page = ufirst_page;
                    last_used_page = first_used_page + fmap_needed - 1;
                    ufirst_free_page = last_used_page + 1;
                    ulast_free_page = (ufmap_sequence * fseg) + ufmap_lastbit;

                    if((last_fmap - used_fmap > 1) &&
                       (emptyfmap_sequence == last_used_page / fseg))
                    {
                        /* One or more unused FMAPs after the last used
                        ** but before the current last fmap.
                        ** Re-fix emptyfmap_pageno as we'll be adding
                        ** new FMAPs to it's pages.
                        */
	                emptyfmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
                        status = fix_map(r, emptyfmap_pageno, &emptyfmap,
                                        &emptyfmap_lock, &fhdr, fhdr_lock, 
                                        dberr);
                        if (status != E_DB_OK)
                            break;
                    }
                    break;
                } 

                if(ufmap_hw_mark == fseg)
                {
                    /* 1 or more empty FMAPs before the latest FMAP
                    ** Maintain a reference to ealiest one as the latest
                    ** used FMAP may not be able to host all of the new
                    ** FMAPs
                    */
                    emptyfmap_pageno = ufmap_pageno;
                    empty_fmap = used_fmap;
                    emptyfmap_sequence = ufmap_sequence;
                    emptyfmap_firstbit = ufmap_firstbit;
                    emptyfmap_lastbit = ufmap_lastbit;
                    emptyfmap_hw_mark = ufmap_hw_mark;
                }

                status = unfix_map(r, &ufmap, &ufmap_lock, (DM1P_FHDR **)NULL,
                                    (DM1P_LOCK *)NULL, dberr);
                if (status != E_DB_OK)
                    break;
                used_fmap--;
            }
            if (status != E_DB_OK)
                break;

            if((used_fmap < 0) && empty_fmap)
            {
                /* The first FMAP with space is unused, unformatted */
                first_used_page = emptyfmap_sequence * fseg;
                first_page = first_used_page;
                last_used_page = first_used_page + fmap_needed - 1;

                /* Re-fix emptyfmap_pageno as we'll be adding
                ** new FMAPs to it's pages.
                */
	        emptyfmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
                status = fix_map(r, emptyfmap_pageno, &emptyfmap,
                                &emptyfmap_lock, &fhdr, fhdr_lock,
                                dberr);
                if (status != E_DB_OK)
                    break;
            } 
            first_page = fhdr_pages;
        }
        else
        {
            /* We don't have empty unused FMAPs */
            if (fmap_hw_mark < fmap_lastbit)
            {
	        /*
	        ** There are allocated but unused (and unformatted) pages 
	        ** on the last fmap which may be used for any new FMAPs
	        ** Note that any remaining unused pages have already been 
	        ** marked free; Thus, first_free_page is the first newly 
	        ** allocated page. (B77416)
	        */
	        first_page = (fmap_sequence * fseg) + fmap_hw_mark + 1;
                first_used_page = first_page;
                last_used_page  = first_used_page + fmap_needed - 1;
            }
            else
            {
	        /* 
	        ** Any new FMAP will reside on newly allocated pages
                ** unless the current FMAP is empty and unused.
	        */
	        first_page = fhdr_pages;
                first_used_page = fhdr_pages;
                last_used_page = fhdr_pages + fmap_needed - 1;    
            }
        }

        first_free_page = max (fhdr_pages, last_used_page + 1);
	last_free_page  = new_physical_pages - 1;


	/*
	** Pre-calculate if we will be updating the FMAP or Free/Highwater
	** hints on the FHDR page so we can log those actions.
	*/
	if (first_used_page <= last_used_page &&
	    fhdr_hwmap < (last_used_page / fseg))
	{
	    hwmap_update = TRUE;
	}

	/*
	** first_free_page is a page number and DM1P_FSEG is the total 
	** number of pages we can map to a page ; hence the calculation
	** (first_free_page / DM1P_FSEG) gives us the sequence number
	** of the FMAP page that first_free_page resides on.  If this is on
	** a new FMAP yet to be created then we won't be updating the
	** free hint i.e. the test against fmap->fmap_sequence should
	** be '<=' and not '>='
	*/
	fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, fmap_sequence);

	if (first_free_page / fseg <= fmap_sequence &&
	    (VPS_TEST_FREE_HINT_MACRO(pgtype, fhdr_map)) == 0)
	{
	    freehint_update = TRUE;
	}

	if (first_page / fseg == fmap_sequence)
	    fmap_update = TRUE;

	/*
	** Log the extend operation.
	** The log record's LSN is written to the FMAP and FHDR pages below.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    /*
	    ** Start a Mini Transaction for the Extend operation (even if we
	    ** are already in one). Remember if we are already in one so we can
	    ** restore RCB fields later.
	    */
	    /* Remember whether we are in a mini already */
	    in_mini = (rcb->rcb_state & RCB_IN_MINI_TRAN);

	    /*
	    ** Logfile space for BM and EM reserved in dm0l_bm.
	    */
	    status = dm0l_bm(r, &bmini_lsn, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** We only need BIs if the database is undergoing backup and
	    ** logging is enabled.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
		/*
		** Online Backup Protocols: Check if BIs must be logged before update.
		*/
		status = dm0p_before_image_check(r, (DMPP_PAGE *) fhdr, dberr);
		if (status == E_DB_OK)
		    status = dm0p_before_image_check(r, (DMPP_PAGE *) fmap, dberr);
                if (status == E_DB_OK && ufmap != NULL)
                    status = dm0p_before_image_check(r, (DMPP_PAGE *) ufmap, dberr);
                if (status == E_DB_OK && emptyfmap != NULL)
                    status = dm0p_before_image_check(r, (DMPP_PAGE *) emptyfmap, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Find physical location ID's for log record below.
	    */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fhdr_pageno);
	    fhdr_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fmap_pageno);
	    fmap_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	    /* 
	    ** Build an array of loc_config_id information used 
	    ** during rollforward for partial recovery.
	    */
	    for (i = 0; i < tbio->tbio_loc_count; i++)
	    {
	 	loc_cnf[i] = tbio->tbio_location_array[i].loc_config_id;
	    }

	    /* Reserve space for extend and its clr */
	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

            /* Same logic as that which might actually force the page */
            if (dm0p_set_lg_force(tbio,rcb))
            {
                lg_force_flag = LG_RS_FORCE;
            } else
            {
                dm0l_flag |= DM0L_FASTCOMMIT;
                lg_force_flag = 0;
            }

	    cl_status = LGreserve(lg_force_flag, r->rcb_log_id, 2, 
		sizeof(DM0L_EXTEND) * 2, &sys_err);
	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM92DC_DM1P_EXTEND);
		status = E_DB_ERROR;
		break;
	    }

	    /* 
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    status = dm0l_extend(rcb->rcb_log_id, dm0l_flag,
		&t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		pgtype, pgsize,
		t->tcb_rel.relloccount, loc_cnf,
		fhdr_pageno,
		fmap_pageno,
		fmap_sequence,
		fhdr_loc_config_id, fmap_loc_config_id, 
		first_used_page, last_used_page,
		first_free_page, last_free_page, 
		old_logical_pages, new_physical_pages, 
		hwmap_update, freehint_update, fmap_update,
		(LG_LSN *)NULL, &lsn, dberr);
	    if (status != E_DB_OK)
		break;

            if(ufmap)
            {
                loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, ufmap_pageno);
                ufmap_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

                cl_status = LGreserve(lg_force_flag, r->rcb_log_id, 2,
                    sizeof(DM0L_UFMAP) * 2, &sys_err);
                if (cl_status != OK)
                {
                    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
                                (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
                    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
                                ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
                                err_code, 1, 0, r->rcb_log_id);
                    SETDBERR(dberr, 0, E_DM92DC_DM1P_EXTEND);
                    status = E_DB_ERROR;
                    break;
                }

                status = dm0l_ufmap(rcb->rcb_log_id, dm0l_flag,
                    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
                    pgtype, pgsize,
                    t->tcb_rel.relloccount,
                    fhdr_pageno,
                    ufmap_pageno,
                    ufmap_sequence,
                    DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, ufmap),
                    fhdr_loc_config_id, ufmap_loc_config_id,
                    first_used_page, last_used_page,
                    (LG_LSN *)NULL, &lsn, dberr);
                if (status != E_DB_OK)
                    break;
            }

            if(emptyfmap)
            {
                loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, emptyfmap_pageno);
                emptyfmap_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

                cl_status = LGreserve(lg_force_flag, r->rcb_log_id, 2,
                    sizeof(DM0L_UFMAP) * 2, &sys_err);
                if (cl_status != OK)
                {
                    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
                                (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
                    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
                                ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
                                err_code, 1, 0, r->rcb_log_id);
                    SETDBERR(dberr, 0, E_DM92DC_DM1P_EXTEND);
                    status = E_DB_ERROR;
                    break;
                }

                status = dm0l_ufmap(rcb->rcb_log_id, dm0l_flag,
                    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
                    pgtype, pgsize,
                    t->tcb_rel.relloccount,
                    fhdr_pageno,
                    emptyfmap_pageno,
                    emptyfmap_sequence,
                    DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, emptyfmap),
                    fhdr_loc_config_id, emptyfmap_loc_config_id,
                    first_used_page, last_used_page,
                    (LG_LSN *)NULL, &lsn, dberr);
                if (status != E_DB_OK)
                    break;
            }
	}

	/* Mutex both the FHDR and the FMAP */
	dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fhdr_lock->pinfo);
	dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);
        if(ufmap)
        {
            dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &ufmap_lock.pinfo);
        }
        if(emptyfmap)
        {
            dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &emptyfmap_lock.pinfo);
        }

	/*
	** Update the FMAP(s) to reflect the newly added free pages and
	** the newly added used pages if there are any new fmaps being added.
	**
	** Only those new pages which fall within the range of the fmap
	** are recorded within.  Note that it is possible that NONE of
	** the new pages fall within the range of the last fmap (if the
	** previous allocation already completely filled the last fmap).
	*/

        if(ufmap)
        {
            if((last_used_page / fseg) > ufmap_sequence)
            {
                VPS_CLEAR_FREE_HINT_MACRO(pgtype, fhdr_map);
                ufmap_firstbit = fseg;
		fhdr_hwmap++; 
		DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, fhdr_hwmap);
            }
            else
            {
                ufmap_firstbit = (last_used_page % fseg) + 1;
            }
            DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, ufmap, ufmap_firstbit);

            dm1p_fmused(ufmap, first_used_page, last_used_page, pgtype, pgsize);
	    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, ufmap, DMPP_MODIFY);

            if (r->rcb_logging & RCB_LOGGING)
                DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, ufmap, lsn);
        }

        if(emptyfmap)
        {
            emptyfmap_firstbit = (last_used_page % fseg) + 1;
            DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, emptyfmap, emptyfmap_firstbit);

            dm1p_fmused(emptyfmap, first_used_page, last_used_page, pgtype, pgsize);
	    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, emptyfmap, DMPP_MODIFY);

            if (r->rcb_logging & RCB_LOGGING)
                DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, emptyfmap, lsn);
        }
       
	/*
	** If new FMAP page numbers are added and fall within the range
	** of this fmap then update the fmap to show that they are used
	** and update the FMAP/FHDR highwater marks.
	*/
        if (first_used_page <= last_used_page &&
	   (first_used_page / fseg) == fmap_sequence)
	{
	    if (fmap_firstbit == fmap_hw_mark + 1)
	    {
		/*
		** There aren't any free pages other than any allocated but 
		** unused pages on this fmap.  We have to do this test here
		** before the highwater mark gets updated by dm1p_fmused().
		** 
		** If the addition of new FMAP page(s) would use up all the
		** free pages on this fmap then update the first free bit and 
		** clear the free hint;  Otherwise, just update the first 
		** free bit. (B77416)
		*/ 

		if((last_used_page / fseg) > fmap_sequence)
		{
		    VPS_CLEAR_FREE_HINT_MACRO(pgtype, fhdr_map);
		    fmap_firstbit = fseg;
		}
		else
                {
                    fmap_firstbit = (last_used_page % fseg) + 1;
                }
		DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, fmap_firstbit);
	    }
	    
	    dm1p_fmused(fmap, first_used_page, last_used_page, pgtype, pgsize);

	    if (fhdr_hwmap <= fmap_sequence)
	    {
		fhdr_hwmap = fmap_sequence + 1;
		DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, fhdr_hwmap);
	    }

	    if (r->rcb_logging & RCB_LOGGING)
		DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, fmap, lsn);
	}

	/*
	** If any of the newly allocated free pages fall within the range
	** of the current fmap, then mark the pages as free in this map
	** and update any hits on the FHDR page.
	*/
	if (first_free_page / fseg <= fmap_sequence)
	{
	    dm1p_fmfree(fmap, first_free_page, last_free_page, pgtype, pgsize);
	    
	    if (VPS_TEST_FREE_HINT_MACRO(pgtype, fhdr_map) == 0)
		VPS_SET_FREE_HINT_MACRO(pgtype,  fhdr_map);

	    if (r->rcb_logging & RCB_LOGGING)
		DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, fmap, lsn);
	}

	/* Unmutex and unfix the FMAP, leave FHDR fixed */
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);
        fmap_status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL,
				(DM1P_LOCK *)NULL, dberr);
        if(fmap_status == OK && ufmap)
        {
            dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &ufmap_lock.pinfo);
            fmap_status = unfix_map(r, &ufmap, &ufmap_lock, (DM1P_FHDR **)NULL,
                                (DM1P_LOCK *)NULL, dberr);
        }

        if(fmap_status == OK && emptyfmap)
        {
            dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &emptyfmap_lock.pinfo);
            fmap_status = unfix_map(r, &emptyfmap, &emptyfmap_lock,
                                (DM1P_FHDR **)NULL, (DM1P_LOCK *)NULL, dberr); 
        }

	/*
	** Update the FHDR to reflect the newly added free pages.
	** but leave it mutexed until we finish adding new FMAPs,
	** if any.
	*/
	DM1P_VPT_SET_FHDR_PAGES_MACRO(pgtype, fhdr, new_physical_pages);
	DM1P_VPT_INCR_FHDR_EXTEND_CNT_MACRO(pgtype, fhdr);
	DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, DMPP_MODIFY);
	if (r->rcb_logging & RCB_LOGGING)
	    DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(pgtype, fhdr, lsn);

        fmap_pageno = first_used_page;

        /* At this point we only need to know the number of NEW fmaps
        ** We no longer care how they are split over existing FMAPs.
        */
	while ((fmap_needed) && fmap_status == E_DB_OK)
	{
	    /*
	    ** Fix free page into the buffer manager for use as an FMAP.
	    **
	    ** It's possible that the new FMAP's fmap_pageno collides with
	    ** some other page in the process of being allocated; if that's
	    ** the case, DM004C_LOCK_RESOURCE_BUSY will be returned and
	    ** we'll have to abandon this FMAP and retry the extend.
	    */
	    fmap_lock.fix_lock_mode = (DM1P_FOR_SCRATCH | FMAP_UPDATE_LKMODE);
	    fmap_status = fix_map(r, fmap_pageno, &fmap, &fmap_lock,
				    &fhdr, fhdr_lock, dberr);
	    if (fmap_status != E_DB_OK)
		break;

	    /*
	    ** Log the addition of the new FMAP
	    */
	    if (r->rcb_logging & RCB_LOGGING)
	    {
		/*
		** We only need BIs if the database is undergoing backup and
		** logging is enabled.
		*/
		if (d->dcb_status & DCB_S_BACKUP)
		{
		    /*
		    ** Online Backup Protocols: Check if BIs must be logged first.
		    */
		    fmap_status = dm0p_before_image_check(r, (DMPP_PAGE *) fhdr, dberr);
		    if (fmap_status == E_DB_OK)
			fmap_status = dm0p_before_image_check(r, (DMPP_PAGE *) fmap, dberr);
		    if (fmap_status != E_DB_OK)
			break;
		}

		/*
		** Find physical location ID's for log record below.
		*/
		loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fmap_pageno);
		fmap_loc_config_id =
				tbio->tbio_location_array[loc_id].loc_config_id;

		dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

		if ( r->rcb_state & RCB_IN_MINI_TRAN )
		    dm0l_flag |= DM0L_MINI;

		/* 
		** Reserve logfile space for fmap and its clr.  Early release
		** of lock means that update must be forced. 
		*/
                /* Same logic as that which might actually force the page */
                if (dm0p_set_lg_force(tbio,rcb))
                {
                    lg_force_flag = LG_RS_FORCE;
                } else
                {
                    dm0l_flag |= DM0L_FASTCOMMIT;
                    lg_force_flag = 0;
                }

		cl_status = LGreserve(lg_force_flag, r->rcb_log_id, 2, 
		    sizeof(DM0L_FMAP) * 2, &sys_err);
                if (cl_status != OK)
		{
		    (VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
				ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
				err_code, 1, 0, r->rcb_log_id);
		    SETDBERR(dberr, 0, E_DM92DC_DM1P_EXTEND);
		    fmap_status = E_DB_ERROR;
		    break;
		}

		/* 
		** We use temporary physical locks for catalogs and extension
		** tables. The same protocol must be observed during recovery.
		*/
		if ( NeedPhysLock(r) )
		    dm0l_flag |= DM0L_PHYS_LOCK;
                else if (row_locking(r))
                    dm0l_flag |= DM0L_ROW_LOCK;
		else if ( crow_locking(r) )
		    dm0l_flag |= DM0L_CROW_LOCK;

		fmap_status = dm0l_fmap(rcb->rcb_log_id, dm0l_flag,
		    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    pgtype, pgsize,
		    t->tcb_rel.relloccount,
		    fhdr_pageno,
		    fmap_pageno,
		    fhdr_count,
		    DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap),
		    fhdr_loc_config_id, fmap_loc_config_id, 
		    first_used_page, last_used_page,
		    first_free_page, last_free_page, 
		    (LG_LSN *)NULL, &lsn, dberr);
		if (fmap_status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Mutex and initialize the new FMAP page.
	    */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);

	    dm1p_fminit(fmap, pgtype, pgsize);

	    DM1P_VPT_SET_FMAP_SEQUENCE_MACRO(pgtype, fmap, fhdr_count);
	    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);
	    if (r->rcb_logging & RCB_LOGGING)
		DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, fmap, lsn);

	    dm1p_fmfree(fmap, first_free_page, last_free_page, pgtype, pgsize);

	    /*
	    ** If some of the new FMAP page numbers fall within the range
	    ** of this fmap then update the fmap to show that they are used
	    ** and update the FMAP/FHDR highwater marks.
	    */
	    if (last_used_page / fseg >= fhdr_count)
	    {
		dm1p_fmused(fmap, first_used_page, last_used_page, pgtype, pgsize);
		
		if (fhdr_hwmap <= fhdr_count)
		{
		    fhdr_hwmap = fhdr_count + 1;
		    DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, fhdr_hwmap);
		}
	    }
	    /* Unmutex and unfix the FMAP, leave the FHDR fixed */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);
	    fmap_status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL,
				    (DM1P_LOCK *)NULL, dberr);


	    /*
	    ** Update the FHDR to show the addition of the new FMAP.
	    ** Note that the FHDR is still mutexed.
	    */
	    fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, fhdr_count);
	    VPS_MAP_FROM_NUMBER_MACRO(pgtype, fhdr_map, fmap_pageno);
	    VPS_SET_FREE_HINT_MACRO(pgtype, fhdr_map);
	    DM1P_VPT_SET_FHDR_COUNT_MACRO(pgtype, fhdr, ++fhdr_count);

	    /*
	    ** Go on to next FMAP page (if any).
	    */
	    fmap_pageno++;
            fmap_needed--;
	    r->rcb_page_adds++;
	}

	/* Unmutex the FHDR */
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fhdr_lock->pinfo);

	if ((status = fmap_status) != E_DB_OK)
	    break;

	/*
	** End the Mini Transaction begun for the extend operation.
	** This serves to Commit the Extend Operation.  After this it
	** will never be backed out. If we were already in a mini-transaction
	** when we entered the extend, reset the rcb to say so.
	*/
	if (rcb->rcb_logging & RCB_LOGGING)
	{
	    status = dm0l_em(r, &bmini_lsn, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	    if (in_mini)
		rcb->rcb_state |= RCB_IN_MINI_TRAN;
	}

	return (E_DB_OK);

    } while (FALSE);

    /*
    ** Error Handling : Release any locally fixed pages.
    */

    if (ufmap)
    {
	status = unfix_map(r, &ufmap, &ufmap_lock, (DM1P_FHDR **)NULL,
				    (DM1P_LOCK *)NULL, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (emptyfmap)
    {
        status = unfix_map(r, &emptyfmap, &emptyfmap_lock, (DM1P_FHDR **)NULL,
                                    (DM1P_LOCK *)NULL, &local_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
        }
    }

    if (fmap)
    {
	status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL,
				    (DM1P_LOCK *)NULL, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    dm1p_log_error(E_DM92DC_DM1P_EXTEND, dberr, 
		   &t->tcb_rel.relid,
		   &t->tcb_dcb_ptr->dcb_name);
    return (E_DB_ERROR);
}

/*{
** Name: dm1p_fminit - Initialize an empty FMAP page.
**
** Description:
**	This routine is called to initialize an empty FMAP page.
**
**
**	fmap				Pointer to FMAP.
**
** Outputs:
**					None.
**                                      
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	14-dec-92 (rogerk)
**	    Changed name from init_fmap and made external so it can be called
**	    by recovery code.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Add page_size parameter to this routine.
**		Removed the initialization of fmap_spare[2 thru 5].
**	20-April-1999 (thaal01) b91567
**	    Use macro DM1P_INIT_FMAP_PAGE_STAT_MACRO, instead  of 
**	    DM1P_SET_FMAP_PAGE_STAT_MACRO. This ensures that its
**	    previous identity as a DATA page is erased.
*/
VOID
dm1p_fminit(
    DM1P_FMAP   *fmap,
    i4		pgtype,
    DM_PAGESIZE page_size)
{
    i4	fseg = DM1P_FSEG_MACRO(pgtype, page_size);
    
    MEfill(DM1P_SIZEOF_FMAP_BITMAP_MACRO(pgtype, page_size), '\0', 
	DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap));
    DM1P_VPT_INIT_FMAP_PAGE_STAT_MACRO(pgtype, fmap, (DMPP_FMAP | DMPP_MODIFY));
    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, fseg);
    DM1P_VPT_SET_FMAP_HW_MARK_MACRO(pgtype, fmap, fseg);
    DM1P_VPT_SET_FMAP_LASTBIT_MACRO(pgtype, fmap, fseg);
    DM1P_VPT_SET_FMAP_SPARE_MACRO(pgtype, fmap, 0, 0);
    DM1P_VPT_SET_FMAP_SPARE_MACRO(pgtype, fmap, 1, 0);
}

/*{
** Name: test_free - Test page as used in bitmap.
**
** Description:
**	This routine is called with the FHDR fixed.  An FMAP pointer can
**	be passed in to optimization consecutive calls (this mechanism is
**	used by the convert and rebuild routines.)
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	free_header			Pointer to FHDR.
**	map_context			Pointer to FMAP Pointer or NULL.
**	pageno				Page number to mark as free.
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK			Pageno is marked as free.
**	    E_DB_INFO			Pageno is marked as used.
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
*/
static DB_STATUS
test_free(
    DMP_RCB	*rcb,
    DM1P_FHDR   *free_header,
    DM1P_LOCK	*fhdr_lock,
    DM1P_FMAP   **fmap_context,
    DM1P_LOCK   *fmap_lock,
    DM_PAGENO	pageno,
    DB_ERROR	*dberr )
{
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DM1P_FHDR	*fhdr = free_header;
    DM1P_FMAP	*fmap_local = 0;
    DM1P_FMAP	**fmap = fmap_context;
    DM1P_FMAP	*fm;
    i4	fmap_block;
    i4	fmap_index;
    i4	fmap_bit;
    DB_STATUS	status;
    DB_STATUS	ret_status;

    i4	pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    i4	fseg = DM1P_FSEG_MACRO(pgtype, pgsize);

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(50))
	TRdisplay("DM1P: test page %d used in table %~t\n",
    		pageno,
    		sizeof(t->tcb_rel.relid),
    		&t->tcb_rel.relid);

    /*	Compute FMAP index in FHDR and bitmap index in FMAP. */

    fmap_index = pageno / fseg;
    fmap_bit = pageno % fseg;

    /*  Check that the current FMAP block is correct. */

    if (fmap == 0)
	fmap = &fmap_local;
    fm = *fmap;
    fmap_block = VPS_NUMBER_FROM_MAP_MACRO(pgtype,
    	DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, fmap_index));
    if (fmap_block == 0)
	return (E_DB_OK);

    if (fm == 0 || fmap_block != 
	DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(pgtype, fm))
    {
	/*	Release current FMAP block. */
	
	if (fm)
	{
	    status = unfix_map(rcb, fmap, fmap_lock, (DM1P_FHDR **)NULL,
				(DM1P_LOCK *)NULL, dberr);
	    if (status != E_DB_OK)
		return (status);
	}

	/*	Read required FMAP block. */

	fmap_lock->fix_lock_mode = FMAP_UPDATE_LKMODE;
	status = fix_map(rcb, fmap_block, 
				fmap, fmap_lock,
				&fhdr, fhdr_lock, dberr);
	if (status != E_DB_OK)
	    return (status);
	fm = *fmap;
    }

    /*	Check free bit in bitmap.	*/

    ret_status = E_DB_INFO;
    if (BITMAP_TST_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fm), fmap_bit))
	ret_status = E_DB_OK;

    if (fmap_context == 0)
    {
	/*  Unfix the locally fixed FMAP block. */

	status = unfix_map(rcb, fmap, fmap_lock, (DM1P_FHDR **)NULL,
			    (DM1P_LOCK *)NULL, dberr);
	if (status != E_DB_OK)
	    return (status);
    }
    return (ret_status);
}

/*{
** Name: find_free - Find a free page in the bitmap.
**
** Description:
**	This routine is called with the FHDR fixed.  The FHDR is searched
**	for an map page with a HINT set.  If the page is empty the hint
**	will be reset.  If no FMAP pages contain any free pages then the
**	file is extended.  The caller can pass in a hint that is used
**	to attempt an allocation near the requested area.  The page pointer
**	returned is fixed for writing and partially initialized, it has
**	not been read from disk.
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	free_header			Pointer to FHDR.
**	fhdr_lock			Pointer to DM1P_LOCK containing
**					fix and lock info for FHDR.
**	flag				Flag to fix page as PHYSICAL.
**
** Outputs:
**	page				Pointer to page pointer.
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN			No free pages available.
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
*/
static DB_STATUS
find_free( 
    DMP_RCB	*rcb,
    DM1P_FHDR	**free_header,
    DM1P_LOCK	*fhdr_lock,
    i4	flag,
    DMP_PINFO	*pinfo,
    DB_ERROR	*dberr )
{
    DM1P_FHDR	*fhdr = *free_header;
    DB_STATUS	status;
    i4		pgsize = rcb->rcb_tcb_ptr->tcb_rel.relpgsize;
    i4          pgtype = rcb->rcb_tcb_ptr->tcb_rel.relpgtype;
    i4	fhdr_count = DM1P_VPT_GET_FHDR_COUNT_MACRO(pgtype, fhdr); 
    i4	hint_index;

    CLRDBERR(dberr);
	
    /*	Scan all maps for a free page. */

    for (hint_index = 0; hint_index < fhdr_count; hint_index++)
    {
	/*  Ignore pages without hints set. */

	if (VPS_TEST_FREE_HINT_MACRO(pgtype, 
		DM1P_VPT_GET_FHDR_MAP_MACRO(pgtype, fhdr, hint_index)) == 0)
	    continue;

	/*  Attempt an allocation from this map page. */

	status = scan_map(rcb, free_header, fhdr_lock, 
				&hint_index, flag, pinfo, dberr);
	if (status == E_DB_OK || status != E_DB_WARN)
	    return (status);
    }

    /*	No free pages available. */

    if (DMZ_AM_MACRO(50))
	TRdisplay("DM1P: find_free failed in table %~t\n",
    	    sizeof(rcb->rcb_tcb_ptr->tcb_rel.relid),
    	    &rcb->rcb_tcb_ptr->tcb_rel.relid);

    return (E_DB_WARN);
}

/*{
** Name: scan_map - Find a free page in the bitmap.
**
** Description:
**	This routine is called with the FHDR fixed.  The FMAP corresponding
**	to the map_index past in is fixed.  The bitmap is searched for
**	a free page.  When a page is found the bit is free bit is turned off
**	and the nextbit hint on the page is incremented.  If the page is
**	empty, then the FHINT in the FHDR is reset.  The FMAP page is unfixed
**	before returning.
**
**	The fmap_firstbit field is increment past the page that was just
**	allocated, it is not set to the next free page.  It is also not
**	set to a page that was skipped because it was locked because if
**	that page is deallocated the fmap_firstbit field will be reset.
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	free_header			Pointer to FHDR.
**	fhdr_lock			Pointer to DM1P_LOCK containing
**					fix and lock info for FHDR.
**	map_index			Pointer to FMAP index into FHDR.
**	flag				Flag to get PHYSICAL lock on page.
**
** Outputs:
**	page				Pointer to returned page pointer.
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK			Allocation successful.
**	    E_DB_WARN			No free page found.
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	rcb_page_adds is incremented.
**
** History:
**      02-feb-90 (Derek)
**          Created.
**	20-Nov-1991 (rmuth)
**	    Added flag DM1P_MINI_TRAN, added comments explaining its
**	    action. If this flag is set we check to make sure that the
**	    page we are going to allocate has not been dealloacted by this
**	    transaction as this causes problems during UNDO/REDO 
**	    processing.
**      08-jun-1992 (kwatts)
**          6.5 MPF change. Replaced call to dm1c_format with accessor call.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Updated with new recovery protocols:
**	      - Each page updated as part of the allocate operation (fhdr,fmap
**		and free page) must be mutexed before the allocate log record
**		is written, and held until the page update is complete.
**	      - The buffer manager no longer mutexes fhdr and fmap pages for
**		us, and does not mark newly allocated pages as MODIFIED if
**		they are formatted as part of a SCRATCH fix.
**	      - The FHDR freebit hints are no longer updated if a scan_map 
**		operation fails to find a free bit.  We now turn off the freebit
**		hints if we successfully allocate the last page in an fmap.
**	      - Removed checksums on fhdr and fmap pages - checksums are now
**		written by the buffer manager on all ingres pages.
**	      - Removed NONREDO and NEEDBI options.  System catalog allocates
**		are now redone like all other allocates and Before Images are
**		no longer used for transaction recovery.  Also removed necessity
**		to force system catalog pages after updates.
**	      - Changed setting of firstbit after allocating a page to only
**		be done if there were no free pages earlier than the one
**		we allocated that happened to be busy at the time.  Otherwise
**		we may lose track of free pages.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-July-1993 (rmuth)
**	    Code was using fmap_firstbit to determine if we need to set
**	    fmap_hw_mark. This is incorrect should be using the actual 
**	    bit allocated as we do not always set fmap_firstbit when extend
**	    the end-of-table, for example busy page skipping.
**	    Missing setting fmap_hw_mk
**	    Incorrectly 
**	26-jul-1993 (rogerk)
**	    Change generation of Before Image log records during Online Backup.
**	    All page modifiers must now call dm0p_before_image_check before
**	    changing the page to implement online backup protocols.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() call.
**	18-oct-1993 (jnash)
**	    Avoid dm0p_before_image_check() call if we do not update the fhdr.
**	24-jan-1994 (gmanning)
**	    Change unsigned i4 to u_i4 to compile in NT environment.
**	30-may-1996 (pchang)
**          Added DM0P_NOESCALATE action when fixing free pages found in the
**	    free maps.  This is necessary because we currently have the FHDR 
**	    fixed and lock escalation here may lead to unrecoverable deadlock 
**	    involving the table-level lock and the FHDR. (B76463)
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_format.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**		Change DM1P_MBIT reference to DM1P_MBIT_MACRO.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Set DM0L_ROW_LOCK flag if row locking is enabled.
**      12-dec-96 (dilma04)
**          If row locking, do not set DM0P_PHYSICAL flag when fixing a page 
**          for CREATE. This fixes bug 79611.
**      06-jan-97 (dilma04)
**          Change comment for bug fix 79611 because we do not request intent
**          page locks in physical mode by default anymore.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      21-apr-98 (stial01)
**          scan_map() Set DM0L_PHYS_LOCK if extension table (B90677)
**	19-Aug-1998 (jenjo02)
**	    When converting rowlocking PHYSICAL pagelock from X to logical IX, 
**	    it must be done in two steps: 1st, explicitly convert PHYSICAL X to IX,
**	    2nd, implicitly convert PHYSICAL IX to logical.
**	01-Sep-1998 (jenjo02)
**	    Well, testing after the earlier concurrency effort uncovered a gaping
**	    hole. Thread A could pick free page 1 and attempt to fix it, but be
**	    put to sleep by the OS. While asleep, thread B also picks free page 1,
**	    fixes it, formats it, and ends its transaction. When thread A finally
**	    awakes, it still thinks that free page 1 is available and formats it.
**	    The solution is to pass a new dm0p flag, DM0P_CREATE_NOFMT, when 
**	    fixing the page. Instead of formatting the page, dm0p will mutex it
**	    and return the fixed page in that state, after which scan_map now
**	    retests the free bit for the page; if off (another thread stole the
**	    page), the page is unmutexed and unfixed, and another candidate 
**	    found. This also eliminates the need to use lock_values with FMAPs.
**	23-Sep-1998 (jenjo02)
**	    Whoops! If we must write a BI for the freshly allocated page, be
**	    sure to format it first!
**      22-sep-98 (stial01)
**          scan_map() If row locking lock requested on page, don't do the
**          downgrade and convert here. The caller will after the mini
**          transaction completes (B92908, B92909)
**	02-Oct-1998 (jenjo02)
**	    In scan_map(), if FHDR must be relocked, recheck the free bit to
**	    make sure the allocated page has not been stolen.
**	21-Jan-1998 (jenjo02)
**	    Avoid deadlock with CP by releasing allocated page's mutex before
**	    calling LGreserve().
**	24-May-2001 (thaju02)
**	    If heap table, do not specify DM0P_NOWAIT. (b104783)
**	28-Dec-2007 (kibro01) b119663
**	    Remove the LG_RS_FORCE flag, since it isn't needed in all
**	    circumstances.  Apply the same conditions that take place when
**	    rollback or recovery is performed.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Use NeedPhysLock(r) to determine physical page lock needs.
**	    If DM1P_PHYSICAL is asserted (index pages, e.g.) when
**	    crow_locking, get the page lock.
*/
static DB_STATUS
scan_map(
    DMP_RCB	*r,
    DM1P_FHDR   **free_header,
    DM1P_LOCK	*fhdr_lock,
    i4	*map_index,
    i4	flag,
    DMP_PINFO	*pinfo,
    DB_ERROR	*dberr )
{
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DM1P_FHDR	    *fhdr = *free_header;
    DM1P_FMAP	    *fmap = NULL;
    LG_LSN	    lsn;
    i4	    i;
    i4	    last_bit_index;
    i4	    bitmap_index;
    i4	    bit_index;
    i4	    freebit;
    i4	    pageno;
    i4	    dm0p_flags;
    i4	    dm0l_flag;
    i4	    fhdr_loc_config_id;
    i4	    fmap_loc_config_id;
    i4	    free_loc_config_id;
    i4	    loc_id;
    DB_STATUS	    status;
    DB_STATUS	    local_status;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    i4	    local_err_code;
    i4		    found_busy_page;
    i4		    last_free_bit;
    i4		    hwmap_update;
    i4		    freehint_update;
    i4		    fhdr_update;

    i4	    pgsize = t->tcb_rel.relpgsize;
    i4      pgtype = t->tcb_rel.relpgtype;
    i4		    mbits = DM1P_MBIT_MACRO(pgtype, pgsize);
    i4	    fseg = DM1P_FSEG_MACRO(pgtype, pgsize);

    u_char	    *fhdr_map;
    i4	    fhdr_hwmap;
    DM_PAGENO	    fhdr_pageno = t->tcb_rel.relfhdr;

    u_i4	    *fmap_bitmap;
    i4	    fmap_firstbit;
    i4	    fmap_lastbit;
    i4	    fmap_hw_mark;
    i4	    fmap_sequence;
    DM_PAGENO	    fmap_pageno;
    
    u_i4	    map;
    DB_STATUS	    fhdr_status;
    DB_STATUS	    fmap_status;

    DM1P_LOCK	    fmap_lock;
    LG_LSN          oldest_lsn; /* 1st lsn of the oldest active transaction */
    i4              len;
    u_i4	    page_low_tran;
    u_i2	    page_lg_id;
    i4		lg_force_flag;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

#ifdef DEBUG_SCAN_MAP
    CS_SID	sid;
    CSget_sid(&sid);
#endif

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fmap_lock.lock_id.lk_unique = 0;

    pinfo->page = (DMPP_PAGE*)NULL;

    fhdr_map = DM1P_VPT_ADDR_FHDR_MAP_MACRO(pgtype, fhdr, *map_index);

    fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(pgtype, fhdr_map);

    /*
    ** Fix the FMAP for UPDATE, start with a read lock to encourage 
    ** concurrency.
    **
    ** FHDR has been locked for either read or update.
    ** If locked for read and we determine that the FHDR must be updated,
    ** we'll relock it in a non-deadlocking way.
    */
    if ((t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_MULTIPLE ||
	  t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_DMCM) &&
	  (t->tcb_table_io.tbio_cache_flags & TBIO_CACHE_READONLY_DB) == 0)
    {
	fmap_lock.fix_lock_mode = FMAP_UPDATE_LKMODE;
    }
    else
	fmap_lock.fix_lock_mode = FMAP_READ_LKMODE;

    status = fix_map(r, fmap_pageno, &fmap, &fmap_lock,
				&fhdr, fhdr_lock, dberr);

    while (status == E_DB_OK)
    {
	fhdr_hwmap = DM1P_VPT_GET_FHDR_HWMAP_MACRO(pgtype, fhdr);

	fmap_bitmap = DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap);
	fmap_firstbit = DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap);
	fmap_lastbit  = DM1P_VPT_GET_FMAP_LASTBIT_MACRO(pgtype, fmap);
	fmap_hw_mark  = DM1P_VPT_GET_FMAP_HW_MARK_MACRO(pgtype, fmap);
	fmap_sequence = DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(pgtype, fmap);

	/* If we haven't fixed a free page yet, do so now */
	if (pinfo->page == NULL)
	{
	    /*	Compute position to start bitmap scan. */
	    if (fmap_firstbit > fmap_lastbit)
	    {
		/* First free bit is beyond last bit, nothing here */
		status = E_DB_WARN;
		break;
	    }
	    else
	    {
		bitmap_index = fmap_firstbit / DM1P_BTSZ;
		bit_index = fmap_firstbit % DM1P_BTSZ;
		found_busy_page = FALSE;
	    }

	    for (; bitmap_index < mbits ; bitmap_index++)
	    {
		/*	Scan bitmap for lowest bit set. */
		map = fmap_bitmap[bitmap_index];

		for (map >>= bit_index; map ; bit_index++, map >>= 1)
		{
		    /*  Skip if bit is clear. */

		    if ((map & 1) == 0)
			continue;

		    /* Compute freebit = bit number of apparently free page */
		    freebit = (bitmap_index * DM1P_BTSZ) + bit_index;

		    /*	Compute page number of apparently free page */
		    pageno = *map_index * fseg + freebit;

		    /*
		    **  Fix page for CREATE_NOFMT.
		    **
		    **  This will get a lock on the page, but will not cause
		    **  the page to be formatted; instead, the page will be
		    **  mutexed on our behalf.
		    **
		    **  If MVCC, no lock will be taken on the page, but the
		    **  buffer will be pinned and mutexed.
		    **
		    **  Also, the buffer will be pinned. This keeps
		    **  MVCC threads from fixing the page while we're still
		    **  in the process of creating it, because we go through
		    **  several iterations of dm0pMutex/Unmutex during this
		    **  process. MVCC pages are not page locked, so BUF_MUTEX
		    **  by itself is worthless.
		    **
		    **  The pin is removed by dm0p_uncache_fix automatically,
		    **  or explicitly by dm0pUnpinBuf.
		    **
		    **  After fixing the page, we'll recheck its free bit to
		    **  make sure that another thread has not stolen this page
		    **  from us; if so, we'll unmutex and unfix it, then try
		    **  another page.
		    */
		    /* Note: DM0P_SCRATCH == DM0P_WRITE | DM0P_CREATE */
		    dm0p_flags = (DM0P_NOWAIT | DM0P_NOESCALATE | 
				  DM0P_WRITE  | DM0P_CREATE_NOFMT);

		    /*
		    ** If crow_locking and DM1P_PHYSICAL asserted,
		    ** get the physical page lock.
		    */
		    if ( !row_locking(r) && (!crow_locking(r) || flag & DM1P_PHYSICAL) )
			/*
			** DM0P_PHYSICAL flag when row locking would mean 
			** that the page being fixed is special (non-leaf 
			** index page, fhdr, fmap). This caused bug 79611.
			** In case of row locking, a physical page lock
			** will be requested because of the 
			** DM0P_CREATE_NOFMT flag.
			*/
			dm0p_flags |= DM0P_PHYSICAL;

#ifdef DEBUG_SCAN_MAP
		    if (fhdr_lock->fix_lock_mode != FHDR_UPDATE_LKMODE)
			TRdisplay("%@ %d fixing %d, bitmap_index %d bit_index %d bits %x firstbit %d\n",
				sid, pageno, bitmap_index, bit_index, 
				fmap_bitmap[bitmap_index], fmap_firstbit);
#endif

		    if (t->tcb_rel.relspec == TCB_HEAP)
			dm0p_flags &= ~DM0P_NOWAIT;

		    status = dm0p_fix_page(r, pageno, dm0p_flags,
					    pinfo, dberr);
		    if (status)
		    {
			if ((status == E_DB_ERROR) && 
			    (dberr->err_code == E_DM004C_LOCK_RESOURCE_BUSY))
			{
#ifdef DEBUG_SCAN_MAP
			    if (fhdr_lock->lock_value.lk_mode != FHDR_UPDATE_LKMODE)
				TRdisplay("%@ %d LKBUSY %d, bitmap_index %d bit_index %d bits %x firstbit %d\n",
					sid, pageno, bitmap_index, bit_index, 
					fmap_bitmap[bitmap_index], fmap_firstbit);
#endif
			    found_busy_page = TRUE;
			    status = E_DB_OK;
			}
		    }
		    else if ( t->tcb_extended
		    		&& r->rcb_lk_type != RCB_K_TABLE
				&& !crow_locking(r) )
		    {
                        /*
                        ** ETABS use physical page lock protocols
                        ** (note dm1p_putfree stores the transaction id
                        ** in the page header)
                        **
                        ** Can current transaction allocate this page?
                        ** Only if the transaction that deallocated the
                        ** page is not active.
                        ** .. or if the page was deallocated by the current
                        ** transaction, it can only be allocated again
                        ** by the current transaction if it is table locking.
                        ** Otherwise it has been seen that if the current
                        ** transaction does a rollback...
                        ** (which will rollback the DM0L_ALLOC & DM0L_DEALLOC)
                        ** another transaction may be successful allocating
                        ** the page in between:
                        ** Rollback DM0L_ALLOC
                        **                      DM0L_ALLOC by another xn
                        ** Rollback DM0L_DEALLOC
                        ** Rollback errors....
                        **
                        ** (because rollback DM0L_ALLOC does not restore
                        ** page tranid to the page header, and the lock on
                        ** the page is physical)
                        */
			if (DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, pinfo->page) == pageno)
			{
			    bool	xn_active = FALSE;

			    page_low_tran = 
				DMPP_VPT_GET_TRAN_ID_LOW_MACRO(pgtype, pinfo->page);
			    page_lg_id = 
				DMPP_VPT_GET_PAGE_LG_ID_MACRO(pgtype, pinfo->page);
			    if (page_low_tran == r->rcb_tran_id.db_low_tran)
			    {
				status = E_DB_WARN;
			    }
			    else
			    {
				/* Check if dealloc transaction is active */
				xn_active = IsTranActive(page_lg_id, page_low_tran); 
			    }
			    if (status != E_DB_OK || xn_active)
			    {
				found_busy_page = TRUE;

				dm0pUnmutex(tbio, 0, r->rcb_lk_id, pinfo);
				status = dm0p_unfix_page(r, DM0P_UNFIX,
						pinfo, dberr);
			    }
			}
		    }
		    else if (flag & DM1P_MINI_TRAN)
		    {
			/* Inside a mini-transaction and this page may not
			** been allocated on disc and the fmap does not
			** have the default value. Then we need to check
			** that we have not deallocated this page earlier 
			** during this transaction as mini-transactions are not
			** UNDOne, hence UNDO processing of the earlier action
			** which deallocated this page would overwrite this action.
			*/ 
			if ( DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, pinfo->page) == pageno &&
			     DMPP_VPT_GET_TRAN_ID_HIGH_MACRO(pgtype, 
				pinfo->page) == r->rcb_tran_id.db_high_tran &&
			     DMPP_VPT_GET_TRAN_ID_LOW_MACRO(pgtype,
				pinfo->page) == r->rcb_tran_id.db_low_tran )
			{
			    found_busy_page = TRUE;

			    dm0pUnmutex(tbio, 0, r->rcb_lk_id, pinfo);
			    status = dm0p_unfix_page(r, DM0P_UNFIX,
			    				pinfo, dberr);
			}
		    }
		    /* If we found a page or there's an error, quit the loop */
		    if (pinfo->page || status == E_DB_ERROR)
			break;

		    /* If a new firstbit has appeared, restart the search */ 
		    if (DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap) < fmap_firstbit)
			break;
		    
		    /* Refresh this map from the fmap */
		    map = fmap_bitmap[bitmap_index];
		    map >>= bit_index;
		}
		if (pinfo->page || status == E_DB_ERROR)
		    break;

		/* If a new firstbit has appeared, restart the search */ 
		if (DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(pgtype, fmap) < fmap_firstbit)
		    break;

		/* Didn't find a page. Continue with bit zero of next bitmap */
		bit_index = 0;
	    }
	    if (status == E_DB_ERROR)
		break;
	}

	/*
	** If no free page was found on this fmap then return a warning.
	** The caller will either find another fmap to scan or will have
	** to add new space to the table.
	**
	** (Note: an optimization could be made here to turn off the FHDR's
	** free space hint to signal that there is no free space on this fmap
	** if we know that we did not attempt to fix any seemingly free pages
	** above that turned out to be locked.  This would prevent us from
	** fruitlessly searching this fmap again the next time space is required.
	** It is not clear however, under what circumstances we will create
	** fmaps with no free pages without having correctly set the fhdr hints.)
	*/
	if (pinfo->page == NULL)
	{
	    if (DMZ_AM_MACRO(50))
		TRdisplay("DM1P: scan_map index %d in table %~t failed.\n",
			*map_index, sizeof(t->tcb_rel.relid), &t->tcb_rel.relid);

	    status = E_DB_WARN;
	    break;
	}

	/*
	** We've found a free page and fixed it.
	*/

	if (DMZ_AM_MACRO(50))
	{
	    TRdisplay("DM1P: scan_map index %d in table %~t pageno = %d\n",
		    *map_index, sizeof(t->tcb_rel.relid),
		    &t->tcb_rel.relid, pageno);
	}

	/*
	** We've found a free page.
	**
	** It's fixed for update, locked LK_X, and mutexed.
	**
	** Now convert the FMAP's read lock to update
	** and recheck that the page is still free.
	*/
	if (fmap_lock.lock_value.lk_mode != FMAP_UPDATE_LKMODE)
	{
	    fmap_lock.new_lock_mode = FMAP_UPDATE_LKMODE;

	    /* Unmutex the page before requesting lock */
	    dm0pUnmutex(tbio, (i4)DM0P_MINCONS_PG, r->rcb_lk_id, pinfo);

	    status = relock_pages(r, &fmap_lock, (DM1P_LOCK *)NULL, dberr);

	    /* Remutex the page */
	    dm0pMutex(tbio, (i4)DM0P_MINCONS_PG, r->rcb_lk_id, pinfo);

	    if (status != E_DB_ERROR)
	    {
		/* 
		** Recheck the free bit. It's possible that another thread
		** has allocated this page from underneath us. If the
		** page is no longer free, unfix it and start over.
		*/
		if ((fmap_bitmap[bitmap_index] & (1 << bit_index)) == 0)
		{
#ifdef DEBUG_SCAN_MAP
		    TRdisplay("%@ %d STOLEN (FMAP) %d, bitmap_index %d bit_index %d bits %x firstbit %d\n",
				sid, pageno, bitmap_index, bit_index, 
				fmap_bitmap[bitmap_index], fmap_firstbit);
#endif
		    /* Reduce the FMAP's lock to read for new free page search */
		    fmap_lock.new_lock_mode = FMAP_READ_LKMODE;


		    /* Unmutex the no longer "free" page */
		    dm0pUnmutex(tbio, DM0P_MINCONS_PG, 
		    			r->rcb_lk_id, pinfo);

		    fmap_status = relock_pages(r, &fmap_lock, (DM1P_LOCK *)NULL, dberr);

		    /* Unfix/UNLOCK the no longer "free" page */
		    status = unfix_nofmt_page(r, pinfo, pageno, dberr);

		    if (status == E_DB_OK)
			status = fmap_status;
		}
	    }
	    
	    /* 
	    ** While the FMAP was locked for read, it may have been
	    ** modified, so loop back up to reload the FMAP variables.
	    */
	    if (status == E_DB_ERROR)
		break;

	    status = E_DB_OK;
	    continue;
	}

	/*
	** We have found a free page and now have the FHDR, FMAP and new page
	** fixed.  Check to see if we have just allocated the last free page
	** on the current fmap.  If so, then we will need to update the free
	** space hints on the fhdr.
	**
	** If we happened across busy free pages or if the next page in the
	** map (after the one we are allocating) is also free, then don't
	** bother checking.
	*/
	if (( ! found_busy_page) && 
	    (fmap_bitmap[bitmap_index] == (1 << bit_index)))
	{
	    /*
	    ** Look through the map starting from the spot of the page just
	    ** allocated and look for other free pages.
	    */
	    last_bit_index = mbits - 1;
	    if (fmap_lastbit < fseg)
		last_bit_index = fmap_lastbit / DM1P_BTSZ;

	    last_free_bit = TRUE;
	    for (i = bitmap_index + 1; i <= last_bit_index; i++)
	    {
		if (fmap_bitmap[i])
		{
		    last_free_bit = FALSE;
		    break;
		}
	    }
	}
	else
	    last_free_bit = FALSE;

	/*
	** Determine if any FHDR information will be updated so we can
	** log the changes.
	*/
	if (*map_index >= fhdr_hwmap || last_free_bit)
	{
	    /*
	    ** FHDR update needed.
	    **
	    ** To do that, we must have the FHDR locked for update.
	    ** If we don't, relock the FHDR, but leave the
	    ** newly allocated page fixed.
	    ** 
	    ** To avoid deadlocks, pass the update-locked FMAP lock information;
	    ** if the FHDR update lock would have to wait, relock_pages() will
	    ** unlock, then relock, the FMAP.
	    */
	    if (fhdr_lock->lock_value.lk_mode != FHDR_UPDATE_LKMODE)
	    {
		fhdr_lock->new_lock_mode = FHDR_UPDATE_LKMODE;
		
		/* Unmutex the possible "free" page before requesting lock*/
		dm0pUnmutex(tbio, (i4)DM0P_MINCONS_PG, r->rcb_lk_id, pinfo);

		status = relock_pages(r, fhdr_lock, &fmap_lock, dberr);

		/* Remutex the page */
		dm0pMutex(tbio, (i4)DM0P_MINCONS_PG, r->rcb_lk_id, pinfo);

		/* 
		** Recheck the free bit. It's possible that another thread
		** has allocated this page from underneath us. If the
		** page is no longer free, unfix it and start over.
		*/
		if ((fmap_bitmap[bitmap_index] & (1 << bit_index)) == 0)
		{
#ifdef DEBUG_SCAN_MAP
		    TRdisplay("%@ %d STOLEN (FHDR) %d, bitmap_index %d bit_index %d bits %x firstbit %d\n",
				sid, pageno, bitmap_index, bit_index, 
				fmap_bitmap[bitmap_index], fmap_firstbit);
#endif
		    /* Reduce the FMAP's lock to read for new free page search */
		    fmap_lock.new_lock_mode = FMAP_READ_LKMODE;

		    /* Unmutex the no longer "free" page */
		    dm0pUnmutex(tbio, DM0P_MINCONS_PG,
		    			r->rcb_lk_id, pinfo);

		    fmap_status = relock_pages(r, &fmap_lock, (DM1P_LOCK *)NULL, dberr);

		    /* Unfix/UNLOCK the no longer "free" page */
		    status = unfix_nofmt_page(r, pinfo, pageno, dberr);

		    if (status == E_DB_OK)
			status = fmap_status;
		}

		if (status == E_DB_ERROR)
		    break;

		/* Loop back up to reload FHDR/FMAP variables, maybe get a new page */
		status = E_DB_OK;
		continue;
	    }

	    fhdr_update = TRUE;
		
	    if (*map_index >= fhdr_hwmap)
		hwmap_update = TRUE;
	    if (last_free_bit)
		freehint_update = TRUE;
	}
	else
	{
	    fhdr_update = FALSE;
	    hwmap_update = FALSE;
	    freehint_update = FALSE;
	}


	if (fhdr_update == FALSE)
	{
	    /*
	    ** Won't be updating the FHDR, so unfix it before
	    ** going on to update the FMAP and free page.
	    ** Caution is advised beyond this point; "fhdr"
	    ** cannot be referenced unless "fhdr_update" is TRUE.
	    */
	    fhdr_status = unfix_header(r, free_header, fhdr_lock,
				&fmap, &fmap_lock, dberr);
	    status = fhdr_status;
	    fhdr = (DM1P_FHDR *)NULL;
	}
	break;
    }

    while (status == E_DB_OK) 
    {
	/*
	** Set the page number in the newly allocated page to keep
	** dm0p_unmutex from choking on a consistency check.
	**
	** Also clear page_stat to zero, for the same reason.
	*/
	DMPP_VPT_SET_PAGE_PAGE_MACRO(pgtype, pinfo->page, pageno);
        DMPP_VPT_INIT_PAGE_STAT_MACRO(pgtype, pinfo->page, 0);
	
	/*
	** Log the update and write the allocate log record's LSN to the
	** updated pages.
	*/
	if (r->rcb_logging & RCB_LOGGING)
	{
	    /*
	    ** We only need BIs if the database is undergoing backup and
	    ** logging is enabled.
	    */
	    if (d->dcb_status & DCB_S_BACKUP)
	    {
		/* Format the page before writing the BI */
		(*t->tcb_acc_plv->dmpp_format)(pgtype, pgsize, pinfo->page,
				    pageno, DMPP_DATA, DM1C_ZERO_FILL);
		/*
		** Online Backup Protocols: Check if BIs must be logged before update.
		** dm0p_before_image_check() logs a BI according to the page's lsn,
		** so avoid the call if we are not going to update the fhdr.
		*/
		if (fhdr_update)
		{
		    status = dm0p_before_image_check(r, (DMPP_PAGE *) fhdr, dberr);
		    if (status != E_DB_OK)
			break;
		}

		if (status == E_DB_OK)
		    status = dm0p_before_image_check(r, (DMPP_PAGE *) fmap, dberr);
		if (status == E_DB_OK)
		    status = dm0p_before_image_check(r, pinfo->page, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /* 
	    ** Reserve log file space for alloc and its clr.  As
	    ** fhdr/fmaps are forced during online recovery, we must pass 
	    ** the force flag to LGreserve. (Amended b119663 - kibro01 -
	    ** LG_RS_FORCE is not required in all circumstances, so long 
	    ** as we're dealing with fast commit single-cache servers)
	    **
	    ** To prevent a possible deadlock situation, first unmutex the
	    ** newly allocated page. This LGreserve call may be stalled due
	    ** to LOGFULL or other conditions. If a CP is running or is
	    ** started while we're stalled, it may encounter the mutexed
	    ** page and will wait for it to become unmutexed, which it
	    ** won't!
	    */
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, pinfo);

	    dm0l_flag = (is_journal(r) ? DM0L_JOURNAL : 0);

	    if ( r->rcb_state & RCB_IN_MINI_TRAN )
	        dm0l_flag |= DM0L_MINI;

            /* Same logic as that which might actually force the page */
            if (dm0p_set_lg_force(tbio,r))
            {
                lg_force_flag = LG_RS_FORCE;
            } else
            {
                dm0l_flag |= DM0L_FASTCOMMIT;
                lg_force_flag = 0;
            }

	    cl_status = LGreserve(lg_force_flag, r->rcb_log_id, 2, 
		sizeof(DM0L_ALLOC) * 2, &sys_err);

	    /* Remutex the page */
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, pinfo);

	    if (cl_status != OK)
	    {
		(VOID) uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    err_code, 1, 0, r->rcb_log_id);
		SETDBERR(dberr, 0, E_DM92F7_DM1P_SCAN_MAP);
		status = E_DB_ERROR;
		break;
	    }

	    /* 
	    ** We use temporary physical locks for catalogs and extension
	    ** tables. The same protocol must be observed during recovery.
	    */
	    if ( NeedPhysLock(r) )
		dm0l_flag |= DM0L_PHYS_LOCK;
            else if (row_locking(r))
                dm0l_flag |= DM0L_ROW_LOCK;
	    else if ( crow_locking(r) )
                dm0l_flag |= DM0L_CROW_LOCK;

	    /*
	    ** Find physical location ID's for log record below.
	    */
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fhdr_pageno);
	    fhdr_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, fmap_pageno);
	    fmap_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;
	    loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count, pageno);
	    free_loc_config_id = tbio->tbio_location_array[loc_id].loc_config_id;

	    status = dm0l_alloc(r->rcb_log_id, dm0l_flag, &t->tcb_rel.reltid,
		&t->tcb_rel.relid, t->tcb_relid_len,
		&t->tcb_rel.relowner, t->tcb_relowner_len,
		pgtype,
		pgsize,
		t->tcb_rel.relloccount, 
		fhdr_pageno,
		fmap_pageno,
		pageno,
		fmap_sequence,
		fhdr_loc_config_id, fmap_loc_config_id,
		free_loc_config_id, hwmap_update, freehint_update,
		(LG_LSN *)NULL, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Update the FHDR page if this allocate has changed its highwater
	** or freemap hints.
	**
	** If FHDR doesn't need updating, it's already been unfixed.
	*/
	if (fhdr_update)
	{
	    dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fhdr_lock->pinfo);
	    /*
	    ** Update the FHDR's highwater mark to indicate that there are
	    ** formatted pages within this fmap's range (if necessary).
	    */
	    if (hwmap_update)
		DM1P_VPT_SET_FHDR_HWMAP_MACRO(pgtype, fhdr, *map_index + 1);

	    /*
	    ** If we have just allocated the last free page in this FMAP
	    ** range, then update the free space hint in the FHDR so we will
	    ** not search this page on the next allocate request.
	    */
	    if (last_free_bit)
		VPS_CLEAR_FREE_HINT_MACRO(pgtype, fhdr_map);

	    DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(pgtype, fhdr, DMPP_MODIFY);
	    if (r->rcb_logging & RCB_LOGGING)
		DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(pgtype, fhdr, lsn);
	    dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fhdr_lock->pinfo);

	    /* Unfix the FHDR and zero caller's pointer to it */
	    fhdr_status = unfix_header(r, free_header, fhdr_lock,
				&fmap, &fmap_lock, dberr);
	    fhdr = (DM1P_FHDR *)NULL;
	}


	/*
	** Update the FMAP page to indicate that the newly allocated page
	** is no longer free.  If there were no potential free pages
	** in front of the one just allocated, then move the firstbit
	** pointer forward to the next spot.
	** Update highwater mark if never set or higher than previous.
	*/
	dm0pMutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);

	{
	    if (BITMAP_TST_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(pgtype, fmap), freebit) == 0 ||
		(fmap_bitmap[bitmap_index] & (1 << bit_index)) == 0)
	    {
		TRdisplay("%@ scan_map has the wrong bit, spinning\n");
		for(;;);
	    }
	}

	fmap_bitmap[bitmap_index] &= ~(1 << bit_index);

#ifdef DEBUG_SCAN_MAP
	if (fhdr_lock->fix_lock_mode != FHDR_UPDATE_LKMODE)
	    TRdisplay("%@ %d RESETB %d, bitmap_index %d bit_index %d bits %x firstbit %d\n",
		    sid, pageno, bitmap_index, bit_index, 
		    fmap_bitmap[bitmap_index], fmap_firstbit);
#endif

	if ( ! found_busy_page && freebit >= fmap_firstbit )
	    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(pgtype, fmap, freebit + 1);

	if (fmap_hw_mark == fseg || freebit > fmap_hw_mark)
	    DM1P_VPT_SET_FMAP_HW_MARK_MACRO(pgtype, fmap, freebit);

	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(pgtype, fmap, DMPP_MODIFY);
	if (r->rcb_logging & RCB_LOGGING)
	    DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(pgtype, fmap, lsn);

	/* Unmutex and unfix the FMAP page, also the FHDR if still fixed */
	dm0pUnmutex(tbio, (i4)0, r->rcb_lk_id, &fmap_lock.pinfo);
	fmap_status = unfix_map(r, &fmap, &fmap_lock, free_header, fhdr_lock, dberr);


	/*
	** If we haven't already formatted the page because of online backup,
	** format the newly allocate page as a data page.
	*/
	if ((d->dcb_status & DCB_S_BACKUP) == 0 ||
	    (r->rcb_logging & RCB_LOGGING) == 0)
	{
	    (*t->tcb_acc_plv->dmpp_format)(pgtype, pgsize, pinfo->page,
				    pageno, (DMPP_DATA|DMPP_MODIFY),
				    DM1C_ZERO_FILL);
	}
	else
	{
	   DMPP_VPT_SET_PAGE_STAT_MACRO(pgtype, pinfo->page, DMPP_MODIFY);
	}

	DMPP_VPT_SET_PAGE_TRAN_ID_MACRO(pgtype, pinfo->page, r->rcb_tran_id);
	DMPP_VPT_SET_PAGE_LG_ID_MACRO(pgtype, pinfo->page, r->rcb_slog_id_id);
	DMPP_VPT_SET_PAGE_SEQNO_MACRO(pgtype, pinfo->page, 0);
	if (r->rcb_logging & RCB_LOGGING)
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(pgtype, pinfo->page, lsn);

	if ((status = fhdr_status) ||
	    (status = fmap_status))
	{
	    break;
	}

	/*
	** Unmutex the allocated page but leave it pinned
	** when row or crow locking.
	**
	** The pin will be removed when the page is unfixed
	** (by dm0p_uncache_fix()) or when explicitly unlocked
	** by dm0pUnpinBuf().
	**
	** This keeps other MVCC threads (which don't rely on the page lock)
	** from seeing the page even though the X page lock has been   
	** released or downgraded to IX by dm1r_lk_convert.
	*/
	if ( (crow_locking(r) || row_locking(r)) )
	    dm0pUnmutex(tbio, 0, r->rcb_lk_id, pinfo);
	else
	{
	    dm0pUnmutex(tbio, DM0P_MUNPIN, r->rcb_lk_id, pinfo);

	    /* 
	    ** Now convert the physical lock on the free page to logical
	    ** ( if we didn't specify that we actually wanted a physical
	    **   lock in the first place ).
	    **
	    ** If (c)row_locking, the caller will convert PHYSICAL X to IX,
	    ** and then convert PHYSICAL to LOGICAL after the mini transaction
	    ** is completed. The lock conversion may result in 
	    ** E_DMA00D_TOO_MANY_LOG_LOCKS, and we cannot escalate (WAIT)
	    ** if buffers are mutexed. (B92909)
	    ** Also if we are adding a new page in a HEAP, we cannot give
	    ** other sessions access to this new page until this session
	    ** has allocated space (B92908)
	    */

	    if ( (flag & DM1P_PHYSICAL) == 0 &&
		  r->rcb_lk_type != RCB_K_TABLE &&
		  r->rcb_k_duration != RCB_K_PHYSICAL )
	    {
		i4	lock_action = 0;
		i4	lock_mode = LK_X;

		/* Convert PHYSICAL lock to logical */
		status = dm0p_trans_page_lock(r, t, 
		    pageno,
		    DM0P_WRITE, lock_mode, &lock_action, 
		    &r->rcb_fix_lkid, dberr);
	    }
	}

	r->rcb_page_adds++;
	return (status);
    }

    /*
    ** Error/warning handling: unfix page and map if necessary and log the error.
    ** No pages have been left in a mutexed state.
    */
    if (pinfo->page)
    {
	dm0pUnmutex(tbio, DM0P_MINCONS_PG, r->rcb_lk_id, pinfo);

	/* Unfix/UNLOCK the no longer "free" page */
	local_status = unfix_nofmt_page(r, pinfo, pageno, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	}
    }
    if (fmap)
    {
	local_status = unfix_map(r, &fmap, &fmap_lock, (DM1P_FHDR **)NULL,
					(DM1P_LOCK *)NULL, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	}
    }

    if (status == E_DB_ERROR)
    {
	dm1p_log_error( E_DM92F7_DM1P_SCAN_MAP, dberr, 
		       &t->tcb_rel.relid, &d->dcb_name);
    }
    return (status);
}

/*{
** Name: fix_header - Fix the free list header page.
**
** Description:
**      This routine fixes the free map header page. 
**
**	All FHDR pages are fixed for WRITE and locked either
**	for read or for update.
**	
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	fhdr_lock			DM1P_LOCK pointer with
**	    fix_lock_mode		    mode of FHDR lock wanted
**						
** Outputs:
**      header                          Pointer to header page pointer=.
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**	    E_DB_WARN			Checksum error, but caller wished it
**				        to be ignored.
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**      19-apr-1990 (Derek)
**          Created.
**	19-Nov-1991 (rmuth)
**	    Removed the action type DM0P_FHFMPSYNC from dm0p_fix_page so
**	    took out the setting of this action.
**	13-Dec-1991 (rmuth)
**	    Added traceback message E_DM92DD_DM1P_FIX_HEADER, altered
**	    error handling to handle traceback message.
**	23-March-1992 (rmuth)
**	    Corrected problem with original error handling which was causing
**	    us to access violate.
**	31-March-1992 (rmuth)
**	    Added function prototypes, This involved the following
**		- Change header from (DMPP_PAGE **) to (DM1P_FHDR **).
**	29-August-1992 (rmuth)
**	    - Removed the on-the-fly conversion.
**	    - Reorder for error handling.
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:
**	      - Removed check_sum of FHDR and FMAP pages.
**	      - Removed the DM0P_MUTEX fix action when fixing FMAP and FHDR
**		pages.  Code which updates these pages must now explicitly
**		mutex the pages while updating them.
**	      - Removed Before Image handling when fixing FMAP and FHDR pages.
**		Before Images are now generated as needed by the Buffer Manager.
**	      - Changed to fix header for READ if DM1P_FOR_UPDATE flag not
**		passed to the call.
**	08-feb-1993 (rmuth)
**	    If we have fixed the page ok but it is not a FHDR page then 
**	    return a warning.
**	26-July-1993 (rmuth)
**	    If relfhdr is corrupt and points to past end-of-table then
**	    catch this and pass back a warning. Used to rebuild table.
**	8-apr-1994 (rmuth)
**	   b62487, incorrectly set errcode when checking for E_DM9206
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	02-jul-1996 (pchang)
**	    Added DM0P_NOESCALATE action when fixing the file header itself
**	    since lock escalation there may also lead to unrecoverable deadlock.
**	28-Feb-2007 (jonj)
**	    Add DM0P_NOLKSTALL action to dm0p_cachefix_page().
**	06-Mar-2007 (jonj)
**	    Add redo_lsn parm to dm0p_cachefix_page() prototype for
**	    online Cluster REDO recovery.
*/
static DB_STATUS
fix_header(
    DMP_RCB	*r,
    DM1P_FHDR	**header,
    DM1P_LOCK	*fhdr_lock,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    i4	    local_err_code;
    DB_STATUS	    status;
    DB_STATUS	    s;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    *header = NULL;

    status = lock_page(r, fhdr_lock, t->tcb_rel.relfhdr, dberr);
    
    if (status == E_DB_OK)
    {
	status = dm0p_cachefix_page(tbio, 
		t->tcb_rel.relfhdr, DM0P_WRITE, DM0P_NOLKSTALL,
		r->rcb_lk_id, r->rcb_log_id,
		&r->rcb_tran_id, (DMPP_PAGE *)NULL,
		t->tcb_acc_plv->dmpp_format,
		&fhdr_lock->pinfo,
		(LG_LSN*)NULL, dberr);

	if (status == E_DB_OK)
	{
	    *header = (DM1P_FHDR*)fhdr_lock->pinfo.page;
	    /* 
	    ** Check that this is a FHDR page, if not return a warning
	    ** message. This is used when so that we can open an ill
	    ** table and try and rebuild the table.
	    */
	    if ( !(DM1P_VPT_GET_FHDR_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		    (*header)) & DMPP_FHDR) )
	    {
		status = E_DB_WARN;
		SETDBERR(dberr, 0, E_DM92DD_DM1P_FIX_HEADER);
	    }
	}
	else
	{
	    /* An error occurred fixing the FHDR; unlock it */
	    s = unlock_page(r, fhdr_lock, &local_dberr);
	    if (s != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	    /*
	    ** An error occurred fixing the FHDR page, return a warning and
	    ** it is up to the caller to decide what they want to do.
	    ** For example patching a table will still want to open
	    ** the table to read the data. 
	    */
	    status = E_DB_WARN;
	}
    }

    if (status != E_DB_OK)
    {
	/* Handle error */

	if (*header)
	{
	    s = unfix_header(r, header, fhdr_lock, 
				(DM1P_FMAP**)NULL, (DM1P_LOCK*)NULL, &local_dberr);
	    if (s != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	}

	dm1p_log_error(E_DM92DD_DM1P_FIX_HEADER, dberr, 
		       &t->tcb_rel.relid,
		       &d->dcb_name);

    }

    return (status);
}

/*{
** Name: unfix_header - Unfix a header page.
**
** Description:
**
**	This routine unfixes an FHDR page, perhaps.
**
**	If an FMAP is still fixed, the FHDR cannot be unfixed because
**	that would leave the FMAP potentially unprotected from concurrent
**	writers under some conditions:
**
**	If we unfix and unlock the FHDR, a FMAP may be still fixed 
**	and locked for update, which seems fine, but a concurrent thread may
**	be granted an update lock on this FHDR and the protocol says that
**	if the FHDR is locked for update, FMAP locks are not needed, so the
**	second thread would fail to take any lock on the FMAP, even
**	though the first thread still has it update locked! We can't allow
**	that, so what we'll do here is to not unfix/unlock the FHDR,
**	if the FMAP does not have sufficient protection of its own.
**	
**	
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	fhdr				Pointer to pointer to FHDR page.
**	fhdr_lock			Current fhdr lock information.
**	fmap				Pointer to pointer to FMAP page, if any.
**	fmap_lock			Lock information for that FMAP.
**
** Outputs:
**      header                          Pointer to header page pointer.
**					Becomes null if FHDR is really unfixed.
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      
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
**
** History:
**      19-apr-1990 (Derek)
**          Created.
**	 3-nov-1991 (rogerk)
**	    Added force flag as part of recovery fixes for File Extend.
**	13-Dec-1991 (rmuth)
**	    Added E_DM92DE_DM1P_UNFIX_HEADER trace back message.
**	10-Feb-1992 (rmuth)
**	    Added error message E_DM92ED_DM1P_ERROR_INFO, to catch a bad page
**	    address being passed in.
**	29-August1992 (rmuth)
**	    - Added more error handling to track down a DMF access violation.
**	    - Add rcb parameter to dm1p_log_error.
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.
**	      - Removed check_sum of FHDR and FMAP pages.
**	      - Removed the DM0P_MUTEX fix action when fixing FMAP and FHDR
**		pages.  Code which updates these pages must now explicitly
**		mutex the pages while updating them.
**	      - Removed requirement that FHDR and FMAP pages be written at
**		unfix time when they are modified.  They are now recovered
**		through standard REDO recovery actions.
**	19-Apr-2010 (jonj)
**	    Bug 123598: "fmap_lock" input parameter may be NULL, 
**	    don't dereference if so.
*/
static DB_STATUS
unfix_header(
    DMP_RCB	*r,
    DM1P_FHDR	**fhdr,
    DM1P_LOCK	*fhdr_lock,
    DM1P_FMAP	**fmap,
    DM1P_LOCK	*fmap_lock,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;

    DB_STATUS	    status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** This routine was AV'ing as the FHDR ptr was pointing to null. Catch
    ** the error more gracefully with the following.
    */
    if ( fhdr && *fhdr)
    {
	/* 
	** If an FMAP is truly locked for update, we can unlock and unfix
	** the FHDR, otherwise we'll have to wait until after the FMAP
	** is unlocked and unfixed.
	*/
	if (r->rcb_lk_type != RCB_K_TABLE && fmap_lock && fmap_lock->pinfo.page &&
	    ( fmap_lock->lock_id.lk_unique == 0 ||
	      fmap_lock->lock_id.lk_common == 0 ||
	      fmap_lock->lock_value.lk_mode != FMAP_UPDATE_LKMODE ))
	{
	    /* Insufficient protection for the FMAP, leave the FHDR */
	    status = E_DB_OK;
	}
	else
	{
	    *fhdr = NULL;

	    status = dm0p_uncache_fix(tbio, DM0P_UNFIX, r->rcb_lk_id,
				      r->rcb_log_id, &r->rcb_tran_id,
				      &fhdr_lock->pinfo, dberr);

	    if (status != E_DB_OK)
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);

	    status = unlock_page(r, fhdr_lock, dberr);
	}
    }
    else
    {
	status = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM92DE_DM1P_UNFIX_HEADER);
    }

    if (status != E_DB_OK)
    {
	dm1p_log_error(E_DM92DE_DM1P_UNFIX_HEADER, dberr, 
		   &t->tcb_rel.relid,
		   &d->dcb_name);
    }

    return (status);
}

/*{
** Name: lock_page - Lock a FHDR/FMAP.
**
** Description:
**	Place a lock on a FHDR/FMAP prior to fixing the page.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      lock                            Pointer to DM1P_LOCK with:
**	    .fix_lock_mode		    the mode to lock the FHDR/FMAP
**	pageno				Page number of FHDR/FMAP.
**
** Outputs:
**	lock
**	    .lock_value.lk_mode		is set to the granted mode of the lock.
**      err_code                        Pointer to an area to return
**                                      error code.
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
**
** History:
**	16-Jun-1998 (jenjo02)
**	    created.
*/
static DB_STATUS
lock_page(
    DMP_RCB	*r,
    DM1P_LOCK	*lock,
    DM_PAGENO	pageno,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DB_STATUS	    status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Get page lock if needed */
    if (r->rcb_lk_type != RCB_K_TABLE)
    {
	status = dm0p_lock_page(r->rcb_lk_id, d, &t->tcb_rel.reltid,
				pageno, LK_PAGE,
				lock->fix_lock_mode & ~(DM1P_FOR_SCRATCH),
				LK_PHYSICAL, r->rcb_timeout, 
				&t->tcb_rel.relid, &t->tcb_rel.relowner,
				&r->rcb_tran_id, &lock->lock_id, (i4 *)NULL,
				&lock->lock_value, dberr);
    }
    else
    {
	/* No page lock needed; make it look like FHDR/FMAP is locked */
	lock->lock_id.lk_unique = 0;
	lock->lock_id.lk_common = 0;
	lock->lock_value.lk_high = 0;
	lock->lock_value.lk_low = 0;
	status = E_DB_OK;
    }

    if (status == E_DB_OK)
	lock->lock_value.lk_mode = lock->fix_lock_mode & ~(DM1P_FOR_SCRATCH);

    return(status);
}

/*{
** Name: unlock_page - Unlock a FHDR/FMAP.
**
** Description:
**	Release a lock on a FHDR/FMAP.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      lock                            Pointer to DM1P_LOCK containing the
**	    .lock_id			lock_id to release.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
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
**
** History:
**	16-Jun-1998 (jenjo02)
**	    created.
*/
static DB_STATUS
unlock_page(
    DMP_RCB	*r,
    DM1P_LOCK	*lock,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DB_STATUS	    status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (r->rcb_lk_type != RCB_K_TABLE &&
        (lock->lock_id.lk_unique || lock->lock_id.lk_common) )
    {
	status = dm0p_unlock_page(r->rcb_lk_id, d, &t->tcb_rel.reltid,
				(DM_PAGENO)0, LK_PAGE,
				&t->tcb_rel.relid, &r->rcb_tran_id,
				&lock->lock_id,
				&lock->lock_value, dberr);
    }
    else
	status = E_DB_OK;

    if (status == E_DB_OK)
    {
	lock->lock_id.lk_unique = 0;
	lock->lock_id.lk_common = 0;
	lock->lock_value.lk_mode = LK_N;
    }
    return(status);
}

/*{
** Name: relock_pages - Convert the lock mode of a FHDR/FMAP.
**
** Description:
**
**	Uses non-deadlocking techniques to convert a FHDR/FMAP lock
**	to another mode.
**
**	The first attempt requests the conversion with NOWAIT; if a
**	wait would be required, the lock is converted to LK_N and
**	a conversion from LK_N to the requested mode is made, this
**	time waiting.
**
**	If two DM1P_LOCKs are passed in and a wait would be required for
**	the first lock, the second is also converted to null before retrying
**	the first lock and converted back to its requested mode after the
**	first lock conversion is granted.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**      lock1                           DM1P_LOCK of first lock to convert with
**	    .new_lock_mode		 the mode to convert to.
**      lock2                           DM1P_LOCK of second lock, if any, to convert, with
**	    .new_lock_mode		 the mode to convert to.
**
** Outputs:
**	lock1.lock_value.lk_mode	granted mode of converted lock
**	lock2.lock_value.lk_mode	granted mode of converted lock, if any.
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**	    E_DB_WARN			If a wait was required for (either) lock.
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	16-Jun-1998 (jenjo02)
**	    created.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
*/
static DB_STATUS
relock_pages(
    DMP_RCB	*r,
    DM1P_LOCK	*lock1,
    DM1P_LOCK	*lock2,
    DB_ERROR	*dberr )
{
    STATUS	    cl_status = OK;
    DB_STATUS	    status = E_DB_OK;
    CL_ERR_DESC	    sys_err;
    i4	lock_flags = r->rcb_tcb_ptr->tcb_table_io.tbio_cache_flags &
        TBIO_CACHE_MULTIPLE ? LK_PHYSICAL | LK_CONVERT | LK_NOWAIT :
        LK_PHYSICAL | LK_CONVERT | LK_NOWAIT | LK_LOCAL;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (r->rcb_lk_type != RCB_K_TABLE)
    {
	while (cl_status == OK)
	{
	    /* First try converting lock1 to new mode NOWAIT */
	    cl_status = LKrequest(lock_flags,
			r->rcb_lk_id, (LK_LOCK_KEY *)NULL, lock1->new_lock_mode,
			&lock1->lock_value, &lock1->lock_id,
			(i4)0, &sys_err);

	    if (cl_status == OK)
	    {
		lock1->lock_value.lk_mode = lock1->new_lock_mode;
		break;
	    }

	    if (cl_status == LK_BUSY)
	    {
		status = E_DB_WARN;
		cl_status = OK;

		/* The next lock requests will wait */
		lock_flags &= ~LK_NOWAIT;
		
		/* If there's a second lock, convert it down to null if not already */
		if (lock2 &&
		    (lock2->lock_id.lk_unique || lock2->lock_id.lk_common) &&
		    lock2->lock_value.lk_mode != LK_N)
		{
		    cl_status = LKrequest(lock_flags,
				r->rcb_lk_id, (LK_LOCK_KEY *)NULL, LK_N,
				&lock2->lock_value, &lock2->lock_id,
				(i4)0, &sys_err);
		    if (cl_status == OK)
			lock2->lock_value.lk_mode = LK_N;
		}
		
		/* 
		** Convert first lock down to NULL;
		*/
		if (cl_status == OK)
		{
		    cl_status = LKrequest(lock_flags,
				r->rcb_lk_id, (LK_LOCK_KEY *)NULL, LK_N,
				&lock1->lock_value, &lock1->lock_id,
				(i4)0, &sys_err);
		    if (cl_status == OK)
			lock1->lock_value.lk_mode = LK_N;
		}

		/* Loop back to retry 1st lock, this time waiting */
	    }
	}
	
	/* If there's a second lock, lock it new mode if not already */
	while (lock2 && cl_status == OK &&
		(lock2->lock_id.lk_unique || lock2->lock_id.lk_common) &&
		lock2->lock_value.lk_mode != lock2->new_lock_mode)
	{
	    cl_status = LKrequest(lock_flags,
			r->rcb_lk_id, (LK_LOCK_KEY *)NULL, lock2->new_lock_mode,
			&lock2->lock_value, &lock2->lock_id,
			(i4)0, &sys_err);
	    if (cl_status == OK)
	    {
		lock2->lock_value.lk_mode = lock2->new_lock_mode;
		break;
	    }

	    if (cl_status == LK_BUSY)
	    {
		status = E_DB_WARN;
		cl_status = OK;
		lock_flags &= ~LK_NOWAIT;
	    }
	}
    }
    else
	return(E_DB_OK);

    /* Return E_DB_WARN if wait required, E_DB_OK if no wait */
    if (cl_status == OK)
	return(status);

    SETDBERR(dberr, 0, cl_status);
    return(E_DB_ERROR);
}

/*{
** Name: force_fpage - Forces the page to disc.
**
** Description:
**	This routine forces either the FHDR or FMAP to disc it leaves the 
**	page fixed in the buffer cache.
**
**	A FHDR is forced to disc by the following
**	    - Physically extending a permanent table.
**	    - If updating the FHDR inside a mini-transaction.
**	
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	page				Pointer to page to force.
**
** Outputs:
**      header                          Pointer to header page pointer=.
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      
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
**	    Any dm0p_unfix_page is automatically logged.
**
** History:
**      29-August-1992 (rmuth)
**	    created.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.  Removed check_sum of FHDR and FMAP pages.
**	14-dec-1992 (jnash)
**	    Reduced Logging Project.  Change function name to force_fpage
**	    to not conflict w force_page in dm0p.
*/
static DB_STATUS
force_fpage(
    DMP_RCB	*rcb,
    DMP_PINFO	*pinfo,
    DB_ERROR	*dberr )
{
    DB_STATUS	status;

    status = dm0p_unfix_page(rcb, DM0P_FORCE, pinfo, dberr);

    return (status);
}
/*{

** Name: fix_map - Fix a free list bitmap (FMAP) page.
**
** Description:
**
**      This routine fixes a free list bitmap page, always for WRITE.
**
**	If the FMAP's FHDR is locked for update, then no FMAP locks are 
**      required and the FMAP will be fixed for WRITE unlocked.
**	
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	fmap_pageno			The disk page number to read.
**	fmap_lock			Pointer to FMAP's lock information with
**	    .fix_lock_mode		  the desired FMAP lock mode.
**	fhdr				Pointer to pointer to FHDR, if any.
**	fhdr_lock			That FHDR's lock information.
**
** Outputs:
**      fmap				Pointer to FMAP page pointer.
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      
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
**          none
**
** History:
**      19-apr-1990 (Derek)
**          Created.
**	19-Nov-1991 (rmuth)
**	    Removed the action type DM0P_FHFMPSYNC from dm0p_fix_page so
**	    took out the setting of this action.
**	13-Dec-1991 (rmuth)
**	    Added E_DM92EE_DM1P_FIX_MAP traceback message, altered 
**	    flow of error handling accordingly.
**	23-March-1992 (rmuth)
**	    Corrected problem with original error handling which was causing
**	    us to access violate.
**	31-March-1992 (rmuth)
**	    Added function prototypes, changed err_code to i4.
**	29-August-1992 (rmuth) 
**	    - Rewrote the error handling.
**	    - Add rcb parameter to dm1p_log_error.
**	30-October-1992 (rmuth)
**	    - Just pass table and db name to dm1p_log_error;
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.
**	      - Removed check_sum of FHDR and FMAP pages.
**	      - Removed the DM0P_MUTEX fix action when fixing FMAP and FHDR
**		pages.  Code which updates these pages must now explicitly
**		mutex the pages while updating them.
**	      - Removed Before Image handling when fixing FMAP and FHDR pages.
**		Before Images are now generated as needed by the Buffer Manager.
**	      - Changed to fix map for READ if DM1P_FOR_UPDATE flag not
**		passed to the call.
**	08-feb-1993 (rmuth)
**	    If we have fixed the page ok but it is not a FMAP page then 
**	    return a warning.
**	26-July-1993 (rmuth)
**	    If FHDR is corrupt and holds page numbers past the end-of-table
**	    then return a warning so that table can be opened and patched.
**	8-apr-1994 (rmuth)
**	   b62487, incorrectly set errcode when checking for E_DM9206
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	28-Feb-2007 (jonj)
**	    Add DM0P_NOLKSTALL action to dm0p_cachefix_page().
**	06-Mar-2007 (jonj)
**	    Add redo_lsn parm to dm0p_cachefix_page() prototype for
**	    online Cluster REDO recovery.
**	02-Apr-2010 (jonj)
**	    SIR 121619 MVCC: Fix new FMAPs with DM0P_NOWAIT; if the page
**	    collides with another being simultaneously allocated,
**	    DM004C_LOCK_RESOURCE_BUSY will returned by dm0p_cachefix_page.
*/
static DB_STATUS
fix_map(
    DMP_RCB	    *r,
    i4	    fmap_pageno,
    DM1P_FMAP	    **fmap,
    DM1P_LOCK	    *fmap_lock,
    DM1P_FHDR	    **fhdr,
    DM1P_LOCK	    *fhdr_lock,
    DB_ERROR	    *dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    i4	    local_err_code;
    DB_STATUS	    status;
    DB_STATUS	    s;

    i4	    fix_action = 0;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    *fmap = NULL;

    /*
    ** FMAPs don't need to be locked if their FHDR is fixed 
    ** and locked for update.
    */
    if (fhdr && *fhdr && fhdr_lock->lock_value.lk_mode == FHDR_UPDATE_LKMODE)
    {
	fmap_lock->lock_id.lk_unique = 0;
	fmap_lock->lock_id.lk_common = 0;
	fmap_lock->lock_value.lk_high = 0;
	fmap_lock->lock_value.lk_low = 0;
	fmap_lock->lock_value.lk_mode = FMAP_UPDATE_LKMODE;
	status = E_DB_OK;
    }
    else
	status = lock_page(r, fmap_lock, fmap_pageno, dberr);
    
    if (status == E_DB_OK)
    {
	if (fmap_lock->fix_lock_mode & DM1P_FOR_SCRATCH)
	    fix_action |= (DM0P_SCRATCH | DM0P_NOWAIT);

	status = dm0p_cachefix_page(tbio, 
		fmap_pageno, DM0P_WRITE, fix_action | DM0P_NOLKSTALL,
		r->rcb_lk_id, r->rcb_log_id,
		&r->rcb_tran_id, (DMPP_PAGE *)NULL,
		t->tcb_acc_plv->dmpp_format,
		&fmap_lock->pinfo,
		(LG_LSN*)NULL, dberr);
	
	if (status == E_DB_OK)
	{
	    *fmap = (DM1P_FMAP*)fmap_lock->pinfo.page;

	    /*
	    ** If creating a new FMAP, initialize its pagetype,
	    ** otherwise make sure we read a FMAP.
	    */
	    if (fmap_lock->fix_lock_mode & DM1P_FOR_SCRATCH)
	    {
		dm0p_pagetype(tbio, (DMPP_PAGE *)*fmap, r->rcb_log_id, DMPP_FMAP);
	    }
	    else if (!(DM1P_VPT_GET_FMAP_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		    (*fmap)) & DMPP_FMAP) )
	    {
		status = E_DB_WARN;
		SETDBERR(dberr, 0, E_DM92EE_DM1P_FIX_MAP);
	    }
	}
	else
	{
	    /* An error occurred fixing the FMAP; unlock it */
	    s = unlock_page(r, fmap_lock, &local_dberr);
	    if (s != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	    /*
	    ** An error occurred on the FMAP page, return a warning and
	    ** it is up to the caller to decide what they want to do.
	    ** For example patching a table will still want to open
	    ** the table to read the data. 
	    */
	    status = E_DB_WARN;
	}
    }

    if (status != E_DB_OK)
    {
	/* Handle error */

	if (*fmap)
	{
	    s = unfix_map(r, fmap, fmap_lock, (DM1P_FHDR **)NULL,
				(DM1P_LOCK *)NULL, &local_dberr);
	    if (s != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	}

	dm1p_log_error(E_DM92EE_DM1P_FIX_MAP, dberr, 
		       &t->tcb_rel.relid, &d->dcb_name);
    }

    return (status);
}

/*{
** Name: unfix_map - Unfix the map page.
**
** Description:
**      This routine unfixes and unlocks a fixed bitmap page.
**	
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	fmap				Pointer to FMAP page pointer.
**	fmap_lock			Pointer to FMAP lock information.
**	fhdr				Optional pointer to FHDR pointer. 
**					If present, the FHDR will be unfixed also.
**	fhdr_lock			FHDR lock information.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
**                                      
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
**          Any error from dm0p_unfix_page is automatically logged.
**
** History:
**      19-apr-1990 (Derek)
**          Created.
**	 3-nov-1991 (rogerk)
**	    Added force flag as part of recovery fixes for File Extend.
**	13-Dec-1991 (rmuth)
**	    Added traceback message E_DM92EF_DM1P_UNFIX_MAP.
**	10-Feb-1992 (rmuth)
**	    Added E_DM92ED_DM1P_ERROR_INFO to catch bad page addresses.
**	29-August-1992 (rmuth)
**	    - Added more handling to catch a DMF access violation.
**	    - Add parameter to dm1p_log_error.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project.
**	      - Removed check_sum of FHDR and FMAP pages.
**	      - Removed the DM0P_MUTEX fix action when fixing FMAP and FHDR
**		pages.  Code which updates these pages must now explicitly
**		mutex the pages while updating them.
**	      - Removed requirement that FHDR and FMAP pages be written at
**		unfix time when they are modified.  They are now recovered
**		through standard REDO recovery actions.
*/
static DB_STATUS
unfix_map(
    DMP_RCB	*r,
    DM1P_FMAP	**fmap,
    DM1P_LOCK	*fmap_lock,
    DM1P_FHDR	**fhdr,
    DM1P_LOCK	*fhdr_lock,
    DB_ERROR	*dberr )
{
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DMP_DCB	    *d = t->tcb_dcb_ptr;
    DMP_TABLE_IO    *tbio = &t->tcb_table_io;
    DB_STATUS	    status, s;
    DM_PAGENO	    fmap_pageno;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** This routine was AV'ing as the FMAP ptr was null, catch
    ** the error more gracefully with the following.
    */
    if ( fmap && *fmap )
    {
	*fmap = NULL;

	status = dm0p_uncache_fix(tbio, DM0P_UNFIX, r->rcb_lk_id,
				  r->rcb_log_id, &r->rcb_tran_id,
				  &fmap_lock->pinfo, dberr);

	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
	
	status = unlock_page(r, fmap_lock, dberr);

	/* If FHDR passed in, unfix it also */
	if (fhdr && *fhdr)
	{
	    s = unfix_header(r, fhdr, fhdr_lock, fmap, fmap_lock,
					&local_dberr);
	    if (s != E_DB_OK)
	    {
		*dberr = local_dberr;
		status = s;
	    }
	}
    }
    else
    {
	SETDBERR(dberr, 0, E_DM92EF_DM1P_UNFIX_MAP);
	status = E_DB_ERROR;
    }

    if (status != E_DB_OK)
	dm1p_log_error(E_DM92EF_DM1P_UNFIX_MAP, dberr, 
		   &t->tcb_rel.relid, &d->dcb_name);
    return (status);
}

/*{
** Name: dm1pLogErrorFcn - Used to log trace back messages.
**
** Description:
**	This routine is used to log traceback messages. The rcb parameter
**	is set if the routine is can be seen from outside this module.
**	
** Inputs:
**	terr_code			Holds traceback message indicating
**					current routine name.
**	dberr				Holds error message generated lower
**					down in the call stack.
**	rcb				Pointer to rcb so can get more info.
**
** Outputs:
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	25-March-1992 (rmuth)
**	   Added header also altered code so that we can generate traceback
**	   messages even when the error message generated lower down in the
**	   call stack is the error message we return to the DMF client.
**	29-August-1992 (rmuth)
**	   Add call to E_DM92ED_DM1P_ERROR_INFO.
**	23-apr-1994 (rmuth)
**	   - b58422, dm1p_log_error(), if encounter a DEADLOCK error suppress
**	     the information message as causes much unnecessary stress among
**	     users.
**      05-may-1999 (nanpr01)
**         dm1p_log_error() LK_TIMEOUT should be supressed (same as DEADLOCK)
**	20-Aug-2002 (jenjo02)
**	   Also suppress LK_UBUSY (TIMEOUT=NOWAIT) encounters.
**	12-Feb-2003 (jenjo02)
**	   Deleted DM006B_NOWAIT_LOCK_BUSY, 
**	   DM004D_LOCK_TIMER_EXPIRED will be returned instead.
**	04-Dec-2008 (jonj)
**	   Renamed to dm1pLogErrorFcn, invoked by dm1p_log_error macro.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new E_DM016B_LOCK_INTR_FA error.
**	02-Apr-2010 (jonj)
**	    SIR 121619 MVCC:
**	    DM004C_LOCK_RESOURCE_BUSY is anticipated.
*/
static VOID
dm1pLogErrorFcn(
    i4	terr_code,
    DB_ERROR	*dberr,
    DB_TAB_NAME	*table_name,
    DB_DB_NAME	*dbname,
    PTR		FileName,
    i4		LineNumber)
{
    i4		l_err_code;
    DB_ERROR	local_dberr;

    /*
    ** Deadlock is sort-of expected event so do not issue the trace
    ** back messages
    */
    if ( dberr->err_code != E_DM0042_DEADLOCK  && 
	 dberr->err_code != E_DM004D_LOCK_TIMER_EXPIRED && 
	 dberr->err_code != E_DM0065_USER_INTR &&
         dberr->err_code != E_DM016B_LOCK_INTR_FA &&
	 dberr->err_code != E_DM004C_LOCK_RESOURCE_BUSY )
    {
	local_dberr = *dberr;
	local_dberr.err_code = E_DM92CB_DM1P_ERROR_INFO;
	
	uleFormat( &local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &l_err_code, 2,
		sizeof( table_name->db_tab_name ),
		table_name->db_tab_name,
		sizeof(dbname->db_db_name),
		dbname->db_db_name);

	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &l_err_code, 0);

	    dberr->err_code = terr_code;
	    dberr->err_file = FileName;
	    dberr->err_line = LineNumber;
	}
	else
	{
	    /* 
	    ** err_code holds the error number which needs to be returned to
	    ** the DMF client. So we do not want to overwrite this message
	    ** when we issue a traceback message
	    */
	    local_dberr.err_code = terr_code;
	    local_dberr.err_data = 0;
	    local_dberr.err_file = FileName;
	    local_dberr.err_line = LineNumber;

	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &l_err_code, 0);
	}

    }
    return;
}

static i4
print(
    PTR		arg,
    i4	  	count,
    char	*buffer )
{
    SCF_CB	scf_cb;

    buffer[count] = '\n';
    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = (u_i4)count + 1;
    scf_cb.scf_ptr_union.scf_buffer = buffer;
    scf_cb.scf_session = DB_NOSESSION;
    scf_call(SCC_TRACE, &scf_cb);
}

/*{
** Name: dm1p_build_SMS - Add the Space Management scheme to a table
**
** Description:
**	This routine is used to add the Space Managemnet scheme, 
**	FHDR/FMAP(s) to a table.
**
**	This routine will write the FHDR/FMAP(s) directly to the file
**	bypassing the buffer manager. It will mark pages as free or
**	used accordingly.
**
**	Its is primarily used by the build and 6.4->6.5 table conversion
**	code. 
**
**	It returns the page number of the FHDR so that the calling routine
**	can set the iirelation.relfhdr field for this table accordingly.
**
**
**
** Inputs:
**	pages_used			Holds the number of pages used in the
**					table. Also used to return the
**					number of pages used once the FHDR
**					,FMAP(s) have been added.
**	pages_allocated			The number of pages currently
**					allocated to the table. This 
**					value is updated if the more pages
**					are allocated.
**	location_array			The location array for the table.
**	loc_count			The number of locations.
**	table_name			The table name
**	dbname				The database name.
**	extend_size			Tables extend size.
**	flag				Values are
**					DM1P_BUILD_SMS_IN_MEMORY - this
**					build the FHDR/FMAP(s) in the
**					Buffer cache.
**	rcb				Used for building the FHDR/FMAP(s)
**					in the BM, otherwise not used.
** Outputs:
**	fhdr_pageno			The page number of the FHDR.
**      err_code                        Pointer to an area to return
**	pages_used			The number of pages used in the 
**					table.
**	pages_allocated			The number of pages allocated to
**					the table.
**
**      Returns:
**
**          E_DB_OK
**          E_DB_ERROR.
**      Exceptions:
**          none
**
** Side Effects:
**          None.
** History:
**      29-August-1992 (rmuth)
**         Created.
**	26-oct-1992 (rogerk)
**	   Initialize last page pointer before calling dm2f_alloc_file. If
**	   we don't, then the allocate routine does not know how to proportion
**	   up the allocate among the underlying locations.
**	30-October-1992 (rmuth)
**	   Use a structure to pass and return all the information for 
**	   build a tables FHDR and FMAP.
**	10-November-1992 (rmuth)
**	   Remove call to check_sum() as this has been retired.
**	11-jan-1993 (mikem)
**	   Changed "owner" argument to dm1p_build_SMS()'s call to 
**	   dm0m_allocate() of space for the fhdr and fmap.  Previosly the 
**	   uninitialized variable "fhdr_ptr" was passed.
**	   Since the owner of this control block is itself (and not available
**	   until after the call) just pass NULL instead.
**	26-July-1993 (rmuth)
**	   When calculating th enumber of fmaps needed we were comparing
**	   pages used and page numbers. This is wrong as page numbers start
**	   from zero corrected accordingly.
**	8-apr-1994 (rmuth)
**	   b61019, If adding the process of adding FHDR/FMAP(s) causes an
**	   a file extend we were incorrectly formatting the FHDR/FMAP(s).
**	26-feb-1996 (pchang)
**	    When rebuilding an existing table's FHDR/FMAPs after data had been
**	    loaded, existing FMAPs that contained free pages prior to loading
**	    were not rebuilt correctly, which resulted in FMAP inconsistency 
**	    and orphan data pages that could not be referenced (B74873). 
**	    Specifically, the condition to test whether an entire FMAP is to be 
**	    marked as used was incomplete in that it left out the case for 
**	    existing FMAPs.  Hence, the free bits as well as the FHINT bits for
**	    such FMAPs were not being cleared accordingly.
**	06-may-1996 (thaju02)
**	    New page format support:
**		Pass added parameter page_size in dm1p_init_fhdr(),
**		dm1p_add_fmap(), dm1p_single_fmap_free(), dm1p_used_range()
**		routines.
**		Change page header references to use macros.
**      1-Nov-1998 (wanfr01)
**        Fix for bug 93547:
**          The incrementing of last_marked_used_pageno to skip pages being
**          checked will cause problems if the final page being updated is the
**          last page on the FMAP.  The incrementing of last_marked_used_pageno
**          causes the next FMAP to be updated even though no pages are actually
**          changed on it. Its fmap_lastbit gets changed to 15999 (DM1P_FSEG-1),
**          corrupting the table
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer to dm0m_deallocate routine.
** 	20-Jul-2000 (jenjo02)
**	    Update caller's SMS_pages_allocated and SMS_pages_used,
**	    which both the comments here and the callers declare
**	    as being done, but was not.
**	2-Mar-2007 (kschendel) SIR 122757
**	    Align page buffers for direct I/O.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dm1p_build_SMS(
DM1P_BUILD_SMS_CB *build_cb,
DB_ERROR	  *dberr )
{
    DM_PAGENO	ignore;
    i4	initial_pages_mapped;
    DM1P_FHDR	*fhdr_ptr;
    DM1P_FMAP	*fmap_ptr;
    DB_STATUS	status = E_DB_OK;
    i4	final_no_pages_used;
    i4	final_no_alloc_pages;
    i4	mapped_to;
    i4	next_mapped_to;
    DM_PAGENO	last_marked_used_pageno;
    DM_PAGENO	first_marked_free_pageno;
    DM_PAGENO	end_pageno;
    DM_PAGENO	fmap_pageno;
    bool	build_SMS = TRUE;
    i4	fmap_needed;
    i4	bit;
    PTR		local_buffer = (PTR)0;

    i4	fseg = DM1P_FSEG_MACRO(build_cb->SMS_page_type, build_cb->SMS_page_size);

    CLRDBERR(dberr);


    if (build_cb->SMS_page_size == 0)
    {
	TRdisplay("sorry. page size of 0 is no good.\n");
	SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	return (E_DB_ERROR);
    }

    if (DMZ_AM_MACRO(50))
    {
	TRdisplay("dm1p_build_SMS: table %~t used %d alloc %d %dK V%d\n",
    	 	sizeof(build_cb->SMS_table_name),
    		build_cb->SMS_table_name.db_tab_name,
		build_cb->SMS_pages_used,
		build_cb->SMS_pages_allocated,
		build_cb->SMS_page_size/1024, build_cb->SMS_page_type);
	TRdisplay("dm1p_build_SMS: %dK V%d maxpages %d max fmap %d fmapintsz %d\n",
		build_cb->SMS_page_size/1024, build_cb->SMS_page_type,
		DM1P_VPT_MAXPAGES_MACRO(build_cb->SMS_page_type, 
			build_cb->SMS_page_size),
		DM1P_NUM_FMAP_PAGENO_ON_FHDR_MACRO(build_cb->SMS_page_type, 
			build_cb->SMS_page_size),
		DM1P_VPT_MAP_INT_SIZE_MACRO(build_cb->SMS_page_type));
    }

    do
    {
	u_i4 directio_align = dmf_svcb->svcb_directio_align;
	u_i4 size;

	size = (build_cb->SMS_page_size * 2) + sizeof(DMP_MISC);
	if (dmf_svcb->svcb_rawdb_count > 0 && directio_align < DI_RAWIO_ALIGN)
	    directio_align = DI_RAWIO_ALIGN;
	if (directio_align != 0)
	    size += directio_align;
	status = dm0m_allocate(
			size,
			(i4)0,
			(i4)MISC_CB,
			(i4)MISC_ASCII_ID,
			(char *)NULL,
			(DM_OBJECT **)(&local_buffer),
			dberr);
	if ( status != E_DB_OK )
	    break;

	fhdr_ptr = (DM1P_FHDR *)((char *)local_buffer + sizeof(DMP_MISC));
	if (directio_align != 0)
	    fhdr_ptr = (DM1P_FHDR *) ME_ALIGN_MACRO(fhdr_ptr, directio_align);
	((DMP_MISC*)local_buffer)->misc_data = (char*)fhdr_ptr;
	fmap_ptr = (DM1P_FMAP *)((char *)fhdr_ptr + build_cb->SMS_page_size);

	/*
	** If a new table create the FHDR otherwise read the
	** existing FHDR.
	*/
	if ( build_cb->SMS_new_table )
	{
	    /* Create a new FHDR */
	    build_cb->SMS_fhdr_pageno = (DM_PAGENO )build_cb->SMS_pages_used;
	    status = get_page_build_SMS( 
				     (DMPP_PAGE *)fhdr_ptr,
				     build_cb->SMS_fhdr_pageno,
				     build_cb, dberr );
	    if ( status != E_DB_OK )
	    	break;
	    
	    dm1p_init_fhdr( fhdr_ptr, &ignore, 
			build_cb->SMS_page_type, build_cb->SMS_page_size );
	}
	else
	{
	    /* Read existing FHDR */
	    status = get_page_build_SMS( (DMPP_PAGE *)fhdr_ptr,
				         build_cb->SMS_fhdr_pageno,
				         build_cb, dberr );
	    if ( status != E_DB_OK )
	    	break;
	}


	/* 
	** Work out if how adding the FHDR and FMAP(s) will affect
	** the number of allocated and used pages. We do this
	** so that we can set up the FHDR/FMAP(s) in one pass
	*/
	initial_pages_mapped = 
	    DM1P_VPT_GET_FHDR_COUNT_MACRO(build_cb->SMS_page_type, fhdr_ptr) * fseg;
	final_no_pages_used  = build_cb->SMS_pages_used;
        final_no_alloc_pages = build_cb->SMS_pages_allocated;

	if ( initial_pages_mapped < final_no_alloc_pages )
	{
	    i4	total_pages_mapped;

	    /*
	    ** work out how many new fmap pages will be added so we
	    ** know the total number of used pages
	    */
	    fmap_needed = (((final_no_alloc_pages - 1) / fseg) + 1) - 
		DM1P_VPT_GET_FHDR_COUNT_MACRO(build_cb->SMS_page_type, fhdr_ptr);

	    final_no_pages_used = build_cb->SMS_pages_used + fmap_needed;

	    /*
	    ** See if we will need to allocate more space to accomodate
	    ** these new pages
	    */
	    while ( final_no_pages_used > final_no_alloc_pages )
	    {
		total_pages_mapped = initial_pages_mapped + 
		      (fmap_needed * fseg);
		final_no_alloc_pages += build_cb->SMS_extend_size;
		/*
		** See if this allocation will cause us to need to add
		** another FMAP
		*/
		if ( final_no_alloc_pages > total_pages_mapped )
		{
	    	    fmap_needed = (((final_no_alloc_pages-1) / fseg) + 1)
			- DM1P_VPT_GET_FHDR_COUNT_MACRO(build_cb->SMS_page_type,
			fhdr_ptr);

		    final_no_pages_used = build_cb->SMS_pages_used + 
								fmap_needed;

		    total_pages_mapped = initial_pages_mapped +
			(fmap_needed * fseg);
		}
	    }
	}

	/*
	** Setup the FHDR and FMAP(s) accordingly
	*/

	mapped_to = 0;
	last_marked_used_pageno  = build_cb->SMS_start_pageno;
	first_marked_free_pageno = final_no_pages_used;

	/*
	** Loop through all the FMAP's, to see if they need pages marked
	** as free or used.
	*/
	while ( mapped_to < final_no_alloc_pages )
	{
	    next_mapped_to = mapped_to + fseg;

	    /*
	    ** See if need to update the current FMAP. Some load
	    ** operations preserve the existing FHDR/FMAP, for 
	    ** example bulk-load into a HEAP table. This means
	    ** some FMAP(s) may not need updating.
	    */
	    if ( last_marked_used_pageno < next_mapped_to )
	    {
		/*
		** See if we need to add a new FMAP yet
		*/
		if ( mapped_to >= initial_pages_mapped )
		{
		    /* 
		    ** add a new FMAP 
		    */
	            status = get_page_build_SMS( 
				     (DMPP_PAGE *)fmap_ptr,
				     build_cb->SMS_pages_used,
				     build_cb, dberr );
	            if ( status != E_DB_OK )
	    	    	break;

		    dm1p_add_fmap( fhdr_ptr, fmap_ptr, &ignore, 
			build_cb->SMS_page_type, build_cb->SMS_page_size );

		}
		else
		{
		    /* 
		    ** Use existing FMAP 
		    */
		    fmap_pageno = VPS_NUMBER_FROM_MAP_MACRO(build_cb->SMS_page_type,
			DM1P_VPT_GET_FHDR_MAP_MACRO(build_cb->SMS_page_type, 
			fhdr_ptr, (mapped_to/fseg)));

	            status = get_page_build_SMS( 
				     (DMPP_PAGE *)fmap_ptr,
				     fmap_pageno, build_cb, dberr );
	            if ( status != E_DB_OK )
	    	    	break;

		}

		/*
		** Check if the current FMAP needs pages marked as used.
	 	*/
		if (( last_marked_used_pageno >= mapped_to ) &&
		    ( last_marked_used_pageno < next_mapped_to ) )
		{
		    if ( final_no_pages_used < next_mapped_to )
		    {
			end_pageno = final_no_pages_used - 1;
		    }
		    else
		    {
			/*
			** All the pages on this FMAP will be marked as
			** used
			*/
			end_pageno = next_mapped_to - 1;
		    }

		    /*
		    ** If the whole page is to be marked as used then 
		    ** turn off all remaining free bits in the FMAP bitmap 
		    ** (except for a new FMAP which need not be done),  
		    ** set the boundary bits and update the FHDR accordingly.
		    */
		    if ( ( (last_marked_used_pageno == mapped_to) ||
			   ((last_marked_used_pageno % fseg) ==
			    DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(build_cb->SMS_page_type, 
			    fmap_ptr)) ) &&
		     	 ( end_pageno == ( next_mapped_to - 1) ) )
		    {
			if (mapped_to < initial_pages_mapped)
			{
			    /* Turn off all free bits for an existing FMAP */

			    for (bit = last_marked_used_pageno;
				    bit <= end_pageno; bit++)
			    {
				DM1P_VPT_CLR_FMAP_BITMAP_MACRO(build_cb->SMS_page_type,
				    fmap_ptr, (bit % fseg) / DM1P_BTSZ, 
				    (1 << ((bit % fseg) % DM1P_BTSZ)));
			    }
			}

			DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(build_cb->SMS_page_type, 
			    fmap_ptr, fseg);
			DM1P_VPT_SET_FMAP_LASTBIT_MACRO(build_cb->SMS_page_type,
			    fmap_ptr, fseg - 1);
			DM1P_VPT_SET_FMAP_HW_MARK_MACRO(build_cb->SMS_page_type,
			    fmap_ptr, fseg - 1);

			DM1P_VPT_SET_FHDR_HWMAP_MACRO(build_cb->SMS_page_type, 
			    fhdr_ptr, 
			    DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(build_cb->SMS_page_type,
			    fmap_ptr) + 1);
			DM1P_VPT_SET_FHDR_PAGES_MACRO(build_cb->SMS_page_type,
			    fhdr_ptr, final_no_alloc_pages);

			/* Clear Free hint in FHDR for this FMAP */
			VPS_CLEAR_FREE_HINT_MACRO(build_cb->SMS_page_type, 
			    DM1P_VPT_GET_FHDR_MAP_MACRO(build_cb->SMS_page_type, 
			    fhdr_ptr, 
			    DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(build_cb->SMS_page_type,
			    fmap_ptr)));
		    }
		    else
		    {
			status = dm1p_used_range(
					fhdr_ptr, fmap_ptr,
					build_cb->SMS_page_type,
					build_cb->SMS_page_size,
					(DM_PAGENO *)&last_marked_used_pageno,
					end_pageno,
					&ignore,
					dberr );
			if ( status != E_DB_OK )
			    break;
		    }

                    last_marked_used_pageno = end_pageno + 1 ;

                    /*
                    **  If this is the last page being updated, don't increment
                    **  last_marked_used_pageno.  It may cause an extra FMAP to
                    **  be updated.
                    */

                    if (last_marked_used_pageno == final_no_pages_used)
                       last_marked_used_pageno--;

		}


		/*
		** Check if the current FMAP needs pages marked as free.
	 	*/
		if ( ( first_marked_free_pageno >= mapped_to ) &&
		     ( first_marked_free_pageno < next_mapped_to ))
		{
		    if ( build_cb->SMS_pages_allocated < next_mapped_to )
		    {
			end_pageno = build_cb->SMS_pages_allocated - 1;
		    }
		    else
		    {
			end_pageno = next_mapped_to - 1;
		    }

		    /*
		    ** Check that there are some free pages left
		    */
		    if ( end_pageno < build_cb->SMS_pages_allocated )
		    {
			status = dm1p_single_fmap_free(
					fhdr_ptr, fmap_ptr,
					build_cb->SMS_page_type,
					build_cb->SMS_page_size,
					&first_marked_free_pageno,
					&end_pageno,
					&ignore,
					dberr );
			if ( status != E_DB_OK )
			    break;
		    }

		    first_marked_free_pageno = end_pageno + 1;
		}

		status = put_page_build_SMS( 
				(DMPP_PAGE *)fmap_ptr,
				 build_cb, dberr);

		if ( status != E_DB_OK )
		    break;

	    }

	    mapped_to = next_mapped_to;


	} /* while */

	if ( status != E_DB_OK )
	    break;


	/*
	** write the FHDR
	*/
	status = put_page_build_SMS( (DMPP_PAGE *)fhdr_ptr,
				      build_cb, dberr);
	if ( status != E_DB_OK)
	    break;

	dm0m_deallocate( (DM_OBJECT **)&local_buffer );

	/* Return possibly updated used/allocated pages */
	build_cb->SMS_pages_used = final_no_pages_used;
	build_cb->SMS_pages_allocated = final_no_alloc_pages;


    } while (FALSE);


    if ( status == E_DB_OK )
	return( E_DB_OK );

    /*
    ** Error condition
    */

    if ( local_buffer )
	(VOID ) dm0m_deallocate( (DM_OBJECT **)&local_buffer );

    dm1p_log_error( E_DM92F1_DM1P_BUILD_SMS, dberr, 
		    &build_cb->SMS_table_name,
		    &build_cb->SMS_dbname );

    return( status );
}

/*{
** Name:  get_page_build_SMS - fix or read a page.
**
** Description:
**	This routine will either read or fix a page it is used by the
**	the Space Management Build code. 
**
**
** Inputs:
**	page				Pointer to page.
**	pageno				Page to read/fix.
**	build_cb			Holds context needed to build
**					the FHDR/FMAP.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
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
**	    None.
**
** History:
**      29-August-1992 (rmuth)
**	    Created.
**	30-October-1992 (rmuth)
**	    Ehanced to use the DM1P_BUILD_SMS_CB inof block and to enable
**	    the fhdr/fmap to be built in one pass.
**	14-dec-1992 (rogerk)
**	    When create new page in the buffer manager, set its page status
**	    to indicate that it is modified so that if the buffer manager
**	    decides to toss the page it will not lose its contents if the
**	    table is in deferred IO state.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass SMS_page_size as the page_size argument to dmpp_format.
**	20-Jul-2000 (jenjo02)
**	    Update tbio_lalloc keyed on whether an RCB is passed instead
**	    of whether this is an inmemory build or not. This corrects
**	    a problem in RAW fastloads.
*/
static DB_STATUS
get_page_build_SMS(
DMPP_PAGE	   *page,
DM_PAGENO	   pageno,
DM1P_BUILD_SMS_CB  *build_cb,
DB_ERROR	   *dberr )
{
    DB_STATUS 		status = E_DB_OK;
    i4		io_count = 1;
    DMP_PINFO		bufferPinfo;
    i4		ignore;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(50))
	TRdisplay("DM1P: get_page_build_SMS: table %~t pageno %d \n",
    	 	sizeof(build_cb->SMS_table_name),
    		build_cb->SMS_table_name.db_tab_name,
		pageno );

    do
    {
	/*
	** See if requesting a new page or to read an existing page
	*/
	if ( pageno < build_cb->SMS_pages_used )
	{
	    /* 
	    ** Request to read an existing page 
	    */
	    if ( build_cb->SMS_build_in_memory )
	    {
		/*
		** The following is to stop us having to hold
		** the mutex on the page for an extended period.
		** We know that no other user can be updating the
		** page as the build code lock the whole table
		** and in-memory 
		*/
    		status = dm0p_fix_page( 
				build_cb->SMS_inmemory_rcb, (i4)pageno,
			    	DM0P_READ, &bufferPinfo, dberr );
		if ( status != E_DB_OK)
		    break;

		MEcopy((PTR)bufferPinfo.page, build_cb->SMS_page_size,
			(PTR)page);

		status = dm0p_unfix_page(
				build_cb->SMS_inmemory_rcb, DM0P_UNFIX,
				&bufferPinfo, dberr );
		if ( status != E_DB_OK )
		    break;
					
	    }
	    else
	    {
    		status = dm2f_read_file(
				build_cb->SMS_location_array,
				build_cb->SMS_loc_count,
				build_cb->SMS_page_size,
				&build_cb->SMS_table_name,
				&build_cb->SMS_dbname,
				&io_count, (i4)pageno,
				(char *)page, dberr);
		if ( status != E_DB_OK)
		    break;

	    }
	}
	else
	{
	    /*
	    ** Request for a new page, make sure that there
	    ** is enough allocated space in the table for a new
	    ** page if not then allocate more space to the table.
	    ** Note during build we keep adding pages to the end
	    ** of the table so even with the minimum extend size of
	    ** one this allocation will include the page
	    */
    	    if ( pageno >= build_cb->SMS_pages_allocated )
    	    {
		i4 local_page = pageno;

	    	status = dm2f_alloc_file(
				build_cb->SMS_location_array,
				build_cb->SMS_loc_count,
				&build_cb->SMS_table_name,
				&build_cb->SMS_dbname,
				build_cb->SMS_extend_size,
				&local_page,
				dberr);
	    	if ( status != E_DB_OK )
		    break;

	    	build_cb->SMS_pages_allocated = local_page + 1;

		if ( build_cb->SMS_inmemory_rcb )
		{
		    /*
		    ** Update tcb_alloc now as another thread in the server
		    ** can see these pages before we complete building
		    ** the table
		    */
	    build_cb->SMS_inmemory_rcb->rcb_tcb_ptr->tcb_table_io.tbio_lalloc = 
							local_page;
		}
    	    }


	    if ( build_cb->SMS_build_in_memory )
	    {
		/*
		** The following is to stop us having to hold
		** the mutex on the page for an extended period.
		** We know that no other user can be updating the
		** page as the build code lock the whole table
		** and in-memory 
		*/
    		status = dm0p_fix_page( 
				build_cb->SMS_inmemory_rcb, (i4)pageno,
			    	DM0P_SCRATCH,&bufferPinfo, dberr );
		if ( status != E_DB_OK)
		    break;

		MEcopy((PTR)bufferPinfo.page, build_cb->SMS_page_size,
			(PTR)page);

		/*
		** Increase this value now as another thread in the
		** server can see these pages before the build is
		** complete
		*/
	        build_cb->SMS_inmemory_rcb->rcb_tcb_ptr->tcb_table_io.
						tbio_lpageno = pageno;

		/*
		** Mark the page as MODIFIED in the buffer manager to prevent
		** the buffer manager from silently tossing the page from the
		** cache to make room for other uses.  Marking it as modified
		** means that at least tossing it will materialize the table
		** if it is in deferred iO state and allow us to refix the 
		** page later.
		*/
		DMPP_VPT_SET_PAGE_STAT_MACRO(build_cb->SMS_page_type, 
		    bufferPinfo.page, DMPP_MODIFY);

		status = dm0p_unfix_page(
				build_cb->SMS_inmemory_rcb, DM0P_UNFIX,
				&bufferPinfo, dberr );
		if ( status != E_DB_OK )
		    break;
	    }
	    else
	    {
		(*build_cb->SMS_plv->dmpp_format)(  build_cb->SMS_page_type,
						    build_cb->SMS_page_size,
						    page, 
						    pageno,
						    (DMPP_MODIFY),
						    DM1C_ZERO_FILL );
	    }

	    /*
	    ** Allocated a new page so increase the number of used pages
	    ** count 
	    */
	    build_cb->SMS_pages_used++;
	}

    } while ( FALSE );

    if ( status != E_DB_OK )
	dm1p_log_error( E_DM92F8_DM1P_GET_PAGE_BUILD_SMS, dberr, 
		        &build_cb->SMS_table_name,
		        &build_cb->SMS_dbname );

    return( status );
}

/*{
** Name:  put_page_build_SMS - unfix or write a page.
**
** Description:
**	This routine will either write or unfix a page it is used by the
**	the Space Management Build code. 
**
**	
**
** Inputs:
**	page				Pointer to page to unfix/write.
**	build_cb			Holds context needed to build
**					the FHDR/FMAP.
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code.
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
**	    None.
**
** History:
**      29-August-1992 (rmuth)
**	    Created for File Extend Phase 2.
**	11-sep-1992 (bryanp)
**	    If no RCB is passed to fhdr build routines, use default local
**	    accessors for page formatting.
**	30-October-1992 (rmuth)
**	    Alter for new DM1P_BUILD_SMS_CB and also to enable the fhdr/fmap
**	    to be built in one pass.
**	10-nov-1992 (rogerk)
**	    Fixed arguments to dm0p_mutex to follow new buffer manager rules.
**	14-dec-1992 (jnash & rogerk)
**	    Reduced logging project.  Insert checksum prior to page write.
**	    Added dm0p_pagetype call after copying page to buffer manager.
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	20-mar-1996 (nanpr01)
**	    Added the page_size parameter in dm0p_insert_checksum call
**	    since page_size is required for computing checksum.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support:
**		Change page header references to use macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
put_page_build_SMS(
DMPP_PAGE		*page,
DM1P_BUILD_SMS_CB	*build_cb,
DB_ERROR		*dberr )
{
    DB_STATUS 		status = E_DB_OK;
    i4		io_count = 1;
    i4		mask;
    DMP_PINFO		bufferPinfo;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(50))
        TRdisplay("DM1P: put_page_build_SMS: table %~t pageno %d \n",
    	 	sizeof(build_cb->SMS_table_name),
    		build_cb->SMS_table_name.db_tab_name,
		DMPP_VPT_GET_PAGE_PAGE_MACRO(build_cb->SMS_page_type, page));

    do
    {
    	if ( build_cb->SMS_build_in_memory )
    	{
	    /*
	    ** The following is to stop us having to hold
	    ** the mutex on the page for an extended period.
	    ** We know that no other user can be updating the
	    ** page as the build code lock the whole table
	    ** and in-memory 
	    */
	    status = dm0p_fix_page(
		      build_cb->SMS_inmemory_rcb, 
		      (i4)DMPP_VPT_GET_PAGE_PAGE_MACRO(build_cb->SMS_page_type,
		      page),
		      DM0P_WRITE, &bufferPinfo, dberr);
	    if ( status != E_DB_OK )
		break;

	    dm0pMutex(&build_cb->SMS_inmemory_rcb->rcb_tcb_ptr->tcb_table_io,
		        (i4)0, build_cb->SMS_inmemory_rcb->rcb_lk_id,
		        &bufferPinfo);

	    MEcopy((PTR)page, build_cb->SMS_page_size, (PTR)bufferPinfo.page);
	    DMPP_VPT_SET_PAGE_STAT_MACRO(build_cb->SMS_page_type, 
	    		bufferPinfo.page, DMPP_MODIFY);

	    /*
	    ** Since this copy may change the page type of the page in the
	    ** buffer manager, we must assign the page type in case the
	    ** buffer manager needs to use special protocols for this type
	    ** of page.
	    */
	    mask = (DMPP_FHDR | DMPP_FMAP | DMPP_DATA | DMPP_LEAF | 
		    DMPP_INDEX | DMPP_CHAIN);
	    dm0p_pagetype(
		&build_cb->SMS_inmemory_rcb->rcb_tcb_ptr->tcb_table_io,
		bufferPinfo.page, build_cb->SMS_inmemory_rcb->rcb_log_id,
		(DMPP_VPT_GET_PAGE_STAT_MACRO(build_cb->SMS_page_type, page) & mask));

	    dm0pUnmutex(&build_cb->SMS_inmemory_rcb->rcb_tcb_ptr->tcb_table_io,
			  (i4)0,
			  build_cb->SMS_inmemory_rcb->rcb_lk_id,
			  &bufferPinfo);

	    status = dm0p_unfix_page(
				build_cb->SMS_inmemory_rcb, DM0P_UNFIX, 
				&bufferPinfo, dberr);
	    if ( status != E_DB_OK )
		break;

        }
        else
        {
	    /*
	    ** Checksum the page before writing it to the database.
	    */
	    dm0p_insert_checksum(page, build_cb->SMS_page_type,
		(DM_PAGESIZE) build_cb->SMS_page_size);

    	    status = dm2f_write_file(
			build_cb->SMS_location_array,
			build_cb->SMS_loc_count,
			build_cb->SMS_page_size,
			&build_cb->SMS_table_name,
			&build_cb->SMS_dbname,
			&io_count,
			(i4)DMPP_VPT_GET_PAGE_PAGE_MACRO(build_cb->SMS_page_type,
					page), 
			(char *)page,
			dberr);
        }

    } while ( FALSE );

    if ( status != E_DB_OK )
	dm1p_log_error( E_DM92F9_DM1P_PUT_PAGE_BUILD_SMS, dberr, 
		        &build_cb->SMS_table_name,
		        &build_cb->SMS_dbname );

    return(status);

}

/*{
** Name: dm1p_convert_table_to_65 - Adds the 6.5 space management scheme to an
**				    exisyting table.
**
** Description:
**	Adds the new FHDR and FMAP pages to a table.
**
**	Note that despite the naming implications, this routine can
**	be called for recent databases, to fix up a rewritten core
**	catalog at upgrade time.
**
** Inputs:
**	tcb				TCB for table to convert.
** Outputs:
**	err_code			error code returned.	
**                                      
**	Returns:
**		E_DB_ERROR		Error.
**		E_DB_OK			All ok.
**
**	Void.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**      29-August-1992 (rmuth)
**          Created.
**	18-apr-1994 (jnash)
**	    fsync project.  Eliminate call to dm2f_force_file().
**      23-mar-1995 (chech02) 67533
**         - b67533, dm1p_convert_table_to_65(), update tcb_page_adds from
**           SMS struct. tcb_page_adds will be used to update relpages of
**           iirelation table.
**	10-Sep-2004 (schka24)
**	    This routine is used during upgradedb to fix up rewritten
**	    core catalogs.  So, it must not assume V1 2K pages any more.
*/
DB_STATUS
dm1p_convert_table_to_65(
    DMP_TCB	*tcb,
    DB_ERROR	*dberr )
{
    DB_STATUS		status;
    DM1P_BUILD_SMS_CB	SMS_build_cb;

    CLRDBERR(dberr);

    if (DMZ_AM_MACRO(52))
	    TRdisplay(
	       "dm1p_convert_table_to_65: Table: %~t, db: %~t\n",
	    	sizeof(tcb->tcb_rel.relid),
		&tcb->tcb_rel.relid,
		sizeof(tcb->tcb_dcb_ptr->dcb_name),
		&tcb->tcb_dcb_ptr->dcb_name);
    do
    {
	/*
	** Find out the size of the table
	*/
	status = dm2f_sense_file(
			tcb->tcb_table_io.tbio_location_array,
			tcb->tcb_table_io.tbio_loc_count,
			&tcb->tcb_rel.relid,
			&tcb->tcb_dcb_ptr->dcb_name,
			&SMS_build_cb.SMS_pages_used,
			dberr);
	if (status != E_DB_OK)
	    break;

#ifdef xDEV_TST
	/*
	** If we are tracing then perform a sanity check to make sure that 
	** the last page is a valid page.
	*/
	if  (DMZ_AM_MACRO(52))
	{
	    i4	io_count;
    	    DMPP_PAGE   page;
	     
	    io_count = 1;
	    status = dm2f_read_file(
			tcb->tcb_table_io.tbio_location_array,
			tcb->tcb_table_io.tbio_loc_count,
			tcb->tcb_table_io.tbio_page_size,
			&tcb->tcb_rel.relid,
			&tcb->tcb_dcb_ptr->dcb_name,
			&io_count, 
			SMS_build_cb.SMS_pages_used,
			(char *)&page,
			dberr);
	    if (status != E_DB_OK)
	    	break;
	    /* 
	    ** The last page is not a valid page it should be ok to
	    ** read this into the buffer manager as it will not be 
	    ** in the table structure.
	    ** If user is tracing the conversion then log an information
	    ** message
	    */
	    if ( page.page_page != SMS_build_cb.SMS_pages_used)
	        TRdisplay(
		"dm1p_convert_table_to_65: Sanity check on last page failed, page_page: %d, page read: %d\n",
			page.page_page,
			SMS_build_cb.SMS_pages_used );

	}
#endif

	/*
	** sense file returns last valid pageno
	*/
	SMS_build_cb.SMS_pages_used++;

	/*
	** Set up context needed to build FHDR and FMAP(s).
	*/
	SMS_build_cb.SMS_pages_allocated = SMS_build_cb.SMS_pages_used;
	SMS_build_cb.SMS_extend_size 	 = 4; 
	SMS_build_cb.SMS_page_type       = tcb->tcb_rel.relpgtype;
	SMS_build_cb.SMS_page_size 	 = tcb->tcb_rel.relpgsize; 
	SMS_build_cb.SMS_start_pageno	 = 0;
	SMS_build_cb.SMS_build_in_memory = FALSE;
	SMS_build_cb.SMS_inmemory_rcb	 = (DMP_RCB *)NULL;
	SMS_build_cb.SMS_new_table	 = TRUE;
	SMS_build_cb.SMS_location_array	= tcb->tcb_table_io.tbio_location_array;
	SMS_build_cb.SMS_loc_count	 = tcb->tcb_table_io.tbio_loc_count;
	SMS_build_cb.SMS_plv		 = tcb->tcb_acc_plv;

	STRUCT_ASSIGN_MACRO( tcb->tcb_rel.relid, SMS_build_cb.SMS_table_name);
	STRUCT_ASSIGN_MACRO(tcb->tcb_dcb_ptr->dcb_name,SMS_build_cb.SMS_dbname);

	/*
	** Build the new 6.5 space management scheme 
	*/
	status = dm1p_build_SMS( &SMS_build_cb, dberr );
	if (status != E_DB_OK)
	    break;

	/*
	** Make sure that all the space allocated to the file
	** has disc space.
	*/
	status = dm2f_guarantee_space( 
				SMS_build_cb.SMS_location_array,
				SMS_build_cb.SMS_loc_count,
				&SMS_build_cb.SMS_table_name,
				&SMS_build_cb.SMS_dbname,
				SMS_build_cb.SMS_pages_used,
				(SMS_build_cb.SMS_pages_allocated -
						SMS_build_cb.SMS_pages_used),
				dberr);
	if ( status != E_DB_OK )
	    break;

	/*
	** Flush the file header to disc
	*/
	status = dm2f_flush_file(
			SMS_build_cb.SMS_location_array,
			SMS_build_cb.SMS_loc_count,
			&SMS_build_cb.SMS_table_name,
			&SMS_build_cb.SMS_dbname,
			dberr );
	if ( status != E_DB_OK )
	    break;

#ifdef xDEV_TEST
	/* 
	** Test failing after we have physically added the FHDR/FMAP to
	** the table but have not updated the relfhdr in memory or
	** on disc yet.
	*/
	if ( DMZ_TST_MACRO(33))
	{
	    status = E_DB_ERROR;
	    break;
	}
#endif

	/*
	** Lets set the relfhdr field for the table
	*/
	tcb->tcb_rel.relfhdr = SMS_build_cb.SMS_fhdr_pageno;
	tcb->tcb_page_adds= SMS_build_cb.SMS_pages_used- tcb->tcb_rel.relpages;
	tcb->tcb_rel.relpages= SMS_build_cb.SMS_pages_used;

    } while (FALSE);

    if ( status != E_DB_OK )
        dm1p_log_error(E_DM92EB_DM1P_CVT_TABLE_TO_65, dberr, 
		       &tcb->tcb_rel.relid,
		       &tcb->tcb_dcb_ptr->dcb_name);


    return( status );
}
/*{
** Name: map_pagetype - Maps a pagetype to a display character
**
** Description:
**	Looks at a page type and maps it to the appropriate
**	character to display.
**
**	NOTE: If the page_type output values change, make sure that
**	vdba is amended accordingly. See fix for b121545 for example.
**
** Inputs:
**	page			Page to map
**	rcb			tables rcb
** Outputs:
**	page_type		Character to display
**                                      
**	Returns:
**	    None
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	08-feb-1993 (rmuth)
**	    Created.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	29-apr-2009 (smeke01) b121545
**	    Added note to description to help prevent recurrence of b121545.
*/
static VOID
map_pagetype(
    DMPP_PAGE	*page,
    DMP_RCB	*rcb,
    char	*page_type)
{
    DMP_TCB   *t = rcb->rcb_tcb_ptr;
    i4 stat = DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page);
    char	isam_index_level[] = {'0','1','2','3','4','5','6','7','8','9'};

    do
    {
	if (stat & DMPP_FREE)
	{
	    *page_type = DM1P_P_FREE;
	    break;
	}

	if (stat & DMPP_FHDR)
	{
	    *page_type = DM1P_P_FHDR;
	    break;
	}

	if (stat & DMPP_FMAP)
	{
	    *page_type = DM1P_P_FMAP;
	    break;
	}

    	if (stat & DMPP_DATA)
	{
	    *page_type = DM1P_P_DATA;
	    if (stat & DMPP_ASSOC)
	    {
		*page_type = DM1P_P_ASSOC;
		break;
	    }

	    if (stat & DMPP_OVFL )
	    {
		*page_type = DM1P_P_OVFDATA;
		break;
	    }

	    break;
	}

	if (stat & DMPP_INDEX)
	{
	    *page_type = DM1P_P_INDEX;
	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page) == 0)
	    {
		*page_type = DM1P_P_ROOT;
		break;
	    }

	    if (stat & DMPP_SPRIG)
	    {
		*page_type = DM1P_P_SPRIG;
		break;
	    }
	    break;
	}

	if (stat & DMPP_LEAF)
	{
	    *page_type = DM1P_P_LEAF;
	    break;
	}

	if (stat & DMPP_CHAIN)
        {
	    *page_type = DM1P_P_OVFLEAF;
	    break;
	}

	if (stat & DMPP_DIRECT)
	{
	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page) ==
				t->tcb_rel.relprim)
	    {
		*page_type = DM1P_P_ROOT;
		break;
	    }

	    if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, page) == 0)
	    {
		*page_type = DM1P_P_LEAF;
		break;
	    }

	    if (( DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, page) > 0)
		&& ( DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, page)
				< 10))
	    {
		*page_type = 
		    isam_index_level[DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, page)];
		break;
	    }
	    else
	    {
	       *page_type = DM1P_P_INDEX;
	       break;
	    }

        }

     	*page_type = DM1P_P_UNKNOWN;

    } while(FALSE);

    return;
}

/*{
** Name: display_hash - Displays a hash table showing overflow chains.
**
** Description:
**	This routine displays a hash table showing overflow chains in
**	such away so that you can relate an overflow chain to a hash
**	bucket. As opposed to scaning the whole table.
**
** Inputs:
**	rcb				Pointer to RCB for this table
**	highwater_mark			Last valid page in the table.
**	total_pages			Total pages on disc for the table.
**	fhdr				Tables FHDR
**
** Outputs:
**	err_code			Pointer to error.
**                                      
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** History:
**	3-June-1992 (rmuth)
**	    Created.
**	11-jan-1993 (mikem)
**	    Initialize next_char in display_pagetypes() to point to beginning 
**	    of "line_type" buffer at beginning of loop.  "used before set" 
**	    error shown up by lint.
**	08-feb-1993 (rmuth)
**	    - Display overflow data pages.
**	    - For ISAM index pages other than the root or bottom level
**	      display the depth.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to macros.
**	15-nov-1998 (nanpr01)
**	    Display the longest chain and other statistics for VDBA.
**	08-jan-1999 (nanpr01)
**	    Display the correct overflow pages.
*/
static DB_STATUS
display_hash(
    DMP_RCB	*rcb,
    DM_PAGENO   highwater_mark,
    DB_ERROR	*dberr )
{
    DMP_RCB	*r = rcb;
    DMP_TCB     *t = r->rcb_tcb_ptr;
    i4	page_number;
    char	line_type[64];
    char	*next_char = line_type;
    char	page_type;
    DMP_PINFO	pinfo;
    char	buffer[132];
    DB_STATUS	status;
    i4	pages_scanned = -1, ovfl_pages_scanned = 0;
    i4	overflow_page = 0;
    i4	low_hashbucket = 0;
    i4	longest_chain = 0, chain_length;
    i4	has_ovfl, ovfl_bucket = 0;
    i4 	longest_bucket = -1;
    i4	is_data, is_ovfl_data, empty_bucket_count = 0;
    i4	bucket_count = t->tcb_rel.relprim;
    i4	ovfl_count =  t->tcb_rel.relpages -
			      t->tcb_rel.relprim;
    i4	empty_ovfl_count = 0;
    i4	empty_bucket_with_ovfl = 0;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    do
    {
	/*
	** Display header
	*/
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    	"\nDisplay structure for %s HASH table: %~t\n\n",
		DU_DB2_CUR_SQL_LEVEL,
	    	sizeof(t->tcb_rel.relid),
	    	&t->tcb_rel.relid );

	r->rcb_state |= RCB_READAHEAD;

    	for ( page_number = 0; 
	      page_number < t->tcb_rel.relprim; page_number++)
    	{
    	    status = dm0p_fix_page(
			r,
			page_number,
			DM0P_READ,
			&pinfo,
			dberr );
    	    if (status != E_DB_OK)
		break;

	    pages_scanned++;
	    map_pagetype( pinfo.page, r, &page_type);
	    remap_pagetype( pinfo.page, r, &page_type);

	    /*
	    ** See if need to print the output yet
	    */
	    if ((pages_scanned % PAGE_PER_LINE) == 0)
	    {
		if ( pages_scanned != 0 )
		{
		    TRformat( print, 0, buffer, sizeof(buffer) - 1,
			      "%4* %8d %t\n", 
			      low_hashbucket,
			      next_char - line_type, 
			      line_type);

		    next_char = line_type;
		    low_hashbucket = page_number;
		}
	    }
	    else
	    { 
		/*
		** If end of a group then seperate from next group with 
		** a space
		*/
		if ((pages_scanned % PAGE_PER_GROUP) == 0)
		{
		    *next_char++ = ' ';
		}
	    }

	    /* 
	    ** Add new page to output buffer
	    */
            *next_char++ = page_type;

	    overflow_page = DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
				pinfo.page);
	    if ((is_data = ((*t->tcb_acc_plv->dmpp_tuplecount)
			       (pinfo.page, t->tcb_rel.relpgsize))) == 0)
		empty_bucket_count++;
	    
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &pinfo, dberr);
	    if ( status != E_DB_OK )
		break;

	    /*
	    ** If page has an overflow chain then scan overflow
	    ** chain
	    */
	    chain_length = 0;
	    has_ovfl = 1;
	    while ( overflow_page != 0 )
	    {
		status = dm0p_fix_page(
			    r,
			    overflow_page,
			    DM0P_READ,
			    &pinfo,
			    dberr );
		if (status != E_DB_OK)
		    break;

		if (has_ovfl)
		{
		    ovfl_bucket++;
		    if (is_data == 0)
			empty_bucket_with_ovfl++;
		    has_ovfl = 0;
		}
	        if ((is_ovfl_data = 
			((*t->tcb_acc_plv->dmpp_tuplecount)
			       (pinfo.page, t->tcb_rel.relpgsize))) == 0)
		    empty_ovfl_count++;
		pages_scanned++;
		ovfl_pages_scanned++;
		chain_length++;
		map_pagetype( pinfo.page, r, &page_type);
	        remap_pagetype( pinfo.page, r, &page_type);

		/*
		** Sanity check that the overflow chains are not
		** corrupt and hence we are looping
		*/
		if ( pages_scanned > highwater_mark )
		{
		    status = dm0p_unfix_page(r, DM0P_UNFIX, &pinfo, dberr);
		    break;
		}

		/*
		** See if need to print the output yet
		*/
		if ((pages_scanned % PAGE_PER_LINE) == 0)
		{
		    TRformat( print, 0, buffer, sizeof(buffer) - 1,
			      "%4* %8d %t\n", 
			      low_hashbucket,
			      next_char - line_type, 
			      line_type);

		    next_char = line_type;
		    low_hashbucket = page_number;
		}
		else
		{
		    /*
		    ** If end of a group then seperate from next group with 
		    ** a space
		    */
		    if ((pages_scanned % PAGE_PER_GROUP) == 0)
		    {
			*next_char++ = ' ';
		    }
		}

		/* 
		** Add new page to output buffer
		*/
		*next_char++ = page_type;

		overflow_page = 
			DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,pinfo.page);

		status = dm0p_unfix_page(r, DM0P_UNFIX, &pinfo, dberr);
		if ( status != E_DB_OK )
		    break;
	    }

	    if ( status != E_DB_OK )
		break;

	    if (chain_length > longest_chain)
	    {
		longest_chain = chain_length;
		longest_bucket = page_number;
	    }

        } /* for-loop */

	if ( status != E_DB_OK )
	    break;

        /*
        ** Display the last buffer
        */
        if (next_char != line_type)
        {
            TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    	"%4* %8d %t\n", 
		low_hashbucket,
	    	next_char - line_type, 
	    	line_type);
    	}

	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"\nD - Non-empty hash buckets.\n");
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"d - Empty hash buckets.\n");
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"o - Non-empty overflow pages.\n");
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"e - Empty overflow pages.\n");
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"\nThere are %d buckets, %d bucket(s) with overflow chains.\n",
		bucket_count, ovfl_bucket);
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"There are %d empty hash bucket(s), %d empty hash bucket(s) with overflow chains.\n",
		empty_bucket_count, empty_bucket_with_ovfl);
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"There are %d overflow pages of which %d are empty.\n",
		ovfl_pages_scanned, empty_ovfl_count);

	if (longest_chain  && (ovfl_pages_scanned != ovfl_bucket))
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"Longest overflow chain is %d in bucket %d\n",
		longest_chain, longest_bucket);
	}
	else if (longest_chain)
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"Longest overflow chain is %d\n",
		longest_chain);
	}

	if (ovfl_bucket)
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"Avg. length of overflow chain is : %f\n",
		(f4) ovfl_pages_scanned/ovfl_bucket);

    } while (FALSE);

    if ( status != E_DB_OK )
	dm1p_log_error( E_DM92F4_DM1P_DISPLAY_PAGETYPES, dberr, 
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);

    return( status );
}
/*{
** Name: dm1p_rebuild - Rebuild a tables FHDR and FMAP.
**
** Description:
**	This routine is called by the patch table code to rebuild a tables
**	FHDR and FMAP.
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control 
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	last_pageno_allocated		Last page allocated in the file.
**	last_pageno_used		Last page used in the table.
**
** Outputs:
**	fhdr_pageno			Location where the FHDR has been
**					placed.
**      err_code                        Pointer to an area to return
**                                      error code.  
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
**
** History:
**	30-feb-1993 (rmuth)
**	    Created.
**	18-apr-1994 (jnash)
**	    fsync project.  Eliminate call to dm2f_force_file().
**	06-mar-1996 (stial01 for bryanp)
**	    Set SMS_page_size when rebuilding table.
*/
DB_STATUS
dm1p_rebuild(
    DMP_RCB	*rcb,
    i4	last_pageno_allocated,
    i4	last_pageno_used,
    i4	*fhdr_pageno,
    DB_ERROR	*dberr)
{
    DB_STATUS		status;
    DM1P_BUILD_SMS_CB	SMS_build_cb;
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;

    CLRDBERR(dberr);

    /*
    ** Set up the context needed to build FHDR and FMAP(s)
    */
    SMS_build_cb.SMS_pages_allocated = last_pageno_allocated + 1;
    SMS_build_cb.SMS_pages_used      = last_pageno_used + 1;
    SMS_build_cb.SMS_extend_size     = t->tcb_rel.relextend;
    SMS_build_cb.SMS_start_pageno    = 0;
    SMS_build_cb.SMS_build_in_memory = TRUE;
    SMS_build_cb.SMS_inmemory_rcb    = r;
    SMS_build_cb.SMS_fhdr_pageno     = *fhdr_pageno;
    SMS_build_cb.SMS_plv	     = t->tcb_acc_plv;
    SMS_build_cb.SMS_location_array  = t->tcb_table_io.tbio_location_array;
    SMS_build_cb.SMS_loc_count       = t->tcb_table_io.tbio_loc_count;
    SMS_build_cb.SMS_page_type       = t->tcb_table_io.tbio_page_type;
    SMS_build_cb.SMS_page_size	     = t->tcb_table_io.tbio_page_size;

    STRUCT_ASSIGN_MACRO( t->tcb_rel.relid, SMS_build_cb.SMS_table_name);
    STRUCT_ASSIGN_MACRO( t->tcb_dcb_ptr->dcb_name, SMS_build_cb.SMS_dbname);

    if ( *fhdr_pageno == DM_TBL_INVALID_FHDR_PAGENO )
    {
	SMS_build_cb.SMS_new_table = TRUE;
    }
    else
    {
	SMS_build_cb.SMS_new_table = FALSE;
    }

    do
    {
	/*
	** Build the new space management code
	*/
	status = dm1p_build_SMS( &SMS_build_cb, dberr );
	if ( status != E_DB_OK )
	    break;

	*fhdr_pageno = SMS_build_cb.SMS_fhdr_pageno;

	/*
	** Make sure that all the space allocated to the file
	** has disc space.
	*/
	status = dm2f_guarantee_space( 
				SMS_build_cb.SMS_location_array,
				SMS_build_cb.SMS_loc_count,
				&SMS_build_cb.SMS_table_name,
				&SMS_build_cb.SMS_dbname,
				SMS_build_cb.SMS_pages_used,
				(SMS_build_cb.SMS_pages_allocated -
						SMS_build_cb.SMS_pages_used),
				dberr);
	if ( status != E_DB_OK )
	    break;

	/*
	** Flush the file header to disc
	*/
	status = dm2f_flush_file(
			SMS_build_cb.SMS_location_array,
			SMS_build_cb.SMS_loc_count,
			&SMS_build_cb.SMS_table_name,
			&SMS_build_cb.SMS_dbname,
			dberr );
	if ( status != E_DB_OK )
	    break;

    } while (FALSE);

    if ( status != E_DB_OK )
	dm1p_log_error(E_DM92D6_DM1P_REBUILD, dberr, 
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);

    return( status );
}

/*{
** Name: dm1p_add_extend - Force an extension of a table.
**
** Description:
**	This routine forces a table to be extended by "extend_amount"
**	size. Called by the "modify to add_extend" code.
**
** Inputs:
**      rcb                             Pointer to record access context.
**	extend_amount			Size to extend table by
**	extend_flags			One of the following:
**					    DM1P_ADD_NEW - additional pages
**					    DM1P_ADD_TOTAL - total pages
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code. 
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
**
** History:
**	30-mar-1993 (rmuth)
**          Created.
**	04-jan-1995 (cohmi01)
**	    Add flag parm to indicate extra new pages, or total pages
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
*/
DB_STATUS
dm1p_add_extend(
    DMP_RCB	*rcb,
    i4	extend_amount,
    i4	extend_flag,
    DB_ERROR	*dberr )
{
    DMP_RCB	    *r = rcb;
    DMP_TCB	    *t = r->rcb_tcb_ptr;
    DM1P_FHDR	    *fhdr = 0;
    DB_STATUS	    status, s;

    DM1P_LOCK	    fhdr_lock;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    /* Invalidate lock_id's */
    fhdr_lock.lock_id.lk_unique = 0;

    /*
    **	Fix the FHDR page.  
    */
    fhdr_lock.fix_lock_mode = FHDR_UPDATE_LKMODE;
    status = fix_header(r, &fhdr, &fhdr_lock, dberr);

    if (status == E_DB_OK)
    {
	/*
	** Extend the table
	*/
	status = extend(r, fhdr, &fhdr_lock, extend_amount, extend_flag, dberr);

	if (status == E_DB_OK)
	{
	    t->tcb_table_io.tbio_lalloc = 
		DM1P_VPT_GET_FHDR_PAGES_MACRO(t->tcb_rel.relpgtype, fhdr) - 1;
	}
    }

    /*	
    ** Unfix the FHDR page. 
    */
    if (fhdr)
    {
	s = unfix_header(r, &fhdr, &fhdr_lock, 
			    (DM1P_FMAP**)NULL, (DM1P_LOCK*)NULL, &local_dberr);

	if (s != E_DB_OK && DB_SUCCESS_MACRO(status))
	{
	    *dberr = local_dberr;
	    status = s;
	}
    }

    if (DB_FAILURE_MACRO(status))
    {
	dm1p_log_error(E_DM92EC_DM1P_ADD_EXTEND, dberr, 
		       &t->tcb_rel.relid,
		       &t->tcb_dcb_ptr->dcb_name);
    }

    return (status);

}
static VOID
remap_pagetype(
    DMPP_PAGE	*page,
    DMP_RCB	*rcb,
    char	*page_type)
{
    DMP_TCB   *t = rcb->rcb_tcb_ptr;
    i4 stat = DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page);
    bool is_data = ((*t->tcb_acc_plv->dmpp_tuplecount)
			       (page, t->tcb_rel.relpgsize));
    do
    {
    	if (stat & DMPP_DATA)
	{
	    if (is_data)
		*page_type = DM1P_P_DATA;
	    else
	        *page_type = DM1P_P_EDATA;

	    if (stat & DMPP_OVFL )
	    {
	        if (is_data)
		    *page_type = DM1P_P_OVFDATA;
		else
		    *page_type = DM1P_P_EOVFDATA;
		break;
	    }

	    break;
	}

    } while(FALSE);

    return;
}


/*{
** Name: unfix_nofmt_page - Unfix/UNLOCK a page that we could not allocate.
**
** Description:
**      This routine is called from scan_map when a page has been STOLEN
**      by another thread - or an error has occurred. 
** 
**      Since this page was fixed with DM0P_CREATE_NOFMT,
**      the page we are unfixing may not be formatted.
**      Call dm0p_uncache_fix with action DM0P_CREATE_NOFMT
**      so that it will bypass consistency checks for this page,
** 
**      It is very important that the page get unlocked!!!!
**      If the page was STOLEN for use as a FMAP page,
**      holding a lock on the FMAP page can cause deadlocks 
**      in DMVE routines that need to fix the FHDR and/or FMAP.
**
** Inputs:
**      rcb                             Pointer to record access context.
**      page
**      pageno				page number 
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      error code. 
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
**
** History:
**     
**      14-mar-05 (stial01)
**          Created for DM0P_CREATE_NOFMT error handling (b114112)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: no dm0p_unlock_page if crow_locking()
*/
static DB_STATUS
unfix_nofmt_page(
DMP_RCB		*rcb,
DMP_PINFO	*pinfo,
i4		pageno,
DB_ERROR	*dberr)
{
    DMP_RCB	*r = rcb;
    DMP_TCB	*t = r->rcb_tcb_ptr;
    DMP_DCB	*d = t->tcb_dcb_ptr;
    DB_STATUS	status;
    DB_STATUS   lk_status;
    DB_ERROR	lk_dberr;
    i4		action;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Pass DM0P_CREATE_NOFMT so that unfix will bypass
    ** page consistency checks for this (possibly) unformatted page.
    */
    action = DM0P_UNFIX | DM0P_CREATE_NOFMT;

    /* NB: This also releases the pin on the page */
    status = dm0p_uncache_fix(&t->tcb_table_io, action,
	r->rcb_lk_id, r->rcb_log_id, &r->rcb_tran_id, pinfo, dberr);

    if ( r->rcb_lk_type != RCB_K_TABLE && !crow_locking(r) )
    {
	/*
	** Since the page may not be formatted, unlock using the
        ** page number this function was called with
	*/
	lk_status = dm0p_unlock_page(r->rcb_lk_id, d, &t->tcb_rel.reltid,
		pageno, LK_PAGE, &t->tcb_rel.relid, &r->rcb_tran_id,
		(LK_LKID *)NULL, (LK_VALUE *)NULL, &lk_dberr);
	if (lk_status)
	    TRdisplay("DM0P_CREATE_NOFMT unlock %d errcode %d\n",
	    pageno, lk_dberr.err_code);
    }

    return (status);
}
