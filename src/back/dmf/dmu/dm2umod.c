/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
**
NO_OPTIM=dr6_us5 i64_aix
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dm0s.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dmse.h>
#include    <dm1u.h>
#include    <dm2d.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dma.h>
#include    <cm.h>
#include    <cui.h>
#include    <dm0pbmcb.h>
#include    <jf.h>
#include    <dm0j.h>
#include    <cx.h>
#include    <dmckp.h>
#include    <dmve.h>
#include    <dmfjsp.h>
#include    <dmfrfp.h>

/**
**
**  Name: DM2UMODIFY.C - Modify table utility operation.
**
**  Description:
**      This file contains routines that perform the modify table
**	functionality.
**	    dm2u_modify	- Modify a permanent table.
**
**  History:
**	20-may-1989 (Derek)
**	    Split out from DM2U.C
**      22-jun-89 (jennifer)
**          Added checks for security (DAC) to insure that only a user
**          with security privilege is allowed to create or delete 
**          a secure table.  A secure table is indicated by having 
**          relstat include TCB_SECURE.  Also changed the default 
**          definitions for iiuser, iilocations, iiextend, iidbaccess,
**          iiusergroup, iirole and iidbpriv to include TCB_SECURE.
**          The only secure tables are in the iidbdb and include those
**          listed above plus the iirelation of the iidbdb (so that 
**          no-one can turn off the TCB_SECURE state of the other ones).
**      08-aug-89 (teg)
**          Add support for Table Check and Table Purge Operations.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of Modify operation.
**	27-sep-89 (jrb)
**	    Initialized cmp_precision field properly.
**	06-nov-89 (fred)
**	    Added checks for usage of non-keyable and/or non-sortable attributes
**	    as keys.
**	22-feb-1990 (fred)
**	    Added peripheral datatype support.
**	23-feb-90 (greg)
**	    dm2u_sysmod () -- SYSMOD was AV'ing when this file was built
**			      with optimization on VMS.  Added intermediate
**			      variable attrl_ptr.  (Sandy and Nancy identified
**			      the problem).
**	17-may-90 (bryanp)
**	    After we make a significant change to the config file (increment
**	    the table ID or modify a core system catalog), make an up-to-date
**	    backup copy of the config file for use in disaster recovery.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	14-jun-1991 (Derek)
**	    Added support for new allocation and extend options along with the
**	    new inferface to the build routines.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-dec-91 (rogerk)
**		Added support for Set Nologging.
**	    25-mar-1991 (bryanp)
**		Add support for Btree Index Compression.
**	    16-jul-1991 (bryanp)
**		B38527: new reltups arg to dm2u_index, dm0l_modify, dm0l_index.
**		Fix journalling of system catalog updates in dm2u_sysmod.
**	    30-aug-1991 (rogerk)
**		Part of fixes for rollforward of reorganize problems.  Pass 
**		DM0L_REORG flag in modify log record for modify to reorganize.
**		Changed dm2u_reorg_table to not depend on the existence of 
**		xcb information during rollforward.
**		Fixed error handling bug in dm2u_reorg_table.
**	30-Oct-1991 (rmuth)
**	    dm2u_modify was not consistent in how it passed allocation and
**	    extend to other routines. Moved the validating/setting of these
**	    values to earlier in the routine.
**	4-dec-1991 (bryanp)
**	    B41119: verifydb on a view crashes the server.
**	06-feb-1992 (ananth)
**	    Use the ADF_CB in the RCB instead of allocating in on the stack 
**	    in dm2u_modify().
**      10-Feb-1992 (rmuth)
**          Correct dm2u_reorg_table() so that it calls dm2f_file_size()
**          to work out the allocation to log in the fcreate log record
**	19-feb-1992 (bryanp)
**	    Temporary table enhancements.
**	15-apr-1992 (bryanp)
**	    Remove the FCB argument to dm2f_rename().
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**      19-may-92 (schang)
**          GW merge.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	16-jul-1992 (kwatts)
**	    MPF change. Use accessor to calculate possible expansion on
**	    compression (comp_katts_count).
**	29-August-1992 (rmuth)
**	    update_temp_table_tcb was not updating the relfhdr field.
**	16-sep-1992 (bryanp)
**	    Set relpgtype and relcomptype in update_temp_table_tcb().
**	18-Sept-1992 (rmuth)
**	    Set relpgtype to TCB_PG_DEFAULT.
**	22-sep-1992 (robf)
**	     disallow verifydb operations on gateway tables
**	24-sep-1992 (rickh)
**	    Added new relstat2 argument to dm2u_create for FIPS CONSTRAINTS.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	06-oct-1992 (robf)
**	     Improve error on verifydb w/gateways since the E_DM009F
**	     is translated into a very generic "internal error" message
**           to the user.
**	16-0ct-1992 (rickh)
**	    Initialize default ids for attributes.
**	22-oct-1992 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    all references to DMP_CMP_LIST with DB_CMP_LIST.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	    Updated update_temp_table_tcb to reflect new dm2t protocols.
**	30-October-1992 (rmuth)
**	    - Remove all references to the DI_ZERO_ALLOC file open flag.
**	    - When checking for FCB_OPEN use the & instead of == because
**	      we may not flush when allocating.
**	02-nov-1992 (jnash)
**	    Reduced Logging Project (phase 2): 
**	    Fill in loc_config_id when building mxcb, reflecting new dm2t 
**	      protocols.  Add new params to dm0l calls.
**	    Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	06-dec-1992 (jnash)
**	    Swap order of gw_id and char_array params in dm2u_index call
**	    to quiet compiler.
**	14-dec-1992 (rogerk)
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location since it's no longer used.
**	18-nov-92 (robf)
**          Remove dm0a.h
**	18-jan-1993 (bryanp)
**	    Don't just assert TCB_BUSY, use proper TCB exclusion protocols.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.
**	25-feb-93 (rickh)
**	    Add relstat2 argument to dm2u_index call.
**	30-feb-1993 (rmuth)
**	    When validating multiple locations if the last location in
**	    the config file is a location specfied and it is not a data
**	    location we would not generate an error. Now fixed.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**	16-mar-1993 (rmuth)
**	    Include di.h
**	30-mar-1993 (rmuth)
**	    Add mod_options2 and the add_extend functionality code.
**	7-apr-93 (rickh)
**	    Added relstat2 argument to dm2u_modify.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	01-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use appropriate case for iirtemp and iiridxtemp in dm2u_sysmod
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**      21-June-1993 (rmuth)
**	    File Extend: Make sure that for multi-location tables that
**	    allocation and extend size are factors of DI_EXTENT_SIZE.
**	21-jun-1993 (jnash)
**	    Force table pages prior to writing modify log records.  This 
**	    is required because the log record signals a "non-redo"
**	    log address for recovery.  Also fix a few compiler warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	27-jul-93 (rickh)
**	    Certain relstat2 bits must persist over modifies:
**	    SYSTEM_GENERATED, SUPPORTS_CONSTRAINT, NOT_USER_DROPPABLE.
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the modify process for new recovery protocols.
**	    Divorced the file creation from the table build operation and
**	    moved both of these to before the logging of dm0l_modify.
**	  - Removed file creation from the dm2u_reorg_table routine.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.
**	  - Added a statement count field to the modify and index log records.
**	  - Fix CL prototype mismatches.
**	16-sep-93 (rickh)
**	    The key_order field in the key descriptors for SCONCUR catalogs
**	    were being stuffed with the flags field from the corresponding
**	    attribute tuples.  Odd.  Changed this to force the SCONCUR keys
**	    to always have ascending order.
**	20-sep-1993 (rogerk)
**	  - Take out references to the xcb dmu count in dm2u_reorg_table.
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	18-oct-1993 (rogerk)
**	    Moved call to dm2u_calc_hash_buckets up from dm2u_load_table so
**	    that the bucket number could be logged in the dm0l_modify log
**	    record.  This allows rollforward to run deterministically.
**	31-jan-94 (jrb)
**	    Fix bug 59242. In dm2u_modify pass location array to
**	    create_build_table so that it will use the correct list of
**	    locations instead of using $default which is incorrect if the table
**	    gets flushed to disk and is later renamed to a different location.
**	05-nov-93 (swm)
**	    Bug #58633
**	    Alignment offset calculations used a hard-wired value of 3
**	    to force alignment to 4 byte boundaries, for both pointer and
**	    i4 alignment. Changed these calculations to use
**	    ME_ALIGN_MACRO.
**	3-feb-1994 (bryanp)
**	    Temporary fix to modifies of temp tables which explicitly modify
**	    the table to reside on "$default". dm2u_modify treats an explicit
**	    modify to $default as meaning "place the result table onto the
**	    database root location", but for in-memory modifies we were making
**	    the build table reside on the session's work location(s), meaning
**	    that if the in-memory build should overflow, necessitating an
**	    eventual file rename of the build table's file to the destination
**	    table's file, the rename would fail because the two files were in
**	    different locations. This is only a temporary fix, because the
**	    effect of this fix is that temporary tables which are modified
**	    are currently ending up on the root location, when we want
**	    them to be ending up on the work location(s).
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open 
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them 
**	    prior to close.
**	20-jun-1994 (jnash)
**	    fsync project.  Open (verifydb -table -xtable) or
**	    (modify to table_patch) tables sync-at-end.  This action may be
**	    overridden by load_optim server config param.
**	31-aug-1994 (mikem)
**	    bug #64832
**          Creating key column's in any order on global temporary tables
**	    now works.  Previous to this change the code to set up a temp
**	    tcb only worked for key's in column order (ie. first col, second
**	    col, ...).  Changed the key setup code to use m->mx_atts_ptr which
**	    is an ordered list of keys, rather than the list of attributes it
**	    was using before.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**      09-jan-1995 (nanpr01)
**	    Bugs #66002 & 66028
**	    Added SQL support for setting table to physically inconsistent,
**	    logically inconsistent or table recovery disallowed. Preserve
**	    the values already set.
**      27-dec-94 (cohmi01)
**          For RAW I/O, pass location ptr to dm2f_rename(), NULL for sysmod.
**	    Allocate FCBs for old locations, so RAW operations (eg rename)
**	    may be done. Add dm2u_raw_extend() for adjusting logical file
**	    size to use entire extent of raw location(s) occupied by table.
**	05-may-1995 (shero03)
**	    For RTREE added dimension, range, and range_granularity on
**	    dm2u_create call
**	 5-jul-95 (dougb) bug # 69382
**	    To avoid consistency point failures (E_DM9397_CP_INCOMPLETE_ERROR)
**	    which can occur if the CP thread runs during or just after
**	    update_temp_table_tcb(), leave it to dm2t_temp_build_tcb() to
**	    insert new TCB into the HCB queue.
**	 4-mar-1996 (nick)
**	    Worst case key length was being calculated incorrectly. #74022
**	06-mar-1996 (stial01 for bryanp)
**	    Support specification of page size during modify.
**	    In dm2u_reorg_table, use tcb_rel.relpgsize to compute page size.
**	    Dimension attribute arrays using DMP_NUM_IIRELATION_ATTS, not "40"
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**          Pass page size to dm2f_write_file()
**          create_build_table() pass page_size characteristic
**          Pass page size to dm2f_build_fcb(), dm2f_read_file()
**      06-may-1996 (thaju02 & nanpr01)
**          New Page Format Project:
**              Pass page size to dm1c_getaccessors().
**	    Also modified dm2u_maxreclen call. We never intended to support
**	    larger keys for this project. So we used the DM_COMPAT_PGSIZE
**	    for key length checking.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	25-oct-1996 (hanch04)
**	    Hard coded ntab_width for II_ATTRIBUTE to 96
**	31-oct-1996 (hanch04)
**	    Replace Hard coded ntab_width for sizeof (DMP_ATTRIBUTE)
**	    
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduction of Distributed Multi-Cache Management (DMCM) protocol.
**          dm2u_modify must cause all other caches to write out dirty pages
**          for the target table before proceeding.
**          Call to dm2t_xlock_tcb, will trigger callbacks on TCBs for this
**          table in other caches, causing all dirty pages for the table to
**          be written out and hence a consistent image is generated prior to
**          the modify.
**      27-jan-1997 (stial01)
**          Do not include TIDP in the key of unique secondary indexes
**      31-jan-1997 (stial01)
**          Commented code for 27-jan-1997 change
**      10-mar-1997 (stial01)
**          Alloc/init mx_crecord, area to be used for compression.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    New parm, tbl_pri, added to dm2u_modify() prototype, in which
**	    table priority is passed.
**	12-apr-1997 (shero03)
**	    Remove dimension and hilbertsize from dm2u_index parms
**      21-may-1997 (stial01)
**          dm2u_modify() Always include tidp in "key length" for BTREE.
**          For compatibility reasons, DO include TIDP for V1-2k unique indices 
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_close_page(), dm2t_unfix_tcb(),
**	    dm2t_destroy_temp_tcb() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	02-Oct-1997 (shero03)
**	    Changed the code to use the mx_log_id in lieu of the xcb_log_id
**	    because the xcb doesn't exist during recovery.
**      21-May-1997 (stial01)
**          dm2u_sysmod() The journal status for each insert into iirtemp
**          depends on the journal status of the underlying user table (b65170)
**      22-May-1997 (stial01)
**          dm2u_sysmod() Invalidate tbio_lpageno, tbio_lalloc after core
**          catalog file renames (B78889, B52131, B57674)
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**          Improved chances of SYSMOD recovery if SYSMOD is interrupted.
**	11-Aug-1998 (jenjo02)
**	    When counting tuples, include counts from RCB(s) as well.
**	22-aug-1998 (somsa01)
**	    In update_temp_table_tcb(), we need to carry over the tcb_et_list
**	    (in the case of blobs) and the logical key from the old TCB.
**	28-oct-1998 (somsa01)
**	    In the case of Global Temoporary Extension Tables, the lock will
**	    live on the session lock list, not the transaction lock list.
**	    (Bug #94059)
**	30-sep-1998 (marro11)
**	    Backing out 26-may-1998 fix for bug 65170; it introduces an 
**	    undesirable side effect:  bug 93404.  Also backing out 17-aug-98
**	    change, as this was stacked on aforementioned change.
**	03-Dec-1998 (jenjo02)
**	    Pass XCB*, page size, extent start to dm2f_alloc_rawextent().
**	23-Dec-1998 (jenjo02)
**	    TCB->RCB list must be mutexed while it being read.
**	23-dec-1998 (somsa01)
**	    In dm2u_modify(), we should be passing lk_list_id to
**	    dm2t_control(), not the xcb_lk_id.
**      18-Oct-1999 (hanal04) Bug 94474 INGSRV633
**          Remove the above change as the lock is now back on the
**          transaction lock list.
**      20-May-2000 (hanal04, stial01 & thaju02) Bug 101418 INGSRV 1170
**          Global session temporary table lock is back on the Session lock 
**          list.
**	02-Oct-2000 (shust01)
**	    In dm2u_modify(), Close the temp table if an error occurred.
**	    Normally, the temp table is closed in update_temp_table_tcb(),
**	    but if an error occurred before then that call wouldn't happen,
**	    and could receive E_DM005D and when quit tm, E_DM9C75, E_DM9270,
**	    E_DM9267, E_SC010E.  bug 101017.
**      15-mar-1999 (stial01)
**          dm2u_modify() can get called if DCB_S_ROLLFORWARD, without xcb,scb
**	19-mar-1999 (nanpr01)
**	    Get rid of extent names since only one table is supported per
**	    raw location.
**	16-Apr-1999 (jenjo02)
**	    Additional changes for RAW locations.
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**	23-Jun-1999 (jenjo02)
**	    More RAW cleanup.
**      28-sep-99 (stial01)
**          Reset tbio_cache_ix whenever tbio_page_size is re-initialized.
**      12-nov-1999 (stial01)
**          Do not add tidp to key for ANY unique secondary indexes
**      21-dec-1999 (stial01)
**          Use TCB_PG_V3 page type for etabs having page size > 2048
**      12-jan-2000 (stial01)
**          Set TCB2_BSWAP in relstat2 for tables having etabs (Sir 99987)
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      15-jun-2000 (stial01 for jenjo02)
**          dm2u_sysmod changing page_size, update hcb_cache_tcbcount's B101850
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-sep-2000 (stial01)
**          Disable vps etab.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**          dm2u_modify() Fixed page_size argument to dm0l_modify
**      01-nov-2000 (stial01)
**          dm2u_modify, check available pgsize <= 65536 (B103081,Star 10406369)
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**	14-Nov-2001 (thaju02)
**	    In update_temp_table_tcb(), in concluding an in-memory build,
**	    we destroy the temporary build table, but neglect to release the
**	    table lock on the temporary build table. Release table lock.
**	    (B106380)
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	    Carry tcb_temp_xccb from old to new TCB in 
**	    update_temp_table_tcb().
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_table_access() if neither
**	    C2 nor B1SECURE.
**      01-feb-2001 (stial01)
**	    Cannot modify temp to different page size OR different page type
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	23-Apr-2001 (jenjo02)
**	    Deleted dm2u_raw_totalblks() function. Fix leaks of
**	    FCBs from "oloc_array".
**	20-may-2001 (somsa01)
**	    Changed 'longnat' to 'i4' from last cross-integration.
**      11-jul-2001 (stial01)
**          dm2u_modify() Fixed initialization of pgsize (B105229)
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	13-Dec-2002 (jenjo02)
**	    Maintain hcb_cache_tcbcount for Index TCBs as well as
**	    for base TCBs (BUG 107628).
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
**       5-Nov-2003 (hanal04) Bug 110920 INGSRV2525
**          Prevent spurious errors when modifying a table from HIDATA
**          to DATA compression.
**      02-jan-2004 (stial01)
**          Do ADD_EXTEND only if extend != 0.
**	15-Jan-2004 (schka24)
**	    temp-build-tcb figures out hash stuff itself now.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	28-Feb-2004 (jenjo02)
**	    Massive changes for partitioned modifies, multiple
**	    input and/or output partitions.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	2-Apr-2004 (schka24)
**	    Proper logging of repartitioning modify stuff for journals.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          mod_key now DM_ATT array not char.
**      14-apr-2004 (stial01)
**          Increase btree key size, varies by pagetype, pagesize (SIR 108315)
**          Return error info in err_data, btree max key len in tup_info
**	15-apr-2004 (somsa01)
**	    Ensure proper alignment of tppArray to avoid 64-bit BUS errors.
**	30-apr-2004 (thaju02)
**	    In rebuild_persistent(), if DMU_PIND_CHAINED then persistent index DMU_CBs
**	    are linked to modify DMU_CB.
**      11-may-2004 (stial01)
**          Removed unnecessary svcb_maxbtreekey
**	11-may-2004 (thaju02)
**	    Online modify and concurrent updates which are rolled back.
**	    Quiesce to ensure that CLR and the compensated log record are both in
**	    the sidefile. Added find_compensated() to locate compensated log rec
**	    allowing sidefile to reconstruct image for positioning.
**	12-may-2004 (thaju02)
**	    Modified do_sidefile - initialize redo/redo2. 
**	    Change E_DM9666 to the more specific E_DM9CB1.
**	19-may-2004 (thaju02)
**	    Modified check_archiver - initialized purge log address to
**	    database's last journal record log address.
**	04-jun-2004 (thaju02)
**	    Added prototypes for do_online_dupcheck, find_compensated.
**	    For rollforwarddb, allocate btree read map. Added 
**	    init_btree_read(). (B112428)
**	16-jul-2004 (thaju02)
**	    Delete may result in the freeing/deallocation of a 
**	    disassociated data page which was not read by sorter. 
**	    position_to_row() fails with E_DM0055. Modified do_sidefile() - 
**	    If position_to_row() returns E_DM0055, check that we have a 
**	    DM0LDEALLOC log record for this page. At completion of
**	    sidefile processing, deallocated page queue (freedpgq) 
**	    should be empty. If not, report E_DM0055_NONEXT. (B112658).
**	    Modified position_to_row() - Once we have retrieved a record,
**	    if table is not unique, check to ensure that the entire record
**	    retrieved matches the image in the log record. 
**	20-jul-2004 (thaju02)
**	    Modified position_to_row() - use adt_tupcmp() to compare 
**	    tuples. 
**	21-jul-2004 (thaju02)
**	    Sidefile processing to update tuple count (tup_info). (B112720)
**	23-jul-2004 (thaju02)
**	    Online modify may result in E_DMA48E_LOGFULL followed by the
**	    dbms server terminating. In do_sidefile(), check for interrupts. 
**	    If user interrupt or force abort, pass the error back up to
**	    qef. (B112736)
**	28-jul-2004 (thaju02)
**	    Online modify sidefile processing may corrupt memory pool; dbms
**	    shutdown loops. In do_sidefile() to uncompress, changed 
**	    dest max length to relwid, rather than pagesize. (B112752)
**	    Rolldb of online modify may fail due to an exception in
**	    dm0m_deallocate(). rnl_dm1p_max must be initialized to the
**	    max pages for pg type/size for rollforward. (B112753)
**	    Online modify of table in which column had previously been
**	    dropped results in table corruption. In sidefile processing
**	    use source table's tcb_att_ptr to uncompress. (B112758)
**	    Help table's # of rows is incorrect - set tp->tpcb_newtup_cnt 
**	    to *tup_info after do_online_modify().
**	19-aug-2004 (thaju02)
**	    In position_to_row(), if no key then DM2R_ALL position. (B112859)
**	    Online modify of btree may fail due to E_DM0055. Last record
**	    on disassoc data page is deleted. Page is not reclaimed, unable
**	    to get X page lock. Page not put on free list, no dealloc log
**	    record. Page not read by dm1b_online_get(). In do_sidefile,
**	    if DM0055 and no corresponding dealloc log record found,
**	    fix page and check if it is an empty disassoc data page. (B112792)
**	16-Aug-2004 (schka24)
**	    Some mct/mx usage cleanups.
**      13-sep-2004 (stial01)
**          Temporarily disable partitioned V5 tables.
**      04-oct-2004 (stial01)
**          Enable partitioned V5 tables.
**	02-dec-2004 (thaju02)
**	    In do_sidefile(), dmd_log only if trace point set.
**	15-feb-2005 (thaju02)
**	    Online modify repartition where new parts > old parts.
**	08-Mar-2005 (thaju02)
**	    Cleanup - SIGSEGV on Linux if TRdisplay tabname rather
**	    than tabname.db_tab_name .
**       02-mar-2005 (stial01)
**          New error for page type V5 WITH NODUPLICATES (b113993)
**	11-Mar-2005 (thaju02)
**	    Cleanup in do_online_modify. Created get_online_rel().
**	    Use $online index relpages, relprim, relmain for index.
**	    Use parent table's tuple count for index reltups. (B114069)
**	04-Apr-2005 (jenjo02)
**	    To assure reproducible TIDs with multiple sources and
**	    targets, add the source's TID8 to the bunch of stuff
**	    to be included in the sort list.
**	    In do_sidefile, find_part, use TID8 hash method to
**	    assign AUTOMATIC partitions.
**	    Lift restriction banning online modify with 
**	    AUTOMATIC partition.
**	08-Apr-2005 (thaju02)
**	    Modified dm2u_modify(), in building key map, check that 
**	    attribute had not been previously dropped. (B105907)
**       02-mar-2005 (stial01)
**          New error for page type V5 WITH NODUPLICATES (b113993)
**	10-May-2005 (raodi01) bug110709 INGSRV2470
**	    Added code changes to handle modifying of a temporary table
**	    with key compression.
**	25-Oct-2005 (hanje04)
**	    Add prototype for check_archiver() to prevent compiler
**	    errors with GCC 4.0 due to conflict with implicit declaration.
**	13-Apr-2006 (jenjo02)
**	    Squished outrageous parm list into (new) DMU_MCB structure,
**	    added stuff for CLUSTERED tables.
**	    Remove DM1B_MAXKEY_MACRO throttle from leaf entry size check.
**     20-oct-2006 (stial01)
**         Disable clustered index change to max_leaflen in ingres2006r2.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      12-apr-2007 (stial01)
**          Moved MAX_RAWDATA_SIZE define to dm0l.h
**      21-dec-2007 (stial01)
**          do_online_modify() Fixed LKrequest error handling (b119665)
**	25-Feb-2008 (kschendel) SIR 122739
**	    Changes throughout for new rowaccess structure.
**	    Break out catalog-only modifies.  Pass a real relstat to
**	    dm2u-create, not some nonsensical flag mish-mash.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	18-Nov-2008 (jonj)
**	    SIR 120874: dm0j_?, dm0c_?, dm2d_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_?, dm1u_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      16-Jun-2009 (hanal04) Bug 122117
**          Treat E_DM016B_LOCK_INTR_FA the same as a user interrupt.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC: Blob support. Consistently use tcb_extended
**	    instead of relstat2 & TCB2_EXTENSION.
**      01-apr-2010 (stial01)
**          Changes for Long IDs, fix error handling
**      10-Feb-2010 (maspa05) bug 122651, trac 442
**          Disallow modify of TCB2_PHYSLOCK_CONCUR tables (e.g. iisequence) to
**          anything other than their default structure (HASH). 
**          Since we use physical (i.e. transient) locks it can cause problems 
**          if the row moves or we have a btree split etc because session A may 
**          insert or update a row and then release the lock meanwhile session 
**          B may cause the row to move making session A's log record invalid. 
**          If session A then needs to rollback we have a problem. So treat 
**          TCB2_PHYSLOCK_CONCUR the same as S_CONCUR and throw
**          E_DM005A if a modify to anything other than HASH is attempted
**          This gets reported as E_US159D to the frontend.
**	01-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Open tables "no coupon" to avoid BQCB, coupon processing stuff.
**      29-Apr-2010 (stial01)
**          dm2u_sysmod iirelation, iirel_idx, iiattribute with compression
**      10-may-2010 (stephenb)
**          Cast new i8 reltups to i4.
**      27-May-2010 (stial01)
**          Set DM1C_CORE_CATALOG for iidevices,iisequence which use phys locks
**	20-Jul-2010 (kschendel) SIR 124104
**	    dm2u-create wants compression now.
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*
** Forward declarations of static functions:
*/
static DB_STATUS update_temp_table_tcb(DM2U_MXCB           *mxcb,
					DB_ERROR           *dberr);
static DB_STATUS create_build_table(
			    DM2U_MXCB	    *mxcb,
			    DMP_DCB	    *dcb,
			    DML_XCB	    *xcb,
			    DB_TAB_NAME	    *table_name,
			    DB_LOC_NAME	    *loc_list,
			    i4	    loc_count,
			    DB_ERROR	*dberr);

static DB_STATUS AddOldTargetLocation(
			DM2U_MXCB	*m,
			DMP_LOCATION	*loc,
			DMP_LOCATION	*nloc,
			DB_ERROR	*dberr);
static DB_STATUS AddNewLocation(
			DM2U_MXCB	*m,
			DB_LOC_NAME	*loc_name,
			DMP_LOCATION	*loc,
			i4		loc_id,
			DB_ERROR	*dberr);

static DB_STATUS logModifyData(
			i4		log_id,
			i4		flags,
			DB_PART_DEF	*part_def,
			i4		def_size,
			DMU_PHYPART_CHAR *ppchar_addr,
			i4		ppchar_entries,
			DB_ERROR	*dberr);

static DB_STATUS logPartDef(
			i4		log_id,
			i4		flags,
			DB_PART_DEF	*part_def,
			i4		def_size,
			DB_ERROR	*dberr);

static DB_STATUS logPPchar(
			i4		log_id,
			i4		flags,
			DMU_PHYPART_CHAR *ppchar_addr,
			i4		ppchar_entries,
			DB_ERROR	*dberr);

static DB_STATUS dm2u_catalog_modify(
			DM2U_MOD_CB	*mcb,
			DMP_RCB		*r,
			DB_ERROR	*dberr);

static DB_STATUS
rebuild_persistent(
			DMP_DCB		    *dcb,
			DML_XCB		    *xcb,
			DB_TAB_ID	    *table_id,
			DMU_CB		    *dmu,
			bool		    online_modify,
			DB_TAB_ID	    *online_tab_id);

static DB_STATUS do_online_modify(
			DM2U_MXCB	    **mxcb,
			i4		    timeout,
			DMU_CB		    *dmu,
			i4		    modoptions,
			i4		    mod_options2,
			i4		    kcount,
			DM2U_KEY_ENTRY	    **key,
			i4		    db_lockmode,
			DB_TAB_ID	    *modtemp_tabid,
			i4		    *tup_info,
			i4		    *has_extensions,
			i4		    relstat2,
			i4		    flagsmask,
			DM2U_RFP_ENTRY  *rfp_entry,
			DMP_RELATION	*online_reltup,
			DB_PART_DEF	    *new_part_def,
			i4		    new_partdef_size,
			DMU_PHYPART_CHAR    *partitions,
			i4		    nparts,
			i4		    verify,
			DB_ERROR	    *dberr);

static DB_STATUS do_online_dupcheck(
			DMP_DCB         *dcb,
			DML_XCB         *xcb,
			DM2U_MXCB       *mxcb,
			DMP_RCB         *online_rcb,
			DB_TAB_ID       *dupchk_tabid,
			i4              db_lockmode,
			DB_ERROR        *dberr);

static DB_STATUS find_compensated(
			DM2U_MXCB       *mxcb,
			DM0L_HEADER     *clr_rec,
			char            *row_image,
			i4              *ndata_len,
			i4              *nrec_size,
			i4              *diff_offset,
			DB_ERROR        *dberr);

static DB_STATUS find_part(
			DM2U_MXCB       *m,
			DB_TAB_ID       *log_tabid,
			char            *log_row,
			i4              *partno,
			DM_TID		*ptid,
			DB_ERROR        *dberr);

static DB_STATUS do_sidefile(
			DMP_DCB             *dcb,
			DML_XCB             *xcb,
			DB_TAB_NAME         *table_name,
			DB_OWN_NAME         *table_owner,
			DM2U_MXCB           *mxcb,
			DM2U_OMCB           *omcb,
			i4                  db_lockmode,
			i4                  *tup_info,
			DB_ERROR            *dberr);

static DB_STATUS position_to_row(
			DM2U_MXCB           *mxcb,
			DMP_RCB             *position_rcb,
			DMP_RCB             *base_rcb,
			char                *log_rec,
			char                *rowbuf,
			DM2R_KEY_DESC       *key_desc,
			DM_TID              *tid,
			DB_ERROR            *dberr);

static DB_STATUS create_new_parts(
			DM2U_MXCB           *m,
			DB_TAB_NAME         *tabname,
			DB_OWN_NAME         *owner,
			DB_PART_DEF         *dmu_part_def,
			DMU_PHYPART_CHAR    *partitions,
			DB_TAB_ID           *master_tabid,
			i4                  db_lockmode,
			i4                  relstat,
			i4                  relstat2,
			DB_ERROR            *dberr);

static DB_STATUS get_online_rel(
                        DM2U_MXCB           *m,
                        DB_TAB_ID           *modtemp_tabid,
                        DMU_CB              *index_cbs,
                        DMU_PHYPART_CHAR    *o_partitions,
                        i4                  db_lockmode,
			DB_ERROR            *dberr);

static DB_STATUS check_archiver(
				DMP_DCB		    *dcb,
				DML_XCB		    *xcb,
				DB_TAB_NAME	    *table_name,
				LG_LSN		    *lsn,
				DB_ERROR	    *dberr);

static DB_STATUS init_readnolock(
			DM2U_MXCB	*mxcb,
			DM2U_OSRC_CB	*osrc,
			DB_ERROR	*dberr);

static DB_STATUS test_redo(
			DM2U_OMCB	*omcb,
			DM0L_HEADER	*record,
			DB_TAB_ID	*log_tabid,
			DM_PAGENO	log_page,
			LG_LSN		*log_lsn,
			bool		*redo,
			DB_ERROR	*dberr);

/*{
** Name: dm2u_modify - Modify a table's structure.
**
** Description:
**      This routine is called from dmu_modify() to modify a table's structure.
**
** Inputs:
**	dcb				The database id. 
**	xcb				The transaction id.
**	tbl_id				The internal table id of the base table 
**					to be modified.
**	structure			Structure to modify.
**      location                        List of location for modified table.
**      l_count                         Number of locations given.
**	i_fill				Fill factor of the index page.
**	l_fill				Fill factor of the leaf page.
**	d_fill				Fill factor of the data page.
**	compressed			TRUE if compressed records.
**	index_compressed		Specifies whether index is compressed.
**	temporary			non-zero if modifying temp table.
**	min_pages			Minimum number of buckets for HASH.
**	max_pages			Maximum number of buckets for HASH.
**	unique				TRUE if unique key.
**      modoptions                      Zero or DM2U_TRUNCATE or 
**                                      DM2U_[NO]DUPLICATES or DM2U_REORG.
**      mod2_options                    Flasg indicating what to do.
**	kcount				Count of key entry.
**	key				Pointer to key array of pointers
**                                      for modifying.
**	db_lockmode			Lockmode of the database for the caller.
**	allocation			Table space pre-allocation amount.
**	extend				Out of space extend amount.
**	page_size			Page size for new table.
**	relstat2			Bits to set in relstat2.
**	has_extensions			Ptr to i4 to receive indication
**					that a table has extensions.  This will
**					be used by the logical layer to decide
**					whether to call the peripheral modify
**					routine to modify the extension tables
**					as well.
**	flagsmask			0 or DMU_INTERNAL_REQ
**	tbl_pri				0, or table priority if 
**					DM2U_2_TABLE_PRIORITY or 
**					DM2U_2_TO_TABLE_PRIORITY, in the range
**					1-DB_MAX_TABLEPRI.
**	partitions			(optional) ppchar array describing
**					physical partitions. Says which ones
**					to do and where they go.
**	nparts				Number of entries in "partitions";
**					if repartitioning this is the NEW
**					number of partitions (should be ==
**					to new_part_def->nphys_parts)
**	verify				Opcodes and modifiers for VERIFY
**					operation, already in DM1U style;
**					zero if not doing a verify.
**
** Outputs:
**	tup_info			Number of tuples in the modified table
**	*has_extensions			Filled as appropriate.
**      err_code                        The reason for an error return status.
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
**      28-may-86 (ac)
**          Created from dmu_modify().
**	30-aug-88 (sandyh)
**	    Bug 2995 -- make index of mod_key[] start at 1, instead
**		of zero, which will cause RFP to fail.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-feb-89 (EdHsu)
**	    Add call to dm2u_ckp_lock to be mutually exclusive with ckp.
**      08-aug-89 (teg)
**          Add support for Table Check and Table Purge Operations.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Check for illegal modify options
**	    for gateway table.  Allow modify of table that has no underlying
**	    physical file - in this case just the system catalogs are updated.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of Modify operation.
**	06-nov-89 (fred)
**	    Added checks for usage of non-keyable and/or non-sortable attributes
**	    as keys.
**	22-feb-1990 (fred)
**	    Added has_extensions parameter.
**	16-jun-90 (linda)
**	    This routine is now called from dmu_create() for gateway tables;
**	    reltups parameter indicates rowcount specified in register table
**	    command (or default); relpages comes from gateway table open.
**	18-jun-90 (linda, bryanp)
**	    New flag to dm2t_open -- DM2T_A_MODIFY, which for Ingres tables is
**	    treated the same as DM2T_A_WRITE, but for gateway tables allows us
**	    to proceed even if the underlying file does not exist (we go through
**	    dm2u_modify only to set system catalog information).
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**	     7-mar-1991 (bryanp)
**		New argument 'index_compressed' to support Btree index
**		compression.  Set TCB_INDEX_COMP bit in relstat properly.
**		Calculate and check maximum key width for compressed Btree
**		index.  Set up new field mx_data_atts in the DM2U_MXCB.
**	    16-jul-1991 (bryanp)
**		B38527: new reltups arg to dm2u_index, dm0l_modify, dm0l_index.
**	    30-aug-1991 (rogerk)
**		Part of fix for rollforward of reorganize problems.  Pass
**		DM0L_REORG flag in modify log record if this is a modify to
**		reorganize.
**	30-Oct-1991 (rmuth)
**	    The routine was not consistent in how it passed allocation and
**	    extend to other routines. Moved the validating/setting of these
**	    values to earlier in the routine.
**	4-dec-1991 (bryanp)
**	    B41119: verifydb on a view crashes the server. The verifydb table
**	    operations cannot be performed on a view; such a request should be
**	    rejected.
**	06-feb-1992 (ananth)
**	    Use the ADF_CB in the RCB instead of allocating in on the stack.
**	19-feb-1992 (bryanp)
**	    Temporary table enhancements: call update_temp_table_tcb, not
**	    dm2u_update_catalogs, after modifying a temporary table.
**	    Added new 'temporary' argument to dm2u_modify.
**	16-jul-1992 (kwatts)
**	    MPF change. Use accessor to calculate possible expansion on
**	    compression (comp_katts_count).
**	29-August-1992 (rmuth)
**	    update_temp_table_tcb was not updating the relfhdr field.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	22-sep-1992 (robf)
**	     disallow verifydb operations on gateway tables
**	06-oct-1992 (robf)
**	     Improve error on verifydb w/gateways since the E_DM009F
**	     is translated into a very generic "internal error" message
**           to the user.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	02-nov-1992 (jnash)
**	    Reduced Logging Project (phase 2): 
**	    Fill in loc_config_id when building mxcb, reflecting new dm2t 
**	      protocols.  Add new params to dm0l calls.
**	    Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location since it's no longer used.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**	30-mar-1993 (rmuth)
**	    Add mod_options2 and the add_extend functionality code.
**	30-feb-1993 (rmuth)
**	    When validating multiple locations if the last location in
**	    the config file is a location specfied and it is not a data
**	    location we would not generate an error. Now fixed.
**	7-apr-93 (rickh)
**	    Added relstat2 argument to dm2u_modify.
**	30-mar-1993 (rmuth)
**	    Add mod_options2 and the add_extend functionality code.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables.
**	28-may-1993 (robf)
**	    Secure 2.0: Reworked old ORANGE code
**      21-June-1993 (rmuth)
**	    File Extend: Make sure that for multi-location tables that
**	    allocation and extend size are factors of DI_EXTENT_SIZE.
**	21-jun-1993 (jnash)
**	    Force table pages prior to writing modify log records.  
**	27-jul-93 (rickh)
**	    Certain relstat2 bits must persist over modifies:
**	    SYSTEM_GENERATED, SUPPORTS_CONSTRAINT, NOT_USER_DROPPABLE.
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the modify process for new recovery protocols.
**	      - We now log/create the table files before doing any other
**		work on behalf of the modify operation.  The creation of
**		the physical files was taken out of the dm2u_load_table
**		routine which now just allocates and formats space.
**	      - The dm2u_load_table call is now made to build the structured
**		table BEFORE logging the dm0l_modify log record.  This is
**		done so that any non-executable modify attempt (one with an
**		impossible allocation for example) will not be attempted 
**		during rollforward recovery.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.  Changed DMU to
**	    to be a journaled log record.
**	  - Added a statement count field to the modify and index log records.
**	  - Added gateway status to the logged modify relstat since modify
**	    of gateway tables is now journaled.
**	  - Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	7-sep-93 (robf)
**	    Don't do MAC on temporary tables
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	18-oct-1993 (rogerk)
**	  - Moved call to dm2u_calc_hash_buckets up from dm2u_load_table so
**	    that the bucket number could be logged in the dm0l_modify log
**	    record.  This allows rollforward to run deterministically.
**	  - Pass bucket count in dm0l_modify call.
**	  - Since the reltups count is no longer required for recovery
**	    processing, it is now always logged as the number of rows in the
**	    result table instead of just for sconcur tables.  The log record
**	    value is used for informational purposes only.
**	31-jan-94 (jrb)
**	    Pass location array to create_build_table so that it will use
**	    the correct list of locations instead of using $default which
**	    is incorrect if the table gets flushed to disk and is later
**	    renamed to a different location.
**	3-feb-1994 (bryanp)
**	    Temporary fix to modifies of temp tables which explicitly modify
**	    the table to reside on "$default". dm2u_modify treats an explicit
**	    modify to $default as meaning "place the result table onto the
**	    database root location", but for in-memory modifies we were making
**	    the build table reside on the session's work location(s), meaning
**	    that if the in-memory build should overflow, necessitating an
**	    eventual file rename of the build table's file to the destination
**	    table's file, the rename would fail because the two files were in
**	    different locations. This is only a temporary fix, because the
**	    effect of this fix is that temporary tables which are modified
**	    are currently ending up on the root location, when we want
**	    them to be ending up on the work location(s).
**	16-feb-94 (robf)
**          Preserve TCB_ALARM across modifies.
**	2-mar-94 (robf)
**          Check SCB_S_SECURITY flag prior to MAC check. This allows
**	    SYSMOD to operate when necessary.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open 
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them 
**	    prior to close.
**	20-jun-1994 (jnash)
**	    fsync project.  Open (verifydb -table -xtable) or
**	    (modify to table_patch) tables sync-at-end.  This action may be
**	    overridden by load_optim server config param.
**	7-jul-94 (robf)
**	    Added flags mask parameter
**      12-jul-94 (ajc) 
**          Fixed jnash integration such source will compile.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**      09-jan-1995 (nanpr01)
**	    Bugs #66002 & 66028
**	    Added SQL support for setting table to physically inconsistent,
**	    logically inconsistent or table recovery disallowed. Preserve
**	    the values already set.
**	 4-mar-1996 (nick)
**	    Code assumed that the attribute list is prefixed by the key
**	    attributes - this is only true for secondary index builds.  As
**	    a result of this, if index compression was specified, we'd 
**	    calculate the worst case key length based on the wrong 
**	    attributes. #74022
**	06-mar-1996 (stial01 for bryanp)
**	    Add new page_size argument. Set it in the mxcb. Pass DM_PG_SIZE
**	    as the page size when logging the modify for sconcur catalogs.
**	    Pass 0 as the page size when logging the modify to merge.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**          Pass page size to dm2f_build_fcb()
**      06-may-1996 (nanpr01)
**	    Modified dm2u_maxreclen call. Also modified the isam key length
**	    calculation. We never intended to support
**	    larger keys for this project. So we used the DM_COMPAT_PGSIZE
**	    for key length checking.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduction of Distributed Multi-Cache Management (DMCM) protocol.
**          Call to dm2t_xlock_tcb, will trigger callbacks on TCBs for this
**          table in other caches, causing all dirty pages for the table to
**          be written out and hence a consistent image is generated prior to
**          the modify.
**	14-nov-1996 (nanpr01)
**	    bug 79257 - Cannot modify page sizes for in-memory temp table.
**	03-jan-1997 (nanpr01)
**	    Use the #define's rather than hard-coded constants.
**      27-jan-1997 (stial01)
**          Do not include TIDP in the key of unique secondary indexes
**      31-jan-1997 (stial01)
**          Commented code for 27-jan-1997 change
**      10-mar-1997 (stial01)
**          Alloc/init mx_crecord, area to be used for compression.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    New parm, tbl_pri, added to dm2u_modify() prototype, in which
**	    table priority is passed.
**      21-may-1997 (stial01)
**          dm2u_modify() Always include tidp in "key length" for BTREE.
**          For compatibility reasons, DO include TIDP for V1-2k unique indices 
**	23-sep-1997 (shero03)
**	    B85561: add the compression overhead size to the record size
**	    for mx_record.  This prevents a storage overlay.
**	02-Oct-1997 (shero03)
**	    Changed the code to use the mx_log_id in lieu of the xcb_log_id
**	    because the xcb doesn't exist during recovery.
**	9-apr-98 (inkdo01)
**	    Add support for "to persistence/unique_scope = statement" options
**	    (just update relstat) for constraint index with feature.
**      28-may-1998 (stial01)
**          dm2u_modify() Support VPS system catalogs.
**	28-oct-1998 (somsa01)
**	    In the case of Global Temoporary Extension Tables, the lock will
**	    live on the session lock list, not the transaction lock list.
**	    (Bug #94059)
**	03-Dec-1998 (jenjo02)
**	    Pass XCB*, page size, extent start to dm2f_alloc_rawextent().
**	23-Dec-1998 (jenjo02)
**	    TCB->RCB list must be mutexed while it being read.
**	23-dec-1998 (somsa01)
**	    We should be passing lk_list_id to dm2t_control(), not the
**	    xcb_lk_id.
**      18-Oct-1999 (hanal04) Bug 94474 INGSRV633
**          Remove the above change as the lock is now back on the
**          transaction lock list.
**      20-May-2000 (hanal04, stial01 & thaju02) Bug 101418 INGSRV 1170
**          Global session temporary table lock is back on the Session lock 
**          list.
**	21-Aug-2000 (thaju02)
**	    Avoid taking exclusive control lock on temporary table during
**	    modify. Temporary table control locks are not taken during
**	    create/drop time. The gtt exists within the session, not
**	    visible to any other sessions, and the gtt schema can be 
**	    read/changed by this session only. At one time, the gtt control 
**	    lock was taken in dmt_show, but this was removed by somsa01's 
**	    change 439577. An exclusive table lock is taken on the gtt 
**	    at creation. (b102321)
**	02-Oct-2000 (shust01)
**	    In dm2u_modify(), Close the temp table if an error occurred.
**	    Normally, the temp table is closed in update_temp_table_tcb(),
**	    but if an error occurred before then that call wouldn't happen,
**	    and could receive E_DM005D and when quit tm, E_DM9C75, E_DM9270,
**	    E_DM9267, E_SC010E.  bug 101017.
**      15-mar-1999 (stial01)
**          dm2u_modify() can get called if DCB_S_ROLLFORWARD, without xcb,scb
**      12-may-2000 (stial01)
**          modify for retrieve into, pick up the next available page size
**          (another change for SIR 98092)
**      17-Apr-2001 (horda03) Bug 104402.
**          The attribute TCB_NOT_UNIQUE, set by system for non-unique
**          constraints must persist over modifies.
**	18-Dec-2001 (thaju02)
**	    Avoid taking exclusive control lock on temporary table during
**	    modify. Temporary table control locks are not taken during
**	    create/drop time. The gtt exists within the session, not
**	    visible to any other sessions, and the gtt schema can be 
**	    read/changed by this session only. At one time, the gtt control 
**	    lock was taken in dmt_show, but this was removed by somsa01's 
**	    change 439577. An exclusive table lock is taken on the gtt 
**	    at creation. (b102321)
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Initialise the new mx_dmveredo field.
**       5-Nov-2003 (hanal04) Bug 110920 INGSRV2525
**          tcb_comp_atts_count is the size a tuple may grow beyond the
**          uncompressed tuple width when we get poor compression.
**          We must use the dmpp_compexpand() output for the new
**          table's accessors and not the recorded value (tcb_comp_atts_count)
**          from the original table's dmpp_compexpand() accessor.
**          This prevents buffer overrun when calling a dmpp_compress()
**          routine which can grow more than the routine used by the
**          old table structure.
**       7-Jun-2004 (hanal04) Bug 112264 INGSRV2813
**          Correct dm0m_allocate() size for mxcb_cb and associated
**          fields. Investigation of the above bug showed PAD over-run
**          when PAD diagnostics were enabled in dm0m.c
**	14-Jan-2004 (jenjo02)
**	    Check relstat & TCB_INDEX instead of tbl_id->db_tab_index != 0.
**	    Preliminary support of Global indexes and TID8s.
**	6-Feb-2004 (schka24)
**	    Update name generation scheme, eliminate dmu count.
**	28-Feb-2004 (jenjo02)
**	    Massive changes for partitioned modifies, multiple
**	    input and/or output partitions.
**	01-Mar-2004 (jenjo02)
**	    Whoops - forgot to set tbl_lk_mode, etc in MXCB.
**	03-Mar-2004 (jenjo02)
**	    Changes to better deal with partition locations.
**	04-Mar-2004 (jenjo02)
**	    Reorg and truncate don't need Source/Target buffers.
**	    Modify doesn't need tpcb_irecord (index space)
**	    either.
**	08-Mar-2004 (jenjo02)
**	    Still straightening out partition locations. 
**	    Deleted tpcb_olocations, tpcb_oloc_count;
**	    spcb_locations, loc_count now hold Source locations.
**	9-Mar-2004 (schka24)
**	    More bug fixing: fix tcb's before messing with them;
**	    make sure master is opened if modify is being done on a
**	    partition.  Also: don't take checkpoint lock or X control
**	    lock for modify to table_debug.
**	10-Mar-2004 (jenjo02)
**	    When DMU_MASTER_OP and new locations, they must be
**	    applied to Master's iirelation as well as any 
**	    affected Partitions. Make them and save them off in
**	    mx_Mlocations for dm2u_update_catalogs().
**	    (schka24) Simplify location-counting thanks to QEU passing in
**	    old loc count;  fix loop counter handling bug.
**	11-Mar-2004 (jenjo02)
**	    Added TPCB pointer array in mx_tpcb_ptrs.
**	12-Mar-2004 (schka24)
**	    Attempt to fix up lame dm1u verify flag mess.
**	09-Apr-2004 (jenjo02)
**	    Ensure correct alignment of SPCB/TPCB arrays
**	    to avoid 64-bit BUS errors.
**	20-Apr-2004 (schka24)
**	    Add dm2u header to record buffers.
**	20-Apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	30-apr-2004 (thaju02)
**	    Online modify. Added spcb_input_rcb. Log tpcb name id and name gen
**	    in online start modify log record.
**	29-Apr-2004 (jenjo02)
**	    Check for Force-Abort when looping through TPCBs
**	    and doing logging to ensure that we catch LOGFULL
**	    situations before they get out of hand.
**	17-May-2004 (jenjo02)
**	    Fabricate a sort field for partition number. MODIFY
**	    sorts are now done only on the Parent TPCB thread
**	    for all sibling targets.
**	17-may-2004 (thaju02)
**	    Removed unnecessary rnl_name/dum_rnl_name from dm0l_modify().
**	20-may-2004 (thaju02) 
**	    Online modify may fail with "E_US138D Table is already in use". 
**	    With online modify, skip check of tcb_open_count > 1. (B112391)
**	02-jun-2004 (thaju02)
**	    Restriction - online modify can not be performed in a multi-server
**	    fast commit off configuration.
**	    If online modify requirements are not met, report error and fail.
**	    For online modify, hold off flushing pages (dm0p_close_page) until
**	    after do_online_modify().
**      22-jun-2004 (stial01)
**          Disable online modify when rows span pages
**	08-jun-2004 (thaju02)
**	    For rolldb of online modify, save rnl rcb ptr in octx, so
**	    we can close the table at a later point. (B112442)
**	    If rolldb of online modify, allocate rnl btree map. 
**	30-jun-2004 (thaju02)
**	    Add bsf lsn as a param to dm0l_modify().
**	5-Aug-2004 (schka24)
**	    Table access check now a no-op, remove.
**      29-Sep-2004 (fanch01)
**          Conditionally add LK_LOCAL flag if database/table
**	    is confined to one node.
**	27-oct-2004 (thaju02)
**	    If online modify specified and we hold exclusive
**	    table lock, revert to regular modify. (B113335)
**	16-dec-04 (inkdo01)
**	    Init cmp_collID's.
**	18-dec-2004 (thaju02)
**	    Online modify of partitioned table.
**	    - removed input_rcb now use omcb block. 
**	    - removed mx_rnl_online, now DM2U_OSRC_CB list allocated 
**	      in do sidefile.
**	    In do_sidefile():
**	    - in do_sidefile, differentiate between put and put clr
**	      log records.
**	    - if modify table is partitioned create $online
**	      master and partitions.
**	    - omcb_osrc_next is list of modify table partitions'
**	      rcbs to furnish to dm2u_modify/load_table. Replaces
**	      input_rcb. 
**	    - omcb_opcb_next is list of $online partitions' rcbs
**	      used by do_sidefile, do_online_dupchk.  
**	    - omcb_dupchk_tabid is list of tabids for temp tbls
**	      (per partitions) which contains duplicates that 
**	      need to be resolved by do_online_dupchk, indexed by
**	      partno. 
**	11-feb-2005 (thaju02)
**	      If automatic partitions, do not allow online modify.
**	18-Feb-2005 (schka24)
**	     Thinko in location counting loop undercounted new-locations
**	     if repartitioning into fewer partitions but more locns per
**	     partition.  Caused memory overrun, eventual segv.
**	03-Mar-2005 (jenjo02)
**	    Repairs dealing with RAW locations, which got bungled
**	    a bit during the partitioning reorganizing of the code.
**	08-Apr-2005 (thaju02)
**	    In building key map, check that attribute had not been
**	    previously dropped. (B105907)
**	12-Apr-2005 (thaju02)
**	    Account for sizeof(DM_TID8) in calc of buf_size. 
**	14-Apr-2005 (thaju02)
**	    Removed next_cmp++ which had accounted for the TID8 in
**	    the attribute comparison list. Inclusion of the TID8
**	    in mx_cmp_list introduced problems in elimination of 
**	    duplicates. 
**	18-Apr-2005 (jenjo02)
**	    Replaced next_cmp++ for TID8. Problem resolved 
**	    in dm2uuti and dmse.
**      13-Jul-2005 (thaju02)
**          For online modify take X table control lock. (B114861)
**	21-Jul-2005 (jenjo02)
**	    Set DMU_PHYPART_CHAR pointer in MXCB for querying by
**	    dm2u_update_catalogs().
**	26-Jul-2005 (thaju02)
**	    Don't convert tbl control lock here. Re-address
**	    of prev fix for B114861.
**	27-Sep-2005 (jenjo02)
**	    Removed dm2u_raw_device_lock from AddSourceLocation and
**	    inlined AddSourceLocation, seeing as how there was nothing 
**	    left.
**	    I can't fathom why we would care whether a source is RAW 
**	    or not, and taking this X lock here only causes confusion 
**	    later as when one does:
**	    modify x with location=(raw2)
**	    commit
**	    modify x with location=(raw1,raw2)
**		takes X on raw2 (source location)
**		then fails in AddNewLocation on raw2 because the
**		X lock isn't a NEW_LOCK, returning E_DM0189 rather than
**		E_DM0190 "already occupied".
**	30-May-2006 (jenjo02)
**	    Figure TidSize early on, save in mx_tidsize.
**	    Indexes on Clustered tables never include the TID    
**	    as a key attribute.
**	15-Aug-2006 (jonj)
**	    Tweaks to support modify of Temp Table Indexes.
**	1-Sep-2006 (kibro01) bug 116482
**	    Check that table as well as database is journaled before
**	    allowing online modify.
**     14-Oct-2006 (wonca01) BUG 116476
**          Storage location for modified table structures are not being
**          stored correctly because the wrong set of variables are passed
**          to dm0l_modify().  All calls to dm0l_modify() should consistently
**          use the local variables loc_count and &tbl_location.
**	13-Feb-2007 (kschendel)
**	    Set main modify rcb's sid for cancel checking.
**	11-may-2007 (dougi)
**	    Slight changes to allow DM2U_2_STATEMENT_LEVL_UNIQUE in combination
**	    with structure change.
**	11-Jun-2007 (kibro01) b117215
**	    For partitioned tables cycle through each partition before 
**	    performing the verify action on the master table, thus converting
**	    each partition to a heap and then the master to a description
**	    of a partitioned heap.
**	13-Jul-2007 (kibro01) b118695
**	    Pass generated array of DM_LOC_NAME structures to dm0l_modify
**	    rather than the single tbl_location value.
**	25-Jul-2007 (kibro01) b118598
**	    Remove sidefile if online modify fails
**	26-sep-2007 (kibro01) b119080
**	    Allow modify to nopersistence
**	04-Apr-2008 (jonj)
**	    Replace dm2t_xlock_tcb() with more robust 
**	    dm2t_bumplock_tcb(0) which will effect the same thing.
**	17-Dec-2008 (jonj)
**	    B121349: When closing modfiles while handling error, ensure file is 
**	    open before attempting close.
**	    Do not redundantly log partition location information in dm0l_modify;
**	    it may not fit, and it's already been logged in RAWDATA.
**	09-Jan-2009 (jonj)
**	    Repair bad assignment vs equality missed in DB_ERROR conversion:
**	    (error = E_DM9CB1_RNL_LSN_MISMATCH) should be
**	    (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH)
**	6-Nov-2009 (kschendel) SIR 122757
**	    Pass private and optionally direct-io for load file.
**	20-Nov-2009 (kschendel) SIR 122739
**	    Fix goof calculating (new) page size if modifying from
**	    uncompressed to compressed.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Temp tables are not DM_SCONCUR_MAX.
**	13-may-2010 (miket) SIR 122403
**	    Fix net-change logic for width for ALTER TABLE.
**	9-Jul-2010 (kschendel) SIR 123450
**	    Index (btree) key compression is hardwired to old-standard for now.
**	    So are catalogs (if compressed).
**	16-Jul-2010 (jonj) BUG 124096
**	    When modify DMU_ONLINE_START and adding partitions, qeu_modify_prep
**	    does not create the new partitions. If the subject table is
**	    already exclusively locked, we skip do_online_modify(), but
**	    still need to create the partitions that QEU did not.
**	    Made some static functions static, added function prototypes
**	    for some that were missing.
**	27-Jul-2010 (kschendel) b124135
**	    Ask for row-versioning when sizing compression control array.
**	    We don't know if there are any versioned/altered atts, assume
**	    the worst rather than the best.
**	30-Jul-2010 (jonj)
**	    Previous fix for B124096 failed to fill in tabid's in newly
**	    created partitions, causing file rename errors later on.
**	03-Aug-2010 (miket) SIR 122403
**	    Trap attempt to change the structure of an encrypted index;
**	    only HASH is valid. Also remove unreferenced input_is_index.
**	09-Aug-2010 (miket) SIR 122403
**	    Fix check for attempt to make an encrypted column a primary
**	    key by moving the code to the correct loop location.
**	25-Aug-2010 (miket) SIR 122403 SD 145902 CRYPT_FIXME
**	    For now disable modify encrypted index from HASH to HASH
**	    because it is broken.
*/
DB_STATUS
dm2u_modify(
DM2U_MOD_CB	*mcb,
DB_ERROR        *dberr)
{
    DMP_DCB		*dcb = mcb->mcb_dcb;
    DML_XCB		*xcb = mcb->mcb_xcb;

    DM2U_MXCB		*m = (DM2U_MXCB*)0;
    DMP_RCB		*r = (DMP_RCB *)0;
    DMP_RCB		*rcb, *pr;
    DMP_RCB		*master_rcb;
    DMP_TCB		*t, *pt;
    char		*p;
    i4			i,j,k;
    i4			cmplist_count;
    i4			bkey_count;
    i4			data_cmpcontrol_size;
    i4			leaf_cmpcontrol_size;
    i4			index_cmpcontrol_size;
    i4			next_cmp;
    i4			key_map[(DB_MAX_COLS + 31) /32];
    DB_TAB_TIMESTAMP	timestamp;
    i4			error, local_error;
    DB_STATUS		status, local_status;
    DB_TAB_NAME		table_name;
    DB_OWN_NAME		owner_name;
    DB_LOC_NAME		tbl_location;
    i4			relstat;
    DM_ATT		mod_key[DB_MAX_COLS + 1];
    char		key_order[DB_MAX_COLS + 1];
    DML_SCB             *scb;
    bool		syscat;
    bool                sconcur;
    i4             	lk_mode;
    i4             	truncate = 0;
    i4             	reorg = 0;
    i4             	duplicates;
    DMP_LOCATION        *MlocArray;
    DMP_LOCATION        *nlocArray;
    DMP_LOCATION        *olocArray;
    DB_LOC_NAME	        *locnArray, *orig_locnArray;
    DB_LOC_NAME         *PartLoc;
    i4             	loc_count, oloc_count, Mloc_count;
    i4             	compare;
    i4			journal;
    i4			dm0l_jnlflag;
    u_i4		db_sync_flag;
    i4			buckets;
    i4			dt_bits;
    i4			gateway = 0;
    i4			view	= 0;
    i4			dm0l_structure;
    ADF_CB		*adf_cb;
    i4			logging;
    LG_LSN		lsn;
    i4			table_access_mode;
    bool		modfile_open = FALSE;
    bool		temp_cleanup = FALSE;
    i4			size_align;
    i4             	timeout = 0;
    i4             	TableLockList, lk_id;
    f8		     	*range_entry;
    i2		     	name_id;
    i4               	name_gen, base_name_gen;
    DB_TRAN_ID       	*tran_id;
    i4               	log_id;
    i4		     	pgtype_flags;
    i4               	row_expansion;   /* Worst case growth of compression */
    i4			key_expansion;
    i4			sources;
    i4			targets;
    DM2U_SPCB		*sp;
    DM2U_TPCB		*tp;
    DM2U_TPCB		**tppArray;
    i4			tppArrayCount;
    i4			buf_size;
    char		*bufArray;
    i4			repartition;
    i4			partno;
    i4			all_sources;
    DB_TAB_ID		*part_tabid;
    DMU_PHYPART_CHAR	*part;
    i4			Tallocation, Textend;
    DB_TAB_ID		master_tabid;
    i4			old_nparts, new_nparts;
    bool		is_table_debug;
    bool		online_modify = FALSE;
    DMP_RELATION	online_reltup;
    DB_TAB_ID		online_tabid;
    i4			table_lock_mode;
    LK_LOCK_KEY		lock_key;
    RFP_OCTX		*octx = 0;
    bool		already_logged = FALSE;
    LG_LSN		bsf_lsn;
    DMP_RCB		*input_rcb = (DMP_RCB *)0;
    i4			OnlyKeyLen = 0;	/* The size of just the key atts */
    DB_ERROR		local_dberr;
    i4			attnmsz;
    char		*nextattname;
    DB_ATTS		*curatt;
    CL_ERR_DESC		sys_err;

    CLRDBERR(dberr);
		
    if (mcb->mcb_omcb)
	input_rcb = mcb->mcb_omcb->omcb_osrc_next->osrc_master_rnl_rcb;

    is_table_debug = ((mcb->mcb_verify & DM1U_OPMASK) == DM1U_DEBUG);

    /* Make sure we acknowlege input temp tables/indexes */
    if ( mcb->mcb_tbl_id->db_tab_base < 0 )
	mcb->mcb_temporary = TRUE;

    if (!mcb->mcb_temporary
      && (dcb->dcb_status & DCB_S_ROLLFORWARD) == 0
      && !is_table_debug)
    {
	status = dm2u_ckp_lock(dcb, (DB_TAB_NAME *)NULL, (DB_OWN_NAME *)NULL,
				xcb, dberr);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    dmf_svcb->svcb_stat.utl_modify++;

    /*	
    ** Initialization. 
    */
    if (mcb->mcb_modoptions & DM2U_TRUNCATE)
	truncate = 1;
    if (mcb->mcb_modoptions & DM2U_REORG)
	reorg = 1;

    /* regular vs journal redo modify don't agree here, make them agree */
    if (mcb->mcb_l_count == 0)
	mcb->mcb_location = NULL;

    /*
    ** So that the tables status can be modified it is opened
    ** for DM2T_A_OPEN_NOACCESS access.
    ** Sync-at-close is used for table PATCH and VPATCH operations to 
    ** speed them up (it is a WRITE access mode variant).
    ** It is expected that these operations do not operate in gateways  
    ** (if they do its a bug, because gateway-specific DM2T_A_MODIFY 
    ** logic in dm2t_open() will not be executed.  There is a check
    ** for this below.)  This action may be overridden by a config option. 
    */
    if (mcb->mcb_mod_options2 & DM2U_2_CAT_ACTIONS)
	table_access_mode = DM2T_A_OPEN_NOACCESS;
    else if (((mcb->mcb_verify & DM1U_OPMASK) == DM1U_PATCH ||
    	   (mcb->mcb_verify & DM1U_OPMASK) == DM1U_FPATCH) )
	table_access_mode = DM2T_A_SYNC_CLOSE;
    else if (is_table_debug)
	table_access_mode = DM2T_A_READ_NOCPN;
    else
	table_access_mode = DM2T_A_MODIFY;


    lk_mode = DM2T_X;
    if (is_table_debug)
	lk_mode = DM2T_S;
    MEfill(sizeof(mod_key), '\0', (PTR) mod_key);
    MEfill(sizeof(key_order), '\0', (PTR) key_order);
    MEfill(sizeof(LG_LSN), 0, &bsf_lsn);
    *mcb->mcb_tup_info = 0;

    if (dcb->dcb_status & DCB_S_ROLLFORWARD)
    {
	scb = (DML_SCB *)0;
	log_id = mcb->mcb_rfp_entry->rfp_log_id;
	lk_id = mcb->mcb_rfp_entry->rfp_lk_id;
	TableLockList = lk_id;
	tran_id = &mcb->mcb_rfp_entry->rfp_tran_id;
	name_id = mcb->mcb_rfp_entry->rfp_name_id;
	name_gen = mcb->mcb_rfp_entry->rfp_name_gen;
	if (mcb->mcb_omcb)
	    octx = (RFP_OCTX *)mcb->mcb_omcb->omcb_octx;
    }
    else
    {
	scb = xcb->xcb_scb_ptr;
	if (mcb->mcb_temporary)
	    TableLockList = scb->scb_lock_list;
	else
	    TableLockList = xcb->xcb_lk_id;
	lk_id = xcb->xcb_lk_id;
	/* Get timeout from set lockmode */
	timeout = dm2t_get_timeout(scb, mcb->mcb_tbl_id); 
	log_id = xcb->xcb_log_id;
	tran_id = &xcb->xcb_tran_id;
	name_id = dmf_svcb->svcb_tfname_id;
	/* name_gen not set until we know how many we need */

	if (!mcb->mcb_temporary)
	{
	    i4		length;
	    LK_LKB_INFO	lock_info;
	    i4		control_lock_mode;

	    control_lock_mode = lk_mode;

	    if (mcb->mcb_flagsmask & DMU_ONLINE_START)
	    {

		/*
		** Check the subject table's lock mode, if any,
		** save it in table_lock_mode.
		**
		** If exclusively locked, we won't need an online
		** modify lock or a control lock, and we won't
		** need to call do_online_modify(), but because
		** qeu_modify_prep() didn't create new partitions,
		** we'll have to do that instead, later.
		*/

		lock_key.lk_type = LK_TABLE;
		lock_key.lk_key1 = (i4)dcb->dcb_id;
		lock_key.lk_key2 = mcb->mcb_tbl_id->db_tab_base;
		lock_key.lk_key3 = 0;
		lock_key.lk_key4 = 0;
		lock_key.lk_key5 = 0;
		lock_key.lk_key6 = 0;

		/* length == 0 if no matching key */
		status = LKshow(LK_S_OWNER_GRANT, TableLockList,
				(LK_LKID*)NULL, &lock_key,
				sizeof(lock_info), (PTR)&lock_info,
				&length, NULL, &sys_err);

		
		/* Remember subject table's lock mode, if any */
		if ( status == OK && length )
		    table_lock_mode = lock_info.lkb_grant_mode;
		else
		    table_lock_mode = LK_N;

		if ( table_lock_mode == LK_X )
		    control_lock_mode = LK_N;
		else
		{
		    lock_key.lk_type = LK_ONLN_MDFY;
		    lock_key.lk_key1 = (i4)dcb->dcb_id;
		    lock_key.lk_key2 = mcb->mcb_tbl_id->db_tab_base;
		    lock_key.lk_key3 = mcb->mcb_tbl_id->db_tab_index;
		    lock_key.lk_key4 = 0;
		    lock_key.lk_key5 = 0;
		    lock_key.lk_key6 = 0;
		    status = LKrequest(dcb->dcb_bm_served == DCB_SINGLE ?
				       LK_LOCAL | LK_PHYSICAL : LK_PHYSICAL,
				       TableLockList, &lock_key, LK_X, 
				       (LK_VALUE *)NULL, (LK_LKID*)NULL, 
				       timeout, &sys_err);
		    if (status != OK)
		    {
			uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 
				    0, NULL, &local_error, 0);
			if ( status == LK_TIMEOUT ) 
			    SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
			else if ( status == LK_DEADLOCK )
			    SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
			else if ( status == LK_NOLOCKS )
			    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
			else if ( status == LK_INTR_FA )
			    SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
			else
			{
			    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err,
				      ULE_LOG, NULL, NULL, 0, NULL, &local_error, 
				      2, 0, LK_X, 0, TableLockList);
			    SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);
			}
			return(E_DB_ERROR);
		    }
		    control_lock_mode = LK_S;
		}
	    }

	    if ( control_lock_mode != LK_N )
	    {
		status = dm2t_control(dcb, mcb->mcb_tbl_id->db_tab_base, TableLockList, 
			    control_lock_mode, (i4)0, timeout, dberr);
		if (status != E_DB_OK)
		    return(E_DB_ERROR);
	    }
	}
    }

    /* Now that we have the control lock if needed, adjust lock mode
    ** for core catalogs.
    */
    if ( mcb->mcb_tbl_id->db_tab_base > 0 &&
         mcb->mcb_tbl_id->db_tab_base <= DM_SCONCUR_MAX)
    {
	lk_mode = DM2T_IX;
    }

    master_rcb = NULL;
    for(;;)
    {   
	if (mcb->mcb_flagsmask & DMU_ONLINE_START) 
	{
	    lk_mode = DM2T_N;
	    table_access_mode = DM2T_A_READ_NOCPN;
	}

	/*  Open up the table.
	** If the specified table is a partition, open the master first.
	*/

	if ( mcb->mcb_tbl_id->db_tab_index & DB_TAB_PARTITION_MASK )
	{
	    master_tabid.db_tab_base = mcb->mcb_tbl_id->db_tab_base;
	    master_tabid.db_tab_index = 0;
	    status = dm2t_open(dcb, &master_tabid, lk_mode,
			DM2T_UDIRECT, table_access_mode,
			timeout, 20, 0, log_id,
			TableLockList, 0, 0,
			mcb->mcb_db_lockmode,
			tran_id, &timestamp, &master_rcb, NULL, dberr);
	    if (status != E_DB_OK)
		break;
	}
	status = dm2t_open(dcb, mcb->mcb_tbl_id, lk_mode, 
            DM2T_UDIRECT, table_access_mode,
	    timeout, (i4)20, (i4)0, log_id, 
            TableLockList, (i4)0, (i4)0, 
            mcb->mcb_db_lockmode, 
	    tran_id, &timestamp, &rcb, (DML_SCB *)0, dberr);
	if (status != E_DB_OK) 
	    break;
	r = rcb;
	t = r->rcb_tcb_ptr;
	r->rcb_xcb_ptr = xcb;
	if (scb && (scb->scb_s_type & SCB_S_FACTOTUM) == 0)
	    r->rcb_sid = scb->scb_sid;

	/* Silently avoid hidata compression on catalogs.  Also ensure that
	** we use OLD-standard.  Work in dm2d.c would be needed to allow
	** NEW-standard compression on core catalogs, best to just
	** avoid the issue.
	*/
	if (t->tcb_rel.relstat & TCB_CATALOG && mcb->mcb_compressed != TCB_C_NONE)
	    mcb->mcb_compressed = TCB_C_STD_OLD;

	/*
	** If input is index, this is online modify of secondary index 
	** We must treat modify table as index so that tidp gets added to key
	** if necessary (and m->mx_context.mct_keys gets set properly)
	** FIX ME change wherever checking TCB_INDEX or db_tab_index
	*/
	if ((t->tcb_rel.relstat & TCB_INDEX) ||
	    (input_rcb && (input_rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_INDEX)))
	{
	    /* ...just to be safe... */
	    mcb->mcb_clustered = FALSE;
	    /* an encrypted secondary index can ONLY be structure hash */
	    if (t->tcb_data_rac.encrypted_attrs == TRUE &&
#if 0
CRYPT_FIXME This should be allowed, in theory, but creates encryption
CRC errors right now, so comment out and revisit when time allows.
		mcb->mcb_structure != TCB_HASH &&
#endif
		mcb->mcb_structure != 0)
	    {
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM019F_INVALID_ENCRYPT_INDEX);
		break;
	    }
	}

        /*
        ** (ICL phil.p)
        ** Take exclusive TCB lock to force other caches to flush
        ** all dirty pages for this table. Unnecessary unless the
        ** database can be opened by more than one server.
        */
        if ((dcb->dcb_status & DCB_S_DMCM) &&
            (dcb->dcb_served == DCB_MULTIPLE) &&
	    ! is_table_debug &&
	    !mcb->mcb_temporary )
        {
	    /* Bumplock with increment of zero, which will just get LK_X */
	    status = dm2t_bumplock_tcb(dcb, t, (DB_TAB_ID*)NULL, (i4)0, dberr);

            if (status != E_DB_OK)
		break;
        }

 	/*
	** Sanity check that Gateway tables do not have patch operations
 	** applied on them.  dm2t_open() turns WRITE access to gateway 
   	** tables into MODIFY access.  That action will not occur for 
	** patch or vpatch modifies where we open the table sync-at-close.
	** We check here to make sure Gateway's don't do these operations.
	*/
	if ((t->tcb_rel.relstat & TCB_GATEWAY) &&
	    (table_access_mode == DM2T_A_SYNC_CLOSE))
	{
	    uleFormat(NULL, W_DM9A11_GATEWAY_PATCH, (CL_ERR_DESC *)NULL, ULE_LOG , NULL,
	            (char * )NULL, 0L, (i4 *)NULL, &local_error, 2, 
		    sizeof(DB_DB_NAME), &dcb->dcb_name, 
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name);
	}

	journal = ((t->tcb_rel.relstat & TCB_JOURNAL) != 0);
	syscat = ((t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)) != 0);
	sconcur = ((t->tcb_rel.relstat & TCB_CONCUR) != 0);
	gateway = ((t->tcb_rel.relstat & TCB_GATEWAY) != 0);
	view    = ((t->tcb_rel.relstat & TCB_VIEW) != 0);
	dm0l_jnlflag = (journal ? DM0L_JOURNAL : 0);

	if (mcb->mcb_has_extensions)
	{
	    *mcb->mcb_has_extensions = t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS;
	}

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if ( dcb->dcb_status & DCB_S_ROLLFORWARD
	    || xcb->xcb_flags & XCB_NOLOGGING
	    || mcb->mcb_temporary )
	    r->rcb_logging = 0;

	if ((dcb->dcb_status & DCB_S_ROLLFORWARD) 
	    || (xcb->xcb_flags & XCB_NOLOGGING)
	    || (t->tcb_temporary == TCB_TEMPORARY && t->tcb_logging == 0))
	    logging = 0;
	else
	    logging = 1;


	/* Can't modify merge on non-btree tables. */

	if (mcb->mcb_merge && t->tcb_rel.relspec != TCB_BTREE)
	{
	    status = dm2t_close(r, DM2T_NOPURGE, dberr);
	    SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
	    break;
	}

	/*
	** cannot modify temp table to different page size or page type
	*/
	if ( mcb->mcb_temporary && !mcb->mcb_clustered )
	{
	    if ( mcb->mcb_page_size && mcb->mcb_page_size != t->tcb_table_io.tbio_page_size || 
		 mcb->mcb_page_type && mcb->mcb_page_type != t->tcb_table_io.tbio_page_type )
	    {
		SETDBERR(dberr, 0, E_DM0183_CANT_MOD_TEMP_TABLE);
		break;
	    }
	    mcb->mcb_page_size = t->tcb_table_io.tbio_page_size;
	    mcb->mcb_page_type = t->tcb_table_io.tbio_page_type;
	}
	if (mcb->mcb_temporary && mcb->mcb_index_compressed)
	{
	    SETDBERR(dberr, 0, E_DM0200_MOD_TEMP_TABLE_KEY);
	    break;
	}

	/*
	** Can't modify gateway table or view to TRUNCATED, MERGE, REORGANIZE,
	** ADD_EXTEND or [NO]READONLY nor change the structure
	** of any of the verifydb options.
	*/
	if ( gateway || view )
	{
	    if (truncate || reorg || mcb->mcb_merge 
				|| 
			(mcb->mcb_mod_options2 & DM2U_2_ADD_EXTEND)
				||
			(mcb->mcb_mod_options2 & DM2U_2_CAT_ACTIONS)
				||
        		(mcb->mcb_verify != 0)
				)
	    {
		if ( gateway )
		    SETDBERR(dberr, 0, E_DM0137_GATEWAY_ACCESS_ERROR);
		else
		    SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);

		break;
	    }
	}
        /*
        ** Preserve relstat2 bits for row label/audit/alarm indicators
	** The ensures security attributes are not removed when
	** modified.
        */
        if (t->tcb_rel.relstat2 & TCB_ROW_AUDIT)
		mcb->mcb_relstat2|= TCB_ROW_AUDIT;
        if (t->tcb_rel.relstat2 & TCB_ALARM)
		mcb->mcb_relstat2|= TCB_ALARM;

	/* Preserve relstat2 bits for byte swapping of etab keys */
	if (t->tcb_rel.relstat2 & TCB2_BSWAP)
		mcb->mcb_relstat2 |= TCB2_BSWAP;

	/* Preserve the relstat2 bit value for altered_column if 
	** MODIFY TO MERGE, else reset the flag value.
	*/

	if (mcb->mcb_merge && (t->tcb_rel.relstat2 & TCB2_ALTERED))
		mcb->mcb_relstat2 |= TCB2_ALTERED;


	/* preserve the relstat2 bit for physical locking */
	if (t->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR)
		mcb->mcb_relstat2 |= TCB2_PHYSLOCK_CONCUR;

	/* Get current duplicates value.  If duplicates are
        ** allowed and we asked to turn them off, set duplicates 
        ** off.  If off and we asked to turn them on, then 
        ** set them on.  Otherwise leave as they are.
	*/

	duplicates = 0;
	if (t->tcb_rel.relstat & TCB_DUPLICATES) 
	    duplicates = 1;
	if (duplicates)
	{
	    if (mcb->mcb_modoptions & DM2U_NODUPLICATES)
		duplicates = 0;
	}
	else
	{
	    if (mcb->mcb_modoptions & DM2U_DUPLICATES)
		duplicates = 1;
	}
		
	if ( !(mcb->mcb_flagsmask & DMU_ONLINE_START) && 
	    t->tcb_open_count > 1 && sconcur == 0)  
	{
	    SETDBERR(dberr, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}		
	
	/* Can't update a system catalog without special privilege. */
	if (syscat &&
	    (dcb->dcb_status & DCB_S_ROLLFORWARD) == 0 &&
	    (scb->scb_s_type & SCB_S_SYSUPDATE) == 0)
	{
	    SETDBERR(dberr, 0, E_DM005E_CANT_UPDATE_SYSCAT);
	    break;
	}

	/* Check for catalog-only modifies, and do them out-of-line. */
	if (mcb->mcb_mod_options2 & DM2U_2_CAT_ACTIONS)
	{
	    status = dm2u_catalog_modify(mcb, r, dberr);
	    if (status != E_DB_OK)
		break;
	    r = NULL;			/* catalog-modify closed this */

	    /* Close master if we opened it (no need to purge).
	    ** Catalog-modifies are (probably?) illegal for partitions,
	    ** but let's go thru the motions here.
	    */
	    if (master_rcb != NULL)
	    {
		status = dm2t_close(master_rcb, 0, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    return (E_DB_OK);
	}

	/*
	** Check for special CHECK_TABLE and PATCH_TABLE options.
	** These are currently done through the modify interface.
	*/
        if (mcb->mcb_verify != 0)
        {
	    /* Note that for partitioned tables we run dm1u_verify for each
	    ** partition, but then also for the master table to update its
	    ** description.  NOTE they must all match. (kibro01) b117215
	    */
	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		/* Do all partitions rather than using a selective list, since
		** it isn't valid to do this for just part of the table.
		*/
		mcb->mcb_nparts = t->tcb_rel.relnparts;
		for ( i = 0; status == E_DB_OK && i < mcb->mcb_nparts; i++ )
		{
		    part_tabid = &t->tcb_pp_array[i].pp_tabid;
		    /* Open the partition */
		    status = dm2t_open(dcb, 
				part_tabid,
				lk_mode, DM2T_UDIRECT,
				table_access_mode,
				timeout, (i4)20, (i4)0, log_id, 
				TableLockList, (i4)0, (i4)0, 
				mcb->mcb_db_lockmode, 
				tran_id, &timestamp, &pr, 
				(DML_SCB *)0, dberr);

		    if ( status == E_DB_OK )
		    {
			pr->rcb_xcb_ptr = xcb;
                	if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
	                    pr->rcb_uiptr = &scb->scb_ui_state;         

       		        status = dm1u_verify(pr, xcb, mcb->mcb_verify, dberr);

			local_status = dm2t_close(pr,
					DM2T_NOPURGE,
					&local_dberr);
			if ( status == E_DB_OK )
			{
			    status = local_status;
			    *dberr = local_dberr;
			}
		    }
		}
                if (status != E_DB_OK)
                    break;  
	    }
            /* Since we are really going to do the table check,
            ** have rcb point to scb_ui_state so user interrupt
            ** can be monitored. */
            if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
                r->rcb_uiptr = &scb->scb_ui_state;         

            status = dm1u_verify(r, xcb, mcb->mcb_verify, dberr);
            if (status != E_DB_OK)
		break;

            /*
            **  Close the table.  Purge the table since uncontrolled scans of
            **  the table may have fixed FHDR or FMAP pages in the wrong
            **  state.  This will eliminate any side-effects in future
            **  queries.
            */
	    i = DM2T_PURGE;
	    if (is_table_debug)
		i = 0;
            status = dm2t_close(r, i, dberr);
            r = 0;
            if (status != E_DB_OK)
		break;
	    /* Close master if we opened it (no need to purge) */
	    if (master_rcb != NULL)
	    {
		status = dm2t_close(master_rcb, 0, dberr);
		if (status != E_DB_OK)
		    break;
		master_rcb = NULL;
	    }
            return (E_DB_OK);
        }


	/* 
	** For non-PARTITIONED modifies...
	**
	** Check the values of extend and allocation if these values
	** are zero then they were not specified on the MODIFY command
	** so take the default values from iirelation. 
	** If a multi-location table make sure that the allocation and
	** extend sizes are factors of DI_EXTENT_SIZE. This is for
	** performance reasons.
	**
	** For PARTITIONED modifies, we'll have to figure this
	** out on a partition-by-partition basis, later.
	*/
	if ( mcb->mcb_allocation <= 0 )
	    mcb->mcb_allocation = t->tcb_rel.relallocation;
	if ( mcb->mcb_extend <= 0 )
	    mcb->mcb_extend = t->tcb_rel.relextend;

	if ( !(t->tcb_rel.relstat & TCB_IS_PARTITIONED) )
	{
	    /* If locations given, use them for new files, else
	    ** use the current ones. */
	    if ( mcb->mcb_location )
		loc_count = mcb->mcb_l_count;
	    else
		loc_count = t->tcb_table_io.tbio_loc_count;

	    oloc_count = t->tcb_table_io.tbio_loc_count;

	    if ( mcb->mcb_allocation && loc_count > 1 )
		dm2f_round_up(&mcb->mcb_allocation);

	    if ( mcb->mcb_extend && loc_count > 1 )
		dm2f_round_up(&mcb->mcb_extend);
	}


	/*
	** MODIFY TO ADD_EXTEND:
	**
	** If the ADD_EXTEND option the call the dm1p layer to add the
	** space to the table and return
	*/
	if ( mcb->mcb_mod_options2 & DM2U_2_ADD_EXTEND )
	{
	    if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED )
	    {
		/* All, or selective partitions? */
		if ( !mcb->mcb_partitions )
		    mcb->mcb_nparts = t->tcb_rel.relnparts;
		    
		for ( i = 0; status == E_DB_OK && i < mcb->mcb_nparts; i++ )
		{
		    if ( mcb->mcb_partitions )
		    {
			/* Selective list, skip "ignore me's" */
			if ( mcb->mcb_partitions[i].flags & DMU_PPCF_IGNORE_ME )
			    continue;
			part_tabid = &mcb->mcb_partitions[i].part_tabid;
		    }
		    else
			/* All partitions */
			part_tabid = &t->tcb_pp_array[i].pp_tabid;
		    
		    /* Open the partition */
		    status = dm2t_open(dcb, 
				part_tabid,
				lk_mode, DM2T_UDIRECT,
				table_access_mode,
				timeout, (i4)20, (i4)0, log_id, 
				TableLockList, (i4)0, (i4)0, 
				mcb->mcb_db_lockmode, 
				tran_id, &timestamp, &pr, 
				(DML_SCB *)0, dberr);

		    if ( status == E_DB_OK )
		    {
			/* Extend the partition */
			pt = pr->rcb_tcb_ptr;

			if ( (Textend = mcb->mcb_extend) > 0 )
			{
			    if ( pt->tcb_table_io.tbio_loc_count > 1 )
				dm2f_round_up(&Textend);
			}
			else
			    Textend = pt->tcb_rel.relextend;

			if ( Textend )
			    status = dm1p_add_extend(pr,
					    Textend,
					    DM1P_ADD_NEW,
					    dberr);

			local_status = dm2t_close(pr,
					DM2T_NOPURGE,
					&local_dberr);
			if ( status == E_DB_OK )
			{
			    status = local_status;
			    *dberr = local_dberr;
			}
		    }
		}
	    }
	    else
	    {
		/* Singleton table or partition */
		if ( mcb->mcb_extend )
		    status = dm1p_add_extend(r, mcb->mcb_extend, DM1P_ADD_NEW, dberr);
		if ( status != E_DB_OK )
		    break;
	    }

	    /*
	    **  Close the (master) table. 
	    **  There is no need to purge the table as
	    **  the the space management code can deal with other DBMS
	    **  extending a table.
	    */
	    status = dm2t_close(r, DM2T_NOPURGE, dberr);
	    r = 0;
	    if (status != E_DB_OK)
		break;
	    /* Close master if we opened it (no need to purge) */
	    if (master_rcb != NULL)
	    {
		status = dm2t_close(master_rcb, 0, dberr);
		if (status != E_DB_OK)
		    break;
		master_rcb = NULL;
	    }

	    return (E_DB_OK);
	}


	STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, table_name);
	STRUCT_ASSIGN_MACRO(t->tcb_rel.relowner, owner_name);
	STRUCT_ASSIGN_MACRO(t->tcb_rel.relloc, tbl_location);
	if (mcb->mcb_tab_name)
	    *mcb->mcb_tab_name = table_name;
	if (mcb->mcb_tab_owner)
	    *mcb->mcb_tab_owner = owner_name;

	/* Start relstat with only TCB_IS_PARTITIONED */
	relstat = t->tcb_rel.relstat & TCB_IS_PARTITIONED;

	/*
	** CLUSTERED forces Unique, 
	** transfers data compression to index compression,
	** turns off data compression.
	**
	** "fillfactor" applies to data pages and is ignored,
	** "leaffill" and "nonleaffill" have their usual meanings.
	*/
	if ( mcb->mcb_clustered )
	{
	    mcb->mcb_unique = TRUE;
	    if ( mcb->mcb_compressed != TCB_C_NONE )
		mcb->mcb_index_compressed = TRUE;
	    mcb->mcb_compressed = TCB_C_NONE;
	}

	if ( mcb->mcb_unique )
	    relstat |= TCB_UNIQUE;
	if (mcb->mcb_compressed != TCB_C_NONE)
	    relstat |= TCB_COMPRESSED;
	if (mcb->mcb_index_compressed)
	    relstat |= TCB_INDEX_COMP;
	if (duplicates)
	    relstat |= TCB_DUPLICATES;
	if (gateway)
	    relstat |= TCB_GATEWAY;

	if (t->tcb_rel.relstat & TCB_INDEX)
	    pgtype_flags = DM1C_CREATE_INDEX;
	else if ( t->tcb_extended )
	    pgtype_flags = DM1C_CREATE_ETAB;
	else
	    pgtype_flags = DM1C_CREATE_DEFAULT;

	if (syscat)
	    pgtype_flags |= DM1C_CREATE_CATALOG;

	/* set DM1C_CREATE_CORE for SCONCUR including iidevices,iisequence */
	if (t->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR)
	    pgtype_flags |= DM1C_CREATE_CORE;

	/* Clustered can't have V1 page type */
	if ( mcb->mcb_clustered )
	    pgtype_flags |= DM1C_CREATE_CLUSTERED;

	/* 
	** For retrieve into.. without page size 
	** If current page size/page type does not fit the row, 
	** pick a new one.
	**
	** Get worstcase-expansion of existing row based on NEW compression
	** (is zero if no compression).
	*/
	row_expansion = dm1c_compexpand(mcb->mcb_compressed,
			t->tcb_data_rac.att_ptrs,
			t->tcb_data_rac.att_count);

	size_align = t->tcb_rel.relwid + row_expansion;
	if (mcb->mcb_page_size == 0 && (mcb->mcb_flagsmask & DMU_RETINTO) &&
	    mcb->mcb_compressed != t->tcb_data_rac.compression_type &&
	    size_align > dm2u_maxreclen(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize)) 
	{
	    i4 pgsize;
	    i4 pgtype;
	    i4 found = FALSE;

	    for (pgsize = t->tcb_rel.relpgsize*2; pgsize <= 65536; pgsize *= 2)
	    {
		/* choose page type */
		status = dm1c_getpgtype(pgsize, pgtype_flags,
			size_align, &pgtype);

		if (status == E_DB_OK &&
		    (size_align <= dm2u_maxreclen(pgtype, pgsize)) &&
		    (dm0p_has_buffers(pgsize)))
		{
		    found = TRUE;
		    mcb->mcb_page_size = pgsize;
		    mcb->mcb_page_type = pgtype;
		    break;
		}
	    }
	    if (!found)
	    {
		SETDBERR(dberr, 0, E_DM0103_TUPLE_TOO_WIDE);
		break;
	    }
	}
	else
	{

	    /* If no page_size given, don't change the page size */
	    if (mcb->mcb_page_size == 0)
		mcb->mcb_page_size = t->tcb_rel.relpgsize;

	    /* choose page type (only recovery and sysmod provide page_type) */
	    if (mcb->mcb_page_type == TCB_PG_INVALID)
	    {
		status = dm1c_getpgtype(mcb->mcb_page_size, pgtype_flags,
		    size_align, &mcb->mcb_page_type) ;
		if (status != E_DB_OK)
		{
		    SETDBERR(dberr, 0, E_DM000E_BAD_CHAR_VALUE);
		    break;
		}
	    }
	}

	/*
	** Disallow rows spanning pages is duplicate row checking possible
	** (Duplicate row checking in dm1b, dm1h, dm1i not supported for
	** rows spanning pages)
	*/
	if (size_align > dm2u_maxreclen(mcb->mcb_page_type, mcb->mcb_page_size)
		&& !duplicates)
	{
	    SETDBERR(dberr, 0, E_DM0159_NODUPLICATES);
	    break;
	}

	/* Don't ever change core-catalog compression from what it was
	** originally.  (Fixing this would need work in dm2d, DB open time.)
	*/
	if (t->tcb_rel.relstat & TCB_CONCUR)
	{
	    mcb->mcb_compressed = t->tcb_rel.relcomptype;
	}

	/* 
	** Cannot specify the relation index for a modify
        ** command, it is automatically done when you specify
        ** the relation table.  Also the database must be
        ** opened exclusively, and you cannot modify to 
        ** anything but hash.
	** (no scb if rollforward, was checked the first time)
	**
	** also disallow modify of iisequence (DM_B_SEQ_TAB_ID)
	** to anything other than hash (b122651)
	*/
	if (((t->tcb_rel.relstat & TCB_CONCUR) ||
	     (t->tcb_rel.relstat2 & TCB2_PHYSLOCK_CONCUR))
	    && (dcb->dcb_status & DCB_S_ROLLFORWARD) == 0
            && ((scb->scb_oq_next->odcb_lk_mode != ODCB_X)
		|| t->tcb_rel.relstat & TCB_INDEX
                || (mcb->mcb_structure != TCB_HASH)
		|| ((t->tcb_rel.relstat & TCB_CONCUR) &&
			(mcb->mcb_compressed != t->tcb_rel.relcomptype))
		|| (mcb->mcb_mod_options2 & 
			(DM2U_2_READONLY | DM2U_2_NOREADONLY |
			 DM2U_2_LOG_INCONSISTENT | DM2U_2_LOG_CONSISTENT |
			 DM2U_2_PHYS_INCONSISTENT | DM2U_2_PHYS_CONSISTENT |
			 DM2U_2_TBL_RECOVERY_DISALLOWED | DM2U_2_TBL_RECOVERY_ALLOWED))
	   ) ) 
	{
	    if ((mcb->mcb_structure != TCB_HASH) || (mcb->mcb_compressed != TCB_C_NONE))
		SETDBERR(dberr, 0, E_DM005A_CANT_MOD_CORE_STRUCT);
	    else
		SETDBERR(dberr, 0, E_DM005E_CANT_UPDATE_SYSCAT);
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &local_error, 0);
	    status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);
	    r = 0;
	    break;
	}		

	/* 
	** SYSMOD:
	**
	** If it is one of the special system catalogs denoted
        ** by sconcur, then call the DM2U_SYSMOD routines.  
        */
	if (sconcur)
	{
	    /* For the iidevices table, let it be modified
            ** from heap to hash the first time, since this
            ** is the only sconcur table not part of the 
            ** templates.
            */

	    if ((t->tcb_rel.reltid.db_tab_base != DM_B_DEVICE_TAB_ID) ||
		(t->tcb_rel.relspec != TCB_HEAP))
	    {
		/*
		** Store actual tuple count into reltups value.  The value
		** is passed to the sysmod call below which will call this
		** routine again to modify the iirtemp table passing this
		** reltups value.
		*/
		i4 toss_relpages;

		/* Mutex the TCB while reading the RCBs */
		dm0s_mlock(&t->tcb_mutex);

		dm2t_current_counts(t, &mcb->mcb_reltups, &toss_relpages);

		dm0s_munlock(&t->tcb_mutex);

		/*  Close no purge the table. */
		status = dm2t_close(r, DM2T_NOPURGE, dberr);
		r = 0;
		if (status != E_DB_OK)
		    break;

		/* 
		** Force any pages left in buffer cache for this table. 
		** This is required for recovery as a "non-redo" log record 
		** immediately follows.
		*/
		status = dm0p_close_page(t, lk_id, log_id,
					DM0P_CLCACHE, dberr);
		if (status != E_DB_OK)
		    break;

		/*
		** Log the sysmod operation - unless logging is disabled.
		** Don't journal the operation as we don't want it redone
		** in rollforward.
		*/
		if (logging)
		{
		    status = dm0l_modify(log_id, 0, mcb->mcb_tbl_id, 
			&table_name, &owner_name, loc_count, &tbl_location, 
			DM0L_SYSMOD, relstat, mcb->mcb_min_pages, mcb->mcb_max_pages, 
			mcb->mcb_i_fill, mcb->mcb_d_fill, mcb->mcb_l_fill, mcb->mcb_reltups, 0, mod_key, key_order, 
			mcb->mcb_allocation, mcb->mcb_extend, mcb->mcb_page_type, mcb->mcb_page_size, 0, 
			(DB_TAB_ID *)0, bsf_lsn, 0, 0, (LG_LSN *)0, &lsn, dberr);
		    if (status != E_DB_OK)
			break;
		}

		status = dm2u_sysmod(dcb, xcb, mcb->mcb_tbl_id, mcb->mcb_structure, 
		    mcb->mcb_page_type, mcb->mcb_page_size, 
		    mcb->mcb_i_fill, mcb->mcb_l_fill, mcb->mcb_d_fill, mcb->mcb_compressed, mcb->mcb_min_pages, mcb->mcb_max_pages,
		    mcb->mcb_unique, mcb->mcb_merge, truncate,mcb->mcb_kcount, mcb->mcb_key, mcb->mcb_db_lockmode,
		    mcb->mcb_allocation, mcb->mcb_extend, mcb->mcb_tup_info, mcb->mcb_reltups, dberr);
		if (status != E_DB_OK)
		{
		    if (dberr->err_code > E_DM_INTERNAL)
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 
			    0);
			SETDBERR(dberr, 0, E_DM00D1_BAD_SYSCAT_MOD);
		    }
		    break;
		}
		return (E_DB_OK);
	    }
	    else
	    {
		/* If this is iidevices table, make sure modifying 
                ** it to hash unique on two keys. */
		if ( 
	        (t->tcb_rel.reltid.db_tab_base == DM_B_DEVICE_TAB_ID) &&
			(
                      (mcb->mcb_structure != TCB_HASH) || 
                      (mcb->mcb_unique != 0) ||
		      (mcb->mcb_kcount != 2) || 
                      (MEcmp(t->tcb_atts_ptr[DM_1_DEVICE_KEY].attnmstr, 
			(char *)&mcb->mcb_key[0]->key_attr_name,
			t->tcb_atts_ptr[DM_1_DEVICE_KEY].attnmlen) != 0 ||
                      (MEcmp(t->tcb_atts_ptr[DM_2_DEVICE_KEY].attnmstr, 
			(char *)&mcb->mcb_key[1]->key_attr_name,
			t->tcb_atts_ptr[DM_2_DEVICE_KEY].attnmlen) != 0))))	    
		{
		    uleFormat(dberr, E_DM00D1_BAD_SYSCAT_MOD, NULL, ULE_LOG, NULL, 
			    NULL, 0, NULL,
			    &local_error, 0);
		    status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);
		    r = 0;
		    break;
		}
		
	    }
	}

	/*
	** MODIFY TO MERGE:
	**
	** Check for special MODIFY to MERGE operation.  This is used
	** for Btrees to compress the btree index.
	*/
	if ( mcb->mcb_merge )
	{	
	    /* 
	    ** Flush any pages left in buffer cache for this table. 
	    ** This is required for recovery as a non-redo log record 
	    ** immediately follows.
	    */
	    status = dm0p_close_page(t, lk_id, log_id, 
					DM0P_CLCACHE, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Log the modify to merge operation.
	    ** Rollforward does nothing for DM0L_MERGE 
	    ** so we'll log a single MERGE for partitioned
	    ** tables as well.
	    */
	    if (logging)
	    {
		status = dm0l_modify(log_id, dm0l_jnlflag, mcb->mcb_tbl_id, 
		    &table_name, &owner_name, loc_count, &tbl_location, 
		    DM0L_MERGE, relstat, mcb->mcb_min_pages, mcb->mcb_max_pages, 
		    mcb->mcb_i_fill, mcb->mcb_d_fill, mcb->mcb_l_fill, mcb->mcb_reltups, 0, mod_key, key_order, 
		    0, 0, TCB_PG_INVALID, 0, 0, 
		    (DB_TAB_ID *)0, bsf_lsn, 0, 0, (LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED )
	    {
		/* All, or selective partitions? */
		if ( !mcb->mcb_partitions )
		    mcb->mcb_nparts = t->tcb_rel.relnparts;
		    
		for ( i = 0; status == E_DB_OK && i < mcb->mcb_nparts; i++ )
		{
		    if ( mcb->mcb_partitions )
		    {
			/* Selective list, skip "ignore me's" */
			if ( mcb->mcb_partitions[i].flags & DMU_PPCF_IGNORE_ME )
			    continue;
			part_tabid = &mcb->mcb_partitions[i].part_tabid;
		    }
		    else
			/* All partitions */
			part_tabid = &t->tcb_pp_array[i].pp_tabid;
		    
		    /* Open the partition */
		    status = dm2t_open(dcb, 
				part_tabid,
				lk_mode, DM2T_UDIRECT,
				table_access_mode,
				timeout, (i4)20, (i4)0, log_id, 
				TableLockList, (i4)0, (i4)0, 
				mcb->mcb_db_lockmode, 
				tran_id, &timestamp, &pr, 
				(DML_SCB *)0, dberr);

		    if ( status == E_DB_OK )
		    {
			if ( (dcb->dcb_status & DCB_S_ROLLFORWARD) == 0 )
			    pr->rcb_uiptr = &scb->scb_ui_state;

			/*
			** Do the merge on this partition.
			**
			** Note that this should really be done
			** in parallel...
			*/
			status = dm1bmmerge(pr, mcb->mcb_i_fill, mcb->mcb_l_fill, dberr);

			local_status = dm2t_close(pr,
					DM2T_PURGE,
					&local_dberr);
			if ( status == E_DB_OK )
			{
			    status = local_status;
			    *dberr = local_dberr;
			}
		    }
		}
	    }
	    else
	    {
		/* Since we are really going to do the merge, then
		** have rcb point to scb_ui_state so user interrupt
		** can be monitored. */

		if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
		    r->rcb_uiptr = &scb->scb_ui_state;    	

		status = dm1bmmerge(r, mcb->mcb_i_fill, mcb->mcb_l_fill, dberr);
	    }

	    if (status != E_DB_OK)
		break;	

	    /*  Close purge the table. */

	    status = dm2t_close(r, DM2T_PURGE, dberr);
	    r = 0;
	    if (status != E_DB_OK)
		break;
	    /* Close master if we opened it (no need to purge) */
	    if (master_rcb != NULL)
	    {
		status = dm2t_close(master_rcb, 0, dberr);
		if (status != E_DB_OK)
		    break;
		master_rcb = NULL;
	    }
	    return (E_DB_OK);
	}


	/* 
        ** All modifies to a key structure require keys, even
        ** if you are modifying from a keyed structure to a
        ** a key strucutre.
	*/

	if ((mcb->mcb_structure != TCB_HEAP) && (mcb->mcb_key == 0))
	{
	    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	/*
	** We've eliminated all the easy stuff, now we'll
	** eventually end up calling either dm2u_load_table() or
	** dm2u_reorg_table().
	**
	** For both we need to figure out the partition and
	** location entanglements and the asymmetry of
	** the old and new partitions.
	** 
	*/
	old_nparts = new_nparts = 0;

	repartition = (mcb->mcb_new_part_def != (DB_PART_DEF*)NULL);
	if (repartition)
	{
	    old_nparts = t->tcb_rel.relnparts;
	    new_nparts = mcb->mcb_new_part_def->nphys_parts;
	    if (mcb->mcb_new_part_def->ndims == 0)
		new_nparts = 0;
	}
	else if (mcb->mcb_partitions != NULL)
	    old_nparts = new_nparts = mcb->mcb_nparts;
	else if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    old_nparts = new_nparts = t->tcb_rel.relnparts;

	/* Count source and target locations */

	/* If there are partitions, we need to examine each (old)
	** partition to count it as a source.  Some partitions may
	** be skipped if there is a partitions array entry saying ignore.
	** The same partition may be a target (_will_ be a target,
	** unless repartitioning), so we can do the target counts
	** too.
	*/
	oloc_count = 0;
	sources = 0;
	loc_count = 0;
	targets = 0;

	if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED )
	{
	    i4 ocount;

	    /* Table being modified is partitioned.  If there's no
	    ** ppchar array, or if we're repartitioning, all "old"
	    ** partitions are sources.  Otherwise look at the ppchar
	    ** array because some partitions may not be participating.
	    ** We can also use the ppchar array to discover which
	    ** locations sources were on, and which locations targets
	    ** go to, for a repartitioning or relocating modify.
	    ** Remember that the ppchar array has NEW_nparts entries...
	    */

	    for ( partno = 0; 
		  status == E_DB_OK && partno < old_nparts; 
		  partno++ )
	    {
		part = NULL;
		if (mcb->mcb_partitions && partno < new_nparts)
		{
		    part = &mcb->mcb_partitions[partno];
		    /* Skip ignore-me's (never set if repartitioning) */
		    if ( part->flags & DMU_PPCF_IGNORE_ME )
			continue;
		}

		/* If QEU had to get the location info, it passed it... */
		if (part != NULL && part->oloc_count != 0)
		    ocount = part->oloc_count;
		else
		{
		    /* Need partition's TCB to get location info */
		    status = dm2t_fix_tcb(dcb->dcb_id,
				&t->tcb_pp_array[partno].pp_tabid,
				tran_id, TableLockList, log_id,
				DM2T_NOPARTIAL | DM2T_VERIFY,
				dcb,
				&pt,
				dberr);
		    if (status != E_DB_OK)
			break;
		    ocount = pt->tcb_table_io.tbio_loc_count;
		    status = dm2t_unfix_tcb(&pt, DM2T_VERIFY,
				TableLockList, log_id, dberr);
		    if (status != E_DB_OK)
			break;
		}
		oloc_count += ocount;
		sources++;
		/* This source might be a target too */
		if (partno < new_nparts)
		{
		    ++ targets;
		    if (part == NULL || part->loc_array.data_in_size == 0)
		    {
			/* No specific location for this target.
			** Therefore it stays where it is.
			*/
			loc_count += ocount;
		    }
		    else
		    {
			loc_count += part->loc_array.data_in_size /
						sizeof(DB_LOC_NAME);
		    }
		}
	    } /* for */

	    if ( status )
		break;
	    
	    /* Set shortcut for later */
	    all_sources = (sources == old_nparts);

	}
	else 
	{
	    /* Single, non-partitioned source */
	    sources = 1;
	    oloc_count = t->tcb_table_io.tbio_loc_count;
	}

	/*
	** "oloc_count" is now the total number of 
	** locations in all (selected) Sources.
	** If the table is partitioned, "loc_count" is the number
	** of locations in all targets, EXCEPT for any newly created
	** ones (repartitioning).
	*/

	if (new_nparts == 0)
	{
	    /* If result is unpartitioned, there's one target and it's
	    ** either where the user specified, or where it already is.
	    ** ("it" being the master, if unpartitioning.)
	    */
	    targets = 1;
	    loc_count = mcb->mcb_l_count;
	    if ( mcb->mcb_location == NULL )
		loc_count = t->tcb_table_io.tbio_loc_count;
	}
	else if (new_nparts > old_nparts)
	{
	    /* Repartitioning and creating additional partitions.
	    ** (Actually the partitions exist thanks to QEU, but they
	    ** are just hanging out there, and we can't reference them yet.)
	    ** The additional partitions were created in their proper
	    ** locations, and QEU is kind enough to (always!) tell us
	    ** via the partitions array where that proper place was.
	    ** (It could be almost anywhere, and we can't open a TCB
	    ** for the partition yet...)
	    */
	    for (i = old_nparts; i < new_nparts; ++i)
	    {
		++ targets;
		loc_count += mcb->mcb_partitions[i].loc_array.data_in_size /
					sizeof(DB_LOC_NAME);
	    }
	}
	/* else new_nparts is > 0 and <= old_nparts, and we already
	** took care of targets when we did sources.
	*/

	/*
	** Compute the size of the Target TPCB pointer array.
	** 
	** Because the Target partition numbers may differ
	** from their position in mx_tpcb_next
	** (partno 8 may not be TPCB[8]), the only right
	** way to find a record's TPCB by partition number
	** is to extract its *TPCB[partno] from this array.
	** Slots which have no corresponding TPCB will be
	** NULL.
	**
	** Note that this isn't needed for reorg as 
	** reorg always works with paired SPCBs and TPCBs
	** and isn't concerned about partition number.
	*/
	if ( reorg )
	    tppArrayCount = 0;
	else if ( new_nparts > old_nparts )
	    tppArrayCount = new_nparts;
	else if ( old_nparts )
	    tppArrayCount = old_nparts;
	else
	    tppArrayCount = targets;


	/*
	** Now, "loc_count" contains the total number of
	** new locations in all Targets,
	**	"oloc_count" contains the total number of
	** old locations in all Sources.
	*/

	/*
	** If modify on Master, then any specified new
	** locations will be applied to Master's
	** iirelation during dm2u_update_catalogs().
	*/
	if ( mcb->mcb_flagsmask & DMU_MASTER_OP && mcb->mcb_location )
	    Mloc_count = mcb->mcb_l_count;
	else
	    Mloc_count = 0;


	/*
	**  Allocate a MDFY control block.  Allocate space for 
        **  the key atts pointers,
	**  the compare list and a record.  Modify to hash 
        **  includes one extra compare list
	**  entry so that data is sorted by bucket.
	**
	**  The number of columns to sort on is the number of columns
	**  in the table, except for a modify to a unique structure,
	**  in which case it's the number of columns in the key.  The
	**  only exception is for modifying indexes; in this case, all
	**  of the key columns plus the tidp column will be included.
	**  Of course, there's an exception to this, too, which is that
	**  tidp is not included in a hashed index.
	*/


	if (truncate)
	    mcb->mcb_structure = TCB_HEAP;

	/*
	** Figure how much space we need for the base table key list and
	** the sort compare list.
	**
	** If secondary index, all attributes need to be in both these lists.
	** If base table, only kcount attributes are needed, unless the the
	** table in non-unique, in which case all attributes will be put
	** in the base table key list in order to do duplicate checking.
	*/
	data_cmpcontrol_size = 0;
	leaf_cmpcontrol_size = 0;
	index_cmpcontrol_size = 0;
	if (reorg)
	{
	    mcb->mcb_kcount = 0;
	    bkey_count = 0;
	    cmplist_count = 0;
	}
	else
	{
	    if (t->tcb_rel.relstat & TCB_INDEX)
	    {
		bkey_count = t->tcb_rel.relatts + 1;
		cmplist_count = t->tcb_rel.relatts + 1;
	    }
	    else
	    {
		bkey_count = mcb->mcb_kcount + 1;
		if (mcb->mcb_unique)
		    cmplist_count = mcb->mcb_kcount + 1;
		else
		    cmplist_count = t->tcb_rel.relatts + 1;
	    }
	    if (mcb->mcb_structure == TCB_HASH)
		cmplist_count++;

	    /* Add another for target partition number, */
	    cmplist_count++;
	    /* ...source partition number, */
	    cmplist_count++;
	    /* ...and source TID */
	    cmplist_count++;

	    /* Guess at (upper limit for) NEW compression control sizes. */
	    data_cmpcontrol_size = dm1c_cmpcontrol_size(
			mcb->mcb_compressed, t->tcb_data_rac.att_count, t->tcb_rel.relversion);
	    if (mcb->mcb_index_compressed)
	    {
		if (mcb->mcb_structure == TCB_RTREE)
		{
		    /* Toss in one more for hilbert */
		    index_cmpcontrol_size = dm1c_cmpcontrol_size(
			TCB_C_STD_OLD, bkey_count+1, t->tcb_rel.relversion);
		}
		else if (mcb->mcb_structure == TCB_BTREE)
		{
		    /* Index entry is keys perhaps with tidp.
		    ** Leaf entry is bkey_count (whole row if index,
		    ** keys if not.  ***NEEDS FIXED FOR CLUSTERED).
		    */
		    index_cmpcontrol_size = dm1c_cmpcontrol_size(
			TCB_C_STD_OLD, mcb->mcb_kcount+1, t->tcb_rel.relversion);
		    leaf_cmpcontrol_size = dm1c_cmpcontrol_size(
			TCB_C_STD_OLD, bkey_count, t->tcb_rel.relversion);
		}
	    }
	    data_cmpcontrol_size = DB_ALIGN_MACRO(data_cmpcontrol_size);
	    index_cmpcontrol_size = DB_ALIGN_MACRO(index_cmpcontrol_size);
	    leaf_cmpcontrol_size = DB_ALIGN_MACRO(leaf_cmpcontrol_size);
	}

	/* Reorg and Truncate don't need buffers */
	if ( reorg || truncate )
	    buf_size = 0;
	else
	{
	    buf_size = DB_ALIGNTO_MACRO(t->tcb_rel.relwid +
			    row_expansion, sizeof(i4));
	    buf_size += sizeof(i4);	/* For hash value if we need if */
	    buf_size += sizeof(DM2U_RHEADER);  /* Include extra header */
	    buf_size += sizeof(u_i2);	/* For partition number */
	    buf_size += sizeof(DM_TID8);	
	    buf_size = DB_ALIGN_MACRO(buf_size);
	}
	
	size_align = ((t->tcb_rel.relatts + 1) * sizeof(i4));
	size_align = DB_ALIGN_MACRO(size_align);

	attnmsz = (mcb->mcb_kcount + 1) * sizeof(DB_ATT_STR);
	attnmsz = DB_ALIGN_MACRO(attnmsz);

	/* Allocate zero-filled memory */
	status = dm0m_allocate(sizeof(DM2U_MXCB) +
	    ((mcb->mcb_kcount + 1) * sizeof(DB_ATTS)) + 
	    ((bkey_count + mcb->mcb_kcount + 1) * sizeof(DB_ATTS *)) +
	    sizeof(ALIGN_RESTRICT) +
	    size_align +
	    data_cmpcontrol_size +
	    leaf_cmpcontrol_size +
	    index_cmpcontrol_size +
	    attnmsz +
	    (cmplist_count * sizeof(DB_CMP_LIST)) + 
	    sizeof(ALIGN_RESTRICT) +
	    (sources * (sizeof(DM2U_SPCB) + 1 * buf_size)) +
	    sizeof(ALIGN_RESTRICT) +
	    (tppArrayCount * sizeof(DM2U_TPCB*)) +
	    sizeof(ALIGN_RESTRICT) +
	    (targets * (sizeof(DM2U_TPCB) + 2 * buf_size)) +
	    buf_size +
	    (Mloc_count * sizeof(DMP_LOCATION)) +
	    (oloc_count * sizeof(DMP_LOCATION)) +
	    (loc_count  * (sizeof(DMP_LOCATION) +
			   sizeof(DB_LOC_NAME))),
	    DM0M_ZERO, (i4)MXCB_CB, (i4)MXCB_ASCII_ID, (char *)xcb,
	    (DM_OBJECT **)&m, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Initialize pointers to dynamic portions of the 
        **  MDFY control block. */

	m->mx_operation = MX_MODIFY;
	m->mx_b_key_atts = (DB_ATTS**)((char *)m + sizeof(DM2U_MXCB));
	m->mx_i_key_atts = (DB_ATTS**)((char *)m->mx_b_key_atts + 
	    (bkey_count) * sizeof(DB_ATTS *));
	m->mx_atts_ptr = (DB_ATTS *)((char *)m->mx_i_key_atts +
	    (mcb->mcb_kcount + 1 ) * sizeof(DB_ATTS *));
	m->mx_cmp_list = (DB_CMP_LIST*)((char *)m->mx_atts_ptr + 
                             (mcb->mcb_kcount + 1) * sizeof(DB_ATTS));
	m->mx_cmp_list = (DB_CMP_LIST *)
			ME_ALIGN_MACRO(m->mx_cmp_list, sizeof(ALIGN_RESTRICT));
	m->mx_att_map = (i4*)((char *)m->mx_cmp_list +
	    cmplist_count * sizeof(DB_CMP_LIST));
	p = (char *) m->mx_att_map +
			  (t->tcb_rel.relatts + 1) * sizeof(i4);
	p = ME_ALIGN_MACRO(p, sizeof(ALIGN_RESTRICT));
	if (data_cmpcontrol_size > 0)
	{
	    m->mx_data_rac.cmp_control = (PTR) p;
	    m->mx_data_rac.control_count = data_cmpcontrol_size;
	    p += data_cmpcontrol_size;
	}
	if (leaf_cmpcontrol_size > 0)
	{
	    m->mx_leaf_rac.cmp_control = (PTR) p;
	    m->mx_leaf_rac.control_count = leaf_cmpcontrol_size;
	    p += leaf_cmpcontrol_size;
	}
	if (index_cmpcontrol_size > 0)
	{
	    m->mx_index_rac.cmp_control = (PTR) p;
	    m->mx_index_rac.control_count = index_cmpcontrol_size;
	    p += index_cmpcontrol_size;
	}

	nextattname = p;
	p += attnmsz;
	for (curatt = m->mx_atts_ptr, i = 0; i < (mcb->mcb_kcount + 1); 
			i++, curatt++)
	{
	    curatt->attnmstr = nextattname;
	    nextattname += sizeof(DB_ATT_STR);
	}

	/* cmpcontrol areas are size aligned to align-restrict, so p is
	** still aligned...
	*/

	/* Initialize array pointers */
	sp = (DM2U_SPCB *) p;

	tppArray = (DM2U_TPCB**)((char*)sp + sources * (sizeof(DM2U_SPCB)));
	tppArray = (DM2U_TPCB**)ME_ALIGN_MACRO(tppArray,
					       sizeof(ALIGN_RESTRICT));
	tp = (DM2U_TPCB*)((char*)tppArray + 
			    tppArrayCount * sizeof(DM2U_TPCB*));
	/* Ensure proper alignment of TPCBs */
	tp = (DM2U_TPCB*)ME_ALIGN_MACRO(tp, sizeof(ALIGN_RESTRICT));

	bufArray = (char*)tp + targets * sizeof(DM2U_TPCB);
	/* Ensure proper alignment of holding buffers (i4's) */
	bufArray = (char *) ME_ALIGN_MACRO(bufArray, sizeof(ALIGN_RESTRICT));
	MlocArray = (DMP_LOCATION*)((char*)bufArray 
					   + (sources * 1 * buf_size)
					   + (targets * 2 * buf_size)
					   + buf_size);
	olocArray = (DMP_LOCATION*)((char*)MlocArray + Mloc_count * 
					sizeof(DMP_LOCATION));
	nlocArray = (DMP_LOCATION*)((char*)olocArray + oloc_count *
					sizeof(DMP_LOCATION));
	locnArray = (DB_LOC_NAME*)((char*)nlocArray + loc_count *
					sizeof(DMP_LOCATION));
	orig_locnArray = locnArray;

	/* Set up the source(s) and target(s) */
	m->mx_spcb_count = sources;
	m->mx_tpcb_count = targets;
	m->mx_spcb_next = (DM2U_SPCB*)NULL;
	m->mx_tpcb_next = (DM2U_TPCB*)NULL;
	m->mx_tpcb_ptrs = tppArray;

	/* ...and partitioning stuff, if any */
	m->mx_part_def = mcb->mcb_new_part_def;
	m->mx_phypart = (PTR)mcb->mcb_partitions;

	/* Init a bunch of stuff that we may soon need.
	** Modify data row is identical to base table's, so just use atts
	** and pointers from base table.  Compression might NOT be the same.
	*/
	m->mx_structure = mcb->mcb_structure;
	m->mx_data_rac.att_ptrs = t->tcb_data_rac.att_ptrs;
	m->mx_data_rac.att_count = t->tcb_data_rac.att_count;
	m->mx_data_rac.compression_type = mcb->mcb_compressed;
	m->mx_index_comp = mcb->mcb_index_compressed;
	if (m->mx_index_comp)
	{
	    /* No syntax for key compression type yet, always use OLD STANDARD */
	    m->mx_leaf_rac.compression_type = TCB_C_STD_OLD;
	    m->mx_index_rac.compression_type = TCB_C_STD_OLD;
	}
	m->mx_unique = mcb->mcb_unique;
	m->mx_dmveredo = FALSE;
	m->mx_truncate = truncate;
        m->mx_duplicates = duplicates;
        m->mx_reorg = reorg;
	m->mx_dfill = mcb->mcb_d_fill;
	m->mx_ifill = mcb->mcb_i_fill;
	m->mx_lfill = mcb->mcb_l_fill;
	m->mx_maxpages = mcb->mcb_max_pages;
	m->mx_minpages = mcb->mcb_min_pages;
	m->mx_ab_count = 0;
	m->mx_ai_count = 0;
	m->mx_width = t->tcb_rel.relwid;
	m->mx_datawidth = t->tcb_rel.reldatawid;
	m->mx_src_width = t->tcb_rel.relwid;
	m->mx_kwidth = 0;
	m->mx_rcb = r;
	m->mx_xcb = xcb;
	m->mx_lk_id = lk_id;
	m->mx_log_id = log_id;
	m->mx_dcb = dcb;
	m->mx_table_id.db_tab_base = mcb->mcb_tbl_id->db_tab_base;
	m->mx_table_id.db_tab_index= mcb->mcb_tbl_id->db_tab_index;
	m->mx_allocation = mcb->mcb_allocation;
	m->mx_extend = mcb->mcb_extend;
	m->mx_page_size = mcb->mcb_page_size;
	m->mx_page_type = mcb->mcb_page_type;
	m->mx_inmemory = 0;
	m->mx_readonly = 0;
	m->mx_concurrent_idx = FALSE;
	m->mx_signal_rec = (DM2U_RHEADER *) bufArray;
	bufArray += buf_size;
	m->mx_flags = 0;

	m->mx_tbl_lk_mode = lk_mode;
	m->mx_tbl_access_mode = table_access_mode;
	m->mx_timeout = timeout;
	m->mx_db_lk_mode = mcb->mcb_db_lockmode;
	m->mx_tranid = tran_id;
	m->mx_clustered = mcb->mcb_clustered;

	/*
	** Make new locations for Master, if needed, 
	** from "location".
	**
	** These are used by dm2u_update_catalogs() to
	** create default location(s) for the Master
	** table, nowhere else.
	*/
	if ( Mloc_count )
	{
	    m->mx_Mloc_count = Mloc_count;
	    m->mx_Moloc_count = t->tcb_table_io.tbio_loc_count;
	    m->mx_Mlocations = MlocArray;

	    for ( k = 0; k < Mloc_count; k++, MlocArray++ )
	    {
		if ( status = AddNewLocation(m, 
					&mcb->mcb_location[k], 
					MlocArray, k, dberr) )
		{
		    break;
		}
	    }
	    if ( status )
		break;
	}
	else
	{
	    m->mx_Mloc_count = 0;
	    m->mx_Moloc_count = 0;
	    m->mx_Mlocations = NULL;
	}

	/* Initialize the sources */
	for ( i = 0, j = 0; i < sources; i++, j++ )
	{
	    /* Link SPCB to MXCB list of sources */
	    if ( i )
		m->mx_spcb_next[i-1].spcb_next = sp;
	    else
		m->mx_spcb_next = sp;
	    sp->spcb_next = (DM2U_SPCB*)NULL;

	    sp->spcb_mxcb = m;

	    /*
	    ** Master's Table, or non-partitioned
	    ** single source, has already been
	    ** opened above ("r").
	    **
	    ** The other source partitions won't be 
	    ** opened until needed.
	    */
	    if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED )
	    {
		sp->spcb_rcb = (DMP_RCB*)NULL;

		if ( all_sources )
		    partno = i;
		else
		{
		    /* If partition is ignored, skip to next selected one */
		    while ( mcb->mcb_partitions[j].flags & DMU_PPCF_IGNORE_ME )
			j++;
		    partno = j;
		}
		/* Need partition's TCB to get location info.
		*/
		status = dm2t_fix_tcb(dcb->dcb_id,
			&t->tcb_pp_array[partno].pp_tabid,
			tran_id, TableLockList, log_id,
			DM2T_NOPARTIAL | DM2T_VERIFY,
			dcb,
			&pt,
			dberr);
		if (status != E_DB_OK)
		    break;

		/* for online modify, setup source partition's input rcb */
		if (mcb->mcb_omcb)
		{
		    DM2U_OSRC_CB	*osrc;
		    bool		found = FALSE;

		    for ( osrc = mcb->mcb_omcb->omcb_osrc_next; 
			  osrc && !found; 
			  osrc = osrc->osrc_next)
		    {
			if (partno == osrc->osrc_partno)
			{
			    sp->spcb_input_rcb = osrc->osrc_rnl_rcb;
			    found = TRUE;
			}
		    }
		    if (!found)
			status = E_DB_ERROR;
		}
	    }
	    else
	    {
		pt = t;
		sp->spcb_rcb = r;

		if (mcb->mcb_omcb)
		    sp->spcb_input_rcb = mcb->mcb_omcb->omcb_osrc_next->osrc_rnl_rcb;
	    }

	    STRUCT_ASSIGN_MACRO(pt->tcb_rel.reltid,
				     sp->spcb_tabid);

	    sp->spcb_partno = pt->tcb_partno;
	    sp->spcb_positioned = FALSE;
	    sp->spcb_log_id = log_id;
	    sp->spcb_lk_id = lk_id;

	    /* Add Source locations to Source locations array */

	    sp->spcb_locations = olocArray;
	    sp->spcb_loc_count = pt->tcb_table_io.tbio_loc_count;

	    for ( k = 0; k < sp->spcb_loc_count; k++, olocArray++ )
	    {
		STRUCT_ASSIGN_MACRO(pt->tcb_table_io.tbio_location_array[k],
				    *olocArray);

		/* Clear FCB stuff always */
		olocArray->loc_fcb = (DMP_FCB*)NULL;
		olocArray->loc_status &= ~LOC_OPEN;
	    }

	    /* If we did a fix above, unfix this partition's TCB */
	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		status = dm2t_unfix_tcb(&pt, DM2T_VERIFY,
				TableLockList, log_id, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /* Set pointer to this Source's record buffer */
	    sp->spcb_record = (DM2U_RHEADER *) bufArray;
	    bufArray += buf_size;

	    sp++;
	}

	if ( status )
	    break;

	/* Initialize the target(s) */

	/* If we're not rollforward, grab one name generator ID number
	** per target.  (If we are rollforward then the name generator
	** ID was logged and we just use that one -- set above.)
	*/
	if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
	{
	    name_gen = CSadjust_counter((i4 *) &dmf_svcb->svcb_tfname_gen,
				targets);
	    name_gen = name_gen - targets + 1;	/* Back off to first one */
	}
	base_name_gen = name_gen;

	/* Targets match sources until one runs out */
	sp = m->mx_spcb_next;

	for ( i = 0; status == E_DB_OK && i < targets; i++ )
	{
	    /* Link TPCB to MXCB list of targets */
	    if ( i )
		m->mx_tpcb_next[i-1].tpcb_next = tp;
	    else
		m->mx_tpcb_next = tp;
	    tp->tpcb_next = (DM2U_TPCB*)NULL;

	    tp->tpcb_mxcb = m;
	    tp->tpcb_buildrcb = (DMP_RCB*)NULL;
	    tp->tpcb_log_id = log_id;
	    tp->tpcb_lk_id = lk_id;
	    tp->tpcb_srt = (DMS_SRT_CB*)NULL;
	    tp->tpcb_srt_wrk = NULL;
	    tp->tpcb_newtup_cnt = 0;
	    tp->tpcb_state = 0;
	    tp->tpcb_sid = (CS_SID)NULL;
	    
	    /* Set pointers to location arrays */
	    tp->tpcb_locations = nlocArray;
	    tp->tpcb_loc_list = locnArray;
	    tp->tpcb_loc_count = 0;
	    tp->tpcb_new_locations = FALSE;

            tp->tpcb_name_id = name_id;
            tp->tpcb_name_gen = name_gen;
            ++ name_gen;

	    /* No more source or target partitions? */
	    if (i >= old_nparts || i >= new_nparts)
		sp = NULL;

	    /* No name copying needed, yet */
	    PartLoc = NULL;

	    if (sp != NULL)
	    {
		/* This target is the same as this source except maybe
		** for its location.  (and both are partitions)
		*/
		STRUCT_ASSIGN_MACRO(sp->spcb_tabid, tp->tpcb_tabid);
		tp->tpcb_oloc_count = sp->spcb_loc_count;
		tp->tpcb_partno = sp->spcb_partno;
		if (mcb->mcb_partitions != NULL)
		{
		    /* Find this target in the partitions array to see
		    ** if it has a changed location from its source.
		    */
		    part = &mcb->mcb_partitions[tp->tpcb_partno];
		    if (part->loc_array.data_in_size > 0)
		    {
			tp->tpcb_loc_count = part->loc_array.data_in_size
					/ sizeof(DB_LOC_NAME);
			PartLoc = (DB_LOC_NAME *) part->loc_array.data_address;
			tp->tpcb_new_locations = TRUE;
		    }
		}

		/* If we don't have a target location yet, it's the same
		** as the source.
		*/
		if (tp->tpcb_loc_count == 0)
		{
		    tp->tpcb_loc_count = sp->spcb_loc_count;
		    for ( k = 0; k < tp->tpcb_loc_count; k++, nlocArray++ )
			MEcopy(&sp->spcb_locations[k], 
				sizeof(DMP_LOCATION),
				nlocArray);
		}

		/* Move to next source */
		sp = sp->spcb_next;

	    }
	    else if (i < new_nparts)
	    {
		/* This target no longer matched any source partition.
		** But it's a new partition, in which case we'll find
		** its info in the partitions array.
		*/
		part = &mcb->mcb_partitions[i];
		STRUCT_ASSIGN_MACRO(part->part_tabid, tp->tpcb_tabid);
		tp->tpcb_partno = i;
		tp->tpcb_loc_count = part->loc_array.data_in_size /
					sizeof(DB_LOC_NAME);
		tp->tpcb_oloc_count = tp->tpcb_loc_count;
		/* New partition is already in the right place, we don't
		** set "new locations".
		*/
		PartLoc = (DB_LOC_NAME *) part->loc_array.data_address;
	    }
	    else
	    {
		/* This target (	THE target) isn't a partition at all.  It's
		** an unpartitioned table (perhaps resulting from an
		** unpartition).
		*/
		STRUCT_ASSIGN_MACRO(t->tcb_rel.reltid, tp->tpcb_tabid);
		tp->tpcb_partno = 0;
		tp->tpcb_oloc_count = t->tcb_table_io.tbio_loc_count;
		if (mcb->mcb_location != NULL)
		{
		    tp->tpcb_loc_count = mcb->mcb_l_count;
		    tp->tpcb_new_locations = TRUE;
		    PartLoc = mcb->mcb_location;
		}
		else
		{
		    tp->tpcb_loc_count = tp->tpcb_oloc_count;
		    for ( k = 0; k < tp->tpcb_loc_count; k++, nlocArray++ )
		    {
			if ( status = AddOldTargetLocation(m,
				    &t->tcb_table_io.tbio_location_array[k],
				    nlocArray, dberr) )
			    break;
		    }
		    if ( status )
			break;
		}
	    }

	    /* At this point, if partLoc is set to something, it's an
	    ** array of tpcb_loc_count DB_LOC_NAME's that have to be
	    ** transmogrified into DMP_LOCATION's.  (These are not
	    ** necessarily "new" locations, mind.)
	    */
	    if (PartLoc != NULL)
	    {
		for (k = 0; k < tp->tpcb_loc_count; ++k, ++nlocArray)
		{
		    if ( status = AddNewLocation(m, 
					    &PartLoc[k], 
					    nlocArray, k, dberr) )
		    {
			break;
		    }
		}
	    }

	    if ( status == E_DB_OK )
	    {
		/*
		** Build location name list from the location array of the
		** target modify table.  This name list is used to build
		** the temporary build table below and to log the target
		** table locations.
		*/
		for ( k = 0; k < tp->tpcb_loc_count; k++, locnArray++ )
		    STRUCT_ASSIGN_MACRO(tp->tpcb_locations[k].loc_name,
		    *locnArray);

		/* Set pointers to this target's record buffers */
		tp->tpcb_crecord = bufArray + 0 * buf_size;
		/* ****FIXME ONLY PARENTS NEED THE srecord BUFFER */
		tp->tpcb_srecord = (DM2U_RHEADER *) (bufArray + 1 * buf_size);
		/* Only CREATE INDEX needs the index buffer */
		tp->tpcb_irecord = NULL;

		/* Set pointer to this TPCB by partno */
		if ( !reorg )
		    m->mx_tpcb_ptrs[tp->tpcb_partno] = tp;

		bufArray += 2 * buf_size;
		tp++;
	    }
	}

	if ( status )
	    break;


	/*
	** Set new cache priority if specified, else use the old one.
	*/
	if (mcb->mcb_mod_options2 & DM2U_2_TABLE_PRIORITY)
	    m->mx_tbl_pri = mcb->mcb_tbl_pri;
	else
	    m->mx_tbl_pri = t->tcb_rel.reltcpri;

	/*
	** Init estimated row count. For SYSMOD iirtemp builds this value is
	** passed in by the caller. Otherwise use the parent table row count.
	*/
	if (mcb->mcb_reltups == 0)
	{
	    i4 toss_relpages;

	    /* Mutex the TCB while reading the RCBs */
	    dm0s_mlock(&t->tcb_mutex);

	    dm2t_current_counts(t, &mcb->mcb_reltups, &toss_relpages);

	    dm0s_munlock(&t->tcb_mutex);
	}
	m->mx_reltups = mcb->mcb_reltups;

	if (mcb->mcb_temporary != 0)
	{
	    if (t->tcb_table_io.tbio_location_array[0].loc_fcb)
		if ((t->tcb_table_io.tbio_location_array[0].loc_fcb->fcb_state &
							FCB_DEFERRED_IO) != 0)
		    m->mx_inmemory = 1;
	}
	/* buildrcb normally == rcb, will get changed if this is an
	** in-memory temporary table modify.
	*/
	m->mx_buildrcb = m->mx_rcb;

	/*
	** For hash tables, calculate the number of hash buckets to allocate.
	** For partitioning, we assume each partition gets 1/#partitions
	** of the total.  If the partitioning isn't changing, it would
	** be considerably better to use the current #rows per partition.
	*/
	if (m->mx_structure == TCB_HASH)
	{
	    if (dcb->dcb_status & DCB_S_ROLLFORWARD)
		buckets = mcb->mcb_rfp_entry->rfp_hash_buckets;
	    else
	    {
		buckets = dm2u_calc_hash_buckets(m->mx_page_type, 
				m->mx_page_size,
				m->mx_dfill, m->mx_width,
				m->mx_reltups / m->mx_tpcb_count,
				m->mx_minpages, m->mx_maxpages);
	    }
	    m->mx_buckets = buckets;
	}
	if (mcb->mcb_mod_options2 & DM2U_2_TBL_RECOVERY_DEFAULT) 
	    mcb->mcb_relstat2 |= TCB2_TBL_RECOVERY_DISALLOWED; 
	m->mx_new_relstat2 = mcb->mcb_relstat2;

	/*
	** Certain characteristics, not set by the user, must persist
	** across modifies.
	*/

	m->mx_new_relstat2 |= t->tcb_rel.relstat2 &
		( TCB_SUPPORTS_CONSTRAINT |
		  TCB_SYSTEM_GENERATED |
		  TCB_NOT_DROPPABLE |
		  TCB_NOT_UNIQUE |
		  TCB2_PARTITION |
		  TCB2_GLOBAL_INDEX
		);

	/*
	** Set the target TID size, TID8s for global indexes, 
	** TID for all others.
	*/
	if ( m->mx_new_relstat2 & TCB2_GLOBAL_INDEX )
	    m->mx_tidsize = sizeof(DM_TID8);
	else
	    m->mx_tidsize = sizeof(DM_TID);

	adf_cb = r->rcb_adf_cb;

	/* 
	** Now that we know we are really going to do the
        ** modify, set the rcb_uiptr to point to the scb_ui_state
        ** value so it can check for user interrupts.
	** Do not set it if rollingforward. 
        */

	if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
	    r->rcb_uiptr = &scb->scb_ui_state;

	if (reorg == 0)
	{	
	    /*  
	    ** Check key descriptions.  Build the atts pointer and 
	    ** the compare list. 
	    */
	    next_cmp = 0;

	    /*
	    ** First compare is on target partition number, which will
	    ** be placed at the end of the record (beyond the
	    ** hash bucket).
	    **
	    ** The sort order will be:
	    **		target partition
	    **		[hash bucket]
	    **		[structure keys]
	    **		source TID8 (partno,TID)
	    **
	    ** Grouping by source partition and TID assures that the
	    ** rows will be presented to the Targets deterministically.
	    */
	    if ( m->mx_structure == TCB_HASH )
		m->mx_cmp_list[0].cmp_offset = t->tcb_rel.relwid + sizeof(i4);
	    else
		m->mx_cmp_list[0].cmp_offset = t->tcb_rel.relwid;
	    m->mx_cmp_list[0].cmp_type = DB_INT_TYPE;
	    m->mx_cmp_list[0].cmp_precision = 0;
	    m->mx_cmp_list[0].cmp_length = sizeof(u_i2);
	    m->mx_cmp_list[0].cmp_direction = 0;
	    m->mx_cmp_list[0].cmp_collID = -1;
	    next_cmp++;

	    if (m->mx_structure == TCB_HASH)
	    {
		m->mx_cmp_list[next_cmp].cmp_offset = t->tcb_rel.relwid;
		m->mx_cmp_list[next_cmp].cmp_type = DB_INT_TYPE;
		m->mx_cmp_list[next_cmp].cmp_precision = 0;
		m->mx_cmp_list[next_cmp].cmp_length = sizeof(i4);
		m->mx_cmp_list[next_cmp].cmp_direction = 0;
		m->mx_cmp_list[next_cmp].cmp_collID = -1;
		next_cmp++;
	    }
	    MEfill(sizeof(key_map), '\0', (PTR) key_map);
	    MEfill((sizeof(*m->mx_att_map) * (t->tcb_rel.relatts + 1)), 
			'\0', (PTR) m->mx_att_map);
	}

	if ((reorg == 0) && (t->tcb_rel.relstat & TCB_INDEX))
	{
	    /*
	    ** It is illegal to modify an index to a unique structure if it is
	    ** not already unique.  This is because there's no good way to
	    ** eliminate the duplicates in the base table if the index
	    ** changes from non-unique to unique.
	    */
	    if (mcb->mcb_unique && !(t->tcb_rel.relstat & TCB_UNIQUE))
	    {
		SETDBERR(dberr, 0, E_DM0111_MOD_IDX_UNIQUE);
		break;
	    }

	    /* Use the BTREE index compexpand accessor if index compression
	    ** is in use to calculate the possible expansion for checking if
	    ** key, after compression, is too large. 
	    */

	    key_expansion = 0;
	    if ( mcb->mcb_index_compressed )
		key_expansion = dm1c_compexpand(TCB_C_STD_OLD,
			t->tcb_data_rac.att_ptrs, t->tcb_data_rac.att_count); 

	    for (i = 0; i < t->tcb_rel.relatts - 1; i++)
	    {
		DB_ATT_NAME tmpattnm;

		/*  Index keys are always a prefix of the index attributes. */ 

		if (i < mcb->mcb_kcount)
		{
		    MEmove(t->tcb_atts_ptr[i + 1].attnmlen,
			t->tcb_atts_ptr[i + 1].attnmstr, ' ',
			DB_ATT_MAXNAME, tmpattnm.db_att_name);
		    if (MEcmp(tmpattnm.db_att_name, 
			(char *)&mcb->mcb_key[i]->key_attr_name,
			sizeof(mcb->mcb_key[i]->key_attr_name)))
		    {
			break;
		    }

		    status = adi_dtinfo(
			adf_cb,
			(DB_DT_ID) t->tcb_atts_ptr[i + 1].type,
			&dt_bits);

		    if (status)
		    {
			SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
			break;
		    }
		    if (dt_bits & (AD_NOSORT | AD_NOKEY))
		    {
			SETDBERR(dberr, 0, E_DM0066_BAD_KEY_TYPE);
			break;
		    }

		    /*	Build map from attribute number to key number. */

		    m->mx_att_map[i + 1] = i + 1;

		    /* mod_key[0] starts at 1 to allow RFP to work */

		    mod_key[i] = i + 1;

		    /*
		    **	Build pointer for next key to corresponding attribute.
		    */

		    m->mx_i_key_atts[m->mx_ai_count] = &t->tcb_atts_ptr[i + 1];

		    /*	Build sort comparision entry for next key. */

		    m->mx_cmp_list[next_cmp].cmp_offset = 
			t->tcb_atts_ptr[i + 1].offset;
		    m->mx_cmp_list[next_cmp].cmp_type = 
                                   t->tcb_atts_ptr[i + 1].type;
		    m->mx_cmp_list[next_cmp].cmp_precision = 
			t->tcb_atts_ptr[i + 1].precision;
		    m->mx_cmp_list[next_cmp].cmp_length = 
			t->tcb_atts_ptr[i + 1].length;
		    m->mx_cmp_list[next_cmp].cmp_direction = 0;
		    m->mx_cmp_list[next_cmp].cmp_collID =
			t->tcb_atts_ptr[i + 1].collID;
		    next_cmp++;
		    m->mx_kwidth += t->tcb_atts_ptr[i + 1].length;
		    OnlyKeyLen += t->tcb_atts_ptr[i + 1].length;
		    m->mx_ai_count++;
		}
		else if (m->mx_structure == TCB_BTREE)
		    m->mx_kwidth += t->tcb_atts_ptr[i + 1].length;
	        m->mx_b_key_atts[m->mx_ab_count] = &t->tcb_atts_ptr[i + 1];
		m->mx_ab_count++;
	    }

	    /*
	    **	Check that all given atributes were processed and that the
	    **  index key was fully specified except for the possibly
	    **  missing tidp key field.  The tidp key would be missing on
	    **	HASH or old ISAM and BTREE indices.
	    */

	    if (i < mcb->mcb_kcount || (mcb->mcb_kcount != t->tcb_keys - 
		(t->tcb_atts_ptr[t->tcb_rel.relatts].key == 0 ? 0 : 1)))
	    {
		SETDBERR(dberr, 0, E_DM001C_BAD_KEY_SEQUENCE);
		break;
	    }

	    /*
	    **  HASH indices never include TIDP as a key.
	    **  Non-unique BTREE and ISAM indices include TIDP as a key field.
	    **
	    **  Unique indices do not include TIDP in key,
	    **  nor do indexes on Clustered Tables.
	    */
	    if (m->mx_structure != TCB_HASH)
	    {
		if ( !mcb->mcb_unique && 
		    !(t->tcb_parent_tcb_ptr->tcb_rel.relstat & TCB_CLUSTERED) )
		{
		    /*	Build pointer for TIDP key, so that tidp can be
		    ** included in the sort ordering even though it's not
		    ** technically part of the key.  This is nonessential
		    ** but makes the index work better.
		    */
		    m->mx_i_key_atts[m->mx_ai_count] = &m->mx_atts_ptr[0];
		    STRUCT_ASSIGN_MACRO(t->tcb_atts_ptr[i + 1], m->mx_atts_ptr[0]);
		    m->mx_atts_ptr[0].key_offset = m->mx_kwidth;
		    m->mx_ai_count++;
		    m->mx_b_key_atts[m->mx_ab_count] = &t->tcb_atts_ptr[i + 1];
		    m->mx_ab_count++;
		    m->mx_kwidth += t->tcb_atts_ptr[i + 1].length;
		    OnlyKeyLen += t->tcb_atts_ptr[i + 1].length;

		    /*	Build sort comparision entry for TIDP. */

		    m->mx_cmp_list[next_cmp].cmp_offset = 
			t->tcb_atts_ptr[i + 1].offset;
		    m->mx_cmp_list[next_cmp].cmp_type = t->tcb_atts_ptr[i + 1].type;
		    m->mx_cmp_list[next_cmp].cmp_precision = 
			t->tcb_atts_ptr[i + 1].precision;
		    m->mx_cmp_list[next_cmp].cmp_length = 
			t->tcb_atts_ptr[i + 1].length;
		    m->mx_cmp_list[next_cmp].cmp_direction = 0;
		    m->mx_cmp_list[next_cmp].cmp_collID =
			t->tcb_atts_ptr[i + 1].collID;
		    next_cmp++;

		    /* Build map from attribute number to key number for TIDP */
		    m->mx_att_map[i + 1] = m->mx_ai_count;
		    mod_key[i] = m->mx_ai_count;
		}
		else if (m->mx_structure == TCB_BTREE)
		{
		    /*
		    **  Btree secondary indices store full data records, so the 
		    **  "key width" for a Btree secondary includes all columns
		    **  (including non-key attributes and tidp)
		    */
		    m->mx_kwidth += m->mx_tidsize;
		}
	    }
	    /* Point leaf rowaccessor at the entire secondary index row;
	    ** the index rowaccessor at the keys.  Btree will use both of
	    ** these, although it will rebuild the index attribute list if
	    ** there are nonkey columns in the table (index).
	    */
	    if (m->mx_structure == TCB_BTREE)
	    {
		m->mx_leaf_rac.att_ptrs = m->mx_data_rac.att_ptrs;
		m->mx_leaf_rac.att_count = m->mx_data_rac.att_count;
		/* Actually we could probably get away with letting
		** btree bbegin do this, but I'm trying to get rowaccess
		** setup moved higher, not lower... (kschendel)
		*/
		m->mx_index_rac.att_ptrs = m->mx_i_key_atts;
		m->mx_index_rac.att_count = m->mx_ai_count;
	    }
	    else if (m->mx_structure == TCB_RTREE)
	    {
		/* FIXME CAN'T MODIFY RTREE (parser doesn't let you and
		** there are holes in the dmu data structuring.)
		** I suspect we'd want to set the index rac to the data rac.
		*/
		m->mx_index_rac.att_ptrs = m->mx_data_rac.att_ptrs;
		m->mx_index_rac.att_count = m->mx_data_rac.att_count;
	    }
	}
	else if (reorg == 0)
	{
	    /* Not a secondary index -- primary structure on base table. */

	    m->mx_ai_count = mcb->mcb_kcount;
	    m->mx_ab_count = mcb->mcb_kcount;

	    for (i = 0; i < mcb->mcb_kcount; i++)
	    {
		DB_ATT_NAME tmpattnm;

		for (j = 1; j <= t->tcb_rel.relatts; j++)
		{
		    MEmove(t->tcb_atts_ptr[j].attnmlen,
			t->tcb_atts_ptr[j].attnmstr, ' ',
			DB_ATT_MAXNAME, tmpattnm.db_att_name);

		    if ((MEcmp(tmpattnm.db_att_name, 
			 (char *)&mcb->mcb_key[i]->key_attr_name,
			 sizeof(mcb->mcb_key[i]->key_attr_name)) == 0) && 
			(t->tcb_atts_ptr[j].ver_altcol == 0) &&
			(t->tcb_atts_ptr[j].ver_added >=
                            t->tcb_atts_ptr[j].ver_dropped))
		    {
			/*   Same attribute can't be specified twice. */
			/*   unless it is altered 		      */
			/*   or it is dropped and new attr with same name is added */

			if ((key_map[j >> 5] & (1 << (j & 0x1f))))
			{
			    SETDBERR(dberr, 0, E_DM001C_BAD_KEY_SEQUENCE);
			    break;
			}

			/* Only encrypted hash secondary indices are allowed
			** for encrypted columns.
			*/
			if (t->tcb_atts_ptr[j].encflags & ATT_ENCRYPT)
			{
			    SETDBERR(dberr, 0, E_DM0066_BAD_KEY_TYPE);
			    break;
			}

			status = adi_dtinfo(
			    adf_cb,
			    (DB_DT_ID) t->tcb_atts_ptr[j].type,
			    &dt_bits);

			if (status)
			{
			    SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
			    break;
			}
			if (dt_bits & (AD_NOSORT | AD_NOKEY))
			{
			    SETDBERR(dberr, 0, E_DM0066_BAD_KEY_TYPE);
			    break;
			}
			key_map[j >> 5] |= 1 << (j & 0x1f);

			/*
			** Build map from attribute number 
                        ** to key number.
			*/

			m->mx_att_map[j] = i + 1;
			mod_key[i] = j;

			/*
			**  Build pointer for next key to 
                        **  corresponding attribute.
			*/

			m->mx_b_key_atts[i] = &m->mx_atts_ptr[i];
			m->mx_i_key_atts[i] = &m->mx_atts_ptr[i];
			STRUCT_ASSIGN_MACRO(t->tcb_atts_ptr[j], 
                                                 m->mx_atts_ptr[i]);
			m->mx_atts_ptr[i].key_offset = m->mx_kwidth;

			/*	Build sort comparision entry for next key. */

			m->mx_cmp_list[next_cmp].cmp_offset = 
			   t->tcb_atts_ptr[j].offset;
			m->mx_cmp_list[next_cmp].cmp_type = 
                                                   t->tcb_atts_ptr[j].type;
			m->mx_cmp_list[next_cmp].cmp_precision = 
			    t->tcb_atts_ptr[j].precision;
			m->mx_cmp_list[next_cmp].cmp_length = 
			    t->tcb_atts_ptr[j].length;
			m->mx_cmp_list[next_cmp].cmp_direction = 0;
			m->mx_cmp_list[next_cmp].cmp_collID =
			    t->tcb_atts_ptr[j].collID;

			/*  Allow alternate sort direction in HEAP. */

			if (mcb->mcb_key[i]->key_order == DM2U_DESCENDING)
			{
			    m->mx_cmp_list[next_cmp].cmp_direction++;
			    key_order[i]++;
			    if (m->mx_structure != TCB_HEAP)
			    {
				SETDBERR(dberr, 0, E_DM007C_BAD_KEY_DIRECTION);
				break;
			    }
			}
			next_cmp++;
			break;
		    }
		}
		if (dberr->err_code)
		    break;
		if (j > t->tcb_rel.relatts)
		{
		    SETDBERR(dberr, 0, E_DM0007_BAD_ATTR_NAME);
		    break;
		}
		m->mx_kwidth += t->tcb_atts_ptr[j].length;
		OnlyKeyLen += t->tcb_atts_ptr[j].length;
	    }
	    if (mcb->mcb_clustered)
	    {
		m->mx_leaf_rac.att_ptrs = m->mx_data_rac.att_ptrs;
		m->mx_leaf_rac.att_count = m->mx_data_rac.att_count;
	    }
	    else
	    {
		m->mx_leaf_rac.att_ptrs = m->mx_b_key_atts;
		m->mx_leaf_rac.att_count = m->mx_ab_count;
	    }
	    m->mx_index_rac.att_ptrs = m->mx_b_key_atts;
	    m->mx_index_rac.att_count = m->mx_ab_count;

	    /* 
	    ** Use the BTREE index compexpand accessor if index compression
	    ** is in use to calculate the possible expansion for checking if
	    ** key, after compression, is too large.  This is not necessarily
	    ** a definitive value (since BTREE load-begin can fool with the
	    ** index atts, which can in theory change the expansion, although
	    ** not in practice with STD compression and tidp changed.)
	    ** It's good enough to verify key sizing though.
	    **
	    ** Note in a Clustered primary, all attributes are in the
	    ** leaf.
	    */

	    key_expansion = 0;
	    if ( mcb->mcb_index_compressed )
	    {
		key_expansion = dm1c_compexpand(TCB_C_STD_OLD,
			m->mx_leaf_rac.att_ptrs, m->mx_leaf_rac.att_count);
	    }

	    if (dberr->err_code)
		break;
	}

	/*  An index can't be modified to heap. */

	if (reorg == 0 )
	{ 
	    if ((t->tcb_rel.relstat & TCB_INDEX) && m->mx_structure == TCB_HEAP)
	    {
		SETDBERR(dberr, 0, E_DM001C_BAD_KEY_SEQUENCE);
		break;
	    }

	    /*
	    **  Check that key is small enough for structure that put 
	    **  limits on key length. If modifying to compressed also
	    **	check that the length of the records will not exceeded
	    **	MAX_TUP in worst case compresssion.  Worse case occurs
	    **  when each C field gets an extra byte in compressed form
	    **	and each CHAR field gets an extra two bytes. 23-sep-88 (sandyh)
	    **
	    **	Note that when checking the key length on a Btree using key
	    **	compression we must use the locally-calculated count of key
	    **	compression overhead, not tcb_comp_katts_count, since the TCB
	    **	information is pre-modify and the locally-calculated information
	    **	reflects the new key which will be built.
	    */

	    if (m->mx_width + row_expansion >
		    dm2u_maxreclen(m->mx_page_type, m->mx_page_size)
		    && DMPP_VPT_PAGE_HAS_SEGMENTS(m->mx_page_type) == 0)
	    {
		SETDBERR(dberr, m->mx_width + row_expansion,
				E_DM0110_COMP_BAD_KEY_LENGTH);
		break;
	    }
	    if (m->mx_structure == TCB_ISAM)
	    {
		if (m->mx_kwidth > 
		    (dm2u_maxreclen(DM_COMPAT_PGTYPE, DM_COMPAT_PGSIZE) + 
			DM_VPT_SIZEOF_LINEID_MACRO(DM_PG_V1) - 
			    2 * DM_VPT_SIZEOF_LINEID_MACRO(DM_PG_V1)) / 2)
		{
		    SETDBERR(dberr, m->mx_kwidth, E_DM010F_ISAM_BAD_KEY_LENGTH);
		    break;
		}	 
		if (m->mx_kwidth > 
		    (dm2u_maxreclen(m->mx_page_type, m->mx_page_size) + 
			DM_VPT_SIZEOF_LINEID_MACRO(m->mx_page_type) - 
			    2 * DM_VPT_SIZEOF_LINEID_MACRO(m->mx_page_type)) / 2)
		{
		    SETDBERR(dberr, m->mx_kwidth, E_DM010F_ISAM_BAD_KEY_LENGTH);
		    break;
		}	 
	    }
	    if (m->mx_structure == TCB_BTREE)
	    {
		i4	max_leaflen;
		i4	adj_leaflen = m->mx_kwidth + key_expansion;

		/* Clustered primaries don't store attribute info on leaf pages */

/* --> comment out clustered index max_leaflen
		max_leaflen = DM1B_VPT_MAXLEAFLEN_MACRO(
					m->mx_page_type, m->mx_page_size, 
					(mcb->mcb_clustered)
						? 0
					        : m->mx_ai_count,
					OnlyKeyLen);
*/

		max_leaflen = min(
		    min(DM1B_KEYLENGTH, DM1B_MAXKEY_MACRO(m->mx_page_size)),
		    DM1B_VPT_MAXKLEN_MACRO(m->mx_page_type, m->mx_page_size, 
					    m->mx_ab_count + 1));

		if (adj_leaflen > max_leaflen || OnlyKeyLen > DM1B_KEYLENGTH )
		{
		    TRdisplay(
		"Bad keylen (%dK V%d) (leaflen %d atts %d klen %d) max (%d,%d)\n",
			m->mx_page_size/1024, m->mx_page_type, adj_leaflen,
			m->mx_ai_count,
			OnlyKeyLen,
			max_leaflen,
			DM1B_KEYLENGTH);
		    SETDBERR(dberr, adj_leaflen, E_DM007D_BTREE_BAD_KEY_LENGTH);
		    *mcb->mcb_tup_info = max_leaflen;
		    break;
		}

		/*
		** Make sure kperleaf >= 2
		** adj_leaflen <= max_leaflen should always guarantee kperleaf >= 2
		*/
		if (m->mx_page_type != TCB_PG_V1 && 
			    m->mx_page_size == DM_COMPAT_PGSIZE)
		{
		    if (dm1b_kperpage(m->mx_page_type, m->mx_page_size,
				    DMF_T_VERSION, m->mx_ai_count,
				    adj_leaflen, DM1B_PLEAF,
				    m->mx_tidsize,
				    mcb->mcb_clustered)
				    < 2)
		    {
			/* Need to use different page type or page size */
			SETDBERR(dberr, adj_leaflen, E_DM007D_BTREE_BAD_KEY_LENGTH);
			*mcb->mcb_tup_info = max_leaflen;
			break;
		    }
		}
	    }

	    /*
	    **  Insert comparisions on non-key domains if not 
	    **  unique key checking so that
	    **  duplicate records can be checked for.  
	    **  Also handle inserting the all key
	    **  fields of an index that didn't have any key fields specified.
	    */

	    if (mcb->mcb_unique == 0 && (t->tcb_rel.relstat & TCB_INDEX) == 0)
	    {
		for (i = 1; i <= t->tcb_rel.relatts; i++)
		{
		    if ((key_map[i >> 5] & (1 << (i & 0x1f))) == 0)
		    {
			m->mx_cmp_list[next_cmp].cmp_offset = 
					t->tcb_atts_ptr[i].offset;
			m->mx_cmp_list[next_cmp].cmp_type = 
                                        t->tcb_atts_ptr[i].type;
			m->mx_cmp_list[next_cmp].cmp_precision = 
					t->tcb_atts_ptr[i].precision;
			m->mx_cmp_list[next_cmp].cmp_length = 
					t->tcb_atts_ptr[i].length;
			m->mx_cmp_list[next_cmp].cmp_direction = 0;
			m->mx_cmp_list[next_cmp].cmp_collID =
			    		t->tcb_atts_ptr[i].collID;
			next_cmp++;
		    }
		}
	    }

	    /*
	    ** Lastly append the source's TID8 (partno,TID)
	    ** as the low-order sort key; it'll be affixed
	    ** at the end of each source row right after the
	    ** hash bucket (if any) and target partno.
	    */
	    if ( m->mx_structure == TCB_HASH )
		m->mx_cmp_list[next_cmp].cmp_offset = t->tcb_rel.relwid
				+ sizeof(i4) + sizeof(u_i2);
	    else
		m->mx_cmp_list[next_cmp].cmp_offset = t->tcb_rel.relwid
				+ sizeof(u_i2);
	    m->mx_cmp_list[next_cmp].cmp_type = DB_INT_TYPE;
	    m->mx_cmp_list[next_cmp].cmp_precision = 0;
	    m->mx_cmp_list[next_cmp].cmp_length = sizeof(DM_TID8);
	    m->mx_cmp_list[next_cmp].cmp_direction = 0;
	    m->mx_cmp_list[next_cmp].cmp_collID = -1;
	    next_cmp++;

	    m->mx_c_count = next_cmp;
	}

	/* 
	** Flush any pages left in buffer cache for this table. 
	** This is required for recovery as a non-redo log record 
	** immediately follows.
	** If online modify, don't flush pages here. Do it after 
	** do_online_modify(). Otherwise may get E_DM9C01 err msgs.
	**
	** If in-memory temp table, don't flush either - there's no
	** need.
	*/
	if ( !m->mx_inmemory && (mcb->mcb_flagsmask & DMU_ONLINE_START) == 0 )
	{
	    status = dm0p_close_page(t, lk_id, log_id, 
				DM0P_CLCACHE, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Create the file(s) to be used for the modify operation.
	**
	** We create a temporary modify file (or set if result of modify
	** is a multi-location table) which we will load just below.
	** When this temporary file is loaded, we will rename it to the
	** real table file.
	*/
	if (! gateway)
	{
	    if ( mcb->mcb_flagsmask & DMU_ONLINE_START )
	    {
		/*
		** If modify table is exclusively locked or
		** online modify is prohibited by trace point,
		** don't do_online_modify(), but we still have to
		** create any new partitions because qeu_modify_prep()
		** didn't.
		*/
		if ( table_lock_mode == LK_X || DMZ_SRT_MACRO(12) )
		{
		    status = dm0p_close_page(t, lk_id, log_id, 
					DM0P_CLCACHE, dberr);
		    if (status != E_DB_OK)
			break;

		    /* If there are new partitions to be created, create them */
		    if ( m->mx_spcb_count < m->mx_tpcb_count )
		    {
			status = create_new_parts(m,
					&t->tcb_rel.relid,
					&t->tcb_rel.relowner,
					mcb->mcb_new_part_def,
					mcb->mcb_partitions,
					&m->mx_table_id,
					mcb->mcb_db_lockmode,
					relstat,
					m->mx_new_relstat2,
					dberr);

			/* Fill in tabid's in new targets */
			for (tp = m->mx_tpcb_next;
			     tp && status == E_DB_OK; 
			     tp = tp->tpcb_next)
			{
			    /* repartition, possibly new targets */
			    if (tp->tpcb_tabid.db_tab_base == 0)
				tp->tpcb_tabid = 
				    mcb->mcb_partitions[tp->tpcb_partno].part_tabid;
			}
		    }
		}
		else if (!((dcb->dcb_bm_served == DCB_MULTIPLE) && 
				((dcb->dcb_status & DCB_S_FASTCOMMIT) == 0)) &&
		    ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0) &&
		    ((t->tcb_rel.relstat & TCB_CONCUR) == 0) &&
		    t->tcb_seg_rows == 0 &&
		    t->tcb_rel.relid.db_tab_name[0] != '$' &&
		    t->tcb_rel.relowner.db_own_name[0] != '$' &&
		    t->tcb_rel.reltid.db_tab_base > DM_SCATALOG_MAX &&
		    !mcb->mcb_temporary &&
		    !CXcluster_enabled() &&
		    ((dcb->dcb_status & DCB_S_JOURNAL) == DCB_S_JOURNAL) &&
		    journal &&
		    t->tcb_rel.reltid.db_tab_index == 0)
		{
		    status = do_online_modify(&m, timeout, mcb->mcb_dmu,
			    mcb->mcb_modoptions, mcb->mcb_mod_options2,
			    mcb->mcb_kcount, mcb->mcb_key, mcb->mcb_db_lockmode,
			    &online_tabid, 
			    mcb->mcb_tup_info, mcb->mcb_has_extensions, 
			    mcb->mcb_relstat2, 
			    (mcb->mcb_flagsmask & ~DMU_ONLINE_START), 
			    mcb->mcb_rfp_entry, &online_reltup, 
			    mcb->mcb_new_part_def, mcb->mcb_new_partdef_size,
			    mcb->mcb_partitions, mcb->mcb_nparts, mcb->mcb_verify,
			    dberr);

		    if ( status == E_DB_OK )
		    {
			/* From here on, don't rely on DMU_ONLINE_START */
			online_modify = TRUE;

			r = rcb;
			m->mx_rcb = r;
			m->mx_newtup_cnt = *mcb->mcb_tup_info;

			STRUCT_ASSIGN_MACRO(m->mx_bsf_lsn, bsf_lsn);

			/* flush pages */
			status = dm0p_close_page(t, lk_id, log_id,
				    DM0P_CLCACHE, dberr);
		    }
		}
		else
		{
		    SETDBERR(dberr, 0, E_DM9CB3_ONLINE_MOD_RESTRICT);
		    status = E_DB_ERROR;
		    if (DMZ_SRT_MACRO(13))
		    {
			TRdisplay("ONLINE MODIFY: tbl %s db %s terminating...:\n",
			  		&t->tcb_rel.relid.db_tab_name,
					&dcb->dcb_name);
			if ((dcb->dcb_bm_served == DCB_MULTIPLE) &&
                                (dcb->dcb_status & DCB_S_FASTCOMMIT) == 0) 
			    TRdisplay("    - multi-dbms, fast commit off\n");
			if (CXcluster_enabled())
			    TRdisplay("    - clustered environment\n");
			if ((dcb->dcb_status & DCB_S_JOURNAL) == 0)
			    TRdisplay("    - database is not journaled\n");
			if (mcb->mcb_temporary)
			    TRdisplay("    - table is a temporary table\n");
			if (t->tcb_rel.reltid.db_tab_index)
			    TRdisplay("    - table is a secondary index\n");
			if ((t->tcb_rel.relstat & TCB_CONCUR) ||
			    (t->tcb_rel.reltid.db_tab_base <= DM_SCATALOG_MAX))
			    TRdisplay("    - catalog table (relstat = 0x%x reltid=(%d,%d)\n", 
					t->tcb_rel.relstat,
					t->tcb_rel.reltid.db_tab_base,
					t->tcb_rel.reltid.db_tab_index);
		    }
		}

		if ( status )
		    break;
	    }

	    /*
	    ** Unless overridden by system configuration option, 
	    ** temporary files created here employ DI_USER_SYNC_MASK
	    ** and are sync'd at end via dm2f_force_file().
	    */
	    if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
		db_sync_flag = 0;
	    else
		db_sync_flag = DI_USER_SYNC_MASK;
	    if (dmf_svcb->svcb_directio_load)
		db_sync_flag |= DI_DIRECTIO_MASK;
	    if (!online_modify)
		db_sync_flag |= DI_PRIVATE_MASK;

            for ( tp = m->mx_tpcb_next;
                  tp && status == E_DB_OK;
                  tp = tp->tpcb_next )
	    {
		/* Check for interrupts in this loop. */
		if ( status = dm2u_check_for_interrupt(xcb, dberr) )
		    break;

		if (m->mx_inmemory)
		{
		    STprintf(table_name.db_tab_name, "$$$$MDFY%d", (i4)
			m->mx_table_id.db_tab_base);
		    STmove(table_name.db_tab_name, ' ', 
			sizeof (table_name.db_tab_name), table_name.db_tab_name);

		    /*
		    ** Pass location name array for the create temp call
		    ** so that the temporary modify table is built on the
		    ** same set of locations as the resultant temporary table.
		    **
		    ** Note that even though this is an in-memory build, the
		    ** build table may get flushed to disk; and if that happens
		    ** then the files underlying the temp build tcb must exactly
		    ** match the result temp table or the file renames done
		    ** at the end of the modify will fail.
		    */
		    status = create_build_table( m, dcb, xcb, &table_name, 
			tp->tpcb_loc_list, tp->tpcb_loc_count, dberr );
		    if (status != E_DB_OK)
			break;

		    temp_cleanup = TRUE;
		}
		else
		{
		    /*
		    ** Allocate fcb for each location.
		    ** Tell dm2f how to build the temp filename, because if this
		    ** is rollforward, it had better match what was fcreated.
		    ** (DO has exclusive control of the database, so there are
		    ** no filename collision problems.)
		    */
		    status = dm2f_build_fcb(lk_id, log_id,
			tp->tpcb_locations, tp->tpcb_loc_count,
			mcb->mcb_page_size,
			DM2F_ALLOC, DM2F_MOD,
			&tp->tpcb_tabid,
			tp->tpcb_name_id, tp->tpcb_name_gen,
			(DMP_TABLE_IO *)0, 0, dberr);
		    if (status != E_DB_OK)
			break;

		    if (((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0) &&
			!online_modify)
		    {
			/*
			** Log and create the file(s).
			*/
			status = dm2u_file_create(dcb, log_id, lk_id,
			    &tp->tpcb_tabid, db_sync_flag, 
			    tp->tpcb_locations, tp->tpcb_loc_count,
			    mcb->mcb_page_size,
			    logging, journal, dberr);
			if (status != E_DB_OK)
			    break;
		    }
		    else
		    {
			  /* ROLLFORWARD, file exists, just open it */
			  status = dm2f_open_file(lk_id, log_id,
				    tp->tpcb_locations, tp->tpcb_loc_count,
				    mcb->mcb_page_size,  DM2F_A_WRITE, db_sync_flag, 
				    (DM_OBJECT *)dcb, dberr);
			  if (status != E_DB_OK)
			      break;
		    }
		    modfile_open = TRUE;
		}
	    }
	}

	if ( status )
	    break;

	/*
	**  Load the new files.
	**
	**  At this point the following MXCB fields should have the following
	**  values based on the type of modify begin performed.  Modifies of
	**  indices that are BTREE or ISAM always include the TIDP 
        **  as a key field.  Data fields are supported for all index types.
	**
	**  mx_b_key_atts  - One pointer for each base table attribute named.
	**  mx_ab_count    - Count of above entries.
	**  mx_i_key_atts  - One pointer for each key attribute plus
	**		     one for the implied TIDP key on non-HASH indices.
	**		     The key_offset field must be correctly set relative
	**		     to beginning of the key.
	**  mx_ai_count    - Count of above entries.
	**  mx_kwidth	   - Sum of key attributes plus TIDP is non-HASH
	**		     indices.  Non-key (data) fields of a btree index
	**		     are also included.
	**  mx_width	   - Total width of record, includes TIDP for indices.
	**  mx_idom_map	   - Map of index attributes to base table attributes
        **		     for key and data fields in an index excepting TIDP.
	**  mx_att_map	   - The key number for a attribute number.  
        **                   For non-HASH
	**		     tables this would be set for the TIDP field.
	**  mx_unique	   - If a unique index is begin created, 
        **                   this field should
	**		     set so that the load code checks 
        **                   for non-unique keys.
	**  mx_tbl_pri	   - Table's cache priority, old or new value.
	**
	** If this is a gateway table, then don't actually load any data.
	** The real table is managed by the gateway itself, we just modify
	** the ingres catalog information.
	*/
	if (mcb->mcb_omcb)
	{
	    for (tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next)
	    {
		tp->tpcb_dupchk_tabid = 
			mcb->mcb_omcb->omcb_dupchk_tabid + tp->tpcb_partno;
	    }
	    m->mx_flags |= MX_ONLINE_MODIFY;

	    if (dcb->dcb_status & DCB_S_ROLLFORWARD)
		m->mx_flags |= MX_ROLLDB_ONLINE_MODIFY;

	    if (logging)
	    {
		i4 dum_flag = DM0L_START_ONLINE_MODIFY;

		if (mcb->mcb_compressed == TCB_C_HICOMPRESS)
		    dum_flag |= DM0L_MODIFY_HICOMPRESS;
		if (mcb->mcb_compressed == TCB_C_STANDARD)
		    dum_flag |= DM0L_MODIFY_NEWCOMPRESS;
		/* Other stuff can go here (page compression, etc) */
		status = logModifyData(log_id, dm0l_jnlflag,
			mcb->mcb_new_part_def, mcb->mcb_new_partdef_size,
			mcb->mcb_partitions, mcb->mcb_nparts, dberr);
		if (status != E_DB_OK)
		{
		    break;
		}

		/*
		** Note that when partitioning, the partition location information
		** in mcb_partitions has already been logged as RAWDATA, above,
		** so there's no need to log any location info in dm0l_modify.
		*/

		status = dm0l_modify(log_id, dm0l_jnlflag,
				mcb->mcb_tbl_id, &table_name, &owner_name,
				(mcb->mcb_partitions) ? 0 : loc_count,
				orig_locnArray, m->mx_structure,
				relstat, mcb->mcb_min_pages, mcb->mcb_max_pages,
				mcb->mcb_i_fill, mcb->mcb_d_fill, mcb->mcb_l_fill,
				m->mx_newtup_cnt,
				buckets, mod_key, key_order, mcb->mcb_allocation,
				mcb->mcb_extend, m->mx_page_type, m->mx_page_size,
				dum_flag,
				&input_rcb->rcb_tcb_ptr->tcb_rel.reltid,
				mcb->mcb_omcb->omcb_bsf_lsn,
				name_id, base_name_gen, 
				(LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
		already_logged = TRUE;
	    }
	}

	if (! gateway)
	{
	    if (reorg)
		status = dm2u_reorg_table(m, dberr);
	    else if (!online_modify)
	    {
		/* Load the table from the sorter. */
		status = dm2u_load_table(m, dberr);

		/* rollforward online modify */
		if ((dcb->dcb_status & DCB_S_ROLLFORWARD) && 
		    (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH) &&
		    (status == E_DB_INFO) &&
		    (mcb->mcb_omcb))
		{
		    /* save mxcb context */
		    octx->octx_mxcb = (PTR)m;
		    return(E_DB_INFO);
		}
	    }
	    if ( status )
		break;

	    /*
	    ** Close the modify file(s) just loaded in preparation for the
	    ** renames below (in dm2u_update_catalogs).
	    */
	    if ( ! m->mx_inmemory)
	    {
		modfile_open = FALSE;

		for ( tp = m->mx_tpcb_next;
		      tp && status == E_DB_OK;
		      tp = tp->tpcb_next )
		{
		    /*
		    ** Opening files DI_USER_SYNC_MASK requires force prior to 
		    ** close.
		    */
		    if (db_sync_flag & DI_USER_SYNC_MASK)
		    {
			status = dm2f_force_file(
			    tp->tpcb_locations, tp->tpcb_loc_count,
			    &table_name, &dcb->dcb_name, dberr);
		    }

		    if ( status == E_DB_OK )
		    {
			status = dm2f_close_file(
					    tp->tpcb_locations,
					    tp->tpcb_loc_count,
					    (DM_OBJECT*)dcb,
					    dberr);
		    }
		}
		if (status != E_DB_OK)
		    break;
	    }
	}
	else
	{
	    DM2U_M_CONTEXT	*mct;	/* TPCB's MCT */

	    /*
	    ** If this is a gateway, then we did not load the new table.
	    ** Unfortunately, the dm2u_update_catalogs call below (that
	    ** updates the iirelation, attribute info to show the the fields
	    ** on which the table is keyed) assumes that certain information
	    ** will have been filled in by the dm2u_load_table call.
	    ** So, fill them into the MCT that the TPCB points to.
	    ** (For gateway modify, there's only one.)
	    */

	    /*
	    ** 16-jun-90 (linda)
	    ** reltups came from table registration (or default value).
	    ** relpages came from gateway table open.  Ensure that we never
	    ** start out with negative reltups value, or negative or 0 relpages.
	    ** Other values are guesses on my part...
	    */

	    /* For Gateways, there will be (had better be) one target */
	    tp = m->mx_tpcb_next;
	    mct = &tp->tpcb_mct;

	    tp->tpcb_newtup_cnt = 
		(t->tcb_rel.reltups >= 0) ? (i4)t->tcb_rel.reltups : 0;
	    mct->mct_relpages = 
		(t->tcb_rel.relpages > 0) ? (i4)t->tcb_rel.relpages : 1;
	    mct->mct_prim =
		(t->tcb_rel.relpages > 0) ? (i4)t->tcb_rel.relpages : 1;

	    mct->mct_main = 1;
	    mct->mct_i_fill = 90;
	    mct->mct_l_fill = 90;
	    mct->mct_d_fill = 90;
	    mct->mct_keys = m->mx_ai_count;
	}

	/*
	** If modify to HEAP with keys (heapsort), zero out the key map and
	** key count so that we won't mark any of the attributes as keys when
	** we update the system catalogs below.
	*/
	if (reorg == 0)
	{
	    if ((m->mx_structure == TCB_HEAP) && (mcb->mcb_kcount > 0))
	    {
		for ( tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next )
		    tp->tpcb_mct.mct_keys = 0;
		MEfill((sizeof(*m->mx_att_map) * (t->tcb_rel.relatts+1)),
			'\0', (PTR) m->mx_att_map);
	    }

	    *mcb->mcb_tup_info = m->mx_newtup_cnt;
	}

	/*  Close purge the table. */
	/* Note that rowaccessors in the MXCB become invalid now!
	** They point to att arrays in the to-be-purged TCB.
	*/

	status = dm2t_close(r, DM2T_PURGE, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;

	/* If we had to open a master, close it.  (No need to purge,
	** we only opened to master to guarantee that the partition open
	** would succeed.)
	*/
	if (master_rcb != NULL)
	{
	    status = dm2t_close(master_rcb, 0, dberr);
	    if (status != E_DB_OK)
		break;
	    master_rcb = NULL;
	}

	/* If rollforward, we are done */
	if (dcb->dcb_status & DCB_S_ROLLFORWARD)
	{
	    for ( tp = m->mx_tpcb_next;
		  tp && status == E_DB_OK;
		  tp = tp->tpcb_next )
	    {
		status = dm2f_release_fcb(lk_id,log_id,
			 tp->tpcb_locations, tp->tpcb_loc_count,
			 DM2F_ALLOC, dberr);
	    }
            if (status != E_DB_OK)
		break;

	    dm0m_deallocate((DM_OBJECT **)&m);
	    return (E_DB_OK);
	}

	/*
	** Log the modify operation - unless logging is disabled.
	** For special modifies (TRUNCATE, REORGANIZE), pass in
	** special modify flags instead of the table structure.
	**
	** The MODIFY log record is written after the actual modify
	** operation above which builds the temporary modify files.
	** Note that until now, the user table (and its metadata) has
	** not been physically altered.  We now log the modify before
	** updating the catalogs and swapping the underlying files.
	**
	** The modify logging is intended to provide enough information
	** so that the dm2u-modify call can essentially be repeated as
	** if it were the first time.
	*/
	if ((logging) && (!already_logged))
	{
	    i4 dum_flag = 0;

	    status = logModifyData(log_id, dm0l_jnlflag,
			mcb->mcb_new_part_def, mcb->mcb_new_partdef_size,
			mcb->mcb_partitions, mcb->mcb_nparts, dberr);
	    if (status != E_DB_OK)
		break;

	    if (truncate)
	        dm0l_structure = DM0L_TRUNCATE;
	    else if (reorg)
	        dm0l_structure = DM0L_REORG;
	    else
	        dm0l_structure = m->mx_structure;

	    if (online_modify)
		dum_flag |= DM0L_ONLINE;
	    if (mcb->mcb_compressed == TCB_C_HICOMPRESS)
		dum_flag |= DM0L_MODIFY_HICOMPRESS;
	    if (mcb->mcb_compressed == TCB_C_STANDARD)
		dum_flag |= DM0L_MODIFY_NEWCOMPRESS;

	    /*
	    ** Note that when partitioning, the partition location information
	    ** in mcb_partitions has already been logged as RAWDATA, above,
	    ** so there's no need to log any location info in dm0l_modify.
	    */

	    status = dm0l_modify(log_id, dm0l_jnlflag,
		mcb->mcb_tbl_id, &table_name, &owner_name,
		(mcb->mcb_partitions) ? 0 : loc_count,
		orig_locnArray,
		dm0l_structure, relstat, mcb->mcb_min_pages, mcb->mcb_max_pages,
		mcb->mcb_i_fill, mcb->mcb_d_fill, mcb->mcb_l_fill, m->mx_newtup_cnt,
		buckets, mod_key, key_order,
		mcb->mcb_allocation, mcb->mcb_extend, m->mx_page_type, m->mx_page_size,
		dum_flag, (DB_TAB_ID *)0,
		bsf_lsn, name_id, base_name_gen,
		(LG_LSN *)0, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*  Perform the neccesary catalog updates. */

	if (mcb->mcb_temporary)
	{
	    status = update_temp_table_tcb(m, dberr);
	    temp_cleanup = FALSE;
	}
	else
	{
	    /* 
	    ** Pass the journal status of the table to dm2u_update_catalogs()
	    ** because rcb has been purged and shouldn't make any further
	    ** reference on it.
	    */
	    status = dm2u_update_catalogs(m, journal, dberr);
	}
	if (status != E_DB_OK)
	    break;
	

	/*
	** Log the DMU operation.  This marks a spot in the log file to
	** which we can only execute rollback recovery once.  If we now
	** issue update statements against the newly-modified table, we
	** cannot do abort processing for those statements once we have
	** begun backing out the modify.
	*/
	if (logging)
	{
	    status = dm0l_dmu(log_id, dm0l_jnlflag, mcb->mcb_tbl_id, 
			    &table_name, &owner_name, (i4)DM0LMODIFY, 
			    (LG_LSN *)0, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Release file control blocks allocated for the table.  If this is
	** a gateway index then there are really no files to release. If we
	** did a build into memory then there are no modify files to release.
	*/
	if (gateway == 0 && m->mx_inmemory == 0)
	{
	    for ( tp = m->mx_tpcb_next;
		  tp && status == E_DB_OK;
		  tp = tp->tpcb_next )
	    {
		status = dm2f_release_fcb(lk_id, log_id,
		    tp->tpcb_locations, tp->tpcb_loc_count,
		    DM2F_ALLOC, dberr);
	    }
	    if ( status )
		break;

	    for ( sp = m->mx_spcb_next;
		  sp && status == E_DB_OK;
		  sp = sp->spcb_next )
	    {
		if ( sp->spcb_locations && sp->spcb_locations[0].loc_fcb )
		    status = dm2f_release_fcb(lk_id, log_id,
			sp->spcb_locations, sp->spcb_loc_count,
			DM2F_ALLOC, dberr);
	    }
	    if (status != E_DB_OK)
		break;
	}

	dm0m_deallocate((DM_OBJECT **)&m);

	/* Rebuild persistent indices */
	return( rebuild_persistent(dcb, xcb, mcb->mcb_tbl_id, 
			mcb->mcb_dmu, online_modify, &online_tabid) );
    }

    /*
    ** Handle cleanup for error recovery.
    */

    if ( m )
    {
	/*
	** If we left the newly created modify files open, then close them.
	** We don't have to worry about deleting them as that is done by
	** rollback processing.
	*/
	if (modfile_open)
	{
	    for ( tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next )
	    {
		/* Don't close that which hasn't been opened */
		if ( tp->tpcb_locations && tp->tpcb_locations[0].loc_fcb )
		{
		    status = dm2f_close_file(
				    tp->tpcb_locations, tp->tpcb_loc_count,
				    (DM_OBJECT*)dcb, &local_dberr);
		    if (status != E_DB_OK)
		    {
			uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    }
		}
	    }
	}

	/*
	** Release the FCB memory allocated by dm2f_build_fcb.
	*/
	for ( tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next )
	{
	    if ( tp->tpcb_locations && tp->tpcb_locations[0].loc_fcb )
	    {
		status = dm2f_release_fcb(lk_id, log_id,
					tp->tpcb_locations, tp->tpcb_loc_count,
					DM2F_ALLOC, &local_dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		}
	    }
	}

	for ( sp = m->mx_spcb_next; sp; sp = sp->spcb_next )
	{
	    if ( sp->spcb_locations && sp->spcb_locations[0].loc_fcb )
	    {
		status = dm2f_release_fcb(lk_id, log_id,
		    sp->spcb_locations, sp->spcb_loc_count,
		    DM2F_ALLOC, &local_dberr);
		if (status != E_DB_OK)
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	}

	/* temporary table never closed, due to error */
	if ( temp_cleanup && m->mx_buildrcb ) 
	{
	    status = dm2t_close(m->mx_buildrcb, DM2T_NOPURGE, &local_dberr);
	    if (status != E_DB_OK)
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}

	dm0m_deallocate((DM_OBJECT**)&m);
    }


    /*
    ** Close the user table.
    */
    if (r)
    {
	status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    /* If we had to open a master, close it too */
    if (master_rcb != NULL)
    {
	status = dm2t_close(master_rcb, DM2T_NOPURGE, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2u_sysmod - Modify a key system catalogs.
**
** Description:
**      This routine is called from dm2u_modify() to modify a 
**      the three key system catalogs table's structure.
**      The relation table (and its index), the attribute
**      table and the indexes table.  These table are used
**      to open the database, therefore, special code is
**      needed to insure the success of these operations
**      without affecting the ability of the server to run.
**      The strategy for modify these core system tables is:
**
**      Before calling this routines all parameters and
**      privileges will be checked.  For example you 
**      can only modify one of these tables from hash to
**      hash on exactly the same keys and only if you
**      are excuting in a priviledged session with the
**      database openned exclusively.  
**
**      This routine creates a new (with different temporary 
**      name )base table, creates
**      and index on it.  Modifies the new base table and
**      index for the appropriate number of hash buckets
**      based on their current tuple counts.  Then
**      reads the tuples from old system catalog into the
**      new base table automatically updating the index.
**      
**      The new base table is updated to get rid of the
**      old base table entries, the new entries are modified
**      to have the correct data, such as correct table-id
**      and correct name.  Now the new tables are ready to 
**      the active catalog tables.
**
**      To make these the active catalog table. A special log 
**      record is written to indicate that some table renames
**      are going to take place, and to indicate what the
**      old and new number of primary pages (hash buckets) are.
**      Once the log record is written, then the old tables 
**      are marked for deletion by changing their name to
**      <oldfilename>.del from <oldfilename>.tab.  The new tables
**      are changed from <newfilename>.tab to <filename>.tab
**      the buffer manager is flushed of all pages for
**      <oldfilename>, the old file is closed, the new file
**      is openned, and the TCB which contains information 
**      needed to read the new table (i.e. # hash buckets)
**      is updated in memory.  The configuration file is
**      updated so the next time the database is openned it
**      has the correct hash information.
**
**      If the process or system crashes at any point, 
**      the special log record is read, and determines how
**      far the process has proceeded and undo anything that
**      is required.
**      
** Inputs:
**	dcb				The database id. 
**	xcb				The transaction id.
**	tbl_id				The internal table id of the base table 
**					to be modified.
**	structure			Structure to modify.
**	i_fill				Fill factor of the index page.
**	l_fill				Fill factor of the leaf page.
**	d_fill				Fill factor of the data page.
**	compressed			TRUE if compressed records.
**	min_pages			Minimum number of buckets for HASH.
**	max_pages			Maximum number of buckets for HASH.
**	unique				TRUE if unique key.
**	kcount				Count of key entry.
**	key				Pointer to key array of pointers
**                                      for modifying.
**	db_lockmode			Lockmode of the database for the caller.
**
** Outputs:
**      err_code                        The reason for an error return status.
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
**      07-nov-86 (jennifer)
**          Created for Jupiter.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() calls.
**	08-nov-88 (teg)
**	    changed error parameter to dm2u_create() to accomodate new call I/F
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	10-feb-89 (rogerk)
**	    Count tuples when filling new syscat table and update correct
**	    reltups value in iirelation.
**	21-jul-89 (teg)
**	    add reltups parameter to fix bug 6805.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Added new arguments to
**	    dm2u_create/dm2u_index.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of the Modify.
**	23-feb-90 (greg)
**	    SYSMOD was AV'ing when this file was built with optimization
**	    on VMS.  Added intermediate variable attrl_ptr.
**	23-apr-90 (linda)
**	    Add parameter to dm2u_index() call for source for gateway object.
**	    In this case (called from dm2u_sysmod()), the parameter is NULL.
**	26-apr-90 (linda)
**	    Add parameter to dm2u_index() call for qry_id -- used by gateways
**	    for iiregistrations catalog.  In this case the parameter is NULL.
**	17-may-90 (bryanp)
**	    After we make a significant change to the config file (increment
**	    the table ID or modify a core system catalog), make an up-to-date
**	    backup copy of the config file for use in disaster recovery.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**	     7-mar-1991 (bryanp)
**		New argument to dm2u_index for Btree index compression project.
**	    11-apr-1991 (rudyw)
**		Integrate change from 6.2 which failed to make it into the
**		current code line. Change last parameter of call to dm2u_index
**		from error (a i4) to errcb (a two i4 structure). The
**		structure is expected and its two values get initialized. Also
**		needed to set error from errcb on bad status return so that it
**		was set as expected. The error did not seem to affect most
**		ports.  It caused sysmod to fail on ODT.
**	    16-jul-1991 (bryanp)
**		B38527: new reltups arg to dm2u_index, dm0l_modify, dm0l_index.
**		'reltups' is passed in here containing the number of tuples in
**		the core catalog being modified. In the case of iirelation,
**		this value is also passed to dm2u_index so that iirel_idx (or,
**		actually, iiridxtemp) will be modified with the appropriate
**		number of buckets. Also, fixed journalling of system catalog
**		changes (journal the iirelation updates, as well as the SM1 &
**		SM2 config records).  Added a new log record,
**		DM0L_SM0_CLOSEPURGE, to be logged to indicate when the load of
**		iirtemp is completed so that rollforward will know when it
**		should close and purge iirtemp preparatory to renaming it with
**		the core catalog it replaces.
**	17-jan-1992 (bryanp)
**	    New "temporary" argument to dm2u_modify()
**	15-apr-1992 (bryanp)
**	    Remove the FCB argument to dm2f_rename().
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Update config file only if iirelation is being modified.
**      26-may-1992 (schang)
**              the addition of gw_id as arg to dm2u_index did not make into
**              last merge so I add it to the end of arg list
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Update relcomptype and relpgtype for in-store
**	    core catalog descriptions. Also re-get accessors in case we now
**	    need different ones.
**	16-0ct-1992 (rickh)
**	    Initialize default ids for attributes.
**	30-October-1992 (rmuth)
**	    - Remove all references to the DI_ZERO_ALLOC file open flag.
**	    - When checking for FCB_OPEN use the & instead of == because
**	      we my not flush when allocating.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section in config file.
**	02-nov-1992 (jnash)
**	    Reduced logging project.  Add new params to dm0l_frename,
**	    dm0l_sm1_rename, dm0l_sm2_config and dm0l_dmu.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	06-dec-1992 (jnash)
**	    Swap order of gw_id and char_array params in dm2u_index call
**	    to quiet compiler.
**	25-feb-93 (rickh)
**	    Add relstat2 argument to dm2u_index call.
**	8-apr-93 (rickh)
**	    Add relstat2 argument to dm2u_modify call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	01-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use appropriate case for iirtemp and iiridxtemp in dm2u_sysmod
**	22-may-93 (andre)
**	    upcase (a bool) was being assigned a result of &'ing a u_i4
**	    field and long constant.  On machines where bool is char, this
**	    assignment will not produce the expected result
**	24-may-1993 (robf)
**	    Increase attr_entry array to 41 to handle new iirelation
**	    attributes. Add an error check to make sure we don't
**	    overflow the array (probably if dmmcre.c is updated without the
**	    array size here being increased.)
**	21-jun-1993 (jnash)
**	    Add error check after dm0p_close_page() call.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    DMU log record is now journaled.
**	16-sep-93 (rickh)
**	    The key_order field in the key descriptors for SCONCUR catalogs
**	    were being stuffed with the flags field from the corresponding
**	    attribute tuples.  Odd.  Changed this to force the SCONCUR keys
**	    to always have ascending order.
**	18-apr-1994 (jnash)
**	    fsync project.  Set db_sync_flag == DI_USER_SYNC_MASK when 
**	    loading temporary files used during the modify.
**	28-june-94 (robf)
**          Updated parameters to dm2u_destroy()
**	22-jul-94 (robf)
**	    Increase attr_entry array to 45 to handle new iirelation
**	    attributes. Add an error check to make sure we don't
**	    overflow the array (probably if dmmcre.c is updated without the
**	    array size here being increased.)
**      27-dec-94 (cohmi01)
**          For RAW I/O, pass location ptr to dm2f_rename().
**	06-mar-1996 (stial01 for bryanp)
**	    Pass DM_PG_SIZE page_size to dm2u_create/dm2u_index/dm2u_modify.
**	    Dimension attribute arrays using DMP_NUM_IIRELATION_ATTS, not "40"
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Pass DM_PG_SIZE to dm1c_getaccessors().
**	25-oct-1996 (hanch04)
**	    Hard coded ntab_width for II_ATTRIBUTE to 96
**	24-Mar-1997 (jenjo02)
**	    Added tbl_pri parm to dm2u_index() prototype.
**	26-may-1998 (angusm for stial01)
**	    dm2u_sysmod() The journal status for each insert into iirtemp
**	    depends on the journal status of the underlying user table (b65170)
**	17-aug-98 (hayke02 for nanpr01)
**	    Fix up segv for sysmod due to null pointer.
**      21-May-1997 (stial01)
**          dm2u_sysmod() The journal status for each insert into iirtemp
**          depends on the journal status of the underlying user table (b65170)
**      22-May-1997 (stial01)
**          dm2u_sysmod() Invalidate tbio_lpageno, tbio_lalloc after core
**          catalog file renames (B78889, B52131, B57674)
**	26-may-1998 (nanpr01)
**	    Initialized the header part of the index_cb control block.
**      28-may-1998 (stial01)
**          dm2u_sysmod() Support VPS system catalogs.
**          Improved chances of SYSMOD recovery if SYSMOD is interrupted.
**	20-jul-1998 (nanpr01)
**	    Fix up the segv for sysmod due to null pointer.
**      28-sep-98 (stial01)
**          dm2u_sysmod() Undo 21-May-1997 (stial01) (B93404)
**	30-sep-1998 (marro11)
**	    Backing out 26-may-1998 fix for bug 65170; it introduces an 
**	    undesirable side effect:  bug 93404.  Also backing out 17-aug-98
**	    change, as this was stacked on aforementioned change.
**	01-oct-1998 (nanpr01)
**	    Added the new dm2r_replace parameter for update performance
**	    improvement.
**	01-apr-1999 (nanpr01)
**	    Put the lost parameter during forward integration back.
**      15-jun-2000 (stial01 for jenjo02)
**          dm2u_sysmod changing page_size, update hcb_cache_tcbcount's B101850
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Initialise the new indxcb_dmveredo field in the index_cb.
**	17-Jan-2004 (schka24)
**	    Jon's very clever idea to update iirelation rows by tid works
**	    wonderfully well, until one modifies iirelation!  Which both
**	    createdb and sysmod do.  If we're moving iirelation around,
**	    dump user TCB's first, and fix up the remaining catalog TCB's
**	    to contain the new iirelation row tid.
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          We modify each of the system catalogs and use "iirtemp" as
**          the temporary copy. Flag whether the temporary relation
**          represents iirelation and whether the current row describes
**          a non-journalled table so that dm1r_put() can add the
**          appropriate journal flags to identify this case.
**	15-dec-04 (inkdo01)
**	    Copy attcollid value, too.
**	14-Jan-2005 (jenjo02)
**	    Added DM2T_TOSS flag when releasing user TCBs.
**	30-May-2006 (jenjo02)
**	    Removed silly reliance on DMP_NUM_IIRELATION_ATTS, allocate
**	    memory for DMF_ATTR_ENTRY stuff based on tcb_relatts, which
**	    is all we care about anyway.
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**	22-Jul-2008 (kschendel) SIR 122757
**	    Don't open the renamed rtemp file according to load-optim!
**	    This isn't a load open, it's the real table open, and must be
**	    done in normal sync mode.
**	    Control direct IO here, via the svcb.
**	29-Sept-2009 (troal01)
**		Add geospatial support
*/
DB_STATUS
dm2u_sysmod(
DMP_DCB		*dcb,
DML_XCB		*xcb,
DB_TAB_ID       *tbl_id,
i4		structure,
i4		newpgtype,
i4		newpgsize,
i4		i_fill,
i4		l_fill,
i4		d_fill,
i4		compressed,
i4		min_pages,
i4		max_pages,
i4		unique,
i4		merge,
i4         truncate,
i4         kcount,
DM2U_KEY_ENTRY  **key,
i4         db_lockmode,
i4		allocation,
i4		extend,
i4		*tup_cnt,
i4		reltups,
DB_ERROR	*dberr)  
{
    DMP_RCB     *r = (DMP_RCB *)0;
    DMP_RCB	*nr = (DMP_RCB *)0;
    DMP_RCB	*dev_rcb = (DMP_RCB *)0;
    DMP_RCB	*r_upt, *rcb;
    DMP_TCB     *t, *nt;
    DMP_FCB     *f;
    DB_TAB_NAME	modidx_name;
    DB_TAB_NAME	modtab_name;
    DB_TAB_NAME	newtab_name;
    DB_TAB_NAME	newidx_name;
    DB_LOC_NAME location;
    DB_OWN_NAME owner;
    DB_TAB_ID       oldtab_id;
    DB_TAB_ID       newtab_id;
    DB_TAB_ID       ntbl_id;
    DB_TAB_ID       nidx_id;
    DB_TAB_ID       tblidx_id;
    DB_TAB_ID       reltbl_id;
    i4             lock_mode;  
    i4             purge_mode;
    i4		index = 0;
    i4		view = 0;
    i4		journal;
    i4		dm0l_jnlflag;
    i4		duplicates;
    i4		ntab_width;
    DM2T_KEY_ENTRY      *key_list[3];
    DM2T_KEY_ENTRY      key_entry[3];
    DB_ATTS		*att_ptr;
    i4             i;
    DB_STATUS           status, local_status;
    DM_TID              tid;
    DM_TID		cat_tid, catx_tid;
    DMP_RELATION	*new_rel;
    DMP_RELATION        old_rel;
    DMP_RELATION        old_ridx;
    DMP_RELATION        save_rel;
    DMP_RELATION        save_ridx;
    DM2R_KEY_DESC	qual_list[2];
    DMP_RELATION        rel_record;
    DM_FILE         	savefilename;
    i4         	filelength, oldfilelength;
    DM_FILE         	old1tabname, old2tabname, new1tabname, new2tabname;
    DM_PATH         	*phys_location;
    i4         	phys_length;
    DM0C_CNF		*config = 0;
    i4             error;
    DB_TAB_TIMESTAMP    timestamp;
    LG_LSN		lsn;
    i4             local_error;
    bool	    	file_closed = FALSE;
    bool            	config_open = FALSE;
    i4         	setrelstat= 0;
    u_i4		relstat2 = 0;
    u_i4		IIRELIDXrelstat2 = 0;
    i4		modoptions = 0;
    i4		mod_options2 = 0;
    u_i4		db_sync_flag;
    i4		sysmod_tupadds;
    bool		upcase;
    DM2U_INDEX_CB	index_cb;
    DM_OBJECT           *mem_ptr = (DM_OBJECT *)0;
    i4             table_cnt = 0;
    i4             table_max = 0;
    u_i4                *tabids;
    u_i4                *cur_tabid;
    char                *recbuf;
    i4			oldpgtype;
    i4			oldpgsize;
    DM2U_MOD_CB		local_mcb, *mcb = &local_mcb;
    STATUS		cl_stat;
    i4			NumAtts;
    DMP_MISC		*AttMem = NULL;
    DMF_ATTR_ENTRY 	**AttList;
    DMF_ATTR_ENTRY	*AttEntry;
    DMF_ATTR_ENTRY     *AttlPtr;
    DB_ERROR		local_dberr;


    /*
    ** For recovery purposes, toss all pages from the cache prior to
    ** each core catalog sysmod.
    ** The last parameter is toss_modified = TRUE. 
    ** If the server crashes during SYSMOD, we won't need to perform
    ** REDO recovery with inconsistent core catalogs
    */
    dm0p_toss_pages(dcb->dcb_id, (i4)0, xcb->xcb_lk_id, (i4)0, 1);

    /* If we are doing iirelation, make sure that all the non-core TCB's
    ** are gone.  The TCB contains an iirelation tid and they will all
    ** become invalid if we're rewriting iirelation.  The core catalogs
    ** stay.  (iidevices does NOT stay, but if we're modifying iirelation,
    ** it's OK to whack iidevices' TCB.)
    */
    if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
    {
	status = dm2d_release_user_tcbs(dcb, xcb->xcb_lk_id,
			xcb->xcb_log_id, DM2T_TOSS, dberr);
	if (status != E_DB_OK)
	    return(status);
    }

    upcase = ((*xcb->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) != 0);

    for (i = 0; i < 3; i++)
	key_list[i] = &key_entry[i];
    
    r = 0;
    nr = 0;
    for (;;)
    {
	/*  Open up the table. */
	status = dm2t_open(dcb, tbl_id, DM2T_IX, 
            DM2T_UDIRECT, DM2T_A_WRITE,
	    (i4)0, (i4)20, (i4)0, xcb->xcb_log_id, 
            xcb->xcb_lk_id, (i4)0, (i4)0, db_lockmode, 
	    &xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;
	r = rcb;
	t = r->rcb_tcb_ptr;
	r->rcb_xcb_ptr = xcb;

	oldpgtype = t->tcb_table_io.tbio_page_type;
	oldpgsize = t->tcb_table_io.tbio_page_size; 

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;

	MEcopy((char *)&t->tcb_rel.relid, sizeof(DB_TAB_NAME),
               (char *)&modtab_name);

	MEmove (7, (upcase ? "IIRTEMP" : "iirtemp"), ' ',
		sizeof(DB_TAB_NAME), (char *)&newtab_name);

	MEcopy((char *)&t->tcb_rel.relowner, sizeof(DB_OWN_NAME),
               (char *)&owner);

	MEcopy((char *)&t->tcb_rel.relloc, sizeof(DB_LOC_NAME),
               (char *)&location);


	duplicates = (t->tcb_rel.relstat & TCB_DUPLICATES) != 0;
	journal = (t->tcb_rel.relstat & TCB_JOURNAL) != 0;
	structure = t->tcb_rel.relspec;
	ntab_width = t->tcb_rel.relwid;
	NumAtts= t->tcb_rel.relatts;
	kcount = t->tcb_rel.relkeys;
	unique = (t->tcb_rel.relstat & TCB_UNIQUE) != 0;
	merge = FALSE;
	truncate = 0;
	if (compressed)
	    setrelstat |= TCB_COMPRESSED;

	dm0l_jnlflag = (journal ? DM0L_JOURNAL : 0);
	
	if (journal)
	    setrelstat |= TCB_JOURNAL;
	
        if (duplicates)
	{
            setrelstat |= TCB_DUPLICATES;
	    mcb->mcb_modoptions |= DM2U_DUPLICATES;
	}

	/* for now, just use the relstat2 from the unmodified relation 
	** but do *not* copy the TCB2_PHYSLOCK_CONCUR flag otherwise
	** we end up treating iirtemp as a concur table and don't
	** journal changes to it - so a rollforward of a sysmod will
	** fail
	**/

	relstat2 = (t->tcb_rel.relstat2 & (~TCB2_PHYSLOCK_CONCUR));

	status = dm0m_allocate(sizeof(DMP_MISC) + (NumAtts *
			    (sizeof(DMF_ATTR_ENTRY*) + sizeof(DMF_ATTR_ENTRY))),
			    DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
			    (char*)NULL, (DM_OBJECT**)&AttMem, dberr);
	if ( status )
	    break;

	AttList = (DMF_ATTR_ENTRY**)((char*)AttMem + sizeof(DMP_MISC));
	AttMem->misc_data = (char*)AttList;
	AttEntry = (DMF_ATTR_ENTRY*)((char*)AttList + 
			(NumAtts * sizeof(DMF_ATTR_ENTRY*)));

	for ( i = 0; i < NumAtts; i++ )
	    AttList[i] = &AttEntry[i];

	for (i = 1; i <= t->tcb_rel.relatts; i++)
	{	
	    /* Load attribute descriptors here. */

	    att_ptr   = &t->tcb_atts_ptr[i];
	    /*	AttlPtr added to work-around VAX optimizer bug */
	    AttlPtr = AttList[i-1];

	    MEmove(att_ptr->attnmlen, att_ptr->attnmstr, ' ',
		DB_ATT_MAXNAME, AttlPtr->attr_name.db_att_name);
	    AttlPtr->attr_type  = att_ptr->type;	    
	    AttlPtr->attr_size  = att_ptr->length;	    
	    AttlPtr->attr_precision  = att_ptr->precision;	    
	    AttlPtr->attr_flags_mask = att_ptr->flag;	    
	    AttlPtr->attr_collID = att_ptr->collID;
	    AttlPtr->attr_geomtype = att_ptr->geomtype;
	    AttlPtr->attr_encflags = att_ptr->encflags;
	    AttlPtr->attr_encwid = att_ptr->encwid;
	    AttlPtr->attr_srid = att_ptr->srid;
	    COPY_DEFAULT_ID( att_ptr->defaultID, AttlPtr->attr_defaultID );
	}
	/*
	** This is hard coded to allow a Pre 2.0 database to OI2.0
	*/
	if ( tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID )
	    ntab_width = sizeof(DMP_ATTRIBUTE);

	for (i=0; i < t->tcb_keys; i++)
	{	
	    /* Load key descriptors here. */

	    att_ptr = t->tcb_key_atts[i];
	    MEmove(att_ptr->attnmlen, att_ptr->attnmstr, ' ',
		DB_ATT_MAXNAME, key_list[i]->key_attr_name.db_att_name);
	    key_list[i]->key_order = DM2U_ASCENDING;
	
	}
	/* Note, all the system sconcur tables except the iidevices
        ** table are always open, if the database is open.  The 
        ** code below works under the assumption the the table and 
        ** files are open.  So just leave the iidevices table open
        ** and close it later. 
        */
	dev_rcb = 0;
	if ( tbl_id->db_tab_base != DM_B_DEVICE_TAB_ID)
	{
	    status = dm2t_close(r, DM2T_NOPURGE, dberr);
	    if (status != E_DB_OK)
		break;
	}
	else 
	    dev_rcb = r;
	r = 0;

	/* Now create and modify the table. */

	status = 
	    dm2u_create(dcb, xcb, &newtab_name, &owner, &location, (i4)1,
		    &ntbl_id, &nidx_id, index, view,
		    setrelstat, relstat2,
		    structure, TCB_C_NONE, ntab_width, ntab_width,
		    NumAtts, AttList, db_lockmode,
		    allocation, extend, newpgtype, newpgsize,
		    (DB_QRY_ID *)0, (DM_PTR *)NULL,
		    (DM_DATA *)NULL, (i4)0, (i4)0,
		    (DMU_FROM_PATH_ENTRY *)NULL, (DM_DATA *)0, 
		    0, 0, (f8*)NULL, 0, NULL, 0, NULL /* DMU_CB */,
		    dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** If running on a checkpointed database that journaling has been
	    ** turned off on, then create will return with a warning telling
	    ** us that journaling will be enabled at the next checkpoint
	    ** (E_DM0101_SET_JOURNAL_ON).  Ignore this warning in sysmod.
	    */
	    if (dberr->err_code != E_DM0101_SET_JOURNAL_ON)
		break;
	    status = E_DB_OK;
	    CLRDBERR(dberr);
	}

	/* Prime the MCB for dm2u_modify: */
	mcb->mcb_dcb = dcb;
	mcb->mcb_xcb = xcb;
	mcb->mcb_tbl_id = &ntbl_id;
	mcb->mcb_omcb = (DM2U_OMCB*)NULL;
	mcb->mcb_dmu = (DMU_CB*)NULL;
	mcb->mcb_structure = structure;
	mcb->mcb_location = &location;
	mcb->mcb_l_count = 1;
	mcb->mcb_i_fill = i_fill;
	mcb->mcb_l_fill = l_fill;
	mcb->mcb_d_fill = d_fill;
	mcb->mcb_compressed = compressed;
	mcb->mcb_index_compressed = FALSE;
	mcb->mcb_temporary = FALSE;
	mcb->mcb_unique = unique;
	mcb->mcb_merge = merge;
	mcb->mcb_clustered = FALSE;
	mcb->mcb_min_pages = min_pages;
	mcb->mcb_max_pages = max_pages;
	mcb->mcb_modoptions = modoptions;
	mcb->mcb_mod_options2 = mod_options2;
	mcb->mcb_kcount = kcount;
	mcb->mcb_key = (DM2U_KEY_ENTRY**)key_list;
	mcb->mcb_db_lockmode = db_lockmode;
	mcb->mcb_allocation = allocation;
	mcb->mcb_extend = extend;
	mcb->mcb_page_type = newpgtype;
	mcb->mcb_page_size = newpgsize;
	mcb->mcb_tup_info = tup_cnt;
	mcb->mcb_reltups = reltups;
	mcb->mcb_tab_name = (DB_TAB_NAME*)NULL;
	mcb->mcb_tab_owner = (DB_OWN_NAME*)NULL;
	mcb->mcb_has_extensions = (i4*)NULL;
	mcb->mcb_relstat2 = relstat2;
	mcb->mcb_flagsmask = 0;
	mcb->mcb_tbl_pri = 0;
	mcb->mcb_rfp_entry = (DM2U_RFP_ENTRY*)NULL;
	mcb->mcb_new_part_def = (DB_PART_DEF*)NULL;
	mcb->mcb_new_partdef_size = 0;
	mcb->mcb_partitions = (DMU_PHYPART_CHAR*)NULL;
	mcb->mcb_nparts = 0;
	mcb->mcb_verify = 0;

	status = dm2u_modify(mcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*  If this is the relation table, place an index on it. */

	tblidx_id.db_tab_base = 0;
	tblidx_id.db_tab_index = 0;
	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	{
	    tblidx_id.db_tab_base = DM_B_RIDX_TAB_ID;
	    tblidx_id.db_tab_index = DM_I_RIDX_TAB_ID;

	    status = dm2t_open(dcb, &tblidx_id, DM2T_IX, 
		DM2T_UDIRECT, DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0, xcb->xcb_log_id, 
		xcb->xcb_lk_id, (i4)0, (i4)0, 
		db_lockmode, 
		&xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, dberr);
	    if (status != E_DB_OK)
		break;
	    r = rcb;
	    t = r->rcb_tcb_ptr;
	    r->rcb_xcb_ptr = xcb;

	    /*
	    ** Check for NOLOGGING - no updates should be written to the log
	    ** file if this session is running without logging.
	    */
	    if (xcb->xcb_flags & XCB_NOLOGGING)
		r->rcb_logging = 0;

	    MEcopy((char *)&t->tcb_rel.relid, sizeof(DB_TAB_NAME),
               (char *)&modidx_name);
	    MEmove (10, (upcase ? "IIRIDXTEMP" : "iiridxtemp"), ' ',
		    sizeof(DB_TAB_NAME), (char *)&newidx_name);
	    MEcopy((char *)&t->tcb_rel.relowner, sizeof(DB_OWN_NAME),
               (char *)&owner);
	    MEcopy((char *)&t->tcb_rel.relloc, sizeof(DB_LOC_NAME),
               (char *)&location);

	    structure = t->tcb_rel.relspec;
	    ntab_width = t->tcb_rel.relwid;
	    kcount = t->tcb_rel.relkeys;
	    merge = 0;
	    truncate = 0;
	    unique = 0;
	
	    for (i=0; i < t->tcb_keys; i++)
	    {	
		/* Load key descriptors here. */

		att_ptr = t->tcb_key_atts[i];
		MEmove(att_ptr->attnmlen, att_ptr->attnmstr, ' ',
		    DB_ATT_MAXNAME, key_list[i]->key_attr_name.db_att_name);
		key_list[i]->key_order = DM2U_ASCENDING;
	
	    }
	    status = dm2t_close(r, DM2T_NOPURGE, dberr);
	    if (status != E_DB_OK)
		break;
	    r = 0;

	    /* Now create the index.  */
            /* schang (04/16/92) add 0 as gw_id at end of arg list */
	    index_cb.q_next = (DM2U_INDEX_CB *) 0;
	    index_cb.q_prev = (DM2U_INDEX_CB *) 0;
	    index_cb.length = sizeof(DM2U_INDEX_CB);
	    index_cb.type = DM2U_IND_CB;
	    index_cb.s_reserved = 0;
	    index_cb.owner = 0;
	    index_cb.ascii_id = DM2U_IND_ASCII_ID;
	    index_cb.indxcb_dcb = dcb;
	    index_cb.indxcb_xcb = xcb;
	    index_cb.indxcb_index_name = &newidx_name;
	    index_cb.indxcb_location = &location;
	    index_cb.indxcb_l_count = 1;
	    index_cb.indxcb_tbl_id = &ntbl_id;
	    index_cb.indxcb_structure = structure;
	    index_cb.indxcb_i_fill = i_fill;
	    index_cb.indxcb_l_fill = l_fill;
	    index_cb.indxcb_d_fill = d_fill;
	    index_cb.indxcb_compressed = compressed;
	    index_cb.indxcb_index_compressed = DM1CX_UNCOMPRESSED;
	    index_cb.indxcb_unique = unique;
	    index_cb.indxcb_dmveredo = FALSE;
	    index_cb.indxcb_min_pages = min_pages;
	    index_cb.indxcb_max_pages = max_pages;
	    index_cb.indxcb_kcount = kcount;
	    index_cb.indxcb_key = (DM2U_KEY_ENTRY **)key_list;
	    index_cb.indxcb_acount = kcount;
	    index_cb.indxcb_db_lockmode = db_lockmode;
	    index_cb.indxcb_allocation = allocation;
	    index_cb.indxcb_extend = extend;
	    index_cb.indxcb_page_type = newpgtype;
	    index_cb.indxcb_page_size = newpgsize;
	    index_cb.indxcb_idx_id = &nidx_id;
	    index_cb.indxcb_range = (f8 *) NULL;
	    index_cb.indxcb_index_flags = (i4)0;
	    index_cb.indxcb_gwattr_array = (DM_PTR *)NULL;
	    index_cb.indxcb_gwchar_array = (DM_DATA *)NULL;
	    index_cb.indxcb_gwsource = (DMU_FROM_PATH_ENTRY *)NULL;
	    index_cb.indxcb_qry_id = (DB_QRY_ID *)NULL;
	    index_cb.indxcb_tup_info = tup_cnt;
	    index_cb.indxcb_tab_name = 0;
	    index_cb.indxcb_tab_owner = 0;
	    index_cb.indxcb_reltups = reltups;
	    index_cb.indxcb_gw_id = 0;
	    index_cb.indxcb_char_array = (DM_DATA *)0;
	    index_cb.indxcb_relstat2 = IIRELIDXrelstat2;
	    index_cb.indxcb_tbl_pri = 0;
	    index_cb.indxcb_errcb = dberr;
	    status = dm2u_index(&index_cb);

	    if (status != E_DB_OK)
		break;
	} /* end if relation table. */

	/* Now that you have created a new table, fill it. */

	/*  Open up the table. */

	status = dm2t_open(dcb, tbl_id, DM2T_IX, 
            DM2T_UDIRECT, DM2T_A_WRITE,
	    (i4)0, (i4)20, (i4)0, xcb->xcb_log_id, 
            xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, 
	    dberr);
	if (status != E_DB_OK)
	    break;
	r = rcb;
	t = r->rcb_tcb_ptr;
	r->rcb_xcb_ptr = xcb;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;

	/*  Open up the table. */

	status = dm2t_open(dcb, &ntbl_id, DM2T_X, 
            DM2T_UDIRECT, DM2T_A_WRITE,
	    (i4)0, (i4)20, (i4)0, xcb->xcb_log_id, 
            xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0,
	    dberr);
	if (status != E_DB_OK)
	    break;
	nr = rcb;
	nt = nr->rcb_tcb_ptr;
	nr->rcb_xcb_ptr = xcb;
		
	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    nr->rcb_logging = 0;

	status = dm2r_position(r, DM2R_ALL, 
                 (DM2R_KEY_DESC *)0, (i4)0,
                 (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	sysmod_tupadds = 0;

	if(r->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base == DM_1_RELATION_KEY &&
	   r->rcb_tcb_ptr->tcb_rel.reltid.db_tab_index == 0)
        {
	    nr->rcb_temp_iirelation = 1;
	}
	else
	{
	    nr->rcb_temp_iirelation = 0;
	}

	for (;;)
	{
	    /* Check for interrupts in this loop. */
	    if ( status = dm2u_check_for_interrupt(xcb, dberr) )
		break;

	    status = dm2r_get(r, &tid, DM2R_GETNEXT, 
                              (char *)&rel_record, dberr);
	    if (status != E_DB_OK)
		break;
            
	    /* Bug 107828 - flag whether the associated user table
	    ** is journaled or not.
	    */
	    if(rel_record.relstat & TCB_JOURNAL)
	    {
		nr->rcb_usertab_jnl = 1;
	    }
	    else
	    {
		nr->rcb_usertab_jnl = 0;
	    }

	    status = dm2r_put(nr, DM2R_NODUPLICATES, 
                         (char *)&rel_record, dberr);
	    if (status != E_DB_OK)
		break;
	    if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	    {
		/* We're rewriting iirelation, fix up the tid in the
		** iiindex or iiattribute TCB.  The iidevices TCB was
		** tossed at the beginning.  The iirelation and iirelidx
		** TCB's will be fixed later after we fix up the new
		** iirelation row.
		*/
		if (rel_record.reltid.db_tab_base == DM_B_INDEX_TAB_ID)
		    STRUCT_ASSIGN_MACRO(nr->rcb_currenttid,
				dcb->dcb_idx_tcb_ptr->tcb_iirel_tid);
		else if (rel_record.reltid.db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
		    STRUCT_ASSIGN_MACRO(nr->rcb_currenttid,
				dcb->dcb_att_tcb_ptr->tcb_iirel_tid);
	    }

	    sysmod_tupadds++;
	}
	if (status == E_DB_ERROR && dberr->err_code != E_DM0055_NONEXT)
	    break;
	CLRDBERR(dberr);

	status = dm2t_close(r, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;
	status = dm2t_close(nr, DM2T_PURGE, dberr);
	if (status != E_DB_OK)
	    break;
	nr = 0;

	/* If modifying anything other than the relation table
        ** update the relation records in the old relation table,
        ** otherwise update the relation records in the new relation
        ** table.
	*/

	reltbl_id.db_tab_base = DM_B_RELATION_TAB_ID;
	reltbl_id.db_tab_index = DM_I_RELATION_TAB_ID;
	lock_mode = DM2T_IX;
	purge_mode = DM2T_NOPURGE;
	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	{
	    reltbl_id.db_tab_base = ntbl_id.db_tab_base;
	    reltbl_id.db_tab_index = ntbl_id.db_tab_index;
	    lock_mode = DM2T_X;
	    purge_mode = DM2T_PURGE;
	}
	/*  Open up the old or new relation table. */

	status = dm2t_open(dcb, &reltbl_id, lock_mode, 
            DM2T_UDIRECT, DM2T_A_WRITE,
	    (i4)0, (i4)20, (i4)0, xcb->xcb_log_id, 
            xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, 
	    dberr);
	if (status != E_DB_OK)
	    break;
	r_upt = rcb;
	t = r_upt->rcb_tcb_ptr;
	rcb->rcb_xcb_ptr = xcb;
	r_upt->rcb_usertab_jnl = (journal) ? 1 : 0;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    r_upt->rcb_logging = 0;

	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *) &ntbl_id.db_tab_base;
	qual_list[1].attr_number = DM_2_ATTRIBUTE_KEY;
	qual_list[1].attr_operator = DM2R_EQ;
	qual_list[1].attr_value = (char *) &ntbl_id.db_tab_index;
	
	status = dm2r_position(r_upt, DM2R_QUAL, qual_list, 
                         (i4)1,
                         (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(r_upt, &tid, DM2R_GETNEXT, 
                             (char *)&rel_record, dberr);
	    if (status == E_DB_OK)
	    {
		if (rel_record.reltid.db_tab_base == 
                                ntbl_id.db_tab_base)
		{
		    if ( rel_record.relstat & TCB_INDEX )
			STRUCT_ASSIGN_MACRO(rel_record, save_ridx);
		    else
			STRUCT_ASSIGN_MACRO(rel_record, save_rel);
		}
		continue;
	    }

	    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
	    {
		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
	    break;
	}
	if (status != E_DB_OK)
	    break;

	qual_list[0].attr_value = (char*)&tbl_id->db_tab_base;
	qual_list[1].attr_value = (char *)&tbl_id->db_tab_index;
	    
	status = dm2r_position(r_upt, DM2R_QUAL, qual_list, 
                         (i4)1,
                         (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(r_upt, &tid, DM2R_GETNEXT, 
                             (char *)&rel_record, dberr);

	    if (status == E_DB_OK)
	    {
		if (rel_record.reltid.db_tab_base == 
                                tbl_id->db_tab_base)
		{
		    /* Save old relation record for logging. */
		    if ( rel_record.relstat & TCB_INDEX )
		    {
			new_rel = &save_ridx;
			STRUCT_ASSIGN_MACRO(rel_record, old_ridx);
		    }			
		    else
		    {
			new_rel = &save_rel;
			STRUCT_ASSIGN_MACRO(rel_record, old_rel);
		    }			
		    rel_record.relspec = TCB_HASH;
		    rel_record.relpages = new_rel->relpages;
		    rel_record.reltups = sysmod_tupadds;
		    rel_record.relprim = new_rel->relprim;
		    rel_record.relmain = new_rel->relmain;
                    rel_record.relmoddate = new_rel->relmoddate;
                    rel_record.relifill = new_rel->relifill;
                    rel_record.reldfill = new_rel->reldfill;
                    rel_record.rellfill = new_rel->rellfill;
                    rel_record.relmin = new_rel->relmin;
                    rel_record.relmax = new_rel->relmax;
		    rel_record.relcmptlvl = new_rel->relcmptlvl;
                    rel_record.relfhdr = new_rel->relfhdr;
		    rel_record.relallocation = new_rel->relallocation;
		    rel_record.relextend = new_rel->relextend;
                    rel_record.relcomptype = new_rel->relcomptype;
                    rel_record.relpgtype = new_rel->relpgtype;
		    rel_record.relpgsize = new_rel->relpgsize;

		    /* Save new relation record for logging. */

		    if (new_rel == &save_ridx)
			STRUCT_ASSIGN_MACRO(rel_record, save_ridx);
		    else
			STRUCT_ASSIGN_MACRO(rel_record, save_rel);
		    status = dm2r_replace(r_upt, &tid,
			DM2R_BYPOSITION | DM2R_RETURNTID, 
                        (char *)&rel_record, (char *)0, dberr);
		    if (status != E_DB_OK)
			break;
		    /* Save what is or will be the new iirelation tid
		    ** for fixing up the modified table's tcb.  This is how
		    ** the TCB tid gets fixed if we're modifying iirelation
		    ** itself.  This also allows for any (unlikely) row
		    ** motion caused by the replace, if we're modifying one
		    ** of the other core catalogs.  cat[x]_tid is stored
		    ** into the proper TCB a bit further on.
		    */
		    if (new_rel == &save_ridx)
			STRUCT_ASSIGN_MACRO(tid, catx_tid);
		    else
			STRUCT_ASSIGN_MACRO(tid, cat_tid);

		}
		continue;
	    }
	    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
	    {
		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
	    break;
	}
	if (status != E_DB_OK)
	    break;
    	status = dm2t_close(r_upt, purge_mode, dberr);
	if (status != E_DB_OK)
	    break;
	r_upt = 0;

	/*
	** Unless logging is disabled, log the record which indicates that
	** the load of iirtemp has been completed and iirtemp has been closed
	** and purged. This record is used only by rollforward currently.
	** Rollforward uses this record to know when iirtemp is about to be
	** swapped with the core catalog it replaces, so that rollforward
	** can close and purge the iirtemp table.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_sm0_closepurge(xcb->xcb_log_id, dm0l_jnlflag,
		&ntbl_id, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/* Get pointer to the system table tcb. */

	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	    t = dcb->dcb_rel_tcb_ptr;
	else if (tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
	    t = dcb->dcb_att_tcb_ptr;
	else if (tbl_id->db_tab_base == DM_B_INDEX_TAB_ID)
	    t = dcb->dcb_idx_tcb_ptr;
	else if (tbl_id->db_tab_base == DM_B_DEVICE_TAB_ID)
	    t = dev_rcb->rcb_tcb_ptr;

	STRUCT_ASSIGN_MACRO(*tbl_id, oldtab_id);
	STRUCT_ASSIGN_MACRO(ntbl_id, newtab_id);
	
	/* Now mark the configuration file to indicate
        ** that system catalog files are in an inconsistent
        ** state since they are about to be renamed. 
        */

	/*  Open the configuration file. */

	status = dm0c_open(dcb,DM0C_PARTIAL, xcb->xcb_lk_id, &config, dberr);
	if (status != E_DB_OK)
	    break;
	config_open = TRUE;

	config->cnf_dsc->dsc_status |= DSC_SM_INCONSISTENT;

	status = dm0c_close(config, DM0C_UPDATE | DM0C_LEAVE_OPEN, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    /* Go through this loop for the modified base table TCB and
	    ** all of its indexes.  Actually it only works for a single
	    ** index, which would be iirel_idx.  If any core catalog gets
	    ** additional indexes, this loop has to be fixed up.
	    */

	    /* Now log all the special stuff for renaming the old file to
	    ** <filename>.del, close at a low level the old file, 
	    ** renaming the new file from <newfilename>.tab <filename>.tab     
	    ** opening the file <filename>.tab which is now the new one,
	    ** updateing the TCB for the table modified, and updating
	    ** the configuration file for the table modified.	    
	    */

	    /* Flush any pages left in buffer cache for this table. */

	    status = dm0p_close_page(t, xcb->xcb_lk_id, xcb->xcb_log_id,
					DM0P_CLOSE, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Close the file associated with this table. */
	
	    f = t->tcb_table_io.tbio_location_array[0].loc_fcb;
	    phys_location = (DM_PATH*)&f->fcb_location->physical;
	    phys_length = f->fcb_location->phys_length;
	    if (f->fcb_state & FCB_OPEN)
	    {
		status = dm2f_close_file(t->tcb_table_io.tbio_location_array,
			t->tcb_table_io.tbio_loc_count, (DM_OBJECT *)f, dberr); 
		if (status != E_DB_OK)
		    break;
		file_closed = TRUE;
	    }

	    /* Rename the file associated with this table so it will
	    ** be deleted at end transaction.
	    */

	    /* Construct the the file name and pathname. */

	    dm2f_filename(DM2F_TAB, &oldtab_id, 0,0,0, &old1tabname);
	    dm2f_filename(DM2F_SM1, &oldtab_id, 0,0,0, &new1tabname);
	    filelength = sizeof(DM_FILE);
	    oldfilelength = sizeof(DM_FILE);

	    /* Save the new file name of old file, need it later. */

	    MEcopy((char *)&new1tabname, 
               sizeof(DM_FILE), (char *)&savefilename);


	    MEcopy((char *)&old1tabname, 
               sizeof(DM_FILE), (char *)&new2tabname);
	
	    dm2f_filename(DM2F_TAB, &newtab_id, 0,0,0, &old2tabname);
	    filelength = sizeof(DM_FILE);
	    oldfilelength = sizeof(DM_FILE);

	    /*
	    ** Log the sysmod rename operation - unless logging is disabled.
	    */
	    if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	    {
		status = dm0l_sm1_rename(xcb->xcb_log_id, dm0l_jnlflag,
		    &oldtab_id, &newtab_id,phys_location, phys_length, 
		    &old1tabname, &new1tabname, &old2tabname,
		    &new2tabname, (LG_LSN *)0, &lsn, 
		    oldpgtype, newpgtype, oldpgsize, newpgsize, dberr); 
		if (status != E_DB_OK)
		    break;
	    }

	    /* Rename the old system catalog to .tmp */

	    status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id,
		phys_location, phys_length, &old1tabname, oldfilelength,
		&new1tabname, filelength, &dcb->dcb_name, dberr);
	    if (status != E_DB_OK)
		break;


	    /* Rename the new system catalog to same file name as old one. */
    
	    status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id,
		phys_location, phys_length, &old2tabname, oldfilelength,
		&new2tabname, filelength, &dcb->dcb_name, dberr);
	    if (status != E_DB_OK)
		break;


	    /* Now open the file just renamed.
	    ** Select file synchronization mode, according to the mode
	    ** style for normal table opens.  (This is NOT a load-file open.)
	    ** i.e. do what dm2f_build_fcb does for table opens.
	    */
	    if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
		db_sync_flag = (u_i4) 0;
	    else 
	    {
		db_sync_flag = (u_i4) DI_SYNC_MASK;
		if (dmf_svcb->svcb_directio_tables)
		    db_sync_flag |= DI_DIRECTIO_MASK;
	    }

	    status = dm2f_open_file(xcb->xcb_lk_id, xcb->xcb_log_id,
		t->tcb_table_io.tbio_location_array, 
		t->tcb_table_io.tbio_loc_count,
		newpgsize, DM2F_A_WRITE, 
		db_sync_flag, (DM_OBJECT *)f, dberr);
	    if (status != E_DB_OK)
		break;
	    file_closed = FALSE;

	    /* Update page size (in the DCB system table tcb) */
	    if (newpgsize != oldpgsize)
	    {
		/* Update hcb_cache_tcbcount for base and index TCBs */
		dm0s_mlock(&dmf_svcb->svcb_hcb_ptr->hcb_tq_mutex);
		dmf_svcb->svcb_hcb_ptr->hcb_cache_tcbcount[DM_CACHE_IX(oldpgsize)]--;
		dmf_svcb->svcb_hcb_ptr->hcb_cache_tcbcount[DM_CACHE_IX(newpgsize)]++;
		dm0s_munlock(&dmf_svcb->svcb_hcb_ptr->hcb_tq_mutex);
		t->tcb_table_io.tbio_page_size = newpgsize;
		t->tcb_table_io.tbio_cache_ix = DM_CACHE_IX(newpgsize);
	    }

	    if (newpgtype != oldpgtype)
		t->tcb_table_io.tbio_page_type = newpgtype;

	    /* To finish the swap of files from file associated 
	    ** with old relation to file associated with new
	    ** relation and vice versa, just renamed the intermediate
	    ** file to newfilename.  Note for logging this 
	    ** is just a normal rename.
	    */

	    /*
	    ** Log the file rename operation - unless logging is disabled.
	    */
	    if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	    {
		/*
		** loc_id for the root location is by definition zero.
		** The loc_config_id may or may not be zero.
		*/
		status = dm0l_frename(xcb->xcb_log_id, 
		    dm0l_jnlflag, phys_location, phys_length, &savefilename, 
		    &old2tabname, &oldtab_id, 0,
		    t->tcb_table_io.tbio_location_array[0].loc_config_id,
		    (LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    filelength = sizeof(DM_FILE);
	    oldfilelength = sizeof(DM_FILE);

	    /* Rename the file. */

	    status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id,
		phys_location, phys_length, &savefilename, oldfilelength,
		&old2tabname, filelength, &dcb->dcb_name, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Update the tcb to have new information. */

	    if (t->tcb_rel.reltid.db_tab_index == 0)
	    {
		STRUCT_ASSIGN_MACRO(save_rel, t->tcb_rel);
		STRUCT_ASSIGN_MACRO(cat_tid, t->tcb_iirel_tid);
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(save_ridx, t->tcb_rel);
		STRUCT_ASSIGN_MACRO(catx_tid, t->tcb_iirel_tid);
	    }

	    /*
	    ** Invalidate tbio_lpageno, tbio_lalloc so they get
	    ** re-initialized correctly (B78889, B52131, B57674)
	    */
	    t->tcb_table_io.tbio_lpageno = -1;
	    t->tcb_table_io.tbio_lalloc = -1;
	    t->tcb_table_io.tbio_checkeof = TRUE;

	    /* Update the set of accessors (may have changed).
	    ** Don't need to reset rowaccessors, compression never changes
	    ** for core catalogs, nor do we ever add/drop/alter columns.
	    */
	    dm1c_get_plv(newpgtype, &t->tcb_acc_plv);

	    /* If the relation table was one modified, then
	    ** must repeat everything for index. 
	    */

	    if ((t->tcb_rel.reltid.db_tab_base == DM_B_RELATION_TAB_ID) &&
		(t->tcb_rel.reltid.db_tab_index == DM_I_RELATION_TAB_ID))
	    {
		t = t->tcb_iq_next;
		oldtab_id.db_tab_base = tblidx_id.db_tab_base;
		oldtab_id.db_tab_index = tblidx_id.db_tab_index;
		newtab_id.db_tab_base = nidx_id.db_tab_base;
		newtab_id.db_tab_index = nidx_id.db_tab_index;
	    }
	    else
	    {
		break;
	    }
	}/* end for close, rename, rename, open loop */
	if (status != E_DB_OK)
	    break;

	config->cnf_dsc->dsc_status &= ~DSC_SM_INCONSISTENT;

	status = dm0c_close(config, DM0C_UPDATE | DM0C_LEAVE_OPEN, dberr);
	if (status != E_DB_OK)
	    break;

	/* Now we are ready to update the configuration file. */

	/*
	** Log the sysmod operation - unless logging is disabled.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_sm2_config(xcb->xcb_log_id, dm0l_jnlflag,
		tbl_id, &old_rel, &save_rel, &old_ridx, &save_ridx,   
		(LG_LSN *)0, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}
	
	/* 
	** Update the configuration file record. We need to update the config
	** file only if iirelation was modified and that too only if relprim
	** changed.
	*/

	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	{
	    config->cnf_dsc->dsc_iirel_relprim = save_rel.relprim;
	}
	if (newpgsize != oldpgsize)
	{
	    if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
		config->cnf_dsc->dsc_iirel_relpgsize = newpgsize;
	    else if (tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID)	
		config->cnf_dsc->dsc_iiatt_relpgsize = newpgsize;
	    else if (tbl_id->db_tab_base == DM_B_INDEX_TAB_ID)
		config->cnf_dsc->dsc_iiind_relpgsize = newpgsize;
	}
	if (newpgtype != oldpgtype)
	{
	    if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
		config->cnf_dsc->dsc_iirel_relpgtype = newpgtype;
	    else if (tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID)	
		config->cnf_dsc->dsc_iiatt_relpgtype = newpgtype;
	    else if (tbl_id->db_tab_base == DM_B_INDEX_TAB_ID)
		config->cnf_dsc->dsc_iiind_relpgtype = newpgtype;
	}
	    
	/* Update the configuration file and make backup copy. */

	status = dm0c_close(config, DM0C_UPDATE | DM0C_COPY, dberr);
	if (status != E_DB_OK)
	    break;
	config_open = FALSE;

	/*  Destroy new table. This also destroys index if one existed. */
	status = dm2u_destroy(dcb, xcb, &ntbl_id, DM2T_X,
	    DM2U_MODIFY_REQUEST, 0, 0, 0, 0, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Log the DMU operation.  This marks a spot in the log file to
	** which we can only execute rollback recovery once.  If we now
	** issue update statements against the newly-modified table, we
	** cannot do abort processing for those statements once we have
	** begun backing out the modify.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_dmu(xcb->xcb_log_id, dm0l_jnlflag, tbl_id, 
		&save_rel.relid, &save_rel.relowner, (i4)DM0LMODIFY, 
		(LG_LSN *) 0, dberr);
	    if (status != E_DB_OK)
		break;
	}

	if (dev_rcb)
	{
	    status = dm2t_close(dev_rcb, DM2T_NOPURGE, dberr);
	    if (status != E_DB_OK)
		break;
	    dev_rcb = 0;
	}

	if ( AttMem )
	    dm0m_deallocate((DM_OBJECT**)&AttMem);

	return (E_DB_OK);

    } /* end outer for. */
    
    /*	Handle cleanup for error recovery. */

    if ( AttMem )
	dm0m_deallocate((DM_OBJECT**)&AttMem);

    /*
    ** If sysmod failed and the system catalog physical file was left closed
    ** then reopen it since the server depends on the system catalogs always
    ** being opened.
    */
    if (file_closed)
    {
	/*
	** In this error case, must be reopened in normal (not user sync) mode.
	*/
	if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
	    db_sync_flag = (u_i4) 0;
	else
	{
	    db_sync_flag = (u_i4) DI_SYNC_MASK;
	    if (dmf_svcb->svcb_directio_tables)
		db_sync_flag |= DI_DIRECTIO_MASK;
	}

	local_status = dm2f_open_file(xcb->xcb_lk_id, xcb->xcb_log_id,
	    t->tcb_table_io.tbio_location_array, t->tcb_table_io.tbio_loc_count,
	    (i4)t->tcb_table_io.tbio_page_size, DM2F_A_WRITE, 
	    db_sync_flag, (DM_OBJECT *)f, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    dm0m_deallocate((DM_OBJECT **)&f);
	}
    }
    if (config_open)
	local_status = dm0c_close(config, DM0C_UPDATE, &local_dberr);
    if (r)
	local_status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);
    if (nr)
	local_status = dm2t_close(nr, DM2T_NOPURGE, &local_dberr);
    if (dev_rcb)
	local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);

    return (E_DB_ERROR);
}

/*{
** Name: dm2u_reorg_table - Reorganize a table to different number of locations.
**
** Description:
**      This routine is called from dm2u_modify to reorganize a table to 
**      a different set of locations.
**
**	At the start of this routine the temporary modify files into which
**	are copied the table pages must have been created and opened
**	by the caller.  They are described by the tpcb_locations array and
**	the open FCBs should be stored therein.
**
**	Upon exit, these files will have been flushed and forced but will
**	be left open.
**
** Inputs:
**      mxcb                             Modify control block. 
** Outputs:
**      error                            The reason for an error return status.
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
**	02-Mar-2004 (jenjo02)
**	    Modified to do the reorg in parallel when there are
**	    multiple partitions being reorganized.
**	    Moved the guts to DM2UReorg().
*/
DB_STATUS
dm2u_reorg_table(
DM2U_MXCB	    *m,
DB_ERROR	    *dberr)
{
    DM2U_SPCB		*sp;
    DB_STATUS		status = E_DB_OK;
    i4			error;

    /*
    ** There must be a 1-1 correspondence between
    ** sources (SPCBs) and targets (TCBS)...
    */

    /* Do in parallel, if multiple Sources */
    if ( m->mx_spcb_threads = SpawnDMUThreads(m, SOURCE_THREADS) )
    {
	/* Signal the Sources to go to work */
	CSp_semaphore(1, &m->mx_cond_sem);

	m->mx_state |= MX_REORG;

	CScnd_broadcast(&m->mx_cond);

	/* Wait for them to end */
	while ( m->mx_spcb_threads )
	    CScnd_wait(&m->mx_cond, &m->mx_cond_sem);

	/* The collective status */
	status = m->mx_status;
	if ( status )
	    *dberr = m->mx_dberr;

	CSv_semaphore(&m->mx_cond_sem);
	CScnd_free(&m->mx_cond);
	CSr_semaphore(&m->mx_cond_sem);
    }
    else
    {
	for ( sp = m->mx_spcb_next; 
	      status == E_DB_OK && sp; 
	      sp = sp->spcb_next )
	{ 
	    status = DM2UReorg(sp, dberr);
	}
    }

    return(status);
}

/*{
** Name: DM2UReorg - Reorganize a table to different number of locations.
**
** Description:
**      This routine is called from dm2u_modify to reorganize a table to 
**      a different set of locations.
**	
**	It may be called directly from dm2u_reorg_table, above,
**	or from a DMUSource thread.
**
**	At the start of this routine the temporary modify files into which
**	are copied the table pages must have been created and opened
**	by the caller.  They are described by the tpcb_locations array and
**	the open FCBs should be stored therein.
**
**	Upon exit, these files will have been flushed and forced but will
**	be left open.
**
** Inputs:
**      sp                               This Source's SPCB. 
** Outputs:
**      error                            The reason for an error return status.
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
**      08-jan-87 (jml)
**          Created for Jupiter.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file(), dm2f_build_fcb(),
**	    and dm2f_create_file() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then don't write any log records.
**	    25-mar-91 (rogerk)
**		Fixed bug in Set Nologging changes - test for rollforward before
**		testing xcb status as rollforward does not provide xcb's.
**	    30-aug-1991 (rogerk)
**		Fixes for rollforward of reorganize operations.
**		Changed to not depend on the existence of xcb information:
**		  - Don't attempt to fill in mx_lk_id from the xcb_lk_id value,
**		    this information has already been filled in by the caller.
**		  - If running rollforward, then don't use the xcb_dmu_cnt to
**		    count dmu statements, since this is used only for backout
**		    information, and we don't have to worry about statement
**		    counts during rollforward.
**		  - Don't check interrupt state during rollforward.
**		Fixed error handling when an error is encountered while in
**		read/write loop.  We were overwriting the status value from the
**		failed read/write with the dm2f_close_file status.  This caused
**		us to forget that an error occured and leave the table
**		incomplete.  Used local_status and local_err_code when closing
**		location files.
**      10-Feb-1992 (rmuth)
**          Call dm2f_file_size() to work out the allocation to log in
**          fcreate log record.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Added accessor parameter to dm2f_write_file call.
**	02-nov-1992 (jnash)
**	    Reduced logging project.  Add new params to dm0l_fcreate.
**	14-dec-1992 (rogerk)
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Added tabid and location id parameters to dm0l_fcreate call.
**	23-aug-1993 (rogerk)
**	    Removed file creation from the dm2u_reorg_table routine.  It is
**	    now the responsibility of the caller to create the files used
**	    in the load process.
**	20-sep-1993 (rogerk)
**	    Take out references to the dmu count in the XCB as they are neither
**	    unsed nor needed.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open
**	    temp files in sync'd databases DI_USER_SYNC_MASK.  The force 
**	    is done by the calling routine.
**	06-mar-1996 (stial01 for bryanp)
**	    Use tcb_rel.relpgsize to compute this table's page size.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb(), dm2f_read_file()
**	28-Feb-2004 (jenjo02)
**	    Added support for multiple partitions.
**	02-Mar-2004 (jenjo02)
**	    Cut from dm2u_reorg_table() to be run either
**	    locally or on behalf of a DMUSource thread.
**	11-Nov-2005 (jenjo02)
**	    Check for external interrupts via XCB when available.
**	23-Jul-2008 (kschendel)
**	    Ask for direct IO reading if config says so.  Assure proper
**	    buffer alignment.
*/
DB_STATUS
DM2UReorg(
DM2U_SPCB	*sp,
DB_ERROR	*dberr)
{
    DM2U_MXCB		*m = sp->spcb_mxcb;
    DM2U_TPCB		*tp;
    DMP_TCB             *t;
    DMP_DCB             *dcb = m->mx_dcb;
    i4			i, k, n;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status;
    i4			local_err_code;
    DMPP_PAGE           *buffer;
    i4			endpage, page;
    DB_TAB_NAME		table_name;
    DMP_MISC		*buf_cb = (DMP_MISC *)0;
    DMP_LOCATION	*loc;
    DML_XCB		*xcb = m->mx_xcb;
    LG_LSN		lsn;
    u_i4		db_sync_flag;
    u_i4		directio_align = dmf_svcb->svcb_directio_align;
    u_i4		size;
    DB_TAB_TIMESTAMP	timestamp;
    i4			error;
    DB_ERROR		local_dberr;

    for (;;)
    {
	/*
	** The Target's TPCB will correspond to the
	** n-th SPCB.
	*/
	tp = &m->mx_tpcb_next[sp - m->mx_spcb_next];

	CLRDBERR(dberr);

	/*
	** Open the source table (partition) if not already.
	**
	** If a single source, dm2u_modify will have already
	** opened this table and we'll leave it open.
	*/
	if ( sp->spcb_rcb == (DMP_RCB*)NULL )
	{
	    status = dm2t_open(dcb,
		     &sp->spcb_tabid,
		     m->mx_tbl_lk_mode, DM2T_UDIRECT,
		     m->mx_tbl_access_mode, 
		     m->mx_timeout,
		     (i4)20, (i4)0, 
		     sp->spcb_log_id,
		     sp->spcb_lk_id,
		     (i4)0, (i4)0,
		     m->mx_db_lk_mode,
		     m->mx_tranid,
		     &timestamp, 
		     &sp->spcb_rcb, 
		     (DML_SCB *)0, dberr);
	    if ( status == E_DB_OK )
	    {
		sp->spcb_rcb->rcb_xcb_ptr = xcb;
		sp->spcb_partno = sp->spcb_rcb->rcb_tcb_ptr->tcb_partno;
		sp->spcb_rcb->rcb_uiptr = m->mx_rcb->rcb_uiptr;
	    }
	}

	if ( status )
	    break;

	/* The (partition's) TCB */
	t = sp->spcb_rcb->rcb_tcb_ptr;

	/* Note that there will be no XCB when running Rollforward */

	MEcopy((char *)&t->tcb_rel.relid, sizeof(DB_TAB_NAME),
	   (char *)&table_name);

	/* Allocate space for pages to be read */

	if (dmf_svcb->svcb_rawdb_count > 0 && directio_align < DI_RAWIO_ALIGN)
	    directio_align = DI_RAWIO_ALIGN;
	size = (t->tcb_rel.relpgsize * DI_EXTENT_SIZE) + sizeof(DMP_MISC);
	if (directio_align != 0)
	    size += directio_align;
	status = dm0m_allocate(
			size,
			DM0M_ZERO, 
			(i4)MISC_CB,
			(i4)MISC_ASCII_ID, (char *)0, 
			(DM_OBJECT **)&buf_cb, 
			dberr);

	if ( status )
	    break;

	buffer = (DMPP_PAGE *)((char *)buf_cb + sizeof(DMP_MISC));
	if (directio_align != 0)
	    buffer = (DMPP_PAGE *) ME_ALIGN_MACRO(buffer, directio_align);
	buf_cb->misc_data = (char *)buffer;

	/*
	** Allocate FCBs and open files of the current table from which
	** to read pages. The new files have been opened by the caller 
	** of this routine.
	*/
	if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
	    db_sync_flag = (u_i4) 0;
	else 
	{
	    db_sync_flag = (u_i4) DI_SYNC_MASK;
	    if (dmf_svcb->svcb_directio_tables)
		db_sync_flag |= DI_DIRECTIO_MASK;
	}

	/* Pass the table's tbio so that sense will work properly for RAW */
	status = dm2f_build_fcb(sp->spcb_lk_id, sp->spcb_log_id,
				sp->spcb_locations, 
				sp->spcb_loc_count, 
				m->mx_page_size, DM2F_ALLOC,
				DM2F_TAB, &sp->spcb_tabid, 0, 0,
				&t->tcb_table_io, 0, 
				dberr);
	if ( status )
	    break;

	status = dm2f_open_file(sp->spcb_lk_id, sp->spcb_log_id,
				sp->spcb_locations, 
				sp->spcb_loc_count, 
				(i4)m->mx_page_size, DM2F_A_WRITE, 
				db_sync_flag,
				(DM_OBJECT *)sp->spcb_rcb, dberr);
	if ( status )
	    break;

	/* Found out how many pages are allocated in old file. */

	status = dm2f_sense_file(sp->spcb_locations, 
				 sp->spcb_loc_count, 
				 &table_name, &dcb->dcb_name, 
				 &endpage, dberr); 
	if ( status )
	    break;
	
	page = 0;
	status = dm2f_alloc_file(tp->tpcb_locations, 
				 tp->tpcb_loc_count, 
				 &table_name, &dcb->dcb_name, 
				 (endpage + 1), &page, dberr); 
	if ( status )
	    break;

	status = dm2f_flush_file(tp->tpcb_locations, 
				 tp->tpcb_loc_count,  
				 &table_name, &dcb->dcb_name, 
				 dberr);
	if ( status )
	    break;

	for ( i = 0; i <= endpage; )
	{
	    /* Read old file. */

	    /*
	    ** Check to see if user interrupt occurred.
	    ** Can't make this check in ROLLFORWARD since there is no
	    ** user interrupt state place to check.
	    */
	    if ( m->mx_state & MX_ABORT || *(sp->spcb_rcb->rcb_uiptr) )
	    {
		CLRDBERR(dberr);

		if ( m->mx_state & MX_ABORT )
		    SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
		else
		{
		    /* If XCB, check via SCB */
		    if ( sp->spcb_rcb->rcb_xcb_ptr )
		    {
			dmxCheckForInterrupt(sp->spcb_rcb->rcb_xcb_ptr,
						&error);
			if ( error )
			    SETDBERR(dberr, 0, error);
		    }
		    else if ( *(sp->spcb_rcb->rcb_uiptr) & RCB_USER_INTR )
			SETDBERR(dberr, 0, E_DM0065_USER_INTR);
		    else if ( *(sp->spcb_rcb->rcb_uiptr) & RCB_FORCE_ABORT )
			SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
		}

		if ( dberr->err_code )
		{
		    status = E_DB_ERROR;
		    break;
		}
	    }
		
	    if (endpage + 1 - i >= DI_EXTENT_SIZE)
		n = DI_EXTENT_SIZE;
	    else
		n = endpage + 1 - i;

	    status = dm2f_read_file(sp->spcb_locations, 
				    sp->spcb_loc_count, 
				    m->mx_page_size,
				    &table_name, &dcb->dcb_name, &n, 
				    (i4)i, (char *)buffer, dberr); 
	    if ( status )
		break;

	    /* Write new file. */

	    status = dm2f_write_file(tp->tpcb_locations, 
				     tp->tpcb_loc_count, 
				     t->tcb_rel.relpgsize,
				     &table_name, &dcb->dcb_name, &n,
				     (i4)i, (char *)buffer, dberr); 
	    if ( status )
		break;
	    i = i + n;
	}
	
	/*
	** Close files used for relocate.
	** If an error was encountered, we still execute this code to close
	** the files : the error is saved in "status" and "error".
	**
	** If an error is encountered closing a file, save it in "save_status"
	** and "save_error" to make sure we don't return without reporting
	** an error.
	*/

	local_status = dm2f_close_file(sp->spcb_locations, 
				       sp->spcb_loc_count, 
				       (DM_OBJECT *)dcb, 
				       &local_dberr);
	if ( local_status )
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    if ( status == E_DB_OK )
	    {
		status = local_status;
		*dberr = local_dberr;
	    }
	}
	break;
    }

    /* If we opened this table, close-purge it */
    if ( sp->spcb_rcb != m->mx_rcb )
    {
	(VOID)dm2t_close(sp->spcb_rcb, DM2T_PURGE, &local_dberr);
	sp->spcb_rcb = (DMP_RCB*)NULL;
    }

    if ( buf_cb )
	dm0m_deallocate((DM_OBJECT **)&buf_cb);

    if ( status && dberr->err_code > E_DM_INTERNAL )
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9342_DM2U_REORG);
    }

    return(status);
}

/*
** The following provide dummy definitions for the routine dm1u_table().
** This routine processes checklink/patchlink operations.
**
** These dummy routines allow us to simulate successfull and unsuccessfull
** checklink/patchlink operations.
**
** These are used for specially built TEST versions of the code.  Normally,
** this code will not be compiled - neither TEST1A nor TEST1B should be defined.
**
** History
**	07-mar-90 (rogerk)
**	    Code moved from Terminator I to Termiantor II path for Teresa's
**	    checklink/patchlink tests.
*/

#ifdef TEST1A 
/*
** DM1U_TABLE - Return Success
**
**	This is a dummy definition of dm1u_table() which always returns success.
**
**	Set to true when run DMTST1 - test path from ESQL to dm1u_table, but do
**	not test dm1u_table operations -- ie use dummy routine -- IF THIS IS
**	SET, DUMMY ROUTINES ALWAYS RETURN SUCCESSFULLY.  NEVER DEFINE BOTH
**	TEST1A AND TEST1B SIMULTANEOUSLY, OR YOU WILL GET COMPILE ERRORS.
*/
DB_STATUS
dm1u_table(
DMP_RCB             *r,
i4             mode,
i4             *error)
{
    return (E_DB_OK);
}
#endif

#ifdef TEST1B 
/*
** DM1U_TABLE - Return Failure
**
**	This is a dummy definition of dm1u_table() which always returns failure.
**
**	Set to true when run DMTST1 - test path from ESQL to dm1u_table, but do
**	not test dm1u_table operations -- ie use dummy routine -- IF THIS IS
**	SET, DUMMY ROUTINES ALWAYS RETURNS WITH A FAILURE STATUS.  NEVER DEFINE
**	BOTH TEST1A AND TEST1B SIMULTANEOUSLY, OR YOU WILL GET COMPILE ERRORS.
*/
DB_STATUS
dm1u_table (
DMP_RCB             *r,
i4             mode,
i4             *error)
{
    *error = 12345;
    return (E_DB_ERROR);
}
#endif

/*{
** Name: update_temp_table_tcb	- Update TCB after modify
**
** Description:
**	Before a modify or index command is finished, the system tables
**	have to be updated to reflect the changed in relation attributes,
**	attribute attributes and indexes attributes.
**
**	Since temporary tables are not cataloged (have no entries in iirelation,
**	iiattribute, etc.), we do not update the actual catalogs for these
**	tables. Instead, we directly update the in-memory TCB.
**
**
**	To update the temp table's in-memory TCB, we perform the following
**	steps:
**	    1) Locate the current TCB, and merge its information together with
**		the information in the MXCB to get a composite description of
**		the new table.
**	    2) Call dm2t_destroy_temp_tcb to get rid of the old TCB. This will
**		destroy the old table's files (if they exist).
**	    3) Rename the new table's files to the appropriate names.
**	    4) Call dm2t_temp_build_tcb to build the new TCB using the
**		composite table information compiled in step (1).
**
** Inputs:
**      mxcb                            The modify index control block.
**
** Outputs:
**      err_code			The reason for an error return status.
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
**	20-sep-1991 (bryanp)
**	    Created.
**	16-sep-1992 (bryanp)
**	    Set relpgtype and relcomptype in update_temp_table_tcb().
**	18-Sept-1992 (rmuth)
**	    Set relpgtype to TCB_PG_DEFAULT.
**	16-0ct-1992 (rickh)
**	    Initialize default ids for attributes.
**	26-oct-1992 (rogerk)
**	    Reduced logging project: changed references to tcb fields to
**	    use new tcb_table_io structure fields.  Changed from old
**	    dm2t_get_tcb_ptr to new dm2t_fix_tcb.  Also explicitly turned
**	    off the BUSY status in the TCB that is set while the new tcb
**	    is built - it is no longer turned off by dm2t_awaken_tcb.
**	30-October-1992 (rmuth)
**	    When checking for FCB_OPEN use the & instead of == because
**	    we my not flush when allocating.
**	18-jan-1993 (bryanp)
**	    Don't just assert TCB_BUSY, use proper TCB exclusion protocols.
**	31-aug-1994 (mikem)
**	    bug #64832
**          Creating key column's in any order on global temporary tables
**	    now works.  Previous to this change the code to set up a temp
**	    tcb only worked for key's in column order (ie. first col, second
**	    col, ...).  Changed the key setup code to use m->mx_atts_ptr which
**	    is an ordered list of keys, rather than the list of attributes it
**	    was using before.
**	 5-jul-95 (dougb) bug # 69382
**	    To avoid consistency point failures (E_DM9397_CP_INCOMPLETE_ERROR)
**	    which can occur if the CP thread runs during or just after
**	    update_temp_table_tcb(), leave it to dm2t_temp_build_tcb() to
**	    insert new TCB into the HCB queue.
**	31-Mar-1997 (jenjo02)
**	    Update relation cache priority from mxcb.
**	22-aug-1998 (somsa01)
**	    If this TCB has extension tables, then we need to also save
**	    the tcb_et_list from the old TCB before tossing it. We also
**	    have to carry over the logical key.
**	14-Nov-2001 (thaju02)
**	    After destroying the temporary modify build table, release the
**	    lock. (B106380)
**	08-Nov-2000 (jenjo02)
**	    Carry tcb_temp_xccb from old to new TCB.
**	15-dec-04 (inkdo01)
**	    Copy attcollid value, too.
**	04-feb-05 (jenjo02 & toumi01)
**	    XCCB list pointer fixup for CPU spike fix.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    "ETL_TYPE" changed to "ETL_CB" for conformance.
**	15-Aug-2006 (jonj)
**	    Support Temp Table indexes.
**	08-Sep-2006 (jonj)
**	    Don't release LK_TABLE lock for new Index TCB
**	    when destroying the old one.
**	7-Aug-2008 (kibro01) b120592
**	    Add new flag to dm2t_temp_build_tcb
**	16-Nov-2009 (kschendel) SIR 122890
**	    Update call to dm2t-destroy-temp-tcb.
*/
static DB_STATUS
update_temp_table_tcb(
DM2U_MXCB           *mxcb,
DB_ERROR      	    *dberr)
{
    DM2U_MXCB		*m = mxcb;
    DM2U_TPCB		*tp;
    DM2U_M_CONTEXT	*mct;
    DB_STATUS		status;
    i4			i,j,k;
    i4             	local_error;
    DB_TAB_TIMESTAMP	timestamp;
    DMP_TCB		*t;
    DMP_TCB		*old_tcb = NULL;
    DMP_TCB		*ParentTCB;
    DMP_RELATION	relation;
    DM_FILE		mod_filename;
    DM_FILE		new_filename;
    DMP_MISC		*loc_cb = 0;
    DB_LOC_NAME		*location_names;
    DMP_MISC		*attr_cb = 0;
    DMF_ATTR_ENTRY	**attr_array;
    DMF_ATTR_ENTRY	*attrs;
    DMP_MISC		*key_cb = 0;
    DM2T_KEY_ENTRY	**key_array;
    DM2T_KEY_ENTRY	*keys;
    DMP_TCB		*build_tcb;
    DB_TAB_ID		build_table_id;
    DB_TAB_NAME		table_name;
    i4			recovery;
    i4			build_flag;
    DMP_ET_LIST		*etlist_ptr, *extension_list = 0, *build_etlist = 0;
    u_i4		tcb_high_logkey, tcb_low_logkey;
    DML_XCCB		*tcb_temp_xccb;
    bool		MustUnbusyBuildTCB = FALSE;
    i4			TableLockList;
    i4			XCBLockList;
    i4			error;
    DB_ERROR		local_dberr;

    for (;;)
    {

	/* Assume a single target and its context */
	tp = m->mx_tpcb_next;
	mct = &tp->tpcb_mct;

	/* Session temp table locks are always on the thread SCB lock list */
        TableLockList = m->mx_xcb->xcb_scb_ptr->scb_lock_list;

	/* The build temp table is not a Session temp, use XCB's lock list */
	XCBLockList = m->mx_xcb->xcb_lk_id;

	/*
	** Get pointer to current temp table TCB.
	*/
	status = dm2t_fix_tcb(m->mx_dcb->dcb_id, &m->mx_table_id, 
			    (DB_TRAN_ID *)0, TableLockList, m->mx_log_id,
			    (i4)(DM2T_NOBUILD | DM2T_NOWAIT),
			    m->mx_dcb, &old_tcb, dberr);
	if (status != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
	    break;
	}
	t = old_tcb;

	/*
	** Set up the array of attribute structures, and the array of pointers
	** to attribute structures, and the array of key structures, and the
	** array of pointers to key structures. It's all a bit unwieldy, but
	** basically we're setting up the arguments to be used, eventually,
	** in the call to dm2t_temp_build_tcb.
	*/

	status = dm0m_allocate(t->tcb_rel.relatts * sizeof(DMF_ATTR_ENTRY) +
			       t->tcb_rel.relatts * sizeof(DMF_ATTR_ENTRY *) +
							    sizeof(DMP_MISC),
		    DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		    (char *)0, (DM_OBJECT **)&attr_cb, dberr);
	if (status)
	    break;
	attr_array = (DMF_ATTR_ENTRY **)((char *)attr_cb + sizeof(DMP_MISC));
	attr_cb->misc_data = (char *)attr_array;
	attrs = (DMF_ATTR_ENTRY *)((char *)attr_array +
		    (t->tcb_rel.relatts * sizeof(DMF_ATTR_ENTRY *)));

	status = dm0m_allocate(mct->mct_keys * sizeof(DM2T_KEY_ENTRY) +
			       mct->mct_keys * sizeof(DM2T_KEY_ENTRY *) +
							    sizeof(DMP_MISC),
		    DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		    (char *)0, (DM_OBJECT **)&key_cb, dberr);
	if (status)
	    break;
	key_array = (DM2T_KEY_ENTRY **)((char *)key_cb + sizeof(DMP_MISC));
	key_cb->misc_data = (char *)key_array;
	keys = (DM2T_KEY_ENTRY *)((char *)key_array +
		    (mct->mct_keys * sizeof(DM2T_KEY_ENTRY *)));

	status = dm0m_allocate(tp->tpcb_loc_count * sizeof(DB_LOC_NAME) +
						    sizeof(DMP_MISC),
		    DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		    (char *)0, (DM_OBJECT **)&loc_cb, dberr);
	if (status)
	    break;
	location_names = (DB_LOC_NAME *)((char *)loc_cb + sizeof(DMP_MISC));
	loc_cb->misc_data = (char *)location_names;

	for (k = 0; k < tp->tpcb_loc_count; k++)
	    STRUCT_ASSIGN_MACRO(tp->tpcb_locations[k].loc_name, location_names[k]);

	/*
	** Build new attr_array and key_array arrays using the old attribute
	** information from the old temp table TCB and the new key information
	** from the modify control block. These arrays will be passed to
	** dm2t_temp_build_tcb to build the new temp table tcb.
	*/
	for ( i = 0; i < t->tcb_rel.relatts; i++ )
	{
	    attr_array[i] = &attrs[i];
	    MEmove(t->tcb_atts_ptr[i+1].attnmlen,
		t->tcb_atts_ptr[i+1].attnmstr, ' ',
		DB_ATT_MAXNAME, attrs[i].attr_name.db_att_name);
	    attrs[i].attr_type = t->tcb_atts_ptr[i+1].type;
	    attrs[i].attr_size = t->tcb_atts_ptr[i+1].length;
	    attrs[i].attr_precision = t->tcb_atts_ptr[i+1].precision;
	    attrs[i].attr_flags_mask = t->tcb_atts_ptr[i+1].flag;
	    COPY_DEFAULT_ID( t->tcb_atts_ptr[i+1].defaultID,
		attrs[i].attr_defaultID );
	    attrs[i].attr_collID = t->tcb_atts_ptr[i+1].collID;
	    attrs[i].attr_geomtype = t->tcb_atts_ptr[i+1].geomtype;
	    attrs[i].attr_encflags = t->tcb_atts_ptr[i+1].encflags;
	    attrs[i].attr_encwid = t->tcb_atts_ptr[i+1].encwid;
	    attrs[i].attr_srid = t->tcb_atts_ptr[i+1].srid;
	}

	/* Make dm2t-suitable copies of the key attributes */

	for ( i = 0; i < mct->mct_keys; i++ )
	{
	    key_array[i] = &keys[i];
	    MEmove(m->mx_i_key_atts[i]->attnmlen,
		m->mx_i_key_atts[i]->attnmstr, ' ',
		DB_ATT_MAXNAME, keys[i].key_attr_name.db_att_name);
	    if (m->mx_i_key_atts[i]->flag & ATT_DESCENDING) 
		keys[i].key_order = DM2T_DESCENDING;
	    else
		keys[i].key_order = DM2T_ASCENDING;
	}

	/* Copy Index key map for dm2t_temp_build_tcb */
	for ( i = 0; i < DB_MAXIXATTS; i++ )
	    m->mx_idom_map[i] = t->tcb_ikey_map[i];

	/*
	** build composite relation tuple using old relation information
	** combined with results of modify:
	*/
	STRUCT_ASSIGN_MACRO(t->tcb_rel, relation);
	if (m->mx_reorg == 0)
	{
	    relation.relmoddate = TMsecs();
	    relation.relspec = m->mx_structure;
	    relation.relpages = mct->mct_relpages;
	    relation.relprim = mct->mct_prim;
	    relation.relmain = mct->mct_main;
	    relation.relifill = mct->mct_i_fill;
	    relation.reldfill = mct->mct_d_fill;
	    relation.rellfill = mct->mct_l_fill;
	    relation.relmin = m->mx_minpages;
	    relation.relmax = m->mx_maxpages;
	    relation.relkeys = mct->mct_keys;
	    relation.reltups = m->mx_newtup_cnt;
	    relation.relfhdr = mct->mct_fhdr_pageno;
	    relation.relpgsize = m->mx_page_size;
	    relation.relpgtype = m->mx_page_type;
	    relation.relstat &=
		~(TCB_COMPRESSED | TCB_UNIQUE | 
		    TCB_INDEX_COMP | TCB_IDXD | 
		    TCB_DUPLICATES | TCB_CLUSTERED);
	    relation.relcmptlvl = DMF_T_VERSION;
	    if (m->mx_data_rac.compression_type != TCB_C_NONE)
		relation.relstat |= TCB_COMPRESSED;
	    relation.relcomptype = m->mx_data_rac.compression_type;
	    if (m->mx_index_comp)
		relation.relstat |= TCB_INDEX_COMP;
	    if (m->mx_unique)
		relation.relstat |= TCB_UNIQUE;
	    if (m->mx_duplicates)
		relation.relstat |= TCB_DUPLICATES;
	    if ( m->mx_clustered )
		relation.relstat |= TCB_CLUSTERED;
	    
	    /* Update cache priority with new or old value */
	    relation.reltcpri = m->mx_tbl_pri;
	}

	/* Save pointer to XCCB, if any */
	tcb_temp_xccb = t->tcb_temp_xccb;

	/* Save pointer to parent TCB, needed if Index */
	ParentTCB = t->tcb_parent_tcb_ptr;

	recovery = t->tcb_logging;

	tcb_high_logkey = t->tcb_high_logical_key;
	tcb_low_logkey = t->tcb_low_logical_key;

	/*
	** Save off the tcb_et_list, if the TCB has extensions.
	*/
	if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    etlist_ptr = t->tcb_et_list;
	    do
	    {
		status = dm0m_allocate( sizeof(DMP_ET_LIST), (i4)0,
					(i4)ETL_CB,
					(i4)ETL_ASCII_ID, (char *)0,
					(DM_OBJECT **)&extension_list,
					dberr);
		if (status)
		    break;

		extension_list->etl_status = etlist_ptr->etl_status;
		STRUCT_ASSIGN_MACRO(etlist_ptr->etl_etab,
				    extension_list->etl_etab);
		if (!build_etlist)
		{
		    build_etlist = extension_list;
		    build_etlist->etl_next = build_etlist->etl_prev =
			build_etlist;
		}
		else
		{
		    extension_list->etl_next = build_etlist;
		    extension_list->etl_prev = build_etlist->etl_prev;
		    build_etlist->etl_prev = extension_list;
		    extension_list->etl_prev->etl_next = extension_list;
		}

		etlist_ptr = etlist_ptr->etl_next;
	    }
	    while (etlist_ptr != t->tcb_et_list);
	}

	/*
	** Free up handle to temp table tcb so we can destroy it.
	*/
	status = dm2t_unfix_tcb(&old_tcb, (i4)0, TableLockList,
				m->mx_log_id, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Note that if we're modifying a Base TempTable, this
	** destroy will also destroy all attached indexes and
	** release all table locks.
	**
	** If it's a TempTable Index, this will only destroy
	** the Index TCB, but we'll want to keep the table lock.
	** To do that, we pass a lock_list of zero.
	*/
	status = dm2t_destroy_temp_tcb((t->tcb_rel.relstat & TCB_INDEX)
					? 0 : TableLockList,
				    m->mx_dcb, 
				    &m->mx_table_id, dberr);
	if (status != E_DB_OK)
	    break;

	build_flag = DM2T_TMP_MODIFIED;

	if (m->mx_inmemory)
	{
	    /*
	    ** Remember the build table's TCB. Mark it busy so that the
	    ** buffer manager knows to leave it alone during the various
	    ** close, reparent, rename etc. operations. Since we've marked
	    ** it busy, we have the "right" to keep examining the TCB even
	    ** after we've closed the table...
	    **
	    ** Also, once the TCB becomes busy, we know that other threads in
	    ** the server won't try to write pages out for the build TCB, and
	    ** this if it's still in deferred IO state at this point, it'll
	    ** stay in deferred IO state until we complete the build. In order
	    ** truly ensure this, we must be certain that any materialization
	    ** of this build tcb which is in progress completes before we
	    ** continue. To await completion of any materialize_fcb() operation
	    ** it is sufficient merely to wait until we are the only agent
	    ** which has the TCB fixed. Then we mark the tcb busy.
	    */

	    build_tcb = m->mx_buildrcb->rcb_tcb_ptr;
	    status = dm2t_reserve_tcb(build_tcb, m->mx_dcb,
				    DM2T_TMPMOD_WAIT, XCBLockList, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Any failures after this must unbusy build_tcb */
	    MustUnbusyBuildTCB = TRUE;

	    status = dm2t_close(m->mx_buildrcb, DM2T_NOPURGE, dberr);
	    m->mx_buildrcb = (DMP_RCB*)NULL;

	    if (status != E_DB_OK)
		break;

	}

	for (k = 0; k < tp->tpcb_loc_count; k++)
	{
	    /*
	    ** If we did a build into memory, and no new files were actually
	    ** created, then there's no renaming needed. Instead, we'll just
	    ** re-parent the in-memory pages over to the new TCB after we
	    ** create it. Indicate this by setting a special build_flag value.
	    */
	    if (m->mx_inmemory != 0 &&
		(build_tcb->tcb_table_io.tbio_location_array[0].loc_fcb->fcb_state &
			FCB_DEFERRED_IO) != 0)
	    {
		build_flag = DM2T_TMP_INMEM_MOD;
		break;
	    }

	    /*
	    ** Build filenames depend on whether this was an in-memory build
	    ** or a modify-filename build.
	    **	    mod_filename - filename of modify file.  This file is
	    **			   renamed to become the user table.
	    **	    new_filename - new user table filename.
	    */

	    if (m->mx_inmemory)
	    {
		STRUCT_ASSIGN_MACRO(
		    build_tcb->tcb_table_io.tbio_location_array[k].
							loc_fcb->fcb_filename,
		    mod_filename);
	
		if (build_tcb->tcb_table_io.tbio_location_array[0].
						loc_fcb->fcb_state & FCB_OPEN)
		{
		    status = dm2f_close_file(
			build_tcb->tcb_table_io.tbio_location_array,
			build_tcb->tcb_table_io.tbio_loc_count,
			(DM_OBJECT *)build_tcb->tcb_table_io.
						tbio_location_array[0].loc_fcb,
			dberr); 

		    if (status != E_DB_OK)
			break;
		}
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO(tp->tpcb_locations[k].loc_fcb->fcb_filename,
		    mod_filename);
	    }
	    dm2f_filename(DM2F_TAB, &m->mx_table_id, 
		    tp->tpcb_locations[k].loc_id, 0, 0, &new_filename);

	    /*
	    ** Note that we don't log the file rename operations on temporary
	    ** tables...
	    */

	    /*
	    ** Rename modify table to the user table file.
	    */
	    status = dm2f_rename(XCBLockList, m->mx_log_id, 
                    &tp->tpcb_locations[k].loc_ext->physical, 
		    tp->tpcb_locations[k].loc_ext->phys_length,
		    &mod_filename, (i4)sizeof(mod_filename),
		    &new_filename, (i4)sizeof(new_filename),
		    &m->mx_dcb->dcb_name, dberr);
	    if (status != E_DB_OK)
		break;

	}

	if (status != E_DB_OK)
	    break;

	/* If rebuilding Index TCB, pass the Base TCB */
	t = ParentTCB;
	
	status = dm2t_temp_build_tcb(m->mx_dcb, &relation.reltid,
			&m->mx_xcb->xcb_tran_id, TableLockList,
			m->mx_log_id, &t, &relation,
			attr_array, relation.relatts,
			key_array, relation.relkeys,
			build_flag, recovery, mct->mct_allocated,
			location_names, tp->tpcb_loc_count,
			m->mx_idom_map,
			dberr,
			FALSE);

	if (status != E_DB_OK)
	    break;

	t->tcb_high_logical_key = tcb_high_logkey;
	t->tcb_low_logical_key = tcb_low_logkey;

	/* Restore XCCB pointer, if any */
	if ( tcb_temp_xccb )
	{
	    t->tcb_temp_xccb = tcb_temp_xccb;
	    tcb_temp_xccb->xccb_t_tcb = t;
	}

	if (t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS)
	{
	    t->tcb_et_list->etl_status = build_etlist->etl_status;
	    STRUCT_ASSIGN_MACRO(t->tcb_et_list->etl_etab,
				build_etlist->etl_etab);

	    etlist_ptr = build_etlist->etl_next;
	    while (etlist_ptr != build_etlist)
	    {
		status = dm0m_allocate( sizeof(DMP_ET_LIST), (i4)0,
					(i4)ETL_CB,
					(i4)ETL_ASCII_ID,
					(char *)t,
					(DM_OBJECT **)&extension_list,
					dberr);
		if (status)
		    break;

		extension_list->etl_status = etlist_ptr->etl_status;
		STRUCT_ASSIGN_MACRO(etlist_ptr->etl_etab,
				    extension_list->etl_etab);
		extension_list->etl_next = t->tcb_et_list;
		extension_list->etl_prev = t->tcb_et_list->etl_prev;
		t->tcb_et_list->etl_prev = extension_list;
		extension_list->etl_prev->etl_next = extension_list;

		etlist_ptr = etlist_ptr->etl_next;
	    }

	    if (build_etlist->etl_next != build_etlist)
	    {
		for (etlist_ptr = build_etlist->etl_next;
		    etlist_ptr != build_etlist;)
		{
		    extension_list = etlist_ptr->etl_next;
		    dm0m_deallocate((DM_OBJECT **)&etlist_ptr);
		    etlist_ptr = extension_list;
		}
	    }
	    dm0m_deallocate((DM_OBJECT **)&build_etlist);
	}

	if (m->mx_inmemory)
	{
	    /*
	    ** To conclude an in-memory build, we now reparent the in-memory
	    ** build pages over to the permanent table, and destroy the
	    ** build-duration table. If there was a file created for the build
	    ** TCB, it's been renamed above, so once we re-parent all the pages
	    ** we can mark the build TCB as in deferred state -- this keeps the
	    ** release code from trying to destroy the non-existent table. Once
	    ** we've completed the awful re-parenting and other low-level
	    ** machinations, we can mark the build TCB as no longer busy and
	    ** destroy it.
	    */
	    status = dm0p_reparent_pages(build_tcb, t, dberr);
	    if (status != E_DB_OK)
		break;

	    STRUCT_ASSIGN_MACRO(build_tcb->tcb_rel.reltid,
				build_table_id);

	    build_tcb->tcb_table_io.tbio_location_array[0].loc_fcb->fcb_state |=
			FCB_DEFERRED_IO;

	    dm0s_mlock(build_tcb->tcb_hash_mutex_ptr);
	    dm2t_awaken_tcb( build_tcb, XCBLockList );
	    build_tcb->tcb_status &= ~(TCB_BUSY);
	    dm0s_munlock(build_tcb->tcb_hash_mutex_ptr);

	    MustUnbusyBuildTCB = FALSE;

	    /* Destroy temp build table and release its table lock */
	    status = dm2t_destroy_temp_tcb(XCBLockList,
			    m->mx_dcb, &build_table_id, dberr);
	    if (status != E_DB_OK)
		break;
	}

	if (loc_cb)
	    dm0m_deallocate((DM_OBJECT **)&loc_cb);
	    
	if (attr_cb)
	    dm0m_deallocate((DM_OBJECT **)&attr_cb);
	    
	if (key_cb)
	    dm0m_deallocate((DM_OBJECT **)&key_cb);
	
	return (E_DB_OK);
    }

    /*	Handle cleanup for error recovery. */

    if ( m->mx_inmemory && m->mx_buildrcb )
    {
	(VOID) dm2t_close(m->mx_buildrcb, DM2T_NOPURGE, &local_dberr);
	m->mx_buildrcb = (DMP_RCB*)NULL;
    }
    
    if ( MustUnbusyBuildTCB )
    {
	dm0s_mlock(build_tcb->tcb_hash_mutex_ptr);
	dm2t_awaken_tcb( build_tcb, XCBLockList );
	build_tcb->tcb_status &= ~(TCB_BUSY);
	dm0s_munlock(build_tcb->tcb_hash_mutex_ptr);
    }

    if (loc_cb)
	dm0m_deallocate((DM_OBJECT **)&loc_cb);

    if (attr_cb)
	dm0m_deallocate((DM_OBJECT **)&attr_cb);
	
    if (key_cb)
	dm0m_deallocate((DM_OBJECT **)&key_cb);

    if (old_tcb)
    {
	status = dm2t_unfix_tcb(&old_tcb, (i4)0, TableLockList,
				m->mx_log_id, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (build_etlist)
    {
	if (build_etlist->etl_next != build_etlist)
	{
	    for (etlist_ptr = build_etlist->etl_next;
		etlist_ptr != build_etlist;)
	    {
		extension_list = etlist_ptr->etl_next;
		dm0m_deallocate((DM_OBJECT **)&etlist_ptr);
		etlist_ptr = extension_list;
	    }
	}
	dm0m_deallocate((DM_OBJECT **)&build_etlist);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: create_build_table	    - create a temp table for build usage
**
** Description:
**	In order to use the "build into the buffer manager" feature, we must
**	have a temporary table open in deferred I/O mode. This subroutine opens
**	such a table.
**
**	The actual columns and their data types don't matter, since we'll just
**	be using this table for fix and unfix page calls.
**
** Inputs:
**      mxcb                            The modify index control block.
**
** Outputs:
**      err_code			The reason for an error return status.
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
**	10-mar-1992 (bryanp)
**	    Created.
**	31-jan-94 (jrb)
**          Accept a location list and then use it to create the temp table.
**	    Before we were just using $default.
*/
static DB_STATUS
create_build_table(
DM2U_MXCB           *mxcb,
DMP_DCB		    *dcb,
DML_XCB		    *xcb,
DB_TAB_NAME	    *table_name,
DB_LOC_NAME	    *loc_list,
i4		    loc_count,
DB_ERROR            *dberr)
{
    DM2U_MXCB		*m = mxcb;
    DM2U_TPCB		*tp;
    DM2U_M_CONTEXT	*mct;
    DB_STATUS		status;
    DMT_CB		dmt_cb;
    DB_TAB_TIMESTAMP	timestamp;
    DMT_CHAR_ENTRY      characteristics[2];

    /* Assume a single target TPCB and its context */
    tp = m->mx_tpcb_next;
    mct = &tp->tpcb_mct;

    /*
    ** Set up DMT_CB control block appropriately and call dmt_create_temp
    */

    MEfill(sizeof(DMT_CB), '\0', (PTR) &dmt_cb);
    dmt_cb.length = sizeof(DMT_CB);
    dmt_cb.type = DMT_TABLE_CB;
    dmt_cb.dmt_flags_mask = DMT_DBMS_REQUEST;

    dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
    dmt_cb.dmt_tran_id = (PTR)xcb;
    MEcopy((char *)table_name, sizeof(dmt_cb.dmt_table), 
	(char *)&dmt_cb.dmt_table);
    MEfill(sizeof(dmt_cb.dmt_owner.db_own_name), (u_char)' ',
				(PTR)dmt_cb.dmt_owner.db_own_name);

    characteristics[0].char_id = DMT_C_PAGE_SIZE;
    characteristics[0].char_value = m->mx_page_size;

    /*
    ** The temp must be created with mx_page_type
    ** Note if we didn't pass the page type chosen by modify we would have
    ** to make sure to send all information necessary to have dmt_create_temp
    ** create the temp with page type mx_page_type
    ** For example if we are modifying an etab, dmt_create temp would have
    ** if (m->mx_rcb->rcb_tcb_ptr->tcb_extended)
    **    dmt_cb.dmt_flags_mask |= DMT_LO_TEMP;
    */
    characteristics[1].char_id = DMT_C_PAGE_TYPE;
    characteristics[1].char_value = m->mx_page_type;

    dmt_cb.dmt_location.data_address = (char *)loc_list;
    dmt_cb.dmt_location.data_in_size = loc_count * sizeof(DB_LOC_NAME);

    dmt_cb.dmt_char_array.data_address = (PTR)characteristics;
    dmt_cb.dmt_char_array.data_in_size = 2 * sizeof(DMT_CHAR_ENTRY);

    dmt_cb.dmt_attr_array.ptr_address = (PTR)0;
    dmt_cb.dmt_attr_array.ptr_in_count = 0;
    dmt_cb.dmt_attr_array.ptr_size = 0;

    status = dmt_create_temp(&dmt_cb);

    if (status != E_DB_OK)
    {
	*dberr = dmt_cb.error;
	return (status);
    }

    status = dm2t_open(dcb, &dmt_cb.dmt_id, DM2T_X, 
            DM2T_UDIRECT, DM2T_A_MODIFY,
	    (i4)0, (i4)20, (i4)0, xcb->xcb_log_id, 
            xcb->xcb_lk_id, (i4)0, (i4)0, 
            DM2T_S, 
	    &xcb->xcb_tran_id, &timestamp, &m->mx_buildrcb, (DML_SCB *)0, 
	    dberr);

    return (status);
}

static DB_STATUS
AddOldTargetLocation(
DM2U_MXCB	*m,
DMP_LOCATION	*loc,
DMP_LOCATION	*nloc,
DB_ERROR	*dberr)
{
    DMP_DCB		*dcb = m->mx_dcb;
    DB_STATUS		status = E_DB_OK;
    i4			i;

    STRUCT_ASSIGN_MACRO(*loc, *nloc);

    nloc->loc_fcb = (DMP_FCB*)NULL;
    nloc->loc_status &= ~LOC_OPEN;

    /*
    ** We're called when there are no new locations
    ** specified. If any of the table's old locations
    ** are RAW, that's an error because we can't
    ** modify on top of the old RAW locations;
    ** "...with locations=()" must be present.
    */

    for ( i = 0; i < dcb->dcb_ext->ext_count; i++ )
    {
	if ( (MEcmp((char*)&nloc->loc_name.db_loc_name,
		    (char*)&dcb->dcb_ext->ext_entry[i].logical,
		    sizeof(DB_LOC_NAME))) == 0 &&
	      dcb->dcb_ext->ext_entry[i].flags & DCB_DATA &&
	      dcb->dcb_ext->ext_entry[i].flags & DCB_RAW )
	{
	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM0191_RAW_LOCATION_MISSING);
	    break;
	}
    }

    return(status);
}

static DB_STATUS
AddNewLocation(
DM2U_MXCB	*m,
DB_LOC_NAME	*loc_name,
DMP_LOCATION	*loc,
i4		loc_id,
DB_ERROR	*dberr)
{
    DMP_DCB		*dcb = m->mx_dcb;
    i4			i, compare;
    DB_STATUS		status;
    LK_LKID		lockid;

    MEfill(sizeof(LK_LKID), 0, &lockid);

    /*  Check location, must be default or valid location. */

    if ( (compare = MEcmp((PTR)DB_DEFAULT_NAME, (PTR)loc_name,
	    sizeof(DB_LOC_NAME))) == 0 )
    {
	loc->loc_ext = dcb->dcb_root;
	STRUCT_ASSIGN_MACRO(dcb->dcb_root->logical,
			     loc->loc_name);

	loc->loc_id = loc_id;
	loc->loc_fcb = NULL;
	loc->loc_config_id = 0;
	loc->loc_status = LOC_VALID;
    }
    else for (i = 0; i < dcb->dcb_ext->ext_count; i++)
    {
	compare = MEcmp((char *)loc_name, 
	      (char *)&dcb->dcb_ext->ext_entry[i].logical, 
	      sizeof(DB_LOC_NAME));

	if (compare == 0 && 
	    (dcb->dcb_ext->ext_entry[i].flags & DCB_DATA))
	{
	    STRUCT_ASSIGN_MACRO(*loc_name, loc->loc_name);
	    loc->loc_ext = &dcb->dcb_ext->ext_entry[i];
	    loc->loc_id = loc_id;
	    loc->loc_fcb = (DMP_FCB*)NULL;
	    loc->loc_config_id = i;
	    loc->loc_status = LOC_VALID;
	    
	    if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW) 
	    {
		status = dm2u_raw_device_lock(dcb, m->mx_xcb,
				&loc->loc_name, &lockid, 
				dberr);
		if (status != LK_NEW_LOCK)
		{
		    SETDBERR(dberr, loc_id, E_DM0189_RAW_LOCATION_INUSE);
		    return(E_DB_ERROR);
		}
		status = dm2u_raw_location_free(dcb, m->mx_xcb,
				&loc->loc_name, dberr);
		if (status != E_DB_OK)
		    return(status);
		loc->loc_status |= LOC_RAW;
	    }
	    break;
	}
	else
	    compare = 1;
    }

    if (compare)
    {
	SETDBERR(dberr, loc_id, E_DM001D_BAD_LOCATION_NAME);
	return(E_DB_ERROR);
    }
    return(E_DB_OK);
}

/*
** Name: logModifyData - log data arrays for MODIFY
**
** Description:
**	In order to replay a journalled MODIFY, it's necessary to be able
**	to reconstruct the original dm2u_modify call.  In some cases,
**	such as repartitioning modifies, it would not suffice to just log
**	a modify for each partition.  (We need the partition definition
**	to know what goes where.)  So, modify replay works by simulating
**	the original modify in its entirety, and that means that we have
**	to log the partition definition and ppchar arrays.
**
**	These cannot be logged with a normal log record, as they can
**	be indefinitely long.  The limit on the number of partitions
**	is 64K, and at present the longest allowable log record is only
**	128K.  Extending the log record size to potentially megabytes
**	is unreasonable.  So, we'll log these things with a RAWDATA
**	log record type, that allows an object to be logged with a
**	series of log records.  RAWDATA places no restrictions on how
**	the object is chopped up, so we'll pick an arbitrary division
**	of 15K max.  (Log buffers are often a multiple of 4K, and we'll
**	leave a bit for overhead.)
**
**	It would be nice if both partition definition and ppchar array
**	were just random data that could be written as-is.  Of course,
**	nothing is ever that simple in the server.  The partition
**	definition is in one piece, but contains pointers.  The
**	ppchar array may point all over the place in addition to
**	potentially containing pointers.
**
**	We're going to assume that the partition definition is writable,
**	but that the ppchar array may not be.
**	This is dirty, and assumes way too much about the outside
**	world, but I don't really want to be calling the partdef
**	copier from here.  As it happens, QEU will be making a partdef
**	copy for us, because its version has all sorts of extra junk
**	in it that we don't need.  The ppchar array however could be
**	the original one built by the parse, and it might be read-only.
**
**	All of this is dreadfully inconvenient for the server.  But
**	nobody asked us...
**
** Inputs:
**	log_id			The logging system ID to use
**	flags			Log record header flags (eg journal)
**	part_def		Address of new partition definition
**	def_size		Size in bytes of partition definition
**	ppchar_addr		Base of ppchar array for partitions
**	ppchar_entries		Number of entries in ppchar array
**	err_code		An output
**
** Output:
**	err_code		Updated with error specifics as needed
**	Returns E_DB_xxx status
**
** Side effects:
**	The partition definition (if any) is de-pointerized and re-pointerized
**	in-place.
**
** History:
**	2-Apr-2004 (schka24)
**	    Write to get replay of modify working again.
**	7-Dec-2007 (kibro01 on behalf of kschendel) b119585
**	    Initialise status since it doesn't always get set.
**	    This can happen with 'modify t partition p to reconstruct'.
**	    (part_def can be null when ppchar_addr isn't)
*/

/* Defines local to this routine and friends */
#define NUM_LOCN_TRACK	10		/* Max # of location arrays to track */

static DB_STATUS
logModifyData(i4 log_id, i4 flags,
	DB_PART_DEF *part_def, i4 def_size,
	DMU_PHYPART_CHAR *ppchar_addr, i4 ppchar_entries,
	DB_ERROR *dberr)
{
    DB_STATUS status = E_DB_OK;		/* Called routine status */
    DM_DATA locns[NUM_LOCN_TRACK];	/* Location arrays we're tracking */
    i4 dim;				/* Partdef dimension number */

    /* See if there's anything that needs to be done */
    if (part_def == NULL && ppchar_addr == NULL)
	return (E_DB_OK);

    /* Do these separately since there are specific complications
    ** for each.
    */
    if (part_def != NULL)
	status = logPartDef(log_id, flags, part_def, def_size, dberr);

    if (status == E_DB_OK && ppchar_addr != NULL)
	status = logPPchar(log_id, flags, ppchar_addr, ppchar_entries,
			dberr);


    return (status);
} /* logModifyData */

/*
** Name: logPartDef - log a partition definition
**
** Description:
**	This routine logs a partition definition via the RAWDATA
**	log mechanism.
**
**	Partition definitions contain addresses internally, so we need
**	to de-pointerize and re-pointerize the partition definition.
**	Fortunately, some caller has arranged that the entire partition
**	definition is in one contiguous memory area, so this is easy.
**
**	The partition definition is de- and re-pointerized in place.
**
** Inputs:
**	log_id			The logging system ID to use
**	flags			Log record header flags (eg journal)
**	part_def		Address of new partition definition
**	def_size		Size in bytes of partition definition
**	err_code		An output
**
** Output:
**	err_code		Updated with error specifics as needed
**	Returns E_DB_xxx status
**
** Side effects:
**	The partition definition (if any) is de-pointerized and re-pointerized
**	in-place.
**
** History:
**	2-Apr-2004 (schka24)
**	    Written.
*/

static DB_STATUS
logPartDef(i4 log_id, i4 flags,
	DB_PART_DEF *part_def, i4 def_size,
	DB_ERROR *dberr)
{

    DB_PART_BREAKS *break_ptr;		/* Pointer to breaks table entry */
    DB_PART_DEF empty_def;		/* For writing nopartitions */
    DB_PART_DIM *dim_ptr;		/* A partition dimension */
    DB_STATUS status;			/* Called routine status */
    i4 cur_size;			/* Size of this write */
    i4 size_left;			/* Bytes left to write */
    LG_LSN lsn;				/* Log LSN, notused */
    PTR cur_posn;			/* Where we are writing at present */
    PTR part_def_base;			/* Base of partdef area */

    /* Deal with special case of writing unpartitioning partdef.
    ** Don't trust what caller fed in this case, particularly size.
    ** (This is entirely paranoia...)
    */
    if (part_def->ndims == 0)
    {
	part_def = &empty_def;
	part_def_base = (PTR) part_def;
	MEfill(sizeof(empty_def), 0, &empty_def);
	def_size = sizeof(empty_def);
    }
    else
    {
	/* De-pointerize */
	/* Don't care about partition names, nor break val-text's */
	part_def_base = (PTR) part_def;
	for (dim_ptr = &part_def->dimension[0];
	     dim_ptr < &part_def->dimension[part_def->ndims];
	     ++ dim_ptr)
	{
	    break_ptr = dim_ptr->part_breaks;
	    if (break_ptr != NULL)
	    {
		do
		{
		    break_ptr->val_base = (PTR) (break_ptr->val_base - part_def_base);
		} while (++break_ptr < &dim_ptr->part_breaks[dim_ptr->nbreaks]);
		dim_ptr->part_breaks = (DB_PART_BREAKS *)
				((PTR) dim_ptr->part_breaks - part_def_base);
	    }
	    if (dim_ptr->part_list != NULL)
		dim_ptr->part_list = (DB_PART_LIST *)
				((PTR) dim_ptr->part_list - part_def_base);
	}
    }

    /* Ok, now write it out.  Write a chunk, advance, till it's done. */
    size_left = def_size;
    cur_posn = part_def_base;
    do
    {
	cur_size = MAX_RAWDATA_SIZE;
	if (size_left < cur_size)
	    cur_size = size_left;	/* Just write what's left */
	status = dm0l_rawdata(log_id, flags,
			DM_RAWD_PARTDEF, 0,
			def_size,
			(cur_posn - part_def_base), cur_size,
			cur_posn,
			NULL, &lsn, dberr);
	if (DB_FAILURE_MACRO(status))
	    break;
	size_left = size_left - cur_size;
	cur_posn = cur_posn + cur_size;
    } while (size_left > 0);

    dm2u_repointerize_partdef(part_def);
    return (status);
} /* logPartDef */

/*
** Name: logPPchar - log a PPCHAR array
**
** Description:
**	This routine logs a PPCHAR array for MODIFY.
**
**	This is rather tricky, because the ppchar array contains
**	embedded DM_DATA (and someday, DM_PTR) structures that point
**	to who knows what.  For the present, we just have to deal with
**	the embedded location arrays, and that's bad enough.  In order
**	to shrink the amount of stuff to log, we'll take advantage
**	of the fact that it's likely that very many of the location
**	DM_DATA's actually point to the exact same thing.  On the other
**	hand, we can't go overboard on this, because they *might* be
**	all different.  The compromise I'm using is to keep track of
**	the first N location array addresses, where N is some small number.
**	If there are more than N distinct location arrays, the additional
**	ones get treated as if they were all distinct.
**
**	Furthermore, unlike the partition definition, the ppchar
**	array and its locations are not necessary contiguous.
**	(The ppchar array per se is, but the location arrays could
**	be just about anywhere.)  This, plus the fact that the ppchar
**	array might be read-only, means that we can't modify it in-place
**	like the partition definition.
**
**	The approach taken is two phases (or three passes over the ppchar
**	array, depending on how you like to count.)  The first pass/phase
**	figures out how much total space we'll need.  It tracks up to
**	NUM_LOCN_TRACK location arrays so that each only needs to be
**	output once.  The second phase outputs the ppchar array in a
**	depointerized state; outputs the tracked location arrays;
**	and outputs the untracked location arrays (which takes yet
**	one more pass over the ppchar array).  Output is done into
**	a buffer, which is flushed to a RAWDATA log record when full.
**
**	For use in reconstructing the ppchar array, the number of
**	array entries is written as an i4 in front of the actual
**	array.
**
**	The original ppchar array is not touched (see the intro to
**	logModifyData).  All necessary changes are done on the fly
**	while buffering.
**
** Inputs:
**	log_id			The logging system ID to use
**	flags			Log record header flags (eg journal)
**	ppchar_addr		Base of ppchar array for partitions
**	ppchar_entries		Number of entries in ppchar array
**	err_code		An output
**
** Output:
**	err_code		Updated with error specifics as needed
**	Returns E_DB_xxx status
**
** History:
**	2-Apr-2004 (schka24)
**	    All of this just to write some journal records...
**	22-Nov-2004 (thaju02)
**	    Align cur_posn before copying ppchar array into rawdata buffer.
**	    (B113515)
**	28-jan-2005 (thaju02)
**	    Fix for b113515 breaks rolldb on 32-bit Solaris.
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
*/

static DB_STATUS
logPPchar(i4 log_id, i4 flags,
	DMU_PHYPART_CHAR *ppchar_addr, i4 ppchar_entries,
	DB_ERROR *dberr)
{
    DB_STATUS status;			/* Called routine status */
    DM_DATA locns[NUM_LOCN_TRACK];	/* Location arrays we're tracking */
    DMP_MISC *holding_area;		/* Holding tank for writing data */
    DMU_PHYPART_CHAR *ppc;		/* Pointer to an array entry */
    DMU_PHYPART_CHAR *outppc;		/* Array entry in the output buf */
    i4 cur_size;			/* Size of this written chunk */
    i4 i;
    i4 num_locns;			/* Number of locations in "locns" */
    i4 locn_offset[NUM_LOCN_TRACK];	/* Where tracked locn will go */
    i4 offset;				/* Offset of this rawdata load */
    i4 total_size;			/* Total size of object */
    i4 untracked_offset;		/* Offset for non-tracked locn array */
    LG_LSN lsn;				/* Log LSN, notused */
    PTR cur_posn;			/* Where we are writing at present */
    PTR rawdata;			/* Build up raw data here */
    PTR rawend;				/* End of above, for convenience */
    i4	error;

    /* First pass: figure out total size needed, and compute the
    ** layout for location arrays that we're tracking.
    ** Allow for a leading i4 "header".
    */
    num_locns = 0;
    untracked_offset = 0;
    total_size = sizeof(i4);
    total_size = DB_ALIGN_MACRO(total_size);
    total_size += (ppchar_entries * sizeof(DMU_PHYPART_CHAR));
    for (ppc = ppchar_addr;
	 ppc < &ppchar_addr[ppchar_entries];
	 ++ppc)
    {
	if (ppc->loc_array.data_address != NULL)
	{
	    /* Need to output a location array, see if we've seen this
	    ** one before
	    */
	    for (i = 0; i < num_locns; ++i)
		if (locns[i].data_address == ppc->loc_array.data_address)
		    break;
	    if (i == num_locns)
	    {
		/* Didn't find it.  Track it if we have room.  */
		if (num_locns < NUM_LOCN_TRACK)
		{
		    STRUCT_ASSIGN_MACRO(ppc->loc_array, locns[num_locns]);
		    locn_offset[num_locns] = total_size;
		    ++ num_locns;
		}
		else if (untracked_offset == 0)
		{
		    /* Remember where untracked locn arrays start at */
		    untracked_offset = total_size;
		}
		total_size += ppc->loc_array.data_in_size;
	    }
	} /* if have loc_array */
    }

    /* We need some place to write from */
    status = dm0m_allocate(MAX_RAWDATA_SIZE + sizeof(DMP_MISC), 0,
		(i4)MISC_CB, (i4)MISC_ASCII_ID,
		NULL, (DM_OBJECT **) &holding_area, dberr);
    if (DB_FAILURE_MACRO(status))
	return (status);
    rawdata = (PTR)holding_area + sizeof(DMP_MISC);
    holding_area->misc_data = rawdata;
    rawend = rawdata + MAX_RAWDATA_SIZE;
    cur_posn = rawdata;

    /* Whirl around, copying array and location bits to the holding
    ** area, and writing it out to the log when it gets full.
    ** There's no need to exactly fill the holding area if it is
    ** inconvenient to do so...
    */

    /* Once around to depointerize and output the ppchar array itself */
    *(i4 *)cur_posn = ppchar_entries;
    cur_posn += sizeof(i4);
    cur_posn = ME_ALIGN_MACRO(cur_posn, sizeof(ALIGN_RESTRICT));
    offset = 0;
    for (ppc = ppchar_addr;
	 ppc < &ppchar_addr[ppchar_entries];
	 ++ppc)
    {
	/* Check if room for this one, flush if not */
	if (cur_posn + sizeof(DMU_PHYPART_CHAR) > rawend)
	{
	    cur_size = cur_posn - rawdata;
	    status = dm0l_rawdata(log_id, flags,
			DM_RAWD_PPCHAR, 0,
			total_size,
			offset, cur_size,
			rawdata,
			NULL, &lsn, dberr);
	    if (DB_FAILURE_MACRO(status))
		goto exit;
	    offset += cur_size;
	    cur_posn = rawdata;
	}
	outppc = (DMU_PHYPART_CHAR *)cur_posn;
	MEcopy(ppc, sizeof(DMU_PHYPART_CHAR), outppc);
	if (outppc->loc_array.data_address != NULL)
	{
	    /* Depointerize this location array address.  See if we've
	    ** tracked it and know its offset.
	    */
	    for (i = 0; i < num_locns; ++i)
		if (locns[i].data_address == outppc->loc_array.data_address)
		    break;
	    /* Note, the extra SCALARP cast here is to stop "cast to pointer
	    ** from int of diff size" warnings on LP64.
	    */
	    if (i < num_locns)
	    {
		outppc->loc_array.data_address = (PTR) (SCALARP) locn_offset[i];
	    }
	    else
	    {
		/* Didn't find it.  Assume it goes out at the end.  */
		outppc->loc_array.data_address = (PTR) (SCALARP) untracked_offset;
		untracked_offset += outppc->loc_array.data_in_size;
	    }
	}
	cur_posn += sizeof(DMU_PHYPART_CHAR);
    }

    /* Next go the tracked location arrays in the order they were seen.
    ** Be lazy and don't bother splitting an array across a bufferful.
    ** It will get re-buffered in the log buffers anyway.
    */
    for (i = 0; i < num_locns; ++i)
    {
	/* Check if room for this one, flush if not */
	if (cur_posn + locns[i].data_in_size > rawend)
	{
	    cur_size = cur_posn - rawdata;
	    status = dm0l_rawdata(log_id, flags,
			DM_RAWD_PPCHAR, 0,
			total_size,
			offset, cur_size,
			rawdata,
			NULL, &lsn, dberr);
	    if (DB_FAILURE_MACRO(status))
		goto exit;
	    offset += cur_size;
	    cur_posn = rawdata;
	}
	MEcopy(locns[i].data_address, locns[i].data_in_size, cur_posn);
	cur_posn += locns[i].data_in_size;
    }

    /* One more time thru the ppchar array, this time looking for and
    ** writing out location arrays that weren't tracked.  (being careful
    ** to do it in the same order as we depointerized.)
    ** We can skip this if there were no untracked location arrays.
    */

    if (untracked_offset > 0)
    {
	for (ppc = ppchar_addr;
	     ppc < &ppchar_addr[ppchar_entries];
	     ++ppc)
	{
	    if (ppc->loc_array.data_address != NULL)
	    {
		/* Look it up in the tiresome little tracking array again */
		for (i = 0; i < num_locns; ++i)
		    if (locns[i].data_address == ppc->loc_array.data_address)
			break;
		if (i == num_locns)
		{
		    /* This is a not-found one, out it goes */
		    if (cur_posn + ppc->loc_array.data_in_size > rawend)
		    {
			cur_size = cur_posn - rawdata;
			status = dm0l_rawdata(log_id, flags,
				    DM_RAWD_PPCHAR, 0,
				    total_size,
				    offset, cur_size,
				    rawdata,
				    NULL, &lsn, dberr);
			if (DB_FAILURE_MACRO(status))
			    goto exit;
			offset += cur_size;
			cur_posn = rawdata;
		    }
		    MEcopy(ppc->loc_array.data_address,
				    ppc->loc_array.data_in_size, cur_posn);
		    cur_posn += ppc->loc_array.data_in_size;
		}
	    }
	}
    } /* if untracked location arrays */

    /* One last flush of the holding tank.  We always flush-then-write
    ** so there's always something left over.
    */
    status = dm0l_rawdata(log_id, flags,
		DM_RAWD_PPCHAR, 0,
		total_size,
		offset, cur_posn - rawdata,
		rawdata,
		NULL, &lsn, dberr);
    /* (retain "status" for exit) */

    /* Deallocate holding tank and leave */
exit:
    dm0m_deallocate((DM_OBJECT **) &holding_area);

    return (status);
} /* logPPchar */

/*
** Name: dm2u_repointerize_partdef
**
** Description:
**	This routine re-pointerizes a partition definition in-place.
**	It's assumed that the entire partition definition occupies one
**	single contiguous area of memory.  We don't need to worry
**	about partition names, nor about break value texts;  those
**	pointers are simply ignored completely.
**
**	The partition definition was de-pointerized originally by
**	the obvious expedient of replacing pointers with the offset
**	from the base of the partition def.  This routine simply
**	reverses that process.
**
**	It's a routine mostly so that recovery as well as dmu can use
**	it.  There's nothing specifically "dm2u-ish" about it, but
**	this is a convenient place to put it.
**
** Inputs:
**	part_def		Pointer to de-pointerized definition
**
** Outputs:
**	None (void)
**	(Re-pointerizes the part-def in place)
**
** History:
**	2-Apr-2004 (schka24)
**	    Written.
*/

void
dm2u_repointerize_partdef(DB_PART_DEF *part_def)
{

    DB_PART_BREAKS *break_ptr;		/* Pointer to breaks table entry */
    DB_PART_DIM *dim_ptr;		/* A partition dimension */
    PTR part_def_base;			/* Base of partdef area */

    part_def_base = (PTR) part_def;
    for (dim_ptr = &part_def->dimension[0];
	 dim_ptr < &part_def->dimension[part_def->ndims];
	 ++ dim_ptr)
    {
	/* Order in which we reconstruct the pointers is important... */

	if (dim_ptr->part_list != (DB_PART_LIST *) 0)
	    dim_ptr->part_list = (DB_PART_LIST *)
			    ((SCALARP) dim_ptr->part_list + part_def_base);
	break_ptr = dim_ptr->part_breaks;
	if (break_ptr != (DB_PART_BREAKS *) 0)
	{
	    break_ptr = (DB_PART_BREAKS *)
			    ((SCALARP) dim_ptr->part_breaks + part_def_base);
	    dim_ptr->part_breaks = break_ptr;
	    do
	    {
		break_ptr->val_base = (PTR) ((SCALARP) break_ptr->val_base + part_def_base);
	    } while (++break_ptr < &dim_ptr->part_breaks[dim_ptr->nbreaks]);
	}
    }
} /* dm2u_repointerize_partdef */

/*
** Name: dm2u_repointerize_ppchar - Re-pointerize ppchar arrah
**
** Description:
**	This routine reverses the transformation done on a ppchar
**	array to log it:  namely, it changes any location-array
**	offsets back into addresses.
**
**	Since the caller is able to read in the entire ppchar array
**	with its associated location arrays into one giant blob of
**	memory, this task vastly simpler than un-pointerizing it was.
**	All we have to do is whirl thru the ppchar entries and
**	change offsets to addresses.  The ppchar logger has been
**	kind enough to stick the number of ppchar entries at the
**	very front of the data.
**
**	While this routine is currently used only by recovery, it seemed
**	to make sense to put it with the un-pointerizing side.
**
** Inputs:
**	rawdata			Base of blob holding #entries, ppchar, locns
**
** Outputs:
**	Returns actual base address of ppchar array
**	(No status return, always succeeds.)
**
** History:
**	2-Apr-2004 (schka24)
**	    Written.
**	22-Nov-2004 (thaju02)
**	    Align ppc_base. (b113515)
*/

DMU_PHYPART_CHAR *
dm2u_repointerize_ppchar(PTR rawdata)
{
    DMU_PHYPART_CHAR *ppc;		/* Address of a ppchar entry */
    DMU_PHYPART_CHAR *ppc_base;		/* Start of ppchars */
    i4 ppchar_entries;			/* Number of entries */

    ppchar_entries = *(i4 *)rawdata;	/* # of entries leads the way */
    ppc_base = (DMU_PHYPART_CHAR *) (rawdata + sizeof(i4));
    ppc_base = (DMU_PHYPART_CHAR *)ME_ALIGN_MACRO((PTR)ppc_base, sizeof(ALIGN_RESTRICT));
    for (ppc = ppc_base;
	 ppc < &ppc_base[ppchar_entries];
	 ++ ppc)
	if (ppc->loc_array.data_address != (PTR) 0)
	    ppc->loc_array.data_address = (PTR)
			((SCALARP)ppc->loc_array.data_address + rawdata);

    return (ppc_base);
} /* dm2u_repointerize_ppchar */


/*
** Name: dm2u_catalog_modify -- Catalog-only modify variants
**
** Description:
**
**	This routine handles various MODIFY actions which only update
**	the catalog status of the table.  These include:
**
**		to [NO]READONLY
**		to PHYS/LOG_[IN]CONSISTENT
**		to TABLE_RECOVERY_[DIS]ALLOWED
**		to CACHE_PRIORITY=n
**		to [NO]PERSISTENCE
**		to [ROW/STATEMENT]_LEVEL_UNIQUE
**		to [NO]PSEUDOTEMPORARY
**		to [NO]CHANGE_TRACKING
**		to ENCRYPT WITH PASSPHRASE='my passphrase' or ''
**			[, NEW_PASSPHRASE='my revised passphrase']
**
**	these are not necessarily the SQL syntax keywords but you get
**	the idea!
**
**	The modification is logged with an ALTER log record, which is
**	largely bogus but does serve to purge the TCB in either direction
**	(do or undo).  The action logged with the ALTER is not even
**	looked at.  The catalog update is itself logged in the normal
**	way, which supplies the actual do / undo / redo for this modify.
**
**	Because of the logging method noted above, we never get here for
**	rollforward, and hence can depend on the lock / log / transaction
**	ID's in the xcb (which must exist).
**
** Inputs:
**	mcb			Modify info block with actions in modopts2
**	r			RCB for table, opened by caller
**	err_code		An output
**
** Outputs:
**	err_code		E_xxx error code returned here, if any
**	Returns E_DB_xxx status
**
** History:
**	20-Mar-2008 (kschendel) SIR 122739
**	    Extract from giant modify mainline.  Add change-tracking,
**	    pseudotemporary.
**	16-Apr-2010 (toumi01) SIR 122403
**	    Add support for MODIFY ENCRYPT.
*/

static DB_STATUS
dm2u_catalog_modify(DM2U_MOD_CB *mcb, DMP_RCB *r,
	DB_ERROR *dberr)
{

    DB_ERROR local_dberr;
    DB_STATUS status;
    DML_XCB *xcb;		/* Transaction control block */
    DMP_TCB *t;			/* TCB for table being modified */
    DMP_DCB *dcb;		/* Database control block */
    DM2U_MXCB local_mxcb;	/* Stub modify control info */
    i4 action;			/* modoptions2 bitmask action */
    i4 journal;			/* Table-is-journaled for updating */
    i4 logged_action;		/* Action for ALTER log record (bogus) */
    i4 local_error;
    LG_LSN toss_lsn;		/* Needed but not used */

    xcb = mcb->mcb_xcb;
    dcb = mcb->mcb_dcb;
    t = r->rcb_tcb_ptr;
    action = mcb->mcb_mod_options2;

    MEfill(sizeof(local_mxcb), 0, (PTR) &local_mxcb);

    /* We hardly need the mxcb as such, it's mainly used as a convenient
    ** way to pass stuff to the dm2u catalog updaters.  A slightly
    ** spiffed up mcb would do as well, but the mxcb was here first...
    */

    local_mxcb.mx_operation = MX_MODIFY;
    local_mxcb.mx_rcb = r;
    local_mxcb.mx_xcb = xcb;
    local_mxcb.mx_lk_id = xcb->xcb_lk_id;
    local_mxcb.mx_log_id = xcb->xcb_log_id;
    local_mxcb.mx_dcb = dcb;
    STRUCT_ASSIGN_MACRO(*mcb->mcb_tbl_id, local_mxcb.mx_table_id);
    local_mxcb.mx_dmveredo = FALSE;
    local_mxcb.mx_new_relstat2 = action;	/* liar, isn't relstat2 */
    local_mxcb.mx_tbl_pri = mcb->mcb_tbl_pri;

    /* all spcb, tpcb stuff zeros and nulls */

    if (action & (DM2U_2_READONLY | DM2U_2_NOREADONLY) )
    {
	/* Double-checking for readonly */
	if ( t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)
	  || t->tcb_rel.relstat & TCB_INDEX
	  || t->tcb_rel.relstat2 & TCB2_PARTITION )
	{
	    SETDBERR(dberr, 0, E_DM0068_MODIFY_READONLY_ERR);
	    (void) dm2t_close(r, DM2T_NOPURGE, &local_dberr);
	    return (E_DB_ERROR);
	}

	local_mxcb.mx_readonly = MXCB_SET_READONLY;
	if (action & DM2U_2_NOREADONLY)
	    local_mxcb.mx_readonly = MXCB_UNSET_READONLY;
	logged_action = DMT_C_READONLY;
    }
    else
    {
	/* Double-checking for others allows secondary index (which may
	** not get thru syntax checking depending on the action).
	*/
	if ( t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)
	  || t->tcb_rel.relstat2 & TCB2_PARTITION )
	{
	    SETDBERR(dberr, 0, E_DM0160_MODIFY_ALTER_STATUS_ERR);
	    (void) dm2t_close(r, DM2T_NOPURGE, &local_dberr);
	    return (E_DB_ERROR);
	}
	/* This next bit is completely silly, since nothing cares, but
	** I suppose I might as well do at least this much.
	*/
	if (action & (DM2U_2_LOG_INCONSISTENT | DM2U_2_LOG_CONSISTENT))
	    logged_action = DMT_C_LOG_CONSISTENT;
	else if (action & (DM2U_2_PHYS_INCONSISTENT | DM2U_2_PHYS_CONSISTENT))
	    logged_action = DMT_C_PHYS_CONSISTENT;
	else
	    logged_action = DMT_C_TBL_RECOVERY_ALLOWED;
    }

    /* Log the alter, which inserts a TCB purge point in case of
    ** rollforward or rollback (although not online redo, oddly enough;
    ** perhaps that is a FIXME??).
    */
    if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
    {
	status = dm0l_alter(xcb->xcb_log_id, 0, mcb->mcb_tbl_id,
		&t->tcb_rel.relid, &t->tcb_rel.relowner,
		0, logged_action,
		NULL, &toss_lsn, dberr);
	if (status != E_DB_OK)
	    return (status);
    }

    /* Make the actual catalog change.  Different routine for [no]readonly,
    ** since it has some goofy extra stuff to do for secondary indexes.
    */

    journal = (t->tcb_rel.relstat & TCB_JOURNAL) != 0;
    if (action & (DM2U_2_READONLY | DM2U_2_NOREADONLY) )
	status = dm2u_readonly_upd_cats(&local_mxcb, journal, dberr);
    else
    if (action & DM2U_2_ENCRYPT)
	status = dm2u_modify_encrypt(&local_mxcb, mcb->mcb_dmu, journal, dberr);
    else
	status = dm2u_alterstatus_upd_cats(&local_mxcb, journal, dberr);
    if (status != E_DB_OK)
	return (status);

    /*
    ** Purge the table
    */
    status = dm2t_close(r, DM2T_PURGE, dberr);
    return( status );
	    
} /* dm2u_catalog_modify */

/*{
** Name: do_online_modify	    - Online modify
**
** Description:
**      This routine implements online modify. This routine generates the 
**      modify load files (.m00) for the modify. Exclusive access to the table
**      is required only at the end of the processing. The steps below are
**      labeled N for NOLOCK, X for EXCLUSIVE lock.
** 
**      N Write a DM0LBSF log record - to start the logging of concurrent
**        updates by the archiver
**	N Create internal table $online_tableid having the same
**        columns as the modify table.
**      N The modify table is opened - readlock = nolock
**      N The $online table is modified, input_rcb = modify table
**        with readlock = nolock
**      N Build persistent indexes on $online table.
**      N Write a DM0LNSF log record to close off current sidefile and open 
**        a new one
**      N Start archiver purge-wait cycle
**      N Apply all updates in the sidefile to $online table
**      X Get exclusive access to the table
**      X Write a DM0LESF log record to close current sidefile
**      X Start archive purge-wait cycle
**      X Apply all updates in current sidefile to $online table
**      X Destroy persistent indexes on $online table, and save the
**        DM2F_DES filenames (.d00) for each.
**      X Destroy $online table 
**      X Rename $online.d00 (DM2F_DES) files -> modtable.m00 files
**
** The only thing the caller (dm2u_modify) must do for online modify is to
** skip the load - since the .m00 files have already been generated. 
** In addition, dm2u_modify now rebuilds the persistent indexes which will
** proceed as usual through dmu_index or dm2u_pindex EXCEPT that the load
** will be skipped - it will only necessary to:
**  RENAME $online_tabid_ix_indexnum.DM2F_DES -> modtable_ix.DM2F_MOD
**  (RENAME $online_tabid_ix_indexnum.d00 -> modtable_ix.m00)
**      
** Inputs:
**	tp				The Target table info.
**	timeout
**      dmu
**      mxcb				Modify control block. 
**      modoptions			Zero or DM2U_TRUNCATE or 
**					DM2U_DUPLICATES or DM2U_REORG.
**      mod2_options			Flags indicating what to do.
**	kcount				Count of key entry.
**	key				Pointer to key array of pointers
**					for modifying.
**	db_lockmode			Lockmode of the database for the caller.
**
** Outputs:
**	tup_info			Number of tuples in the modified table
**					or the bad key length if key length
**					is too wide.
**	*has_extensions			Filled as appropriate.
**      online_reltup			Filled in to use for modtable 
**					catalog updates
**      err_code                        The reason for an error return status.
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
**      02-dec-2003 (stial01)
**	    Created.
**	03-jun-2004 (thaju02)
**	    Modified the logging of rnl lsn array. Added still_to_log.
**	15-dec-04 (inkdo01)
**	    Copy attcollid value, too.
**      29-Sep-2004 (fanch01)
**          Conditionally add LK_LOCAL flag if database/table is
**	    confined to one node.
**	15-dec-04 (inkdo01)
**	    Copy attcollid value, too.
**	31-jan-2005 (thaju02)
**	    Online modify of partitioned table. 
**	    - create $online partitions using source information.
**	    - open modify table's partitions readnolock.
**	    - hang rnl lsn stuff off of spcb entry.
**	    - log rnl lsn for each source entry.
**	    - setup the targets reltup info here, rather than having to 
**	      pass it back to dm2u_modify for setup there.
**	    - in do_sidefile, open $online partitions and if appropriate
**	      open secondary index's partitions for positioning; added
**	      open_parts() and close_parts(). Added find_part().
**	    - for rename of $online .dxx -> .mxx use tpcb_name_id/gen.
**	15-feb-2005 (thaju02)
**	    Online modify repartition with new partitions. 
**	    - In a regular modify, new partitions are created in 
**	      qeu_modify_prep(). Added create_new_parts(), to
**	      create new parts for $online and modify table. 
**	    - create $online ppchar array, initializing new
**	      parts part_tabid's. initialize modify table's
**	      ppchar array with new parts part_tabid's.
**	11-Mar-2005 (thaju02)
**	    Cleanup. Created get_online_rel(). Use $online
**	    index relpages, relprim, relmain for index.
**	    Use parent table's tuple count for index
**	    reltups. (B114069)
**	07-Apr-2005 (thaju02)
**	    Prev change for b114069 misplaced get_online_rel().
**	26-Jul-2005 (thaju02)
**	    Convert to X tbl control lock before taking X tbl lock.
**	    Re-address bug 114861. 
**	30-Aug-2005 (thaju02)
**	    Propagate SYS_MAINTAINED. 
**	23-May-2006 (jenjo02)
**	    Carry Clustered wishes to modified table, partitions.
**	19-Dec-2007 (jonj)
**	    Replace PCsleep() with CSsuspend(). PCsleep() affects
**	    the entire process with Ingres threading.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
**	27-Jul-2009 (thaju02) B122283
**	    If target is non-partition, set DM2U_ONLINE_REPART flag. 
**	5-Nov-2009 (kschendel) SIR 122739
**	    Fix outrageous name confusion re acount vs kcount.
**	 
*/
static DB_STATUS
do_online_modify(
DM2U_MXCB	    **mxcb,
i4		    timeout,
DMU_CB		    *dmu,
i4		    modoptions,
i4		    mod_options2,
i4		    kcount,
DM2U_KEY_ENTRY	    **key,
i4		    db_lockmode,
DB_TAB_ID	    *modtemp_tabid,
i4		    *tup_info,
i4		    *has_extensions,
i4		    relstat2,
i4		    flagsmask,
DM2U_RFP_ENTRY  *rfp_entry,
DMP_RELATION	*online_reltup,
DB_PART_DEF	    *new_part_def,
i4		    new_partdef_size,
DMU_PHYPART_CHAR    *partitions,
i4		    nparts,
i4		    verify,
DB_ERROR	    *dberr)
{
    DM2U_MXCB		*m = (*mxcb);
    DM2U_TPCB		*tp = m->mx_tpcb_next;
    DMP_DCB		*dcb = m->mx_dcb;
    DML_XCB		*xcb = m->mx_xcb;
    DMP_RCB		*r = m->mx_rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DMP_RCB		*xrcb = (DMP_RCB *)0;
    i4			logging = t->tcb_logging;
    DB_TAB_NAME		table_name;
    DB_OWN_NAME		table_owner;
    DB_TAB_NAME		newtab_name;
    char		mastertab_name[DB_TAB_MAXNAME + 1];
    char		newparttab_name[DB_TAB_MAXNAME + 30];
    DB_TAB_NAME		newix_name;
    DB_OWN_NAME		owner;
    DB_TAB_NAME		index_name;
    DB_TAB_ID		new_tabid;
    DB_TAB_ID		newpart_tabid;
    DB_TAB_ID		new_ixid;
    DB_TAB_ID		newpart_ixid;
    i4			index = 0;
    i4			view = 0;
    i4			duplicates;
    i4			attr_count;
    DMF_ATTR_ENTRY	*attrs;
    DMF_ATTR_ENTRY     **attr_ptrs;
    i4			i,k;
    DB_STATUS           status;
    DM_TID              tid;
    DB_TAB_TIMESTAMP	timestamp;
    LG_LSN		lsn;
    i4			setrelstat= 0;
    DMP_MISC		*misc_cb = (DMP_MISC *)0;
    DMP_RCB		*readnolock_rcb = (DMP_RCB *)0;
    DMP_TCB		*online_tcb = (DMP_TCB *)0;
    i4			extend = 0;
    i4			allocation = 0;
    i4			nofiles = 0;
    i4			local_error;
    i4			error;
    DB_STATUS		local_status;
    i4			dm0l_flag = 0;
    i4			journal;
    DMU_CB		*index_cbs;
    DMU_CB		*ix_dmu;
    i4			numberOfIndices;
    DML_XCCB		*xccb;
    bool		found_xccb;
    DM2U_INDEX_CB	dm2u_index_cb;
    DB_ERROR		local_dberr;
    i4			temp_tup_cnt;
    i4			base_relstat2;
    DB_TAB_NAME		modbase_name;
    DM2U_KEY_ENTRY	*keys;
    DM2U_KEY_ENTRY	**key_ptrs;

    LK_LOCK_KEY         lock_key;
    LK_LKID             lkid;
    CL_ERR_DESC         sys_err;
    DM2U_MOD_CB		local_mcb, *mcb = &local_mcb;

    /* following added for partitioned tbl */
    DMP_TCB		*pt = (DMP_TCB *)0;
    DB_PART_DEF		dmu_part_def;
    DM2U_SPCB		*sp;
    DMP_MISC		*omisc_cb = (DMP_MISC *)0;
    DM2U_OPCB		*opcb = 0;
    DM2U_OMCB		omcb;
    DM2U_OSRC_CB	*osrc;
    DMP_MISC		*loc_misc_cb = (DMP_MISC *)0;
    DB_LOC_NAME		*locnArray = 0;
    i4			parts = 0;
    i4			destroy_flag;
    DMP_MISC		*pp_misc_cb = (DMP_MISC *)0;
    DMU_PHYPART_CHAR	*o_partitions = (DMU_PHYPART_CHAR *)0;

    /* following used to log rnl_lsn list */
    DMP_RNL_ONLINE      *rnl;   
    i4                  lsn_cnt;
    i4                  logged, cnt, still_to_log;

    /* Needed to tidy up sidefile if online modify fails */
    bool		outstanding_sidefile = FALSE;

    MEfill(sizeof(DM2U_OMCB), 0, &omcb);

    if (!logging)
	return (E_DB_ERROR);

    /* Generate name for temp table needed for online modify */
    if ((*xcb->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) != 0)
	STprintf(newtab_name.db_tab_name, "$ONLINE_%x_%x", (i4)
	    m->mx_table_id.db_tab_base, m->mx_table_id.db_tab_index);
    else
	STprintf(newtab_name.db_tab_name, "$online_%x_%x", (i4)
	     m->mx_table_id.db_tab_base, m->mx_table_id.db_tab_index);

    STmove(newtab_name.db_tab_name, ' ', 
	sizeof (newtab_name.db_tab_name), newtab_name.db_tab_name);

    /*
    ** If online modify of secondary index, 
    ** we need to create a temp table and temp index
    ** Generate name for temp index
    */
    if (t->tcb_rel.relstat & TCB_INDEX)
    {
	STRUCT_ASSIGN_MACRO(newtab_name, newix_name);
	if ((*xcb->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) != 0)
	    newix_name.db_tab_name[7] = 'I';
	else
	    newix_name.db_tab_name[7] = 'i';
    }

    STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, table_name);
    STRUCT_ASSIGN_MACRO(t->tcb_rel.relowner, table_owner);
    if (DMZ_SRT_MACRO(13))
	TRdisplay("ONLINE MODIFY: %~t Using temp %~t\n",
	    sizeof(DB_TAB_NAME), table_name.db_tab_name,
	    sizeof(DB_TAB_NAME), newtab_name.db_tab_name);

    for (;;)
    {
	/* quiesce table */
	lock_key.lk_type = LK_TABLE;
	lock_key.lk_key1 = dcb->dcb_id;
	lock_key.lk_key2 = t->tcb_rel.reltid.db_tab_base;
	lock_key.lk_key3 = 0;
	lock_key.lk_key4 = 0;
	lock_key.lk_key5 = 0;
	lock_key.lk_key6 = 0;
	MEfill(sizeof(LK_LKID), 0, &lkid);

	status = LKrequest(dcb->dcb_bm_served == DCB_SINGLE ?
                           LK_PHYSICAL | LK_LOCAL : LK_PHYSICAL,
                           xcb->xcb_lk_id, &lock_key, LK_S, (LK_VALUE *)0,
                           (LK_LKID *)&lkid, 0, &sys_err);
	if (status != E_DB_OK)
	{
	    if (status == LK_NOLOCKS)
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else if (status == LK_TIMEOUT)
		SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
	    else if ((status == LK_INTERRUPT) || (status == LK_INTR_GRANT))
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
            else if (status == LK_INTR_FA)
                SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
	    else
		SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);
	    status = E_DB_ERROR;
	    break;
	}

	status = dm0l_bsf(m->mx_log_id, dm0l_flag, &t->tcb_rel.reltid, 
	    &table_name, &table_owner,  (LG_LSN *)0, &m->mx_bsf_lsn, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** call check_archiver to purge log to mx_bsf_lsn
	*/
	status = check_archiver(dcb, xcb, &table_name, &m->mx_bsf_lsn,
			dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** FIX ME
	** Now make sure sidefile was created successfully
	*/
	/* sidefile at this stage definitely needs clearing up */
	outstanding_sidefile = TRUE;

	status = LKrelease(0, xcb->xcb_lk_id, &lkid, &lock_key, 
			(LK_VALUE *)0, &sys_err);
	if (status)
	{
	    SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);
	    break;
	}

	if (DMZ_SRT_MACRO(14))
	{

	    TRdisplay("ONLINE MODIFY: %~t Sleeping after log bsf to allow concurrent updates\n",
		sizeof(DB_TAB_NAME), table_name.db_tab_name);
	    CSsuspend(CS_TIMEOUT_MASK, 10, 0);
	    TRdisplay("ONLINE MODIFY: %~t End sleeping\n",
		sizeof(DB_TAB_NAME), table_name.db_tab_name);
	}

	MEcopy((char *)&t->tcb_rel.relowner, sizeof(DB_OWN_NAME),
               (char *)&owner);

	duplicates = (t->tcb_rel.relstat & TCB_DUPLICATES) != 0;
	journal = (t->tcb_rel.relstat & TCB_JOURNAL) != 0;
	attr_count= t->tcb_rel.relatts;
	
        if (duplicates)
            setrelstat |= TCB_DUPLICATES;
	if (journal)
	{
	    setrelstat |= TCB_JOURNAL;
	    dm0l_flag |= DM0L_JOURNAL;
	}
	if (t->tcb_rel.relstat & TCB_SYS_MAINTAINED)
	    setrelstat |= TCB_SYS_MAINTAINED;

	if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	{
	    setrelstat |= TCB_IS_PARTITIONED;
	    dmu_part_def.nphys_parts = t->tcb_rel.relnparts;
	    dmu_part_def.ndims = t->tcb_rel.relnpartlevels;
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("ONLINE MODIFY: %~t is PARTITIONED\n",
			sizeof(DB_TAB_NAME), newtab_name.db_tab_name);
	}

	if ( m->mx_clustered )
	    setrelstat |= TCB_CLUSTERED;

	/* for now, just use the relstat2 from the unmodified relation */
	relstat2 = t->tcb_rel.relstat2;

	status = dm0m_allocate(sizeof (DMP_MISC) + 
		(i4)(t->tcb_rel.relatts * sizeof (DMF_ATTR_ENTRY)) + 
		(i4)(t->tcb_rel.relatts * sizeof(DMF_ATTR_ENTRY *)) + 
		(i4)(t->tcb_rel.relatts * sizeof(DM2U_KEY_ENTRY)) + 
		(i4)(t->tcb_rel.relatts * sizeof(DM2U_KEY_ENTRY *)),
		0, (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)0, 
		(DM_OBJECT **)&misc_cb, dberr);
	if (status != E_DB_OK)
	    break;

	attrs = (DMF_ATTR_ENTRY *)(misc_cb + 1);
	misc_cb->misc_data = (char*)attrs;
	attr_ptrs = (DMF_ATTR_ENTRY **)(attrs + (t->tcb_rel.relatts));
	keys = (DM2U_KEY_ENTRY *)(attr_ptrs + t->tcb_rel.relatts);
	key_ptrs = (DM2U_KEY_ENTRY **)(keys + t->tcb_rel.relatts);

	for ( i = 0; i < t->tcb_rel.relatts; i++ )
	{
	    attr_ptrs[i] = &attrs[i];
	    MEmove( t->tcb_atts_ptr[i+1].attnmlen, 
		t->tcb_atts_ptr[i+1].attnmstr,  ' ',
		DB_ATT_MAXNAME, attrs[i].attr_name.db_att_name);

	    attrs[i].attr_type = t->tcb_atts_ptr[i+1].type;
	    attrs[i].attr_size = t->tcb_atts_ptr[i+1].length;
	    attrs[i].attr_precision = t->tcb_atts_ptr[i+1].precision;
	    attrs[i].attr_flags_mask = t->tcb_atts_ptr[i+1].flag;
	    COPY_DEFAULT_ID( t->tcb_atts_ptr[i+1].defaultID,
		attrs[i].attr_defaultID );
	    attrs[i].attr_collID = t->tcb_atts_ptr[i+1].collID;
	    attrs[i].attr_geomtype = t->tcb_atts_ptr[i+1].geomtype;
	    attrs[i].attr_encflags = t->tcb_atts_ptr[i+1].encflags;
	    attrs[i].attr_encwid = t->tcb_atts_ptr[i+1].encwid;
	    attrs[i].attr_srid = t->tcb_atts_ptr[i+1].srid;
	    if (t->tcb_rel.relstat & TCB_INDEX)
	    {
		/* FIX ME building another key array because not sure if
		** btree non-key atts will show up otherwise 
		*/
		key_ptrs[i] = &keys[i];
		MEmove(t->tcb_atts_ptr[i+1].attnmlen, 
		    t->tcb_atts_ptr[i+1].attnmstr, ' ',
		    DB_ATT_MAXNAME, keys[i].key_attr_name.db_att_name);
	    }
	}

	if (t->tcb_rel.relstat & TCB_INDEX)
	{
	    /* Note creating temp base table like index (minus tidp column) */
	    attr_count -= 1; /* remove tidp for create table */
	    base_relstat2 = 0;
	}
	else
	    base_relstat2 = relstat2;

	/* Create the table */
	status = dm2u_create(dcb, xcb, &newtab_name, &owner,
		    tp->tpcb_loc_list, tp->tpcb_loc_count, 
		    &new_tabid, &new_ixid, 0, view,
		    (setrelstat), base_relstat2,
		    DB_HEAP_STORE, TCB_C_NONE,
		    t->tcb_rel.relwid, t->tcb_rel.reldatawid,
		    attr_count, attr_ptrs, db_lockmode,
		    allocation, extend, 
		    m->mx_page_type, m->mx_page_size,
		    (DB_QRY_ID *)0, (DM_PTR *)NULL,
		    (DM_DATA *)NULL, (i4)0, (i4)0,
		    (DMU_FROM_PATH_ENTRY *)NULL, (DM_DATA *)0, 
		    0, 0, (f8*)NULL, 0, &dmu_part_def, 0,
		    NULL /* DMU_CB */, dberr);

	if ((t->tcb_rel.relstat & TCB_INDEX) == 0)
	{
	    STRUCT_ASSIGN_MACRO(new_tabid, *modtemp_tabid);
	}
	else
	{
	    dm2u_index_cb.q_next = (DM2U_INDEX_CB *) 0;
	    dm2u_index_cb.q_prev = (DM2U_INDEX_CB *) 0;
	    dm2u_index_cb.length = sizeof(DM2U_INDEX_CB);
	    dm2u_index_cb.type = DM2U_IND_CB;
	    dm2u_index_cb.s_reserved = 0;
	    dm2u_index_cb.owner = 0;
	    dm2u_index_cb.ascii_id = DM2U_IND_ASCII_ID;
	    dm2u_index_cb.indxcb_dcb = dcb;
	    dm2u_index_cb.indxcb_xcb = xcb;
	    dm2u_index_cb.indxcb_index_name = &newix_name;
	    dm2u_index_cb.indxcb_location = tp->tpcb_loc_list;
	    dm2u_index_cb.indxcb_l_count = tp->tpcb_loc_count;
	    dm2u_index_cb.indxcb_tbl_id = &new_tabid;
	    dm2u_index_cb.indxcb_structure = m->mx_structure;
	    dm2u_index_cb.indxcb_i_fill = m->mx_ifill;
	    dm2u_index_cb.indxcb_l_fill = m->mx_lfill;
	    dm2u_index_cb.indxcb_d_fill = m->mx_dfill;
	    dm2u_index_cb.indxcb_compressed = m->mx_data_rac.compression_type;
	    dm2u_index_cb.indxcb_index_compressed = 0;
	    dm2u_index_cb.indxcb_unique = m->mx_unique;
	    dm2u_index_cb.indxcb_dmveredo = FALSE;
	    dm2u_index_cb.indxcb_min_pages = 0;
	    dm2u_index_cb.indxcb_max_pages = 0;
	    dm2u_index_cb.indxcb_kcount = kcount;
	    dm2u_index_cb.indxcb_key = (DM2U_KEY_ENTRY **)key_ptrs;
	    dm2u_index_cb.indxcb_acount = attr_count; /* acount = #atts */
	    dm2u_index_cb.indxcb_db_lockmode = db_lockmode;
	    dm2u_index_cb.indxcb_allocation = 0;
	    dm2u_index_cb.indxcb_extend = 0;
	    dm2u_index_cb.indxcb_page_type = m->mx_page_type;
	    dm2u_index_cb.indxcb_page_size = m->mx_page_size;
	    dm2u_index_cb.indxcb_idx_id = &new_ixid;
	    dm2u_index_cb.indxcb_range = (f8 *) NULL;
	    dm2u_index_cb.indxcb_index_flags = (i4)0;
	    dm2u_index_cb.indxcb_gwattr_array = (DM_PTR *)NULL;
	    dm2u_index_cb.indxcb_gwchar_array = (DM_DATA *)NULL;
	    dm2u_index_cb.indxcb_gwsource = (DMU_FROM_PATH_ENTRY *)NULL;
	    dm2u_index_cb.indxcb_qry_id = (DB_QRY_ID *)NULL;
	    dm2u_index_cb.indxcb_tup_info = &temp_tup_cnt;
	    dm2u_index_cb.indxcb_tab_name = 0;
	    dm2u_index_cb.indxcb_tab_owner = 0;
	    dm2u_index_cb.indxcb_reltups = 0;
	    dm2u_index_cb.indxcb_gw_id = 0;
	    dm2u_index_cb.indxcb_char_array = (DM_DATA *)0;
	    dm2u_index_cb.indxcb_relstat2 = relstat2;
	    dm2u_index_cb.indxcb_tbl_pri = 0;
	    dm2u_index_cb.indxcb_errcb = dberr;
	    status = dm2u_index(&dm2u_index_cb);

	    if (status != E_DB_OK)
		break;
	    STRUCT_ASSIGN_MACRO(new_ixid, *modtemp_tabid);
	}

	if (status != E_DB_OK)
	{
	    /*
	    ** If running on a checkpointed database that journaling has been
	    ** turned off on, then create will return with a warning telling
	    ** us that journaling will be enabled at the next checkpoint
	    ** (E_DM0101_SET_JOURNAL_ON).  Ignore this warning in sysmod.
	    */
	    if (dberr->err_code != E_DM0101_SET_JOURNAL_ON)
		break;

	    status = E_DB_OK;
	    CLRDBERR(dberr);
	}

	if (partitions)
	{
           /* kludge online ppchar array, to hold $online tabids */
           status = dm0m_allocate(sizeof (DMP_MISC) +
                        (m->mx_tpcb_count * sizeof(DMU_PHYPART_CHAR)),
                        DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
                        (char *)0, (DM_OBJECT **)&pp_misc_cb, dberr);
            if (status != E_DB_OK)
		break;
            o_partitions = (DMU_PHYPART_CHAR *)(pp_misc_cb + 1);
	    pp_misc_cb->misc_data = (char*)o_partitions;
	    MEcopy(partitions, (m->mx_tpcb_count * sizeof(DMU_PHYPART_CHAR)),
			o_partitions);
	}

	/* create $online partitions */
        if (m->mx_spcb_count > 1)
        {
	    MEcopy(newtab_name.db_tab_name, DB_TAB_MAXNAME, mastertab_name);
	    mastertab_name[DB_TAB_MAXNAME] = '\0';
	    for ( sp = m->mx_spcb_next;
		  sp && status == E_DB_OK;
		  sp = sp->spcb_next )
	    {
		setrelstat &= ~TCB_IS_PARTITIONED;
		base_relstat2 |= TCB2_PARTITION;
		newpart_tabid.db_tab_base = new_tabid.db_tab_base;
		newpart_tabid.db_tab_index = new_tabid.db_tab_base;

		/* create $online partitions to match targets */
		STprintf(newparttab_name, "ii%x pp%d-%s", 
			sp->spcb_partno + new_tabid.db_tab_base, 
			sp->spcb_partno, mastertab_name);

		/* create loc name array */
		status = dm0m_allocate(sizeof(DMP_MISC) + (sp->spcb_loc_count *
                        sizeof(DB_LOC_NAME)), 0,
                        (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)0,
                        (DM_OBJECT **)&loc_misc_cb, dberr);
		if (status != E_DB_OK)
		    break;

		locnArray = (DB_LOC_NAME *)(loc_misc_cb + 1);
		loc_misc_cb->misc_data = (char*)locnArray;

		for ( i = 0; i < sp->spcb_loc_count; i++)
		{
                    STRUCT_ASSIGN_MACRO(sp->spcb_locations[i].loc_name,
				locnArray[i]);
		}

		status = dm2u_create(dcb, xcb, 
		    (DB_TAB_NAME *)newparttab_name, &owner,
                    locnArray, sp->spcb_loc_count,
                    &newpart_tabid, &newpart_ixid, 0, view,
                    setrelstat, base_relstat2,
                    DB_HEAP_STORE, TCB_C_NONE,
		    t->tcb_rel.relwid, t->tcb_rel.reldatawid,
                    0, 0, db_lockmode,
                    allocation, extend,
                    m->mx_page_type, m->mx_page_size,
                    (DB_QRY_ID *)0, (DM_PTR *)NULL,
                    (DM_DATA *)NULL, (i4)0, (i4)0,
                    (DMU_FROM_PATH_ENTRY *)NULL, (DM_DATA *)0,
                    0, 0, (f8*)NULL, 0, 
		    &dmu_part_def, sp->spcb_partno,
		    NULL /* DMU_CB */, dberr);
	
		dm0m_deallocate((DM_OBJECT **)&loc_misc_cb);

		if (status != E_DB_OK) 
		{
		    if (dberr->err_code != E_DM0101_SET_JOURNAL_ON)
			break;
		    CLRDBERR(dberr);
		    status = E_DB_OK;
	 	}

		if (o_partitions)
		    STRUCT_ASSIGN_MACRO(newpart_tabid, 
				o_partitions[sp->spcb_partno].part_tabid);

		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: Using %~t partition %~t\n",
			sizeof(DB_TAB_NAME), newtab_name.db_tab_name,
			sizeof(DB_TAB_NAME), newparttab_name);
	    }
	    if (status)
		break;
        }
	
	/* continue create of new $online partitions */
	if (m->mx_spcb_count < m->mx_tpcb_count)
	{
	    status = create_new_parts(m, &newtab_name, &owner, 
			new_part_def, o_partitions, &new_tabid,
			db_lockmode, setrelstat, base_relstat2, 
			dberr);
	    if (status)
		break;
	}
 
	/* Open modify table readnolock */
	status = dm2t_open(dcb, &t->tcb_rel.reltid, DM2T_N,
	    DM2T_UDIRECT, DM2T_A_READ_NOCPN,
	    (i4)0, (i4)20, (i4)0, m->mx_log_id,
	    xcb->xcb_lk_id, (i4)0, (i4)0,
	    db_lockmode,
	    &xcb->xcb_tran_id, &timestamp, &readnolock_rcb, (DML_SCB *)0,
	    dberr);

	if (status != E_DB_OK)
	    break;

        /* allocate for online specific stuff */
        status = dm0m_allocate(sizeof (DMP_MISC) +
                (m->mx_tpcb_count * sizeof(DB_TAB_ID)) +
                (m->mx_spcb_count * sizeof(DM2U_OSRC_CB)) +
                (m->mx_tpcb_count * sizeof(DM2U_OPCB)), 
                DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
                (char *)0, (DM_OBJECT **)&omisc_cb, dberr);
        if (status != E_DB_OK)
	    break;

        /* establish dupchk tabid array */
        omcb.omcb_dupchk_tabid = (DB_TAB_ID *)(omisc_cb + 1);
	omisc_cb->misc_data = (char*)omcb.omcb_dupchk_tabid;
        omcb.omcb_osrc_next = osrc = 
		(DM2U_OSRC_CB *)((char *)omcb.omcb_dupchk_tabid +
                        (m->mx_tpcb_count * sizeof(DB_TAB_ID)));
        STRUCT_ASSIGN_MACRO(m->mx_bsf_lsn, omcb.omcb_bsf_lsn);
        omcb.omcb_opcb_next = opcb = (DM2U_OPCB *)((char *)osrc +
                        (m->mx_spcb_count * sizeof(DM2U_OSRC_CB)));

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: building online input (osrc) list\n");

	for ( i = 0; i < m->mx_spcb_count; i++)
	{
	    if (i)
		omcb.omcb_osrc_next[i-1].osrc_next = osrc;

	    osrc->osrc_next = (DM2U_OSRC_CB *)0;

	    if (m->mx_spcb_count == 1)
	    {
		/* single source */
		osrc->osrc_rnl_rcb = readnolock_rcb;
		osrc->osrc_master_rnl_rcb = readnolock_rcb;
		STRUCT_ASSIGN_MACRO(readnolock_rcb->rcb_tcb_ptr->tcb_rel.reltid,
				osrc->osrc_tabid);
		STRUCT_ASSIGN_MACRO(table_name, osrc->osrc_rnl_tabname);
		STRUCT_ASSIGN_MACRO(table_owner, osrc->osrc_rnl_owner);
	    }
	    else
	    {
		status = dm2t_open(dcb, 
		    &readnolock_rcb->rcb_tcb_ptr->tcb_pp_array[i].pp_tabid,
		    DM2T_N, DM2T_UDIRECT, DM2T_A_READ_NOCPN, 0, 20, 0,
		    m->mx_log_id, xcb->xcb_lk_id, (i4)0, (i4)0,
		    db_lockmode, &xcb->xcb_tran_id, &timestamp,
		    &(osrc->osrc_rnl_rcb), (DML_SCB *)0, dberr);
		if (status != E_DB_OK)
		    break;
		osrc->osrc_master_rnl_rcb = readnolock_rcb;
		osrc->osrc_partno = i;
		STRUCT_ASSIGN_MACRO(osrc->osrc_rnl_rcb->rcb_tcb_ptr->tcb_rel.reltid,
					osrc->osrc_tabid);
		STRUCT_ASSIGN_MACRO(osrc->osrc_rnl_rcb->rcb_tcb_ptr->tcb_rel.relid, 
					osrc->osrc_rnl_tabname);
		STRUCT_ASSIGN_MACRO(osrc->osrc_rnl_rcb->rcb_tcb_ptr->tcb_rel.relowner,
					osrc->osrc_rnl_owner);
	    }

	    /* Initialize DMP_RNL_LSN */
	    status = init_readnolock(m, osrc, dberr);
	    if (status != E_DB_OK)
		break;

	    osrc++;
	}

	/* Initialize MCB for dm2u_modify: */
	mcb->mcb_dcb = dcb;
	mcb->mcb_xcb = xcb;
	mcb->mcb_tbl_id = modtemp_tabid;
	mcb->mcb_omcb = &omcb;
	mcb->mcb_dmu = (DMU_CB*)NULL;
	mcb->mcb_structure = m->mx_structure;
	mcb->mcb_location = tp->tpcb_loc_list;
	mcb->mcb_l_count = tp->tpcb_loc_count;
	mcb->mcb_i_fill = m->mx_ifill;
	mcb->mcb_l_fill = m->mx_lfill;
	mcb->mcb_d_fill = m->mx_dfill;
	mcb->mcb_compressed = m->mx_data_rac.compression_type;
	mcb->mcb_index_compressed = m->mx_index_comp;
	mcb->mcb_temporary = FALSE;
	mcb->mcb_unique = m->mx_unique;
	mcb->mcb_merge = FALSE;
	mcb->mcb_clustered = m->mx_clustered;
	mcb->mcb_min_pages = m->mx_minpages;
	mcb->mcb_max_pages = m->mx_maxpages;
	mcb->mcb_modoptions = modoptions;
	mcb->mcb_mod_options2 = mod_options2;
	mcb->mcb_kcount = kcount;
	mcb->mcb_key = key;
	mcb->mcb_db_lockmode = db_lockmode;
	mcb->mcb_allocation = m->mx_allocation;
	mcb->mcb_extend = m->mx_extend;
	mcb->mcb_page_type = m->mx_page_type;
	mcb->mcb_page_size = m->mx_page_size;
	mcb->mcb_tup_info = tup_info;
	mcb->mcb_reltups = m->mx_reltups;
	mcb->mcb_tab_name = &newtab_name;
	mcb->mcb_tab_owner = &owner;
	mcb->mcb_has_extensions = has_extensions;
	mcb->mcb_relstat2 = relstat2;
	mcb->mcb_flagsmask = flagsmask;
	mcb->mcb_tbl_pri = m->mx_tbl_pri;
	mcb->mcb_rfp_entry = rfp_entry;
	mcb->mcb_new_part_def = new_part_def;
	mcb->mcb_new_partdef_size = new_partdef_size;
	mcb->mcb_partitions = o_partitions;
	mcb->mcb_nparts = nparts;
	mcb->mcb_verify = verify;


	/* modify $online table, input modtable readlock nolock */
	status = dm2u_modify(mcb, dberr);

	if (status != E_DB_OK)
	    break;

	/* log RNL LSN array */
        for ( osrc = omcb.omcb_osrc_next; osrc && (status == E_DB_OK);
	      osrc = osrc->osrc_next)
	{
	    /* determine how many lsns per log record */
	    rnl = &(osrc->osrc_rnl_online);
	    cnt = ((rnl->rnl_lsn_cnt > DM0L_MAX_LSNS_PER_REC) ? 
		DM0L_MAX_LSNS_PER_REC : rnl->rnl_lsn_cnt);

	    /* write out the rnl_lsn list into log records */
	    for ( logged = 0, still_to_log = rnl->rnl_lsn_cnt; 
		  still_to_log && (logged < rnl->rnl_lsn_cnt); )
	    {
		/* write out rnl pg lsns log record(s) */
		status = dm0l_rnl_lsn(m->mx_log_id, dm0l_flag, 
			&(osrc->osrc_tabid), 
			&(osrc->osrc_rnl_tabname),
			&(osrc->osrc_rnl_owner), 
			rnl->rnl_bsf_lsn, rnl->rnl_lastpage, 
			rnl->rnl_lsn_cnt, cnt, (PTR)(rnl->rnl_lsn + logged),
			(LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t dm0l_rnl_lsn totcnt=%d  cnt=%d  still_to_log=%d\n", 
			sizeof(DB_TAB_NAME), osrc->osrc_rnl_tabname.db_tab_name,
			rnl->rnl_lsn_cnt, cnt, still_to_log);
		logged+=cnt;
		still_to_log-=cnt;
		if (still_to_log < cnt)
		    cnt = still_to_log;
	    }
	}

	/* close readnolock partitions */
        for ( osrc = omcb.omcb_osrc_next; 
	      osrc && (status == E_DB_OK); osrc = osrc->osrc_next)
	{
	    if (osrc->osrc_rnl_rcb != readnolock_rcb)
	    {
		status = dm2t_close(osrc->osrc_rnl_rcb, DM2T_NOPURGE, 
				dberr);
	    }
	    osrc->osrc_rnl_rcb = (DMP_RCB *)0;
	}
	if (status != E_DB_OK)
	    break;

	/* close readnolock master */
	if (readnolock_rcb)
	{
	    status = dm2t_close(readnolock_rcb, DM2T_NOPURGE, dberr);
	    if (status != E_DB_OK)
		break;
	    readnolock_rcb = (DMP_RCB *)0;
	}

	/* Build persistent indices on $online table */
	if (dmu && dmu->q_next)
	    index_cbs = (DMU_CB *)dmu->q_next;
	else
	    index_cbs = (DMU_CB *)0;

	if (index_cbs)
	{
	    /* Count how many control block was passed */
	    for ( numberOfIndices = 0, ix_dmu = index_cbs; 
			status == E_DB_OK && ix_dmu; 
			      ix_dmu = ( DMU_CB * ) ix_dmu->q_next)
	    {
		numberOfIndices++;
		STRUCT_ASSIGN_MACRO(*modtemp_tabid, ix_dmu->dmu_tbl_id);
		if ((*xcb->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) != 0)
		    STprintf(index_name.db_tab_name, 
			"$ONLINE_%x_%x_IX_%x", (i4)
			m->mx_table_id.db_tab_base, m->mx_table_id.db_tab_index,
			numberOfIndices);
		else
		    STprintf(index_name.db_tab_name,
			"$online_%x_%x_ix_%x", (i4)
			m->mx_table_id.db_tab_base, m->mx_table_id.db_tab_index,
			numberOfIndices);

		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t index %~t -> %~t\n",
		    sizeof(DB_TAB_NAME), table_name.db_tab_name,
		    sizeof(DB_TAB_NAME), ix_dmu->dmu_index_name.db_tab_name,
		    sizeof(DB_TAB_NAME), index_name.db_tab_name);

		/* save the original idx name, to restore later */
		STRUCT_ASSIGN_MACRO(ix_dmu->dmu_index_name, 
					ix_dmu->dmu_online_name);
		STRUCT_ASSIGN_MACRO(index_name, ix_dmu->dmu_index_name);
		STmove(ix_dmu->dmu_index_name.db_tab_name, ' ', 
		    sizeof (ix_dmu->dmu_index_name.db_tab_name), 
		    ix_dmu->dmu_index_name.db_tab_name);
		ix_dmu->dmu_flags_mask |= DMU_ONLINE_START;
		ix_dmu->dmu_dupchk_tabid.db_tab_base = 0;
		ix_dmu->dmu_dupchk_tabid.db_tab_index = 0;
	    }
	    if (numberOfIndices > 1 && 
		(dmu->dmu_flags_mask & DMU_NO_PAR_INDEX) == 0)
	    {
		status = dmu_pindex(index_cbs);
		if (status != E_DB_OK)
		     *dberr = index_cbs->error;
	    }
	    else
	    {
		for (ix_dmu = index_cbs; 
			    status == E_DB_OK && ix_dmu; 
				    ix_dmu = (DMU_CB *)ix_dmu->q_next)
		{
		    status = dmu_index(ix_dmu);
		    if (status != E_DB_OK)
		    {
			*dberr = ix_dmu->error;
			break;
		    }
		}
	    }
	}

	if (status != E_DB_OK)
	    break;

	if ( get_online_rel(m, modtemp_tabid, index_cbs,
		o_partitions, db_lockmode, dberr) )
	    break;

	if (DMZ_SRT_MACRO(14))
	{
	    TRdisplay("ONLINE MODIFY: %~t Sleeping after reading table to allow concurrent updates\n",
		sizeof(DB_TAB_NAME), table_name.db_tab_name);
	    CSsuspend(CS_TIMEOUT_MASK, 10, 0);
	    TRdisplay("ONLINE MODIFY: %~t End sleeping\n",
		sizeof(DB_TAB_NAME), table_name.db_tab_name);
	}

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Request exclusive access to table now\n",
		sizeof(DB_TAB_NAME), table_name.db_tab_name);

	/* convert control lock */
	status = dm2t_control(dcb, m->mx_table_id.db_tab_base,
		xcb->xcb_lk_id, LK_X, (i4)0, timeout, dberr);
	if (status != E_DB_OK)
	    break;

	/* Get exclusive access to the table. */
	status = dm2t_open(dcb, &m->mx_table_id, DM2T_X, 
	    DM2T_UDIRECT, DM2T_A_MODIFY,
	    timeout, (i4)20, (i4)0, m->mx_log_id, 
	    xcb->xcb_lk_id, (i4)0, (i4)0, 
	    db_lockmode, 
	    &xcb->xcb_tran_id, &timestamp, &xrcb, (DML_SCB *)0, dberr);

	if (status != E_DB_OK)
	    break;

	status = dm2t_close(xrcb, DM2T_NOPURGE, dberr);
	xrcb = (DMP_RCB *)0;
	if (status != E_DB_OK)
	    break;

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Exclusive access to table granted\n",
		sizeof(DB_TAB_NAME), table_name.db_tab_name);

	status = dm0l_esf(m->mx_log_id, dm0l_flag, &t->tcb_rel.reltid, 
	    &table_name, &table_owner, (LG_LSN *)0, &m->mx_esf_lsn, dberr);
	if (status != E_DB_OK)
	    break;

	status = check_archiver(dcb, xcb, &table_name, &m->mx_esf_lsn, dberr);
	if (status != E_DB_OK)
	    break;

        /* create new partitions */
        if (m->mx_spcb_count < m->mx_tpcb_count)
        {
            status = create_new_parts(m, &t->tcb_rel.relid, &owner,
                        new_part_def, partitions, &m->mx_table_id,
                        db_lockmode, setrelstat, base_relstat2, 
			dberr);
            if (status)
                break;
        }

	/* Open $online table to logically reapply SF updates  */
	status = dm2t_open(dcb, modtemp_tabid, DM2T_X,
            DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, 0, 20, 0, m->mx_log_id,
            xcb->xcb_lk_id, (i4)0, (i4)0, db_lockmode, &xcb->xcb_tran_id, 
	    &timestamp, &opcb->opcb_master_rcb, (DML_SCB *)0, dberr);

	if (status != E_DB_OK)
	    break;

	opcb->opcb_next = 0;

	online_tcb = opcb->opcb_master_rcb->rcb_tcb_ptr;

	if (online_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)
	{
	    /* Open $online partitions to reapply SF updates */
	    for ( i = 0; i < online_tcb->tcb_rel.relnparts; i++)
	    {
		if (i)
		{
		    omcb.omcb_opcb_next[i-1].opcb_next = opcb;
		    opcb->opcb_next = 0;
		    opcb->opcb_master_rcb = 
			omcb.omcb_opcb_next->opcb_master_rcb;
		}

		status = dm2t_open(dcb,
			&online_tcb->tcb_pp_array[i].pp_tabid,
			DM2T_X, DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, 0,
			(i4)20, (i4)0, m->mx_log_id, xcb->xcb_lk_id,
			(i4)0, (i4)0, db_lockmode, &xcb->xcb_tran_id,
			&timestamp, &opcb->opcb_rcb, (DML_SCB *)0,
			dberr);
		if (status)
		    break;

		STRUCT_ASSIGN_MACRO(online_tcb->tcb_pp_array[i].pp_tabid,
			    opcb->opcb_tabid);
		opcb++;
	    }
	}
	else
	{
	    /* nonpartitioned $online */
	    STRUCT_ASSIGN_MACRO(*modtemp_tabid, opcb->opcb_tabid);
	    opcb->opcb_rcb = opcb->opcb_master_rcb;
	}

	status = do_sidefile(dcb, xcb, &table_name, &table_owner, m,
			&omcb, db_lockmode, tup_info, dberr);

	if (status != E_DB_OK)
	    break;

	/* sidefile has been deleted by do_sidefile succeeding */
	outstanding_sidefile = FALSE;

	/* Close PURGE partitions/master after doing sidefile updates */
	for ( opcb = omcb.omcb_opcb_next; opcb; opcb = opcb->opcb_next)
	{
	    /* phantom duplicate checking */
	    if (online_tcb->tcb_rel.relstat & TCB_UNIQUE)
	    {
		DB_TAB_ID	dup_tabid;
		i4		partno = opcb->opcb_rcb->rcb_tcb_ptr->tcb_partno;
		STRUCT_ASSIGN_MACRO(omcb.omcb_dupchk_tabid[partno], dup_tabid);
		if (dup_tabid.db_tab_base)
		{
		    /*
		    **if sorter noted duplicates, they had better have
		    ** been resolved during sidefile processing otherwise
		    ** this is bad
		    */
		    status = do_online_dupcheck(dcb, xcb, m, opcb->opcb_rcb,
				&dup_tabid, db_lockmode, dberr);
		    if (status)
			break;
		}
	    }
	    if (opcb->opcb_rcb != opcb->opcb_master_rcb)
	    {
		status = dm2t_close(opcb->opcb_rcb, DM2T_PURGE, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL,
                        ULE_LOG, NULL, (char *)NULL, (i4)0,
                        (i4 *)NULL, &error, 0);
		}
		opcb->opcb_rcb = (DMP_RCB *)0;
	    }
        }

	if (status != E_DB_OK)
	    break;

	if (omcb.omcb_opcb_next->opcb_master_rcb)
	{
	    status = dm2t_close(omcb.omcb_opcb_next->opcb_master_rcb, 
				DM2T_PURGE, dberr);
	    if (status != E_DB_OK)
		break;
	    omcb.omcb_opcb_next->opcb_master_rcb = (DMP_RCB *)0;
	}
	online_tcb = (DMP_TCB *)0;

	/* FIX ME destroy each persistent index and save DM2F_DES filename */
	if (index_cbs)
	{
	    /* phantom duplicate checking */
	    for(ix_dmu = index_cbs; status == E_DB_OK && ix_dmu && 
			ix_dmu->dmu_dupchk_tabid.db_tab_base;
		  ix_dmu = (DMU_CB *)ix_dmu->q_next)
            {
		DMP_RCB		*ix_rcb;
		/* check for duplicates */
		status = dm2t_open(dcb, &ix_dmu->dmu_idx_id, DM2T_N,
            			DM2T_UDIRECT, DM2T_A_READ_NOCPN, (i4)0, 
				(i4)20, (i4)0, m->mx_log_id,
            			xcb->xcb_lk_id, (i4)0, (i4)0,
            			db_lockmode, &xcb->xcb_tran_id, 
				&timestamp, &ix_rcb, (DML_SCB *)0,
				dberr);

		if (status != E_DB_OK)
		    break;

		status = do_online_dupcheck(dcb, xcb, m, ix_rcb,
				&ix_dmu->dmu_dupchk_tabid, db_lockmode, 
				dberr);
		if (status)
		    break;
	
		status = dm2t_close(ix_rcb, DM2T_NOPURGE, dberr);
		if (status)
		    break;
		ix_rcb = (DMP_RCB *)0;
		
	    }
	    if (status)
		break;

	    for ( ix_dmu = index_cbs; status == E_DB_OK && ix_dmu; 
		  ix_dmu = ( DMU_CB * ) ix_dmu->q_next)
	    {
		status = dm2u_destroy(dcb, xcb, &ix_dmu->dmu_idx_id, DM2T_X,
			DM2U_MODIFY_REQUEST, 0, 0, 0, 0, dberr);

		/* FIX ME it would help to put table_name in xccb */
		/* FIX ME ix_dmu will have table id */
		/* so when we use ix_dmu to create persistent index
		** on modtable - do file renames then -- 
		** using ix_dmu->dmu_idx_id to find xccb for DM2F_DES file */
		xccb = xcb->xcb_cq_next;

		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t persistent index %~t\n",
			sizeof(DB_TAB_NAME), table_name.db_tab_name,
			xccb->xccb_f_len, &xccb->xccb_f_name);

		if (status != E_DB_OK)
		    break;

		/* Restore modtable tabid the index_cbs */
		/* Restore index name in index_cbs or else they are
		** recreated with the name $online_tabid_ix_indexnum */

		STRUCT_ASSIGN_MACRO(dmu->dmu_tbl_id, ix_dmu->dmu_tbl_id);
		STRUCT_ASSIGN_MACRO(ix_dmu->dmu_online_name, 
						ix_dmu->dmu_index_name);
		/* use parent table row count */
		ix_dmu->dmu_on_reltups = *tup_info;
		ix_dmu->dmu_flags_mask &= ~(DMU_ONLINE_START);
	    }
	    if ( status )
	        break;
	}


	destroy_flag = DM2U_MODIFY_REQUEST;
	if ((m->mx_spcb_count > m->mx_tpcb_count)  && (m->mx_tpcb_count > 1))
	{
	    for (opcb = omcb.omcb_opcb_next; 
		 opcb && (status == E_DB_OK); 
		 opcb = opcb->opcb_next)
	    {
		status = dm2u_destroy(dcb, xcb, &opcb->opcb_tabid,
			DM2T_X, destroy_flag, 0, 0, 0, 0,
			dberr);
	    }
	    destroy_flag |= DM2U_ONLINE_REPART;
	}
	else if (m->mx_tpcb_count == 1)
	    destroy_flag |= DM2U_ONLINE_REPART;

	if ( status )
	    break;

	/* DESTROY $online table */
	status = dm2u_destroy(dcb, xcb, &new_tabid, DM2T_X,
			destroy_flag, 0, 0, 0, 0, dberr);
	if (status != E_DB_OK)
	    break;

	for (tp = m->mx_tpcb_next, opcb = omcb.omcb_opcb_next; 
	     tp && status == E_DB_OK; 
	     tp = tp->tpcb_next)
	{
	    DB_TAB_ID       *tabid;

	    if (m->mx_tpcb_count == 1)
		tabid = &t->tcb_rel.reltid;
	    else if (partitions)
	    {
		/* repartition, possibly new targets */
		tabid = &partitions[tp->tpcb_partno].part_tabid;
		if (tp->tpcb_tabid.db_tab_base == 0)
		    STRUCT_ASSIGN_MACRO(partitions[tp->tpcb_partno].part_tabid,
				tp->tpcb_tabid);
	    }
	    else
		tabid = &t->tcb_pp_array[tp->tpcb_partno].pp_tabid;

	    /* adjust tupcnt due to sidefile processing */
	    tp->tpcb_newtup_cnt += opcb[tp->tpcb_partno].opcb_tupcnt_adjust;

	    /* FIX ME rename $online.d00 files -> modtable.m00 */
	    for (k = 0; k < tp->tpcb_loc_count; k++)
	    {
		DM_FILE		mod_filename; /* for modtable */

		dm2f_filename(DM2F_MOD, tabid,
			tp->tpcb_locations[k].loc_id, 
			tp->tpcb_name_id,
			tp->tpcb_name_gen, &mod_filename);

		/*
		** Scan the xcb list of deleted files to find the 
		** DM2F_DES filename for each location
		** NOTE We can't recreate the filenames here because they
		** were generated using svcb->svcb_DM2F_TempTblCnt
		*/ 
		for ( xccb = xcb->xcb_cq_next, found_xccb = FALSE;
		      xccb != (DML_XCCB *)&xcb->xcb_cq_next;
		      xccb = xccb->xccb_q_next )
		{
		    if (!MEcmp((char *)&xccb->xccb_t_table_id,
			    &omcb.omcb_opcb_next[tp->tpcb_partno].opcb_tabid, 
			    sizeof(DB_TAB_ID))
			&& !MEcmp((char *)&xccb->xccb_p_name,
			    &tp->tpcb_locations[k].loc_ext->physical,
			    tp->tpcb_locations[k].loc_ext->phys_length))
		    {
			found_xccb = TRUE;
			break;
		    }
		}

		if (!found_xccb)
		{
		    TRdisplay("Can't find xccb for DM2F_DES file for loc %d\n",k);
		    /* *err_code = FIX_ME */
		    status = E_DB_ERROR;
		    break;
		}

		/* FIX ME RAW loc */

		/*
		** RENAME $online.DM2F_DES -> modtable.DM2F_MOD
		** Remove the pending XCCB_F_DELETE XCCB 
		** $online_xxx_0 created with nojournaling (see setrelstat)
		*/

		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t Rename %~t %~t\n",
		    sizeof(DB_TAB_NAME), table_name.db_tab_name,
		    xccb->xccb_f_len, &xccb->xccb_f_name, 
		    sizeof(mod_filename), &mod_filename);

		if (logging)
		{
		    status = dm0l_frename(m->mx_log_id, dm0l_flag,
			    &tp->tpcb_locations[k].loc_ext->physical,
			    tp->tpcb_locations[k].loc_ext->phys_length,
			    &xccb->xccb_f_name, &mod_filename, 
			    &t->tcb_rel.reltid, k, 
			    tp->tpcb_locations[k].loc_config_id,
			    (LG_LSN *)0, &lsn, dberr);
		    if (status != E_DB_OK)
			break;
		}

		status = dm2f_rename(m->mx_lk_id, m->mx_log_id,
		    &tp->tpcb_locations[k].loc_ext->physical,
		    tp->tpcb_locations[k].loc_ext->phys_length,
		    &xccb->xccb_f_name, xccb->xccb_f_len,
		    &mod_filename, (i4)sizeof(mod_filename),
		    &m->mx_dcb->dcb_name, dberr);
		if (status != E_DB_OK)
		    break;

		/*  Remove from queue. */
		xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
		xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;

		/*  Deallocate. */ 
		dm0m_deallocate((DM_OBJECT **)&xccb);
	    }
	}

	/* clean up rnl stuff hanging off of spcb */
	for (osrc = omcb.omcb_osrc_next; osrc; osrc = osrc->osrc_next)
	{
	    if (osrc->osrc_rnl_online.rnl_btree_xmap)
		dm0m_deallocate((DM_OBJECT **)&osrc->osrc_rnl_online.rnl_btree_xmap);
	    if (osrc->osrc_rnl_online.rnl_xmap)
		dm0m_deallocate((DM_OBJECT **)&osrc->osrc_rnl_online.rnl_xmap);
	    if (osrc->osrc_rnl_online.rnl_xlsn)
		dm0m_deallocate((DM_OBJECT **)&osrc->osrc_rnl_online.rnl_xlsn);
	}

	if (misc_cb)
	    dm0m_deallocate((DM_OBJECT **)&misc_cb);

	if (omisc_cb)
	    dm0m_deallocate((DM_OBJECT **)&omisc_cb);

	if (loc_misc_cb)
	    dm0m_deallocate((DM_OBJECT **)&loc_misc_cb);

	if (pp_misc_cb)
	    dm0m_deallocate((DM_OBJECT **)&pp_misc_cb);

	return (E_DB_OK);

    } /* end outer for. */

    for ( osrc = omcb.omcb_osrc_next; osrc; osrc = osrc->osrc_next)
    {
	if (osrc->osrc_rnl_online.rnl_btree_xmap)
	    dm0m_deallocate((DM_OBJECT **)&osrc->osrc_rnl_online.rnl_btree_xmap);
	if (osrc->osrc_rnl_online.rnl_xmap)
	    dm0m_deallocate((DM_OBJECT **)&osrc->osrc_rnl_online.rnl_xmap);
	if (osrc->osrc_rnl_online.rnl_xlsn)
	    dm0m_deallocate((DM_OBJECT **)&osrc->osrc_rnl_online.rnl_xlsn);

	if ( osrc->osrc_rnl_rcb && 
	    (osrc->osrc_rnl_rcb != readnolock_rcb) )
	{
	    local_status = dm2t_close(osrc->osrc_rnl_rcb, DM2T_NOPURGE, 
				&local_dberr);
	    if (local_status != E_DB_OK)
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (outstanding_sidefile)
    {
	local_status = dm0j_delete(DM0J_MERGE,
			(char *)&dcb->dcb_jnl->physical,
			dcb->dcb_jnl->phys_length, 0, &m->mx_table_id,
			&local_dberr);
	if (local_status != E_DB_OK)
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }

    if (readnolock_rcb)
    {
	local_status = dm2t_close(readnolock_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status != E_DB_OK)
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }

    if (xrcb)
	local_status = dm2t_close(xrcb, DM2T_NOPURGE, &local_dberr);
	
    /* close $online partition/master */
    for (opcb = omcb.omcb_opcb_next; opcb && opcb->opcb_rcb; 
	 opcb = opcb->opcb_next)
    {
        if (opcb->opcb_rcb != opcb->opcb_master_rcb)
            local_status = dm2t_close(opcb->opcb_rcb, DM2T_NOPURGE,
			&local_dberr);

    }

    if (omcb.omcb_opcb_next && omcb.omcb_opcb_next->opcb_master_rcb) 
	local_status = dm2t_close(omcb.omcb_opcb_next->opcb_master_rcb,
			DM2T_NOPURGE, &local_dberr);

    if (loc_misc_cb)
	dm0m_deallocate((DM_OBJECT **)&loc_misc_cb);

    if (omisc_cb != 0)
	dm0m_deallocate((DM_OBJECT **)&omisc_cb);

    if (pp_misc_cb != 0)
	dm0m_deallocate((DM_OBJECT **)&pp_misc_cb);

    if (misc_cb != 0)
	dm0m_deallocate((DM_OBJECT **)&misc_cb);

    if (status != E_DB_OK)
    {
	if ((dberr->err_code != E_DM0045_DUPLICATE_KEY) &&
	    (dberr->err_code != E_DM010C_TRAN_ABORTED) &&
	    (dberr->err_code != E_DM0065_USER_INTR) &&
	    (dberr->err_code != E_DM004B_LOCK_QUOTA_EXCEEDED) &&
	    (dberr->err_code != E_DM004D_LOCK_TIMER_EXPIRED) &&
            (dberr->err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(dberr, 0, E_DM9CA6_ONLINE_MODIFY);
	}
    }

    return (E_DB_ERROR);
}


static DB_STATUS
rebuild_persistent(
DMP_DCB		    *dcb,
DML_XCB		    *xcb,
DB_TAB_ID	    *table_id,
DMU_CB		    *dmu,
bool		    online_modify,
DB_TAB_ID	    *online_tab_id)
{
    DMU_CB		*index_cbs;
    DMU_CB		*ix_dmu;
    i4			numberOfIndices;
    DB_STATUS		status = E_DB_OK;

    if (dmu && (dmu->dmu_flags_mask & DMU_PIND_CHAINED) && dmu->q_next)
    {
	index_cbs = (DMU_CB *)dmu->q_next;
	/* Count how many control block was passed */
	for ( numberOfIndices = 0, ix_dmu = index_cbs; ix_dmu; 
	      ix_dmu = ( DMU_CB * ) ix_dmu->q_next)
	{
	    if (online_modify)
	    {
		/* This flag indicates that dmu_idx_id has table id
		** for $online_tabid_ix_indexnum persistent index file
		** which was created and dropped, so the .d00 file
		** should be used for the build
		*/
		ix_dmu->dmu_flags_mask |= DMU_ONLINE_END;
	    }
	    numberOfIndices++;
	}

	if (numberOfIndices > 1 && 
	    (dmu->dmu_flags_mask & DMU_NO_PAR_INDEX) == 0)
	{
	    status = dmu_pindex(index_cbs);
	    dmu->error = index_cbs->error;
	}
	else
	{
	    for (ix_dmu = index_cbs; ix_dmu; 
			    ix_dmu = (DMU_CB *)ix_dmu->q_next)
	    {
		/* oops... */
		ix_dmu->dmu_tran_id = dmu->dmu_tran_id;

		status = dmu_index(ix_dmu);
		if (status != E_DB_OK)
		{
		    dmu->error = ix_dmu->error;
		    break;
		}
	    }
	}
    }

    return(status);
}

static DB_STATUS
check_archiver(
DMP_DCB		    *dcb,
DML_XCB		    *xcb,
DB_TAB_NAME	    *table_name,
LG_LSN		    *lsn,
DB_ERROR	    *dberr)
{
    STATUS		cl_status = OK;
    CL_ERR_DESC		sys_err;
    LG_LA		purge_la;
    LG_LA		dbjnl_la;
    LG_DATABASE		database;
    i4		event;
    i4		event_mask;
    i4		length;
    i4             local_error;
    bool		have_transaction = FALSE;
    bool		purge_complete = FALSE;
    char		line_buffer[132];
    DB_STATUS		status = E_DB_OK;
    i4		error;

    /*
    ** Start Purge-Wait cycle:
    **
    **	Show the database and check the DUMP and Journal windows.
    **	Until the database has been completely processed up to the
    **	    purge address (recorded above), then signal an archive
    **	    cycle and wait for archive processing to complete.
    ** FIX ME NEED ALL NEW MESSAGES IN check_archiver
    */
    do
    {
	/*
	** Show the database to get its current Dump/Journal windows.
	*/
	database.db_id = (LG_DBID) dcb->dcb_log_id;
	cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
	    &length, &sys_err);
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, LG_S_DATABASE);
	    break;
	}

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Archiver Purge-Wait purge_la = %d %d %d\n",
		    sizeof(DB_TAB_NAME), table_name->db_tab_name,
		    database.db_l_jnl_la.la_sequence,
		    database.db_l_jnl_la.la_block,
		    database.db_l_jnl_la.la_offset);

	purge_la.la_sequence = database.db_l_jnl_la.la_sequence;
	purge_la.la_block    = database.db_l_jnl_la.la_block;
	purge_la.la_offset   = database.db_l_jnl_la.la_offset;

	/*
	** Start Purge-Wait cycle
	**
	**	Show the database and check the DUMP and Journal windows.
	**	Until the database has been completely processed up to the
	**	    purge address (recorded above), then signal an archive
	**	    cycle and wait for archive processing to complete.
	*/
	for ( ; ; )
	{
	    database.db_id = (LG_DBID) dcb->dcb_log_id;
	    cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
		&length, &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		    0, LG_S_DATABASE);
		break;
	    }

	    if (DMZ_SRT_MACRO(13))
		TRdisplay("ONLINE MODIFY: %~t Archiver Purge-Wait %d %d %d\n",
			sizeof(DB_TAB_NAME), table_name->db_tab_name,
			database.db_f_jnl_la.la_sequence,
			database.db_f_jnl_la.la_block,
			database.db_f_jnl_la.la_offset);

	    dbjnl_la.la_sequence = database.db_f_jnl_la.la_sequence;
	    dbjnl_la.la_block    = database.db_f_jnl_la.la_block;
	    dbjnl_la.la_offset   = database.db_f_jnl_la.la_offset;

	    /*
	    ** If the database dump window indicates there is no dump work
	    ** to process, then the purge wait is complete.
	    */
	    if ((database.db_f_jnl_la.la_sequence == 0) &&
		(database.db_f_jnl_la.la_block == 0) &&
		(database.db_l_jnl_la.la_sequence == 0) &&
		(database.db_l_jnl_la.la_block == 0))
	    {
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t Purge-wait complete (no work)\n",
			sizeof(DB_TAB_NAME), table_name->db_tab_name);
		purge_complete = TRUE;
		break;
	    }

	    /*
	    ** If the database has no more dump/journal work to do previous
	    ** to the purge address, then the purge wait is complete.
	    */
	    if (LGA_GT(&dbjnl_la, &purge_la))
	    {
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t Purge-wait complete (> purge la)\n",
			sizeof(DB_TAB_NAME), table_name->db_tab_name);
		purge_complete = TRUE;
		break;
	    }

	    /*
	    ** Check the state of the archiver.  If it is dead then we have
	    ** to fail.  If its alive but not running then give it a nudge.
	    */
	    event_mask = (LG_E_ARCHIVE | LG_E_START_ARCHIVER);

	    cl_status = LGevent(LG_NOWAIT,
		xcb->xcb_log_id, event_mask, &event, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error,
		    1, 0, event_mask);
		break;
	    }
	    else if (event & LG_E_START_ARCHIVER)
	    {
	    /* FIX ME New msg 
		uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
     */
		TRdisplay("DEAD ARCHIVER\n");
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If there is still archiver work to do, but the archiver is
	    ** is not processing our database, then signal an archive event.
	    */
	    if ( ! (event & LG_E_ARCHIVE))
	    {
		cl_status = LGalter(LG_A_ARCHIVE, (PTR)NULL, 0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &error,
			1, 0, LG_A_ARCHIVE);
		    break;
		}
	    }

	    /*
	    ** Wait a few seconds before rechecking the database/system states.
	    */
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("ONLINE MODIFY: %~t Archiver Purge-Wait Sleep\n",
			sizeof(DB_TAB_NAME), table_name->db_tab_name);
	    CSsuspend(CS_TIMEOUT_MASK, 1, 0);

	}

    } while (FALSE);

    if (cl_status != OK)
	status = E_DB_ERROR;

    if (status != E_DB_OK)
    {
	SETDBERR(dberr, 0, E_DM9CA5_ONLINE_ARCHIVER);
	uleFormat(dberr, 0, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    return (status);
}

/*
**	04-Apr-2005 (jenjo02)
**	      Pass appropriate TID from log record to find_part()
**	      when processing the sidefile. The TID provides
**	      a deterministic way of figuring out AUTOMATIC
**	      partitions.
**	13-Feb-2008 (kschendel) SIR 122739
**	    Revise uncompress call, remove record-type and tid.
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
*/
static DB_STATUS
do_sidefile(
DMP_DCB		    *dcb,
DML_XCB		    *xcb,
DB_TAB_NAME	    *table_name,
DB_OWN_NAME	    *table_owner,
DM2U_MXCB           *mxcb,
DM2U_OMCB	    *omcb,
i4		    db_lockmode,
i4		    *tup_info,
DB_ERROR	    *dberr)
{
    DM2U_MXCB		*m = mxcb;
    DM2U_OPCB		*opcb_list = omcb->omcb_opcb_next;
    DMP_TCB		*t = opcb_list->opcb_master_rcb->rcb_tcb_ptr;
    DB_STATUS		status;
    DB_STATUS		local_status;
    i4			local_error;
    i4			error;
    DM0J_CTX		*jnl = (DM0J_CTX *)0;
    DM0L_HEADER		*record;
    i4			l_record;
    i4			rec_cnt;
    DM_TID		tid;
    char		*log_row;
    char		*log_new_row;
    DM_OBJECT 		*mem_ptr = 0;
    i4			mem_needed;
    char		*rowbuf;
    char		*new_row_buf;
    char		*old_row, *new_row;
    DM2R_KEY_DESC	*key_desc; 
    DMP_TCB		*it;
    DMP_TCB		*position_tcb = (DMP_TCB *)0;
    DMP_RCB		*position_rcb = (DMP_RCB *)0;
    DB_TAB_TIMESTAMP    timestamp;
    i4			dm2r_flag;
    DM0L_PUT		*put;
    DM0L_DEL		*del;
    DM0L_REP		*rep;
    DM0L_DEALLOC	*dall;
    DM_PAGENO		log_page;
    DM_PAGENO		log_page2;
    LG_LSN		*log_lsn;
    bool		redo = FALSE, redo2 = FALSE;
    bool		is_compressed;
    char		*origrec; 
    char		*newrec;  
    char		*wrksp;  
    i4			uncompressed_length;
    u_i2		row_version;
    i4                  ndata_len, nrec_size, diff_offset;
    bool		comprec;
    DM2U_FREEPG_CB	*freed_pgq = NULL;
    DM2U_FREEPG_CB	*cand;
    DMP_RCB		*src_rcb = m->mx_rcb;
    DMP_TCB		*src_tcb = src_rcb->rcb_tcb_ptr;
    i4			record_size = src_rcb->rcb_proj_relwid;

    DB_TAB_ID		log_tabid;
    i4			i;

    DMP_MISC		*pos_misc_cb = (DMP_MISC *)0;
    DMP_MISC		*fpsrc_misc_cb = (DMP_MISC *)0;
    DM2U_OPCB		*pos_opcb = (DM2U_OPCB *)0;
    DM2U_OPCB		*pos_opcb_list = (DM2U_OPCB *)0;
    DMP_RCB		*rcb = (DMP_RCB *)0;
    DM2U_OPCB		*src_part_list = (DM2U_OPCB *)0;
    /* old TID, new TID from log record */
    DM_TID		*otid, *ntid;
    DB_ERROR		local_dberr;
  
    do 
    {
	/* Open Sidefile */
	status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical, 
		dcb->dcb_jnl->phys_length, &dcb->dcb_name,
		dcb->dcb_jnl_block_size, 0, 0, 0, DM0J_M_READ, -1, DM0J_FORWARD,
		&m->mx_table_id, (DM0J_CTX **)&jnl, dberr);

	if (status != E_DB_OK)
	{
	    TRdisplay("ONLINE MODIFY: %~t Can't open sidefile\n",
		    sizeof(DB_TAB_NAME), table_name->db_tab_name);
	    break;
	}

	/* For compression type, we can use source RCB's rowaccessor rather
	** than worrying about what is in the log records.  (We have the
	** advantage here over dmve that the source RCB is still open.)
	*/
	is_compressed = (src_rcb->rcb_data_rac->compression_type != TCB_C_NONE);

	/* 
	** Find best index to use to find old record for DM0LDEL and DM0LREP
	** Use primary if it is unique
	** Otherwise use smallest unique secondary index
	** 
	** If no unique index exists the online modify sidefile processing 
	** for DM0LDEL and DM0LREP may perform poorly. DMF will need help from
	** the optimizer in choosing an index for sidefile processing.
	*/
	if (t->tcb_rel.relstat & TCB_UNIQUE)
	    position_tcb = t;
	else
	{
	    for (it = t->tcb_iq_next;
		it != (DMP_TCB*)&t->tcb_iq_next;
		it = it->tcb_q_next)
	    {
		if (it->tcb_rel.relstat & TCB_UNIQUE)
		{
		    if (!position_tcb)
			position_tcb = it;
		    else if (it->tcb_klen < position_tcb->tcb_klen)
			position_tcb = it;
		}
	    }
	}

	if (!position_tcb)
	{
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("ONLINE MODIFY: %~t WARNING no unique index for sf processing\n",
		    sizeof(DB_TAB_NAME), table_name->db_tab_name);
	    position_tcb = t;
	}
	else
	{
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("ONLINE MODIFY: %~t Best index for sf processing %~t\n",
		    sizeof(DB_TAB_NAME), table_name->db_tab_name,
		    sizeof(DB_TAB_NAME), position_tcb->tcb_rel.relid.db_tab_name);
	}

	/*
	** for now init to master, until we figure out which partition if
	** any to use
	*/
	if (position_tcb == t)
	{
	    /* positioning using $online table or partitions */
	    /* init online_pos master and parts to that of $online */
	    dm2r_flag = DM2R_BYPOSITION;
	    pos_opcb = pos_opcb_list = opcb_list;
	}
	else
	{
	    i4	mem_needed = sizeof(DMP_MISC);
	
	    if (position_tcb->tcb_rel.relnparts == 0)
		mem_needed += sizeof(DM2U_OPCB);
	    else
		mem_needed += (position_tcb->tcb_rel.relnparts * 
				sizeof(DM2U_OPCB));

            status = dm0m_allocate(mem_needed, DM0M_ZERO, (i4)MISC_CB, 
			(i4)MISC_ASCII_ID, (char *)0, 
			(DM_OBJECT **)&pos_misc_cb, dberr);
            if (status)
		break;

            pos_opcb = pos_opcb_list = (DM2U_OPCB *)(pos_misc_cb + 1);
	    pos_misc_cb->misc_data = (char*)pos_opcb;

	    /* positioning using index */
	    dm2r_flag = DM2R_BYTID;
	    status = dm2t_open(dcb, &position_tcb->tcb_rel.reltid, DM2T_X,
		DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, 0, 20, 0, m->mx_log_id,
		xcb->xcb_lk_id, (i4)0, (i4)0,
		db_lockmode,
		&xcb->xcb_tran_id, &timestamp, &position_rcb, (DML_SCB *)0,
		dberr);
	    if (status != E_DB_OK)
	    {
		TRdisplay("ONLINE MODIFY: %~t Can't open %~t (%x)\n",
		    sizeof(DB_TAB_NAME), table_name->db_tab_name,
		    sizeof(DB_TAB_NAME), position_tcb->tcb_rel.relid.db_tab_name,
		    dberr->err_code);
		break;
	    }

	    /*
	    ** NOTE: currently idx can not be partitioned, this is
	    ** just here for future implementation 
	    */
	    pos_opcb->opcb_next = (DM2U_OPCB *)0;
	    
	    /* allocate for array of positioning tbl partitions */
	    if (position_tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		/* index is partitioned, position using partitions */
		for (i = 0; i < position_tcb->tcb_rel.relnparts; i++)
		{
		    if (i)
		    {
			pos_opcb_list[i-1].opcb_next = pos_opcb;
		    }
		    pos_opcb->opcb_next = (DM2U_OPCB *)0;

		    status = dm2t_open(dcb,
				&position_tcb->tcb_pp_array[i].pp_tabid,
				DM2T_X, DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, 0,
				(i4)20, (i4)0, m->mx_log_id, xcb->xcb_lk_id,
				(i4)0, (i4)0, db_lockmode, &xcb->xcb_tran_id,
				&timestamp, &pos_opcb->opcb_rcb,
				(DML_SCB *)0, dberr);
		    if (status)
			break;
		    pos_opcb->opcb_master_rcb = position_rcb;
		    pos_opcb++;
		}
	    }
	    else
	    {
		/* non partitioned index */
		pos_opcb->opcb_rcb = position_rcb;
		pos_opcb->opcb_master_rcb = position_rcb;
	    }
	}

	/*
	** Allocate a buffer for DM0LREP unpacked new row (new_row_buf)
	** a buffer to use for position to old row (row_buf)
	** a buffer to hold uncompressed old DM0LREP row (origrec)
	** a buffer to hold uncompressed new DM0LREP row (newrec)
	** a buffer to hold DM0LREP CLR after image delta (wrksp)
	** a DM2R_KEY_DESC array for positon_to_row (key_desc)
	*/
        mem_needed = sizeof(DMP_MISC) + (5 * record_size) +
			(position_tcb->tcb_keys * sizeof(DM2R_KEY_DESC));

        status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB,
            (i4)MISC_ASCII_ID, (char *)0, &mem_ptr, dberr);
        if (status != OK)
        {
            uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
            uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
                NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
                0, mem_needed);
            break;
        }
        else
        {
	    key_desc = (DM2R_KEY_DESC *)((char *)mem_ptr + sizeof(DMP_MISC));
	    ((DMP_MISC*)mem_ptr)->misc_data = rowbuf;
            rowbuf = (char *)key_desc + 
			(position_tcb->tcb_keys * sizeof(DM2R_KEY_DESC));
            new_row_buf = rowbuf + record_size;
            origrec = new_row_buf + record_size;
            newrec = origrec + record_size;
            wrksp = newrec + record_size;
        }

	if (DMZ_SRT_MACRO(13))
	{
	    DM2U_OSRC_CB	*osrc;
	    TRdisplay("ONLINE MODIFY: %~t Begin sidefile processing\n",
		sizeof(DB_TAB_NAME), table_name->db_tab_name);

	    for (osrc = omcb->omcb_osrc_next; osrc; osrc = osrc->osrc_next)
	    {
	    TRdisplay("ONLINE MODIFY: %~t (%~t) Readnolock pages read %d lsn>bsf %d\n",
		sizeof(DB_TAB_NAME), table_name->db_tab_name,
		sizeof(DB_TAB_NAME), osrc->osrc_rnl_tabname.db_tab_name,
		osrc->osrc_rnl_online.rnl_read_cnt, 
		osrc->osrc_rnl_online.rnl_lsn_cnt);
	    }
	}
			
	for ( rec_cnt = 0, comprec = FALSE; 
	      status == E_DB_OK; 
	      rec_cnt++, comprec = FALSE, redo = redo2 = FALSE)
	{
	    status = dm0j_read(jnl, (PTR *)&record, &l_record, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(dberr);
		    break;
		}
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
			    &error, 0);
		/* FIX ME new error ->*err_code = E_DM1305_RFP_JNL_OPEN; */
		break;
	    }

	    /* Skip records not needed for REDO processing */
	    if (record->flags & (DM0L_NONREDO | DM0L_DUMP))
	    {
		if (DMZ_SRT_MACRO(13))
		    TRdisplay("ONLINE MODIFY: %~t Skip NONREDO log record\n",
			sizeof(DB_TAB_NAME), table_name->db_tab_name);
		continue;
	    }

	    /* check for interrupts */
	    if (status = dm2u_check_for_interrupt(m->mx_xcb, dberr))
		break;

	    /* FIX ME check lsn > mx_bsf_lsn */
	    /* FIX ME check lsn < mx_esf_lsn */

	    /* FIX ME apply sf filter logic */

	    /* Don't worry about log record comptype, use src rcb. */
	    if (record->type == DM0LPUT)
	    {
		put = (DM0L_PUT *)record;
		STRUCT_ASSIGN_MACRO(put->put_tbl_id, log_tabid);
		log_page = put->put_tid.tid_tid.tid_page;
		log_lsn = &put->put_header.lsn;
		log_row = ((char *)put) + sizeof(DM0L_PUT);
		row_version = put->put_row_version;
		otid = ntid = &put->put_tid;
	    }
	    else if (record->type == DM0LDEL)
	    {
		del = (DM0L_DEL *)record;
		STRUCT_ASSIGN_MACRO(del->del_tbl_id, log_tabid);
		log_page = del->del_tid.tid_tid.tid_page;
		log_lsn = &del->del_header.lsn;
		log_row = ((char *)del) + sizeof(DM0L_DEL);
		row_version = del->del_row_version;
		otid = ntid = &del->del_tid;
	    }
	    else if (record->type == DM0LREP) /* FIX ME replace check old and new pages */
	    {
		rep = (DM0L_REP *)record;
		STRUCT_ASSIGN_MACRO(rep->rep_tbl_id, log_tabid);
		log_page = rep->rep_otid.tid_tid.tid_page;
		log_page2 = rep->rep_ntid.tid_tid.tid_page;
		log_lsn = &rep->rep_header.lsn;
		log_row = ((char *)rep) + sizeof(DM0L_REP);
		log_new_row = log_row + rep->rep_odata_len;
		otid = &rep->rep_otid;
		ntid = &rep->rep_ntid;
	    }
	    else if (record->type == DM0LDEALLOC)
	    {
		if (freed_pgq)
		{
		    dall = (DM0L_DEALLOC *)record;
		    STRUCT_ASSIGN_MACRO(dall->dall_tblid, log_tabid);
		    log_page = dall->dall_free_pageno;
		    for (cand = freed_pgq; cand != NULL; cand = cand->q_next)
		    {
			if (!MEcmp(&log_tabid, &cand->freepg_tabid, 
					sizeof(DB_TAB_ID)) && 
			    (log_page == cand->freepg_no))
			{
			    if (cand->q_next != NULL)
				cand->q_next->q_prev = cand->q_prev;
			    if (cand->q_prev != NULL)
				cand->q_prev->q_next = cand->q_next;
			    else
				freed_pgq = cand->q_next;

			    if (DMZ_SRT_MACRO(13))
				TRdisplay("do_sidefile: %~t dealloc log rec for pg = %d\n", 
					sizeof(DB_TAB_NAME), table_name->db_tab_name,
					log_page);
			    dm0m_deallocate((DM_OBJECT **)&cand);
			    break;
			}
		    }
		}
		continue;
	    }

	    status = test_redo(omcb, record, &log_tabid, log_page, 
			log_lsn, &redo, dberr);
	    if (status != E_DB_OK)
		break;
	    if (record->type == DM0LREP)
	    {
		if (log_page == log_page2)
		    redo2 = redo;
		else
		{
		    status = test_redo(omcb, record, &log_tabid, log_page2, 
				log_lsn, &redo2, dberr);
		    if (status != E_DB_OK)
			break;
		}
	    }

	    /* DEBUG */
	    if (DMZ_SRT_MACRO(13))
		dmd_log(FALSE, (PTR)record, dcb->dcb_jnl_block_size);

	    if (!redo && !redo2)
	    {
		/* Was unconditional dmd-log of record; trdisplay??? */
		continue;
	    }

	    if ( (record->type == DM0LPUT && !(record->flags & DM0L_CLR)) ||
		 (record->type == DM0LDEL && (record->flags & DM0L_CLR)) )
	    {
		if (is_compressed)
		{
		    status = (*src_rcb->rcb_data_rac->dmpp_uncompress)(
				src_rcb->rcb_data_rac,
				log_row, origrec,
				record_size, &uncompressed_length,
				&src_rcb->rcb_tupbuf, row_version,
				src_rcb->rcb_adf_cb);
		    log_row = origrec;
		}

		    status = find_part(m, &log_tabid, log_row, &i, 
					ntid, dberr);
		    if (status)
			break;

		/* Don't do duplicate checking */
		/* DM2R_REDO_SF: Don't do blob processing */
		status = dm2r_put(opcb_list[i].opcb_rcb, 
				DM2R_DUPLICATES | DM2R_REDO_SF, 
				log_row, dberr);

		opcb_list[i].opcb_tupcnt_adjust++;
		(*tup_info)++;
	    }
	    else if (record->type == DM0LDEL ||
		(record->type == DM0LPUT && (record->flags & DM0L_CLR)))
	    {
		if (record->flags & DM0L_CLR)
		{
		    /*
		    ** put CLR log rec does not contain rec image 
		    ** locate compensated log rec to extract image 
		    */
		    status = find_compensated(m, record, wrksp, (i4 *)0, (i4 *)0, 
					(i4 *)0, dberr);
		    if (status)
		    {
			if (DMZ_SRT_MACRO(13))
			    TRdisplay("ONLINE MODIFY: %~t find_compensated failed complsn=(%x,%x)\n",
				sizeof(DB_TAB_NAME), table_name->db_tab_name,
				record->compensated_lsn.lsn_high,
				record->compensated_lsn.lsn_low);
			break;
		    }
		    log_row = wrksp;
		}

		if (is_compressed)
		{
		    status = (*src_rcb->rcb_data_rac->dmpp_uncompress)(
				src_rcb->rcb_data_rac,
				log_row, origrec,
				record_size, &uncompressed_length,
				&src_rcb->rcb_tupbuf, row_version,
				src_rcb->rcb_adf_cb);
		    log_row = origrec;
		}

                    status = find_part(m, &log_tabid, log_row, &i, 
					otid, dberr);
                    if (status)
                        break;

		status = position_to_row(m, pos_opcb_list[i].opcb_rcb, 
				opcb_list[i].opcb_rcb, 
				log_row, rowbuf, &key_desc[0], &tid, 
				dberr);
		if (status == E_DB_OK)
		{
		    status = dm2r_delete(opcb_list[i].opcb_rcb, &tid, 
			    dm2r_flag | DM2R_REDO_SF, dberr);
		    opcb_list[i].opcb_tupcnt_adjust--;
		    (*tup_info)--;
		}
		else if (dberr->err_code == E_DM0055_NONEXT)
		{
		    DB_STATUS		local_stat = E_DB_OK;
		    bool		found = FALSE;

		    /* 
		    ** This may be special case in which disassociated data
		    ** page had been deallocated before we were able to read it.
		    ** Save pg #. If we don't encounter the dealloc log 
		    ** record by the end of sidefile processing, then err.
		    */
		    status = E_DB_OK;
		    CLRDBERR(dberr);

		    /* do we already have an entry in free pg queue for this page? */
		    for (cand = freed_pgq; cand != NULL; cand = cand->q_next)
		    {
			if ( !MEcmp(&log_tabid, &cand->freepg_tabid, 
					sizeof(DB_TAB_ID)) && 
			     (log_page == cand->freepg_no))
			{
			    found = TRUE;
			    break;
			}
		    }

		    if (!found)
		    {
			local_status = dm0m_allocate(sizeof(DM2U_FREEPG_CB),
					0, (i4)DM2U_FPG_CB, (i4)DM2U_FPGCB_ASCII_ID, (char *)0, 
					(DM_OBJECT **)&cand, &local_dberr);

			if (local_status)
			    status = local_status;

			cand->freepg_no = log_page;
			STRUCT_ASSIGN_MACRO(log_tabid, cand->freepg_tabid);

			/* put entry on free page queue */
			cand->q_next = freed_pgq;
			cand->q_prev = NULL;
			if (freed_pgq)
			    freed_pgq->q_prev = cand;
			freed_pgq = cand;
			if (DMZ_SRT_MACRO(13))
			    TRdisplay("do_sidefile: %~t E_DM0055_NONEXT for delete on pg = %d\n",
					sizeof(DB_TAB_NAME), table_name->db_tab_name,
					log_page);
		    }
		}
	    }
	    else if (record->type == DM0LREP)
	    {
		/*
		** Determine if the full row before image was logged.  
		** If trace point DM902 was set when the record was logged
		** only the changed bytes of the before image were logged.
		** - in which case we have to extract the old row from the page
		** to build the old and new row versions
		** This is impossible to do with logical redo recovery
		*/
		if (rep->rep_header.flags & DM0L_COMP_REPL_OROW)
		{
		    TRdisplay("Cannot reconstruct old compressed row\n");
		    status = E_DB_ERROR;
		    break;
		}

		if (record->flags & DM0L_CLR)
		{
                    /* 
                    ** rep CLR log rec does not contain after image delta
		    ** locate compensated log rec to extract after image delta
                    */
		    status = find_compensated(m, record, wrksp, &ndata_len, 
					&nrec_size, &diff_offset, dberr);
		    if (status)
		    {
			if (DMZ_SRT_MACRO(13))
			    TRdisplay("ONLINE MODIFY: %~t find_compensated failed complsn=(%x,%x)\n",
				sizeof(DB_TAB_NAME), table_name->db_tab_name,
				record->compensated_lsn.lsn_high,
				record->compensated_lsn.lsn_low);
			break;
		    }
		    log_new_row = wrksp;
		    comprec = TRUE;
		}

		/* Construct new row using the old row and changed bytes */
		dm0l_row_unpack(log_row, rep->rep_orec_size, log_new_row, 
				comprec ? ndata_len : rep->rep_ndata_len,
				new_row_buf, 
				comprec ? nrec_size : rep->rep_nrec_size, 
				comprec ? diff_offset : rep->rep_diff_offset);
		new_row = new_row_buf;

                if (is_compressed)
                {
		    status = (*src_rcb->rcb_data_rac->dmpp_uncompress)(
				src_rcb->rcb_data_rac,
				log_row, origrec,  
				record_size, &uncompressed_length,
				&src_rcb->rcb_tupbuf, rep->rep_orow_version,
				src_rcb->rcb_adf_cb);
		    old_row = origrec;

		    status = (*src_rcb->rcb_data_rac->dmpp_uncompress)(
				src_rcb->rcb_data_rac,
				new_row_buf, newrec,
				record_size, &uncompressed_length,
				&src_rcb->rcb_tupbuf, rep->rep_nrow_version,
				src_rcb->rcb_adf_cb);
		    new_row = newrec;
                }
		else
		    /* Get pointer to full row before image */
		    old_row = log_row;

		if (record->flags & DM0L_CLR)
		{
		    char	*tmp = old_row;
		    bool	tmp_redo = redo;
	
		    old_row = new_row;
		    new_row = tmp;
		    redo = redo2;
		    redo2 = tmp_redo;
		}

		if (redo)
		{
                    status = find_part(m, &log_tabid, old_row, &i, 
					otid, dberr);
                    if (status)
                        break;

		    status = position_to_row(m, pos_opcb_list[i].opcb_rcb, 
				opcb_list[i].opcb_rcb, 
				old_row, rowbuf, &key_desc[0], &tid, 
				dberr);

		    if (status == E_DB_OK)
		    {
			status = dm2r_delete(opcb_list[i].opcb_rcb, &tid, 
				dm2r_flag | DM2R_REDO_SF, dberr);
		    }
		}

		if (redo2)
		{
		    /* Don't do duplicate checking */
		    /* DM2R_REDO_SF: Don't do blob processing */
		    if (status == E_DB_OK)
		    {
			status = find_part(m, &log_tabid, new_row, &i, 
					ntid, dberr);
			if (status)
			    break;

			status = dm2r_put(opcb_list[i].opcb_rcb, 
			    DM2R_DUPLICATES | DM2R_REDO_SF, new_row, dberr);
		    }
		}
	    }
	}

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Finished sidefile processing records=%d\n",
		sizeof(DB_TAB_NAME), table_name->db_tab_name, rec_cnt);

    } while (FALSE); 

    if (pos_misc_cb)
    {
	for ( pos_opcb = pos_opcb_list; 
	      pos_opcb; 
	      pos_opcb = pos_opcb->opcb_next)
	{
	    if (pos_opcb->opcb_rcb)
	    {
		local_status = dm2t_close(pos_opcb->opcb_rcb, DM2T_NOPURGE,
				&local_dberr);
		if (local_status != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, (i4)0,
			    (i4 *)NULL, &error, 0);
		    status = local_status;
		}
            }
        }
    dm0m_deallocate((DM_OBJECT **)&pos_misc_cb);
    }

    /* Close Sidefile */
    if (jnl)
    {
	local_status = dm0j_close(&jnl, &local_dberr);

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Close sidefile status %d\n",
		sizeof(DB_TAB_NAME), table_name->db_tab_name, local_dberr.err_code);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    status = local_status;
	}
	else
	{
	    local_status = dm0j_delete(DM0J_MERGE, 
			(char *)&dcb->dcb_jnl->physical, 
			dcb->dcb_jnl->phys_length, 0, &m->mx_table_id, 
			&local_dberr);
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("ONLINE MODIFY: %~t Delete sidefile status %d\n",
		    sizeof(DB_TAB_NAME), table_name->db_tab_name, local_dberr.err_code);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		status = local_status;
	    }
	}
    }

    if (mem_ptr != 0) 
	dm0m_deallocate(&mem_ptr);

    if (freed_pgq)
    {
	if (src_tcb->tcb_rel.relnparts)
	{
	    local_status = dm0m_allocate(sizeof(DMP_MISC) +
			(src_tcb->tcb_rel.relnparts * sizeof(DM2U_OPCB)),
			DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
                        (char *)0, (DM_OBJECT **)&fpsrc_misc_cb, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		status = local_status;
	    }
	    src_part_list = (DM2U_OPCB *)(fpsrc_misc_cb + 1);
	    fpsrc_misc_cb->misc_data = (char*)src_part_list;
	}
	else
	{
	    rcb = src_rcb;
	    t = rcb->rcb_tcb_ptr;
	}
    }

    while (freed_pgq)
    {
	/*
	** Entries still on the freed page queue.
	** Check if disassociated page is empty, but was not deallocated
	** due to row level locking session unable to obtain X page lock.
	** Empty disassociated data page should still exist in table.
	** Deallocate pg entry from queue and return the E_DM0055_NONEXT now 
	** if appropriate.
	*/
        cand = freed_pgq;
	
	if (status == E_DB_OK)
	{
	    if (src_tcb->tcb_rel.relnparts)
	    {
		/* figure out which partition we need to open & check */
		for ( i = 0; i < src_tcb->tcb_rel.relnparts; i++)
		{
		    if (!MEcmp(&src_tcb->tcb_pp_array[i].pp_tabid, 
				&cand->freepg_tabid, sizeof(DB_TAB_ID)))
		    {
			if ( src_part_list[i].opcb_rcb == (DMP_RCB *)NULL )
			{
			    local_status = dm2t_open(dcb, 
				&src_tcb->tcb_pp_array[i].pp_tabid, 
				DM2T_N, DM2T_UDIRECT, DM2T_A_READ_NOCPN,
				(i4)0, (i4)20, (i4)0, m->mx_log_id,
				xcb->xcb_lk_id, (i4)0, (i4)0,
				db_lockmode, &xcb->xcb_tran_id, 
				&timestamp, &src_part_list[i].opcb_rcb,
				(DML_SCB *)0, dberr);
			    if (local_status != E_DB_OK)
			    {
				status = local_status;
				break;
			    }
			}
			rcb = src_part_list[i].opcb_rcb;
			t = rcb->rcb_tcb_ptr;
			break;
		    }
		}
	    }

	    if (rcb->rcb_data.page)
	    {
		local_status = dm0p_unfix_page(rcb, DM0P_UNFIX, 
				&rcb->rcb_data, &local_dberr);
		if (local_status)
		    status = local_status;
	    }

	    if (dm0p_fix_page(rcb, cand->freepg_no, DM0P_READ,
			&rcb->rcb_data, &local_dberr) == E_DB_OK)
	    {
		DMPP_PAGE	*pg = rcb->rcb_data.page;

		/* empty disassociated data page? */
		if ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
				pg) & DMPP_ASSOC) ||
		    !(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
				pg) & DMPP_DATA) ||
		    ((*t->tcb_acc_plv->dmpp_tuplecount)(pg, 
				t->tcb_rel.relpgsize) != 0))
		{
		    /* definitely a problem, pg is not empty disassoc data pg */
		    if (DMZ_SRT_MACRO(13))
			TRdisplay("do_sidefile: %~t E_DM0055_NONEXT on pg = %d\n",
				sizeof(DB_TAB_NAME), table_name->db_tab_name, 
				cand->freepg_no);
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		    status = E_DB_ERROR;
		}
		else if (DMZ_SRT_MACRO(13))
		    TRdisplay("do_sidefile: %~t E_DM0055_NONEXT resolved \
for pg = %d  pgstat=0x%x\n",
			sizeof(DB_TAB_NAME), table_name->db_tab_name, 
			cand->freepg_no, 
			DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, pg));
	    }
	}

        /* remove entry from queue, deallocate block */
        if (cand->q_next)
            cand->q_next->q_prev = cand->q_prev;
        if (cand->q_prev)
            cand->q_prev->q_next = cand->q_next;
        else
            freed_pgq = cand->q_next;
        dm0m_deallocate((DM_OBJECT **)&cand);
    }

    if (fpsrc_misc_cb)
    {
	for (i = 0; i < src_tcb->tcb_rel.relnparts; i++)
	{
	    if (src_part_list[i].opcb_rcb)
	    {
		local_status = dm2t_close(src_part_list[i].opcb_rcb, 
				DM2T_NOPURGE, &local_dberr);
		if (local_status)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		   status = local_status;
		}
	    }
	}
	dm0m_deallocate((DM_OBJECT **)&fpsrc_misc_cb);
    }

    if (status != E_DB_OK)
    {
	if ((dberr->err_code != E_DM0065_USER_INTR) && 
	    (dberr->err_code != E_DM010C_TRAN_ABORTED) &&
            (dberr->err_code != E_DM016B_LOCK_INTR_FA))
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9CA2_SIDEFILE_TABLE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		sizeof(DB_TAB_NAME), table_name->db_tab_name);
	    SETDBERR(dberr, 0, E_DM9CA3_SIDEFILE);
	}
    }

    /*
    ** FIX ME if archive sees DM0LABORT and sidefile exists for transaction
    ** it should destroy the sidefile
    ** Or in do_online_modify we need to keep track, if bsf logged and there 
    ** is an error before we delete sf, need to delete here 
    ** Also archiver BSF open sidefile should not fail if sf exists...
    ** just delete and create a new sf
    **
    ** FIX ME archive startup should list sf and delete
    */

    return (status);
}


static DB_STATUS
position_to_row(
DM2U_MXCB	*mxcb,
DMP_RCB		*position_rcb,
DMP_RCB		*base_rcb,
char		*log_rec,
char		*rowbuf,
DM2R_KEY_DESC	*key_desc,
DM_TID		*tid,
DB_ERROR	*dberr)
{
    DM0L_DEL		*log_del;
    DM0L_REP		*log_rep;
    DMP_TCB		*t = base_rcb->rcb_tcb_ptr;
    DMP_TCB		*pt = position_rcb->rcb_tcb_ptr;
    i4			i;
    DB_STATUS		status;
    DM2U_MXCB		*m = mxcb;
    i4			error;
    
    do
    {
	for (i = 0; i < pt->tcb_keys; i++)
	{
	    key_desc[i].attr_number = pt->tcb_key_atts[i]->ordinal_id;
	    key_desc[i].attr_operator = DM2R_EQ;
	    if (position_rcb == base_rcb)
	    {
		key_desc[i].attr_value = 
		    &log_rec[t->tcb_key_atts[i]->offset];
	    }
	    else
	    {
		key_desc[i].attr_value =
		    &log_rec[t->tcb_data_rac.att_ptrs[pt->tcb_ikey_map[i] - 1]->offset];
	    }
	}

	/* pass position tcb's tcb_keys rather than base tcb's */
	status = dm2r_position(position_rcb, (pt->tcb_keys ? DM2R_QUAL : DM2R_ALL), 
		    key_desc, pt->tcb_keys, (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	while(1)
	{
	    status = dm2r_get(position_rcb, tid, DM2R_GETNEXT, 
				(char *)rowbuf, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** if we are positioning using the base table
	    ** and keys are not unique, compare entire row in log rec
	    ** to row retrieved.
	    */
	    if ((base_rcb == position_rcb) &&
		!(position_rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_UNIQUE))
	    {
		i4		compare;

		adt_tupcmp(base_rcb->rcb_adf_cb, (i4)t->tcb_rel.relatts, 
			t->tcb_atts_ptr, log_rec, rowbuf, (i4 *)&compare);
		/* if mismatch, get next qualifying record */
		if (compare)
		    continue;
	    }
	    break;
	}

	if (status != E_DB_OK)
	    break;

	/* We're done if locating row via primary index */
	if (base_rcb == position_rcb)
	    break;

	/* Use tid in secondary index row to locate row in base table */
	MEcopy(rowbuf + pt->tcb_rel.relwid - sizeof(DM_TID), sizeof(DM_TID),
		(PTR)tid);
	
	status = dm2r_get(base_rcb, tid, DM2R_BYTID, (char *)rowbuf, dberr);

	/* FIX ME compare rowbuf to log_rec */

    } while (FALSE);

    /*
    ** for btree, hold off reporting E_DM0055 just yet.
    ** need to check for corresponding dealloc page log record 
    ** (Issue arises when ORIGINAL table is btree, so check mx-rcb.)
    */
    if (status != E_DB_OK)
    {
	if ((dberr->err_code == E_DM0055_NONEXT) && 
	 (m->mx_rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE))
	{
	    if (DMZ_SRT_MACRO(13))
		TRdisplay("position_to_row: btree E_DM0055 %~t\n",
		    sizeof(DB_TAB_NAME), 
		    position_rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name);
	}
	else
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(dberr, 0, E_DM9CA4_ONLINE_REDO);
	}
    }
    return (status);
}


static DB_STATUS
init_readnolock(
DM2U_MXCB	*mxcb,
DM2U_OSRC_CB	*osrc,
DB_ERROR	*dberr)
{
    DMP_RCB		*r = osrc->osrc_rnl_rcb;
    DMP_RNL_ONLINE	*rnl = &osrc->osrc_rnl_online; 
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DM_PAGENO		lastused_pageno;
    DB_STATUS		status;
    i4			size;
    i4			error;

    do
    {
	status = dm1p_lastused(r, (u_i4)DM1P_LASTUSED_DATA,
		    &lastused_pageno, (DMP_PINFO*)NULL, dberr);
	if (status != E_DB_OK)
	    break;

	rnl->rnl_lastpage = lastused_pageno;
	rnl->rnl_xmap_cnt = lastused_pageno + (lastused_pageno/5);
	rnl->rnl_dm1p_max = 
	    DM1P_VPT_MAXPAGES_MACRO(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize);
	if (rnl->rnl_xmap_cnt > rnl->rnl_dm1p_max)
	    rnl->rnl_xmap_cnt = rnl->rnl_dm1p_max;
	size = sizeof(DMP_MISC) + ((rnl->rnl_xmap_cnt + 1) / 8) + 1;

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Alloc size %d xmap %d \n", 
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name, 
		size, rnl->rnl_xmap_cnt);

	status = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB, 
	    (i4) MISC_ASCII_ID, (char *) 0, 
	    (DM_OBJECT **) &rnl->rnl_xmap, dberr);
	if ( status != E_DB_OK )
	    break;

	rnl->rnl_map = (char *) &rnl->rnl_xmap[1];
	rnl->rnl_xmap->misc_data = (char *)rnl->rnl_map;

	rnl->rnl_lsn_max = 5000;
	rnl->rnl_lsn_cnt = 0;
	size = sizeof(DMP_MISC) + sizeof(DMP_RNL_LSN) * rnl->rnl_lsn_max;

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("ONLINE MODIFY: %~t Alloc size %d xlsn %d \n", 
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name, 
		size, rnl->rnl_lsn_max);

	status = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB, 
	    (i4) MISC_ASCII_ID, (char *) 0, 
	    (DM_OBJECT **) &rnl->rnl_xlsn, dberr);
	if ( status != E_DB_OK )
	    break;

	rnl->rnl_lsn = (DMP_RNL_LSN *) &rnl->rnl_xlsn[1];
	rnl->rnl_xlsn->misc_data = (char *)rnl->rnl_lsn;
	STRUCT_ASSIGN_MACRO(mxcb->mx_bsf_lsn, rnl->rnl_bsf_lsn);
	rnl->rnl_read_cnt = 0;

	if (t->tcb_rel.relspec == TCB_BTREE)
	{
	    if (status = dm2u_init_btree_read(t, rnl, 
			lastused_pageno, dberr))
	    {
		break;
	    }
	}
	else
	{
	    rnl->rnl_btree_xmap = (DMP_MISC *)NULL;
	    rnl->rnl_btree_flags = 0;
	}

	r->rcb_rnl_online = &osrc->osrc_rnl_online;

    } while (FALSE);

    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9CA7_ONLINE_INIT);
    }

    return (status);
}

static DB_STATUS
test_redo(
DM2U_OMCB	*omcb,
DM0L_HEADER	*record,
DB_TAB_ID	*log_tabid,
DM_PAGENO	log_page,
LG_LSN		*log_lsn,
bool		*redo,
DB_ERROR	*dberr)
{
    DB_STATUS		status = E_DB_OK;
    i4			i;
    DM2U_OSRC_CB	*osrc = 0;
    DM2U_OSRC_CB	*cand = omcb->omcb_osrc_next;
    i4			error;

    if (cand->osrc_rnl_rcb != cand->osrc_master_rnl_rcb)
    {
	/* find pertinent partition */
	for ( ; cand; cand = cand->osrc_next)
        {
	    if (!MEcmp(&cand->osrc_tabid, log_tabid, sizeof(DB_TAB_ID)))
	    {
		osrc = cand;
		break;
	    }
	}
    }
    else  /* nonpartitioned */
	osrc = omcb->omcb_osrc_next;
    
    if (!osrc)
    {
	*redo = FALSE;
    }
    else if (log_page > osrc->osrc_rnl_online.rnl_xmap_cnt)
    {
	/*
	** We never read this page during load phase
	** All updates in sidefile for this page must be redone
	*/
	*redo = TRUE;
    }
    else if (BTtest(log_page, osrc->osrc_rnl_online.rnl_map) == 0)
    {
	/* All updates in sidefile for this page must be redone */
	*redo = TRUE;
    }
    else
    {
	/* Find readnolock lsn for this page */
	/* Redo update only if update lsn > readnolock lsn */
	/* FIX ME after filling array it should be sorted */
	/* FIX ME this should be a binary search */
	DMP_RNL_LSN	*rnl_lsn = osrc->osrc_rnl_online.rnl_lsn;
	bool		found = FALSE;
	
	*redo = FALSE;
	for (i = 0; i < osrc->osrc_rnl_online.rnl_lsn_cnt; i++)
	{
	    if (log_page == rnl_lsn[i].rnl_page)
	    {
		*redo = LSN_GT(log_lsn, &rnl_lsn[i].rnl_lsn);
		if (DMZ_SRT_MACRO(13))
		    TRdisplay(
			"ONLINE MODIFY: %~t %s pg %d log_lsn %d rnl_lsn %d\n", 
			sizeof(DB_TAB_NAME), 
			osrc->osrc_rnl_tabname.db_tab_name,
			(*redo == 0) ? "SKIP" : 
			(record->flags & DM0L_CLR) ? "UNDO" : "REDO",
			log_page, log_lsn->lsn_low, rnl_lsn[i].rnl_lsn.lsn_low);
		found = TRUE;
		break;
	    }
	}
	if (!found)
	{
	    if (DMZ_SRT_MACRO(13))
		TRdisplay( "ONLINE MODIFY: %~t test_redo failed to find \
rnl_lsn for pg %d log_lsn %d\n",
                        sizeof(DB_TAB_NAME),
                        osrc->osrc_rnl_tabname.db_tab_name,
			log_page, log_lsn->lsn_low);
	    SETDBERR(dberr, 0, E_DM9CB0_TEST_REDO_FAIL);
	    status = E_DB_ERROR;
	}
    }

    return (status);
}


/*
** Name: do_online_dupcheck - For online processing, check for
**                      phantom duplicates
**
** Description:
**    Sorter read rows using RNL pages, which may introduce
**    phantom duplicates in $online table defined with unique
**    keys. Sidefile processing should rectify these phantom
**    duplicates. This routine is responsible for verifying
**    phantom duplicates are not present in the $online table.
**
** Inputs:
**
** Outputs:
**
**    Returns:
**      E_DB_OK
**      E_DB_ERROR
**    Exceptions:
**      None
**
** Side Effects:
**    None
**
** History:
**    25-Feb-2004 (thaju02)
**      Created.
**    02-jun-2004 (thaju02)
**	Return E_DM0045_DUPLICATE_KEY.
**    08-jul-2004 (thaju02)
**	Cleanup.
*/
static DB_STATUS
do_online_dupcheck(
DMP_DCB		*dcb,
DML_XCB		*xcb,
DM2U_MXCB	*mxcb,
DMP_RCB         *online_rcb,
DB_TAB_ID	*dupchk_tabid,
i4		db_lockmode,
DB_ERROR        *dberr)
{
    DMP_RCB		*r = online_rcb;
    DMP_TCB		*t = online_rcb->rcb_tcb_ptr;
    ADF_CB		*adf_cb = r->rcb_adf_cb;
    DMP_RCB		*dupchk_rcb = (DMP_RCB *)0;
    DMP_TCB		*dupchk_tcb = (DMP_TCB *)0;
    DM2R_KEY_DESC	*key_desc;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		cmpstatus = E_DB_OK;
    DB_STATUS		localstat = E_DB_OK;
    DB_TAB_TIMESTAMP    timestamp;
    char		*key, *record;
    i4			mem_needed;
    i4			keys_given;
    i4			compare, match;
    i4			localerr = 0;
    DM_OBJECT		*mem_ptr = 0;
    DM_TID		dupchk_tid, tid;
    u_i4		i = 0;
    DMT_CB		dmt_cb;
    i4			error;
    DB_ERROR		local_dberr;

    for (;;)
    {
	/* open $dupchk tbl & do a full scan */
	status = dm2t_open(dcb, dupchk_tabid, DM2T_N, DM2T_UDIRECT, 
		DM2T_A_READ_NOCPN, (i4)0, (i4)20, (i4)0, mxcb->mx_log_id, 
		xcb->xcb_lk_id, (i4)0, (i4)0, db_lockmode, &xcb->xcb_tran_id, 
		&timestamp, &dupchk_rcb, (DML_SCB *)0, dberr);

	if (status)
	    break;

	dupchk_tcb = dupchk_rcb->rcb_tcb_ptr;

	if (DMZ_SRT_MACRO(13))
	    TRdisplay("%@ do_online_dupcheck: %~t, dupchktbl = %~t (%d,%d)\n",
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		sizeof(DB_TAB_NAME), dupchk_tcb->tcb_rel.relid.db_tab_name,
		dupchk_tabid->db_tab_base, dupchk_tabid->db_tab_index); 

	status = dm2r_position(dupchk_rcb, DM2R_ALL, (DM2R_KEY_DESC *)0,
                (i4)0, (DM_TID *)0, dberr);

	if (status)
	    break;

	keys_given = t->tcb_keys;

	/* for secondary index, exclude tidp from key in positioning */
	if ((t->tcb_rel.relstat & TCB_INDEX) &&
	    (t->tcb_atts_ptr[t->tcb_rel.relatts].key != 0))
	    keys_given--;

	/* 
	** allocate key to store $dupchk record 
	** dupchk_tcb's relwid should be the same as the key length for
	** $online table 
	*/  
	mem_needed = sizeof(DMP_MISC) + 
			(keys_given * sizeof(DM2R_KEY_DESC)) +
			dupchk_tcb->tcb_rel.relwid +
			t->tcb_rel.relwid;

	status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB,
		(i4)MISC_ASCII_ID, (char *)0, &mem_ptr, dberr);
	if (status != OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, mem_needed);
            break;
	}

	key_desc = (DM2R_KEY_DESC *)((char *)mem_ptr + sizeof(DMP_MISC));
	key = (char *)key_desc + (keys_given * sizeof(DM2R_KEY_DESC)); 
	((DMP_MISC*)mem_ptr)->misc_data = (char*)key_desc;
	record = key + dupchk_tcb->tcb_rel.relwid;

	/* loop getting rec from $dupchk tbl */
	for (; cmpstatus == E_DB_OK && status == E_DB_OK;)   
	{
	    /* 
	    ** retrieve row from $dupchk table, position in 
	    ** $online based on key, and then scan for duplicate key 
	    ** value 
	    */
	    status = dm2r_get(dupchk_rcb, &dupchk_tid, DM2R_GETNEXT, key, 
			dberr); 

	    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
	    {
		/* we have exhausted $dupchk records */
		status = E_DB_OK;
		CLRDBERR(dberr);
		break;
	    }

	    /* build key desc, key and then position in $online */
	    /* $online tbl keys match $dupchk attributes */
	    for (i = 0; i < keys_given; i++)
	    {
		key_desc[i].attr_number = t->tcb_key_atts[i]->ordinal_id;
		key_desc[i].attr_operator = DM2R_EQ;
                key_desc[i].attr_value = &key[dupchk_tcb->tcb_data_rac.att_ptrs[i]->offset];
	    }

	    status = dm2r_position(r, DM2R_QUAL, key_desc, keys_given,
			(DM_TID *)NULL, dberr);

	    if (status != E_DB_OK)
		break;
		
	    /* 
	    ** positioned in the $online table, scan for duplicate 
	    */
	    for( match = 0;;)
	    {
		status = dm2r_get(r, &tid, DM2R_GETNEXT, record, 
			dberr); 

		if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
		{
		    /* we have exhausted qualifying records */
		    status = E_DB_OK;
		    CLRDBERR(dberr);
		    break;
		}
		else
		{
		    status = adt_keytupcmp(adf_cb, keys_given, key,
				t->tcb_key_atts, record, (i4 *)&compare);
		    if (status == E_DB_OK)
		    {
			if (compare == 0)
			{
			    if (match++)
			    {
				if (DMZ_SRT_MACRO(13))
				    TRdisplay("%@ do_online_dupcheck: %~t duplicate \
found lowtid=(%d,%d) currenttid=(%d,%d)\n",
                                        sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
                                        r->rcb_lowtid.tid_tid.tid_page, 
					r->rcb_lowtid.tid_tid.tid_line,
                                        r->rcb_currenttid.tid_tid.tid_page, 
                                        r->rcb_currenttid.tid_tid.tid_line);
				cmpstatus = status = E_DB_INFO;
				SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
				break;
			    }
			    if (DMZ_SRT_MACRO(13))
				TRdisplay("%@ do_online_dupcheck: %~t initial key found \
lowtid=(%d,%d) currenttid=(%d,%d)\n",
					sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
					r->rcb_lowtid.tid_tid.tid_page, 
					r->rcb_lowtid.tid_tid.tid_line,
					r->rcb_currenttid.tid_tid.tid_page, 
					r->rcb_currenttid.tid_tid.tid_line);
			}
		    }
		    else
		    {
			uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, (CL_ERR_DESC *)NULL, 
				ULE_LOG, (DB_SQLSTATE *) NULL, (char *)NULL, (i4)0, 
				(i4 *)NULL, &error, 0);
			uleFormat(NULL, E_DMF012_ADT_FAIL, (CL_ERR_DESC *)NULL, 
				ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				&error, 0);
			break;
		    }
		}
	    }    /* loop getting rec from $online tbl */
	}    /* loop getting rec from $dupchk tbl */
	break;
    }

    if (status && (dberr->err_code != E_DM0045_DUPLICATE_KEY))
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9CAF_ONLINE_DUPCHECK);
    }

    if (dupchk_rcb)
    {
        localstat = dm2t_close(dupchk_rcb, DM2T_NOPURGE, &local_dberr);
	if (localstat)
	{
	    /* FIX ME - ule_format */
	}
    }

    MEfill(sizeof(DMT_CB), 0, &dmt_cb);
    dmt_cb.length = sizeof(DMT_CB);
    dmt_cb.type = DMT_TABLE_CB;
    dmt_cb.ascii_id = DMT_ASCII_ID;
    STRUCT_ASSIGN_MACRO(*dupchk_tabid, dmt_cb.dmt_id);
    dmt_cb.dmt_tran_id = (PTR)xcb;
    dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
    localstat = dmt_delete_temp(&dmt_cb);
    if (localstat)
    {
	uleFormat(&dmt_cb.error, 0, NULL, ULE_LOG, NULL, NULL, 0, 
			NULL, &error, 0);
	status = localstat;
    }

    if (DMZ_SRT_MACRO(13))
	TRdisplay("%@ do_online_dupcheck: deleted %~t (%d,%d)\n",
		sizeof(DB_TEMP_TABLE)-1, DB_TEMP_TABLE,
		dupchk_tabid->db_tab_base, dupchk_tabid->db_tab_index);

    if (mem_ptr)
	dm0m_deallocate((DM_OBJECT **)&mem_ptr);

    return (status);
}

/*{
** 
** Name: find_compensated - locate compensated log rec 
**
** Description: 
**	Sidefile processing needs rec image in order to build key to
**	position to row.
**	Put CLR log record does not contain record image. 
**	Replace CLR log record does not contain the new image delta.
**	Need to scan sidefile and locate the compensated log rec to 
**	extract rec image.
** 	
** Inputs:
**	mxcb
**	clr_rec     - CLR log record
**
** Outputs:
**	row_image   - buffer to copy image into
**	ndata_len   - for replace CLR only, needed for unpack
**	nrec_size   - for replace CLR only, needed for unpack
**	diff_offset - for replace CLR only, needed for unpack
**
**      Returns:
**          E_DB_OK       
**          E_DB_ERROR   
**          err_code        
**
** 
** History:
**	4-may-2004 (thaju02)
**	    Created for online modify sidefile processing of CLR log records.
*/
static DB_STATUS
find_compensated(
DM2U_MXCB	*mxcb,
DM0L_HEADER	*clr_rec,
char		*row_image,
i4		*ndata_len,
i4		*nrec_size,
i4		*diff_offset,
DB_ERROR	*dberr)
{
    DM2U_MXCB		*m = mxcb;
    DMP_DCB		*dcb = m->mx_dcb;
    DM0J_CTX		*jnl = (DM0J_CTX *)0;
    char		*log_row;
    DB_STATUS		status = E_DB_OK;
    DM0L_HEADER		*record;
    i4			rec_len;
    i4			error;

    do
    {
	/* open sidefile */
	status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical,
                dcb->dcb_jnl->phys_length, &dcb->dcb_name,
                dcb->dcb_jnl_block_size, 0, 0, 0, DM0J_M_READ, -1, DM0J_FORWARD,
                &m->mx_table_id, (DM0J_CTX **)&jnl, dberr);

        if (status != E_DB_OK)
	    break;
	
	for (;;)
	{
	    status = dm0j_read(jnl, (PTR *)&record, &rec_len, dberr);
            if (status != E_DB_OK)
            {
                if (dberr->err_code == E_DM0055_NONEXT)
                {
                    status = E_DB_OK;
		    CLRDBERR(dberr);
                    break;
                }
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
                break;
            }
	
	    if ( ((record->flags & DM0L_CLR) == 0) &&
		  LSN_EQ(&record->lsn, &clr_rec->compensated_lsn) )
	    {
		/* found the log record that coincides with our CLR log rec */
		if (record->type == DM0LPUT)
		{
		    DM0L_PUT	*put = (DM0L_PUT *)record;

		    log_row = ((char *)put) + sizeof(DM0L_PUT);
		    MEcopy(log_row, put->put_rec_size, row_image);
		}
		else if (record->type == DM0LREP)
		{
		    DM0L_REP	*rep = (DM0L_REP *)record;

		    log_row = ((char *)rep) + sizeof(DM0L_REP) + rep->rep_odata_len;
		    MEcopy(log_row, rep->rep_ndata_len, row_image);
		    *ndata_len = rep->rep_ndata_len;
		    *nrec_size = rep->rep_nrec_size;
		    *diff_offset = rep->rep_diff_offset;
		}
		break;
	    }
	}
    } while(FALSE);

    /* close sidefile */
    if (jnl)
    {
	DB_STATUS	local_status;
	DB_ERROR	local_dberr;

	local_status = dm0j_close(&jnl, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    status = local_status;
	    *dberr = local_dberr;
	}
    }

    if (status)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9CAE_FIND_COMPENSATED);
    }
    return(status);
}

DB_STATUS
dm2u_init_btree_read(
DMP_TCB		*t,
DMP_RNL_ONLINE	*rnl,
DM_PAGENO	lastused_pageno,
DB_ERROR	*dberr)
{
    i4		maxcnt;
    i4		size;
    DB_STATUS	status = E_DB_OK;

    if (rnl->rnl_dm1p_max == 0)
	rnl->rnl_dm1p_max =
            DM1P_VPT_MAXPAGES_MACRO(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize);

    maxcnt = lastused_pageno + (lastused_pageno/5);

    if ((maxcnt = lastused_pageno + (lastused_pageno/5)) > rnl->rnl_dm1p_max)
	maxcnt = rnl->rnl_dm1p_max;
    size = sizeof(DMP_MISC) + ((maxcnt + 1) / 8) + 1;

    status = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB,
			(i4) MISC_ASCII_ID, (char *) 0,
			(DM_OBJECT **) &rnl->rnl_btree_xmap, dberr);
    if ( status == E_DB_OK )
    {
	rnl->rnl_btree_map = (char *) &rnl->rnl_btree_xmap[1];
	rnl->rnl_btree_xmap->misc_data = (char *)rnl->rnl_btree_map;
	rnl->rnl_btree_xmap_cnt = maxcnt;
	rnl->rnl_btree_flags = RNL_INIT;
    }
    return(status);
}

static DB_STATUS
find_part(
DM2U_MXCB	*m,
DB_TAB_ID	*log_tabid,
char		*log_row,
i4		*partno,
DM_TID		*ptid,
DB_ERROR	*dberr)
{
    DB_STATUS	status = E_DB_OK;
    DM2U_SPCB	*sp;
    DM_TID8	ptid8;

    /* In every case, first find row's Source, thence partition */
    for ( sp = m->mx_spcb_next; sp; sp = sp->spcb_next)
    {
	if ( log_tabid->db_tab_base  == sp->spcb_tabid.db_tab_base &&
	     log_tabid->db_tab_index == sp->spcb_tabid.db_tab_index )
	{
	    *partno = sp->spcb_partno;
	    break;
	}
    }

    /*
    ** If new partition definition, we use the log record's
    ** partition plus TID to construct a deterministic HASH
    ** to decide automatic partitions. It only matters if the
    ** partitioning scheme includes any AUTOMATIC dimensions,
    ** but we set it up always to remain transparent of
    ** ADF stuff.
    **
    ** See similar in dm2uuti.c:NextFromSource.
    */
    if ( sp && m->mx_part_def )
    {
	ADF_CB	*adf_cb = m->mx_rcb->rcb_adf_cb;
	u_i2	i;

	/* Make a TID8 for hashing */
	ptid8.tid_i4.tpf = 0;
	ptid8.tid.tid_partno = sp->spcb_partno;
	ptid8.tid_i4.tid = *(u_i4*)ptid;

	/* Tell adt_whichpart_auto about it */
	adf_cb->adf_autohash = (u_i8*)&ptid8;

	if (status = adt_whichpart_no(adf_cb, m->mx_part_def, 
			log_row, (u_i2 *)&i))
	    *dberr = adf_cb->adf_errcb.ad_dberror;
	*partno = (i4)i;
    }
    return(status);
}

static DB_STATUS
create_new_parts(
DM2U_MXCB		*m,
DB_TAB_NAME		*tabname,
DB_OWN_NAME		*owner,
DB_PART_DEF		*dmu_part_def,
DMU_PHYPART_CHAR	*partitions,
DB_TAB_ID		*master_tabid,
i4			db_lockmode,
i4			relstat,
i4			relstat2,
DB_ERROR		*dberr)
{
    DMP_DCB		*dcb = m->mx_dcb;
    DML_XCB		*xcb = m->mx_xcb;
    DMP_TCB		*t = m->mx_rcb->rcb_tcb_ptr;
    DB_STATUS		status = E_DB_OK;
    i4			partno;
    DMU_PHYPART_CHAR	*part;
    DB_TAB_ID		temp_tabid;
    DB_TAB_ID		newpart_tabid;
    DB_TAB_ID		newpart_ixid;
    char                mastertab_name[DB_TAB_MAXNAME + 1];
    char                newparttab_name[DB_TAB_MAXNAME + 30];

    MEcopy(tabname->db_tab_name, DB_TAB_MAXNAME, mastertab_name);
    mastertab_name[DB_TAB_MAXNAME] = '\0';
	
    relstat &= ~TCB_IS_PARTITIONED;
    relstat2 |= TCB2_PARTITION;

    /* glommed & massaged from qeu_modify_prep */
    for ( partno = ((m->mx_spcb_count == 1) ? 0 : m->mx_spcb_count); 
	  (partno < m->mx_tpcb_count) && partitions; partno++)
    {
	part = &partitions[partno];

	status = dm2u_get_tabid(dcb, xcb, &newpart_tabid, FALSE, 
				1, &temp_tabid, dberr);
	if (status)
	    break;

	newpart_tabid.db_tab_index = temp_tabid.db_tab_base;
	newpart_tabid.db_tab_base = master_tabid->db_tab_base;
	newpart_tabid.db_tab_index -= (partno + 1);
	STprintf(newparttab_name, "ii%x pp%d-%s",
		newpart_tabid.db_tab_base, partno, mastertab_name);

	status = dm2u_create(dcb, xcb, (DB_TAB_NAME *)newparttab_name, 
		owner, (DB_LOC_NAME *)part->loc_array.data_address,
		part->loc_array.data_in_size/sizeof(DB_LOC_NAME),
		&newpart_tabid, &newpart_ixid, 0, 0,
		relstat, relstat2,
		DB_HEAP_STORE, TCB_C_NONE, t->tcb_rel.relwid, t->tcb_rel.reldatawid,
		0, 0, db_lockmode, 0, 0,
		m->mx_page_type, m->mx_page_size,
		(DB_QRY_ID *)0, (DM_PTR *)NULL,
		(DM_DATA *)NULL, (i4)0, (i4)0,
		(DMU_FROM_PATH_ENTRY *)NULL, (DM_DATA *)0,
		0, 0, (f8*)NULL, 0, dmu_part_def, partno,
		NULL /* DMU_CB */, dberr);

	if (status != E_DB_OK)
	{
	    if (dberr->err_code != E_DM0101_SET_JOURNAL_ON)
		break;
	    status = E_DB_OK;
	    CLRDBERR(dberr);
	}

	STRUCT_ASSIGN_MACRO(newpart_tabid, part->part_tabid);
	part->flags |= DMU_PPCF_NEW_TABID;
    }
    return(status);
}

static DB_STATUS
get_online_rel(
DM2U_MXCB               *m,
DB_TAB_ID               *modtemp_tabid,
DMU_CB                  *index_cbs,
DMU_PHYPART_CHAR        *o_partitions,
i4                      db_lockmode,
DB_ERROR                *dberr)
{
    DMP_DCB             *dcb = m->mx_dcb;
    DML_XCB             *xcb = m->mx_xcb;
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RCB             *online_rcb = (DMP_RCB *)0;
    DMP_TCB             *online_tcb = (DMP_TCB *)0;
    DMP_TCB             *pt, *it;
    DM2U_TPCB           *tp;
    DB_TAB_ID           *fix_tabid;
    DMU_CB              *ix_dmu;
    DB_STATUS           status = E_DB_OK;
    DB_STATUS           local_status = E_DB_OK;
    i4                  local_error;
    DB_ERROR		local_dberr;

    for (;;)
    {
        /* Open $online table readnolock nolock to get reltup */
        status = dm2t_open(dcb, modtemp_tabid, DM2T_N,
            DM2T_UDIRECT, DM2T_A_READ_NOCPN,
            (i4)0, (i4)20, (i4)0, m->mx_log_id,
            xcb->xcb_lk_id, (i4)0, (i4)0,
            db_lockmode,
            &xcb->xcb_tran_id, &timestamp, &online_rcb, (DML_SCB *)0,
            dberr);

        if (status != E_DB_OK)
	    break;

        online_tcb = online_rcb->rcb_tcb_ptr;

        for  ( tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next)
        {
            if (m->mx_tpcb_count == 1)
                pt = online_tcb;
            else
            {
                if ((tp->tpcb_partno >= m->mx_spcb_count) && o_partitions)
                    fix_tabid = &o_partitions[tp->tpcb_partno].part_tabid;
                else
                    fix_tabid = &online_tcb->tcb_pp_array[tp->tpcb_partno].pp_tabid;

                status = dm2t_fix_tcb(dcb->dcb_id, fix_tabid,
                        &xcb->xcb_tran_id, xcb->xcb_lk_id, m->mx_log_id,
                        DM2T_NOPARTIAL | DM2T_VERIFY,
                        dcb, &pt, dberr);
                if (status != E_DB_OK)
		    break;
            }

            tp->tpcb_newtup_cnt = (i4)pt->tcb_rel.reltups;
            tp->tpcb_mct.mct_relpages = (i4)pt->tcb_rel.relpages;
            tp->tpcb_mct.mct_prim = pt->tcb_rel.relprim;
            tp->tpcb_mct.mct_main = pt->tcb_rel.relmain;
            tp->tpcb_mct.mct_i_fill = pt->tcb_rel.relifill;
            tp->tpcb_mct.mct_d_fill = pt->tcb_rel.reldfill;
            tp->tpcb_mct.mct_l_fill = pt->tcb_rel.rellfill;
            /* FIX ME online_reltup.relkeys is not correct if
            ** we did online modify of secondary index
            ** where tidp was added as a key
            ** this didn't happen when we modified index as index
            ** just if we modify index as base table
            ** ONE fix would be to create table like index (minus tidp)
            ** then create index on that temp
            ** Or before we modify that table - make it look like an index
            ** Need t->tcb_rel.relstat & TCB_INDEX set before calling
            ** modify for table
            */
            tp->tpcb_mct.mct_keys = pt->tcb_rel.relkeys;
            tp->tpcb_mct.mct_fhdr_pageno = pt->tcb_rel.relfhdr;

            if (m->mx_tpcb_count > 1)
            {
                status = dm2t_unfix_tcb(&pt, DM2T_VERIFY,
                        xcb->xcb_lk_id, m->mx_log_id, dberr);
                if (status != E_DB_OK)
		    break;
            }
            pt = (DMP_TCB *)0;
        }
        if (status != E_DB_OK)
            break;

        for (ix_dmu = index_cbs; ix_dmu; ix_dmu = (DMU_CB *)ix_dmu->q_next)
        {
            for (it = online_tcb->tcb_iq_next; it; it = it->tcb_q_next)
            {
                if (ix_dmu->dmu_idx_id.db_tab_index ==
                                it->tcb_rel.reltid.db_tab_index)
                {
                    ix_dmu->dmu_on_relfhdr = it->tcb_rel.relfhdr;
                    ix_dmu->dmu_on_relpages = it->tcb_rel.relpages;
                    ix_dmu->dmu_on_relprim = it->tcb_rel.relprim;
                    ix_dmu->dmu_on_relmain = it->tcb_rel.relmain;
                    break;
                }
            }
        }

        status = dm2t_close(online_rcb, DM2T_NOPURGE, dberr);
        if (status != E_DB_OK)
	    break;

        return(E_DB_OK);
    }

    if (online_rcb)
    {
        if (pt && (pt != online_rcb->rcb_tcb_ptr))
            local_status = dm2t_unfix_tcb(&pt, DM2T_VERIFY,
                        xcb->xcb_lk_id, m->mx_log_id, &local_dberr);

        local_status = dm2t_close(online_rcb, DM2T_NOPURGE,
                        &local_dberr);
        if (local_status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
                        NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                        &local_error, 0);
    }
    return(status);
}
/*
** FIX ME
** modify table to table_debug takes X lock
*/

/*
** FIX ME
** DB, TABLE must be journalled - but if we quiesce the table before
** we start we can set bit in relstat to make sure updates go
** to archiver (for sidefile only, not journalling)
** (Also may need to create jnl directory if it doesn't exist
*/

/* FIX ME
** modindex.sql should work but it doesn't
** i.e. modify sec-index to btree on column
*/

/*
** FIX ME
** If online modify of btree secondary index -
** we need the sidefile to contain DM0L_BTPUT, DM0L_BTDEL records
** FIX ME BSF should list log record types to go to sidefile
** FIX ME redo secondary index updates can use si_put, si_delete, si_replace
** OR don't support online modify btree secondaries initially
*/

/* FIX ME
** using readnolock, maybe should be using shared and cursor stability
** readnolock and 256k row
** concurrent_reads?
*/
