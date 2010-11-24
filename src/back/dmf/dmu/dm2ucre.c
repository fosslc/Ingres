/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <mh.h>
#include    <pc.h>
#include    <sr.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <hshcrc.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulb.h>
#include    <ulf.h>
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
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm1c.h>
#include    <dm1p.h>
#include    <dm1s.h>
#include    <dmfgw.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmpecpn.h>
#include    <cm.h>
#include    <st.h>
#include    <cui.h>
#include    <adurijndael.h>

/**
**
**  Name: DM2UCREATE.C - Create table utility operation.
**
**  Description:
**      This file contains routines that perform the create table
**	functionality.
**	    dm2u_create	- Create a permanent table.
**
**  History:
**	20-may-1989 (Derek)
**	    Split out from DM2U.C
**	05-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Changed iiapplication_id to iirole.
**      22-may-89 (jennifer)
**          Added B1 code to add ii_sec_label and ii_sec_tabkey to
**          all tables created for B1 system.
**      22-jun-89 (jennifer)
**          Added checks for security (DAC) to insure that only a user
**          with security privilege is allowed to create or delete
**          a secure table.  A secure table is indicated by having
**          relstat include TCB_SECURE.  Also changed the default
**          definitions for iiuser, iilocations, iidbaccess,
**          iiusergroup, iirole and iidbpriv to include TCB_SECURE.
**          The only secure tables are in the iidbdb and include those
**          listed above plus the iirelation of the iidbdb (so that
**          no-one can turn off the TCB_SECURE state of the other ones).
**      27-jun-89 (jennifer)
**          Added definition of iisecuritystate to system catalogs.
**	15-jul-89 (ralph)
**	    ORANGE:  Added iisecuritystate to systab[].
**      17-jul-89 (jennifer)
**          Don't mark frontend catalogs as system catalogs for
**          C2 systems.  This means that they are owned by the
**          dba, not $ingres.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of create operation.
**	16-jan-90 (ralph)
**	    Added iidbloc to systab[].
**	24-jan-1990 (fred)
**	    Added support for creation of extension tables.
**	13-feb-90 (carol)
**	    Added iidbms_comment and iisynonym to systab[].
**	04-jun-90 (linda)
**	    Add gwrowcount parameter, so that reltups can reflect the value
**	    (if any) provided by the user when registering a table.
**	14-jun-90 (linda,bryanp)
**	    Don't set relstat bit to reflect GW_NOUPDT until just before
**	    updating iirelation table.  Code tests relstat and assumes that
**	    if any bits are set, it's either a system catalog or an extended
**	    system catalog.
**      17-jul-90 (mikem)
**          bug #30833 - lost force aborts
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	14-jun-1991 (Derek)
**	    Added support for new allocation and extend options.
**	14-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-dec-91 (rogerk)
**		Added support for Set Nologging.
**	15-oct-1991 (bryanp)
**	    Terminate an un-terminated comment.
**	7-Feb-1992  (rmuth)
**	    Added parameter to dm2f_file_size.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	29-may-92 (andre)
**	    added IIPRIV to systab[]
**	10-jun-92 (andre)
**	    Added IIXPRIV to systab[]
**	18-jun-92 (andre)
**	    added IIXPROCEDURE and IIXEVENT to systab[]
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**    09-jun-1992 (kwatts)
**        6.5 MPF Changes. Setting of relcomptype and relpgtype for new
**        table. Adding accessor parameter to call of dm2f_init_file.
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  added new system catalogs:  iiintegrityidx,
**	    iiruleidx, iikey, iidefault, iidefaultidx.
**	17-aug-92 (rickh)
**	    Added relstat2 argument to dm2u_create.
**	29-August-1992 (rmuth)
**	   - Removed the 6.4->6.5 on-the-fly conversion code.
**	   - Added constants for default allocation,extend and relfhdr.
**	   - Zero fill relrecord before using it.
**	   - Correct setting of relpages.
**	11-sep-1992 (bryanp)
**	    Fix typo: pass address of relrecord to MEfill().
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	08-oct-1992 (robf)
**	    Set the RCB JOURNAL flag when doing a gateway create so
** 	    that all pertinent REGISTER information is written to the
**	    journals, otherwise rollforward will lose part of the register
**	    info (iiqrytext and extended catalogs will be rolled forward,
**	    but the base info will be missing)
**	09-oct-1992 (jnash)
**	    Reduced logging Project.
**	    --  Set up SYSCAT page accessor type for SCONCUR tables
**		AND for iirtemp and iiridxtemp.
**	16-oct-1992 (rickh)
**	    Added default id transcription for attributes.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	9-nov-92 (ed)
**	    change DB_MAXNAME constants from 24 to 32
**      02-nov-92 (anitap)
**          Added IISCHEMA and IISCHEMAIDX to systab[].
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	14-dec-1992 (rogerk & jnash)
**	    Reduced Logging Project:
**	    - Fill in loc_config_id in loc_array,
**	    - New params to dm0l_fcreate, dm0l_create and dm0l_dmu.
**	    - Replace dm2f_init_file with dm1s_empty_table.
**	    - Add dm0l_crverify log record.
**	    - Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	13-jan-1993 (jnash)
**	    Eliminate prev_lsn param in dm0l_create(). Create has no CLR.
**	22-jan-93 (anitap)
**	    Change iischema and iischemaidx from 24 chars to 32 chars.
**	10-feb-93 (rickh)
**	    Added iiprocedure_parameter.
**	30-feb-1993 (rmuth)
**	    When creating an INDEX or a VIEW the code updates the base
**	    relation to reflect that one of the above exists on the table.
**	    During the above we overwrote the iirelation information we
**	    had built up for the table which was being used later on in
**	    the code. Fixed by reading the base table into another variable.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**	15-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Use STbcompare instead of MEcmp where appropriate to treat
**	    reserved table names and prefixes case insensitively.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix iiattribute pages after updating iiattribute. This is
**		    needed to comply with cluster node failure recovery's rule
**		    that a transaction may only fix one system catalog page
**		    at a time.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Make table names case sensitive according to case translation
**	    semantics of the session.
**	    Don't validate table owner name; trust upstream validation.
**	    Use scb_cat_owner rather than $ingres.
**	    Use case insensitive comparison with "ii" to determine if catalog.
**	14-may-1993 (robf)
**	    Add security label id catalogs (iiseclabid/idx)
**	15-may-1993 (rmuth)
**	    Concurrent-access index build changes.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	27-may-1993 (robf)
**	    Secure 2.0: Remove ORANGE code, replace by new functionality.
**	15-jun-93 (rickh)
**	    Added IIXPROTECT index on the IIPROTECT catalog.
**	21-June-1993 (rmuth)
**	    File Extend: Make sure that for multi-location tables that
**	    allocation and extend size are factors of DI_EXTENT_SIZE.
**	30-jun-1993 (shailaja)
**	    Fixed prototype incompatibility in LKrequest.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-jul-1993 (jnash)
**	    Add param to dm1s_empty_table().
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the create process for new recovery protocols.
**	    Divorced the file creation from the table build operation and
**	    moved both of these to before the system catalog updates.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.
**	  - Added relstat to dm0l_create log record.
**	31-aug-93 (robf)
**	    Add iiprofile to list of catalogs
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	  - Fixed bug introduced when create steps were re-ordered in the
**	    last integration.  Filled in relstat values for new table before
**	    writing create log record.
**	  - Bypass writing of CRVERIFY log record for gateway and view
**	    creations as the dmve_crverify code cannot be executed on these
**	    objects during rollforward.
**	24-feb-94 (arc)
**	    Add NO_OPTIM for su4_cmw to resolve errors creating iidevices
**	    table during 'iidbdb' creation, using Sun 'acc' (SC1.0)
**	7-apr-94 (stephenb)
**	    Fix bug 62084 by making iiextended_relation a non-security catalog.
**	8-apr-1994 (rmuth)
**	  b60701, when creating a table do not increase the dmu_cnt field.
**	  This is used to uniquely identify filenames during muiltple
**	  modifies of the same table within the same transaction. As
**	  the filename is based of the table id and all tables have a
**	  unique table id it is not needed here.
**	09-feb-1995 (cohmi01)
**	    For RAW I/O, accept new rawextnm param, listing any extent names
**	    that user provided, indicating where in raw file to put table.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Examine the usage when checking for a logical name in the
**	    config file.
**	26-may-1995 (shero03)
**	    For RTree add iirange
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for specifiable page size.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**          Pass page size to dm2f_build_fcb()
**      06-may-1996 (thaju02)
**          New Page Format Project:
**              Pass page size to dm1c_getaccessors().
**      09-jul-1996 (thaju02)
**	    Modified dm2u_maxreclen().
**          Maximum tuple length for 64k pages is (32k - page overhead).
**          Limitation due to system catalog fields being i2 (e.g.
**          iirelation.relwid and iiattribute.attoff).
**      22-jul-1996 (ramra01 for bryanp)
**          Alter Table Project:
**          Initialize relversion to 0 when creating new table.
**          Initialize reltotwid to the relwid when creating new table.
**          Initialize attintl_id, attver_added, attver_dropped, attval_from,
**          and attfree for each attribute record when creating new table.
**	13-aug-1996 (somsa01)
**	    Added DM2U_ETAB_JOURNAL for enabling journal after next checkpoint
**	    for etab tables.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	23-sep-96 (kch)
**	    In dm2u_create(), added check for invalid location information. This
**	    change fixes bug 78656.
**	12-nov-96 (hanch04)
**	    Check to see if ext is NULL when seaching for valid locations.
**	    Bug 79325
**	31-Jan-1997 (jenjo02)
**	    Removed some dead code in dm2u_create():
**		lk_mode = DM2T_X;
**		if (tbl_id->db_tab_base <= DM_SCONCUR_MAX)
**		    lk_mode = DM2T_IX;
**	    tbl_id is an output parm and its contents unpredictable.
**	    lk_mode is never used, so this is dead (and potentially deadly)
**	    code.
**	18-mar-1997 (rosga02)
**          Integrated changes for SEVMS
**          18-jan-95 (harch02)
**            Bug #74213
**            If supplied use the given security label id to create the table.
**            Needed to assign the base table security label id to an extension
**            table label id. They must match.
**	21-Mar-1997 (jenjo02)
**	    Table priority project.
**	    Prototype changed to pass in Table priority.
**	17-may-97 (devjo01)
**	    Free 'loc_cb' AFTER last use of suballocated array 'loc_array'.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	26-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	03-Dec-1998 (jenjo02)
**	    Pass XCB*, page size, extent start to dm2f_alloc_rawextent().
**	12-mar-1999 (nanpr01)
**	    Support for raw locations & get rid of raw extents.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**	20-may-1999 (nanpr01)
**	    Added new secondary index in the iirule catalog.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	07-Jun-1999 (shero03)
**	    Hash the owner & table_name when obtaining a lock.
**	29-Jun-1999 (jenjo02)
**	    When RAW files are involved, allocate the lesser of
**	    "allocation" or raw_totalblks.
**      03-aug-1999 (stial01)
**          dm2u_create() Create etab tables for any peripheral atts
**      10-sep-1999 (stial01)
**          dm2u_create() etab page size will be determined in dmpe
**      21-dec-1999 (stial01)
**          Use TCB_PG_V3 page type for etabs having page size > 2048
**      12-jan-2000 (stial01)
**          Set TCB2_BSWAP in relstat2 for tables having etabs (Sir 99987)
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-sep-2000 (stial01)
**          Disable vps etab.
**	07-mar-2002 (rigka01) bug#103857
**	    When attempting to create a view using a blob, E_US1263 is 
**	    received at the front end and E_DM0072, E_DM9B07, and E_DM9028
**	    are written to the error log. Ensure dmpe_create_extension is
**	    not called in the case of a view containing a blob. 
**      01-feb-2001 (stial01)
**          Pass page type to dm1s_empty_table (Variable Page Types SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	23-Apr-2001 (jenjo02)
**	    Deleted dm2u_raw_totalblks() function, plant code inline.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	19-Dec-2003 (schka24)
**	    Add new partitioned table catalogs to the ii-name table.
**	    (Which holds DB_TAB_NAMEs, not DB_OWN_NAMEs!)
**      02-jan-2004 (stial01)
**          Removed DM2U_ETAB_JOURNAL
**	29-Jan-2004 (schka24)
**	    Turns out we didn't need iipartition.
**	11-Feb-2004 (schka24)
**	    Munge table create to do partitions too.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**          dm2u_maxreclen, increase max record length for 64k pages since i2
**          limit of 32767 has been lifted
**          Removed tuple length limit of svcb_maxtuplen
**	29-Dec-2004 (schka24)
**	    Remove pointless index iixprotect from systab.
**	26-jan-2005 (abbjo03)
**	    Do not create a physical file for an index at this point.
**	10-feb-2005 (abbjo03)
**	    The 26-jan change was needed to prevent multiple file versions from
**	    being created on VMS as a result of a CREATE INDEX, something that
**	    did not occur prior to the partitioning code.  However, the change
**	    causes other, more serious problems elsewhere so it is being undone
**	    until a more complete solution is found.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**      04-nov-2008 (stial01)
**          dm2u_create() Define catalog names without blanks.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1s_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dmf_gw? functions converted to DB_ERROR *
**      11-dec-2008 (stial01)
**          dm2u_create() nov-4 change missed one catalog name
**	26-Aug-2009 (kschendel) 121804
**	    Need hshcrc.h to satisfy gcc 4.3.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	01-apr-2010 (toumi01) SIR 122403
**	    Support for column encryption.
**      10-Feb-2010 (maspa05) b122651
**          Added TCB2_PHYSLOCK_CONCUR to iidevices and iisequence as this is 
**          how we check for the need for physical locks now
**      14-May-2010 (stial01)
**          Alloc/maintain exact size of column names (iirelation.relattnametot)
**	25-May-2010 (kschendel)
**	    Add missing mh include.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/

/*
NO_OPTIM=su4_cmw i64_aix
*/

/*{
** Name: dm2u_create	- Create a permanent table.
**
** Description:
**      This routine is called from dmu_create() to create a permanent table.
**	The table to be created can be an index table, view or normal table.
**
** Inputs:
**	dcb				The database id.
**	xcb				The transaction id.
**	table_name			Name of the table to be created.
**	owner				Owner of the table to be created.
**	location			Location where the table will reside.
**      l_count                         Number of locations given.
**	tbl_id				The internal table id of the base table
**					if create an index table. Otherwise
**					this wil be an output parameter.
**	index				TRUE if create an index table.
**	view				TRUE if create a view.
**	relstat				A mask indicating attributes of the
**					table - one or more of the following:
**					  TCB_JOURNAL 	    - journal rel
**					  TCB_DUPLICATES   - allow duplicates
**					  TCB_CATALOG 	    - base catalogue
**					  TCB_EXTCATALOG   - extended catalogue
**					  TCB_SYS_MAINTAINED -
**						contains at least one attribute
**						(ie. a logical key, ...)
**					  TCB_GATEWAY	    - gateway table
**					  TCB_GW_NOUPDT	    - gateway table
**					  TCB_VGRANT_OK - DMT_VGRANT_OK must be
**						set (views only)
**	relstat2			More flags pre-set for relstat2
**	ntab_width			Width in bytes of the table record
**					width.
**	attr_count			Number of attributes in the table.
**	attr_entry			Pointer to an attribute description
**					array of pointers.
**	db_lockmode			Lockmode of the database for the caller.
**	allocation			Table space pre-allocation amount.
**	extend				Out of space extend amount.
**	page_size			Page size for table.
**	gwattr_array			Gateway attributes - valid only for
**					a gateway table.
**	gwchar_array			Gateway characteristics - valid only for
**					a gateway table.
**	gw_id				The gateway id
**	gwrowcount			The number of rows in the gateway table
**					at the time it is registered.
**	gwsource			The 'source' for the gateway object.
**	dmu_chars			Address of DMU_CHARACTERISTICS
**	tbl_pri				Table's cache priority.
**
** Outputs:
**	tbl_id				The internal table id assigned to the
**					created table.
**	idx_id				The internal table id assigned to the
**					created index table.
**      qry_id                          Pointer to query id for a view or a GW
**					table or index.
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
**      28-may-86 (ac)
**          Created from dmu_create().
**	15-Aug-88 (teg)
**	    Stop setting TCB_ZOPTSTAT relstat bit during creation of sys
**	    catalog.  Also modify to call dm2u_get_tabid to get table id from
**	    config file.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_create_file() and
**	    dm2f_build_fcb() calls.
**	08-Nov-88 (teg)
**	    modified DM2U_CREATE to use DB_ERROR instead of i4 for err
**	    code, so that it can return parameters.  Also modified it to return
**	    index to bad location in location array.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-feb-89 (EdHsu)
**	    Add call to dm2u_ckp_lock to be mutually exclusive with ckp.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added iiusergroup, iiapplication to systab[].
**	06-mar-89 (neil)
**          Rules: Added system table iirule.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added iidbpriv to systab[] array in dm2u_create.
**	17-apr-89 (mikem)
**	    Logical key development.  Support building a table with a
**	    system maintained column.
**      22-may-89 (jennifer)
**          Added B1 code to add ii_sec_label and ii_sec_tabkey to
**          all tables created for B1 system.
**      22-jun-89 (jennifer)
**          Added checks for security (DAC) to insure that only a user
**          with security privilege is allowed to create or delete
**          a secure table.  A secure table is indicated by having
**          relstat include TCB_SECURE.  Also changed the default
**          definitions for iiuser, iilocations, iidbaccess,
**          iiusergroup, iirole and iidbpriv to include TCB_SECURE.
**          The only secure tables are in the iidbdb and include those
**          listed above plus the iirelation of the iidbdb (so that
**          no-one can turn off the TCB_SECURE state of the other ones).
**       27-jun-89 (jennifer)
**          Added definition of iisecuritystate to system catalogs.
**       17-jul-89 (jennifer)
**          Don't mark frontend catalogs as system catalogs for
**          C2 systems.  This means that they are owned by the
**          dba, not $ingres.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Accept DMU_GATEWAY type table.
**	    Added support for creating/destroying tables that have no
**	    underlying physical file.  Call dmf_gwt_register to register the
**	    table with the gateway.  Added gateway characteristic parameters.
**	    Pass these down directly to the register routine, let the gateway
**	    parse these options.  Update iirelation to show gateway id.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of the Create.
**	25-oct-89 (neil)
**          Alerters: Added system table iievent.
**	16-jan-90 (ralph)
**	    Added iidbloc to systab[].
**	24-jan-1990 (fred)
**	    Added support for creation of table extensions.  This amounts to
**	    setting the appropriate relstat bits, and to setting up the table
**	    name for the table.  The latter is done because table names are set
**	    for extension based on the underlying table id, which isn't known
**	    until this routine is called.
**
**	    Also added support for the new system catalog iiextended_relation,
**	    which catalogs the aforementioned table extensions.  This is a new,
**	    sconcur system catalog -- which exists only in database which
**	    contain tables which need extensions.
**	13-feb-90 (carol)
**	    Added iidbms_comment and iisynonym to systab[].
**	02-mar-90 (andre)
**	    introduced DM2U_VGRANT_OK
**	25-apr-90 (fourt)
**	    get relqid1/2 fields in iirelation set on Gateway tables to match
**	    up with iiqrytext, to make view iiregistrations work.
**	25-may-90 (linda)
**	    Before call to dmf_gwt_register(), close the iirelation table to
**	    clear exclusive lock held for tuple insert.  This is necessary as
**	    the gateway code issues DMT_SHOW calls to get extended catalog
**	    information -- these calls require shared read locks on iirelation
**	    which may conflict if the extended catalog tuple is on the same
**	    page as the new table's tuple.  Only happens with iirelation --
**	    the information requested by the gateway is DMT_M_TABLE only.
**	04-jun-90 (linda)
**	    Add gwrowcount parameter, so that reltups can reflect the value
**	    (if any) provided by the user when registering a table.
**	14-jun-90 (linda,bryanp)
**	    Don't set relstat bit to reflect GW_NOUPDT until just before
**	    updating iirelation table.  Code tests relstat and assumes that
**	    if any bits are set, it's either a system catalog or an extended
**	    system catalog.
**      17-jul-90 (mikem)
**          bug #30833 - lost force aborts
**          LKrequest has been changed to return a new error status
**          (LK_INTR_GRANT) when a lock has been granted at the same time as
**          an interrupt arrives (the interrupt that caused the problem was
**          a FORCE ABORT).   Callers of LKrequest which can be interrupted
**          must deal with this new error return and take the appropriate
**          action.  Appropriate action includes signaling the interrupt
**          up to SCF, and possibly releasing the lock that was granted.
**	    In the case of dm2u_create() just handle the new return the same
**	    as LK_INTERRUPT (the lock gets released as part LK release all).
**	14-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**	15-oct-1991 (bryanp)
**	    Terminate an un-terminated comment.
**	7-Feb-1992  (rmuth)
**	    Added extra parameter to dm2f_file_size
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    0-fill relfree field in iirelation tuple.
**	29-may-92 (andre)
**	    added IIPRIV to systab[]
**	10-jun-92 (andre)
**	    Added IIXPRIV to systab[]
**	18-jun-92 (andre)
**	    added IIXPROCEDURE and IIXEVENT to systab[]
**    09-jun-1992 (kwatts)
**        6.5 MPF Changes. Setting of relcomptype and relpgtype for new
**        table. Adding accessor parameter to call of dm2f_init_file.
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  added new system catalogs:  iiintegrityidx,
**	    iiruleidx, iikey, iidefault, iidefaultidx.
**	17-aug-92 (rickh)
**	    Added relstat2 argument to dm2u_create.
**	29-August-1992 (rmuth)
**	   - Removed the 6.4->6.5 on-the-fly conversion code.
**	   - Added constants for default allocation,extend and relfhdr.
**	   - Zero fill relrecord before using it.
**	11-sep-1992 (bryanp)
**	    Fix typo: pass address of relrecord to MEfill().
**	30-sep-1992 (robf)
**	    Get char_array to pass onto gateway for more error checking.
**	05-oct-1992 (robf)
**	    Added iigw06_relation/attribute to systab for F-C2 auditing
**	08-oct-1992 (robf)
**	    Set the RCB JOURNAL flag when doing a gateway create so
** 	    that all pertinent REGISTER information is written to the
**	    journals, otherwise rollforward will lose part of the register
**	    info (iiqrytext and extended catalogs will be rolled forward,
**	    but the base info will be missing)
**	    IfDef ORANGE the special code for DBA-owned catalogs, currently
**	    breaks Createdb/Upgradedb and isn't needed for C2
**	21-oct-1992 (jnash)
**	    6.5 Recovery Project
**	    --  Set up SYSCAT page accessor type for SCONCUR tables
**		and for iirtemp and iiridxtemp.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	    Fixed casts to dm2f_init_file to fix compile warnings.
**      02-nov-92 (anitap)
**          Added iischema and iischemaidx to systab[].
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	14-dec-1992 (rogerk & jnash)
**	    Reduced Logging Project (phase 2):
**	    - Fill in loc_config_id in loc_array,
**	    - New params to dm0l_fcreate, dm0l_create and dm0l_dmu.
**	    - Replace dm2f_init_file with dm1s_empty_table.
**	    - Add dm0l_crverify log record.
**	    - Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	18-nov-1992 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	13-jan-1993 (jnash)
**	    Eliminate prev_lsn param in dm0l_create(). Create has no CLR.
**	22-jan-93 (anitap)
**	    Changed iischema and iischemaidx from 24 to 32.
**	10-feb-93 (rickh)
**	    Added iiprocedure_parameter.
**	30-feb-1993 (rmuth)
**	    When creating an INDEX or a VIEW the code updates the base
**	    relation to reflect that one of the above exists on the table.
**	    During the above we overwrote the iirelation information we
**	    had built up for the table which was being used later on in
**	    the code. Fixed by reading the base table into another variable.
**	15-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Use STbcompare instead of MEcmp where appropriate to treat
**	    reserved table names and prefixes case insensitively.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Make table names case sensitive according to case translation
**	    semantics of the session.
**	    Don't validate table owner name; trust upstream validation.
**	    Use scb_cat_owner rather than $ingres.
**	    Use case insensitive comparison with "ii" to determine if catalog.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Unfix iiattribute pages after updating iiattribute. This is
**		    needed to comply with cluster node failure recovery's rule
**		    that a transaction may only fix one system catalog page
**		    at a time.
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Check for and log any errors encountered during error cleanup.
**		Cleaned up and simplified various error-handling paths.
**	15-may-1993 (rmuth)
**	    - Concurrent-access index build change, if a cncurrent index then
**	      - Do not take dm2t_control_lock.
**	      - Share lock the base table.
**	    - Use dm2t_lock_table to request table lock as opposed to explicit
**	      LK request.
**	14-may-1993 (robf)
**	   Add security label mapping catalogs
**	22-may-93 (andre)
**	    upcase (a bool) was being assigned a result of &'ing a u_i4
**	    field and long constant.  On machines where bool is char, this
**	    assignment will not produce the expected result
**	15-jun-93 (rickh)
**	    Added IIXPROTECT index on the IIPROTECT catalog.
**	21-June-1993 (rmuth)
**	    File Extend: Make sure that for multi-location tables that
**	    allocation and extend size are factors of DI_EXTENT_SIZE.
**	23-jul-93 (robf)
**	   Add secid  output param to pass back table security label.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-jul-1993 (jnash)
**	    Add FALSE build_tcb_flag param to dm1s_empty_table() call.
**	    Also, don't log crverify log records for iirtemp and iiridxtemp,
**	    there are no syscat entries available to open these tables
**	    during recovery.
**	23-aug-1993 (rogerk)
**	    Reordered steps in the create process for new recovery protocols.
**	      - We now log/create the table files before doing any other
**		work on behalf of the create operation.  The creation of
**		the physical files was taken out of the dm1s_empty_table
**		routine which now just allocates and formats space.
**	      - The dm1s_empty_table call is now made to build the initial
**		table structure BEFORE logging the dm0l_create log record.
**		This is done so that any non-executable create attempt (one
**		with an impossible allocation for example) will not be
**		attempted during rollforward recovery.
**	      - The system catalog updates are now done after building the
**		table just so that the CREATE log record will come before
**		the system catalog log records.
**	  - Changes to handling of non-journaled tables.  ALL system catalog
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.  Changed DMU to
**	    to be a journaled log record.
**	  - Added relstat to dm0l_create log record so we can determine in
**	    recovery whether the relation is a gateway table or not.
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	  - Collapsed gateway journal logic into "journal_flag" setting,
**	    removing the DM0L_JOURNAL log record flag being written for
**	    gateway creates in non-journaled databases.
**	  - Fixed bug introduced when create steps were re-ordered in the
**	    last integration.  Filled in relstat values for new table before
**	    writing create log record.
**	  - Bypass writing of CRVERIFY log record for gateway and view
**	    creations as the dmve_crverify code cannot be executed on these
**	    objects during rollforward.
**	  - Disable old "table will be journaled at next checkpoint" as all
**	    tables are now created with journaling by default.
**	  - Turn on JON (journal at next checkpoint) for ingres catalogs.
**	8-oct-93 (robf)
**          Set error to E_DM9112_DEF_LABEL_ERR if unable to generate
**	    default security label when creating the table.
**	15-oct-93 (andre)
**	    Added IIXSYNONYM to systab[]
**	27-oct-93 (andre)
**	    if creating a view or a gateway object, compute the query text id
**	    by pumping object id through ulb_rev_bits().  Store the query text
**	    id in *qry_id for the caller to use in IIQRYTEXT and IITREE (for
**	    views) tuples
**	26-nov-93 (robf)
**          Add iisecalarm to table list
**	12-dec-93 (robf)
**           Remove final ORANGE code, we now handle catalog ownership the
**	     same for all (base/C2/B1) DBMS. Also don't set TCB_VBASE always,
**	     this caused VERIFYDB problems and should be handled nicer when
**	     creating views.
**	7-mar-94 (robf)
**           Add iirolegrant to catalog list
**	7-apr-94 (stephenb)
**	    Fix bug 62084 by removing TCB_SECURE bit from iiextended_relation
**	    this is an extended catalog and should not require securuty
**	    privilege.
**	8-apr-1994 (rmuth)
**	  b60701, when creating a table do not increase the dmu_cnt field.
**	  This is used to uniquely identify filenames during muiltple
**	  modifies of the same table within the same transaction. As
**	  the filename is based of the table id and all tables have a
**	  unique table id it is not needed here.
**	15-jul-94 (robf)
**          Checking for row label/audit override on system catalogs should
**	    be independent of B1, and whether or not the table actually
**	    enabled. Reordered code accordingly.
**	09-feb-1995 (cohmi01)
**	    For RAW I/O, accept new rawextnm param, listing any extent names
**	    that user provided, indicating where in raw file to put table.
**	    Added iirawextents to system catalog list.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Examine the usage when checking for a logical name in the
**	    config file.
**	26-may-1995 (shero03)
**	    For RTree add iirange: dimension, hilbertsize, and range
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for specifiable page size.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm2u_maxreclen to determine maximum record length.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**          Pass page size to dm2f_build_fcb()
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Pass page size to dm1c_getaccessors().
**      22-jul-1996 (ramra01 for bryanp)
**	    Alter Table Project:
**          Initialize relversion to 0 when creating new table.
**	    Initialize reltotwid to the relwid when creating new table.
**          Initialize attintl_id, attver_added, attver_dropped, attval_from,
**          and attfree for each attribute record when creating new table.
**	13-aug-1996 (somsa01)
**	    Added DM2U_ETAB_JOURNAL for enabling journal after next checkpoint
**	    for etab tables.
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	23-sep-96 (kch)
**	    Added check for invalid location information (ext->flags == 0).
**	    This change fixes bug 78656.
**	03-jan-1997 (nanpr01)
**	    Use the define's rather than hardcoded values.
**	31-Jan-1997 (jenjo02)
**	    Removed some dead code in dm2u_create():
**		lk_mode = DM2T_X;
**		if (tbl_id->db_tab_base <= DM_SCONCUR_MAX)
**		    lk_mode = DM2T_IX;
**	    tbl_id is an output parm and its contents unpredictable.
**	    lk_mode is never used, so this is dead (and potentially deadly)
**	    code.
**	21-Mar-1997 (jenjo02)
**	    Table priority project.
**	    Prototype changed to pass in Table priority.
**	    Added reltcpri to relation to hold table priority.
**	17-may-97 (devjo01)
**	    Free 'loc_cb' AFTER last use of suballocated array 'loc_array'.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance
**	    improvement.
**	03-Dec-1998 (jenjo02)
**	    Pass XCB*, page size, extent start to dm2f_alloc_rawextent().
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**	07-Jun-1999 (shero03)
**	    Hash the owner & table_name when obtaining a lock.
**      03-aug-1999 (stial01)
**          dm2u_create() Create etab tables for any peripheral atts
**      31-aug-1999 (stial01)
**          dm2u_create() Support VPS etabs
**	05-Mar-2002 (jenjo02)
**	    Added support for iisequence catalog.
**	07-mar-2002 (rigka01) bug#13857
**	    When attempting to create a view using a blob, E_US1263 is 
**	    received at the front end and E_DM0072, E_DM9B07, and E_DM9028
**	    are written to the error log. Ensure dmpe_create_extension is
**	    not called in the case of a view containing a blob. 
**	9-Feb-2004 (schka24)
**	    Update call to build fcb.
**	    Fixes for partition masters: zero out anything row- or page-
**	    count related in the master iirelation entry.
**	8-Mar-2004 (schka24)
**	    Don't take name-locks on partitions, no value and tends to
**	    run the transaction out of locks because they aren't
**	    escalate-able.
**	    Fix one more bit of repartition braindamage assigning ID's,
**	    and don't think partitions are catalogs.
**	3-Mar-2004 (gupsh01)
**	    Initialize attver_altcol for alter table alter column support.
**      29-Sep-2004 (fanch01)
**          Conditionally add LK_LOCAL flag if database/table is
**	    confined to one node.
**	15-dec-04 (inkdo01)
**	    Load attcollid column with collation ID.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were turning on iirelation.relstat TCB_VBASE bit. 
**          TCB_VBASE not tested anywhere in 2.6 and not turned off when view 
**          dropped qeu_cview() call to dmt_alter() to set bit has been removed,
**          removeing references to TCB_VBASE here also.
**	3-jan-06 (dougi)
**	    Added support for iicolcompare catalog.
**	13-Apr-2006 (jenjo02)
**	    Set TCB_CLUSTERED in relstat when appropriate.
**	23-nov-2006 (dougi)
**	    Reinstate code to set TCB_VBASE.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      04-jan-2007 (stial01)
**          Don't get CREATE TABLE lock for etabs, names are uniquely generated
**      21-Apr-2005 (hanal04) Bug 114330 INGSRV3260
**          Update the dcb->rep_regist_tab_idx when creating dd_reg_tbl_idx
**          for the first time. This will ensure we are able to continue
**          to use the same relid if the table is recreated as part of a
**          modify on dd_regist_tables. If the ID changes other DBMS servers
**          will hold stale relid values in their DCBs and fail to open
**          dcb->rep_regist_tab_idx in build_tcb(). This lead to E_US0845
**          errors for tables that did exist.
**	20-Feb-2008 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.
**	    Fix the insanity with table-flags being relstat, but not.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
**  29-Sept-2009 (troal01)
**  	Add geospatial support
**	6-Nov-2009 (kschendel) SIR 122757
**	    Ask for direct IO if config says to.
**	01-apr-2010 (toumi01) SIR 122403
**	    Support for column encryption. Start passing dmu.
**	13-may-2010 (miket) SIR 122403
**	    Fix net-change logic for width for ALTER TABLE.
**	09-Jun-2010 (jonj) SIR 121123
**	    Adapt hash of owner, table_name to long names.
**	20-Jul-2010 (kschendel) SIR 124104
**	    Add compression to the ridiculously long parameter list.
**	08-aug-2010 (miket) SIR 122403
**	    Improve AES key generation and encryption:
**	    (1) ensure full use of key space (MHrand2 tends to return
**		00s for high bits but I don't want to rock the CL boat)
**	    (2) make sure encrypted blocks are unpredictable by filling
**		the pad space with random bits rather than 00s
**	    (3) seed the random number generator for table create
**	27-Aug-2010 (jonj)
**	    Pass location's "k" index to dm2u_raw_location_free.
**	    Remove redundant SETDBERR.
*/

/* ****FIXME THIS parameter list is simply ridiculous.  Just pass
** the DMU_CB!!!  it's not like it has cooties or anything.
** Started passing it and used it for new args, but there is still
** room for improvement! (MikeT)
*/


DB_STATUS
dm2u_create(
DMP_DCB		    *dcb,
DML_XCB		    *xcb,
DB_TAB_NAME	    *table_name,
DB_OWN_NAME	    *owner,
DB_LOC_NAME	    *location,
i4		    l_count,
DB_TAB_ID	    *tbl_id,
DB_TAB_ID	    *idx_id,
i4		    index,
i4		    view,
i4		    relstat,
u_i4		    relstat2,
i4		    structure,
i4		    compression,
i4		    ntab_width,
i4		    ntab_data_width,
i4		    attr_count,
DMF_ATTR_ENTRY	    **attr_entry,
i4		    db_lockmode,
i4		    allocation,
i4		    extend,
i4		    page_type,
i4		    page_size,
DB_QRY_ID	    *qry_id,
DM_PTR		    *gwattr_array,
DM_DATA		    *gwchar_array,
i4		    gw_id,
i4		    gwrowcount,
DMU_FROM_PATH_ENTRY *gwsource,
DMU_CHARACTERISTICS *dmu_chars,
i4		    dimension,
i4		    hilbertsize,
f8 		    *range,
i4		    tbl_pri,
DB_PART_DEF	    *dmu_part_def,
i4		    dmu_partno,
DMU_CB		    *dmu,
DB_ERROR	    *errcb)
{
    DMP_RCB		*rel_rcb = (DMP_RCB *)0;
    DMP_RCB		*relindex_rcb = (DMP_RCB *)0;
    DMP_RCB		*attr_rcb = (DMP_RCB *)0;
    DMP_RCB		*dev_rcb = (DMP_RCB *)0;
    DMP_LOC_ENTRY	*ext =(DMP_LOC_ENTRY *)0;
    LK_LOCK_KEY		lockkey;
    LK_LKID		lockid;
    DMP_ATTRIBUTE	attrrecord;
    DMP_RELATION	relrecord;
    DMP_RELATION	base_relrecord;
    DMP_RINDEX		rindexrecord;
    DMP_DEVICE		devrecord;
    DB_TAB_ID		table_id;
    DB_TAB_ID		ntab_id;
    i4		ntab_offset;
    DB_TAB_TIMESTAMP	timestamp;
    DM_TID		tid;
    DM2R_KEY_DESC	key_desc[2];
    CL_ERR_DESC		sys_err;
    i4		compare;
    i4		error, local_error;
    i4		status, local_status;
    i4		jrnl_on = 0;
    i4		journal = 0;
    i4		extension = 0;
    i4		has_extensions = 0;
    i4			i,k;
    DML_SCB		*scb = xcb->xcb_scb_ptr;
    DB_ERROR		log_err;
    i4		loc_count = 1;
    DMP_MISC		*loc_cb = 0;
    DMP_LOCATION	*loc_array = 0;
    u_i4		db_sync_flag;
    bool		nofile_create = FALSE;
    bool		syscat = FALSE;	/* Regular or extended catalog
					** (not etab or user partition) */
    i4			gateway = 0;
    DMPP_ACC_PLV	*loc_plv;
    LG_LSN		lsn;
    bool		upcase;
    char		temptab[DB_TAB_MAXNAME];
    char		*tempstr2;
    i4		tbl_lk_mode;
    bool		concurrent_index = FALSE;
    i4		grant_mode;
    i4		dm0l_create_flags = 0;
    i4		journal_flag;
    bool		have_raw_loc = FALSE;
    DMP_ETAB_CATALOG    etab_record;
    bool	dd_reg_tbl_idx = FALSE;
    DB_ERROR		local_dberr;
    u_i2		dmu_enc_flags;
    u_i2		dmu_enc_flags2;
    u_char		*dmu_enc_passphrase;
    u_char		*dmu_enc_aeskey;
    u_i2		dmu_enc_aeskeylen;
    i4			alen;
    i4			attr_nametot;

    /* List of all the system catalogs with fixed table ID numbers.
    ** Any ii-table excluding blob etabs and user partitions that
    ** is not on this list, is an "extended" catalog and gets numbered
    ** just like any other table.
    */
    static struct
    {
	char	      *catstr;
	DB_TAB_ID    tab_id;
	i4      relstat;
	i4      relstat2;
    } systab[] =
    {
        {"iidevices",
	    {DM_B_DEVICE_TAB_ID, DM_I_DEVICE_TAB_ID},
	    TCB_CATALOG | TCB_PROALL | TCB_CONCUR, TCB2_PHYSLOCK_CONCUR},
	{"iiqrytext",
	    {DM_B_QRYTEXT_TAB_ID, DM_I_QRYTEXT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL,   0},
	{"iitree",
	    {DM_B_TREE_TAB_ID, DM_I_TREE_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iiprotect",
	    {DM_B_PROTECT_TAB_ID, DM_I_PROTECT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iiintegrity",
	    {DM_B_INTEGRITY_TAB_ID, DM_I_INTEGRITY_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iiintegrityidx",
	    {DM_B_INTEGRITYIDX_TAB_ID, DM_I_INTEGRITYIDX_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iirule",
	    {DM_B_RULE_TAB_ID, DM_I_RULE_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iiruleidx",
	    {DM_B_RULEIDX_TAB_ID, DM_I_RULEIDX_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iiruleidx1",
	    {DM_B_RULEIDX1_TAB_ID, DM_I_RULEIDX1_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iidbloc",
	    {DM_B_DBLOC_TAB_ID, DM_I_DBLOC_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iidbdepends",
	    {DM_B_DEPENDS_TAB_ID, DM_I_DEPENDS_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iixdbdepends",
	    {DM_B_XDEPENDS_TAB_ID, DM_I_XDEPENDS_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iistatistics",
	    {DM_B_STATISTICS_TAB_ID, DM_I_STATISTICS_TAB_ID}, TCB_CATALOG, 0},
        {"iihistogram",
	    {DM_B_HISTOGRAM_TAB_ID, DM_I_HISTOGRAM_TAB_ID}, TCB_CATALOG, 0},
        {"iicolcompare",
	    {DM_B_COLCOMPARE_TAB_ID, DM_I_COLCOMPARE_TAB_ID}, TCB_CATALOG, 0},
	{"iiprocedure",
	    {DM_B_DBP_TAB_ID, DM_I_DBP_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL,   0},
        {"iilocations",
	    {DM_B_LOCATIONS_TAB_ID, DM_I_LOCATIONS_TAB_ID},
	    TCB_CATALOG | TCB_PROALL | TCB_SECURE, 0},
        {"iidatabase",
	    {DM_B_DATABASE_TAB_ID, DM_I_DATABASE_TAB_ID},
	    TCB_CATALOG | TCB_PROALL, 0},
        {"iidbaccess",
	    {DM_B_DBACCESS_TAB_ID, DM_I_DBACCESS_TAB_ID},
	    TCB_CATALOG | TCB_PROALL | TCB_SECURE, 0},
        {"iiextend",
	    {DM_B_EXTEND_TAB_ID, DM_I_EXTEND_TAB_ID},
	    TCB_CATALOG | TCB_PROALL , 0},
        {"iiuser",
	    {DM_B_USER_TAB_ID, DM_I_USER_TAB_ID},
	    TCB_CATALOG | TCB_PROALL | TCB_SECURE, 0},
        {"iiusergroup",
	    {DM_B_GRPID_TAB_ID, DM_I_GRPID_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
        {"iirole",
	    {DM_B_APLID_TAB_ID, DM_I_APLID_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
        {"iidbpriv",
	    {DM_B_DBPRIV_TAB_ID, DM_I_DBPRIV_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
        {"iisecuritystate",
	    {DM_B_SECSTATE_TAB_ID, DM_I_SECSTATE_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
        {"iievent",
	    {DM_B_EVENT_TAB_ID, DM_I_EVENT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iidbms_comment",
	    {DM_B_COMMENT_TAB_ID, DM_I_COMMENT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iisynonym",
	    {DM_B_SYNONYM_TAB_ID, DM_I_SYNONYM_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
	{"iipriv",
	    {DM_B_PRIV_TAB_ID, DM_I_PRIV_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
	{"iixpriv",
	    {DM_B_XPRIV_TAB_ID, DM_I_XPRIV_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
	{"iixprocedure",
	    {DM_B_XDBP_TAB_ID, DM_I_XDBP_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
	{"iixevent",
	    {DM_B_XEVENT_TAB_ID, DM_I_XEVENT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iiextended_relation",
	    {DM_B_ETAB_TAB_ID, DM_I_ETAB_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL,0},
        {"iitemp_locations",
            {DM_B_TEMP_LOC_TAB_ID, DM_I_TEMP_LOC_TAB_ID},
            TCB_CATALOG | TCB_PROALL | TCB_SECURE, 0},
        {"iiphys_database",
            {DM_B_PHYS_DB_TAB_ID, DM_I_PHYS_DB_TAB_ID},
            TCB_CATALOG | TCB_PROALL, 0},
        {"iiphys_extend",
            {DM_B_PHYS_EXTEND_TAB_ID, DM_I_PHYS_EXTEND_TAB_ID},
            TCB_CATALOG | TCB_PROALL , 0},
        {"iitemp_codemap",
            {DM_B_CODEMAP_TAB_ID, DM_I_CODEMAP_TAB_ID},
            TCB_CATALOG | TCB_PROALL , 0},
        {"iikey",
	    {DM_B_IIKEY_TAB_ID, DM_I_IIKEY_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iidefault",
	    {DM_B_IIDEFAULT_TAB_ID, DM_I_IIDEFAULT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL , 0},
        {"iidefaultidx",
	    {DM_B_IIDEFAULTIDX_TAB_ID, DM_I_IIDEFAULTIDX_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iigw06_relation",
            {DM_B_GW06_REL_TAB_ID, DM_I_GW06_REL_TAB_ID},
            TCB_CATALOG | TCB_NOUPDT | TCB_PROALL| TCB_SECURE  , 0},
        {"iigw06_attribute",
            {DM_B_GW06_ATTR_TAB_ID, DM_I_GW06_ATTR_TAB_ID},
            TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE , 0},
	{"iischema",
            {DM_B_IISCHEMA_TAB_ID, DM_I_IISCHEMA_TAB_ID},
            TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iischemaidx",
            {DM_B_IISCHEMAIDX_TAB_ID, DM_I_IISCHEMAIDX_TAB_ID},
            TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
	{"iiprocedure_parameter",
	    {DM_B_IIPROC_PARAM_TAB_ID, DM_I_IIPROC_PARAM_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL,   0},
	{"iixsynonym",
	    {DM_B_XSYN_TAB_ID, DM_I_XSYN_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL,   0},
	{"iilabelmap",
	    {DM_B_IISECID_TAB_ID, DM_I_IISECID_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
	{"iilabelmapidx",
	    {DM_B_IISECIDX_TAB_ID, DM_I_IISECIDX_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
	{"iilabelaudit",
	    {DM_B_IILABAUDIT_TAB_ID, DM_I_IILABAUDIT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
	{"iiprofile",
	    {DM_B_IIPROFILE_TAB_ID, DM_I_IIPROFILE_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
	{"iisecalarm",
	    {DM_B_IISECALARM_TAB_ID, DM_I_IISECALARM_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
	{"iirolegrant",
	    {DM_B_IIROLEGRANT_TAB_ID, DM_I_IIROLEGRANT_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL | TCB_SECURE, 0},
        {"iirange",
	    {DM_B_RANGE_TAB_ID, DM_I_RANGE_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iisequence",
	    {DM_B_SEQ_TAB_ID, DM_I_SEQ_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, TCB2_PHYSLOCK_CONCUR},
        {"iidistscheme",
	    {DM_B_DSCHEME_TAB_ID, DM_I_DSCHEME_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iidistcol",
	    {DM_B_DISTCOL_TAB_ID, DM_I_DISTCOL_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iidistval",
	    {DM_B_DISTVAL_TAB_ID, DM_I_DISTVAL_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
        {"iipartname",
	    {DM_B_PNAME_TAB_ID, DM_I_PNAME_TAB_ID},
	    TCB_CATALOG | TCB_NOUPDT | TCB_PROALL, 0},
    };

    /* If the DMU_CB was passed in grab values from it. We are still
    ** passing in individual args that could come from here. Where
    ** are the janitors?! ;-)
    */
    if (dmu == NULL)
    {
	dmu_enc_flags = 0;
	dmu_enc_flags2 = 0;
	dmu_enc_passphrase = NULL;
	dmu_enc_aeskey = NULL;
	dmu_enc_aeskeylen = 0;
    }
    else
    {
	dmu_enc_flags = dmu->dmu_enc_flags;
	dmu_enc_flags2 = dmu->dmu_enc_flags2;
	dmu_enc_passphrase = dmu->dmu_enc_passphrase;
	dmu_enc_aeskey = dmu->dmu_enc_aeskey;
	dmu_enc_aeskeylen = dmu->dmu_enc_aeskeylen;
    }

    CLRDBERR(errcb);
    CLRDBERR(&log_err);

    upcase = ((*scb->scb_dbxlate & CUI_ID_REG_U) != 0);
	
    MEfill(sizeof(LK_LKID), 0, &lockid);

    status = dm2u_ckp_lock(dcb, table_name, owner, xcb, errcb);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    dmf_svcb->svcb_stat.utl_create++;

    if (relstat & TCB_JOURNAL)
    {
	journal = 1;
	relstat &= ~TCB_JOURNAL;	/* Will re-set after further checks */
    }
    if (relstat & TCB_GATEWAY)
	gateway = 1;
    if (relstat2 & TCB2_EXTENSION)
	extension = 1;
    if (relstat2 & TCB2_HAS_EXTENSIONS)
    {
	has_extensions = 1;
	relstat2 |= TCB2_BSWAP;		/* Always set on new tables */
    }
    if (compression != TCB_C_NONE)
	relstat |= TCB_COMPRESSED;

    if (((dcb->dcb_status & DCB_S_JOURNAL) != DCB_S_JOURNAL) && (journal))
    {
	journal = 0;
	jrnl_on = 1;
    }

    /*
    ** Journaling of the updates associated with this DDL action are
    ** conditional upon the journaling state of the created table.
    ** Gateway table and view creations are always journaled.
    */
    journal_flag = ((journal) ? DM0L_JOURNAL : 0);
    if ((gateway || view) && (dcb->dcb_status & DCB_S_JOURNAL))
	journal_flag = DM0L_JOURNAL;

    /*
    ** If creating a gateway table, make sure the database is configured to
    ** do so.
    */
    if (gateway && (! dmf_svcb->svcb_gateway))
    {
	SETDBERR(errcb, 0, E_DM0135_NO_GATEWAY);
	return (E_DB_ERROR);
    }

    /*
    ** If view or gateway, then there is no physical table or locations
    ** to deal with.
    ** Partition masters don't have physical files, but do have locations,
    ** oddly enough.  (The locations are needed so that a global index
    ** has a default location to go to.)
    */
    if (view || gateway || (relstat & TCB_IS_PARTITIONED))
    {
	nofile_create = TRUE;
	if (view || gateway) l_count = 0;
    }

    error = E_DB_OK;
    loc_count = l_count;

    do
    {
	for (k = 0; k < loc_count; k++)
	{
	    /*  Check location, must be default or valid location. */
	    if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&location[k],
		sizeof(DB_LOC_NAME)) == 0)
	    {
		ext = dcb->dcb_root;
	    }
	    else
	    {
		ext = (DMP_LOC_ENTRY*)0;
		for (i = 0; i < dcb->dcb_ext->ext_count; i++)
		{
		    compare = MEcmp((char *)&location[k],
                          (char *)&dcb->dcb_ext->ext_entry[i].logical,
                          sizeof(DB_LOC_NAME));
		    if (compare == 0 &&
			(dcb->dcb_ext->ext_entry[i].flags & DCB_DATA))
		    {
			ext = &dcb->dcb_ext->ext_entry[i];
			if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
			{    
	    		    /* 
	    		    ** Get the value lock for location devices
	    		    ** before getting rid of them to make sure that
	    		    ** the location cannot be used for raw devices 
			    ** for rollback
	    		    */
			    status = dm2u_raw_device_lock(dcb, xcb,
						&location[k], &lockid, 
						errcb);
			    if (status != LK_NEW_LOCK)
			    {
				SETDBERR(errcb, k, E_DM0189_RAW_LOCATION_INUSE);
		    		return (E_DB_ERROR);
			    }
			}
			break;
		    }
		}
		if ( ( (ext) == (DMP_LOC_ENTRY *)0 ) || 
		     ( compare || ext->flags == 0) ) 
		{
		    SETDBERR(errcb, k, E_DM001D_BAD_LOCATION_NAME);
		    return (E_DB_ERROR);
		}
	    }
	} /* end multiple location for loop */

	/* Now check to see if this is the name of a catalog.
        ** All catalogs start with ii.  If it is the name of
        ** a system catalog then need to be in a special
        ** session to create.
	** Etabs and partitions have ii-names but they are NOT catalogs.
        */

	if (!extension && (relstat2 & TCB2_PARTITION) == 0)
	    compare = STncasecmp((char *)table_name->db_tab_name,
				 "ii", 2 );
	else
	    compare = 1;

	if (compare == 0)
	{
	    syscat = TRUE;
	    if ((scb->scb_s_type & SCB_S_SYSUPDATE) == 0)
	    {
		SETDBERR(errcb, 0, E_DM005E_CANT_UPDATE_SYSCAT);
		return (E_DB_ERROR);
	    }
	    relstat |= TCB_CATALOG;
	    (VOID)MEcopy((PTR)scb->scb_cat_owner, sizeof(DB_OWN_NAME),
		 	(PTR)owner);
	}

	/*
	** Make sure this is not the same name
        ** as a system catalog name.
	** Don't need to check etabs and partitions, they have server
	** generated names and aren't catalogs.
	*/
	table_id.db_tab_base = DM_B_RIDX_TAB_ID; 
	table_id.db_tab_index = DM_I_RIDX_TAB_ID;

	if (!extension && (relstat2 & TCB2_PARTITION) == 0)
	{
	    status = dm2t_open(dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20,
		xcb->xcb_sp_id,
		xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0,
		db_lockmode,
		&xcb->xcb_tran_id, &timestamp,
		&relindex_rcb, (DML_SCB *)0, errcb);
	    if (status != E_DB_OK)
		break;
	    relindex_rcb->rcb_xcb_ptr = xcb;

	    /*  Position table using table name and INGRES id. */
	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_RELIDX_KEY;
	    key_desc[0].attr_value = (char *)table_name;
	    key_desc[1].attr_operator = DM2R_EQ;
	    key_desc[1].attr_number = DM_2_RELIDX_KEY;
	    key_desc[1].attr_value = (char *)scb->scb_cat_owner;

	    status = dm2r_position(relindex_rcb, DM2R_QUAL, key_desc,
			(i4)2, (DM_TID *)0,
			errcb);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Retrieve all records with this key, should get error indicating
	    ** none.  Do not want to find a match.
	    */
	    do
	    {
		status = dm2r_get(relindex_rcb, &tid, DM2R_GETNEXT,
			    (char *)&rindexrecord, errcb);

		if (status != E_DB_OK && errcb->err_code != E_DM0055_NONEXT)
		    break;
		if (status == E_DB_OK)
		{
		    if ((MEcmp((char *)&rindexrecord.relname,
			    (char *)table_name,
			    sizeof(DB_TAB_NAME)) == 0)
			&&
			(MEcmp((char *)&rindexrecord.relowner,
			    (char *)owner,
			    sizeof(DB_OWN_NAME)) == 0))
		    {
			SETDBERR(errcb, 0, E_DM0039_BAD_TABLE_NAME);
			break;
		    }
		}
	    } while (errcb->err_code != E_DM0055_NONEXT);
	    if (errcb->err_code != E_DM0055_NONEXT)
		break;

	    /* Close the index table, no longer needed. */

	    status = dm2t_close(relindex_rcb, DM2T_NOPURGE, errcb);
	    relindex_rcb = NULL;
	    if (status != E_DB_OK)
		break;
	}

	/* Don't take the create lock or re-check iirelation if this
	** is a partition.  Partition tables are deliberately named with
	** the tabid as the start of the name, so unless the config file
	** is hosed, duplication should be impossible.
	** (FIXME Probably should extend this to etabs as well.)
	*/
	if (!extension && (relstat2 & TCB2_PARTITION) == 0)
	{
	    i4		olen, orem, nlen, nrem;

	    olen = sizeof(*owner) / 2;
	    orem = sizeof(*owner) - olen;
	    nlen = sizeof(*table_name) / 3;
	    nrem = sizeof(*table_name) - (nlen * 2);

	    lockkey.lk_type = LK_CREATE_TABLE;
	    lockkey.lk_key1 = dcb->dcb_id;  /* till were done deleteing it */
	    /* There are 5 i4's == 20 bytes of key to work with.
	    ** Force-fit the owner into the first 8 and the name into
	    ** the last 12, hashing if necessary.
	    */
	    tempstr2 = (char *)owner;	    /* see if owner fits in 8 bytes */
	    for (i = 0; i < 9; i++, tempstr2++)
	    {
		if (*tempstr2 == ' ')
		    break;
	    }		
	    if (i < 9)        
		MEcopy((PTR)owner, 8, (PTR)&lockkey.lk_key2);
	    else
	    {	
	       lockkey.lk_key2 = HSH_char((PTR)owner, olen);
	       lockkey.lk_key3 = HSH_char((PTR)owner + olen, orem);
	    }
	    tempstr2 = (char *)table_name;  /* see if table fits in 12 bytes */
	    for (i = 0; i < 13; i++, tempstr2++)
	    {
		if (*tempstr2 == ' ')
		    break;
	    }		
	    if (i < 13)        
		MEcopy((PTR)table_name, 12, (PTR)&lockkey.lk_key4);
	    else
	    {	
		lockkey.lk_key4 = HSH_char((PTR)table_name, nlen);
		lockkey.lk_key5 = HSH_char((PTR)table_name + nlen, nlen);
		lockkey.lk_key6 = HSH_char((PTR)table_name + 2*nlen, nrem);
	    }


	    status = LKrequest(dcb->dcb_bm_served == DCB_SINGLE ?
                LK_LOCAL : 0, xcb->xcb_lk_id, &lockkey,
                LK_X, (LK_VALUE * )0, (LK_LKID *)&lockid, 0L,
                &sys_err);

	    if (status != OK)
	    {
		if (status == LK_NOLOCKS)
		{
		    uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL,
			&local_error, 2, sizeof(DB_TAB_NAME), table_name,
			sizeof(DB_DB_NAME), &dcb->dcb_name);
		    SETDBERR(errcb, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		    return (E_DB_ERROR);
		}
		else if ((status == LK_INTERRUPT) || (status == LK_INTR_GRANT))
		{
		    SETDBERR(errcb, 0, E_DM0065_USER_INTR);
		    return (E_DB_ERROR);
		}
                else if (status == LK_INTR_FA)
                {
                    SETDBERR(errcb, 0, E_DM016B_LOCK_INTR_FA);
                    return (E_DB_ERROR);
                }

		uleFormat( NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 2, 0, LK_X, 0, xcb->xcb_lk_id);
		uleFormat( NULL, E_DM901F_BAD_TABLE_CREATE, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 2, sizeof(DB_DB_NAME),
			&dcb->dcb_name,
			sizeof(DB_TAB_NAME), table_name);
		if (status == LK_DEADLOCK)
		{
		    SETDBERR(errcb, 0, E_DM0042_DEADLOCK);
		    return (E_DB_ERROR);
		}
		else
		{
		    SETDBERR(errcb, 0, E_DM0077_BAD_TABLE_CREATE);
		    return (E_DB_ERROR);
		}
	    }


	    /*
	    ** After the name is locked, try to find a match in the
	    ** relation table.
	    */
	    table_id.db_tab_base = DM_B_RIDX_TAB_ID;
	    table_id.db_tab_index = DM_I_RIDX_TAB_ID;

	    if (!extension)
	    {
		status = dm2t_open(dcb, &table_id, DM2T_IX,
		    DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20,
		    xcb->xcb_sp_id,
		    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0,
		    db_lockmode, &xcb->xcb_tran_id, &timestamp,
		    &relindex_rcb, (DML_SCB *)0, errcb);
		if (status)
		    break;
		relindex_rcb->rcb_xcb_ptr = xcb;

		/*  Position table using table name and user id. */
		key_desc[0].attr_operator = DM2R_EQ;
		key_desc[0].attr_number = DM_1_RELIDX_KEY;
		key_desc[0].attr_value = (char *) table_name;
		key_desc[1].attr_operator = DM2R_EQ;
		key_desc[1].attr_number = DM_2_RELIDX_KEY;
		key_desc[1].attr_value = (char *) owner;

		status = dm2r_position(relindex_rcb, DM2R_QUAL, key_desc,
			(i4)2, (DM_TID *)0, errcb);
		if (status != E_DB_OK)
		    break;

		/*
		** Retrieve all records with this key, should get error
		** indicating none.  Do not want to find a match.
		*/
		do
		{
		    status = dm2r_get(relindex_rcb, &tid, DM2R_GETNEXT,
			    (char *)&rindexrecord, errcb);

		    if (status != E_DB_OK && errcb->err_code != E_DM0055_NONEXT)
			break;
		    if (status == E_DB_OK)
		    {
			if ((MEcmp((char *)&rindexrecord.relname,
				(char *)table_name,
				sizeof(DB_TAB_NAME)) == 0)
			    &&
			    (MEcmp((char *)&rindexrecord.relowner,
				(char *)owner,
				sizeof(DB_OWN_NAME)) == 0))

			{
			    SETDBERR(errcb, 0, E_DM0078_TABLE_EXISTS);
			    break;
			}
		    }
		} while (errcb->err_code != E_DM0055_NONEXT);
		if (errcb->err_code != E_DM0055_NONEXT)
		    break;

		/* Close the index table, no longer needed. */

		status = dm2t_close(relindex_rcb, DM2T_NOPURGE, errcb);
		relindex_rcb = NULL;
		if (status != E_DB_OK)
		    break;
	    }
	} /* if not a partition */
    } while (FALSE);

    if (errcb->err_code)
    {
	/* Clean up, close table if opened. */
	if (relindex_rcb != NULL)
	{ 
	    local_status = dm2t_close(relindex_rcb, DM2T_NOPURGE, &local_dberr);
	    relindex_rcb = NULL;
	}

	return (E_DB_ERROR);
    }

    /* Open the relation and attribute tables. */

    table_id.db_tab_base = DM_B_RELATION_TAB_ID;
    table_id.db_tab_index = DM_I_RELATION_TAB_ID;

    status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0,
            db_lockmode,
            &xcb->xcb_tran_id, &timestamp,
	    &rel_rcb, (DML_SCB *)0, errcb);
    if (status)
	return (E_DB_ERROR);

    /*
    ** Check for NOLOGGING - no updates should be written to the log
    ** file if this session is running without logging.
    */
    if (xcb->xcb_flags & XCB_NOLOGGING)
	rel_rcb->rcb_logging = 0;

    /*
    ** Set user table journal state.
    ** Note that gateway tables are always listed as journaled.
    */
    rel_rcb->rcb_usertab_jnl = (journal_flag) ? 1 : 0;

    rel_rcb->rcb_xcb_ptr = xcb;

    /* Don't open iiattribute for partitions, they piggyback off of the
    ** master attribute rows
    */
    if ((relstat2 & TCB2_PARTITION) == 0)
    {
	table_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
	table_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

	status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0,
            db_lockmode,
            &xcb->xcb_tran_id, &timestamp,
	    &attr_rcb, (DML_SCB *)0, errcb);
	if (status)
	{
	    local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	    return (E_DB_ERROR);
	}

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    attr_rcb->rcb_logging = 0;

	attr_rcb->rcb_usertab_jnl = (journal_flag) ? 1 : 0;
	attr_rcb->rcb_xcb_ptr = xcb;
    }

    /* Open the iidevices table if there is more than one location. */

    dev_rcb = 0;
    if (loc_count > 1)
    {
	table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
	table_id.db_tab_index = DM_I_DEVICE_TAB_ID;

	status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,(i4)0,
            db_lockmode,
            &xcb->xcb_tran_id, &timestamp,
	    &dev_rcb, (DML_SCB *)0, errcb);
	if (status)
	{
	    local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	    if (attr_rcb != NULL)
		local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
	    return (E_DB_ERROR);
	}

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    dev_rcb->rcb_logging = 0;

	dev_rcb->rcb_usertab_jnl = (journal_flag) ? 1 : 0;
	dev_rcb->rcb_xcb_ptr = xcb;
    }
    /* If this is a system catalog, determine if it is
    ** extended one or not by searching list.  If it is in
    ** the list, then the table_id is fixed, don't go to the
    ** configuration file for the next available table id.
    */

    ntab_id.db_tab_base  = 0;
    ntab_id.db_tab_index = 0;
    if (syscat)
    {
	for (k= 0;k < sizeof(systab)/sizeof(systab[0]); k++)
	{
	    (VOID)STmove(systab[k].catstr, ' ', sizeof(temptab), &temptab[0]);
	    if (upcase)
	    {
		for (tempstr2 = temptab;
		     tempstr2 < &temptab[DB_TAB_MAXNAME];
		     tempstr2++)
		    CMtoupper(tempstr2,tempstr2);
	    }
	    compare = STncmp((char *)table_name->db_tab_name,
				 (char *)temptab, DB_TAB_MAXNAME );
	    if (compare == 0)
	    {
		STRUCT_ASSIGN_MACRO(systab[k].tab_id, ntab_id);
		/* Guarantee no junk flags: */
		relstat &= ~(TCB_GATEWAY | TCB_GW_NOUPDT | TCB_IS_PARTITIONED);
		relstat |= systab[k].relstat;
		/* Add extra relstat2 flags from systab */
		relstat2 |= systab[k].relstat2;
		break;
	    }

	}
	/* If compare is zero, we had a fixed-ID catalog */
	if (compare != 0)
	{
	    syscat = FALSE;		/* Tells below we need a table ID */
	    relstat |= (TCB_EXTCATALOG | TCB_PROALL);
	}

	/*
	** All Ingres catalogs are created with journaling defined.
	*/
	if (dcb->dcb_status & DCB_S_JOURNAL)
	    relstat |= TCB_JOURNAL;
	else
	    relstat |= TCB_JON;
    }

    if (loc_count > 1 && relstat & (TCB_CATALOG | TCB_EXTCATALOG))
    {
	local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	if (attr_rcb != NULL)
	    local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
	if (dev_rcb)
	    local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);
	SETDBERR(errcb, 0, E_DM0071_LOCATIONS_TOO_MANY);
	return (E_DB_ERROR);


    }
    if (! syscat)
    {
        if(STncasecmp(table_name->db_tab_name, "dd_reg_tbl_idx", 14) == 0)
        {
            dd_reg_tbl_idx = TRUE;
        }

        if(dd_reg_tbl_idx && dcb->rep_regist_tab_idx.db_tab_index)
        {
            /* We are recreating dd_reg_tbl_idx. Keep the same table id */
            ntab_id.db_tab_base = dcb->rep_regist_tab_idx.db_tab_base;
            ntab_id.db_tab_index = dcb->rep_regist_tab_idx.db_tab_index;
        }
        else
        {
	    /* The table needs to have an ID assigned.
	    ** If this is a partition, we already got N table ID's when
	    ** the master was created.  Partitioned table create is going
	    ** to be slow enough without acquiring ID numbers one at a time.
	    ** The partition's ID is index + partition number + 1.
	    ** (Partition numbers are zero origin.)
	    */
	    if (relstat2 & TCB2_PARTITION)
	    {
	        ntab_id.db_tab_base = tbl_id->db_tab_base;
	        ntab_id.db_tab_index = (tbl_id->db_tab_index + dmu_partno + 1)
					    | DB_TAB_PARTITION_MASK;
	    }
	    else
	    {
	        /*
	        ** Get a new table id. The last table_id used is found in the
	        ** configuration file.  to get a new one the config file must be
	        ** locked, read, updated, and unlocked.
	        */
	        i = 1;
	        if (relstat & TCB_IS_PARTITIONED)
		    i = dmu_part_def->nphys_parts + 1;
	        status = dm2u_get_tabid( dcb, xcb, tbl_id, index,
				    i, &ntab_id, errcb);
	        if (status != E_DB_OK)
	        {
		    local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, 
                                                &local_dberr);
		    if (attr_rcb != NULL)
		        local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
		    if (dev_rcb)
		        local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);
		    return (E_DB_ERROR);
	        }
                if(dd_reg_tbl_idx)
                {
                    /* We've just created dd_reg_tbl_idx for the first time
                    ** set dcb->rep_regist_tab_idx or this server could
                    ** still acquire a new ID if we modify dd_regist_tables
                    ** without dropping the DCB in the meantime.
                    */
                    dcb->rep_regist_tab_idx.db_tab_base = ntab_id.db_tab_base;
                    dcb->rep_regist_tab_idx.db_tab_index = ntab_id.db_tab_index;
                }
    
	        if (extension)
	        {
		    char	    etab_name[DB_TAB_MAXNAME+1];
		    /* Then create the table's name */
    
		    STprintf(etab_name, "iietab_%x_%x", tbl_id->db_tab_base,
			        ntab_id.db_tab_base);
		    MEmove(STlength(etab_name), etab_name, ' ',
			    sizeof(table_name->db_tab_name),
			    table_name->db_tab_name);
	        }
            }
	}
    } /* end if no syscat or extended syscat. */

    /*
    ** Determine if a concurrent index and then set the lock modes
    ** accordingly
    */
    concurrent_index = ((relstat2 & TCB2_CONCURRENT_ACCESS ) != 0);
    if (concurrent_index)
    {
	dm0l_create_flags |= DM0L_CREATE_CONCURRENT_INDEX;
	tbl_lk_mode = LK_S;
    }
    else
    {
	tbl_lk_mode = LK_X;
    }

    status = dm2t_lock_table( dcb, &ntab_id, tbl_lk_mode,
			      xcb->xcb_lk_id,
			      (i4) 0L, /* timeout */
			      &grant_mode,
			      &lockid, errcb);
    if ( status != E_DB_OK )
    {
	local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	if (attr_rcb != NULL)
	    local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
	if (dev_rcb)
	    local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);
	return (E_DB_ERROR);
    }

    /*
    **  All parameters have been checked, can really create the table.
    **  First log that a create is taking place.
    **  Take a savepoint for error recovery.
    **  Then get a table id, add a record to the relation table
    **  and then add a record to the attribute table for each attribute.
    **  When all of this has succeeded, create the file.
    **  If any error occur along the way, abort to the savepoint.
    **
    **	A few notes on error handling from here on out:
    **	    The important variable is 'error'. To indicate an error has
    **		occurred, set 'error' to the appropriate error number and
    **		break from the 'for' loop.
    **	    If an error has occurred, it can be useful to additionally set
    **		'log_err'. If both 'error' and 'log_err' are set, and
    **		if 'error' is > E_DM_INTERNAL, then 'log_err' is logged before
    **		returning.
    **	    Whether or not 'log_err' is set, 'error' will be the error which
    **		is returned.
    **	    In general, do not use 'err_code' nor 'local_error'. 'err_code' is
    **		set to point into the caller's errcb structure; we will set
    **		*err_code = error at the very end of this routine. Attempts to
    **		set *err_code and break are futile, since *err_code will be
    **		overwritten with 'error' at the end of this routine.
    **	    Setting '*err_code' and then returning (not breaking) is NOT
    **		acceptable. Earlier in this routine, this technique was used,
    **		but it is not acceptable here. It is important to set 'error'
    **		and 'break', since break takes us to the error-cleanup code
    **		which cleans up the work we have done up to this point.
    **	    'local_error' should be used as the "returned error code" argument
    **		to ule_format calls. Most other uses of 'local_error' are
    **		suspect, except in the final error-handling cleanup code, where
    **		'local_error' is used so as not to disturb 'error'.
    */

    do
    {
	/*
	** Create table requires a table control lock so that nobody can
	** SHOW the table or open it with readnolock while the system catalog
	** entries are not complete and consistent. (The table control lock
	** is held until commit so that nobody can show the table while we
	** are aborting the create).
	**
	** This lock is not needed during a concurrent index build request
	** as we are not changing the base table.
	**
	** If this is a partition we already have this lock, on the master,
	** but the request goes quickly.
	*/
	if (!concurrent_index)
	{
	    status = dm2t_control( dcb, ntab_id.db_tab_base, xcb->xcb_lk_id,
		                   LK_X, (i4)0, (i4)0, errcb);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Pick file create sync mode.
	*/
	db_sync_flag = 0;
	if ((dcb->dcb_sync_flag & DCB_NOSYNC_MASK) == 0)
	{
	    db_sync_flag = DI_SYNC_MASK;
	    if (dmf_svcb->svcb_directio_tables)
		db_sync_flag |= DI_DIRECTIO_MASK;
	}

	/*
	** Adjust allocation and extend values appropriately - substituting
	** default values if none were explicitly specified.
	*/
	if (allocation <= 0)
	    allocation = DM_TBL_DEFAULT_ALLOCATION;
	if (extend <= 0)
	    extend =  DM_TBL_DEFAULT_EXTEND;

	/*
	** If a multi-location table make sure that the allocation and
	** extend sizes are factors of DI_EXTENT_SIZE. This is for
	** performance reasons.
	*/
	if ( loc_count > 1 )
	{
	     dm2f_round_up( (i4 *)&allocation);
	     dm2f_round_up( (i4 *)&extend);
	}

	/*
	** Get page accessors for recovery actions, assuming
	** that the created table is a heap.
	*/
	dm1c_get_plv(page_type, &loc_plv);

	/*
	** Now create the physical file for the table.
	** If this is a special table with no physical file, then skip
	** this work.  Some care is needed;  partition masters don't
	** have physical files, but do have locations, so the location
	** array formatting is required.  (To translate $default, if
	** for no other reason!)
	*/
	if ( loc_count > 0)
	{
	    /*
	    ** Allocate and format location information array.
	    */
	    status = dm0m_allocate(
		(i4)sizeof(DMP_LOCATION)*loc_count + sizeof(DMP_MISC),
		DM0M_ZERO,
		(i4)MISC_CB,
		(i4)MISC_ASCII_ID, (char *)0, (DM_OBJECT **)&loc_cb,
		errcb);
	    if (status != E_DB_OK)
		break;
	    loc_array = (DMP_LOCATION *)((char *)loc_cb + sizeof(DMP_MISC));
	    loc_cb->misc_data = (char *)loc_array;

	    for (k=0; k < loc_count; k++)
	    {
		loc_array[k].loc_id = k;
		loc_array[k].loc_ext = 0;
		loc_array[k].loc_fcb = 0;
		loc_array[k].loc_status = LOC_VALID;   /* resets LOC_UNAVAIL */

		compare = MEcmp((PTR)&location[k], (PTR)DB_DEFAULT_NAME,
		    sizeof(DB_LOC_NAME));
		if (compare == 0)
		{
		    loc_array[k].loc_ext = dcb->dcb_root;
		    loc_array[k].loc_config_id = 0;
		    STRUCT_ASSIGN_MACRO(dcb->dcb_root->logical,
			loc_array[k].loc_name);
		    continue;
		}

		STRUCT_ASSIGN_MACRO(location[k], loc_array[k].loc_name);
		for (i = 0; i < dcb->dcb_ext->ext_count; i++)
		{
		    if ((dcb->dcb_ext->ext_entry[i].flags & DCB_DATA) == 0)
			continue;

		    compare = MEcmp((char *)&location[k],
			      (char *)&dcb->dcb_ext->ext_entry[i].logical,
			      sizeof(DB_LOC_NAME));
		    if (compare == 0 )
		    {
			loc_array[k].loc_config_id = i;
			loc_array[k].loc_ext = &dcb->dcb_ext->ext_entry[i];
			if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
			{
			    status = dm2u_raw_location_free(dcb, xcb, 
					&location[k], k, errcb); 
	    		    if (status != E_DB_OK)
			    {
				compare = 1;
				/* dm2u_raw_location_free has set errcb info */
				break;
			    }
			    loc_array[k].loc_status |= LOC_RAW;
		            have_raw_loc = TRUE;
			}
			break;
		    }
		}
		if (compare)
		    break;
	    } /* end multiple files for */
	    if (status != E_DB_OK)
		break;

	    if (! nofile_create)
	    {
		/*
		** Allocate FCBs for the files.
		*/
		status = dm2f_build_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
		    loc_array, loc_count, 
		    page_size, DM2F_ALLOC, DM2F_TAB, &ntab_id, 0, 0,
		    (DMP_TABLE_IO *)0, 0, errcb);
		if (status != E_DB_OK)
		    break;

		/*
		** Log and create the file(s).
		*/
		status = dm2u_file_create(dcb, xcb->xcb_log_id, xcb->xcb_lk_id,
			    &ntab_id, db_sync_flag, loc_array, loc_count, 
			    page_size,
			    ((xcb->xcb_flags & XCB_NOLOGGING) == 0), 
			    journal_flag, errcb);
		if (status != E_DB_OK)
		    break;

		/*
		** RAW files are bounded by the size of the smallest RAW
		** location. dm2f_build_fcb() guarantees that each raw FCB's 
		** fcb_rawpages contains that of the smallest location.
		*/
		if (have_raw_loc)
		{
		    for (k=0; k < l_count; k++)
		    {
			if ( loc_array[k].loc_status & LOC_RAW && 
			     loc_array[k].loc_status & LOC_VALID )
			{
			    if ( loc_count * loc_array[k].loc_fcb->fcb_rawpages 
					< allocation )
			    {
				allocation = 
				    loc_count * loc_array[k].loc_fcb->fcb_rawpages;
			    }
			    break;
			}
		    }
		}

		/*
		** Create the new table assuming fixed pages numbers.
		** Recovery code and the log record written below both make
		** these assumptions at this time.
		*/
		status = dm1s_empty_table(dcb, &ntab_id, allocation, loc_plv,
		    (DM_PAGENO) 1,     		/* fhdr */
		    (DM_PAGENO) 2,     		/* 1st fmap */
		    (DM_PAGENO) 2,     		/* last fmap */
		    (DM_PAGENO) 3,     		/* first free */
		    (DM_PAGENO) (allocation - 1), /* last free */
		    (DM_PAGENO) 0,     		/* first used */
		    (DM_PAGENO) 0,     		/* last used */
		    page_type, page_size,
		    loc_count, loc_array, errcb);
		if (status != E_DB_OK)
		    break;
	    } /* ! nofile-create */
	} /* any locations */

	for (i = 0, attr_nametot = 0; i < attr_count; i++)
	{
	    for (alen = DB_ATT_MAXNAME;  
		attr_entry[i]->attr_name.db_att_name[alen-1] == ' ' 
			&& alen >= 1; alen--);
	    attr_nametot += alen;
	}

	/*
	** Build IIRELATION row for the new table being created.
	*/
	MEfill(sizeof(relrecord), 0, (PTR)&relrecord);

	relrecord.reltid.db_tab_base = ntab_id.db_tab_base;
	relrecord.reltid.db_tab_index = ntab_id.db_tab_index;
	MEcopy((PTR)table_name, sizeof(DB_TAB_NAME), (PTR)&relrecord.relid);
	if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&location[0],
		sizeof(DB_LOC_NAME)) == 0)
	{
	    MEcopy((PTR) dcb->dcb_root->logical.db_loc_name,
		sizeof(DB_LOC_NAME), (PTR)&relrecord.relloc);
	}
	else
	{
	    MEcopy((PTR)&location[0], sizeof(DB_LOC_NAME),
                 (PTR)&relrecord.relloc);
	}
	MEcopy((PTR)owner, sizeof(DB_OWN_NAME), (PTR)&relrecord.relowner);
	relrecord.relatts = attr_count;
	relrecord.relattnametot = attr_nametot;
	relrecord.relwid = ntab_width;
	relrecord.reltotwid = ntab_width;	        
	relrecord.reldatawid = ntab_data_width;	        
	relrecord.reltotdatawid = ntab_data_width;	        
	relrecord.relkeys = 0;
	relrecord.relspec = structure;
	relrecord.reltups = 0;
	/*
	** FHDR and FMAPs are counted in the relpages total, the allocation
	** size cannot be specified on a create table statement. Hence
	** relpages for a newley created table will be 3, one data page,
	** one fhdr and one fmap
	*/
	relrecord.relpages = 3;
	relrecord.relprim = 1;
        relrecord.relmain = 1;
        relrecord.relsave = 0;
	MEfill(sizeof(relrecord.relfree), 0, relrecord.relfree);
	relrecord.relcmptlvl = DMF_T_VERSION;
	TMget_stamp((TM_STAMP *)&relrecord.relstamp12);
	relrecord.relcreate = TMsecs();
	relrecord.relmoddate = relrecord.relcreate;
	relrecord.relidxcount = 0;
	relrecord.relifill = 0;
        relrecord.reldfill = 100;
        relrecord.rellfill = 0;
        relrecord.relmin = 1;
        relrecord.relmax = 1;
	relrecord.relloccount = loc_count;
	relrecord.relhigh_logkey = 0;
	relrecord.rellow_logkey = 0;
	relrecord.relqid1 = 0;
	relrecord.relqid2 = 0;
	relrecord.relfhdr = DM_TBL_DEFAULT_FHDR_PAGENO;
	relrecord.relallocation = allocation;
	relrecord.relextend = extend;
	relrecord.relpgsize = page_size;
	relrecord.relversion = 0;  

	/* Set table's cache priority */
	relrecord.reltcpri = tbl_pri;

	/* Set Table's Page Type */
	relrecord.relpgtype = page_type;

        relrecord.relcomptype = compression;

	if (relstat & TCB_IS_PARTITIONED)
	{
	    relrecord.relnparts = dmu_part_def->nphys_parts;
	    relrecord.relnpartlevels = dmu_part_def->ndims;
	    /* No row/page counts in the master, it's all in the
	    ** partitions.  (Should perhaps this be any no-file type?)
	    */
	    relrecord.relpages = 0;
	    relrecord.relmain = 0;
	    relrecord.relprim = 0;
	    relrecord.relfhdr = 0;
	}
	else if (relstat2 & TCB2_PARTITION)
	{
	    relrecord.relnparts = dmu_partno;
	}

	/* Calculate relation status. */
	/* BEWARE!  We continue setting flags in relrecord.relstat and
	** don't bother keeping the relstat variable up to date...
	*/

	relrecord.relstat = relstat ;
	if ((relstat & (TCB_CATALOG | TCB_EXTCATALOG)) == 0)
	    relrecord.relstat |= TCB_PROALL | TCB_PROTECT;

	if (view)
	{
	    relrecord.relstat |= TCB_VIEW;

	    relrecord.relqid1 = qry_id->db_qry_high_time =
		ulb_rev_bits(ntab_id.db_tab_base);
	    relrecord.relqid2 = qry_id->db_qry_low_time =
		 ulb_rev_bits(ntab_id.db_tab_index);
	    relrecord.relloccount = 0;

	}
	if (journal)
	    relrecord.relstat |= TCB_JOURNAL;
	if (jrnl_on)
	    relrecord.relstat |= TCB_JON;
        if (index)
	    relrecord.relstat |= TCB_INDEX;
	if (loc_count > 1)
	    relrecord.relstat |= TCB_MULTIPLE_LOC;

	/*
	** Core catalogs have special formats so ensure
	** any row auditing/label flags are set appropriately
	** (may be set on due to default values).
	**
	** Note that this includes extended catalogs as well currently.
	** If a user need arises to have row auditing/labelling on
	** extended catalogs then this test should be modified to
	** exclude tables with TCB_EXTCATALOG set. If this is done then
	** the check further down this function which eliminates associated
	** attributes must also be updated in the same way.
	*/
	if(relstat & TCB_CATALOG)
	{
	    relstat2 &= ~(TCB_ROW_AUDIT);
	    /*
	    ** Loop over all attributes checking
	    ** for security label/key
	    */
	    for (i = 0; i < attr_count; i++)
	    {
		if(attr_entry[i]->attr_flags_mask & (DMU_F_SEC_KEY))
		{
		    /*
		    ** Don't want this attribute
		    ** Note that we DONT decrease
		    ** attr_count since atts may
		    ** not be at the end.
		    */
		    relrecord.relatts--;
		    /*
		    ** Decrease record size by
		    ** this amount
		    */
		    relrecord.relwid-=attr_entry[i]->attr_size;
		    relrecord.reltotwid-=attr_entry[i]->attr_size;
		    relrecord.reldatawid-=attr_entry[i]->attr_size;
		    relrecord.reltotdatawid-=attr_entry[i]->attr_size;
		}
	    }
	}

	/*
	** If this is a gateway table then mark gateway specific information.
	*/
        if (gateway)
	{
	    relrecord.relgwid = gw_id;

	    /*
	    ** Fill in a starting tuple count as specified in the register
	    ** statement.  The more accurate this tuple count is the better
	    ** job the optimizer can do selecting query plans.
	    **
	    ** NOTE that it would be beneficial here to calculate reasonable
	    ** values for fill-factors, page_counts and any other values used
	    ** in statistics gathering and cost evaluation.
	    */
	    relrecord.reltups = gwrowcount;

	    /*
	    ** Compute the qry_id for gateways for join with iiqrytext for
	    ** view iiregistrations and copy it where the caller can see it.
	    */
	    relrecord.relqid1 = qry_id->db_qry_high_time =
		ulb_rev_bits(ntab_id.db_tab_base);
	    relrecord.relqid2 = qry_id->db_qry_low_time =
		ulb_rev_bits(ntab_id.db_tab_index);
	}

	/*
	** Set up encryption at the table level.
	** - store a version number as "future changes insurance"
	** - store the type of encrytion as a flag
	** - generate a key for the selected type and
	**	- append a validating hash
	**	- fill the padding with random bits
	**	- encrypt the lot with the user passphrase
	** Layout of key / hash / ?-pad / 0-extra bytes is as follow:
	** AES 128
	** <----BLOCK1----><----BLOCK2---->
	** KKKKKKKKKKKKKKKKhhhh????????????00000000000000000000000000000000
	** AES 192
	** <----BLOCK1----><----BLOCK2---->
	** KKKKKKKKKKKKKKKKKKKKKKKKhhhh????00000000000000000000000000000000
	** AES 256
	** <----BLOCK1----><----BLOCK2----><----BLOCK3---->
	** KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKhhhh????????????0000000000000000
	** Note that the original design called for a SHA-1 hash rather
	** than the Ingres hash, and requires up to 4 blocks, as shown
	** below. For various reasons of software licensing and storage
	** space efficiency (in user records) TCB_ENC_VER_1 does not
	** require more than 3 blocks (see above), but the "extra" space
	** in iirelation is retained as "future insurance", should SHA-1
	** or some other lengthier hash be desired for a later release.
	** AES 128
	** <----BLOCK1----><----BLOCK2----><----BLOCK3---->
	** KKKKKKKKKKKKKKKKhhhhhhhhhhhhhhhhhhhh????????????0000000000000000
	** AES 192
	** <----BLOCK1----><----BLOCK2----><----BLOCK3---->
	** KKKKKKKKKKKKKKKKKKKKKKKKhhhhhhhhhhhhhhhhhhhh????0000000000000000
	** AES 256
	** <----BLOCK1----><----BLOCK2----><----BLOCK3----><----BLOCK4---->
	** KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKhhhhhhhhhhhhhhhhhhhh????????????
	*/
	if (dmu_enc_flags & DMU_ENCRYPTED)
	{
	    i4 i, j, nrounds, keybits, keybytes, blocks;
	    union {
		u_i4	rand_i4[AES_BLOCK*4/sizeof(u_i4)];
		u_char	randkey[AES_BLOCK*4];
	    } u_key;
	    u_i4 rk[RKLENGTH(AES_256_BITS)];
	    u_char *et, *pt, *pprev;
	    u_i4 crc;
	    HRSYSTIME hrtime;
	    i4 temprand = MHrand2();

	    /* seed the random generator: kick the can an arbitrary distance
	    ** down the road from an arbitrary starting point
	    */
	    TMhrnow(&hrtime);
	    MHsrand2(hrtime.tv_nsec * temprand);

	    relrecord.relencver = TCB_ENC_VER_1;
	    relrecord.relencflags = TCB_ENCRYPTED;
	    if (dmu_enc_flags & DMU_AES128)
	    {
		relrecord.relencflags |= TCB_AES128;
		keybits = AES_128_BITS;
		keybytes = AES_128_BYTES;
		blocks = 2;
	    }
	    else
	    if (dmu_enc_flags & DMU_AES192)
	    {
		relrecord.relencflags |= TCB_AES192;
		keybits = AES_192_BITS;
		keybytes = AES_192_BYTES;
		blocks = 2;
	    }
	    else
	    if (dmu_enc_flags & DMU_AES256)
	    {
		relrecord.relencflags |= TCB_AES256;
		keybits = AES_256_BITS;
		keybytes = AES_256_BYTES;
		blocks = 3;
	    }
	    else
	    {
		/* internal error */
		SETDBERR(errcb, 0, E_DM0175_ENCRYPT_FLAG_ERROR);
		return (E_DB_ERROR);
	    }

	    MEfill(sizeof(u_key.randkey), 0, (PTR)&u_key.randkey);
	    if (dmu_enc_flags2 & DMU_AESKEY)
	    {
		/* The user chose to supply an explicit AES key value for
		** this table; just take them at their word.
		*/
		MEcopy((PTR)dmu_enc_aeskey, dmu_enc_aeskeylen,
		    (PTR)u_key.randkey);
	    }
	    else
	    {
		/* Construct an encryption key by generating enough pseudo
		** random u_i4s to fill in the key for the AES type.
		*/
		for ( i = 0 ; i < (keybytes / sizeof(u_i4)) ; i++ )
		{
		    u_key.rand_i4[i] = MHrand2();
		    u_key.rand_i4[i] *= MHrand2(); /* ensure high bits */
		}
	    }
	    /* fill the padding after the crc with random bits */
	    for ( i = (keybytes / sizeof(u_i4)) + 1,	/* +1/-1 skips crc */
		  j = (blocks * sizeof(u_i4)) - (keybytes / sizeof(u_i4)) - 1 ;
		  j > 0 ; i++, j-- )
	    {
		u_key.rand_i4[i] = MHrand2();
		u_key.rand_i4[i] *= MHrand2(); /* ensure high bits */
	    }
	    /* fill in the hash */
	    crc = -1;
	    HSH_CRC32(u_key.randkey, keybytes, &crc);
	    MEcopy((PTR)&crc, sizeof(crc), u_key.randkey + keybytes);
	    /* Encrypt the internal encryption key with the passphrase */
	    nrounds = adu_rijndaelSetupEncrypt(rk, dmu_enc_passphrase, keybits);
	    pprev = NULL;
	    et = relrecord.relenckey;	/* Encrypted Text in relation */
	    pt = u_key.randkey;		/* Plain Text on stack */
	    for ( i = blocks ; i > 0 ; i-- )
	    {
		/* CBC cipher mode */
		if (pprev)
		    for ( j = 0 ; j < AES_BLOCK ; j++ )
			pt[j] ^= pprev[j];
		adu_rijndaelEncrypt(rk, nrounds, pt, et);
		pprev = et;
		et += AES_BLOCK;
		pt += AES_BLOCK;
	    }
	}

	relrecord.relstat2 = relstat2;

	/*
	** Log the create operation - unless logging is disabled.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    /*
	    ** Note assumptions made here about the location of pages
	    ** created during dm1s_empty_table.
	    */
	    status = dm0l_create(xcb->xcb_log_id, journal_flag,
		&ntab_id, table_name, owner, allocation,
		dm0l_create_flags, relrecord.relstat,
		(DM_PAGENO) 1,     /* fhdr page */
		(DM_PAGENO) 2,     /* 1st fmap page */
		(DM_PAGENO) 2,     /* last fmap page */
		(DM_PAGENO) 3,     /* first free page */
		(DM_PAGENO) (allocation - 1),/* last free page */
		(DM_PAGENO) 0,     /* first used page */
		(DM_PAGENO) 0,     /* last used page */
		loc_count, location, 
		page_size, relrecord.relpgtype,
		(LG_LSN *)0, &lsn, errcb);
	    if (status != E_DB_OK)
		break;
	}


	/*
	** Add new relation record to iirelation table.
	*/
	status = dm2r_put(rel_rcb, DM2R_DUPLICATES, (char *)&relrecord, errcb);
	if (status != E_DB_OK)
	{
	    uleFormat(errcb, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
	    break;
	}

	/* Don't write attribute rows for partitions */
	if (attr_rcb != NULL)
	{
	    ntab_offset = 0;
	    attrrecord.attid = 0;
	    attrrecord.attintl_id = 0;

	    for (i = 0; i < attr_count; i++)
	    {
		/*
		** Skip any default row audit key or security label for core
		** catalogs, these are handled differently.
		*/
		if ((relstat & TCB_CATALOG) &&
		     attr_entry[i]->attr_flags_mask & (DMU_F_SEC_KEY))
			    continue;

		attrrecord.attrelid.db_tab_base = ntab_id.db_tab_base;
		attrrecord.attrelid.db_tab_index = ntab_id.db_tab_index;
		attrrecord.attid++;
		attrrecord.attxtra = 0;
		attrrecord.attoff = ntab_offset;
		ntab_offset += attr_entry[i]->attr_size;
		attrrecord.attfmt = attr_entry[i]->attr_type;
		attrrecord.attfml = attr_entry[i]->attr_size;
		attrrecord.attfmp = attr_entry[i]->attr_precision;
		attrrecord.attkey = 0;
		attrrecord.attflag = attr_entry[i]->attr_flags_mask;
		COPY_DEFAULT_ID( attr_entry[i]->attr_defaultID,
		    attrrecord.attDefaultID );
		STRUCT_ASSIGN_MACRO(attr_entry[i]->attr_name, attrrecord.attname);
		attrrecord.attintl_id++;
		attrrecord.attver_added = 0;
		attrrecord.attver_dropped = 0;
		attrrecord.attval_from = 0;
		attrrecord.attver_altcol = 0;
		attrrecord.attcollID = attr_entry[i]->attr_collID;
		attrrecord.attgeomtype = attr_entry[i]->attr_geomtype;
		attrrecord.attencflags = attr_entry[i]->attr_encflags;
		attrrecord.attencwid = attr_entry[i]->attr_encwid;
		attrrecord.attsrid = attr_entry[i]->attr_srid;
		MEfill(sizeof(attrrecord.attfree), 0, (PTR)&attrrecord.attfree);

		status = dm2r_put(attr_rcb, DM2R_DUPLICATES, (char *)&attrrecord,
				 errcb);
		if (status != E_DB_OK)
		{
		    uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		    break;
		}
	    }

	    /* Check for errors inserting records. */

	    if (status != E_DB_OK)
		break;

	    /*
	    ** Unfix any fixed pages from iiattribute here. Keeping system
	    ** catalog pages fixed for a long time while updating them reduces
	    ** concurrency and also causes problems with cluster node failure
	    ** recovery, which assumes that there is never more than one core
	    ** system catalog page fixed by a transaction at a time.
	    */
	    status = dm2r_unfix_pages(attr_rcb, errcb);
	    if (status != E_DB_OK)
	    {
		uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		break;
	    }
	} /* if writing attrs */

	/* Create etab tables for any peripheral atts */
	if (has_extensions && !view)
	{
	    ADF_CB              adf_scb;
	    i4                  dt_bits;
	    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

	    for (i = 0; i < attr_count; i++)
	    {
		status = adi_dtinfo(&adf_scb, attr_entry[i]->attr_type, 
			&dt_bits);
		if (status != E_DB_OK)
		{
		    *errcb = adf_scb.adf_errcb.ad_dberror;
		    break;
		}

		if ((dt_bits & AD_PERIPHERAL) == 0)
		    continue;

		/*
		** VPS extension tables
		** Page size will be determined in dmpe
		*/
		status = dmpe_create_extension(xcb, &relrecord,
			0, /* page_size determined in dmpe */
			loc_array, loc_count,
			i + 1, attr_entry[i]->attr_type, &etab_record, errcb);

		if (status)
		    break;
	    }
	}

	if (status != E_DB_OK)
	{
	    uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
	    break;
	}

	/* Build device table record for table creating if locations
        ** greater than one. */

	if (loc_count > 1)
	{
	    for (k=1; k<loc_count; k++)
	    {
		devrecord.devreltid.db_tab_base = ntab_id.db_tab_base;
		devrecord.devreltid.db_tab_index = ntab_id.db_tab_index;
		devrecord.devlocid = k;
		MEcopy((PTR)&location[k], sizeof(DB_LOC_NAME),
		     (PTR)&devrecord.devloc);

		/* Add record to iidevices table. */

		status = dm2r_put(dev_rcb, DM2R_DUPLICATES,
                                  (char *)&devrecord, errcb);
		if (status != E_DB_OK)
		{
		    uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);

		    break;
		}

	    }
	    if (status != E_DB_OK)
		break;
	}

	/* Check for errors inserting records. */

	if (status != E_DB_OK)
	    break;

	/*
	** If a view or an ordinary index then update the base table
	** accordingly. A concurrent index is only exposed once the
	** base table is modified back to NOREADONLY.
        */

	if ((view || index) && (!concurrent_index))
	{
	    /*
	    ** If the table creating is a view or an index, update the base
	    ** table relation record approriately.
	    */

	    /*  Position table using base table id. */

	    table_id.db_tab_base = tbl_id->db_tab_base;
	    table_id.db_tab_index = 0;
	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_RELATION_KEY;
	    key_desc[0].attr_value = (char *) &table_id.db_tab_base;

	    status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, errcb);
	    if (status != E_DB_OK)
	    {
		uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    for (;;)
	    {
		/*
		** Get a qualifying relation record.  This will retrieve
		** all records for a base table (find the base table).
		*/

		status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT,
                              (char *)&base_relrecord, errcb);
		    
		if (status != E_DB_OK )
		{
		    if (errcb->err_code == E_DM0055_NONEXT)
		    {
			CLRDBERR(errcb);
			status = E_DB_OK;
			break;
		    }

		    uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		    break;
		}
		if (base_relrecord.reltid.db_tab_base != table_id.db_tab_base)
		    continue;
		if (base_relrecord.reltid.db_tab_index != table_id.db_tab_index)
		    continue;

		/*
                ** For the base table record, update the record.
                */

		if (index)
		{
		    base_relrecord.relidxcount += 1;
		}
                if (view)
		{
		    base_relrecord.relstat |= TCB_VBASE;
		}
	    	status = dm2r_replace(rel_rcb, &tid,
                                 DM2R_BYPOSITION, (char*)&base_relrecord,
				 (char *)0, errcb);
		if (status != E_DB_OK )
		{
		    uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		    break;
		}
	    } /* End update of base table record. */
	}
	if (status != E_DB_OK)
	    break;

	/*
	** If we are creating the system catalogs for a gateway table,
	** then call to the gateway to register the table.
	*/
	if (gateway)
	{
	    /*
	    ** 16-jun-90 (linda)
	    **	NOTE that tup_cnt, although passed in to dmf_gwt_register, is
	    **	not used .  RMS Gateway will, however, pass back a "page" count
	    **	(== blocks allocated to file).
	    */
	    i4	    tup_cnt = 0;
	    i4	    pg_cnt = 0;

	    /*
	    ** 25-may-90 (linda, bryanp, walt)
	    ** Close iirelation table here, because need to release the
	    ** physical page lock taken above so that gwt_register can do
	    ** a dmt_show() which needs to take a shared page lock.
	    */
	    if (rel_rcb)
	    {
		local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
		rel_rcb = (DMP_RCB *)0;
		if (local_status)
		{
		    status = local_status;
		    *errcb = local_dberr;
		    uleFormat(errcb, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&local_error, 0);
		    break;
		}
	    }
	    if (attr_rcb)
	    {
		local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
		attr_rcb = (DMP_RCB *)0;
		if (local_status)
		{
		    status = local_status;
		    *errcb = local_dberr;
		    uleFormat(errcb, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&local_error, 0);
		    break;
		}
	    }

	    status = dmf_gwt_register(scb->scb_gw_session_id, table_name, owner,
		&ntab_id, attr_count, attr_nametot, attr_entry,
		gwchar_array, gwattr_array,
		xcb, gwsource, gw_id, &tup_cnt, &pg_cnt, dmu_chars, errcb);
	    if (status)
		break;

	    /* Open the relation table. */

	    table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	    table_id.db_tab_index = DM_I_RELATION_TAB_ID;

	    status = dm2t_open(dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20,
		xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0,
		(i4)0,  db_lockmode, &xcb->xcb_tran_id, &timestamp,
		&rel_rcb, (DML_SCB *)0, errcb);
	    if (status)
		break;

	    /*
	    ** Check for NOLOGGING - no updates should be written to the log
	    ** file if this session is running without logging.
	    */
	    if (xcb->xcb_flags & XCB_NOLOGGING)
		rel_rcb->rcb_logging = 0;
	    rel_rcb->rcb_usertab_jnl = (journal_flag) ? 1 : 0;

	    /*
	    ** Update the iirelation entry to reflect the number of "pages" in
	    ** the gateway table.  For the RMS Gateway, for now, this is just
	    ** the number of allocated blocks in the file.  We may want to
	    ** refine the meaning at some point.  At least the optimizer has
	    ** something relatively meaningful to use in estimates.
	    */
	    STRUCT_ASSIGN_MACRO(ntab_id, table_id);
	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = 1;
	    key_desc[0].attr_value = (char *) &table_id.db_tab_base;

	    status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, errcb);
	    if (status != E_DB_OK)
	    {
		uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    for (;;)
	    {
		status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT,
					(char *)&base_relrecord, errcb);
		if (status != E_DB_OK )
		{
		    if ( errcb->err_code == E_DM0055_NONEXT )
		    {
		        CLRDBERR(errcb);
			status = E_DB_OK;
		    }
		    break;
		}
		if ((base_relrecord.reltid.db_tab_base ==
						table_id.db_tab_base) &&
		    (base_relrecord.reltid.db_tab_index ==
						table_id.db_tab_index))
		{
		    base_relrecord.relgwother = 0;/* unused, so set to 0 */
		    base_relrecord.relpages = pg_cnt;

		    status = dm2r_replace(rel_rcb, &tid, DM2R_BYPOSITION,
			(char*)&base_relrecord, (char *)0, errcb);
		    if (status != E_DB_OK )
			break;
		}
	    }
	    if (status != E_DB_OK)
	    {
		uleFormat(errcb, 0, (CL_ERR_DESC *)0, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    /*
	    ** Since this is a gateway table, we don't want to create
	    ** the underlying files below, so set nofile_create.
	    */
	    nofile_create = TRUE;
	}

	status = E_DB_OK;

	/*
	** Log a create-verify log record, used to verify that
	** recovery recreates the record as originally created.
	**
	** Tables which have no underlying physical structures have no
	** crverify processing as the file are not recreated in recovery
	** and iirtemp/iiridxtemp cannot be crverified as they have no
	** system catalog entries and cannot be logically opened.
	*/
	if (((xcb->xcb_flags & XCB_NOLOGGING) == 0) &&
	    ! nofile_create &&
	    (STncmp((char *)table_name->db_tab_name,
		(upcase ? (char *)"IIRTEMP" : (char *)"iirtemp"), 7 ) != 0) &&
	    (STncmp((char *)table_name->db_tab_name, 
		(upcase ? (char *)"IIRIDXTEMP" : (char *)"iiridxtemp"),
		10 ) != 0) )
	{
	    status = dm0l_crverify(xcb->xcb_log_id, journal_flag,
		&ntab_id, table_name, &relrecord.relowner,
		relrecord.relallocation,
		1,			/* fhdr pageno */
		1,			/* Number of fmaps */
		relrecord.relpages,
		relrecord.relmain,
		relrecord.relprim,
		relrecord.relpgsize,
		&lsn, 
		errcb);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Log the DMU operation.  This marks a spot in the log file to
	** which we can only execute rollback recovery once.  If we now
	** issue update statements against the newly-created table, we
	** cannot do abort processing for those statements once we have
	** begun backing out the create.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_dmu(xcb->xcb_log_id, journal_flag, &ntab_id,
		table_name, owner, (i4)DM0LCREATE, (LG_LSN *)0, errcb);
	    if (status != E_DB_OK)
		break;
	}

	if (! nofile_create)
	{
	    status = dm2f_close_file(loc_array, loc_count, (DM_OBJECT *)dcb,
		    errcb);
	    if (status != E_DB_OK)
		break;

	    status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
		    loc_array, loc_count, DM2F_ALLOC, errcb);
	    if (status != E_DB_OK)
		break;
	}
    
    } while (FALSE);

    if ( status )
    {
	/* If we have an error saved up to log - then do it */
	if (log_err.err_code && errcb->err_code > E_DM_INTERNAL)
	{
	    uleFormat(&log_err, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		    sizeof(DB_DB_NAME), &dcb->dcb_name);
	}
    }

    /*
    ** Close down any of the core system catalogs that haven't been closed
    ** yet. If we get any errors here, then log the error and return it.
    */

    if (rel_rcb)    /* may be 0 if gwt_register() above failed. */
    {
	local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*errcb = local_dberr;
		status = local_status;
	    }
	}
    }

    if (attr_rcb)   /* May not be open for partitions or gateway */
    {
	local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*errcb = local_dberr;
		status = local_status;
	    }
	}
    }

    if (dev_rcb)
    {
	local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*errcb = local_dberr;
		status = local_status;
	    }
	}
    }

    if ( (loc_array) && (loc_array[0].loc_fcb != 0))
    {
	local_status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
	    loc_array, loc_count, DM2F_ALLOC, &local_dberr);
	if (local_status)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
	    		NULL, 0, NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*errcb = local_dberr;
		status = local_status;
	    }
	}
    }

    if (loc_cb != 0)
	dm0m_deallocate((DM_OBJECT **)&loc_cb);

    if ( status )
	return (E_DB_ERROR);

    if (index)
    {
	idx_id->db_tab_base = ntab_id.db_tab_base;
	idx_id->db_tab_index = ntab_id.db_tab_index;
    }
    else
    {
	tbl_id->db_tab_base = ntab_id.db_tab_base;
	tbl_id->db_tab_index = ntab_id.db_tab_index;
    }

    return (E_DB_OK);
}

/*
** Name: dm2u_maxreclen	    - what is the max rec length for this page size?
**
** Description:
**	This routine is called by various software which wishes to know the
**	maximum record length for a page of a particular size.
**
** Inputs:
**	page size	    - page size for this table. One of:
**				2048, 4096, 8192, 16384, 32768, 65536
**
** Outputs:
**	None
**
** Returns:
**	maximum row length which will fit on a page of this size (after
**	allowance for header, line table, etc.)
**
** History:
**	06-mar-1996 (stial01 for bryanp)
**	    Created in order to isolate/remove DB_MAXTUP dependencies.
**      06-mar-1996 (stial01)
**          Changed parameters to dm2u_maxreclen.
**      06-may-1996 (nanpr01)
**          Use header constant macros for Multiple Page Format project.
**	    Also subtract the tuple header length if one is available.
**	    Also take out alter_ok flag since this will not be required.
**	09-jul-1996 (thaju02)
**	    Set maximum tuple length for 64k pages is 32767.
**	    Limitation due to system catalog fields being i2 (e.g.
**	    iirelation.relwid and iiattribute.attoff).
*/
i4
dm2u_maxreclen(i4 input_page_type, i4 page_size)
{
    i4	maxreclen;
    i4	page_type;
    i4	overhead;

    if (!DM_VALID_PAGE_TYPE(input_page_type))
    {
	return 0;
    }

    /*
    ** We don't want to have to document different max tuple lengths
    ** for different page types...
    ** We can do this for 4k-64k pages provided new page types for  
    ** these page sizes always have smaller page/row overheads than V2
    ** page/row overheads.
    ** We can't do this for V1-2k pages because new page types have larger
    ** page/row overheads than the original V1 page/row overheads.
    */
    if (page_size == DM_COMPAT_PGSIZE && input_page_type == TCB_PG_V1)
	page_type = TCB_PG_V1;  /* for compatibility */
    else 
	page_type = TCB_PG_V2; /* has largest page/row overheads */

    overhead = DMPPN_VPT_OVERHEAD_MACRO(page_type) +
		DM_VPT_SIZEOF_LINEID_MACRO(page_type) +
		DMPP_VPT_SIZEOF_TUPLE_HDR_MACRO(page_type);
    maxreclen = page_size - overhead;

    return (maxreclen);
}
