/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <sl.h>
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
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm2f.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>



/**
**
**  Name: DMVESM1RENAME.C - The recovery of a sysmod rename file operation.
**
**  Description:
**	This file contains the routine for the recovery of a sysmod rename
**	file operation.
**
**          dmve_sm1_rename - The recovery of a sysmod rename file operation.
**
**
**  History:
**      11-nov-86 (jennifer)    
**          Created new for Jupiter.
**      22-nov-87 (jennifer)
**          Changed for multiple locations support.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	 3-apr-1989 (rogerk)
**	    Initialized 'closedfile' flag to avoid opening file when not needed.
**	    When opening files just to check for existence, pass flags to
**	    dm2f_open_file to not write an error to the log file if the
**	    file does not exist.
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added rollforward (DMVE_DO) support for sysmod rename.
**	25-sep-1991 (mikem) integrated following change: 22-jul-1991 (bryanp)
**	    B38740: Add UNDO support for sysmod of iidevices.
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename().
**	07-july-1992 (jnash)
**	    Add DMF function prototypes.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	01-dec-1992 (jnash)
**	    Reduced Logging Project.  Add CLR handling.
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
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	19-jan-1995 (cohmi01)
**	    Add new DMP_LOCATION parm to dm2f_rename() calls. Its always
**	    NULL here as system catalogs can not (yet) be on RAW files.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_close_page() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**      15-feb-1999 (nanpr01)
**          Change the hard-coded constants.
**      21-mar-1999 (nanpr01)
**          Support raw locations.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-jul-2003
**          Added include adf.h
**      05-jul-2005 (stial01)
**          dmv_do_sm1_rename: if iidevices, dm2t_open DM2T_A_OPEN_NOACCESS
**          and remove (unecessary) dm0p_close_page (b114797)
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**          
**/

static DB_STATUS dmv_do_sm1_rename(DMVE_CB *dmve_cb);

/*{
** Name: dmve_sm1_rename - The recovery of a sysmod rename file operation.
**
** Description:
**	This function performs the recovery of the sysmod rename file operation.
**	In the case of UNDO, checks the existence of the files oldname,
**      and tempname, if oldname exists, then entire operation
**      was completed undo by renaming rename to newname, and tempname to
**      oldname.  If oldname does not exist, then only acomplished first
**      part so rename tempname to oldname.
**	In the case of DO, rename the oldname  to the tempname and 
**	the newname to rename.  Since this is the system catalog
**      we are dealing with must flush buffer cache and close
**      before rename and open after so it actually reads the 
**      correct file for system catalogs.
**
**	The 'iidevices' core catalog is handled somewhat differently because
**	it may not always be open, and there is no hard-coded TCB pointer for
**	iidevices. Instead, we must open iidevices in order to get the
**	relevant FCB information.
**
**	NOTE:  IF ANYTHING GOES WRONG, DATABASE MAY NOT BE RECOVERABLE.
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
**	11-nov-86 (jennifer)
**          Created for Jupiter.
**      22-nov-87 (jennifer)
**          Changed for multiple locations support.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	 3-apr-1989 (rogerk)
**	    Initialized 'closedfile' flag to avoid opening file when not needed.
**	    When opening files just to check for existence, pass flags to
**	    dm2f_open_file to not write an error to the log file if the
**	    file does not exist. 
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added rollforward (DMVE_DO) support for sysmod rename.
**	25-sep-1991 (mikem) integrated following change: 22-jul-1991 (bryanp)
**	    B38740: Add UNDO support for sysmod of iidevices.
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename().
**	01-dec-1992 (jnash)
**	    Reduced Logging Project.  Add CLR handling.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      28-may-1998 (stial01)
**          dmve_sm1_rename() Support VPS system catalogs. Use correct page
**          size for table opens. Also removed dm0p_close_page call from
**          UNDO of DM0LSM1RENAME. The pages in the buffer cache get flushed
**          before this log record is written. So if the DM0LSM1RENAME is 
**          interrupted, rollback should not need flush any pages.
**          The dm0p_close_page call had to be moved to the undo processing
**          of DM0LSM2CONFIG, where the page size can change and page flushes
**          must be done using the correct page size in the tcb and tbio.
**          Reset config file status.
**          Init loc_status in DMP_LOC structure.
**	6-Jun-2010 (kschendel)
**	    After fixing a typo in tab_name init, discover it's not used at
**	    all, delete it.
*/
DB_STATUS
dmve_sm1_rename(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    i4		sys_err;
    i4		error = E_DB_OK;
    DM0L_SM1_RENAME	*log_rec = (DM0L_SM1_RENAME*)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->sm1_header.lsn; 
    DI_IO		di_file;
    i4		filelength;
    DMP_DCB             *dcb;
    DMP_TCB             *t;
    DMP_FCB             *f;
    DB_TAB_ID           *tbl_id;
    i4             phys_length;
    DM_PATH             *phys_location;
    bool		oldexist;
    bool		tempexist;
    bool		newexist;
    bool		closedfile = FALSE;
    DMP_LOCATION        loc;
    i4             loc_count = 1;
    DMP_LOC_ENTRY       ext;
    DMP_FCB             fcb;
    DMP_RCB		*dev_rcb = 0;
    i4		local_error;
    DB_TAB_TIMESTAMP	timestamp;
    u_i4		db_sync_flag;
    i4		recovery_action;
    i4		dm0l_flags;
    LG_LSN		lsn;
    DM0C_CNF            *config = 0;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    for (;;)
    {
	if (log_rec->sm1_header.length != sizeof(DM0L_SM1_RENAME) || 
	    log_rec->sm1_header.type != DM0LSM1RENAME ||
	    dmve->dmve_action == DMVE_REDO)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	dcb = dmve->dmve_dcb_ptr;
	filelength	= sizeof(log_rec->sm1_oldname);
	tbl_id = &log_rec->sm1_tbl_id;

	fcb.fcb_di_ptr = &di_file;
	fcb. fcb_last_page = 0;
	fcb.fcb_location = (DMP_LOC_ENTRY *) 0;
	fcb.fcb_state = FCB_CLOSED;
	loc.loc_ext = &ext;
	loc.loc_fcb = &fcb;
	/* Location id not important since file name is 
        ** taken from log record. Set to zero. */
	loc.loc_id = 0;
	loc.loc_status = LOC_VALID;

	/*
	** Perform appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->sm1_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_UNDO:

	    /*
	    ** Write CLR if necessary
	    */
	    if ( (dmve->dmve_logging) &&
	         ((log_rec->sm1_header.flags & DM0L_CLR) == 0) )
	    {
		dm0l_flags = (log_rec->sm1_header.flags | DM0L_CLR);

		status = dm0l_sm1_rename(dmve->dmve_log_id, dm0l_flags, 
		    &log_rec->sm1_tbl_id, &log_rec->sm1_tmp_id,
		    &log_rec->sm1_path, 
		    log_rec->sm1_l_path, &log_rec->sm1_oldname, 
		    &log_rec->sm1_tempname, &log_rec->sm1_newname, 
		    &log_rec->sm1_rename, log_lsn, &lsn, 
		    log_rec->sm1_oldpgtype, log_rec->sm1_newpgtype,
		    log_rec->sm1_oldpgsize, log_rec->sm1_newpgsize, 
		    &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    /* XXXX Better error message and continue after logging. */
		    TRdisplay(
			"dmve_sm1_rename: dm0l_sm1_rename error, status: %d, error: %d\n", 
			status, dmve->dmve_error.err_code);
		    /*
		     * Bug56702: return logfull indication.
		     */
		    dmve->dmve_logfull = dmve->dmve_error.err_code;
		    break;
		}
	    }

	    if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	    {
		t = dcb->dcb_rel_tcb_ptr;
		if (tbl_id->db_tab_index == DM_I_RIDX_TAB_ID)
		    t = t->tcb_iq_next;
	    }	    
	    else
		if (tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
		    t = dcb->dcb_att_tcb_ptr;
		else
		    if (tbl_id->db_tab_base == DM_B_INDEX_TAB_ID)
			t = dcb->dcb_idx_tcb_ptr;
	    
	    if (tbl_id->db_tab_base > 4)
	    {
		/*
		** This is iidevices. It may not be open at this time, so
		** open it and remember that we've done so:
		*/
		status = dm2t_open(dcb, tbl_id, DM2T_IX, DM2T_UDIRECT,
				    DM2T_A_WRITE, (i4)0, (i4)20,
				    (i4)0, dmve->dmve_log_id,
				    dmve->dmve_lk_id, (i4)0, (i4)0,
				    dmve->dmve_db_lockmode, &dmve->dmve_tran_id,
				    &timestamp, &dev_rcb, (DML_SCB *)0, 
				    &dmve->dmve_error);
		if (status)
		    break;
		t = dev_rcb->rcb_tcb_ptr;
	    }


	    /* Flush any pages left in buffer cache for this table. */
	    /* FIX ME... there shouldn't be any pages to flush here...
	    ** shouldve been done in dmvesm2.c where page size might change
	    ** FIX ME dm0p_close_page -> force_page can't handle tbio page size
	    ** to be incorrect
	    */
		
	    status = dm0p_close_page(t, dmve->dmve_lk_id, dmve->dmve_log_id,
					DM0P_CLOSE, &dmve->dmve_error);

	    /* Close the file associated with this table. */
	
	    f = t->tcb_table_io.tbio_location_array[0].loc_fcb;
	    if (f->fcb_state == FCB_OPEN)
	    {
		status = dm2f_close_file(t->tcb_table_io.tbio_location_array,
                        t->tcb_table_io.tbio_loc_count, (DM_OBJECT *)t, 
			&dmve->dmve_error);
		if (status != OK)
		    break;
		closedfile = TRUE;
	    }

	    /* Reset last page value since size of files are different. */

	    f->fcb_last_page = 0;

	    /* Assume entire operation ws done. */

	    oldexist = TRUE;
	    tempexist = TRUE;
	    newexist = TRUE;

	    
	    MEcopy((char *)&log_rec->sm1_path, 
		sizeof(DM_PATH), (char *)&ext.physical);
	    ext.phys_length = log_rec->sm1_l_path;
	    MEcopy((char *)&log_rec->sm1_oldname, 
		sizeof(DM_FILE), (char *)&fcb.fcb_filename);
	    fcb.fcb_namelength = filelength;
	    fcb.fcb_state = FCB_CLOSED;

	    /*  Check the existence of the file. */

	    if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
		db_sync_flag = (u_i4) 0;
	    else
		db_sync_flag = (u_i4) DI_SYNC_MASK;

	    status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
		&loc, loc_count, 
		(i4)DM_PG_SIZE, DM2F_E_READ, db_sync_flag, 
		(DM_OBJECT *)t, &dmve->dmve_error);

	    if (status != E_DB_OK && dmve->dmve_error.err_code == E_DM9291_DM2F_FILENOTFOUND)
	    {
		oldexist = FALSE;
	    }
	    else if (status != E_DB_OK)
	    {
		break;
	    }	
	    if (status == OK)
	    {
		oldexist = TRUE;
		status = dm2f_close_file(&loc,loc_count, (DM_OBJECT *)t, &dmve->dmve_error);
	    }

	    MEcopy((char *)&log_rec->sm1_tempname, 
		sizeof(DM_FILE), (char *)&fcb.fcb_filename);
	    fcb.fcb_state = FCB_CLOSED;


	    status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
		&loc, loc_count, 
		(i4)DM_PG_SIZE, DM2F_E_READ, db_sync_flag, 0, &dmve->dmve_error);

	    if (status != E_DB_OK && dmve->dmve_error.err_code == E_DM9291_DM2F_FILENOTFOUND)
	    {
		tempexist = FALSE;
	    }
	    else if (status != OK)
	    {
		break;
	    }
	    if (status == OK)
	    {
		tempexist = TRUE;
		status = dm2f_close_file(&loc,loc_count, (DM_OBJECT *)t,  
				&dmve->dmve_error);
	    }


	    MEcopy((char *)&log_rec->sm1_newname, 
		sizeof(DM_FILE), (char *)&fcb.fcb_filename);
	    fcb.fcb_state = FCB_CLOSED;
	    status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
		&loc, loc_count, 
		(i4)DM_PG_SIZE, DM2F_E_READ, db_sync_flag, 0, &dmve->dmve_error);

	    if (status != E_DB_OK && dmve->dmve_error.err_code == E_DM9291_DM2F_FILENOTFOUND)
	    {
		newexist = FALSE;
	    }
	    else if (status != OK)
	    {
		break;
	    }
	    if (status == OK)
	    {
		newexist = TRUE;
		status = dm2f_close_file(&loc, loc_count, (DM_OBJECT *)t, 
				&dmve->dmve_error);
	    }


	    if (newexist == FALSE)
	    {
		    /* Must undo entire operation. */
		    /* Rename the rename to the newname. */

		    status = dm2f_rename(dmve->dmve_lk_id, dmve->dmve_log_id,
			&log_rec->sm1_path, log_rec->sm1_l_path, 
			&log_rec->sm1_rename, filelength, &log_rec->sm1_newname,
			filelength, &dmve->dmve_dcb_ptr->dcb_name,
			&dmve->dmve_error);
		    if (status != OK)
			break;
		    oldexist = FALSE;
	    }
	    if (oldexist == FALSE)
	    {
		    /* Must undo first half operation. */
		    /* Rename the tempname to the oldname. */

		    status = dm2f_rename(dmve->dmve_lk_id, dmve->dmve_log_id,
			&log_rec->sm1_path, log_rec->sm1_l_path, 
			&log_rec->sm1_tempname, filelength, 
			&log_rec->sm1_oldname, filelength, 
			&dmve->dmve_dcb_ptr->dcb_name,
			&dmve->dmve_error);
		    if (status != OK)
			break;
		    oldexist = TRUE;
		    tempexist = FALSE;

	    }
	    /* MUST use correct (old) page size for this open */
	    status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
		t->tcb_table_io.tbio_location_array, 
	        t->tcb_table_io.tbio_loc_count, 
		(i4)log_rec->sm1_oldpgsize, 
		DM2F_A_WRITE, db_sync_flag, 0, &dmve->dmve_error);
	    if (status != OK)
		break;
            closedfile = FALSE;
	    /*
	    ** If iidevices was opened, close it:
	    */
	    if (dev_rcb)
	    {
		status = dm2t_close(dev_rcb, DM2T_NOPURGE, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    dev_rcb = 0;
		    break;
		}
	    }

	    if (dcb->dcb_status & (DCB_S_RECOVER | DCB_S_ONLINE_RCP))
	    {
		/*  Open the configuration file. */
		status = dm0c_open(dcb, DM0C_PARTIAL, dmve->dmve_lk_id, 
				    &config, &dmve->dmve_error);
		if (status != E_DB_OK)
		    break;

		config->cnf_dsc->dsc_status &= ~DSC_SM_INCONSISTENT;

		status = dm0c_close(config, DM0C_UPDATE, &dmve->dmve_error);
		if (status != E_DB_OK)
		    break;
	    }

	    status = E_DB_OK;
	    CLRDBERR(&dmve->dmve_error);
	    break;

	case DMVE_DO:
	    status = dmv_do_sm1_rename(dmve);
	    break;    

	} /* end switch. */
	if (status != E_DB_OK)
	    break;
	return(E_DB_OK);
 
   } /* end for. */

    if (closedfile == TRUE)
    {
	if (dmve->dmve_dcb_ptr->dcb_sync_flag & DCB_NOSYNC_MASK)
	    db_sync_flag = (u_i4) 0;
	else
	    db_sync_flag = (u_i4) DI_SYNC_MASK;

	/* MUST use correct (new) page size */
	local_status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
	    t->tcb_table_io.tbio_location_array,
	    t->tcb_table_io.tbio_loc_count,
	    (i4)log_rec->sm1_newpgsize, 
	    DM2F_A_WRITE, db_sync_flag, 0, &local_dberr);
    }
    if (dev_rcb)
    {
	local_status = dm2t_close(dev_rcb, DM2T_NOPURGE, &local_dberr);
    }
    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9611_DMVE_SM1RENAME);
    }
    return(status);
}

/*
** Name: dmv_do_sm1_rename	- rollforward of sysmod rename operation
**
** Description:
**	This routine performs the sysmod catalog rename operation for
**	rollforward. Since the core catalogs are special opjects, a special
**	log record is written when an old core catalog is replaced with the
**	newly modified core catalog.
**
**	During normal processing dm2u_sysmod works as follows:
**		1) Build a temporary table, named 'iirtemp', and load it
**		   with the records from the core catalog being modified. If
**		   the core catalog is iirelation, then also build an index
**		   on iirtemp, called 'iiridxtemp'.
**		2) Now replace the current core catalog with the new one:
**			a) rename the old catalog to its 'sysmod rename' name.
**			b) rename the new catalog to the old one.
**			c) rename the sysmod rename catalog to the iirtemp name.
**		   The first 2 steps are logged with the DM0L_SM1_RENAME
**		   log record, while the third is just a simple DM0L_RENAME.
**		3) If iirelation is being modified, perform step (2) again for
**		   swapping iirel_idx with iiridxtemp.
**		4) Finally, update the config file and in-memory TCB information
**	 	   for the core catalog being modified. This operation is
**		   logged with the DM0L_SM2_CONFIG log record.
**
**	This routine is called to perform DMVE_DO processing for step (2) above.
**	DO processing is performed as follows:
**
**		a) Get tcb pointer. Most tables that can be sysmod'd are
**		   described in the dcb, so just use the tcb stored there.
**		   If the table is 'iidevices', then it isn't open, so open it.
**		b) Flush any pages in the buffer cache for this core catalog.
**		c) low-level close this core catalog file (dm2f_close).
**		   (remember that we've done so for error recovery purposes)
**		d) Rename the old core catalog to the tempname
**		e) Rename the new core catalog to the rename
**		f) low-level open the new core catalog (dm2f_open)
**		   (remember that we've done so for error recovery purposes)
**
**	At the end of this, the old core catalog has been swapped with the
**	new one, and the core catalog in-memory TCB now references the new
**	catalog's physical file.
**
**	The complex protocol described above ensures that the buffer manager
**	will actually read the correct file from now on for all the core
**	catalog pages.
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
**
** History:
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    Written as part of bugfix for B38527.
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename().
*/
static DB_STATUS
dmv_do_sm1_rename( 
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DM0L_SM1_RENAME	*log_rec = (DM0L_SM1_RENAME*)dmve_cb->dmve_log_rec;
    DI_IO		di_file;
    i4		filelength;
    DMP_DCB             *dcb;
    DMP_TCB             *t;
    DMP_FCB             *f;
    DB_TAB_ID           *tbl_id;
    i4             phys_length;
    DM_PATH             *phys_location;
    bool		file_closed = FALSE;
    DMP_LOCATION        loc;
    i4             loc_count = 1;
    DMP_LOC_ENTRY       ext;
    DMP_FCB             fcb;
    DMP_RCB		*rcb = 0;
    DB_TAB_TIMESTAMP	timestamp;
    i4		error = E_DB_OK;
    i4		local_error = E_DB_OK;
    u_i4		db_sync_flag;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);

	
    for (;;)
    {
	tbl_id = &log_rec->sm1_tbl_id;
	dcb = dmve->dmve_dcb_ptr;
	fcb.fcb_di_ptr = &di_file;

	/*
	** a) Get tcb pointer. Most tables that can be sysmod'd are described
	**    in the dcb, so just use the tcb stored there.
	** 
	**    If the table is 'iidevices', then it isn't open, so open it.
	*/
	t = 0;
	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	{
	    t = dcb->dcb_rel_tcb_ptr;
	    if (tbl_id->db_tab_index == DM_I_RIDX_TAB_ID)
		t = t->tcb_iq_next;
	}	    
	else if (tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
	    t = dcb->dcb_att_tcb_ptr;
	else if (tbl_id->db_tab_base == DM_B_INDEX_TAB_ID)
	    t = dcb->dcb_idx_tcb_ptr;
	else
	{
	    /*
	    ** open iidevices
	    **
	    ** Specify DM2T_A_OPEN_NOACCESS so that iidevices is not reopened
	    ** (iidevices should have been close/purged in
	    ** dmve_sm0_closepurge() for SM0CLOSEPURG processing)
	    ** and since catalog updates have been made for this sysmod,
	    ** the iirelation entry may not match the file. (page size ...)
	    */
	    status = dm2t_open(dcb, tbl_id, DM2T_IX, DM2T_UDIRECT,
				DM2T_A_OPEN_NOACCESS, (i4)0, (i4)20,
				(i4)0, dmve->dmve_log_id,
				dmve->dmve_lk_id, (i4)0, (i4)0,
				dmve->dmve_db_lockmode, &dmve->dmve_tran_id,
				&timestamp, &rcb, (DML_SCB *)NULL, &dmve->dmve_error);
	    if (status)
		break;
	    t = rcb->rcb_tcb_ptr;
	}

	/*
	** b) Flush any pages in the buffer cache for this core catalog.
	**
	** Skip this for iidevices since it should have been close/purged
	** in dmve_sm0_closepurge() for SM0CLOSEPURG processing
	*/
	if (tbl_id->db_tab_base != DM_B_DEVICE_TAB_ID)
	    status = dm0p_close_page(t, dmve->dmve_lk_id, dmve->dmve_log_id,
				DM0P_CLOSE, &dmve->dmve_error);
	if (status)
	    break;

	/*
	** c) low-level close this core catalog file (dm2f_close).
	**       (remember that we've done so for error recovery purposes)
	*/
	f = t->tcb_table_io.tbio_location_array[0].loc_fcb;
	phys_location = (DM_PATH *)&log_rec->sm1_path;
	phys_length = log_rec->sm1_l_path;
	if (f->fcb_state == FCB_OPEN)
	{
	    status = dm2f_close_file( t->tcb_table_io.tbio_location_array, 
		t->tcb_table_io.tbio_loc_count, (DM_OBJECT *)f, &dmve->dmve_error);
	    if (status)
		break;
	    file_closed = TRUE;
	}

	/*
	** d) Rename the old core catalog to the tempname
	*/
	status = dm2f_rename(dmve->dmve_lk_id, dmve->dmve_log_id,
			    phys_location, phys_length,
			    &log_rec->sm1_oldname, (i4)sizeof(DM_FILE),
			    &log_rec->sm1_tempname, (i4)sizeof(DM_FILE),
			    &dcb->dcb_name, &dmve->dmve_error);
	if (status)
	    break;

	/*
	** e) Rename the new core catalog to the rename
	*/
	status = dm2f_rename(dmve->dmve_lk_id, dmve->dmve_log_id,
			    phys_location, phys_length,
			    &log_rec->sm1_newname, (i4)sizeof(DM_FILE),
			    &log_rec->sm1_rename, (i4)sizeof(DM_FILE),
			    &dcb->dcb_name, &dmve->dmve_error);
	if (status)
	    break;

	/*
	** f) low-level open the new core catalog (dm2f_open)
	**    (remember that we've done so for error recovery purposes)
	*/
	status = dm2f_open_file(dmve->dmve_lk_id, dmve->dmve_log_id,
			t->tcb_table_io.tbio_location_array,
			t->tcb_table_io.tbio_loc_count, 
			(i4)log_rec->sm1_newpgsize,
			DM2F_A_WRITE, (i4)DI_SYNC_MASK,
			(DM_OBJECT *)f, &dmve->dmve_error);
	if (status)
	    break;
	file_closed = FALSE;
	
	if (rcb)
	{
	    /*
	    ** If iidevices was opened, close it:
	    */
	    status = dm2t_close(rcb, DM2T_NOPURGE, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		rcb = 0;
		break;
	    }
	}

	return (E_DB_OK);
    }

    /*
    ** Error handling & cleanup. If we low-level closed a file, re-open it.
    ** If we opened iidevices, close it. Log the error if necessary and
    ** return.
    */

    if (file_closed)
    {
	_VOID_ dm2f_open_file( dmve->dmve_lk_id, dmve->dmve_log_id,
			t->tcb_table_io.tbio_location_array,
			t->tcb_table_io.tbio_loc_count, 
			(i4)log_rec->sm1_newpgsize,
			DM2F_A_WRITE, (u_i4)DI_SYNC_MASK,
			(DM_OBJECT *)f, &local_dberr);
    }

    /*
    ** If iidevices was left open, close it:
    */
    if (rcb)
	_VOID_ dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);

    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9611_DMVE_SM1RENAME);
    }

    return (status);
}
