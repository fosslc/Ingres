/*
** Copyright (c) 1985, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <tm.h>
#include    <scf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmrcb.h>
#include    <dmacb.h>
#include    <lg.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm1p.h>
#include    <dma.h>
#include    <dmppn.h>

/**
** Name:  DMTSHOW.C - Implement the DMF show table operation.
**
** Description:
**      The file contains the function that implements the DMF
**      show table operation.
**      This file defines:
**
**      dmt_show - Show table information.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	20-mar-1989 (rogerk)
**	    Changed LKshow to pass lock list id in the 2nd ('llid') parameter
**	    rather than the lockid parm.
**	23-apr-1990 (rogerk)
**	    Added missing 'break' statement after HASH case when filling
**	    in page count information.
**	2-may-90 (bryanp)
**	    Enhanced the support for tables with missing underlying physical
**	    files so that it now detects the case where the base table is OK
**	    but the underlying data file for one of the secondary index tables
**	    is missing.
**	11-jul-90 (rogerk)
**	    Put in some sanity checking into page and tuple counts.
**	12-jul-1990 (walt, bryanp)
**	    When queueing the dm2t_control lock request on the session lock
**	    list, temporarily alter the session lock list to be interruptible.
**	    This way, if someone else is holding the lock and holds it for a
**	    LONG time (e.g., does a modify and never commits), this poor user
**	    who is hung in the lock wait can at least interrupt his wait.
**	14-jul-90 (teresa)
**	    fixed bug where number of index pages for ISAM was miscalculated
**	    by 1.  The correct equation is relprim - relpages + 1.
**	16-oct-1990 (bryanp)
**	    Bug 33809: when showing index information, OPF expects that
**	    idx_key_count does NOT include the TIDP, even if the TIDP is part
**	    of the key. Thus we must subtract 1 from idx_key_count when the
**	    TIDP is part of the key.
**	21-jan-1991 (rogerk)
**	    Added support for specifying lock timeout value for the
**	    table control lock.  Added dmt_char_array field to show cb
**	    to pass in show options.  Added checks for user set lockmode
**	    settings as well.  Added check_char and check_setlock routines.
**      10-oct-1991 (jnash) merged 08-mar-1990 (teg)
**          Return E_DM013A_INDEX_COUNT_MISMATCH if requester's index count does
**          not match the index count in the TCB.  It used to return
**          E_DM002A_BAD_PARAMETER, which was too obscure for RDF error recovery
**          to manage.
**      10-oct-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: support by-name lookup of session-
**	    specific temporary tables by calling dm2t_locate_tcb.
**	28-feb-1992 (bryanp)
**	    Greatly simplified the whole table control lock mess by depending
**	    on some additional functionality in dm2t_control.
**	7-jul-1992 (bryanp)
**	    Prototyping DMF.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**    	17-mar-1992 (kwatts)
**          Added setting of new MPF fields tbl_comp_type and tbl_pg_type
**	29-August-1992 (rmuth)
**	    Add calculating the number of overflow pages and returning it as
**	    part of DMT_TBL_ENTRY.
**	18-sep-1992 (ananth)
**	    Expose the relstat2 field of DMP_RELATION.
**	16-oct-1992 (rickh)
**	    Fill in attribute defaults.
**	26-oct-1992 (rogerk)
**	    Changes for reduced logging project to support new dm2t protocols.
**	    Changed dm2t_find_tcb calls to dm2t_fix_tcb, dm2t_locate_tcb
**	    to dm2t_lookup_tabid and changed error handling logic.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	30-October-1992 (rmuth)
**	    Alter the overflow page count calculation for isam and
**	    the data page calculation for btrees.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	30-November-1992 (rmuth)
**	    File Extend: Set tbl_alloc_pg_count.
**	14-dec-1992 (rogerk)
**	    Pass NOPARTIAL flag when fixing tcb to avoid picking up partial
**	    tcb built by recovery operations.
**	18-dec-1992 (robf)
**	    Remove dm0a.h
**	12-feb-93 (rickh)
**	    Added tbl_allocation and tbl_extend.
**	23-feb-93 (rickh)
**	    Exposing relstat2 involves translating TCB bits into DMT bits,
**	    not just a copy of relstat2 into tbl_2_status_mask.
**	30-feb-1993 (rmuth)
**	   Change what is returned in the DMT_IDX_ENTRY structure as follows:
**	      - The idx_dpage_count and idx_ipage_count are not currently
**	        set to meaning values, i.e. they do not value the rules used
**	        for setting tbl_dpage_count and tbl_ipage_count. Checking
**	        with the OPF people these values are never used. As they are
**	        currently set using an unitialised variable causing problems
**	        on some machines just zero them out.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**		Correct cast in assignment to att_default.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	28-may-93 (robf)
**	    Secure 2.0: Rework ORANGE code
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rmuth)
**	    - Setting HEAP overflow page count ot zero, in 6.4 this value
**	      is number of data pages - 1, for backward compatiblity use
**	      this calculation.
**	    - Return iirelation.relstat2 information on READONLY tables
**	      and concurrent Indicies in the tbl_2_status_mask.
**	31-jan-1994 (bryanp) B58539
**	    Format scf_error.err_code if scf_call fails.
**      10-nov-1994 (cohmi01) B48713 B45905   Complete Rogerk's 1-91 fix for
**          timeout of cntrl lock by passing timeout as parm to dm2t_control().
**      15-feb-95 (inkdo01)
**          transfer logical/physical inconsistency flags from iirelation
**          image to RDFINFO structure (rdr_rel)
**	17-may-1995 (shero03)
**	    Support Rtree
**	7-jun-1995 (lewda02/thaju02)
**	    RAAT API Project:
**	    Enable dmt_show to place attribute and index information
**	    directly into (multiple) communication buffers. 
**	29-dec-1995 (nick)
**	    Use the transaction locklist rather than the session one when
**	    fixing a TCB. #73305
**	10-jan-96 (allst01)
**	    Fixed various memory leak bugs. These seriously affected
**	    B1 secure systems, to the tune of 1.6M per createdb !
**      06-mar-1996 (stial01, nanpr01)
**          Variable Page Size project: Init tbl_pgsize, index page size
**	14-jun-1996 (shero03)
**	    RTree: init tbl_dimension, hilbertsize, and range
**	 4-jul-96 (nick)
**	    Ensure we share mode lock iirelidx and iirelation. #77557
**	08-jul-96 (toumi01)
**	    Modified to support axp.osf (64-bit Digital Unix).
**	    For RAAT facility use MEcopy to avoid bus errors moving
**	    unaligned data.
**      15-jul-1996 (ramra01)
**          Alter Table Project:
**              Column attr information update-# of atts, bld attr array.
**	28-nov-96 (pchang)
**	    Fixed a typo in the test for TCB_VIEW and TCB_GATEWAY flag which 
**	    caused tbl_loc_count to be set to 1 instead of 0 for views and
**	    gateway tables - changed 'relspec' to 'relstat'. 
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - removed check_setlock() routine;
**          - replaced check_setlock() calls with the code to match the 
**          new way of storing and setting of locking characteristics.
** 	19-mar-97 (consi01 bug 81094)
**	    Changed processing of DMT_M_LOC flag so that all location names
**	    are copied back to the target array of dmt_show_cb. Prior to 
**	    this change the first location was not copied to the array.
**	25-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added tbl_cachepri to DMT_TBL_ENTRY.
**      09-apr-97 (cohmi01)
**          Allow timeout system default to be configured. #74045, #8616.
**      16-may-97 (dilma04)
**          Bug 81864. Removed check for existence of SLCB before obtaining 
**          session level timeout value. 
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_unfix_tcb() calls.
**	03-Dec-1998 (jenjo02)
**	    Collect real-time page and tuple counts from TCB and associated
**	    RCBs.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**	23-Dec-1998 (jenjo02)
**	    Mutex the TCB->RCB list when reading it.
**	23-dec-1998 (somsa01)
**	    If we are executing this against a temporary table, then there is
**	    no need to obtain a control lock on the table (since ONLY the
**	    current session can use it). Also, if we are passing in a
**	    temporary table id rather than a table name, set found_in_memory.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	05-nov-1999 (nanpr01)
**	    Pass the page and row estimate correctly. This change is temporary
**	    and will be superseded by Alison's change.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      22-Nov-1999 (hweho01)
**          Used MEcopy to copy pointer for ris_u64 (AIX 64-bit platform).
**	24-Aug-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) to list of 64-bit platforms using
**	    MECopy to copy pointer.
**      12-oct-2000 (stial01)
**          Init new fields in DMT_TBL_ENTRY to be used by optimizer (opf)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**	08-Jan-2001 (jenjo02)
**	    Get DML_SCB* from ODCB instead of SCU_INFORMATION.
**	26-jul-2001 (stephenb)
**	    Add LP64 to platforms using MEcopy to assign pointers. FIX_ME
**	    we really should define LP64 on the other 64-bit platforms in this
**	    list, and not pollute the generic code with platform specifics.
**	12-Oct-2001 (somsa01)
**	    Added NT_IA64 to the list of 64-bit platforms needing alignment.
**      08-mar-2002 (stial01)
**          Don't get control lock on Temp tables when DMT_M_ACCESS_ID (rcb) 
**          specified (b107371)
**      24-may-2002 (chash01)
**          B107805 - issue 11851769.
**          Register a rms gateway table then do a "help table" results
**          in -ve in overflow page count.  Test for < 0 and set opage_count
**          to 0 if true eliminates this problem. change in dmt_show(). 
**	14-aug-2003 (jenjo02)
**	    dm2t_unfix_tcb() "tcb" parm now pointer-to-pointer,
**	    obliterated by unfix to prevent multiple unfixes
**	    by the same transaction and corruption of 
**	    tcb_ref_count.
**	02-oct-2003 (somsa01)
**	    Added NT_AMD64 to the list of 64-bit platforms needing alignment.
**	5-Jan-2004 (schka24)
**	    Return partitioning info from tcb.
**	17-Feb-2004 (schka24)
**	    Forgot to add up the partition row/page counts for the
**	    master -- fix.  Allow show of a partition by fixing the
**	    master too;  this will not be needed except in rare cases.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	15-Mar-2005 (schka24)
**	    Fix the TCB show-only.  This allows RDF as well as dmf_tbl_info
**	    (ie iitables view) to see info for tables even if the cache
**	    for that table pagesize is not available.
**     02-nov-2006 (horda03) Bug 117024
**          The locks taken by dmt_show() should be held in the TX lock list
**          if this lock list exists. Otherwise an undetected deadlock could
**          occur if dmt_show() request a lock that the session already holds
**          in its TX lock list.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to manage DB_ERROR error source.
**	    Use new form of uleFormat().
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
**  Definition of static variables and forward static functions.
*/

    /* check for caller supplied lock characteristics. */
static STATUS check_char(
    DMT_SHW_CB	*dmt_cb,
    i4	*timeout);


/*{
** Name: dmt_show - Show information about a table
**
**  INTERNAL DMF call format:  status = dmt_show(&dmt_shw_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_SHOW, &dmt_shw_cb);
**
** Description:
**      The show table operation will return you the table information
**      the attribute information and the index information for an opened
**      or unopened table.  Any combination of the above items can be
**      requested.  It is expected that the show command will be used in
**      place of direct queries on the system tables.  This must be
**      called prior to any other table operations to determine the internal
**      table identifier required as input for these operations.
**
**	The default is to lookup a table by table identifier.  Optionally a
**	table can be lookup by name+owner using the DMT_M_NAME flag.  Also,
**	a table that is already opened can use show with the DMT_M_ACCESS_ID
**	flag to return information about the already open table.
**
**	The attributes are returned in the same order that they were specified
**	at the time that the table was created.
**
**	Show requires a short-term shared Table Control Lock.  This prevents
**	show from running concurrently with a session that is modifying the
**	information SHOW needs to collect.  Show may be blocked by another
**	session doing a modify/index/create/destroy.  A timeout value may
**	be defined for the table control lock through the dmt_char_array
**	interface, or through entries in the sessions lockmode settings list.
**
**	If this session already has a transaction lock list, then the table
**	control lock is acquired on the transaction lock list (it may already
**	be held there exclusively if the transaction has previously performed
**	a modify/index/create/destroy). When releasing the lock list, we
**	count on dm2t_control to "do the right thing" and not totally release
**	the lock if we're currently holding it exclusively (our shared request
**	has been quietly promoted to an exclusive request if we already held
**	the lock exclusively).
**
**	NOTE: users of DMT_M_INDEX should be aware that the idx_key_count field
**	in the DMT_INDEX_ENTRY is overloaded:
**	    1) For HASH indexes, it is the total number of key columns.
**	    2) For Btree and Isam indexes where the TIDP is NOT part of the
**		the key (obtainable only through convto60), it is the total
**		number of key columns.
**	    3) For Btree and Isam indexes where the TIDP IS part of the key,
**		it is ONE LESS than the total number of key columns.
**	This overloading is unfortunate; however, at this point it's
**	grandfathered in.
**
**	When the DMT_M_MULTIBUF flag is set, dmt_show will place attribute and
**	index information into a linked list of buffers.  This is useful in the
**	case when the data buffers will be passed directly to GCA, which has
**	a limited packet size and requires the data to be spread across multiple
**	buffers.  NOTE:  The ptr_size field of the DM_PTR structures used for
**	dmt_attr_array and dmt_index_array is overloaded when the DMT_M_MULTIBUF
**	flag is used.  It takes the value of the pointer to the next DM_PTR
**	structure in the linked list.
**
**	You don't normally dmt-show physical partitions.  Do the show on
**	the partitioned master, and ask for the DMT_M_PHYPART info which
**	will return the physical-partition array.  It is possible to
**	dmt-show a partition, and consistent information ought to come
**	back;  but this is typically useful only in special circumstances,
**	such as the table-info function callback.  Since partition
**	table names are deliberately non-identifiers, users will have
**	to work to show a partition from a normal query, and the parse
**	will end up rejecting the name anyway.
**
** Inputs:
**      dmt_shw_cb
**          .type                           Must be set to DMT_SH_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_SHW_CB) bytes.
**	    .dmt_session_id		    Session identifier obtained from
**					    SCF.
**          .dmt_db_id                      Database to use.
**          .dmt_tab_id                     Internal table name to show about.
**          .dmt_name                       External table name.
**          .dmt_owner                      External owner name.
**	    .dmt_access_id		    Record access identifier.
**          .dmt_flags_mask                 A mask identifing the information to
**                                          be returned.  Any combination of:
**                                          DMT_M_TABLE, DMT_M_ATTR,
**                                          DMT_M_INDEX and DMT_M_BYNAME or 
**                                          DMT_M_ACCESS_ID.
**					    Also DMT_M_PHYPART for partitions;
**					    DMT_M_COUNT_UPD for counts.
**          .dmt_table.data_address	    Pointer to location to return 
**                                          object of
**					    type DMT_TBL_ENTRY.
**          .dmt_table.data_in_size	    The size of the area in bytes.  
**                                          Must be at
**					    least sizeof(DMT_TBL_ENTRY) bytes.
**          .dmt_attr_array.ptr_address	    Pointer to array of pointer.
**          .dmt_attr_array.ptr_in_count    The number of pointers in the array.
**	    .dmt_attr_array.ptr_size	    The size of each element pointed at
**					    by an array pointer.  Must be at 
**                                          least sizeof(DMT_ATT_ENTRY).
**          .dmt_index_array.ptr_address    Pointer to array of pointers.
**          .dmt_index_array.ptr_in_count   The number of pointers in the array.
**	    .dmt_index_array.ptr_size	    The size of each element pointed 
**                                          at by an
**					    array pointer.  Must be at least
**					    sizeof(DMT_IDX_ENTRY).
**          .dmt_loc_array.ptr_address      Pointer to array of pointers.
**          .dmt_loc_array.ptr_in_count     The number of pointers in the array.
**	    .dmt_loc_array.ptr_size	    The size of each element pointed 
**                                          at by an
**					    array pointer.  Must be at least
**					    sizeof(DMT_LOC_ENTRY).
**          .dmt_char_array.data_address    Optional pointer to characteristic
**                                          array of type DMT_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmt_char_array> entries.
**          .dmt_char_array.data_in_size    Length of dmt_char_array in bytes.
**
**          <dmt_char_array> entries of type DMT_CHAR_ARRAY and
**          must have the following format:
**              .char_id                    The characteristics identifer
**					    denoting the characteristics to
**					    set.  Current supported options:
**					    DMT_TIMEOUT_LOCK - timeout value
**						for table control lock request.
**						Default is to wait forever.
**              .char_value                 The value to use to set this 
**                                          characteristic.
** Outputs:
**
**      dmt_shw_cb
**          .dmt_table.data_address         Pointer to area used to return
**                                          an entry of type DMT_TABLE_ENTRY.
**          .dmt_attr_array.ptr_out_count   The number of pointers that were 
**                                          used
**					    to return DMT_ATT_ENTRY's.
**          .dmt_index_array.ptr_out_size   The number of pointers that were 
**                                          used
**					    to return DMT_IDX_ENTRY's.
**                                          in the area.
**	    .dmt_phypart_array.data_address DMT_PHYS_PART array returned here.
**	    .dmt_phypart_array.data_out_size  Size of same.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**					    E_DM004D_LOCK_TIMER_EXPIRED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**                                          E_DM0100_DB_INCONSISTENT
**					    E_DM010B_ERROR_SHOWING_TABLE
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**					    E_DM0114_FILE_NOT_FOUND
**                                          E_DM013A_INDEX_COUNT_MISMATCH
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmt_shw_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmt_shw_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_shw_cb.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	23-oct-88 (teg)
**	    Added logic to perform show operation even if table does not have
**	    underlaying data file.  However, in that case, will return warning.
**	    (for bug 2344)
**	16-dec-1988 (rogerk)
**	    Fix Unix compile warnings.
**	20-mar-1989 (rogerk)
**	    Changed LKshow to pass lock list id in the 2nd ('llid') parameter
**	    rather than the lockid parm.
**	23-apr-1990 (rogerk)
**	    Added missing 'break' statement after HASH case when filling
**	    in page count information.
**	25-apr-90 (linda)
**	    Added tbl_relgwid and tbl_relgwother to the information returned
**	    by dmt_show() for a table, for non-SQL gateway support.
**	2-may-90 (bryanp)
**	    If a standard Ingres table (i.e., not a VIEW or GATEWAY) is missing
**	    its underlying data file, this is a serious error. Instead of
**	    attempting to treat this as a warning, we treat it as an error.
**	    Furthermore, we also check every secondary index table (if this
**	    is a base table) to see that they too have underlying physical
**	    files.
**	11-jul-90 (rogerk)
**	    Put in checking to make sure that sane (non-negative) values are
**	    returned for btree page estimates.  We make these calculations
**	    based on the number of tuples and if this estimate is way off,
**	    it can cause us to estimate page counts outside the realm of
**	    the real universe.  Also sanity checks for page/tuple counts
**	    of other storage structures.
**	12-jul-1990 (walt, bryanp)
**	    When queueing the dm2t_control lock request on the session lock
**	    list, temporarily alter the session lock list to be interruptible.
**	    This way, if someone else is holding the lock and holds it for a
**	    LONG time (e.g., does a modify and never commits), this poor user
**	    who is hung in the lock wait can at least interrupt his wait.
**	16-oct-1990 (bryanp)
**	    Bug 33809: when showing index information, OPF expects that
**	    idx_key_count does NOT include the TIDP, even if the TIDP is part
**	    of the key. Thus we must subtract 1 from idx_key_count when the
**	    TIDP is part of the key.
**	17-dec-1990 (jas)
**	    Smart Disk project integration:
**	    When showing table information, return the Smart Disk type
**	    for the table.
**	21-jan-1991 (rogerk)
**	    Added support for specifying lock timeout value for the
**	    table control lock.  Added dmt_char_array field to show cb
**	    to pass in show options.  Added checks for user set lockmode
**	    settings as well.
**      10-oct-1991 (jnash) merged 08-mar-1990 (teg)
**          Return E_DM013A_INDEX_COUNT_MISMATCH if requester's index count does
**          not match the index count in the TCB.  It used to return
**          E_DM002A_BAD_PARAMETER, which was too obscure for RDF error recovery
**          to manage.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements: support by-name lookup of temporary
**	    tables. If we can't find the table by name in iirel_idx, then
**	    perhaps it's a session-specific temporary table (which isn't
**	    cataloged, and exists only in the in-memory TCB's). Call
**	    dm2t_locate_tcb() to find the table and return its table id.
**	    Set tbl_temporary in the DMT_TBL_ENTRY appropriately.
**	28-feb-1992 (bryanp)
**	    Greatly simplified the whole table control lock mess by depending
**	    on some additional functionality in dm2t_control.
**	29-August-1992 (rmuth)
**	    Add calculating the number of overflow pages and returning it as
**	    part of DMT_TBL_ENTRY.
**	18-sep-1992 (ananth)
**	    Expose the relstat2 field of DMP_RELATION.
**	    Copy the relstat2 into the tbl_2_status_mask field of DMT_TBL_ENTRY.
**	16-oct-1992 (rickh)
**	    Fill in attribute defaults.
**	26-oct-1992 (rogerk)
**	    Changed dm2t_find_tcb call to new dm2t_fix_tcb.  Now dm2t_unfix_tcb
**	    must be called when finished rather than the old dm2t_close or
**	    dm2r_release_rcb calls.  Made corresponding change to ORANGE
**	    code to allocate an rcb for the dm1a_compare call.
**	    Changed tcb fields that refer to file access information to now
**	    be stored the tcb_table_io portion of the tcb.
**	    Eliminated the use the DM2T_SHOW_READ open option and the checks
**	    for a returned tcb without any underlying files.  This type of
**	    open is no longer supported.
**	    The dm2t_locate_tcb routine was changed to dm2t_lookup_tabid.
**	    Also changed error logic to format internal errors in the error
**	    handling code at the end of the routine rather than as they are
**	    encountered.
**	30-October-1992 (rmuth)
**	    Alter the overflow page count calculation for isam and
**	    the data page calculation for btrees.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	30-November-1992 (rmuth)
**	    File Extend: Set tbl_alloc_pg_count.
**	14-dec-1992 (rogerk)
**	    Pass NOPARTIAL flag when fixing tcb to avoid picking up partial
**	    tcb built by recovery operations.
**	12-feb-93 (rickh)
**	    Added tbl_allocation and tbl_extend.
**	23-feb-93 (rickh)
**	    Exposing relstat2 involves translating TCB bits into DMT bits,
**	    not just a copy of relstat2 into tbl_2_status_mask.
**	30-feb-1993 (rmuth)
**	   Change what is returned in the DMT_IDX_ENTRY structure as follows:
**	      - The idx_dpage_count and idx_ipage_count are not currently
**	        set to meaning values, i.e. they do not value the rules used
**	        for setting tbl_dpage_count and tbl_ipage_count. Checking
**	        with the OPF people these values are never used. As they are
**	        currently set using an unitialised variable causing problems
**	        on some machines just zero them out.
**	26-apr-1993 (bryanp)
**	    Correct cast in assignment to att_default.
**	28-may-1993 (robf)
**	    Secure 2.0: Rework ORANGE code, pass back table security label
**	    and row label/audit information from relstat2
**	26-jul-1993 (rmuth)
**	    - Setting HEAP overflow page count ot zero, in 6.4 this value
**	      is number of data pages - 1, for backward compatiblity use
**	      this calculation.
**	    - Return iirelation.relstat2 information on READONLY tables
**	      and concurrent Indicies in the tbl_2_status_mask.
**	7-sep-93 (robf)
**	   Don't do MAC on temporary tables
**	31-jan-1994 (bryanp) B58539
**	    Format scf_error.err_code if scf_call fails.
**	16-feb-94 (robf)
**         Return TCB_ALARM status to caller as DMT_ALARM.
**      10-nov-1994 (cohmi01) B48713 B45905   Complete Rogerk's 1-91 fix for
**          timeout of cntrl lock by passing timeout as parm to dm2t_control().
**      15-feb-95 (inkdo01)
**          transfer logical/physical inconsistency flags from iirelation
**          image to RDFINFO structure (rdr_rel)
**      7-jun-1995 (lewda02/thaju02)
**          RAAT API Project:  Enable dmt_show to place attribute and
**          index information directly into (multiple) communication buffers.
**	29-dec-1995 (nick)
**	    Change locklist used in call to dm2t_fix_tcb() to the transaction
**	    lock list rather than the session's.  Fixes #73305.
**	 9-jan-96 (allst01)
**	    Fixed a memory leak problem with createdb.
**	    Must release the rcb after the security check.
**	    Removed the redundant check in the error path, which harks back
**	    to a different set of calls.
**	    Also release the rcb if an iirelidx is found in memory,
**	    it's released if it isn't. This is a minor memory leak.
**	06-mar-1996 (stial01,nanpr01)
**          Variable Page Size project: Init tbl_pgsize, index page size
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Change DM1P_FSEG reference to DM1P_FSEG_MACRO.
**	14-jun-1996 (shero03)
**	    RTree: init tbl_dimension, hilbertsize, and range
**	 4-jul-96 (nick)
**	    Initialise rcb_k_mode for iirelidx and iirelation. #77557
**	15-jul-1996 (ramra01)
**	    Alter Table Project:
**		Column attr information update-# of atts, bld attr array.
**	28-nov-96 (pchang)
**	    Fixed a typo in the test for TCB_VIEW and TCB_GATEWAY flag which 
**	    caused tbl_loc_count to be set to 1 instead of 0 for views and
**	    gateway tables - changed 'relspec' to 'relstat'. 
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Replaced check_setlock() calls with the code to match the
**          new way of storing and setting of locking characteristics.
** 	19-mar-97 (consi01 bug 81094)
**	    Changed processing of DMT_M_LOC flag so that all location names
**	    are copied back to the target array of dmt_show_cb. Prior to 
**	    this change the first location was not copied to the array.
**	25-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added tbl_cachepri to DMT_TBL_ENTRY.
**	09-apr-97 (cohmi01)
**	    Allow timeout system default to be configured. #74045, #8616.
**      16-may-97 (dilma04)
**          Bug 81864. Removed check for existence of SLCB before obtaining 
**          session level timeout value. 
**	8-apr-98 (inkdo01)
**	    Added DMT_I_PERSISTS_OVER_MODIFIES to idx_status for constraint
**	    index with option feature.
**	22-jul-1998 (thaju02)
**	    change DM1P_FSEG to DM1P_FSEG_MACRO.
**	03-Dec-1998 (jenjo02)
**	    Collect real-time page and tuple counts from TCB and associated
**	    RCBs.
**	23-Dec-1998 (jenjo02)
**	    Mutex the TCB->RCB list when reading it.
**	23-dec-1998 (somsa01)
**	    If we are executing this against a temporary table, then there is
**	    no need to obtain a control lock on the table (since ONLY the
**	    current session can use it). Also, if we are passing in a
**	    temporary table id rather than a table name, set found_in_memory.
**	13-jan-1999 (somsa01)
**	    The previous change introduced a check of the
**	    dmt_tab_id.db_tab_base against 0xffff0000. This has been moved
**	    further down, since dmt_tab_id is only valid if DMT_M_NAME is not
**	    set.
**	15-jan-1999 (nanpr01)
**	    Pass ptr to ptr to dm2r_release_rcb routine.
**	2-nov-99 (stephenb)
**	    If we are called for a temp table, we never re-set status, this
**	    may cause strage errors in QEF. Re-set status for this case, if
**	    everything is O.K.
**      17-Apr-2001 (horda03) Bug 104402.
**          Added new attribute "TCB_NOT_UNIQUE" to indicate that a constraint
**          index does not need to be UNIQUE (e.g. for foreign key constraints).
**    28-May-2002 (wanfr01) Bug 107776, INGSRV 1774
**          E_DM002A returned erroneously.  The beginning of dmt_show has
**          a default error code of E_DM002A, and assumes you will later
**          change status to TRUE.  The latter part of dmt_show assumes
**          status is TRUE.  This can result in random E_DM002A errors.
**          Changed initial status to TRUE and change to FALSE only if you
**          need to return an error code.
**      24-may-2002 (chash01)
**          B107805 - issue 11851769.
**          Register a rms gateway table then do a "help table" results
**          in -ve in overflow page count.  Test for < 0 and set opage_count
**          to 0 if true eliminates this problem 
**	9-Jan-2004 (schka24)
**	    Return partitioning info.
**	    Fix potential bug where rel-rcb is not cleared after use.
**	6-Feb-2004 (schka24)
**	    Update dm2f-filename call.
**	11-Mar-2004 (schka24)
**	    The change to return all locations forgot to return the proper
**	    pointer count also, confused the heck out of me!  Return
**	    locations as db-loc-names, nobody cares about the filenames.
**	16-Mar-2004 (schka24)
**	    Remove diagnostic for control lock on tabid 0, RDF bug is fixed.
**	28-Apr-2004 (schka24)
**	    Nobody had asked for just the pp array before, and it gave
**	    a "bad flag" error.  Fix.  Also turns out that second half
**	    of flag check is "FALSE", remove it.
**	14-dec-04 (inkdo01)
**	    Add collation ID column support.
**	18-Jan-2005 (jenjo02)
**	    Replace extremely pricey call to dm2t_lookup_tabid() 
**	    with quick scan of session's ODCB->XCCB TempTable list.
**	17-Aug-2005 (jenjo02)
**	    Delete db_tab_index from DML_SLCB. Lock semantics apply
**	    universally to all underlying indexes/partitions.
**    12-Sep-2005 (hanal04) Bug 114348 INGSRV3264
**          Extend the range of temporary table IDs to all negative
**          values not just the first 65535.
**	30-May-2006 (jenjo02)
**	    Expand the dimension of tcb_ikey_map from DB_MAXKEYS to
**	    DB_MAXIXATTS to support indexes on Clustered tables.
**	15-Aug-2006 (jonj)
**	    Temp Table index support: expand table name search from
**	    Base TCB to its attached indexes.
**      02-nov-2006 (horda03) Bug 117024
**          If the Session has a TX lock list use it to hold the locks.
**	15-May-2007 (kibro01) b118103
**	    For partitioned ISAM table, go through partitions to determine
**	    correct number of overflow pages, since total pages including
**	    all partitions plus overflow is stored in tbl->tbl_page_count
**	13-aug-2007 (dougi)
**	    Add hack to force data type of histogram column of iihistogram
**	    table to DB_BYTE_TYPE.
**	15-oct-2007 (toumi01) FIXME UTF8
**	    Add similar hack for ii_encoded_forms.cfdata.
**	    Also adjust sizeof constant for historgram test.
**	31-oct-2007 (toumi01)
**	    Previous change inadvertently broke histogram override
**	    (from DB_BYTE_TYPE to DB_VBYTE_TYPE). Fix it.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	16-Oct-2008 (gupsh01)
**	    Add to the list of catalogs which are defined as
**	    char/varchar but contain binary data. In a UTF8
**	    installation the data could get corrupted due to
**	    normalization or truncation. Treat such columns as if
**	    they are byte/varbyte columns.
**	29-Sept-2009 (troal01)
**		Add support for geospatial columns.
**	16-Nov-2009 (kschendel) SIR 122890
**	    When searching by name, diverge immediately based on session
**	    temp or not.  Real tables need never look at the session temp
**	    list, and temp tables need never look at iirelation.
**	    Count show's for the DMF shutdown report.
**	    Use proper tran_id for master fix when showing a partition.
*/

DB_STATUS
dmt_show(
DMT_SHW_CB  *dmt_show_cb)
{
    DMT_SHW_CB	    *dmt = dmt_show_cb;
    DML_ODCB	    *odcb;
    DML_SCB	    *scb;
    DMP_RCB	    *rcb = 0;
    DMP_RCB	    *rel_rcb = 0;
    DMP_TCB	    *tcb = 0;
    DMP_TCB	    *master_tcb = NULL;
    DMP_TCB	    *partition_tcb = NULL;
    DMP_DCB	    *dcb;
    DMP_TCB	    *itcb, *ttcb;
    DML_XCCB	    *xccb;
    DMT_TBL_ENTRY   *tbl;
    DMT_ATT_ENTRY   **att;
    DB_LOC_NAME	    **loc;
    DMT_IDX_ENTRY   **idx;
    DB_TAB_ID	    table_id;
    DB_TAB_ID	    master_id;
    DB_TRAN_ID      tran_id;
    DB_TRAN_ID      *tran_id_ptr;
    DM2R_KEY_DESC   qual_list[2];
    DMP_RELATION    relation;
    DMP_RINDEX	    rel_index;
    DM_TID	    tid;
    DB_STATUS	    local_status;
    DB_STATUS	    status;
    i4	    	    error;
    i4		    local_error;
    i4	    i;
    i4	    j;
    i4         k;
    i4	    timeout;
    i4	    attr_drop_count = 0;		
    LK_LLID	    lock_list;
    LG_LXID	    log_id;
    i4		    control_lock = FALSE;
    DML_SLCB        *slcb;
    bool	    session_temp = FALSE;
    bool	    ii_datatype_hack = FALSE;
    bool	    iihistogram = FALSE;
    bool	    iidd_histograms = FALSE;
    bool	    iitree = FALSE;
    bool	    iiddb_tree = FALSE;
    bool	    iidd_ddb_tree = FALSE;
    bool	    ii_stored_bitmaps = FALSE;
    bool	    ii_encoded_forms = FALSE;
    bool	    is_utf8 = FALSE ;
    DB_ERROR	    local_dberr;

    #define CFDATA_COL_POS 5
    #define TREETREE_COL_POS 8 
    #define HISTOGRAM_COL_POS 5 
    #define HIST_TEXT_SEGMENT_POS 5 
    #define II_STORED_BITMAPS_POS 4 

    is_utf8 = CMischarset_utf8();
     
    CLRDBERR(&dmt->error);

    status = E_DB_OK;
    for (;;)
    {
	if ((dmt->dmt_flags_mask & 
              (DMT_M_TABLE|DMT_M_ATTR|DMT_M_INDEX|DMT_M_LOC|DMT_M_COUNT_UPD|DMT_M_PHYPART)) == 0)
	{
	    SETDBERR(&dmt->error, 0, E_DM001A_BAD_FLAG);
            status = E_DB_ERROR;
	    break;
	}

	if ( (odcb = (DML_ODCB *)dmt->dmt_db_id) == 0 ||
	     (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK) )
	{
	    SETDBERR(&dmt->error, 0, E_DM0010_BAD_DB_ID);
            status = E_DB_ERROR;
	    break;
	}

	if ( (scb = odcb->odcb_scb_ptr) == 0 ||
	     (dm0m_check((DM_OBJECT *)scb, (i4)SCB_CB) != E_DB_OK) )
	{
	    SETDBERR(&dmt->error, 0, E_DM002F_BAD_SESSION_ID);
            status = E_DB_ERROR;
	    break;
	}

	/*
	** If this session has a transaction lock list, use that; else use the
	** session lock list. Ditto for log_id.
	*/
	if (scb->scb_x_next != (DML_XCB*)&scb->scb_x_next) 
	{
	    lock_list = scb->scb_x_next->xcb_lk_id;
	    log_id = scb->scb_x_next->xcb_log_id;
            tran_id_ptr = &scb->scb_x_next->xcb_tran_id;
	}
        else
        {
	    lock_list = scb->scb_lock_list;
	    log_id = 0;

	    tran_id.db_high_tran = 0;
	    tran_id.db_low_tran = 0;
	    tran_id_ptr = &tran_id;
        }

	dcb = odcb->odcb_dcb_ptr;

	if (dmt->dmt_flags_mask & DMT_M_ACCESS_ID)
	{
	    rcb = (DMP_RCB *)dmt->dmt_record_access_id;
	    if (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) != E_DB_OK)
	    {
		SETDBERR(&dmt->error, 0, E_DM002B_BAD_RECORD_ID);
		status = E_DB_ERROR;
		break;
	    }
	    STRUCT_ASSIGN_MACRO(rcb->rcb_tcb_ptr->tcb_rel.reltid, table_id);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(dmt->dmt_tab_id, table_id);
	}

	if (((dmt->dmt_flags_mask & DMT_M_TABLE) && 
                    (dmt->dmt_table.data_address == 0 ||
		    dmt->dmt_table.data_in_size < sizeof(DMT_TBL_ENTRY))) ||
	    ((dmt->dmt_flags_mask & DMT_M_ATTR) && 
                    (dmt->dmt_attr_array.ptr_address == 0 ||
		    (dmt->dmt_attr_array.ptr_size < sizeof(DMT_ATT_ENTRY) &&
		        !(dmt->dmt_flags_mask & DMT_M_MULTIBUF)))) ||
	    ((dmt->dmt_flags_mask & DMT_M_LOC) && 
                    (dmt->dmt_loc_array.ptr_address == 0 ||
		    dmt->dmt_loc_array.ptr_size < sizeof(DB_LOC_NAME))) ||
	    ((dmt->dmt_flags_mask & DMT_M_INDEX) && 
                    (dmt->dmt_index_array.ptr_address == 0 ||
		    (dmt->dmt_index_array.ptr_size < sizeof(DMT_IDX_ENTRY) &&
			!(dmt->dmt_flags_mask & DMT_M_MULTIBUF)))))
	{
	    SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	++dmf_svcb->svcb_stat.tbl_show;

	/*  Get TCB by name or by access identifier or by table identifier. */

	if (dmt->dmt_flags_mask & DMT_M_NAME)
	{
	    /* If table is a session temp, it's not in the catalogs, but we
	    ** might find it on the ODBC XCCB temp table list.
	    **
	    ** If the table is not a session temp, it might be in the catalogs,
	    ** but it for sure won't be on the temp table list.
	    ** So, decide now which kind of search to do.
	    */

	    /* This is a prefix test, owner starts with $Sess... */
	    if (MEcmp(DB_SESS_TEMP_OWNER, &dmt->dmt_owner.db_own_name[0],
			sizeof(DB_SESS_TEMP_OWNER)-1) == 0)
	    {
		/* Session temp -- try the temp table list.
		** Also for any temp table with secondary indexes, look
		** at the indexes for a match.
		*/
		if ( odcb->odcb_cq_next != (DML_XCCB*)&odcb->odcb_cq_next )
		{
		    /*
		    ** Look up named temp table on 
		    ** ODCB's XCCB list.
		    ** For that we need a mutex:
		    */
		    dm0s_mlock(&odcb->odcb_cq_mutex);
		    for ( xccb = odcb->odcb_cq_next;
			  xccb != (DML_XCCB*)&odcb->odcb_cq_next;
			  xccb = xccb->xccb_q_next )
		    {
			/* First, match on owner name */
			if ( xccb->xccb_operation == XCCB_T_DESTROY &&
			     xccb->xccb_t_dcb == dcb &&
			     (ttcb = xccb->xccb_t_tcb) &&
			     (ttcb->tcb_status & (TCB_VALID | TCB_BUSY))
				 == TCB_VALID &&
			     MEcmp((char*)&ttcb->tcb_rel.relowner,
				 (char*)&dmt->dmt_owner,
				 sizeof(DB_OWN_NAME)) == 0 )
			{
			    /* Right owner, check the Base table */
			    if ( MEcmp((char*)&ttcb->tcb_rel.relid,
					(char*)&dmt->dmt_name,
					sizeof(DB_TAB_NAME)) == 0 )
			    {
				itcb = ttcb;
				session_temp = TRUE;
				break;
			    }
			    /* Search Base TCB for Temp Index */
			    else for ( itcb = ttcb->tcb_iq_next;
				       itcb != (DMP_TCB*)&ttcb->tcb_iq_next;
				       itcb = itcb->tcb_q_next )
			    {
				if ( MEcmp((char*)&itcb->tcb_rel.relid,
					    (char*)&dmt->dmt_name,
					    sizeof(DB_TAB_NAME)) == 0 )
				{
				    session_temp = TRUE;
				    break;
				}
			    }
			}
		    }
		    if ( session_temp )
		    {
			STRUCT_ASSIGN_MACRO(itcb->tcb_rel.reltid,
						    table_id);
		    }
		    dm0s_munlock(&odcb->odcb_cq_mutex);
		}
		if ( session_temp )
		    status = E_DB_OK;
		else
		{
		    SETDBERR(&dmt->error, 0, E_DM0054_NONEXISTENT_TABLE);
		    status = E_DB_ERROR;
		    break;
		}
	    }
	    else
	    {
		/* Not a session temp, try for real table */
		/* Get the table id by name using the iirel_idx index. */

		status = dm2r_rcb_allocate(
			odcb->odcb_dcb_ptr->dcb_rel_tcb_ptr->tcb_iq_next,
			(DMP_RCB *)0, tran_id_ptr, 
			lock_list, (i4)0,
			&rel_rcb, &dmt->error);
		if (status != E_DB_OK)
		    break;

		rel_rcb->rcb_lk_mode = RCB_K_IS;
		rel_rcb->rcb_k_mode = RCB_K_IS;
		rel_rcb->rcb_access_mode = RCB_A_READ;

		qual_list[0].attr_number = DM_1_RELIDX_KEY;
		qual_list[0].attr_operator = DM2R_EQ;
		qual_list[0].attr_value = (char*) &dmt->dmt_name;
		qual_list[1].attr_number = DM_2_RELIDX_KEY;
		qual_list[1].attr_operator = DM2R_EQ;
		qual_list[1].attr_value = (char *) &dmt->dmt_owner;
		status = dm2r_position(rel_rcb, DM2R_QUAL, qual_list, 
				 (i4)2,
				 (DM_TID *)0, &dmt->error);
		if (status != E_DB_OK)
		    break;

		/*
		** Loop through iirelidx records looking for one which matches
		** the requested table.  The loop will end either with a match
		** or with a NONEXT error (if the table is not found).
		*/
		do
		{
		    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
				    (char *)&rel_index, &dmt->error);
		    if ( status )
		    {
			/* Error, translate eof to not-found */
			if ( dmt->error.err_code == E_DM0055_NONEXT )
			    dmt->error.err_code = E_DM0054_NONEXISTENT_TABLE;
			/* continue to release rcb */
		    }
		    if ( status != E_DB_OK
		      || ( MEcmp((char *)&rel_index.relname,
                                 (char *)&dmt->dmt_name,
                                 sizeof(DB_TAB_NAME)) == 0 &&
                           MEcmp((char *)&rel_index.relowner,
                                 (char *)&dmt->dmt_owner,
                                 sizeof(DB_OWN_NAME)) == 0) )
		    {
			/* we need status & error from get! don't smash */
			(void) dm2r_release_rcb(&rel_rcb, &local_dberr);
			rel_rcb = NULL;
		    }
		} while (rel_rcb != NULL);
		if (status != E_DB_OK)
		    break;

		/*
		** Found iirel_idx record. Retrieve corresponding iirelation
		** record and extract table ID from it:
		*/
		status = 
		  dm2r_rcb_allocate(odcb->odcb_dcb_ptr->dcb_rel_tcb_ptr, 
		    (DMP_RCB *)0,
		    tran_id_ptr, 
		    lock_list, (i4)0,
		    &rel_rcb, &dmt->error);
		if (status != E_DB_OK)
		    break;

		/*  Initialize the RCB's. */

		rel_rcb->rcb_lk_mode = RCB_K_IS;
		rel_rcb->rcb_k_mode = RCB_K_IS;
		rel_rcb->rcb_access_mode = RCB_A_READ;

		/*
		** Lookup the iirelation row.  If the row is not found,
		** then return NONEXISTENT_TABLE.  Treat any other error
		** as an internal show error.
		*/
		status = dm2r_get(rel_rcb, (DM_TID *)&rel_index.tidp, 
			    DM2R_BYTID, (char *)&relation, &dmt->error);
		if (status != E_DB_OK)
		{
		    if (dmt->error.err_code == E_DM0044_DELETED_TID)
			SETDBERR(&dmt->error, 0, E_DM0054_NONEXISTENT_TABLE);
		    else
			SETDBERR(&dmt->error, 0, E_DM010B_ERROR_SHOWING_TABLE);

		    break;
		}

		status = dm2r_release_rcb(&rel_rcb, &dmt->error);
		rel_rcb = NULL;
		if (status != E_DB_OK)
		    break;
		STRUCT_ASSIGN_MACRO(relation.reltid, table_id);
	    }
	}
	else if (table_id.db_tab_base < 0)
	{
	    /*
	    ** Then, this is a temporary table id (which will NOT be found
	    ** in the catalogs).
	    */
	    session_temp = TRUE;
	    status = E_DB_OK;
	}

	/*
	** Check for overrides of locking defaults for the table control
	** lock request.  The following characteristics can be overridden:
	**
	**	Lock timeout
	**
	** These characteristics can be specified either by the caller
	** in the dmt_char_array field of the dmt_cb, or by the user
	** by setting table or session level lockmode values.
	**
	** The order of precedence for locking values from lowest to highest is:
	**
	**	default values
	**	user supplied session level values
	**	caller supplied values
	**	user supplied table level values
	**
	** This check for locking characteristics must be made after the
	** above lookup for the table_id, as the table_id is needed
	** to check setlock options.
	*/

	/*
	** Obtain session level timeout value. 
	*/
        timeout = scb->scb_timeout;

	/*
	** Check for characteristics options specified by caller.
	** On error, the dmt_show_cb err_code and err_data fields
	** will have been filled in by check_char - preserve the
	** error value so it will not be overwritten in the
	** error handling code at the bottom of this routine.
	*/
	if (dmt->dmt_char_array.data_address &&
	    dmt->dmt_char_array.data_in_size)
	{
	    if (check_char(dmt, &timeout))
		break;
	}

	/*
	** Check for user supplied table level locking values.
        ** Loop through the slcb list looking for timeout value 
        ** that apply to this table.
        ** If a lockmode value is SESSION, just leave the lockmode
        ** value alone and assume it was set to the right thing when
        ** the session level setting was processed.
        */
        for (slcb = scb->scb_kq_next;
            slcb != (DML_SLCB*) &scb->scb_kq_next;
            slcb = slcb->slcb_q_next)
        {
            /* Check if this SLCB applies to the current table. */
            if (slcb->slcb_db_tab_base == table_id.db_tab_base )
            {
                if (slcb->slcb_timeout != DMC_C_SESSION)
                    timeout = slcb->slcb_timeout;
		break;
            }
        }
 
	if (timeout == DMC_C_SYSTEM)
	    timeout = dmf_svcb->svcb_lk_waitlock;  /* default system timeout */ 

	/*
	** Get shared Table Control lock while we are building and/or
	** looking at the TCB.  This will prevent the table from being
	** destroyed or modified while we are in the middle of building a
	** tcb for it.  It also prevents the TCB from being purged while we
	** are looking at the information in it.
        **
        ** If this is not a permanent table, then there is no need to
        ** obtain a control lock on it, since it can ONLY be used by
        ** the current session.
        */
        if (!session_temp)
        {
	    status = dm2t_control(odcb->odcb_dcb_ptr, table_id.db_tab_base,
		lock_list, LK_S, (i4)0, timeout, &dmt->error);

	    if (status != E_DB_OK)
		break;
	    control_lock = TRUE;
	}

	/*  If this is by access id then the TCB is easily procured. */

	if (dmt->dmt_flags_mask & DMT_M_ACCESS_ID)
	{
	    tcb = rcb->rcb_tcb_ptr;
	}
	else
	{
	    /*
	    ** Locate or create the TCB in memory.
	    ** Pass NOPARTIAL flag to ensure that we get a full tcb with
	    ** system catalog information.
	    ** Pass SHOWONLY flag because we'll accept a TCB that has
	    ** full info, but can't be used for real table operations
	    ** because the cache for that table's pagesize isn't there.
	    **
	    ** If we're going after a partition, fix the master first;
	    ** this is unusual, but can happen from function callbacks
	    ** (eg the functions that are used by the iitables view, if
	    ** a view row corresponding to a partition is retrieved).
	    ** Most SHOW's just ask for the master, not the partition.
	    */

	    if (table_id.db_tab_index < 0)
	    {
		/* It's a partition, make sure we have the master first */
		master_id.db_tab_base = table_id.db_tab_base;
		master_id.db_tab_index = 0;
		status = dm2t_fix_tcb(dcb->dcb_id, &master_id, tran_id_ptr,
			lock_list, log_id,
			(DM2T_VERIFY | DM2T_NOPARTIAL | DM2T_SHOWONLY),
			dcb, &master_tcb, &dmt->error);
		if (status != E_DB_OK)
		    break;
	    }
	    status = dm2t_fix_tcb(dcb->dcb_id, &table_id, tran_id_ptr, 
		lock_list, log_id,
		(DM2T_VERIFY | DM2T_NOPARTIAL | DM2T_SHOWONLY),
		dcb, &tcb, &dmt->error);
	    if (status != E_DB_OK)
		break;
	}

	/* Setup search for iihistogram.histogram. */
	if (tcb->tcb_rel.reltid.db_tab_base == DM_B_HISTOGRAM_TAB_ID &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    iihistogram = ii_datatype_hack = TRUE;

	/* Setup search for ii_encoded_forms.cfdata. */
	if (MEcmp(tcb->tcb_rel.relid.db_tab_name, "ii_encoded_forms ",
		sizeof("ii_encoded_forms")) == 0 &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    ii_encoded_forms = ii_datatype_hack = TRUE;

 	if ((is_utf8) &&
	    (MEcmp(tcb->tcb_rel.relid.db_tab_name, "ii", 2) == 0 ))
	{
	  /* Setup search for iidd_ddb_tree.treetree */
	  if (MEcmp(tcb->tcb_rel.relid.db_tab_name, "iitree ",
		sizeof("iitree")) == 0 &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    iitree = ii_datatype_hack = TRUE;

	  /* Setup search for iidd_ddb_tree.treetree */
	  if (MEcmp(tcb->tcb_rel.relid.db_tab_name, "iiddb_tree ",
		sizeof("iiddb_tree")) == 0 &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    iiddb_tree = ii_datatype_hack = TRUE;

	  /* Setup search for iidd_histograms.text_segment */
	  if (MEcmp(tcb->tcb_rel.relid.db_tab_name, "iidd_histograms ",
		sizeof("iidd_histograms")) == 0 &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    iidd_histograms = ii_datatype_hack = TRUE;

	  /* Setup search for iidd_ddb_tree.treetree */
	  if (MEcmp(tcb->tcb_rel.relid.db_tab_name, "iidd_ddb_tree ",
		sizeof("iidd_ddb_tree")) == 0 &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    iidd_ddb_tree = ii_datatype_hack = TRUE;

	  /* Setup search for ii_stored_bitmaps.text_value */
	  if (MEcmp(tcb->tcb_rel.relid.db_tab_name, "ii_stored_bitmaps ",
		sizeof("ii_stored_bitmaps")) == 0 &&
	    tcb->tcb_rel.reltid.db_tab_index == 0)
	    ii_stored_bitmaps= ii_datatype_hack = TRUE;
	}

	/*  Copy data back to caller. */

	for (i = 1; i <= tcb->tcb_rel.relatts; i++)	
	{
#ifdef xDebug
	   bool printtrace = FALSE;
#endif
	    if (tcb->tcb_atts_ptr[i].ver_dropped)
		 
		attr_drop_count++;

	  if (ii_datatype_hack)
	  {
	    /* Special case to force "histogram" column of iihistogram
	    ** table to BYTE, to avoid Unicode normalization on UTF8 dbs. */
	    if (iihistogram && i == HISTOGRAM_COL_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "histogram") == 0) 
	    {
		tcb->tcb_atts_ptr[i].type = DB_BYTE_TYPE;
#ifdef xDebug
		printtrace = TRUE;
#endif
	    }
	    /* Special case for "text_segment" column of iidd_histograms */
	    else if (iidd_histograms && i == HIST_TEXT_SEGMENT_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "text_segment") == 0)
	    {
		tcb->tcb_atts_ptr[i].type = DB_BYTE_TYPE;
#ifdef xDebug
		printtrace = TRUE;
#endif
	    }
	    /* Special case for "cfdata" column of ii_encoded_forms */
	    else if (ii_encoded_forms && i == CFDATA_COL_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "cfdata") == 0)
	    {
		    tcb->tcb_atts_ptr[i].type = DB_VBYTE_TYPE;
#ifdef xDebug
		    printtrace = TRUE;
#endif
	    }
	    /* Special case for "treetree" column of iidd_ddb_tree */
	    else if (iidd_ddb_tree && i == TREETREE_COL_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "treetree") == 0)
	    {
		    tcb->tcb_atts_ptr[i].type = DB_VBYTE_TYPE;
#ifdef xDebug
		    printtrace = TRUE;
#endif
	    }
	    /* Special case for "treetree" column of iiddb_tree */
	    else if (iiddb_tree && i == TREETREE_COL_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "treetree") == 0)
	    {
		    tcb->tcb_atts_ptr[i].type = DB_VBYTE_TYPE;
#ifdef xDebug
		    printtrace = TRUE;
#endif
	    }
	    /* Special case for "treetree" column of iitree */
	    else if (iitree && i == TREETREE_COL_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "treetree") == 0)
	    {
		    tcb->tcb_atts_ptr[i].type = DB_VBYTE_TYPE;
#ifdef xDebug
		    printtrace = TRUE;
#endif
	    }
	    /* Special case for "treetree" column of iitree */
	    else if (ii_stored_bitmaps && i == II_STORED_BITMAPS_POS &&
		STcompare(tcb->tcb_atts_ptr[i].attnmstr, "text_value") == 0)
	    {
		    tcb->tcb_atts_ptr[i].type = DB_VBYTE_TYPE;
#ifdef xDebug
		    printtrace = TRUE;
#endif
	    }

#ifdef xDebug
	    if (printtrace)
	    { 
	      TRdisplay("%@ dmt_show - Table:(%~t) Column:(%~t) new type:(%d)\n",
               		sizeof(tcb->tcb_rel.relid.db_tab_name),
                      	tcb->tcb_rel.relid.db_tab_name,
			tcb->tcb_atts_ptr[i].attnmlen,
			tcb->tcb_atts_ptr[i].attnmstr,
		    	tcb->tcb_atts_ptr[i].type) ;
	    }
#endif
	  }
	}

	if (dmt->dmt_flags_mask & (DMT_M_TABLE | DMT_M_COUNT_UPD))
	{
	    /* Compute current, real-time tuple and page counts */
	    /* Mutex the TCB to prevent the RCB list from changing */
	    dm0s_mlock(&tcb->tcb_mutex);
	    dm2t_current_counts(tcb,
			&dmt->dmt_reltups, &dmt->dmt_relpages);
	    dm0s_munlock(&tcb->tcb_mutex);
	    /* The current-counts routine makes sure that the returned
	    ** values are OK, not negative, etc.
	    */
	}

	if (dmt->dmt_flags_mask & DMT_M_TABLE)
	{
	    if (dmt->dmt_table.data_in_size < sizeof(DMT_TBL_ENTRY))
	    {
		SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
	    }

	    tbl = (DMT_TBL_ENTRY *)dmt->dmt_table.data_address;
	    dmt->dmt_table.data_out_size = sizeof(DMT_TBL_ENTRY);
	    tbl->tbl_record_count = dmt->dmt_reltups;
	    tbl->tbl_page_count = dmt->dmt_relpages;
	    tbl->tbl_id = tcb->tcb_rel.reltid;
	    tbl->tbl_name = tcb->tcb_rel.relid;
	    tbl->tbl_owner = tcb->tcb_rel.relowner;
	    tbl->tbl_location = tcb->tcb_rel.relloc;
	    tbl->tbl_attr_count = (tcb->tcb_rel.relatts - attr_drop_count);
	    tbl->tbl_attr_high_colno = tcb->tcb_rel.relatts;
	    tbl->tbl_index_count = (i4)tcb->tcb_index_count;
	    tbl->tbl_width = tcb->tcb_rel.relwid;
	    tbl->tbl_pgsize = tcb->tcb_rel.relpgsize;
	    tbl->tbl_storage_type = tcb->tcb_rel.relspec;
	    tbl->tbl_status_mask = tcb->tcb_rel.relstat;
            tbl->tbl_create_time = tcb->tcb_rel.relcreate;
            tbl->tbl_qryid.db_qry_high_time = tcb->tcb_rel.relqid1;
            tbl->tbl_qryid.db_qry_low_time = tcb->tcb_rel.relqid2;
            tbl->tbl_struct_mod_date = tcb->tcb_rel.relmoddate;
	    tbl->tbl_i_fill_factor = tcb->tcb_rel.relifill;
	    tbl->tbl_d_fill_factor = tcb->tcb_rel.reldfill;
	    tbl->tbl_l_fill_factor = tcb->tcb_rel.rellfill;
            tbl->tbl_min_page = tcb->tcb_rel.relmin;
            tbl->tbl_max_page = tcb->tcb_rel.relmax;
	    tbl->tbl_loc_count = 1;
	    if (tcb->tcb_rel.relstat & TCB_MULTIPLE_LOC) 
		tbl->tbl_loc_count = tcb->tcb_table_io.tbio_loc_count;
	    if ( (tcb->tcb_rel.relstat & TCB_VIEW) ||
		 (tcb->tcb_rel.relstat & TCB_GATEWAY) )
		    tbl->tbl_loc_count = 0;
            tbl->tbl_relgwid = tcb->tcb_rel.relgwid;
	    tbl->tbl_relgwother = tcb->tcb_rel.relgwother;
            tbl->tbl_comp_type = tcb->tcb_rel.relcomptype;
            tbl->tbl_pg_type = tcb->tcb_rel.relpgtype;
	    tbl->tbl_temporary = tcb->tcb_temporary;
	    tbl->tbl_dimension = tcb->tcb_dimension;
	    tbl->tbl_hilbertsize = tcb->tcb_hilbertsize;
	    tbl->tbl_rangedt = tcb->tcb_rangedt;
	    tbl->tbl_rangesize = tcb->tcb_rangesize;
	    if (tcb->tcb_range != NULL)
	      MEcopy((PTR)tcb->tcb_range, tcb->tcb_rangesize,
		   (PTR)tbl->tbl_range);
	    tbl->tbl_cachepri = tcb->tcb_rel.reltcpri;
	    tbl->tbl_nparts = tcb->tcb_rel.relnparts;
	    tbl->tbl_ndims = tcb->tcb_rel.relnpartlevels;

	    /* FIXME:  make these DMT-flags and TCB2-flags identical, get
	    ** rid of TCB2 defs, ditch all this silly translation code.
	    */
	    tbl->tbl_2_status_mask = 0;
	    if ( tcb->tcb_rel.relstat2 & TCB2_EXTENSION )
	    {   tbl->tbl_2_status_mask |= DMT_TEXTENSION;	}
	    if ( tcb->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS )
	    {   tbl->tbl_2_status_mask |= DMT_HAS_EXTENSIONS;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_PERSISTS_OVER_MODIFIES )
	    {   tbl->tbl_2_status_mask |= DMT_PERSISTS_OVER_MODIFIES;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_SUPPORTS_CONSTRAINT )
	    {   tbl->tbl_2_status_mask |= DMT_SUPPORTS_CONSTRAINT;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_NOT_UNIQUE )
	    {   tbl->tbl_2_status_mask |= DMT_NOT_UNIQUE;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_SYSTEM_GENERATED )
	    {   tbl->tbl_2_status_mask |= DMT_SYSTEM_GENERATED;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_NOT_DROPPABLE )
	    {   tbl->tbl_2_status_mask |= DMT_NOT_DROPPABLE;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE )
	    {   tbl->tbl_2_status_mask |= DMT_STATEMENT_LEVEL_UNIQUE;	}
	    if ( tcb->tcb_rel.relstat2 & TCB2_READONLY )
	    {   tbl->tbl_2_status_mask |= DMT_READONLY;	}
	    if ( tcb->tcb_rel.relstat2 & TCB2_CONCURRENT_ACCESS )
	    {   tbl->tbl_2_status_mask |= DMT_CONCURRENT_ACCESS; }
	    if ( tcb->tcb_rel.relstat2 & TCB_ROW_AUDIT)
	    {   tbl->tbl_2_status_mask |= DMT_ROW_SEC_AUDIT;	}
	    if ( tcb->tcb_rel.relstat2 & TCB_ALARM)
	    {   tbl->tbl_2_status_mask |= DMT_ALARM;	}
	    if ( tcb->tcb_rel.relstat2 & TCB2_LOGICAL_INCONSISTENT)
	    {   tbl->tbl_2_status_mask |= DMT_LOGICAL_INCONSISTENT;}
	    if ( tcb->tcb_rel.relstat2 & TCB2_PHYS_INCONSISTENT)
	    {   tbl->tbl_2_status_mask |= DMT_PHYSICAL_INCONSISTENT;}
	    if ( tcb->tcb_rel.relstat2 & TCB2_GLOBAL_INDEX)
	    {   tbl->tbl_2_status_mask |= DMT_GLOBAL_INDEX;}
	    if ( tcb->tcb_rel.relstat2 & TCB2_PARTITION)
	    {   tbl->tbl_2_status_mask |= DMT_PARTITION;}

	    /* Fix up the relstat if stale for online ckp */
	    if ((tcb->tcb_dcb_ptr->dcb_status & DCB_S_JOURNAL) && 
		(tcb->tcb_rel.relstat & TCB_JON) &&
		((tcb->tcb_dcb_ptr->dcb_status & DCB_S_BACKUP) ||
		 (((tcb->tcb_dcb_ptr->dcb_status & DCB_S_BACKUP) == 0) &&
		  (LSN_LT(&tcb->tcb_iirel_lsn, 
			&tcb->tcb_dcb_ptr->dcb_ebackup_lsn)))))
	    {
		tbl->tbl_status_mask &= ~TCB_JON;
		tbl->tbl_status_mask |= TCB_JOURNAL;
	    }

	    tbl->tbl_alloc_pg_count = tcb->tcb_table_io.tbio_lalloc + 1;

	    tbl->tbl_allocation = tcb->tcb_rel.relallocation;
	    tbl->tbl_extend = tcb->tcb_rel.relextend;

	    tbl->tbl_tperpage = tcb->tcb_tperpage;

	    switch (tcb->tcb_rel.relspec)
	    {
	    case TCB_HEAP:
		tbl->tbl_dpage_count = 
		    tbl->tbl_page_count - 1 -
		    ((tcb->tcb_table_io.tbio_lalloc / 
		    DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1);
		tbl->tbl_ipage_count = 0;
		tbl->tbl_opage_count = tbl->tbl_dpage_count - 1;
		tbl->tbl_kperpage = 0;
		tbl->tbl_kperleaf = 0;
		break;

	    case TCB_HASH:
		tbl->tbl_dpage_count = tcb->tcb_rel.relmain;
		tbl->tbl_ipage_count = 0;
		/*
		** Overflow pages = total_number of pages in table minus
		**  	- Nmber of hash bucket pages 
		**	- Number of FHDR pages, ie one.
		**	- Number of FMAP pages
		*/
		tbl->tbl_opage_count =  
		       tbl->tbl_page_count - tbl->tbl_dpage_count -
		       1 - ((tcb->tcb_table_io.tbio_lalloc / 
		       DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1);
		tbl->tbl_kperpage = 0;
		tbl->tbl_kperleaf = 0;
		tbl->tbl_lpage_count = 0;
		tbl->tbl_ixlevels = 0;
		break;

	    case TCB_ISAM:
		tbl->tbl_dpage_count = tcb->tcb_rel.relmain;
		tbl->tbl_ipage_count = 
                         tcb->tcb_rel.relprim - tcb->tcb_rel.relmain + 1;
		/*
		** Overflow pages = total_number of pages in table minus
		**	- relprim holds the pagenumber of the root index
		**	  page, all pages after this are either fhdr/fmap
		**	  or overflow pages.
		**	- Number of FHDR pages, ie one.
		**	- Number of FMAP pages
		*/
		tbl->tbl_opage_count =  
		       tbl->tbl_page_count - (tcb->tcb_rel.relprim + 1) - 1 - 
		       ( (tcb->tcb_table_io.tbio_lalloc / 
		       DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1);

		/* If this ISAM table is partitioned, go through all subsequent
		** partitions and subtract their pages from the overflow
		** page count.  The pages in subsequent partitions are included
		** in tbl->tbl_page_count but not in tcb->tcb_rel_relprim, so
		** it ends up looking worryingly like there are huge numbers
		** of overflow pages (kibro01) b118103
		*/ 
		if ((tcb->tcb_rel.relstat & TCB_IS_PARTITIONED) &&
			tcb->tcb_rel.relnparts)
		{
		    i4 i;
		    i4 part_pages;
		    i4 part_fmap;
		    DMT_PHYS_PART *tcb_pp_ptr;

		    for (i = 1, tcb_pp_ptr = &tcb->tcb_pp_array[1];
			i < tcb->tcb_rel.relnparts;
			i++, tcb_pp_ptr++)
		    {
	    		tran_id.db_high_tran = 0;
	    		tran_id.db_low_tran = 0;
			if (!tcb_pp_ptr->pp_tcb)
			{
			    STRUCT_ASSIGN_MACRO(tcb_pp_ptr->pp_tabid,master_id);
			    status = dm2t_fix_tcb(dcb->dcb_id, &master_id,
				&tran_id, lock_list, log_id,
				(DM2T_VERIFY | DM2T_NOPARTIAL | DM2T_SHOWONLY),
				dcb, &partition_tcb, &dmt->error);

			    if (status != E_DB_OK)
			        break;
			    part_pages = partition_tcb->tcb_rel.relprim+1;
			    part_fmap = partition_tcb->tcb_table_io.tbio_lalloc /
				DM1P_FSEG_MACRO(partition_tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize) + 1;

			    local_status = dm2t_unfix_tcb(&partition_tcb,
				DM2T_VERIFY, lock_list, log_id, &local_dberr);
	    		    if (local_status != E_DB_OK)
			    {
			        uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL,
				    NULL, 0, NULL, &local_error, 0);
			        break;
			    }
			} 
			else
			{
			    part_pages = tcb_pp_ptr->pp_tcb->tcb_rel.relprim+1;
			    part_fmap = tcb_pp_ptr->pp_tcb->tcb_table_io.tbio_lalloc /
				DM1P_FSEG_MACRO(tcb_pp_ptr->pp_tcb->tcb_rel.relpgtype, 
				    tcb_pp_ptr->pp_tcb->tcb_rel.relpgsize) + 1;
			}
			/* Now reduce overflow by valid pages in this part 
			** using same calculation as for master partition
			** (including the +1 for FHDR)
			*/
			tbl->tbl_opage_count -= part_pages + part_fmap + 1;
		    }
		}

		tbl->tbl_kperpage = (tcb->tcb_rel.relpgsize - 
				DMPPN_VPT_OVERHEAD_MACRO(tcb->tcb_rel.relpgtype))/
				(tcb->tcb_klen + 
				DM_VPT_SIZEOF_LINEID_MACRO(tcb->tcb_rel.relpgtype));
		/* Prior to Ingres 2.5, keys per page limited to 512 */
		if (tbl->tbl_kperpage > (DM_TIDEOF + 1))
		{
		    if (tcb->tcb_rel.relcmptlvl == DMF_T0_VERSION ||
			tcb->tcb_rel.relcmptlvl == DMF_T1_VERSION ||
			tcb->tcb_rel.relcmptlvl == DMF_T2_VERSION ||
			tcb->tcb_rel.relcmptlvl == DMF_T3_VERSION ||
			tcb->tcb_rel.relcmptlvl == DMF_T4_VERSION)
			tbl->tbl_kperpage = DM_TIDEOF + 1;
		}

		tbl->tbl_kperleaf = 0;
		tbl->tbl_lpage_count = 0;
		tbl->tbl_ixlevels = 0;
		break;

	    case TCB_BTREE:
		{
		    i4	tuples = tbl->tbl_record_count;
		    i4	lfanout;
		    i4	ifanout;
		    char	firsttime = 'F';

		    ifanout = (tcb->tcb_kperpage * 
					tbl->tbl_i_fill_factor + 99) / 100;
		    ifanout = max(2, ifanout);
		    lfanout = (tcb->tcb_kperleaf * 
					tbl->tbl_l_fill_factor + 99) / 100;
		    lfanout = max(2, lfanout);

		    tbl->tbl_opage_count = 0; 
		    tbl->tbl_ipage_count = 1;
		    tbl->tbl_ixlevels = 0;
		    do
		    {
		      if (firsttime == 'F')
		      {
			firsttime = 'N';
			tuples = (tuples + lfanout -1) / lfanout;
			tbl->tbl_lpage_count = tuples;
		      }
		      else
		        tuples = (tuples + ifanout - 1) / ifanout;

		      tbl->tbl_ipage_count += tuples;
		      tbl->tbl_ixlevels += 1;

		    } while (tuples > 1);

		    /* 
		    ** Make sure sane values are returned.  Some of our
		    ** estimates here are based on # of tuples which may
		    ** be an incorrect estimate. Minimum number of pages
		    ** in a BTREE table is one data, leaf, index, FHDR and 
		    ** FMAP page 
		    */
		    if (tbl->tbl_page_count < 5)
		    {
			/*
			**  Calulate a guess at number of pages as one
			** data, leaf, index and FHDR plus number of 
			** FMAPs to map the allocated pages count.
			*/
			tbl->tbl_page_count = 3 + 1 + 
			((tcb->tcb_table_io.tbio_lalloc / 
			DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1 );
		    }

		    if (tbl->tbl_ipage_count >= tbl->tbl_page_count)
			tbl->tbl_ipage_count = tbl->tbl_page_count - 1;

		    /*
		    ** Subtract the index pages, FHDR and FMAP(s) pages.
		    */
		    tbl->tbl_dpage_count = 
			tbl->tbl_page_count - tbl->tbl_ipage_count - 1 - 
			((tcb->tcb_table_io.tbio_lalloc / 
			DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1 );
		}
		tbl->tbl_kperpage = tcb->tcb_kperpage;
		tbl->tbl_kperleaf = tcb->tcb_kperleaf;
		break;

	    case TCB_RTREE:
		{
		    i4	tuples = tbl->tbl_record_count;
		    i4	lfanout;
		    i4	ifanout;
		    char	firsttime = 'F';

		    ifanout = (tcb->tcb_kperpage * tbl->tbl_i_fill_factor + 99)
				/ 100;
		    ifanout = max(2, ifanout);
		    lfanout = (tcb->tcb_kperleaf * tbl->tbl_l_fill_factor + 99)
				/ 100;
		    lfanout = max(2, lfanout);

		    tbl->tbl_opage_count = 0; 
		    tbl->tbl_ipage_count = 1;
		    tbl->tbl_ixlevels = 0;
		    do
		    {
		      if (firsttime == 'F')
		      {
			firsttime = 'N';
			tuples = (tuples + lfanout -1) / lfanout;
			tbl->tbl_lpage_count = tuples;
		      }
		      else
		        tuples = (tuples + ifanout - 1) / ifanout;

		      tbl->tbl_ipage_count += tuples;
		      tbl->tbl_ixlevels += 1;

		    } while (tuples > 1);

		    /* 
		    ** Make sure sane values are returned.  Some of our
		    ** estimates here are based on # of tuples which may
		    ** be an incorrect estimate. Minimum number of pages
		    ** in a RTREE index is one leaf, index, FHDR and 
		    ** FMAP page 
		    */
		    if (tbl->tbl_page_count < 4)
		    {
			/*
			**  Calulate a guess at number of pages as one
			** leaf, index and FHDR plus number of 
			** FMAPs to map the allocated pages count.
			*/
			tbl->tbl_page_count = 2 + 1 + 
			((tcb->tcb_table_io.tbio_lalloc / 
			DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1 );
		    }

		    if (tbl->tbl_ipage_count >= tbl->tbl_page_count)
			tbl->tbl_ipage_count = tbl->tbl_page_count - 1;

		    /*
		    ** Subtract the index pages, FHDR and FMAP(s) pages.
		    */
		    tbl->tbl_dpage_count = 
			tbl->tbl_page_count - tbl->tbl_ipage_count - 1 - 
			((tcb->tcb_table_io.tbio_lalloc / 
			DM1P_FSEG_MACRO(tcb->tcb_rel.relpgtype, tcb->tcb_rel.relpgsize)) + 1 );
		}
		tbl->tbl_kperpage = tcb->tcb_kperpage;
		tbl->tbl_kperleaf = tcb->tcb_kperleaf;
		break;
	    }
	    tbl->tbl_expiration = tcb->tcb_rel.relsave;
	    tbl->tbl_date_modified = tcb->tcb_rel.relstamp12;
	    dm2f_filename(DM2F_TAB, &tcb->tcb_rel.reltid, 
			tcb->tcb_table_io.tbio_location_array[0].loc_id,
			0, 0,
			(DM_FILE *)tbl->tbl_filename.db_loc_name);

	    /*
	    ** Make sure page/row counts are non-negative.
	    ** Inaccuracies in page/tuple counts could cause these
	    ** to become negative - which will cause OPF to die.
	    */
	    if (tbl->tbl_dpage_count <= 0)
		tbl->tbl_dpage_count = 1;
	    if (tbl->tbl_ipage_count < 0)
		tbl->tbl_ipage_count = 0;
	    if (tbl->tbl_opage_count < 0)
		tbl->tbl_opage_count = 0;
	}

	if (dmt->dmt_flags_mask & DMT_M_PHYPART)
	{
	    if ((tcb->tcb_rel.relstat & TCB_IS_PARTITIONED) == 0)
	    {
		/* Common case of no partitions, skip this quickly */
		dmt->dmt_phypart_array.data_out_size = 0;
	    }
	    else
	    {
		DMP_TCB *pp_tcb;
		DMT_PHYS_PART *out_pp_ptr;
		DMT_PHYS_PART *tcb_pp_ptr;
		DM_MUTEX *hash_mutex;
		i4 i;

		if (dmt->dmt_phypart_array.data_in_size < tcb->tcb_rel.relnparts * sizeof(DMT_PHYS_PART))
		{
		    SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
		    status = E_DB_ERROR;
		    break;
		}
		/* Same mutex works for everyone... */
		hash_mutex = tcb->tcb_hash_mutex_ptr;
		dmt->dmt_phypart_array.data_out_size = tcb->tcb_rel.relnparts * sizeof(DMT_PHYS_PART);
		out_pp_ptr = (DMT_PHYS_PART *) dmt->dmt_phypart_array.data_address;
		tcb_pp_ptr = &tcb->tcb_pp_array[0];
		i = tcb->tcb_rel.relnparts;
		while (--i >= 0)
		{
		    STRUCT_ASSIGN_MACRO(tcb_pp_ptr->pp_tabid,out_pp_ptr->pp_tabid);
		    out_pp_ptr->pp_reltups = tcb_pp_ptr->pp_reltups;
		    out_pp_ptr->pp_relpages = tcb_pp_ptr->pp_relpages;
		    /* Dirty-peek at the partition TCB;  if there is one, we
		    ** have a shot at more up-to-date counts for this
		    ** partition, but it's a wee bit more work!  We have to
		    ** take the hash mutex and check again for a real TCB,
		    ** and if it still looks like there's one there, mutex
		    ** the TCB itself and count it.
		    ** Notice that we don't change the real PP array, it
		    ** will get updated all in good time.
		    */
		    if (tcb_pp_ptr->pp_tcb != NULL)
		    {
			dm0s_mlock(hash_mutex);
			if ( (pp_tcb = tcb_pp_ptr->pp_tcb) != NULL)
			{
			    dm0s_mlock(&pp_tcb->tcb_mutex);
			    dm2t_current_counts(pp_tcb,
					&out_pp_ptr->pp_reltups,
					&out_pp_ptr->pp_relpages);
			    dm0s_munlock(&pp_tcb->tcb_mutex);
			}
			dm0s_munlock(hash_mutex);
		    }
		    out_pp_ptr->pp_tcb = NULL;
		    out_pp_ptr->pp_iirel_tid.tid_i4 = 0;
		    ++ tcb_pp_ptr;
		    ++ out_pp_ptr;
		}
		/*
		** Because this isn't being done at the same time as
		** the master count above, it's possible that the sum
		** over the returned PP array is different from the
		** master reltups/relpages;  if this is an issue, add
		** up the caller's pparray here to recalculate the
		** master numbers.  Not doing it until something needs it.
		*/
	    }
	} /* DMT_M_PHYPART */

  	/*
	** Use (multiple) communication buffers for attribute info.
	*/
	if ((dmt->dmt_flags_mask & DMT_M_ATTR) &&
	    (dmt->dmt_flags_mask & DMT_M_MULTIBUF)) 
	{
	    DM_PTR	*dm_ptr;
	    DMT_ATT_ENTRY    *a;

	    dm_ptr = (DM_PTR *)&dmt->dmt_attr_array;
	    a = (DMT_ATT_ENTRY *)dm_ptr->ptr_address;
	    dm_ptr->ptr_out_count = 1;

	    for (i = 1, j = 0; i <= tcb->tcb_rel.relatts; i++)
	    {

		if (j >= dm_ptr->ptr_in_count)
		{
		    /* exceed comm buffer size. */
		    dm_ptr->ptr_out_count = dm_ptr->ptr_in_count;
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || \
    defined(LP64) || defined(NT_IA64) || defined(NT_AMD64)
		    MEcopy(&dm_ptr->ptr_size, sizeof(PTR), &dm_ptr);
#else
		    dm_ptr = (DM_PTR *)dm_ptr->ptr_size;
#endif
		    if (dm_ptr == NULL)
		    {
			SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
			status = E_DB_ERROR;
			break;
		    }
		    a = (DMT_ATT_ENTRY *)dm_ptr->ptr_address;
		    j = 0;
		}

		if (tcb->tcb_atts_ptr[i].ver_dropped)
		   continue;

		MEmove(tcb->tcb_atts_ptr[i].attnmlen,
		    tcb->tcb_atts_ptr[i].attnmstr, ' ',
		    DB_ATT_MAXNAME, a[j].att_name.db_att_name);
		a[j].att_number = tcb->tcb_atts_ptr[i].ordinal_id;
		a[j].att_type = tcb->tcb_atts_ptr[i].type;
		a[j].att_width = tcb->tcb_atts_ptr[i].length;
		a[j].att_offset = tcb->tcb_atts_ptr[i].offset;
		a[j].att_prec = tcb->tcb_atts_ptr[i].precision;
		a[j].att_collID = tcb->tcb_atts_ptr[i].collID;
		a[j].att_geomtype = tcb->tcb_atts_ptr[i].geomtype;
		a[j].att_srid = tcb->tcb_atts_ptr[i].srid;
		a[j].att_flags = tcb->tcb_atts_ptr[i].flag;
		COPY_DEFAULT_ID( tcb->tcb_atts_ptr[i].defaultID,
		    a[j].att_defaultID );
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || \
    defined(LP64) || defined(NT_IA64) || defined(NT_AMD64)
		MEfill(sizeof(PTR), 0, &(a[j].att_default));
#else
		a[j].att_default = ( PTR ) NULL;
#endif
		a[j].att_key_seq_number = tcb->tcb_atts_ptr[i].key;
		a[j].att_intlid = tcb->tcb_atts_ptr[i].intl_id;
		j++;
	    }
	    if (status)
		break;

	    dm_ptr->ptr_out_count = --j;
	}
	else if (dmt->dmt_flags_mask & DMT_M_ATTR)
	{
	    if ((dmt->dmt_attr_array.ptr_in_count < 
                     (tcb->tcb_rel.relatts + 1 - attr_drop_count)) ||
                (dmt->dmt_attr_array.ptr_size <
                     sizeof(DMT_ATT_ENTRY)))
	    {
		SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
	    }

	    att = (DMT_ATT_ENTRY **)dmt->dmt_attr_array.ptr_address;
	    dmt->dmt_attr_array.ptr_out_count = 
                      (tcb->tcb_rel.relatts + 1 - attr_drop_count);

	    for (i = 1, j = 0; i <= tcb->tcb_rel.relatts; i++)	
	    {
		DMT_ATT_ENTRY    *a;

		if (tcb->tcb_atts_ptr[i].ver_dropped)
		   continue;

		j++;
		a = att[j];
		MEmove(tcb->tcb_atts_ptr[i].attnmlen,
		    tcb->tcb_atts_ptr[i].attnmstr, ' ',
		    DB_ATT_MAXNAME, a->att_name.db_att_name);
		a->att_number = tcb->tcb_atts_ptr[i].ordinal_id;
		a->att_type = tcb->tcb_atts_ptr[i].type;
		a->att_width = tcb->tcb_atts_ptr[i].length;
		a->att_offset = tcb->tcb_atts_ptr[i].offset;
		a->att_prec = tcb->tcb_atts_ptr[i].precision;
		a->att_collID = tcb->tcb_atts_ptr[i].collID;
		a->att_geomtype = tcb->tcb_atts_ptr[i].geomtype;
		a->att_srid = tcb->tcb_atts_ptr[i].srid;
		a->att_flags = tcb->tcb_atts_ptr[i].flag;
		COPY_DEFAULT_ID( tcb->tcb_atts_ptr[i].defaultID, a->att_defaultID );
		a->att_default = ( PTR ) NULL;
		a->att_key_seq_number = tcb->tcb_atts_ptr[i].key;
		a->att_intlid = tcb->tcb_atts_ptr[i].intl_id;
	    }
	}
	if (dmt->dmt_flags_mask & DMT_M_LOC)
	{
	    if (dmt->dmt_loc_array.ptr_in_count < tcb->tcb_table_io.tbio_loc_count)
	    {
		SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
	    }

	    loc = (DB_LOC_NAME **)dmt->dmt_loc_array.ptr_address;
	    dmt->dmt_loc_array.ptr_out_count = tcb->tcb_table_io.tbio_loc_count;

	    /* consi01 bug 81094 copy back all locations */
	    for (i = 0; i < tcb->tcb_table_io.tbio_loc_count; i++, loc++)
	    {
		STRUCT_ASSIGN_MACRO(
			tcb->tcb_table_io.tbio_location_array[i].loc_name,
			**loc);
	    }
	}
  	/*
	** Use (multiple) communication buffers for index info.
	*/
	if ((dmt->dmt_flags_mask & DMT_M_INDEX) &&
	    (dmt->dmt_flags_mask & DMT_M_MULTIBUF)) 
	{
	    DM_PTR	*dm_ptr;
	    DMT_IDX_ENTRY    *ix;

	    dm_ptr = (DM_PTR *)&dmt->dmt_index_array;
	    ix = (DMT_IDX_ENTRY *)dm_ptr->ptr_address;

	    for (
		k = 0, itcb = tcb->tcb_iq_next, j = 0;
		itcb != (DMP_TCB*)&tcb->tcb_iq_next;
		k++,itcb = itcb->tcb_q_next, j++
		)
	    {
		if (j >= dm_ptr->ptr_in_count)
		{
		    /* exceed comm buffer size. */
		    dm_ptr->ptr_out_count = dm_ptr->ptr_in_count;
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || \
    defined(LP64) || defined(NT_IA64) || defined(NT_AMD64)
		    MEcopy(&dm_ptr->ptr_size, sizeof(PTR), &dm_ptr);
#else
		    dm_ptr = (DM_PTR *)dm_ptr->ptr_size;
#endif
		    if (dm_ptr == NULL)
		    {
			SETDBERR(&dmt->error, 0, E_DM013A_INDEX_COUNT_MISMATCH);
			status = E_DB_ERROR;
			break;
		    }
		    ix = (DMT_IDX_ENTRY *)dm_ptr->ptr_address;
		    j = 0;
		}
		ix[j].idx_name = itcb->tcb_rel.relid;
		STRUCT_ASSIGN_MACRO(itcb->tcb_rel.reltid, ix[j].idx_id);
		for (i = 0; i < DB_MAXIXATTS; i++)
		    ix[j].idx_attr_id[i] =
		   	tcb->tcb_atts_ptr[itcb->tcb_ikey_map[i]].ordinal_id;

		/*
		** Get key count. OPF/RDF expect that for a non-HASH index, the
		** TIDP is NOT counted in this value, even if the TIDP is part
		** of the key. Since tcb_rel.relkeys includes all the key
		** columns, including the TIDP, we must check to see if the
		** TIDP is part of the key or not and decrement idx_key_count
		** by one if it is.  The TIDP is usually part of the key for
		** non-hash indexes, but for 5.0 secondaries which were
		** converted by convto60, it may not be part of the key
		** (B33809).
		*/
		ix[j].idx_key_count = itcb->tcb_rel.relkeys;
		if (itcb->tcb_rel.relspec != TCB_HASH &&
		    itcb->tcb_atts_ptr[itcb->tcb_rel.relatts].key != 0)
		    ix[j].idx_key_count--;

		ix[j].idx_array_count = 0;
		for (i = 0; i < DB_MAXIXATTS; i++)
		{
		    if (itcb->tcb_ikey_map[i] == 0)
			break;
		    ix[j].idx_array_count++;
		}

		ix[j].idx_status = 
		    (itcb->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE)
			? DMT_I_STATEMENT_LEVEL_UNIQUE : 0;
                if (itcb->tcb_rel.relstat2 & (TCB2_LOGICAL_INCONSISTENT |
                        TCB2_PHYS_INCONSISTENT))
                    ix[j].idx_status |= DMT_I_INCONSISTENT;
                if (itcb->tcb_rel.relstat2 & TCB_PERSISTS_OVER_MODIFIES)
                    ix[j].idx_status |= DMT_I_PERSISTS_OVER_MODIFIES;

		ix[j].idx_storage_type = itcb->tcb_rel.relspec;
		ix[j].idx_pgsize = itcb->tcb_rel.relpgsize;

		/*
		** The following two fields are not used by OPF performs
		** a explicit DMT_SHOW call on each index to find out the
		** information.
		** Until we decide what values we should return here 
		** just zero out the fields.
		*/
		ix[j].idx_dpage_count = 0;
		ix[j].idx_ipage_count = 0;

	    }
	    if (status)
		break;
	}

	else if (dmt->dmt_flags_mask & DMT_M_INDEX)
	{
	    if ((dmt->dmt_index_array.ptr_in_count < 
                       tcb->tcb_index_count) ||
	        (dmt->dmt_index_array.ptr_size < 
                       sizeof(DMT_IDX_ENTRY)))
	    {
		SETDBERR(&dmt->error, 0, E_DM013A_INDEX_COUNT_MISMATCH);
		status = E_DB_ERROR;
		break;
	    }

	    idx = (DMT_IDX_ENTRY**)dmt->dmt_index_array.ptr_address;
	    dmt->dmt_index_array.ptr_out_count = 
                        tcb->tcb_index_count;
	    for (
		k = 0, itcb = tcb->tcb_iq_next;
		itcb != (DMP_TCB*)&tcb->tcb_iq_next;
		k++,itcb = itcb->tcb_q_next
		)
	    {
		DMT_IDX_ENTRY    *ix;
		ix = idx[k];
		ix->idx_name = itcb->tcb_rel.relid;
		STRUCT_ASSIGN_MACRO(itcb->tcb_rel.reltid, ix->idx_id);
		for (i = 0; i < DB_MAXIXATTS; i++)
		    ix->idx_attr_id[i] = 
		   	tcb->tcb_atts_ptr[itcb->tcb_ikey_map[i]].ordinal_id;

		/*
		** Get key count. OPF/RDF expect that for a non-HASH index, the
		** TIDP is NOT counted in this value, even if the TIDP is part
		** of the key. Since tcb_rel.relkeys includes all the key
		** columns, including the TIDP, we must check to see if the
		** TIDP is part of the key or not and decrement idx_key_count
		** by one if it is.  The TIDP is usually part of the key for
		** non-hash indexes, but for 5.0 secondaries which were
		** converted by convto60, it may not be part of the key
		** (B33809).
		*/
		ix->idx_key_count = itcb->tcb_rel.relkeys;
		if (itcb->tcb_rel.relspec != TCB_HASH &&
		    itcb->tcb_atts_ptr[itcb->tcb_rel.relatts].key != 0)
		    ix->idx_key_count--;

		ix->idx_array_count = 0;
		for (i = 0; i < DB_MAXIXATTS; i++)
		{
		    if (itcb->tcb_ikey_map[i] == 0)
			break;
		    ix->idx_array_count++;
		}

		ix->idx_status = 
		    (itcb->tcb_rel.relstat2 & TCB_STATEMENT_LEVEL_UNIQUE)
			? DMT_I_STATEMENT_LEVEL_UNIQUE : 0;
                if (itcb->tcb_rel.relstat2 & (TCB2_LOGICAL_INCONSISTENT |
                        TCB2_PHYS_INCONSISTENT))
                    ix->idx_status |= DMT_I_INCONSISTENT;
                if (itcb->tcb_rel.relstat2 & TCB_PERSISTS_OVER_MODIFIES)
                    ix->idx_status |= DMT_I_PERSISTS_OVER_MODIFIES;

		ix->idx_storage_type = itcb->tcb_rel.relspec;
		ix->idx_pgsize = itcb->tcb_rel.relpgsize;

		/*
		** The following two fields are not used by OPF performs
		** a explicit DMT_SHOW call on each index to find out the
		** information.
		** Until we decide what values we should return here 
		** just zero out the fields.
		*/
		ix->idx_dpage_count = 0;
		ix->idx_ipage_count = 0;

	    }
	}

	/*
	** Unfix the tcb if it was fixed locally here in dmt_show.
	** (Unfix calls null out the tcb pointers.)
	*/
	if ((dmt->dmt_flags_mask & DMT_M_ACCESS_ID) == 0)
	{
	    status = dm2t_unfix_tcb(&tcb, (i4)DM2T_VERIFY, 
		lock_list, log_id, &dmt->error);
	    if (status != E_DB_OK)
		break;
	    if (master_tcb != NULL)
	    {
		status = dm2t_unfix_tcb(&master_tcb, DM2T_VERIFY,
			lock_list, log_id, &dmt->error);
		if (status != E_DB_OK)
		    break;
	    }
	}

	/*
	** Release Table Control lock.
	*/
	if (control_lock)
	{
	    control_lock = FALSE;
	    status = dm2t_control(odcb->odcb_dcb_ptr, table_id.db_tab_base,
		lock_list, LK_N, (i4)0, (i4)0, &dmt->error);
	    if (status != E_DB_OK)
		break;
	}

	return (E_DB_OK); 
    }

    /* 
    ** Error occured, handle releasing any temporary 
    ** resources that are held.
    */
    if (rel_rcb)
    {
	local_status = dm2r_release_rcb(&rel_rcb, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)0, (i4)0, (i4 *)0, &local_error, (i4)0);
	}
    }

    if (tcb && (dmt->dmt_flags_mask & DMT_M_ACCESS_ID) == 0)
    {
	if (master_tcb != NULL)
	{
	    local_status = dm2t_unfix_tcb(&master_tcb, DM2T_VERIFY,
		    lock_list, log_id, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL,
		    NULL, 0, NULL, &local_error, 0);
	    }
	}
	local_status = dm2t_unfix_tcb(&tcb, (i4)DM2T_VERIFY, 
	    lock_list, log_id, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)0, (i4)0, (i4 *)0, &local_error, (i4)0);
	}
    }

    if (control_lock)
    {
	/*
	** Release Table Control lock.
	*/
	local_status = dm2t_control(odcb->odcb_dcb_ptr, table_id.db_tab_base,
	    lock_list, LK_N, (i4)0, (i4)0, &local_dberr);
	if (local_status)
	{
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)0, (i4)0, (i4 *)0, &local_error, (i4)0);
	}
    }

    if (dmt->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmt->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)0, (i4)0, (i4 *)0, &local_error, (i4)0);
	SETDBERR(&dmt->error, 0, E_DM010B_ERROR_SHOWING_TABLE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: check_char - check for caller supplied lock characteristics.
**
** Description:
**	Check the dmt_show_cb for caller supplied lock characteristics
**	to use during the table control lock request.
**
**	The only currently supported option is TIMEOUT.
**
** Inputs:
**      dmt_shw_cb	    - show request control block
**	timeout		    - lock timeout value
** Outputs:
**	timeout		    - set to new timeout value if caller supplied value
**			      is found in the show request control block.
**	dmt_shw_cb
**	    error.err_code  - set to error status:
**				    E_DM000D_BAD_CHAR_ID
**	    error.err_data  - set to parameter # of bad characteristic
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_ERROR                       Bad control block entry.
**
** History:
**	21-jan-1991 (rogerk)
**	    Written to allow timeout values on show commands.
*/
static STATUS
check_char(
DMT_SHW_CB	*dmt_cb,
i4		*timeout)
{                    
    DMT_CHAR_ENTRY  *char_entry =
			(DMT_CHAR_ENTRY *)dmt_cb->dmt_char_array.data_address;
    i4	    char_count = dmt_cb->dmt_char_array.data_in_size / 
				    sizeof(DMT_CHAR_ENTRY);
    i4	    i;

    for (i = 0; i < char_count; i++)
    {
	switch (char_entry[i].char_id)
	{
	case DMT_C_TIMEOUT_LOCK:
	    *timeout = char_entry[i].char_value;
	    break;

	default:
	    SETDBERR(&dmt_cb->error, i, E_DM000D_BAD_CHAR_ID);
	    return (E_DB_ERROR);
	}
    }
    return (E_DB_OK);
}
