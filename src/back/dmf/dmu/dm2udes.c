/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <hshcrc.h>
#include    <dudbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <scf.h>
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
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm1p.h>
#include    <dmfgw.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dma.h>
#include    <cm.h>
#include    <gwf.h>
#include    <dmfcrypt.h>

/**
**
**  Name: DM2UDESTROY.C - Destroy table utility operation.
**
**  Description:
**      This file contains routines that perform the destroy table
**	functionality.
**	    dm2u_destroy    - Destroy a permanent table.
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
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of destroy operation.
**	22-feb-1990 (fred)
**	    Added has_extensions parameter.  This is used to indicate that the
**	    table being (just) destroyed has (had) extensions.  This will be
**	    used by the logical layer to call the table extension routines to
**	    destroy any extensions as well.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-dec-91 (rogerk)
**		Added support for Set Nologging.
**	    17-mar-1991 (bryanp)
**		B35359: Don't assume 2-ary's have same set of locations as base
**		table in dm2u_destroy.
**	15-apr-1992 (bryanp)
**	    Remove "fcb" argument to dm2f_rename().
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	31-oct-1992 (jnash)
**	    Reduced Logging Project: 
**	    New params to dm0l_frename, dm0l_dmu.
**	    Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-nov-1992 (robf)
**          Remove dm0a.h
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix system catalog pages following updates. This improves
**		    concurrency, but more importantly it ensures that we comply
**		    with the cluster node failure recovery rule that a
**		    transaction may only fix one core system catalog page at
**		    a time.
**		Removed the "HACK TO COMPILE" commented-out code, which seemed
**		    to have been accidentally integrated.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (andys)
**	    Remove ifdef'd out code which had been the fix for bug 45467.
**	    Bug script does not fail in 6.5.
**	23-aug-1993 (rogerk)
**	    Changed journaling of DDL operations to be dependent only on
**	    the database jnl status and not on the table jnl status.
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	  - Check for dummy iidevices rows in read_tcb.
**	18-oct-1993 (rogerk)
**	    Change destroy to not reference iidevices when dropping core
**	    catalog tables since these can never be multi-location and since
**	    iidevices itself may sometimes be dropped (in upgradedb).
**	19-oct-93 (stephenb)
**	    Updated to pass back the fact that we are dealing with a gateway
**	    table so we can audit in dmu_destroy().
**	17-nov-93 (stephenb)
**	    If we are trying to destroy an SXA gateway table, check that user
**	    has security priv, audit failure and return if not.
**	05-jan-1994 (jnash)
**	    B57729. Fix problem where iidevices position done on single 
**	    location table, resulting in spurious E_DM0074_NOT_POSITIONED 
**	    error.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	2-mar-94 (robf)
**          Changed SXA check from SECURITY privlege to AUDITOR privilege
**	    Also audit security label for table when writing failed access.
**	9-mar-94 (stephenb)
**	    Alter dma_write_audit() calls to current prototype.
**      27-dec-94 (cohmi01)
**          For RAW I/O, pass DM2F_GETRAWINFO when building FCB to obtain any
**          raw extent info. Pass DMP_LOCATION pointer to dm2f_rename(). When 
**          updating pending queue, add additional info to XCCB.
**	08-feb-95 (cohmi01)
**	    For RAW I/O, propagate DCB_RAW bit to LOC_RAW in loc_status.
**	07-mar-1995 (shero03)
**	    Bug #67276
**	    Examine the location usage when scanning the extents.
**      17-may-95 (dilma04)
**          Do not let the user drop an index on READONLY table.
**      06-mar-1996 (stial01)
**          Pass page_size to dm2f_build_fcb()
**	29-apr-1996 (shero03)
**	    Support RTree indexes
**      16-may-97 (dilma04)
**          Bug 81835. Obtain and pass timeout value to dm2t_control() and 
**          dm2t_open().
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	26-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**      21-mar-1999 (nanpr01)
**          Support for raw locations.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_table_access() if neither
**	    C2 nor B1SECURE.
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**	20-Feb-2004 (schka24)
**	    Make drop of a partitioned table work.
**	1-Mar-2004 (schka24)
**	    comment update only
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dma_?, dmf_gw? functions converted to DB_ERROR *
**	26-Aug-2009 (kschendel) b121804
**	    Need hshcrc.h for gcc 4.3.
[@history_template@]...
**/

GLOBALREF	DMC_CRYPT	*Dmc_crypt;


/*{
** Name: dm2u_destroy - Destroy a permanent table.
**
** Description:
**      This routine is called from dmu_destroy() to destroy a permanent table.
**
**	If the table is indexed, or partitioned, all indexes and/or
**	partitions are dropped as well.  If the table is a partition, the
**	caller had better have "modify request" set, and indeed actually
**	be in a modify with control locks taken, etc.
**
** Inputs:
**	dcb				The database id. 
**	xcb				The transaction id.
**	tbl_id				The internal table id of the table to be
**					destroyed. (ID consists of two components:
**					base table id and index table ID)
**      flag                            Either zero or some of:
**		DM2U_MODIFY_REQUEST	the drop is called from modify,
**					so skip taking control locks, opening
**					tables, etc.
**		DM2U_DROP_NOFILE	Skip the actual disk file rename
**					(but do everything else).  See
**					discussion of DMU_DROP_NOFILE in
**					dmucb.h.
**	db_lockmode			Lockmode of the database for the caller.
**	has_extensions			Ptr to i4 to recieve indication
**					the table being destroyed has extensions.
** Outputs:
**      err_code                        The reason for an error return status.
**	*has_extensions			Filled as appropriate.
**	*table_type			Type of table
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
**          Created from dmu_destroy().
**	01-jun-1877 (teg)
**	    modified to handle deleting table without physical file.
**	30-Aug-88 (teg)
**	    fixed location specific error propagation bug.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	21-Oct-1988 (teg)
**	    modified approach to deletling table without physical file because
**	    of modified approach to DMTSHOW.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-feb-89 (EdHsu)
**	    Add call to dm2u_ckp_lock to be mutually exclusive with ckp.
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
**	    Log DMU operation at the end of the Destroy.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.
**	    Call dmf_gwt_remove to tell the gateway we are removing the table.
**	    Added support for creating/destroying tables that have no
**	    underlying physical file.
**	22-feb-1990 (fred)
**	    Added has_extensions parameter.  This is used to indicate that the
**	    table being (just) destroyed has (had) extensions.  This will be
**	    used by the logical layer to call the table extension routines to
**	    destroy any extensions as well.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**	    15-feb-91 (rajp, bryanp)
**		Fix for bug #35359 where secondary indexes were not being
**		dropped cleanly when referenced on a base table spread over
**		more than one location.  Fix involved not assuming that the
**		base table had the same set of locations as each of its
**		secondary indexes.  Logic for determining loc_count moved into
**		the loop which processed qualifying base and index records.
**		The retrieved record's reltid should be checked against tbl_id
**		not table_id, as table_id's value could be changed while
**		executing in the same loop.  Need to position the table using
**		base table id before getting and deleting the index record.
**	15-apr-1992 (bryanp)
**	    Remove "fcb" argument to dm2f_rename().
**      22-jul-1992 (andys)
**          When an index is deleted we update the iirelation tuple for
**          the base table to reflect the change in numbers of indices
**          and so on.  This update should be journaled if journaling
**          is turned on for the base table. [bug 45467]
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	17-oct-92 (robf)
**	    Set RCB journaling flag for Gateway tables, otherwise ROLLFORWARDDB
**	    of a REMOVE operation may fail, with not all the necessary info
**          being journaled.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	31-oct-1992 (jnash)
**	    Reduced Logging Project: Fill in loc_config_id field in
**	    loc_array, add params New params to dm0l_frename, dm0l_dmu,
**	    dm0l_destroy.  Also, cleaned up long lines, ule_format's, etc.
**	14-dec-1992 (jnash)
**	    Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**      17-dec-1992 (schang)
**          delay the closing of user table so that we can use (rms) file name
**          in file closing at GW table remove time.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Unfix system catalog pages following updates. This improves
**		    concurrency, but more importantly it ensures that we comply
**		    with the cluster node failure recovery rule that a
**		    transaction may only fix one core system catalog page at
**		    a time.
**		Added some additional error checking in places where we were
**		    previously ignoring error returns.
**		Removed the "HACK TO COMPILE" commented-out code, which seemed
**		    to have been accidentally integrated.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables. The table is not opened
**	    via "dmt_open" hence we bypass the "readonly-check" so add
**	    a similar check here not allowing the user to drop a READONLY
**	    table.
**	23-aug-1993 (andys)
**	    Remove ifdef'd out code which had been the fix for bug 45467.
**	    Bug script does not fail in 6.5.
**	23-aug-1993 (rogerk)
**	  - Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	  - Changed journaling of DDL operations to be dependent only on
**	    the database jnl status and not on the table jnl status.
**	20-sep-1993 (rogerk)
**	  - Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	  - Collapsed gateway variable into journal setting for journal
**	    decisions since gateway table DDL are always treated as journaled.
**	    This also prevents us from logging DM0L_JOURNAL records on a
**	    non-journaled database.
**	  - Add support for new iidevices table handling.  Due to non-jnl
**	    protocols, iidevices rows are not deleted when a table is reorged
**	    to a fewer number of locations.  Rather, they are updated to be
**	    dummy iidevices rows.  We must now check for these dummy rows when
**	    deleting the rows from the catalog.  We now open and read the
**	    iidevices table when dropping tables even when the table indicates
**	    it is a single-location object.
**	  - Moved the replace of the base table iirelation row in drop index
**	    processing to after the delete of the index iirelation row.
**	    Rollforward keeps track of index delete operations on non-journaled
**	    base tables so it can determine how to rollforward the base table
**	    iirelation update.
**	18-oct-1993 (rogerk)
**	    Change destroy to not reference iidevices when dropping core
**	    catalog tables since these can never be multi-location and since
**	    iidevices itself may sometimes be dropped (in upgradedb).
**	19-oct-93 (stephenb)
**	    changed is_view variable name to table_type and updated it to 
**	    contain 3 values (table (0), TCB_VIEW or TCB_GATEWAY), we need the
**	    info in dmu_destroy() to audit.
**	17-nov-93 (stephenb)
**	    If we are trying to destroy an SXA gateway table, check that user
**	    has security priv, audit failure and return if not.
**	05-jan-1994 (jnash)
**	    B57729. Fix problem where iidevices position done on single 
**	    location table, resulting in spurious E_DM0074_NOT_POSITIONED 
**	    error.
**	28-jun-94 (robf)
**          Return table security label on B1 so caller can do security
**	    auditing appropriately at the higher level (this follows the
**	    existing model for table_type)
**	27-dec-94 (cohmi01)
**	    For RAW I/O, pass DM2F_GETRAWINFO when building FCB to obtain any
**	    raw extent info. Pass DMP_LOCATION pointer to dm2f_rename(). When 
**	    updating pending queue, add additional info to XCCB.
**	07-mar-1995 (shero03)
**	    Bug #67276
**	    Examine the location usage when scanning the extents.
**      17-may-95 (dilma04)
**	    Do not let the user drop an index on READONLY table.
**      06-mar-1996 (stial01)
**          Pass page_size to dm2f_build_fcb()
**	29-apr-1996 (shero03)
**	    Support RTree indexes
**	03-jan-1997 (nanpr01)
**	    Use the #define's rather than hard-coded values.
**      16-may-97 (dilma04)
**          Bug 81835. Obtain and pass timeout value to dm2t_control() and 
**          dm2t_open().
**	06-jun-97 (nanpr01)
**	    Initialize new field in xccb.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance
**	    enhancement.
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**	14-Jan-2004 (jenjo02)
**	    Check tcb_rel.relstat & TCB_INDEX instead of tbl_id->db_tab_index.
**	20-Feb-2004 (schka24)
**	    Partition drop support.
**	10-Mar-2004 (schka24)
**	    Need another flag for partition drop as part of repartitioning
**	    modify.  (to distinguish it from "modify to drop partition",
**	    when we get around to doing that.)
**	11-Jun-2004 (schka24)
**	    Fix sloppy error handling from gateway drops, caused segv's
**	    in imasql runall test.
**	6-Aug-2004 (schka24)
**	    Table-access check now a no-op, remove.
**  29-Sep-2004 (fanch01)
**      Conditionally add LK_LOCAL flag if database/table is confined to one node.
**	11-feb-2005 (thaju02)
**	    Online modify: DM2U_ONLINE_REPART (targets less than sources).
**	    File may not exist for each partition entry.
**	17-Jan-2006 (kschendel)
**	    (For lluo@datallegro.com) Compute the name lock key the same
**	    way that create does.  Otherwise drop and an uncommitted create
**	    get into a race; also concurrent drops on long table names
**	    improperly interfere with one another.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**	10-Nov-2009 (kschendel) SIR 122757
**	    No need for db sync flag here, files aren't actually opened.
**	20-Apr-2010 (toumi01) SIR 122403
**	    If the table is encrypted, delete its enabling Dmc_crypt entry.
**	5-May-2010 (kschendel)
**	    It's kind of silly to try to open iidevices when dropping
**	    iidevices, so don't do it.
**	09-Jun-2010 (jonj) SIR 121123
**	    Adapt hash of owner, table_name to long names.
**	27-Jul-2010 (toumi01) BUG 124133
**	    Store shm encryption keys by dbid/relid, not just relid! Doh!
*/
DB_STATUS
dm2u_destroy(
DMP_DCB		*dcb,		    /* data base control block */
DML_XCB		*xcb,		    /* transaction control block */
DB_TAB_ID       *tbl_id,	    /* table id - base table and index table */
i4         db_lockmode,
i4         flag,
DB_TAB_NAME	*tab_name,
DB_OWN_NAME	*tab_owner,
i4		*table_type,
i4		*has_extensions,
DB_ERROR	*dberr)	    
{
    DML_XCCB        *xccb;	   
    i4         ref_count;
    DMP_TCB         *tcb;			/* table control block */
    DMP_RCB	    *dest_rcb = (DMP_RCB *)0;	/* rcb for tbl destroyed */
    DMP_RCB	    *rel_rcb = (DMP_RCB *)0;	/* rcb for iirelation */
    DMP_RCB	    *attr_rcb = (DMP_RCB *)0;	/* rcb for iiattribute */
    DMP_RCB	    *dev_rcb = (DMP_RCB *)0;	/* rcb for iidevices */
    DMP_RCB	    *index_rcb = (DMP_RCB *)0;	/* rcb for iiindex */
    DMP_RCB	    *rtree_rcb = (DMP_RCB *)0;	/* rcb for iirange */
    i4	    error, local_error;
    i4	    status, local_status;
    i4	    i,k;
    i4         attr_count, dest_count;
    DMP_ATTRIBUTE   attrrecord;			/* iiattribute tuple */
    DMP_RELATION    relrecord;			/* iirelation tuple */
    DMP_INDEX       indexrecord;		/* iiindex tuple */
    DMP_DEVICE      devrecord;		        /* iidevice tuple */
    DMP_RANGE       rtreerecord;	        /* iirange tuple */
    DM_FILE         filename;
    DB_TAB_NAME     table_name;
    DB_OWN_NAME     owner_name;
    DB_LOC_NAME	    location;
    i4         filelength, oldfilelength;
    i4         journal;
    i4 	    gateway;
    i4 	    view;
    i4 	    sconcur;
    DB_TAB_ID       table_id;
    DB_TAB_TIMESTAMP timestamp;
    i4         compare;
    DM_TID          tid;
    DM2R_KEY_DESC   key_desc[2];
    CL_ERR_DESC	    sys_err;
    LK_LOCK_KEY      lockkey;
    LK_LKID          lockid;
    bool             syscat;
    bool	    no_file = FALSE;
    bool	    nodevices_destroy = FALSE;
    DML_SCB          *scb;
    i4	     lk_mode;
    DB_ERROR	    log_err;
    i4         loc_count = 1;
    DMP_MISC        *loc_cb = 0;
    DMP_LOCATION    *loc_array;
    LG_LSN	    lsn;
    i4	    journal_flag;
    i4	    dm0l_flag;
    i4		timeout = 0;
    char	*tempstr2;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);
    CLRDBERR(&log_err);

    status = dm2u_ckp_lock(dcb, tab_name, tab_owner, xcb, dberr);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    dmf_svcb->svcb_stat.utl_destroy++;	    /* # times delete utility evoked */

    scb = xcb->xcb_scb_ptr;	/* session control block reference */
    lk_mode = DM2T_X;		/* exclusive (or write) lock */

    /* 
    ** if condition met, we are attempting to delete a core 
    ** catalog -- should never happen. 
    */
    if (tbl_id->db_tab_base <= DM_SCONCUR_MAX)   
	lk_mode = DM2T_IX;

    timeout = dm2t_get_timeout(scb, tbl_id); /* from set lockmode */

    /* 
    ** All parameters are acceptable, now check the table id.
    */

    for (;;)
    {
	if ((flag & DM2U_MODIFY_REQUEST) == 0)
	{   
	    /* 
	    ** We are called from dmu_destroy, so we have more work to do than
	    ** if we were merely deleleting an index table that's already
	    ** been locked.  We have to get an exclusive lock on the table,
	    ** then check the TCB to make sure that our transaction isnt 
	    ** already using this table.
	    */

	    /* obtain a table control lock to block readlock = nolock types */ 
	    status = dm2t_control(dcb, tbl_id->db_tab_base, xcb->xcb_lk_id, 
		LK_X, (i4)0, timeout, dberr);
	    if (status != E_DB_OK)
		break;

	    /* now open the table.  This obtains the RCB, TCB and a lock on
	    ** the table.  Since there is the slight possibility that the table
	    ** we are trying to delete is missing it's physical file, use a
	    ** special access_mode that will skip building the FCB if the 
	    ** data file is missing -- we're trying to get rid of the table 
	    ** anyhow, so we wont be reading data from the file.
	    */
	    status = dm2t_open(dcb, tbl_id, lk_mode, DM2T_UDIRECT, 
		DM2T_A_OPEN_NOACCESS, timeout, (i4)20, 
		xcb->xcb_sp_id, xcb->xcb_log_id, 
		xcb->xcb_lk_id,(i4)0, (i4)0, 
		db_lockmode, &xcb->xcb_tran_id, &timestamp,
		&dest_rcb, (DML_SCB *)0, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Check the tcb to insure that the table to destroy is not
	    ** currently open by this same transaction.
	    */

	    dest_rcb->rcb_xcb_ptr = xcb;
	    tcb = dest_rcb->rcb_tcb_ptr;
	    ref_count = (tcb)  ? tcb->tcb_open_count : 0;
	    if (tab_name)
		*tab_name = dest_rcb->rcb_tcb_ptr->tcb_rel.relid;
	    if (tab_owner)
		*tab_owner = dest_rcb->rcb_tcb_ptr->tcb_rel.relowner;
	    if (table_type)
		*table_type = 
		    dest_rcb->rcb_tcb_ptr->tcb_rel.relstat & 
		    (TCB_VIEW | TCB_GATEWAY);
	    if (has_extensions)
	    {
		*has_extensions = dest_rcb->rcb_tcb_ptr->tcb_rel.relstat2 &
		    TCB2_HAS_EXTENSIONS;
	    }

	    if (ref_count > 1)
	    {   /* table still in use, cant delete */
		status = dm2t_close(dest_rcb, DM2T_KEEP, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
		}
		SETDBERR(dberr, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
		break;
	    }

            /*
            ** Do not let the user drop an index on READONLY table
            */
	    if ( tcb->tcb_rel.relstat & TCB_INDEX &&
                (tcb->tcb_parent_tcb_ptr->tcb_rel.relstat2 & TCB2_READONLY))
            {
                status = dm2t_close(dest_rcb, DM2T_KEEP, dberr);
                if (status != E_DB_OK)
                {
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
                }
		SETDBERR(dberr, 0, E_DM0170_READONLY_TABLE_INDEX_ERR);
                break;
            }

	    /*
	    ** If this is an SXA gateway table, user must have security priv.
	    */
	    if (tcb->tcb_rel.relgwid == GW_SXA)
	    {
		/*
		** Get information from SCF.
		*/
        	SCF_CB      scf_cb;
        	SCF_SCI     sci_list;
		i4	    ustat;

        	scf_cb.scf_length = sizeof(SCF_CB);
        	scf_cb.scf_type = SCF_CB_TYPE;
        	scf_cb.scf_facility = DB_DMF_ID;
        	scf_cb.scf_session = DB_NOSESSION;
        	scf_cb.scf_len_union.scf_ilength = 1;
        	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
        	sci_list.sci_code = SCI_USTAT;
        	sci_list.sci_length = sizeof(ustat);
        	sci_list.sci_aresult = (PTR) &ustat;
        	sci_list.sci_rlength = NULL;
        	status = scf_call(SCU_INFORMATION, &scf_cb);
        	if (status != E_DB_OK)
        	{
		    *dberr = scf_cb.scf_error;
		    break;
        	}
		/*
		**	Now check the AUDITOR bit in the status
		*/
		if ( !(ustat & DU_UAUDITOR)) /* no auditor priv */
		{
		    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
		    {
			/* 
			** Audit failure to remove audit log 
			*/
			status = dma_write_audit(SXF_E_SECURITY, 
			    SXF_A_FAIL | SXF_A_REMOVE, 
			    (char *)tcb->tcb_rel.relid.db_tab_name,
			    sizeof(tcb->tcb_rel.relid.db_tab_name),
			    (DB_OWN_NAME *)&tcb->tcb_rel.relowner,
			    I_SX2042_REMOVE_TABLE, FALSE, &local_dberr, NULL);
		    }

		    status = dm2t_close(dest_rcb, DM2T_KEEP, dberr);
		    if (status != E_DB_OK)
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
		    }
		    SETDBERR(dberr, 0, E_DM201E_SECURITY_DML_NOPRIV);
		    break;
		}
	    }

	    /* 
	    ** check to see if there are any locations associated with
	    ** this table.
	    ** Partitioned masters have locations, handle them specially,
	    ** don't set no-file for them.
	    */
	    if ((tcb->tcb_rel.relstat & (TCB_VIEW | TCB_GATEWAY)) ||
		(tcb->tcb_table_io.tbio_location_array[0].loc_fcb == 
							    (DMP_FCB *) NULL))
	    {
		no_file = TRUE;
	    }
	    if (tcb->tcb_rel.relstat & TCB_IS_PARTITIONED)
		no_file = FALSE;

            /* 
            ** schang: delay the closing until remove so that  we can 
            ** have the oppertunity to close (RMS) file during remove
            */
            if (!(tcb->tcb_rel.relstat & TCB_GATEWAY) ||
                   tcb->tcb_rel.relstat & TCB_INDEX)
            {
	        status = dm2t_close(dest_rcb, DM2T_PURGE, dberr);
                if (status)
		    break;
            }

	} /* endif called by delete instead of modify */

	/* 
	**  Must get record information from relation record,
	**  table has already been locked exclusively by modify
	**  (and is partially destroyed) or was locked earlier
	**  by this routine.
	**  Read the relation table records for the table to drop.
	*/
    
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
						/* this identifies iirelation */
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
   
	status = dm2t_open(dcb, &table_id, DM2T_IX,
		DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, 
                xcb->xcb_sp_id,
		xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
                db_lockmode, &xcb->xcb_tran_id, &timestamp,
		&rel_rcb, (DML_SCB *)0, dberr);
	if (status)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, (i4)0,
			(i4 *)NULL, &local_error, (i4)0);
	    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
	    break;
	}
	rel_rcb->rcb_xcb_ptr = xcb;

	/*  Position table using base table id. */

	table_id.db_tab_base = tbl_id->db_tab_base;
	table_id.db_tab_index = tbl_id->db_tab_index;
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELATION_KEY;
	key_desc[0].attr_value = (char *) &table_id.db_tab_base; 

	status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, (i4)0,
			(i4 *)NULL, &local_error, (i4)0);
	    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
	    break;
	}

	dest_count = 0;
	for (;;)	/* loop until we find the iirelation tuple we want */
	{
	    /* 
	    ** Get a qualifying relation record.  This will retrieve
	    ** all records for a base table (i.e. the base and all
	    ** indices), or just one for an index. 
	    */
	    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
		    (char *)&relrecord, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    CLRDBERR(dberr);
		    status = E_DB_OK;
		    break;
		}
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, (i4)0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    /* If this isnt exactly the tuple we are searching for (such as a
	    ** different index table than the one we want), skip it 
	    */
	    if (relrecord.reltid.db_tab_index != table_id.db_tab_index)
		continue;

	    /* this condition should never be met -- dm2r_get should always
	    **	return a tuple with the db_tab_base that it was asked to match.
	    **	However, we check for it anyhow (in case of corruption) 
	    */
	    if (relrecord.reltid.db_tab_base != table_id.db_tab_base)
		continue;

	    /* we have the tuple we want */


	    if ((flag & DM2U_MODIFY_REQUEST) == 0)
	    {   /* we were not called from modify to drop an index table, so
		**  we potentially have more work to do.  We must make sure
		**  that its legal to drop this table.  Make sure that it
		**  is not a system catalog.  Also make sure that this table
		**  is not being used by this transaction
		*/

		/* determine if table is system catalog -- we dont delete these */
		syscat = (relrecord.relstat & (TCB_CATALOG | TCB_EXTCATALOG)) 
		      == TCB_CATALOG;

		if (syscat && 
                    ((scb->scb_s_type & SCB_S_SYSUPDATE)==0)
                   )  /* cant delete sys catalog */
		{
		    SETDBERR(dberr, 0, E_DM005E_CANT_UPDATE_SYSCAT);
		    break;
		}	        
		if (syscat && (relrecord.relstat & TCB_SECURE) &&
                    ((scb->scb_s_type & SCB_S_SECURITY)==0)
                   )  /* cant delete security catalog */
		{
		    SETDBERR(dberr, 0, E_DM0129_SECURITY_CANT_UPDATE);
		    break;
		}	        

		/*
		** Do not let the user drop a READONLY table
		*/
		if ( relrecord.relstat2 & TCB2_READONLY )
		{
		    SETDBERR(dberr, 0, E_DM0067_READONLY_TABLE_ERR);
		    break;
		}

	    } /* endif called by delete instead of modify */


	    /* we have the tuple we want, so save table name, owner name,
	    **	location and journal status 
	    */
	    MEcopy((char *)&relrecord.relid, sizeof(DB_TAB_NAME),
		(char *)&table_name);

	    MEcopy((char *)&relrecord.relowner, sizeof(DB_OWN_NAME),
	        (char *)&owner_name);

	    MEcopy((char *)&relrecord.relloc, sizeof(DB_LOC_NAME),
		(char *)&location);

	    journal = (relrecord.relstat & TCB_JOURNAL) ? 1 : 0;
	    gateway = (relrecord.relstat & TCB_GATEWAY) ? 1 : 0;
	    view    = (relrecord.relstat & TCB_VIEW)    ? 1 : 0;
	    sconcur = (relrecord.relstat & TCB_CONCUR)  ? 1 : 0;

	    /*
	    ** Check for destroy of special tables that may preclude lookups
	    ** to the iidevices table because iidevices may not exist.
	    ** Core catalogs and sysmod temps fall into this group.
	    ** If destroying iidevices itself (upgradedb), don't lookup
	    ** into it (duh).  None of these are multi-location catalogs
	    ** so it's ok.
	    */
	    if ((sconcur) ||
		(STncasecmp((char *)&table_name, "iirtemp",7) == 0) ||
		(STncasecmp((char *)&table_name, "iiridxtemp",10) == 0) ||
		(table_id.db_tab_base == DM_B_DEVICE_TAB_ID
		 && table_id.db_tab_index == DM_I_DEVICE_TAB_ID) )
	    {
		nodevices_destroy = TRUE;
	    }

	    /*
	    ** Journaling of the updates associated with this DDL action are
	    ** partially conditional upon the journaling state of the table.
	    **
	    ** The system catalog deletes will be journaled regardless of
	    ** the table state as is the non-journal table protocols for
	    ** destroy actions.  The file deletion will be similarly journaled.
	    **
	    ** Actions associated with gateway table and view destruction
	    ** are always journaled.
	    */
	    journal_flag = (journal ? DM0L_JOURNAL: 0);
	    if ((gateway || view) && (dcb->dcb_status & DCB_S_JOURNAL))
		journal_flag = DM0L_JOURNAL;

	    /* now close iirelation */
	    status = dm2t_close(rel_rcb, DM2T_NOPURGE, dberr);
	    if (status != E_DB_OK)
		break;

	    /* 
	    ** exit this loop
	    */
	    break;
	} /* end for (loop to find the iirelation tuple we want) */
	break;
    } /* end for (loop to handle errors by breaking out of it) */    

    if (dberr->err_code)   /* we cant continue deleteing this file */
    {
	if (rel_rcb != 0)  /* if iirelation is still open, close it */
	{
	    local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
	    }
	}

	if (log_err.err_code && dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(&log_err, 0, NULL, ULE_LOG, NULL,
			(char *) NULL, 0L, (i4 *)NULL, &local_error, 1,
			sizeof(DB_DB_NAME),
			&dcb->dcb_name);
	}
	return (E_DB_ERROR);
    }
    
    /* It looks like we're going to do the destroy. If this is a base record
    ** and encryption is enabled for the system, check for a Dmc_crypt key
    ** entry slot to delete.
    */
    if ( Dmc_crypt != NULL && table_id.db_tab_index == 0 )
    {
	DMC_CRYPT_KEY	*cp;
	i4		keycount;
	bool		found_it = FALSE;

	for ( cp = (DMC_CRYPT_KEY *)((PTR)Dmc_crypt + sizeof(DMC_CRYPT)),
		keycount = 0 ;
		keycount < Dmc_crypt->seg_active ; cp++, keycount++ )
	{
	    /* found the entry for this record */
	    if ( cp->db_id == dcb->dcb_id &&
		 cp->db_tab_base == table_id.db_tab_base )
	    {
		found_it = TRUE;
		break;
	    }
	}
	if ( found_it == TRUE )
	{
	    CSp_semaphore(TRUE, &Dmc_crypt->crypt_sem);
	    /* if dirty read is verified, delete the entry */
	    if ( cp->db_id == dcb->dcb_id &&
		 cp->db_tab_base == table_id.db_tab_base )
	    {
		MEfill(sizeof(DMC_CRYPT_KEY), 0, (PTR)cp);
		cp->status = DMC_CRYPT_INACTIVE;
	    }
	    else
		TRdisplay("Dropping a table, deleting the encryption key for reltid %d, the\nDmc_crypt slot was not found. This is not expected, but is not an error.\n",cp->db_tab_base);
	    CSv_semaphore(&Dmc_crypt->crypt_sem);
	}
    }

    do		/* here we have table name, owner name, etc and exclusive lock
		   on table.  We can preceed with the delete. */
    {
	/* Lock the table name so nobody can create a table of the same
	** name until we're completely done deleting it.
	** (the control lock only protects the table ID).
	** There's no need for this for partitions because the name
	** includes the table ID, so the same name can't be reused.
	** And there might be a bazillion partitions and we don't want
	** run the user out of locks.
	*/
	if (tbl_id->db_tab_index >= 0)
	{
	    i4		olen, orem, nlen, nrem;

	    olen = sizeof(owner_name) / 2;
	    orem = sizeof(owner_name) - olen;
	    nlen = sizeof(table_name) / 3;
	    nrem = sizeof(table_name) - (nlen * 2);

	    lockkey.lk_type = LK_CREATE_TABLE; 
	    lockkey.lk_key1 = dcb->dcb_id;    /* till were done deleteing it */
	    /* There are 5 i4's == 20 bytes of key to work with.
	    ** Force-fit the owner into the first 8 and the name into
	    ** the last 12, hashing if necessary.
	    */
	    tempstr2 = (char *)&owner_name;    /* see if owner fits in 8 bytes */
	    for (i = 0; i < 9; i++, tempstr2++)
	    {
		if (*tempstr2 == ' ')
		    break;
	    }		
	    if (i < 9)
		MEcopy((PTR)&owner_name, 8, (PTR)&lockkey.lk_key2);
	    else
	    {
	       lockkey.lk_key2 = HSH_char((PTR)&owner_name, olen);
	       lockkey.lk_key3 = HSH_char((PTR)&owner_name + olen, orem);
	    }
	    tempstr2 = (char *)&table_name;  /* see if table fits in 12 bytes */
	    for (i = 0; i < 13; i++, tempstr2++)
	    {
		if (*tempstr2 == ' ')
		    break;
	    }
	    if (i < 13)
		MEcopy((PTR)&table_name, 12, (PTR)&lockkey.lk_key4);
	    else
	    {	
		lockkey.lk_key4 = HSH_char((PTR)&table_name, nlen);
		lockkey.lk_key5 = HSH_char((PTR)&table_name + nlen, nlen);
		lockkey.lk_key6 = HSH_char((PTR)&table_name + 2*nlen, nrem);
	    }
	    MEfill(sizeof(LK_LKID), 0, &lockid);

	    status = LKrequest(dcb->dcb_bm_served == DCB_SINGLE ?
                               LK_LOCAL : 0, xcb->xcb_lk_id, &lockkey,
                               LK_X, (LK_VALUE * )0, &lockid, 0L, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
		if (status == LK_NOLOCKS)
		{	
		    uleFormat(NULL, E_DM9046_TABLE_NOLOCKS, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 2, sizeof(DB_TAB_NAME), &table_name,
			sizeof(DB_DB_NAME), &dcb->dcb_name);
		    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		    break;
		}

		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG , NULL,
	            (char * )NULL, 0L, (i4 *)NULL, 
                    &local_error, 2, 0, LK_X, 0, xcb->xcb_lk_id);
		uleFormat(NULL, E_DM9025_BAD_TABLE_DESTROY, &sys_err, ULE_LOG , NULL,
	            (char * )NULL, 0L, (i4 *)NULL, 
                    &local_error, 2, sizeof(DB_DB_NAME),
                    &dcb->dcb_name, 
                    sizeof(DB_TAB_NAME), &table_name);
		if (status == LK_DEADLOCK)
		{	
		    SETDBERR(dberr, 0, E_DM0042_DEADLOCK);
		    break;
		}
		else
		{
		    SETDBERR(dberr, 0, E_DM009D_BAD_TABLE_DESTROY);
		    break;
		}
	    }
	} /* if not a partition */

    } while (0);

    if (dberr->err_code)
    {
	return (E_DB_ERROR);
    }

    for (;;)
    {
	/* Open the relation, attribute, and index tables.
	** iidevices will get opened later if it's needed.
	** ditto for iirtree.
	*/

	rel_rcb = 0;	/* null pointers to record control blocks to */
	attr_rcb = 0;   /* indicate tables aren't open yet */
	index_rcb = 0;
	dev_rcb = 0;
	rtree_rcb = 0;

	/* reltid, reltidx of iirelation */
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;;

	/* open iirelation with IX lock */   
	status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp,
	    &rel_rcb, (DML_SCB *)0, dberr);
	if (status)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, (i4)0);
	    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
	    break;
	}

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    rel_rcb->rcb_logging = 0;

	/*
	** Indicate user table journaling state.
	** Note that gateway tables are always listed as journaled.
	*/
	rel_rcb->rcb_usertab_jnl = journal_flag ? 1 : 0;

	rel_rcb->rcb_xcb_ptr = xcb;

	/* indicate iiattribute */
        table_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
	table_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

        /* open iiattribute with IX lock   */
	status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp,
	    &attr_rcb, (DML_SCB *)0, dberr);
	if (status)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, (i4)0);
	    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
	    break;
	}

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    attr_rcb->rcb_logging = 0;

        /* indicate if table to delete is journaled */
	attr_rcb->rcb_usertab_jnl = journal_flag ? 1 : 0;
	attr_rcb->rcb_xcb_ptr = xcb;

	/* indicate iiindex */
	table_id.db_tab_base = DM_B_INDEX_TAB_ID;
	table_id.db_tab_index = DM_I_INDEX_TAB_ID;
   
	/* open iiindex with IX lock */
	status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp,
	    &index_rcb, (DML_SCB *)0, dberr);
	if (status)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, (i4)0);
	    SETDBERR(&log_err, 0, E_DM9027_INDEX_UPDATE_ERR);
	    break;
	}

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    index_rcb->rcb_logging = 0;

	/* indicate if the file to delete is journaled */
	index_rcb->rcb_usertab_jnl = journal_flag ? 1 : 0;
	index_rcb->rcb_xcb_ptr = xcb;

	break;

    } /* end for */

    if (dberr->err_code)
    {
        /* format / report error */
	if (log_err.err_code && dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(&log_err, 0, NULL, ULE_LOG, NULL,
		    (char *) NULL, 0L, (i4 *)NULL, &local_error, 1,
		    sizeof(DB_DB_NAME),
		    &dcb->dcb_name);
	}

        /* close any open system catalogs */
	if (rel_rcb)
	{
	    local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
	    }
	}
	if (attr_rcb)
	{
	    local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
	    }
	}
	if (index_rcb)
	{
	    local_status = dm2t_close(index_rcb, DM2T_NOPURGE, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0,
				(i4 *)NULL, &local_error, (i4)0);
	    }
	}

	return (E_DB_ERROR);
    }

    /* 
    **  All parameters have been checked, can really destroy the table. 
    **  First log that a destroy is taking place. 
    **  Take a savepoint for error recovery.
    **  Then update the relation table, the attribute table, and the index table
    **  for each base table  and index table associated with the base. 
    **  When all of this has succeeded, rename the files . 
    **  If any error occur along the way, abort to the savepoint.
    */

    for (;;)
    {

	/*
	** Log the destroy operation - unless logging is disabled.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_destroy(xcb->xcb_log_id, journal_flag, 
				    tbl_id, &table_name, &owner_name, 
				    loc_count, &location, (LG_LSN *)0,
				    &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}


	/* 
	**  Read the relation table records for the table to delete.
	*/
    
	/*  Position table using base table id. */
	table_id.db_tab_base = tbl_id->db_tab_base;
	table_id.db_tab_index = tbl_id->db_tab_index;
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELATION_KEY;
	key_desc[0].attr_value = (char *) &table_id.db_tab_base;

	status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, (i4)0);
	    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
	    break;
	}

	dest_count = 0;
	for (;;)
	{

	    /* 
            ** Get a qualifying relation record.  This will retrieve
            ** all records for a base table (i.e. the base and all
            ** indices), or just one for an index. 
            */

	    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
                              (char *)&relrecord, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    CLRDBERR(dberr);
		    status = E_DB_OK;
		    break;
		}

		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, (i4)0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    /*
	    ** Is this a record we're interested in? If it's for a different
	    ** table, or if we are dropping only a particular secondary index
	    ** and this is not the record for that particular secondary index,
	    ** then continue on to the next iirelation record.
	    **
	    ** Note that we must check the retrieved record's reltid against
	    ** 'tbl_id', not 'table_id', since 'table_id' may be changed
	    ** inside this loop (if, for example, we go and open iidevices).
	    */
	    if (relrecord.reltid.db_tab_base != tbl_id->db_tab_base)
		continue;
	    if (tbl_id->db_tab_index != 0 &&	
		(relrecord.reltid.db_tab_index != tbl_id->db_tab_index))
		continue;	    
	  
            /*  
	    ** Determine the location count for the base table or index we are
	    ** processing. The base table may or may not be spread over the
	    ** same set of locations as its indices. We must figure out the set
	    ** of locations independently for each object we are destroying.
            */

	    if (relrecord.relstat & TCB_MULTIPLE_LOC)
		loc_count = relrecord.relloccount;
	    else if (relrecord.relstat & ( TCB_VIEW | TCB_GATEWAY) )
        	loc_count = 0;
	    else if (no_file)
		loc_count = 0;
	    else
 		loc_count = 1;

	    /* 
	    ** Get the value lock for location devices
	    ** before getting rid of them to make sure that
	    ** the location cannot be used for raw devices for rollback
	    */
	    for (i = 0; i < dcb->dcb_ext->ext_count; i++)
	    {
		if ( (dcb->dcb_ext->ext_entry[i].flags & (DCB_DATA | DCB_RAW))
				== (DCB_DATA | DCB_RAW) &&
		     (MEcmp((char *)&relrecord.relloc, 
			  (char *)&dcb->dcb_ext->ext_entry[i].logical, 
			  sizeof(DB_LOC_NAME))) == 0 )
		{  
		    status = dm2u_raw_device_lock(dcb, xcb,
			    &relrecord.relloc, &lockid, dberr);
		    if (status == LK_NEW_LOCK)
			status = E_DB_OK;
		    break;
		}
	    }
	    if ( status != E_DB_OK )
		break;

	    /* Got a qualifying tuple, delete it from iirelation. */
	    status = dm2r_delete(rel_rcb, &tid, 
                                 DM2R_BYPOSITION, dberr);
	    if (status != E_DB_OK )
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, (i4)0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    dest_count += 1;	/* increment count of # tables destroyed */

	    status = dm2r_unfix_pages(rel_rcb, dberr);
	    if (status)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_error, (i4)0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    /* Get location information. */
	    if (loc_count )
	    {
		/*
		** Open iidevices table if it has not already been opened
		** by a previous pass through this loop.
		**
		** The iidevices table cannot be opened on some destroys since
		** the table may not exist yet during createdb or upgradedb.
		** This is OK since these tables are always single location.
		*/
		if ((dev_rcb == 0) && ( ! nodevices_destroy))
		{
	            /* open iidevices to obtain location info from it */
		    table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
		    table_id.db_tab_index = DM_I_DEVICE_TAB_ID;
	       
		    status = dm2t_open(dcb, &table_id, DM2T_IX,
			DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, 
			(i4)20, xcb->xcb_sp_id, xcb->xcb_log_id, 
			xcb->xcb_lk_id, (i4)0, (i4)0, 
			db_lockmode, &xcb->xcb_tran_id, &timestamp,
			&dev_rcb, (DML_SCB *)0, dberr);
		    if (status)
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);

			SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);
			break;
		    }

		    /*
		    ** Check for NOLOGGING - no updates should be written to
		    ** the log file if this session is running without logging.
		    */
		    if (xcb->xcb_flags & XCB_NOLOGGING)
			dev_rcb->rcb_logging = 0;

		    dev_rcb->rcb_usertab_jnl = journal_flag ? 1 : 0;
		    dev_rcb->rcb_xcb_ptr = xcb;
		}

		/* 
		** Get location information from relation record and 
                ** device records. 
		*/
		loc_cb = 0;
		status = dm0m_allocate(
                    (i4)sizeof(DMP_LOCATION)*loc_count + sizeof(DMP_MISC),
		    DM0M_ZERO, (i4)MISC_CB,
		    (i4)MISC_ASCII_ID, (char *)0, (DM_OBJECT **)&loc_cb, 
		    dberr);
		if (status != E_DB_OK)
		    break;

		loc_array = (DMP_LOCATION *)((char *)loc_cb + sizeof(DMP_MISC));
		loc_cb->misc_data = (char *)loc_array;
		STRUCT_ASSIGN_MACRO(relrecord.relloc,
	                                    loc_array[0].loc_name);
		loc_array[0].loc_fcb = 0;
		loc_array[0].loc_id = 0;
		loc_array[0].loc_status = LOC_VALID;
	    }

	    /* For all indexes, delete index record in index table. */
	    if ( relrecord.relstat & TCB_INDEX )
	    {
		/* Now get and delete the index record. */

        	/* Search iiindex for records matching the given basetable id */

		table_id.db_tab_base = tbl_id->db_tab_base;
		key_desc[0].attr_operator = DM2R_EQ;
		key_desc[0].attr_number = DM_1_INDEX_KEY;
		key_desc[0].attr_value = (char *) &table_id.db_tab_base;

		status = dm2r_position(index_rcb, DM2R_QUAL, key_desc, 
		    (i4)1, (DM_TID *)0, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9027_INDEX_UPDATE_ERR);
		    break;
		}

		for (;;)
		{

		    /* 
		    ** Get a qualifying index record.  
		    */
		    status = dm2r_get(index_rcb, &tid, DM2R_GETNEXT, 
                                            (char *)&indexrecord, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			{
			    CLRDBERR(dberr);
			    status = E_DB_OK;
			}
		    	break;
		    }
		    if (indexrecord.baseid != relrecord.reltid.db_tab_base)
			continue;
		    if (indexrecord.indexid != relrecord.reltid.db_tab_index)
			continue;	    

		    /* Got a qualifying tuple, delete it from iindex */
		    status = dm2r_delete(index_rcb, &tid, 
                                          DM2R_BYPOSITION, dberr);
		    if (status != E_DB_OK)
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
			SETDBERR(&log_err, 0, E_DM9027_INDEX_UPDATE_ERR);
			break;
		    }
	    
		    break;

		} /* end for */

		if ( status )
		    break;	    	

		status = dm2r_unfix_pages(index_rcb, dberr);
		if (status)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9027_INDEX_UPDATE_ERR);
		    break;
		}
	    } /* end if */ 
    
	    /* Now find all the attributes associated with this table.
	    ** If the table we're currently processing is a partition,
	    ** it has no iiattribute rows, don't bother.
	    */

	    if ((relrecord.relstat2 & TCB2_PARTITION) == 0)
	    {
		key_desc[0].attr_operator = DM2R_EQ;
		key_desc[0].attr_number = DM_1_ATTRIBUTE_KEY;
		key_desc[0].attr_value = (char *) &relrecord.reltid.db_tab_base;
		key_desc[1].attr_operator = DM2R_EQ;
		key_desc[1].attr_number = DM_2_ATTRIBUTE_KEY;
		key_desc[1].attr_value = (char *) &relrecord.reltid.db_tab_index;

		status = dm2r_position(attr_rcb, DM2R_QUAL, key_desc, (i4)2,
                          (DM_TID *)0, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		    break;
		}
		attr_count = 0;
		for (;;)
		{
		    /* 
		    ** Get a qualifying attribute record.  
		    */
		    status = dm2r_get(attr_rcb, &tid, DM2R_GETNEXT, 
                                  (char *)&attrrecord, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			{
			    status = E_DB_OK;
			    break;
			}

			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
			SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
			break;
		    }

		    if (attrrecord.attrelid.db_tab_base 
                         != relrecord.reltid.db_tab_base)
			continue;
		    if (attrrecord.attrelid.db_tab_index 
                         != relrecord.reltid.db_tab_index)
			continue;	    

		    /* Got a qualifying record, delete it. */

		    status = dm2r_delete(attr_rcb, &tid, DM2R_BYPOSITION, dberr);
		    if (status != E_DB_OK )
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
			SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
			break;
		    }

		    attr_count += 1;
		} /* end for */

		if (dberr->err_code == E_DM0055_NONEXT && attr_count > 0)
		    CLRDBERR(dberr);
		if ( status )
		    break;

		status = dm2r_unfix_pages(attr_rcb, dberr);
		if (status != E_DB_OK )
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		    break;
		}
	    } /* if not a partition */

    
	    /* Now delete the iirange record associated with this table. */

	    if (relrecord.relspec == TCB_RTREE)
	    {

	      if (rtree_rcb == 0)
	      {
	        /* open iidevices to obtain location info from it */
	        table_id.db_tab_base = DM_B_RANGE_TAB_ID;
	        table_id.db_tab_index = DM_I_RANGE_TAB_ID;
	       
	        status = dm2t_open(dcb, &table_id, DM2T_IX,
	  		DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, 
			(i4)20, xcb->xcb_sp_id, xcb->xcb_log_id, 
			xcb->xcb_lk_id, (i4)0, (i4)0, 
			db_lockmode, &xcb->xcb_tran_id, &timestamp,
			&rtree_rcb, (DML_SCB *)0, dberr);
	        if (status)
	        {
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);
		    break;
	        }
	      }

	      key_desc[0].attr_operator = DM2R_EQ;
	      key_desc[0].attr_number = DM_1_DEVICE_KEY;
	      key_desc[0].attr_value = (char *) &relrecord.reltid.db_tab_base;
	      key_desc[1].attr_operator = DM2R_EQ;
	      key_desc[1].attr_number = DM_2_DEVICE_KEY;
	      key_desc[1].attr_value = (char *) &relrecord.reltid.db_tab_index;

	      status = dm2r_position(rtree_rcb, DM2R_QUAL, key_desc, (i4)2,
                          (DM_TID *)0, dberr);
	      if (status != E_DB_OK)
	      {
		  uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_error, (i4)0);
		  SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		  break;
	      }
	    
	      for (;;)  
	      {
	        status = dm2r_get(rtree_rcb, &tid, DM2R_GETNEXT, 
                                  (char *)&rtreerecord, dberr);
	        if (status != E_DB_OK)
	        {
	          if (dberr->err_code == E_DM0055_NONEXT)
	       	    break;

		  uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_error, (i4)0);
		  SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
	          break;
 	        }

	        if (rtreerecord.range_baseid != 
                         relrecord.reltid.db_tab_base)
	          continue;
	        if (rtreerecord.range_indexid !=
                         relrecord.reltid.db_tab_index)
	          continue;	    

		/* Got a qualifying record, delete it. */

	        status = dm2r_delete(rtree_rcb, &tid, DM2R_BYPOSITION, dberr);
	        if (status != E_DB_OK )
 	        {
		  uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_error, (i4)0);
		  SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
	          break;
	        }

	      } /* end for */

	      if (dberr->err_code == E_DM0055_NONEXT)
	          CLRDBERR(dberr);
	      if (dberr->err_code)
		break;

	      status = dm2r_unfix_pages(rtree_rcb, dberr);
	      if (status != E_DB_OK )
	      {
		  uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_error, (i4)0);
		  SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		  break;
	      }
	    }  /* Rtree index */
	    
	    /* 
	    ** Now find all the additional locations associated 
	    ** with this table. 
	    **
	    ** IIdevices cannot be queried during some table destroys because
	    ** it may not exist during createdb or upgradedb.
	    */
	    if ((loc_count > 1) && ( ! nodevices_destroy))
	    {
		i4 dev_count;

		key_desc[0].attr_operator = DM2R_EQ;
		key_desc[0].attr_number = DM_1_DEVICE_KEY;
		key_desc[0].attr_value = (char *) &relrecord.reltid.db_tab_base;
		key_desc[1].attr_operator = DM2R_EQ;
		key_desc[1].attr_number = DM_2_DEVICE_KEY;
		key_desc[1].attr_value = 
                                        (char *) &relrecord.reltid.db_tab_index;

		status = dm2r_position(dev_rcb, DM2R_QUAL, key_desc, (i4)2,
                          (DM_TID *)0, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);
		    break;
		}

		dev_count = 0;
		for (;;)
		{ 
		    /* 
		    ** Find and delete all tuples from iidevice that describe 
		    ** locations for the table we are deleting.
		    */

		    /* 
		    ** Get a qualifying device record.  
		    */
		    status = dm2r_get(dev_rcb, &tid, DM2R_GETNEXT, 
                                  (char *)&devrecord, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			    break;

			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				&local_error, (i4)0);
			SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);
			break;
		    }
		    if (devrecord.devreltid.db_tab_base 
			    != relrecord.reltid.db_tab_base)
			continue;

		    if (devrecord.devreltid.db_tab_index 
			    != relrecord.reltid.db_tab_index)
			continue;	    

		    /* 
		    ** Get the value lock for location devices
		    ** before getting rid of them to make sure that
		    ** the location cannot be used for raw devices for rollback
		    */
		    for (i = 0; i < dcb->dcb_ext->ext_count; i++)
		    {
			if ( (dcb->dcb_ext->ext_entry[i].flags & (DCB_DATA | DCB_RAW))
					== (DCB_DATA | DCB_RAW) &&
			     (MEcmp((char *)&devrecord.devloc, 
				  (char *)&dcb->dcb_ext->ext_entry[i].logical, 
				  sizeof(DB_LOC_NAME))) == 0 )
			{  
			    status = dm2u_raw_device_lock(dcb, xcb,
				    &devrecord.devloc, &lockid, dberr);
			    if (status == LK_NEW_LOCK)
				status = E_DB_OK;
			    break;
			}
		    }
		    if ( status != E_DB_OK )
			break;

		    /* Got a qualifying record, delete it from iidevices. */
	    
		    status = dm2r_delete(dev_rcb, &tid, 
                                     DM2R_BYPOSITION, dberr);
		    if (status != E_DB_OK )
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				&local_error, (i4)0);
			SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);
			break;
		    }

		    /*
		    ** Record location information for the file deletes
		    ** below.  Ignore dummy iidevices rows (those with blank
		    ** location names).
		    */
		    if (devrecord.devloc.db_loc_name[0] != ' ')
		    {
			dev_count += 1;
			STRUCT_ASSIGN_MACRO(devrecord.devloc,
					    loc_array[dev_count].loc_name);
			loc_array[dev_count].loc_fcb = 0;
			loc_array[dev_count].loc_id = devrecord.devlocid;
			loc_array[dev_count].loc_status = LOC_VALID;
		    }
		} /* end for */

		/*
		** NONEXT is the expected return from the above loop.
		** Make sure that if the table is multi-location that rows
		** in iidevices were found.
		*/
		if ((dberr->err_code == E_DM0055_NONEXT) && 
		    ((loc_count == 1) || (dev_count > 0)))
		{
		    CLRDBERR(dberr);
		    status = E_DB_OK;
		}

		if ( status )
		    break;

		status = dm2r_unfix_pages(dev_rcb, dberr);
		if (status != E_DB_OK )
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9048_DEV_UPDATE_ERROR);
		    break;
		}
	    }

	    /*
	    ** If this is a gateway table, then call the gateway to remove the
	    ** registration of it.
	    */
	    if (relrecord.relstat & TCB_GATEWAY)
	    {
		/*
		** Because the gateway code will do a DMT_SHOW operation on
		** the iirelation and iiattribute tables, we must close them
		** here (otherwise may get self-deadlock).
		*/

		local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
		if (local_status)
		{
		    *dberr = local_dberr;
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    status = local_status;
		    break;
		}
		rel_rcb = 0;

		local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
		if (local_status)
		{
		    *dberr = local_dberr;
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    status = local_status;
		    break;
		}
		attr_rcb = 0;

		status = dmf_gwt_remove(relrecord.relgwid,
		    scb->scb_gw_session_id, &relrecord.reltid,
		    &relrecord.relid, &relrecord.relowner,
                    dest_rcb, xcb, dberr);
		if (status)
		    break;
                /* 
                ** schang: delay the closing until remove so that  we can 
                ** have the oppertunity to close (RMS) file during remove
                ** Since the gwt_remove logic only close base table file,
                ** index removal should not call dm2t_close below.  ALSO
                ** gateway specific tcb is already deleted by gwt_remove,
                ** thus this dm2t_close should not worry if gateway specific
                ** tcb is not found.  We set DM2T_A_OPEN_NOACCESS flag to
                ** indicate that this close call is from dm2u_destroy.
                */
		if ( (relrecord.relstat & TCB_INDEX) == 0 )
                {
                    dest_rcb->rcb_access_mode = DM2T_A_OPEN_NOACCESS;
	            status = dm2t_close(dest_rcb, DM2T_PURGE, dberr);
		    if (status)
			break;
                }

		/*
		** Now reopen iirelation and iiattribute tables for further
		** processing.
		*/

		/* reltid, reltidx of iirelation */
		table_id.db_tab_base = DM_B_RELATION_TAB_ID;
		table_id.db_tab_index = DM_I_RELATION_TAB_ID;;

		/* open iirelation with IX lock */   
		status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
		    DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
		    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
		    db_lockmode, &xcb->xcb_tran_id, &timestamp, &rel_rcb,
		    (DML_SCB *)0, dberr);
		if (status)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		    break;
		}

		/*
		** Check for NOLOGGING - no updates should be written to the
		** log file if this session is running without logging.
		*/
		if (xcb->xcb_flags & XCB_NOLOGGING)
		    rel_rcb->rcb_logging = 0;

		/* determine if table to delete is journaled */
		rel_rcb->rcb_usertab_jnl = journal_flag ? 1 : 0;
		rel_rcb->rcb_xcb_ptr = xcb;

		/*  Position table using base table id. */
		table_id.db_tab_base = tbl_id->db_tab_base;
		table_id.db_tab_index = tbl_id->db_tab_index;
		key_desc[0].attr_operator = DM2R_EQ;
		key_desc[0].attr_number = 1;
		key_desc[0].attr_value = (char *) &table_id.db_tab_base;

		status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
				(DM_TID *)0, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		    break;
		}

		/* indicate iiattribute */
		table_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
		table_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

		/* open iiattribute with IX lock   */
		status = dm2t_open(dcb, &table_id, DM2T_IX, DM2T_UDIRECT,
		    DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
		    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
		    db_lockmode, &xcb->xcb_tran_id, &timestamp, &attr_rcb,
		   (DML_SCB *)0,  dberr);
		if (status)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9028_ATTR_UPDATE_ERR);
		    break;
		}

		/*
		** Check for NOLOGGING - no updates should be written to the
		** log file if this session is running without logging.
		*/
		if (xcb->xcb_flags & XCB_NOLOGGING)
		    attr_rcb->rcb_logging = 0;

		/* indicate if table to delete is journaled */
		attr_rcb->rcb_usertab_jnl = journal_flag ? 1 : 0;
		attr_rcb->rcb_xcb_ptr = xcb;

		/*
		** End iirelation, iiattribute reopens for gateway.
		*/
	    }

	    /*
	    ** If table is not a view or a table without an associated disk
	    ** file, then destroy the file.
	    ** Partitioned masters have no files, but do have locations.
	    ** Partitions may already be renamed if we're in a modify.
	    */
	    if (no_file
	      || (relrecord.relstat & TCB_IS_PARTITIONED)
	      || ((flag & DM2U_ONLINE_REPART) && 
			(relrecord.relstat2 & TCB2_PARTITION))
	      || (flag & DM2U_DROP_NOFILE) )
	    {
		/* Sometimes we have locations, but no files.  Avoid
		** memory leaks.
		*/
		if (loc_count > 0)
		    dm0m_deallocate((DM_OBJECT **)&loc_cb);
		continue;
	    }

	    /* Get the path information for location array
	    ** entries. */

	    for (k=0; k < loc_count; k++)	
	    {

		compare = MEcmp((PTR)&loc_array[k].loc_name,
		    (PTR)DB_DEFAULT_NAME, sizeof(DB_LOC_NAME));
		if (compare == 0)
		{
		   /* 
		    ** This is the default, so set the location ext 
		    ** to the root for this database.
		    */
		    loc_array[k].loc_ext = dcb->dcb_root;
		    loc_array[k].loc_config_id = 0;
		    continue;	
		}

		compare = -1;
		for (i = 0; i < dcb->dcb_ext->ext_count; i++)
		{
		    /* Bug B67276
		    ** Only look for usage of data
		    */
		    if ((dcb->dcb_ext->ext_entry[i].flags & DCB_DATA) == 0)
		        continue;

		    compare = MEcmp((char *)&loc_array[k].loc_name, 
                          (char *)&dcb->dcb_ext->ext_entry[i].logical, 
                          sizeof(DB_LOC_NAME));
		    if (compare == 0 )
		    {  
			loc_array[k].loc_ext = &dcb->dcb_ext->ext_entry[i];
			loc_array[k].loc_config_id = i;
			if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
			    loc_array[k].loc_status |= LOC_RAW;
			break;
		    }
		}

		if (compare != 0)
		    break;
	    }
	    if (compare != 0)
	    {
		SETDBERR(dberr, 0, E_DM001D_BAD_LOCATION_NAME);
		status = E_DB_ERROR;
		break;
	    }

	    /* Allocate the fcb for locations. */

	    status = dm2f_build_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
		loc_array, loc_count, 
		relrecord.relpgsize ? relrecord.relpgsize : DM_PG_SIZE,
		DM2F_ALLOC, DM2F_TAB, &relrecord.reltid, 
		0, 0, 0, 0, dberr);
	    if (status != E_DB_OK)
		break;

	    for (k = 0; k < loc_count; k++) /* for each file location */
	    {
		error = E_DM009D_BAD_TABLE_DESTROY;

		if ((loc_array[k].loc_status & LOC_RAW) == 0)
		{
		    /* 
		    ** Generate a filename, giving it the
		    ** ext dxx - where xx is the location code in hex 
		    */
		    dm2f_filename(DM2F_DES, &relrecord.reltid, 
                       loc_array[k].loc_id, 0, 0, &filename);

		    oldfilelength = sizeof(DM_FILE);
		    filelength = sizeof(DM_FILE);

		    /*
		    ** Log the file rename operation - unless logging is 
		    ** disabled.
		    **
		    ** The journal status of the frename log record is decided
		    ** based on the journal state of the database rather than
		    ** the table.  The physical operations associated with 
		    ** DROP's are rolled forward even on non-journaled tables.
		    */
		    if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
		    {
		        dm0l_flag = journal_flag ? DM0L_JOURNAL : 0;
		        if ((!journal_flag) && 
				(dcb->dcb_status & DCB_S_JOURNAL))
			    dm0l_flag = (DM0L_JOURNAL | DM0L_NONJNL_TAB);

		        status = dm0l_frename(xcb->xcb_log_id, dm0l_flag, 
			    &loc_array[k].loc_ext->physical,
			    loc_array[k].loc_ext->phys_length, 
			    &loc_array[k].loc_fcb->fcb_filename, 
			    &filename, &relrecord.reltid, 
			    k, loc_array[k].loc_config_id,
			    (LG_LSN *)0, &lsn, dberr);
		        if (status != E_DB_OK)
			    break;
		    }

#ifdef xDEBUG
	    TRdisplay("DESTROY rename %~t %~t \n",
	    oldfilelength, &loc_array[k].loc_fcb->fcb_filename, 
	    filelength, &filename);
#endif

		    /* Rename the file. */
		    status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id, 
		        &loc_array[k].loc_ext->physical, 
                        loc_array[k].loc_ext->phys_length, 
		        &loc_array[k].loc_fcb->fcb_filename,
                        oldfilelength, &filename, filelength, 
		        &dcb->dcb_name, dberr);
		    if (status != E_DB_OK)
			break;

		    /* Now pend this file for delete. */
		    status = dm0m_allocate((i4)sizeof(DML_XCCB), 
		        DM0M_ZERO, (i4)XCCB_CB,
		        (i4)XCCB_ASCII_ID, (char *)xcb, (DM_OBJECT **)&xccb, 
		        dberr);
		    if (status != E_DB_OK)
			break;
	    
		    xccb->xccb_operation = XCCB_F_DELETE;
		    xccb->xccb_f_len = filelength;
		    MEcopy((char *)&filename, filelength,
		       (char *)&xccb->xccb_f_name); 
		    xccb->xccb_p_len = loc_array[k].loc_ext->phys_length;
		    MEcopy((char *)&loc_array[k].loc_ext->physical, 
		       loc_array[k].loc_ext->phys_length, 
                       (char *)&xccb->xccb_p_name);
		    xccb->xccb_q_next = xcb->xcb_cq_next;
		    xccb->xccb_q_prev = (DML_XCCB*) &xcb->xcb_cq_next;
		    xcb->xcb_cq_next->xccb_q_prev = xccb;
		    xcb->xcb_cq_next = xccb;
		    xccb->xccb_xcb_ptr = xcb;
		    xccb->xccb_sp_id = xcb->xcb_sp_id;

		    /* Store table id for online modify */
		    STRUCT_ASSIGN_MACRO(relrecord.reltid, xccb->xccb_t_table_id);
		}
	    } /* end for for deleteing files */

	    if (loc_count)
	    {  
		/* free up the memory used for mulitple locations cb */
		local_status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
				loc_array, loc_count, DM2F_ALLOC, &local_dberr);
		if (local_status)
		{
		    if (dberr->err_code == 0)
			*dberr = local_dberr;
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		    status = local_status;
		    break;
		}
		dm0m_deallocate((DM_OBJECT **)&loc_cb);
	    }

	    /* if an error occurred during the loop to handle each location
	    ** during a file deletion, then the error must be trapped before
	    ** another ideration of the iirelation record loop. 
	    */
	    if (status != E_DB_OK)  /* teg */
		break;
	    else if (local_status != E_DB_OK)
	    {	
		SETDBERR(dberr, 0, local_error);
		break;
	    }
	    
	} /* end for for reading relation records. */ 

	if (dberr->err_code == E_DM0055_NONEXT && dest_count > 0)
	{
	    CLRDBERR(dberr);
	    status = E_DB_OK;
	}
	if ( status )
	    break;

	/* 
	** If the destroy operation is on an index rather than a base
	** table, then we must update the base table iirelation tuple
	** to reflect the deletion of one of its indexes.
	*/
	if (tbl_id->db_tab_index > 0)
	{
	    /*  Position table using base table id. */
	    table_id.db_tab_base = tbl_id->db_tab_base;
	    table_id.db_tab_index = 0;
	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_RELATION_KEY;
	    key_desc[0].attr_value = (char *) &table_id.db_tab_base;

	    status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			    NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			    &local_error, (i4)0);
		SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    for (;;)
	    {
		/* 
		** Get a qualifying relation record.  This will return
		** the base table row AND any secondary index rows.
		*/
		status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
                              (char *)&relrecord, dberr);
		if (status != E_DB_OK)
		{
		    if (dberr->err_code == E_DM0055_NONEXT)
			break;

		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				&local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		    break;
		}

		/*
		** Skip secondary index rows returned by dm2r_get.
		*/
		if ((relrecord.reltid.db_tab_base != table_id.db_tab_base) ||
		    (relrecord.reltid.db_tab_index != table_id.db_tab_index))
		    continue;

		/* 
                ** For the base table record, decrement the index count by one.
                ** If index count goes to zero, turn off the TCB_IDXD bit.
		** Also change the timestamp of the base table to show
		** that query plans should change.
                */
		relrecord.relidxcount -= 1;
    		if (relrecord.relidxcount == 0)
		    relrecord.relstat &= ~(TCB_IDXD);
		TMget_stamp((TM_STAMP *)&relrecord.relstamp12);

		status = dm2r_replace(rel_rcb, &tid, 
		    DM2R_BYPOSITION, (char*)&relrecord, (char *)0, dberr);
		if (status != E_DB_OK )
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				&local_error, (i4)0);
		    SETDBERR(&log_err, 0, E_DM9026_REL_UPDATE_ERR);
		    break;
		}
	    } /* End update of base table record. */
	}
	if (status != E_DB_OK && dberr->err_code != E_DM0055_NONEXT)
	    break;

	status = E_DB_OK;
	CLRDBERR(dberr);

	/*
	** Log the DMU operation.  This marks a spot in the log file to
	** which we can only execute rollback recovery once.
	*/
	if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	{
	    status = dm0l_dmu(xcb->xcb_log_id, journal_flag, tbl_id, 
		&table_name, &owner_name, (i4)DM0LDESTROY, 
		(LG_LSN *)0, dberr);
	    if (status != E_DB_OK)
		break;
	}

        break;	    	
    } /* end for (error control loop) */

    /* clean up by closing open files and freeing allocated memory.
    ** If gateway, some tables may be closed already.
    */
    if (rel_rcb != NULL)
    {
	local_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
	}
    }

    if (attr_rcb != NULL)
    {
	local_status = dm2t_close(attr_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
	}
    }

    if (index_rcb != NULL)
    {
	local_status = dm2t_close(index_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
	}
    }

    if (dev_rcb)
    {
	local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
	}
    }

    if (rtree_rcb)
    {
	local_status = dm2t_close(rtree_rcb, DM2T_NOPURGE, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    if ( status == E_DB_OK )
	    {
		*dberr = local_dberr;
		status = local_status;
	    }
	}
    }

    if (loc_cb != 0)
	dm0m_deallocate((DM_OBJECT **)&loc_cb);


    if ( status )
    {
	if (log_err.err_code && dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(&log_err, 0, NULL, ULE_LOG, NULL,
		    (char *) NULL, 0L, (i4 *)NULL, &local_error, 1,
		    sizeof(DB_DB_NAME), &dcb->dcb_name);
	}
	return (E_DB_ERROR);
    }
    return (E_DB_OK);

}
