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
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2d.h>

/**
**
**  Name: DMVELOC.C - The recovery of location-related operations.
**
**  Description:
**	This file contains routines for the recovery of location-related
**	operations.
**
**          dmve_location     - The recovery of an add location operation.
**	    dmve_extent_alter - Recover alteration of an extent alteration.
**		dmve_del_location - The recovery of a delete location operation.
**
**  History:    
**      26-mar-87 (jennifer)    
**          Created new for Jupiter.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	10-jul-89 (rogerk)
**	    When backing out location extend operation, don't request config
**	    file lock.
**	17-may-90 (bryanp)
**	    After updating config file, make backup copy of file.
**	7-july-92 (jnash)
**	    Add DMF function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	28-nov-1992 (jnash)
**	    Reduced logging project. Add CLR handling.
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
**          Remove location entry from DCB during UNDO.
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	15-sep-93 (jrb)
**	    Added dmve_ext_alter for MLSort project.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      05-apr-1995 (stial01)
**          dmve_location() when redoing add location operation, and looking
**          for specific location name, also check location usage.
**	12-mar-1999 (nanpr01)
**	    Fix up undo and redo recovery of location for raw location
**	    support.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-jul-2003
**          Added include adf.h
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	29-Apr-2004 (gorvi01)
**		Added function dmve_del_location for recovery of
**		delete location operation
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_?, dm2d_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**/



/*{
** Name: dmve_location - The recovery of an add location operation.
**
** Description:
**	This function performs the recovery of the add location 
**      update operation.  This is to update the config file 
**      with any new locations added with ACEESSDB.
**	In the case of UNDO, reads the config file, if update has been
**      made, then it undoes it otherwise it just closes and continues.
**      In case of DO, reads the config file, if there ignores, otherwise
**      adds.
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
**	16-mar-87 (jennifer)
**          Created for Jupiter.
**	10-jul-89 (rogerk)
**	    When backing out location extend operation, don't request config
**	    file lock, as the lock may be held by the transaction being backed
**	    out.  The lock is not needed since an exclusive DB lock is requried
**	    to add a location anyway.
**	25-jul-89 (sandyh)
**	    Recovering a DB from checkpoint that occured before the extend
**	    happened results in that location being unusable in update ops
**	    since the DCB does not contain the new location as valid for this
**	    DB. bug 5019
**	17-may-90 (bryanp)
**	    After updating config file, make backup copy of file. Note that
**	    we do not need to make a backup in the DO case, since rollforward
**	    will make a backup of the config file at the successful end of
**	    rolling forward the DB. 
**	28-nov-1992 (jnash)
**	    Reduced logging project. Add CLR handling. 
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      26-jul-1993 (jnash)
**          Remove location entry from DCB during UNDO.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      05-apr-1995 (stial01)
**          When redoing add location operation, and looking for specific 
**          location name, also check location usage.
*/
DB_STATUS
dmve_location(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    i4		error = E_DB_OK, local_error = E_DB_OK;
    DM0L_LOCATION	*log_rec = (DM0L_LOCATION *)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->loc_header.lsn; 
    DMP_DCB             *dcb;
    DM0C_CNF		*config = 0;
    DM0C_CNF		*cnf = 0;
    i4             lock_list;
    DMP_LOC_ENTRY       *l;
    i4	        loc_count;
    i4	        i;
    i4	        recovery_action;
    i4	        dm0l_flags;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	if (log_rec->loc_header.length != sizeof(DM0L_LOCATION) || 
	    log_rec->loc_header.type != DM0LLOCATION)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	dcb = dmve->dmve_dcb_ptr;
	lock_list = dmve->dmve_lk_id; 

	recovery_action = dmve->dmve_action;
	if (log_rec->loc_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_UNDO:

	    /*
	    ** Write CLR if necessary
	    */
	    if ((dmve->dmve_logging) &&
	        ((log_rec->loc_header.flags & DM0L_CLR) == 0))
	    {
		dm0l_flags = log_rec->loc_header.flags | DM0L_CLR;

		status = dm0l_location(dmve->dmve_log_id, dm0l_flags, 
		    log_rec->loc_type, &log_rec->loc_name, 
		    log_rec->loc_l_extent, &log_rec->loc_extent,
		    log_rec->loc_raw_start, log_rec->loc_raw_blocks,
		    log_rec->loc_raw_total_blocks,
		    log_lsn, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    /* XXXX Better error message and continue after logging. */
		    TRdisplay(
		 "dmve_location: dm0l_location error, status: %d, error: %d\n", 
			status, dmve->dmve_error.err_code);
		    /*
		     * Bug56702: return logfull indication.
		     */
		    dmve->dmve_logfull = dmve->dmve_error.err_code;
		    break;
		}
	    }

	    /* 
	    ** Remove the location entry from the DCB, if it exists.
	    */
	    if (dcb->dcb_ext && dcb->dcb_ext->ext_count)
		loc_count = dcb->dcb_ext->ext_count;
	    else
	  	loc_count = 0;

	    for (i = 0; i < loc_count; i++)
	    {	    
		l = &dcb->dcb_ext->ext_entry[i];
		if (MEcmp((char *)&l->logical, (char *)&log_rec->loc_name,
		    sizeof(DB_LOC_NAME)) == 0)
			break;
	    }
	    if (i >= loc_count)
	    {
		/* No entry found, nothing to remove. */
		;
#ifdef xDEBUG
		TRdisplay(
		    "dmve_location: UNDO location '%s' not found in DCB.\n",
		    (char *)&log_rec->loc_name);
#endif
	    }
	    else if (i == (loc_count - 1))
	    {
		/* This is last entry, easy. */
		dcb->dcb_ext->ext_entry[i].phys_length = 0;
	        dcb->dcb_ext->ext_count--;
	    }		
	    else
	    {	    
		/* In middle of list, compress. */
		loc_count--;
		MEcopy((char *)&dcb->dcb_ext->ext_entry[i+1].logical,
		   sizeof(DMP_LOC_ENTRY) * (loc_count-i),
		   (char *)&dcb->dcb_ext->ext_entry[i].logical);

		/* Mark the end of list. */

		dcb->dcb_ext->ext_entry[loc_count].phys_length = 0;
	        dcb->dcb_ext->ext_count--;
	    }

	    /*  
	    ** Open the configuration file. 
	    */
            status = dm0c_open(dcb, DM0C_NOLOCK, lock_list, 
                                       &cnf, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = cnf;

	    /* 
	    ** Delete this entry from the list. 
	    */
	    loc_count = cnf->cnf_dsc->dsc_ext_count;
	    for (i = 0; i < loc_count; i++)
	    {	    
		l = &cnf->cnf_ext[i].ext_location;
		if (MEcmp((char *)&l->logical, (char *)&log_rec->loc_name,
		    sizeof(DB_LOC_NAME)) == 0)
			break;
	    }
	    if (i >= loc_count)
	    {
		/* No entry found, nothing to undo. */
		break;
	    }

	    if (i == (loc_count - 1))
	    {
		/* This is last entry, easy. */
		cnf->cnf_ext[i].length = 0;
		cnf->cnf_ext[i].type = 0;
	        cnf->cnf_dsc->dsc_ext_count--;
		
	    }		
	    else
	    {	    
		/* In middle of list, compress. */
		loc_count--;
		MEcopy((char *)&cnf->cnf_ext[i+1].ext_location.logical,
			       sizeof(DMP_LOC_ENTRY)*(loc_count-i),
                               (char *)&cnf->cnf_ext[i].ext_location.logical);

		/* Mark the end of list. */

		cnf->cnf_ext[loc_count].length = 0;
		cnf->cnf_ext[loc_count].type = 0;
	        cnf->cnf_dsc->dsc_ext_count--;

	    }
	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = 0;
	    break;    

	case DMVE_REDO:
	    break;

	case DMVE_DO:

	    /*  Open the configuration file. */

	    l = dcb->dcb_ext->ext_entry;
	    loc_count = dcb->dcb_ext->ext_count;
	    for (i = 0; i < loc_count; i++, l++)
	    if ((MEcmp((char *)&l->logical, (char *)&log_rec->loc_name,
		sizeof(DB_LOC_NAME)) == 0)
		&& (l->flags == log_rec->loc_type))
		break;
	    if (i < loc_count)
	    {
		/* Found this entry, return error. */
		SETDBERR(&dmve->dmve_error, 0, E_DM007E_LOCATION_EXISTS);
		break;
	    }
		    
            status = dm0c_open(dcb, 0, lock_list, 
                                       &cnf, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = cnf;

	    /* Check if there is room. */

            if (cnf->cnf_free_bytes	< sizeof(DM0C_EXT))
	    {
		status = dm0c_extend(cnf, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM0071_LOCATIONS_TOO_MANY);
		    break;
		}
	    }


	    i = cnf->cnf_dsc->dsc_ext_count++;
	    cnf->cnf_ext[i].length = sizeof(DM0C_EXT);
	    cnf->cnf_ext[i].type = DM0C_T_EXT;
	    MEcopy((char *)&log_rec->loc_name, sizeof(DB_LOC_NAME),
                               (char *)&cnf->cnf_ext[i].ext_location.logical);
	    MEcopy((char *)&log_rec->loc_extent,  sizeof(DM_FILENAME),
			       (char *)&cnf->cnf_ext[i].ext_location.physical);
	    cnf->cnf_ext[i].ext_location.flags = log_rec->loc_type;
	    cnf->cnf_ext[i].ext_location.phys_length = log_rec->loc_l_extent;
	    cnf->cnf_ext[i].ext_location.raw_start
					  = log_rec->loc_raw_start;
	    cnf->cnf_ext[i].ext_location.raw_blocks 
					  = log_rec->loc_raw_blocks;
	    cnf->cnf_ext[i].ext_location.raw_total_blocks 
					  = log_rec->loc_raw_total_blocks;
	    cnf->cnf_ext[i+1].length = 0;
	    cnf->cnf_ext[i+1].type = 0;

	    /* Add new location info to DCB so RFP will be able to use it. */

	    dcb->dcb_ext->ext_count = cnf->cnf_dsc->dsc_ext_count;
	    STRUCT_ASSIGN_MACRO(cnf->cnf_ext[i].ext_location,
				dcb->dcb_ext->ext_entry[i]);

	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, DM0C_UPDATE, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = 0;
	    break;    

	} /* end switch. */
	if (config != 0)
	{
	    (void) dm0c_close(cnf, 0, &local_dberr);
	}	
	if (status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    break;
	}
	return(E_DB_OK);

    } /* end for. */
    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9617_DMVE_LOCATION);
    }
    return(status);

}

/*{
** Name: dmve_ext_alter - The recovery of an extent alteration.
**
** Description:
**	This function performs the recovery of the extent alteration
**	done when iiqef_alter_extension (an internal procedure) changes
**	bits in the config file to alter an extent type.
**
**	Currently the only operation supported is changing a defaultable
**	work location (DU_EXT_WORK) to an auxiliary work location (DU_EXT_AWORK)
**	and vice-versa.
**
**	For UNDO, we read the config file and if the update has been made
**	we reverse it, otherwise we just close it and continue.  For DO, we
**	read the config file and if the update was done we ignore, otherwise
**	we make the update.
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
**	15-sep-93 (jrb)
**          Created for MLSort project.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (nanpr01)
**	    Get rid of compiler warning message.
*/
DB_STATUS
dmve_ext_alter(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    i4		error = E_DB_OK, local_error = E_DB_OK;
    DM0L_EXT_ALTER	*log_rec = (DM0L_EXT_ALTER *)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->ext_header.lsn; 
    DMP_DCB             *dcb;
    DM0C_CNF		*config = 0;
    DM0C_CNF		*cnf = 0;
    i4             lock_list;
    DMP_LOC_ENTRY       *l;
    i4	        loc_count;
    i4	        i;
    i4	        recovery_action;
    i4	        dm0l_flags;
    DM2D_ALTER_INFO	dm2d;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	if (log_rec->ext_header.length != sizeof(DM0L_EXT_ALTER) || 
	    log_rec->ext_header.type != DM0LEXTALTER)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	dcb = dmve->dmve_dcb_ptr;
	lock_list = dmve->dmve_lk_id; 

	recovery_action = dmve->dmve_action;
	if (log_rec->ext_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	  case DMVE_UNDO:

	    /*
	    ** Write CLR if necessary
	    */
	    if ((dmve->dmve_logging) &&
	        ((log_rec->ext_header.flags & DM0L_CLR) == 0))
	    {
		dm0l_flags = log_rec->ext_header.flags | DM0L_CLR;

		status = dm0l_ext_alter(dmve->dmve_log_id, dm0l_flags, 
		    log_rec->ext_otype, log_rec->ext_ntype,
		    &log_rec->ext_lname, log_lsn, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    /* XXXX Better error message and continue after logging. */
		    TRdisplay(
		 "dmve_ext_alter: dm0l_ext_alter error, status: %d, error: %d\n", 
			status, dmve->dmve_error.err_code);
		    /*
		     * Bug56702: return logfull indication.
		     */
		    dmve->dmve_logfull = dmve->dmve_error.err_code;
		    break;
		}
	    }

	    /*  
	    ** Open the configuration file. 
	    */
            status = dm0c_open(dcb, DM0C_NOLOCK, lock_list, &cnf, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = cnf;

	    /* 
	    ** Change bits for location named in the log_rec
	    */
	    loc_count = cnf->cnf_dsc->dsc_ext_count;
	    for (i = 0; i < loc_count; i++)
	    {	    
		l = &cnf->cnf_ext[i].ext_location;
		if (MEcmp((char *)&l->logical, (char *)&log_rec->ext_lname,
		    sizeof(DB_LOC_NAME)) == 0)
			break;
	    }
	    if (i >= loc_count)
	    {
		/* No entry found; this is bad... */
		TRdisplay(
	       "dmve_ext_alter: UNDO location '%s' not found in config file.\n",
		    (char *)&log_rec->ext_lname);

		SETDBERR(&dmve->dmve_error, 0, E_DM92A0_DMVE_ALTER_UNDO);
		status = E_DB_ERROR;
		break;
	    }

	    /* Undo changes to bits of the current location if necessary */
	    if (l->flags & log_rec->ext_ntype)
	    {
		l->flags &= ~(log_rec->ext_ntype);
		l->flags |= log_rec->ext_otype;
	    }

	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = 0;
	    break;    

	case DMVE_REDO:
	    break;

	case DMVE_DO:
	    /* Fill in dm2d block in preparation for calling dm2d_alter_db
	    ** to rollforward the extent alteration
	    **
	    ** lock_no_wait doesn't matter because we won't do any logging
	    ** or locking in dm2d_alter_db when calling it from here.
	    */
	    dm2d.lock_list	= lock_list;
	    dm2d.lock_no_wait	= 1;
	    dm2d.logging	= 0;
	    dm2d.locking	= 0;
	    dm2d.name		= &dcb->dcb_name;
	    dm2d.db_loc		= (char *) &dcb->dcb_location.physical;
	    dm2d.l_db_loc	= dcb->dcb_location.phys_length;
	    dm2d.location_name  = &log_rec->ext_lname;
	    dm2d.alter_op	= DM2D_EXT_ALTER;

	    dm2d.alter_info.ext_info.drop_loc_type = log_rec->ext_otype; 
	    dm2d.alter_info.ext_info.add_loc_type  = log_rec->ext_ntype;

	    status = dm2d_alter_db(&dm2d, &dmve->dmve_error);

	    break;    
	} /* end switch. */

	if (config != 0)
	{
	    (void) dm0c_close(cnf, 0, &local_dberr);
	}	
	if (status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    break;
	}
	return(E_DB_OK);

    } /* end for. */

    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9617_DMVE_LOCATION);
    }
    return(status);

}
/*{
** Name: dmve_del_location - The recovery of an delete location operation.
**
** Description:
**	This function performs the recovery of the delete location 
**      update operation.  This is to update the config file 
**      with any new locations deleted by unextenddb.
**	In the case of UNDO, reads the config file, if update has been
**      made, then it adds it otherwise it just closes and continues.
**      In case of DO, reads the config file, deletes and then closes.
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
**	29-apr-2004 (gorvi01)
**          Created for UNEXTENDDB.
**	09-Nov-2004 (jenjo02)
**	    Relocated misplaced logging of CLR from DMVE_DO 
**	    to DMVE_UNDO
*/
DB_STATUS
dmve_del_location(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    i4		error = E_DB_OK, local_error = E_DB_OK;
    DM0L_DEL_LOCATION	*log_rec = (DM0L_DEL_LOCATION *)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->loc_header.lsn; 
    DMP_DCB             *dcb;
    DM0C_CNF		*config = 0;
    DM0C_CNF		*cnf = 0;
    i4             lock_list;
    DMP_LOC_ENTRY       *l;
    i4	        loc_count;
    i4	        i;
    i4	        recovery_action;
    i4	        dm0l_flags;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	if (log_rec->loc_header.length != sizeof(DM0L_DEL_LOCATION) || 
	    log_rec->loc_header.type != DM0LDELLOCATION)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	dcb = dmve->dmve_dcb_ptr;
	lock_list = dmve->dmve_lk_id; 

	recovery_action = dmve->dmve_action;
	if (log_rec->loc_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{

	   case DMVE_REDO:
	     break;

    	   case DMVE_DO:

	    /* 
	    ** Remove the location entry from the DCB, if it exists.
	    */
	    if (dcb->dcb_ext && dcb->dcb_ext->ext_count)
		loc_count = dcb->dcb_ext->ext_count;
	    else
	  	loc_count = 0;

	    for (i = 0; i < loc_count; i++)
	    {	    
		l = &dcb->dcb_ext->ext_entry[i];
		if (MEcmp((char *)&l->logical, (char *)&log_rec->loc_name,
		    sizeof(DB_LOC_NAME)) == 0)
			break;
	    }
	    if (i >= loc_count)
	    {
		/* No entry found, nothing to remove. */
		;
#ifdef xDEBUG
		TRdisplay(
		    "dmve_del_location: UNDO location '%s' not found in DCB.\n",
		    (char *)&log_rec->loc_name);
#endif
	    }
	    else if (i == (loc_count - 1))
	    {
		/* This is last entry, easy. */
		dcb->dcb_ext->ext_entry[i].phys_length = 0;
	        dcb->dcb_ext->ext_count--;
	    }		
	    else
	    {	    
		/* In middle of list, compress. */
		loc_count--;
		MEcopy((char *)&dcb->dcb_ext->ext_entry[i+1].logical,
		   sizeof(DMP_LOC_ENTRY) * (loc_count-i),
		   (char *)&dcb->dcb_ext->ext_entry[i].logical);

		/* Mark the end of list. */

		dcb->dcb_ext->ext_entry[loc_count].phys_length = 0;
	        dcb->dcb_ext->ext_count--;
	    }

	    /*  
	    ** Open the configuration file. 
	    */
            status = dm0c_open(dcb, DM0C_NOLOCK, lock_list, 
                                       &cnf, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = cnf;

	    /* 
	    ** Delete this entry from the list. 
	    */
	    loc_count = cnf->cnf_dsc->dsc_ext_count;
	    for (i = 0; i < loc_count; i++)
	    {	    
		l = &cnf->cnf_ext[i].ext_location;
		if (MEcmp((char *)&l->logical, (char *)&log_rec->loc_name,
		    sizeof(DB_LOC_NAME)) == 0)
			break;
	    }
	    if (i >= loc_count)
	    {
		/* No entry found, nothing to undo. */
		break;
	    }

	    if (i == (loc_count - 1))
	    {
		/* This is last entry, easy. */
		cnf->cnf_ext[i].length = 0;
		cnf->cnf_ext[i].type = 0;
	        cnf->cnf_dsc->dsc_ext_count--;
		
	    }		
	    else
	    {	    
		/* In middle of list, compress. */
		loc_count--;
		MEcopy((char *)&cnf->cnf_ext[i+1].ext_location.logical,
			       sizeof(DMP_LOC_ENTRY)*(loc_count-i),
                               (char *)&cnf->cnf_ext[i].ext_location.logical);

		/* Mark the end of list. */

		cnf->cnf_ext[loc_count].length = 0;
		cnf->cnf_ext[loc_count].type = 0;
	        cnf->cnf_dsc->dsc_ext_count--;

	    }
	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = 0;
	    break;    

	case DMVE_UNDO:

	    /*
	    ** Write CLR if necessary
	    */
	    if ((dmve->dmve_logging) &&
	        ((log_rec->loc_header.flags & DM0L_CLR) == 0))
	    {
		dm0l_flags = log_rec->loc_header.flags | DM0L_CLR;

		status = dm0l_del_location(dmve->dmve_log_id, dm0l_flags, 
		    log_rec->loc_type, &log_rec->loc_name, 
		    log_rec->loc_l_extent, &log_rec->loc_extent,
		    log_lsn, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    /* XXXX Better error message and continue after logging. */
		    TRdisplay(
		 "dmve_del_location: dm0l_del_location error, status: %d, error: %d\n", 
			status, dmve->dmve_error.err_code);
		    /*
		     * Bug56702: return logfull indication.
		     */
		    dmve->dmve_logfull = dmve->dmve_error.err_code; 
		    break;
		}
	    }

	    /*  Open the configuration file. */

	    l = dcb->dcb_ext->ext_entry;
	    loc_count = dcb->dcb_ext->ext_count;
	    for (i = 0; i < loc_count; i++, l++)
	    if ((MEcmp((char *)&l->logical, (char *)&log_rec->loc_name,
		sizeof(DB_LOC_NAME)) == 0)
		&& (l->flags == log_rec->loc_type))
		break;
	    if (i < loc_count)
	    {
		/* Found this entry, return error. */
		SETDBERR(&dmve->dmve_error, 0, E_DM007E_LOCATION_EXISTS);
		break;
	    }
		    
            status = dm0c_open(dcb, 0, lock_list, 
                                       &cnf, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = cnf;

	    /* Check if there is room. */

            if (cnf->cnf_free_bytes	< sizeof(DM0C_EXT))
	    {
		status = dm0c_extend(cnf, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM0071_LOCATIONS_TOO_MANY);
		    break;
		}
	    }


	    i = cnf->cnf_dsc->dsc_ext_count++;
	    cnf->cnf_ext[i].length = sizeof(DM0C_EXT);
	    cnf->cnf_ext[i].type = DM0C_T_EXT;
	    MEcopy((char *)&log_rec->loc_name, sizeof(DB_LOC_NAME),
                               (char *)&cnf->cnf_ext[i].ext_location.logical);
	    MEcopy((char *)&log_rec->loc_extent,  sizeof(DM_FILENAME),
			       (char *)&cnf->cnf_ext[i].ext_location.physical);
	    cnf->cnf_ext[i].ext_location.flags = log_rec->loc_type;
	    cnf->cnf_ext[i].ext_location.phys_length = log_rec->loc_l_extent;
	    cnf->cnf_ext[i+1].length = 0;
	    cnf->cnf_ext[i+1].type = 0;

	    /* Add new location info to DCB so RFP will be able to use it. */

	    dcb->dcb_ext->ext_count = cnf->cnf_dsc->dsc_ext_count;
	    STRUCT_ASSIGN_MACRO(cnf->cnf_ext[i].ext_location,
				dcb->dcb_ext->ext_entry[i]);

	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, DM0C_UPDATE, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;
	    config = 0;
	    break;    

	} /* end switch. */
	if (config != 0)
	{
	    (void) dm0c_close(cnf, 0, &local_dberr);
	}	
	if (status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    break;
	}
	return(E_DB_OK);

    } /* end for. */
    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9617_DMVE_LOCATION);
    }
    return(status);

}
