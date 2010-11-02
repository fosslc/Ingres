/*
** Copyright (c) 1985, 2010 Ingres Corporation
NO_OPTIM=dr6_us5 i64_aix
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <mh.h>
#include    <pc.h>
#include    <sd.h>
#include    <sr.h>
#include    <tm.h>
#include    <tr.h>
#include    <nm.h>
#include    <er.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <st.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmccb.h>
#include    <dmse.h>
#include    <dmucb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm1m.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0pbmcb.h>
#include    <dm0s.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1i.h>
#include    <dm1h.h>
#include    <dm1r.h>
#include    <dm1s.h>
#include    <dm1p.h>
#include    <dm2f.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm2r.h>
#include    <dm2rlct.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dml.h>
#include    <dmpbuild.h>
#include    <dmfgw.h>
#include    <dmpecpn.h>
#include    <dmpepcb.h>
#include    <dma.h>
#include    <dm1cn.h>
#include    <dm2rep.h>
#include    <rep.h>
#include    <cui.h>
#include    <nmcl.h>
#include    <dmrthreads.h>
#include    <dmfcrypt.h>

#define no_dup_rows(t)                                         \
  ((t->tcb_table_type != TCB_HEAP) && !(t->tcb_rel.relstat & TCB_DUPLICATES))
#define			    NOT_ALLOCATED	((u_i4) 0xffffffff)

/**
**
**  Name: DM2R.C - Record operations.
**
**  Description:
**      This file contains the routines that break down logical
**      record level operations	into calls of the appropriate
**      physical access method dependent routines.
**	This includes the updating of indices.
**
**	external functions:
**
**          dm2r_rcb_allocate - Allocate a RCB.
**          dm2r_release_rcb - Deallocate an RCB.
**	    dm2r_return_rcb - Return RCB to free list from the allocated list.
**          dm2r_delete - Delete a record.
**          dm2r_get - Get a record.
**          dm2r_load - Load a record into a sort hold file.
**          dm2r_position - Position a record stream.
**          dm2r_put - Put a record.
**          dm2r_replace - Replace a record.
**	    dm2r_unfix_pages - Unfix pages in rcb fixed-page slots.
**
**	internal functions:
**	    si_dupcheck - Check for duplicates on unique secondary indices.
**	    update_load_tab - updates iirelation to reflect state of loaded tbl.
**	    rename_load_tab - renames files during a dm2r_load operation.
**
**  History:
**      16-dec-1985 (derek)
**          Created new for jupiter.
**	15-Jun-1988 (rogerk)
**	    Log logical PUT, DEL, and REP log records if using Fast Commit
**	    to be used in REDO processing.  Also need to log secondary index
**	    updates for REDO.  This is because the secondary index is updated
**	    after the base table, and if the server fails after forcing
**	    the base table change but before making the secondary index
**	    change, then the recovery process may not know to REDO the update
**	    to the secondary index.
**	30-sep-1988 (rogerk)
**	    Added rcb_hash_nofull support.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_create_file() and
**	    dm2f_build_fcb() calls.
**	01-nov-88 (sandyh)
**	    Changed tcb_chr_atts_count to tcb_comp_atts_count for
**	    inclusion of CHAR fields in compression.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	26-feb-89 (mmm)
**	    Just added a comment about logical logging of
**	    appends to btree secondary indexes to dm2r_put().
**	9-mar-1989 (EdHsu)
**	    Added set_jflag routine to set the journal flag passing to dm0l.
**	10-apr-89 (mmm)
**	    Logical key development. Added a check for a "system maintained"
**	    table and call dm1c_sys_init() if set.
**	 3-may-89 (rogerk)
**	    When searching for secondary index row to update in a delete
**	    or replace, build key to 2nd index rather than the entire row.
**	    The search routines expect a key rather than a tuple.
**	    Make sure we return an error when fail to update secondary
**	    index row for some reason - give error indicating reason for
**	    failure.
**	12-may-89 (anton)
**	    local collation support
**	11-apr-1989 (Derek/Sandyh)
**	    Added support for multi part key lookups, as in EQUAL/RANGE case.
**	    Previously, this caused a scan since the BOF was used as the low
**	    mark. Now the combination of keys is used to position low key &
**	    high key to a restricted point.
**	29-may-1989 (rogerk)
**	    Put in checks for errors compressing records.
**	26-jun-1989 (sandyh)
**	    Fix bug 6616 in position for multiple fix of btree leaf pages.
**	15-aug-1989 (rogerk)
**	    Add support for Gateways.  Call gateway for get/delete/put/replace
**	    operations on gateway tables.  Added tcb_table_type field which
**	    indicates access method to use for record level calls.  When doing
**	    record updates, use rcb_logging/tcb_logging values to decide what
**	    type of logging to do.  When release rcb for a gateway table, make
**	    sure that the gateway has been notified.  On BYTID queries, make
**	    sure that tid access is allowed.
**	17-aug-1989 (Derek)
**	    Add checking of security label in get,put,replace,delete.
**       4-sep-1989 (rogerk)
**          Put in trace flag to bypass SBI logging.
**      19-sep-1989 (jennifer)
**          Add security label for bulk copy and maintaining system
**          maintained logkey/tabkey.
**	11-sep-1989 (rogerk)
**	    Log DMU operation after doing LOAD.
**	25-sep-89 (paul)
**	    Added support for dm2r_replace to return the new tid of
**	    the record after a replace in support of bug 8054.
**	27-sep-89 (jrb)
**	    Initialized prec fields.
**	 5-dec-1989 (rogerk)
**	    Put back in the BI logging of BTREE leaf pages.  These are needed
**	    for UNDO recovery by the online-backup code during processing
**	    of the DUMP file information.
**	11-jan-1989 (rogerk)
**	    Only do BI logging of BTREE leaf pages if we are doing online
**	    backup.  The rcb_bt_bi_logging is enabled if online backup is
**	    going on, and the dm0l_bi() routine is only called if this flag
**	    is enabled.
**	25-jan-1990 (fred)
**	    Added support for peripheral operations.
**	28-apr-1990 (rogerk)
**	    Fix btree secondary index update bug.  Check for replace row
**	    being moved after dm1b_allocate call.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	15-oct-90 (linda)
**	    Merged in a couple of gateway fixes, 6.3 -> 6.5.
**      25-jan-1991 (JENNIFER)
**          For dm2r_load indicated that these sorts have EXTERNAL estimates.
**      12-nov-1990 (bryanp)
**          Don't check for duplicate 2-ary index records if the data changed
**          in representation only (as opposed to a true value change) B33703.
**	14-jun-1991 (Derek)
**	    Many changes made in support for new allocation routines:
**		Encapsulated BI generation knowledge in a new dm0p_bicheck()
**		routine. Change allocation processing to assume that the
**		access method specific function don't update the page
**		on an allocation.  Added peformance counters.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-1990 (rogerk)
**		Allocate ADF_CB when allocate RCB in dm2r_rcb_allocate.
**		This will be used for all ADF calls made on this table rather
**		than allocating locally all over dmf.  Initialize the adf
**		control block to default values.
**		This was done as part of the DBMS Timezone changes.
**	    17-dec-1990 (jas)
**		Smart Disk project changes:  if we are positioning for or
**		fetching from a Smart Disk scan, hand off to dm1sd_position
**		and dm1sd_get, respectively.  Also turn off the RCB_SDPOSITIONED
**		bit wherever we turn off the RCB_POSITIONED bit.  Also, free
**		up any Smart Disk resources when an rcb is freed.
**	    3-jan-91 (rogerk)
**		Make sure that pages fixed into local page pointers are unfixed
**		before leaving dm2r_replace.  We were leaving leaf pages fixed
**		after btree allocate which would cause a dmd_check with errors
**		E_DM9301_DM0P_FORCE_PAGE or E_DM9305_DM0P_TRAN_INVALIDATE.
**	    4-dec-91 (rogerk)
**		Added support for Set Nologging.
**		Nothing should be logged for sessions which have set NoLogging.
**		Check rcb_logging before syscat and 2nd index updates.
**		Use rcb_logging rather than tcb_logging to direct logging
**		actions on 2nd indexes.
**
**		Also in dm2r_rcb_allocate, don't set rcb_logging on secondary
**		index if the table is specifically marked as a nologging table -
**		even if the base table is logged.  This should not effect any
**		currently supported tables, but in the future we may have base
**		tables which are logged while secondaries are not.
**
**	    08-feb-1991 (mikem)
**		Added code to set new field in the rcb which keeps track of the
**		last logical key assigned.
**	     7-mar-1991 (bryanp)
**		Added support for Btree index compression: set new fields in the
**		DM2U_M_CONTEXT structure before calling the Btree build routines
**		in dm2r_load().
**	    22-mar-1991 (bryanp)
**		B36630: Added dm2r_set_rcb_btbi_info().
**	    10-apr-1991 (bryanp)
**		B36837: Pass DMSE_CHECK_KEYS properly to dmse_begin.
**	    02-may-91 (kwatts)
**		Smart Disk changes to parameters passed to SDfree. These are
**		because of a spec change to cope with Task Specification Blocks
**		being shared in a shared query plan. The pseudo page is now
**		passed to SDfree instead of the TSB being passed in calls from
**		dm2r_release_scb and dm2r_return_scb..
**	    22-jul-1991 (rogerk)
**		Integrated Jennifer's fix to avoid pre-allocating super-large
**		sort files.  Add new sort flag - DMSE_EXTERNAL - which indicates
**		that the rows are being fed to the sorter from an outside
**		source.  In this case we do not trust the row_count estimate
**		and do not try to pre-allocate the space for the sort files.
**		We start with a small amount (64 pages) and add space at runtime
**		as required.  This prevents us from trying to allocate 50 GB of
**		disk space whenever we predict that a 10-way join will produce
**		a trillion records.  This change was made to fix Bug # 34235.
**	    11-sep-91 (kwatts)
**		Call of SDfree in dm2r_release_rcb should not be made if we
**		are in the process of releasing the TCB too.
**	24-oct-1991 (mikem)
**	    Updated dm1c_sys_init() call to match new call parameters changed
**	    as part of the 6.4->6.5 merge.
**	 3-nov-1991 (rogerk)
**	    Fix recovery problem in COPY to existing HEAP table.  We were
**	    logging garbage value for mct_recreate which would cause us to
**	    not do proper recovery in dmve_load.
**	31-mar-1992 (bryanp)
**	    When calling dm0p_close_page for a temporary table, don't pass
**	    DM0P_CLOSE as the flag -- we can just pass a zero flag value.
**	14-apr-1992 (rogerk)
**	    Fix arguments to ule_format calls in dm2r_delete and dm2r_replace.
**	    Pass tid values by value instead of by reference (bug 42510).
**	14-apr-1992 (rogerk)
**	    Bug #40998: Fix byte-align problem in Hash loads.  In dm2r_load,
**	    use I4ASSIGN_MACRO to put hash value into sort row, rather
**	    than direct assignment.  Then when taking rows out of the sorter
**	    in order to call dm1hbput(), use I4ASSIGN_MACRO to extract out
**	    the hash value rather than passing a pointer into the unaligned
**	    tuple buffer.
**      26-may-92 (schang)
**          GW merge.
**	7-July-1992 (rmuth)
**	    Prototyping DMF
**	29-August-1992 (rmuth)
**	    - Add flag to dm1p_lastused.
**	    - When a dm2r_load was performed on a temporary table it was not
**	      copying the new iirelation info into its TCB. This was causing
**	      problems latter.
**	14-sep-1992 (bryanp)
**	    Bug #39344: If dm2r_load encounters deadlock in dm2t_control, just
**	    return the deadlock (don't map it into a DMF Internal error).
**      25-sep-1992 (stevet)
**          Changed svcb_timezone to svcb_tzcb to support new timezone
**          adjustment methods.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	30-October-1992 (rmuth)
**	    Set the mct_guarantee_on_disk flag.
**	09-Nov-1992 (fred)
**	    In dm2r_delete(), Moved code dealing with extension table
**	    maintenance.  It had migrated to after the secondary
**	    index code (or had always been there) and this routine
**	    was returning before dealing with extensions if the base
**	    table had no secondary indicies.
**	06-nov-92 (jrb)
**	    Changed interface to dmse_begin in dm2r_load to pass in multiple
**	    locations.
**	8-oct-1992 (bryanp)
**	    Initialize mct_last_data_pageno to -1 in dm2r_load().
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Took out calls to log put, delete and
**	    replace operations.  These are now logged from inside the page
**	    updating routines in DM1R.  Also removed Before Image log
**	    records as these are no longer used.
**	14-dec-1992 (jnash)
**	    Removed temporary use of rcb_recovery flag used by dm1c routines.
**	    Initialize the rcb_journal field in dm2r_rcb_allocate and propagate
**	        the journal status from base table tcbs into the index tcbs.
**	    Added rcb_usertab_jnl field and removed set_jflag routine.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	05-dec-1992 (kwatts)
**	    Smart Disk enhancement. Calls to dm1sd_position have one less
**	    parameter.
**	18-dec-92 (robf)
**	    Removed obsolete dm0a.h
**	16-feb-1993 (rogerk)
**	    Fix problem in replace of btree with a secondary index found by
**	    Ken Watts.  The 'in_place' flag should not be used on the secondary
**	    index update.
**	18-feb-1993 (jhahn)
**	    Added check for statement level unique indexes.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed old rcb_abt_sp_addr field used with BI recovery.
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	    Deleted dm2r_set_rcb_btbi_info routine.
**	    Removed unused xcb_lg_addr.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix core catalog data page as soon as base table change is
**		    complete. This improves concurrency, but more importantly
**		    it satisfies the only-fix-one-core-catalog protocol for
**		    node failure recovery.
**		Improve error logging in dm2r_put.
**	06-may-93 (anitap)
**          Added support for "SET UPDATE_ROWCOUNT" statement in dm2r_replace().
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-may-1993 (kwatts)
**	    Cleared a possible memory leak in dm2r_return_rcb. Always
**	    release the store for a Smart Disk page if there is one.
**      21-June-1993 (rmuth)
**          When loading data, dm2r_load, were not setting mct_recreate flag
**          properly.
**	21-jun-1993 (mikem)
**	    Changed update_load_tab to only update specific fields of the
**	    iirelation tuple from the input relrecord (fields associated with
**	    the physical structure of the table), rather than all fields.
**	    Previously the iirelation value for the logical key was being
**	    updated with the tcb version, which caused duplicate logical keys
**	    after repeated loads to the table.
**       1-Jul-1993 (fred)
**          Removed assignment for rcb into coupon from dm2r_get().
**          This is now handled by the various dm1*_get() calls.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-July-1993 (rmuth)
**	    Add the build_tbl_info parameter to dm2r_load, this allows
**	    the caller to pass information on how to build the table.
**      23-aug-1993 (rogerk)
**	    Changed dm2r_load to use new dm2u_file_create routine.
**	    Turned off journaling on the iirelation updates for load operations.
**	20-sep-1993 (kwatts)
**	    Bring forward a 6.4 bug fix:
**          Bug #49377.  Modify dm2r_position() to turn on RCB_READAHEAD for
**          equal partial-key btree searches.
**	18-oct-1993 (bryanp) Bug #B49920
**	    Change "inplace_replace" argument to dm1b_replace from
**		(action == IN_PLACE) to ((action & ALLOCATE) == 0) to repair
**		btree replaces which didn't change the key but required dup
**		checking -- they were erroneously claiming to be non-in-place
**		replaces.
**	18-oct-1993 (rogerk)
**	    Changed decisions on when to use rebuild and recreate mode
**	    load algorithms.  We now sometimes use recreate/rebuild
**	    loads on heap copies if the row estimate is large.  This
**	    can make recovery much faster and friendlier on large loads.
**      27-Oct-1993 (fred)
**          Correct support for deallocation of peripheral control blocks,
**          and for lists of these blocks.
**	31-jan-1994 (mikem)
**	    bug #57664
**	    Changed dm2r_load to always properly initialize the db_sync_flag
**	    which controls whether or not table updates are guaranteed on disk
**	    following a DIwrite().  Previous to this change the variable was
**	    uninitialized in the case of bulk load (copy into empty file) into
**	    a structured table.
**	31-jan-1994 (mikem)
**	    Code inspection/lint changes.  Removed unused variables.  Changed
**	    NOT_ALLOCATED constant to be unsigned so that it could be compared
**	    correctly with tid_i4 structure members.
**	05-nov-93 (swm)
**	    Bug #58633
**	    Alignment offset calculations used a hard-wired value of 3
**	    to force alignment to 4 byte boundaries, for pointer alignment.
**	    Changed these calculations to use the ME_ALIGN_MACRO.
**      22-feb-1994 (andys)
**	    Integrate 1-nov-1993 (andys) from ingres63p.
**          Loads must wait if an online checkpoint is in progress.
**          [bug 52253]
**	28-mar-1994 (rogerk)
**	    Fixed bug in constraints checks: don't unfix current rcb pages
**	    on unique constraint violations as qef expects to reaccess the
**	    row at the current position (bug #59241).
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them
**	    prior to close.
**	21-apr-1994 (rogerk)
**	    Added test trace point (DM654).
**	23-may-1994 (andys)
**	    When loading into a non-empty heap, update reltups rather than
**	    overwriting it. [bug 49726]
**	23-may-1994 (rmuth)
**	    bug #63511,
**	    When loading into a heap table we never rebuild the existing table
**	    but always just add to the end. This requires us to read some
**	    of the existing tables's pages e.g. The last datapage, FHDR,
**	    FMAP(s). For temporary tables the only code which knows if these
**	    pages are on disk or in memory is the buffer manager so make sure
**	    that in the above case we always set mct_inmmemory and related
**	    fields to TRUE. This will force the dm1x code to use the buffer
**	    manager which is guaranteed to do the correct thing.
**      23-may-1994 (mikem)
**          bug #63259
**          Make sure the "mod_loc" file in rename_load_tab() is closed before
**          it is renamed to the "new" file.
**          In the case of copying into an empty structured
**          table, previous to this change, modify files would be left "opened."
**          On unix this would mean that even if the tables were destroyed,
**          disk space associated with the tables would not be reclaimed until
**          either the server was shut down or the slave system would LRU close
**          the file descriptor.
**	24-aug-1994 (cohmi01)
**	    Added backwards support - setting DM1C_GETPREV.
**      27-dec-94 (cohmi01)
**          For RAW IO, pass location ptr to dm2f_rename() in rename_load_tab().
**	    Save additional info in XCCB, for identifying correct raw extent.
**	22-feb-95 (cohmi01)
**	    Adjust setup of xccb for old_loc info, for RAW locations.
**      04-apr-1995 (dilma04)
**          Support repeatable read: set RCB_PAGE_HAD_RECORDS and
**          RCB_TABLE_HAD_RECORDS if a record is successfully retrieved.
**	19-Apr-1995 (cohmi01)
**	    if 'checkeof' is on for a secondary index, call dm1p_checkhwm()
**	    to adjust eof so that group buffers may be used.
**      19-apr-1995 (stial01)
**          Modified dm2r_position() to support select last tuple from a
**          btree table for MANMANX. The new flag is DM2R_LAST.
**	09-May-1995 (cohmi01)
**	    Added support for asynchronous readahead (prefetch) of related
**	    secondary indexes when doing a get on table that is 'likely'
**	    to be updated or deleted from. Actual reads are handled by the
**	    new read-ahead thread. See dml!dmcread.c for more info.
**	07-jun-1995 (cohmi01)
**	    Updated prefetch system to handle multiple RA requests, as
**	    well as multiple threads handling a single compound request
**	    concurrently (eg multiple sec index on one table).
**	17-Jul-1995 (cohmi01)
**	    dm2r_put() - Utilize asynch prefetch for sec index pages that
**	    will be needed when processing an insert. This is only if more than
**	    one index exists, to ensure possibility of IO overlap.
**	21-Jul-1995 (cohmi01)
**	    Change dm2r_count() to support 'gateway' tables.
**	19-july-1995 (b69768)(carly01)(from change 274922 in ingres63p codeline)
**		Transaction id assignment should be a STRUCT_ASSIGN_MACRO.
**	10-aug-95 (sarjo01)
**	    dm2r_load(): added support for fastload function.
**	11-aug-1995 (shust01)
**	    remove setting of DM_TIDEOF after call to dm1b_search() in case
**	    DM2R_LAST in dm2r_position(). Not needed.
**	10-Aug-1995 (cohmi01)
**	    Move xxx_prefetch functions to new dm2p.c file. Rename calls
**	    to these functions. Cancel prefetch requests based on type
**	    indicated in RCB. Use rcb_updstats_rcb when determining what
**	    updates have been done, to account for related RCBs. If appropriate,
**	    request data page list prefetch when positioning in index/btree.
**	01-sep-1995 (cohmi01)
**	    Add support for DM2R_LAST on an ISAM table. Update epilogue of
**	    DM2R_LAST processing to save rcb_p_xxxtids, only mark as positioned
**	    if dm1x_xxx was successful. Move dm2r_count() logic to dm2a.c,
**	29-dec-1995 (nick)
**	    Fix #73388 introduced by AndyS change 22-feb-1994.  Stall for
**	    online checkpoint was done unconditionally.
**	31-jan-1996 (prida01)
**	    In dm2r_load(), fix temp. table bug #73737.
**	 4-mar-1996 (nick)
**	    Correct comment history.
**      06-mar-1996 (stial01)
**          dm2r_load() Set mct_page_size to tcb_rel.relpgsize.
**          rename_load_tab() pass relpgsize to dm2f_open_file.
**          Pass page size to dm2f_build_fcb()
**          dm2r_schedule_prefetch() pass page size to dm0p_has_buffers
**	24-apr-1996 (wonst02)
**	    Add RTREE support.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	17-may-96 (stephenb)
**	    Add replicator code, check for replication on update, insert and
**	    delete, perform data capture and open the input queue if
**	    necesary.
**	06-jun-1996 (wonst02)
**	    Added BuildRtreeRecord to support insert.
**	 1-jul-96 (nick)
**	    Duplicate key checking being done twice in dm2r_replace().
**	30-jul-96 (nick)
**	    Clear ADI_WARN area of new RCB. #78024.
**	 5-aug-96 (stephenb)
**	    store remote tx id when replication server adds records to the
**	    replication input queue.
**	13-Aug-1996 (prida01) bug 78108 access violations using long
**	    varchars caused by incorrect parameter casting.
**	13-sep-1996 (canor01)
**	    Allocate a tuple buffer for data uncompression as part of RCB.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dm1h_hash() and dm1h_newhash().
**	11-Oct-1996 (wonst02)
**	    Use BuildRtreeRecord in dm2r_delete and dm2r_replace.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced support for the Distributed Multi-Cache Management
**          (DMCM) protocol.
**	24-Oct-1996 (somsa01)
**	    In dm2r_put(), relocated dm1c_pput() code; if a table contains
**	    blobs, first logically lock the appropriate page in the base table.
**	    After that is done, then move the blob segment tuples into the ETAB
**	    table from the appropriate ETAB temporary table.
**	11-Nov-96 (kitch01) bug 78094 updates to secondary index fail if
**	    II_COLLATION has been set and national language characters are
**	    used in the update statement. Fix to dm2r_replace.
**	28-Nov-1996 (pchang)
**	    Fixed bug 79320.  A typo in dm2r_load() - 'relstat' for 'relspec'
**	    could cause BTREE tables with non-unique keys to have two leaf pages
**	    containing entries with the same key values after data had been 
**	    loaded.  As a consequence, partial updates could go undetected and 
**	    result in the breach of data integrity.
**	12-nov-1996 (hanch04)
**	    Set replicator parameters to NULL in dm2r_rcb_allocate
**	    Bug 79305
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          dm2r_rcb_allocate: Init new fields in RCB.
**          dm2r_put, dm2r_replace: check for si dups before allocate in base
**          dm2r_delete, dm2r_put, dm2r_replace: value lock for unique key/row
**          si_dupcheck: Acquire value lock for unique key
**          dm2r_replace: if row locking execute as delete/put
**          dm2r_position: changes for REPOSITION
**          Added si_delete(), si_put(), si_replace(), base_replace()
**          Added base_delete_put(), dm2r_fetch_record(), dm2r_lock_value()
**      22-nov-1996 (nanpr01)
**          init the version with the current table version to dmpp_load.
**      25-nov-1996 (stial01)
**          dm2r_rcb_allocate() fixed init of key buffers, adf_cb
**	03-dec-1996 (wonst02)
**	    Added ancestor stack parameters to dm1m_replace().
**      17-dec-1996 (stial01)
**          dm2r_replace() If allocate caused split, we may need to 
**          re-fetch tuple to be deleted.
**      17-jan-97 (stial01)
**          si_dupcheck() Return E_DB_ERROR if dm2r_lock_value fails (B80066)
**      22-jan-97 (dilma04)
**          Fix bug 80112: Do not lock rows if doing get to update secondary
**          index.
**      27-jan-97 (dilma04)
**          - prevent phantom protection when updating secondary indexes; 
**          - initialize status variable in si_dupcheck().
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - use macro to check for serializable transaction;
**          - initialize rcb_iso_level field in RCB.
**      13-mar-97 (stial01)
**          dm2r_rcb_allocate() Fixed init of adf_cb. (again)
**	10-Jan-1997 (kitch01)
**	    Further problems with II_COLLATION, bug 79891. I found that this
**	    is not a problem in 6.4 because bug 43950 fixes it. I have
**	    removed my previous change, bug 78094, and manually integrated
**	    the fix for 43950 from 1992! This sets the collation sequence
**	    during RCB allocation, it also sets the timezone. Manual
**	    integration was required as adf_timezone is now called adf_tzcb.
**	10-July-1997(angusm)
**	    Add handling for cross-table update ambiguous replace
**	    (bug 79623).
**	24-july-1997(angusm)
**	    Wrong #, type of parameters passed to dmpp_dput()
**	13-mar-97 (stephenb)
**	    when opening the replicator input queue (dd_input_queue) in various
**	    states, set the rcb_xcb_ptr in the resulting rcb.
**      10-mar-1997 (stial01)
**          dm2r_rcb_allocate() Pass relwid to dm0m_tballoc()
**          dm2r_put() Removed EXTRA si_dupcheck added by cross-integration
**              of blob changes. Those changes also required changes in
**              dm1r_allocate,dm1r_put.
**          dm2r_load() Init mct_crecord, area to compress a record
**      21-apr-97 (stial01)
**          dm2r_fetch_record() Changed parameters to dm1b_bybid_get()
**	14-May-1997 (jenjo02)
**	    Inherit rcb_access_mode from base_rcb instead of forcing it to
**	    RCB_A_WRITE (dm2r_rcb_allocate()).
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - set RCB_*_HAD_RECORDS flags in dm2r_get, if a record is 
**          successfully retrieved;
**          - if isolation level is RR, convert page locks related to the
**          returned record to logical locks;
**          - after a record is fetched by dm2r_fetch_record(), set
**          RCB_*_UPDATED flags, convert all physical locks to logical
**          and turn RCB_CSRR_LOCK flag off.
**       29-may-97 (stial01)
**          dm2r_return_rcb() always unfix pages in rcb
**          dm2r_replace: If unique SI, always lock old value (value_set = 0)
**          si_dupcheck: Memory leak if only getting value lock
**	3-jun-97 (stephenb)
**	    if the user is updating the replicator input queue and 
**	    replication is active, then set the input queue value in the
**	    transaction control block so that the input queue values will
**	    be distributed on commit.
**      13-jun-97 (dilma04)
**          dm2r_get(): do not convert leaf page's lock to logical, because 
**              it either will be released, or converted to a logical lock, 
**              if the page is updated;
**              release leaf page's lock if page has not been updated.
**          dm2r_rcb_allocate(): do not inherit row locking from base rcb if
**              it is not allowed on secondary index.     
**          si_dupcheck(): ensure that no locks will be released during 
**              duplicate checking. 
**      16-jun-97 (stial01)
**          dm2r_position() Initialize r->rcb_p_lk_type, r->rcb_p_hk_type
**	2-jul-97 (stephenb)
**	    Allow the replication server (trace point dm32) to trigger
**	    the replication queue management wakeup event on commit, when
**	    it has inserted records into the input queue.
**      29-jul-97 (stial01)
**          Pass original operation mode (PUT/DEL/REPL) to dm1*_put,dm1*_delete
**          for logging.
**          Space reclamation for leaf entries depends on page size not lockmode
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call. also alter replicator
**	    code to cope with upper case catalogs (sql92)
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_close_page(), dm0p_reclaim_tcb(),
**	    dmse_begin() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**      29-aug-97 (stial01)
**          B84909: If index is DM_COMPAT_PGSIZE, rcb_lk_type can't be RCB_K_ROW
**      29-sep-97 (stial01)
**          dm2r_position() Dont init rcb_pos_clean_count = DM1B_CC_INVALID.
**          It gets initialized during dm1b_search while page is mutexed.
**	23-oct-1997 (hanch04)
**	    Add missing DML_SCB for dm2t_open()
**      12-nov-97 (stial01)
**          dm2r_get() If cursor stability, unlock previous row before
**          we unlock/unfix the data page it is on.
**          Pass lockKEY even if unlocking with lockID (for set lock_trace)
**          Consolidated rcb fields into rcb_csrr_flags.
**	14-nov-97 (stephenb)
**	    Correct locking paramater in dm2t_open() when openeing the
**	    replicator input queue so that we inherit the locking strategy
**	    from the user RCB.
**	25-nov-97 (stephenb)
**	    Add code to set the locking strategy for the replicator input
**	    queue when opening for base table updates in dm2r_put(), 
**	    dm2r_replace() and dm2r_delete().
**      20-jan-98 (stial01)
**          dm2r_rcb_allocate() If index is RTREE, rcb_lk_type can't be row.
**	05-mar-1998 (hanch04)
**	    Call dm0m_allocate instead of using static memory.
**      28-may-98 (stial01)
**          dm2r_rcb_allocate() moved code to dm2t_row_lock()
**	23-Jun-1998 (jenjo02)
**	    Added LK_VALUE parm to dm0p_unlock_page() prototype.
**      07-jul-98 (stial01)
**          dm2r_fetch_record() convert physical locks to logical only if CS.
**	28-jul-1998 (somsa01)
**	    Added another parameter to dm1c_pput() which is set if a table
**	    which is being bulk loaded contains blob columns. In the case of
**	    bulk loading blobs, added the appropriate code to dm2r_load().
**	    Also removed variables which were not in use anymore.
**	    (Bug #92217)
**      13-Nov-98 (stial01)
**          Renamed DM1C_SI_UPD->DM1C_GET_SI_UPD, 
**          Renamed DM1C_SI_UPDATE->DM1C_FIND_SI_UPD
**          si_replace() pass DM1C_GET_SI_UPD to avoid unecessary locking
**      16-nov-98 (stial01)
**          Check for btree primary key projection
**	12-feb-1999 (somsa01)
**	    In dm2r_load(), set load_blob based upon rcb_reltups. It is
**	    more accurate than the TCB's reltups.
**	17-feb-1999 (somsa01)
**	    In dm2r_replace(), if it looks like nothing has changed and we
**	    are dealing with peripheral columns, then check lobj_changed.
**	    If peripheral tuples were in fact changed (which may not be
**	    obvious by examining just the coupon), set type_nkey_change to
**	    ADT_VALUE. Remember that peripheral columns cannot be part of
**	    a key.
**	19-Feb-1999 (kosma01)
**	    Changed second argument of dm2r_cancel_prefetch() to be a 
**	    i4 instead of u_i2. In dm2r_cancel_prefetch(), the
**	    second argument, item, is compared against two negative
**	    constants.
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
** 	08-apr-1999 (wonst02)
** 	    Treat RTREE more like BTREE in si_replace (called by dm2r_replace);
** 	    Add dmd.h for prototypes.
**      05-may-1999 (nanpr01,stial01)
**          More concurrency if serializable with equal predicate:
**          Value locking for phantom protection, instead of page locking
**          Also value locks acquired for duplicate checking are no longer held
**          for duration of transaction. (changes also made in dupcheck rtns)
**          Also Duplicate row checking, uses KEY value lock not ROW value lock.
**          Added optimization of secondary duplicate checking. Defer until
**          after base table is updated (so that it is done with allocate tid),
**          whenever we know that a duplicate error will result in rollback,
**          and unique scope is row.
**          Added DM720 to force duplicate checking before updating base table
**      28-may-1999 (stial01)
**          Only call dm1r_check_lock if DMZ_SES_MACRO(33)
**      28-may-1999 (stial01)
**          Fixed problem with previous change
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**	21-Apr-1999 (shust01)
**	    Set fetch_tid.tid_i4 to be rcb_currenttid (current tid) instead
**	    of rcb_fetchtid (current bid) in dm2r_replace() for special case 
**	    of old row and new (replaced) row being identical.  The new tid
**	    (not bid) should be returned.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**      23-sep-1999 (stial01)
**          dm2r_replace(), if DM2R_BYTID, defer dm2r_fetch_record until
**          after we know if key is being updated. (STARTRAK 7391263,2)
**      20-oct-99 (stial01)
**          dm2r_fetch_record() Fixed error handling after lock convert 
**      03-nov-99 (stial01)
**          Fix for 05-may-1999 change. In dm2r_replace, si_dupcheck may be
**          deferred until after updating base table. Make sure we do not
**          dupcheck if only tidp changed in index.
**      04-nov-99 (stial01)
**          base_replace() must pass uncompressed record to allocate rtn
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      15-nov-1999 (stial01)
**          si_replace() pass record,leafkey to dm1b_replace
**	05-dec-99 (nanpr01)
**	    Optimized the code to reduce the tuple header copy from the page.
**      12-Jul-1999 (hanal04)
**          Added detection and handling of E_AD101B_BAD_FLT_VALUE. b85041.
**      22-Jul-1999 (hanal04)
**          Adjust above change to report errors from DMF rather than
**          ADF to fix build problems. b98034.
**      08-dec-1999 (stial01)
**          No update mode locking for etabs
**	17-dec-99 (gupsh01)
**	    Added additional checks to give out correct error message in case 
**	    of updating a readonly database in dm2r_put, dm2r_replace, 
**	    dm2r_load and dm2r_delete.
**	10-Jan-2000 (jenjo02)
**	    Replaced direct reference to obsolete bm_fcount with 
**	    DM0P_COUNT_QUEUE macro in dm2r_schedule_prefetch().
**      24-jan-2000 (stial01)
**          Backout cross integration of 442832 for b69562, problems with
**          replicating updates of peripheral columns.
**      27-jan-2000 (stial01)
**          Init (renamed) reposition fields in rcb rcb_repos*
**      31-jan-2000 (stial01)
**          dm2r_replace() Check return code after dm2r_fetch_record (B100257)
**          dm2r_get() (Row) lock coupling is handled in dm1r_lock_row
**      02-may-2000 (stial01)
**          dm2r_get() Check for DM2R_RAAT flag 
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026) 
**      1-Jul-2000 (wanfr01)
**        Bug 101521, INGSRV 1226 - Added <nmcl.h> to read value of 
**	       II_RAAT_RETRY if RAAT is being used.
**	10-jan-2001 (hayek02)
**	    Added hpb_us5 to NO_OPTIM - this prevents ACCVIO in dbms during
**	    createdb when DMF calls DI_sense(). This change fixes bug 103684.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Pass record and compressed record to allocate routines
**      01-nov-2000 (stial01)
**          si_replace() if compression, pass compressed rec to replace(b103044)
**	08-Nov-2000 (jenjo02)
**	    When releasing/returning an RCB for an updated
**	    session temp table, update savepoint id in
**	    table's XCCB.
**	    In dm2r_load(), set RCB_TABLE_UPDATED.
**      20-dec-2000 (stial01)
**          si_replace() fixed criteria for duplicate checking (b103538,b103192)
**      19-jul-2001 (stial01)
**          dm2r_get() DM1C_PKEY_PROJECTION only if read cursor (b105210)
**      10-jan-2001 (hayek02)
**          Added hpb_us5 to NO_OPTIM - this prevents ACCVIO in dbms during
**          createdb when DMF calls DI_sense(). This change fixes bug 103684.
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB from XCB instead of SCU_INFORMATION.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_row_access() if neither
**	    C2 nor B1SECURE.
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**	    Init new rcb fields
**	09-Feb-2001 (jenjo02)
**	    Avoid copying record image to rcb_record_ptr in 
**	    dm2r_get() unless it seems prudent to do so.
**	09-apr-2001 (abbjo03)
**	    Initialize the Unicode collation table in dm2r_rcb_allocate().
**	12-Apr-2001 (jenjo02)
**	    09-Feb-2001 change above init'd rcb_fetchtid to a value that
**	    is incomptable with gateways, which don't use TIDs; in those
**	    cases init rcb_fetchtid to 0.
**	26-Apr-2001 (jenjo02)
**	    Fix another gateway problem. Always copy row to rcb_record_ptr
**	    if gateway.
**      01-may-2001 (stial01)
**          Init adf_ucollation, rcb_ucollation
**      10-may-2001 (stial01)
**          Added rcb_*_nextleaf to optimize btree reposition
**      19-jul-2001 (stial01)
**          dm2r_get() DM1C_PKEY_PROJECTION only if read cursor (b105210)
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	05-Mar-2002 (jenjo02)
**	    Use physical locking for Sequence Generator catalog
**	    (DM_B_SEQ_TAB_ID).
**      23-may-2002 (horda03) Bug 107439
**          The attributes array in the TCB is accessed 1..relatts,
**          not 0..(relatts - 1). This can lead to problems when a column is added
**          to a table and made the index (its the last attribute of the table).
**	08-nov-2002 (somsa01)
**	    Removed hpb_us5 from NO_OPTIM.
**      28-may-2003 (stial01)
**          Remove (ifdef) TRdisplays
**      04-aug-2003 (stial01)
**          dm2r_replace() Validate rcb_currenttid before skipping get (b110637)
**          (bug 110637 was caused by changes for SIR 97248, and SIR 98478).
**      16-sep-2003 (stial01)
**          Init rcb_galloc* for SIR 110923
**      25-nov-2003 (stial01)
**          Added DM723, svcb_etab_logging to set etab logging on/off
**      02-jan-2004 (stial01)
**          Removed blob rcb_galloc fields, init new fields in rcb for blobs
**          dm2r_put, call dm1b_bulk_put if dmpe is doing bulk load
**	15-Jan-2004 (schka24)
**	    Changes for partitioned tables project.
**	24-Jan-2004 (jenjo02)
**	    Isolated si_put(), si_delete(), si_replace(), added
**	    external functions dm2r_si_put(), dm2r_si_delete(),
**	    dm2r_si_replace() in support of parallel dmrThreads.
**	    Added support for Global secondary indexes.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-mar-2004 (stial01)
**          dm2r_return_rcb() Don't deallocate rcb_pcb 
**	10-May-2004 (schka24)
**	    Remove rcb_pcb, cleanup list is now attached to XCB.
**	    Handle blob force-abort just like interrupt, pass it on up.
**	02-Jul-2004 (hanal04) Bug 110565 INGSRV2889
**	    Prevent E_DM9385 when deleting from a table with an RTREE
**	    secondary index.
**      8-jul-2004 (stial01)
**          Init rcb_3rnl_page
**      4-oct-2004 (stial01)
**          dm2r_rcb_allocate() Fix rcb_segbuf allocation
**	16-dec-04 (inkdo01)
**	    Added bunches of collID's.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	03-feb-2005 (thaju02)
**	    Init rcb_temp_iirelation.
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag.
**      Mar-18-2005 (hweho01)
**          Cross-integrated change #474714 by inifa01 from ingres26 
**          codeline:
**          06-Jan-2005 (inifa01) b113796/INGREP171
**          dm2r_replace() check if attribute(s) being updated are replicated
**          columns before calling dm2rep_capture().
**      09-may-2005 (horda03) Bug 114471/INGSRV3294
**           Initialize the rcb_temp_iirelation field when allocating an RCB
**      21-sep-2005 (stial01)
**	    si_replace() Fixed err_code checking (b115245) 
**      29-sep-2005 (stial01)
**          dm2r_fetch_record() convert all physical locks if repeatable read
**          and record ptr wasn't valid prior to dm2r_fetch_record (b115299) 
**      17-oct-2005 (stial01)
**          si_put() Assume si_dupcheck got value lock only if
**          RCB_SI_DUP_PRECHECK (b115398)
**      09-feb-2006 (stial01)
** 	    dm2r_fetch_record - Check status from dm0p_trans_page_lock to make
**          sure LK_U to LK_X conversion was successful. (b115741)
**      31-mar-2006 (stial01)
**          base_delete_put - DON'T overrwrite E_DB_ERROR status (b115922)
**      22-May-2007 (hweho01)
**          Avoid error during unicode data update, need to initialize the  
**          user area when a new RCB is allocated, 
**      26-nov-2007 (stial01)
**          Fix estimated_pages for small row estimates (b119521)
**	19-Feb-2008 (kschendel) SIR 122739
**	    Reorganization of tuple level access info into DMP_ROWACCESS.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dmse_?, dm2r_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1m_?, dm1s_?, dm1i_?, dm1h_? dm1r_?, dm1b_?, dm2rep_? 
**	    functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1?b?, dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dmf_gw?, dm1c_?, dm0p_? functions converted to DB_ERROR *
**      08-jan-2009 (stial01)
**          Compare replicator catalog names to unpadded strings 
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      15-jun-2009 (stial01) b122176
**	    dm2r_replace() rows span pages, Execute replace as delete,ALLOC,put
**      16-Jun-2009 (hanal04) Bug 122117
**          Treat E_DM016B_LOCK_INTR_FA the same as a user interrupt.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**      10-sep-2009 (stial01)
**          Don't use string compare for relcmptlvl comparisons
**	17-Nov-2009 (kschendel) SIR 122890
**	    Ifdef-away prefetch stuff completely.  Break out invalidate of
**	    loaded table, will make future insert/select via LOAD easier.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**      11-jan-2010 (stial01)
**          dm1b_search with DM1C_FIND_SI_UPD/DM1C_EXACT, DM1C_TID/DM1C_EXACT
**          locates key, so save it in rcb_fet_ptr in dm1b_search, not here.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**	    sensitized to crow_locking().
**	24-Feb-2010 (troal01)
**	    Propagate E_DM5423_SRID_MISMATCH errors.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	01-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**      10-Feb-2010 (maspa05) bug 122651, trac 442
**          Changed test for iisequence for physical locking to a more
**          generic one using a new relstat2 flag, TCB2_PHYSLOCK_CONCUR
**      14-May-2010 (stial01)
**          More changes for Long IDs
**	25-May-2010 (kschendel)
**	    Add missing mh include.
**      09-Jun-2010 (stial01) (B123905)
**          dm2r_position() use logical value locks if not releasing
**          dm2r_put() release value lock after put (not after allocate)
**          base_delete_put() release value lock after put (not after allocate)
**      12-Jul-2010 (stial01) (SIR 121619 MVCC, B124076)
**          dm2r_replace() if row/crow_locking, defer_add_new() else dmpp_dput()
**      19-Jul-2010 (thich01)
**          Need to setup a coupon when inserting into a table with an rtree
**          index already associated with it.
**	20-Aug-2010 (miket) SIR 122403 SD 145904
**	    Encrypted indexes get the encryption shm slot number from the base
**	    table parent.
**	31-Aug-2010 (miket) SIR 122403
**	    For dm1e_aes calls follow dmf convention of breaking or
**	    continuing with status test rather than returning on error.
*/

static DB_STATUS BuildRtreeRecord(
    				DMP_TCB		*t,
    				DMP_TCB		*it,
    				DMP_RCB         *ir,
    				char		*brec,
    				char		*rec);

static DB_STATUS dm2r_fetch_record(
				DMP_RCB             *rcb,
				DM_TID              *tid,
				i4             flag,
				i4             opflag,
				DB_ERROR            *dberr );

static DB_STATUS base_replace(
				DMP_RCB     *rcb,
				i4     action,
				i4     flag,
				char        *newrecord,
				i4     oldlength,
				i4     newlentgh,
				DM_TID      *oldtid,
				DM_TID      *oldbid,
				DM_TID      *newtid,
				DM_TID      *newbid,
				DMP_PINFO   *newdata_p,
				DMP_PINFO   *newleaf_p,
				i4	    delta_start,
				i4	    delta_end,
				DB_ERROR    *dberr);

static DB_STATUS base_delete_put(
				DMP_RCB     *rcb,
				i4     action,
				i4     flag,
				char        *newrecord,
				i4     oldlength,
				i4     newlength,
				DM_TID      *oldtid,
				DM_TID      *oldbid,
				DM_TID      *newtid,
				DM_TID      *newbid,
				DMP_PINFO   *newdata_p,
				DMP_PINFO   *newleaf_p,
				DB_ERROR    *dberr);

static DB_STATUS si_put(
				DMP_TCB	    *it,
				DMP_RCB     *rcb,
				char        *record,
				i4	    flag,
				DB_ERROR    *dberr);

static DB_STATUS si_delete(
				DMP_TCB	    *it,
				DMP_RCB     *rcb,
				i4	    flag,
				DB_ERROR    *dberr);

static DB_STATUS si_replace(
				DMP_TCB	    *it,
				DMP_RCB     *rcb,
				char        *record,
				i4	    flag,
				DB_ERROR    *dberr);

static DB_STATUS si_dupcheck(
				DMP_TCB	        *it,
				DMP_RCB		*rcb,
				char        	*record,
				i4		flag,
				DB_ERROR    *dberr);

static DB_STATUS rename_load_tab(
				DMP_RCB         *rcb,
				DMP_LOCATION    *old_loc,
				DMP_LOCATION    *mod_loc,
				i4         loc_count,
				DB_ERROR    *dberr);

static DB_STATUS update_load_tab(
				DMP_RCB         *rcb,
				DMP_RELATION    *relrecord,
				DB_ERROR    *dberr);

static DB_STATUS invalidate_load_tab(
				i4		is_rebuild,
				DMP_RCB		*rcb,
				DB_ERROR	*dberr);

#ifdef NEVER_USED
/* Probably should just remove, but might yet be of academic interest: */
static void dm2r_schedule_prefetch(
				DMP_RCB		*r,
				i4		prefet_type);

static void dm2r_cancel_prefetch(
				DMP_RCB		*r,
				i4		item);
#endif /* NEVER_USED */

static DB_STATUS rtree_position(
    				DMP_RCB		*rcb,
    				DM2R_KEY_DESC	*key_desc,
    				i4		key_count,
				DB_ERROR    *dberr);

static DB_STATUS SIcomplete(
				DMP_RCB		*r, 
				DB_STATUS	status,
				DB_ERROR    *dberr);


/*{
** Name: dm2r_rcb_allocate - Allocate an RCB.
**
** Description:
**      This function allocates a RCB from the TCB free RCB queue.  If no more
**      RCB's are on the queue, an RCB is allocated from dynamic memory.
**      The RCB is initialized and queued to the TCB active RCB queue.
**	This routine must be called either holding the HCB mutex, or
**	(preferably) with the TCB ref count greater then zero.
**
**      The RCB is initialized with the following characteristics:
**	    update mode = DIRECT
**	    access mode = WRITE
**	    lock level  = PAGE
**	    lock mode	= IX
**	    max locks   = 10
**	    timeout     = 0
**	If something else is desired, the caller can make whatever changes are
**	desired.
**
** Inputs:
**      tcb                             The TCB to allocate an RCB from.
**	base_rcb			Parent RCB from which this rcb should
**					inherit session information.
**	tran_id				Session's transaction id.
**	lock_id				Session's lock list.
**	log_id				Session's log id.
**
** Outputs:
**      rcb                             The allocated RCB.
**	err_code			Reason for error return status.
**					    E_DB0001_ALLOC_MEM_ERROR
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
**	17-dec-85 (derek)
**          Created new for jupiter.
**	31-mar-87 (ac)
**	    Added read/nolock support.
**	 7-nov-88 (rogerk)
**	    Added rcb_compress field.
**	12-may-89 (anton)
**	    clear rcb_collation descriptor field
**	15-aug-89 (rogerk)
**	    Add support for Gateways.  Set rcb_logging according to tcb_logging
**	    value in TCB.  Initialize rcb_gateway_rsb.
**	11-jan-1989 (rogerk)
**	    Only do BI logging of BTREE leaf pages if we are doing online
**	    backup.  The rcb_bt_bi_logging is enabled if online backup is
**	    going on, and the dm0l_bi() routine is only called if this flag
**	    is enabled.  Initialize the rcb_bt_bi_logging field to zero
**	    when allocating the rcb.
**	25-jan-1990 (fred)
**	    Added support for peripheral operations.  Initialize rcb_pcb field.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-1990 (rogerk)
**		Allocate ADF_CB and attach to the RCB.  This will be used for
**		all ADF calls made on this table rather than allocating locally
**		all over dmf.  Initialize the adf control block to default
**		values.  This was done as part of the DBMS Timezone changes.
**	    17-dec-1990 (jas)
**		Smart Disk project integration:
**		Initialize the rcb_sdpage field to NULL when allocating an RCB.
**	     4-feb-1991 (rogerk)
**		Don't set rcb_logging to TRUE for a secondary index if the
**		table is specifically marked as a nologging table - even if
**		the base table is logged.
**	09-jun-1992 (kwatts)
**	    Set rcb_compress to new values based on the iirelation field
**	    relcomptype, introduced for multiple page format.
**      25-sep-1992 (stevet)
**          Changed svcb_timezone to svcb_tzcb to support new timezone
**          adjustment methods.
**	05-nov-1992 (jnash)
**	    Temp hack to init rcb_recovery, used only until new recovery
**	    code in place.
**	14-dec-1992 (jnash)
**	    Reduced logging project.
**	      - Initialize the rcb_journal field and propagate the journal
**		status from base table tcbs into the index tcbs.
**	      - Added rcb_usertab_jnl field.
**	      - Removed temp use of rcb_recovery flag.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed old rcb_abt_sp_addr field used with BI recovery.
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**	19-july-1995 (b69768)(carly01)(from change 274922 in ingres63p codeline)
**		Transaction id assignment should be a STRUCT_ASSIGN_MACRO.
**	30-jul-96 (nick)
**	    Clear the new RCB's ADF warning area.
**	13-sep-1996 (canor01)
**	    Allocate a tuple buffer for data uncompression as part of RCB.
**	12-nov-1996 (hanch04)
**	    Set replicator parameters to NULL in dm2r_rcb_allocate
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Init new fields in RCB:
**      25-nov-1996 (stial01)
**          dm2r_rcb_allocate() fixed init of key buffers, adf_cb
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - initialize rcb_iso_level field in RCB;
**          - inherit rcb_iso_level from base_rcb.
**      13-mar-97 (stial01)
**          Fixed init of adf_cb. (again)
**	10-Jan-1997 (kitch01)
**	    Manual integration of fix for bug 43950. This fixes bugs 78094
**	    and 79891. Inherit the clooation and timezone values from
**	    base_rcb.
**      31-Jan-1997 (merja01)
**          Initialize adf_dfmt to zero to prevent segmentation 
**          violations using the ADF DATE function.
**      10-mar-97 (stial01)
**          Pass relwid to dm0m_tballoc()
**	14-May-1997 (jenjo02)
**	    Inherit rcb_access_mode from base_rcb instead of forcing it to
**	    RCB_A_WRITE.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Initialize new fields added to the RCB for this project. 
**	30-may-1997 (nanpr01)
**	    Set new replicator parameters to NULL in dm2r_rcb_allocate.
**      13-jun-97 (dilma04)
**          Do not inherit row locking from base rcb if it is not allowed
**          on secondary index. 
**	18-jun-97 (stephenb)
**	    Replicator fields were only being intialized if we needed to
**	    allocate memory for a new RCB rather than using one from the
**	    free queue, this causes problems in replicator since we no
**	    longer initialize them in dm2t_open.
**	11-aug-97 (nanpr01)
**	    Add the t->tcb_comp_atts_count with relwid in dm0m_tballoc call.
**      29-aug-97 (stial01)
**          B84909: If index is DM_COMPAT_PGSIZE, rcb_lk_type can't be RCB_K_ROW
**      20-jan-98 (stial01)
**          dm2r_rcb_allocate() If index is RTREE, rcb_lk_type can't be row.
**      28-may-98 (stial01)
**          dm2r_rcb_allocate() moved code to dm2t_row_lock()
**      07-jul-98 (stial01)
**          dm2r_rcb_allocate() dm2r_rcb_allocate() if base_rcb, 
**          inherit rcb_xcb_ptr (B63389)
**	11-Aug-1998 (jenjo02)
**	    dm2r_rcb_allocate(), initialize rcb_tup_adds, rcb_page_adds.
**      03-nov-99 (stial01)
**          Fix for 05-may-1999 change. In dm2r_replace, si_dupcheck may be
**          deferred until after updating base table. Make sure we do not
**          dupcheck if only tidp changed in index.
**	09-apr-2001 (abbjo03)
**	    Initialize the Unicode collation table.
**	22-Jan-2004 (jenjo02)
**	    Add, init new fields for parallel dmrThreads,
**	    partition row locking.
**	12-May-2004 (schka24)
**	    Init etab parent ID (dmpe will set it if need be).
**	28-Jul-2004 (schka24)
**	    Init rcb-locked-reltid for cleanliness.
**      09-may-2005 (horda03) Bug 114471/INGSRV3294
**          Initialize the rcb_temp_iirelation.
**	13-Feb-2007 (kschendel)
**	    Copy rcb sid for cancel checks, if copying from base rcb.
**	10-may-2007 (dougi)
**	    Init adf_utf8_flag in the "fake" ADF_CB.
**	11-May-2007 (gupsh01)
**	    Initialize adf_utf8_flag.
**	16-Apr-2008 (kschendel)
**	    Rearrange RCB layout code (layout got changed, then changed back,
**	    but the new way is easier to add stuff to, so I left it).
**	    Init default adf math exception state to arith-warnings.
**      21-may-2009 (huazh01)
**          init rcb_hashbuf to NULL. (b122075)
**	26-Feb-2010 (jonj)
**	    Init rcb_crib_ptr.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    dm2t_row_lock prototype change, pass proposed lock level.
**	    Init some critical stuff in the RCB before releasing
**	    the tcb_mutex.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Init bqcb ptr and manual EOR flag.  Drop row version.
**	4-jun-2010 (toumi01) SIR 122403
**	    Instead of TMget_stamp use TMhrnow, because the result of the
**	    former is supposed to be opaque (we should not reference
**	    tm_stamp.tms_usec). This fixes portability to VMS.
**	01-Sep-2010 (miket) SIR 122403
**	    Adjust encrypted record buffer to correct overrun flagged in
**	    dbms log ("Memory overrun rcb_hl_ptr").
**	07-Sep-2010 (miket) SIR 122403
**	    Replace AES_BLOCK (=16) lucky WAG for additional buffer required
**	    for rcb_erecord_ptr with the actual extra needed by MODIFY for
**	    tacking on hash bucket, partition no, and tid8 (=14). Since the
**	    three record buffers can end up being used in common ways while
**	    doing compression and encryption processing, make all three the
**	    same size.
*/
DB_STATUS
dm2r_rcb_allocate(
DMP_TCB             *tcb,
DMP_RCB		    *base_rcb,
DB_TRAN_ID	    *tran_id,
i4		    lock_id,
i4		    log_id,
DMP_RCB		    **rcb,
DB_ERROR	    *dberr )
{
    DMP_TCB             *t = tcb;
    DMP_RCB		*r;
    ADF_CB		*adf_ptr;
    DB_STATUS		status;
    CL_ERR_DESC          sys_err;
    bool		reclaimed;
    i4             key_buffers;
    i4			rec_buffers;
    i4                  klen;
    i4			seglen = 0;
    HRSYSTIME		hrtime;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9203_DM2R_RCB_ALLOC);
#endif

    if (t->tcb_seg_rows)
	seglen = t->tcb_rel.reltotwid + t->tcb_data_rac.worstcase_expansion;

    rec_buffers = 2;			/* record, srecord */    
    if ((t->tcb_rel.relencflags & TCB_ENCRYPTED) ||
	((t->tcb_parent_tcb_ptr) &&
	 (t->tcb_parent_tcb_ptr->tcb_rel.relencflags & TCB_ENCRYPTED)))
    {
	i4 tempi = MHrand2();
	rec_buffers++;			/* erecord */
	TMhrnow(&hrtime);
	/* seed the random generator: kick the can an arbitrary distance
	** (tv_nsec) down the road from an arbitrary starting point (tempi)
	*/
	MHsrand2(hrtime.tv_nsec * tempi);
    }

    if (t->tcb_table_type == TCB_BTREE)
	key_buffers = 5;
    else
	key_buffers = 3;

    klen = t->tcb_klen + 1;

    /*	Lock the TCB. */

    dm0s_mlock(&t->tcb_mutex);

    r = t->tcb_fq_next;
    r->rcb_q_next->rcb_q_prev = r->rcb_q_prev;
    r->rcb_q_prev->rcb_q_next = r->rcb_q_next;
    if (r == (DMP_RCB*)&t->tcb_fq_next)
    {
	char *p;

	/*  Unlock the TCB and allocate a new RCB. */

	dm0s_munlock(&t->tcb_mutex);

	reclaimed = FALSE;
	for (;;)
	{	
	    /* lint truncation warnings if size of ptr > int, but code valid */
	    status = dm0m_allocate(sizeof(DMP_RCB) +
		DB_ALIGN_MACRO(t->tcb_rel.relwid + MODIFY_BUCKET_PNO_TID8_SZ +
			t->tcb_data_rac.worstcase_expansion) * rec_buffers +
		DB_ALIGN_MACRO(klen) * key_buffers +
		sizeof(ADF_CB) + seglen,
		DM0M_ZERO, (i4)RCB_CB, (i4)RCB_ASCII_ID,
		(char *)t, (DM_OBJECT **)rcb, dberr);
	    if (status)
	    {
		if (dberr->err_code != E_DM923D_DM0M_NOMORE)
		    break;
		if (reclaimed == TRUE)
		    break;
	
		status = dm2t_reclaim_tcb(lock_id, log_id, dberr);
		if (status != E_DB_OK)
		    break;
		reclaimed = TRUE;
	    }
	    else
	    {
		break;
	    }
	}
	if (status != E_DB_OK)
	{
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
	    	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM9203_DM2R_RCB_ALLOC);
	    }
	    return (status);
	}
	r = *rcb;
	r->rcb_tcb_ptr = t;
	p = (char *) r + sizeof(DMP_RCB);

	r->rcb_record_ptr = p;
	p += t->tcb_rel.relwid + t->tcb_data_rac.worstcase_expansion +
	    MODIFY_BUCKET_PNO_TID8_SZ;
	p = ME_ALIGN_MACRO(p, sizeof(PTR));

	r->rcb_srecord_ptr = p;
	p += t->tcb_rel.relwid + t->tcb_data_rac.worstcase_expansion +
	    MODIFY_BUCKET_PNO_TID8_SZ;
	p = ME_ALIGN_MACRO(p, sizeof(PTR));

	if (rec_buffers == 3)		/* encryption */
	{
	    r->rcb_erecord_ptr = p;
	    p += t->tcb_rel.relwid + t->tcb_data_rac.worstcase_expansion +
		MODIFY_BUCKET_PNO_TID8_SZ;
	    p = ME_ALIGN_MACRO(p, sizeof(PTR));
	}

	r->rcb_hl_ptr = p;
	*(char *)(r->rcb_hl_ptr + (klen-1)) = 0xFE;
	p += klen;
	p = ME_ALIGN_MACRO(p, sizeof(PTR));

	r->rcb_ll_ptr = p;
	*(char *)(r->rcb_ll_ptr + (klen-1)) = 0xFE;
	p += klen;
	p = ME_ALIGN_MACRO(p, sizeof(PTR));

	r->rcb_s_key_ptr = p;
	*(char *)(r->rcb_s_key_ptr + (klen-1)) = 0xFE;
	p += klen;
	p = ME_ALIGN_MACRO(p, sizeof(PTR));

	if (t->tcb_table_type == TCB_BTREE)
	{
	    r->rcb_repos_key_ptr = p;
	    *(char *)(r->rcb_repos_key_ptr + (klen-1)) = 0xFE;
	    p += klen;
	    p = ME_ALIGN_MACRO(p, sizeof(PTR));

	    r->rcb_fet_key_ptr = p;
	    *(char *)(r->rcb_fet_key_ptr + (klen-1)) = 0xFE;
	    p += klen;
	    p = ME_ALIGN_MACRO(p, sizeof(PTR));
	}
	r->rcb_adf_cb = (ADF_CB *) p;
	p += sizeof(ADF_CB);

	r->rcb_tupbuf = NULL;
        r->rcb_hashbuf = NULL;

	if (seglen)
	{
	    r->rcb_segbuf = p;
	}
	else
	    r->rcb_segbuf = NULL;

	/* Re-init rcb_new_max and rcb_new_tids only for new RCB's */
	r->rcb_new_max = RCB_TMP_TID_MAX;
	r->rcb_new_tids = &r->rcb_tmp_tids[0];

	/*  Relock the TCB for insertion of new RCB onto the active queue. */

	dm0s_mlock(&t->tcb_mutex);
    }
    else
    {
#ifdef xDEBUG
	/* Re-use rcb */
	if (r->rcb_new_tids != &r->rcb_tmp_tids[0])
	{
	    TRdisplay("Re-use tid list for table %d used %d %x\n", 
		r->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base,r->rcb_new_cnt, 
		(char *)r->rcb_new_tids - sizeof(DMP_MISC));
	}
#endif

	/*  Deallocate memory used to do a DMF projection */
	if (r->rcb_proj_misc)
	{
	    dm0m_deallocate((DM_OBJECT **)&r->rcb_proj_misc);
	    r->rcb_proj_misc = NULL;
	}

	if (r->rcb_bulk_misc)
	{
	    dm0m_deallocate((DM_OBJECT **)&r->rcb_bulk_misc);
	    r->rcb_bulk_misc = NULL;
	}
    }

    /* Whether new or reused, clear out possibly-update list links */
    r->rcb_upd_list.q_next = NULL;
    r->rcb_upd_list.q_prev = NULL;
    r->rcb_mupd_list.q_next = NULL;
    r->rcb_mupd_list.q_prev = NULL;

    /*
    ** Set the TCB's "real-time" page and tuple counts in the RCB.
    ** These numbers include the current values from the TCB
    ** as well the counts from any active RCBs that are not
    ** being aborted. These values are passed to QEF by
    ** dmt_open() to validate query plans.
    ** Note that the active RCBs' counts may be changing
    ** while we do this, so it's not 100% accurate!
    **
    ** Since the TCB's tcb_tup_adds and tcb_page_adds are not
    ** thread-safe, the tuple and page count changes are now kept
    ** in the RCB, which is thread safe, and the TCB's 
    ** corresponding values updated under the protection of
    ** tcb_mutex when the RCB is released.
    ** 
    ** If a transaction aborts, dmxe_abort() will clear
    ** the tuple and page add counts in each RCB, negating
    ** the transaction's effect on the TCB counts.
    **
    ** Unfortunately, secondary index RCBs do not persist for
    ** the life of the txn, but are allocated and released
    ** for each index operation, so the opportunity exists
    ** for secondary index TCB tuple and page counts to be off.
    ** For example, when an index entry is added, an RCB
    ** is allocated, the tuple added to the table and 
    ** rcb_tup_adds incremented, and the RCB is released.
    ** If the txn aborts after this point, there is no way
    ** to undo the rcb_tup_adds increment short of involving
    ** the recovery routines with the maintenance of the TCB
    ** count fields or making the index RCB persist for
    ** the life of the transaction (link it to the XCB's
    ** RCB queue, e.g.). So for now we'll just have to
    ** accept that secondary index tuple and page counts
    ** might get out of whack.
    */
    dm2t_current_counts(t, &r->rcb_reltups, &r->rcb_relpages);
    r->rcb_tup_adds = 0;
    r->rcb_page_adds = 0;
    
    /*	Queue the RCB to the TCB used queue. */

    r->rcb_q_next = t->tcb_rq_next;
    r->rcb_q_prev = (DMP_RCB*)&t->tcb_rq_next;
    t->tcb_rq_next->rcb_q_prev = r;
    t->tcb_rq_next = r;

    /* Init this stuff before releasing the mutex: */

    STRUCT_ASSIGN_MACRO(*tran_id, r->rcb_tran_id); 	/*69768*/
    r->rcb_lk_id = lock_id;
    r->rcb_log_id = log_id;
    r->rcb_xcb_ptr = (DML_XCB *)NULL;
    r->rcb_lk_type = RCB_K_PAGE;
    r->rcb_lk_mode = RCB_K_IX;
    r->rcb_k_type = RCB_K_PAGE;
    r->rcb_k_mode = RCB_K_IX;

    /* MVCC stuff: */
    /* Assume not shared transaction */
    r->rcb_slog_id_id = LOG_ID_ID(log_id);
    r->rcb_dmr_opcode = 0;
    r->rcb_crow_tid.tid_i4 = -1;
    r->rcb_mvcc_lkid.lk_unique = 0;
    r->rcb_mvcc_lkid.lk_common = 0;
    r->rcb_crib_ptr = (LG_CRIB*)NULL;

    /*	Now unlock the TCB. */

    dm0s_munlock(&t->tcb_mutex);

    DMP_PINIT(&r->rcb_data);
    DMP_PINIT(&r->rcb_other);
    r->rcb_sid = (CS_SID) 0;
    r->rcb_iso_level = RCB_SERIALIZABLE;
    r->rcb_update_mode = RCB_U_DIRECT;
    r->rcb_access_mode = RCB_A_WRITE;
    r->rcb_lk_limit = 10;
    r->rcb_lk_count = 0;
    r->rcb_timeout = 0;
    r->rcb_rl_next = r;
    r->rcb_f_qual = NULL;
    r->rcb_f_arg = NULL;
    r->rcb_state = RCB_OPEN;
    r->rcb_hl_given = 0;
    r->rcb_ll_given = 0;
    r->rcb_internal_req = RCB_INTERNAL;
    r->rcb_k_duration = RCB_K_TRAN;
    r->rcb_uiptr = &r->rcb_uidefault;
    r->rcb_uidefault = 0;
    r->rcb_rnl_cb = NULL;
    r->rcb_1rnl_page = 0;
    r->rcb_2rnl_page = 0;
    r->rcb_3rnl_page = 0;
    r->rcb_hash_nofull = -1;
    r->rcb_collation = NULL;
    r->rcb_ucollation = NULL;
    r->rcb_gateway_rsb = NULL;
    r->rcb_val_logkey = 0;
    r->rcb_usertab_jnl = 0;
    r->rcb_temp_iirelation = 0;
    DM1B_POSITION_INIT_MACRO(r, RCB_P_START);
    DM1B_POSITION_INIT_MACRO(r, RCB_P_GET);
    DM1B_POSITION_INIT_MACRO(r, RCB_P_FETCH);
    DM1B_POSITION_INIT_MACRO(r, RCB_P_ALLOC);
    r->rcb_locked_data = 0;
    r->rcb_data_lkid.lk_unique = 0;
    r->rcb_data_lkid.lk_common = 0;
    r->rcb_csrr_flags = 0;
    r->rcb_locked_leaf = 0;
    r->rcb_leaf_lkid.lk_unique = 0;
    r->rcb_leaf_lkid.lk_common = 0;
    r->rcb_row_lkid.lk_unique = 0;
    r->rcb_row_lkid.lk_common = 0;
    r->rcb_base_lkid.lk_unique = 0;
    r->rcb_base_lkid.lk_common = 0;
    r->rcb_val_lkid.lk_unique = 0;
    r->rcb_val_lkid.lk_common = 0;
    r->rcb_locked_tid.tid_i4 = -1;
    r->rcb_locked_tid_reltid.db_tab_base = 0;
    r->rcb_locked_tid_reltid.db_tab_index = 0;
    r->rep_shad_rcb    = NULL;
    r->rep_arch_rcb    = NULL;
    r->rep_shadidx_rcb = NULL;
    r->rep_prio_rcb    = NULL;
    r->rep_cdds_rcb    = NULL;
    r->rcb_new_cnt = 0;
    r->rcb_new_fullscan = 0;
    r->rcb_opt_extend = t->tcb_rel.relextend;
    r->rcb_bulk_cnt = 0;
    r->rcb_bulk_misc = NULL;
    r->rcb_rnl_online = (DMP_RNL_ONLINE *)0;
    r->rcb_et_parent_id = 0;
    r->rcb_temp_iirelation = 0;
    r->rcb_bqcb_ptr = NULL;	/* Caller will figure this one out */
    r->rcb_manual_endrow = 0;

    /* 
    ** Initialize fields used to do a DMF projection.
    ** (Currently only done during parallel index build)
    ** Initialize RCB to retrieve the ordinary full data row.
    */
    r->rcb_data_rac = &t->tcb_data_rac;
    r->rcb_proj_misc = NULL;
    r->rcb_proj_atts_ptr = NULL;
    r->rcb_proj_relwid = t->tcb_rel.relwid;
    r->rcb_proj_relversion = t->tcb_rel.relversion;

    /* Init fields used for secondary index processing */
    r->rcb_si_oldtid.tid_i4 = NOT_ALLOCATED;
    r->rcb_si_newtid.tid_i4 = NOT_ALLOCATED;
    r->rcb_si_flags = 0;
    r->rcb_compare = 0;

    /* Init fields used by dmrAgents */
    r->rcb_rtb_ptr = NULL;
    r->rcb_dmrAgent = FALSE;
    r->rcb_siAgents = FALSE;

    /* Init stuff for partition row locking */
    r->rcb_reltid = (DB_TAB_ID*)NULL;
    r->rcb_partno = 0;

    /*
    ** Set logging actions based on the table type.
    ** Temporary tables and tables marked no-logging are not logged.
    */
    r->rcb_logging = RCB_LOGGING;
    if ( ! t->tcb_logging)
	r->rcb_logging = 0;

    /*
    ** Set the RCB journaling flag according to the Table's JOURNAL
    ** status in iirelation.relstat.  This indicates whether log records
    ** updates to this table will be archived to the journal files.
    **
    ** Note that special journal handling is done for core system catalogs:
    ** log records written for them may not be journaled even though this
    ** journaling flag is set (the journal status is taken from the user
    ** table rcb).
    */
    r->rcb_journal = (t->tcb_rel.relstat & TCB_JOURNAL) ? RCB_JOURNAL : 0;

    /*
    ** Allocate a tuple buffer for compression/uncompression if required.
    ** Allocate a tuple buffer if the table has been altered and tuple
    ** conversion may be necessary
    */
    if ( r->rcb_tupbuf == NULL && 
	( t->tcb_data_rac.compression_type != TCB_C_NONE
	  || t->tcb_rel.relversion != 0) )
	r->rcb_tupbuf = dm0m_tballoc(t->tcb_rel.relwid +
					t->tcb_data_rac.worstcase_expansion);

    /*
    ** Init adf control block to use for operations on this table.
    ** Set default values for adf variables - collation and timezone.
    ** These defaults may be overridden by the caller.
    */
    adf_ptr = r->rcb_adf_cb;
    adf_ptr->adf_errcb.ad_ebuflen = 0;
    adf_ptr->adf_errcb.ad_emsglen = 0;
    adf_ptr->adf_errcb.ad_errmsgp = NULL;
    adf_ptr->adf_maxstring = DB_MAXSTRING;
    adf_ptr->adf_collation = NULL;
    adf_ptr->adf_ucollation = t->tcb_dcb_ptr->dcb_ucollation;
    if (adf_ptr->adf_ucollation)
    {
	if (t->tcb_dcb_ptr->dcb_dbservice & DU_UNICODE_NFC)
	    adf_ptr->adf_uninorm_flag = AD_UNINORM_NFC;
	else adf_ptr->adf_uninorm_flag = AD_UNINORM_NFD;
    }
    else adf_ptr->adf_uninorm_flag = 0;
    adf_ptr->adf_tzcb = dmf_svcb->svcb_tzcb;
    adf_ptr->adf_dfmt = 0;
    if (CMischarset_utf8())
       adf_ptr->adf_utf8_flag = AD_UTF8_ENABLED;
    else
       adf_ptr->adf_utf8_flag = 0;
    adf_ptr->adf_exmathopt = ADX_IGN_MATHEX;

    MEfill(sizeof(ADI_WARN), '\0', &adf_ptr->adf_warncb);

    /*	Set any inherited information. */

    if ( r->rcb_base_rcb = base_rcb )
    {
	r->rcb_xcb_ptr = base_rcb->rcb_xcb_ptr;
	r->rcb_slog_id_id = base_rcb->rcb_slog_id_id;
	r->rcb_dmr_opcode = base_rcb->rcb_dmr_opcode;
	if ( base_rcb->rcb_state & RCB_CONSTRAINT )
	    r->rcb_state |= RCB_CONSTRAINT;
	r->rcb_mvcc_lkid = base_rcb->rcb_mvcc_lkid;
	r->rcb_crib_ptr = base_rcb->rcb_crib_ptr;
	r->rcb_sid = base_rcb->rcb_sid;
	r->rcb_update_mode = base_rcb->rcb_update_mode;
        r->rcb_iso_level = base_rcb->rcb_iso_level;
	r->rcb_lk_mode = base_rcb->rcb_lk_mode;
	r->rcb_lk_type = base_rcb->rcb_lk_type;
	r->rcb_k_type = base_rcb->rcb_k_type;

	/*
	** If RCB_K_(C)ROW, make sure row locking supported for this table
	*/
	if ( (row_locking(r) || crow_locking(r)) && !dm2t_row_lock(t, r->rcb_k_type) )
	    r->rcb_lk_type = RCB_K_PAGE;

	r->rcb_k_type = r->rcb_lk_type;
	r->rcb_k_mode = base_rcb->rcb_k_mode;
	r->rcb_timeout = base_rcb->rcb_timeout;
	r->rcb_lk_limit = base_rcb->rcb_lk_limit;
	r->rcb_k_duration = base_rcb->rcb_k_duration;
	r->rcb_seq_number = base_rcb->rcb_seq_number;
	r->rcb_collation = base_rcb->rcb_collation;
	r->rcb_ucollation = base_rcb->rcb_ucollation;
	r->rcb_access_mode = base_rcb->rcb_access_mode;

	/*
	** Override the collation and timezone values by the base table
	** settings.
	*/
	adf_ptr->adf_collation=base_rcb->rcb_adf_cb->adf_collation;
	adf_ptr->adf_ucollation=base_rcb->rcb_adf_cb->adf_ucollation;
	adf_ptr->adf_uninorm_flag=base_rcb->rcb_adf_cb-> adf_uninorm_flag;
	adf_ptr->adf_utf8_flag=base_rcb->rcb_adf_cb-> adf_utf8_flag;
	adf_ptr->adf_tzcb=base_rcb->rcb_adf_cb->adf_tzcb;

	/*
	** The table journal status is derived from the journal state
	** of the base table.
	*/
	r->rcb_journal = is_journal(base_rcb);
	r->rcb_usertab_jnl = base_rcb->rcb_usertab_jnl;

	/*
	** Set logging state - base table logging state should not
	** override specific table nologging state.
	*/
	r->rcb_logging = base_rcb->rcb_logging;
	if ( ! t->tcb_logging)
	    r->rcb_logging = 0;

	/*
	** Put this RCB in the ring of RCB's accessing the
	** secondary index if the base rcb has an XCB.
	*/

	if (base_rcb->rcb_xcb_ptr)
	{
	    DML_XCB	    *x = base_rcb->rcb_xcb_ptr;
	    DML_XCB	    *next_r_queue;
	    DMP_RCB	    *next_rcb;

	    /* Scan for RCB's in this transaction with the same TCB */

	    for (next_r_queue = (DML_XCB*) x->xcb_rq_next;
		next_r_queue != (DML_XCB*) &x->xcb_rq_next;
		next_r_queue = next_r_queue->xcb_q_next)
	    {
		/*  Calculate RCB starting address. */

		next_rcb = (DMP_RCB *)((char *)next_r_queue -
		    ((char *)&((DMP_RCB*)0)->rcb_xq_next));
		if (next_rcb->rcb_tcb_ptr == t)
		{
		    DMP_RCB	    *save_next;

		    save_next = next_rcb->rcb_rl_next;
		    next_rcb->rcb_rl_next = r->rcb_rl_next;
		    r->rcb_rl_next = save_next;
		    break;
		}
	    }
	}
    }

    /*
    ** Set locking actions based on the table type.
    */

    /* use physical locking for core catalogs and those
    ** specifically flagged for physical locking
    */
    if (t->tcb_rel.relstat & TCB_CONCUR ||
        t->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR)
	r->rcb_k_duration = RCB_K_PHYSICAL;

    if (t->tcb_dcb_ptr->dcb_status & DCB_S_EXCLUSIVE ||
	t->tcb_temporary == TCB_TEMPORARY)
    {
	/*  Set table locking to avoid setting page locks. */

	r->rcb_lk_type = RCB_K_TABLE;
	r->rcb_lk_mode = RCB_K_X;
	r->rcb_k_type = RCB_K_TABLE;
	r->rcb_k_mode = RCB_K_X;
    }

    /* If rcb is a candidate for readahead, set indicator in status */
    if ( (dmf_svcb->svcb_status & SVCB_READAHEAD) &&
	((t->tcb_rel.relstat & TCB_CONCUR) == 0))
	r->rcb_state |= RCB_PREFET_AVAILABLE;

    r->rcb_ra_idxupd = 0;	/* bitmap of indexes last updated */

    status = LGshow(LG_S_OLD_LSN, (PTR)&r->rcb_oldest_lsn, 
		sizeof(r->rcb_oldest_lsn), &klen, &sys_err);	
    if (status != E_DB_OK)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, (char *)NULL, (i4)0, 
			(i4 *)NULL, err_code, 0);
	status = E_DB_ERROR;
	return(status);
    }

    /* Aw go ahead and init and track those stats */
    r->rcb_s_fix = 0;
    r->rcb_s_ins = 0;
    r->rcb_s_io = 0;
    r->rcb_s_qual = 0;
    r->rcb_s_scan = 0;
    r->rcb_s_load = 0;
    r->rcb_s_get = 0;
    r->rcb_s_del = 0;
    r->rcb_s_rep = 0;

    *rcb = r;
    return (E_DB_OK);
}

/*
** {
** Name: dm2r_release_rcb	- Release an RCB.
**
** Description:
**      This function deallocates an RCB from the active/free queue.
**	Any pages that are fixed in the RCB data page and other page
**	pointers are released with the option DM0P_UNFIX.
**
** Inputs:
**      rcb                             The RCB to de deallocated.
**
** Outputs:
**      err_code                        Reason for error status.
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
**	17-dec-85 (derek)
**          Created new for jupiter.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  When release rcb for a gateway
**	    table, make sure that the gateway has been notified.
**	25-jan-1990 (fred)
**	    Added support for peripheral operations.  Deallocate any associated
**	    peripheral action (indicated by the rcb_pcb field).
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    15-jan-91 (jas)
**		Added check for Smart Disk resources in use; free buffers and
**		hardware if so.
**	    02-may-91 (kwatts)
**		Smart Disk changes: pass pseudo page to SDfree.
**	    11-sep-91 (kwatts)
**		Call of SDfree uses the loc_fcb which may by now be null if
**		we are releasing everything, including the TCB. Add code to
**		check and not call SDfree if there is no FCB around.
**      27-Oct-1993 (fred)
**          Added looping in the deallocation of associated peripheral
**          control blocks.  This allows for multiple streams
**          during the processing of large object.  Also, use
**          dmpe_deallocate() instead of dm0m_deallocate().  This
**          enforces the correct semantics for deallocation of
**          peripheral objects.
**	09-May-1995 (cohmi01)
**	    Cancel any pending prefetch event for this RCB, its too late now.
**	13-sep-1996 (canor01)
**	    Deallocate RCB's data uncompression tuple buffer.
**	11-Aug-1998 (jenjo02)
**	    Roll rcb_tup_adds, rcb_page_adds into TCB.
**	    If the RCB is free (not RCB_OPEN), skip all the machinations
**	    and just free the memory.
**      06-oct-98 (stial01)
**          dm2r_release_rcb() rcb_tupbuf memory leak
**	16-Jan-2004 (schka24)
**	    Unlink from possibly-updating lists.
**	27-May-2004 (jenjo02)
**	    Cache core catalog RCBs rather than deallocating them.
**      21-may-2009 (huazh01)
**          release memory allocated for rcb_hashbuf. (b122075)
**	26-Feb-2010 (jonj)
**	    If cursor CRIB, decrement inuse count; if zero,
**	    delink and deallocate.
*/
DB_STATUS
dm2r_release_rcb(
DMP_RCB            **rcb,
DB_ERROR           *dberr )
{
    DMP_RCB	    *r = *rcb;
    DB_STATUS	    status = E_DB_OK;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9310_DM2R_RELEASE_RCB);
#endif

    /*
    ** If this RCB is for one of the core catalogs and
    ** the underlying base table is not being closed
    ** (indicated by tcb_ref_count == 0), then keep the
    ** RCB cached on the TCB rather than constantly
    ** freeing and acquiring memory for the RCB.
    */
    if ( t->tcb_rel.reltid.db_tab_base <= DM_B_INDEX_TAB_ID &&
	 t->tcb_parent_tcb_ptr->tcb_ref_count > 0 )
    {
	return(dm2r_return_rcb(rcb, dberr));
    }

    if (r->rcb_state & RCB_OPEN)
    {
	if (r->rcb_data.page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data,
				     dberr);
	}
	if (status == E_DB_OK && r->rcb_other.page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
				     dberr);
	}
	if (status != E_DB_OK)
	    return (status);

	/*
	** If rcb is still associated with a gateway table, then release the
	** connection to the foreign table.
	** If the gateway returns an error, then log it, but continue with the
	** release.
	*/
	if (r->rcb_gateway_rsb && (t->tcb_rel.relstat & TCB_GATEWAY))
	{
	    status = dmf_gwt_close(r, dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		status = E_DB_OK;
	    }
	}

#ifdef NEVER_USED
	/* cancel all outstanding prefetch readahead requests for this rcb */
	if (r->rcb_state & RCB_PREFET_PENDING)
	    dm2r_cancel_prefetch(r, PFR_CANCEL_ALL);
#endif

	/* 
	** If updated session temp table, update
	** savepoint id in table's XCCB.
	**
	** This is used by dmx_abort() to decide which 
	** session temporary tables must be destroyed
	** during transaction abort or rollback <to savepoint>.
	*/
	if ( t->tcb_rel.relstat2 & TCB_SESSION_TEMP &&
	     r->rcb_state & RCB_TABLE_UPDATED )
	    t->tcb_temp_xccb->xccb_sp_id = r->rcb_xcb_ptr->xcb_sp_id;

	/*	Remove RCB from transaction RCB ring. */

	if (r->rcb_rl_next != r)
	{
	    DMP_RCB		*next_rcb;

	    /* Find RCB preceeding this RCB. */

	    next_rcb = r->rcb_rl_next;
	    while (next_rcb->rcb_rl_next != r)
		next_rcb = next_rcb->rcb_rl_next;
	    next_rcb->rcb_rl_next = r->rcb_rl_next;
	    r->rcb_rl_next = r;
	}

    }

    if (t->tcb_table_type == TCB_BTREE)
    {
	if (r->rcb_hl_ptr && *(r->rcb_hl_ptr + t->tcb_klen) != (char)0xFE)
	    TRdisplay("Memory overrun rcb_hl_ptr\n");
	if (r->rcb_ll_ptr && *(r->rcb_ll_ptr + t->tcb_klen) != (char)0xFE)
	    TRdisplay("Memory overrun rcb_ll_ptr\n");
	if (r->rcb_s_key_ptr && *(r->rcb_s_key_ptr + t->tcb_klen) != (char)0xFE)
	    TRdisplay("Memory overrun rcb_s_key_ptr\n");
	if (r->rcb_fet_key_ptr && *(r->rcb_fet_key_ptr + t->tcb_klen) != (char)0xFE)
	    TRdisplay("Memory overrun rcb_fet_key_ptr\n");
	if (r->rcb_repos_key_ptr && *(r->rcb_repos_key_ptr + t->tcb_klen) != (char)0xFE)
	    TRdisplay("Memory overrun rcb_repos_key_ptr\n");
    }

    if ( r->rcb_tupbuf )
    {
	/*
	** If there is an associated tuple buffer for compression,
	** then deallocate it.
	*/
	dm0m_tbdealloc( &r->rcb_tupbuf );
    }

    if (r->rcb_hashbuf)
    {
       MEfree((PTR)r->rcb_hashbuf); 
       r->rcb_hashbuf = NULL;
    }

    if (r->rcb_new_tids != &r->rcb_tmp_tids[0])
    {
	DMP_MISC *misc;
	misc = (DMP_MISC *)((char *)r->rcb_new_tids - sizeof(DMP_MISC));
#ifdef xDEBUG
	TRdisplay("Dealloc tid list for table %d used %d %x\n", 
	    r->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base,r->rcb_new_cnt, misc);
#endif
	dm0m_deallocate((DM_OBJECT **)&misc);
	r->rcb_new_tids = (DM_TID *)0;
    }

    /*  Deallocate memory used to do a DMF projection */
    if (r->rcb_proj_misc)
    {
	dm0m_deallocate((DM_OBJECT **)&r->rcb_proj_misc);
	r->rcb_proj_misc = NULL;
    }

    if (r->rcb_bulk_misc)
    {
	dm0m_deallocate((DM_OBJECT **)&r->rcb_bulk_misc);
	r->rcb_bulk_misc = NULL;
    }

    /* Decrement cursor CRIB inuse count */
    if ( r->rcb_crib_ptr && r->rcb_crib_ptr->crib_cursid )
    {
	LG_CRIB		*curscrib = r->rcb_crib_ptr;

	/* Decrement inuse count, if zero, deallocate CRIB */
        if ( --curscrib->crib_inuse == 0 )
	{
	    DM_OBJECT	*object;

	    /* Delink cursor CRIB from transaction's list */
	    curscrib->crib_next->crib_prev = curscrib->crib_prev;
	    curscrib->crib_prev->crib_next = curscrib->crib_next;

	    object = (DM_OBJECT*)((char*)curscrib - sizeof(DM_OBJECT));
	    dm0m_deallocate(&object);
	}
    }
    r->rcb_crib_ptr = (LG_CRIB*)NULL;

    /*	Dequeue the RCB from either TCB queue (active or free). */

    dm0s_mlock(&t->tcb_mutex);
    r->rcb_q_next->rcb_q_prev = r->rcb_q_prev;
    r->rcb_q_prev->rcb_q_next = r->rcb_q_next;

    /* Roll tup_adds and page_adds deltas into TCB while mutex held */
    t->tcb_tup_adds  += r->rcb_tup_adds;
    t->tcb_page_adds += r->rcb_page_adds;
    /* Remove from TCB's possibly-updating list used for counts gathering */
    if (r->rcb_upd_list.q_next != NULL)
    {   
	r->rcb_upd_list.q_next->q_prev = r->rcb_upd_list.q_prev;
	r->rcb_upd_list.q_prev->q_next = r->rcb_upd_list.q_next;
    }
    
    dm0s_munlock(&t->tcb_mutex);

    /* Same thing for partition-master list, if RCB is on one */
    if (r->rcb_mupd_list.q_next != NULL)
    {
	DMP_TCB *master_tcb = t->tcb_pmast_tcb;

	dm0s_mlock(&master_tcb->tcb_mutex);
	master_tcb->tcb_tup_adds  += r->rcb_tup_adds;
	master_tcb->tcb_page_adds += r->rcb_page_adds;
	r->rcb_mupd_list.q_next->q_prev = r->rcb_mupd_list.q_prev;
	r->rcb_mupd_list.q_prev->q_next = r->rcb_mupd_list.q_next;
	dm0s_munlock(&master_tcb->tcb_mutex);
    }


    /*	Deallocate the RCB. */

    dm0m_deallocate((DM_OBJECT **)rcb);

    return (E_DB_OK);
}

/*{
** Name: dm2r_return_rcb	- Return an RCB.
**
** Description:
**      This function returns a RCB to the free list from the allocated
**	list.  Any pages that are fixed in the RCB data page and other
**	page pointers are released with the option DM0P_UNFIX.
**
** Inputs:
**      rcb                             The RCB to be returned.
**
** Outputs:
**      err_code                        Reason for error status.
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
**	19-jan-88 (derek)
**          Created new for jupiter.
**	25-jan-1990 (fred)
**	    Added support for peripheral operations.  Deallocate any associated
**	    peripheral action (indicated by the rcb_pcb field).
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    15-jas-91 (jas)
**		Added check for Smart Disk scan in progress on this rcb,
**		calling SDfree() if so to free up the Smart Disk hardware.
**	    02-may-91 (kwatts)
**		Smart Disk changes: pass pseudo page to SDfree.
**	14-may-1993 (kwatts)
**	    Cleared a possible memory leak in dm2r_return_rcb. Always
**	    release the store for a Smart Disk page if there is one.
**      29-may-97 (stial01)
**          dm2r_return_rcb() always unfix pages in rcb
**	11-Aug-1998 (jenjo02)
**	    Roll rcb_tup_adds, rcb_page_adds into TCB.
**      05-may-1999 (nanpr01)
**          dm2r_return_rcb() Set the rcb to null. 
**	26-Feb-2010 (jonj)
**	    If cursor CRIB, decrement inuse count; if zero,
**	    delink and deallocate.
*/
DB_STATUS
dm2r_return_rcb(
    DMP_RCB     **rcb,
    DB_ERROR	*dberr )
{
    DMP_RCB	    *r = *rcb;
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS       local_status;
    i4         local_error;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9310_DM2R_RELEASE_RCB);
#endif

    if (r->rcb_data.page)
    {
	local_status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data,
	    &local_dberr);
	if (local_status != E_DB_OK)
	{
	    status = local_status;
	    *dberr = local_dberr;
	}
    }

    if (r->rcb_other.page)
    {
	local_status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
            &local_dberr);
	if (local_status != E_DB_OK)
	{
	    status = local_status;
	    *dberr = local_dberr;
	}
    }

    if (status != E_DB_OK)
	return (status);

    /* 
    ** If updated session temp table, update
    ** savepoint id in table's XCCB.
    **
    ** This is used by dmx_abort() to decide which 
    ** session temporary tables must be destroyed
    ** during transaction abort or rollback <to savepoint>.
    */
    if ( t->tcb_rel.relstat2 & TCB_SESSION_TEMP &&
	 r->rcb_state & RCB_TABLE_UPDATED )
	t->tcb_temp_xccb->xccb_sp_id = r->rcb_xcb_ptr->xcb_sp_id;

    /*	Remove RCB from transaction RCB ring. */
    if (r->rcb_rl_next != r)
    {
	DMP_RCB		*next_rcb;

	/* Find RCB preceeding this RCB. */

	next_rcb = r->rcb_rl_next;
	while (next_rcb->rcb_rl_next != r)
	    next_rcb = next_rcb->rcb_rl_next;
	next_rcb->rcb_rl_next = r->rcb_rl_next;
	r->rcb_rl_next = r;
    }

    /* Decrement cursor CRIB inuse count */
    if ( r->rcb_crib_ptr && r->rcb_crib_ptr->crib_cursid )
    {
	LG_CRIB		*curscrib = r->rcb_crib_ptr;

	/* Decrement inuse count, if zero, deallocate CRIB */
        if ( --curscrib->crib_inuse == 0 )
	{
	    DM_OBJECT	*object;

	    /* Delink cursor CRIB from transaction's list */
	    curscrib->crib_next->crib_prev = curscrib->crib_prev;
	    curscrib->crib_prev->crib_next = curscrib->crib_next;

	    /* If BQCB is having etabs opened against this cursor CRIB,
	    ** it can't do that any more.  Use the transaction CRIB if an
	    ** etab open is needed.
	    */
	    if (r->rcb_bqcb_ptr != NULL
	      && r->rcb_bqcb_ptr->bqcb_crib == curscrib)
	    {
		r->rcb_bqcb_ptr->bqcb_crib = r->rcb_xcb_ptr->xcb_crib_ptr;
	    }

	    object = (DM_OBJECT*)((char*)curscrib - sizeof(DM_OBJECT));
	    dm0m_deallocate(&object);
	}
    }
    r->rcb_crib_ptr = (LG_CRIB*)NULL;
    r->rcb_bqcb_ptr = NULL;		/* Cleanliness */

    /* Update master counts, remove from master possibly-update list if
    ** we're on one.  Do this before allowing the RCB to appear on the
    ** TCB's free-list.
    */
    if (r->rcb_mupd_list.q_next != NULL)
    {
	DMP_TCB *master_tcb = t->tcb_pmast_tcb;

	dm0s_mlock(&master_tcb->tcb_mutex);
	master_tcb->tcb_tup_adds  += r->rcb_tup_adds;
	master_tcb->tcb_page_adds += r->rcb_page_adds;
	r->rcb_mupd_list.q_next->q_prev = r->rcb_mupd_list.q_prev;
	r->rcb_mupd_list.q_prev->q_next = r->rcb_mupd_list.q_next;
	dm0s_munlock(&master_tcb->tcb_mutex);
    }

    /*	Requeue RCB from allocated to free. */
    dm0s_mlock(&t->tcb_mutex);
    r->rcb_q_next->rcb_q_prev = r->rcb_q_prev;
    r->rcb_q_prev->rcb_q_next = r->rcb_q_next;
    r->rcb_q_next = t->tcb_fq_next;
    r->rcb_q_prev = (DMP_RCB*)&t->tcb_fq_next;
    t->tcb_fq_next->rcb_q_prev = r;
    t->tcb_fq_next = r;

    /* Roll and clear tuple and page deltas into TCB while mutex held */
    t->tcb_tup_adds  += r->rcb_tup_adds;
    t->tcb_page_adds += r->rcb_page_adds;
    r->rcb_tup_adds  = 0;
    r->rcb_page_adds = 0;

    /* Remove from TCB's possibly-updating list used for counts gathering */
    if (r->rcb_upd_list.q_next != NULL)
    {   
	r->rcb_upd_list.q_next->q_prev = r->rcb_upd_list.q_prev;
	r->rcb_upd_list.q_prev->q_next = r->rcb_upd_list.q_next;
    }

    r->rcb_state = 0;
    r->rcb_upd_list.q_next = NULL;
    r->rcb_upd_list.q_prev = NULL;
    r->rcb_mupd_list.q_next = NULL;
    r->rcb_mupd_list.q_prev = NULL;

    dm0s_munlock(&t->tcb_mutex);

    /* set the rcb to null */
    *rcb = 0;

    return (E_DB_OK);
}

/*
** {
** Name: dm2r_delete	- Delete a record.
**
** Description:
**      This function will delete the current record in a scan or will
**      delete a record by tuple identifier.  All secondary indices are
**      updated as result of this operation.  The RCB must have been opened
**      for write access.  The RCB must currently be positioned if a delete
**	current record is performed.
**      This routine handles tracking of replaces and
**	deletes when multiple RCB's exist on a table
**	within a transaction.
**
** Inputs:
**      rcb                             The RCB to perform the deletion with.
**      tid                             If flag == DM2R_BYTID, then this is the
**					tuple identifier of the record to be
**					deleted.
**	flag				DM2R_BYPOSITION or DM2R_BYTID.
**
** Outputs:
**      err_code                        Reason for error return status.
**					    E_DM003C_BAD_TID
**					    E_DM0044_DELETED_TID
**					    E_DM0042_DEADLOCK
**					    E_DM004D_LOCK_TIMER_EXPIRED
**					    E_DM0073_RECORD_ACCESS_CONFLICT
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL			Operation partially completed,
**                                      transaction
**					must be aborted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-dec-85 (derek)
**          Created new for jupiter.
**	15-Jun-88 (rogerk)
**	    Log logical delete of base and secondary index entries if
**	    using fast commit.
**	 3-may-89 (rogerk)
**	    Build secondary index key rather than tuple to search for
**	    secondary index row to delete.
**	    Don't return NONEXT when fail to update secondary index.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Call gateway for deletes on
**	    gateway tables. Use rcb_logging/tcb_logging fields to determine
**	    appropriate logging actions.  On BYTID deletes, make sure tid
**	    access is allowed.
**       4-sep-89 (rogerk)
**          Put in trace flag to bypass SBI logging.
**	 5-dec-1989 (rogerk)
**	    Put back in the BI logging of BTREE leaf pages.  These are needed
**	    for UNDO recovery by the online-backup code during processing
**	    of the DUMP file information.
**	11-jan-1989 (rogerk)
**	    Only do BI logging of BTREE leaf pages if we are doing online
**	    backup.  The rcb_bt_bi_logging is enabled if online backup is
**	    going on, and the dm0l_bi() routine is only called if this flag
**	    is enabled.
**	25-jan-1990 (fred)
**	    Added code to delete extension objects.  This is for peripheral
**	    (== large) object support.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    17-dec-1990 (jas)
**		Smart Disk project integration: If we're turning off the
**		RCB_POSITIONED bit, force RCB_SDPOSITIONED off, as well.
**	    4-feb-1991 (rogerk)
**		Add support for Set Nologging.  Check rcb_logging status
**		before updating system catalogs and secondary indexes.  Use
**		rcb_logging rather than tcb_logging to direct logging actions
**		on 2nd indexes.
**	14-apr-1992 (rogerk)
**	    Pass deltid value to ule_format using deltid.tid_i4 instead of
**	    passing the structure itself.
**	26-may-92 (schang)
**	    GW merge.
**	    19-jun-1991 (rickh)
**		Plugged up some Gateway holes.  Gateways should never execute
**		code specific to INGRES tables types.  Look for table type in
**		t->tcb_table_type, not t->tcb_rel.relspec.
**	09-Nov-1992 (fred)
**	    Moved code dealing with extension table maintenance.  It had
**	    migrated to after the secondary index code (or had always been
**	    there) and this routine was returning before dealing with
**	    extensions if the base table had no secondary indicies.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Took out calls to log put, delete and
**	    replace operations.  These are now logged from inside the page
**	    updating routines in DM1R.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix core catalog data page as soon as base table DELETE is
**		    complete. This improves concurrency, but more importantly
**		    it satisfies the only-fix-one-core-catalog protocol for
**		    node failure recovery.
**      15-Apr-1994 (fred)
**          Add handling (i.e. ignoring) of interrupts when processing
**          dmpe calls.
**	19-Apr-1995 (cohmi01)
**	    if 'checkeof' is on for a secondary index, call dm1p_checkhwm()
**	    to adjust eof so that group buffers may be used.
**	09-May-1995 (cohmi01)
**	    Cancel any pending prefetch event for this RCB, its too late now.
**	    Remove ifdef from increment of rcb_s_del, its needed for prefetch.
**	20-may-96 (stephenb)
**	    Perform replication data capture for deletes and open input
**	    queue if necesary.
**	13-Aug-1996 (prida01)
**	    Give relatts correct i4 casting.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Row locking: Value lock for unique key/row
**          Moved common code to dm2r_fetch_record()
**          Moved common code to si_delete()
**      21-may-97 (stial01)
**          dm2r_delete() dm2r_return_rcb will unfix pages, remove extra unfix. 
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**	3-jun-97 (stephenb)
**	    if the user is updating the replicator input queue and 
**	    replication is active, then set the input queue value in the
**	    transaction control block so that the input queue values will
**	    be distributed on commit.
**     29-jul-97 (stial01)
**          dm2r_delete() Pass opflag DM1C_DELETE to dm1*_delete, si_delete
**	25-sep-97 (stephenb)
**	    move replication code so that it is done before we delete the
**	    the peripheral datatypes. Peripheral deletes delete the extra 
**	    records in the peripheral table, this means that we
**	    can't realize the coupon to get to the old values for replication 
**	    archives.
**	22-oct-98 (stephenb)
**	    Make sure logging and journalling characteristics of the replication
**	    input queue RCB match those of the base table.
**      05-may-1999 (nanpr01)
**          dm2r_delete() Lock/unlock key value to prevent delete of: 
**          - Unique key that a serializable trans tried to insert & failed.
**	17-dec-99 (gupsh01)
**	    Added additional check to give out correct error message in case of 
**	    updating a readonly database. 
**      1-Jul-2000 (wanfr01)
**          Bug 101521, INGSRV 1226 - If using RAAT, and II_RAAT_RETRY is sent,
**          reattempt to acquire the lock.  If there is a timeout, return the
**	    new error code E_DM0173_RAAT_SI_TIMER.   This reduces the chance
**	    of corrupting the secondary index.
**	09-Feb-2001 (jenjo02)
**	    dm2r_get() may not have placed a copy of the record to
**	    be deleted in rcb_record_ptr. If needed, fetch it.
**      07-apr-2005 (huazh01)
**          if table is to be replicated to another database, then use 
**          RCB of dd_input_queue to decide whether or not journaling 
**          is needed. 
**          This fixes b114215, INGSRV3229. 
**	22-Jan-2004 (jenjo02)
**	    Added support for parallel SI update threads.
**	26-Jan-2004 (jenjo02)
**	    Get SI list from master's TCB.
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
**	18-Aug-2008 (kibro01) b120490
**	    Do not allow replication on a non-journalled table within a
**	    journalled database.
*/
DB_STATUS
dm2r_delete(
    DMP_RCB     *rcb,
    DM_TID      *tid,
    i4     	flag,
    DB_ERROR	*dberr )
{
    DMP_RCB             *r = rcb;
    DMR_RTB		*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_RCB		*ir;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_TCB		*it;
    DB_STATUS		status;
    i4			local_err_code;
    i4			rcb_si_flags;
    DB_TAB_TIMESTAMP	local_timestamp;
    DMP_DCB		*dcb = rcb->rcb_tcb_ptr->tcb_dcb_ptr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9311_DM2R_DELETE);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9311_DM2R_DELETE);
#endif

    /* Base table delete: */
    status = E_DB_OK;

    for (;;) /* something to break from */
    {
	/* TID for secondary indexes not yet known */
	r->rcb_si_oldtid.tid_i4 = NOT_ALLOCATED;

	rcb_si_flags = 0;

	dmf_svcb->svcb_stat.rec_delete++;
	r->rcb_s_del++;

	/*	Check that the RCB is for write access. */
	if (r->rcb_access_mode != RCB_A_WRITE)
	{
	    if (r->rcb_xcb_ptr->xcb_x_type & XCB_RONLY)
		SETDBERR(dberr, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    else
		SETDBERR(dberr, 0, E_DM0073_RECORD_ACCESS_CONFLICT);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** We need a copy of the record being deleted if:
	**
	**  o table has indexes which must be updated
	**  o (c)row locking and duplicate checking must be done
	**  o table has extensions
	**  o table is being replicated
	**  o security label must be checked
	**
	** i.e., any of the code below that dereferences
	** rcb_record_ptr.
	*/
	if ( ((r->rcb_state & RCB_RECORD_PTR_VALID) == 0 &&
	      (r->rcb_state & RCB_FETCH_REQ) == 0) &&
	    ( t->tcb_update_idx ||
	      ((row_locking(r) || crow_locking(r)) && 
		  (t->tcb_rel.relstat & TCB_UNIQUE || no_dup_rows(t))) ||
	       t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS ||
	       t->tcb_replicate == TCB_REPLICATE ||
	      (r->rcb_internal_req != RCB_INTERNAL && 
		  dmf_svcb->svcb_status & SVCB_IS_SECURE) )
	   )
	{
	    /* Don't have it and must get it */
	    r->rcb_state |= RCB_FETCH_REQ;
	}
	
	status = dm2r_fetch_record(r, tid, flag, DM1C_MDELETE, dberr);
	if ( status )
	    break;

	if ( r->rcb_internal_req != RCB_INTERNAL &&
	     dmf_svcb->svcb_status & SVCB_IS_SECURE )
	{
	    /*
	    ** Always check the label of record for a delete.
	    */

	    status = dma_row_access(DMA_ACC_DELETE, r, r->rcb_record_ptr, NULL,
				    &r->rcb_compare, dberr);

	    if ( status == E_DB_OK )
	    {
		if (r->rcb_compare != DMA_ACC_PRIV)
		{
		    SETDBERR(dberr, 0, E_DM201E_SECURITY_DML_NOPRIV);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

	/*
	** For duplicate checking, get key value locks
	*/
	if ( (row_locking(r) || crow_locking(r)) &&
	    (t->tcb_rel.relstat & TCB_UNIQUE || no_dup_rows(t)) &&
	    (status = dm1r_lock_value(r, (i4)DM1R_LK_PHYSICAL, r->rcb_record_ptr, 
				     dberr)) )
	{
	    break;
	}

	/*
	** Check if our page(s) were unfixed
	*/
	if ( r->rcb_state & RCB_FETCH_REQ )
	    status = dm2r_fetch_record(r, tid, flag, DM1C_MDELETE, dberr);

	break;
    }

    /* If SI threading, need the RTB's semaphore */
    if ( r->rcb_siAgents )
	CSp_semaphore(1, &rtb->rtb_cond_sem);

    /* Add the resulting flags to the Base RCB */
    r->rcb_si_flags |= rcb_si_flags;
    /* Un-busy the Base RCB, SI threads can proceed */
    r->rcb_si_flags &= ~RCB_SI_BUSY;

    /* If good status, set "oldtid" for SI deletes */
    if ( status == E_DB_OK )
	r->rcb_si_oldtid.tid_i4 = r->rcb_currenttid.tid_i4;

    /* Notify all SI waiters of un-BUSY Base RCB */
    if ( r->rcb_siAgents )
    {
	rtb->rtb_dm2r_flags = flag;
	rtb->rtb_status = status;
	rtb->rtb_error = *dberr;
	if ( rtb->rtb_cond_waiters )
	    CScnd_broadcast(&rtb->rtb_cond);
	CSv_semaphore(&rtb->rtb_cond_sem);
    }

    /* If failure thus far, return */
    if ( status )
	return(SIcomplete(r, status, dberr));

    /*
    ** Perform the deletion by access method.
    */

    switch (t->tcb_table_type)
    {
    case TCB_HEAP:
    case TCB_ISAM:
    case TCB_HASH:
	status = dm1r_delete(r, &r->rcb_currenttid, DM1C_BYPOSITION, 
				(i4)DM1C_MDELETE, dberr);
	break;

    case TCB_BTREE:
	status = dm1b_delete(r, &r->rcb_fetchtid, &r->rcb_currenttid,
				(i4)DM1C_MDELETE, dberr);
	break;

    case TCB_RTREE:
	status = dm1m_delete(r, &r->rcb_fetchtid, &r->rcb_currenttid, dberr);
	break;

    case TCB_TAB_GATEWAY:
	/*
	** Call gateway to delete record.
	*/
	status = dmf_gwr_delete(r, tid, dberr);
	break;
    }

    if (status == E_DB_OK && (row_locking(r) || crow_locking(r)) && 
        (t->tcb_rel.relstat & TCB_UNIQUE || no_dup_rows(t)))
    {
	status = dm1r_unlock_value(r, r->rcb_record_ptr, dberr);
    }

    /* rcb_record_ptr is no longer trustworthy */
    r->rcb_state &= ~RCB_RECORD_PTR_VALID;

    /*
    ** perform replication prcessing, we need to do this before we delete
    ** the peripheral records since we will need them to construct the 
    ** archive record
    */
    if ( status == E_DB_OK && dcb->dcb_status & DCB_S_REPLICATE &&
	t->tcb_replicate == TCB_REPLICATE )
    {
        /*
        ** check if this session has the input queue open, if not open it
        */
        if (r->rcb_xcb_ptr->xcb_rep_input_q == NULL || 
	    r->rcb_xcb_ptr->xcb_rep_input_q == (DMP_RCB *)-1)
        {
            i4     rep_iq_lock = 0;
	    i4	rep_iq_maxlocks = 0;

	    /*
	    ** check for locking startegy, "set lockmode" for the input
	    ** queue over-rides all other settings, otherwhise we use PM
	    ** variables. If they are not set the initial user base-table
	    ** update determins the locking strategy
	    */
	    if (rcb->rcb_xcb_ptr->xcb_scb_ptr && 
		rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next)
	    {
		DML_SLCB    *slcb = rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next;
		for (;;)
    		{
        	    if (slcb == (DML_SLCB*) 
			&rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next || 
			slcb == 0)
            		break;
        	    if ( slcb->slcb_db_tab_base == 
			    dcb->rep_input_q.db_tab_base )
        	    {
            		if (slcb->slcb_lock_level == DMC_C_ROW)
                	    rep_iq_lock = DM2T_RIX;
            		else if (slcb->slcb_lock_level == DMC_C_PAGE)
                	    rep_iq_lock = DM2T_IX;
            		else if (slcb->slcb_lock_level == DMC_C_TABLE)
                	    rep_iq_lock = DM2T_X;
			if (slcb->slcb_maxlocks)
			    rep_iq_maxlocks = slcb->slcb_maxlocks;
			break;
		    }
		    slcb = slcb->slcb_q_next;
		}
	    }
	    if (rep_iq_lock == 0) 
	    {
		/* 
		** not explicitly set to row, page or table with 
		** "set lockmode" check PM
		*/
		if (dmf_svcb->svcb_rep_iqlock == DMC_C_ROW)
		    rep_iq_lock = DM2T_RIX;
		else if (dmf_svcb->svcb_rep_iqlock == DMC_C_PAGE)
		    rep_iq_lock = DM2T_IX;
		else if (dmf_svcb->svcb_rep_iqlock == DMC_C_TABLE)
		    rep_iq_lock = DM2T_X;
		else if (r->rcb_k_type == RCB_K_ROW)
		    /* no values set, use base table values */
		    rep_iq_lock = DM2T_RIX;
		else
		    rep_iq_lock = DM2T_IX;
	    }
	    if (rep_iq_maxlocks == 0)
		/* not set in "set lockmode" use base table value */
		rep_iq_maxlocks = r->rcb_lk_limit;

            status = dm2t_open(dcb, &dcb->rep_input_q, rep_iq_lock,
                DM2T_UDIRECT, DM2T_A_WRITE, r->rcb_timeout,
		rep_iq_maxlocks, r->rcb_sp_id, r->rcb_log_id, r->rcb_lk_id,
		r->rcb_seq_number, (i4)0, r->rcb_lk_mode,
                &r->rcb_tran_id, &local_timestamp,
                &r->rcb_xcb_ptr->xcb_rep_input_q, (DML_SCB *)0, dberr);

            if ( status == E_DB_OK )
	    {
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_xcb_ptr = r->rcb_xcb_ptr;
		/* set logging and journalling from base RCB */
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_logging = r->rcb_logging;
	    	r->rcb_xcb_ptr->xcb_rep_input_q->rcb_journal = 
                            is_journal(r->rcb_xcb_ptr->xcb_rep_input_q);

		/* Do not allow non-journalled tables to be replicated within
		** journalled databases since the internal tables are 
		** replicated.  (kibro01) b120490
		*/
		if (!r->rcb_journal && (dcb->dcb_status & DCB_S_JOURNAL))
		{
		    i4 local_err_code;
		    uleFormat(NULL, E_DM9582_REP_NON_JNL, (CL_ERR_DESC *)NULL,
			   ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			   &local_err_code, 1,
			   DB_TAB_MAXNAME, t->tcb_rel.relid.db_tab_name);
		    SETDBERR(dberr, 0, E_DM008D_ERROR_DELETING_RECORD);
		    status = E_DB_ERROR;
		}
	    }
        }
	if ( status == E_DB_OK &&
	   (DMZ_SES_MACRO(32) == 0 || dmd_reptrace() == FALSE) )
	{
	    /* only if we are not the replication server */
	    status = dm2rep_capture(DM2REP_DELETE, r, (char *)NULL, dberr);
	}
    }
    else if ( status == E_DB_OK && dcb->dcb_status & DCB_S_REPLICATE && 
		r->rcb_xcb_ptr &&
		r->rcb_xcb_ptr->xcb_rep_input_q == 0 &&
		(MEcmp(t->tcb_rel.relid.db_tab_name,
		(*r->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) ? 
		DD_INPUT_QUEUE_U : DD_INPUT_QUEUE, 
		DD_INPUT_QUEUE_SIZE) == 0) &&
		(STskipblank(t->tcb_rel.relid.db_tab_name + DD_INPUT_QUEUE_SIZE, 
		DB_MAXNAME - DD_INPUT_QUEUE_SIZE)) == NULL)
    {
	/*
	** user is updating the input queue, set up input_q value
	** in the XCB so that the distribution threads are activated to
	** distribute the values. We use the value -1 so that we know it
	** was a user update and not an internal DBMS update
	*/
	r->rcb_xcb_ptr->xcb_rep_input_q = (DMP_RCB *)-1;
    }

    /*
    **  If there are extensions, then we need to delete any extension tuples.
    */
    if ((status == E_DB_OK) && (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
		&& (flag & DM2R_REDO_SF) == 0)
    {
	status = dm1c_pdelete(t->tcb_atts_ptr,
				r,
				r->rcb_record_ptr,
				dberr);
	if (status)
	{
	    if (dberr->err_code != E_DM0065_USER_INTR
	      && dberr->err_code != E_DM010C_TRAN_ABORTED
              && dberr->err_code != E_DM016B_LOCK_INTR_FA)
	    {
		/*
		** User interrupts are now a possibility, since dmpe
		** updates may make recursive DMF calls to mung with
		** objects in the extension tables.  Given that that's
		** the case, we just pass the error back up the chain,
		** and the upper layers will handle it correctly
		*/
		
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL,
			   ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			   &local_err_code, 0);
		SETDBERR(dberr, 0, E_DM9B00_DELETE_DMPE_ERROR);
	    }
	}
    }

    /*
    ** If this is a core system catalog (must be iirelation, since it's the
    ** only one which has a secondary index), then unfix its pages here. This
    ** ensures that we comply with the cluster node failure recovery rule that
    ** a transaction may only fix one core system catalog page at a time.
    */
    if ( status == E_DB_OK && t->tcb_rel.relstat & TCB_CONCUR &&
	 t->tcb_update_idx )
    {
	status = dm2r_unfix_pages(r, dberr);
    }

    /*
    ** Check for secondary index updates.
    ** For secondary index updates you don't have to worry about
    ** multiple curors within a transaction changing the lowtid
    ** value thus needing the fetchtid value for updates,
    ** since this is all done before returing.  Therefore
    ** other cursors can't be doing anything between the get and
    ** update on the index.
    **
    ** For Gateway tables, secondary indices aren't updated
    ** (tcb_update_idx is FALSE).
    */

    if ( t->tcb_update_idx && !r->rcb_siAgents )
    {
	i4 cancel_ix;	/* ordinal position of index in progress */

	/* Get indexes from master TCB */

	for (it = t->tcb_pmast_tcb->tcb_iq_next, cancel_ix = 1;
	    status == E_DB_OK && 
		it != (DMP_TCB*) &t->tcb_pmast_tcb->tcb_iq_next;
	    it = it->tcb_q_next, cancel_ix++)
	{
#ifdef NEVER_USED
    	    /* Cancel outstanding prefetch readahead requests for this rcb  */
	    /* that refer to this index in case RA thread is falling behind.*/
	    if (r->rcb_state & RCB_PREFET_PENDING)
	    	dm2r_cancel_prefetch(r, cancel_ix);
#endif

	    status = si_delete(it, r, flag | DM2R_MDELETE, dberr);
	}
    }

    return(SIcomplete(r, status, dberr));

} /* dm2r_delete() */

/*{
** Name: dm2r_si_delete	- Delete a record's secondary index entry (threaded).
**
** Description:
**	
**      This function deletes an entry from an index.
**      One secondary index is updated.
**	It is called from dmrAgent by a SI agent.
**
** Inputs:
**      ir                              The index's RCB.
**
** Outputs:
**      err_code                        The reason for error status.
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
**	11-Nov-2003 (jenjo02)
**	    Created for Parallel/Partitioning project.
*/
DB_STATUS
dm2r_si_delete(
DMP_RCB		*ir,
DB_ERROR	*dberr)
{
    DMP_RCB	*r = ir->rcb_base_rcb;
    DMR_RTB	*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB	*it = ir->rcb_tcb_ptr;
    DMP_DCB	*d = it->tcb_dcb_ptr;
    DB_STATUS	status = E_DB_OK;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    CSp_semaphore(1, &rtb->rtb_cond_sem);

    /* 
    ** Must wait for the Base RCB to be initialized 
    ** and for the Base row's (old) TID to materialize.
    */
    while ( rtb->rtb_status == E_DB_OK &&
	   (r->rcb_si_flags & RCB_SI_BUSY ||
	    r->rcb_si_oldtid.tid_i4 == NOT_ALLOCATED) )
    {
	rtb->rtb_cond_waiters++;
	CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	rtb->rtb_cond_waiters--;
    }

    /* Now do the delete */
    if ( rtb->rtb_status == E_DB_OK )
    {
	CSv_semaphore(&rtb->rtb_cond_sem);
	status = si_delete(it, ir,
			   rtb->rtb_dm2r_flags | 
			      (DM2R_SI_AGENT | DM2R_MDELETE),
			   dberr);
    }
    else
	CSv_semaphore(&rtb->rtb_cond_sem);

    /* Unfix any SI pages we may have left fixed */
    if ( dm2r_unfix_pages(ir, &local_dberr) && status == E_DB_OK )
    {
	status = E_DB_ERROR;
	*dberr = local_dberr;
    }

    /* Return status */
    return(status);
}

/*{
** Name: dm2r_get	- Get a record.
**
** Description:
**      This function returns the next record that matches the optional
**	key criteria and the optional caller qualifcation function.  It
**	can also return the record at a particular TID.  The get next
**	operation only works if the record stream is currently positioned.
**
** Inputs:
**      rcb                             The RCB to get a record from.
**	tid				If opflag == DM2R_BYTID
**					the tid of the record to be retrieved.
**      opflag                          DM2R_GETNEXT, DM2R_GETPREV
**					or DM2R_BYTID.
**
** Outputs:
**	record				Pointer to location to return record.
**	tid				The tid of the record retrieved.
**      err_code                        The reason for error status.
**					    E_DM003C_BAD_TID
**					    E_DM0044_DELETED_TUPLE
**					    E_DM0042_DEADLOCK
**					    E_DM0074_NOT_POSITIONED
**					    E_DM0073_RECORD_ACCESS_CONFLICT
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
**	17-dec-1985 (derek)
**          Created new for jupiter.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  If table is a gateway table,
**	    then call to the gateway to get the next row.
**	25-jan-1990 (fred)
**	    Added code to fill rcb address into coupon's on their return.  This
**	    is for peripheral (== large) object support.
**	14-jun-90 (linda, bryanp)
**	    Added check for RCB_A_OPEN_NOACCESS; gets are not permitted in
**	    this mode.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    17-dec-90 (jas)
**		Smart Disk project integration:
**		If we're doing a Smart Disk scan, do a call dm1sd_get in lieu
**		of the storage-structure based get routine.
**       1-Jul-1993 (fred)
**          Removed aforementioned assignment for rcb into coupon.
**          This is now handled by the various dm1*_get() calls.
**      04-apr-1995 (dilma04)
**          Support repeatable read: set RCB_PAGE_HAD_RECORDS and
**          RCB_TABLE_HAD_RECORDS if a record is successfully retrieved.
**	24-Aug-1994 (cohmi01)
**	    Added backwards support - setting DM1C_GETPREV.
**	09-May-1995 (cohmi01)
**	    Added support for asynchronous readahead (prefetch) of related
**	    secondary indexes. Cancel any pending prefetch event for this RCB.
**	    Remove ifdef from increment of rcb_s_get, its needed now for prefetch
**	    calc. If certain criteria are met, schedule a prefetch event.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - set RCB_*_HAD_RECORDS if a record is successfully retrieved;
**          - if isolation level is RR, convert page locks related to the
**          returned record to logical locks;  
**          - if E_DM0055_NONEXT is returned, release unneeded locks.
**      13-jun-97 (dilma04)
**          Repeatable Read optimizations: 
**          - do not convert leaf page's lock to logical, because it either 
**          will be released, or converted to logical, if the page is updated; 
**          - release leaf page's lock if page has not been updated.
**      12-nov-97 (stial01)
**          dm2r_get() If cursor stability, unlock previous row before
**          we unlock/unfix the data page it is on.
**          Pass lockKEY even if unlocking with lockID (for set lock_trace)
**	02-jun-98 (kitch01)
**	    Bug 90627. Change loc_err from being a pointer to prevent GPF.
**	21-feb-1999 (nanpr01)
**	    Convert the update mode physical to logical lock.
**      05-may-1999 (nanpr01)
**          dm2r_get() If row locking, serializable and equal predicate, 
**          and unique key, release value lock before returning the record.
**	09-Feb-2001 (jenjo02)
**	    Don't copy record to rcb_record_ptr on EVERY get, be
**	    more selective.
**	24-Oct-2006 (kschendel)
**	    Fix locking problem with core catalogs when repeatable read:
**	    we try to convert a physical lock to logical, but the RCB says
**	    the table does physical locking.  So the convert ends up being
**	    a second physical request instead, thus leaving a bogus lock
**	    outstanding.  Don't convert if it's not going to happen.
**	    LATER NOTE: probably shouldn't get into that condition at all,
**	    dmtopen fixed to not ask for RR with sconcur catalogs;  but
**	    the fix here is not wrong so keep it.
**	04-Apr-2008 (jonj)
**	    Don't use update mode locking if not supported by LK.
**	11-Feb-2010 (jonj)
**	    SIR 121619 MVCC: When NeedCrowLock, get the lock nowait and
**	    if a wait is needed, wait, then reget/requalify the row.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Relocate NeedCrowLock logic back into dm1r/dm1b.
*/
DB_STATUS
dm2r_get(
    DMP_RCB     *r,
    DM_TID	*tid,
    i4     opflag,
    char	*record,
    DB_ERROR	*dberr )
{
    DMP_TCB             *t = r->rcb_tcb_ptr;
    i4		flag;
    i4             loc_err;
    DB_STATUS           status;
    DB_STATUS           loc_stat;
    DM_TID		fetchtid;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9312_DM2R_GET);
#endif

    if (r->rcb_access_mode == RCB_A_OPEN_NOACCESS)
    {
	SETDBERR(dberr, 0, E_DM0073_RECORD_ACCESS_CONFLICT);
	return (E_DB_ERROR);
    }

#ifdef NEVER_USED
    /* cancel all outstanding index related prefetch readahead requests */
    if (r->rcb_state & RCB_PREFET_PENDING)
	dm2r_cancel_prefetch(r, PFR_CANCEL_IX);
#endif

    /*
    ** If table opened for write, remember the current TID, and
    ** use update mode locking except if catalog or etab, but only
    ** if update locks supported.
    */
    if (r->rcb_access_mode == RCB_A_WRITE)
    {
	fetchtid.tid_i4 = r->rcb_fetchtid.tid_i4;
	if ((t->tcb_rel.relstat & TCB_CATALOG) == 0 &&
	  !t->tcb_extended &&
	  dmf_svcb->svcb_status & SVCB_ULOCKS )
	{
	    r->rcb_state |= RCB_UPD_LOCK;
	}
    }

    dmf_svcb->svcb_stat.rec_get++;
    
    if (opflag & DM2R_BYTID)
	flag = DM1C_BYTID;
    else
    {
	if ((r->rcb_state & RCB_POSITIONED) == 0)
	{
	    if (r->rcb_state & RCB_WAS_POSITIONED)
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    else
		SETDBERR(dberr, 0, E_DM0074_NOT_POSITIONED);
	    return (E_DB_ERROR);
	}
	flag = DM1C_GETNEXT;
	if (opflag & DM2R_GETPREV)
	{
	    flag = DM1C_GETPREV;
	    if (!(r->rcb_p_flag & RCB_P_LAST) && 
		!(r->rcb_p_flag & RCB_P_ENDQUAL))
	    {
		SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
	        return (E_DB_ERROR);
	    }
	}
    }

    switch (t->tcb_table_type)
    {
	case TCB_HEAP:
	case TCB_HASH:
	case TCB_ISAM:
	    status = dm1r_get(r, tid, record, flag, dberr);
	    break;

	case TCB_BTREE:
	    /* DM1C_PKEY_PROJECTION only if read cursor (b105210) */
	    if ( (opflag & DM2R_PKEY_PROJ) &&
		r->rcb_access_mode == RCB_A_READ)
		flag |= DM1C_PKEY_PROJECTION;
	    if (opflag & DM2R_RAAT)
		flag |= DM1C_RAAT;

	    status = dm1b_get(r, tid, record, flag, dberr);

	    break;

	case TCB_RTREE:
	    status = dm1m_get(r, tid, record, flag, dberr);
	    break;

	case TCB_TAB_GATEWAY:
	    status = dmf_gwr_get(r, tid, record, flag, dberr);
	    break;
    }

    if (status == E_DB_OK)
    {
	r->rcb_s_get++;

	/*
	**  Save record in RCB for possible update
	**  if WRITE access and updates have occurred
	**  on this RCB or if the table is being
	**  replicated, otherwise assume the
	**  record won't be updated and avoid the
	**  copy. If that assumption turns out to be 
	**  wrong, dm2r_delete(), dm2r_update() will 
	**  re-fetch the record.
	**  If DM1C_PKEY_PROJECTION, the record is invalid because
	**  it is the projection of key columns only. Currently we 
	**  do this optimization for read cursors only. (b105210)
	*/
	if ( r->rcb_access_mode == RCB_A_WRITE )
	{
	    /* Don't change record_ptr state if refetch */
	    if ( fetchtid.tid_i4 != r->rcb_fetchtid.tid_i4 
		|| t->tcb_table_type == TCB_TAB_GATEWAY )
	    {
		if ( t->tcb_table_type == TCB_TAB_GATEWAY
		     || r->rcb_s_rep
		     || (r->rcb_s_del &&
			    (t->tcb_update_idx ||
			      ((row_locking(r) || crow_locking(r)) && 
			      (t->tcb_rel.relstat & TCB_UNIQUE || no_dup_rows(t))) ||
			       t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS ||
			      (r->rcb_internal_req != RCB_INTERNAL && 
				  dmf_svcb->svcb_status & SVCB_IS_SECURE)
			    )
			 )
		     || t->tcb_replicate == TCB_REPLICATE
		     || r->rcb_state & RCB_PREFET_AVAILABLE )
		{
		    MEcopy(record, t->tcb_rel.relwid,
			r->rcb_record_ptr);
		    r->rcb_state |= RCB_RECORD_PTR_VALID;
		}
		else
		    r->rcb_state &= ~RCB_RECORD_PTR_VALID;
	    }

#ifdef NEVER_USED
	    /* Signal a readahead request for this records sec index entries */
	    /* if readahead is available for rcb, and if it is likely that   */
	    /* they will be needed, for example, if we have already done a   */
	    /* delete or update with this rcb.                               */
	    if ((r->rcb_state & RCB_PREFET_AVAILABLE) &&
		t->tcb_update_idx &&
		((r->rcb_s_del && (r->rcb_s_del >= (r->rcb_s_get/4))) ||
		 (r->rcb_s_rep && (r->rcb_s_rep >= (r->rcb_s_get/4)) )))
		    dm2r_schedule_prefetch(r, PFR_PREFET_SECIDX);
#endif
	}

        r->rcb_state |= (RCB_DPAGE_HAD_RECORDS|RCB_TABLE_HAD_RECORDS);
        if (t->tcb_table_type == TCB_BTREE)
             r->rcb_state |= RCB_LPAGE_HAD_RECORDS;
        
	/* 
	** If serializable with equal predicate we acquired a value lock
	** during the position for phantom protection.
	** For unique keys, the row lock can provide phantom protection,
	** so release the value lock.
	** If this is get after reposition, we may have already released
	** the value lock so only try to unlock if rcb_val_lkid is init.
	*/
	if (row_locking(r)
	    && (r->rcb_val_lkid.lk_common || r->rcb_val_lkid.lk_unique)
	    && t->tcb_rel.relkeys && serializable(r)
	    && (opflag & DM2R_BYTID) == 0
	    && (t->tcb_rel.relstat & TCB_UNIQUE)
	    && r->rcb_p_hk_type == RCB_P_EQ 
	    && r->rcb_hl_given == t->tcb_keys)
	{
	    status = dm1r_unlock_value(r, record, dberr);
	    if (status != E_DB_OK)
		TRdisplay("dm1r_get() unlock value failed %d\n", status);
	}

        /*
        ** If isolation level is RR, convert physical lock on data page 
        ** from where record has been returned to logical lock, because
        ** it will stay until the end of transaction. 
        */
        if (r->rcb_iso_level == RCB_REPEATABLE_READ && 
            r->rcb_lk_type != RCB_K_TABLE &&
	    (r->rcb_k_duration & RCB_K_PHYSICAL) == 0 &&
	    r->rcb_state & RCB_CSRR_LOCK)
        {
             i4 lock_action = 0;
             i4 page_number;
             i4 lock_mode;

             if (r->rcb_access_mode == RCB_A_READ)
                 lock_mode = row_locking(r) ? LK_IS : LK_S;
             else
                 lock_mode = row_locking(r) ? LK_IX : 
				((r->rcb_state & RCB_UPD_LOCK) ? LK_U : LK_X);
                 
             if (r->rcb_data.page != NULL)
             {
                 page_number = 
                     DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
                     r->rcb_data.page);
                 status =  dm0p_trans_page_lock(r, t, page_number,
                     DM0P_LK_CONVERT, lock_mode, &lock_action, (LK_LKID *)0, 
                     dberr);
		     
                 r->rcb_csrr_flags &= ~RCB_CS_DATA;
             }
        }

	return (E_DB_OK);
    }

    if (dberr->err_code == E_DM0055_NONEXT)
    {
	r->rcb_state &= ~(RCB_POSITIONED);

	/* Convert the last update locks to shared lock */
	if (((r->rcb_iso_level == RCB_SERIALIZABLE) ||
	     (r->rcb_iso_level == RCB_REPEATABLE_READ)) &&
	    (row_locking(r)) &&
	    ((i4) r->rcb_locked_tid.tid_i4 != -1) &&
	    ((r->rcb_state & RCB_UPD_LOCK)) &&
	    (r->rcb_lk_mode == LK_IX))
	{
	    loc_stat = dm1r_lock_downgrade(r, &local_dberr);
	    if (loc_stat != E_DB_OK)
	    {
		*dberr = local_dberr;
		return(loc_stat);
	    }		
	}

        /* 
        ** There is no more data that satisfies request, so there will be
        ** no more lock requests and, therefore, no more calls to release  
        ** unneeded locks at CS or RR isolation levels. Because of this,
        ** we should release those locks here. 
        */
        if ((r->rcb_state & RCB_CSRR_LOCK) && r->rcb_lk_type != RCB_K_TABLE) 
        {
	    DMP_DCB *d = t->tcb_dcb_ptr;

            if ((r->rcb_csrr_flags & RCB_CS_ROW) &&
                (r->rcb_state & RCB_ROW_UPDATED) == 0 &&
                r->rcb_iso_level == RCB_CURSOR_STABILITY)
            {
                if (r->rcb_row_lkid.lk_unique == 0)
                    dmd_check(E_DM9312_DM2R_GET); 
                loc_stat = dm1r_unlock_row(r, (LK_LOCK_KEY *)0, &local_dberr);
                if (loc_stat != E_DB_OK)
                {
		    *dberr = local_dberr;
                    return (loc_stat);
                }
                r->rcb_csrr_flags &= ~RCB_CS_ROW;
            }

            if ((r->rcb_csrr_flags & RCB_CS_DATA) && 
                (r->rcb_state & RCB_DPAGE_UPDATED) == 0 &&
                ((r->rcb_iso_level != RCB_REPEATABLE_READ) || 
                 (r->rcb_state & RCB_DPAGE_HAD_RECORDS) == 0))
            {
                if (r->rcb_data_lkid.lk_unique == 0)
                    dmd_check(E_DM9312_DM2R_GET); 
		loc_stat = dm0p_unlock_page(r->rcb_lk_id, d,
		    &t->tcb_rel.reltid,
		    r->rcb_locked_data,
		    LK_PAGE, &t->tcb_rel.relid,
		    &r->rcb_tran_id, &r->rcb_data_lkid, (LK_VALUE *)0, 
		    &local_dberr);
 
                if (loc_stat != E_DB_OK)
		{
		    *dberr = local_dberr;
                    return (loc_stat);
		}
		r->rcb_locked_data = 0;
                r->rcb_csrr_flags &= ~RCB_CS_DATA;
            }

            if ((r->rcb_csrr_flags & RCB_CS_LEAF) &&
                (r->rcb_state & RCB_LPAGE_UPDATED) == 0) 
            {
                if (r->rcb_leaf_lkid.lk_unique == 0)
                    dmd_check(E_DM9312_DM2R_GET); 
		loc_stat = dm0p_unlock_page(r->rcb_lk_id, d,
		    &t->tcb_rel.reltid,
		    r->rcb_locked_leaf,
		    LK_PAGE, &t->tcb_rel.relid,
		    &r->rcb_tran_id, &r->rcb_leaf_lkid, (LK_VALUE *)0, 
		    &local_dberr);
 
                if (loc_stat != E_DB_OK)
                {
		    *dberr = local_dberr;
                    return (loc_stat);
                }
		r->rcb_locked_leaf = 0;
                r->rcb_csrr_flags &= ~RCB_CS_LEAF;
            }
        }
    }
    return (status);
}

/*{
** Name: dm2r_position	- Position a record stream.
**
** Description:
**      This function is given a description of the range of keys that
**      are desired and an optional record qualification function.  This
**      information is used to when retrieving records with dm2r_get().
**	The key descriptions are sorted into the set of key specifying
**	greater than or equal and the set of keys that specify less than
**	or equal or exactly equal.  The record stream is positioned to
**	read from low to high key range or for an exact key.  Partial
**	exact matchs on structures that support ranges cause the range
**	of keys to be returned.
**
**	Position for a full scan or for a range scan will use readahead
**	operation in bringing the pages. If table is BTREE and position
**	for exact, will use readahead operation too.
**
** Inputs:
**      rcb                             The RCB of the table to position.
**	flag				Either DM2R_QUAL or DM2R_ALL for
**					position by qualification or scan
**					all records. Use DM2R_BY_TID for
**                                      temporary files only to set lowtid
**                                      value for scan.  The hightid will
**                                      always be end of file.
**	key_desc			Pointer to an array of DM2R_KEY_DESC
**					entries that describe the position
**					qualification.
**	key_count			The number of key_desc entries.
**	q_fcn				Pointer to qualification function.
**	q_arg				Pointer to qualification argument.
**	tid				Pointer to TID for position by tid.
**
** Outputs:
**      err_code                        Reason for error return status.
**					    E_DM0073_RECORD_ACCESS_CONFLICT
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
**	17-dec-1985 (derek)
**          Created new for jupiter.
**	04-Feb-1987 (ac)
**	    Added readahead support.
**	31-oct-1988 (rogerk)
**	    Fix bug where we set rcb_p_hightid incorrectly.  We were setting
**	    it to the contents of rcb_p_hightid rather than rcb_hightid.
**	11-apr-1989 (Derek & Sandyh)
**	    Added support for multi part key lookups, as in EQUAL/RANGE case.
**	    Previously, this caused a scan since the BOF was used as the low
**	    mark. Now the combination of keys is used to position low key &
**	    high key to a restricted point.
**	26-jun-1989 (sandyh)
**	    fixed bug 6616 dealing with multiple fixes of btree leaf pages.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Call gateway to position
**	    gateway tables.
**	14-jun-90 (linda, bryanp)
**	    Added check for RCB_A_OPEN_NOACCESS; gets are not permitted in
**	    this mode.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    17-dec-90 (jas)
**		Smart Disk project integration:
**		If a Smart Disk position is being requested, call
**		dm1sd_position to do the work.
**	05_dec_1992 (kwatts)
**	    Smart Disk enhancement, interface to dm1sd_position changed.
**	20-sep-1993 (kwatts)
**	    Bring forward a 6.4 bug fix:
**          Bug #49377.  Modify dm2r_position() to turn on RCB_READAHEAD for
**          equal partial-key btree searches.
**	2-dec-93 (robf)
**         Changed error E_DM0074_NOT_POSITION to E_DM008E_ERROR_POSITIONING
**	   if unable to position the table. Note this should not normally
**	   happen - higher level code should be passing us a "sensible"
**         position request.
**	     E_DM0074 is confusing here since it applies to a get/delete etc
**	    failing (it also gets reported to the user as E_US11FD Cursor not
**          currently positioned on a row, which is misleading as well)
**     19-apr-95 (stial01)
**         Added support for DM2R_LAST for selecting last tuple from a
**         btree table for MANMANX.
**	09-May-1995 (cohmi01)
**	    Cancel any pending prefetch event for this RCB, its too late now.
**	11-aug-1995 (shust01)
**	    remove setting of DM_TIDEOF after call to dm1b_search() in case
**	    DM2R_LAST. Not needed.
**	10-Aug-1995 (cohmi01)
**	    Request data page list prefetch when positioning in index/btree.
**	01-sep-1995 (cohmi01)
**	    Add support for DM2R_LAST on an ISAM table. Update epilogue of
**	    DM2R_LAST processing to save rcb_p_xxxtids, only mark as positioned
**	    if dm1x_xxx() call was successful.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Changes for REPOSITION when row locking
**	31-dec-1996 (nanpr01, stial01)
**	    Bug 79925 : We need to save the position for DM2R_LAST.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Use macro to check for serializable transaction.
**      21-may-97 (stial01)
**          dm2r_position() Init rcb_pos_clean_count, removed all calls to
**          dm1b_save_bt_position. The page lsn and clean count will be
**          saved in the rcb in dm1b_position while the page is still mutexed.
**      16-jun-97 (stial01)
**          dm2r_position() Initialize r->rcb_p_lk_type, r->rcb_p_hk_type
**      29-sep-97 (stial01)
**          dm2r_position() Dont init rcb_pos_clean_count = DM1B_CC_INVALID.
**          It gets initialized during dm1b_search while page is mutexed.
**          DM1B_CC_INVALID forces us to search on the first get even if
**          no one updated the leaf between the position and the first get.
**	23-jul-1998 (nanpr01)
**	    Making position code aware of scan direction. It basically
**	    sets up positioning for descending scan.
**	13-aug-1998 (nanpr01)
**	    Fix up the btree-position code for descending scan.
**	06-dec-1998 (nanpr01)
**	    Donot readahead when evaluating equal predicate on unique BTREE
**	    secondary index for 2k pages. Since it is part of the key
**	    the code mistakenly thought that this is a part key.
**	7-may-1999 (nanpr01)
**	    Donot initialize the fields that are never going to be accessed.
**      05-may-1999 (nanpr01)
**          dm2r_position() If row locking, serializable and equal predicate, 
**          get value locks during position
**	04-nov-1999 (nanpr01)
**	    Make the read-ahead decision as before but do not use the readahead
**	    if the dm2r_position was asked not to use read ahead on qualified
**	    position.
**	03-July-2001 (thaju02)
**	    Initialize lk_map/hk_map arrays in its entirety. (B105190)
**	09-aug-2001 (hayke02)
**	    We now call dm1b_search() with rcb_lowtid rather than rcb_hightid
**	    when we have a zero hk_count and we are doing a backwards read
**	    through a btree as part of the topsort removal for 'order by ...
**	    desc' queries (flag & DM2R_ENDQUAL). This ensures that we are
**	    correctly positioned at the end of the btree before we begin the
**	    backwards read. This change fixes bug 105447.
**      23-may-2002 (horda03) Bug 107439
**          The attributes array in the TCB is accessed 1..relatts,
**          not 0..(relatts - 1). This can lead to problems when a column is added
**          to a table and made the index (its the last attribute of the table).
**      04-Apr-2008 (jonj)
**          Don't use update mode locking if not supported by LK.
**	11-Apr-2008 (kschendel)
**	    Don't pass qual-function stuff in this call.  The qual-function
**	    RCB stuff has been expanded, and is now setup in the RCB directly.
**       9-Feb-2009 (hanal04) Bug 121544
**          For exact key match retrievals under B-tree and ISAM set
**          rcb_ll_op_type and rcb_ll_given so that the respective dm1X_get()
**          calls can correctly set DMA_ACC_BYKEY, thereby flagging
**          dma_row_access() to write an audit record for the keyed SELECT.
**	25-Aug-2010 (miket) SIR 122403
**	    Remove encryption debugging code and dev-only trace point.
*/
DB_STATUS
dm2r_position(
    DMP_RCB             *rcb,
    i4		flag,
    DM2R_KEY_DESC	*key_desc,
    i4		key_count,
    DM_TID	        *tid,
    DB_ERROR		*dberr )
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DB_ATTS		*a = t->tcb_atts_ptr;
    DM2R_KEY_DESC	*k = key_desc;
    i4		lk_map[(DB_MAX_COLS + 31) / 32];
    i4		hk_map[(DB_MAX_COLS + 31) / 32];
    i4		lk_count;
    i4		lk_type;
    i4		hk_count;
    i4		hk_type;
    i4		structure = t->tcb_table_type;
    i4		i;
    i4		j;
    DB_STATUS		status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9313_DM2R_POSITION);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9313_DM2R_POSITION);
#endif

    if (r->rcb_access_mode == RCB_A_OPEN_NOACCESS)
    {
	SETDBERR(dberr, 0, E_DM0073_RECORD_ACCESS_CONFLICT);
	return (E_DB_ERROR);
    }

#ifdef NEVER_USED
    /* cancel all outstanding prefetch readahead requests for this rcb */
    if (r->rcb_state & RCB_PREFET_PENDING)
	dm2r_cancel_prefetch(r, PFR_CANCEL_ALL);
#endif

    /*
    ** If table opened for write, use update mode locking except if 
    ** catalog or etab, but only if update locks supported.
    */
    if ( rcb->rcb_access_mode == RCB_A_WRITE && 
	(t->tcb_rel.relstat & TCB_CATALOG) == 0 &&
	!t->tcb_extended &&
	dmf_svcb->svcb_status & SVCB_ULOCKS )
    {
	rcb->rcb_state |= RCB_UPD_LOCK;
    }

    dmf_svcb->svcb_stat.rec_position++;

    /* If this is a load file, it cannot be
    ** positioned until it is finished loading.
    */
    if (((t->tcb_loadfile & TCB_LOAD ) == TCB_LOAD) &&
        ((r->rcb_state & RCB_LEND) != RCB_LEND))
    {
	SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
	return (E_DB_ERROR);
    }

    /*	Release any pages fixed in the RCB for a previous position. */
    if (r->rcb_data.page)
    {
	status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data,
            dberr);
	if (status)
	    return (status);
    }
    if (r->rcb_other.page)
    {
	status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
            dberr);
	if (status)
	    return (status);
    }

    /* Reset info used by dm2r_get() for record copy */
    r->rcb_state &= ~RCB_RECORD_PTR_VALID;
    if ( structure == TCB_TAB_GATEWAY )
    {
	/* Gateways don't use TIDs */
	r->rcb_fetchtid.tid_i4 =  0;
    }
    else
    {
	r->rcb_fetchtid.tid_tid.tid_page = 0;
	r->rcb_fetchtid.tid_tid.tid_line = DM_TIDEOF;
    }

    /* If this is a reposition request, then check
    ** to see if it was ever positioned. If so
    ** reposition, if not, return error.
    */
    if (flag & DM2R_REPOSITION)
    {
	if ((r->rcb_state & RCB_WAS_POSITIONED) == 0)
	{
	    SETDBERR(dberr, 0, E_DM0074_NOT_POSITIONED);
	    return (E_DB_ERROR);
	}

	/*
	** BTREE: fix the page of for lowtid and see if it has changed
	*/
	if (structure == TCB_BTREE)
	{
	    i4 fix_action = DM0P_WRITE;

	    if (r->rcb_access_mode == RCB_A_READ)
		fix_action = DM0P_READ;
	    status = dm0p_fix_page(r,
			    (DM_PAGENO)r->rcb_p_lowtid.tid_tid.tid_page,	
			    fix_action, &r->rcb_other, dberr);
	    if (status != E_DB_OK)
		return (status);

	    status = dm1b_position(r, dberr);

	    if (status != E_DB_OK)
		return (status);

	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
		dberr);

	    if (status)
		return (status);
	}

	/* Set position values to original ones. */	
	r->rcb_lowtid.tid_i4 = r->rcb_p_lowtid.tid_i4;
	r->rcb_hightid.tid_i4 = r->rcb_p_hightid.tid_i4;
	r->rcb_currenttid.tid_i4 = r->rcb_lowtid.tid_i4;

	/*
	** If this is a gateway, then call the gateway to reposition the table.
	*/
	if (t->tcb_rel.relstat & TCB_GATEWAY)
	{
	    status = dmf_gwr_position(r, dberr);
	    if (status)
		return (status);
	}

	r->rcb_state |= RCB_POSITIONED;
	return (E_DB_OK);

    }

    /*	Reinitialize the position criteria in the RCB. */
    r->rcb_state &= ~(RCB_POSITIONED);
    r->rcb_hl_given = 0;
    r->rcb_ll_given = 0;

    /*  Handle the position for BY_TID on a temporary table. */
    if (flag & DM2R_BYTID)
    {
	if (t->tcb_temporary == 0)
	{
	    SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
	    return (E_DB_ERROR);
	}
	tid->tid_tid.tid_line --;
	r->rcb_lowtid.tid_i4 = tid->tid_i4;
	r->rcb_hightid.tid_i4 = -1;
	r->rcb_currenttid.tid_i4 = r->rcb_lowtid.tid_i4;

	/* Save positioned information */

	r->rcb_p_lowtid.tid_i4 = r->rcb_lowtid.tid_i4;
        r->rcb_p_hightid.tid_i4 = r->rcb_hightid.tid_i4;	

	/* Mark as positioned. */

	r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);
	r->rcb_p_flag = RCB_P_BYTID;
	return (E_DB_OK);
	
    }	

    /*  Handle the position to bottom case */
    if (flag & DM2R_LAST)
    {
        switch (structure)
        {
            case TCB_BTREE:
                        status = dm1b_search(r, (char *)NULL, (i4)0,
                                DM1C_LAST, DM1C_NEXT, &r->rcb_other,
                                &r->rcb_lowtid, &r->rcb_currenttid, dberr);
			break;
	    case TCB_ISAM:
			status = dm1i_search(r, &r->rcb_data,
				(char *)NULL, (i4)0,
				DM1C_LAST, DM1C_NEXT,&r->rcb_hightid,dberr);
			break;
            default:
			SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
                        return (E_DB_ERROR);

        }

        /* Record positioning and mark as positioned. */
	if (status == E_DB_OK)
	{
	    r->rcb_p_lowtid.tid_i4 = r->rcb_lowtid.tid_i4;
	    r->rcb_p_hightid.tid_i4 = r->rcb_hightid.tid_i4;	
	    r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);
	    r->rcb_p_flag = RCB_P_LAST;
	}
        return (status);
    }

    /*	Handle the position for full scan case. */
    if (flag & DM2R_ALL)
    {
	/* Mark for READAHEAD operation. */

	dmf_svcb->svcb_stat.rec_p_all++;
	r->rcb_state |= RCB_READAHEAD;

	switch (structure)
	{
	case TCB_HEAP:
	case TCB_HASH:
	case TCB_ISAM:
	    status = dm1s_search(r, &r->rcb_data,
                (char *)NULL, (i4)0, DM1C_EXTREME, DM1C_PREV,
		&r->rcb_lowtid, dberr);
	    status |= dm1s_search(r, 0, (char *)NULL, (i4)0,
                DM1C_EXTREME, DM1C_NEXT, &r->rcb_hightid, dberr);
	    r->rcb_currenttid = r->rcb_lowtid;
            if (row_locking(r) && serializable(r))
            {
                /* To provide phantom protection, upgrade
                ** locking granularity to page level.
                */
                r->rcb_lk_type = RCB_K_PAGE;
            }
	    break;

	case TCB_BTREE:
	    status = dm1b_search(r, (char *)NULL, (i4)0, DM1C_EXTREME,
                DM1C_PREV, &r->rcb_other,
		&r->rcb_lowtid, &r->rcb_currenttid, dberr);
	    r->rcb_lowtid.tid_tid.tid_line--;
	    break;

	case TCB_RTREE:
	    status = dm1m_search(r, (char *)NULL, (i4)0, DM1C_EXTREME,
                DM1C_PREV, &r->rcb_other,
		&r->rcb_lowtid, &r->rcb_currenttid, dberr);
	    r->rcb_lowtid.tid_tid.tid_line--;
	    break;

	case TCB_TAB_GATEWAY:
	    status = dmf_gwr_position(r, dberr);
	    break;
	}

	if (status != E_DB_OK)
	    return (E_DB_ERROR);

	r->rcb_p_lowtid.tid_i4 = r->rcb_lowtid.tid_i4;
        r->rcb_p_hightid.tid_i4 = r->rcb_hightid.tid_i4;	
	r->rcb_p_flag = RCB_P_ALL;
	
	/* Mark as positioned. */

	r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);
	return (E_DB_OK);
    }

    /*
    ** If this is an RTREE request, let rtree_position do the work.
    */
    if (structure == TCB_RTREE)
    {
    	status = rtree_position(r, key_desc, key_count, dberr);
	return(status);
    }

    /* It is illegal to do backward scan on tables other than BTREE */
    if (flag & DM2R_ENDQUAL)
    {
        switch (structure)
        {
            case TCB_BTREE:
		break;
	    default:
		SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
		return(E_DB_ERROR);
	}
    }

    /*	
    **  Prepare for counting low and high key specifiers. 
    **  Initialize the whole array to prevent B105190.
    */
    MEfill(sizeof(lk_map), '\0', lk_map);
    MEfill(sizeof(hk_map), '\0', hk_map);

    lk_count = 0;
    hk_count = 0;
    hk_type = 0;
    lk_type = 0;

    /*  Build low and high keys from key description. */
    for (i = 0; i < key_count; i++, k++)
    {
	if ( ( (t->tcb_rel.relversion != 0) &&
	     ( (a[k->attr_number].ordinal_id != k->attr_number) ||
	       (a[k->attr_number].ver_dropped != 0) ) ) )
	{
	   for (j=1; j <= t->tcb_rel.relatts; j++)
	   {
	       if ( (a[j].ordinal_id == k->attr_number) &&
		    (a[j].ver_dropped == 0) )
	       {
		    k->attr_number = a[j].intl_id;
		    break;
	       }
	   }
	}

        /*	Check for attribute number is a key. */
        if (a[k->attr_number].key == 0)
	    continue;

        /*	Collect the low key specifiers. */
        if (k->attr_operator != DM2R_LTE)
        {
	    if (((lk_map[k->attr_number >> 5] >> (k->attr_number & 0x1f))
	        & 1) == 0)
	    {
	        MEcopy(k->attr_value, a[k->attr_number].length,
		        &r->rcb_ll_ptr[a[k->attr_number].key_offset]);
		lk_map[k->attr_number >> 5] |=
		    (1 << (k->attr_number & 0x1f));
		lk_count++;

		/*
		**  Set the low-key type equal to the type of the first
		**  key specified.  If the type of the next key part is 
		**  different from the previous key part, change both to 
		**  range key.
		**  Don't change high-key type if it has never been set.
		*/

		if (lk_type == 0)
		    lk_type = k->attr_operator;
		else if (lk_type != k->attr_operator)
		{
		    if (hk_type != 0)
		    	hk_type = DM2R_LTE;
		    lk_type = DM2R_GTE;
		}
	    }
        }
        if (k->attr_operator != DM2R_GTE)
        {
	    /*  Collect high key specifiers. */
	    if (((hk_map[k->attr_number >> 5] >> (k->attr_number & 0x1f))
		& 1) == 0)
	    {
		MEcopy(k->attr_value, a[k->attr_number].length,
			&r->rcb_hl_ptr[a[k->attr_number].key_offset]);
		hk_map[k->attr_number >> 5] |=
		        (1 << (k->attr_number & 0x1f));
		hk_count++;

		/*
		**  Set the high-key type equal to the type of the first
		**  key specified.  If the type of the next key part is 
		**  different from the previous key part, change both to 
		**  range key.
		**  Don't change low-key type if it has never been set.
		*/
		if (hk_type == 0)
		    hk_type = k->attr_operator;
		else if (hk_type != k->attr_operator)
		{
		    if (lk_type != 0)
		    	lk_type = DM2R_GTE;
		    hk_type = DM2R_LTE;
		}
	    }
	}
    }

    /*  Determine the length of the prefix in a partial key. */
    if (hk_count && hk_count != t->tcb_rel.relkeys)
    {
        for (hk_count = 0 ; hk_count < t->tcb_keys; hk_count++)
	    if ((hk_map[t->tcb_att_key_ptr[hk_count] >> 5] &
		(1 << (t->tcb_att_key_ptr[hk_count] & 0x1f))) == 0)
	    {
	        break;
	    }
	if (hk_count == 0)
	    hk_type = 0;
    }

    if (lk_count && lk_count != t->tcb_rel.relkeys)
    {
        for (lk_count = 0; lk_count < t->tcb_keys; lk_count++)
	    if ((lk_map[t->tcb_att_key_ptr[lk_count] >> 5] &
		(1 << (t->tcb_att_key_ptr[lk_count] & 0x1f))) == 0)
	    {
	        break;
	    }
	if (lk_count == 0)
	    lk_type = 0;
    }

    /*
    **  At this point we have the following cases:
    **
    **  lk_count is the number of prefix key attributes for the low key.
    **  hk_count is the number of prefix key attributes for the high key.
    **  lk_type is the equality type of the low key.
    **  hk_type is the equality type of the high key.
    **
    **  BOF - beginning of file.
    **  EOF - end of file.
    **  LOW - set low partial or full key.
    **  HIGH - set high partial or full key.
    **  EXACT - set exact key.
    **
    **  lk_count=part lk_type=EQ  hk_count=part hk_type=EQ  LOW, HIGH
    **  lk_count=full lk_type=EQ  hk_count=full hk_type=EQ  EXACT
    **
    **  lk_count=0    lk_type=0   hk_count=part hk_type=LTE BOF, HIGH
    **  lk_count=0    lk_type=0   hk_count=full hk_type=LTE BOF, HIGH
    **
    **  lk_count=part lk_type=GTE hk_count=0    hk_type=0   LOW, EOF
    **  lk_count=full lk_type=GTE hk_count=0    hk_type=0   LOW, EOF
    **
    **  lk_count=part lk_type=GTE hk_count=part hk_type=LTE LOW, HIGH
    **  lk_count=part lk_type=GTE hk_count=full hk_type=LTE LOW, HIGH
    **  lk_count=full lk_type=GTE hk_count=part hk_type=LTE LOW, HIGH
    **  lk_count=full lk_type=GTE hk_count=full hk_type=LTE LOW, HIGH
    */

    /*	Return error for no useful keys. */
    if (structure == TCB_HEAP ||
	(lk_count == 0 && hk_count == 0) ||
	(structure == TCB_HASH &&
	    (hk_count != t->tcb_keys || hk_type != DM2R_EQ)))
    {
	/*  No useable access path found,
	**  return error status
	*/
	SETDBERR(dberr, 0, E_DM008E_ERROR_POSITIONING);
	return (E_DB_ERROR);
    }
	
    if (structure == TCB_TAB_GATEWAY)
    {
	r->rcb_ll_given = lk_count;
	r->rcb_hl_given = hk_count;
	status = dmf_gwr_position(r, dberr);
	if (status != E_DB_OK)
	    return(status);
    }
    else if (structure == TCB_HASH)
    {
	/*  Build the tuple from key description. */
	for (k = key_desc, i = 0; i < key_count; i++, k++)
	{
	    MEcopy(k->attr_value, a[k->attr_number].length,
		&r->rcb_record_ptr[a[k->attr_number].offset]);
	}

        /*
        ** If row locking get value locks if when we position .
	** Request LOGICAL value lock, it's not going to get released
        */
        if ((row_locking(r)) && (r->rcb_iso_level == RCB_SERIALIZABLE))
        {
	    status = dm1r_lock_value(rcb, (i4)(DM1R_PHANPRO_VALUE), 
			r->rcb_record_ptr, dberr);
	    if (status != E_DB_OK)
	        return (status);
        }

	status = dm1h_search(r, &r->rcb_data, r->rcb_record_ptr,
	    DM1C_FIND, DM1C_EXACT, &r->rcb_lowtid, dberr);
	if (status != E_DB_OK)
	    return (status);

	r->rcb_hl_given = hk_count;
	r->rcb_hl_op_type = RCB_EQ;
	r->rcb_hightid = r->rcb_lowtid;
	dmf_svcb->svcb_stat.rec_p_exact++;
    }
    else
    {	
	/*  At this point we have ISAM or BTREE left.  */
	if (lk_count == 0)
	{
	    /*	Set low to beginning of file.	*/
	    if (structure == TCB_ISAM)
	    {	
		status = dm1i_search(r, &r->rcb_data, (char *)NULL,
		    (i4)0, DM1C_EXTREME, DM1C_PREV, &r->rcb_lowtid,
		    dberr);
	        if (status != E_DB_OK)
		    return (status);
	    }
	    else
	    {
    		if (flag & DM2R_QUAL)
		{
		    status = dm1b_search(r, (char *)NULL, (i4)0,
		        DM1C_EXTREME, DM1C_PREV, &r->rcb_other,
		        &r->rcb_lowtid, &r->rcb_currenttid, dberr);
		    r->rcb_lowtid.tid_tid.tid_line--;
	            if (status != E_DB_OK)
			return (status);
		}
	    }
	    if ((flag & DM2R_NOREADAHEAD) == 0)
	        r->rcb_state |= RCB_READAHEAD;
	}
	else if (hk_count == 0)
	{
	    /*	Set high to end of file. */
	    if (structure == TCB_ISAM)
	    {
		status = dm1i_search(r, (DMP_PINFO*)NULL, (char *)NULL, 
			(i4)0, DM1C_EXTREME, DM1C_NEXT, &r->rcb_hightid, 
			dberr);
		if (status != E_DB_OK)
		    return (status);
	    }
	    else 
	    {
    		if (flag & DM2R_ENDQUAL)
		{
		    /* qualified backward scan from EOF */
		    status = dm1b_search(r, (char *)NULL, (i4)0,
		        DM1C_LAST, DM1C_NEXT, &r->rcb_other,
		        &r->rcb_lowtid, &r->rcb_currenttid, dberr);
	    	    if (status != E_DB_OK)
			return (status);
		}
	    }
	    if ((flag & DM2R_NOREADAHEAD) == 0)
	        r->rcb_state |= RCB_READAHEAD;
	}
	if (lk_type == DM2R_GTE)
	{
	    /* Set lower bound. */
	    if (structure == TCB_ISAM)
	    {
		status = dm1i_search(r, &r->rcb_data, r->rcb_ll_ptr, 
		    lk_count, DM1C_RANGE, DM1C_PREV, &r->rcb_lowtid, dberr);
	    	if (status != E_DB_OK)
		    return (status);
	    }
	    else
	    {
    		if (flag & DM2R_QUAL)
		{
		    status = dm1b_search(r, r->rcb_ll_ptr, lk_count,
		        DM1C_RANGE, DM1C_PREV, &r->rcb_other, 
			&r->rcb_lowtid, &r->rcb_currenttid, dberr);
		    r->rcb_lowtid.tid_tid.tid_line--;
	    	    if (status != E_DB_OK)
			return (status);
		}
	    }
	    r->rcb_ll_op_type = RCB_GTE;
	    r->rcb_ll_given = lk_count;
	    if ((flag & DM2R_NOREADAHEAD) == 0)
	        r->rcb_state |= RCB_READAHEAD;
	}
	if (hk_type == DM2R_LTE)
	{
	    /* Set high bound. */
	    if (structure == TCB_ISAM)
	    {
		status = dm1i_search(r, (DMP_PINFO*)NULL, r->rcb_hl_ptr, 
				hk_count, DM1C_RANGE, DM1C_NEXT,
		    		&r->rcb_hightid, dberr);
	    	if (status != E_DB_OK)
		    return (status);
	    }
	    else
	    {
    		if (flag & DM2R_ENDQUAL)
		{
		    status = dm1b_search(r, r->rcb_hl_ptr, hk_count,
		        DM1C_RANGE, DM1C_NEXT, &r->rcb_other, 
			&r->rcb_lowtid, &r->rcb_currenttid, dberr);
	    	    if (status != E_DB_OK)
			return (status);
		}
	    }
	    r->rcb_hl_given = hk_count;
	    r->rcb_hl_op_type = RCB_LTE;
	    if ((flag & DM2R_NOREADAHEAD) == 0)
	        r->rcb_state |= RCB_READAHEAD;
	}
	if (hk_type == DM2R_EQ)
	{
	    if (flag & DM2R_ENDQUAL)
	    {
		SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
		return(E_DB_ERROR);
	    }
	    if (hk_count != t->tcb_keys)
	    {
		/* Exact match on partial key. */
		if (structure == TCB_ISAM)
		{
		    status = dm1i_search(r, &r->rcb_data,
			r->rcb_hl_ptr, hk_count, DM1C_RANGE, DM1C_PREV,
			&r->rcb_lowtid, dberr);
		    if (status != E_DB_OK)
			return (status);
		    status = dm1i_search(r, (DMP_PINFO*)NULL, r->rcb_hl_ptr, 
			hk_count, DM1C_RANGE, DM1C_NEXT, &r->rcb_hightid, 
			dberr);
		    if (status != E_DB_OK)
			return (status);
		    r->rcb_ll_given = lk_count;
		    r->rcb_ll_op_type = RCB_GTE;
		    MEcopy(r->rcb_hl_ptr, t->tcb_klen, r->rcb_ll_ptr);
		    if (r->rcb_hightid.tid_tid.tid_page
				       > r->rcb_lowtid.tid_tid.tid_page)
		    {
	    		if ((flag & DM2R_NOREADAHEAD) == 0)
			    r->rcb_state |= RCB_READAHEAD;

		        /* 
		        ** For 2k secondary ISAM unique index, TIDP is part of 
		        ** key. Therefore on equal search on key, donot do 
			** read-ahead.
		        */
			if ((t->tcb_rel.relpgtype == TCB_PG_V1) &&
		            (t->tcb_rel.relstat & TCB_INDEX) &&
			    (t->tcb_rel.relstat & TCB_UNIQUE) &&
			    (hk_count == (t->tcb_keys - 1)))
		           r->rcb_state &= ~RCB_READAHEAD;
		    }
		}
		else
		{
		    status = dm1b_search(r, r->rcb_hl_ptr, hk_count,
			DM1C_RANGE, DM1C_PREV,
			&r->rcb_other, &r->rcb_lowtid,
			&r->rcb_currenttid, dberr);
		    if (status != E_DB_OK)
			return (status);
		    r->rcb_lowtid.tid_tid.tid_line--;
	    	    if ((flag & DM2R_NOREADAHEAD) == 0)
		        r->rcb_state |= RCB_READAHEAD;

		    /* 
		    ** For 2k secondary BTREE unique index, TIDP is part of key.
		    ** Therefore on equal search on key, donot do read-ahead.
		    */
		    if ((t->tcb_rel.relpgtype == TCB_PG_V1) &&
		        (t->tcb_rel.relstat & TCB_INDEX) &&
			(t->tcb_rel.relstat & TCB_UNIQUE) &&
			(hk_count == (t->tcb_keys - 1)))
		       r->rcb_state &= ~RCB_READAHEAD;
		}
		r->rcb_hl_given = hk_count;
		r->rcb_hl_op_type = RCB_LTE;
	    }
	    else
	    {
		/*	Exact match on full key. */
	        /*  Build the tuple from key description. */
	        for (k = key_desc, i = 0; i < key_count; i++, k++)
	        {
	            MEcopy(k->attr_value, a[k->attr_number].length,
		        &r->rcb_record_ptr[a[k->attr_number].offset]);
	        }

                /*
                ** If row locking get value locks if when we position .
                */
                if ((row_locking(r)) && (r->rcb_iso_level == RCB_SERIALIZABLE))
                {
	            status = dm1r_lock_value(rcb, 
				(i4)(DM1R_LK_PHYSICAL | DM1R_PHANPRO_VALUE), 
				r->rcb_record_ptr, dberr);
	            if (status != E_DB_OK)
			return (status);
                }

		if (structure == TCB_ISAM)
		{
		    status = dm1i_search(r, &r->rcb_data,
			r->rcb_hl_ptr, hk_count, DM1C_FIND, DM1C_EXACT,
			&r->rcb_lowtid, dberr);
		    r->rcb_hightid = r->rcb_lowtid;
		}
		else
		{
		    /* Btree position for exact match use readahead too. */
		    status = dm1b_search(r, r->rcb_hl_ptr, hk_count,
			    DM1C_FIND, DM1C_EXACT, &r->rcb_other, 
			    &r->rcb_lowtid, &r->rcb_currenttid, dberr);

		    if (status != E_DB_OK && dberr->err_code == E_DM0056_NOPART)
		    {
			SETDBERR(dberr, 0, E_DM0055_NONEXT);
			return (status);
		    }
		    r->rcb_lowtid.tid_tid.tid_line--;
		}	
                r->rcb_ll_given = lk_count;
                r->rcb_ll_op_type = RCB_EQ;
		r->rcb_hl_given = hk_count;
		r->rcb_hl_op_type = RCB_EQ;
		if (status != E_DB_OK)
		    return (status);
		dmf_svcb->svcb_stat.rec_p_exact++;
	    }
	}
	else
	    dmf_svcb->svcb_stat.rec_p_range++;
    }

    /* Save original hk_type */
    switch (hk_type)
    {
	case DM2R_LTE:  r->rcb_p_hk_type = RCB_P_LTE;
			break;
	case DM2R_GTE:  r->rcb_p_hk_type = RCB_P_GTE;
			break;
	case DM2R_EQ:   r->rcb_p_hk_type = RCB_P_EQ;
			break;
	default:        r->rcb_p_hk_type = 0;
			break;
    }

    /* Save original lk_type */
    switch (lk_type)
    {
	case DM2R_LTE:  r->rcb_p_lk_type = RCB_P_LTE;
			break;
	case DM2R_GTE:  r->rcb_p_lk_type = RCB_P_GTE;
			break;
	case DM2R_EQ:   r->rcb_p_lk_type = RCB_P_EQ;
			break;
	default:        r->rcb_p_lk_type = 0;
		        break;
    }

    /* Mark as positioned */
    r->rcb_p_lowtid.tid_i4 = r->rcb_lowtid.tid_i4;
    r->rcb_p_hightid.tid_i4 = r->rcb_hightid.tid_i4;	
    if (flag & DM2R_ENDQUAL)
        r->rcb_p_flag = RCB_P_ENDQUAL;
    else
        r->rcb_p_flag = RCB_P_QUAL;
    r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);

    return (E_DB_OK);
}


/*{
** Name: rtree_position - Position Rtree for retrieval/update.
**
** Description:
** 	Handle positioning when called by dm2r_position to do
** 	DM2R_POSITION with the following operation types:
** 	- DMR_OP_OVERLAY - indexed object overlaps search object
** 	- DMR_OP_INTERSECTS - indexed object intersects search object
** 	- DMR_OP_INSIDE - indexed object is inside search object
** 	- DMR_OP_CONTAINS - indexed object contains search object, i.e.,
** 			  search object inside indexed object
**
** Inputs:
**      rcb                             Pointer to record access context
**                                      which contains table control
**                                      block (TCB) pointer, tran_id,
**                                      and lock information associated
**                                      with this request.
**	key_desc			Pointer to an array of DM2R_KEY_DESC
**					entries that describe the position
**					qualification.
**	key_count			The number of key_desc entries.
**
** Outputs:
**      err_code                        Pointer to an area to return
**                                      errors when E_DB_ERROR is returned.
** Returns:
**
** Exceptions:
**	    none
**
** Side Effects:
**
** History:
** 	23-aug-1996 (wonst02)
** 	    Integrate a change for the Alter Table Project: recompute
** 	    attribute number if any attribute dropped.
**      23-may-2002 (horda03) Bug 107439
**          The attributes array in the TCB is accessed 1..relatts,
**          not 0..(relatts - 1). This can lead to problems when a column is added
**          to a table and made the index (its the last attribute of the table).
**/

DB_STATUS
rtree_position(
    DMP_RCB		*rcb,
    DM2R_KEY_DESC	*key_desc,
    i4		key_count,
    DB_ERROR		*dberr)
{
    DMP_RCB	*r = rcb;			/* local RCB pointer */
    DMP_TCB	*t = rcb->rcb_tcb_ptr;		/* TCB pointer */
    DB_ATTS	*a = t->tcb_atts_ptr;		/* Attribute array */
    DM2R_KEY_DESC *k = key_desc;		/* Key description array */
    i4	direction;			/* Search direction */
    DB_STATUS	status;     			/* Routine return status. */
    i4	i, j;				/* counters */
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (key_count != 1)
    {
    	/*
	** Only 1 key is supported currently.
	*/
	SETDBERR(dberr, 0, E_DM008E_ERROR_POSITIONING);
	return(E_DB_ERROR);
    }

    if      (k->attr_operator == DMR_OP_OVERLAY)
    	direction = DM1C_OVERLAPS;
    else if (k->attr_operator == DMR_OP_INTERSECTS)
    	direction = DM1C_INTERSECTS;
    else if (k->attr_operator == DMR_OP_INSIDE)
    	direction = DM1C_INSIDE;
    else if (k->attr_operator == DMR_OP_CONTAINS)
    	direction = DM1C_CONTAINS;
    else
    {
    	/*
	** No usable operator found.
	*/
	SETDBERR(dberr, 0, E_DM008E_ERROR_POSITIONING);
	return(E_DB_ERROR);
    }

    /*
    ** With Alter Table, must revise attribute number if any dropped.
    */
    if ( ( (t->tcb_rel.relversion != 0) &&
         ( (a[k->attr_number].ordinal_id != k->attr_number) ||
           (a[k->attr_number].ver_dropped != 0) ) ) )
    {
       for (j=1; j <= t->tcb_rel.relatts; j++)
       {
           if ( (a[j].ordinal_id == k->attr_number) &&
    	   	(a[j].ver_dropped == 0) )
           {
    	   	k->attr_number = a[j].intl_id;
    	   	break;
           }
       }
    }

    /*  Build the tuple from key description. */

    for (k = key_desc, i = 0; i < key_count; i++, k++)
    {
        MEcopy(k->attr_value, a[k->attr_number].length,
	       &r->rcb_hl_ptr[a[k->attr_number].offset]);
    }
    r->rcb_hl_given = key_count;
    r->rcb_hl_op_type = (
    		direction == DM1C_OVERLAPS	? RCB_OVERLAPS	:
    		direction == DM1C_INTERSECTS	? RCB_INTERSECTS:
    		direction == DM1C_INSIDE	? RCB_INSIDE	:
    		/* otherwise DM1C_CONTAINS     */ RCB_CONTAINS	);
    if (r->rcb_other.page)
    {
	status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
		dberr);
	if (status != E_DB_OK)
    	    return(status);
    }
    status = dm1m_search(r, r->rcb_hl_ptr, key_count,
                	 DM1C_RANGE, direction, &r->rcb_other,
			 &r->rcb_lowtid, &r->rcb_currenttid, dberr);

    if (status != E_DB_OK && dberr->err_code == E_DM0056_NOPART)
    {
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
	return(status);
    }
    r->rcb_lowtid.tid_tid.tid_line--;
    r->rcb_hightid = r->rcb_lowtid;
    if (status != E_DB_OK)
	return(status);
    dmf_svcb->svcb_stat.rec_p_exact++;

    /* Mark as positioned. */

    r->rcb_p_lowtid.tid_i4 = r->rcb_lowtid.tid_i4;
    r->rcb_p_hightid.tid_i4 = r->rcb_hightid.tid_i4;
    r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);

    return(status);

}


/*{
** Name: dm2r_put	- Put a record.
**
** Description:
**      This function adds a record to a table.  The table must be open
**      for writing.  All secondary indices are updated.  If the record is
**      a duplicate or the key is a duplicate the operation will be rejected.
**	A put operation will not disrupt the current position of a record
**	stream.
**
** Inputs:
**      rcb                             The RCB to add the record to.
**	flag				Optional flags: DM2R_NODUPLICATES
**                                      or DM2R_DUPLICATES.
**      record                          Pointer to record to add.
**
** Outputs:
**      err_code                        The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL			Operation was partially completed,
**					the transaction must be aborted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-dec-1985 (derek)
**          Created new for jupiter.
**      26_feb_87 (jennifer)
**          Added code for unique secondary indices.
**	31-Mar-1987 (ac)
**	    Added read/nolock support.
**	15-Jun-88 (rogerk)
**	    Log logical puts to base and secondary indexes if using fast commit.
**	26-feb-89 (mmm)
**	    Just added a comment about logical logging of
**	    appends to btree secondary indexes to dm2r_put().
**	10-apr-89 (mmm)
**	    Logical key development. Added a check for a "system maintained"
**	    table and call dm1c_sys_init() if set.
**	29-may-89 (rogerk)
**	    Put in checks for errors from dm1c_comp_rec.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Call gateway to add records to
**	    a gateway table.  Use rcb_logging/tcb_logging fields to determine
**	    appropriate logging actions.
**       4-sep-89 (rogerk)
**          Put in trace flag to bypass SBI logging.
**	 5-dec-1989 (rogerk)
**	    Put back in the BI logging of BTREE leaf pages.  These are needed
**	    for UNDO recovery by the online-backup code during processing
**	    of the DUMP file information.
**	11-jan-1990 (rogerk)
**	    Only do BI logging of BTREE leaf pages if we are doing online
**	    backup.  The rcb_bt_bi_logging is enabled if online backup is
**	    going on, and the dm0l_bi() routine is only called if this flag
**	    is enabled.
**	25-jan-1990 (fred)
**	    Added code to insert the necessary peripheral objects into the
**	    extension tables.  This is for peripheral (== large) object support.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  Check rcb_logging status
**		before updating system catalogs and secondary indexes.  Use
**		rcb_logging rather than tcb_logging to direct logging actions
**		on 2nd indexes.
**	    08-feb-1991 (mikem)
**		Added code to dm2r_put() to set new fields in the rcb which
**		keep track of the last logical key assigned and their types.
**		Support needed to be able to return this information to the
**		client which caused the insert.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Replaced calls of dm1c_compress with accessors.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Took out calls to log put, delete and
**	    replace operations.  These are now logged from inside the page
**	    updating routines in DM1R.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix core catalog data page as soon as base table PUT is
**		    complete. This improves concurrency, but more importantly
**		    it satisfies the only-fix-one-core-catalog protocol for
**		    node failure recovery.
**		Improve error logging in dm2r_put.
**      15-Apr-1994 (fred)
**              Pass interrupts from dmpe routines back up the chain.
**	19-Apr-1995 (cohmi01)
**	    if 'checkeof' is on for a secondary index, call dm1p_checkhwm()
**	    to adjust eof so that group buffers may be used.
**	17-Jul-1995 (cohmi01)
**	    Utilize Readahead threads to prefetch sec index pages that will
**	    be needed when processing an insert. This is only if more than
**	    a certain # of indexes exist, to ensure possibility of IO overlap.
**	20-may-96 (stephenb)
**	    Perform replication data capture for inserts and open input
**	    queue if necesary.
**	5-aug-96 (stephenb)
**	    Store remote tx id when the replication server adds records to
**	    the input queue.
**	13-Aug-1996 (prida01)
**	    Give relatts correct i4 type for call to dm1c_pput
**	24-Oct-1996 (somsa01)
**	    Relocated dm1c_pput() code; if a table contains blobs, first
**	    logically lock the appropriate page in the base table. After
**	    that is done, then move the blob segment tuples into the ETAB
**	    table from the appropriate ETAB temporary table.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Row locking: Acquire value lock for duplicate checking
**          Check for dups in secondary index before allocate in base
**          Moved common code to si_put()
**	07-jan-1997 (nanpr01)
**	    Removed the extra code left out from cross-integration. Also
**	    corrected the error handling.
**	13-mar-97 (stephenb)
**	    When adding a record to dd_databases, check wether replication 
**	    should be enabled, conditions are that we have created all of
**	    the replicator catalogs, and the record we are adding is one
**	    for our local database.
**      10-mar-1997 (stial01)
**          dm2r_put() Removed EXTRA si_dupcheck added by cross-integration
**              of blob changes. Those changes also required changes in
**              dm1r_allocate,dm1r_put.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Set RCB_*_UPDATED flags when record is inserted.
**      21-may-1997 (stial01)
**          dm2r_put() Get rid of local page pointers. dm1b_put may change
**          rcb_other_page_ptr when it repositions to the reserved leaf entry.
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**          dm2r_put() dm2r_return_rcb will unfix pages, remove extra unfix.
**	3-jun-97 (stephenb)
**	    if the user is updating the replicator input queue and 
**	    replication is active, then set the input queue value in the
**	    transaction control block so that the input queue values will
**	    be distributed on commit.
**	29-jul-97 (stial01)
**	    dm2r_put() Pass opflag DM1C_PUT to dm1*_put, si_put for logging
**	23-oct-1997 (nanpr01)
**	    Assumption of record always on word/half-word boundary is wrong and
**	    causes bus error time to time from move config.
**	28-jul-1998 (somsa01)
**	    Added another parameter to dm1c_pput().  (Bug #92217)
**	22-oct-98 (stephenb)
**	    Make sure logging and journalling characteristics of the replication
**	    input queue RCB match those of the base table.
**      05-may-1999 (nanpr01)
**          dm2r_put() Lock/unlock key value to prevent: 
**          Insert of key that a serializable transaction has positioned on.
**          Concurrent dup checking for same key by different transactions 
**          Defer SI dupcheck until after base table update if possible
**	17-dec-99 (gupsh01)
**	    Added additional check to give out correct error message in case of 
**	    updating a readonly database. 
**      1-Jul-2000 (wanfr01)
**          Bug 101521, INGSRV 1226 - If using RAAT, and II_RAAT_RETRY is sent,
**          reattempt to acquire the lock.  If there is a timeout, return the
**          new error code E_DM0173_RAAT_SI_TIMER.   This reduces the chance
**          of corrupting the secondary index.
**      23-may-2002 (horda03) Bug 107439
**          The attributes array in the TCB is accessed 1..relatts,
**          not 0..(relatts - 1). This can lead to problems when a column is added
**          to a table and made the index (its the last attribute of the table).
**      07-apr-2005 (huazh01)
**          if table is to be replicated to another database, then use 
**          RCB of dd_input_queue to decide whether or not journaling 
**          is needed. 
**          This fixes b114215, INGSRV3229. 
**	15-Jan-2004 (schka24)
**	    Hand off logical key stuff to dm1c.
**	22-Jan-2004 (jenjo02)
**	    Added support for parallel SI update threads.
**	26-Jan-2004 (jenjo02)
**	    Get SI list from master's TCB.
**      02-Mar-2005 (inifa01) INGREP167 b113572
**	    Problems when using Replicator 1.1 in a 2.5 database.
**	    Sometimes and randomly the 2.5 change recording becomes
**	    active causing several problems(duplicate information in
**	    shadow table and dd_distrib_queue).  Investigations
**	    determined that sometimes dm2r_get() does not retrieve the
**	    dd_transaction_id table from iirelation even though it is
**	    present.
**	    FIX: Introduced use of system variable II_DBMS_REP to switch
**	    OFF any replication processing by the DBMS server.
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
**	31-Oct-2005 (gupsh01)
**	    Fixed testing for dbms_replicate. The test below caused it
**	    not to replicate in all cases.
**	9-Nov-2005 (schka24)
**	    Don't fetch II_DBMS_REP in the middle of put!  Check a new
**	    svcb status bit set at startup.
**	18-Aug-2008 (kibro01) b120490
**	    Do not allow replication on a non-journalled table within a
**	    journalled database.
**	28-jan-2009 (stephenb proxy for sprint fix in summer '08)
**	   add call to dma_row_access() to ensure that inserts are audited
**	   correctly for row auditing.
**       5-Feb-2009 (hanal04) Bug 121564
**         Correct previous change for Bug 121564. dma_row_access() now expects
**         a DB_ERROR pointer not an i4 err_code.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Logical keys for LOB's are no longer done here, but in dmpe.
**	    Update the pput call.
**	    After doing the put, if there are LOB's, do a pget to fix up
**	    the coupons with "get" info.  The record might be going to
**	    a rule dbproc later.
**	30-Aug-2010 (miket) SIR 122403 SD 146530
**	    We need to re-encrypt after dupe update to refresh the coupon
**	    with the table id of the permanent etab.
*/
DB_STATUS
dm2r_put(
    DMP_RCB     *rcb,
    i4    	flag,
    char        *record,
    DB_ERROR	*dberr )
{
    DMP_RCB		*r = rcb;
    DMR_RTB		*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_RCB		*ir;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_TCB		*it;
    DMP_DCB		*dcb = rcb->rcb_tcb_ptr->tcb_dcb_ptr;
    DM_TID		tid;
    DM_TID		bid;
    char		*rec;
    char		*crec = (PTR)0;
    i4			rec_len;
    i4			i;
    DB_STATUS		status;
    i4			opflag;
    i4			loc_err;
    DB_TAB_TIMESTAMP	local_timestamp;
    i4			rcb_si_flags;
    DB_STATUS		local_status;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    bool		value_lock = FALSE;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9314_DM2R_PUT);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9314_DM2R_PUT);
#endif

    /* Base table put: */
    status = E_DB_OK;

    for (;;) /* something to break from */
    {
	/* TID for secondary index puts not yet known */
	r->rcb_si_newtid.tid_i4 = NOT_ALLOCATED;

	r->rcb_compare = 0;
	rcb_si_flags = 0;

	dmf_svcb->svcb_stat.rec_put++;
	r->rcb_s_ins++;

	/*	Check for write access. */

	if (r->rcb_access_mode != RCB_A_WRITE)
	{		
	    if (r->rcb_xcb_ptr->xcb_x_type & XCB_RONLY)
		SETDBERR(dberr, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    else
		SETDBERR(dberr, 0, E_DM0073_RECORD_ACCESS_CONFLICT);
	    status = E_DB_ERROR;
	    break;
	}

#ifdef NEVER_USED
	/* cancel all outstanding index related prefetch readahead requests */
	if (r->rcb_state & RCB_PREFET_PENDING)
	    dm2r_cancel_prefetch(r, PFR_CANCEL_IX);
#endif

	/* if update lock is set in RCB, reset it */
	r->rcb_state &= ~RCB_UPD_LOCK;

	/* Do we need to do duplicate checking on base table?
	** No duplicate checking if:
	** Duplicates allowed and non-unique key
	** Replaying online modify sidefile updates
	*/
	if ( (flag & DM2R_DUPLICATES &&  !(t->tcb_rel.relstat & TCB_UNIQUE)) ||
	     (flag & DM2R_REDO_SF) )
	    opflag = DM1C_DUPLICATES;
	else
	    opflag = DM1C_NODUPLICATES;

	/* If duplicate found, will caller continue or rollback */
	if ( flag & DM2R_DUP_CONTINUE || DMZ_TBL_MACRO(20))
	{
	    /* This applies to the Base table */
	    flag |= DM2R_DUP_CONTINUE;

	    /*
	    ** When DUP_CONTINUE, unique secondary indexes must be
	    ** checked for duplicates before the base table
	    ** put is done.
	    */
	    if ( t->tcb_unique_index && 
		(t->tcb_rel.relstat & TCB_GATEWAY) == 0 )
	    {
		rcb_si_flags |= RCB_SI_DUP_PRECHECK;
		if ( r->rcb_siAgents )
		    rtb->rtb_si_dupchecks = t->tcb_unique_count;
	    }
	}

	/* Check for system/dmf initialized attributes */
	/* Skip if redoing online modify sidefile updates */
	if ((t->tcb_rel.relstat & TCB_SYS_MAINTAINED) &&
	    (flag & DM2R_REDO_SF) == 0)
	{
	    u_i4	high;
	    u_i4	low;

	    /* Get logical key, generating new one if needed */
	    status = dm1c_get_high_low_key(t, r, &high, &low, dberr);

	    if ( status == E_DB_OK )
	    {
		status = dm1c_sys_init(&r->rcb_logkey, t->tcb_atts_ptr,
			      (i4)t->tcb_rel.relatts, record,
			      &r->rcb_val_logkey, dberr);
	    }

	    if ( status )
	    {
		if (dberr->err_code == E_DM9380_TAB_KEY_OVERFLOW)
		{
		    /* error can be returned from dm1c_sys_init(), not logged
		    ** there because it doesn't have the necessary context for
		    ** the error parameters.
		    */
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4) 0, (i4 *)NULL,
			err_code, 3,
			sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
			sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
			sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);

		    SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
		}
		break;
	    }
	}
        if (t->tcb_temporary == TCB_PERMANENT)
        {
            /* Verify we have access to the row we are trying to create */
            if ( dmf_svcb->svcb_status & SVCB_IS_SECURE )
            {
                /*
	        ** Check MAC label/access
	        */
                i4     compare;
                status = dma_row_access(
                    DMA_ACC_INSERT,
                    r,
                    record,             /* New pointer */
                    NULL, /* Old pointer */
                    &compare, dberr);

                if (status != E_DB_OK)
                    break;
            }
        }
	break;
    }

    /* If SI agents, we need the mutex */
    if ( r->rcb_siAgents )
	CSp_semaphore(1, &rtb->rtb_cond_sem);

    /* Add the resulting flags to the Base RCB */
    r->rcb_si_flags |= rcb_si_flags;
    /* Un-busy the Base RCB */
    r->rcb_si_flags &= ~RCB_SI_BUSY;

    /* Wake up all waiters on this busy RCB */
    if ( r->rcb_siAgents )
    {
	rtb->rtb_dm2r_flags = flag;
	rtb->rtb_status = status;
	rtb->rtb_error = *dberr;
	if ( rtb->rtb_cond_waiters )
	    CScnd_broadcast(&rtb->rtb_cond);
	CSv_semaphore(&rtb->rtb_cond_sem);
    }

    if ( status )
	return(SIcomplete(r, status, dberr));

    /*
    **  Check for duplicate index values unless Secondary Indexes
    **  are being processed separately..
    **
    **    We can check for dups before we allocate space in the base
    **    table because the tid is not used during duplicate checking.
    **    We only do the si dupcheck now if
    **	  rcb_si_flags & RCB_SI_DUP_PRECHECK.
    **    In this case we must dupcheck before updating the base table.
    */
    if ( r->rcb_si_flags & RCB_SI_DUP_PRECHECK )
    {
	if ( r->rcb_siAgents )
	{
	    /* Must wait for all threaded dupchecks to complete */
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    while ( rtb->rtb_status == E_DB_OK &&
		    rtb->rtb_si_dupchecks > 0 )
	    {
		rtb->rtb_cond_waiters++;
		CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
		rtb->rtb_cond_waiters--;
	    }
	    /* Collective "status" from all dupchecks */
	    status = rtb->rtb_status;
	    *dberr = rtb->rtb_error;
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
	else
	{
	    /* Dupcheck -all- indexes */
	    status = si_dupcheck(t, r, record, 
				 flag | DM2R_MPUT, dberr);
	}
    }

    /*	Check for compressed storage and encryption. */
    rec_len = t->tcb_rel.relwid;
    rec = record;

    /* If there are encrypted columns, encrypt the record */
    if ( status == E_DB_OK && t->tcb_rel.relencflags & TCB_ENCRYPTED)
    {
	status = dm1e_aes_encrypt(r, &t->tcb_data_rac, rec,
			r->rcb_erecord_ptr, dberr);
	rec = r->rcb_erecord_ptr;
    }

    /*	Check for compressed storage. */
    if ( status == E_DB_OK && t->tcb_data_rac.compression_type != TCB_C_NONE )
    {
	rec = crec = r->rcb_record_ptr;
	status = (*t->tcb_data_rac.dmpp_compress)(&t->tcb_data_rac,
	    (t->tcb_rel.relencflags & TCB_ENCRYPTED) ?
	    r->rcb_erecord_ptr : record,
	    (i4)t->tcb_rel.relwid, rec, &rec_len);
	if ( status )
	    SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
	/*
	**  If this table contains blob columns, make sure we,
	**  at the very least, allocate enough space for this
	**  record. This way, when we re-compress this record
	**  later on after inserting the coupon into the record,
	**  we will have enough space on the page to insert the
	**  compressed record.
	**  Only worry about this if the compression in use compresses
	**  coupons.  Otherwise, the coupon space doesn't change and
	**  it isn't an issue.
	*/
	else if ( t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS &&
		  t->tcb_data_rac.compresses_coupons )
	    rec_len = t->tcb_rel.relwid + t->tcb_data_rac.worstcase_expansion;
    }

    /*
    ** If row locking get value locks if we have a key.
    */
    if ( status == E_DB_OK && t->tcb_rel.relkeys 
	    && (row_locking(r) || crow_locking(r)) )
    {
	status = dm1r_lock_value(r, (i4)DM1R_LK_PHYSICAL, record, dberr);
	if (status == E_DB_OK)
	    value_lock = TRUE;
    }

    /*	Allocate space for the new record. */
    tid.tid_tid.tid_line = 0;
    tid.tid_tid.tid_page = 0;

    if ( status == E_DB_OK )
	switch (t->tcb_table_type)
    {
    case TCB_HEAP:
	status = dm1s_allocate(r, &r->rcb_data, &tid, record, crec,
		rec_len, DM1C_DUPLICATES, dberr);
	break;

    case TCB_HASH:
	status = dm1h_allocate(r, &r->rcb_data, &tid, record, crec,
		rec_len, opflag, dberr);
	break;

    case TCB_ISAM:
	status = dm1i_allocate(r, &r->rcb_data, &tid, record, crec,
		rec_len, opflag, dberr);
	break;

    case TCB_BTREE:
	status = dm1b_allocate(r, &r->rcb_other,
		&r->rcb_data, &bid, &tid, record, crec, rec_len,
		r->rcb_s_key_ptr, opflag, dberr);
	break;

    case TCB_RTREE:
	status = E_DB_ERROR; /* primary rtree indexes not supported */
	break;

    default:;
    }
 
    if (status == E_DB_OK && (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
			&& (flag & DM2R_REDO_SF) == 0)
    {
	/*
	**  If there are extensions, then we need to move any
	**  extension tuples.  This is all managed by the DMPE
	**  routines.
	**  Note that this has to happen against the original record as
	**  passed in;  if the caller re-uses the row for more (such as
	**  QEF passing it to a rule-fired DBproc), it needs to see real
	**  coupons, not old or temp coupons.  See also the pget call below.
	*/
	status = dm1c_pput(t->tcb_atts_ptr,
			    r,
			    record,
			    dberr);
	if ( status && dberr->err_code != E_DM0065_USER_INTR
	  && dberr->err_code != E_DM010C_TRAN_ABORTED 
          && dberr->err_code != E_DM016B_LOCK_INTR_FA
          && dberr->err_code != E_DM5423_SRID_MISMATCH)
	{
	    /* 
	    ** User interrupts are now a possibility, since dmpe
	    ** updates may make recursive DMF calls to mung with
	    ** objects in the extension tables.  Given that that's
	    ** the case, we just pass the error back up the chain,
	    ** and the upper layers will handle it correctly.
	    ** Ditto with force-abort.
	    */
		
	    uleFormat( dberr, 0,
			(CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL,
			&loc_err, 0);
	    SETDBERR(dberr, 0, E_DM9B02_PUT_DMPE_ERROR);
	}

	/*
	**  If there are encrypted columns, re-encrypt the record. Then,
	**  if compression is turned on, re-compress the record, now
	**  that the actual coupon has been put into the record. This
	**  will also get the right size of the newly compressed
	**  record.
	**
	** FIXME This re-encrypting and re-compressing is ugly. But it
	** works, and I dare not make a bigger change on the cusp of
	** a code freeze. Added this reminder comment that this bears
	** revisiting when we are in here again. (MikeT - 30 Aug 2010)
	*/

	if ( status == E_DB_OK && t->tcb_rel.relencflags & TCB_ENCRYPTED )
	{
	    status = dm1e_aes_encrypt(r, &t->tcb_data_rac, record,
			r->rcb_erecord_ptr, dberr);
	    rec = r->rcb_erecord_ptr;
	}

	if ( status == E_DB_OK && t->tcb_data_rac.compression_type != TCB_C_NONE )
	{
	    rec = crec = r->rcb_record_ptr;
	    status = (*t->tcb_data_rac.dmpp_compress)(&t->tcb_data_rac,
		(t->tcb_rel.relencflags & TCB_ENCRYPTED) ?
		r->rcb_erecord_ptr : record,
		(i4)t->tcb_rel.relwid, rec, &rec_len);
	    if ( status )
		SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
	}
    }

    /*	Now make the change. */
    if ( status == E_DB_OK )
	switch (t->tcb_table_type)
    {
    case TCB_HEAP:
    case TCB_HASH:
    case TCB_ISAM:
	status = dm1r_put(r, &r->rcb_data, &tid, rec, (char *)NULL, 
		rec_len, (i4)DM1C_MPUT, dberr);
	break;

    case TCB_BTREE:
	if (t->tcb_extended && r->rcb_bulk_misc && r->rcb_bulk_cnt > 1)
	{
	    status = dm1b_bulk_put(r, &r->rcb_other, 
		&r->rcb_data,
		&bid, &tid, rec, r->rcb_s_key_ptr, rec_len,
		(i4)DM1C_MPUT, dberr);
	}
	else
	{
	    status = dm1b_put(r, &r->rcb_other, &r->rcb_data,
			&bid, &tid, rec, r->rcb_s_key_ptr, rec_len,
			(i4)DM1C_MPUT, dberr);
	}
	break;

    case TCB_RTREE:
	status = dm1m_put(r, &r->rcb_other, &r->rcb_data,
			&bid, &tid, rec, r->rcb_s_key_ptr, rec_len, dberr);
	break;

    case TCB_TAB_GATEWAY:
	/*
	** Call gateway to append record.
	*/
	status = dmf_gwr_put(r, rec, rec_len, &tid, dberr);
	break;
    }

    if ( status == E_DB_OK )
    {
	/* Set tid so it can be returned to caller. */

	r->rcb_currenttid.tid_i4 = tid.tid_i4;

	/*
	** Now si_put's can proceed.
	** Set base's TID for si_put() 
	*/
	/* Wake up all waiting for the TID to appear */
	if ( r->rcb_siAgents )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    r->rcb_si_newtid.tid_i4 = tid.tid_i4;
	    if ( rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
	else
	    r->rcb_si_newtid.tid_i4 = tid.tid_i4;

	r->rcb_state |= (RCB_TABLE_UPDATED | RCB_DPAGE_UPDATED);
	if (t->tcb_table_type == TCB_BTREE)
	    r->rcb_state |= RCB_LPAGE_UPDATED;

	/*
	** check wether we need to enable replication (this is done so that
	** a session in repmgr can setup replication and have it enabled without
	** having to disconnect and re-connect). We will enable replication
	** if the user is about to insert a local database record in dd_databases
	** and all other replicator catalogs are present, but replication is
	** not yet enabled. But don't enable if II_DBMS_REP is set.
	*/
	if ((dcb->dcb_status & DCB_S_REPCATS) && 
	    (dmf_svcb->svcb_status & SVCB_NO_REP) == 0 &&
	    ((dcb->dcb_status & DCB_S_REPLICATE) == 0) &&
	    (MEcmp(t->tcb_rel.relid.db_tab_name, 
		(rcb->rcb_xcb_ptr && rcb->rcb_xcb_ptr->xcb_scb_ptr &&
		  rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &&
		  (*rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U)) ?
		DD_DATABASES_U : DD_DATABASES, DD_DATABASES_SIZE) == 0) &&
		(STskipblank(t->tcb_rel.relid.db_tab_name + DD_DATABASES_SIZE, 
		DB_MAXNAME - DD_DATABASES_SIZE)) == NULL)
	{
	    i2	is_local;

	    /* get offset of local_db */
	    for (i = 1; i <= t->tcb_rel.relatts; i++)
	    {
		if (STcompare(t->tcb_atts_ptr[i].attnmstr,
		    (rcb->rcb_xcb_ptr && rcb->rcb_xcb_ptr->xcb_scb_ptr &&
		    rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate &&
		    (*rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U)) ?
		    "LOCAL_DB" : "local_db") == 0)
			break;
	    }
	    if (i == t->tcb_rel.relatts)
	    {
		/* no local_db field */
		status = E_DB_ERROR;
		uleFormat(dberr, E_DM9550_NO_LOCAL_DB, (CL_ERR_DESC *)NULL, 
		    ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, (i4)0);
	    }
	    else
	    {
		MEcopy(record + t->tcb_atts_ptr[i].offset, sizeof(i2), &is_local);
		if (is_local) /* then we need to enable replication */
		{
		    MEcopy(record, sizeof(dcb->dcb_rep_db_no), &dcb->dcb_rep_db_no);
		    dcb->dcb_status |= DCB_S_REPLICATE;
		}
	    }
	}
    }

    /*
    ** If this is a core system catalog (must be iirelation, since it's the
    ** only one which has a secondary index), then unfix its pages here. This
    ** ensures that we comply with the cluster node failure recovery rule that
    ** a transaction may only fix one core system catalog page at a time.
    */
    if ( status == E_DB_OK && t->tcb_rel.relstat & TCB_CONCUR &&
	 t->tcb_update_idx )
    {
	status = dm2r_unfix_pages(r, dberr);
    }

    /*
    ** Release the value lock (after the put)
    ** Previously we were releasing after the allocate, before the put
    ** causing btree errors because dm1b_dupcheck DM1B_SKIP_DELETED_KEY_MACRO
    ** was true for another transaction trying to insert the same key
    ** (t1-alloc, t2-alloc-uses-t1-reserved-leaf-entry, t2-put, t1-put-error)
    ** Since the btree reserved key entry doesn't have a valid tid to 
    ** waitlock on, hold the value lock until after the put.
    */
    if ( value_lock ) 
    {
	local_status = dm1r_unlock_value(r, record, &local_dberr);
	if (local_status)
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &loc_err, 0);
    }

    if (status != E_DB_OK)
    {
	if (r->rcb_data.page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data,
				    &local_dberr);
	    if (status)
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &loc_err, 0);
	}
	if (r->rcb_other.page)
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_other,
				    &local_dberr);
	    if (status)
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &loc_err, 0);
	}
	status = E_DB_ERROR;
    }

    /*	Check for secondary index updates. */

    /*
    ** Check tcb_update_idx rather than relstat & TCB_IDXD which
    ** may not be set for Partitions.
    */

    /* Reconstitute short-term part of coupons in case QEF wants the
    ** record for more "stuff" such as rule firing.
    ** This is moved to happen before the index stuff below.  For spatial
    ** rtree indexes the data are LOBs.  Therefore all coupon stuff needs to
    ** happen before any indexing happens.
    */
    if (status == E_DB_OK && t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	status = dm1c_pget(t->tcb_atts_ptr, r, record, dberr);

    if ( status == E_DB_OK && t->tcb_update_idx && !r->rcb_siAgents )
    {
	i4 cancel_ix;	/* ordinal position of index in progress */

#ifdef NEVER_USED
	/* in order to speed sec ind page allocation by having readahead  */
	/* threads do some ixs concurrently, if readahead is available,   */
	/* and there are enough indexes to make it worthwhile, schedule   */
	/* the request. RA thread will do ixs from end of queue (prev)    */
	if ((r->rcb_state & RCB_PREFET_AVAILABLE)  &&
	    (r->rcb_tcb_ptr->tcb_index_count > PFR_PREALLOC_MIN))
	{
	    MEcopy(record, r->rcb_tcb_ptr->tcb_rel.relwid,
		r->rcb_srecord_ptr);
	    dm2r_schedule_prefetch(r, PFR_PREALLOC_SECIDX);
	}
#endif

	/* Get indexes from partition master */

	for ( it = t->tcb_pmast_tcb->tcb_iq_next, cancel_ix = 1;
	      it != (DMP_TCB*)&t->tcb_pmast_tcb->tcb_iq_next;
	      it = it->tcb_q_next, cancel_ix++)
	{
#ifdef NEVER_USED
    	    /* Cancel outstanding prefetch readahead requests for this rcb  */
	    /* that refer to this index in case RA thread is falling behind.*/
	    if ((r->rcb_state & RCB_PREFET_PENDING) &&
		cancel_ix > PFR_PREALLOC_MIN)
	    	dm2r_cancel_prefetch(r, cancel_ix);
#endif

	    if ( status = si_put(it, r, record, flag | DM2R_MPUT, 
					dberr) )
	    {
		if (dberr->err_code >= E_DM_INTERNAL)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
		}
#ifdef NEVER_USED
		/* Cancel all outstanding prefetch readahead requests for rcb  */
		if (r->rcb_state & RCB_PREFET_PENDING)
		    dm2r_cancel_prefetch(r, PFR_CANCEL_IX);
#endif
		break;
	    }
	}
    }

    /*
    ** check for replication and perform data capture in necesary
    */
    if ( status == E_DB_OK && dcb->dcb_status & DCB_S_REPLICATE &&
	 t->tcb_replicate == TCB_REPLICATE )
    {
        /*
        ** check if this session has the input queue open, if not open it
        */
        if (r->rcb_xcb_ptr->xcb_rep_input_q == NULL ||
	    r->rcb_xcb_ptr->xcb_rep_input_q == (DMP_RCB *)-1)
        {
            i4     rep_iq_lock = 0;
	    i4	rep_iq_maxlocks = 0;

	    /*
	    ** check for locking startegy, "set lockmode" for the input
	    ** queue over-rides all other settings, otherwhise we use PM
	    ** variables. If they are not set the initial user base-table
	    ** update determins the locking strategy
	    */
	    if (rcb->rcb_xcb_ptr->xcb_scb_ptr && 
		rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next)
	    {
		DML_SLCB    *slcb = rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next;
		for (;;)
    		{
        	    if (slcb == (DML_SLCB*) 
			&rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next || 
			slcb == 0)
            		break;
        	    if ( slcb->slcb_db_tab_base == 
			    dcb->rep_input_q.db_tab_base )
        	    {
            		if (slcb->slcb_lock_level == DMC_C_ROW)
                	    rep_iq_lock = DM2T_RIX;
            		else if (slcb->slcb_lock_level == DMC_C_PAGE)
                	    rep_iq_lock = DM2T_IX;
            		else if (slcb->slcb_lock_level == DMC_C_TABLE)
                	    rep_iq_lock = DM2T_X;
			if (slcb->slcb_maxlocks)
			    rep_iq_maxlocks = slcb->slcb_maxlocks;
			break;
		    }
		    slcb = slcb->slcb_q_next;
		}
	    }
	    if (rep_iq_lock == 0) 
	    {
		/* 
		** not explicitly set to row, page or table with 
		** "set lockmode" check PM
		*/
		if (dmf_svcb->svcb_rep_iqlock == DMC_C_ROW)
		    rep_iq_lock = DM2T_RIX;
		else if (dmf_svcb->svcb_rep_iqlock == DMC_C_PAGE)
		    rep_iq_lock = DM2T_IX;
		else if (dmf_svcb->svcb_rep_iqlock == DMC_C_TABLE)
		    rep_iq_lock = DM2T_X;
		else if (r->rcb_k_type == RCB_K_ROW)
		    /* no values set, use base table values */
		    rep_iq_lock = DM2T_RIX;
		else
		    rep_iq_lock = DM2T_IX;
	    }
	    if (rep_iq_maxlocks == 0)
		/* not set in "set lockmode" use base table value */
		rep_iq_maxlocks = r->rcb_lk_limit;

            status = dm2t_open(dcb, &dcb->rep_input_q, rep_iq_lock,
                DM2T_UDIRECT, DM2T_A_WRITE, r->rcb_timeout,
		r->rcb_lk_limit, r->rcb_sp_id, r->rcb_log_id, r->rcb_lk_id,
		r->rcb_seq_number, (i4)0, r->rcb_lk_mode,
                &r->rcb_tran_id, &local_timestamp,
                &r->rcb_xcb_ptr->xcb_rep_input_q, (DML_SCB *)0, dberr);

            if ( status == E_DB_OK )
	    {
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_xcb_ptr = r->rcb_xcb_ptr;
		/* set logging and journalling from base RCB */
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_logging = r->rcb_logging;
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_journal = 
			    is_journal(r->rcb_xcb_ptr->xcb_rep_input_q);
		/* Do not allow non-journalled tables to be replicated within
		** journalled databases since the internal tables are 
		** replicated.  (kibro01) b120490
		*/
		if (!r->rcb_journal && (dcb->dcb_status & DCB_S_JOURNAL))
		{
		    i4 local_err_code;
		    uleFormat(NULL, E_DM9582_REP_NON_JNL, (CL_ERR_DESC *)NULL,
			   ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			   &local_err_code, 1,
			   DB_TAB_MAXNAME, t->tcb_rel.relid.db_tab_name);
		    SETDBERR(dberr, 0, E_DM008B_ERROR_PUTTING_RECORD);
		    status = E_DB_ERROR;
		}
	    }
        }

	if ( status == E_DB_OK &&
	   (DMZ_SES_MACRO(32) == 0 || dmd_reptrace() == FALSE) )
	{
	    /* only if we are not the replication server */
	    status = dm2rep_capture(DM2REP_INSERT, r, record, dberr);
	}
    }
    else if ( status == E_DB_OK &&
             (dcb->dcb_status & DCB_S_REPLICATE) && r->rcb_xcb_ptr &&
	     r->rcb_xcb_ptr->xcb_rep_input_q == 0 &&
	     (MEcmp(t->tcb_rel.relid.db_tab_name, 
	     (*r->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) ? 
		DD_INPUT_QUEUE_U : DD_INPUT_QUEUE, 
		DD_INPUT_QUEUE_SIZE) == 0) &&
	    (STskipblank(t->tcb_rel.relid.db_tab_name + DD_INPUT_QUEUE_SIZE, 
		DB_MAXNAME - DD_INPUT_QUEUE_SIZE)) == NULL)
    {
	/*
	** user is updating the input queue, set up input_q value
	** in the XCB so that the distribution threads are activated to
	** distribute the values. We use the value -1 so that we know it
	** was a user update and not an internal DBMS update
	*/
	r->rcb_xcb_ptr->xcb_rep_input_q = (DMP_RCB *)-1;
    }
    /*
    ** if this is the replication server (dm32) and we are adding
    ** a record to the input queue, then store the remote TX id
    */
    if ( status == E_DB_OK &&
        (dcb->dcb_status & DCB_S_REPLICATE) && r->rcb_xcb_ptr &&
	r->rcb_xcb_ptr->xcb_rep_remote_tx == 0 &&
	DMZ_SES_MACRO(32) && dmd_reptrace() &&
	(MEcmp(r->rcb_tcb_ptr->tcb_rel.relid.db_tab_name,
	    (*r->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) ? 
	    DD_INPUT_QUEUE_U : DD_INPUT_QUEUE, DD_INPUT_QUEUE_SIZE) == 0) &&
	(STskipblank(t->tcb_rel.relid.db_tab_name + DD_INPUT_QUEUE_SIZE, 
	    sizeof(DB_TAB_NAME) - DD_INPUT_QUEUE_SIZE)) == NULL)
    {
	MEcopy(record + 2, sizeof(r->rcb_xcb_ptr->xcb_rep_remote_tx), 
			(char *)&r->rcb_xcb_ptr->xcb_rep_remote_tx);
    }

    return(SIcomplete(r, status, dberr));
} /* dm2r_put() */

/*{
** Name: dm2r_si_put	- Put a record's secondary index (threaded).
**
** Description:
**	
**      This function adds a record to an index.
**      One secondary indicex is updated.  If the record is
**      a duplicate or the key is a duplicate the operation 
**	will be rejected.
**	It is called from dmrAgent by a SI agent.
**
** Inputs:
**      ir                              The index's RCB.
**      record                          Pointer to record being added.
**
** Outputs:
**      err_code                        The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL			Operation was partially completed,
**					the transaction must be aborted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-Nov-2003 (jenjo02)
**	    Created for Parallel/Partitioning project.
*/
DB_STATUS
dm2r_si_put(
DMP_RCB		*ir,
char		*record,
DB_ERROR	*dberr)
{
    DMP_RCB	*r = ir->rcb_base_rcb;
    DMR_RTB	*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB	*it = ir->rcb_tcb_ptr;
    DMP_DCB	*d = it->tcb_dcb_ptr;
    DB_STATUS	status = E_DB_OK;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    CSp_semaphore(1, &rtb->rtb_cond_sem);

    /* Must wait until the Base RCB is initialized */
    while ( rtb->rtb_status == E_DB_OK &&
	    r->rcb_si_flags & RCB_SI_BUSY )
    {
	rtb->rtb_cond_waiters++;
	CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	rtb->rtb_cond_waiters--;
    }
     
    /*
    ** If this is a unique index and it's slated for
    ** pre-put duplicate checking, do it now.
    */
    if ( rtb->rtb_status == E_DB_OK &&
         it->tcb_rel.relstat & TCB_UNIQUE &&
	 r->rcb_si_flags & RCB_SI_DUP_PRECHECK )
    {
	CSv_semaphore(&rtb->rtb_cond_sem);
	/* Note this decrements rtb_si_dupchecks */
	status = si_dupcheck(it, ir, record, 
			     rtb->rtb_dm2r_flags | 
				(DM2R_SI_AGENT | DM2R_MPUT),
			     dberr);
	CSp_semaphore(1, &rtb->rtb_cond_sem);
    }
    
    /*
    ** Must wait for all other SI dupchecks to complete and
    ** for the Base row's (new) TID to materialize.
    */
    while ( rtb->rtb_status == E_DB_OK &&
	   (r->rcb_si_newtid.tid_i4 == NOT_ALLOCATED ||
	    rtb->rtb_si_dupchecks) )
    {
	rtb->rtb_cond_waiters++;
	CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	rtb->rtb_cond_waiters--;
    }

    /* The real "put" */
    if ( rtb->rtb_status == E_DB_OK )
    {
	CSv_semaphore(&rtb->rtb_cond_sem);
	status = si_put(it, ir, record,
			 rtb->rtb_dm2r_flags | 
			     (DM2R_SI_AGENT | DM2R_MPUT),
			 dberr);
    }
    else
	CSv_semaphore(&rtb->rtb_cond_sem);

    /* Unfix any SI pages we may have left fixed */
    if ( dm2r_unfix_pages(ir, &local_dberr) && status == E_DB_OK )
    {
	status = E_DB_ERROR;
	*dberr = local_dberr;
    }

    /* Return status */
    return(status);
}


/*{
** Name: BuildRtreeRecord - build an R-tree secondary index record
**
** Description:
** 	Build a record for an R-tree secondary index from the attributes
** 	of the base table.
**
** Inputs:
** 	t				Table Control Block pointer for the
** 					  base table.
** 	it				TCB ptr for the secondary index.
** 	ir				RCB ptr for the secondary index.
**	brec				Pointer to the base table record.
**
** Outputs:
** 	rec				Pointer to the record to be built.
**      err_code                        The reason for a error status return.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	   none
**
** Side Effects:
**	    none
**
** History:
** 	06-jun-1996 (wonst02)
** 	    New.
**	10-Jun-1997 (shero03)
**	    Use Rangesize from the tcb instead of calculating it.
**
*/
DB_STATUS
BuildRtreeRecord(
    DMP_TCB		*t, 	/* tcb pointer for base table */
    DMP_TCB		*it,	/* tcb pointer for secondary index */
    DMP_RCB             *ir,	/* rcb pointer for secondary index */
    char		*brec,	/* pointer to base table record */
    char		*rec)	/* pointer to output record */
{
    ADF_CB		*adf_cb = ir->rcb_adf_cb;
    DB_DATA_VALUE	obj;
    DB_DATA_VALUE	range;
    DB_DATA_VALUE	nbr;
    DB_DATA_VALUE	hilbert;
    DB_STATUS		status;

    /*
    ** Fill in the DATA_VALUE fields
    **
    ** Note that the order is:
    ** NBR | HILBERT | TIDP
    */

    obj.db_data = &brec[t->tcb_atts_ptr[it->tcb_ikey_map[0]].offset];
    obj.db_length = t->tcb_atts_ptr[it->tcb_ikey_map[0]].length;
    obj.db_datatype = abs(t->tcb_atts_ptr[it->tcb_ikey_map[0]].type);
    obj.db_prec = 0;
    obj.db_collID = t->tcb_atts_ptr[it->tcb_ikey_map[0]].collID;
    range.db_data = (char *)it->tcb_range;
    range.db_length = it->tcb_rangesize;
    range.db_datatype = it->tcb_acc_klv->box_dt_id;
    range.db_prec = 0;
    range.db_collID = -1;
    nbr.db_data = (char *)&rec[it->tcb_atts_ptr[1].offset];
    nbr.db_length = it->tcb_atts_ptr[1].length;
    nbr.db_datatype = it->tcb_acc_klv->nbr_dt_id;
    nbr.db_prec = 0;
    nbr.db_collID = it->tcb_atts_ptr[1].collID;
    hilbert.db_data = (char *)&rec[it->tcb_atts_ptr[2].offset];
    hilbert.db_length = it->tcb_atts_ptr[2].length;
    hilbert.db_datatype = DB_BYTE_TYPE;
    hilbert.db_prec = 0;
    hilbert.db_collID = -1;

    /* Create the NBR */
    status = (*it->tcb_acc_klv->dmpp_nbr)(adf_cb, &obj, &range, &nbr);
    if (status != E_DB_OK)
        return(status);

    /* Create the HILBERT */
    status = (*it->tcb_acc_klv->dmpp_hilbert)(adf_cb, &nbr, &hilbert);

    return(status);
} /* BuildRtreeRecord */


/*{
** Name: dm2r_replace	- Replace a record.
**
** Description:
**      This function replaces the current record in a scan or a record
**      by tuple identifier.  The replace is rejected if a duplicate key or
**      a duplicate record is detected.  All secondary that need updating
**      are automatically updated.  The record stream must be positioned
**	for a replace current to work.  A replace by TID will de-position
**	the record stream.
**      This routine handles tracking of replaces and
**	deletes when multiple RCB's exist on a table
**	within a transaction.
**
** Inputs:
**      rcb                             The RCB to replace a record on.
**	tid				If flag & DM2R_BYTID then this is the
**					tuple identifier for the record to
**					replace. If flag & DM2R_RETURNTID the
**					new TID is returned in this field.
**	flag				Either DM2R_BYTID or DM2R_BYPOSITION
**                                      or DM2R_DUPLICATES OR DM2R_NODUPLICATES.
**					If DM2R_RETURNTID, the new tid is
**					returned in tid.
**	record				The new value of the record.
**
** Outputs:
**      err_code                        The reason for a error status return.
**
**	tid				TID after replace operation if tid !=
**					NULL.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	   none
**
** Side Effects:
**	    none
**
** History:
**	15-jan-1985 (derek)
**          Created new for jupiter.
**      26_feb_87 (jennifer)
**          Added code for unique secondary indices.
**	31-Mar-1987 (ac)
**	    Added read/nolock support.
**	15-Jun-88 (rogerk)
**	    Log logical replaces to base and secondary indexes if using
**	    fast commit.
**	    When log replace, write log record address to both old and new
**	    data pages.
**	 3-may-89 (rogerk)
**	    Build secondary index key rather than tuple to search for
**	    secondary index row to replace.
**	    Don't return NONEXT when fail to update secondary index.
**	12-may-89 (anton)
**	    local collation support - fill in adf_collation
**	29-may-89 (rogerk)
**	    Put in checks for errors from dm1c_comp_rec.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Call gateway to replace records
**	    in gateway tables.  Use rcb_logging/tcb_logging fields to determine
**	    appropriate logging actions.  Check if tid access is appropriate if
**	    BYTID specified.
**       4-sep-89 (rogerk)
**          Put in trace flag to bypass SBI logging.
**	25-sep-89 (paul)
**	    Added support for dm2r_replace to return the new tid of
**	    the record after a replace in support of bug 8054.
**	 5-dec-1989 (rogerk)
**	    Put back in the BI logging of BTREE leaf pages.  These are needed
**	    for UNDO recovery by the online-backup code during processing
**	    of the DUMP file information.
**	11-jan-1989 (rogerk)
**	    Only do BI logging of BTREE leaf pages if we are doing online
**	    backup.  The rcb_bt_bi_logging is enabled if online backup is
**	    going on, and the dm0l_bi() routine is only called if this flag
**	    is enabled.
**	28-apr-1990 (rogerk)
**	    Fix btree secondary index update bug.  The dm1b_allocate call
**	    for the 2nd index update may cause the row being replaced to
**	    be moved off of the page currently fixed.  Check for this case
**	    and fix the correct page for the dm1b_replace call.
**      12-nov-1990 (bryanp)
**          Don't check for duplicate 2-ary index records if the data changed
**          in representation only (as opposed to a true value change) B33703.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-90 (rogerk)
**		Changed to use the rcb's adf control block rather than
**		allocating local adf cb's off of the stack.  This was done as
**		part of the DBMS timezone changes.
**	    17-dec-1990 (jas)
**		Smart Disk project integration:
**		If we're turning off the RCB_POSITIONED bit, force
**		RCB_SDPOSITIONED off, as well.
**	     3-jan-91 (rogerk)
**		Make sure that pages fixed into local page pointers are unfixed
**		before leaving routine.  We were leaving leaf pages fixed after
**		calls to dm1b_allocate by assuming that btree routines would
**		unfix certain pages for us.  It seems much safer to check
**		before returning that required pages are unfixed.
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  Check rcb_logging status before
**		updating system catalogs and secondary indexes.  Use
**		rcb_logging rather than tcb_logging to direct logging actions
**		on 2nd indexes.
**	14-apr-1992 (rogerk)
**	    Pass base_oldtid value to ule_format using base_oldtid.tid_i4
**	    instead of passing the structure itself.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Replaced calls of dm1c_compress with accessors.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Took out calls to log put, delete and
**	    replace operations.  These are now logged from inside the page
**	    updating routines in DM1R.  Changed dm1r_replace calls to pass
**	    newdata pointer by value instead of by reference - the page is
**	    now never unfixed by dm1r_replace and must be unfixed by this
**	    routine.
**	16-feb-1993 (rogerk)
**	    Fix problem in replace of btree with a secondary index.  The
**	    'in_place' flag should not be used on the secondary index update
**	    since:  A) it describes the update action on the base table, not
**	    the secondary; and B) secondary indexes have no in_place actions
**          as there is always an index update required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix core catalog data page as soon as base table REPLACE is
**		    complete. This improves concurrency, but more importantly
**		    it satisfies the only-fix-one-core-catalog protocol for
**		    node failure recovery.
**      06-may-93 (anitap)
**          If there was no change in the row value for the update statement,
**          give error message to QEF so that QEF will not update the rowcount
**          if "SET UPDATE_ROWCOUNT TO CHANGED."
**	18-oct-1993 (bryanp) Bug #B49920
**	    Change "inplace_replace" argument to dm1b_replace from
**		(action == IN_PLACE) to ((action & ALLOCATE) == 0) to repair
**		btree replaces which didn't change the key but required dup
**		checking -- they were erroneously claiming to be non-in-place
**		replaces.
**	28-mar-1994 (rogerk)
**	    Removed unfixing of pages fixed in the RCB fields on errors,
**	    leaving that cleanup until the rcb is closed.  Some errors
**	    (like uniqueness violations) are not fatal to the query and
**	    the table is expected to remain positioned and accessable.
**	21-apr-1994 (rogerk)
**	    Added test trace point (DM654) to bypass the checks for no changes
**	    to the row and execute the replace anyway.
**	19-Apr-1995 (cohmi01)
**	    if 'checkeof' is on for a secondary index, call dm1p_checkhwm()
**	    to adjust eof so that group buffers may be used.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	20-may-96 (stephenb)
**	    Perform replication data capture for updates and open input
**	    queue if necesary.
**	 1-jul-96 (nick)
**	    Dup. checking being done twice for inplace replaces with
**	    unique secondaries.
**	19-jul-1996 (ramra01 for bryanp)
**	    Alter Table Project:
**	    Compute the sizes of the old and new records. The new record will
**	    always be relwid in size unless this table uses compression, in
**	    which case we must compress the row to determine the compressed
**	    length. The old record will be relwid in size for an uncompressed
**	    , unaltered table.
**	    For a table which has had ALTER TABLE ADD/DROP COLUMN operations
**	    performed, or which uses compression, we determine the old row
**	    length by callin the reclen accessor.
**	13-Aug-1996 (prida01)
**	    Give relatts correct i4 type for dm1c_preplace
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Moved common code to dm2r_fetch_record()
**          Check for dups in secondary index before allocate in base
**          Row locking: Acquire value lock for duplicate checking
**          Row locking: Execute replace as delete/put
**      17-dec-1996 (stial01)
**          dm2r_replace() If allocate caused split, we may need to 
**          re-fetch tuple to be deleted.
**	28-jan-97 (inkdo01)
**	    ADT_REPRESENTATION treated same as ADT_VALUE for primary indexes
**	    (already treated that way for secondaries). Important for hash 
**	    indexes, since even representation differences change hash 
**	    computation (fix for bugs 66329 and 74033).
**	11-Nov-96 (kitch01)
**	    Set collation sequence for secondary index updates.
**	10-Jan-97 (kitch01)
**	    Remove previous change. I have integrated a better fix from
**	    bug 43950 from the 6.4 code.
**	10-July-1997(angusm)
**	    Add handling for cross-table update ambiguous replace
**	    (bug 79623).
**	24-july-1997(angusm)
**	    Wrong #, type of parameters passed to dmpp_dput()
**	11-August-1997(angusm)
**	    Pass r->rcb_currenttid instead of r->rcb_fetchtid. Leads to trouble
**	    with btrees: get bogus ambiguous replace error (bug 84313).
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**          Get value lock if duplicate checking or non in place replace.
**      29-may-97 (stial01)
**          dm2r_replace: If unique SI, always lock old value (value_set = 0)
**	3-jun-97 (stephenb)
**	    if the user is updating the replicator input queue and 
**	    replication is active, then set the input queue value in the
**	    transaction control block so that the input queue values will
**	    be distributed on commit.
**      29-jul-97 (stial01)
**          dm2r_replace() Pass opflag DM1C_REPLACE to dm1*, si_put/si_delete
**          Space reclamation for leaf entries depends on page size not lockmod
**	25-sep-97 (stephenb)
**	    move replication code so that it is done before we update the
**	    the peripheral datatypes. Peripheral updates make changes to the
**	    coupon in the base (old) record hanging from the RCB directly, 
**	    rather than overlaying the new record at a later stage (nasty hack!)
**	    this means that we can't get to the old values for replication 
**	    archives.
**      05-oct-1998 (stial01)
**          dm2r_replace() Call dmpp_dput with new update parameter
**	22-oct-98 (stephenb)
**	    Make sure logging and journalling characteristics of the replication
**	    input queue RCB match those of the base table.
**	06-oct-1998 (nanpr01)
**	    Update optimization project.
**	03-nov-1998 (mcgem01)
**	    Fix compilation error caused by renaming the dcb to d without
**	    updating the parameter list in the call to dm2t_open.
**	03-dec-1998 (nanpr01)
**	    We will not use the update optimization for altered table
**	    because opc sets the attribute array as if there is no deleted
**	    column but the att list does have those columns which makes
**	    it tough to figure things out.
**	17-feb-1999 (somsa01)
**	    If it looks like nothing has changed, and we are dealing with
**	    peripheral columns, then check lobj_changed. If peripheral
**	    tuples were in fact changed (which may not be obvious by
**	    examining just the coupon), set type_nkey_change to ADT_VALUE.
**	    Remember that peripheral columns cannot be part of a key.
**	7-may-1999 (nanpr01)
**	    Initialize only the portions of array that is going to be used.
**      05-may-1999 (nanpr01)
**          dm2r_replace() Lock/unlock NEW key value to prevent: 
**          Insert of key that a serializable transaction has positioned on.
**          Concurrent dup checking for same key by different transactions 
**          Lock/unlock OLD key value to prevent delete of: 
**          - Unique key that a serializable trans tried to insert & failed.
**          Defer SI dupcheck until after base table update if possible
**	21-Apr-1999 (shust01)
**	    Set fetch_tid.tid_i4 to be rcb_currenttid (current tid) instead
**	    of rcb_fetchtid (current bid) in dm2r_replace() for special case 
**	    of old row and new (replaced) row being identical.  The new tid
**	    (not bid) should be returned.
**      12-Jul-1999 (hanal04)
**          Added detection and handling of E_AD101B_BAD_FLT_VALUE when
**          returned from adt_compute_change(). b85041.
**      22-Jul-1999 (hanal04)
**          Adjust above change to report errors from DMF rather than
**          ADF to fix build problems. b98034.
**	17-dec-99 (gupsh01)
**	    Added additional check to give out correct error message in case of 
**	    updating a readonly database. 
**      1-Jul-2000 (wanfr01)
**          Bug 101521, INGSRV 1226 - If using RAAT, and II_RAAT_RETRY is sent,
**          reattempt to acquire the lock.  If there is a timeout, return the
**          new error code E_DM0173_RAAT_SI_TIMER.   This reduces the chance
**          of corrupting the secondary index.
**	09-Feb-2001 (jenjo02)
**	    dm2r_get() may not have placed a copy of the record to
**	    be deleted in rcb_record_ptr. If not, fetch it.
**	18-Jan-2002 (inifa01) Bug 106823 INGSRV 1659
**	    when updating data on a table which has a compressed secondary index
**	    the query fails with Error trying to replace a record. (E_QE007)
**	    Case occurs with hash/heap/isam compressed sec index.
**	    Reset delta_start to zero if table is compressed.  
**      22-nov-2004 (huazh01)
**          Initialize the entire 'value_set[]' and 'change_set[]'. 
**          This fixes b113489, INGSRV3051.  
**	22-Jan-2004 (jenjo02)
**	    Added support for parallel SI update threads.
**	26-Jan-2004 (jenjo02)
**	    Get SI list from master's TCB.
**      Mar-18-2005 (hweho01)
**          Cross-integrated change #474714 by inifa01 from ingres26 
**          codeline:
**      06-Jan-2005 (inifa01)  b113796/INGREP171
**          The DBMS server is performing update replication for vertically
**	    partitioned tables where none of the participating columns have
**	    been updated.
**          FIX: check that the attset bit map of columns being updated
**	    includes a replicated column before triggering replication.
**      07-apr-2005 (huazh01)
**          if table is to be replicated to another database, then use 
**          RCB of dd_input_queue to decide whether or not journaling 
**          is needed. 
**          This fixes b114215, INGSRV3229. 
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
**	12-Sep-2005 (schka24)
**	    dm0p-mutex call change got the flags in the wrong place!  fix.
**	07-May-2008 (kibro01) b120345
**	    Correct original XIP of b114215 which missed out the RCB change
**	    for updates to prevent dd_input_queue being journalled when
**	    it shouldn't be.
**	18-Aug-2008 (kibro01) b120490
**	    Do not allow replication on a non-journalled table within a
**	    journalled database.
**	26-Feb-2009 (kschendel) SIR 122890
**	    Don't assume in-place updates for core catalogs.  While key
**	    updates never happen normally, they would occur if a table ID
**	    reclaimer were written;  and there is no need to disobey a
**	    key update request if one happens.
**	28-Apr-2010 (jonj) SD 144272
**	    Use DMP_PINIT to fully initialize local pinfos, use
**	    direct references to pinfo.page.
**	11-Aug-2010 (miket) SIR 122403 SD 146244
**	    For encryption the difference in the encrypted and unencrypted
**	    buffer layouts and the fact that the external value can remain
**	    the same while the internal value changes can confuse the
**	    tuple change routine adt_compute_part_change, so use
**	    adt_compute_change instead.
*/
DB_STATUS
dm2r_replace(
DMP_RCB             *rcb,
DM_TID		    *tid,
i4		    flag,
char		    *record,
char   		    *attset,
DB_ERROR	    *dberr )
{
    DMP_RCB             *r = rcb;
    DMR_RTB		*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_DCB		*d = rcb->rcb_tcb_ptr->tcb_dcb_ptr;
    DMP_TCB		*it;
    DMP_RCB		*ir;
    DM_TID		oldbid;
    DM_TID		oldtid;
    DM_TID		newbid;
    DM_TID		newtid;
    DM_TID		fetched_tid;
    i4			oldlength;
    i4			newlength;
    char		*oldrecord;
    char		*newrecord;
    i4			type_key_change;
    i4			type_nkey_change;
    i4			action;
#define			IN_PLACE	0
#define			ALLOCATE	0x01
#define			DUP_CHECK	0x02
    i4			i;
    DB_STATUS		status, loc_status;
    ADF_CB		*adf_cb;
    i4			loc_err;
    i4			idxnum;  /* ordinal position of index in tcb queue */
    DB_TAB_TIMESTAMP	local_timestamp;
    i4             	del_reclaim_space = TRUE;
    i4			alloc_flag;
    i4			lobj_changed;
    i4			delta_start, delta_end;
    i4			rcb_si_flags;
    i4                  replicate_change = FALSE;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO		newleafPinfo, newdataPinfo;

    CLRDBERR(dberr);

    /* Fully init local pinfos */
    DMP_PINIT(&newleafPinfo);
    DMP_PINIT(&newdataPinfo);

#ifdef xDEBUG
    if (r->rcb_type != RCB_CB)
	dmd_check(E_DM9315_DM2R_REPLACE);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9315_DM2R_REPLACE);
#endif


    /* Base table replace: */
    status = E_DB_OK;

    for (;;) /* something to break from */
    {
	r->rcb_si_oldtid.tid_i4 = NOT_ALLOCATED;
	r->rcb_si_newtid.tid_i4 = NOT_ALLOCATED;
	r->rcb_compare = 0;

	rcb_si_flags = 0;

	r->rcb_s_rep++;
	dmf_svcb->svcb_stat.rec_replace++;

	/*	Check that the RCB is for write access. */

	if (r->rcb_access_mode != RCB_A_WRITE)
	{
	    if (r->rcb_xcb_ptr->xcb_x_type & XCB_RONLY)
		SETDBERR(dberr, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    else
		SETDBERR(dberr, 0, E_DM0073_RECORD_ACCESS_CONFLICT);
	    status = E_DB_ERROR;
	    break;
	}

	/* If duplicate found, will caller continue or rollback */
	if ( flag & DM2R_DUP_CONTINUE || 
	     t->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE ||
	     t->tcb_stmt_unique || DMZ_TBL_MACRO(20) )
	{
	    /* This applies to the base table... */
	    flag |= DM2R_DUP_CONTINUE;

	    /* Must also precheck unique indexes, if any */
	    if ( t->tcb_unique_index &&
		(t->tcb_rel.relstat & TCB_GATEWAY) == 0 )
	    {
		rcb_si_flags |= RCB_SI_DUP_PRECHECK;
		if ( r->rcb_siAgents )
		    rtb->rtb_si_dupchecks = t->tcb_unique_count;
	    }
	}

	if ( t->tcb_table_type == TCB_BTREE && flag & DM2R_BYTID )
	{
	    /*
	    ** BTREE: just get data page until we know if key change,
	    ** but only if we don't already have it.
	    */
	    if ( tid->tid_i4 != r->rcb_fetchtid.tid_i4 ||
		 tid->tid_i4 != r->rcb_currenttid.tid_i4 ||
		 (r->rcb_state & RCB_RECORD_PTR_VALID) == 0 ||
		  r->rcb_state & RCB_FETCH_REQ )
	    {
		status = dm1r_get(r, tid, r->rcb_record_ptr, DM1C_BYTID, dberr);
	    }
	       
	    flag |= DM2R_HAVE_DATA;
	}
	else
	{
	    /* If dm2r_get() didn't copy rcd to rcb_record_ptr, fetch it */
	    if ( (r->rcb_state & RCB_RECORD_PTR_VALID) == 0 )
	    {
		/* Must fetch record into rcb_record_ptr */
		r->rcb_state |= RCB_FETCH_REQ;
	    }

	    status = dm2r_fetch_record(r, tid, flag, DM1C_MREPLACE, dberr);
	}

	if ( status )
	    break;

	/*
	** Save the TID we actually fetched. In the case where we are asked
	** to return the new tid and the rows are identical causing the
	** update action to be a NO-OP. We will need this value to return
	** as the new TID.
	*/
	fetched_tid.tid_i4 = r->rcb_currenttid.tid_i4;

     /*
     ** For vertically partitioned replication where some of the columns in the
     ** table will be replicated and not all, check if the attset bit map of
     ** columns being updated includes a replicated column before triggering
     ** replication. B113796 INGREP171.
     */
     if ((d->dcb_status & DCB_S_REPLICATE) && (t->tcb_replicate == TCB_REPLICATE)
         && attset )
     {
         for (i = BTnext(-1, attset, r->rcb_tcb_ptr->tcb_rel.relatts +1); i != -1;
              i = BTnext(i, attset, r->rcb_tcb_ptr->tcb_rel.relatts +1))
         {
             if (t->tcb_atts_ptr[i].replicated)
             {
                 replicate_change = TRUE;
                 break;
             }
         }
     }


	/*
	**	Must now calculate the type of change made to the record.
	**  Different actions are taken based on the type of change
	**  made to the record.  The following table
	**	describes the type of changes made and the set of actions taken.
	**
	**	field_changed   length          type		duplicates  action
	**	-----------------------------------------------------------------------
	**
	**	key		same		value		no	    allocate+dup_check
	**	key		same		value		yes	    allocate
	**	key		same		representation	no|yes	    inplace
	**	key		different	value		no	    allocate+dup_check
	**	key		different	value		yes	    allocate
	**	key		different	representation	no|yes	    allocate
	**
	**	non_key		same		value		no	    in_place+dup_check
	**	non_key		same		value		yes	    inplace
	**	non_key		same		representation	no|yes	    inplace
	**	non_key		different	value		no	    allocate+dup_check
	**	non_key		different	value		yes	    allocate
	**	non_key		different	representation	no|yes	    allocate
	**
	**-------------------------------------------------------------------------------
	**
	**	key_unique	same		value		no	    allocate+dup_check
	**	key_unique	same		representation	no	    inplace
	**	key_unique	different	value		no	    allocate+dup_check
	**	key_unique	different	representation	no	    allocate
	**
	**	non_key_unique	same		value		no	    inplace
	**	non_key_unique	same		representation	no	    inplace
	**	non_key_unique	different	value		no	    allocate
	**	non_key_unique	different	represention	no	    allocate
	**
	**	The actions for the key fields are computed and the actions for
	**	the non-key fields are or'ed together to get the actual action required.
	*/
	/*
	** check for replication, this must be done before we operate on 
	** pripherals since we may change the coupon in the old record 
	** (when an update on the peripheral field is performed) which alters
	** the information we need for the archive record.
	*/
        if ((d->dcb_status & DCB_S_REPLICATE) && replicate_change &&
            (t->tcb_replicate == TCB_REPLICATE))
	{
	    /*
	    ** check if this session has the input queue open, if not open it
	    */
	    if (r->rcb_xcb_ptr->xcb_rep_input_q == NULL ||
		r->rcb_xcb_ptr->xcb_rep_input_q == (DMP_RCB *)-1)
	    {
		i4     rep_iq_lock = 0;
		i4	rep_iq_maxlocks = 0;

		/*
		** check for locking startegy, "set lockmode" for the input
		** queue over-rides all other settings, otherwhise we use PM
		** variables. If they are not set the initial user base-table
		** update determins the locking strategy
		*/
		if (rcb->rcb_xcb_ptr->xcb_scb_ptr && 
		    rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next)
		{
		    DML_SLCB    *slcb = rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next;
		    for (;;)
		    {
			if (slcb == (DML_SLCB*) 
			    &rcb->rcb_xcb_ptr->xcb_scb_ptr->scb_kq_next || 
			    slcb == 0)
			    break;
			if ( slcb->slcb_db_tab_base == 
			        d->rep_input_q.db_tab_base )
			{
			    if (slcb->slcb_lock_level == DMC_C_ROW)
				rep_iq_lock = DM2T_RIX;
			    else if (slcb->slcb_lock_level == DMC_C_PAGE)
				rep_iq_lock = DM2T_IX;
			    else if (slcb->slcb_lock_level == DMC_C_TABLE)
				rep_iq_lock = DM2T_X;
			    if (slcb->slcb_maxlocks)
				rep_iq_maxlocks = slcb->slcb_maxlocks;
			    break;
			}
			slcb = slcb->slcb_q_next;
		    }
		}
		if (rep_iq_lock == 0) 
		{
		    /* 
		    ** not explicitly set to row, page or table with 
		    ** "set lockmode" check PM
		    */
		    if (dmf_svcb->svcb_rep_iqlock == DMC_C_ROW)
			rep_iq_lock = DM2T_RIX;
		    else if (dmf_svcb->svcb_rep_iqlock == DMC_C_PAGE)
			rep_iq_lock = DM2T_IX;
		    else if (dmf_svcb->svcb_rep_iqlock == DMC_C_TABLE)
			rep_iq_lock = DM2T_X;
		    else if (r->rcb_k_type == RCB_K_ROW)
			/* no values set, use base table values */
			rep_iq_lock = DM2T_RIX;
		    else
			rep_iq_lock = DM2T_IX;
		}
		if (rep_iq_maxlocks == 0)
		    /* not set in "set lockmode" use base table value */
		    rep_iq_maxlocks = r->rcb_lk_limit;

		status = dm2t_open(d, &d->rep_input_q, rep_iq_lock,
		    DM2T_UDIRECT, DM2T_A_WRITE, r->rcb_timeout,
		    r->rcb_lk_limit, r->rcb_sp_id, r->rcb_log_id, r->rcb_lk_id,
		    r->rcb_seq_number, (i4)0, r->rcb_lk_mode,
		    &r->rcb_tran_id, &local_timestamp,
		    &r->rcb_xcb_ptr->xcb_rep_input_q, (DML_SCB *)0, dberr);
		if ( status )
		    break;
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_xcb_ptr = r->rcb_xcb_ptr;
		/* set logging and journalling from base RCB */
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_logging = r->rcb_logging;
		r->rcb_xcb_ptr->xcb_rep_input_q->rcb_journal =
			    is_journal(r->rcb_xcb_ptr->xcb_rep_input_q);
		/* Do not allow non-journalled tables to be replicated within
		** journalled databases since the internal tables are 
		** replicated.  (kibro01) b120490
		*/
		if (!r->rcb_journal && (d->dcb_status & DCB_S_JOURNAL))
		{
		    i4 local_err_code;
		    uleFormat(NULL, E_DM9582_REP_NON_JNL, (CL_ERR_DESC *)NULL,
			   ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			   &local_err_code, 1,
			   DB_TAB_MAXNAME, t->tcb_rel.relid.db_tab_name);
		    SETDBERR(dberr, 0, E_DM008C_ERROR_REPLACING_RECORD);
		    status = E_DB_ERROR;
		}
	    }

	    if ((DMZ_SES_MACRO(32) == 0 || dmd_reptrace() == FALSE) )
	    {
	       /* only if we are not the replication server */
	       status = dm2rep_capture(DM2REP_UPDATE, r, record, dberr);
	    }
	}
	else if ( d->dcb_status & DCB_S_REPLICATE &&
		  r->rcb_xcb_ptr &&
		  r->rcb_xcb_ptr->xcb_rep_input_q == 0 &&
		  (MEcmp(t->tcb_rel.relid.db_tab_name, 
		    (*r->rcb_xcb_ptr->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) ? 
		    DD_INPUT_QUEUE_U : DD_INPUT_QUEUE,
		    DD_INPUT_QUEUE_SIZE) == 0) &&
		(STskipblank(t->tcb_rel.relid.db_tab_name + DD_INPUT_QUEUE_SIZE, 
		    sizeof(DB_TAB_NAME) - DD_INPUT_QUEUE_SIZE)) == NULL)
	{
	    /*
	    ** user is updating the input queue, set up input_q value
	    ** in the XCB so that the distribution threads are activated to
	    ** distribute the values. We use the value -1 so that we know it
	    ** was a user update and not an internal DBMS update
	    */
	    r->rcb_xcb_ptr->xcb_rep_input_q = (DMP_RCB *)-1;
	}

	if ( status )
	    break;

	/*
	**	Before we go any further, if the table has peripheral objects, we must
	**	go figure out where the peripheral objects will be (and also move them
	**	there).  Peripheral objects, in the tuple, are really just pointers to
	**	other places (in the extension tables).  Therefore, until they are
	**	placed, which can only be done at this level, it is not known where
	**	they will end up, and, therefore, whether their representation in the
	**	base tuple has changed.  Thus, we change things here before making a
	**	decision on what has changed.
	*/

	if ((t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) &&
	    (flag & DM2R_REDO_SF) == 0)
	{
	    status = dm1c_preplace(t->tcb_atts_ptr,
				    r,
				    r->rcb_record_ptr,
				    record,
				    dberr);

	    if (DB_FAILURE_MACRO(status))
	    {
		if (dberr->err_code != E_DM0065_USER_INTR
		  && dberr->err_code != E_DM010C_TRAN_ABORTED
                  && dberr->err_code != E_DM016B_LOCK_INTR_FA
                  && dberr->err_code != E_DM5423_SRID_MISMATCH)
		{
		    /*
		    ** User interrupts are now a possibility, since dmpe
		    ** updates may make recursive DMF calls to mung with
		    ** objects in the extension tables.  Given that that's
		    ** the case, we just pass the error back up the chain,
		    ** and the upper layers will handle it correctly
		    */
		    
		    uleFormat(dberr, 0,
			       (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			       (char *)NULL, (i4)0, (i4 *)NULL,
			       &loc_err, 0);
		    SETDBERR(dberr, 0, E_DM9B03_REPLACE_DMPE_ERROR);
		}
		break;
	    }
	    else if (dberr->err_code == E_DM0154_DUPLICATE_ROW_NOTUPD)
	    {
		CLRDBERR(dberr);
		lobj_changed = FALSE;
		status = E_DB_OK;
	    }
	    else
		lobj_changed = TRUE;
	}

	newrecord = record;
	oldrecord = r->rcb_record_ptr;

	/*
	** Now we  have determined old/new records check MAC access and
	** security auditing
	*/
	if ( r->rcb_internal_req != RCB_INTERNAL &&
	     dmf_svcb->svcb_status & SVCB_IS_SECURE )
	{
	    /* 
	    ** Always check the label of record for a replace.
	    */
	    status = dma_row_access(
		    DMA_ACC_UPDATE,
		    r, 
		    newrecord,	/* New pointer */
		    oldrecord, 	/* Old pointer */
		    &r->rcb_compare, dberr);

	    if ( status == E_DB_OK && r->rcb_compare == DMA_ACC_NOPRIV )
	    {
		SETDBERR(dberr, 0, E_DM201E_SECURITY_DML_NOPRIV);
		status = E_DB_ERROR;
	    }
	    if ( status )
		break;
	}

	/*	
	**  Compute the set of attributes that changed.
	**  b113489, INGSRV3051: need to initialize the entire array.
	*/ 
	MEfill((((DB_MAX_COLS + 31) / 32) * sizeof(i4)), '\0', 
		    (PTR) r->rcb_change_set);
	MEfill((((DB_MAX_COLS + 31) / 32) * sizeof(i4)), '\0', 
		    (PTR) r->rcb_value_set);

	type_key_change = ADT_NOCHANGE;
	type_nkey_change = ADT_NOCHANGE;
	delta_start = 0;
	delta_end = 0;
	adf_cb = r->rcb_adf_cb;

	if ( attset && (t->tcb_rel.relstat2 & TCB2_ALTERED) == 0 &&
		!(t->tcb_rel.relencflags & TCB_ENCRYPTED) )
	{
	    status = adt_compute_part_change(adf_cb,r->rcb_tcb_ptr->tcb_rel.relatts,
		t->tcb_atts_ptr,record, r->rcb_record_ptr,
		r->rcb_change_set, 
		r->rcb_value_set,
		(i4 *)attset, 
		&type_key_change, 
		&type_nkey_change, 
		&delta_start, &delta_end);
	}
	else
	{
	    status  = adt_compute_change(adf_cb, r->rcb_tcb_ptr->tcb_rel.relatts,
		t->tcb_atts_ptr,record, r->rcb_record_ptr,
		r->rcb_change_set, 
		r->rcb_value_set,
		&type_key_change, 
		&type_nkey_change);
	}
	if ( status )
	{
	    if (adf_cb->adf_errcb.ad_errcode == E_AD101B_BAD_FLT_VALUE)
	    {
		uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, (i4)0,
			    (i4 *)NULL, err_code, (i4)0);
		SETDBERR(dberr, 0, E_DM008C_ERROR_REPLACING_RECORD);
	    }
	    else
	    {
		SETDBERR(dberr, 0, E_DM004A_INTERNAL_ERROR);
		status = E_DB_FATAL;
	    }
	    break;
	}

#ifdef xDEBUG
	/*
	** If trace point 654 is enabled then fall through with replace even
	** if no changes were made to the target row.
	*/
	if ((type_key_change == ADT_NOCHANGE) &&
	    (type_nkey_change == ADT_NOCHANGE) &&
	    (DMZ_AM_MACRO(54)))
	{
	    type_nkey_change = ADT_REPRESENTATION;
	}
#endif

	/*	See if nothing changed. */
	if ( type_key_change == ADT_NOCHANGE && type_nkey_change == ADT_NOCHANGE )
	{
	    if ((t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) && lobj_changed &&
		    (flag & DM2R_REDO_SF) == 0)
	    {
		/*
		** There were peripheral objects which in fact were updated.
		** Since they cannot be part of a key, this is a value update.
		*/
		type_nkey_change = ADT_VALUE;
	    }
	    else
	    {
		if ((flag & DM2R_XTABLE) && r->rcb_update_mode == RCB_U_DEFERRED)
		{
		    /*
		    ** If row locking/crow locking add the tid to rcb_new_tids
		    ** instead of doing unlogged updates to the row header
		    ** which could interfere with mvcc undo processing
		    */
		    if (row_locking(r) || crow_locking(r))
		    {
			/* Don't do unlogged deferred processing on page */
			status = defer_add_new(r, &r->rcb_currenttid, 
				FALSE, err_code);
		    }
		    else
		    {
			/* Must first swap from CR to Root */
			dm0pLockBufForUpd(r, &r->rcb_data);
			dm0pMutex(&t->tcb_table_io, (i4)0, r->rcb_lk_id,
				    &r->rcb_data);
			status = (*t->tcb_acc_plv->dmpp_dput)(r, 
				r->rcb_data.page, &r->rcb_currenttid,err_code);
			dm0pUnmutex(&t->tcb_table_io, (i4)0, r->rcb_lk_id, 
				    &r->rcb_data);
			/* Release exclusive readlock on Root, restore CR */
			dm0pUnlockBuf(r, &r->rcb_data);
		    }
		    if ( status )
			SETDBERR(dberr, 0, *err_code);
		}	
		/* give error message to QEF */
		if (status == E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM0154_DUPLICATE_ROW_NOTUPD);
		    /*
		    ** If we must return the new tid, return the tid we fetched the
		    ** record with.
		    */
		    if ( flag & DM2R_RETURNTID && tid )
			tid->tid_i4 = fetched_tid.tid_i4;
		}
		else
		    break;
	    }
	}

	/* Still no change? */
	if ( dberr->err_code != E_DM0154_DUPLICATE_ROW_NOTUPD )
	{
	    /* What's in rcb_record_ptr is no longer what's on page */
	    r->rcb_state &= ~RCB_RECORD_PTR_VALID;

	}
	break;
    }

    /* If SI agents, must take mutex */
    if ( r->rcb_siAgents )
	CSp_semaphore(1, &rtb->rtb_cond_sem);

    /* Add the resulting flags to the Base RCB */
    r->rcb_si_flags |= rcb_si_flags;
    /* Un-busy the Base RCB */
    r->rcb_si_flags &= ~RCB_SI_BUSY;


    /* Signal all waiters on RCB_SI_BUSY */
    if ( r->rcb_siAgents )
    {
	rtb->rtb_dm2r_flags = flag;
	rtb->rtb_status = status;
	rtb->rtb_error = *dberr;
	if ( rtb->rtb_cond_waiters )
	    CScnd_broadcast(&rtb->rtb_cond);
	CSv_semaphore(&rtb->rtb_cond_sem);
    }
    /*
    ** If error, or no change to base record, or if it was
    ** polyinstantiated, nothing else to do.
    */
    if ( status || dberr->err_code == E_DM0154_DUPLICATE_ROW_NOTUPD ||
 	 r->rcb_compare == DMA_ACC_POLY )
    {
	return(SIcomplete(r, status, dberr));
    }

    /*
    **  Check for duplicate index values.
    **
    **    We can check for dups before we allocate space in the base
    **    table because the tid is not used during duplicate checking.
    **    We only do the si dupcheck now if 
    **	  rcb_si_flags & RCB_SI_DUP_PRECHECK;
    **    In this case we must dupcheck before updating the base table.
    */
    if ( r->rcb_si_flags & RCB_SI_DUP_PRECHECK )
    {
	if ( r->rcb_siAgents )
	{
	    /* Must wait for all external dupchecks to complete */
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    while ( rtb->rtb_status == E_DB_OK &&
		    rtb->rtb_si_dupchecks > 0 )
	    {
		rtb->rtb_cond_waiters++;
		CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
		rtb->rtb_cond_waiters--;
	    }
	    /* Collective "status" from all dupchecks */
	    status = rtb->rtb_status;
	    *dberr = rtb->rtb_error;
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
	else
	{
	    /* Dupcheck -all- indexes */
	    status = si_dupcheck(t, r, newrecord, 
				 flag | DM2R_MREPLACE, dberr);
	}
    }

    /*  Compute the sizes of the old and new records. */
    oldlength = newlength = t->tcb_rel.relwid;

    /* If there are encrypted columns, encrypt the record */
    if (t->tcb_rel.relencflags & TCB_ENCRYPTED)
	status = dm1e_aes_encrypt(r, &t->tcb_data_rac, newrecord,
			r->rcb_erecord_ptr, dberr);

    /*  Check for compressed storage. */
    if ( status == E_DB_OK &&
        (t->tcb_data_rac.compression_type != TCB_C_NONE ||
         t->tcb_rel.relstat2 & TCB2_ALTERED) )
    {
        if (t->tcb_data_rac.compression_type != TCB_C_NONE)
	{
	    status = (*t->tcb_data_rac.dmpp_compress)(&t->tcb_data_rac,
		      (t->tcb_rel.relencflags & TCB_ENCRYPTED) ?
		      r->rcb_erecord_ptr :
                      newrecord, (i4)t->tcb_rel.relwid,
                      r->rcb_srecord_ptr, &newlength);
	    if ( status )
		SETDBERR(dberr, 0, E_DM008C_ERROR_REPLACING_RECORD);
	}
	if ( status == E_DB_OK )
	    (*t->tcb_acc_plv->dmpp_reclen)(t->tcb_rel.relpgtype, 
		t->tcb_rel.relpgsize, r->rcb_data.page, 
		r->rcb_currenttid.tid_tid.tid_line, &oldlength);
	/* 
	** We cannot use this information to find delta of log record for
	** compressed table or altered table.
	*/
	delta_start = 0;
	delta_end = 0;
    }

    /*	Compute the action to take. */
    action = IN_PLACE;

    /*
    ** Treat all gateway updates as in-place.
    ** (kschendel: why?).  Used to also assume in-place for core catalogs,
    ** but there's no need to prevent e.g. verifydb from updating core
    ** catalog key columns given exclusive access.
    */
    if ( status == E_DB_OK &&
        (t->tcb_rel.relstat & TCB_GATEWAY) == 0 )
    {
	if (type_key_change != ADT_NOCHANGE)
	{
	    if (type_key_change & ADT_VALUE)
	    {
		if ((flag & DM2R_DUPLICATES) &&
		       (t->tcb_rel.relstat & TCB_UNIQUE) == 0)
		    action = ALLOCATE;
		else
		    action = ALLOCATE | DUP_CHECK;
	    }
	    else 
		action = ALLOCATE;
	}
	if (type_nkey_change != ADT_NOCHANGE)
	{
	    if ((t->tcb_rel.relstat & TCB_UNIQUE) == 0 &&
	       type_nkey_change == ADT_VALUE && (flag & DM2R_NODUPLICATES))
	    {
		action |= DUP_CHECK;
	    }
	    if (newlength != oldlength)
		action |= ALLOCATE;
	}

	/* rows span pages, Execute replace as delete,ALLOCATE,put */
	if (t->tcb_seg_rows)
	    action |= ALLOCATE;
    }

    /*
    ** For BTREE update by tid we deferred fixing leaf until it 
    ** was determined if the key was changing
    */
    if ( status == E_DB_OK && flag & DM2R_HAVE_DATA)
    {
        if ((action & ALLOCATE) == 0)
	    flag |= DM2R_IN_PLACE;
	status = dm2r_fetch_record(r, tid, flag, DM1C_MREPLACE, dberr);
    }

    newbid.tid_i4 = NOT_ALLOCATED;
    newtid.tid_i4 = NOT_ALLOCATED;

    /*
    ** Perform the actual replace according to the access method.
    */
    if ( status == E_DB_OK && 
	((row_locking(r) || crow_locking(r)) && (action & ALLOCATE) || 
	    t->tcb_seg_rows) )
    {
	/*
	** Execute replace as delete/allocate/put
	** This is necessary to follow the stricter page fixing protocol
	** required for row locking
	*/
	status = base_delete_put(r, action, flag, newrecord, oldlength, 
				newlength, &oldtid, &oldbid, &newtid, &newbid,
				&newdataPinfo, &newleafPinfo, dberr);
    }
    else if ( status == E_DB_OK )
    {
	/* Execute replace as replace */
	status = base_replace(r, action, flag, newrecord, oldlength, newlength,
				&oldtid, &oldbid, &newtid, &newbid, &newdataPinfo, 
				&newleafPinfo, delta_start, delta_end, dberr);
    }

    /*
    ** Make sure that all locally fixed pages are unfixed.
    */
    if (newdataPinfo.page && (status == E_DB_OK))
    {
	status = dm0p_unfix_page(r, DM0P_UNFIX, &newdataPinfo, dberr);
    }
    if (newleafPinfo.page && (status == E_DB_OK))
    {
	status = dm0p_unfix_page(r, DM0P_UNFIX, &newleafPinfo, dberr);
    }

    if ( status )
    {
	if (newdataPinfo.page)
	    status = dm0p_unfix_page(r, DM0P_UNFIX,
			&newdataPinfo, &local_dberr);
	if (newleafPinfo.page)
	    status = dm0p_unfix_page(r, DM0P_UNFIX,
			&newleafPinfo, &local_dberr);
	status = E_DB_ERROR;
    }


    /*	Check for secondary index updates. If we are done here then check
    ** for replication and perform data capture
    **
    ** Since Gateway indices aren't updated, this is the little hole
    ** through which the Gateway ball drains out of this pinball
    ** machine.   schang: gw merge 05/26/92
    */
    /*
    ** FIX_ME: (stephenb) It might be a good idea to add a replication
    ** check here, that way we could also replicate inside the non-relational
    ** gateway DBMS's (something to consider at another time)
    */

    /* ...Now the threaded si_replace's are allowed to run... */

    /* Wake up all waiters on the old and new TIDs */
    if ( status == E_DB_OK )
    {
	if ( r->rcb_siAgents )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    r->rcb_si_newtid.tid_i4 = newtid.tid_i4;
	    r->rcb_si_oldtid.tid_i4 = oldtid.tid_i4;
	    if ( rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
	else
	{
	    r->rcb_si_newtid.tid_i4 = newtid.tid_i4;
	    r->rcb_si_oldtid.tid_i4 = oldtid.tid_i4;
	}
    }



    /*
    ** If this is a core system catalog (must be iirelation, since it's the
    ** only one which has a secondary index), then unfix its pages here. This
    ** ensures that we comply with the cluster node failure recovery rule that
    ** a transaction may only fix one core system catalog page at a time.
    */
    if ( status == E_DB_OK && t->tcb_rel.relstat & TCB_CONCUR &&
	 t->tcb_update_idx )
    {
	status = dm2r_unfix_pages(r, dberr);
    }

    /*	Prepare to loop through the indices on (master's) table. */

    if ( status == E_DB_OK && t->tcb_update_idx && !r->rcb_siAgents )
    {
	idxnum = 0;
	r->rcb_ra_idxupd = 0;

	/* Get indexes from master, not this partition */

	for (it = t->tcb_pmast_tcb->tcb_iq_next;
	     status == E_DB_OK && 
		 it != (DMP_TCB*)&t->tcb_pmast_tcb->tcb_iq_next;
	     it = it->tcb_q_next, ++idxnum)
	{
	    status = si_replace(it, r, record, flag, dberr);

	    /* 
	    ** For readahead determination, record what indexes will be updated 
	    ** in a bitmask. Use the position off the tcb, as its just for this
	    ** instance of the tcb/rcb that it will be saved/used.           
	    */ 
	    if ( status == E_DB_OK && dberr->err_code != E_DM0154_DUPLICATE_ROW_NOTUPD )
	    {
		if ( idxnum < BITS_IN(r->rcb_ra_idxupd) )
		    r->rcb_ra_idxupd |= 1 << idxnum;
	    }
	}
    }

    if ( (status = SIcomplete(r, status, dberr)) == E_DB_OK )
    {
	/* If tid != NULL, return the new tid after the replace operation. */
	if ( flag & DM2R_RETURNTID && tid )
	    *tid = newtid;
    }

    /*	Return completion. */

    return (status);
} /* dm2r_replace */

/*{
** Name: dm2r_si_replace - Replace a record's secondary index entry (threaded).
**
** Description:
**	
**      This function replaces an entry in an index.
**      One secondary index is updated.
**	It is called from dmrAgent by a SI agent.
**
** Inputs:
**	it				The index TCB
**      rcb                             The index's Base RCB.
**	rcb_flag			DM2R_BYPOSITION or
**                                      or DM2R_BYTID.
**	record				Pointer to the new Base record.
**
** Outputs:
**      err_code                        The reason for error status.
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
**	11-Nov-2003 (jenjo02)
**	    Created for Parallel/Partitioning project.
*/
DB_STATUS
dm2r_si_replace(
DMP_RCB		*ir,
char		*record,
DB_ERROR	*dberr)
{
    DMP_RCB	*r = ir->rcb_base_rcb;
    DMR_RTB	*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB	*it = ir->rcb_tcb_ptr;
    DMP_DCB	*d = it->tcb_dcb_ptr;
    DB_STATUS	status = E_DB_OK;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    CSp_semaphore(1, &rtb->rtb_cond_sem);

    /* Wait until the Base RCB is initialized */
    while ( rtb->rtb_status == E_DB_OK &&
	    r->rcb_si_flags & RCB_SI_BUSY )
    {
	rtb->rtb_cond_waiters++;
	CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	rtb->rtb_cond_waiters--;
    }

    /* If no change or row was polyinstantiated, do nothing */
    if ( rtb->rtb_status ||
	 rtb->rtb_error.err_code == E_DM0154_DUPLICATE_ROW_NOTUPD ||
 	 r->rcb_compare == DMA_ACC_POLY )
    {
	CSv_semaphore(&rtb->rtb_cond_sem);
    }
    else
    {
	/*
	** If this is a unique index and it's slated for
	** pre-replace duplicate checking, do it now.
	*/
	if ( it->tcb_rel.relstat & TCB_UNIQUE &&
	     r->rcb_si_flags & RCB_SI_DUP_PRECHECK )
	{
	    CSv_semaphore(&rtb->rtb_cond_sem);
	    /* Note that this decrements rtb_si_dupchecks */
	    status = si_dupcheck(it, ir, record, 
				 rtb->rtb_dm2r_flags |
				     (DM2R_MREPLACE | DM2R_SI_AGENT),
				 dberr);
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	}
	
	/*
	** Must wait for all other dupchecks to complete and
	** for the Base row's old and new TIDs to materialize.
	*/
	while ( rtb->rtb_status == E_DB_OK &&
		(rtb->rtb_si_dupchecks ||
		 r->rcb_si_oldtid.tid_i4 == NOT_ALLOCATED ||
		 r->rcb_si_newtid.tid_i4 == NOT_ALLOCATED) )
	{
	    rtb->rtb_cond_waiters++;
	    CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	    rtb->rtb_cond_waiters--;
	}

	/* Now do the replace */
	if ( rtb->rtb_status == E_DB_OK )
	{
	    CSv_semaphore(&rtb->rtb_cond_sem);
	    status = si_replace(it, ir, record, 
				rtb->rtb_dm2r_flags | 
				    (DM2R_MREPLACE | DM2R_SI_AGENT),
				dberr);
	}
	else
	    CSv_semaphore(&rtb->rtb_cond_sem);

	/* Unfix any SI pages we may have left fixed */
	if ( dm2r_unfix_pages(ir, &local_dberr) && status == E_DB_OK )
	{
	    status = E_DB_ERROR;
	    *dberr = local_dberr;
	}
    }

    /* Return our status, not RTB status */
    return(status);
}

/*{
** Name: si_dupcheck - Check for duplicates on unique secondary indices.
**
** Description:
**      This function checks  to see if any of the unique secondary
**      indices have duplicates.
**
**      If 'value_set' is non-zero, it is checked to determine which of the
**      2-ary index's columns actually changed. Unless at least one key column
**      had a true change in value (as opposed to a change in representation
**      only), the update cannot result in a duplicate 2-ary index entry, so
**      we don't need to check.
**
**      If value_set is zero, or if at least one key column actually changed
**      value, then the new secondary index key may duplicate an entry which
**      is already present. We check for duplicates by calling the appropriate
**      allocate routine with the 'only dup check' flag set; if the new key
**      would duplicate an already existing key we return that fact to our
**      caller.
**
** Inputs:
**      rcb                             RCB of base table.
**      value_set                       description of the columns which
**                                      actually changed value. value_set is
**                                      0 if this is a put.
**      record                          Record to add.
**
** Outputs:
**	err_code			Reason for error return status.
**					    E_DM0045_DUPLICATE_KEY
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
**	26-feb-87 (jennifer)
**          Created new for jupiter.
**	 1-dec-88 (rogerk)
**	    Don't check duplicates if a non-keyed entry of the secondary index
**	    changed.
**      12-nov-1990 (bryanp)
**          Updated the comments to reflect that this routine is called with
**          the set of columns which changed VALUE, as opposed to the set of
**          columns which changed in REPRESENTATION only. If the only change
**          is a representation change, then this replace cannot cause a
**          duplicate key (and, in fact, we cannot check for one -- else we'd
**          reject the record as a duplicate of itself!).
**	18-feb-1993 (jhahn)
**	    Added check for statement level unique indexes.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Row locking: Acquire value lock for duplicate checking
**      17-jan-97 (stial01)
**          Return E_DB_ERROR if dm2r_lock_value fails (B80066)
**      27-jan-97 (dilma04)
**          Initialize status variable which could be left uninitialized
**          after previous fix to this routine.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**      29-may-97 (stial01)
**          si_dupcheck: Memory leak if only getting value lock
**          Remove explicit unfix, dm2r_return_rcb will unfix pages in rcb
**      13-jun-97 (dilma04)
**          Ensure that no locks will be released during duplicate checking.
**      03-may-99 (stial01)
**          Only SI dupcheck if it must be done before base table put
**	30-mar-03 (wanfr01)
**          Bug 109914, INGSRV 2156
**	    Add DM1C_DUP_NONKEY flag to indicate si_dupcheck did not need
**          to perform duplicate checking (so no control lock grabbed)
**	22-Jan-2004 (jenjo02)
**	    Modified to be callable from the mainline code or from
**	    a parallel SI Agent.
**	    Added Global SI, TID8 support.
**	26-Jan-2004 (jenjo02)
**	    Get SI from master's TCB.
**	07-Apr-2004 (jenjo02)
**	    Add RCB_SI_VALUE_LOCKED flag.
*/
static DB_STATUS
si_dupcheck(
    DMP_TCB	*it,
    DMP_RCB	*r,
    char        *record,
    i4		flag,
    DB_ERROR	*dberr )
{
    DB_STATUS	    status, rtb_status;
    DMP_RCB         *ir;
    DMR_RTB	    *rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB         *t = r->rcb_tcb_ptr;
    char            *irec;
    i4         	    irec_len;
    DM_TID          idxtid, idxbid;
    i4         	    i;
    char	    *crec = (PTR)0;
    i4		    rcb_update_mode, rcb_state;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = E_DB_OK;
    rtb_status = E_DB_OK;

    /*
    ** Check a single index (threaded), or all indexes?
    **
    ** If called with "it" being a base table's TCB
    ** rather than an index TCB, we dupe check
    ** all unique index TCBs associated with the base table's
    ** master TCB.
    */
    if ( (it->tcb_rel.relstat & TCB_INDEX) == 0 )
    {
	/* All (master's) indexes: */

	for ( it = t->tcb_pmast_tcb->tcb_iq_next; 
	      status == E_DB_OK && 
		  it != (DMP_TCB*)&t->tcb_pmast_tcb->tcb_iq_next;
	      it = it->tcb_q_next)
	{
	    if ( it->tcb_rel.relstat & TCB_UNIQUE )
		status = si_dupcheck(it, r, record, flag, dberr);
	}

	/* Note in the Base RCB that all secondaries have been checked */
	r->rcb_si_flags |= RCB_SI_DUP_CHECKED;
	return(status);
    }

    /* If dmrAgent dupcheck, "r" is the SI RCB */
    if ( flag & DM2R_SI_AGENT )
    {
	ir = r;
	r  = ir->rcb_base_rcb;
	t  = r->rcb_tcb_ptr;
	rtb_status = rtb->rtb_status;
    }

    /* Do nothing if global failure in Base RCB */
    while ( rtb_status == E_DB_OK )
    {
	/*
	** If "replace":
	**
	** Check to set if any keyed attribute in this index changed.
	** If one did, then we have to make sure that the new secondary
	** index entry will not duplicate an already existing entry.
	**
	** If only non-keyed attributes changed, then we don't have 
	** to do duplicate checking.
	*/
	if ( flag & DM2R_MREPLACE )
	{
	    for (i = 0; i < it->tcb_rel.relatts - 1; i++)
	    {
		/*
		** See if the value changed and check if the
		** attribute is a key of the secondary index.
		*/

		if ((r->rcb_value_set[it->tcb_ikey_map[i] >> 5] &
			(1 << (it->tcb_ikey_map[i] & 0x1f))) &&
		    (it->tcb_atts_ptr[i+1].key))
		{
		    break;
		}
	    }
	    if ( i == it->tcb_rel.relatts-1 )
		break;
	}

	/*
	**  Allocate an RCB for the secondary index. Do duplicate
	**  checking with DIRECT mode cursor - even if base table
	**  is deferred.
	*/
	if ( flag & DM2R_SI_AGENT )
	{
	    /* If durable SI RCB, preserve update mode, state */
	    rcb_update_mode = ir->rcb_update_mode;
	    rcb_state = ir->rcb_state;
	}
	else if ( status = dm2r_rcb_allocate(it, r, &r->rcb_tran_id,
		r->rcb_lk_id, r->rcb_log_id, &ir, dberr) )
	{
	    break;
	}

	ir->rcb_update_mode = RCB_U_DIRECT;
	
	/*
	** Turn RCB_CSRR_LOCK off to ensure that we are not going to 
	** release locks during duplicate checking.
	*/
	ir->rcb_state &= ~RCB_CSRR_LOCK;

	/*
	** Build the secondary index record from the primary record 
	** and a dummy tid.

	** (big)Tid is not used for duplicate checking even when it is part of key
	*/

	for (i = 0; i < it->tcb_rel.relatts - 1; i++)
	{
	    MEcopy(&record[t->tcb_atts_ptr[it->tcb_ikey_map[i]].offset],
		t->tcb_atts_ptr[it->tcb_ikey_map[i]].length,
		&ir->rcb_record_ptr[it->tcb_atts_ptr[i + 1].offset]);
	}

	/* Zero the tid */
	MEfill( (it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX)
		    ? sizeof(DM_TID8) : sizeof(DM_TID), '\0',
	         &ir->rcb_record_ptr[it->tcb_atts_ptr[i + 1].offset]);

	/*
	** When calling allocate to allocate space you need to pass
	** the length of the compressed tuple
	** When calling allocate to check for duplicates only, the length
	** is ignored, so don't bother to compress the si record
	*/

	irec = ir->rcb_record_ptr;
	irec_len = it->tcb_rel.relwid;

	/*
	** Get value lock. It must be held until after allocate 
	** reserves space, that is, it cannot be released
	** after allocate(DM1C_ONLYDUPCHECK), below.
	*/
	if ( row_locking(ir) || crow_locking(ir) )
	{
	    status = dm1r_lock_value(ir, (i4)0,
			ir->rcb_record_ptr, dberr);

	    /*
	    ** Note that we now hold the value lock.
	    **
	    ** This is worthless information for non-threaded
	    ** SI updates because we're constantly allocating
	    ** and throwing away the SI RCB and by the time
	    ** we get to si_put, we'll have no way of knowing
	    ** if the value lock is held or not.
	    **
	    ** This is a problem for "non-key" replaces;
	    ** if we skipped the dup check, above, because
	    ** it's a non-key update, then we don't acquire
	    ** this value lock, but si_put has no way of 
	    ** knowing that except to redo the test for
	    ** non-key updates.
	    **
	    ** For threaded updates, the RCB persists as will
	    ** this flag.
	    */
	    ir->rcb_si_flags |= RCB_SI_VALUE_LOCKED;
	}

	/* Now check for duplicates... */
	if (status == E_DB_OK)
	{
	    switch (it->tcb_rel.relspec)
	    {

	    case TCB_HASH:
		status = dm1h_allocate(ir, &ir->rcb_data, &idxtid, 
		    ir->rcb_record_ptr, crec, irec_len, 
		    DM1C_ONLYDUPCHECK, dberr);
		break;


	    case TCB_ISAM:
		status = dm1i_allocate(ir, &ir->rcb_data, &idxtid,
		    ir->rcb_record_ptr, crec, irec_len, 
		    DM1C_ONLYDUPCHECK, dberr);
		break;

	    case TCB_BTREE:
		status = dm1b_allocate(ir, &ir->rcb_other, 
		    &ir->rcb_data, &idxbid, &idxtid,
		    ir->rcb_record_ptr, crec, irec_len, ir->rcb_s_key_ptr,
		    DM1C_ONLYDUPCHECK, dberr);
		break;

	    case TCB_RTREE:
		status = E_DB_ERROR; /* unique rtree indexes not supported */
		break;
	    }
	}

	/* Give up the non-threaded SI RCB */
	if ( flag & DM2R_SI_AGENT )
	{
	    /* Restore update mode and state, preserve SI RCB */
	    ir->rcb_update_mode = rcb_update_mode;
	    ir->rcb_state = rcb_state;
	}
	else if ( dm2r_return_rcb(&ir, &local_dberr) && status == E_DB_OK )
	{
	    status = E_DB_ERROR;
	    *dberr = local_dberr;
	}

	if ( status )
	{
	    if ( dberr->err_code == E_DM0045_DUPLICATE_KEY ||
		 dberr->err_code == E_DM0046_DUPLICATE_RECORD )
	    {
		if (it->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE)
		    SETDBERR(dberr, 0, E_DM0151_SIDUPLICATE_KEY_STMT);
		else
		    SETDBERR(dberr, 0, E_DM0048_SIDUPLICATE_KEY);
	    }
	}
	break;
    }

    /*
    ** If separate SI threads, set the worst status
    ** in the RTB, decrement the count of
    ** indexes being dupchecked, and wake up
    ** anyone waiting for this count to go to zero.
    */
    if ( flag & DM2R_SI_AGENT )
    {
	CSp_semaphore(1, &rtb->rtb_cond_sem);
	if ( status && rtb->rtb_status == E_DB_OK )
	{
	    rtb->rtb_status = status;
	    rtb->rtb_error = *dberr;
	}
	if ( --rtb->rtb_si_dupchecks <= 0 )
	{
	    /* Flag the Base RCB that all SI checks are done */
	    r->rcb_si_flags |= RCB_SI_DUP_CHECKED;
	    if ( rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	}
	CSv_semaphore(&rtb->rtb_cond_sem);
    }


    return (status);
}

/*{
** Name: dm2r_unfix_pages - Unfix pages in rcb fixed-page slots.
**
** Description:
**      This function unfixes any pages that have been fixed in the
**	given rcb.  It is used by dmr routines to make sure no pages
**	from backend system catalogs are left locked on return from
**	dmf.  Only short-term page locks should be used on these
**	relations.
**
**	Set RCB_FETCH_REQ in the rcb when pages are unfixed to indicate
**	that the next time into DMR, the page for the specified tid
**	must be refetched.
**
** Inputs:
**      rcb                             RCB of base table.
**
** Outputs:
**	err_code			Reason for error return status.
**					    E_DM9316_DM2R_UNFIX_PAGES
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
**	19-jul-87 (rogerk)
**          Created new for jupiter.
**	19-Nov-2003 (jenjo02)
**	    Unfix both pages, even if first unfix fails.
*/
DB_STATUS
dm2r_unfix_pages(
    DMP_RCB   	*rcb,
    DB_ERROR	*dberr )
{
    DB_STATUS	    status = E_DB_OK;
    i4		    loc_err;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (rcb->rcb_other.page)
    {
	status = dm0p_unfix_page(rcb, DM0P_UNFIX, &rcb->rcb_other,
	    &local_dberr);
	if ( status )
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &loc_err, 0);
    }

    if (rcb->rcb_data.page)
    {
	status = dm0p_unfix_page(rcb, DM0P_UNFIX, &rcb->rcb_data,
	    &local_dberr);
	if ( status )
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &loc_err, 0);
    }

    rcb->rcb_state |= RCB_FETCH_REQ;

    if ( status )
	SETDBERR(dberr, 0, E_DM9316_DM2R_UNFIX_PAGES);

    return (status);
}

/*{
** Name: dm2r_load	- Load a record.
**
** Description:
**	DM2R_LOAD is a fast way to add records to an empty table, or
**	to a non-empty heap.  The table may be temporary or permanent
**	and of any structure.
**
**	While the table is being loaded, no other record level calls may
**	be performed it.  When all rows have been added, DMR2_LOAD should
**	be called with the DM2R_L_END flag.  This will make the table
**	available for normal record access.
**
**	TUPLES ADDED WITH DM2R_LOAD ARE NOT LOGGED.  Recovery consists of
**	restoring the table to its empty state, or in the case of a
**	non-empty heap, to the state it was in prior to the load.
**
**	When the DM2R_L_BEGIN flag is specified, the sorter is initialized.
**	When the DM2R_L_ADD flag is specified, the records given are handed
**	    to the sorter.
**	When the DM2R_L_END flag is given, all records are transferred from
**	    the sorter to the table being loaded.
**
**	If the DM2R_NOSORT flag is specified for a heap table, the sorter
**	will be bypassed and rows are added to the table as they received.
**	This is referred to as "heapdirect" in the code.
**
**	If the DM2R_ELIMINATE_DUPS flag is set, duplicate rows will be removed
**	silently.  If the table is a UNIQUE structure, unique keys will be
**	removed silently, unless DM2R_DUPKEY_ERR is specified - in which
**	case an error will be returned.
**
**	This routine is complicated by the numerous combinations
**	of table structures and input actions, and by recovery
**	constraints.  Recovery is performed either by renaming
**	old versions of files back to the original, or by truncating
**	the file.
**
**	The DM2R_EMPTY flag indicates that the table is known to be empty.
**	If the empty flag is not passed the table may in fact be empty but
**	we are not guaranteed such.
**
** Inputs:
**      rcb				The Record access Control Block.
**	tcb				Describes table being loaded.
**      type                            Type of operation: must be
**                                      DM2R_L_BEGIN, DM2R_L_END, or
**				        DM2R_L_ADD.
**	flag				Load operational flags:
**                                      DM2R_ELIMINATE_DUPS, DM2R_CMP_SORT,
**					DM2R_NOSORT, DM2R_DUPKEY_ERR or
**					DM2R_EMPTY.
**	count				Number of records in record_list.
**      record_list                     Pointer to list of DM_MDATA structs
**					containing records to load.
**	est_rows			estimate of number rows to be loaded.
**
** Outputs:
**	count				On DM2R_L_END call, this holds the
**					number of rows added to the table.
**					On an error, this tells which record
**					in the record_list the error occurred on
**      err_code                        The reason for error status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL			Operation was partially completed,
**					the transaction must be aborted.
**	Exceptions:
**	    none
**
** Side Effects:
**	Tuple describing table in IIRELATION may be updated.
**
** History:
**	04-sep_86 (jennifer)
**          Created new for jupiter.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_create_file() and
**	    dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**      19-sep-1989 (jennifer)
**          Add security label for bulk copy and maintaining system
**          maintained logkey/tabkey.
**	27-sep-89 (jrb)
**	    Copied precision into cmp_precision field for decimal.
**      25-jan-1991 (JENNIFER)
**          Indicated that these sorts have EXTERNAL estimates.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then don't write load log records.
**	    12-feb-91 (jrb)
**		Added work location array parameter to dmse_begin call.  This
**		is for 6.3/04 only and will not be needed in later versions.
**	     7-mar-1991 (bryanp)
**		Added support for Btree index compression: set new fields in
**		the DM2U_M_CONTEXT structure before calling the Btree build
**		routines.
**	    10-apr-1991 (bryanp)
**		B36837: Btree loading wasn't setting the 'dup' flag properly
**		for dm1bbput() because we never got E_DB_INFO from
**		dmse_get_record because we didn't correctly set DMSE_CHECK_KEYS
**		for dmse_begin.
**	24-oct-1991 (mikem)
**	    Updated dm1c_sys_init() call to match new call parameters changed
**	    as part of the 6.4->6.5 merge.
**	 3-nov-1991 (rogerk)
**	    Fix recovery problem in COPY to existing HEAP table.  We were
**	    logging garbage value for mct_recreate which would cause us to
**	    not do proper recovery in dmve_load.
**	    Initialize mct_recreate to 0.  We no longer do the create-new-file
**	    then rename-it-to-oldfile dance here in dm2r_load.  To create a
**	    new version of a table, we now just write over the old file and
**	    mark all unused pages as free (ie mct_recreate is always 0).
**	31-mar-1992 (bryanp)
**	    When calling dm0p_close_page for a temporary table, don't pass
**	    DM0P_CLOSE as the flag -- we can just pass a zero flag value.
**	14-apr-1992 (rogerk)
**	    Bug #40998: Fix byte-align problem in Hash loads.  In dm2r_load,
**	    use I4ASSIGN_MACRO to put hash value into sort row, rather
**	    than direct assignment.  Then when taking rows out of the sorter
**	    in order to call dm1hbput(), use I4ASSIGN_MACRO to extract out
**	    the hash value rather than passing a pointer into the unaligned
**	    tuple buffer.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set mct_data_atts, not mct_atts_ptr which has
**	    now gone. Set new mct_acc_tlv and set mct_compressed to type
**	    of compression.
**	29-August-1992 (rmuth)
**	    - Use new dm1p_lastused flag to find out last used data page in
**	      the table.
**	    - When a dm2r_load was performed on a temporary table it was not
**	      copying the new iirelation info into its TCB. This was causing
**	      problems latter.
**      14-sep-1992 (bryanp)
**	    Bug #39344: If dm2t_control encounters deadlock, return deadlock
**	    to caller (rather than returning a DMF Internal error).
**	8-oct-1992 (bryanp)
**	    Initialize mct_last_data_pageno to -1 in dm2r_load().
**	30-October-1992 (rmuth)
**	    Set the mct_guarantee_on_disk flag.
**	06-nov-92 (jrb)
**	    Changed interface to dmse_begin to pass in multiple locations.
**	14-dec-1992 (jnash)
**	    Reduced logging project.
**	      - Back out change that removed temp file creation, file
**		swapping, etc.  The old approach remains the default
**		behavior for ISAM, BTREE and HASH tables, except that ISAM
**		builds no longer require the contruction of a temporary
**		overflow file.  This involved removing references to
**		dm2t_rename_file and introducing the update_load_tab()
**		and rename_load_tab() functions.
**	      - Modify the meaning of the mct.rebuild and mct.recreate
**	  	states and consequential actions.
**	      - Eliminate redundant DM2R_TABLE_EXISTS logic, replace with
**		TCB_TEMPORARY equivalent.
**	      - Changed the parameters to dm0l_frename.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-June-1993 (rmuth)
**          Not setting mct_recreate flag properly, precedence problem.
**	26-July-1993 (rmuth)
**	    Add the build_tbl_info parameter to dm2r_load, this allows
**	    the caller to pass information on how to build the table.
**	23-aug-1993 (rogerk)
**	    Changed dm2r_load to use new dm2u_file_create routine.
**	16-oct-93 (jrb)
**	    Added dcb_wrkspill_idx to dmse_begin call.
**	18-oct-1993 (rogerk)
**	    Changed decisions on when to use rebuild and recreate mode
**	    load algorithms.  We now sometimes use recreate/rebuild
**	    loads on heap copies if the row estimate is large.  This
**	    can make recovery much faster and friendlier on large loads.
**	    Added support for creating load table for heapdirect recreate case.
**	31-jan-1994 (mikem)
**	    bug #57664
**	    Changed dm2r_load to always properly initialize the db_sync_flag
**	    which controls whether or not table updates are guaranteed on disk
**	    following a DIwrite().  Previous to this change the variable was
**	    uninitialized in the case of bulk load (copy into empty file) into
**	    a structured table.
**      22-feb-1994 (andys)
**	    Integrate 1-nov-1993 (andys) from ingres63p.
**          Loads must wait if an online checkpoint is in progress.
**          [bug 52253]
**	18-apr-1994 (jnash)
**	    fsync project.  Open temp files in sync'd databases
**	    DI_USER_SYNC_MASK, force them prior to close.  (A change
**	    of this type could also be made to heapdirect loads, but
**	    has not yet been done.)
**      15-Apr-1994 (fred)
**          Pass user interrupts back up the chain.
**	23-may-1994 (andys)
**	    When loading into a non-empty heap, update reltups rather than
**	    overwriting it. [bug 49726]
**	23-may-1994 (rmuth)
**	    bug #63511,
**	    When loading into a heap table we never rebuild the existing table
**	    but always just add to the end. This requires us to read some
**	    of the existing tables's pages e.g. The last datapage, FHDR,
**	    FMAP(s). For temporary tables the only code which knows if these
**	    pages are on disk or in memory is the buffer manager so make sure
**	    that in the above case we always set mct_inmmemory and related
**	    fields to TRUE. This will force the dm1x code to use the buffer
**	    manager which is guaranteed to do the correct thing.
**	10-aug-95 (sarjo01)
**	    Added support for fastload function.
**	27-nov-1995 (kinpa04)
**	    changed dm0m_allocate() call to use ME_ALIGN_MACRO to calculate
**	    correctly for axp_osf
**	29-dec-1995 (nick)
**	    Stall if online checkpoint is in progress just before logging
**	    the load record and not unconditionally at the start. #73388
**	09-jan-96 (toumi01; from 1.1 axp_osf port) (kinpa04)
**	    In case DM2R_L_BEGIN call to dm0m_allocate use ME_ALIGN_MACRO
**	    to ensure correct computation of storage size even on a system
**	    with 64-bit alignment requirement.
**	31-jan-1996 (prida01)
**	    73737 with temporary tables need to ensure we write through the
**	    cache so mct_inmemory needs to be set. We need to ensure this
**	    is set to TRUE.
**	06-mar-1996 (stial01 for bryanp)
**	    Set mct_page_size to tcb_rel.relpgsize.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
**      06-may-1996 (nanpr01)
**          Remove the dependency of DB_MAXTUP which was omitted on VPS
**	    change submission. Also initialize mct_acc_plv.
**	    Modified the dm2u_maxreclen call.
**	13-Aug-1996 (prida01)
**	    Give relatts correct type for call to dm1c_pput
**	28-Nov-1996 (pchang)
**	    Fixed bug 79320.  A typo - 'relstat' for 'relspec' could cause 
**	    BTREE tables with non-unique keys to have two leaf pages containing
**	    entries with the same key values after data had been loaded. 
**	    Consequently, partial updates could go undetected and result in the
**	    breach of data integrity.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dm1h_hash() and dm1h_newhash().
**      22-nov-1996 (nanpr01)
**          init the version with the current table version to dmpp_load.
**      10-mar-1997 (stial01)
**          dm2r_load() Init mct_crecord, area to compress a record
**	05-mar-1998 (hanch04)
**	    Call dm0m_allocate instead of using static memory.
**	22-jul-1998 (thaju02)
**	    change DM1P_FSEG to DM1P_FSEG_MACRO.
**	28-jul-1998 (somsa01)
**	    Tell dm1c_pput() that we are performing a load operation.
**	    Also, added code to perform a DM2R_L_END on each extension
**	    table which is being loaded as well.  (Bug #92217)
**      08-oct-98 (stial01)
**          dm2r_load() if mct_recreate, fcb memory leak fixed. 
**      08-oct-98 (stial01)
**          dm2r_load() DM2R_L_END, DM2R_SORTNC: Don't reset RCB_LSTART because
**          we still have the load context buffer (LCT) to be freed.
**          Also if mct_recreate, fcb memory leak fixed. 
**      16-oct-98 (stial01)
**          dm2r_load() Return error if sort load with 0 sort keys
**       7-jan-99 (stial01)
**          dm2r_load() Removed buffer parameter for dm1h_hash, dm1h_newhash
**	12-feb-1999 (somsa01)
**	    Set load_blob based upon rcb_reltups. It is more accurate than
**	    the TCB's reltups.
**      09-feb-99 (stial01)
**          dm2r_load() Init mct_kperleaf
**	17-dec-99 (gupsh01)
**	    Added additional check to give out correct error message in case of 
**	    updating a readonly database. 
**	08-Nov-2000 (jenjo02)
**	    Set rcb_state RCB_TABLE_UPDATED.
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB from XCB instead of SCU_INFORMATION.
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin() prototype.
**	15-Jan-2004 (schka24)
**	    Ask utility to do logical key stuff.
**	6-Feb-2004 (schka24)
**	    Update call to build-fcb.
**	11-Aug-2004 (schka24)
**	    Build data cleanup: eliminate some unused lct/mct things.
**	18-Apr-2005 (jenjo02)
**	    Additional "cmp_count" parm for dmse_begin.
**	13-Apr-2006 (jenjo02)
**	    Tell all if loading CLUSTERED Btree.
**      13-Nov-2006 (kschendel)
**         Substantially reduce decision point for rebuild vs nonrebuild
**         load into empty heap (prefer rebuild loads).
**	11-Jan-2008 (kschendel) b122122
**	    Load into an empty heap wasn't forcing, because end load was
**	    looking at the db sync flags, but those were set at begin load.
**	25-Mar-2009 (thaju02)
**	    If an error occurs during iietab load, proceed to error 
**	    handling. (B121852)
**      21-may-2009 (huazh01)
**          dm1h_hash(), dm1h_newhash(), dm1h_keyhash() now includes
**          DMP_RCB and err_code in the parameter list. (b122075)
**	6-Nov-2009 (kschendel) SIR 122757
**	    Pass direct-io, private flags for load files.
**	12-Nov-2009 (kschendel) SIR 122882
**	    De-support ancient (6.0) hash tables.
**	17-Nov-2009 (kschendel) SIR 122890
**	    Break out invalidate of loaded table separately.  Include
**	    the DMF changes needed for insert/select to use load:
**	    don't take a control lock for nonrebuild/nonrecreate load;
**	    do iirelation update via tid rather than scan; write
**	    cache pages pre-load and toss post-load, if non-rebuild load;
**	    Always do empty-heap loads rebuild+recreate, the other way
**	    is too prone to mistakes (bad row estimates and slowness).
**	23-Mar-2010 (kschendel) SIR 123448
**	    Define no-parallel-sort flag for load.
**	    Fix master reltups, relpages after bulk-load of a partition.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Don't make LOB etab bulk-load decisions here.  Move all of that
**	    to dmpe.
**	10-may-2010 (stephenb)
**	    reltups and relpages are now i8 on disk (and therefore in the
**	    DMP_RELATION structure, use new CSadjust_i8counter to peform
**	    atomic counter adjustment. Also, restrict actual values to
**	    MAXI4 until we can handle larger numbers.
*/
DB_STATUS
dm2r_load(
    DMP_RCB     *rcb,
    DMP_TCB	*t,
    i4	type,
    i4	flag,
    i4	*count,
    DM_MDATA	*record_list,
    i4	est_rows,
    DM2R_BUILD_TBL_INFO *build_tbl_info,
    DB_ERROR	*dberr )
{
    DML_XCB             *xcb = rcb->rcb_xcb_ptr;
    DM2R_L_CONTEXT	*lct = (DM2R_L_CONTEXT *)t->tcb_lct_ptr;
    DM2U_M_CONTEXT      *mct;
    DMP_TABLE_IO 	*tbio = &t->tcb_table_io;
    DM_MDATA		*data_ptr;
    DB_ATTS		*att;
    char	        *put_record;
    char		*p;
    DMP_RELATION	relrecord;
    LG_LSN		lsn;
    i4		i, k;
    i4		sort_keys;
    i4		sort_width;
    i4		leaf_cmpcontrol_size;
    i4		index_cmpcontrol_size;
    i4			dup;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4		error, local_err_code;
    DMP_LOCATION        *loc_array;
    i4             loc_count;
    u_i4		db_sync_flag;
    i4		close_flag;
    i4			hash_bucket;
    i4		tup_per_page;
    u_i4		estimated_pages;
    bool		file1_open = FALSE;
    DMP_EXT             *ext;
    i4             size;
    DML_LOC_MASKS	*lm;
    DML_SCB		*scb = xcb->xcb_scb_ptr;
    DB_ATTS             **keyatts = (DB_ATTS **)0;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);


#ifdef xDEBUG
    if (rcb->rcb_type != RCB_CB)
	dmd_check(E_DM9275_DM2R_LOAD);
    if (t->tcb_type != TCB_CB)
	dmd_check(E_DM9275_DM2R_LOAD);
#endif
    dmf_svcb->svcb_stat.rec_load++;

    /*	Check for write access. */

    if (rcb->rcb_access_mode != RCB_A_WRITE)
    {
	if(xcb->xcb_x_type & XCB_RONLY)
	    SETDBERR(dberr, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	else
	    SETDBERR(dberr, 0, E_DM0073_RECORD_ACCESS_CONFLICT);
	return (E_DB_ERROR);
    }

    /* if update lock is set in RCB, reset it */
    rcb->rcb_state &= ~RCB_UPD_LOCK;

    /* Tell all that this table has been updated */
    rcb->rcb_state |= RCB_TABLE_UPDATED;

    switch (type & ~DM2R_L_FLOAD)
    {
      case DM2R_L_ADD:

	mct = &lct->lct_mct;
	/* Take an initial run thru the list and handle system maintained
	** or LOB columns.
	*/
	if (t->tcb_rel.relstat & TCB_SYS_MAINTAINED
	  || t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    for (i = 0, data_ptr = record_list; i < *count;
		     i++, data_ptr = data_ptr->data_next)
	    {
		/*
		** First update any system maintained attributes and
		** insert any required security label.
		*/

		put_record = data_ptr->data_address;

		/* Check for system/dmf initialized attributes */

		status = E_DB_OK;
		if (t->tcb_rel.relstat & TCB_SYS_MAINTAINED)
		{
		    u_i4	high;
		    u_i4	low;

		    /* Generate a logical key value */

		    status = dm1c_get_high_low_key(t, rcb, &high, &low, dberr);

		    if (status == E_DB_OK)
		    {
			status = dm1c_sys_init(&rcb->rcb_logkey,
					    t->tcb_atts_ptr,
					    (i4)t->tcb_rel.relatts,
					    put_record, &rcb->rcb_val_logkey,
					    dberr);
		    }
		}

		if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
		{
		    /*
		    **  If there are extensions, then we need to move any
		    **  extension tuples.  This is all managed by the DMPE
		    **  routines.
		    */
		    status = dm1c_pput( t->tcb_atts_ptr,
					rcb,
					put_record,
					dberr);

		    if (DB_FAILURE_MACRO(status))
		    {
			if (dberr->err_code != E_DM0065_USER_INTR
			  && dberr->err_code != E_DM010C_TRAN_ABORTED
			  && dberr->err_code != E_DM016B_LOCK_INTR_FA
			  && dberr->err_code != E_DM5423_SRID_MISMATCH)
			{
			    /*
			    ** User interrupts are now a
			    ** possibility, since dmpe updates may
			    ** make recursive DMF calls to mung
			    ** with objects in the extension
			    ** tables.  Given that that's the
			    ** case, we just pass the error back
			    ** up the chain, and the upper layers
			    ** will handle it correctly.
			    */
	    
			    i4	    local_err_code;
		    
			    uleFormat(dberr, 0,
				       (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				       (char *)NULL, (i4)0, (i4 *)NULL,
				       &local_err_code, 0);
			    SETDBERR(dberr, 0, E_DM9B04_LOAD_DMPE_ERROR);
			}
		    }
		}

		if (status != E_DB_OK)
		{
		    if (dberr->err_code == E_DM9380_TAB_KEY_OVERFLOW)
		    {
			/* error can be returned from dm1c_sys_init(),
			** not logged
			** there because it doesn't have the necessary
			** context for
			** the error parameters.
			*/
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char *)NULL, (i4) 0, (i4 *)NULL,
			    err_code, 3,
			    sizeof(DB_TAB_NAME), &t->tcb_rel.relid,
			    sizeof(DB_OWN_NAME), &t->tcb_rel.relowner,
			    sizeof(DB_DB_NAME), &t->tcb_dcb_ptr->dcb_name);

			SETDBERR(dberr, 0, E_DM9275_DM2R_LOAD);
		    }

		    break;
		}
	    }
	    if (status != E_DB_OK)
		break;
	} /* if prepass */

	/*
	** Give this set of tuples to the sorter.
	** If we are loading a heap table NOSORT, then send these rows directly
	** to the heap build routines.
	*/
	if (! mct->mct_heapdirect )
	{
	    for (i = 0, data_ptr = record_list;
		 i < *count;
		 i++, data_ptr = data_ptr->data_next)
	    {
		++rcb->rcb_s_load;
		put_record = data_ptr->data_address;
		if (t->tcb_rel.relspec == TCB_HASH)
		{
		    /*
		    ** If loading a HASH table, append the hash bucket number to
		    ** the end of the row.  First build the key and then call
		    ** the hash function to find the bucket.  Since putting this
		    ** value at the end of the row requires extra space, first
		    ** we have to copy the row to the lct_record storage space.
		    */

		    status = dm1h_newhash(rcb, mct->mct_key_atts, 
                               mct->mct_keys, put_record, mct->mct_buckets, 
                               &hash_bucket, dberr);

		    if (status != E_DB_OK)
			break;

		    /*
		    ** Write hash bucket to end of lct_record storage space and
		    ** copy the row in front of it.
		    */
		    I4ASSIGN_MACRO(hash_bucket,
			*((i4 *) &lct->lct_record[lct->lct_width]));

		    MEcopy(put_record, lct->lct_width, lct->lct_record);
		    put_record = lct->lct_record;
		}

		status = dmse_put_record(lct->lct_srt, put_record, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	}
	else
	{
	    for (i = 0, data_ptr = record_list; i < *count;
		 i++, data_ptr = data_ptr->data_next)
	    {
		++rcb->rcb_s_load;
		status = dm1sbput(mct, data_ptr->data_address, lct->lct_width,
		    (i4)0, dberr);
		if (status != E_DB_OK)
		    break;
		lct->lct_tupadds++;
	    }
	}

        if (status != E_DB_OK)
	{
	    /* Set count to be the row number the error is on. */
	    *count = i + 1;
	    break;
	}
	return (E_DB_OK);


      case DM2R_L_BEGIN:

        if (type & DM2R_L_FLOAD)
        {
	    /* scb = xcb->xcb_scb_ptr */
	    
            ext = t->tcb_dcb_ptr->dcb_ext;
            size = sizeof(i4) *
                   ((ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4));
	    status = dm0m_allocate((i4)sizeof(DML_LOC_MASKS) + 3*size,
	      DM0M_ZERO, (i4)LOCM_CB, (i4)LOCM_ASCII_ID, (char *)scb,
	      (DM_OBJECT **)&lm, dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(dberr, 0, NULL, ULE_LOG , NULL, (char * )NULL, 0L,
		    (i4 *)NULL, &local_err_code, 0);
		SETDBERR(dberr, 0, E_DM0086_ERROR_OPENING_DB);
		break;
	    }

            lm->locm_bits = size * BITSPERBYTE;
            lm->locm_w_allow = (i4 *)((char *)lm + sizeof(DML_LOC_MASKS));
            lm->locm_w_use   =
                        (i4 *)((char *)lm + sizeof(DML_LOC_MASKS) + size);
            lm->locm_d_allow =
                 (i4 *)((char *)lm + sizeof(DML_LOC_MASKS) + size + size);

            for (i=0; i < ext->ext_count; i++)
            {
                if (ext->ext_entry[i].flags & DCB_WORK)
                {
                    BTset(i, (char *)lm->locm_w_allow);
                    BTset(i, (char *)lm->locm_w_use);
                }

                if (ext->ext_entry[i].flags & DCB_AWORK)
                    BTset(i, (char *)lm->locm_w_allow);

                if (ext->ext_entry[i].flags & DCB_DATA)
                    BTset(i, (char *)lm->locm_d_allow);
            }

        }

	/*
	** Allocate an lct for this LOAD.  Initialize it and its modify
	** control block.
	*/
	sort_width = t->tcb_rel.relwid;
	if (t->tcb_rel.relspec == TCB_HASH)
	    sort_width += 4;

	loc_count = t->tcb_table_io.tbio_loc_count;
	index_cmpcontrol_size = 0;
	leaf_cmpcontrol_size = 0;
	/* Allocate local copies of compression-control arrays, if the
	** (existing) table needs them.  We have to do this because
	** build-begin does a "setup" on the index and leaf rowaccessors,
	** which in particular builds any needed compression-control.
	** If we try to do this on top of a copy of the TCB rowaccessors
	** we'll blow it up.
	** It ought to be possible to simply share the TCB rowaccessors,
	** if we can move the setup out of load-begin;  but BTREE load-
	** begin does a lot of pissing around, and until that can get
	** pushed higher up it will have to stay this way.
	*/
	if (t->tcb_index_rac.cmp_control != NULL)
	{
	    index_cmpcontrol_size = dm1c_cmpcontrol_size(
			t->tcb_index_rac.compression_type,
			t->tcb_index_rac.att_count,
			t->tcb_rel.relversion);
	    index_cmpcontrol_size = DB_ALIGN_MACRO(index_cmpcontrol_size);
	}
	if (t->tcb_leaf_rac.cmp_control != NULL)
	{
	    leaf_cmpcontrol_size = dm1c_cmpcontrol_size(
			t->tcb_leaf_rac.compression_type,
			t->tcb_leaf_rac.att_count,
			t->tcb_rel.relversion);
	    leaf_cmpcontrol_size = DB_ALIGN_MACRO(leaf_cmpcontrol_size);
	}
	status = dm0m_allocate( DB_ALIGN_MACRO(sizeof(DM2R_L_CONTEXT)) +
		DB_ALIGN_MACRO( DB_ALIGN_MACRO(sort_width) + sizeof(i4) ) +
	    ((t->tcb_rel.relatts + 1) * sizeof(DB_CMP_LIST))+
	    index_cmpcontrol_size +
	    leaf_cmpcontrol_size +
            ( (loc_count) * DB_ALIGN_MACRO(sizeof(DMP_LOCATION)) ),
	    (i4)0, (i4)LCT_CB, (i4)LCT_ASCII_ID, (char *)xcb,
	    (DM_OBJECT **)&lct, dberr);
	if (status != E_DB_OK)
	    break;
	t->tcb_lct_ptr = (PTR)lct;
	mct = &lct->lct_mct;

	/*
	**  Initialize pointers to dynamic portions of the
        **  MDFY control block.
	*/
	lct->lct_cmp_list = (DB_CMP_LIST*) ((char *)lct + sizeof(*lct));
	lct->lct_record = (char *)lct->lct_cmp_list +
                           ((t->tcb_rel.relatts + 1) * sizeof(DB_CMP_LIST));
	p = (char *)lct->lct_record + sort_width + sizeof(i4);
	if (index_cmpcontrol_size > 0)
	{
	    mct->mct_index_rac.cmp_control = (PTR) p;
	    mct->mct_index_rac.control_count = index_cmpcontrol_size;
	    p = p + index_cmpcontrol_size;
	}
	if (leaf_cmpcontrol_size > 0)
	{
	    mct->mct_leaf_rac.cmp_control = (PTR) p;
	    mct->mct_leaf_rac.control_count = leaf_cmpcontrol_size;
	    p = p + leaf_cmpcontrol_size;
	}

	mct->mct_location = (DMP_LOCATION *) ME_ALIGN_MACRO(p, sizeof(PTR));

	mct->mct_loc_count = loc_count;
	loc_array = mct->mct_location;
	for (k=0; k < loc_count; k++)
	{
	    STRUCT_ASSIGN_MACRO(t->tcb_table_io.tbio_location_array[k],
				loc_array[k]);
	}
	/*
	** Initialize the MCT and LCT for the load operations.
	*/
	lct->lct_c_count = t->tcb_rel.relkeys;
	lct->lct_width = t->tcb_rel.relwid;
	lct->lct_flags = flag;
	lct->lct_tupadds = 0;
	lct->lct_srt = 0;

	/*
	** Build the compare list for sorter.
	**
	** If table is HASH, build a compare list entry for the hash bucket.
	** Build a compare list entry for each key in the table.
	**
	** If the sorter is to be used to eliminate duplicate records (but
	** not duplicate keys) then add the non-key attributes to the compare
	** list so we can compare on all fields when duplicate checking.
	**
	** If we are loading a user heap table, and we want it sorted, then
	** build a compare list from the non-key attributes since there are
	** no keyed ones.
	*/
	keyatts = t->tcb_key_atts; /* isam, hash */

	sort_keys = 0;
	if (t->tcb_rel.relspec == TCB_HASH)
	{
	    lct->lct_cmp_list[0].cmp_offset = lct->lct_width;
	    lct->lct_cmp_list[0].cmp_type = DB_INT_TYPE;
	    lct->lct_cmp_list[0].cmp_precision = 0;
	    lct->lct_cmp_list[0].cmp_length = 4;
	    lct->lct_cmp_list[0].cmp_direction = 0;
	    lct->lct_cmp_list[0].cmp_collID = -1;
	    sort_keys = 1;
	}
	for (i = 0; i < t->tcb_rel.relkeys; i++, sort_keys++)
	{
	    att = keyatts[i];
	    lct->lct_cmp_list[sort_keys].cmp_offset = att->offset;
	    lct->lct_cmp_list[sort_keys].cmp_precision = att->precision;
	    lct->lct_cmp_list[sort_keys].cmp_type = att->type;
	    lct->lct_cmp_list[sort_keys].cmp_length = att->length;
	    lct->lct_cmp_list[sort_keys].cmp_direction = 0;
	    if ((att->flag & ATT_DESCENDING) == ATT_DESCENDING)
		lct->lct_cmp_list[sort_keys].cmp_direction = 1;
	    lct->lct_cmp_list[sort_keys].cmp_collID = att->collID;
	}

	if (((t->tcb_rel.relspec == TCB_HEAP) && (flag & DM2R_TABLE_EXISTS) &&
		((flag & DM2R_NOSORT) == 0)) ||
	    ((flag & DM2R_ELIMINATE_DUPS) &&
		((t->tcb_rel.relstat & TCB_UNIQUE) == 0)) ||
	    (flag & DM2R_DUPKEY_ERR))
	{
	    for (i = 1; i <= t->tcb_rel.relatts; i++)
	    {
		att = &t->tcb_atts_ptr[i];
		if (att->key == 0)
		{
		    lct->lct_cmp_list[sort_keys].cmp_offset = att->offset;
		    lct->lct_cmp_list[sort_keys].cmp_precision = att->precision;
		    lct->lct_cmp_list[sort_keys].cmp_type = att->type;
		    lct->lct_cmp_list[sort_keys].cmp_length = att->length;
		    lct->lct_cmp_list[sort_keys].cmp_direction = 0;
		    lct->lct_cmp_list[sort_keys].cmp_collID = att->collID;
		    sort_keys++;
		}
	    }
	}

	/* MCT location stuff set above, now do the rest */
    	mct->mct_oldrcb = rcb;
	mct->mct_buildrcb = rcb;
	STRUCT_ASSIGN_MACRO(t->tcb_data_rac, mct->mct_data_rac);
	mct->mct_key_atts = keyatts;
	mct->mct_relwid = t->tcb_rel.relwid;
	mct->mct_index_rac.att_ptrs = t->tcb_index_rac.att_ptrs;
	mct->mct_index_rac.att_count = t->tcb_index_rac.att_count;
	mct->mct_index_rac.compression_type = t->tcb_index_rac.compression_type;
	mct->mct_leaf_rac.att_ptrs = t->tcb_leaf_rac.att_ptrs;
	mct->mct_leaf_rac.att_count = t->tcb_leaf_rac.att_count;
	mct->mct_leaf_rac.compression_type = t->tcb_leaf_rac.compression_type;
	/* Index, leaf rowaccessors are setup by "begin" if needed */
	mct->mct_relpages = 0;
	mct->mct_buffer = (PTR)0;
	mct->mct_keys = t->tcb_keys;
	mct->mct_klen = t->tcb_klen; 
	if (t->tcb_rel.relspec == TCB_BTREE || t->tcb_rel.relspec == TCB_RTREE)
	{
	    mct->mct_kperpage = t->tcb_kperpage;
	    mct->mct_kperleaf = t->tcb_kperleaf;
	}
	else
	{
	    mct->mct_kperpage = t->tcb_kperpage;
	    mct->mct_kperleaf = 0;
	}
	mct->mct_acc_plv = t->tcb_acc_plv;
	mct->mct_index = ((t->tcb_rel.relstat & TCB_INDEX) != 0);
	mct->mct_index_comp = DM1B_INDEX_COMPRESSION(rcb);
	mct->mct_crecord = rcb->rcb_tupbuf;
	mct->mct_segbuf = rcb->rcb_segbuf;
	mct->mct_unique = ((t->tcb_rel.relstat & TCB_UNIQUE) != 0);
	mct->mct_page_type = t->tcb_rel.relpgtype;
        mct->mct_page_size = t->tcb_rel.relpgsize;
	mct->mct_ver_number = t->tcb_rel.relversion;
	mct->mct_seg_rows = t->tcb_seg_rows;

	/* Loading a CLUSTERED primary Btree? */
	mct->mct_clustered = (t->tcb_rel.relstat & TCB_CLUSTERED) != 0;

	/*
	** If user has passed information on how to build the
	** table use it otherwise take the defaults from the
	** system catalogs
	*/
	if ( build_tbl_info)
	{
	    mct->mct_d_fill = build_tbl_info->dm2r_fillfactor;
	    mct->mct_i_fill = build_tbl_info->dm2r_nonleaffill;
	    mct->mct_l_fill = build_tbl_info->dm2r_leaffill;
	    mct->mct_buckets = build_tbl_info->dm2r_hash_buckets;
	    mct->mct_extend = build_tbl_info->dm2r_extend;
	    mct->mct_allocation = build_tbl_info->dm2r_allocation;
	}
	else
	{
	    mct->mct_d_fill = t->tcb_rel.reldfill;
	    mct->mct_i_fill = t->tcb_rel.relifill;
	    mct->mct_l_fill = t->tcb_rel.rellfill;
	    mct->mct_buckets = t->tcb_rel.relprim;
	    mct->mct_extend = t->tcb_rel.relextend;
	    mct->mct_allocation = t->tcb_rel.relallocation;
	}

	/* If Global index, set tidsize to sizeof(DM_TID8) */
	if ( t->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	    mct->mct_tidsize = sizeof(DM_TID8);
	else
	    mct->mct_tidsize = sizeof(DM_TID);

	/*
	** Where possible try and optimise allocation size
	*/
	mct->mct_override_alloc = 0;
	/*
	** Make sure allocation size is large enough to cover the
	** number of hash buckets. Factor in the FHDR and any FMAPS.
	*/
	if ( (t->tcb_rel.relspec == TCB_HASH) &&
	     (mct->mct_allocation < mct->mct_buckets ) )
	{
	    mct->mct_override_alloc =
		mct->mct_buckets + 1  + 
		((mct->mct_buckets/
		DM1P_FSEG_MACRO(mct->mct_page_type, mct->mct_page_size)) + 1);
	
	    /*
	    ** For multi-location tables round accordingly
	    */
	    if ( loc_count > 1 )
	        dm2f_round_up( &mct->mct_override_alloc );
	}

	mct->mct_last_data_pageno = -1;

	/*
	** "heapdirect" loads bypass the sorter.  This can be done when loading
	** to a heap in non-heapsort mode and when duplicate elimination is
	** not needed.
	*/
	mct->mct_heapdirect = (t->tcb_rel.relspec == TCB_HEAP) &&
	    (flag & DM2R_NOSORT) && ((flag & DM2R_ELIMINATE_DUPS) == 0);

	/*
	** Estimate the number of pages this copy will add.
	** This estimate is used below in determining the type of load
	** to execute.
	*/
	tup_per_page = ((dm2u_maxreclen(mct->mct_page_type, mct->mct_page_size) 
			    + DM_VPT_SIZEOF_LINEID_MACRO(mct->mct_page_type)) *
			 mct->mct_d_fill / 100 ) / (mct->mct_relwid +
			    DM_VPT_SIZEOF_LINEID_MACRO(mct->mct_page_type));
	if (tup_per_page == 0)
	    ++tup_per_page;
	if (est_rows == 0)
	    estimated_pages = 0;
	else
	    estimated_pages = (est_rows + tup_per_page - 1) / tup_per_page;

	/*
	** "rebuild" implies that the table structure will be recreated.
	** (the table will get new FHDRs and FMAPs.)  Only heap structures
	** can be loaded in non-rebuild mode.
	**
	** "recreate" means that we will create new versions of the
	** file during this load operation (involving file rename
	** operations, etc.)
	**
	** Most loads are executed in both rebuild and recreate modes.  This
	** means a new load file is created and loaded and then renamed over
	** the existing file at the end of the operation.
	**
	** Exceptions to this rule are:
	**
	**   - Temporary tables are loaded in nonrecreate mode.  The new
	**     load tables are built on top of the existing table structures
	**     using the same file and allocated space.  (Note that temp
	**     tables have no recovery requirements so the old structure does
	**     not need to be saved.)
	**
	**   - Loads to non-empty heap tables must be executed in nonrebuild
	**     and nonrecreate mode.  The new rows are appended to the end
	**     of the existing heap.
	**
	** Empty heaps used to run in either mode depending on row
	** estimate.  Empty-heap load now runs in rebuild/recreate like
	** everything else;  the nonrebuild/nonrecreate load is slow because
	** the table file is being extended O_SYNC, and if the caller row
	** estimate is off, we're sunk.  Very small loads would win in
	** nonrebuild/nonrecreate, but it's impossible to be sure.
	** 
	** (If dm2r_load were clever enough to defer the decision until
	** a few build buffers had been filled, that would be neat.
	** But it isn't.)
	**
	*/
	mct->mct_recreate = TRUE;
	mct->mct_rebuild = TRUE;

	if (t->tcb_temporary == TCB_TEMPORARY)
	    mct->mct_recreate = FALSE;

	/*
	** Non-empty heaps are loaded nonrebuild+nonrecreate.
	*/
	if ((t->tcb_rel.relspec == TCB_HEAP) && (flag & DM2R_EMPTY) == 0)
	{
	    mct->mct_recreate = FALSE;
	    mct->mct_rebuild = FALSE;

	    /*
	    ** Completely rebuild empty temp tables since the old table
	    ** contents are not needed for recovery.
	    */
	    if ((t->tcb_temporary == TCB_TEMPORARY) && (flag & DM2R_EMPTY))
		mct->mct_rebuild = TRUE;
	}


	/*
	** Determine if the table should be in-memory, true if
	**  - Temporary table and still in deffered I/O state
	**  - Temporary table and not rebuilding the table, see comments
	**    above for bug 63511 for more info.
	*/
	mct->mct_inmemory = FALSE;
	/*
	** BUG 73737 this TEMP TABLE could be in FCB_OPEN|FCB_IO as well
	** cater for these conditions
	*/
	if ( (t->tcb_temporary == TCB_TEMPORARY) &&
	   ( ( !(mct->mct_rebuild) ||
	     (t->tcb_table_io.tbio_location_array[0].loc_fcb->fcb_state &
		      (FCB_DEFERRED_IO|FCB_OPEN|FCB_IO)) != 0) ) )
	{
	    mct->mct_inmemory = TRUE;
	}

	/*
	** Get an X table control lock on the table in order to load it.
	** NOTE: for the Datallegro fastload-a-partition change, this would
	** have to be an IX lock, or (better) a partition lock.
	** Don't take a control lock if loading nonrebuild+nonrecreate
	** (the nonempty heap case), ordinary locking suffices and a
	** control lock would prevent insert into t select * from t,
	** using load.
	*/
	if (t->tcb_temporary != TCB_TEMPORARY
	  && (mct->mct_rebuild || mct->mct_recreate))
	{
	    status = dm2t_control(t->tcb_dcb_ptr, t->tcb_rel.reltid.db_tab_base,
		xcb->xcb_lk_id, LK_X, (i4)0, (i4)0, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code > E_DM_INTERNAL)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL,
			(i4)0, (i4 *)NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9288_TBL_CTRL_LOCK);
		}
		break;
	    }
	}

	/*
	** If we are not going to rebuild the table,
	** get last used and last data page numbers now (I think
	** this should happen only for non-empty heap.)
	** These will be used for dmse_begin and for the log write
	** (which may not be done until the sort completes).
	** We only do this if NOT rebuild, otherwise we will use
	** mct_last_used == -1, initialized above.
	** We must do this before the dm0p_close_page() call.
	*/
	if ( ! mct->mct_rebuild )
	{
	    status = dm1p_lastused(rcb, 0, &mct->mct_lastused,
		(DMP_PINFO*)NULL, dberr);	
	    if (status != E_DB_OK)
		break;

	    status = dm1p_lastused(rcb, DM1P_LASTUSED_DATA,
		 &mct->mct_last_data_pageno,  (DMP_PINFO*)NULL, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** If this table has pages in the cache, we have to flush them out
	** since we will be bypassing the buffer manager to load the table.
	** The table may have pages in the cache if it is a user table or
	** a temporary table that has been dumped in order to be reused.
	*/
	if ((t->tcb_temporary != TCB_TEMPORARY) ||
		(rcb->rcb_state & RCB_LRESTART))
	{
	    if (rcb->rcb_data.page)
	    {
		status = dm0p_unfix_page(rcb, DM0P_UNFIX,
		    &rcb->rcb_data, dberr);
		if (status)
		    break;
	    }
	    if (rcb->rcb_other.page)
	    {
		status = dm0p_unfix_page(rcb, DM0P_UNFIX,
		    &rcb->rcb_other, dberr);
		if (status)
		    break;
	    }

	    close_flag = DM0P_CLOSE;
	    if (t->tcb_temporary == TCB_TEMPORARY)
		close_flag = 0;
	    /* If this is a (heap) nonrebuild load, use CLFORCE instead.
	    ** Might be insert into t select from t, and the t orig
	    ** could have a t cache page fixed.  For that case, all
	    ** we really need is for modified pages to be written;
	    ** we'll toss at the end of the load.
	    */
	    if (! mct->mct_rebuild)
		close_flag = DM0P_CLFORCE;
	    status = dm0p_close_page(t, rcb->rcb_lk_id, rcb->rcb_log_id,
					close_flag, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** If the load is going to be done to a temp file (which will be
	** renamed to the user table when the load is complete) create
	** an FCB for the temp file.
	*/
	if (mct->mct_recreate)
	{
	    for (k=0; k < loc_count; k++)
		loc_array[k].loc_fcb = 0;

	    /* Nothing external depends on the filename we build, so tell
	    ** dm2f to make up its own name.  Bulk-load can't be journaled,
	    ** so there is no possibility of DO'ing the load in rollforward
	    ** and getting into the sort of filename trouble that index
	    ** and modify gets into.
	    ** This call only allocates the FCB, no need to compute sync_flag.
	    */
	    status = dm2f_build_fcb(xcb->xcb_lk_id, xcb->xcb_log_id, 
		loc_array, loc_count,
		tbio->tbio_page_size,
		DM2F_ALLOC, DM2F_MOD, &t->tcb_rel.reltid,
		0, 0,
		tbio, 0, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Prepare the sorter to accept the records.
	** If this is a load to a HEAP NOSORT, then instead of preparing the
	** sorter, prepare the heap load routine to begin receiving records.
	**
	** Since the rows are coming from an outside sorce (outside DMF)
	** pass the DMSE_EXTERNAL flag.
	*/
	if (! mct->mct_heapdirect )
	{
	    i4	    sort_flags = DMSE_EXTERNAL;
	    i4	    check_keys = t->tcb_rel.relkeys;

	    if (flag & DM2R_ELIMINATE_DUPS)
		sort_flags |= DMSE_ELIMINATE_DUPS;
	    if (flag & DM2R_NOPARALLEL)
		sort_flags |= DMSE_NOPARALLEL;
	    if (t->tcb_rel.relspec == TCB_HASH &&
		t->tcb_rel.relstat & TCB_UNIQUE &&
		lct->lct_flags & DM2R_DUPKEY_ERR)
	    {
		sort_flags |= DMSE_CHECK_KEYS;
		check_keys++;
	    }
	    if (t->tcb_rel.relspec == TCB_ISAM ||
                t->tcb_rel.relspec == TCB_BTREE ||
                t->tcb_rel.relspec == TCB_RTREE)
		sort_flags |= DMSE_CHECK_KEYS;

	    if (sort_keys == 0)
	    {
		TRdisplay("Load with 0 sort keys (%@)\n");
		SETDBERR(dberr, 0, E_DM9707_SORT_BEGIN);
		break;
	    }
	    status = dmse_begin(sort_flags, 
		&t->tcb_rel.reltid,
		lct->lct_cmp_list,
		sort_keys, check_keys, sort_keys,
		t->tcb_dcb_ptr->dcb_ext->ext_entry,
                (type & DM2R_L_FLOAD) ? lm->locm_w_use :
				scb->scb_loc_masks->locm_w_use,
                (type & DM2R_L_FLOAD) ? lm->locm_bits :
				scb->scb_loc_masks->locm_bits,
		&t->tcb_dcb_ptr->dcb_wrkspill_idx, sort_width, est_rows,
		rcb->rcb_uiptr, xcb->xcb_lk_id, xcb->xcb_log_id, &lct->lct_srt,
		rcb->rcb_collation, rcb->rcb_ucollation, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else
	{
	    /*
	    ** We are bypassing the sorter, log the load and initialize
	    ** the table.  Note that we begin the load with the page after
	    ** the current last data page, thereby wasting a page.  This is
	    ** done so that recovery has an easier time restoring the table to
	    ** its state prior to the load.
	    */
	    if ( (rcb->rcb_logging) && (t->tcb_temporary != TCB_TEMPORARY) )
	    {
        	status = dm2u_ckp_lock(t->tcb_dcb_ptr, &t->tcb_rel.relid,
					&t->tcb_rel.relowner, xcb, dberr);
        	if (status != E_DB_OK)
		    break;

		status = dm0l_load(xcb->xcb_log_id, 0, &t->tcb_rel.reltid,
		    &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    &t->tcb_rel.relloc, (i4) t->tcb_rel.relspec,
		    mct->mct_recreate,       /* ? XXX */
		    mct->mct_last_data_pageno,
		    (LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** If loading in RECREATE mode, create the new file which will be
	    ** loaded and renamed to the user table.
	    */
	    if (mct->mct_recreate)
	    {
		/*
		** Set up file synchronization mode.  Load files are
		** opened DI_USER_SYNC_MASK, and optionally DIRECTIO,
		** and are forced at the end of the load.
		** We can also mark them PRIVATE because they are never
		** written to by more than one thread.
		*/
		if (t->tcb_dcb_ptr->dcb_sync_flag & DCB_NOSYNC_MASK)
		    db_sync_flag = 0;
		else
		    db_sync_flag = DI_USER_SYNC_MASK;
		if (dmf_svcb->svcb_directio_load)
		    db_sync_flag |= DI_DIRECTIO_MASK;
		db_sync_flag |= DI_PRIVATE_MASK;

		status = dm2u_file_create(t->tcb_dcb_ptr, xcb->xcb_log_id,
			    xcb->xcb_lk_id, &t->tcb_rel.reltid, db_sync_flag,
			    mct->mct_location, mct->mct_loc_count,
			    mct->mct_page_size,
			    rcb->rcb_logging, (i4)0, dberr);
		if (status != E_DB_OK)
		    break;
		file1_open = TRUE;
	    }

	    status = dm1sbbegin(mct, dberr);
	    if (status != E_DB_OK)
		break;
	}

	rcb->rcb_state |= RCB_LSTART;
	return (E_DB_OK);

      case DM2R_L_END:

	mct = &lct->lct_mct;
	status = E_DB_OK;
	/*
	**  Now start the load phase.  Open the output file.  Prepare to
	**  read the sorted record stream.
	**
	** If loading a NOSORT HEAP, then the table has already been built
	** as the rows were collected.  Just clean up the build routine.
	**
	** If loading a SORTNC, there's no copying to temp table required,
	** just the input_end call to initiate the merge process.
	*/

	/*
	** If this table has extension tables, then we want to finish
	** up the load which was going on in any of those extensions
	** tables related to the base table.
	*/
	if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    /* If we're ending a load, the query itself must be on the way
	    ** to being over, so just call the query ender.
	    ** (If this is a wrong guess, e.g. ctas in nested DB proc,
	    ** it's not fatal;  we'll just have to rebuild context as
	    ** the query continues.)
	    ** Not an error, don't drop any holding temps.
	    */
	    status = dmpe_query_end(FALSE, FALSE, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** We are done with load, but don't reset RCB_LSTART until
	** we finish reading records from sort in dmrget.
	** The RCB_LSTART flag means there is an LCT buffer to be deallocated
	*/
	if ((status == E_DB_OK) && (lct->lct_flags & DM2R_SORTNC))
	{
	    status = dmse_input_end(lct->lct_srt, dberr);
	    rcb->rcb_state |= RCB_LEND;		/* flag we're done load */
	    return(status);
	}

	if (mct->mct_recreate)
	{
	    /* In recreate mode, set the file sync flags now;  this way,
	    ** they are available for examination even in heapdirect mode
	    ** (which actually creates the file at BEGIN time, not here,
	    ** but we need to examine the sync flag settings here).
	    */
	    if (t->tcb_dcb_ptr->dcb_sync_flag & DCB_NOSYNC_MASK)
		db_sync_flag = 0;
	    else
		db_sync_flag = DI_USER_SYNC_MASK;
	    if (dmf_svcb->svcb_directio_load)
		db_sync_flag |= DI_DIRECTIO_MASK;
	    db_sync_flag |= DI_PRIVATE_MASK;
	}

	for (;;)
	{
	    if (status != E_DB_OK)
		break;

	    if (mct->mct_heapdirect)
	    {
		/* Finish up the build and break to the cleanup code */
		status = dm1sbend(mct, dberr);
		break;
	    }

	    status = dmse_input_end(lct->lct_srt, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** If this is a load to a user table, log the LOAD operation.
	    ** If a recreate, the table will be renamed to its old version
	    ** on backout.  If a rebuild operation, the table will be
	    ** truncated.
	    */
	    if ( rcb->rcb_logging && (t->tcb_temporary != TCB_TEMPORARY) )
	    {
        	status = dm2u_ckp_lock(t->tcb_dcb_ptr, &t->tcb_rel.relid,
					&t->tcb_rel.relowner, xcb, dberr);
        	if (status != E_DB_OK)
		    break;

		status = dm0l_load(xcb->xcb_log_id, 0,
		    &t->tcb_rel.reltid, &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    &t->tcb_rel.relloc, (i4) t->tcb_rel.relspec,
		    mct->mct_recreate, mct->mct_last_data_pageno,
		    (LG_LSN *) 0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** If loading in RECREATE mode, create the new file which will be
	    ** loaded and renamed to the user table.
	    */
	    if (mct->mct_recreate)
	    {
		status = dm2u_file_create(t->tcb_dcb_ptr, xcb->xcb_log_id,
			    xcb->xcb_lk_id, &t->tcb_rel.reltid, db_sync_flag,
			    mct->mct_location, mct->mct_loc_count,
			    mct->mct_page_size,
			    rcb->rcb_logging, (i4)0, dberr);
		if (status != E_DB_OK)
		    break;
		file1_open = TRUE;
	    }

	    switch (t->tcb_rel.relspec)
	    {
	    case TCB_HEAP:

		/*  Prepare to load a HEAP table. */

		status = dm1sbbegin(mct, dberr);
		if (status != E_DB_OK)
		    break;

		for (;;)
		{
		    /*  Get record from the sorter. */

		    status = dmse_get_record(lct->lct_srt, &lct->lct_record,
				dberr);
		    if (status != E_DB_OK)
			break;

		    /*	Add to the heap table. */

		    status = dm1sbput(mct, lct->lct_record, lct->lct_width,
				(i4)0, dberr);
		    if (status != E_DB_OK)
			break;
		    lct->lct_tupadds++;
		}
		if (DB_FAILURE_MACRO(status) && (dberr->err_code != E_DM0055_NONEXT))
		{
		    /* There was an error - break to error handling code */
		    break;
		}

		/* Finish loading a HEAP table. */
		status = dm1sbend(mct, dberr);
		break;

	    case TCB_HASH:

		/*  Prepare to load a HASH table. */

		status = dm1hbbegin(mct, dberr);
		if (status != E_DB_OK)
		    break;

		for (;;)
		{
		    /*  Get record from the sorter. */
		    status = dmse_get_record(lct->lct_srt,
                                         &lct->lct_record, dberr);
		    if (status == E_DB_ERROR)
			break;
		    if (status == E_DB_INFO)
		    {
			SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			status = E_DB_ERROR;
			break;
		    }

		    /*
		    ** Get the hash bucket number for this row.  We extract it
		    ** it from the end of the row where we shoved it when we
		    ** fed the row into the sorter.
		    */
		    I4ASSIGN_MACRO(*((i4 *) &lct->lct_record[lct->lct_width]),
			hash_bucket);

		    /*  Add to the hash table. */

		    status = dm1hbput(mct, (i4) hash_bucket,
			lct->lct_record, lct->lct_width, (i4)0, dberr);
		    if (status != E_DB_OK)
			break;
		    lct->lct_tupadds++;
		}
		if (DB_FAILURE_MACRO(status) && (dberr->err_code != E_DM0055_NONEXT))
		{
		    /* There was an error - break to error handling code */
		    break;
		}

		/* Finish loading a HEAP table. */
		status = dm1hbend(mct, dberr);
		break;

	    case TCB_BTREE:

		/*  Prepare to load a BTREE table. */

		status = dm1bbbegin(mct, dberr);
		if (status != E_DB_OK)
		    break;

		for (;;)
		{
		    /*  Get record from the sorter. */

		    status = dmse_get_record(lct->lct_srt,
                                        &lct->lct_record, dberr);

		    if (status == E_DB_ERROR)
			break;
		    dup = FALSE;
		    if (status == E_DB_INFO)
		    {
			if (mct->mct_unique)
			{
			    if (lct->lct_flags & DM2R_DUPKEY_ERR)
			    {
				SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
				status = E_DB_ERROR;
				break;
			    }
			    continue;
			}
			dup = TRUE;
    		    }

		    /*  Add to the btree table. */

		    status = dm1bbput(mct, lct->lct_record, lct->lct_width,
			dup, dberr);
		    if (status != E_DB_OK)
			break;
		    lct->lct_tupadds++;
		}
		if (DB_FAILURE_MACRO(status) && (dberr->err_code != E_DM0055_NONEXT))
		{
		    /* There was an error - break to error handling code */
		    break;
		}

		/* Finish loading BTREE table. */
		status = dm1bbend(mct, dberr);
		break;

	    case TCB_RTREE:

		/*  Prepare to load an RTREE table. */

		status = dm1mbbegin(mct, dberr);
		if (status != E_DB_OK)
		    break;

		for (;;)
		{
		    /*  Get record from the sorter. */

		    status = dmse_get_record(lct->lct_srt,
                                        &lct->lct_record, dberr);

		    if (status == E_DB_ERROR)
			break;
		    dup = FALSE;
		    if (status == E_DB_INFO)
		    {
			if (mct->mct_unique)
			{
			    if (lct->lct_flags & DM2R_DUPKEY_ERR)
			    {
				SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
				status = E_DB_ERROR;
				break;
			    }
			    continue;
			}
			dup = TRUE;
    		    }

		    /*  Add to the RTREE table. */

		    status = dm1mbput(mct, lct->lct_record, lct->lct_width,
			dup, dberr);
		    if (status != E_DB_OK)
			break;
		    lct->lct_tupadds++;
		}
		if (DB_FAILURE_MACRO(status) && (dberr->err_code != E_DM0055_NONEXT))
		{
		    /* There was an error - break to error handling code */
		    break;
		}

		/* Finish loading RTREE table. */
		status = dm1mbend(mct, dberr);
		break;

	    case TCB_ISAM:

		/*  Prepare to load a ISAM table. */

		status = dm1ibbegin(mct, dberr);
		if (status != E_DB_OK)
		    break;

		for (;;)
		{
		    /*  Get record from the sorter. */
		    status = dmse_get_record(lct->lct_srt,
                                        &lct->lct_record, dberr);

		    if (status == E_DB_ERROR)
			break;
		    dup = FALSE;
		    if (status == E_DB_INFO)
		    {
			if (mct->mct_unique)
			{
			    if (lct->lct_flags & DM2R_DUPKEY_ERR)
			    {
				SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
				status = E_DB_ERROR;
				break;
			    }
			    continue;
			}
			dup = TRUE;
    		    }

		    /*  Add to the isam table. */

		    status = dm1ibput(mct, lct->lct_record, lct->lct_width,
			dup, dberr);
		    if (status != E_DB_OK)
			break;
		    lct->lct_tupadds++;
		}
		if (DB_FAILURE_MACRO(status) && (dberr->err_code != E_DM0055_NONEXT))
		{
		    /* There was an error - break to error handling code */
		    break;
		}

		/* Finish loading ISAM table. */
		status = dm1ibend(mct, dberr);
		break;

	    }
	    break;
	}

	/*
	**  Now perform the cleanup phase.
	*/

	for (;;)
	{
	    if (status != E_DB_OK)
		break;

	    /* Set count to number of rows added */
	    *count = lct->lct_tupadds;

	    if (lct->lct_srt)
	    {
		status = dmse_end(lct->lct_srt, dberr);
		lct->lct_srt = 0;	
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** If RECREATE mode, close the new file and rename it to the
	    ** user table.  Format a new iirelation tuple to describe
	    ** the correct number of pages and tuples in the new file.
	    **
	    ** If not RECREATE, set rcb_tup_adds and rcb_page_adds to show
	    ** the newly added pages and tuples.
	    **
	    ** In any case, write a dmu log for the operation.
	    **
	    ** Note that the dmu operation must also be logged when
	    ** logic is introduced to support "emptytable" loads.
	    */
	    if ( ! mct->mct_rebuild )
	    {
		rcb->rcb_tup_adds += lct->lct_tupadds;
		/* mct-relpages is where the load ended up.  The TCB
		** relpages might be higher if some earlier, larger load
		** was rolled back.  Don't reduce relpages, only increase.
		*/
		if (mct->mct_relpages > t->tcb_rel.relpages)
		    rcb->rcb_page_adds += mct->mct_relpages - t->tcb_rel.relpages;
		if (t->tcb_temporary != TCB_TEMPORARY)
		{
		    /* If multi-server or multi-cache, ensure that other
		    ** servers invalidate old pages from their cache.
		    ** Also, toss any stale pages from our cache.
		    */
		    status = invalidate_load_tab(FALSE, rcb, dberr);
		    if (status != E_DB_OK)
			break;

		}
	    }
            else
	    {
	  	STRUCT_ASSIGN_MACRO(t->tcb_rel, relrecord);
		relrecord.relpages = mct->mct_relpages;
		relrecord.reltups = lct->lct_tupadds;
		relrecord.relprim = mct->mct_prim;
		relrecord.relmain = mct->mct_main;
		relrecord.relfhdr = mct->mct_fhdr_pageno;
		relrecord.relmoddate = TMsecs();

		if (t->tcb_temporary != TCB_TEMPORARY)
		{
		    /*
		    ** Update iirelation, also do invalidate
		    */
		    status = update_load_tab(rcb, &relrecord, dberr);
		    if (status != E_DB_OK)
			break;
		}
		else
		{
		    /*
		    ** Copy the updated iirelation record information
		    ** into the TCB
		    */
		    STRUCT_ASSIGN_MACRO(relrecord, t->tcb_rel);
		}

		if (t->tcb_rel.relstat2 & TCB2_PARTITION)
		{
		    DMP_TCB *master_tcb;
		    DMT_PHYS_PART *pp_ptr;
		    i8 page_delta, tup_delta;

		    /* Updating a partition, roll into pp array too. */
		    master_tcb = t->tcb_pmast_tcb;
		    pp_ptr = &master_tcb->tcb_pp_array[t->tcb_partno];
		    /* restrict reltups and relpages to MAXI4 */
		    tup_delta = (master_tcb->tcb_rel.reltups + 
			    relrecord.reltups - pp_ptr->pp_reltups) > MAXI4 ?
				    0 : relrecord.reltups - pp_ptr->pp_reltups;
		    page_delta = (master_tcb->tcb_rel.relpages + 
			    relrecord.relpages - pp_ptr->pp_relpages) > MAXI4 ?
				    0 : relrecord.relpages - pp_ptr->pp_relpages;
		    pp_ptr->pp_reltups = (i4)relrecord.reltups;
		    pp_ptr->pp_relpages = (i4)relrecord.relpages;
		    /* Roll deltas into master, so that master base counts are
		    ** always the sum of the pp-array counts.
		    */
		    CSadjust_i8counter(&master_tcb->tcb_rel.reltups, tup_delta);
		    CSadjust_i8counter(&master_tcb->tcb_rel.relpages, page_delta);
		}
		t->tcb_tup_adds = 0;
		t->tcb_page_adds = 0;
		rcb->rcb_tup_adds = 0;
		rcb->rcb_page_adds = 0;
		rcb->rcb_reltups = (i4)t->tcb_rel.reltups;
		rcb->rcb_relpages = (i4)t->tcb_rel.relpages;
	    }

	    if (mct->mct_recreate)
	    {
		/*
		** Load operations that involve creating new files
		** are performed with DI_USER_SYNC_MASK, and require
		** forcing files prior to the rename.
		** Make sure we use the mct location array, it's the one
		** with the FCB IO flags set!
		** (Note that files that are neither ordinary sync nor
		** user sync had better be opened sync-on-close, which
		** is the way rollforward does it.  That mode is not
		** safe for online use because it syncs too late.)
		*/
		if (db_sync_flag & DI_USER_SYNC_MASK)
		{
		    status = dm2f_force_file(
			mct->mct_location,
			mct->mct_loc_count, &t->tcb_rel.relid,
			&t->tcb_dcb_ptr->dcb_name, dberr);
		    if (status != E_DB_OK)
			break;
		}
		else if ((db_sync_flag & DI_SYNC_MASK) == 0)
		{
		    /* Presumably sync-on-close, make sure that the
		    ** tbio fcb's show the IO flag for close forcing!
		    */
		    loc_array = mct->mct_location;
		    for (k = 0; k < mct->mct_loc_count; k++)
			t->tcb_table_io.tbio_location_array[k].loc_fcb->fcb_state |=
				(loc_array[k].loc_fcb->fcb_state & (FCB_FLUSH|FCB_IO));
		}

		status = rename_load_tab(rcb,
		    t->tcb_table_io.tbio_location_array,
		    mct->mct_location, mct->mct_loc_count, dberr);
		if (status != E_DB_OK)
		    break;
		status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
			mct->mct_location, mct->mct_loc_count, 
			DM2F_ALLOC, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Log the DMU operation.  This marks a spot in the log
	    ** file to which we can only execute rollback recovery
	    ** once.  If we now issue update statements against the
	    ** newly-created table, we cannot do abort processing
	    ** for those statements once we have begun backing out
	    ** the create.
	    */
	    if ( (rcb->rcb_logging) && (t->tcb_temporary != TCB_TEMPORARY) )
	    {
		status = dm0l_dmu(xcb->xcb_log_id, 0, &t->tcb_rel.reltid,
		    &t->tcb_rel.relid, &t->tcb_rel.relowner,
		    (i4)DM0LLOAD, (LG_LSN *)0, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    if (t->tcb_lct_ptr != 0)
	    {
		if (lct->lct_mct.mct_buffer != (PTR)0)
		{
		    dm0m_deallocate((DM_OBJECT **)&lct->lct_mct.mct_buffer);
		}

		dm0m_deallocate((DM_OBJECT **)&t->tcb_lct_ptr);
	    }

	    rcb->rcb_state &= ~RCB_LSTART;
	    rcb->rcb_state |= RCB_LEND;
	    return(E_DB_OK);

	}

	/* Fall through to error recovery */
    }

    /*
    ** Error recovery and cleanup.
    ** Close sorter and free up load control block.
    ** Close load file.
    ** Release the table control lock.
    */
    *count = 0;
    lct = (DM2R_L_CONTEXT *)t->tcb_lct_ptr;
    if (lct && lct->lct_srt)
    {
	local_status = dmse_end(lct->lct_srt, &local_dberr);

	lct->lct_srt = 0;
    }

    if (file1_open)
    {
	status = dm2f_close_file(mct->mct_location,
                    mct->mct_loc_count, (DM_OBJECT *)rcb, &local_dberr);
	status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
		    mct->mct_location, mct->mct_loc_count, 
		    DM2F_ALLOC, &local_dberr);
    }

    if (lct)
    {
	if (lct->lct_mct.mct_buffer != (PTR)0)
	{
	    dm0m_deallocate((DM_OBJECT **)&lct->lct_mct.mct_buffer);
	}

	dm0m_deallocate((DM_OBJECT **)&t->tcb_lct_ptr);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9275_DM2R_LOAD);
    }

    rcb->rcb_state &= ~(RCB_LSTART | RCB_LEND);
    return (E_DB_ERROR);
}

/*{
** Name: update_load_tab
**
** Description:
**	This routine updates iirelation to reflect the new state
**	of a just-loaded table.
**
**	Since the load will never be rolled forward, the update is
**	not journaled, even if the database is.
**
**	update_load_tab is only called for a REBUILD style load,
**	never for non-rebuild (nonempty heap) loads.
**
** Inputs:
**	rcb				RCB of the table being renamed
**	relrecord			iirelation tuple describing new file,
**				        only fields listed are updated.
**	    .relpages
**	    .reltups
**	    .relprim
**	    .relmain
**	    .relfhdr
**	    .relmoddate
**
** Outputs:
**	err_code			Reason for error return.
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
**	21-nov-1992 (jnash)
**	    Created for reduced logging project from the old
**	    dm2t_rename_file.
**	21-jun-1993 (mikem)
**	    Changed update_load_tab to only update specific fields of the
**	    iirelation tuple from the input relrecord (fields associated with
**	    the physical structure of the table), rather than all fields.
**	    Previously the iirelation value for the logical key was being
**	    updated with the tcb version, which caused duplicate logical keys
**	    after repeated loads to the table.
**	23-aug-1993 (rogerk)
**	    Turned off journaling on the iirelation updates for load operations.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Prior to bumping lock on TX locklist, drop TCB lock on server
**          locklist to Null. Prevents bump clashing with shared lock held
**          on server lock list when running DMCM.
**	03-jan-1997 (nanpr01)
**	    use the #define values rather than hard-coded constants.
**	17-jan-2002 (hayke02)
**	    Update the [min,max]pages values (relmin, relmax) for hash
**	    tables. The relrecord (tcb_rel) values will have been set in
**	    start_load() if values have been specified in a copy. Subsequent
**	    unloaddb/copydb runs will retain these [min,max]pages values
**	    in the copy statement. This change fixes bug 106799.
**	04-Apr-2008 (jonj)
**	    Replace dm2t_convert_tcb_lock() with more robust
**	    dm2t_bumplock_tcb().
**	1-Mar-2008 (kschendel) SIR 122890
**	    Extract out TCB invalidation stuff.
**	    Use TCB tid to access iirelation row directly, the way that
**	    update-rel in dm2t does it.  I've been meaning to make this
**	    change for YEARS now and kept forgetting about it!
*/
static DB_STATUS
update_load_tab(
DMP_RCB		*rcb,
DMP_RELATION	*relrecord,
DB_ERROR	*dberr)
{
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DML_XCB		*xcb = rcb->rcb_xcb_ptr;
    DMP_DCB		*dcb = t->tcb_dcb_ptr;
    DMP_RCB		*rel_rcb;
    DMP_RELATION	rel;
    DB_TAB_ID		table_id;
    DB_TAB_TIMESTAMP	timestamp;
    DB_STATUS		status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    do
    {
	/*
	** Open IIRELATION and update the record describing this table.
	*/
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
	status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
	    DM2T_A_WRITE, (i4)0, (i4)20, (i4)0,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, DM2T_S,
	    &xcb->xcb_tran_id, &timestamp, &rel_rcb, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    rel_rcb->rcb_logging = 0;

	/*
	** Turn off journaling for this update.
	** Since the operation was a LOAD, we know that it will never be
	** rolled forward.
	*/
	rel_rcb->rcb_journal = 0;

	status = dm2r_get(rel_rcb, &t->tcb_iirel_tid, DM2R_BYTID,
		    (char *) &rel, dberr);
	if (status != E_DB_OK)
	    break;
	/* Double-check that we got the proper record */
	if (rel.reltid.db_tab_base != t->tcb_rel.reltid.db_tab_base
	  || rel.reltid.db_tab_index != t->tcb_rel.reltid.db_tab_index)
	{
	    TRdisplay("%@ (dm2r)update_load_tab expected (%d,%d) at %d, got (%d,%d)\n",
			t->tcb_rel.reltid.db_tab_base,
			t->tcb_rel.reltid.db_tab_index,
			t->tcb_iirel_tid.tid_i4,
			rel.reltid.db_tab_base, rel.reltid.db_tab_index);
	    SETDBERR(dberr, 0, E_DM9272_REL_UPDATE_ERROR);
	    status = E_DB_ERROR;
	    break;
	}

	rel.relpages = relrecord->relpages;
	rel.reltups = relrecord->reltups;
	rel.relprim = relrecord->relprim;
	rel.relmain = relrecord->relmain;
	rel.relfhdr = relrecord->relfhdr;
	rel.relmoddate = relrecord->relmoddate;
	if (t->tcb_rel.relspec == TCB_HASH)
	{
	    rel.relmin = relrecord->relmin;
	    rel.relmax = relrecord->relmax;
	}

	status = dm2r_replace(rel_rcb, &t->tcb_iirel_tid,
		DM2R_BYTID | DM2R_RETURNTID,
		(char *)&rel, (char *)NULL, dberr);
	if (status != E_DB_OK)
	    break;

	MEcopy((char *)relrecord, sizeof(DMP_RELATION), (char *)&t->tcb_rel);

	status = dm2t_close(rel_rcb, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Bump the lock value for the TCB so that other openers
	** will rebuild their TCB's.
	*/
	status = invalidate_load_tab(TRUE, rcb, dberr);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    } while (0);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, (char *)NULL, 0L,
	    (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C19_UPDATE_LOAD);
    }
    return (E_DB_ERROR);
}

/*{
** Name: rename_load_tab
**
** Description:
**	This routine renames files during a dm2r_load operation.  The
**	file renames are logged and new files opened.
**
** Inputs:
**	rcb				Pointer to rcb
**	old_loc 			Pointer to old location information
**	mod_loc 			Pointer to new location information
**	loc_count			Number of locations
**
** Outputs:
**	err_code			Error info
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	21-nov-1992 (jnash)
**	    Created for reduced logging project from the old
**	    dm2t_rename_file.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-may-1994 (mikem)
**	    bug #63259
**	    Make sure the "mod_loc" file in rename_load_tab() is closed before
**	    it is renamed to the "new" file.
**	    In the case of copying into an empty structured
**	    table, previous to this change, modify files would be left "opened."
**	    On unix this would mean that even if the tables were destroyed,
**	    disk space associated with the tables would not be reclaimed until
**	    either the server was shut down or the slave system would LRU close
**	    the file descriptor.
**      27-dec-94 (cohmi01)
**          For RAW I/O, pass location ptr to dm2f_rename().
**	    Save additional info in XCCB, for identifying correct raw extent.
**	22-feb-95 (cohmi01)
**	    Adjust setup of xccb for old_loc info, for RAW locations.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_open_file()
**	06-jun-1997 (nanpr01)
**	    Initialize the new fields in xccb structure.
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
**	23-Jul-2008 (kschendel) SIR 122757
**	    Make the decision on direct IO here, not in DI.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Pending file-delete XCCB's must go on session list.
*/
static DB_STATUS
rename_load_tab(
DMP_RCB		*rcb,
DMP_LOCATION	*old_loc,
DMP_LOCATION	*mod_loc,
i4		loc_count,
DB_ERROR	*dberr)
{
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DML_XCB		*xcb = rcb->rcb_xcb_ptr;
    DMP_DCB		*dcb = t->tcb_dcb_ptr;
    DML_XCCB		*xccb;
    DM_FILE		old_filename;
    DM_FILE		del_filename;
    LG_LSN		lsn;
    DB_STATUS		status;
    i4		k;
    u_i4		db_sync_flag;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** Close the old file and rename it to the delete file.
	*/
	status = dm2f_close_file(old_loc, loc_count, (DM_OBJECT *)rcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Close the "modify" file in preparation for renaming it to the
	** actual user file (rename should only be run on "closed" files.
	*/
	status =
	    dm2f_close_file(mod_loc, loc_count, (DM_OBJECT *)rcb, dberr);

	if (status != E_DB_OK)
	    break;

	for (k=0; k < loc_count; k++)
	{
	    DMP_TABLE_IO  *tbio = &t->tcb_table_io;

	    /*
	    ** Swap the new files with the ones which currently make up
	    ** the table.  Rename the current files to be delete files and
	    ** put them on the delete queue.
	    */

	    /*
	    ** Generate delete filename for this location.
	    */
	    dm2f_filename(DM2F_DES, &t->tcb_rel.reltid,
		old_loc[k].loc_id, 0, 0, &del_filename);

	    /*
	    ** Save user table filename.
	    */
	    STRUCT_ASSIGN_MACRO(old_loc[k].loc_fcb->fcb_filename, old_filename);

	    if ((old_loc[k].loc_status & LOC_RAW) == 0)
	    {
		DML_XCB *ses_xcb;

	        /*
	        ** Log rename operation (if not NOLOGGING).
	        **	    old_filename -> del_filename
	        **	    mod_filename -> old_filename
	        */
	        if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	        {
		    /*
		    ** Log the rename of the old to the delete
		    */
		    status = dm0l_frename(xcb->xcb_log_id, (i4)0,
		        &old_loc[k].loc_ext->physical,
		        old_loc[k].loc_ext->phys_length,
		        &old_loc[k].loc_fcb->fcb_filename,
		        &del_filename, &t->tcb_rel.reltid,
		        k, tbio->tbio_location_array[k].loc_config_id,
		        (LG_LSN *)0, &lsn, dberr);
		    if (status != E_DB_OK)
			break;
	        }

	        /*
	        ** Rename old table to delete table.
	        */
	        status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id,
		    &old_loc[k].loc_ext->physical,
		    old_loc[k].loc_ext->phys_length,
		    &old_loc[k].loc_fcb->fcb_filename,
                    (i4)sizeof(old_loc[k].loc_fcb->fcb_filename),
		    &del_filename, (i4)sizeof(del_filename),
		    &dcb->dcb_name, dberr);
	        if (status != E_DB_OK)
		    break;

	        /*
	        ** Pend the old file for delete at commit time.
		** This XCCB must be attached to the SESSION's XCB,
		** even if we're in a child thread.  Otherwise the delete
		** will happen at disconnect time instead of commit time.
		** Which would be bad.
	        */
		ses_xcb = xcb->xcb_odcb_ptr->odcb_scb_ptr->scb_x_next;
	        status = dm0m_allocate((i4)sizeof(DML_XCCB),
		    DM0M_ZERO | DM0M_LONGTERM,
		    (i4)XCCB_CB, (i4)XCCB_ASCII_ID, (char *)ses_xcb,
		    (DM_OBJECT **)&xccb, dberr);
	        if (status != E_DB_OK)
		    break;

	        xccb->xccb_f_len = sizeof(del_filename);
	        xccb->xccb_p_len = old_loc[k].loc_ext->phys_length;
	        STRUCT_ASSIGN_MACRO(del_filename, xccb->xccb_f_name);
	        STRUCT_ASSIGN_MACRO(old_loc[k].loc_ext->physical,
					xccb->xccb_p_name);
		dm0s_mlock(&ses_xcb->xcb_cq_mutex);
	        xccb->xccb_q_next = ses_xcb->xcb_cq_next;
	        xccb->xccb_q_prev = (DML_XCCB*)&ses_xcb->xcb_cq_next;
	        ses_xcb->xcb_cq_next->xccb_q_prev = xccb;
	        ses_xcb->xcb_cq_next = xccb;
	        xccb->xccb_xcb_ptr = ses_xcb;
	        xccb->xccb_sp_id = ses_xcb->xcb_sp_id;
	        xccb->xccb_operation = XCCB_F_DELETE;
		dm0s_munlock(&ses_xcb->xcb_cq_mutex);
	    }

	    if ((mod_loc[k].loc_status & LOC_RAW) == 0)
	    {
	        if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	        {
		    /*
		    ** Log the rename of the mod to the user
		    */
		    status = dm0l_frename(xcb->xcb_log_id, (i4)0,
		        &mod_loc[k].loc_ext->physical,
		        mod_loc[k].loc_ext->phys_length,
		        &mod_loc[k].loc_fcb->fcb_filename,
		        &old_filename, &t->tcb_rel.reltid,
		        k, tbio->tbio_location_array[k].loc_config_id,
		        (LG_LSN *)0, &lsn, dberr);
		    if (status != E_DB_OK)
			break;
	        }

	        /*
	        ** Rename the newly created table to the user table name.
	        */
	        status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id,
		    &mod_loc[k].loc_ext->physical, 
		    mod_loc[k].loc_ext->phys_length,
		    &mod_loc[k].loc_fcb->fcb_filename,
                    (i4)sizeof(mod_loc[k].loc_fcb->fcb_filename),
		    &old_filename, (i4)sizeof(old_filename),
		    &dcb->dcb_name, dberr);
	        if (status != E_DB_OK)
	    	    break;
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** Open the new file into the old FCB.  Now the TCB's file descriptor
	** will have the opened new file.
	*/

	db_sync_flag = DI_SYNC_MASK;
	if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
	    db_sync_flag = 0;
	else if (dmf_svcb->svcb_directio_tables)
	    db_sync_flag |= DI_DIRECTIO_MASK;

	status = dm2f_open_file(xcb->xcb_lk_id, xcb->xcb_log_id, old_loc, 
			loc_count, (i4)t->tcb_rel.relpgsize,
		    	DM2F_A_WRITE, db_sync_flag, (DM_OBJECT *)rcb, dberr);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
        (VOID) uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
            (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9C1A_RENAME_LOAD);
    }

    return (E_DB_ERROR);
}


/*{
** Name: invalidate_load_tab
**
** Description:
**	This routine ensures that other servers in a multi-server
**	or multi-cache environment see the revised edition of the
**	table.  This is done by bumping the lock values for the
**	locks on cached table pages (for multi-cache), and the
**	lock on the TCB itself.  Other servers will see the updated
**	values and invalidate any pages or TCB's they have.
**
** Inputs:
**	rcb				RCB of the table being loaded
**
** Outputs:
**	err_code			Reason for error return.
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
**	17-Nov-2009 (kschendel) SIR 122890
**	    Extract from update-load-tab so that it can be called separately
**	    in the case of LOADs into existing heaps.
*/
static DB_STATUS
invalidate_load_tab(
i4		is_rebuild,
DMP_RCB		*rcb,
DB_ERROR	*dberr)
{
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    DMP_DCB		*dcb = t->tcb_dcb_ptr;
    DB_STATUS		status;
    i4			local_err_code;

    for (;;)
    {
	/* A non-rebuild LOAD might be part of an insert into t
	** select from t, and there might be pages of t in the DMF cache.
	** We need to ensure that no pages remain in the cache that
	** belong to the table.  Particularly troublesome would
	** be cached copies of the original last-data page, and
	** cached fmap/fhdr pages;  all of which we've written
	** during the load, bypassing the cache.  Since we've
	** held an X table (though not control) lock since tossing
	** pages at the start of the load, just invalidate,
	** no need to worry about modified pages.
	** Since we're at the end, and since orig nodes kindly
	** unfix the last page at eof, we should not see fixed
	** pages here.
	**
	** Do this before bumplock so that the TCB stamp matches the cache.
	**
	** If it was rebuild mode, we tossed the cache before starting
	** the load, and took a control lock;  no need to toss again here.
	*/
	if (! is_rebuild)
	{
	    status = dm0p_close_page(t, rcb->rcb_lk_id, rcb->rcb_log_id,
			    0, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** If this table can be opened by a server using a different
	** buffer manager then call dm0p_tblcache to invalidate any
	** pages cached by other servers on this table.
	*/
	if (dcb->dcb_bm_served == DCB_MULTIPLE)
	{
	    status = dm0p_tblcache(dcb->dcb_id, t->tcb_rel.reltid.db_tab_base,
		DM0P_MODIFY, rcb->rcb_lk_id, rcb->rcb_log_id, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/* If this table can be opened by another server, bump the
	** TCB lock value so that those other servers will rebuild
	** their TCB's.
	*/
	if (dcb->dcb_served == DCB_MULTIPLE)
	{
	    status = dm2t_bumplock_tcb(dcb, t, (DB_TAB_ID*)NULL, (i4)1, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Bump our TCB's stamp by 1 also, since we have no desire to
	    ** invalidate our own TCB unnecessarily!  Do this after tossing
	    ** the cache, so there's no cache-vs-TCB stamp mismatch.
	    */
	    ++ t->tcb_table_io.tbio_cache_valid_stamp;
	}

	return (E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, (char *)NULL, 0L,
	    (i4 *)NULL, &local_err_code, 0);
	SETDBERR(dberr, 0, E_DM9C19_UPDATE_LOAD);
    }
    return (E_DB_ERROR);
} /* invalidate_load_tab */

/*{
** Name: dm2r_schedule_prefetch - Queue up prefetch request for Readahead thread
**
** Description:
**	This routine queues up a request to read some specified data, this
**	request will be executed if possible by a readahead thread. The
**	request will not get executed if the readahead threads never get
**	dispatched (they run at the lowest priority) or if the request
**	becomes marked as stale by dm2r_cancel_prefetch, or if the readahead
**	thread otherwise detects that the request is stale.
**
**	This routine should only be called if it is known that readahead
**	threads are available, and only if it makes sense. For example,
**	only when enough overlap is possible due to user-think-time as in
**	cursor operation, or in the case of secondary index requests, if
**	enough sec indexes exist that user threads and RA threads can
**	overlap their I/O operations.
**
** Inputs:
**	rcb				Pointer to rcb
**	prefet_type 			Indicator of type of request, eg:
**					PFR_PREFET_SECIDX
**					    prefetch sec index entries for tup.
**					PFR_PREALLOC_SECIDX
**					    read in sec index pages needed
**					    for insertion of new tup.
**					PFR_PREFET_PAGE
**					    read a particular page.
**
** Outputs:
**	    none
**
** Returns:
**	    none
**
** Exceptions:
**	    none
**
** Side Effects:
**	- Readahead mutex taken/released
**	- One Prefetch request block may be reserved for request.
**	- items in svcb-> prefetch header updated.
**
** History:
**	09-May-1995 (cohmi01)
**	    Created.
**	17-Jul-1995 (cohmi01)
**	    Simplify queue management. Init array of interrupt indicators in
**	    prefetch request. RA thread's rcb_uiptr will be pointed at these,
**	    to cause dm0p_fix_page to abort without starting I/O.
**	10-Jan-2000 (jenjo02)
**	    Replaced direct reference to obsolete bm_fcount with 
**	    DM0P_COUNT_QUEUE macro.
*/

#ifdef NEVER_USED
static void
dm2r_schedule_prefetch(
DMP_RCB		*r,
i4		prefet_type)
{
    DMP_PFH        *dm_prefet  = dmf_svcb->svcb_prefetch_hdr;
    DMP_PFREQ	   *pfreq;
    i4 		   count, fcount, pgsize;

    /* see if the system has enough free buffers to handle readahead */
    pgsize = r->rcb_tcb_ptr->tcb_rel.relpgsize;
    DM0P_COUNT_QUEUE(dmf_svcb->svcb_lbmcb_ptr->lbm_bmcb
	[DM_CACHE_IX(pgsize)]->bm_fpq, fcount);
    if (dm0p_buffers_full(pgsize, fcount))
    {
	dm_prefet->pfh_bufstress++;	/* just a statistic, no mutex needed */
	return;
    }	

    /* get prefetch queue mutex  */
    dm0s_mlock(&dm_prefet->pfh_mutex);

    /* see if there are any free blocks available */
    if (dm_prefet->pfh_numsched >= dm_prefet->pfh_numreqs)
    {
    	dm_prefet->pfh_no_blocks++;
    	dm0s_munlock(&dm_prefet->pfh_mutex);
	return;
    }

    /* scan thru list looking for free one. Start at 'freehdr' as thats   */
    /* where free ones will be placed by the RA thread, but do exhaustive */
    /* search as there may be holes, even if freehdr isnt free anymore    */
    for (pfreq = dm_prefet->pfh_freehdr, count = dm_prefet->pfh_numreqs;
	count--; pfreq = pfreq->pfr_next)
    {
	if ((pfreq->pfr_status & PFR_FREE) ||
	    (pfreq->pfr_next == dm_prefet->pfh_freehdr))
	    break;
    }

    /* make sure loop ended because we found a free one */
    if (! (pfreq->pfr_status & PFR_FREE))
    {
	/* this is really a logic error, as numsched indicates free */
    	dm_prefet->pfh_no_blocks++;
    	dm0s_munlock(&dm_prefet->pfh_mutex);
	return;
    }

    /* set up a prefetch event */
    pfreq->pfr_status &= ~PFR_FREE;	/* this block no longer free */
    pfreq->pfr_type = prefet_type;	/* type of operation to perform */
    pfreq->t.si.rcb = r;		/* rcb this request is for */
    pfreq->pfr_pincount = 0;		/* no threads processing it yet */
    MEfill(sizeof(pfreq->pfr_intrupt), '\0', (PTR)  pfreq->pfr_intrupt);

    pfreq->t.si.totix =
	r->rcb_tcb_ptr->tcb_index_count; /* total # of ix to process  */

    /* this item will constantly change to indicate where userthread is upto. */
    /* Initial setting depends on whether we anticipate thinktime for overlap */
    if (prefet_type == PFR_PREFET_SECIDX)
    	pfreq->t.si.userix = PFR_PREFET_MIN;
    else if (prefet_type == PFR_PREALLOC_SECIDX)
	pfreq->t.si.userix = PFR_PREALLOC_MIN;
    else
	pfreq->t.si.userix = 65535;	/* prefet_type ??  */

    /* indicate next ix# for RA thread, starts from end of list of indexes */
    pfreq->t.si.raix = pfreq->t.si.totix;
    STRUCT_ASSIGN_MACRO(r->rcb_tcb_ptr->tcb_rel.reltid, pfreq->pfr_table_id);


    /* signal the prefetch event */
    if (dm_prefet->pfh_status & DMP_PFWAIT)
    {
	dm_prefet->pfh_status &= ~DMP_PFWAIT;
	dm0s_erelease(r->rcb_lk_id, DM0S_READAHEAD_EVENT,dmf_svcb->svcb_id);
    }	
    dm_prefet->pfh_scheduled++;
    dm_prefet->pfh_numsched++;
    dm_prefet->pfh_freehdr = pfreq->pfr_next; /* advance to next (free?) item */

    r->rcb_state |= RCB_PREFET_PENDING;

    /* release mutex */
    dm0s_munlock(&dm_prefet->pfh_mutex);

    /* All done, no return code needed, caller doesnt care about us */
    return;
}
#endif /* NEVER_USED */


/*{
** Name: dm2r_cancel_prefetch - Cancel a previously queued up prefetch request
**
** Description:
**	This routine cancels a prefetch request that was previously queued
**	up by a call to dm2r_schedule_prefetch(). It can cancel an entire
**	request, or, for those requests consisting of multiple items, (eg.
**	secondary indexes) it can cancel any individual item. Since the
**	readahead threads are asynchronously processing the request, the
**	'cancel' is not necessarily immediate.  The items of the request
**	may be in any of the following states, without regard to their order
**	in the tcb list:
**	- Already done - no action needed.
**	- Being executed - this means that the RA thread has already called
**	  a dm2r_xxx or lower routine to process the request. In order
**	  to cancel such an item, we set a word in an interrupt array
**	  subscripted by the item number. The RA thread had pointed the
**	  rcb's rcb_uiptr to this word in the array. dm0p_fix_page will
**	  detect that it has been set, and return accordingly when RA thread
**	  running this item gets dispatched. So at worse, the cancel will
**	  lag only by one I/O that dm0p_fix_page has already requested.
**	- Execution not yet started - RA thread did not yet call a dm2r_xxx
**	  routine. Setting the 'userix' field in the request block will
**	  let RA thread know that user is upto this item, and not to bother
**	  executing it.
**
**	Much of the goal of the above is to allow RA threads to do as much
**	I/O in advance of the or concurrently with the user thread as possible,
**	but without ending up with IOWAITs (buf busy), or with the RA thread
**	reading items already handled by user thread.
**	
**
**	This routine should only be called if it is known that a prefetch
**	request is pending on an RCB, as indicated by the RCB_PREFET_PENDING
**	bit in rcb_state.
**
** Inputs:
**	rcb				Pointer to rcb
**	item 				Item number to cancel, or
**					PFR_CANCEL_ALL
**					PFR_CANCEL_IX
** Outputs:
**	    none
**
** Returns:
**	    none
**
** Exceptions:
**	    none
**
** Side Effects:
**	- Readahead mutex taken/released
**	- One or more Prefetch request blocks may be  updated.
**	- items in svcb-> prefetch header updated.
**
** History:
**	09-May-1995 (cohmi01)
**	    Created.
**	17-Jul-1995 (cohmi01)
**	    Simplify queue management. Utilize rcb_uiptr interrupt mechanism
**	    to cause dm0p_fix_page to abort without starting I/O.
**      12-Feb-1999 (kosma01)
**          Changed second argument of dm2r_cancel_prefetch() to be a
**          i4 instead of u_i2. In dm2r_cancel_prefetch(), the
**          second argument, item, is compared against two negative
**          constants.
*/

#ifdef NEVER_USED
static void
dm2r_cancel_prefetch(
DMP_RCB		*r,
i4		item)
{
    DMP_PFH   	   *dm_prefet  = dmf_svcb->svcb_prefetch_hdr;
    DMP_PFREQ	   *pf;
    u_i2	   i;

    /* get prefetch queue mutex  */
    dm0s_mlock(&dm_prefet->pfh_mutex);

    /* if no schduled request blocks, return without doing anything */
    if (! dm_prefet->pfh_numsched)
    {
    	dm0s_munlock(&dm_prefet->pfh_mutex);
	r->rcb_state &= ~RCB_PREFET_PENDING;
	return;
    }

    /* scan all non-free requests for ones related to this rcb */
    for (pf = dm_prefet->pfh_next;  ;pf = pf->pfr_next)
    {
	if ((! (pf->pfr_status & PFR_FREE)) && pf->t.si.rcb == r)
	{
	    /* This request is for the RCB whose requests we wish to cancel. */

	    /* Cancel as per the value of 'item'.			     */
	    if ((item == PFR_CANCEL_ALL) ||
		(item == PFR_CANCEL_IX &&
		    (pf->pfr_type & (PFR_PREFET_SECIDX | PFR_PREALLOC_SECIDX))))
	    {
		/* this entire prefetch request must be cancelled */
		pf->pfr_status |= PFR_STALE_REQ;
		r->rcb_state &= ~RCB_PREFET_PENDING;
    		dm_prefet->pfh_cancelled++;
		/* Indicate interrupt for each possible active ix rcb */
		for (i = 0; i < pf->t.si.totix && i < PFR_MAX_ITEMS; i++)
		    pf->pfr_intrupt[i]  |= RCB_RAHEAD_INTR;
	    }
	    else
	    if (pf->pfr_type & (PFR_PREFET_SECIDX | PFR_PREALLOC_SECIDX))
	    {
		if (item >= pf->t.si.raix)
		{
		    /* user is upto the ix that RA thread would do next when */
		    /* it wakes up, so tell RA not to bother. Could just set */
		    /* userix here, but we wish to un-pend the RCB as soon   */
		    /* as possible, without waiting for RA thread.	     */
		    pf->pfr_status |= PFR_STALE_REQ;
                    dm_prefet->pfh_lagbehind++;
		}
		else
		    pf->t.si.userix = item;	/* let RA thread know limit */

		/* either way, set interrupt for current item being cancelled */
		if (item <= PFR_MAX_ITEMS)
		    pf->pfr_intrupt[item -1]  |= RCB_RAHEAD_INTR;
	    }
	}

	/* check for loop termination if back to start of queue */
	if (pf->pfr_next == dm_prefet->pfh_next)
	    break;
    }	


    /* release mutex */
    dm0s_munlock(&dm_prefet->pfh_mutex);
}
#endif /* NEVER_USED */


/*{
** Name: si_put - Put record into secondary index.
**
** Description:
**      After inserting a row into a base table, this function is called
**      to insert the corresponding secondary index entry.
**
**
** Inputs:
**      tcb                             TCB of secondary index
**      rcb                             RCB of base table with
**					rcb_si_newtid holding the
**					base table's TID.
**      record                          Pointer to (base table) record.
**      flag 				DM2R_PUT if real "put"
**					DM2R_REPLACE if called from
**					si_replace; in this case,
**					"rcb" will be that of SI.
**
** Outputs:
**	err_code			Reason for error return status.
**
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
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Moved index 'put' code to its own routine
**      29-jul-97 (stial01)
**          si_put() Replaced rcb_update parameter with opflag parameter.
**          Pass original operation mode 'opflag' to dm1*_put for logging.
**	30-mar-03 (wanfr01)
**          Bug 109914, INGSRV 2156
**	    Add DM1C_DUP_NONKEY flag to indicate si_dupcheck did not need
**          to perform duplicate checking (so no control lock grabbed)
**	22-Jan-2004 (jenjo02)
**	    Modified to be callable from the mainline code or from
**	    a parallel SI Agent.
**	    Added Global SI, TID8 support.
**	07-Apr-2004 (jenjo02)
**	    Repair wanfr01's DM1C_DUP_NONKEY logic for non-key updates.
**	    DM1C_DUP_NONKEY applied to all indexes, not just those
**	    with non-key updates. Added RCB_SI_VALUE_LOCKED instead,
**	    redo the check for non-key updates when there's no
**	    persistent RCB to contain that flag.
**	    Fix silently lost "put" from si_replace when row 
**	    locking.
**	12-Apr-2004 (jenjo02)
**	    Tweak to last fix: value lock not held if duplicate
**	    checking was not done (i.e., non-UNIQUE index).
*/
static DB_STATUS
si_put(
    DMP_TCB	*it,
    DMP_RCB     *r,
    char        *record,
    i4		flag,
    DB_ERROR 	*dberr )
{
    DMP_RCB		*ir;
    DMR_RTB	    	*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB		*t;
    i4			alloc_flag;
    DB_STATUS           status, rtb_status;
    DM_TID              bid;
    DM_TID8		new_tid8;
    i4             	i;
    char                *rec;
    char		*crec = (PTR)0;
    i4             	rec_len;
    i4			si_retries = 0;
    char    		*raat_retry;
    i4			opflag;
    i4			loc_err;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = E_DB_OK;
    rtb_status = E_DB_OK;

    /*
    ** If called from an SIAgent or from si_replace,
    ** "r" is the index's RCB.
    */
    if ( flag & (DM2R_SI_AGENT | DM2R_MREPLACE) )
    {
	ir = r;
	r  = ir->rcb_base_rcb;
	if ( flag & DM2R_SI_AGENT )
	    rtb_status = rtb->rtb_status;
    }
    else
	ir = (DMP_RCB*)NULL;

    if ( flag & DM2R_MPUT )
    {
	opflag = DM1C_MPUT;

	if ( rtb_status == E_DB_OK )
	{
	    /* Allocate an RCB for the SI */
	    if ( ir == (DMP_RCB*)NULL )
		status = dm2r_rcb_allocate(it, r, &r->rcb_tran_id,
					    r->rcb_lk_id,
					    r->rcb_log_id,
					    &ir, dberr);

	    /* If RAAT, get the retry count */
	    if ( status == E_DB_OK && flag & DM2R_RAAT )
	    {
	       NMgtAt("II_RAAT_RETRY", &raat_retry);
	       if (raat_retry && *raat_retry ) 
		  si_retries = atoi(raat_retry);
	       if (si_retries < 1) 
		  si_retries = 0;
	    } 
	}
    }
    else if ( flag & DM2R_MREPLACE )
    {
	opflag = DM1C_MREPLACE;

	/*
	** Called from inside si_replace, which has 
	** already acquired a SI RCB ("r")
	** and is inside a RAAT retry loop, if needed.
	*/

	if ( rtb_status )
	{
	    /* Global threading error, NOOP the "put" */
	    ir = (DMP_RCB*)NULL;
	}
    }
    else
	dmd_check(E_DM9314_DM2R_PUT);

    /* The Base TCB */
    t = r->rcb_tcb_ptr;
       

    /* If a previous operation on the TBIO for this index raised
    ** a 'check_high_water_mark' exception, then process it now,
    ** so that this and future operations against this index wont
    ** be prevented from making use of those optimizations that
    ** require a proper EOF (hwm) to be known. One such operation
    ** is use of groups buffers, which can now be used for secondary
    ** indexes in certain cases (batch).
    */
    if ( ir && ir->rcb_tcb_ptr->tcb_table_io.tbio_checkeof )
	dm1p_checkhwm(ir, dberr);

    if ( ir && rtb_status == E_DB_OK )
	do
    {
	status = E_DB_OK;

	/*
	** If non-threaded REPLACE and row-locking, 
	** we have to repeat this check,
	** which was already done by si_dupcheck, but since we
	** tossed the si_dupcheck RCB, we don't know whether this
	** is a "non-key" update.
	**
	** If it's a non-key update, then si_dupcheck didn't
	** acquire a value lock for the row.
	**
	******
	** Check to set if any keyed attribute in this index changed.
	** If one did, then we have to make sure that the new secondary
	** index entry will not duplicate an already existing entry.
	**
	** If only non-keyed attributes changed, then we don't have 
	** to do duplicate checking.
	*/
	if ( (row_locking(ir) || crow_locking(ir)) &&
	     (flag & DM2R_SI_AGENT) == 0 && 
	     (flag & DM2R_MREPLACE) && 
	     (r->rcb_si_flags & RCB_SI_DUP_PRECHECK))
	{
	    ir->rcb_si_flags &= ~RCB_SI_VALUE_LOCKED;

	    /* Value lock was not taken if dup checking not done */
	    if ( it->tcb_rel.relstat & TCB_UNIQUE )
	    {
		for (i = 0; i < it->tcb_rel.relatts - 1; i++)
		{
		    /*
		    ** See if the value changed and check if the
		    ** attribute is a key of the secondary index.
		    */

		    if ((r->rcb_value_set[it->tcb_ikey_map[i] >> 5] &
			    (1 << (it->tcb_ikey_map[i] & 0x1f))) &&
			(it->tcb_atts_ptr[i+1].key))
		    {
			/* ...then si_dupcheck got a value lock */
			ir->rcb_si_flags |= RCB_SI_VALUE_LOCKED;
			break;
		    }
		}
	    }
	}
	
	/*
	** Build the secondary index record from the primary
	** record and tid.
	*/
	if (it->tcb_rel.relspec != TCB_RTREE)
	{
	    for (i = 0; i < it->tcb_rel.relatts - 1; i++)
	    {
		MEcopy(&record[t->tcb_atts_ptr[it->tcb_ikey_map[i]].offset],
		    t->tcb_atts_ptr[it->tcb_ikey_map[i]].length,
		    &ir->rcb_record_ptr[it->tcb_atts_ptr[i + 1].offset]);
	    }
	}
	else
	{
	    status = BuildRtreeRecord(t, it, ir, record,ir->rcb_record_ptr);
	    if ( status )
	    {
		SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
		break;
	    }
	    i = 2;
	}

	if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    /* Global index, add the row's partition to the TID */
	    new_tid8.tid_i4.tpf = 0;
	    new_tid8.tid.tid_partno = t->tcb_partno;
	    new_tid8.tid_i4.tid = r->rcb_si_newtid.tid_i4;
	    MEcopy((char *)&new_tid8, sizeof(DM_TID8),
	       &ir->rcb_record_ptr[it->tcb_atts_ptr[i + 1].offset]);
	}
	else
	    MEcopy((char *)&r->rcb_si_newtid, sizeof(DM_TID),
	       &ir->rcb_record_ptr[it->tcb_atts_ptr[i + 1].offset]);

	/*	Encrypt the record if needed. */
	if (it->tcb_data_rac.encrypted_attrs == TRUE)
	{
	    status = dm1e_aes_encrypt(r, &it->tcb_data_rac, ir->rcb_record_ptr,
			ir->rcb_erecord_ptr, dberr);
	    if (status != E_DB_OK)
		break;
	    rec = ir->rcb_erecord_ptr;
	}
	else
	    rec = ir->rcb_record_ptr;
	rec_len = it->tcb_rel.relwid;

	/*	Compress the record if needed. */
	if (it->tcb_data_rac.compression_type != TCB_C_NONE)
	{
	    rec = crec = ir->rcb_srecord_ptr;
	    (VOID) (*it->tcb_data_rac.dmpp_compress)(&it->tcb_data_rac,
		ir->rcb_record_ptr,
		(i4)it->tcb_rel.relwid,
		ir->rcb_srecord_ptr, &rec_len);
	}

	/*
	** If secondary index is unique - we have to do duplicate checking 
	** if it hasn't already been done.
	*/
	alloc_flag = DM1C_DUPLICATES;   /* don't do dup checking */
	if ( (it->tcb_rel.relstat & TCB_UNIQUE) && !(flag & DM2R_REDO_SF) )
	{
	    if ( (r->rcb_si_flags & RCB_SI_DUP_CHECKED) == 0 )
		alloc_flag = DM1C_NODUPLICATES; /* do dup checking */
	}

	/*
	** Take value lock for phantom protection, duplicate checking
	** If we already did duplicate checking, we already have the value lock
	** (We no longer have the lk_id, so the key will be rehashed to unlock)
	*/
	if ( (row_locking(ir) || crow_locking(ir))
		&& (ir->rcb_si_flags & RCB_SI_VALUE_LOCKED) == 0 )
	{
	    status = dm1r_lock_value(ir, (i4)DM1R_LK_PHYSICAL, ir->rcb_record_ptr, 
					    dberr);
	}

	if ( status == E_DB_OK )
	{
	    switch (it->tcb_rel.relspec)
	    {
	    case TCB_HASH:
		/* indexes use the base record encryption slot */
		ir->rcb_enckey_slot = r->rcb_enckey_slot;
		status = dm1h_allocate(ir, &ir->rcb_data,
				&ir->rcb_si_newtid, ir->rcb_record_ptr, crec,
		    		rec_len, alloc_flag, dberr);
		break;

	    case TCB_ISAM:
		status = dm1i_allocate(ir, &ir->rcb_data, 
				&ir->rcb_si_newtid, ir->rcb_record_ptr, crec,
		    		rec_len, alloc_flag, dberr);
		break;

	    case TCB_BTREE:
		status = dm1b_allocate(ir, &ir->rcb_other, 
				&ir->rcb_data, 
				&bid, &ir->rcb_si_newtid, 
		    		ir->rcb_record_ptr, crec, 
				rec_len, ir->rcb_s_key_ptr, alloc_flag, 
		    		dberr);
		break;

	    case TCB_RTREE:
		status = dm1m_allocate(ir, &ir->rcb_other, 
				&ir->rcb_data, 
				&bid, &ir->rcb_si_newtid, 
		    		ir->rcb_record_ptr, rec_len, 
				ir->rcb_s_key_ptr, alloc_flag, 
		    		dberr);
		break;
	    }
	}

	/* 
	** Once the record is on the page we can release the value lock 
	** for secondary indices we take the value locks for row locking
	*/
	if ( status == E_DB_OK && (row_locking(ir) || crow_locking(ir)) ) 
	{
	    status = dm1r_unlock_value(ir, ir->rcb_record_ptr, dberr);
	}

	/*	Now make the change. */

	if (status == E_DB_OK)
	{
	    switch (it->tcb_rel.relspec)
	    {
	    case TCB_HASH:
	    case TCB_ISAM:
		status = dm1r_put(ir, &ir->rcb_data, 
			    &ir->rcb_si_newtid, rec, (char *)NULL,
			    rec_len, opflag, dberr);
		break;

	    case TCB_BTREE:
		status = dm1b_put(ir, &ir->rcb_other, 
			    &ir->rcb_data, 
			    &bid, &ir->rcb_si_newtid, rec,
			    ir->rcb_record_ptr, 
			    rec_len, opflag, dberr);
		break;

	    case TCB_RTREE:
		status = dm1m_put(ir, &ir->rcb_other, 
			    &ir->rcb_data, 
			    &bid, &ir->rcb_si_newtid, rec,
			    ir->rcb_record_ptr, 
			    rec_len, dberr);
		break;
	    }
	}

	if ( status && (dberr->err_code == E_DM0045_DUPLICATE_KEY ||
			dberr->err_code == E_DM0046_DUPLICATE_RECORD) )
	{
	    if (it->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE)
		SETDBERR(dberr, 0, E_DM0151_SIDUPLICATE_KEY_STMT);
	    else
		SETDBERR(dberr, 0, E_DM0048_SIDUPLICATE_KEY);
	}
    } while ( status && flag & DM2R_RAAT &&
	       (dberr->err_code ==  E_DM004D_LOCK_TIMER_EXPIRED ||
		dberr->err_code ==  E_DM9043_LOCK_TIMEOUT) &&
	       (dberr->err_code = E_DM0173_RAAT_SI_TIMER) &&
	       (si_retries--) );

    /* Give up the SI RCB if we acquired it */
    if ( flag & DM2R_MPUT && ir && (flag & DM2R_SI_AGENT) == 0 &&
	dm2r_return_rcb(&ir, &local_dberr) && status == E_DB_OK )
    {
	*dberr = local_dberr;
	status = E_DB_ERROR;
    }

    if ( status )
    {
	/* If REPLACE, it'll handle the error */
	if ( flag & DM2R_MPUT && dberr->err_code >= E_DM_INTERNAL )
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
	}

	/*
	** If SI threads, set the worst
	** status in the RTB.
	*/
	if ( flag & DM2R_SI_AGENT )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    if ( rtb->rtb_status == E_DB_OK )
	    {
		rtb->rtb_status = status;
		rtb->rtb_error = *dberr;
	    }
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
    }
	
    return (status);
}


/*{
** Name: si_delete - Delete from secondary index.
**
** Description:
**      After deleting a row from a base table, this function is called
**      to delete the corresponding secondary index entry.
**
**
** Inputs:
**      ir                              RCB of secondary index.
**      deltid                          Tid of base table tuple deleted
**      opflag
**
** Outputs:
**	err_code			Reason for error return status.
**
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
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Moved index 'delete' code to its own routine
**      22-jan-97 (dilma04)
**          Fix bug 80112: Add DM1C_GET_SI_UPD flag to dm1r_get call to prevent
**          unnecessary row locking. Also change DM1C_BYTID_SI_UPD flag in
**          dm1b_get() call to DM1C_GET_SI_UPD to be consistent with dm1r_get().
**      27-jan-97 (dilma04)
**          Pass DM1C_FIND_SI_UPD flag to search routines to prevent phantom
**          protection when updating secondary indexes. This fixes bug 80114. 
**          Also for consistency with other routines set DM1C_BYTID when 
**          calling dm1b_get to update secondary index.
**      21-may-97 (stial01)
**          si_delete() BTREE/RTREE: Removed unecessary get after search.
**          When row locking, the get by tid would require reposition
**          logic in secondary index get by tid.
**      29-jul-97 (stial01)
**          si_delete() Replaced rcb_update parameter with opflag parameter.
**          Pass original operation mode 'opflag' to dm1*_put for logging.
**	09-Feb-2001 (jenjo02)
**	    Pushed TID search into dm1r_get().
**	22-Jan-2004 (jenjo02)
**	    Modified to be callable from the mainline code or from
**	    a parallel SI Agent.
**	    Added Global SI, TID8 support.
**	07-Apr-2004 (jenjo02)
**	    Fix silently lost "delete" from si_replace when row 
**	    locking.
**	02-Jul-2004 (hanal04) Bug 110565 INGSRV2889
**	    RTREE secondary index search is supposed to use a key
**	    built from the base table record, not a record built from
**	    the base table key. Corrected parameters passwd to 
**	    BuildRtreeRecord() and dm1m_search() respectively.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Set index's rcb_dmr_opcode properly when doing
**	    GET, DELETE.
*/
static DB_STATUS
si_delete(
    DMP_TCB	*it,
    DMP_RCB     *r,
    i4		flag,
    DB_ERROR   	*dberr )
{
    DMP_RCB		*ir;
    DMR_RTB	    	*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB		*t;
    DB_STATUS           status, rtb_status;
    DM_TID              tid;
    DM_TID8		old_tid8;
    i4             	i;
    i4             	save_update_mode;
    i4                  attno;
    i4			si_retries = 0;
    char    		*raat_retry;
    i4			opflag;
    i4			loc_err;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = E_DB_OK;
    rtb_status = E_DB_OK;

    /*
    ** If called from an SIAgent or from si_replace,
    ** "r" is the index's RCB.
    */
    if ( flag & (DM2R_SI_AGENT | DM2R_MREPLACE) )
    {
	ir = r;
	r  = ir->rcb_base_rcb;
	if ( flag & DM2R_SI_AGENT )
	    rtb_status = rtb->rtb_status;
    }
    else
	ir = (DMP_RCB*)NULL;

    if ( flag & DM2R_MDELETE )
    {
	opflag = DM1C_MDELETE;

	if ( rtb_status == E_DB_OK )
	{
	    /* Allocate an RCB for the SI */
	    if ( ir == (DMP_RCB*)NULL )
		status = dm2r_rcb_allocate(it, r, &r->rcb_tran_id,
					    r->rcb_lk_id,
					    r->rcb_log_id,
					    &ir, dberr);

	    /* If RAAT, get the retry count */
	    if ( status == E_DB_OK && flag & DM2R_RAAT )
	    {
	       NMgtAt("II_RAAT_RETRY", &raat_retry);
	       if (raat_retry && *raat_retry ) 
		  si_retries = atoi(raat_retry);
	       if (si_retries < 1) 
		  si_retries = 0;
	    } 
	}
    }
    else if ( flag & DM2R_MREPLACE )
    {
	opflag = DM1C_MREPLACE;

	/*
	** Called from inside si_replace, which has 
	** already acquired a SI RCB ("r")
	** and is inside a RAAT retry loop, if needed.
	*/

	if ( rtb_status )
	{
	    /* Global threading error, NOOP the "delete" */
	    ir = (DMP_RCB*)NULL;
	}
    }
    else
	dmd_check(E_DM9311_DM2R_DELETE);

    /* The Base TCB */
    t = r->rcb_tcb_ptr;

    /* Format big tid for global index */
    old_tid8.tid_i4.tpf = 0;
    old_tid8.tid.tid_partno = t->tcb_partno;
    old_tid8.tid_i4.tid = r->rcb_si_oldtid.tid_i4;

    /* If a previous operation on the TBIO for this index raised
    ** a 'check_high_water_mark' exception, then process it now,
    ** so that this and future operations against this index wont
    ** be prevented from making use of those optimizations that
    ** require a proper EOF (hwm) to be known. One such operation
    ** is use of groups buffers, which can now be used for secondary
    ** indexes in certain cases (batch).
    */
    if ( ir && ir->rcb_tcb_ptr->tcb_table_io.tbio_checkeof)
	dm1p_checkhwm(ir, dberr);

    if ( ir && rtb_status == E_DB_OK )
	do
    {
	status = E_DB_OK;

	/*
	** Build the secondary index key from the primary record and
	** tid.  Btree, Rtree, and Isam secondary indexes include the tidp
	** field as the last key entry, while hash indexes do not.
	**
	** NOTE:  The hash search routine below expects a tuple parameter
	** while the isam and R/btree search routines expect a key.  Here
	** we are building a key value, not a full tuple.  We are assuming
	** that we can send this key to dm1h_search rather than a full
	** tuple because of the following:
	**	    - dm1h_search only looks at the keyed fields (and we are
	**	      providing all of the keyed fields).
	**	    - secondary indexes must be keyed on a prefix of the full
	**	      secondary index tuple (thus the keyed fields have the
	**	      same offset in the key as they would in a full tuple).
	**	    - hash 2nd indexes do not use the tidp as part of the key.
	*/
	if (it->tcb_rel.relspec != TCB_RTREE)
	{
	    for (i = 0; i < it->tcb_keys; i++)
	    {
		attno = it->tcb_ikey_map[it->tcb_att_key_ptr[i] - 1];
		if (attno > 0)
		{
		    MEcopy(&r->rcb_record_ptr[t->tcb_atts_ptr[attno].offset],
			t->tcb_atts_ptr[attno].length,
			&ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
		}
		else
		{
		    /*
		    ** If the attribute is the tidp, then get the value from
		    ** the delete tuple id.
		    */
		    if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
		    {
			MEcopy((char *)&old_tid8, sizeof(DM_TID8),
			    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
		    }
		    else
			MEcopy((char *)&r->rcb_si_oldtid, sizeof(DM_TID),
			    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
		}
	    }
	}
	else
	{
	    status = BuildRtreeRecord(t, it, ir, r->rcb_record_ptr,
				      ir->rcb_s_key_ptr);
	    if (status != E_DB_OK)
		SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
	    else
	    {
		if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
		    MEcopy((char *)&old_tid8, sizeof(DM_TID8),
			&ir->rcb_s_key_ptr[it->tcb_atts_ptr[3].key_offset]);
		else
		    MEcopy((char *)&r->rcb_si_oldtid, sizeof(DM_TID),
			&ir->rcb_s_key_ptr[it->tcb_atts_ptr[3].key_offset]);
	    }
	}

	/*
	** take the value lock to prevent insert to same key value 
	** NOTE: dm1r_lock value requires a tuple, and hashes the key fields
	** (excluding tidp)
	** Since the non-tidp key fields are at the same offset in the key/rec
	** we can pass the key we built.
	*/
	if ( status == E_DB_OK && it->tcb_rel.relstat & TCB_UNIQUE
		&& (row_locking(ir) || crow_locking(ir)) )
	{
	    status = dm1r_lock_value(ir, (i4)DM1R_LK_PHYSICAL, 
					ir->rcb_s_key_ptr, dberr);
	}

	if ( status )
	    continue;

	/*
	** If the update cursor is a deferred cursor, then we want to
	** do some special processing here.  When we update the tuples
	** on the data page, we want to use the deferred cursor protocol
	** (write our sequence number on the page and set the changed
	** bit), but when we are looking for tuples in the secondary
	** index, we want to use DIRECT cursor protocol, since using
	** DEFERRED could cause us to skip over the secondary index
	** tuple we are trying to update.
	*/
	save_update_mode = ir->rcb_update_mode;
	ir->rcb_update_mode = RCB_U_DIRECT;

	/* Let dm0p know we're doing a GET so Consistent Read is done */
	ir->rcb_dmr_opcode = DMR_GET;

	switch (it->tcb_rel.relspec)
	{
	    case TCB_HASH:
		/* indexes use the base record encryption slot */
		ir->rcb_enckey_slot = r->rcb_enckey_slot;
		status = dm1h_search(ir, &ir->rcb_data,
		    ir->rcb_s_key_ptr, DM1C_FIND_SI_UPD, DM1C_EXACT,
		    &ir->rcb_lowtid, dberr);

		if ( status == E_DB_OK )
		    ir->rcb_hightid.tid_i4 = ir->rcb_lowtid.tid_i4;
		break;

	    case TCB_ISAM:
		status = dm1i_search(ir, &ir->rcb_data,
		    ir->rcb_s_key_ptr, it->tcb_keys,
		    DM1C_FIND_SI_UPD, DM1C_EXACT, &ir->rcb_lowtid, dberr);

		if ( status == E_DB_OK )
		    ir->rcb_hightid.tid_i4 = ir->rcb_lowtid.tid_i4;
		break;

	    case TCB_BTREE:
		ir->rcb_currenttid.tid_i4 = r->rcb_si_oldtid.tid_i4;
		status = dm1b_search(ir, ir->rcb_s_key_ptr, it->tcb_keys,
		    DM1C_FIND_SI_UPD, DM1C_EXACT,
		    &ir->rcb_other, &ir->rcb_lowtid,
		    &ir->rcb_currenttid, dberr);

		if ( status == E_DB_OK )
		{
		    tid.tid_i4 = ir->rcb_lowtid.tid_i4;
		    /* dm1b_search DM1C_FIND_SI_UPD will also fetch the key */
		    ir->rcb_fetchtid.tid_i4 = ir->rcb_lowtid.tid_i4;
		    ir->rcb_state &= ~RCB_FETCH_REQ;
		}

		break;

	    case TCB_RTREE:
                ir->rcb_currenttid.tid_i4 = r->rcb_si_oldtid.tid_i4;
		status = dm1m_search(ir, ir->rcb_s_key_ptr, it->tcb_keys,
                    DM1C_TID, DM1C_EXACT,
		    &ir->rcb_other, &ir->rcb_lowtid,
                    &ir->rcb_currenttid, dberr);

		if ( status == E_DB_OK )
		{
		    tid.tid_i4 = ir->rcb_lowtid.tid_i4;
		    status = dm1m_get(ir, &tid, ir->rcb_record_ptr,
			DM1C_BYTID, dberr);
		}
		break;
	}

	if ( status )
	    continue;

	if ((it->tcb_rel.relspec != TCB_BTREE) &&
	    (it->tcb_rel.relspec != TCB_RTREE))
	{
	    /*  BTREE & RTREE return the exact row, all other access
	    **  methods have to search. */

	    /*
	    ** Pass the tid we're looking for to dm1r_get() 
	    ** which will search on our behalf until it's found
	    ** (triggered by DM1C_GET_SI_UPD).
	    */
	    tid.tid_i4 = r->rcb_si_oldtid.tid_i4;

	    /* indexes use the base record encryption slot */
	    ir->rcb_enckey_slot = r->rcb_enckey_slot;
	    status = dm1r_get(ir, &tid, ir->rcb_record_ptr,
		DM1C_GETNEXT | DM1C_GET_SI_UPD, dberr);
	}

	if ( status )
	    continue;

	/*
	** Now make the change.
	** Restore the sec index update mode which was set to DIRECT for
	** search call above.
	*/
	ir->rcb_update_mode = save_update_mode;

	/* Let dm0p know we're doing a DELETE */
	ir->rcb_dmr_opcode = DMR_DELETE;

	switch (it->tcb_rel.relspec)
	{
	    case TCB_HASH:
	    case TCB_ISAM:
		status = dm1r_delete(ir, &ir->rcb_lowtid, DM1C_BYPOSITION,
		    opflag, dberr);
		break;

	    case TCB_BTREE:
		status = dm1b_delete(ir, &ir->rcb_lowtid, &ir->rcb_currenttid,
		    opflag, dberr);
		break;

	    case TCB_RTREE:
		status = dm1m_delete(ir, &ir->rcb_lowtid, &ir->rcb_currenttid,
                    dberr);
		break;
	}

	/*
	** release the value lock
	** NOTE: dm1r_lock value requires a tuple, and hashes the key fields
	** (excluding tidp)
	** Since the non-tidp key fields are at the same offset in the key/rec
	** we can pass the key we built.
	*/
	if ( (row_locking(ir) || crow_locking(ir)) && 
		it->tcb_rel.relstat & TCB_UNIQUE )
	{
	    status = dm1r_unlock_value(ir, ir->rcb_s_key_ptr, dberr);
	}

    } while ( status && flag & DM2R_RAAT &&
	       (dberr->err_code ==  E_DM004D_LOCK_TIMER_EXPIRED ||
		dberr->err_code ==  E_DM9043_LOCK_TIMEOUT) &&
	       (dberr->err_code = E_DM0173_RAAT_SI_TIMER) &&
	       (si_retries--) );

    /* Give up the SI RCB if we acquired it */
    if ( flag & DM2R_MDELETE && ir && (flag & DM2R_SI_AGENT) == 0 &&
	dm2r_return_rcb(&ir, &local_dberr) && status == E_DB_OK )
    {
	*dberr = local_dberr;
	status = E_DB_ERROR;
    }

    if ( status )
    {
	/* If REPLACE, it'll handle reporting any errors */
	if ( flag & DM2R_MDELETE )
	{
	    /* Identify tranid for debugging */
	    TRdisplay("%@ si_delete:%d tran %x tid [%d,%d] error %x\n",
	    		__LINE__,
			r->rcb_tran_id.db_low_tran,
			r->rcb_si_oldtid.tid_tid.tid_page,
			r->rcb_si_oldtid.tid_tid.tid_line,
			dberr->err_code);

	    /* Show the error and its origin */
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &loc_err, 0);

	    /*
	    ** If NONEXT or NOPART then we failed trying to look up the
	    ** secondary index tuple in the index table.
	    */
	    if (dberr->err_code == E_DM0055_NONEXT || dberr->err_code == E_DM0056_NOPART)
	    {
		uleFormat(NULL, E_DM9385_NO_INDEX_TUPLE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, r->rcb_si_oldtid.tid_i4);
		SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
	    }

	    if ( dberr->err_code > E_DM_INTERNAL )
	    {
		if ( dberr->err_code != E_DM9387_INDEX_UPDATE_ERROR )
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
			(i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9386_SEC_INDEX_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
		    sizeof(DB_TAB_NAME), &it->tcb_rel.relid.db_tab_name, 
		    0, r->rcb_si_oldtid.tid_i4);
		SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
	    }
	}

	/*
	** If SI threads, set the worst
	** status in the RTB.
	*/
	if ( flag & DM2R_SI_AGENT )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    if ( rtb->rtb_status == E_DB_OK )
	    {
		rtb->rtb_status = status;
		rtb->rtb_error = *dberr;
	    }
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
    }

    return (status);
}


/*{
** Name: si_replace - Update secondary index.
**
** Description:
**      After replacing a row into a base table, this function is called
**      to update the corresponding secondary index entry.
**
**
** Inputs:
**      tcb                             TCB of secondary index.
**      rcb                             RCB of base table.
**      record                          Pointer to (base table) record.
**
** Outputs:
**	err_code			Reason for error return status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			Index update not needed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Moved index 'replace' code to its own routine
**      27-jan-97 (dilma04)
**          Pass DM1C_FIND_SI_UPD flag to search routines to prevent phantom
**          protection when updating secondary indexes. Set DM1C_GET_SI_UPD
**          flag in dm1b_get call to be consistent with si_delete. 
**      10-mar-97 (stial01)
**          Use rcb_tupbuf for compressing record, to avoid rebuilding and
**          recompressing index records.
**      21-may-97 (stial01)
**          si_replace() BTREE/RTREE: Removed unecessary get after search.
**          When row locking, the get by tid would require reposition
**          logic in secondary index get by tid.
**      29-jul-97 (stial01)
**          si_replace() Added row locking consistency check.
**	01-oct-1998 (nanpr01)
**	    update performance improvements.
** 	08-apr-1999 (wonst02)
** 	    Treat RTREE more like BTREE in si_replace (called by dm2r_replace).
**      13-nov-2000 (stial01)
**          si_replace() dup checking only for ADT_VALUE key changes (b103192)
**	09-Feb-2001 (jenjo02)
**	    Pushed TID search into dm1r_get().
**	22-Jan-2004 (jenjo02)
**	    Modified to be callable from the mainline code or from
**	    a parallel SI Agent.
**	    Added Global SI, TID8 support.
**	6-Mar-2006 (kschendel)
**	    Fix typo in above (how did it slip by all this time) that put the
**	    tid8 into the wrong place (rcb_record instead of rcb_srecord)
**	    for global index replaces.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Set index's rcb_dmr_opcode properly when doing
**	    GET, REPLACE.
**	28-Apr-2010 (jonj) SD 144272
**	    Remove indirect references to pinfo.page, use DMP_PINIT to
**	    fully init pinfos.
**	24-Aug-2010 (miket) SIR 122403 SD 145904
**	    Encrypted indexes get the encryption shm slot number from the base
**	    table parent. Encrypt SIs on replace.
*/
static DB_STATUS
si_replace(
    DMP_TCB	*it,
    DMP_RCB     *r,
    char        *record,
    i4		flag,
    DB_ERROR   	*dberr )
{
    DMP_RCB		*ir;
    DMR_RTB	    	*rtb = (DMR_RTB*)r->rcb_rtb_ptr;
    DMP_TCB		*t;
    DB_STATUS           status, rtb_status, loc_status;
    DM_TID              oldbid;
    DM_TID              oldtid;
    DM_TID              newbid;
    DM_TID              newtid;
    i4             	i;
    i4                  attno;
    i4             	oldlength;
    i4             	newlength;
    char                *oldrecord;
    char                *newrecord;
    i4             	save_update_mode;
    DM_TID		ancestors[RCB_MAX_RTREE_LEVEL];
    i4			ancestor_level;
    char		*crec = (PTR)0;
    i4			si_retries;
    char    		*raat_retry;
    i4			delta_start, delta_end;
    i4			loc_err;
    i4			row_deleted = FALSE;
    i4			type_key_change;
    i4			alloc_flag;
    DM_TID8		old_tid8, new_tid8;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    DMP_PINFO		newdataPinfo, newleafPinfo;

    CLRDBERR(dberr);

    status = E_DB_OK;
    rtb_status = E_DB_OK;

    DMP_PINIT(&newdataPinfo);
    DMP_PINIT(&newleafPinfo);

    if ( flag & DM2R_SI_AGENT )
    {
	ir = r;
	r  = ir->rcb_base_rcb;
	rtb_status = rtb->rtb_status;
    }
    else
	ir = (DMP_RCB*)NULL;
    
    /* The Base TCB */
    t = r->rcb_tcb_ptr;

    /*  Check to see if any attributes of this index
    **  changed in any way. */

    type_key_change = ADT_NOCHANGE;
    alloc_flag = DM1C_DUPLICATES; /* don't need to dup check */
    delta_start = 0;

    for (i = 0; i < it->tcb_rel.relatts - 1; i++)
    {
	/*  See if the bit representation changed. */

	if ((r->rcb_change_set[it->tcb_ikey_map[i] >> 5] &
	    (1 << (it->tcb_ikey_map[i] & 0x1f))) == 0)
	{
	    continue;
	}

	/*  Check to see if the actual value changed. */
	if ( (r->rcb_value_set[it->tcb_ikey_map[i] >> 5] &
	     (1 << (it->tcb_ikey_map[i] & 0x1f))) )
	{
	    if ( type_key_change != ADT_VALUE )
	    {
		delta_start = it->tcb_atts_ptr[i + 1].offset;
		type_key_change = ADT_VALUE;
	    }
	    /*
	    ** If we haven't already duplicate checked,
	    ** check if any keyed attribute in this index changed.
	    ** If only non-keyed attributes changed, then we don't
	    ** have to duplicate check in si_replace().
	    */
	    if ( it->tcb_rel.relstat & TCB_UNIQUE && 
		 (r->rcb_si_flags & RCB_SI_DUP_CHECKED) == 0 &&
		 it->tcb_atts_ptr[i+1].key )
	    {
		/* Need to do duplicate checking */
		alloc_flag = DM1C_NODUPLICATES;
		break;
	    }
	}
	else if ( type_key_change != ADT_VALUE )
	{
	    type_key_change = ADT_REPRESENTATION;
	    delta_start = it->tcb_atts_ptr[i + 1].offset;
	}
    }

    /*
    ** Return if nothing changed, but watch out for
    ** only a change it TIDs.
    */

    if ( type_key_change == ADT_NOCHANGE )
    {
	if ( r->rcb_si_oldtid.tid_i4 == r->rcb_si_newtid.tid_i4 )
	{
	    SETDBERR(dberr, 0, E_DM0154_DUPLICATE_ROW_NOTUPD);
	    ir = (DMP_RCB*)NULL;
	}
	else
	    type_key_change = ADT_REPRESENTATION;
    }

    /*
    ** Allocate an RCB for the SI unless 
    ** there's been a global failure or 
    ** no change is needed
    */

    if ( rtb_status == E_DB_OK && 
	 type_key_change != ADT_NOCHANGE &&
	 (flag & DM2R_SI_AGENT) == 0 )
    {
        status = dm2r_rcb_allocate(it, r, &r->rcb_tran_id,
				    r->rcb_lk_id,
				    r->rcb_log_id,
				    &ir, dberr);
    }

    if ( ir )
    {
	/* If a previous operation on the TBIO for this index raised
	** a 'check_high_water_mark' exception, then process it now,
	** so that this and future operations against this index wont
	** be prevented from making use of those optimizations that
	** require a proper EOF (hwm) to be known. One such operation
	** is use of groups buffers, which can now be used for secondary
	** indexes in certain cases (batch).
	*/
	if (ir->rcb_tcb_ptr->tcb_table_io.tbio_checkeof)
	    dm1p_checkhwm(ir, dberr);

	/* If RAAT, set up the timeout retry loop count */
	if ( flag & DM2R_RAAT )
	{
	   NMgtAt("II_RAAT_RETRY", &raat_retry);
	   if (raat_retry && *raat_retry ) 
	      si_retries = atoi(raat_retry);
	   if (si_retries < 1) 
	      si_retries = 0;
	} 
	else
	    si_retries = 0;
    }

    if ( ir && rtb_status == E_DB_OK )
	do
    {
	status = E_DB_OK;
	DMP_PINIT(&newdataPinfo);
	DMP_PINIT(&newleafPinfo);

	/* Make big tids for global indexes */
	if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	{
	    /* Global index, add the row's partition to the TIDs */
	    new_tid8.tid_i4.tpf = 0;
	    new_tid8.tid.tid_partno = t->tcb_partno;
	    new_tid8.tid_i4.tid = r->rcb_si_newtid.tid_i4;
	    old_tid8.tid_i4.tpf = 0;
	    old_tid8.tid.tid_partno = t->tcb_partno;
	    old_tid8.tid_i4.tid = r->rcb_si_oldtid.tid_i4;
	}

	/* When row locking, we cannot mutex/log two pages deadlock-free */
	if ( row_locking(ir) || crow_locking(ir) )
	{
	    /* Delete using base r->rcb_si_oldtid */
	    if ( row_deleted == FALSE && 
		 (!rtb || rtb->rtb_status == E_DB_OK) )
	    {
		status = si_delete(it, ir, flag | DM2R_MREPLACE, dberr);
	    }

	    if ( status == E_DB_OK &&
		 (!rtb || rtb->rtb_status == E_DB_OK) )
	    {
		/* Don't raat-retry delete */
		row_deleted = TRUE;

		if (ir->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE)
		    ir->rcb_si_oldtid.tid_i4 = ir->rcb_lowtid.tid_i4;

		/* Put using base r->rcb_si_newtid */
		status = si_put(it, ir, record, flag | DM2R_MREPLACE, dberr);

		/* rcb update */
		if ( status == E_DB_OK &&
		    ir->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE &&
		     (!rtb || rtb->rtb_status == E_DB_OK) )
		{
		    status = dm1b_rcb_update(ir, &ir->rcb_si_oldtid, 
				    &ir->rcb_si_newtid, (i4)0, 
				    DM1C_MREPLACE, FALSE,
				    &r->rcb_si_oldtid, 
				    &r->rcb_si_newtid, 
				    ir->rcb_s_key_ptr, 
				    dberr);
		}
	    }
	}
	else
	{
	    /*
	    ** Build new secondary index tuple from new record and its tid.
	    */
	    if (it->tcb_rel.relspec != TCB_RTREE)
	    {
		for (i = 0; i < it->tcb_rel.relatts - 1; i++)
		{
		    MEcopy(&record[t->tcb_atts_ptr[it->tcb_ikey_map[i]].offset],
			t->tcb_atts_ptr[it->tcb_ikey_map[i]].length,
			&ir->rcb_srecord_ptr[it->tcb_atts_ptr[i + 1].offset]);
		}
	    }
	    else
	    {
		status = BuildRtreeRecord(t, it, ir, record, ir->rcb_srecord_ptr);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
		    break;
		}
		i = 2;
	    }

	    if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
	    {
		/* Global index, add the row's partition to the TID */
		MEcopy((char *)&new_tid8, sizeof(DM_TID8),
		   &ir->rcb_srecord_ptr[it->tcb_atts_ptr[i + 1].offset]);
	    }
	    else
		MEcopy((char *)&r->rcb_si_newtid, sizeof(DM_TID),
		   &ir->rcb_srecord_ptr[it->tcb_atts_ptr[i+1].offset]);

	    /*
	    ** Build key to search for old secondary index tuple from the
	    ** old tuple and its tid.
	    **
	    ** NOTE:  The hash search routine below expects a tuple parameter
	    ** while the isam and R/btree search routines expect a key.  Here
	    ** we are building a key value, not a full tuple.  We are assuming
	    ** that we can send this key to dm1h_search rather than a full
	    ** tuple because of the following:
	    **	- dm1h_search only looks at the keyed fields (and we are
	    **	  providing all of the keyed fields).
	    **	- secondary indexes must be keyed on a prefix of the full
	    **	  secondary index tuple (thus the keyed fields have the
	    **	  same offset in the key as they would in a full tuple).
	    **	- hash 2nd indexes do not use the tidp as part of the key.
	    */
	    if (it->tcb_rel.relspec != TCB_RTREE)
	    {
		for (i = 0; i < it->tcb_keys; i++)
		{
		    attno = it->tcb_ikey_map[it->tcb_att_key_ptr[i] - 1];
		    if (attno > 0)
		    {
			MEcopy(&r->rcb_record_ptr[t->tcb_atts_ptr[attno].offset],
			    t->tcb_atts_ptr[attno].length,
			    &ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
		    }
		    else
		    {
			/*
			** If the attribute is the tidp, then get the value from
			** the old tuple id.
			*/
			if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
			{
			    MEcopy((char *)&old_tid8, sizeof(DM_TID8),
				&ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
			}
			else
			    MEcopy((char *)&r->rcb_si_oldtid, sizeof(DM_TID),
				&ir->rcb_s_key_ptr[it->tcb_key_atts[i]->key_offset]);
		    }
		}
	    }
	    else
	    {
		status = BuildRtreeRecord(t, it, ir, r->rcb_record_ptr,
					  ir->rcb_s_key_ptr);
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
		    break;
		}

		if ( it->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX )
		    MEcopy((char *)&old_tid8, sizeof(DM_TID8),
		       &ir->rcb_s_key_ptr[it->tcb_atts_ptr[3].key_offset]);
		else
		    MEcopy((char *)&r->rcb_si_oldtid, sizeof(DM_TID),
		       &ir->rcb_s_key_ptr[it->tcb_atts_ptr[3].key_offset]);
	    }

	    /*
	    ** If the update cursor is a deferred cursor, then we want to
	    ** do some special processing here.  When we update the tuples
	    ** on the data page, we want to use the deferred cursor protocol
	    ** (write our sequence number on the page and set the changed
	    ** bit), but when we are looking for tuples in the secondary
	    ** index, we want to use DIRECT cursor protocol, since using
	    ** DEFERRED could cause us to skip over the secondary index
	    ** tuple we are trying to update.
	    */
	    save_update_mode = ir->rcb_update_mode;
	    ir->rcb_update_mode = RCB_U_DIRECT;

	    /* Let dm0p know we're doing a GET so Consistent Read is done */
	    ir->rcb_dmr_opcode = DMR_GET;

	    /*  Locate the old index record. */

	    switch (it->tcb_rel.relspec)
	    {
	    case TCB_HASH:
		/* indexes use the base record encryption slot */
		ir->rcb_enckey_slot = r->rcb_enckey_slot;
		status = dm1h_search(ir, &ir->rcb_data,
		    ir->rcb_s_key_ptr,
		    DM1C_FIND_SI_UPD, DM1C_EXACT, &ir->rcb_lowtid, dberr);
		if (status != E_DB_OK)
		    break;
		ir->rcb_hightid.tid_i4 = ir->rcb_lowtid.tid_i4;
		break;

	    case TCB_ISAM:
		status = dm1i_search(ir, &ir->rcb_data,
		    ir->rcb_s_key_ptr, it->tcb_keys,
		    DM1C_FIND_SI_UPD, DM1C_EXACT, &ir->rcb_lowtid, dberr);
		if (status != E_DB_OK)
		    break;
		ir->rcb_hightid.tid_i4 = ir->rcb_lowtid.tid_i4;
		break;

	    case TCB_BTREE:
		ir->rcb_currenttid.tid_i4 = r->rcb_si_oldtid.tid_i4;
		status = dm1b_search(ir, ir->rcb_s_key_ptr, it->tcb_keys,
		    DM1C_FIND_SI_UPD, DM1C_EXACT,&ir->rcb_other,
		    &ir->rcb_lowtid, &ir->rcb_currenttid, dberr);
		if (status != E_DB_OK)
		    break;
		oldtid.tid_i4 = ir->rcb_lowtid.tid_i4;

		/* dm1b_search DM1C_FIND_SI_UPD will also fetch the key */
		ir->rcb_fetchtid.tid_i4 = ir->rcb_lowtid.tid_i4;
		ir->rcb_state &= ~RCB_FETCH_REQ;

		break;

	    case TCB_RTREE:
		ir->rcb_currenttid.tid_i4 = r->rcb_si_oldtid.tid_i4;
		status = dm1m_search(ir, ir->rcb_s_key_ptr, it->tcb_keys,
		    DM1C_TID, DM1C_EXACT,&ir->rcb_other,
		    &ir->rcb_lowtid, &ir->rcb_currenttid, dberr);
		if (status != E_DB_OK)
		    break;
		/*
		** Save the RCB ancestor stack, which contains the
		** position information for the old (to-be-deleted) row.
		** The old position is needed for dm1m_replace, below.
		*/
		ancestor_level = ir->rcb_ancestor_level;
		MEcopy(ir->rcb_ancestors, sizeof ir->rcb_ancestors, ancestors);
		oldtid.tid_i4 = ir->rcb_lowtid.tid_i4;
		oldbid.tid_i4 = ir->rcb_lowtid.tid_i4;
		/*
		** We don't actually need to get the leaf entry because
		** we already know the key,tidp and bid.
		** Init rcb so it looks like we did a get of the leaf entry.
		*/
		ir->rcb_fetchtid.tid_i4 = ir->rcb_lowtid.tid_i4;
		ir->rcb_state &= ~RCB_FETCH_REQ;
		break;
	    }

	    if (status != E_DB_OK)
		break;

	    if ((it->tcb_rel.relspec != TCB_BTREE) &&
		(it->tcb_rel.relspec != TCB_RTREE))
	    {
		/*  BTREE & RTREE return the exact row, all other access methods
		**  have to search. */

		/*
		** Pass the tid we're looking for to dm1r_get() 
		** which will search on our behalf until it's found
		** (triggered by DM1C_GET_SI_UPD).
		*/
		oldtid.tid_i4 = r->rcb_si_oldtid.tid_i4;

		/* indexes use the base record encryption slot */
		ir->rcb_enckey_slot = r->rcb_enckey_slot;
		status = dm1r_get(ir, &oldtid, ir->rcb_record_ptr,
			DM1C_GETNEXT | DM1C_GET_SI_UPD, dberr);

		if (status != E_DB_OK)
		    break;
	    }

	    /* Restore the secondary index rcb's update mode */
	    ir->rcb_update_mode = save_update_mode;

	    newrecord = ir->rcb_srecord_ptr;
	    oldrecord = ir->rcb_record_ptr;

	    /* Encrypt the index if needed. */
	    if (t->tcb_data_rac.encrypted_attrs)
	    {
		status = dm1e_aes_encrypt(r, &it->tcb_data_rac, newrecord,
			ir->rcb_erecord_ptr, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*  Compute the sizes of the old and new records. */

	    oldlength = newlength = it->tcb_rel.relwid;
	    if ((it->tcb_data_rac.compression_type != TCB_C_NONE) &&
		(it->tcb_rel.relspec != TCB_BTREE) &&
		(it->tcb_rel.relspec != TCB_RTREE))
	    {

		/*
		** Reset 'delta_start' to zero here if secondary index is compressed. 
		** Fixes bug 106823.  
		*/
		delta_start = 0;

		/*  Compress old record to get it's length. */
		(VOID) (*it->tcb_data_rac.dmpp_compress)(&it->tcb_data_rac,
			      ir->rcb_record_ptr, (i4)it->tcb_rel.relwid,
			      ir->rcb_tupbuf, &oldlength);


		 /* Compress new record to get its length. */
		crec = ir->rcb_tupbuf;
		(VOID) (*it->tcb_data_rac.dmpp_compress)(&it->tcb_data_rac,
			      ir->rcb_srecord_ptr, (i4)it->tcb_rel.relwid,
			      ir->rcb_tupbuf, &newlength);
	    }

	    /*
	    **  Secondary indices are not checked for duplicate records because
	    **  the tidp field of a index is unique regardless of the key.
	    **
	    **  The only thing to worry about here is move the record if
	    **  the keys change or if the size of the record changes.
	    */

	    /* Let dm0p know we're doing a REPLACE now */
	    ir->rcb_dmr_opcode = DMR_REPLACE;

	    newbid.tid_i4 = NOT_ALLOCATED;
	    newtid.tid_i4 = NOT_ALLOCATED;
	    if (type_key_change != ADT_NOCHANGE || newlength != oldlength)
	    {
		switch (it->tcb_rel.relspec)
		{
		case TCB_HASH:
		    /* indexes use the base record encryption slot */
		    ir->rcb_enckey_slot = r->rcb_enckey_slot;
		    status = dm1h_allocate(ir, &newdataPinfo, &newtid,
				newrecord, crec, newlength, alloc_flag, dberr);
		    break;

		case TCB_ISAM:
		    status = dm1i_allocate(ir, &newdataPinfo, &newtid,
				newrecord, crec, newlength, alloc_flag, dberr);
		    break;

		case TCB_BTREE:
		    status = dm1b_allocate(ir, &newleafPinfo, &newdataPinfo,
				&newbid, &newtid, newrecord, crec, newlength,
				ir->rcb_s_key_ptr, alloc_flag, dberr);
			
		    if (DB_FAILURE_MACRO(status))
			break;

		    /*
		    ** Reposition to correct btree leaf entry if necessary
		    ** The allocate above may have caused a split.
		    ** For V2 pages, the allocate may also do 
		    ** a page clean, and space reservation when row locking.
		    **
		    ** For V1 pages, the bid would have been adjusted by 
		    ** active_rcb_update. Below we make sure the correct
		    ** leaf is fixed for dm1b_replace().
		    **
		    ** If not V1, reposition will be done in dm1b_replace()
		    */

		    /*
		    ** Code below depends on the fact that leaf with the old
		    ** tuple is fixed into rcb_other.page.
		    ** If the replace tuple has moved because of a split, then
		    ** fix the correct page into rcb_other.page.
		    ** Only need to do this for V1 pages
		    ** V2 pages will reposition in dm1b_replace if necessary
		    */
		    if (t->tcb_rel.relpgtype == TCB_PG_V1 &&
		        DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			ir->rcb_other.page) !=
					     ir->rcb_fetchtid.tid_tid.tid_page)
		    {
			status = dm0p_unfix_page(ir, DM0P_UNFIX,
			    &ir->rcb_other, dberr);
			if (DB_FAILURE_MACRO(status))
			    break;
			status = dm0p_fix_page(ir,
			    (DM_PAGENO) ir->rcb_fetchtid.tid_tid.tid_page,
			    DM0P_WRITE | DM0P_NOREADAHEAD,
			    &ir->rcb_other, dberr);
			if (DB_FAILURE_MACRO(status))
			    break;
		    }
		    break;

		case TCB_RTREE:
		    status = dm1m_allocate(ir, &newleafPinfo, &newdataPinfo, 
		    		    &newbid, &newtid, newrecord, newlength, 
				    ir->rcb_s_key_ptr, alloc_flag, dberr);
		    if (DB_FAILURE_MACRO(status))
			break;

		    /*
		    ** Code below depends on the fact that leaf with the old
		    ** tuple is fixed into rcb_other.page.
		    ** If the replace tuple has moved because of a split, then
		    ** fix the correct page into rcb_other.page.
		    */
		    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
						 ir->rcb_other.page) !=
			ir->rcb_fetchtid.tid_tid.tid_page)
		    {
			status = dm0p_unfix_page(ir, DM0P_UNFIX,
			    &ir->rcb_other, dberr);
			if (DB_FAILURE_MACRO(status))
			    break;
			status = dm0p_fix_page(ir,
			    (DM_PAGENO) ir->rcb_fetchtid.tid_tid.tid_page,
			    DM0P_WRITE | DM0P_NOREADAHEAD, &ir->rcb_other,
			    dberr);
			if (DB_FAILURE_MACRO(status))
			    break;
		    }
		    break;
		}

		if (status != E_DB_OK)
		    break;
	    }	

	    /*
	    **  Compute the values for old (bid,tid) and new (bid,tid).
	    **  This calculation has to be deferred until after the above
	    **  allocations have taken place.  This is required because a BTREE/
	    **  RTREE split can move the current position of old (bid,tid).  An
	    **  allocation is noted when the newbid or newtid has a legal
	    **  value in it.
	    */

	    if ((it->tcb_rel.relspec == TCB_BTREE) ||
		(it->tcb_rel.relspec == TCB_RTREE))
	    {
		oldbid.tid_i4 = ir->rcb_fetchtid.tid_i4;
		oldtid.tid_i4 = ir->rcb_currenttid.tid_i4;
		if (newbid.tid_i4 == NOT_ALLOCATED)
		    newbid.tid_i4 = oldbid.tid_i4;
	    }
	    else
		oldtid.tid_i4 = ir->rcb_currenttid.tid_i4;
	    if (newtid.tid_i4 == NOT_ALLOCATED)
		newtid.tid_i4 = oldtid.tid_i4;

	    /*
	    ** If the index is compressed, pass the compressed record to replace
	    */
	    if (it->tcb_rel.relcomptype != TCB_C_NONE)
		newrecord = crec;

	    /* if encrypted, pass the encrypted record to replace */
	    if (t->tcb_data_rac.encrypted_attrs)
		newrecord = ir->rcb_erecord_ptr;

	    /* This is constant */
	    delta_end = it->tcb_rel.relwid;

	    /*  Perform the actual replace. */
	    switch (it->tcb_rel.relspec)
	    {
	    case TCB_ISAM:
	    case TCB_HASH:
		status = dm1r_replace(ir, &oldtid, oldrecord, oldlength, &newdataPinfo,
			    &newtid, newrecord, newlength, (i4)0, 
			    delta_start, delta_end, dberr);
		break;

	    case TCB_BTREE:
		status = dm1b_replace(ir, &oldbid, &oldtid, oldrecord,
		    oldlength, &newleafPinfo, &newdataPinfo, &newbid, &newtid,
		    newrecord, newlength, ir->rcb_srecord_ptr, (i4)0,
		    delta_start, delta_end, dberr);
		break;

	    case TCB_RTREE:
		status = dm1m_replace(ir, &oldbid, &oldtid, oldrecord,
		    oldlength, ancestors, ancestor_level, 
		    &newleafPinfo, &newdataPinfo, &newbid, &newtid, newrecord,
		    newlength, ir->rcb_s_key_ptr, (i4)0, dberr);
		break;
	    }
	}
    } while ( status && flag & DM2R_RAAT &&
	       (dberr->err_code ==  E_DM004D_LOCK_TIMER_EXPIRED ||
		dberr->err_code ==  E_DM9043_LOCK_TIMEOUT) &&
	       (dberr->err_code = E_DM0173_RAAT_SI_TIMER) &&
	       (si_retries--) );

    if ( status )
    {
	if ( dberr->err_code == E_DM0045_DUPLICATE_KEY ||
	     dberr->err_code == E_DM0046_DUPLICATE_RECORD )
	{
	    if (it->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE)
		SETDBERR(dberr, 0, E_DM0151_SIDUPLICATE_KEY_STMT);
	    else
		SETDBERR(dberr, 0, E_DM0048_SIDUPLICATE_KEY);
	}
    }

    if ( newdataPinfo.page && 
	(loc_status = dm0p_unfix_page(ir, DM0P_UNFIX, &newdataPinfo, &local_dberr)) &&
	 status == E_DB_OK )
    {
	status = loc_status;
	*dberr = local_dberr;
    }

    if ( newleafPinfo.page && 
	(loc_status = dm0p_unfix_page(ir, DM0P_UNFIX, &newleafPinfo, &local_dberr)) &&
	 status == E_DB_OK )
    {
	status = loc_status;
	*dberr = local_dberr;
    }

    /* Give up the SI RCB */
    if ( ir && (flag & DM2R_SI_AGENT) == 0 &&
	dm2r_return_rcb(&ir, &local_dberr) && status == E_DB_OK )
    {
	*dberr = local_dberr;
	status = E_DB_ERROR;
    }

    if ( status )
    {
	/*
	** If NONEXT or NOPART then we failed trying to look up the
	** secondary index tuple in the index table.
	*/
	if (dberr->err_code == E_DM0055_NONEXT || dberr->err_code == E_DM0056_NOPART)
	{
	    uleFormat(NULL, E_DM9385_NO_INDEX_TUPLE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, r->rcb_si_oldtid.tid_i4);
	    SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
	}

	if (dberr->err_code > E_DM_INTERNAL)
	{
	    if ( dberr->err_code != E_DM9387_INDEX_UPDATE_ERROR )
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9386_SEC_INDEX_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
		sizeof(DB_TAB_NAME), &it->tcb_rel.relid.db_tab_name, 
		0, r->rcb_si_oldtid.tid_i4);
	    SETDBERR(dberr, 0, E_DM9387_INDEX_UPDATE_ERROR);
	}

	/*
	** If SI threads, set the worst
	** status in the RTB.
	*/
	if ( flag & DM2R_SI_AGENT )
	{
	    CSp_semaphore(1, &rtb->rtb_cond_sem);
	    if ( rtb->rtb_status == E_DB_OK )
	    {
		rtb->rtb_status = status;
		rtb->rtb_error = *dberr;
	    }
	    CSv_semaphore(&rtb->rtb_cond_sem);
	}
    }

    return (status);
}

/*{
** Name: base_delete_put   - Execute replace on base table as delete/put.
**
** Description:
**      Execute NON in place replace on base table as follows:
**        - Delete old record
**        - Allocate space (duplicate checking already done)
**        - Put new record
**
**      NON in place replaces are being processed as delete/put to
**      follow the stricter page fixing protocol that is required for
**      row locking.
**
**
** Inputs:
**      rcb                             RCB of base table.
**	action				whether to do dupcheck and/or allocate
**      flag
**      newrecord                       Pointer to new record
**      oldlength                       Length of old record
**      newlength                       Length of new record
**      oldtid
**      oldbid
**      newtid
**      newbid
**      newdata_p
**      newleaf_p
**
** Outputs:
**	err_code			Reason for error return status.
**      oldbid                          May change if we need to refetch
**      newtid,newbid                   Tid,bid of new tuple
**
** Outputs:
**	err_code			Reason for error return status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    None. Should be only called for row locking.
**
** History:
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**     29-jul-97 (stial01)
**          base_delete_put() Pass opflag DM1C_REPLACE to dm1* put/del routines
**          Space reclamation for leaf entries depends on page size not lockmode
*/
static DB_STATUS
base_delete_put(
    DMP_RCB     *rcb,
    i4     action,
    i4     flag,
    char        *newrecord,
    i4     oldlength,
    i4     newlength,
    DM_TID      *oldtid,
    DM_TID      *oldbid,
    DM_TID      *newtid,
    DM_TID      *newbid,
    DMP_PINFO   *newdata_p,
    DMP_PINFO   *newleaf_p,
    DB_ERROR    *dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = rcb->rcb_tcb_ptr;
    bool                dupcheck = FALSE;
    DB_STATUS           status = E_DB_OK;
    i4			alloc_flag = DM1C_DUPLICATES;
    char                *oldrecord = r->rcb_record_ptr;
    i4                  orig_action = action;
    LK_LKID             newval_lkid;
    char		*crec = (PTR)0;
    i4		    *err_code = &dberr->err_code;
    i4			loc_err;
    DB_STATUS		local_status;
    DB_ERROR		local_dberr;
    bool		new_value_lock = FALSE;

    CLRDBERR(dberr);

    if (t->tcb_rel.relencflags & TCB_ENCRYPTED)
	crec = r->rcb_erecord_ptr;
    if (t->tcb_rel.relcomptype != TCB_C_NONE)
	crec = r->rcb_srecord_ptr;

    /*
    **	Compute the values for old (bid,tid).
    */
    if ((t->tcb_rel.relspec == TCB_BTREE) || (t->tcb_rel.relspec == TCB_RTREE))
    {
	oldbid->tid_i4 = r->rcb_fetchtid.tid_i4;
	oldtid->tid_i4 = r->rcb_currenttid.tid_i4;
    }
    else
	oldtid->tid_i4 = r->rcb_currenttid.tid_i4;

    if (t->tcb_rel.relkeys) 
    {
	/* Lock New key/row value */
	status = dm1r_lock_value(r, (i4)DM1R_LK_PHYSICAL, newrecord, dberr);
	if (status != E_DB_OK)
	    return (status);
	
	/* Save the lkid so we don't have to rehash to unlock */
	STRUCT_ASSIGN_MACRO(r->rcb_val_lkid, newval_lkid);
	new_value_lock = TRUE;
    }

    /*
    ** If statement level uniqueness constraint enforced using primary index, 
    ** we need to dupcheck before starting the delete
    ** Since we are not allocating, this will not have any effect on
    ** the leaf position for the old record.
    ** If unique key, we can't allocate space before the delete, in case
    ** the key is not changing
    */
    if ( action & DUP_CHECK && flag & DM2R_DUP_CONTINUE )
    {
	action &= ~DUP_CHECK;
	switch (t->tcb_table_type)
	{
	case TCB_HEAP:
	    status = dm1s_allocate(r, newdata_p, newtid, newrecord, crec, 
		newlength, DM1C_ONLYDUPCHECK, dberr);
	    break;

	case TCB_HASH:
	    status = dm1h_allocate(r, newdata_p, newtid, newrecord, crec,
		newlength, DM1C_ONLYDUPCHECK, dberr);
	    break;

	case TCB_ISAM:
	    status = dm1i_allocate(r, newdata_p, newtid, newrecord, crec,
		newlength, DM1C_ONLYDUPCHECK, dberr);
	    break;

	case TCB_BTREE:
	    status = dm1b_allocate(r, newleaf_p, newdata_p, newbid,
		    newtid, newrecord, crec, newlength, r->rcb_s_key_ptr, 
		    DM1C_ONLYDUPCHECK, dberr);
	    break;

	case TCB_RTREE:
	    status = E_DB_ERROR; /* primary rtree indexes not supported */
	    break;
	}
	if (status != E_DB_OK)
	{
	    if (dberr->err_code == E_DM0045_DUPLICATE_KEY &&
		(t->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE) != 0)
	    {
		SETDBERR(dberr, 0, E_DM0150_DUPLICATE_KEY_STMT);
	    }
	    return (E_DB_ERROR);
	}
    }

    /*
    ** We need a key value lock if duplicate checking OR non-inplace replace
    ** For duplicate checking, get key value locks
    */
    if ((t->tcb_rel.relstat & TCB_UNIQUE) || no_dup_rows(t))
    {
	/* Lock Old key/row value */
	status = dm1r_lock_value(r, (i4)DM1R_LK_PHYSICAL, r->rcb_record_ptr, 
					dberr);
	if (status != E_DB_OK)
            return (status);
    }

    /*
    ** Perform the deletion by access method.
    */
    switch (t->tcb_table_type)
    {
    case TCB_HEAP:
    case TCB_ISAM:
    case TCB_HASH:
	status = dm1r_delete(r, &r->rcb_fetchtid, DM1C_BYPOSITION, 
				(i4)DM1C_MREPLACE, dberr);
	break;

    case TCB_BTREE:
	status = dm1b_delete(r, &r->rcb_fetchtid, &r->rcb_currenttid,
				(i4)DM1C_MREPLACE, dberr);
	break;

    case TCB_TAB_GATEWAY:
	/* We should not be coming here for gateways */
	SETDBERR(dberr, 0, E_DM9A0D_GATEWAY_DELETE);
	status = E_DB_ERROR;
	break;
    }

    if (status != E_DB_OK)
	return (status);

    if ((t->tcb_rel.relstat & TCB_UNIQUE) || no_dup_rows(t))
    {
	/* Unlock the value lock */
	status = dm1r_unlock_value(r, r->rcb_record_ptr, dberr);
	if (status != E_DB_OK)
	    return (status);
    }

    if (action & DUP_CHECK)
	alloc_flag = DM1C_NODUPLICATES;

    /*
    ** Allocate space for new tuple, and do duplicate checking if required.
    */
    switch (t->tcb_table_type)
    {
	case TCB_HEAP:
	    status = dm1s_allocate(r, newdata_p, newtid,
			    newrecord, crec, newlength, alloc_flag, dberr);
	    break;

	case TCB_HASH:
	    status = dm1h_allocate(r, newdata_p, newtid,
			    newrecord, crec, newlength, alloc_flag, dberr);
	    break;

	case TCB_ISAM:
	    status = dm1i_allocate(r, newdata_p, newtid,
			    newrecord, crec, newlength, alloc_flag, dberr);
	    break;

	case TCB_BTREE:
	    status = dm1b_allocate(r, newleaf_p, newdata_p,
			newbid, newtid, newrecord, crec,
			newlength, r->rcb_s_key_ptr, alloc_flag, dberr);
	    break;

	case TCB_RTREE:
	    status = E_DB_ERROR; /* primary rtree indexes not supported */
	    break;
    }

    if (status != E_DB_OK)
    {
	if (dberr->err_code == E_DM0045_DUPLICATE_KEY &&
	    (t->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE) != 0)
	{
	    SETDBERR(dberr, 0, E_DM0150_DUPLICATE_KEY_STMT);
	}
	return (status);
    }

    if (t->tcb_rel.relcomptype != TCB_C_NONE)
	newrecord = r->rcb_srecord_ptr;

    /*	Insert new tuple */
    switch (t->tcb_table_type)
    {
	case TCB_HEAP:
	case TCB_HASH:
	case TCB_ISAM:
	    status = dm1r_put(r, newdata_p, newtid, newrecord, (char *)NULL,
			    newlength, (i4)DM1C_MREPLACE, dberr);
	    break;

	case TCB_BTREE:
	    status = dm1b_put(r, newleaf_p, newdata_p, newbid, newtid,
			newrecord, r->rcb_s_key_ptr, newlength,
			(i4)DM1C_MREPLACE, dberr);
	    break;
    }

    /* 
    ** Release the value lock (after the put)
    ** Previously we were releasing after the allocate, before the put
    ** causing btree errors because dm1b_dupcheck DM1B_SKIP_DELETED_KEY_MACRO
    ** was true for another transaction trying to insert the same key
    ** (t1-alloc, t2-alloc-uses-t1-reserved-leaf-entry, t2-put, t1-put-error)
    ** Since the btree reserved key entry doesn't have a valid tid to 
    ** waitlock on, hold the value lock until after the put.
    */
    if ( new_value_lock )
    {
	STRUCT_ASSIGN_MACRO(newval_lkid, r->rcb_val_lkid);
        local_status = dm1r_unlock_value(r, newrecord, &local_dberr);
	if (local_status)
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL, &loc_err, 0);
    }

    /* rcb update */
    if (status == E_DB_OK && r->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE)
    {
	i4             del_reclaim_space;

	del_reclaim_space = FALSE;

	status = dm1b_rcb_update(r, oldbid, newbid,
			    (i4)0, DM1C_MREPLACE,
			    del_reclaim_space,
			    oldtid, newtid, r->rcb_s_key_ptr, dberr);
    }

    /* Set tid so it can be returned to caller. */

    r->rcb_currenttid.tid_i4 = newtid->tid_i4;
    return (status);
}


/*{
** Name: base_replace	 - Execute replace on base table.
**
** Description:
**      Replace a tuple in base table
**      If row locking, error if ALLOCATE needed.
**
**
** Inputs:
**      rcb                             RCB of base table.
**      action                          IN_PLACE,ALLOCATE,DUP_CHECK
**      flag
**      newrecord                       Pointer to new record
**      oldlength                       Length of old record
**      newlength                       Length of new record
**      oldtid
**      oldbid
**      newtid
**      newbid
**      newdata_p
**      newleaf_p
**
** Outputs:
**	err_code			Reason for error return status.
**      oldbid                          May change if we need to refetch
**      newtid,newbid                   Tid,bid of new tuple
**
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
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**	01-oct-1998 (nanpr01)
**	    Update optimization project.
**      04-nov-99 (stial01)
**          base_replace() must pass uncompressed record to allocate rtn
*/
static DB_STATUS
base_replace(
    DMP_RCB     *rcb,
    i4     action,
    i4     flag,
    char        *newrecord,
    i4     oldlength,
    i4     newlength,
    DM_TID      *oldtid,
    DM_TID      *oldbid,
    DM_TID      *newtid,
    DM_TID      *newbid,
    DMP_PINFO   *newdata_p,
    DMP_PINFO   *newleaf_p,
    i4		delta_start,
    i4		delta_end,
    DB_ERROR    *dberr)
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    DB_STATUS           status = E_DB_OK;
    i4             opflag = DM1C_BYPOSITION;
    char                *oldrecord = r->rcb_record_ptr;
    i4			alloc_action;
    i4			loc_err;
    char		*crec = (PTR)0;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (t->tcb_rel.relencflags & TCB_ENCRYPTED)
	crec = r->rcb_erecord_ptr;
    if (t->tcb_rel.relcomptype != TCB_C_NONE)
	crec = r->rcb_srecord_ptr;

    if (action != IN_PLACE)
    {
	if (action == DUP_CHECK)
	    alloc_action = DM1C_ONLYDUPCHECK; /* dup check only */
	else if (action == ALLOCATE)
	    alloc_action = DM1C_DUPLICATES;   /* allocate only */
	else 
	    alloc_action = DM1C_NODUPLICATES; /* dup check and allocate */

	if ( (row_locking(r) || crow_locking(r)) && (t->tcb_rel.relkeys))
	{
	    /* Lock New key/row value */
	    status = dm1r_lock_value(r, (i4)DM1R_LK_PHYSICAL, newrecord, 
				dberr);
	    if (status != E_DB_OK)
		return (status);
	}

	switch (t->tcb_table_type)
	{
	case TCB_HEAP:
	    status = dm1s_allocate(r, newdata_p, newtid, newrecord, crec,
		newlength, alloc_action, dberr);
	    break;

	case TCB_HASH:
	    status = dm1h_allocate(r, newdata_p, newtid, newrecord, crec,
		newlength, alloc_action, dberr);
	    break;

	case TCB_ISAM:
	    status = dm1i_allocate(r, newdata_p, newtid, newrecord, crec,
		newlength, alloc_action, dberr);
	    break;

	case TCB_BTREE:
	    status = dm1b_allocate(r, newleaf_p, newdata_p, newbid,
		    newtid, newrecord, crec, newlength, r->rcb_s_key_ptr, 
		    alloc_action, dberr);
	    break;

	case TCB_RTREE:
	    status = E_DB_ERROR; /* primary rtree indexes not supported */
	    break;
	}

	if (t->tcb_table_type == TCB_BTREE || t->tcb_table_type == TCB_RTREE)
	{
	    if (alloc_action == DM1C_ONLYDUPCHECK)
	    {
		newbid->tid_i4 = NOT_ALLOCATED;
		newtid->tid_i4 = NOT_ALLOCATED;
	    }
	    else if (status == E_DB_OK)
	    {
		/*
		** Code below depends on the fact that leaf with the old
		** tuple is fixed into rcb_other.page.
		** If the replace tuple has moved because of a split, then
		** fix the correct page into rcb_other.page.
		** Note if V2 pages, we will reposition in dm1b_replace
		** if the allocate caused the leaf entry to move.
		*/
		if (DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
		    r->rcb_other.page) != r->rcb_fetchtid.tid_tid.tid_page)
		{
		    status = dm0p_unfix_page(r, DM0P_UNFIX,
			&r->rcb_other, dberr);

		    if (status == E_DB_OK)
		    {
			status = dm0p_fix_page(r,
			    (DM_PAGENO) r->rcb_fetchtid.tid_tid.tid_page,
			    DM0P_WRITE | DM0P_NOREADAHEAD, 
			    &r->rcb_other, dberr);
		    }
		}
	    }
	}

	/* Once the record is on the page we can release the value lock */
	if ((status == E_DB_OK) && (row_locking(r) || crow_locking(r)) 
			&& (t->tcb_rel.relkeys))
	{
	    status = dm1r_unlock_value(r, newrecord, dberr);
	    if (status != E_DB_OK)
		return (status);
	}
    }

    if (status != E_DB_OK)
    {
	if (newdata_p->page)
	    status = dm0p_unfix_page(r, DM0P_UNFIX, newdata_p, &local_dberr);
	if (newleaf_p->page)
	    status = dm0p_unfix_page(r, DM0P_UNFIX, newleaf_p, &local_dberr);

	if (dberr->err_code == E_DM0045_DUPLICATE_KEY &&
	    (t->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE) != 0)
	{
	    SETDBERR(dberr, 0, E_DM0150_DUPLICATE_KEY_STMT);
	}
	return (E_DB_ERROR);
    }

    if ((row_locking(r) || crow_locking(r)) && (action & DUP_CHECK))
    {
	/* Lock Old key/row value */
	status = dm1r_lock_value(r, (i4)DM1R_LK_PHYSICAL, oldrecord, 
				dberr);
	if (status != E_DB_OK)
	    return (status);
    }

    /*
    **	Compute the values for old (bid,tid) and new (bid,tid).
    **	This calculation has to be deferred until after the above
    **  allocations have taken place.  This is required because
    **	a BTREE/RTREE split can move the current position of old (bid,tid).
    **	An allocation is noted when the newbid or newtid has a legal
    **	value in it.
    */
    if ((t->tcb_rel.relspec == TCB_BTREE) || (t->tcb_rel.relspec == TCB_RTREE))
    {
	oldbid->tid_i4 = r->rcb_fetchtid.tid_i4;
	oldtid->tid_i4 = r->rcb_currenttid.tid_i4;
	if ( (action & ALLOCATE) == 0 )
	    newbid->tid_i4 = oldbid->tid_i4;
    }
    else
	oldtid->tid_i4 = r->rcb_currenttid.tid_i4;

    if ( (action & ALLOCATE) == 0 )
        newtid->tid_i4 = oldtid->tid_i4;

    /*
    ** If the table is compressed, then pass the compressed record to replace
    ** We pass the uncompressed record to allocate, lock/unlock value routines
    */
    if (t->tcb_rel.relencflags & TCB_ENCRYPTED)
	newrecord = r->rcb_erecord_ptr;
    if (t->tcb_rel.relcomptype != TCB_C_NONE)
	newrecord = r->rcb_srecord_ptr;

    /*
    ** Perform the actual replace according to the access method.
    */
    switch (t->tcb_table_type)
    {
	case TCB_HEAP:
	case TCB_ISAM:
	case TCB_HASH:
	    status = dm1r_replace(r, oldtid, oldrecord, oldlength,
		newdata_p, newtid, newrecord, newlength, opflag, 
		delta_start, delta_end, dberr);
	    break;

	case TCB_BTREE:
	    status = dm1b_replace(r, oldbid, oldtid, oldrecord, oldlength,
		newleaf_p, newdata_p, newbid, newtid, newrecord, newlength,
		r->rcb_s_key_ptr, (i4)((action & ALLOCATE) == 0),
		delta_start, delta_end, dberr);
	    break;

	case TCB_TAB_GATEWAY:
	    /*
	    ** Call Gateway to do replace.
	    */
	    status = dmf_gwr_replace(r, oldrecord, oldlength,
			newrecord, newlength, oldtid, newtid, dberr);
	    break;
    }

    if ((status == E_DB_OK) && (row_locking(r) || crow_locking(r)) 
    		&& (action & DUP_CHECK))
    {
	status = dm1r_unlock_value(r, oldrecord, dberr);
	if (status != E_DB_OK)
	    return (status);
    }

    return (status);
}


/*{
** Name: dm2r_fetch_record - Fetch a record
**
** Description:
**      This function will fetch the current record in a scan or will
**      fetch a record by tuple identifier.
**      This functionality is needed by dm2r_delete, dm2r_replace.
**
**
** Inputs:
**      rcb                             RCB of table.
**      tid                             If flag == DM2R_BYTID, then this is the
**                                      tuple identifier of the record to be
**                                      fetched.
**      flag                            DM2R_BYPOSITION or DM2R_BYTID.
**      opflag                          DM1C_MDELETE or DM1C_MREPLACE
**
** Outputs:
**	err_code			Reason for error return status.
**      fetched_tid                     Save the tid we actually fetched if
**                                      DM2R_BYPOSITION.
**
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
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Created.
**      21-apr-97 (stial01)
**          dm2r_fetch_record() Changed parameters to dm1b_bybid_get()
**      14-may-97 (dilma04)
**          Cursor Stability Project. After record is fetched:
**          - set RCB_*_UPDATED flags in the rcb_state;
**          - convert all physical locks to logical;
**          - turn RCB_CSRR_LOCK flag off.
**      21-may-97 (stial01)
**          Row locking: No more LK_PH_PAGE locks, so page(s) can stay fixed.
**          Update rcb_fet* info after we search to find key.
**      07-jul-98 (stial01)
**          dm2r_fetch_record() convert physical locks to logical only if CS.
**      09-dec-98 (stial01)
**          dm2r_fetch_record() fixed error handling for lock conversion failure
**	02-apr-99 (nanpr01)
**	    Update mode lock - Convert to logical from physical where 
**	    necessary.
**	30-aug-99 (nanpr01)
**	    bug 98575 : We need to convert the leaf page physical lock 
**	    in repeatable read to logical lock before we update. Otherwise 
**	    there is a possibility that the undo code will try to convert it to 
**	    logical and will fail with out of lock error.
**      20-oct-99 (stial01)
**          dm2r_fetch_record() Fixed error handling after lock convert 
**	16-Aug-2001 (inifa01)
**	    X-INT of (stial01) unsubmitted fix to bug 105548.
**	    undo recovery fails, pass abort recovery fails, and database is marked 
**	    inconsisent. Check return status on call to dm0p_trans_page_lock to ensure 
**	    LK_U to LK_X conversion was successful.        
**	26-Jun-2009 (kibro01) b122169
**	    Have to reread if we have no data OR if this row has been updated, so the
**	    rcb_record_ptr gets updated and we don't end up logging record A changing
**	    to B, followed by A changing to C followed by A changing to D, etc.
*/
static DB_STATUS
dm2r_fetch_record(
DMP_RCB             *rcb,
DM_TID              *tid,
i4             flag,
i4             opflag,
DB_ERROR            *dberr )
{
    DMP_RCB             *r = rcb;
    DMP_TCB             *t = rcb->rcb_tcb_ptr;
    DB_STATUS           status = E_DB_OK;
    i4             i;
    i4                  local_err;
    i4			input_rcb_state = r->rcb_state;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;
    DM_TID		localtid;


    CLRDBERR(dberr);

    /*
    ** If crow locking, then we need to lock the row being deleted/replaced
    ** and check the Root page for update conflicts.
    **
    ** To do that, we first need to fix the page matching the TID,
    ** if it's not already fixed.
    */
    if ( crow_locking(r))
    {
        if ( flag & DM2R_BYTID )
	    localtid.tid_i4 = tid->tid_i4;
	else
	    localtid.tid_i4 = r->rcb_currenttid.tid_i4;

	if ( r->rcb_data.page &&
	     DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, r->rcb_data.page)
	         != localtid.tid_tid.tid_page )
	{
	    status = dm0p_unfix_page(r, DM0P_UNFIX, 
	    			     &r->rcb_data, dberr);
	}

	if ( status == E_DB_OK && r->rcb_data.page == (DMPP_PAGE*)NULL )
	    status = dm0p_fix_page(r, (DM_PAGENO)localtid.tid_tid.tid_page,
				   DM0P_WRITE, &r->rcb_data, dberr);
	if ( status == E_DB_OK )
	    status = dm1r_crow_lock(r, 0, &localtid, &r->rcb_data, dberr);

	if ( status )
	    return(status);
    }

    /*	Check for delete/replace by TID. */

    if (flag & DM2R_BYTID)
    {
	/*    merged by schang 05/26/92
	** Check if tid access is allowed.  Allowed for RMS Gateway.
	** As they are added, other non-relational Gateways will have
	** to figure out how to set this bit (done in dm2t).
	*/
	if (t->tcb_no_tids)
	{
	    uleFormat(NULL, E_DM9317_DM2R_TID_ACCESS_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    if (opflag == DM1C_MDELETE)
		SETDBERR(dberr, 0, E_DM9311_DM2R_DELETE);
	    else
		SETDBERR(dberr, 0, E_DM9315_DM2R_REPLACE);
	}

	/*
	**  Mark the RCB as not positioned and read the record to
	**  delete/replace into rcb_record_ptr.
	*/

	r->rcb_state &= ~(RCB_POSITIONED);
	r->rcb_currenttid.tid_i4 = tid->tid_i4;
	r->rcb_lowtid.tid_i4 = tid->tid_i4;
	r->rcb_fetchtid.tid_i4 = tid->tid_i4;

	/*
	** Go directly to data page for record, regardless of access method
	*/

	/*
	** schang (04/16/92) - GW merge.
	*/
	if ( t->tcb_table_type == TCB_TAB_GATEWAY )
	{
	    /*
	    ** Fetch for replace did dm2r_get,
	    ** Fetch for delete did dmf_gwr_get
	    ** dmf_gwr_get should be ok for replace and delete (stial01)
	    */
	    status = dmf_gwr_get(r, tid, r->rcb_record_ptr, DM1C_BYTID,
				dberr);
	}
	else
	{
	    /* Have to reread if we have no data OR if this row has been
	    ** updated, so the rcb_record_ptr gets updated and we don't
	    ** end up logging record A changing to B, followed by A changing
	    ** to C followed by A changing to D, etc (kibro01) b122169
	    */
	    if (((flag & DM2R_HAVE_DATA) == 0) ||
		((r->rcb_state & RCB_RECORD_PTR_VALID) == 0))
		status = dm1r_get(r, tid, r->rcb_record_ptr, DM1C_BYTID, 
			dberr);
	}

	if (status != E_DB_OK)
	    return (status);

	/*
	** For BTREE must point to index record associated with
	** record from index page.
	*/

	/* schang : gw merge. schang: 05/26/92        */
	if ( t->tcb_table_type == TCB_BTREE && (flag & DM2R_IN_PLACE) == 0)
	{
	    if (r->rcb_other.page)
	    {
		status = dm0p_unfix_page(r, DM0P_UNFIX,
				      &r->rcb_other, dberr);
		if (status != E_DB_OK)
		    return (status);
	    }

	    for (i = 0; i < t->tcb_keys; i++)
		MEcopy(&r->rcb_record_ptr[t->tcb_key_atts[i]->offset],
		    t->tcb_key_atts[i]->length,
		    &r->rcb_s_key_ptr[t->tcb_key_atts[i]->key_offset]);

	    /* dm1b_search DM1C_TID, DM1C_EXACT will also fetch the key */
	    status = dm1b_search(r, r->rcb_s_key_ptr, t->tcb_keys,
		DM1C_TID, DM1C_EXACT,
		&r->rcb_other, &r->rcb_lowtid,
		&r->rcb_currenttid, dberr);

	    if (status != E_DB_OK)
		return (status);

	    if (t->tcb_leaf_rac.att_count != t->tcb_index_rac.att_count)
	    {
		TRdisplay("base table leaf atts != index atts\n");
		return (E_DB_ERROR);
	    }

	    r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;

	    /*
	    ** If no data page, refix data page
	    ** It is not necessary to re get tuple
	    */
	    if (!r->rcb_data.page)
	    {
		i4 fix_action = DM0P_WRITE;

		if (r->rcb_access_mode == RCB_A_READ)
		    fix_action = DM0P_READ;
		status = dm0p_fix_page(r,
				(DM_PAGENO)tid->tid_tid.tid_page,	
				fix_action, &r->rcb_data, dberr);
		if (status != E_DB_OK)
		    return (status);
	    }
	} /* end if (t->tcb_rel.relspec == TCB_BTREE) */
        else if ( t->tcb_table_type == TCB_RTREE)
	{
	    /*  Release any fixed index page. */
	    if (r->rcb_other.page)
	    {
		status = dm0p_unfix_page(r, DM0P_UNFIX,
                                      &r->rcb_other, dberr);
		if (status != E_DB_OK)
		    return (status);
	    }

	     /*  Build the keys for the old record. */
	    for (i = 0; i < t->tcb_keys; i++)
		MEcopy(&r->rcb_record_ptr[t->tcb_key_atts[i]->offset],
		    t->tcb_key_atts[i]->length,
		    &r->rcb_s_key_ptr[t->tcb_key_atts[i]->key_offset]);

	    /*  Search for the matching index entry. */
	    status = dm1m_search(r, r->rcb_s_key_ptr, t->tcb_keys,
                DM1C_TID, DM1C_EXACT, &r->rcb_other, &r->rcb_lowtid,
                &r->rcb_currenttid, dberr);

	    if (status != E_DB_OK)
		return (status);
	    r->rcb_fetchtid.tid_i4 = r->rcb_lowtid.tid_i4;
	} /* end else if (t->tcb_rel.relspec == TCB_RTREE) */
    }
    else
    {
	if ((r->rcb_state & RCB_POSITIONED) == 0)
	{
	    SETDBERR(dberr, 0, E_DM0074_NOT_POSITIONED);
	    return (E_DB_ERROR);
	}
	if (r->rcb_state & RCB_FETCH_REQ)
	{
	    if ((r->rcb_fetchtid.tid_tid.tid_page == 0)
			  &&
		(r->rcb_fetchtid.tid_tid.tid_line == DM_TIDEOF))
	    {
		*err_code = E_DM0044_DELETED_TID;
		return (E_DB_ERROR);
	    }

	    /* Fetch the record pointed to by rcb_fetchtid. */

	    /* schang : gw merge. schang: 05/26/92 */
	    switch ( t->tcb_table_type )
	    {
		case TCB_BTREE:
		    status = dm1b_bybid_get(r, opflag, dberr);
		    break;
 	        case TCB_RTREE:
		    status = dm1m_bybid_get(r, &r->rcb_fetchtid,
				   &r->rcb_currenttid,
                                   r->rcb_record_ptr, dberr);
		    break;
		case TCB_TAB_GATEWAY:
		    status = dmf_gwr_get(r, &r->rcb_fetchtid, r->rcb_record_ptr,
				    DM1C_BYTID, dberr);
		    break;
		default:
		    status = dm1r_get(r, &r->rcb_fetchtid, r->rcb_record_ptr,
				    DM1C_BYTID, dberr);
		    break;
	    }	/* end switch */
	    if (status != E_DB_OK)
		return (status);

	    r->rcb_state &= ~RCB_FETCH_REQ;
	}
    }

    r->rcb_state |= (RCB_TABLE_UPDATED | RCB_DPAGE_UPDATED |
		     RCB_ROW_UPDATED | RCB_RECORD_PTR_VALID);
    if (t->tcb_table_type == TCB_BTREE && (flag & DM2R_IN_PLACE) == 0)
        r->rcb_state |= RCB_LPAGE_UPDATED;

    if ( crow_locking(r) )
    {
	/*
	** we might have gotten row lock at top of this routine,
	** or in dm1b_bybid_get
	*/
	if (DMZ_SES_MACRO(37)) /* FIX ME reserve new session trace point  */
	{
	    DB_STATUS    tmp_status;

	    /* Make sure we already have the lock if delete from base table */
	    tmp_status = dm1r_check_lock(r, &localtid);
	    if (tmp_status != E_DB_OK)
	    {
		TRdisplay("dm2r_fetch_record: No row lock for %~t\n",
		    sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
	    }
	}
    }

    if ( !crow_locking(r) )
    {
	/*
	** If isolation level is CS convert all physical locks to
	** logical and turn RCB_CSRR_LOCK flag off.
	*/
	if (r->rcb_state & RCB_CSRR_LOCK 
	    && (r->rcb_iso_level == RCB_CURSOR_STABILITY ||
		    (input_rcb_state & RCB_RECORD_PTR_VALID) == 0)
	    && r->rcb_lk_type != RCB_K_TABLE)
	{
	     i4 lock_mode = LK_X;
	     i4 lock_action = 0;
	     i4 new_lock = FALSE;
	     LK_LOCK_KEY	save_lock_key;

	     /* Turn off the bit so that dm1r_lock_row escalates to LK_X */
	     r->rcb_state &= ~RCB_UPD_LOCK;

	     if (row_locking(r) && (t->tcb_rel.relstat & TCB_INDEX) == 0)
	     {
		 if (DMZ_SES_MACRO(33))
		 {
		     DB_STATUS    tmp_status;

		     /* Make sure we already have the lock */
		     tmp_status = dm1r_check_lock(r, &r->rcb_currenttid);
		     if (tmp_status != E_DB_OK)
		     {
			TRdisplay("dm2r_fetch_record: No row lock for %~t\n",
			    sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
			SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
			return (E_DB_ERROR);
		     }
		 }

		 /*
		 ** Convert physical to logical
		 ** The only reason this should fail is if we hit transaction
		 ** lock limit and deadlock trying to escalate
		 */
		 status = dm1r_lock_row(r, DM1R_LK_CONVERT|DM1R_CLEAR_LOCKED_TID, 
			    &r->rcb_currenttid, &new_lock, 
			    &save_lock_key, &local_dberr);
		 if (status != E_DB_OK)
		 {
		     *dberr = local_dberr;
		     uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &local_err, 0);
		     return (E_DB_ERROR);
		 }

		 r->rcb_csrr_flags &= ~RCB_CS_ROW; 
		 lock_mode = LK_IX;
	     }

	     status = dm0p_trans_page_lock(r, t, 
		 r->rcb_currenttid.tid_tid.tid_page,
		 DM0P_LK_CONVERT,
		 lock_mode, &lock_action, (LK_LKID *)0, dberr);
	    /* Note conversion from LK_U to LK_X can fail, check status */
	    if (status != E_DB_OK)
		return (E_DB_ERROR);
	     r->rcb_csrr_flags &= ~RCB_CS_DATA;

	     if (r->rcb_other.page != NULL && (flag & DM2R_IN_PLACE) == 0)
	     {
		 status =  dm0p_trans_page_lock(r, r->rcb_tcb_ptr, 
		     DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		     r->rcb_other.page),
		     DM0P_LK_CONVERT, lock_mode, 
		     &lock_action, (LK_LKID *)0, dberr);
		/* Note conversion from LK_U to LK_X can fail, check status */
		if (status != E_DB_OK)
		    return (E_DB_ERROR);
		 r->rcb_csrr_flags &= ~RCB_CS_LEAF;
	     }

	     r->rcb_state &= ~RCB_CSRR_LOCK;
	}
	else if (((r->rcb_iso_level == RCB_REPEATABLE_READ) ||
		  (r->rcb_iso_level == RCB_SERIALIZABLE)) && 
		 (r->rcb_lk_type != RCB_K_TABLE) && (r->rcb_state & RCB_UPD_LOCK))
	{
	    i4 lock_mode = LK_X;
	    i4 lock_action = 0;
	    i4 action = 0;
	    i4 new_lock = FALSE;

	    /* Turn off the bit so that dm1r_lock_row escalates to LK_X */
	    r->rcb_state &= ~RCB_UPD_LOCK;

	    /* Otherwise we will be requesting physical locks in X mode */
	    if (r->rcb_iso_level == RCB_REPEATABLE_READ)
		action = DM0P_LK_CONVERT;

	    if (row_locking(r) && (t->tcb_rel.relstat & TCB_INDEX) == 0)
	    {
		DB_STATUS    tmp_status;

		LK_LOCK_KEY	 save_lock_key;

		if (DMZ_SES_MACRO(33))
		{
		    /* Make sure we already have the lock */
		    tmp_status = dm1r_check_lock(r, &r->rcb_currenttid);
		    if (tmp_status != E_DB_OK)
		    {
			TRdisplay("dm2r_fetch_record: No row lock for %~t\n",
			    sizeof(t->tcb_rel.relid),  &t->tcb_rel.relid);
			SETDBERR(dberr, 0, E_DM9C2A_NOROWLOCK);
			return (E_DB_ERROR);
		    }
		}

		/*
		** Convert to X from LK_U mode
		*/
		status = dm1r_lock_row(r, (action | DM1R_CLEAR_LOCKED_TID), 
			    &r->rcb_currenttid, &new_lock, 
			    &save_lock_key, &local_dberr);
		if (status != E_DB_OK)
		{
		     *dberr = local_dberr;
		     uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &local_err, 0);
		     return (E_DB_ERROR);
		}
		lock_mode = LK_IX;
	    }
	    status = dm0p_trans_page_lock(r, t, 
		    r->rcb_currenttid.tid_tid.tid_page, action,
		    lock_mode, &lock_action, &r->rcb_data_lkid, dberr);
	    /* Note conversion from LK_U to LK_X can fail, check status */
	    if (status != E_DB_OK)
		return (E_DB_ERROR);
	    if (r->rcb_other.page != NULL && (flag & DM2R_IN_PLACE) == 0)
	    {
		status =  dm0p_trans_page_lock(r, r->rcb_tcb_ptr, 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		    r->rcb_other.page), DM0P_LK_CONVERT, lock_mode, 
		    &lock_action, &r->rcb_leaf_lkid, dberr);
		/* Note conversion from LK_U to LK_X can fail, check status */
		if (status != E_DB_OK)
		    return (E_DB_ERROR);
	    }
	    if (r->rcb_state & RCB_CSRR_LOCK)
		r->rcb_state &= ~RCB_CSRR_LOCK;
	}
    }
    return (status);
}


/*{
** Name: dm2r_init_projection -  Initialize RCB for DMF projection
**
** Description:
**      Initialize RCB for DMF projection.
**
**	A data area is allocated to hold a copy of the TCB data attributes,
**	plus anything else needed (i.e. a rowaccessor structure, att pointer
**	array, compression-control array).
**
**	The projection user should build a bit-map of the columns to
**	project (keep) from the data row, and then call dm2r-init-proj-cols
**	to finish the projection setup.
**
**	The projection data area will be deallocated when the RCB is closed.
**
** Inputs:
**      rcb				The Record access Control Block.
**
** Outputs:
**      err_code                        The reason for error status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**      None.
**
** History:
**      24-apr-2003 (stial01)
**          Created for b110061.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dm2r_init_projection(
DMP_RCB		*r,
DB_ERROR	*dberr)
{
    DMP_TCB		*t = r->rcb_tcb_ptr;
    i4			ctl_size;
    i4			j, k;
    DB_STATUS		status = E_DB_OK;
    DMP_MISC		*proj_misc = NULL;
    DB_ATTS		*proj_atts_ptr;
    DB_ATTS		**proj_data_atts;
    DMP_ROWACCESS	*rac;

    CLRDBERR(dberr);

    /*
    ** Projection can only be done on base tables
    */
    if (t->tcb_rel.relstat & TCB_INDEX)
	return (E_DB_ERROR);

    /* We'll be fooling with relversions, tell sizing routine */
    ctl_size = dm1c_cmpcontrol_size(t->tcb_data_rac.compression_type,
			t->tcb_rel.relatts, 1);
    ctl_size = DB_ALIGN_MACRO(ctl_size);
    if (!r->rcb_proj_misc)
    {
	/* Get LongTerm memory */
	status = dm0m_allocate(sizeof(DMP_MISC) +
		    sizeof(DMP_ROWACCESS) +
		    ctl_size +
		    ((t->tcb_rel.relatts + 1) * sizeof(DB_ATTS)) + 
		    (t->tcb_rel.relatts * sizeof(DB_ATTS *)),
		    DM0M_LONGTERM, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		    (char *)NULL, (DM_OBJECT **)&r->rcb_proj_misc, dberr);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    proj_misc = r->rcb_proj_misc;
    rac = (DMP_ROWACCESS *) ((char *)proj_misc + sizeof(DMP_MISC));
    proj_misc->misc_data = (char*) rac;
    if (ctl_size > 0)
    {
	rac->cmp_control = (PTR) rac + sizeof(DMP_ROWACCESS);
	proj_atts_ptr = (DB_ATTS *)((char *) rac->cmp_control + ctl_size);
    }
    else
	proj_atts_ptr = (DB_ATTS *)((char *)rac + sizeof(DMP_ROWACCESS));
    proj_data_atts = (DB_ATTS **)((char *)proj_atts_ptr + 
		    ((t->tcb_rel.relatts + 1) * sizeof(DB_ATTS)));

    MEcopy(t->tcb_atts_ptr, (t->tcb_rel.relatts + 1) * sizeof(DB_ATTS),
		proj_atts_ptr);

    for (j = 1; j <= t->tcb_rel.relatts; j++)
	proj_data_atts[j - 1] = &(proj_atts_ptr[j]);

    r->rcb_proj_atts_ptr = proj_atts_ptr;
    rac->att_ptrs = proj_data_atts;
    rac->att_count = t->tcb_rel.relatts;
    rac->compression_type = t->tcb_data_rac.compression_type;
    rac->control_count = ctl_size;
    r->rcb_data_rac = rac;
    /* Can't setup the rowaccess until we have all of the projected
    ** attributes figured out, since projection will fiddle with drop
    ** versions.
    */
    r->rcb_proj_relwid = t->tcb_rel.relwid;
    r->rcb_proj_relversion = t->tcb_rel.relversion;

    return (E_DB_OK);
}


/*{
** Name: dm2r_init_proj_cols - Init RCB to do DMF projection of columns
**
** Description:
**      Init RCB to do projection of columns in proj_col_map.  This is
**	called after the projection user has decided which columns are
**	needed.  (For parallel indexing, this is after processing all of
**	the index CB's involved.)
**
**	The way we do projection is by fiddling the copy of the data-
**	attributes list that init-projection made.  The attributes to
**	be dropped by the projection have their drop-versions advanced,
**	so that it looks like all rows have these obsolete columns in them
**	to be removed.  The attributes to keep have their offsets adjusted
**	appropriatedly.  (Obviously the projection user has to refer to
**	the projection atts if it's going to reference the data row,
**	NOT the standard TCB atts.)
**
** Inputs:
**      rcb				The Record access Control Block.
**      proj_col_map			bit map of cols to be projected
**
** Outputs:
**      err_code                        The reason for error status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**      None.
**
** History:
**      24-apr-2003 (stial01)
**          Created for b110061.
*/
DB_STATUS
dm2r_init_proj_cols(
DMP_RCB		*rcb, 
i4		proj_col_map[],
DB_ERROR	*dberr)
{
    DMP_RCB		*r = rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    i4			j, k;
    DB_STATUS		status = E_DB_OK;
    DMP_MISC		*proj_misc = NULL;
    DB_ATTS		*proj_atts_ptr;

    CLRDBERR(dberr);

    if (!r->rcb_proj_misc)
	return (E_DB_ERROR);

    for (j = 1; j <= t->tcb_rel.relatts; j++)
    {
	if (proj_col_map[j >> 5] & (1 << (j & 0x1f)))
	{
#ifdef xDEBUG
	    TRdisplay("TABLE %~t PROJECT %~t %d len %d\n", 
	    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
	    sizeof(DB_ATT_NAME), r->rcb_proj_atts_ptr[j].name, j,
	    r->rcb_proj_atts_ptr[j].length);
#endif
	}
	else if (r->rcb_proj_atts_ptr[j].ver_dropped == 0)
	{
	    r->rcb_proj_relversion++;
	    r->rcb_proj_atts_ptr[j].ver_dropped = r->rcb_proj_relversion;
	    r->rcb_proj_relwid -= r->rcb_proj_atts_ptr[j].length;
	    for (k = j + 1; k <= t->tcb_rel.relatts; k++)
	    {
		r->rcb_proj_atts_ptr[k].ordinal_id--;
		r->rcb_proj_atts_ptr[k].offset -= 
					r->rcb_proj_atts_ptr[j].length;
	    }
	}
    }

    status = dm1c_rowaccess_setup(r->rcb_data_rac);
    return (status);
}


/*{
** Name: SIcomplete - Wait for SI threads to complete.
**
** Description:
**	When SI threads are used to update indexes
**	the "main" Base RCB thread must wait until all
**	threads have completed before returning, unless
**	it too is being asynchronously threaded, in which
**	case dmrThread will handle the synchronization
**	of all the threads.
**
**	Sets "Base" function's status in Base RCB
**	unless there's already been an error.
**
** Inputs:
**      rcb				The Base RCB.
**      status				The Base RCB's local status.
**	err_code			THe Base RCB's local error.
**
** Outputs:
**      status                          The collective status of
**					all threads.
**      err_code                        ...and its error code.
**
**	Returns:
**	    DB_STATUS			Same as "status", above.
**
**	Exceptions:
**	    none
**
** Side Effects:
**      None.
**
** History:
**	07-Nov-2003 (jenjo02)
**	    Created for Partitioned Table project
*/
static DB_STATUS
SIcomplete(
DMP_RCB		*r, 
DB_STATUS	status,
DB_ERROR	*dberr)
{
    DMR_RTB	*rtb;

    /*
    ** Appropos only when SI updates are threaded and
    ** the Base RCB's update is synchronous.
    */
    if ( r->rcb_siAgents && !r->rcb_dmrAgent )
    {
	rtb = (DMR_RTB*)r->rcb_rtb_ptr;

	CSp_semaphore(1, &rtb->rtb_cond_sem);

	/*
	** If base dm2r_? failed, notify
	** any SI agents of the failure.
	*/
	if ( status && rtb->rtb_status == E_DB_OK )
	{
	    rtb->rtb_status = status;
	    rtb->rtb_error = *dberr;

	    if ( rtb->rtb_cond_waiters )
		CScnd_broadcast(&rtb->rtb_cond);
	}

	/*
	** Wait until all the SI agents have completed
	** their work.
	*/
	while ( rtb->rtb_active )
	{
	    rtb->rtb_cond_waiters++;
	    CScnd_wait(&rtb->rtb_cond, &rtb->rtb_cond_sem);
	    rtb->rtb_cond_waiters--;
	}

	/* Collective "status" from all SI operations */
	if ( rtb->rtb_status )
	{
	    status = rtb->rtb_status;
	    *dberr = rtb->rtb_error;
	}

	CSv_semaphore(&rtb->rtb_cond_sem);
    }
    return(status);
}
