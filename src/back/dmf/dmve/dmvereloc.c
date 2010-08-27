/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <me.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2f.h>
#include    <dm0p.h>

/*
**
**  Name: DMVERELOC.C - The recovery of a relocate table operation.
**
**  Description:
**	This file contains the routine for the recovery of a relocate table
**	operation.
**
**          dmve_reloc - The recovery of a relocate table operation.
**
**
**  History:
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() and dm2f_build_fcb()
**	    calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	25-sep-1991 (mikem) integrated following change: 30-aug-91 (walt & rogerk)
**	    Bug 38668 - Fixed several bugs in routine involving wrong number
**	    and type of arguments to dm2f calls.
**	7-july-92 (jnash)
**	    Add DMF function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	26-nov-1992 (jnash)
**	    Reduced Logging Project: Restructured, added CLR handling
**	    and partial recovery hooks, add file creation during
**	    ROLLFORWARD previously done within dmve_fcreate.
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
**      26-jul-1993 (jnash)
**	    Fix problem where cached pages were not tossed prior
**	    to file read/write.  Also, read/write 16 pages at a time, rather 
**	    than 1.
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	  - Divorced file creation from relocate recovery code.  We now
**	    depend on the rolling forward of the fcreate log records written
**	    in the relocate operation.
**	  - Removed the create_new_file routine.
**	20-sep-1993 (rogerk)
**	    Purge TCB following redo of the relocation operation.  This is
**	    needed so that the next reference to the table by rollforward
**	    reads the new catalog information to find the correct table locs.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <dm0p.h>.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by user configuration,
**	    set db_sync_flag = DI_USER_SYNC_MASK when loading temporary 
**	    files into the new location, force them prior to close.
**      15-apr-1994 (chiku)
**          Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      06-mar-1996 (stial01)
**          dmve_relocate() variable page size support
**          Pass page_size to dm2f_write_file, dm2f_build_fcb, dm2f_read_file
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_toss_pages() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	16-Apr-1999 (jenjo02)
**	    Set LOC_RAW in location if RAW location.
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
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_? functions converted to DB_ERROR *
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

static DB_STATUS 	dmv_rereloc(
				DMVE_CB         *dmve);

static DB_STATUS 	dmv_unreloc(
				DMVE_CB         *dmve);


/*{
** Name: dmve_relocate - The recovery of a relocate table opration.
**
** Description:
**	This function re-execute the relocate table opration using the
**	parameters stored in the log record. The system table changes,
**	the file creation and the file rename operations associated
**	with this relocate table operation appear later in the journal
**	file, so these operations are not performed during recovery.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The relocate table log record.
**	    .dmve_action	    Should only be DMVE_DO.
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
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() and dm2f_build_fcb()
**	    calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	25-sep-1991 (mikem) integrated following change: 01-aug-91 (walt)
**	    Bug 38668 - calls to dm2f_build_fcb were missing the ampersand on
**	    the log_rec->dur_tbl_id parameter.
**	25-sep-1991 (mikem) integrated following change: 30-aug-91 (walt & rogerk)
**	    Bug 38668 - Fixed several bugs in routine:
**	      - Fixed initialization of old-location information, we were
**		mistakenly using the new-location name from the relocate
**		log record rather than the old-location name.
**	      - Fixed arguments to dm2f_build_tcb calls: should pass
**		pointer to table-id structure rather than trying to pass
**		the entire structure on the stack.
**	      - Added checks for return value from dm2f_build_tcb calls.
**	      - Fixed arguments to dm2f_open_file calls: was missing the
**		dcb argument and the db_sync_flag was out of order.
**	      - Fixed arguments to dm2f_flush_file calls: pass location
**		pointer and location count rather than old style FCB ptr.
**	      - Fixed problems with error handling.  When closing files
**		opened for relocate processing: make sure we do not overwrite
**		the error status with the status from the close_file calls.
**		Also make sure to return with the appropriate error status.
**	      - Changed pageno argument passed to dm2f_sense_file to be
**		a i4 instead of DM_PAGENO as defined.  DM_PAGENO is
**		unsigned and dm2f_sense_file may return -1 if the file
**		is empty.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Added page_fmt parameter to call of
**	    dm2f_write_file.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	26-nov-1992 (jnash)
**	    Reduced Logging Project: Restructured, add partial recovery 
**	    hooks and CLR handling.  
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	23-aug-1993 (rogerk)
**	    Added checks for errors in partial recovery.
*/
DB_STATUS
dmve_relocate(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_RELOCATE   	*log_rec = (DM0L_RELOCATE *)dmve_cb->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    i4		error = 0;
    i4		recovery_action;
    bool		oloc_recover;
    bool		nloc_recover;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    for (;;)
    {
	if (log_rec->dur_header.length != sizeof(DM0L_RELOCATE) || 
	    log_rec->dur_header.type != DM0LRELOCATE ||
	    dmve->dmve_action == DMVE_REDO)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** Check recovery requirements for this operation.  If partial
	** recovery is in use, then we may not need to recover one
	** of the two locations.
	*/
	oloc_recover = dmve_location_check(dmve, 
					(i4)log_rec->dur_ocnf_loc_id);
	nloc_recover = dmve_location_check(dmve, 
					(i4)log_rec->dur_ncnf_loc_id);

	/*
	** If neither locations need recovery then the operation can be
	** skipped.
	*/
	if ( (nloc_recover == FALSE) && (oloc_recover == FALSE) )
	    return(E_DB_OK);

	/*
	** If one of the locations needs recovery, but one does not, then
	** this operation cannot be performed.
	*/
	if ( (nloc_recover == FALSE) || (oloc_recover == FALSE) )
	{
	    TRdisplay("dmve_reloc: Partial recovery -- relocate impossible.\n");
	    return(E_DB_ERROR);
	} 

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.  Code within UNDO recognizes the CLR and does not
	** write another CLR log record.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->dur_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_UNDO:
	    status = dmv_unreloc(dmve);
	    break;

	case DMVE_DO:
	    status = dmv_rereloc(dmve);
	    break;
	}

	if (status != E_DB_OK)
	    break;

	return(E_DB_OK);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM960E_DMVE_RELOC);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_rereloc	- ROLLFORWARD Recovery of a relocate table operation
**
** Description:
**
**	This routine is used to perform ROLLFORWARD recovery of 
**	a relocate table.  Recovery is performed on each table 
**	location independently, and may be skipped in partial
**	recovery circumstances.
**
**	It is expected that the result files of the relocate have already
**	been created through the recovery of the FCREATE log records logged 
**	in the original relocate opration.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The relocate table operation log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**
** Outputs:
**	    error	    	    The reason for error status.
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
**	    Restructured for the reduced logging project.  
**	    New recovery protocols have rollforward of a dm0l_create 
**	    create an initalized table, and dm0l_fcreate do nothing.  
**	    This required that we must create the new file in this 
**	    case.  Also, update of new loc_config_id field.  
**      26-jul-1993 (jnash)
**	    Fix recovery problem where cached pages were not tossed prior
**	    to file read/write.  Also, read/write 'svcb_mwrite_blocks' pages 
**	    at a time, rather than 1.
**	23-aug-1993 (rogerk)
**	  - Divorced file creation from relocate recovery code.  We now
**	    depend on the rolling forward of the fcreate log records written
**	    in the relocate operation.
**	  - Changed error handling and added traceback message.
**	20-sep-1993 (rogerk)
**	    Purge TCB following redo of the relocation operation.  This is
**	    needed so that the next reference to the table by rollforward
**	    reads the new catalog information to find the correct table locs.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by user configuration,
**	    set db_sync_flag = DI_USER_SYNC_MASK when loading temporary 
**	    files into the new location, force them prior to close.
**      06-mar-1996 (stial01)
**          variable page size support
**          Pass page_size to dm2f_write_file, dm2f_build_fcb, dm2f_read_file
**	6-Feb-2004 (schka24)
**	    Update build-fcb calls.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	6-Nov-2009 (kschendel) SIR 122757
**	    Align for direct-io as needed.
*/
static DB_STATUS
dmv_rereloc(
DMVE_CB         *dmve)
{
    DMP_MISC 		*loc_cb = (DMP_MISC *) 0;
    DM0L_RELOCATE	*log_rec = (DM0L_RELOCATE *)dmve->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    i4		save_error = 0;
    DB_STATUS		save_status = E_DB_OK;
    DMP_LOCATION        newloc_info;
    DMP_LOCATION        oldloc_info;
    DM_PAGENO		pageno;
    DMPP_PAGE		*page_array;
    i4             loc_count = 1;
    u_i4		old_sync_flag, new_sync_flag;
    u_i4		directio_align;
    i4		i;
    i4		n;
    i4		endpage;
    i4		compare;
    i4		local_error;
    i4		page_count;
    i4		mwrite_blocks;
    i4		allocate_size;
    bool		oldfile_open = FALSE;
    bool		newfile_open = FALSE;
    bool		oldfcb_alloc = FALSE;
    bool		newfcb_alloc = FALSE;
    DB_ERROR		local_dberr;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Get the new and old physical location names.
	** Check location, must be default or valid location.
	*/
	for (i = 0; i < dmve->dmve_dcb_ptr->dcb_ext->ext_count; i++)
	{
	    compare = MEcmp((char *)&log_rec->dur_nlocation, 
		    (char *)&dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].logical, 
		    sizeof(DB_LOC_NAME));
	    if (compare == 0) 
	    {
		/* Location must be able to handle data. */

		if ((dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].flags & 
			DCB_DATA) == 0)
		    compare = -1;
		newloc_info.loc_ext = 
		    &dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i];
		STRUCT_ASSIGN_MACRO(log_rec->dur_nlocation, 
				       newloc_info.loc_name);
		newloc_info.loc_id = log_rec->dur_loc_id;
		newloc_info.loc_config_id = i;
		newloc_info.loc_fcb = 0;		
		newloc_info.loc_status = LOC_VALID;		
		if (dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].flags & DCB_RAW)
		    newloc_info.loc_status |= LOC_RAW;
		break;			    	    	    	    
	    }
	}

	if (compare != 0)
	{
#ifdef xDEBUG
	    TRdisplay("dmve_rereloc: Valid location '%s' not found.\n",
		(char *)&dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].logical);
#endif
	    status = E_DB_ERROR;
	    SETDBERR(&dmve->dmve_error, 0, E_DM001D_BAD_LOCATION_NAME);
	    break;
	}

	/* 
	** Get physical name of old location.
	*/
	for (i = 0; i < dmve->dmve_dcb_ptr->dcb_ext->ext_count; i++)
	{
	    compare = MEcmp((char *)&log_rec->dur_olocation, 
		    (char *)&dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].logical, 
		    sizeof(DB_LOC_NAME));
	    if (compare == 0) 
	    {
		/* Location must be able to handle data. */

		if ((dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].flags & 
			DCB_DATA) == 0)
		    compare = -1;
		oldloc_info.loc_ext = 
		    &dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i];
		STRUCT_ASSIGN_MACRO(log_rec->dur_olocation, 
				       oldloc_info.loc_name);
		oldloc_info.loc_id = log_rec->dur_loc_id;
		oldloc_info.loc_fcb = 0;		
		oldloc_info.loc_status = LOC_VALID;		
		if (dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].flags & DCB_RAW)
		    oldloc_info.loc_status |= LOC_RAW;
		break;			    	    	    	    
	    }
	}

	if (compare != 0)
	{
#ifdef xDEBUG
	    TRdisplay("dmve_rereloc: Valid location '%s' not found\n",
		(char *)&dmve->dmve_dcb_ptr->dcb_ext->ext_entry[i].logical);
#endif
	    SETDBERR(&dmve->dmve_error, 0, E_DM001D_BAD_LOCATION_NAME);
	    break;
	}

	/*
	** Allocate space for Page Buffers for I/O
	*/
	directio_align = dmf_svcb->svcb_directio_align;
	if (dmf_svcb->svcb_rawdb_count > 0 && directio_align < DI_RAWIO_ALIGN)
	    directio_align = DI_RAWIO_ALIGN;
	mwrite_blocks = dmf_svcb->svcb_mwrite_blocks;
	allocate_size = sizeof(DMP_MISC) +
			(mwrite_blocks * log_rec->dur_page_size);
	if (directio_align != 0)
	    allocate_size += directio_align;
	status = dm0m_allocate(allocate_size, DM0M_ZERO, (i4)MISC_CB,
	    (i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT **)&loc_cb, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	page_array = (DMPP_PAGE *)((char *)loc_cb + sizeof(DMP_MISC));
	if (directio_align != 0)
	    page_array = (DMPP_PAGE *) ME_ALIGN_MACRO(page_array, directio_align);
	loc_cb->misc_data = (char*)page_array;

	/*
	** Toss cached pages remaining so they'll be copied too.
	*/
	dm0p_toss_pages(dmve->dmve_dcb_ptr->dcb_id, 
	    log_rec->dur_tbl_id.db_tab_base, dmve->dmve_lk_id, dmve->dmve_log_id, 1);

	/*
	** Build FCBs for file opens below.
	*/
	if (dmve->dmve_dcb_ptr->dcb_sync_flag & DCB_NOSYNC_MASK)
	    old_sync_flag = 0;
	else
	    old_sync_flag = DI_SYNC_MASK;
	if (dmf_svcb->svcb_directio_load)
	    old_sync_flag |= DI_DIRECTIO_MASK;

	new_sync_flag = old_sync_flag | DI_PRIVATE_MASK;

	status = dm2f_build_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&oldloc_info, loc_count,
				log_rec->dur_page_size,
				DM2F_ALLOC, DM2F_TAB, &log_rec->dur_tbl_id, 
				0, 0, (DMP_TABLE_IO *)NULL, 0,
				&dmve->dmve_error);
	if (status != OK)
	    break;
	oldfcb_alloc = TRUE;

	status = dm2f_build_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
				&newloc_info, loc_count,
				log_rec->dur_page_size,
				DM2F_ALLOC, DM2F_TAB, &log_rec->dur_tbl_id, 
				0, 0, (DMP_TABLE_IO *)NULL, 0,
				&dmve->dmve_error);
	if (status != OK)
	    break;
	newfcb_alloc = TRUE;

	/*
	** Open the old and new files.
	*/
	status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
			    &oldloc_info, loc_count,
			    (i4)log_rec->dur_page_size, DM2F_A_WRITE,   
			    old_sync_flag,
			    (DM_OBJECT *)dmve->dmve_dcb_ptr,
			    &dmve->dmve_error);
	if (status != OK)
	    break;
	oldfile_open = TRUE;

	status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
			    &newloc_info, loc_count,
			    (i4)log_rec->dur_page_size, DM2F_A_WRITE, 
			    new_sync_flag,
			    (DM_OBJECT *)dmve->dmve_dcb_ptr,
			    &dmve->dmve_error);
	if (status != OK)
	    break;
	newfile_open = TRUE;

	/* Find out how many pages are allocated in old file. */

	status = dm2f_sense_file(&oldloc_info, loc_count, &log_rec->dur_name,
			     &dmve->dmve_dcb_ptr->dcb_name, 
			     (i4 *)&endpage, &dmve->dmve_error); 
	if (status != OK)
	    break;
	
	pageno = 0;
	status = dm2f_alloc_file(&newloc_info, loc_count, &log_rec->dur_name,
			    &dmve->dmve_dcb_ptr->dcb_name, 
			    (i4)(endpage + 1), 
			    (i4 *)&pageno, &dmve->dmve_error); 
	if (status != OK)
	    break;

	status = dm2f_flush_file(&newloc_info, loc_count, &log_rec->dur_name,
			    &dmve->dmve_dcb_ptr->dcb_name, &dmve->dmve_error);
	if (status != OK)
	    break;

	/*
	** Loop reading pages from the old file and writing them to the
	** new file.  Read/Write 'mwrite_blocks' pages at a time.
	*/
	for (i = 0; i <= endpage; i += page_count)
	{
	    /*
	    ** Determine how many pages to read/write.
	    ** Use mwrite_blocks unless there are not this many pages
	    ** left in the file.
	    */
	    page_count = mwrite_blocks;
	    if (i + page_count > endpage + 1)
		page_count = endpage + 1 - i;

	    /*
	    ** Read old file.
	    */
	    status = dm2f_read_file(&oldloc_info, loc_count, 
		log_rec->dur_page_size,
		&log_rec->dur_name, &dmve->dmve_dcb_ptr->dcb_name,
		&page_count, (i4)i, (char *)page_array, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;

            /*
            ** Write new file.
            */
	    status = dm2f_write_file(&newloc_info, loc_count, 
		log_rec->dur_page_size,
		&log_rec->dur_name, &dmve->dmve_dcb_ptr->dcb_name, 
		&page_count, (i4)i, (char *)page_array, &dmve->dmve_error); 
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Close files used for relocate and deallocate the FCBs.
	*/
	oldfile_open = FALSE;
	status = dm2f_close_file(&oldloc_info, loc_count, 
			     (DM_OBJECT *)dmve->dmve_dcb_ptr, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	oldfcb_alloc = FALSE;
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
			     &oldloc_info, loc_count, DM2F_ALLOC, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Files opened DI_USER_SYNC_MASK require force prior to close.
	*/
	if (new_sync_flag & DI_USER_SYNC_MASK)
	{
	    status = dm2f_force_file(&newloc_info, loc_count,
		&log_rec->dur_name, &dmve->dmve_dcb_ptr->dcb_name, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	}

	newfile_open = FALSE;
	status = dm2f_close_file(&newloc_info, loc_count, 
			     (DM_OBJECT *)dmve->dmve_dcb_ptr, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	newfcb_alloc = FALSE;
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
			     &newloc_info, loc_count, DM2F_ALLOC, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Purge the table descriptor so that our next reference to this
	** table will read the new catalog entries describing the new
	** locations we just moved the data to.
	*/
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &log_rec->dur_tbl_id,
			(DM2T_PURGE | DM2T_NORELUPDAT), dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	dm0m_deallocate((DM_OBJECT **)&loc_cb);
	return(E_DB_OK);
    }

    /*
    ** Error cleanup.
    */

    /*
    ** Close files used for relocate and deallocate the FCBs.
    */
    if (oldfile_open)
    {
	status = dm2f_close_file(&oldloc_info, loc_count, 
			     (DM_OBJECT *)dmve->dmve_dcb_ptr, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (oldfcb_alloc)
    {
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
			     &newloc_info, loc_count, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (newfile_open)
    {
	status = dm2f_close_file(&newloc_info, loc_count, 
			     (DM_OBJECT *)dmve->dmve_dcb_ptr, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (newfcb_alloc)
    {
	status = dm2f_release_fcb(dmve->dmve_lk_id, dmve->dmve_log_id,
			     &oldloc_info, loc_count, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (loc_cb)
	dm0m_deallocate((DM_OBJECT **)&loc_cb);

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9686_REDO_RELOCATE);

    return(E_DB_ERROR);
}

/*{
** Name: dmv_unreloc	- UNDO Recovery of a relocate table operation
**
** Description:
**
**	This routine is used to perform UNDO recovery of 
**	a relocate table operation.  Purge the table descriptor in case
**	someone has already looked in the iirelation catalog and
**	has seen the table used to exist.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The relocate table operation log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**
** Outputs:
**	dmve_cb
**	    error		    The reason for error status.
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
**	    Made into new function for reduced logging project.  Add
**	    CLR handling.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Added traceback error message.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**      15-apr-1994 (chiku)
**          Bug56702: return logfull indication.

*/
static DB_STATUS
dmv_unreloc(
DMVE_CB         *dmve)
{
    DM0L_RELOCATE	*log_rec = (DM0L_RELOCATE *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dur_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Relocate was aborted.  Purge the table descriptor in case
	** someone (presumably this transaction) has done a build_tcb 
	** on the table in its new state.
	*/
	status = dm2t_purge_tcb(dmve->dmve_dcb_ptr, &log_rec->dur_tbl_id,
			(DM2T_PURGE | DM2T_NORELUPDAT), dmve->dmve_log_id, 
			dmve->dmve_lk_id, &dmve->dmve_tran_id, 
			dmve->dmve_db_lockmode, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	     ((log_rec->dur_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->dur_header.flags | DM0L_CLR);

	    status = dm0l_relocate(dmve->dmve_log_id, dm0l_flags, 
			&log_rec->dur_tbl_id, &log_rec->dur_name, 
			&log_rec->dur_owner, &log_rec->dur_olocation, 
			&log_rec->dur_nlocation, 
			log_rec->dur_page_size,
			log_rec->dur_loc_id,
			log_rec->dur_ocnf_loc_id, log_rec->dur_ncnf_loc_id, 
			&log_rec->dur_file, log_lsn, &lsn, &dmve->dmve_error);
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
    SETDBERR(&dmve->dmve_error, 0, E_DM9687_UNDO_RELOCATE);

    return(E_DB_ERROR);
}
