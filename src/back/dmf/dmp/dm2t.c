/*
** Copyright (c) 1985, 2010 Ingres Corporation
**
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <me.h>
#include    <nm.h>
#include    <tm.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <lkdef.h>
#include    <st.h>
#include    <cm.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <gwf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmse.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1m.h>
#include    <dm1r.h>
#include    <dm1s.h>
#include    <dm2d.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm2rlct.h>
#include    <dm2rep.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0s.h>
#include    <dm0l.h>
#include    <dmxe.h>
#include    <dm0pbmcb.h>
#include    <dmppn.h>
#include    <dmd.h>
#include    <dmfgw.h>
#include    <dmftrace.h>
#include    <sxf.h>
#include    <dma.h>
#include    <scf.h>
#include    <rep.h>
#include    <cui.h>
#include    <dmfcrypt.h>

/**
**
**  Name: DM2T.C - TCB manipulation routines.
**
**  Description:
**      This file contains the code that manipulates TCB's.
**
**	    dm2t_open - Open a table.
**	    dm2t_close - Close a table.
**	    dm2t_fix_tcb - Find and fix TCB.
**	    dm2t_unfix_tcb - Release handle to TCB.
**          dm2t_release_tcb - Release a TCB.
**          dm2t_reclaim_tcb - Find a victim to reclaim.
**	    dm2t_control - Set or reset control lock.
**	    dm2t_lock_table - Obtain transaction table lock.
**          dm2t_temp_build - Build a TCB for a temproary table.
**          dm2t_destroy_temp_tcb - Destroy a TCB for a temproary table.
**	    dm2t_purge_tcb  - Purge the TCB.
**	    dm2t_lookup_tabid	- search in-memory TCB's for session temp tbl.
**	    dm2t_update_logical_key - update value of unique id in iirelation
**	    dm2t_wait_for_tcb - wait for TCB to become non-busy.
**	    dm2t_awaken_tcb - wake up any waiters on TCB.
**	    dm2t_fx_tabio_cb - Find and fix Table IO Control Block.
**	    dm2t_ufx_tabio_cb - Release handle to Table IO Control Block.
**	    dm2t_init_tabio_cb - Build Table IO Control Block for BM calls.
**	    dm2t_add_tabio_file - Add new file to existing Partial Tabio cb.
**	    dm2t_wt_tabio_ptr - Wait for busy Table IO Control Block.
**          dm2t_tcb_callback - Blocking callback for TCBs.
**          dm2t_convert_tcb_lock - Convert TCB lock.
**          dm2t_xlock_tcb - Exclusive lock and release a TCB lock.
**          dm2t_alloc_tcb - Allocate memory for and initialize a TCB.
**          dm2t_pin_tabio_cb - Bump a TCB's reference count.
**          dm2t_unpin_tabio_cb - Decrement a TCB's reference count.
**	    dm2t_latest_counts - Get latest reltups/relpages.
**
**  History:
**      07-dec-1985 (derek)
**          Created new for jupiter.
**      25-NOV_87 (jennifer)
**          Added multiple locations per table support.
**      27-APR-88 (teresa)
**	    created dm2t_hunt_tcb
**	12-sep-1988 (rogerk)
**	    Don't blank out the table name and owner in dm2t_build_tcb
**	    as they are useful for error messages.
**	20-sep-1988 (rogerk)
**	    Check if there is read-nolock buffer to deallocate before calling
**	    dm0m_deallocate in dm2t_close.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() and dm2f_build_fcb()
**	    calls.
**	14-oct-88 (sandyh)
**	    added null value checking for tcb_comp_atts_count value for
**	    DB_CHA_TYPE & DB_CHR_TYPE attributes in build_tcb() and
**	    dm2t_temp_build_tcb().
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	 6-feb-89 (rogerk)
**	    Added dm2t_get_tcb_ptr for Buffer Manager to use to look up tcb's.
**	    Changed tcb hashing scheme to use the database id (dcb->dcb_id)
**	    instead of the value of the dcb pointer to form a hash value.
**	13-feb-89 (rogerk)
**	    Fixed update_rel bug with updating page/tuple counts on 2nd indexes.
**	28-feb-89 (rogerk)
**	    Took out build lock and added usage of TCB_BUSY bit.  This allows
**	    TCB's to remain on hash queues while being deallocated (for the
**	    fast commit threads) and allows us to close a table without
**	    requiring a lock (which causes resource problems).
**	    Also added WARNING return from dm2t_release_tcb when dm0p_close_page
**	    call fails.
**	    Added calls to dm0p_tblcache when build or purge tcb.
**	    Added DM2T_TOSS flag to dm2t_release_tcb - default action is not
**	    to toss all pages from buffer cache.
**	20-Mar-89 (rogerk)
**	    Fixed bug introduced with finding VIEW tcb's.
**	14-apr-89 (mikem)
**	    Logical key development. Added dm2t_update_logical_key().
**	    Also initialized tcb_{high,low}_logical_key to 0 in dm2t_open().
**	12-may-89 (anton)
**	    Added local collation support
**	15-may-1989 (rogerk)
**	    Check return value of dm0p_tblcache calls.
**	15-aug-1989 (rogerk)
**	    Added support for Gateways.  Call gateway to open/close gateway
**	    tables.  Added tcb attribute flags (tcb_logging, tcb_logical_log,
**	    tcb_nofile, tcb_no_tids, tcb_update_idx) that should be filled in
**	    when a tcb is built. Don't look for physical file when opening
**	    VIEW or GATEWAY tables.
**	 2-oct-1989 (rogerk)
**	    Use dm0l_fswap to log file renames in dm2t_rename_file.
**	16-oct-89 (rogerk)
**	    Changed dm2t_reclaim_tcb to give up reclaim attempts after ten
**	    failures rather than two.  This adds a little more fault tolerance.
**	2-may-90 (bryanp)
**	    Disabled the code which allowed the building of TCB's without
**	    associated FCB's. This code was an attempt to allow limited use of
**	    tables for which one or more of the underlying physical files did
**	    not exist or could not be opened. However, (a) clients of DMF did
**	    not correctly check for this type of table, and (b) other parts of
**	    DMF assumed that if the TCB existed, it was correct and contained
**	    full FCB information. The only thing you can do with a table
**	    without underlying physical files is to delete it with verifyDB.
**	17-jul-90 (mikem)
**	    bug #30833 - lost force aborts
**	    LKrequest has been changed to return a new error status
**	    (LK_INTR_GRANT) when a lock has been granted at the same time as
**	    an interrupt arrives (the interrupt that caused the problem was
**	    a FORCE ABORT).   Callers of LKrequest which can be interrupted
**	    must deal with this new error return and take the appropriate
**	    action.  Appropriate action includes signaling the interrupt
**	    up to SCF, and possibly releasing the lock that was granted.
**	11-sep-1990 (jonb)
**	    Integrate ingres6301 changes 280136 and 280140 (rogerk):
**            - Changed parameters to dm2t_get_tcb_ptr: added option flags and
**              validation stamp parameter.  Changed build_tcb to copy the tcb's
**              validation stamp into its secondary index tcb's as well.
**            - Fixed AV bug introduced in previous fix.  Make sure tcb pointer
**              is valid before checking validation stamp.
**	12-sep-1990 (jonb)
**	    Integrate ingres6301 change 280141 (rogerk):
**            After building a TCB, call dm0p_set_valid_stamp to mark all
**            previously cached buffers as belonging to this table.
**	19-nov-1990 (bryanp)
**	    bug #33123 -- dmd_check in dm0s_mlock
**	    Fix error handling in dm2t_release_tcb() so that it ALWAYS releases
**	    the hcb_mutex before returning, even under error conditions.
**	19-nov-90 (rogerk)
**	    Added adf control block to the RCB.
**	    Set collation sequence value in the adf control block in dm2t_open.
**	    This was done as part of the DBMS Timezone changes.
**	14-dec-1990 (mikem)
**	    bug #34339 (I THINK THIS IS 34342 (bryanp))
**	    In the case of readlock=nolock sessions, move the code to obtain the
**	    dm2t_control() lock from after the call to dm2t_find_tcb() to before
**	    it. This prevents the possibility of readlock=nolock sessions from
**	    building a TCB concurrently with another "normal" session.
**
**	    Previous to this fix it was possible for the readlock routine
**	    to encounter any of the following errors (when the readlock
**	    sessions tried to build the TCB concurrent with another
**	    adding an index to the table): E_DM9331_DM2T_TBL_IDX_MISMATCH,
**	    E_DM0075_BAD_ATTRIBUTE_ENTRY, E_DM9333_DM2T_EXTRA_IDX.  In some
**	    cases, when the IDX_MISMATCH error is encountered, the session
**	    building the index (not the readlock=nolock session) will silently
**	    fail leaving behind a bad index (retrieves from the index will
**	    point to to the wrong tuples in the base table).
**	17-dec-90 (jas)
**	    Smart Disk project changes:  find out the Smart Disk type,
**	    if any, for a table when it is opened (in build_tcb and
**	    dm2t_temp_build_tcb).
**	 4-feb-91 (rogerk)
**	    Added support for Set Nologging.
**	 4-feb-1991 (rogerk)
**	    Added support for fixed table priorities.
**	    Added tcb_bmpriority field.  Call buffer manager to get table
**	    cache priority when tcb is built.
**	25-feb-1991 (rogerk)
**	    Changed dm0p_gtbl_priority call to take regular DB_TAB_ID.
**	 7-mar-1991 (bryanp)
**	    Changed the DMP_TCB to support Btree index compression, and added
**	    support here: Added new field tcb_data_atts, which is analogous to
**	    tcb_key_atts but lists ALL attributes. These two attribute pointer
**	    lists are used for compressing and uncompressing data & keys.
**	    Added new field tcb_comp_katts_count to the TCB and added code here
**	    to compute the field's value. This field is used by index compresion
**	    to determine the worst-case overhead which compression might add to
**	    an index entry.
**	30-sep-1991 (walt,bryanp)
**	    Re-acquire the hcb_mutex when removing TCBs from the hash queue.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    17-aug-1989 (Derek)
**		Changed hard-coded attribute-key number to compile-time
**		constants that are different for normal and B1 systems.
**	    14-jun-1991 (Derek)
**		Added new routine read_tcb.  Moved code from build_tcb that
**		read the system catalog information for the table into this
**		new routine.
**	    14-jun-1991 (Derek)
**		Add file extend changes.  Added tcb_checkeof field.
**		Took out file rename code from dm2t_rename_file.  We no
**		longer do file renames at the end of load operations.  The
**		routine dm2t_rename_file remains and now just does catalog
**		updates to support the load operation.
**	    14-jun-1991 (Derek)
**		Add support for the automatic conversion of relation records to
**		include new allocation information the first time a table is
**		opened for writing.
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**      24-oct-1991 (jnash)
**          Within dm2t_purge_tcb, relocate "return(E_DB_ERROR)" outside
**	    curly brackets or LINT.
**	29-oct-1991
**	    Add banner and additional error handling to read_tcb.
**	3-dec-1991 (bryanp)
**	    Re-enable automatic table conversion for the iidbdb. Fix automatic
**	    conversion for Btrees.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: added dm2t_locate_tcb(). Also, added
**	    support for DM2T_DESTROY flag to dm2t_close.
**	28-feb-1992 (bryanp)
**	    Added documentation about the table control lock (B38326).
**	13-apr-1992 (bryanp)
**	    B42225 -- truncate 'sequence' to 16 bits, since that's all we have
**	    on the page.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	15-jul-1992 (bryanp)
**	    Set tcb_lalloc properly when building a temp tcb following a modify.
**	26-aug-1992 (25-jan-1990) (fred)
**	    BLOB support:
**	    Added support for the new extented table fields in the tcb
**	    (tcb_et_mutex, tcb_et_{prev,next}.  This is for large objects
**	    support (initially).
**	26-aug-1992 (bryanp)
**	    Temporary Tables bug fixes to dm2t_temp_build_tcb: set Btree key
**	    information (tcb_klen, tcb_kperpage, key attr array, etc.)
**	    correctly.
**	29-August-1992 (rmuth)
**	    - Removed the 6.4->6.5 on the fly conversion code.
**	    - Add extra check to make sure relfhdr is a valid value
**	      before calling dm1p_checkhwm.
**	    - Remove setting temporary files to DM2F_LOAD, always now
**	      call dm1s_deallocate.
**	16-sep-1992 (bryanp)
**	    Move setting of relpgtype and relcomptype for temporary tables out
**	    of dm2t_temp_build_tcb and make its caller set these fields.
**	25-sep-1992 (robf)
**	    Continue cleanup on gateway error, otherwise may get spurious
**	    TCB/RCB left around. (causing "phantom" objects)
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	07-oct-92 (robf)
**	    Special handling on DM2T_DEL_WRITE with gateways. This is
**	    used during rollback processing, and needs to proceed despite
**	    read-only gateways or problems opening the GW physical file.
**	    Symptoms included "phantom" tables left behind after aborts,
**	    or PSF errors indicating table was still in use.
**	16-oct-1992 (rickh)
**	    Added default id transcription for attributes.
**	22-oct-1992 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:  Rewrote most of module!!!!
**	    Added support for Partial TCB's and changed some get-tcb protocols.
**	    Added dm2t_fix_tcb and dm2t_unfix_tcb routines and removed
**	    dm2t_find_tcb, dm2t_hunt_tcb, and dm2t_get_tcb_ptr.
**	    Renamed dm2t_locate_tcb to dm2t_lookup_tabid.
**	    Added new routine dm2t_lock_table to consolidate code and allow
**	    function to be called from recovery modules.
**	    Added new routines that allow dmf routines to request and fix
**	    TABLE_IO control blocks for buffer manager calls:
**	    		dm2t_fx_tabio_cb, dm2t_ufx_tabio_cb.
**	    Added new routines to allow TABLE_IO structure to be built without
**	    a full TCB being initialized (partial tcb):
**			dm2t_init_tabio_cb
**			dm2t_add_tabio_file
**			dm2t_wt_tabio_ptr
**	    Moved code that scans tcb hash queue to new locate_tcb routine.
**	    Created new local routines lock_tcb, unlock_tcb, and bumplock_tcb
**	    to manage tcb validation locks.
**	    Added capability to wait on TCB for other than TCB_BUSY status.
**	    Routines/clients which need exclusive access to a TCB can now
**	    wait for fixers of the tcb to release it.  Unfix operations will
**	    now check for tcb waiters and signal wakeups if appropriate.
**	    Changed arguments to dm2t_release_tcb routine.
**	    Removed showflag and some of the dm2t_open modes.
**      30-October-1992 (rmuth)
**	    - Remove references to file open flag DI_ZERO_ALLOC.
**	    - Set tbio_lalloc, tbio_lpageno and tbio_checkeof correctly.
**    	    - Initialise tbio_tmp_hw_pageno.
**	    - dm2t_temp_build_tcb(), If not building a new temp table make
**	      sure we always call dm2f_set_alloc.
**	09-nov-1992 (kwatts)
**	    Added a call to dm1sd_disktype for all TCBs including
**	    those with no file (VIEW and GATEWAY). In these cases it will
**	    set TCB to have no Smart Disk. Without this change the field was
**	    left undefined. This code is now in build_tcb (was read_tcb)
**	    because of the previous logging changes.
**	15-oct-1992 (bryanp)
**	    Log LK status code when LKrequest fails.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
** 	14-dec-1992 (jnash)
**	    Reduced logging project.
**	      - Eliminate dm2t_rename_file, this work is now done
**		entirely within dm2r.c
**	      - Renamed bumplock_tcb to dm2t_bumplock_tcb and made
**		non-static so that it can be called by dm2r_load.
**	14-dec-1992 (rogerk)
**	    Changed Gateway handling of open calls from purge operations to
**	    check for DM2T_A_OPEN_NOACCESS requests rather than DM2T_DEL_WRITE.
**	    The latter mode is no longer used.
**	14-dec-1992 (rogerk)
**	    Put back checks for iirelation<-->iiindex information mismatches
**	    in build_tcb during rollforward operations.  It is possible
**	    for a table to not be listed as indexed but for there to exist
**	    iirelation rows for an index table that is in the process of being
**	    created.
**	18-jan-1993 (bryanp)
**	    Add dm2t_reserve_tcb() for use by modify of a temporary table.
**	18-jan-1993 (rogerk)
**	    Included CSP in places where special logic was done for RCP
**	    processing since the CSP no longer turns on the special DCB_S_RCP
**	    flag.  Fixed problem with tcb pointers in dm2t_init_tabio_cb when
**	    waiting for exclusive access to the tcb.
**	18-jan-1993 (rogerk)
**	    Bypassed dm1p_checkhwm in dm2t_open call when called in
**	    OPEN_NOACCESS mode.
**	 2-feb-1993 (rogerk)
**	    Initialized tcb_temporary field of tcb when building partial tcb.
**	16-feb-1993 (rogerk)
**	    Fix partial TCB problem in dm2t_release_tcb where we were
**	    mistakenly bypassing the close_page call if no files were open
**          in a partial base table tcb which had open secondary index tcbs.
**	    Added tcb_open_count field to track number of actual openers of
**	    a table rather than the number of fixes of the TCB.  A fix for
**	    a table open is currently recognized by the passing of the
**	    TCB_VERIFY flag.
**	30-feb-1993 (rmuth)
**	    - Multi-location tables, in read_tcb type mean't that we were
**	      checking a db_tab_base with a db_tab_index hence we never
**	      found all the location records in iidevices.
**	    - dm2t_lookup_tabid, make sure the TCB's are valid and not
**	      busy before we look at them.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	    Fix bug in dm2t_awaken_tcb where DM2T_TMPMOD_WAIT used in
**		place of TCB_TMPMOD_WAIT.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		In read_tcb, when scanning iiattribute, set rcb_access_mode
**		    to RCB_A_READ so that we don't fix pages for WRITE.
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables and concurrent indicies.
**	    Concurrent_idx
**		- Make sure build_tcb ignores these indicies as there are
**	          not exposed to the outside world until "modify to noreadonly"
**	          is issued
**	        - In "build_index_info", understand that a tcb's secondary
**		  index list can have less entries than the iiindex table.
**		  Caused by build_tcb ingnoring concurrent indicies.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Don't use dmd_check messages as traceback messages.
**	    Log table name and database name when failing to build a TCB.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	    Fix error message problems in DM9C74 (pass &loc_name, not loc_name).
**      20-jul-1993 (pearl)
**          Bug 49115.  In dm2t_update_logical_key(); after call to dm2r_get(),
**          check the index part of the reltid to make sure we've got the
**          right tuple.
**	26-jul-1993 (rogerk)
**	  - Changed warning given when relloccount found to be zero to only
**	    be printed if the table is not a view.
**	  - Include respective CL headers for all cl calls.
**	23-aug-1993 (mikem)
**          Changed E_DM9383_DM2T_UPDATE_LOGICAL_KEY_ERROR and
**          E_DM9384_DM2T_UPDATE_LOGICAL_KEY_ERROR, to
**          E_DM9383_DM2T_UPD_LOGKEY_ERR and E_DM9384_DM2T_UPD_LOGKEY_ERR to
**          bring them under the 32 character limit imposed by ERcompile().
**	23-aug-1993 (rogerk)
**	    Changed dm2t_purge_tcb to call bumplock_tcb when table can't be
**	    opened even for purges on secondary indexes.
**	    Changed DCB_S_RCP name to DCB_S_ONLINE_RCP since it is specified
**	    only during online recovery.
**	    Changed decisions on whether to update reltups values to be based
**	    on DCB_S_RECOVER mode rather than DCB_S_ONLINE_RCP.
**	20-sep-1993 (rogerk)
**	    Add support for new non-journal table handling.  Check for dummy
**	    iidevices rows in read_tcb.
**	18-oct-1993 (jnash)
**	    Fix bug where temp tables incorrectly opened O_SYNC: open
**	    them with DCB_NOSYNC_MASK.
**	18-oct-1993 (rogerk)
**	    Took out support for PEND_DESTROY tcb state.  This was used for
**	    temporary tables in dm2t_close when the caller had multiple fixes
**	    of the same tcb.  The caller must now wait until the last close of
**	    the tcb to request the destroy.  Added consistency check for
**	    dm2t_destroy_temp_tcb calls when the caller has multiple fixes.
**	10-oct-93 (swm)
**	    Bug #56440
**	    Replaced instances of tcb/dummy_tcb used as lock event values to
**	    dm0s_eset(), dm0s_ewait() and dm0s_erelease() with value from new
**	    tcb_unique_id field. This change is for platforms where the size
**	    of a pointer is larger than the size of a u_i4.
**	9-mar-94 (stephenb)
**	    Alter dma_write_audit() calls to current prototype.
**	23-may-94 (jnash)
**	  - In the RCP and ROLLFORWARDDB, fix all table's buffer manager
**	    priorities, making the DMF cache "flat".  This results in
**	    improved cache utilitization during recoveries, and in the
**	    case of the RCP, lessens the liklihood of encountering "absolute
**	    logfull".
**	  - Add DM433 fix the priority of all tables.
**	18-may-1994 (pearl)
**	    Bug 63428
**	    In dm2t_update_logical_key(), indicate in the iirelation rcb
**	    whether the user table is journaled.  Do this by assigning the
**	    rcb_journal field of the user table rcb to the rcb_usertab_jnl
**	    field of the iirelation rcb.  This is relevant to the log
**	    record, and if the table is journaled, the journal record.
**	20-jun-1994 (jnash)
**	    fsync project.
**	    - Add new DM2T_A_SYNC_CLOSE dm2t_open() access_mode, which
**	      requests that underlying files be opened DI_USER_SYNC_MASK
**	      and forced to disk when closed.  The same is performed on
**	      individual tables when the database is opened sync-at-close.
**	    - Eliminate obsolete tcb_logical_log code.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905 - define dm2t_get_timeout for use by dmu
**          routines to get timeout value for dm2t_open/control calls.
**      22-nov-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Table level rollforwarddb
**                  - When fixing a tcb add open mode DM2T_A_OPEN_RECOVERY.
**                    This mode returns all necessary information about the
**                    table but does not physically open the underlying files.
**                    I.E. they may not be there.
**                  - In dm2t_control() if database is exclusively locked then
**                    don't bother obtaining the lock.
**	09-jan-1995 (nanpr01)
**	    Add support for log_inconsistent, phys_inconsistent support.
**	19-jan-1995 (cohmi01)
**	    In read_tcb(), propagate RAW bit from DCB to DMP_LOCATION.
**	06-mar-1995 (shero03)
**	    Bug B67276
**	    Compare the location usage when opening a table.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ (originally proposed by bryanp)
**      17-may-1995 (dilma04)
**          If a READONLY table, avoid multiple page locks by locking whole
**          table in S mode.
**      19-may-1994 (lewda02/shust01)
**          RAAT API Project:
**          Altered test used to free readnolock pages from RCB.
**	 4-jul-95 (dougb) bug # 69382
**	    Before fixing any pages for a temporary table in
**	    dm2t_temp_build_tcb(), place it into the hcb queue.  This removes
**	    a window during which a consistency point will not be able to find
**	    a TCB for pages in the buffer cache (leading to
**	    E_DM9397_CP_INCOMPLETE_ERROR).
**	21-aug-95 (tchdi01)
**	    Added support for the production mode feature. When a system
**	    catalog is opened for writing, check whether the database is
**	    in the production mode. If it is, do not open the catalog and
**	    return an error. Added a new access mode DM2T_A_SET_PRODUCTION.
**	    The mode opens a system catalog for writing in production
**	    mode.
**	 6-sep-1995 (nick)
**	    Fix broken logic in build_tcb() which had caused concurrent index
**	    creation to break. #70556
**	19-oct-1995 (nick)
**	    Fix dm2t_get_timeout() - it didn't work in all cases. #67054
**	11-jan-1995 (dougb)
**	    Remove debugging code which should not exist in generic files.
**	    (Replace printf() calls with TRdisplay().)
**	13-feb-1996 (canor01)
**	    If database is in production mode, don't take the IX tablelock
**	    in dm2t_open().
**	06-mar-1996 (stial01 for bryanp)
**	    Don't use DM_PG_SIZE, use relpgsize instead.
**	    If relpgsize is 0 in iirelation, force it to be DM_PG_SIZE.
**	    Add page_size argument to dm2t_init_tabio_cb and dm2t_add_tabio_file
**		for managing a table's page size during recovery.
**      06-mar-1996 (stial01)
**          Variable Page Size:
**          build_tcb() Deny access to a table if there are no buffers
**          for the page size for that table.
**          build_tcb() Deny access to a table if the table width is > than
**          the maximum tuple width currently defined for the installation.
**          Call dm2f_add_fcb with tbio_page_size
**          Limit tcb_kperpage to (DM_TIDEOF + 1) - DM1B_OFFSEQ
**          Pass page size to dm2f_build_fcb()
**	08-may-96 (nick)
**	    Fix misleading comments ; dm2t_fx_tabio_cb() never builds a TCB.
**	17-may-96 (stephenb)
**	    Add replication code, check for replication at table open and
**	    open some replication support tables. Check for replication
**	    when building the TCB and add replication info to TCB for
**	    later use. also add get_supp_tabid() static routine.
**	25-jun-1996 (shero03)
**	    RTree project:  support integer and float ranges
**	 4-jul-96 (nick)
**	    Ensure rcb_k_mode is set to IS lock where necessary ; the default
**	    is IX and caused us to lock catalogs queried herein exclusively.
**	10-jul-96 (nick)
**	    Explicit opens of core catalogs forced the lock mode to page level
**	    exclusive when page level shared was more appropriate. #77614
**	17-jul-96 (pchang)
**	    Fixed bug 76526.  In dm2t_temp_build_tcb(), when setting unwritten
**	    allocation in the tbio of a temporary table that has spilled into a
**	    real file, we were incorrectly adding mct_allocated to tbio_lalloc
**	    instead of setting tbio_lalloc to mct_allocate - 1.
**      22-jul-1996 (ramra01 for bryanp)
**          For Alter Table support, set new DB_ATTS fields from corresponding
**              values in iiattribute records.
**          Set up tcb_total_atts, and complete attribute information.
**          Set rcb_readlock_mode when allocating a read-only RCB.
**	14-aug-96 (stephenb)
**	    When doing replicator work in dm2t_open, check wether table
**	    is currently registered for replication.
**	21-aug-96 (stephenb)
**	    Add code for replicator cdds lookup table
**	23-aug-96 (nick)
**	    Ensure we catch the fact that a base table has an inconsistent
**	    secondary index ;  base table access is prohibited until this
**	    is either dropped or recreated.
**	20-sep-96 (kch)
**	    In dm2t_temp_build_tcb(), we now check for a work location or a
**	    data location when building the extent information for the table's
**	    location(s). This change fixes bug 78060.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced support for Distributed Multi-Cache Management (DMCM)
**          protocol :-
**            - All TCB lock requests made via LKrequestWithCallback interface
**              supplying dm2t_tcb_callback as the blocking callback function.
**            - TCBs locked at LK_S when fixed.
**            - TCBs marked TCB_VALIDATED when locked at LK_S.
**            - verify_tcb skipped if TCB marked TCB_VALIDATED.
**            - TCBs released with DM2T_FLUSH flag are retained in cache with
**              a Null lock.
**            - Call to dm2t_xlock_tcb requests an LK_X TCB lock on the server
**              locklist, causing all other caches to flush this TCB and all
**              its dirty pages.
**      23-oct-1996 (hanch04)
**          During an upgrade, attintl_id will not exist.
**          Use attid for the default.
**	24-Oct-1996 (somsa01)
**	    In dm2t_open(), set the locking scheme for extended tables as
**	    physical page locks. This is because there will be concurrent
**	    access to extended tables.
**	12-dec-1996 (cohmi01)
**	    Add detailed err messages for corrupt system catalogs and
**	    lock related errors. (Bug 79763).
**	28-feb-1997 (cohmi01)
**	    Keep track of tcbs, reclaim when limit reached. Bugs 80242, 80050.
**	03-mar-1997 (sarjo01)
**	    Bug 75355: Assure that all temporary database files created
**	    for a COPY FROM load operation are closed. On NT, UNDOing
**	    (deleting) these files when they are open causes UNDO to fail.
**	24-Oct-1996 (jenjo02)
**	    Create tcb_mutex, tcb_et_mutex names using table name to uniquify
**	    for iimonitor, sampler.
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	    Added **hash to locate_tcb() prototype into which the pointer to
**	    the hash_entry will be returned.
**	    Added *hash to build_tcb() prototype in which caller must pass
**	    the TCB's hash bucket.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**            - dm2t_open() Initialize lock information for row locking;
**            - dm2t_open() If DM716 is set, force row locking.
**      12-dec-96 (dilma04)
**          In dm2t_open() move grant_mode initialization to the right place.
**	07-jan-1997 (canor01)
**	    For production mode, we must take the IX tablelock in
**	    dm2t_open() (which was removed in change of 13-feb-1996).
**	    We can avoid taking the control lock.
**	22-Jan-1997 (jenjo02)
**	    Added dm2t_alloc_tcb() to consolidate the multitude of places
**	    which allocate and initialize TCBs. Remove DM0M_ZERO flag from
**	    calls to dm0m_allocate() for TCBs.
**	23-jan-96 (stephenb)
**	    Add rep_catalog() routine and Rep_cats array to determin wether
**	    given table is a replicator catalog. Use routine un build_tcb()
**	    to ensure we don't replicate replicator catalogs
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change dm2t_open(), dm2t_close(), dm2t_get_timeout() and
**          get_supp_tabid() to match the new way of storing and setting
**          of locking characteristics.
**	24-Feb-1997 (jenjo02 for somsa01)
**	    Cross-integrated 428939 from main.
**	    Added initialization of tbio_extended.
**	11-Mar-1997 (jenjo02)
**	    Temp tables are always writable. Set rcb_access_mode = RCB_A_WRITE
**	    in dm2t_open.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	  - Table priority is optionally set when table is created or modified,
**	    defaulting to 0 (no priority). Removed call to dm0p_gtbl_priority().
**	  - Replaced BMCB_MAX_PRI with DB_MAX_TABLEPRI.
**	28-Mar-1997 (wonst02)
**	    Temp tables can be written, even to a "readonly" database.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - add support for trace points DM717 and DM718 to default
**          isolation level to CS and RR respectively;
**          - allow CS and RR isolation for user tables only;
**          - release all uneeded locks when closing a table.
**	15-apr-1997 (shero03)
**	    Support 3D NBR in R-tree
**      09-apr-97 (cohmi01)
**          Allow timeout system default to be configured. #74045, #8616.
**      16-may-97 (dilma04)
**          In dm2t_get_timeout(), removed unneeded check that did not allow 
**          user to set default timeout on a table, if session level timeout 
**          was not default.        
**      21-May-1997 (stial01)
**          Enable row locking for tables with blobs, disable for temp tables
**      21-May-1997 (stial01)
**          For compatibility reasons, DO include TIDP for V1-2k unique indexes
**              (2k unique indexes that do not include TIDP in key must
**               be 5.0 indexes, in which case we do not include TIDP in 
**               tcb_klen.
**          V2 (4k+) unique indexes do not include tidp in key however,
**          tidp should be included in tcb_klen.
**      03-Jun-97 (hanal04)
**          Disable group reads for temporary tables which span multiple
**          locations. This prevents attempts to read pages that do not
**          exist. (Pr# 68 / Bug# 82762 which is similar to Bug# 72806).
**      12-jun-97 (stial01)
**          Include attribute information in kperpage calculation
**      13-jun-97 (dilma04)
**          Moved code to enforce isolation levels from dm2t_open() to
**          dmt_set_lock_values() in dmtopen.c where it belongs to. 
**          Cursor Stability/Repeatable Read optimizations in dm2t_close():
**          - turn on CSRR locking mode, if isolation level is CS or RR. This
**          allows dm0p_unfix_page() set flags to release page locks;
**          - release leaf page's lock if page has not been updated.
**	18-jun-1997 (wonst02)
** 	    A readonly database has only one location. Do not compare logical 
**	    names in read_tcb, because they will be different.
**	29-jul-1997 (wonst02)
**	    In build_tcb, if dcb access is "read", open table for read-only.
**	15-aug-97 (stephenb)
**	    replicator support for SQL92 case semantics, add upper case
**	    catalog names to Rep_cats, add code to cope with upper case
**	    catalog names.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb(), dm2t_release_tcb(),
**	    dm2t_reclaim_tcb(), dm2t_destroy_temp_tcb(), dm2t_ufx_tabio_cb(),
**	    dm2t_add_tabio_file(), dm2t_init_tabio_cb() prototypes 
**	    to be passed eventually to dm0p_close_page().
**      29-Aug-1997 (horda03,thaal01)
**          ifdef VMS around ule_format() function due to the different structure
**          of CL_ERR_DESC on VMS.Temporary fix !!
**	01-sep-1997 (ocoro02)
**	    Several ule_format calls in dm2t_control & dm2t_lock_table were
**	    passing the value of tablename & ownername instead of the
**	    address. This caused garbled error text (#84248).
**      11-sep-97 (thaju02)
**          bug#81340 - Modified dm2t_open(). With readlock=nolock, table 
**	    control lock on extension table is not released. If extension 
**	    table and readlock=nolock do not setup to use physical page 
**	    locks, instead fall thru to else if lock_duration == 
**	    DM2T_NOREADLOCK and allocate rnl_cb.
**      22-sep-1997 (matbe01)
**          Added 10 bytes to sem_name in dm2t_alloc_tcb and in dm2t_init_
**          tabio_cb to eliminate the overflow of STprintf when table names
**          are over 24 bytes in length.
**      14-oct-97 (stial01)
**          dm2t_close() Changes to distinguish readlock value/isolation level
**	28-Oct-1997 (jenjo02)
**	    Add roundup size to key_atts_size in dm2t_alloc_tcb().
**      12-nov-97 (stial01)
**          Pass lockKEY even if unlocking with lockID (for set lock_trace)
**          Consolidated rcb fields into rcb_csrr_flags.
**      20-jan-98 (stial01)
**          dm2t_open() Disable row locking from RTREE indexes.
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. Modified dm2t_temp_build_tcb.
**      26-mar-98 (shust01)
**          Reset SVCB_WB_FUTILITY bit in svcb_status when tcb is created. This
**          will have the write-behind threads for this server wake up.
**          Bug 88988.
**      21-apr-98 (stial01)
**          Set tcb_extended if this is an extension table (B90677)
**          Disable row locking for extension tables for which we do temporary 
**          physical page locking.       
**	06-May-1998 (jenjo02)
**	    Protect hcb_tcbcount with hcb_tq_mutex to guarantee its accuracy.
**	    Set tbio_cache_ix when initializing a TCB, track TCB allocation
**	    counts in HCB by cache.
**	18-may-1998(angusm)
**	    Bug 83251: reset *err_code when tcb successfully fixed,
**	    else the W_DM9C50 warning can propagate up the error chain
**	    nd cause intermittent failures of various queries.
**	29-May-1998 (shust01)
**	    In dm2t_add_tabio_file(), if the location that we are trying to
**	    add is already in the TABLE_IO cb, we should just return. What
**	    was happening was status was set to OK, we break out of for loop,
**	    and fall into error handler, which returns E_DM9C71. bug 91079.
**      28-may-1998 (stial01)
**          dm2t_open() Non-core catalogs default to row locking if possible.
**          Moved code to dm2t_row_lock()
**	23-Jun-1998 (jenjo02)
**	    Added LK_VALUE parm to dm0p_unlock_page() prototype.
**	18-aug-1998 (somsa01)
**	    In dm2t_alloc_tcb(), etl_status (which hangs off of the
**	    tcb_et_list) needs to be initialized to zero.
**      23-Jul-1998 (wanya01) 
**          X-integrate change 432896(sarjo01)
**          in dm2t_update_logical_key(). 
**          Bug 75355: Assure that all temporary database files created
**          for a COPY FROM load operation are closed. On NT, UNDOing
**          (deleting) these files when they are open causes UNDO to fail.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-99 (stial01)
**          Pass relcmptlvl to dm1*_kperpage
**	19-feb-99 (nanpr01)
**	    Support update mode locking.
**	19-mar-99 (nanpr01)
**	    Support raw location.
** 	25-mar-1999 (wonst02)
** 	    Fix RTREE bug B95350 in Rel. 2.5...
** 	    Added includes for missing function prototypes. The call to
** 	    dm1m_kperpage() had too many parameters, which passed erroneous data.
**      12-apr-1999 (stial01)
**          Btree secondary index: non-key fields only on leaf pages
**          Different att/key info for LEAF vs. INDEX pages
**          Changed parameters to dm2t_alloc_tcb.
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**	23-Jun-1999 (jenjo02)
**	    Added 2 external functions for GatherWrite support, 
**	    dm2t_pin_tabio_cb(), dm2t_unpin_tabio_cb().
**	18-Aug-1998 (consi01) Bug 91837 Problem INGSRV 502
**	     Extend the fix for bug 78060 in dm2t_temp_build_tcb() to allow
**	     the use of auxilary work locations as well as work and data 
**	     locations.
**	05-nov-1998 (devjo01)
**	    XIP stial01 change 438294. (possible SEGV from ref through
**	    RCB structure already put on free list.   Also cleaned
**	    function to consistently use 'r' instead of 'r' and 'rcb'
**	    in an arbitrary mismash.
**	08-jul-1999 (hanch04)
**	    Correct prototype for dma_write_audit
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. Also store the iirelation page's lsn
**	    to figure out when the TCB was built.
**      31-aug-1999 (stial01)
**          Init tcb_et_dmpe_len
**	12-oct-1999 (gygro01)
**	    Changed lockmode from exclusive to shared for replicator
**	    lookup tables (horizontal and priority). There is only reading
**	    done on this tables during data capture, so exclusive locks are
**	    not necessary. This resolves contentions problems. 
**	    Bug 99176/pb ingrep 66.
**      21-oct-99 (stial01)
**          dm2t_open() Row locking for catalogs only if grant mode not table.
**      22-oct-1999 (nanpr01)
**          Calculate the tuple per page at the table open time and store it
**	    in tcb.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	16-Nov-1999 (jenjo02)
**	    Include test of TCB_BUSY when deciding to wait for a TCB
**	    prior to calling dm2t_release_tcb(). dm2t_release_tcb()
**	    sets TCB_BUSY and releases the protecting mutex even
**	    when tcb_valid_count and tcb_ref_count are zero.
**      08-dec-1999 (stial01)
**          dmt2_open() Force isolation level SERIALIZABLE for etabs
**      21-dec-1999 (stial01)
**          build_tcb() add page cache if necesary for fastload (etab)
**	17-dec-99 (gupsh01)
**	    Changed the error returned to E_DM006A_TRAN_ACCESS_CONFLICT instead 
**	    of E_DM003E_DB_ACCESS_CONFLICT for readonly databases.
**      09-feb-2000 (stial01)
**          dm2t_close() test RCB_UPD_LOCK before downgrade, skip errors B100424
**	05-May-2000 (jenjo02)
**	    Save lengths of blank-stripped table and owner names in
**	    tcb_relid_len, tcb_relowner_len when constructing TCB so that
**	    LAR doesn't have to constantly recalculate these values.
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb, and initialization of 
**          att/key arrays in init_bt_si_tcb() (b102026) 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (hanch04)
**	    Fix for bug 101920 removed code needed to upgrade a database.
**	    Re-add the code until a new fix.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	08-Nov-2000 (jenjo02)
**	    When releasing/returning an RCB for an updated
**	    session temp table, update savepoint id in
**	    table's pending destroy XCCB.
**	17-Nov-2000 (hanch04)
**	    Code that was re-addd for bug 101920 had a type of = should be ==
**	18-Dec-2000 (wansh01)
**	    Code in line 12477  if (access_mode = DM2T_A_READ) should be ==
**          not =                            
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-feb-2001 (stial01)
**          Row locking depends on page type (Variable Page Types SIR 102955)
**      31-jan-2001 (stial01)
**	    dm2t_alloc_tcb() remove temp code mistakenly added on 08-Nov-2000
**      01-may-2001 (stial01)
**          Init adf_ucollation, rcb_ucollation
**      12-Jun-2001 (horda03)
**          In the port for 1.2 to 2.0 something has been lost to the change
**          I made on 29-Aug-1997. But anyhow, things have moved on, and now
**          VMS has the missing fields within the CL_ERR_DESC structure.
**          The temporary VMS fix can now be removed, which fixes Bug 104932
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call
**          in dm2t_update_logical_key() routine.
**	01-Feb-2002 (jenjo02)
**	    Ensure that TCB mutexes are destroyed before freeing TCB
**	    memory.
**	28-Feb-2002 (jenjo02)
**	    Removed LK_MULTITHREAD from LKrequest/release flags. 
**	    MULTITHREAD is now an attribute of the lock list.
**	20-Aug-2002 (jenjo02)
**	    Support TIMEOUT=NOWAIT, new LK_UBUSY status from LKrequest()
**	    for TABLE and CONTROL locks.
**	    Write E_DM9066_LOCK_BUSY info message if DMZ_SES_MACRO(35).
**	    Return E_DM006B_NOWAIT_LOCK_BUSY.
**	07-nov-2002 (somsa01)
**	    In dm2t_close(), make sure we close off any load table files
**	    too.
**	26-nov-2002 (somsa01)
**	    Amended last change to close off any temporary load table
**	    files if mct_recreate is TRUE.
**	13-Dec-2002 (jenjo02)
**	    Maintain hcb_cache_tcbcount for Index TCBs as well as
**	    for base TCBs (BUG 107628).
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**      18-mar-2003 (stial01)
**          dm2t_add_tabio_file() CSsuspend if retrying... (b107375)
**      30-may-2003 (stial01)
**          Always use table locking for DMPE Temp tables (b110331)
**	23-Jul-2003 (horda03) Bug 110266 
**	    E_DM937E reported by a session trying to obtain a new TCB from
**          the free queue, is a possible situation. So rather than return
**          the TCB to the free Queue, leave it alone. The session that
**          made the TCB busy will add the TCB to the free list when it has
**          finished with the table. This prevents subsequent SIGSEGVs and
**          Inconsistent DBs.
**          Also, tidied up the code to prevent compiler warnings.
**      15-sep-2003 (stial01)
**          dm2t_open() Don't force PHYS page lock protocols for etabs when
**          lock level TABLE has been specified to the open (b110923)
**      16-sep-2003 (stial01)
**          dm2t_open() Table locking for HEAP etabs
**          dm2t_close() call dm2f_galloc_file for heap etabs (SIR 110923)
**      18-sep-2003 (stial01)
**          dm2t_close() fix up deferred call to dm2f_galloc_file
**	10-nov-2003 (kinte01)
**	    Add include of ex.h since an exdelete call has been added with
**	    change 465453. Without the inclusion of the header file, on 
**	    VMS an undefined symbol for EXdelete is seen when linking
**      02-jan-2004 (stial01)
**          Removed galloc code for HEAP etabs (temporary trace point dm722)
**      02-jan-2004 (huazh01/thaju02)
**          Modify the fix for 110331. Extend the fix of 
**          b81340 to cover the case of openning a temp table
**          with readlock = nolock.
**          bug 110857, INGSRV2509.
**	14-Jan-2004 (schka24)
**	    Additions for partitioned tables project.
**	    Build the PP array for masters;  build partition TCB's from the
**	    master without looking at iiattribute.
**	    Revise the hash mutexing so that masters and partitions are known
**	    to share a hash mutex.  Revise iirelation handling so that updating
**	    is done by tid.  (Jon's idea, not mine!)
**	6-Feb-2004 (schka24)
**	    Thread-safe incrementing of tcb unique id.
**	17-Feb-2004 (schka24)
**	    Partitioned master row/page counting wasn't right, fix.
**	5-Mar-2004 (schka24)
**	    Teach release-tcb to take TCB off the free list, so that all
**	    the callers don't have to.  (One wasn't.)
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**          build_tcb: Don't restrict access to table if relwid > svcb_maxtuplen
**      22-jun-2004 (stial01)
**          Disable row locking if DMPP_VPT_PAGE_HAS_SEGMENTS 
**      8-jul-2004 (stial01)
**          Alloc additional readnolock buffer if DMPP_VPT_PAGE_HAS_SEGMENTS
**      07-feb-2005 (horda03) Bug 113860/INGSRV3150
**          When the number of pages in a base table changes by a "significant"
**          amount (I'm using the same calculation as for invalidating QEPs)
**          then need to inform other servers who are accessing the table
**          that they need to upgrade their page counts. This is done by
**          marking the TCB as invalid.
**          The new number of pages in the table, may cause a recalculation of
**          a REPEATED QUERY's QEP.
**      9-feb-2005 (stial01)
**          init_bt_si_tcb() fix tcb_ixklen initialization (b113870)
**	15-Mar-2005 (schka24)
**	    Add "show only" support for TCB's that can be used for show,
**	    but not for real table access (e.g. due to proper size cache
**	    not being available).
**      10-aug-2006 (horda03) Bug 114498/INGSRV3299
**          Added tcb status TCB_BEING_RELEASED to signify that the
**          TCB is being reclaimed for used for another table. This
**          TCB should not be "located".
**	15-Aug-2005 (jenjo02)
**	    Don't count partition TCBs in hcb_tcbcount; they quickly
**	    swamp hcb_tcblimit, preventing real tables from
**	    opening (NOFREE_TCB).
**	31-Aug-2006 (jonj)
**	    Added dm2tInvalidateTCB().
**      07-Sep-2006 (stial01)
**          build_tcb() add dummy tcb using base reltid. (b116591)
**      11-Sep-2006 (horda03) Bug 116613
**          Fix for bug 114498 could cause the DBMS to spin if the
**          server is using Internal Threads, and the thread which
**          is waiting for the TCB_BEING_RELEASED to be cleared is
**          a higher priority thread than the thread that set the flag.
**      29-sep-2006 (stial01)
**          Support multiple RANGE formats: OLD format:leafkey,NEW format:ixkey
**      07-nov-2007 (stial01)
**          Use DMPP_TPERPAGE_MACRO to get tcb_tperpage (b119431)
**	19-Feb-2008 (kschendel) SIR 122739
**	    Reorganize tuple/key access stuff into the DMP_ROWACCESS.
**	    The row-accessors have to be built with the TCB.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**      27-Jun-2008 (ashco01) Bug 120565
**          In dm2t_close() check that the table is not a gateway table
**          before attempting to unlock it.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_?, dmse_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1s_?, dm1r_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dmf_gw?, dm0p_? functions converted to DB_ERROR *
**      08-jan-2009 (stial01)
**          rep_catalog() compare name with unpadded strings in Rep_cats
**      17-mar-2009 (stial01)
**          If building tcb sem name, don't assume CS_SEM_NAME_LEN >= DB_MAXNAME
**	25-Aug-2009 (kschendel) 121804
**	    Need dm2d.h and dma.h to satisfy gcc 4.3.
**	17-Nov-2009 (kschendel) SIR 122890
**	    DO count partition TCB's towards the TCB limit.  Users will
**	    simply have to set tcb_limit high enough.  Without this
**	    change, there is no way to control DMF memory in the presence
**	    of lots of partitioned tables.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	01-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**      10-Feb-2010 (maspa05) bug 122651, trac 442
**          set tcb_sysrel for TCB2_PHYSLOCK_CONCUR tables so that physical 
**          locks are used consistently - they were being used during creation 
**          but not rollback. 
**          Change tests for TCB_CONCUR (for locking) to TCB2_PHYSLOCK_CONCUR
**          as all TCB_CONCUR now have TCB2_PHYSLOCK_CONCUR as well
**      14-May-2010 (stial01)
**          Alloc/maintain exact size of column names (iirelation.relattnametot)
*/

GLOBALREF	DMC_CRYPT	*Dmc_crypt;

static VOID		locate_tcb(
				i4		db_id,
				DB_TAB_ID	*table_id,
				DMP_TCB		**tcb,
				DMP_HASH_ENTRY  **hash,
				DM_MUTEX	**hash_mutex);

static DB_STATUS	build_tcb(
				DMP_DCB		*dcb,
				DB_TAB_ID	*table_id,
				DB_TRAN_ID      *tran_id,
				DMP_HASH_ENTRY  *hash,
				DM_MUTEX	*hash_mutex,
				i4         lock_id,
				i4         log_id,
                                i4         sync_flag,
				i4		flags,
				DMP_TCB		**tcb,
				DB_ERROR	*dberr);

static DB_STATUS	find_pp_for_partition(
				DMP_DCB		*dcb,
				DB_TAB_ID	*table_id,
				DMP_TCB		**master_tcb,
				DMT_PHYS_PART	**pp_ptr);

static void		copy_from_master(
				DMP_TCB		*tcb,
				DMP_TCB		*master_tcb);

static DB_STATUS	read_tcb(
				DMP_TCB		*tcb,
				DMP_RELATION	*rel,
				DMP_DCB		*dcb,
				DB_TRAN_ID      *tran_id,
				i4         lock_id,
				i4         log_id,
                                i4         flag,
				DB_ERROR	*dberr);

static DB_STATUS	build_index_info(
				DMP_TCB		*base_tcb,
				DMP_TCB		**index_list,
				DB_TRAN_ID      *tran_id,
				i4         lock_id,
				i4         log_id,
				bool		parallel_idx,
				DB_ERROR	*dberr);

static DB_STATUS	verify_tcb(
				DMP_TCB		*tcb,
				DB_ERROR	*dberr);

static DB_STATUS	update_rel(
				DMP_TCB		*tcb,
				i4		lock_list,
				bool		*InvalidateTCB,
				DB_ERROR	*dberr);

static DB_STATUS	open_tabio_cb(
				DMP_TABLE_IO	*tabio,
				i4         lock_id,
				i4         log_id,
				i4		dm2f_flags,
				i4		dbio_flag,
				DB_ERROR	*dberr);

static DB_STATUS	lock_tcb(
				DMP_TCB		*tcb,
				DB_ERROR	*dberr);

static DB_STATUS	unlock_tcb(
				DMP_TCB		*tcb,
				DB_ERROR	*dberr);

static DB_STATUS 	get_supp_tabid(
				DMP_DCB		*dcb,
				DB_TAB_NAME	*shad_name,
				DB_TAB_NAME	*arch_name,
				DB_TAB_NAME	*shadix_name,
				DB_TAB_NAME	*prio_lookup_name,
				DB_TAB_NAME	*cdds_lookup_name,
				DB_OWN_NAME	*table_owner,
				DB_TAB_ID	*shad_id,
				DB_TAB_ID	*arch_id,
				DB_TAB_ID	*shadix_id,
				DB_TAB_ID	*prio_id,
				DB_TAB_ID	*cdds_id,
				DB_TRAN_ID	*tran_id,
				i4		lock_id,
				i4		log_id,
				DB_ERROR	*dberr);
static bool		rep_catalog(
				char	*tabname);
static i4		rep_lockmode(
				DML_SCB		*scb,
				DB_TAB_ID	*table_id,
				i4		lockmode,
				i4		access_mode);

static VOID             init_bt_si_tcb(
				DMP_TCB         *t,
				bool		*leaf_setup);

static DB_STATUS	dm2t_convert_tcb_lock(
				DMP_TCB		*tcb,
				DB_ERROR	*dberr);

static DB_STATUS dm2t_mvcc_lock(
				DMP_DCB		*dcb,
				DB_TAB_ID	*table_id,
				i4		lock_list,
				i4		lock_mode,
				i4		timeout,
				LK_LKID		*lockid,
				DB_ERROR	*dberr);

#ifdef xDEBUG
static void debug_tcb_free_list(char *str,DMP_TCB *the_tcb);
#else
#define debug_tcb_free_list(a,b) /* dummy */
#endif

static READONLY struct 
  {
	char		*namestr;
	i4		namelen;
  }  Rep_cats[] =
{
    { DD_CDDS,			DD_CDDS_SIZE },
    { DD_CDDS_NAME_IDX,		DD_CDDS_NAME_IDX_SIZE },
    { DD_DATABASES,		DD_DATABASES_SIZE },
    { DD_DB_CDDS,		DD_DB_CDDS_SIZE },
    { DD_DB_NAME_IDX,		DD_DB_NAME_IDX_SIZE },
    { DD_DBMS_TYPES,		DD_DBMS_TYPES_SIZE },
    { DD_DISTRIB_QUEUE,		DD_DISTRIB_QUEUE_SIZE },
    { DD_EVENTS,		DD_EVENTS_SIZE },
    { DD_FLAG_VALUES,		DD_FLAG_VALUES_SIZE },
    { DD_INPUT_QUEUE,		DD_INPUT_QUEUE_SIZE },
    { DD_LAST_NUMBER,		DD_LAST_NUMBER_SIZE },
    { DD_MAIL_ALERT,		DD_MAIL_ALERT_SIZE },
    { DD_NODES,			DD_NODES_SIZE },
    { DD_OPTION_VALUES,		DD_OPTION_VALUES_SIZE },
    { DD_PATHS,			DD_PATHS_SIZE },
    { DD_REG_TBL_IDX,		DD_REG_TBL_IDX_SIZE },
    { DD_REGIST_COLUMNS,	DD_REGIST_COLUMNS_SIZE },
    { DD_REGIST_TABLES,		DD_REGIST_TABLES_SIZE },
    { DD_SERVER_FLAGS,		DD_SERVER_FLAGS_SIZE },
    { DD_SERVERS,		DD_SERVERS_SIZE },
    { DD_SUPPORT_TABLES,	DD_SUPPORT_TABLES_SIZE },
    { DD_CDDS_U,		DD_CDDS_SIZE },
    { DD_CDDS_NAME_IDX_U,	DD_CDDS_NAME_IDX_SIZE },
    { DD_DATABASES_U,		DD_DATABASES_SIZE },
    { DD_DB_CDDS_U,		DD_DB_CDDS_SIZE },
    { DD_DB_NAME_IDX_U,		DD_DB_NAME_IDX_SIZE },
    { DD_DBMS_TYPES_U,		DD_DBMS_TYPES_SIZE },
    { DD_DISTRIB_QUEUE_U,	DD_DISTRIB_QUEUE_SIZE },
    { DD_EVENTS_U,		DD_EVENTS_SIZE },
    { DD_FLAG_VALUES_U,		DD_FLAG_VALUES_SIZE },
    { DD_INPUT_QUEUE_U,		DD_INPUT_QUEUE_SIZE },
    { DD_LAST_NUMBER_U,		DD_LAST_NUMBER_SIZE },
    { DD_MAIL_ALERT_U,		DD_MAIL_ALERT_SIZE },
    { DD_NODES_U,		DD_NODES_SIZE },
    { DD_OPTION_VALUES_U,	DD_OPTION_VALUES_SIZE },
    { DD_PATHS_U,		DD_PATHS_SIZE },
    { DD_REG_TBL_IDX_U,		DD_REG_TBL_IDX_SIZE },
    { DD_REGIST_COLUMNS_U,	DD_REGIST_COLUMNS_SIZE },
    { DD_REGIST_TABLES_U,	DD_REGIST_TABLES_SIZE },
    { DD_SERVER_FLAGS_U,	DD_SERVER_FLAGS_SIZE },
    { DD_SERVERS_U,		DD_SERVERS_SIZE },
    { DD_SUPPORT_TABLES_U,	DD_SUPPORT_TABLES_SIZE },
    {0,0} 
};


/*{
** Name: dm2t_open      - Open a table.
**
** Description:
**      This routine opens a table for a transaction.  It returns a
**	Record Control Block (RCB) which can be used to access the table.
**
**	The actual table descriptor (TCB) is fixed in the DMF Table Cache.
**	While fixed by the caller, the TCB cannot be removed from the cache.
**
**	The table is locked for the caller according to the requested lock
**	mode (and implied type):
**
**		DM2T_X		Table Lock Mode, private write access
**		DM2T_S		Table Lock Mode, shared read access
**		DM2T_IX		Page Lock Mode, shared write access
**		DM2T_IS		Page Lock Mode, shared read access
**
**	If X or S mode is specified, no page locks will be obtained during
**	subsequent accesses to the table.  Table locks are released when the
**	transaction completes (not when the table is closed).
**
**	If the TCB for the requested table does not yet exist in the DMF
**	table cache then it is built from system catalog information and
**	its physical files are opened.  TCB's are shared by all sessions
**	of the server and may be in use concurrently by more than one
**	transaction.
**
**	The server uses cache locks to maintain cache consistency across
**	servers when multiple servers share the same databases.  If running
**	in this mode, a TCB's cache lock must be validated whenever a
**	requested table is found unfixed in the cache.
**
**	Physical partition tables can only be opened if their partitioned
**	master has also been opened.
**
** Inputs:
**      dcb                             Database Control Block.
**      table_id                        Table identifier for table to be opened.
**      lock_mode                       Table lock mode:
**					    DM2T_X  - Exclusive (table level)
**					    DM2T_S  - Shared (table level)
**					    DM2T_IS - Intended Shared (Page)
**					    DM2T_IX - Intended Exclusive (Page)
**	update_mode			Defines cursor update semantics:
**					    DM2U_UDIRECT or DM2T_UDEFERRED
**	req_acces_mode			Must be one of:
**					    DM2T_A_READ   - read access
**					    DM2T_A_WRITE  - write access
**					    DM2T_A_MODIFY - open for Modify
**					    DM2T_A_OPEN_NOACCESS - open for
**						administrative purposes only,
**						the table cannot be accessed.
**						(This implies that a show-only
**						TCB is acceptable.)
**					    DM2T_A_SYNC_CLOSE - open
**					        files DI_USER_SYNC_MASK,
**						sync during dm2t_close.
**					    DM2T_A_SET_PRODUCTION - set
**						production mode.
**	timeout				Number of seconds after which to cancel
**					table lock request (zero is no timeout).
**	maxlocks			Number of page locks which should be
**					granted during access to the table
**					before escalating to table locking.
**	sp_id				Last Savepoint operation in this xact.
**	log_id				Logging System handle
**	lock_id				Lock List handle
**	sequence			Query Statement Number - used for
**					deferred udpate protocols.
**      iso_level		        Isolation Level which is one of:
**                                          DMC_C_READ_UNCOMMITTED
**                                          DMC_C_READ_COMMITTED
**                                          DMC_C_REPEATABLE_READ
**                                          DMC_C_SERIALIZABLE
**                                          DMC_C_READ_EXCLUSIVE
**	db_lockmode			The database lockmode.
**	tran_id				The current transaction.
**
** Outputs:
**      timestamp                       Table timestamp - specifies the last
**					time the table structure or layout
**					was changed.
**      rcb                             The RCB returned for this opened table.
**      err_code                        Reason for error return status.
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	31-dec-1985 (derek)
**          Created new for jupiter.
**	31-mar-1987 (ac)
**	    Add read/nolock support.
**	27-Oct-1988 (teg)
**	    put hooks in to support opening a table (for delete, specified
**	    by req_access_mode = DM2T_DEL_WRITE) with no assoc data file.
**	14-apr-1989 (mikem)
**	    Logical key development.  Initialize tcb_{high,low}_logical_key
**	    fields to 0.
**	12-may-1989 (anton)
**	    Put collation descriptor in rcb
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  When opening RCB for gateway
**	    table, call gateway to open the foreign table.
**	14-jun-90 (linda)
**	    Add support for access mode DM2T_A_OPEN_NOACCESS, indicating that
**	    the only operation which can legally be performed is table close.
**	    Used by gateways only for now, supports remove table where
**	    underlying data file does not exist.
**	18-jun-90 (linda, bryanp)
**	    New flag to dm2t_open -- DM2T_A_MODIFY, which for Ingres tables is
**	    treated the same as DM2T_A_WRITE, but for gateway tables allows us
**	    to proceed even if the underlying file does not exist (we go through
**	    dm2u_modify only to set system catalog information).
**	17-jul-90 (mikem)
**	    bug #30833 - lost force aborts
**	    LKrequest has been changed to return a new error status
**	    (LK_INTR_GRANT) when a lock has been granted at the same time as
**	    an interrupt arrives (the interrupt that caused the problem was
**	    a FORCE ABORT).   Callers of LKrequest which can be interrupted
**	    must deal with this new error return and take the appropriate
**	    action.  Appropriate action includes signaling the interrupt
**	    up to SCF, and possibly releasing the lock that was granted.
**	    dm2t_open() already cleaned up locks in the error case, so just
**	    needed to be changed to recognize the new LK error return.
**	19-nov-90 (rogerk)
**	    Added adf control block to the RCB.  This takes away necessity
**	    for most DMF routines to allocate adf cb's on the stack.
**	    Set collation sequence value in the adf control block here
**	    rather than locally in all DMF places before calling ADF.
**	    This was done as part of the DBMS Timezone changes.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added file extend changes.  Added support for the tcb_checkeof
**		flag.  This indicates that we should update the tcb allocation
**		fields to find out the true EOF.
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	25-sep-1992 (robf)
**	    Handle Read-only gateways opened in WRITE mode. Previously this
**	    was silently turned into READ mode, which causes later
**	    (non-obvious) errors. Changed to return gateway access error
**	    instead.
**	07-oct-92 (robf)
**	    Special handling on DM2T_DEL_WRITE with gateways. This is
**	    used during rollback processing, and needs to proceed despite
**	    read-only gateways or problems opening the GW physical file.
**	    Symptoms included "phantom" tables left behind after aborts,
**	    or PSF errors indicating table was still in use.
**	14-nov-1992 (rogerk)
**	    Rewritten as part of the 6.5 Recovery Project.
**	      - Added dm2t_fix_tcb and dm2t_lock_table calls.
**	      - Changed handling of table lock to not escalate table access
**		mode to IX (where X page locks are requested) when an IS table
**		lock is requested but an IX lock is granted.
**	      - Changed call to check_hwm to not be dependent upon the current
**		relpages value which may not be accurate.
**	      - Removed access modes DM2T_DEL_WRITE and DM2T_SHOW_READ.
**	14-dec-1992 (rogerk)
**	    Changed Gateway handling of open calls from purge operations to
**	    check for DM2T_A_OPEN_NOACCESS requests rather than DM2T_DEL_WRITE.
**	    The latter mode is no longer used.
**	18-jan-1993 (rogerk)
**	    Bypassed dm1p_checkhwm call when called in OPEN_NOACCESS mode so
**	    that we do not try to read the table when opening for a PURGE
**	    operation where the tcb may not reflect the correct contents of
**	    the table's files.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables.
**	26-may-1993 (robf)
**	    Initialize the tcb_sec_lbl_att and tcb_sec_key_att pointers
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	20-jun-1994 (jnash)
**	    fsync project.  Add DM2T_A_SYNC_CLOSE access_mode,
**	    propagate request to dm2t_fix_tcb.
**	09-jan-1995 (nanpr01)
**	    Add support for log_inconsistent, phys_inconsistent support.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ (originally proposed by bryanp)
**      17-may-1995 (dilma04)
**          If a READONLY table, avoid multiple page locks by locking whole
**          table in S mode.
**	21-aug-95 (tchdi01)
**	    Added support for the production mode feature. When a system
**	    catalog is opened for writing, check whether the database is
**	    in the production mode. If it is, do not open the catalog and
**	    return an error. Added a new access mode DM2T_A_SET_PRODUCTION.
**	    The mode opens a system catalog for writing in production
**	    mode.
**	13-feb-1996 (canor01)
**	    If database is in production mode, don't take the IX tablelock.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't use sizeof(DMPP_PAGE), use relpgsize instead.
**	17-may-96 (stephenb)
**	    Add replication code, check for replication and open shadow
**	    archive, and shad index tables.
**	10-jul-96 (nick)
**	    Code to force system catalogs to page type locking ignored the
**	    lock mode and always used DM2T_IX when of course it should be
**	    be DM2T_IS if we requested DM2T_S. #77614
**	21-aug-96 (stephenb)
**	    Add code for replicator cdds lookup table
**	23-aug-96 (nick)
**	    Fail the open if there are inconsistent secondaries on the base
**	    table ( if we're not intending to modify or alter the table ).
**	    #77429
**	24-Oct-1996 (somsa01)
**	    For extended tables, set the locking scheme as physical page locks.
**	    This is because there will be concurrent access to extended tables.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**            - Initialize lock information for row locking;
**            - If DM716 is set, force row locking.
**      12-dec-96 (dilma04)
**          Move grant_mode initialization to the right place.
**	07-jan-1997 (canor01)
**	    For production mode, we must take the IX tablelock (which was
**	    removed in change of 13-feb-1996).  We can avoid taking the
**	    control lock.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - replace lock_duration parameter with iso_level;
**          - change lock_mode values for row locking;
**          - check lock_mode for NOREADLOCK protocoln;
**          - set rcb_iso_level according to iso_level.
**	11-Mar-1997 (jenjo02)
**	    Temp tables are always writable. Set rcb_access_mode = RCB_A_WRITE.
**	28-Mar-1997 (wonst02)
**	    Temp tables can be written, even to a "readonly" database.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - if DM717 is set, force CS overriding higher isolation levels;
**          - if DM718 is set, force RR isolation instead of serializable;
**          - allow CS and RR isolation for user tables only. 
**      21-May-1997 (stial01)
**          Enable row locking for tables with blobs, disable for temp tables
**	30-may-1997 (nanpr01)
**	    Take out the unnecessary initialization from dm2t_open and 
**	    initialize it in dm2r_rcb_allocate for replicator. Also added
**	    the error check.
**      13-jun-97 (dilma04)
**          Moved code to enforce isolation levels to dmtopen.c where isolation
**          level processing is grouped.
**	11-sep-97 (thaju02)
**	    bug#81340 - With readlock=nolock, table control lock on extension
**	    table is not released. If extension table and readlock=nolock
**	    do not setup to use physical page locks, instead fall thru 
**	    to else if lock_duration == DM2T_NOREADLOCK and allocate rnl_cb.
**	14-nov-97 (stephenb)
**	    row locking for replicator support tables was not being inherited
**	    from the user lock_mode because we re-set it at the start of this 
**	    routine for some reason. Update the code so that the locking 
**	    strategy is inherited from the row_lock bool.
**	25-nov-97 (stephenb)
**	    Add code to set replicator locking strategy using PM set values
**	    or inherit from "set lockmode".
**      20-jan-98 (stial01)
**          dm2t_open() Disable row locking from RTREE indexes.
**      21-apr-98 (stial01)
**          Disable row locking for extension tables for which we do temporary 
**          physical page locking.       
**      28-may-1998 (stial01)
**          dm2t_open() Non-core catalogs default to row locking if possible.
**      15-Jul-98 (wanya01)
**          Bug #68560 - E_US125D Ambiguous Replace message is generated
**          if there are more than 65536 statements in a single transaction
**          Problem is due to page_seq_number is defined as u_i2 for 2k page
**          page_seq_number has been changed to u_i4 for page_size greater
**          than 2k.  Add if condition to utilize the new page format.
**	22-jul-98 (stephenb)
**	    When constructing replicator support tables (shadow and archive),
**	    convert non alpha-numerics to underscores. Replicator frontends
**	    don't handle them.
**      12-jan-99 (thaju02)
**          if readlock=nolock, set rcb_lk_type to RCB_K_PAGE for all table
**          structures; must use page level locking not only for btree
**          index pages but also for fhdr page for all structures. (B94845)
**	6-apr-99 (stephenb)
**	    Fix replicator shadow and archive table name construction
**	    to comply with repmgr's warped view of the world.
**	21-apr-99 (stephenb)
**	    Above change performed wrong check, fixed this.
**      05-may-99 (nanpr01)
**          Gateway tables should not be row locking.
**      30-Mar-99 (islsa01)
**          Bug 94775.  Ingres will follow lock protocols defined by current
**          isolation level, which is REPEATABLE READ (RR) in this case.
**          However the use of READLOCK=NOLOCK should override this isolation
**          level REPEATABLE READ to READ UNCOMMITTED.
**	12-oct-1999 (gygro01)
**	    Changed lockmode from exclusive to shared for replicator
**	    lookup tables (horizontal and priority). There is only reading
**	    done on this tables during data capture, so exclusive locks are
**	    not necessary. This resolves contentions problems.
**	    Bug 99176 / pb ingrep 66.
**      21-oct-99 (stial01)
**          dm2t_open() Row locking for catalogs only if grant mode not table.
**      04-nov-99 (stial01)
**          dm2t_open() Move determination of catalog lock level to dml/dmtopen
**	17-dec-99 (gupsh01)
**	    Changed the error returned to E_DM006A_TRAN_ACCESS_CONFLICT instead 
**	    of E_DM003E_DB_ACCESS_CONFLICT for readonly databases.
**      02-jan-2004 (huazh01/thaju02)
**          Extend the fix of bug 81340 to cover the case of 
**          openning a table with readlock == nolock.  
**          bug 110857, INGSRV2509.
**	17-feb-04 (inkdo01)
**	    Fixed init. of local variable in partitioning code.
**	5-Mar-2004 (schka24)
**	    Don't tangle up master rcb with partition rcbs in updating list.
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag.
**	16-Mar-2005 (schka24)
**	    No-access opens are done when we're going to do something like
**	    drop the table/view.  If no-access is requested, tell fix that
**	    a show-only TCB is acceptable.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMdigit/CMalpha macro calls to relaxed bool form
**	24-Jan-2007 (kibro01) b117249
**	    Locks should not be held on IMA gateway tables, so once we've
**	    opened the table and established it is IMA, release the lock.
**      29-jan-2007 (huazh01)
**          update the DMP_TCB for replicator's shadow, shadow idx, and archive
**          table to indicate they are replicator specific table. 
**          This fixes bug 117355.
**	20-Feb-2007 (kibro01) b117249
**	    Additionally, ensure the whole database isn't locked exclusively -
**	    if it is then there are no individual locks to drop (kibro01)
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	11-May-2007 (gupsh01)
**	    Initialize adf_utf8_flag.
**	16-Apr-2008 (kschendel)
**	    ADFCB utf8 flag init is redundant, already done in dm2r.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add support for MVCC lock level, new LK_TBL_MVCC
**	    lock on table.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Serializable ROW locking is incompatible with
**	    MVCC because it might change to PAGE locking to provide
**	    phantom protection.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC: Modify to support MVCC protocols on blobs.
**	    Reference tcb_extended instead of relstat2 & TCB2_HAS_EXTENSIONS,
**	    which are equivalent, for consistency.
**	    Don't subvert MVCC if read-only table and nologging.
**	    Combined two separate "is MVCC ok" blocks of code into one,
**	    delay check for MVCC disabled in database until we actually
**	    think we're going to use it (b123376).
**	    Issue warning if MVCC update mode with nologging before switching
**	    to ROW, just so everyone knows.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Accept "no coupon" access modes;  for ordinary read/write with
**	    LOB's in the table, find or create BQCB, save in RCB.
*/
DB_STATUS
dm2t_open(
DMP_DCB             *dcb,
DB_TAB_ID           *table_id,
i4             lock_mode,
i4             update_mode,
i4             req_access_mode,
i4             timeout,
i4             maxlocks,
i4             sp_id,
i4             log_id,
i4             lock_id,
i4             sequence,
i4             iso_level,
i4             db_lockmode,
DB_TRAN_ID          *tran_id,
DB_TAB_TIMESTAMP    *timestamp,
DMP_RCB             **record_cb,
DML_SCB		    *scb,
DB_ERROR            *dberr)
{
    DMP_RCB		*rcb = NULL;
    DMP_TCB		*tcb = NULL;
    DB_STATUS		status = E_DB_OK;
    i4		table_lock = FALSE;
    i4		control_lock = FALSE;
    i4		grant_mode;
    LK_LOCK_KEY		lock_key;
    LK_LKID		lockid;
    DMP_RCB		*r;
    ADF_CB		*adf_cb;
    i4		local_error;
    i4		flags;
    DB_STATUS		local_status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    DB_TAB_NAME		rep_shad_tab;
    DB_TAB_NAME		rep_arch_tab;
    DB_TAB_NAME		rep_shad_idx;
    DB_TAB_NAME		rep_prio_lookup;
    DB_TAB_NAME		rep_cdds_lookup;
    DB_TAB_ID		rep_arch_tabid;
    DB_TAB_ID		rep_shad_tabid;
    DB_TAB_ID		rep_shad_idxid;
    DB_TAB_ID		rep_prio_tabid;
    DB_TAB_ID		rep_cdds_tabid;
    i4			idx;
    char		tabno[6];
    i4			size;
    DB_TAB_TIMESTAMP	local_timestamp;
    i4			cha;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    i4			mvcc_lock_mode;
    LK_LKID		mvcc_lkid;
    bool		mvcc_lock = FALSE;
    bool		nocoupon;
    i4			lockLevel;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
        dmd_check(E_DM9323_DM2T_OPEN);
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_OPEN       (%~t,%d,%d)\n",
        sizeof(dcb->dcb_name), &dcb->dcb_name,
        table_id->db_tab_base, table_id->db_tab_index);
#endif

    dmf_svcb->svcb_stat.tbl_open++;

    lockid.lk_unique = 0;
    lockid.lk_common = 0;

    mvcc_lkid.lk_unique = 0;
    mvcc_lkid.lk_common = 0;

    /* Only regular read/write wants coupon / LOB processing.
    ** Once that's set, normalize no-coupon read/write into ordinary r/w.
    */

    nocoupon = TRUE;
    if (req_access_mode == DM2T_A_READ || req_access_mode == DM2T_A_WRITE
      || req_access_mode == DM2T_A_SYNC_CLOSE)
	nocoupon = FALSE;

    if (req_access_mode == DM2T_A_READ_NOCPN)
	req_access_mode = DM2T_A_READ;
    else if (req_access_mode == DM2T_A_WRITE_NOCPN)
	req_access_mode = DM2T_A_WRITE;

    /*
    ** The conversion of lock_mode is needed
    ** for compatibility with page locking.
    **
    ** In a MVCC-eligible database we take a new lock in addition
    ** to LK_TABLE, LK_TBL_MVCC, to enable concurrent locking
    ** methods when possible.
    **
    ** If the database is not locked exclusively and this table
    ** open lock mode is not exclusive, then we request a MVCC
    ** lock prior to the LK_TABLE lock.
    **
    **	Table lock mode Lock level      MVCC lock mode  Compatible with
    **
    **	DM2T_S      	TABLE		LK_S		TABLE, PAGE, ROW
    **	DM2T_IS,	PAGE		LK_S		TABLE, PAGE, ROW
    **  DM2T_IX
    **	DM2T_RIS,	ROW		LK_IS		TABLE, PAGE, ROW,
    **  DM2T_RIX					MVCC
    **  DM2T_MVCC	MVCC		LK_IX		ROW, MVCC
    **
    **  So, ROW and MVCC can coexist, MVCC and anything else - one must
    **  wait on the other.
    */
    mvcc_lock_mode = DM2T_N;

    /* Assume page locking */
    lockLevel = RCB_K_PAGE;

    /* Temporary tables are always locked X, period. */
    if ( table_id->db_tab_base < 0 )
        lock_mode = DM2T_X;

    if ( lock_mode == DM2T_RIS || lock_mode == DM2T_RIX )
    {
	/*
	** Row locking.
	**
	** If MVCC database, MVCC-lock the table LK_IS, unless
	** the session is running with NOLOGGING and is
	** opening the table for write access.
	**
	** Because MVCC works by producing read-consistent
	** pages from the log records, we can't allow
	** NOLOGGING holes when the table is opened for MVCC (LK_IX)
	** so we'll open the table LK_S, waiting for the MVCC
	** LK_IX lock(s) to be released. This also prevents
	** new MVCC table opens, as LK_IX is incompatible with LK_S.
	**
	** If isolation level is serializable, Ingres may change
	** the lock level from ROW to PAGE to provide phantom
	** protection. That will be in conflict with MVCC, so
	** get the mvcc lock in an incompatible mode.
	*/
	lockLevel = RCB_K_ROW;

	if ( dcb->dcb_status & DCB_S_MVCC &&
	     (scb && scb->scb_sess_mask & SCB_NOLOGGING &&
	      req_access_mode != DM2T_A_READ)
	    ||
	      iso_level == DMC_C_SERIALIZABLE )

	{
	    mvcc_lock_mode = LK_S;
	}
	else
	    mvcc_lock_mode = LK_IS;

	if ( lock_mode == DM2T_RIS )
	    lock_mode = DM2T_IS;
	else
	    lock_mode = DM2T_IX;
    }
    else if ( lock_mode == DM2T_MIS || lock_mode == DM2T_MIX )
    {
	/*
	** MVCC locking is row locking without covering page locks.
	**
	** Lock MVCC IX, the TABLE IS or IX.
	*/
	lockLevel = RCB_K_CROW;

	mvcc_lock_mode = LK_IX;
	if ( lock_mode == DM2T_MIS )
	    lock_mode = DM2T_IS;
	else
	    lock_mode = DM2T_IX;
    }
    else
    {
	/* Table/page locking */
	if ( lock_mode != DM2T_IS && lock_mode != DM2T_IX )
	    lockLevel = RCB_K_TABLE; 

	/*
	** If MVCC database, MVCC-lock LK_S 
	** unless exclusively locking the table.
	** If readlock=nolock, no MVCC lock.
	*/
        if ( dcb->dcb_status & DCB_S_MVCC && 
	     lock_mode != DM2T_X &&
	     lock_mode != DM2T_N )
	{
	    mvcc_lock_mode = LK_S;
	}
    }

    grant_mode = lock_mode;

    /*
    ** If DB is locked exclusively or catalog (which don't use MVCC),
    ** or DB is ineligible for row/mvcc locking, no need for MVCC lock.
    ** Note that temp tables (db_tab_base < 0) fall into this category.
    */
    if ( dcb->dcb_status & DCB_S_EXCLUSIVE ||
	 !(dcb->dcb_status & DCB_S_MVCC) ||
         table_id->db_tab_base < DM_SCATALOG_MAX ||
	 !dm2d_row_lock(dcb) )
    {
        mvcc_lock_mode = LK_N;
    }

    do
    {
	/*
	** Sync at close requires X table lock.
	*/
	if (req_access_mode == DM2T_A_SYNC_CLOSE
	    && lock_mode != DM2T_X)
	{
	    SETDBERR(dberr, 0, E_DM003E_DB_ACCESS_CONFLICT);
	    return (E_DB_ERROR);
	}

	/*
	** Override requests to lock core DMF catalogs in Table Mode and force
	** them to Page Mode.
	**
	** Don't include temp tables as "core DMF catalogs" - oops
	*/
	if ( table_id->db_tab_base > 0 &&
	     table_id->db_tab_base <= DM_SCONCUR_MAX &&
	     req_access_mode != DM2T_A_SET_PRODUCTION )
	{
	    if (lock_mode == DM2T_S || lock_mode == DM2T_N)
	    {
		lock_mode = DM2T_IS;
		lockLevel = RCB_K_PAGE;
	    }
	    else if (lock_mode == DM2T_X)
	    {
		lock_mode = DM2T_IX;
		lockLevel = RCB_K_PAGE;
	    }
	}


        if (lock_mode == DM2T_N)
	{
	    /* control lock not needed in production mode */
	    if (!(dcb->dcb_status & DCB_S_PRODUCTION))
	    {
	        /*
	        ** bug #34339 (I THINK THIS IS 34342 (bryanp))
	        ** Set control lock before calling fix_tcb, so that
	        ** NOREADLOCK thread can correctly build the tcb if necessary.
	        ** If lock is not set, it is possible for the built TCB to be
	        ** corrupted by concurrent builders leading to various errors.
	        */
	        status = dm2t_control(dcb, table_id->db_tab_base, lock_id,
		    LK_S, (i4)0, timeout, dberr);
	        if (status != E_DB_OK)
		    break;
	        control_lock = TRUE;
	    }
	}

	if ( mvcc_lock_mode != DM2T_N )
	{
	    status = dm2t_mvcc_lock(dcb, table_id, lock_id, mvcc_lock_mode,
	    			timeout, &mvcc_lkid, dberr);
	    if ( status )
	        break;
	    mvcc_lock = TRUE;
	}

        if ( lock_mode != DM2T_N )
	{
	    status = dm2t_lock_table(dcb, table_id, lock_mode, lock_id,
		timeout, &grant_mode, &lockid, dberr);
	    if (status != E_DB_OK)
		break;
	    table_lock = TRUE;
	}


	/*
	** Fix the TCB in server memory.
	**
	** If a valid TCB is not already in the cache, then one will be built
	** and its files opened.
	*/
	flags = (DM2T_NOPARTIAL | DM2T_VERIFY);

	/*
	** If the table or database open request is to sync-at-close, pass
	** the indication along to the tcb/tabio build code.
	*/
	if ((req_access_mode == DM2T_A_SYNC_CLOSE) ||
	    (dcb->dcb_status & DCB_S_SYNC_CLOSE))
	    flags |= DM2T_CLSYNC;

	/* If the table will not be accessed, but we're just acquiring a
	** lock and verifying reference counts, a show-only TCB is acceptable.
	*/
	if (req_access_mode == DM2T_A_OPEN_NOACCESS)
	    flags |= DM2T_SHOWONLY;

        status = dm2t_fix_tcb(dcb->dcb_id, table_id, tran_id, lock_id, log_id,
	    flags, dcb, &tcb, dberr);
        if (status != E_DB_OK)
            break;

	/* If this is HEAP etab, use table locking, unless MVCC locking */
	if ( tcb->tcb_extended && tcb->tcb_rel.relspec == DB_HEAP_STORE &&
		lockLevel != RCB_K_CROW )
	{
	    if (lock_mode == DM2T_IS)
	    {
		lock_mode = DM2T_S;
		lockLevel = RCB_K_TABLE;
	    }
	    else if (lock_mode == DM2T_IX)
	    {
		lock_mode = DM2T_X;
		lockLevel = RCB_K_TABLE;
	    }
	}

	/*  Check for write access to a read only database. */

	if (tcb->tcb_temporary == TCB_PERMANENT
	    && dcb->dcb_access_mode == DCB_A_READ
	    && req_access_mode != DM2T_A_READ)
	{
	    SETDBERR(dberr, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
            status = E_DB_ERROR;
            break;
	}


        if (tcb->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG))
        {
	    /* it's a system catalog */
            if (   (   (req_access_mode == DM2T_A_WRITE)
                    || (req_access_mode == DM2T_A_MODIFY))
                && dcb->dcb_status & DCB_S_PRODUCTION)

            {
		SETDBERR(dberr, 0, E_DM0181_PROD_MODE_ERR);
	        status = E_DB_ERROR;
	        break;
            }
        }

	/*
	** Trace the MVCC and Table Locks if granted - we do this after fixing
	** the TCB so so that we can include the table name in the message.
	*/
	if ( DMZ_SES_MACRO(1) )
	{
	    lock_key.lk_key1 = dcb->dcb_id;
	    lock_key.lk_key2 = table_id->db_tab_base;
	    lock_key.lk_key3 = 0;
	    lock_key.lk_key4 = 0;
	    lock_key.lk_key5 = 0;
	    lock_key.lk_key6 = 0;

	    if ( mvcc_lock_mode != DM2T_N )
	    {
		lock_key.lk_type = LK_TBL_MVCC;
		dmd_lock(&lock_key, lock_id, LK_REQUEST, LK_PHYSICAL,
		    mvcc_lock_mode, timeout, tran_id, &tcb->tcb_rel.relid,
		    &dcb->dcb_name);
	    }

	    if ( lock_mode != DM2T_N )
	    {
		lock_key.lk_type = LK_TABLE;
		dmd_lock(&lock_key, lock_id, LK_REQUEST, LK_PHYSICAL,
		    grant_mode, timeout, tran_id, &tcb->tcb_rel.relid,
		    &dcb->dcb_name);
	    }
	}

	/*
        ** If a READONLY table, avoid multiple page locks by locking whole
        ** table in S mode and make sure we are not trying to write to the
        ** table or modify it.
        */
        if (tcb->tcb_rel.relstat2 & TCB2_READONLY)
	{
            if (req_access_mode == DM2T_A_READ)
	    {
                lock_mode = DM2T_S;
		lockLevel = RCB_K_TABLE;
	    }
            else if ((req_access_mode == DM2T_A_WRITE) ||
                        (req_access_mode == DM2T_A_MODIFY))
            {
		SETDBERR(dberr, 0, E_DM0067_READONLY_TABLE_ERR);
                status = E_DB_ERROR;
                break;
            }
	}

	/*
	** If a table is marked physically/logically inconsistent,
	** do not let it continue.
	*/
	if ( ( tcb->tcb_rel.relstat2 & TCB2_LOGICAL_INCONSISTENT ) &&
	     (req_access_mode != DM2T_A_OPEN_NOACCESS) )
	{
	    SETDBERR(dberr, 0, E_DM0161_LOG_INCONSISTENT_TABLE_ERR);
	    status = E_DB_ERROR;
	    break;
	}
	if ( (tcb->tcb_rel.relstat2 & TCB2_PHYS_INCONSISTENT ) &&
	     (req_access_mode != DM2T_A_OPEN_NOACCESS) )
	{
	    SETDBERR(dberr, 0, E_DM0162_PHYS_INCONSISTENT_TABLE_ERR);
	    status = E_DB_ERROR;
	    break;
	}
	/*
	** don't permit the open to proceed unless we are changing
	** the table's state or perfoming a modify ( in which case the
	** inconsistent indicies will be dropped )
	*/
	if ((tcb->tcb_status & TCB_INCONS_IDX) &&
	    (req_access_mode != DM2T_A_OPEN_NOACCESS) &&
	    (req_access_mode != DM2T_A_MODIFY))
	{
	    SETDBERR(dberr, 0, E_DM0168_INCONSISTENT_INDEX_ERR);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Validate attempts to use lock level MVCC:
	**
	** o Catalogs and iidbdb don't use MVCC
	** o The database and table must support row locking
	** o SET NOLOGGING can only be in effect if the table
	**   is being opened for read access
	*/
	if ( lockLevel == RCB_K_CROW )
	{
	    /* Skip catalogs and iidbdb */
	    if ( table_id->db_tab_base > DM_SCATALOG_MAX &&
	         !(tcb->tcb_rel.relstat & TCB_CATALOG) &&
		 dcb->dcb_id != 1 )
	    {
		/*
		** DB/Table must be row-lockable and
		** nologging can only be on if opening for read
		*/
	        if ( !dm2t_row_lock(tcb, lockLevel) ||
			(scb->scb_sess_mask & SCB_NOLOGGING &&
			 req_access_mode != DM2T_A_READ) )
		{
		    /* Error if lock level explicitly set for session/table */
		    if ( dmf_svcb->svcb_lk_level != DMC_C_MVCC )
		    {
			/*
			** System lock level is -not- MVCC, set explicitly
			** by user.
			*/
			uleFormat(dberr, E_DM0012_MVCC_INCOMPATIBLE, (CL_ERR_DESC *)NULL,
			      ULE_LOG, NULL, NULL, 0, NULL, &local_error, 2,
			      sizeof(DB_TAB_NAME), tcb->tcb_rel.relid.db_tab_name,
			      sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name);
			status = E_DB_ERROR;
			break;
		    }
		    else if ( !dm2t_row_lock(tcb, lockLevel) )
		    {
			/*
			** System lock level -is- MVCC:
			**
			** Issue a warning that table isn't row-locking
			** compatible (like 2k, for example).
			*/
			TRdisplay("%@ dm2t_open:%d WARNING: Table %~t in Database %~t"
			      " is incompatible with MVCC, using PAGE locking\n",
			      __LINE__,
			      sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
			      sizeof(dcb->dcb_name), &dcb->dcb_name);
			lockLevel = RCB_K_PAGE;
		    }
		    else
		    {
			/* 
			** All else checks out, but opening in write mode with
			** nologging - issue a warning, switch to ROW locking.
			*/
			TRdisplay("%@ dm2t_open:%d WARNING: Table %~t in Database %~t"
			      " NOLOGGING is incompatible with MVCC update, using ROW locking\n",
			      __LINE__,
			      sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
			      sizeof(dcb->dcb_name), &dcb->dcb_name);
			lockLevel = RCB_K_ROW;
		    }
		}
		else if ( dcb->dcb_status & DCB_S_MVCC_DISABLED )
		{
		    /* Otherwise ok, but alterdb -disable_mvcc has been done */
		    uleFormat(dberr, E_DM0013_MVCC_DISABLED, (CL_ERR_DESC *)NULL,
		      ULE_LOG, NULL, NULL, 0, NULL, &local_error, 1,
		      sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name);
		    status = E_DB_ERROR;
		    break;
		}
		else
		{
		    /* Passed all checks, use MVCC */

		    /*
		    ** With MVCC, RR and Serializable are equivalent,
		    ** use Serializable in RCB for consistency.
		    */
		    if ( iso_level == DMC_C_REPEATABLE_READ )
			iso_level = DMC_C_SERIALIZABLE;
		}
	    }
	    else
		/* Catalogs, iidbdb; always silently page lock */
	        lockLevel = RCB_K_PAGE;
	}

	/* Check row-locking ability, use PAGE if unable */
	if ( lockLevel == RCB_K_ROW && !dm2t_row_lock(tcb, lockLevel) )
	    lockLevel = RCB_K_PAGE;

	/*
	** Allocate Record Control Block for accessing this table.
	*/
	status = dm2r_rcb_allocate( tcb, (DMP_RCB *)0, tran_id,
 	    lock_id, log_id, &rcb, dberr);
	if (status != E_DB_OK)
	    break;


	/**********************************************************
	**
	** Fill in RCB fields appropriate for this Table Open.
	**
	***********************************************************/

	r = rcb;
	r->rcb_sp_id = sp_id;
	r->rcb_update_mode = update_mode;
	if (nocoupon)
	    r->rcb_state |= RCB_NO_CPN;

	/*
	** Set collation sequence to use - this is determined by the database.
	*/
	r->rcb_collation = dcb->dcb_collation;
	r->rcb_ucollation = dcb->dcb_ucollation;

	/*
	** Reset variable fields in the RCB's adf control block with database
	** level information.  The timezone and collation sequence are
	** are required for all ADF value comparisons made on this table.
	**
	** NOTE: Currently, only the collation sequence is set here - the
	** timezone is not.  Timezones are not currently tied to the database
	** itself - DMF functions either use the timezone in which the
	** server is currently running or the timezone of the user session.
	** Eventually, we would like to get away from using variable timezones
	** inside DMF and always use a constant Timezone associated with the DB.
	*/
	adf_cb = r->rcb_adf_cb;
	adf_cb->adf_collation = r->rcb_collation;
	adf_cb->adf_ucollation = r->rcb_ucollation;
	if (adf_cb->adf_ucollation)
	{
	    if (dcb->dcb_dbservice & DU_UNICODE_NFC)
		adf_cb->adf_uninorm_flag = AD_UNINORM_NFC;
	    else adf_cb->adf_uninorm_flag = AD_UNINORM_NFD;
	}
	else adf_cb->adf_uninorm_flag = 0;

	/*
	** Set the table access mode which determines the set of operations
	** which can be performed with this RCB.
	*/
	if (tcb->tcb_temporary == TCB_TEMPORARY)
        {
	    /* temp tables are always in WRITE mode, because QEF always
	    ** passes READ mode for select statements!  but, if this is
	    ** really a gateway table, it can't be a QEF temp, so use
	    ** the caller passed in mode in that case.
	    ** (Ideally QEF would be fixed, but it doesn't know when a
	    ** table on the valid list is for a QEF temp, which would
	    ** in turn mean fixing opc...)
	    */
	    r->rcb_access_mode = RCB_A_WRITE;
	    if (tcb->tcb_rel.relstat & TCB_GATEWAY
	      && req_access_mode == DM2T_A_READ)
		r->rcb_access_mode = RCB_A_READ;
        }
	else
	{
	    switch (req_access_mode)
	    {
		case    DM2T_A_READ:
		case    DM2T_A_SET_PRODUCTION:
			    r->rcb_access_mode = RCB_A_READ;
			    break;
		case    DM2T_A_WRITE:
			    /*
			    ** If this is a Read-Only Gateway, then it is an
			    ** error to open a table for write.  (We can't just
			    ** set mode to read here as later errors will result).
			    */
			    if (tcb->tcb_rel.relstat & TCB_GW_NOUPDT)
			    {
				SETDBERR(dberr, 0, E_DM0137_GATEWAY_ACCESS_ERROR);
				status=E_DB_ERROR;
				break;
			    }
			    r->rcb_access_mode = RCB_A_WRITE;
			    break;
		case    DM2T_A_OPEN_NOACCESS:
			    r->rcb_access_mode = RCB_A_OPEN_NOACCESS;
			    break;
		case    DM2T_A_MODIFY:
			    /*
			    ** If this is a Gateway table, indicate that the RCB is
			    ** used for 'modify' access only -- we'll just need the
			    ** statistical information about the table. For an
			    ** Ingres table, we need full write access.
			    */
			    if (tcb->tcb_rel.relstat & TCB_GATEWAY)
				r->rcb_access_mode = RCB_A_MODIFY;
			    else
				r->rcb_access_mode = RCB_A_WRITE;
			    break;
		case    DM2T_A_SYNC_CLOSE:
			    /*
			    ** This requests async writes.
			    */
			    r->rcb_access_mode = RCB_A_WRITE;
			    break;
		default:
			    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
			    status = E_DB_ERROR;
			    break;
	    }
	    if (status != E_DB_OK)
		break;
	}

	/* Potentially updating RCB's are linked into their tcb's potentially-
	** updating-rcb list.  We only use this to make it easy to gather up
	** row and page count deltas for running queries, so this list doesn't
	** have to be "perfect".
	** This is done here, rather than in rcb-allocate, partly because
	** I want to make sure we only count real queries, and not
	** strange on-the-fly internal stuff.
	** Please note that masters should only carry partition rcbs on
	** its list.  The master RCB itself is something of a dummy.
	** (It's a bad idea to have one queue pointing to two, as I
	** found out the hard way.)
	*/
	if (r->rcb_access_mode == RCB_A_WRITE)
	{
	    if ((tcb->tcb_rel.relstat & TCB_IS_PARTITIONED) == 0)
	    {
		/* Hook RCB at the front of tcb's updateable-rcb list */
		dm0s_mlock(&tcb->tcb_mutex);
		r->rcb_upd_list.q_next = tcb->tcb_upd_rcbs.q_next;
		r->rcb_upd_list.q_prev = &tcb->tcb_upd_rcbs;
		r->rcb_upd_list.q_next->q_prev = &r->rcb_upd_list;
		tcb->tcb_upd_rcbs.q_next = &r->rcb_upd_list;
		dm0s_munlock(&tcb->tcb_mutex);
	    }

	    if (tcb->tcb_rel.relstat2 & TCB2_PARTITION)
	    {
		DMP_TCB *master_tcb = tcb->tcb_pmast_tcb;

		/* Hook RCB at the front of master tcb's updateable-rcb list */
		dm0s_mlock(&master_tcb->tcb_mutex);
		r->rcb_mupd_list.q_next = master_tcb->tcb_upd_rcbs.q_next;
		r->rcb_mupd_list.q_prev = &master_tcb->tcb_upd_rcbs;
		r->rcb_mupd_list.q_next->q_prev = &r->rcb_mupd_list;
		master_tcb->tcb_upd_rcbs.q_next = &r->rcb_mupd_list;
		dm0s_munlock(&master_tcb->tcb_mutex);
	    }
	}

        if (tcb->tcb_rel.relpgtype == TCB_PG_V1)
           r->rcb_seq_number = (sequence & 0x0ffff);
        else
           r->rcb_seq_number = sequence;
	r->rcb_timeout = timeout;
	r->rcb_lk_limit = maxlocks;

	/*
	** Initialize Lock Information: lock mode and lock type. These will
	** determine the locking characteristics of the query.
	**
	** A lock may be granted at a higher mode that it is requested if the
	** transaction already holds the lock at a higher mode.  If we
	** requested the table lock in PAGE mode, but are granted the lock
	** in a TABLE mode we escalate our query to table level.
	**
	** rcb_k_mode and rcb_k_type are set according to the requested mode.
	** rcb_lk_mode and rcb_lk_type represent the escalated values.
	**
	** rcb_lk_mode must represent the actual escalated lock mode
	** and will be used by code attempting to figure out the level
	** at which the table is locked.
	**
	** rcb_k_mode must represent the requested mode and will be used
	** later to determine the type of page locks needed.
	**
	**     request    grant mode                comment
	**     -------    ----------   -------------------------------------
        **       IS        S,SIX,X      Request escalated to Table Level
        **       IX        X            Request escalated to Table Level
	**       IX        SIX          No escalation: rcb_k_mode set to IX
        **                              to indicate X page locks are needed.
        **       IS        IX           No escalation. rcb_k_mode set to IS
        **                              to indicate S page locks are needed.
	*/

	/* Save MVCC lock id, if any, in RCB */
	r->rcb_mvcc_lkid = mvcc_lkid;

	r->rcb_k_type = lockLevel;
	r->rcb_lk_type = lockLevel;
	
	r->rcb_k_mode = lock_mode;
	r->rcb_lk_mode = grant_mode;
	if ((grant_mode == DM2T_X) ||
	    ((grant_mode >= DM2T_S) && (lock_mode == DM2T_IS)))
	{
	    r->rcb_lk_type = RCB_K_TABLE;
	}

	/*
	** Unless overriden below, rcb_k_duration for MVCC-opened
	** tables is the default "RCB_K_TRAN" (set by dm2r_allocate()).
	**
	** MVCC rows are only locked when updated, and only LK_X,
	** hence the table itself is normally opened LK_IX.
	*/


	/*
	** If temporary table, simulate exclusive table-level locking even
	** though a table lock was not obtained.  This causes page locking
	** logic to be bypassed.
	*/
        if (tcb->tcb_temporary)
	{
	    r->rcb_lk_type = RCB_K_TABLE;
	    r->rcb_lk_mode = DM2T_X;
	}
	else if ( tcb->tcb_extended && lock_mode != DM2T_N )
	{
	    /*
	    **  bug#81340, thaju02 - if extension table and readlock=nolock
	    **  do not use physical page locks, instead fall thru to else if
	    **  lock_duration == DM2T_NOREADLOCK and allocate rnl_cb.
	    **
	    **  NB: physical page locks are not used when MVCC-locking blobs
	    */
	    if ( r->rcb_lk_type != RCB_K_TABLE && !crow_locking(r) )
	    {
		r->rcb_k_duration = RCB_K_PHYSICAL;
		r->rcb_lk_type = RCB_K_PAGE;
		r->rcb_lk_limit = 20;
	    }
	}
	/* we don't strictly need check for both TCB_CONCUR and 
	 * TCB2_PHYSLOCK_CONCUR - but it's better to keep it in case the user
	 * didn't upgradedb their database (which creates the TCB2 flag)
	 */
	else if ((((tcb->tcb_rel.relstat & TCB_CONCUR) || 
		   (tcb->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR)) &&
	    (dcb->dcb_status & DCB_S_EXCLUSIVE) == 0))
	{
	    /*
	    **  Force CORE system tables to short duration page locks if the
	    **  database is allowing concurrent access. Do this also for
	    **  extended tables, since they will be concurrently accessible.
	    */
	    r->rcb_k_duration = RCB_K_PHYSICAL;
	    r->rcb_lk_type = RCB_K_PAGE;
	    r->rcb_lk_limit = 20;
	}
        else if (lock_mode == DM2T_N)
	{
	    i4		rnl_buf_cnt;

	    /*
	    ** NB: MVCC protocols will never use NOREADLOCK.
	    **
	    ** If opened NOREADLOCK, the table control lock will have already
	    ** been acquired above (prior to the fix_tcb() call to protect
	    ** the table from being modified/destroyed while we are reading it).
	    **
	    ** Set Page Level locking to avoid further locks unless btree since
	    ** we must to page locking in the index.
	    **
	    ** Also allocate 2 page buffers to store readnolock pages.  When
	    ** we fix a readnolock page, we make a copy of it to avoid problems
	    ** with concurrent updaters.
	    */
	    r->rcb_k_duration = RCB_K_READ;
	    r->rcb_lk_type = RCB_K_PAGE;
           
            /*
            **  Fix for the Bug #94775, if we have acquired READNOLOCK,
            **  set isolation level to READ UNCOMMITTED.
            */

            iso_level = DMC_C_READ_UNCOMMITTED;

	
	    /*
	    **	Allocate a MISC_CB to hold the two pages needed to support
	    **  readlock=nolock.
	    **
	    **  jenjo02: I don't know why this storage needs to be
	    **		 zero-filled. dm0p moves the buffer into
	    **	         this space, so I don't think it needs to
	    **		 be cleared. I pulled the DM0M_ZERO flag.
	    */
	    if (tcb->tcb_seg_rows)
		rnl_buf_cnt = 3;
	    else
		rnl_buf_cnt = 2;
	    /* Get LongTerm memory */
	    status = dm0m_allocate(
		sizeof(DMP_MISC) + tcb->tcb_rel.relpgsize * rnl_buf_cnt,
		(i4)DM0M_LONGTERM, (i4)MISC_CB, (i4)MISC_ASCII_ID,
                (char *)r, (DM_OBJECT **)&r->rcb_rnl_cb, dberr);
	    if (status != E_DB_OK)
		break;
	    r->rcb_rnl_cb->misc_data = (char*)r->rcb_rnl_cb + sizeof(DMP_MISC);
    	}

        /*
        ** Set isolation level for the rcb.
        */
        if (iso_level == DMC_C_READ_UNCOMMITTED)
            r->rcb_iso_level = RCB_READ_UNCOMMITTED; 
        else if  (iso_level == DMC_C_READ_COMMITTED)
            r->rcb_iso_level = RCB_CURSOR_STABILITY;
        else if (iso_level == DMC_C_REPEATABLE_READ)
            r->rcb_iso_level = RCB_REPEATABLE_READ;
        else
            r->rcb_iso_level = RCB_SERIALIZABLE;

	/*
	** Force etabs to be serializable
	** They have their own physical page lock protocol
	** which should not be converted to logical when
	** lower isolation levels are specified by the user
	**
	** Don't override isolation level if MVCC-locking blob
	*/
	if ( tcb->tcb_extended && !crow_locking(r) )
	    r->rcb_iso_level = RCB_SERIALIZABLE;

	/*
	** If table is open for read-only, then no need to do logging.
	*/
	if (r->rcb_access_mode == RCB_A_READ)
	    r->rcb_logging = 0;

	/*
	** If table is a gateway table, then call gateway to open the
	** foreign database table.
	*/
	if (tcb->tcb_rel.relstat & TCB_GATEWAY)
	{
	    status = dmf_gwt_open(r, dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** Special open modes which are used only for accessing
		** system catalog information or in-memory TCB structures
		** are allowed here, even if the underlying gateway table
		** could not be accessed.  This is necessary so that PURGE
		** TCB operations can be performed.
		*/
		if (req_access_mode != DM2T_A_OPEN_NOACCESS)
		    break;

		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
	}

	/*
	**  Check whether we should update last known page number.
	**  Buffer manager sets this flag when a group page read is attempted
	**  beyond the current last page mark.  This allows group reads
	**  to be used on the newly allocated section of the file. A return
	**  status of E_DB_WARN from dm1p_checkhwm() means that a checksum
	**  error has occured.  Let the open complete so that the table
	**  can be destroyed or rebuilt.
	*/

	/*
	**  Check whether we should update last known page number.
	**  This value can only be determined from the FHDR/FMAP(s)
	**  scheme. This event only occurs if a combination of the
	**  following are true :
	**  1. The reflfhdr points to a valid location, this check allows
	**     a table without a FHDR/FMAP to be opened, for example
	**     conversion.
	**  2. The BM sets tabio_checkeof when a group page read is attempted
	**     beyond the tables current highwater mark, this value is cached
	**     in tbio_lpageno.
	**  3. Or the tbio_checkeof field has been set because the TCB has
	**     just recently been built.
	**  4. The TCB is being opened with the intention of reading the table.
	**     If the table is opened with the NOACCESS flag, then the hwm
	**     will not be of use and may not be readable (the table open may
	**     be for a purge request or a destroy request where the contents
	**     of the table are not consistent).
	**
	**  A return status of E_DB_WARN from dm1p_checkhwm() means that a
	**  checksum error has occured. Let the open complete so that the table
	**  can be destroyed or rebuilt.
	*/
	if ((tcb->tcb_rel.relfhdr != DM_TBL_INVALID_FHDR_PAGENO ) &&
	    (tcb->tcb_nofile == FALSE ) &&
	    (tcb->tcb_table_io.tbio_checkeof) &&
	    (req_access_mode != DM2T_A_OPEN_NOACCESS))
	{
	    status = dm1p_checkhwm(r, dberr);
	    if (status != E_DB_OK && status != E_DB_WARN)
		break;
	}


	*timestamp = r->rcb_tcb_ptr->tcb_rel.relstamp12;

	/*
	** if table is opened in write mode and is replicated, open shadow
	** and archive tables (but not if this connection belongs to the
	** replication server). Also don't replicate modifies or system catalog
	** access.
	** NOTE: recursive use of dm2t_open....should be O.K.
	*/
	if ((dcb->dcb_status & DCB_S_REPLICATE) &&
	    req_access_mode != DM2T_A_READ &&
	    req_access_mode != DM2T_A_MODIFY &&
	    req_access_mode != DM2T_A_OPEN_NOACCESS &&
	    (tcb->tcb_rel.relstat & TCB_CATALOG) == 0 &&
	    tcb->tcb_replicate == TCB_REPLICATE &&
	    (DMZ_SES_MACRO(32) == 0 || dmd_reptrace() == FALSE))
	{
	    i4	rep_sa_lock = 0;

	    /*
	    ** Construct shadow and archive table names, names are space
	    ** filled so we just work back until we find the first non-space
	    ** and add shadow and archive identifiers to the end of the name.
	    ** This is O.K. since tabnames are less than (not equal to)
	    ** DB_TAB_MAXNAME chars, otherwhise we could not replicate them
	    **
	    ** FIX_ME: This could be done more efficiently in build_tcb()
	    ** for the base table, we are scroling round iirelation there anyway
	    ** so we may as well get the tabids for the shadow and archive
	    ** tables, that way it only gets done once per TCB rathar than
	    ** once per RCB.
	    */
	    MEcopy(tcb->tcb_rel.relid.db_tab_name, DB_TAB_MAXNAME,
		rep_shad_tab.db_tab_name);
	    MEcopy(tcb->tcb_rel.relid.db_tab_name, DB_TAB_MAXNAME,
		rep_arch_tab.db_tab_name);
	    MEcopy(tcb->tcb_rel.relid.db_tab_name, DB_TAB_MAXNAME,
		rep_shad_idx.db_tab_name);
	    for (idx = DB_TAB_MAXNAME - 1; rep_shad_tab.db_tab_name[idx] == ' ';
		idx--);
	    idx++;
	    /*
	    ** we do some hocus pocus here to keep the tablename length within
	    ** 18 chars for ANSI, basically take the 1st 10 chars of the base
	    ** tablename, add 5 chars table number (always unique for a
	    ** database), and 3 chars extension type
	    */
	    if (idx > 10)
	    {
		MEfill(DB_TAB_MAXNAME - 10, ' ', rep_arch_tab.db_tab_name + 10);
		MEfill(DB_TAB_MAXNAME - 10, ' ', rep_shad_tab.db_tab_name + 10);
		MEfill(DB_TAB_MAXNAME - 10, ' ', rep_shad_idx.db_tab_name + 10);
		idx = 10;
	    }
	    /*
	    ** convert non-alpha numberics to undercores, this is done to
	    ** be compatible with the frontend replicator tools which don't
	    ** support non-printable characters in support table names. No
	    ** digits allowed in the 1st char.
	    */
	    for (cha = 0; cha < idx; cha++)
	    {
		if (cha == 0 && CMdigit(&tcb->tcb_rel.relid.db_tab_name[cha]) ||
		     !CMalpha(&tcb->tcb_rel.relid.db_tab_name[cha]) &&
		     !CMdigit(&tcb->tcb_rel.relid.db_tab_name[cha]))
		{
		    rep_arch_tab.db_tab_name[cha] = '_';
		    rep_shad_tab.db_tab_name[cha] = '_';
		    rep_shad_idx.db_tab_name[cha] = '_';
		}
	    }
	    CVna(tcb->tcb_rep_info->dd_reg_tables.tabno, tabno);
	    size = STlength(tabno);
	    MEfill(5 - size, '0', rep_arch_tab.db_tab_name + idx);
	    MEcopy(tabno, size, rep_arch_tab.db_tab_name + idx + 5 - size);
	    MEfill(5 - size, '0', rep_shad_tab.db_tab_name + idx);
	    MEcopy(tabno, size, rep_shad_tab.db_tab_name + idx + 5 - size);
	    MEfill(5 - size, '0', rep_shad_idx.db_tab_name + idx);
	    MEcopy(tabno, size, rep_shad_idx.db_tab_name + idx + 5 - size);
	    /*
	    ** Use upper case if needed (for SQL92).
	    ** Note: we depend on the fact that we must be in a transaction
	    ** in order to open a table in write mode here (otherwhise we
	    ** could be in trouble when using SQL92 case semantics).
	    **
	    ** Due to the fact that replicator servers do not use quotes to
	    ** delimit the support tables, if, in an SQL-92 installation, a
	    ** delimited lower case base table name is used, this name is auto
	    ** converted to upper case when naming the support tables. The DBMS
	    ** needs to reflect this policy by uper casing the names for SQL-92.
	    **
	    ** NOTE: None of this SQL-92 upper case hocus-pocus works on a 
	    ** double-byte platform, but that shouldn't be a problem, since
	    ** I don't think we support SQL-92 for double byte, the concept
	    ** of upper-case doesn't exist in double byte charater sets.
	    ** The fact is that we need a null terminated string to run CVupper
	    ** and a space delimited string for evenrything else, this is the
	    ** most effecient way of getting that, since other scenarios would
	    ** involve a memory copy of the string.
	    */
	    if (scb && scb->scb_dbxlate && (*scb->scb_dbxlate & CUI_ID_REG_U))
	    {
		/* 4 bytes copies the null terminator */
	    	MEcopy("ARC", 4, rep_arch_tab.db_tab_name + idx + 5);
	    	MEcopy("SHA", 4, rep_shad_tab.db_tab_name + idx + 5);
	    	MEcopy("SX1", 4, rep_shad_idx.db_tab_name + idx + 5);
		CVupper(rep_shad_idx.db_tab_name);
		CVupper(rep_shad_tab.db_tab_name);
		CVupper(rep_shad_idx.db_tab_name);
		/* remove the null terminator */
		rep_arch_tab.db_tab_name[idx+8] = 
		    rep_shad_tab.db_tab_name[idx+8] =
		    rep_shad_idx.db_tab_name[idx+8] = ' ';
	    }
	    else
	    {
	    	MEcopy("arc", 3, rep_arch_tab.db_tab_name + idx + 5);
	    	MEcopy("sha", 3, rep_shad_tab.db_tab_name + idx + 5);
	    	MEcopy("sx1", 3, rep_shad_idx.db_tab_name + idx + 5);
	    }
	    MEcopy(tcb->tcb_rep_info->dd_reg_tables.prio_lookup_table,
		DB_TAB_MAXNAME, rep_prio_lookup.db_tab_name);
	    MEcopy(tcb->tcb_rep_info->dd_reg_tables.cdds_lookup_table,
		DB_TAB_MAXNAME, rep_cdds_lookup.db_tab_name);
	    /*
	    ** get tab ids, table owner is assumed to be the DBA
	    */
	    status = get_supp_tabid(dcb, &rep_shad_tab, &rep_arch_tab,
		&rep_shad_idx, &rep_prio_lookup, &rep_cdds_lookup,
		&dcb->dcb_db_owner, &rep_shad_tabid, &rep_arch_tabid,
		&rep_shad_idxid, &rep_prio_tabid, &rep_cdds_tabid,
		tran_id, lock_id, log_id, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** check for default locking strategy. NOTE: "set lockmode"
	    ** for the specific table will over-ride other definitions 
	    ** (un-supported)
	    */

	    if (dmf_svcb->svcb_rep_salock == DMC_C_ROW)
		rep_sa_lock = DM2T_RIX;
	    else if (dmf_svcb->svcb_rep_salock == DMC_C_PAGE)
		rep_sa_lock = DM2T_IX;
	    else if (dmf_svcb->svcb_rep_salock == DMC_C_TABLE)
		rep_sa_lock = DM2T_X;
	    else if (lockLevel == RCB_K_ROW)
		rep_sa_lock = DM2T_RIX;
	    else /* default to user defined locking */
		rep_sa_lock = lock_mode;
	    /*
	    ** now open the tables. Note, we use direct update mode for the
	    ** shadow table since we may insert a compensation shadow record
	    ** and then try to update it in the same update action.
	    */
	    status = dm2t_open(dcb, &rep_shad_tabid, rep_lockmode(scb, 
		&rep_shad_tabid, rep_sa_lock, DM2T_A_WRITE), DM2T_UDIRECT,
		req_access_mode, timeout, maxlocks, sp_id, log_id, lock_id,
		sequence, iso_level, db_lockmode, tran_id, &local_timestamp,
		&r->rep_shad_rcb, (DML_SCB *)0, dberr);
	    if (status != E_DB_OK)
		break;
	    status = dm2t_open(dcb, &rep_arch_tabid, rep_lockmode(scb,
		&rep_arch_tabid, rep_sa_lock, DM2T_A_WRITE), update_mode,
		req_access_mode, timeout, maxlocks, sp_id, log_id, lock_id,
		sequence, iso_level, db_lockmode, tran_id, &local_timestamp,
		&r->rep_arch_rcb, (DML_SCB *)0, dberr);
	    if (status != E_DB_OK)
		break;
	    status = dm2t_open(dcb, &rep_shad_idxid, rep_lockmode(scb,
		&rep_shad_idxid, rep_sa_lock, DM2T_A_WRITE), update_mode,
		req_access_mode, timeout, maxlocks, sp_id, log_id, lock_id,
		sequence, iso_level, db_lockmode, tran_id, &local_timestamp,
		&r->rep_shadidx_rcb, (DML_SCB *)0, dberr);
	    if (status != E_DB_OK)
		break;

            /* bug 117355 */
            r->rep_shad_rcb->rcb_tcb_ptr->tcb_reptab = TCB_REPTAB; 
            r->rep_arch_rcb->rcb_tcb_ptr->tcb_reptab = TCB_REPTAB;
            r->rep_shadidx_rcb->rcb_tcb_ptr->tcb_reptab = TCB_REPTAB;

	    if (rep_prio_tabid.db_tab_base)
	    {
	    	status = dm2t_open(dcb, &rep_prio_tabid, rep_lockmode(scb,
		    &rep_prio_tabid, rep_sa_lock, DM2T_A_READ), update_mode, 
		    DM2T_A_READ, timeout, maxlocks, sp_id, 
		    log_id, lock_id, sequence, iso_level, db_lockmode, tran_id,
		    &local_timestamp, &r->rep_prio_rcb, (DML_SCB *)0, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    if (rep_cdds_tabid.db_tab_base)
	    {
	    	status = dm2t_open(dcb, &rep_cdds_tabid, rep_lockmode(scb,
		    &rep_cdds_tabid, rep_sa_lock, DM2T_A_READ), update_mode, 
		    DM2T_A_READ, timeout, maxlocks, sp_id, 
		    log_id, lock_id, sequence, iso_level, db_lockmode, tran_id,
		    &local_timestamp, &r->rep_cdds_rcb, (DML_SCB *)0, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	}

	/* If this is an IMA table (and a lock was taken - not DM2T_N), the
	** lock should not be kept since this table doesn't really exist in
	** this database (kibro01) b117249
	** Additionally, ensure the whole database isn't locked exclusively -
	** if it is then there are no individual locks to drop (kibro01)
	*/
	if (tcb->tcb_rel.relgwid == GW_IMA && 
		table_lock && lock_mode != DM2T_N &&
		(dcb->dcb_status & DCB_S_EXCLUSIVE) == 0)
	{
	    stat = LKrelease((i4)0, lock_id, (LK_LKID *)&lockid,
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	    if (stat != OK)
	    {
		uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL, (i4)0,
		    (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		    0, lock_id);
		status = E_DB_ERROR;
		break;
	    }
	}

	*record_cb = r;

	/* If this is an encrypted table, locate its key in Dmc_crypt and
	** save the slot number in the RCB. Will not find it if encryption
	** is not enabled yet. Indices inherit the encryption of the parent.
	*/
	r->rcb_enckey_slot = 0;
	if ( (Dmc_crypt != NULL) &&
	     (tcb->tcb_rel.relencflags & TCB_ENCRYPTED ||
	      tcb->tcb_parent_tcb_ptr->tcb_rel.relencflags & TCB_ENCRYPTED)
	   )
	{
	    DMC_CRYPT_KEY *cp;
	    int	i;

	    for ( cp = (DMC_CRYPT_KEY *)((PTR)Dmc_crypt + sizeof(DMC_CRYPT)),
			i = 0 ; i < Dmc_crypt->seg_active ; cp++, i++ )
	    {
		if ( cp->db_tab_base == tcb->tcb_rel.reltid.db_tab_base )
		{
		    r->rcb_enckey_slot = i;
		    break;
		}
	    }
	}

	return (E_DB_OK);
    } while (0);


    /*
    ** Error handling:
    **
    **     Release RCB if it was allocated before the error.
    **	   Unfix the TCB and release any table lock.
    **	   also release shadow and archive RCBs if necesary
    */
    if (rcb)
    {
	if (r->rep_shad_rcb)
	    local_status = dm2t_close(r->rep_shad_rcb, (i4)0, &local_dberr);

	if (r->rep_arch_rcb)
	    local_status = dm2t_close(r->rep_arch_rcb, (i4)0, &local_dberr);

	if (r->rep_shadidx_rcb)
	    local_status = dm2t_close(r->rep_shadidx_rcb, (i4)0, &local_dberr);

	if (r->rep_prio_rcb)
	    local_status = dm2t_close(r->rep_prio_rcb, (i4)0, &local_dberr);

	/*
	** Release the readnolock buffer before deallocating the rcb.
	*/
	if (r->rcb_rnl_cb)
	    dm0m_deallocate((DM_OBJECT **) &r->rcb_rnl_cb);
	local_status = dm2r_release_rcb(&rcb, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    /*
    ** Unfix the TCB if we have fixed it.
    */
    if (tcb)
    {
	local_status = dm2t_unfix_tcb(&tcb, DM2T_VERIFY, lock_id, log_id, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    /*
    ** Release any table lock we have acquired (table or control).
    ** (Note that if the database is open exclusively, then no table lock
    ** was actually taken, even though the table_lock and grant_mode were set.
    */
    if (control_lock)
    {
	local_status = dm2t_control(dcb, table_id->db_tab_base, lock_id,
	    LK_N, (i4)0, timeout, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		&local_error, (i4)0);
	}
    }
    if ( mvcc_lock && mvcc_lkid.lk_unique )
    {
        local_status = dm2t_mvcc_lock(dcb, (DB_TAB_ID*)NULL, lock_id,
	    LK_N, (i4)0, &mvcc_lkid, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		&local_error, (i4)0);
	}
    }

    if (table_lock && (lockid.lk_unique) &&
		((dcb->dcb_status & DCB_S_EXCLUSIVE) == 0))
    {
	stat = LKrelease((i4)0, lock_id, (LK_LKID *)&lockid,
	    (LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, (char *)NULL, (i4)0,
		(i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		0, lock_id);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	SETDBERR(dberr, 0, E_DM9276_TBL_OPEN);
    }

    *record_cb = NULL;
    return (status);
}

/*{
** Name: dm2t_close	- Close a table.
**
** Description:
**	This function closes an open instance of a table.  The caller's
**	Record Control Block (RCB) is returned and the reference count
**	to the Table Control Block (TCB) is decremented.
**
**	The TCB remains in the server cache and its files open (unless
**	the PURGE option is specified).  If the TCB's reference count
**	reaches zero, it becomes a candidate for deallocation.
**
**	Any pages left fixed in the RCB's page pointers are unfixed
**	automatically.  Table locks requested at dm2t_open time are not
**	released and will remain until the transaction completes.  However,
**	any table control locks taken at dm2t_open time are released.
**
**	The following flags modify the default close behavior:
**
**	    DM2T_PURGE - The TCB is tossed from the table cache.  Its cache
**		    lock (if multi-server) is invalidated so that other servers
**		    will also purge their copies.  This option is used for
**		    operations which modify descriptor information making the
**		    TCB out-of-date.  The caller must have the table locked
**		    exclusively to specify this option.  If a partitioned
**		    master is purged, all its partitions are purged too.
**	    DM2T_NORELUPDAT - Indicates that row and tuple count information
**		    should not be updated in iirelation even if if the tcb
**		    page_adds and tup_adds fields indicate that new row/pages
**		    have been added.
**
**	Callers dealing with partitioned tables should close all partitions
**	before closing the master, to prevent potential unfix problems if the
**	master is no longer in use by anyone.
**
****
** Inputs:
**      rcb                             The RCB of the table to close.
**	flag				Optional operations:
**					    DM2T_PURGE
**					    DM2T_NORELUPDAT
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1986 (derek)
**          Created new for jupiter.
**	31-Mar-1987 (ac)
**	    Added read/nolock support.
**	20-sep-1988 (rogerk)
**	    Check if there is read-nolock buffer to deallocate before calling
**	    dm0m_deallocate.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  When closing RCB for gateway
**	    table, call gateway to close the foreign table.
**	17-dec-1990 (jas)
**	    Smart Disk project integration:  free Smart Disk data structures,
**	    if any, when a table is closed.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: Added DM2T_DESTROY flag.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:
**	    Turned off DM2T_PURGE flag if close-purge called on a core system
**	    catalog.  Previously purge operations were ignored because the
**	    tcb_ref_count was always non-zero on them.  The release code no
**	    longer just skips purge requests on fixed tables, so we must make
**	    sure we don't ask for a purge on a nonfree table.
**	26-oct-1992 (rogerk)
**	    Added logging of error return from gateway close before just
**	    continuing with the operation.
**	14-nov-1992 (rogerk)
**	    Rewritten as part of the 6.5 Recovery Project.
**	      - added dm2t_unfix_tcb call
**	20-jun-1994 (jnash)
**	    fsync project: If tbio_sync_close == true, purge the tcb as it
**	    it is inappropriate for subsequent openers.  Files are
**	    forced dm2t_release_tcb().
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL ... and READLOCK=
**          READ_COMMITTED/REPEATABLE_READ (originally proposed by bryanp)
**      19-may-1994 (lewda02/shust01)
**          RAAT API Project:
**          Altered test used to free readnolock pages from RCB.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          If called by transaction abort drop LK_X TCB lock to LK_S.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Change rcb_k_duration to rcb_iso_level
**	29-alpr-97 (stephenb)
**	    Close replicator cdds lookup table if it's open.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          If isolation level is CS or RR, release all unneeded locks
**          on this table. 
**      13-jun-97 (dilma04)
**          Cursor Stability/Repeatable Read optimizations:
**          - turn on CSRR locking mode, if isolation level is CS or RR. This
**          allows dm0p_unfix_page() set flags to release page locks; 
**          - release leaf page's lock if page has not been updated.
**	18-jun-97 (stephenb)
**	    Null out the replicator RCBs after closing the tables.
**      14-oct-97 (stial01)
**          dm2t_close() Changes to distinguish readlock value/isolation level
**          Readlock = nolock is possible with ANY isolation level, 
**          unlock_table = FALSE if readlock nolock
**      12-nov-97 (stial01)
**          Pass lockKEY even if unlocking with lockID (for set lock_trace)
**          Consolidated rcb fields into rcb_csrr_flags.
**      07-jul-98 (stial01)
**          dm2t_close() Continue table close even if unlock page or row fails.
**	11-Aug-1998 (jenjo02)
**	    Roll rcb_tup_adds, rcb_page_adds, into TCB.
**	    Plugged rcb_log_id memory leak; when passed to dm2t_unfix_tcb(),
**	    the RCB has been released and moved to the TCB's free list where
**	    it's free for pickin' by another thread.
**      19-oct-98 (stial01)
**          dm2t_close() Fixed SEGV in CURSOR STABILITY unlock_table code
**	05-nov-98 (devjo01)
**	    XIP stial01 change 438294. (possible SEGV from ref through
**	    RCB structure already put on free list.  Info required is
**	    now safely copied off into local vars.   Also cleaned up
**	    function to consistently use 'r' instead of 'r' and 'rcb'
**	    in an arbitrary mismash.
**      09-feb-2000 (stial01)
**          dm2t_close() test RCB_UPD_LOCK before downgrade, skip errors B100424
**	08-Nov-2000 (jenjo02)
**	    When releasing/returning an RCB for an updated
**	    session temp table, update savepoint id in
**	    table's pending destroy XCCB.
**	07-nov-2002 (somsa01)
**	    Make sure we close off any load table files too.
**	26-nov-2002 (somsa01)
**	    Amended last change to close off any temporary load table
**	    files if mct_recreate is TRUE.
**      09-Jul-2003 (huazh01)
**          For readlock = nolock, do not attempt to unlock the table.
**          This fixes bug 110547, INGSRV 2407
**	14-Jan-2004 (schka24)
**	    Use generic return-rcb routine instead of inline duplication.
**	22-Jan-2004 (jenjo02)
**	    Call dmrThrClose() before getting rid of the RCB.
**	    dm1r_unlock_row() now takes (LK_LOCK_KEY*) instead
**	    of (DM_TID*).
**	28-Jul-2004 (schka24)
**	    tid is unsigned, some compilers may compile tid == -1 as FALSE.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**    17-jun-2005 (huazh01)
**        For read-nolock RCB, release the control lock only after 
**        we unfix the table-control-block.
**        This fixes bug 109872, INGSRV 2163. 
**    27-Jun-2008 (ashco01) B120565
**        Check that the table is not a Gateway table before attempting to
**        unlock it.
*/
DB_STATUS
dm2t_close(
DMP_RCB		*rcb,
i4		flag,
DB_ERROR	*dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*tcb = r->rcb_tcb_ptr;
    DMP_DCB             *d = tcb->tcb_dcb_ptr;
    i4		lock_list = r->rcb_lk_id;
    i4		log_id = r->rcb_log_id;
    i4		unfix_flags;
    i4		local_error;
    DM2R_L_CONTEXT      *lct;
    DB_STATUS		status = E_DB_OK;
    bool                unlock_table = FALSE;
    bool		control_lock = FALSE;
    CL_ERR_DESC         sys_err;
    LK_LOCK_KEY         lock_key;
    i4             base_table_id;
    i4             dcb_id;
    DB_TRAN_ID          tran_id;
    DB_TAB_NAME         relid;
    DB_DB_NAME          dcb_name;
    LK_VALUE            lockvalue;
    STATUS              stat;
    bool                rnl_cb = FALSE;    /* read-nolock RCB */
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    LK_LKID		mvcc_lkid;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9321_DM2T_CLOSE);
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_CLOSE      (%~t,%~t)\n",
	sizeof(tcb->tcb_dcb_ptr->dcb_name), &tcb->tcb_dcb_ptr->dcb_name,
	sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid);
#endif

    dmf_svcb->svcb_stat.tbl_close++;
    for (;;)
    {
	/*
	** close shadow, archive and priority lookup tables if opened
	*/
	if (r->rep_shad_rcb)
	{
	    status = dm2t_close(r->rep_shad_rcb, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	    r->rep_shad_rcb = NULL;
	}
	if (r->rep_arch_rcb)
	{
	    status = dm2t_close(r->rep_arch_rcb, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	    r->rep_arch_rcb = NULL;
	}
	if (r->rep_shadidx_rcb)
	{
	    status = dm2t_close(r->rep_shadidx_rcb, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	    r->rep_shadidx_rcb = NULL;
	}
	if (r->rep_prio_rcb)
	{
	    status = dm2t_close(r->rep_prio_rcb, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	    r->rep_prio_rcb = NULL;
	}
	if (r->rep_cdds_rcb)
	{
	    status = dm2t_close(r->rep_cdds_rcb, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	    rcb->rep_cdds_rcb = NULL;
	}

        /*
        ** If isolation level is CS or RR, turn RCB_CSRR_LOCK flag ON 
        ** to be able to release unneeded locks.
        */
	if ( !crow_locking(r) &&
	       (r->rcb_iso_level == RCB_CURSOR_STABILITY ||
		r->rcb_iso_level == RCB_REPEATABLE_READ) )
        {
            r->rcb_state |= RCB_CSRR_LOCK;
        }

	if (r->rcb_rnl_cb)
	{
	    base_table_id = tcb->tcb_rel.reltid.db_tab_base;
	    control_lock = TRUE;
	}

	/* Save MVCC lock id before tossing RCB */
	mvcc_lkid = r->rcb_mvcc_lkid;

	/* Close dmrAgents(s), move RTB to TCB's idle queue */
	if ( r->rcb_rtb_ptr )
	    dmrThrClose(r);

         
	/*	Unfix any pages saved in the RCB. */
	if (r->rcb_data.page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, 
                dberr);
	    if (status != E_DB_OK)
		break;
	}
	if (r->rcb_other.page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other, 
                dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Downgrade the update lock to shared 
	*/
	if (((r->rcb_iso_level == RCB_SERIALIZABLE) ||
	     (r->rcb_iso_level == RCB_REPEATABLE_READ)) &&
	    (row_locking(r)) &&
	    ((i4) r->rcb_locked_tid.tid_i4 != -1) &&
	    ((r->rcb_state & RCB_UPD_LOCK)) &&
	    ((r->rcb_state & RCB_ROW_UPDATED) == 0) &&
	    (r->rcb_lk_mode == LK_IX))
	{
	    status = dm1r_lock_downgrade(r, &local_dberr);
	    if (status != E_DB_OK)
	    {
		TRdisplay("%@dm2t_close downgrade ROW lock failed\n");
		status = E_DB_OK;
	    }
	}

        /*
	** If not readlock nolock, and isolation level is CS or RR
	** there are locks on this table that can be released,
	** Release them now.
        */
        if (!r->rcb_rnl_cb && r->rcb_state & RCB_CSRR_LOCK)
        {
            if ((r->rcb_csrr_flags & RCB_CS_ROW) 
		&& (r->rcb_state & RCB_ROW_UPDATED) == 0)
            {
                status = dm1r_unlock_row(r, (LK_LOCK_KEY *)NULL, &local_dberr);
		if (status != E_DB_OK)
		{
		    TRdisplay("%@dm2t_close unlock ROW failed\n");
		    status = E_DB_OK;
		}
            }

            if ((r->rcb_csrr_flags & RCB_CS_DATA) && 
                (r->rcb_state & RCB_DPAGE_UPDATED) == 0 &&
                (r->rcb_iso_level != RCB_REPEATABLE_READ ||
                   (r->rcb_state & RCB_DPAGE_HAD_RECORDS) == 0))
            {
                status = dm0p_unlock_page(r->rcb_lk_id, d,
                    &tcb->tcb_rel.reltid, 
		    r->rcb_locked_data, LK_PAGE,
		    &tcb->tcb_rel.relid, &r->rcb_tran_id,
		    &r->rcb_data_lkid, (LK_VALUE *)NULL, dberr);
                if (status != E_DB_OK)
		{
		    TRdisplay("%@dm2t_close unlock DATA failed\n");
		    status = E_DB_OK;
		}
            }

            if ((r->rcb_csrr_flags & RCB_CS_LEAF) &&  
                (r->rcb_state & RCB_LPAGE_UPDATED) == 0) 
            {
                status = dm0p_unlock_page(r->rcb_lk_id, d,
                    &tcb->tcb_rel.reltid, 
		    r->rcb_locked_leaf, LK_PAGE,
		    &tcb->tcb_rel.relid, &r->rcb_tran_id,
		    &r->rcb_leaf_lkid, (LK_VALUE *)NULL, dberr);
		if (status != E_DB_OK)
		{
		    TRdisplay("%@dm2t_close unlock LEAF failed\n");
		    status = E_DB_OK;
		}
            }

            if (r->rcb_lk_mode < RCB_K_X &&
                (r->rcb_state & RCB_TABLE_UPDATED) == 0 &&
		(r->rcb_lk_mode != RCB_K_N) &&
                (r->rcb_iso_level == RCB_CURSOR_STABILITY ||
                    (r->rcb_state & RCB_TABLE_HAD_RECORDS) == 0))
            {
		dcb_id = tcb->tcb_dcb_ptr->dcb_id;
		base_table_id = tcb->tcb_rel.reltid.db_tab_base;
		STRUCT_ASSIGN_MACRO(r->rcb_tran_id, tran_id);
		MEcopy((PTR)&tcb->tcb_rel.relid, sizeof(relid), (PTR)&relid);
		MEcopy((PTR)&tcb->tcb_dcb_ptr->dcb_name, sizeof(dcb_name),
			(PTR)&dcb_name);

                if (r->rcb_gateway_rsb && (tcb->tcb_rel.relstat & TCB_GATEWAY))
                {
                    unlock_table = FALSE;
                }
		else if (r->rcb_k_type == RCB_K_CROW)
		{
                    unlock_table = FALSE;
		}
                else
                    unlock_table = TRUE;
            }
        }

	/*
	** If this was a read-nolock RCB, then release control lock
	** and read-nolock buffers.
	*/
        /* Deallocate the MISC_CB for the readlock pages. */

        if (r->rcb_rnl_cb)
        {
            rnl_cb = TRUE; 
            base_table_id = tcb->tcb_rel.reltid.db_tab_base;

            dm0m_deallocate((DM_OBJECT **) &r->rcb_rnl_cb);

        }

	/*
	** If this table is a gateway table, then tell the gateway that
	** we are closing it.  Check the RSB to make sure that we really
	** have a gateway handle to the table.
	*/
	if (r->rcb_gateway_rsb && (tcb->tcb_rel.relstat & TCB_GATEWAY))
	{
	    /*
	    ** On a Gateway error, continue with the cleanup so that our
	    ** DMF resources can be released after logging the error.
	    */
	    status = dmf_gwt_close(r, dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    (DB_SQLSTATE *)NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
		status = E_DB_OK;
	    }
	}

	/*
	** If this is a file open for loading sorted data,
	** make sure sort has ended before closing it.
	*/
	if ((r->rcb_state & RCB_LSTART) == RCB_LSTART)
	{
	    if (tcb->tcb_lct_ptr != 0)
	    {
		lct = (DM2R_L_CONTEXT*) tcb->tcb_lct_ptr;
		if (lct->lct_srt)
		{
		    status = dmse_end(lct->lct_srt, dberr);
		    if (status != E_DB_OK)
			break;
		    lct->lct_srt = 0;
		}

		/*
		** Make sure we close off any load files.
		*/
		if (lct->lct_mct.mct_recreate &&
		    lct->lct_mct.mct_location &&
		    lct->lct_mct.mct_location->loc_fcb->fcb_state & FCB_OPEN)
		{
		    status = dm2f_close_file(lct->lct_mct.mct_location,
					     lct->lct_mct.mct_loc_count,
					     (DM_OBJECT *)r, dberr);
		    if (status != E_DB_OK)
			break;

		    status = dm2f_release_fcb(lock_list, log_id,
					      lct->lct_mct.mct_location,
					      lct->lct_mct.mct_loc_count,
					      DM2F_ALLOC, dberr);
		    if (status != E_DB_OK)
			break;
		}

		/* Make sure the page buffers allocated for build were freed */
		if (((DM2R_L_CONTEXT*)tcb->tcb_lct_ptr)->lct_mct.mct_buffer
                               != (PTR)0)
		{
		    dm0m_deallocate(
	(DM_OBJECT **)&((DM2R_L_CONTEXT*)tcb->tcb_lct_ptr)->lct_mct.mct_buffer);
		}
		dm0m_deallocate((DM_OBJECT **)&tcb->tcb_lct_ptr);
	    }
	}

	/* Return the RCB to the tcb's free-list, null out r */
	status = dm2r_return_rcb(&r, dberr);
	if (status != E_DB_OK)
	    break;

	/* RCB cannot be referenced now that it's been put on free list */
	rcb = (DMP_RCB*)NULL;

	/*
	** Set options for unfix operation to use whatever the caller
	** specified.   Also include VERIFY to indicate that when we
	** fixed the tcb (in dm2t_open) that we specified that any
	** necessary tcb cache validation be done.
	*/
	unfix_flags = (flag | DM2T_VERIFY);

	/*
	** Turn off any requested PURGE actions on the four primarey
	** core catalogs.  The TCB's for these are fixed in memory by
	** dm2d_open_db and must remain so until the database is closed.
	**
	** There are some operations which will currently request a
	** close-purge of a core catalog after updating table characteristics
	** (like view-base or statistics).  Since these core catalog tcb's
	** cannot be tossed and are never revalidated, this information may not
	** be propogated to other servers.
	**
	** do not disable PURGE for iidevices (DM_B_DEVICE_TAB_ID) 
	*/
	if ((tcb->tcb_rel.relstat & TCB_CONCUR) && 
	    (tcb->tcb_rel.reltid.db_tab_base != DM_B_DEVICE_TAB_ID))
	{
	    unfix_flags &= ~(DM2T_PURGE);
	}
	else if (tcb->tcb_table_io.tbio_sync_close)
	{
	    /*
	    ** TCB's opend sync-at-close must be purged.  Underlying
	    ** files will be forced in dm2t_release_tcb().  Note dependency
	    ** on X table access.
	    */
	    unfix_flags |= (DM2T_PURGE);
	}

	/*
	** Unfix the Table Control Block and release our pointer to it.
	** Pass any requested release flags (which may cause the unfix
	** routine to purge the tcb from the cache).
	*/
	status = dm2t_unfix_tcb(&tcb, unfix_flags, lock_list, log_id,
				dberr);
	if (status != E_DB_OK)
	    break;

        /*
        ** if readlock-nolock RCB, then release the control lock.
        */
        if (control_lock)
        {
            status = dm2t_control(d, base_table_id, lock_list,
                LK_N, (i4)0, (i4)0, dberr);

            if (status != E_DB_OK)
                break;
        }

        if (unlock_table)
        {
	    /*
	    ** Unless we're safely releasing the table lock,
	    ** leave the MVCC lock on in case we get involved
	    ** in transaction abort and rollback. We don't want
	    ** the uninvited to show up while we're in the midst
	    ** of recovery.
	    */
	    if ( mvcc_lkid.lk_unique )
	    {
		status = dm2t_mvcc_lock(d, (DB_TAB_ID*)NULL, lock_list,
			    LK_N, (i4)0, &mvcc_lkid, dberr);

		if (status != E_DB_OK)
		    break;
		if ( DMZ_SES_MACRO(1) )
		{
		    lock_key.lk_type = LK_TBL_MVCC;
		    lock_key.lk_key1 = dcb_id;
		    lock_key.lk_key2 = base_table_id;
		    lock_key.lk_key3 = 0;
		    lock_key.lk_key4 = 0;
		    lock_key.lk_key5 = 0;
		    lock_key.lk_key6 = 0;
		    dmd_lock(&lock_key, lock_list, LK_RELEASE, LK_SINGLE,
			0, 0, &tran_id, &relid, &dcb_name);
		}
	    }

            lock_key.lk_type = LK_TABLE;
            lock_key.lk_key1 = dcb_id;
            lock_key.lk_key2 = base_table_id;
            lock_key.lk_key3 = 0;
            lock_key.lk_key4 = 0;
            lock_key.lk_key5 = 0;
            lock_key.lk_key6 = 0;
            status = LKrelease((i4)0, lock_list,
                (LK_LKID *)NULL, (LK_LOCK_KEY *)&lock_key, (LK_VALUE *)NULL,
                &sys_err);
            if (status != OK)
            {
                uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
                            &local_error, 0);
                uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
                    (char *)NULL, 0L, (i4 *)NULL, &local_error, 1, 0, lock_list);
		SETDBERR(dberr, 0, E_DM926C_TBL_CLOSE);
                break;
            }
            if (DMZ_SES_MACRO(1))
            {
                dmd_lock(&lock_key, lock_list, LK_RELEASE, LK_SINGLE,
                    0, 0, &tran_id, &relid, &dcb_name);
            }
        }

	return (E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	SETDBERR(dberr, 0, E_DM926C_TBL_CLOSE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm2t_fix_tcb   - Find and fix a tcb
**
** Description:
**	This routine finds and fixes a Table Control Block into the TCB cache.
**	If no TCB currently exists, then a new one is built into the cache.
**
**	The TCB reference count is incremented and the TCB is removed from
**	the free table list if it is currently on it.  While fixed, the TCB
**	cannot be removed from the cache.
**
**	When a currently-unfixed TCB is fixed, it may need to be verified
**	before it can be used.  This is because the description information
**	may have been invalidated by a Modify/Index/DMU operation in another
**	server.  When running in multi-server mode, cache locks are used to
**	protect the integrity of TCB information.  All operations which modify
**	TCB infomration invalidate the table's cache lock.  And anytime an
**	unreferenced TCB is fixed that cache lock must be validated.  When
**	an invalid TCB is found it is removed from the cache.
**
**	The caller of this routine is assumed to have guaranteed that this
**	routine will never attempt to rebuild TCB information while a table
**	is concurrently undergoing a Modify-type operation.  This is generally
**	done by obtaining a Table or Table Control lock prior to fixing the
**	TCB.  Callers which request TCB's without locks must specify the
**	NOBUILD flag.
**
**	The server allows for partially-open tables to be used for certain
**	operations (currently recovery actions).  Table Control Blocks for
**	partially-open tables are listed as TCB_PARTIAL.  Callers which
**	require fully open TCB's must specify the DM2T_NOPARTIAL flag to
**	ensure that a partial TCB is not returned.  If a partial TCB is
**	found for a table which is requested with the NOPARTIAL directive,
**	the current TCB is removed from the cache (so that a new full one can
**	be built).
**
**	The dm2t_fix_tcb call can be made to return a TCB for a secondary
**	index as well as a base table.  Since the cache always links 2nd
**	index TCBs onto their base table TCBs, the base table descriptor
**	must be found (and built if necessary) in order to return a 2nd
**	index TCB.
**
**	Somewhat similarly, in order to fix a partition, one must have fixed
**	the master table.  Partitions can't be opened without having the
**	master TCB around.
**
**	The dm2t_fix_tcb call is modified by the following actions:
**
**	    DM2T_VERIFY - indicates that the TCB should be verified if
**			running multi-server and the TCB is not currently
**			fixed by a caller which specified VERIFY.  When fixed
**			with this flag, the caller must also specify the
**			VERIFY flag when it unfixes the table.
**	    DM2T_NOWAIT - if specified then if the TCB is marked BUSY then
**			a W_DM9C51_DM2T_FIX_BUSY warning is returned rather
**			than waiting for the busy status to be turned off.
**	    DM2T_NOBUILD - if specified then if the TCB is not found then
**			a W_DM9C50_DM2T_FIX_NOTFOUND warning is returned rather
**			than building a new TCB.
**	    DM2T_NOPARTIAL - if specified then a partially open TCB will not
**			be returned.  If a partial TCB is found for the
**			requested table it will be tossed and a new one
**			built (unless NOBUILD is also specified).
**	    DM2T_CLSYNC - New TCB is built requesting async writes
**			on file updates, with files forced
**			to disk when the table is closed.  Existing
**			TCB is tossed, if any, and a new one is built.
**          DM2T_RECOVERY - Indicates that the TCB is being used for
**                      rollforwarddb recovery. Therefore we should not
**                      open the underlying files as they may not exist.
**	    DM2T_SHOWONLY - Indicates that the caller is just doing a
**			DMT_SHOW, and does not need the TCB for real table
**			access.  If a "show only" TCB is around, we can
**			use it.  If a TCB must be built, and conditions are
**			not OK for doing real table access (eg lack of a
**			cache for the page size), build a "show only" TCB.
**
** Inputs:
**	db_id				Database id
**	table_id			Pointer to table id
**	tran_id				Pointer to transaction id
**	lock_list			Lock list id
**	log_id				Log id
**	flags				Options to the Fix action:
**					    DM2T_VERIFY
**					    DM2T_NOWAIT
**					    DM2T_NOBUILD
**					    DM2T_NOPARTIAL
**                                          DM2T_RECOVERY
**	dcb 				Database Control block - required
**					unless the NOBUILD flag is specified.
**
** Outputs:
**	tcb				Pointer to pointer to tcb
**	err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	24-may-1993 (bryanp)
**	    Don't use dmd_check messages as traceback messages.
**	20-jun-1994 (jnash)
**	    Add DM2T_CLSYNC flag, causing tcb to be built requesting
**	    async writes on underlying files.  Files are sync'd in
**	    dm2t_close().  When this request received, release any existing
**	    tcb so that a new one will be built with the appropriate flags.
**      12-jul-1994 (ajc)
**          Fix jnash integration such that it will compile.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Add DM2T_RECOVERY flag to table level rollforwarddb
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Under DMCM protocol, TCBs are locked at LK_S and status flag
**          TCB_VALIDATED is set. With flag set, fixers can skip the
**          verify_tcb call as lock prevents it being invalidated by any
**          other server.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	18-may-1998(angusm)
**	    Bug 83251: reset *err_code when tcb successfully fixed,
**	    else the W_DM9C50 warning can propagate up the error chain
**	    nd cause intermittent failures of various queries.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	15-Mar-2005 (schka24)
**	    Add show-only stuff.
**	04-Apr-2008 (jonj)
**	    Check for busy Master if partition TCB is itself not busy.
**	14-Aug-2008 (kschendel) b120791
**	    Init "built" in the loop, since there is a continue statement.
*/
DB_STATUS
dm2t_fix_tcb(
i4		    db_id,
DB_TAB_ID           *table_id,
DB_TRAN_ID          *tran_id,
i4             lock_list,
i4             log_id,
i4		    flags,
DMP_DCB             *dcb,
DMP_TCB		    **tcb,
DB_ERROR            *dberr)
{
    DMP_HCB		*hcb = dmf_svcb->svcb_hcb_ptr;
    DMP_TCB		*return_tcb = NULL;
    DMP_TCB		*t;
    DMP_DCB		*d;
    DMP_TCB		*it;
    DM_MUTEX		*hash_mutex;
    DB_STATUS		status;
    i4		built;
    i4		sync_flag;
    DMP_HASH_ENTRY	*h;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (((flags & DM2T_NOBUILD) == 0) &&
	((dcb == 0) || (dcb->dcb_type != DCB_CB)))
	dmd_check(E_DM9322_DM2T_FIND_TCB);
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_FIX_TCB    (%%d,%d,%d)\n",
	db_id, table_id->db_tab_base, table_id->db_tab_index);
#endif

    for (;;)
    {
	t = NULL;
	built = FALSE;

	status = E_DB_OK;

	/*
	** Search for the Table Control Block in the tcb cache.
	** If not found, build the TCB into the cache.
	**
	** locate_tcb() will return with hash_mutex locked.
	*/
	hash_mutex = NULL;
	locate_tcb(db_id, table_id, &t, &h, &hash_mutex);
	if (t == NULL)
	{
	    /*
	    ** If NOBUILD option specified, then return warning.
	    */ 
	    if (flags & DM2T_NOBUILD)
	    {
		dm0s_munlock(hash_mutex);
		SETDBERR(dberr, 0, W_DM9C50_DM2T_FIX_NOTFOUND);
		status = E_DB_WARN;
		break;
	    }

	    /*
	    ** Build_tcb builds a table control block and puts it in the cache.
	    ** The TCB is not fixed by build_tcb, and is left on the free queue.
	    **
	    ** We are guaranteed, however, that the TCB just built is valid
	    ** so we remember that we did the build operation so we can skip
	    ** the verify_tcb call below.
	    */
	    sync_flag = flags & DM2T_CLSYNC;

	    status = build_tcb(dcb, table_id, tran_id, h, hash_mutex,
		lock_list, log_id, sync_flag, flags, &t, dberr);
	    if (status != E_DB_OK)
	    {
		dm0s_munlock(hash_mutex);
		if (status == E_DB_WARN)
		    continue;
		break;
	    }
	    built = TRUE;
	}

	/* Get pointer to database control block */
	d = t->tcb_dcb_ptr;

	/*
	** Found the table control block.  Check if it is Busy.
	** The Busy status indicates that the tcb is in the midst of being
	** built or destroyed.  Wait until that action is complete and
	** retry the tcb search. (We have to research because the busy action
	** may have been to remove and delete the tcb).
	**
	** If partition TCB and it's not busy but it's Master is,     
	** apply same logic.
	**
	** If the NOWAIT option was specified, then return with a warning.
	*/
	if ( t->tcb_status & TCB_BUSY ||
	     (t->tcb_rel.relstat2 & TCB2_PARTITION &&
	      t->tcb_pmast_tcb->tcb_status & TCB_BUSY) )
	{
	    if (flags & DM2T_NOWAIT)
	    {
		dm0s_munlock(hash_mutex);
		SETDBERR(dberr, 0, W_DM9C51_DM2T_FIX_BUSY);
		status = E_DB_WARN;
		break;
	    }

	    /*
	    ** Note that the wait call here releases the hash mutex.
	    */
	    if ( t->tcb_status & TCB_BUSY )
		dm2t_wait_for_tcb(t, lock_list);
	    else
		dm2t_wait_for_tcb(t->tcb_pmast_tcb, lock_list);

	    continue;
	}

	/*
	** Check for Partial TCB - if this is a tcb with only low level IO
	** information used by the buffer manager.  If a full TCB is
	** needed, we toss out this partial TCB so we can build a new one.
	**
	** Similarly, if we need a "real" TCB, and all we have is a show-
	** only one, pitch it and try again.  (In this case, the build is
	** unlikely to succeed, and we'll let the appropriate error occur.)
	**
	** If the partial tcb is currently in use then we wait for it
	** to be unfixed and retry.
	**
	** Sync-at-close opens introduce the requirement that tcb's built
	** prior to a DM2T_CLSYNC request be tossed when the
	** request is received.  This allows a new tcb to be built with the
	** correct flag.  The close-at-sync tcb will be tossed when
	** when table is closed.
	*/
	if (
	    ((t->tcb_status & TCB_PARTIAL) && (flags & DM2T_NOPARTIAL))
	  ||
	    ((t->tcb_status & TCB_SHOWONLY) && (flags & DM2T_SHOWONLY) == 0)
	  )
	{
	    /*
	    ** If the tcb is currently in use then wait for all current fixers
	    ** to unfix it (note that the wait below will wake up each time
	    ** the tcb is unfixed, so it is possible for us to loop through
	    ** this case more than once before getting an unreferenced tcb.
	    */
	    if ( t->tcb_ref_count )
	    {
		/*
		** Register reason for waiting.  Done for debug/trace purposes.
		*/
		t->tcb_status |= TCB_TOSS_WAIT;

		/* Note that the wait call returns without the hash mutex. */
		dm2t_wait_for_tcb(t, lock_list);
		continue;
	    }

	    /*
	    ** Get rid of the uninteresting partial or show-only TCB.
	    */

	    status = dm2t_release_tcb(&t, (i4)0, lock_list, log_id, dberr);
	    dm0s_munlock(hash_mutex);
	    if (status != E_DB_OK)
		break;
	    continue;
	}

	/*
	** Verify that the TCB is valid and has not been made out of date
	** by some sort of DMU or Alter statement run by another server
	** (any such operation done in this server will have either changed
	** the TCB directly or tossed it from the cache).
	**
	** Validation is not required in the following circumstance where we
	** can guarantee the tcb is OK:
	**
	**   o The TCB is already fixed by someone and it was validated by
	**     them.  While it is fixed it is protected by table or table
	**     control locks.
	**
	**   o The Database is open in sole-server mode (dcb_served not
	**     DCB_MULTIPLE) - any operation which could have invalidated
	**     the TCB must have been run from this server.
	**
	**   o The TCB is for a temp table - no other session can affect it.
	**
	**   o We have just built the TCB.
	**
	**   o The TCB is for a partition - we validate on the master.  Utility
	**     operations that mess with partitions always mark the master too.
	**
	** If we find an invalid TCB, we toss the one we have and retry the
	** tcb search loop expecting this time to build a new one.
	**
	** If the VERIFY flag is not specified, then the TCB is returned to
	** the caller without validation; it is assume the caller knows
	** how to validate it.  Note that in this case, the tcb_valid_count
	** will not be incremented below and the next fixer may have to
	** validate the tcb (and may end requesting to toss it even though
	** there is an outstanding fix of the TCB).
        **
        ** (ICL phil.p)
        ** If the TCB is marked TCB_VALIDATED, it has a shared lock on it
        ** and hence we can skip the verify_tcb call.
	*/
	if ((t->tcb_valid_count == 0) &&
	    (d->dcb_served == DCB_MULTIPLE) &&
	    (t->tcb_temporary == TCB_PERMANENT) &&
	    (built == FALSE) &&
            (flags & DM2T_VERIFY) &&
	    ((t->tcb_rel.relstat2 & TCB2_PARTITION) == 0) &&
            ((t->tcb_status & TCB_VALIDATED) == 0))
	{
	    status = verify_tcb(t, dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** DB_INFO indicates invalid TCB.  Any other error means
		** we're messed up.
		*/
		if (status != E_DB_INFO)
		{
		    dm0s_munlock(hash_mutex);
		    break;
		}

		/*
		** If the TCB is currently referenced (unlikely, yet possible)
		** then we have to wait for it to become unreferenced in order
		** to toss it.  We wait for whoever has it to unfix it and
		** then retry.
		*/
		if ( t->tcb_ref_count )
		{
		    /*
		    ** Register reason for waiting for debug/trace purposes.
		    */
		    t->tcb_status |= TCB_TOSS_WAIT;

		    /* Note that the wait call returns without the hash mutex. */
		    dm2t_wait_for_tcb(t, lock_list);
		    continue;
		}

		/*
		** Release this invalid TCB.  Specify to toss any cached pages
		** for this table as they are probably invalid as well.
		*/
		status = dm2t_release_tcb(&t, DM2T_TOSS, lock_list, log_id, dberr);
		dm0s_munlock(hash_mutex);
		if (status != E_DB_OK)
		    break;
		continue;
	    }
	}

	/*
	** Up to this point, we have been dealing only with base table TCB's.
	** If the caller has actually asked for a 2nd index tcb, then search
	** for the index on this base tcb's index queue.
	*/
	return_tcb = t;
	if (table_id->db_tab_index > 0)
	{
	    /* Loop through index queue looking for matching table id */

	    for (it = t->tcb_iq_next;
		it != (DMP_TCB *) &t->tcb_iq_next;
		it = it->tcb_q_next)
	    {
		if (it->tcb_rel.reltid.db_tab_index == table_id->db_tab_index)
		{
		    return_tcb = it;
		    break;
		}
	    }

	    if (it == (DMP_TCB *) &t->tcb_iq_next)
	    {
		/* If can't find index return error. */

		dm0s_munlock(hash_mutex);
		SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);
		status = E_DB_ERROR;
		break;
	    }
	    if ((it->tcb_status & TCB_SHOWONLY) && (flags & DM2T_SHOWONLY) == 0)
	    {
		/* An index TCB was marked show-only and we don't want that.
		** Ditch out the base and all index TCB's, loop again to
		** redo the build, which will issue the appropriate error
		** message this time.
		*/
		/* Wait in the usual manner if the base TCB is being used. */
		return_tcb = NULL;
		if ( t->tcb_ref_count )
		{
		    /* Register reason for waiting. */
		    t->tcb_status |= TCB_TOSS_WAIT;

		    /* the wait call returns without the hash mutex. */
		    dm2t_wait_for_tcb(t, lock_list);
		    continue;
		}

		status = dm2t_release_tcb(&t, (i4)0, lock_list, log_id, dberr);
		dm0s_munlock(hash_mutex);
		if (status != E_DB_OK)
		    break;
		continue;
	    }
	}

	/*
	** Now that we've verified that the tcb we want exists and is valid,
	** increment its reference count and take it off of the free tcb list
	** (if its on the free list).
	*/
	if (t->tcb_ref_count++ == 0)
	{
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    t->tcb_ftcb_list.q_prev->q_next = t->tcb_ftcb_list.q_next;
	    t->tcb_ftcb_list.q_next->q_prev = t->tcb_ftcb_list.q_prev;
	    t->tcb_ftcb_list.q_next = t->tcb_ftcb_list.q_prev = &t->tcb_ftcb_list;
	    debug_tcb_free_list("off",t);
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}

	/*
	** If the caller passed in the VERIFY flag we can assume that
	** the caller protects this TCB from being invalidated while it is
	** fixed so we increment the tcb_valid_count.  When this count reaches
	** zero again (by unfix actions) the next caller may have to validate
	** the TCB again.
	**
	** We also currently assume that all verify-fixers are opening RCB's
	** on the table.
	*/
	if (flags & DM2T_VERIFY)
	{
	    t->tcb_valid_count++;
	    t->tcb_open_count++;
	}

	dm0s_munlock(hash_mutex);
	status = E_DB_OK;
	break;
    }

    /*
    ** If no fatal error was returned, then return the correct tcb.
    **
    ** Note that the warning statuses:
    **     W_DM9C50_DM2T_FIX_NOT_FOUND
    **     W_DM9C51_DM2T_FIX_BUSY
    ** are returned from here.
    */
    if (DB_SUCCESS_MACRO(status))
    {
	*tcb = return_tcb;
	if ( (return_tcb != NULL) && (dberr->err_code == W_DM9C50_DM2T_FIX_NOTFOUND) )
	     CLRDBERR(dberr);

	if (DMZ_AM_MACRO(13) && return_tcb && 
		(return_tcb->tcb_rel.relspec == TCB_BTREE 
		    || return_tcb->tcb_rel.relspec == TCB_RTREE))
	{
	    t = return_tcb;
	    TRdisplay(
	    "fix_tcb(%d,%d): Leaf  attcnt=%d klen=%d kperpage %d\n",
		t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
		t->tcb_leaf_rac.att_count, t->tcb_klen,
		t->tcb_kperleaf);
	    TRdisplay(
	    "fix_tcb(%d,%d): Index attcnt=%d klen=%d kperpage %d\n",
		t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
		t->tcb_index_rac.att_count, t->tcb_ixklen,
		t->tcb_kperpage);
	
	}
	return (status);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C8A_DM2T_FIX_TCB);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm2t_unfix_tcb   - Unfix a tcb
**
** Description:
**	This routine unfixes a TCB which was previously returned from a
**	dm2t_fix_tcb call.
**
**	The TCB's reference count is decremented, but the TCB remains in
**	the server table cache (unless the flags parameter specifies to
**	toss it).
**
**	If the DM2T_PURGE option is specified then the TCB is tossed from
**	the server table cache (and any modified pages are written from
**	the page cache).  In order to toss a TCB there must be no outstanding
**	fixes of the table.  If the reference count is not zero when a purge
**	request is made, then this routine will wait for current fixers to
**	release the tcb so that the purge can be done.  It is expected that
**	the caller will have ensured that any other fixers will soon release
**	the tcb (this is normally done by holding an Exclusive Table Lock -
**	meaning that the only other fixers of the table can be short term
**	fixes by background threads).
**
**	If the TCB becomes unreferenced after being unfixed and the tcb count
**	of new pages/tuples exceeds 500 rows, 100 pages or 10% of the current
**	values in iirelation, then iirelation is updated.
**
**	After unfixing a TCB, this routine checks for current waiters on
**	the TCB and wakes them up.  There may be other sessions waiting for
**	exclusive access to this Table Descriptor.
**
**	The dm2t_unfix_tcb call is modified by the following actions:
**
**	    DM2T_VERIFY - indicates that the TCB was fixed with the VERIFY
**			option.  When unfixed, this flag indicates to decrement
**			the count of fixes which have verified the tcb.
**	    DM2T_PURGE - the tcb is tossed from the cache and its cache lock
**			(if there is one) is invalidated.
**	    DM2T_TOSS - the tcb is tossed from the cache if not referenced.
**	    DM2T_NORELUPDAT - specifies that iirelation page/tuple counts
**			should not be updated even if the page_adds or tup_adds
**			counts exceed 10% of the current stored values.
**
**	If the TCB is a partitioned master TCB, and the unfix actually ends up
**	purging the TCB, all partitions will be purged as well.
**
** Inputs:
**	tcb				Pointer to pointer to tcb
**	flags				Options to the Fix action:
**					    DM2T_VERIFY
**					    DM2T_PURGE
**					    DM2T_TOSS
**					    DM2T_NORELUPDAT
**	lock_list			Lock list id
**
** Outputs:
**	tcb				Nullified when tcb_ref_count
**					decremented.
**	err_code
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	18-oct-1993 (rogerk)
**	    Took out support for PEND_DESTROY tcb state.  This was used for
**	    temporary tables to allow a destroy request when the caller held
**	    multiple fixes of the same tcb.  The caller must now wait until
**	    the last close of the tcb to request the destroy.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Implementation of callbacks on TCB cache locks for the
**          Distributed Multi-Cache Management (DMCM) Protocol.
**          The last unfixer should flush all dirty pages from the cache if
**          the TCB_FLUSH flag is set and release will cause the lock
**          to be dropped to NULL and the TCB_VALIDATED status unset.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and subsequent corruption of 
**	    tcb_ref_count. Added sanity check of TCB.
**	11-Jun-2004 (schka24)
**	    Remove unused destroy-at-close flag, could cause rdf problems.
*/
DB_STATUS
dm2t_unfix_tcb(
DMP_TCB		**tcb,
i4		flags,
i4		lock_list,
i4		log_id,
DB_ERROR	*dberr)
{
    DMP_DCB		*dcb;
    DMP_HCB             *hcb = dmf_svcb->svcb_hcb_ptr;
    DMP_TCB		*base_tcb;
    DMP_HASH_ENTRY	*h;
    DM_MUTEX	*hash_mutex;		/* base/master's hash mutex */
    i4		release_flag = 0;
    i4		release_warning = FALSE;
    DB_STATUS		status = E_DB_OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Sanity check TCB */
    if ( !tcb || !(*tcb) || (*tcb)->tcb_type != TCB_CB )
	dmd_check(E_DM9C53_DM2T_UNFIX_TCB);

    /*
    ** Get pointer to base table tcb (in case this is a secondary index).
    ** Fix actions are always performed on the base table, not on secondaries.
    */
    base_tcb = (*tcb)->tcb_parent_tcb_ptr;
    dcb = base_tcb->tcb_dcb_ptr;
    h = base_tcb->tcb_hash_bucket_ptr;
    hash_mutex = base_tcb->tcb_hash_mutex_ptr;

    for (;;)
    {
	/*
	** Lock the TCB hash entry during the unfix operation.
	*/
	dm0s_mlock(hash_mutex);

        /*
        ** (ICL phil.p)
        ** Store the FLUSH flag in case we are unable to flush the TCB on
        ** this call. It will be flushed on the last unfix.
        */
        if (flags & DM2T_FLUSH)
        {
            base_tcb->tcb_status |= TCB_FLUSH;
        }

	/*
	** If a purge request was made, and the TCB is currently
	** in use by someone else, then we have to wait for exclusive
	** access to the tcb before we can purge it.
	**
	** There is a strong assumption here that callers which request a
	** purge operation will have taken steps to assure that such a
	** request can be honored:
	**
	**     they have exclusive access to the table
	**     they do not have another outstanding fix of the table
	**
	** If these rules are obeyed, then only temporary non-locking
	** fixers can have access to the tcb (like the buffer manager),
	** and we are guaranteed that those clients will shortly release
	** their accesses.
	**
	** Note that this wait will wake up each time an unfix is done
	** on our tcb - which means that if several threads have the
	** tcb fixed we will wake up and go back to sleep several times
	** before finally gaining exclusive access to the tcb.  It's also
	** theoretically possible with these protocols (although not really
	** likely) for a livelock situation to occur where background jobs
	** are constantly fixing and unfixing the tcb so that we never
	** get exclusive access.
        **
        ** If the TCB is being released, then the ref. count will not be
        ** decremented until the  release is complete; and the session
        ** doing the release could be this one.
	*/
	if ( ( !(base_tcb->tcb_status & TCB_BEING_RELEASED) ) &&
             (flags & DM2T_PURGE) && (base_tcb->tcb_ref_count > 1))
	{
	    /*                   
	    ** Consistency Check:  check for a caller requesting a purge
	    ** operation on a table that is fixed by an actual user thread
	    ** rather than some background job.  This is an unexpected
	    ** condition as purge requests should only be made on tables
	    ** which are locked exclusively by the caller.  Such a request
	    ** may result in a "hanging server".
	    */
	    if (base_tcb->tcb_open_count > 1)
	    {
		/*
		** Log unexpected condition and try continuing.  At worst
		** the session will hang.
		*/
		uleFormat(NULL, E_DM9C5D_DM2T_PURGEFIXED_WARN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
		    (i4 *)NULL, err_code, (i4)6,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    sizeof(DB_TAB_NAME), base_tcb->tcb_rel.relid.db_tab_name,
		    sizeof(DB_OWN_NAME), base_tcb->tcb_rel.relowner.db_own_name,
		    0, base_tcb->tcb_open_count, 0, base_tcb->tcb_ref_count,
		    0, base_tcb->tcb_status);
	    }

	    /*
	    ** Register reason for waiting - also do consistency check
	    ** to make sure that two callers are not waiting to purge
	    ** the same tcb.
	    */
	    if (base_tcb->tcb_status & TCB_PURGE_WAIT)
	    {
		dm0s_munlock(hash_mutex);
		uleFormat(NULL, E_DM9C5F_DM2T_PURGEBUSY_WARN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
		    (i4 *)NULL, err_code, (i4)6,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    sizeof(DB_TAB_NAME), base_tcb->tcb_rel.relid.db_tab_name,
		    sizeof(DB_OWN_NAME), base_tcb->tcb_rel.relowner.db_own_name,
		    0, base_tcb->tcb_open_count, 0, base_tcb->tcb_ref_count,
		    0, base_tcb->tcb_status);
		SETDBERR(dberr, 0, E_DM9C53_DM2T_UNFIX_TCB);
		status = E_DB_ERROR;
		break;
	    }

	    base_tcb->tcb_status |= TCB_PURGE_WAIT;

	    /* Note that the wait call returns without the hash mutex. */
	    dm2t_wait_for_tcb(base_tcb, lock_list);
	    continue;
	}

	/*
	** Decrement the reference count.
	** If the caller protected the TCB from invalidation, then decrement
	** the count of such protectors.
	*/

	/*
	** When we decrement the reference count, destroy the
	** caller's TCB pointer so that we can't accidentally be
	** recalled to unfix the same TCB by the same transaction
	** again and corrupt tcb_ref_count.
	*/
	base_tcb->tcb_ref_count--;
	*tcb = (DMP_TCB*)NULL;

	if (flags & DM2T_VERIFY)
	{
	    base_tcb->tcb_valid_count--;
	    base_tcb->tcb_open_count--;
	}

	/*
	** If our unfix of the TCB makes it completely unfixed, then
	** return the TCB to the free list (or release it completely
	** if a PURGE operation was specified).
	**
	** Temporary TCB's are not placed on the free queue but are just
	** left on the tcb hash queue's in an unfixed state.
        **
        ** horda03 - if the TCB is being released, then we're done.
        **
	*/
	if ((base_tcb->tcb_ref_count == 0) &&
	    (base_tcb->tcb_temporary == TCB_PERMANENT))
	{   
	    /*
	    ** Consistency Check:  If the tcb is unreferenced, but its
	    ** valid count is still non-zero, then somebody is fixing the
	    ** tcb specifying VERIFY, but is then not unfixing the TCB
	    ** with the same flag.  This could lead to us thinking a TCB
	    ** is valid when it really isn't!
	    */
	    if (base_tcb->tcb_valid_count != 0)
	    {
		uleFormat(NULL, E_DM9C52_VALID_COUNT_MISMATCH, (CL_ERR_DESC *)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)3,
		    sizeof(dcb->dcb_name), &dcb->dcb_name,
		    sizeof(base_tcb->tcb_rel.relid), &base_tcb->tcb_rel.relid,
		    0, base_tcb->tcb_valid_count);
		base_tcb->tcb_valid_count = 0;
		base_tcb->tcb_open_count = 0;
	    }

	    /*
	    ** Set flags for possible release_tcb call.
	    ** Check for necessity to update tuple or page counts.
	    **
	    ** On a normal unfix, we update tuple/page counts if one
	    ** of them changed by more than 10% and at least 100 tuples
	    ** or 20 pages were added/deleted (unless the NORELUPDAT).
	    **
	    ** Honor NORELUPDAT if combined with other flags.
	    */
	    if (flags & (DM2T_PURGE | DM2T_TOSS))
	    {
		release_flag = DM2T_TOSS;
		if (flags & DM2T_PURGE)
		    release_flag = DM2T_PURGE;
	    }
            else if (base_tcb->tcb_status & TCB_FLUSH)
            {
                /*
                ** (ICL phil.p)
                ** Write out all dirty pages and
                ** convert tcb cache lock to null.
                */
                release_flag = DM2T_FLUSH;
            }
	    else if ((flags & DM2T_NORELUPDAT) == 0
		&& (base_tcb->tcb_status & TCB_SHOWONLY) == 0)
	    {
		i4	    tup_adds = base_tcb->tcb_tup_adds;
		i4	    page_adds = base_tcb->tcb_page_adds;

		if (tup_adds < 0)
		    tup_adds = -tup_adds;
		if (page_adds < 0)
		    page_adds = -page_adds;

		if ((tup_adds > 500) || (page_adds > 100) ||
		    ((tup_adds > 100) &&
		     (tup_adds > base_tcb->tcb_rel.reltups / 10)) ||
		    ((page_adds > 20) &&
		     (page_adds > base_tcb->tcb_rel.relpages / 10)))
		{
		    release_flag = DM2T_KEEP;
		}
	    }

	    /*
	    ** If we are tossing the TCB or just updating tuple/page
	    ** counts, then call release_tcb.  If the release flag
	    ** specifies DM2T_KEEP then our base_tcb pointer will
	    ** still be valid on return from the routine, otherwise
	    ** the tcb memory will have been released.
	    **
	    ** If the release returns E_DB_WARN, then the release could
	    ** not be completed, but the TCB is still valid (regardless
	    ** of whether the KEEP flag was specified).  In the most
	    ** common E_DB_WARN case, a busy TCB, no other action was
	    ** taken by release.  Generally this is not a problem unless
	    ** the caller wanted an explicit PURGE.
	    **
	    ** In the typical case, with no special unfix flags and no
	    ** iirelation updating to do, we don't call tcb-release at all.
	    */
	    if (release_flag)
	    {
		if ( flags & DM2T_NORELUPDAT )
		    release_flag |= DM2T_NORELUPDAT;

		status = dm2t_release_tcb(&base_tcb, release_flag, lock_list,
		    			  log_id, dberr);
		if (status != E_DB_OK)
		{
		    if (DB_FAILURE_MACRO(status))
		    {
			dm0s_munlock(hash_mutex);
			break;
		    }

		    /*
		    ** If the release returned a warning,
		    ** then the TCB still exists.
		    */
		    release_warning = TRUE;
		    status = E_DB_OK;
		}

		/*
		** If the tcb has been successfully released, then break
		** to the bottom since we cannot reference the deallocated
		** tcb memory any longer.
		*/
		if (!base_tcb )
		{
		    dm0s_munlock(hash_mutex);
		    break;
		}
	    }

	    /*
	    ** If the tcb is no longer fixed, put it on the tcb free queue.
	    ** It will be available to be scavenged to make room for new tcb's.
	    ** The TCB goes on the end of the list.
	    */
	    if ( base_tcb->tcb_ref_count == 0 && 
		(base_tcb->tcb_status & TCB_BUSY) == 0 )
	    {
		dm0s_mlock(&hcb->hcb_tq_mutex);
		debug_tcb_free_list("on",base_tcb);
		base_tcb->tcb_ftcb_list.q_next = hcb->hcb_ftcb_list.q_prev->q_next;
		base_tcb->tcb_ftcb_list.q_prev = hcb->hcb_ftcb_list.q_prev;
		hcb->hcb_ftcb_list.q_prev->q_next = &base_tcb->tcb_ftcb_list;
		hcb->hcb_ftcb_list.q_prev = &base_tcb->tcb_ftcb_list;
		dm0s_munlock(&hcb->hcb_tq_mutex);
	    }
	}
	else if ( flags & DM2T_FLUSH )
	{
	    /*
	    ** DMCM callback, but TCB can't be released because
	    ** the reference count is not zero.
	    ** To free the logjam and avoid a deadlock, 
	    ** convert the TCB cache lock to null, marking
	    ** the TCB as needing validation.
	    ** The last unfixer will still see TCB_FLUSH set
	    ** and do all of the "flush" stuff in dm2t_release_tcb.
	    */
	    status = dm2t_convert_tcb_lock(base_tcb, dberr);
	}

	/*
	** Check if there are other sessions waiting for us to unfix the tcb.
	*/
	if (base_tcb->tcb_status & TCB_WAIT)
	    dm2t_awaken_tcb(base_tcb, lock_list);

	dm0s_munlock(hash_mutex);

	break;
    }

    /*
    ** If a purge request was made, but could not be honored, then return
    ** an error.
    */
    if ((release_warning) && (flags & DM2T_PURGE))
	status = E_DB_ERROR;

    if (status == E_DB_OK)
	return (E_DB_OK);


    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C53_DM2T_UNFIX_TCB);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm2t_release_tcb	- Release a TCB
**
** Description:
**      This function tosses a Table Control Block from the TCB cache.
**	It should only be called on base table TCB's and will result in
**	the release of the base table as well as its secondary indexes.
**
**      A busy / still referenced TCB causes a warning return with no
**      other action taken, regardless of flags.
**
**      If called with the DM2T_KEEP option then it really doesn't toss
**      the TCB at all, it just updates the page/tuple counts in iirelation.
**
**	If the TCB supplied is for a partition master, and we're actually
**	tossing it (not just updating iirelation), we toss all partitions.
**	If everyone followed the rules, all the partition TCB's that are
**	still around should be idle.  (If not, it's an internal error.)
**	If we're just updating iirelation, only the given table is updated.
**
**	If the TCB is on the TCB free queue, it is taken off.  The caller
**	should hold the hash mutex on entry to this routine, but should
**	not hold the TCB mutex.
**
**	In the process of releasing the TCB the following actions must be
**	performed:
**
**	    1) The TCB is removed from the TCB free-list if it's on it.
**	    2) If there's some reason to update the table's iirelation
**	       tups/pages, do that (unless the caller said not to).
**	    3) All modified pages for this table and its secondary indexes
**	       are written from the cache.  If the PURGE or TOSS flags are
**	       specified, the cached pages are tossed.
**	    4) The base table file is closed and its FCB's released.
**	    5) Each secondary index file is closed and its FCB's released.
**	    6) Each secondary index TCB is deallocated.
**	    7) The TCB lock value is bumped if this is a PURGE operation.
**	    8) The TCB cache lock is released.
**	    9) The TCB is removed from the TCB hash queue.
**	    10) The TCB memory is deallocated.
**
**	While the release operation is being performed, the TCB is marked
**	BUSY.  All other sessions attempting to reference this table will
**	wait (via dm0s_ewait) until the BUSY bit is turned off.  They must
**	then research the hash queues for the TCB as it may have been
**	deallocated.
**
**	If the TCB cannot be released due to problems in the buffer manager,
**	then a warning will be returned.  The TCB will still be left valid.
**	It is up to the caller to determine the correct action to take.  If
**	the caller was merely selecting a victim to release, it can select
**	a different one.  If the release was critical to caller's operation
**	(ie a modify or destroy) then that operation can not be performed.  If
**	the caller is closing the database, then this represents a possible
**	closedb problem and if the caller cannot force the buffer manager pages,
**	recovery may be needed.
**
**	If the DM2T_PURGE flag is specified, then the TCB cache lock is
**	invaliated so that any other servers will know to toss the TCB from
**	their caches.
**
** Inputs:
**      tcb                             Pointer-to-pointer of 
**					the TCB to release.
**      flag                            Action values:
**					DM2T_UPDATE - update tuple/page counts
**					DM2T_KEEP - don't toss TCB, just do
**					    page/tuple count updates.
**					DM2T_PURGE - table structure changing,
**					    invalidate tcb and cached pages
**					DM2T_TOSS - toss cached pages.
**					    (implies DM2T_NORELUPDAT)
**					DM2T_NORELUPDAT - skip update_rel()
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK			TCB successfully released
**		tcb			    Caller's pointer is nullified
**	    E_DB_WARN			TCB could not be deallocated due to
**					    errors tossing pages from cache,
**					    or it is still fixed or busy.
**					    The TCB is still valid and was
**					    left on the TCB hash queue.
**	    E_DB_ERROR			TCB was not successfully deallocated.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	07-dec-1985 (derek)
**          Created new for jupiter.
**	 6-Feb-89 (rogerk)
**	    Changed tcb hashing to use dcb_id rather than dcb pointer.
**	    Changed normal action value to dm0p_close_page to be DM0P_CLCACHE.
**	    to not toss the pages associated with the table.  The pages are
**	    tossed if the flag value DM2T_PURGE or DM2T_TOSS is specified.
**	    Added call to dm0p_tblcache when PURGE flag is specified.
**	20-feb-89 (rogerk)
**	    Took out build lock and added usage of TCB_BUSY bit.  This allows
**	    TCB's to remain on hash queues while being deallocated (for the
**	    fast commit threads) and allows us to close a table without
**	    requiring a lock (which causes resource problems).
**	    Also added WARNING return when dm0p_close_page fails.
**	15-may-1989 (rogerk)
**	    Check return value of dm0p_tblcache call.
**	19-nov-1990 (bryanp)
**	    bug #33123 -- dmd_check in dm0s_mlock
**	    This routine must ALWAYS return without the hcb_mutex held, even in
**	    error conditions. Added a dmos_unlock() call to the error return
**	    path.
**	30-sep-1991 (walt,bryanp)
**	    Re-acquire the hcb_mutex when removing TCBs from the hash queue.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	26-aug-1992 (25-jan-1990) (fred)
**	    BLOB support:
**          Free up any extended table descriptions which may exist.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project:
**	    Changed order of arguments.  Changed to always call update_rel
**	    if we are releasing a TCB that has new tup_adds or page_adds
**	    rather than needing the DM2T_UPDATE flag.
**	16-feb-1993 (rogerk)
**	    Fix partial TCB problem.  Call dm0p_close_page to toss pages
**	    for a tcb if any of the secondary index tbios have been opened.
**	    We were bypassing the close page if the base table had no open
**	    files which was incorrect if secondary index files were open.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	23-aug-1993 (rogerk)
**	    Changed decision on whether to update reltups values to be based
**	    on DCB_S_RECOVER mode rather than DCB_S_ONLINE_RCP.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	20-jun-1994 (jnash)
**	    fsync project.  Tables opened sync-at-close at forced to disk here.
**	28-feb-1997 (cohmi01)
**	    Keep track in hcb of tcbs reclaimed. Bugs 80242, 80050.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Implementation of TCB callbacks for the Distributed Multi-Cache
**          Management (DMCM) protocol.
**          New status DM2T_FLUSH, causes TCB locked to be converted to NULL,
**          TCB_FLUSH and TCB_VALIDATED statuses cancelled and TCB to be
**          retained on the free list.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Caller must hold tcb's hash entry's hash_mutex.
**	    To clarify earlier comments regarding the hash (hcb) mutex, this
**	    function ALWAYS returns with it locked.
**      23-Jul-2003 (horda03) Bug 110266
**          Return E_DM937E if TCB is busy. Only re-acquire the hash_mutex if
**          the tcb was set busy.
**	14-aug-2003 (jenjo02)
**	    Destroy caller's TCB pointer if TCB memory is deallocated,
**	    sanity check TCB.
**	    Reset TCB_BUSY only if we set it.
**	    Delay marking TCB_BUSY until after update_rel() completes
**	    so background threads (WriteBehind, CP) won't be prevented
**	    from fixing this TCB and continuing their work.
**	15-Jan-2004 (schka24)
**	    Toss partitions when tossing a master.
**	22-Jan-2004 (jenjo02)
**	    Call dmrThrTerminate() before freeing the TCB(s).
**	5-Mar-2004 (schka24)
**	    Clean up partition tcb's via purge rather than direct
**	    recursive release.
**	02-Dec-2004 (jenjo02)
**	    Redid handling of partitions when the Master is
**	    being released. Check first that Master isn't otherwise
**	    busy before releasing its partitions, wait for busy
**	    partitions, "close" them by recursively calling
**	    dm2t_release_tcb rather than purging them, which
**	    really is overkill and fails to update_rel() the
**	    partitions.
**      30-jun-2005 (horda03) Bug 114498/INGSRV3299
**          While updating the iirelation information for the TCB being
**          released, don't allow locate_tcb() to find the TCB by marking
**          the TCB as TCB_BEING_RELEASED. We need to do this as the hash
**          mutex is released while updating iirelation, but we haven't
**          set the BUSY flag (see code for why). Because the ref_count is
**          incremented here, a session opening the TCB would also increment
**          the ref_count and we'd get a hang where both sessions are waiting
**          for the other to decrement the ref_count.
**	15-Aug-2006 (jonj)
**	    Temp Table index support. 
**	    Allow release_tcb on Temp Table Index TCB by itself.
**	    If releasing Temp Table Base, also release all
**	    attached indexes.
**      11-Sep-2006 (horda03) Bug 116613
**          Setting TCB_BEING_RELEASED doesn't prevent locate_tcb() from
**          finding the TCB, instead it signals to other users that a session
**          is already releasing the TCB and has released the TCB while it
**          updates iirelation. During the update of iirelation, the session
**          may need to locate the TCB its releasing. Having locte_tcb()
**          stalling in this circumstance causes a hang.
**      30-Nov-2007 (kibro01 on behalf of kschendel) b120327
**          After an update-rel, return WARN if the TCB remains busy or
**          referenced, just as is done before checking whether update-rel
**          is needed.  The caller could be a reclaim, and some other
**          session could have fixed the TCB during update-rel, and we
**          don't want the reclaim to sit indefinitely while the fixer
**          uses the TCB.
**	04-Apr-2008 (jonj)
**	    Fix a variety of DMCM, shared cache problems. 
**	      o If update_rel() returns "InvalidateTCB", then we must
**	        first purge the cache before bumping the TCB lock value
**		to avoid deadlocking with other sessions that may need
**	        one of our cache locks.
**	      o If not DM2T_KEEP, and DMCM, toss the pages from cache
**		to avoid NO TCB crashes in dm0p_dmcm_callback().
**	      o When releasing on behalf of dm2t_tcb_callback(DM2T_FLUSH),
**		toss all pages from cache, do not keep the TCB.
**	13-May-2008 (jonj)
**	    Fix for fix for B120327, which caused dm2d_close_db()
**	    failures. Rather that positing that the caller -could- be a
**	    reclaim, check for explicit DM2T_RECLAIM, then don't wait
**	    for busy TCB after update_rel().
**      14-Aug-2008 (kschendel) b120791
**          Make sure that busy+reclaim really returns warning; above change
**          repositioned some loops.
**	22-Apr-2010 (kschendel) SIR 123485
**	    TCB purge needs to dump any BQCB, else alter table involving
**	    adding or dropping blob columns will fail.
*/
DB_STATUS
dm2t_release_tcb(
DMP_TCB		**tcb,
i4		flag,
i4		lock_list,
i4		log_id,
DB_ERROR	*dberr)
{
    DMP_TCB		*t;
    DMP_HCB		*hcb = dmf_svcb->svcb_hcb_ptr;
    DMP_DCB		*dcb;
    DB_STATUS		status = E_DB_OK;
    DMP_TCB		*it;
    DMP_TCB		*pt;
    DMT_PHYS_PART	*pp_ptr;		/* Phys partition array pointer */
    i4		close_flag;
    i4		release_fcb_flag;
    bool		close_page_needed;
    i4			unwait_tcb = FALSE;
    i4			sem_released = FALSE;
    bool		InvalidateTCB = FALSE;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Sanity check TCB */
    if ( !tcb || !(t = *tcb) || t->tcb_type != TCB_CB )
	dmd_check(E_DM9C54_DM2T_RELEASE_TCB);

    dcb = t->tcb_dcb_ptr;

#ifdef xDEBUG
    if (DMZ_TBL_MACRO(10))
	TRdisplay("DM2T_RELEASE_TCB(%~t,%~t) %v\n",
	  sizeof(dcb->dcb_name), &dcb->dcb_name,
	  sizeof(t->tcb_rel.relid), &t->tcb_rel.relid,
	  "PURGE,NOUPDATE,UPDATE,KEEP,TOSS,DESTROY,FLUSH", flag);
#endif

    /* If we're not keeping the TCB and it's not otherwise busy, 
    ** release all of its partitions.  It's likely
    ** that this is unnecessary, because partitions are supposed to be closed
    ** before masters.  But there are situations (DMU, DB close, reclaim, ...)
    ** where tables are not released in a "nice" order.  Since we're at the end
    ** of the line, if any partitions are around and not in a state where they
    ** can be released, the recursive release call will get an error.
    ** The hash mutex is still held after this loop, although it may get
    ** released and re-taken if a partition is released, but to keep
    ** the curious out, we'll mark Master's TCB as busy.
    */
    if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED && 
	 (flag & DM2T_KEEP) == 0 && 
	 (t->tcb_status & TCB_BUSY) == 0 &&
	 t->tcb_ref_count == 0 )
    {
	/* Mark master as busy while we do this */
	t->tcb_status |= TCB_BUSY;

	for (pp_ptr = &t->tcb_pp_array[t->tcb_rel.relnparts-1];
	     pp_ptr >= &t->tcb_pp_array[0] && status == E_DB_OK;
	     --pp_ptr)
	{
	    if ( (pt = pp_ptr->pp_tcb) != NULL )
	    {
		/* Wait for busy Partition, retry */
		if ( pt->tcb_status & TCB_BUSY || pt->tcb_ref_count )
		{
		    /*
		    ** Strong assumption here that Partition's 
		    ** hash_mutex is always the same as Masters.
		    */
		    /* Returns without hash_mutex */
		    dm2t_wait_for_tcb(pt, lock_list);
		    dm0s_mlock(t->tcb_hash_mutex_ptr);
		    pp_ptr++;
		}
		else
		    status = dm2t_release_tcb(&pt, flag, 
					lock_list, log_id, dberr);
	    }
	}
	/* Unbusy master. We'll wake up waiters below */
	t->tcb_status &= ~TCB_BUSY;
    }


    while ( status == E_DB_OK )
    {
	/*
	** Verify that this is a base table or partition TCB.
	** Release cannot be called with a secondary index tcb
	** that is not a temporary table index.
	*/
	if ( t->tcb_rel.relstat & TCB_INDEX && !(t->tcb_rel.relstat2 & TCB_SESSION_TEMP) )
	{
	    uleFormat(NULL, E_DM9C70_DM2T_RELEASE_IDXERR, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name);
	    SETDBERR(dberr, 0, E_DM9270_RELEASE_TCB);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Make sure the TCB is not busy or referenced
	** (this should not happen...but might if someone grabs a TCB that's
	** being reclaimed.  Reclaim can't lock the hash mutex until it picks
	** a free TCB...)
	*/
	if ((t->tcb_status & TCB_BUSY) || (t->tcb_ref_count != 0))
	    break;

	/* If we are really tossing the TCB, make sure it's not on
	** the TCB freelist.  (It might not be, in which case the
	** tcb tq pointers should point to themselves, and no harm is
	** done.)
	*/
	if ( (flag & DM2T_KEEP) == 0 )
	{
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    t->tcb_ftcb_list.q_prev->q_next = t->tcb_ftcb_list.q_next;
	    t->tcb_ftcb_list.q_next->q_prev = t->tcb_ftcb_list.q_prev;
	    /* Ensure this can be done again without damage */
	    t->tcb_ftcb_list.q_next = t->tcb_ftcb_list.q_prev = &t->tcb_ftcb_list;
	    debug_tcb_free_list("off",t);
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}

	/*
	** Update reltups and relpages counts if the tcb specifies that new
	** tuples or pages were added during the life of this TCB.
	**
	** Bypass iirelation updates if:
	**
	**     TOSS flag specified: the count fields have likely been
	**         invalidated along with the TCB
	**     This is a temporary table: there's no catalog information for it
	**     This is the RCP or CSP: we don't update counts during recovery.
	**     It's a show-only TCB: can't be used for real table access.
	**     It's a DM2T_FLUSH (DMCM callback). It's a bad idea to be
	**         acquiring additional resources during a lock callback
	**	   when the intent is to unto the SV_TABLE lock incompatability.
	*/
	if ( (t->tcb_page_adds || t->tcb_tup_adds) &&
	     (flag & (DM2T_TOSS | DM2T_NORELUPDAT | DM2T_FLUSH)) == 0 &&
	      t->tcb_temporary != TCB_TEMPORARY &&
	     (t->tcb_status & TCB_SHOWONLY) == 0 &&
	     (t->tcb_dcb_ptr->dcb_status & DCB_S_RECOVER) == 0 )
	{
	    /*
	    ** We can't set TCB_BUSY yet.
	    **
	    ** Once set, the WriteBehind and CP threads won't be
	    ** able to flush pages for this table (they can't
	    ** fix a TBIO for a TCB that's BUSY), and if the 
	    ** table's cache is saturated with modified buffers
	    ** that belong to this table and contains few or no
	    ** free buffers, update_rel() will stall waiting for
	    ** a free buffer which will never materialize.
	    **
	    ** Instead, artificially bump the TCB's 
	    ** reference count while update_rel() does
	    ** its business, then set TCB_BUSY. 
	    **
	    ** This does two things:
	    **     o The WB and CP threads are able to fix
	    **       the TCB and flush these pages while
	    **       update_rel() runs.
	    **	   o If we simply delay setting TCB_BUSY
	    **	     and do not pin tcb_ref_count, the
	    **	     following stack-busting loop might 
	    **	     occur:
	    **		update_rel
	    **		   |
	    **		dm2r_position
	    **		   |
	    **		dm1h_search
	    **		   |
	    **		dm0p_fix_page
	    **		   |
	    **		dm0p_cachefix_page
	    **		   |
	    **		fault_page
	    **		   |
	    **		fix_tableio_ptr (this TCB's tbio, ref == 1)
	    **		   |
	    **		(force modified page)
	    **		   |
	    **		unfix_tableio_ptr
	    **		   |
	    **		dm2t_ufx_tabio_cb
	    **		   |
	    **		dm2t_unfix_tcb (now TCB's ref == 0)
	    **		   |
	    **		dm2t_release_tcb
	    **		   |                                ^
	    **		update_rel starts it all over again |
	    **
	    ** Pinning tcb_ref_count prevents the recursive call
	    ** to dm2t_release_tcb and keeps other threads 
	    ** out of here.
	    **
	    ** After update_rel() completes, we unpin the
	    ** ref_count and wait until all other fixers have
	    ** released the TCB before continuing.
	    **
	    ** If this is a "KEEP" operation (the TCB really
	    ** isn't being obliterated), it matters not if
	    ** there are still fixers, and we return 
	    ** E_DB_WARN to notify the caller of the
	    ** non-zero fix count.
	    */

	    /* Pin the TCB, update iirelation */
	    t->tcb_ref_count++;

            /* Indicate that the TCB is being released.
            */
            t->tcb_status |= TCB_BEING_RELEASED;
	    dm0s_munlock(t->tcb_hash_mutex_ptr);

    	    update_rel(t, lock_list, &InvalidateTCB, dberr);

	    /*
	    ** Unpin the TCB.  If we're RECLAIMing the TCB and it looks
	    ** busy now, return WARN just like we would have above.
	    ** Otherwise, wait for other fixers if not KEEPing the TCB.
	    */
	    dm0s_mlock(t->tcb_hash_mutex_ptr);

	    while ( (--t->tcb_ref_count || t->tcb_status & TCB_BUSY)
		   && !(flag & DM2T_KEEP) )
	    {
		if ( flag & DM2T_RECLAIM )
		{
		    /* Busy during RECLAIM, return E_DB_WARN */
		    dm2t_awaken_tcb(t, lock_list);
		    t->tcb_status &= ~TCB_BEING_RELEASED;
		    SETDBERR(dberr, 0, E_DM937E_TCB_BUSY);
		    return (E_DB_WARN);
		}
		t->tcb_ref_count++; /* Prevent TCB from vanishing */
		t->tcb_status |= TCB_TOSS_WAIT;
		dm2t_wait_for_tcb(t, lock_list);
		dm0s_mlock(t->tcb_hash_mutex_ptr);
	    }

            /* Now the TCB has been unpinned and iirelation updated,
            ** the release of the TCB can be protected by the BUSY
            ** status.
            */
            t->tcb_status &= ~TCB_BEING_RELEASED;
	}


	/*
	** If the caller has specified to not really release the TCB (the
	** caller just wanted to update the tuple/page counts above), then
	** don't continue closing the tcb.
	**
	** But, if update_tcb(), above, suggested invalidating the TCB
	** in other servers and this is DMCM, then we must continue
	** on to flush pages from the cache and release their page locks
	** or we may (silently) deadlock below in dm2t_bumplock_tcb()
	** when we attempt to convert the LK_S to LK_X.
	*/
	if ( !(flag & DM2T_KEEP)
	    || (InvalidateTCB 
		  && dcb->dcb_status & DCB_S_DMCM
		  && t->tcb_lkid_ptr->lk_unique) )
	{
	    /*
	    ** Now mark the TCB busy so that nobody will reference it while we are
	    ** releasing it.  Any callers trying to use this TCB should wait for
	    ** it to be marked not busy and then research the TCB hash array.
	    */
	    t->tcb_status |= TCB_BUSY;
	    unwait_tcb = TRUE;

	    /*
	    ** Release hash mutex while we do actual deallocation.  The tcb must
	    ** be marked busy before we release this.
	    */
	    dm0s_munlock(t->tcb_hash_mutex_ptr);
	    sem_released = TRUE;

	    /*
	    ** Since we are closing the TCB, we need to write out any dirty pages
	    ** for this table from the cache.  Normally, we can leave the non
	    ** modified pages cached - however, if the PURGE or TOSS flag is
	    ** specified, then the pages must be tossed from the cache.
	    **
	    ** This force is required if the table (or any of its secondary
	    ** indexes) has actually been opened.  The underlying close_page
	    ** code will not actually be able to do anything with a non-open
	    ** table.  If the table has any underlying files open, then close_page
	    ** is called to write/toss pages from the cache.  Note that if the
	    ** TCB passed in is only partially open, only those pages which are
	    ** accessable by files open in this TCB can be written/tossed from
	    ** the cache.
	    **
	    ** This force does not guarantee that all dirty pages are flushed
	    ** unless the caller has followed these protocols:
	    **
	    **	- has provided a complete (non-PARTIAL) tcb.  Otherwise,
	    **        if the cache is shared by other servers there may be
	    **        cached pages not accessable by this tcb.
	    **
	    **      - has acquired an exclusive table lock.  Otherwise, there
	    **        may be pages fixed in the cache by other sessions which
	    **        cannot then be tossed.
	    **
	    ** This force will always guarantee that if this is the last open
	    ** TCB for this table by any server connected to this cache, that
	    ** all dirty pages are forced.
	    **
	    ** If we encounter an error forcing pages from the cache then we cannot
	    ** release the TCB.  Requeue the TCB to the free queue and return a
	    ** warning to the caller.
	    */
	    close_page_needed = FALSE;
	    if (t->tcb_table_io.tbio_status & TBIO_OPEN)
		close_page_needed = TRUE;
	    else for (it = t->tcb_iq_next;
		      it != (DMP_TCB *) &t->tcb_iq_next;
		      it = it->tcb_q_next)
	    {
		if (it->tcb_table_io.tbio_status & TBIO_OPEN)
		{
		    close_page_needed = TRUE;
		    break;
		}
	    }


	    /* Show-only's should never be TBIO_OPEN, but just make sure (in case
	    ** something got buggered up with secondary indexes)
	    **
	    ** It's a bad thing to -not- close_page when it appears to
	    ** be needed, even if "show-only" as we don't want modified
	    ** pages left in cache with no TCB left to write them.
	    **
	    if (t->tcb_status & TCB_SHOWONLY)
		close_page_needed = FALSE;
	    **
	    */


	    if (close_page_needed)
	    {
		/*
		** Force all dirty pages for this table.  Unless PURGE or TOSS
		** or FLUSH flag is specified, allow pages to remain in cache.
		**
		** If DMCM and we're not keeping the TCB, then close(TOSS)
		** the pages.
		*/
		if (t->tcb_temporary == TCB_TEMPORARY)
		    close_flag = 0;
		else if ( (flag & (DM2T_PURGE | DM2T_TOSS | DM2T_FLUSH))
		     || (!(flag & DM2T_KEEP)
			  && dcb->dcb_status & DCB_S_DMCM) )
		{
		    close_flag = DM0P_CLOSE;
		}
		else
		    close_flag = DM0P_CLCACHE;

		status = dm0p_close_page(t, lock_list, log_id, close_flag, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE*)NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9371_RELEASE_TCB_WARNING);
		    status = E_DB_WARN;
		    break;
		}
	    }
	}

	/*
	** If update_rel() noted that the TCB needs invalidating,
	** do it now after cached pages have been closed.
	*/
	if ( InvalidateTCB )
	{
	    /*
	    ** Note that we request a "bump" of zero.
	    **
	    ** If DMCM, this just cause a conversion of the LK_S SV_TABLE
	    ** lock to LK_X without changing the lock value. That will
	    ** fire dm2t_tcb_callback() in those servers holding blocking
	    ** locks and they'll then do a DM2T_FLUSH to cause their TCBs
	    ** to be rebuilt using the new iirelation pages/tuples info.
	    **
	    ** Because on this type of "invalidate" the TCB structure hasn't
	    ** changed, all cached pages are still ok.
	    **
	    ** If not DMCM, the bumplock will actually bump the lock value,
	    ** even though we specified zero.
	    */
	    status = dm2t_bumplock_tcb(dcb, t, (DB_TAB_ID*)NULL, (i4)0, dberr);
	    if ( status )
		break;
	}

	/* If KEEP, we're done */
	if ( flag & DM2T_KEEP )
	    break;

	/* Continue on to close and deallocate the TCB if FLUSH... */

	/*
	** If the PURGE flag is specified, then we are performing an operation
	** which may change the structure of the table or may bypass the buffer
	** manager and its cache locking.  If this table can be opened by a
	** server using a different buffer manager then call dm0p_tblcache to
	** invalidate any saved pages in the closed-table cache.
	*/
	if ((flag & DM2T_PURGE) && (dcb->dcb_bm_served == DCB_MULTIPLE)
	  && (t->tcb_status & TCB_SHOWONLY) == 0)
	{
	    status = dm0p_tblcache(dcb->dcb_id, t->tcb_rel.reltid.db_tab_base,
		DM0P_MODIFY, lock_list, log_id, dberr);
	    if (status != E_DB_OK)
	    {
		/*
		** If fail, TCB is still valid, just can't bump cache lock.
		** Set status to warn so TCB will be reused below.
		*/
		status = E_DB_WARN;
		break;
	    }
	}

	/*
	** At this point we begin to physically close the TCB.  If we
	** encounter an error now, we must take the TCB off of the tcb
	** queue, clean up whatever we can and mark the TCB invalid.
	*/

	/*
	** Close the FCB if there is one.
	*/
	if (t->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    release_fcb_flag = 0;
	    if (t->tcb_temporary == TCB_TEMPORARY)
	    {
		release_fcb_flag = DM2F_TEMP;
	    }
	    else if (t->tcb_table_io.tbio_sync_close)
	    {
		/*
		** Tables opened sync-at-close must be forced.
		*/
		status = dm2f_force_file(t->tcb_table_io.tbio_location_array,
		    t->tcb_table_io.tbio_loc_count, &t->tcb_rel.relid,
		    &t->tcb_dcb_ptr->dcb_name, dberr);
		if (status != E_DB_OK)
		    break;

#ifdef xFSYNC_TEST
		TRdisplay(
		    "dm2t_release_tcb: Table %~t forced via sync-at-close.\n",
		    sizeof(t->tcb_rel.relid), &t->tcb_rel.relid);
#endif
	    }

	    status = dm2f_release_fcb(lock_list, log_id,
		t->tcb_table_io.tbio_location_array,
		t->tcb_table_io.tbio_loc_count, release_fcb_flag, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Release any extended table entries.
	*/
	if (t->tcb_et_list)
	{
	    if (t->tcb_et_list->etl_next != t->tcb_et_list)
	    {
		DMP_ET_LIST	*etl;
		DMP_ET_LIST	*etl_next;

		for (etl = t->tcb_et_list->etl_next; etl != t->tcb_et_list;)
		{
		    etl_next = etl->etl_next;
		    dm0m_deallocate((DM_OBJECT **) &etl);
		    etl = etl_next;
		}
		t->tcb_et_list->etl_next = t->tcb_et_list;
		t->tcb_et_list->etl_prev = t->tcb_et_list;
	    }
	}

	/* If table has blobs, and we're purging, make sure that there is
	** no BQCB for the table either.  BQCB's contain an array of blob
	** attributes, which may now be out of date.
	*/
	if ((flag & DM2T_PURGE) && (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS))
	    dmpe_purge_bqcb(t->tcb_rel.reltid.db_tab_base);

	/*
	** Release any index TCB's.
	*/
	while (t->tcb_iq_next != (DMP_TCB*) &t->tcb_iq_next)
	{
	    it = t->tcb_iq_next;
	    if (it->tcb_table_io.tbio_status & TBIO_OPEN)
	    {
		release_fcb_flag = 0;
		if ( it->tcb_temporary == TCB_TEMPORARY )
		    release_fcb_flag = DM2F_TEMP;
		else if (it->tcb_table_io.tbio_sync_close)
		{
		    status = dm2f_force_file(
			it->tcb_table_io.tbio_location_array,
			it->tcb_table_io.tbio_loc_count, &it->tcb_rel.relid,
			&t->tcb_dcb_ptr->dcb_name, dberr);
		    if (status != E_DB_OK)
			break;

#ifdef xFSYNC_TEST
		    TRdisplay(
			"dm2t_release_tcb: Table %~t forced via sync-at-close.\n",
			sizeof(it->tcb_rel.relid), &it->tcb_rel.relid);
#endif
		}

		status = dm2f_release_fcb(lock_list, log_id,
		    it->tcb_table_io.tbio_location_array,
		    it->tcb_table_io.tbio_loc_count, release_fcb_flag, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /* Terminate any dmrAgents */
	    if ( it->tcb_rtb_next )
		dmrThrTerminate(it);

	    /*	Release any RCB's on the free queue. */

	    while (it->tcb_fq_next != (DMP_RCB*) &it->tcb_fq_next)
	    {
		DMP_RCB		*trcb = it->tcb_fq_next;
		status = dm2r_release_rcb(&trcb, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    if (status != E_DB_OK)
		break;

	    /*	Remove the TCB from the index queue. */

	    it->tcb_q_next->tcb_q_prev = it->tcb_q_prev;
	    it->tcb_q_prev->tcb_q_next = it->tcb_q_next;

	    /* Decrement index's cache open TCB count.
	    ** Note that an index can be showonly even if the base table
	    ** TCB isn't (although not the reverse).
	    */
	    if ((it->tcb_status & TCB_SHOWONLY) == 0)
	    {
		dm0s_mlock(&hcb->hcb_tq_mutex);
		hcb->hcb_cache_tcbcount[it->tcb_table_io.tbio_cache_ix]--;
		dm0s_munlock(&hcb->hcb_tq_mutex);
	    }

	    dm0s_mrelease(&it->tcb_et_mutex);
	    dm0s_mrelease(&it->tcb_mutex);
	    dm0m_deallocate((DM_OBJECT **)&it);
	}
	if (status != E_DB_OK)
	    break;

	/*	Release any RCB's on the free queue. */

	while (t->tcb_fq_next != (DMP_RCB*) &t->tcb_fq_next)
	{
	    DMP_RCB		*trcb = t->tcb_fq_next;
	    status = dm2r_release_rcb(&trcb, dberr);
	    if (status != E_DB_OK)
		break;
	}
	if (status != E_DB_OK)
	    break;


	/*
	** Release the TCB validation lock if this database is multiply served.
	** If a PURGE operation was requested, then bump the lock value first,
	** unless we've already bumped it on InvalidateTCB, above.
	*/
	if ( dcb->dcb_served == DCB_MULTIPLE && t->tcb_lkid_ptr->lk_unique )
	{
	    if ( flag & DM2T_PURGE && !InvalidateTCB )
		status = dm2t_bumplock_tcb(dcb, t, (DB_TAB_ID*)NULL, (i4)1, dberr);

	    if ( status == E_DB_OK )
		status = unlock_tcb(t, dberr);

	    if (status != E_DB_OK)
		break;
	}

	/*
	** Take the TCB off of the hash queue, wake up anybody waiting for
	** it and deallocate the TCB memory. Note that we must re-acquire the
	** hash mutex in order to manipulate the TCB hash queues.
	*/
	dm0s_mlock(&hcb->hcb_tq_mutex);
	hcb->hcb_tcbcount--;	/* keep track of TCBs allocated */
	hcb->hcb_cache_tcbcount[t->tcb_table_io.tbio_cache_ix]--;
	dm0s_munlock(&hcb->hcb_tq_mutex);

	/* 
	** NB: If TempTable Index, this removes the TCB from the
	** Base TCB's list of index TCBs, just what we want.
	*/
	dm0s_mlock(t->tcb_hash_mutex_ptr);
	t->tcb_q_next->tcb_q_prev = t->tcb_q_prev;
	t->tcb_q_prev->tcb_q_next = t->tcb_q_next;
	t->tcb_hash_bucket_ptr = 0;

	/* If TempTable SI, update parent TCB with one less index */
	if ( t->tcb_rel.relstat & TCB_INDEX )
	{
	    if ( --t->tcb_parent_tcb_ptr->tcb_index_count == 0 )
	    {
		t->tcb_parent_tcb_ptr->tcb_rel.relstat &= ~TCB_IDXD;
		t->tcb_parent_tcb_ptr->tcb_update_idx = FALSE;
	    }
	    if ( t->tcb_rel.relstat & TCB_UNIQUE )
	    {
		if ( --t->tcb_parent_tcb_ptr->tcb_unique_count == 0 )
		    t->tcb_parent_tcb_ptr->tcb_unique_index = 0;
	    }
	}
	/* Kill the TCB pointer in the pp-array, but not if it's what
	** the caller fed us!  otherwise the deallocate call below sees
	** a null pointer...
	*/
	if (t->tcb_rel.relstat2 & TCB2_PARTITION
	  && tcb != &t->tcb_pmast_tcb->tcb_pp_array[t->tcb_partno].pp_tcb)
	    t->tcb_pmast_tcb->tcb_pp_array[t->tcb_partno].pp_tcb = NULL;

	dm2t_awaken_tcb(t, lock_list);

	/* Terminate any dmrAgents */
	if ( t->tcb_rtb_next )
	    dmrThrTerminate(t);

	dm0s_mrelease(&t->tcb_et_mutex);
	dm0s_mrelease(&t->tcb_mutex);
	/* deallocate and obliterate caller's TCB pointer */
	dm0m_deallocate((DM_OBJECT **)tcb);
	/* Return with hash mutex still locked */
	return (E_DB_OK);
    }

    /* Relock TCB's hash queue, if unlocked */
    if ( sem_released )
	dm0s_mlock(t->tcb_hash_mutex_ptr);

    if (DB_FAILURE_MACRO(status) && (dberr->err_code > E_DM_INTERNAL))
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9270_RELEASE_TCB);
    }

    /* Wake up any sessions waiting on this TCB */
    if ( t->tcb_status & TCB_WAIT )
	dm2t_awaken_tcb(t, lock_list);
    
    /* Sanity check: turn off TCB_BUSY only if we turned it on */
    if ( unwait_tcb )
	t->tcb_status &= ~(TCB_BUSY);

    if (DB_SUCCESS_MACRO(status))
    {
	/* If TCB is still fixed or busy, return warning */
	if ( t->tcb_ref_count || t->tcb_status & TCB_BUSY )
	{
	    SETDBERR(dberr, 0, E_DM937E_TCB_BUSY);
	    status = E_DB_WARN;
	}
    }
    else
    {
	/*
	** If an error occurred releasing the TCB, then this tcb is no longer
	** valid.  Take it off the TCB hash queue and mark it invalid.
	*/
	if ( !(t->tcb_rel.relstat & TCB_INDEX) )
	{
	    t->tcb_q_next->tcb_q_prev = t->tcb_q_prev;
	    t->tcb_q_prev->tcb_q_next = t->tcb_q_next;
	    t->tcb_hash_bucket_ptr = 0;
	}
	t->tcb_status |= TCB_INVALID;
    }

    return (status);
}

/*{
** Name: dm2t_reclaim_tcb - Reclaim space of TCBs available for release
**
**
** Description:
**      This routine scans the TCB free queue off the hash control
**      block looking for TCBs that can be reclaimed.
**
** Inputs:
**      tcb                             The base TCB to update.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-oct-86 (jennifer)
**          Created new for Jupiter.
**	16-oct-89 (rogerk)
**	    Changed to give up reclaim attempts after 10 failures rather than
**	    two.  This adds a little more fault tolerance in the case when
**	    multiple threads are attempting to toss tcb's at the same time.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	23-aug-1993 (rogerk)
**	    Changed decision on whether to update reltups values to be based
**	    on DCB_S_RECOVER mode rather than DCB_S_ONLINE_RCP.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    hcb_tq_mutex for free TCB queue.
**	14-aug-2003 (jenjo02)
**	    Removed long-deprecated DM2T_UPDATE check.
**	15-Jan-2003 (schka24)
**	    Updated hash mutexing scheme.
**	02-Dec-2004 (jenjo02)
**	    Drop ule_format if E_DB_WARN returned from dm2t_release_tcb().
**	    It's not useful information and just raises alarms where
**	    there ought to be none.
**      30-Nov-2007 (kibro01 on behalf of kschendel) b120327
**          Be a bit more correct about mutex handling to avoid
**          reclaim/reclaim races.
**	13-May-2008 (jonj)
**	    Explicity tell dm2t_release_tcb() that this is a RECLAIM.
*/
DB_STATUS
dm2t_reclaim_tcb(
i4		    lock_list,
i4		    log_id,
DB_ERROR	    *dberr)
{
    DMP_HCB     *hcb = dmf_svcb->svcb_hcb_ptr;
    DM_MUTEX	*hash_mutex;
    QUEUE	*fq;
    DMP_TCB     *base_tcb;
    DMP_TCB     *t;
    DB_STATUS   status = E_DB_OK;
    i4		retry_count = 20;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (DMZ_TBL_MACRO(10))
	TRdisplay("DM2T_RECLAIM\n");
#endif
    for(;;)
    {
	dm0s_mlock(&hcb->hcb_tq_mutex);
	debug_tcb_free_list("reclaim",NULL);

        /* Get first list entry, if list is empty there's nothing to reclaim */
        fq = hcb->hcb_ftcb_list.q_next;
        if (fq == &hcb->hcb_ftcb_list)
	{
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM9328_DM2T_NOFREE_TCB);
	    break;
	}

	/*
	** Get the base offset for the possibly free TCB.
	** Note that the free queue points through the middle of the TCB's
	** on the queue, so we have to do some pointer arithmetic to get
	** to the actual tcb ptr.
	*/
	base_tcb = (DMP_TCB *)( (char *)fq -
					CL_OFFSETOF(DMP_TCB, tcb_ftcb_list));
        hash_mutex = base_tcb->tcb_hash_mutex_ptr;
 
        /* In order to look at the TCB safely (as well as attempt to
        ** release it), we need the TCB's hash mutex, which has to be
        ** taken before the hash free q mutex.  After taking the hash
        ** mutex, make sure that the TCB is still on the free queue
        ** somewhere, and that it's the same TCB with the same hash
        ** mutex coverage;  if it isn't, a racing reclaim might have
        ** disposed of the TCB (and possibly even reused it) while we
        ** fiddled with the mutexes.
        */
        dm0s_munlock(&hcb->hcb_tq_mutex);
        dm0s_mlock(hash_mutex);
        dm0s_mlock(&hcb->hcb_tq_mutex);
        fq = hcb->hcb_ftcb_list.q_next;
        while (fq != &hcb->hcb_ftcb_list)
        {
            t = (DMP_TCB *)( (char *)fq - CL_OFFSETOF(DMP_TCB, tcb_ftcb_list));
            if (t == base_tcb)
                break;
            fq = fq->q_next;
        }
        if (t != base_tcb || hash_mutex != t->tcb_hash_mutex_ptr)
        {
            /* Someone else pulled the rug out.  Just try again from
            ** the start.
            */
            dm0s_munlock(&hcb->hcb_tq_mutex);
            dm0s_munlock(hash_mutex);
            continue;
        }

	/*
	** Remove from the TCB free queue.
	** This is done now, rather than allowing release-tcb to do it,
        ** because we want to make sure no other reclaimer sees this TCB
        ** when we let go of the free-list mutex.
	*/
	base_tcb->tcb_ftcb_list.q_prev->q_next = base_tcb->tcb_ftcb_list.q_next;
	base_tcb->tcb_ftcb_list.q_next->q_prev = base_tcb->tcb_ftcb_list.q_prev;
	/* release-tcb is going to try to do this again, make sure it's OK */
	base_tcb->tcb_ftcb_list.q_next = base_tcb->tcb_ftcb_list.q_prev =
			&base_tcb->tcb_ftcb_list;
	debug_tcb_free_list("off",base_tcb);

	dm0s_munlock(&hcb->hcb_tq_mutex);

	/*
	** Release the TCB.
	**
	** If release_tcb returns with a warning, then the tcb could not
	** be freed.  We may need to put it back on the free queue and try
	** the next tcb (we put the tcb at the end of the free queue so
	** our next attempt will be with a different tcb).
	**
	** When the release() sees a TCB_BUSY or the reference count is not
	** zero E_DB_WARN is returned.  We must not put the tcb back on the 
	** free list in this situation.
	**
	** Inform dm2t_release_tcb() that this is a RECLAIM so it won't
	** stall us waiting for a busy TCB after update_rel().
	**
	** After too many retries, just give up and return an error.
	*/
	status = dm2t_release_tcb(&base_tcb, DM2T_RECLAIM, lock_list, 
				  log_id, dberr);
        if (status == E_DB_OK)
        {
            /* Normal, success return */
            dm0s_munlock(hash_mutex);
            hcb->hcb_tcbreclaim++;      /* Count successful reclaims */
            return (E_DB_OK);
        }
        else if (status != E_DB_WARN)
        {
            dm0s_munlock(hash_mutex);
            break;
        }
        /* Don't put on free list if now fixed or busy */
        if ( base_tcb->tcb_ref_count == 0 &&
            (base_tcb->tcb_status & TCB_BUSY) == 0 )
        {
            /* Put on the back of the free list */
            dm0s_mlock(&hcb->hcb_tq_mutex);
            debug_tcb_free_list("on",base_tcb);
            base_tcb->tcb_ftcb_list.q_next = hcb->hcb_ftcb_list.q_prev->q_next;
            base_tcb->tcb_ftcb_list.q_prev = hcb->hcb_ftcb_list.q_prev;
            hcb->hcb_ftcb_list.q_prev->q_next = &base_tcb->tcb_ftcb_list;
            hcb->hcb_ftcb_list.q_prev = &base_tcb->tcb_ftcb_list;
            dm0s_munlock(&hcb->hcb_tq_mutex);
        }
        dm0s_munlock(hash_mutex);
        if (--retry_count == 0)
	{
	    SETDBERR(dberr, 0, E_DM9328_DM2T_NOFREE_TCB);
	    break;
	}
	/* try reclaiming another one... */
    }

    if ((dberr->err_code > E_DM_INTERNAL) && (dberr->err_code != E_DM9328_DM2T_NOFREE_TCB))
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM926E_RECLAIM_TCB);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm2t_control	- Set/Reset table control lock.
**
** Description:
**	This routine sets or resets the table control lock.  The table
**	control lock allow or disallows readlock=nolock readers access
**	to the table.
**
**	Sometimes this lock is acquired exclusively and held until end of
**	transaction, and sometimes it is acquired shared and released soon
**	thereafter. For example, when you modify a table you hold the table
**	control lock exclusively until you commit the modify.
**
**	In addition, sometimes a transaction may first acquire the table control
**	lock exclusively, then later may go and request it in a shared mode on
**	the same lock list. The shared re-request is quietly granted at
**	LK_X mode, but note that when we go to release the LK_S request,
**	we don't then want to release the lock entirely! To handle this
**	case, we make the table control lock a PHYSICAL lock. Physical locks
**	have the property that each lock request is counted, and there must be
**	a matching lock release for each lock request before the lock is
**	released. Thus the LKrelease call on the lock only truly releases the
**	lock if it was only held once (i.e., only by the LK_S requestor, since
**	the code which requests the lock at LK_X never releases it).
**
**	(The whole point of this little dance is to allow DMT_SHOW to naively
**	request the table control lock on the transaction lock list (if one
**	exists) without having to be concerned about whether or not to release
**	the table control lock later. DMT_SHOW always "releases" the table
**	control lock; we then let the locking system make the decision about
**	whether to perform a REAL release or whether simply to decrement the
**	count of lock requests for this lock.)
**
**	Table control locks are always taken on partitioned masters, never on
**	partitions.  You'll note that only the base (master) table ID is passed.
**
** Inputs:
**	base_id				Base table id.
**      lock_list			Lock list to lock table.
**	mode				Lock mode.
**	flags				Lock request flags: LK_NOWAIT, LK_STATUS
**	timeout				Lock request timeout value.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			Input flags specified LK_STATUS and the
**					    lock request returned LK_NEW_LOCK.
**					    Table control lock granted.
**	    E_DB_WARN			Input flags specified LK_NOWAIT and the
**					    lock could not be granted.
**	    E_DB_ERROR			Error requesting lock.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-feb-1988 (derek)
**          Created.
**	17-jul-90 (mikem)
**	    bug #30833 - lost force aborts
**	    LKrequest has been changed to return a new error status
**	    (LK_INTR_GRANT) when a lock has been granted at the same time as
**	    an interrupt arrives (the interrupt that caused the problem was
**	    a FORCE ABORT).   Callers of LKrequest which can be interrupted
**	    must deal with this new error return and take the appropriate
**	    action.  Appropriate action includes signaling the interrupt
**	    up to SCF, and possibly releasing the lock that was granted.
**	    dm2t_control() was changed to release the lock in this situation
**	    as the control lock was not released by the calling code in this
**	    situation.
**	28-feb-1992 (bryanp)
**	    Just added some comments about the table control lock.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              If database is exclusively locked then don't bother
**              obtaining the lock for table level rollforwarddb
**	13-dec-1996 (cohmi01)
**	    Add more info to lock timeout/deadlock msgs (Bug 79763).
**	01-sep-1997 (ocoro02)
**	    Several ule_format calls in dm2t_control & dm2t_lock_table were
**	    passing the value of tablename & ownername instead of the
**	    address. This caused garbled error text (#84248).
**	20-Aug-2002 (jenjo02)
**	    Support TIMEOUT=NOWAIT, new LK_UBUSY status from LKrequest().
**	    Write E_DM9066_LOCK_BUSY info message if DMZ_SES_MACRO(35).
**	    Return E_DM006B_NOWAIT_LOCK_BUSY.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
**	16-Mar-2010 (jonj)
**	    Bug 123377, deadlocks between CONTROL/TABLE lock.
**	    If LK_S and lock list already holds a TABLE lock,
**	    then get a LK_N control lock instead of LK_S.
*/
DB_STATUS
dm2t_control(
DMP_DCB		    *dcb,
i4		    base_id,
i4		    lock_list,
i4		    lock_mode,
i4		    flags,
i4		    timeout,
DB_ERROR            *dberr)
{
    LK_LOCK_KEY		ctrl_lock_key;
    CL_ERR_DESC		sys_err;
    STATUS		status;
    i4			error;
    char		errbuf[32];
    DB_TAB_ID		tabid;
    DB_TAB_NAME		tablename;
    DB_OWN_NAME		ownername;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    /*
    ** If the database is locked exclusively, then don't really lock the
    ** table.
    */
    if (dcb->dcb_status & DCB_S_EXCLUSIVE)
    {
        return (E_DB_OK);
    }

    ctrl_lock_key.lk_type = LK_TBL_CONTROL;
    ctrl_lock_key.lk_key1 = dcb->dcb_id;
    ctrl_lock_key.lk_key2 = base_id;
    ctrl_lock_key.lk_key3 = 0;
    ctrl_lock_key.lk_key4 = 0;
    ctrl_lock_key.lk_key5 = 0;
    ctrl_lock_key.lk_key6 = 0;

    if (lock_mode == LK_N)
    {
	status = LKrelease((i4)0, lock_list,
	    (LK_LKID *)NULL, (LK_LOCK_KEY *)&ctrl_lock_key, (LK_VALUE *)NULL,
	    &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 1, 0, lock_list);
	    SETDBERR(dberr, 0, E_DM9288_TBL_CTRL_LOCK);
	    return (E_DB_ERROR);
	}

	return (E_DB_OK);
    }

    /* Just reading? */
    if ( lock_mode == LK_S )
    {
	/*
	** If this lock list holds any kind of TABLE lock on the table, then
	** get a LK_N control lock, not LK_S.
	**
	** This satisfies the caller's need to have a control lock to release,
	** but gets the lock in a compatible mode.
	**
	** Even though TABLE locks are physical, they are not released until
	** end of transaction (see comment in dm2t_close()) to keep the
	** table from being altered or dropped while the transaction is alive
	** and subject to abort recovery.
	**
	** Keeping the TABLE locks sets the stage for deadlocks with CONTROL
	** locks because there is no predictable hierarchy of lock order. It
	** may seem like Ingres is careful about taking a CONTROL lock before
	** a TABLE lock, but because the TABLE locks are kept, this carefully
	** crafted order goes out the window.
	*/
	ctrl_lock_key.lk_type = LK_TABLE;

	status = LKshow(LK_S_OWNER, lock_list, (LK_LKID*)NULL, &ctrl_lock_key,
			0, NULL, (u_i4*)NULL, (u_i4*)NULL, &sys_err);

	/* status == OK means we hold the TABLE lock */
	if ( status == OK )
	    lock_mode = LK_N;

	ctrl_lock_key.lk_type = LK_TBL_CONTROL;
    }

    /*
    ** Request a table control lock for protecting the integrity
    ** of the table for read/nolock reader against operations(modify,
    ** index, etc) that need exclusively access of the table.
    */

    if ( dcb->dcb_bm_served == DCB_SINGLE )
	flags |= LK_LOCAL;

    status = LKrequest( flags | LK_PHYSICAL, 
			lock_list, 
			&ctrl_lock_key, 
			lock_mode, 
			(LK_VALUE *)NULL, 
			(LK_LKID *)NULL, 
			timeout, 
			&sys_err);

    if ( status == OK )
	return(E_DB_OK);

    /* If caller requested lock grant status, return INFO if new lock granted */
    else if ( status == LK_NEW_LOCK )
	return(E_DB_INFO);

    /* If NOWAIT specified and lock could not be granted, return WARNING */
    else if ( status == LK_BUSY )
    {
	SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
	return(E_DB_WARN);
    }

    /*
    ** Error conditions. LK_UBUSY is ok (TIMEOUT=NOWAIT)
    */
    if ( status != LK_UBUSY )
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
    /*
    ** Try to lookup table name/owner
    */
    MEcopy("TABLE_ID", 8, errbuf);
    CVla(base_id, &errbuf[8]);
    tabid.db_tab_base=base_id;
    tabid.db_tab_index=0;
    if (dm2t_lookup_tabname(dcb, &tabid,
		&tablename, &ownername, &local_dberr) != E_DB_OK)
    {
	 MEcopy(errbuf,STlength(errbuf)+1,(PTR)&tablename);
	 MEfill(sizeof(ownername),0,(PTR)&ownername);
    }

    if (status == LK_DEADLOCK)
    {
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	     _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2736_TABLE_DEADLOCK,
		    FALSE, /* Not force */
		    dberr, NULL);

	uleFormat(NULL, E_DM9045_TABLE_DEADLOCK, &sys_err,
	    ULE_LOG, NULL, NULL, 0, NULL, &error, 5, sizeof(ownername), &ownername,
	    sizeof(tablename), &tablename, sizeof(DB_DB_NAME), &dcb->dcb_name, 
	    0, lock_mode, 0, sys_err.moreinfo[0].data.string);
	SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
    }
    else if (status == LK_NOLOCKS)
    {
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	     _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2739_LOCK_LIMIT,
		    FALSE, /* Not force */
		    dberr, NULL);
	uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err,
	    ULE_LOG, NULL, NULL, 0, NULL, &error, 2, sizeof(tablename), &tablename,
	    sizeof(DB_DB_NAME), &dcb->dcb_name);
	SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
    } 
    else if (status == LK_TIMEOUT) 
    {
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE ) _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2738_LOCK_TIMEOUT,
		    FALSE, /* Not force */
		    dberr, NULL);
	uleFormat(NULL, E_DM9043_LOCK_TIMEOUT, &sys_err,
	    ULE_LOG, NULL, NULL, 0, NULL, &error, 6, sizeof(tablename), &tablename,
	    sizeof(DB_DB_NAME), &dcb->dcb_name, 0, lock_mode,
	    sizeof(ownername), &ownername, 0, "CONTROL", 
	    0, sys_err.moreinfo[0].data.string);
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
    }
    else if (status == LK_INTERRUPT)
    {
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    }
    else if (status == LK_INTR_GRANT)
    {
	/* We have recieved an interrupt (most probably a FORCE ABORT) at
	** exactly the same time as being granted a lock.  We will return
	** the user interrupt (otherwise the FORCE ABORT will be lost).  We
	** must also release the table control lock, as it sometimes will
	** is not released if the interrupt is returned.
	*/
	status = LKrelease((i4)0, lock_list,
	    (LK_LKID *)NULL, (LK_LOCK_KEY *)&ctrl_lock_key, (LK_VALUE *)NULL,
	    &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 1, 0, lock_list);
	}

	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    }
    else if ( status == LK_INTR_FA )
    {
        SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
    }
    else if ( status == LK_UBUSY )
    {

	/* Output error message iff DMZ_SES_MACRO(35) in on in this SCB */
	if ( DMZ_SES_MACRO(35) )
	{
	    CS_SID	sid;
	    CSget_sid(&sid);

	    if ( DMZ_MACRO((GET_DML_SCB(sid))->scb_trace, 35) )
	    {
		uleFormat(NULL, E_DM9066_LOCK_BUSY, &sys_err,
		    ULE_LOG, NULL, NULL, 0, NULL, &error, 6, sizeof(tablename), &tablename,
		    sizeof(DB_DB_NAME), &dcb->dcb_name, 0, lock_mode,
		    sizeof(ownername), &ownername, 0, "CONTROL", 
		    0, sys_err.moreinfo[0].data.string);
	    }
	}
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
    }
    else
    {
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
	    0, 0, 0, &error, 2, 0, lock_mode, 0, lock_list);
	SETDBERR(dberr, 0, E_DM9288_TBL_CTRL_LOCK);
    }
    return(E_DB_ERROR);
}

/*{
** Name: dm2t_mvcc_lock - Get physical MVCC lock on table.
**
** Description:
**	Gets a physical LK_TBL_MVCC lock on a table in the
**	mode requested.
**
** Inputs:
**	dcb				Pointer to Database control block
**	table_id			Pointer to table id
**	lock_list			Lock list id
**	lock_mode			Mode at which to MVCC-lock table
**	timeout				Lock request timeout value
**
** Outputs:
**	lockid				lockid of MVCC lock
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Error requesting lock.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created.
*/
static DB_STATUS
dm2t_mvcc_lock(
DMP_DCB		*dcb,
DB_TAB_ID	*table_id,
i4		lock_list,
i4		lock_mode,
i4		timeout,
LK_LKID		*lockid,
DB_ERROR	*dberr)
{
    LK_LOCK_KEY	    lock_key;
    STATUS	    stat;
    CL_ERR_DESC	    sys_err;
    DB_TAB_NAME	    tablename;
    DB_OWN_NAME	    ownername;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* If LK_N, release previously acquired LK_TBL_MVCC lock */
    if ( lock_mode == LK_N )
    {
	stat = LKrelease((i4)0, lock_list, lockid, 
			(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);
	if ( stat != OK )
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_list);
	    SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);
	    return (E_DB_ERROR);
	}
	return (E_DB_OK);
    }

    /*
    ** Format the lock key using the Database Id and the Base TableId.
    ** Note that base table and secondary index locks collide!!!
    ** Similarly for partitions we actually lock the master.
    */
    lock_key.lk_type = LK_TBL_MVCC;
    lock_key.lk_key1 = dcb->dcb_id;
    lock_key.lk_key2 = table_id->db_tab_base;
    lock_key.lk_key3 = 0;
    lock_key.lk_key4 = 0;
    lock_key.lk_key5 = 0;
    lock_key.lk_key6 = 0;

    lockid->lk_unique = 0;
    lockid->lk_common = 0;

    /*
    ** Request MVCC lock.
    */
    stat = LKrequest(dcb->dcb_bm_served == DCB_SINGLE ?
                     LK_PHYSICAL | LK_LOCAL : LK_PHYSICAL,
                     lock_list, &lock_key, lock_mode,
                     (LK_VALUE *)NULL, lockid, timeout,
                     &sys_err);
    if ( stat != OK )
    {
	char	    errbuf[48];

	STprintf(errbuf, "TABLE_ID (%d,%d)", table_id->db_tab_base,
			table_id->db_tab_index);

	/*
	** Error conditions. LK_UBUSY is ok (TIMEOUT=NOWAIT)
	*/
	if ( stat != LK_UBUSY )
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	/*
	** Try to lookup table name/owner
	*/
	if (dm2t_lookup_tabname(dcb, table_id,
		&tablename, &ownername, &local_dberr) != E_DB_OK)
	{
		MEcopy(errbuf,STlength(errbuf)+1,(PTR)&tablename);
		MEfill(sizeof(ownername),0,(PTR)&ownername);
	}

	if (stat == LK_DEADLOCK)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	       _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2736_TABLE_DEADLOCK,
		    FALSE, /* Not force */
		    dberr, NULL);
	    uleFormat(NULL, E_DM9045_TABLE_DEADLOCK, &sys_err,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 5, 
		sizeof(ownername), &ownername, sizeof(tablename), &tablename,
		sizeof(DB_DB_NAME), &dcb->dcb_name, 0, lock_mode,
		0, sys_err.moreinfo[0].data.string);
	    SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
	}
	else if (stat == LK_NOLOCKS)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		 _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2739_LOCK_LIMIT,
		    FALSE, /* Not force */
		    dberr, NULL);
	    uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 2, 
		sizeof(tablename), &tablename,
		sizeof(DB_DB_NAME), &dcb->dcb_name);
	    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	}
	else if (stat == LK_TIMEOUT)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		 _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2738_LOCK_TIMEOUT,
		    FALSE, /* Not force */
		    dberr, NULL);
	    uleFormat(NULL, E_DM9043_LOCK_TIMEOUT, &sys_err, ULE_LOG, NULL, 
		0, 0, 0, err_code, 6, sizeof(tablename), &tablename,
		sizeof(DB_DB_NAME), &dcb->dcb_name,
		0, lock_mode, sizeof(ownername), &ownername, 
		0, "TBL_MVCC", 0, sys_err.moreinfo[0].data.string);
	    SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	}
	else if ((stat == LK_INTERRUPT) || (stat == LK_INTR_GRANT))
	{
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	}
        else if (stat == LK_INTR_FA)
        {
	    SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
        }
	else if ( stat == LK_UBUSY )
	{
	    /* Output error message iff DMZ_SES_MACRO(35) in on in this SCB */
	    if ( DMZ_SES_MACRO(35) )
	    {
		CS_SID	sid;
		CSget_sid(&sid);

		if ( DMZ_MACRO((GET_DML_SCB(sid))->scb_trace, 35) )
		{
		    uleFormat(NULL, E_DM9066_LOCK_BUSY, &sys_err, ULE_LOG, NULL, 
			0, 0, 0, err_code, 6, sizeof(tablename), &tablename,
			sizeof(DB_DB_NAME), &dcb->dcb_name,
			0, lock_mode, sizeof(ownername), &ownername, 
			0, "TBL_MVCC", 0, sys_err.moreinfo[0].data.string);
		}
	    }
	    SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	}
	else
	{
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		0, 0, 0, err_code, 2, 0, lock_mode, 0, lock_list);
	    SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);
	}

	return (E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: dm2t_lock_table - Set Table Level Transaction Lock.
**
** Description:
**	This routine obtains a transaction lock on the requested table.
**	The lock is requested at the indicated mode.  Because the table
**	lock may already be held by the transaction, a lock mode higher
**	than the requested one may be returned.
**
**	If the caller has the database locked exclusively, the routine
**	returns without requesting the table lock, but sets the grant mode
**	imitate a table-level lock being granted.
**
** Inputs:
**	dcb				Pointer to Database control block
**	table_id			Pointer to table id
**	request_mode			Mode at which to lock table
**	lock_list			Lock list id
**	timeout				Lock request timeout value
**
** Outputs:
**	grant_mode			Mode at which the lock is acquired
**	lockid				Id of the granted lock.
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Error requesting lock.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	13-dec-1995 (cohmi01)
**	    Add more info to lock timeout/deadlock msgs (Bug 79763).
**	01-sep-1997 (ocoro02)
**	    Several ule_format calls in dm2t_control & dm2t_lock_table were
**	    passing the value of tablename & ownername instead of the
**	    address. This caused garbled error text (#84248).
**	20-Aug-2002 (jenjo02)
**	    Support TIMEOUT=NOWAIT, new LK_UBUSY status from LKrequest().
**	    Write E_DM9066_LOCK_BUSY info message if DMZ_SES_MACRO(35).
**	    Return E_DM006B_NOWAIT_LOCK_BUSY.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, return E_DM004D_LOCK_TIMER_EXPIRED instead.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
*/
DB_STATUS
dm2t_lock_table(
DMP_DCB		*dcb,
DB_TAB_ID	*table_id,
i4		request_mode,
i4		lock_list,
i4		timeout,
i4		*grant_mode,
LK_LKID		*lockid,
DB_ERROR	*dberr)
{
    LK_LOCK_KEY	    lock_key;
    LK_VALUE	    lockvalue;
    STATUS	    stat;
    CL_ERR_DESC	    sys_err;
    DB_TAB_NAME	    tablename;
    DB_OWN_NAME	    ownername;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If the database is locked exclusively, then don't really lock the
    ** table, but pretend like we locked it in TABLE mode so that page
    ** lock requests are bypassed.
    */
    if (dcb->dcb_status & DCB_S_EXCLUSIVE)
    {
	*grant_mode = DM2T_X;
	if (dcb->dcb_access_mode == DCB_A_READ)
	    *grant_mode = DM2T_S;
	return (E_DB_OK);
    }

    /*
    ** Format the lock key using the Database Id and the Base TableId.
    ** Note that base table and secondary index locks collide!!!
    ** Similarly for partitions we actually lock the master.
    */
    lock_key.lk_type = LK_TABLE;
    lock_key.lk_key1 = dcb->dcb_id;
    lock_key.lk_key2 = table_id->db_tab_base;
    lock_key.lk_key3 = 0;
    lock_key.lk_key4 = 0;
    lock_key.lk_key5 = 0;
    lock_key.lk_key6 = 0;


    /*
    ** Request table level lock.
    **
    ** It is OK if LK_VAL_NOTVALID is returned from this lock request.
    ** It indicates that the lock request was granted, but that the stored
    ** lock-value of the lock resource has been lost.  Since we are only
    ** interested in the 'mode' portion of the lockvalue return variable
    ** (which is guaranteed to be correct) we can igore the return code.
    */
    stat = LKrequest(dcb->dcb_bm_served == DCB_SINGLE ?
                     LK_PHYSICAL | LK_LOCAL : LK_PHYSICAL,
                     lock_list, &lock_key, request_mode,
                     (LK_VALUE *)&lockvalue, (LK_LKID *)lockid, timeout,
                     &sys_err);
    if (stat != OK && stat != LK_VAL_NOTVALID)
    {
	char	    errbuf[48];

	STprintf(errbuf, "TABLE_ID (%d,%d)", table_id->db_tab_base,
			table_id->db_tab_index);

	/* LK_UBUSY is ok, not really an error */
	if ( stat != LK_UBUSY )
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	/*
	** Try to lookup table name/owner
	*/
	if (dm2t_lookup_tabname(dcb, table_id,
		&tablename, &ownername, &local_dberr) != E_DB_OK)
	{
		MEcopy(errbuf,STlength(errbuf)+1,(PTR)&tablename);
		MEfill(sizeof(ownername),0,(PTR)&ownername);
	}

	if (stat == LK_DEADLOCK)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	       _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2736_TABLE_DEADLOCK,
		    FALSE, /* Not force */
		    dberr, NULL);
	    uleFormat(NULL, E_DM9045_TABLE_DEADLOCK, &sys_err,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 5, 
		sizeof(ownername), &ownername, sizeof(tablename), &tablename,
		sizeof(DB_DB_NAME), &dcb->dcb_name, 0, request_mode,
		0, sys_err.moreinfo[0].data.string);
	    SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
	}
	else if (stat == LK_NOLOCKS)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		 _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2739_LOCK_LIMIT,
		    FALSE, /* Not force */
		    dberr, NULL);
	    uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 2, 
		sizeof(tablename), &tablename,
		sizeof(DB_DB_NAME), &dcb->dcb_name);
	    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	}
	else if (stat == LK_TIMEOUT)
	{
	    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		 _VOID_ dma_write_audit(
		    SXF_E_RESOURCE,
		    SXF_A_FAIL | SXF_A_LIMIT,
		    (char*)&tablename,		/* Table name */
		    sizeof(tablename),
		    &ownername,
		    I_SX2738_LOCK_TIMEOUT,
		    FALSE, /* Not force */
		    dberr, NULL);
	    uleFormat(NULL, E_DM9043_LOCK_TIMEOUT, &sys_err, ULE_LOG, NULL, 
		0, 0, 0, err_code, 6, sizeof(tablename), &tablename,
		sizeof(DB_DB_NAME), &dcb->dcb_name,
		0, request_mode, sizeof(ownername), &ownername, 
		0, "TABLE", 0, sys_err.moreinfo[0].data.string);
	    SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	}
	else if ((stat == LK_INTERRUPT) || (stat == LK_INTR_GRANT))
	{
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	}
        else if (stat == LK_INTR_FA)
        {
	    SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
        }
	else if ( stat == LK_UBUSY )
	{
	    /* Output error message iff DMZ_SES_MACRO(35) in on in this SCB */
	    if ( DMZ_SES_MACRO(35) )
	    {
		CS_SID	sid;
		CSget_sid(&sid);

		if ( DMZ_MACRO((GET_DML_SCB(sid))->scb_trace, 35) )
		{
		    uleFormat(NULL, E_DM9066_LOCK_BUSY, &sys_err, ULE_LOG, NULL, 
			0, 0, 0, err_code, 6, sizeof(tablename), &tablename,
			sizeof(DB_DB_NAME), &dcb->dcb_name,
			0, request_mode, sizeof(ownername), &ownername, 
			0, "TABLE", 0, sys_err.moreinfo[0].data.string);
		}
	    }
	    SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	}
	else
	{
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		0, 0, 0, err_code, 2, 0, request_mode, 0, lock_list);
	    SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);
	}

	return (E_DB_ERROR);
    }

    /*
    ** Table lock has been granted.
    **
    ** Return granted lock mode - which may be actually be different (higher,
    ** but never lower) than the requested lock mode.  The caller may use
    ** this information increase query performance (for instance, if an IS
    ** lock is requested and an S lock is returned, the caller can set its
    ** query lock mode to Table Level and avoid page locking).
    */
    *grant_mode = lockvalue.lk_mode;

    return (E_DB_OK);
}

/*{
** Name: dm2t_temp_build_tcb	- Build a TCB for a temporary table.
**
** Description:
**      This routine is builds a TCB for a temporary table decribed by a
**	a relation record and a set of attribute characteristics.
**      The TCB is linked into the TCB hash array.  The table-id is
**      obtained through a value lock which is set to an intial value of
**      ffff0000.  Each time a temporary table is created it gets this
**      lock exclusively, increments it, uses the value for a table-id,
**      and converts the lock to null.  This is necessary since the file
**      name is derived from the table-id and must be unique across
**      two servers accessing same database.
**
** Inputs:
**      dcb                             The database to search.
**      table_id                        The table to search for.
**	tran_id				Pointer to transaction id.
**	lock_id				Lock list identifier.
**	log_id				Log file identifier.
**      relrecord                       Pointer to the relation record.
**      attr_array                      Pointer to an array describing
**                                      attributes.
**      attr_count                      Count of number of attributes.
**      key_array                       Pointer to an array describing
**                                      keys for LOAD files.
**      key_count                       Count of number of keys.
**      flag                            Indicates if this is a LOAD
**                                      (SORT) file.
**					DM2T_LOAD   - this is a load file
**					DM2T_TMP_MODIFIED - this temporary
**					    table already exists, and has just
**					    been modified.
**					DM2T_TMP_INMEMO_MOD - this temporary
**					    table already exists, and was just
**					    modified with an in-memory build.
**	recovery			Should this table perform recovery?
**					    0 -- no, it should not
**					    non-zero -- yes, it should
**	num_alloc			If flag was DM2t_TMP_INMEM_MOD, this
**					    is the allocated size of the table.
**	location			Pointer to an array of DB_LOC_NAMEs
**	loc_count			Number of locations in array.
**
** Outputs:
**      tcb                             The built TCB.
**      err_code                        Reason for error return status.
**					Which includes one of the following
**                                      error codes:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-1986 (jennifer)
**          Created new for jupiter.
**	12-sep-1988 (rogerk)
**	    Don't blank out the table name and owner as they are useful for
**	    error messages.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Temporary tables never need "sync_flag" enabled.
**	21-feb-89 (rogerk)
**	    Set new tcb_status field.
**	15-aug-89 (rogerk)
**	    Add support for Gateway. Define tcb_logging, tcb_logical_log and
**	    tcb_nofile fields.  Fill in tcb_table_type.
**	17-dec-1990 (jas)
**	    Smart Disk project integration:  Find out the Smart Disk type,
**	    if any, for the table and store the type in the tcb.
**	 4-feb-91 (rogerk)
**	    Initialize tcb_bmpriority field.
**	 7-mar-1991 (bryanp)
**	    Changed the DMP_TCB to support Btree index compression, and added
**	    support here: Added new field tcb_data_atts, which is analogous to
**	    tcb_key_atts but lists ALL attributes. These two attribute pointer
**	    lists are used for compressing and uncompressing data & keys.
**	    Added new field tcb_comp_katts_count to the TCB. Temporary tables
**	    are always heaps and do not have indices, so they may not use
**	    index compression. This routine re-checks the relstat and complains
**	    if index compression is on. tcb_comp_katts_count is set to 0 for
**	    all temporary tables.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added file extend changes.  Added support for the tcb_checkeof
**		flag.  This indicates that we should update the tcb allocation
**		fields to find out the true EOF.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements:
**	    Added 'recovery' and 'location' and
**	    'loc_count' arguments.
**	    Added DM2T_TMP_MODIFIED to allow modify of temp tables.
**	    Pass DM2F_DEFERRED_IO when creating a new temp table.
**	    Support multi-location temporaries.
**	15-jul-1992 (bryanp)
**	    Set tcb_lalloc properly when building a temp tcb following a modify.
**	26-aug-1992 (bryanp)
**	    Temporary Tables bug fixes to dm2t_temp_build_tcb: set Btree key
**	    information (tcb_klen, tcb_kperpage, key attr array, etc.)
**	    correctly.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set up relcomptype and relpgtype and called
**	    dm1c_getaccessors to initialize tcb_acc_plv and tcb_acc_tlv.
**	    Calculated normal compression count using an accessor
**	    rather than in-line.
**	29-August-1992 (rmuth)
**	    Remove setting of DM2F_LOAD, temp tables that have not just been
**	    modifed will now always call dm1s_deallocate.
**	16-sep-1992 (bryanp)
**	    Move setting of relpgtype and relcomptype for temporary tables out
**	    of dm2t_temp_build_tcb and make its caller set these fields.
**	16-oct-1992 (rickh)
**	    Added default id transcription for attributes.
**	26-oct-1992 (rogerk)
**	    Added initialization of tcb's extended table list (tcb_et_list);
**	    Initialized the loc_config_id field of each entry in the table's
**	    location array.
**	30-October-1992 (rmuth)
**	    - Initialise tcb_tmp_hw_pageno.
**	    - If we are not building a new temporary table make sure that
**	      we always call dm2f_set_alloc.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	18-jan-1993 (rogerk)
**	    Enabled recovery flag in tbio in CSP as well as RCP since the
**	    DCB_S_RCP status is no longer turned on in the CSP.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	18-oct-1993 (jnash)
**	    Fix bug where temp tables incorrectly opened O_SYNC: open
**	    them with DCB_NOSYNC_MASK.
**	10-oct-93 (swm)
**	    Bug #56440
**	    Assign unique identifier (for use as an event value) to new
**	    tcb_unique_id field.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      20-jun-1994 (jnash)
**          Initialize tbio_sync_close, temp tables never sync'd.  Remove
**	    logical_log maintenance.
**	 4-jul-95 (dougb) bug # 69382
**	    Before fixing any pages for a temporary table, place it into the
**	    hcb queue.  This removes a window during which a consistency point
**	    will not be able to find a TCB for pages in the buffer cache
**	    (leading to E_DM9397_CP_INCOMPLETE_ERROR).
**	06-mar-1996 (stial01 for bryanp)
**	    Don't use DM_PG_SIZE, use relpgsize instead.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all the page header access as macro for New Page Format
**	    project. Also changed the Btree key per page calculation.
**	    Pass page size to dm1c_getaccessors().
**	17-jul-96 (pchang)
**	    Fixed bug 76526.  When setting unwritten allocation in the tbio of
**	    a temporary table that has spilled into a real file, we were
**	    incorrectly adding mct_allocated to tbio_lalloc instead of setting
**	    tbio_lalloc to mct_allocate - 1.
**      22-jul-1996 (ramra01 for bryanp)
**          Set up tcb_total_atts, and complete attribute information. Assume
**              That temporary tables can't be altered, for now.
**	20-sep-96 (kch)
**	    We now check for a work location (dcb->dcb_ext->ext_entry[i].flags
**	    & DCB_WORK) or a data location (dcb->dcb_ext->ext_entry[i].flags &
**	    DCB_DATA) when building the extent information for the table's
**	    location(s). This change fixes bug 78060.
**	15-Oct-1996 (jenjo02)
**	    Create tcb_mutex, tcb_et_mutex names using table name to uniquify
**	    for iimonitor, sampler.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	24-Feb-1997 (jenjo02 for somsa01)
**	    Cross-integrated 428939 from main.
**	    Added initialization of tbio_extended.
**	28-feb-97 (cohmi01)
**	    Keep track in hcb of tcbs created. Bugs 80242, 80050.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Caller has set relation->reltcpri to cache priority.
**	    Removed setting tbio_cache_priority to DM0P_NOPRIORITY.
**      03-Jun-97 (hanal04)
**          Disable group reads for temporary tables which span multiple
**          locations. This prevents attempts to read pages that do not
**          exist. (Pr# 68 / Bug# 82762 which is similar to Bug# 72806).
**      12-jun-1997 (stial01)
**          dm2t_temp_build_tcb Include attribute info in kperpage calculation
**	19-sep-97 (stephenb)
**	    Replicator code cleanup, initialise rep_key_seq to zero
**      14-Jan-98 (thaju02)
**          Bug #87880 - inserting or copying blobs into a temp table chews
**          up cpu, locks and file descriptors. Account for tcb_et_list
**          during allocation of tcb. Initialize tcb_et_list.
**      09-feb-99 (stial01)
**          dm2t_temp_build_tcb() Pass relcmptlvl to dm1*_kperpage
**	18-Aug-1998 (consi01) Bug 91837 Problem INGSRV 502
**	    Extend the fix for bug 78060 20-sep-96 (kch) to allow the use of
**	    auxilary work locations as well as work and data locations.
**	15-Jan-2004 (schka24)
**	    Compute hash stuff here instead of making caller pass it.
**	03-Mar-2004 (gupsh01)
**	    Added support for alter table alter column.
**	15-Sep-2004 (wanfr01)
**	    Bug 111553, Problem INGSRV2655
**	    Do not immediately deallocate temp table if you received E_DM9246
**	    Because buffers still need to be flushed at the next CP.
**	14-dec-04 (inkdo01)
**	    Added collID for collation support.
**	13-Apr-2006 (jenjo02)
**	    Support CLUSTERED temp tables.
**	15-Aug-2006 (jonj)
**	    Support Indexes on Session Temps.
**      29-May-2007 (hanal04) Bug 115558
**          Tighten fix for bug 78060 to ensure the location's work 
**          extension is used unless the user specifically requested
**          II_DATABASE.
**	7-Aug-2008 (kibro01) b120592
**	    If being called from an online modify, the temporary table is
**	    allowed to go into a non-II_DATABASE data location (since that's 
**	    where the table it will replace exists).
**	29-Sept-2009 (troal01)
**		Added support for geospatial.
**	27-May-2010 (gupsh01) BUG 123823
**	    Fix error handling for error E_DM0075_BAD_ATTRIBUTE_ENTRY. 
*/
DB_STATUS
dm2t_temp_build_tcb(
DMP_DCB             *dcb,
DB_TAB_ID           *table_id,
DB_TRAN_ID	    *tran_id,
i4		    lock_id,
i4		    log_id,
DMP_TCB             **tcb,
DMP_RELATION        *relation,
DMF_ATTR_ENTRY	    **attr_array,
i4             	    attr_count,
DM2T_KEY_ENTRY	    **key_array,
i4             	    key_count,
i4             	    flag,
i4		    recovery,
i4		    num_alloc,
DB_LOC_NAME	    *location,
i4		    loc_count,
i4		    *IdomMap,
DB_ERROR	    *dberr,
bool		    from_online_modify)
{
    DMP_TCB		*BaseTCB;
    DMP_TCB		*t = 0, *itcb;
    DB_STATUS		status;
    i4             	i,j,k;
    bool                compare;
    i4             	local_error;
    i4             	offset = 0;
    bool		reclaimed;
    i4             	open_flags;
    DMP_HCB		*hcb = dmf_svcb->svcb_hcb_ptr;
    DMP_HASH_ENTRY	*hash;
    DM_MUTEX		*hash_mutex;
    bool		t_queued = FALSE;	/* Have we queued TCB? */
    bool		ok_data_loc;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    i4			alen;
    DB_ATTS		*a;
    PTR			nextattname;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
	dmd_check(E_DM9320_DM2T_BUILD_TCB);
#endif

    dmf_svcb->svcb_stat.tbl_create++;
    *err_code = 0;
    hash = &hcb->hcb_hash_array[
	HCB_HASH_MACRO(dcb->dcb_id, relation->reltid.db_tab_base, relation->reltid.db_tab_index)];
    hash_mutex = &hcb->hcb_mutex_array[
	HCB_MUTEX_HASH_MACRO(dcb->dcb_id, relation->reltid.db_tab_base)];
    for (;;)
    {
	/* 
	** Cannot specify btree index compression
	*/
	if ( relation->relstat & TCB_INDEX_COMP )
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	/* If creating an index, caller passed *BaseTCB in *tcb */
	if ( relation->relstat & TCB_INDEX )
	    BaseTCB = *tcb;
	else
	    BaseTCB = (DMP_TCB*)NULL;

	relation->relattnametot = 0;
	for ( i = 0; i < attr_count; i++)
	{
	    for (alen = DB_ATT_MAXNAME;  
		attr_array[i]->attr_name.db_att_name[alen-1] == ' ' 
			&& alen >= 1; alen--);
	    relation->relattnametot += alen;
	}

	reclaimed = FALSE;
	for (;;)
	{
	    /*	Allocate a TCB for this temporary table/index. */
	    status = dm2t_alloc_tcb(relation, relation->relloccount,
		    dcb, &itcb, dberr);
	    if (status != E_DB_OK)
	    {
		/* If the error indicates out of memory, then
		** try to reclaim if haven't tried already. */

		if (dberr->err_code != E_DM923D_DM0M_NOMORE)
		    break;
		if (reclaimed == TRUE)
		    break;
		reclaimed = TRUE;
		status = dm2t_reclaim_tcb(lock_id, log_id, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    else
		break;
	}
	if (status != E_DB_OK)
	    break;

	t = itcb;

	/* Override default TCB values */
	t->tcb_temporary = TCB_TEMPORARY;
	t->tcb_hash_bucket_ptr = hash;
	t->tcb_hash_mutex_ptr = hash_mutex;
	t->tcb_keys = key_count;
	t->tcb_logging = recovery;
	t->tcb_nofile = FALSE;
	t->tcb_update_idx = FALSE;
	t->tcb_no_tids = FALSE;

	/*
	** Override default Table IO values
	*/
	t->tcb_table_io.tbio_checkeof = FALSE;
	t->tcb_table_io.tbio_temporary = TRUE;
	t->tcb_table_io.tbio_sysrel = FALSE;
	t->tcb_table_io.tbio_extended = FALSE;

	t->tcb_table_io.tbio_sync_close = FALSE;

	/*
	** Pr# 68 / Bug# 82762
	** Must ensure that group read is disabled for
	** Temporary tables which span multiple locations.
	*/
	if (t->tcb_table_io.tbio_loc_count > 1)
	    t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_NO_GROUPREAD;

	/*
	** Build extent information for this table's location(s):
	*/
    	nextattname = t->tcb_atts_names;
	for (k = 0; k < t->tcb_table_io.tbio_loc_count; k++)
	{
	    t->tcb_table_io.tbio_location_array[k].loc_id = k;
	    STRUCT_ASSIGN_MACRO(location[k],
			t->tcb_table_io.tbio_location_array[k].loc_name);

	    if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&location[k],
			sizeof(DB_LOC_NAME)) == 0)
	    {
		t->tcb_table_io.tbio_location_array[k].loc_ext =
							    dcb->dcb_root;
		t->tcb_table_io.tbio_location_array[k].loc_config_id = 0;
		t->tcb_table_io.tbio_location_array[k].loc_status = 
							    LOC_VALID;
	    }
	    else
	    {
		for (i = 0; i < dcb->dcb_ext->ext_count; i++)
		{
		    compare = MEcmp((char *)&location[k],
			  (char *)&dcb->dcb_ext->ext_entry[i].logical,
			  sizeof(DB_LOC_NAME));

		    ok_data_loc = FALSE;
		    if (dcb->dcb_ext->ext_entry[i].flags & DCB_DATA)
		    {
			if (from_online_modify)
			    ok_data_loc = TRUE;
			else if (STxcompare((char *)&location[k],
                                         sizeof(DB_LOC_NAME),
                                         ING_DBDIR, sizeof(DB_LOC_NAME),
                                         TRUE, TRUE) == 0)
			    ok_data_loc = TRUE;
		    }
		    if (compare == 0 &&
			(ok_data_loc ||
                         (dcb->dcb_ext->ext_entry[i].flags & DCB_WORK) ||
			 (dcb->dcb_ext->ext_entry[i].flags & DCB_AWORK)
                        ))
		    {
			t->tcb_table_io.tbio_location_array[k].loc_ext
				= &dcb->dcb_ext->ext_entry[i];
			t->tcb_table_io.tbio_location_array[k].loc_config_id
				= i;
			t->tcb_table_io.tbio_location_array[k].loc_status =
						    LOC_VALID;
			if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
			    t->tcb_table_io.tbio_location_array[k].loc_status |= 
						    LOC_RAW;
			break;
		    }
		}
		if (i >= dcb->dcb_ext->ext_count)
		{
		    SETDBERR(dberr, 0, E_DM001D_BAD_LOCATION_NAME);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    if ( status )
		break;

	}
	if ( status )
	    break;

	if (flag == DM2T_LOAD)
	    t->tcb_loadfile = TCB_LOAD;

	for ( i = 1; i <= attr_count; i++)
	{
	    a = &(t->tcb_atts_ptr[i]);
	    for (alen = DB_ATT_MAXNAME;  
		attr_array[i-1]->attr_name.db_att_name[alen-1] == ' ' 
			&& alen >= 1; alen--);
	    t->tcb_atts_ptr[i].encflags = 0;
	    t->tcb_atts_ptr[i].encwid = 0;

	    /* make sure we have space for attribute name */
	    if (t->tcb_atts_used + alen + 1 > t->tcb_atts_size)
	    {
		SETDBERR(dberr, 0, E_DM0075_BAD_ATTRIBUTE_ENTRY);
		status = E_DB_ERROR;
		break;
	    }

	    a->attnmlen = alen;
	    a->attnmstr = nextattname;
	    MEcopy(attr_array[i-1]->attr_name.db_att_name, alen, nextattname);
	    nextattname += alen;
	    *nextattname = '\0';
	    nextattname++;
	    t->tcb_atts_used += (alen + 1);

	    a->type = attr_array[i-1]->attr_type;
	    a->length = attr_array[i-1]->attr_size;
	    a->precision = attr_array[i-1]->attr_precision;
	    a->flag = attr_array[i-1]->attr_flags_mask;
	    COPY_DEFAULT_ID( attr_array[i-1]->attr_defaultID, a->defaultID );
	    a->defaultTuple = 0;
	    a->collID = attr_array[i-1]->attr_collID;
	    a->geomtype = attr_array[i-1]->attr_geomtype;
	    a->srid = attr_array[i-1]->attr_srid;
	    a->replicated = FALSE;
	    a->rep_key_seq = 0;
	    a->key = 0;
	    a->offset = offset;
	    a->intl_id = i;
	    a->ordinal_id = i;
	    a->ver_added = 0;
	    a->ver_dropped = 0;
	    a->val_from = 0;
	    a->ver_altcol = 0;

	    /*
	    ** Set up list of pointers to attribute info for use in
	    ** data compression. Note that the tcb_data_rac and
	    ** tcb_key_atts lists are indexed from 0, not from 1.
	    */
	    t->tcb_data_rac.att_ptrs[i - 1] = a;

	    offset = offset + attr_array[i-1]->attr_size;
	}

	if ( status )
	    break;

	if (t->tcb_keys != t->tcb_rel.relkeys ||
	    attr_count != t->tcb_rel.relatts)
	{
	    SETDBERR(dberr, 0, E_DM0075_BAD_ATTRIBUTE_ENTRY);
	    status = E_DB_ERROR;
	    break;
	}

	status = dm1c_rowaccess_setup(&t->tcb_data_rac);
	if (status != E_DB_OK)
	    break;

	for (i = 0; i < key_count; i++)
	{
	    compare = FALSE;
	    for ( j = 1; j <= attr_count; j++ )
	    {
		DB_ATT_NAME tmpattnm;

		MEmove(t->tcb_atts_ptr[j].attnmlen,
		    t->tcb_atts_ptr[j].attnmstr, ' ',
		    DB_ATT_MAXNAME, tmpattnm.db_att_name);

		if (MEcmp(tmpattnm.db_att_name,
		  (char *)&key_array[i]->key_attr_name,
		  sizeof(key_array[i]->key_attr_name)) == 0)
		{
		    compare = TRUE;
		    break;
		}
	    }
	    if (compare == FALSE)
	    {
		SETDBERR(dberr, 0, E_DM001C_BAD_KEY_SEQUENCE);
		status = E_DB_ERROR;
		break;
	    }
	    t->tcb_key_atts[i] = &t->tcb_atts_ptr[j];
	    /* j-th attribute is i-th element of the key (i >= 1) */
	    t->tcb_atts_ptr[j].key = i + 1;
	    /* offset of this key is the length of key seen so far */
	    t->tcb_atts_ptr[j].key_offset = t->tcb_klen;
	    t->tcb_klen += t->tcb_atts_ptr[j].length;
	    t->tcb_atts_ptr[j].flag &= ~ATT_DESCENDING;
	    if (key_array[i]->key_order == DM2T_DESCENDING)
		t->tcb_atts_ptr[j].flag |= ATT_DESCENDING;
	    t->tcb_att_key_ptr[i] = t->tcb_atts_ptr[j].intl_id;
	}
	if ( status )
	    break;

	t->tcb_tperpage = DMPP_TPERPAGE_MACRO(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize,t->tcb_rel.relwid);

	/*
	** Init tcb fields for BTREE temporary tables and indexes
	*/
	if ( t->tcb_rel.relspec == TCB_BTREE )
	{
	    bool leaf_setup;

	    if ( t->tcb_rel.relstat & TCB_INDEX )
	    {
		/* Btree secondary can have non-key fields and/or
		** compressed keys
		*/
		init_bt_si_tcb(t, &leaf_setup);
	    }
	    else
	    {
		/* For clustered primary btree, the leaf is the entire row */
		if ( t->tcb_rel.relstat & TCB_CLUSTERED )
		{
		    offset = 0;
		    for ( i = 0; i < t->tcb_keys; i++ )
		    {
			/* 
			** If CLUSTERED Btree, we need separate copies of
			** key DB_ATTS, one for the Leaf pages, and one
			** for the Index pages.  The Leaf attributes will
			** have key_offset set to the natural offset of
			** the column in the tuple; the Index keys are
			** extracted and concatenated together, so their
			** key_offsets will relative to the concatenated
			** key pieces.
			**
			** tcb_key_atts are those of the Leaf,
			** tcb_ixkeys are an array of pointers to the
			** tcb_ixatts_ptr Index copies.
			** 
			** Note the space for tcb_ixkeys and tcb_ixatts was
			** preallocated by dm2t_alloc_tcb when CLUSTERED.
			*/
			/* Copy the LEAF key attribute */
			STRUCT_ASSIGN_MACRO(*t->tcb_key_atts[i],
					    t->tcb_ixatts_ptr[i]);

			/* Point to the new INDEX attribute */
			t->tcb_ixkeys[i] = &t->tcb_ixatts_ptr[i];

			/* Fiddle the key offsets */
			t->tcb_key_atts[i]->key_offset = t->tcb_key_atts[i]->offset;
			t->tcb_ixkeys[i]->key_offset = offset;
			offset += t->tcb_ixkeys[i]->length;
		    }
		    /* all row attributes are in the leaf */
		    t->tcb_leaf_rac.att_ptrs = t->tcb_data_rac.att_ptrs;
		    t->tcb_leaf_rac.att_count = t->tcb_data_rac.att_count;
		    t->tcb_leafkeys  = t->tcb_key_atts;
		    t->tcb_klen = t->tcb_rel.relwid;

		    /* tcb_ixkeys point to the INDEX DB_ATTS in
		    ** tcb_ixatts_ptr */

		    /* INDEX pages just have the key */
		    t->tcb_ixklen = offset;
		    leaf_setup = TRUE;
		}
		else
		{
		    /* For (nonclustered) primary btree, att/keys
		    ** are same on LEAF/INDEX pages
		    */
		    t->tcb_ixkeys = t->tcb_key_atts;
		    t->tcb_ixklen = t->tcb_klen;
		    t->tcb_leaf_rac.att_ptrs = t->tcb_ixkeys;
		    t->tcb_leaf_rac.att_count = t->tcb_keys;
		    t->tcb_leafkeys  = t->tcb_key_atts;
		    leaf_setup = FALSE;
		}

		/* Clustered or not, INDEX pages just have keys */
		t->tcb_index_rac.att_ptrs = t->tcb_ixkeys;
		t->tcb_index_rac.att_count = t->tcb_keys;

		/* Clustered or not, primary LEAF range entries just
		** have keys
		*/
		t->tcb_rngkeys = t->tcb_ixkeys;
		t->tcb_rng_rac = &t->tcb_index_rac;
		t->tcb_rngklen = t->tcb_ixklen;
	    }
	    status = dm1c_rowaccess_setup(&t->tcb_index_rac);
	    if (status != E_DB_OK)
		break;

	    /* If the leaf may have different compression needs than
	    ** the index, set up its row accessor.  Otherwise just
	    ** copy the stuff from the index row accessor to the leaf.
	    */
	    if (leaf_setup)
	    {
		status = dm1c_rowaccess_setup(&t->tcb_leaf_rac);
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		t->tcb_leaf_rac.cmp_control = t->tcb_index_rac.cmp_control;
		t->tcb_leaf_rac.dmpp_compress =
			t->tcb_index_rac.dmpp_compress;
		t->tcb_leaf_rac.dmpp_uncompress =
			t->tcb_index_rac.dmpp_uncompress;
		t->tcb_leaf_rac.control_count =
			t->tcb_index_rac.control_count;
		t->tcb_leaf_rac.worstcase_expansion =
			t->tcb_index_rac.worstcase_expansion;
		t->tcb_leaf_rac.compresses_coupons =
			t->tcb_index_rac.compresses_coupons;
	    }

	    /* Compute kperpage for LEAF/INDEX.
	    ** Session temporary tables always have table
	    ** version DMF_T_VERSION
	    */
	    t->tcb_kperpage = 
		dm1b_kperpage(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		DMF_T_VERSION, t->tcb_index_rac.att_count, t->tcb_ixklen,
		DM1B_PINDEX,
		sizeof(DM_TID), t->tcb_rel.relstat & TCB_CLUSTERED);
	    t->tcb_kperleaf = 
		dm1b_kperpage(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		DMF_T_VERSION, t->tcb_leaf_rac.att_count, t->tcb_klen,
		DM1B_PLEAF,
		sizeof(DM_TID), t->tcb_rel.relstat & TCB_CLUSTERED);

	    if ( t->tcb_rel.relstat & TCB_CLUSTERED )
		t->tcb_tperpage = t->tcb_kperleaf;

	} /* btree */
	/* Note: temp tables aren't RTREE which is another case that
	** requires some key setup.
	*/

	/*
	** Open the physical file(s) for this table.
	*/
	if (flag == DM2T_TMP_MODIFIED)
	    open_flags = DM2F_OPEN;
	else
	    open_flags = (DM2F_TEMP | DM2F_DEFERRED_IO);

	status = open_tabio_cb(&t->tcb_table_io, lock_id, log_id, open_flags,
	    DCB_NOSYNC_MASK, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Now, our TCB is pretty much complete.  Before faulting in
	** any pages for the temporary table, ensure that the CP thread
	** can find this TCB.
	*/
	if ( t->tcb_rel.relstat & TCB_INDEX )
	{
	    /* Link Index TCB to Base TCB */
	    t->tcb_parent_tcb_ptr = BaseTCB;
	    t->tcb_q_next = BaseTCB->tcb_iq_next;
	    t->tcb_q_prev = (DMP_TCB*)&BaseTCB->tcb_iq_next;
	    BaseTCB->tcb_iq_next->tcb_q_prev = t;
	    BaseTCB->tcb_iq_next = t;
	    BaseTCB->tcb_index_count++;
	    /* Note that Base TCB now has secondary index(es) */
	    BaseTCB->tcb_rel.relstat |= TCB_IDXD;
	    BaseTCB->tcb_update_idx = TRUE;

	    /* Note in BaseTCB that there are N unique indexes */
	    if ( t->tcb_rel.relstat & TCB_UNIQUE )
	    {
		BaseTCB->tcb_unique_index = 1;
		BaseTCB->tcb_unique_count++;
	    }

	    /* ...and if there are any STATEMENT_LEVEL_UNIQUE indexes */
	    if ( t->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE )
		BaseTCB->tcb_stmt_unique = TRUE;

	    /* Fill in the ikey map from that provided by caller */
	    for ( i = 0; i < DB_MAXIXATTS; i++ )
		t->tcb_ikey_map[i] = IdomMap[i];

	    /* Count another TCB opened in index's cache */
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_cache_tcbcount[t->tcb_table_io.tbio_cache_ix]++;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	else
	{
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount++;	/* keep track of TCBs allocated */
	    hcb->hcb_cache_tcbcount[t->tcb_table_io.tbio_cache_ix]++;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	    
	    dm0s_mlock( hash_mutex );
	    t->tcb_q_next = hash->hash_q_next;
	    t->tcb_q_prev = (DMP_TCB *)&hash->hash_q_next;
	    hash->hash_q_next->tcb_q_prev = t;
	    hash->hash_q_next = t;
	    dm0s_munlock( hash_mutex );
	    t_queued = TRUE;
	}

	if ((open_flags & DM2F_DEFERRED_IO) != 0 &&
	    flag != DM2T_TMP_INMEM_MOD)
	{
	    DMP_RCB		*tmp_rcb;

	    status = dm2r_rcb_allocate(t, (DMP_RCB *)NULL,
			    tran_id, lock_id, log_id, &tmp_rcb, dberr);
	    if (status != E_DB_OK)
		break;

	    status = dm1s_deallocate(tmp_rcb, dberr);
	    if (status != E_DB_OK)
	    {
		_VOID_ dm2r_release_rcb(&tmp_rcb, &local_dberr);
		break;
	    }

	    status = dm2r_release_rcb(&tmp_rcb, dberr);
	    if (status != E_DB_OK)
		break;
	    tmp_rcb = 0;
	}
	else
	{
	    t->tcb_table_io.tbio_lalloc = num_alloc - 1;
	    dm2f_set_alloc(t->tcb_table_io.tbio_location_array,
		t->tcb_table_io.tbio_loc_count, num_alloc);
	}

	*tcb = t;
	return (E_DB_OK);

    } /* end for */

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9271_BUILD_TEMP_TCB);
    }
    if (t && (dberr->err_code != E_DM9246_DM1S_DEALLOCATE))
    {
	/*
	** If we have entered this TCB into the hcb queue, remove it
	** now.  Don't want to leave a bogus TCB in any lists.
	*/
	if ( t_queued )
	{
	    dm0s_mlock( hash_mutex );
	    t->tcb_q_next->tcb_q_prev = t->tcb_q_prev;
	    t->tcb_q_prev->tcb_q_next = t->tcb_q_next;
	    dm0s_munlock( hash_mutex );

	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount--;	/* keep track of TCBs allocated */
	    hcb->hcb_cache_tcbcount[t->tcb_table_io.tbio_cache_ix]--;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	else if ( t->tcb_rel.relstat & TCB_INDEX &&
		  t->tcb_parent_tcb_ptr == BaseTCB )
	{
	    /* Then must unlink Index TCB from Base before deallocating */
	    t->tcb_q_next->tcb_q_prev = t->tcb_q_prev;
	    t->tcb_q_prev->tcb_q_next = t->tcb_q_next;
	    if ( --BaseTCB->tcb_index_count == 0 )
	    {
		/* Note that Base TCB no longer has secondary index(es) */
		BaseTCB->tcb_rel.relstat &= ~TCB_IDXD;
		BaseTCB->tcb_update_idx = FALSE;
	    }
	    if ( t->tcb_rel.relstat & TCB_UNIQUE )
	    {
		if ( --BaseTCB->tcb_unique_count == 0 )
		    BaseTCB->tcb_unique_index = 0;
	    }
	    /* Count one less TCB opened in index's cache */
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_cache_tcbcount[t->tcb_table_io.tbio_cache_ix]--;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}

	/*	Deallocate the FCB if it exists. */

	if (t->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    status = dm2f_release_fcb(lock_id, log_id,
		t->tcb_table_io.tbio_location_array,
		t->tcb_table_io.tbio_loc_count, 0, &local_dberr);
	}
	/* TCB mutexes must be destroyed */
	dm0s_mrelease(&t->tcb_mutex);
	dm0s_mrelease(&t->tcb_et_mutex);
	dm0m_deallocate((DM_OBJECT **)&t);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm2t_destroy_temp_tcb	- Destroy a TCB for a temporary table.
**
** Description:
**      This routine searchs in memory TCB's for table described by
**      the table identifier passed in.
**
**	If the TCB exists it is unreferenced then it is deallocated.
**
**	If the TCB is fixed by an internal server thread then this routine
**	will sleep until that thread unfixes the tcb and will then proceed
**	with the destroy.
**
**	If the TCB is referenced by an actual user thread (even if by
**	the session making the destroy call) then the temp table is not
**	destroyed and an error is returned.
**
** Inputs:
**	lock_list			Lock list holding the table lock that
**					reserves the temp-table ID number.
**      dcb                             The database to search.
**	table_id			The table to search for.
**
** Outputs:
**	err_code			The reason for the error status.
**					Which includes one of the following
**                                      error codes:
**                                      E_DM0043_DEALLOC_MEM_ERROR
**                                      E_DM0016_BAD_FILE_DELETE
**                                      E_DM005D_TABLE_ACCESS_CONFLICT
**                                      E_DM0054_NONEXISTENT_TABLE
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-1986 (Jennifer)
**          Created new for jupiter.
**	26-oct-1992 (rogerk)
**	    Check for current users of temp table tcb before releasing.  It
**	    is possible for write behind threads to have the tcb fixed.
**	18-oct-1993 (rogerk)
**	    Added consistency check for destroy request on a tcb that has a
**	    true open outstanding on it (presumably by the same session
**	    requesting the destroy).  This will cause this code to error rather
**	    than wait forever.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	15-Jan-2003 (schka24)
**	    Use locate-tcb utility instead of inline.
**	11-Jun-2004 (jenjo02)
**	    With parallel threads, temp TCBs may be opened/referenced
**	    by multiple sessions. If we have to wait for a busy TCB,
**	    re-locate_tcb() and if not found, then another thread
**	    has destroyed it, and that's ok.
**	15-Aug-2006 (jonj)
**	    Temp Table index support. 
**	    If destroying Base table, all attached indexes
**	    will also be destroyed.
**	    If destroying just an index, that's ok, too.
**	    Pushed release of Temp Table lock into here, previously
**	    up to the caller to screw up.
**	08-Sep-2006 (jonj)
**	    Before releasing locks, make sure we have a lock_list.
**	    When called after dmxe_abort, the lock_list (and all locks)
**	    have already been released and lock_list == 0.
**      11-Sep-2006 (horda03) Bug 116613
**          If the TCB is being released, then allow the other session
**          to complete the work.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Delete unneeded log-id and unused opflag parameters.
*/
DB_STATUS
dm2t_destroy_temp_tcb(
i4		    lock_list,
DMP_DCB             *dcb,
DB_TAB_ID           *table_id,
DB_ERROR            *dberr)
{
    DMP_HASH_ENTRY      *h;
    DMP_TCB		*t, *it, *DestTCB;
    DB_STATUS		status;
    DM_MUTEX		*hash_mutex;
    LK_LOCK_KEY		LockKey;
    STATUS		LkStatus;
    CL_ERR_DESC		SysErr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
	dmd_check(E_DM9322_DM2T_FIND_TCB);
#endif

    dmf_svcb->svcb_stat.tbl_destroy++;

    /*	Search HCB for this table, returns Base TCB */
    hash_mutex = NULL;
    locate_tcb(dcb->dcb_id, table_id, &t, &h, &hash_mutex);

    for (;;)
    {
	if (t == NULL)
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);
	    break;
	}

	if (t->tcb_open_count != 0)
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

        /* horda03 - if the tcb is already in the process of being released
        ** let that session do its work.
        */
        if (t->tcb_status & TCB_BEING_RELEASED)
        {
           status = E_DB_OK;
           break;
        }

	/*
	** If the tcb is currently in use then wait for all current fixers
	** to unfix it.
	**
	** Also must wait if the TCB is busy. This can occur even when
	** tcb_ref_count and tcb_valid_count are zero because 
	** dm2t_release_tcb marks the TCB as TCB_BUSY, then releases
	** the protecting mutex.
	*/
	while ( t && t->tcb_status & TCB_BUSY || t->tcb_ref_count )
	{
	    /*
	    ** Consistency Check:  check for a caller requesting a destroy
	    ** operation on a tcb that is fixed by an actual user thread
	    ** rather than some background job.  This is an unexpected
	    ** condition as purge requests should only be made on tables
	    ** which are locked exclusively by the caller.  Such a request
	    ** may result in a "hanging server".
	    */
	    if (t->tcb_open_count > 1)
	    {
		/*
		** Log unexpected condition and try continuing.  At worst
		** the session will hang.
		*/
		uleFormat(NULL, E_DM9C5D_DM2T_PURGEFIXED_WARN, (CL_ERR_DESC *)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
		    (i4 *)NULL, err_code, (i4)6,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		    0, t->tcb_open_count, 0, t->tcb_ref_count,
		    0, t->tcb_status);
	    }

	    /*
	    ** Register reason for waiting.  Done for debug/trace purposes.
	    */
	    t->tcb_status |= TCB_TOSS_WAIT;

	    /* Note that the wait call returns without the hash mutex. */
	    dm2t_wait_for_tcb(t, lock_list);
	    hash_mutex = NULL;

	    /* After waiting, refind the TCB */
	    locate_tcb(dcb->dcb_id, table_id, &t, &h, &hash_mutex);
	}

	/* If Base TCB is now gone, that's OK */
	if ( t )
	{
	    DestTCB = (DMP_TCB*)NULL;

	    /* If caller is destroying Base table, destroy it and all indexes */
	    if ( table_id->db_tab_index <= 0 )
		DestTCB = t;
	    else
	    {
		for ( it = t->tcb_iq_next; it != (DMP_TCB*)&t->tcb_iq_next; )
		{
		    if ( it->tcb_rel.reltid.db_tab_index == table_id->db_tab_index )
		    {
			DestTCB = it;

			if ( it->tcb_status & TCB_BUSY || it->tcb_ref_count )
			{
			    it->tcb_status |= TCB_TOSS_WAIT;
			    dm2t_wait_for_tcb(it, lock_list);
			    hash_mutex = NULL;
			    /* After waiting, refind the Base TCB */
			    locate_tcb(dcb->dcb_id, table_id, &t, &h, &hash_mutex);

			    /* If still there, refind Index TCB */
			    if ( t )
			    {
				it = t->tcb_iq_next;
				continue;
			    }
			}
			break;
		    }
		    else
			it = it->tcb_q_next;
		}

		/* If it was there but is no longer after waiting, that's ok */
		if ( !t || it == (DMP_TCB*)&t->tcb_iq_next )
		{
		    if ( DestTCB )
		    {
			status = E_DB_OK;
			DestTCB = (DMP_TCB*)NULL;
		    }
		    else
		    {
			/* Wasn't there to begin with */
			SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);
			status = E_DB_ERROR;
		    }
		}
		
	    }
	    if ( DestTCB )
	    {
		status = dm2t_release_tcb(&DestTCB, (i4)0, lock_list, 0, dberr);

		dm0s_munlock(hash_mutex);
		hash_mutex = NULL;

		/*
		** Release the table lock that reserves the temp table_id.
		**
		** If destroying just an Index, release only that lock;
		** if destroying the Base table and all underlying indexes,
		** LK_PARTIAL will release all locks matching db_tab_base,
		** indexes included.
		*/
		if ( status == E_DB_OK && lock_list )
		{
		    LockKey.lk_type = LK_TABLE;
		    LockKey.lk_key1 = dcb->dcb_id;
		    LockKey.lk_key2 = table_id->db_tab_base;
		    LockKey.lk_key3 = table_id->db_tab_index;
		    LockKey.lk_key4 = 0;
		    LockKey.lk_key5 = 0;
		    LockKey.lk_key6 = 0;

		    LkStatus = LKrelease((table_id->db_tab_index != 0) 
						? 0 : LK_PARTIAL,
					 lock_list, (LK_LKID*)NULL, &LockKey,
					 (LK_VALUE*)NULL, &SysErr);
		    if ( LkStatus )
		    {
			uleFormat(NULL, LkStatus, &SysErr, ULE_LOG, NULL, (char * )0,
					0L, (i4 *)NULL, err_code, 0);
			uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &SysErr,
				    ULE_LOG, NULL,(char * )0, 0L, (i4 *)NULL, err_code,
				    1, 0, lock_list);
			SETDBERR(dberr, 0, E_DM009D_BAD_TABLE_DESTROY);
			status = E_DB_ERROR;
		    }
		}
	    }
	}
	else
	    status = E_DB_OK;

	break;
    }

    if ( hash_mutex )
	dm0s_munlock(hash_mutex);

    return(status); 
}

/*{
** Name: dm2t_purge_tcb	- Purge a TCB.
**
** Description:
**      This routine purge the tcb and also upate the tcb lock value so
**	that if someone has already looked into the table information
**	knows to re-read the current table information.
**
**	TCB purging is usually (currently, always) done in a recovery
**	context.  Thus we're following log records and have no control
**	over the order in which things take place.  In particular,
**	partitions may get purged without the master being around,
**	and we have to deal with that.
**
** Inputs:
**      dcb                             The database to search.
**	table_id			The table to search for.
**      flag				Flag to indicate if the reltups,
**					relpages count should should be
**					updated.  Pass DM2T_PURGE always.
**	log_id				Log identifier.
**	lock_id				Lock list identifier.
**	tran_id				NOTUSED, ignored (FIXME).
**      db_lockmode                     NOTUSED, ignored.
** Outputs:
**	err_code			The reason for the error status.
**					Which includes one of the following
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-Sep-1987 (ac)
**          Created for Jupiter.
**	27-Oct-1988 (teg)
**	    Modified how table is open, so that tables that are missing
**	    their data file can still have the TCB purged.
**	 6-Feb-1989 (rogerk)
**	    Added call to dm0p_tblcache to invalidate cached pages for
**	    this table.
**	15-may-1989 (rogerk)
**	    Check return value of dm0p_tblcache call.
**	24-oct-1991 (jnash)
**	    Relocate "return(E_DB_ERROR)" outside curly brackets
**	    for LINT.
**	26-oct-1992 (rogerk)
**	    Updated for Reduced Logging Project.  Changed to use new
**	    bumplock routine.
**	14-dec-1992 (rogerk)
**	    Changed access mode on the table open to DM2T_A_OPEN_NOACCESS
**	    to accomodate gateway files which do not like granting full
**	    access to tables.
**	23-aug-1993 (rogerk)
**	    Changed to call bumplock_tcb when table can't be opened even for
**	    purges on secondary indexes.  We can't bypass this step.
**	19-Feb-2004 (schka24)
**	    Don't open TCB, gets us in trouble with partitions and is
**	    wasted work anyway.  Toss cache pages, fix TCB nobuild, and
**	    if we come up with one, toss it.
**	26-Mar-2004 (schka24)
**	    Above change missed the fact that dm2t-close (the old way)
**	    turns off the purge flag for fixed core catalogs.  These
**	    can't be purged, and in fact dmve deals with them specially
**	    by hand.
*/
DB_STATUS
dm2t_purge_tcb(
DMP_DCB             *dcb,
DB_TAB_ID           *table_id,
i4             flag,
i4		    log_id,
i4		    lock_id,
DB_TRAN_ID	    *tran_id, /* notused */
i4             db_lockmode, /* notused */
DB_ERROR            *dberr)
{
    DB_STATUS		status;
    DMP_TCB		*tcb;
    DB_TAB_TIMESTAMP	timestamp;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** If this table can be opened by a server using a different
	** buffer manager then call dm0p_tblcache to invalidate any
	** pages cached by other servers on this table.
	*/
	if (dcb->dcb_bm_served == DCB_MULTIPLE)
	{
	    status = dm0p_tblcache(dcb->dcb_id, table_id->db_tab_base,
		DM0P_MODIFY, lock_id, log_id, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/* If this is a fixed core catalog, just return.  These can't
	** be close-purged.  The caller (DMVE) will simply have to deal
	** with manually updating the TCB, if necessary.  (See e.g. the
	** SM2 undo handling.)
	** The test for iirelation catches iirel_idx too.
	*/
	if (table_id->db_tab_base == DM_B_RELATION_TAB_ID
	  || table_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID
	  || table_id->db_tab_base == DM_B_INDEX_TAB_ID)
	    return (E_DB_OK);

	/* We want to find the TCB so we can close-purge it.  Don't
	** however bother building a TCB just to throw it away, plus
	** that will get us in trouble with partitions if the master
	** TCB isn't around.
	** Note: no tran_id needed since we're doing nobuild.
	*/
	status = dm2t_fix_tcb(dcb->dcb_id, table_id, NULL,
		lock_id, log_id, DM2T_NOBUILD, dcb, &tcb, dberr);
	if (status == E_DB_OK)
	{
	    /* Found some sort of TCB, pitch it (assuming that the
	    ** caller asked for DM2T_PURGE -- FIXME maybe this should
	    ** be implied given the name of this routine!
	    */
	    status = dm2t_unfix_tcb(&tcb, flag, lock_id, log_id, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else
	{
	    /*
	    ** If we couldn't open the table, try invalidating the TCB
	    ** directly to communicate a table change to other servers.
	    **
	    ** The lock invalidation must be done on the base table id,
	    ** not on an index or partition id, but dm2t_bumplock_tcb()
	    ** knows that.
	    */
    	    if (dcb->dcb_served == DCB_MULTIPLE)
	    {
		status = dm2t_bumplock_tcb(dcb, 
				(DMP_TCB*)NULL, table_id, (i4)1, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	}
	return(E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9330_DM2T_PURGE_TCB);
    }
    return(E_DB_ERROR);
}

/*{
** Name: dm2t_current_counts
**
** Description:
**	This routine retrieves the latest row and page counts for a given TCB.
**	We want to take into account any ongoing queries that any threads
**	(in this server) might be running, so the total is:
**
**	o The tcb reltups and relpages, plus
**	o the tcb row and page deltas, plus
**	o all possibly-updating RCB row and page deltas
**
**	This last is pretty trivial for regular tables, and partitions,
**	but if we happen to be asking about a partitioned master, it's
**	nontrivial.  That's because the RCB list that was traditionally
**	used is specific to their owning TCB.  A master wants to know
**	about RCB's possibly updating any partition.
**
**	To answer the question quickly, any possibly-updating RCB is on
**	one or two lists.  It's on a list for its own TCB, via rcb_upd_list;
**	and it's on a list for the master TCB (if there is one), via
**	rcb_mupd_list.
**
**	The only tricky bit here is that the lists are standard doubly-
**	linked queues such that the queue pointers point at the lists
**	themselves, not at the start of the RCB.  Therefore the offset
**	back to the start of the RCB is different depending on which
**	list we're looking at!  Fortunately it's easy to tell just by
**	looking at the supplied TCB.
**
**	Realize of course that what's returned is just a snapshot.
**	As other queries continue to run, the numbers will change.
**
**	If overflow occurs in calculating rows or page count, return 
**	MAXI4. Some external code relies on relpages being at least 1 so
**	make sure that is the case until that code has been rectified.
**
**	The caller has to mutex the TCB before calling here.
**
** Inputs:
**	tcb		The TCB we want counts for
**	reltups		an output
**	relpages	an output
**
** Outputs:
**	reltups		Current row count
**	relpages	Current page count
**
** History:
**	16-Jan-2004 (schka24)
**	    Written.
**	16-Sep-2009 (smeke01) b122244
**	    If overflow occurs, return MAXI4 (rather than 0 or 1). 
**	    Leave lowest value returned for pages as 1.
**	25-Sep-2009 (smeke01) b122244
**	    Take account of those values that are deltas, and can 
**	    legitimately be negative.
**	10-may-2010 (stephenb)
**	    reltups and relpages are now i8 on disk (and therefore in the
**	    DMP_RELATION structure, use new CSadjust_i8counter to peform
**	    atomic counter adjustment. Also, restrict actual values to
**	    MAXI4 until we can handle larger numbers.
*/

void
dm2t_current_counts(DMP_TCB *tcb, i4 *reltups, i4 *relpages)
{
    DMP_RCB *rcb;	/* A possibly-updating RCB */
    i4 list_offset;	/* Proper list offset from front of RCB */
    i4 pages, tups;	/* Current counts */
    QUEUE *next;	/* Next queue entry in list */
    bool tups_overflow, pages_overflow;

    list_offset = CL_OFFSETOF(DMP_RCB, rcb_upd_list);
    if (tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)
	list_offset = CL_OFFSETOF(DMP_RCB, rcb_mupd_list);

    tups = pages = 0;
    tups_overflow = pages_overflow = FALSE;

    /* reltups is a total, and is clipped to 0 throughout the code */
    if ( (tcb->tcb_rel.reltups < 0 ) || ((MAXI4 - tcb->tcb_rel.reltups) < 0))
	tups_overflow = TRUE;
    else
	tups = (i4)tcb->tcb_rel.reltups;

    /* tcb_tup_adds is a delta and can be negative. Clip result to 0 */
    if ((tcb->tcb_tup_adds > 0 ) && ((MAXI4 - tcb->tcb_tup_adds) < tups))
	tups_overflow = TRUE;
    else
    {
	tups = tups + tcb->tcb_tup_adds;
	if (tups < 0) tups = 0;
    }

    /* relpages is a total, and is clipped to 1 throughout the code */
    if ( (tcb->tcb_rel.relpages < 0) || ((MAXI4 - tcb->tcb_rel.relpages) < 0))
	pages_overflow = TRUE;
    else
	pages = (i4)tcb->tcb_rel.relpages;

    /* tcb_page_adds is a delta and can be negative. Clip result to 0 whilst we are still accumulating */
    if ((tcb->tcb_page_adds > 0) && ((MAXI4 - tcb->tcb_page_adds) < pages))
	pages_overflow = TRUE;
    else
    {
	pages = pages + tcb->tcb_page_adds;
	if (pages < 0) pages = 0;
    }

    /* 
    ** RCB rcb_tup_adds and rcb_page_adds are deltas and can be negative.
    */ 
    for (next = tcb->tcb_upd_rcbs.q_next;
	 next != &tcb->tcb_upd_rcbs;
	 next = next->q_next)
    {
	/* Next points into an RCB, we need the start of the RCB */
	rcb = (DMP_RCB *) ((PTR) next - list_offset);

	if ( (rcb->rcb_tup_adds > 0) && ( (MAXI4 - rcb->rcb_tup_adds) < tups ) )
            tups_overflow = TRUE;
        else
	{
	    tups += rcb->rcb_tup_adds;
	    if (tups < 0) tups = 0;
	}

	/* As we're still accumulating don't clip to 1 just yet */
	if ( (rcb->rcb_page_adds > 0) && ((MAXI4 - rcb->rcb_page_adds) < pages) )
	    pages_overflow = TRUE;
	else
	{
	    pages += rcb->rcb_page_adds;
	    if (pages < 0) pages = 0;
	}
    }

    *reltups = tups_overflow ? MAXI4 : tups;
    *relpages = pages_overflow ? MAXI4 : ( pages < 1 ? 1 : pages);

} /* dm2t_current_counts */

/*{
** Name: dm2t_lookup_tabid	- Find Table Id in tcb cache given
**				  tblname/ownername
**
** Description:
**      This routine searchs the in memory TCB's for table described by
**      the table name and owner name passed in.  If if is found, the table ID
**	of the table is returned.  Otherwise, non-existent table is returned.
**
**	The current subroutine does not search 2-ary index TCB's. Currently,
**	user-visible temporary tables never have 2-ary index TCB's.
**
** Inputs:
**      dcb                             The database to search.
**	table_name			The table name to search for.
**	owner_name			The owner name to search for.
**
** Outputs:
**	table_id			The table ID, if a tCB is located.
**	err_code			Set if E_DB_ERROR is returned, to:
**					E_DM0054_NONEXISTENT_TABLE
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Non-existent table.
**	Exceptions:
**	    none
**
** History:
**	16-jan-1992 (bryanp)
**	    Created for temporary table enhancements project.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: changed name from dm2t_locate_tcb.
**	30-feb-1993 (rmuth)
**	    Make sure TCB's are VALID and not BUSY, being built, before
**	    we look at them.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
*/
DB_STATUS
dm2t_lookup_tabid(
DMP_DCB		*dcb,
DB_TAB_NAME     *table_name,
DB_OWN_NAME     *owner_name,
DB_TAB_ID       *table_id,
DB_ERROR	*dberr)
{
    DMP_HCB	*hcb;
    DMP_HASH_ENTRY      *h;
    DM_MUTEX	*hash_mutex;
    DMP_TCB		*t;
    i4		i;
    DB_STATUS		status;

    CLRDBERR(dberr);

    /*	Search HCB for this table. */

    /*
    ** Assume table is not found. We'll change our minds if we find it.
    */
    status = E_DB_ERROR;

    hcb = dmf_svcb->svcb_hcb_ptr;

    for (i = 0; i < HCB_HASH_ARRAY_SIZE; i++)
    {
	h = &hcb->hcb_hash_array[i];
	hash_mutex = &hcb->hcb_mutex_array[HCB_HASH_TO_MUTEX_MACRO(i)];
	/* With the hash array larger these days, a lot of buckets may be empty,
	** skip mutexing them if so.  This is a dirty-peek.
	*/
	if (h->hash_q_next != (DMP_TCB *) h)
	{
	    dm0s_mlock(hash_mutex);
	    for (t = h->hash_q_next;
		t != (DMP_TCB*) h;
		t = t->tcb_q_next)
	    {
		    if ((t->tcb_dcb_ptr == dcb)
			     &&
		    ( t->tcb_status & TCB_VALID )
			     &&
		    ( (t->tcb_status & TCB_BUSY) == 0 )
			     &&
		    (MEcmp((char *)&t->tcb_rel.relid, (char *)table_name,
			sizeof(DB_TAB_NAME)) == 0)
			     &&
		    (MEcmp((char *)&t->tcb_rel.relowner, (char *)owner_name,
			sizeof(DB_OWN_NAME)) == 0))
		{
		    status = E_DB_OK;
		    STRUCT_ASSIGN_MACRO(t->tcb_rel.reltid, *table_id);
		    break;
		}
	    }

	    dm0s_munlock(hash_mutex);
	}
	if (status == E_DB_OK)
	    break;
	/*
	** Else continue around and try the next hash queue
	*/
    }

    if (status != E_DB_OK)
	SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);

    return (status);
}

/*{
** Name: dm2t_update_logical_key()	- update unique id
**
** Description:
**	Update in the TCB the unique id which is used to generate unique id's
**	for the logical_key datatype.  The two fields "tcb_low_logical_key"
**	and "tcb_high_logical_key" are used to generate an 8 byte integer
**	which is unique within a table.
**
**	dm2t.c!build_tcb() initializes both "tcb_low_logical_key" and
**	"tcb_high_logical_key" to 0 when a TCB is first created.  The first
**	time a dm2r_put() on a relation is executed on a relation which
**	contains a logical_key, a call is made to dm2t_update_logical_key.
**
**	The input RCB may point to a partition.  We'll follow the links to
**	the master, as logical keys belong to the entire table -- not to a
**	specific partition.  (An RCB is passed so that we can do a mini
**	transaction in the same context as the RCB.)
**
**	The current iirelation tuple for the tcb is read and locked
**	exclusively.  The value in the database of the 8 byte unique integer
**	represented in the catalog by the "relhigh_logkey" and
**	"rellow_logkey" (refered to here a db_unique_int).  When a relation is
**	created the db_unique_int field is initialized to 0.  At any given time
**	(except during a small window when the relation tuple is being updated),
**	the value of db_uniq_int in the iirelation represents a unused unique
**	integer in the table.  In addition, the protocol implemented by this
**	routine (which is the only updater of this value) guarantees that all
**	values from db_unique_int through db_unique_int +
**	(TCB_LOGICAL_KEY_INCREMENT - 1) are guaranteed to be unused in the
**	current relation.
**
**	Once read and locked the iirelation tuple is updated, with following
**	replacement on the db_uniq_int:
**
**		db_uniq_int += TCB_LOGICAL_KEY_INCREMENT
**
**	In this way the server "reserves" the use of the next
**	TCB_LOGICAL_KEY_INCREMENT unique ids.  Once "reserved" the set of id's
**      is used by the server until either the TCB for the relation is
**	released, or it uses all of the integers allocated.  If the TCB for
**	the table is released before using all of the keys, then the rest of
**	the keys are discarded never to be used as unique keys.
**
**	This function is not called until the first append is made using the
**	passed in TCB.  So if a table is readonly for the lifetime of it's
**	TCB in the server, no set of unique integers will be reserved.
**
**	This design makes it possible for two servers to append to the same
**	table without blocking on each other over exclusive access to the
**	unique int field.  Also extra I/O's to the data base are limited to 2
**	(one read/one write) per every TCB_LOGICAL_KEY_INCREMENT appends
**	to a table.
**
**	Upon success this function returns with the tcb fields updated to match
**	what was just read from the database.
**
**	RECOVERY ISSUES:
**
**	The update of the unique id in the iirelation has the same
**	characteristics as a btree split.  The update of the iirelation must
**	not be backed out as part of another transaction.  Also the update of
**	the iirelation should not be "redone" as part of a fast-commit on-line
**	recovery.  The replace of the iirelation must be redone as part of
**	roll forward processing.  In order to insure this the update is done
**	in the context of a mini-transaction.
**
**	CALLER'S RESPONSIBILITIES:
**
**	o  This function assumes that the the tcb is semaphore locked on entry
**	   (and will remain locked on exit).
**
**	o  It is up to the caller to read and update the tcb version of the
**	   unique integer, and to call this routine only when a new range of
**	   unique id's is to be reserved.
**
**
** Inputs:
**      rcb                             An RCB for the table wanting the
**					unique id.
**	lock_list			lock_list of current xact.
**
** Outputs:
**      err_code                        Reason for error return status.
**		E_DM9354_OBJ_LOGICAL_KEY_OVERFLOW - 8 byte unique id overflowed.
**		E_DM
**
**	Returns:
**	    E_DB_OK
**
** Side Effects:
**	    Value of tuple in iirelation is updated.
**
** History:
**      05-mar-89 (mikem)
**	    written.
**	 4-feb-1991 (rogerk)
**	    Add support for Set Nologging.  If transaction is Nologging, then
**	    mark table-opens done here as nologging.  Also, don't log
**	    begin and end transactions.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    17-aug-1989 (Derek)
**		Changed hard-coded attribute-key number to compile-time
**		constants that are different for normal and B1 systems.
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed Begin and End Mini log records to use LSN's rather than LAs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      20-jul-1993 (pearl)
**          Bug 49115.  In dm2t_update_logical_key(); after call to dm2r_get(),
**          check the index part of the reltid to make sure we've got the
**          right tuple.
**	18-may-1994 (pearl)
**	    Bug 63428
**	    In dm2t_update_logical_key(), indicate in the iirelation rcb
**	    whether the user table is journaled.  Do this by assigning the
**	    rcb_journal field of the user table rcb to the rcb_usertab_jnl
**	    field of the iirelation rcb.  This is relevant to the log
**	    record, and if the table is journaled, the journal record.
**      23-Jul-1998 (wanya01)
**          X-integrate change 432896(sarjo01)
**          in dm2t_update_logical_key().
**          Bug 75355: Assure that all temporary database files created
**          for a COPY FROM load operation are closed. On NT, UNDOing
**          (deleting) these files when they are open causes UNDO to fail.
**	 4-mar-1999 (thaju02)
**	    For global session temporary tables (relstat2 & TCB_SESSION_TEMP)
**	    unique logical keys are now generated. BLOBs in global temporary
**	    tables are dependent on the logical keys being unique; the logical
**	    key is used to key into the temporary extension table.
**	16-Jan-2004 (schka24)
**	    Revise to use get-by-tid.
**	15-Jun-2004 (schka24)
**	    Fix duh, input almost has to be for a partition, follow TCB
**	    pointer to master.  Fix header comments that implied that a
**	    TCB pointer is passed in.
**	16-Apr-2010 (kschendel) SIR 123485
**	    Logical key updates are journaled, and one might happen as we're
**	    couponifying incoming blobs.  Make sure we've written any delayed
**	    BT necessary. (used to be in dmpe but this is the right place IMO.)
*/
DB_STATUS
dm2t_update_logical_key(
DMP_RCB	*input_rcb,
DB_ERROR	*dberr)
{
    DB_STATUS		status = 0;
    DMP_RCB		*rcb;
    DMP_TCB		*t = input_rcb->rcb_tcb_ptr->tcb_pmast_tcb;
    DMP_RELATION	relrecord;
    DM_TID		tid;
    u_i4	max_low_value = ((u_i4) MAXI4) -
                                (2 * TCB_LOGICAL_KEY_INCREMENT);
    u_i4	low_id = 0;
    u_i4	high_id = 0;
    LG_LSN	bm_lsn;
    LG_LSN	lsn;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9383_DM2T_UPD_LOGKEY_ERR);
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_UPDATE_LOGICAL_KEY (%~t,%~t)\n",
	sizeof(t->tcb_dcb_ptr->dcb_name), &t->tcb_dcb_ptr->dcb_name,
	sizeof(t->tcb_rel.relid), &t->tcb_rel.relid);
#endif

    /* If partition, internal error (could probably remove this) */
    if (t->tcb_rel.relstat2 & TCB2_PARTITION)
    {
	SETDBERR(dberr, 0, E_DM93F8_DM2T_NOT_MASTER);
	return (E_DB_ERROR);
    }

    if (t->tcb_rel.relstat2 & TCB_SESSION_TEMP)
    {
	/* Handle session temporary entirely in memory here */
	if (t->tcb_low_logical_key >= max_low_value)
	{
	    if (t->tcb_high_logical_key == (u_i4) MAXI4)
	    {
		SETDBERR(dberr, 0, E_DM9382_OBJECT_KEY_OVERFLOW);
		return (E_DB_ERROR);
	    }
	    t->tcb_high_logical_key++;
	    t->tcb_low_logical_key = 0;
	}
	else if (!t->tcb_low_logical_key && !t->tcb_high_logical_key)
	    t->tcb_low_logical_key++;
	else
	    t->tcb_low_logical_key += TCB_LOGICAL_KEY_INCREMENT;
	return (E_DB_OK);
    }

    dm0s_munlock(&t->tcb_mutex);

    for (;;)
    {

	status = dm2r_rcb_allocate(t->tcb_dcb_ptr->dcb_rel_tcb_ptr, 0,
	    &input_rcb->rcb_tran_id,
	    input_rcb->rcb_lk_id,
	    input_rcb->rcb_log_id, &rcb, dberr);
	if (status != E_DB_OK)
            break;

	rcb->rcb_k_duration |= RCB_K_TEMPORARY;

	/* is the base table journaled?  */
	rcb->rcb_usertab_jnl = is_journal(input_rcb);

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (input_rcb->rcb_xcb_ptr->xcb_flags & XCB_NOLOGGING)
	    rcb->rcb_logging = 0;

	/* Write any delayed BT necessary */
	if (input_rcb->rcb_xcb_ptr->xcb_flags & XCB_DELAYBT
	  && input_rcb->rcb_logging != 0)
	{
	    status = dmxe_writebt(input_rcb->rcb_xcb_ptr, rcb->rcb_usertab_jnl, dberr);
	    if (status != E_DB_OK)
	    {
		input_rcb->rcb_xcb_ptr->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}

	/*
	** dm2r_rcb_allocate() defaults to exclusive access, so this
	** get will get the tuple with exclusive access (this will
	** prevent other threads from changing this value at the same
	** time).
	*/

	status = dm2r_get(rcb, &t->tcb_iirel_tid, DM2R_BYTID,
			(char *) &relrecord, dberr);
	if (status != E_DB_OK)
	    break;

	/* Double-check that we got the right record */
	if (relrecord.reltid.db_tab_base != t->tcb_rel.reltid.db_tab_base
	  || relrecord.reltid.db_tab_index != t->tcb_rel.reltid.db_tab_index)
	{
	    TRdisplay("%@dm2t_update_logical_key expected (%d,%d) at %d, got (%d,%d)\n",
		t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
		t->tcb_iirel_tid.tid_i4,
		relrecord.reltid.db_tab_base, relrecord.reltid.db_tab_index);
	    SETDBERR(dberr, 0, E_DM9384_DM2T_UPD_LOGKEY_ERR);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** update the base table tcb from the database copy
	*/

	low_id = relrecord.rellow_logkey;
	high_id = relrecord.relhigh_logkey;

	if (low_id >= max_low_value)
	{
	    if (relrecord.relhigh_logkey == (u_i4) MAXI4)
	    {
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM9382_OBJECT_KEY_OVERFLOW);
		break;
	    }

	    relrecord.relhigh_logkey++;
	    relrecord.rellow_logkey = 0;
	}
	else
	{
	    relrecord.rellow_logkey += TCB_LOGICAL_KEY_INCREMENT;
	}

	if (rcb->rcb_logging)
	{
	    if ((status = dm0l_bm(rcb, &bm_lsn, dberr)) != E_DB_OK)
		break;
	}

	status = dm2r_replace(rcb, &t->tcb_iirel_tid,
			    DM2R_BYTID | DM2R_RETURNTID,
			    (char *)&relrecord, (char *)NULL, dberr);
	if (status != E_DB_OK)
	    break;

	/* dm2r_replace() guarantees page on disk for system
	** catalog replaces, no matter whether fast commit
	** enabled or not.
	*/

	if (rcb->rcb_logging)
	{
	    if ((status = dm0l_em(rcb, &bm_lsn, &lsn, dberr)) != E_DB_OK)
		break;
	}

	/* 21-Jan-1995 (canor01) moved from above */
	status = dm2r_release_rcb(&rcb, dberr);

	break;
    }
    if (status != E_DB_OK)
    {
	if (dberr->err_code == E_DM9382_OBJECT_KEY_OVERFLOW)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *) NULL, ULE_LOG, NULL,
		(char *) 0, (i4) 0, (i4 *) 0,
		err_code, 3,
		sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
		sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
		sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);
	}
	else
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *) NULL, ULE_LOG, NULL,
		(char * )0, 0L, (i4 *)NULL, err_code, 0);

	    uleFormat(NULL, E_DM9383_DM2T_UPD_LOGKEY_ERR,
		NULL, ULE_LOG , NULL, (char * )0,
		0L, (i4 *)NULL, err_code, sizeof(DB_DB_NAME),
		&t->tcb_dcb_ptr->dcb_name);
	}
	SETDBERR(dberr, 0, E_DM9384_DM2T_UPD_LOGKEY_ERR);
    }

    dm0s_mlock(&t->tcb_mutex);

    if (status == E_DB_OK)
    {
	/* update the tcb on success */

	t->tcb_high_logical_key = high_id;

	if(!high_id && !low_id)
	    low_id++;
	t->tcb_low_logical_key = low_id;
    }

    return (status);
}

/*{
** Name: dm2t_wait_for_tcb	    - wait for a TCB to become available
**
** Description:
**	If a TCB is marked busy, this routine allows the caller to wait for
**	the TCB to become free again.
**
**	This routine should be called while holding the hcb_mutex. The mutex
**	will be released while waiting for the TCB to become free. This
**	routine returns with the mutex NOT held.
**
**      Unless the caller has the TCB pinned in some manner (e.g. by
**      incrementing the reference count), callers MUST assume that upon
**      return, the TCB may have been deleted entirely.  No reference to
**      the TCB after a wait is allowed, without specific pinning.
**
** Inputs:
**	tcb			    the TCB which is currently busy
**	lock_list		    lock list to use for waiting.
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	12-may-1992 (bryanp)
**	    Abstracted out of current dm2t code for use by dm2umod.c
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	    Caller must hold TCB's hash_mutex, which we'll release.
**      14-Aug-2008 (kschendel) b120791
**          Don't reference tcb after mutex unlock, in case the tcb wait
**          is for disposal of the tcb.
*/
VOID
dm2t_wait_for_tcb(
DMP_TCB	    *tcb,
i4	    lock_list)
{
    i4 unique_id = tcb->tcb_unique_id; /* Fetch now while tcb valid */

    tcb->tcb_status |= TCB_WAIT;
    dm0s_eset(lock_list, DM0S_TCBWAIT_EVENT, unique_id);
    dm0s_munlock(tcb->tcb_hash_mutex_ptr);

    /* No references to tcb allowed beyond this point */

    /* Wait for the TCB to become free */
    dm0s_ewait(lock_list, DM0S_TCBWAIT_EVENT, unique_id);
    return;
}

/*
** Name: dm2t_awaken_tcb	    - wake up any sessions waiting on TCB
**
** Description:
**	If a TCB is marked busy, this routine allows the caller to awaken any
**	sessions waiting for the TCB to become free again.
**
**	This routine should be called while holding the hash_mutex, and returns
**	still holding the mutex.
**
**	The WAIT status as well as the specific wait reason are turned
**	of in the tcb_status field.
**
** Inputs:
**	tcb			    the TCB which is currently busy
**	lock_id			    lock list to use for waiting.
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	12-may-1992 (bryanp)
**	    Abstracted out of current dm2t code for use by dm2umod.c
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed to not turn off the TCB_BUSY
**	    status when waking up waiters of the TCB.  The BUSY status
**	    must be turned off explicitly by the caller.
**	18-jan-1993 (bryanp)
**	    Added support for DM2T_TMPMOD_WAIT for temporary table modifies.
**	    Also, clear the tcb_status field BEFORE releasing the lock event,
**	    so that any thread which is awoken will be sure to see the bits in
**	    the right state.
**	15-mar-1993 (jnash)
**	    Fix bug where DM2T_TMPMOD_WAIT used in place of TCB_TMPMOD_WAIT.
*/
VOID
dm2t_awaken_tcb(
DMP_TCB	    *tcb,
i4	    lock_list)
{
    i4		waiting_threads = (tcb->tcb_status & TCB_WAIT) != 0;

    tcb->tcb_status &= ~(TCB_WAIT | TCB_PURGE_WAIT | TCB_TOSS_WAIT |
			 TCB_BUILD_WAIT | TCB_XACCESS_WAIT | TCB_TMPMOD_WAIT);
    if (waiting_threads)
	dm0s_erelease(lock_list, DM0S_TCBWAIT_EVENT, tcb->tcb_unique_id);

    return;
}

/*{
** Name: dm2t_fx_tabio_cb	- Fix the tabio data structure
**
** Description:
**      This routine is used by callers who need a handle to a Table IO
**	Descriptor in order to read or write pages.  It fixes the table's
**	TCB and then returns a pointer to the TBIO structure contained
**	within the Table Descriptor.
**
**	If no table descriptor is found, then the call returns the warning
**	W_DM9C50_DM2T_FIX_NOTFOUND. This routine will never cause a new TCB
**	to be built into the cache.
**
**	The dm2t_fx_tabio_cb call is modified by the following actions:
**
**	    DM2T_VERIFY - indicates that the TCB should be verified if
**			running multi-server and the TCB is not currently
**			fixed by a caller which specified VERIFY.  When fixed
**			with this flag, the caller must also specify the
**			VERIFY flag when it unfixes the tabio.
**	    DM2T_NOWAIT - if specified then if the TCB is marked BUSY then
**			a W_DM9C51_DM2T_FIX_BUSY warning is returned rather
**			than waiting for the busy status to be turned off.
**	    DM2T_NOPARTIAL - if specified then a partially open TableIO cb will
**			not be returned.
**	    DM2T_CLSYNC - (ignored, should not be passed)
**
** Inputs:
**	db_id				Database id
**	table_id			Pointer to table id
**	tran_id				Pointer to transaction id
**	lock_list			Lock list id
**	log_id				Log id
**	flags				Options to the Fix action:
**					    DM2T_VERIFY
**					    DM2T_NOWAIT
**					    DM2T_NOPARTIAL
**
** Outputs:
**	tabio_cb			Pointer to Table IO control block
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	28-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	07-may-96 (nick)
**	    Modified misleading comments ; this routine NEVER builds a TCB.
**	1-Sep-2008 (kibro01) b120843
**	    Add flag to state whether the database is in recovery.
*/
DB_STATUS
dm2t_fx_tabio_cb(
i4             db_id,
DB_TAB_ID           *table_id,
DB_TRAN_ID          *tran_id,
i4             lock_id,
i4             log_id,
i4		    flags,
DMP_TABLE_IO	    **tabio_cb,
DB_ERROR            *dberr)
{
    DMP_TCB		*tcb = NULL;
    DB_STATUS		status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = dm2t_fix_tcb(db_id, table_id, tran_id, lock_id, log_id,
	(flags | DM2T_NOBUILD), (DMP_DCB *)NULL, &tcb, dberr);

    if (status == E_DB_OK)
    {
	tcb->tcb_table_io.tbio_in_recovery = 0;
	if (tcb->tcb_dcb_ptr->dcb_status & DCB_S_RECOVER)
		tcb->tcb_table_io.tbio_in_recovery = 1;
	*tabio_cb = &tcb->tcb_table_io;
	return (E_DB_OK);
    }

    if (status == E_DB_WARN)
    {
	*tabio_cb = NULL;
	return (E_DB_WARN);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C55_DM2T_FIX_TABIO);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2t_pin_tabio_cb 	- Pin a tabio data structure
**
** Description:
**	This function increments the TCB reference count in a TCB
**	previously fixed by a dm2t_fx_tabio_cb call.
**
**	A pointer to the parent TCB is computed and the reference count
**	incremented.
**
** Inputs:
**	tabio_cb			Pointer to TABLE_IO
** Outputs:
**	none
**      Returns:
**          VOID
**
** History:
**	09-Jun-1999 (jenjo02)
**	    Created for GatherWrite.
*/
VOID
dm2t_pin_tabio_cb(
DMP_TABLE_IO	*tabio_cb)
{
    DMP_TCB		*tcb, *base_tcb;

    tcb = (DMP_TCB *)((char *)tabio_cb - CL_OFFSETOF(DMP_TCB, tcb_table_io));
    /*
    ** Get pointer to base table tcb (in case this is a secondary index).
    ** Pin actions are always performed on the base table, not on secondaries.
    */
    base_tcb = tcb->tcb_parent_tcb_ptr;

    /* Lock the TCB hash entry while pinning. */
    dm0s_mlock(base_tcb->tcb_hash_mutex_ptr);
    base_tcb->tcb_ref_count++;
       
    dm0s_munlock(base_tcb->tcb_hash_mutex_ptr);

    return;
}

/*{
** Name: dm2t_unpin_tabio_cb 	- Unpin a tabio data structure
**
** Description:
**	This function decrements the TCB reference count in a TCB
**	previously pinned by a dm2t_pin_tabio_cb call.
**
**	A pointer to the parent TCB is computed and the reference count
**	decremented.
**
** Inputs:
**	tabio_cb			Pointer to TABLE_IO
**	lock_list			Caller's lock_list
**
** Outputs:
**	none
**      Returns:
**          VOID
**
** History:
**	09-Jun-1999 (jenjo02)
**	    Created for GatherWrite.
*/
VOID
dm2t_unpin_tabio_cb(
DMP_TABLE_IO	*tabio_cb,
i4	lock_list)
{
    DMP_TCB		*tcb, *base_tcb;

    tcb = (DMP_TCB *)((char *)tabio_cb - CL_OFFSETOF(DMP_TCB, tcb_table_io));
    /*
    ** Get pointer to base table tcb (in case this is a secondary index).
    ** Pin/unpin actions are always performed on the base table, not on secondaries.
    */
    base_tcb = tcb->tcb_parent_tcb_ptr;

    /* Lock the TCB hash entry while unpinning. */
    dm0s_mlock(base_tcb->tcb_hash_mutex_ptr);

    base_tcb->tcb_ref_count--;

    /* If there are waiters, awaken them */
    if (base_tcb->tcb_status & TCB_WAIT)
	dm2t_awaken_tcb(base_tcb, lock_list);
    
    dm0s_munlock(base_tcb->tcb_hash_mutex_ptr);

    return;
}

/*{
** Name: dm2t_ufx_tabio_cb 	- Unfix a tabio data structure
**
** Description:
**      This routine unfixes a Table IO Control block previously fixed
**	by a dm2t_fx_tabio_cb call.
**
**	A pointer to the parent TCB is computed and that control block
**	is unfixed.
**
**	If the caller originally fixed the tabio with the DM2T_VERIFY flag,
**	then it is expected to call this routine with that same flag.
**
**	The dm2t_ufx_tabio_cb call is modified by the following actions:
**
**	    DM2T_VERIFY - indicates that the tcb was fixed with the VERIFY
**			option.  When unfixed, this flag indicates to decrement
**			the count of fixes which have verified the tcb.
**	    DM2T_NORELUPDAT - specifies that iirelation page/tuple counts
**			should not be updated even if the page_adds or tup_adds
**			counts exceed 10% of the current stored values.
**
** Inputs:
**	tabio_cb			Pointer to TABLE_IO
**	flags				Options to the Fix action:
**					    DM2T_VERIFY
**					    DM2T_NORELUPDAT
**	lock_id				Lock list id
**
** Outputs:
**	err_code
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	28-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
*/
DB_STATUS
dm2t_ufx_tabio_cb(
DMP_TABLE_IO	*tabio_cb,
i4		flags,
i4		lock_id,
i4		log_id,
DB_ERROR	*dberr)
{
    DMP_TCB		*tcb;
    DB_STATUS		status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    tcb = (DMP_TCB *)((char *)tabio_cb - CL_OFFSETOF(DMP_TCB, tcb_table_io));
    status = dm2t_unfix_tcb(&tcb, flags, lock_id, log_id, dberr);
    if (status == E_DB_OK)
	return (E_DB_OK);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C56_DM2T_UFX_TABIO);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2t_init_tabio_cb 	- Initialize a tabio data structure
**
** Description:
**	This routine builds an initial shell TCB for a caller who wishes
**	to create a tabio structure in order to read/write database pages.
**
**	The routine allocates a TCB and initializes its tabio fields.
**	The TCB is marked TCB_PARTIAL and placed into the tcb cache.
**
**	The TABIO structure is also listed TBIO_PARTIAL_CB.  No files
**	are opened and none of the location entries are filled in.
**	Before the created Tabio structure can be used for IO purposes,
**	the required locations must be initialized and opened via the
**	dm2t_add_tabio_file call.
**
**	Since the tcb cache links secondary index tables to their base
**	tables, if this routine is called to initialize a 2nd index, it
**	will first allocate a shell base table tcb onto which to link the
**	shell index tcb.
**
**	If the requested tcb (or its parent table tcb) is found and is
**	busy, then this routine waits for it to become non-busy.
**
**	If a valid tcb is found for the requested table in the cache, then
**	no work is performed and this routine returns OK.
**
** Inputs:
**	dcb				Pointer to Database control block
**	table_id			Pointer to table id
**	table_name			Pointer to table name
**	table_owner			Pointer to table owner
**	loc_count			Number of locations in the table
**	lock_id				Lock list id
**	page_size			Page size for this table.
**
** Outputs:
**	err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	28-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	14-dec-1992 (rogerk)
**	    Bug fixes for partial TCB handling: initialize table extension list.
**	17-dec-1992 (rogerk)
**	    Initialize tbio_cache_valid_stamp and copy sec index value from
**	    the parent tcb.
**	18-jan-1993 (rogerk)
**	    Enabled recovery flag in tbio in CSP as well as RCP since the
**	    DCB_S_RCP status is no longer turned on in the CSP.
**	    Fixed problem with setting reason for wait status when waiting
**	    for exclusive access to the tcb.  Use correct 'base_tcb' pointer
**	    instead of 'tcb'.
**	 2-feb-1993 (rogerk)
**	    Initialized tcb_temporary field of tcb when building partial tcb.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	23-may-94 (jnash)
**	    In the RCP and ROLLFORWARDDB, fix all table's buffer manager
**	    priorities, making the DMF cache "flat".  This results in
**	    improved cache utilitization during recoveries, and in the
**	    case of the RCP, lessens the liklihood of encountering "absolute
**	    logfull".
**	20-jun-1994 (jnash)
**	    fsync project: Fill in table-level synchronization mode based on
**	    database open mode.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm2t_init_tabio_cb and dm2t_add_tabio_file
**		for managing a table's page size during recovery.
**	10-jul-96 (nick)
**	    Test for SCONCUR relations should be '<='.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          DMCM protocol. When lock TCB, set TCB_VALIDATED status.
**          When unlocking, cancel TCB_VALIDATED status.
**	15-Oct-1996 (jenjo02)
**	    Create tcb_mutex, tcb_et_mutex names using table name to uniquify
**	    for iimonitor, sampler.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	18-may-1998(angusm)
**	    Bug 83251: reset *err_code when tcb successfully fixed,
**	    else the W_DM9C50 warning can propagate up the error chain
**	    nd cause intermittent failures of various queries.
**	05-May-2000 (jenjo02)
**	    Save lengths of blank-stripped table and owner names in
**	    tcb_relid_len, tcb_relowner_len.
**	05-Mar-2002 (jenjo02)
**	    Mark Sequence Generator (DM_B_SEQ_TAB_ID) TBIO's
**	    as "sysrel" so they can use physical locking.
**	15-Jan-2004 (schka24)
**	    Make sure partitioned master is there if doing a partition.
**	29-Apr-2008 (jonj)
**	    Test TCB_BUSY rather than tcb_ref_count when checking for "busy"
**	    master/parent when opening an index/partition.
**      14-Aug-2008 (kschendel) b120791
**          Set TCB lock sooner, before queuing to hash chain, so that
**          lock-tcb can do its thing while the hash mutex is set.  (actually
**          lock-tcb doesn't need the mutex unless the TCB is on the hash
**          chain, but the code is already holding the hash mutex for the
**          duration of constructing the new TCB.
**	05-Jun-2009 (thaju02)
**	    Pass lock_id to dm0p_set_valid_stamp(). (B118454)
*/
DB_STATUS
dm2t_init_tabio_cb(
DMP_DCB		*dcb,
DB_TAB_ID	*table_id,
DB_TAB_NAME	*table_name,
DB_OWN_NAME	*table_owner,
i4		loc_count,
i4		lock_id,
i4		log_id,
i4		page_type,
i4		page_size,
DB_ERROR	*dberr)
{
    DMP_HCB		*hcb = dmf_svcb->svcb_hcb_ptr;
    DMP_HASH_ENTRY	*tcb_h;
    DMP_HASH_ENTRY	*toss_h;
    DMP_TCB		*request_tcb;
    DMP_TCB		*base_tcb;
    DMP_TCB		*tcb;
    DMP_TCB		*it;
    DM_MUTEX		*tcb_hm;		/* Mutex for related table hashes */
    DM_MUTEX		*toss_hm;
    DB_TAB_NAME		base_table_name;
    DB_TAB_ID		base_table_id;
    i4		retry = 0;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4		local_error;
    char		sem_name[CS_SEM_NAME_LEN + DB_TAB_MAXNAME + 10];
    char		*cptr;
    i4			i;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	tcb = NULL;
	base_tcb = NULL;
	request_tcb = NULL;

	/*
	** Check for an existing TCB for the specified table.
	** If the requested tcb is an index, lookup the base table
	** and search for an index tcb linked to it.
	**
	** locate_tcb() returns with the TCB's hash_mutex locked.
	*/
	tcb_hm = NULL;
	locate_tcb(dcb->dcb_id, table_id, &tcb, &tcb_h, &tcb_hm);
	if (tcb != NULL)
	{
	    /*
	    ** If the tcb is busy, wait for it to become non busy and
	    ** retry the search.
	    */
	    if (tcb->tcb_status & TCB_BUSY)
	    {
		/* Note that the wait call returns without the hash mutex. */
		dm2t_wait_for_tcb(tcb, lock_id);
		continue;
	    }

	    request_tcb = tcb;

	    /*
	    ** Base table found; if the caller requested a secondary index
	    ** then search the index chain for the tcb.
	    */
	    if (table_id->db_tab_index > 0)
	    {
		request_tcb = 0;
		for (it = tcb->tcb_iq_next;
		    it != (DMP_TCB *) &tcb->tcb_iq_next;
		    it = it->tcb_q_next)
		{
		    if (it->tcb_rel.reltid.db_tab_index ==
							table_id->db_tab_index)
		    {
			request_tcb = it;
			break;
		    }
		}
	    }
	    /* As long as the index lookup (if any) didn't fail we found the
	    ** right TCB.  Just return it.
	    */
	    if (request_tcb != NULL)
	    {
		dm0s_munlock(tcb_hm);
		if (dberr->err_code == W_DM9C50_DM2T_FIX_NOTFOUND)
		    CLRDBERR(dberr);
		return (E_DB_OK);
	    }
	}


	/*
	** If we are attempting to add a secondary index tcb and there is
	** currently no base table tcb, then we must first add a base tcb
	** onto which to link the one we are really intending to build (did
	** that make sense?).
	**
	** When we allocate a base table tcb like this, we do not know
	** the number of locations in the table and cannot allocate a valid
	** TCB structure to use for real actions on the table.  So instead,
	** this call creates a dummy tcb which will actually be discarded the
	** first time the base table is referenced and replaced with a "real"
	** base table tcb (the state of being a dummy tcb is indicated by it
	** having zero locations).
	**
	** For partitions, masters and partitions may be on different hash
	** chains.  So it could be that the master tcb is out there, in which
	** case our recursive call will simply find it.
	*/
	if ((table_id->db_tab_index != 0) && (tcb == 0))
	{

	    base_table_id.db_tab_base = table_id->db_tab_base;
	    base_table_id.db_tab_index = 0;
	    base_tcb = NULL;
	    /* For partitions, see if the master is around via lookup.
	    ** We already did this for secondary indexes (locate-tcb looks up
	    ** the base table).  Pass in already-held mutex.
	    */

	    if (table_id->db_tab_index < 0)
		locate_tcb(dcb->dcb_id, &base_table_id, &base_tcb, &toss_h, &tcb_hm);
	    if (base_tcb == NULL)
	    {
		/* Secondary index, or partition w/o master around. */
		dm0s_munlock(tcb_hm);
		tcb_hm = NULL;
		/*
		** Call init_tabio_cb recursively to build a base table tcb.
		** Since we can't really predict the base table name, we
		** just make one up.
		*/
		STmove(DB_NOT_TABLE, ' ', sizeof(DB_TAB_NAME), (PTR)&base_table_name);
    
		status = dm2t_init_tabio_cb(dcb, &base_table_id,
		    &base_table_name, table_owner,
		    (i4)0, lock_id, log_id, page_type, page_size, dberr);
		if (status != E_DB_OK)
		    break;
    
		/*
		** Lookup the base table as a double-check, then retry it all.
		*/
		locate_tcb(dcb->dcb_id, &base_table_id, &base_tcb, &toss_h, &tcb_hm);
		dm0s_munlock(tcb_hm);
		tcb_hm = NULL;
		if (base_tcb == NULL)
		{
		    SETDBERR(dberr, 0, W_DM9C50_DM2T_FIX_NOTFOUND);
		    break;
		}
		continue;
	    }
	}
	/* At this point we know that the base or master tcb is there.
	** tcb is either null, or a base table tcb, or a partition tcb.
	** If tcb is not null and the request is for an index/partition, we know
	** the base table (==tcb) or master (==base_tcb) is around.
	*/
	if (table_id->db_tab_index > 0)
	{
	    /* Simplify things a little if we're after an index */
	    base_tcb = tcb;
	}

	/*
	** If we are adding a secondary index/partition and the base tcb already
	** exists, then we have to obtain exclusive access to it in order
	** to execute the build.  After waiting for exclusive access, we
	** have to re-search to see if the same tcb's still do/don't exist.
	**
	** Checking tcb_ref_count appears silly and causes indefinite waits.
	** A query opens and searches an index (tcb_ref_count = 1) to discover 
	** the partition containing the indexed row, then enters here to open
	** the partition, and we're stuck.
	**
	** Instead, check if base_tcb is busy (being constructed or destructed).
	*/
	if ( table_id->db_tab_index != 0 &&
	     base_tcb &&
	     base_tcb->tcb_status & TCB_BUSY )
	{
	    /*
	    ** Register reason for waiting.  Done for debug/trace purposes.
	    */
	    base_tcb->tcb_status |= TCB_BUILD_WAIT;

	    /* Note that the wait call returns without the hash mutex. */
	    dm2t_wait_for_tcb(base_tcb, lock_id);
	    continue;
	}
	/* We have the base/master tcb, if there is any, and it's not refed. */

	/*
	** Allocate TCB.  If the memory cannot be allocated then attempt to
	** free up an existing TCB and retry the request.
	*/
	status = dm2t_alloc_tcb((DMP_RELATION *)NULL, (i4)loc_count,
			dcb, &tcb, dberr);

	if (status != E_DB_OK)
	{
	    dm0s_munlock(tcb_hm);
	    if (dberr->err_code != E_DM923D_DM0M_NOMORE)
		break;

	    /* Stop if its sort of obvious we're not making any progress */
	    if (retry++ > 10)
		break;

	    /*
	    ** If we get cannot allocate the memory, then try to reclaim
	    ** resources by freeing an unused TCB.
	    */
	    status = dm2t_reclaim_tcb(lock_id, log_id, dberr);
	    if (status != E_DB_OK)
		break;

	    continue;
	}

	/*
	** Fill in partial tcb fields with passed in logical information
	** and mark the TCB as PARTIAL.
	**
	** Note that without a DMP_RELATION, this data was unavailable
	** to dm2t_alloc_tcb(), so we have to supply it here.
	*/
	tcb->tcb_status = (TCB_PARTIAL | TCB_VALID);
	STRUCT_ASSIGN_MACRO(*table_id, tcb->tcb_rel.reltid);
	STRUCT_ASSIGN_MACRO(*table_name, tcb->tcb_rel.relid);
	STRUCT_ASSIGN_MACRO(*table_owner, tcb->tcb_rel.relowner);
	tcb->tcb_rel.relpgtype = page_type;
	tcb->tcb_rel.relpgsize = page_size;
	tcb->tcb_hash_bucket_ptr = tcb_h;
	tcb->tcb_hash_mutex_ptr = tcb_hm;

	/*
	** Compute blank-stripped lengths of table and owner names,
	** save those lengths in the TCB:
	*/
	for ( cptr = (char *)&tcb->tcb_rel.relid.db_tab_name + DB_TAB_MAXNAME - 1,
		     i = DB_TAB_MAXNAME;
	      cptr >= (char *)&tcb->tcb_rel.relid.db_tab_name && *cptr == ' ';
	      cptr--, i-- );
	tcb->tcb_relid_len = i;
	for ( cptr = (char *)&tcb->tcb_rel.relowner.db_own_name + DB_OWN_MAXNAME - 1,
		     i = DB_OWN_MAXNAME;
	      cptr >= (char *)&tcb->tcb_rel.relowner.db_own_name && *cptr == ' ';
	      cptr--, i-- );
	tcb->tcb_relowner_len = i;

	/*
	** Initialize the TCB mutexes
	** CS will truncate the name to CS_SEM_NAME_LEN
	** Since table name may be > CS_SEM_NAME_LEN, 
	** use reltid,reltidx to make the semaphore name unique,
	** plus as much of the table name as we can fit
	*/
	dm0s_minit(&tcb->tcb_mutex,
		      STprintf(sem_name, "TCB %x %x %*s", 
			tcb->tcb_rel.reltid.db_tab_base, 
			tcb->tcb_rel.reltid.db_tab_index,
			tcb->tcb_relid_len, tcb->tcb_rel.relid.db_tab_name));
	dm0s_minit(&tcb->tcb_et_mutex,
		      STprintf(sem_name, "TCB et %x %x %*s",
			tcb->tcb_rel.reltid.db_tab_base, 
			tcb->tcb_rel.reltid.db_tab_index,
			tcb->tcb_relid_len, tcb->tcb_rel.relid.db_tab_name));

	if (table_id->db_tab_index > 0)
	{
	    tcb->tcb_parent_tcb_ptr = base_tcb;
	    tcb->tcb_lkid_ptr = &base_tcb->tcb_lk_id;
	}
	else if (table_id->db_tab_index < 0)
	{
	    tcb->tcb_pmast_tcb = base_tcb;
	    tcb->tcb_lkid_ptr = &base_tcb->tcb_lk_id;
	}

	/*
	** Override initialized Table IO values.
	*/
	tcb->tcb_table_io.tbio_status = (TBIO_VALID | TBIO_PARTIAL_CB);
	tcb->tcb_table_io.tbio_page_type = tcb->tcb_rel.relpgtype;
	tcb->tcb_table_io.tbio_page_size = tcb->tcb_rel.relpgsize;
	tcb->tcb_table_io.tbio_cache_ix = DM_CACHE_IX(tcb->tcb_rel.relpgsize);
	tcb->tcb_table_io.tbio_reltid = tcb->tcb_rel.reltid;

	/*
	** set "sysrel" for tables needing physical locks.
	** testing for both TCB2_PHYSLOCK_CONCUR and DM_SCONCUR_MAX is
	** really redundant but leave the later test in case the user
	** didn't run upgradedb to create the TCB2 flag in their existing DBs
	**
	** However, exclude temp tables from sysrel.
	*/
	if ( (table_id->db_tab_base > 0 &&
	      table_id->db_tab_base <= DM_SCONCUR_MAX) ||
       	     tcb->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR )
	    tcb->tcb_table_io.tbio_sysrel = TRUE;
	else
	    tcb->tcb_table_io.tbio_sysrel = FALSE;

	/*
	** dm2t_alloc_tcb() has set the location array pointer and
	** initialized the array.
	**
	** If there are no locations specified
	** then no location array is allocated.  In this case this TCB will
	** not be useable for any IO actions (this type of tcb is used only
	** as a placeholder in the tcb cache and is expected to be tossed
	** eventually rather than used).
	*/

	/*
	** If this is a base table tcb, then set those fields specific
	** to base tables and put the tcb on the tcb hash queue.
	**
	** The tcb is marked busy during the process since the hcb mutex
	** must be released.
	*/
	if (table_id->db_tab_index <= 0)
	{
	    /*
	    ** Get TCB validation lock.
	    **
	    ** This lock is used if the database is open in multi-server mode
	    ** to recognize operations performed in other servers which
	    ** invalidate this TCB's contents.
	    **
	    ** The lock's value is stored in the TCB and is rechecked each time
	    ** the TCB is referenced.  If the stored lock value does not match
	    ** the actual value, then the TCB must be tossed and rebuilt.
	    **
	    ** The lock value is also used by the buffer manager to make sure
	    ** that only valid TCB's are used to toss pages from the data cache.
	    */
	    if (dcb->dcb_served == DCB_MULTIPLE)
	    {
		status = lock_tcb(tcb, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    tcb->tcb_status |= TCB_BUSY;

	    /*
	    ** Put the tcb on its hash queue.
	    */
	    tcb->tcb_q_next = tcb_h->hash_q_next;
	    tcb->tcb_q_prev = (DMP_TCB*) &tcb_h->hash_q_next;
	    tcb_h->hash_q_next->tcb_q_prev = tcb;
	    tcb_h->hash_q_next = tcb;

	    /*
	    ** Now that the TCB is marked BUSY, we can release the hash mutex
	    ** while we complete the work.
	    */
	    dm0s_munlock(tcb_hm);

	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount++;	/* keep track of TCBs allocated */
	    hcb->hcb_cache_tcbcount[tcb->tcb_table_io.tbio_cache_ix]++;
	    dm0s_munlock(&hcb->hcb_tq_mutex);

	    /*
	    ** If this database can be opened by a server using a different
	    ** buffer manager then we must check if the cached pages for this
	    ** table have been invalidated while we have not had a TCB lock
	    ** on it.
	    */
	    if (dcb->dcb_bm_served == DCB_MULTIPLE)
	    {
		status = dm0p_tblcache(dcb->dcb_id, table_id->db_tab_base,
		    DM0P_VERIFY, lock_id, log_id, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Call the buffer manager to set the tcb validation value for all
	    ** already-cached pages from this table.
	    */
	    if (table_id->db_tab_index == 0)
		dm0p_set_valid_stamp(dcb->dcb_id, table_id->db_tab_base,
			tcb->tcb_rel.relpgsize,
			tcb->tcb_table_io.tbio_cache_valid_stamp,
			lock_id);

	    /*
	    ** Reacquire mutex to complete build actions:
	    **    wake up any waiters and turn off the busy status.
	    */
	    dm0s_mlock(tcb_hm);

	    /*
	    ** Place the tcb on the free list since it is currently unfixed
	    ** (where theoretically it could soon be chosen as a victim to free)
	    ** As usual the TCB goes on the end of the list.
	    */
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    debug_tcb_free_list("on",tcb);
	    tcb->tcb_ftcb_list.q_next = hcb->hcb_ftcb_list.q_prev->q_next;
	    tcb->tcb_ftcb_list.q_prev = hcb->hcb_ftcb_list.q_prev;
	    hcb->hcb_ftcb_list.q_prev->q_next = &tcb->tcb_ftcb_list;
	    hcb->hcb_ftcb_list.q_prev = &tcb->tcb_ftcb_list;
	    dm0s_munlock(&hcb->hcb_tq_mutex);

	    /*
	    ** Build action complete.
	    ** Wake up any BUSY waiters and mark the tcb valid and ready to use.
	    */
	    dm2t_awaken_tcb(tcb, lock_id);
	    tcb->tcb_status &= ~(TCB_BUSY);
	}
	else
	{
	    /*
	    ** If the tcb being built is for a secondary index, then copy
	    ** relavent fields down from the base table tcb.
	    */
	    tcb->tcb_table_io.tbio_cache_valid_stamp =
				base_tcb->tcb_table_io.tbio_cache_valid_stamp;

	    /*
	    ** Link the new tcb onto the base table's index queue.
	    */
	    tcb->tcb_q_next = base_tcb->tcb_iq_next;
	    tcb->tcb_q_prev = (DMP_TCB*)&base_tcb->tcb_iq_next;
	    base_tcb->tcb_iq_next->tcb_q_prev = tcb;
	    base_tcb->tcb_iq_next = tcb;
	    base_tcb->tcb_index_count++;

	    /* Count another TCB opened in index's cache */
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_cache_tcbcount[tcb->tcb_table_io.tbio_cache_ix]++;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}

	dm0s_munlock(tcb_hm);

	/*
	** New TCB has been successfully allocated, initialized, and inserted
	** into the tcb cache.
	*/
	if (dberr->err_code == W_DM9C50_DM2T_FIX_NOTFOUND)
	    CLRDBERR(dberr);
	return (E_DB_OK);

    }

    /*
    ** Error handling - we assume here that the hash mutex has been released.
    **    Any tcb allocated in this routine must be deallocated.
    **    Any Busy waiters must be awakened.
    */

    /*
    ** If a base table TCB was allocated and is currently BUSY (meaning it
    ** is in the middle of being built), then release any lock resources
    ** it may have acquired and remove it from the hash queue.
    */
    if ((table_id->db_tab_index == 0) &&
	(tcb) &&
	(tcb->tcb_status & TCB_BUSY))
    {
	/*
	** Release the TCB validation lock if one was allocated.
	*/
	if ( dcb->dcb_served == DCB_MULTIPLE && tcb->tcb_lkid_ptr->lk_unique )
	{
	    local_status = unlock_tcb(tcb, &local_dberr);

	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &local_error, (i4)0);
	    }
	}

	dm0s_mlock(&hcb->hcb_tq_mutex);
	hcb->hcb_tcbcount--;	/* keep track of TCBs allocated */
	hcb->hcb_cache_tcbcount[tcb->tcb_table_io.tbio_cache_ix]--;
	dm0s_munlock(&hcb->hcb_tq_mutex);

	/* Remove from the TCB hash queue. */
	dm0s_mlock(tcb_hm);
	tcb->tcb_q_next->tcb_q_prev = tcb->tcb_q_prev;
	tcb->tcb_q_prev->tcb_q_next = tcb->tcb_q_next;

	/* Wake up any BUSY waiters. */
	dm2t_awaken_tcb(tcb, lock_id);
	dm0s_munlock(tcb_hm);
    }

    if (tcb)
    {
	/* TCB mutexes must be destroyed */
	dm0s_mrelease(&tcb->tcb_mutex);
	dm0s_mrelease(&tcb->tcb_et_mutex);
	dm0m_deallocate((DM_OBJECT **)&tcb);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C57_DM2T_INIT_TABIO);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2t_add_tabio_file 	- Add location to TABLE_IO.
**
** Description:
**	This routine adds a location to a Partial TCB in the table cache
**	so that pages on that location may be read/written.  This call
**	is used by recovery functions which need IO capability to tables but
**	cannot make full table open calls (dm2t_open).
**
**	Normally, the callers first make a dm2t_init_tabio_cb call to
**	build a shell TCB with no open locations and then call this routine
**	to open those locations which are needed.
**
**	There must be an existing TCB for the table in the cache and
**	this routine requires exclusive access to it.  If the TCB is
**	fixed, then this routine will wait for all fixers to release it.
**
**	The passed-in location information is copied into the tcb's tableio
**	structure and the table's file on that location is opened.
**
** Inputs:
**	dcb				Pointer to database control block
**	table_id			Pointer to table id
**	table_name			Pointer to table name
**	table_owner			Pointer to table owner name
**	location_cnf_id			Offset of location in config file
**	location_id			Offset of location in table's loc array
**	location_count			Number of locations in table
**	lock_id				Lock list id.
**	page_size			Page size for this table.
**
** Outputs:
**	err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	28-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	14-dec-1992 (rogerk)
**	    Fixes to partial TCB handling.  Invalidate tcb if its location
**	    count is less than the required one, not <=.
**	26-jul-1993 (bryanp)
**	    Fix error message parameter (loc name passed by value, not by ref).
**	20-jun-1994 (jnash)
**	    fsync project. Just a note that as dm2t_init_tabio_cb() is always
**	    called prior to this routine, this routine does not need to pass
**	    a sync-at-end flag to dm2t_fix_tcb().  If this changes
**	    then both table and database level synchronization flags
**	    may need to get propagated.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm2t_init_tabio_cb and dm2t_add_tabio_file
**		for managing a table's page size during recovery.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	29-May-1998 (shust01)
**	    In dm2t_add_tabio_file(), if the location that we are trying to
**	    add is already in the TABLE_IO cb, we should just return. What
**	    was happening was status was set to OK, we break out of for loop,
**	    and fall into error handler, which returns E_DM9C71. bug 91079.
*/
DB_STATUS
dm2t_add_tabio_file(
DMP_DCB		*dcb,
DB_TAB_ID	*table_id,
DB_TAB_NAME	*table_name,
DB_OWN_NAME	*table_owner,
i4		location_cnf_id,
i4		location_id,
i4		location_count,
i4		lock_id,
i4		log_id,
i4		page_type,
i4		page_size,
DB_ERROR	*dberr)
{
    DMP_TCB		*tcb = NULL;
    DMP_LOCATION	*loc_entry;
    i4		retry;
    i4		local_error;
    DB_STATUS		local_status;
    DB_STATUS		status = E_DB_OK;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#define MAX_RETRY 300

    for (retry = 0; retry < MAX_RETRY; retry++)
    {
	if (retry > 20)
	{
	    /*
	    ** Maybe the writebehind thread in the dmfrcp has this tcb fixed
	    ** Give the writebehind thread a chance to finish and unfix the tcb
	    */
#ifdef xDEBUG
	    TRdisplay("dm2t_add_tabio_file retry %d.. wait for tcb\n", retry);
#endif

	    CSsuspend(CS_TIMEOUT_MASK, 1, 0);
	}

	/*
	** Consistency Check:  Verify that the passed in location information
	** matches the known number of locations in the database and table.
	*/
	if ((location_id >= location_count) ||
	    (location_cnf_id >= dcb->dcb_ext->ext_count))
	{
	    uleFormat(NULL, E_DM9C5C_DM2T_ADD_TBIO_LOCERR, (CL_ERR_DESC *)NULL,
		ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		err_code, (i4)6,
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		sizeof(DB_TAB_NAME), table_name->db_tab_name,
		0, location_id, 0, location_count,
		0, location_cnf_id, 0, dcb->dcb_ext->ext_count);
	    SETDBERR(dberr, 0, E_DM9C58_DM2T_ADD_TABIO);
	    break;
	}

	/*
	** Lookup the existing TCB for the specified table.
	** If there is not currently a TCB for the table, then build one.
	*/
	status = dm2t_fix_tcb(dcb->dcb_id, table_id, (DB_TRAN_ID *)NULL, lock_id,
	    log_id, (i4)(DM2T_VERIFY | DM2T_NOBUILD), dcb,
	    &tcb, dberr);
	if (status != E_DB_OK)
	{
	    if (dberr->err_code != W_DM9C50_DM2T_FIX_NOTFOUND)
		break;

	    status = dm2t_init_tabio_cb(dcb, table_id, table_name, table_owner,
		location_count, lock_id, log_id, page_type, page_size, dberr);
	    if (status != E_DB_OK)
		break;

	    continue;
	}

	/*
	** If the location we are trying to add is already added to this
	** tableio_cb, then just return OK.
	*/
	if ((tcb->tcb_table_io.tbio_status & TBIO_OPEN) &&
	    (tcb->tcb_table_io.tbio_location_array[location_id].loc_status
								& LOC_OPEN))
	{
	    status = dm2t_unfix_tcb(&tcb, (i4)DM2T_VERIFY, lock_id,
				    log_id, dberr);
	    if (status != E_DB_OK)
		break;

	    return (E_DB_OK);
	}

	/*
	** We must have exclusive access to the TCB in order to add a new
	** location to it.  If this TCB is fixed by someone else, we have
	** to wait for it to become completely free and then retry our
	** fix operation.
	*/
	dm0s_mlock(tcb->tcb_hash_mutex_ptr);

	if (tcb->tcb_ref_count > 1)
	{
	    dm0s_munlock(tcb->tcb_hash_mutex_ptr);

	    status = dm2t_unfix_tcb(&tcb, (i4)DM2T_VERIFY, lock_id,
				    log_id, dberr);
	    if (status != E_DB_OK)
		break;

	    dm2t_wt_tabio_ptr(dcb->dcb_id, table_id, lock_id, DM2T_EXCLUSIVE);

	    continue;
	}

	/*
	** Check for dummy placeholder TCB.
	** If the discovered tcb has no locations, then it was created
	** merely as a placeholder rather than as a tcb that was intended
	** to be used.  In this case we toss out the dummy tcb and go back
	** so that we can allocate a real tcb.
	**
	** Consistency Check: Check for TCB which was not allocated with the
	** correct number of locations (so there is no spot into which to
	** build the currently requested location).  We log this unexpected
	** occurrence and then handle it like the dummy tcb case, tossing the
	** bad tcb and rebuilding it.
	*/
	if ((tcb->tcb_table_io.tbio_loc_count == 0) ||
	    (tcb->tcb_table_io.tbio_loc_count < location_count))
	{
	    dm0s_munlock(tcb->tcb_hash_mutex_ptr);

	    status = dm2t_unfix_tcb(&tcb, (i4)(DM2T_VERIFY | DM2T_TOSS),
		lock_id, log_id, dberr);
	    if (status != E_DB_OK)
		break;

	    continue;
	}

	/*
	** Mark TCB busy to prevent new people from fixing the TCB after we
	** release the hash mutex until after we have finished adding the
	** new location.
	*/
	tcb->tcb_status |= TCB_BUSY;
	dm0s_munlock(tcb->tcb_hash_mutex_ptr);

	/*
	** Fill in specified location entry with the passed in location
	** information.
	*/
	loc_entry = &tcb->tcb_table_io.tbio_location_array[location_id];
	loc_entry->loc_id = location_id;
	loc_entry->loc_config_id = location_cnf_id;
	loc_entry->loc_ext = &dcb->dcb_ext->ext_entry[location_cnf_id];
	STRUCT_ASSIGN_MACRO(dcb->dcb_ext->ext_entry[location_cnf_id].logical,
	      loc_entry->loc_name);
	loc_entry->loc_status = LOC_VALID;
	if (dcb->dcb_ext->ext_entry[location_cnf_id].flags & DCB_RAW)
	   loc_entry->loc_status |= LOC_RAW;

	/*
	** Open the new physical file which holds the portion of this table
	** on the new location.
	*/
	status = open_tabio_cb(&tcb->tcb_table_io, lock_id, log_id,
	    (i4)DM2F_OPEN, dcb->dcb_sync_flag, dberr);
	if (status != E_DB_OK)
	     break;

	/*
	** Wake up anybody who is waiting on our Build action and release
	** the BUSY status.
	*/
	dm0s_mlock(tcb->tcb_hash_mutex_ptr);
	dm2t_awaken_tcb(tcb, lock_id);
	tcb->tcb_status &= ~(TCB_BUSY);
	dm0s_munlock(tcb->tcb_hash_mutex_ptr);

	/*
	** Unfix our handle to the tcb.
	*/
	status = dm2t_unfix_tcb(&tcb, (i4)DM2T_VERIFY, lock_id, log_id, dberr);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*
    ** Error handling: we assume that the hash mutex is not held.
    **     The requested tcb may be currently fixed and if so we must
    **     unfix it.
    */

    /*
    ** If we exceeded the retry count, then log this as an error.
    ** Note that it is possible that highly concurrent activity will cause
    ** us to retry our attempts to get exclusive access to the needed tcb
    ** a few times before being successfull.  We pick a retry count that
    ** we think is unlikely to be reached unless something is really wrong
    ** with the system.
    */
    if (retry >= MAX_RETRY)
	SETDBERR(dberr, 0, E_DM9C5E_DM2T_ADD_RETRY_CNT);

    /*
    ** If we have the tcb fixed, unfix it.
    ** If it is currently marked busy then we must have been in the midst
    ** of updating it.  We need not clean up any location information which
    ** we have filled in as DM2F manages the occurrence of half-opened
    ** locations itself.
    */
    if (tcb != NULL)
    {
	if (tcb->tcb_status & TCB_BUSY)
	{
	    dm0s_mlock(tcb->tcb_hash_mutex_ptr);
	    dm2t_awaken_tcb(tcb, lock_id);
	    tcb->tcb_status &= ~(TCB_BUSY);
	    dm0s_munlock(tcb->tcb_hash_mutex_ptr);
	}
	local_status = dm2t_unfix_tcb(&tcb, (i4)DM2T_VERIFY, lock_id,
	    				log_id, &local_dberr);
	if (local_status != E_DB_OK)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C58_DM2T_ADD_TABIO);
    }

    uleFormat(NULL, E_DM9C71_DM2T_ADD_TBIO_INFO, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)7,
	sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
	sizeof(DB_TAB_NAME), table_name->db_tab_name,
	sizeof(DB_OWN_NAME), table_owner->db_own_name,
	0, location_id, 0, location_cnf_id, 0, location_count,
	sizeof(DB_LOC_NAME),
	    dcb->dcb_ext->ext_entry[location_cnf_id].logical.db_loc_name);

    return (E_DB_ERROR);
}

/*{
** Name: dm2t_wt_tabio_ptr -	Wait for access to a TABLE_IO control block.
**
** Description:
**	Waits until the specified type of access can be granted to a
**	Table IO control block.
**
**	Used by callers for one of the following reasons:
**
**	    mode == 0 : Access is needed to a Table IO control block, but the
**		TCB is currently BUSY.
**	    mode == DM2T_EXCLUSIVE : Exclusive access is needed to a Table IO
**		control block, but the TCB is currently fixed by other threads.
**
**	If the caller is waiting for exclusive access, then it will return
**	when no other sessions have the table fixed.  Otherwise it will
**	return when the table no longer indicates the TCB_BUSY status.
**
**	When waiting for exclusive access to a TCB, this routine will be
**	awakened each time another session unfixes the table.  The routine
**	will not return (it will instead re-arm the wait) until no sessions
**	have the table fixed.
**
** Inputs:
**	db_id				Database ID
**	table_id			Pointer to table id
**	mode				Request Mode:
**					    Zero - wait for TCB to be not busy
**					    DM2T_EXCLUSIVE - wait for TCB to
**						be not referenced
**	lock_id				The lock list id
**
** Outputs:
**	None
**
**      Returns:
**          VOID
**
** History:
**	28-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	30-Mar-2009 (jonj)
**	    SD 135242:
**	    If partition and not busy, check if the partition's 
**	    master TCB is busy, wait for it to become unbusy
**	    (introduced by change 491612 for clusters).
**	    Don't loop back to locate_tcb() unless mode
**	    is DM2T_EXCLUSIVE.
*/
VOID
dm2t_wt_tabio_ptr(
i4		db_id,
DB_TAB_ID	*table_id,
i4		mode,
i4		lock_id)
{
    DMP_HASH_ENTRY	*h;
    DM_MUTEX		*hash_mutex;
    DMP_TCB		*tcb;

    for (;;)
    {
	tcb = NULL;

	/* locate_tcb() will return with TCB's hash_mutex locked */
	hash_mutex = NULL;
	locate_tcb(db_id, table_id, &tcb, &h, &hash_mutex);

	if (tcb != NULL)
	{
	    if ((tcb->tcb_status & TCB_BUSY) ||
		((tcb->tcb_ref_count) && (mode == DM2T_EXCLUSIVE)))
	    {
		if (mode == DM2T_EXCLUSIVE)
		    tcb->tcb_status |= TCB_XACCESS_WAIT;
		/* dm2t_wait_for_tcb() will release TCB's hash_mutex */
		dm2t_wait_for_tcb(tcb, lock_id);
	    }
	    else if ( tcb->tcb_rel.relstat2 & TCB2_PARTITION &&
	       	      tcb->tcb_pmast_tcb->tcb_status & TCB_BUSY )
	    {
		/* dm2t_wait_for_tcb() will release TCB's hash_mutex */
	        dm2t_wait_for_tcb(tcb->tcb_pmast_tcb, lock_id);
	    }
	    if ( mode != DM2T_EXCLUSIVE )
	       return;
	}

	dm0s_munlock(hash_mutex);
	break;
    }

    return;
}

/*{
** Name: locate_tcb - 	Locate a tcb
**
** Description:
**
** 	This call looks up a base table tcb by scanning the tcb hash queue.
**	The given ID can be for a regular base table, a partitioned master,
**	or a partition table.
**	It should not be called to look up a secondary index tcb.
**	If the passed in table ID is for a secondary index, we'll return
**	the base table TCB if it's there.
**
**	The normal call is with *hash_mutex null initially, meaning that the
**	caller holds none of the hash mutexes.  In a few (rare) cases the caller
**	may already have the appropriate hash mutex (for a partition, e.g., and
**	looking for the master).  In that case pass the hash mutex address in
**	with *hash_mutex.  Notice that the way the mutex hashing works, any
**	hash chains that any partition could hash to are all locked by the
**	same (base/master) hash mutex.  Thus we only need one hash mutex
**	at a time to deal with masters and partitions.
**
**	Upon return, the mutex pointed to by *hash_mutex is held (tcb found
**	or not).
**
** Inputs:
**	db_id				Database ID
**	table_id			Pointer to table id
**	hash_mutex			Points to hash-mutex already owned, or NULL
**
** Outputs:
**	tcb				Pointer to Table Control Block
**	h				Pointer to Hash Entry
**	hash_mutex			Pointer to hash mutex owned
**
**      Returns:
**          VOID
**
** History:
**	28-oct-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Instead of the caller acquiring the hcb_mutex prior to calling
**	    this function, we acquire the correct hash_mutex and
**	    return with it locked.
**	    Added **hash to locate_tcb() prototype into which the pointer to
**	    the hash_entry will be returned.
**	15-Jan-2004 (schka24)
**	    Separate hash chain and mutex, return both.
**	25-Feb-2005 (schka24)
**	    Oops.  If caller wants a secondary index, make sure we locate
**	    the base table tcb, not some random partition!
**      30-jun-2005 (horda03) Bug 114498/INGSRV3299
**          If the required TCB flagged TCB_BEING_RELEASED, then re-do the
**          scan.
**      11-sep-2006 (horda03) Bug 116613
**          TCB_BEING_RELEASED is now handled else where, because of the way things
**          work, the session releasing the TCB could be the session trying to locate
**          the TCB.
*/
static VOID
locate_tcb(
i4		db_id,
DB_TAB_ID	*table_id,
DMP_TCB		**tcb,
DMP_HASH_ENTRY  **hash,
DM_MUTEX	**hash_mutex)
{
    DMP_TCB		*t;
    DMP_HCB		*hcb;
    DMP_HASH_ENTRY	*h;
    DM_MUTEX		*hm;
    i4			tab_base, tab_index;

    hcb = dmf_svcb->svcb_hcb_ptr;
    tab_base = table_id->db_tab_base;
    tab_index = table_id->db_tab_index;

    /*
    ** Hash database id and base table id to get tcb hash bucket.
    */
    hm = &hcb->hcb_mutex_array[
			HCB_MUTEX_HASH_MACRO(db_id,tab_base)];
    *hash = h = &hcb->hcb_hash_array[
	    HCB_HASH_MACRO(db_id,tab_base,tab_index)];

    /* Lock the hash entry */
    if (hm != *hash_mutex)
    {
	/* TEMP diagnostic */
	if (*hash_mutex != NULL)
	    TRdisplay("%@(dm2t)locate_tcb: caller holding %x, tabid (%d,%d) hash %x\n",
			*hash_mutex, tab_base, tab_index, hm);
	dm0s_mlock(hm);
    }
    *hash_mutex = hm;

    /*
    ** Look down the tcb list chained off of this bucket for a tcb matching
    ** the requested base table id.  If caller wants a secondary index,
    ** find the base table instead.
    */
    if (tab_index > 0)
	tab_index = 0;
    for (t = h->hash_q_next; t != (DMP_TCB*) h;)
    {
	if ((t->tcb_dcb_ptr->dcb_id == db_id) &&
	    (t->tcb_rel.reltid.db_tab_base == tab_base) &&
	    (t->tcb_rel.reltid.db_tab_index == tab_index))
	{
	    *tcb = t;
	    return;
	}

        t = t->tcb_q_next;
    }

    /* TCB not found */

    *tcb = NULL;
#if 0
    TRdisplay("dm2t(locate_tcb): NOT-FOUND in bucket %d is table (%d,%d)\n",
		(db_id + table_id->db_tab_base) % hcb->hcb_buckets_hash,
		db_id, table_id->db_tab_base);
#endif

    return;
}

/*{
** Name: build_tcb	- Build a tcb
**
** Description:
**      This routine is builds a TCB for a table decribed by a table
**	identifier.  The table identifier is used to access the system
**      tables that contain information about a table.  Ths TCB is linked
**	into the TCB hash array.  It is left unfixed, but the hash mutex
**	is held when build-tcb returns (error or not).
**
**	While the TCB is being built, a dummy tcb is placed on the tcb
**	hash queues so that other threads looking for the same tcb will
**	not also try to build one.  The dummy tcb has status BUSY which
**	will cause other threads to wait for this build to complete.  When
**	the real tcb is finished it is put in the hash queue in place of
**	where the dummy tcb was and all waiters are reawakened.  Those
**	threads should always research the hash queue to find the real tcb.
**
**	The table cache always links together secondary index tcbs with
**	their parent tcbs.  When build_tcb is called to build a base table
**	it will also build tcbs for any secondary indexes of that table.
**
**	With certain exceptions (tables which are actually VIEWs or GATEWAYS
**	to foreign data files), we check at this time to ensure that the
**	underlying physical Ingres file for this table actually exists. If
**	any such file does NOT exist, even if it is a secondary index file,
**	the TCB is not built and an error is returned to the caller. These
**	situations reflect SEVERE internal inconsistencies and typically
**	result in the database being marked inconsistent.
**
**	Another such exception is a "show only" TCB.  When all we need is
**	table information for a DMT_SHOW, it's allowable to build a TCB that
**	doesn't have files open.  Typically this would occur because the
**	necessary page-size cache isn't available.  Please note that we
**	always build a normal TCB if at all possible;  a show-only TCB is
**	the exception.  It's possible to have a show-only secondary index
**	TCB hung off a regular base TCB, but not the reverse (ie if the base
**	TCB is show-only, we mark all index TCB's the same way, mostly just
**	because it seems safer this way).
**
**	If the table is a partition, it's master should already be around.
**	We'll need to fill in the master's PP array entry for our new TCB.
**	If the table is a partitioned master, we need to inspect (but not open)
**	all the partitions to build the PP array.  A significant part of the
**	rationale for this is that the master needs the row/page counts, and
**	these are stored partition by partition.  (So that modify of a
**	partition can maintain them properly.)
**
**	For partitions, if the master is a show-only TCB, the partition TCB
**	is marked that way too.
**
** Inputs:
**      dcb                             Pointer to Database Control Block
**      table_id                        Pointer to Table Id of table to build
**	tran_id				Pointer to transaction id
**	lock_id				Lock list identifier
**	log_id				Log file identifier
**      sync_flag                       One of:
**					    0 - Perform synchronous i/o
**					    DM2T_CLSYNC - Perform
**					       asynch i/o, synch on dm2t_close
**				
**      flags                           Same as passed in to dm2t_fix_tcb.
**
**
** Outputs:
**	build_tcb			The newly-built TCB.
**	err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** History:
**	31-dec-1985 (derek)
**          Created new for jupiter.
**      25-nov-87 (jennifer)
**          Added support for multiple locations.
**	07-oct-1988 (rogerk)
**	    Initialize tcb_sysrel field.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	21-oct-88 (teg)
**	    added logic to detect that tcb is being built for show and
**	    avoid generating an error when dm2f_build_fcb fails because of
**	    a physical file not found.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-feb-89 (rogerk)
**	    Added new build algorithm.  Put dummy tcb onto hash queue with
**	    status BUSY so other threads will find it and wait for the build
**	    to complete.  When finished with build, check for waiters and
**	    wake them up.
**	15-aug-89 (rogerk)
**	    Added support for Gateways.  Added new table attribute fields that
**	    are filled in when the tcb is built.  These attributes determine
**	    actions to be taken (or not taken) during processing on this table:
**		tcb_logging	- log updates to the table
**		tcb_logical_log	- log logical records for updates to the table
**		tcb_nofile	- table has no underlying physical file
**		tcb_update_idx	- secondary indexes should be updated along with
**				  the base table.
**		tcb_no_tids	- table does not support tid access.
**	    Don't look for physical file when opening VIEW or GATEWAY tables.
**	    Verify that gateway is present when try to open gateway table.
**	25-jan-1990 (fred)
**	    Added support for extension tables for (initially) large object
**	    support.  This is managed by keeping a list of extension tables
**	    hanging off the tcb.  At this point, the support necessary in this
**	    routine is to initialize the tcb_et_mutex semaphore, and to build an
**	    empty list of extension tables.
**	18-apr-1990 (fred)
**	    Add support for setting special TCB flags for cache locking
**	    associated with system catalogs or peripheral datatype support
**	    tables.
**	2-may-90 (bryanp)
**	    Disabled the code which allowed the building of TCB's without
**	    associated FCB's. This code was an attempt to allow limited use of
**	    tables for which one or more of the underlying physical files did
**	    not exist or could not be opened. However, (a) clients of DMF did
**	    not correctly check for this type of table, and (b) other parts of
**	    DMF assumed that if the TCB existed, it was correct and contained
**	    full FCB information. The only thing you can do with a table
**	    without underlying physical files is to delete it with verifyDB.
**	    VIEW && GATEWAY tables are handled through a different mechanism.
**	11-sep-1990 (jonb)
**	    Integrate ingres6301 change 280136 (rogerk):
**            Copy tcb_valid_stamp into the secondary index tcb's so that we
**            don't have to trace back to the parent tcb to store validation
**            stamps in fault_page.
**	12-sep-1990 (jonb)
**	    Integrate ingres6301 change 280141 (rogerk):
**            After building a TCB, call dm0p_set_valid_stamp to mark all
**            previously cached buffers as belonging to this table.
**	17-dec-1990 (jas)
**	    Smart Disk project integration:  Find out the Smart Disk type,
**	    if any, for the table and store the type in the tcb.
**	 4-feb-1991 (rogerk)
**	    Added support for fixed table priorities.
**	    Added tcb_bmpriority field.  Call buffer manager to get table
**	    cache priority when tcb is built.
**	25-feb-1991 (rogerk)
**	    Changed dm0p_gtbl_priority call to take regular DB_TAB_ID.
**	 7-mar-1991 (bryanp)
**	    Changed the DMP_TCB to support Btree index compression, and added
**	    support here: Added new field tcb_data_atts, which is analogous to
**	    tcb_key_atts but lists ALL attributes. These two attribute pointer
**	    lists are used for compressing and uncompressing data & keys.
**	    Added new field tcb_comp_katts_count to the TCB and added code here
**	    to compute the field's value. This field is used by index compresion
**	    to determine the worst-case overhead which compression might add to
**	    an index entry.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    17-aug-1989 (Derek)
**		Changed hard-coded attribute-key number to compile-time
**		constants that are different for normal and B1 systems.
**		Added security fields to tcb (tcb_sec_lbloff,tcb_sec_keyoff).
**	    14-jun-1991 (Derek)
**		Moved code that reads system catalogs into TCB into a new
**		routine (read_tcb).  This new routine also allocates the tcb
**		structure.
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	    25-jun-1991 (Derek)
**		Add support for HEAP tables that are described as having
**		keys.  Changed keybuilding code to not attempt to build
**		a key descriptor for such a table (and to not error).
**		This was done to allow PATCH to HEAP operations on structured
**		tables without having to change their key descriptions.
**          26-may-92 (schang)
**              GW merge 11-mar-91 (linda)
**	        Added code to set t->tcb_no_tids to FALSE for RMS Gateway, still
**	        defaults to TRUE for other NR gateways.  This is part of tid
**	        support for the RMS Gateway.
**	26-aug-1992 (25-jan-1990) (fred)
**	    Blob support:
**	    Added support for extension tables for (initially) large object
**	    support.  This is managed by keeping a list of extension tables
**	    hanging off the tcb.  Allocate space for extension list in the TCB.
**	26-aug-1992 (rogerk)
**	    Rewritten as part of Reduced logging project:
**	      - Removed showflag parameter.
**	      - Changed to read iirelation tuple for each table with the
**		indicated base table id and then call read_tcb to read the
**		rest of the system catalog information and fill in the tcbs.
**	      - Added open_tabio_cb to fill in the new tableio control block
**		and open any physical files.
**	      - Added lock_tcb call to do cache locking.
**	      - Added build_index_info call to link secondary index tcb's to
**		the base table's index list.
**	09-nov-1992 (kwatts)
**	    Added a call to dm1sd_disktype for all TCBs including
**	    those with no file (VIEW and GATEWAY). In these cases it will
**	    set TCB to have no Smart Disk. This change was originally in
**	    read_tcb but is moved with rogerk rewrite.
**	14-dec-1992 (rogerk)
**	    Put back checks for iirelation<-->iiindex information mismatches
**	    during rollforward operations.  It is possible during rollforward
**	    for a table to not be listed as indexed but for there to exist
**	    iirelation rows for an index table that is in the process of being
**	    created.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	15-may-1993 (rmuth)
**	    Add support for building concurrent indicies. Build_tcb will
**	    ignore these indicies as there are not exposed to the outside
**	    world until "modify to noreadonly" is issued
**	24-may-1993 (bryanp)
**	    Don't use dmd_check messages as traceback messages.
**	    Log table name and database name when failing to build a TCB.
**	26-jul-1993 (rogerk)
**	    Changed warning given when relloccount found to be zero to only
**	    be printed if the table is not a view.
**	23-aug-1993 (rogerk)
**	    Changed decision on whether to bypass gateway check to be based
**	    on DCB_S_RECOVER mode rather than DCB_S_ONLINE_RCP.
**	10-oct-93 (swm)
**	    Bug #56440
**	    Assign unique identifier (for use as an event value) to new
**	    tcb_unique_id field.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	20-jun-1994 (jnash)
**	    Add flag param, used to pass along sync-at-end info.  In this
**	    cause its just passed along to read_tcb.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Add the flags argument which has same value as the
**              dm2t_fix_tcb flag argument. Use the DM2T_RECOVERY option to
**              stop code opening the underlying files as they may not exist
**	 6-sep-1995 (nick)
**	    Correct logic in above change by AndyW - this had broken
**	    concurrent index creation. #70556
**	11-jan-1995 (dougb)
**	    Remove debugging code which should not exist in generic files.
**	    (Replace printf() calls with TRdisplay().)
**	06-mar-1996 (stial01 for bryanp)
**	    If relpgsize is 0, set relpgsize to DM_PG_SIZE.
**      06-mar-1996 (stial01)
**          Variable Page Size:
**          Deny access if there are no buffers for page size of this table
**          Deny access if the table width is > than the maximum tuple width
**          currently allowed for the installation 'dmf_maxtuplen'.
**	17-may-96 (stephenb)
**	    Add replication code, check for replication and fill in some
**	    usefull TCB info for later use.
**	 4-jul-96 (nick)
**	    We were locking iirelation with X mode page locks rather than S
**	    leading to deadlock problems ; caused by the use in OpenIngres
**	    of the rcb_k_mode flag ( which was ignored in 6.4 ).
**	14-aug-96 (stephenb)
**	    Check wether replication is currently enabled for this table
**	    using the rules_created field in dd_regist_tables.
**	23-aug-96 (nick)
**	    Mark tcb_status if we find inconsistent secondary indicies when
**	    building the base table's TCB. #77429
**	28-feb-97 (cohmi01)
**	    Keep track in hcb of tcbs created. If reach hcb_tcblimit threshold,
**	    reclaim TCB as if dm0m_allocate failed. for bugs 80242, 80050.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	    Added *hash parm to prototype in which caller passes the
**	    hash bucket for this TCB with it's hash_mutex held.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for dummy TCB.
**	    We'll initialize only those fields we need.
**	23-jan-97 (stephenb)
**	    Check for replicator catalogs using rep_catalog() before
**	    adding replicator info to TCB, we don't replicate replicator
**	    catalogs.
**	3-jun-97 (stephenb)
**	    If a column is replicated, then set it's replication key sequence
**	    in the DB_ATTS.
**	29-jul-1997 (wonst02)
**	    If dcb access is "read", open table for read-only access.
**	14-nov-97 (stephenb)
**	    Add log_id parameter to dm2t_open() calls to avoid logging 
**	    errors.
**	15-may-1998 (nanpr01)
**	    Parallel Index code needs to tolerate certains inconsistencies
**	    during its operation. But at the end, the core catalogs will 
**	    be consistent.
**	15-Jan-2004 (schka24)
**	    Add hash-mutex parameter; deal with partitions.
**	6-Feb-2004 (schka24)
**	    Generate tcb unique-id safely.
**	17-feb-04 (inkdo01)
**	    Added missing braces to error checking code.
**	01-Mar-2004 (jenjo02)
**	    When opening a Partition, copy relatts, relkeys values
**	    from Master TCB prior to TCB memory allocation.
**	5-Apr-2004 (schka24)
**	    Ignore partitions that have invalid partition numbers instead
**	    of erroring on them.  They are probably new partitions created
**	    for an in-progress modify.
**	27-May-2004 (jenjo02)
**	    Cache partition information if encountered before
**	    Master's iirelation, obviating the need for two
**	    passes through the iirelation table. Two passes were
**	    also being done for indexes, which was a boo-boo.
**	    Count the number of indexes and partitions as they
**	    are encountered; when we have them all, quit the
**	    iirelation gets.
**	15-Mar-2005 (schka24)
**	    Implement show-only TCB's.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	12-Jul-2006 (kschendel)
**	    Cut back on stack partition cache to reduce stack usage.
**	08-Oct-2008 (jonj)
**	    Keep note of which partitions have been found, not how
**	    many. In the case of modify from, say n to n-1 partitions,
**	    Master's iirelation.relnparts will be n-1 but we need to
**	    find all "n" partitions to avoid holes in Master's tcb_pp_array.
**	    (bug 121032).
**	03-Dec-2008 (jonj)
**	    Relocate misplaced CLRDBERR().
**	17-Dec-2008 (jonj)
**	    B121349, PMASK_SIZE incorrectly defined, fix.
**	05-Jun-2009 (thaju02)
**	    Pass lock_id to dm0p_set_valid_stamp(). (B118454)
**	26-Oct-2009 (wanfr01) b122797
**	    Correctly handle non-replicated tables in a replicated db.
**	    These tables are not supposed to exist in dd_regist_tables
**	    Fix SEGV if you break before tcb_list has been modified.
**	27-Oct-2009 (kschendel) SIR 122739
**	    Ignore SI iirelation entries with relkeys == 0.  We're probably
**	    in rollforward, and a new index table has been created but not
**	    yet INDEX'ed.  The dm2t-open of the base table to do the re-index
**	    can find the newly created index entry;  since there's no key
**	    or iiindex data for it yet, attempting to build the tcb for the
**	    index may fall over (and would be pointless anyway).
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change the short-lived '10.0' relcmptlvl to DMF_T10_VERSION.
**	    De-support ancient (6.0) T0 version tables, if hashed or
**	    compressed.
**	10-may-2010 (stephenb)
**	    reltups and relpages are now i8 on disk (and therefore in the
**	    DMP_RELATION structure, use new CSadjust_i8counter to peform
**	    atomic counter adjustment. Also, restrict actual values to
**	    MAXI4 until we can handle larger numbers.
*/

static DB_STATUS
build_tcb(
DMP_DCB		*dcb,
DB_TAB_ID	*table_id,
DB_TRAN_ID      *tran_id,
DMP_HASH_ENTRY  *hash,
DM_MUTEX	*hash_mutex,
i4         lock_id,
i4         log_id,
i4	  	sync_flag,
i4		flags,
DMP_TCB		**build_tcb,
DB_ERROR	*dberr)
{
    i4			opening_partition;
    DMP_HCB		*hcb = dmf_svcb->svcb_hcb_ptr;
    DMP_TCB		*base_tcb = 0;
    DMP_TCB		*dummy_tcb = 0;
    DMP_TCB		*master_tcb;	/* Pointer to partitioned master TCB */
    DMP_TCB		*tcb_list = 0;
    DMP_TCB		*tcb = 0;
    DMP_TCB		*tcb_ptr = 0;
    DMP_TCB		*rep_tcb = 0;
    DMP_RCB		*rel_rcb = 0;
    DMP_RCB		*rep_rcb = 0;
    DMP_RCB		*rep_rcb_idx = 0;
    DMP_RCB		*rep_rcb_col = 0;
    DMP_REP_INFO	*rep_info = 0;
    DMP_RELATION	relation;
    DM2R_KEY_DESC	key_desc[1];
    DM_TID		tid;
    DMT_PHYS_PART	*pp_ptr;	/* PP array entry for partition */
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4		local_error;
    i4		reclaimed;
    STATUS		stat;
    i4             open_tabio_flags;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	key[2];
    char		*reg_col_rec;
    DM_TID		rep_tid;
    i4			i, j;
    DM2R_KEY_DESC	rep_col_key;
    STATUS		cl_stat;
    char		*rep_rec;
    char		*rep_idx_rec;
    bool		parallel_idx = FALSE;
    bool		build_showonly;
    i4			indexes_found = 0;
    
    /*
    ** Only used locally by build_tcb to
    ** cache partition info until Master's iirelation
    ** is encountered. Caching lets us get away
    ** with a single pass through the iirelation hash.
    */
    typedef struct
    {
	i4		db_tab_index;
	DM_TID		tid;
	i4		reltups;
	i4		relpages;
    } PCACHE;

#define	PCACHE_MAX	20	/* Arbitrary, keep this smallish */
#define PCACHE_INITIAL_ME 500	/* If we have to allocate, start here */
    PCACHE		local_pcache[PCACHE_MAX];
    PCACHE		*lcache = &local_pcache[0];
    PCACHE		*pcache = lcache;
    u_i2		pcache_max = PCACHE_MAX;
    u_i2		new_max;
    char		*new_pcache;

/* One bit for every possible partition */
#define PMASK_SIZE	(65536 / BITS_IN(i4)) * sizeof(i4)
    u_i4		*partitions_found = NULL;
    u_i4		*partitions_needed = NULL;
#define	SETPARTNO(m, p) \
	((m)[(p) >> 5] |= (1U << ((p) & (BITS_IN(i4)-1))))

    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
	dmd_check(E_DM9320_DM2T_BUILD_TCB);
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_BUILD_TCB  (%~t,%d,%d)\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name,
	table_id->db_tab_base, table_id->db_tab_index);
#endif

    dmf_svcb->svcb_stat.tbl_build++;

    opening_partition = (table_id->db_tab_index & DB_TAB_PARTITION_MASK);

    for (;;)
    {
	/*
	** Allocate a dummy TCB, initialize it as though it represents the
	** base table we are building, and mark it as being busy.  This must be
	** done before releasing hash mutex so that other sessions looking for
	** this TCB will wait for this build to complete.  We must use a dummy
	** TCB since we cannot predict the size of the needed TCB structure yet.
	**
	** We'll soon discard this dummy TCB, so get ShortTerm memory.
	*/
	if (hcb->hcb_tcbcount < hcb->hcb_tcblimit)
	{
	    status = dm0m_allocate((i4)(sizeof(DMP_TCB)),
		(i4)(DM0M_ZERO | DM0M_SHORTTERM), (i4)TCB_CB, (i4)TCB_ASCII_ID, 
		(char *)dcb, (DM_OBJECT **)&dummy_tcb, dberr);
	}
	else
	{
	    /* TCB limit has been reached, handle as if dm0m_allocate failed */
	    status  = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
	}
	
	if (status != E_DB_OK)
	{
	    dm0s_munlock(hash_mutex);
	    if (dberr->err_code != E_DM923D_DM0M_NOMORE)
		break;

	    /*
	    ** If we get cannot allocate the memory, then try to reclaim
	    ** resources by freeing an unused TCB.  We return a RETRY
	    ** warning to the caller since another thread may have built
	    ** the same TCB while we were searching for resources.
	    */
	    status = dm2t_reclaim_tcb(lock_id, log_id, dberr);
	    if (status != E_DB_OK)
		break;

	    dm0s_mlock(hash_mutex);
	    return (E_DB_WARN);
	}

	/* Init dummy TCB fields */
	STRUCT_ASSIGN_MACRO(*table_id, dummy_tcb->tcb_rel.reltid);
	dummy_tcb->tcb_dcb_ptr = dcb;
	dummy_tcb->tcb_ref_count = 0;
	dummy_tcb->tcb_status = TCB_BUSY;
	dummy_tcb->tcb_unique_id = CSadjust_counter((i4 *)&dmf_svcb->svcb_tcb_id,1);

	/* Always add dummy tcb for base table (b116591) */
	if (dummy_tcb->tcb_rel.reltid.db_tab_index > 0)
	    dummy_tcb->tcb_rel.reltid.db_tab_index = 0;

	/* Put dummy tcb on hash queue */
	dummy_tcb->tcb_q_next = hash->hash_q_next;
	dummy_tcb->tcb_q_prev = (DMP_TCB*) &hash->hash_q_next;
	hash->hash_q_next->tcb_q_prev = dummy_tcb;
	hash->hash_q_next = dummy_tcb;

	/* this was missing: */
	dummy_tcb->tcb_hash_bucket_ptr = hash;
	dummy_tcb->tcb_hash_mutex_ptr = hash_mutex;

	/*
	** Now that the dummy tcb is in the TCB list we can release the
	** hash array semaphore.
	*/
	dm0s_munlock(hash_mutex);

	/*
	** Get iirelation rcb and position it to return rows for the table
	** we are trying to open.  (Note that this will also return rows for
	** any secondary indexes and partitions as well.)
	*/
	status = dm2r_rcb_allocate(dcb->dcb_rel_tcb_ptr,
	     (DMP_RCB *)NULL, tran_id, lock_id, log_id,
	     &rel_rcb, dberr);
	if (status != E_DB_OK)
	    break;

	rel_rcb->rcb_lk_mode = RCB_K_IS;
	rel_rcb->rcb_k_mode = RCB_K_IS;
	rel_rcb->rcb_access_mode = RCB_A_READ;

	if (opening_partition)
	{
	    /* For a partition, find the master, get the physical partition
	    ** array entry.  It holds the iirelation tid.
	    */
	    status = find_pp_for_partition(dcb, table_id,
			&master_tcb, &pp_ptr);
	    if (status != E_DB_OK)
	    {
		uleFormat(dberr, E_DM93F9_DM2T_MISSING_PP, NULL, ULE_LOG, 
				NULL, NULL, 0, 0, err_code,
				2, 0, table_id->db_tab_base,
				0, table_id->db_tab_index);
		break;
	    }
	}
	else
	{
	    /* It's not a partition, need to build key for positioning
	    ** iirelation.  We'll get all rows that have the same
	    ** db-tab-base: indexes, partitions, everything.
	    */

	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_RELATION_KEY;
	    key_desc[0].attr_value = (char *) &table_id->db_tab_base;

	    /*
	    ** Position the relation table.
	    */
	    status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc,
		 (i4)1,
		 (DM_TID *)NULL, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Loop through iirelation rows that describe this base table and
	** its secondary indexes.  For the base/master table and indexes,
	** build a Table Control Block.  For partitions, just make a physical
	** partition array entry.
	**
	** When the loop completes the base table tcb should be pointed to
	** by 'base_tcb' and its secondary indexes (if any) should be linked
	** together on the 'tcb_list'.
	**
	** If we're doing a partition, this "loop" just gets the one partition
	** row, by tid.
	*/

	do
	{
	    build_showonly = FALSE;
	    if (opening_partition)
	    {
		i8 row_delta, page_delta;

		status = dm2r_get(rel_rcb, (DM_TID *) &pp_ptr->pp_iirel_tid, DM2R_BYTID,
			(char *) &relation, dberr);
		if (status != E_DB_OK)
		    break;
		/* Had better be at the right row! */
		if (relation.reltid.db_tab_base != table_id->db_tab_base
		  || relation.reltid.db_tab_index != table_id->db_tab_index)
		{
		    TRdisplay("%@(dm2t)build_tcb expected (%d,%d) at %d, got (%d,%d)\n",
			table_id->db_tab_base, table_id->db_tab_index,
			pp_ptr->pp_iirel_tid.tid_i4,
			relation.reltid.db_tab_base, relation.reltid.db_tab_index);
		    /* ****FIXME better error code here */
		    SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);
		    status = E_DB_ERROR;
		    break;
		}
		/* Since this is a second, more recent look at the iirelation
		** row for the partition, take this opportunity to grab the
		** latest page/row counts for this partition.  They will
		** be the same as what's in the PP array, unless we're
		** running multiple servers!
		*/
		/* restrict reltups and relpages to MAXI4 */
		row_delta = (master_tcb->tcb_rel.reltups + 
			relation.reltups - pp_ptr->pp_reltups) > MAXI4 ?
				0 : relation.reltups - pp_ptr->pp_reltups;
		page_delta = (master_tcb->tcb_rel.relpages +
			relation.relpages - pp_ptr->pp_relpages) > MAXI4 ?
				0 :relation.relpages - pp_ptr->pp_relpages;
		if (row_delta != 0 || page_delta != 0)
		{
		    pp_ptr->pp_reltups = (i4)relation.reltups;
		    pp_ptr->pp_relpages = (i4)relation.relpages;
		    /* The master's base counts are always the sum of the
		    ** pp-array counts, so adjust them.  (without bothering
		    ** to mutex the master...)
		    */
		    CSadjust_i8counter(&master_tcb->tcb_rel.reltups, row_delta);
		    CSadjust_i8counter(&master_tcb->tcb_rel.relpages, page_delta);
		}
	    }
	    else
	    {
		/* Not a partition - read next row, check it out */

		status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT,
			(char *)&relation, dberr);
		if (status != E_DB_OK)
		{
		    /*
		    ** A NONEXT error is the expected completion of this read
		    ** loop.
		    */
		    if (dberr->err_code == E_DM0055_NONEXT)
			status = E_DB_OK;
		    break;
		}

		/*
		** Skip rows not describing this table which just happen to be
		** on the same page.
		*/
		if (relation.reltid.db_tab_base != table_id->db_tab_base)
		    continue;

		/* Skip secondary index entries with zero relkeys.  That
		** is an index that has been created, but not yet made into
		** a real index.  We can find such things during rollforward
		** (ie we're replayed the index CREATE, and we're doing the
		** index INDEX, and re-index is trying to open the base
		** table.)
		*/
		if (relation.reltid.db_tab_index > 0 && relation.relkeys == 0)
		    continue;

		/*
		** As long as this is not a recovery of the table
		** operation then skip rows which describe concurrent indicies.
		** This is done to stop users seeing the index until
		** it has finished being built, no X lock is held on the
		** base table.
		**
		** Recovery needs to know about these indicies as "point-in-time"
		** recovery may be required to recover the index as well.
		*/
		if (( flags & DM2T_RECOVERY ) == 0)
		{
		    if ((relation.relstat & TCB_INDEX)
						&&
			(relation.relstat2 & TCB2_CONCURRENT_ACCESS))
		    {
			continue;
		    }
		    if ((relation.relstat & TCB_INDEX)
						&&
			(relation.relstat2 & TCB2_PARALLEL_IDX))
		      parallel_idx = TRUE;
		}
	    }

	    /* We have the iirelation record, do something with it */

	    /* Fix old style relcmptlvl for 10.0 development databases
	    ** created after relcmptlvl was bumped but before we revised
	    ** it to be a number.  Do this now before it gets into the TCB.
	    */
	    if (relation.relcmptlvl == DMF_T10x_VERSION)
		relation.relcmptlvl = DMF_T10_VERSION;

	    /*
	    ** Variable Page Sizes: Make sure the installation is
	    ** configured with this page size, otherwise deny access
	    ** to this table.
	    */
	    if (!(dm0p_has_buffers(relation.relpgsize)))
	    {
		/* If fastload..., allocate page array */
		if (dmf_svcb->svcb_status & SVCB_SINGLEUSER)
		    status = dm0p_alloc_pgarray(relation.relpgsize, dberr);
		else if (flags & DM2T_SHOWONLY)
		    build_showonly = TRUE;
		else
		    status = E_DB_ERROR;

		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM0157_NO_BMCACHE_BUFFERS);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /* No longer supported:  T0 or T1 tables that are compressed (they
	    ** used an old style of compression).  T0 tables that are hashed
	    ** (they used an old hash algorithm).
	    */
	    if ( ( (relation.relcmptlvl == DMF_T0_VERSION
		    || relation.relcmptlvl == DMF_T1_VERSION)
		  && relation.relcomptype != TCB_C_NONE)
	      || (relation.relcmptlvl == DMF_T0_VERSION && relation.relspec == TCB_HASH) )
	    {
		if (flags & DM2T_SHOWONLY)
		    build_showonly = TRUE;
		else
		{
		    uleFormat(NULL, E_DM905F_OLD_TABLE_NOTSUPP, NULL, ULE_LOG,
			NULL, NULL, 0, NULL, &local_error, 2,
			sizeof(relation.relid), &relation.relid,
			sizeof(relation.relowner), &relation.relowner);
		    SETDBERR(dberr, 0, E_DM008F_ERROR_OPENING_TABLE);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Check if this table can be opened by this server.  Allow the
	    ** RCP to open any tables it wants to.
	    */
	    if ((relation.relstat & TCB_GATEWAY) &&
		(! dmf_svcb->svcb_gateway) &&
		((dcb->dcb_status & DCB_S_RECOVER) == 0))
	    {
		SETDBERR(dberr, 0, E_DM0135_NO_GATEWAY);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** For tables which have no locations (views) set the relloccount
	    ** value to 1 here so that the allocation and setting of the
	    ** location array will be correct.  We currently initialize a
	    ** location entry even for tables with no underlying files.
	    */
	    if (((relation.relstat & TCB_MULTIPLE_LOC) == 0) &&
		(relation.relloccount != 1))
	    {
#ifdef xDEBUG
		/*
		** Consistency check: Long ago in a galaxy far, far away
		** we did not have multi-location tables and the relloccount
		** field was not initialized.  Check for any left around.
		*/
		if ( ! (relation.relstat & (TCB_GATEWAY | TCB_VIEW)))
		{
		    TRdisplay("Warning: iirelation row for single location\n");
		    TRdisplay("  table (%t) has an invalid relloccount field\n",
			sizeof(relation.relid), relation.relid.db_tab_name);
		    TRdisplay("  Setting relloccount value to 1.\n");
		}
#endif
		relation.relloccount = 1;
	    }

	    /*
	    ** Until database conversion is in place, all tables created before
	    ** the multiple page size project have a 0 in relpgsize. Treat
	    ** this as meaning DM_PG_SIZE.
	    */
	    if (relation.relpgsize == 0)
		relation.relpgsize = DM_PG_SIZE;

	    /* *** FIXME FIXME FIXME
	    ** Remove this after the iirelation conversion in dm2d is
	    ** activated.  It will ensure that old tables don't accidently
	    ** present "clustered".  For now, clustered is NOTIMP.
	    */
	    relation.relstat &= ~TCB_CLUSTERED;

	    if (! opening_partition)
	    {
		/* Opening a regular table, or a master... */
		/* If we're looking at an iirelation row for a partition, just
		** stash it's info into the master TCB pp-array and continue.
		** If master hasn't been encountered yet, cache the
		** partition info for later.
		**
		** Don't build TCB's for the partitions yet.
		*/
		if (relation.relstat2 & TCB2_PARTITION)
		{
		    if ( !partitions_found )
		    {
			partitions_found = (u_i4*)MEreqmem(0, 
					PMASK_SIZE,
					TRUE, 
					&cl_stat);
			if ( cl_stat )
			{
			    status  = E_DB_ERROR;
			    SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
			    break;
			}
		    }
		    /* Validate the partition number.  If the relation row
		    ** is for a bogus looking partition, just skip it:
		    ** most likely we're in a modify, and we're looking at
		    ** what will become a future partition of this table
		    ** (but it isn't one yet!), or it may be a partition
		    ** that has been dropped but not yet cleaned up by qeu.
		    */
		    if ( base_tcb )
		    {
			if ( relation.relnparts >= base_tcb->tcb_rel.relnparts )
			    continue;
			pp_ptr = &base_tcb->tcb_pp_array[relation.relnparts];
			STRUCT_ASSIGN_MACRO(relation.reltid, pp_ptr->pp_tabid);
			/* Expedient, but wrong...
			** will need fixed for tid8 here.
			*/
			pp_ptr->pp_iirel_tid.tid_i4 = tid.tid_i4;
			pp_ptr->pp_reltups = relation.reltups;
			pp_ptr->pp_relpages = relation.relpages;
		    }
		    else
		    {
			/*
			** Master not encountered yet, we'll have
			** to cache the partition info
			*/
			if ( relation.relnparts >= pcache_max )
			{
			    /* then we need a larger cache */

			    /* Get half again as much */
			    if (pcache == lcache)
				new_max = PCACHE_INITIAL_ME;
			    else
				new_max = pcache_max + pcache_max / 2;

			    if ( new_max < relation.relnparts )
			    {
				new_max = relation.relnparts
				    + relation.relnparts / 2;
			    }

			    /* Perhaps this should be a ST dm0m alloc? */
			    new_pcache = MEreqmem(0,
				    new_max * sizeof(PCACHE),
				    FALSE, &cl_stat);

			    if ( cl_stat )
			    {
				status  = E_DB_ERROR;
				SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
				break;
			    }

			    /* Copy old cache to new */
			    MEcopy((char*)pcache, 
				    pcache_max * sizeof(PCACHE),
				    new_pcache);

			    /* Discard old cache */
			    if ( pcache != lcache )
				MEfree((char*)pcache);

			    pcache_max = new_max;
			    pcache = (PCACHE*)new_pcache;
			}

			/* Cache the partition info */
			pcache[relation.relnparts].db_tab_index =  
				relation.reltid.db_tab_index;
			pcache[relation.relnparts].tid.tid_i4 = 
				tid.tid_i4;
			pcache[relation.relnparts].reltups = 
				relation.reltups;
			pcache[relation.relnparts].relpages = 
				relation.relpages;
		    }

		    /* Note this partition found */
		    SETPARTNO(partitions_found, relation.relnparts);

		    /* Whirl around for another iirelation row */
		    continue;
		}
	    }
	    else
	    {
		/*
		** Opening a partition.
		** To ensure that its TCB memory is sufficiently
		** allocated, set relatts, relkeys to Master's.
		*/
		relation.relatts = master_tcb->tcb_rel.relatts;
		relation.relkeys = master_tcb->tcb_rel.relkeys;
	    }

	    /*
	    ** Allocate TCB block for this table.  If can't allocate the
	    ** memory then try freeing resources and retrying.
	    */

	    for (reclaimed = 0; ; reclaimed++)
	    {
		status = dm2t_alloc_tcb(&relation, relation.relloccount,
			dcb, &tcb, dberr);

		if ((status == E_DB_OK) || (dberr->err_code != E_DM923D_DM0M_NOMORE))
		    break;

		/* Only retry once. */
		if (reclaimed > 0)
		    break;

		status = dm2t_reclaim_tcb(lock_id, log_id, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    if (status != E_DB_OK)
		break;

	    /* Set TCB hash pointers that alloc-tcb can't. */
	    tcb->tcb_hash_bucket_ptr = hash;
	    tcb->tcb_hash_mutex_ptr = hash_mutex;
	    if (opening_partition)
	    {
		tcb->tcb_pmast_tcb = master_tcb;
		tcb->tcb_lkid_ptr = &master_tcb->tcb_lk_id;
		/* Expedient, but ultimately wrong: will need fixed... */
		/* problem is DB_TID vs DM_TID, not assignable */
		tcb->tcb_iirel_tid.tid_i4 = pp_ptr->pp_iirel_tid.tid_i4;
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(tid, tcb->tcb_iirel_tid);
	    }
	    if (build_showonly)
		tcb->tcb_status |= TCB_SHOWONLY;

	    /*
	    ** Initialize and Fill in TCB with system catalog information.
	    ** For partitions, this only reads location info, not attribute
	    ** info (because there isn't any!).
	    */
	    status = read_tcb(tcb, &relation, dcb, tran_id, lock_id, log_id,
		sync_flag, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Store the iirelation LSN to TCB for stale TCB for online ckp */
	    STRUCT_ASSIGN_MACRO(*(DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(
		rel_rcb->rcb_tcb_ptr->tcb_rel.relpgtype, 
		(DMPP_PAGE *)rel_rcb->rcb_data.page)), tcb->tcb_iirel_lsn); 
 
	    if (opening_partition)
	    {
		copy_from_master(tcb, master_tcb);
		if (master_tcb->tcb_status & TCB_SHOWONLY)
		    tcb->tcb_status |= TCB_SHOWONLY;
	    }
	    /* If index, and we've found the base table, and it's a
	    ** show-only TCB, mark the index the same way.  This is just
	    ** an optimization, we may have to close files for indexes
	    ** found before the base table (see build-index-info).
	    */
	    if ( (relation.relstat & TCB_INDEX) && base_tcb != NULL
	      && (base_tcb->tcb_status & TCB_SHOWONLY) )
		tcb->tcb_status |= TCB_SHOWONLY;

	    /*
	    ** Open table's physical files.
	    */
	    if ( (! tcb->tcb_nofile) && (tcb->tcb_status & TCB_SHOWONLY) == 0 )
	    {
                /*
                ** If the TCB is being built for recovery then do not
                ** open the underlying files as they may not exist
                */
                if ( flags & DM2T_RECOVERY )
                    open_tabio_flags = DM2F_ALLOC;
                else
                    open_tabio_flags = (dcb->dcb_access_mode == DCB_A_READ) ? 
		    	DM2F_OPEN | DM2F_READONLY : 
		    	DM2F_OPEN;

                status = open_tabio_cb(&tcb->tcb_table_io, lock_id, log_id,
                    open_tabio_flags, dcb->dcb_sync_flag, dberr);
                if (status != E_DB_OK)
                    break;
	    }

	    /*
	    ** Link this TCB to our build list according to its table type.
	    */
	    if (relation.relstat & TCB_INDEX)
	    {
		tcb->tcb_q_next = tcb_list;
		tcb_list = tcb;
		/* Count another index for base table */
		indexes_found++;
	    }
	    else
	    {
		/* Must be the master/base/partition table */
		base_tcb = tcb;

		/* Make bit map of partitions needed */
		if ( base_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED )
		{
		    partitions_needed = (u_i4*)MEreqmem(0, 
				    PMASK_SIZE,
				    TRUE, 
				    &cl_stat);
		    if ( cl_stat )
		    {
			status  = E_DB_ERROR;
			SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
			break;
		    }
		    
		    for ( i = 0; i < base_tcb->tcb_rel.relnparts; i++ )
			SETPARTNO(partitions_needed, i);
		}
	    }

	    /*
	    ** Zero tcb pointer so error handling code below won't try to
	    ** deallocate the same cb twice.
	    */
	    tcb = 0;

	    /*
	    ** If opening anything other than a partition and
	    ** the base table has been constructed and all of
	    ** its indexes and partitions found, then there's
	    ** no point in continuing the 'gets' - we're done.
	    */
	    if ( !opening_partition && base_tcb &&
		 base_tcb->tcb_rel.relidxcount == indexes_found &&
		 (!(base_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED) ||
		     ( partitions_found && partitions_needed &&
		         MEcmp((PTR)partitions_found, (PTR)partitions_needed,
		     		PMASK_SIZE) == 0)) )
	    {
		break;
	    }

	    /* Just one time thru if doing a partition */
	} while (! opening_partition);

	/* Free partition masks, if any */
	if ( partitions_needed )
	{
	    MEfree((PTR)partitions_needed);
	    partitions_needed = NULL;
	}
	if ( partitions_found )
	{
	    MEfree((PTR)partitions_found);
	    partitions_found = NULL;
	}

	if (status != E_DB_OK)
	    break;

	/*
	** Release iirelation RCB
	*/
	status = dm2r_release_rcb(&rel_rcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Finished Read iirelation Loop: the base table tcb should be pointed
	** to by 'base_tcb' and its secondary indexes (if any) should be linked
	** together on the 'tcb_list'.
	*/

	/*
	** Make sure a base table row was found.
	*/
	if (base_tcb == NULL)
	{
	    SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);
	    status = E_DB_ERROR;
	    break;
	}

	/* If opening Master, fill in cached partition info, if any */
	if ( base_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED )
	{
	    /*
	    ** Depending on the order in which the base table
	    ** and partitions iirelations appeared, some of the
	    ** partition information may already be filled into
	    ** the pp_array (db_tab_index != 0) (the base table
	    ** was constructed before the partitions were 
	    ** encountered), or some or all of the partitions
	    ** were found before the base table, in which case
	    ** those partitions are cached (pp_array db_tab_index
	    ** == 0).
	    **
	    ** Fill in the blanks, if any, from the cache,
	    ** and sum reltups/relpages.
	    */
	    base_tcb->tcb_rel.reltups = 0;
	    base_tcb->tcb_rel.relpages = 0;

	    for ( i = 0; i < base_tcb->tcb_rel.relnparts; i++ )
	    {
		pp_ptr = &base_tcb->tcb_pp_array[i];

		/* If not filled in, it better be in cache... */
		if ( pp_ptr->pp_tabid.db_tab_index == 0 )
		{
		    pp_ptr->pp_tabid.db_tab_base = base_tcb->tcb_rel.reltid.db_tab_base;
		    pp_ptr->pp_tabid.db_tab_index = pcache[i].db_tab_index;
		    pp_ptr->pp_iirel_tid.tid_i4 = pcache[i].tid.tid_i4;
		    pp_ptr->pp_reltups = pcache[i].reltups;
		    pp_ptr->pp_relpages = pcache[i].relpages;
		}
		/*
		** The Master reltups/pages is always the sum
		** of the pp-array reltups/pages.
		*/
		CSadjust_i8counter(&base_tcb->tcb_rel.reltups, pp_ptr->pp_reltups);
		CSadjust_i8counter(&base_tcb->tcb_rel.relpages, pp_ptr->pp_relpages);
	    }
	    /* restrict reltups and relpages to i4 */
	    if (base_tcb->tcb_rel.relpages > MAXI4)
		base_tcb->tcb_rel.relpages = MAXI4;
	    if (base_tcb->tcb_rel.relpages > MAXI4)
		base_tcb->tcb_rel.relpages = MAXI4;
	    /* Free partition cache */
	    if ( pcache != lcache )
		MEfree((char*)pcache);
	    pcache = lcache;
	}

	/*
	** Get TCB validation lock.
	**
	** This lock is used if the database is open in multi-server mode
	** to recognize operations performed in other servers which invalidate
	** this TCB's contents.
	**
	** The lock's value is stored in the TCB and is rechecked each time
	** the TCB is referenced.  If the stored lock value does not match the
	** actual value, then the TCB must be tossed and rebuilt.
	**
	** The lock value is also used by the buffer manager to make sure that
	** only valid TCB's are used to toss pages from the data cache.
	*/
	if (dcb->dcb_served == DCB_MULTIPLE)
	{
	    status = lock_tcb(base_tcb, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Build Secondary Index information in the TCB and link
	** the secondary index TCB's onto the base table.
	**
	** The build_index_info routine expects the tcb lock value
	** to have already been placed into the base table tcb.
	*/
	if (base_tcb->tcb_rel.relstat & TCB_IDXD)
	{
	    status = build_index_info(base_tcb, &tcb_list, tran_id, lock_id,
		log_id, parallel_idx, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else if (tcb_list)
	{
	    /*
	    ** It is possible during some rollforward situations to have
	    ** to open a table while its index information is not yet
	    ** rolled forward and to get an index<->iirelation mismatch.
	    **
	    ** This occurs because the CREATE operation which creates the
	    ** secondary index table is rolled forward before the INDEX
	    ** operation which update the base table index information.
	    **
	    ** If we are in rollforward the excess index tcbs are silently
	    ** deallocated.
	    */
            if ((base_tcb->tcb_dcb_ptr->dcb_status & DCB_S_ROLLFORWARD) ||
		(parallel_idx))
	    {
		while (tcb_list)
		{
		    tcb_ptr = tcb_list;
		    tcb_list = tcb_ptr->tcb_q_next;
		    if (tcb_ptr->tcb_table_io.tbio_status & TBIO_OPEN)
		    {
			local_status = dm2f_release_fcb(lock_id, log_id,
			    tcb_ptr->tcb_table_io.tbio_location_array,
			    tcb_ptr->tcb_table_io.tbio_loc_count, 0,
			    &local_dberr);
		    }
		    /* TCB mutexes must be destroyed */
		    dm0s_mrelease(&tcb_ptr->tcb_mutex);
		    dm0s_mrelease(&tcb_ptr->tcb_et_mutex);
		    dm0m_deallocate((DM_OBJECT **)&tcb_ptr);
		}
	    }
	    else
	    {
		/* XXXX Add better error message */
		SETDBERR(dberr, 0, E_DM9331_DM2T_TBL_IDX_MISMATCH);
		status = E_DB_ERROR;
		break;
	    }
	}
	/*
	** check table for replication and fill in replication flag.
	** Don't do this for the dd_regist_tables catalog or index or
	** dd_regist_collumns, since we use dm2t_open in the code and get
	** into an infinite calling loop on dm2t_open(). Implication is that we
	** cannot replicate dd_regist_tables or it's index or dd_regist_columns.
	** We also exclude all tables marked as system catalogs.
	** Some of the replicator frontent tools will use dmt_show() whilst
	** building replicator catalog info, at this stage the replicator
	** catalogs are inconsistent. dmt_show() will call build_tcb() and
	** this causes problems here because of the catalog inconsistencies.
	** We must basically ban the replication of all replicator catalogs
	** to avoid an inconsistent state.
	*/
	base_tcb->tcb_replicate = TCB_NOREP;
	base_tcb->tcb_rep_info = NULL;
	if ((dcb->dcb_status & DCB_S_REPLICATE) &&
	    !(base_tcb->tcb_rel.relstat & TCB_CATALOG) &&
	    (base_tcb->tcb_status & TCB_SHOWONLY) == 0 &&
	    (rep_catalog(base_tcb->tcb_rel.relid.db_tab_name) == FALSE))
	{
	    /*
	    ** open the dd_regist_tables catalog and index
	    */
	    status = dm2t_open(dcb, &dcb->rep_regist_tab, DM2T_IS,
		DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)10, 0, log_id,
		lock_id, (i4)0,(i4)0, 0, tran_id, &timestamp,
		&rep_rcb, (DML_SCB *)NULL, dberr);
	    if (status != E_DB_OK)
		break;
	    status = dm2t_open(dcb, &dcb->rep_regist_tab_idx, DM2T_IS,
		DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)10, 0, log_id,
		lock_id, (i4)0,(i4)0, 0, tran_id, &timestamp,
		&rep_rcb_idx, (DML_SCB *)NULL, dberr);
	    if (status != E_DB_OK)
		break;
	    /*
	    ** now position and check for a row for the current table
	    */
	    key[0].attr_operator = DM2R_EQ;
	    key[0].attr_number = 1;
	    key[0].attr_value = base_tcb->tcb_rel.relid.db_tab_name;
	    key[1].attr_operator = DM2R_EQ;
	    key[1].attr_number = 2;
	    key[1].attr_value = base_tcb->tcb_rel.relowner.db_own_name;
	    status = dm2r_position(rep_rcb_idx, DM2R_QUAL, key, (i4)2,
		(DM_TID *)NULL, dberr);
	    if ((status != E_DB_OK) && (*err_code != E_DM0055_NONEXT))
		break;

	    if (status == E_DB_OK)
	    {
		rep_idx_rec = MEreqmem(0, rep_rcb_idx->rcb_tcb_ptr->tcb_rel.relwid,
		TRUE, &cl_stat);
		if (rep_idx_rec == NULL || cl_stat != OK)
	        {
		    SETDBERR(dberr, 0, E_DM9555_REP_NOMEM);
            	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
                    	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                    	err_code, (i4)0);
            	    status = E_DB_ERROR;
            	    break;
		}
	    }

	    status = dm2r_get(rep_rcb_idx, &rep_tid, DM2R_GETNEXT,
		rep_idx_rec, dberr);
	    if (status != E_DB_OK)
	    {
		rep_info = NULL;
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    /* no record for table, do not replicate */
		    base_tcb->tcb_replicate = TCB_NOREP;
		    status = E_DB_OK;
		}
		else
		    break;
	    }
	    else /* replicate and add rep info to TCB,
		 ** also find replicated columns */
	    {
		int		tmp_size;
		rep_tcb = rep_rcb->rcb_tcb_ptr;
		rep_rec = MEreqmem(0, rep_tcb->tcb_rel.relwid, TRUE, &cl_stat);
	    	if (rep_rec == NULL || cl_stat != OK)
	    	{
            	    uleFormat(NULL, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
                        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                        err_code, (i4)0);
            	    status = E_DB_ERROR;
            	    break;
	    	}
		MEcopy(rep_idx_rec + rep_rcb_idx->rcb_tcb_ptr->tcb_rel.relwid
		    - sizeof(DM_TID), sizeof(DM_TID), (char *)&rep_tid);
		status = dm2r_get(rep_rcb, &rep_tid, DM2R_BYTID,
		    rep_rec, dberr);
		if (status != E_DB_OK)
		    break;
		/*
		** check rules_created field to see if replication is
		** currently enabled (this is a hangover from the old
		** architecture)
		*/
		if (*(rep_rec + rep_tcb->tcb_atts_ptr[6].offset) != 0 &&
		    *(rep_rec + rep_tcb->tcb_atts_ptr[6].offset) != ' ')
		{
	    	    rep_info = (DMP_REP_INFO *)MEreqmem(0,
			sizeof(DMP_REP_INFO), TRUE, &cl_stat);
	            if (rep_info == NULL || cl_stat != OK)
	    	    {
            	        uleFormat(NULL, E_DM9555_REP_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
                            (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
			    (i4 *)NULL, err_code, (i4)0);
            	        status = E_DB_ERROR;
            	        break;
	    	    }
		    /*
		    ** fill in rep_info from record, not very nice but we have
		    ** to do it this way to avoid bus allignment problems
		    */
		    tmp_size = sizeof(rep_info->dd_reg_tables.cdds_no);
		    MEcopy(rep_rec, sizeof(rep_info->dd_reg_tables.tabno),
		        (char *)&rep_info->dd_reg_tables.tabno);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[2].offset,
			DB_TAB_MAXNAME, rep_info->dd_reg_tables.tab_name);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[3].offset,
			DB_OWN_MAXNAME, rep_info->dd_reg_tables.tab_owner);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[4].offset,
		        sizeof(rep_info->dd_reg_tables.reg_date),
		        rep_info->dd_reg_tables.reg_date);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[5].offset,
		        sizeof(rep_info->dd_reg_tables.sup_obs_cre_date),
		        rep_info->dd_reg_tables.sup_obs_cre_date);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[6].offset,
		        sizeof(rep_info->dd_reg_tables.rules_cre_date),
		        rep_info->dd_reg_tables.rules_cre_date);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[7].offset,
		        sizeof(rep_info->dd_reg_tables.cdds_no),
		        (char *)&rep_info->dd_reg_tables.cdds_no);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[8].offset,
			DB_TAB_MAXNAME, rep_info->dd_reg_tables.cdds_lookup_table);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[9].offset,
		    	DB_TAB_MAXNAME, rep_info->dd_reg_tables.prio_lookup_table);
		    MEcopy(rep_rec + rep_tcb->tcb_atts_ptr[10].offset,
			DB_TAB_MAXNAME, rep_info->dd_reg_tables.index_used);
		    /*
		    ** Open dd_regist_columns and get rows for this table
		    ** then set the replicate field in tcb atts
		    */
	    	    status = dm2t_open(dcb, &dcb->rep_regist_col, DM2T_IS,
		        DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)10, 0,
			log_id, lock_id, (i4)0,(i4)0, 0, tran_id,
			&timestamp, &rep_rcb_col, (DML_SCB *)NULL, dberr);
	    	    if (status != E_DB_OK)
		        break;
	    	    rep_col_key.attr_operator = DM2R_EQ;
	    	    rep_col_key.attr_number = 1;
	    	    rep_col_key.attr_value =
			(char *)&rep_info->dd_reg_tables.tabno;
		    reg_col_rec = (char *)MEreqmem(0,
			rep_rcb_col->rcb_tcb_ptr->tcb_rel.relwid, TRUE,
			&cl_stat);
	    	    status = dm2r_position(rep_rcb_col, DM2R_QUAL,
			&rep_col_key, (i4)1,
			(DM_TID *)NULL, dberr);
	    	    if (status != E_DB_OK)
		        break;
		    for (i = 0;;i++)
		    {
		        status = dm2r_get(rep_rcb_col, &tid, DM2R_GETNEXT,
		    	    reg_col_rec, dberr);
		        if (status != E_DB_OK)
		        {
			    if (dberr->err_code == E_DM0055_NONEXT && i != 0)
			    {
			        /* out of records */
			        status = E_DB_OK;
		    	        break;
			    }
			    else
			        break;
		        }
		        /* nasty!! we use byte offset here to indicate the
			** collumn sequence field, we can't easily use a
			** structure because of bus alignment (record is
			** compacted)
		        */
		        if (*((i4 *)(reg_col_rec +
			    rep_rcb_col->rcb_tcb_ptr->tcb_atts_ptr[3].offset))
			    == 0)
			    /* col is not replicated */
			    continue;
		        for(j = 1; j <= base_tcb->tcb_rel.relatts; j++)
			{
			    DB_ATT_NAME tmpattnm;

			    MEmove(base_tcb->tcb_atts_ptr[j].attnmlen,
				base_tcb->tcb_atts_ptr[j].attnmstr, ' ',
				DB_ATT_MAXNAME, tmpattnm.db_att_name);

			    if (!MEcmp(tmpattnm.db_att_name, reg_col_rec + 4, 
					DB_ATT_MAXNAME))
				break;
			}
		        if (j > base_tcb->tcb_rel.relatts)
		        {
			    /* no attribute found */
        		    uleFormat(dberr, E_DM9552_REP_NO_ATTRIBUTE, 
				(CL_ERR_DESC *)NULL, ULE_LOG,
            		        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
			        (i4 *)NULL, err_code, (i4)2, 
				DB_ATT_MAXNAME, reg_col_rec + 2,
				DB_TAB_MAXNAME, base_tcb->tcb_rel.relid);
			    status = E_DB_ERROR;
			    break;
		        }
		        base_tcb->tcb_atts_ptr[j].replicated = TRUE;
			/*
			** add replication key
			*/
			MEcopy(reg_col_rec + 
			    rep_rcb_col->rcb_tcb_ptr->tcb_atts_ptr[4].offset,
			    4, &base_tcb->tcb_atts_ptr[j].rep_key_seq);
		    }
		    if (status != E_DB_OK)
		        break;
		    /*
		    ** close dd_regist_columns table
		    */
		    status = dm2t_close(rep_rcb_col, (i4)0, dberr);
		    if (status != E_DB_OK)
		        break;

		    base_tcb->tcb_replicate = TCB_REPLICATE;
	    	    base_tcb->tcb_rep_info = rep_info;
		}
	    }
	    /*
	    ** close dd_regist_tables and index
	    */
	    status = dm2t_close(rep_rcb, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	    status = dm2t_close(rep_rcb_idx, (i4)0, dberr);
	    if (status != E_DB_OK)
		break;
	}

	if ( (! opening_partition) && (base_tcb->tcb_status & TCB_SHOWONLY) == 0)
	{
	    /* Multi-cache validation already done for master, don't
	    ** repeat for partition
	    */
	    /*
	    ** If this database can be opened by a server using a different buffer
	    ** manager then we must check if the cached pages for this table have
	    ** been invalidated while we have not had a TCB lock on it.
	    */
	    if (dcb->dcb_bm_served == DCB_MULTIPLE)
	    {
		status = dm0p_tblcache(dcb->dcb_id, table_id->db_tab_base,
		    DM0P_VERIFY, lock_id, log_id, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Call the buffer manager to set the tcb validation value for all
	    ** already-cached pages from this table.
	    */
	    dm0p_set_valid_stamp(dcb->dcb_id, table_id->db_tab_base,
		base_tcb->tcb_rel.relpgsize,
		base_tcb->tcb_table_io.tbio_cache_valid_stamp,
		lock_id);
	}

	/*
	** Link the new TCB into the tcb hash array in place of where we put
	** the dummy tcb entry.  Wake up any sessions waiting for the build
	** to complete and deallocate the dummy TCB.
	** Skip all the cache stuff if show-only.
	*/
	if ((base_tcb->tcb_status & TCB_SHOWONLY) == 0)
	{
	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount++;	/* keep track of TCBs allocated */
	    hcb->hcb_cache_tcbcount[base_tcb->tcb_table_io.tbio_cache_ix]++;
	    /*
	    ** mark TCB status if there are inconsistent secondaries,
	    ** also increment cache open TCB count for indexes.
	    ** Note that index can be showonly even if base table isn't.
	    */
	    for (tcb_ptr = base_tcb->tcb_iq_next;
		 tcb_ptr != (DMP_TCB*) &base_tcb->tcb_iq_next;
		 tcb_ptr = tcb_ptr->tcb_q_next)
	    {
		if ((tcb_ptr->tcb_status & TCB_SHOWONLY) == 0)
		    hcb->hcb_cache_tcbcount[tcb_ptr->tcb_table_io.tbio_cache_ix]++;
		if (tcb_ptr->tcb_rel.relstat2 &
		    (TCB2_LOGICAL_INCONSISTENT | TCB2_PHYS_INCONSISTENT))
		{
		    base_tcb->tcb_status |= TCB_INCONS_IDX;
		}
	    }
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	
	dm0s_mlock(hash_mutex);
	dummy_tcb->tcb_q_next->tcb_q_prev = base_tcb;
	dummy_tcb->tcb_q_prev->tcb_q_next = base_tcb;
	base_tcb->tcb_q_next = dummy_tcb->tcb_q_next;
	base_tcb->tcb_q_prev = dummy_tcb->tcb_q_prev;

	if (dummy_tcb->tcb_status & TCB_WAIT)
	    dm0s_erelease(lock_id, DM0S_TCBWAIT_EVENT,
		dummy_tcb->tcb_unique_id);
	dm0m_deallocate((DM_OBJECT **)&dummy_tcb);

	/*
	** Link the new TCB into the free tcb queue since it is not
	** currently fixed.  Note that it is likely that the caller of this
	** routine is fixing the tcb and will be soon taking the TCB back
	** off of the free queue.  We'll put the TCB at the end of the list.
	*/
	dm0s_mlock(&hcb->hcb_tq_mutex);
	debug_tcb_free_list("on",base_tcb);
	base_tcb->tcb_ftcb_list.q_next = hcb->hcb_ftcb_list.q_prev->q_next;
	base_tcb->tcb_ftcb_list.q_prev = hcb->hcb_ftcb_list.q_prev;
	hcb->hcb_ftcb_list.q_prev->q_next = &base_tcb->tcb_ftcb_list;
	hcb->hcb_ftcb_list.q_prev = &base_tcb->tcb_ftcb_list;
	dm0s_munlock(&hcb->hcb_tq_mutex);

	/*
	** Return Base Table TCB.
	*/
	*build_tcb = base_tcb;

#ifdef NEVER_USED
	/* Should just delete this, but might be of academic interest: */
	/* if readahead is available, sort multiple index tcb in ascending */
	/* order of disk size, RA threads will start at end, with those    */
	/* most likely to incur disk IO being done first.		   */
	if ((base_tcb->tcb_index_count > 1) &&
	    (base_tcb->tcb_status & TCB_SHOWONLY) == 0 &&
	    (dmf_svcb->svcb_status & SVCB_READAHEAD))
	{
    	    bool 	exch;				/* TRUE if exchange */
     	    DMP_TCB 	*t = base_tcb;
    	    int 	numix = t->tcb_index_count;	/* # compares to do */
     	    int 	i;
     	    DMP_TCB	**pprev;			/* ptr to ptr to prev */

	    numix = base_tcb->tcb_index_count;
	    do
	    {
	    	exch = FALSE;
    	    	t = base_tcb;
	    	for (i = numix, pprev = &t->tcb_iq_next, t = t->tcb_iq_next;
			i--; pprev = &t->tcb_q_next, t = t->tcb_q_next)
	    	{
		    if (t->tcb_rel.relpages > t->tcb_q_next->tcb_rel.relpages)
		    {
		    	exch = TRUE;
		    	t = t->tcb_q_next;
		    	(*pprev)->tcb_q_next = t->tcb_q_next;
		    	t->tcb_q_next->tcb_q_prev = *pprev;
		    	t->tcb_q_next = *pprev;
		    	t->tcb_q_prev = (*pprev)->tcb_q_prev;
		    	t->tcb_q_next->tcb_q_prev = t;
		    	*pprev = t;
		    }
	    	}
	    	numix--;
	    } while (exch);
	    if (DMZ_BUF_MACRO(405))
	    {
		DMP_TCB *it;
		TRdisplay( " TCBs sorted for prefetch: %x  %x\n",
			  base_tcb->tcb_iq_prev,
			  base_tcb->tcb_iq_prev->tcb_q_next );

		for (it = base_tcb->tcb_iq_prev;
    		    it != (DMP_TCB*) &base_tcb->tcb_iq_next;
    		    it = it->tcb_q_prev)
		{
		    TRdisplay( "%x:  %x  %x  %d\n", it,
			      it->tcb_q_next, it->tcb_q_prev,
			      it->tcb_rel.relpages );
	    	}
	    }
	}
#endif /* NEVER_USED */
	/* Last thing to do is hook TCB onto PP array if partition */
	if (opening_partition)
	    pp_ptr->pp_tcb = base_tcb;

	/* Return holding hash mutex */
	return (E_DB_OK);
    }

    /*
    ** Error Condition (note that hash_mutex should not be held at this point).
    **
    ** Clean up:
    **	   replication rcb's and control blocks
    **     iirelation rcb used for select loop
    **     base table validation lock
    **     any allocated tcb's
    **     the dummy tcb linked to the hash queue at the start of the operation
    */

    /* Free partition cache */
    if ( pcache != lcache )
	MEfree((char*)pcache);

    if ( partitions_needed )
	MEfree((PTR)partitions_needed);
    if ( partitions_found )
	MEfree((PTR)partitions_found);

    if (rep_rcb)
	local_status = dm2t_close(rep_rcb, (i4)0, &local_dberr);

    if (rep_rcb_idx)
	local_status = dm2t_close(rep_rcb_idx, (i4)0, &local_dberr);

    if (rep_info)
	MEfree((char *)rep_info);

    if (rel_rcb)
    {
	local_status = dm2r_release_rcb(&rel_rcb, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    /*
    ** It is possible that many TCB's were allocated by this routine before
    ** error was generated.  These TCB's may be found in a number of places
    ** (although we are guaranteed that the same tcb cannot be linked to more
    ** than one place):
    **
    **     current 'tcb'
    **     base_tcb pointer
    **     chained on the base_tcb's index queue
    **     the list of index tcb's not yet linked to base_tcb
    */

    /*
    ** Link all allocated tcb's into one tcb list for deallocation.
    */
    if (tcb)
    {
	tcb->tcb_q_next = tcb_list;
	tcb_list = tcb;
    }
    if (base_tcb && tcb_list)
    {
	/*
	** Take each index tcb off of the base table index queue and put
	** them back on the local tcb list.
	*/
	while (base_tcb->tcb_iq_next != (DMP_TCB*)&base_tcb->tcb_iq_next)
	{
	    tcb = base_tcb->tcb_iq_next;
	    tcb->tcb_q_next->tcb_q_prev = tcb->tcb_q_prev;
	    tcb->tcb_q_prev->tcb_q_next = tcb->tcb_q_next;

	    tcb->tcb_q_next = tcb_list;
	    tcb_list = tcb;
	}

	base_tcb->tcb_q_next = tcb_list;
	tcb_list = base_tcb;
    }

    /*
    ** Deallocate each of the TCB's now linked on the local list.
    */
    while (tcb_list)
    {
	tcb = tcb_list;
	tcb_list = tcb->tcb_q_next;

	if (tcb->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    local_status = dm2f_release_fcb(lock_id, log_id,
		tcb->tcb_table_io.tbio_location_array,
		tcb->tcb_table_io.tbio_loc_count, 0, &local_dberr);
	    if (local_status)
	    {
        	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &local_error, (i4)0);
	    }
	}

	dm0s_mrelease(&tcb->tcb_et_mutex);
	dm0s_mrelease(&tcb->tcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&tcb);
    }

    /*
    ** Log traceback error message, and information about the table which
    ** couldn't be built.
    */
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	uleFormat(NULL, E_DM9C8B_DM2T_TBL_INFO, (CL_ERR_DESC *)NULL, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &local_error, (i4)3,
		    sizeof(dcb->dcb_name), &dcb->dcb_name,
		    0, table_id->db_tab_base,
		    0, table_id->db_tab_index);
	SETDBERR(dberr, 0, E_DM9C89_DM2T_BUILD_TCB);
    }

    /*
    ** Take the dummy_tcb off of the tcb hash queue and wake up anybody
    ** waiting for the build.  Return with the hash semaphore held.
    */
    dm0s_mlock(hash_mutex);
    if (dummy_tcb)
    {
	dummy_tcb->tcb_q_next->tcb_q_prev = dummy_tcb->tcb_q_prev;
	dummy_tcb->tcb_q_prev->tcb_q_next = dummy_tcb->tcb_q_next;

	if (dummy_tcb->tcb_status & TCB_WAIT)
	    dm0s_erelease(lock_id, DM0S_TCBWAIT_EVENT,
		dummy_tcb->tcb_unique_id);
	dm0m_deallocate((DM_OBJECT **)&dummy_tcb);
    }

    return (E_DB_ERROR);
}

/*{
** Name: find_pp_for_partition - Find PP array entry for partition
**
** Description:
**	When preparing to build a TCB for a physical partition, it's
**	neither necessary nor desirable to scan iirelation looking
**	for the proper row.  We've already been there once, when the
**	master TCB was opened (which has to happen first).
**
**	The master TCB open builds the physical partition array, and
**	stashes the iirelation row TID there.  This is ugly and hacky,
**	but it works.  As preparation for building the partition TCB,
**	this routine is called to find the master TCB as well as the
**	correct PP entry for the partition.
**
** Inputs:
**	dcb		Database control block
**	table_id	ID of the partition we're after
**	master_tcb	an output
**	pp_ptr		an output
**
** Outputs:
**	Returns OK or DB_STATUS error status
**	master_tcb	TCB pointer for master stored here
**	pp_ptr		Returned physical partition array entry pointer
**
** History:
**	16-Jan-2004 (schka24)
**	    Write, after much flailing around inside dm2t...
*/

static DB_STATUS
find_pp_for_partition(DMP_DCB *dcb, DB_TAB_ID *table_id,
	DMP_TCB **master_tcb, DMT_PHYS_PART **pp_ptr)
{

    DB_TAB_ID base_id;			/* Partitioned master ID */
    DMP_HASH_ENTRY *master_h;		/* Master TCB's hash bucket */
    DMT_PHYS_PART *pp;			/* PP array entry for partition */
    DM_MUTEX *hash_mutex;		/* Hash mutex for both partition and master */
    i4 i;

    /* Find the master TCB */

    base_id.db_tab_base = table_id->db_tab_base;
    base_id.db_tab_index = 0;
    hash_mutex = NULL;
    locate_tcb(dcb->dcb_id, &base_id, master_tcb, &master_h, &hash_mutex);
    if (*master_tcb == NULL)
    {
	dm0s_munlock(hash_mutex);
	return (E_DB_ERROR);
    }
    if ((*master_tcb)->tcb_ref_count <= 0)
    {
	/* FIXME not sure if this is valid or what, might be in recovery? */
	TRdisplay("%@ Master TCB for partition (%d,%d) not fixed.\n",
			table_id->db_tab_base,table_id->db_tab_index);
    }
    /* Don't need to hold the hash mutex while we work */
    dm0s_munlock(hash_mutex);

    /* Find the proper PP array entry */
    for (pp = &(*master_tcb)->tcb_pp_array[(*master_tcb)->tcb_rel.relnparts-1];
	 pp >= &(*master_tcb)->tcb_pp_array[0];
	 -- pp)
    {
	if (pp->pp_tabid.db_tab_index == table_id->db_tab_index)
	    break;
    }
    if (pp < &(*master_tcb)->tcb_pp_array[0])
    {
	return (E_DB_ERROR);
    }
    *pp_ptr = pp;
    return (E_DB_OK);

} /* find_pp_for_partition */

/*{
** Name: copy_from_master - Copy physical partition info from master TCB
**
** Description:
**	Building a TCB for a physical partition is rather different, and
**	somewhat simpler, than building a master TCB.  Much of the information
**	we need is already in memory.
**
**	Building a partition TCB is special mainly because there's no
**	iiattribute rows for the physical partition.  All partitions
**	share the master's column list.  For the present, all partitions
**	also share the master's storage structure.
**
**	This routine chases around and copies attribute and key stuff
**	from the master TCB.  Since some of the info is in the form of
**	pointer arrays, the work is slightly nontrivial.  In general,
**	though, this routine is mostly boring data movement...
**
** Inputs:
**	tcb		Partition TCB pointer
**	master_tcb	Master TCB pointer
**
** Outputs:
**	nothing
**	partition TCB is filled in.
**
** History:
**	16-Jan-2004 (schka24)
**	    Write for partitioned table project.
**	26-Jan-2004 (jenjo02)
**	    Added some stuff to copy from Master to Partition
**	    TCB to ensure that Partition TCB references know
**	    what they need to about Global indexes attached
**	    to Master.
**	5-May-2004 (schka24)
**	    Fix a use-before-set that screwed up ixatts in some cases.
**	26-Apr-2006 (jenjo02)
**	    For Btrees, don't try to second-guess what's already been
**	    figured out for master, just copy the stuff. CLUSTERED
**	    primary tables really fool around with keys.
*/

static void
copy_from_master(DMP_TCB *tcb, DMP_TCB *master_tcb)
{
    /*
    ** Partition TCB's are not allocated space for attributes,
    ** they share Masters, so all we need to do here is
    ** copy all of Master's read-only attribute stuff to
    ** the Partition TCB:
    */
    STRUCT_ASSIGN_MACRO(master_tcb->tcb_data_rac, tcb->tcb_data_rac);
    STRUCT_ASSIGN_MACRO(master_tcb->tcb_leaf_rac, tcb->tcb_leaf_rac);
    STRUCT_ASSIGN_MACRO(master_tcb->tcb_index_rac, tcb->tcb_index_rac);
    tcb->tcb_atts_ptr = master_tcb->tcb_atts_ptr;
    tcb->tcb_keys = master_tcb->tcb_keys;
    tcb->tcb_att_key_ptr = master_tcb->tcb_att_key_ptr;
    tcb->tcb_key_atts = master_tcb->tcb_key_atts;
    tcb->tcb_klen = master_tcb->tcb_klen;
    tcb->tcb_sec_key_att = master_tcb->tcb_sec_key_att;
    tcb->tcb_tperpage = master_tcb->tcb_tperpage;
    tcb->tcb_kperpage = master_tcb->tcb_kperpage;
    tcb->tcb_kperleaf = master_tcb->tcb_kperleaf;
    tcb->tcb_leafkeys = master_tcb->tcb_leafkeys;
    tcb->tcb_ixklen = master_tcb->tcb_ixklen;
    tcb->tcb_ixkeys = master_tcb->tcb_ixkeys;
    tcb->tcb_ixatts_ptr = master_tcb->tcb_ixatts_ptr;
    tcb->tcb_rng_rac = master_tcb->tcb_rng_rac;
    tcb->tcb_rngkeys = master_tcb->tcb_rngkeys;
    tcb->tcb_rngklen = master_tcb->tcb_rngklen;

    /*
    ** Make partition knowledgable about indexing stuff, especially
    ** that Master has indexes that may need to be updated when
    ** the Partition is updated.
    **
    ** Don't yet know how we're going to deal with local indexes...
    **
    ** For now, all indexes are Global and linked to Master's
    ** tcb_iq_next list; the Partition's tcb_iq_next will
    ** be empty, so one must be careful to check 
    ** tcb_pmast_tcb->tcb_iq_next when looking for indexes
    ** to update, not Partition's tcb_iq_next.
    */
    tcb->tcb_update_idx = master_tcb->tcb_update_idx;
    tcb->tcb_dups_on_ovfl = master_tcb->tcb_dups_on_ovfl;
    tcb->tcb_rng_has_nonkey = master_tcb->tcb_rng_has_nonkey;
    tcb->tcb_unique_index = master_tcb->tcb_unique_index;
    tcb->tcb_index_count = master_tcb->tcb_index_count;
    tcb->tcb_stmt_unique = master_tcb->tcb_stmt_unique;
    tcb->tcb_unique_count = master_tcb->tcb_unique_count;

    return;

} /* copy_from_master */

/*{
** Name: read_tcb
**
** Description:
**	This function fills in TCB information from system catalog data.
**
**	It uses the supplied iirelation row and then looks up all entries
**	for this table in the iiattribute and iidevices tables.
**
**	The caller is expected to have allocated the TCB memory and hands
**	this routine a pointer to that memory.
**
** Inputs:
**	tcb				Table Control Block to fill in
**	rel				IIrelation tuple for this table
**	dcb				Pointer to Database Control Block
**      tran_id                         Pointer to transaction id.
**      lock_id                         Lock list identifier.
**      log_id                          Log file identifier.
**      flag                            One of:
**					    0 - Do not perform async io/sync
**					       at close.
**					    DM2T_CLSYNC - Perform async i/o,
**					       sync files in dm2t_close
**
**
** Outputs:
**	err_code
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-aug-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	26-aug-1992 (25-jan-1990) (fred)
**	    Added support for extension tables for (initially) large object
**	    support.  This is managed by keeping a list of extension tables
**	    hanging off the tcb.  At this point, the support necessary in this
**	    routine is to initialize the tcb_et_mutex semaphore, and to build an
**	    empty list of extension tables.
**	29-August-1992 (rmuth)
**	    Remove the 6.4->6.5 on-the fly table conversion.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set up relcomptype and relpgtype and called
**	    dm1c_getaccessors to initialize tcb_acc_plv and tcb_acc_tlv.
**	    Calculated normal and key compression counts using an accessor
**	    rather than in-line.
**	    Added special code for VME forward compatibility.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	    Also initialized some new table_io fields for debug routines.
**	16-oct-1992 (rickh)
**	    Fill in the default id.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Changes:
**	      - Set tcb_sysrel correctly for iidevices catalog.
**	      - Changed to support new tableio structure in the TCB.
**	      - Initialized the loc_config_id field of each entry in the
**		table's location array.
**	      - Initialized tbio_checkeof to TRUE to cause the first logical
**		open of the table to initialize the lpageno value.
**	30-October-1992 (rmuth)
**	   - Remove references to file open flag DI_ZERO_ALLOC.
**	   - Set tbio_lalloc, tbio_lpageno and tbio_checkeof correctly.
**	   - Initialise tbio_tmp_hw_pageno.
**	18-jan-1993 (rogerk)
**	    Enabled recovery flag in tbio in CSP as well as RCP since the
**	    DCB_S_RCP status is no longer turned on in the CSP.
**	30-feb-1993 (rmuth)
**	    Multi-location tables, in read_tcb type mean't that we were
**	    checking a db_tab_base with a db_tab_index hence we never
**	    found all the location records in iidevices.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		In read_tcb, when scanning iiattribute, set rcb_access_mode
**		    to RCB_A_READ so that we don't fix pages for WRITE.
**		Remove special checks for DCB_S_CSP, since DCB's built by the
**		    CSP process no longer get special handling.
**	26-jul-1993 (bryanp)
**	    Fix error message problems in DM9C74 (pass &loc_name, not loc_name).
**	20-sep-1993 (rogerk)
**	    Add support for new non-journal table handling.  Due to non-jnl
**	    protocols, iidevices rows are not deleted when a table is reorged
**	    to a fewer number of locations.  Rather, they are updated to be
**	    dummy iidevices rows.  We must now check for these dummy rows when
**	    reading in rows from the catalog.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	23-may-1994 (jnash)
**	  - Set table's buffer manager cache priority to BMCB_MAX_PRI - 2
**	    for ROLLFORWARDDB and RCP.
**	  - Add trace point DM433 to fix all table priorities.
**	20-jun-1994 (jnash)
**	    fsync project.
**	    - Add flag param used currently to pass sync-at-end info.
**	    - Eliminate tcb_logical_log maintenance, remnants of old recovery.
**      12-jul-1994 (ajc)
**          Fix function prototype such that it will compile.
**	19-jan-1995 (cohmi01)
**	    Propagate RAW bit from DCB to DMP_LOCATION.
**	06-mar-1995 (shero03)
**	    Bug B67276
**	    Compare the location usage when opening a table.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't use DM_PG_SIZE, use relpgsize instead.
**	28-apr-1996 (shero03)
**	    Support RTree where attr size = sizeof(hilbert)
**	06-may-1996 (nanpr01 & thaju02)
**	    Change all page header access as macro for New Page Format
**	    project. Also changed the key per page calculation.
**	    Pass page size to dm1c_getaccessors().
**	 4-jul-96 (nick)
**	    Ensure we lock the catalogs used here in shared mode.
**	12-dec-1996 (cohmi01)
**	    Add detailed err messages for corrupt system catalogs (Bug 79763).
**      22-jul-1996 (ramra01 for bryanp)
**          For Alter Table support, set new DB_ATTS fields from corresponding
**              values in iiattribute records.
**	15-Oct-1996 (jenjo02)
**	    Create tcb_mutex, tcb_et_mutex names using table name to uniquify
**	    for iimonitor, sampler.
**	23-oct-1996 (hanch04)
**          During an upgrade, attintl_id will not exist.
**          Use attid for the default.
**	22-Jan-1997 (jenjo02)
**	    Moved TCB allocation and initialization to a function,
**	    dm2t_alloc_tcb(). When entered here, the TCB has
**	    been allocated and initialized to default values.
**      21-May-1997 (stial01)
**          For compatibility reasons, DO include TIDP for V1-2k unique indexes
**              (2k unique indexes that do not include TIDP in key must
**               be 5.0 indexes, in which case we do not include TIDP in 
**               tcb_klen.
**          V2 (4k+) unique indexes do not include tidp in key however,
**          tidp should be included in tcb_klen.
**      12-jun-97 (stial01)
**          read_tcb() Include attribute information in kperpage calculation
**	19-sep-97 (stephenb)
**	    Replicator code cleanup, initialise rep_key_seq to zero
**      09-feb-99 (stial01)
**          read_tcb() Pass relcmptlvl to dm1*_kperpage
**	22-Jun-2000 (thaju02 & inifa01)
**	   bug 101920
**	   'Alter table drop column' corrupts tables of upgraded databases.
**	   select queries fail with E_QE007C Error trying to get a record.
**	   Attempts to drop the table causes the server to crash in
**	   dm1cn_compexpand.
**	   Removed the re-initialization of attribute.attintl_id =
**	   attribute.attid 
**	   for upgraded databases since this is already initialized at point of 
**	   upgradeddb.  Re-initializing this value each time in read_tcb to the
**	   current
**	   attid value causes us to loose the attintl_id records for dropped
**	   columns. 
**	16-nov-00 (hayke02)
**	    The above fix for bug 101920 has been backed out, and the original
**	    code to set attintl_id = attid has been changed so that we test
**	    for a page size of 2K for the relation containing the attribute
**	    being processed. The test/setting of db_tab_base has been removed.
**	    This change fixes bug 103243.
**	16-Jan-2004 (schka24)
**	    For partitions, stop reading after location stuff.
**	22-Jan-2004 (jenjo02)
**	    Set tcb_partno, allow for TID8s in Global Btree indexes.
**	03-Mar-2004 (gupsh01)
**	    Added support for alter table alter column.
**	14-dec-04 (inkdo01)
**	    Added collID in iiattribute.
**	13-Apr-2006 (jenjo02)
**	    Add support for CLUSTERED primary Btree tables.
**	29-Sept-2009 (troal01)
**		Add support for geospatial.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Set flag for btree-leaf-uses-overflow-for-duplicates.
**	27-May-2010 (gupsh01) BUG 123823
**	    Fix error handling for error E_DM0075_BAD_ATTRIBUTE_ENTRY. 
*/

static DB_STATUS
read_tcb(
DMP_TCB		*tcb,
DMP_RELATION	*rel,
DMP_DCB		*dcb,
DB_TRAN_ID      *tran_id,
i4         	lock_id,
i4         	log_id,
i4         	flag,
DB_ERROR	*dberr)
{
    DMP_TCB		*t = tcb;
    DMP_RCB		*att_rcb = 0;
    DMP_TCB		*att_tcb;
    DMP_RCB		*dev_rcb = 0;
    DMP_ATTRIBUTE	attribute;
    DB_ATTS		*a;
    DMP_DEVICE		devrecord;
    DMP_LOCATION	*loc_entry;
    DM2R_KEY_DESC	key_desc[2];
    DB_STATUS		status = E_DB_OK;
    DM_TID		tid;
    DB_TAB_ID		tableid;
    DB_TAB_TIMESTAMP    timestamp;
    i4			i,k;
    i4			att_count;
    i4			offset;
    i4			local_error;
    DB_STATUS		local_status;
    DB_ERROR		local_dberr;
    bool		encrypted_attrs = FALSE;
    i4		    *err_code = &dberr->err_code;
    i4			alen;
    PTR			nextattname = t->tcb_atts_names;

    CLRDBERR(dberr);

    /*
    ** Initialize portions of the TCB used for IO access and those dependent`
    ** on other system catalog information.
    */

    for (;;)
    {
	/*****************************************************
	**
	** Fill in the Table IO control block of the TCB.
	**
	******************************************************/

	/*
	** Fill in the first location of the table's location array from
	** the iirelation record.  If there is more than one location
	** then we have to read iidevices to get location information.
	**
	** Note that views and gateway tables which do not really have
	** Ingres files still get the first location entry initialized here
	** but are expected to have a location count of 1 to avoid iidevices
	** lookups.
	*/
	t->tcb_table_io.tbio_location_array[0].loc_id = 0;
	STRUCT_ASSIGN_MACRO(rel->relloc,
			t->tcb_table_io.tbio_location_array[0].loc_name);

	/*
	** Fill in needed information from iidevices.
	*/
	k = t->tcb_table_io.tbio_loc_count;
	if (t->tcb_table_io.tbio_loc_count > 1)
	{
	    /*
	    ** Open iidevices table.
	    */
	    tableid.db_tab_base = DM_B_DEVICE_TAB_ID;
	    tableid.db_tab_index= DM_I_DEVICE_TAB_ID;
	    status = dm2t_open(dcb, &tableid, DM2T_IS, DM2T_UDIRECT,
		DM2T_A_READ, (i4)0, (i4)20, (i4)0,
		log_id, lock_id, (i4)0, (i4)0, DM2T_IS,
		tran_id, &timestamp, &dev_rcb, (DML_SCB *)NULL, dberr);
	    if (status != E_DB_OK)
		break;

	    dev_rcb->rcb_lk_mode = RCB_K_IS;
	    dev_rcb->rcb_k_mode = RCB_K_IS;
	    dev_rcb->rcb_access_mode = RCB_A_READ;

	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_DEVICE_KEY;
	    key_desc[0].attr_value = (char *) &t->tcb_rel.reltid.db_tab_base;
	    key_desc[1].attr_operator = DM2R_EQ;
	    key_desc[1].attr_number = DM_2_DEVICE_KEY;
	    key_desc[1].attr_value = (char *) &t->tcb_rel.reltid.db_tab_index;

	    /*
	    ** Position iidevices table to return rows for the given relation.
	    */
	    status = dm2r_position(dev_rcb, DM2R_QUAL, key_desc,
		(i4)2, (DM_TID *)NULL, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Loop through location entries in iidevices, filling in the
	    ** location array as we go.  This loop is expected to end by
	    ** getting a NONEXT error from the dm2r_get call.
	    */
	    k = 1;
	    for (;;)
	    {
		status = dm2r_get(dev_rcb, &tid, DM2R_GETNEXT,
			     (char *)&devrecord, dberr);
		if (status != E_DB_OK)
		{
		    if (dberr->err_code == E_DM0055_NONEXT)
			status = E_DB_OK;
		    break;
		}

		/*
		** Skip rows not belonging to this table.
		*/
		if ((devrecord.devreltid.db_tab_base !=
					    t->tcb_rel.reltid.db_tab_base) ||
		    (devrecord.devreltid.db_tab_index !=
					    t->tcb_rel.reltid.db_tab_index))
		{
		    continue;
		}

		/*
		** Skip rows with no devloc value.  These are considered
		** dummy rows and are the result of spreading a table over
		** multiple locations and then reducing the number of locs
		** on which the table is defined.
		*/
		if (devrecord.devloc.db_loc_name[0] == ' ')
		    continue;

		/*
		** Verify location number and count.
		*/
		if ((++k > t->tcb_table_io.tbio_loc_count) ||
		    (devrecord.devlocid >= t->tcb_table_io.tbio_loc_count))
		{
		    uleFormat(NULL, E_DM9C72_DM2T_LOCCOUNT_MISMATCH, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&local_error, (i4)6,
			sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			0, k, 0, devrecord.devlocid,
			0, t->tcb_table_io.tbio_loc_count);
		    SETDBERR(dberr, 0, E_DM007A_BAD_DEVICE_ENTRY);
		    status = E_DB_ERROR;
		    break;
		}

		loc_entry =
		    &t->tcb_table_io.tbio_location_array[devrecord.devlocid];
		STRUCT_ASSIGN_MACRO(devrecord.devloc, loc_entry->loc_name);
		loc_entry->loc_id = devrecord.devlocid;
	    }
	    if (status != E_DB_OK)
		break;

	    status = dm2t_close(dev_rcb, 0, dberr);
	    if (status != E_DB_OK)
		break;
	    dev_rcb = 0;
	}

	/*
	** Verify that the correct number of location records was found.
	*/
	if (k != t->tcb_table_io.tbio_loc_count)
	{
	    uleFormat(NULL, E_DM9C73_DM2T_LOCCOUNT_ERROR, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		&local_error, (i4)5,
		sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		0, k, 0, t->tcb_table_io.tbio_loc_count);
	    SETDBERR(dberr, 0, E_DM007A_BAD_DEVICE_ENTRY);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Fill in location names for each table location from the
	** database extent lists in the DCB.
	**
	** Search through extent list for an entry matching each location
	** in the location list.  The location name "$default" is special
	** and indicates to use the root database location.
	*/
	for (k=0; k < t->tcb_table_io.tbio_loc_count; k++)
	{
	    /* Check for $default location */
	    if (MEcmp((PTR)DB_DEFAULT_NAME,
		(PTR)&t->tcb_table_io.tbio_location_array[k].loc_name,
		sizeof(DB_LOC_NAME)) == 0)
	    {
		t->tcb_table_io.tbio_location_array[k].loc_ext = dcb->dcb_root;
		t->tcb_table_io.tbio_location_array[k].loc_config_id = 0;
	    }
	    else
	    {
		i4	    compare = -1;

		for (i = 0; i < dcb->dcb_ext->ext_count; i++)
		{
		    /* B67276 */
		    /* verify this is a data location */
		    if ((dcb->dcb_ext->ext_entry[i].flags & DCB_DATA) == 0)
			continue;

		    /* 
		    ** A readonly database has only one location. Do not
		    ** compare logical names, because they will be different.
		    */
		    if (dcb->dcb_access_mode == DCB_A_READ)
		    {
		      STRUCT_ASSIGN_MACRO(dcb->dcb_ext->ext_entry[i].logical, 
			t->tcb_table_io.tbio_location_array[k].loc_name); 
		      compare = 0;
		    }
		    else
		      compare = MEcmp(
			(char*)&t->tcb_table_io.tbio_location_array[k].loc_name,
			(char *)&dcb->dcb_ext->ext_entry[i].logical,
			sizeof(DB_LOC_NAME));
		    if (compare == 0)
		    {
			t->tcb_table_io.tbio_location_array[k].loc_ext
				   = &dcb->dcb_ext->ext_entry[i];
			if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
			    t->tcb_table_io.tbio_location_array[k].loc_status
				   |= LOC_RAW;
			t->tcb_table_io.tbio_location_array[k].loc_config_id=i;
			break;
		    }
		}
		if (compare != 0)
		{
		    uleFormat(NULL, E_DM9C74_DM2T_LOCNAME_ERROR, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&local_error, (i4)4,
			sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			sizeof(DB_LOC_NAME),
		t->tcb_table_io.tbio_location_array[k].loc_name.db_loc_name);

		    SETDBERR(dberr, 0, E_DM007A_BAD_DEVICE_ENTRY);
		    status = E_DB_ERROR;
		    break;
		}

	    }
	    t->tcb_table_io.tbio_location_array[k].loc_status |= LOC_VALID;
	}
	if (status != E_DB_OK)
	    break;

	/* For partitions, stop here.  Partitions share the master's
	** iiattribute info.  Attempting to find iiattribute rows for a
	** partition will end in embarrassing failure...
	*/
	if (t->tcb_rel.relstat2 & TCB2_PARTITION)
	{
	    /*
	    ** Copy the partition number.  relnparts is overloaded
	    ** and this way it's clear to everyone when
	    ** a partition's number is needed.
	    */
	    t->tcb_partno = t->tcb_rel.relnparts;
	    return (E_DB_OK);
	}


	/*************************************************************
	**
	** Fill in attribute information from the IIATTRIBUTE catalog.
	**
	**************************************************************/

	/*
	** Open RCB for iiattribute table.
	*/
	att_tcb = dcb->dcb_att_tcb_ptr;
	status = dm2r_rcb_allocate(att_tcb, (DMP_RCB *)NULL,
	    tran_id, lock_id, log_id, &att_rcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Indicate that access to iiattribute through this RCB will be
	** read-only, so we don't try to fix pages for WRITE.
	*/
	att_rcb->rcb_lk_mode = RCB_K_IS;
	att_rcb->rcb_k_mode = RCB_K_IS;
	att_rcb->rcb_access_mode = RCB_A_READ;

	/*
	** Position to read records for this table.
	*/
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_ATTRIBUTE_KEY;
	key_desc[0].attr_value = (char *) &t->tcb_rel.reltid.db_tab_base;
	key_desc[1].attr_operator = DM2R_EQ;
	key_desc[1].attr_number = DM_2_ATTRIBUTE_KEY;
        key_desc[1].attr_value = (char*) &t->tcb_rel.reltid.db_tab_index;

        status = dm2r_position(att_rcb, DM2R_QUAL, key_desc, (i4)2,
            (DM_TID *)NULL, dberr);
        if (status != E_DB_OK)
            break;

	/*
	** Initialize attribute fields udpated with aggregate information
	** as each row is processed.
	*/
	att_count = 0;

	t->tcb_data_rac.encrypted_attrs = FALSE;
	/*
	** Read through iiattribute records describing each column in
	** in the tcb's attribute array.
	*/
	for (;;)
	{
	    /*
	    ** Get the next attribute for this table.
	    ** This read loop is expected to end with a NONEXT return from
	    ** this call.
	    */
	    status = dm2r_get(att_rcb, &tid, DM2R_GETNEXT,
		(char *)&attribute, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		    status = E_DB_OK;
		break;
	    }

	    /*
	    ** Skip rows not belonging to the requested table.
	    */
	    if ((attribute.attrelid.db_tab_base !=
				t->tcb_rel.reltid.db_tab_base) ||
		(attribute.attrelid.db_tab_index !=
				t->tcb_rel.reltid.db_tab_index))
	    {
		continue;
	    }

	    /*
	    ** During an upgrade, attintl_id will not exist.
	    ** Use attid for the default. We are in an upgrade when
	    ** reltotwid < relwid
	    */
	    if (( att_tcb->tcb_rel.reltotwid < att_tcb->tcb_rel.relwid) &&
		(t->tcb_rel.relpgsize == DM_COMPAT_PGSIZE))
	    {
		attribute.attintl_id = attribute.attid;
		attribute.attver_added = 0;
		attribute.attver_dropped = 0;
		attribute.attval_from = 0;
		attribute.attver_altcol = 0;
	    }

	    /*
	    ** During an upgrade, attintl_id will not exist.
	    ** Use attid for the default. We are in an upgrade when
	    ** reltotwid < relwid
	    */
	    if (( att_tcb->tcb_rel.reltid.db_tab_base == 3 ) &&
		( att_tcb->tcb_rel.reltotwid < att_tcb->tcb_rel.relwid) )
	    {
		attribute.attintl_id = attribute.attid;
		attribute.attver_added = 0;
		attribute.attver_dropped = 0;
		attribute.attval_from = 0;
		attribute.attver_altcol = 0;
	    }

	    /*
	    ** Check for wacko attribute information.
	    */

	    att_count++;
	    if ((attribute.attid > t->tcb_rel.relatts) || 
		(attribute.attid <= 0))
	    {
		uleFormat(NULL, E_DM00A8_BAD_ATT_ENTRY, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, &local_error, (i4)5, 0, attribute.attid,
		    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
		    0, t->tcb_rel.relatts);
		SETDBERR(dberr, 0, E_DM9C59_DM2T_READ_TCB);
		status = E_DB_ERROR;
		break;
	    }

	    else if (att_count > t->tcb_rel.relatts)
	    {
		uleFormat(NULL, E_DM00A9_BAD_ATT_COUNT, 
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, &local_error, (i4)5, 0, att_count, 
		    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
		    0, t->tcb_rel.relatts);
		SETDBERR(dberr, 0, E_DM9C59_DM2T_READ_TCB);
		status = E_DB_ERROR;
		break;
	    }

	    a = &t->tcb_atts_ptr[attribute.attintl_id];

	    /* Compute blank-stripped length of attribute name */
	    for (alen = DB_ATT_MAXNAME;  
		attribute.attname.db_att_name[alen-1] == ' ' && alen >= 1; 
			alen--);

	    /* make sure we have space for attribute name */
	    if (t->tcb_atts_used + alen + 1 > t->tcb_atts_size)
	    {
		SETDBERR(dberr, 0, E_DM0075_BAD_ATTRIBUTE_ENTRY);
	        status = E_DB_ERROR;
		break;
	    }

	    a->attnmlen = alen;
	    a->attnmstr = nextattname;
	    MEcopy(attribute.attname.db_att_name, alen, nextattname);
	    nextattname += alen;
	    *nextattname = '\0';
	    nextattname++;
	    t->tcb_atts_used += (alen + 1);

	    a->offset = attribute.attoff;
	    a->type = attribute.attfmt;
	    a->length = attribute.attfml;
	    a->precision = attribute.attfmp;
	    a->key = attribute.attkey;
	    a->flag = attribute.attflag;
	    a->key_offset = 0;

	    COPY_DEFAULT_ID( attribute.attDefaultID, a->defaultID );

	    a->defaultTuple = 0;
	    a->collID = attribute.attcollID;
	    a->replicated = FALSE;
	    a->rep_key_seq = 0;
	    a->intl_id = attribute.attintl_id;
	    a->ver_added = attribute.attver_added;
	    a->ver_dropped = attribute.attver_dropped;
	    a->val_from = attribute.attval_from;
	    a->ordinal_id = attribute.attid;
	    a->ver_altcol = attribute.attver_altcol;
	    a->collID = attribute.attcollID;
	    a->geomtype = attribute.attgeomtype;
	    a->srid = attribute.attsrid;
	    a->encflags = attribute.attencflags;
	    if (attribute.attencflags & ATT_ENCRYPT)
		t->tcb_data_rac.encrypted_attrs = TRUE;
	    a->encwid = attribute.attencwid;



	    /*
	    ** Set up list of pointers to attribute info for use in
	    ** data compression. Note that the tcb_data_rac and
	    ** tcb_key_atts lists are indexed from 0, not from 1.
	    */
	    t->tcb_data_rac.att_ptrs[attribute.attintl_id - 1] = a;

	    /*
	    ** Store Key information.
	    */
	    if (attribute.attkey && t->tcb_rel.relspec != TCB_HEAP)
	    {
		if (t->tcb_keys++ > t->tcb_rel.relkeys || 
		    attribute.attkey > t->tcb_rel.relkeys)
		{
	    	    uleFormat(NULL, E_DM00AA_BAD_ATT_KEY_COUNT, 
			(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
			(i4)0, (i4 *)NULL, &local_error, (i4)6, 
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
			0, t->tcb_rel.relkeys, 0, t->tcb_keys, 
			0, attribute.attkey);
		    SETDBERR(dberr, 0, E_DM9C59_DM2T_READ_TCB);
	    	    status = E_DB_ERROR;
	    	    break;
		}
		t->tcb_att_key_ptr[attribute.attkey - 1] = attribute.attintl_id;
		t->tcb_key_atts[attribute.attkey - 1] = a;
		t->tcb_klen += attribute.attfml;
	    }

	    if (attribute.attflag & ATT_SEC_KEY)
		t->tcb_sec_key_att = a;

	}
	if (status != E_DB_OK)
	    break;

	/*
	** Release iiattribute RCB.
	*/
	status = dm2r_release_rcb(&att_rcb, dberr);
	if (status != E_DB_OK)
	    break;
	att_rcb = 0;

	/*
	** Verify that all appropriate attribute rows and keys were found.
	*/
	if (t->tcb_keys != t->tcb_rel.relkeys) 
	{
	    uleFormat(NULL, E_DM00AA_BAD_ATT_KEY_COUNT, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
		(i4)0, (i4 *)NULL, &local_error, (i4)6, 
		sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
		0, t->tcb_rel.relkeys, 0, t->tcb_keys, 0, attribute.attkey);
	    SETDBERR(dberr, 0, E_DM9C59_DM2T_READ_TCB);
	    status = E_DB_ERROR;
	    break;
	}
	else if (att_count != t->tcb_rel.relatts)
	{
	    uleFormat(NULL, E_DM00A9_BAD_ATT_COUNT, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
		(i4 *)NULL, &local_error, (i4)5, 0, att_count, 
		sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		sizeof(DB_DB_NAME), t->tcb_dcb_ptr->dcb_name.db_db_name,
		0, t->tcb_rel.relatts);
	    SETDBERR(dberr, 0, E_DM9C59_DM2T_READ_TCB);
	    status = E_DB_ERROR;
	    break;
	}

	status = dm1c_rowaccess_setup(&t->tcb_data_rac);
	if (status != E_DB_OK)
	    break;

	t->tcb_tperpage = DMPP_TPERPAGE_MACRO(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize,t->tcb_rel.relwid);

	/* 
	** Compute the key offset information.
	*/
	for ( i = 0, offset = 0; i < t->tcb_keys; i++ )
	{
	    /* 
	    ** If CLUSTERED Btree, we need separate copies of key DB_ATTS,
	    ** one for the Leaf pages, and one for the Index pages.
	    ** The Leaf attributes will have key_offset set to the 
	    ** natural offset of the column in the tuple; the Index
	    ** keys are extracted and concatenated together, so their
	    ** key_offsets will relative to the concatenated key pieces.
	    **
	    ** tcb_key_atts are those of the Leaf,
	    ** tcb_ixkeys are an array of pointers to the
	    ** tcb_ixatts_ptr Index copies.
	    ** 
	    ** Note the space for tcb_ixkeys and tcb_ixatts was
	    ** preallocated by dm2t_alloc_tcb when CLUSTERED.
	    **
	    ** In all others cases, LEAF and INDEX are the same, contiguous
	    ** and offset from zero.
	    */
	    if ( t->tcb_rel.relspec == TCB_BTREE &&
	         t->tcb_rel.relstat & TCB_CLUSTERED )
	    {
		/* Copy the LEAF key attribute */
		STRUCT_ASSIGN_MACRO(*t->tcb_key_atts[i],
				    t->tcb_ixatts_ptr[i]);
				    
		/* Point to the new INDEX attribute */
		t->tcb_ixkeys[i] = 
		    &t->tcb_ixatts_ptr[i];

		/* LEAF key_offset is natural column offset */
		t->tcb_key_atts[i]->key_offset = t->tcb_key_atts[i]->offset;
		/* INDEX key_offset is compacted */
		t->tcb_ixkeys[i]->key_offset = offset;
	    }
	    else
		t->tcb_key_atts[i]->key_offset = offset;
	    offset += t->tcb_key_atts[i]->length;
	}

	/*
	** Make any storage structure dependent calculations.
	*/
	if ( t->tcb_rel.relspec == TCB_BTREE )
	{
	    bool leaf_setup;

	    if ( t->tcb_rel.relstat & TCB_INDEX )
	    {
		/* Btree secondary can have non-key fields AND/OR compressed keys */
		init_bt_si_tcb(t, &leaf_setup);
	    }
	    else
	    {
		/* For clustered primary btree, the leaf is the entire row */
		if ( t->tcb_rel.relstat & TCB_CLUSTERED )
		{
		    /* All row attributes are in the leaf */
		    t->tcb_leaf_rac.att_ptrs = t->tcb_data_rac.att_ptrs;
		    t->tcb_leaf_rac.att_count = t->tcb_data_rac.att_count;
		    t->tcb_leafkeys  = t->tcb_key_atts;
		    t->tcb_klen = t->tcb_rel.relwid;

		    /* tcb_ixkeys points to the INDEX DB_ATTS in tcb_ixatts_ptr */

		    /* INDEX pages just have the key */
		    t->tcb_ixklen = offset;
		    /* The leaf may compress its own way, set up its
		    ** row accessor in case its compressing.
		    */
		    leaf_setup = TRUE;
		}
		else
		{
		    /* For nonclustered primary btree, att/keys are same on LEAF/INDEX pages */
		    t->tcb_ixkeys = t->tcb_key_atts;
		    t->tcb_leaf_rac.att_ptrs = t->tcb_ixkeys;
		    t->tcb_leaf_rac.att_count = t->tcb_keys;
		    t->tcb_leafkeys  = t->tcb_key_atts;
 		    t->tcb_ixklen = t->tcb_klen;
		    /* Leaf compresses identically to index */
		    leaf_setup = FALSE;
		}

		/* Clustered or not, INDEX pages just have keys */
		t->tcb_index_rac.att_ptrs = t->tcb_ixkeys;
		t->tcb_index_rac.att_count = t->tcb_keys;

		/* Clustered or not, primary LEAF range entries just have keys */
		t->tcb_rngkeys = t->tcb_ixkeys;
		t->tcb_rng_rac = &t->tcb_index_rac;
		t->tcb_rngklen = t->tcb_ixklen;
	    }
	    status = dm1c_rowaccess_setup(&t->tcb_index_rac);
	    if (status != E_DB_OK)
		break;

	    /* If the leaf may have different compression needs than
	    ** the index, set up its row accessor.  Otherwise just
	    ** copy the stuff from the index row accessor to the leaf.
	    */
	    if (leaf_setup)
	    {
		status = dm1c_rowaccess_setup(&t->tcb_leaf_rac);
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		t->tcb_leaf_rac.cmp_control = t->tcb_index_rac.cmp_control;
		t->tcb_leaf_rac.dmpp_compress = t->tcb_index_rac.dmpp_compress;
		t->tcb_leaf_rac.dmpp_uncompress =
			    t->tcb_index_rac.dmpp_uncompress;
		t->tcb_leaf_rac.control_count =
			    t->tcb_index_rac.control_count;
		t->tcb_leaf_rac.worstcase_expansion =
			    t->tcb_index_rac.worstcase_expansion;
		t->tcb_leaf_rac.compresses_coupons =
			    t->tcb_index_rac.compresses_coupons;
	    }

	    /* Compute kperpage for INDEX/LEAF */
	    t->tcb_kperpage = 
		dm1b_kperpage(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_rel.relcmptlvl, t->tcb_index_rac.att_count,
		t->tcb_ixklen, DM1B_PINDEX,
		sizeof(DM_TID), t->tcb_rel.relstat & TCB_CLUSTERED);

	    /* Global SI will have TID8-sized tids on LEAF pages */
	    t->tcb_kperleaf = 
		dm1b_kperpage(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_rel.relcmptlvl, t->tcb_leaf_rac.att_count,
		t->tcb_klen, DM1B_PLEAF,
		(t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX)
		    ? sizeof(DM_TID8) : sizeof(DM_TID),
		t->tcb_rel.relstat & TCB_CLUSTERED);

	    /* If CLUSTERED, tups per page same as keys per leaf */
	    if ( t->tcb_rel.relstat & TCB_CLUSTERED )
		t->tcb_tperpage = t->tcb_kperleaf;
	    /* This version test gets V10 and newer. */
	    t->tcb_dups_on_ovfl = (t->tcb_rel.relpgtype == TCB_PG_V1
			|| t->tcb_rel.relcmptlvl < DMF_T_OLD_VERSION);
	}
	else if (t->tcb_rel.relspec == TCB_RTREE)
	{
	    t->tcb_klen = t->tcb_rel.relwid;
	    if (t->tcb_atts_ptr[t->tcb_rel.relatts].key == 0)
	    {
		if (tcb->tcb_rel.relpgtype == TCB_PG_V1)
		    t->tcb_klen -= sizeof(DM_TID);
	    }
	    else
	    {
		t->tcb_key_atts[t->tcb_keys - 1]->key_offset =
		    t->tcb_rel.relwid - sizeof(DM_TID);
	    }
	    t->tcb_index_rac.att_ptrs = t->tcb_key_atts;
	    t->tcb_index_rac.att_count = t->tcb_keys;
	    status = dm1c_rowaccess_setup(&t->tcb_index_rac);
	    if (status != E_DB_OK)
		break;
	    /* Right now RTREE Is not using these new fields... */
	    t->tcb_kperpage = 
		dm1m_kperpage(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_rel.relcmptlvl, 0, t->tcb_klen, DM1B_PINDEX);

	    t->tcb_kperleaf = 
		dm1m_kperpage(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
		t->tcb_rel.relcmptlvl, 0, t->tcb_klen - t->tcb_hilbertsize, DM1B_PLEAF);
	}

	break;
    }

    if (status == E_DB_OK)
	return (E_DB_OK);

    if (dev_rcb)
    {
	local_status = dm2t_close(dev_rcb, 0, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    if (att_rcb)
    {
	local_status = dm2r_release_rcb(&att_rcb, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL && dberr->err_code != E_DM9C59_DM2T_READ_TCB)
    {
	/* only issue errmsg if detailed one not already issued above */
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	SETDBERR(dberr, 0, E_DM9C59_DM2T_READ_TCB);
    }

    return (E_DB_ERROR);
}

/*{
** Name: build_index_info - Build Secondary Index information into TCB's
**
** Description:
**	This routine is called during the building of a TCB for a table
**	which has secondary indexes.
**
**	It reads iiindex information for the table and writes it to the
**	TCBs.  It also links the secondary index tcbs onto the base
**	table's index list.
**
**	An error is returned if the iiindex catalog entries do not match
**	the index tcb list passed in.
**
** Inputs:
**	base_tcb			Table Control Block for base table
**	index_list			List of TCB's for secondary indexes
**      tran_id                         Pointer to transaction id.
**      lock_id                         Lock list identifier.
**      log_id                          Log file identifier.
**
** Outputs:
**	err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-aug-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	15-may-1993 (rmuth)
**	    Add support for building concurrent indicies.
**	29-apr-1996 (shero03)
**	    Add support for RTree indexes
**	 4-jul-96 (nick)
**	    Lock iiindex in shared mode.
**	26-Mar-1997 (jenjo02)
**	    Propagate base table's cache priority to index tcbs, unless
**	    index was defined with an explicit priority.
**	26-Jun-1997 (shero03)
**	    Fix a problem with Rtree iirange float values.
**      09-feb-99 (stial01)
**          build_index_info() Pass relcmptlvl to dm1*_kperpage
**       1-Jul-2004 (hanal04) Bug 112284 INGSRV2820
**          Recalculate tcb_kperleaf for RTREEs once the tcb_hilbersize
**          has been established. The initial calculation in 
**          read_tcb() will have used the uninitialised tcb_hilbersize 
**          and set an incorrect value for tcb_kperleaf.
**	15-Mar-2005 (schka24)
**	    Propagate show-onlyness of base table to indexes.
**	30-May-2006 (jenjo02)
**	    DMP_INDEX expanded to DB_MAXIXATTS idoms 
**	    in support of indexes on clustered tables.
*/
static DB_STATUS
build_index_info(
DMP_TCB		*base_tcb,
DMP_TCB		**index_list,
DB_TRAN_ID      *tran_id,
i4         lock_id,
i4         log_id,
bool		parallel_idx,
DB_ERROR	*dberr)
{
    DMP_DCB		*dcb = base_tcb->tcb_dcb_ptr;
    DMP_RCB		*idx_rcb = 0;
    DMP_TCB		*t;
    DMP_RCB		*rtree_rcb = 0;
    DMP_TCB		**prev_tcb_ptr;
    DM2R_KEY_DESC	key_desc[2];
    DMP_INDEX		index;
    DM_TID		tid;
    i4		j;
    i4		local_error;
    DB_STATUS		local_status;
    DB_STATUS		status;
    i4                   i;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** Open RCB on the iiindex catalog.
	*/
	status = dm2r_rcb_allocate(dcb->dcb_idx_tcb_ptr, (DMP_RCB *)NULL,
	    tran_id, lock_id, log_id, &idx_rcb, dberr);
	if (status != E_DB_OK)
	    break;

	idx_rcb->rcb_lk_mode = RCB_K_IS;
	idx_rcb->rcb_k_mode = RCB_K_IS;
	idx_rcb->rcb_access_mode = RCB_A_READ;

	/* Build the key for positioning the table. */

	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELATION_KEY;
	key_desc[0].attr_value = (char *) &base_tcb->tcb_rel.reltid.db_tab_base;

	status = dm2r_position(idx_rcb, DM2R_QUAL, key_desc,
	    (i4)1, (DM_TID *)NULL, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	**  For each index record for this base table update the corresponding
	**  index TCB with the attribute numbers that this index is defined on.
	*/
	for (;;)
	{
	    /*
	    ** Get the next indexes record.
	    ** This read loop is expected to be terminated by a NONEXT return
	    ** from this get call.
	    */
	    status = dm2r_get(idx_rcb, &tid, DM2R_GETNEXT,
		(char *)&index, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		    status = E_DB_OK;
		break;
	    }

	    /*
	    ** Make sure it's one that we want.
	    */
	    if (index.baseid != base_tcb->tcb_rel.reltid.db_tab_base)
		continue;

	    /*
	    ** Search the passed in tcb list for the one described by this
	    ** iiindex row.  It is an error to not find the index.
	    */
	    for (t = *index_list, prev_tcb_ptr = index_list;
		 t != NULL;
		 prev_tcb_ptr = &t->tcb_q_next, t = t->tcb_q_next)
	    {
		if (t->tcb_rel.reltid.db_tab_index == index.indexid)
		    break;
	    }

	    if (t == NULL)
	    {
		if ( base_tcb->tcb_rel.relstat2 & TCB2_READONLY )
		{
		    /*
		    ** When a base table is in readonly mode a user is
		    ** allowed to create concurrent indicies. These are
		    ** ignored by build_tcb which may cause this mismatch.
		    ** It would be nice to make further consistency checks
		    ** but at this momement assume all is ok.
		    */
		    continue;
		}
		else
		{
		    /* XXXX Add better error message */
		    SETDBERR(dberr, 0, E_DM9334_DM2T_MISSING_IDX);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Fill in index key information.
	    ** Verify that each piece of index information is valid.
	    */
	    if (t->tcb_rel.relspec == TCB_RTREE)
	    {
    	       DM_TID		rtree_tid;
	       ADF_CB 		*adf_cb;
	       DMP_RANGE	rrec;
    	       DB_TAB_ID	tableid;
	       DB_TAB_TIMESTAMP	timestamp;
	       i4		i;

	       /*
	       **  Read the iirange record for this RTree index
	       **  Populate the tcb with the dimension, range and klv
	       */

	       if (rtree_rcb == 0)
	       {
	         /*
	         ** Open the iirange catalog.
	         */
	         tableid.db_tab_base = DM_B_RANGE_TAB_ID;
	         tableid.db_tab_index= DM_I_RANGE_TAB_ID;
	         status = dm2t_open(dcb, &tableid, DM2T_IS, DM2T_UDIRECT,
			DM2T_A_READ, (i4)0, (i4)20, (i4)0,
			log_id, lock_id, (i4)0, (i4)0, DM2T_IS,
			tran_id, &timestamp, &rtree_rcb, (DML_SCB *)NULL, 
			dberr);
	         if (status != E_DB_OK)
		   break;

	         rtree_rcb->rcb_lk_mode = RCB_K_IS;
	         rtree_rcb->rcb_access_mode = RCB_A_READ;
	       }

	       /* Build the key for positioning the table. */

	       key_desc[0].attr_operator = DM2R_EQ;
	       key_desc[0].attr_number = DM_1_RELATION_KEY;
	       key_desc[0].attr_value = (char *)
		 &t->tcb_rel.reltid.db_tab_base;
	       key_desc[1].attr_operator = DM2R_EQ;
	       key_desc[1].attr_number = DM_2_ATTRIBUTE_KEY;
	       key_desc[1].attr_value = (char *)
		 &t->tcb_rel.reltid.db_tab_index;

	       adf_cb = rtree_rcb->rcb_adf_cb;

	       status = dm2r_position(rtree_rcb, DM2R_QUAL, key_desc,
	    	(i4)2, (DM_TID *)NULL, dberr);
	       if (status != E_DB_OK)
	         break;

	       status = dm1c_getklv(adf_cb,
			base_tcb->tcb_atts_ptr[index.idom[0]].type,
			t->tcb_acc_klv);
	       if (status)
	       {
		    SETDBERR(dberr, 0, E_DM9332_DM2T_IDX_DOMAIN_ERROR);
	          status = E_DB_ERROR;
	          break;
	       }

	       for (;;)
	       {
	          /*
	          ** Get the next iirange record.
	          ** This read loop is expected to be terminated by a NONEXT
	          */
	          status = dm2r_get(rtree_rcb, &rtree_tid, DM2R_GETNEXT,
		   	(char *)&rrec, dberr);
	          if (status != E_DB_OK)
	          {
		     if (dberr->err_code == E_DM0055_NONEXT)
		        status = E_DB_OK;
		     break;
	          }
	          else
	          {
	             if (rrec.range_baseid ==
		           t->tcb_rel.reltid.db_tab_base &&
	                 rrec.range_indexid ==
		           t->tcb_rel.reltid.db_tab_index)
	             {
		        t->tcb_dimension = rrec.range_dimension;
		        t->tcb_hilbertsize = rrec.range_hilbertsize;

			t->tcb_kperleaf = dm1m_kperpage(t->tcb_rel.relpgtype,
					      t->tcb_rel.relpgsize,
					      t->tcb_rel.relcmptlvl, 0,
					      t->tcb_klen - t->tcb_hilbertsize,
					      DM1B_PLEAF);

		        t->tcb_rangedt = (DB_DT_ID)rrec.range_datatype;
		        if (rrec.range_type == 'F')
		        {
		           f8	*fll, *fur;
		           t->tcb_rangesize = sizeof(f8) *
						rrec.range_dimension * 2;
		           fll = &t->tcb_range[0];
		           fur = &t->tcb_range[t->tcb_dimension];
		           for (i = 0; i < DB_MAXRTREE_DIM; i++, fll++, fur++)
		           {
		               if ( i < t->tcb_dimension)
		               {
		                  *fll = rrec.range_ll[i];
		                  *fur = rrec.range_ur[i];
				  continue;
		               }
                               else if (i == t->tcb_dimension)
                               {
                                  fll = fur;
                                  fur = fur + i;
		               }
			       
			       *fll = 0.0;
			       *fur = 0.0;
		           }
		        }
		        else
		        {
		           i4	*irange = (i4 *)t->tcb_range;
		           i4	*ill, *iur;

		           t->tcb_rangesize = sizeof(i4) *
					rrec.range_dimension * 2;
		           ill = &irange[0];
		           iur = &irange[t->tcb_dimension];

		           for (i = 0; i < DB_MAXRTREE_DIM; i++, ill++, iur++)
		           {
		             if ( i < t->tcb_dimension)
		             {
		                *ill = (i4)rrec.range_ll[i];
		                *iur = (i4)rrec.range_ur[i];
                                continue;
		             }
                             else if (i == t->tcb_dimension)
                             {
                                ill = iur;
                                iur = iur + i;
		             }
                             *ill = 0;
                             *iur = 0;
	   	          }
		       }
                  }
                  else
                    continue;

	           /* RTree indexes are the size of NBR */
		   if ((index.idom[0]) &&
		       ((index.idom[0] > base_tcb->tcb_rel.relatts) ||
		       (t->tcb_atts_ptr[1].length != t->tcb_hilbertsize *
				t->tcb_dimension) ||
		       (t->tcb_atts_ptr[1].type != t->tcb_acc_klv->nbr_dt_id)) )
		   {
		     /* XXXX Add better error message */
		    SETDBERR(dberr, 0, E_DM9332_DM2T_IDX_DOMAIN_ERROR);
		     status = E_DB_ERROR;
		     break;
		   }

		   t->tcb_ikey_map[0] = index.idom[0];
	         }
	      }

	      status = dm2t_close(rtree_rcb, 0, dberr);
	      if (status != E_DB_OK)
	        break;
	      rtree_rcb = 0;

	    } /* rtree index */
	    else
	    {
	      /*
	      ** Note for indexes on clustered tables, there may
	      ** be up to DB_MAXKEYS (32) user-defined attributes
	      ** -plus- up to DB_MAXKEYS cluster attributes.
	      **
	      ** This is defined as DB_MAXIXATTS:
	      */
	      for (j = 0; j < DB_MAXIXATTS; j++)
	      {
		if ((index.idom[j]) &&
		    ((index.idom[j] > base_tcb->tcb_rel.relatts) ||
		     (t->tcb_atts_ptr[j + 1].length !=
			base_tcb->tcb_atts_ptr[index.idom[j]].length) ||
		     (t->tcb_atts_ptr[j + 1].type !=
			base_tcb->tcb_atts_ptr[index.idom[j]].type)))
		{
		    /* XXXX Add better error message */
		    SETDBERR(dberr, 0, E_DM9332_DM2T_IDX_DOMAIN_ERROR);
		    status = E_DB_ERROR;
		    break;
		}
		t->tcb_ikey_map[j] = index.idom[j];
	      }
	      t->tcb_range = NULL;
	      t->tcb_acc_klv = NULL;
	    }
	    if (status != E_DB_OK)
	        break;

	    /* 
	    ** Note in the base TCB that there are N
	    ** unique indexes.
	    */
	    if (t->tcb_rel.relstat & TCB_UNIQUE)
	    {
		base_tcb->tcb_unique_index = 1;
		base_tcb->tcb_unique_count++;
	    }

	    /* ...and if there are any STATEMENT_LEVEL_UNIQUE indexes */
	    if ( t->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE )
		base_tcb->tcb_stmt_unique = TRUE;

	    t->tcb_tperpage = DMPP_TPERPAGE_MACRO(t->tcb_rel.relpgtype,
		    t->tcb_rel.relpgsize,t->tcb_rel.relwid);

	    /*
	    ** Copy the base tcb's valid stamp into the secondary index
	    ** so it can be used in fault_page without tracing back to
	    ** the parent tcb.
	    */
	    t->tcb_table_io.tbio_cache_valid_stamp =
		base_tcb->tcb_table_io.tbio_cache_valid_stamp;
	    
	    /* 
	    ** Propagate the base tcb's cache priority to the index tcb
	    ** unless the index has an explicit cache priority of its own.
	    */
	    if (t->tcb_table_io.tbio_cache_priority == DM0P_NOPRIORITY)
		t->tcb_table_io.tbio_cache_priority = 
		    base_tcb->tcb_table_io.tbio_cache_priority;

	    /*
	    ** Remove index from tcb list and queue to base TCB.
            ** Note that the base TCB isn't on the main hash chain yet,
            ** so it's OK to be doing this without holding a hash mutex.
	    */
	    *prev_tcb_ptr = t->tcb_q_next;
	    t->tcb_q_next = base_tcb->tcb_iq_next;
	    t->tcb_q_prev = (DMP_TCB*)&base_tcb->tcb_iq_next;
	    t->tcb_parent_tcb_ptr = base_tcb;
	    base_tcb->tcb_iq_next->tcb_q_prev = t;
	    base_tcb->tcb_iq_next = t;
	    /* If base TCB is show-only, index must be also.  This may
	    ** involve closing out opened files, in the case where we
	    ** have the cache for the index pagesize but not the base
	    ** table pagesize (and we found the index first).
	    */
	    if (base_tcb->tcb_status & TCB_SHOWONLY
	      && (t->tcb_status & TCB_SHOWONLY) == 0)
	    {
		t->tcb_status |= TCB_SHOWONLY;
		if (t->tcb_table_io.tbio_status & TBIO_OPEN)
		{
		    local_status = dm2f_release_fcb(lock_id, log_id,
			    t->tcb_table_io.tbio_location_array,
			    t->tcb_table_io.tbio_loc_count, 0,
			    &local_dberr);
		}
		t->tcb_table_io.tbio_status &= ~TBIO_OPEN;
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** Since we've been taking tcb's off of the supplied index_list
	** as they were processed, the index list should now be empty.
	** If it is not then there were iirelation records describing
	** indexes not listed in the iiindex catalog.
	*/
	if (*index_list)
	{
	    /*
	    ** It is possible during some rollforward situations to have
	    ** to open a table while its index information is not yet
	    ** rolled forward and to get an index<->iirelation mismatch.
	    **
	    ** This occurs because the CREATE operation which creates the
	    ** secondary index table is rolled forward before the INDEX
	    ** operation which update the base table index information.
	    **
	    ** If we are in rollforward the excess index tcbs are silently
	    ** deallocated.
	    */
            if ((base_tcb->tcb_dcb_ptr->dcb_status & DCB_S_ROLLFORWARD) ||
		(parallel_idx))
	    {
		while (*index_list)
		{
		    t = *index_list;
		    *index_list = t->tcb_q_next;
		    if (t->tcb_table_io.tbio_status & TBIO_OPEN)
		    {
			local_status = dm2f_release_fcb(lock_id, log_id,
			    t->tcb_table_io.tbio_location_array,
			    t->tcb_table_io.tbio_loc_count, 0,
			    &local_dberr);
		    }
		    /* TCB mutexes must be destroyed */
		    dm0s_mrelease(&t->tcb_mutex);
		    dm0s_mrelease(&t->tcb_et_mutex);
		    dm0m_deallocate((DM_OBJECT **)&t);
		}
	    }
	    else
	    {
		/* XXXX Add better error message */
		SETDBERR(dberr, 0, E_DM9333_DM2T_EXTRA_IDX);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Clean up iiindex rcb.
	*/
	status = dm2r_release_rcb(&idx_rcb, dberr);
	if (status != E_DB_OK)
	    break;

	if (rtree_rcb)
	{
	  /*
	  ** Clean up iirange rcb.
	  */
	  status = dm2t_close(rtree_rcb, 0, dberr);
	  if (status != E_DB_OK)
	    break;
	  rtree_rcb = 0;
	}

	return (E_DB_OK);
    }

    /*
    ** Error cleanup
    */
    if (idx_rcb)
    {
	local_status = dm2r_release_rcb(&idx_rcb, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    if (rtree_rcb)
    {
	local_status = dm2t_close(rtree_rcb, 0, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	SETDBERR(dberr, 0, E_DM9C5A_DM2T_BUILD_INDX);
    }

    return (E_DB_ERROR);
}

/*{
** Name: verify_tcb	- Verify validity of TCB.
**
** Description:
**      This routine verifies that the TCB in memory is still consistent
**      with the TCB on disk that could have been changed by other instances
**      of DMF.
**
**	It uses TCB cache locks to do this.  When a TCB is built, a cache
**	lock is obtained on the tableid and the value of that lock is recorded
**	inside the table control block (tcb_table_io.tbio_cache_valid_stamp).
**
**	Whenever a TCB is purged as part of a table modifying operation the
**	cache lock value is incremented.
**
**	This routine requests the cache lock value and verifies that it is
**	the same as the value stored in the TCB.  If not, then this TCB
**	cannot be used as it may not match the current structure/location
**	or layout of the table it describes.
**
**	The hash mutex must be held upon entry, and is held throughout.
**
** Inputs:
**      tcb                             The TCB to verify.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK			TCB is valid
**	    E_DB_INFO			TCB is not valid
**	    E_DB_ERROR			Error validating the TCB
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1986 (derek)
**          Created new for jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          DMCM protocol. TCB is locked at LK_S.
**	01-aug-1996 (nanpr01)
**	    We donot take tcb cache locks on secondary indexes.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
*/
static DB_STATUS
verify_tcb(
DMP_TCB		*tcb,
DB_ERROR	*dberr)
{
    LK_VALUE    lockvalue;
    CL_ERR_DESC  sys_err;
    STATUS	stat;
    i4		lock_mode;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_VERIFY_TCB (%~t,%~t)\n",
	sizeof(tcb->tcb_dcb_ptr->dcb_name), &tcb->tcb_dcb_ptr->dcb_name,
	sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid);
#endif

    dmf_svcb->svcb_stat.tbl_validate++;

    /* Get the lock value see if it is the same. */

    if (tcb->tcb_dcb_ptr->dcb_status & DCB_S_DMCM)
    {
	/* Don't know, yet */
	tcb->tcb_status &= ~TCB_VALIDATED;

	lock_mode = LK_S;

        stat = LKrequestWithCallback( LK_PHYSICAL | LK_NODEADLOCK | LK_CONVERT,
                        dmf_svcb->svcb_lock_list, (LK_LOCK_KEY *)NULL, lock_mode,
                        (LK_VALUE *)&lockvalue, tcb->tcb_lkid_ptr, 0L,
			(LKcallback) dm2t_tcb_callback, (PTR) tcb, &sys_err);

    }
    else
    {
	lock_mode = LK_N;

        stat = LKrequest( LK_PHYSICAL | LK_NODEADLOCK | LK_CONVERT,
			dmf_svcb->svcb_lock_list, (LK_LOCK_KEY *)NULL, lock_mode,
                        (LK_VALUE * )&lockvalue, tcb->tcb_lkid_ptr, 0L, &sys_err);

    }

    if (stat != OK && stat != LK_VAL_NOTVALID)
    {
	uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
	    0, 0, 0, err_code, 2, 0, lock_mode, 0, dmf_svcb->svcb_lock_list);
	SETDBERR(dberr, 0, E_DM9274_TBL_VERIFY);
	return (E_DB_ERROR);
    }

    if ( stat == LK_VAL_NOTVALID ||
	 tcb->tcb_table_io.tbio_cache_valid_stamp != lockvalue.lk_low )
    {
	dmf_svcb->svcb_stat.tbl_invalid++;
	return (E_DB_INFO);
    }

    /* Mark TCB as validated */
    if ( tcb->tcb_dcb_ptr->dcb_status & DCB_S_DMCM )
	tcb->tcb_status |= TCB_VALIDATED;

    return (E_DB_OK);
}

/*{
** Name: update_rel - Update record and page counts.
**
** Description:
**      This routine updates the record and page counts in the database
**      with the information stored in a TCB.  The TCB is always a
**	base table or partition TCB, never a secondary index TCB.
**
**	As pages and rows are added to a table, the iirelation values
**	relpages and reltups are not directly updated.  The addition of
**	the new pages/rows is recorded in the tcb fields:
**
**		tcb_page_adds		- new pages added to the table
**		tcb_tup_adds		- new rows added to the table
**
**	The values in these fields may be negative if pages or rows were
**	removed from a table.
**
**	This routine looks up the current iirelation row and updates the
**	relpages and reltups columns with tcb_page_adds and tcb_tup_adds.
**
**	Note that the new iirelation relpages may not be the same as the sum
**	(tcb->tcb_rel.relpages + tcb->tcb_page_adds) since the actual
**	iirelation relpages column may differ from the copy inside the tcb
**	(the same is true for reltups).
**
**	The TCB is updated with the new relpages and reltups values.
**
**      If the number of pages in the table has changed significantly from
**      the number of pages that the TCB has recorded, then invalidate the
**      TCB to force all servers to get the latest number of pages.
**
** Inputs:
**      tcb                             The base TCB to update.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-oct-86 (jennifer)
**          Created new for Jupiter.
**	13-feb-89 (rogerk)
**	    Changed to update secondary index page and tuple counts based
**	    on the values in their tcb's, not on the base tcb values.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    17-aug-1989 (Derek)
**		Changed hard-coded attribute-key number to compile-time
**		constants that are different for normal and B1 systems.
**	    14-jun-1991 (Derek)
**		Added trace point macros in xDEBUG trace printing.
**	16-Jan-2004 (schka24)
**	    Update PP array and specific iirelation row if doing a partition.
**	    Use by-tid access, might be many many rows to look through if
**	    partitioned table.
**	17-Feb-2004 (schka24)
**	    Master counts not being done right, fix.
**	27-May-2004 (jenjo02)
**	    If partition master with no indexes, do nothing.
**      09-feb-2005 (horda03) Bug 113860/INGSRV3150
**          If the number of pages in the table has significantly changed,
**          from the number of pages that the server was told was in the
**          table originally, invalidate the TCB to force every server to 
**          reload it.
**	05-Feb-2008 (jonj)
**	    Fix to 09-Feb-2005 "invalidate_tcb". At the end of the loop,
**	    "t" is &tcb->tcb_iq_next (not a TCB); reference "tcb" instead
**	    to avoid bumplock_tcb locking errors, and worse.
**	04-Apr-2008 (jonj)
**	    Better fix to "invalidate_tcb" to resolve DMCM problems -
**	    don't invalidate here, return suggestion to caller.
**	    Don't do it for the catalog, during RCP recovery or rollforward.
**       2-Sep-2008 (hanal04) Bug 116310
**          dm2d_release_user_tcbs() will lead to calls to update_rel()
**          for TCBs that may have been invalidated by activity in
**          another DBMS. If we fail to GET the iirelation record we
**          should not return a failure if the TCB has been invalidated.
**       4-Sep-2008 (hanal04) Bug 116310
**          verify_tcb() demands that the caller hold the hash mutex.
**          The only call to update_rel() drops the mutex around
**          the sys cat update but we are safe to take and release it
**          to verify the TCB on a GET error.
**	02-Apr-2010 (thaju02) Bug 122261
**	    In determining whether to invalidate tcb, the comparision of
**	    tcb relpages to iirel relpages should be '>' rather than '<'.
**	    Added relpages delta constant check.
**	10-may-2010 (stephenb)
**	    reltups and relpages are now i8 on disk (and therefore in the
**	    DMP_RELATION structure, use new CSadjust_i8counter to peform
**	    atomic counter adjustment. Also, restrict actual values to
**	    MAXI4 until we can handle larger numbers.
*/
static DB_STATUS
update_rel(
DMP_TCB             *tcb,
i4		    lock_list,
bool		    *InvalidateTCB,
DB_ERROR	    *dberr)
{
    DB_STATUS		status, tcb_status;
    i4			tcb_err_code;
    DMP_DCB             *dcb;
    DMP_RCB		*rcb;
    DMP_TCB		*t = tcb;
    DMP_TCB		*it;
    DMP_TCB		*master_tcb;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION	relrecord;
    DB_TRAN_ID		tran_id;
    DM_TID		tid;
    DMT_PHYS_PART	*pp_ptr;		/* Physical partition array entry */
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (tcb->tcb_type != TCB_CB)
	dmd_check(E_DM9325_DM2T_UPDATE_REL);
    if (DMZ_TBL_MACRO(10))
    TRdisplay("DM2T_UPDATE_REL (%~t,%~t)\n",
	sizeof(tcb->tcb_dcb_ptr->dcb_name), &tcb->tcb_dcb_ptr->dcb_name,
	sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid);
#endif

    *InvalidateTCB = FALSE;

    /* If Partition Master with no indexes, do nothing */
    if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED &&
	 t->tcb_iq_next == (DMP_TCB*)&t->tcb_iq_next )
    {
	return(E_DB_OK);
    }

    dmf_svcb->svcb_stat.tbl_update++;

    tran_id.db_high_tran = 0;
    tran_id.db_low_tran = 0;

    status = dm2r_rcb_allocate(t->tcb_dcb_ptr->dcb_rel_tcb_ptr, 0,
	&tran_id, lock_list, 0, &rcb, dberr);
    if (status != E_DB_OK)
	return (E_DB_OK);

    rcb->rcb_logging = 0;
    rcb->rcb_journal = 0;
    rcb->rcb_k_duration |= RCB_K_TEMPORARY;

    /* For the given table and all its indexes... */
    do
    {
	/* Don't even bother doing partition masters, as they don't store
	** reltups/relpages.  (We can't completely ignore them, though,
	** because a master might be indexed.)
	** Master counts are managed incrementally by the partitions.
	*/
	if ((t->tcb_rel.relstat & TCB_IS_PARTITIONED) == 0)
	{
	    status = dm2r_get(rcb, &t->tcb_iirel_tid, DM2R_BYTID,
			(char *) &relrecord, dberr);
	    if (status != E_DB_OK)
            {
                dm0s_mlock(t->tcb_hash_mutex_ptr);
                tcb_status = verify_tcb(t, &local_dberr);
                dm0s_munlock(t->tcb_hash_mutex_ptr);
                if(tcb_status == E_DB_INFO)
                {
                    /* The TCB has been invalidated so ignore the get failure */
                    status = E_DB_OK;
                }
		break;
            }

	    /* Double-check that we got the right record, not that I
	    ** don't believe Jon or anything like that...
	    */
	    if (relrecord.reltid.db_tab_base != t->tcb_rel.reltid.db_tab_base
	      || relrecord.reltid.db_tab_index != t->tcb_rel.reltid.db_tab_index)
	    {
		TRdisplay("%@(dm2t)update_rel expected (%d,%d) at %d, got (%d,%d)\n",
		    t->tcb_rel.reltid.db_tab_base, t->tcb_rel.reltid.db_tab_index,
		    t->tcb_iirel_tid.tid_i4,
		    relrecord.reltid.db_tab_base, relrecord.reltid.db_tab_index);
		SETDBERR(dberr, 0, E_DM9272_REL_UPDATE_ERROR);
		status = E_DB_ERROR;
		break;
	    }

	    /* Roll in the deltas from this tcb. */
	    /* Clip the deltas so that tups can't go negative and
	    ** pages can't go below 1.  Work on the deltas, not the result,
	    ** so that everything is adjusted by the same amount.
	    */
	    if (-t->tcb_tup_adds > relrecord.reltups)
		t->tcb_tup_adds = -relrecord.reltups;
	    if (-t->tcb_page_adds > (relrecord.relpages-1))
		t->tcb_page_adds = -(relrecord.relpages-1);

            /* Has the number of pages significantly changed ? */
            dcb = t->tcb_dcb_ptr;

	    /* Don't do this for the catalog, during RCP recovery, rollforward */
	    if ( t->tcb_rel.reltid.db_tab_base > DM_SYSCAT_MAX
	         && !(*InvalidateTCB)
                 && dcb->dcb_served == DCB_MULTIPLE
		 && !(dcb->dcb_status & (DCB_S_RECOVER | DCB_S_ROLLFORWARD))
                 && ((abs(t->tcb_rel.relpages - relrecord.relpages) > 25000) ||
                  ((2 * t->tcb_rel.relpages + 5) > relrecord.relpages)) )
            {
               *InvalidateTCB = TRUE;
            }
	    
	    /* restrict relpages and reltups to MAXI4 */
	    relrecord.reltups = (relrecord.reltups + t->tcb_tup_adds) > MAXI4 ?
		    MAXI4 : relrecord.reltups + t->tcb_tup_adds;
	    relrecord.relpages = (relrecord.relpages + t->tcb_page_adds) > MAXI4 ?
		    MAXI4 : relrecord.relpages + t->tcb_page_adds;

	    t->tcb_rel.reltups = relrecord.reltups;
	    t->tcb_rel.relpages = relrecord.relpages;
	    if (t->tcb_rel.relstat2 & TCB2_PARTITION)
	    {
		/* Updating a partition, roll into pp array too. */
		master_tcb = t->tcb_pmast_tcb;
		pp_ptr = &master_tcb->tcb_pp_array[t->tcb_partno];
		pp_ptr->pp_reltups = (i4)relrecord.reltups;
		pp_ptr->pp_relpages = (i4)relrecord.relpages;
		/* Roll deltas into master, so that master base counts are
		** always the sum of the pp-array counts, and take this
		** partition's deltas out of the master deltas.
		*/
		CSadjust_counter(&master_tcb->tcb_tup_adds, -t->tcb_tup_adds);
		CSadjust_counter(&master_tcb->tcb_page_adds, -t->tcb_page_adds);
		/* restrict relpages and reltups to MAXI4 */
		if (master_tcb->tcb_rel.reltups + t->tcb_tup_adds > MAXI4)
		    t->tcb_tup_adds = 0;
		if (master_tcb->tcb_rel.relpages + t->tcb_page_adds > MAXI4)
		    t->tcb_page_adds = 0;
		CSadjust_i8counter(&master_tcb->tcb_rel.reltups, t->tcb_tup_adds);
		CSadjust_i8counter(&master_tcb->tcb_rel.relpages, t->tcb_page_adds);
	    }
	    t->tcb_tup_adds = 0;
	    t->tcb_page_adds = 0;
	    status = dm2r_replace(rcb, &t->tcb_iirel_tid,
		DM2R_BYTID | DM2R_RETURNTID,
		(char *)&relrecord, (char *)NULL, dberr);
	    if (status != E_DB_OK)
		break;
	} /* if not a partition master */

	/* Move on to the first or next index if any */
	if (t == tcb)
	    t = t->tcb_iq_next;
	else
	    t = t->tcb_q_next;
    } while (t != (DMP_TCB *) &tcb->tcb_iq_next);

    (void) dm2r_release_rcb(&rcb, &local_dberr);

    /*
    ** There is nothing you can do if this did not execute correctly, so
    ** just log warning and  return OK.
    */

    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	uleFormat(NULL, E_DM9273_TBL_UPDATE, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
	CLRDBERR(dberr);
    }
    return (E_DB_OK);
}

/*{
** Name: open_tabio_cb	- 	Open a physical file
**
** Description:
**      This routine opens the underlying database files described by the
**	tableio control block.
**
**	It may be called with a table control block which already has open
**	files.  In this case it will only open those files described in
**	in the location array which are not currently open.
**
**	It may also be called on for a multi-location table which is being
**	partially open.  The caller may mark locations which it does not
**	want or cannot open as LOC_INVALID which will cause the dm2f routines
**	to skip them.
**
**	After calling dm2f to open the indicated locations, this routine
**	scans the location list to see if there are any locations which
**	are not open (by checking for LOC_OPEN status, set by dm2f).  If
**	there are any unopen locations then the tableio cb is set to
**	TBIO_PARTIAL_CB status.
**
**	If any locations are open, a dm2f_sense_file call is made and the
**	tbio_lalloc field is initialized.  Note that when a partially open
**	tbio is being used dm2f_sense is force to make an estimate of the
**	number of pages on the non-open locations.  It assumes that they
**	are the same size (approximately) as the files around them.  If
**	those files are subsequently added to the tcb, this routine will
**	be called again and a new dm2f_sense will be done which will give
**	more accurate results.  The subsequent sense may return LESS pages
**	than the original one!!!
**
** Inputs:
**	tabio				Pointer to Table IO Control Block
**	lock_id				Lock List Id
**	dm2f_flags			Flags to dm2f routines:
**					    DM2F_OPEN - open physical files
**					    DM2F_TEMP - temporary table flag
**					    DM2F_DEFERRED_IO - don't open
**						files until actual IO is needed
**					    DM2F_READONLY - read-only access
**	dbio_flag			Database IO flag:
**					    DCB_NOSYNC_MASK
**
** Outputs:
**      err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-aug-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	20-jun-1994 (jnash)
**	    If tbio->tbio_sync_close requested, open physical files
**	    with DI_USER_SYNC_MASK.  Synchonization will take
**	    place when the file is closed.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to prototype, to be passed to dm2f_build_fcb(),
**	    dm2f_add_fcb().
**	6-Feb-2004 (schka24)
**	    Pass zero id to build-fcb.
*/
static DB_STATUS
open_tabio_cb(
DMP_TABLE_IO	*tabio,
i4         lock_id,
i4         log_id,
i4		dm2f_flags,
i4		dbio_flag,
DB_ERROR	*dberr)
{
    DB_STATUS		status = E_DB_OK;
    u_i4		sync_flag = 0;
    i4		total_pages;
    i4		i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Specify flags to DI subsystem that indicate what sort of rigamarole
    ** we have to go through to make sure that database updates are forced
    ** to permanent storage.
    */
    if ((dbio_flag & DCB_NOSYNC_MASK) == 0)
    {
	if (tabio->tbio_sync_close)
	    sync_flag |= (u_i4) DI_USER_SYNC_MASK;
	else
	    sync_flag |= (u_i4) DI_SYNC_MASK;
    }

    /*
    ** Open physical files for the table and calculate the number of
    ** pages in the table.
    **
    ** If the DM2F_OPEN flag is not passed in, then the files are not
    ** really being opened, but the FCB's are being allocated for a
    ** Deferred-open file.  In this case we cannot sense the file and
    ** leave its size at zero pages.
    **
    ** Note that we may be opening only a subset of the files which
    ** comprise this table (if tbio_status specifies TBIO_PARTIAL_CB, then
    ** the location array will only include entries for some of the files
    ** which make up the table - the rest will be empty.  The dm2f call
    ** will only open those files listed in the location array.)
    */
    for (;;)
    {
	/*
	** If this location array has not yet been initialized, then call
	** dm2f_build_fcb; otherwise call dm2f_add_fcb to add in new location
	** information.
	*/
	if ((tabio->tbio_status & TBIO_OPEN) == 0)
	{
	    status = dm2f_build_fcb(lock_id, log_id, tabio->tbio_location_array,
		tabio->tbio_loc_count, tabio->tbio_page_size,
		dm2f_flags, DM2F_TAB,
		&tabio->tbio_reltid, 0, 0, tabio, sync_flag, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else
	{
	    status = dm2f_add_fcb(lock_id, log_id, tabio->tbio_location_array,
		tabio->tbio_page_size,
		tabio->tbio_loc_count, dm2f_flags, &tabio->tbio_reltid,
		sync_flag, dberr);
	    if (status != E_DB_OK)
		break;
	}

	tabio->tbio_status |= TBIO_OPEN;

	/*
	** Check if the table has only a portion of its physical files opened.
	** If so, mark the table as only Partially Open.
	**
	** Deferred IO tables are marked OPEN even though there is no real
	** open file since they are completely accessable.  Currently there
	** is no support for partially open deferred io tables.
	*/
	tabio->tbio_status &= ~(TBIO_PARTIAL_CB);
	for (i = 0; i < tabio->tbio_loc_count; i++)
	{
	    if (((tabio->tbio_location_array[i].loc_status & LOC_OPEN) == 0) &&
		((dm2f_flags & DM2F_DEFERRED_IO) == 0))
	    {
		tabio->tbio_status |= TBIO_PARTIAL_CB;
	    }
	}

	/*
	** Get count of allocated pages in the table.
	*/
	total_pages = 0;
	if (dm2f_flags & DM2F_OPEN)
	{
	    status = dm2f_sense_file(tabio->tbio_location_array,
		tabio->tbio_loc_count, tabio->tbio_relid, tabio->tbio_dbname,
		&total_pages, dberr);
	    if (status != E_DB_OK)
		break;
	}
	tabio->tbio_lalloc = total_pages;

	break;
    }

    if (status == E_DB_OK)
	return (E_DB_OK);

    /*
    ** Error occurred:
    **     Map File-not-found error to an internal error since it is not
    **     expected at this point.
    */
    if (dberr->err_code == E_DM0114_FILE_NOT_FOUND)
	SETDBERR(dberr, 0, E_DM9291_DM2F_FILENOTFOUND);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	SETDBERR(dberr, 0, E_DM9C5B_DM2T_OPEN_TABIO);
    }

    return (E_DB_ERROR);
}

/*{
** Name: lock_tcb - Request TCB cache lock
**
** Description:
**	Obtains cache lock for TCB and stores the lock value in the tcb.
**	Subsequent fixes of the TCB can then call verify_tcb to ensure that
**	the cached table descriptor is still valid.
**
**	The cache lock is held in NULL mode which actually locks nothing, it
**	just holds a spot in the locking system to store the lock value.
**
**	Either the hash mutex has to be held, or the TCB has to be
**	under construction and not on the hash chains yet.
**
**	The following TCB fields are filled in:
**
**	    tcb_lk_id			- stores the lock id
**	    tcb_lkid_ptr		- points to the tcb_lk_id to use;
**					  indexes use the base table's lk_id;
**					  partitions use the master's lk_id
**	    tcb_table_io.tbio_cache_valid_stamp		- stores the lock value
**
** Inputs:
**	tcb				Table Control Block
**
** Outputs:
**      err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-aug-1992 (rogerk)
**	    Created as part of Reduced logging project.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Implementation of Callbacks on TCB cache locks for the
**          Distributed Multi-Cache Management (DMCM) procotol.
**          TCB locked at LK_S.
**	01-aug-1996 (nanpr01)
**	    We donot take TCB cache locks on secondary indexes.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**	04-Apr-2008 (jonj)
**	    Cleaned up the code to better deal with DMCM issues.
*/
static DB_STATUS
lock_tcb(
DMP_TCB		*tcb,
DB_ERROR	*dberr)
{
    DB_STATUS		status = E_DB_OK;
    LK_LOCK_KEY		lockkey;
    LK_VALUE		lockvalue;
    CL_ERR_DESC		sys_err;
    STATUS		stat;
    i4			lock_mode;
    i4			lock_flags;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Partition table locks are held on the master TCB;
    ** presume it's been locked and copy its lock value
    ** to this partition's tbio.
    ** Only Master TCB's are verified/invalidated.
    */
    if ( tcb->tcb_rel.relstat2 & TCB2_PARTITION )
    {
	tcb->tcb_table_io.tbio_cache_valid_stamp = 
	    tcb->tcb_pmast_tcb->tcb_table_io.tbio_cache_valid_stamp;
    }
    else
    {
	lockkey.lk_type = LK_SV_TABLE;
	lockkey.lk_key1 = tcb->tcb_dcb_ptr->dcb_id;
	lockkey.lk_key2 = tcb->tcb_rel.reltid.db_tab_base;
	lockkey.lk_key3 = 0;
	lockkey.lk_key4 = 0;
	lockkey.lk_key5 = 0;
	lockkey.lk_key6 = 0;

	lock_flags = (tcb->tcb_table_io.tbio_cache_flags & TBIO_CACHE_MULTIPLE)
			    ? LK_PHYSICAL | LK_NODEADLOCK
			    : LK_PHYSICAL | LK_NODEADLOCK | LK_LOCAL;
	/*
	** (ICL phil.p)
	** Support locking strategies for both DMCM and non-DMCM protocols.
	*/
	if (tcb->tcb_dcb_ptr->dcb_status & DCB_S_DMCM)
	{
	    lock_mode = LK_S;

	    if ( tcb->tcb_lkid_ptr->lk_unique )
		lock_flags |= LK_CONVERT;
	    
	    stat = LKrequestWithCallback( lock_flags,
		    dmf_svcb->svcb_lock_list, &lockkey, lock_mode,
		    (LK_VALUE * )&lockvalue, tcb->tcb_lkid_ptr, 0L,
		    (LKcallback) dm2t_tcb_callback, (PTR) tcb, &sys_err);
	}
	else
	{
	    lock_mode = LK_N;

	    stat = LKrequest( lock_flags,
		    dmf_svcb->svcb_lock_list, &lockkey, lock_mode,
		    (LK_VALUE * )&lockvalue, tcb->tcb_lkid_ptr, 0L, &sys_err);
	}

	if (stat != OK && stat != LK_VAL_NOTVALID)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    if (stat == LK_NOLOCKS)
	    {
		if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		     _VOID_ dma_write_audit(
			SXF_E_RESOURCE,
			SXF_A_FAIL | SXF_A_LIMIT,
			(char*)&tcb->tcb_rel.relid,	/* Table name */
			sizeof(tcb->tcb_rel.relid),	/* Table name */
			&tcb->tcb_rel.relowner,
			I_SX2739_LOCK_LIMIT,
			FALSE, /* Not force */
			dberr,
			NULL);

		uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 2,
		    sizeof(DB_TAB_NAME), &tcb->tcb_rel.relid,
		    sizeof(DB_DB_NAME), &tcb->tcb_dcb_ptr->dcb_name);
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    }
	    else
	    {
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG,
		    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 2, 0, lock_mode, 0, dmf_svcb->svcb_lock_list);
		SETDBERR(dberr, 0, E_DM926A_TBL_CACHE_LOCK);
	    }

	    return (E_DB_ERROR);
	}

	/* LKrequest set the lock id in the TCB; save the value */
	tcb->tcb_table_io.tbio_cache_valid_stamp = lockvalue.lk_low;

	/* If DMCM, then mark TCB as validated */
	if ( tcb->tcb_dcb_ptr->dcb_status & DCB_S_DMCM )
	    tcb->tcb_status |= TCB_VALIDATED;
    }

    return (E_DB_OK);
}

/*{
** Name: unlock_tcb - release TCB cache lock
**
** Description:
**      This routine releases a TCB cache lock granted by lock_tcb.
**	It is normally called as the TCB is being released from the table cache.
**
**      Note that we DO NOT clear the "validated" flag from the status.
**      That's because we don't expect the hash mutex to be held at this
**      point, and changing tcb_status without the mutex is not allowed.
**      (It can lead to lost status updates and various problems.)
**      Since the TCB is targeted for disposal anyway, leaving the status
**      unchanged is OK.  We presume that the TCB is BUSY or otherwise
**      protected from deletion by other meddling threads.
**
** Inputs:
**	tcb				Table Control Block
**
** Outputs:
**      err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-aug-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	01-aug-1996 (nanpr01)
**	    TCB Cache locks are only taken for base tcb.
**	14-Aug-2008 (kschendel) b120791
**          Fix race leading to tcb-fixers stuck in LKevent wait:  since
**          it's illegal to update tcb-status without the hash mutex, and
**          since we're not holding the hash mutex here, don't bother with
**          the status update.
*/
static DB_STATUS
unlock_tcb(
DMP_TCB		*tcb,
DB_ERROR	*dberr)
{
    DB_STATUS		status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    STATUS		stat;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Partitions are never cached locked, only Master. */
    if ( !(tcb->tcb_rel.relstat2 & TCB2_PARTITION) )
    {
	stat = LKrelease((i4)0, dmf_svcb->svcb_lock_list, tcb->tcb_lkid_ptr, 
			    0, (LK_VALUE * ) NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, (DB_SQLSTATE*)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)1,
		0, dmf_svcb->svcb_lock_list);
	    SETDBERR(dberr, 0, E_DM926F_TBL_CACHE_CHANGE);
	    status = E_DB_ERROR;
	}
    }

    return (status);
}

/*{
** Name: dm2t_bumplock_tcb 	- Bump the TCB cache lock value.
**
** Description:
**      This routine increments the TCB lock value in order to invalidate
**	cached TCBs in other servers.
**
**	It is normally called when a modify-type operation has been performed
**	on a table which has changed cached table descriptor information.
**	To prevent other servers from running with invalid table descriptions,
**	the cache lock value is changed.  Those other servers are expected
**	to re-validate their TCBs (via verify_tcb) the next time they are
**	referenced.
**
**	This routine may operate without use of a TCB pointer so that operations
**	which do not actually have a TCB, but which change descriptor
**	information, can invalidate cache entries.
**
**	A new version of the cache lock is requested, its value changed and
**	then released.
**
** Inputs:
**	dcb				Database Control Block
**	tcb				TCB, if known.
**	table_id			Pointer to the Table Id, if TCB is null.
**	increment			What to add to the validation lock value,
**					normally 1, but may be zero in DMCM
**					during update_rel() inspired InvalidateTCB.
**
** Outputs:
**      err_code
**
** Outputs:
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-aug-1992 (rogerk)
**	    Created as part of Reduced logging project.
**	24-nov-1992 (jnash)
**	    Renamed dm2t_bumplock_tcb and made not static so that
**	    it can be called by dm2r_load.
**  22-Sep-2004 (fanch01)
**      Remove meaningless LK_TEMPORARY flag from LKrequest call.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**	04-Apr-2008 (jonj)
**	    Eliminated dm2t_xlock_tcb(), dm2t_convert_bumplock_tcb(), 
**	    incorporating their functionalities into this function.
**	22-Apr-2009 (kschendel) SIR 122890
**	    TCB-less bumplock wasn't incrementing the lock value. (?)
*/
DB_STATUS
dm2t_bumplock_tcb(
DMP_DCB		*dcb,
DMP_TCB		*tcb,
DB_TAB_ID	*table_id,
i4		increment,
DB_ERROR	*dberr)
{
    STATUS		stat = OK;
    LK_LKID		lockid, *lockidP;
    LK_LOCK_KEY		lockkey, *lockkeyP;
    LK_VALUE		lockvalue;
    CL_ERR_DESC		sys_err;
    i4			lock_mode;
    i4			lk_flags = LK_PHYSICAL | LK_NODEADLOCK | LK_NONINTR;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** No bumplock if recovery or rollforward, or there's
    ** no lock on the TCB (the core catalogs do not take
    ** cache locks). Don't bumplock partitions, either.
    */
    if ( !(dcb->dcb_status & (DCB_S_RECOVER | DCB_S_ROLLFORWARD))
	&& (!tcb || (tcb->tcb_lkid_ptr->lk_unique &&
		   !(tcb->tcb_rel.relstat2 & TCB2_PARTITION))) )
    {
	if ( tcb )
	{
	    /*
	    ** It is assumed that the TCB is marked BUSY or is hash mutexed
	    ** to prevent any concurrent use of 
	    ** tcb_lk_id_ptr lockid, in particular by
	    ** verify_tcb().
	    */

	    lockidP = tcb->tcb_lkid_ptr;
	    lockkeyP = (LK_LOCK_KEY*)NULL;
	    lk_flags |= LK_CONVERT;
	}
	else
	{
	    if ( dcb->dcb_bm_served == DCB_SINGLE )
		lk_flags |= LK_LOCAL; 

	    lockkey.lk_type = LK_SV_TABLE;
	    lockkey.lk_key1 = dcb->dcb_id;
	    lockkey.lk_key2 = table_id->db_tab_base;
	    lockkey.lk_key3 = 0;
	    lockkey.lk_key4 = 0;
	    lockkey.lk_key5 = 0;
	    lockkey.lk_key6 = 0;
	    lockkeyP = &lockkey;

	    lockid.lk_unique = 0;
	    lockid.lk_common = 0;
	    lockidP = &lockid;
	}


	/*
	** Get/convert LK_SV_TABLE to LK_X 
	**
	** Note we don't set a callback if DMCM
	** as we will immediately downgrade the LK_X to LK_S
	** and we don't want to trigger a dm2t_tcb_callback
	** while we temporarily hold this X lock.
	*/
	stat = LKrequest(lk_flags,
			 dmf_svcb->svcb_lock_list, 
			 lockkeyP, 
			 LK_X, 
			 (LK_VALUE * )&lockvalue,
			 lockidP, 
			 0L, 
			 &sys_err);

	if (stat != OK && stat != LK_VAL_NOTVALID)
	{
	    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)2,
		0, LK_X, 0, dmf_svcb->svcb_lock_list);
	}
	else
	{
	    if ( tcb )
	    {
		/* Down-convert LK_X lock to LK_S if DMCM, LK_N if not */
		if ( dcb->dcb_status & DCB_S_DMCM )
		{
		    lock_mode = LK_S;

		    /* Bump the lock value by "increment" (1 or zero) */
		    lockvalue.lk_low += increment;

		    stat = LKrequestWithCallback(lk_flags,
				     dmf_svcb->svcb_lock_list, 
				     lockkeyP, 
				     lock_mode, 
				     (LK_VALUE * )&lockvalue,
				     lockidP, 
				     0L, 
				     (LKcallback) dm2t_tcb_callback,
				     (PTR)tcb,
				     &sys_err);
		}
		else
		{
		    lock_mode = LK_N;

		    /* Bump the lock value, ignore "increment" */
		    lockvalue.lk_low++;


		    stat = LKrequest(lk_flags,
				     dmf_svcb->svcb_lock_list, 
				     lockkeyP, 
				     lock_mode, 
				     (LK_VALUE * )&lockvalue,
				     lockidP, 
				     0L, 
				     &sys_err);
		}

		if ( stat )
		{
		    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)2,
			0, lock_mode, 0, dmf_svcb->svcb_lock_list);
		}
	    }
	    else
	    {
		/* No TCB, release the SV_TABLE lock */
		/* Bump the lock value, ignore "increment" */
		lockvalue.lk_low++;
		stat = LKrelease(0, dmf_svcb->svcb_lock_list, lockidP, 0,
			(LK_VALUE * )&lockvalue, &sys_err);
		if (stat != OK)
		{
		    uleFormat(NULL, stat, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)1,
			0, dmf_svcb->svcb_lock_list);
		}
	    }
	}
    }

    if ( stat )
	SETDBERR(dberr, 0, E_DM926F_TBL_CACHE_CHANGE);

    return ( (stat) ? E_DB_ERROR : E_DB_OK );
}

/*
** Name: dm2t_reserve_tcb:	    acquire exclusive control of fixed tcb.
**
** Description:
**	This routine takes a fixed TCB, waits for the caller to become the only
**	fixer of that TCB, then marks the TCB busy. This routine is used by
**	the temporary table modify code to ensure that the temporary table is
**	protected during the time that its TCB is rebuilt at the end of the
**	modify.
**
** Inputs:
**	tcb			- tcb to reserve (must be fixed)
**	dcb			- dcb for this tcb (for error messages)
**	wait_reason		- reason for reserving. Currently supported:
**				    DM2T_TMPMOD_WAIT
**
** Outputs:
**	err_code		- reason for error reserving TCB.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	18-jan-1993 (bryanp)
**	    Created to fix temporary table modify problems, such as concurrent
**	    buffer manager access to temporary table pages while the temp table
**	    tcb is being rebuilt.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
*/
DB_STATUS
dm2t_reserve_tcb(
DMP_TCB	    *tcb,
DMP_DCB	    *dcb,
i4	    wait_reason,
i4	    lock_list,
DB_ERROR    *dberr)
{
    i4	tcb_wait_status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    switch (wait_reason)
    {
	case DM2T_TMPMOD_WAIT:
	    tcb_wait_status = TCB_TMPMOD_WAIT;
	    break;

	default:
	    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	    return (E_DB_ERROR);
    }

    dm0s_mlock(tcb->tcb_hash_mutex_ptr);
    while (tcb->tcb_ref_count > 1)
    {
	/*
	** Register reason for waiting - also do consistency check
	** to make sure that two callers are not waiting on
	** the same tcb.
	*/
	if (tcb->tcb_status & tcb_wait_status)
	{
	    dm0s_munlock(tcb->tcb_hash_mutex_ptr);
	    uleFormat(NULL, E_DM9C5F_DM2T_PURGEBUSY_WARN, (CL_ERR_DESC *)NULL,
		ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0,
		(i4 *)NULL, err_code, (i4)6,
		sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		sizeof(DB_TAB_NAME), tcb->tcb_rel.relid.db_tab_name,
		sizeof(DB_OWN_NAME), tcb->tcb_rel.relowner.db_own_name,
		0, tcb->tcb_valid_count, 0, tcb->tcb_ref_count,
		0, tcb->tcb_status);
	    SETDBERR(dberr, 0, E_DM9C53_DM2T_UNFIX_TCB);
	    return (E_DB_ERROR);
	}

	tcb->tcb_status |= tcb_wait_status;

	dm2t_wait_for_tcb(tcb, lock_list);

	/* the wait call returns without the hash mutex: */

        /* Referencing the TCB after a wait-for is normally illegal, but in
        ** this specific case we know that it's a temporary build TCB that
        ** no other session is going to dispose of.  (If we're waiting for
        ** anyone, it would be a cache agent type thread.)
        */

	dm0s_mlock(tcb->tcb_hash_mutex_ptr);
    }
    tcb->tcb_status |= TCB_BUSY;
    dm0s_munlock(tcb->tcb_hash_mutex_ptr);

    return (E_DB_OK);
}

/*{
** Name: dm2t_lookup_tabname	- Find Table Name/Owner in tcb cache given
**				  tblid
**
** Description:
**      This routine searchs the in memory TCB's for table described by
**      the table id passed in.  If if is found, the table name/owner
**	of the table is returned.  Otherwise, non-existent table is returned.
**
**	The current subroutine does not search 2-ary index TCB's. Currently,
**	user-visible temporary tables never have 2-ary index TCB's.
**
** Inputs:
**      dcb                             The database to search.
**	table_id			The table ID
**
** Outputs:
**	table_name			The table name if found
**	owner_name			The owner name if found
**	err_code			Set if E_DB_ERROR is returned, to:
**					E_DM0054_NONEXISTENT_TABLE
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			Non-existent table.
**	Exceptions:
**	    none
**
** History:
**	9-nov-93 (robf)
**	    Created for security auditing by name
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry,
**	    Added hcb_tq_mutex to protect TCB free queue.
**	16-Jan-2004 (schka24)
**	    Use locate-tcb instead of duplicating it.
*/
DB_STATUS
dm2t_lookup_tabname(
DMP_DCB		*dcb,
DB_TAB_ID       *table_id,
DB_TAB_NAME     *table_name,
DB_OWN_NAME     *owner_name,
DB_ERROR	*dberr)
{
    DMP_HASH_ENTRY      *h;
    DMP_TCB		*t;
    DM_MUTEX	*hash_mutex;
    DB_STATUS		status;

    CLRDBERR(dberr);

    /*
    ** Assume table is not found. We'll change our minds if we find it.
    */
    status = E_DB_ERROR;

    /*	Search HCB for this table. */
    hash_mutex = NULL;
    locate_tcb(dcb->dcb_id, table_id, &t, &h, &hash_mutex);
    if (t != NULL
      && (t->tcb_status & TCB_VALID)
      && (t->tcb_status & TCB_BUSY) == 0)
    {
	status = E_DB_OK;
	STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, *table_name);
	STRUCT_ASSIGN_MACRO(t->tcb_rel.relowner, *owner_name);
    }
    dm0s_munlock(hash_mutex);

    if (status != E_DB_OK)
	SETDBERR(dberr, 0, E_DM0054_NONEXISTENT_TABLE);

    return (status);
}




/*{
** Name: dm2t_get_timeout - obtain user supplied lock timeout, if any.
**
** Description:
**	Check the session's "set lockmode" list for timeout values on
**	the table specified, or the session - if none specified for table.
**
** Inputs:
**	scb		- session control block
**	Table_id        - table identifier.
** Outputs:
**      Returns:        - New timeout value if caller supplied value
**			   is found in the session lock option list.
**
** History:
**	10-nov-1994 (cohmi01)
**	    Written to allow timeout values on various DMU utils.
**	19-oct-95 (nick)
**	    We would allow the preset defaults to trash the timeout
**	    value ( these are the -ve values Mike was trying to avoid
**	    returning ).  Exact table match also requires a match on
**	    reltidx as well as reltid.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Pick up user supplied session level timeout value from
**          the SCB, and table level value from the SLCB.
**      09-apr-97 (cohmi01)
**          Allow timeout system default to be configured. #74045, #8616.
**      16-may-97 (dilma04)
**          Removed unneeded check that did not allow user to set default 
**          timeout on a table, if session level timeout was not default. 
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
**
*/
i4
dm2t_get_timeout(
DML_SCB		*scb,
DB_TAB_ID	*table_id)
{
    DML_SLCB	    *slcb;
    i4	    timeout = DMC_C_SYSTEM;    /* assume default value */

    /*
    ** Pick up user supplied session level timeout value.
    */
    timeout = scb->scb_timeout;

    /*
    ** Loop through the slcb list looking for lockmode parameters
    ** that apply to this table.
    ** If a lockmode value is SESSION, just leave the lockmode
    ** value alone and assume it was set to the right thing when
    ** the session level settings were processed.
    */

    for (slcb = scb->scb_kq_next;
        slcb != (DML_SLCB*) &scb->scb_kq_next;
        slcb = slcb->slcb_q_next)
    {
        /* Check if this SLCB applies to the current table. */
        if ( slcb->slcb_db_tab_base == table_id->db_tab_base )
        {
            if (slcb->slcb_timeout != DMC_C_SESSION) 
                timeout = slcb->slcb_timeout;
	    break;
        }
    }

    if (timeout == DMC_C_SYSTEM)
        timeout = dmf_svcb->svcb_lk_waitlock;  /* default system timeout */ 

    return (timeout);
}

/*
** Name: get_supp_tabid - get table ID of replicator support tables given
**			  table name
**
** Description:
**	Use dm2 low level dmf routines to get the table ID of replicator
**	support tables given their name (using iirelation, iirel_idx).
**	Table IDs may then be used to open the tables
**
** Inputs:
**	dcb			Database control block
**	shad_name		Name of the shadow table.
**	arch_name		Name of the archive table
**	shadix_name		Name of the shadow table seconary index
**	prio_lookup_name	Name of priority lookup table
**	cdds_lookup_name	Name of cdds lookup tables]
**	table_owner		Owner of the tables.
**	tran_id			ID of current tx
**	lock_id			ID of lock list
**	log_id			Log ID of tran
**
** Outputs:
**	shad_id			ID of the shadow table.
**	arch_id			ID of the archive table
**	shadix_id		ID of the shadow table seconadry index
**	prio_id			ID of priority lookup table
**	cdds_id			ID of cdds lookup table
**	err_code		error code.
**
** History:
**	25-apr-94 (stephenb)
**	    Initial creation.
**	24-apr-96 (stephenb)
**	    Modified for use in the DBMS
**	21-aug-96 (stephenb)
**	    Add code to find cdds lookup table
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Do not need to turn off isolation level flags.
**	05-May-05 (wanfr01)
**	    Bug 114305, INGSRV3251
**	    Print better error messages so the user can diagnose
**	    E_DM0055 and E_QE0018.
**
*/
static DB_STATUS
get_supp_tabid(	DMP_DCB		*dcb,
		DB_TAB_NAME	*shad_name,
		DB_TAB_NAME	*arch_name,
		DB_TAB_NAME	*shadix_name,
		DB_TAB_NAME	*prio_lookup_name,
		DB_TAB_NAME	*cdds_lookup_name,
		DB_OWN_NAME	*table_owner,
		DB_TAB_ID	*shad_id,
		DB_TAB_ID	*arch_id,
		DB_TAB_ID	*shadix_id,
		DB_TAB_ID	*prio_id,
		DB_TAB_ID	*cdds_id,
		DB_TRAN_ID	*tran_id,
		i4		lock_id,
		i4		log_id,
		DB_ERROR	*dberr)
{
    DB_STATUS		status;
    DMP_RCB		*r_idx = NULL;
    DMP_RCB             *rel = NULL;
    DM2R_KEY_DESC	key_desc[2];
    DM_TID              tid;
    DM_TID              rel_tid;
    DMP_RELATION        relation;
    DMP_RINDEX          rindexrecord;
    DB_STATUS		local_stat;
    char		check = 0;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for(;;)
    {
	/*
	** allocate RCB for iirelation and iirel_idx
	*/
	status = dm2r_rcb_allocate(dcb->dcb_rel_tcb_ptr, (DMP_RCB *)NULL,
	    tran_id, lock_id, log_id, &rel, dberr);
	if (status != E_DB_OK)
	{
	    check = 1;
	    break;
	}
	status = dm2r_rcb_allocate(dcb->dcb_relidx_tcb_ptr, (DMP_RCB *)NULL,
	    tran_id, lock_id, log_id, &r_idx, dberr);
	if (status != E_DB_OK)
	{
	    check = 2;
	    break;
	}
        /*
        ** force read actions to avoid deadlocks
        */
        rel->rcb_k_mode = RCB_K_IS;
        rel->rcb_access_mode = RCB_A_READ;
        r_idx->rcb_k_mode = RCB_K_IS;
        r_idx->rcb_access_mode = RCB_A_READ;
	/*
	** find iirelation entries using iirel_idx
	*/
	/* shadow table */
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELIDX_KEY;
	key_desc[0].attr_value = shad_name->db_tab_name;
	key_desc[1].attr_operator = DM2R_EQ;
	key_desc[1].attr_number = DM_2_RELIDX_KEY;
	key_desc[1].attr_value = table_owner->db_own_name;

	status = dm2r_position(r_idx, DM2R_QUAL, key_desc, (i4)2,
	    (DM_TID *)NULL, dberr);
	if (status != E_DB_OK)
	{
	    check = 3;
	    break;
	}

	status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord,
	    dberr);
	if (status == E_DB_OK)
	    rel_tid.tid_i4 = rindexrecord.tidp;
	else
	{
	    check = 4;
	    break;
	}

	/*
	** now use iirelation to get the table id
	*/
	status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	    dberr);
	if (status != E_DB_OK)
	{
	    check = 5;
	    break;
	}

	/*
	** fill in table ID
	*/
	shad_id->db_tab_base = relation.reltid.db_tab_base;
	shad_id->db_tab_index = relation.reltid.db_tab_index;

	/* archive table */
	key_desc[0].attr_value = arch_name->db_tab_name;
	status = dm2r_position(r_idx, DM2R_QUAL, key_desc, (i4)2,
	    (DM_TID *)NULL, dberr);
	if (status != E_DB_OK)
	{
	    check = 6;
	    break;
	}

	status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord,
	    dberr);

	if (status == E_DB_OK)
	    rel_tid.tid_i4 = rindexrecord.tidp;
	else
	{
	    check = 7;
	    break;
	}

	/*
	** now use iirelation to get the table id
	*/
	status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	    dberr);
	if (status != E_DB_OK)
	{
	    check = 8;
	    break;
	}

	/*
	** fill in table ID
	*/
	arch_id->db_tab_base = relation.reltid.db_tab_base;
	arch_id->db_tab_index = relation.reltid.db_tab_index;

	/* shadow index */
	key_desc[0].attr_value = shadix_name->db_tab_name;
	status = dm2r_position(r_idx, DM2R_QUAL, key_desc, (i4)2,
	    (DM_TID *)NULL, dberr);
	if (status != E_DB_OK)
	{
	    check = 9;
	    break;
	}

	status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord,
	    dberr);
	if (status == E_DB_OK)
	    rel_tid.tid_i4 = rindexrecord.tidp;
	else
	{
	    check = 10;
	    break;
	}

	/*
	** now use iirelation to get the table id
	*/
	status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	    dberr);
	if (status != E_DB_OK)
	{
	    check = 11;
	    break;
	}

	/*
	** fill in table ID
	*/
	shadix_id->db_tab_base = relation.reltid.db_tab_base;
	shadix_id->db_tab_index = relation.reltid.db_tab_index;

	/* priority lookup */
	if (prio_lookup_name->db_tab_name[0] != ' ')
	{
	    key_desc[0].attr_value = prio_lookup_name->db_tab_name;
	    status = dm2r_position(r_idx, DM2R_QUAL, key_desc, (i4)2,
	        (DM_TID *)NULL, dberr);
	    if (status != E_DB_OK)
	    {
		check = 12;
	        break;
	    }

	    status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord,
	        dberr);
	    if (status == E_DB_OK)
	        rel_tid.tid_i4 = rindexrecord.tidp;
	    else
	    {
		check = 13;
	        break;
	    }

	    /*
	    ** now use iirelation to get the table id
	    */
	    status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	        dberr);
	    if (status != E_DB_OK)
	    {
		check = 14;
	        break;
	    }

	    /*
	    ** fill in table ID
	    */
	    prio_id->db_tab_base = relation.reltid.db_tab_base;
	    prio_id->db_tab_index = relation.reltid.db_tab_index;
	}
	else
	{
	    prio_id->db_tab_base = 0;
	    prio_id->db_tab_index = 0;
	}
	/* cdds  lookup */
	if (cdds_lookup_name->db_tab_name[0] != ' ')
	{
	    key_desc[0].attr_value = cdds_lookup_name->db_tab_name;
	    status = dm2r_position(r_idx, DM2R_QUAL, key_desc, (i4)2,
	        (DM_TID *)NULL, dberr);
	    if (status != E_DB_OK)
	    {
		check = 15;
	        break;
	    }

	    status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, (char *)&rindexrecord,
	        dberr);
	    if (status == E_DB_OK)
	        rel_tid.tid_i4 = rindexrecord.tidp;
	    else
	    {
		check = 16;
	        break;
	    }

	    /*
	    ** now use iirelation to get the table id
	    */
	    status = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
	        dberr);
	    if (status != E_DB_OK)
	    {
		check = 17;
	        break;
	    }

	    /*
	    ** fill in table ID
	    */
	    cdds_id->db_tab_base = relation.reltid.db_tab_base;
	    cdds_id->db_tab_index = relation.reltid.db_tab_index;
	}
	else
	{
	    cdds_id->db_tab_base = 0;
	    cdds_id->db_tab_index = 0;
	}

	/*
	** cleanup
	*/
	status = dm2r_release_rcb(&rel, dberr);
	if (status != E_DB_OK)
	{
	    check = 18;
	    break;
	}


	status = dm2r_release_rcb(&r_idx, dberr);
	if (status != E_DB_OK)
	{
	    check = 19;
	    break;
	}

	return (E_DB_OK);
    }
    /*
    ** handle errors
    */
    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	(i4)0, (i4 *)NULL, err_code, 0);

    if (r_idx)
	local_stat = dm2r_release_rcb(&r_idx, &local_dberr);

    if (rel)
	local_stat = dm2r_release_rcb(&rel, &local_dberr);


    TRdisplay ("Warning %d (sts = %d) for database %s: \n",check,status,dcb->dcb_name.db_db_name);
    TRdisplay ("Warning:  Verify the following replicator tables: \n");
    TRdisplay ("   shadow = %s\n",shad_name->db_tab_name);
    TRdisplay ("   arch = %s\n",arch_name->db_tab_name);
    TRdisplay ("   shadow idx = %s\n",shadix_name->db_tab_name);


    return (E_DB_ERROR);
}

/*
** Name: dm2t_tcb_callback - Callback routine for DMCM related TCB cache
**                            lock requests.
**
** Description:
**
**
** History:
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Created for support of the Distributed Multi-Cache Management
**          (DMCM) protocol.
**	01-aug-1996 (nanpr01)
**	    Since lockid may not be unique, we have to pass the lock key
**	    as parameter.
**	08-Apr-1997 (jenjo02)
**	    Use new LK_S_SLOCKQ function to silence LK errors if lockid
**	    can't be found, a "normal" condition.
**	    If lockid is stale, return E_DB_WARN so lk_callback can track
**	    statistics.
**	04-Apr-2008 (jonj)
**	    Cleaned up the code, add LK_CALLBACK_TRACE debugging stuff,
**	    don't wait for busy TCB.
*/
DB_STATUS
dm2t_tcb_callback( i4     	lock_mode,
                   PTR    	ptr,
    		   LK_LKID	*lockid,
		   LK_LOCK_KEY  *lock_key)
{
    DMP_TCB	*newtcb;
    DB_STATUS	status;
    i4		page_count;
    LK_LKB_INFO info_block;
    u_i4	length;
    u_i4	context = 0;
    CL_ERR_DESC sys_err;
    i4		lk_type = lock_key->lk_type;
    i4		db_id = lock_key->lk_key1;
    i4		table_id = lock_key->lk_key2;
    DB_TAB_ID	reltid;
    TM_STAMP	tim;
    char	err_buffer[256];
    char	keydispbuf[128];
    PID		myPid;
    DB_ERROR	local_dberr;

    bool	CallbackDebug = FALSE;

    PCpid(&myPid);

    LK_CALLBACK_TRACE( 60, lock_key );


    status = LKshow( LK_S_SLOCKQ, (i4 ) 0, lockid,
			(LK_LOCK_KEY *)NULL, sizeof(info_block),
			(PTR) &info_block, &length, &context, &sys_err );

    /*
    ** Assume that if the LKshow fails, it is because the resource is no
    ** longer locked. In which case, the callback has nothing to do.
    */
    if ( status != OK ||
	 info_block.lkb_key[0] != lk_type ||
	 info_block.lkb_key[1] != db_id   ||
	 info_block.lkb_key[2] != table_id )
    {
	if ( CallbackDebug )
	{
	    TMget_stamp(&tim);
	    TMstamp_str(&tim, err_buffer);
	    STprintf(err_buffer + STlength(err_buffer),
		" %x dm2t_tcb_callback %d: status %x %s",
		    myPid, __LINE__, status,
		    LKkey_to_string(lock_key, keydispbuf));
	    ERlog(err_buffer, STlength(err_buffer), &sys_err);
	}

	LK_CALLBACK_TRACE( 61, lock_key );
	return(E_DB_WARN);
    }

    reltid.db_tab_base = table_id;
    reltid.db_tab_index = 0;

    /* Find and fix the TCB, but don't wait if it's busy */
    status = dm2t_fix_tcb( db_id, &reltid, (DB_TRAN_ID *) 0,
	    dmf_svcb->svcb_lock_list, (i4 ) 0, DM2T_NOBUILD | DM2T_NOWAIT,
	    (DMP_DCB *) 0, &newtcb, &local_dberr);

    if (status != E_DB_OK)
    {
	if ( CallbackDebug )
	{
	    TMget_stamp(&tim);
	    TMstamp_str(&tim, err_buffer);
	    STprintf(err_buffer + STlength(err_buffer),
		" %x dm2t_tcb_callback %d: status %x err %x %s",
		myPid, __LINE__, status, local_dberr.err_code,
		LKkey_to_string((LK_LOCK_KEY*)&info_block.lkb_key[0], keydispbuf));
	    ERlog(err_buffer, STlength(err_buffer), &sys_err);
	}

	if ( status == E_DB_WARN )
	{
	    if ( local_dberr.err_code == W_DM9C50_DM2T_FIX_NOTFOUND )
	    {
		LK_CALLBACK_TRACE( 62, (LK_LOCK_KEY*)&info_block.lkb_key[0] );

		status = E_DB_OK;
		page_count = dm0p_count_pages( db_id, table_id );

		if (page_count > 0)
		{
		    TMget_stamp(&tim);
		    TMstamp_str(&tim, err_buffer);
		    STprintf(err_buffer + STlength(err_buffer),
			" dm2t_tcb_callback %d: NO TCB lock_id %x.%x mode %d lock_key %s, state %d gr %d rq %d pid %x",
			    __LINE__,
			    lockid->lk_unique, lockid->lk_common,
			    lock_mode,
			    LKkey_to_string((LK_LOCK_KEY*)&info_block.lkb_key[0], keydispbuf),
			    info_block.lkb_state,
			    info_block.lkb_grant_mode,
			    info_block.lkb_request_mode,
			    info_block.lkb_pid);
		    ERlog(err_buffer, STlength(err_buffer), &sys_err);

		    dmd_check(E_DM93F2_NO_TCB);
		}
	    }
	    else if ( local_dberr.err_code == W_DM9C51_DM2T_FIX_BUSY )
	    {
		/*
		** TCB is busy. Rather than waiting and maybe silently
		** deadlocking with locks on other nodes,
		** return E_DB_INFO to reschedule this callback.
		*/
		LK_CALLBACK_TRACE( 63, (LK_LOCK_KEY*)&info_block.lkb_key[0] );
		status = E_DB_INFO;
	    }
	    else
	    {
		/* Huh? Not sure what other WARN there could be */
		LK_CALLBACK_TRACE( 64, (LK_LOCK_KEY*)&info_block.lkb_key[0] );
	    }
	}
    }
    else
    {
	/*
	** When there are no more fixers of this TCB
	** have unfix_tcb force release_tcb to flush
	** the cache of all dirty pages.
	*/
	if ( CallbackDebug )
	{
	    TMget_stamp(&tim);
	    TMstamp_str(&tim, err_buffer);
	    STprintf(err_buffer + STlength(err_buffer),
		" %x dm2t_tcb_callback %d: status %x %s",
		myPid, __LINE__, status,
		LKkey_to_string((LK_LOCK_KEY*)&info_block.lkb_key[0], keydispbuf));
	    ERlog(err_buffer, STlength(err_buffer), &sys_err);
	}
	
	LK_CALLBACK_TRACE( 65, (LK_LOCK_KEY*)&info_block.lkb_key[0] );

	status = dm2t_unfix_tcb(&newtcb, (i4) DM2T_FLUSH,
			dmf_svcb->svcb_lock_list, (i4)0, &local_dberr);
    }


    if ( CallbackDebug )
    {
	TMget_stamp(&tim);
	TMstamp_str(&tim, err_buffer);
	STprintf(err_buffer + STlength(err_buffer),
	    " %x dm2t_tcb_callback %d: status %x err_code %x %s",
		myPid, __LINE__, status, local_dberr.err_code,
		LKkey_to_string((LK_LOCK_KEY*)&info_block.lkb_key[0], keydispbuf));
	ERlog(err_buffer, STlength(err_buffer), &sys_err);
    }

    if ( status )
	LK_CALLBACK_TRACE( 66, (LK_LOCK_KEY*)&info_block.lkb_key[0] );

    LK_CALLBACK_TRACE( 69, (LK_LOCK_KEY*)&info_block.lkb_key[0] );
    return(status);
}


/*
** Name: dm2t_convert_tcb_lock - Converts the tcb lock held on the
**                                svcb_lock_list, to LK_N to relieve
**				  DMCM lock contention
**
**			This function is only called in response
**			to a DM2T_FLUSH which is only used by
**			the DMCM dm2t_tcb_callback() function.
**
**	The hash mutex must be held on entry and is held throughout.
**
** Description:
**
**
** Inputs :-
**          *tcb      - pointer to the TCB.
**
** History:
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Created for support of the Distributed Multi-Cache Management
**          (DMCM) protocol.
**	01-aug-1996 (nanpr01)
**	    TCB cache locks are only taken for base TCB's and also it
**	    was assumed in this routine that a TCB cache lock is already
**	    held.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**	04-Apr-2008 (jonj)
**	    Made function static, do nothing for partitions.
**
*/
static DB_STATUS
dm2t_convert_tcb_lock(
DMP_TCB		*tcb,
DB_ERROR	*dberr)
{
    CL_ERR_DESC	sys_err;
    STATUS	cl_status = OK;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* Partitions are not cached locked, only Master */
    if ( !(tcb->tcb_rel.relstat2 & TCB2_PARTITION) &&
	   tcb->tcb_status & TCB_VALIDATED )
    {
	/*
	** Down-converting from LK_S->LK_N will set the lock value
	** if we supply one, so don't.
	*/
	cl_status = LKrequestWithCallback( 
		LK_PHYSICAL | LK_NODEADLOCK | LK_CONVERT | LK_NOSTALL,
		dmf_svcb->svcb_lock_list, (LK_LOCK_KEY *)NULL,
		LK_N, (LK_VALUE * )NULL, tcb->tcb_lkid_ptr, 
		0L, (LKcallback) dm2t_tcb_callback, (PTR) tcb, &sys_err);


	if ( cl_status )
	{
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		err_code, 2, 0, LK_N, 0, dmf_svcb->svcb_lock_list);
	    SETDBERR(dberr, 0, E_DM926A_TBL_CACHE_LOCK);
	}

	/* TCB will need to be revalidated */
	tcb->tcb_status &= ~TCB_VALIDATED;
    }

    return( (cl_status) ? E_DB_ERROR : E_DB_OK );
}



/** (ICL phil.p)
** Added for debugging of TCB Free list corruption.
** If anything wrong is found, it forces a segv - probably ought to use
** it in conjunction with running under a debugger.
** NOTE this is dummied out as a null macro if not xDEBUG!  Don't
** panic over all the debug_tcb_free_list calls you see!
** Call with the_tcb == the tcb about to go on to or just removed from
** the free list (or null if neither applies).
*/

#ifdef xDEBUG
static void debug_tcb_free_list(char *str,DMP_TCB *the_tcb)
{
    DMP_HCB	*hcb = dmf_svcb->svcb_hcb_ptr;
    QUEUE	*start = &hcb->hcb_ftcb_list;
    QUEUE	*posn = hcb->hcb_ftcb_list.q_next;
    DMP_TCB	*tcb;
    int		count = 0;
    int		*fred = NULL;
    bool	loop1 = FALSE;
    bool	loop2 = FALSE;

    TRdisplay("[debug_tcb_free_list] TCB %p %s freelist\n",the_tcb,str);
    if (posn == start)
        TRdisplay("Walk_List. TCB Free list is empty.\n");
    else
    {
        while (posn != start)
        {
	    tcb = (DMP_TCB *)( (char *)(posn) - CL_OFFSETOF(DMP_TCB, tcb_ftcb_list));
            count++;
            if ((tcb == the_tcb) || (tcb->tcb_ascii_id != (i4) TCB_ASCII_ID))
                *fred = 0;
            if ((posn->q_next == posn->q_prev) &&
                (posn->q_next != start))
            {
                loop1 = TRUE;
                break;
            }
            posn = posn->q_next;
        }
    }

    posn = hcb->hcb_ftcb_list.q_prev;
    if (!loop1)
    {
        if (posn == start)
            TRdisplay("Walk_List. TCB Free list is empty. Backwards.\n");
        else
        {
            while (posn != start)
            {
	        tcb = (DMP_TCB *)( (char *)(posn) - CL_OFFSETOF(DMP_TCB, tcb_ftcb_list));
                count++;
		if ((tcb == the_tcb) || (tcb->tcb_ascii_id != (i4) TCB_ASCII_ID))
                    *fred = 0;
                if ((posn->q_next == posn->q_prev) &&
                    (posn->q_next != start))
                {
                    loop2 = TRUE;
                    break;
                }
                posn = posn->q_prev;
            }
        }
    }

    /*
    ** Print List - if looped.
    */
    if (loop1)
    {
        count = 0;
        posn = hcb->hcb_ftcb_list.q_next;
        TRdisplay("\nDump TCB free list via NEXT ptrs.\n\n");
        while (posn != start)
        {

            if (((posn->q_next == posn->q_prev) &&
                (posn->q_next != start)) || (count >= 50))
            {
                break;
            }
            TRdisplay("---------------\n");
            TRdisplay("POSN = %08x\n", posn);
            TRdisplay("Next = %08x\n", posn->q_next);
            TRdisplay("Prev = %08x\n", posn->q_prev);
            TRdisplay("---------------\n\n");
            count++;

            posn = posn->q_next;
        }
        TRflush();
        *fred = 0;
    }
    if (loop2)
    {
        count = 0;
        posn = hcb->hcb_ftcb_list.q_prev;
        TRdisplay("\nDump TCB free list via PREV ptrs.\n\n");
        while (posn != start)
        {

            if (((posn->q_next == posn->q_prev) &&
                (posn->q_prev != start)) || (count >= 50))
            {
                break;
            }
            TRdisplay("---------------\n");
            TRdisplay("POSN = %08x\n", posn);
            TRdisplay("Next = %08x\n", posn->q_next);
            TRdisplay("Prev = %08x\n", posn->q_prev);
            TRdisplay("---------------\n\n");
            count++;

            posn = posn->q_prev;
        }
        TRflush();
        *fred = 0;
    }
} /* debug_tcb_free_list */
#endif

/*
** Name: rep_cat - determins wether the named table is a replicator catalog
**
** Description:
**	This routine uses a static table lookup to determin wether the named
**	table is a replicator catalog. This routine is only a temporary
**	measure and should disapear when the replicator tables realy become
**	proper Ingres catalogs.
**
** Inputs:
**	tabname	- name of table to check
**
** Outputs:
**	none.
**
** Returns:
**	TRUE - if table is a replicator catalog
**	FALSE - otherwhise
**
** History:
**	22-jan-97 (stephenb)
**	    Initial creation.
*/
static bool
rep_catalog(
	char	*tabname)
{
    i4		i;

    /* replicator catalogs must start with dd_ or DD_ depending on case */
    if (MEcmp(tabname, "dd_", 3) && MEcmp(tabname, "DD_", 3))
	return (FALSE);

    for (i=0; Rep_cats[i].namestr != 0; i++)
    {
	if (MEcmp(tabname, Rep_cats[i].namestr, Rep_cats[i].namelen))
	    continue;

	/* tabname matches unpadded string, make sure the rest is blanks */
	if ((STskipblank(tabname + Rep_cats[i].namelen,
		DB_TAB_MAXNAME - Rep_cats[i].namelen)) == NULL)
	    return(TRUE);
    }
    return(FALSE);
}

/*{
** Name: dm2t_alloc_tcb
**
** Description:
**	This function allocates and initializes a TCB to background values.
**
** Inputs:
**      rel                             relation tuple (if any)
**	loc_count			number of locations
**	dcb				Pointer to Database Control Block
**	**tcb				Where to return TCB pointer.
**
**
** Outputs:
**	**tcb				The allocated TCB.
**					If attribute/key storage is allocated,
**					it is returned uninitialized on
**					the assumption that the caller will
**					initialize it.
**					    See tcb_atts_ptr
**					        tcb_data_atts
**					        tcb_att_key_ptr
**					        tcb_key_attS
**					DMP_LOCATION memory is initialized
**					to null values.
**      Returns:
**          DB_STATUS
**
** History:
**	22-Jan-1997 (jenjo02)
**	    Invented to consolidate the multitude of places which
**	    initialize a TCB and also to obviate the need for
**	    allocating TCBs with cleared memory (DM0M_ZERO).
**	24-Feb-1997 (jenjo02 for somsa01)
**	    Cross-integrated 428939 from main.
**	    Added initialization of tbio_extended.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Table priority has been set in relation->reltcpri before calling
**	    this function. It defaults to 0 (no priority) and is decremented
**	    and copied into tbio_cache_priority.
**	    Removed call to dm0p_gtbl_priority.
**	28-Oct-1997 (jenjo02)
**	    Add roundup size to key_atts_size.
**	29-Dec-1997 (merja01/schte01)
**		Altered rtree_size by removing sizeof(i4) + sizeof(f8) and 
**		adding ME_ALIGN_MACRO to DMPP_ACC_KLV.  This insures an 8 byte
**		boundary and resolves an unaligned access violation on 64 bit
**		axp_osf.
**      26-mar-98 (shust01)
**          Reset SVCB_WB_FUTILITY bit in svcb_status when tcb is created. This
**          will have the write-behind threads for this server wake up.
**          Bug 88988.
**      21-apr-98 (stial01)
**          dm2t_alloc_tcb() Set tcb_extended if extension table (B90677)
**	18-aug-1998 (somsa01)
**	    etl_status (which hangs off of the tcb_et_list) needs to be
**	    initialized to zero.
**      09-feb-1999 (stial01)
**          dm2t_alloc_tcb() Init tcb_kperpage = 0
**      12-apr-1999 (stial01)
**          dm2t_alloc_tcb() Changed parameters
**	05-May-2000 (jenjo02)
**	    Save lengths of blank-stripped table and owner names in
**	    tcb_relid_len, tcb_relowner_len.
**	08-Nov-2000 (jenjo02)
**	    Initialize tcb_temp_xccb;
**	05-Mar-2002 (jenjo02)
**	    Mark Sequence Generator (DM_B_SEQ_TAB_ID) TBIO's
**	    as "sysrel" so they can use physical locking.
**	16-Jan-2004 (schka24)
**	    Init new tcb members for partitioning; allocate pp array.
**	13-Apr-2006 (jenjo02)
**	    Allocate space for copy of key DB_ATTS if CLUSTERED
**	    Btree.
**      29-jan-2007 (huazh01)
**          Introduce tcb_reptab bit and initialize it to 0.
**          bug 117355. 
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

DB_STATUS
dm2t_alloc_tcb(
DMP_RELATION    *rel,
i4		loc_count,
DMP_DCB		*dcb,
DMP_TCB		**tcb,
DB_ERROR	*dberr)
{
    DMP_TCB		*t;
    DB_STATUS		status;
    i4			i;
    i4			atts_size = 0;
    i4			key_ixs_size = 0;
    i4			data_atts_size = 0;
    i4			data_cmpcontrol_size = 0;
    i4			bt_key_atts_size = 0;
    i4			bt_ix_atts_size = 0;
    i4			location_size = 0;
    i4			key_ptrs_size = 0;
    i4			key_cmpcontrol_size = 0;
    i4			leaf_cmpcontrol_size = 0;
    i4			rtree_size = 0;
    i4			pp_size = 0;
    i4			attnmsz = 0;
    char		*next_slot;
    char		sem_name[CS_SEM_NAME_LEN + DB_TAB_MAXNAME + 10];
    bool                blobs = FALSE;
    char		*cptr;
    DB_ATTS		*a;

    /*
    ** Compute the sizes of all the extensions to the TCB.
    */
    if (rel)
    {
	if (rel->relstat2 & TCB2_HAS_EXTENSIONS)
	    blobs = TRUE;


	/* Partitions share Master's Attribute information */
	if ( (rel->relstat2 & TCB2_PARTITION) == 0 )
	{
	    if (rel->relatts)
	    {
		atts_size      = (rel->relatts + 1) * sizeof(DB_ATTS);

		/* attribute name size plus null for each attribute */
		if (rel->relattnametot <= 0)
		{
		    TRdisplay("dm2t_alloc_tcb %~t FIXING %d \n", DB_TAB_MAXNAME,
			rel->relid.db_tab_name, rel->relattnametot);
		    attnmsz = rel->relatts * DB_ATT_MAXNAME;
		}
		else
		{
		    attnmsz = rel->relattnametot + rel->relatts; 
		}

		/* Always make sure attnmsz is enough for core catalogs */
		if ((rel->relstat & TCB_CONCUR) && attnmsz <  24*rel->relatts)
		    attnmsz = 24 * rel->relatts;

		attnmsz = DB_ALIGN_MACRO(attnmsz);

		data_atts_size = (rel->relatts * sizeof(DB_ATTS *));
		data_cmpcontrol_size = dm1c_cmpcontrol_size(rel->relcomptype,
			rel->relatts, rel->relversion);
		data_cmpcontrol_size = DB_ALIGN_MACRO(data_cmpcontrol_size);
	    }

	    if (rel->relkeys)
	    {
		key_ixs_size = (rel->relkeys + 1) * sizeof(i2);
		/* Keep things aligned */
		key_ixs_size = DB_ALIGN_MACRO(key_ixs_size);
		key_ptrs_size = (rel->relkeys * sizeof(DB_ATTS *));
		/* Add an amount for rounding up */
		key_ptrs_size += sizeof(PTR);

		/* Key compression is indicated solely by a relstat flag,
		** and is not controllable as to type (at least at present).
		*/
		if (rel->relstat & TCB_INDEX_COMP)
		{
		    i4 keys = rel->relkeys;

		    /* Deal with the special case of an old-style (pre-2.5)
		    ** btree secondary index with key compression.  For these
		    ** old style indexes, index and leaf pages were identical
		    ** and both carried all non-key columns as well.  So for
		    ** that case, we need to allow for all atts, not just all
		    ** keys.
		    */
		    if (rel->relstat & TCB_INDEX
		      && (rel->relcmptlvl == DMF_T0_VERSION ||
			  rel->relcmptlvl == DMF_T1_VERSION ||
			  rel->relcmptlvl == DMF_T2_VERSION ||
			  rel->relcmptlvl == DMF_T3_VERSION ||
			  rel->relcmptlvl == DMF_T4_VERSION) )
			keys = rel->relatts;
		    key_cmpcontrol_size = dm1c_cmpcontrol_size(
				TCB_C_STANDARD, keys,
				rel->relversion);
		    key_cmpcontrol_size = DB_ALIGN_MACRO(key_cmpcontrol_size);
		}

		/*
		** We now distinguish atts/key arrays for Btree LEAF/INDEX pages
		**
		** If this is a btree secondary index having NON-KEY fields, 
		** INDEX entries are different from LEAF entries.
		** The index atts and keys look like the leaf keys,
		** except that the tidp in the index is at a different
		** offset (no intervening non-key columns).  So we need
		** an extra key-atts pointer array that points to a
		** different tidp att entry (different offset/key_offset).
		**
		** Note that there are some cases where the extra key
		** atts pointer array is not needed:  if the tidp is not
		** a key, or if the SI is a pre-Ingres 2.5 table which
		** carried the non-key columns into the index.  However
		** we can't know the former at this point, and it's not
		** worth testing here for the latter.
		**
		** If CLUSTERED Btree, we need separate copies of key DB_ATTS,
		** one for the Leaf pages, and one for the Index pages.
		** The Leaf attributes will have key_offset set to the 
		** natural offset of the key column in the rows; the Index
		** keys are extracted and concatenated together, so their
		** key_offsets will relative to the concatenated key pieces.
		**
		** tcb_key_atts are those of the Leaf,
		** tcb_ixkeys are an array of pointers to the Index
		** key attribute copies in tcb_ixatts_ptr.
		*/
		if ( rel->relspec == TCB_BTREE
		  && ((rel->relstat & TCB_INDEX && rel->relatts != rel->relkeys)
		      || rel->relstat & TCB_CLUSTERED) )
		{
		    /* If key compressed, the leaf will have the same layout
		    ** as the data.  Figure the compression control needed
		    ** for the leaf compression (don't just grab the data
		    ** rowaccessor info, as the data may not be compressed!).
		    */
		    if (rel->relstat & TCB_INDEX_COMP)
		    {
			/* Key compression hardwired to "standard" for now */
			leaf_cmpcontrol_size = dm1c_cmpcontrol_size(
				TCB_C_STANDARD, rel->relatts,
				rel->relversion);
			leaf_cmpcontrol_size = DB_ALIGN_MACRO(leaf_cmpcontrol_size);
		    }
		    if ( rel->relstat & TCB_INDEX && rel->relatts != rel->relkeys )
		    {
			/*
			** Allocate another array for INDEX page keys 
			** plus one more DB_ATT for tidp description
			*/
			bt_key_atts_size = key_ptrs_size + sizeof(DB_ATTS);
		    }
		    else /* clustered primary */
		    {
			/* 
			** Allocate another array for a separate INDEX
			** key array and another set of key DB_ATTS.
			** The leaf will look like data.
			*/
			bt_key_atts_size = key_ptrs_size;
			bt_ix_atts_size = rel->relkeys * sizeof(DB_ATTS);
		    }
		}
	    }
	}

	if (rel->relspec == TCB_RTREE)
	{
	    rtree_size = DB_ALIGNTO_MACRO(sizeof(DMPP_ACC_KLV), sizeof(f8));
	    rtree_size = rtree_size + DB_MAXRTREE_DIM * sizeof(f8) * 2;
	}
	if (rel->relstat & TCB_IS_PARTITIONED)
	{
	    pp_size = sizeof(DMT_PHYS_PART) * rel->relnparts;
	    pp_size = (pp_size + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	}
    }

    location_size = (loc_count * sizeof(DMP_LOCATION));

    /*
    ** Allocate uninitialized storage to contain the TCB
    ** and its appernate structures.
    */
    status = dm0m_allocate((i4)(sizeof(DMP_TCB) +
	atts_size +
	attnmsz +
	key_ixs_size +
	data_atts_size +
	data_cmpcontrol_size +
	key_ptrs_size +
	key_cmpcontrol_size +
	bt_key_atts_size +
	bt_ix_atts_size +
	leaf_cmpcontrol_size +
	location_size +
	rtree_size +
	pp_size +
	sizeof(PTR) +
	(blobs ? sizeof(DMP_ET_LIST) : 0)),
	(i4)0, (i4)TCB_CB, (i4)TCB_ASCII_ID,
	(char *)dcb, (DM_OBJECT **)&t, dberr);

    if (status != E_DB_OK)
    {
	*tcb = (DMP_TCB *)NULL;
	return(status);
    }

    *tcb = t;

    /*
    ** Initialize TCB Control Block pointers and lists.
    */
    next_slot = (char*)t + sizeof(DMP_TCB);

    MEfill(sizeof(DMP_ROWACCESS), 0, (PTR) &t->tcb_data_rac);
    MEfill(sizeof(DMP_ROWACCESS), 0, (PTR) &t->tcb_index_rac);
    MEfill(sizeof(DMP_ROWACCESS), 0, (PTR) &t->tcb_leaf_rac);
    t->tcb_atts_names = NULL;
    t->tcb_atts_size = 0;
    t->tcb_atts_used = 0;
    if (data_atts_size)
    {
	t->tcb_atts_ptr = (DB_ATTS*)next_slot;
	/* Zero-fill the first DB_ATTS, normally unused */
	MEfill(sizeof(DB_ATTS), 0, next_slot);

	next_slot += atts_size;
	t->tcb_data_rac.att_ptrs = (DB_ATTS**)next_slot;
	next_slot += data_atts_size;

	t->tcb_atts_names = next_slot;
	t->tcb_atts_size = attnmsz;
/*
	if (rel->relstat & TCB_CONCUR)
	{
	    char	*nextattname = t->tcb_atts_names;
	    for (i = 0, a = t->tcb_atts_ptr; i <= rel->relatts; i++, a++)
	    {
		a->attnmstr = nextattname;
		nextattname += AVG_NAME_SIZE + 1;
	    }
	}
*/

	next_slot += attnmsz;
	t->tcb_data_rac.att_count = rel->relatts;
    }
    else
    {
	t->tcb_atts_ptr = (DB_ATTS*)NULL;
	t->tcb_atts_names = NULL;
	t->tcb_atts_size = 0;
    }

    t->tcb_leafkeys = (DB_ATTS **)NULL;
    t->tcb_key_atts = (DB_ATTS **)NULL;
    t->tcb_ixklen = 0;
    t->tcb_keys = 0;
    t->tcb_klen = 0;
    t->tcb_kperleaf = 0;
    t->tcb_kperpage = 0;

    t->tcb_ixkeys = (DB_ATTS **)NULL;
    t->tcb_ixatts_ptr = t->tcb_atts_ptr;

    t->tcb_rng_rac = NULL;
    t->tcb_rngkeys = NULL;
    t->tcb_rngklen = 0;

    t->tcb_atts_used = 0;

    if (data_cmpcontrol_size)
    {
	t->tcb_data_rac.cmp_control = (PTR) next_slot;
	next_slot += data_cmpcontrol_size;
	next_slot = ME_ALIGN_MACRO(next_slot, sizeof(PTR));
	/* Save size in bytes until "setup" converts to entries */
	t->tcb_data_rac.control_count = data_cmpcontrol_size;
    }
    if (key_ptrs_size)
    {
	t->tcb_att_key_ptr = (i2 *)next_slot;
	/* Zero fill tcb_attr_key storage */
	MEfill(key_ixs_size, 0, next_slot);
	next_slot += key_ixs_size;
	next_slot = (char *)ME_ALIGN_MACRO(next_slot, sizeof(PTR));
	t->tcb_key_atts = (DB_ATTS**)next_slot;
	next_slot += key_ptrs_size;
	if (bt_key_atts_size)
	{
	    t->tcb_ixkeys = (DB_ATTS **)next_slot;
	    next_slot += bt_key_atts_size;
	}
	if ( bt_ix_atts_size )
	{
	    /* Separate set of INDEX attributes */
	    t->tcb_ixatts_ptr = (DB_ATTS*)next_slot;
	    next_slot += bt_ix_atts_size;
	}
    }
    else
    {
	t->tcb_att_key_ptr = (i2*)NULL;
    }
    if (key_cmpcontrol_size)
    {
	t->tcb_index_rac.cmp_control = (PTR) next_slot;
	next_slot += key_cmpcontrol_size;
	next_slot = ME_ALIGN_MACRO(next_slot, sizeof(PTR));
	/* Save size in bytes until "setup" converts to entries */
	t->tcb_index_rac.control_count = key_cmpcontrol_size;
    }
    if (leaf_cmpcontrol_size)
    {
	t->tcb_leaf_rac.cmp_control = (PTR) next_slot;
	next_slot += leaf_cmpcontrol_size;
	next_slot = ME_ALIGN_MACRO(next_slot, sizeof(PTR));
	/* Save size in bytes until "setup" converts to entries */
	t->tcb_leaf_rac.control_count = leaf_cmpcontrol_size;
    }

    if (loc_count)
    {
	t->tcb_table_io.tbio_location_array =
		(DMP_LOCATION*)next_slot;
	next_slot += location_size;
    }
    else
    {
	t->tcb_table_io.tbio_location_array =
		(DMP_LOCATION*)NULL;
    }

    if (rtree_size)
    {
	t->tcb_acc_klv = (DMPP_ACC_KLV *)next_slot;
	t->tcb_range = (f8 *)((char *)t->tcb_acc_klv + sizeof(DMPP_ACC_KLV));
	t->tcb_range = (f8 *)ME_ALIGN_MACRO(t->tcb_range, sizeof(f8));
	next_slot += rtree_size;
    }
    else
    {
        t->tcb_range = NULL;
        t->tcb_acc_klv = (DMPP_ACC_KLV*)NULL;
    }

    t->tcb_pp_array = NULL;
    if (pp_size != 0)
    {
	t->tcb_pp_array = (DMT_PHYS_PART *) next_slot;
	MEfill(pp_size, 0, next_slot);
	next_slot = next_slot + pp_size;
    }

    t->tcb_et_dmpe_len = 0; /* Segment size for etabs */
    if (blobs)
    {
	/*
	** Initialize extended table list.  Set to empty list.
	*/
	t->tcb_et_list = (DMP_ET_LIST*)next_slot;
	t->tcb_et_list->etl_next = t->tcb_et_list;
	t->tcb_et_list->etl_prev = t->tcb_et_list;
	t->tcb_et_list->etl_status = 0;
	t->tcb_et_list->etl_etab.etab_type = DMP_LH_ETAB;
	t->tcb_et_list->etl_etab.etab_base = 0;
	t->tcb_et_list->etl_etab.etab_extension = 0;
    }
    else
	t->tcb_et_list = (DMP_ET_LIST*)NULL;

    t->tcb_dcb_ptr = dcb;
    t->tcb_unique_id = CSadjust_counter((i4 *) &dmf_svcb->svcb_tcb_id,1);
    /* The two hash ptrs have to be filled in by the caller */
    t->tcb_hash_bucket_ptr = 0;
    t->tcb_hash_mutex_ptr = 0;
    /* Assume base table, assume partitioned master */
    t->tcb_parent_tcb_ptr = t;
    t->tcb_pmast_tcb = t;
    t->tcb_partno = 0;

    t->tcb_q_next  = (DMP_TCB*) &t->tcb_q_next;
    t->tcb_q_prev  = (DMP_TCB*) &t->tcb_q_next;
    t->tcb_iq_next = (DMP_TCB*) &t->tcb_iq_next;
    t->tcb_iq_prev = (DMP_TCB*) &t->tcb_iq_next;
    t->tcb_fq_next = (DMP_RCB*) &t->tcb_fq_next;
    t->tcb_fq_prev = (DMP_RCB*) &t->tcb_fq_next;
    t->tcb_rq_next = (DMP_RCB*) &t->tcb_rq_next;
    t->tcb_rq_prev = (DMP_RCB*) &t->tcb_rq_next;
    t->tcb_ftcb_list.q_next = &t->tcb_ftcb_list;
    t->tcb_ftcb_list.q_prev = &t->tcb_ftcb_list;
    t->tcb_upd_rcbs.q_next = &t->tcb_upd_rcbs;
    t->tcb_upd_rcbs.q_prev = &t->tcb_upd_rcbs;

    t->tcb_status = TCB_VALID;

    t->tcb_temp_xccb = (DML_XCCB*)NULL;
    t->tcb_bits_free = 0;
    t->tcb_temporary = TCB_PERMANENT;
    t->tcb_sysrel = TCB_USER_REL;
    t->tcb_extended = 0;
    t->tcb_loadfile = TCB_NOLOAD;
    t->tcb_logging = TRUE;
    t->tcb_nofile = FALSE;
    t->tcb_update_idx = FALSE;
    t->tcb_no_tids = FALSE;
    t->tcb_dups_on_ovfl = 0;
    t->tcb_rng_has_nonkey = 0;
    t->tcb_seg_rows = 0;
    t->tcb_unique_index = 0;
    t->tcb_replicate = TCB_NOREP;
    t->tcb_ref_count = 0;
    t->tcb_valid_count = 0;
    t->tcb_open_count = 0;
    t->tcb_lct_ptr = 0;
    t->tcb_index_count = 0;
    t->tcb_lk_id.lk_unique = 0;
    t->tcb_lk_id.lk_common = 0;
    t->tcb_lkid_ptr = &t->tcb_lk_id;
    t->tcb_tup_adds = 0;
    t->tcb_page_adds = 0;
    t->tcb_tperpage = 0;
    t->tcb_kperpage = 0;
    t->tcb_kperleaf = 0;
    t->tcb_sec_key_att = 0;
    t->tcb_table_type = 0;
    t->tcb_acc_plv = 0;
    t->tcb_rep_info = 0;
    t->tcb_dimension = 0;
    t->tcb_hilbertsize = 0;
    t->tcb_rangesize = 0;
    t->tcb_stmt_unique = 0;
    t->tcb_unique_count = 0;
    t->tcb_rtb_next = NULL;
    t->tcb_reptab = 0;
    MEfill(sizeof(t->tcb_rangedt), 0, &t->tcb_rangedt);
    MEfill(sizeof(t->tcb_ikey_map), 0, &t->tcb_ikey_map);

    /*
    ** Initialize the table logical key values.
    ** These start at 0, the first time anyone needs to use them they
    ** will be updated from the relation relation, reserving 1024 of
    ** them.  This is guaranteed, since the first check done when
    ** allocating a logical key is to see if it the current low value
    ** is evenly divisiable by TCB_LOGICAL_KEY_INCREMENT, which
    ** is always true of 0.
    */
    t->tcb_high_logical_key = 0;
    t->tcb_low_logical_key = 0;

    /*
    ** Copy the iirelation row into the TCB and fill in tcb fields derived
    ** from the iirelation contents, if any.
    */
    if (rel)
    {
	STRUCT_ASSIGN_MACRO(*rel, t->tcb_rel);
	t->tcb_index_count = rel->relidxcount;
	t->tcb_data_rac.compression_type = rel->relcomptype;
	if (rel->relstat & TCB_INDEX_COMP)
	{
	    t->tcb_index_rac.compression_type = TCB_C_STANDARD;
	    t->tcb_leaf_rac.compression_type = TCB_C_STANDARD;
	}

	/*
	** Compute blank-stripped lengths of table and owner names,
	** save those lengths in the TCB:
	*/
	for ( cptr = (char *)&t->tcb_rel.relid.db_tab_name + DB_TAB_MAXNAME - 1,
		     i = DB_TAB_MAXNAME;
	      cptr >= (char *)&t->tcb_rel.relid.db_tab_name && *cptr == ' ';
	      cptr--, i-- );
	t->tcb_relid_len = i;
	for ( cptr = (char *)&t->tcb_rel.relowner.db_own_name + DB_OWN_MAXNAME - 1,
		     i = DB_OWN_MAXNAME;
	      cptr >= (char *)&t->tcb_rel.relowner.db_own_name && *cptr == ' ';
	      cptr--, i-- );
	t->tcb_relowner_len = i;

	/*
	** Initialize the TCB mutexes
	** CS will truncate the name to CS_SEM_NAME_LEN
	** Since table name may be > CS_SEM_NAME_LEN, 
	** use reltid,reltidx to make the semaphore name unique,
	** plus as much of the table name as we can fit
	*/
	dm0s_minit(&t->tcb_mutex,
		      STprintf(sem_name, "TCB %x %x %*s", 
			    t->tcb_rel.reltid.db_tab_base, 
			    t->tcb_rel.reltid.db_tab_index,
			    t->tcb_relid_len, t->tcb_rel.relid.db_tab_name));
	dm0s_minit(&t->tcb_et_mutex,
		      STprintf(sem_name, "TCB et %x %x %*s",
			    t->tcb_rel.reltid.db_tab_base, 
			    t->tcb_rel.reltid.db_tab_index,
			    t->tcb_relid_len, t->tcb_rel.relid.db_tab_name));

	/*
	** Fill in table type.  This is normally the same as the storage
	** structure, but allows for tables that are not "normal" tables.
	*/
	t->tcb_table_type = rel->relspec;
	if (rel->relstat & TCB_GATEWAY)
	{
	    t->tcb_table_type = TCB_TAB_GATEWAY;

	    /*
	    ** Determine whether the table supports tids or not.  Base Ingres
	    ** tables and some gateway (e.g. RMS) tables support tids .
	    */
	    /*
	    ** NOTE we may want to change this to a gateway call.  For now,
	    ** just switch based on specific gateway.
	    */
	    switch (t->tcb_rel.relgwid)
	    {
	      case GW_RMS:
		t->tcb_no_tids = FALSE;
		break;

	      default:
		t->tcb_no_tids = TRUE;
		break;
	    }
	}

	/*
	** Set system catalog type if this is a SCONCUR table.
	** (iirelation, iiattribute, iiindex and iirel_idx are built at dbopen
	** time in dm2d and have their tcb's initialized there.  This check
	** should currently only occur for iidevices which is a core catalog
	** but which is opened through dm2t_open like all other tables.)
	**
	** iisequence uses Physical locks and so needs to
	** be treated as a 'sysrel' table also. If tcb_sysrel is set then
	** the DM0L_PHYS_LOCK flag is set in the log record which means
	** physical locks are taken during undo (see dm1r)
	** previously this wasn't happening but physical locks were taken
	** during the transaction itself - this could lead to a problem
	** recovering as you may not be able to get the locks during
	** undo
	**
	** test for this is for TCB2_PHYSLOCK_CONCUR which is a new flag
	** assigned to iisequence for tables other than TCB_CONCURs that
	** need physical locks
	** All TCB_CONCUR tables should also have TCB2_PHYSLOCK_CONCUR 
	** but leave that test in case the user didn't run upgradedb
	** (which is what creates the TCB2 flag)
	** (b122651)
	*/
	if (rel->relstat & TCB_CONCUR || 
	    rel->relstat2 & TCB2_PHYSLOCK_CONCUR)
	    t->tcb_sysrel = TCB_SYSTEM_REL;

	/*
	** Set tcb_extended if this is an extension table
	*/
	if (rel->relstat2 & TCB2_EXTENSION)
	    t->tcb_extended = 1;

	/*
	** Set tcb_update_idx according to whether secondary indexes should
	** be updated when base table is changed.
	** Current Gateways do not require secondary indexes to be updated
	** when the base table is modified.
	*/
	if ((t->tcb_rel.relstat & (TCB_IDXD | TCB_GATEWAY)) == TCB_IDXD)
	    t->tcb_update_idx = TRUE;

	/*
	** Fill in logging requirements.  Most tables require logging -
	** exceptions are gateway tables, views and temporary tables.
	**
	** Databases that are journaled or that are opened with Fast
	** Commit require logical log records to be written.
	** Journaling alone does not require that logical log records be
	** written for 2nd indexes.  Fast Commit, however, does.
	*/
	if (t->tcb_rel.relstat & TCB_GATEWAY)
	    t->tcb_logging = FALSE;

	/*
	** If this table is a view or a gateway, then it has no underlying
	** physical file.  Likewise if it's a partitioned master.
	*/
	if (t->tcb_rel.relstat & (TCB_GATEWAY | TCB_VIEW | TCB_IS_PARTITIONED))
	    t->tcb_nofile = TRUE;

	if (rel->reltotwid > dm2u_maxreclen(rel->relpgtype, rel->relpgsize))
	    t->tcb_seg_rows = TRUE;
	else
	    t->tcb_seg_rows = FALSE;

	/*
	** Fill in the page accessor dispatch based on page type.
	*/
	dm1c_get_plv(t->tcb_rel.relpgtype, &t->tcb_acc_plv);
    }
    else
    {
	MEfill(sizeof(t->tcb_rel), 0, &t->tcb_rel);
	t->tcb_rel.relloccount = loc_count;
    }

    /*
    ** Initialize Table IO Control Block.
    */
    t->tcb_table_io.tbio_q_next = t->tcb_table_io.tbio_q_prev = 0;
    t->tcb_table_io.tbio_length = 0;
    t->tcb_table_io.tbio_reserved = 0;
    t->tcb_table_io.tbio_type = TBIO_CB;
    t->tcb_table_io.tbio_status = TBIO_VALID;
    t->tcb_table_io.tbio_bits_free = 0;
    t->tcb_table_io.tbio_lalloc = -1;
    t->tcb_table_io.tbio_lpageno = -1;
    t->tcb_table_io.tbio_tmp_hw_pageno = -1;
    t->tcb_table_io.tbio_checkeof = TRUE;
    t->tcb_table_io.tbio_cache_valid_stamp = 0;
    t->tcb_table_io.tbio_dcbid = dcb->dcb_id;
    t->tcb_table_io.tbio_reltid = t->tcb_rel.reltid;
    t->tcb_table_io.tbio_table_type = t->tcb_table_type;
    t->tcb_table_io.tbio_page_type = t->tcb_rel.relpgtype;
    t->tcb_table_io.tbio_page_size = t->tcb_rel.relpgsize;
    t->tcb_table_io.tbio_cache_ix = DM_CACHE_IX(t->tcb_rel.relpgsize);
    t->tcb_table_io.tbio_temporary = FALSE;
    t->tcb_table_io.tbio_sysrel = t->tcb_sysrel;
    
    t->tcb_table_io.tbio_dbname = &dcb->dcb_name;
    t->tcb_table_io.tbio_relid = &t->tcb_rel.relid;
    t->tcb_table_io.tbio_relowner = &t->tcb_rel.relowner;
    t->tcb_table_io.tbio_loc_count = t->tcb_rel.relloccount;
    t->tcb_table_io.tbio_extended = ((t->tcb_rel.relstat2 & TCB2_EXTENSION) != 0);

    for (i = 0; i < t->tcb_table_io.tbio_loc_count; i++)
    {
	t->tcb_table_io.tbio_location_array[i].loc_status = 0;
	t->tcb_table_io.tbio_location_array[i].loc_id = 0;
	t->tcb_table_io.tbio_location_array[i].loc_config_id = 0;
	t->tcb_table_io.tbio_location_array[i].loc_ext = 0;
	t->tcb_table_io.tbio_location_array[i].loc_fcb = 0;
    }

    /*
    ** Establish sync mode based on database level mode.
    */
    if (dcb->dcb_status & DCB_S_SYNC_CLOSE)
	t->tcb_table_io.tbio_sync_close = TRUE;
    else
	t->tcb_table_io.tbio_sync_close = FALSE;

    t->tcb_table_io.tbio_cache_flags = 0;
    if (dcb->dcb_bm_served == DCB_MULTIPLE)
	t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_MULTIPLE;
    if (dcb->dcb_status & DCB_S_FASTCOMMIT)
	t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_FASTCOMMIT;
    if (dcb->dcb_access_mode == DCB_A_READ)
	t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_READONLY_DB;
    if (dcb->dcb_status & DCB_S_ONLINE_RCP)
	t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_RCP_RECOVERY;
    if (dcb->dcb_status & DCB_S_ROLLFORWARD)
	t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_ROLLDB_RECOVERY;

    /*
    ** (ICL phil.p)
    */
    if (dcb->dcb_status & DCB_S_DMCM)
        t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_DMCM;

    /*
    ** Cache priority for this table was optionally set when
    ** the table was created or modified. It defaults to
    ** 0 (no priority), which is transformed to DM0P_NOPRIORITY here.
    **
    ** tcb_rel.reltcpri can be in the range 0-DB_MAX_TABLEPRI,
    ** but tbio_cache_priority must be 0-based for use 
    ** as an array index, so when we assign it, decrement it by
    ** one.
    **
    ** Table priorities are fixed in the RCP and ROLLFORWARDDB, 
    ** who work best with a flat cache.
    */
    if ( (dcb->dcb_status & (DCB_S_RECOVER | DCB_S_ROLLFORWARD)) ||
	  DMZ_BUF_MACRO(33) )
    {
	t->tcb_table_io.tbio_cache_priority = DB_MAX_TABLEPRI - 2;
    }
    else
    {
	t->tcb_table_io.tbio_cache_priority = t->tcb_rel.reltcpri - 1;
    }

    return(status);
}

/*
** Name:	rep_lockmode
**
** Description:
**	Sets lockmode for replicator support tables
**
**	Logic: This routine will check for the replicator support tables 
**	in the list of tables set in the "set lockmode" statement, and will 
**	use the lockomde set there first, this over-rides all other setttings.
**	It will then use the default lockmode passed in. This is currently
**	set by first checking the value of !.rep_sa_lockmode in the PM file
**	and setting to that, if no valid PM variable is set, the user 
**	lockmode for the base table will determin the locking startegy.
**	
** Inputs:
**	scb		- DMF session control block
**	table_id	- ID of the replicator support table to be checked
**	def_lockmode	- Default lockmode (set as described above)
**
** Outputs:
**	None
**
** Returns:
**	The correct lockmode to use for this table
**
** History:
**	18-nov-97 (stephenb)
**	    Initial creation.
**	12-oct-1999 (gygro01)
**	    Only shared locks need to be taken on the lookup tables,
**	    because they are only accessed for read during data capture.
**	    This resolves some contention issues. Bug 99176 / ingrep 66.
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
*/
static	i4
rep_lockmode(
	DML_SCB		*scb,
	DB_TAB_ID	*table_id,
	i4		lockmode,
	i4		access_mode)
{
    if (scb == NULL ||  scb->scb_kq_next == NULL)
    {
       if (access_mode == DM2T_A_READ)
	  return(DM2T_IS);
       else
	  return(lockmode);
    }

    {
        DML_SLCB	*slcb = scb->scb_kq_next;

    	for (;;)
    	{

	    if (slcb == (DML_SLCB*) &scb->scb_kq_next || slcb == 0)
	    	break;
	    if ( slcb->slcb_db_tab_base == table_id->db_tab_base )
	    {
		if (access_mode == DM2T_A_READ)
		{
		    if (slcb->slcb_lock_level == DMC_C_ROW)
 			return(DM2T_RIS);
 	    	    else if (slcb->slcb_lock_level == DMC_C_PAGE)
 			return(DM2T_IS);
 	    	    else if (slcb->slcb_lock_level == DMC_C_TABLE)
 			return(DM2T_S);
 	   	    else
 			return(DM2T_IS);
 		}
 		else
 		{
	    	    if (slcb->slcb_lock_level == DMC_C_ROW)
		    	return(DM2T_RIX);
	    	    else if (slcb->slcb_lock_level == DMC_C_PAGE)
		   	return(DM2T_IX);
	      	    else if (slcb->slcb_lock_level == DMC_C_TABLE)
		    	return(DM2T_X);
	    	    else
		    	return(lockmode);
		}
	    }
	    slcb = slcb->slcb_q_next;
    	}
    }
    if (access_mode == DM2T_A_READ)
 	return(DM2T_IS);
    else 	
        return(lockmode);
}


/*
** Name: dm2t_row_lock
**
** Description: Check if (c)row locking supported for a table
**
** Inputs:
**      tcb             - pointer to the TCB.
**	lockLevel	- suggested lock level,
**			  RCB_K_ROW or RCB_K_CROW
**
** Outputs:
**	None
**
** Returns:
**      TRUE/FALSE
**
** History:
**	28-may-98 (stephenb)
**	    Initial creation.
**	05-may-99 (nanpr01)
**	    Gateway tables should not be row locking.
**	26-Apr-2005 (jenjo02)
**	    Clustered tables have no tids, so no row locking.
**	03-Mar-2010 (jonj)
**	    Pass requested lock level as a parameter.
**	    Include dm2d_row_lock here, allow
**	    MVCC (crow) locking on blob extension tables.
**	10-Feb-2010 (maspa05)
**	    Changed test for TCB_CONCUR to TCB2_PHYSLOCK_CONCUR
**          There are now catalogs which are not 'core' but use physical locks
*/
bool
dm2t_row_lock(
DMP_TCB         *tcb, i4 lockLevel)
{
    DMP_TCB     *t = tcb;

    /*
    ** Row locking is not supported for:
    **	  databases that don't support it
    **    V1 tables
    **    Temporary tables (no locking)
    **    Rtree tables
    **    CORE system catalogs (short duration physical page locks)
    **	  Should not be Gateway table
    **    Rows span pages - in which case rows also never share pages
    **	  row locking on blob extension table
    **
    ** Crow locking has the same restrictions, except that
    ** blob etabs are supported.
    **
    ** strictly speaking all TCB_CONCUR tables are also TCB2_PHYSLOCK_CONCUR
    ** but user may not have run upgradedb to add the TCB2 flag to their
    ** existing DBs
    */
    if ( (lockLevel == RCB_K_ROW || lockLevel == RCB_K_CROW) &&
            dm2d_row_lock(t->tcb_dcb_ptr) &&
            DMPP_VPT_PAGE_HAS_ROW_LOCKING(t->tcb_rel.relpgtype) &&
	    !t->tcb_temporary &&
	    t->tcb_rel.relspec != TCB_RTREE &&
	    t->tcb_table_type != TCB_TAB_GATEWAY &&
	    !(t->tcb_rel.relstat & TCB_CONCUR) &&
	    !(t->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR) &&
	    !t->tcb_seg_rows &&
	    (!t->tcb_extended || lockLevel == RCB_K_CROW) &&
	    !(t->tcb_rel.relstat & TCB_CLUSTERED) )
        return(TRUE);
    else
        return(FALSE);
}

/*
** Init tcb key related fields for BTREE secondary indexes
**
** leaf_setup returned TRUE if tcb_leaf_rac needs rowaccessor "setup",
** returned FALSE if leaf is just a copy of the index rowaccessor.
**
**	21-Jan-2004 (jenjo02)
**	    Allow for TID8s in Global indexes.
**	13-Apr-2006 (jenjo02)
**	    Starting with Ingres 2007, restrict range
**	    entries to just key attributes.
**	21-Feb-2008 (kschendel) SIR 122739
**	    Change call to indicate leaf setup requirements
**	    (29-Oct-2009) Set TCB flag for range rac requirements.
**	3-Nov-2009 (kschendel) SIR 122739
**	    Fix error in above related to non-key tidp's.
*/
static VOID
init_bt_si_tcb(
DMP_TCB	*t,
bool	*leaf_setup)
{
    i4			 i,j, tidp_i;
    i4			 LeafTidSize;
    DB_ATTS              **ixkeys;

    LeafTidSize = (t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX)
			? sizeof(DM_TID8) : sizeof(DM_TID);

    /* Btree secondary indexes don't have a data page level;  the
    ** leaf is in effect the data row.  Note however that being a
    ** leaf, the compression (if any) is given by the index-compression
    ** flag and is hardwired to "standard".  The relcomptype in a SI
    ** is ignored.
    ** The leaf rowaccessor will include any non-key columns.
    */
    t->tcb_leaf_rac.att_ptrs = t->tcb_data_rac.att_ptrs;
    t->tcb_leaf_rac.att_count = t->tcb_data_rac.att_count;
    t->tcb_klen = t->tcb_rel.relwid; /* tcb_klen is LEAF klen */
    t->tcb_leafkeys = t->tcb_key_atts;
    /* If tidp is part of the key, adjust key_offset in leaf entry */
    if (t->tcb_atts_ptr[t->tcb_rel.relatts].key) 
	t->tcb_key_atts[t->tcb_keys - 1]->key_offset =
	    t->tcb_rel.relwid - LeafTidSize;

    /* For old-style index (thru 2.0), att/keys are same on LEAF/INDEX pages */
    if (t->tcb_rel.relcmptlvl == DMF_T0_VERSION ||
	t->tcb_rel.relcmptlvl == DMF_T1_VERSION ||
	t->tcb_rel.relcmptlvl == DMF_T2_VERSION ||
	t->tcb_rel.relcmptlvl == DMF_T3_VERSION ||
	t->tcb_rel.relcmptlvl == DMF_T4_VERSION)
    {
	/* index atts = leaf = data */
	t->tcb_index_rac.att_ptrs = t->tcb_data_rac.att_ptrs;
	t->tcb_index_rac.att_count = t->tcb_data_rac.att_count;
	t->tcb_ixklen = t->tcb_rel.relwid;
	t->tcb_ixkeys = t->tcb_key_atts;   /* index keys = leaf keys */
	*leaf_setup = FALSE;

	/*
	** Check if the TIDP is included in the key portion of the
	** secondary index or not (the tidp is always the last column).
	** This indicates to us whether this is an old-style (5.0)
	** index or a new-style (post release 6) one.  (The term
	** "new" here is relative, as all of the versions dealt
	** with in this IF-branch are pretty ancient.)
	**
	** New-style indexes always include the TIDP as part of the
	** key and allow non-key columns to be carried around in the
	** index.  Old-style indexes do not allow arbitrary non-key
	** columns to be included, with the exception that the TIDP
	** column will not be part of the key.
	**
	** Also, new-style btree indexes contain the TIDP value in
	** the key-value portion of each leaf page entry AS WELL AS
	** in the tidp field of the entry.  Old-style btree indexes
	** contain only the keyed columns in the key-value portion
	** of each leaf page entry.
	**
	** For old-style indexes, the klen is the length of the
	** keyed portion of the index row (rowlen - TID).  For
	** new-style indexes the klen is the length of the entire row.
	**
	** For new-style indexes, we must reset the key offset for
	** the TIDP column calculated just above. In most secondary
	** indexes the key portion of the index MUST be a prefix of the
	** index columns.  The key offset computation above assumed
	** this to be true.  In a btree index, if a partial key is
	** is used, the TIDP may be tacked onto the key even though
	** it is not contiguous with the other keys.
	**
	** if (pgtype != TCB_PG_V1)
	** Unique indices do not include TIDP in the key, however
	** the key width for a btree secondary includes ALL columns
	** (including non-key attributes and tidp)
	*/
	if (t->tcb_atts_ptr[t->tcb_rel.relatts].key == 0)
	{
	    if (t->tcb_rel.relpgtype == TCB_PG_V1)
	    {
		t->tcb_klen -= LeafTidSize;
		t->tcb_ixklen -= LeafTidSize;
	    }
	}
    }
    else
    {
	/* v5 or greater table format.
	** Tidp is part of the key for non-unique indexes but not unique
	** indexes.  Index entries carry only keys.  If there are non-key
	** columns and tidp is a key, we'll need a copy of the key att
	** array where the tidp is at a different offset than it is on the
	** leaf.  Recall that for an SI, all key columns must come first,
	** then all nonkey columns (except for tidp which can be a key
	** and is always last).
	*/

	t->tcb_index_rac.att_count = t->tcb_keys;
	if (t->tcb_rel.relatts == t->tcb_keys
	  || t->tcb_atts_ptr[t->tcb_rel.relatts].key == 0)
	{
	    /* Key is not split up */

	    t->tcb_index_rac.att_ptrs = t->tcb_key_atts;
	    t->tcb_ixkeys = t->tcb_key_atts;   /* index keys = leaf keys */
	    t->tcb_ixklen = t->tcb_atts_ptr[t->tcb_keys].offset +
			    t->tcb_atts_ptr[t->tcb_keys].length;
	    /* Leaf needs an independent rowaccessor if there are nonkeys */
	    *leaf_setup = (t->tcb_leaf_rac.att_count != t->tcb_keys);
	}
	else
	{
	    if (DMZ_AM_MACRO(12)) 
		TRdisplay("Build SI tcb for %t, key tidp after non-keys\n", sizeof(DB_TAB_NAME), &t->tcb_rel.relid);

	    /*
	    ** btree secondary index has non-key cols and tidp is part of key
	    ** tidp is at different offsets on leaf/index pages
	    **
	    ** t->tcb_ixkeys is pointing to un-init area in tcb.
	    ** Build an index atts list where the tidp entry points to
	    ** a rejiggered tidp att instead of the "normal" one.
	    **
	    ** Thanks to different leaf/index offsets, we always need an
	    ** independent leaf rowaccessor.
	    */
	    *leaf_setup = TRUE;
	    ixkeys = t->tcb_ixkeys;

	    /* Init: INDEX key array = LEAF key array */
	    for (i = 0, j = 0; i < t->tcb_rel.relkeys; i++) 
		ixkeys[j++] = t->tcb_key_atts[i];

	    /*
	    ** Now adjust tidp entry for INDEX atts/keys
	    ** DB_ATT for INDEX page tidp was allocated after ixkeys array
	    ** (INDEX keys in ixkeys[0] thru ixkeys[t->tcb_keys - 1])
	    */
	    tidp_i = t->tcb_keys - 1;
	    ixkeys[tidp_i] = (DB_ATTS *)&ixkeys[t->tcb_keys]; 
	    STRUCT_ASSIGN_MACRO(*t->tcb_leafkeys[tidp_i], *ixkeys[tidp_i]);

	    /* Adjust offset,key_offset for tidp in INDEX att/key arrays */
	    ixkeys[tidp_i]->offset = ixkeys[tidp_i -1]->offset +
				     ixkeys[tidp_i -1]->length;
	    ixkeys[tidp_i]->key_offset = ixkeys[tidp_i]->offset;

	    t->tcb_index_rac.att_ptrs = t->tcb_ixkeys;
	    t->tcb_ixklen = ixkeys[tidp_i]->key_offset + 
				ixkeys[tidp_i]->length;
	}
    }

    /* cmptlvl < old-version means T10 and newer. */
    if (t->tcb_rel.relcmptlvl == DMF_T8_VERSION
      || t->tcb_rel.relcmptlvl == DMF_T9_VERSION
      || t->tcb_rel.relcmptlvl < DMF_T_OLD_VERSION)
    {
	/* New style (>= v8) secondary LEAF range entries just have keys */
	t->tcb_rngkeys = t->tcb_ixkeys;
	t->tcb_rng_rac = &t->tcb_index_rac;
	t->tcb_rngklen = t->tcb_ixklen;
    }
    else
    {
	/* Old style secondary LEAF range entries have key/nonkey atts */
	t->tcb_rngkeys = t->tcb_leafkeys;
	t->tcb_rng_rac = &t->tcb_leaf_rac;
	t->tcb_rngklen = t->tcb_klen;
	t->tcb_rng_has_nonkey = 1;
    }

    /* Display att/key arrays for leaf/index entries */
    if (DMZ_AM_MACRO(12))
    {
	DB_ATTS *a;
	TRdisplay("Build tcb for %t\n", sizeof(DB_TAB_NAME), &t->tcb_rel.relid);
	TRdisplay("LEAF atts %d entrylen %d keys %d\n", 
		    t->tcb_leaf_rac.att_count, t->tcb_klen, t->tcb_keys);
	for (i = 0; i < t->tcb_leaf_rac.att_count; i++)
	{
	    a = t->tcb_leaf_rac.att_ptrs[i];
	    TRdisplay("ATT %s (%d %d)\n", a->attnmstr,
		a->offset,a->key_offset);
	}
	for (i = 0; i < t->tcb_keys; i++)
	    TRdisplay("KEY %s (%d %d)\n", t->tcb_leafkeys[i]->attnmstr,
		t->tcb_leafkeys[i]->offset, t->tcb_leafkeys[i]->key_offset);

	TRdisplay("INDEX atts %d entrylen %d, keys %d\n",
		    t->tcb_index_rac.att_count, t->tcb_ixklen, t->tcb_keys);
	for (i = 0; i < t->tcb_index_rac.att_count; i++)
	{
	    a = t->tcb_index_rac.att_ptrs[i];
	    TRdisplay("ATT %s (%d %d)\n", a->attnmstr, 
		a->offset, a->key_offset);
	}
	for (i = 0; i < t->tcb_keys; i++)
	    TRdisplay("KEY %s (%d %d)\n", t->tcb_ixkeys[i]->attnmstr,
		t->tcb_ixkeys[i]->offset, t->tcb_ixkeys[i]->key_offset);
	TRdisplay("RANGE entries match %s\n",
	    (t->tcb_rng_rac == &t->tcb_index_rac) ? "index" : "leaf (data)");
    }
}

/*
** Name:	dm2tInvalidateTCB
**
**			dmf_call(DMT_INVALIDATE_TCB, *dmt_cb)
**
** Description: Releases a TCB in real-time when an RDF invalidate
**		event has been raised in a multi-server installation.
**		The call is driven from the scs_check_term_thread
**		on receipt of an RDF invalidate event originating
**		in another server. RDF has invalidated its table
**		cache stuff and called here to release any TCBs
**		we may have cached for that table.
**
**		The RDF invalidates normally occur when a table
**		is dropped or modified such that its characteristics
**		are unreliable, and it must be rebuilt.
**	
**		The invalidate is broadcast to all connected
**		servers. It's assumed that the server that originated
**		the broadcast has already cleaned up its TCBs
**		and its the other servers that need to synch-up.
**
**		This pro-actively frees up TCBs, FCBs, SV_TABLE locks,
**		and file descriptors rather than waiting for the
**		the table to be re-referenced or purged by
**		dm2t_reclaim_tcb() when, by definition, 
**		these resources are near or at exhaustion.
**
** Inputs:
**	dmt
**          dmt_unique_dbid	- the dcb_id of the table's database.
**	    dmt_id		- the table's table_id.
**
** Outputs:
**	None
**
** Returns:
**      E_DB_OK
**
** History:
**	31-Aug-2006 (jonj)
**	    Invented.
**	23-Mar-2010 (jonj) BUG 123468
**	    Some waits deep inside dm2t_release_tcb() may be
** 	    unavoidable, like for a busy page we're trying
**	    to toss. To do those waits, we need a lock list.
*/
DB_STATUS
dm2tInvalidateTCB(
DMT_CB		*dmt)
{
    DMP_TCB		*t;
    DMP_HASH_ENTRY	*h;
    DM_MUTEX		*hash_mutex;
    DB_ERROR		local_dberr;
    STATUS		clstatus;
    DB_STATUS		status;
    CL_ERR_DESC		clerror;
    LK_LLID		lockList;
    i4			local_error;

    /*
    ** Search for the TCB in the tcb cache.
    **
    ** locate_tcb() will return with hash_mutex locked
    ** and a pointer to the Base TCB.
    **
    ** We don't care if we don't find the TCB, but if
    ** we do, check that it's not busy or referenced,
    ** and that its validation stamp has changed
    ** and rendered it obsolete, then toss it.
    */

    t = NULL;
    hash_mutex = NULL;

    locate_tcb(dmt->dmt_unique_dbid, &dmt->dmt_id, &t, &h, &hash_mutex);

    if ( t &&  t->tcb_dcb_ptr->dcb_served == DCB_MULTIPLE &&
	       t->tcb_valid_count == 0 &&
	       t->tcb_ref_count == 0 &&
	      (t->tcb_status & (TCB_BUSY | TCB_VALIDATED)) == 0 )
    {
	/*
	** Verify that the TCB has been made out of date
	** by some sort of DMU or Alter statement run by another server
	*/
	if ( (verify_tcb(t, &local_dberr)) == E_DB_INFO )
	{
	    /*
	    ** This runs under the check_term thread in SCS, which
	    ** does not have a lock list.
	    **
	    ** We only toss pages from the cache, not force them,
	    ** so we don't need a log id, but might need a lock list.
	    **
	    ** Get a short-use lock list, in case dm2t_release_tcb() or
	    ** it's minions have to wait, like for a busy buffer or such.
	    */
	    clstatus = LKcreate_list(LK_ASSIGN | LK_NONPROTECT,
				    (LK_LLID)0,
				    (LK_UNIQUE*)NULL,
				    &lockList,
				    0,
				    &clerror);
	    if ( clstatus == OK )
	    {
		/*
		** Release this invalid TCB.  Specify to toss any cached pages
		** for this table as they are probably invalid as well.
		*/
		status = dm2t_release_tcb(&t, DM2T_TOSS, 
					lockList,
					(i4)0,	/* log_id */ 
					&local_dberr);
		if ( DB_FAILURE_MACRO(status) )
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC*)NULL, ULE_LOG, NULL, 
		    	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);

		/* Toss the lock list */
		clstatus = LKrelease(LK_ALL, lockList,
				    (LK_LKID*)NULL,
				    (LK_LOCK_KEY*)NULL,
				    (LK_VALUE*)NULL,
				    &clerror);
	    }
	    if ( clstatus != OK )
		uleFormat(NULL, clstatus, &clerror, ULE_LOG, NULL, (char *)NULL, (i4)0,
			    (i4 *)NULL, &local_error, 0);
	}
    }
    dm0s_munlock(hash_mutex);

    return(E_DB_OK);
}
