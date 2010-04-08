/*
**Copyright (c) 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <tm.h>
#include    <sr.h>
#include    <me.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
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
#include    <dmu.h>
#include    <dm1c.h>
#include    <dm1b.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <sxf.h>
#include    <dma.h>
#include    <cm.h>
#include    <st.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dm0pbmcb.h>
#include    <dmpp.h>
#include    <gwf.h>

/**
** Name:  DMUCREATE.C - Functions used to create a table.
**
** Description:
**      This file contains the functions necessary to create a permanent table.
**      Common routines used by all DMU functions are not included here, but
**      are found in DMUCOMMON.C.  This file defines:
**    
**      dmu_create()    -  Routine to perfrom the normal create table
**                         operation.
**
** History:
**      13-Jan-86 (jennifer)
**          Created for Jupiter.
**      16-nov-87 (jennifer)
**          Added multiple location support.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	21-mar-89 (jrb)
**	    Changed call to adc_lenchk to reflect a change in its interface.
**	17-apr-1989 (mikem)
**	    Logical key development.  Support creating a table with two new
**	    attribute attributes (DMU_F_SYS_MAINTAINED, DMU_F_WORM).
**      22-may-89 (jennifer)
**          For B1 systems check that attribute name is not one of 
**          the ones reserved for security label "ii_sec_label" or the
**          table identifier "ii_sec_tabkey".
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Accept DMU_GATEWAY attribute.
**	    Added gateway attribute arrays to the DMU_CB control block.
**	17-aug-1989 (Derek)
**	    Added DAC success auditing.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	24-Jan-1990 (fred)
**	    Added support for peripheral datatypes.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	17-dec-1990 (bryanp)
**	    New argument to dm2u_modify() for Btree index compression.
**	14-jun-1991 (Derek)
**	    Added support for new allocation and extend options.
**	17-jan-1992 (bryanp)
**	    Temporary table enhancements. New argument to dm2u_modify.
**      30-apr-1992 (schang)
**          change dm2u_modify call params sequence to match dm2u_modify
**          args sequence.  This is just for gateway	
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	17-aug-92 (rickh)
**	    Improvements for FIPS CONSTRAINTS.  Translate new relstat2 bits.
**	30-sep-1992 (robf)
**	    Pass char_array onto dm2u_create to pass to GWF for more
**	    error checking
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	03-nov-1992 (ralph)
**	    Delimited identifiers:  remove attribute character validation.
**	    DMF should trust PSF to validate object name characters....
*	18-nov-92 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	    All auditing now thorough dma_write_audit()
**	30-mar-1993 (rmuth)
**	    Add mod_options2 field to dm2u_modify.
**	8-apr-93 (rickh)
**	    Added relstat2 argument to dm2u_modify call.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**      11-aug-93 (ed)
**          added missing includes
**	08-sep-93 (swm)
**	    Moved cs.h include above lg.h because lg.h now needs the CS_SID
**	    session id. type definition.
**	8-oct-93 (stephenb)
**	    Updated call to dma_write_audit() so that it finds the correct
**	    context to write an audit record when creating temporary tables
**	    also added a new message for this scenario.
**      20-Oct-1993 (fred)
**          Added support for session temporaries having large object
**          attributes.  This had been misplaced.
**	19-nov-93 (stephenb)
**	    Altered call to dma_write_audit() so that a better message is
**	    written to distinguish CREATE from a gateway REGISTER.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**      23-may-1994 (andys)
**          Add cast(s) to quiet qac/lint
**      06-Jan-1995 (nanpr01)
**          Bug #66028
**          set the bit TABLE_RECOVERY_DISALLOWED the time of table 
**          creation.
**	09-Feb-1995 (cohmi01)
**	    For RAW I/O, pass new rawextname parm to dm2u_create.
**	    also pass a null rawextname to dm2u_modify.
**	06-mar-1996 (stial01 for bryanp)
**	    Recognize DMU_PAGE_SIZE characteristic; pass page_size argument to
**		dm2u_create() and dm2u_modify().
**	    If DM715 is set, keep changing svcb_page_size to enable a strange
**		pseudo-random page size selection within the server.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**      19-apr-1996 (stial01)
**          Validate page_size parameter
**	13-aug-1996 (somsa01)
**	    Added DMU_ETAB_JOURNAL for enabling journal after next checkpoint
**	    for etab tables.
**	17-sep-96 (nick)
**	    DMU_SYS_MAINTAINED now has a value. #78619
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	18-mar-1997 (rosga02)
**          Integrated changes for SEVMS
**          18-jan-95 (harch02)
**            Bug #74213
**            If supplied use a given security label id to create the table.
**            Needed to assign the base table security label id to an extension
**            table label id. They must match.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Set table priority from DMU_TABLE_PRIORITY, pass to 
**	    create_temporary_table(), dm2u_create().
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**      23-Jul-1998 (wanya01)
**          X-integrate change 432896(sarjo01)
**          Bug 67867: Added characteristic DMT_C_SYS_MAINTAINED to propagate
**          flag DM2U_SYS_MAINTAINED for temp tables.
**	21-mar-1999 (nanpr01)
**	    Get rid of the raw extents. One raw location only supports
**	    one table.
**	02-apr-1999 (nanpr01)
**	    Put back the missing parameter lost in forward integration.
**	17-dec-1999 (gupsh01)
**	    Corrected error checking in case of global temporary tables for 
**	    readonly databases. 
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-nov-2000 (stial01)
**          dmu_create() check available pgsize <= 65536 (B103081,Star 10406369)
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
**      02-jan-2004 (stial01)
**          Changes to support CREATE TABLE ... WITH [NO]BLOBJOURNALING;
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Disallow rows spanning pages if page type selected can't support it.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      13-may-2004 (stial01)
**          Removed changes to support CREATE TABLE WITH [NO]BLOBJOURNALING;
**      06-oct-2004 (stial01)
**          Allow rows spanning pages when used_default_page_size
**          Allow rows spanning pages for views
**	10-jan-2005 (thaju02)
**	    Changed dm2u_modify params for online modify of partitioned tbl.
**      02-mar-2005 (stial01)
**          New error for page type V5 WITH NODUPLICATES (b113993)
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	22-Feb-2008 (kschendel) SIR 122739
**	    Fix insanity whereby we use something called relstat partly
**	    as relstat and partly for "dm2u" flags, with no guarantee
**	    that they won't overlap.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**      24-aug-2009 (stial01)
**          Create some dbms catalogs with 8k page size for DB_MAXNAME 256 
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**/

/*
** <# defines>
*/

/*
** <typedefs>
** <forward references>
** <externs>
*/

/*
** <statics>
*/

static	DB_STATUS   create_temporary_table(
			DML_ODCB	    *odcb,
			DML_XCB		    *xcb,
			DB_TAB_NAME	    *table_name,
			DB_OWN_NAME	    *owner,
			DB_LOC_NAME	    *location,
			i4		    l_count,
			DB_TAB_ID	    *tbl_id,
			i4		    relstat,
			i4		    relstat2,
			i4		    structure,
			i4		    ntab_width,
			i4		    attr_count,
			DMF_ATTR_ENTRY	    **attr_entry,
			i4		    allocation,
			i4		    extend,
			i4		    page_size,
			i4		    recovery,
			i4		    tbl_pri,
			DB_ERROR	    *errcb);



/*{
** Name: dmu_create - Creates a table.
**
**  INTERNAL DMF call format:    status = dmu_create(&dmu_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMU_CREATE_TABLE,&dmu_cb);
**
** Description:
**      The dmu_create function handles the creation of permanent tables.
**      This dmu function is allowed inside a user specified transaction.
**      The table is always created with a heap storage structure.  The 
**      table name must not be the same as a system table name and must not be 
**      the same name as any other table owned by the same user. 
**      Returns an internal identifier needed on all other table operations
**      except show.
**
**	When a partition is to be created, the master should first be
**	created.  This will generate iiattribute rows and allocate
**	a batch of table ID's.  After that, partitions should be created,
**	passing the master table base ID each time in BOTH dmu_tbl_id base
**	AND dmu_tbl_id index, and the partition number in dmu_partno.
**	(If something funny is going on, like repartitioning, the idea
**	is that dmu_tbl_id index + dmu_partno + 1 is the generated
**	partition index ID number, and we'll attach the partition-flag.)
**	Partitions should pass all the master attribute stuff even though
**	no attribute rows will be created for them.  It is OK for
**	partitions to have different location specifications than the
**	master had.
**
** Inputs:
**      dmu_cb 
**         .type                            Must be set to DMU_UTILITY_CB.
**         .length                          Must be at least sizeof(DMU_CB).
**         .dmu_tran_id                     Must be the current transaction 
**                                          identifier returned from the begin 
**                                          transaction operation.
**         .dmu_flags_mask		    
**		DMU_VGRANT_OK		    DMT_VGRANT_OK bit must be set in
**					    relstat
**		DMU_PARTITION		    Creating a partition, see above
**         .dmu_db_id                       Must be the identifier for the 
**                                          odcb returned from open database.
**         .dmu_table_name                  External name of table to create.
**					    Ignored if the DMU_EXT_CREATE
**					    characteristic is specified (see
**					    below).
**         .dmu_tbl_id                      When DMF calls this routines for
**                                          creating an index this is 
**                                          the internal table identifier
**                                          of base table.
**					    When creating a partition, see
**					    above for what goes here.
**	   .dmu_qry_id                      When creating a view, this
**                                          is the query id associated 
**                                          with the view, otherwise 0.
**         .dmu_owner                       Owner of table to create.
**         .dmu_location.data_address       Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  
**	   .dmu_location.data_in_size       The size of the location array
**                                          in bytes.
**         .dmu_key_array.ptr_address	    Not currently used.
**                                          Pointer to array of pointer. Each
**                                          Pointer points to an attribute
**                                          entry of type DMU_KEY_ENTRY.  See
**                                          below for description of entry.
**         .dmu_key_array.ptr_in_count      The number of pointers in the array.
**	   .dmu_key_array.ptr_size	    The size of each element pointed at
**         .dmu_attr_array.ptr_address	    Pointer to array of pointer. Each
**                                          Pointer points to an attribute
**                                          entry of type DMU_ATT_ENTRY.  See
**                                          below for description of entry.
**         .dmu_attr_array.ptr_in_count     The number of pointers in the array.
**	   .dmu_attr_array.ptr_size	    The size of each element pointed at
**         .dmu_char_array.data_address     Pointer to an area used to pass 
**                                          an array of entries of type
**                                          DMU_CHAR_ENTRY.
**                                          See below for description of 
**                                          <dmu_char_array> entries.
**         .dmu_char_array.data_in_size     Length of char_array data in bytes.
**	   .dmu_gwchar_array.data_address   Pointer to an array of gateway table
**					    characteristics.  These are used
**					    if the table is a DMU_GATEWAY type
**					    table.  These characteristics are
**					    passed directly down to the Ingres
**					    Gateway system.
**	   .dmu_gwchar_array.data_in_size   Length of gwchar_array in bytes.
**	   .dmu_gwattr_array.ptr_address    Pointer to array of pointers, each
**					    of which describes gateway specific
**					    information about a table column.
**					    This is used only if the table is
**					    a DMU_GATEWAY type table.  These
**					    entries are passed directly down to
**					    the Ingres Gateway system.
**	   .dmu_gwattr_array.ptr_size	    The size of each element in array.
**	   .dmu_gwattr_array.ptr_address    The number of pointers in the array.
**
**          <dmu_key_array> entries are of type DMU_KEY_ENTRY and
**          must have following format:
**          key_attr_name                   Name of attribute.
**          key_order                       Must be DMU_ASCENDING or 
**                                          DMU_DESCENDING. DMU_DESCENDING 
**                                          is only legal on a heap structure.
**
**         <dmu_attr_array> entries are of type DMF_ATTR_ENTRY and 
**         must have following values:
**         attr_name                        Name of attribute.
**         attr_type                        Type of attribute. Must be one 
**                                          of the INGES supported types 
**                                          such as DB_(float)        
**         attr_size                        Size of attribute in bytes.
**         attr_precision                   Precision of attribute if 
**                                          type supports precision.
**	   attr_collID			    Collation ID (if string type).
**         attr_flags_mask                       Must be zero or DMU_F_NDEFAULT.
**
**         <dmu_char_array> entries are of type DMU_CHAR_ENTRY and 
**         must have following values:
**         char_id                          Characteristic identifier.
**                                          Must be one of the following:
**                                          DMU_STRUCTURE - storage structure:
**						DB_HASH_STORE, DB_ISAM_STORE,
**						DB_BTRE_STORE, DB_HEAP_STORE
**					    DMU_TEMP_TABLE - temporary table
**                                          DMU_VIEW_CREATE - view
**					    DMU_JOURNALED - journaled table
**                                          DMU_DUPLICATES - allows duplicates
**					    DMU_UNIQUE - table has unique keys
**					    DMU_IDX_CREATE - secondary index
**					    DMU_IFILL - index fill factor:
**						   numeric value between 1-100
**					    DMU_DATAFILL - datapage fill factor:
**						   numeric value between 1-100
**					    DMU_MINPAGES - min primary pages:
**						   numeric value > 0
**					    DMU_MAXPAGES - max primary pages:
**						   numeric value > 0
**					    DMU_SYS_MAINTAINED - has system
**						maintained columns
**					    DMU_GATEWAY - is a Gateway table
**					    DMU_RECOVERY - does this temporary
**						table perform recovery?
**						(IF GIVEN, MUST BE DMU_C_OFF)
**					    DMU_EXT_CREATE - is a table
**						extension (used for peripheral
**						datatype support -- blobs).
**					    DMU_TABLE_PRIORITY - value is
**						table's cache priority
**         char_value                       Value of characteristic. Unless
**					    specified above, must be an 
**					    integer value of DMU_C_ON,DMU_C_OFF.
**
** Outputs:
**         .dmu_tbl_id                      The internal table identifier
**                                          of table created.
**      dmu_cb   
**          .dmu_tbl_id                     The internal table identifier
**                                          assigned to this table.
**          .dmu_idx_id                     The internal table identifier
**                                          assigned to the index table.
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
**                                          E_DM001E_DUP_LOCATION_NAME
**                                          E_DM0021_TABLES_TOO_MANY
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM0039_BAD_TABLE_NAME
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM0071_LOCATIONS_TOO_MANY
**                                          E_DM0072_NO_LOCATION
**                                          E_DM0077_BAD_TABLE_CREATE
**                                          E_DM0078_TABLE_EXISTS
**                                          E_DM0079_BAD_OWNER_NAME
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM0100_DB_INCONSISTENT:
**                                          E_DM010C_TRAN_ABORTED:
**					    
**         Note the following is returned
**         with E_DB_WARN.
**                                          E_DM0101_SET_JOUNRAL_ON
**
**          .error.err_data                 Set to attribute in error by 
**                                          returning index into attribute 
**                                          array.
**					    OR (if err_code = E_DM001D_BAD_
**					        LOCATION_NAME) then an index
**						into the location array
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
**      13-jan-86 (jennifer)
**          Created for Jupiter.
**      19-nov-87 (jennifer)
**          Added multiple location support.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	8-Nov-1988 (teg)
**	    Added I/F to return index to bad location in error.err_data.
**      9-nov-1988 (teg)
**	    initialized new ADFCB parameter.
**	21-mar-1989 (jrb)
**	    Changed call to adc_lenchk to reflect a change in its interface.
**	17-apr-1989 (mikem)
**	    Logical key development.  Support creating a table with two new
**	    attribute attributes (DMU_F_SYS_MAINTAINED, DMU_F_WORM).
**      22-may-89 (jennifer)
**          For B1 systems check that attribute name is not one of 
**          the ones reserved for security label "ii_sec_label" or the
**          table identifier "ii_sec_tabkey".
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Accept DMU_GATEWAY attribute.
**	    Pass gateway attribute and characteristic options to dm2u_create.
**	    Added arguments to dm2u_create call.
**	24-dec-89 (paul)
**	    Add to non-sql gateway support. Another parameter for the filename.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**	 2-mar-90 (andre)
**	    dmu_flags_mask will no longer be required to be 0.  It may be set to
**	    DMU_VGRANT_OK to indicate that DMT_VGRANT_OK is to be set in
**	    relstat.
**	16-jun-90 (linda)
**	    Add call to dm2u_modify() for all gateway tables, so that tuple
**	    count and related information will be updated (normal Ingres table
**	    creation produces a table with 0 rows, but this is not true for
**	    gateway tables).
**	18-dec-90 (linda)
**	    Set unique variable to 1 if char value was DMU_C_ON, else set it
**	    to 0.  It was always being set to 1 if the characteristic entry
**	    for unique existed at all.
**	7-feb-1991 (bryanp)
**	    New argument to dm2u_modify() for Btree index compression.
**	17-jan-1992 (bryanp)
**	    Temporary table enhancements: Renamed DMU_TEMP_CREATE characteristic
**	    to DMU_TEMP_TABLE. If DMU_TEMP_TABLE characteristic
**	    is sent, call dmt_create_temp, not dm2u_create. New 'temporary'
**	    argument to dm2u_modify. Added support for DMU_RECOVERY
**	    characteristic and used it to set DMT_RECOVERY flag.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	17-aug-92 (rickh)
**	    Improvements for FIPS CONSTRAINTS.  Translate new relstat2 bits.
**	30-sep-1992 (robf)
**	    Pass char_array onto dm2u_create to pass to GWF for more
**	    error checking
**	04-nov-1992 (robf)
**	    When creating a VIEW make the audit message corrospond
**	    to the appropriate SXF message.
**	03-nov-1992 (ralph)
**	    Delimited identifiers:  remove attribute character validation.
**	    DMF should trust PSF to validate object name characters....
**	14-dec-1992 (rogerk)
**	    Initialize location count to 1 so that create's which specify no
**	    location list will be recorded in iirelation as having 1 location.
**	18-dec-1992 (robf)
**	    Improve auditing & error handling on failure from gateway or
**	    audit write.
**	30-mar-1993 (rmuth)
**	    Add mod_options2 field to dm2u_modify.
**	8-apr-93 (rickh)
**	    Added relstat2 argument to dm2u_modify call.
**	19-nov-93 (stephenb)
**	    Altered call to dma_write_audit() so that a better message is
**	    written to distinguish CREATE from a gateway REGISTER.
**	7-jul-94 (robf)
**          Add new parameter to dm2u_modify()
**      06-Jan-1995 (nanpr01)
**          Bug #66028
**          set the bit TABLE_RECOVERY_DISALLOWED the time of table 
**          creation.
**	09-Feb-1995 (cohmi01)
**	    For RAW I/O, pass new rawextname parm to dm2u_create.
**	    also pass a null rawextname to dm2u_modify.
**	06-mar-1996 (stial01 for bryanp)
**	    Recognize DMU_PAGE_SIZE characteristic; pass page_size argument to
**		dm2u_create() and dm2u_modify().
**	    If DM715 is set, keep changing svcb_page_size to enable a strange
**		pseudo-random page size selection within the server.
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**      19-apr-1996 (stial01)
**          Validate page_size parameter
**	06-may-1996 (nanpr01)
**	    Take out the alter_ok flag from the dm2u_maxreclen call.
**	13-aug-1996 (somsa01)
**	    Added DMU_ETAB_JOURNAL for enabling journal after next checkpoint
**	    for etab tables.
**	17-sep-96 (nick)
**	    At some point the usage of DMU_SYS_MAINTAINED changed from 
**	    being on if specified to having a value ; unfortunately, whoever
**	    did this forgot to change the assumption here ... #78619
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction 
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Set table priority from DMU_TABLE_PRIORITY, pass to 
**	    create_temporary_table(), dm2u_create().
**      28-may-1998 (stial01)
**          dmu_create() Support VPS system catalogs.
**      15-mar-1999 (stial01)
**          dmu_create() additional param to dm2u_modify
**	19-mar-1999 (nanpr01)
**	    Get rid of the raw extents. One raw location only supports
**	14-jul-1999 (nanpr01)
**	    Implementation of SIR 98092. Pick up the next available page size
**	    when row can not fit the default page_size.
**	17-dec-1999 (gupsh01)
**	    Corrected error checking in case of global temporary tables for 
**	    readonly databases. 
**      17-Apr-2001 (horda03) Bug 104402
**          Set the TCB_NOT_UNIQUE attribute.
**	04-Feb-2004 (jenjo02)
**	    Added support for GLOBAL indexes.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**	    Allow partition creates.
**	10-Mar-2004 (schka24)
**	    Fix modify new-partition braindamage, re-doc here
**	2-Apr-2004 (schka24)
**	    another param for dm2u-modify.
**	10-May-2004 (schka24)
**	    ODCB scb not the same as thread SCB in a factotum, remove check.
**	13-dec-04 (inkdo01)
**	    Added attr_collID.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	13-Apr-2006 (jenjo02)
**	    Handle DMU_CLUSTERED create option, use (new) DM2U_MOD_CB
**	    structure instead of stupidly long parm list.
**	27-Nov-2006 (kibro01) bug 117046
**	    Perform a test open and close on an IMA gateway file, since that
**	    is the only time the index options are validated and the user
**	    wants to know when the table is registered, not when it is used
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
*/
DB_STATUS
dmu_create(DMU_CB        *dmu_cb)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DMU_CB	    *dmu = dmu_cb;
    DML_XCB	    *xcb;
    DML_ODCB	    *odcb;
    DMU_CHAR_ENTRY  *char_entry;
    DMF_ATTR_ENTRY  **attr_entry;
    i4	    	    char_count;
    i4         	    attr_count;
    i4	    	    error, local_error;
    i4	    	    status;
    i4	    	    i,j;
    i4         	    index = 0;
    i4	    	    extension = 0;
    i4         	    structure = DB_HEAP_STORE;
    i4         	    duplicates = 0;
    i4         	    view = 0;
    i4         	    temporary = 0;
    i4	    	    recovery = 1;
    i4	    	    page_type = TCB_PG_INVALID;
    i4	    	    page_size = svcb->svcb_page_size;
    i4         	    bad_name, bad_attr;
    i4         	    db_lockmode;
    i4         	    ntab_width;
    DB_DATA_VALUE   adc_dv1;
    DB_STATUS       s;
    ADF_CB          adf_cb;
    i4         	    relstat = 0;
    u_i4            relstat2 = TCB2_TBL_RECOVERY_DISALLOWED;
    DB_LOC_NAME     *location;
    i4         	    loc_count = 1;
    bool            bad_loc;
    bool	    used_default_page_size = TRUE;
    i4		    gateway = 0;
    i4		    gwupdate = 0;
    i4	    	    allocation = 0;
    i4	    	    extend = 0;
    i4	    	    unique = 0;
    i4	    	    modoptions = 0;
    i4	    	    tup_info;
    i4		    bits;
    SXF_ACCESS	    access;
    i4	    	    message;
    i4	    	    mask;
    i4	    	    tbl_pri = 0;
    i4	    	    pgtype_flags;
    i4		    Clustered = FALSE;
    DB_ERROR	    local_dberr;

    CLRDBERR(&dmu->error);

    MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);
    status = E_DB_ERROR;

    do
    {
	/* dmu_flags_mask may be set to DMU_VGRANT_OK */
	mask = ~(DMU_VGRANT_OK|DMU_INTERNAL_REQ|DMU_PARTITION);
        if (dmu->dmu_flags_mask != 0 && (dmu->dmu_flags_mask & mask) != 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	xcb = (DML_XCB*) dmu->dmu_tran_id;
	
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

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

	odcb = (DML_ODCB*) dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	/*  Check that this is a update transaction on the database 
        **  that can be updated. */
	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	db_lockmode = DM2T_S;
	if (odcb->odcb_lk_mode == ODCB_X)
	    db_lockmode = DM2T_X;


	if ((dmu->dmu_flags_mask & DMU_PARTITION)
	  && dmu->dmu_tbl_id.db_tab_base == 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	/*  Check for characteristics. */

	if (dmu->dmu_char_array.data_address && 
		dmu->dmu_char_array.data_in_size)
	{
	    char_entry = (DMU_CHAR_ENTRY*) dmu->dmu_char_array.data_address;
	    char_count = dmu->dmu_char_array.data_in_size / 
			    sizeof(DMU_CHAR_ENTRY);
	    for (i = 0; i < char_count; i++)
	    {
		switch (char_entry[i].char_id)
		{
		case DMU_STRUCTURE:
		    structure = char_entry[i].char_value;
		    if (structure == DB_HEAP_STORE || 
                        structure == DB_HASH_STORE ||
                        structure == DB_ISAM_STORE ||
                        structure == DB_BTRE_STORE) 
			break;
		    else
		    {
			SETDBERR(&dmu->error, i, E_DM000E_BAD_CHAR_VALUE);
			return (E_DB_ERROR);
		    }		    

		case DMU_PAGE_SIZE:
		    used_default_page_size = FALSE;
		    page_size = char_entry[i].char_value;
		    if (page_size != 2048   && page_size != 4096  &&
			page_size != 8192   && page_size != 16384 &&
			page_size != 32768  && page_size != 65536)
		    {
			SETDBERR(&dmu->error, i, E_DM000E_BAD_CHAR_VALUE);
			return (E_DB_ERROR);
		    }		    
		    else if (!dm0p_has_buffers(page_size))
		    {
			SETDBERR(&dmu->error, i, E_DM0157_NO_BMCACHE_BUFFERS);
			return (E_DB_ERROR);
		    }		    
		    else
		    {
			break;
		    }		    

		case DMU_ALLOCATION:
		    allocation = char_entry[i].char_value;
		    break;

		case DMU_EXTEND:
		    extend = char_entry[i].char_value;
		    break;

		case DMU_DATAFILL:
		case DMU_IFILL:
		case DMU_MINPAGES:
		case DMU_MAXPAGES:
		    break;

		case DMU_UNIQUE:
		    unique = char_entry[i].char_value == DMU_C_ON;
		    break;

		case DMU_SYS_MAINTAINED:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat |= TCB_SYS_MAINTAINED;
		    break;

		case DMU_TEMP_TABLE:
		    /* Give GTT's and temp registers a break, and assume
		    ** "with norecovery" -- don't make the parser do it.
		    */
		    if (char_entry[i].char_value == DMU_C_ON)
		    {
			temporary = 1;
			recovery = 0;
		    }
		    break;

		case DMU_RECOVERY:
		    recovery = char_entry[i].char_value == DMU_C_ON;
		    break;

		case DMU_VIEW_CREATE:
		    view = char_entry[i].char_value == DMU_C_ON;
		    break;

		case DMU_DUPLICATES:
		    duplicates = char_entry[i].char_value == DMU_C_ON;
		    modoptions = (duplicates ? 0 : DM2U_NODUPLICATES);
		    break;

		case DMU_JOURNALED:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat |= TCB_JOURNAL;
		    break;

		case DMU_IDX_CREATE:
		    index = char_entry[i].char_value == DMU_C_ON;
		    break;

		case DMU_GATEWAY:
		    gateway = (char_entry[i].char_value == DMU_C_ON);
		    break;

		case DMU_GW_UPDT:
		    gwupdate = (char_entry[i].char_value == DMU_C_ON);
		    break;

		case DMU_EXT_CREATE:
		    extension = char_entry[i].char_value == DMU_C_ON;
		    break;
		    
		case DMU_NOT_DROPPABLE:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_NOT_DROPPABLE;
		    break;

		case DMU_STATEMENT_LEVEL_UNIQUE:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_STATEMENT_LEVEL_UNIQUE;
		    break;

		case DMU_PERSISTS_OVER_MODIFIES:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_PERSISTS_OVER_MODIFIES;
		    break;

		case DMU_SYSTEM_GENERATED:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_SYSTEM_GENERATED;
		    break;

		case DMU_SUPPORTS_CONSTRAINT:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_SUPPORTS_CONSTRAINT;
		    break;

		case DMU_NOT_UNIQUE:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_NOT_UNIQUE;
		    break;

		case DMU_ROW_SEC_AUDIT:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB_ROW_AUDIT;
		    break;

		case DMU_TABLE_PRIORITY:
		    tbl_pri = char_entry[i].char_value;

		    if (tbl_pri < 0 || tbl_pri > DB_MAX_TABLEPRI)
		    {
			SETDBERR(&dmu->error, i, E_DM000E_BAD_CHAR_VALUE);
			return (E_DB_ERROR);
		    }		    
		    break;

		case DMU_GLOBAL_INDEX:
		    if (char_entry[i].char_value == DMU_C_ON)
			relstat2 |= TCB2_GLOBAL_INDEX;
		    break;

		case DMU_CLUSTERED:
		    Clustered = char_entry[i].char_value == DMU_C_ON;
		    break;

		default:
		    SETDBERR(&dmu->error, i, E_DM000D_BAD_CHAR_ID);
		    return (E_DB_ERROR);
		}
	    }

	    if (
	      ((index || extension) && dmu->dmu_part_def != NULL)
		||
	      (temporary && recovery) )
	    {
		SETDBERR(&dmu->error, 0, E_DM002A_BAD_PARAMETER);
		break;
	    }

	    if (xcb->xcb_x_type & XCB_RONLY && !temporary)
	    {
		SETDBERR(&dmu->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
		break;
	    }
	}

	if (index)
	    pgtype_flags = DM1C_CREATE_INDEX;
	else if (extension)
	    pgtype_flags = DM1C_CREATE_ETAB;
	else
	    pgtype_flags = DM1C_CREATE_DEFAULT;

	if ((dmu->dmu_table_name.db_tab_name[0] == 'i' &&
	    dmu->dmu_table_name.db_tab_name[1] == 'i') && !extension)
	{
	    pgtype_flags |= DM1C_CREATE_CATALOG;
	    if (page_size < 8192  &&
		(2*DB_MAXNAME) > DM1B_COMPAT_KEYLENGTH &&
		dm0p_has_buffers(8192) &&
		(!MEcmp(dmu->dmu_table_name.db_tab_name, "iiprocedure", 11) ||
		!MEcmp(dmu->dmu_table_name.db_tab_name, "iiusergroup", 11) ||
		!MEcmp(dmu->dmu_table_name.db_tab_name, "iirolegrant", 11) ||
		!MEcmp(dmu->dmu_table_name.db_tab_name, "iidbpriv", 8)))
		page_size = 8192; /* temporary fix !!! */
	}

	/*  Make sure a structure was given. */
	if (structure == 0)
	    structure = DB_HEAP_STORE;

	if ( Clustered && structure == DB_BTRE_STORE )
	{
	    pgtype_flags |= DM1C_CREATE_CLUSTERED;
	    /* Clustered implies and requires Unique */
	    unique = TRUE;
	}
	else
	     Clustered = FALSE;

	/* Ensure no junk in gateway ID so that we can easily test for
	** is-gateway by looking at gateway ID.
	*/
	if (!gateway)
	    dmu->dmu_gw_id = DMGW_NONE;

	/* Check the attribute values. */

	if (dmu->dmu_attr_array.ptr_address && 
	    (dmu->dmu_attr_array.ptr_size == sizeof(DMF_ATTR_ENTRY))
	    )
	{
	    attr_entry = (DMF_ATTR_ENTRY**) dmu->dmu_attr_array.ptr_address;
	    attr_count = dmu->dmu_attr_array.ptr_in_count;
	    ntab_width = 0;
	    for (i = 0; i < attr_count; i++)
	    {
		bad_attr = FALSE;
		bad_name = 1;

		/*
		** Validation of characters in the attribute name
		** was removed from here as part of the delimited
		** identifier project.  DMF should "trust" PSF
		** to validate and translate the attribute name.
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
			bad_name = 0;
			SETDBERR(&dmu->error, i, E_DM0007_BAD_ATTR_NAME);
			break;
		    }	    	    
		}
		if (bad_name == 0)
		    break;		    

		adf_cb.adf_errcb.ad_ebuflen = 0;
		adf_cb.adf_errcb.ad_errmsgp = 0;
		adf_cb.adf_maxstring = DB_MAXSTRING;
                adc_dv1.db_datatype = (i2) attr_entry[i]->attr_type;
                adc_dv1.db_length   = attr_entry[i]->attr_size;
                adc_dv1.db_prec     = (i2) attr_entry[i]->attr_precision;
                adc_dv1.db_collID   = attr_entry[i]->attr_collID;
	        s = adc_lenchk(&adf_cb, FALSE, &adc_dv1, NULL);
		if (s != E_DB_OK)
		{
		    if (adf_cb.adf_errcb.ad_errcode == E_AD2004_BAD_DTID)
			SETDBERR(&dmu->error, i, E_DM000A_BAD_ATTR_TYPE);
		    else
			SETDBERR(&dmu->error, i, E_DM0009_BAD_ATTR_SIZE);
	            bad_attr = TRUE;
		    break;
    		}	    	
		ntab_width += attr_entry[i]->attr_size;
		s = adi_dtinfo(&adf_cb, attr_entry[i]->attr_type, &bits);
		if (s != E_DB_OK)
		{
		    dmu->error = adf_cb.adf_errcb.ad_dberror;
		    dmu->error.err_data = i;
		    bad_attr = TRUE;
		    break;
		}
		if (bits & AD_PERIPHERAL)
		{
		    /*
		    ** If peripheral datatypes are found,
		    **	1) mark the attribute as peripheral, and
		    **	2) mark the relation as having extensions.
		    **
		    **	The `having extensions' is necessary because DMF uses
		    **	table keys to keep the peripheral datatypes in sync with
		    **	the base tuple.  A table key in the peripheral
		    **	datatype's coupon is used as the key in the table
		    **	extension.
		    */
		    
		    attr_entry[i]->attr_flags_mask |= DMU_F_PERIPHERAL;
		    relstat2 |= TCB2_HAS_EXTENSIONS;
		}
		/* check for appropriate attribute flags */
		if (attr_entry[i]->attr_flags_mask & ~DMU_F_LEGAL_ATTR_BITS)
		{
		    SETDBERR(&dmu->error, i, E_DM0006_BAD_ATTR_FLAGS);
		    bad_attr = TRUE;
		    break;
		}
	    }

	    if (bad_attr == TRUE || bad_name == 0)
		break;
	}
	else
	{
	    /* Must have at least one attribute. */
	    break;	    	
	}

	if (ntab_width > DM_TUPLEN_MAX_V5)
	{
	    SETDBERR(&dmu->error, 0, E_DM0103_TUPLE_TOO_WIDE);
	    break;
	}

	/* choose page type */
	status = dm1c_getpgtype(page_size, pgtype_flags,
			ntab_width, &page_type);

	if (status != E_DB_OK)
	{
	    page_type = TCB_PG_INVALID;

	    if (!used_default_page_size)
	    {
		SETDBERR(&dmu->error, 0, E_DM0103_TUPLE_TOO_WIDE);
		break;
	    }
	}

	/* 
	** If the default page size won't work or 
	** or will work only with rows spanning pages
	** look for a bigger page size that will hold the row
	*/
	if ((status != E_DB_OK ||
		ntab_width > dm2u_maxreclen(page_type, page_size))
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
		if (!dm0p_has_buffers(pgsize))
		    continue;

		/* choose page type */
		status = dm1c_getpgtype(pgsize, pgtype_flags, 
			    ntab_width, &pgtype);

		if (status != E_DB_OK ||
			ntab_width > dm2u_maxreclen(pgtype, pgsize))
		    continue;

		page_size = pgsize;
		page_type = pgtype;
		break;
	    }
	}

	/*
	** It is possible we didn't find a valid (page size, page type)
	** This can happen if the dbms is not configured with V5 pages 
	*/
	if (page_type ==  TCB_PG_INVALID)
	{
	    SETDBERR(&dmu->error, 0, E_DM0103_TUPLE_TOO_WIDE);
	    break;
	}

	/*
	** Disallow rows spanning pages if duplicate row checking possible
	** (Duplicate row checking in dm1b, dm1h, dm1i not supported for
	** rows spanning pages)
	**
	** but we should probably disallow only if
	**  (ntab_width > dm2u_maxreclen(page_type, 65536)
	** (the row size can't fit on ANY page size)
	** because duplicate row checking will only be done if 
	** this table is later modified to non unique btree/hash/isam
	*/
	if (ntab_width > dm2u_maxreclen(page_type, page_size) &&
						!duplicates && !view)
	{
	    SETDBERR(&dmu->error, 0, E_DM0159_NODUPLICATES);
	    break;
	}

	/* If it gets here, then everything was OK. */

	/* Check the location names for duplicates, too many. */

	location = 0;
	if (dmu->dmu_location.data_address && 
	    (dmu->dmu_location.data_in_size >= sizeof(DB_LOC_NAME))
	    )
	{
	    location = (DB_LOC_NAME *) dmu->dmu_location.data_address;
	    loc_count = dmu->dmu_location.data_in_size/sizeof(DB_LOC_NAME);
	    if (loc_count > DM_LOC_MAX)
	    {
		SETDBERR(&dmu->error, 0, E_DM0071_LOCATIONS_TOO_MANY);
		break;
	    }
	    bad_loc = FALSE;
	    for (i = 0; i < loc_count; i++)
	    {
		for (j = 0; j < i; j++)
		{
		    /* 
                    ** Compare this attribute name against other 
                    ** already given, they cannot be the same.
                    */

		    if (MEcmp(location[j].db_loc_name,
                              location[i].db_loc_name,
			      sizeof(DB_LOC_NAME)) == 0 )
		    {
			SETDBERR(&dmu->error, i, E_DM001E_DUP_LOCATION_NAME);
			bad_loc = TRUE;
			break;
		    }	    	    
		}
		if (bad_loc == TRUE)
		    break;		    
	    }

	    if (bad_loc == TRUE)
		break;
	}
	if (location == 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM0072_NO_LOCATION);
	    break;
	}

	/* If it gets here everything is OK. */

    } while (FALSE);

    if (dmu->error.err_code)
    {
	if (dmu->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&dmu->error, 0, E_DM0077_BAD_TABLE_CREATE);
	}
	return (E_DB_ERROR);
    }

    /*
    ** If this is the first write operation for this transaction,
    ** then we need to write the begin transaction record.
    */
    if ((xcb->xcb_flags & XCB_DELAYBT) != 0 && recovery != 0)
    {
	status = dmxe_writebt(xcb, TRUE, &dmu->error);
	if (status != E_DB_OK)
	{
	    xcb->xcb_state |= XCB_TRANABORT;
	    if (dmu->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmu->error, 0, E_DM0077_BAD_TABLE_CREATE);
	    }
	    return (E_DB_ERROR);
	}
    }

    if (used_default_page_size == TRUE && (dmu->dmu_flags_mask & DMU_PARTITION) == 0)
    {
	if (DMZ_TBL_MACRO(15))
	{
	    i4 next;

	    /*
	    ** Trace point DM715 keeps changing the default page size, for
	    ** testing purposes in which we wish to have a "nice" selection of
	    ** page sizes in the server.
	    */
	    if (svcb->svcb_page_size == 65536)
		next = 2048;
	    else
		next = svcb->svcb_page_size * 2;

	    for (  ; next != (i4) svcb->svcb_page_size; )
	    {
		if (dm0p_has_buffers(next))
		{
		    svcb->svcb_page_size = next;
		    break;
		}

		if (next == 65536)
		    next = 2048;
		else
		    next *= 2;
	    }
	}
    }

    /* 
    ** All parameters are acceptable, now check the relation name.
    ** This is done separately since it is a somewhat complicated task
    ** involving Locking the table name, checking to see if it already
    ** exists as a system table or as a table previously defined by 
    ** this user. 
    */

    if (duplicates)
	relstat |= TCB_DUPLICATES;
    if (gateway)
    {
	relstat |= TCB_GATEWAY | TCB_GW_NOUPDT;
	if (gwupdate)
	    relstat &= ~TCB_GW_NOUPDT;
    }
    if (dmu->dmu_part_def != NULL && (dmu->dmu_flags_mask & DMU_PARTITION) == 0)
	relstat |= TCB_IS_PARTITIONED;
    if ( Clustered )
	relstat |= TCB_CLUSTERED;

    if (dmu->dmu_flags_mask & DMU_PARTITION)
	relstat2 |= TCB2_PARTITION;

    /*
    ** Check for view grant permission - this is passed in flags mask instead
    ** of using the table characteristics array (we might want to change
    ** this in the future to conform to how other table options are passed in).
    */
    if (view && dmu->dmu_flags_mask & DMU_VGRANT_OK)
	relstat |= TCB_VGRANT_OK;
    if (extension)
	relstat2 |= TCB2_EXTENSION;
	
    /* Calls the physical layer to process the rest of the create */

    if (temporary)
    {
	status = create_temporary_table(odcb, xcb,
			&dmu->dmu_table_name, 
			&dmu->dmu_owner, location, loc_count, 
			&dmu->dmu_tbl_id,
			relstat, relstat2, structure, ntab_width, attr_count, 
			attr_entry,
			allocation, extend, page_size, recovery,
			tbl_pri,
			&dmu->error);

    }
    else
	status = dm2u_create(odcb->odcb_dcb_ptr, xcb, &dmu->dmu_table_name, 
			&dmu->dmu_owner, location, loc_count, 
			&dmu->dmu_tbl_id, &dmu->dmu_idx_id, index, view, 
			relstat, relstat2, structure, ntab_width, attr_count, 
			attr_entry, db_lockmode,
			allocation, extend, page_type, page_size,
			&dmu->dmu_qry_id, &dmu->dmu_gwattr_array,
			&dmu->dmu_gwchar_array, dmu->dmu_gw_id,
			dmu->dmu_gwrowcount,
			(DMU_FROM_PATH_ENTRY *)dmu->dmu_olocation.data_address,
			&dmu->dmu_char_array,
			0, 0, (f8 *)NULL, tbl_pri,
			dmu->dmu_part_def, dmu->dmu_partno, &dmu->error);

    if (dmu->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmu->error, 0, E_DM0077_BAD_TABLE_CREATE);
    }

    /*
    **  Audit creation of TABLE/VIEW. 
    **  For a gateway, we log REGISTER rather than CREATE, since thats what 
    **	are doing.
    */
    /*
    ** For temporary tables we will use new new message I_SX2040_TMP_TBL_CREATE
    ** and log the current username as the table owner
    ** ****FIXME how to log partitions, or log them at all?
    */
    s = E_DB_OK;
    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	if (status==E_DB_OK)
	    access=SXF_A_SUCCESS;
	else
	    access=SXF_A_FAIL;

	if (gateway) /* gateway table */
	{
	    access |= SXF_A_REGISTER;
	    message = I_SX2043_GW_REGISTER;
	}
	else if (view) /* view */
	{
	    access |= SXF_A_CREATE;
	    message = I_SX2014_VIEW_CREATE;
	}
	else if (temporary) /* temporary table */
	{
	    access |= SXF_A_CREATE;
	    message = I_SX2040_TMP_TBL_CREATE;
	}
	else /* ordinary ingres table */
	{
	    access |= SXF_A_CREATE;
	    message = I_SX201E_TABLE_CREATE;
	}

	s = dma_write_audit(
	    view ? SXF_E_VIEW : SXF_E_TABLE,
	    access,
	    dmu->dmu_table_name.db_tab_name,/* Table/view name */
	    sizeof(dmu->dmu_table_name.db_tab_name),/* Table/view name */
	    temporary ? &xcb->xcb_username : &dmu->dmu_owner, /* Table/view owner */
	    message,
	    FALSE, /* Not force */
	    &local_dberr, NULL);
    }

    if (status != E_DB_OK || s!=E_DB_OK)
    {
	switch (dmu->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM0077_BAD_TABLE_CREATE:
	    case E_DM0136_GATEWAY_ERROR:
	    case E_DM006A_TRAN_ACCESS_CONFLICT:
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
	}
	/*
	** If the audit returned a fatal error remember to
	** return this.
	*/
	if (s > status)
	{
	    status = s;
	    dmu->error = local_dberr;
	}

	return (status);
    }

    if (gateway)
    {
	DM2U_MOD_CB	local_mcb, *mcb = &local_mcb;
	/* 
	** If this is a gateway table, then perform a modify to record the key
	** columns.  dm2u_modify() handles updating catalog information without
	** actually trying to physically modify the table.
	*/

	/* Init the MCB for dm2u_modify: */
	mcb->mcb_dcb = odcb->odcb_dcb_ptr;
	mcb->mcb_xcb = xcb;
	mcb->mcb_tbl_id = &dmu->dmu_tbl_id;
	mcb->mcb_omcb = (DM2U_OMCB*)NULL;
	mcb->mcb_dmu = (DMU_CB*)NULL;
	mcb->mcb_structure = structure;
	mcb->mcb_location = location;
	mcb->mcb_l_count = loc_count;
	mcb->mcb_i_fill = 0;
	mcb->mcb_l_fill = 0;
	mcb->mcb_d_fill = 0;
	mcb->mcb_compressed = TCB_C_NONE;
	mcb->mcb_index_compressed = FALSE;
	mcb->mcb_temporary = FALSE;
	mcb->mcb_unique = unique;
	mcb->mcb_merge = FALSE;
	mcb->mcb_clustered = FALSE;
	mcb->mcb_min_pages = 0;
	mcb->mcb_max_pages = 0;
	mcb->mcb_modoptions = modoptions;
	mcb->mcb_mod_options2 = 0;
	mcb->mcb_kcount = dmu_cb->dmu_key_array.ptr_in_count;
	mcb->mcb_key = (DM2U_KEY_ENTRY**)dmu->dmu_key_array.ptr_address;
	mcb->mcb_db_lockmode = DM2T_X;
	mcb->mcb_allocation = allocation;
	mcb->mcb_extend = extend;
	mcb->mcb_page_type = page_type;
	mcb->mcb_page_size = page_size;
	mcb->mcb_tup_info = &tup_info;
	mcb->mcb_reltups = dmu->dmu_gwrowcount;
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

	status = dm2u_modify(mcb, &dmu->error);
	if (status != E_DB_OK)
	{
	    if (dmu->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmu->error, 0, E_DM0077_BAD_TABLE_CREATE);
	    }

	    switch (dmu->error.err_code)
	    {
		case E_DM004B_LOCK_QUOTA_EXCEEDED:
		case E_DM0112_RESOURCE_QUOTA_EXCEED:
		case E_DM0136_GATEWAY_ERROR:
		    xcb->xcb_state |= XCB_STMTABORT;
		    break;

		case E_DM0065_USER_INTR:
		    xcb->xcb_state |= XCB_USER_INTR;
		    break;

		case E_DM010C_TRAN_ABORTED:
		    xcb->xcb_state |= XCB_FORCE_ABORT;
		    break;

		default:
		    xcb->xcb_state |= XCB_TRANABORT;
		    break;
	    }
	    /* Think we should return now... */
	    return (status);
	}

	if (dmu->dmu_gw_id == GW_IMA)
	{
	    /* Now we have changed the IMA gateway file, do a dummy open/close 
	    ** which performs the validation on the options used.  We want to 
	    ** tell the user that it won't work when it's created - not when
	    ** it's used.  (kibro01) bug 117046
	    */
	    DMP_RCB *this_rcb;
	    DB_TAB_TIMESTAMP    timestamp;

	    status = dm2t_open(odcb->odcb_dcb_ptr, &dmu->dmu_tbl_id,
		DM2T_N,		/* No readlock */
		DM2T_UDIRECT, 	/* */
		DM2T_A_READ,	/* Open for read access */
		0,		/* Timeout */
		20,		/* Maxlocks */
		0,		/* sp_id */
		xcb->xcb_log_id,	/* log_id */
		xcb->xcb_lk_id,	/* lock_id */
		0,		/* sequence */
		0,		/* iso_level */
		db_lockmode,
		&xcb->xcb_tran_id,
		&timestamp, &this_rcb, NULL, &dmu->error);

	    if (status == E_DB_OK)
	    {
	        status = dm2t_close(this_rcb, 0, &dmu->error);
	    }
	    if (status != E_DB_OK)
	    {
	        switch (dmu->error.err_code)
	        {
		    case E_DM004B_LOCK_QUOTA_EXCEEDED:
		    case E_DM0112_RESOURCE_QUOTA_EXCEED:
		    case E_DM0136_GATEWAY_ERROR:
		        xcb->xcb_state |= XCB_STMTABORT;
		        break;

		    case E_DM0065_USER_INTR:
		        xcb->xcb_state |= XCB_USER_INTR;
		        break;

		    case E_DM010C_TRAN_ABORTED:
		        xcb->xcb_state |= XCB_FORCE_ABORT;
		        break;

		    default:
		        xcb->xcb_state |= XCB_TRANABORT;
		        break;
	        }

	        return (status);
	    }
	} /* end if dmu->dmu_gw_id == GW_IMA */
    } /* end "if (gateway)" */

    return (E_DB_OK);

}

/*
** Name: create_temporary_table	    - create a temporary table
**
** Description:
**	This routine is called by dmu_create to create a temporary table.
**
**	It sets up the appropriate control blocks and calls dmt_create_temp
**	to perform the temporary table creation.
**
** Inputs:
**	odcb				The Open Database Control Block ptr
**	xcb				The transaction id.
**	table_name			Name of the table to be created.
**	owner				Owner of the table to be created.
**	location			Location where the table will reside.
**      l_count                         Number of locations given.
**	relstat				Some initial relstat flags:
**					  TCB_DUPLICATES   - allow duplicates
**					  TCB_SYS_MAINTAINED - 
**						contains at least one attribute
**						(ie. a logical key, ...)
**					  TCB_GATEWAY	    - gateway table
**	relstat2			More flags, corresponding to relstat2;
**					is-a-partition, has-extensions.
**	ntab_width			Width in bytes of the table record 
**					width.
**	attr_count			Number of attributes in the table.
**	attr_entry			Pointer to an attribute description
**					array of pointers.
**	allocation			initial allocation
**	extend				amount to extend by
**	page_size			Page size for table.
**	recovery			Does this temp table perform recovery?
**					    0 -- no, it does not
**					    !0 -- yes, it does
**
** Outputs:
**	tbl_id				The internal table id assigned to the
**					created table.
**      errcb->err_code		        The reason for an error return status.
**	errcb->err_data			Data assoc with error status.  Currently
**					only supply index to location in the
**					location_array.
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
**	17-jan-1992 (bryanp)
**	    Created.
**	21-jan-1992 (bryanp)
**	    Added allocation & extend arguments; ignored for now.
**	31-jan-1992 (bryanp)
**	    Added recovery argument and used it to set DMT_RECOVERY.
**      20-Oct-1993 (fred)
**          Added support for session temporaries having large object
**          attributes.  This had been misplaced.
**      23-may-1994 (andys)
**          Add cast(s) to quiet qac/lint
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument.
**	06-mar-1996 (nanpr01)
**	    size of array.data_in_size = 5 * sizeof(DMT_CHAR_ENTRY)
**	    instead of array.data_in_size = 4 * sizeof(DMT_CHAR_ENTRY)
**	    due to increase in array size.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Passed table priority which we pass on to dmt_create_temp().
**      23-Jul-1998 (wanya01)
**          X-integrate change 432896(sarjo01)
**          Bug 67867: Added characteristic DMT_C_SYS_MAINTAINED to propagate
**          flag DM2U_SYS_MAINTAINED for temp tables.
*/

#define NUM_TEMP_CHARACTERISTICS 7

static DB_STATUS
create_temporary_table(
DML_ODCB	    *odcb,
DML_XCB		    *xcb,
DB_TAB_NAME	    *table_name,
DB_OWN_NAME	    *owner,
DB_LOC_NAME	    *location,
i4		    l_count,
DB_TAB_ID	    *tbl_id,
i4		    relstat,
i4		    relstat2,
i4		    structure,
i4		    ntab_width,
i4		    attr_count,
DMF_ATTR_ENTRY	    **attr_entry,
i4		    allocation,
i4		    extend,
i4		    page_size,
i4		    recovery,
i4		    tbl_pri,
DB_ERROR	    *errcb)
{
    DMT_CB		dmt_cb;
    DB_STATUS		status;
    DMT_CHAR_ENTRY	characteristics[NUM_TEMP_CHARACTERISTICS];

    if (allocation == 0)
	allocation = DM_TTBL_DEFAULT_ALLOCATION;
    if (extend == 0)
	extend = DM_TTBL_DEFAULT_EXTEND;

    /*
    ** Set up DMT_CB control block appropriately and call dmt_create_temp
    */

    MEfill(sizeof(DMT_CB), '\0', (PTR)&dmt_cb);
    dmt_cb.length = sizeof(DMT_CB);
    dmt_cb.type = DMT_TABLE_CB;
    dmt_cb.dmt_flags_mask = DMT_SESSION_TEMP;
    if (recovery)
	dmt_cb.dmt_flags_mask |= DMT_RECOVERY;
    characteristics[0].char_id = DMT_C_DUPLICATES;
    characteristics[0].char_value =
		    (relstat & TCB_DUPLICATES) ? DMT_C_ON : DMT_C_OFF;
    characteristics[1].char_id = DMT_C_ALLOCATION;
    characteristics[1].char_value = allocation;
    characteristics[2].char_id = DMT_C_EXTEND;
    characteristics[2].char_value = extend;
    characteristics[3].char_id = DMT_C_HAS_EXTENSIONS;
    characteristics[3].char_value =
	    	    (relstat2 & TCB2_HAS_EXTENSIONS) ? DMT_C_ON : DMT_C_OFF;
    characteristics[4].char_id = DMT_C_PAGE_SIZE;
    characteristics[4].char_value = page_size;
    characteristics[5].char_id = DMT_C_TABLE_PRIORITY;
    characteristics[5].char_value = tbl_pri;
    characteristics[6].char_id = DMT_C_SYS_MAINTAINED;
    characteristics[6].char_value =
                  (relstat & TCB_SYS_MAINTAINED) ? DMT_C_ON : DMT_C_OFF;
    dmt_cb.dmt_char_array.data_address = (PTR)characteristics;
    dmt_cb.dmt_char_array.data_in_size = NUM_TEMP_CHARACTERISTICS *
                              sizeof(DMT_CHAR_ENTRY);
    dmt_cb.dmt_db_id = (PTR)odcb;
    dmt_cb.dmt_tran_id = (PTR)xcb;
    MEcopy((char *)table_name, sizeof(dmt_cb.dmt_table), 
			(char *)&dmt_cb.dmt_table);
    MEcopy((char *)owner,      sizeof(dmt_cb.dmt_owner), 
			(char *)&dmt_cb.dmt_owner);
    dmt_cb.dmt_location.data_address = (char *)location;
    dmt_cb.dmt_location.data_in_size = l_count * sizeof(DB_LOC_NAME);
    dmt_cb.dmt_attr_array.ptr_address = (PTR)attr_entry;
    dmt_cb.dmt_attr_array.ptr_in_count = attr_count;
    dmt_cb.dmt_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

    status = dmt_create_temp(&dmt_cb);

    if (status == E_DB_OK)
	STRUCT_ASSIGN_MACRO(dmt_cb.dmt_id, *tbl_id);
    else
        *errcb = dmt_cb.error;

    return (status);
}
