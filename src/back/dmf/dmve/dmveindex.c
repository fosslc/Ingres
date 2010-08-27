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
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmve.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmpp.h>
#include    <dmse.h>
#include    <dm0m.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1p.h>
#include    <dmucb.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm0l.h>
#include    <dm2f.h>




/**
**
**  Name: DMVEINDEX.C - The recovery of a index table operation.
**
**  Description:
**	This file contains the routine for the recovery of a index table
**	operation.
**
**          dmve_index - The recovery of a index table operation.
**
**
**  History:
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**      23-nov-87 (jennifer)
**          Support for multiple locations per tables added.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**      28-jul-89 (rogerk)
**          Initialize mx_reltups field as it is used in dm2u_load_table.
**	14-jun-1991 (Derek)
**	    Added support for allocation and extend options and new build
**	    routine interface.
**	25-sep-1991 (mikem) integrated following change: 12-apr-1991 (bryanp)
**	    Added support for Btree Index Compression.
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Pass mx_reltups value from log record to dm2u_load_table.
**	17-oct-1991 (rmuth)
**	    Set the new fields mx_extend and mx_allocation in the MXCB from
**	    the DM0L_INDEX log record.
**	14-apr-1992 (bryanp)
**	    Set mx_inmemory and mx_buildrcb fields in the MXCB.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	30-nov-1992 (jnash)
**	    Reduced logging project.  
**	      - Fill in new config_loc_id when initializing data in 
**		location array.  
**	      - Add error checking after dm2t_open to make a half-hearted 
**		attempt at supporting partial recovery.  
**	      - Because this routine does nothing for UNDO, we do not 
**		bother logging a CLR.  We may desire to change this
**		for consistency with other recovery routines.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location since it's no longer used.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Include <me.h> since we use ME functions in this file.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Fixed setting of extend and allocation fields passed to the build
**	    code.  We were incorrectly using the dui_extend value as the
**	    allocation amount and the dui_allocation value as the extend amount.
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	  - Broke main body of function into dmv_reindex and dmv_unindex.
**	  - Removed file creation code from dm2u_load_table call.
**	    We must now open the load files prior to calling this routine.
**	  - Added statement count to modify and index log records.  We now
**	    use the stmt_cnt when initializing the load files.
**	  - Added checks for gateway tables since gateway index records are
**	    now journaled.
**	  - Removed support for OLDINDEX record type.  Old log/journal records
**	    are desupported with 6.5.
**	  - Fix CL prototype mismatches.
**	23-aug-1993 (rmuth)
**	    Always open the base table in DM2T_A_READ mode.
**	20-sep-1993 (rogerk)
**	    Fix problems with rollforward of hash structured indexes.  The
**	    TIDP column must be included in the column list for hash tables
**	    as well as other structures.
**	18-oct-1993 (rogerk)
**	    Initialized mx_buckets field for dm2u_load_table call from the new
**	    dui_buckets log record value.  This makes sure that rollforward 
**	    recreates hash tables with the same structural layout as the 
**	    original index operation.
**	05-nov-93 (swm)
**	    Bug #58633
**	    Alignment offset calculations used a hard-wired value of 3
**	    to force alignment to 4 byte boundaries, for both pointer and
**	    i4 alignment. Changed these calculations to use
**	    ME_ALIGN_MACRO.
**	28-mar-1994 (rogerk)
**	    Fix bug from 31-jan changes where we miscalculate the amount of
**	    memory to allocate for the tuple buffers in the MXCB allocation.
**	    We were missing one i4 per buffer causing us to overwrite
**	    memory past where we allocated (bug #59947).
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by user configuration,
**	    set db_sync_flag = DI_USER_SYNC_MASK when loading temporary 
**	    files used when building the index, force them prior to close.
**      15-apr-1994 (chiku)
**          Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass dui_page_size to the dm0l_index routine.
**      06-mar-1996 (stial01)
**          dmv_reindex(): Pass dui_page_size to dm2f_open_file().
**          dmv_reindex() init mx_page_size
**          dmve_reindex() Pass page size to dm2f_build_fcb()
**	23-may-1996 (shero03)
**	    RTree: added dimension, hilbertsize, and range
**      10-mar-1997 (stial01)
**          dmv_reindex: Alloc/init mx_crecord, area to be used for compression.
**	15-apr-1997 (shero03)
**	    Remove dimension from getklv
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	01-Oct-1997 (shero03)
**	    B85456: use the mx_log_id in lieu of the xcb_log_id.
**	04-mar-1999 (nanpr01)
**	    Fix up recovery problem for hash secondary index.
**      15-Mar-1999 (stial01)
**          dmv_reindex() Build DM2U_INDEX_CB instead of more complex MXCB.
**          Call dm2u_index instead of dm2u_load_table.
**          NOTE this change removes the need for 04-Mar-1999 changes by
**          nanpr01 and shero03
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	23-Oct-2009 (kschendel) SIR 122739
**	    Log record now has actual compression type in it, fix here.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Fix outrageous name confusion re acount vs kcount.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	16-Jul-2010 (kschendel) SIR 123450
**	    Log record now has actual compression types in it.
**/

static DB_STATUS dmv_reindex(
	DMVE_CB         *dmve,
	DM0L_INDEX      *log_rec);


static DB_STATUS dmv_unindex(
	DMVE_CB         *dmve,
	DM0L_INDEX      *log_rec);


/*{
** Name: dmve_index - The recovery of a index table opration.
**
** Description:
**	This function re-execute the index table opration using the
**	parameters stored in the log record. The system table changes,
**	the file creation and the file rename operations associated
**	with this index table operation appear later in the journal
**	file, so these opeorations are not performed during recovery.
**	This routine has undo code to purge the table descriptors.
**	In undo case, before purge tcb, invalidate pages in local cache.
**	Also, call dm2t_close with NORELUPDAT flag.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The index table operation log record.
**	    .dmve_action	    Should only be DMVE_DO.
**	    .dmve_lg_addr	    The log address of the log record.
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
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**      28-jul-89 (rogerk)
**          Initialize mx_reltups field as it is used in dm2u_load_table.
**	27-sep-89 (jrb)
**	    Initialized cmp_precision field for DECIMAL.
**	25-sep-1991 (mikem) integrated following change: 12-apr-1991 (bryanp)
**	    Added support for Btree index compression.
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Pass mx_reltups value from log record to dm2u_load_table.
**	    See dmve_modify() for a detailed explanation of what this value
**	    is used for and why there are two different INDEX log records.
**	17-oct-1991 (rmuth)
**	    Set the new fields mx_extend and mx_allocation in the MXCB from
**	    the DM0L_INDEX log record.
**	14-apr-1992 (bryanp)
**	    Set mx_inmemory and mx_buildrcb fields in the MXCB.
**	16-jul-1992 (kwatts)
**	    MPF change. Use accessor to calculate possible expansion on
**	    compression (comp_katts_count).
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	30-nov-1992 (jnash)
**	    Reduced logging project.  
**	      - Fill in new config_loc_id when initializing data in 
**		location array.  
**	      - Add error checking after dm2t_open to make a half-hearted 
**		attempt at supporting partial recovery.  
**	      - Because this routine does nothing for UNDO, we do not 
**		bother logging a CLR.  We may desire to change this
**		in the future for consistency with other recovery routines.
**	14-dec-92 (jrb)
**	    Removed initialization of mx_s_location since it's no longer used.
**	26-jul-1993 (rogerk)
**	    Fixed setting of extend and allocation fields passed to the build
**	    code.  We were incorrectly using the dui_extend value as the
**	    allocation amount and the dui_allocation value as the extend amount.
**	23-aug-1993 (rogerk)
**	    Broke up into dmv_reindex and dmv_unindex routines.
*/
DB_STATUS
dmve_index(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_INDEX		*log_rec = (DM0L_INDEX *)dmve_cb->dmve_log_rec;
    i4		recovery_action;
    i4		error;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	if (log_rec->dui_header.length > sizeof(DM0L_INDEX) || 
	    log_rec->dui_header.type != DM0LINDEX ||
	    dmve->dmve_action == DMVE_REDO)
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
	if (log_rec->dui_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_reindex(dmve, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unindex(dmve, log_rec);
	    break;
	}

	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    /*
    ** Error handling
    */

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM960B_DMVE_INDEX);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_reindex	- ROLLFORWARD Recovery of a table index operation
**
** Description:
**
**	This routine is used to perform ROLLFORWARD recovery of a build
**	index operation.
**
**	Unlike other page-oriented recovery operations, create index table 
**	recovery is entirely logical.
**
**	The base table is opened and scanned via the appropriate access method
**	for its table structure.
**
**	The rows extracted are fed into the sorter, the output of which
**	is fed into the access method build routines appropriate for its
**	new index table structure.
**
**	The new table table is built into temporary load files.  These
**	files are assumed to already exist when this routine is called.
**
**	It is expected that these load files were created through the
**	recovery of the FCREATE log records logged when they were originally
**	created.  It is further expected that these load files will replace
**	the current physical table files that make up the index table
**	through the recovery of the FRENAME log records logged at the end
**	of the original index operation.
**
** Inputs:
**	dmve			Recovery context block.
**	log_rec			Index log record.
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
**	  - Routine created from dmve_index code.
**	  - Removed file creation code from dm2u_load_table call.
**	    We must now open the load files prior to calling this routine.
**	  - Added statement count to modify and index log records.  We now
**	    use the stmt_cnt when initializing the load files.
**	  - Added checks for gateway tables since gateway index records are
**	    now journaled.
**	  - Removed support for OLDINDEX record type.  Old log/journal records
**	    are desupported with 6.5.
**	23-aug-1993 (rmuth)
**	    Always open the base table in DM2T_A_READ mode.
**	20-sep-1993 (rogerk)
**	    Fix problems with rollforward of hash structured indexes.  The
**	    TIDP column must be included in the column list for hash tables
**	    as well as other structures.  Without this, the compression code
**	    blows up.
**	18-oct-1993 (rogerk)
**	    Initialized mx_buckets field for dm2u_load_table call from the new
**	    dui_buckets log record value.  This makes sure that rollforward 
**	    recreates hash tables with the same structural layout as the 
**	    original index operation.
**	28-mar-1994 (rogerk)
**	    Fix bug from 31-jan changes where we miscalculate the amount of
**	    memory to allocate for the tuple buffers in the MXCB allocation.
**	    We were missing one i4 per buffer causing us to overwrite
**	    memory past where we allocated (bug #59947).
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by user configuration,
**	    set db_sync_flag = DI_USER_SYNC_MASK when loading temporary 
**	    files used when building the index, force them prior to close.
**	06-mar-1996 (stial01)
**	    Pass page size to dm2f_build_fcb()
**	23-may-1996 (shero03)
**	    RTree: added dimension, hilbertsize, and range
**      10-mar-1997 (stial01)
**          Alloc/init mx_crecord, area to be used for compression.
**      23-sep-1997 (shero03)
**          B85561: add comp/expand overhead to mx_crecord size
**	01-Oct-1997 (shero03)
**	    B85456: use the mx_log_id in lieu of the xcb_log_id.
**      15-Mar-1999 (stial01)
**          dmv_reindex() Build DM2U_INDEX_CB instead of more complex MXCB.
**          Call dm2u_index instead of dm2u_load_table.
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Set indxcb_dmveredo to TRUE so that we do not perform
**          unnecessary duplicate checking during the recreation
**          of the unique index table.
**	23-Dec-2003 (jenjo02)
**	    Added dui_relstat2 for Global Indexes, Partitioning.
**	6-Feb-2004 (schka24)
**	    Pass along name generator info.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Open no-coupon to avoid unnecessary LOB overhead.
*/
static DB_STATUS
dmv_reindex(
DMVE_CB         *dmve,
DM0L_INDEX 	*log_rec)
{
    DMP_RCB		*rcb = NULL;
    DMP_TCB		*t;
    DB_TAB_TIMESTAMP	timestamp;
    i4                  i;  
    i4                  local_error;
    DB_STATUS		status = E_DB_OK;
    i4                  keycnt = 0;
    i4                  attcnt = 0;
    DM2U_KEY_ENTRY       *keyentry;
    DM2U_KEY_ENTRY       *curentry;
    DM2U_KEY_ENTRY       **keyptrs;
    DB_OWN_NAME         table_owner;
    DB_TAB_NAME         table_name;
    DM2U_INDEX_CB       *indx_cb = NULL;
    i4                  tup_cnt = 0;
    i4			*err_code = &dmve->dmve_error.err_code;
    DB_ERROR		local_dberr;
    DB_ATTS		*a;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	/*
	** Creation of gateway indexes has no rollforward actions here. The 
	** system catalog updates will be rolled forward but no action is 
	** taken on the underlying gateway table.
	*/
	if (log_rec->dui_status & TCB_GATEWAY)
	{
	    return (E_DB_OK);
	}

	/*
	** Open the table to allow us to extract the old rows from its
	** access methods.
	*/
	status = dm2t_open(dmve->dmve_dcb_ptr, &log_rec->dui_tbl_id, 
			DM2T_X, DM2T_UDIRECT, DM2T_A_READ_NOCPN,
			(i4)0, (i4)20, (i4)0,
			dmve->dmve_log_id, dmve->dmve_lk_id, 
			(i4)0, (i4)0, dmve->dmve_db_lockmode,
			&dmve->dmve_tran_id, &timestamp, &rcb, (DML_SCB *)NULL,
			&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	t = rcb->rcb_tcb_ptr;

	/* Key count is in log record, user-specified att count is figured
	** by scanning the dui-key map.  (Keys are always at the front.)
	*/
	keycnt = log_rec->dui_kcount;
	for (i = 0; i < DB_MAXIXATTS; i++)
	{
	    if (log_rec->dui_key[i] != 0)
		attcnt++;
	    else
		break;
	}

	/*
	** Allocate memory for DM2U_INDEX_CB, 
	** and DM2U_KEY_ENTRY pointers and entries
	*/
	status = dm0m_allocate(sizeof(DM2U_INDEX_CB) +
	    attcnt * sizeof (DM2U_KEY_ENTRY *) + 
	    attcnt * sizeof(DM2U_KEY_ENTRY),
	    (i4)0, (i4)DM2U_IND_CB, (i4)DM2U_IND_ASCII_ID,
	    (char *)NULL, (DM_OBJECT **)&indx_cb, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/* We have attribute numbers, allocate & init DM2U_KEY_ENTRYs */
	keyptrs = (DM2U_KEY_ENTRY **)(indx_cb + 1);
	keyentry = (DM2U_KEY_ENTRY *)(keyptrs + attcnt);

	for (i = 0; i < attcnt; i++)
	{
	    keyptrs[i] = curentry = keyentry++;
	    curentry->key_order = DMU_ASCENDING;
	    a = &t->tcb_atts_ptr[log_rec->dui_key[i]];
	    MEmove(a->attnmlen, a->attnmstr, ' ',
		DB_ATT_MAXNAME, curentry->key_attr_name.db_att_name);
	}

	/* Close the table */
	status = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	rcb = (DMP_RCB *)NULL;

	/* Initialize indxcb */
	indx_cb->indxcb_dcb = dmve->dmve_dcb_ptr;
	indx_cb->indxcb_xcb = (DML_XCB *)NULL;
	indx_cb->indxcb_index_name = (DB_TAB_NAME *)NULL; /* not logged */
	if (log_rec->dui_loc_count)
	    indx_cb->indxcb_location = log_rec->dui_location;
	else
	    indx_cb->indxcb_location = (DB_LOC_NAME *)NULL;
	indx_cb->indxcb_l_count = log_rec->dui_loc_count;

	indx_cb->indxcb_tbl_id = &log_rec->dui_tbl_id;
	indx_cb->indxcb_structure = log_rec->dui_structure;
	indx_cb->indxcb_i_fill = log_rec->dui_ifill;
	indx_cb->indxcb_l_fill = log_rec->dui_lfill;
	indx_cb->indxcb_d_fill = log_rec->dui_dfill;

	indx_cb->indxcb_compressed = log_rec->dui_comptype;
	/* FIXME we need another pass on key compression type to make
	** sure everyone in dmp knows it's a type not just a flag!
	*/
	indx_cb->indxcb_index_compressed = (log_rec->dui_ixcomptype != TCB_C_NONE);

	if (log_rec->dui_status & TCB_UNIQUE)
	    indx_cb->indxcb_unique = 1;
	else
	    indx_cb->indxcb_unique = 0;

	indx_cb->indxcb_relstat2 = log_rec->dui_relstat2;

        indx_cb->indxcb_dmveredo = TRUE;

	indx_cb->indxcb_min_pages = log_rec->dui_min;
	indx_cb->indxcb_max_pages = log_rec->dui_max;
	indx_cb->indxcb_acount = attcnt; /* all atts in index */
	indx_cb->indxcb_key = keyptrs;
	indx_cb->indxcb_kcount = keycnt; /* which cols are keys */
	indx_cb->indxcb_db_lockmode = DM2T_X;
	indx_cb->indxcb_allocation = log_rec->dui_allocation;
	indx_cb->indxcb_extend = log_rec->dui_extend;
	indx_cb->indxcb_page_type = log_rec->dui_pg_type;
	indx_cb->indxcb_page_size = log_rec->dui_page_size;
	indx_cb->indxcb_idx_id = &log_rec->dui_idx_tbl_id;
	if (log_rec->dui_structure == TCB_RTREE)
	    indx_cb->indxcb_range = &log_rec->dui_range[0];
	else
	    indx_cb->indxcb_range = (f8 *)NULL;
	indx_cb->indxcb_index_flags = 0;
	indx_cb->indxcb_gwattr_array = (DM_PTR *)NULL;
	indx_cb->indxcb_gwchar_array = (DM_DATA *)NULL;
	indx_cb->indxcb_gwsource = (DMU_FROM_PATH_ENTRY *)NULL;
	indx_cb->indxcb_qry_id = 0; /* Not used if rollforward */
	indx_cb->indxcb_tup_info = &tup_cnt;
	indx_cb->indxcb_tab_name = &table_name;
	indx_cb->indxcb_tab_owner = &table_owner;
	indx_cb->indxcb_reltups = log_rec->dui_reltups;
	indx_cb->indxcb_gw_id = 0;
	indx_cb->indxcb_char_array = (DM_DATA *)NULL; /* Not used if rollforward */
	indx_cb->indxcb_tbl_pri = 0; /* Not used if rollforward */
	indx_cb->indxcb_errcb = &dmve->dmve_error;

	/*
	** From rollforward, (dcb->dcb_status & DCB_S_ROLLFORWARD) != 0
	** If dm2u_index is called from rollforward:
	**   - Skips dm2u_file_create (file already exists)
	**   - Generates the same temp filename as the created file
	**   - Skips catalog updates (dm2u_update_catalogs)
	**   - Skips logging
	**   - Has no XCB or SCB, so we must pass log_id, lk_id, tran_id...
	*/
	indx_cb->indxcb_rfp.rfp_log_id = dmve->dmve_log_id;
	indx_cb->indxcb_rfp.rfp_lk_id = dmve->dmve_lk_id;
	STRUCT_ASSIGN_MACRO(dmve->dmve_tran_id,indx_cb->indxcb_rfp.rfp_tran_id);
	/* FIX ME case should be set from jsx_reg_case */
	indx_cb->indxcb_rfp.rfp_upcase = FALSE;
	indx_cb->indxcb_rfp.rfp_name_id = log_rec->dui_name_id;
	indx_cb->indxcb_rfp.rfp_name_gen = log_rec->dui_name_gen;
	indx_cb->indxcb_rfp.rfp_hash_buckets = log_rec->dui_buckets;

	/*
	** Call the physical layer to process the rest of the index create
	*/
	status = dm2u_index(indx_cb);
	if (status != E_DB_OK)
	    break;

	dm0m_deallocate((DM_OBJECT **)&indx_cb);
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

    if (indx_cb)
	dm0m_deallocate((DM_OBJECT **)&indx_cb);

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9684_REDO_INDEX);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_unindex	- UNDO Recovery of a table index operation
**
** Description:
**
**	This routine is used during UNDO recovery of a create index.  
**
**	The vast majority of the index operation is rolled back by undoing
**	the system catalog updates and File Rename operations via recovery
**	of their respective log records.
**
**	The actual creation of the table which makes up the secondary index
**	will be aborted by rolling back the CREATE of that index table and
**	the log records associated with it.
**
**	This routine in fact performs no recovery work but only logs a CLR
**	to the operation (which marks a non-redo point).
**
**	No purge-tcb is done here as it is assumed that one will be done as
**	part of the recovery of the CREATE operation which must have preceded
**	the INDEX.
**
** Inputs:
**	dmve			Recovery context block.
**	log_rec			Index log record.
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
**	  - Routine created from dmve_index code.
**	  - Added a statement count field to the modify and index log records.
**	18-oct-1993 (rogerk)
**	    Added bucket count to index log record.
**      15-apr-1994 (chiku)
**          Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass dui_page_size to the dm0l_index routine.
**	23-may-1996 (shero03)
**	    RTree: added dimension, hilbertsize, and range
**	23-Dec-2003 (jenjo02)
**	    Added dui_relstat2 for Global Indexes, Partitioning.
*/
static DB_STATUS
dmv_unindex(
DMVE_CB         *dmve,
DM0L_INDEX 	*log_rec)
{
    LG_LSN		*log_lsn = &log_rec->dui_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	     ((log_rec->dui_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->dui_header.flags | DM0L_CLR);

	    status = dm0l_index(dmve->dmve_log_id, dm0l_flags, 
			&log_rec->dui_tbl_id, &log_rec->dui_idx_tbl_id, 
			&log_rec->dui_name, &log_rec->dui_owner, 
			log_rec->dui_loc_count, log_rec->dui_location, 
			log_rec->dui_structure, log_rec->dui_status, 
			log_rec->dui_relstat2,
			log_rec->dui_min, log_rec->dui_max, 
			log_rec->dui_ifill, log_rec->dui_dfill, 
			log_rec->dui_lfill, log_rec->dui_reltups, 
			log_rec->dui_buckets,
			&log_rec->dui_key[0], log_rec->dui_kcount,
			log_rec->dui_allocation, log_rec->dui_extend, 
			log_rec->dui_pg_type,
			log_rec->dui_page_size,
			log_rec->dui_comptype, log_rec->dui_ixcomptype,
			log_rec->dui_name_id,
			log_rec->dui_name_gen, 
			log_rec->dui_dimension,
			log_rec->dui_hilbertsize, 
			(f8 *)&log_rec->dui_range,
			log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK) 
	    {
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }
	}

	return(E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9685_UNDO_INDEX);

    return(E_DB_ERROR);
}
