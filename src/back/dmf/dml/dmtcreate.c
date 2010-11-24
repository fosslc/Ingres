/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <cm.h>
#include    <st.h>
#include    <sl.h>
#include    <sr.h>
#include    <tr.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0s.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <cui.h>

/**
** Name: DMTCREATE.C - Implements the DMF create temporary table operation.
**
** Description:
**      This file contains the functions necessary to create a temporary table.
**      This file defines:
**
**      dmt_create_temp - Create a temporary table.
**
** History:
**      28-feb-86 (jennifer)
**          Created for Jupiter.
**	12-sep-88 (rogerk)
**	    Fill in table name for temporary tables so error messages will
**	    be nicer.
**	08-feb-1990 (fred)
**	    Add special temporaries for large object temporaries.  So error
**	    messages will be really nice...
**	31-jul-91 (markg)
**	    Added check on new tuple width to dmt_create_temp(). Bug 38959.
**      10-oct-91 (jnash) merged 14-jun-1991 (Derek)
**          Added new allocation and extend options.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements -- allow user access to temporaries.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	17-jul-1992 (rogerk)
**	    Fix unitialized variable problem with dbms-internal temp tables.
**	27-aug-92 (rickh)
**	    Fill in relstat2 for FIPS CONSTRAINTS project.
**	29-August-1992 (rmuth)
**	    Init iirelation.relfhdr to DM_TBL_INVALID_FHDR_PAGENO.
**	16-sep-1992 (bryanp)
**	    Set relpgtype and relcomptype for temporary tables.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Changes:
**	    Changed dm2t_locate_tcb to dm2t_lookup_tabid.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	04-apr-1993 (ralph)
**	    DELIM_IDENT: Use appropriate case for $ingres in dmt_create_temp()
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      10-jan-94 (jrb)
**          Added support for temp tables on work locations; MLSorts project.
**	31-jan-1994 (bryanp) B58534, B58535
**	    Format LK status if LK call fails.
**	    Move XCCB allocation before call to dm2t_temp_build_tcb so that
**		if XCCB allocation fails, we don't end up returning an error
**		yet leaving the temp table created.
**	23-may-1994 (bryanp)
**	    Cleanup a goldrush-reported error-handling problem which occurs
**		when the temporary table lock request fails to generate a new
**		lock value and we instead discover that the TCB for the new
**		temp table already exists.
**	23-may-1994 (andys)
**	    Fix dmt_create_temp to use work locations correctly [bug 62076]
**	26-jun-95 (dougb)
**	    Integrate nick's change for bug 56666 from ingres63p:
**	24-apr-95 (nick)
**	    When creating a temporary table, lock the resource immediately
**	    to enable detection of the fact that we have wrapped around the 
**	    64K available ids and need to skip to another id.
**	 5-jul-95 (dougb) bug # 69382
**	    When creating a temporary table, use a valid transaction id for
**	    its pages.  (Do not pass the address of an XCB instead.)
**	    To avoid consistency point failures (E_DM9397_CP_INCOMPLETE_ERROR)
**	    which can occur if the CP thread runs during or just after
**	    dmt_create_temp(), leave it to dm2t_temp_build_tcb() to insert new
**	    TCB into the HCB queue.
**	06-mar-1996 (stial01 for bryanp)
**	    Accept DMT_C_PAGE_SIZE characteristic, use it to specify page size.
**	    Set relrecord.relpgsize when creating a temporary table.
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**      06-may-1996 (nanpr01)
**          Changed parameters to dm2u_maxreclen. Took out alter_ok param.
**      15-jul-1996 (ramra01 for bryanp)
**          Set relrecord.relversion when creating a temporary table.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for XCCB, manually
**	    initialize remaining fields instead.
**	    Clear relrecord (DMP_RELATION) to zeroes before initializing it
**	    and calling dm2t_temp_build_tcb(); dm2t_alloc_tcb() expects
**	    an authentic-looking relation.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Caller may pass table cache priority in characteristic
**	    DMT_C_TABLE_PRIORITY.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**	25-Aug-1997 (jenjo02)
**	    Fill in xccb_xcb_ptr always. dmc_close_db() depends on this being
**	    here.
**      23-Jul-1998 (wanya01)
**          X-integrate change 432896 (sarjo01)
**          Bug 67867: set TCB_SYS_MAINTAINED flag if temp table has logical
**          keys.
**	23-oct-1998 (somsa01)
**	    In the case of session temporary tables, place the lock on the
**	    SESSION lock list and not on the TRANSACTION lock list.
**	    (Bug #94059)
**	01-feb-1999 (somsa01)
**	    If DMT_LO_LONG_TEMP is set, this is NOT a session-wide
**	    temporary table. This is a table which will last through more
**	    than 1 transaction (internal or external).
**	19-mar-1999 (nanpr01)
**	    Raw location support in Ingres.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	    Set pointer to XCCB in TCB for session temp table.
**	11-Nov-2000 (jenjo02)
**	    When creating a Session temporary table, set its savepoint
**	    id to -1 to indicate that no updates to the table have
**	    yet been done. This prevents the temp table from being
**	    destroyed if a rollback occurs and it has not been updated.
**      01-feb-2001 (stial01)
**          Updated test for valid page types (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	05-Nov-2001 (hanje04)
**	    Removed changes related to bug 105755 because they cause bug
**	    106280
**	15-Jan-2004 (schka24)
**	    Changes for partitioned tables.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      28-apr-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      31-oct-2006 (stial01)
**          dmt_create_temp() init reltotwid (b116996)
**      09-feb-2007 (stial01)
**          Init xccb_blob_temp_flags
**      17-may-2007 (stial01)
**          Added DMT_LOCATOR_TEMP to be created with session lifetime
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**/

/*
** {
** Name:  dmt_create_temp - Create a temporary table.
**
**  INTERNAL DMF call format:  status = dmt_create_temp(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_CREATE_TEMP,&dmt_cb);
**
** Description:
**      This function creates a temporary table.  It is unknown what all the
**      semantics will be, the semantics that are known include:  visible only
**      to the user creating it in the transaction that it is
**      created, it shouldn't make any catalog entries and is automatically
**      deleted at end of transaction.  It cannot be  structured or have 
**      any indices nor can it be modified.  The optimizer is expected
**      to generate plans that use this form of tables versus the full
**      function create DMU.  If the duplicates characteristic is not
**      given, it will assume no duplicates are allowed.  However, since
**      all temporaries are heaps and no duplicate checking is done on
**      heaps, this is not useful until structures other than heaps
**      are supported.
**
**	Per user request, some temporary tables have additional features:
**	    Some have session duration, rather than transaction duration.
**	    Some perform logging, some do not.
**	    Some can have storage structures (i.e., be modified).
**
**	Internal DBMS temporary tables are allowed to be created with no
**	columns. For example, the build-into-the-buffer-manager feature uses
**	this capability in order simply to get a table object which it can use
**	to allocate pages (the pages will later be re-parented into the REAL
**	tcb which contains real attribute definitions).
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Database to create table in.
**          .dmt_tran_id                    Transaction to associate with.
**          .dmt_table			    Table name of table to create.
**          .dmt_flags_mask                 Must be zero or for normal 
**                                          temporary files not "loaded"  
**                                          if it is a request from dbms
**                                          then DMT_DBMS_REQUEST. 
**                                          If this
**                                          is a temporary file for sorted 
**                                          data then it must be DMT_LOAD and
**                                          can include the options 
**                                          DMT_ELIMINATE_DUPS sorter option.
**					    DMT_SESSION_TEMP: this temporary
**						table should have session
**						duration and scope.
**					    DMT_RECOVERY: This temporary table
**						should perform recovery.
**						CURRENTLY, THIS FLAG MAY NOT BE
**						SET. IT IS A PLACEHOLDER FOR A
**						POSSIBLE FUTURE PROJECT TO
**						SUPPORT XACT ABORT ON TEMP TBLS.
**					    DMT_LO_TEMP is used to indicate a
**					        large object temporary file.
**					    DMT_LO_HOLDING_TEMP is used to
**						flag a DMT_LO_TEMP for cleanup
**						at blob-temp-cleanup time.
**          .dmt_location.data_address      Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DB_LOC_NAME.
**          .dmt_location.data_in_size      Length of location data in bytes.
**
**          .dmt_char_array.data_address    Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DMT_CHAR_ENTRY.
**                                          See below for description of 
**                                          <dmt_char_array> entries.
**          .dmt_char_array.data_in_size    Length of char_array data in bytes.
**
**          .dmt_attr_array.ptr_address	    Pointer to array of pointer. Each
**                                          Pointer points to an attribute
**                                          entry of type DMT_ATT_ENTRY.  See
**                                          below for description of entry.
**          .dmt_attr_array.ptr_in_count    The number of pointers in the array.
**	    .dmt_attr_array.ptr_size	    The size of each element pointed at
**          .dmt_key_array.ptr_address	    Pointer to array of pointer. Each
**                                          Pointer points to an attribute
**                                          entry of type DMT_KEY_ENTRY.  See
**                                          below for description of entry.
**          .dmt_key_array.ptr_in_count     The number of pointers in the array.
**	    .dmt_key_array.ptr_size	    The size of each element pointed at
**
**          <dmt_attr_array> entries of type DMT_ATT_ARRAY and must have the
**          following format:
**              .attr_name                  Name of the attribute.
**              .attr_type                  Datatype of the attribute.
**              .attr_size                  Storage size of attribute.
**              .attr_precision                  Precision of attribute.
**              .attr_collID		    Collation ID of attribute.
**              .attr_flags_mask                 Must be zero.
**
**          <dmt_key_array> entries of type DMT_KEY_ARRAY and must have the
**          following format:
**              .key_attr_name              Name of the attribute.
**              .key_order                  Must be DMT_ASCENDING or 
**                                          DMT_DESCENDING.
**
**          <dmt_char_array> entries are of type DMT_CHAR_ENTRY and 
**          must have following values:
**              .char_id                    Characteristic identifier.
**                                          Must be one of the following:
**                                          DMT_C_DUPLICATES. 
**              .char_value                 Value of characteristic. Must be 
**                                          an integer value of DMT_C_ON or
**                                          DMT_C_OFF. 
**
** Outputs:
**      dmt_id;   		            Table identifier of created table.
**      dmt_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK
**                                          E_DM0006_BAD_ATTR_FLAGS
**                                          E_DM0007_BAD_ATTR_NAME
**                                          E_DM0008_BAD_ATTR_PRECISION
**                                          E_DM0009_BAD_ATTR_SIZE
**                                          E_DM000A_BAD_ATTR_TYPE
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM0039_BAD_TABLE_NAME
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**                                          E_DM0077_BAD_TABLE_CREATE
**                                          E_DM0078_TABLE_EXISTS
**                                          E_DM0079_BAD_OWNER_NAME
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**					    with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      28-feb-86 (jennifer)
**          Created for Jupiter.
**	12-sep-88 (rogerk)
**	    Fill in table name for temporary tables so error messages will
**	    be nicer.
**	 6-Feb-89 (rogerk)
**	    Changed tcb hashing to use dcb_id instead of dcb pointer to
**	    form hash value.
**	21-mar-89 (jrb)
**	    Changed call to adc_lenchk to reflect a change in its interface.
**	31-jul-91 (markg)
**	    Added check on new tuple width. If the table we are attempting to
**	    create has a tuple width greater than DB_MAXTUP, return an error.
**	    Bug 38959.
**      10-oct-91 (jnash) merged 14-jun-1991 (Derek)
**          Added new allocation and extend options.
**	16-jan-1992 (bryanp)
**	    Temporary table enhancements:
**		If DMT_DBMS_REQUEST is off, use passed-in owner & table name.
**		Pend DMT_SESSION_TEMP temporaries to the ODCB, not the XCB.
**		Support DMT_RECOVERY flag (that is, recognize that it is a legal
**		flag bit, but don't allow it to be set now, since we don't
**		support recovery processing on session tables.)
**	17-jul-1992 (rogerk)
**	    Fix unitialized variable problem with dbms-internal temp tables.
**	    When DMT_DBMS_REQUEST is used, a table may have no attributes.
**	    In this case, its table width should just be zero.  Initialized
**	    newtable_width so that it does not have wild values in cases when
**	    there is not attribute list.`
**	29-August-1992 (rmuth)
**	    init iirelation.relfhdr to  DM_TBL_INVALID_FHDR_PAGENO.
**	16-sep-1992 (bryanp)
**	    Set relpgtype and relcomptype for temporary tables.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	26-oct-1992 (rogerk)
**	    Reduced Logging Changes:
**	    Changed dm2t_locate_tcb to dm2t_lookup_tabid.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	04-apr-1993 (ralph)
**	    DELIM_IDENT: Use appropriate case for $ingres in dmt_create_temp()
**	21-may-93 (andre)
**	    we will no longer validate table and attribute names - it is up to
**	    the caller to ensure that they are valid names and to present them
**	    to us in correct case.  This will make it possible to create tables
**	    with not necessarily lower case table/attribute names containing
**	    non-alphanumeric characters (as can happen when a name is specified
**	    using a delimited identifier.  This becomes of particular relevance
**	    if you are running with FIPS-compatible settings for default case of
**	    regular and delimited identifiers.
**      20-Oct-1993 (fred)
**          Added support for creating session temp's which have large
**          object attributes.
**      10-jan-94 (jrb)
**          Added support for temp tables on work locations; MLSorts project.
**	    Basically, we now translate "$default" into the current set of
**	    work locations and use them instead.
**	31-jan-1994 (bryanp) B58534, B58535
**	    Format LK status if LK call fails.
**	    Move XCCB allocation before call to dm2t_temp_build_tcb so that
**		if XCCB allocation fails, we don't end up returning an error
**		yet leaving the temp table created.
**	    Fixed LK error message which was logging LK_S as the requested lock
**		mode when in fact we were requesting the lock in LK_X.
**	8-feb-94 (robf)
**          Initialize relation security label value. 
**	23-may-1994 (bryanp)
**	    Cleanup a goldrush-reported error-handling problem which occurs
**		when the temporary table lock request fails to generate a new
**		lock value and we instead discover that the TCB for the new
**		temp table already exists.
**	23-may-1994 (andys)
**	    When checking the list of locations to use, we allow work 
**	    locations as well as data locations. Also add a defensive check 
**	    that will catch unlikely but illegal combinations. [bug 62076]
**	26-jun-95 (dougb)
**	    Integrate nick's change for bug 56666 from ingres63p:
**	24-apr-95 (nick)
**	    Take out lock on the new table id - if this can't be obtained
**	    or is granted but isn't a new lock for this particular transaction,
**	    then start over with the next table id.  #56666
**	    Also, simplify code to avoid leaving this function while
**	    holding hcb->hcb_mutex.
**	 5-jul-95 (dougb) bug # 69382
**	    When creating a temporary table, use a valid transaction id for
**	    its pages.  (Do not pass the address of an XCB instead.)
**	    To avoid consistency point failures (E_DM9397_CP_INCOMPLETE_ERROR)
**	    which can occur if the CP thread runs during or just after
**	    dmt_create_temp(), leave it to dm2t_temp_build_tcb() to insert new
**	    TCB into the HCB queue.
**		24-november-1995 (angusm)
**		Validation here should allow aux work locations as well as data
**		and work locations. (bug 71074).
**      23-jan-1996(angusm)
**          Apply DMT_DBMS_REQUEST as a flag to TCB
**	06-mar-1996 (stial01 for bryanp)
**	    Accept DMT_C_PAGE_SIZE characteristic, use it to specify page size.
**	    Set relrecord.relpgsize when creating a temporary table.
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01, nanpr01)
**          Changed parameters to dm2u_maxreclen.
**          Now we need to fall through the char_array logic even for
**	    load. Otherwise we will not be able to set page_size more than
**	    2K.
**      06-may-1996 (nanpr01)
**          Changed parameters to dm2u_maxreclen. Took out alter_ok param.
**      15-jul-1996 (ramra01 for bryanp)
**          Set relrecord.relversion when creating a temporary table.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for XCCB, manually
**	    initialize remaining fields instead.
**	    Clear relrecord (DMP_RELATION) to zeroes before initializing it
**	    and calling dm2t_temp_build_tcb(); dm2t_alloc_tcb() expects
**	    an authentic-looking relation.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Caller may pass table cache priority in characteristic
**	    DMT_C_TABLE_PRIORITY.
**	3-jun-97 (nanpr01)
**	    Fix bug 78352 : When transactions are rolled back, temporary
**	    tables that were updated does not get dropped as documented.
**      23-Jul-1998 (wanya01)
**          X-integrate change 432896 (sarjo01)
**          Bug 67867: set TCB_SYS_MAINTAINED flag if temp table has logical
**          keys.
**	23-oct-1998 (somsa01)
**	    In the case of session temporary tables, place the lock on the
**	    SESSION lock list and not on the TRANSACTION lock list.
**	    (Bug #94059)
**	01-feb-1999 (somsa01)
**	    If DMT_LO_LONG_TEMP is set, this is NOT a session-wide
**	    temporary table. This is a table which will last through more
**	    than 1 transaction (internal or external).
**	 3-mar-1998 (hayke02)
**	    We now set TCB_SESSION_TEMP in relstat2 rather than relstat.
**	12-may-1999 (thaju02)
**	    If request for LK_DB_TEMP_ID lock is interrupted, report 
**	    E_DM0065. (B96940)
**	28-aug-2001 (hayke02)
**	    Use new #define for DB_TEMP_TAB_NAME for internal temporary
**	    table name of "$<Temporary Table>".
**	08-Nov-2000 (jenjo02)
**	    Set pointer to XCCB in TCB for session temp table.
**      20-May-2000 (hanal04, stial01 & thaju02) Bug 101418 INGSRV 1170
**          Move locks for global temporary table id back onto the Session
**          list. The previous hole in the 23-oct-1998 change was that the
**          lock request needed to be L_X not L_IX. The lock mode change
**          has been applied outside of oping20 with change 445801.
**	21-Dec-2000 (thaju02)
**	    (hanje04) X-Integ of change 451613
**	    For gtt set savepoint id in xccb, in case the dgtt as 
**	    select query fails and the gtt needs to be destroyed upon
**	    abort to savepoint. (B103567)
**	31-Oct-2001 (thaju02)
**	    For DMT_LO_TEMP, note in the tcb_status that this is a 
**	    large object (blob) temporary. (B106154)
**	26-Mar-2002 (thaju02)
**	    Backed out previous change for bug 103567, caused regression.
**	05-Nov-2001 (hanje04)
**	    Removed changes related to bug 103567 because thay cause bug
**	    106280.
**	17-Mar-2003 (jenjo02)
**	    Changed temp table id lock mode from IX (which allows
**	    sharers) to X. "LK_NEW_LOCK" means "new to this lock list",
**	    not "new resource and not locked by anyone else". 
**	    STAR 12527188, 12559920, etal.
====
**	15-Jan-2004 (schka24)
**	    Update hash-table mutexing scheme.
**	18-Mar-2004 (jenjo02)
**	    Clean out attribute fields the caller may not have that
**	    don't apply to temp tables.
**	    Init XCCB to background zeros so that dmd_dmp_pool doesn't
**	    choke in the unlikely but witnessed event the server 
**	    crashes before the XCCB is initialized.
**	    Free the XCCB memory if the dm2t_temp_build_tcb() fails
**	    lest we leak.
**	7-Apr-2004 (schka24)
**	    Remove default-id part of above fix, session temps kinda
**	    need their default info.  Callers will just have to practice
**	    better hygiene.  (FYI the above fix was to cure a semaphore
**	    init failure caused by garbage left over in a default-id,
**	    which looked like "ISEM" and got into DMF memory like a virus.)
**	10-May-2004 (schka24)
**	    ODCB scb need not equal thread SCB.  We want parent (ODCB)
**	    scb here.  All the good stuff should be in both, but be safe.
**	    Kill long-temp flag, not used now.
**	11-Jun-2004 (schka24)
**	    Above change caused session temps to use parent lock ID, wrong
**	    in parallel query situation -- revert to thread SCB lock id.
**	    Set xccb is-session flag for delete convenience.
**	    Protect odcb xccb list for || query, since I'm in here anyway.
**	21-Jun-2004 (schka24)
**	    Reinstate another blob holding-temp flag and use it to set
**	    the I-am-a-blob-temp flag in the XCCB.  Thus all temp cleanups
**	    are centered around the XCCB list(s) and there's no separate
**	    blob-cleanup list to deal with.
**	16-dec-04 (inkdo01)
**	    Added a collID.
**	18-Jan-2005 (jenjo02)
**	    Point XCCB->xccb_t_tcb to TempTable's TCB for quick 
**	    lookup by dmt_show().
**      12-Sep-2005 (hanal04) Bug 114348 INGSRV3264
**          Extend the range of temporary table IDs to all negative
**          values not just the first 65535.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	15-Aug-2006 (jonj)
**	    Extracted code from here to make dm2uGetTempTableId(),
**	    dm2uGetTempTableLocs(), also used to create Temp indexes.
**	11-Sep-2006 (jonj)
**	    Don't dmxCheckForInterrupt if internal request.
**	    Transaction may not be at an atomic point
**	    in execution as required for LOGFULL_COMMIT.
**	    Get LONGTERM memory for session temps.
**       27-nov-2006 (huazh01)
**          Extend the fix for b113199 so that it covers temp
**          table. This fixes b117195.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**	7-Aug-2008 (kibro01) b120592
**	    Mark the fact that we're inside an online modify so that the
**	    temporary table creation logic knows that it's acceptable to create
**	    it in an alternate location
**	23-Mar-2010 (kschendel) SIR 123448
**	    XCB's XCCB list now has to be mutex protected.
**	20-Jul-2010 (kschendel) SIR 124104
**	    Pass in compression too.
*/
DB_STATUS
dmt_create_temp(
DMT_CB  *dmt_cb)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DMT_CB	    *dmt = dmt_cb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;		/* Thread's SCB */
    DML_ODCB	    *odcb;
    DMP_DCB         *dcb;
    DML_XCCB        *xccb;
    DMP_TCB         *t = 0;    
    DMP_HASH_ENTRY  *h;
    DM_MUTEX	    *hash_mutex;
    DMF_ATTR_ENTRY  **attr_entry;
    DMT_KEY_ENTRY   **key_entry = 0;
    i4	    	    attr_count;
    i4         	    key_count = 0;
    DMT_CHAR_ENTRY  *char_entry;
    i4         	    char_count;
    i4	    	    error, local_error;
    DB_STATUS	    status;
    i4	    	    i,j;
    i4         	    newtable_width;    
    i4         	    bad_attr;    
    i4         	    structure;    
    i4	    	    allocation = DM_TTBL_DEFAULT_ALLOCATION;
    i4	    	    extend = DM_TTBL_DEFAULT_EXTEND;
    i4		    compression = TCB_C_NONE;
    i4	    	    page_type = TCB_PG_INVALID;
    i4	    	    page_size = svcb->svcb_page_size;
    DMP_RELATION    relrecord;
    DB_DATA_VALUE   adc_dv1;
    ADF_CB          adf_cb; 
    i4         	    loadflag;
    i4         	    duplicates;
    DB_LOC_NAME	    *location;
    i4	    	    loc_count;
    DB_TAB_ID	    temp_table_id;
    i4              has_extensions = 0;
    i4	    	    tbl_pri = 0;
    i4              sys_maintained = 0;
    DM_OBJECT	    *locs_block;
    i4	    	    lk_list_id;
    i4		    pgtype_flags;
    bool            used_default_page_size = TRUE;
    i4		    dm0mFlags;
    bool	    from_online_modify = FALSE;

    CLRDBERR(&dmt->error);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);

    duplicates = -1;
    status = E_DB_ERROR;
    for (;;)
    {

	/* Check the input parameters for correctness. */

	locs_block = (DM_OBJECT*)NULL;

    
	/* DMT_RECOVERY is a valid bitmask, but it's not currently supported */

	if (dmt->dmt_flags_mask != 0)
	    if (dmt->dmt_flags_mask &
		    ~(DMT_LOAD | DMT_DBMS_REQUEST | DMT_LO_HOLDING_TEMP
			| DMT_LO_TEMP | DMT_SESSION_TEMP | DMT_LOCATOR_TEMP
			| DMT_ONLINE_MODIFY))
	{
	    SETDBERR(&dmt->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	xcb = (DML_XCB *)dmt->dmt_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state && !(dmt->dmt_flags_mask & DMT_DBMS_REQUEST) )
	    dmxCheckForInterrupt(xcb, &error);

	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dmt->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dmt->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dmt->error, 0, E_DM0064_USER_ABORT);
	    else if (xcb->xcb_state & XCB_WILLING_COMMIT)
		SETDBERR(&dmt->error, 0, E_DM0132_ILLEGAL_STMT);
	    break;
	}

	scb = xcb->xcb_scb_ptr;
	odcb = (DML_ODCB *)dmt->dmt_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmt->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}
	dcb = odcb->odcb_dcb_ptr;

	loc_count = dmt->dmt_location.data_in_size / sizeof(DB_LOC_NAME);
	location  = (DB_LOC_NAME *)dmt->dmt_location.data_address;

	/* Set up the location information */
	if ( status = dm2uGetTempTableLocs(dcb,
					   odcb->odcb_scb_ptr,
					   &location,
					   &loc_count,
					   &locs_block,
					   &dmt->error) )
	{
	    break;
	}

	if ((dmt->dmt_flags_mask & DMT_DBMS_REQUEST) != 0)
	{
	    /* set up ingres as owner of temporary files. */

	    MEcopy((PTR)scb->scb_cat_owner,
		   sizeof(DB_OWN_NAME),
		   (PTR)&dmt->dmt_owner);
	}

	/* Set structure to heap. */

	structure = DB_HEAP_STORE;

	/* Check the attribute values. */

	attr_entry = (DMF_ATTR_ENTRY **)dmt->dmt_attr_array.ptr_address;
	attr_count = dmt->dmt_attr_array.ptr_in_count; 
	newtable_width = 0;
	if (dmt->dmt_attr_array.ptr_address && 
	    dmt->dmt_attr_array.ptr_in_count &&
            (dmt->dmt_attr_array.ptr_size == sizeof(DMF_ATTR_ENTRY)))
	{
	    for (i = 0; i < attr_count; i++)
	    {
		/*
		** we no longer validate and lowercase attribute names; it is
		** entirely up to the caller to validate attribute names and
		** present them to us in appropriate case
		*/

		for (j = 0; j < i; j++)
		{
		    /* 
		    ** Compare this attribute name against other 
		    ** already given, they cannot be the same.
		    */

		    if (MEcmp(attr_entry[j]->attr_name.db_att_name,
			      attr_entry[i]->attr_name.db_att_name,
			      sizeof(DB_ATT_NAME)) == 0 )
		    {
			break;
		    }	    	    
		}
		if (j < i)
		    break;
                
		adf_cb.adf_errcb.ad_ebuflen = 0;
		adf_cb.adf_errcb.ad_errmsgp = 0;
		adf_cb.adf_maxstring = DB_MAXSTRING;
		adc_dv1.db_datatype = attr_entry[i]->attr_type;
                adc_dv1.db_length   = attr_entry[i]->attr_size;
                adc_dv1.db_prec     = attr_entry[i]->attr_precision;
                adc_dv1.db_collID   = attr_entry[i]->attr_collID;
	        status = adc_lenchk(&adf_cb, FALSE, &adc_dv1, NULL);
		if (status != E_DB_OK)
		{
		    if (adf_cb.adf_errcb.ad_errcode == E_AD2004_BAD_DTID)
			SETDBERR(&dmt->error, i, E_DM000A_BAD_ATTR_TYPE);
		    else
			SETDBERR(&dmt->error, i, E_DM0009_BAD_ATTR_SIZE);
	            bad_attr = TRUE;
		    break;
    		}	    	

		/* check for appropriate attribute flag */
		if (attr_entry[i]->attr_flags_mask & ~DMU_F_LEGAL_ATTR_BITS)
		{
		    SETDBERR(&dmt->error, 0, E_DM0006_BAD_ATTR_FLAGS);
		    bad_attr = TRUE;
		    break;
		}

		newtable_width += attr_entry[i]->attr_size;
	    }
	    if (dmt->error.err_code)
		break;
	}
	else if ((dmt->dmt_flags_mask & DMT_DBMS_REQUEST) == 0)
	{
	    /* Only an internal DBMS request can omit the columns */
	    SETDBERR(&dmt->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	if ((dmt->dmt_flags_mask & DMT_LOAD) == DMT_LOAD)
	{
	    if (dmt->dmt_key_array.ptr_address && 
                      dmt->dmt_key_array.ptr_in_count 
               && (dmt->dmt_key_array.ptr_size ==
		    sizeof(DMT_KEY_ENTRY)))
	    {
		key_entry = (DMT_KEY_ENTRY**) dmt->dmt_key_array.ptr_address;
		key_count = dmt->dmt_key_array.ptr_in_count; 
		/* break; */
	    }
	    /* break; */
	}

	/*  Check for characteristics. */

	if (dmt->dmt_char_array.data_address && 
		dmt->dmt_char_array.data_in_size)
	{
	    char_entry = (DMT_CHAR_ENTRY*) dmt->dmt_char_array.data_address;
	    char_count = dmt->dmt_char_array.data_in_size / 
			    sizeof(DMT_CHAR_ENTRY);
	    for (i = 0; i < char_count; i++)
	    {
		switch (char_entry[i].char_id)
		{
		case DMT_C_DUPLICATES:
		    duplicates = char_entry[i].char_value == DMT_C_ON;
		    break;

		case DMT_C_ALLOCATION:
		    allocation = char_entry[i].char_value;
		    break;

		case DMT_C_EXTEND:
		    extend = char_entry[i].char_value;
		    break;

		case DMT_C_HAS_EXTENSIONS:
		    has_extensions = char_entry[i].char_value;
		    break;

		case DMT_C_PAGE_SIZE:
		    page_size = char_entry[i].char_value;
                    used_default_page_size = FALSE;
		    if (page_size == 0)
		    {
			SETDBERR(&dmt->error, i, E_DM000D_BAD_CHAR_ID);
			return (E_DB_ERROR);
		    }
		    break;

		case DMT_C_PAGE_TYPE:
		    page_type = char_entry[i].char_value;
		    if (!DM_VALID_PAGE_TYPE(page_type))
		    {
			SETDBERR(&dmt->error, i, E_DM000D_BAD_CHAR_ID);
			return (E_DB_ERROR);
		    }
		    break;

		case DMT_C_TABLE_PRIORITY:
		    tbl_pri = char_entry[i].char_value;
		    break;

                case DMT_C_SYS_MAINTAINED:
                    sys_maintained = char_entry[i].char_value == DMT_C_ON;
                    break;

		case DMT_C_COMPRESSED:
		    compression = char_entry[i].char_value;
		    break;

		default:
		    SETDBERR(&dmt->error, i, E_DM000D_BAD_CHAR_ID);
		    return (E_DB_ERROR);
		}
	    }
	}

	if (page_type == TCB_PG_INVALID)
	{
	    /*
	    ** get and or validate page type
	    ** If called from modify, the page type may have already been
	    ** determined
	    */
	    if (dmt->dmt_flags_mask & DMT_LO_TEMP)
		pgtype_flags = DM1C_CREATE_ETAB; 
	    else
		pgtype_flags = DM1C_CREATE_DEFAULT;

	    /* should have attributes and table width if not modify */
	    if (!newtable_width)
	    {
		SETDBERR(&dmt->error, 0, E_DM0103_TUPLE_TOO_WIDE);
		break;
	    }

	    status = dm1c_getpgtype(page_size, pgtype_flags, 
			newtable_width, &page_type);
	    if (status != E_DB_OK && !used_default_page_size)
	    {
		SETDBERR(&dmt->error, 0, E_DM0103_TUPLE_TOO_WIDE);
		break;
	    }
	}

        /*
        ** bug 117195:
        ** If the default page size won't work or
        ** or will work only with rows spanning pages
        ** look for a bigger page size that will hold the row
        */
        if ((status != E_DB_OK
	    || newtable_width > dm2u_maxreclen(page_type, page_size))
            && used_default_page_size)
        {
            i4 pgsize;
            i4 pgtype;

            /*
            ** Current default page size does not fit the row.. So pick one
            ** which does..
            */
            for (pgsize = page_size*2; pgsize <= 65536; pgsize *= 2)
            {

                /* choose page type */
                status = dm1c_getpgtype(pgsize, pgtype_flags,
                            newtable_width, &pgtype);

                if (status != E_DB_OK || 
			newtable_width > dm2u_maxreclen(pgtype, pgsize)) 
                    continue;

                page_size = pgsize;
                page_type = pgtype;
                break;
            }
        }

	if (newtable_width > dm2u_maxreclen(page_type, page_size))
	{
	    /*
	    ** Allow rows to span page size up to a maximum of DM_TUPLEN_MAX
	    ** but only if the table is defined with duplicates rows allowed
	    */
	    if (newtable_width > DM_TUPLEN_MAX_V5 || duplicates != 1)
	    {
		SETDBERR(&dmt->error, 0, E_DM0103_TUPLE_TOO_WIDE);
		break;
	    }
	}

    	/* If it gets here, then everything was OK. */

        break;
    }
    if (dmt->error.err_code)
    {
	if ( locs_block )
	    dm0m_deallocate((DM_OBJECT **)&locs_block);

	return (E_DB_ERROR);
    }

    for (;;)
    {
#if 0
	TRdisplay("dmt_create_temp, creating temp tbl with page size %d\n",
		page_size);
#endif

	/* 
	** All parameters are acceptable, now check the relation name.
	*/

	if ((dmt->dmt_flags_mask & DMT_DBMS_REQUEST) == 0)
	{
	    /*
	    ** we no longer validate and lowercase table name; it is entirely
	    ** up to the caller to validate table name and present it to us in
	    ** appropriate case
	    */

	    status = dm2t_lookup_tabid(dcb, (DB_TAB_NAME *)&dmt->dmt_table,
			(DB_OWN_NAME *)&dmt->dmt_owner, &temp_table_id, &dmt->error);
	    if (status == E_DB_OK)
		break;
	}
	else
	{
	    /*
	    ** For now, put phoney table name in the name descriptor so error
	    ** messages will make sense.  If QEF ever begins putting names into
	    ** temp files, then this could be taken out.
	    */
	    if (dmt->dmt_flags_mask & DMT_LO_TEMP)
	    {
		static  i4	    temp_counter = 0;
		DB_TAB_NAME		    temp_name;
		
		STprintf(temp_name.db_tab_name,
				"<DMPE Temp #%d>",
				temp_counter++);
		STmove(temp_name.db_tab_name, ' ',
		    sizeof(dmt->dmt_table.db_tab_name),
		    dmt->dmt_table.db_tab_name);
	    }
	    else
	    {
		STmove(DB_TEMP_TABLE, ' ',
		    sizeof(dmt->dmt_table.db_tab_name),
		    dmt->dmt_table.db_tab_name);
	    }
	}

	for (;;)
	{
	    /*
	    ** If this a session-wide temporary table, then tie all locks
	    ** to the session's lock list and allocate LONGTERM memory
	    ** for the XCCB. Shared/parallel threads may also share
	    ** the temp table and we can't have the XCCB memory
	    ** implicitly deallocated when one of those threads
	    ** terminates.
	    */
	    if (dmt->dmt_flags_mask & (DMT_SESSION_TEMP | DMT_LO_TEMP | DMT_LOCATOR_TEMP) )
	    {
		lk_list_id = scb->scb_lock_list;
		dm0mFlags = DM0M_LONGTERM;
	    }
	    else
	    {
		lk_list_id = xcb->xcb_lk_id;
		dm0mFlags = DM0M_SHORTTERM;
	    }
	
	    /* 
	    ** First get a unique id for the table.  This is done
	    ** by getting a lock on the database which keeps track of
	    ** temporary table ids.
	    */

	    if ( status = dm2uGetTempTableId(dcb,
					     lk_list_id,
					     &dmt->dmt_id,
					     FALSE,		/* not an index */
					     &dmt->dmt_id,
					     &dmt->error) )
	    {
		SETDBERR(&dmt->error, 0, E_DM0077_BAD_TABLE_CREATE);
		break;
	    }

	    /* 
	    ** Everything is verified now, build a relation record for the
	    ** temporary table. 
	    */

#ifdef xDEBUG
    {CS_SID sid; CSget_sid(&sid);
    TRdisplay("%@ Sess %x: temp open id %d, name %~t, flags %v\n",
    sid, dmt->dmt_id.db_tab_base, sizeof(DB_TAB_NAME),&dmt->dmt_table,
    "NOWAIT,FORCE_CLOSE,LOAD,DBMS_REQ,SHOW_STAT,TIMESTAMP,SESSION_TEMP,RECOVERY,LO_TEMP,CONSTRAINT", dmt->dmt_flags_mask);
    }
#endif

	    
	    /*
	    ** Start with a zeroed relation record. dm2t_alloc_tcb() expects
	    ** an authentic-looking relation.
	    */
	    MEfill(sizeof(relrecord), 0, (char *)&relrecord);

	    relrecord.reltid.db_tab_base = dmt->dmt_id.db_tab_base;
	    relrecord.reltid.db_tab_index = dmt->dmt_id.db_tab_index;
	    MEcopy((char *)&dmt->dmt_table, sizeof(DB_TAB_NAME),
		    (char *)&relrecord.relid);
	    MEcopy((char *)&location[0], sizeof(DB_LOC_NAME),
		     (char *)&relrecord.relloc);
	    MEcopy((char *)&dmt->dmt_owner, sizeof(DB_OWN_NAME),
		     (char *)&relrecord.relowner);
	    relrecord.relatts = attr_count;
	    relrecord.relwid = newtable_width;	        
	    relrecord.reltotwid = newtable_width;
	    relrecord.relkeys = key_count;
	    relrecord.relspec = TCB_HEAP;
	    relrecord.relpages = 1;
	    relrecord.relprim = 1;
	    relrecord.relmain = 1;
	    relrecord.relfhdr = DM_TBL_INVALID_FHDR_PAGENO;
	    relrecord.relloccount = loc_count;
	    relrecord.relcmptlvl = DMF_T_VERSION;
	    TMget_stamp((TM_STAMP *)&relrecord.relstamp12);
	    if (duplicates == 1)
		relrecord.relstat |= TCB_DUPLICATES;	    
            if (sys_maintained)
                relrecord.relstat |= TCB_SYS_MAINTAINED;
	    if (compression != TCB_C_NONE)
		relrecord.relstat |= TCB_COMPRESSED;
	    if ((dmt->dmt_flags_mask != 0) &&
		(dmt->dmt_flags_mask & DMT_SESSION_TEMP))
		relrecord.relstat2 |= TCB_SESSION_TEMP;
	    if (has_extensions)
		relrecord.relstat2 |= TCB2_HAS_EXTENSIONS;
	    if (dmt->dmt_flags_mask & DMT_LO_TEMP)
		relrecord.relstat2 |= TCB2_EXTENSION;
	    relrecord.reldfill = 100;
	    relrecord.relallocation = allocation;
	    relrecord.relextend = extend;
	    relrecord.relpgtype = page_type;
	    relrecord.relcomptype = compression;
	    relrecord.relpgsize = page_size;
	    relrecord.reltcpri  = tbl_pri;

	    /* Figure out which hash queue this table will connect to. */

	    hash_mutex = &dmf_svcb->svcb_hcb_ptr->hcb_mutex_array[
		HCB_MUTEX_HASH_MACRO(dcb->dcb_id, relrecord.reltid.db_tab_base)];
	    h = &dmf_svcb->svcb_hcb_ptr->hcb_hash_array[
		HCB_HASH_MACRO(dcb->dcb_id, relrecord.reltid.db_tab_base, relrecord.reltid.db_tab_index)];
	    dm0s_mlock(hash_mutex);
	    for (
		t = h->hash_q_next;
		t != (DMP_TCB*) h;
		t = t->tcb_q_next
		)
	    {
		if (t->tcb_dcb_ptr == dcb && t->tcb_rel.reltid.db_tab_base 
			 == relrecord.reltid.db_tab_base)
		{	
		    /* Should never find TCB already in existence. */
		    status = E_DB_ERROR;
		    SETDBERR(&dmt->error, 0, E_DM0077_BAD_TABLE_CREATE);
		    break;
		}
	    }

	    dm0s_munlock(hash_mutex);

	    if (status)
		break;

	    /* Get zeroed memory for XCCB */
	    status = dm0m_allocate((i4)sizeof(DML_XCCB), dm0mFlags | DM0M_ZERO, 
		(i4)XCCB_CB,
		(i4)XCCB_ASCII_ID, (char *)xcb, (DM_OBJECT **)&xccb, 
		&dmt->error);
	    if (status != OK)
	    {
		uleFormat( &dmt->error, 0, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
		uleFormat( NULL, E_DM901F_BAD_TABLE_CREATE, NULL, ULE_LOG , NULL, 
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 2, sizeof(DB_DB_NAME),
			&odcb->odcb_dcb_ptr->dcb_name, 
			sizeof(DB_TAB_NAME), &dmt->dmt_table);
		SETDBERR(&dmt->error, 0, E_DM0077_BAD_TABLE_CREATE);
		break;
	    }
# ifdef xDEV_TST
TRdisplay("\nCREATE TEMP TABLE id %d %d, WIDTH %d\n",
		dmt->dmt_id.db_tab_base,
		dmt->dmt_id.db_tab_index,
		relrecord.relwid);
# endif
	    
	    /* Build a TCB for the temporary table. */
	    
	    /* Note that we're inside an online modify
	    ** (kibro01) b120592
	    */
	    if ((dmt->dmt_flags_mask & DMT_ONLINE_MODIFY) == DMT_ONLINE_MODIFY)
		from_online_modify = TRUE;

	    loadflag = DM2T_NOLOAD;
	    if ((dmt->dmt_flags_mask & DMT_LOAD) == DMT_LOAD)
		loadflag = DM2T_LOAD;
	    /* Tell dm2t making a Base TCB, not an index */
	    t = (DMP_TCB*)NULL;
	
	    status = dm2t_temp_build_tcb(dcb, &relrecord.reltid, 
					 &xcb->xcb_tran_id,
			     lk_list_id, xcb->xcb_log_id, &t, &relrecord,
			    attr_entry, attr_count,
			    (DM2T_KEY_ENTRY **)key_entry, key_count, loadflag, 
			     (dmt->dmt_flags_mask & DMT_RECOVERY) != 0,
			     (i4)0, location, loc_count, (i4*)NULL,
			     &dmt->error,
			     from_online_modify);

	    if (status != OK)
	    {
		/* Toss the XCCB memory */
		dm0m_deallocate((DM_OBJECT **)&xccb);
		   
		uleFormat( &dmt->error, 0, NULL, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 0);

		uleFormat( NULL, E_DM901F_BAD_TABLE_CREATE, NULL, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 2, sizeof(DB_DB_NAME),
			&odcb->odcb_dcb_ptr->dcb_name,
			sizeof(DB_TAB_NAME), &dmt->dmt_table);
		SETDBERR(&dmt->error, 0, E_DM0077_BAD_TABLE_CREATE);
		break;
	    }

	    /*
	    ** bug 72806
	    ** DMT_DBMS_REQUEST tables
	    ** on multiple locations shouldn't do group read.
	    */
	    if (dmt->dmt_flags_mask & DMT_DBMS_REQUEST && loc_count > 1)
		t->tcb_table_io.tbio_cache_flags |= TBIO_CACHE_NO_GROUPREAD;

	    /* 
	    ** Pend the DESTROY for this CREATE to the transaction control block
	    ** so it is destroyed at transaction end time.  This is just a 
	    ** reliablity check, caller should destroy it.
	    **
	    ** If this is a session lived temporary, pend this action to the
	    ** session control block, instead of the transaction control block.
	    ** For user-created session temporaries, this is more than just a
	    ** reality check: the user is not required to explicitly drop his
	    ** temporaries and may well simply end his session and expect us to
	    ** destroy them for him.
	    */

	    xccb->xccb_t_dcb = dcb;
	    xccb->xccb_t_tcb = t;
	    xccb->xccb_t_table_id.db_tab_base = relrecord.reltid.db_tab_base;
	    xccb->xccb_t_table_id.db_tab_index= relrecord.reltid.db_tab_index;
	    xccb->xccb_operation = XCCB_T_DESTROY;
	    xccb->xccb_is_session = FALSE;
	    xccb->xccb_blob_temp_flags = 0;
	    xccb->xccb_blob_locator = 0;

	    /* Session or transaction lifetime?
	    ** Blob temps get session lifetime, see explanations in dmpe_temp
	    ** Locator temp gets session lifetime, see dmpe_create_locator_temp
	    */
	    if ((dmt->dmt_flags_mask & (DMT_SESSION_TEMP|DMT_LO_TEMP|DMT_LOCATOR_TEMP)) == 0)
	    {
		dm0s_mlock(&xcb->xcb_cq_mutex);
		xccb->xccb_q_next = xcb->xcb_cq_next;
		xccb->xccb_q_prev = (DML_XCCB*) &xcb->xcb_cq_next;
		xcb->xcb_cq_next->xccb_q_prev = xccb;
		xcb->xcb_cq_next = xccb;
		dm0s_munlock(&xcb->xcb_cq_mutex);
		xccb->xccb_xcb_ptr = xcb;
		xccb->xccb_sp_id = xcb->xcb_sp_id;
	    }
	    else
	    {
		/* Pend a destroy to the Open Database control block */

		dm0s_mlock(&odcb->odcb_cq_mutex);
		xccb->xccb_q_next = odcb->odcb_cq_next;
		xccb->xccb_q_prev = (DML_XCCB *) &odcb->odcb_cq_next;
		odcb->odcb_cq_next->xccb_q_prev = xccb;
		odcb->odcb_cq_next = xccb;
		dm0s_munlock(&odcb->odcb_cq_mutex);
		xccb->xccb_xcb_ptr = xcb;
		xccb->xccb_sp_id = -1;
		xccb->xccb_is_session = TRUE;
		if (dmt->dmt_flags_mask & DMT_LO_HOLDING_TEMP)
		    xccb->xccb_blob_temp_flags |= BLOB_HOLDING_TEMP;
		/* Point TCB at XCCB */
		t->tcb_temp_xccb = xccb;
	    }

	    if (dmt->dmt_flags_mask & DMT_LOCATOR_TEMP)
		xccb->xccb_blob_temp_flags |= BLOB_LOCATOR_TBL;

	    break;

	} /* end for */
	break;
    }

    if ( locs_block )
	dm0m_deallocate((DM_OBJECT **)&locs_block);

    if (dmt->error.err_code)
    {
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}
