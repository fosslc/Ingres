/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <di.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmve.h>
#include    <dm2f.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmpp.h>
#include    <dmse.h>
#include    <dm1b.h>
#include    <dm1p.h>
#include    <dm0m.h>
#include    <dmucb.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm0l.h>




/**
**
**  Name: DMVEMODIFY.C - The recovery of a modify table operation.
**
**  Description:
**	This file contains the routine for the recovery of a modify table
**	operation.
**
**          dmve_modify - The recovery of a modify table operation.
**
**
**  History:
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**      23-nov-87 (jennifer)
**          Support for multiple locations per tables added.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb calls().
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**      28-jul-89 (rogerk)
**          Initialize mx_reltups field as it is used in dm2u_load_table.
**	27-sep-89 (jrb)
**	    Initialized cmp_precision field for DECIMAL.
**	14-jun-1991 (Derek)
**	    Added support for allocation and extend options and new build
**	    routine interface.
**	25-sep-1991 (mikem) integrated following change: 12-apr-1991 (bryanp)
**	    Added support for Btree index compression.
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added new MODIFY record containing 'reltups' value.
**	25-sep-1991 (mikem) integrated following change: 30-aug-1991 (rogerk)
**	    Added handling for modify to reorganize.
**	17-oct-1991 (rmuth)
**	    Set the new fields mx_allocation and mx_extend in the MXCB from
**	    the DM0L_MODIFY log record.
**	14-apr-1992 (bryanp)
**	    Set mx_inmemory and mx_buildrcb fields in the MXCB.
**	7-july-92 (jnash)
**	    Add DMF function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	12-nov-1992 (jnash)
**	    Reduced Logging Project: Log CLR during UNDO, update
**	    loc_config_id.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location since it's no longer used.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Start of Reduced Logging DDL recovery fixes: remove Modify to Merge
**	    recovery processing from the dmve_modify routine.  This recovery
**	    is now done by logging directly the updates performed by the
**	    merge operation.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	  - Broke main body of function into dmv_remodify and dmv_unmodify.
**	  - Removed file creation code from dm2u_load and dm2u_reorg calls.
**	    We must now open the modify files prior to calling those routines.
**	  - Added statement count to modify and index log records.  We now
**	    use the stmt_cnt when initializing the modify files.
**	  - Added checks for gateway tables since gateway modify records are
**	    now journaled.
**	  - Removed support for OLDMODIFY record type.  Old log/journal records
**	    are desupported with 6.5.
**	  - Fix CL prototype mismatches.
**	18-oct-1993 (rogerk)
**	    Initialized mx_buckets field for dm2u_load_table call from the new
**	    dum_buckets log record value.  This makes sure that rollforward 
**	    recreates hash tables with the same structural layout as the 
**	    original modify.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	05-nov-93 (swm)
**	    Bug #58633
**	    Alignment offset calculations used a hard-wired value of 3
**	    to force alignment to 4 byte boundaries, for pointer and
**	    i4 alignment. Changed these calculations to use
**	    ME_ALIGN_MACRO.
**	31-jan-1994 (jnash)
**	  - Fix problem where next_dmp incremented prior to setting
**	    cmp_direction, resulting in (c/)heapsort recovery rebuilding  
**	    the table in the wrong order.
**	  - Fix problem certain keys added twice into key_map.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by user configuration,
**	    set db_sync_flag = DI_USER_SYNC_MASK when loading temporary 
**	    files used during the modify, force them prior to close.
**	21-apr-1994 (rogerk)
**	    Fix bug # 61138: rollforward of modify sorting rows incorrectly.
**	    Change code which sets key_map to avoid adding the same key twice
**	    into the comparison list to check the mx_atts_ptr array rather
**	    than the mx_data_atts array.  Otherwise we always map out the
**	    first kcount columns, regardless of whether they were keys.
**    15-apr-1994 (chiku)
**          Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass dum_page_size to dm0l_modify().
**      06-mar-1996 (stial01)
**          dmv_remodify() Pass dum_page_size to dm2f_open_file.
**          dmv_remodify() init mx_page_size
**          Pass page size to dm2f_build_fcb()
**      10-mar-1997 (stial01)
**          dmv_remodify: Alloc/init mx_crecord, to be used for compression.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	02-Oct-1997 (shero03)
**	    Added a log_id to the mxcb.
**      15-Mar-1999 (stial01)
**          dmv_remodify() Build DM2U_INDEX_CB instead of more complex MXCB.
**          Call dm2u_modify instead of dm2u_load_table.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Apr-2002 (hanal04) Bug 107549 INGSRV 1750
**          Adjust the key count in dmv_remodify() to prevent
**          E_DM001C in dm2u_modify() during rollforward of a modify
**          index to non-unique ISAM.
**	10-jan-2005 (thaju02)
**	    Recovery of online modify of partitioned table.
**	    For redo, open input table and partitions if any, to pass
**	    to dm2u_modify by omcb ptr.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions, dm2u_init_btree_read() 
**	    converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	13-Apr-2010 (kschendel) SIR 123485
**	    Open no-coupon to avoid unnecessary LOB overhead.
**/

static DB_STATUS dmv_remodify(
	DMVE_CB         *dmve,
	DM0L_MODIFY     *log_rec);


static DB_STATUS dmv_unmodify(
	DMVE_CB         *dmve,
	DM0L_MODIFY     *log_rec);


/*{
** Name: dmve_modify - The recovery of a modify table opration.
**
** Description:
**	This function re-execute the modify table opration using the
**	parameters stored in the log record. The system table changes,
**	the file creation and the file rename operations associated
**	with this modify table operation appear later in the journal
**	file, so these opeorations are not performed during recovery.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The modify table operation log record.
**	    .dmve_action	    Should only be DMVE_DO and DMVE_UNDO.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	    .dmve_db_lockmode	    The lockmode of the database. Should be 
**				    DM2T_X or DM2T_S.
**
** Outputs:
**	dmve_cb
**	    .dmve_error.err_code    The reason for error status.
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
**	30-jun-86 (ac)
**          Created for Jupiter.
**      23-nov-87 (jennifer)
**          Support for multiple locations per tables added.
**	27-jul-88 (sandyh)
**	    bugs 2993 & 3012 -- added initialization of m->mx_duplicates
**			to prevent memory trashing from causing improper
**			duplicates handling.
**	28-jul-88 (sandyh)
**	    bug 2995 -- m->mx_ab_count used in for loop of table attribute
**			building should be initialized at that time to pre-
**			vent index attribute calculations from failing.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb calls().
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**      28-jul-89 (rogerk)
**          Initialize mx_reltups field as it is used in dm2u_load_table.
**	25-sep-1991 (mikem) integrated following change: 12-apr-1991 (bryanp)
**	    Added support for Btree index compression.
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: when sysmod modifies the 'iirtemp' catalog, it fakes out
**	    the number of tuples so that it can cause the hash bucket
**	    calculation performed by dm2u_load_table to be 'reasonable'. That
**	    change omitted the logging of the mx_reltups value, with the
**	    result that the value was set to 0 during rollforward and the hash
**	    bucket calculation performed by the rollforward of a sysmod was
**	    different than the hash bucket calculation performed during normal
**	    calculation: rollforward of a sysmod failed. This change fixes that
**	    bug by logging the mx_reltups value so that we can ensure that
**	    we pass the same value to dm2u_load_table during rollforward as
**	    during normal processing. In order to achieve compatibility without
**	    rejecting old journal files as structurally invalid, I have
**	    introduced a new DM0L_MODIFY record type and renamed the old version
**	    to DM0L_OLD_MODIFY. Based on the log record type, this routine
**	    performs recovery using the appropriate structure definition.
**	25-sep-1991 (mikem) integrated following change: 30-aug-1991 (rogerk)
**	    Added handling for modify to reorganize.
**	    Previous to this change, this code did not handle a modify
**	    to reorganize type of action.  When we got one, we would proceed
**	    do a structural modify instead.
**	    We now allocate and format the old-location information in the
**	    modify control block using the TCB information (pre-modify state
**	    of the table).  After all the control blocks are initialized
**	    we call dm2u_reorg_table instead of dm2u_load_table for reorganize
**	    operations.
**	17-oct-1991 (rmuth)
**	    Set the new fields mx_allocation and mx_extend in the MXCB from
**	    the DM0L_MODIFY log record.
**	14-apr-1992 (bryanp)
**	    Set mx_inmemory and mx_buildrcb fields in the MXCB.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	12-nov-1992 (jnash)
**	    Reduced Logging Project: Log CLR during UNDO.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location since it's no longer used.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Start of Reduced Logging DDL recovery fixes: remove Modify to Merge
**	    recovery processing from the dmve_modify routine.  This recovery
**	    is now done by logging directly the updates performed by the
**	    merge operation.
**	23-aug-1993 (rogerk)
**	    Broke up into dmv_remodify and dmv_unmodify routines.
**	13-Jul-2007 (kibro01) b118695
**	    DM0L_MODIFY structure may be larger if the number of locations
**	    exceeds DM_MAX_LOC.
*/
DB_STATUS
dmve_modify(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_MODIFY		*log_rec = (DM0L_MODIFY *)dmve_cb->dmve_log_rec;
    i4		recovery_action;
    i4		error;
    DB_STATUS		status = E_DB_OK;
    i4		actual_size;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	if (log_rec->dum_header.type != DM0LMODIFY ||
	    dmve->dmve_action == DMVE_REDO)
	{
	    error = E_DM9601_DMVE_BAD_PARAMETER;
	    break;
	}

	actual_size = sizeof(DM0L_MODIFY);
	if (log_rec->dum_loc_count > DM_LOC_MAX)
	    actual_size +=
		(log_rec->dum_loc_count - DM_LOC_MAX) * sizeof(DB_LOC_NAME);

	if (log_rec->dum_header.length > actual_size)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.  Code within UNDO recognizes the CLR and does not
	** write another CLR log record.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->dum_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_remodify(dmve, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unmodify(dmve, log_rec);
	    break;
	}

	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    /*
    ** Error handling
    */

    if ((log_rec->dum_flag & DM0L_START_ONLINE_MODIFY) &&
	(status == E_DB_INFO) &&
	(dmve->dmve_error.err_code == E_DM9CB1_RNL_LSN_MISMATCH))
    {
	return(E_DB_INFO);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM960C_DMVE_MODIFY);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_remodify	- ROLLFORWARD Recovery of a table modify operation
**
** Description:
**
**	This routine is used to perform ROLLFORWARD recovery of the following
**	types of table modifies:
**
**	    Modify to structure     : Standard Modify
**	    Modify to reorganize    : Changes multi-location page striping
**				      layout.  No changes are made to the
**				      ingres pages themselves.
**	    Modify to truncate	    : Empty heap table created.
**
**	Unlike other page-oriented recovery operations, modify table 
**	recovery is entirely logical.
**
**	Standard Modify Operations:
**	---------------------------------------------------------------------
**	The table is opened and scanned via the appropriate access method
**	for its old table structure.
**
**	The rows extracted are fed into the sorter, the output of which
**	is fed into the access method build routines appropriate for its
**	new table structure (for heap modifies the sorter is bypassed).
**	---------------------------------------------------------------------
**
**	Reorganize Operations:
**	---------------------------------------------------------------------
**	The table is opened so that pages can be read from all of its
**	physical locations.
**
**	Pages are read sequentially from the old location array and written
**	in the appropriate striping layout to the new location list.
**	---------------------------------------------------------------------
**
**	Modify to Truncate:
**	---------------------------------------------------------------------
**	The heap build routines are called to create a new heap table.
**	No rows are fed into the build routines and an empty heap is created.
**	---------------------------------------------------------------------
**
**	Other Modify Operations:
**	---------------------------------------------------------------------
**	The following modify actions are not actually recovered through the 
**	MODIFY command, but are recovered by logging the actual updates made
**	by the update operation.  In these cases, the MODIFY record is written
**	for auditing purposes and is not actually used by recovery:
**
**	    Modify to Merge	    : Btree operation to shrink index
**	    Modify to Sysmod	    : Sysmod operation
**	---------------------------------------------------------------------
**
**
**	The new table table is built into temporary modify files.  These
**	files are assumed to already exist when this routine is called.
**
**	It is expected that these modify files were created through the
**	recovery of the FCREATE log records logged when they were originally
**	created.  It is further expected that these modify files will replace
**	the current physical table files that make up the old table structure
**	through the recovery of the FRENAME log records logged at the end
**	of the original modify operation.
**
** Inputs:
**	dmve			Recovery context block.
**	log_rec			Modify log record.
**
** Outputs:
**	err_code		The reason for error status.
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
**	23-aug-1993 (rogerk)
**	  - Routine created from dmve_modify code.
**	  - Removed file creation code from dm2u_load and dm2u_reorg calls.
**	    We must now open the modify files prior to calling those routines.
**	  - Added statement count to modify and index log records.  We now
**	    use the stmt_cnt when initializing the modify files.
**	  - Added checks for gateway tables since gateway modify records are
**	    now journaled.
**	  - Removed support for OLDMODIFY record type.  Old log/journal records
**	    are desupported with 6.5.
**	18-oct-1993 (rogerk)
**	    Initialized mx_buckets field for dm2u_load_table call from the new
**	    dum_buckets log record value.  This makes sure that rollforward 
**	    recreates hash tables with the same structural layout as the 
**	    original modify.
**	  - Added bypass of SYSMOD type modify operations in case we ever
**	    journal one of these log records.
**	31-jan-1994 (jnash)
**	  - Fix problem where next_dmp incremented prior to setting
**	    cmp_direction, resulting in (c/)heapsort recovery rebuilding  
**	    table in wrong order.
**	  - Fix problem certain keys added twice into key_map.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by user configuration,
**	    set db_sync_flag = DI_USER_SYNC_MASK when loading temporary 
**	    files used during the modify, force them prior to close.
**	21-apr-1994 (rogerk)
**	    Fix bug # 61138: rollforward of modify sorting rows incorrectly.
**	    Change code which sets key_map to avoid adding the same key twice
**	    into the comparison list to check the mx_atts_ptr array rather
**	    than the mx_data_atts array.  Otherwise we always map out the
**	    first kcount columns, regardless of whether they were keys.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
**      10-mar-1997 (stial01)
**          Alloc/init mx_crecord, area to be used for compression.
**	23-sep-1997 (shero03)
**	    B85561: add comp/expand overhead to mx_crecord size
**      15-Mar-1999 (stial01)
**          dmv_remodify() Build DM2U_INDEX_CB instead of more complex MXCB.
**          Call dm2u_modify instead of dm2u_load_table.
**      10-Apr-2002 (hanal04) Bug 107549 INGSRV 1750
**          Do not include tidp in the key count (kcount) for non-unique
**          index tables. dm2u_modify() will add the tidp back later.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not just != 0.
**	6-Feb-2004 (schka24)
**	    Pass along all name generator info to modify.
**	28-Feb-2004 (jenjo02)
**	    Add three new parms to dm2u_modify() for partitions.
**	12-Mar-2004 (schka24)
**	    and one more for dm1u flag cleanup.
**	5-Apr-2004 (schka24)
**	    partition and ppchar logged as rawdata, pass to modify.
**	    Open master instead of partition for attribute info.
**	30-jun-2004 (thaju02)
**	    Add dum_bsf_lsn param to dm0l_modify().
**	    For rolldb, removed close of rnl table. Table to be closed
**	    when we encounter the modify (DM0L_ONLINE) log record.
**	22-Nov-2004 (thaju02)
**	    Align ppchar. (B113515)
**	10-jan-2005 (thaju02)
**	    Recovery of online modify of partitioned table.
**	    For redo, open input table and partitions if any, to pass
**	    to dm2u_modify by omcb ptr.
**	11-feb-2005 (thaju02)
**	    If osrc_created, init rcb_rnl_online to 0.
**	13-Apr-2006 (jenjo02)
**	    Replace stupid dm2u_modify parm list with DM2U_MOD_CB
**	    structure, deal with TCB_CLUSTERED.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	23-Oct-2009 (kschendel) SIR 122739
**	    Do hidata compression modifies properly.
**	9-Jul-2010 (kschendel) SIR 123450
**	    Ditto for new-standard compression.
*/
static DB_STATUS
dmv_remodify(
DMVE_CB         *dmve,
DM0L_MODIFY 	*log_rec)
{
    DMP_RCB		*rcb = 0;
    DMP_TCB		*t;
    DB_TAB_TIMESTAMP	timestamp;
    i4		i;
    i4		kcount = 0;
    i4		local_error;
    DB_STATUS		status = E_DB_OK;
    i4                  merge = 0;
    DM2U_KEY_ENTRY       *keyentry;
    DM2U_KEY_ENTRY       *curentry;
    DM2U_KEY_ENTRY       **keyptrs;
    DB_OWN_NAME         table_owner;
    DB_TAB_NAME         table_name;
    DB_TAB_ID		master_id;
    i4                  modoptions = 0;
    i4                  mod_options2 = 0;
    i4                  has_extensions = 0;
    i4                  structure;
    i4                  tup_cnt = 0;
    DMP_MISC            *misc = 0;
    DM2U_RFP_ENTRY      rfp_context;
    DB_PART_DEF		*part_def;
    DMU_PHYPART_CHAR	*ppchar;
    PTR			pp_rawdata;
    i4			pp_entries;
    i4			flagsmask = 0;

    DMP_RCB		*rnl_rcb = 0;
    DMP_TCB		*rnl_tcb = 0;
    DM2U_OMCB		omcb;
    RFP_OCTX		*octx = dmve->dmve_octx;
    DM2U_OSRC_CB	*osrc, *osrc_entry;
    DMP_MISC		*osrc_misc_cb = 0;
    DB_TAB_ID		*tabid;
    DMP_TCB		*osrc_tcb;
    i4			online_sources;
    bool		online_modify = FALSE;
    DM2U_MOD_CB		local_mcb, *mcb = &local_mcb;
    i4			*err_code = &dmve->dmve_error.err_code;
    DB_ERROR		local_dberr;
    DB_ATTS		*a;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Modify to merge is not recovered through MODIFY, but by applying
	** actual updates logged by the btree merge code.
	**
	** Sysmod writes log a SYSMOD modify log record, but the operation
	** is recovered through the other log records it writes, including:
	** a create and modify of the iirtemp table and special sysmod log
	** records.
	**
	** Modify of gateway tables has no rollforward actions here. The system
	** catalog updates will be rolled forward but no action is taken on
	** the underlying gateway table.
	*/
	if ((log_rec->dum_structure == DM0L_MERGE) ||
	    (log_rec->dum_structure == DM0L_SYSMOD) ||
	    (log_rec->dum_status & TCB_GATEWAY))
	{
	    return (E_DB_OK);
	}

	/*
	** Open the table to get some attribute info that wasn't logged
	** as such.  All we need is the number of columns and some of
	** their names.  Use the master (if modifying a partition),
	** as the master has identical info, and is easier to open.
	*/
	STRUCT_ASSIGN_MACRO(log_rec->dum_tbl_id, master_id);
	if (master_id.db_tab_index < 0)
	    master_id.db_tab_index = 0;
	
	status = dm2t_open(dmve->dmve_dcb_ptr, &master_id,
			DM2T_X, DM2T_UDIRECT, DM2T_A_WRITE_NOCPN,
			(i4)0, (i4)20, (i4)0, 
			dmve->dmve_log_id, dmve->dmve_lk_id,
			(i4)0, (i4)0, dmve->dmve_db_lockmode,
			&dmve->dmve_tran_id, &timestamp, &rcb, (DML_SCB *)NULL,
			&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	t = rcb->rcb_tcb_ptr;

	for (i = 0; i < DB_MAX_COLS; i++)
	{
	    if (log_rec->dum_key[i] != 0)
		kcount++;
	    else
		break;
	}

	/* dm2u_modify() will add tidp back to the key do not include
	** it in the key count here.
	*/
	if(log_rec->dum_tbl_id.db_tab_index > 0 && 
	     ((log_rec->dum_status & TCB_UNIQUE) == 0) && 
	     (kcount == t->tcb_rel.relatts))
	{
	    kcount--;
	}

	/*
	** Allocate memory for MISC_CB, 
	** and DM2U_KEY_ENTRY pointers and entries
	*/
	status = dm0m_allocate(sizeof(DMP_MISC) +
	    kcount * sizeof (DM2U_KEY_ENTRY *) + 
	    kcount * sizeof(DM2U_KEY_ENTRY),
	    (i4)DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	    (char *)NULL, (DM_OBJECT **)&misc, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;
	misc->misc_data = (char*)misc + sizeof(DMP_MISC);

	/* We have attribute numbers, allocate & init DM2U_KEY_ENTRYs */
	if (log_rec->dum_structure == DM0L_TRUNCATE)
	{
	    keyptrs = (DM2U_KEY_ENTRY **)NULL;
	    keyentry = (DM2U_KEY_ENTRY *)NULL;
	}
	else
	{
	    keyptrs = (DM2U_KEY_ENTRY **)(misc + 1);
	    keyentry = (DM2U_KEY_ENTRY *)(keyptrs + kcount);

	    for (i = 0; i < kcount; i++)
	    {
		keyptrs[i] = curentry = keyentry++;
		a = &t->tcb_atts_ptr[log_rec->dum_key[i]];
		MEmove(a->attnmlen, a->attnmstr, ' ',
		    DB_ATT_MAXNAME, curentry->key_attr_name.db_att_name);
		if (log_rec->dum_order[i])
		    curentry->key_order = DMU_DESCENDING;
		else
		    curentry->key_order = DMU_ASCENDING;
	    }
	}

	/* Close the table */
	status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	rcb = (DMP_RCB *)NULL;
	t = (DMP_TCB *)NULL;

	structure = log_rec->dum_structure;
	if (log_rec->dum_structure == DM0L_TRUNCATE)
	{
	    modoptions |= DM2U_TRUNCATE;
	    structure = TCB_HEAP;
	}
	else if (log_rec->dum_structure == DM0L_REORG)
	{
	    modoptions |= DM2U_REORG;
	}

	/*
	** From rollforward, (dcb->dcb_status & DCB_S_ROLLFORWARD) != 0
	** If dm2u_modify is called from rollforward:
	**   - Skips dm2u_file_create (file already exists)
	**   - Makes sure to generate the same temp filenames(s) as last time
	**   - Skips catalog updates (dm2u_update_catalogs)
	**   - Skips logging
	**   - Has no XCB or SCB, so we must pass log_id, lk_id, tran_id...
	*/
	rfp_context.rfp_log_id = dmve->dmve_log_id;
	rfp_context.rfp_lk_id = dmve->dmve_lk_id;
	STRUCT_ASSIGN_MACRO(dmve->dmve_tran_id, rfp_context.rfp_tran_id);
	/* FIX ME case should be set from jsx_reg_case */
	rfp_context.rfp_upcase = FALSE;
	rfp_context.rfp_name_id = log_rec->dum_name_id;
	rfp_context.rfp_name_gen = log_rec->dum_name_gen;
	rfp_context.rfp_hash_buckets = log_rec->dum_buckets;

	/* If there was a partition definition, re-pointerize it */
	part_def = (DB_PART_DEF *) dmve->dmve_rawdata[DM_RAWD_PARTDEF];
	if (part_def != NULL)
	    dm2u_repointerize_partdef(part_def);

	/* If there was a ppchar, re-pointerize and get the entry count */
	pp_rawdata = dmve->dmve_rawdata[DM_RAWD_PPCHAR];
	pp_entries = 0;
	ppchar = NULL;
	if (pp_rawdata != NULL)
	{
	    dm2u_repointerize_ppchar(pp_rawdata);
	    /* Entry count at the front, then the ppchar proper */
	    pp_entries = *(i4 *) pp_rawdata;
	    ppchar = (DMU_PHYPART_CHAR *) (pp_rawdata + sizeof(i4));
	    ppchar = (DMU_PHYPART_CHAR *)ME_ALIGN_MACRO((PTR)ppchar, sizeof(ALIGN_RESTRICT));
	}

	/* for online modify, open modify table and setup omcb */
	if ((dmve->dmve_dcb_ptr->dcb_status & DCB_S_ROLLFORWARD) &&
	    (log_rec->dum_flag & DM0L_START_ONLINE_MODIFY) &&
	    (log_rec->dum_name.db_tab_name[0] == '$') && octx)
	{
	    bool	osrc_created = FALSE;
	    MEfill(sizeof(DM2U_OMCB), 0, &omcb);

	    omcb.omcb_octx = (PTR)octx;

	    status = dm2t_open(dmve->dmve_dcb_ptr, &log_rec->dum_rnl_tbl_id, 
		    DM2T_N, DM2T_UDIRECT, DM2T_A_READ_NOCPN,
		    (i4)0, (i4)20, (i4)0, dmve->dmve_log_id, dmve->dmve_lk_id,
		    (i4)0, (i4)0, dmve->dmve_db_lockmode, &dmve->dmve_tran_id, 
		    &timestamp, &rnl_rcb, (DML_SCB *)NULL, &dmve->dmve_error);
	    if (status)
		break;

	    rnl_tcb = rnl_rcb->rcb_tcb_ptr;

	    if ((online_sources = rnl_tcb->tcb_rel.relnparts) == 0)
		online_sources = 1;   /* no partitions */
	
	    for ( i = 0; i < online_sources; i++)
	    {
		osrc = (DM2U_OSRC_CB *)NULL;

		if (online_sources == 1)
		    tabid = &rnl_tcb->tcb_rel.reltid;
		else
		    tabid = &rnl_tcb->tcb_pp_array[i].pp_tabid;

		/* find osrc block for this table */
		for ( osrc_entry = (DM2U_OSRC_CB *)octx->octx_osrc_next;
		      osrc_entry;
		      osrc_entry = osrc_entry->osrc_next)
		{
		    if ( !MEcmp(&osrc_entry->osrc_tabid, tabid,
				sizeof(DB_TAB_ID)) )
		    {
			osrc = osrc_entry;
			break;
		    }
		}

		if (osrc == (DM2U_OSRC_CB *)NULL)
		{
		    /* allocate block. rfp has no notion as to table's 
		    ** number of partitions.
		    ** rfp allocated only osrc blocks for those partitions 
		    ** that have rnl lsn log records. 
		    */
		    status = dm0m_allocate(sizeof(DMP_MISC) + 
				sizeof(DM2U_OSRC_CB), DM0M_ZERO,
                                MISC_CB, MISC_ASCII_ID, (char *)NULL, 
				(DM_OBJECT **)&osrc_misc_cb, &dmve->dmve_error);
		    if (status)
			break;
		
		    osrc = (DM2U_OSRC_CB *)(osrc_misc_cb + 1);
		    osrc_misc_cb->misc_data = (char*)osrc;
		    osrc->osrc_misc_cb = osrc_misc_cb;
		    STRUCT_ASSIGN_MACRO(*tabid, osrc->osrc_tabid);

		    /* put osrc block on list */
		    osrc->osrc_next = (DM2U_OSRC_CB *)octx->octx_osrc_next;
		    octx->octx_osrc_next = (PTR)osrc;
		    osrc_created = TRUE;
		}

		if (rnl_tcb->tcb_rel.relnparts)
		{
		    status = dm2t_open(dmve->dmve_dcb_ptr, tabid, DM2T_N, 
				DM2T_UDIRECT, DM2T_A_READ_NOCPN, (i4)0, (i4)20,
				(i4)0, dmve->dmve_log_id, dmve->dmve_lk_id,
				(i4)0, (i4)0, dmve->dmve_db_lockmode, 
				&dmve->dmve_tran_id, &timestamp, 
				&osrc->osrc_rnl_rcb, (DML_SCB *)NULL, &dmve->dmve_error);
		    if (status)
			break;
		}
		else
		    osrc->osrc_rnl_rcb = rnl_rcb;

		osrc->osrc_master_rnl_rcb = rnl_rcb;
		osrc->osrc_partno = i;
		if (osrc_created)
		    osrc->osrc_rnl_rcb->rcb_rnl_online = (DMP_RNL_ONLINE *)NULL;
		else
		    osrc->osrc_rnl_rcb->rcb_rnl_online = 
					&osrc->osrc_rnl_online;
		osrc_tcb = osrc->osrc_rnl_rcb->rcb_tcb_ptr;
		if (osrc_tcb->tcb_rel.relspec == TCB_BTREE)
		{
		    DM_PAGENO	lastused_pageno;

		    if (status = dm1p_lastused(osrc->osrc_rnl_rcb,
				(u_i4)DM1P_LASTUSED_DATA,
                                &lastused_pageno, (DMP_PINFO*)NULL, 
				&dmve->dmve_error))
		    {
			break;
		    }

                    if (status = dm2u_init_btree_read(osrc_tcb, 
				&osrc->osrc_rnl_online, lastused_pageno, 
				&dmve->dmve_error))
		    {
			break;
		    }
		}
	    }
	    if (status)
		break;

	    omcb.omcb_osrc_next = (DM2U_OSRC_CB *)octx->octx_osrc_next;
	    online_modify = TRUE;
	}

	/* Prepare the MCB for dm2u_modify: */
	mcb->mcb_dcb = dmve->dmve_dcb_ptr;
	mcb->mcb_xcb = (DML_XCB*)NULL;
	mcb->mcb_tbl_id = &log_rec->dum_tbl_id;
	mcb->mcb_omcb = (online_modify) ? &omcb : (DM2U_OMCB*)NULL;
	mcb->mcb_dmu = (DMU_CB*)NULL;
	mcb->mcb_structure = structure;
	mcb->mcb_location = log_rec->dum_location;
	mcb->mcb_l_count = log_rec->dum_loc_count;
	mcb->mcb_i_fill = log_rec->dum_ifill;
	mcb->mcb_l_fill = log_rec->dum_lfill;
	mcb->mcb_d_fill = log_rec->dum_dfill;
	mcb->mcb_compressed = TCB_C_NONE;
	if (log_rec->dum_status & TCB_COMPRESSED)
	    mcb->mcb_compressed = TCB_C_STD_OLD;
	if (log_rec->dum_flag & DM0L_MODIFY_NEWCOMPRESS)
	    mcb->mcb_compressed = TCB_C_STANDARD;
	if (log_rec->dum_flag & DM0L_MODIFY_HICOMPRESS)
	    mcb->mcb_compressed = TCB_C_HICOMPRESS;
	mcb->mcb_index_compressed = (log_rec->dum_status & TCB_INDEX_COMP) ? TRUE : FALSE;
	mcb->mcb_temporary = FALSE;
	mcb->mcb_unique = (log_rec->dum_status & TCB_UNIQUE) ? TRUE : FALSE;
	mcb->mcb_merge = FALSE;
	mcb->mcb_clustered = (log_rec->dum_status & TCB_CLUSTERED) ? TRUE : FALSE;
	mcb->mcb_min_pages = log_rec->dum_min;
	mcb->mcb_max_pages = log_rec->dum_max;
	mcb->mcb_modoptions = modoptions;
	mcb->mcb_mod_options2 = mod_options2;
	mcb->mcb_kcount = kcount;
	mcb->mcb_key = keyptrs;
	mcb->mcb_db_lockmode = DM2T_X;
	mcb->mcb_allocation = log_rec->dum_allocation;
	mcb->mcb_extend = log_rec->dum_extend;
	mcb->mcb_page_type = log_rec->dum_pg_type;
	mcb->mcb_page_size = log_rec->dum_page_size;
	mcb->mcb_tup_info = &tup_cnt;
	mcb->mcb_reltups = log_rec->dum_reltups;
	mcb->mcb_tab_name = &table_name;
	mcb->mcb_tab_owner = &table_owner;
	mcb->mcb_has_extensions = &has_extensions;
	mcb->mcb_relstat2 = 0;
	mcb->mcb_flagsmask = flagsmask;
	mcb->mcb_tbl_pri = 0;
	mcb->mcb_rfp_entry = &rfp_context;
	mcb->mcb_new_part_def = part_def;
	mcb->mcb_new_partdef_size = dmve->dmve_rawdata_size[DM_RAWD_PARTDEF];
	mcb->mcb_partitions = ppchar;
	mcb->mcb_nparts = pp_entries;
	mcb->mcb_verify = 0;

        /* Call the physical layer to process the rest of the modify */
	status = dm2u_modify(mcb, &dmve->dmve_error);

	if (status != E_DB_OK)
	    break;

	dm0m_deallocate((DM_OBJECT **)&misc);
	return(E_DB_OK);	
    }

    /*
    ** Error Cleanup.
    */
    if (rcb)
    {
	status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (misc)
	dm0m_deallocate((DM_OBJECT **)&misc);

    /* rolldb of online modify */
    if ((log_rec->dum_flag & DM0L_START_ONLINE_MODIFY) &&
	(status == E_DB_INFO) &&
	(dmve->dmve_error.err_code == E_DM9CB1_RNL_LSN_MISMATCH))
    {
	return(E_DB_INFO);
    }

    if (octx)
    {
	/* close partitions */
	for ( osrc = (DM2U_OSRC_CB *)octx->octx_osrc_next; 
	      osrc; osrc = osrc->osrc_next )
	{
	    if (osrc->osrc_rnl_rcb)
	    {
		status = dm2t_close(osrc->osrc_rnl_rcb, DM2T_NOPURGE, 
				&local_dberr);
		if (status)
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
				&local_error, 0);
	    }
	}

	/* close master */
	if (rnl_tcb->tcb_rel.relnparts)
	{
	    status = dm2t_close(rnl_rcb, DM2T_NOPURGE, 
			&local_dberr);
	    if (status)
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_error, 0);
	}
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9682_REDO_MODIFY);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_unmodify	- UNDO Recovery of a table modify operation
**
** Description:
**
**	This routine is used during UNDO recovery of a modify of a table.  
**
**	The vast majority of the modify operation is rolled back by undoing
**	the system catalog updates and File Rename operations via recovery
**	of their respective log records.
**
**	This routine purges the table TCB (and notifies other servers of
**	the structure change by bumping the cache lock).
**
** Inputs:
**	dmve			Recovery context block.
**	log_rec			Modify log record.
**
** Outputs:
**	err_code		The reason for error status.
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
**	23-aug-1993 (rogerk)
**	  - Routine created from dmve_modify code.
**	  - Removed support for OLDMODIFY record type.  Old log/journal records
**	    are desupported with 6.5.
**	  - Added a statement count field to the modify and index log records.
**	18-oct-1993 (rogerk)
**	    Added bucket count to modify log record.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**      15-apr-1994 (chiku)
**          Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass dum_page_size to dm0l_modify().
**	17-may-2004 (thaju02)
**	    Removed unnecessary rnl_name/dum_rnl_name from dm0l_modify().
*/
static DB_STATUS
dmv_unmodify(
DMVE_CB         *dmve,
DM0L_MODIFY 	*log_rec)
{
    LG_LSN		*log_lsn = &log_rec->dum_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Modify was aborted.  Purge the table descriptor in case
	** someone (presumably this transaction) has done a build_tcb 
	** on the table in its modified state.
	*/
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &log_rec->dum_tbl_id,
			(DM2T_PURGE | DM2T_NORELUPDAT), dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	     ((log_rec->dum_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->dum_header.flags | DM0L_CLR);

	    status = dm0l_modify(dmve->dmve_log_id, dm0l_flags, 
			&log_rec->dum_tbl_id, 
			&log_rec->dum_name, &log_rec->dum_owner, 
			log_rec->dum_loc_count, log_rec->dum_location, 
			log_rec->dum_structure, log_rec->dum_status, 
			log_rec->dum_min, log_rec->dum_max, 
			log_rec->dum_ifill, log_rec->dum_dfill, 
			log_rec->dum_lfill, log_rec->dum_reltups, 
			log_rec->dum_buckets,
			&log_rec->dum_key[0], &log_rec->dum_order[0], 
			log_rec->dum_allocation, log_rec->dum_extend, 
			log_rec->dum_pg_type,
			log_rec->dum_page_size,
			log_rec->dum_flag,
			&log_rec->dum_rnl_tbl_id,
			log_rec->dum_bsf_lsn,
			log_rec->dum_name_id,
			log_rec->dum_name_gen, log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK) 
	    {
                /*
                 * Bug56702: return logfull indication.
                 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }

	    /*
	    ** Release log file space allocated for the logforce done
	    ** in the purge tcb call above.
	    */
	    dmve_unreserve_space(dmve, 1);
	}

	return(E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9683_UNDO_MODIFY);

    return(E_DB_ERROR);
}
