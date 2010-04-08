/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
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
#include    <dm2f.h>
#include    <dm1b.h>
#include    <dm0l.h>

/**
**
**  Name: DMVEFCRE.C - The recovery of a create file operation.
**
**  Description:
**	This file contains the routine for the recovery of a create
**	file operation.
**
**          dmve_fcreate - The recovery of a create file operation.
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
**	    added "sync_flag" parameter to dm2f_create_file() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	15-nov-1992 (jnash)
**	    Reduced logging project.  Change the way in which 
**	    files are created and initialized.  Instead of having this 
**	    routine initialize the file (via dm2f_init_file), it simply 
**	    handles UNDO recovery of the file creation, assuming that 
**	    DM0L_CREATE log records are used to handle ROLLFORWARD.
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
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Add Rolldb handling of File Create operations.
**	    Created dmv_refcreate and dmv_unfcreate routines.
**	    Changed order of file delete and CLR logging in undo case.
**	24-aug-1993 (wolf)
**	    Fix typo--E_DM967A_REDO_FCREATE should probably be E_DM968A...
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass fc_page_size to dm0l_fcreate.
**      06-mar-1996 (stial01)
**          Pass fc_page_size to dm2f_create_file
**	25-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**      21-mar-1999 (nanpr01)
**          Support raw locations.
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
**/

static DB_STATUS	dmv_refcreate(
				DMVE_CB             *dmve,
				DM0L_FCREATE	    *log_rec);

static DB_STATUS	dmv_unfcreate(
				DMVE_CB             *dmve,
				DM0L_FCREATE	    *log_rec);


/*{
** Name: dmve_fcreate - The recovery of a create file operation.
**
** Description:
**	This function performs the recovery of the create file operation.
**
**	In UNDO mode the created file is deleted if it exists.
**	In REDO mode the file is recreated.
**
**	This routine is only called in REDO mode during ROLLFORWARD recovery.
**	File Creates (or undo of file creates) are never redone in online
**	recovery since the file creation/deletion action takes permanent
**	effect.
**	
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create file log record.
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
**	    added "sync_flag" parameter to dm2f_create_file() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**      24-oct-1991 (rogerk)
**	    Pass allocation, mf_pos and mf_max information from the fcreate
**	    log record into the dm2f_init_file call as required.  Also put
**	    in a check to not call dm2f_init_file if the fcreate log record
**	    specifies to allocate no disk space.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Pass page_fmt to dm2f_init_file.
**	15-nov-1992 (jnash)
**	    Reduced logging project.  Add CLR handling.  Changed 
**	    along with dmve_create to make this routine only have
**	    UNDO responsibilities, elim allocation, mf_pos and mf_max.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Add Rolldb handling of File Create operations.
**	    Created dmv_refcreate and dmv_unfcreate routines.
**	    Changed order of file delete and CLR logging in undo case.
**	    Added check for partial recovery.
*/
DB_STATUS
dmve_fcreate(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_FCREATE	*log_rec = (DM0L_FCREATE *)dmve_cb->dmve_log_rec;
    i4		recovery_action;
    i4		error;
    DB_STATUS		status = E_DB_OK;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	if (log_rec->fc_header.length != sizeof(DM0L_FCREATE) || 
	    log_rec->fc_header.type != DM0LFCREATE)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** FCREATE actions are never applied during true online REDO.
	*/
	if (dmve->dmve_action == DMVE_REDO)
	    break;

	/*
	** Check if this location is being recovered.
	** If it is not, then there is no recovery to execute here.
	*/
	if ( ! dmve_location_check(dmve, (i4)log_rec->fc_cnf_loc_id))
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->fc_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	    status = dmv_refcreate(dmve, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unfcreate(dmve, log_rec);
	    break;
	}

	break;
    }

    if (status != E_DB_OK)
    {
	if (dmve->dmve_error.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9609_DMVE_FCREATE);
	}

	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}


/*{
** Name: dmv_refcreate - Redo file create action.
**
** Description:
**	This action creates a file as part of ROLLDB recovery of a File Create
**	action.  It is called only by Rollforward recovery, never in true
**	online redo operations.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**	log_record			Pointer to the log record
**
** Outputs:
**	err_code			Pointer to Error return area
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
**      23-aug-1993 (rogerk)
**	    Written.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
*/
static DB_STATUS
dmv_refcreate(
DMVE_CB             *dmve,
DM0L_FCREATE	    *log_rec)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_LOCATION        loc_entry; 
    DMP_LOC_ENTRY       ext;
    DB_TAB_ID		dummy_tabid;
    u_i4		db_sync_flag;
    i4		local_error;
    DB_STATUS		status;
    DB_ERROR		local_dberr;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** Pick file create sync mode.
    */
    db_sync_flag = ((dcb->dcb_sync_flag & DCB_NOSYNC_MASK) ? 0 : DI_SYNC_MASK);

    for (;;)
    {
	/*
	** Fill in enough of the loc, ext and fcb structures to allow
	** the file create operation to run.
	*/
	loc_entry.loc_status = log_rec->fc_loc_status;
	loc_entry.loc_id = 0;
	loc_entry.loc_fcb = 0;
	loc_entry.loc_ext = &ext;

	ext.phys_length = log_rec->fc_l_path;
	MEcopy((char *)&log_rec->fc_path, sizeof(DM_PATH),
		(char *)&ext.physical);

	/*
	** Allocate and format FCB fields.  Pass in dummy table_id and
	** location id and just overwrite the filename computed by the routine
	** with the logged filename below.
	*/
	dummy_tabid.db_tab_base = 0;
	dummy_tabid.db_tab_index = 0;
	status = dm2f_build_fcb(dmve->dmve_lk_id, dmve->dmve_log_id, 
				&loc_entry, 1, log_rec->fc_page_size, 
				DM2F_ALLOC, DM2F_TAB, &dummy_tabid, 0, 0,
				(DMP_TABLE_IO *)NULL, db_sync_flag, 
				&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Replace the filename computed by dm2f_build_fcb with the one
	** logged in the DM0L_FCREATE record.
	*/
	MEcopy((char *)&log_rec->fc_file, sizeof(DM_FILE), 
		(char *)&loc_entry.loc_fcb->fcb_filename);
	loc_entry.loc_fcb->fcb_namelength = sizeof(log_rec->fc_file);

	/*
	** Create the file.
	*/
	status = dm2f_create_file(dmve->dmve_lk_id, dmve->dmve_log_id, 
				&loc_entry, 1, log_rec->fc_page_size, 
				db_sync_flag,	(DM_OBJECT *)dcb, 
				&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Close the file since the dm2f_create leaves it open.
	*/
	status = dm2f_close_file(&loc_entry, 1, (DM_OBJECT *)dcb, 
				&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Release the FCB memory.
	*/
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&loc_entry, 1, DM2F_ALLOC, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    /*
    ** Release allocated FCB memory if we failed after allocating it.
    */
    if (loc_entry.loc_fcb)
    {
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&loc_entry, 1, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM968A_REDO_FCREATE);

    return (E_DB_ERROR);
}

/*{
** Name: dmv_unfcreate - Undo file create action.
**
** Description:
**	This action deletes a file as part the undo recovery of a File Create
**	action.
**
**	Unless rolling forward an FCREATE CLR we log a CLR record describing
**	the undo action.  This CLR is logged AFTER the actual file delete
**	so that we do not ever have to REDO a file operation.
**
**	It is assumed here that Fcreate recovery is idempotent and can
**	be re-executed if we crash after deleting the file but before
**	logging the CLR.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**	log_record			Pointer to the log record
**
** Outputs:
**	err_code			Pointer to Error return area
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
**      23-aug-1993 (rogerk)
**	    Written.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass fc_page_size to dm0l_fcreate.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb()
**	30-Apr-2004 (jenjo02)
**	    Call dmve_unreserve_space call to release 
**	    LG_RS_FORCE logspace.
*/
static DB_STATUS
dmv_unfcreate(
DMVE_CB             *dmve,
DM0L_FCREATE	    *log_rec)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    LG_LSN		*log_lsn = &log_rec->fc_header.lsn; 
    LG_LSN		lsn;
    DMP_LOCATION        loc_entry; 
    DMP_LOC_ENTRY       ext;
    DB_TAB_ID		dummy_tabid;
    u_i4		db_sync_flag;
    i4		dm0l_flags;
    i4		local_error;
    DB_STATUS		status;
    DB_ERROR		local_dberr;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** Pick file create sync mode.
    */
    db_sync_flag = ((dcb->dcb_sync_flag & DCB_NOSYNC_MASK) ? 0 : DI_SYNC_MASK);

    for (;;)
    {
	/*
	** Fill in enough of the loc, ext and fcb structures to allow
	** the file create operation to run.
	*/
	loc_entry.loc_status = log_rec->fc_loc_status;
	loc_entry.loc_id = 0;
	loc_entry.loc_fcb = 0;
	loc_entry.loc_ext = &ext;

	ext.phys_length = log_rec->fc_l_path;
	MEcopy((char *)&log_rec->fc_path, sizeof(DM_PATH),
		(char *)&ext.physical);

	/*
	** Allocate and format FCB fields.  Pass in dummy table_id and
	** location id and just overwrite the filename computed by the routine
	** with the logged filename below.
	*/
	dummy_tabid.db_tab_base = 0;
	dummy_tabid.db_tab_index = 0;
	status = dm2f_build_fcb(dmve->dmve_lk_id, dmve->dmve_log_id, 
				&loc_entry, 1, log_rec->fc_page_size, 
				DM2F_ALLOC, DM2F_TAB, &dummy_tabid, 0, 0,
				(DMP_TABLE_IO *)NULL, db_sync_flag, 
				&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Replace the filename computed by dm2f_build_fcb with the one
	** logged in the DM0L_FCREATE record.
	*/
	MEcopy((char *)&log_rec->fc_file, sizeof(DM_FILE), 
		(char *)&loc_entry.loc_fcb->fcb_filename);
	loc_entry.loc_fcb->fcb_namelength = sizeof(log_rec->fc_file);

	/*
	** Delete the file - ignore errors caused by its non-existence.
	*/
	status = dm2f_delete_file(dmve->dmve_lk_id, dmve->dmve_log_id, 
				&loc_entry, 1, (DM_OBJECT *)dcb, (i4) TRUE, 
				&dmve->dmve_error);
	if (status != E_DB_OK)
	{
	    if ((dmve->dmve_error.err_code != E_DM9291_DM2F_FILENOTFOUND) &&
		(dmve->dmve_error.err_code != E_DM9292_DM2F_DIRNOTFOUND) &&
		(dmve->dmve_error.err_code != E_DM9293_DM2F_BADDIR_SPEC))
	    {
		break;
	    }
	    CLRDBERR(&dmve->dmve_error);
	    status = E_DB_OK;
	}

	/*
	** Release the FCB memory.
	*/
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&loc_entry, 1, DM2F_ALLOC, 
				&dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->fc_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->fc_header.flags | DM0L_CLR);

	    status = dm0l_fcreate(dmve->dmve_log_id, dm0l_flags,
				&log_rec->fc_path, log_rec->fc_l_path,
				&log_rec->fc_file, &log_rec->fc_tbl_id,
				log_rec->fc_loc_id, log_rec->fc_cnf_loc_id,
				log_rec->fc_page_size, log_rec->fc_loc_status,
				log_lsn, &lsn, &dmve->dmve_error);

	    if (status != E_DB_OK) 
	    {
            	/*
	     	 * Bug56702: return logfull indication.
	     	 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }

	    /* Release extra reserved log space */
	    dmve_unreserve_space(dmve, 1);
	}

	return (E_DB_OK);
    }

    /*
    ** Release allocated FCB memory if we failed after allocating it.
    */
    if (loc_entry.loc_fcb)
    {
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&loc_entry, 1, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM967B_UNDO_FCREATE);

    return (E_DB_ERROR);
}
