/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <st.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm1c.h>
#include    <dm1cn.h>
#include    <dm1p.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dm0c.h>
#include    <dm0m.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0s.h>
#include    <dmm.h>
#include    <dudbms.h>
#include    <cv.h>

/**
**
** Name: DMMCREATE.C - Functions needed to create a minimal database.
**
** Description:
**      This file contains the functions necessary to create a minimal
**	database populated with only the core system catalogs.  This
**	function is used by CREATEDB utility to create a database.  The
**	bulk of the database creation occurs using vanilla SQL.
**    
**      dmm_create()       -  Routine to perfrom the create operation.
**
** History:
**      20-mar-89 (derek) 
**          Created for Orange.
**	20-nov-1990 (bryanp)
**	    Clear the cluster node info when creating database.
**	25-sep-1991 (mikem) integrated following change: 5-aug-1991 (bryanp)
**	    Log additional error information before returning E_DM011B.
**	17-oct-1991 (rogerk)
**	    Changed iirel_idx.tidp to be an i4 instead of c4.
**	    The iiattribute table for iirel_idx specified ATT_CHA
**	    for the tidp column instead of ATT_INT.  This caused the
**	    system to interpret this field as a 4 byte string rather
**	    than a numeric value.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	18-jul-92 (daveb)
**	    Correct offsets for relgwid and relgwother, which were
**	    two short of where they belonged.  Relloccount is an i4
**	    at offset 156, so relgwid better be at 160, not 158 where
**	    it had been placed in the template here.
**	    This may require recreation of all databases, again.  Sigh.
**      05-jun-92 (kwatts)
**          MPF project changes, adding two new iirelation fields and using
**          a local set of accessors to set up pages.
**	29-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  added relstat2 column to iirelation.  Added
**	    attdefid1 and attdefid2 to iiattribute.
**      28-aug-92 (kwatts)
**          Replaced all uses of 'sizeof(DMP_INDEX)' by new DMP_INDEX_SIZE
**          define. SOme systems (eg Sun4, DRS6000) pad the 'sizeof' and so
**          do not give the tuple size which is what is required.
**      29-August-1992 (rmuth)
**          Removed the on-the-fly 6.4->6.5 table conversion code. This means
**          we now have to create a FHDR/FMAP for the core catalogs as the
**          original method created 6.4 tables and used the on-the-fly
**          conversion code upgrade them to 6.5 tables.
**	29-sep-1992 (ananth)
**	    Expose relstat2 field of DMP_RELATION.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	17-sep-1992 (jnash)
**	    6.5 Recovery Project.
**	     -- Substitute TCB_PG_SYSCAT for TCB_PG_DEFAULT in the 
**		static initialization of system catalog pages.
**	     -- Use PG_SYSCAT (system catalog) page accessors in build_file.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Initialized fcb allocation fields in newly
**	    created files so that later dm2f_alloc_calls will work properly.
**	30-October-1992 (rmuth)
**	    - Prototyping DI functions showed complier warnings.
**	    - Reduce DIalloc calls while building the core tables.
**	    - dmm_add_SMS() - Pass a DM1P_BUILD_SMS_CB structure to 
**	      dm1p_build_SMS()
**	05-nov-92 (jrb)
**	    Create WORK directory at database creation time.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency, increase constants to DB_MAXNAME
**	    config file change made by Ananth
**	22-oct-1992 (bryanp)
**	    Improve error logging.
**	3-nov-1992 (rickh)
**	    General cleanup.  Remove the ORANGE code.  Replace hardcoded
**	    table ids with some clearer symbols.  Replace with UNINITIALIZED
**	    the attribute count field in each IIRELATION tuple as well as
**	    the attribute number field in IIATTRIBUTE.  Replace the offset
**	    field in IIATTRIBUTE tuples with a macro that computes the offset
**	    at compile time.  At DMF startup time, compute the UNINITIALIZED
**	    fields and space pad object names to DB_MAXNAME characters.
**	    Added dmm_init_catalog_templates( ).
**	12-nov-1992 (bryanp)
**	    In build_file, if the overflow chain ends after exactly filling the
**	    last page, ensure that we terminate the overflow chain correctly
**	    rather than pointing to a (non-existent) next overflow page.
**	12-nov-1992 (bryanp)
**	    Extended attdefid1 and attdefid2 to 32 bytes to match maxname chgs.
**	30-November-1992 (rmuth)
**	    Correct an Integration problem.
**	14-dec-1992 (rogerk)
**	    Changed core iirelation rows to set relloccount to 1 rather than 0.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-92 (jrb)
**	    Removed misc_flag and DM_M_DIREXISTS stuff because it's not used.
**	    Also, allowed work directory to already exist.
**	9-feb-92 (rickh)
**	    Changed GLOBALCONSTDEFs to GLOBALDEFs.
**	16-feb-1993 (rogerk)
**	    Changed core iiidex row for iirel_idx to set sequence number
**	    to zeo rather than one.  Fixed compile warnings: Removed & from
**	    the front of core catalog array references.
**	15-mar-1993 (rogerk)
**	    Fix compile warnings.
**	30-feb-1993 (rmuth)
**	    Shorten DM92FC.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-apr-1993 (jnash)
**	    As part of ALTERDB changes, change initial values of journal 
**	    and dump file characteristics, as follows:
**		jnl_bksz/dmp_bksz (was 8192, now 16384), 
**		jnl_maxcnt/dmp_maxcnt (was 1024, now 512), 
**	   	jnl_blkcnt/dmp_blkcnt (was 256, now 4), 
**	    Initial values changed because real file space now allocated at 
**	    file create time (used to be true on VMS, now also true on UNIX).  
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Introduced upper-case version of templates.
**	    Add support for upper case regular identifiers in create_db().
**	    Initialize upper-case versions of templates.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (robf)
**	    Removed last ORANGE remnant, added relsecid entry.
**	    Improved low-level error handling.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-1993 (rmuth)
**	    VMS could not calculate the number of elements in 
**	    DM_core_relations from another module. Add a globaldef
**	    DM_sizeof_core_rel to hold this value.
**	18-oct-1993 (kwatts) Bug # 45676
**	    fill in iirelation.relcreate for the core catalog entries using 
**	    TMsecs().
**	20-oct-1993 (rmuth)
**	    - Fixed problem initialising uppercase templates, where we
**	      running over the end-of-array.
**	    - Fix incompatble argument errors on STmove calls.
**	09-nov-93 (rblumer)
**	    DELIM_IDENT:  changed dmm_create to put the correctly-cased
**	    database owner name into the CONFIG file.
**	14-Jan-1993 (rmuth)
**	    b58109, b58110, 
**	    b58111 - Improve error handling in dmm_setup_tables_relfhdr().
**	    b58108, 
**          b58107 - Improve error handling in build_file().
**	    b58104 - Remove list_func() as never called.
**	    b58103 - dmm_add_create() - Remove redundent check of status
**	             as never get reach there.
**	    b58106 - create_db() improve error handling.
**	18-apr-1994 (jnash)
**	    fsync project.  Remove DIforce() call.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	23-may-94 (robf)
**          Bug 63535 - core catalogs are not marked as TCB_SECURE, so
**	    allowing unexpected updates by  non-security users. These are
**	    marked as secure since they contain basic table information,
**	    including table security label.
**	27-apr-1995 (cohmi01)
**	   dmm_add_SMS() - Initialize loc_status field, contained junk.
**	06-mar-1996 (stial01 for bryanp)
**	    Add relpgsize to iirelation to hold a table's page size.
**	    Set SMS_page_size in the DM1P_BUILD_SMS_CB.
**	    Verify DMP_NUM_IIRELATION_ATTS.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dmpp_get rather than dmpp_get_offset.
**      20-may-1996 (ramra01)
**          Added DMPP_TUPLE_INFO to get/load accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      15-jul-1996 (ramra01 for bryanp)
**          Add relversion to iirelation to hold table's metadata version.
**          Add reltotwid to iirelation to hold table's row length which is
**		the sum tot of active + dropped column widths. This will
**		enable chk for tup width with add column and help from tm.
**          Add attintl_id, attver_added, attver_dropped, attval_from, and
**              attfree to iiattribute to hold Alter Table information.
**          In dmm_init_catalog_templates, initialize the new iiattribute
**              columns appropriately.
**          Pass 0 as the current table version to dmpp_load when building
**              core system catalogs.
**	23-sep-1996 (canor01)
**	    Move global data definitions to dmmdata.c.
**	02-Nov-1996 (jenjo02)
**	    Collapsed dm0s_minit(), dm0s_name() into single call.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**	08-may-1997 (wonst02)
** 	    If getting access to existing readonly database, skip the create_db.
**	13-may-1997 (wonst02)
** 	    Support readonly databases.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**	21-oct-1999 (nanpr01)
**	    Do not pass the tuple header structure when no required. This is
**	    a CPU optimization.
**      07-mar-2000 (gupsh01)
**          Modified dmm_add_create to include error checking for creating 
**        readonly database.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	02-Oct-2002 (hanch04)
**	    If runing against a 64-bit server, then dbservice should be set 
**	    to LP64.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**          iirelation changes for 256k rows, relwid, reltotwid i2->i4
**          iiattribute changes for 256k rows, attoff i2->i4
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**      04-feb-2010 (stial01)
**          dmm_init_catalog_templates() Init ucore from core
**	26-Mar-2010 (toumi01) SIR 122403
**	    For encryption project add attencflags, attencwid.
**      01-apr-2010 (stial01)
**          Changes for Long IDs Init varchar lengths in core catalogs
**          Use more readable variable names
**	23-apr-2010 (wanfr01)
**	    include cv.h for function definitions
**      29-Apr-2010 
**          Create core catalogs having long ids with compression=(data)
**	06-May-2010
**	    Remove unsued variable which was a typo
**      14-May-2010 (stial01)
**          dmm_init_catalog_templates, init relattnametot, init attDefaultID
**          add consistency checks, more error internal handling
*/

/* core table ids */

#define	NOT_AN_INDEX		0
#define	IIRELATION_BASE_ID	1
#define	IIREL_IDX_INDEX_ID	2
#define	IIATTRIBUTE_BASE_ID	3
#define	IIINDEX_BASE_ID		4

#define	IIRELATION_TABLE_ID	{ IIRELATION_BASE_ID, NOT_AN_INDEX }
#define	IIREL_IDX_TABLE_ID	{ IIRELATION_BASE_ID, IIREL_IDX_INDEX_ID  }
#define	IIATTRIBUTE_TABLE_ID	{ IIATTRIBUTE_BASE_ID, NOT_AN_INDEX }
#define	IIINDEX_TABLE_ID	{ IIINDEX_BASE_ID, NOT_AN_INDEX }

/* covers for CL_OFFSETOF */

#define	REL_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_RELATION, fieldName ) )
#define	REL_IDX_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_RINDEX, fieldName ) )
#define	ATT_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_ATTRIBUTE, fieldName ) )
#define	IIINDEX_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_INDEX, fieldName ) )

/* uninitialized fields should identify themselves to the developer */

#define	DUMMY_ATTRIBUTE_COUNT	-1
#define	DUMMY_ATTRIBUTE_NUMBER	-1
#define	DUMMY_TUPLE_COUNT	-1

/*
**  Forward and/or External function references for static functions.
*/

static DB_STATUS 	create_db(
				    DML_XCB		*xcb,
				    DMP_MISC		*cnf,
				    DB_DB_NAME		*db_name,
				    i4		db_id,
				    i4		db_access,
				    i4		db_service,
				    DB_LOC_NAME		*location_name,
				    i4		device_length,
				    DM_PATH		*device_name,
				    i4		path_length,
				    DM_PATH		*path_name,
				    i4			page_type,
				    i4			page_size,
				    DB_ERROR		*dberr);

static DB_STATUS 	build_file(
				    DB_DB_NAME		*db_name,
				    i4			db_service,
				    DMP_RELATION	*rel,
				    DMP_ROWACCESS	*rac,
				    DM_PATH		*path_name,
				    i4			path_length,
				    char		*records,
				    DB_ERROR		*dberr);

	

static DB_STATUS 	dmm_add_SMS(
				    DI_IO		*di_context,
				    DB_LOC_NAME		*loc_name,
				    DM_PATH		*path_name,
				    i4 		path_length,
				    i4			page_type,
				    i4			page_size,
				    DM_FILE		*file_name,
				    DM_PAGENO		*next_pageno,
				    DB_TAB_NAME		*tab_name,
				    DB_DB_NAME		*dbname,
				    DM_PAGENO		*fhdr_pageno,
				    i4		*pages_allocated,
    				    DMPP_ACC_PLV	*plv,
				    DB_ERROR		*dberr);

static DB_STATUS 	dmm_setup_tables_relfhdr(
				    DB_TAB_ID		*table_id,
				    i4		fhdr_pageno,
				    DM_PATH 		*path_name,
				    i4		path_length,
				    i4			page_type,
				    i4			page_size,
    				    DMPP_ACC_PLV	*plv,
				    DB_TAB_NAME	 	*tab_name,
				    DB_DB_NAME	 	*dbname,
				    DB_ERROR		*dberr);

/*{
** Name: dmm_create - Creates the minimal database files and directory.
**
**  INTERNAL DMF call format:      status = dmm_modify(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_CREATE_DB, &dmm_cb);
**
** Description:
**      The dmm_create function handles the creation of the default location 
**      for a database and the creation of the core or template files required for
**      a minimal database.  Once the directory and files exist, the database can 
**      be openned and accessed using SQL commands to populate the remaining 
**      locations and system catalogs required for a complete INGRES database.  
**
**      The core or template files contain just enough information to define 
**      the core system catalogs needed to define all other objects within the database.
**
** Inputs:
**      dmu_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**	    .dmm_flags_mask		    Must be zero.
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
** Output:
**      dmm_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**                                          E_DM010C_TRAN_ABORTED
**
**         .error.err_data                  Set to attribute in error by 
**                                          returning index into attribute list.
**
** History:
**	20-mar-1989 (derek)
**	    Created for ORANGE.
**	20-nov-1990 (bryanp)
**	    Clear the cluster node info when creating database.
**	17-oct-1991 (rogerk)
**	    Changed iirel_idx.tidp to be an i4 instead of c4.
**	    The iiattribute table for iirel_idx specified ATT_CHA
**	    for the tidp column instead of ATT_INT.  This caused the
**	    system to interpret this field as a 4 byte string rather
**	    than a numeric value.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	29-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  added relstat2 column to iirelation.  Added
**	    attdefid1 and attdefid2 to iiattribute.
**	29-sep-1992 (ananth)
**	    Expose relstat2 field of DMP_RELATION.
**	    Add attribute record for relstat2 in DM_core_attributes.
**	    Fix DM_core_relations.
**	04-nov-92 (rickh)
**	    General cleanup.  Removed ORANGE ifdefs, eliminated hard
**	    to maintain magic numbers.
**	14-dec-1992 (rogerk)
**	    Changed core iirelation rows to set relloccount to 1 rather than 0.
**	16-feb-1993 (rogerk)
**	    Changed core iiidex row for iirel_idx to set sequence number
**	    to zeo rather than one.
**	26-apr-1993 (jnash)
**	    As part of ALTERDB changes, change initial values of journal 
**	    and dump file characteristics, as follows:
**		jnl_bksz/dmp_bksz (was 8192, now 16384), 
**		jnl_maxcnt/dmp_maxcnt (was 1024, now 512), 
**	   	jnl_blkcnt/dmp_blkcnt (was 256, now 4), 
**	    Initial values changed because real file space now allocated at 
**	    file create time (used to be true on VMS, now also true on UNIX).  
**	22-nov-1993 (jnash)
**	    B56095.  Initialize config file jnl_first_jnl_seq entry.
*/


/*
** DM_core_relations contains IIRELATION records for core catalogs.
** DM_ucore_relations contains IIRELATION records with upper-case identifiers.
** At server startup we space pad names to DB_MAXNAME positions
** and fill in number of attributes per relation.
*/
GLOBALREF DMP_RELATION DM_core_relations[];
GLOBALREF DMP_RELATION DM_ucore_relations[];

/*
** DM_core_attributes contains IIATTRIBUTE records for core catalogs.
** DM_ucore_attributes contains IIATTRIBUTE records with upper-case identifiers.
** At server startup we space pad names to DB_MAXNAME positions
*/
GLOBALREF DMP_ATTRIBUTE DM_core_attributes[];
GLOBALREF DMP_ATTRIBUTE DM_ucore_attributes[];
GLOBALREF i4 DM_core_att_cnt;

/*
**  DM_core_rindex contains IIREL_IDX records for core catalogs.
**  DM_ucore_rindex contains IIREL_IDX records with upper-case identifiers.
**  At server startup we space pad names to DB_MAXNAME positions
*/
GLOBALREF DMP_RINDEX DM_core_rindex[];
GLOBALREF DMP_RINDEX DM_ucore_rindex[];

/*
**  DM_core_index contains IIINDEX records for core catalogs.
**  There is no upper-case version because IIINDEX does not have identifiers.
*/
GLOBALREF DMP_INDEX DM_core_index[];
GLOBALREF i4 DM_core_index_cnt;


/*
** Name: dmm_create		- create the CONFIG file for a database
**
** History:
**	09-nov-93 (rblumer)
**	    DELIM_IDENT:
**	    In order to put the database owner name into the CONFIG file in the
**	    correct case for the database BEING CREATED (instead of the correct
**	    case for the iidbdb), call dmm_add_create with the passed-in
**	    dmm_owner name instead of with the scb_user for the session.
**      23-dec-98 (stial01)
**          Init catalog page sizes to DM_COMPAT_PGSIZE
**	19-mar-2001 (stephenb)
**	    Add unicode collation table parameter to dmm_add_create
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
*/
DB_STATUS
dmm_create(
DMM_CB    *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;
    DB_STATUS	    status;
    i4		    *err_code = &dmm->error.err_code;

    CLRDBERR(&dmm->error);

    /*	Check arguments. */

    /*  Check Transaction Id. */

    xcb = (DML_XCB *)dmm->dmm_tran_id;
    if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
    {
	scb = xcb->xcb_scb_ptr;

	/* Check for external interrupts */
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, err_code);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
	        SETDBERR(&dmm->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
	        SETDBERR(&dmm->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
	        SETDBERR(&dmm->error, 0, E_DM0064_USER_ABORT);
	}
    }
    else
	SETDBERR(&dmm->error, 0, E_DM003B_BAD_TRAN_ID);

    /*  Check Location. */

    if ( dmm->error.err_code == 0 )
    {
	if (dmm->dmm_db_location.data_in_size == 0 ||
	    dmm->dmm_db_location.data_address == 0 ||
	    (dmm->dmm_loc_list.ptr_in_count &&
		(dmm->dmm_loc_list.ptr_size != sizeof(DMM_LOC_LIST) ||
		 dmm->dmm_loc_list.ptr_address == 0)))
	{
	    SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
	}
    }

    if ( dmm->error.err_code )
	return (E_DB_ERROR);

    status = dmm_add_create(xcb, &dmm->dmm_db_name,
	&dmm->dmm_owner,
	dmm->dmm_db_id, dmm->dmm_db_service, dmm->dmm_db_access,
	&dmm->dmm_location_name,
	dmm->dmm_db_location.data_in_size,
	dmm->dmm_db_location.data_address,
	dmm->dmm_loc_list.ptr_in_count,
	(DMM_LOC_LIST **) dmm->dmm_loc_list.ptr_address,
	dmm->dmm_table_name.db_tab_name, dmm->dmm_utable_name.db_tab_name,
	&dmm->error);

    return (status);
}

/*
** Name: dmm_add_create - build the CONFIG file and validate locations for 
**			  create db
** History:
**	13-may-1997 (wonst02)
** 	    Support readonly databases.
**      28-may-1998 (stial01)
**          dmm_add_create() Support VPS system catalogs.
**    07-mar-2000 (gupsh01)
**        Added error checking for creating readonly database
**        when the specified data location does not have the 
**        database directory or the cnf files
**	15-Oct-2001 (jenjo02)
**	    Set DMM_EXTEND_LOC_AREA flag in loc_flags.
**	04-Dec-2001 (jenjo02)
**	    Check for attempt to use Raw area as root location,
**	    return error.
**	15-Apr-2003 (bonro01)
**	    Correct parms on ule_format() that were swapped.
**	6-Jan-2005 (schka24)
**	    DB version in config file is DB compat level, not rel compat
**	    level.  The two need not be in sync.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change cmptlvl to an integer.
*/
DB_STATUS
dmm_add_create(
DML_XCB		*xcb,
DB_DB_NAME	*db_name,
DB_OWN_NAME	*db_owner,
i4		db_id,
i4		service,
i4		access,
DB_LOC_NAME	*location_name,
i4		l_root,
char		*root,
i4		loc_count,
DMM_LOC_LIST	*loc_list[],
char		*collation,
char		*ucollation,
DB_ERROR	*dberr)
{
    DMP_MISC	    *cnf;
    DM0C_DSC	    *desc;
    DM0C_JNL	    *jnl;
    DM0C_DMP	    *dmp;
    DM0C_TAB	    *rel;
    DM0C_EXT	    *ext;
    i4	    *eof;
    i4	    i, k;
    i4	    next_att = 0;
    i4	    loc_flags;
    i4	    rdloc_flags;
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4	    l_loc_dir;
    i4	    l_wloc_dir;
    i4	    l_db_dir;
    i4	    l_loc_buffer;
    i4	    l_rdloc_dir;
    i4	    l_rddb_dir;
    char	    *ptr;
    char	    loc_buffer[sizeof(DM_PATH) + sizeof(int)];
    char	    loc_dir[sizeof(DM_PATH) + sizeof(int)];
    char	    wloc_dir[sizeof(DM_PATH) + sizeof(int)];
    char	    rdloc_dir[sizeof(DM_PATH) + sizeof(int)];
    char	    db_dir[sizeof(DM_PATH) + sizeof(int)];
    char	    rddb_dir[sizeof(DM_PATH) + sizeof(int)];
    LOCATION	    cl_loc;
    i4	    local_err_code;
    i4		    page_type;
    i4		    page_size;
    i4		    max_core_relwid;
    i4		    *err_code = &dberr->err_code;

    /*
    ** All core catalogs should be created with same page size
    ** Copy the page size from the server control block in case it is
    ** changes via "set trace point dm714"
    */
    page_size = dmf_svcb->svcb_page_size;

    /* choose page type */
    max_core_relwid = (sizeof(DMP_RELATION) > sizeof(DMP_ATTRIBUTE)) ?
		sizeof(DMP_RELATION) : sizeof(DMP_ATTRIBUTE);
    max_core_relwid = (max_core_relwid > DMP_INDEX_SIZE) ? max_core_relwid :
		DMP_INDEX_SIZE;
	
    /* All core catalogs should be created with the same page type */
    status = dm1c_getpgtype(page_size, DM1C_CREATE_CORE, 
		max_core_relwid, &page_type);
    if (status != E_DB_OK)
    {
	SETDBERR(dberr, 0, E_DM0103_TUPLE_TOO_WIDE);
	return(status);
    }

    /*	Construct path name for location and pathname for database. */
    loc_flags = DMM_EXTEND_LOC_AREA;

    status = dmm_loc_validate(DMM_L_DATA, db_name, l_root, root,
	loc_dir, &l_loc_dir, db_dir, &l_db_dir, &loc_flags, dberr);
    if (status != E_DB_OK)
	return (status);

    /* Check that raw location usage is not being attempted */
    if ( loc_flags & DCB_RAW )
    {
	SETDBERR(dberr, 0, E_DM0199_INVALID_ROOT_LOCATION);
	return(E_DB_ERROR);
    }

    /*
    ** If getting access to an existing readonly database, use II_DATABASE area 
    ** to build a copy of the .cnf file that points to the readonly database.
    */
    if (access & DU_RDONLY)
    {
       /*
       ** For read only database make sure that the Location
       ** specified for the read only database is ok
       */
       {
       char    db_temp_buffer[sizeof(DB_DB_NAME) + sizeof(int)];
       char    loc_temp_buffer[sizeof(DM_PATH) + sizeof(int)];
       bool    db_path_exists = FALSE;
       bool    cnf_file_exists = FALSE;
       LOINFORMATION   cl_loc_inf;
       char    *cnf_file = "aaaaaaaa.cnf";
       i4      cl_flagword = LO_I_ALL;
       char    *location_type = LO_DB;
       LOCATION        cl_temp_loc;
 
       /*
       ** Construct path name for location and pathname for database. 
       */
       for (i = 0; i < sizeof(DB_DB_NAME); i++)
           if ((db_temp_buffer[i] = db_name->db_db_name[i]) == ' ')
           break;
       db_temp_buffer[i] = 0;
       MEmove(l_root, root, 0, sizeof(loc_temp_buffer), loc_temp_buffer);
  
       /* 
       ** First check that the readonly data base exists at the 
       ** user specified location. 
       ** Get location for database. 
       */
 
       LOingpath(loc_temp_buffer, db_temp_buffer, location_type, &cl_temp_loc);
       if (LOinfo(&cl_temp_loc, &cl_flagword, &cl_loc_inf) == OK)
       {
           db_path_exists = TRUE;
       /* 
       ** check if the cnf file exists in the database location
       */
           LOfstfile(cnf_file, &cl_temp_loc);
           if (LOinfo(&cl_temp_loc, &cl_flagword, &cl_loc_inf) == OK)
           cnf_file_exists = TRUE;
       }
 
           if (!(db_path_exists && cnf_file_exists))
           {
	       SETDBERR(dberr, 0, E_DM0192_CREATEDB_RONLY_NOTEXIST);
               return (E_DB_ERROR);
           }
       }
	rdloc_flags = DMM_EXTEND_LOC_AREA;

    	status = dmm_loc_validate(DMM_L_DATA, db_name, 
		sizeof(ING_DBDIR), ING_DBDIR, rdloc_dir, &l_rdloc_dir, 
		rddb_dir, &l_rddb_dir, &rdloc_flags, dberr);
    	if (status != E_DB_OK)
	    return (status);
    }

    /*	Build config file in memory. */

    {
	/*
	** Allocate buffer to format config file. Request that the memory be
	** initially cleared with zeros, so that we only need initialize the
	** non-zero portions.
	*/

	status = dm0m_allocate((DM0C_PGS_HEADER*DM_PG_SIZE) + sizeof(DMP_MISC),
	    (i4) DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	    (char *)NULL, (DM_OBJECT **)&cnf, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
	    return (E_DB_ERROR);
	}
	cnf->misc_data = (char *)cnf + sizeof(DMP_MISC);

	/*  Initialize description section of config. */

	desc = (DM0C_DSC *)cnf->misc_data;
	desc->length = sizeof(DM0C_DSC);
	desc->type = DM0C_T_DSC;
	desc->dsc_cnf_version = CNF_VCURRENT;
	desc->dsc_c_version = DSC_VCURRENT;
	desc->dsc_version = DSC_VCURRENT;

	desc->dsc_status = (access & DU_RDONLY) ? 0 : DSC_VALID;
	desc->dsc_type = DSC_PUBLIC;
	desc->dsc_open_count = 0;
	desc->dsc_sync_flag = 0;
	desc->dsc_access_mode = (access & DU_RDONLY) ? DSC_READ : DSC_WRITE;
	desc->dsc_dbid = db_id;
	desc->dsc_sysmod = 0;
	desc->dsc_last_table_id = 128;
	desc->dsc_ext_count = loc_count + 1;
	desc->dsc_dbcmptlvl = DU_CUR_DBCMPTLVL;
	desc->dsc_1dbcmptminor = 0;
	desc->dsc_dbaccess = access;
	desc->dsc_dbservice = service;
#if defined(LP64)
	desc->dsc_dbservice |= DU_LP64;
#endif /* LP64 */
	desc->dsc_iirel_relprim = 1;
	desc->dsc_iirel_relpgtype = page_type;
	desc->dsc_iiatt_relpgtype = page_type;
	desc->dsc_iiind_relpgtype = page_type;
	desc->dsc_iirel_relpgsize = page_size;
	desc->dsc_iiatt_relpgsize = page_size;
	desc->dsc_iiind_relpgsize = page_size;

	MEcopy((PTR) db_name, sizeof(desc->dsc_name), (PTR) &desc->dsc_name);
	MEcopy((PTR) db_owner, sizeof(desc->dsc_owner), (PTR) &desc->dsc_owner);
	MEcopy((PTR) collation, sizeof(desc->dsc_collation), 
	    (PTR) desc->dsc_collation);
	MEcopy((PTR) ucollation, sizeof(desc->dsc_ucollation), 
	    (PTR) desc->dsc_ucollation);
	
	/*  Initialize journal section of config file. */

	jnl = (DM0C_JNL *)&desc[1];
	jnl->length = sizeof(DM0C_JNL);
	jnl->type = DM0C_T_JNL;
	jnl->jnl_count = 0;
	jnl->jnl_ckp_seq = 0;
	jnl->jnl_fil_seq = 0;
	jnl->jnl_blk_seq = 0;
	jnl->jnl_update = 0;
	jnl->jnl_first_jnl_seq = 0;
	jnl->jnl_bksz = 16384;
	jnl->jnl_blkcnt = 4;
	jnl->jnl_maxcnt = 512;
	MEfill(sizeof(jnl->jnl_history), 0, (char *)jnl->jnl_history);
	MEfill(sizeof(jnl->jnl_node_info), 0, (PTR)jnl->jnl_node_info);

	/*  Initialize dump section of config file. */

	dmp = (DM0C_DMP *)&jnl[1];
	dmp->length = sizeof(DM0C_DMP);
	dmp->type = DM0C_T_DMP;
	dmp->dmp_count = 0;
	dmp->dmp_ckp_seq = 0;
	dmp->dmp_fil_seq = 0;
	dmp->dmp_blk_seq = 0;
	dmp->dmp_update = 0;
	dmp->dmp_bksz = 16384;
	dmp->dmp_blkcnt = 4;
	dmp->dmp_maxcnt = 512;
	MEfill(sizeof(dmp->dmp_history), '\0', (PTR) dmp->dmp_history);

	/*  Add root location to config file. */

	ext = (DM0C_EXT *)&dmp[1];
	ext->length = sizeof(*ext);
	ext->type = DM0C_T_EXT;
	ext->ext_location.flags = DCB_ROOT | DCB_DATA;
	ext->ext_location.logical = *location_name;
	MEmove(l_db_dir, db_dir, ' ',
	    sizeof(ext->ext_location.physical.name),
	    ext->ext_location.physical.name);
	ext->ext_location.phys_length = l_db_dir;

	status = E_DB_OK;
	for (i = 0; i < loc_count; i++)
	{
	    loc_flags = DMM_EXTEND_LOC_AREA;
	    status = dmm_loc_validate(loc_list[i]->loc_type, db_name,
		loc_list[i]->loc_l_area, loc_list[i]->loc_area,
		wloc_dir, &l_wloc_dir, loc_buffer, &l_loc_buffer, &loc_flags,
		dberr);
	    if (status != E_DB_OK)
		break;

	    /* if initial work location we must create the directory */
	    if (loc_list[i]->loc_type == DMM_L_WRK)
	    {
		char	    db_buffer[sizeof(DB_DB_NAME) + 4];
		i4	    db_length;
		DI_IO	    di_context;

		for (k = 0; k < sizeof(DB_DB_NAME); k++)
		    if ((db_buffer[k] = db_name->db_db_name[k]) == ' ')
			break;
		db_buffer[k] = 0;
		db_length = STlength(db_buffer);
		status = DIdircreate(&di_context, wloc_dir, l_wloc_dir,
		    db_buffer, db_length, &sys_err);

		/* we allow directory to already exist... */
		if (status == DI_EXISTS)
		    status = OK;

		if (status != OK)
		{
		    uleFormat(NULL, E_DM9002_BAD_FILE_CREATE, &sys_err, ULE_LOG,
			NULL, NULL, 0, NULL,
			err_code, 4,
			db_length, db_buffer,
			0, "",
			l_wloc_dir, wloc_dir,
			db_length, db_buffer);
		    SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
		    break;
		}
	    }

	    ext = (DM0C_EXT *)&ext[1];
	    ext->length = sizeof(*ext);
	    ext->type = DM0C_T_EXT;
	    ext->ext_location.flags = loc_flags;
	    ext->ext_location.logical = loc_list[i]->loc_name;
	    MEmove(l_loc_buffer, loc_buffer, ' ',
		sizeof(ext->ext_location.physical.name),
		ext->ext_location.physical.name);
	    ext->ext_location.phys_length = l_loc_buffer;
	}

	/*  Mark end of configuration file. */

	eof = (i4 *)&ext[1];
	*eof = 0;
    }

    /*	Create the database. */

    if (status == E_DB_OK)
    	if (access & DU_RDONLY)
	    status = create_db(xcb, cnf, db_name, db_id, access,
	    	service, location_name, l_rdloc_dir, (DM_PATH *)rdloc_dir, 
	    	l_rddb_dir, (DM_PATH *)rddb_dir, page_type, page_size, dberr);
	else
	    status = create_db(xcb, cnf, db_name, db_id, access,
	    	service, location_name, l_loc_dir, (DM_PATH *)loc_dir, 
	    	l_db_dir, (DM_PATH *)db_dir, page_type, page_size, dberr);

    dm0m_deallocate((DM_OBJECT **)&cnf);

    if (status == E_DB_OK)
	return (status);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
    }

    return (E_DB_ERROR);
}

/*
** Name: create_db		- internal subroutine.
**
** History:
**	25-sep-1991 (mikem) integrated following change: 5-aug-1991 (bryanp)
**	    Log additional error information before returning E_DM011B.
**	14-dec-1992 (rogerk)
**	    Added new dm0l_crdb arguments.
**	30-feb-1993 (rmuth)
**	    Prototype DI showed errors.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Add support for upper case regular identifiers in create_db().
**	11-Jan-1994 (rmuth)
**	    b58106 - create_db() improve error handling.
** 	13-may-1997 (wonst02)
**	    If a readonly database, do not build the relations.
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
*/
static DB_STATUS
create_db(
DML_XCB		*xcb,
DMP_MISC	*cnf,
DB_DB_NAME	*db_name,
i4		db_id,
i4		db_access,
i4		db_service,
DB_LOC_NAME	*location_name,
i4		device_length,
DM_PATH		*device_name,
i4		path_length,
DM_PATH		*path_name,
i4		page_type,
i4		page_size,
DB_ERROR	*dberr)
{
    DML_XCB	*x = xcb;
    DI_IO	di_context;
    i4	db_length;
    i4	page_number;
    i4	write_count;
    CL_ERR_DESC	sys_err;
    CL_ERR_DESC	tmp_sys_err;
    DB_STATUS	dbstatus;
    STATUS	status;
    STATUS	tmp_status;
    i4	config_open =0;
    i4		*err_code = &dberr->err_code;

    DMP_RELATION    *lcl_relations	= DM_core_relations;
    DMP_ATTRIBUTE   *lcl_attributes	= DM_core_attributes;
    DMP_RINDEX	    *lcl_rindex		= DM_core_rindex;
    DML_SCB	    *scb;
#define MAX_CORE_ATTS 75
    i4		    i, j, k;
    DB_ATTS	    db_atts[MAX_CORE_ATTS];
    DB_ATTS	    *db_att_ptrs[MAX_CORE_ATTS];
    i4		    cmpcontrol_size;
    char	    cmpcontrol_buf[MAX_CORE_ATTS * 3 * sizeof(DM1CN_CMPCONTROL)];
    DMP_ROWACCESS   rac;
    DMP_RELATION    *rel;
    DMP_ATTRIBUTE   *att;
    char	    *records;
    i4		    record_count;
    DMP_RELATION    tmp_relations[DM_CORE_REL_CNT];
    i4		    data_pages;
    i4		    tperpage;

    /*	Compute length of database name. */

    for (db_length = 0;
	db_length < sizeof(DB_DB_NAME) && db_name->db_db_name[db_length] != ' ';
	db_length++
	);
        
    /*	Log create_db operation if not called from DMC_ADD for 'iidbdb'. */

    if (x)
    {
	dbstatus = dm0l_crdb(x->xcb_log_id, (i4)0,
		db_id, db_name, db_access, db_service, location_name, 
		path_length, path_name, (LG_LSN *)NULL, dberr);
	if (dbstatus != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM004A_INTERNAL_ERROR);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Determine proper templates based on case translation semantics.
    */

    if (db_service & DU_NAME_UPPER)
    {
	lcl_relations	= DM_ucore_relations;
	lcl_attributes	= DM_ucore_attributes;
	lcl_rindex	= DM_ucore_rindex;
    }

    status = DIdircreate(&di_context, device_name->name, device_length,
	(char *)db_name, db_length, &sys_err);
    if (status != OK)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM011A_ERROR_DB_DIR);
	return (E_DB_ERROR);
    }

    /*	Create configuration file. */
    do
    {
        status = DIcreate( &di_context, path_name->name, path_length,
	                   "aaaaaaaa.cnf", 12, DM_PG_SIZE, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
	    uleFormat(NULL, E_DM9337_DM2F_CREATE_ERROR, &sys_err, ULE_LOG, NULL,
		       NULL, 0, NULL, err_code, 0);
	    break;
	}

	status = DIopen( &di_context, path_name->name, path_length,
	                 "aaaaaaaa.cnf", 12, DM_PG_SIZE, DI_IO_WRITE, 
			  DI_SYNC_MASK, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
            uleFormat( NULL, E_DM9004_BAD_FILE_OPEN, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, err_code, 4,
			sizeof(DB_DB_NAME), db_name,
			11, "config file",
			path_length, path_name->name,
			12, "aaaaaaaa.cnf");
	    break;
	}
        config_open++;

	write_count = DM0C_PGS_HEADER;

	status = DIalloc(&di_context, DM0C_PGS_HEADER, &page_number, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
            uleFormat( NULL, E_DM9000_BAD_FILE_ALLOCATE, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, err_code, 4,
			sizeof(DB_DB_NAME), db_name,
			11, "config file",
			path_length, path_name->name,
			12, "aaaaaaaa.cnf");
	    break;
	}

	status = DIflush(&di_context, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
            uleFormat( NULL, E_DM9008_BAD_FILE_FLUSH, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, err_code, 4,
			sizeof(DB_DB_NAME), db_name,
			11, "config file",
			path_length, path_name->name,
			12, "aaaaaaaa.cnf");
	    break;
	}

	status = DIwrite(&di_context, &write_count, page_number,
		         cnf->misc_data, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
            uleFormat( NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, err_code, 5,
			sizeof(DB_DB_NAME), db_name,
			11, "config file",
			path_length, path_name->name,
			12, "aaaaaaaa.cnf",
			0, page_number);
	    break;
	}


	config_open--;
	status = DIclose(&di_context, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
            uleFormat( NULL, E_DM9001_BAD_FILE_CLOSE, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, err_code, 4,
			sizeof(DB_DB_NAME), db_name,
			11, "config file",
			path_length, path_name->name,
			12, "aaaaaaaa.cnf");
	    break;
	}

    } while (FALSE);

    /*
    ** If error creating the config file then bail !
    */
    if ( status != OK )
    {
	if (config_open)
	{
	    STATUS lstatus;

	    lstatus = DIclose(&di_context, &sys_err);
	    if ( lstatus != OK )
	    {
		uleFormat(NULL, lstatus, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
		uleFormat( NULL, E_DM9001_BAD_FILE_CLOSE, &sys_err, ULE_LOG, 
			    NULL, NULL, 0, NULL, err_code, 4,
			    sizeof(DB_DB_NAME), db_name,
			    11, "config file",
			    path_length, path_name->name,
			    12, "aaaaaaaa.cnf");
	    }
	}

	SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
	return (E_DB_ERROR);
    }

    /* If a readonly database, do not build the relations. */
    if (db_access & DU_RDONLY)
    	return status;

    /*
    ** Fix all iirelation tuples before we build iirelation
    ** Do this here so we calculate relfhdr once 
    ** build_file() allocates (rel->relfhdr - 1) data pages
    ** This eliminates any need to fix up the rel->relhdr after loading rows
    */
    for (rel = &tmp_relations[0], i = 0; i < DM_CORE_REL_CNT; rel++, i++)
    {
	*rel = lcl_relations[i];
	tperpage = DMPP_TPERPAGE_MACRO(page_type, page_size, rel->relwid);
	data_pages = (rel->reltups / tperpage) + 1;
	rel->relpgtype = page_type;
	rel->relpgsize = page_size;
	rel->relloc = *location_name;
	rel->relfhdr = data_pages + 1;
    }

    /* reset lcl_relations to the iirelation tups we fixed */
    lcl_relations = &tmp_relations[0];

    /*	Create core relations. */
    for ( rel = lcl_relations, i = 0; i < DM_CORE_REL_CNT; rel++, i++)
    {
	if (rel->reltid.db_tab_base == DM_B_RELATION_TAB_ID &&
		rel->reltid.db_tab_index == DM_I_RELATION_TAB_ID)
	    records = (char *)lcl_relations; /* the reltups we FIXED */
	else if (rel->reltid.db_tab_base == DM_B_RIDX_TAB_ID &&
		rel->reltid.db_tab_index == DM_I_RIDX_TAB_ID)
	    records = (char *)lcl_rindex;
	else if (rel->reltid.db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
	    records = (char *)lcl_attributes;
	else
	    records = (char *)DM_core_index;

	/* Build DB_ATTS for the core catalog we are loading */
	MEfill(sizeof(db_atts), 0, &db_atts);
	MEfill(sizeof(rac), 0, &rac);
	rac.att_ptrs = &db_att_ptrs[0];
	rac.att_count = rel->relatts;
	rac.compression_type = rel->relcomptype;
	cmpcontrol_size = dm1c_cmpcontrol_size(rel->relcomptype,
		rel->relatts, rel->relversion);
	cmpcontrol_size = DB_ALIGN_MACRO(cmpcontrol_size);
	rac.control_count = cmpcontrol_size;
	rac.cmp_control = cmpcontrol_buf;
	if (rac.control_count > sizeof(cmpcontrol_buf))
	{
	    /* stack buffer isn't big enough */
	    SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
	    return (E_DB_ERROR);
	}

	for (att = DM_core_attributes, j = 0, k = 0; 
		j < DM_core_att_cnt; j++, att++)
	{
	    if (att->attrelid.db_tab_base == rel->reltid.db_tab_base &&
		    att->attrelid.db_tab_index == rel->reltid.db_tab_index)
	    {
		/* init DB_ATTS fields used by compression routines */
		db_att_ptrs[k] = &db_atts[k];
		db_atts[k].length = att->attfml;
		db_atts[k].type = att->attfmt;
		db_atts[k].precision = att->attfmp;
		k++;
	    }
	}
	dbstatus = dm1c_rowaccess_setup(&rac);
	if (dbstatus != E_DB_OK)
	    break;

	dbstatus = build_file(db_name, db_service, rel, &rac,
	    path_name, path_length, records, dberr);

	if (dbstatus)
	    break;
    }

    if (dbstatus == E_DB_OK)
	return (dbstatus);

    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		NULL, 0, NULL, err_code, 0);

    SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
    return (E_DB_ERROR);
}

/*{
** Name: build_file  - Build a one bucket hash table on disc and populate it
**                     with the records supplied.
**
** Description:
**      This routine
**
** Inputs:
**      db_name                         Pointer to database name.
**      db_service
**      rel				relation tuple for catalog to build
**	rac				DMP_ROWACCESS
**      path_name                       Pointer to physical path.
**      path_length                     length of physical path.
**      records                         Pointer to tuples to put in table.
**
** Outputs:
**      err_code                        Pointer to an area to return
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
**         Created header.
**      29-August-1992 (rmuth)
**         Because we have removed the 6.4->6.5 on-the-fly table we now need
**         to create the FHDR/FMAP(s) structure in the file.
**	09-oct-1992 (jnash)
**	   6.5 Reduced logging project.  Use PG_SYSCAT page accessors.
**	30-October-1992 (rmuth)
**	   - Change for new file extend DI paradigm.
**	   - Reduce DIalloc calls by tracking allocated pages.
**	12-nov-1992 (bryanp)
**	    In build_file, if the overflow chain ends after exactly filling the
**	    last page, ensure that we terminate the overflow chain correctly
**	    rather than pointing to a (non-existent) next overflow page.
**	19-nov-92 (rickh)
**	    If the last tuple won't fit on the current page, make sure we
**	    put it on the next page and write that page!
**	30-feb-1993 (rmuth)
**	    Prototyping DI showed up some warnings.
**	14-Jan-1993 (rmuth)
**	    b58108 - Check status and log errors after DI calls.
**	    b58107 - Dec open_file when close file.
**	25-may-1993 (robf)
**	     Changed E_DM9004 to E_DM9058 to more clearly say what went wrong,
**	     problem is building the file, not opening it.
**	12-oct-93 (robf)
**           Updated handling for initializing table labels.
**	18-apr-1994 (jnash)
**	    fsync project.  Remove DIforce() call.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass DM_PG_SIZE as the page_size argument to dmpp_format.
**	06-may-1996 (nanpr01 & thaju02)
**	    Change the page header access as macro for New Page Format
**	    project. Pass DM_PG_SIZE to dm1c_getaccessors().
**	20-may-1996 (ramra01)
**	    Added new param tup_info to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      15-jul-1996 (ramra01 for bryanp)
**          Pass 0 as the current table version to dmpp_load when building
**              core system catalogs.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      28-may-1998 (stial01)
**          build_file() Support VPS system catalogs.
**	20-Feb-2008 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.
*/
static DB_STATUS
build_file(
DB_DB_NAME	*db_name,
i4		db_service,
DMP_RELATION	*rel,
DMP_ROWACCESS	*rac,
DM_PATH		*path_name,
i4		path_length,
char		*records,
DB_ERROR	*dberr)
{
    char	*r = records;
    i4	count = rel->reltups;
    DM_PAGENO	page_number = 0;
    i4	write_count;
    i4	file_open = 0;
    DB_STATUS	db_status = E_DB_OK;
    STATUS	status;
    DMPP_PAGE	*pagep = (DMPP_PAGE *)0;
    DI_IO	di_context;
    DM_FILE	file_name;
    CL_ERR_DESC	sys_err;
    i4	lerr_code;
#define 	MAX_CORE_TUP_SIZE 2048
    char	tuple[MAX_CORE_TUP_SIZE];
    char	ctuple[MAX_CORE_TUP_SIZE];
    char	*rec;
    DM_PAGENO   fhdr_pageno;
    i4	pages_allocated = 0;
    DMPP_ACC_PLV        *local_plv;
    i4		*err_code = &dberr->err_code;
    i4		rec_len;
    i4		page_size = rel->relpgsize;
    i4		page_type = rel->relpgtype;
    DB_LOC_NAME	*location_name = &rel->relloc;
    i4		record_count = rel->reltups;

    do
    {
        /*
	** Get a local set of default accessors to work with 
	** Make sure the stack buffer is big enough for the core tuple
	*/
	dm1c_get_plv(page_type, &local_plv);

	if ((pagep = (DMPP_PAGE *)dm0m_pgalloc(page_size)) == 0 ||
		MAX_CORE_TUP_SIZE < rel->relwid)
	{
	    SETDBERR(dberr, 0, E_DM9425_MEMORY_ALLOCATE);
	    status = E_DB_ERROR;
	    break;
	}

        /*
	** Format the first page of the relation.
	*/
        (*local_plv->dmpp_format) (page_type, page_size, pagep,
				    page_number, DMPP_DATA|DMPP_MODIFY,
				    DM1C_ZERO_FILL);

	/*
	** Work out the tables filename
	*/
        dm2f_filename(DM2F_TAB, &rel->reltid, 0, 0, 0, &file_name);

        /*
	** Create file for relation. 
	*/
        status = DIcreate( &di_context, path_name->name, path_length,
			   (char *)&file_name, sizeof(file_name),
			   page_size, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,&lerr_code,0);
            uleFormat(NULL, E_DM9337_DM2F_CREATE_ERROR, &sys_err, ULE_LOG, NULL, 
		   NULL, 0, NULL, &lerr_code, 0);
	    break;
	}

	/*
	** Open the file 
	*/
	status = DIopen( &di_context, path_name->name, path_length,
	    		 file_name.name, sizeof(file_name),
	    		 page_size, DI_IO_WRITE, DI_SYNC_MASK,
	    		 &sys_err);
	if (status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,&lerr_code,0);
            uleFormat( NULL, E_DM9004_BAD_FILE_OPEN, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, &lerr_code, 4,
			sizeof(DB_DB_NAME), db_name,
			sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
			path_length, path_name->name,
			sizeof(file_name), &file_name);
	    break;
	}

	file_open++;

	for (r = records; 
		--count >= 0 || page_number < rel->relfhdr; 
			r += rel->relwid)
	{
	    if (count >= 0)
	    {
		MEcopy(r, rel->relwid, tuple);

		if (db_service & DU_NAME_UPPER)
		{
		    /* upper case semantics for char columns */
		    /* FIX ME FIXME */
		}

		rec = tuple;
		rec_len = rel->relwid;
		if (rel->relstat & TCB_COMPRESSED)
		{
		    rec = ctuple;
		    status = (*rac->dmpp_compress)(rac, tuple, rec_len, 
			    ctuple, &rec_len);
		    if ( status )
		    {
			SETDBERR(dberr, 0, E_DM9381_DM2R_PUT_RECORD);
			break;
		    }
		}
	    }

            /* 
	    ** While more data and space in page.
	    */
            while ( (count >= 0 & (db_status =
                     (*local_plv->dmpp_load)(page_type, page_size, pagep,
			rec, rec_len,
			DM1C_LOAD_NORMAL, 0, 0, (u_i2)0, (DMPP_SEG_HDR *)0))
                        != E_DB_OK) || 
			(count <= 0 && page_number < rel->relfhdr)) 
	    {
		/*
		** Current page is full
		** or we're need to alloc another data page up to relfhdr
		*/

		/*
		** See if need to allocate space in the table
		*/
		if ( ( page_number + 1 ) > pages_allocated )
		{
		    status = DIalloc( 
				&di_context, DM_TBL_DEFAULT_EXTEND, 
				(i4 *)&page_number, &sys_err);
		    if ( status != OK )
		    {
	                uleFormat( NULL, status, &sys_err, ULE_LOG,
				NULL,NULL,0,NULL, &lerr_code,0);
			uleFormat( NULL, E_DM9000_BAD_FILE_ALLOCATE, &sys_err, 
				ULE_LOG, NULL, NULL, 0, NULL, &lerr_code, 4,
				sizeof(DB_DB_NAME), db_name,
				sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
				path_length, path_name->name,
				sizeof(file_name), &file_name);
		        break;
		    }

		    pages_allocated = page_number + DM_TBL_DEFAULT_EXTEND;
		}

		/*
		** Set the pages overflow pointer, if still more data
		** to load then set the current pages overflow ptr to
		** point to the next data page, otherwise null
		** the overflow pointer
		*/
		if ( ( count == 0 ) && ( db_status == E_DB_OK ) )
		{
		    DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, pagep, 0);
		}
		else
		{
		    DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, pagep, page_number + 1);
		}

		/*  
		** Write the current page to disk. 
		*/
		write_count = 1;
		status = DIwrite( &di_context, &write_count,
				  page_number, (char *)pagep, &sys_err);
		if (status != OK)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,
			    &lerr_code,0);
		    uleFormat( NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &lerr_code, 5,
			    sizeof(DB_DB_NAME), db_name,
			    sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
			    path_length, path_name->name,
			    sizeof(file_name), &file_name,
			    0, page_number);
		    break;
		}

		/*
		** Format the next page
		*/
                (*local_plv->dmpp_format)(page_type, page_size, pagep,
			++page_number, DMPP_DATA|DMPP_MODIFY,
			DM1C_ZERO_FILL);

		/*
		** If no more data then exit
		*/
		if ( ( count == 0 ) && ( db_status == E_DB_OK ) )
		    break;

	    } /* while */

	    /*
	    ** If there was an error writting the current data page
	    ** to disc, or if we have run out of data to load then
	    ** exit
	    */
	    if ( (status != OK) || ( count <= 0 && page_number == rel->relfhdr))
		break;

	} /* for */

	/*
	** See if there was an error adding a new page
	*/
	if (status != OK)
	    break;

        /*
        ** Add the FHDR/FMAP(s) Space Management Scheme.
        */
	db_status = dmm_add_SMS( &di_context, location_name,
				 path_name, path_length, page_type, page_size,
				 &file_name, &page_number,
				 &rel->relid, db_name,
				 &fhdr_pageno, &pages_allocated,
				 local_plv, dberr);
	if ( db_status != E_DB_OK )
	    break;

	/* Make sure we have placed the fhdr where we thought we would */
	if ( fhdr_pageno != rel->relfhdr)
	{
	    /*  create_db() didn't calculate relfhdr correctly */
	    TRdisplay("relid %~t fhdr_pageno %d relfdhr %d\n",
		10, rel->relid.db_tab_name, fhdr_pageno, rel->relfhdr);
	    SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Guarantee all the space we have allocated
	*/
	status = DIguarantee_space( 
			&di_context, page_number,
		        (i4)( pages_allocated - (i4)page_number ), 
				    &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,
	    		&lerr_code,0);
            uleFormat( NULL, E_DM9000_BAD_FILE_ALLOCATE, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, &lerr_code, 4,
			sizeof(DB_DB_NAME), db_name,
			sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
			path_length, path_name->name,
			sizeof(file_name), &file_name);
            uleFormat(NULL, E_DM92CE_DM2F_GUARANTEE_ERR, &sys_err, ULE_LOG, NULL, 
		   NULL, 0, NULL, &lerr_code, 0);
	    break;
	}

	/*
	** Flush the file
	*/
	status = DIflush(&di_context, &sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,
	    		&lerr_code,0);
            uleFormat( NULL, E_DM9008_BAD_FILE_FLUSH, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, &lerr_code, 4,
			sizeof(DB_DB_NAME), db_name,
			sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
			path_length, path_name->name,
			sizeof(file_name), &file_name);
	    break;
	}
	
	/*
	** Close the file for the relation
	*/
	file_open--;
	status = DIclose( &di_context, &sys_err );
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,
	    		&lerr_code,0);
            uleFormat( NULL, E_DM9001_BAD_FILE_CLOSE, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, &lerr_code, 4,
			sizeof(DB_DB_NAME), db_name,
			sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
			path_length, path_name->name,
			sizeof(file_name), &file_name);
	    break;
	}

	if (pagep)
	    dm0m_pgdealloc((char **)&pagep);

        return( E_DB_OK);	

    } while ( FALSE );

    /*
    ** Error condition
    */
    if (pagep)
	dm0m_pgdealloc((char **)&pagep);

    if (file_open)
    {
	STATUS close_status;

	close_status = DIclose(&di_context, &sys_err);
	if ( close_status != OK )
	{
	    uleFormat(NULL, close_status, &sys_err, ULE_LOG,NULL,NULL,0,NULL,
	    		&lerr_code,0);
            uleFormat( NULL, E_DM9001_BAD_FILE_CLOSE, &sys_err, ULE_LOG, 
			NULL, NULL, 0, NULL, &lerr_code, 4,
			sizeof(DB_DB_NAME), db_name,
			sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
			path_length, path_name->name,
			sizeof(file_name), &file_name);
	}
    }

    /* 
    ** If db_status error then err_code wil have been filled in so issue
    ** an error
    */
    if ( db_status != E_DB_OK )
    {
        uleFormat( dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    }

    uleFormat(NULL, E_DM9058_BAD_FILE_BUILD, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
	err_code, 4,
	sizeof(DB_DB_NAME), db_name,
	sizeof(DB_TAB_NAME), rel->relid.db_tab_name,
	path_length, path_name->name,
	sizeof(file_name), &file_name);

    SETDBERR(dberr, 0, E_DM011B_ERROR_DB_CREATE);
    return (E_DB_ERROR);
}

/*{
** Name: dmm_add_SMS  -  Adds a tables FHDR/FMAP(s) space management scheme.
**
** Description:
**	This routine is used to add a tables FHDR/FMAP(s). It
**	creates a DMP_LOCATION structure for DM2F context and
**	then calls DM1P to add the FHDR/FMAP(s). 
**
** Inputs:
**	di_context			The DI_IO context.
**	loc_name			The location name.
**	path_name			The physical path name.
**	path_length			Path length.
**      page_type			page type
**      page_size                       page size
**	next_pageno			Next available page number
**	tab_name			The tables name.
**	dbname				The database name.
** Outputs:
**	fhdr_pageno			Page number of the FHDR.
**      err_code                        Pointer to an area to return
**	pages_allocated			Number of pages allocated to the
**					file.
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
**	   Initialized fcb allocation fields so that later dm2f_alloc_calls
**	   will work properly.
**	30-October-1992 (rmuth)
**	   Build the dm1p build control block.
**	30-November-1992 (rmuth)
**	   Integration problem, a similar piece of code was added in two
**	   different places hence CMS showed no conflict and i just 
**	   noticed it.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	25-may-1992 (robf)
**	   Removed excess error traceback, was causing message to be logged
**	   twice.
**	27-apr-1995 (cohmi01)
**	   Initialize loc_status field, contained junk from stack.
**	06-mar-1996 (stial01 for bryanp)
**	    Set SMS_page_size in the DM1P_BUILD_SMS_CB.
**      28-may-1998 (stial01)
**          dmm_add_SMS() Support VPS system catalogs.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    FCBs are small and easily fit in the stack, so why
**	    dm0m_allocate?
**
*/
static DB_STATUS
dmm_add_SMS(
    DI_IO		*di_context,
    DB_LOC_NAME		*loc_name,
    DM_PATH		*path_name,
    i4 		path_length,
    i4			page_type,
    i4			page_size,
    DM_FILE		*file_name,
    DM_PAGENO		*next_pageno,
    DB_TAB_NAME		*tab_name,
    DB_DB_NAME		*dbname,
    DM_PAGENO		*fhdr_pageno,
    i4		*pages_allocated,
    DMPP_ACC_PLV	*plv,
    DB_ERROR		*dberr )
{
    DMP_LOCATION 	location;
    DMP_LOC_ENTRY	loc_entry;
    DMP_FCB		loc_fcb;
    DMP_FCB		*loc_fcb_ptr = &loc_fcb;
    DB_STATUS		status;
    DM1P_BUILD_SMS_CB	SMS_build_cb;
    i4			*err_code = &dberr->err_code;


    do
    {
	location.loc_name = *loc_name;
	location.loc_status = LOC_VALID;

	/* Initialise DMP_LOC_ENTRY */

	STRUCT_ASSIGN_MACRO( *loc_name, loc_entry.logical );
	loc_entry.flags = ( DCB_ROOT | DCB_DATA );
	loc_entry.phys_length = path_length;
	STRUCT_ASSIGN_MACRO( *path_name, loc_entry.physical );

	location.loc_ext = &loc_entry;

	/* Initialise local DMP_FCB */

	loc_fcb_ptr->fcb_tcb_ptr = 0;
	loc_fcb_ptr->fcb_state	 = FCB_OPEN;
	loc_fcb_ptr->fcb_locklist = 0;
	loc_fcb_ptr->fcb_deferred_allocation = 0;
	dm0s_minit( &loc_fcb_ptr->fcb_mutex, "dmmcre FCB mutex" );
	loc_fcb_ptr->fcb_last_page = -1;
	loc_fcb_ptr->fcb_namelength = 12;
	STRUCT_ASSIGN_MACRO( *file_name, loc_fcb_ptr->fcb_filename );
	loc_fcb_ptr->fcb_di_ptr  = di_context;
	loc_fcb_ptr->fcb_location = &loc_entry;
	location.loc_fcb =  loc_fcb_ptr;
	loc_fcb_ptr->fcb_physical_allocation = *pages_allocated;
	loc_fcb_ptr->fcb_logical_allocation = *pages_allocated;

	/*
	** Setup the space management build control block
	*/
	SMS_build_cb.SMS_pages_allocated = *pages_allocated;
	SMS_build_cb.SMS_pages_used	 = *next_pageno;
	SMS_build_cb.SMS_fhdr_pageno	 = DM_TBL_INVALID_FHDR_PAGENO;
	SMS_build_cb.SMS_extend_size	 = DM_TBL_DEFAULT_EXTEND;
	SMS_build_cb.SMS_page_type       = page_type;
	SMS_build_cb.SMS_page_size	 = page_size;
	SMS_build_cb.SMS_start_pageno	 = 0;
	SMS_build_cb.SMS_build_in_memory = FALSE;
	SMS_build_cb.SMS_new_table	 = TRUE;
	SMS_build_cb.SMS_location_array  = &location;
	SMS_build_cb.SMS_loc_count	 = 1;
	SMS_build_cb.SMS_inmemory_rcb	 = (DMP_RCB *)0;
	SMS_build_cb.SMS_plv		 = plv;
	STRUCT_ASSIGN_MACRO( *tab_name, SMS_build_cb.SMS_table_name );
	STRUCT_ASSIGN_MACRO( *dbname, SMS_build_cb.SMS_dbname);

	status = dm1p_build_SMS( &SMS_build_cb, dberr );
	if ( status != E_DB_OK )
	    break;

	*next_pageno     = SMS_build_cb.SMS_pages_used;
	*pages_allocated = SMS_build_cb.SMS_pages_allocated;
	*fhdr_pageno	 = SMS_build_cb.SMS_fhdr_pageno;
	

    } while (FALSE);

    dm0s_mrelease( &loc_fcb_ptr->fcb_mutex );

	
    if ( status == E_DB_OK )
	return( E_DB_OK );


    /*
    ** Log dm1p error message
    */
    uleFormat( dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    /*
    ** return this error up
    */
    SETDBERR(dberr, 0, E_DM92FB_DMM_ADD_SMS);
    return(status );
}

/*{
** Name: dmm_init_catalog_templates - initialize templates for core catalogs
**
**
** Description:
**	This routine initializes certain fields in the templates for the
**	core catalogs.  This routine is called once at DMF startup.
**
**	More specifically, this routine fills in the attribute count in
**	the template IIRELATION tuples, the attribute number fields in
**	the IIATTRIBUTE tuples, and space pads out to DB_MAXNAME positions
**	the name fields in IIRELATION, IIATTRIBUTE, and IIRELIDX.
**
**	If you add new core catalogs and don't account for them here,
**	your server will blow chunks at startup time.
**
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**      Returns:
**
**          E_DB_OK
**          E_DB_FATAL
**      Exceptions:
**          none
**
** Side Effects:
**          None.
** History:
**      4-nov-92 (rickh)
**         Created.
**	6-jan-92 (rickh)
**	   Fill in reltups, too.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Initialize upper-case versions of templates.
**	18-oct-1993 (kwatts) Bug # 45676
**	    fill in iirelation.relcreate for the core catalog entries using 
**	    TMsecs().
**	20-oct-1993 (rmuth)
**	    - Fixed problem initialising uppercase templates, where we
**	      running over the end-of-array.
**	    - Fix incompatble argument errors on STmove calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Verify DMP_NUM_IIRELATION_ATTS.
**	06-mar-1996 (stial01 for bryanp)
**	    Verify that each catalog's tuple width is correct.
**	12-apr-1999 (nanpr01)
**	    Initialize the new fields for iiattribute catalogs correctly.
**	03-mar-2004 (gupsh01)
**	    Added attver_altcol for alter table alter column support.
**	15-dec-04 (inkdo01)
**	    Init attcollID column.
**	30-May-2006 (jenjo02)
**	    Removed DMP_NUM_IIRELATION_ATTS check; dm2u_sysmod can
**	    figure this out now from tcb_rel.relatts...
**	29-Sept-2009 (troal01)
**		Initialize geomtype and srid.
*/

static DB_TAB_ID IIRELATION_TABLE =	IIRELATION_TABLE_ID;
static DB_TAB_ID IIREL_IDX_TABLE =	IIREL_IDX_TABLE_ID;
static DB_TAB_ID IIATTRIBUTE_TABLE =	IIATTRIBUTE_TABLE_ID;
static DB_TAB_ID IIINDEX_TABLE =	IIINDEX_TABLE_ID;

#define SAME_TABLE( tableDesc, tableName )	\
	( ( tableDesc.db_tab_base == tableName.db_tab_base )	\
	  &&	\
	  ( tableDesc.db_tab_index == tableName.db_tab_index )	\
	)

struct	catalogStats
{
	i2	columnCount;
	i4	tupleWidth;
	i4	colnametot;
};


DB_STATUS 
dmm_init_catalog_templates( )
{
    DMP_RELATION	*rel;
    DMP_ATTRIBUTE	*att;
    DMP_RINDEX		*ridx;
    DMP_RELATION	*u_rel;
    DMP_ATTRIBUTE	*u_att;
    DMP_RINDEX		*u_ridx;
    i4			tuplesInIIRELATION;
    i4			tuplesInIIATTRIBUTE;
    i4			tuplesInIIRELIDX;
    i4			tuplesInIIINDEX;
    i4			tupleCount;
    STATUS		status = E_DB_OK;
    i4			i;
    struct		catalogStats iirelationStats = { 0, 0, 0 };
    struct		catalogStats iirelidxStats = { 0, 0, 0 };
    struct		catalogStats iiattributeStats = { 0, 0, 0};
    struct		catalogStats iiindexStats = { 0, 0, 0 };
    struct		catalogStats	*catalogStats;
    DB_ERROR		local_dberr;
    i4			local_err_code;

    tuplesInIIRELATION = DM_CORE_REL_CNT;
    tuplesInIIRELIDX = DM_CORE_RINDEX_CNT;
    tuplesInIIATTRIBUTE = DM_core_att_cnt;
    tuplesInIIINDEX = DM_core_index_cnt;

    for ( ; ; )	/* code block to break out of */
    {

      /*
      ** Initialize the attribute number in each template IIATTRIBUTE tuple.
      ** Space pad out to DB_MAXNAME characters each attribute name.
      ** For each core catalog, count its number of attributes.
      */

      for (	att = DM_core_attributes,
		u_att = DM_ucore_attributes, i = 0;
		i < tuplesInIIATTRIBUTE;
		att++,
		u_att++, i++
	  )
      {
	    if ( SAME_TABLE( att->attrelid, IIRELATION_TABLE ) )
		catalogStats = &iirelationStats;
	    else if ( SAME_TABLE( att->attrelid, IIREL_IDX_TABLE ) )
		catalogStats = &iirelidxStats;
	    else if ( SAME_TABLE( att->attrelid, IIATTRIBUTE_TABLE ) )
		catalogStats = &iiattributeStats;
	    else if ( SAME_TABLE( att->attrelid, IIINDEX_TABLE ) )
		catalogStats = &iiindexStats;
	    else
	    {
		SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);
		status = E_DB_FATAL;
		break;
	    }

	    if ( catalogStats->tupleWidth != att->attoff )
	    {
		SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);
		status = E_DB_FATAL;
		break;
	    }

	    catalogStats->columnCount++;
	    catalogStats->tupleWidth += att->attfml;
	    catalogStats->colnametot += STlength(att->attname.db_att_name);
	    att->attid = catalogStats->columnCount;
	    att->attintl_id = catalogStats->columnCount;
	    att->attver_added = 0;
	    att->attver_dropped = 0;
	    att->attval_from = 0;
	    att->attver_altcol = 0;
	    att->attcollID = -1;
	    att->attgeomtype = -1;
	    att->attsrid = -1;
	    att->attencflags = 0;
	    att->attencwid = 0;
	    MEfill(sizeof(att->attfree), 0, att->attfree);
	    if (att->attfmt == ATT_CHA)
	    {
		att->attDefaultID.db_tab_base = DB_DEF_ID_BLANK;
		att->attDefaultID.db_tab_index = 0;
	    }
	    else 
	    {
		att->attDefaultID.db_tab_base = DB_DEF_ID_0;
		att->attDefaultID.db_tab_index = 0;
	    }

	    if ( SAME_TABLE( att->attrelid, IIRELATION_TABLE ) &&
		(att->attid == DM_RELID_FIELD_NO 
			&& STcompare(att->attname.db_att_name, "relid"))
		|| (att->attid == DM_RELOWNER_FIELD_NO 
			&& STcompare(att->attname.db_att_name, "relowner") ))
	    {
		SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);
		status = E_DB_FATAL;
		break;
	    }

	    /* AFTER initializing attribute record, copy to u_att */
	    STRUCT_ASSIGN_MACRO(*att, *u_att);
	    CVupper(u_att->attname.db_att_name);

	    /* pad the attribute name, do this last */
	    STmove( att->attname.db_att_name, ' ',
		    sizeof( att->attname.db_att_name ),
		    att->attname.db_att_name );

	    STmove( u_att->attname.db_att_name, ' ',
		    sizeof( u_att->attname.db_att_name ),
		    u_att->attname.db_att_name );

      }	/* end for (end of tour through IIATTRIBUTE */
      if ( status != E_DB_OK ) break;

      /*
      ** Cross check the tuple widths. Unfortunately, sizeof(DMP_INDEX) is
      ** greater than the iiindex tuple width, due to alignment. So it gets
      ** special cased here.
      */
      if (iirelationStats.tupleWidth != sizeof(DMP_RELATION) ||
	    iiattributeStats.tupleWidth != sizeof(DMP_ATTRIBUTE) ||
	    iiindexStats.tupleWidth > sizeof(DMP_INDEX) ||
	    iirelidxStats.tupleWidth != sizeof(DMP_RINDEX))
      {
	SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);
	status = E_DB_FATAL;
	break;
      }

	TRdisplay("iirelation: cols=%d, width=%d, struct size=%d\n",
		    iirelationStats.columnCount, iirelationStats.tupleWidth,
		    sizeof(DMP_RELATION));
	TRdisplay("iiattribute: cols=%d, width=%d, struct size=%d\n",
		    iiattributeStats.columnCount, iiattributeStats.tupleWidth,
		    sizeof(DMP_ATTRIBUTE));
	TRdisplay("iiindex: cols=%d, width=%d, struct size=%d\n",
		    iiindexStats.columnCount, iiindexStats.tupleWidth,
		    sizeof(DMP_INDEX));
	TRdisplay("iirelidx: cols=%d, width=%d, struct size=%d\n",
		    iirelidxStats.columnCount, iirelidxStats.tupleWidth,
		    sizeof(DMP_RINDEX));


      /*
      ** For each template tuple in IIRELATION, fill in the attribute count
      ** of the corresponding catalog.  Also, space pad out to DB_MAXNAME
      ** places the catalog name, catalog owner, and default location.
      ** Also use server default page size
      */

      for ( rel = DM_core_relations,
	    u_rel = DM_ucore_relations, i = 0;
	    i < tuplesInIIRELATION;
	    rel++,
	    u_rel++, i++
	  )
      {
	    if ( SAME_TABLE( rel->reltid, IIRELATION_TABLE ) )
	    {
		catalogStats = &iirelationStats;
		tupleCount = tuplesInIIRELATION;
	    }
	    else if ( SAME_TABLE( rel->reltid, IIREL_IDX_TABLE ) )
	    {
		catalogStats = &iirelidxStats;
		tupleCount = tuplesInIIRELIDX;
	    }
	    else if ( SAME_TABLE( rel->reltid, IIATTRIBUTE_TABLE ) )
	    {
		catalogStats = &iiattributeStats;
		tupleCount = tuplesInIIATTRIBUTE;
	    }
	    else if ( SAME_TABLE( rel->reltid, IIINDEX_TABLE ) )
	    {
		catalogStats = &iiindexStats;
		tupleCount = tuplesInIIINDEX;
	    }
	    else
	    {
		SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);
		status = E_DB_FATAL;
		break;
	    }

	    if (rel->relwid != catalogStats->tupleWidth)
	    {
		SETDBERR(&local_dberr, 0, E_DM004A_INTERNAL_ERROR);
		status = E_DB_FATAL;
		break;
	    }

	    MEfill(sizeof(rel->relfree), 0, rel->relfree);

	    /* Fill in the relcreate field */
	    rel->relcreate = TMsecs();
	    rel->relatts = catalogStats->columnCount;
	    rel->relattnametot = catalogStats->colnametot;
	    rel->reltups = tupleCount;

	    /* AFTER initializing iirelation record, copy to u_rel */
	    STRUCT_ASSIGN_MACRO(*rel, *u_rel);
	    CVupper(u_rel->relid.db_tab_name);
	    CVupper(u_rel->relowner.db_own_name);
	    CVupper(u_rel->relloc.db_loc_name);

	    /* space pad the character strings */
	    STmove( rel->relid.db_tab_name, ' ',
		    sizeof( rel->relid.db_tab_name ),
		    rel->relid.db_tab_name );
	    STmove( u_rel->relid.db_tab_name, ' ',
		    sizeof( u_rel->relid.db_tab_name ),
		    u_rel->relid.db_tab_name );
	    STmove( rel->relowner.db_own_name, ' ',
		    sizeof( rel->relowner.db_own_name ),
		    rel->relowner.db_own_name );
	    STmove( u_rel->relowner.db_own_name, ' ',
		    sizeof( u_rel->relowner.db_own_name ),
		    u_rel->relowner.db_own_name );
	    STmove( rel->relloc.db_loc_name, ' ',
		    sizeof( rel->relloc.db_loc_name ),
		    rel->relloc.db_loc_name );
	    STmove( u_rel->relloc.db_loc_name, ' ',
		    sizeof( u_rel->relloc.db_loc_name ),
		    u_rel->relloc.db_loc_name );

      }	/* end for (tour of IIRELATION) */
      if ( status != E_DB_OK ) break;

      /*
      ** Space pad the catalog name and owner for the template tuples
      ** in IIRELIDX.
      */

      for (	ridx = DM_core_rindex,
		u_ridx = DM_ucore_rindex, i = 0;
		i < tuplesInIIRELIDX;
		ridx++,
		u_ridx++, i++
	  )
      {
	    STRUCT_ASSIGN_MACRO(*ridx, *u_ridx);
	    CVupper(u_ridx->relname.db_tab_name);
	    CVupper(u_ridx->relowner.db_own_name);

	    STmove( ridx->relname.db_tab_name, ' ',
		    sizeof( ridx->relname.db_tab_name ),
		    ridx->relname.db_tab_name );
	    STmove( u_ridx->relname.db_tab_name, ' ',
		    sizeof( u_ridx->relname.db_tab_name ),
		    u_ridx->relname.db_tab_name );
	    STmove( ridx->relowner.db_own_name, ' ',
		    sizeof( ridx->relowner.db_own_name ),
		    ridx->relowner.db_own_name );
	    STmove( u_ridx->relowner.db_own_name, ' ',
		    sizeof( u_ridx->relowner.db_own_name ),
		    u_ridx->relowner.db_own_name );

      }	/* end for (tour of IIINDEX) */

      break;
    }	/* end of code block */

    if (status)
    {
	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
		   ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		   &local_err_code, 0);
    }

    return( status );
}
