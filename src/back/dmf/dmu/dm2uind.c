/*
** Copyright (c) 1986, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <tm.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
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
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm0s.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dma.h>
#include    <cm.h>
#include    <cui.h>
#include    <gwf.h>

/**
**
**  Name: DM2UINDEX.C - Index table utility operation.
**
**  Description:
**      This file contains routines that perform the index table
**	functionality.
**      dm2u_index - Create an index on a table.
**	dm2uMakeMxcb - Create MXCB from index CB
**	dm2uMakeIndAtts - create the index attributes and stuff
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
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of index operation.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.
**	27-sep-89 (jrb)
**	    Initialized cmp_precision field properly.
**	06-nov-89 (fred)
**	    Added support for non-keyable datatypes as index keys.  More
**	    correctly, added code to detect and reject this usage.  This should
**	    be checked for in PSF, but we will double check here for safety's
**	    sake.
**	30-apr-90 (bryanp)
**	    Fixed a memory smashing bug in secondary hash index creation.
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
**	06-feb-1992 (ananth)
**	    Use the ADF_CB in the RCB instead of allocating in on the stack
**	    in dm2u_index().
**	14-apr-1992 (bryanp)
**	    Set mx_inmemory and mx_buildrcb fields in the MXCB.
**      26-may-1992 (schang)
**          GW merge.  Add gw_id as last arg of dm2u_index, and fix
**          the bug that registering index with nonexisting RMS file
**          cause base table to be locked.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	16-jul-1992 (kwatts)
**	    MPF change. Use accessor to calculate possible expansion on
**	    compression (comp_katts_count).
**	27-aug-92 (rickh)
**	    FIPS CONSTRAINTS:  new relstat2 field in IIRELATION tuple.
**	29-August-1992 (rmuth)
**	    Use constants for the default allocation and extend size.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	3-oct-1992 (bryanp)
**	    New characteristics array argument passed to dm2u_create for
**	    gateway use.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	07-oct-92 (robf)
**	    Make a better guess at gateway rowcount (same as base table's)
**  	    and page count, so optimizer can get better query plans.
**	16-oct-1992 (rickh)
**	    Added initialization of default values.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**      10-dec-1992 (schang)
**          For RMS gateway, do not fill in the index row number, and return
**          zero.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location field since it's no longer
**	    used.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.
**	25-feb-1993 (rickh)
**	    Make sure to set relstat2.
**	30-feb-1993 (rmuth)
**	    When calling dm2u_create to create the table which is turned
**	    into the index we currently pass down the index's allocation
**	    and extend sizes. This is bad as if the user has specified an
**	    allocation of 1000 we would first create the heap table with
**	    1000 pages and then create the index with 1000 pages and then
**	    delete the former. To aliveate the problem change the code to
**	    pass the default allocation and extend sizes, hence a table
**	    of 4 pages will be created and destroyed. Ideally we should
**	    build the index over the heap table.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**	8-apr-93 (rickh)
**	    Initialize relstat2 for dm2u_update_catalogs call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	05-apr-1993 (ralph)
**	    DELIM_IDENT: Use appropriate case for "tidp"
**	15-may-1993 (rmuth)
**	    Concurrent-Index: Add support for an index to be built
**	    by only taking share locks out on the base table.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (rogerk)
**	    Pass index_compression attribute in the dm0l_index log record.
**	    Previously we were passing only the data-compression attribute,
**	    and ignore index_compression in the log record.  This caused
**	    rollforward to build compressed btree indexes incorrectly.
**	21-June-1993 (rmuth)
** 	    File Extend: Make sure that for multi-location tables that
**	    allocation and extend size are factors of DI_EXTENT_SIZE.
**	21-jun-1993 (jnash)
**	    Force table pages prior to writing index log records.  This
**	    is required because the index log record flags a "non-redo"
**	    log address for recovery.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the index process for new recovery protocols.
**	    Divorced the file creation from the table build operation and
**	    moved both of these to before the logging of dm0l_index.
**	  - Removed file creation from the dm2u_reorg_table routine.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.
**	  - Added a statement count field to the modify and index log records.
**	  - Fix CL prototype mismatches.
**	23-aug-1993 (rmuth)
**	    Always open the base table in DM2T_A_READ mode.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	18-oct-1993 (rogerk)
**	    Moved call to dm2u_calc_hash_buckets up from dm2u_load_table so
**	    that the bucket number could be logged in the dm0l_index log
**	    record.  This allows rollforward to run deterministically.
**	05-nov-93 (swm)
**	    Bug #58633
**	    Alignment offset calculations used a hard-wired value of 3
**	    to force alignment to 4 byte boundaries, for both pointer and
**	    i4 alignment. Changed these calculations to use
**	    ME_ALIGN_MACRO.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them
**	    prior to close.
**	11-apr-1994 (rmuth)
**	    Remove some old redundent code.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**	03-may-1995 (morayf)
**	    De-ANSIfied dm2u_index() declaration for odt_us5 as SCO C compiler
**	    can currently only handle up to 31 function prototype arguments.
**	    Thus unfortunately 2 sets of argument lists should be maintained.
**	10-feb-1995 (cohmi01)
**	    For RAW I/O, pass DB_RAWEXT_NAME parm to dm2u_create.
**          Add DB_RAWEXT_NAME parm to dm2u_index.
**	05-may-1995 (shero03)
**          Support RTree
**	07-may-1995 (wadag01)
**	    Aligned separately maintained parameter list for SCO's (odt_us5)
**	    version of dm2_index(): added DB_RAWEXT_NAME.
**	06-mar-1996 (stial01 for bryanp)
**	    Accept new page_size argument and use it to pass page_size argument
**		to dm2u_create().
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**          Pass page size to dm2f_build_fcb()
**      06-may-1996 (nanpr01)
**          Changed parameters to dm2u_maxreclen.
**          Modified the isam key length calculation. We never intended to
**	    support the increased key size for this project. So we use
**	    DM_COMPAT_PGSIZE parameter for key length checking.
**	 3-jun-96 (nick)
**	    Don't journal index creation on relations which won't be journaled
**	    until next checkpoint. #76741
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	21-nov-1996 (shero03)
**	    B79469: only move RTree parameters when the structure is RTREE
**      27-jan-1997 (stial01)
**          Do not include TIDP in the key of unique secondary indexes
**      31-jan-1997 (stial01)
**          Commented code for 27-jan-1997 change
**      10-mar-1997 (stial01)
**          Alloc/init mx_crecord, area to be used for compression.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added tbl_pri parm to dm2u_index() prototype, dm2u_create() call.
**	12-apr-1997 (shero03)
**	    Remove dimension from input parms of get_klv
**      28-apr-1997 (stial01)
**          dm2u_index() Init mx_atts_ptr[i+1] from tcb_atts_ptr[j] so that 
**          new fields added for alter table get initialized.
**      21-may-1997 (stial01)
**          dm2u_index() Always include tidp in "key length" for BTREE.
**          dm2u_index() Init new fields added for alter table in attribute
**          info for tidp.
**          For compatibility reasons, DO include TIDP for V1-2k unique indices
**	28-Jul-1997 (shero03)
**	    Allow an Rtree index to be build on a long spatial data type
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_close_page() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	02-Oct-1997 (shero03)
**	    Changed the code to use the mx_log_id in lieu of the xcb_log_id
**	    because there is no xcb during recovery.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**	11-Aug-1998 (jenjo02)
**	    When counting tuples, count current RCB changes as well.
**      23-nov-1998 (stial01)
**          Moved check for bad keylength back after addition of TIDP to keylist
**          (b94278).
**	23-Dec-1998 (jenjo02)
**	    TCB->RCB list must be mutexed while it being read.
**      15-mar-1999 (stial01)
**          dm2u_index() can get called if DCB_S_ROLLFORWARD, without xcb,scb
**	19-mar-1999 (nanpr01)
**	    Get rid of extents since we support one raw table per raw location.
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**      12-nov-1999 (stial01)
**          Do not add tidp to key for ANY unique secondary indexes
**	03-Jul-2000 (zhahu02)
**	    Modified the code for counting the maximum allowable keywidth
**	    m->mx_kwidth for btree index to include tidp contribution. (B102065)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_table_access() if neither
**	    C2 nor B1SECURE.
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	20-may-2001 (somsa01)
**	    Re-added crossed fix for bug 102065 properly.
**	22-Jan-2004 (jenjo02)
**	    Added initial support for Global indexes, TID8s.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          idx_key now defined as DM_ATT.
**      14-apr-2004 (stial01)
**	    Increase btree key size, varies by pagetype, pagesize (SIR 108315)
**	15-apr-2004 (somsa01)
**	    Ensure proper alignment of mx_tpcb_ptrs to avoid 64-bit BUS
**	    errors.
**      11-may-2004 (stial01)
**          Removed unnecessary svcb_maxbtreekey
**	16-Aug-2004 (schka24)
**	    Some mx/mct usage cleanups.
**      01-mar-2005 (stial01)
**          Fixed calculation of key width when key is compressed (b113992)
**	11-Mar-2005 (thaju02)
**	    Use $online idx relation info. (B114069)
**	16-May-2005 (thaju02)
**	    dm2u_index: in building index attribute list, check that 
**	    attribute has not been previously dropped. (B114519)
**	15-Aug-2006 (jonj)
**	    Support Indexes on Session Temp Tables.
**     20-oct-2006 (stial01)
**         Disable clustered index change to max_leaflen in ingres2006r2.
**	25-Feb-2008 (kschendel) SIR 122739
**	    Changes to set up the new row-access scheme for indexes.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Fix outrageous name confusion re acount vs kcount.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	26-Mar-2010 (toumi01) SIR 122403
**	    Add new width fields for encryption.
**	    Special rules for encryped indices.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      22-apr-2010 (stial01)
**          Fix the for loop condition for the attnmstr initialization
**	13-may-2010 (miket) SIR 122403
**	    Fix net-change logic for width for ALTER TABLE.
**      14-May-2010 (stial01)
**          Alloc/maintain exact size of column names (iirelation.relattnametot)
**      19-Jul-2010 (thich01)
**          Some rtree related fixes around coupon setup and data fill.
**	20-Jul-2010 (kschendel) SIR 124104
**	    dm2u-create wants compression now.
**/

/*{
** Name: dm2u_index - Create an index on a table.
**
** Description:
**      This routine is called from dmu_index() to create an index on a table.
**
** Inputs:
**	dcb				The database id.
**	xcb				The transaction id.
**	index_name			Name of the index to be created.
**	location			Location where the index will reside.
**      l_count                         Count of number of locations.
**	tbl_id				The internal table id of the base table
**					to be indexed.
**	structure			Structure of the index table.
**	i_fill				Fill factor of the index page.
**	l_fill				Fill factor of the leaf page.
**	d_fill				Fill factor of the data page.
**	compressed			TRUE if compressed records.
**	index_compressed		Specifies whether index is compressed.
**	min_pages			Minimum number of buckets for HASH.
**	max_pages			Maximum number of buckets for HASH.
**	kcount				Count of key attributes in the index.
**	acount				Count of attributes in the index.
**	key				Pointer to key array for indexing.
**	db_lockmode			Lockmode of the database for the caller.
**	allocation			Space pre-allocation amount.
**	extend				Out of space extend amount.
**	page_size			Page size for this index.
**	index_flags			Index options:
**					    DM2U_GATEWAY  - gateway index.
**	gwattr_array			Gateway attributes - valid only for
**					a gateway index.
**	gwchar_array			Gateway characteristics - valid only for
**					a gateway index.
**	gwsource			The source for the gateway object.  This
**					value is passed down to be interpreted
**					by the individual gateway.
**	reltups				This field is used only when modifying
**					the magic table 'iiridxtemp' as part of
**					a sysmod operation. In that case, it
**					contains the number of tuples in
**					iirelation and is passed on to
**					dm2u_load_table for use in bucket
**					calculation. Otherwise, it is zero.
**	char_array			Index characteristics - used only for
**					a gateway index.
**	relstat2			Flags to go into relstat2.
**	tbl_pri				Table cache priority.
**
** Outputs:
**	tup_info			Number of tuples in the indexed table
**					or the bad key length if key length
**					is too wide.
**	idx_id				The internal table id assigned to the
**					index table.
**      errcb.err_code			The reason for an error return status.
**	errcb.err_data			Additional information.
**					E_DM0007_BAD_ATTR_NAME
**					E_DM001C_BAD_KEY_SEQUENCE
**					E_DM001D_BAD_LOCATION_NAME
**					    err_data specifies location
**					E_DM005A_CANT_MOD_CORE_STRUCT
**					E_DM005D_TABLE_ACCESS_CONFLICT
**					E_DM005E_CANT_UPDATE_SYSCAT
**					E_DM005F_CANT_INDEX_CORE_SYSCAT
**					E_DM0066_BAD_KEY_TYPE
**					E_DM007D_BTREE_BAD_KEY_LENGTH
**					    err_data gives bad length value
**					E_DM009F_ILLEGAL_OPERATION
**					E_DM00D1_BAD_SYSCAT_MOD
**					E_DM0103_TUPLE_TOO_WIDE
**					E_DM010F_ISAM_BAD_KEY_LENGTH
**					    err_data gives bad length value
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
**          Created from dmu_index().
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	08-Nov-88 (teg)
**	    changed error parameter to dm2u_create() cuz call I/F changed.
**	29-nov-88 (rogerk)
**	    When find error condition, set 'error' before breaking, not
**	    '*err_code', as err_code will be set to 'error' value at bottom
**	    of routine.
**	29-nov-88 (rogerk)
**	    Changed err_code parameter to be an errcb.  Pass back err_data
**	    describing which location was bad when E_DM001D_BAD_LOCATION_NAME
**	    is returned.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-feb-89 (EdHsu)
**	    Add call to dm2u_ckp_lock to be mutually exclusive with ckp.
**	14-apr-89 (sandyh)
**	    Added checking to see if journaling has been set on base table to
**	    indexed, but not yet enabled. This allows future DML actions on
**	    the index to be handled correctly (bug 5397).
**      07-may-89 (jennifer)
**          Added B1 security code to dm2u_index to check if the
**          user specified the ii_sec_label or ii_sec_tabkey fields
**          as part of secondary index key.  If so, they are not added
**          otherwise they are added as part of index, but not part of key.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Accept DMU_GATEWAY type table.
**	    Added support for creating index that is not managed by the local
**	    system.  Added gateway characteristic parameters.  Pass these down
**	    to dm2u_create so they can be passed to the gateway.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of index operation.
**	06-nov-89 (fred)
**	    Added support for non-keyable datatypes as index keys.  More
**	    correctly, added code to detect and reject this usage.  This should
**	    be checked for in PSF, but we will double check here for safety's
**	    sake.
**	23-apr-90 (linda)
**	    Added parameter gwsource for the gateway object source.  This is
**	    passed down to be interpreted by the specific gateway.
**	26-apr-90 (linda)
**	    Add parameter to dm2u_index() call for qry_id -- this field was
**	    originally just for views and was used to define the join criterion
**	    for the "iiviews" standard catalog.  For gateways, we use a similar
**	    catalog "iiregistrations", for either tables or indexes.
**	30-apr-90 (bryanp)
**	    Fixed a memory smashing bug in secondary hash index creation: if
**	    the 2-ary index key includes ALL base table attributes, then after
**	    we append the TIDP and the hash value, the resulting intermediate
**	    index tuple is of size base-table-size + sizeof(DM_TID) +
**	    sizeof(hash-value). Changed dm2u_index to allocate that much
**	    space.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**	     7-mar-1991 (bryanp)
**		New argument 'index_compressed' to support Btree index
**		compression.  If index compression is used, add worst-case
**		compression overhead to key width when checking to see if key
**		is too large. Uses new tcb_comp_katts_count field in tcb to do
**		this.  Set up mx_data_atts in the DM2U_MXCB correctly.
**	    12-apr-1991 (bryanp)
**		Btree index compression bugfix: mx_comp_katts_count in
**		dm2u_index
**	    16-jul-1991 (bryanp)
**		B38527: new reltups arg to dm2u_index, dm0l_modify, dm0l_index.
**	06-feb-1992 (ananth)
**	    Use the ADF_CB in the RCB instead of allocating in on the stack.
**	14-apr-1992 (bryanp)
**	    Set mx_inmemory and mx_buildrcb fields in the MXCB.
**	26-may-1992 (schang)
**          GW merge. add gw_id as input arg and pass it to dm2u_create()
**	16-jul-1992 (kwatts)
**	    MPF change. Use accessor to calculate possible expansion on
**	    compression (comp_katts_count).
**	27-aug-92 (rickh)
**	    FIPS CONSTRAINTS:  new relstat2 field in IIRELATION tuple.
**	29-August-1992 (rmuth)
**	    Use constants for the default allocation and extend size.
**	3-oct-1992 (bryanp)
**	    New characteristics array argument passed to dm2u_create for
**	    gateway use.
**	02-oct-1992 (robf)
**	    New parameter passed in for table char_array
**	16-oct-1992 (rickh)
**	    Added initialization of default values.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	02-nov-1992 (jnash)
**	    Reduced Logging Project: Fill in loc_config_id when building
**	    mxcb, reflecting new dm2t protocols.  New params to dm0l_dmu.
**	18-nov-1992 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**      10-dec-1992 (schang)
**          For RMS gateway, do not fill in the index row number, and return
**          zero.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location field since it's no longer
**	    used.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**	25-feb-1993 (rickh)
**	    Make sure to set relstat2.
**	30-feb-1993 (rmuth)
**	    When calling dm2u_create to create the table which is turned
**	    into the index we currently pass down the index's allocation
**	    and extend sizes. This is bad as if the user has specified an
**	    allocation of 1000 we would first create the heap table with
**	    1000 pages and then create the index with 1000 pages and then
**	    delete the former. To aliveate the problem change the code to
**	    pass the default allocation and extend sizes, hence a table
**	    of 4 pages will be created and destroyed. Ideally we should
**	    build the index over the heap table.
**	8-apr-93 (rickh)
**	    Initialize relstat2 for dm2u_update_catalogs call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	05-apr-1993 (ralph)
**	    DELIM_IDENT: Use appropriate case for "tidp"
**	15-may-1993 (rmuth)
**	    Concurrent-Index: Add support for an index to be built
**	    by only taking share locks out on the base table.
**	22-may-93 (andre)
**	    upcase (a bool) was being assigned a result of &'ing a u_i4
**	    field and long constant.  On machines where bool is char, this
**	    assignment will not produce the expected result
**	24-may-1993 (rogerk)
**	    Pass index_compression attribute in the dm0l_index log record.
**	    Previously we were passing only the data-compression attribute,
**	    and ignore index_compression in the log record.  This caused
**	    rollforward to build compressed btree indexes incorrectly.
**	28-may-1993 (robf)
**	    Secure 2.0: Reworked ORANGE code
**	21-June-1993 (rmuth)
** 	    File Extend: Make sure that for multi-location tables that
**	    allocation and extend size are factors of DI_EXTENT_SIZE.
**	21-jun-1993 (jnash)
**	    Force table pages prior to writing index log records.  This
**	    is required because the index log record flags a "non-redo"
**	    log address for recovery.
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the index process for new recovery protocols.
**	      - We now log/create the table files before doing any other
**		work on behalf of the index operation.  The creation of
**		the physical files was taken out of the dm2u_load_table
**		routine which now just allocates and formats space.
**	      - The dm2u_load_table call is now made to build the structured
**		table BEFORE logging the dm0l_index log record.  This is
**		done so that any non-executable index attempt (one with an
**		impossible allocation for example) will not be attempted
**		during rollforward recovery.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.  Changed DMU to
**	    to be a journaled log record.
**	  - Added a statement count field to the modify and index log records.
**	  - Added gateway status to the logged index relstat since index
**	    of gateway tables is now journaled.
**	23-aug-1993 (rmuth)
**	    Always open the base table in DM2T_A_READ mode.
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	13-oct-93 (robf)
**          Removed sec_attr_count (again), last integration put it back.
**	18-oct-1993 (rogerk)
**	  - Moved call to dm2u_calc_hash_buckets up from dm2u_load_table so
**	    that the bucket number could be logged in the dm0l_index log
**	    record.  This allows rollforward to run deterministically.
**	  - Pass bucket count in dm0l_index call.
**	  - Since the reltups count is no longer required for recovery
**	    processing, it is now always logged as the number of rows in the
**	    result table instead of just for sconcur tables.  The log record
**	    value is used for informational purposes only.
**	2-mar-94  (robf)
**          Update MAC check requirements to allow for security privilege
**	    on system catalogs
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them
**	    prior to close.
**	11-apr-1994 (rmuth)
**	    Remove some old redundent code.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**	03-may-1995 (morayf)
**	    De-ANSIfied dm2u_index() declaration for odt_us5 as SCO C compiler
**	    can currently only handle up to 31 function prototype arguments.
**	    Thus unfortunately 2 sets of argument lists should be maintained.
**	10-feb-1995 (cohmi01)
**	    For RAW I/O, pass DB_RAWEXT_NAME parm to dm2u_create.
**          Add DB_RAWEXT_NAME parm to dm2u_index.
**	05-may-1995 (shero03)
**          For RTREE, add dimension, range, and hilbertsize
**	    Note that the RTRee key is : NBR, HILBERT, TIDP
**	07-may-1995 (wadag01)
**	    Aligned separately maintained parameter list for SCO's (odt_us5)
**	    version of dm2_index(): added DB_RAWEXT_NAME.
**	06-mar-1996 (stial01 for bryanp)
**	    Accept new page_size argument and use it to pass to dm2u_create.
**	    Also, pass page_size argument to dm2u_file_create.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**          Pass page size to dm2f_build_fcb()
**      06-may-1996 (nanpr01)
**          Changed parameters to dm2u_maxreclen.
**          Modified the isam key length calculation. We never intended to
**	    support the increased key size for this project. So we use
**	    DM_COMPAT_PGSIZE parameter for key length checking.
**	 3-jun-96 (nick)
**	    Ignore relstat of TCB_JON on base table ( this was probably missed
**	    in the change 20-sep-93 above ). #76741
**	17-sep-96 (nick)
**	    Remove compiler warning.
**      27-jan-1997 (stial01)
**          Do not include TIDP in the key of unique secondary indexes
**      31-jan-1997 (stial01)
**          Commented code for 27-jan-1997 change
**      10-mar-1997 (stial01)
**          Alloc/init mx_crecord, area to be used for compression.
**      28-apr-1997 (stial01)
**          dm2u_index() Init mx_atts_ptr[i+1] from tcb_atts_ptr[j] so that 
**          new fields added for alter table get initialized.
**      21-may-1997 (stial01)
**          dm2u_index() Always include tidp in "key length" for BTREE.
**          dm2u_index() Init new fields added for alter table in attribute
**          info for tidp.
**          For compatibility reasons, DO include TIDP for V1-2k unique indices
**	28-Jul-1997 (shero03)
**	    Allow an Rtree index to be build on a long spatial data type
**      23-sep-1997 (shero03)
**          B85561: add comp/expand overhead to mx_crecord size
**	02-Oct-1997 (shero03)
**	    Changed the code to use the mx_log_id in lieu of the xcb_log_id
**	    because there is no xcb during recovery.
**	16-Oct-1997 (shero03)
**	    B85456: don't journal a new index if the base table isn't 
**	    journaling until the next check point.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**      23-nov-1998 (stial01)
**          Moved check for bad keylength back after addition of TIDP to keylist
**	23-Dec-1998 (jenjo02)
**	    TCB->RCB list must be mutexed while it being read.
**      15-mar-1999 (stial01)
**          dm2u_index() can get called if DCB_S_ROLLFORWARD, without xcb,scb
**      18-mar-1999 (stial01)
**          Removed duplicate initialization of setrelstat,setrelstat2
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          1) Ensure we log whether the index is unique or not. Failure
**          to do so will cause a unique index table to be created as
**          a non-unique index table. This will lead to problems reloading
**          the index as the number of entries per page is different.
**          2) Pickup the dmveredo flag value to determine whether we
**          are recreating the index during recovery or not.
**       7-Jun-2004 (hanal04) Bug 112264 INGSRV2813
**          Correct dm0m_allocate() size for mxcb_cb and associated
**          fields. Investigation of the above bug showed PAD over-run
**          when PAD diagnostics were enabled in dm0m.c
**	6-Feb-2004 (schka24)
**	    Revise temp name generation scheme, eliminate dmu count.
**	    Update call to dm2u_create.
**	28-Feb-2004 (jenjo02)
**	    Changes for Global indexes, multiple sources, single
**	    target.
**	04-Mar-2004 (jenjo02)
**	    Allocate tpcb_srecord buffer. When threading targets,
**	    source record is copied here.
**	08-Mar-2004 (jenjo02)
**	    Deleted tpcb_olocations, oloc_count, added
**	    spcb_locations, spcb_loc_count.
**	10-Mar-2004 (jenjo02)
**	    Init new mx_Mlocation fields in MXCB.
**	11-Mar-2004 (jenjo02)
**	    Added TPCB pointer array to mx_tpcb_ptrs.
**	17-Mar-2004 (schka24)
**	    rtree thing overlaid locations, fix.
**	09-Apr-2004 (jenjo02)
**	    Ensure correct alignment of SPCB/TPCB arrays
**	    to avoid 64-bit BUS errors.
**	20-Apr-2004 (schka24)
**	    Include dm2u record header in record buffers.
**	    Fix hash global indexes, they built but the data was buggered up.
**	19-May-2004 (schka24)
**	    Btree key length check added tid twice (it's already included
**	    in mx_kwidth), fix.
**	22-jun-2004 (thaju02)
**	    Make key_map[] big enough to handle max 1024 columns. (B112539)
**	08-jul-2004 (thaju02)
**	    Online Modify - after sort/load, save fhdr pageno. For
**	    online_index_build (rename/swapping files), use saved fhdr
**	    pageno to update catalogs. (B112610)
**	6-Aug-2004 (schka24)
**	    dma_table_access is now a no-op if not asking for success
**	    auditing, remove call.
**	25-aug-2004 (thaju02)
**	    Rolldb of online modify with persistent index fails.
**	    If online_index_build, do not log DM0LINDEX. (B112887)
**	16-dec-04 (inkdo01)
**	    Init. various instances of collation IDs.
**	18-dec-2004 (thaju02)
**	    Removed reference to mx_input_rcb.
**	    Replaced m->mx_online_tabid with tp->tpcb_dupchk_tabid. 
**	11-Mar-2005 (thaju02)
**	    Use $online idx relation info. (B114069)
**	16-May-2005 (thaju02)
**	    In building index attribute list, check that attribute
**	    has not been previously dropped. (B114519)
**	21-Jul-2005 (jenjo02)
**	    Init (new) mx_phypart pointer to NULL.
**	13-Apr-2006 (jenjo02)
**	    Prototype change to dm1b_kperpage() for CLUSTERED
**	    primary Btree.
**	    Remove DM1B_MAXKEY_MACRO throttle from leaf entry size check.
**	    Add temporary prohibition of indexes on Clustered tables.
**      30-jun-2006 (stial01)
**          Fix buckets parameter to dm0l_index.
**	15-Aug-2006 (jonj)
**	    Support Indexes on Temp Tables.
**	05-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      23-Apr-2007 (hanal04) Bug 118099
**          Do not take the table control lock here. Allow the phys table
**          lock to be acquired first and the control lock for the base
**          will subsequently be acquired in dm2u_create().
**          This reduces deadlock opportunities during index creation
**          caused by the use of control locks that are not held for
**          the duration of a transaction (bad locking semantics).
**	26-Feb-2008 (kschendel) SIR 122739
**	    Extract common mxcb setup code;  fix badly broken indentation.
**     31-oct-2008 (huazh01 for horda03)
**          remove the unnecessary E_DM005D check. (bug 121112)
**	6-Nov-2009 (kschendel) SIR 122757
**	    Ask for direct-io if config suggests it.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Update call to dm2t-destroy-temp-tcb.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Use no-coupon access modes.
**	10-may-2010 (stephenb)
**	    Cast new i8 reltups back to i4
**	16-Jul-2010 (kschendel) SIR 123450
**	    Really pass the compression type(s) in the log record.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Replace dmu char array with dmu characteristics.
*/
DB_STATUS
dm2u_index(
DM2U_INDEX_CB   *index_cb)
{
    DMP_DCB	    *dcb;
    DML_XCB	    *xcb;
    DML_SCB         *scb;
    DB_OWN_NAME	    owner;
    DM2U_MXCB	    *m = (DM2U_MXCB*)NULL;
    DM2U_MXCB	    *mxcb_cb = (DM2U_MXCB *)NULL;
    DMP_RCB	    *r = (DMP_RCB *)NULL;
    DMP_TCB	    *t;
    i4	     	    i;
    i4	     	    NumCreAtts;
    DB_TAB_TIMESTAMP timestamp;
    DMF_ATTR_ENTRY  att_entry[DB_MAXIXATTS + 1];
    DMF_ATTR_ENTRY  *att_list[DB_MAXIXATTS + 1];
    i4	     	    local_error;
    DB_STATUS	    status;
    i4              ref_count;
    DB_TAB_NAME	    table_name;
    DB_LOC_NAME	    tbl_location;
    i4	     	    journal;
    i4	     	    journal_flag;
    i4              log_relstat;
    i4              setrelstat;
    u_i4            setrelstat2 = 0;
    i4              tbl_lk_mode;
    i4	     	    tbl_access_mode = DM2T_A_READ_NOCPN;
    i4              loc_count;
    i4              timeout = 0;
    u_i4	    db_sync_flag;
    LG_LSN	    lsn;
    bool            syscat;
    bool            sconcur;
    bool            concurrent_index  = FALSE;
    bool            modfile_open = FALSE;
    i4              gateway = 0;
    i2		    name_id;
    i4              name_gen;
    DB_TRAN_ID      *tran_id;
    i4              log_id;
    i4              lk_id;
    i4		    TableLockList;
    i4		    sources;
    i4		    targets;
    DM2U_TPCB	    *tp = NULL;
    DM2U_M_CONTEXT  *mct;
    i4		     online_index_build;
    i4		    TempTable;
    DB_ERROR	    *dberr;
    DB_ERROR	    local_dberr;


    dcb = index_cb->indxcb_dcb;
    xcb = index_cb->indxcb_xcb;

    /* Note this points to a DMU_CB->error */
    dberr = index_cb->indxcb_errcb;

    CLRDBERR(dberr);

    *index_cb->indxcb_tup_info = 0;
    gateway = ((index_cb->indxcb_index_flags & DM2U_GATEWAY) != 0);
    online_index_build = ((index_cb->indxcb_index_flags & DM2U_ONLINE_END) != 0);

    /* Creating an Index on a Temp Table? */
    TempTable = (index_cb->indxcb_tbl_id->db_tab_base < 0);

    if ( !(dcb->dcb_status & DCB_S_ROLLFORWARD) && !TempTable )
    {
	status = dm2u_ckp_lock(dcb, 
			       index_cb->indxcb_index_name, (DB_OWN_NAME *)NULL,
			       xcb, 
			       dberr);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    dmf_svcb->svcb_stat.utl_index++;

    /*
    ** If a concurrent-index build then
    **    - Do not take out table control lock
    **    - Share lock the base table
    */
    concurrent_index = 
	((index_cb->indxcb_relstat2 & TCB2_CONCURRENT_ACCESS) != 0);
    if ( TempTable )
	tbl_lk_mode = DM2T_X;
    else if ( concurrent_index )
    {
        if (index_cb->indxcb_tbl_id->db_tab_base <= DM_SCONCUR_MAX)
            tbl_lk_mode = DM2T_IS;
        else
            tbl_lk_mode = DM2T_S;
    }
    else
    {
        if (index_cb->indxcb_tbl_id->db_tab_base <= DM_SCONCUR_MAX)
            tbl_lk_mode = DM2T_IX;
        else
            tbl_lk_mode = DM2T_X;
    }

    for (i=0; i < DB_MAXIXATTS + 1; i++)
        att_list[i] = &att_entry[i];

    loc_count = index_cb->indxcb_l_count;

    if ( dcb->dcb_status & DCB_S_ROLLFORWARD )
    {
	scb = (DML_SCB *)0;
	log_id = index_cb->indxcb_rfp.rfp_log_id;
	lk_id = index_cb->indxcb_rfp.rfp_lk_id;
	TableLockList = lk_id;
	tran_id = &index_cb->indxcb_rfp.rfp_tran_id;
	name_id = index_cb->indxcb_rfp.rfp_name_id;
	name_gen = index_cb->indxcb_rfp.rfp_name_gen;
    }
    else
    {
	scb = xcb->xcb_scb_ptr;
	/* Get timeout from set lockmode */
	timeout = dm2t_get_timeout(scb, index_cb->indxcb_tbl_id); 
	log_id = xcb->xcb_log_id;
	/* TempTable table locks are on thread SCB lock list */
	if ( TempTable )
	    TableLockList = scb->scb_lock_list;
	else
	    TableLockList = xcb->xcb_lk_id;
	/* ...all other locks are on transaction list */
	lk_id = xcb->xcb_lk_id;
	tran_id = &xcb->xcb_tran_id;
	name_id = dmf_svcb->svcb_tfname_id;
	name_gen = CSadjust_counter((i4 *)&dmf_svcb->svcb_tfname_gen, 1);
	/* For convenience in passing this stuff to the make-mxcb
	** routine, stick a copy of log_id etc into the indexcb just like
	** it was rollforward.
	*/
	index_cb->indxcb_rfp.rfp_log_id = log_id;
	index_cb->indxcb_rfp.rfp_lk_id = lk_id;
	index_cb->indxcb_rfp.rfp_name_id = name_id;
	index_cb->indxcb_rfp.rfp_name_gen = name_gen;
    }

    do
    {
        /*  Open up the (master) table. */

        /*  schang : the 5th argument was set to DM2T_A_WRITE, which  */
        /*    causes failure in creating index on nonexisting RMS     */
        /*    file.  Setting it to DM2T_A_MODIFY makes it work.       */
        /*    we choose one or another based on the flag gateway      */


        /* 
         * Set this to a normal read if it's an rtree index so that coupons
         * are not ignored and therefore set up correctly. 
         * For spatial rtree indexes, all data are LOBs.  So all coupon setup
         * must be done if building an rtree.
         */
        if(index_cb->indxcb_structure == TCB_RTREE) 
            tbl_access_mode = DM2T_A_READ;
        status = dm2t_open(dcb, index_cb->indxcb_tbl_id, 
		 tbl_lk_mode, DM2T_UDIRECT, 
		 ((gateway)?DM2T_A_MODIFY:tbl_access_mode), timeout, 
		 (i4)20, (i4)0, log_id,
                 TableLockList, (i4)0, (i4)0,
                 index_cb->indxcb_db_lockmode, 
		 tran_id, 
		 &timestamp, &r, (DML_SCB *)0, dberr);
        if (status != E_DB_OK)
	    break;

        t = r->rcb_tcb_ptr;
        r->rcb_xcb_ptr = xcb;

	/* Check for duplicate TempTable Index name */
	if ( TempTable )
	{
	    DMP_TCB	*it;

	    for ( it = t->tcb_iq_next; 
	          it != (DMP_TCB*)&t->tcb_iq_next;
		  it = it->tcb_q_next )
	    {
		if ( (MEcmp((char*)index_cb->indxcb_index_name,
			 (char*)&it->tcb_rel.relid,
			 sizeof(DB_TAB_NAME)) == 0) )
		{
		    SETDBERR(dberr, 0, E_DM0078_TABLE_EXISTS);
		    break;
		}
	    }
	    if ( dberr->err_code )
	    {
		status = E_DB_ERROR;
		break;
	    }
	}

	/* Note for a TempTable this will be "$Sessnnnnnnnn" */
        MEcopy((char *)&t->tcb_rel.relowner, sizeof(DB_OWN_NAME),
               (char *)&owner);

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	** TempTables don't log either.
	*/
	if ( dcb->dcb_status & DCB_S_ROLLFORWARD ||
	     xcb->xcb_flags & XCB_NOLOGGING ||
	     TempTable )
	{
	    r->rcb_logging = 0;
	}

	ref_count = t->tcb_open_count;
	syscat = (t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)) != 0;
	sconcur = (t->tcb_rel.relstat & TCB_CONCUR);
	journal = ((t->tcb_rel.relstat & TCB_JOURNAL) != 0);
	journal_flag = journal ? DM0L_JOURNAL : 0;

	/* Figure out the number of input sources */
	if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED )
	    sources = t->tcb_rel.relnparts;
	else
	    sources = 1;

	/*
	** Until we support partitioned indexes, there'll be but one 
	** output target.
	*/
	targets = 1;

	/*
	** For system catalogs... index page size = base table page size
	** Page size is determined at the time of createdb.
	** SYSMOD rebuilds indexes.... the page size must be preserved
	*/
	if (syscat || sconcur)
	    index_cb->indxcb_page_size = t->tcb_rel.relpgsize;

	if ( concurrent_index )
	{
	    /*
	    ** Concurrent index not allowed on system catalogs or
	    ** non READONLY base tables.
	    ** Make sure base table is in READONLY mode
	    */
	    if ( syscat || !(t->tcb_rel.relstat2 & TCB2_READONLY))
	    {
		SETDBERR(dberr, 0, E_DM0069_CONCURRENT_IDX_ERR);
		status = E_DB_ERROR;
		break;
	    }
	}

	if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
	{
	    /*
	    ** Can't index an sconcur system catalog.
	    ** Can't index a non-sconcur system catalog
	    ** without update privilege.
	    */

	    if ( (sconcur != 0) ||
		 (syscat &&
		  ((scb->scb_s_type & SCB_S_SYSUPDATE) == 0)
		 )
	       )
	    {
		SETDBERR(dberr, 0, E_DM005F_CANT_INDEX_CORE_SYSCAT);
		status = E_DB_ERROR;
		break;

	    }
        }
        STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, table_name);
        STRUCT_ASSIGN_MACRO(t->tcb_rel.relloc, tbl_location);
        if (index_cb->indxcb_tab_name)
            *index_cb->indxcb_tab_name = t->tcb_rel.relid;
        if (index_cb->indxcb_tab_owner)
            *index_cb->indxcb_tab_owner = t->tcb_rel.relowner;

	setrelstat2 = index_cb->indxcb_relstat2;

	/* Create and partially set up the index MXCB. */

	status = dm2uMakeIndMxcb(&mxcb_cb, index_cb, r,
			timeout, tbl_lk_mode, sources, targets);
        if (status != E_DB_OK)
	    break;
	m = mxcb_cb;

	/* Set up a couple things inconvenient for make-mxcb. */
	m->mx_inmemory = TempTable;
	m->mx_tranid = tran_id;

	/*
	** Build the attributes.
	**
	** Returns with m->mx_ab_count, m->mx_ai_count, m->mx_c_count,
	** NumCreAtts corrected and all the various attribute lists
	** and bits built.
	*/
	if ( dm2uMakeIndAtts(index_cb, m, r, att_list, &NumCreAtts, 
			FALSE, NULL, dberr) )
	{
	    status = E_DB_ERROR;
	    break;
	}

        /* Now that we know we are really going to do the
        ** modify, set the rcb_uiptr to point to the scb_ui_state
        ** value so it can check for user interrupts.
        ** Don't do this if rollingforward, no session exists.
        */

        if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
            r->rcb_uiptr = &scb->scb_ui_state;

	/* The (only) target TPCB... */
	tp = m->mx_tpcb_next;
	mct = &tp->tpcb_mct;

        /* 
         * Setup the coupon structure. For spatial rtree indexes, all data are
         * LOBs.  So all coupon setup must be done if building an rtree.
         */
        if(index_cb->indxcb_structure == TCB_RTREE && 
           t->tcb_rel.relstat2 & TCB2_HAS_EXTENSIONS) 
            dmpe_find_or_create_bqcb(r, dberr);
	/*
	** All's ready now.
	**
	** Creating a Temp Table index is easier as it doesn't involve
	** logging or catalog updates, so we'll do it straight away.
	*/
	if ( TempTable )
	{
	    m->mx_buildrcb = (DMP_RCB*)NULL;

	    /* Make a TempIndex TCB, indxcb_idx_id */
	    status = dm2uCreateTempIndexTCB(t, m, index_cb,
					NumCreAtts, att_list,
					dberr);

	    /* ...then open it, getting mx_buildrcb */
	    if ( status == E_DB_OK )
	    {
		status = dm2t_open(dcb, 
			 	index_cb->indxcb_idx_id, 
			 	LK_X, DM2T_UDIRECT, 
			 	DM2T_A_WRITE_NOCPN,
			 	(i4)0,		/* timeout */
			 	(i4)20, 	/* maxlocks */
			 	(i4)0, 		/* savepoint id */
			 	log_id,
				TableLockList,
			 	(i4)0, 		/* sequence */
			 	(i4)0,		/* iso_level */
			 	index_cb->indxcb_db_lockmode, 
			 	&xcb->xcb_tran_id, 
			 	&timestamp, 
			 	&m->mx_buildrcb, 
			 	scb,
			 	dberr);
	    }

	    /* ...then load it up */
	    if ( status == E_DB_OK )
		status = dm2u_load_table(m, dberr);

	    /* Update the Index TCB iirelation */
	    if ( status == E_DB_OK )
	    {
		DMP_TCB		*IxTCB = m->mx_buildrcb->rcb_tcb_ptr;
		DMP_RELATION	*rel = &IxTCB->tcb_rel;

		rel->reltups = m->mx_newtup_cnt;
		rel->relpages = mct->mct_relpages;
		rel->relprim = mct->mct_prim;
		rel->relmain = mct->mct_main;
		rel->relfhdr = mct->mct_fhdr_pageno;
		rel->relifill = mct->mct_i_fill;
		rel->reldfill = mct->mct_d_fill;
		rel->rellfill = mct->mct_l_fill;

		*index_cb->indxcb_tup_info = rel->reltups;
	    }

	    /* Close the Index TCB */
	    if ( m->mx_buildrcb )
	    {
		(VOID)dm2t_close(m->mx_buildrcb, DM2T_NOPURGE, &local_dberr);

		/* If build failed, destroy the Temp Index */
		if ( status )
		    (VOID)dm2t_destroy_temp_tcb(TableLockList,
						dcb,
						&m->mx_table_id,
						&local_dberr);

	    }
	    /* Close the Base Temp Table from which it was built */
            (VOID)dm2t_close(r, DM2T_NOPURGE, &local_dberr);

	    /* ...and we're done */
	    dm0m_deallocate((DM_OBJECT **)&mxcb_cb);
	    return(status);
	}

        /*  Create a table to contain the (permanent) index. */

        setrelstat = TCB_DUPLICATES;
        if (t->tcb_rel.relstat & TCB_JOURNAL) /* 85456 remove | TCB_JON */
            setrelstat |= TCB_JOURNAL;
        if (t->tcb_rel.relstat & TCB_CATALOG)
            setrelstat |= TCB_CATALOG;
        if (t->tcb_rel.relstat & TCB_EXTCATALOG)
            setrelstat |= TCB_EXTCATALOG;
        if (gateway)
	{
            setrelstat |= TCB_GATEWAY;
	    if (t->tcb_rel.relstat & TCB_GW_NOUPDT)
		setrelstat |= TCB_GW_NOUPDT;
	}

        /* schang : GW merge.  Replace (i4)0 with gw_id */
	if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0)
	{
	    status = dm2u_create(dcb, xcb, 
	      index_cb->indxcb_index_name, &owner, index_cb->indxcb_location, 
	      loc_count, index_cb->indxcb_tbl_id, index_cb->indxcb_idx_id, 
	      (i4)1, (i4)0, setrelstat, setrelstat2,
	      m->mx_structure, TCB_C_NONE, m->mx_width, m->mx_width, NumCreAtts,
	      att_list, index_cb->indxcb_db_lockmode,
	      DM_TBL_DEFAULT_ALLOCATION, DM_TBL_DEFAULT_EXTEND, 
	      m->mx_page_type, m->mx_page_size, index_cb->indxcb_qry_id, 
	      index_cb->indxcb_gwattr_array, index_cb->indxcb_gwchar_array,
	      index_cb->indxcb_gw_id, (i4)0,
	      index_cb->indxcb_gwsource, index_cb->indxcb_dmu_chars,
	      m->mx_dimension, m->mx_hilbertsize,
	      m->mx_range, m->mx_tbl_pri, NULL, 0,
	      NULL /* DMU_CB */, &local_dberr);
	    if (status != E_DB_OK)
	    {
	        /*
	        ** If the base table is journaled, but journaling has been turned
	        ** off on this database, then create will return with a warning
	        ** that journaling will be enabled at the next checkpoint
	        ** (E_DM0101_SET_JOURNAL_ON).  We should ignore this warning.
	        */
	        if (local_dberr.err_code != E_DM0101_SET_JOURNAL_ON)
	        {
		    *dberr = local_dberr;
		    break;
	        }
	        status = E_DB_OK;
	    }

	    /*
	    ** Force any pages left in buffer cache for this table.
	    ** This is required for recovery as a non-redo log record immediately
	    ** follows.  We must use DM0P_CLFORCE here because the table
	    ** may be in use if concurrent builds are taking place.
	    */
	    status = dm0p_close_page(t, lk_id, log_id, DM0P_CLFORCE, dberr);
	    if (status != E_DB_OK)
		break;
	}

	STRUCT_ASSIGN_MACRO(*index_cb->indxcb_idx_id, m->mx_table_id);

	/* Copy tabid to (only) target's TPCB */
	STRUCT_ASSIGN_MACRO(*index_cb->indxcb_idx_id, tp->tpcb_tabid);



        /*
        **  Load the index.
        **
	**  At this point the following MXCB fields should have the following
	**  values based on the type of index that was created.
	**
	**      HASH indices never include TIDP as a key.
	**      Unique indices never include TIDP as a key
	**
	**      Non-unique BTREE and ISAM indices include TIDP as a key field.
	**
	**  Data fields are supported for all index types.
        **
        **  mx_b_key_atts  - One pointer for each base table attribute named.
        **  mx_ab_count    - Count of above entries.
        **  mx_i_key_atts  - One pointer for each key for this index record plus
        **         	     one for the implied TIDP key on non-HASH indices.
        **         	     The key_offset field must be correctly set relative
        **         	     to beginning of the index record.
        **  mx_ai_count    - Count of above entries.
        **  mx_kwidth      - Sum of index key attributes plus TIDP is non-HASH
        **         	     indices.  Non-key (data) fields of a btree index
        **         	     are also included.
        **  mx_data_atts   - One pointer for each attribute in the table or
        **         	     index being modified. Used for data compression.
        **  mx_width       - Total width of index record, includes TIDP.
        **  mx_idom_map    - Map of index attributes to base table attributes
        **                   for key and data fields in an index excepting TIDP.
        **  mx_att_map     - The key number for a attribute number.
        **                   For non-HASH tables this would be set for the TIDP field.
        **  mx_unique      - If a unique index is being created,
        **                   this field should set so that the load code checks
        **                   for non-unique keys.
        **
        ** If this is a gateway index, then don't load any data - the real
        ** index is in the gateway and is managed by the gateway database.
        */
        if (! gateway)
        {
            /*
            ** Select file synchronization mode.
            */
            if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
                db_sync_flag = 0;
	    else
                db_sync_flag = DI_USER_SYNC_MASK;
	    if (dmf_svcb->svcb_directio_load)
		db_sync_flag |= DI_DIRECTIO_MASK;
	    if (!online_index_build)
		db_sync_flag |= DI_PRIVATE_MASK;

	    /*
	    ** Allocate fcb for each location.
	    ** Tell dm2f how to build the temp filename, because if this is
	    ** rollforward, it had better match what was fcreated last time.
	    ** (DO has exclusive control of the database so there are no
	    ** filename collision concerns.)
	    */
	    status = dm2f_build_fcb(lk_id, log_id,
			tp->tpcb_locations, tp->tpcb_loc_count,
			m->mx_page_size,
			DM2F_ALLOC, DM2F_MOD, &m->mx_table_id,
			name_id, name_gen,
                        (DMP_TABLE_IO *)0, 0, dberr);
	    if (status != E_DB_OK)
		break;

	    if ((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0 && !online_index_build)
	    {
		/*
		** Log and create the file(s).
		*/
		status = dm2u_file_create(dcb, log_id, lk_id,
			&m->mx_table_id, db_sync_flag,
			tp->tpcb_locations,
			tp->tpcb_loc_count,
			m->mx_page_size,
			((xcb->xcb_flags & XCB_NOLOGGING) == 0),
			journal, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    else
	    {
		/*
		** ROLLFORWARD or online index build
		** file exists, just open it 
		*/
		if (online_index_build)
		{
		    i4		k = 0;
		    i4		nofiles = 0;
		    DML_XCCB 	*xccb;
		    bool  	found_xccb;

		    /* Need to do those file renames first.
		    ** This is really a for-loop on k...
		    */
		    while ( !nofiles)
		    {
			DM_FILE		mod_filename; /* for modtable */

			dm2f_filename(DM2F_MOD, &m->mx_table_id,
			    tp->tpcb_locations[k].loc_id, name_id, name_gen,
			    &mod_filename);

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
				    &index_cb->indxcb_online_tabid, 
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
			** RENAME $online_tabid_ix_indexnum.DM2F_DES 
			** TO     modtable.DM2F_MOD
			** Remove the pending XCCB_F_DELETE XCCB 
			*/

			if (DMZ_SRT_MACRO(13))
			{
			    TRdisplay("ONLINE MODIFY rename %~t %~t \n",
				xccb->xccb_f_len, &xccb->xccb_f_name, 
				sizeof(mod_filename), &mod_filename);
			}

			if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
			{
			    status = dm0l_frename(m->mx_log_id, journal_flag,
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
			    &dcb->dcb_name, dberr);
			if (status != E_DB_OK)
			    break;

			/*  Remove from queue. */
			xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
			xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;

			/*  Deallocate. */ 
			dm0m_deallocate((DM_OBJECT **)&xccb);
		    
			k++;
			if (k >= tp->tpcb_loc_count)
			    break;
		    } /* while */
		    /* we want dm2u_load_table to initialize mct block only;
		    ** do not sort */
		    m->mx_flags |= MX_ONLINE_INDEX_BUILD;
		} /* if online */

		status = dm2f_open_file(lk_id, log_id,
			    tp->tpcb_locations,
			    tp->tpcb_loc_count,
			    m->mx_page_size,  DM2F_A_WRITE, db_sync_flag, 
			    (DM_OBJECT *)dcb, dberr);
		if (status != E_DB_OK)
		    break;
	    }
	  
            modfile_open = TRUE;

	    if (index_cb->indxcb_index_flags & DM2U_ONLINE_MODIFY)
	    {
		m->mx_flags |= MX_ONLINE_MODIFY;
		tp->tpcb_dupchk_tabid = index_cb->indxcb_dupchk_tabid;
	    }

	    /*
	    ** Load the index.
	    */
	    status = dm2u_load_table(m, dberr);
	    if (status != E_DB_OK)
		break;

	    if (online_index_build)
	    {

		/* update index relation from $online relation info */
		m->mx_newtup_cnt = index_cb->indxcb_on_reltups;
		tp->tpcb_newtup_cnt = index_cb->indxcb_on_reltups;
		mct->mct_fhdr_pageno = index_cb->indxcb_on_relfhdr;
		mct->mct_relpages = index_cb->indxcb_on_relpages;
		mct->mct_prim = index_cb->indxcb_on_relprim;
		mct->mct_main = index_cb->indxcb_on_relmain;
	    }

	    *index_cb->indxcb_tup_info = m->mx_newtup_cnt;

	    /*
	    ** Opening file DI_USER_SYNC_MASK requires force prior to close.
	    */
	    if (db_sync_flag & DI_USER_SYNC_MASK)
	    {
		status = dm2f_force_file(tp->tpcb_locations,
				tp->tpcb_loc_count,
				index_cb->indxcb_tab_name, 
				&dcb->dcb_name, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /*
	    ** Close the index file(s) just loaded in preparation for the
	    ** renames below (in dm2u_update_catalogs).
	    */
	    modfile_open = FALSE;
	    status = dm2f_close_file(tp->tpcb_locations,
			tp->tpcb_loc_count,
			(DM_OBJECT*)dcb, dberr);
	    if (status != E_DB_OK)
		break;
	}
        else
        {
	    /* Gateway */

	    DMP_TCB *btcb= r->rcb_tcb_ptr;
	    DMP_TCB *bbase_tcb= btcb->tcb_parent_tcb_ptr;
	    i4       tups_per_page;
	    /*
	    ** If this is a gateway, then we did not load the index.
	    ** Unfortunately, the dm2u_update_catalogs call below (that
	    ** updates the iirelation, attribute info to show the the fields
	    ** on which the index is keyed) assumes that certain information
	    ** will have been filled in by the dm2u_load_table call.
	    **
	    ** Although the gateway "knows" about the size of the index,
	    ** we try to fill in what we can for the optimizer,
	    ** - Number of tuples should the same as the main table
	    **   (rather than a random value)
	    ** - Guestimate the relpages/prim/main values by a calculation
	    **   from the number of tups and size of tuple.
	    */
	    /*
	    ** schang: RMS GW does not behave if m->mx_newtup_cnt is filled
	    **  Until the real fix is found, we leave it undefined.
	    */

	    switch (bbase_tcb->tcb_rel.relgwid)
	    {
              case GW_RMS:
		tp->tpcb_newtup_cnt = 0;
		*index_cb->indxcb_tup_info = 0;
		break;

              default:
		tp->tpcb_newtup_cnt = (i4)bbase_tcb->tcb_rel.reltups;
		*index_cb->indxcb_tup_info = tp->tpcb_newtup_cnt;
		break;
	    } /* end switch */

	    m->mx_newtup_cnt = tp->tpcb_newtup_cnt;

	    tups_per_page =
		dm2u_maxreclen(m->mx_page_type, m->mx_page_size)/m->mx_width;
	    if (tups_per_page < 1)
		tups_per_page = 1;

	    mct->mct_relpages = 2+(tp->tpcb_newtup_cnt/tups_per_page);
	    mct->mct_prim = mct->mct_relpages;
	    mct->mct_main = mct->mct_relpages;
	    mct->mct_i_fill = index_cb->indxcb_i_fill;
	    mct->mct_l_fill = index_cb->indxcb_l_fill;
	    mct->mct_d_fill = index_cb->indxcb_d_fill;
	    mct->mct_keys = m->mx_ai_count;
        }

        /*
        ** If a concurrent index do not purge the table at close
        ** time. The new index will be made visible once the base
        ** table is modifed back to NOREADONLY.
        */
	m->mx_rcb = (DMP_RCB*)NULL;
	status = dm2t_close(r, 
			    (concurrent_index) ? DM2T_NOPURGE : DM2T_PURGE,
			    dberr);

        r = 0;
        if (status != E_DB_OK)
	    break;

	/* If rollforward, we are done */
	if (dcb->dcb_status & DCB_S_ROLLFORWARD)
	{
            status = dm2f_release_fcb(m->mx_lk_id, m->mx_log_id,
				tp->tpcb_locations,
				tp->tpcb_loc_count,
				DM2F_ALLOC, dberr);
            if (status != E_DB_OK)
		break;

	    dm0m_deallocate((DM_OBJECT **)&mxcb_cb);
	    return(E_DB_OK);
	}

        /*
        ** Log the index operation - unless logging is disabled.
        **
        ** The INDEX log record is written after the actual index
        ** operation above which builds the temporary index load files.
        ** Note that until now, the user table (and its metadata) has
        ** not been physically altered.  We now log the index before
        ** updating the catalogs and swapping the underlying files.
        */

        if ((xcb->xcb_flags & XCB_NOLOGGING) == 0 && !online_index_build)
        {
	    /*
	    ** Get relstat bits for index log record.  These will be used
	    ** at build time in ROLLFORWARD to determine the compression
	    ** state of the index rows.  Note that the UNIQUE index characteristic
	    ** is not logged here.  This causes us to bypass uniqueness checking
	    ** during the index build.
	    ** Bug 108330 - We must log TCB_UNIQUE to ensure the page is
	    ** recreated correctly. Index recovery routines now identify
	    ** themselves so that we can safely bypass uniqueness checking.
	    */
	    log_relstat = 0;
	    if (index_cb->indxcb_compressed != TCB_C_NONE)
		log_relstat |= TCB_COMPRESSED;
	    if (index_cb->indxcb_index_compressed)
		log_relstat |= TCB_INDEX_COMP;
	    if (index_cb->indxcb_unique)
		log_relstat |= TCB_UNIQUE;
	    if (gateway)
		log_relstat |= TCB_GATEWAY;
	    status = dm0l_index(log_id, journal_flag,
		index_cb->indxcb_tbl_id, index_cb->indxcb_idx_id, &table_name, 
		&owner, loc_count, index_cb->indxcb_location, 
		index_cb->indxcb_structure, log_relstat, 
		index_cb->indxcb_relstat2,
		index_cb->indxcb_min_pages, index_cb->indxcb_max_pages,
		index_cb->indxcb_i_fill, index_cb->indxcb_d_fill, 
		index_cb->indxcb_l_fill, m->mx_newtup_cnt, m->mx_buckets,
		index_cb->indxcb_idx_key, index_cb->indxcb_kcount,
		index_cb->indxcb_allocation, index_cb->indxcb_extend, 
		m->mx_page_type, m->mx_page_size, 
		index_cb->indxcb_compressed,
		index_cb->indxcb_index_compressed ? TCB_C_STD_OLD : TCB_C_NONE,
		name_id, name_gen,
		m->mx_dimension, m->mx_hilbertsize,(f8 *)&m->mx_range,  
		(LG_LSN*)0, &lsn, dberr);
            if (status != E_DB_OK)
		break;
        }

        /*  Change the system tables for the index operation. */
        status = dm2u_update_catalogs(m, journal, dberr);
        if (status != E_DB_OK)
	    break;

        /*
        ** Log the DMU operation.  This marks a spot in the log file to
        ** which we can only execute rollback recovery once.  If we now
        ** issue update statements against the newly-indexed table, we
        ** cannot do abort processing for those statements once we have
        ** begun backing out the index.
        */
        if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
        {
            status = dm0l_dmu(log_id, journal_flag, 
		     index_cb->indxcb_tbl_id,
                     &table_name, &owner, (i4)DM0LINDEX, (LG_LSN *)0, 
		     dberr);
            if (status != E_DB_OK)
		break;
        }

        /*
        ** Release file control blocks allocated for the index.  If this is
        ** a gateway index then there are really no files to release.
        */
        if (! gateway)
        {
            status = dm2f_release_fcb(lk_id, log_id,
				tp->tpcb_locations,
				tp->tpcb_loc_count,
				DM2F_ALLOC, dberr);
            if (status != E_DB_OK)
		break;
        }

	if (status == E_DB_OK)
	{
	    dm0m_deallocate((DM_OBJECT **)&mxcb_cb);
	    return(E_DB_OK);
	}
    } while (FALSE);

    /*
    ** Handle cleanup for error recovery.
    */

    /*
    ** If we left the newly created modify files open, then close them.
    ** We don't have to worry about deleting them as that is done by
    ** rollback processing.
    */
    if (modfile_open)
    {
        status = dm2f_close_file(tp->tpcb_locations,
			tp->tpcb_loc_count,
			(DM_OBJECT*)dcb, &local_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                 (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
        }
    }

    /*
    ** Release the FCB memory allocated by dm2f_build_fcb.
    */
    if (tp && tp->tpcb_locations && (tp->tpcb_locations[0].loc_fcb))
    {
        status = dm2f_release_fcb(lk_id, log_id,
		    tp->tpcb_locations,
		    tp->tpcb_loc_count, DM2F_ALLOC, &local_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                 (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
        }
    }

    /*
    ** Close the user table.
    */
    if (r)
    {
	if ( m )
	    m->mx_rcb = (DMP_RCB*)NULL;
        status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                 (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
        }
    }

    if ( mxcb_cb )
	dm0m_deallocate((DM_OBJECT **)&mxcb_cb);

    return(E_DB_ERROR);
}


/*
** Name: dm2uMakeIndMxcb - Make the MXCB for a create index.
**
** Description:
**	Given an index CB and a few other dribs and drabs of information,
**	allocate and fill in an MXCB that will be used for loading that
**	particular index.
**
**	It would be nice to consolidate MXCB setup for modify as well,
**	but modify setup is quite a bit more complicated since it has to
**	deal with old/new locations, partitions, etc.
**
** Inputs:
**	index_cb		The DM2U_INDEX_CB to get the info from.
**				Note that even for non-rollforward, we expect
**				to find the following things in the
**				indxcb_rfp:  log_id, lk_id, name_id,
**				and name_gen.  (Simply to keep the routine
**				parameter list reasonable.)
**	rcb			Base table RCB
**	timeout			Lock-timeout to use
**	tbl_lk_mod		Lock-mode to use on later base table opens,
**				if any
**	sources			Number of sources; this should be:
**				# of base table partitions (or 1) for an
**				ordinary create index;
**				zero for a parallel create index.
**				(because parallel does its own thing with
**				base table projections, and thus parallelizes
**				the result rather than the source.)
**	targets			Number of targets, should be 1 for now.
**				(partitioned indexes not supported.)
**	mxcb			An output
**
** Outputs:
**	mxcb			Returned pointer to allocated MXCB;
**				much of MXCB is filled in, TPCB's and SPCB's
**				allocated.  Att stuff is not done, caller
**				needs to follow up with make-ind-atts.
**
**	Returns E_DB_xxx OK or error indication;  error info in problem
**	indexcb filled in.
**
** History:
**	25-Feb-2008 (kschendel)  SIR 122739
**	    Extract another 500 or so lines of common code between index
**	    and parallel index, so that I can figure out where to put the
**	    new row-accessor stuff.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Use no-coupon access modes.
**	9-Jul-2010 (kschendel) SIR 123450
**	    Index btree key compression is wired to old-standard for now.
**	27-Jul-2010 (kschendel) b124135
**	    Ask for row-versioning when sizing compression control array.
**	    We don't know if there are any versioned/altered atts, assume
**	    the worst rather than the best.
*/

DB_STATUS
dm2uMakeIndMxcb(DM2U_MXCB **mxcb,
	DM2U_INDEX_CB *index_cb, DMP_RCB *r,
	i4 timeout, i4 tbl_lk_mode,
	i4 sources, i4 targets)
{
    char *bufArray;			/* Record buffer area */
    char *p;				/* Layout pointer */
    DB_ERROR *dberr;			/* DB error code in caller CB */
    DB_STATUS status;
    DM2U_MXCB *m;			/* The new MXCB */
    DM2U_SPCB *sp;			/* Source partition CB */
    DM2U_TPCB *tp;			/* Target partition CB */
    DML_XCB *xcb;			/* Transaction control block */
    DMP_DCB *dcb;			/* Database control block */
    DMP_LOCATION *locArray;		/* Location array base */
    DMP_LOCATION *TlocArray;		/* Locations per target */
    DMP_TCB *t;				/* Base table's TCB */
    f8 *range_entry;
    i4 AllAttsCount;
    i4 buf_size;			/* Record buffer size */
    i4 ClusterKeyCount, ClusterKeyLen;
    i4 compare;
    i4 data_cmpcontrol_size;
    i4 i,k;
    i4 index_cmpcontrol_size;
    i4 leaf_cmpcontrol_size;
    i4 loc_count;			/* Total locations, all partitions */
    i4 reltups;				/* Base table row count */
    i4 Rtreesize;			/* Extra RTREE stuff needed */
    i4 TidSize;				/* tidp size needed */
    i4 attnmsz;
    char *nextattname;
    DB_ATTS *curatt;

    t = r->rcb_tcb_ptr;
    dcb = index_cb->indxcb_dcb;
    xcb = index_cb->indxcb_xcb;
    loc_count = index_cb->indxcb_l_count;

    /* Note this points to a DMU_CB->error */
    dberr = index_cb->indxcb_errcb;

    data_cmpcontrol_size = 0;
    leaf_cmpcontrol_size = 0;
    index_cmpcontrol_size = 0;

    /*
    **  Allocate a MODIFY/INDEX control block.
    **  Allocate space for the key atts pointers, the compare list and a
    **  record.  Modify to hash includes one extra compare list entry so
    **  that data is sorted by bucket.
    **
    **  ADDITIONAL NOTE: At the time that we perform the allocation, we
    **  don't have the index tuple size readily available. Therefore, we
    **  assume that it will be less than the size of the base table tuple
    **  plus the TIDP and an extra i4 for the HASH value or Hilbert
    **  value (HASH value is used only for secondary HASH indexes).
    **  (Hilbert value is used only for secondary RTree indexes).
    **  Most 2-ary indexes don't need this much space --
    **  we are allocating the max size here.
    **
    */

    /*
    ** If creating a global index on a partitioned table
    ** the indexed rows have TID8s (partition_number, TID).
    **
    ** If the indexed table is clustered, the rows of the table live inside
    ** Leaf pages and their TIDs are actually BIDs. These BIDs cannot by
    ** themselves accurately locate a row in the Clustered table because
    ** Leaf entries can move (hence their BIDs may change). Rather, the BID
    ** is used as a "hint" and tells where the Clustered row was at the
    ** time it was inserted into the index; the unique Cluster key is also
    ** stored in the index row along with the hint BID and is used when
    ** needed to find and/or verify the actual indexed Cluster row. As such,
    ** the BID will never be a key attribute.
    */
    if ( index_cb->indxcb_relstat2 & TCB2_GLOBAL_INDEX )
	TidSize = sizeof(DM_TID8);
    else
	TidSize = sizeof(DM_TID);

    /* Indexing a Clustered Table? */
    ClusterKeyLen = 0;
    ClusterKeyCount = 0;

    if ( t->tcb_rel.relstat & TCB_CLUSTERED )
    {
	/* Compute the number of atts and length of cluster key */
	while ( ClusterKeyCount < t->tcb_keys )
	{
	    ClusterKeyLen += t->tcb_key_atts[ClusterKeyCount]->length;
	    ++ ClusterKeyCount;
	}
    }

    /* The sum of all attributes */
    AllAttsCount = index_cb->indxcb_acount + ClusterKeyCount;

    /* Guess at (upper limit for) NEW compression control sizes.
    ** Throw in a +1 in case tidp ends up part of the key.
    ** Another +1 in case rtree and hilbert.  (It's just an upper limit.)
    */
    data_cmpcontrol_size = dm1c_cmpcontrol_size(
		index_cb->indxcb_compressed, AllAttsCount+2, t->tcb_rel.relversion);
    if (index_cb->indxcb_index_compressed)
    {
	/* Key compression is wired to "old standard" at present */
	if (index_cb->indxcb_structure == TCB_RTREE)
	{
	    index_cmpcontrol_size = dm1c_cmpcontrol_size(
		TCB_C_STD_OLD, AllAttsCount+2, t->tcb_rel.relversion);
	}
	else if (index_cb->indxcb_structure == TCB_BTREE)
	{
	    /* Index entry is keys perhaps with tidp.
	    ** Leaf entry is the whole row including any non-key atts.
	    */
	    index_cmpcontrol_size = dm1c_cmpcontrol_size(
		TCB_C_STD_OLD, index_cb->indxcb_kcount+1, t->tcb_rel.relversion);
	    leaf_cmpcontrol_size = dm1c_cmpcontrol_size(
		TCB_C_STD_OLD, AllAttsCount+1, t->tcb_rel.relversion);
	}
    }
    data_cmpcontrol_size = DB_ALIGN_MACRO(data_cmpcontrol_size);
    index_cmpcontrol_size = DB_ALIGN_MACRO(index_cmpcontrol_size);
    leaf_cmpcontrol_size = DB_ALIGN_MACRO(leaf_cmpcontrol_size);

    /* Allocate a set of 4 record buffers for each input source.
    ** Recall per above that we need an extra i4 for hash or Hilbert
    ** value -- not always but we might as well tack it on.
    */
    buf_size = t->tcb_rel.relwid + t->tcb_data_rac.worstcase_expansion;
    buf_size = DB_ALIGNTO_MACRO(buf_size, sizeof(i4));
    buf_size += ClusterKeyLen + TidSize + sizeof(i4);
    buf_size += sizeof(DM2U_RHEADER);
    buf_size = DB_ALIGN_MACRO(buf_size);

    if ( index_cb->indxcb_structure == TCB_RTREE)
	Rtreesize = sizeof(DMPP_ACC_KLV);
    else
	Rtreesize = 0;

    /* Allocate zero-filled memory */
    /* 3 sets of atts pointers: base, index, data, allowing 2 extra for
    ** tidp and possible hash/hilbert compare att;
    ** The compare list;
    ** The actual attribute array, starting at 1, plus 2 extra (tidp etc);
    ** Space for an i4 attribute number mapping array;
    ** The rtree overhead if any;
    ** SPCB source control blocks (n/u for parallel index);
    ** source record buffers (n/u for parallel index);
    ** TPCB target control block pointer array;
    ** target TPCB's;
    ** 3 record buffers and location arrays per target;
    ** one dummy record buffer for signalling.
    ** Ideally these would be individually db-aligned, but throwing in the
    ** occasional padding works just as well.
    */
    attnmsz = (AllAttsCount + 3) * sizeof(DB_ATT_STR);
    attnmsz = DB_ALIGN_MACRO(attnmsz);

    status = dm0m_allocate(sizeof(DM2U_MXCB) +
	     3 * ((AllAttsCount + 2) * sizeof(DB_ATTS *)) +
	     (AllAttsCount + 2) * sizeof(DB_CMP_LIST) +
	     (AllAttsCount + 3) * sizeof(DB_ATTS) +
	     (AllAttsCount + 3) * sizeof(i4) +
	     sizeof(ALIGN_RESTRICT) +
	     data_cmpcontrol_size +
	     leaf_cmpcontrol_size +
	     index_cmpcontrol_size +
	     attnmsz +
	     sources * sizeof(DM2U_SPCB) +
	     sources * buf_size +
	     sizeof(ALIGN_RESTRICT) +
	     targets * sizeof(DM2U_TPCB *) +
	     sizeof(ALIGN_RESTRICT) +
	     (targets * (sizeof(DM2U_TPCB) + 3 * buf_size +
			    loc_count * sizeof(DMP_LOCATION))) +
	     Rtreesize +
	     sizeof(ALIGN_RESTRICT) +
	     buf_size,
	     DM0M_ZERO, (i4)MXCB_CB, (i4)MXCB_ASCII_ID,
	     (char *)xcb, (DM_OBJECT **) mxcb,
	     dberr);
    if (status != E_DB_OK)
	return (status);

    m = *mxcb;

    /* Lay out the big chunk of memory that was allocated */
    m->mx_operation = MX_INDEX;
    m->mx_b_key_atts = (DB_ATTS **)((char *)m + sizeof(DM2U_MXCB));
    m->mx_i_key_atts = (DB_ATTS **)((char *)m->mx_b_key_atts +
		       (AllAttsCount + 2) * sizeof(DB_ATTS *));
    m->mx_atts_ptr = (DB_ATTS *)((char *)m->mx_i_key_atts +
		       (AllAttsCount + 2) * sizeof(DB_ATTS *));
    m->mx_data_rac.att_ptrs = (DB_ATTS **)((char *)m->mx_atts_ptr +
		       (AllAttsCount + 3) * sizeof(DB_ATTS));
    m->mx_cmp_list = (DB_CMP_LIST *)((char *)m->mx_data_rac.att_ptrs +
		       (AllAttsCount + 2) * sizeof(DB_ATTS *));
    m->mx_att_map = (i4 *)((char *)m->mx_cmp_list +
		       (AllAttsCount + 2) * sizeof(DB_CMP_LIST));
    p = (char *) m->mx_att_map + (AllAttsCount + 3) * sizeof(i4);
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
    for (curatt = m->mx_atts_ptr, i = 0; i < (AllAttsCount + 3); i++,curatt++)
    {
	curatt->attnmstr = nextattname;
	nextattname += sizeof(DB_ATT_STR);
    }
    /* P remains aligned since cmpcontrol sizes are aligned */

    m->mx_rcb = r;
    m->mx_xcb = xcb;
    m->mx_dcb = dcb;

    /* Non-rollforward calls put proper log ID, etc into the RFP area
    ** for parameter passing convenience.  For normal indexing these things
    ** come from the XCB.
    */
    m->mx_log_id = index_cb->indxcb_rfp.rfp_log_id;
    m->mx_lk_id = index_cb->indxcb_rfp.rfp_lk_id;
    /* name-id and name-gen too, but they go into the TPCB(s) */

    m->mx_tidsize = TidSize;

    /* Stuff needed later for source's dm2t_open() calls */
    m->mx_db_lk_mode = index_cb->indxcb_db_lockmode;
    m->mx_tbl_lk_mode = tbl_lk_mode;
    m->mx_tbl_access_mode = DM2T_A_READ_NOCPN;
    m->mx_timeout = timeout;

    /* Until we have partitioned indexes, this will be null */
    m->mx_part_def = NULL;
    m->mx_phypart = NULL;

    m->mx_Mlocations = NULL;
    m->mx_Mloc_count = 0;
    m->mx_Moloc_count = 0;

    /* Set up the source(s) and target(s) */
    m->mx_spcb_count = sources;
    m->mx_tpcb_count = targets;
    m->mx_spcb_next = (DM2U_SPCB *) NULL;
    m->mx_tpcb_next = (DM2U_TPCB *) NULL;

    /* Init threading stuff (not used by || index) */
    m->mx_status = E_DB_OK;
    CLRDBERR(&m->mx_dberr);
    m->mx_state = 0;
    m->mx_threads = 0;
    m->mx_new_threads = 0;

    /*
    ** Memory layout:
    **
    **	SPCBs (1 per source, none if parallel index)
    **	TPCBs (1 per target)
    **	buffers (1 per source, 3 per target, plus 1)
    **	target locations
    **
    ** (currently only one target because we don't have partitioned indexes,
    ** but no harm in being generic.)
    */

    sp = (DM2U_SPCB *) p;
    m->mx_tpcb_ptrs = (DM2U_TPCB **)((char*)sp + sources * sizeof(DM2U_SPCB));
    m->mx_tpcb_ptrs = (DM2U_TPCB **) ME_ALIGN_MACRO(m->mx_tpcb_ptrs,
						  sizeof(ALIGN_RESTRICT));
    tp = (DM2U_TPCB*)((char*)m->mx_tpcb_ptrs + targets * sizeof(DM2U_TPCB*));
    /* Ensure proper alignment of TPCBs */
    tp = (DM2U_TPCB*)ME_ALIGN_MACRO(tp, sizeof(ALIGN_RESTRICT));

    bufArray = (char*)tp + targets * sizeof(DM2U_TPCB);
    locArray = (DMP_LOCATION *) ((char*)bufArray
				    + (sources * 1 * buf_size)
				    + (targets * 3 * buf_size)
				    + buf_size);

    /* One record for signalling thru CUT */
    m->mx_signal_rec = (DM2U_RHEADER *) bufArray;
    bufArray = bufArray + buf_size;

    /* If parallel index, no sources, mx_spcb_next remains NULL */
    for ( i = 0; i < sources; i++ )
    {
	/* Link SPCB to MXCB list of sources */
	if ( i )
	    m->mx_spcb_next[i-1].spcb_next = sp;
	else
	    m->mx_spcb_next = sp;
	sp->spcb_next = (DM2U_SPCB *) NULL;

	sp->spcb_mxcb = m;

	/*
	** Master's Table, or non-partitioned
	** single source, has already been
	** opened iby the caller ("r").
	**
	** The other source(s) won't be opened until
	** needed by dm2u_load_table
	*/
	if ( t->tcb_rel.relstat & TCB_IS_PARTITIONED )
	{
	    sp->spcb_rcb = (DMP_RCB*)NULL;
	    STRUCT_ASSIGN_MACRO(t->tcb_pp_array[i].pp_tabid,
				 sp->spcb_tabid);
	    sp->spcb_partno = i;
	}
	else
	{
	    sp->spcb_rcb = r;
	    STRUCT_ASSIGN_MACRO(t->tcb_rel.reltid,
				 sp->spcb_tabid);
	    sp->spcb_partno = t->tcb_partno;
	}

	sp->spcb_positioned = FALSE;
	sp->spcb_log_id = m->mx_log_id;
	sp->spcb_lk_id = m->mx_lk_id;
	sp->spcb_loc_count = 0;
	sp->spcb_locations = NULL;
	sp->spcb_record = (DM2U_RHEADER *) bufArray;

	bufArray += buf_size;
	sp++;
    }

    /* Set up the target(s) */
    TlocArray = locArray;

    for( i = 0; i < targets; i++ )
    {
	/* Link TPCB to MXCB list of targets */
	if ( i )
	    m->mx_tpcb_next[i-1].tpcb_next = tp;
	else
	    m->mx_tpcb_next = tp;
	tp->tpcb_next = (DM2U_TPCB*)NULL;

	tp->tpcb_mxcb = m;

	/*
	** Index's table id will be made known by dm2u_create when it happens.
	*/
	tp->tpcb_tabid.db_tab_base = 0;
	tp->tpcb_tabid.db_tab_index = 0;
	/* Indexes are not (yet) partitioned */
	tp->tpcb_partno = 0;
	tp->tpcb_buildrcb = (DMP_RCB *)NULL;
	tp->tpcb_log_id = m->mx_log_id;
	tp->tpcb_lk_id = m->mx_lk_id;
	tp->tpcb_srt = (DMS_SRT_CB *)NULL;
	tp->tpcb_srt_wrk = NULL;
	tp->tpcb_newtup_cnt = 0;
	tp->tpcb_state = 0;
	tp->tpcb_sid = (CS_SID)NULL;
	tp->tpcb_name_id = index_cb->indxcb_rfp.rfp_name_id;
	tp->tpcb_name_gen = index_cb->indxcb_rfp.rfp_name_gen;
	tp->tpcb_loc_list = (DB_LOC_NAME*)NULL;
	/* **** FIXME when we get into partitioned targets there's
	** a goof-up with loc_count (total) vs loc_count (one target);
	** the loc_count variable here is total across all targets.
	** not a problem for now
	*/
	tp->tpcb_loc_count = loc_count;
	tp->tpcb_locations = TlocArray;
	tp->tpcb_oloc_count = loc_count;
	tp->tpcb_new_locations = FALSE;

	tp->tpcb_crecord = bufArray;
	tp->tpcb_irecord = bufArray + 1 * buf_size;
	tp->tpcb_srecord = (DM2U_RHEADER *) (bufArray + 2 * buf_size);

	bufArray += 3 * buf_size;

	TlocArray += tp->tpcb_loc_count;

	m->mx_tpcb_ptrs[tp->tpcb_partno] = tp;

	tp++;
    }

    m->mx_acc_klv = NULL;
    range_entry = &m->mx_range[0];
    if ( index_cb->indxcb_structure == TCB_RTREE)
    {
	m->mx_acc_klv = (DMPP_ACC_KLV *)((char *)locArray +
			    loc_count * sizeof(DMP_LOCATION) );
	m->mx_acc_klv = (DMPP_ACC_KLV *) ME_ALIGN_MACRO(m->mx_acc_klv, sizeof(ALIGN_RESTRICT));
    }
    if (index_cb->indxcb_structure != TCB_RTREE
      || index_cb->indxcb_range == NULL)
    {
       for (i = 0; i < DB_MAXRTREE_DIM * 2; i++, range_entry++)
	    *range_entry = 0.0;
    }
    else
    {
	for (i = 0; i < DB_MAXRTREE_DIM * 2; i++, range_entry++)
	    *range_entry = index_cb->indxcb_range[i];
    }

    m->mx_dimension = 0;
    m->mx_hilbertsize = 0;

    m->mx_structure = index_cb->indxcb_structure;
    m->mx_data_rac.compression_type = index_cb->indxcb_compressed;
    m->mx_index_comp = index_cb->indxcb_index_compressed;
    if (index_cb->indxcb_index_compressed)
    {
	m->mx_leaf_rac.compression_type = TCB_C_STD_OLD;
	m->mx_index_rac.compression_type = TCB_C_STD_OLD;
    }
    m->mx_unique = index_cb->indxcb_unique;
    m->mx_dmveredo = index_cb->indxcb_dmveredo;
    m->mx_truncate = 0;
    m->mx_duplicates = 1;
    m->mx_reorg = 0;
    m->mx_dfill = index_cb->indxcb_d_fill;
    m->mx_ifill = index_cb->indxcb_i_fill;
    m->mx_lfill = index_cb->indxcb_l_fill;
    m->mx_maxpages = index_cb->indxcb_max_pages;
    m->mx_minpages = index_cb->indxcb_min_pages;

    /* All attributes, key and non-key */
    m->mx_ab_count = index_cb->indxcb_acount;
    /* Just key attributes */
    m->mx_ai_count = index_cb->indxcb_kcount;

    m->mx_width = 0;
    m->mx_kwidth = 0;
    m->mx_src_width = t->tcb_rel.relwid;

    m->mx_page_size = index_cb->indxcb_page_size;
    m->mx_page_type = index_cb->indxcb_page_type;
    m->mx_buildrcb = r;		/* Same as mx_rcb */
    m->mx_flags = 0;
    m->mx_concurrent_idx = ((index_cb->indxcb_relstat2 & TCB2_CONCURRENT_ACCESS) != 0);
    m->mx_newtup_cnt = 0;
    /* No such thing as Clustered Indexes */
    m->mx_clustered = FALSE;

    /* mx_inmemory is set by caller */

    /*
    ** Set index table's cache priority.
    */
    m->mx_tbl_pri = index_cb->indxcb_tbl_pri;

    if (index_cb->indxcb_allocation <= 0)
	index_cb->indxcb_allocation = DM_TBL_DEFAULT_ALLOCATION;
    if (index_cb->indxcb_extend <= 0)
	index_cb->indxcb_extend = DM_TBL_DEFAULT_EXTEND;

    /* Future: if partitioned index, slice the allocation among partitions */

    /*
    ** If a multi-location table make sure that the allocation and
    ** extend sizes are factors of DI_EXTENT_SIZE. This is for
    ** performance reasons.
    */
    if (loc_count > 1 )
    {
	dm2f_round_up( &index_cb->indxcb_allocation );
	dm2f_round_up( &index_cb->indxcb_extend );
    }

    m->mx_allocation = index_cb->indxcb_allocation;
    m->mx_extend = index_cb->indxcb_extend;

    /*
    ** Init estimated row count. For SYSMOD iirtempx builds this value is
    ** passed in by the caller. Otherwise use the parent table row count.
    */
    reltups = index_cb->indxcb_reltups;
    if (reltups == 0)
    {
	i4 toss_relpages;

	/* Mutex the list while we peek at it */
	dm0s_mlock(&t->tcb_mutex);

	dm2t_current_counts(t, &reltups, &toss_relpages);

	dm0s_munlock(&t->tcb_mutex);
    }
    m->mx_reltups = reltups;

    for (k = 0; k < loc_count; k++)
    {
	/*  Check location, must be default or valid location. */
	if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&index_cb->indxcb_location[k],
	    sizeof(DB_LOC_NAME)) == 0)
	{
	    locArray[k].loc_ext = dcb->dcb_root;
	    STRUCT_ASSIGN_MACRO(dcb->dcb_root->logical,
				locArray[k].loc_name);
	    locArray[k].loc_id = k;
	    locArray[k].loc_fcb = 0;
	    locArray[k].loc_status = LOC_VALID;
	    locArray[k].loc_config_id = 0;
	    continue;
	}
	else
	{
	    for (i = 0; i < dcb->dcb_ext->ext_count; i++)
	    {
		compare = MEcmp((char *)&index_cb->indxcb_location[k],
			    (char *)&dcb->dcb_ext->ext_entry[i].logical,
			    sizeof(DB_LOC_NAME));
		if (compare == 0 &&
		   (dcb->dcb_ext->ext_entry[i].flags & DCB_DATA))
		{
		    locArray[k].loc_ext = &dcb->dcb_ext->ext_entry[i];
		    STRUCT_ASSIGN_MACRO(index_cb->indxcb_location[k],
					locArray[k].loc_name);
		    locArray[k].loc_id = k;
		    locArray[k].loc_fcb = 0;
		    locArray[k].loc_status = LOC_VALID;
		    if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
			locArray[k].loc_status |= LOC_RAW;

		    locArray[k].loc_config_id = i;
		    break;
		}
	    }
	    if (compare)
	    {
		/*
		** Error on one of the locations. Set err_data to the index
		** to the bad location entry.
		*/
		SETDBERR(dberr, k, E_DM001D_BAD_LOCATION_NAME);
		break;
	    }
	}
    } /* end multiple location for loop */


    if (dberr->err_code)
    {
	return (E_DB_ERROR);
    }

    /* Note this includes TCB2_GLOBAL_INDEX bit */
    m->mx_new_relstat2 = index_cb->indxcb_relstat2;

    return (E_DB_OK);
} /* dm2uMakeIndMxcb */

/*{
** Name: dm2uMakeIndAtts - Make all the attribute stuff for an index.
**
** Description:
**      Makes all the attribute information for an index, verifies that
**	it all "fits".
**
** Inputs:
**	index_cb			Describes the index being created:
**	    indxcb_structure		Index structure
**	    indxcb_kcount		Number of key attributes
**	    indxcb_acount		Number of key+non-key attributes
**	m				DM2U_MXCB allocated by the caller
**					and presumed be correctly sized.
**	r				Base Table DMP_RCB.
**	CreAtts				A place to build DMF_ATTR_ENTRY
**					structures for dm2u_create.
** Outputs:
**	index_cb	
**	    indxcb_idx_key
**	DM2U_MXCB	
**	    mx_dimension		
**	    mx_hilbertsize
**	    mx_idom_map
**	    mx_b_key_atts
**	    mx_atts_ptr
**	    mx_data_rac (att_ptrs, att_count)
**	    mx_leaf_rac (att_ptrs, att_count)
**	    mx_index_rac (att_ptrs, att_count)
**	    mx_att_map
**	    mx_i_key_atts
**	    mx_cmp_list
**	    mx_width
**	    mx_kwidth
**	NumCreAtts			Number of DMF_ATTR_ENTRY things
**					made for dm2u_create.
**	ProjColMap			Updated key projection map,   
**					if doing key projection.
**	dberr
**          .err_code			The reason for an error return status.
**					E_DM0007_BAD_ATTR_NAME
**					E_DM001C_BAD_KEY_SEQUENCE
**					E_DM0066_BAD_KEY_TYPE
**					E_DM007D_BTREE_BAD_KEY_LENGTH
**					    err_data gives bad length value
**					E_DM0103_TUPLE_TOO_WIDE
**					E_DM010F_ISAM_BAD_KEY_LENGTH
**					    err_data gives bad length value
**	    .error_data			Additional information, if any.
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
**	26-May-2006 (jenjo02)
**	    Built from all-too-similar code in both dm2u_index and
**	    dm2u_pindex, mostly to support secondary indexes on
**	    Clustered tables, but also to consolidate the code and
**	    ensuring bugs in one place.
**	11-Apr-2008 (kschendel)
**	    RCB's adfcb now typed as such, minor fixes here.
**	29-Sept-2009 (troal01)
**		Add geospatial support.
**	9-Jul-2010 (kschendel) SIR 123450
**	    Index btree key compression is wired to old-standard for now.
**      19-Jul-2010 (thich01)
**              Initialize colencwid properly so rtree data is filled.
*/
DB_STATUS
dm2uMakeIndAtts(
DM2U_INDEX_CB	*index_cb,
DM2U_MXCB	*m,
DMP_RCB		*r,
DMF_ATTR_ENTRY **CreAtts,
i4		*NumCreAtts,
bool		KeyProjection,
i4		ProjColMap[],
DB_ERROR	*dberr)
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    ADF_CB	*adf_cb = r->rcb_adf_cb;
    i4		AttMap[((DB_MAX_COLS + 31) / 32) + 1];
    i4		i,j,k;
    i4		ax, bx, cx, dx, ix, mx;
    i4		dt_bits;
    i4		collength;
    i4		colencwid = 0;
    i2		coltype;
    bool	upcase;
    bool	encrypted_index;
    i4		OnlyKeyLen = 0;
    DB_STATUS	status = E_DB_OK;
    DB_ATT_NAME tmpattnm;

    CLRDBERR(dberr);

    if ( index_cb->indxcb_dcb->dcb_status & DCB_S_ROLLFORWARD )
	upcase = index_cb->indxcb_rfp.rfp_upcase;
    else
	upcase = (*index_cb->indxcb_xcb->xcb_scb_ptr->scb_dbxlate & CUI_ID_REG_U) != 0;

    do
    {
        /*
        ** Check key descriptions.  Build the atts pointer and the
        ** compare list.
        */

	MEfill(sizeof(AttMap), '\0', (PTR)AttMap);
	MEfill(sizeof(index_cb->indxcb_idx_key), '\0',
		(PTR)index_cb->indxcb_idx_key);

	/*
	** ax indexes the key and non-key attributes (mx_atts_ptr), 1-based
	** bx indexes the base table attributes (mx_b_key_atts)
	** cx indexes the DMF_ATTR_ENTRY atts for dm2u_create (CreAtts)
	** dx indexes the data attributes (mx_data_rac) for compression
	** ix indexes the key attributes (mx_i_key_atts)
	** mx indexes the compare attributes (mx_cmp_list)
	*/
	ax = 1;
	bx = m->mx_ab_count = 0;
	cx = *NumCreAtts = 0;
	dx = 0;
	ix = m->mx_ai_count = 0;
	mx = m->mx_c_count = 0;

	/* If this is an encrypted table and we are building an index
	** on it, it might be a normal index, or it might be an encrypted
	** index, which must comply with these rules:
	** (1) only one column in the index
	** (2) column must be encrypted NOSALT
	** (3) must be a HASH index
	*/
	encrypted_index = FALSE;
	if ((t->tcb_rel.relencflags & TCB_ENCRYPTED) &&
	    (index_cb->indxcb_acount == 1))
	{ /* we might have a valid encrypted index */
            for (j = 1; j <= t->tcb_rel.relatts; j++)
            {
		MEmove(t->tcb_atts_ptr[j].attnmlen,
		    t->tcb_atts_ptr[j].attnmstr,
		    ' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);
		if ((MEcmp(tmpattnm.db_att_name,
		     (char *)&index_cb->indxcb_key[0]->key_attr_name,
		     sizeof(index_cb->indxcb_key[0]->key_attr_name)) == 0) &&
		    (t->tcb_atts_ptr[j].ver_altcol == 0) &&
		    (t->tcb_atts_ptr[j].ver_added >= t->tcb_atts_ptr[j].ver_dropped))
                 {
                     break;
                 }
            }
            if (j <= t->tcb_rel.relatts &&
		    t->tcb_atts_ptr[j].encflags & ATT_ENCRYPT)
	    {
		if (!(t->tcb_atts_ptr[j].encflags & ATT_ENCRYPT_SALT) && 
			m->mx_structure == TCB_HASH)
		    encrypted_index = TRUE;
		else
		{
		    status = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM019F_INVALID_ENCRYPT_INDEX);
		    break;
		}
	    }
	}

	/*
	** If HASH, reserve 1st compare attribute for hash bucket;
	** If RTREE, reserve 1st two compare attributes, first for
	** HILBERT value, second for TIDP.
	*/
        if (m->mx_structure == TCB_HASH)
	  mx = 1;
	else if (m->mx_structure == TCB_RTREE)
	  mx = 2;

	/* For each user-named attribute in the Index... */
        for ( i = 0; i < index_cb->indxcb_acount; i++, ax++, bx++, cx++, dx++ )
        {
	    /* Find the attribute in the TCB's list: */
            for (j = 1; j <= t->tcb_rel.relatts; j++)
            {
		MEmove(t->tcb_atts_ptr[j].attnmlen,
		    t->tcb_atts_ptr[j].attnmstr,
		    ' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);
		if ((MEcmp(tmpattnm.db_att_name,
		     (char *)&index_cb->indxcb_key[i]->key_attr_name,
		     sizeof(index_cb->indxcb_key[i]->key_attr_name)) == 0) &&
		    (t->tcb_atts_ptr[j].ver_altcol == 0) &&
		    (t->tcb_atts_ptr[j].ver_added >= t->tcb_atts_ptr[j].ver_dropped))
                 {
                     break;
                 }
            }
            if (j > t->tcb_rel.relatts)
            {
		/* Didn't find it */
		SETDBERR(dberr, 0, E_DM0007_BAD_ATTR_NAME);
                break;
            }
	    /* Named again? */
            if (AttMap[j >> 5] & (1 << (j & 0x1f)))
            {
		SETDBERR(dberr, 0, E_DM001C_BAD_KEY_SEQUENCE);
                break;
            }
	    /* if we stumble across an encrypted index column and have not
	    ** already decided this is a valid encrypted index, then there
	    ** is something rotten in the state of Denmark.
	    */
	    if ((t->tcb_atts_ptr[j].encflags & ATT_ENCRYPT) &&
	        encrypted_index == FALSE)
	    {
		SETDBERR(dberr, 0, E_DM0066_BAD_KEY_TYPE);
		break;
	    }
                
            if (m->mx_structure == TCB_RTREE)
            {
                if ( dm1c_getklv(adf_cb, t->tcb_atts_ptr[j].type,
                                     m->mx_acc_klv) )
                {
		    SETDBERR(dberr, 0, E_DM0066_BAD_KEY_TYPE);
		    break;
                }
		m->mx_dimension = m->mx_acc_klv->dimension;
		m->mx_hilbertsize = m->mx_acc_klv->hilbertsize;
    	    }

	    if ( adi_dtinfo(adf_cb, (DB_DT_ID) t->tcb_atts_ptr[j].type, &dt_bits) )
            {
		SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
                break;
            }
	    /*
	    ** For Rtree allow a nosort or noindex data type as long as the
	    ** nbr function is defined.  (shero03)
	    */
	    if ( m->mx_structure != TCB_RTREE && 
			dt_bits & (AD_NOSORT | AD_NOKEY) )
            {
		SETDBERR(dberr, 0, E_DM0066_BAD_KEY_TYPE);
                break;
            }

	    /* Remember we've seen this one */
            AttMap[(j >> 5)] |= 1 << (j & 0x1f);

            /*
            **  Build a attribute record used to call create table
            **  for next attribute in the index.
            **  N.B. For RTree it is: NBR, HILBERT, TIDP 
	    **  However, the sort record is Hilbert, TIDP, NBR
            **  NBRs are hilbertsize * 2
	    **  e.g. hilbertsize = 8 => NBR is 8 bytes * 2 or 16
            */

            if (m->mx_structure == TCB_RTREE)
	    {
              collength = m->mx_hilbertsize * 2;
              colencwid = 0;
	      coltype = m->mx_acc_klv->nbr_dt_id;
	    }
            else
	    {
              collength = t->tcb_atts_ptr[j].length;
              colencwid = t->tcb_atts_ptr[j].encwid;
	      coltype = t->tcb_atts_ptr[j].type;
	    }

	    MEmove(t->tcb_atts_ptr[j].attnmlen,
		t->tcb_atts_ptr[j].attnmstr,
		' ', DB_ATT_MAXNAME, CreAtts[cx]->attr_name.db_att_name);
            CreAtts[cx]->attr_type = coltype;
            CreAtts[cx]->attr_size = collength;
            CreAtts[cx]->attr_precision = t->tcb_atts_ptr[j].precision;
            CreAtts[cx]->attr_flags_mask = t->tcb_atts_ptr[j].flag;
            COPY_DEFAULT_ID( t->tcb_atts_ptr[j].defaultID,
                           CreAtts[cx]->attr_defaultID );
            CreAtts[cx]->attr_collID = t->tcb_atts_ptr[j].collID;
            CreAtts[cx]->attr_geomtype = t->tcb_atts_ptr[j].geomtype;
            CreAtts[cx]->attr_srid = t->tcb_atts_ptr[j].srid;
            CreAtts[cx]->attr_encflags = t->tcb_atts_ptr[j].encflags;
            CreAtts[cx]->attr_encwid = t->tcb_atts_ptr[j].encwid;


            /* 
	    ** Build the map of key number to attribute number for
            ** logging. 
	    */
            index_cb->indxcb_idx_key[i] = j;

            /*  Build map from key number to attribute number. */
            m->mx_idom_map[i] = j;

            /*
            **  Build pointer to attribute description for index key
            **  record from the base table.
            */
	    if ( KeyProjection )
	    {
		m->mx_b_key_atts[bx] = &r->rcb_proj_atts_ptr[j];
		ProjColMap[(j >> 5)] |= 1 << (j & 0x1f);
	    }
	    else
		m->mx_b_key_atts[bx] = &t->tcb_atts_ptr[j];

            /*
            **  Build a attribute entry for the next record of the
            **  index table for use by the access method specific
            **  build routines.
            */
	    STRUCT_ASSIGN_MACRO(t->tcb_atts_ptr[j], m->mx_atts_ptr[ax]);
            m->mx_atts_ptr[ax].length = collength;
            m->mx_atts_ptr[ax].offset = m->mx_width;
            m->mx_atts_ptr[ax].key = 0;
            m->mx_atts_ptr[ax].key_offset = m->mx_kwidth;
	    /* mxcb memory initially zeroed, so ver_added etc is clear */

	    m->mx_data_rac.att_ptrs[dx] = &m->mx_atts_ptr[ax];

            if (i < index_cb->indxcb_kcount)
            {
                /*  Build map from attribute number to key number. */
                m->mx_att_map[ax] = ax;
                m->mx_atts_ptr[ax].key = ax;

                /*  Build pointer to attribute description for index key
                **  record from the index table.
                */
                m->mx_i_key_atts[ix] = &m->mx_atts_ptr[ax];

                /*
                **  Build a record comparision list for the next attribute
                **  in the index record.
                */
                m->mx_cmp_list[mx].cmp_type = coltype;
                m->mx_cmp_list[mx].cmp_precision = t->tcb_atts_ptr[j].precision;
                m->mx_cmp_list[mx].cmp_length = collength;
                m->mx_cmp_list[mx].cmp_offset = m->mx_width;
                m->mx_cmp_list[mx].cmp_direction = 0;
                m->mx_cmp_list[mx].cmp_collID = t->tcb_atts_ptr[j].collID;
		ix++;
                mx++;
            }

            m->mx_width += colencwid > 0 ? colencwid : collength;
            m->mx_datawidth += collength;

            /*
            **  If this column is part of the key, update the key width.
            **  Btree secondary indices store full data records, so the "key
            **  width" for a Btree secondary includes all columns.
            */

	    if ( i < index_cb->indxcb_kcount )
		OnlyKeyLen += collength;
            if (i < index_cb->indxcb_kcount || m->mx_structure == TCB_BTREE) 
                m->mx_kwidth += collength;
        }
        if ( dberr->err_code )
	{
	    status = E_DB_ERROR;
            break;
	}

	/* After the User-specified attributes come the Cluster Key atts: */
	if ( t->tcb_rel.relstat & TCB_CLUSTERED )
	{
	    for ( j = 0; j < t->tcb_keys; j++ ) 
	    {
		/*
		** Extract the Base Table cluster keys.
		**
		** Cluster Keys look just like user-defined non-key
		** attributes and are used to verify rows in a Clustered
		** table because the TID is really a BID and cannot
		** by itself guarantee that the correct base row has been
		** read when accessed via the Index.
		**
		** There may be up to 65 attributes in the index,
		** 32 user-defined plus 32 Cluster keys plus the TID.
		**
		** Note that these attributes are included in the index's
		** domain map (mx_idom_map). 
		**
		** But are included in the logged
		** key-number-to-base-table-attribute-number map (idx_key)
		** for REDO (though I don't think I need to do this if I
		** make dmveindex smart) ***FIXME
		**
		** Cluster key attributes that have been coincidentally
		** named and included by the user are skipped.
		*/
		k = t->tcb_key_atts[j]->ordinal_id;

		if ( !(AttMap[k >> 5] & (1 << (k & 0x1f))) )
		{
		    /* Add cluster key to logging map */
		    index_cb->indxcb_idx_key[bx] = j;

		    /*  Add cluster key to index domain map */
		    m->mx_idom_map[bx] = j;

		    /* Make an attribute for Create Table */
		    MEmove(t->tcb_key_atts[j]->attnmlen,
			t->tcb_key_atts[j]->attnmstr,
		    ' ', DB_ATT_MAXNAME, CreAtts[cx]->attr_name.db_att_name);

		    CreAtts[cx]->attr_type = t->tcb_key_atts[j]->type; 
		    CreAtts[cx]->attr_size = t->tcb_key_atts[j]->length;
		    CreAtts[cx]->attr_precision = t->tcb_key_atts[j]->precision;
		    CreAtts[cx]->attr_flags_mask = t->tcb_key_atts[j]->flag;
		    COPY_DEFAULT_ID(t->tcb_key_atts[j]->defaultID,
					CreAtts[cx]->attr_defaultID);
		    CreAtts[cx]->attr_defaultTuple = (DB_IIDEFAULT*)NULL;
		    CreAtts[cx]->attr_collID = t->tcb_key_atts[j]->collID;
		    CreAtts[cx]->attr_geomtype = t->tcb_key_atts[j]->geomtype;
		    CreAtts[cx]->attr_srid = t->tcb_key_atts[j]->srid;
		    CreAtts[cx]->attr_encflags = t->tcb_key_atts[j]->encflags;
		    CreAtts[cx]->attr_encwid = t->tcb_key_atts[j]->encwid;

		    /*
		    **  Build a attribute entry for the next record of the
		    **  index table for use by the access method specific
		    **  build routines.
		    */
		    STRUCT_ASSIGN_MACRO(*t->tcb_key_atts[j], m->mx_atts_ptr[ax]);
		    m->mx_atts_ptr[ax].offset = m->mx_width;
		    m->mx_atts_ptr[ax].key = 0;
		    m->mx_atts_ptr[ax].key_offset = m->mx_kwidth;

		    /* Add this att to list of atts from base table */
		    m->mx_b_key_atts[bx] = t->tcb_key_atts[j];

		    /* Include cluster key in compression */
		    m->mx_data_rac.att_ptrs[dx] = &m->mx_atts_ptr[ax];

		    /* Add to the row's width */
		    if ( m->mx_structure == TCB_BTREE )
			m->mx_kwidth += t->tcb_key_atts[j]->length;
		    m->mx_width += t->tcb_key_atts[j]->length;
		    m->mx_datawidth += t->tcb_key_atts[j]->length;

		    ax++;
		    bx++;
		    cx++;
		    dx++;
		}
	    }
	}

	/*
	** Finish up attribute stuff, noting that none
	** of those remaining have their origins in the TCB.
	**
	** "ax" is the number of mx_atts_ptr we've made
	** "bx" is the number of mx_b_key_atts we've made
	** "cx" is the number of CreAtts we've made
	** "dx" is the number of mx_data_atts we've made
	** "ix" is the number of mx_i_key_atts we've made
	** "mx" is the number of mx_cmp_list we've made
	*/

	if (m->mx_structure == TCB_RTREE)
        {
	    /* Fortunately, RTREES won't involve intervening Cluster Keys... */

            /*
            ** Add a comparision on Hilbert values in sort of the index records.
            */

            m->mx_cmp_list[0].cmp_offset = m->mx_width;
            m->mx_cmp_list[0].cmp_type = DB_BYTE_TYPE;
            m->mx_cmp_list[0].cmp_precision = 0;
            m->mx_cmp_list[0].cmp_length = m->mx_hilbertsize;
            m->mx_cmp_list[0].cmp_direction = 0;
            m->mx_cmp_list[0].cmp_collID = -1;
	    /* TIDP comes next, at [1]  */

	    STcopy((upcase ? "HILBERT" : "hilbert"),
		m->mx_atts_ptr[ax].attnmstr);
	    m->mx_atts_ptr[ax].attnmlen = STlength(m->mx_atts_ptr[ax].attnmstr);
	    m->mx_atts_ptr[ax].type = DB_BYTE_TYPE;
	    m->mx_atts_ptr[ax].length = m->mx_hilbertsize;
	    m->mx_atts_ptr[ax].precision = 0;
	    m->mx_atts_ptr[ax].flag = 0;
	    m->mx_atts_ptr[ax].offset = m->mx_width;
	    m->mx_atts_ptr[ax].key = index_cb->indxcb_kcount;
	    m->mx_atts_ptr[ax].key_offset = m->mx_width;
	    m->mx_atts_ptr[ax].ver_added = 0;
	    m->mx_atts_ptr[ax].ver_dropped = 0;
	    m->mx_atts_ptr[ax].val_from = 0;
	    m->mx_atts_ptr[ax].collID = -1;
	    m->mx_atts_ptr[ax].geomtype = -1;
	    m->mx_atts_ptr[ax].srid = -1;
	    m->mx_data_rac.att_ptrs[dx] = &(m->mx_atts_ptr[ax]);
	    m->mx_i_key_atts[ix] = &(m->mx_atts_ptr[ax]);
	    m->mx_kwidth += m->mx_hilbertsize;
	    m->mx_width += m->mx_hilbertsize;
	    m->mx_datawidth += m->mx_hilbertsize;
	    m->mx_att_map[ax] = ix+1;
	    m->mx_atts_ptr[ax].encflags = 0;
	    m->mx_atts_ptr[ax].encwid = 0;

	    MEmove(7, (upcase ? "HILBERT" : "hilbert"), ' ',
		sizeof(CreAtts[cx]->attr_name),
		(char *)&CreAtts[cx]->attr_name);
	    CreAtts[cx]->attr_type = DB_BYTE_TYPE;
	    CreAtts[cx]->attr_size = m->mx_hilbertsize;
	    CreAtts[cx]->attr_precision = 0;
	    CreAtts[cx]->attr_flags_mask = 0;
	    SET_CANON_DEF_ID( CreAtts[cx]->attr_defaultID,
			DB_DEF_NOT_DEFAULT);
	    CreAtts[cx]->attr_collID = -1;
	    CreAtts[cx]->attr_geomtype = -1;
	    CreAtts[cx]->attr_srid = -1;
	    CreAtts[cx]->attr_encflags = 0;
	    CreAtts[cx]->attr_encwid = 0;

	    ax++;
	    cx++;
	    dx++;
	    ix++;
        }



        /*
        ** Do all the TIDP attribute stuff, always the last attribute:
        ** For gateway indexes, no TIDP column.
	**
        ** schang: gw merge 04/16/92
        ** OPF expects secondary indexes to have a tidp column even if
        ** gateways don't use them.  15-dec-91 (linda)
        */

        /*
        ** Add a comparision on TIDP for the sort of the index records.
        */
	if ( m->mx_structure == TCB_RTREE )
	    i = 1;	/* Use reserved second slot, after HILBERT */
	else
	    i = mx++;	/* Make a new one after the last */
        m->mx_cmp_list[i].cmp_offset = m->mx_width;
        m->mx_cmp_list[i].cmp_type = DB_INT_TYPE;
        m->mx_cmp_list[i].cmp_precision = 0;
        m->mx_cmp_list[i].cmp_length = m->mx_tidsize;
        m->mx_cmp_list[i].cmp_direction = 0;
        m->mx_cmp_list[i].cmp_collID = -1;

        /*
        **  Add a entry for TIDP to the attribute list parameter for
        **  for create table.
        */

        MEmove(4, (upcase ? "TIDP" : "tidp"), ' ',
               sizeof(CreAtts[cx]->attr_name),
               (char *)&CreAtts[cx]->attr_name);
        CreAtts[cx]->attr_type = DB_INT_TYPE;
        CreAtts[cx]->attr_size = m->mx_tidsize;
        CreAtts[cx]->attr_precision = 0;
        CreAtts[cx]->attr_flags_mask = 0;
        SET_CANON_DEF_ID( CreAtts[cx]->attr_defaultID, DB_DEF_NOT_DEFAULT );
        CreAtts[cx]->attr_collID = -1;
        CreAtts[cx]->attr_geomtype = -1;
        CreAtts[cx]->attr_srid = -1;
        CreAtts[cx]->attr_encflags = 0;
        CreAtts[cx]->attr_encwid = 0;

        /*
        ** Add the TIDP (last) to the attribute list.
	*/

        STcopy((upcase ? "TIDP" : "tidp"), m->mx_atts_ptr[ax].attnmstr);
	m->mx_atts_ptr[ax].attnmlen = STlength(m->mx_atts_ptr[ax].attnmstr);
        m->mx_atts_ptr[ax].type = DB_INT_TYPE;
        m->mx_atts_ptr[ax].length = m->mx_tidsize;
        m->mx_atts_ptr[ax].precision = 0;
        m->mx_atts_ptr[ax].flag = 0;
        m->mx_atts_ptr[ax].offset = m->mx_width;
        m->mx_atts_ptr[ax].key = 0;
        m->mx_atts_ptr[ax].key_offset = m->mx_kwidth;
	m->mx_atts_ptr[ax].ver_added = 0;
	m->mx_atts_ptr[ax].ver_dropped = 0;
	m->mx_atts_ptr[ax].val_from = 0;
	m->mx_atts_ptr[ax].collID = -1;
	m->mx_atts_ptr[ax].geomtype = -1;
	m->mx_atts_ptr[ax].srid = -1;
        m->mx_data_rac.att_ptrs[dx] = &m->mx_atts_ptr[ax];
	m->mx_atts_ptr[ax].encflags = 0;
	m->mx_atts_ptr[ax].encwid = 0;

	/*
	**  HASH indices never include TIDP as a key.
	**  Non-unique BTREE and ISAM indices include TIDP as a key field.
	**
	**  Unique indices do not include TIDP in key,
	**  nor do indexes on Clustered tables.
	*/
	if ( m->mx_structure != TCB_HASH )
	{
	    if ( !index_cb->indxcb_unique && !(t->tcb_rel.relstat & TCB_CLUSTERED) )
	    {
		m->mx_atts_ptr[ax].key = ix+1;
		m->mx_i_key_atts[ix] = &m->mx_atts_ptr[ax];
		m->mx_att_map[ax] = ix+1;
		m->mx_kwidth += m->mx_tidsize;
		OnlyKeyLen += m->mx_tidsize;
		ix++;
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

	ax++;
        cx++;
	dx++;

        m->mx_width += m->mx_tidsize;
        m->mx_datawidth += m->mx_tidsize;

	/* Done with TIDP... */

        if (m->mx_structure == TCB_HASH)
        {
            /* Insert (first) comparison on hash bucket.  Has to be done after the
	    ** final value of mx_width is determined!
	    */
            m->mx_cmp_list[0].cmp_offset = m->mx_width;
            m->mx_cmp_list[0].cmp_type = DB_INT_TYPE;
            m->mx_cmp_list[0].cmp_precision = 0;
            m->mx_cmp_list[0].cmp_length = sizeof(i4);
            m->mx_cmp_list[0].cmp_direction = 0;
            m->mx_cmp_list[0].cmp_collID = -1;
        }

	/* Done with attribute construction, update mx counts */
	m->mx_ab_count = bx;
	m->mx_ai_count = ix;
	m->mx_c_count = mx;
	*NumCreAtts = cx;
	m->mx_data_rac.att_count = dx;

	if (m->mx_structure == TCB_BTREE)
	{
	    /* Btree index leaf is same as (index) data. */
	    m->mx_leaf_rac.att_ptrs = m->mx_data_rac.att_ptrs;
	    m->mx_leaf_rac.att_count = m->mx_data_rac.att_count;
	}
	/* Index entry is just the keys, however note that BTREE with
	** non-key columns included will redo this att list to point to
	** an adjusted TIDP att copy.
	** (BTREE will do this at btree build begin time.)
	*/
	m->mx_index_rac.att_ptrs = m->mx_i_key_atts;
	m->mx_index_rac.att_count = m->mx_ai_count;

	/* Check that what we've constructed will fit: */

	/* choose page type (only recovery and sysmod provide page_type) */
	if (m->mx_page_type == TCB_PG_INVALID)
	{
	    if ( dm1c_getpgtype(m->mx_page_size, 
				(t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG))
				    ? DM1C_CREATE_CATALOG
				    : DM1C_CREATE_INDEX,
				m->mx_width, &m->mx_page_type) )
	    {
		SETDBERR(dberr, 0, E_DM0103_TUPLE_TOO_WIDE);
		status = E_DB_ERROR;
		break;
	    }
	}

        /*
        **  23-sep-88 (sandyh)
        **  Check that key is small enough for structure that put limits on key
        **  length. If modifying to compressed also check that the length of
        **  the records will not exceeded MAX_TUP in worst case compresssion.
        **  Worse case occurs when each C field gets an extra byte in
        **  compressed form and each CHAR type gets an extra two bytes.
        */
	if (m->mx_width + dm1c_compexpand(m->mx_data_rac.compression_type,
			m->mx_data_rac.att_ptrs, m->mx_data_rac.att_count) >
		dm2u_maxreclen(m->mx_page_type, m->mx_page_size))
	{
	    SETDBERR(dberr, 0, E_DM0103_TUPLE_TOO_WIDE);
	    status = E_DB_ERROR;
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
		status = E_DB_ERROR;
		break;
	    }	 
	    if (m->mx_kwidth > 
		(dm2u_maxreclen(m->mx_page_type, m->mx_page_size) + 
		    DM_VPT_SIZEOF_LINEID_MACRO(m->mx_page_type) - 
			2 * DM_VPT_SIZEOF_LINEID_MACRO(m->mx_page_type)) / 2)
	    {
		SETDBERR(dberr, m->mx_kwidth, E_DM010F_ISAM_BAD_KEY_LENGTH);
		status = E_DB_ERROR;
		break;
	    }	 
	}
	if (m->mx_structure == TCB_BTREE)
	{
	    /* Tidp is already part of kwidth, but if the key is compressed
	    ** we also need to allow for worst-case key expansion.
	    */
	    i4	adj_leaflen;
	    i4  max_leaflen;

	    adj_leaflen = m->mx_kwidth;
	    if (index_cb->indxcb_index_compressed)
		adj_leaflen += dm1c_compexpand(TCB_C_STD_OLD,
				m->mx_data_rac.att_ptrs,
				m->mx_data_rac.att_count);

/* --> comment out clustered index max_leaflen
	    max_leaflen = DM1B_VPT_MAXLEAFLEN_MACRO(
					m->mx_page_type, m->mx_page_size, 
					m->mx_ab_count + 1,
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
			m->mx_ab_count + 1,
			OnlyKeyLen,
			max_leaflen,
			DM1B_KEYLENGTH);
		SETDBERR(dberr, adj_leaflen, E_DM007D_BTREE_BAD_KEY_LENGTH);
		index_cb->indxcb_maxklen = max_leaflen;
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Make sure kperleaf >= 2
	    ** adj_leaflen <= max_leaflen should always guarantee kperleaf >= 2
	    ** mx_ab_count = count of base table columns (+1 for tidp)
	    */
	    if (m->mx_page_type != TCB_PG_V1 && 
			m->mx_page_size == DM_COMPAT_PGSIZE)
	    {
		if (dm1b_kperpage(m->mx_page_type, m->mx_page_size,
				DMF_T_VERSION, m->mx_ab_count + 1,
				adj_leaflen, DM1B_PLEAF, m->mx_tidsize, 0) < 2)
		{
		    /* Need to use different page type or page size */
		    SETDBERR(dberr, adj_leaflen, E_DM007D_BTREE_BAD_KEY_LENGTH);
		    index_cb->indxcb_maxklen = max_leaflen;
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

        /*
        ** For hash tables, calculate the number of hash buckets to allocate.
        */
        if (m->mx_structure == TCB_HASH)
        {
	    /* Rollforward MUST use logged number of buckets */
	    if (index_cb->indxcb_dcb->dcb_status & DCB_S_ROLLFORWARD)
		m->mx_buckets = index_cb->indxcb_rfp.rfp_hash_buckets;
	    else
		m->mx_buckets = dm2u_calc_hash_buckets(m->mx_page_type,
				   m->mx_page_size,
				   m->mx_dfill, m->mx_width,
				   m->mx_reltups, m->mx_minpages, m->mx_maxpages);
        }
    } while (FALSE);

    return(status);
}

/*
** Name: dm2uCreateTempIndexTCB	    - Create an Index TCB on a Temp Table
**
** Description:
**	This routine is called by dm2u_index and dm2u_pindex to create a 
**	temporary table index TCB suitable for loading.
**
** Inputs:
**	BaseTCB				The Base TempTable TCB.
**	m				The ready-to-roll DM2U_MXCB describing
**					the index.
**	idx				DM2U_INDEX_CB with the remaining bits.
**	AttCount			Number of attributes in the index.
**	AttList				Pointer array to those DB_ATTS.
**
** Outputs:
**	idx
**		indxcb_idx_id		Assigned db_tab_id of index.
**	m
**		mx_table_id		Assigned db_tab_id of index.
**		mx_tpcb_next
**			tpcb_tabid	Assigned db_tab_id of index.
**	dberr
**          .err_code	        	The reason for an error return status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    Assigns a database-unique table_id to the index, and holds
**	    an LK_X Table Lock on that value, which will be released
**	    when the Index is destroyed either implicitly by DROP INDEX,
**	    DROP TABLE, or end of session.
**
** History:
**	15-Aug-2006 (jonj)
**	    Invented.
**	7-Aug-2008 (kibro01) b120592
**	    Added extra flag to dm2t_temp_build_tcb
*/
DB_STATUS
dm2uCreateTempIndexTCB(
DMP_TCB		*BaseTCB,
DM2U_MXCB	*m,
DM2U_INDEX_CB	*idx,
i4		AttCount,
DMF_ATTR_ENTRY	**AttList,
DB_ERROR	*dberr)
{
    DMP_DCB		*dcb;
    DML_XCB		*xcb;
    DMP_TCB		*IxTCB;
    DB_STATUS		status;
    DMP_RELATION	relrecord;
    DM2T_KEY_ENTRY	DM2TKey[DB_MAXIXATTS];
    DM2T_KEY_ENTRY	*DM2TKeys[DB_MAXIXATTS];
    i4			i;
    DM_OBJECT		*locs_mem;
    i4			alen;
    i4			attnmsz = 0;

    CLRDBERR(dberr);

    dcb = idx->indxcb_dcb;
    xcb = idx->indxcb_xcb;

    /* Convert DB_ATTS keys to DM2T_KEY_ENTRY format for dm2t_temp_build_tcb */
    for ( i = 0; i < m->mx_ai_count; i++ )
    {
	DM2TKeys[i] = &DM2TKey[i];
	MEmove(m->mx_i_key_atts[i]->attnmlen, m->mx_i_key_atts[i]->attnmstr,
		' ', DB_ATT_MAXNAME, DM2TKeys[i]->key_attr_name.db_att_name);
	if ( m->mx_i_key_atts[i]->flag & ATT_DESCENDING )
	    DM2TKeys[i]->key_order = DM2T_DESCENDING;
	else
	    DM2TKeys[i]->key_order = DM2T_ASCENDING;
    }
	

    /*
    ** Start with a zeroed relation record. dm2t_alloc_tcb() expects
    ** an authentic-looking relation.
    */
    MEfill(sizeof(relrecord), 0, (char *)&relrecord);

    idx->indxcb_idx_id->db_tab_base = 0;
    idx->indxcb_idx_id->db_tab_index = 0;

    /*
    ** Resolve location information, then
    ** get a unique table_id for this index.
    **
    ** The form of the table_id is (BaseTabId,IndexTabId), and
    ** it's locked LK_TABLE with mode = LK_X.
    */
    if ( (status = dm2uGetTempTableLocs(dcb,
				       xcb->xcb_scb_ptr,
				       &idx->indxcb_location,
				       &idx->indxcb_l_count,
				       &locs_mem,
				       dberr)) == E_DB_OK &&

         (status = dm2uGetTempTableId(dcb,
				      xcb->xcb_scb_ptr->scb_lock_list, 
				      &BaseTCB->tcb_rel.reltid,
				      TRUE,
				      &relrecord.reltid,
				      dberr)) == E_DB_OK )
    {
	/* 
	** Everything is set now, build a relation record for the
	** temporary table index.
	*/
	for ( i = 1; i <= AttCount; i++)
	{
	    for (alen = DB_ATT_MAXNAME;  
		AttList[i-1]->attr_name.db_att_name[alen-1] == ' ' 
			&& alen >= 1; alen--);
	    attnmsz += alen;
	}

	MEcopy((char*)idx->indxcb_index_name, sizeof(DB_TAB_NAME), 
		(char*)&relrecord.relid);

	/* Session Tables/Indexes are owned by "$Sess000000..." */
	MEcopy((char*)&BaseTCB->tcb_rel.relowner, sizeof(DB_OWN_NAME),
		 (char *)&relrecord.relowner);
	
	MEcopy ((char*)&idx->indxcb_location[0], sizeof(DB_LOC_NAME), 
		(char*)&relrecord.relloc);
	relrecord.relloccount = idx->indxcb_l_count;

	relrecord.relatts = AttCount;
	relrecord.relattnametot = attnmsz;
	relrecord.relwid = m->mx_width;	        
	relrecord.reltotwid = m->mx_width;	        
	relrecord.reldatawid = m->mx_width;	        
	relrecord.reltotdatawid = m->mx_width;	        
	relrecord.relkeys = m->mx_ai_count;
	relrecord.relspec = m->mx_structure;
	relrecord.relpages = 3;
	relrecord.relprim = 1;
	relrecord.relmain = 1;
	relrecord.relmin = m->mx_minpages;
	relrecord.relmax = m->mx_maxpages;
	relrecord.relfhdr = DM_TBL_INVALID_FHDR_PAGENO;
	relrecord.relcmptlvl = DMF_T_VERSION;
	TMget_stamp((TM_STAMP *)&relrecord.relstamp12);
	relrecord.relcreate = TMsecs();
	relrecord.relmoddate = relrecord.relcreate;

	relrecord.relstat = TCB_INDEX;
	if ( m->mx_data_rac.compression_type != TCB_C_NONE )
	    relrecord.relstat |= TCB_COMPRESSED;
	relrecord.relcomptype = m->mx_data_rac.compression_type;

	if ( m->mx_duplicates )
	    relrecord.relstat |= TCB_DUPLICATES;	    
	if ( BaseTCB->tcb_rel.relstat & TCB_SYS_MAINTAINED )
	    relrecord.relstat |= TCB_SYS_MAINTAINED;
	if ( m->mx_unique )
	    relrecord.relstat |= TCB_UNIQUE;
	if ( m->mx_index_comp )
	    relrecord.relstat |= TCB_INDEX_COMP;
	
	relrecord.relstat2 = m->mx_new_relstat2 | TCB_SESSION_TEMP;

	relrecord.reldfill = m->mx_dfill;
	relrecord.relallocation = m->mx_allocation;
	relrecord.relextend = m->mx_extend;
	relrecord.relpgtype = m->mx_page_type;
	relrecord.relpgsize = m->mx_page_size;
	relrecord.reltcpri  = m->mx_tbl_pri;
	
	/* Build a TCB for the temporary table index */

	/*
	** Pass BaseTCB to dm2t.
	**
	** This tells it we're building an index TCB
	*/
	IxTCB = BaseTCB;

	status = dm2t_temp_build_tcb(dcb, &relrecord.reltid, 
				     &xcb->xcb_tran_id,
				     xcb->xcb_scb_ptr->scb_lock_list, 
				     xcb->xcb_log_id, &IxTCB, &relrecord,
				     AttList, AttCount,
				     DM2TKeys, m->mx_ai_count,
				     DM2T_NOLOAD,
				     0,		/* "recovery" */
				     (i4)0, 	/* "num_alloc" */
				     idx->indxcb_location, idx->indxcb_l_count,
				     m->mx_idom_map,
				     dberr,
				     FALSE);
	if ( status == E_DB_OK )
	{
	    /* Return Index table id to caller */
	    idx->indxcb_idx_id->db_tab_base = relrecord.reltid.db_tab_base;
	    idx->indxcb_idx_id->db_tab_index = relrecord.reltid.db_tab_index;

	    /* ..stick it in the MXCB */
	    m->mx_table_id.db_tab_base = relrecord.reltid.db_tab_base;
	    m->mx_table_id.db_tab_index = relrecord.reltid.db_tab_index;

	    /* ...and to (only) target's TPCB */
	    m->mx_tpcb_next->tpcb_tabid.db_tab_base = relrecord.reltid.db_tab_base;
	    m->mx_tpcb_next->tpcb_tabid.db_tab_index = relrecord.reltid.db_tab_index;
	    /* Point at the base table's XCCB for savepoint ID updating */
	    IxTCB->tcb_temp_xccb = IxTCB->tcb_parent_tcb_ptr->tcb_temp_xccb;
	}
    }

    /* If dm2uGetTempTableLocs() allocated memory, release it */ 
    if ( locs_mem )
	dm0m_deallocate((DM_OBJECT**)&locs_mem);

    return(status);
}
