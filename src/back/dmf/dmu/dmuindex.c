/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <tm.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <tr.h>
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
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm0s.h>
#include    <di.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dmu.h>
#include    <st.h>
#include    <dm0pbmcb.h>

/**
**
** Name: DMUINDEX.C
**
** Description:
**      This file contains the functions necessary to create an index 
**      on a table.
**      Common routines used by all DMU functions are not included here, but
**      are found in DMUCOMMON.C.  This file defines:
**    
**      dmu_index()      -  Routine to perfrom the normal create index
**                          operation.
**	dmuIndexSetup()	 -  Code common to dmu_index, dmu_pindex, to make
**			    a DM2U_INDEX_CB ready for the create.
**
** History:
**      01-sep-85 (jennifer) 
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**      27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Added gateway attribute arrays
**	    to the DMU_CB control block.
**	17-aug-1989 (Derek)
**	    Added DAC success auditing.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	11-jan-1991 (bryanp)
**	    Added support for Btree index compression.
**	14-jun-1991 (Derek)
**	    Added support for new allocation and extend options.
**	16-jul-1991 (bryanp)
**	    B38527: Add new 'reltups' argument to dm2u_index.
**      20-may-1992 (schang)
**          GW merge.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Pass dmu_char_array to dm2u_index for gateway use.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
*	18-nov-92 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	    All auditing now thorough dma_write_audit()
**	25-feb-93 (rickh)
**	    Added relstat2 argument to dm2u_index call and added support
**	    for the corresponding DMU characteristics.
**	15-may-93 (rmuth)
**	    Add support which enables an index to be created in 
**	    "concurrent_access" mode. This causes the index to be built
**	    by only taking out share locks. Cuurently this is only
**	    supported on base tables which are READONLY.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	19-aug-93 (stephenb)
**	    Changed call to dma_write_audit() to audit index name on index
**	    creation rather than table name, to be consistent with drop index
**	    audit records.
**	18-oct-93 (rogerk)
**	    Initialize min_pages, max_pages values.
**	    Fix prototype warnings.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**      06-jan-95 (nanpr01)
**          Bug # 66028. Set the TCB2_TBL_RECOVERY_DISALLOWED flag on 
**          relstat2 field at the time of index creation.
**	10-feb-95 (cohmi01)
**	    For RAW I/O, add DB_RAWEXT_NAME parm to dm2u_index.
**	05-mary-1995 (shero03)
**	    For RTREE, added dimension, range, and data_clustered
**	06-mar-1996 (stial01 for bryanp)
**	    Recognize DMU_PAGE_SIZE characteristic; pass page_size argument to
**		dm2u_create.
**      19-apr-1996 (stial01)
**          Validate page_size parameter
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Use DMU_TABLE_PRIORITY to set table priority and pass on to
**	    dm2u_index().
**	15-apr-1997 (shero03)
**	    Remove dimension and hilbertsize - obtain it later with a 
**	    getklv call.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
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
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-feb-2001 (stial01)
**          Updated test for valid page types (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      14-apr-2004 (stial01)
**          E_DM007D_BTREE_BAD_KEY_LENGTH should return klen and max klen
**	11-Mar-2005 (thaju02)
**	    Use $online idx relation info. (B114069)
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**/

/*{
** Name: dmu_index - Creates an index on a table.
**
**  INTERNAL DMF call format:      status = dmu_index(&dmu_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMU_INDEX_TABLE,&dmu_cb);
**
** Description:
**    The dmu_index function handles the creation of indices.
**    This dmu function is allowed inside a user specified transaction.
**    The table name must not be the same as a system table name and must 
**    not be the same name as any other table owned by the same user. 
**    The table that this index is defined on must exist and must be
**    identified by use of the internal identifier obtained from a dmt_show
**    operation.  It is assumed that the caller has verified that the
**    base table is owned by the same user.
**
**      Note: For B1 secure server, this routine must make sure
**      that the security label of the requester is exactly the
**      same as the security label on the table.  If is not 
**      identical, the operation is rejected with the error
**      E_DB0125_DDL_SECURITY_ERROR.
**
** Inputs:
**      .dmu_cb
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**          .dmu_flags_mask                 Must be zero.
**          .dmu_tbl_id                     Internal name of table to be 
**                                          indexed.
**          .dmu_index_name                 External name of index to be 
**                                          created.
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
**          .dmu_olocation.data_address     This holds source of gateway
**					    table if this is a gateway register.
**          .dmu_key_array.ptr_address      Pointer to an area used to input
**                                          an array of pointer to entries
**					    of type DMU_KEY_ENTRY.  
**                                          See below for description of 
**                                          <dmu_key_list> entries.
**          .dmu_key_array.ptr_size         Size of an entry.
**          .dmu_key_array.ptr_in_count     Count of entries.
**	    .dmu_attr_array.ptr_address	    Pointer to to area used to input
**					    an array or pointers to entries
**					    of type DMU_KEY_ENTRY.
**					    If this entry is not passed in
**					    all the key's given in the key
**					    array are considered part of the
**					    key.  If this pass in, only the
**					    keys in this list are considered
**					    part of the key.  The keys listed
**					    in this list must be a prefix set
**					    of the keys listed in the key array.
**          .dmu_attr_array.ptr_size        Size of an entry.
**          .dmu_attr_array.ptr_in_count    Count of entries.
**	    .dmu_chars			    Various characteristics
**	    .dmu_gwchar_array.data_address  Pointer to an array of gateway table
**					    characteristics.  These are used
**					    if the table is a DMU_GATEWAY type
**					    table.  These characteristics are
**					    passed directly down to the Ingres
**					    Gateway system.
**	    .dmu_gwchar_array.data_in_size  Length of gwchar_array in bytes.
**	    .dmu_gwattr_array.ptr_address   Pointer to array of pointers, each
**					    of which describes gateway specific
**					    information about a table column.
**					    This is used only if the table is
**					    a DMU_GATEWAY type table.  These
**					    entries are passed directly down to
**					    the Ingres Gateway system.
**	    .dmu_gwattr_array.ptr_size	    The size of each element in array.
**	    .dmu_gwattr_array.ptr_address   The number of pointers in the array.
**
**          <dmu_key_array> entries are of type DMU_KEY_ENTRY and
**          must have following format:
**          key_attr_name                   Name of attribute.
**          key_order                       Must be DMU_ASCENDING.
**
** Output:
**      dmu_cb 
**          .dmu_idx_id                     The internal table identifier 
**                                          assigned to this index.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM0006_BAD_ATTR_FLAG
**                                          E_DM0007_BAD_ATTR_NAME
**                                          E_DM0009_BAD_ATTR_SIZE
**                                          E_DM0008_BAD_ATTR_PRECISION
**                                          E_DM000A_BAD_ATTR_TYPE
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**					    E_DM000D_BAD_CHAR_ID
**					    E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001C_BAD_KEY_SEQUENCE.
**                                          E_DM001D_BAD_LOCATION_NAME.
**                                          E_DM001E_DUP_LOCATION_NAME.
**                                          E_DM001A_BAD_FLAG
**                                          E_DM0021_TABLES_TOO_MANY
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM0039_BAD_TABLE_NAME
**                                          E_DM003A_BAD_TABLE_OWNER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0045_DUPLICATE_KEY
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM0059_NOT_ALL_KEYS
**                                          E_DM005D_TABLE_ACCESS_CONFLICT
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM005F_CANT_INDEX_CORE_SYSCAT
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM0071_LOCATIONS_TOO_MANY
**                                          E_DM0072_NO_LOCATION
**                                          E_DM0078_TABLE_EXISTS
**					    E_DM007D_BTREE_BAD_KEY_LENGTH
**					    E_DM010F_ISAM_BAD_KEY_LENGTH
**					    E_DM0110_COMP_BAD_KEY_LENGTH
**					    E_DM0092_ERROR_INDEXING_TABLE
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM0103_TUPLE_TOO_WIDE
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0125_DDL_SECURITY_ERROR
**
**          .error.err_data                 Set to attribute in error by 
**                                          returning index into attribute list.
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmu_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmu_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmu_cb.error.err_code.
**
** History:
**      01-sep-85 (jennifer) 
**          Created.
**      20-nov-87 (jennifer)
**          Added multiple loclation support.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**      27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**	31-mar-1989 (EdHsu)
**	    Added param to dmxe_writebt to support cluster online backup
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Added gateway attribute arrays
**	    to the DMU_CB call control block.  These are options that are
**	    meaningful to the gateway - they are passed down to the gateway
**	    facility for parsing.  Check for DMU_GATEWAY index option and pass
**	    appropriate flag to dm2u.  Added arguments to dm2u_index call.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**	23-apr-1990 (linda)
**	    Add parameter to dm2u_index() call for source for gateway object.
**	26-apr-90 (linda)
**	    Add parameter to dm2u_index() call for qry_id -- this field was
**	    originally just for views and was used to define the join criterion
**	    for the "iiviews" standard catalog.  For gateways, we use a similar
**	    catalog "iiregistrations", for either tables or indexes.
**	11-jan-1991 (bryanp)
**	    Added support for DMU_INDEX_COMP to implement Btree index
**	    compression. This characteristic is used to set the new 
**	    'index_compressed' argument to dm2u_index(). If DM616 is set, force
**	    index compression to be used.
**	16-jul-1991 (bryanp)
**	    B38527: Add new 'reltups' argument to dm2u_index. This routine
**	    builds only standard table indexes; that is, it is not called for
**	    the 'iiridxtemp' index built by sysmod on iirtemp. Therefore, it
**	    always passes 0, indicating that dm2u_load_table should calculate
**	    the number of hash buckets based on the actual number of tuples
**	    in the base table we're creating an index for.
**      16-apr-1992 (schang)
**          add gwid at the end of dm2u_index() arg list.  This arg is used to
**          identify which gateway in case of gateway index.
**	3-oct-1992 (bryanp)
**	    Pass dmu_char_array to dm2u_index for gateway use.
**	04-nov-1992
**	    When creating an INDEX, make sure appropriate message is audited.
**	14-dec-1992 (rogerk)
**	    Initialize location count to 1 so that create's which specify no
**	    location list will be recorded in iirelation as having 1 location.
**	25-feb-93 (rickh)
**	    Added relstat2 argument to dm2u_index call and added support
**	    for the corresponding DMU characteristics.
**	30-mar-93 (rickh)
**	    Fix bug in 25-feb-93 change;  DMU characteristics weren't being
**	    correctly translated to relstat2 bits.
**	15-may-93 (rmuth)
**	    Add support which enables an index to be created in 
**	    "concurrent_access" mode. This causes the index to be built
**	    by only taking out share locks. Cuurently this is only
**	    supported on base tables which are READONLY.
**	28-jun-93 (robf)
**	    Add support for row security label/key to be specified
**	19-aug-93 (stephenb)
**	    Changed call to dma_write_audit() to audit index name on index
**	    creation rather than table name, to be consistent with drop index
**	    audit records.
**	18-oct-93 (rogerk)
**	    Initialize min_pages, max_pages values so when non-hash indexes
**	    are created the iirelation tuple fields will be deterministic.
**	15-nov-93 (robf)
**          Update error handling if unable to get default security label.
**	    (previously error was unset, now E_DM0092_ERROR_INDEXING_TABLE)
**      06-jan-95 (nanpr01)
**          Bug # 66028. Set the TCB2_TBL_RECOVERY_DISALLOWED flag on 
**          relstat2 field at the time of index creation.
**	10-feb-95 (cohmi01)
**	    For RAW I/O, add DB_RAWEXT_NAME parm to dm2u_index.
**	05-may-1995 (shero03)
**	    For RTREE, add dimension, range, and data_clustered 
**	06-mar-1996 (stial01 for bryanp)
**	    Recognize DMU_PAGE_SIZE characteristic; pass page_size argument to
**		dm2u_create.
**      19-apr-1996 (stial01)
**          Validate page_size parameter
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	18-dec-1996 (nanpr01)
**	    If one of the string is NULL, STbcompare returns 0 as documented.
**	    We therefore need to use STxcompare to make sure that comparisons
**	    are done correctly. Bug 79836 exposed this problem when 
**	    persistent indexes are recreated may not have index names since
**	    they are recreated.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Use DMU_TABLE_PRIORITY to set table priority and pass on to
**	    dm2u_index().
**	15-apr-1997 (shero03)
**	    Support 3D NBR for R-tree
**	26-may-1998 (nanpr01)
**	    Fix compilation warning.
**      28-may-1998 (stial01)
**          dmu_index() Support VPS system catalogs.
**      17-Apr-2001 (horda03) Bug 104402
**          Added support for the TCB_NOT_UNIQUE attribute.
**	12-mar-1999 (nanpr01)
**	    Get rid of extent names since only one table is supported per
**	    raw location.
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Initialise the new indxcb_dmveredo field.
**	22-Dec-2003 (jenjo02)
**	    Added DMU_GLOBAL_INDEX for Partitioned Table Project.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**      08-jul-2004 (thaju02)
**          Online Modify - init indxcb_online_fhdr_pageno.
**          (B112610)
**	20-Jul-2004 (schka24)
**	    B1 code removal left uninitialized oops and DM0092's, fix.
**	11-Mar-2005 (thaju02)
**	    Use $online idx relation info. (B114069)
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**      25-oct-2006 (stial01)
**          Fixed initialization of DM2U_INDEX_CB
*/
DB_STATUS
dmu_index(DMU_CB    *dmu_cb)
{
    DMU_CB		*dmu = dmu_cb;
    DM2U_INDEX_CB	indx_cb;
    DB_OWN_NAME		table_owner;
    DB_TAB_NAME		table_name;
    DML_XCB		*xcb;
    i4			local_error;
    DB_STATUS		status;

    CLRDBERR(&dmu->error);

    /* Setup control blocks for the create index */
    indx_cb.indxcb_tab_name = &table_name;
    indx_cb.indxcb_tab_owner = &table_owner;
    status = dmuIndexSetup(dmu, (PTR)&indx_cb);

    /* Calls the physical layer to process the rest of the index create */
    /*
    ** schang 04/16/92 : add last arg of dm2u_index,
    ** it is gateway id
    */
    if ( status == E_DB_OK )
	status = dm2u_index(&indx_cb);

    /*	Audit successful index on TABLE. */

    if ( status == E_DB_OK && dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	/*
	**	Audit success
	*/
	status = dma_write_audit(
		    SXF_E_TABLE,
		    SXF_A_SUCCESS | SXF_A_INDEX,
		    dmu->dmu_index_name.db_tab_name, /* index name */
		    sizeof(dmu->dmu_index_name.db_tab_name),
		    &dmu->dmu_owner,		   /* Table/view owner */
		    I_SX2011_INDEX_CREATE,
		    FALSE, /* Not force */
		    &dmu->error, NULL);
    }

    if ( status )
    {
	if (dmu->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmu->error, 0, E_DM0092_ERROR_INDEXING_TABLE);
	}
	else if ( xcb = indx_cb.indxcb_xcb )
	    switch (dmu->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM0092_ERROR_INDEXING_TABLE:
	    case E_DM0045_DUPLICATE_KEY:
	    case E_DM006A_TRAN_ACCESS_CONFLICT:
	    case E_DM0078_TABLE_EXISTS:
		xcb->xcb_state |= XCB_STMTABORT;
		break;

	    case E_DM0042_DEADLOCK:
	    case E_DM004A_INTERNAL_ERROR:
	    case E_DM0100_DB_INCONSISTENT:
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    case E_DM0065_USER_INTR:
		xcb->xcb_state |= XCB_USER_INTR;
		break;
	    case E_DM010C_TRAN_ABORTED:
		xcb->xcb_state |= XCB_FORCE_ABORT;
		break;
	    case E_DM007D_BTREE_BAD_KEY_LENGTH:
		dmu->dmu_tup_cnt = indx_cb.indxcb_maxklen;
		break;
	}
    }

    return (status);
}

/*{
** Name: dmuIndexSetup - Prepare a DM2U_INDEX_CB from a DMU_CB
**
** Description:
**	Code common to dmu_index, dmu_pindex, that makes a
**	DM2U_INDEX_CB from a DMU_CB in preparation for
**	create index.
**
** Inputs:
**      .dmu_cb
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**          .dmu_flags_mask                 Must be zero.
**          .dmu_tbl_id                     Internal name of table to be 
**                                          indexed.
**          .dmu_index_name                 External name of index to be 
**                                          created.
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
**          .dmu_olocation.data_address     This holds source of gateway
**					    table if this is a gateway register.
**          .dmu_key_array.ptr_address      Pointer to an area used to input
**                                          an array of pointer to entries
**					    of type DMU_KEY_ENTRY.  
**                                          See below for description of 
**                                          <dmu_key_list> entries.
**          .dmu_key_array.ptr_size         Size of an entry.
**          .dmu_key_array.ptr_in_count     Count of entries.
**	    .dmu_attr_array.ptr_address	    Pointer to to area used to input
**					    an array or pointers to entries
**					    of type DMU_KEY_ENTRY.
**					    If this entry is not passed in
**					    all the key's given in the key
**					    array are considered part of the
**					    key.  If this pass in, only the
**					    keys in this list are considered
**					    part of the key.  The keys listed
**					    in this list must be a prefix set
**					    of the keys listed in the key array.
**          .dmu_attr_array.ptr_size        Size of an entry.
**          .dmu_attr_array.ptr_in_count    Count of entries.
**	    .dmu_chars			    Various characteristics
**	    .dmu_gwchar_array.data_address  Pointer to an array of gateway table
**					    characteristics.  These are used
**					    if the table is a DMU_GATEWAY type
**					    table.  These characteristics are
**					    passed directly down to the Ingres
**					    Gateway system.
**	    .dmu_gwchar_array.data_in_size  Length of gwchar_array in bytes.
**	    .dmu_gwattr_array.ptr_address   Pointer to array of pointers, each
**					    of which describes gateway specific
**					    information about a table column.
**					    This is used only if the table is
**					    a DMU_GATEWAY type table.  These
**					    entries are passed directly down to
**					    the Ingres Gateway system.
**	    .dmu_gwattr_array.ptr_size	    The size of each element in array.
**	    .dmu_gwattr_array.ptr_address   The number of pointers in the array.
**
**          <dmu_key_array> entries are of type DMU_KEY_ENTRY and
**          must have following format:
**          key_attr_name                   Name of attribute.
**          key_order                       Must be DMU_ASCENDING.
**
** Output:
**	indx_cb				    All filled in and ready to go.
**      dmu_cb 
**          .dmu_idx_id                     The internal table identifier 
**                                          assigned to this index.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM0006_BAD_ATTR_FLAG
**                                          E_DM0007_BAD_ATTR_NAME
**                                          E_DM0009_BAD_ATTR_SIZE
**                                          E_DM0008_BAD_ATTR_PRECISION
**                                          E_DM000A_BAD_ATTR_TYPE
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**					    E_DM000D_BAD_CHAR_ID
**					    E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001C_BAD_KEY_SEQUENCE.
**                                          E_DM001D_BAD_LOCATION_NAME.
**                                          E_DM001E_DUP_LOCATION_NAME.
**                                          E_DM001A_BAD_FLAG
**                                          E_DM0021_TABLES_TOO_MANY
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM0039_BAD_TABLE_NAME
**                                          E_DM003A_BAD_TABLE_OWNER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0045_DUPLICATE_KEY
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM0059_NOT_ALL_KEYS
**                                          E_DM005D_TABLE_ACCESS_CONFLICT
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM005F_CANT_INDEX_CORE_SYSCAT
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM0071_LOCATIONS_TOO_MANY
**                                          E_DM0072_NO_LOCATION
**                                          E_DM0078_TABLE_EXISTS
**					    E_DM007D_BTREE_BAD_KEY_LENGTH
**					    E_DM010F_ISAM_BAD_KEY_LENGTH
**					    E_DM0110_COMP_BAD_KEY_LENGTH
**					    E_DM0092_ERROR_INDEXING_TABLE
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM0103_TUPLE_TOO_WIDE
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0125_DDL_SECURITY_ERROR
**
**          .error.err_data                 Set to attribute in error by 
**                                          returning index into attribute list.
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmu_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmu_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmu_cb.error.err_code.
**
** History:
**	12-Jul-2006 (jonj)
**	    Carved from duplicate 450 lines of code in dmupindex, dmuindex,
**	    holy crap.
**	    Support indexes on Temporary Tables.
**	22-Oct-2009 (kschendel) SIR 122739
**	    Minor change to compression characteristic setting.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Fix outrageous name confusion re acount vs kcount.
**	12-Oct-2010 (kschendel) SIR 124544
**	    dmu_char_array replaced with DMU_CHARACTERISTICS.
**	    Fix the operation preamble here.
*/
DB_STATUS
dmuIndexSetup(
DMU_CB		*dmu,
PTR		ix_cb)
{
    DM2U_INDEX_CB	*indx_cb = (DM2U_INDEX_CB*)ix_cb;
    DML_ODCB		*odcb;
    DML_XCB		*xcb;
    DMU_KEY_ENTRY	**att;
    bool		statementLevelUnique = FALSE;
    bool		persistsOverModifies = FALSE;
    bool		systemGenerated = FALSE;
    bool		supportsConstraint = FALSE;
    bool		notUnique = FALSE;
    bool		notDroppable = FALSE;
    bool		GlobalIndex = FALSE;
    bool		rowSecAudit = FALSE;
    bool		TempTable;
    i4			error;
    i4			i,j;
    i4			indicator;
    DB_STATUS		status;

    CLRDBERR(&dmu->error);

    /* Error codes are always returned in DMU_CB */
    indx_cb->indxcb_errcb = &dmu->error;


    /* Creating index on a temp table? */
    TempTable = (dmu->dmu_tbl_id.db_tab_base < 0) ? TRUE : FALSE;

    indx_cb->indxcb_index_flags = 0;
    indx_cb->indxcb_allocation = 0;
    indx_cb->indxcb_extend = 0;
    indx_cb->indxcb_page_size = dmf_svcb->svcb_page_size;
    indx_cb->indxcb_page_type = TCB_PG_INVALID; 
    indx_cb->indxcb_relstat2 = TCB2_TBL_RECOVERY_DISALLOWED;
    indx_cb->indxcb_tbl_pri = 0;
    indx_cb->indxcb_gwsource = 0;
    indx_cb->indxcb_l_count = 1;
    indx_cb->indxcb_dmveredo = FALSE;
    indx_cb->indxcb_xcb = (DML_XCB*)NULL;

    do
    {
	/*  Check for bad flags. */

	if ( dmu->dmu_flags_mask & ~(DMU_ONLINE_END | DMU_ONLINE_START) )
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/*  Check for valid transaction id. */

	xcb = (DML_XCB *)dmu->dmu_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	indx_cb->indxcb_xcb = xcb;

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);

	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
	    {
		SETDBERR(&dmu->error, 0, E_DM0065_USER_INTR);
		break;
	    }
	    if (xcb->xcb_state & XCB_FORCE_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM010C_TRAN_ABORTED);
		break;
	    }
	    if (xcb->xcb_state & XCB_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM0064_USER_ABORT);
		break;
	    }	    
	}

	/*  Check the database identifier. */

	odcb = (DML_ODCB *)dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	indx_cb->indxcb_dcb = odcb->odcb_dcb_ptr;
	indx_cb->indxcb_db_lockmode = DM2T_X;

	/*  Check that this is a update transaction on the database 
        **  that can be updated. */
	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	/* 
	** If creating an index on a TempTable, check that the
	** index name isn't in use as a Temp Base table name.
	** Further checks are done in dm2u_index() after
	** the Base TCB is opened.
	**
	** TempTables don't log and are immune to readonly checks
	*/
	if ( TempTable )
	{
	    DML_XCCB	*xccb;
	    DMP_TCB	*t;

	    dm0s_mlock(&odcb->odcb_cq_mutex);

	    for ( xccb = odcb->odcb_cq_next;
		  xccb != (DML_XCCB*)&odcb->odcb_cq_next;
		  xccb = xccb->xccb_q_next )
	    {
		 if ( xccb->xccb_operation == XCCB_T_DESTROY &&
		      xccb->xccb_is_session &&
		      (t = xccb->xccb_t_tcb) &&
		      MEcmp((char*)&t->tcb_rel.relid,
			    (char*)&dmu->dmu_table_name,
			    sizeof(DB_TAB_NAME)) == 0 )
		    {
			SETDBERR(&dmu->error, 0, E_DM0078_TABLE_EXISTS);
			break;
		    }
	    }
	    dm0s_munlock(&odcb->odcb_cq_mutex);
	}
	else
	{
	    if (xcb->xcb_x_type & XCB_RONLY)
		SETDBERR(&dmu->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);

	    /*
	    ** If this is the first write operation for this transaction,
	    ** then we need to write the begin transaction record,
	    ** unless TempTable index, which isn't logged.
	    */
	    else if ( xcb->xcb_flags & XCB_DELAYBT )
	    {
		status = dmxe_writebt(xcb, TRUE, &dmu->error);
		if (status != E_DB_OK)
		{
		    xcb->xcb_state |= XCB_TRANABORT;
		}
	    }
	}

	if ( dmu->error.err_code)
	    break;

	/*  Check the characteristics. */

	indx_cb->indxcb_structure = TCB_BTREE;
	indx_cb->indxcb_i_fill = 0;
	indx_cb->indxcb_l_fill = 0;
	indx_cb->indxcb_d_fill = 0;
	indx_cb->indxcb_compressed = TCB_C_NONE;
	indx_cb->indxcb_index_compressed = 0;
	indx_cb->indxcb_unique = 0;
	indx_cb->indxcb_min_pages = 0;
	indx_cb->indxcb_max_pages = 0;

	indicator = -1;
	while ((indicator = BTnext(indicator, dmu->dmu_chars.dmu_indicators, DMU_CHARIND_LAST)) != -1)
	{
	    switch (indicator)
	    {
	    case DMU_STRUCTURE:
		indx_cb->indxcb_structure = dmu->dmu_chars.dmu_struct;
		break;

	    case DMU_IFILL:
		indx_cb->indxcb_i_fill = dmu->dmu_chars.dmu_nonleaff;
		if (indx_cb->indxcb_i_fill > 100)
		    indx_cb->indxcb_i_fill = 100;
		break;

	    case DMU_LEAFFILL:
		indx_cb->indxcb_l_fill = dmu->dmu_chars.dmu_leaff;
		if (indx_cb->indxcb_l_fill > 100)
		    indx_cb->indxcb_l_fill = 100;
		break;

	    case DMU_DATAFILL:
		indx_cb->indxcb_d_fill = dmu->dmu_chars.dmu_fillfac;
		if (indx_cb->indxcb_d_fill > 100)
		    indx_cb->indxcb_d_fill = 100;
		break;

	    case DMU_PAGE_SIZE:
		indx_cb->indxcb_page_size = dmu->dmu_chars.dmu_page_size;
		if (indx_cb->indxcb_page_size != 2048  &&
		    indx_cb->indxcb_page_size != 4096  &&
		    indx_cb->indxcb_page_size != 8192  &&
		    indx_cb->indxcb_page_size != 16384 &&
		    indx_cb->indxcb_page_size != 32768 &&
		    indx_cb->indxcb_page_size != 65536)
		{
		    SETDBERR(&dmu->error, indicator, E_DM000E_BAD_CHAR_VALUE);
		}		    
		else if (!dm0p_has_buffers(indx_cb->indxcb_page_size))
		    SETDBERR(&dmu->error, 0, E_DM0157_NO_BMCACHE_BUFFERS);
		break;

	    case DMU_MINPAGES:
		indx_cb->indxcb_min_pages = dmu->dmu_chars.dmu_minpgs;
		break;

	    case DMU_MAXPAGES:
		indx_cb->indxcb_max_pages = dmu->dmu_chars.dmu_maxpgs;
		break;

	    case DMU_DCOMPRESSION:
		/* Hidata isn't allowed.  If we add new index compression
		** types, pass the type here.
		*/
		if (dmu->dmu_chars.dmu_dcompress != DMU_COMP_OFF)
		    indx_cb->indxcb_compressed = TCB_C_STANDARD;
		break;

	    case DMU_UNIQUE:
		indx_cb->indxcb_unique = TRUE;
		break;

	    case DMU_GATEWAY:
		indx_cb->indxcb_index_flags |= DM2U_GATEWAY;
		indx_cb->indxcb_gwsource =
		    (DMU_FROM_PATH_ENTRY *)dmu->dmu_olocation.data_address;
		break;

	    case DMU_ALLOCATION:
		indx_cb->indxcb_allocation = dmu->dmu_chars.dmu_alloc;
		break;

	    case DMU_EXTEND:
		indx_cb->indxcb_extend = dmu->dmu_chars.dmu_extend;
		break;

	    case DMU_CONCURRENT_ACCESS:
		if ( dmu->dmu_chars.dmu_flags & DMU_FLAG_CONCUR_A )
		    indx_cb->indxcb_relstat2 |= TCB2_CONCURRENT_ACCESS;
		break;

	    case DMU_KCOMPRESSION:
		indx_cb->indxcb_index_compressed = (dmu->dmu_chars.dmu_kcompress != DMU_COMP_OFF);
		break;

	    case DMU_STATEMENT_LEVEL_UNIQUE:
		statementLevelUnique = (dmu->dmu_chars.dmu_flags & DMU_FLAG_UNIQUE_STMT) != 0;
		break;

	    case DMU_PERSISTS_OVER_MODIFIES:
		persistsOverModifies = (dmu->dmu_chars.dmu_flags & DMU_FLAG_PERSISTENCE) != 0;
		break;

	    case DMU_SYSTEM_GENERATED:
		systemGenerated = TRUE;
		break;

	    case DMU_SUPPORTS_CONSTRAINT:
		supportsConstraint = TRUE;
		break;

	    case DMU_NOT_UNIQUE:
		notUnique = TRUE;
		break;

	    case DMU_ROW_SEC_AUDIT:
		rowSecAudit = TRUE;
		break;

	    case DMU_NOT_DROPPABLE:
		notDroppable = TRUE;
		break;

	    case DMU_DIMENSION:
		/* dimension = dmu->dmu_chars.dmu_dimension;   ignore value */
		break;

	    case DMU_TABLE_PRIORITY:
		indx_cb->indxcb_tbl_pri = dmu->dmu_chars.dmu_cache_priority;
		if (indx_cb->indxcb_tbl_pri < 0 ||
		    indx_cb->indxcb_tbl_pri > DB_MAX_TABLEPRI)
		{
		    SETDBERR(&dmu->error, indicator, E_DM000E_BAD_CHAR_VALUE);
		}
		break;

	    case DMU_GLOBAL_INDEX:
		GlobalIndex = TRUE;
		break;

	    case DMU_CONCURRENT_UPDATES:
		if (dmu->dmu_chars.dmu_flags & DMU_FLAG_CONCUR_U)
		{
		    indx_cb->indxcb_index_flags |= DM2U_ONLINE_MODIFY;
		    indx_cb->indxcb_dupchk_tabid = &dmu->dmu_dupchk_tabid;
		}
		break;

	    default:
		/* Just ignore stuff we don't understand */
		break;
	    }

	    if ( dmu->error.err_code )
		break;
	}

	if ( dmu->error.err_code )
	    break;

	/* Fill in structure defaults as needed.  indxcb members are zero
	** if they haven't been set to something interesting.  (Obviously
	** we could BTtest the dmu chars as well, but this works too.)
	** FIXME stupid constants here, consolidate with dmu-modify later!
	*/
	if (indx_cb->indxcb_structure == TCB_ISAM)
	{
	    if (indx_cb->indxcb_i_fill == 0)
		indx_cb->indxcb_i_fill = 100;
	    if (indx_cb->indxcb_d_fill == 0)
		indx_cb->indxcb_d_fill = 80;
	}
	else if (indx_cb->indxcb_structure == TCB_HASH)
	{
	    if (indx_cb->indxcb_d_fill == 0)
		indx_cb->indxcb_d_fill = 50;
	    if (indx_cb->indxcb_min_pages == 0)
		indx_cb->indxcb_min_pages = 2;
	    if (indx_cb->indxcb_max_pages == 0)
		indx_cb->indxcb_max_pages = 8388607;
	}
	else if (indx_cb->indxcb_structure == TCB_BTREE)
	{
	    if (indx_cb->indxcb_i_fill == 0)
		indx_cb->indxcb_i_fill = 80;
	    if (indx_cb->indxcb_l_fill == 0)
		indx_cb->indxcb_l_fill = 70;
	    if (indx_cb->indxcb_d_fill == 0)
		indx_cb->indxcb_d_fill = 80;
	}
	else if (indx_cb->indxcb_structure == TCB_RTREE)
	{
	    if (indx_cb->indxcb_i_fill == 0)
		indx_cb->indxcb_i_fill = 95;
	    if (indx_cb->indxcb_l_fill == 0)
		indx_cb->indxcb_l_fill = 95;
	}
	/* Fill in unused/bogus fillfactors so SEP tests are happy */
	if (indx_cb->indxcb_i_fill == 0)
	    indx_cb->indxcb_i_fill = 80;
	if (indx_cb->indxcb_l_fill == 0)
	    indx_cb->indxcb_l_fill = 70;
	if (indx_cb->indxcb_d_fill == 0)
	    indx_cb->indxcb_d_fill = 80;

	if ( TempTable )
	    indx_cb->indxcb_relstat2 |= TCB_SESSION_TEMP;
	if ( statementLevelUnique )
	    indx_cb->indxcb_relstat2 |= TCB_STATEMENT_LEVEL_UNIQUE;
	if ( persistsOverModifies )
	    indx_cb->indxcb_relstat2 |= TCB_PERSISTS_OVER_MODIFIES;
	if ( systemGenerated )
	    indx_cb->indxcb_relstat2 |= TCB_SYSTEM_GENERATED;
	if ( supportsConstraint )
	    indx_cb->indxcb_relstat2 |= TCB_SUPPORTS_CONSTRAINT;
	if ( notUnique )
	    indx_cb->indxcb_relstat2 |= TCB_NOT_UNIQUE;
	if ( notDroppable )
	    indx_cb->indxcb_relstat2 |= TCB_NOT_DROPPABLE;
	if ( rowSecAudit )
	    indx_cb->indxcb_relstat2 |= TCB_ROW_AUDIT;
	if ( GlobalIndex )
	    indx_cb->indxcb_relstat2 |= TCB2_GLOBAL_INDEX;

	if (indx_cb->indxcb_structure == TCB_BTREE)
	{
	    if ( DMZ_AM_MACRO(16) && !TempTable )
	    {
		/* DM616 -- forces index compression to be used: */
		indx_cb->indxcb_index_compressed = 1;
	    }
	}

	/* Prepare key array and attr array.
	** Probably for historical reasons, PSF sends the entire list
	** of index attributes over in dmu_key_array, and the list of
	** (prefix) key attributes in dmu_attr_array.  If there are
	** no non-key attributes (ie if there was no KEY=() clause),
	** the dmu_attr_array count is zero.  Both point to the same
	** list of attributes, since in an index, keys prefix non-keys.
	** (Only used specified attributes are listed and counted,
	** never tidp nor any of the R-tree add-on's.)
	*/

	indx_cb->indxcb_key = (DMU_KEY_ENTRY **)dmu->dmu_key_array.ptr_address;
	indx_cb->indxcb_acount = dmu->dmu_key_array.ptr_in_count;
	if ((indx_cb->indxcb_acount == 0) || (indx_cb->indxcb_key == NULL))
	{
	    SETDBERR(&dmu->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	att = (DMU_KEY_ENTRY **)dmu->dmu_attr_array.ptr_address;
	indx_cb->indxcb_kcount = dmu->dmu_attr_array.ptr_in_count;
	for (i = 0; i < indx_cb->indxcb_kcount; i++)
	{
	    if (MEcmp((PTR)&att[i]->key_attr_name, 
		      (PTR)&indx_cb->indxcb_key[i]->key_attr_name,
		      sizeof(DB_ATT_NAME)))
	    {
		break;
	    }
	}
	if (i < indx_cb->indxcb_kcount)
	{
	    SETDBERR(&dmu->error, i, E_DM002A_BAD_PARAMETER);
	    break;
	}
	if (indx_cb->indxcb_kcount == 0)
	    indx_cb->indxcb_kcount = indx_cb->indxcb_acount;

	/* Check the location names for duplicates, too many. */

	indx_cb->indxcb_location = 0;
	if (dmu->dmu_location.data_address && 
	    dmu->dmu_location.data_in_size >= sizeof(DB_LOC_NAME))
	{
	    indx_cb->indxcb_location = 
		(DB_LOC_NAME *) dmu->dmu_location.data_address;
	    indx_cb->indxcb_l_count = 
		dmu->dmu_location.data_in_size/sizeof(DB_LOC_NAME);
	    if (indx_cb->indxcb_l_count > DM_LOC_MAX)
	    {
		SETDBERR(&dmu->error, 0, E_DM0071_LOCATIONS_TOO_MANY);
		break;
	    }
	    for (i = 0; i < indx_cb->indxcb_l_count; i++)
	    {
		for (j = 0; j < i; j++)
		{
		    /* 
                    ** Compare this attribute name against other 
                    ** already given, they cannot be the same.
                    */

		    if (MEcmp(indx_cb->indxcb_location[j].db_loc_name,
                              indx_cb->indxcb_location[i].db_loc_name,
			      sizeof(DB_LOC_NAME)) == 0 )
		    {
			SETDBERR(&dmu->error, i, E_DM001E_DUP_LOCATION_NAME);
			break;
		    }	    	    
		}
		if ( dmu->error.err_code )
		    break;		    
	    }
	}
	else
	{
	    if (dmu->dmu_location.data_address &&
					dmu->dmu_location.data_in_size)
		SETDBERR(&dmu->error, 0, E_DM001F_LOCATION_LIST_ERROR);
	    else
		SETDBERR(&dmu->error, 0, E_DM0072_NO_LOCATION);
	}

	if ( dmu->error.err_code )
	    break;		    

	/*
	** Currently, index compression is available only for BTREE structures:
	*/
	if ((indx_cb->indxcb_index_compressed != 0) && 
	    (indx_cb->indxcb_structure != TCB_BTREE))
	{
	    SETDBERR(&dmu->error, 0, E_DM002A_BAD_PARAMETER);
	    /* error = E_DMxxxx_CANT_COMP_INDEX; */
	    break;
	}
	indx_cb->indxcb_index_name = &dmu->dmu_index_name;
	indx_cb->indxcb_tbl_id = &dmu->dmu_tbl_id;
	indx_cb->indxcb_idx_id = &dmu->dmu_idx_id;
	indx_cb->indxcb_range = dmu->dmu_range;
	indx_cb->indxcb_gwattr_array = &dmu->dmu_gwattr_array;
	indx_cb->indxcb_gwchar_array = &dmu->dmu_gwchar_array;
	indx_cb->indxcb_qry_id = &dmu->dmu_qry_id;
	indx_cb->indxcb_tup_info = &dmu->dmu_tup_cnt;
	indx_cb->indxcb_gw_id = dmu->dmu_gw_id;
	indx_cb->indxcb_dmu_chars = &dmu->dmu_chars;
	indx_cb->indxcb_reltups = 0;
	if (dmu->dmu_flags_mask & DMU_ONLINE_END)
	{
	    indx_cb->indxcb_index_flags |= DM2U_ONLINE_END;

	    /*
	    ** This same DMU_CB was used to build the online index,
	    ** and its tabid should still be in the dmu_idx_id 
	    */
	    STRUCT_ASSIGN_MACRO(dmu->dmu_idx_id, indx_cb->indxcb_online_tabid);
	    indx_cb->indxcb_on_reltups = dmu->dmu_on_reltups;
	    indx_cb->indxcb_on_relfhdr = dmu->dmu_on_relfhdr;
	    indx_cb->indxcb_on_relpages = dmu->dmu_on_relpages;
	    indx_cb->indxcb_on_relprim = dmu->dmu_on_relprim;
	    indx_cb->indxcb_on_relmain = dmu->dmu_on_relmain;
	}
    } while (FALSE);

    return( (dmu->error.err_code) ? E_DB_ERROR : E_DB_OK );
}
