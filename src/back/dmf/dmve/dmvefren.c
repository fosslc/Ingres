/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
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
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2f.h>

/**
**
**  Name: DMVEFREN.C - The recovery of a rename file operation.
**
**  Description:
**	This file contains the routine for the recovery of a rename
**	file operation.
**
**          dmve_frename - The recovery of a rename file operation.
**          dmv_rerename - ROLLFORWARD recovery of a rename file operation.
**          dmv_unrename - UNDO recovery of a rename file operation.
**
**
**  History:    
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**      23-nov-87 (jennifer)
**          Added multiple locations per table support.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() calls.
**	 2-oct-89 (rogerk)
**	    Pass flag to dm2f_open indicating to not print error if
**	    file does not exist.
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename call.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	29-oct-1992 (jnash)
**	    Reduced logging project.  Modified and restructured for
**	    new recovery protocols.  Now that rename operations are
**	    idempotent via CLR's, we no longer log FSWAP log records.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
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
**	    Start of DDL bugfixing for recovery project.  Don't delete
**	    files in rollforward of rename operations when the resultant
**	    renamed file has a delete extension.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Took out renaming of logged file name to use zero statement
**	    counts in rollforward processing.  Changed order of undo recovery 
**	    actions: rename the file and then log the CLR record.
**	20-sep-1993 (rogerk)
**	    Changes to rollforward recovery to support non-journaled tables.
**	    Added calls to pend frename action when a file is renamed to
**	    a delete file. This saves context to delete the file later when
**	    the ET record is found. (This functionality moved here from dmfrfp)
**	    Redo of frenames now ignores file-not-exist errors when the
**	    rename operation is associated with a non-journaled table.
**	    Created file_exists routine to encapsulate file checking code.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	19-jan-1995 (cohmi01)
**	    For RAW locations: Callers of file_exists() allocate a DMP_LOCATION
**	    which gets filled in by file_exists(), and used by the caller
**	    if necesary for any calls to dm2f_rename().
**      06-mar-1996 (stial01)
**          file_exists(): need to pass page size to dm2f_build_fcb,
**	25-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-jul-2003
**          Added include adf.h
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2f_? functions converted to DB_ERROR *
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/


/*
**  Definition of static variables and forward static functions.
*/

static DB_STATUS dmv_rerename(	
	DMVE_CB    	*dmve,
	DM0L_FRENAME 	*log_rec);

static DB_STATUS dmv_unrename(
	DMVE_CB    	*dmve,
	DM0L_FRENAME 	*log_rec);

static bool	file_exists(
	DMVE_CB    	*dmve,
	i4		path_length,
	DM_PATH		*path,
	DM_FILE		*filename);


/*{
** Name: dmve_frename - The recovery of a rename file operation.
**
** Description:
**	This function performs the recovery of the rename file operation.
**	For UNDO if the new file exists it is renamed to the old
**	file. For ROLLFORWARD, rename the old file to the new file and delete
**	the new file if operation succeed.
**
**	As part of the reduced logging project, file swap log records were
**	eliminated, supplanted with dm0l_frename log records.  CLR's now make
**	rename operations idempotent.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The rename file log record.
**	    .dmve_action	    Should be DMVE_UNDO, DMVE_REDO or DMVE_DO.
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
**      23-nov-87 (jennifer)
**          Added multiple locations per table support.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() call.  Sync param
**	    is always 0 since we are just checking for existance (ie. no 
**	    writes are ever done.)
**	 2-oct-89 (rogerk)
**	    Pass flag to dm2f_open indicating to not print error if
**	    file does not exist.
**	    Pass correct error parameter to dm2f_rename.
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename call.
**	29-oct-1992 (jnash)
**	    Reduced logging project.  Modified and restructured for
**	    new recovered protocols.
**	08-Jan-2009 (jonj)
**	    Upgraded missed ule_format to uleFormat.
*/
DB_STATUS
dmve_frename(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_FRENAME	*log_rec = (DM0L_FRENAME*)dmve_cb->dmve_log_rec;
    i4         	recovery_action;
    i4		error;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    for (;;)
    {
	if (log_rec->fr_header.length != sizeof(DM0L_FRENAME) || 
	    log_rec->fr_header.type != DM0LFRENAME)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** FRENAME actions are never applied during true online REDO.
	*/
	if (dmve->dmve_action == DMVE_REDO)
	    break;

	/*
	** Check if this location is being recovered.
	** If it is not, then there is no recovery to execute here.
	**
	** When table-level recovery is introduced, we'll also
	** have to check if the table being recovered is available.
	** At that time a dmve_tabid_check() routine be required, and 
	** the table id's in the log record will be used.  
	*/
	if ( ! dmve_location_check(dmve, (i4)log_rec->fr_cnf_loc_id))
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->fr_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_rerename(dmve, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unrename(dmve, log_rec);
	    break;
	}

	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    if (status != E_DB_OK)
    {
	if (dmve->dmve_error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DM960A_DMVE_FRENAME);
	}

	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: dmv_unrename - UNDO the Rename operation
**
** Description:
**      This function performs UNDO recovery of a file rename
**	operation.  If the new file exists it is renamed to the old
**      file.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**
** Outputs:
**	error				Pointer to Error return area
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-oct-1992 (jnash)
**	    Rewritten from an older version for reduced logging project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Changed order of recovery actions: rename the file and then
**	    log the CLR record.  Use dm2f_build_fcb to allocate fcb's.
**	20-sep-1993 (rogerk)
**	    Moved code which checks existence of file into file_exists
**	    routine to be shared with rerename routine.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	19-jan-1995 (cohmi01)
**	    For raw file support, since dm2f_rename now requires a
**	    DMP_LOCATION ptr (and fcb ...) add a local one here and let
**	    file_exists() init it, use it, and leave it for us to use and free.
**	30-Apr-2004 (jenjo02)
**	    Call dmve_unreserve_space call to release 
**	    LG_RS_FORCE logspace.
*/
static DB_STATUS
dmv_unrename(
DMVE_CB    	*dmve,
DM0L_FRENAME 	*log_rec)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    LG_LSN		*log_lsn = &log_rec->fr_header.lsn; 
    LG_LSN		lsn;
    bool		rename_needed;
    i4		dm0l_flags;
    DB_STATUS		status;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	rename_needed = file_exists(dmve, log_rec->fr_l_path, 
		    &log_rec->fr_path, &log_rec->fr_newname);

	/* 
	** If the new file exists, rename it back to the old filename.
	*/ 
	if (rename_needed)
	{
	    status = dm2f_rename(dmve->dmve_lk_id, dmve->dmve_log_id,
		&log_rec->fr_path, log_rec->fr_l_path, 
		&log_rec->fr_newname, sizeof(log_rec->fr_newname), 
		&log_rec->fr_oldname, sizeof(log_rec->fr_oldname), 
		&dcb->dcb_name, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->fr_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->fr_header.flags | DM0L_CLR);

	    status = dm0l_frename(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->fr_path, log_rec->fr_l_path, 
		&log_rec->fr_oldname, &log_rec->fr_newname,
		&log_rec->fr_tbl_id, log_rec->fr_loc_id, 
		log_rec->fr_cnf_loc_id,  
		log_lsn, &lsn, &dmve->dmve_error);

	    if (status != E_DB_OK) 
	    {
                /*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }

	    /* Release extra log space */
	    dmve_unreserve_space(dmve, 1);
	}

	return (E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9681_UNDO_FRENAME);

    return (E_DB_ERROR);
}

/*{
** Name: dmv_rerename - ROLLFORWARD recovery a file Rename operation
**
** Description:
**      This function performs DO recovery of a file rename operation.
**	Rename the old file to the new file and delete the new file.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**
** Outputs:
**	error				Pointer to Error return area
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-oct-1992 (jnash)
**	    Created from older version for the reduced logging project.
**	21-jun-1993 (rogerk)
**	    Start of DDL bugfixing for recovery project.  Don't delete
**	    files in rollforward of rename operations when the resultant
**	    renamed file has a delete extension.  Since we no longer can
**	    assume that the update was committed we have to leave the delete 
**	    file around until the ET record is encountered.
**	23-aug-1993 (rogerk)
**	    Took out renaming of logged file name to use zero statement
**	    count.  Previously, the rollforward of modify/index operations
**	    ran with an assumed dmu_stmt_cnt of zero. This meant that the
**	    files they expected to find to load would have filenames built
**	    by calling dm2f_filename with stmt_count=0.  This caused us to
**	    have to catch the filenames we created/renamed and fix up the
**	    front of the filename as if it were created with stmt_count=0.
**	    We now log the statement count in the modify and index log records
**	    so we don't have to do this filename mucking anymore.
**	20-sep-1993 (rogerk)
**	    Changes to rollforward recovery to support non-journaled tables.
**	    Added calls to pend frename action when a file is renamed to
**	    a delete file. This saves context to delete the file later when
**	    the ET record is found. (This functionality moved here from dmfrfp)
**	    Redo of frenames now ignores file-not-exist errors when the
**	    rename operation is associated with a non-journaled table.  Added
**	    calls to open the file first to check for existence prior to the
**	    rename call.
**	19-jan-1995 (cohmi01)
**	    For raw file support, since dm2f_rename now requires a
**	    DMP_LOCATION ptr (and fcb ...) add a local one here and let
**	    file_exists() init it, use it, and leave it for us to use and free.
**	    Call file_exists() no matter what, so LOC gets setup OK.
*/
static DB_STATUS
dmv_rerename(
DMVE_CB    	*dmve,
DM0L_FRENAME 	*log_rec)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    LG_LSN		*log_lsn = &log_rec->fr_header.lsn; 
    LG_LSN		lsn;
    RFP_REC_ACTION	*action;
    bool		rename_needed;
    bool		file_exist;  
    i4		dm0l_flags;
    DB_STATUS		status;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Check for existence of the old filename when redoing operation
	** on behalf of a non-journaled table.  In this situation it is
	** possible that the table/file does not exist so this operation
	** may need to be skipped.
	**
	** Otherwise call the existence check and assume file exists.
	*/
	rename_needed = TRUE;
	file_exist = file_exists(dmve, log_rec->fr_l_path, 
		    &log_rec->fr_path, &log_rec->fr_oldname);
	if (log_rec->fr_header.flags & DM0L_NONJNL_TAB)
	{
	    rename_needed = file_exist;
	}

	/*
	** Rename the old filename to the new filename.
	*/
	if (rename_needed)
	{
	    status = dm2f_rename(dmve->dmve_lk_id, dmve->dmve_log_id,
		&log_rec->fr_path, log_rec->fr_l_path, 
		&log_rec->fr_oldname, sizeof(log_rec->fr_oldname), 
		&log_rec->fr_newname, sizeof(log_rec->fr_newname),
		&dcb->dcb_name, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** If this is a rollforward of a rename to a DEL file, then we need
	** to save the context of the rename so we can remember to later
	** remove the temporary file.  This is done when the End Transaction
	** record for this xact is found.
	*/
	if ((dmve->dmve_action == DMVE_DO) &&
	    (log_rec->fr_newname.name[9] == 'd') &&
	    (rename_needed))
	{
	    status = rfp_add_action(dmve, &dmve->dmve_tran_id, log_lsn, 
			RFP_FRENAME, &action);
	    if (status != E_DB_OK)
		break;

	    action->rfp_act_dbid = log_rec->fr_header.database_id;
	    action->rfp_act_tid.tid_tid.tid_page = 0;
	    action->rfp_act_tid.tid_tid.tid_line = 0;
	    action->rfp_act_p_len = log_rec->fr_l_path;
	    action->rfp_act_f_len = sizeof(log_rec->fr_newname);
	    STRUCT_ASSIGN_MACRO(log_rec->fr_path, action->rfp_act_p_name);
	    STRUCT_ASSIGN_MACRO(log_rec->fr_newname, action->rfp_act_f_name);
	    STRUCT_ASSIGN_MACRO(log_rec->fr_tbl_id, action->rfp_act_tabid);
	}

	return (E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9680_REDO_FRENAME);

    return (E_DB_ERROR);
}

/*{
** Name: file_exists - Check for necessity of rename operation.
**
** Description:
**	This routine checks for the existence of the source file in
**	a file rename recovery operation to determine whether the
**	the rename operation is really needed.
**
**	If an error is encountered then TRUE is returned.  This will
**	probably lead to an error attempting to rename the file as well.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**	path_length			Length of path name string
**	path				Path to filename
**	filename			Source file name
**
** Outputs:
**	none
**	Returns:
**	    TRUE		- source file exists
**	    FALSE		- source file does not exist, no rename needed
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-sep-1993 (rogerk)
**	    Created.
**	19-jan-1995 (cohmi01)
**	    For raw file support, since callers of this routine need a
**	    DMP_LOCATION ptr (and fcb ...), they will pass one to us to use.
**	    Caller will now free the FCB memory.                               
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb
**	6-Feb-2004 (schka24)
**	    Update call to build-fcb.
*/
static bool
file_exists(
DMVE_CB    	*dmve,
i4		path_length,
DM_PATH		*path,
DM_FILE		*filename) 
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DB_TAB_ID		dummy_tabid;
    DB_STATUS		status;
    u_i4		db_sync_flag;
    DMP_LOCATION	loc_entry;
    DMP_LOC_ENTRY	ext;
    i4		error;
    bool		file_exists = TRUE;
    i4             page_size = DM_PG_SIZE; /* smallest ingres page size */
    DB_ERROR		local_dberr;

    CLRDBERR(&local_dberr);

    /*
    ** Pick file create sync mode.
    */
    db_sync_flag = ((dcb->dcb_sync_flag & DCB_NOSYNC_MASK) ? 0 : DI_SYNC_MASK);

    for (;;)
    {
	/*
	** Fill in enough of the loc, ext and fcb structures to allow
	** the file open operation to run.
	*/
	loc_entry.loc_status = LOC_VALID;
	loc_entry.loc_id = 0;
	loc_entry.loc_fcb = 0;
	loc_entry.loc_ext = &ext;

	ext.phys_length = path_length;
	MEcopy((char *)path, sizeof(DM_PATH), (char *)&ext.physical);

	/*
	** Allocate and format FCB fields.  Pass in dummy table_id and
	** location id and just overwrite the filename computed by the routine
	** with the logged filename below.
	*/
	dummy_tabid.db_tab_base = 0;
	dummy_tabid.db_tab_index = 0;
	status = dm2f_build_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&loc_entry, 1, page_size, DM2F_ALLOC,
				DM2F_TAB, &dummy_tabid, 0, 0, (DMP_TABLE_IO *)NULL,
				db_sync_flag, &local_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Replace the filename computed by dm2f_build_fcb with the one
	** logged in the DM0L_FRENAME record.
	*/
	MEcopy((char *)filename, sizeof(DM_FILE), 
		(char *)&loc_entry.loc_fcb->fcb_filename);
	loc_entry.loc_fcb->fcb_namelength = sizeof(DM_FILE);

	/* 
	** Open the file to check for its existence.
	*/
	status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id, &loc_entry,
	    1, (i4)DM_PG_SIZE, DM2F_E_READ, 0, (DM_OBJECT *)dcb, &local_dberr);
	if (status != E_DB_OK)
	{
	    if (local_dberr.err_code != E_DM9291_DM2F_FILENOTFOUND)
		break;

	    file_exists = FALSE;
	    status = E_DB_OK;
	    CLRDBERR(&local_dberr);
	}
	else
	{
	    status = dm2f_close_file(&loc_entry, 1, (DM_OBJECT *)dcb, &local_dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Release the FCB memory.
	*/
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id, 
				&loc_entry, 1, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	    break;
	return (file_exists);
    }

    /*
    ** Error handling:
    **     Format error, clean up resources and return that a file rename
    **     should be attempted.
    */

    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);

    /*
    ** Release allocated FCB memory if we failed after allocating it.
    */
    if (loc_entry.loc_fcb)
    {
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&loc_entry, 1, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
    	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    return (TRUE);
}
