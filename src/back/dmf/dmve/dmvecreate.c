/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
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
#include    <dmpp.h>
#include    <dm1c.h>
#include    <dm1s.h>
#include    <dm2f.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmve.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <ddb.h>             /* Buncha stuff needed just for rdf.h */
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>

/**
**
**  Name: DMVECREATE.C - The recovery of a create table operation.
**
**  Description:
**	This file contains the routine for the recovery of a create table
**	operation.
**
**	    dmve_create  - The main routine
**	    dmv_uncreate - UNDO recovery
**	    dmv_recreate - ROLLFORWARD recovery
**
**  History:    
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	12-nov-1992 (jnash)
**	    Rewritten for reduced logging project.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	30-feb-1993 (rmuth)
**	    Need to include di.h for dm0c.h
**	15-may-1993 (rmuth)
**	    Concurrent_index: If create log record is for a concurrent
**	    index do not PURGE the base table. The purge operation could
**	    cause deadlock problems and is not needed as these indicies
**	    have a special TCB update method.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (jnash)
**	    Add TRUE build_tcb_flag to dm1s_empty_table().
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-93 (rogerk)
**	    Added CLR logging in undo of table create operations.
**	    Changed parameters to lower-level recovery routines.
**	20-sep-1993 (rogerk)
**	    Fixed problem with rollforward of views and gateways.
**	    Added checks for view creation, don't attempt to rollforward
**	    file operations when the create is a view (same as gateway
**	    handling).  Also made sure that we DO rollforward the config
**	    file tableid reservation in the recovery of a view or gateway
**	    registration.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Examine the usage when checkinf the logical name in the config
**	    file
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass duc_page_size argument to dm0l_create.
**	    Pass duc_page_size argument to dm1s_empty_table.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
**	10-apr-1996 (thaju02)
**	    New Page Format Project: pass page size to dm1c_getaccessors().
**	25-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	21-mar-1999 (nanpr01)
**	    Raw location support and recovery.
**      21-dec-1999 (stial01)
**          Use logged page_type to get page accessor routines
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	20-Feb-2008 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_? functions converted to DB_ERROR *
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1s_? functions converted to DB_ERROR *
**/

static DB_STATUS build_location_info(
	DMVE_CB		*dmve_cb, 
	DMP_LOCATION	*loc_array);

static DB_STATUS dmv_recreate(
	DMVE_CB         *dmve,
	DM0L_CREATE     *log_rec);


static DB_STATUS dmv_uncreate(
	DMVE_CB         *dmve,
	DM0L_CREATE     *log_rec);


/*
** Name: dmve_create - The recovery of a create table opration.
**
** Description:
**	This function re-execute the create table opration using the
**	parameters stored in the log record. The system table changes,
**	the file creation and the file rename operations associated
**	with this create table operation appear later in the journal
**	file, so these opeorations are not performed during recovery.
**	This routine has undo code to purge the table descriptor.
**	When purge table descriptor in undo case, don't update the system
**	catalog info for the table (since there is not system catalog info
**	on this table).  Also, invalidate any pages associated with this
**	table in the cache.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create table operation log record.
**	    .dmve_action	    Should only be DMVE_DO.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
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
**	17-nov-92 (jnash)
**          Rewritten for the reduced logging project.
**	23-aug-93 (rogerk)
**	    Changed parameters to lower-level recovery routines.
**	    Moved accessor code down to dmv_recreate.
**	13-Jul-2007 (kibro01) b118695
**	    DM0L_CREATE structure may be larger if the number of locations
**	    exceeds DM_MAX_LOC.
*/
DB_STATUS
dmve_create(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_CREATE		*log_rec = (DM0L_CREATE *)dmve_cb->dmve_log_rec;
    i4		recovery_action;
    i4		error;
    DB_STATUS		status = E_DB_OK;
    i4		actual_size;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	if (log_rec->duc_header.type != DM0LCREATE ||
	    dmve->dmve_action == DMVE_REDO)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	actual_size = sizeof(DM0L_CREATE);
	if (log_rec->duc_loc_count > DM_LOC_MAX)
	    actual_size +=
		(log_rec->duc_loc_count - DM_LOC_MAX) * sizeof(DB_LOC_NAME);

	if (log_rec->duc_header.length > actual_size)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.  Code within UNDO recognizes the CLR and does not
	** write another CLR log record.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->duc_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_recreate(dmve, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_uncreate(dmve, log_rec);
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
    SETDBERR(&dmve->dmve_error, 0, E_DM9606_DMVE_CREATE);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_recreate	- ROLLFORWARD Recovery of a table create operation
**
** Description:
**
**	This routine is used to perform ROLLFORWARD recovery of 
**	the creation of a table.  Recovery is performed on each table 
**	location independently.
**
**	Unlike other page-oriented recovery operations, create table 
**	recovery is entirely logical.
**
**	This routine assumes that tables are always created as heaps, and 
**	that a single DATA, a single FHDR and some number of FMAP pages 
**	reside on contiguous pages.  In order to provide additional 
**	assurance that the logical recovery performed here is accurate, a 
**	DM0L_CREATE_VERIFY is written after a table create operation 
**	is complete, and used to verify that ROLLFORWARD rebuilt 
**	the table as it was originally built.
**
**	The routine is designed to handle "create table with allocation = 
**	xxx" syntax, even though that capability does not exist elsewhere.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create table operation log record.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
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
**	12-nov-1992 (jnash)
**	    Rewritten for reduced logging project.
**	26-jul-1993 (jnash)
**	    Add TRUE build_tcb_flag to dm1s_empty_table().
**	23-aug-1993 (rogerk)
**	  - Changed arguments, moved accessors locally.
**	  - Took file creation work out of dm1s_empty_table; it is now done
**	    by the recovery of the fcreate log records.  Added dm2f_open_file
**	    and dm2f_build_fcb calls to open the files before calling
**	    dm1s_empty_table.  Added close operations on the table when done.
**	  - Changed parameters to dm1s_empty_table call.
**	  - Made config file update dependent upon its current state - only
**	    updating with the new table_id if the config file does not already
**	    have a last_table_id value greater than the just-created table.
**	    This just seems more in-line with partial recovery capabilities.
**	  - Added check for gateway creates.
**	  - Added traceback error number.
**	20-sep-1993 (rogerk)
**	    Fixed problem with rollforward of views and gateways.
**	    Added checks for view creation, don't attempt to rollforward
**	    file operations when the create is a view (same as gateway
**	    handling).  Also made sure that we rollforward the config
**	    file tableid reservation in the recovery of a view or gateway
**	    registration.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass duc_page_size argument to dm1s_empty_table.
**	    Pass duc_page_size argument to dm2f_open_file.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
**	10-apr-1996 (thaju02)
**	    New Page Format Project: pass page size to dm1c_getaccessors().
**      01-feb-2001 (stial01)
**          Pass page type to dm1s_empty_table (Variable Page Type SIR 102955)
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for an index, not just != 0.
**	6-Feb-2004 (schka24)
**	    Update call to build fcb.
**	    Don't redo partitioned master creates.
*/
static DB_STATUS
dmv_recreate(
DMVE_CB         *dmve,
DM0L_CREATE 	*log_rec)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DM0C_CNF		*config;
    DMPP_ACC_PLV 	*loc_plv;
    u_i4		db_sync_flag;
    i4		tabid_number;
    bool		built_fcbs = FALSE;
    bool		opened_files = FALSE;
    i4		local_error;
    DB_STATUS		status = E_DB_OK;
    DMP_LOCATION	loc_cnf[DM_LOC_MAX];   /* loc info for all locs */
    DB_ERROR		local_dberr;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);


    for (;;)
    {
	/*
	** Update the configuration file last_table_id to make sure
	** that it is at least as great as this table id just created.
	*/
	status = dm0c_open(dcb, DM0C_PARTIAL, dmve->dmve_lk_id, 
			&config, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	tabid_number = log_rec->duc_tbl_id.db_tab_base;
	if (log_rec->duc_tbl_id.db_tab_index > 0)
	    tabid_number = log_rec->duc_tbl_id.db_tab_index;

	if (config->cnf_dsc->dsc_last_table_id <= tabid_number)
	{
	    config->cnf_dsc->dsc_last_table_id = tabid_number + 1;
	}

	status = dm0c_close(config, DM0C_UPDATE, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;


	/*
	** If the creation is of an ingres object which has no underlying
	** physical table (gateway or view), then we can drop out of
	** recovery processing now.  The only updates required were to
	** the config file above.
	*/
	if (log_rec->duc_status & (TCB_GATEWAY | TCB_VIEW | TCB_IS_PARTITIONED))
	{
	    return (E_DB_OK);
	}


	/*
	** Get page accessors for recovery actions, assuming
	** that the created table is a heap.
	*/
	dm1c_get_plv(log_rec->duc_page_type, &loc_plv);

	/*
	** Pick file open sync mode.
	*/
	db_sync_flag = 
		((dcb->dcb_sync_flag & DCB_NOSYNC_MASK) ? 0 : DI_SYNC_MASK);

	/*
	** Build DMP_LOCATION info for each location in log record.
	*/
	status = build_location_info(dmve, loc_cnf);
	if (status != E_DB_OK)
	    break;

	/*
	** Allocation and initialize FCB information for file access.
	*/
	status = dm2f_build_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
			loc_cnf, log_rec->duc_loc_count,  
			log_rec->duc_page_size, DM2F_ALLOC, DM2F_TAB,
			&log_rec->duc_tbl_id, 0, 0, (DMP_TABLE_IO *)NULL, 
			db_sync_flag, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;
	built_fcbs = TRUE;

	/*
	** Open the table files for the build operation below.
	*/
	status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id, loc_cnf, 
			log_rec->duc_loc_count, log_rec->duc_page_size,
			DM2F_A_WRITE, 
			db_sync_flag, (DM_OBJECT*)dcb, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;
	opened_files = TRUE;

	/*
	** Build an empty heap, note partial recov info in loc_cnt.
	*/
	status = dm1s_empty_table(dcb, &log_rec->duc_tbl_id,
			log_rec->duc_allocation, loc_plv, log_rec->duc_fhdr,
			log_rec->duc_first_fmap, log_rec->duc_last_fmap, 
			log_rec->duc_first_free, log_rec->duc_last_free, 
			log_rec->duc_first_used, log_rec->duc_last_used,
			log_rec->duc_page_type,  log_rec->duc_page_size,
			log_rec->duc_loc_count, loc_cnf, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Close the files initialized in the build operation.
	*/
	opened_files = FALSE;
	status = dm2f_close_file(loc_cnf, log_rec->duc_loc_count,
			(DM_OBJECT *)dcb, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Deallocate FCB memory.
	*/
	built_fcbs = FALSE;
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id, loc_cnf,
			log_rec->duc_loc_count, DM2F_ALLOC, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    /*
    ** Check if there is file context to clean up.
    */
    if (opened_files)
    {
	status = dm2f_close_file(loc_cnf, log_rec->duc_loc_count,
			(DM_OBJECT *)dcb, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (built_fcbs)
    {
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id, loc_cnf,
			log_rec->duc_loc_count, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM967C_REDO_CREATE);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_uncreate	- UNDO Recovery of a table create operation
**
** Description:
**
**	This routine is used to perform UNDO recovery of 
**	the creation of a table.  
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create table operation log record.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
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
**	12-nov-1992 (jnash)
**	    Created for reduced logging project.
**	15-may-1993 (rmuth)
**	    Concurrent_index: If create log record is for a concurrent
**	    index do not PURGE the base table. The purge operation could
**	    cause deadlock problems and is not needed as these indicies
**	    have a special TCB update method.
**	23-aug-1993 (rogerk)
**	    Added logging of CLR record so tcb purge will be done if the
**	    create-abort is rolled forward.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass duc_page_size argument to dm0l_create.
**      13-Mar-2010 (hanal04) Bug 123423
**          Invalidate the RDF cache entry for a a table when we UNDO
**          a create.
*/
static DB_STATUS
dmv_uncreate(
DMVE_CB         *dmve,
DM0L_CREATE 	*log_rec)
{
    LG_LSN		*log_lsn = &log_rec->duc_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;
    RDF_CB	rdfcb;            /* RDF call request block */

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Create was aborted.  Purge the table descriptor in case
	** someone has already looked in the iirelation catalog and
	** has seen the table used to exist.
	**
	** We don't purge the TCB of the base table during abort of a
	** Concurrent Index create.  The new index is not actually added
	** to the base table until a later operation.  The TCB will be
	** purged at that time (and during recovery of that operation).
	*/
	if ((log_rec->duc_flags & DM0L_CREATE_CONCURRENT_INDEX) == 0)
	{
	    status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &log_rec->duc_tbl_id, 
			    (DM2T_PURGE | DM2T_NORELUPDAT), dmve->dmve_log_id, 
			    dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			    dmve->dmve_db_lockmode, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	}

        /* Invalidate the RDF reference to the table. Ignore any error 
        ** on this RDF invalidate, not much can be done.
        */
        MEfill(sizeof(RDF_CB), 0, &rdfcb);
        STRUCT_ASSIGN_MACRO(log_rec->duc_tbl_id, rdfcb.rdf_rb.rdr_tabid);
        rdfcb.rdf_rb.rdr_session_id = DB_NOSESSION;
        rdfcb.rdf_rb.rdr_unique_dbid = log_rec->duc_header.database_id;
        rdfcb.rdf_rb.rdr_db_id = (PTR)dmve->dmve_dcb_ptr;
        (void) rdf_call(RDF_INVALIDATE, &rdfcb);

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->duc_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->duc_header.flags | DM0L_CLR);

	    status = dm0l_create(dmve->dmve_log_id, dm0l_flags,
		&log_rec->duc_tbl_id, &log_rec->duc_name, &log_rec->duc_owner,
		log_rec->duc_allocation, log_rec->duc_flags,
		log_rec->duc_status, log_rec->duc_fhdr,
		log_rec->duc_first_fmap, log_rec->duc_last_fmap,
		log_rec->duc_first_free, log_rec->duc_last_free,
		log_rec->duc_first_used, log_rec->duc_last_used,
		log_rec->duc_loc_count, log_rec->duc_location,
		log_rec->duc_page_size, log_rec->duc_page_type,
		log_lsn, &lsn, &dmve->dmve_error);
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
    SETDBERR(&dmve->dmve_error, 0, E_DM967D_UNDO_CREATE);

    return(E_DB_ERROR);
}

/*{
** Name: build_location_info	- Build list of recovered locations
**
** Description:
**
**	Given a list of locations from the table create log record,
**	build DMP_LOCATION information for each location.  
**	We assume that the list of locations is in "location id" order (the 
**	order of the entries in iidevices).
**	
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create table operation log record.
**	    .dmve_action	    Should only be DMVE_DO.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**
** Outputs:
**	loc_array		    Pointer to array of location info
**      err_code                    Reason for error status.
**					
** Side Effects:
**	    
** History:
**	12-nov-1992 (jnash)
**	    Created for reduced logging project.
**	16-dec-1992 (jnash)
**	    Fix bug by changing MEmove to MEcopy.
**	23-aug-1993 (rogerk)
**	    Set location status to LOC_VALID when initialize location fields.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Examine the usage when checkinf the logical name in the config
**	    file
*/
static DB_STATUS
build_location_info(
DMVE_CB			*dmve_cb, 
DMP_LOCATION		*loc_array)
{
    DM0L_CREATE		*log_rec = (DM0L_CREATE *)dmve_cb->dmve_log_rec;
    DMP_DCB             *dcb = dmve_cb->dmve_dcb_ptr;
    DMP_EXT 		*ext = dcb->dcb_ext;
    i4		i, j;
    i4		compare;
    DMP_LOCATION	*loc;
    DB_LOC_NAME		*log_loc;

    /* 
    ** Build list of loc_config_ids for locations in log.
    ** Flag if location is not being recovered (partial recovery case).
    */
    for (i = 0; i < log_rec->duc_loc_count; i++)
    {
	loc = &loc_array[i];
	log_loc = &log_rec->duc_location[i];
        loc->loc_status = LOC_UNAVAIL;       

	/* 
	** Check root location
	*/	
	if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)log_loc,
	     sizeof(DB_LOC_NAME)) == 0)
	{
	    MEcopy((PTR)log_loc, sizeof(DB_LOC_NAME), (PTR)&loc->loc_name);
	    loc->loc_ext = dcb->dcb_root;
	    loc->loc_config_id = 0;
	    loc->loc_id = i;
	    loc->loc_status = LOC_VALID;
	    loc->loc_fcb = 0;
	}
	else
	{
	    /* 
	    ** Check extents 
	    */	
	    compare = -1;
	    for (j = 0; j < ext->ext_count; j++)
	    {
		/*
		**  Bug B67276
		**  Only check DATA usage locations
		*/
		if ((dcb->dcb_ext->ext_entry[j].flags & DCB_DATA) == 0)
		    continue;

		compare = MEcmp((PTR)log_loc, 
		    (char *)&dcb->dcb_ext->ext_entry[j].logical,
		    sizeof(DB_LOC_NAME));
		if (compare == 0)
		{
		    MEcopy((PTR)log_loc, sizeof(DB_LOC_NAME), 
			 (PTR)&loc->loc_name);
		    loc->loc_ext = &dcb->dcb_ext->ext_entry[j];
		    loc->loc_config_id = j;
		    loc->loc_id = i;
		    loc->loc_status = LOC_VALID;
		    loc->loc_fcb = 0;
		    if (dcb->dcb_ext->ext_entry[j].flags & DCB_RAW)
			loc->loc_status |= LOC_RAW;
		    break;
		}
	    }

	    if (compare != 0)
	    {
		/* XXXX Better error information XXXX */
		TRdisplay("build_location_list: logic error\n");
		return(E_DB_ERROR);
	    }
	}
    }

    return(E_DB_OK);
}
