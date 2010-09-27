/*
** Copyright (c) 1989, 2010 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <ex.h>
#include    <di.h>
#include    <me.h>
#include    <mh.h>
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
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1h.h>
#include    <dm1p.h>
#include    <dm0p.h>
#include    <dm0l.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmpbuild.h>
#include    <cm.h>
#include    <dmftrace.h>
#include    <scf.h>
#include    <cut.h>
#include    <cui.h>
#include    <adurijndael.h>
#include    <dmfcrypt.h>

/**
**
**  Name: DM2UUTI.C - DM2U Utility operations.
**
**  Description:
**      This file contains routines that perform physical utility operations.
**
**          dm2u_load_table 		- Load table from sort list of from another table.
**          dm2u_update_catalogs 	- Perform the catalog updates needed for
**                                 	  modify/index.
**	    dm2u_get_tabid 		- create unique table identifier.
**	    dm2u_ckp_lock 		- get ckp compatiblity lock
**	    dm2uGetTempTableId		- Assign a db_tab_base/db_tab_index
**				  	  to a Temporary Table/Index.
**	    dm2uGetTempTableLocs  	- Resolve Temporary Table/Index
**				  	  location information.
**
**  History:
**	20-may-1989 (Derek)
**	    Split out from DM2U.C
**	12-may-89 (anton)
**	    Added collation argument to dmse_begin
**	21-jul-89 (teg)
**	    Added logic to get size of S_CONCUR catalog IFF we are doing a
**	    modify of an S_CONCUR catalog.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Check for modify on tables that
**	    have no underlying file (like a gateway table).  Don't do file
**	    renames on such tables.
**	 2-oct-89 (rogerk)
**	    Use dm0l_fswap to log filename operations in MODIFY and SYSMOD.
**	23-oct-89 (rogerk)
**	    Added database name/owner to lock key to match lock taken by
**	    checkpoint process.
**	26-dec-89 (greg)
**	    dm2u_load_table --	Use k_loc and tmp_width to align buffers
**				last arg to dm2u_index should be DB_ERROR
**				not i4
**	21-feb-90 (nancy)
**	    dm2u_load_table() -- fix bug 20175, get correct key list for
**				 modify index case.
**	17-may-90 (bryanp)
**	    After we make a significant change to the config file (increment
**	    the table ID or modify a core system catalog), make an up-to-date
**	    backup copy of the config file for use in disaster recovery.
**      17-jul-90 (mikem)
**          bug #30833 - lost force aborts
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	14-jun-1991 (Derek)
**	    Added support for new allocation and extend options along with the
**	    new inferface to the build routines and new fields in iirelation
**	    records.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-90 (rogerk)
**		Took out unused local ADF_CB's allocated in dm2u_load_table.
**	    17-mar-1991 (bryanp)
**		B35359: Don't assume 2-ary's have same set of locations as base
**		table in dm2u_destroy.
**	     4-dec-91 (rogerk)
**		Added support for Set Nologging.
**	    30-apr-1991 (bryanp)
**		Removed dm2u_reserve, as it is not currently used.
**	23-oct-1991 (rogerk)
**	    Fixed call to dm2r_delete in dm2u_update_catalogs where we were
**	    passing an extra parameter when deleting iidevices rows.
**	7-Feb-1992 (rmuth)
**	    Added new parameter to dm2f_file_size().
**	19-feb-1992 (bryanp)
**	    Temporary table enhancements.
**	10-mar-1992 (bryanp)
**	    Build into the buffer manager.
**	15-apr-1992 (bryanp)
**	    Remove the "FCB" argument from dm2f_rename().
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	04-sep-1992 (fred)
**	    Added parameter dm2u_destroy() call to match the changes necessary
**	    for peripheral datatypes.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set up mct_acc_tlv. Set mct_compressed to
**	    a value rather than a boolean.
**      29-August-1992 (rmuth)
**          Removed the 6.4->6.5 on-the-fly conversion code.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	30-October-1992 (rmuth)
**	    - Include <dm1c.h> to get dm1c function prototypes.
**	    - Set the mct_guarantee_on_disk flag when building a table.
**	06-nov-92 (jrb)
**	    Changed call to dmse_begin to reflect new interface (for ML Sorts).
**	29-oct-1992 (jnash)
**	    Reduced logging project (phase 2):
**	    - Add new params to dm0l_fcreate, dm0l_dmu, eliminate dm0l_fswap.
**	    - Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	14-dec-92 (jrb)
**	    Allowed absence of an xcb and scb at dmse_begin time in
**	    dm2u_load_table.
**	18-nov-92 (robf)
**	    Removed dm0a.h
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**	16-mar-1993 (rmuth)
**	    Include di.h
**	8-apr-93 (rickh)
**	    Added support for new relstat2 bits (FIPS CONSTRAINTS, release
**	    6.5).
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix core system catalog pages after updating them, to comply
**		    with the node failure recovery rule that no transaction
**		    can fix more than one syscat page at a time.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables and concurrent indicies.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (robf)
**	    Secure 2.0: Add dm2u_get_secid() to allocate a new security id.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	26-jul-1993 (rmuth)
**	    Encapsulate the code that calculates the number of hash
**	    buckets into dm2u_calc_hash_buckets(). The user can now
**	    specify min/maxpages on the copy command so now called
**	    from dmr_load.
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	  - Added dm2u_file_create routine.
**	  - Removed file creation from the dm2u_load_table routine.  It is
**	    now the responsibility of the caller to create the files used
**	    in the load process.
**	  - Changes to handling of non-journaled tables.  ALL system catalog
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	  - Changed journaling state of the dm0l_frename log records to be
**	    dependent upon the database journal state rather than the usertab.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	  - Changed updating of iidevices rows in modify/reorganize operations
**	    to replace existing entries rather than deleting the old ones and
**	    inserting new ones.  Also changed to leave around excess entries
**	    but blank the devloc field turning them into dummy rows.
**      16-oct-93 (jrb)
**          Added dcb_wrkspill_size to dmse_begin call for MLSorts.
**	18-oct-1993 (rogerk)
**	    Moved call to dm2u_calc_hash_buckets out of dm2u_load_table and
**	    made the determination of the number of buckets the responsibility
**	    of its caller.  This was done so that the hash bucket count could
**	    be taken out of the modify or index log record during rollforward
**	    recovery.
**	18-apr-1994 (jnash)
**	    fsync project: Just add comment noting that DI_USER_SYNC_MASK may
**	    be passed to dm2u_file_create().
**	28-june-94 (robf)
**          Updated parameters to dm2u_destroy()
**	21-nov-1994 (shero03)
**	    Use LZW compression if this is a standard, non-index,
**	    non-catalog table.
**      27-dec-94 (cohmi01)
**          For RAW I/O, changes to pass more info down to dm2f_xxx functions.
**          If tbio ptr in FCB is null, set up a local DMP_TABLE_IO with the
**          tid from mx, point FCB to it. Pass location ptr to dm2f_rename().
**          When updating pending queue, add additional info to XCCB.
**	21-feb-1995 (shero03)
**	    Bug #66931
**	    Use LZW compression for "btree with compression",
**	    not just for cbtree.
**	09-apr-95 (medji01)
**	    Bug #64699
**		Enhanced dm2u_ckp_lock() to note any current session level
**		lockmode timeout and apply this to the lock request.
**	19-may-1995 (shero03)
**          Added support for RTree
**	06-mar-1996 (stial01 for bryanp)
**	    Set mct_page_size to the value of mx_page_size.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**	    Ensure that relpgsize is updated to match the post-modify pagesize.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Pass page size to dm1c_getaccessors().
**	12-jun-1996 (thaju02)
**	    Modified dm2u_calc_hash_buckets(), calculation of tuples per page
**	    to account for tuple header size and page header overhead.
**	26-jun-1996 (shero03)
**	    Support float & integer ranges in RTree project
**      22-jul-1996 (ramra01)
**          Alter Table Project:
**              In modifying base tables (storage structures only) reformat
**              tuples and update iirelation and iiattribute.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dm1h_newhash().
**	25-oct-1996 (hanch04)
**	    Set intl_id = to attid durring an upgrade.
**      27-jan-1997 (stial01)
**          dm2u_load_table: Don't decrement sort_kcount for non-hash unique
**          secondary indexes. We don't include TIDP in key of unique 
**          secondary indexes anymore.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - removed check_setlock() routine;
**          - replaced check_setlock() call with the code to fit the 
**          new way of storing and setting of locking characteristics.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Update table priority in iirelation when asked to.
**      21-May-1997 (stial01)
**          For compatibility reasons, DO include TIDP for V1-2k unique indexes
**          Undo change of 27-jan-1997 for 2k indices.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dmse_begin() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	02-Oct-1997 (shero03)
**	    Use the log_id from the mxcb instead of the xcb because there
**	    is no xcb during recovery.
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**	    Pass mct_acc_smsv to dm1c_getaccessors().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**      12-nov-1999 (stial01)
**          We did not add tidp to key for ANY unique secondary indexes
**          No need to decrement sort_kcount anymore
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	16-feb-2000 (stial01)
**	    dm2u_load_table() Init mct_ver_number (b104008)
**      05-apr-2002 (hanal04) Bug 107319 INGSRV 1723
**          Ensure E_DM0065 is returned from dm2u_load_table in order
**          to prevent loss of data.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	28-Feb-2004 (jenjo02)
**	    Modified to deal with multiple partitions.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	31-Mar-2004 (schka24)
**	    Clean up a couple race conditions managing the targets.
**	16-Apr-2004 (schka24)
**	    Use CUT for source-target communications.
**	29-Apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	11-may-2004 (thaju02)
**	    Removed dm2u_rfp_load_table(). Incorporated it into dm2u_load_table.
**	12-may-2004 (thaju02)
**	    Changed E_DM9666 to the online modify specific E_DM9CB1.
**      25-jun-2004 (stial01)
**          Init mct_segbuf to be different than mct_crecord
**      29-Sep-2004 (fanch01)
**          Conditionally add LK_LOCAL flag if database/table is
**	    confined to one node.
**      04-oct-2004 (stial01)
**          No threads for very wide tables.
**      10-jan-2005 (thaju02)
**          Online modify of partitioned table:
**          - moved dupchk_tabid from mxcb to tpcb. Duplicate checking
**            temporary table per partition.
**          - Added MX_ROLLDB_ONLINE_MODIFY.
**          - For rolldb, added mx_resume_spcb, which is set when
**            load is halted due to rnl lsn mismatch and used to
**            resume load.
**	20-jul-2005 (abbjo03)
**	    In dm2u_update_catalogs, call RenameForDeletion() for partitioned
**	    tables as well as CREATE INDEX.
**	21-Jul-2005 (jenjo02)
**	    RenameForDeletion only -new- partitions.
**	15-Aug-2006 (jonj)
**	    Added dm2uGetTempTableId(), dm2uGetTempTableLocs() for
**	    Temporary Table Indexes.
**	24-Feb-2008 (kschendel) SIR 122739
**	    Changes for the new row-accessor scheme.
**	    Consolidate some of the MCT setup code, reduce duplication.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_?, dmse_? functions converted to 
**	    DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1?b? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      21-Jan-2009 (horda03) Bug 121519
**          Extend range of Table ids. As the table_id's approach
**          the maximum value warn the user. Once the max table id
**          is assigned, no new tables/indices can be created. No
**          current way to recover either; need to create a new DB.
**      28-Jan-2009 (horda03) Bug 121519
**          Need to calculate the IDS_LEFT before closing the CNF
**          file. As closing the CNF file will release the memory used
**          to hold the last Table_ID used.
**          Need to close the CNF file after sending the warning message,
**          as the warning message uses config structure too.
**          Need to close the CNF file after sending the warning message,
**          as the warning message uses config structure too.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	01-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**	10-may-2010 (stephenb)
**	    Cast new i8 reltups to i4
**      29-Jul-2009 (stial01) (b124163)
**          If building mxcb sem name, don't assume CS_SEM_NAME_LEN>=DB_MAXNAME
**/

GLOBALREF	DMC_CRYPT	*Dmc_crypt;

/*
** Forward function declarations
*/
static STATUS DMUHandler(
			EX_ARGS		    *ex_args);
static VOID SignalAllTargets(
			DM2U_MXCB	*m,
			i4		state_flag);
static DB_STATUS BeginSort(
			DM2U_TPCB	*tp,
			DB_ERROR	*dberr);
static DB_STATUS PutToSort(
			DM2U_MXCB	*m,
			DM2U_TPCB	*tp,
			char		*record,
			DM_TID8		*tid8p,
			DB_ERROR	*dberr);
static DB_STATUS PutToTarget(
			DM2U_MXCB	*m,
			DM2U_TPCB	*tp,
			char		*record,
			DB_STATUS	get_status,
			DB_ERROR	*dberr);
static DB_STATUS NextFromSort(
			DM2U_MXCB	*m,
			DM2U_TPCB	*tp,
			DM2U_TPCB	**Ttp,
			char		**record,
			DB_ERROR	*dberr);
static DB_STATUS NextFromAllSorts(
			DM2U_MXCB	*m,
			DM2U_TPCB	**tp_p,
			char		**record,
			DB_ERROR	*dberr);
static DB_STATUS NextFromSource(
			DM2U_MXCB	*m,
			DM2U_SPCB	*sp,
			DM2U_TPCB	**tp_p,
			char		**record,
			DM_TID8		*tid8p,
			DB_ERROR	*dberr);
static DB_STATUS NextFromAllSources(
			DM2U_MXCB	*m,
			DM2U_SPCB	**sp_p,
			DM2U_TPCB	**tp_p,
			char		**record,
			DM_TID8		*tid8p,
			DB_ERROR	*dberr);
static DB_STATUS OpenTarget(
			DM2U_TPCB	*tp,
			DB_ERROR	*dberr);
static DB_STATUS OpenAllTargets(
			DM2U_MXCB	*m,
			DB_ERROR	*dberr);
static DB_STATUS CloseTarget(
			DM2U_TPCB	*tp,
			DB_ERROR	*dberr);
static DB_STATUS CloseAllTargets(
			DM2U_MXCB	*m,
			DB_ERROR	*dberr);
static DB_STATUS RenameForDeletion(
			DM2U_MXCB	*m,
			i4		dm0l_flag,
			DB_TAB_ID	*tabid,
			DMP_LOCATION	*locArray,
			i4		k,
			DB_ERROR	*dberr);
static DB_STATUS dm2u_fix_comment(
			DM2U_MXCB	*mxcb,
			i4		attr_map[],
			i4		new_map[],
			i4		colcnt,
			bool		*comment,
			DB_ERROR	*dberr);

#define DM2UU_TAB_ID_PROXIMITY 1000000 /* How close to DB_TAB_ID_MAX to start warnings */
#define DM2UU_TAB_ID_WARN_FREQ 100     /* How often to warn of DB_TAB_ID_MAX proximity */
#define DM2UU_TAB_ID_WARN_NOW  20      /* At what point to Warn each time a ID is used */

/* Declare variables for TAB_ID proximity values, to allow for overriding
** with some config parameter.
*/
i4 dm2uu_tab_id_proximity = DM2UU_TAB_ID_PROXIMITY;
i4 dm2uu_tab_id_warn_freq = DM2UU_TAB_ID_WARN_FREQ;
i4 dm2uu_tab_id_warn_now  = DM2UU_TAB_ID_WARN_NOW;

/*{
** Name: dm2u_load_table	- Load a table.
**
** Description:
**      This routine is called from modify and index to load sorted data
**	into a structured table.  It can also be called to unsorted data
**	into a heap.
**
**	For a sufficiently small temporary table, we attempt to perform the
**	build "in-memory", which involves:
**	    using the in-memory features of the sorter, and
**	    using the in-memory features of the dm1x build buffer manager.
**
**	For non-inmemory modifies, the temporary modify files into which
**	are built the new table structure must have been created and opened
**	by the caller.  They are described by the tpcb_locations array and
**	the open FCBs should be stored therein.
**
**	Upon exit, these files will have been flushed and forced but will
**	be left open.
**
** Inputs:
**      mxcb                            The modify index control block.
**
** Outputs:
**      error                           The reason for an error return status.
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
**	24-feb-86 (derek)
**          Created new for jupiter.
**      19-nov-87 (jennifer)
**          Added multiple location per table support.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_create_file() and
**	    dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	10-feb-89 (rogerk)
**	    When loading a unique secondary index, don't include the TID
**	    pointer as one of the fields to check for uniqueness.  Including
**	    it causes all tuples to look unique even though they are not.
**	 3-may-89 (rogerk)
**	    When modifying a secondary index to HASH structure, make sure not
**	    to include non-keyed fields in the hash key.
**	12-may-89 (anton)
**	    Added collation argument to dmse_begin
**	21-jul-89 (teg)
**	    Added logic to get size of S_CONCUR catalog IFF we are doing a
**	    modify of an S_CONCUR catalog.
**	25-aug-89 (nancy)
**	    Fix bug where sort_kcount was inadvertently decremented for
**	    hash unique, causing error E_US1592 when creating a secondary
**	    with hash unique structure.
**	26-dec-89 (greg)
**	    Fix byte-align problems caused by assigning tid and hash buckets
**	    directly into tuples.  Must use I4ASSIGN_MACRO.
**	    Changed local declaration of 'k' to avoid conflict with routine's
**	    declaration of 'k'.
**	21-feb-90 (nancy)
**	    fix bug 20175 -- isam index is corrupted when modify is done.
**	    Get correct key list when table is index and operation is modify.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-90 (rogerk)
**		Took out unused local ADF_CB's.
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then don't write any log records.
**	     7-mar-1991 (bryanp)
**		Add support for Btree Index Compression.
**	    25-mar-91 (rogerk)
**		Fixed bug in Set Nologging changes - test for rollforward before
**		testing xcb status as rollforward does not provide xcb's.
**	    7-Feb-1992 (rmuth)
**		Added new parameter to dm2f_file_size.
**	10-mar-1992 (bryanp)
**	    Build into the buffer manager.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set up mct_acc_tlv. Set mct_compressed to
**	    a value rather than a boolean.
**	30-October-1992 (rmuth)
**	    Set the mct_guarantee_on_disk flag when building a table.
**	06-nov-92 (jrb)
**	    Changed call to dmse_begin to reflect new interface (for ML Sorts).
**	    a value rather than a boolean.
**	29-oct-1992 (jnash)
**	    Reduced logging project (phase 2):
**	    - Add new param to dm0l_fcreate.
**	    - Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	14-dec-92 (jrb)
**	    Allowed absence of an xcb and scb at dmse_begin time (when doing
**	    recovery these cb's are not created); in their absence we make a
**	    temporary work location array composed of all default work locations
**	    found in the dcb.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rmuth)
**	    - Use dm2u_calc_hash_buckets.
**	    - Set mct_override_alloc accordingly.
**	23-aug-1993 (rogerk)
**	    Removed file creation from the dm2u_load_table routine.  It is
**	    now the responsibility of the caller to create the files used
**	    in the load process.
**      16-oct-93 (jrb)
**          Added dcb_wrkspill_size to dmse_begin call for MLSorts.
**	18-oct-1993 (rogerk)
**	    Moved call to dm2u_calc_hash_buckets out of this routine and
**	    instead use the mx_buckets value for hash loads.  This was done
**	    so that the hash bucket count could be taken out of the modify
**	    or index log record during rollforward recovery.
**	22-nov-1994 (shero03)
**	    Use LZW compression for standard, non-index pages
**	21-feb-1995 (shero03)
**	    Bug #66931
**	    Use LZW compression for "btree with compression" 
**	19-may-1995 (shero03)
**          Added support for RTree
**	06-mar-1996 (stial01 for bryanp)
**	    Set mct_page_size to the value of mx_page_size.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Project:
**		Pass page size to dm1c_getaccessors().
**	    Store the new acccessors in the mct structure.
**	16-sep-1996 (canor01)
**	    Pass tuple buffer to dm1h_newhash().
**      27-jan-1997 (stial01)
**          Don't decrement sort_kcount for non-hash unique secondary indexes.
**          We don't include TIDP in key of unique secondary indexes anymore.
**      10-mar-1997 (stial01)
**          Init mct_crecord
**	02-Oct-1997 (shero03)
**	    Use the log_id from the mxcb instead of the xcb because there
**	    is no xcb during recovery.
**      27-Jul-1998 (islsa01)
**         Bug #85809 : The lower level function dm1r_get traps the user
**         interrupt and returns the appropriate error code and status
**         to dm2r_get function.  The following function dm2u_load_table
**         receives the status and error code from dm2r_get function -
**         checks for user interrupt error code - if it is found it calls
**         dmse_input_end function to end the input phase.
**      08-oct-98 (stial01)
**         dm2u_load_table: Deallocate build memory
**	30-Mar-2001 (thaju02)
**	   Initialize the mct->mct_ver_number. (B104382)
**
**      7-jan-98 (stial01)
**          Removed buffer parameter from dm1h_newhash.
**      05-Apr-2002 (hanal04) Bug 107319 INGSRV 1723
**         We must return an error if we received a user interrupt
**         whilst loading the dmse sort table. Failure to do so
**         leads to a loss of some or all rows in the output.
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          If mx_dmveredo is set and the table is unique we do not
**          need to do duplicate checking because we are in
**          rollforward recovery. Currently this will ONLY be set
**          for the recreation of index tables.
**      13-feb-03 (chash01) x-integrate change#461908
**         For TCB_RTREE, add code to set and check for duplicate, compile
**         complains about uninitialized variable "dup".
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin() prototype.
**	23-Dec-2003 (jenjo02)
**	    Support DM_TID8 for Global Indexes.
**	    Cleaned up code to make it appear less obtuse.
**	28-Feb-2004 (jenjo02)
**	    Modified to deal with multiple partitions and 
**	    parallel loads from/to multiple files.
**	01-Mar-2004 (jenjo02)
**	    Tweaks to DMUSource, DMUTarget so that they actually
**	    do something.
**	03-Mar-2004 (jenjo02)
**	    When doing symmetric modify, spawn only Source
**	    threads, not Target threads, and let them do 
**	    all of the work.
**	11-Mar-2004 (jenjo02)
**	    Fixed some sync problems when loading from empty
**	    sources, added sanity checks in NextFromSource
**	    to verify that the partition we wanted is the
**	    one we got. Added TPCB pointer array in
**	    mx_tpcb_ptrs.
**	6-Apr-2004 (schka24)
**	    Can't depend only on symmetric flag for second phase,
**	    since it implies threading.  Rearrange things a little more
**	    so that nonthreaded repartitioning modifies work, sorting
**	    or not sorting.
**	18-Apr-2004 (schka24)
**	    Use CUT for data transfer between source and target.
**	11-may_2004 (thaju02)
**	    For rolldb online modify restart, upon exit force & close file,
**	    close table, release fcb and close input rcb (normally performed
**	    in dm2u_modify). 
**	08-jun-2004 (thaju02)
**	    For rolldb online modify, do not close rnl table yet. Must wait 
**	    until all 'do' of sidefile processing has completed. (B112442)
**	15-Jul-2004 (jenjo02)
**	    CUT_BUFFER renamed to CUT_RCB, added MX_PRESORT state flag.
**	20-aug-2004 (thaju02)
**	    For rolldb online modify to heap, need to be able to halt the
**	    direct read from base table until pg at the appropriate lsn.
**	12-Aug-2004 (schka24)
**	    mct usage cleanups.
**	16-Sep-2004 (schka24)
**	    Set parentage for CUT threads.
**	06-Jan-2005 (jenjo02)
**	    Divide total table "with allocation" amongst the
**	    number of target partitions. This number is later
**	    further sliced across the locations for each
**	    partition.
**	04-Apr-2005 (jenjo02)
**	    To assure repeatable TID creation when there are
**	    multiple sources and targets and we otherwise 
**	    don't need to sort, sort anyway to order the 
**	    sources by their TIDs.
**	23-Nov-2005 (schka24)
**	    Round up sliced allocation if multi-location, partitioned table.
**	13-Apr-2006 (jenjo02)
**	    Propagate mx_clustered to mct_clustered.
**	30-May-2006 (jenjo02)
**	    Caller is now responsible for setting proper target TidSize
**	    in mx_tidsize.
**	12-Feb-2007 (kschendel)
**	    Add cancel checks to non-threaded variants.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	24-Sep-2008 (smeke01) b120774
**	    When using child threads do not call dm0m_deallocate on their MISC_CB 
**	    memory allocations after they have exited. This is because the child 
**	    threads ALWAYS call dm0m_destroy before exiting. 
**	12-Apr-2010 (kschendel) SIR 123485
**	    No need to set no-coupon in RCB, access mode does it now.
**	03-June-2010 (thaju02) Bug 122698
**	    If thread erred, set *dberr to mx_dberr.
**	19-Aug-2010 (miket) SIR 122403
**	    Preserve extra stuff that might be at the end of the sort
**	    record across encryption processing: bucket, partition, tid8.
**	07-Sep-2010 (miket) SIR 122403
**	    Use (new) MODIFY_BUCKET_PNO_TID8_SZ instead of one-off size
**	    definition. We care about this size when we allocate the rcb.
*/

DB_STATUS
dm2u_load_table(
DM2U_MXCB	    *mxcb,
DB_ERROR	    *dberr)
{
    DM2U_MXCB		*m = mxcb;
    DMP_RCB		*r = m->mx_rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DM2U_M_CONTEXT	*mct;
    ADF_CB		*adf_cb = r->rcb_adf_cb;
    DM_TID8		tid8;
    i4			i,k;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status;
    i4			local_err_code;
    LG_LSN		lsn;
    char	        *record;
    DM2U_SPCB		*sp;
    DM2U_TPCB		*tp;
    DM2U_TPCB		*Ptp;
    CUT_RCB		cut_rcb;
    DB_ERROR		cut_error;
    bool		cut_attached = FALSE;
    bool		om_rolldb_restart = FALSE;
    DB_ERROR		local_dberr;
    i4			switch_counter;

    CLRDBERR(dberr);

    /*	Initialize the MCT(s) for the load operations. */
    for ( tp = m->mx_tpcb_next; 
	  tp && !(m->mx_flags & MX_ROLLDB_RESTART_LOAD); 
	  tp = tp->tpcb_next )
    {
	tp->tpcb_parent = tp;
	status = dm2uInitTpcbMct(tp);
	if (status != E_DB_OK)
	{
	    /* Don't know what else to say here... */
	    SETDBERR(dberr, 0, E_DM9100_LOAD_TABLE_ERROR);
	    return (status);
	}
    }

    if (m->mx_flags & MX_ONLINE_INDEX_BUILD)
	return(E_DB_OK);

    /* Spawn Source threads, if needed */
    m->mx_spcb_threads = 0;
    m->mx_tpcb_threads = 0;
    m->mx_threads = 0;
    m->mx_state = 0;
    /*
    ** If modifying to truncate, don't need any of the code
    ** which reads and sorts records.
    */
    if ( !m->mx_truncate && m->mx_ai_count )
	m->mx_state |= MX_PRESORT;

    if (DMZ_SRT_MACRO(11))
	TRdisplay("%@ modify starting, %d sources, %d targets, %d rows\n",
		m->mx_spcb_count, m->mx_tpcb_count,
		t->tcb_rel.reltups);

    m->mx_spcb_threads = SpawnDMUThreads(m, SOURCE_THREADS);

    /* Spawn Target threads, if needed */
    if ( m->mx_spcb_threads )
    {
	m->mx_tpcb_threads = SpawnDMUThreads(m, TARGET_THREADS);

	if (m->mx_tpcb_threads > 0)
	{
	    /* If there are separate source and target threads, we need
	    ** to attach to the target cut buffers for signalling
	    */
	    for ( i = 0; i < m->mx_tpcb_count; i += Ptp->tpcb_count)
	    {
		Ptp = &m->mx_tpcb_next[i];
		cut_rcb.cut_buf_use = CUT_BUF_WRITE | CUT_BUF_PARENT;
		cut_rcb.cut_buf_handle = Ptp->tpcb_buf_handle;
		status = cut_attach_buf(&cut_rcb, &cut_error);
		if (status != E_DB_OK)
		    break;
		cut_attached = TRUE;
	    }

	    if ( m->mx_spcb_threads > 1 &&
	         !(m->mx_state & MX_PRESORT && m->mx_truncate) &&
		   t->tcb_rel.relstat & TCB_JOURNAL )
	    {
		/*
		** Asymmetric (multiple source threads feeding
		** multiple target threads).
		**
		** If not otherwise sorting and the table is journaled,
		** sort anyway to assure that the source rows are
		** presented to the Targets in (partition,TID)
		** sequence. This is necessary to ensure that if the
		** modify is rolled forward, the TIDs assigned to the
		** Target file(s) are reproducible.
		**
		** Note that dm2umod set up the extra sort compare
		** stuff for the TID8 just in case this happened.
		*/
		m->mx_state |= MX_PRESORT;
	    }
	}
    }
    
    /*
    **  If keys are specified then sort the records so
    **  that they load faster.
    */
    if ( m->mx_state & MX_PRESORT )
    {
	/*
	**  For all storage structure except HEAP,
        **  the records are sorted so that
	**  they can be loaded faster.  If this is a modify to heap on a set of
	**  keys then the heap data is also sorted first.
	*/

	/*
	**  Different handling for the following cases:
	**	Modify to anthing but hash
	**	Modify to hash
	**	Index or Index hash
	*/

	/* Still threading? */
	if ( m->mx_threads )
	{
	    /* Signal PutToSort to the Sources */
	    CSp_semaphore(1, &m->mx_cond_sem);
	    m->mx_state |= MX_PUTTOSORT;
	    CScnd_broadcast(&m->mx_cond);
	     
	    /* Wait for all input to be put to the sorters */
	    while ( m->mx_spcb_threads )
		CScnd_wait(&m->mx_cond, &m->mx_cond_sem);

	    /* All of the source threads should be gone now.
	    ** (If it was symmetric, they did everything.)
	    */
	    status = m->mx_status;
	    *dberr = m->mx_dberr;
	    CSv_semaphore(&m->mx_cond_sem);
	}
	else
	{
	    /* Nonthreaded -- load sorter and sort, serially */
	    /* Suck in all records from all sources */
	    sp = (DM2U_SPCB*)NULL;

	    switch_counter = 1000;
	    while ( status == E_DB_OK &&
		    (status = NextFromAllSources(m, &sp, &tp, &record,
				&tid8, dberr)) == E_DB_OK )
	    {
		/*
		** Returns the Source's SPCB and the Target
		** TPCB that the record maps to.
		*/
		status = PutToSort(m, tp, record, &tid8, dberr);
		if (--switch_counter <= 0)
		{
		    CScancelCheck(r->rcb_sid);
		    switch_counter = 1000;
		}
	    }

	    if ( (m->mx_flags & MX_ROLLDB_ONLINE_MODIFY) && 
		 (status == E_DB_INFO) &&
		 (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH) )
	    {
		return(status);
	    }

	    /* End of all sources (or error) */
	    if ( status && dberr->err_code == E_DM0055_NONEXT )
	    {
		status = E_DB_OK;
		CLRDBERR(dberr);
	    }
	    /* End of all sources, or error, or interrupt */

	    /* Tell each sort "end of input". This begins the merge phase */
	    for ( tp = m->mx_tpcb_next; status == E_DB_OK && tp; tp = tp->tpcb_next )
	    {
		/* If Sources empty, sorts may not have started */
		if ( tp->tpcb_srt )
		    status = dmse_input_end(tp->tpcb_srt, dberr);
	    }
	}
	if (DMZ_SRT_MACRO(11))
	    TRdisplay("%@ modify presort done\n");
	
	/* need to remember that rolldb was halted/restarted,
	** to close file at the end of load */
	if (m->mx_flags & MX_ROLLDB_RESTART_LOAD)
	{
	    om_rolldb_restart = TRUE;
	    m->mx_flags &= ~MX_ROLLDB_RESTART_LOAD;
	}

    } /* if we're sorting */


    /*
    **	Now start the load phase.  Open the output file(s).  Prepare to
    **  read the sorted record stream.  Modify to HEAP without keys
    **  will read the base table directly.
    **
    **  If this is a symmetric (threaded) sorted modify, then the Sources
    **  have completed all the work and are now gone.
    */

    if ( status == E_DB_OK
      && ! (m->mx_state & MX_PRESORT && m->mx_state & MX_SYMMETRIC) )
    {
	/*
	** If we didn't pre-sort, then the Source threads
	** are still waiting to be told to do something.
	**
	** If we pre-sorted the input(s), then the Source
	** threads are gone, but the Target threads are
	** alive. Signal them to begin their merge/create/load
	** phases.  (Must be a repartitioning modify.)
	**
	** If it's not a threaded modify, we need to move rows
	** from the input (source or sort) to the targets here.
	*/
	if ( m->mx_threads )
	{
	    if ( m->mx_state & MX_PRESORT )
		SignalAllTargets(m, MX_LOADFROMSORT);
	    else
	    {
		/* Signal it's time to build the output files.
		** Tell the sources to load from the inputs.
		** If symmetric, the sources will stuff the outputs;
		** if not symmetric, the sources will stuff to the
		** target threads which will write the outputs.
		*/
		CSp_semaphore(1, &m->mx_cond_sem);
		m->mx_state |= MX_LOADFROMSRC;
		CScnd_broadcast(&m->mx_cond);

		/* Wait for all the Sources to complete */
		while ( m->mx_spcb_threads )
		    CScnd_wait(&m->mx_cond, &m->mx_cond_sem);

		CSv_semaphore(&m->mx_cond_sem);
	    }
	}
	else
	{
	    /* Not threading, move from source or sort to target(s) */
	    sp = (DM2U_SPCB*)NULL;
	    tp = (DM2U_TPCB*)NULL;

	    if (!(m->mx_flags & MX_ROLLDB_RESTART_LOAD))
		status = OpenAllTargets(m, dberr);

	    switch_counter = 1000;
	    while ( !m->mx_truncate && status == E_DB_OK )
	    {
		if ( m->mx_state & MX_PRESORT )
		    status = NextFromAllSorts(m, &tp, &record, dberr);
		else
		    status = NextFromAllSources(m, &sp, &tp, &record, 
						    &tid8, dberr);

		/* May return E_DB_INFO if duplicate - that's ok */
		if ( (m->mx_flags & MX_ROLLDB_ONLINE_MODIFY) && 
		     (status == E_DB_INFO) &&
		     (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH) )
		{
		    return(status);
		}

		/* If there are encrypted columns, encrypt them.
		** MX_MODIFY
		**    we decide on whether to encrypt based on the table
		**    encryption setting and we use the table's data_rac
		**    as the pattern for what/how to encrypt
		** MX_INDEX
		**    since an encrypted index has just two columns (the
		**    TID and the encrypted column) we check to see if that
		**    one attribute-level encryption flag is on and we us
		**    the mxcb's data_rac for what/how to encrypt
		*/
		if (DB_SUCCESS_MACRO(status))
		{
		    local_status = E_DB_OK;
		    if (m->mx_operation == MX_MODIFY &&
			t->tcb_rel.relencflags & TCB_ENCRYPTED)
		    {
			local_status = dm1e_aes_encrypt(r, &t->tcb_data_rac,
			    record, r->rcb_erecord_ptr, dberr);
			/* (may) need to preserve bucket, partition, tid8 */
			MEcopy(record+m->mx_width, MODIFY_BUCKET_PNO_TID8_SZ,
			    r->rcb_erecord_ptr+m->mx_width);
			record = r->rcb_erecord_ptr;
		    }
		    else
		    if (m->mx_operation == MX_INDEX &&
			m->mx_atts_ptr[1].encflags & ATT_ENCRYPT)
		    {
			local_status = dm1e_aes_encrypt(r, &m->mx_data_rac,
			    record, r->rcb_erecord_ptr, dberr);
			/* (may) need to preserve bucket, partition, tid8 */
			MEcopy(record+m->mx_width, MODIFY_BUCKET_PNO_TID8_SZ,
			    r->rcb_erecord_ptr+m->mx_width);
			record = r->rcb_erecord_ptr;
		    }
		    if (local_status != E_DB_OK)
			return(local_status);
		}

		/* May return E_DB_INFO if duplicate - that's ok */
		if ( DB_SUCCESS_MACRO(status) )
		    status = PutToTarget(m, tp, record, status, dberr);

		if (--switch_counter <= 0)
		{
		    CScancelCheck(r->rcb_sid);
		    switch_counter = 1000;
		}
	    }

	    /* If success, finish off the targets */
	    if ( status && dberr->err_code == E_DM0055_NONEXT )
	    {
		status = E_DB_OK;
		CLRDBERR(dberr);
	    }
	    if ( status == E_DB_OK )
		status = CloseAllTargets(m, dberr);
	}
    }

    /* End of load, good status or bad */

    if ( m->mx_state & MX_THREADED )
    {
	/* Signal any remaining Targets that we're done.
	** Should be safe to look at unprotected mx_status, sources
	** are gone now one way or another.
	*/
	if ( m->mx_status )
	    SignalAllTargets(m, MX_ABORT);
	else
	    SignalAllTargets(m, MX_END);
	if (DMZ_SRT_MACRO(11))
	    TRdisplay("%@ modify end signalled\n");

	/* Wait for all threads to end */
	CSp_semaphore(1, &m->mx_cond_sem);
	while ( m->mx_threads )
	    CScnd_wait(&m->mx_cond, &m->mx_cond_sem);

	/* The collective status */
	status = m->mx_status;
	*dberr = m->mx_dberr;

	CSv_semaphore(&m->mx_cond_sem);
        CScnd_free(&m->mx_cond);
        CSr_semaphore(&m->mx_cond_sem);

    }

    if (cut_attached)
    {
	/* Clean up CUT, including resetting attention/error status for CS */
	(void) cut_signal_status(NULL, E_DB_OK, &cut_error);
	(void) cut_thread_term(TRUE, &cut_error);
    }

    /* Cleanup all targets, end all sorts */
    for ( tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next )
    {
	mct = &tp->tpcb_mct;

	/* Accumulate tuple counts from all targets */
	m->mx_newtup_cnt += tp->tpcb_newtup_cnt;

	if ( tp->tpcb_srt )
	{
	    local_status = dmse_end(tp->tpcb_srt, &local_dberr);
	    /* Don't log errors that are artifacts of error propagation */
	    if ( local_status != E_DB_OK
	      && local_dberr.err_code != E_CU0204_ASYNC_STATUS
	      && local_dberr.err_code != E_DM9714_PSORT_CFAIL)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);

		/* Override any otherwise good status */
		if ( status == E_DB_OK )
		{
		    status = local_status;
		    *dberr = local_dberr;
		}
	    }
	    tp->tpcb_srt = NULL;
	}
	/* 
	** Don't deallocate if we had child threads doing the work, as they 
	** will have finished by now (since we waited for them) and will have 
	** dm0m_destroy-ed their short term memory allocation. (b120774).
	*/
	if (!(m->mx_state & MX_THREADED) )
	{
	    if ( tp->tpcb_srt_wrk )
	    {
		dm0m_deallocate((DM_OBJECT **)&tp->tpcb_srt_wrk);
	    }
	    if ( mct->mct_buffer )
	    {
		dm0m_deallocate( (DM_OBJECT **)&mct->mct_buffer);
	    }
	}
    }

    /* Cleanup all Sources */
    for ( sp = m->mx_spcb_next; sp; sp = sp->spcb_next )
    {
	/* Unless error, this should already have been done */
	if ( sp->spcb_rcb && sp->spcb_rcb != m->mx_rcb )
	{
	    /* On error or NONEXT, close the RCB */
	    (VOID)dm2t_close(sp->spcb_rcb,
			     DM2T_NOPURGE,
			     &local_dberr);
	    sp->spcb_rcb = (DMP_RCB*)NULL;
	    sp->spcb_positioned = FALSE;
	}
    }

    if ( (m->mx_flags & MX_ROLLDB_ONLINE_MODIFY) && 
	 ( om_rolldb_restart || 
	 (m->mx_flags & MX_ROLLDB_RESTART_LOAD) ) )
    {
	/* 
	** Normally this is handled in dm2u_modify, but
	** for rolldb of online modify if load is stopped 
	** and resumed we need to close file here.
	*/
	for (tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next)
	{
	    local_status = dm2f_force_file(tp->tpcb_locations, 
			tp->tpcb_loc_count, &t->tcb_rel.relid,
			&m->mx_dcb->dcb_name, &local_dberr);
                                                                         
	    local_status = dm2f_close_file(tp->tpcb_locations, 
			tp->tpcb_loc_count,
                        (DM_OBJECT *)m->mx_dcb, 
			&local_dberr);
	}

	local_status = dm2t_close(m->mx_rcb, DM2T_PURGE, 
			&local_dberr);

	for (tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next)
	{
	    local_status = dm2f_release_fcb(m->mx_lk_id, 
			m->mx_log_id, tp->tpcb_locations, 
			tp->tpcb_loc_count, DM2F_ALLOC, 
			&local_dberr);
	}
    }

    for ( tp = m->mx_tpcb_next; tp; tp = tp->tpcb_next)
    {
	if (tp->tpcb_dupchk_rcb)
	{
	    local_status = dm2t_close(tp->tpcb_dupchk_rcb, DM2T_PURGE, 
				&local_dberr);
	    if (local_status)
	    {
		/* FIX ME - report something */
	    }
	    tp->tpcb_dupchk_rcb = (DMP_RCB *)NULL;
	}
    }

    if ( status && dberr->err_code > E_DM_INTERNAL )
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &local_err_code, 0);
	SETDBERR(dberr, 0, E_DM9100_LOAD_TABLE_ERROR);
    }
    if (DMZ_SRT_MACRO(11))
	TRdisplay("%@ dm2u_load_table done, %d rows\n",m->mx_newtup_cnt);

    return(status);
}

/*{
** Name: SignalAllTargets - Signal a state change to all 
**			    target threads.
**
** Description:
**
**	Signals a condition change to all Target threads
**	and any Source threads that may be waiting on
**	the Target.
**
**	Signalling is done synchronously (except for an abort signal).
**	A fake record is stuffed through all target threads' CUT buffers
**	that contains the desired state change.  This ensures that the
**	targets have completely processed all input up to the point of
**	the state change.
**
**	Abort is done a little differently, by signalling a CUT status
**	change.  Since this involves a CSattn call on each thread
**	attached to each target CUT buffer, it's important to not hold
**	any event condition semaphores upon entry.
**
** Inputs:
**	m		MXCB with list of Target threads.
**	state_flag	The state change mask, for example
**			MX_ABORT.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    The threads should respond appropriately 
**	    to the state change.
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	12-Mar-2004 (jenjo02)
**	    When a Target thread manages multiple TPCBs,
**	    modify the state in all managed TPCBs.
**	31-Mar-2004 (schka24)
**	    Wait for all targets to be not-busy until we sent the thread
**	    the event.
**	18-Apr-2004 (schka24)
**	    Redo to send signals via CUT.
**	15-Jul-2004 (jenjo02)
**	    CUT_BUFFER renamed to CUT_RCB, added cut_thr_handle.
**	08-Sep-2004 (jenjo02)
**	    cut_write_buf prototype change for DIRECT mode.
*/
static VOID
SignalAllTargets(
DM2U_MXCB	*m,
i4		state_flag)
{
    bool	waited;
    DM2U_TPCB	*Ptp;			/* Thread controller TPCB */
    CUT_RCB	cut_rcb;
    DB_ERROR	cut_error;
    DB_STATUS	status;
    i4		i, j, num_cells;

    if ( m->mx_tpcb_threads )
    {
	/* Set up signaller record in case we use it */
	m->mx_signal_rec->rhdr_state = state_flag;

	for ( i = 0; i < m->mx_tpcb_count; i += Ptp->tpcb_count)
	{
	    Ptp = &m->mx_tpcb_next[i];

	    if ( (Ptp->tpcb_state & (MX_ABORT | MX_THREADED | MX_TERMINATED))
				== MX_THREADED )
	    {
		/* Abort is signalled with an async CUT signal.
		** Other states are signalled by stuffing a signal record
		** through with a state change.  Not a wildly elegant way
		** of doing things, but assures proper synchronous operation
		** give our N-writer 1-reader (per target thread) model.
		*/
		cut_rcb.cut_buf_handle = Ptp->tpcb_buf_handle;
		cut_rcb.cut_thr_handle = NULL;
		if (state_flag & MX_ABORT)
		{
		    (void) cut_signal_status(&cut_rcb, E_DB_ERROR, &cut_error);
		}
		else
		{
		    num_cells = 1;
		    /* Tell CUT to awaken targets now (flush) */
		    status = cut_write_buf(&num_cells, &cut_rcb,
				(PTR*)&m->mx_signal_rec,
				CUT_RW_WAIT | CUT_RW_FLUSH,
				&cut_error);
		    if (status != E_DB_OK)
		    {
			/* Oops.  Go back and send aborts */
			state_flag |= MX_ABORT;
			i = 0;
			continue;
		    }
		}
	    }
	} /* for */
    }
    return;
}

/*{
** Name: NextFromAllSorts - Returns next record from all 
**			    sorts.
**
** Description:
**
**	When not threading, serially steps through all
**	Target sorts, returning the next sorted record
**	until all sorts are emptied.
**
** Inputs:
**	m		MXCB with list of Targets.
**	tp_p		A pointer-to-pointer to the current
**			TPCB of the sort being drained.
**			On the first call to this function,
**			this must be pointer to null.
**
** Outputs:
**	*tp_p		TPCB corresponding to the 
**			returned sort record.
**	record		The sort record.
**	err_code	E_DM0055_NO_NEXT when all sorts
**			have been drained.
**	Returns:
**	    DB_STATUS	E_DB_INFO if the returned record
**			is a duplicate.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
*/
static DB_STATUS
NextFromAllSorts(
DM2U_MXCB	*m,
DM2U_TPCB	**tp_p,
char		**record,
DB_ERROR	*dberr)
{
    DM2U_TPCB		*tp;
    DB_STATUS		status = E_DB_OK;

    /* First time, caller must init *tp_p to NULL */
    if ( (tp = *tp_p) == (DM2U_TPCB*)NULL )
	tp = m->mx_tpcb_next;

    while ( (*tp_p = tp) && status == E_DB_OK )
    {
	status = NextFromSort(m, tp, tp_p, record, dberr);

	/* May return E_DB_INFO if duplicate */
	if ( DB_SUCCESS_MACRO(status) )
	    return(status);

	/* End of sort records? */
	if ( dberr->err_code == E_DM0055_NONEXT )
	{
	    status = E_DB_OK;
	    CLRDBERR(dberr);

	    /* Move on to next sort */
	    tp = tp->tpcb_next;
	}
    }

    /* Successful end of all sorts? */
    if ( status == E_DB_OK )
    {
	status = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
    }

    return(status);
}

/*{
** Name: NextFromSort	 - Returns next record from a 
**			   single sort.
**
** Description:
**
**	Calls dmse_get_record() to get the next record
**	back from a sort.
**
**	When the last record has been returned, calls
**	dmse_end() to terminate the sort, then frees
**	any sort memory that may have been allocated.
**
** Inputs:
**	m		MXCB
**	tp		The TPCB associated with the sort
**
** Outputs:
**	record		The sort record.
**	err_code	E_DM0055_NO_NEXT when the sort
**			has been drained.
**	Returns:
**	    DB_STATUS	E_DB_INFO if the returned record
**			is a duplicate.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	17-May-2004 (jenjo02)
**	    Parent TPCB thread now does sorting for all sibling
**	    Targets. Partition number added as most significant
**	    sort key. Record's Target TPCB is returned to caller
**	    in "*Ttp".
*/
static DB_STATUS
NextFromSort(
DM2U_MXCB	*m,
DM2U_TPCB	*tp,
DM2U_TPCB	**Ttp,
char		**record,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    DM2U_TPCB	*Ptp = tp->tpcb_parent;
    u_i2	pno;
    DB_ERROR	local_dberr;

    /* May have never opened the sort if Source(s) empty */
    if ( Ptp->tpcb_srt )
    {
	status = dmse_get_record(Ptp->tpcb_srt, record, dberr);

	/* May return E_DB_INFO if duplicate */

	if ( DB_FAILURE_MACRO(status) )
	{
	    /* End this sort */
	    (VOID)dmse_end(Ptp->tpcb_srt, &local_dberr);

	    /* Free any sort memory we may have allocated */
	    Ptp->tpcb_srt = (DMS_SRT_CB*)NULL;
	    if ( Ptp->tpcb_srt_wrk )
		dm0m_deallocate((DM_OBJECT **)&Ptp->tpcb_srt_wrk);
	}
	else if ( m->mx_operation == MX_INDEX )
	    /* Until partitioned indexes are supported... */
	    *Ttp = Ptp;
	else
	{
	    /* Extract the partition number from the sort record, */
	    if ( m->mx_structure == TCB_HASH )
		I2ASSIGN_MACRO((*record)[m->mx_width+4], pno);
	    else
		I2ASSIGN_MACRO((*record)[m->mx_width], pno);

	    /* Return pointer to record's Target TPCB */
	    *Ttp = m->mx_tpcb_ptrs[pno];
	}
    }
    else
    {
	status = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
    }

    return(status);
}

/*{
** Name: NextFromAllSources - Returns next record from all 
**			      Sources.
**
** Description:
**
**	When not threading, serially steps through all
**	Source SPCBs, returning the next record
**	until all Sources are exhausted.
**
** Inputs:
**	m		MXCB with list of Sources.
**	sp_p		A pointer-to-pointer to the current
**			SPCB of the Source being read.
**			On the first call to this function,
**			this must be pointer to null.
**
** Outputs:
**	*sp_p		SPCB corresponding to the 
**			returned record.
**	*tp_p		TPCB corresponding to the record's
**			Target.
**	record		The record.
**	tid8p		...and its TID.
**	err_code	E_DM0055_NO_NEXT when all Sources
**			have been read.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	10-jan-2005 (thaju02)
**	    Rolldb of online modify of partitioned table. Resume
**	    starting with mx_resume_spcb. 
*/
static DB_STATUS
NextFromAllSources(
DM2U_MXCB	*m,
DM2U_SPCB	**sp_p,
DM2U_TPCB	**tp_p,
char		**record,
DM_TID8		*tid8p,
DB_ERROR	*dberr)
{
    DM2U_SPCB		*sp;
    DB_STATUS		status = E_DB_OK;

    /* First time, caller must init *sp_p to NULL */
    if ( (sp = *sp_p) == (DM2U_SPCB*)NULL )
	sp = m->mx_spcb_next;

    /* for rolldb of online modify, resume load */
    if ((m->mx_flags & MX_ROLLDB_ONLINE_MODIFY) && (m->mx_resume_spcb))
    {
	sp = m->mx_resume_spcb;
	m->mx_resume_spcb = (DM2U_SPCB *)0;
    }

    while ( (*sp_p = sp) && status == E_DB_OK )
    {
	status = NextFromSource(m, sp, tp_p, record, tid8p, dberr);

	if ( status == E_DB_OK )
	     return(status);

	if ( (m->mx_flags & MX_ROLLDB_ONLINE_MODIFY) && 
	     (status == E_DB_INFO) && 
	     (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH) )
	{
	    return(status);
	}

	/* End of this source's records? */
	if ( dberr->err_code == E_DM0055_NONEXT )
	{
	    status = E_DB_OK;
	    CLRDBERR(dberr);

	    /* Move on to next source */
	    sp = sp->spcb_next;
	}
    }

    /* Successfully read all sources? */
    if ( status == E_DB_OK )
    {
	/* Then return NONEXT */
	status = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
    }

    return(status);
}

/*{
** Name: NextFromSource - Returns next record from a 
**			  single Source.
**
** Description:
**
**	Calls dm2r_get() to retrieve the next record
**	from a Source and map it to a Target.  The record is read
**	into the area pointed to by spcb_record (after the header).
**
**	On the first call, the Source Table is opened
**	and positioned for a full scan.
**	
**	When the last record has been read, the Source's
**	Table is closed.
**
** Inputs:
**	m		MXCB
**	sp		The SPCB of the Source being read.
**
** Outputs:
**	*tp_p		TPCB corresponding to the record's
**			Target.
**	*record		Pointer to pointer to the record.
**	tid8p		...and its TID.
**	err_code	E_DM0055_NO_NEXT when all the Source's
**			records have been read.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	11-Mar-2004 (jenjo02)
**	    Added sanity checks to make sure partition
**	    mapping returns the right result.
**	6-Apr-2004 (schka24)
**	    Symmetric isn't on if not threading, adjust sanity checks.
**	11-may-2004 (thaju02)
**	    For rolldb online modify restart, unfix data/other pages,
**	    pages have since changed.
**	08-jun-2004 (thaju02)
**	    For rolldb online modify restart, if rnl table is btree,
**	    do not refresh leaf page. (B112428)
**	28-jun-2004 (thaju02)
**	    For rolldb online modify restart, if rnl table is btree,
**	    do not unfix data/leaf pages here. We will handle it in
**	    dm1b_online_get(). (B112566)
**      11-aug-2004 (thaju02)
**          Rollforward of online modify of btree, position may
**          have to halt if pg lsn is not at rnl lsn. (B112790)
**	10-jan-2005 (thaju02)
**	    Rolldb of online modify of partitioned table. If rnl
**	    lsn mismatch, save current spcb in mx_resume_spcb for
**	    load resume.
**	04-Apr-2005 (jenjo02)
**	    Use determinstic TID8 method for AUTOMATIC
**	    partitions.
**	11-Nov-2005 (jenjo02)
**	    Check for external interrupts via XCB when available.
**	19-Dec-2007 (jonj)
**	    Replace PCsleep() with CSsuspend(). PCsleep() affects
**	    the entire process with Ingres threading.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
*/
static DB_STATUS
NextFromSource(
DM2U_MXCB	*m,
DM2U_SPCB	*sp,
DM2U_TPCB	**tp_p,
char		**record,
DM_TID8		*tid8p,
DB_ERROR	*dberr)
{
    ADF_CB		*adf_cb;
    DMP_TCB		*t;
    DB_STATUS		status = E_DB_OK;
    i4			local_error;
    DM_TID		*tidp = (DM_TID*)&tid8p->tid_i4.tid;
    u_i2		partno;
    i4			error;
    DB_ERROR		local_dberr;

    /* Actual record data is after the dm2u row header */
    *record = (char *) sp->spcb_record + sizeof(DM2U_RHEADER);

    /* Open the source's table, if not already */
    if ( sp->spcb_rcb == (DMP_RCB*)NULL )
    {
	DB_TAB_TIMESTAMP	timestamp;

	/* Open the source's table */
	status = dm2t_open(m->mx_dcb,
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
	    sp->spcb_rcb->rcb_xcb_ptr = m->mx_xcb;
	    sp->spcb_partno = sp->spcb_rcb->rcb_tcb_ptr->tcb_partno;
	    sp->spcb_positioned = FALSE;
	    /* Catch query interrupts */
	    sp->spcb_rcb->rcb_uiptr = m->mx_rcb->rcb_uiptr;
	    /* Leave RCB sid zero, caller will do cancel checks if needed */
	}
    }

    /* Position for a full scan of the source */
    if ( status == E_DB_OK && sp->spcb_positioned == FALSE )
    {
	status = dm2r_position(sp->spcb_input_rcb ? 
	    sp->spcb_input_rcb : sp->spcb_rcb, DM2R_ALL,
	    (DM2R_KEY_DESC *)0, (i4)0,
	    (DM_TID *)0, dberr);
	if (status == E_DB_OK)
	    sp->spcb_positioned = TRUE;
    }

    if ((m->mx_dcb->dcb_status & DCB_S_ROLLFORWARD) &&
	(m->mx_flags & MX_ROLLDB_RESTART_LOAD) &&
	(sp->spcb_input_rcb) &&
	!(sp->spcb_input_rcb->rcb_tcb_ptr->tcb_rel.relspec == TCB_BTREE))
    {
	DMP_RCB		*r = sp->spcb_input_rcb;

	/* 
	** rolldb of online modify
	** unfix page ptrs, refix to refresh page snapshot
	*/
	if (r->rcb_data.page)
	    status = dm0p_unfix_page(r, DM0P_UNFIX,
                        &r->rcb_data, dberr);

        if (r->rcb_other.page)
            status = dm0p_unfix_page(r, DM0P_UNFIX,
                        &r->rcb_other, dberr);
    }
	
    if ( status == E_DB_OK )
	status = dm2r_get(sp->spcb_input_rcb ?
			    sp->spcb_input_rcb : sp->spcb_rcb, 
			    tidp, DM2R_GETNEXT,
			    *record, dberr);

    if ((status == E_DB_OK) && DMZ_SRT_MACRO(15))
    {
	TRdisplay("dm2u_load_table: dm2r_get returned tid = \
(%d,%d)\n",
                 tidp->tid_tid.tid_page, tidp->tid_tid.tid_line);
	TRdisplay("    Sleeping after rnl get to allow concurrent updates\n");
	CSsuspend(CS_TIMEOUT_MASK, 10, 0);
    }

    /* Check to see if user interrupt or abort occurred */
    if ( status == E_DB_OK && *(sp->spcb_rcb->rcb_uiptr) )
    {
	/* If XCB, check via SCB */
	if ( sp->spcb_rcb->rcb_xcb_ptr )
	{
	    dmxCheckForInterrupt(sp->spcb_rcb->rcb_xcb_ptr, &error);
	    if ( error )
		SETDBERR(dberr, 0, error);
        }
	else if ( *(sp->spcb_rcb->rcb_uiptr) & RCB_USER_INTR )
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	else if ( *(sp->spcb_rcb->rcb_uiptr) & RCB_FORCE_ABORT )
	    SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
	if ( dberr->err_code )
	    status = E_DB_ERROR;
    }

    if ( (m->mx_flags & MX_ROLLDB_ONLINE_MODIFY) && 
	 (status) &&
	 (dberr->err_code == E_DM9CB1_RNL_LSN_MISMATCH) )
    {
	/* for rolldb, do not close table, keep position */
	m->mx_flags |= MX_ROLLDB_RESTART_LOAD;
	m->mx_resume_spcb = sp;
	return(E_DB_INFO);
    }

    /* Return with record, tid, pointer to target's TPCB */
    if ( status == E_DB_OK )
    {
	/* Return the Source's partno in the TID */
	tid8p->tid_i4.tpf = 0;
	tid8p->tid.tid_partno = sp->spcb_partno;

	/*
	** If there's a single Target, then 
	** partition number isn't an issue.
	**
	** If multiple Targets (partitions),
	** and there's a new partitioning
	** scheme, then apply the predicate
	** to this record to determine the
	** appropriate TPCB.
	**
	** If there's no new scheme, then
	** the target TPCB is symmetric
	** with the source.  (Although the MX_SYMMETRIC flag
	** may not be on -- that flag implies threading.)
	*/
	if ( m->mx_tpcb_count == 1 )
	    *tp_p = m->mx_tpcb_ptrs[0];
	else
	{
	    if ( m->mx_part_def == NULL )
		partno = sp->spcb_partno;
	    else
	    {
		adf_cb = sp->spcb_rcb->rcb_adf_cb;

		/*
		** Use TID8 as seed for AUTOMATIC
		** partition hash, if any.
		*/
		adf_cb->adf_autohash = (u_i8*)tid8p;

		if ( status = adt_whichpart_no(adf_cb,
				m->mx_part_def,
				*record,
				&partno) )
		{
		    *dberr = adf_cb->adf_errcb.ad_dberror;
		}
	    }

	    if ( status == E_DB_OK )
	    {
		if ( (*tp_p = m->mx_tpcb_ptrs[partno]) != NULL )
		{
		    if ( (*tp_p)->tpcb_partno != partno )
		    {
			/* Oops.  Somehow we did bad. */
			t = sp->spcb_rcb->rcb_tcb_ptr;

			uleFormat(NULL, E_DM0025_PARTITION_MISMATCH, NULL, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &local_error, 4,
			    sizeof(DB_DB_NAME), t->tcb_rel.relowner.db_own_name,
			    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			    0, partno,
			    0, (*tp_p)->tpcb_partno);
			SETDBERR(dberr, 0, E_DM9100_LOAD_TABLE_ERROR);
			status = E_DB_ERROR;
		    }
		}
		else
		{
		    /* Yikes, we really screwed up */
		    t = sp->spcb_rcb->rcb_tcb_ptr;

                    uleFormat(NULL, E_DM0024_NO_PARTITION_INFO, NULL, ULE_LOG, NULL,
                            (char *)NULL, 0L, (i4 *)NULL, &local_error, 2,
			    sizeof(DB_DB_NAME), t->tcb_rel.relowner.db_own_name,
			    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name);
		    SETDBERR(dberr, 0, E_DM9100_LOAD_TABLE_ERROR);
		    status = E_DB_ERROR;
		}
	    }
	}
    }

    if ( status && sp->spcb_rcb && sp->spcb_rcb != m->mx_rcb )
    {
	/* On error or NONEXT, close the RCB */
	(VOID)dm2t_close(sp->spcb_rcb,
			 DM2T_NOPURGE,
			 &local_dberr);
	sp->spcb_rcb = (DMP_RCB*)NULL;
	sp->spcb_positioned = FALSE;
    }

    return(status);
}

/*{
** Name: OpenAllTargets - Prepare Targets for build.
**
** Description:
**
**	Prepares all Targets for the ensuing build.
**
** Inputs:
**	m		MXCB with list of Targets.
**
** Outputs:
**	err_code	Zero or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
*/
static DB_STATUS
OpenAllTargets(
DM2U_MXCB	*m,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    DM2U_TPCB	*tp = m->mx_tpcb_next;

    /* Open all output files */
    while ( tp && (status = OpenTarget(tp, dberr)) == E_DB_OK )
	tp = tp->tpcb_next;

    return(status);
}

/*{
** Name: OpenTarget - Prepare a Target for build.
**
** Description:
**
**	Calls the appropriate dm1?begin() function
**	to prepare a Target's files for build.
**
** Inputs:
**	tp		The TPCB of the Target.
**
** Outputs:
**	err_code	Zero or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
*/
static DB_STATUS
OpenTarget(
DM2U_TPCB	*tp,
DB_ERROR	*dberr)
{
    DB_STATUS	status;

    /* Open an output file */
    switch ( tp->tpcb_mxcb->mx_structure )
    {
	case TCB_HEAP:
	    status = dm1sbbegin(&tp->tpcb_mct, dberr);
	    break;
	case TCB_ISAM:
	    status = dm1ibbegin(&tp->tpcb_mct, dberr);
	    break;
	case TCB_HASH:
	    status = dm1hbbegin(&tp->tpcb_mct, dberr);
	    break;
	case TCB_BTREE:
	    status = dm1bbbegin(&tp->tpcb_mct, dberr);
	    break;
	case TCB_RTREE:
	    status = dm1mbbegin(&tp->tpcb_mct, dberr);
	    break;
    }
    tp->tpcb_state |= MX_OPENED;
    return(status);
}

/*{
** Name: CloseAllTargets - Closes all Targets for build.
**
** Description:
**
**	Closes all Target files after build.
**
** Inputs:
**	m		MXCB with list of Targets.
**
** Outputs:
**	err_code	Zero or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
*/
static DB_STATUS
CloseAllTargets(
DM2U_MXCB	*m,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    DM2U_TPCB	*tp = m->mx_tpcb_next;

    /* Finish up all output files */
    while ( tp && (status = CloseTarget(tp, dberr)) == E_DB_OK )
	tp = tp->tpcb_next;

    return(status);
}

/*{
** Name: CloseTarget - Closes a Target after build
**
** Description:
**
**	Calls the appropriate dm1?end() function
**	to complete the build process for a Target.
**
**	Deallocates any build memory that may
**	have been allocated.
**
** Inputs:
**	tp		The TPCB of the Target.
**
** Outputs:
**	err_code	Zero or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
*/
static DB_STATUS
CloseTarget(
DM2U_TPCB	*tp,
DB_ERROR	*dberr)
{
    DB_STATUS	status;

    /* Finish up an output file */
    switch ( tp->tpcb_mxcb->mx_structure )
    {
	case TCB_HEAP:
	    status = dm1sbend(&tp->tpcb_mct, dberr);
	    break;
	case TCB_ISAM:
	    status = dm1ibend(&tp->tpcb_mct, dberr);
	    break;
	case TCB_HASH:
	    status = dm1hbend(&tp->tpcb_mct, dberr);
	    break;
	case TCB_BTREE:
	    status = dm1bbend(&tp->tpcb_mct, dberr);
	    break;
	case TCB_RTREE:
	    status = dm1mbend(&tp->tpcb_mct, dberr);
	    break;
    }

    /* Deallocate build memory */
    if ( tp->tpcb_mct.mct_buffer )
	dm0m_deallocate( (DM_OBJECT **)&tp->tpcb_mct.mct_buffer);

    tp->tpcb_state |= MX_CLOSED;
    return(status);
}

/*{
** Name: PutToSort - Puts a Source record to a Target's sorter
**
** Description:
**
**	Opens a sort on the designated Target's TPCB
**	(if not already),
**	constructs the sort record from the Source record,
**	and calls dmse_put_record().
**
** Inputs:
**	m		MXCB
**	tp		The Target's TPCB.
**	record		The Source record to be sorted
**	tid8p		... and its TID.
**
** Outputs:
**	err_code	Zero, or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	17-May-2004 (jenjo02)
**	    Parent TPCB thread now does sorting for all sibling
**	    Targets. Partition number postfixed as most significant
**	    sort key.
**	16-dec-04 (inkdo01)
**	    Add various db_collID's and attr_collID's.
**	04-Apr-2005 (jenjo02)
**	    Stuff Source's TID8 at the end.
**      21-may-2009 (huazh01)
**          dm1h_hash(), dm1h_newhash(), dm1h_keyhash() now includes
**          DMP_RCB and err_code in the parameter list. (b122075)
*/
static DB_STATUS
PutToSort(
DM2U_MXCB	*m,
DM2U_TPCB	*tp,
char		*record,
DM_TID8		*tid8p,
DB_ERROR	*dberr)
{
    DM2U_M_CONTEXT	*mct = &tp->tpcb_mct;
    DM2U_TPCB		*Ptp = tp->tpcb_parent;
    DB_STATUS		status = E_DB_OK;
    i4			i, bucket;
    u_i2		pno = tp->tpcb_partno;
    char		*tup_ptr;
    i4			error;

    /* If sort not yet begun on this Parent, fire it up */
    if ( Ptp->tpcb_srt == (DMS_SRT_CB*)NULL )
	status = BeginSort(Ptp, dberr);

    /* Creating an index? */
    if ( status == E_DB_OK && m->mx_operation == MX_INDEX)
    {
	/*
	** Until we have partitioned indexes, the partition
	** number is not postfixed to the sort record.
	*/

	/* This is where we build the index record */
	tup_ptr = Ptp->tpcb_irecord;

	if (m->mx_structure == TCB_RTREE)
	{
	    DB_DATA_VALUE obj;
	    DB_DATA_VALUE range;
	    DB_DATA_VALUE nbr;
	    DB_DATA_VALUE hilbert;
	    ADF_CB	  *adf_cb;

	    adf_cb = m->mx_rcb->rcb_adf_cb;
	    /*
	    ** Fill in the constant DATA_VALUE fields
	    **
	    ** Note that the order is:
	    ** NBR | HILBERT | TIDP 
	    */

	    /* obj.db_data is variable */
	    obj.db_length = m->mx_b_key_atts[0]->length;
	    obj.db_datatype = abs(m->mx_b_key_atts[0]->type);
	    obj.db_prec = 0;
	    obj.db_collID = -1;
	    range.db_data = (char *)m->mx_range;
	    if (m->mx_acc_klv->range_type == 'F')
	      range.db_length = m->mx_dimension * sizeof(f8) * 2;
	    else
	      range.db_length = m->mx_dimension * sizeof(i4) * 2;
	    range.db_datatype = m->mx_acc_klv->box_dt_id;
	    range.db_prec = 0;
	    range.db_collID = -1;
	    nbr.db_data = tp->tpcb_irecord;
	    nbr.db_length = m->mx_hilbertsize * 2;
	    nbr.db_datatype = m->mx_acc_klv->nbr_dt_id;
	    nbr.db_prec = 0;
	    nbr.db_collID = -1;
	    hilbert.db_data = tp->tpcb_irecord + nbr.db_length;
	    hilbert.db_length = m->mx_hilbertsize;
	    hilbert.db_datatype = DB_BYTE_TYPE;
	    hilbert.db_prec = 0;
	    hilbert.db_collID = -1;

	    /*  Copy the TIDP */
	    tup_ptr += nbr.db_length + m->mx_hilbertsize;
	    if ( m->mx_new_relstat2 & TCB2_GLOBAL_INDEX )
		MEcopy((char*)tid8p, sizeof(DM_TID8), tup_ptr);
	    else
		MEcopy((char*)(&tid8p->tid_i4.tid),
			    sizeof(DM_TID), tup_ptr);
		

	    /* Create the NBR */
	    obj.db_data = &record[m->mx_b_key_atts[0]->offset];
	    status = (*m->mx_acc_klv->dmpp_nbr)(adf_cb,
					     &obj, &range, &nbr);
	    if ( status == E_DB_OK )
	    {
		/* Create the HILBERT */
		status = (*m->mx_acc_klv->dmpp_hilbert)(adf_cb,
						 &nbr, &hilbert);
	    }
	}
	else
	{
	    /* Construct the (non_RTREE) index tuple */
	    for (i = 0; i < m->mx_ab_count; i++)
	    {
		MEcopy(&record[m->mx_b_key_atts[i]->offset],
		      m->mx_b_key_atts[i]->length, tup_ptr);
		tup_ptr += m->mx_b_key_atts[i]->length;
	    }

	    /*
	    ** Put the tid at the end of the tuple, 
	    ** TID8's for Global Indexes.
	    */
	    if ( m->mx_new_relstat2 & TCB2_GLOBAL_INDEX )
		MEcopy((char*)tid8p, sizeof(DM_TID8), tup_ptr);
	    else
		MEcopy((char*)(&tid8p->tid_i4.tid),
			    sizeof(DM_TID), tup_ptr);

	    /*
	    ** If HASH index, put hash bucket at the end of the index
	    ** row so it can be used in the sorter.
	    */
	    if (m->mx_structure == TCB_HASH)
	    {
		if (dm1h_newhash(m->mx_rcb, m->mx_i_key_atts, m->mx_ai_count,
		    tp->tpcb_irecord, mct->mct_buckets, &bucket, dberr) 
                    != E_DB_OK)
                    return (E_DB_ERROR); 

		I4ASSIGN_MACRO(bucket, Ptp->tpcb_irecord[m->mx_width]);
	    }
	}
	record = tp->tpcb_irecord;
    }
    else if ( status == E_DB_OK )
    {
	/* MODIFY */

	if ( m->mx_structure == TCB_HASH )
	{
	    i4	       kcount;
	    DB_ATTS    **katts;
	    /*
	    ** Get correct # keys to hash on.  If the table is a secondary
	    ** index, then mx_ab_count may include non-keyed fields of
	    ** the secondary index.  Use mx_ai_count instead.
	    */
	    kcount = m->mx_ab_count;
	    katts = m->mx_b_key_atts;
	    if (m->mx_rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_INDEX)
	    {
		kcount = m->mx_ai_count;
		katts = m->mx_i_key_atts;
	    }

	    /*
	    ** Put bucket at end of row so we can sort on it.
	    */
	    if (dm1h_newhash(m->mx_rcb, katts, kcount, record,
				  mct->mct_buckets, &bucket, dberr) 
                != E_DB_OK)
                return (E_DB_ERROR); 

	    I4ASSIGN_MACRO(bucket, record[m->mx_width]);

	    /* Stuff target partition number after hash bucket */
	    I2ASSIGN_MACRO(pno, record[m->mx_width+4]);
	    /* Stuff source's TID8 after target partno */
	    I8ASSIGN_MACRO(*tid8p, record[m->mx_width+4+2]);
	}
	else
	{
	    /* Stuff target partition number at the end of the row */
	    I2ASSIGN_MACRO(pno, record[m->mx_width]);
	    /* Stuff source's TID8 after target partno */
	    I8ASSIGN_MACRO(*tid8p, record[m->mx_width+2]);
	}
    }

    /* Put the record to the Target's sorter */
    if ( status == E_DB_OK )
	status = dmse_put_record(Ptp->tpcb_srt, record, dberr);

    return(status);
}

/*{
** Name: BeginSort - Begin a sort on a Target's TPCB
**
** Description:
**
**	Calls dmse_begin() to set up a sort on a TPCB.
**
** Inputs:
**	tp		The Target's TPCB.
**
** Outputs:
**	err_code	Zero or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	17-May-2004 (jenjo02)
**	    Parent TPCB thread now does sorting for all sibling
**	    Targets. Partition number added as most significant
**	    sort key.
**	04-Apr-2005 (jenjo02)
**	    Source's TID8 is least significant sort field now.
**	12-Apr-2005 (thaju02)
**	    Removed sort_kcount++ for inclusion of tid8, caused
**	    problem with isam.
**	18-Apr-2005 (jenjo02)
**	    Exclude TID8 from duplicate consideration by the
**	    sorter via sort_cmp_count.
*/
static DB_STATUS
BeginSort(
DM2U_TPCB	*tp,
DB_ERROR	*dberr)
{
    DM2U_MXCB		*m = tp->tpcb_mxcb;
    DM2U_M_CONTEXT	*mct = &tp->tpcb_mct;
    DMP_RCB		*r = m->mx_rcb;
    DMP_TCB		*t = r->rcb_tcb_ptr;
    DML_XCB		*xcb = m->mx_xcb;
    DB_STATUS		status = E_DB_OK;
    i4	    		sort_flag;
    i4	    		sort_width;
    i4	    		sort_kcount;
    i4	    		sort_cmp_count;
    i4			*wrk_loc_mask;
    i4			wrk_loc_size;
    i4			est_recs;
    i4			error;

    /*
    ** Note that "r" and "t" reference the (Master) Table,
    ** not the output of the create or modify.
    */

    /*
    **	Indexes and unique keys don't have duplicate records eliminated
    **	because none exist in the case of an index, and unique keyed
    **	primary table get errors for non-unique keys.
    */

    sort_flag = DMSE_ELIMINATE_DUPS;
    if (m->mx_duplicates ||
	t->tcb_rel.relstat & TCB_INDEX ||
	mct->mct_unique)
    {
	sort_flag = 0;
    }

    sort_width = m->mx_width;
    sort_kcount = m->mx_ai_count;
    sort_cmp_count = m->mx_c_count;

    /*
    ** Include target partition number and Source's TID8
    ** in sort (but not if MX_INDEX)
    */
    if ( m->mx_operation == MX_MODIFY )
    {
	sort_width += sizeof(u_i2);
	sort_kcount++;

	/* Include Source's TID8 */
	sort_width += sizeof(DM_TID8);
	/* Exclude TID8 from "ELIMINATE_DUPS" checking */
	sort_cmp_count--;
    }

    switch (m->mx_structure)
    {
       case TCB_HASH:
	   sort_width += sizeof(i4);
	   sort_kcount++;
	   if (m->mx_unique && !(m->mx_dmveredo))
	       sort_flag |= DMSE_CHECK_KEYS;
	   break;

       case TCB_ISAM:
       case TCB_BTREE:

	   if ( ! mct->mct_index || 
		       (m->mx_unique && !(m->mx_dmveredo)))
	       sort_flag |= DMSE_CHECK_KEYS;
	   break;
    }

    /*  Prepare the sorter to accept the records. */

    /*  First we determine the set of work locations which will be
    **  used to sort; if this is a dbms session then the location
    **	masks in the scb will determine which work locations the user
    **	will use.  If we are doing recovery, however, we must choose
    **	what set of work locations will be used; the current decision
    **	is to use all default work locations listed in the dcb.
    */

    if ( xcb && xcb->xcb_scb_ptr )
    {
	wrk_loc_mask = xcb->xcb_scb_ptr->scb_loc_masks->locm_w_use;
	wrk_loc_size = xcb->xcb_scb_ptr->scb_loc_masks->locm_bits;
    }
    else
    {
	/* allocate and fill a temporary work location bit mask; the
	** sorter will expect this object to live until the sort is
	** terminated
	*/
	DMP_EXT		*ext;
	i4		size, i;

	ext = m->mx_dcb->dcb_ext;
	size = sizeof(i4) *
		(ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4);

	status = dm0m_allocate((i4)sizeof(DMP_MISC) + size,
	    DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	    (char *)NULL, (DM_OBJECT**)&tp->tpcb_srt_wrk, dberr);

	if ( status == E_DB_OK )
	{
	    wrk_loc_size = size * BITSPERBYTE;
	    wrk_loc_mask =
		    (i4 *)((char *)tp->tpcb_srt_wrk + sizeof(DMP_MISC));
	    ((DMP_MISC*)(tp->tpcb_srt_wrk))->misc_data = (char*)wrk_loc_mask;

	    /* initialize location usage bit masks in scb */
	    for (i=0; i < ext->ext_count; i++)
	    {
		if (ext->ext_entry[i].flags & DCB_WORK)
		    BTset(i, (char *)wrk_loc_mask);
	    }
	}
    }

    /*
    ** Estimate number of records per sort.
    **
    ** If sorts are being done by Parent threads, slice the
    ** number of tuples by the number of Parents.
    **
    ** Otherwise the sorts are symmetric, with one per
    ** Source/Target pair.
    **
    ** Note that this assumes a fairly even distribution
    ** of records across all Targets, which may be a totally
    ** false assumption.
    */
    if ( m->mx_tpcb_threads )
	est_recs = (i4)t->tcb_rel.reltups / m->mx_tpcb_threads;
    else
	est_recs = (i4)t->tcb_rel.reltups / m->mx_tpcb_count;

    if ( status == E_DB_OK )
	status = dmse_begin(sort_flag, 
	    &tp->tpcb_tabid,
	    m->mx_cmp_list,
	    m->mx_c_count, sort_kcount, sort_cmp_count,
	    m->mx_dcb->dcb_ext->ext_entry, wrk_loc_mask, wrk_loc_size,
	    &m->mx_dcb->dcb_wrkspill_idx, sort_width,
	    est_recs,
	    r->rcb_uiptr, tp->tpcb_lk_id, tp->tpcb_log_id,
	    (DMS_SRT_CB **)&tp->tpcb_srt, r->rcb_collation, 
	    r->rcb_ucollation, dberr);

    return(status);
}

/*{
** Name: PutToTarget - Add a record to a Target's build
**		       table.
**
** Description:
**
**	Calls the dm1?bput() appropriate to the
**	Targets structure to add a record to the
**	build table.
**
** Inputs:
**	m		MXCB
**	tp		The TPCB of the Target.
**	record		The record to add to the table.
**	get_status	E_DB_OK, or E_DB_INFO if this
**			record is a duplicate.
**
** Outputs:
**	err_code	Zero or why it failed.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    A record is added to the build table.
**	    TPCB's tpcb_newtup_cnt is incremented.
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	19-Mar-2004 (jenjo02)
**	    Add missing parens around setting of "dup".
**	30-apr-2004 (thaju02)
**	    Changed params to dm2u_put_dupchktbl. 
**	02-jun-2004 (thaju02)
**	    Moved setting of dup.
*/
static DB_STATUS
PutToTarget(
DM2U_MXCB	*m,
DM2U_TPCB	*tp,
char		*record,
DB_STATUS	get_status,
DB_ERROR	*dberr)
{
    DM2U_M_CONTEXT	*mct = &tp->tpcb_mct;
    DB_STATUS		status;
    i4			dup;
    i4			bucket;
    bool		duplicate = FALSE;
    i4			error;
    
    if ((dup = (get_status == E_DB_INFO)) &&
	 (m->mx_flags & MX_ONLINE_MODIFY) && 
	 mct->mct_unique &&
	 m->mx_structure != TCB_HEAP )
    {
	DB_STATUS	local_status = E_DB_OK;
	DB_ERROR	local_dberr;

	if  ((m->mx_dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
	    local_status = dm2u_put_dupchktbl(tp, record, &local_dberr);

	get_status = E_DB_OK;
        duplicate = TRUE;
    }
	
    if ( (get_status == E_DB_INFO) && mct->mct_unique &&
	 m->mx_structure != TCB_HEAP )
    {
	SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
	status = E_DB_ERROR;
    }
    else switch ( m->mx_structure )
    {
	case TCB_HEAP:
	    /*	Add to the heap table. */
	    status = dm1sbput(mct, record, m->mx_width, (i4)0, dberr);
	    break;

	case TCB_ISAM:
	    /*	Add to the ISAM table. */
	    status = dm1ibput(mct, record, m->mx_width,
				dup, dberr);
	    break;


	case TCB_HASH:
	    /*	Add to the HASH table. */
	    I4ASSIGN_MACRO(record[m->mx_width], bucket);
	    status = dm1hbput(mct, bucket, record,
				m->mx_width, (i4)0, dberr);
	    break;

	case TCB_BTREE:
	    /*	Add to the BTREE table. */
	    status = dm1bbput(mct, record, m->mx_width, 
				dup, dberr);
	    break;

	case TCB_RTREE:
	    /*  Add to the RTREE table. */
	    status = dm1mbput(mct, record,
			     m->mx_width, dup, dberr);
	    break;
    }

    if ( (status == E_DB_OK)  && !duplicate)
	tp->tpcb_newtup_cnt++;

    return(status);
}

/*{
** Name: SpawnDMUThreads - Create Source or Target threads
**
** Description:
**
**	Spawns DMU threads when it seems appropriate.
**
**	Threading is not done if "modify to truncate",
**	as that's just a bunch of file renames.
**
**	If there are multiple Sources or multiple Targets,
**	then threading is presumed to be of some benefit.
**
**	We spawn some threads for the modify Sources (partition)
**	and some for the modify Targets (partition) and 
**	wait for them to successfully initialize before
**	returning.
**
** 	However, if this is a symmetric modify in which
**	each Source will only resolve to one Target,
**	we'll spawn a thread for each Source, but none
**	for the Targets. The Source threads will do the
**	"puts" in-line.
**
**	If the spawn process fails for any reason, then
**	all previously begat threads are terminated and
**	the modify will be processed serially.
**
** Inputs:
**	m		MXCB with list of Sources and Targets
**	thread_type	Either "SOURCE_THREADS" or "TARGET_THREADS"
**
** Outputs:
**	Returns:
**	    i4		The number of new threads spawned;
**			if zero, then threading is inappropriate
**			or has been abandoned.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	02-Mar-2004 (jenjo02)
**	    Don't spawn Target threads if symmetric modify.
**	    Sources will be paired 1-1 with TPCBs.
**	08-Mar-2004 (jenjo02)
**	    Added support for DegreeOfParallelism (DOP) to
**	    throttle the number of Source/Target threads.
**	    Each Source/Target thread may now be responsible
**	    for more than one SPCB/TPCB.
**	12-Mar-2004 (jenjo02)
**	    Fixed mis-located setting of MX_SYMMETRIC which
**	    can't occur until it's time to spawn Targets.
**	5-Apr-2004 (schka24)
**	    Don't try to thread when it's rollforward running.
**	12-Apr-2004 (jenjo02)
**	    Extract DOP from SCB, setable by trace point DM36.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Start kids with DMF facility.
**	27-Apr-2010 (kschendel)
**	    Shorten thread names so they aren't truncated.
**	03-Sep-2010 (miket) SIR 122403 SD 146662
**	    No threads for encryption because
**	    (1) threaded load paths not all encryption enabled
**	    (2) possible collision on encryption work buffer
*/
i4
SpawnDMUThreads(
DM2U_MXCB	*m,
i4		thread_type)
{
    DMP_TCB	*t = m->mx_rcb->rcb_tcb_ptr;
    DM2U_SPCB	*sp;
    DM2U_TPCB	*tp;
    DB_STATUS	status;
    SCF_CB	scf_cb;
    SCF_FTC	ftc;
    STATUS	scf_status = OK;
    i4		new_threads;
    char	sem_name[CS_SEM_NAME_LEN + DB_TAB_MAXNAME + 16];
    char	thread_name[60];
    i4		avail_threads;
    i4		threads, remainder;

    /*
    ** No threads for journal redo...
    ** No threads for very wide tables...
    ** otherwise, extract DegreeOfParallelization from SCB.
    */
    if ((m->mx_dcb->dcb_status & DCB_S_ROLLFORWARD) || 
	(m->mx_flags & MX_ONLINE_MODIFY) ||
	(m->mx_width > 32768) ||
	(t->tcb_rel.relencflags & TCB_ENCRYPTED))
	avail_threads = 0;
    else
	avail_threads = m->mx_xcb->xcb_scb_ptr->scb_dop;

    new_threads = 0;
    m->mx_new_threads = 0;
    m->mx_status = E_DB_OK;
    CLRDBERR(&m->mx_dberr);

    /* No threading if modify to truncate */

    if ( !m->mx_truncate &&  avail_threads > 1 &&
	    (m->mx_spcb_count > 1 || m->mx_tpcb_count > 1) )
    {
	if ( (m->mx_state & MX_THREADED) == 0 )
	{
	    /* Init the MXCB stuff we'll need to sync the threads */
	    CScnd_init(&m->mx_cond);

	    /*
	    ** Initialize the MXCB mutexes
	    ** CS will truncate the name to CS_SEM_NAME_LEN
	    ** Since table name may be > CS_SEM_NAME_LEN, 
	    ** use reltid,reltidx to make the semaphore name unique,
	    ** plus as much of the table name as we can fit
	    */
	    CSw_semaphore(&m->mx_cond_sem, CS_SEM_SINGLE,
		      STprintf(sem_name, "MXCB %x %x %*s", 
			t->tcb_rel.reltid.db_tab_base, 
			t->tcb_rel.reltid.db_tab_index,
			t->tcb_relid_len, t->tcb_rel.relid.db_tab_name));
	    CSget_sid(&m->mx_sid);
	    m->mx_state |= MX_THREADED;

	    /* Compute the number of Sources and Targets per thread */
	    if ( !m->mx_part_def && m->mx_spcb_count == m->mx_tpcb_count )
	    {
		/*
		** Symmetric (non-repartition) modify. 
		** There's no need for additional Target 
		** threads. We're assured
		** that each Source will be paired with 
		** exactly one Target.
		*/
		/* Don't use more threads than we have work for */
		if (m->mx_spcb_count < avail_threads)
		    avail_threads = m->mx_spcb_count;

		m->mx_tpcb_threads = 0;
		m->mx_spcb_threads = avail_threads;
	    }
	    else
	    {
		/* Don't use more threads than we have work for */
		if (m->mx_spcb_count + m->mx_tpcb_count < avail_threads)
		    avail_threads = m->mx_spcb_count + m->mx_tpcb_count;

		m->mx_tpcb_threads = avail_threads / 2;
		m->mx_spcb_threads = avail_threads / 2;

		/*
		** Give the extra thread, if any,
		** to that with the most need.
		*/
		if ( m->mx_spcb_count >= m->mx_tpcb_count )
		    m->mx_spcb_threads += avail_threads % 2;
		else
		    m->mx_tpcb_threads += avail_threads % 2;

		/*
		** If more source threads than Sources, give
		** the extra to the Targets, and vv.
		*/
		if ( m->mx_spcb_threads > m->mx_spcb_count )
		{
		    m->mx_tpcb_threads += 
			m->mx_spcb_threads - m->mx_spcb_count;
		    m->mx_spcb_threads = m->mx_spcb_count;
		}
		if ( m->mx_tpcb_threads > m->mx_tpcb_count )
		{
		    m->mx_spcb_threads += 
			m->mx_tpcb_threads - m->mx_tpcb_count;
		    m->mx_tpcb_threads = m->mx_tpcb_count;
		}
	    }

	    /* Now figure the number of Sources/Targets per thread,
	    ** truncated to the next smaller integer.  Any leftover
	    ** sources/targets will be divvied up amongst the threads
	    ** as we create them.
	    */
	    if ( (m->mx_spcb_per_thread = m->mx_spcb_threads) != 0 )
		m->mx_spcb_per_thread = 
		    m->mx_spcb_count / m->mx_spcb_threads;
	    if ( (m->mx_tpcb_per_thread = m->mx_tpcb_threads) != 0 )
		m->mx_tpcb_per_thread = 
		    m->mx_tpcb_count / m->mx_tpcb_threads;

	    /*
	    ** The modulus SPCB/TPCB(s), if any, will be parceled 
	    ** out amongst the threads.
	    */
		    
	}


	/* Spawning Sources? */
	if ( thread_type == SOURCE_THREADS )
	{
	    threads = m->mx_spcb_threads;
	    remainder = m->mx_spcb_count -
		(m->mx_spcb_threads * m->mx_spcb_per_thread);
	    sp = m->mx_spcb_next;
	}
	/* Spawning Targets? */
	else if ( thread_type == TARGET_THREADS )
	{
	    threads = m->mx_tpcb_threads;
	    if ( threads != 0 )
	    {
		remainder = m->mx_tpcb_count -
		    (threads * m->mx_tpcb_per_thread);
		tp = m->mx_tpcb_next;
	    }
	    else
	    {
		/*
		** Symmetric (non-repartition) modify.  This means
		** that the source threads are going to do all the work,
		** because each source is paired with exactly one
		** target.  Note that this flag also implies threading.
		*/

		/* Let all know this is the case */
		m->mx_state |= MX_SYMMETRIC;
	    }
	}

	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_ptr_union.scf_ftc = &ftc;

	ftc.ftc_thread_exit = NULL;
	ftc.ftc_data_length = 0;
	ftc.ftc_priority = SCF_CURPRIORITY;
	/* So the kids can do ShortTerm dm0m_allocates */
	ftc.ftc_facilities = 1 << DB_DMF_ID;
	ftc.ftc_thread_name = thread_name;

	while ( threads-- && scf_status == OK )
	{
	    if ( thread_type == SOURCE_THREADS )
	    {
		sp->spcb_count = m->mx_spcb_per_thread;
		if ( remainder )
		    sp->spcb_count++;

		STprintf(thread_name,
			  "DMUs (%d,%d):%d",
			  sp->spcb_tabid.db_tab_base,
			  sp->spcb_tabid.db_tab_index 
			      & ~DB_TAB_PARTITION_MASK,
			  sp->spcb_count);
		ftc.ftc_thread_entry = DMUSource;
		ftc.ftc_data = (PTR)sp;
	    }
	    else
	    {
		tp->tpcb_count = m->mx_tpcb_per_thread;
		if ( remainder )
		    tp->tpcb_count++;

		STprintf(thread_name,
			  "DMUt (%d,%d):%d",
			  tp->tpcb_tabid.db_tab_base,
			  tp->tpcb_tabid.db_tab_index 
			      & ~DB_TAB_PARTITION_MASK,
			  tp->tpcb_count);
		ftc.ftc_thread_entry = DMUTarget;
		ftc.ftc_data = (PTR)tp;
	    }
		
	    if ( remainder )
		remainder--;

	    /* Create and initiate the thread */
	    if ( (scf_status = scf_call(SCS_ATFACTOT, &scf_cb)) == OK )
	    {
		new_threads++;

		if ( thread_type == SOURCE_THREADS )
		    sp += sp->spcb_count;
		else
		    tp += tp->tpcb_count;
	    }
	}

	CSp_semaphore(1, &m->mx_cond_sem);

	/* Add to the total of all threads */
	m->mx_threads += new_threads;
	m->mx_new_threads += new_threads;

	/* If any threads failed to start, signal abort */
	if ( scf_status )
	    m->mx_state |= MX_ABORT;

	/* Wait for all threads to initialize */ 
	while ( m->mx_new_threads )
	    CScnd_wait(&m->mx_cond, &m->mx_cond_sem);
	status = m->mx_status;
	CSv_semaphore(&m->mx_cond_sem);

	/* If any failed to init, abandon threading */
	if ( status )
	{
	    /* Signal abort to any remaining threads */
	    SignalAllTargets(m, MX_ABORT);
	    CSp_semaphore(1, &m->mx_cond_sem);

	    m->mx_state |= MX_ABORT;
	    CScnd_broadcast(&m->mx_cond);

	    /* Wait for them to all go away */
	    while ( m->mx_threads )
		CScnd_wait(&m->mx_cond, &m->mx_cond_sem);
	    new_threads = 0;
	    /* Destroy the condition */
	    m->mx_state &= ~MX_THREADED;
	    CSv_semaphore(&m->mx_cond_sem);
	    CSr_semaphore(&m->mx_cond_sem);
	    CScnd_free(&m->mx_cond);
	}
    }

    /* Return number of threads started, if any */
    return(new_threads);
}

/*{
** Name: DMUSource - Factotum thread to read from
**		     a Source and put records to
**		     Target(s).
**
** Description:
**
**      Started by SpawnDMUThreads for a Source.
**
**	Connects to the transaction logging/locking
**	contexts, then waits for something to do.
**
**	That "something" is signalled by a change
**	in the MXCB's state:
**
**		MX_PUTTOSORT	Get records from the Source,
**				put them to a Target's
**				sorter.
**		MX_LOADFROMSRC	Get records from the Source,
**				put them to a Target, no
**				sort involved.
**		MX_REORG	Modify to reorganize.
**		MX_ABORT	Stop whatever you're doing
**				and terminate.
**
**	When putting to a target, we put to a target thread
**	if MX_SYMMETRIC is off, or put directly to the target if
**	MX_SYMMETRIC is on.  A target thread is signalled with
**	exactly the same state flag (PUTTOSORT or LOADFROMSRC)
**	that is driving the source.
**
**	When we terminate, we decrement mx_spcb_threads and 
**	mx_threads and disconnect the log/locking contexts.
**
** Inputs:
**	ftx		SCF_FTX from SpawnDMUThreads with
**			ftx_data pointing to Source's SPCB.
**
** Outputs:
**	none
**
**	Returns:
**	    DB_STATUS	Always E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    If an error is encountered by this thread,
**	    the status and error code are placed in
**	    the MXCB and MX_ABORT is signalled to
**	    all other threads.
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	02-Mar-2004 (jenjo02)
**	    If symmetric modify (1-1 Sources/Targets),
**	    there won't be Target threads, so do
**	    their work here.
**	    Added support for MX_REORG 
**	    (modify to reorganize).
**	04-Mar-2004 (jenjo02)
**	    Must, for now, copy source record and TID to 
**	    Target's private space when Target is threaded
**	    to avoid overlaying it with next Source record.
**	08-Mar-2004 (jenjo02)
**	    Each Source thread may be responsible for
**	    more than just one SPCB, depending on the mix
**	    of Source/Targets and the number of 
**	    available threads.
**	12-Mar-2004 (jenjo02)
**	    If symmetric Source is empty, make sure the
**	    Target output files get created anyway.
**	31-Mar-2004 (schka24)
**	    Separate target busy from thread wakeup.
**	    (jenjo02) Add missing EXdelete().
**	18-Apr-2004 (schka24)
**	    Send data through CUT for buffering.
**	08-Sep-2004 (jenjo02)
**	    cut_write_buf prototype change for DIRECT mode.
**	11-Nov-2008 (jonj)
**	    Removed extra ";" introduced by last change for
**	    SIR 120874.
*/
DB_STATUS
DMUSource(
SCF_FTX		*ftx)
{
    DM2U_SPCB	*Psp = (DM2U_SPCB*)ftx->ftx_data, *sp;
    DM2U_TPCB	*tp, *Ptp, *Ttp;
    DM2U_MXCB	*m = Psp->spcb_mxcb;
    STATUS	lg_status, lk_status;
    i4		log_id = 0;
    i4		lk_id  = 0;
    DB_STATUS	status = E_DB_OK, local_status;
    i4		err_code, local_err;
    i4		logging = m->mx_rcb->rcb_logging;
    i4		i;
    DM_TID8	tid8;
    CL_ERR_DESC sys_err;
    EX_CONTEXT	context;
    CUT_RCB	cut_rcb;
    DB_ERROR	cut_error;
    char	*rec_ptr;
    i4		num_cells;
    bool	cut_attached = FALSE;
    i4		error;
    DB_ERROR	local_dberr;
    DB_ERROR	dberr;

    CLRDBERR(&dberr);

    if ( EXdeclare(DMUHandler, &context) )
    {
	status = E_DB_ERROR;
	SETDBERR(&dberr, 0, E_DM004A_INTERNAL_ERROR);
	CSp_semaphore(1, &m->mx_cond_sem);

	/* Count ourselves down for the count */
	m->mx_spcb_threads--;
	m->mx_threads--;
    }
    else
    {
	/* First, connect to logging/locking */
	if ( lg_status = logging )
	    lg_status = LGconnect(m->mx_log_id,
				   &log_id,
				   &sys_err);

	if ( (lk_status = lg_status) == OK )
	    lk_status = LKconnect((LK_LLID)m->mx_lk_id, 
				   (LK_LLID*)&lk_id,
				    &sys_err);

	if ( lg_status || lk_status )
	{
	    status = E_DB_ERROR;
	    if ( lg_status == LG_EXCEED_LIMIT )
		SETDBERR(&dberr, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else if ( lk_status == LK_NOLOCKS )
		SETDBERR(&dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dberr, 0, E_DM9500_DMXE_BEGIN);
	}
	else
	{
	    Psp->spcb_log_id = log_id;
	    Psp->spcb_lk_id  = lk_id;
	}

	/* Signal the query session that we're up and running */
	CSp_semaphore(1, &m->mx_cond_sem);
	m->mx_new_threads--;
	CScnd_signal(&m->mx_cond, m->mx_sid);

	if ( status == E_DB_OK )
	{
	    /* Wait til we're signalled to do something */
	    while ( (m->mx_state & 
		    (MX_PUTTOSORT | MX_LOADFROMSRC | 
		     MX_REORG | MX_ABORT)) == 0 )
	    {
		/* Abort if interrupted */
		if ( CScnd_wait(&m->mx_cond, &m->mx_cond_sem) )
		    m->mx_state |= MX_ABORT;
	    }
	    CSv_semaphore(&m->mx_cond_sem);

	    /* If parent didn't attach to CUT buffers yet, and if we need
	    ** to, do so.  We need to attach if there are target threads.
	    ** Note: we don't attach above, because the targets create the
	    ** CUT buffers.  By waiting until now, we know that all targets
	    ** are initialized and ready to go.
	    */
	    if (! cut_attached && m->mx_tpcb_threads > 0
	      && (m->mx_state & MX_ABORT) == 0)
	    {
		/* Attach to each target in write mode */
		for ( i = 0; i < m->mx_tpcb_count; i += Ptp->tpcb_count)
		{
		    Ptp = &m->mx_tpcb_next[i];
		    cut_rcb.cut_buf_use = CUT_BUF_WRITE;
		    cut_rcb.cut_buf_handle = Ptp->tpcb_buf_handle;
		    status = cut_attach_buf(&cut_rcb, &cut_error);
		    if (status != E_DB_OK)
			break;
		}
		cut_attached = TRUE;
	    }

	    /* For each Source we're responsible for... */
	    for ( i = 0; 
		  i < Psp->spcb_count && (m->mx_state & MX_ABORT) == 0
				 && status == E_DB_OK;
		  i++ )
	    {
		sp = &Psp[i];

		sp->spcb_log_id = log_id;
		sp->spcb_lk_id  = lk_id;

		if ( m->mx_state & MX_REORG )
		    status = DM2UReorg(sp, &dberr);
		else while ( (m->mx_state & MX_ABORT) == 0 && status == E_DB_OK )
		{
		    /*
		    ** Drain our source, put records to either
		    ** a sorter or directly to a Target.
		    */
		    status = NextFromSource(m, sp, 
					    &tp, &rec_ptr,
					    &tid8, &dberr);

		    /*
		    ** Returns with "tp" pointing to the assigned
		    ** target, rec_ptr pointing to the record,
		    ** and tid8 holding the record's TID.
		    ** (Rec_ptr points into spcb_record.)
		    **
		    ** If asymmetric target, feed it the record thru
		    ** its CUT buffer.
		    */
		    if ( status == E_DB_OK )
		    {
			if ( !(m->mx_state & MX_SYMMETRIC) )
			{
			    Ptp = tp->tpcb_parent;
			    sp->spcb_record->rhdr_state = m->mx_state & (MX_PUTTOSORT | MX_LOADFROMSRC);
			    sp->spcb_record->rhdr_partno = tp->tpcb_partno;
			    STRUCT_ASSIGN_MACRO(tid8, 
					sp->spcb_record->rhdr_tid8);
			    num_cells = 1;
			    cut_rcb.cut_buf_handle = Ptp->tpcb_buf_handle;
			    status = cut_write_buf(&num_cells, &cut_rcb,
					(PTR*)&sp->spcb_record, CUT_RW_WAIT,
					&cut_error);
			    if (status != E_DB_OK)
				dberr = cut_error;
			}
			else
			{
			    /*
			    ** The Target is not threaded. There is
			    ** a 1-1 symmetry between this source and
			    ** its target.
			    **
			    ** Put this source record either to the
			    ** sort or directly to the target build
			    ** function.
			    */
			    if ( m->mx_state & MX_PUTTOSORT )
				status = PutToSort(m, tp, rec_ptr,
						    &tid8, &dberr);
			    else if ( m->mx_state & MX_LOADFROMSRC )
			    {
				if ( (tp->tpcb_state & MX_OPENED) == 0 )
				    status = OpenTarget(tp, &dberr);

				if ( status == E_DB_OK )
				    status = PutToTarget(m, tp, 
							 rec_ptr,
							 status,
							 &dberr);
			    }
			}
		    }
		    else if ( status && dberr.err_code == E_DM0055_NONEXT &&
			     m->mx_state & MX_SYMMETRIC )
		    {
			/*
			** End of Source input and symmetric
			** target; we're not done yet.
			**
			** If sorted, extract the records from
			** the sort and put them to the
			** target's build function.
			**
			** If not sorting, we're done and
			** can finish the build by closing
			** the build files.
			*/

			/*
			** If Source is empty, "tp" was never
			** set, so do it now.
			*/
			tp = m->mx_tpcb_ptrs[sp->spcb_partno];

			if ( m->mx_state & MX_PUTTOSORT )
			{
			    status = E_DB_OK;
			    CLRDBERR(&dberr);

			    if ( tp->tpcb_srt )
				status = dmse_input_end(tp->tpcb_srt,
							&dberr);

			    if ( status == E_DB_OK )
			    {
				status = OpenTarget(tp, &dberr);

				while ( status == E_DB_OK &&
					(m->mx_state & MX_ABORT) == 0 )
				{
				    /*
				    ** Since there's but one Target,
				    ** "Ttp" returned will always
				    ** equal "tp".
				    */
				    status = NextFromSort(m, tp, &Ttp,
							&rec_ptr,
							&dberr);
				    if ( DB_SUCCESS_MACRO(status) )
					status = PutToTarget(m, Ttp,
							rec_ptr,
							status,
							&dberr);
				}
			    }
			}

			/* Close the Target files */
			if ( status && dberr.err_code == E_DM0055_NONEXT )
			{
			    /* If the Source was empty, Open hasn't been done */
			    local_status = E_DB_OK;

			    if ( (tp->tpcb_state & MX_OPENED) == 0 )
				local_status = OpenTarget(tp, &local_dberr);

			    if ( local_status == E_DB_OK )
				local_status = CloseTarget(tp, &local_dberr);

			    if ( local_status )
			    {
				status = local_status;
				dberr = local_dberr;
			    }
			}
		    }
		}

		/* This Source is finished */
		if ( status && dberr.err_code == E_DM0055_NONEXT )
		{
		    status = E_DB_OK;
		    CLRDBERR(&dberr);
		}
	    }
	    CSp_semaphore(1,&m->mx_cond_sem);

	    /* Count one less Source thread */
	    m->mx_spcb_threads--;
	    m->mx_threads--;
	}
    }

    if ( status && dberr.err_code != E_DM0055_NONEXT )
    {
	/* Signal abort to all threads */
	m->mx_state |= MX_ABORT;
	m->mx_status = status;
	m->mx_dberr = dberr;
	CScnd_broadcast(&m->mx_cond);
    }
    else
	CScnd_signal(&m->mx_cond, m->mx_sid);

    CSv_semaphore(&m->mx_cond_sem);

    /* Careful: our beloved DM2U_SPCB memory may be gone now */
    Psp = (DM2U_SPCB*)NULL;

    /* If we connected to a log context, disconnect */
    if ( log_id )
	LGend(log_id, 0, &sys_err);

    /* If we connected to a lock context, disconnect */
    if ( lk_id )
	LKrelease(LK_ALL, lk_id, (LK_LKID*)0,
			(LK_LOCK_KEY*)0, (LK_VALUE*)0,
			&sys_err);

    /* If we attached to CUT, disconnect */
    if (cut_attached)
    {
	/* Reset status just in case, so that termination always works */
	(void) cut_signal_status(NULL, E_DB_OK, &cut_error);
	(void) cut_thread_term(TRUE, &cut_error);
    }

    EXdelete();

    return(E_DB_OK);
}

/*{
** Name: DMUTarget - Factotum thread to receive records
**		     from a Source and put them to
**		     a Target.
**
** Description:
**
**      Started by SpawnDMUThreads for a Target.
**
**	Connects to the transaction logging/locking
**	contexts, then waits for something to do.
**
**	That "something" is signalled by data available in the
**	target's CUT buffer.  (Sources may sometimes operate entirely
**	on their own without CUT buffering, but targets are always fed
**	through a CUT buffer.)  Each record is prefixed by a state:
**
**		MX_PUTTOSORT	Put the next record to the
**				sorter using PutToSort().
**		MX_LOADFROMSORT	Get records back from the
**				sorter with NextFromSort()
**				and pass them to 
**				PutToTarget().
**		MX_LOADFROMSRC	Pass the next record directly
**				to PutToTarget().
**		MX_END		No more Source record coming;
**				call CloseTarget() to
**				complete the build.
**
**	Additionally, we watch for abort state in either the tpcb or
**	the mxcb.  Aborts are propagated asynchronously via cut_signal_status.
**
**		MX_ABORT	Stop whatever you're doing
**				and terminate.
**
**	When we terminate, we decrement mx_tpcb_threads and 
**	mx_threads and disconnect the log/locking contexts.
**
**	Note to the reader:  if you're looking to this stuff as an example
**	of how to use CUT, you may wish to look elsewhere.  This was my
**	first application of CUT released to the wild; and I might have
**	done it a little differently given some of the CUT machinery we
**	invented later on.  (schka24)
**
** Inputs:
**	ftx		SCF_FTX from SpawnDMUThreads with
**			ftx_data pointing to Target's TPCB.
**
** Outputs:
**	none
**
**	Returns:
**	    DB_STATUS	Always E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    If an error is encountered by this thread,
**	    the status and error code are placed in
**	    the MXCB and MX_ABORT is signalled to
**	    all other threads.
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
**	08-Mar-2004 (jenjo02)
**	    Each Target thread may be responsible for
**	    more than just one TPCB, depending on the mix
**	    of Source/Targets and the number of 
**	    available threads.
**	11-Mar-2004 (jenjo02)
**	    Make sure that if a Target receives no data
**	    its build file still gets created.
**	12-Mar-2004 (jenjo02)
**	    Fixes for lost MX_END signals, hung threads.
**	24-Mar-2004 (jenjo02)
**	    Catch MX_END and set Terminate while 
**	    inside TPCB loop, not outside.
**	31-Mar-2004 (schka24)
**	    Still racy, rework end handling so that caller waits for
**	    targets to empty before signalling end.  Separate target-busy
**	    flag from thread-wakeup flag.  Check for abort during
**	    sort-to-target loop.
**	    (jenjo02) Add missing EXdelete().
**	16-Apr-2004 (schka24)
**	    Redo (again!), this time to install CUT.  Two reasons: one, to
**	    get more buffering between source and target (with the old one-
**	    record buffer, we were pounding the condition variable with
**	    every single record).  Two, to make it a little easier to
**	    wire the COPY bulk-load mechanism into this stuff and consolidate
**	    bulk-load with modify.
**	26-Apr-2004 (schka24)
**	    Wait for a (writer) to attach before entering read loop.
**	    This was done to allow for the CUT read changes I'm making.
**	17-May-2004 (jenjo02)
**	    Parent TPCB thread now does sorting for all sibling
**	    Targets. Partition number added as most significant
**	    sort key.
**	15-Jul-2004 (jenjo02)
**	    CUT_BUFFER renamed to CUT_RCB, use larger CUT buffer to
**	    maybe improve concurrency.
**	18-Aug-2004 (jenjo02)
**	    cut_read_buffer prototype change, output buffer is
**	    now pointer to pointer for LOCATE mode support.
**	04-Apr-2005 (jenjo02)
**	    Adjust CUT buffer to include Source's TID8,
**	    drop test of MX_PRESORT which may not be known, yet.
**	11-Sep-2008 (kibro01) b120774
**	    Free the source memory if we get an error with the target but
**	    haven't finished the source read yet - otherwise the parent
**	    will find the pointer and try to free it later (in dmse_end_serial).
**	19-May-2010 (thaju02) Bug 122698
**	    If writing to a sort work file fails, DMUSource/cut_write_buf
**	    hangs waiting for something to write. Instead signal cut buffer
**	    threads that an error has occurred. 
*/
DB_STATUS
DMUTarget(
SCF_FTX		*ftx)
{
    DM2U_TPCB	*Ptp = (DM2U_TPCB*)ftx->ftx_data, *tp, *Ttp, *PrevTtp;
    DM2U_MXCB	*m = Ptp->tpcb_mxcb;
    STATUS	lg_status, lk_status;
    i4		log_id = 0;
    i4		lk_id  = 0;
    DB_STATUS	status = E_DB_OK;
    i4		err_code;
    i4		logging = m->mx_rcb->rcb_logging;
    i4		i;
    i4		state;
    i4		num_cells;
    CL_ERR_DESC sys_err;
    char	thread_name[40];
    EX_CONTEXT	context;
    CUT_RCB	cut_rcb;
    CUT_RCB	*cut_rcb_ptr;
    DB_ERROR	cut_error;
    PTR		rec_ptr;
    char	*big_buffer = NULL;
    SIZE_TYPE	big_buffer_size;
    char 	*cut_rec;
    STATUS	cl_stat;
    DM2U_RHEADER rheader;
    i4		cells_in_big_buffer;
    DB_ERROR	local_dberr, dberr;

    CLRDBERR(&dberr);

    if ( EXdeclare(DMUHandler, &context) )
    {
	status = E_DB_ERROR;
	SETDBERR(&dberr, 0, E_DM004A_INTERNAL_ERROR);
	CSp_semaphore(1, &m->mx_cond_sem);

	/* Count ourselves down for the count */
	m->mx_tpcb_threads--;
	m->mx_threads--;
    }
    else
    {
	/* Initialize our state and condition variables */
	Ptp->tpcb_state = MX_THREADED;

	/* First, connect to logging/locking */
	if ( lg_status = logging )
	    lg_status = LGconnect(m->mx_log_id, 
				   &log_id,
				   &sys_err);

	if ( (lk_status = lg_status) == OK )
	    lk_status = LKconnect((LK_LLID)m->mx_lk_id, 
				   (LK_LLID*)&lk_id,
				    &sys_err);

	if ( lg_status || lk_status )
	{
	    status = E_DB_ERROR;
	    if ( lg_status == LG_EXCEED_LIMIT )
		SETDBERR(&dberr, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else if ( lk_status == LK_NOLOCKS )
		SETDBERR(&dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dberr, 0, E_DM9500_DMXE_BEGIN);
	}
	else
	{
	    Ptp->tpcb_log_id = log_id;
	    Ptp->tpcb_lk_id  = lk_id;
	}

	/* Create an input buffer, attach to it */
	if (status == E_DB_OK)
	{
	    STprintf(thread_name,"DMUt (%d,%d):%d",Ptp->tpcb_tabid.db_tab_base,
			Ptp->tpcb_tabid.db_tab_index & ~DB_TAB_PARTITION_MASK,
			Ptp->tpcb_count);
	    STlcopy(thread_name,&cut_rcb.cut_buf_name[0],sizeof(cut_rcb.cut_buf_name)-1);

	    /* Note here, rows going thru CUT are the original source (base
	    ** table) rows, mx_width is the wrong width because it's the
	    ** result row size.
	    */
	    cut_rcb.cut_cell_size = m->mx_src_width + sizeof(DM2U_RHEADER);
	    /*
	    ** Well, that's almost true...
	    **
	    ** Allow for the worst case in which additional stuff
	    ** is appended to the source rows:
	    **
	    **		i4	hash_bucket
	    **		u_i2	Target partition
	    **		TID8	Source record's TID8
	    **
	    ** Normally we'd only be concerned with this if we 
	    ** will be sorting the source rows, but the decision
	    ** whether to sort or not (MX_PRESORT) may lag where
	    ** this thread is right now. By the time we get our
	    ** first row from CUT, the decision will have been
	    ** made and reflected in rhdr_state.
	    **
	    ** Index rows are rebuilt in a buffer apart from the cut buffer
	    ** (tpcb_irecord), so those we don't have to worry about
	    ** cell-size-wise.
	    */
	    if ( m->mx_operation == MX_MODIFY )
	    {
		/* Add sizeof(i2) for postfixed target partition number */
		cut_rcb.cut_cell_size += sizeof(i2);

		/* Add sizeof(TID8) for postfixed source TID8 */
		cut_rcb.cut_cell_size += sizeof(DM_TID8);

		/* Hash needs another four bytes to stow the bucket */
		if ( m->mx_structure == TCB_HASH )
		    cut_rcb.cut_cell_size += sizeof(i4);
	    }

	    /* Might be better to have some intelligence here,
	    ** although testing seems to indicate that buffer size is not
	    ** all that crucial.  Use the sort exchange buffer size for
	    ** lack of anything better.
	    */
	    /* Assume an even distribution of tuples to targets */
	    cut_rcb.cut_num_cells = 
		(i4)m->mx_rcb->rcb_tcb_ptr->tcb_rel.reltups /
			m->mx_tpcb_threads;
	    if ( cut_rcb.cut_num_cells > dmf_svcb->svcb_stbsize )
		cut_rcb.cut_num_cells = dmf_svcb->svcb_stbsize;
	    if ( cut_rcb.cut_num_cells < 10 )
		cut_rcb.cut_num_cells = 10;

	    cut_rcb.cut_buf_use = CUT_BUF_READ;
	    cut_rcb.cut_buf_handle = NULL;
	    cut_rcb_ptr = &cut_rcb;
	    status = cut_create_buf(1, &cut_rcb_ptr, &cut_error);
	    if (status == E_DB_OK)
	    {
		Ptp->tpcb_buf_handle = cut_rcb.cut_buf_handle;

		/* Try first for an output buffer 1/2 cut size */
		cells_in_big_buffer = cut_rcb.cut_num_cells / 2; 

		do
		{
		    /* Failing that, try iteratively for 1/2 that... */
		    big_buffer_size = cut_rcb.cut_cell_size * cells_in_big_buffer;
		    big_buffer = MEreqmem(0, big_buffer_size, FALSE, &cl_stat);

		} while ( cl_stat && cells_in_big_buffer << 1 > 1 );
		
		if ( cl_stat )
		{
		    /* Couldn't get blocked-up buffer, fall back to one row */
		    big_buffer = NULL;
		    cells_in_big_buffer = 1;
		}
		else
		    Ptp->tpcb_srecord = (DM2U_RHEADER*)big_buffer;
	    }
	}

	/* Initialize all other Targets belonging to this thread */
	for ( i = 1; i < Ptp->tpcb_count; i++ )
	{
	    Ptp[i].tpcb_parent = Ptp;
	    Ptp[i].tpcb_log_id = Ptp->tpcb_log_id;
	    Ptp[i].tpcb_lk_id = Ptp->tpcb_lk_id;
	    Ptp[i].tpcb_state = 0;
	}

	/* Signal the query session that we're up and running */
	CSp_semaphore(1, &m->mx_cond_sem);
	m->mx_new_threads--;
	CScnd_signal(&m->mx_cond, m->mx_sid);
	CSv_semaphore(&m->mx_cond_sem);

	/* Wait for at least one other thread to attach.  It will be a
	** writer.  If we don't do this, the cut-reads get E_DB_WARN
	** and the loop spins frantically until someone attaches!
	** One of the attaching threads will be the driver,
	** and the sources won't start up (and possibly exit!) until
	** the driver is attached.  So it all hangs together...
	*/
	if (status == E_DB_OK)
	{
	    i = 2;			/* us + one other */
	    status = cut_event_wait(&cut_rcb, CUT_ATTACH,
			(PTR) &i, &cut_error);
	    if (status != E_DB_OK)
		dberr = cut_error;
	}

	/* Big read loop, loop until driver sends us an END dummy record */
	/* NOTE: we don't have to deal with E_DB_WARN from the CUT read.
	** The way the DMU stuff was coded, there's an explicit END
	** record, and we never look for more after that.
	*/
	num_cells = 0;

	while (status == E_DB_OK)
	{
	    if ( --num_cells > 0 )
		cut_rec += cut_rcb.cut_cell_size;
	    else
	    {
		/* How many we want */
		num_cells = cells_in_big_buffer;

		cut_rcb.cut_buf_handle = Ptp->tpcb_buf_handle;
		status = cut_read_buf(&num_cells, &cut_rcb, 
				      (PTR*)&Ptp->tpcb_srecord,
				      CUT_RW_WAIT, &cut_error);

		if ( status == E_DB_ERROR || num_cells == 0 )
		{
		    dberr = cut_error;
		    break;
		}

		/* CUT set num_cells to actual */
		status = E_DB_OK;
		cut_rec = (PTR)Ptp->tpcb_srecord;
	    }
	    MEcopy((char*)cut_rec, sizeof(DM2U_RHEADER),
		   (char*)&rheader);


	    /* Extract state for this record */
	    state = rheader.rhdr_state;
	    rec_ptr = (char *)cut_rec + sizeof(DM2U_RHEADER);
	    
	    if ( (state & (MX_END | MX_LOADFROMSORT)) != 0)
	    {
		/* "end" and "load from sort" are both signalled
		** synchronously by the driver after all sources are
		** clear.  So these two apply to all targets for this
		** thread.
		*/
		if ( state & MX_END )
		{
		    /*
		    ** When "end" is signalled, make sure that all
		    ** Targets have been opened and closed;
		    ** some targets may receive no source rows
		    ** but the underlying files must be created.
		    */
		    for (i = 0; 
			 i < Ptp->tpcb_count && status == E_DB_OK;
			 ++i)
		    {
			tp = &Ptp[i];

			if ( (tp->tpcb_state & MX_OPENED) == 0 )
			    status = OpenTarget(tp, &dberr);

			if ( status == E_DB_OK &&
			     (tp->tpcb_state & MX_CLOSED) == 0 )
			{
			    status = CloseTarget(tp, &dberr);
			}
		    }
		}
		/* May have never started a sort on this Parent */
		else if ( state & MX_LOADFROMSORT && Ptp->tpcb_srt )
		{
		    i4 counter = 0;

		    /* Set for partno control break */
		    PrevTtp = NULL;
		    
		    /* End of input, do merge phase */
		    status = dmse_input_end(Ptp->tpcb_srt, &dberr);

		    /* Drain sorter into output file */
		    while ( status == E_DB_OK )
		    {
			if (++ counter >= 10000 /* arbitrary */ )
			{
			    /* Every now and then, check for abort
			    ** that may have been signalled by some
			    ** other thread, to avoid wasting hours
			    ** on a useless build.
			    */
			    CSp_semaphore(1, &m->mx_cond_sem);
			    counter = m->mx_state;
			    CSv_semaphore(&m->mx_cond_sem);
			    if (counter & MX_ABORT)
			    {
				/* Pretend it came from cut */
				status = E_DB_ERROR;
				SETDBERR(&dberr, 0, E_CU0204_ASYNC_STATUS);
				break;
			    }
			    counter = 0;
			}
			    
			/*
			** The sort holds all rows for all
			** Targets under the control of this
			** Parent, grouped by partition number.
			**
			** "NextFromSort" extracts the appended
			** partition number from the sort record
			** and returns its Target TPCB in "Ttp".
			*/
			status = NextFromSort(m, tp, &Ttp,
					&rec_ptr,
					&dberr);
			if ( DB_SUCCESS_MACRO(status) )
			{
			    /*
			    ** When a change in partition occurs,
			    ** close the current Target, 
			    ** open the new one.
			    */
			    if ( Ttp != PrevTtp )
			    {
				if ( (PrevTtp &&
				       CloseTarget(PrevTtp, &dberr))
				    || OpenTarget(Ttp, &dberr) )
				{
				    status = E_DB_ERROR;
				}
				else
				   PrevTtp = Ttp;
			    }

			    if ( DB_SUCCESS_MACRO(status) )
			    {
				status = PutToTarget(m, Ttp,
					    rec_ptr,
					    status, &dberr);

/* If we fail to insert the record, we are responsible for clearing the
** memory we've used (at least removing the references to it) so that the
** parent doesn't try to - it's our memory, not the parent's
** (kibro01) b120774
*/
				if (status != E_DB_OK)
				{
				    DM2U_TPCB	*Ptp = tp->tpcb_parent;
				    DM2U_M_CONTEXT	*mct;
				    Ptp->tpcb_srt = (DMS_SRT_CB*)NULL;
				    if ( Ptp->tpcb_srt_wrk )
					dm0m_deallocate((DM_OBJECT **)&Ptp->tpcb_srt_wrk);
				    mct = &tp->tpcb_mct;
				    if ( mct->mct_buffer )
					dm0m_deallocate( (DM_OBJECT **)&mct->mct_buffer);
				}
			    }
			}
		    } /* next from sort */

		    /*
		    ** At the end of the sort, close the last
		    ** Target.
		    */
		    if ( status && dberr.err_code == E_DM0055_NONEXT )
			status = CloseTarget(Ttp, &dberr);
		} /* if LOADFROMSORT */

		if (status != E_DB_OK || state & MX_END)
		    /* Error or normal end, exit the cut read loop */
		    break;
		else
		    /* load-from-sort, wait for more instructions */
		    continue;

	    } /* if END or LOADFROMSORT */

	    /* Pass record and/or state to proper TPCB.
	    ** Must be doing put-to-sort or put-to-target.
	    ** (In the target, put-to-target is flagged by the
	    ** MX_LOADFROMSRC state, rather counter-intuitively.)
	    */

	    tp = m->mx_tpcb_ptrs[rheader.rhdr_partno];

	    if (state & MX_PUTTOSORT)
	    {
		status = PutToSort(m, tp, rec_ptr,
			    &rheader.rhdr_tid8, &dberr);
	    }
	    else if ( state & MX_LOADFROMSRC )
	    {

		if ( (tp->tpcb_state & MX_OPENED) == 0 )
		{
		    status = OpenTarget(tp, &dberr);
		}

		if ( status == E_DB_OK )
		    status = PutToTarget(m, tp, rec_ptr,
				    status, &dberr);
	    }
	    /* FIXME else should issue internal error */

	    if ( status )
	    {
		tp->tpcb_state |= MX_ABORT;
		Ptp->tpcb_state |= MX_ABORT;
	    }

	} /* while read */

	if (DMZ_SRT_MACRO(11) && Ptp->tpcb_buf_handle != NULL)
	{
	    CUT_BUF_INFO cut_stats;
	    cut_rcb.cut_buf_handle = Ptp->tpcb_buf_handle;
	    cut_info(CUT_INFO_BUF,&cut_rcb,(PTR)&cut_stats,&cut_error);
	    TRdisplay("%@ Target %d: reads %d, writes %d, read-waits %d, write-waits %d, num_cells %d\n",
		Ptp - m->mx_tpcb_next, cut_stats.cut_cell_reads, cut_stats.cut_cell_writes,
		cut_stats.cut_read_waits, cut_stats.cut_write_waits, cells_in_big_buffer);
	}

	Ptp->tpcb_state |= MX_TERMINATED;

	CSp_semaphore(1, &m->mx_cond_sem);

	/* Count one less Target thread */
	m->mx_tpcb_threads--;
	m->mx_threads--;
    }

    if ( status != E_DB_OK
      && dberr.err_code != E_CU0204_ASYNC_STATUS
      && dberr.err_code != E_DM9714_PSORT_CFAIL)
    {
	/* Record error status globally,
	** unless our abort came from the outside already.
	** (CSattn from CUT gives sorter a 9714.)
	*/
	m->mx_state |= MX_ABORT;
	m->mx_status = status;
	m->mx_dberr = dberr;
	CScnd_broadcast(&m->mx_cond);
    }
    else
	CScnd_signal(&m->mx_cond, m->mx_sid);

    CSv_semaphore(&m->mx_cond_sem);

    /* Careful: our beloved DM2U_TPCB memory may be gone now */
    Ptp = (DM2U_TPCB*)NULL;

    /* Detach CUT, ignore error status; reset CUT status first to assure
    ** that termination will always work.
    */
    if (m->mx_state & MX_ABORT)
	(void) cut_signal_status(NULL, E_DB_ERROR, &cut_error);
    else
	(void) cut_signal_status(NULL, E_DB_OK, &cut_error);
    (void) cut_thread_term(TRUE, &cut_error);

    /* If we connected to a log context, disconnect */
    if ( log_id )
	LGend(log_id, 0, &sys_err);
    log_id = 0;

    /* If we connected to a lock context, disconnect */
    if ( lk_id )
	LKrelease(LK_ALL, lk_id, (LK_LKID*)0,
			(LK_LOCK_KEY*)0, (LK_VALUE*)0,
			&sys_err);
    lk_id = 0;

    /* Free big buffer if acquired */
    if ( big_buffer )
	MEfree(big_buffer);

    EXdelete();

    return(E_DB_OK);
}

/*{
** Name: DMUHandler	- DMU threads exception handler
**
** Description:
**	Catches exceptions that occur in DMU threads.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-Feb-2004 (jenjo02)
**	    Created.
*/
static STATUS
DMUHandler(
EX_ARGS		    *ex_args)
{
    i4	    err_code;
    i4	    status;
    
    if ( ex_args->exarg_num == EX_DMF_FATAL )
	err_code = E_DM904A_FATAL_EXCEPTION;
    else
	err_code = E_DM9049_UNKNOWN_EXCEPTION;

    ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    return (EXDECLARE);
}

/*{
** Name: dm2u_update_catalogs	- Update catalogs after modify or index.
**
** Description:
**	Before a modify or index command is finished, the system tables
**	have to be updated to reflect the changed in relation attributes,
**	attribute attributes and indexes attributes.  This routine handles
**	making the appropriate changes for each.
**
** Inputs:
**      mxcb                            The modify index control block.
**	journal				Indicates whether user table is
**					journaled.
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
**	24-feb-86 (derek)
**          Created new for jupiter.
**      19-nov-87 (jennifer)
**          Added support for multiple locations per file.
**	31-oct-88 (rogerk)
**	    When modify or create a secondary index, update the timestamp of
**	    the base table.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Check for modify on tables that
**	    have no underlying file (like a gateway table).  Don't do file
**	    renames on such tables.
**	 2-oct-89 (rogerk)
**	    When moving new modify files over to be table files, if the new
**	    table file has the same name as the old one, then log both
**	    renames (old -> del, mod -> new) with one dm0l_fswap operation.
**	    This fixes problem where if we fail in the middle of the file
**	    rename operations, we don't evaporate the user's table.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**	     7-mar-1991 (bryanp)
**		Added support for Btree index compression:
**		1) when turning off TCB_COMPRESSED, turn off TCB_INDEX_COMP
**		2) if index compression was specified, turn on TCB_INDEX_COMP
**	23-oct-1991 (rogerk)
**	    Fixed call to dm2r_delete to not pass pointer to device record
**	    being deleted.  The dm2r_delete call does not take this parameter.
**	    Turns out this probably did not cause many problems, other than
**	    if an error occurred in dm2r_delete, we would log a bogus errcode.
**	15-apr-1992 (bryanp)
**	    Remove the "FCB" argument from dm2f_rename().
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Update relcomptype.
**    	29-August-1992 (rmuth)
**          Removed the 6.4->6.5 on-the-fly conversion code.
**	29-oct-1992 (jnash)
**	    Reduced logging project.  Elimin dm0l_fswap, replace with
**	    dm0l_fcreate.  CLR's make rename operations idempotent, making
**	    swaps are no longer necessary.
**	8-apr-93 (rickh)
**	    Added support for new relstat2 bits (FIPS CONSTRAINTS, release
**	    6.5).
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix core system catalog pages after updating them, to comply
**		    with the node failure recovery rule that no transaction
**		    can fix more than one syscat page at a time.
**	15-may-1993 (rmuth)
**	    Add support for concurrent index. If building a concurrent index
**	    do not update the base table's iirelation record to indicate that
**	    the new index exists.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Changed journaling state of the dm0l_frename log records to be
**	    dependent upon the database journal state rather than the usertab.
**	    Documented 'journal' parameter to mean journal status of user
**	    table rather than whether or not updates should be journaled.
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	    Removed journaling of log records in this routine except when
**	    updating catalogs for journaled tables.
**	  - Changed updating of iidevices rows to replace existing entries
**	    rather than deleting the old ones and inserting new ones.  Also
**	    changed to leave around excess entries but blank the devloc field
**	    turning them into dummy rows.
**      27-dec-94 (cohmi01)
**	    For RAW I/O, changes to pass more info down to dm2f_xxx functions.
**	    If tbio ptr in FCB is null, set up a local DMP_TABLE_IO with the
**	    tid from mx, point FCB to it. Pass location ptr to dm2f_rename().
**          When updating pending queue, add additional info to XCCB.
**	21-feb-1995 (shero03)
**	    Bug #66931
**	    Use LZW compression for "btree with compression" 
**	06-mar-1996 (stial01 for bryanp)
**	    Ensure that relpgsize is updated to match the post-modify pagesize.
**	29-apr-1996 (shero03)
**	    Fill in the iirange table for RTree
**	22-jul-1996 (ramra01)
**	    Alter Table Project:
**		In modifying base tables (storage structures only) reformat
**		tuples and update iirelation and iiattribute.
**	25-oct-1996 (hanch04)
**	    Set intl_id = to attid durring an upgrade.
**	31-jan-1997 (nanpr01)
**	    Bug : 79853 : Comment catalogs are not fixed up correctly
**	    for altered tables with comment.
**	31-Mar-1997 (jenjo02)
**	    Update relation's cache priority with value in mxcb.
**	06-Jun-1997 (nanpr01)
**	    Initialize the new fields in xccb.
**	01-oct-1998 (nanpr01)
**	    Added the new param in dm2r_replace for update performance imp.
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**	06-Jan-1998 (shero03)
**	    For Rtree tables, delete the old iirange records - don't destroy it.
**	09-Apr-1999 (jenjo02)
**	    Removed local DMP_TABLE_IO (tio) for RAW, no longer needed.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0,
**	    <= 0 is a base table.
**	6-Feb-2004 (schka24)
**	    Update dm2f-filename calls.
**	28-Feb-2004 (jenjo02)
**	    Modified to deal with multiple partitions.
**	01-Mar-2004 (jenjo02)
**	    Don't bother looking for iiattributes for 
**	    partitions; they won't have any.
**	04-Mar-2004 (jenjo02)
**	    Init status, clarified comments re 
**	    Partitions and TPCBs.
**	08-Mar-2004 (jenjo02)
**	    Repair Source/Target file renames,
**	    set/unset appropriate TCB_IS_PARTITIONED
**	    iirelation values.
**	10-Mar-2004 (jenjo02)
**	    Apply Master-wide location changes to 
**	    Master's iirelation/iidevices.
**	29-Apr-2004 (jenjo02)
**	    Check for user interrupts and force-abort; 
**	    when lots of partitions are paired with a 
**	    too-small log file, the log might fill.
**	18-Apr-2005 (jenjo02)
**	    Carved source file rename, pend delete code,
**	    into RenameForDeletion() static so it can be
**	    called for CREATE INDEX targets as well as MODIFY
**	    sources.
**	20-jul-2005 (abbjo03)
**	    Call RenameForDeletion() for partitioned tables as well as INDEX.
**	21-Jul-2005 (jenjo02)
**	    RenameForDeletion only -new- partitions.
**	28-Nov-2005 (kschendel)
**	    Properly set partition's relallocation.   It was being set to
**	    the master's relallocation.
**	13-Apr-2006 (jenjo02)
**	    Set TCB_CLUSTERED in relstat when indicated, force
**	    TCB_UNIQUE.
**	30-May-2006 (jenjo02)
**	    DMP_INDEX idom expanded from DB_MAXKEYS to DB_MAXIXATTS to support
**	    indexes on clustered tables.
**	11-Nov-2009 (smeke01) b113524
**	    When creating an index (and primary key and foreign key 
**	    constraints since these are implemented as indexes), copy the 
**	    journaling state from the base table into the index's iirelation 
**	    record.
**	4-May-2010 (kschendel)
**	    DM9026 can't take parameters, used too often by setdberr.
**	    Drop the param in the message here.
**	13-may-2010 (miket) SIR 122403
**	    Fix net-change logic for width for ALTER TABLE.
*/
DB_STATUS
dm2u_update_catalogs(
DM2U_MXCB           *m,
i4		    journal,
DB_ERROR      	    *dberr)
{
    DM2U_M_CONTEXT	*mct;
    DM2U_TPCB		*tp;
    DM2U_SPCB		*sp;
    DMP_RCB		*r = (DMP_RCB *)0;
    DMP_RCB		*rcb;
    DB_TAB_ID		table_id;
    DB_TAB_ID		idx_id;
    DM_TID		tid;
    DB_STATUS		status = E_DB_OK;
    i4			i,k;
    i4             	local_error;
    i4			dm0l_flag;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	qual_list[2];
    LG_LSN		lsn;
    CL_ERR_DESC		sys_err;
    i4			nofiles = 0;
    i4			drop_cnt = 0;
    i4			col_cnt = 0;
    i4			attr_map[DB_MAX_COLS + 1];
    i4			new_map[DB_MAX_COLS + 1];
    i4			attr_id = 0;
    i4			order = 0;
    bool		comment = 0;
    i4			new_locations, loc_count, oloc_count;
    i4			error;
    DMP_LOCATION 	*loc;
    DB_TAB_ID		*loc_tabid;
    DMU_PHYPART_CHAR    *phypart = (DMU_PHYPART_CHAR*)m->mx_phypart;

    DMP_RELATION	rel;
    DMP_ATTRIBUTE	att;
    DMP_INDEX		idx;
    DB_ERROR		local_dberr;
    u_i4		journaling = 0;
    bool		base_table_seen = FALSE;
    DM_TID		index_tid;

    index_tid.tid_i4 = 0;

    for (;;)
    {
	/*  Build a qualification list used for searching the system tables. */

	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *)&m->mx_table_id.db_tab_base;
	qual_list[1].attr_number = DM_2_ATTRIBUTE_KEY;
	qual_list[1].attr_operator = DM2R_EQ;
	qual_list[1].attr_value = (char *)&m->mx_table_id.db_tab_index;

	if (m->mx_reorg == 0)
	{
	    /*  Update indexes catalog and destroy indices. */

	    table_id.db_tab_base = DM_B_INDEX_TAB_ID;
	    table_id.db_tab_index = DM_I_INDEX_TAB_ID;
	    status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
		DM2T_A_WRITE, (i4)0, (i4)20, (i4)0,
		m->mx_log_id, m->mx_lk_id, (i4)0, (i4)0,  
		DM2T_S, &m->mx_xcb->xcb_tran_id, &timestamp, &rcb, 
		(DML_SCB *)0, dberr);
	    if (status != E_DB_OK)
		break;
	    r = rcb;
	    r->rcb_xcb_ptr = m->mx_xcb;

	    /*
	    ** Check for NOLOGGING - no updates should be written to the log
	    ** file if this session is running without logging.
	    */
	    if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
		r->rcb_logging = 0;
	    r->rcb_usertab_jnl = (journal) ? 1 : 0;

	    if (m->mx_operation == MX_MODIFY)
	    {
		status = dm2r_position(r, DM2R_QUAL, qual_list,
			     (i4)1,
			     (DM_TID *)0, dberr);
		if (status != E_DB_OK)
		    break;

		for (;;)
		{
		    status = dm2r_get(r, &tid, DM2R_GETNEXT,
				 (char *)&idx, dberr);
			
		    if (status == E_DB_OK)
		    {
			/* FIX ME if online modify don't destroy new
			** persistent indexes
			*/
			if (m->mx_table_id.db_tab_index <= 0 && 
			    idx.baseid == m->mx_table_id.db_tab_base)
			{
			    idx_id.db_tab_base = idx.baseid;
			    idx_id.db_tab_index = idx.indexid;

			    status = dm2r_unfix_pages(r, dberr);
			    if (status)
				break;

			    status = dm2u_destroy(m->mx_dcb, m->mx_xcb,
				     &idx_id, DM2T_X, DM2U_MODIFY_REQUEST,
				     0, 0, 0, 0, dberr);

			    if (status != E_DB_OK)
				break;
			}
			continue;
		    }
		    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(dberr);
		    }
		    break;
		}
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		/* Create INDEX */
		idx.baseid = m->mx_table_id.db_tab_base;
		idx.indexid = m->mx_table_id.db_tab_index;
		idx.sequence = 0;
		for (i = 0; i < DB_MAXIXATTS; i++)
			idx.idom[i] = m->mx_idom_map[i];

		status = dm2r_put(r, DM2R_NODUPLICATES,
			     (char *)&idx, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    status = dm2t_close(r, 0, dberr);
	    if (status != E_DB_OK)
		break;
	    r = 0;

	    /* Partitions have no attributes of their own, use Master's */
	    if ( !(m->mx_table_id.db_tab_index & DB_TAB_PARTITION_MASK) )
	    {
		/*  Update attribute records.  attkey. */

		table_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
		table_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;
		status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
				    DM2T_A_WRITE, (i4)0, (i4)20, 
				    (i4)0, m->mx_log_id, m->mx_lk_id,
				    (i4)0, (i4)0, DM2T_S, 
				    &m->mx_xcb->xcb_tran_id, &timestamp, &rcb, 
				    (DML_SCB *)0, dberr);
		if (status != E_DB_OK)
		    break;
		r = rcb;
		r->rcb_xcb_ptr = m->mx_xcb;

		/*
		** Check for NOLOGGING - no updates should be written to the log
		** file if this session is running without logging.
		*/
		if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
		    r->rcb_logging = 0;
		r->rcb_usertab_jnl = (journal) ? 1 : 0;

		status = dm2r_position(r, DM2R_QUAL, qual_list,
			     (i4)2,
			     (DM_TID *)0, dberr);
		if (status != E_DB_OK)
		    break;


		for (attr_id = 0; attr_id < DB_MAX_COLS + 1; attr_id++)
		{
		    attr_map[attr_id] = 0;
		    new_map[attr_id] = 0;
		}

		for (;;)
		{
		    /*
		    ** Required for upgrade of iiattribute
		    */
		    att.attintl_id = 0;
		    status = dm2r_get(r, &tid, DM2R_GETNEXT,
				 (char *)&att, dberr);

		    if (status == E_DB_OK)
		    {
			/*
			** Required for upgrade of iiattribute
			*/
			if ( att.attintl_id == 0 )
			{
			    att.attintl_id = att.attid;
			    att.attver_added = 0;
			    att.attver_dropped = 0;
			    att.attval_from = 0;
			    att.attver_altcol = 0;
			}
			if (att.attrelid.db_tab_base ==
					       m->mx_table_id.db_tab_base &&
			    att.attrelid.db_tab_index ==
					       m->mx_table_id.db_tab_index)
			{
			    col_cnt++;
			    if (att.attver_dropped >
				    att.attver_added)
			    {
				attr_map[att.attintl_id] = 1; 
				drop_cnt++;
				continue;
			    }
			}
			continue;
		    }
		    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(dberr);
		    }
		    break;
		}
		if (status != E_DB_OK)
		    break;

		status = dm2r_position(r, DM2R_QUAL, qual_list, (i4)2, 
				       (DM_TID *)0, 
				       dberr);
		if (status != E_DB_OK)
		    break;
     
		for (;;)
		{
		    /*
		    ** Required for upgrade of iiattribute
		    */
		    att.attintl_id = 0;
		    status = dm2r_get(r, &tid, DM2R_GETNEXT,
				 (char *)&att, dberr);

		    if (status == E_DB_OK)
		    {
			/*
			** Required for upgrade of iiattribute
			*/
			if ( att.attintl_id == 0 )
			{
			   att.attintl_id = att.attid;
			   att.attver_added = 0;
			   att.attver_dropped = 0;
			   att.attval_from = 0;
			   att.attver_altcol = 0;
			}
			if (att.attrelid.db_tab_base ==
					       m->mx_table_id.db_tab_base &&
			    att.attrelid.db_tab_index ==
					       m->mx_table_id.db_tab_index)
			{
			    if (att.attver_dropped > att.attver_added)
			    {
				status = dm2r_delete(r, &tid, DM2R_BYPOSITION, 
							    dberr);
				if (status != E_DB_OK)
				    break;
				continue;
			    }

			    if (m->mx_att_map[att.attintl_id] != att.attkey)
			    {
				att.attkey = m->mx_att_map[att.attintl_id];
			    }
			    {
				att.attver_dropped = 0;
				att.attver_added = 0;
			        att.attver_altcol = 0;
				order = 0;
				for(attr_id = att.attintl_id;
				    attr_id > 0;
				    attr_id--)
				{
				    if (attr_map[attr_id] == 1)
					order++;
				}
				/* Store the new att_intl_id */
				new_map[att.attintl_id] = att.attintl_id - order;
				att.attintl_id -= order;
				status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
					     (char *)&att, 
					     (char *)NULL, dberr);
				if (status != E_DB_OK)
				    break;

			    }
			}
			continue;
		    }
		    if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(dberr);
		    }
		    break;
		}
		if (status != E_DB_OK)
		    break;

		status = dm2t_close(r, DM2T_NOPURGE, dberr);
		if (status != E_DB_OK)
		    break;
		r = 0;
	    }

	    if (m->mx_structure == TCB_RTREE)
	    {
		  DMP_RANGE	iirrec;
		  i4	*irange;
		  f8		*fll, *fur;

	      /*  Update range catalog. */

	      table_id.db_tab_base = DM_B_RANGE_TAB_ID;
	      table_id.db_tab_index = DM_I_RANGE_TAB_ID;
	      status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
		            DM2T_A_WRITE,
		            (i4)0, (i4)20, (i4)0,
		            m->mx_log_id, m->mx_lk_id,
		            (i4)0, (i4)0,  DM2T_S, 
			    &m->mx_xcb->xcb_tran_id, &timestamp, &rcb, 
			    (DML_SCB *)0, dberr);
	      if (status != E_DB_OK)
		  break;

	      r = rcb;
	      r->rcb_xcb_ptr = m->mx_xcb;

	      /*
	      ** Check for NOLOGGING - no updates should be written to the log
	      ** file if this session is running without logging.
	      */
	      if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
		    r->rcb_logging = 0;
	      r->rcb_usertab_jnl = (journal) ? 1 : 0;

	      status = dm2r_position(r, DM2R_QUAL, qual_list,
			(i4)2,
			(DM_TID *)0, dberr);
	      if (status != E_DB_OK)
		  break;

	      for (;;)
	      {	
	         status = dm2r_get(r, &tid, DM2R_GETNEXT,
			(char *)&iirrec, dberr);

	         if (status == E_DB_OK)
	         {
	            if (iirrec.range_baseid == m->mx_table_id.db_tab_base &&
	  	        iirrec.range_indexid == m->mx_table_id.db_tab_index)
	            {
		      status = dm2r_delete(r, &tid,DM2R_BYPOSITION, dberr);
		      if (status != E_DB_OK)
			  break;
		    }
	         }
		 else
		    break;
	      }

	      if (status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
	      {
	        status = E_DB_OK;
		CLRDBERR(dberr);
	      }
	    
	      iirrec.range_baseid = m->mx_table_id.db_tab_base;
	      iirrec.range_indexid = m->mx_table_id.db_tab_index;
	      iirrec.range_dimension = m->mx_acc_klv->dimension;
	      iirrec.range_hilbertsize = m->mx_acc_klv->hilbertsize;
	      iirrec.range_datatype = m->mx_acc_klv->box_dt_id;
	      iirrec.range_type = m->mx_acc_klv->range_type;
	      if (iirrec.range_type == 'I')
	        irange = (i4 *)m->mx_range;
	      fll = &iirrec.range_ll[0];
	      fur = &iirrec.range_ur[0];
	      for (i = 0; i < DB_MAXRTREE_DIM; i++, fll++, fur++)
	      {
	        if (i < m->mx_dimension)
	        {
		  if (iirrec.range_type == 'I')
		  {
	       	    *fll = irange[i];
	            *fur = irange[i + m->mx_dimension];
		  }
		  else
		  {
	       	    *fll = m->mx_range[i];
		    *fur = m->mx_range[i + m->mx_dimension];
		  }
	        }
	        else
	        {
	          *fll = 0.0;
	       	  *fur = 0.0;
	        }
	      }

	      status = dm2r_put(r, DM2R_NODUPLICATES,
				            (char *)&iirrec, dberr);
	      if (status != E_DB_OK)
	        break;
	      
	      status = dm2t_close(r, 0, dberr);
	      if (status != E_DB_OK)
	        break;
	
	      r = 0;

	    }  /* RTree index */

	} /* End if reorg not set. */

	/*
	** ******** Update IIDEVICES **********
	**
	** If the table changed locations and the old or new table had
	** multiple locations then we need to update iidevices information.
	** (Relocation of a single loc table is implemented by simply
	** updating the relloc field of iirelation).
	**
	** To conform to non-journal table protocols, we cannot delete
	** entries from iidevices as we don't want to delete rows from
	** journaled core catalogs that describe non-journaled tables.
	** If the old location count exceeds the new count then any excess
	** iidevices rows are updated to 'dummy' rows (with blank devloc
	** fields) rather than being deleted.  Any later reorganize operations
	** may be able to re-use these dummy rows.
	**
	** See DMFRFP for further information on non-journaled table protocols.
	*/

	/*
	** In partitioned tables, both Master and the Partitions
	** may have location information; Master's is only used
	** as the "default" location info for the Partitions,
	** never for I/O.
	*/

	tp = (DM2U_TPCB*)NULL;

	do
	{
	    /* Check for interrupts in this loop. */
	    if ( status = dm2u_check_for_interrupt(m->mx_xcb, dberr) )
		break;

	    /* Do Master first, then individual targets */
	    if ( m->mx_Mlocations && tp == (DM2U_TPCB*)NULL )
	    {
		new_locations = TRUE;
		loc_count = m->mx_Mloc_count;
		oloc_count = m->mx_Moloc_count;
		loc = m->mx_Mlocations;
		loc_tabid = &m->mx_table_id;
		/* After Master, do Partitions */
		tp = m->mx_tpcb_next;
	    }
	    else
	    {
		if ( !tp )
		    tp = m->mx_tpcb_next;
		new_locations = tp->tpcb_new_locations;
		loc_count = tp->tpcb_loc_count;
		oloc_count = tp->tpcb_oloc_count;
		loc = tp->tpcb_locations;
		loc_tabid = &tp->tpcb_tabid;
		/* Set up for next Partition */
		tp = tp->tpcb_next;
	    }

	    if ( new_locations && (loc_count > 1 || oloc_count > 1) )
	    {
		DMP_DEVICE          devrecord;
		i4             devices_rows;
		i4             k;

		if ( !r )
		{
		    table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
		    table_id.db_tab_index= DM_I_DEVICE_TAB_ID;
		    status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
			DM2T_A_WRITE, (i4)0, (i4)20, (i4)0,
			m->mx_log_id, m->mx_lk_id,
			(i4)0, (i4)0, DM2T_S, &m->mx_xcb->xcb_tran_id,
			&timestamp, &rcb, (DML_SCB *)0, dberr);
		    if (status != E_DB_OK)
			break;
		    r = rcb;
		    r->rcb_xcb_ptr = m->mx_xcb;

		    /*
		    ** Check for NOLOGGING - no updates should be written to the log
		    ** file if this session is running without logging.
		    */
		    if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
			r->rcb_logging = 0;
		    r->rcb_usertab_jnl = (journal) ? 1 : 0;
		}

		qual_list[1].attr_value = (char*)&loc_tabid->db_tab_index;

		status = dm2r_position(r, DM2R_QUAL, qual_list,
			     (i4)2,
			     (DM_TID *)0, dberr);
		if (status != E_DB_OK)
		    break;

		/*
		** Update any existing iidevices rows to the new location set.
		** If there are more existing iidevices entries than are needed
		** to describe the new location set then the extra entries will
		** be replaced to be 'dummy' rows - rows with blank devloc values.
		*/
		for (devices_rows = 0;; devices_rows++)
		{
		    status = dm2r_get(r, &tid, DM2R_GETNEXT,
				 (char *)&devrecord, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			{
			    status = E_DB_OK;
			    CLRDBERR(dberr);
			}
			break;
		    }

		    /*
		    ** If this iidevices row describes a locid that is included
		    ** in the new location list, then update it to show the new
		    ** location, otherwise change it to be a dummy row.
		    */
		    if (devrecord.devlocid < loc_count)
		    {
			STRUCT_ASSIGN_MACRO(
			    loc[devrecord.devlocid].loc_name,
			    devrecord.devloc);
		    }
		    else
		    {
			MEfill(sizeof(devrecord.devloc), ' ',
			    (char *) &devrecord.devloc);
		    }

		    status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
				 (char *)&devrecord, (char *)NULL, dberr);
		    if (status != E_DB_OK)
			break;
		}
		if (status != E_DB_OK)
		    break;

		/*
		** Add new iidevices rows if the new location count exceeds the
		** old one.
		*/
		for (k = (devices_rows + 1); k < loc_count; k++)
		{
		    devrecord.devreltid.db_tab_base = loc_tabid->db_tab_base;
		    devrecord.devreltid.db_tab_index = loc_tabid->db_tab_index;
		    devrecord.devlocid = loc[k].loc_id;
		    STRUCT_ASSIGN_MACRO(loc[k].loc_name,
					     devrecord.devloc);

		    status = dm2r_put(r, DM2R_DUPLICATES,
			     (char *)&devrecord, dberr);
		    if (status != E_DB_OK)
			break;
		}
	    }
	} while ( tp && status == E_DB_OK );

	if ( r && status == E_DB_OK )
	{
	    /* Close IIDEVICES */
	    status = dm2t_close(r, DM2T_NOPURGE, dberr);
	    r = 0;
	}

	if (status != E_DB_OK)
	    break;

	/* Reset DMR qualifier to modify table */
	qual_list[1].attr_value = (char *)&m->mx_table_id.db_tab_index;

	/*  Update the relation record.  relspec,relstat,relstamp... */

	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;

	status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0,
		m->mx_log_id,m->mx_lk_id,
		(i4)0, (i4)0, DM2T_S, &m->mx_xcb->xcb_tran_id,
                &timestamp, &rcb, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;
	r = rcb;
	r->rcb_xcb_ptr = m->mx_xcb;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;
	r->rcb_usertab_jnl = (journal) ? 1 : 0;

	status = dm2r_position(r, DM2R_QUAL, qual_list,
                     (i4)1,
                     (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;
	for (;;)
	{
	    /* Check for interrupts in this loop. */
	    if ( status = dm2u_check_for_interrupt(m->mx_xcb, dberr) )
		break;

	    /*
	    ** Note that we'll fetch all db_tab_base rows,
	    ** including partitions and indexes, just what we want. 
	    */
	    status = dm2r_get(r, &tid, DM2R_GETNEXT,
                         (char *)&rel, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(dberr);
		}
		break;
	    }

	    TMget_stamp((TM_STAMP *)&rel.relstamp12);

	    /*
	    ** Make sure the base table is marked as having an index.
	    */
	    if (m->mx_operation == MX_INDEX &&
		rel.reltid.db_tab_base ==
                              m->mx_table_id.db_tab_base &&
		rel.reltid.db_tab_index == 0 &&
		(rel.relstat & TCB_INDEX) == 0)
	    {
		/*
		** This is the base table of the index being created.
		*/

		/*
		** If not a concurrent index and base table does not
		** show it is indexed then update relstat. Concurrent
		** indecies are currently exposed via modify to noreadonly.
		*/
		if ( ((rel.relstat & TCB_IDXD) == 0)
				&&
		     (!m->mx_concurrent_idx) )
		{
		    rel.relstat |= TCB_IDXD;
		}

		/* Squirrel away the base table's journaling state */
		journaling |= rel.relstat & (TCB_JON | TCB_JOURNAL);
		base_table_seen = TRUE;	

		status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&rel, (char *)NULL, dberr);
	    }
	    else if ( rel.reltid.db_tab_base ==
                                  m->mx_table_id.db_tab_base
				&& 
		      (rel.reltid.db_tab_index ==
                                  m->mx_table_id.db_tab_index ||
		       (m->mx_operation == MX_MODIFY &&
		        m->mx_table_id.db_tab_index == 0 &&
			rel.reltid.db_tab_index & DB_TAB_PARTITION_MASK)) )
	    {
		/*
		** Either this is EXACTLY the iirelation being 
		** modified, or Master is being modified and
		** this is one of its partitions.
		*/

		/* Find the TPCB that matches this iirelation */
		for ( tp = m->mx_tpcb_next; 
		      tp && tp->tpcb_tabid.db_tab_index !=
			    rel.reltid.db_tab_index;
		      tp = tp->tpcb_next );

		if ( tp )
		{
		    mct = &tp->tpcb_mct;
		    new_locations = tp->tpcb_new_locations;
		    loc_count = tp->tpcb_loc_count;
		    loc = tp->tpcb_locations;
		}
		else if ( m->mx_operation == MX_MODIFY &&
			 m->mx_table_id.db_tab_index == 0 &&
			 rel.reltid.db_tab_index & DB_TAB_PARTITION_MASK )
		{
		    /*
		    ** TPCB not found for this partition.
		    ** This is likely due to a selective
		    ** modify (some, not all partitions) of
		    ** Master or a repartitioning modify.
		    ** Ignore those partitions for which
		    ** there is no TPCB.
		    */
		    continue;
		}
		else
		{
		    /*
		    ** No TPCB and it's either not a MODIFY
		    ** or it's a MODIFY of a non-partition
		    ** (like of Master, which won't have
		    ** a TPCB). Use the MCT of any
		    ** TPCB to extract the structure
		    ** information.
		    */
		    mct = &m->mx_tpcb_next->tpcb_mct;

		    if ( m->mx_Mlocations )
		    {
			new_locations = TRUE;
			loc_count = m->mx_Mloc_count;
			loc = m->mx_Mlocations;
		    }
		    else
			new_locations = FALSE;
		}

		if (m->mx_reorg == 0)
		{
		    rel.relmoddate = TMsecs();
		    rel.relspec = m->mx_structure;
		    rel.relpages = mct->mct_relpages;
		    rel.relprim = mct->mct_prim;
		    rel.relmain = mct->mct_main;
		    rel.relifill = mct->mct_i_fill;
		    rel.reldfill = mct->mct_d_fill;
		    rel.rellfill = mct->mct_l_fill;
		    rel.relmin = m->mx_minpages;
		    rel.relmax = m->mx_maxpages;
		    rel.relkeys = mct->mct_keys;

		    if ( tp )
		    {
			/* Partition's reltups and relallocation */
			rel.reltups = tp->tpcb_newtup_cnt;
			rel.relallocation = mct->mct_allocation;
		    }
		    else
		    {
			/* Table's reltups and master relallocation */
			rel.reltups = m->mx_newtup_cnt;
			rel.relallocation = m->mx_allocation;
		    }

		    rel.relextend = m->mx_extend;
		    rel.relfhdr = mct->mct_fhdr_pageno;
		    rel.relpgsize = m->mx_page_size;
		    rel.relpgtype = m->mx_page_type;
		    rel.relstat &=
			~(TCB_COMPRESSED | TCB_UNIQUE |
			    TCB_INDEX_COMP | TCB_IDXD | 
			    TCB_DUPLICATES | TCB_CLUSTERED);

		    if ( rel.reltid.db_tab_base == m->mx_table_id.db_tab_base &&
			    rel.reltid.db_tab_index == m->mx_table_id.db_tab_index &&
			    rel.reltid.db_tab_index > 0 &&  
			    m->mx_operation == MX_INDEX ) 
		    {
			/* 
			** This is the secondary index being created. If we've seen the base 
			** table for the index already in this loop, then we have its 
			** journaling status, so use it. If not, then store away the tid
			** of the index so that we can get the index back at the end of the 
			** loop, by which time we will have seen the base table.
			*/
			if (base_table_seen)
			{
			    rel.relstat &= ~(TCB_JON | TCB_JOURNAL); 	/* clear...   */
			    rel.relstat |= journaling; 			/* ...and set */
			}
			else
			{
			    index_tid = tid;
			}
		    }	

		    rel.relcmptlvl = DMF_T_VERSION;
		    /*
		    ** Use compression type as used during modify.
		    */
		    if (m->mx_data_rac.compression_type != TCB_C_NONE)
			rel.relstat |= TCB_COMPRESSED;
		    rel.relcomptype = m->mx_data_rac.compression_type;

		    if (m->mx_index_comp)
			rel.relstat |= TCB_INDEX_COMP;
		    if (m->mx_unique)
			rel.relstat |= TCB_UNIQUE;
		    if (m->mx_duplicates)
			rel.relstat |= TCB_DUPLICATES;

		    if ( m->mx_clustered )
			rel.relstat |= (TCB_CLUSTERED | TCB_UNIQUE);

		    /* Check the altered bit before it gets cleared */
		    if ( rel.relstat2 & TCB2_ALTERED && 
			 rel.relstat  & TCB_COMMENT  &&
		         (status = dm2u_fix_comment(m, 
				     attr_map, new_map, col_cnt, 
				     &comment, dberr)) )
		    {
			break;
		    }

		    /*
		    ** use the relstat2 bits passed in from our caller
		    ** EXCEPT for the blob support bits.  presumably,
		    ** the user can't turn off blob support.
		    ** This will clear TCB2_ALTERED bit as well.
		    **
		    ** Watch for other critical relstat2 bits, too!
		    */
		    rel.relstat2 &=
			(TCB2_EXTENSION | TCB2_HAS_EXTENSIONS |
			 TCB2_PARTITION | TCB2_GLOBAL_INDEX);

		    rel.relstat2 |= m->mx_new_relstat2;
		    rel.relversion = 0;
		    rel.reltotwid = m->mx_width;
		    rel.reltotdatawid = m->mx_datawidth;
		    rel.relatts = col_cnt - drop_cnt;

		    /*
		    ** Partitions don't "have" attributes or
		    ** keys, but get that info from Master
		    ** when the partition TCB is built.
		    */
		    if ( rel.relstat2 & TCB2_PARTITION )
		    {
			rel.relatts = 0;
			rel.relkeys = 0;
		    }

		    /*
		    ** If repartitioning, update Master's
		    ** number-of-partitions and levels.
		    */
		    if ( m->mx_part_def && rel.reltid.db_tab_index == 0 )
		    {
			if ( m->mx_part_def->ndims )
			{
			    rel.relstat |= TCB_IS_PARTITIONED;
			    rel.relnparts = m->mx_part_def->nphys_parts;
			    rel.relnpartlevels = m->mx_part_def->ndims;
			}
			else
			{
			    /* This is an un-partition */
			    rel.relstat &= ~TCB_IS_PARTITIONED;
			    rel.relnparts = 0;
			    rel.relnpartlevels = 0;
			}
		    }
		}
		else
		    rel.relmoddate = TMsecs();

		if ( new_locations )
		{
		    STRUCT_ASSIGN_MACRO(loc[0].loc_name, rel.relloc);
		    rel.relloccount = loc_count;
		    if (loc_count > 1)
			rel.relstat |= TCB_MULTIPLE_LOC;
		    else
			rel.relstat &= ~TCB_MULTIPLE_LOC;
		}

		/* Update cache priority to new or old value */
		rel.reltcpri = m->mx_tbl_pri;


		status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&rel, (char *)NULL, dberr);

		/*
		** Record whether this table has an underlying physical file.
		** This will be used below to determine whether to do file
		** renames as part of the INDEX/MODIFY operation.
		*/
		nofiles = (rel.relstat & TCB_GATEWAY);
	    }
	    else if ((rel.reltid.db_tab_base ==
					m->mx_table_id.db_tab_base) &&
		      (rel.relstat & TCB_INDEX) == 0)
	    {
		/*
		** This is a base table of the table being updated.
		** Change its timestamp to invalidate old query plans.
		*/
		status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&rel, (char *)NULL, dberr);
	    }
	    if (status != E_DB_OK)
		break;
	}

	for (;;) /* something to break out of */
	{
	    if (status != E_DB_OK)
		break;
	    
	    if( base_table_seen && index_tid.tid_i4 != 0 )
	    {
		status = dm2r_get(r, &index_tid, DM2R_BYTID, (char *)&rel, dberr);
		if (status != E_DB_OK)
		    break;

		rel.relstat &= ~(TCB_JON | TCB_JOURNAL); 	/* clear...   */
		rel.relstat |= journaling; 			/* ...and set */

		status = dm2r_replace(r, &index_tid, DM2R_BYTID, (char *)&rel, (char *)0, dberr);
	    }
	    break;
	}

	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &local_error, 0);
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
		uleFormat(NULL, E_DM9026_REL_UPDATE_ERR, NULL, ULE_LOG, NULL,
		    (char *) NULL, 0L, (i4 *)NULL, &local_error, 0);
	    }
	    break;
	}

    	status = dm2t_close(r, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;
	
	/*
	** For each file at an old location, rename for deletion.
	** For each new file at the new locations, rename to its permanent name.
	** Check for case of table that has no underlying files.
        */
	dm0l_flag = (journal ? DM0L_JOURNAL : 0);

	if ( !nofiles )
	{
	    DM_FILE       mod_filename;
	    DM_FILE       new_filename;

	    /* First, do the Source file renames */
	    for ( sp = m->mx_spcb_next; status == E_DB_OK && sp; sp = sp->spcb_next )
	    {
		for ( k = 0; status == E_DB_OK && k < sp->spcb_loc_count; k++ )
		{
		    /* Check for interrupts in this loop. */
		    if ( status = dm2u_check_for_interrupt(m->mx_xcb, dberr) )
			break;

		    /*
		    ** Log each individual rename with dm0l_frename,
		    ** CLR's make each idempotent.
		    */
		    if ( (sp->spcb_locations[k].loc_status & LOC_RAW) == 0 )
			status = RenameForDeletion(m, dm0l_flag,
						   &sp->spcb_tabid,
						   sp->spcb_locations,
						   k,
						   dberr);
		}
	    }

	    /* Next do the Target file renames */
	    for ( tp = m->mx_tpcb_next; status == E_DB_OK && tp; tp = tp->tpcb_next )
	    {
		for ( k = 0; status == E_DB_OK && k < tp->tpcb_loc_count; k++ )
		{
		    /* Check for interrupts in this loop. */
		    if ( status = dm2u_check_for_interrupt(m->mx_xcb, dberr) )
			break;

		    if ( (tp->tpcb_locations[k].loc_status & LOC_RAW) == 0 )
		    {
			/*
			** If create INDEX or new partition, rename and
			** delete the unused and pesky "old" .t00 target file
			** to avoid file versioning problems on VMS.
			*/
			if ( m->mx_operation == MX_INDEX ||
			    (phypart && 
			     phypart[tp->tpcb_partno].flags 
				 & DMU_PPCF_NEW_TABID) )
			    status = RenameForDeletion(m, dm0l_flag,
						       &tp->tpcb_tabid,
						       tp->tpcb_locations,
						       k,
						       dberr);
			if ( status == E_DB_OK )
			{
			    /*
			    ** Build filenames
			    **    mod_filename - filename of modify file.
			    **		     This file is renamed to 
			    **		     become the user table.
			    **    new_filename - new user table filename.
			    */
			    STRUCT_ASSIGN_MACRO(tp->tpcb_locations[k].loc_fcb->fcb_filename,
				mod_filename);

			    dm2f_filename(DM2F_TAB, &tp->tpcb_tabid,
				tp->tpcb_locations[k].loc_id, 0, 0, &new_filename);
			    /*
			    ** Rename modify file to the user table file.
			    */

			    if ((m->mx_xcb->xcb_flags & XCB_NOLOGGING) == 0)
			    {
				status = dm0l_frename(m->mx_log_id, dm0l_flag,
				    &tp->tpcb_locations[k].loc_ext->physical,
				    tp->tpcb_locations[k].loc_ext->phys_length,
				    &mod_filename, &new_filename,
				    &tp->tpcb_tabid, k,
				    tp->tpcb_locations[k].loc_config_id,
				    (LG_LSN *)0, &lsn, dberr);
			    }

			    if ( status == E_DB_OK )
			    {
				status = dm2f_rename(m->mx_lk_id, m->mx_log_id,
				    &tp->tpcb_locations[k].loc_ext->physical,
				    tp->tpcb_locations[k].loc_ext->phys_length,
				    &mod_filename, (i4)sizeof(mod_filename),
				    &new_filename, (i4)sizeof(new_filename),
				    &m->mx_dcb->dcb_name, dberr);
			    }
			}
		    }
		}
	    }
	}

	if (status != E_DB_OK)
	    break;
	if (DMZ_SRT_MACRO(11))
	    TRdisplay("%@ done with dm2u_update_catalogs\n");
	return (E_DB_OK);
    }

    /*	Handle cleanup for error recovery. */

    if (r)
	status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: RenameForDeletion - Rename some file to ".d00"
**			     and pend it for deletion.
**
** Description:
**	Used by both MODIFY and CREATE INDEX to deal with
**	renaming and pending files for delete.
**
** Inputs:
**      m                               The modify index control block.
**	dm0l_flag			Indicates whether user table is
**					journaled.
**	tabid				The table id.
**	locArray			Some location array
**	k				Which entry in locArray
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
**	18-Apr-2005 (jenjo02)
**	    Extracted from dm2u_update_catalogs() so functionality
**	    can be used for both MODIFY sources and INDEX
**	    fake ".t00" outputs (which must be renamed and deleted
**	    to avoid VMS file versioning problems).
**	23-Mar-2010 (kschendel) SIR 123448
**	    XCB's XCCB list now has to be mutex protected.
*/
static DB_STATUS
RenameForDeletion(
DM2U_MXCB	*m,
i4		dm0l_flag,
DB_TAB_ID	*tabid,
DMP_LOCATION	*locArray,
i4		k,
DB_ERROR	*dberr)
{
    DM_FILE		old_filename;
    DM_FILE		del_filename;
    DML_XCB		*xcb;
    DML_XCCB		*xccb;
    LG_LSN		lsn;
    DB_STATUS		status = E_DB_OK;
    i4			error;
    
    /*
    ** Build filenames
    **    old_filename - table filename.
    **    del_filename - filename for delete file.
    **		     Old file is renamed to
    **		     this file then deleted.
    */
    dm2f_filename(DM2F_TAB, tabid,
	locArray[k].loc_id, 0, 0, &old_filename);
    dm2f_filename(DM2F_DES, tabid,
	locArray[k].loc_id, 0, 0, &del_filename);
    /*
    ** Rename file to the delete file.
    */
    xcb = m->mx_xcb;
    if ( (xcb->xcb_flags & XCB_NOLOGGING) == 0 )
    {
	status = dm0l_frename(m->mx_log_id, dm0l_flag,
	    &locArray[k].loc_ext->physical,
	    locArray[k].loc_ext->phys_length,
	    &old_filename, &del_filename,
	    tabid, k,
	    locArray[k].loc_config_id,
	    (LG_LSN *)0, &lsn, dberr);
    }

    if ( status == E_DB_OK )
    {
	status = dm2f_rename(m->mx_lk_id, m->mx_log_id,
	    &locArray[k].loc_ext->physical,
	    locArray[k].loc_ext->phys_length,
	    &old_filename, (i4)sizeof(old_filename),
	    &del_filename, (i4)sizeof(del_filename),
	    &m->mx_dcb->dcb_name, dberr);

	if ( status == E_DB_OK )
	{
	    /*
	    ** Now pend the delete files for delete.
	    */
	    status = dm0m_allocate((i4)sizeof(DML_XCCB),
		DM0M_ZERO | DM0M_LONGTERM, (i4)XCCB_CB,
		(i4)XCCB_ASCII_ID, (char *)xcb,
		(DM_OBJECT **)&xccb, dberr);

	    if ( status == E_DB_OK )
	    {
		/* XCB should be the session thread's XCB even if worker
		** threads are around.  We're probably in the session thread,
		** but mutex anyway.
		*/
		xccb->xccb_f_len = sizeof(del_filename);
		xccb->xccb_p_len = locArray[k].loc_ext->phys_length;
		STRUCT_ASSIGN_MACRO(del_filename, xccb->xccb_f_name);
		STRUCT_ASSIGN_MACRO(locArray[k].loc_ext->physical,
					xccb->xccb_p_name);
		dm0s_mlock(&xcb->xcb_cq_mutex);
		xccb->xccb_q_next = xcb->xcb_cq_next;
		xccb->xccb_q_prev = (DML_XCCB*) &xcb->xcb_cq_next;
		xcb->xcb_cq_next->xccb_q_prev = xccb;
		xcb->xcb_cq_next = xccb;
		xccb->xccb_xcb_ptr = xcb;
		xccb->xccb_sp_id = xcb->xcb_sp_id;
		xccb->xccb_operation = XCCB_F_DELETE;
		dm0s_munlock(&xcb->xcb_cq_mutex);
	    }
	}
    }

    return(status);
}

/*{
** Name: dm2u_get_tabid	- Create unique table id.
**
** Description:
**      This routine is called from dm2u_create() to get a unique table id, or
**	it can be called directly from dmu_get_tabid().
**
** Inputs:
**	dcb				The database id.
**	xcb				The transaction id.
**	base_tbl_id			The internal table id of the base table
**					 (used if createing an index table id)
**	index				TRUE if create an index table.
**	numids				Number of ID's to get, usually 1
** Outputs:
**	tbl_id				The (first) internal table id assigned
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
**      27-jul-88 (teg)
**          Created from dmu_getid().
**	30-Aug-88 (teg)
**	    fixed bug found during stress test that causes access violation
**	17-may-90 (bryanp)
**	    After we make a significant change to the config file (increment
**	    the table ID or modify a core system catalog), make an up-to-date
**	    backup copy of the config file for use in disaster recovery.
**	11-Feb-2004 (schka24)
**	    Allow getting of more than 1 ID at a time for partitions.
*/
DB_STATUS
dm2u_get_tabid(
DMP_DCB		*dcb,		/* database control block */
DML_XCB		*xcb,		/* transaction control block, for locking */
DB_TAB_ID	*base_tbl_id,	/* contains base table id for index table id */
bool	        index,		/* true => create index table id */
i4		numids,		/* Number of ID's to get */
DB_TAB_ID	*new_tab_id,	/* output, unique new table id */
DB_ERROR	*dberr)		/* output, error code */
{
    DM0C_CNF	*config;
    DB_STATUS	status;
    i4	    	id;
    i4          local_err_code;
    SCF_CB      scf_cb;
    char        buf [DB_ERR_SIZE];
    i4          len;
    i4		error;
    DB_ERROR	local_dberr;

    status = dm0c_open(dcb, DM0C_PARTIAL, xcb->xcb_lk_id, &config, dberr);

    if ( status == E_DB_OK )
    {
	/*	Compute next table id. */

	/* Check that "numids" doesn't blow the DB_TAB_ID_MAX */
	if (config->cnf_dsc->dsc_last_table_id + numids > DB_TAB_ID_MAX)
	{
	    (VOID) dm0c_close(config, 0, &local_dberr);
	    SETDBERR(dberr, 0, E_DM0021_TABLES_TOO_MANY);
	    status = E_DB_ERROR;
	}
	else
	{
            i4 ids_left;

	    id = config->cnf_dsc->dsc_last_table_id + 1;
	    config->cnf_dsc->dsc_last_table_id += numids;
	    if (index)
	    {
		new_tab_id->db_tab_base = base_tbl_id->db_tab_base;
		new_tab_id->db_tab_index = id;
	    }
	    else
	    {
		new_tab_id->db_tab_base  = id;
		new_tab_id->db_tab_index = 0;
	    }
	    /* If partition, caller will do it */

            ids_left = DB_TAB_ID_MAX - config->cnf_dsc->dsc_last_table_id;

            if (ids_left <= dm2uu_tab_id_proximity)
            {
               /* Output a warning at an appropriate rate. */

               if ( ( ids_left <= dm2uu_tab_id_warn_now ) || 
                    (  (ids_left % dm2uu_tab_id_warn_freq) == 0) )
               {
                  ule_format(E_DM0026_TABLE_ID_WARNING, (CL_SYS_ERR *)0, ULE_LOG, NULL,
                       buf, sizeof(buf), &len, &local_err_code, 3,
                       sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
                       0, config->cnf_dsc->dsc_last_table_id,
                       0, DB_TAB_ID_MAX);

                  scf_cb.scf_length = sizeof(scf_cb);
                  scf_cb.scf_type = SCF_CB_TYPE;
                  scf_cb.scf_facility = DB_DMF_ID; 
                  scf_cb.scf_len_union.scf_blength = len;
                  scf_cb.scf_ptr_union.scf_buffer = buf;
                  scf_cb.scf_session = DB_NOSESSION;

                  (void) scf_call( SCC_WARNING, &scf_cb );
               }
            }

	    /*	Close and update configuration file, making backup copy. */

	    status = dm0c_close(config, DM0C_UPDATE | DM0C_COPY, dberr);
	}
    }

    return(status);
}

/*{
** Name: dm2u_ckp_lock	- Get ckp lock before continues
**
** Description:
**      This routine is called from dm2u_xxxxx() to get a ckp lock. Xact
**	will be blocked if online backup is processing.
**
** Inputs:
**	dcb				The database id.
**	tab_name			The table name.
**	owner_name			The owner of table.
**	xcb				The transaction id.
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
**      28-feb-89 (EdHsu)
**          Created for Terminator.
**	23-oct-89 (rogerk)
**	    Added database name/owner to lock key to match lock taken by
**	    checkpoint process.
**      17-jul-90 (mikem)
**          bug #30833 - lost force aborts
**          LKrequest has been changed to return a new error status
**          (LK_INTR_GRANT) when a lock has been granted at the same time as
**          an interrupt arrives (the interrupt that caused the problem was
**          a FORCE ABORT).   Callers of LKrequest which can be interrupted
**          must deal with this new error return and take the appropriate
**          action.  Appropriate action includes signaling the interrupt
**          up to SCF, and possibly releasing the lock that was granted.
**	    In the case of dm2u_ckp_lock() just handle the new return the same
**	    as LK_INTERRUPT (the lock gets released as part LK release all).
**	09-apr-95 (medji01)
**	    Bug #64699
**		Enhanced dm2u_ckp_lock() to note any current session level
**		lockmode timeout and apply this to the lock request.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Session level timeout and max_locks values are now stored 
**          in the scb. 
**	27-may-1999 (nanpr01)
**	    Implementation of SIR 6189325. Print out lock timeout messages
**	    in errlog.log.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
*/
DB_STATUS
dm2u_ckp_lock(
DMP_DCB	    *dcb,
DB_TAB_NAME *tab_name,
DB_OWN_NAME *owner_name,
DML_XCB	    *xcb,
DB_ERROR    *dberr)
{
    LK_LOCK_KEY     lockkey;
    LK_LKID         lockid;
    CL_ERR_DESC	    sys_err;
    DB_STATUS	    status;
    DML_SCB         *scb;
    i4         timeout;
    i4         max_locks;
    char            msgbuf[64];
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    **	Request special LK_CKP_DB type for online backup purposes.
    **	In order to prevent dm2u operations to be single threaded,
    **	we use the LK_IX mode, such that all dm2u operations will
    **	be compatible with each other. However, the lock mode that
    **	the online backup process is using is the LK_SIX, such that
    **	the dm2u and online backup operations are not compatible
    **	with each other. Also, online backup uses the physical
    **	lock, and the dm2u operations use logical locks.
    */

    lockkey.lk_type = LK_CKP_DB;
    lockkey.lk_key1 = dcb->dcb_id;
    MEcopy((PTR)&dcb->dcb_db_owner, 8, (PTR)&lockkey.lk_key2);
    MEcopy((PTR)&dcb->dcb_name, 12, (PTR)&lockkey.lk_key4);
    MEfill(sizeof(LK_LKID), 0, &lockid);

    timeout = 0;
    max_locks = 20;
    scb = xcb->xcb_scb_ptr;

    if (scb->scb_timeout != DMC_C_SYSTEM)
        timeout = scb->scb_timeout;
    if (scb->scb_maxlocks != DMC_C_SYSTEM)
        max_locks = scb->scb_maxlocks;

    status = LKrequest(0, xcb->xcb_lk_id, &lockkey, LK_IX,
	(LK_VALUE  *) 0, &lockid, timeout, &sys_err);

    if (status == E_DB_OK)
	return (E_DB_OK);

    if (status == LK_NOLOCKS)
	SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
    else if (status == LK_TIMEOUT)
    {
          uleFormat(NULL, E_DM9071_LOCK_TIMEOUT_ONLINE_CKP, &sys_err, ULE_LOG, NULL,
              (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
              sizeof(DB_TAB_NAME), 
	      ((tab_name == NULL)? "?" : tab_name->db_tab_name),
              sizeof(DB_DB_NAME), &dcb->dcb_name, 0, LK_IX,
              sizeof(DB_OWN_NAME), 
              ((owner_name == NULL) ? "?" : owner_name->db_own_name), 
	      0, sys_err.moreinfo[0].data.string);
	SETDBERR(dberr, 0, E_DM004D_LOCK_TIMER_EXPIRED);
    }
    else if ((status == LK_INTERRUPT) || (status == LK_INTR_GRANT))
	SETDBERR(dberr, 0, E_DM0065_USER_INTR);
    else if (status == LK_INTR_FA)
        SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
    else
	SETDBERR(dberr, 0, E_DM926D_TBL_LOCK);

    return (status);
}


/* the following are here for code path test purposes.  For normal operations,
**  neither TEST1A nor TEST1B should be defined.
*/

#ifdef TEST1A /* set to true when run DMTST1 - test path from ESQL to
              ** dm1u_table, but do not test dm1u_table operations -- ie use
              ** dummy routine -- IF THIS IS SET, DUMMY ROUTINES ALWAYS
              ** RETURN SUCCESSFULLY.  NEVER DEFINE BOTH TEST1A AND TEST1B
              ** SIMULTANEOUSLY, OR YOU WILL GET COMPILE ERRORS.
              */
        /* DUMMY ROUTINE - returns success status. */
        DB_STATUS
        dm1u_table(r, mode, , error)
            DMP_RCB             *r;
            i4             mode;
            i4             *error;
        {
            return (E_DB_OK);
        };


#endif

#ifdef TEST1B /* set to true when run DMTST1 - test path from ESQL to
              ** dm1u_table, but do not test dm1u_table operations -- ie use
              ** dummy routine -- IF THIS IS SET, DUMMY ROUTINES ALWAYS
              ** RETURNS WITH A FAILURE STATUS.  NEVER DEFINE BOTH TEST1A AND
              ** TEST1B SIMULTANEOUSLY, OR YOU WILL GET COMPILE ERRORS.
              */
        /* DUMMY ROUTINE -- returns ERROR status. */
        DB_STATUS
        dm1u_table (r, mode, error)
            DMP_RCB             *r;
            i4             mode;
            i4             *error;
        {
            *error = 12345;
            return (E_DB_ERROR);
        };

#endif

/*{
** Name: dm2u_readonly_upd_cats - Set/unset the readonly bit for a table.
**
** Description:
**
** Inputs:
**      mxcb                            The modify index control block.
**	journal				Indicates whether user table is
**					journaled.
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
**	15-may-1993 (rmuth)
**	    Created.
**	23-aug-1993 (rogerk)
**	    Changes to handling of non-journaled tables.  ALL system catalog
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	    Documented 'journal' parameter to mean journal status of user
**	    table rather than whether or not updates should be journaled.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 to indicate an index, not simply != 0.
**	28-Feb-2004 (jenjo02)
**	    Modified to deal with multiple partitions.
*/
DB_STATUS
dm2u_readonly_upd_cats(
DM2U_MXCB           *mxcb,
i4		    journal,
DB_ERROR            *dberr)
{
    DM2U_MXCB		*m = mxcb;
    DMP_RCB		*r = (DMP_RCB *)0;
    DMP_RCB		*rcb;
    DB_TAB_ID		table_id;
    DM_TID		tid;
    DB_STATUS		status;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	qual_list[1];
    i4			concurrent_idx_cnt = 0;
    DMP_RELATION	rel;
    i4			error;
    DB_ERROR		local_dberr;

    do
    {
	/*
	** Build a qualification list used for searching the system tables.
	*/
	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *)&m->mx_table_id.db_tab_base;

	/*
	** Update the relstat bits for the base table and any
	** secondary indicies that were built on the table whilst
	** table was in index_build mode
	*/
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;

	status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0,
		m->mx_log_id,m->mx_lk_id,
		(i4)0, (i4)0, DM2T_S, &m->mx_xcb->xcb_tran_id,
                &timestamp, &rcb, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;

	r = rcb;
	r->rcb_xcb_ptr = m->mx_xcb;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;
	r->rcb_usertab_jnl = (journal) ? 1 : 0;


	/*
	** First pass if iirelation look for any concurrent secondary
	** indicies and make them visible. Only do this if we are
	** modifying to NOREADONLY
	*/
	if ( m->mx_readonly == MXCB_UNSET_READONLY )
	{
	    status = dm2r_position(r, DM2R_QUAL, qual_list,
			 (i4)1,
			 (DM_TID *)0, dberr);
	    if (status != E_DB_OK)
		break;

	    for (;;)
	    {
		status = dm2r_get(r, &tid, DM2R_GETNEXT,
			     (char *)&rel, dberr);
		if (status != E_DB_OK)
		{
		    if (dberr->err_code == E_DM0055_NONEXT)
		    {
			CLRDBERR(dberr);
			status = E_DB_OK;
		    }
		    break;
		}

		/*
		** Skip all but indexes
		*/
		if (( rel.reltid.db_tab_base ==
				    m->mx_table_id.db_tab_base)
				    &&
		    (rel.relstat & TCB_INDEX) == 0)
		{
		    continue;
		}

		if ( rel.relstat2 & TCB2_CONCURRENT_ACCESS )
		{
		    TMget_stamp((TM_STAMP *)&rel.relstamp12);
		    concurrent_idx_cnt++;

		    rel.relstat2 &= ~TCB2_CONCURRENT_ACCESS;

		    status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
				 (char *)&rel, (char *)NULL, dberr);

		    if ( status != E_DB_OK )
			break;
		}

	    }

	    /*
	    ** If error updating the secondary indicies then bail
	    */
	    if ( status != E_DB_OK )
		break;
        }

	/*
	** Update the base table and partitions accordingly
	*/
	status = dm2r_position(r, DM2R_QUAL, qual_list,
                     (i4)1,
                     (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(r, &tid, DM2R_GETNEXT,
                         (char *)&rel, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    CLRDBERR(dberr);
		    status = E_DB_OK;
		}
		break;
	    }

	    /*
	    ** Skip the index tables
	    */
	    if (( rel.reltid.db_tab_base == m->mx_table_id.db_tab_base)
				&&
		  rel.relstat & TCB_INDEX)
	    {
		continue;
	    }

	    TMget_stamp((TM_STAMP *)&rel.relstamp12);

	    /*
	    ** Check this is the correct base table
	    ** or partition.
	    */
	    if ( rel.reltid.db_tab_base !=
				m->mx_table_id.db_tab_base )

	    {
		status = E_DB_ERROR;
		break;
	    }

	    /* This applies only to the base table, not partitions */
	    if ( rel.reltid.db_tab_index == 0 && concurrent_idx_cnt )
	    {
		rel.relidxcount += concurrent_idx_cnt;
		rel.relstat |= TCB_IDXD;
	    }

	    /*
	    ** Set or unset the READONLY bit
	    */
	    if ( m->mx_readonly == MXCB_SET_READONLY )
		rel.relstat2 |= TCB2_READONLY;
	    else
		rel.relstat2 &= ~TCB2_READONLY;


	    status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&rel, (char *)NULL, dberr);

	    if ( status != E_DB_OK )
	        break;
	}

	/*
	** If error updating the base table then bail
	*/
        if ( status != E_DB_OK )
	    break;

    	status = dm2t_close(r, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;


    } while (FALSE);

    /*	Handle cleanup for error recovery. */

    if (r)
    {
	DB_STATUS local_status;

	local_status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);

	if ( local_status != E_DB_OK )
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    if ( status && (dberr->err_code == 0 || dberr->err_code > E_DM_INTERNAL) )
    {
	if (dberr->err_code )
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &error, 0);

	uleFormat( NULL, E_DM930D_DM2U_READONLY_UPD_CATS, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL,
	            (i4)0, (i4 *)NULL, &error, 3,
		    sizeof( m->mx_dcb->dcb_name),
		    &m->mx_dcb->dcb_name,
		    sizeof( m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid),
		    &m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid,
		    sizeof( m->mx_rcb->rcb_tcb_ptr->tcb_rel.relowner),
		    &m->mx_rcb->rcb_tcb_ptr->tcb_rel.relowner);

	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
    }


    return (status);
}

/*{
** Name: dm2u_calc_hash_buckets - Calculate the number of hash buckets.
**
** Description:
**	Using the input parameters derive the number of hash buckets
**	needed for the table.
**
**	The number of HASH buckets is calculated as follows
**
**	Calcuate the number of tuples per page given the fill factor,
**	then calculate the number of pages needed, then adjust so
**	that it is within minpages .. maxpages.  Then finally,
**	find the closest power of 2 and use this number if it is
**	within the minpage..maxpage window.  Powers of 2 are the best
**	choice for the hash function, any other number whether
**	prime of not doesn't make much of a difference.
**
** Inputs:
**	page_size	- page size.
**	fillfactor	- For the bucket pages
**	width		- row width
**	row_count	- estimated number of rows in the table
**	min_pages	- min_pages specified.
**	max_pages	- max_pages specified.
**
** Outputs:
**	Returns:
**	    The number of HASH buckets.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jul-1993 (rmuth)
**	    Created.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page size argument to replace hard-coded DB_MAXTUP.
**	06-may-1996 (nanpr01)
**	    Use sizeof function instead of the constant.
**	12-jun-1996 (thaju02)
**	    Modified calculation of tup_per_page from
**	    tup_per_page = (page_size * fillfactor / 100 )/
**		(width + DM_SIZEOF_LINEID_MACRO(page_size));
**	    to
**	    tup_per_page = ((page_size - page_overhead) * fillfactor / 100)/
**		(width + DM_SIZEOF_LINEID_MACRO(page_size) + 
**		DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(relpgtype));
**	    to account for tuple header and page header overhead.
*/
i4
dm2u_calc_hash_buckets(
i4		page_type,
i4		page_size,
i4		fillfactor,
i4		width,
i4		row_count,
i4		min_pages,
i4		max_pages)
{
    i4	buckets;
    i4	tup_per_page;
    i4	power_two;
    i4	pg_overhead;

    pg_overhead = DMPPN_VPT_OVERHEAD_MACRO(page_type);

    tup_per_page = ((page_size - pg_overhead) * fillfactor / 100 )/ 
			(width + DM_VPT_SIZEOF_LINEID_MACRO(page_type) +
			DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type));

    if (tup_per_page == 0)
        ++tup_per_page;

    buckets = row_count / tup_per_page;

    if ( min_pages > max_pages )
	min_pages = max_pages;

    if (buckets > max_pages)
	buckets = max_pages;

    if (buckets < min_pages)
	buckets = min_pages;

    /*  Compute power of two less then buckets. */

    for ( power_two = 16; power_two + power_two < buckets;)
	  power_two += power_two;

    /*  Use power of two which is closer and within the window. */

    if (power_two < buckets)
    {
	if (buckets - power_two > power_two + power_two - buckets)
	    power_two += power_two;
    }

    if (power_two >= min_pages && power_two <= max_pages)
	buckets = power_two;

    return( buckets );

}

/*{
** Name: dm2u_file_create - Log and Create files as part of DMU operation.
**
** Description:
**	This routine creates the individual files which make up a (possibly)
**	multi-location table.
**
**	***  The routine returns with the file left open. ***
**
**	Each file create is logged appropriately.
**	The created file(s) contain no pages.
**
**	The caller must have filled in the location list and called
**	dm2f_build_fcb to fill in the FCB fields.
**
**	This routine is used by CREATE, MODIFY, LOAD, INDEX and RELOCATE
**	to create new table files.
**
** Inputs:
**	dcb		- Database control block.
**	log_id		- Handle to logging system for writing log records.
**	lock_id		- Handle to locking system for lock events.
**	tbl_id		- Table file belongs to.
**	db_sync_flag	- specifies file sync requirements.
**			      DCB_NOSYNC_MASK    - no DI sync
**			      DCB_USER_SYNC_MASK - DI sync when requested.
**			      0 - sync DI
**	loc_array	- Array of location descriptors.
**	loc_count	- Number of entries in above location array.
**	page_size	- page size for this table.
**	logging		- whether log records should be written (TRUE/FALSE).
**	journal		- whether log records should be journaled (TRUE/FALSE).
**
** Outputs:
**	err_code	- error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	The created file is left open on successful return.
**
** History:
**	23-aug-1993 (rogerk)
**	    Created.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      27-dec-94 (cohmi01)
**          For RAW I/O, changes to pass more info down to dm2f_xxx functions.
**          If tbio ptr in FCB is null, set up a local DMP_TABLE_IO with the
**          tid from mx, point FCB to it.
**	06-mar-1996 (stial01 for bryanp)
**	    Add new page_size argument for variable page size support.
**	09-Apr-1999 (jenjo02)
**	    Remove local DMP_TABLE_IO (tio) for RAW files, no longer needed.
*/
DB_STATUS
dm2u_file_create(
DMP_DCB		*dcb,
i4		log_id,
i4		lock_id,
DB_TAB_ID	*tbl_id,
u_i4	db_sync_flag,
DMP_LOCATION	*loc_array,
i4		loc_count,
i4		page_size,
i4		logging,
i4		journal,
DB_ERROR	*dberr)
{
    DB_STATUS	    status = E_DB_OK;
    DMP_LOCATION    *loc_entry;
    LG_LSN	    lsn;
    i4	    i;
    i4	    dm0l_flag;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (i = 0; i < loc_count; i++)
    {
	loc_entry = &loc_array[i];

	/*
	** Log the new file creation.
	*/
	if (logging)
	{
	    dm0l_flag = (journal ? DM0L_JOURNAL : 0);

	    status = dm0l_fcreate(log_id, dm0l_flag,
			    &loc_entry->loc_ext->physical,
			    loc_entry->loc_ext->phys_length,
			    &loc_entry->loc_fcb->fcb_filename,
			    tbl_id, i, loc_entry->loc_config_id,
			    page_size, loc_entry->loc_status,
			    (LG_LSN *)0, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Create the file.
	*/
	status = dm2f_create_file(lock_id, log_id, loc_entry, 1, page_size,
	    db_sync_flag, (DM_OBJECT *)dcb, dberr);

	if (status != E_DB_OK)
	    break;
    }

    if (status != E_DB_OK)
    {
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9441_DM2U_FILE_CREATE);
	}
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}
/*{
** Name: dm2u_alterstatus_upd_cats - Set/unset the alterstatus bit 
**				     and/or Table priority for a table.
**
** Description:
**
** Inputs:
**      mxcb                            The modify index control block.
**	journal				Indicates whether user table is
**					journaled.
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
**	09-jan-1995 (nanpr01)
**	    Created.
**	21-jan-1997 (nanpr01)
**	    Bug 80163 : Allow setting log_consistent etc for secondary indexes.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Update table priority in iirelation when asked to
**	    MODIFY ... TO PRIORITY = <n>
**	9-apr-98 (inkdo01)
**	    Support "modify ... to persistence/unique_scope=statement" for
**	    constraint index with options.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 to indicate an index, not simply != 0.
**	28-Feb-2004 (jenjo02)
**	    Modified to deal with multiple partitions.
**	26-sep-2007 (kibro01) b119080
**	    Allow modify to nopersistence
*/
DB_STATUS
dm2u_alterstatus_upd_cats(
DM2U_MXCB           *mxcb,
i4		    journal,
DB_ERROR            *dberr)
{
    DM2U_MXCB		*m = mxcb;
    DMP_RCB		*r = (DMP_RCB *)0;
    DMP_TCB		*t;
    DB_TAB_ID		table_id;
    DM_TID		tid;
    DB_STATUS		status;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION	rel;
    i4			error;
    DB_ERROR		local_dberr;

    do
    {
	/*
	** Build a qualification list used for searching the system tables.
	*/
	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *)&m->mx_table_id.db_tab_base;

	/*
	** Update the relstat bits for the base table and any
	** secondary indicies that were built on the table whilst
	** table was in index_build mode
	*/
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;

	status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0,
		m->mx_log_id,m->mx_lk_id,
		(i4)0, (i4)0, DM2T_S, &m->mx_xcb->xcb_tran_id,
                &timestamp, &r, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;

	r->rcb_xcb_ptr = m->mx_xcb;

	t = m->mx_rcb->rcb_tcb_ptr;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;
	r->rcb_usertab_jnl = (journal) ? 1 : 0;

	/*
	** Update the base table accordingly
	*/
	status = dm2r_position(r, DM2R_QUAL, qual_list,
                     (i4)1,
                     (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(r, &tid, DM2R_GETNEXT,
                         (char *)&rel, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    CLRDBERR(dberr);
		    status = E_DB_OK;
		}
		break;
	    }

	    TMget_stamp((TM_STAMP *)&rel.relstamp12);

	    /*
	    ** Check this is the correct base table
	    */
	    if ( (rel.reltid.db_tab_base !=
				m->mx_table_id.db_tab_base)
				||
		(rel.reltid.db_tab_index !=
				m->mx_table_id.db_tab_index ))


	    {
		/*
		** If altering a Master table, then also alter
		** its underlying partitions (but not indexes).
		*/
		if ( rel.reltid.db_tab_base != m->mx_table_id.db_tab_base ||
		    !(t->tcb_rel.relstat & TCB_IS_PARTITIONED) ||
		     rel.relstat & TCB_INDEX )
		{
		    continue;
		}
	    }
	    /*
	    ** Set the bits from new_relstat2 to mx_relation.relstat2
	    ** Note that in this case, "new_relstat2" is NOT relstat2,
	    ** it's mcb_mod_options2 bits.
	    ** With the current syntax and drivers, only one action bit
	    ** will be set, but go ahead and pretend otherwise.
	    */
	    if (m->mx_new_relstat2 & DM2U_2_LOG_INCONSISTENT)
	      rel.relstat2 |= TCB2_LOGICAL_INCONSISTENT;
	    else if (m->mx_new_relstat2 & DM2U_2_LOG_CONSISTENT)
	      rel.relstat2 &= ~TCB2_LOGICAL_INCONSISTENT;

	    if (m->mx_new_relstat2 & DM2U_2_PHYS_INCONSISTENT)
	      rel.relstat2 |= TCB2_PHYS_INCONSISTENT;
	    else if (m->mx_new_relstat2 & DM2U_2_PHYS_CONSISTENT)
	      rel.relstat2 &= ~TCB2_PHYS_INCONSISTENT;

	    if (m->mx_new_relstat2 & DM2U_2_TBL_RECOVERY_DISALLOWED)
	      rel.relstat2 |= TCB2_TBL_RECOVERY_DISALLOWED;
	    else if (m->mx_new_relstat2 & DM2U_2_TBL_RECOVERY_ALLOWED)
	      rel.relstat2 &= ~TCB2_TBL_RECOVERY_DISALLOWED;

	    if (m->mx_new_relstat2 & DM2U_2_PERSISTS_OVER_MODIFIES)
	      rel.relstat2 |= TCB_PERSISTS_OVER_MODIFIES;
	    else if (m->mx_new_relstat2 & DM2U_2_NOPERSIST_OVER_MODIFIES)
	      rel.relstat2 &= ~TCB_PERSISTS_OVER_MODIFIES;

	    if (m->mx_new_relstat2 & DM2U_2_STATEMENT_LEVEL_UNIQUE)
	      rel.relstat2 |= TCB_STATEMENT_LEVEL_UNIQUE;

	    /*
	    ** Set new Table Cache Priority if specified.
	    */
	    if (m->mx_new_relstat2 & DM2U_2_TO_TABLE_PRIORITY)
		rel.reltcpri = m->mx_tbl_pri;


	    status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&rel, (char *)NULL, dberr);

	    if ( status != E_DB_OK )
	        break;
	}

	/*
	** If error updating the base table then bail
	*/
        if ( status != E_DB_OK )
	    break;

    	status = dm2t_close(r, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;


    } while (FALSE);

    /*	Handle cleanup for error recovery. */

    if (r)
    {
	DB_STATUS local_status;

	local_status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);

	if ( local_status != E_DB_OK )
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    if ( status && (dberr->err_code == 0 || dberr->err_code > E_DM_INTERNAL) )
    {
	if (dberr->err_code )
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &error, 0);

	uleFormat( NULL, E_DM930D_DM2U_READONLY_UPD_CATS, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL,
	            (i4)0, (i4 *)NULL, &error, 3,
		    sizeof( m->mx_dcb->dcb_name),
		    &m->mx_dcb->dcb_name,
		    sizeof( m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid),
		    &m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid,
		    sizeof( m->mx_rcb->rcb_tcb_ptr->tcb_rel.relowner),
		    &m->mx_rcb->rcb_tcb_ptr->tcb_rel.relowner);

	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
    }
    return (status);
}

/*{
** Name: dm2u_modify_encrypt -  Unlock encryption and perhaps modify
**				the encryption passphrase.
**
** Description:
**	Most commonly we will be here to unlock encryption for a table.
**	In that case we just ensure that the table exists, the user has
**	access to it, and the user knows the encryption passphrase. An
**	encryption key to process user data will be posted to shared memory
**	(the Dmc_crypt block) for access by the AES encryption routines. A
**	variant on the above occurs when PASSPHRASE='' is specified. In
**	this case we clear out the Dmc_crypt entry. Note that this action
**	does not require knowledge of the passphrase, just valid ownership
**	of the table. Finally, it may be that the NEW_PASSPHRASE= option
**	was specified. In this case we decrypt the AES key with the old
**	passphrase and re-encrypt with the new passphrase. As a byproduct we
**	clear the Dmc_crypt entry. This forces a subsequent MODIFY ENCRYPT
**	command, implementing a sort of a "please retype your password"
**	feature. If this second MODIFY fails (oops, typed the wrong
**	thing!) and providing a COMMIT has not been issued, the user can
**	ROLLBACK and try again. Note that once the NEW_PASSPHRASE has been
**	implemented and a COMMIT has been done there is no access to the
**	data without this new passphrase (the only recovery option being
**	restoration from a backup of the database before the passphrase
**	change).
**
** Inputs:
**      mxcb                            The modify index control block.
**	dmu				The modify table control block.
**	journal				Indicates whether user table is
**					journaled.
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
**	    unlocks/locks table's encryption key in shared memory
**
** History:
**	16-apr-2010 (toumi01) SIR 122403
**	    Created with some cloning from dm2u_alterstatus_upd_cats.
**	27-Jul-2010 (toumi01) BUG 124133
**	    Store shm encryption keys by dbid/relid, not just relid! Doh!
**	29-Jul-2010 (miket) BUG 124154
**	    Improve dmf_crypt_maxkeys handling.
**	    Fix counter limit checking (was increment before test -
**	    should be test and if okay increment else error).
**	04-Aug-2010 (miket) SIR 122403
**	    Change encryption activation terminology from
**	    enabled/disabled to unlock/locked.
**	06-Aug-2010 (miket) SIR 122403
**	    Modify of catalog-stored AES keys is encrypting for
**	    128/192/256 bits 3/3/4 blocks, but it should be 2/2/3
**	    (see dm2u_create code and lengthy comment).
**	21-Aug-2010 (miket) SIR 122403 SD 146013
**	    Correct key cache lookup to use the db_tab_base of the record we
**	    are modifying, not the last one on which we did a get!
*/
DB_STATUS
dm2u_modify_encrypt(
DM2U_MXCB	*mxcb,
DMU_CB		*dmu,
i4		journal,
DB_ERROR	*dberr)
{
    DM2U_MXCB		*m = mxcb;
    DMP_RCB		*r = (DMP_RCB *)0;
    DMP_TCB		*t;
    DB_TAB_ID		table_id;
    DM_TID		tid;
    DB_STATUS		status;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION	rel;
    i4			error;
    DB_ERROR		local_dberr;

    bool		old_decrypt_done = FALSE;
    bool		new_encrypt_done = FALSE;
    u_char		*oldkey;
    u_char		*newkey;
    u_char		decrypted_key[sizeof(rel.relenckey)];
    i4			nrounds, keybits, keybytes, blocks;
    i4			i, cbc;
    char		*et, *pt, *pprev;
    u_i4		crc;
    u_i4		rk[RKLENGTH(AES_256_BITS)];


    if (Dmc_crypt == NULL)
    {
	i4 slots = 0;
	uleFormat( NULL, E_DM0179_DMC_CRYPT_SLOTS_EXHAUSTED,
	    (CL_ERR_DESC *)NULL,
	    ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &error, 1,
	    sizeof(slots), &slots);
	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
	return (E_DB_ERROR);
    }

    do
    {
	/*
	** Build a qualification list used for searching the system tables.
	*/
	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char *)&m->mx_table_id.db_tab_base;

	/*
	** Update the relstat bits for the base table and any
	** secondary indices that were built on the table whilst
	** table was in index_build mode
	*/
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;

	status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE,
		(i4)0, (i4)20, (i4)0,
		m->mx_log_id,m->mx_lk_id,
		(i4)0, (i4)0, DM2T_S, &m->mx_xcb->xcb_tran_id,
                &timestamp, &r, (DML_SCB *)0, dberr);
	if (status != E_DB_OK)
	    break;

	r->rcb_xcb_ptr = m->mx_xcb;

	t = m->mx_rcb->rcb_tcb_ptr;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
	    r->rcb_logging = 0;
	r->rcb_usertab_jnl = (journal) ? 1 : 0;

	/*
	** Update the base table accordingly
	*/
	status = dm2r_position(r, DM2R_QUAL, qual_list,
                     (i4)1,
                     (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(r, &tid, DM2R_GETNEXT,
                         (char *)&rel, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    CLRDBERR(dberr);
		    status = E_DB_OK;
		}
		break;
	    }

	    /*
	    ** If modifying the passphrase we will be updating the
	    ** relation record.
	    */
	    if (dmu->dmu_enc_flags2 & DMU_NEWPASS)
		TMget_stamp((TM_STAMP *)&rel.relstamp12);

	    /*
	    ** Check this is the correct base table
	    */
	    if ( (rel.reltid.db_tab_base !=
				m->mx_table_id.db_tab_base)
				||
		(rel.reltid.db_tab_index !=
				m->mx_table_id.db_tab_index ))
	    {
		/*
		** If altering a Master table, then also alter
		** its underlying partitions (but not indexes).
		*/
		if ( rel.reltid.db_tab_base != m->mx_table_id.db_tab_base ||
		    !(t->tcb_rel.relstat & TCB_IS_PARTITIONED) ||
		     rel.relstat & TCB_INDEX )
		{
		    continue;
		}
	    }

	    /*
	    ** Decrypt the relation-stored key with the passphrase= value.
	    ** The result is what we use to encrypt and decrypt user data.
	    */

	    /* Prepare based on the AES encryption type */
	    if ( old_decrypt_done || dmu->dmu_enc_flags2 & DMU_NULLPASS )
	    	continue;
	    if (!(rel.relencflags & TCB_ENCRYPTED))
	    {
		SETDBERR(dberr, 0, E_DM0177_RECORD_NOT_ENCRYPTED);
		status = E_DB_ERROR;
		break;
	    }
	    if (rel.relencflags & TCB_AES128)
	    {
		keybits = AES_128_BITS;
		keybytes = AES_128_BYTES;
		blocks = 2;
		oldkey = dmu->dmu_enc_old128pass;
		newkey = dmu->dmu_enc_new128pass;
	    }
	    else
	    if (rel.relencflags & TCB_AES192)
	    {
		keybits = AES_192_BITS;
		keybytes = AES_192_BYTES;
		blocks = 2;
		oldkey = dmu->dmu_enc_old192pass;
		newkey = dmu->dmu_enc_new192pass;
	    }
	    else
	    if (rel.relencflags & TCB_AES256)
	    {
		keybits = AES_256_BITS;
		keybytes = AES_256_BYTES;
		blocks = 3;
		oldkey = dmu->dmu_enc_old256pass;
		newkey = dmu->dmu_enc_new256pass;
	    }
	    else
	    {
		/* internal error */
		SETDBERR(dberr, 0, E_DM0175_ENCRYPT_FLAG_ERROR);
		status = E_DB_ERROR;
		break;
	    }

	    nrounds = adu_rijndaelSetupDecrypt(rk, oldkey, keybits);
	    pprev = NULL;
	    et = rel.relenckey;	/* encrypted key in relation */
	    pt = decrypted_key;	/* decrypted key on stack */
	    MEfill(sizeof(decrypted_key), 0, (PTR)&decrypted_key);
	    for ( i = blocks ; i > 0 ; i-- )
	    {
		adu_rijndaelDecrypt(rk, nrounds, et, pt);
		if (pprev)
		    for ( cbc = 0 ; cbc < AES_BLOCK ; cbc++ )
			pt[cbc] ^= pprev[cbc];
		pprev = et;
		et += AES_BLOCK;
		pt += AES_BLOCK;
	    }

	    old_decrypt_done = TRUE;

	    /*
	    ** Check the hash to verify we decrypted correctly. If not
	    ** it is a probable user error. Correct and retry!
	    */
	    crc = -1;
	    HSH_CRC32(decrypted_key, keybytes, &crc);
	    if (MEcmp((PTR)&crc, (PTR)decrypted_key + keybytes,
		sizeof(crc)) != 0)
	    {
		PCsleep(2000);	/* slow down rapid-fire guesses */
		SETDBERR(dberr, 0, E_DM0178_PASSPHRASE_FAILED_CRC);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** If we are just enabling encryption we'll not need to
	    ** modify the relation, so we're done here.
	    */
	    if (!(dmu->dmu_enc_flags2 & DMU_NEWPASS))
		break;

	    /*
	    ** It seems we're here to modify the passphrase, so we need
	    ** to re-encrypt the internal key with the new phrase.
	    */
	    if (!new_encrypt_done)
	    {
		nrounds = adu_rijndaelSetupEncrypt(rk, newkey, keybits);
		pprev = NULL;
		et = rel.relenckey;	/* encrypted key in relation */
		pt = decrypted_key;	/* plain key on stack */
		for ( i = blocks ; i > 0 ; i-- )
		{
		    /* CBC cipher mode */
		    if (pprev)
			for ( cbc = 0 ; cbc < AES_BLOCK ; cbc++ )
			    pt[cbc] ^= pprev[cbc];
		    adu_rijndaelEncrypt(rk, nrounds, pt, et);
		    pprev = et;
		    et += AES_BLOCK;
		    pt += AES_BLOCK;
		}
		new_encrypt_done = TRUE;
	    }

	    status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&rel, (char *)NULL, dberr);

	    if ( status != E_DB_OK )
	        break;
	}

	/*
	** If error updating the base table then bail
	*/
        if ( status != E_DB_OK )
	    break;

    	status = dm2t_close(r, DM2T_NOPURGE, dberr);
	if (status != E_DB_OK)
	    break;
	r = 0;

    } while (FALSE);

    /*	Handle cleanup for error recovery. */

    if (r)
    {
	DB_STATUS local_status;

	local_status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);

	if ( local_status != E_DB_OK )
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    if ( status && (dberr->err_code == 0 || dberr->err_code > E_DM_INTERNAL) )
    {
	if (dberr->err_code )
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &error, 0);

	uleFormat( NULL, E_DM944F_DM2U_MOD_ENCRYPT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL,
	            (i4)0, (i4 *)NULL, &error, 3,
		    sizeof( m->mx_dcb->dcb_name),
		    &m->mx_dcb->dcb_name,
		    sizeof( m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid),
		    &m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid,
		    sizeof( m->mx_rcb->rcb_tcb_ptr->tcb_rel.relowner),
		    &m->mx_rcb->rcb_tcb_ptr->tcb_rel.relowner);

	SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
    }

    /*
    ** If all is well, post the encryption key for user data to shared
    ** memory, keyed on the base table id.
    */
    if ( status == E_DB_OK )
    {
	DMC_CRYPT_KEY	*cp;
	DMC_CRYPT_KEY	*cp_inactive = NULL;

	CSp_semaphore(TRUE, &Dmc_crypt->crypt_sem);
	/*
	** If we changed the passphrase, nullify the encryption enabling
	** so that the user has to respecify it to turn on encryption again.
	** That way, they are forced to prove they know the key and can
	** ROLLBACK their passphrase change and try again if they fat-finger
	** the new key specification.
	*/ 
	if (dmu->dmu_enc_flags2 & DMU_NEWPASS)
	    dmu->dmu_enc_flags2 |= DMU_NULLPASS;
	/*
	** A linear search. Not elegant, but we only have to look through
	** memory at active slots, one per table with encryption that has
	** been unlocked, matching on base reltid. Should be fast.
	*/
	for ( cp = (DMC_CRYPT_KEY *)((PTR)Dmc_crypt + sizeof(DMC_CRYPT)),
		i = 0 ;
		i < Dmc_crypt->seg_active ; cp++, i++ )
	{
	    /* found the entry for this record ... done */
	    if ( cp->db_id == t->tcb_dcb_ptr->dcb_id &&
		 cp->db_tab_base == m->mx_table_id.db_tab_base )
		break;
	    /* found first inactive slot ... remember */
	    if ( cp->status == DMC_CRYPT_INACTIVE &&
		 cp_inactive == NULL )
		cp_inactive = cp;
	}
	/*
	** Are we here just to relock encryption because PASSPHRASE=''
	** was specified? If so and we found our man, clear him out!
	** Otherwise, encryption was already off for the table.
	*/
	if (dmu->dmu_enc_flags2 & DMU_NULLPASS)
	{
	    if ( cp->db_id == t->tcb_dcb_ptr->dcb_id &&
		 cp->db_tab_base == m->mx_table_id.db_tab_base )
	    {
		MEfill(sizeof(DMC_CRYPT_KEY), 0, (PTR)cp);
		cp->status = DMC_CRYPT_INACTIVE;
	    }
	}
	else
	/*
	** Use (in order of preference)
	** 1. our own existing slot
	** 2. the first inactive slot
	** 3. a new slot at the end
	*/
	{
	    if ( cp->db_id == t->tcb_dcb_ptr->dcb_id &&
		 cp->db_tab_base == m->mx_table_id.db_tab_base )
	    	;			/* reuse our slot */
	    else
	    if ( cp_inactive != NULL)
		cp = cp_inactive;	/* reuse empty slot */
	    else
	    {
		if (Dmc_crypt->seg_active+1 > Dmc_crypt->seg_size)
		{
		    /* > config.dat ii.*.dbms.*.dmf_crypt_maxkeys */
		    uleFormat( NULL, E_DM0179_DMC_CRYPT_SLOTS_EXHAUSTED,
			(CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL,
			(i4)0, (i4 *)NULL, &error, 1,
			sizeof(Dmc_crypt->seg_size),
			&Dmc_crypt->seg_size);
		    SETDBERR(dberr, 0, E_DM9101_UPDATE_CATALOGS_ERROR);
		    status = E_DB_ERROR;
		}
		else
		    Dmc_crypt->seg_active++;	/* new slot */
	    }
	    if ( status == E_DB_OK )
	    {
		MEfill(sizeof(DMC_CRYPT_KEY), 0, (PTR)cp);
		MEcopy((PTR)decrypted_key, keybytes, (PTR)cp->key);
		cp->db_id = t->tcb_dcb_ptr->dcb_id;
		cp->db_tab_base = m->mx_table_id.db_tab_base;
		cp->status = DMC_CRYPT_ACTIVE;
	    }
	}
	CSv_semaphore(&Dmc_crypt->crypt_sem);
    }

    return (status);
}

/*{
** Name: dm2u_bbox_range - create an ibox from the range box
**
** Description:
**      Create an ibox from the box in the range field.
**	    ADF routines are used to locate the correct data types and routine.
**
** Inputs:
**	tcb				Table Control Block
**  adf_cb			ADF Session Control Block
**
** Outputs:
**      err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	26-jun-1996 (shero03)
**	    Created from dm2t.c as part of RTree project.
**	15-apr-1997 (shero03)
**	    Support 3D nbr and 3D range boxes.
**	14-may-1998 (nanpr01)
**	    make this function available globally and change its name
**	    to dm2u_bbox_range from bbox_range (static function).
*/
DB_STATUS
dm2u_bbox_range(
DM2U_MXCB	*m,
ADF_CB		*adf_scb,
DB_ERROR	*dberr)
{
    DB_STATUS	status = E_DB_OK;
    f8		*fll;
    f8		*fur;
    i4		i;
    i4	*ill;
    i4	*iur;
    i4	irange_values[DB_MAXRTREE_DIM * 2];

    /*
    **  Create an integer box just larger than the float vertexes.
    */
    fll = &m->mx_range[0];
    fur = &m->mx_range[m->mx_dimension];
    ill = &irange_values[0];
    iur = &irange_values[m->mx_dimension];
     
    for (i = 0; i < m->mx_dimension; i++, fll++, fur++, ill++, iur++)
    {
    	*ill = (i4)MHfdint(*fll);
	*iur = (i4)MHceil(*fur);
    }

    MEcopy((PTR)&irange_values, sizeof(i4)*m->mx_dimension*2, (PTR)&m->mx_range);

    return (status);
}


/*{
** Name: dm2u_fix_comment - fix the comments catalogs if the table was altered. 
**
** Description:
**	Fix up the attribute id's in the comment catalog.
**
** Inputs:
**	m		The control block
**	attr_map	Map of attributes which were dropped
**	new_map		New Attribute numbers
**	colcnt		Number of cols before modified
**
** Outputs:
**	comment		if relstat need to be updated.
**      err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	31-jan-1997 (nanpr01)
**	    Created to fix the comments catalog if the table was altered.
*/
static DB_STATUS
dm2u_fix_comment(
DM2U_MXCB	*mxcb,
i4		attr_map[],
i4		new_map[],
i4		colcnt,
bool		*comment,
DB_ERROR	*dberr)
{
    DM2U_MXCB           *m = mxcb;
    DMP_RCB             *r = (DMP_RCB *)0;
    DMP_RCB             *rcb;
    DB_TAB_ID           table_id;
    DB_IICOMMENT	dbcomment;
    DB_STATUS           status;
    i4             local_error;
    DM2R_KEY_DESC       qual_list[2];
    DM_TID		tid;
    DB_TAB_TIMESTAMP    timestamp;
    i4			error;
    DB_ERROR		local_dberr;


    *comment = 0;
    table_id.db_tab_base = DM_B_COMMENT_TAB_ID;
    table_id.db_tab_index = DM_I_COMMENT_TAB_ID;

    /*  Build a qualification list used for searching the system tables. */
 
    qual_list[0].attr_number = DM_1_RELATION_KEY;
    qual_list[0].attr_operator = DM2R_EQ;
    qual_list[0].attr_value = (char *)&m->mx_table_id.db_tab_base;
    qual_list[1].attr_number = DM_2_ATTRIBUTE_KEY;
    qual_list[1].attr_operator = DM2R_EQ;
    qual_list[1].attr_value = (char *)&m->mx_table_id.db_tab_index;
 

    status = dm2t_open(m->mx_dcb, &table_id, DM2T_IX, DM2T_UDIRECT, 
			DM2T_A_WRITE, (i4)0, (i4)20, (i4)0,
			m->mx_xcb->xcb_log_id,m->mx_lk_id, (i4)0, 
			(i4)0, DM2T_S, &m->mx_xcb->xcb_tran_id,
			&timestamp, &rcb, (DML_SCB *)0, dberr);
    if (status != E_DB_OK)
	return(status);
    r = rcb;
    r->rcb_xcb_ptr = m->mx_xcb;
 
    /*
    ** Check for NOLOGGING - no updates should be written to the log
    ** file if this session is running without logging.
    */
    if (m->mx_xcb->xcb_flags & XCB_NOLOGGING)
	r->rcb_logging = 0;
 
    status = dm2r_position(r, DM2R_QUAL, qual_list, (i4)1,
			   (DM_TID *)0, dberr);
 
    if (status != E_DB_OK)
	return(status);
    for (;;)
    {
	status = dm2r_get(r, &tid, DM2R_GETNEXT, (char *)&dbcomment, dberr);
	if (status != E_DB_OK)
	{
	    if (dberr->err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		CLRDBERR(dberr);
	    }
	    break;
	}

	if (dbcomment.dbc_tabid.db_tab_base ==  m->mx_table_id.db_tab_base &&
	    dbcomment.dbc_tabid.db_tab_index == m->mx_table_id.db_tab_index )
	{
	   if (attr_map[dbcomment.dbc_attid] == 1)
	   {
		/* delete the row */
		status = dm2r_delete(r, &tid, DM2R_BYPOSITION, dberr);
		if (status != E_DB_OK)
		    break;
	   }
	   else {
	     dbcomment.dbc_attid = new_map[dbcomment.dbc_attid];
	     *comment = 1;
	     /* replace the row */
             status = dm2r_replace(r, &tid, DM2R_BYPOSITION,
                             (char *)&dbcomment, (char *)NULL, dberr);
	     if (status != E_DB_OK)
		break;
	   }
	}
    }
 
    if ( dm2t_close(r, DM2T_NOPURGE, &local_dberr) )
    {
	if ( status == E_DB_OK )
	{
	    status = E_DB_ERROR;
	    *dberr = local_dberr;
	}
    }
    return(status);
}

/*{
** Name: dm2u_raw_device_lock - Gets a Xclusive value lock on raw location.
**
** Description:
**	Since one raw location can only be used by one table and since 
**	no file can be created on that location, in order to be recoverable, 
**	we will get the exclusive value lock on that device so that we can 
**	protect the content of the device until the end of transaction.
**
** Inputs:
**	dcb		database control block.
**	xcb		transaction control block.
**	location	location description.
**	
**
** Outputs:
**	lockid		returns lockid when successful
**      err_code
**
**      Returns:
**          LK_NEW_LOCK
**          E_DB_ERROR
**
** History:
**	12-mar-1999 (nanpr01)
**	    Created.
**	03-Mar-2005 (jenjo02)
**	    Added lock tracing for debugging.
*/
STATUS
dm2u_raw_device_lock(
DMP_DCB             *dcb,
DML_XCB             *xcb,
DB_LOC_NAME         *location,
LK_LKID             *lockid,
DB_ERROR	    *dberr)
{
    STATUS	status;
    LK_LOCK_KEY	lockkey;
    CL_ERR_DESC sys_err;
    i4		lk_flags;

    lk_flags = dcb->dcb_bm_served == DCB_SINGLE ?
                       LK_STATUS | LK_LOCAL : LK_STATUS;

    lockkey.lk_type = LK_VAL_LOCK;
    lockkey.lk_key1 = dcb->dcb_id;
    MEcopy((char *)location, 20, (char *)&lockkey.lk_key2);

    /* Output a lock trace message if tracing is enabled. */
    if (DMZ_SES_MACRO(1))
	dmd_lock(&lockkey, xcb->xcb_lk_id, LK_REQUEST, lk_flags, LK_X, 
	    0L, xcb->xcb_tran_id, (DB_TAB_NAME*)location, 
	    &dcb->dcb_name);
	    
    status = LKrequest(lk_flags,
                       xcb->xcb_lk_id, &lockkey, LK_X,
                       (LK_VALUE * )0, lockid, 0L, &sys_err);
    if (status != LK_NEW_LOCK)
    {
	SETDBERR(dberr, 0, E_DM0189_RAW_LOCATION_INUSE);
        return(status);
    }
    return(status);
}

/*{
** Name: dm2u_raw_location_free - Checks to see if the raw location is in use?
**
** Description:
**	This function checks in iidevices and iirelation to make sure the 
**	location is not in use. We must acquire the value locks on the raw
**	location before this routine is called. Consequently a lower isolation
**	level can be used to check for the existence of the location.
**
** Inputs:
**	dcb		database control block.
**	xcb		transaction control block.
**	location	location description.
**	
**
** Outputs:
**      err_code
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**	12-mar-1999 (nanpr01)
**	    Created.
**	27-Aug-2010 (jonj)
**	    Pass locid parameter to communicate location index
**	    to QEF via SETDBERR.
*/
DB_STATUS
dm2u_raw_location_free(
DMP_DCB             *dcb,
DML_XCB             *xcb,
DB_LOC_NAME         *location,
i4		    locid,
DB_ERROR	    *dberr)
{
    DMP_RCB             *rcb = 0;
    DB_TAB_ID           table_id;
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RELATION        relation;
    DMP_DEVICE		devrecord;
    DM_TID		tid;
    DB_STATUS           status;
    i4			error;			
    DB_ERROR		local_dberr;

    table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
    table_id.db_tab_index = DM_I_DEVICE_TAB_ID;
 
    status = dm2t_open(dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
		xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0, DM2T_S,
		&xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, dberr);
    if ( status )
	return(E_DB_ERROR);

    /* Position for a full table scan. */
    status = dm2r_position(rcb, DM2R_ALL, 0, 0, NULL, dberr);
    if (status != E_DB_OK)
	return(E_DB_ERROR);
 
    /* Examine each record returned. */
    for (;;)
    {
	/*  Get the next record. */
	status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (char *)&devrecord,
                              dberr);
	if (status != E_DB_OK)
	{
	    if (dberr->err_code == E_DM0055_NONEXT)
	    {
		CLRDBERR(dberr);
		status = E_DB_OK;
	    }
            break;
        }
	if (MEcmp((char *)&devrecord.devloc, (char *)location, 
				sizeof(DB_LOC_NAME)) == 0)
	{
    	    /* Close the table. */
	    status = dm2t_close(rcb, DM2T_NOPURGE, dberr);
	    SETDBERR(dberr, locid, E_DM0190_RAW_LOCATION_OCCUPIED);
	    return(E_DB_ERROR);
	}
    }
    /* Close the table. */
    status = dm2t_close(rcb, DM2T_NOPURGE, dberr);
    if (status != E_DB_OK)
	return(E_DB_ERROR);
 
    /* Open the relation */
    table_id.db_tab_base = DM_B_RELATION_TAB_ID;
    table_id.db_tab_index = DM_I_RELATION_TAB_ID;

    status = dm2t_open(dcb, &table_id, DM2T_IX,
            DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
            xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0, DM2T_S,
            &xcb->xcb_tran_id, &timestamp, &rcb, (DML_SCB *)0, dberr);
    if (status)
	return(E_DB_ERROR);

    /* Position for a full table scan. */
    status = dm2r_position(rcb, DM2R_ALL, 0, 0, NULL, dberr);
    if (status != E_DB_OK)
    {
	status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	return(E_DB_ERROR);
    }
 
    /* Examine each record returned. */
    for (;;)
    {
	/*  Get the next record. */
	status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (char *)&relation,
                              dberr);
	if (status != E_DB_OK)
	{
	    if (dberr->err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		CLRDBERR(dberr);
	    }
            break;
        }
	if (MEcmp((char *)&relation.relloc, (char *)location, 
				sizeof(DB_LOC_NAME)) == 0)
	{
    	    /* Close the table. */
	    status = dm2t_close(rcb, DM2T_NOPURGE, dberr);
	    SETDBERR(dberr, locid, E_DM0190_RAW_LOCATION_OCCUPIED);
	    return(E_DB_ERROR);
	}
    }
    /* Close the table. */
    status = dm2t_close(rcb, DM2T_NOPURGE, dberr);

    return(status);
}


/*
** Name: dm2u_put_dupchktbl - create table to hold duplicate key values 
**
** Description:
**      This routine creates and opens a table, if tbl does not already
**	exist, to hold duplicate key values encountered by the sorter 
** 	during online modify. The key is then put into the table.
**	Save new table's table id, such that we can open table to perform
**	duplicate checking after sort and sidefile processing has 
**	completed (see do_online_dupcheck() in dm2umod.c).
**	Table will be closed at the end of the sort (see dm2u_load_table()).
**
** Inputs:
**      mxcb		pointer to mxcb
**	record		pointer to the record
**
**
** Outputs:
**      tpcb_dupchk_rcb	pointer to rcb for new table to hold 
**			duplicate key values
**      err_code	pointer to err code 
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
** History:
**      1-mar-2004 (thaju02)
**          Created.
**	16-dec-04 (inkdo01)
**	    Add attr_collID.
**	12-Nov-2008 (jonj)
**	    Relocate dupchk_tcb declaration.
**	29-Sept-2009 (troal01)
**		Add geospatial support
**	12-Apr-2010 (kschendel) SIR 123485
**	    Open no-coupon to avoid BLOB handling overhead.
*/

DB_STATUS
dm2u_put_dupchktbl(
DM2U_TPCB	*tp,
char		*record,
DB_ERROR	*dberr)
{
    DM2U_MXCB           *m = tp->tpcb_mxcb;
    DM2U_M_CONTEXT	*mct = &tp->tpcb_mct;
    DMP_RCB             *r = m->mx_rcb;  /* $online modtable  */
    DMP_TCB             *t = r->rcb_tcb_ptr;  /* $online modtable */
    DMP_DCB		*dcb = m->mx_dcb;
    DML_XCB		*xcb = m->mx_xcb;
    DMP_MISC            *misc_cb = (DMP_MISC *)0;
    i4                  attr_count;
    DB_OWN_NAME         owner;
    DB_STATUS		status = E_DB_OK;
    char                *key, *pos;
    DB_TAB_ID		*new_tabid = tp->tpcb_dupchk_tabid;
    i4			error, localerr;
    DB_TAB_TIMESTAMP    timestamp;
    DB_ATTS		*keyatt;
    i4                  i;
    i4			mem_needed;
    DMP_TCB		*dupchk_tcb;

    for (;;)
    {
	/* $online key makes up $dupchk attributes */
	attr_count= mct->mct_keys;

	/* FIX ME - how to handle online modify of indexes? */
	if ((mct->mct_index) && !(mct->mct_unique) && 
	    (mct->mct_page_type == TCB_PG_V1) &&
	    (m->mx_atts_ptr[mct->mct_data_rac.att_count].key != 0))
	{
	    /* Note creating temp base table like index (minus tidp column) */
	    attr_count -= 1; /* remove tidp for create table */
	}

	/* 
	** $dupchk table needs to be created?
	** $dupchk temp tbl may have already been created in the case of 
	** parallel indices.
	*/
	if (!(tp->tpcb_dupchk_rcb) && !(new_tabid->db_tab_base))
	{
	    DMT_CB		dmt_cb;
	    DMT_CHAR_ENTRY      characteristics[3];
	    DB_LOC_NAME         *loc_list;
	    i4                  loc_count = tp->tpcb_loc_count;
	    DMF_ATTR_ENTRY     *attrs;
	    DMF_ATTR_ENTRY     **attr_ptrs;
	    
            MEfill(sizeof(DMT_CB), '\0', (PTR) &dmt_cb);
            dmt_cb.length = sizeof(DMT_CB);
            dmt_cb.type = DMT_TABLE_CB;
            dmt_cb.dmt_flags_mask = DMT_DBMS_REQUEST;
	    dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
	    dmt_cb.dmt_tran_id = (PTR)xcb;

	    /* 
	    ** this is not necessary, since currently dmt_create_temp 
	    ** assigns DB_TEMP_TAB_NAME
	    */
	    STprintf(dmt_cb.dmt_table.db_tab_name, "$dupchk_%x_%x", 
		    (i4) m->mx_table_id.db_tab_base, 
		    m->mx_table_id.db_tab_index);

	    STmove(dmt_cb.dmt_table.db_tab_name, ' ', 
		    sizeof(dmt_cb.dmt_table.db_tab_name), 
		    dmt_cb.dmt_table.db_tab_name);

	    MEfill(sizeof(dmt_cb.dmt_owner.db_own_name),
		    (u_char)' ', (PTR)dmt_cb.dmt_owner.db_own_name);

	    characteristics[0].char_id = DMT_C_PAGE_SIZE;
	    characteristics[0].char_value = m->mx_page_size;
	    characteristics[1].char_id = DMT_C_PAGE_TYPE;
	    characteristics[1].char_value = m->mx_page_type;
	    characteristics[2].char_id = DMT_C_DUPLICATES;
	    characteristics[2].char_value = DMT_C_ON;

	    dmt_cb.dmt_char_array.data_address = (PTR)characteristics;
	    dmt_cb.dmt_char_array.data_in_size = 2 * sizeof(DMT_CHAR_ENTRY);

	    mem_needed = sizeof(DMP_MISC) +
		    (i4)(attr_count * sizeof(DMF_ATTR_ENTRY)) +
		    (i4)(attr_count * sizeof(DMF_ATTR_ENTRY *)) +
		    mct->mct_klen + /* FIX ME -  acct for secondary idx tidp */
		    (i4)(loc_count + sizeof(DB_LOC_NAME));

	    status = dm0m_allocate(mem_needed, 0, (i4)MISC_CB, 
			    (i4)MISC_ASCII_ID, (char *)NULL, 
			    (DM_OBJECT **)&misc_cb, dberr);
	    if (status)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(dberr, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			    0, mem_needed);
		break;
	    }

	    attrs = (DMF_ATTR_ENTRY *)((char *)misc_cb + sizeof(DMP_MISC));
	    misc_cb->misc_data = (char*)attrs;
	    attr_ptrs = (DMF_ATTR_ENTRY **)((char *)attrs + 
			    (attr_count * sizeof(DMF_ATTR_ENTRY)));
	    pos = key = (char *)attr_ptrs + (attr_count * sizeof(DMF_ATTR_ENTRY *));
	    loc_list = (DB_LOC_NAME *)(key + mct->mct_klen);

	    /* 
	    ** build $dupchk attributes based on $online keys
	    ** and build $dupchk tuple
	    */
	    for ( i = 0; i < attr_count; i++ )
	    {
		keyatt = mct->mct_key_atts[i];
		attr_ptrs[i] = &attrs[i];
		MEmove(keyatt->attnmlen, keyatt->attnmstr, ' ',
			DB_ATT_MAXNAME, attrs[i].attr_name.db_att_name);
		attrs[i].attr_type = keyatt->type;
		attrs[i].attr_size = keyatt->length;
		attrs[i].attr_precision = keyatt->precision;
		attrs[i].attr_flags_mask = keyatt->flag;
		COPY_DEFAULT_ID( keyatt->defaultID, attrs[i].attr_defaultID );
		attrs[i].attr_collID = keyatt->collID;
		attrs[i].attr_geomtype = keyatt->geomtype;
		attrs[i].attr_srid = keyatt->srid;
		attrs[i].attr_encflags = keyatt->encflags;
		attrs[i].attr_encwid = keyatt->encwid;
		MEcopy((PTR)(record + keyatt->offset), keyatt->length, pos);
		pos += keyatt->length;
	    }

	    for (i = 0; i < loc_count; i++)
		STRUCT_ASSIGN_MACRO(tp->tpcb_locations[i].loc_name, loc_list[i]);

	    dmt_cb.dmt_location.data_address = (PTR)loc_list;
	    dmt_cb.dmt_location.data_in_size = loc_count * sizeof(DB_LOC_NAME);

            dmt_cb.dmt_attr_array.ptr_address = (PTR)attr_ptrs;
            dmt_cb.dmt_attr_array.ptr_in_count = attr_count;
            dmt_cb.dmt_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

            status = dmt_create_temp(&dmt_cb);

            if (status != E_DB_OK)
            {
		*dberr = dmt_cb.error;
                break;
            }

            STRUCT_ASSIGN_MACRO(dmt_cb.dmt_id, *new_tabid);

	    if (DMZ_SRT_MACRO(13))
		TRdisplay("%@ dm2u_put_dupchktbl: %~t created %~t (%d,%d), \
columns = %d, length = %d\n",
				sizeof(DB_TAB_NAME), t->tcb_rel.relid,
				sizeof(DB_TAB_NAME), dmt_cb.dmt_table, 
				new_tabid->db_tab_base, new_tabid->db_tab_index,
				attr_count, (pos - key));
	}   
	else
	{
	    /* build tuple for $dupchk from $online key value */
	    status = dm0m_allocate(sizeof(DMP_MISC) + mct->mct_klen, 0, 
			(i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL,
			(DM_OBJECT **)&misc_cb, dberr);
	    if (status)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(dberr, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
			0, mem_needed);
		break;
	    }
	    pos = key = ((char *)misc_cb + sizeof(DMP_MISC));
	    misc_cb->misc_data = (char*)pos;

	    for (i = 0; i < attr_count; i++)
	    {
		keyatt = mct->mct_key_atts[i];
		MEcopy((PTR)(record + keyatt->offset), keyatt->length, pos);
		pos += keyatt->length;
	    }
	}

	if (!(tp->tpcb_dupchk_rcb))
	{
           /* Open $dupchk table */
            status = dm2t_open(dcb, new_tabid, DM2T_X,
		DM2T_UDIRECT, DM2T_A_WRITE_NOCPN,
                (i4)0, (i4)20, (i4)0, m->mx_log_id, m->mx_lk_id, (i4)0, (i4)0,
                DM2T_X, &xcb->xcb_tran_id, &timestamp, &tp->tpcb_dupchk_rcb,
                (DML_SCB *)0, dberr);

            if (status)
            {
                uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
                                    (i4)0, (i4 *)NULL, &error, 0);
		SETDBERR(dberr, 0, E_DM008F_ERROR_OPENING_TABLE);
                break;
            }

            tp->tpcb_dupchk_rcb->rcb_xcb_ptr = xcb;
	}

	status = dm2r_put(tp->tpcb_dupchk_rcb, DM2R_DUPLICATES, key, dberr);
	if (status)
	{
	    dupchk_tcb = tp->tpcb_dupchk_rcb->rcb_tcb_ptr;

	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
		    (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM904D_ERROR_PUTTING_RECORD, NULL, ULE_LOG, NULL,
		(char *)NULL, 0L, (i4 *)NULL, &error, 3,
		sizeof(DB_DB_NAME), &dupchk_tcb->tcb_dcb_ptr->dcb_name,
		sizeof(DB_OWN_NAME),&dupchk_tcb->tcb_rel.relowner,
		sizeof(DB_TAB_NAME),&dupchk_tcb->tcb_rel.relid);
	    SETDBERR(dberr, 0, E_DM008B_ERROR_PUTTING_RECORD);
	}
	if (DMZ_SRT_MACRO(13))
	    TRdisplay("%@ dm2u_put_dupchktbl: %~t (%d,%d) appended tuple, \
length = %d\n",
		sizeof(DB_TAB_NAME),
		tp->tpcb_dupchk_rcb->rcb_tcb_ptr->tcb_rel.relid,
		new_tabid->db_tab_base, new_tabid->db_tab_index,
		(pos - key));
	break;
    }

    if (misc_cb)
	dm0m_deallocate((DM_OBJECT **)&misc_cb);

    return(status);
}


/*{
** Name: dm2u_check_for_interrupt - Check for User interrupt or
**				    Force Abort.
**
** Description:
**
**	Checks XCB state for User interrupt or Force Abort event.
**
** Inputs:
**	xcb		transaction control block.
**
** Outputs:
**
**      Returns:
**          E_DB_OK		if no interrupt.
**          E_DB_ERROR		If user interrupt or force-abort.
**				with
**              err_code	describing which it is.
**
** History:
**	29-Apr-2004 (jenjo02)
**	    Created for many-partitioned modifies.
**	04-May-2004 (jenjo02)
**	    ...but make sure there's an XCB first,
**	    won't be one if rollforward.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	13-Feb-2007 (kschendel)
**	    Add in a cancel check as well.  We assume that the XCB
**	    points to the running thread's SCB.  (DMU factota are
**	    created without bothering about a separate XCB, but they
**	    don't call this routine.)
*/
DB_STATUS
dm2u_check_for_interrupt(
DML_XCB		*xcb,
DB_ERROR	*dberr)
{
    i4		*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    
    if ( xcb )
    {
	DML_SCB *scb = xcb->xcb_scb_ptr;

	/* Check for external interrupts */
	if ((scb->scb_s_type & SCB_S_FACTOTUM) == 0)
	    CScancelCheck(scb->scb_sid);
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, err_code);
	if ( xcb->xcb_state & XCB_USER_INTR )
	    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	else if ( xcb->xcb_state & XCB_FORCE_ABORT )
	    SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
	else if ( xcb->xcb_state & XCB_ABORT )
	    SETDBERR(dberr, 0, E_DM0064_USER_ABORT);
    }

    return( (dberr->err_code) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: dm2uGetTempTableId	- Create unique Temporary table id.
**
** Description:
**      This routine is called from dmt_create_temp() and 
**	CreateTempIndexTCB() to assign a database-unique
**	Temporary Table db_tab_base or db_tab_index.
**
** Inputs:
**	dcb				The database.
**	lock_list			The appropriate lock list
**	base_tbl_id			The table id of the base table
**					 (used if creating an index table id)
**	index				TRUE if create an index table.
** Outputs:
**	new_tab_id			The table id (base,index) assigned
**      err_code                        The reason for an error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    An X LK_TABLE lock is taken on the table_id, held until the
**	    Table/Index is explicitly dropped or end of session, whichever
**	    comes first.
**
** History:
**	14-Aug-2006 (jonj)
**	    Extracted from dmt_create_temp() for Temp Base and Index tables.
**      21-Jan-2009 (horda03) Bug 121519
**          Ensure Temp. Table IDs are within required range. Use defined macros
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
*/
DB_STATUS
dm2uGetTempTableId(
DMP_DCB		*dcb,
i4		lock_list,
DB_TAB_ID	*base_tbl_id,
bool	        index,
DB_TAB_ID	*new_tab_id,
DB_ERROR	*dberr)
{
    LK_LOCK_KEY		tti_k_key;
    LK_LKID		tti_k_id;
    LK_VALUE		tti_k_value;
    LK_LOCK_KEY		lock_key;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    i4			local_error;

    /* 
    ** Get a unique id for the table/index.  This is done
    ** by getting a lock on the database which keeps track of temporary
    ** table ids.
    **
    ** The caller is responsible for providing the appropriate lock list,
    ** either scb_lock_list for Session temps, or xcb_lk_id for
    ** internal temp tables.
    */
    for (;;)
    {
	tti_k_key.lk_type = LK_DB_TEMP_ID;
	MEfill(sizeof(LK_LKID), 0, &tti_k_id);
	MEcopy( (PTR)&dcb->dcb_name, LK_KEY_LENGTH, (PTR)&tti_k_key.lk_key1);

	status = LKrequest(LK_PHYSICAL, lock_list, &tti_k_key, 
			 LK_X, (LK_VALUE * )&tti_k_value, 
			 &tti_k_id, (i4)0, &sys_err); 
	if (status != OK && status != LK_VAL_NOTVALID)
	{
	    if (status == LK_NOLOCKS)
	    {
		uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err,
		    ULE_LOG, NULL, NULL, 0, NULL, &local_error, 2, 9, "TEMPORARY",
		    sizeof(DB_DB_NAME), &dcb->dcb_name);
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		break;
	    }
	    else if ((status == LK_INTERRUPT) || (status == LK_INTR_GRANT))
	    {
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
		break;
	    }
            else if (status == LK_INTR_FA)
            {
                SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
                break;
            }

	    uleFormat( NULL, status, &sys_err, ULE_LOG , NULL, (char *)NULL, 0L,
		    (i4 *)NULL, &local_error, 0);
	    uleFormat( NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, 
		    &local_error, 2, 0, LK_X, 0, lock_list);
	    if (status == LK_DEADLOCK)
	    {	
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;
	    }
	    else
	    {
		SETDBERR(dberr, 0, E_DM0077_BAD_TABLE_CREATE);
		break;
	    }
	}

	if (tti_k_value.lk_low == 0)
	    tti_k_value.lk_low = DB_TEMP_TAB_ID_MIN;
	if (++tti_k_value.lk_low == DB_TEMP_TAB_ID_MAX)
	    tti_k_value.lk_low = DB_TEMP_TAB_ID_MIN;

	/*
	** Release the lock with the updated value. 
	*/

	status = LKrelease((i4)0, lock_list, (LK_LKID *)&tti_k_id, 
		  (LK_LOCK_KEY *)0, &tti_k_value, &sys_err);
	if (status != OK)
	{
	    uleFormat( NULL, status, &sys_err, ULE_LOG , NULL, (char *)NULL, 0L,
		    (i4 *)NULL, &local_error, 0);
	    uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, 
		    &local_error, 1, 0, lock_list);
	    SETDBERR(dberr, 0, E_DM901B_BAD_LOCK_RELEASE);
	    break;
	}

	/*
	** Try and obtain a lock on this temp. table.  We use
	** LK_NOWAIT to return immediately from LKrequest() if the
	** lock can't be granted and hence we can't use this
	** particular id ( i.e. someone else has the table ) - try
	** again in this instance with the next available temp. table
	** id.
	**
	** We also try again if we don't get LK_NEW_LOCK back - this
	** implies that this thread already has the table ( unlikely
	** but possible ).
	**
	** Note we will keep the lock if obtained - it will be
	** released at session/transaction end or when the table
	** is dropped.  This lock holds the temp table ID.
	**
	** For Base Tables, the table_id and lock are
	**
	**		db_tab_base  = tti_k_value.lk_low
	**		db_tab_index = 0;
	**
	** For an Index
	**		db_tab_base  = BaseTableId.db_tab_base
	**		db_tab_index = tti_k_value.lk_low
	**				with the PARTITION bit
	**				(0x80000000) turned off
	**				so it doesn't look like
	**				a partition.
	*/
	if ( index )
	{
	    new_tab_id->db_tab_base = base_tbl_id->db_tab_base;
	    new_tab_id->db_tab_index = tti_k_value.lk_low;

	    /* TempTable Indexes musn't look like Partitions */
	    new_tab_id->db_tab_index &= ~DB_TAB_PARTITION_MASK;
	}
	else
	{
	    new_tab_id->db_tab_base = tti_k_value.lk_low;
	    new_tab_id->db_tab_index = 0;
	}

	lock_key.lk_type = LK_TABLE;
	lock_key.lk_key1 = dcb->dcb_id;
	lock_key.lk_key2 = new_tab_id->db_tab_base;
	lock_key.lk_key3 = new_tab_id->db_tab_index;
	lock_key.lk_key4 = 0;
	lock_key.lk_key5 = 0;
	lock_key.lk_key6 = 0;

	status = LKrequest( LK_PHYSICAL | LK_NOWAIT | LK_STATUS,
			   lock_list, &lock_key,
			   LK_X, (LK_VALUE *)0, (LK_LKID *)0,
			   (i4)0, &sys_err );

	if ( status != LK_NEW_LOCK)
	{
	    if ((status == LK_BUSY) || (status == OK))
	    {
		/*
		 ** we can't obtain the lock or already have it
		 ** hence try with next id
		 */
		TRdisplay("%@ Temporary %s creation - clash on id (%d,%d) with status = %d\n",
			  (index) ? "index" : "table",
			  new_tab_id->db_tab_base,
			  new_tab_id->db_tab_index,
			  status);
		continue;
	    }

	    if (status == LK_NOLOCKS)
	    {
		uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err,
			   ULE_LOG, NULL, NULL, 0, NULL, &local_error, 2, 9,
			   "TEMPORARY",
			   sizeof(DB_DB_NAME),
			   &dcb->dcb_name);
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		break;
	    }

	    uleFormat( NULL, status, &sys_err, ULE_LOG , NULL, (char *)NULL, 0L,
		       (i4 *)NULL, &local_error, 0);
	    uleFormat( NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG ,
		       NULL, (char *)NULL, 0L, (i4 *)NULL,
		       &local_error, 2, 0, LK_X, 0, lock_list);

	    if (status == LK_DEADLOCK)
	    {
		SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		break;
	    }
	    else
	    {
		SETDBERR(dberr, 0, E_DM0077_BAD_TABLE_CREATE);
		break;
	    }
	} 
	else
	    status = OK;
	break;
    }

    return(status);
}

/*{
** Name: dm2uGetTempTableLocs	- Figure out Temp Table/Index locations.
**
** Description:
**      This routine is called from dmt_create_temp() and 
**	CreateTempIndexTCB() to resolve the location    
**	information for Temporary Base tables and Indexes.
**
** Inputs:
**	dcb				The database.
**	scb				Session control block.
**	locs				Input list of locations.
**	loc_count			Number of locations.
** Outputs:
**	locs				Possibly new set of locations
**	loc_count			and their number.
**	locs_mem			New location memory, if allocated.
**      err_code                        The reason for an error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    May allocate locs_mem, which must be freed by caller.
**
** History:
**	14-Aug-2006 (jonj)
**	    Extracted from dmt_create_temp() for Temp Base and Index tables.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	25-Jun-2007 (kschendel) b118561
**	    Fix above, needed an extra indirection.  One symptom was
**	    sporadic E_US07FD errors due to page_type being smashed in
**	    a64_lnx callers.
*/
DB_STATUS
dm2uGetTempTableLocs(
DMP_DCB		*dcb,
DML_SCB		*scb,
DB_LOC_NAME	**locs,
i4		*loc_count,
DM_OBJECT	**locs_mem,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    DB_LOC_NAME *location = *locs;
    i4		lcount = *loc_count;
    i4		local_error;
    bool	ValidLoc;
    i4		i, j, k;
    i4		error;

    location = *locs;
    lcount = *loc_count;
    *locs_mem = (DM_OBJECT*)NULL;

    CLRDBERR(dberr);
    
    for (k = 0; k < lcount; k++)
    {
	/*  Check location, must be default or valid location.
	**
	**  If there's only one location and it's "$default" then we
	**  use work locations instead.
	*/

	if ( lcount == 1 &&
	     MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&location[k], 
				    sizeof(DB_LOC_NAME)) == 0 )
	{
	    i4		avail_locs, num_locs;
	    i4		next_bit = -1;

	    /* If '$default' was specified, this means we use work
	    ** locations instead of the locations passed into this routine.
	    **
	    ** First find how many work locations are available for use...
	    */
	    avail_locs = BTcount((char *)scb->scb_loc_masks->locm_w_use,
				    scb->scb_loc_masks->locm_bits);

	    if (avail_locs < 1)
	    {
		SETDBERR(dberr, 0, E_DM013E_TOO_FEW_LOCS);
		break;
	    }
	    if (avail_locs > DM_WRKLOC_MAX)
	    {
		SETDBERR(dberr, 0, E_DM013F_TOO_MANY_LOCS);
		break;
	    }
	    num_locs = avail_locs;

	    /* We now know how many locs to use; allocate an array of
	    ** locations.
	    */
	    if ( status = dm0m_allocate(sizeof(DMP_MISC) +
		    sizeof(DB_LOC_NAME) * num_locs, 0, (i4)MISC_CB,
		    (i4)MISC_ASCII_ID, (char *)scb,
		    (DM_OBJECT **)locs_mem, dberr) )
	    {
		break;
	    }
	    location = (DB_LOC_NAME *)((char *)*locs_mem + sizeof(DMP_MISC));
	    ((DMP_MISC*)(*locs_mem))->misc_data = (char*)location;
	    *locs = location;
	    *loc_count = num_locs;

	    /* Finally, we copy locations out of the dcb into this array
	    ** using the "spill index" to decide where to begin copying.
	    */

	    /* point to the first work location to use */
	    for (i = 0; i < dcb->dcb_wrkspill_idx; i++)
	    {
		next_bit = BTnext(next_bit,
				(char *)scb->scb_loc_masks->locm_w_use,
				scb->scb_loc_masks->locm_bits);
	    }

	    /* Do a non-semaphored update of the dcb spill index (the 
	    ** round-robin pointer that keeps track of which work location
	    ** to use next).
	    */
	    dcb->dcb_wrkspill_idx =
			(dcb->dcb_wrkspill_idx + num_locs) % avail_locs;

	    for (j = 0; j < num_locs; j++)
	    {
		/* Note: with BTnext we can easily wrap-around in the bit
		**    stream because it returns a -1 when it hits the end
		*/
		while ((next_bit = BTnext(next_bit,
				    (char *)scb->scb_loc_masks->locm_w_use,
				    scb->scb_loc_masks->locm_bits)) == -1 )
		{
		    continue;
		}

		MEcopy((PTR)&dcb->dcb_ext->ext_entry[next_bit].logical,
		       sizeof(DB_LOC_NAME), (PTR)&location[j]);
	    }
	}
	else
	{
	    /* Find this location in DCB, check allowed types */
	    ValidLoc = FALSE;
	    for (i = 0; i < dcb->dcb_ext->ext_count; i++)
	    {
		if ( (MEcmp((PTR)&location[k], 
			  (PTR)&dcb->dcb_ext->ext_entry[i].logical, 
			  sizeof(DB_LOC_NAME))) == 0 &&
		      dcb->dcb_ext->ext_entry[i].flags & 
				    (DCB_DATA|DCB_WORK|DCB_AWORK) )
		{
		    ValidLoc = TRUE;
		    break;
		}
	    }
	    if ( !ValidLoc )
	    {
		SETDBERR(dberr, 0, E_DM001D_BAD_LOCATION_NAME);
		break;
	    }
	}
    }

    /* If any problem, log it, free allocated memory, if any */
    if ( dberr->err_code )
    {
	uleFormat( dberr, 0, NULL, ULE_LOG , NULL, 
		    (char *)NULL, 0L, (i4 *)NULL, 
		    &local_error, 0);
	if ( *locs_mem )
	    dm0m_deallocate(locs_mem);
	return(E_DB_ERROR);
    }
    else
	return(E_DB_OK);
}

/*
** Name: dm2uInitTpcbMct -- Initialize MCT in a target CB
**
** Description:
**	This routine initializes the MCT that is part of a target CB (TPCB),
**	using the info that is in the overall modify context MXCB.
**
** Inputs:
**	tp			Pointer to TPCB containing MCT to set up
**
** Outputs:
**	Returns E_DB_xxx status
**
** History:
**	24-Feb-2008 (kschendel) SIR 122739
**	    Extract from common code in modify and parallel index;
**	    part of row-accessor restructuring.  (Trying to get a handle on
**	    how to best fill in the rowaccessors in the MCT!)
*/

DB_STATUS
dm2uInitTpcbMct(DM2U_TPCB *tp)
{
    DB_ERROR local_dberr;
    DB_STATUS status;
    DM2U_MXCB *m;		/* Modify control block describing result */
    DM2U_M_CONTEXT *mct;	/* MCT being set up */
    DMP_TCB *t;			/* TCB for source table */

    m = tp->tpcb_mxcb;
    mct = &tp->tpcb_mct;
    t = m->mx_rcb->rcb_tcb_ptr;

    /* Note this is RCB of Master, if Partitioned table */
    mct->mct_oldrcb = m->mx_rcb;

    mct->mct_crecord = tp->tpcb_crecord;
    if (m->mx_structure == TCB_ISAM)
	mct->mct_segbuf = m->mx_rcb->rcb_record_ptr;
    else
	mct->mct_segbuf = NULL;
    mct->mct_buffer = NULL;
    mct->mct_index_comp = m->mx_index_comp;
    mct->mct_d_fill = m->mx_dfill;
    mct->mct_i_fill = m->mx_ifill;
    mct->mct_l_fill = m->mx_lfill;
    mct->mct_buckets = m->mx_buckets;
    mct->mct_relpages = 0;
    mct->mct_keys = m->mx_ai_count;
    mct->mct_klen = m->mx_kwidth;	/* Leaf or rtree width */
    mct->mct_clustered = m->mx_clustered;
    mct->mct_relwid = m->mx_width;
    mct->mct_rebuild = TRUE;
    mct->mct_recreate = TRUE;
    mct->mct_unique = m->mx_unique;
    mct->mct_override_alloc = 0;
    /* Slice allocation, if any, by number of targets (partitions).
    ** Round up if multi location.
    */
    mct->mct_allocation = m->mx_allocation / m->mx_tpcb_count;
    if (tp->tpcb_loc_count > 1)
	dm2f_round_up(&mct->mct_allocation);
    /* Doesn't seem to be a define for minimum allocation? */
    if (mct->mct_allocation < 4)
	mct->mct_allocation = 0;	/* In case we sliced it away */
    mct->mct_extend = m->mx_extend;
    mct->mct_page_type = m->mx_page_type;
    mct->mct_page_size = m->mx_page_size;
    if ( m->mx_width > dm2u_maxreclen(m->mx_page_type, m->mx_page_size))
	mct->mct_seg_rows = TRUE;
    else
	mct->mct_seg_rows = FALSE;
    mct->mct_ver_number = 0;
    mct->mct_inmemory = m->mx_inmemory;
    mct->mct_buildrcb = m->mx_buildrcb;
    if (m->mx_structure == TCB_RTREE)
    {
	DB_ERROR local_dberr;

        mct->mct_hilbertsize = m->mx_hilbertsize;
        mct->mct_acc_klv = m->mx_acc_klv;
        if (m->mx_acc_klv->range_type == 'I')
	    dm2u_bbox_range(m, m->mx_rcb->rcb_adf_cb, &local_dberr);
    }
    else
    {
        mct->mct_hilbertsize = 0;
        mct->mct_acc_klv = NULL;
    }

    /* Initialise the page accessors in the MCT */
    dm1c_get_plv(m->mx_page_type, &mct->mct_acc_plv);

    /* Initialize the location pointers. */
    mct->mct_location = tp->tpcb_locations;
    mct->mct_loc_count = tp->tpcb_loc_count;

    if (m->mx_structure == TCB_HASH)
    {
	/*
	** Optimise the allocation, if hash buckets is greater
	** than mct_allocation. Factor in the FHDR and any FMAPS
	*/
	if ( mct->mct_allocation < mct->mct_buckets )
	{
	    mct->mct_override_alloc = mct->mct_buckets + 1  +
		((mct->mct_buckets/
		DM1P_FSEG_MACRO(mct->mct_page_type, mct->mct_page_size)) + 1);

	    /*
	    ** For multi-location tables round accordingly
	    */
	    if ( mct->mct_loc_count > 1 )
		dm2f_round_up( &mct->mct_override_alloc );
	}
    }

    /* Caller has set proper build TidSize in mx_tidsize */
    mct->mct_partno = tp->tpcb_partno;
    mct->mct_tidsize = m->mx_tidsize;

    /* We can set up the data rowaccessor here.  The index (and leaf if
    ** btree) will wait for the load "begin" routine, since for btree
    ** especially we fool with the attribute arrays some more even though
    ** dm2u has done plenty of that already!
    **
    ** (A basic issue here is that table load, dm2r_load, does not use the
    ** MXCB and hence can't take advantage of the dm2u stuff.  So we have
    ** to keep the rowaccessors in the MCT until some additional
    ** data structure refactoring can be done.)
    **
    ** All of the target MCT's for a result table point at the identical
    ** control array memory.  Fortunately they will all construct the
    ** identical control array as a result of setup.  This is stupid but
    ** eventually we'll get the rowaccessors factored out better.
    */

    STRUCT_ASSIGN_MACRO(m->mx_data_rac, mct->mct_data_rac);
    status = dm1c_rowaccess_setup(&mct->mct_data_rac);
    if (status != E_DB_OK)
	return (status);

    STRUCT_ASSIGN_MACRO(m->mx_leaf_rac, mct->mct_leaf_rac);
    STRUCT_ASSIGN_MACRO(m->mx_index_rac, mct->mct_index_rac);
    /* Not to be setup until bbegin time */

    /*
    **	Pick the correct key list for the build routines. When index
    **	compression is in use, the build routines need the attributes
    **	for the index entry -- for a primary table, this is the
    **	set of 'key' attributes, whereas for a secondary index,
    **	this is the full set of attributes.
    */

    mct->mct_index = ((t->tcb_rel.relstat & TCB_INDEX) != 0)
			|| (m->mx_operation == MX_INDEX);
    if (mct->mct_index)
	mct->mct_key_atts = m->mx_i_key_atts;
    else
	mct->mct_key_atts = m->mx_b_key_atts;

    /* Don't let anyone use these until a bbegin routine sets them up! */
    mct->mct_index_maxentry = MINI4;
    mct->mct_leaf_maxentry = MINI4;

    return (E_DB_OK);
} /* dm2uInitTpcbMct */
